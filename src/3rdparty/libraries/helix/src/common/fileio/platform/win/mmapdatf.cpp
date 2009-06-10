/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmapdatf.cpp,v 1.14 2007/07/06 20:35:17 jfinnecy Exp $
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

#include "hxcom.h"
#include "hxresult.h"
#include "hxtypes.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/sys/types.h"
#include "hlxclib/io.h"
#include "hlxclib/fcntl.h"
#include "hlxclib/windows.h"
#include "hxbuffer.h"
#include "mmapdatf.h"
#include "mmapmgr.h"
#include "hxfiles.h"
#include "hxprefs.h"
#include "filespec.h"
#include "filespecutils.h"
#include "platform/win/filetimeutil.h"
#include "hxcore.h"
#include "pckunpck.h"

MemoryMapManager* MemoryMapDataFile::m_zpMMM = NULL;
HXBOOL MemoryMapDataFile::g_NT = FALSE;

MemoryMapDataFile::MemoryMapDataFile(IUnknown* pContext,
    REF(IUnknown*) pPersistantObject, HXBOOL bDisableMemoryMappedIO,
    UINT32 ulChunkSize, HXBOOL bEnableFileLocking)
    : m_lRefCount(0)
    , m_hFile(INVALID_HANDLE_VALUE)
    , m_pFilename(0)
    , m_ulPos(-1)
    , MmapHandle(0)
    , m_ulChunkSize(ulChunkSize)
    , m_ulLastError(HXR_OK)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    if (m_zpMMM == NULL)
    {
	m_zpMMM = new MemoryMapManager(pContext, bDisableMemoryMappedIO,
		m_ulChunkSize);
	OSVERSIONINFO vinfo;
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);
	if (GetVersionEx(&vinfo))
	{
	    if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
	    {
		g_NT = TRUE;
	    }
	}
    }

    if (!pPersistantObject)
    {
	pPersistantObject = m_zpMMM;
	pPersistantObject->AddRef();
    }
}

MemoryMapDataFile::~MemoryMapDataFile()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }
    if (m_pFilename)
    {
	delete[] m_pFilename;
	m_pFilename = 0;
    }

    HX_RELEASE(m_pContext);
}

/*
 *  IUnknown methods
 */
STDMETHODIMP
MemoryMapDataFile::QueryInterface(REFIID riid,
			void** ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if(IsEqualIID(IID_IHXDataFile, riid))
    {
	AddRef();
	*ppvObj = (IHXDataFile*)this;
	return HXR_OK;
    }
    *ppvObj = 0;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(ULONG32)
MemoryMapDataFile::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}
    
/*
 *  IHXDataFile methods
 */

/* Bind DataFile Object with FileName */
STDMETHODIMP_(void)
MemoryMapDataFile::Bind(const char* FileName)
{
    /* XXXPM
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }*/

    if (m_pFilename)
    {
	delete[] m_pFilename;
	m_pFilename = 0;
    }
    m_pFilename = new char[strlen(FileName) + 1];
    strcpy(m_pFilename, FileName); /* Flawfinder: ignore */
}


/* Creates a datafile using the specified mode
 * uOpenMode --- File Open mode - HX_FILE_READ/HX_FILE_WRITE/HX_FILE_BINARY
 */
STDMETHODIMP
MemoryMapDataFile::Create(UINT16 uOpenMode)
{
    return HXR_NOTIMPL;
}

/*
 *  Open will open a file with the specified mode
 */
