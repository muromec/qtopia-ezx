/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmapmgr.cpp,v 1.15 2005/09/04 15:38:32 dcollins Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

// no memory map support on VXWORKS or Symbian
#if !defined(_VXWORKS) && !defined(_SYMBIAN)

/*
 * MMapMgr needs atomic ref increments.
 */
#define _WIN32_USE_INTERLOCKED_INCREMENT
#include "hxcom.h"
#include "hxtypes.h"
#ifndef _MACINTOSH

#include <stdio.h>
#include <stdlib.h>

#ifdef _UNIX
#include <unistd.h>
#include <fcntl.h>
#endif

#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/io.h"

#if defined(_UNIX) && !defined(_BEOS)
#include <sys/mman.h>
#endif

#include "hxcom.h"
#include "hxtypes.h"
#include "hxbuffer.h"
#include "hxstrutl.h"
#include "hxmap.h"
#include "hxengin.h"
#include "mmapmgr.h"
#include "hxspriv.h"
#include "debug.h"
#include "mmapdatf.h"
#include "hxassert.h"
#include "hxmon.h"

#ifdef _MMM_NEED_MUTEX
#include "hxcomm.h"
#endif

//#define  MMAPDEBUG
//#define  MMAPMOREDEBUG

#ifdef MMAPDEBUG
#define MMAPDPRINTF(x) printf x
#else
#define MMAPDPRINTF(x)
#endif

/* Must be a multiple of the page size */
#define MMAP_INITIAL_CHUNK_SIZE						\
		(256 * 1024)

#define MMAP_EXTRA_SLOP_SIZE						\
		65536

#define MAX_ADDRESS_SPACE_USED						\
		1 * 1024 * 1000 * 1000	// 1 Gig

#define MS_BETWEEN_HANDLE_REAP_BUCKETS					\
		3000

#define NUMBER_OF_REAP_BUCKETS_TO_EMPTY_ON_ADDRESS_SPACE_EXHUSTED	\
		2


#ifndef MAP_FAIL
#define MAP_FAIL ((void *)-1)
#endif

/*
 * On Linux/NT we have 1 address space for all procs.  On other unicies we
 * have 'n' processes and 'n' address spaces.  This variables tracks address
 * usage correctly on all 3 types of OSes.
 */
UINT32     g_ulAddressSpaceUsed = 0;


/*
 *  On the server we wish to reap the memory after a little bit of time
 *  because it is very likely that we will want to use it again in a 
 *  short amount of time. On the client we do not wish to do this, becuase we
 *  can not do good things like delete the file if it is mem-mapped.
 */

static HXBOOL z_bDeterminedIfWeAreWithinServer    = FALSE;
static HXBOOL z_bWithinServer                     = FALSE;

/*
 * On NT we only have one MemoryMapManager per server.
 * On other unix platforms, we have on MemoryMapManager
 * per process.
 *
 * XXXSMP The Linux code is Sub-Optimal!!  Since we have 1 address space,
 * we should take advantage of the fact that pages that we are trying to
 * map in one thread may already be mapped by another thread.  With 'n'
 * MMM's, we can't take advantage of this optimization.
 */
