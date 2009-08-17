/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unbufdataf.cpp,v 1.7 2006/02/07 19:21:13 ping Exp $
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

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxbuffer.h"
#include "debug.h"

#include "hxdataf.h"
#include "datffact.h"
#include "unbufdataf.h"

/////////////////////////////////////////////////////////////////////////
//  Method:
//      UnBufferedDataFile::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
UnBufferedDataFile::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXDataFile))
    {
        AddRef();
        *ppvObj = (IHXDataFile*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      UnBufferedDataFile::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
UnBufferedDataFile::AddRef()
{
    DPRINTF(0x5d000000, ("UBDF::AddRef() = %ld\n", m_lRefCount+1));
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      UnBufferedDataFile::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
UnBufferedDataFile::Release()
{
    DPRINTF(0x5d000000, ("UBDF::Release() = %ld\n", m_lRefCount-1));
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

UnBufferedDataFile::UnBufferedDataFile(IUnknown* pContext)
		  : m_lRefCount(0)
		  , m_ulLastError(HXR_OK)
		  , m_pFilename(NULL)
		  , m_nFD(-1)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    CreateBufferCCF(m_pFilename, m_pContext);
    DPRINTF(0x5d000000, ("UnBufferedDataFile::UnBufferedDataFile()\n"));
}

// ~CHXFile should close the file if it is open
UnBufferedDataFile::~UnBufferedDataFile()
{ 
    // close the file if it is open
    if (m_nFD > 0)
    {
       close(m_nFD);
       m_nFD = -1;
    }
    HX_RELEASE(m_pFilename);

    DPRINTF(0x5d000000, ("UnBufferedDataFile::~UnBufferedDataFile()\n"));
}

/*
 *  IHXDataFile methods
 */

/* Bind DataFile Object with FileName */
STDMETHODIMP_(void)
UnBufferedDataFile::Bind(const char* pFilename)
{
    m_pFilename->Set((BYTE *)pFilename, strlen(pFilename)+1);

    DPRINTF(0x5d000000, ("UnBufferedDataFile::Bind(%s)\n", 
	    (const char *)m_pFilename->GetBuffer()));
}


/* Creates a datafile using the specified mode
 * uOpenMode --- File Open mode - HX_FILEFLAG_READ/HX_FILEFLAG_WRITE/HX_FILEFLAG_BINARY
 */
STDMETHODIMP
UnBufferedDataFile::Create(UINT16 uOpenMode)
{
    return HXR_NOTIMPL;
}


/* Open will open a file with the specified mode
 */
STDMETHODIMP
UnBufferedDataFile::Open(UINT16 uOpenMode)
{
    DPRINTF(0x5d000000, ("UnBufferedDataFile::Open()\n"));
    int mode = 0;

    if (uOpenMode & HX_FILEFLAG_WRITE)
    {
        mode |= (O_CREAT | O_RDWR);
	if (!(uOpenMode & HX_FILEFLAG_NOTRUNC))
	{
	    mode |= O_TRUNC;
	}
    }
    else if (uOpenMode & HX_FILEFLAG_READ)
    {
	mode |= O_RDONLY;
    }

    // close previous file if necessary
    if (m_nFD > 0)
	close(m_nFD);

    // open file
    m_ulLastError = HXR_OK;
#define FILEPERM (S_IREAD | S_IWRITE)
    if ((m_nFD = open((const char *)m_pFilename->GetBuffer(), 
	mode, FILEPERM)) < 0)
    {
	m_ulLastError = errno;
	return HXR_DOC_MISSING;
    }
    // change permissions to allow everyone to read the file and owner to write
    // only if I have to create this file
#if !defined(_VXWORKS) && !defined(_BEOS)
    if (mode & O_CREAT)
	fchmod(m_nFD, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif

    return HXR_OK;
}


/* Close closes a file 
 * If the reference count on the IHXDataFile object is greater than 1, 
 * then the underlying file cannot be safely closed, so Close() becomes 
 * a noop in that case. Close it only when the object is destroyed. 
 * This would be safe, but could lead to a file descriptor leak.
 */
STDMETHODIMP
UnBufferedDataFile::Close()
{
    if (m_nFD > 0)
    {
	close(m_nFD);
	m_nFD = -1;
    }
    return HXR_OK;
}


/* Name returns the currently bound file name in FileName.
 * and returns TRUE, if the a name has been bound.  Otherwise
 * FALSE is returned.
 */
STDMETHODIMP_(HXBOOL)
UnBufferedDataFile::Name(REF(IHXBuffer*) pFileName)
{
    return HXR_NOTIMPL;
}


/*
 * IsOpen returns TRUE if file is open.  Otherwise FALSE.
 */
inline HXBOOL
UnBufferedDataFile::IsOpen()
{
    return (m_nFD > 0 ? TRUE : FALSE);
}


/* Seek moves the current file position to the offset from the
 * fromWhere specifier returns current position of file or -1 on
 * error.
 */
STDMETHODIMP
UnBufferedDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    if (m_nFD > 0)
    {
	m_ulLastError = HXR_OK;       
	if (lseek(m_nFD, offset, fromWhere) < 0)
	{
	    m_ulLastError = errno;
	    return HXR_INVALID_FILE;
	}
	return HXR_OK;
    }
    return HXR_INVALID_FILE;
}


/* Tell returns the current file position in the file */
STDMETHODIMP_(ULONG32)
UnBufferedDataFile::Tell()
{
    INT32 offset = -1;
    if (m_nFD > 0)
    {
	m_ulLastError = HXR_OK;       
	// so we do this instead....
	if ((offset = lseek(m_nFD, 0, SEEK_CUR)) < 0)
	{
	    m_ulLastError = errno;
	}
    }
    return (ULONG32)offset;
}


/* Read reads up to count bytes of data into buf.
 * returns the number of bytes read, EOF, or -1 if the read failed 
 */
STDMETHODIMP_(ULONG32)
UnBufferedDataFile::Read(REF(IHXBuffer *) pBuf, ULONG32 count)
{
    HX_ASSERT(pBuf);

    pBuf->AddRef();
    int ncnt = 0;           // number of bytes read

    if (m_nFD > 0)
    { 
	m_ulLastError = HXR_OK;
	ULONG32 tmpCheck = Tell();

	if ((ncnt = read(m_nFD, (void *)pBuf->GetBuffer(), count)) < 0)
	{
	    m_ulLastError = errno;
	}
    }
    pBuf->Release();

    return (ULONG32)ncnt;
}


/* Write writes up to count bytes of data from buf.
 * returns the number of bytes written, or -1 if the write failed 
 */
STDMETHODIMP_(ULONG32)
UnBufferedDataFile::Write(REF(IHXBuffer *) pBuf)
{
    HX_ASSERT(pBuf);

    pBuf->AddRef();
    int ncnt = -1;           // number of bytes written
    int count = pBuf->GetSize();

    if (m_nFD > 0)
    {
	m_ulLastError = HXR_OK;

	if ((ncnt = write(m_nFD, (const char *)pBuf->GetBuffer(), count)) < 0)
	{
	    m_ulLastError = errno;
	}
    }
    pBuf->Release();

    return (ULONG32)ncnt;
}


/* Flush out the data in case of unbuffered I/O
 */
STDMETHODIMP
UnBufferedDataFile::Flush()
{
    return HXR_OK;
}


/*
 * Return info about the data file such as permissions, time of creation
 * size in bytes, etc.
 */
STDMETHODIMP
UnBufferedDataFile::Stat(struct stat* statbuf)
{
    if (m_nFD > 0)
    {
	if (!fstat(m_nFD, statbuf))
	    return HXR_OK;
    }
    else if (m_pFilename->GetSize())
    {
	if (!stat((const char *)m_pFilename->GetBuffer(), statbuf))
	    return HXR_OK;
    }
    return HXR_FAIL;
}


/* Return the file descriptor */
inline INT16
UnBufferedDataFile::GetFd()
{
    return m_nFD;
}


/* GetLastError returns the platform specific file error */
STDMETHODIMP
UnBufferedDataFile::GetLastError()
{
    return HXR_NOTIMPL;
}


/* GetLastError returns the platform specific file error in
 * string form.
 */
STDMETHODIMP_(void)
UnBufferedDataFile::GetLastError(REF(IHXBuffer*) err)
{
}

