/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbihxdataf.cpp,v 1.14 2006/03/23 16:38:23 damann Exp $
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

/****************************************************************************
 *  Includes
 */
#include "hlxclib/string.h"
#include "hlxclib/fcntl.h"

#include "hxbuffer.h"
#include "hxassert.h"
#include "hlxosstr.h"
#include "hxstrutl.h"
#include "pckunpck.h"

#include "symbihxdataf.h"
#include "symbsessionmgr.h"


/****************************************************************************
 *  CSymbIHXDataFile
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CSymbIHXDataFile::CSymbIHXDataFile(IUnknown* pContext, IUnknown** ppCommonObj)
    : m_pFileName(NULL)
    , m_pSessionManager(NULL)
    , m_bHaveSession(FALSE)
    , m_bOpen(FALSE)
    , m_symbLastError(KErrNone)
    , m_lRefCount(0)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
    CSymbSessionMgr::Create(m_pSessionManager, ppCommonObj);
}

CSymbIHXDataFile::~CSymbIHXDataFile(void)
{
    Close();
    HX_RELEASE(m_pFileName);
    HX_RELEASE(m_pSessionManager);
    HX_RELEASE(m_pContext);
}


/****************************************************************************
 *  IHXDataFile methods
 */
STDMETHODIMP_(void) CSymbIHXDataFile::Bind(const char* pFileName)
{
    m_symbLastError = KErrNone;

    if (!m_pFileName)
    {
	CreateBufferCCF(m_pFileName, m_pContext);
    }

    Close();

    if (m_pFileName && pFileName)
    {
	if (m_pFileName->Set((UINT8*) pFileName, ::strlen(pFileName) + 1) != HXR_OK)
	{
	    HX_RELEASE(m_pFileName);
	}
    }
}

HX_RESULT CSymbIHXDataFile::_Create(UINT16 uOpenMode, HXBOOL bCreateFile)
{
    HX_RESULT retVal = HXR_FAIL;

    Close();

    if (m_pFileName && (m_pFileName->GetSize() != 0) && GetSession())
    {
	OS_STRING_TYPE osFileName(m_pFileName->GetBuffer());
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));
	TUint symbFileMode = TranslateOpenMode(uOpenMode);
	
	if (bCreateFile)
	{
	    m_symbLastError = m_symbFile.Create(m_symbSession, 
						symbNameDesc,
						symbFileMode);
	    if ((m_symbLastError != KErrNone) && 
		!(uOpenMode & HX_FILEFLAG_NOTRUNC))
	    {
		m_symbSession.Delete(symbNameDesc);
		m_symbLastError = m_symbFile.Create(m_symbSession, 
						    symbNameDesc,
						    symbFileMode);
	    }
	}
	else
	{
	    m_symbLastError = m_symbFile.Open(m_symbSession, 
					      symbNameDesc,
					      symbFileMode);
	}
	
	if (m_symbLastError == KErrNone)
	{
	    m_bOpen = TRUE;
	    retVal = HXR_OK;
	}
    }

    return retVal;
}

STDMETHODIMP CSymbIHXDataFile::Create(UINT16 uOpenMode)
{
   return _Create(uOpenMode, TRUE);
}
	
STDMETHODIMP CSymbIHXDataFile::Open(UINT16 uOpenMode)
{
   return _Create(uOpenMode, FALSE);
}

STDMETHODIMP CSymbIHXDataFile::Close()
{
    HX_RESULT retVal = HXR_OK;

    m_symbLastError = KErrNone;

    if (m_bOpen)
    {
	retVal = HXR_FAIL;

	if ((m_symbLastError = m_symbFile.Flush()) == KErrNone)
	{
	    retVal = HXR_OK;
	}
	m_symbFile.Close();
	m_bOpen = FALSE;
    }

    return retVal;
}

STDMETHODIMP_(HXBOOL) CSymbIHXDataFile::Name(REF(IHXBuffer*) pFileName)
{
    m_symbLastError = KErrNotReady;

    if (m_pFileName && (m_pFileName->GetSize() != 0))
    {
	m_symbLastError = KErrNone;
	pFileName = m_pFileName;
	pFileName->AddRef();
	return TRUE;
    }

    return FALSE;
}
		
STDMETHODIMP_(HXBOOL) CSymbIHXDataFile::IsOpen()
{
    m_symbLastError = KErrNone;
    return m_bOpen;
}