MemoryMapManager::MemoryMapManager(IUnknown* pContext,
	HXBOOL bDisableMemoryMappedIO, UINT32 ulChunkSize)
    : m_pMMMCallback(NULL)
    , m_pMMappedDataSize(0)
{
    m_pDevINodeToFileInfoMap = new CHXMapStringToOb;
    m_pDevINodeToFileInfoMap->InitHashTable(517);
    m_ulActiveReapList = 0;

    HX_VERIFY (HXR_OK ==
	pContext->QueryInterface(IID_IHXScheduler, (void **)&m_pScheduler));

    if (!z_bDeterminedIfWeAreWithinServer)
    {
        z_bDeterminedIfWeAreWithinServer = TRUE;

        IHXRegistry* pHXReg = NULL;

        if (HXR_OK == pContext->QueryInterface(IID_IHXRegistry,
	    (void **)&pHXReg))
	{
            HXPropType type = pHXReg->GetTypeByName("server.version");
            if (type!= PT_UNKNOWN)
                z_bWithinServer = TRUE;
            HX_RELEASE(pHXReg);
	}
        IHXRegistry2* pHXReg2 = NULL;
        if (HXR_OK == pContext->QueryInterface(IID_IHXRegistry2,
	    (void **)&pHXReg2))
	{
	    pHXReg2->GetIntRefByName("server.MMappedDataSize",
		m_pMMappedDataSize);
            HX_RELEASE(pHXReg2);
	}
    }

    if (ulChunkSize)
    {
	m_ulChunkSize = ulChunkSize;
    }
    else
    {
	m_ulChunkSize = MMAP_INITIAL_CHUNK_SIZE;
    }
    m_lRefCount = 0;

    if (z_bWithinServer)
    {
	m_pMMMCallback = new MMMCallback(this);

	if (m_pMMMCallback)
	{
	    m_pMMMCallback->AddRef();
	    m_pMMMCallback->m_hPendingHandle = m_pScheduler->RelativeEnter(m_pMMMCallback, MS_BETWEEN_HANDLE_REAP_BUCKETS);
	}
    }

#ifdef _MMM_NEED_MUTEX
    m_pMutex = NULL;
#ifdef _DEBUG
    m_bHaveMutex = FALSE;
#endif
    IHXCommonClassFactory* pCCF;
    if (HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory,
	(void**)&pCCF))
    {
	pCCF->CreateInstance(CLSID_IHXMutex, (void**)&m_pMutex);
	pCCF->Release();
    }
#endif
    m_pFastAlloc = NULL;
    pContext->QueryInterface(IID_IHXFastAlloc, (void **)&m_pFastAlloc);
    m_bDisableMemoryMappedIO = bDisableMemoryMappedIO;
}


MemoryMapManager::~MemoryMapManager()
{
    /* This had better be empty by now!! */
    HX_DELETE(m_pDevINodeToFileInfoMap);

    if (m_pMMMCallback && m_pMMMCallback->m_hPendingHandle)
    {
	m_pScheduler->Remove(m_pMMMCallback->m_hPendingHandle);
    }
    HX_RELEASE(m_pMMMCallback);

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pFastAlloc);

#ifdef _MMM_NEED_MUTEX
    HX_RELEASE(m_pMutex);
#endif
}


STDMETHODIMP
MemoryMapManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


/*
 * These are the only AddRef/Release pairs
 * that need to be thread safe.  All the
 * other don't need to be.  Since _WIN32_USE_INTERLOCKED_INCREMENT
 * is defined at the top of this file, every instance will be actual
 * so for the ones that we don't want to use it we just do ++/--
 */
STDMETHODIMP_(ULONG32) 
MemoryMapManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32) 
MemoryMapManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;

#ifdef _WIN32
    MemoryMapDataFile::m_zpMMM = NULL;
#endif

    return 0;
}


void
MemoryMapManager::ProcessIdle()
{
    LockMutex();
    m_ulActiveReapList = (m_ulActiveReapList + 1) % NUMBER_OF_REAP_BUCKETS;

    EmptyReapBuckets();

    /*
     * Note: This callback is always called on Streamer #1 on NT, so the first
     * streamer to be spawned will reap dead pages for all other processes.
     */
    m_pMMMCallback->m_hPendingHandle =m_pScheduler->RelativeEnter(
	m_pMMMCallback, MS_BETWEEN_HANDLE_REAP_BUCKETS);

    UnlockMutex();
    return;
}


/*
 * This context is from the correct process to charge the descriptor usage
 * against.
 */
 
void*
MemoryMapManager::GetMMHandle(FILE_IDENTIFIER Descriptor)
{
    char pLookup[128]; /* Flawfinder: ignore */
    UINT32 ulSize = 0;

#ifdef _UNIX
    struct stat s;

    if (fstat(Descriptor, &s) == 0)
    {
	if (!s.st_dev || !s.st_ino)
	{
	    return 0;
	}

#ifdef _RED_HAT_5_X_
    /* Why make devices 64 bit??? */
	sprintf (pLookup, "%lld,%ld", s.st_dev, s.st_ino); /* Flawfinder: ignore */
#else
	sprintf (pLookup, "%d,%ld", s.st_dev, s.st_ino); /* Flawfinder: ignore */
#endif
	ASSERT(s.st_dev);
	ASSERT(s.st_ino);
	ulSize = (UINT32)s.st_size;

	MMAPDPRINTF(("%p: GetHandle %d %d %ld  ", this,
	    Descriptor, s.st_dev, s.st_ino));
    }
#elif defined(_WINDOWS)
    BY_HANDLE_FILE_INFORMATION FileInformation;

    if (GetFileInformationByHandle(Descriptor, &FileInformation))
    {
	sprintf (pLookup, "%x,%x,%x", FileInformation.nFileIndexLow, /* Flawfinder: ignore */
	    FileInformation.nFileIndexHigh, FileInformation.dwVolumeSerialNumber);

	ulSize = FileInformation.nFileSizeLow;

	MMAPDPRINTF(("%p: '%s'\n", Descriptor, pLookup));
    }
#endif

    if (ulSize == 0)
    {
	return 0;
    }

    void* pVoid = NULL;
    LockMutex();
    m_pDevINodeToFileInfoMap->Lookup(pLookup, pVoid);
    UnlockMutex();
    return pVoid;
}

