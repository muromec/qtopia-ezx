/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmapdatf.cpp,v 1.15 2006/05/01 14:31:47 bobclark Exp $
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
#include "mmapdatf.h"
#include "mmapmgr.h"
#include "filespec.h"

#if defined(_MAC_UNIX)
#include "filespecutils.h"
#endif

/////////////////////////////////////////////////////////////////////////
//  Method:
//      MemoryMapDataFile::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
MemoryMapDataFile::QueryInterface(REFIID riid, void** ppvObj)
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
//      MemoryMapDataFile::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::AddRef()
{
    DPRINTF(0x5d000000, ("UBDF::AddRef() = %ld\n", m_lRefCount+1));
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      MemoryMapDataFile::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Release()
{
    DPRINTF(0x5d000000, ("UBDF::Release() = %ld\n", m_lRefCount-1));
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   


MemoryMapDataFile::MemoryMapDataFile(IUnknown* pContext,
    REF(IUnknown*) pPersistantObject, HXBOOL bDisableMemoryMappedIO,
    UINT32 ulChunkSize, HXBOOL bEnableFileLocking)
: m_lRefCount(0)
, m_ulLastError(HXR_OK)
, m_pFilename(NULL)
, m_nFD(-1)
, m_ulPos(0)
, m_ulFilePointerPos(0)
, MmapHandle(0)
, m_pMMM((MemoryMapManager*)pPersistantObject)
, m_ulChunkSize(ulChunkSize)
, m_bEnableFileLocking(bEnableFileLocking)
, m_bLockedIt(FALSE)
{
    m_pContext = pContext;
    m_pContext->AddRef();
    
    CreateBufferCCF(m_pFilename, m_pContext);
    DPRINTF(0x5d000000, ("MemoryMapDataFile::MemoryMapDataFile()\n"));
    if (m_pMMM == NULL)
    {
	/* Create one Memory Map Manager per process */
	m_pMMM = new MemoryMapManager(pContext, bDisableMemoryMappedIO,
		m_ulChunkSize);
	pPersistantObject = m_pMMM;
	pPersistantObject->AddRef();
    }
    m_pMMM->AddRef();
}

// ~CHXFile should close the file if it is open
MemoryMapDataFile::~MemoryMapDataFile()
{ 
    // close the file if it is open
    if (m_nFD > 0)
    {
       if (m_bEnableFileLocking)
       {
	   UnlockFile();
       }
       close(m_nFD);
       m_nFD = -1;
    }
    HX_RELEASE(m_pFilename);
    HX_RELEASE(m_pMMM);
    HX_RELEASE(m_pContext);

    DPRINTF(0x5d000000, ("MemoryMapDataFile::~MemoryMapDataFile()\n"));
}

/*
 *  IHXDataFile methods
 */

/* Bind DataFile Object with FileName */
STDMETHODIMP_(void)
MemoryMapDataFile::Bind(const char* pFilename)
{
#if defined(_MAC_UNIX)
    // on OS X, memory mapping a file from an audio CD
    // causes a kernel panic. This has been reported
    // to Apple as radr://3730990 but we still need
    // to avoid this situation.
    
    CHXFileSpecifier fileSpec(pFilename);
    CHXDirSpecifier dirSpec = fileSpec.GetParentDirectory();
    
    bool bMemMappingOK = true;
    
    if (CHXFileSpecUtils::IsDiskAudioCD(dirSpec))
    {
        bMemMappingOK = false;
    }
    
    // if a windows share (smb) is shut down remotely then if
    // we're memory-mapping we crash. This happened with the
    // windows implementation of MemoryMapDataFile ages ago,
    // and I'm pulling over their solution which is to ensure
    // that memory mapping does not happen unless the disk is
    // local AND non-ejectable.
    if (!CHXFileSpecUtils::IsDiskLocal(dirSpec) || CHXFileSpecUtils::IsDiskEjectable(dirSpec))
    {
        bMemMappingOK = false;
    }
    
    // if a windows share (smb) is shut down remotely then if
    // we're memory-mapping we crash. This happened with the
    // windows implementation of MemoryMapDataFile ages ago,
    // and I'm pulling over their solution which is to ensure
    // that memory mapping does not happen unless the disk is
    // local AND non-ejectable.
    if (!CHXFileSpecUtils::IsDiskLocal(dirSpec) || CHXFileSpecUtils::IsDiskEjectable(dirSpec))
    {
        bMemMappingOK = false;
    }
    
    if (!bMemMappingOK)
    {
        if (MmapHandle && m_pMMM)
        {
            m_pMMM->CloseMap(MmapHandle);
            MmapHandle = 0;
        }
        HX_RELEASE(m_pMMM);
	m_pMMM = new MemoryMapManager(m_pContext, TRUE, m_ulChunkSize);
    }
    
#endif

    m_pFilename->Set((BYTE *)pFilename, strlen(pFilename)+1);

    DPRINTF(0x5d000000, ("MemoryMapDataFile::Bind(%s)\n", 
	    (const char *)m_pFilename->GetBuffer()));
}


/* Creates a datafile using the specified mode
 * uOpenMode --- File Open mode - HX_FILEFLAG_READ/HX_FILEFLAG_WRITE/HX_FILEFLAG_BINARY
 */
STDMETHODIMP
MemoryMapDataFile::Create(UINT16 uOpenMode)
{
    return HXR_NOTIMPL;
}


/* Open will open a file with the specified mode
 */
STDMETHODIMP
MemoryMapDataFile::Open(UINT16 uOpenMode)
{
    DPRINTF(0x5d000000, ("MemoryMapDataFile::Open()\n"));
    m_ulLastError = HXR_OK;

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
    {
	if (m_bEnableFileLocking)
	{
	    UnlockFile();
	}
	if (close(m_nFD) < 0)
        {
            m_ulLastError = errno;
            return HXR_FAIL;
        }
    }

    // open file
    m_ulLastError = HXR_OK;
#define FILEPERM (S_IREAD | S_IWRITE)
    if ((m_nFD = open((const char *)m_pFilename->GetBuffer(), 
        mode, FILEPERM)) < 0)
    {
        m_ulLastError = errno;
        fprintf(stderr, "open() failed with %d %s\n", m_ulLastError, strerror(m_ulLastError));

        if (m_ulLastError == EMFILE)
        {
            return HXR_NO_MORE_FILES;
        }
        else
        {
            return HXR_DOC_MISSING;
        }
    }

#if defined(HELIX_FEATURE_SERVER) && defined(_SOLARIS) 
    if (m_nFD < 256)
    {
        int nNewFD = fcntl(m_nFD, F_DUPFD, 256);
        int nTemp = m_nFD;
        m_nFD = nNewFD;
        close(nTemp);
    }
#endif //defined(HELIX_FEATURE_SERVER) && defined (_SOLARIS)

    // change permissions to allow everyone to read the file and owner to write
    // only if I have to create this file
#ifndef _BEOS
    if (mode & O_CREAT)
        fchmod(m_nFD, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif

    if (m_bEnableFileLocking)
    {
        HXBOOL bRet = LockFile();
        HX_ASSERT(bRet);
    }

    if (MmapHandle)
    {
        m_pMMM->CloseMap(MmapHandle);
        MmapHandle = 0;
    }
    MmapHandle = m_pMMM->OpenMap(m_nFD, m_pContext);
    /*
     * XXXSMP
     * This class doesn't need to keep the original file open if we 
     * are using mmap() on the file.
     */
    m_ulPos = 0;
    m_ulFilePointerPos = 0;

    return HXR_OK;
}

/* Delete File */
STDMETHODIMP
MemoryMapDataFile::Delete()
{
    Close();
    if (unlink((const char*)m_pFilename->GetBuffer()) != 0)
    {
        return HXR_FAIL;
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
MemoryMapDataFile::Close()
{
    m_ulLastError = HXR_OK;

    if (m_nFD > 0)
    {
        if (m_bEnableFileLocking)
        {
            UnlockFile();
        }
        if (close(m_nFD) < 0)
        {
            m_ulLastError = errno;
        }
        m_nFD = -1;
        if (MmapHandle)
        {
            m_pMMM->CloseMap(MmapHandle);
            MmapHandle = 0;
        }
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
    return FALSE;
}


/*
 * IsOpen returns TRUE if file is open.  Otherwise FALSE.
 */
inline HXBOOL
MemoryMapDataFile::IsOpen()
{
    return (m_nFD > 0 ? TRUE : FALSE);
}


/* Seek moves the current file position to the offset from the
 * fromWhere specifier returns current position of file or -1 on
 * error.
 */
STDMETHODIMP
MemoryMapDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    ULONG32 offset2 = 0;
    m_ulLastError = HXR_OK;

    switch (fromWhere)
    {
    case SEEK_CUR:
	m_ulPos += offset;
	m_ulFilePointerPos += offset;
	break;
    case SEEK_SET:
	if (((LONG32) offset) < 0)
	{
	    offset2 = (offset & 0x01);
	    offset = (offset >> 1);
	    offset2 += offset;
	}
	m_ulPos = offset;
	m_ulFilePointerPos = offset;
	break;
    default:
	ASSERT(0);
	break;
    }

    if (m_nFD > 0)
    {
	if (lseek(m_nFD, offset, fromWhere) < 0)
	{
	    m_ulLastError = errno;
	    return HXR_INVALID_FILE;
	}

	if (offset2 == 0)
	{
	    return HXR_OK;
	}
	else
	{
	    if (((LONG32) offset2) > 0)
	    {
		return Seek(offset2, SEEK_CUR);
	    }
	}
    }

    return HXR_INVALID_FILE;
}


/* Tell returns the current file position in the file */
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Tell()
{
    INT32 offset = -1;
    if (MmapHandle)
    {
	return m_ulPos;
    }
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

void
MemoryMapDataFile::StopMmap()
{
    if (MmapHandle == NULL)
	return;
    m_pMMM->CloseMap(MmapHandle);
    MmapHandle = 0;
    Seek(m_ulPos, SEEK_SET);
}

/* Read reads up to count bytes of data into buf.
 * returns the number of bytes read, EOF, or -1 if the read failed 
 */
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Read(REF(IHXBuffer *) pBuf, ULONG32 count)
{
    UINT32 ncnt = 0;           // number of bytes read
    m_ulLastError = HXR_OK;

    if (MmapHandle)
    {
	ncnt = m_pMMM->GetBlock(pBuf, MmapHandle, m_ulPos, count);
	if (ncnt >= MMAP_EXCEPTION)
	{
	    if (ncnt != MMAP_EOF_EXCEPTION)
	    {
		StopMmap();
	    }
	    else
	    {
		Seek(m_ulPos, SEEK_SET);
	    }

	    goto normal_read;
	}
	if (ncnt > 0)
	{
	    m_ulPos += ncnt;
	    m_ulFilePointerPos += ncnt;
	}
	return (ULONG32)ncnt;
    }

normal_read:

    if (HXR_OK != CreateBufferCCF(pBuf, m_pContext))
    {
	return 0;
    }

    pBuf->SetSize(count);

    if (m_nFD > 0)
    { 
	ULONG32 tmpCheck = Tell();
	if (tmpCheck != m_ulPos)
	{
	    if (lseek(m_nFD, m_ulPos, SEEK_SET) < 0)
            {
                m_ulLastError = errno;
                return 0;
            }
	}

	if ((int)(ncnt = read(m_nFD, (void *)pBuf->GetBuffer(), count)) < 0)
	{
	    m_ulLastError = errno;
	    pBuf->Release();
	    pBuf = NULL;
	    /*
	    * XXX PM Have to return 0 here because it is unsigned
	    * return value.
	    */
	    return 0;
	}
	else
	{
	    m_ulPos += ncnt;
	}
	if (ncnt < count)
	{
	    pBuf->SetSize(ncnt);
	}
    }

    return (ULONG32)ncnt;
}


/* Write writes up to count bytes of data from buf.
 * returns the number of bytes written, or -1 if the write failed 
 */
STDMETHODIMP_(ULONG32)
MemoryMapDataFile::Write(REF(IHXBuffer *) pBuf)
{

    if (m_ulPos != m_ulFilePointerPos)
    {
	Seek(m_ulPos, SEEK_SET);
    }

    /*
     * XXXSMP
     * This is silly.  This routine should create a buffer and pass it
     * back, just like read().
     */
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
	else
	{
	    m_ulPos += ncnt;
	    m_ulFilePointerPos += ncnt;
	}
    }
    pBuf->Release();

    return (ULONG32)ncnt;
}


/* Flush out the data in case of unbuffered I/O
 */
STDMETHODIMP
MemoryMapDataFile::Flush()
{
    StopMmap();
    return HXR_OK;
}


/*
 * Return info about the data file such as permissions, time of creation
 * size in bytes, etc.
 */
STDMETHODIMP
MemoryMapDataFile::Stat(struct stat* statbuf)
{
    //
    // XXXtbradley : we can't set m_ulLastError in this function because
    // the thing is defined as const
    //
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

    //
    // XXXtbradley : so we do this instead
    //
    if (errno == ETIMEDOUT)
    {
        return HXR_SERVER_TIMEOUT;
    }

    return HXR_FAIL;
}


/* Return the file descriptor */
inline INT16
MemoryMapDataFile::GetFd()
{
    return m_nFD;
}


/* GetLastError returns the platform specific file error */
STDMETHODIMP
MemoryMapDataFile::GetLastError()
{
    if (m_ulLastError == ETIMEDOUT)
    {
	return HXR_SERVER_TIMEOUT;
    }
    if (m_ulLastError != HXR_OK)
    {
	return HXR_FAIL;
    }

    return HXR_OK;
}


/* GetLastError returns the platform specific file error in
 * string form.
 */
STDMETHODIMP_(void)
MemoryMapDataFile::GetLastError(REF(IHXBuffer*) err)
{
}

HXBOOL
MemoryMapDataFile::LockFile()
{
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int ret;
    ret = fcntl(m_nFD, F_SETLK, &lock);
    HX_ASSERT(ret != -1);
    if (ret != -1)
    {
	m_bLockedIt = TRUE;
    }
    return m_bLockedIt;
}

HXBOOL
MemoryMapDataFile::UnlockFile()
{
    if (m_bLockedIt)
    {
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	fcntl(m_nFD, F_SETLK, &lock);
	m_bLockedIt = FALSE;
    }
    return TRUE;
}