STDMETHODIMP
MemoryMapDataFile::Open(UINT16 uOpenMode)
{
    m_ulLastError = HXR_OK;

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }
    
    DWORD dwAccess = 0;
    DWORD dwShare = 0;
    DWORD dwCreate = 0;
    DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
    m_uOpenMode = uOpenMode;
    if (uOpenMode & HX_FILE_WRITE)
    {
        // Set read access so file can be opened read-only simultaneously
        uOpenMode |= HX_FILE_READ;
	dwAccess |= GENERIC_WRITE;
	dwShare |= FILE_SHARE_WRITE;
	if (g_NT)
	{
	    dwShare |= FILE_SHARE_DELETE;
	}
	if (uOpenMode & HX_FILE_NOTRUNC)
	{
	    dwCreate = OPEN_ALWAYS;
	}
	else
	{
	    dwCreate = CREATE_ALWAYS;
	}
    }

    if (uOpenMode & HX_FILE_READ)
    {
	dwAccess |= GENERIC_READ;
	dwShare |= FILE_SHARE_READ;
	if (!(uOpenMode & HX_FILE_WRITE))
	{
	    dwCreate = OPEN_EXISTING;
	}

	IHXPreferences* pPref = NULL;
	if (m_pContext && 
	    (m_pContext->QueryInterface(IID_IHXPreferences, (void**) &pPref) == HXR_OK) &&
	    pPref)
	{
	    IHXBuffer* pBuffer = NULL;
	    if (pPref->ReadPref("FileReadWriteAccess", pBuffer) == HXR_OK &&
		pBuffer &&
		(::atoi((char*) pBuffer->GetBuffer()) == 1))
	    {
		dwShare |= FILE_SHARE_WRITE;

		if (g_NT)
		{
		    dwShare |= FILE_SHARE_DELETE;
		}
	    }

	    HX_RELEASE(pBuffer);
	    HX_RELEASE(pPref);
	}
    }

    /*
     * Nothing to be done for HX_FILE_BINARY
     */
    m_hFile = CreateFile(OS_STRING(m_pFilename), dwAccess, dwShare, NULL,
	    dwCreate, dwFlags, NULL);
    
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        if (::GetLastError() == ERROR_FILE_NOT_FOUND)
	{
            m_ulLastError = HXR_FAIL;

	    return HXR_FILE_NOT_FOUND;
	}
	/*
	 * Check if we are trying to truncate.  If this is the
	 * case the file is probably mapped and we have to unmap
	 * it.
	 */
	HXBOOL bReopen = FALSE;
	if ((uOpenMode & (HX_FILE_WRITE|HX_FILE_NOTRUNC)) == HX_FILE_WRITE)
	{
	    _AttemptUnmap();
	    bReopen = TRUE;
	}
	
	/*
	 * Check if we tried to do read only.  Perhaps the file
	 * is mapped read/write.  We'll have to open it like that too.
	 */
	if (!(uOpenMode & HX_FILE_WRITE))
	{
	    dwAccess |= GENERIC_WRITE;
	    dwShare |= FILE_SHARE_WRITE;
	    if (g_NT)
	    {
		dwShare |= FILE_SHARE_DELETE;
	    }
	    dwCreate = OPEN_ALWAYS;
	    bReopen = TRUE;
	}
	if (!bReopen)
	{
            m_ulLastError = HXR_FAIL;
	    return HXR_FAIL;
	}
	m_hFile = CreateFile(OS_STRING(m_pFilename), dwAccess, dwShare, NULL,
		dwCreate, dwFlags, NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
            m_ulLastError = HXR_FAIL;
	    return HXR_FAIL;
	}
    }
    m_ulPos = 0;

//  On windows we do now want to use memory mapped IO if we are going across
//  a network share. And we only want to disable this for clients, not servers.

    //check to see if we are within the player...

    IHXClientEngine* pEngine = NULL;
    if (HXR_OK == m_pContext->QueryInterface(IID_IHXClientEngine, (void**)&pEngine))
    {
        HX_RELEASE(pEngine);
        CHXDirSpecifier  dirspec(m_pFilename);

        if (!CHXFileSpecUtils::IsDiskLocal( dirspec) || CHXFileSpecUtils::IsDiskEjectable(dirspec) )
        {
            if (MmapHandle)
            {
	        m_zpMMM->CloseMap(MmapHandle);
	        MmapHandle = 0;
            }
            return HXR_OK;
        }
    }
    
    if (MmapHandle)
    {
	m_zpMMM->CloseMap(MmapHandle);
	MmapHandle = 0;
    }
    MmapHandle = m_zpMMM->OpenMap(m_hFile, m_pContext);
    
    return HXR_OK;
}