void*
MemoryMapManager::OpenMap(FILE_IDENTIFIER Descriptor, IUnknown* pContext)
{
    char pLookup[128]; /* Flawfinder: ignore */
    UINT32 ulSize = 0;

    if (m_bDisableMemoryMappedIO)
    {
	return NULL;
    }

#ifdef _UNIX
    struct stat s;

    if (fstat(Descriptor, &s) == 0)
    {
	if (!s.st_dev || !s.st_ino)
	{
	    return 0;
	}

#ifdef _RED_HAT_5_X_
	/* Why make devices 64 bit??? */
	sprintf (pLookup, "%lld,%ld", s.st_dev, s.st_ino); /* Flawfinder: ignore */
#else
	sprintf (pLookup, "%d,%ld", s.st_dev, s.st_ino); /* Flawfinder: ignore */
#endif
	ASSERT(s.st_dev);
	ASSERT(s.st_ino);
	ulSize = (UINT32)s.st_size;

	MMAPDPRINTF(("%p: OpenMap %d %d %ld  ", this,
	    Descriptor, s.st_dev, s.st_ino));
    }
#elif defined(_WINDOWS)
    BY_HANDLE_FILE_INFORMATION FileInformation;

    if (GetFileInformationByHandle(Descriptor, &FileInformation))
    {
	sprintf (pLookup, "%x,%x,%x", FileInformation.nFileIndexLow, /* Flawfinder: ignore */
	    FileInformation.nFileIndexHigh, FileInformation.dwVolumeSerialNumber);

	ulSize = FileInformation.nFileSizeLow;

	MMAPDPRINTF(("%p: '%s'\n", Descriptor, pLookup));
    }
#endif

    if (ulSize == 0)
    {
	return 0;
    }

    void* pVoid = NULL;
    struct _FileInfo* pInfo = NULL;

    LockMutex();
    m_pDevINodeToFileInfoMap->Lookup(pLookup, pVoid);
    if (pVoid)
    {
	MMAPDPRINTF(("(OpenMap Found Already)\n"));
	pInfo = (struct _FileInfo*) pVoid;
	HX_ASSERT (pInfo->Descriptor != 0);
	pInfo->ulRefCount++;
	pInfo->ulUseCount++;
	/* In case the file size has changed */
	pInfo->ulSize = ulSize;

	UnlockMutex();
	return pVoid;
    }
    else
    {
	MMAPDPRINTF(("(OpenMap New)\n"));
#ifdef _UNIX
	m_pDevINodeToFileInfoMap->SetAt(pLookup, pInfo = new struct _FileInfo);
	pInfo->Descriptor = dup(Descriptor);
#else

	HANDLE hTest = CreateFileMapping(Descriptor, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!hTest)
	{
	    /*
	     * Try to open the mapping for read only.  This is supposed to handle
	     * readonly files.  If the file was opened for write then this
	     * will fail anyway, so we can't really do the wrong thing here.
	     */
	    hTest = CreateFileMapping(Descriptor, NULL, PAGE_READONLY, 0, 0, NULL);
	    if (!hTest)
	    {
		UnlockMutex();
		return NULL;
	    }
	}
	m_pDevINodeToFileInfoMap->SetAt(pLookup, pInfo = new struct _FileInfo);
	pInfo->Descriptor = hTest;
#endif
	SafeStrCpy(pInfo->pKey, pLookup, FILEINFO_KEY_SIZE);
	pInfo->ulSize = ulSize;
	pInfo->pMgr = this;
	pInfo->pMgr->AddRef();
	pInfo->ulRefCount = 1;
	pInfo->ulUseCount = 1;
#ifdef _WIN32
	pInfo->m_pPTEList = NULL;
#endif
	memset(pInfo->pPageTable, 0, sizeof (pInfo->pPageTable));
	if (HXR_OK == pContext->QueryInterface(IID_IHXDescriptorRegistration,
	    (void **)&pInfo->pDescReg))
	{
	    pInfo->pDescReg->RegisterDescriptors(1);
	}
	else
	{
	    pInfo->pDescReg = NULL;
	}

	UnlockMutex();
	return (void *)pInfo;
    }
}


