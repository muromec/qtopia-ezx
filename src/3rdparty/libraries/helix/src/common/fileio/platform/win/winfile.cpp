/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: winfile.cpp,v 1.9 2007/07/06 20:35:17 jfinnecy Exp $
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
#include "hxbuffer.h"
#include "winfile.h"
#include "hxfiles.h"
#include "pckunpck.h"

#include "hlxclib/io.h"
#include "hlxclib/fcntl.h"

WinFile::WinFile(IUnknown* pContext)
: m_lRefCount(0)
, m_hFile(INVALID_HANDLE_VALUE)
, m_pFilename(0)
, m_ulPos(-1)
, m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
}

WinFile::~WinFile()
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
WinFile::QueryInterface(REFIID riid,
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
WinFile::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32)
WinFile::Release()
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
WinFile::Bind(const char* FileName)
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
WinFile::Create(UINT16 uOpenMode)
{
    return HXR_NOTIMPL;
}

/* Open will open a file with the specified mode
 */
STDMETHODIMP
WinFile::Open(UINT16 uOpenMode)
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }
    DWORD dwFlags = 0;
    DWORD dwShare = 0;
    DWORD dwCreate = 0;
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

    m_hFile = CreateFile(OS_STRING(m_pFilename),
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
    m_ulPos = 0;
    return HXR_OK;
	
}

/* Close closes a file 
 * If the reference count on the IHXDataFile object is greater than 1, 
 * then the underlying file cannot be safely closed, so Close() becomes 
 * a noop in that case. Close it only when the object is destroyed. 
 * This would be safe, but could lead to a file descriptor leak.
 */
STDMETHODIMP
WinFile::Close()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }
    m_ulPos = -1;
    return HXR_OK;
}

/* Name returns the currently bound file name in FileName.
 * and returns TRUE, if the a name has been bound.  Otherwise
 * FALSE is returned.
 */
STDMETHODIMP_(HXBOOL)
WinFile::Name(REF(IHXBuffer*) pFileName)
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
WinFile::IsOpen()
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
WinFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    DWORD fw = 0;
    switch (fromWhere)
    {
	case SEEK_CUR:
	    fw = FILE_CURRENT;
	    m_ulPos += offset;
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

    DWORD dwRet;
    dwRet = SetFilePointer(m_hFile,
	offset,
	NULL,
	fw);
    
    if (dwRet == -1)
    {
	m_ulPos = -1;
	return HXR_FAIL;
    }
    
    return 0;
}

/* Tell returns the current file position in the file */
STDMETHODIMP_(ULONG32)
WinFile::Tell()
{
    return m_ulPos;
}

/* Read reads up to count bytes of data into buf.
 * returns the number of bytes read, EOF, or -1 if the read failed 
 */
STDMETHODIMP_(ULONG32)
WinFile::Read(REF(IHXBuffer*) pBuf, 
		ULONG32 count)
{
    HXBOOL bRet = FALSE;
    UINT32 ulRead = 0;

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
	pBuf = NULL;
	return 0;
    }

    if (HXR_OK == CreateBufferCCF(pBuf, m_pContext))
    {
	pBuf->SetSize(count);
	bRet = ReadFile(m_hFile,
	    (void*)pBuf->GetBuffer(),
	    count,
	    &ulRead,
	    NULL);

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
WinFile::Write(REF(IHXBuffer*) pBuf)
{
    UINT32 ulWritten = 0;
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
	return 0;
    }
    HXBOOL bRet;
    bRet = WriteFile(m_hFile,
	(void*)pBuf->GetBuffer(),
	pBuf->GetSize(),
	&ulWritten,
	NULL);

    return ulWritten;
}

/* Flush out the data in case of unbuffered I/O
 */
STDMETHODIMP
WinFile::Flush()
{
    return HXR_NOTIMPL;
}

/*
 * Return info about the data file such as permissions, time of creation
 * size in bytes, etc.
 */

STDMETHODIMP
WinFile::Stat(struct stat* buffer)
{
    if (!m_pFilename)
    {
	return HXR_UNEXPECTED;
    }

    int ret = _stat(m_pFilename, (struct _stat*)buffer);
    if (ret == 0)
    {
	return HXR_OK;
    }
    return HXR_FAIL;
    /*
     * XXXPM
     * This way is faster, but the times are incompatible with stat times.
     * Maybe someday I will convert.
     *
    HXBOOL bClose = 0;
    HANDLE hUse = m_hFile;
    if (hUse == INVALID_HANDLE_VALUE)
    {
	bClose = 1;
	hUse = CreateFile(m_pFilename,
		0, //query
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
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
	buffer->st_atime = mss.ftLastAccessTime.dwLowDateTime;
	buffer->st_ctime = mss.ftCreationTime.dwLowDateTime;
	buffer->st_mtime = mss.ftLastWriteTime.dwLowDateTime;
	buffer->st_size = mss.nFileSizeLow;
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
    */

}

/* Return the file descriptor */
STDMETHODIMP_(INT16)
WinFile::GetFd()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
	return 1;
    }
    return -1;
}

/* GetLastError returns the platform specific file error */
STDMETHODIMP
WinFile::GetLastError()
{
    return HXR_NOTIMPL;
}

/* GetLastError returns the platform specific file error in
 * string form.
 */
STDMETHODIMP_(void)
WinFile::GetLastError(REF(IHXBuffer*) err)
{
    err = 0;
    return;
}