/*
 * Original Version
 */
    
/*
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }
    DWORD dwPlatformSpecific = 0;
    if (g_NT)
    {
	dwPlatformSpecific = FILE_SHARE_DELETE;
    }
    DWORD dwFlags = 0;
    DWORD dwShare = dwPlatformSpecific;
    DWORD dwCreate = 0;
    m_uOpenMode = uOpenMode;
    if (uOpenMode & HX_FILE_WRITE)
    {
	dwShare |= FILE_SHARE_WRITE;
	dwFlags |= GENERIC_WRITE;
	if (uOpenMode & HX_FILE_NOTRUNC)
	{
	    dwCreate |= OPEN_ALWAYS;
	}
	else
	{
	    dwCreate |= CREATE_ALWAYS;
	}
    }
    if (uOpenMode & HX_FILE_READ)
    {
	dwShare |= FILE_SHARE_READ;
	dwFlags |= GENERIC_READ;
 	if (!(uOpenMode & HX_FILE_WRITE))
	{
	    dwCreate |= OPEN_EXISTING;
	}
    }
    if (uOpenMode & HX_FILE_BINARY)
    {
    }

    m_hFile = CreateFile(m_pFilename,
	    GENERIC_READ | GENERIC_WRITE,
	    FILE_SHARE_READ | FILE_SHARE_WRITE | dwPlatformSpecific,
	    NULL,
	    dwCreate,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL);


    if (m_hFile == INVALID_HANDLE_VALUE)
    {
	if (::GetLastError() == ERROR_FILE_NOT_FOUND)
	{
	    return HXR_FAIL;
	}

	/*
	 * File may be mapped and we are trying to truncate it.
	 * It will only let us do that if we unmap the file NOW.
	 *
	if (uOpenMode & HX_FILE_WRITE && dwCreate == CREATE_ALWAYS)
	{
	    _AttemptUnmap();

	    /*
	     * We are now going to open the file, so make
	     * sure there is always a read mixed this this write.
	     *
	    dwFlags |= GENERIC_READ;
	    dwShare |= FILE_SHARE_READ;
	}

	/*
	 * Try and re-open the file.  This will either be read only
	 * if we failed reading, in case it is a readonly file,
	 * of it could be another attempt at "w" after unampping
	 * the file.  All vars are set up for either case.
	 *
	m_hFile = CreateFile(m_pFilename,
	    dwFlags,
	    dwShare,
	    NULL,
	    dwCreate,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
	    return HXR_FAIL;
	}
    }
    m_ulPos = 0;

    if (MmapHandle)
    {
	m_zpMMM->CloseMap(MmapHandle);
	MmapHandle = 0;
    }
    MmapHandle = m_zpMMM->OpenMap(m_hFile, m_pContext);

    return HXR_OK;
}
*/

/* Delete File */
STDMETHODIMP
MemoryMapDataFile::Delete()
{
    Close();
    if (!m_pFilename)
    {
	return HXR_UNEXPECTED;
    }
    if (DeleteFile(OS_STRING(m_pFilename)))
    {
	return HXR_OK;
    }
    else if (_AttemptUnmap())
    {
        if (DeleteFile(OS_STRING(m_pFilename)))
	{
	    return HXR_OK;
	}
    }
    return HXR_FAIL;
}


/* Close closes a file 
 * If the reference count on the IHXDataFile object is greater than 1, 
 * then the underlying file cannot be safely closed, so Close() becomes 
 * a noop in that case. Close it only when the object is destroyed. 
 * This would be safe, but could lead to a file descriptor leak.
 */
STDMETHODIMP
MemoryMapDataFile::Close()
{
    m_ulLastError = HXR_OK;

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }
    m_ulPos = -1;

    if (MmapHandle)
    {
        // XXDWW - This shouldn't be NULL.  Just check to make sure
        // that it isn't before use.
        HX_ASSERT( m_zpMMM );
        if ( m_zpMMM )
        {
	    m_zpMMM->CloseMap(MmapHandle);
        }
	MmapHandle = 0;
    }

    return HXR_OK;
}