#ifdef _WIN32
void
MemoryMapManager::AttemptCloseMapNow(void* pHandle)
{
    LockMutex();
    struct _FileInfo* pInfo = (struct _FileInfo*)pHandle;
    _PageTableEntry* pPTE;
    _PageTableEntry* pNextPTE;
    pNextPTE = pInfo->m_pPTEList;
    while (pNextPTE)
    {
	pPTE = pNextPTE;
	pNextPTE = pPTE->m_pNextPTE;
	pPTE->bReapMe = TRUE;
	UINT32 ulPTESize = pPTE->ulSize;
	CheckAndReapPageTableEntry(pPTE);
    }
    UnlockMutex();
}
#endif

void
MemoryMapManager::CloseMap(void* pHandle)
{
    LockMutex();
    MMAPDPRINTF(("CloseMap\n"));
    struct _FileInfo* pInfo = (struct _FileInfo*)pHandle;
    pInfo->ulRefCount--;
    pInfo->ulUseCount--;

    if (pInfo->ulRefCount == 0)
    {
        DestroyFileInfo(pHandle);
    }

#if 0
    /*
     * This code is really slow and not needed.  These pages will be
     * reaped shortly by the timed reaper.
     */
    if (pInfo->ulUseCount == 0)
    {
	for (int i = 0; i < NUM_PTES; i++)
	{
	    if (pInfo->pPageTable[i] == NULL)
	    {
		continue;
	    }

	    struct _PageTableEntry* pIE = 0;

	    for (int j = 0; j < NUM_PTES; j++)
	    {
		pIE = (struct _PageTableEntry*)
		    &(pInfo->pPageTable[i]->pEntry[j]);
		if (pIE->bActive == FALSE)
		{
		    continue;
		}
		pIE->bReapMe = TRUE;
		UINT32 ulPTESize = pIE->ulSize;
		CheckAndReapPageTableEntry(pIE);
		if (pInfo->pPageTable[i] == NULL)
		{
		    break;
		}
	    }
	}
    }
#endif
    MMAPDPRINTF(("Done CloseMap\n"));

    UnlockMutex();
}


/*
 * On _WIN32 whoever calls this MUST have the mUTex!!!
 */
void MemoryMapManager::EmptyReapBuckets()
{
#if defined  _MMM_NEED_MUTEX && defined _DEBUG
    HX_ASSERT(m_bHaveMutex);
#endif

    UINT32 ulListToReap = (m_ulActiveReapList + 1) % NUMBER_OF_REAP_BUCKETS;

    if (!ReapBuckets[ulListToReap].IsEmpty())
    {
	CHXSimpleList* pList = &ReapBuckets[ulListToReap];
	struct _PageTableEntry* pIE = 0;
	LISTPOSITION l = pList->GetHeadPosition();
	LISTPOSITION temp;

	while (l)
	{
	    pIE = (struct _PageTableEntry*)pList->GetAt(l);
	    pIE->bReapMe = TRUE;
	    temp = l;
	    pList->GetNext(l);
	    UINT32 ulPTESize = pIE->ulSize;
	    if (CheckAndReapPageTableEntry(pIE) == FALSE)
	    {
		pIE->bDeadPage = TRUE;
		pList->RemoveAt(temp);
	    }
	}
    }
}


