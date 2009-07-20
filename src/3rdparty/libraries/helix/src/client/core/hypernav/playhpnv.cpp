/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: playhpnv.cpp,v 1.9 2006/10/07 14:01:04 ping Exp $
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

#include "hlxclib/stdio.h"
#if defined(_AIX)
#include <string.h>
#define strnicmp strncasecmp
#endif // _AIX
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxprefs.h"
#include "hxhyper.h"
#include "hxplugn.h"
#include "hxfiles.h"
#include "hxtick.h"
#include "hxengin.h"
#include "hxcore.h"
#include "playhpnv.h"

#include "dbcs.h"
#include "hxstring.h"
#include "hxrquest.h"
#include "nptime.h"
#include "chxpckts.h"
#include "hxstrutl.h"
#include "hxgroup.h"
#include "hxmon.h"
#include "dbcs.h"
#include "corshare.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
 *
 *  Interface:
 *
 *	HXHyperNavigate
 *
 *  Purpose:
 *
 *	TBD
 *
 *  IID_IHXHyperNavigate:
 *
 *	{00000900-61DF-11d0-9CEE-080017035B43}
 *
 */


PlayerHyperNavigate::PlayerHyperNavigate() :
      m_bInitialized(FALSE)
    , m_lRefCount(0)
    , m_pContext(NULL)
    , m_pPlayer(0)
    , m_pGroupManager(0)
    , m_pHyperNavigate(0)
    , m_pHyperNavigateWithContext(NULL)
    , m_pPendingRequest(NULL)
    , m_pScheduler(NULL)
    , m_CallbackHandle(0)
    , m_pFileObject(0)
{
}

PlayerHyperNavigate::~PlayerHyperNavigate()
{
    Close();
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your
//		object.
//
STDMETHODIMP PlayerHyperNavigate::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXHyperNavigate), (IHXHyperNavigate*)this },
            { GET_IIDHANDLE(IID_IHXHyperNavigate2), (IHXHyperNavigate2*)this },
            { GET_IIDHANDLE(IID_IHXHyperNavigateHint), (IHXHyperNavigateHint*)this },
            { GET_IIDHANDLE(IID_IHXHTTPRedirectResponse), (IHXHTTPRedirectResponse*)this },
            { GET_IIDHANDLE(IID_IHXFileSystemManagerResponse), (IHXFileSystemManagerResponse*)this },
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXHyperNavigate*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) PlayerHyperNavigate::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) PlayerHyperNavigate::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;

    return 0;
}


void     
PlayerHyperNavigate::Close(void)
{
    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pPlayer);
    HX_RELEASE(m_pGroupManager);
    HX_RELEASE(m_pHyperNavigate);
    HX_RELEASE(m_pHyperNavigateWithContext);
    HX_RELEASE(m_pPendingRequest);
    if (m_CallbackHandle && m_pScheduler)
    {
	m_pScheduler->Remove(m_CallbackHandle);
	m_CallbackHandle = 0;
    }
    HX_RELEASE(m_pScheduler);

}