STDMETHODIMP CSymbIHXDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    HX_RESULT retVal = HXR_FAIL;
    TSeek symbSeekMode = TranslateSeekMode(fromWhere);
    TInt symbOffset = (TInt) offset;

    m_symbLastError = KErrNotReady;
    if (m_bOpen && ((m_symbLastError = m_symbFile.Seek(symbSeekMode, symbOffset)) == KErrNone))
    {
	retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CSymbIHXDataFile::Tell()
{
    TInt symbOffset = 0;

    m_symbLastError = KErrNotReady;
    if (m_bOpen)
    {
	m_symbLastError = m_symbFile.Seek(ESeekCurrent, symbOffset);
    }

    return (ULONG32) symbOffset;
}

STDMETHODIMP_(ULONG32) CSymbIHXDataFile::Read(REF(IHXBuffer*) pBuf,
					      ULONG32 ulSize)
{
    ULONG32 ulBytesRead = 0;

    m_symbLastError = KErrNotReady;
    if (m_bOpen)
    {
	IHXBuffer* pNewBuffer = NULL;
	
	CreateBufferCCF(pNewBuffer, m_pContext);
	if (pNewBuffer)
	{
	    if (pNewBuffer->SetSize(ulSize) == HXR_OK)
	    {
		TPtr8 symbBufferDesc((TUint8*) pNewBuffer->GetBuffer(), 
				     (TInt) ulSize);
		if ((m_symbLastError = m_symbFile.Read(symbBufferDesc)) == KErrNone)
		{
		    ulBytesRead = symbBufferDesc.Length();
		    
		    if (ulBytesRead == ulSize)
		    {
			pBuf = pNewBuffer;
			pNewBuffer = NULL;
		    }
		    else
		    {
			if (ulBytesRead < ulSize)
			{
			    if (pNewBuffer->SetSize(ulBytesRead) == HXR_OK)
			    {
				pBuf = pNewBuffer;
				pNewBuffer = NULL;
			    }
			}
			
			if (pNewBuffer)
			{
			    ulBytesRead = 0;
			}
		    }
		}
	    }
	}
	
	HX_RELEASE(pNewBuffer);
    }

    return ulBytesRead;
}
	
STDMETHODIMP_(ULONG32) CSymbIHXDataFile::Write(REF(IHXBuffer*) pBuf)
{
    ULONG32 ulBytesWritten = 0;
    ULONG32 ulBytesToWrite = pBuf->GetSize();
    
    m_symbLastError = KErrNotReady;
    if (m_bOpen)
    {
	TPtrC8 symbBufferDesc((TUint8*) pBuf->GetBuffer(), 
			      (TInt) ulBytesToWrite);

	if ((m_symbLastError = m_symbFile.Write(symbBufferDesc)) == KErrNone)
	{
	    ulBytesWritten = ulBytesToWrite;
	}
    }

    return ulBytesWritten;
}

STDMETHODIMP CSymbIHXDataFile::Flush()
{
    HX_RESULT retVal = HXR_FAIL;

    m_symbLastError = KErrNotReady;
    if (m_bOpen && ((m_symbLastError = m_symbFile.Flush()) == KErrNone))
    {
	retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CSymbIHXDataFile::Stat(struct stat* pStatBuffer)
{
    TInt symbSize = 0;
    HXBOOL bWasOpen = m_bOpen;
    HX_RESULT retVal = HXR_OK;
    
    if (!m_bOpen)
    {
	retVal = Open(HX_FILEFLAG_READ | HX_FILEFLAG_BINARY);
    }

    if (bWasOpen)
    {
	retVal = HXR_FAIL;
	if ((m_symbLastError = m_symbFile.Flush()) == KErrNone)
	{
	    retVal = HXR_OK;
	}
    }

    if (retVal == HXR_OK)
    {
	retVal = HXR_FAIL;
	if ((m_symbLastError = m_symbFile.Size(symbSize)) == KErrNone)
	{
	    retVal = HXR_OK;
	}
    }
	
    if (retVal == HXR_OK)
    {
	pStatBuffer->st_mode = 0;
	pStatBuffer->st_size = symbSize;
	pStatBuffer->st_atime = 0;
	pStatBuffer->st_ctime = 0;
	pStatBuffer->st_mtime = 0;
    }

    if (!bWasOpen)
    {
	Close();
    }

    return retVal;
}

STDMETHODIMP CSymbIHXDataFile::Delete()
{
    HX_RESULT retVal = HXR_FAIL;

    m_symbLastError = KErrNotReady;

    Close();

    if (m_pFileName && (m_pFileName->GetSize() != 0) && GetSession())
    {
	OS_STRING_TYPE osFileName(m_pFileName->GetBuffer());
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));

	if ((m_symbLastError = m_symbSession.Delete(symbNameDesc)) == KErrNone)
	{
	    retVal = HXR_OK;
	}
    }

    return retVal;
}	

STDMETHODIMP_(INT16) CSymbIHXDataFile::GetFd()
{
    INT16 iFd = -1;

    m_symbLastError = KErrNotReady;
    if (m_bOpen)
    {
	m_symbLastError = KErrNone;
	iFd = (INT16) m_symbFile.SubSessionHandle();
    }

    return iFd;
}
			
STDMETHODIMP CSymbIHXDataFile::GetLastError(void)
{
    HX_RESULT retVal = (m_symbLastError == KErrNone) ? HXR_OK : HXR_FAIL;

    return retVal;
}

STDMETHODIMP_(void) CSymbIHXDataFile::GetLastError(REF(IHXBuffer*) err)
{
    ;
}
	

/************************************************************************
 *  IHXAsyncDataFile methods
 */
STDMETHODIMP CSymbIHXDataFile::SetReceiver(IHXCallback* pCallback)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CSymbIHXDataFile::SeekAsync(REF(HX_RESULT) status, 
					 ULONG32 offset, UINT16 fromWhere)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CSymbIHXDataFile::ReadAsync(REF(ULONG32) ulReadSize,
					 REF(IHXBuffer*) pBuf, 
					 ULONG32 count)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CSymbIHXDataFile::WriteAsync(REF(ULONG32) ulWrittenSize,
					  REF(IHXBuffer*) pBuf)
{
    return HXR_NOTIMPL;
}


/****************************************************************************
 *  CSymbIHXDataFile custom public methods
 */
void CSymbIHXDataFile::Bind(IHXBuffer *pFileNameBuffer)
{
    Close();

    HX_RELEASE(m_pFileName);

    m_pFileName = pFileNameBuffer;

    if (m_pFileName)
    {
	m_pFileName->AddRef();
    }
}

HXBOOL CSymbIHXDataFile::GetTemporaryFileName(const char *tag, char* filename, UINT32 ulBufLen)
{
    HXBOOL bRetVal = FALSE;

    if (GetSession())
    {
	TFileName* psymbTempPath = new TFileName;

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
        if (m_symbSession.PrivatePath(*psymbTempPath) == KErrNone)
#else
	if (m_symbSession.DefaultPath(*psymbTempPath) == KErrNone)
#endif  
	{
	    TFileName* psymbTempFileName = new TFileName;

	    if (psymbTempFileName)
	    {
		RFile symbTempFile;

		if (symbTempFile.Temp(m_symbSession, 
				      *psymbTempPath, 
				      *psymbTempFileName, 
				      EFileShareExclusive) == KErrNone)
		{
		    filename[0] = '\0';
		    SafeStrCpy(filename, 
			       OS_STRING2((OS_TEXT_PTR) psymbTempFileName->Ptr(), 
					  psymbTempFileName->Length()), 
			       ulBufLen);

		    symbTempFile.Close();

		    m_symbSession.Delete(*psymbTempFileName);

		    bRetVal = TRUE;
		}

		delete psymbTempFileName;
	    }
	}

	HX_DELETE(psymbTempPath);
    }

    return bRetVal;
}


/************************************************************************
 *  Private methods
 */
TUint CSymbIHXDataFile::TranslateOpenMode(UINT16 uOpenMode)
{
    TUint symbMode = EFileShareAny;
    
    if (uOpenMode & HX_FILEFLAG_WRITE)
    {
	symbMode |= EFileWrite;
    }
    else if (uOpenMode & HX_FILEFLAG_READ)
    {
	symbMode |= EFileRead;
    }
    
    if (uOpenMode & HX_FILEFLAG_BINARY)
    {
	symbMode |= EFileStream;
    }

    return symbMode;
}

TSeek CSymbIHXDataFile::TranslateSeekMode(UINT16 fromWhere)
{
    switch (fromWhere)
    {
    case SEEK_SET:
	return ESeekStart;
    case SEEK_CUR:
	return ESeekCurrent;
    }

    return ESeekEnd;
}

HXBOOL CSymbIHXDataFile::GetSession(void)
{
    if (m_bHaveSession)
    {
	// Assume single thread usage for now
	return TRUE;
    }
    else if (m_pSessionManager)
    {
	m_bHaveSession = 
	    (m_pSessionManager->GetSession(m_symbSession) == HXR_OK);
    }
    
    return m_bHaveSession;
}


/************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP CSymbIHXDataFile::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) this },
		{ GET_IIDHANDLE(IID_IHXDataFile), (IHXDataFile*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CSymbIHXDataFile::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CSymbIHXDataFile::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