UINT32
MemoryMapManager::GetBlock(REF(IHXBuffer*) pBuffer, void* pHandle,
    UINT32 ulOffset, UINT32 ulSize)
{
    LockMutex();
    struct _FileInfo* pInfo = (struct _FileInfo*)pHandle;
    UINT32 ulNeededPageNumber = (ulOffset / m_ulChunkSize);
    UINT32 ulNeededL1Entry = ulNeededPageNumber / NUM_PTES;
    UINT32 ulNeededL2Entry = ulNeededPageNumber % NUM_PTES;
    struct _PageTableEntry* pEntry = 0;

#ifdef MMAPMOREDEBUG
    MMAPDPRINTF(("%p GetBlock %d %d, Page %d/%d\n", pHandle, ulOffset, ulSize,
	    ulNeededL1Entry, ulNeededL2Entry));
    fflush(0);
#endif

    if (ulOffset >= pInfo->ulSize)
    {
	/* Maybe the file has grown */
#ifdef _UNIX
	struct stat s;
	if (fstat(pInfo->Descriptor, &s) == 0)
	{
	    pInfo->ulSize = (UINT32)s.st_size;
	}
#elif defined(_WINDOWS)
        BY_HANDLE_FILE_INFORMATION FileInformation;
        if (GetFileInformationByHandle(pInfo->Descriptor, &FileInformation))
	{
	    pInfo->ulSize = FileInformation.nFileSizeLow;
	}
#endif

	if (ulOffset >= pInfo->ulSize)
	{
	    pBuffer = NULL;
	    UnlockMutex();
	    return (ULONG32) MMAP_EOF_EXCEPTION;
	}
    }

    if (pInfo->pPageTable[ulNeededL1Entry])
    {
	if (pInfo->pPageTable[ulNeededL1Entry]->pEntry[ulNeededL2Entry].bActive)
	{
	    pEntry = &pInfo->pPageTable[ulNeededL1Entry]->
		pEntry[ulNeededL2Entry];
	}
    }

    UCHAR* ulDataPointer = 0;

    if (!pEntry)
    {
	UINT32 ulEmptyAttempts = 0;

	while (g_ulAddressSpaceUsed > MAX_ADDRESS_SPACE_USED &&
	    ulEmptyAttempts++ <
	    NUMBER_OF_REAP_BUCKETS_TO_EMPTY_ON_ADDRESS_SPACE_EXHUSTED)
	{
	    /*
	     * Attempt to unmap some old pages and hopefully free up
	     * some address space.
	     */
	    m_ulActiveReapList = (m_ulActiveReapList + 1)
		% NUMBER_OF_REAP_BUCKETS;

	    EmptyReapBuckets();
	}

	if (g_ulAddressSpaceUsed > MAX_ADDRESS_SPACE_USED)
	{
	    MMAPDPRINTF((" Address Space Exceeded, Exception\n"));
	    UnlockMutex();
	    return MMAP_EXCEPTION;
	}

	if (pInfo->pPageTable[ulNeededL1Entry] == NULL)
	{
	    pInfo->pPageTable[ulNeededL1Entry] = 
		new struct MemoryMapManager::_PageTableLevel1;
	    memset(pInfo->pPageTable[ulNeededL1Entry], 0, sizeof(
		struct MemoryMapManager::_PageTableLevel1));
	    pInfo->pPageTable[ulNeededL1Entry]->
		ulNumberOfPageTableEntriesInUse = 0;
	    pInfo->pPageTable[ulNeededL1Entry]->
		pMyEntryInParentsPageTable =
		&pInfo->pPageTable[ulNeededL1Entry];
#ifdef _WIN32
	    pInfo->m_pPTEList = NULL;
#endif
	}
	pInfo->pPageTable[ulNeededL1Entry]->
	    ulNumberOfPageTableEntriesInUse++;
	pEntry = &pInfo->pPageTable[ulNeededL1Entry]->pEntry[ulNeededL2Entry];
	pEntry->bActive = TRUE;
	pEntry->bReapMe = FALSE;
	pEntry->bDeadPage = FALSE;
	pEntry->ulPageRefCount = 0;
	pEntry->pParent = pInfo->pPageTable[ulNeededL1Entry];
	pEntry->pInfo = pInfo;
	

	UINT32 ulChunkSize = m_ulChunkSize + MMAP_EXTRA_SLOP_SIZE;
	if (ulChunkSize + ulNeededPageNumber * m_ulChunkSize > pInfo->ulSize)
	{
	    ulChunkSize = pInfo->ulSize - ulNeededPageNumber * m_ulChunkSize;
	}
	
	pEntry->ulSize = ulChunkSize;
	pInfo->ulRefCount++;

#ifdef _BEOS	// no mmap for BeOS yet...
	pEntry->pPage = MAP_FAIL;
#else
#ifdef _UNIX
	pEntry->pPage =
	    mmap(0, ulChunkSize, PROT_READ, MAP_PRIVATE, 
	    pInfo->Descriptor, ulNeededPageNumber * m_ulChunkSize);
#else
	pEntry->pPage = MapViewOfFile(pInfo->Descriptor, FILE_MAP_READ, 0,
	    ulNeededPageNumber * m_ulChunkSize, ulChunkSize);
	if (pEntry->pPage == 0)
	{
	    pEntry->pPage = MAP_FAIL;
	}
	else
	{
#if !defined(HELIX_FEATURE_SERVER) && defined(_WINDOWS)
            // When MapViewOfFile is called it returns a handle to a page of memory that
            // the system no longer knows about so an exception occurs.  It is usally a 
            // EXCEPTION_IN_PAGE_ERROR. Reading the documentation it appears that all access
            // to handles returned from memory mapped I/O should be wrapped in try/catch blocks 
            // since the handles may be invalid.  I added try/catch logic to test this out and
            // it fixes the bug.  The main problem I'm not sure about is that this code is also 
            // used by the server and IsBadReadPtr may iterate over the memory block which can 
            // be slow.  Because this is the layer of code that "knows" it is using memory mapped 
            // I/O this is probably where the try/catch code belongs so that if an exception
            // occurs then it can return an error and no buffer since the buffer wouldn't be
            // accessible anyway.
            HXBOOL bInvalid = TRUE;

            try
            {
               bInvalid = ::IsBadReadPtr(pEntry->pPage, ulChunkSize);
            }
            catch (...)
            {
            }

            if (bInvalid)
            {
               pEntry->pPage = MAP_FAIL;
            }
            else
            {
                if (pInfo->m_pPTEList)
                {
                    pInfo->m_pPTEList->m_pPrevPTE = pEntry;
                }
                pEntry->m_pNextPTE = pInfo->m_pPTEList;
                pEntry->m_pPrevPTE = NULL;
                pInfo->m_pPTEList = pEntry;
            }
#else
	    if (pInfo->m_pPTEList)
	    {
		pInfo->m_pPTEList->m_pPrevPTE = pEntry;
	    }
	    pEntry->m_pNextPTE = pInfo->m_pPTEList;
	    pEntry->m_pPrevPTE = NULL;
	    pInfo->m_pPTEList = pEntry;
#endif /* !HELIX_FEATURE_SERVER && _WINDOWS */
	}
#endif
#endif /* _BEOS */

	MMAPDPRINTF(("MMAP from %d Size %ld Pos %ld = %p (entry %p)\n",
	    pInfo->Descriptor, ulChunkSize,
	    ulNeededPageNumber * m_ulChunkSize,
	    pEntry->pPage, pEntry));

	g_ulAddressSpaceUsed += ulChunkSize;
        if (m_pMMappedDataSize)
            HXAtomicAddINT32(m_pMMappedDataSize, ulChunkSize);

	pEntry->usReapListNumber = m_ulActiveReapList;
	pEntry->ReapListPosition = ReapBuckets[m_ulActiveReapList].
	    AddHead(pEntry);

	if (pEntry->pPage == MAP_FAIL)
	{
	    pBuffer = NULL;
	    pEntry->bReapMe = TRUE;
	    UINT32 ulPTESize = pEntry->ulSize;
	    CheckAndReapPageTableEntry(pEntry);
	    UnlockMutex();
	    return (UINT32) MMAP_EXCEPTION;
	}
    }
    else
    {
	if (pEntry->bDeadPage ||
	    (m_ulActiveReapList != pEntry->usReapListNumber))
	{
	    if (pEntry->bDeadPage)
	    {
		MMAPDPRINTF(("UnDeadPage to %p!\n", pEntry));
	    	pEntry->bDeadPage = FALSE;
	    }
	    else
	    {
		ReapBuckets[pEntry->usReapListNumber].
		    RemoveAt(pEntry->ReapListPosition);
	    }
	    pEntry->usReapListNumber = m_ulActiveReapList;
	    pEntry->ReapListPosition = ReapBuckets[m_ulActiveReapList].
		AddHead(pEntry);
	}
	
	pEntry->bReapMe = FALSE;
    }

    ulDataPointer = ((UCHAR *) pEntry->pPage) + ulOffset % m_ulChunkSize;

    /*
     * Go back to normal read() if we are asking for something past
     * the mapped region.
     */
    if ((ulOffset + ulSize > pInfo->ulSize) ||
        (ulOffset % m_ulChunkSize + ulSize > pEntry->ulSize))
    {
	/*
	 * If the file has grown, then the region we mapped may not be
	 * large enough, but we can remap it.  This is a rare optimization
	 * that we can do without.
	 */
	HXBOOL bEOF = (ulOffset + ulSize > pInfo->ulSize);

	MMAPDPRINTF((" Memory Map Page Overrun, Exception\n"));

	pBuffer = NULL;

	if (!z_bWithinServer)
	{
	    // Do not defer clean-up of unused pages when in client
	    pEntry->bReapMe = TRUE;
	    UINT32 ulPTESize = pEntry->ulSize;
	    CheckAndReapPageTableEntry(pEntry);
	}

	UnlockMutex();

	if (bEOF)
	{
	    return MMAP_EOF_EXCEPTION;
	}
	else
	{
	    return MMAP_EXCEPTION;
	}
    }

    pBuffer = new(m_pFastAlloc) Buffer(pEntry, ulDataPointer, ulSize);
    if(pBuffer)
    {
        pBuffer->AddRef();
    }
    else
    {
        ulSize = 0;
    }

    UnlockMutex();
    return ulSize;
}