STDMETHODIMP PlayerHyperNavigate::Init(IUnknown* pContext, IHXHyperNavigate* pHyperNavigate, 
				       IHXHyperNavigateWithContext* pHyperNavigateWithContext)
{
    if (!pContext)
    {
	return HXR_UNEXPECTED;
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    IHXPlayer* pPlayer = 0;

    if (pContext && pContext->QueryInterface(IID_IHXPlayer, (void**)&pPlayer) == HXR_OK)
    {
        m_pPlayer = pPlayer;
    }

    IHXGroupManager* pGroupManager = 0;

    if (m_pPlayer && m_pPlayer->QueryInterface(IID_IHXGroupManager, (void**)&pGroupManager) == HXR_OK)
    {
	m_pGroupManager = pGroupManager;
    }

    if (m_pPlayer)
    {
	m_pPlayer->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    }

    m_pHyperNavigate		= pHyperNavigate;
    m_pHyperNavigateWithContext = pHyperNavigateWithContext;

    if (m_pHyperNavigate)
	m_pHyperNavigate->AddRef();

    if (m_pHyperNavigateWithContext)
	m_pHyperNavigateWithContext->AddRef();

    m_bInitialized = TRUE;

    return HXR_OK;
}

/*
 *	IHXHyperNavigate methods
 */

#define URL_COMMAND_SEEK "seek("
#define URL_COMMAND_PLAY "play("
#define URL_COMMAND_PAUSE "pause("
#define URL_COMMAND_STOP "stop("
#define URL_COMMAND_PREVIOUSCLIP "previousclip("
#define URL_COMMAND_NEXTCLIP "nextclip("
#define URL_COMMAND_ADSCOOKIES "adscookies("

/************************************************************************
 *	Method:
 *	    IHXHyperNavigate::GoToURL
 *	Purpose:
 *	    Parses and delegates player commands or performs a simple Go To
 *          URL operation in the user's browser.
 */

STDMETHODIMP PlayerHyperNavigate::GoToURL(const char* pURL, const char* pTarget)
{
    return ExecuteWithContext(pURL, pTarget, NULL, NULL, NULL, NULL);
}


/************************************************************************
 *	Method:
 *	    IHXHyperNavigate2::Execute
 *	Purpose:
 *	    
 *	Parameters:
 *      pURL:	    URL (absolute or relative)
 *	    pTargetInstance:	
 *	    pTargetApplication: 
 *	    pTargetRegion:
 *	    pParams:
 */
STDMETHODIMP 
PlayerHyperNavigate::Execute(const char* pURL,
			     const char* pTargetInstance,
			     const char* pTargetApplication,
			     const char* pTargetRegion,
			     IHXValues* pParams)
{
    return ExecuteWithContext(pURL, pTargetInstance, pTargetApplication,
			      pTargetRegion, pParams, NULL);
}


HX_RESULT
PlayerHyperNavigate::ExecuteWithContext(const char* pURL,
					const char* pTargetInstance,
					const char* pTargetApplication,
					const char* pTargetRegion,
					IHXValues* pParams,
					IUnknown*	pContext)
{
    if (!m_bInitialized)
    {
	return HXR_NOT_INITIALIZED;
    }

    if (pURL == NULL)
    {
	return HXR_FAIL;
    }

    HX_RESULT hxrResult = HXR_NOTIMPL;

    // first, deleegate to the alternate hypernavigation interface

    if (m_pHyperNavigateWithContext)
    {
	hxrResult = m_pHyperNavigateWithContext->ExecuteWithContext(pURL, pTargetInstance, 
		    pTargetApplication, pTargetRegion, pParams, pContext);
    }
    else if (m_pHyperNavigate)
    {
	hxrResult = m_pHyperNavigate->GoToURL(pURL, pTargetInstance);
    }

    // if alternate hypernavigation says it doesn't implement
    if (hxrResult != HXR_NOTIMPL)
	return(hxrResult);

    return HandleCommands(pURL, pTargetInstance, pParams);
}

STDMETHODIMP 
PlayerHyperNavigate::Hint(const char* pURL,
                          const char* pTarget,
                          IHXValues*  pParams)
{
    HX_RESULT		    hxrResult = HXR_NOTIMPL;
    IHXHyperNavigateHint*   pHyperNavigateHint = NULL;

    if (m_pHyperNavigateWithContext)
    {
	m_pHyperNavigateWithContext->QueryInterface(IID_IHXHyperNavigateHint, (void**) &pHyperNavigateHint);
    }

    if (!pHyperNavigateHint && m_pHyperNavigate)
    {
	m_pHyperNavigate->QueryInterface(IID_IHXHyperNavigateHint, (void**) &pHyperNavigateHint);
    }

    if (pHyperNavigateHint)
    {
	hxrResult = pHyperNavigateHint->Hint(pURL, pTarget, pParams);
	HX_RELEASE(pHyperNavigateHint);
    }

    return hxrResult;
}

HX_RESULT
PlayerHyperNavigate::HandleCommands(const char* pURL, const char* pTarget, IHXValues* pParams)
{
    HX_RESULT	hxrResult = HXR_NOTIMPL;
    char*	pArgument = NULL;

    //
    // Check for player commands.
    //
    if (strnicmp(pURL, URL_COMMAND, sizeof(URL_COMMAND) - 1) == 0)
    {
	const char *pOpen = HXFindChar(pURL, '(');
	const char *pClose = HXReverseFindChar(pURL, ')');

	if (! pOpen || ! pClose || (pOpen > pClose))
	{
	    return HXR_FAILED;
	}

	// beginning of actual player command
	const char* pCommand = pURL + sizeof(URL_COMMAND) - 1;

	int   iLength = pClose - (pOpen + 1);

	if (strnicmp(pCommand, URL_COMMAND_SEEK, sizeof(URL_COMMAND_SEEK) - 1) == 0)
	{
	    if (m_pPlayer)
	    {
		pArgument = new_string(pOpen + 1, iLength);
		if (!pArgument)
		{
		    return HXR_OUTOFMEMORY;
		}

		NPTime  time(pArgument);

		hxrResult = m_pPlayer->Seek(time);
	    }
	}
	else if (strnicmp(pCommand, URL_COMMAND_PLAY, sizeof(URL_COMMAND_PLAY) - 1) == 0)
	{
	    if ((iLength == 0) && m_pPlayer)
	    {
		hxrResult = m_pPlayer->Begin();
	    }
	}
	else if (strnicmp(pCommand, URL_COMMAND_PAUSE, sizeof(URL_COMMAND_PAUSE) - 1) == 0)
	{
	    if ((iLength == 0) && m_pPlayer)
	    {
		hxrResult = m_pPlayer->Pause();
	    }
	}
	else if (strnicmp(pCommand, URL_COMMAND_STOP, sizeof(URL_COMMAND_STOP) - 1) == 0)
	{
	    if ((iLength == 0) && m_pPlayer)
	    {
		hxrResult = m_pPlayer->Stop();
	    }
	}
	else if (strnicmp(pCommand, URL_COMMAND_PREVIOUSCLIP, sizeof(URL_COMMAND_PREVIOUSCLIP) - 1) == 0)
	{
	    if ((iLength == 0) && m_pGroupManager)
	    {
		UINT16  iCurrent = 0;
		
		m_pGroupManager->GetCurrentGroup(iCurrent);

		// play previous clip if one exists
		if ((iCurrent > 0) && ((iCurrent - 1) < m_pGroupManager->GetGroupCount()))
		{
		    hxrResult = m_pGroupManager->SetCurrentGroup(iCurrent - 1);
		}
	    }
	}
	else if (strnicmp(pCommand, URL_COMMAND_NEXTCLIP, sizeof(URL_COMMAND_NEXTCLIP) - 1) == 0)
	{
	    if ((iLength == 0) && m_pGroupManager)
	    {
		UINT16  iCurrent = 0;
		
		m_pGroupManager->GetCurrentGroup(iCurrent);

		// play next clip if one exists
		if (iCurrent < m_pGroupManager->GetGroupCount())
		{
		    hxrResult = m_pGroupManager->SetCurrentGroup(iCurrent + 1);
		}
	    }
	}
	else if (strnicmp(pCommand, URL_COMMAND_ADSCOOKIES, sizeof(URL_COMMAND_ADSCOOKIES) - 1) == 0)
	{
	    pArgument = new_string(pOpen + 1, iLength);
	    if (!pArgument)
	    {
		return HXR_OUTOFMEMORY;
	    }

	    hxrResult = SendAdsCookies(pArgument);
	}

	HX_VECTOR_DELETE(pArgument);
	return hxrResult;
    }
    else if (pTarget && stricmp(pTarget, "_player") == 0)
    {
	/* we want to unwind the stack...since it may come from
	 * an OnTimeSync() into syncMM renderer...and if we call 
	 * OpenURL() syncronously, bad things happen in the
	 * core (srcinfo gets deleted).
	 * use our friendly scheduler...
	 */
	HX_RELEASE(m_pPendingRequest);
	IHXCommonClassFactory* pCommonClassFactory = NULL;
	m_pPlayer->QueryInterface(IID_IHXCommonClassFactory,
						(void**)&pCommonClassFactory);
	if (pCommonClassFactory)
	{
	    pCommonClassFactory->CreateInstance(IID_IHXRequest, (void**) &m_pPendingRequest);
	    HX_ASSERT(m_pPendingRequest);

	    if (m_pPendingRequest)
	    {
		m_pPendingRequest->SetURL(pURL);
		if (pParams)
		{
		    m_pPendingRequest->SetRequestHeaders(pParams);
		}
	    }

	    HX_RELEASE(pCommonClassFactory);
	}

	if (m_pScheduler && m_pPendingRequest)
	{
	    if (!m_CallbackHandle)
	    {
		m_CallbackHandle = m_pScheduler->RelativeEnter((IHXCallback*) this, 0);
	    }
	}
	else
	{
	    Func();
	}

	return HXR_OK;
    }

    return HXR_FAILED;
}
/////////////////////////////////////////////////////////////////////////
//  Method:
//  	IHXHyperNavigateRedirectResponse::RedirectDone
//
STDMETHODIMP
PlayerHyperNavigate::RedirectDone(IHXBuffer*   pBuffer)
{
    if (pBuffer && m_pHyperNavigate)
    {
	// deligate to alternate hypernavigation interface
	return m_pHyperNavigate->GoToURL((const char*)pBuffer->GetBuffer(), NULL);
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	IHXFileSystemManagerResponse::InitDone
 *	Purpose:
 */
STDMETHODIMP
PlayerHyperNavigate::InitDone(HX_RESULT status)
{
    return HXR_OK;
}

STDMETHODIMP
PlayerHyperNavigate::FileObjectReady(HX_RESULT status,
				     IUnknown* pObject)
{   
    HX_RESULT		theErr = HXR_OK;
    IHXHTTPRedirect*	pRedirect = NULL;

    if (!pObject)
    {
	theErr = HXR_FAILED;
    }
    else if (HXR_OK != pObject->QueryInterface(IID_IHXFileObject, (void**)&m_pFileObject))
    {
	theErr = HXR_FAILED;
    }
    else if (HXR_OK != pObject->QueryInterface(IID_IHXHTTPRedirect, (void**)&pRedirect))
    {
	theErr = HXR_FAILED;
    }
    else
    {
	theErr = pRedirect->Init((IHXHTTPRedirectResponse*)this);
    }

    HX_RELEASE(pRedirect);

    return theErr;
}

/*
 * The following method is deprecated and should return HXR_NOTIMPL
 */
STDMETHODIMP
PlayerHyperNavigate::DirObjectReady(HX_RESULT status,
				    IUnknown* pDirObject)
{
    return HXR_NOTIMPL;
}

/*
 *	IHXCallback methods
 */
STDMETHODIMP
PlayerHyperNavigate::Func()
{
    m_CallbackHandle = 0;

    if (m_pPlayer && m_pPendingRequest)
    {
	IHXPlayer2* pPlayer2 = NULL;
	m_pPlayer->QueryInterface(IID_IHXPlayer2, (void**) &pPlayer2);
	pPlayer2->OpenRequest(m_pPendingRequest);
	m_pPlayer->Begin();
	pPlayer2->Release();
	HX_RELEASE(m_pPendingRequest);
    }

    return HXR_OK;
}

HX_RESULT
PlayerHyperNavigate::SendAdsCookies(char* pURL)
{
    HX_RESULT		    hr = HXR_OK;
    IHXRequest*	    pRequest = NULL;
    IHXFileSystemManager*  pFileSystemManager = NULL;
    IHXCommonClassFactory* pCommonClassFactory = NULL;

    if (HXR_OK != m_pPlayer->QueryInterface(IID_IHXCommonClassFactory,
					    (void**)&pCommonClassFactory))
    {
	hr = HXR_FAILED;
    }
    else if (HXR_OK != pCommonClassFactory->CreateInstance(CLSID_IHXFileSystemManager,
						     (void**)&pFileSystemManager))
    {
	hr = HXR_FAILED;
    }
    else if (HXR_OK != pFileSystemManager->Init((IHXFileSystemManagerResponse*)this))
    {
	hr = HXR_FAILED;
    }
    else
    {
	IHXPreferences* pPreferences = NULL;
	if (m_pPlayer->QueryInterface(IID_IHXPreferences, (void **) &pPreferences) == HXR_OK)
	{
	    IHXRegistry* pRegistry = NULL;   
	    m_pPlayer->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);

	    ::SetRequest(pURL,
			 FALSE,
			 pPreferences,
			 pRegistry,
			 NULL,
                         pCommonClassFactory,
			 pRequest);

	    HX_RELEASE(pRegistry);
	}
	HX_RELEASE(pPreferences);

	hr = pFileSystemManager->GetNewFileObject(pRequest, 0);
    }

    HX_RELEASE(pRequest);
    HX_RELEASE(pFileSystemManager);
    HX_RELEASE(pCommonClassFactory);

    return hr;	
}