/* Name returns the currently bound file name in FileName.
 * and returns TRUE, if the a name has been bound.  Otherwise
 * FALSE is returned.
 */
STDMETHODIMP_(HXBOOL)
MemoryMapDataFile::Name(REF(IHXBuffer*) pFileName)
{
    if (!m_pFilename)
    {
	return FALSE;
    }

    if (HXR_OK == CreateAndSetBufferCCF(pFileName, (UCHAR*)m_pFilename,
					strlen(m_pFilename) + 1, m_pContext))
    {
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

/*
 * IsOpen returns TRUE if file is open.  Otherwise FALSE.
 */
STDMETHODIMP_(HXBOOL)
MemoryMapDataFile::IsOpen()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	return TRUE;
    }
    return FALSE;
}

/* Seek moves the current file position to the offset from the
 * fromWhere specifier returns current position of file or -1 on
 * error.
 */
STDMETHODIMP
MemoryMapDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    LONG32 lHighOffset = 0;
    m_ulLastError = HXR_OK;

    DWORD fw = 0;
    switch (fromWhere)
    {
	case SEEK_CUR:
	    fw = FILE_CURRENT;
	    m_ulPos += offset;
	    if (((LONG32) offset) < 0)
	    {
		lHighOffset = -1;
	    }
	    break;

	case SEEK_END:
	    fw = FILE_END;
	    if (offset > m_ulPos)
	    {
		m_ulPos = 0;
	    }
	    else
	    {
		m_ulPos -= offset;
	    }
	    break;

	case SEEK_SET:
	    fw = FILE_BEGIN;
	    m_ulPos = offset;
	    break;
    }

    DWORD dwRet = SetFilePointer(m_hFile, offset, &lHighOffset, fw);
    if (dwRet == -1)
    {
	if (GetLastError() != NO_ERROR)
	{
	    m_ulPos = -1;
	    m_ulLastError = HXR_FAIL;
	    return HXR_FAIL;
	}
    }
    
    return 0;
}

/* Tell returns the current file position in the file */
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Tell()
{
    m_ulLastError = HXR_OK;
    return m_ulPos;
}

void MemoryMapDataFile::StopMmap()
{
    if (MmapHandle == NULL)
	return;
    m_zpMMM->CloseMap(MmapHandle);
    MmapHandle = 0;
    Seek(m_ulPos, SEEK_SET);
}

/* Read reads up to count bytes of data into buf.
 * returns the number of bytes read, EOF, or -1 if the read failed 
 */
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Read(REF(IHXBuffer*) pBuf, 
		ULONG32 count)
{
    DWORD dwError = 0;
    m_ulLastError = HXR_OK;

    if (!(m_uOpenMode & HX_FILE_READ))
    {
	return 0;
    }
    UINT32 ulRead = 0;

    if (MmapHandle)
    {
	ulRead = m_zpMMM->GetBlock(pBuf, MmapHandle, m_ulPos, count);
	if (ulRead >= MMAP_EXCEPTION)
	{
	    if (ulRead != MMAP_EOF_EXCEPTION)
	    {
		StopMmap();
	    }
	    else
	    {
		Seek(m_ulPos, SEEK_SET);
	    }

	    goto normal_read;
	}
	if (ulRead > 0)
	{
	    m_ulPos += ulRead;
	}
	return ulRead;
    }

normal_read:

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
	pBuf = NULL;
	return 0;
    }

    if (HXR_OK == CreateBufferCCF(pBuf, m_pContext))
    {
	pBuf->SetSize(count);
	if (!ReadFile(m_hFile, (void*)pBuf->GetBuffer(), count, &ulRead, NULL))
	{
	    switch (dwError = ::GetLastError()) 
	    { 
		// this can happen if the network share where the file resides
		// is closed, we need to return HXR_ABORT since HXR_FAILED is 
		// interpreted by the File Format as EOF
		case ERROR_NETNAME_DELETED: 
		    { 
			m_ulLastError = HXR_ABORT;
			break;
		    } 
		default:
		    {
			m_ulLastError = HXR_FAILED;
			break;
		    }
	    }
	}

	m_ulPos += ulRead;

	if (ulRead < count)
	{
	    pBuf->SetSize(ulRead);
	}
    }

    return ulRead;
}