/*
 * On _WIN32 you MUST have the mutex to call this!
 */
HXBOOL
MemoryMapManager::CheckAndReapPageTableEntry(struct _PageTableEntry* pPTE)
{
#if defined  _MMM_NEED_MUTEX && defined _DEBUG
    HX_ASSERT(pPTE->pInfo->pMgr->m_bHaveMutex);
#endif

    struct _FileInfo* pInfo = pPTE->pInfo;

    if (pPTE->ulPageRefCount == 0 && pPTE->bReapMe)
    {
	MMAPDPRINTF(("Unmap Chunk %p %ld  ", pPTE->pPage, pPTE->ulSize));

	if (pPTE->pPage != MAP_FAIL)
	{
#ifdef _BEOS
	    // no memory mapped IO for BeOS yet!
#else
#ifdef _UNIX
#ifdef _SOLARIS
	    munmap((char*)(pPTE->pPage), pPTE->ulSize);
#else
	    munmap(pPTE->pPage, pPTE->ulSize);
#endif
#else
	    UnmapViewOfFile(pPTE->pPage);
	    if (pPTE->m_pPrevPTE)
	    {
		pPTE->m_pPrevPTE->m_pNextPTE = pPTE->m_pNextPTE;
	    }
	    else
	    {
		pInfo->m_pPTEList = pPTE->m_pNextPTE;
	    }
	    if (pPTE->m_pNextPTE)
	    {
		pPTE->m_pNextPTE->m_pPrevPTE = pPTE->m_pPrevPTE;
	    }
#endif
#endif /* _BEOS */
	}

	pInfo->ulRefCount--;
	g_ulAddressSpaceUsed -= pPTE->ulSize;
        if (pInfo->pMgr->m_pMMappedDataSize)
            HXAtomicSubINT32(pInfo->pMgr->m_pMMappedDataSize, pPTE->ulSize);

	MMAPDPRINTF((" (Down to %0.2f), Reap %u\n",
	    g_ulAddressSpaceUsed
	    / (1.0 * pInfo->pMgr->m_ulChunkSize + MMAP_EXTRA_SLOP_SIZE),
	    pPTE->bReapMe));
	pPTE->bActive = FALSE;

	if (pPTE->bDeadPage == FALSE)
	{
	    pInfo->pMgr->ReapBuckets[pPTE->usReapListNumber].
		RemoveAt(pPTE->ReapListPosition);
	}

	if (--pPTE->pParent->ulNumberOfPageTableEntriesInUse == 0)
	{
	    struct _PageTableLevel1** pPTL1 =
		pPTE->pParent->pMyEntryInParentsPageTable;
	    *pPTL1 = 0;
	    delete pPTE->pParent;
	}

        if (pInfo->ulRefCount == 0)
            DestroyFileInfo((void*)pInfo);

	return TRUE;
    }

    return FALSE;
}

