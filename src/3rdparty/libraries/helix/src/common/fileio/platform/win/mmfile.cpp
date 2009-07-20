/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmfile.cpp,v 1.7 2007/07/06 20:35:17 jfinnecy Exp $
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

#include "chxdataf.h"
#include "platform/win/mmfile.h"
#include "hlxclib/windows.h"
#include "pckunpck.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

BEGIN_INTERFACE_LIST(_CBufferWinMemMapped)
    INTERFACE_LIST_ENTRY(IID_IHXBuffer,  IHXBuffer)
END_INTERFACE_LIST

UINT32 
_CBufferWinMemMapped::_AllocGran()
{
    static UINT32 m_ulAllocGran = 0;

    if (!m_ulAllocGran)
    {
	SYSTEM_INFO sysinfCurrent;
	GetSystemInfo(&sysinfCurrent);
	m_ulAllocGran = sysinfCurrent.dwAllocationGranularity;
    }
    
    return m_ulAllocGran;
}

void 
_CBufferWinMemMapped::_UnMap()
{
    if (m_pData)
    {
	UnmapViewOfFile(m_pData);
	m_ulLength = 0;
    }
}

HX_RESULT 
_CBufferWinMemMapped::_SetMapping
(
    HANDLE hfmoFile, 
    UINT32 ulOffsetHigh, 
    UINT32 ulOffsetLow , 
    UINT32 ulLength
)
{
    _UnMap();
    
    if(!ulLength || !hfmoFile)
    {
	return HXR_INVALID_PARAMETER;
    }

    m_ulOffset = ulOffsetLow%_AllocGran();
    if (m_ulOffset < ulOffsetLow)
    {
	ulOffsetLow -= m_ulOffset;
    }
    else
    {
	m_ulOffset = ulOffsetLow;
	ulOffsetLow = 0;
    }

    m_pData = (UCHAR*)MapViewOfFile
    (
	hfmoFile, 
	FILE_MAP_READ,
	ulOffsetHigh, 
	ulOffsetLow, 
	ulLength+m_ulOffset
    );

    if(m_pData)
    {
	m_ulLength = ulLength;
    }
    else
    {
	return HRESULT_FROM_WIN32(::GetLastError());
    }
    
    return HXR_OK;
}

/*  Create creates a file using the specified mode */
HX_RESULT	CWin32MMFile::Create
(
    const char *filename, 
    UINT16 mode, 
    HXBOOL textflag
)
{
    return Open(filename, mode, textflag);
}

/*  Open will open a file with the specified permissions */
HX_RESULT	CWin32MMFile::Open
(
    const char *filename, 
    UINT16 mode, 
    HXBOOL textflag
)
{
    // _O_APPEND	Repositions the file pointer to the end of the file 
    //		before every write operation.
    // _O_CREAT	Creates and opens a new file for writing; this has 
    //		no effect if the file specified by filename exists.
    // _O_EXCL	Returns an error value if the file specified by 
    //		filename exists. Only applies when used with 
    //		_O_CREAT.
    // _O_RDONLY	Opens file for reading only; if this flag is given, 
    //		neither _O_RDWR nor _O_WRONLY can be given.
    // _O_RDWR	Opens file for both reading and writing; if this 
    //		flag is given, neither _O_RDONLY nor _O_WRONLY can 
    //		be given.
    // _O_TRUNC	Opens and truncates an existing file to zero 
    //		length; the file must have write permission. 
    //		The contents of the file are destroyed. If this 
    //		flag is given, you cannot specify _O_RDONLY.
    // _O_WRONLY	Opens file for writing only; if this flag is given, 
    //		neither _O_RDONLY nor _O_RDWR can be given.

    if(mode&_O_APPEND)// || textflag)
    {
	return HXR_INVALID_PARAMETER;
    }

    Close();

    //CreateFile Parameters
    //
    LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;
    DWORD dwCreationDistribution = 0;
    DWORD dwFlagsAndAttributes = 0;
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    //CreateFileMapping Parameters
    //
    DWORD flProtect = SEC_COMMIT | SEC_NOCACHE;
    DWORD dwMaximumSizeHigh=0;
    DWORD dwMaximumSizeLow=0;

    if(mode&_O_RDWR || mode&_O_WRONLY)
    {
	dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	flProtect |= PAGE_READWRITE;
    }
    else
    {
	dwDesiredAccess = GENERIC_READ;
	flProtect |= PAGE_READONLY;
    }
    
    if(mode&_O_EXCL && mode&_O_CREAT)
    {
	dwCreationDistribution |= CREATE_NEW;
    }
    else if(mode&_O_CREAT)
    {
	dwCreationDistribution |= OPEN_ALWAYS;
    }
    else if(mode&_O_TRUNC)
    {
	dwCreationDistribution |= TRUNCATE_EXISTING;
    }
    else
    {
	dwCreationDistribution |= OPEN_EXISTING;
    }

    m_hFile = CreateFile
    (
	OS_STRING(filename), 
	dwDesiredAccess, 
	dwShareMode, 
	lpSecurityAttributes, 
	dwCreationDistribution, 
	dwFlagsAndAttributes, 
	NULL
    );
    
    if(m_hFile == INVALID_HANDLE_VALUE)
    {
	return HRESULT_FROM_WIN32(::GetLastError());
    }

    m_ulSizeLow = GetFileSize(m_hFile, &m_ulSizeHigh);
    
    m_hfmoFile = CreateFileMapping
    (
	m_hFile, 
	NULL, 
	flProtect, 
	dwMaximumSizeHigh, 
	dwMaximumSizeLow, 
	NULL
    );

    if(!m_hfmoFile)
    {
	return HRESULT_FROM_WIN32(::GetLastError());
    }

    return HXR_OK;
}

