/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmapmgr.h,v 1.10 2007/07/06 20:35:11 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _MMAPMGR_H_
#define _MMAPMGR_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxbuffer.h"
#include "hxslist.h"
#include "hxengin.h"
#include "hxcomm.h"
#include "hxmap.h"

struct IHXScheduler;
struct IHXDescriptorRegistration;

#define NUMBER_OF_REAP_BUCKETS				3
#define MMAP_EXCEPTION                                  0xfffffff1
#define MMAP_EOF_EXCEPTION  				0xfffffff2
#define NUM_PTES					128

#if defined(_UNIX) || defined(_SYMBIAN)
#define FILE_IDENTIFIER					int
#else
#define FILE_IDENTIFIER					HANDLE
#endif

#if defined _WIN32 || defined(HELIX_FEATURE_THREADSAFE_MEMMAP_IO)
#define _MMM_NEED_MUTEX
#endif

class MMMCallback;

class MemoryMapManager : public IUnknown
{
public:
    MemoryMapManager(IUnknown* pContext, HXBOOL bDisableMemoryMappedIO = 0,
	    UINT32 ulChunkSize = 0);
    virtual ~MemoryMapManager();

    void*   OpenMap(FILE_IDENTIFIER Descriptor, IUnknown* pContext);
    void    CloseMap(void* pHandle);
#ifdef _WIN32
    void    AttemptCloseMapNow(void* pHandle);
#endif
    void*   GetMMHandle(FILE_IDENTIFIER Descriptor);
    UINT32  GetBlock(REF(IHXBuffer*) pBuffer, void* pHandle,
	UINT32 ulOffset, UINT32 ulSize);
    static void DestroyFileInfo(void* pHandle);

    STDMETHOD(QueryInterface)   (THIS_
				 REFIID riid,
				 void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    void ProcessIdle(void);

    INT32*	 m_pMMappedDataSize;
    MMMCallback* m_pMMMCallback;
    friend class MMMCallback;

    struct _FileInfo;
    struct _PageTableLevel1;

    struct _PageTableEntry
    {
	UINT32			    ulPageRefCount;
	UINT32			    ulSize;
	void*			    pPage;

	/* Page is active (i.e. this entry is valid) */
	unsigned char		    bActive : 1;

	/* Page should be reaped in CheckAndReapPageTableEntry() */
	unsigned char		    bReapMe : 1;

	/* Page is dead and will be reaped by refcount==0 */
	unsigned char		    bDeadPage : 1;

	UINT8			    usReapListNumber : 8;
	LISTPOSITION		    ReapListPosition;
	struct _FileInfo*	    pInfo;
	struct _PageTableLevel1*    pParent;
#ifdef _WIN32
	_PageTableEntry*	    m_pNextPTE;
	_PageTableEntry*	    m_pPrevPTE;
#endif
    };
    struct _PageTableLevel1
    {
	struct _PageTableEntry	    pEntry[NUM_PTES];
	UINT32			    ulNumberOfPageTableEntriesInUse;
	struct _PageTableLevel1**   pMyEntryInParentsPageTable;
    };

#define FILEINFO_KEY_SIZE   32

    struct _FileInfo
    {
	FILE_IDENTIFIER		    Descriptor;
	UINT32		    	    ulSize;
	UINT32		    	    ulRefCount;
	UINT32		    	    ulUseCount;
	char		    	    pKey[FILEINFO_KEY_SIZE]; /* Flawfinder: ignore */
	MemoryMapManager*   	    pMgr;
	struct _PageTableLevel1*    pPageTable[NUM_PTES];
	IHXDescriptorRegistration* pDescReg;
#ifdef _WIN32
	_PageTableEntry*	    m_pPTEList;
#endif
    };

    static HXBOOL CheckAndReapPageTableEntry(struct _PageTableEntry* pPTE);
private:
    class Buffer : public IHXBuffer
    {
    public:
	FAST_CACHE_MEM
	Buffer(struct _PageTableEntry* pEntry, UCHAR* pData, ULONG32 ulLength);
	virtual ~Buffer();

	STDMETHOD(QueryInterface)   (THIS_
                                     REFIID riid,
                                     void** ppvObj);
	STDMETHOD_(ULONG32,AddRef)  (THIS);
	STDMETHOD_(ULONG32,Release) (THIS);

	STDMETHOD(Get)              (THIS_
                                    REF(UCHAR*) pData, 
                                    REF(ULONG32) ulLength);
	STDMETHOD(Set)              (THIS_
                                    const UCHAR* pData, 
                                    ULONG32 ulLength);

	STDMETHOD(SetSize)          (THIS_
                                    ULONG32 ulLength);

	STDMETHOD_(ULONG32,GetSize) (THIS);

	STDMETHOD_(UCHAR*,GetBuffer)(THIS);
    private:
	LONG32					    m_lRefCount;
	ULONG32					    m_ulLength;
	UCHAR*					    m_pData;
#ifdef _WINDOWS
	_PageTableEntry*			    m_pPTE;
#else
	class MemoryMapManager::_PageTableEntry*   m_pPTE;
#endif
    };

    void	EmptyReapBuckets();

    CHXSimpleList	ReapBuckets[NUMBER_OF_REAP_BUCKETS];
    CHXMapStringToOb*	m_pDevINodeToFileInfoMap;
    UINT8		m_ulActiveReapList;
    IHXScheduler*	m_pScheduler;
    UINT32		m_ulChunkSize;
    INT32		m_lRefCount;
    CallbackHandle	m_PendingHandle;

    /*
     * Buffer needs to call back into here
     * on delete.
     */
    friend class MemoryMapManager::Buffer;
    void		LockMutex();
    void		UnlockMutex();
#ifdef _MMM_NEED_MUTEX
    IHXMutex*		m_pMutex;
#ifdef _DEBUG
    HXBOOL		m_bHaveMutex;
#endif
#endif
    IHXFastAlloc*	m_pFastAlloc;
    HXBOOL		m_bDisableMemoryMappedIO;
};

class MMMCallback : public IHXCallback
{
public:
   
    MemoryMapManager*	m_pMMM;
    CallbackHandle	m_hPendingHandle;

			MMMCallback(MemoryMapManager* pMMM);
			~MMMCallback();
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXCallback methods
     */
    STDMETHOD(Func)		(THIS);

protected:
    LONG32		m_lRefCount;
};

#ifdef _MMM_NEED_MUTEX
/*
 * If you are in here your os has one mmapmgr for the whole 
 * server.
 */
inline void
MemoryMapManager::LockMutex()
{
    AddRef();
    if (m_pMutex)
    {
	m_pMutex->Lock();
    }
#ifdef _DEBUG
    m_bHaveMutex = TRUE;
#endif
}

inline void
MemoryMapManager::UnlockMutex()
{
#ifdef _DEBUG
    m_bHaveMutex = FALSE;
#endif
    if (m_pMutex)
    {
	m_pMutex->Unlock();
    }
    Release();
}
#else
/*
 * If you are in here then each server process has its
 * own mmapmgr.
 */
inline void
MemoryMapManager::LockMutex()
{
}

inline void
MemoryMapManager::UnlockMutex()
{
}
#endif

#endif /* _MMAPMGR_H_ */