/*
 * On _WIN32 you MUST have the mutex to call this!
 */
void
MemoryMapManager::DestroyFileInfo(void* pHandle)
{
#if defined  _MMM_NEED_MUTEX && defined _DEBUG
    HX_ASSERT(((struct _FileInfo*)pHandle)->pMgr->m_bHaveMutex);
#endif

    struct _FileInfo* pInfo = (struct _FileInfo*)pHandle;

    MMAPDPRINTF(("Remove %s from %p\n", pInfo->pKey,
	pInfo->pMgr->m_pDevINodeToFileInfoMap));
    pInfo->pMgr->m_pDevINodeToFileInfoMap->RemoveKey
	((const char *)pInfo->pKey);

    /*
     * Don't use MemoryMapManager's context!  You will credit the wrong
     * process.
     */
    if (pInfo->pDescReg)
    {
	pInfo->pDescReg->UnRegisterDescriptors(1);
	HX_RELEASE(pInfo->pDescReg);
    }

#ifdef _UNIX
    close(pInfo->Descriptor);
#else
    CloseHandle(pInfo->Descriptor);
#endif

    HX_RELEASE(pInfo->pMgr);
    delete pInfo;
}


MemoryMapManager::Buffer::Buffer(struct _PageTableEntry* pEntry, UCHAR* pData,
    ULONG32 ulLength)
    : m_lRefCount(0)
    , m_ulLength(ulLength)
    , m_pData(pData)
    , m_pPTE(pEntry)
{
    ASSERT(m_pPTE);
    m_pPTE->ulPageRefCount++;
}