/*  Close closes a file */
HX_RESULT	CWin32MMFile::Close		(void)
{
    if(m_hfmoFile)
    {
	CloseHandle(m_hfmoFile);
	m_hfmoFile = NULL;
    }

    if(m_hFile != INVALID_HANDLE_VALUE)
    {
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
    }

    m_ulPositionHigh = 0;
    m_ulPositionLow = 0;
    m_ulSizeHigh = 0;
    m_ulSizeLow = 0;

    HX_RELEASE(m_pContext);
    return HXR_OK;
}

/*  Seek moves the current file position to the offset from the fromWhere 
specifier */
HX_RESULT	CWin32MMFile::Seek
(
    ULONG32 offset, 
    UINT16 fromWhere
)
{
    if(!m_hfmoFile)
    {
	return HXR_FAIL;
    }

    switch(fromWhere)
    {
    case SEEK_SET:
	{
	    m_ulPositionHigh = 0;
	    m_ulPositionLow = offset;
	}
	break;
    case SEEK_END:
	{
	    // Set to End..
	    m_ulPositionHigh = m_ulSizeHigh;
	    m_ulPositionLow = m_ulSizeLow;
	}
	// Then do the offset..
    case SEEK_CUR:
	{
	    m_ulPositionLow += offset;

	    // Handle overflow
	    //
	    if (m_ulPositionLow < offset && offset>1)
		++m_ulPositionHigh;
	    else if (m_ulPositionLow > offset && offset<1)
		--m_ulPositionHigh;
	}
	break;
    };
    
    return HXR_OK;
}

/*  Tell returns the current file position in the ra file */
ULONG32	CWin32MMFile::Tell		(void)
{
    if (!m_hfmoFile || m_ulPositionHigh)
    {
	return UINT32(-1);
    }

    return m_ulPositionLow;
}

/*  Read reads up to count bytes of data into buf.
returns the number of bytes read, EOF, or -1 if the read failed */
ULONG32	CWin32MMFile::Read		
(
    char* buf, 
    ULONG32 count
)
{
    return (ULONG)-1;
}

/*  Read reads up to ulCount bytes of data into ppbufOut.
returns a result code */
HX_RESULT	CWin32MMFile::ReadToBuffer	
(
    ULONG32 ulCount, 
    IHXBuffer** ppbufOut
)
{
    if(!m_hfmoFile)
    {
	return HXR_FAIL;
    }
    
    UINT32 ulActualCount = ulCount;

    if (m_ulPositionHigh >= m_ulSizeHigh)
    {
	if(m_ulPositionLow >= m_ulSizeLow)
	{
	    return HXR_FAIL;
	}

	if(m_ulPositionLow+ulCount > m_ulSizeLow)
	{
	    ulActualCount = m_ulSizeLow - m_ulPositionLow;
	}
    }

    HX_RESULT pnrRes;

    IHXFragmentedBuffer* pfbufOut;
    IHXBuffer* pbufOut;
    _CBufferWinMemMapped* pmmbufOut;

    pmmbufOut = _CBufferWinMemMapped::CreateObject();
    pmmbufOut->AddRef();

    pnrRes = pmmbufOut->_SetMapping
    (
	m_hfmoFile, 
	m_ulPositionHigh, 
	m_ulPositionLow, 
	ulCount
    );

    if (SUCCEEDED(pnrRes))
    {
	pmmbufOut->QueryInterface(IID_IHXBuffer, (void**)&pbufOut);
	
	CreateFragmentedBufferCCF(pfbufOut, m_pContext);
	pfbufOut->Append(pbufOut, 0, ulCount);

	pfbufOut->QueryInterface(IID_IHXBuffer, (void**)ppbufOut);

	Seek(ulCount, SEEK_CUR);

	HX_RELEASE(pfbufOut);
	HX_RELEASE(pbufOut);
    }

    HX_RELEASE(pmmbufOut);

    return pnrRes;
}

/*  Write writes up to count bytes of data from buf.
returns the number of bytes written, or -1 if the write failed */
ULONG32	CWin32MMFile::Write
(
    const char *buf, 
    ULONG32 count
)
{
    return (ULONG)-1;
}

/*  Rewinds the file to the start of the file */
HX_RESULT	CWin32MMFile::Rewind		(void)
{
    return Seek(0, SEEK_SET);
}

/*  Return the file descriptor                      */
INT16	CWin32MMFile::GetFd		(void)
{
    return 0;
}

/*  Delete deletes a file */
HX_RESULT	CWin32MMFile::Delete
(
    const char *filename
)
{
    return E_NOTIMPL;
}
