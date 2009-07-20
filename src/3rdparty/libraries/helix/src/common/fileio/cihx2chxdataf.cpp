/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cihx2chxdataf.cpp,v 1.10 2007/07/06 20:35:11 jfinnecy Exp $
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

/****************************************************************************
 *  Includes
 */
#include "hlxclib/string.h"
#include "hlxclib/fcntl.h"
#include "hlxclib/sys/stat.h"

#include "hxbuffer.h"
#include "hxassert.h"

#include "chxdataf.h"
#include "cihx2chxdataf.h"
#include "pckunpck.h"


/****************************************************************************
 *  CIHX2CHXDataFile
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CIHX2CHXDataFile::CIHX2CHXDataFile(IUnknown* pContext, UINT32 ulFlags, IUnknown** ppCommonObj)
    : m_pCHXFile(CHXDataFile::Construct(pContext, ulFlags, ppCommonObj))
    , m_pFileName(NULL)
    , m_bOpen(FALSE)
    , m_lRefCount(0)   
{
    m_pContext = pContext;
    HX_RELEASE(m_pContext);
}

CIHX2CHXDataFile::~CIHX2CHXDataFile(void)
{
    HX_DELETE(m_pCHXFile);
    HX_RELEASE(m_pFileName);
    HX_RELEASE(m_pContext);
}


/****************************************************************************
 *  IHXDataFile methods
 */
STDMETHODIMP_(void) CIHX2CHXDataFile::Bind(const char* pFileName)
{
    if (!m_pFileName)
    {
	CreateBufferCCF(m_pFileName, m_pContext);
    }

    if (m_bOpen)
    {
	Close();
    }

    if (m_pFileName && pFileName)
    {
	if (m_pFileName->Set((UINT8*)pFileName, ::strlen(pFileName) + 1) != HXR_OK)
	{
	    HX_RELEASE(m_pFileName);
	}
    }
}

STDMETHODIMP CIHX2CHXDataFile::Create(UINT16 uOpenMode)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pCHXFile)
    {
	if (m_pFileName && (m_pFileName->GetSize() != 0))
	{
	    retVal = HXR_OK;

	    if (uOpenMode & HX_FILEFLAG_NOTRUNC)
	    {
		if (m_pCHXFile->Open((char*) m_pFileName->GetBuffer(),
				     HX_FILEFLAG_READ | HX_FILEFLAG_BINARY)
		    == HXR_OK)
		{
		    m_pCHXFile->Close();
		    retVal = HXR_FAIL;	// file exists - cannot destroy
		}
	    }
    
	    if (retVal == HXR_OK)
	    {
		retVal = m_pCHXFile->Create((char*) m_pFileName->GetBuffer(), 
					    TranslateMode(uOpenMode));
	    }
	    
	    m_bOpen = (retVal == HXR_OK);
	}
    }

    return retVal;
}
	
STDMETHODIMP CIHX2CHXDataFile::Open(UINT16 uOpenMode)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pCHXFile)
    {
	if (m_pFileName && (m_pFileName->GetSize() != 0))
	{
	    retVal = m_pCHXFile->Open((char*) m_pFileName->GetBuffer(), 
				      TranslateMode(uOpenMode));
	    m_bOpen = SUCCEEDED(retVal);
	}
    }

    return retVal;
}

STDMETHODIMP CIHX2CHXDataFile::Close()
{
    HX_RESULT retVal = HXR_OK;

    if (m_pCHXFile)
    {
	retVal = m_pCHXFile->Close();
	m_bOpen = (m_bOpen && FAILED(retVal));
    }

    return retVal;
}

STDMETHODIMP_(HXBOOL) CIHX2CHXDataFile::Name(REF(IHXBuffer*) pFileName)
{
    if (m_pFileName && (m_pFileName->GetSize() != 0))
    {
	pFileName = m_pFileName;
	pFileName->AddRef();
	return TRUE;
    }

    return FALSE;
}
		
STDMETHODIMP_(HXBOOL) CIHX2CHXDataFile::IsOpen()
{
    return m_bOpen;
}

