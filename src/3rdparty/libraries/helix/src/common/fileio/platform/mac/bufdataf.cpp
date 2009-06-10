/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bufdataf.cpp,v 1.9 2007/07/06 20:35:13 jfinnecy Exp $
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

#include <stdio.h>
#include <stat.h>
#include <types.h>
#include <fcntl.h>
#include <errno.h>

#include <unix.mac.h>  // Anyone using this file MUST #undef _UNIX
#undef _UNIX


#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "hxbuffer.h"
#include "debug.h"

#include "hxdataf.h"
#include "datffact.h"
#include "bufdataf.h"
#include "pckunpck.h"

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BufferedDataFile::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
BufferedDataFile::QueryInterface(REFIID riid, void** ppvObj)
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
//      BufferedDataFile::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
BufferedDataFile::AddRef()
{
    DPRINTF(0x5d000000, ("UBDF::AddRef() = %ld\n", m_lRefCount+1));
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BufferedDataFile::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
BufferedDataFile::Release()
{
    DPRINTF(0x5d000000, ("UBDF::Release() = %ld\n", m_lRefCount-1));
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

BufferedDataFile::BufferedDataFile(IUnknown* pContext)
		  : m_lRefCount(0)
		  , m_ulLastError(HXR_OK)
		  , m_pFilename(NULL)
		  , m_pFile(0)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    CreateBufferCCF(m_pFilename, m_pContext);
    DPRINTF(0x5d000000, ("BufferedDataFile::BufferedDataFile()\n"));
}

// ~CHXFile should close the file if it is open
BufferedDataFile::~BufferedDataFile()
{ 
    // close the file if it is open
    if (m_pFile)
    {
       fclose(m_pFile);
       m_pFile = 0;
    }
    HX_RELEASE(m_pFilename);

    DPRINTF(0x5d000000, ("BufferedDataFile::~BufferedDataFile()\n"));
}

/*
 *  IHXDataFile methods
 */

/* Bind DataFile Object with FileName */
STDMETHODIMP_(void)
BufferedDataFile::Bind(const char* pFilename)
{
    m_pFilename->Set((BYTE *)pFilename, strlen(pFilename)+1);

    DPRINTF(0x5d000000, ("BufferedDataFile::Bind(%s)\n", 
	    (const char *)m_pFilename->GetBuffer()));
}


/* Creates a datafile using the specified mode
 * uOpenMode --- File Open mode - HX_FILEFLAG_READ/HX_FILEFLAG_WRITE/HX_FILEFLAG_BINARY
 */
STDMETHODIMP
BufferedDataFile::Create(UINT16 uOpenMode)
{
    return HXR_NOTIMPL;
}


/* Open will open a file with the specified mode
 */
STDMETHODIMP
BufferedDataFile::Open(UINT16 uOpenMode)
{
    DPRINTF(0x5d000000, ("BufferedDataFile::Open()\n"));

    char modeflags[4]; /* Flawfinder: ignore */
 
    if(uOpenMode & HX_FILEFLAG_READ)
    {
        modeflags[0] = 'r';
        modeflags[1] = 0;
        if(uOpenMode & HX_FILEFLAG_WRITE)
        {
            modeflags[1] = '+';
            modeflags[2] = 0;
        }
        if(uOpenMode & HX_FILEFLAG_BINARY)
            strcat(modeflags, "b"); /* Flawfinder: ignore */
    }
    else if(uOpenMode & HX_FILEFLAG_WRITE)
    {
        modeflags[0] = 'w';
        modeflags[1] = 0;
        if(uOpenMode & HX_FILEFLAG_BINARY)
            strcat(modeflags, "b"); /* Flawfinder: ignore */
    }
    else if(!uOpenMode)
    {
        uOpenMode = HX_FILEFLAG_READ | HX_FILEFLAG_BINARY;
        modeflags[0] = 'r';
        modeflags[1] = 'b';
        modeflags[2] = 0;
    }
    else
    {
        return HXR_INVALID_PARAMETER;
    }

    // close previous file if necessary
    if (m_pFile)
	fclose(m_pFile);

    DPRINTF(0x5d000000, ("BDF::Open() -- %s\n", modeflags));

    // open file
    m_ulLastError = HXR_OK;
    if ((m_pFile = fopen((const char *)m_pFilename->GetBuffer(), modeflags)) == NULL)
    {
	m_ulLastError = errno;
	return HXR_DOC_MISSING;
    }

    return HXR_OK;
}


/* Close closes a file 
 * If the reference count on the IHXDataFile object is greater than 1, 
 * then the underlying file cannot be safely closed, so Close() becomes 
 * a noop in that case. Close it only when the object is destroyed. 
 * This would be safe, but could lead to a file descriptor leak.
 */
STDMETHODIMP
BufferedDataFile::Close()
{
    if (m_pFile)
    {
	fclose(m_pFile);
	m_pFile = 0;
    }
    return HXR_OK;
}



/* Name returns the currently bound file name in FileName.
 * and returns TRUE, if the a name has been bound.  Otherwise
 * FALSE is returned.
 */
STDMETHODIMP_(HXBOOL)
BufferedDataFile::Name(REF(IHXBuffer*) pFileName)
{
    return HXR_NOTIMPL;
}


/*
 * IsOpen returns TRUE if file is open.  Otherwise FALSE.
 */
inline HXBOOL
BufferedDataFile::IsOpen()
{
    return (m_pFile ? TRUE : FALSE);
}


/* Seek moves the current file position to the offset from the
 * fromWhere specifier returns current position of file or -1 on
 * error.
 */
STDMETHODIMP
BufferedDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    if (m_pFile)
    {
	m_ulLastError = HXR_OK;       
	if (fseek(m_pFile, offset, fromWhere) < 0)
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
BufferedDataFile::Tell()
{
    INT32 offset = -1;
    if (m_pFile)
    {
	m_ulLastError = HXR_OK;       
	// so we do this instead....
	if ((offset = fseek(m_pFile, 0, SEEK_CUR)) < 0)
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
BufferedDataFile::Read(REF(IHXBuffer *) pBuf, ULONG32 count)
{
    HX_ASSERT(pBuf);

    pBuf->AddRef();
    int ncnt = 0;           // number of bytes read

    if (m_pFile)
    { 
	m_ulLastError = HXR_OK;
	ULONG32 tmpCheck = Tell();

	if ((ncnt = fread((void *)pBuf->GetBuffer(), sizeof(char), 
	    count, m_pFile)) < count)
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
BufferedDataFile::Write(REF(IHXBuffer *) pBuf)
{
    HX_ASSERT(pBuf);

    pBuf->AddRef();
    int ncnt = -1;           // number of bytes written
    int count = pBuf->GetSize();

    if (m_pFile)
    {
	m_ulLastError = HXR_OK;

	if ((ncnt = fwrite((const char *)pBuf->GetBuffer(), sizeof(char),
	    count, m_pFile)) < count)
	{
	    m_ulLastError = errno;
	}
    }
    pBuf->Release();

    return (ULONG32)ncnt;
}



/* Flush out the data in case of Buffered I/O
 */
STDMETHODIMP
BufferedDataFile::Flush()
{
    if (m_pFile)
    {
	m_ulLastError = HXR_OK;       
	if (!fflush(m_pFile))
	{
	    m_ulLastError = errno;
	    return HXR_INVALID_FILE;
	}
	return HXR_OK;
    }
    return HXR_INVALID_FILE;
}


/*
 * Return info about the data file such as permissions, time of creation
 * size in bytes, etc.
 */
STDMETHODIMP
BufferedDataFile::Stat(struct stat* statbuf)
{
    if (m_pFile)
    {
	if (!fstat(fileno(m_pFile), statbuf))
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
BufferedDataFile::GetFd()
{
    return m_pFile ? fileno(m_pFile) : -1;
}


/* GetLastError returns the platform specific file error */
STDMETHODIMP
BufferedDataFile::GetLastError()
{
    return HXR_NOTIMPL;
}


/* GetLastError returns the platform specific file error in
 * string form.
 */
STDMETHODIMP_(void)
BufferedDataFile::GetLastError(REF(IHXBuffer*) err)
{
}