MemoryMapManager::Buffer::~Buffer()
{
    MemoryMapManager* pMgr = m_pPTE->pInfo->pMgr;
    pMgr->LockMutex();

    m_pPTE->ulPageRefCount--;
    if (!z_bWithinServer && !m_pPTE->ulPageRefCount)
        m_pPTE->bReapMe = TRUE;
    UINT32 ulPTESize = m_pPTE->ulSize;
    MemoryMapManager::CheckAndReapPageTableEntry(m_pPTE);

    pMgr->UnlockMutex();
}


STDMETHODIMP
MemoryMapManager::Buffer::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IUnknown*)this },
		{ GET_IIDHANDLE(IID_IHXBuffer), (IUnknown*)(IHXBuffer*)this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}


STDMETHODIMP_(ULONG32) 
MemoryMapManager::Buffer::AddRef()
{
    m_lRefCount++;
    return m_lRefCount;
}


STDMETHODIMP_(ULONG32) 
MemoryMapManager::Buffer::Release()
{
    m_lRefCount--;
    if (m_lRefCount > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
MemoryMapManager::Buffer::Get(REF(UCHAR*) pData, REF(ULONG32) ulLength)
{
    pData = m_pData;
    ulLength = m_ulLength;

    return HXR_OK;
}


STDMETHODIMP
MemoryMapManager::Buffer::Set(const UCHAR* pData, ULONG32 ulLength)
{
    /* XXXSMP We should support this. */
    PANIC(("Internal Error mmgr/620"));
    return HXR_UNEXPECTED;
}


STDMETHODIMP
MemoryMapManager::Buffer::SetSize(ULONG32 ulLength)
{
    /* XXXSMP We should support this. */
    if (ulLength <= m_ulLength)
    {
	m_ulLength = ulLength;
	return HXR_OK;
    }
    else
    {
    PANIC(("Internal Error mmgr/635"));
	return HXR_UNEXPECTED;
    }
}


STDMETHODIMP_(ULONG32)
MemoryMapManager::Buffer::GetSize()
{
    return m_ulLength;
}


STDMETHODIMP_(UCHAR*)
MemoryMapManager::Buffer::GetBuffer()
{
    return m_pData;
}

// MemoryMapManager callback

MMMCallback::MMMCallback(MemoryMapManager* pMMM)
   :m_lRefCount(0)
   ,m_pMMM(pMMM)
   ,m_hPendingHandle(0)
{
}

MMMCallback::~MMMCallback()
{
}

STDMETHODIMP
MMMCallback::QueryInterface(REFIID riid, void**ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
MMMCallback::AddRef()
{
    m_lRefCount++;
    return m_lRefCount;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
MMMCallback::Release()
{
    m_lRefCount--;
    if (m_lRefCount > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
MMMCallback::Func(void)
{
    m_hPendingHandle = 0;

    if (m_pMMM)
    {
	m_pMMM->AddRef();
	m_pMMM->ProcessIdle();
	m_pMMM->Release();
    }

    return HXR_OK;
}

#endif
#endif /* _VXWORKS */