STDMETHODIMP CIHX2CHXDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pCHXFile)
    {
	retVal = m_pCHXFile->Seek(offset, fromWhere);
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CIHX2CHXDataFile::Tell()
{
    ULONG32 ulPos = 0;

    if (m_pCHXFile)
    {
	ulPos = m_pCHXFile->Tell();
    }

    return ulPos;
}

STDMETHODIMP_(ULONG32) CIHX2CHXDataFile::Read(REF(IHXBuffer*) pBuf,
					      ULONG32 ulSize)
{
    ULONG32 ulBytesRead = 0;

    if (m_pCHXFile)
    {
	if (SUCCEEDED(m_pCHXFile->ReadToBuffer(ulSize, &pBuf)))
	{
	    ulBytesRead = pBuf->GetSize();
	}
    }

    return ulBytesRead;
}
	
STDMETHODIMP_(ULONG32) CIHX2CHXDataFile::Write(REF(IHXBuffer*) pBuf)
{
    ULONG32 ulBytesWritten = 0;

    if (m_pCHXFile)
    {
	UINT8* pData = NULL;
	ULONG32 ulSize = 0;

	if (pBuf->Get(pData, ulSize) == HXR_OK)
	{
	    ulBytesWritten = m_pCHXFile->Write((const char*) pData, ulSize);
	}
    }

    return ulBytesWritten;
}

STDMETHODIMP CIHX2CHXDataFile::Flush()
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_bOpen)
    {
	retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CIHX2CHXDataFile::Stat(struct stat* pStatBuffer)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pFileName && (m_pFileName->GetSize() != 0))
    {
	retVal = HXR_OK;

	if (stat((char*) m_pFileName->GetBuffer(), pStatBuffer) != 0)
	{
	    retVal = HXR_FAIL;

	    if (m_pCHXFile && m_bOpen)
	    {
		ULONG32 ulSize = m_pCHXFile->GetSize();

		retVal = HXR_OK;

		if (ulSize == 0)
		{
		    retVal = m_pCHXFile->GetLastError();
		}

		if (retVal == HXR_OK)
		{
		    pStatBuffer->st_mode = 0;
		    pStatBuffer->st_size = ulSize;
		    pStatBuffer->st_atime = 0;
		    pStatBuffer->st_ctime = 0;
		    pStatBuffer->st_mtime = 0;

		    retVal = HXR_INCOMPLETE;
		}
	    }
	}
    }

    return retVal;
}

STDMETHODIMP CIHX2CHXDataFile::Delete()
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pCHXFile)
    {
	if (m_bOpen)
	{
	    Close();
	}

	if (m_pFileName && (m_pFileName->GetSize() != 0))
	{
	    retVal = m_pCHXFile->Delete((const char*) m_pFileName->GetBuffer());
	}
    }

    return retVal;
}	

STDMETHODIMP_(INT16) CIHX2CHXDataFile::GetFd()
{
    INT16 iFd = -1;

    if (m_pCHXFile)
    {
	iFd = m_pCHXFile->GetFd();
    }

    return iFd;
}
			
STDMETHODIMP CIHX2CHXDataFile::GetLastError(void)
{
    HX_RESULT retVal = HXR_OK;

    if (m_pCHXFile)
    {
	retVal = m_pCHXFile->GetLastError();
    }

    return retVal;
}

STDMETHODIMP_(void) CIHX2CHXDataFile::GetLastError(REF(IHXBuffer*) err)
{
    ;
}
	

/************************************************************************
 *  Private methods
 */
UINT16 CIHX2CHXDataFile::TranslateMode(UINT16 uOpenMode)
{
    UINT16 uMode = 0;
    
    if ((uOpenMode & HX_FILEFLAG_READ) && (uOpenMode & HX_FILEFLAG_WRITE))
    {
	uMode |= O_RDWR;
    }
    else if (uOpenMode & HX_FILEFLAG_READ)
    {
	uMode |= O_RDONLY;
    }
    else if (uOpenMode & HX_FILEFLAG_WRITE)
    {
	uMode |= O_WRONLY;
    }
    
    if (uOpenMode & HX_FILEFLAG_BINARY)
    {
	uMode |= O_BINARY;
    }

    return uMode;
}


/************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP CIHX2CHXDataFile::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXDataFile), (IHXDataFile*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

STDMETHODIMP_(ULONG32) CIHX2CHXDataFile::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CIHX2CHXDataFile::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