/* Write writes up to count bytes of data from buf.
 * returns the number of bytes written, or -1 if the write failed 
 */
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Write(REF(IHXBuffer*) pBuf)
{
    m_ulLastError = HXR_OK;

    if (!(m_uOpenMode & HX_FILE_WRITE))
    {
	return 0;
    }
    StopMmap();

    UINT32 ulWritten = 0;
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
	return 0;
    }
    if (!WriteFile(m_hFile, (void*)pBuf->GetBuffer(), pBuf->GetSize(),
                   &ulWritten, NULL))
    {
        m_ulLastError = HXR_FAIL;
    }

    return ulWritten;
}

/* Flush out the data in case of unbuffered I/O
 */
STDMETHODIMP
MemoryMapDataFile::Flush()
{
    StopMmap();

    return HXR_NOTIMPL;
}

/*
 * Return info about the data file such as permissions, time of creation
 * size in bytes, etc.
 */

STDMETHODIMP
MemoryMapDataFile::Stat(struct stat* buffer)
{
    if (!m_pFilename)
    {
	return HXR_UNEXPECTED;
    }
    HXBOOL bClose = FALSE;
    HANDLE hUse = m_hFile;
    if (hUse == INVALID_HANDLE_VALUE)
    {
	bClose = TRUE;
	hUse = CreateFile(OS_STRING(m_pFilename),
		0, //query
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
    }

    if (hUse == INVALID_HANDLE_VALUE)
    {
        // if m_pFilename points to a directory, the above call will fail, per msdn.
        // we need to call CreateFile with a FILE_FLAG_BACKUP_SEMANTICS for it to succeed.
        bClose = TRUE;
        hUse = CreateFile(OS_STRING(m_pFilename),
                0, //query
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL);
    }

    if (hUse == INVALID_HANDLE_VALUE)
    {
	return HXR_FAIL;
    }

    BY_HANDLE_FILE_INFORMATION mss;
    memset(buffer, 0, sizeof(*buffer));

    buffer->st_nlink = 1;
    
    HX_RESULT res;
    if (GetFileInformationByHandle(hUse,
	    &mss))
    {
	buffer->st_atime = FileTimeTotime_t(&mss.ftLastAccessTime);
	buffer->st_ctime = FileTimeTotime_t(&mss.ftCreationTime);
	buffer->st_mtime = FileTimeTotime_t(&mss.ftLastWriteTime);
	buffer->st_size  = mss.nFileSizeLow;
	if ( mss.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
	    //The file is a directory.
	    buffer->st_mode |= HX_S_IFDIR;
	}
	res = HXR_OK;
    }
    else
    {
	res = HXR_FAIL;
    }

    if (bClose)
    {
	CloseHandle(hUse);
    }

    return res;

}

/* Return the file descriptor */
STDMETHODIMP_(INT16)
MemoryMapDataFile::GetFd()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	return 1;
    }
    return -1;
}

/* GetLastError returns the platform specific file error */
STDMETHODIMP
MemoryMapDataFile::GetLastError()
{
    return m_ulLastError;
}

/* GetLastError returns the platform specific file error in
 * string form.
 */
STDMETHODIMP_(void)
MemoryMapDataFile::GetLastError(REF(IHXBuffer*) err)
{
    err = 0;
    return;
}

HXBOOL
MemoryMapDataFile::_AttemptUnmap()
{
    HANDLE hFile;
    hFile = CreateFile(OS_STRING(m_pFilename),
	    GENERIC_READ,
	    FILE_SHARE_READ |
		(g_NT ? FILE_SHARE_DELETE : 0),
	    NULL,
	    OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
	return FALSE;
    }
    void* pHandle = m_zpMMM->GetMMHandle(hFile);
    CloseHandle(hFile);
    if (!pHandle)
    {
	return FALSE;
    }
    m_zpMMM->AttemptCloseMapNow(pHandle);
    return TRUE;
}

