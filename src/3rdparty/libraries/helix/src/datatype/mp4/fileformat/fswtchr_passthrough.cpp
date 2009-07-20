/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
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
#include "fswtchr_passthrough.h"
#include "hxassert.h"
#include "qtffrefcounter.h"
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"       // IHXMediaBytesToMediaDur, IHXPDStatusMgr
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS) */


/****************************************************************************
 *  Class CFileSwitcherPassthrough
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CFileSwitcherPassthrough::CFileSwitcherPassthrough(void)
    : m_State(FSWCHR_Offline)
    , m_pFileObject(NULL)
    , m_pResponse(NULL)
    , m_lRefCount(0)
{
    g_nRefCount_qtff++;
}


CFileSwitcherPassthrough::~CFileSwitcherPassthrough()
{
    Reset();
    g_nRefCount_qtff--;
}


/****************************************************************************
 *  IHXFileSwitcher private methods
 */
/****************************************************************************
 *  Reset
 */
void CFileSwitcherPassthrough::Reset(void)
{
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pFileObject);
}


/****************************************************************************
 *  IHXFileSwitcher methods - Main interface
 */
/****************************************************************************
 *  Init
 */
STDMETHODIMP CFileSwitcherPassthrough::Init(IHXFileObject* pFileObject,
					    ULONG32 ulFlags,
					    IHXFileResponse* pResponse,
					    IUnknown* pContext,
					    UINT16 uMaxChildCount)
{
    HX_RESULT retVal = HXR_OK;

    if (m_State != FSWCHR_Offline)
    {
	retVal = HXR_UNEXPECTED;
    }

    HX_ASSERT(pFileObject);
    HX_ASSERT(pResponse);

    m_pFileObject = pFileObject;
    retVal = HXR_FAIL;
    if (m_pFileObject)
    {
	m_pFileObject->AddRef();
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_State = FSWCHR_ProcInit;

	retVal = pFileObject->Init(ulFlags, (IHXFileResponse*) this);
        if( retVal != HXR_OK )
        {
            HandleFailureSync();
        }
    }

    return retVal;
}

/****************************************************************************
 *  Read
 */
STDMETHODIMP CFileSwitcherPassthrough::Read(ULONG32 ulSize,
					    IHXFileResponse* pResponse,
					    const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State == FSWCHR_Ready) && !pFileName)
    {
	HX_ASSERT(!m_pResponse);

	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_State = FSWCHR_ProcRead;

	retVal = m_pFileObject->Read(ulSize);
        if( retVal != HXR_OK )
        {
	    HandleFailureSync();
        }
    }

    return retVal;	
}

/****************************************************************************
 *  Write
 */
STDMETHODIMP CFileSwitcherPassthrough::Write(IHXBuffer* pBuffer,
					     IHXFileResponse* pResponse,
					     const char* pFileName)
{
    return HXR_NOTIMPL;
}

/****************************************************************************
 *  Seek
 */
STDMETHODIMP CFileSwitcherPassthrough::Seek(ULONG32 ulOffset,
					    HXBOOL bRelative,
					    IHXFileResponse* pResponse,
					    const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State == FSWCHR_Ready) && !pFileName)
    {
	HX_ASSERT(!m_pResponse);

	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_State = FSWCHR_ProcSeek;

	retVal = m_pFileObject->Seek(ulOffset, bRelative);
        if( retVal != HXR_OK )
        {
	    HandleFailureSync();
        }
    }

    return retVal;
}

/****************************************************************************
 *  Advise
 */
STDMETHODIMP CFileSwitcherPassthrough::Advise(ULONG32 ulInfo,
					      const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State == FSWCHR_Ready) && !pFileName)
    {
	HX_ASSERT(!m_pResponse);

	retVal = m_pFileObject->Advise(ulInfo);
    }

    return retVal;
}

/****************************************************************************
 *  Close
 */
STDMETHODIMP CFileSwitcherPassthrough::Close(IHXFileResponse *pResponse,
					     const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State != FSWCHR_Offline) && !pFileName)
    {
	HX_ASSERT(!m_pResponse);

	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_State = FSWCHR_ProcClose;

	retVal = m_pFileObject->Close();
    }

    return retVal;
}


/****************************************************************************
 *  IHXFileResponse methods
 */
/****************************************************************************
 *  InitDone
 */
STDMETHODIMP CFileSwitcherPassthrough::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    if (m_State == FSWCHR_ProcInit)
    {
	// Initialization complete
	IHXFileResponse* pResponse = m_pResponse;
	
	HX_ASSERT(pResponse);
	
	m_pResponse = NULL;
	if (SUCCEEDED(status))
	{
	    m_State = FSWCHR_Ready;
	}
	
	retVal = pResponse->InitDone(status);
	pResponse->Release();
    }

    return retVal;
}

/****************************************************************************
 *  CloseDone
 */
STDMETHODIMP CFileSwitcherPassthrough::CloseDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == FSWCHR_ProcClose)
    {
	// Close completion
	IHXFileResponse* pResponse = m_pResponse;
	
	HX_ASSERT(pResponse);
	
	m_pResponse = NULL;

	Reset();
	m_State = FSWCHR_Offline;

	retVal = pResponse->CloseDone(status);
	pResponse->Release();
    }

    return retVal;
}

/****************************************************************************
 *  ReadDone
 */
STDMETHODIMP CFileSwitcherPassthrough::ReadDone(HX_RESULT status,
						IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == FSWCHR_ProcRead)
    {
	// Close completion
	IHXFileResponse* pResponse = m_pResponse;
	
	HX_ASSERT(pResponse);
	
	m_pResponse = NULL;
	m_State = FSWCHR_Ready;

	retVal = pResponse->ReadDone(status, pBuffer);
	pResponse->Release();
    }

    return retVal;
}

/****************************************************************************
 *  WriteDone
 */
STDMETHODIMP CFileSwitcherPassthrough::WriteDone(HX_RESULT status)
{
    return HXR_UNEXPECTED;
}

/****************************************************************************
 *  SeekDone
 */
STDMETHODIMP CFileSwitcherPassthrough::SeekDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == FSWCHR_ProcSeek)
    {
	IHXFileResponse *pResponse = m_pResponse;
	
	HX_ASSERT(m_pResponse);
	
	m_pResponse = NULL;
	m_State = FSWCHR_Ready;
	
	retVal = pResponse->SeekDone(status);
	
	pResponse->Release();
    }

    return retVal;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CFileSwitcherPassthrough::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXFileSwitcher))
    {
	AddRef();
	*ppvObj = (IHXFileSwitcher*) this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    else if (IsEqualIID(riid, IID_IHXMediaBytesToMediaDur))
    {
        if (m_pResponse)
        {
	    return m_pResponse->QueryInterface(IID_IHXMediaBytesToMediaDur,
                    ppvObj);
        }
    }
    else if (IsEqualIID(riid, IID_IHXPDStatusMgr))
    {
        if (m_pResponse)
        {
	    return m_pResponse->QueryInterface(IID_IHXPDStatusMgr,
                    ppvObj);
        }
    }
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CFileSwitcherPassthrough::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CFileSwitcherPassthrough::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/****************************************************************************
 *  HandleFailureSync
 */
void CFileSwitcherPassthrough::HandleFailureSync()
{
    // Synchronous failures may occur during Read, Seek, or Init.
    HX_RELEASE(m_pResponse);

    if (m_State == FSWCHR_ProcInit)
    {
	m_State = FSWCHR_Offline;
    }
    else
    {
	m_State = FSWCHR_Ready;
    }

    return;
}

