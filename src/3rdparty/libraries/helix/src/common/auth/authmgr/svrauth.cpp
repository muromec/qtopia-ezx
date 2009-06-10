/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: svrauth.cpp,v 1.6 2007/07/06 20:34:54 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxstrutl.h"
#include "hxcom.h"
#include "hxcomm.h"

//#include "iostream.h"

#include "hxauth.h"

#include "hxmon.h"
#include "hxplugn.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxauthn.h"
#include "hxdb.h"

#include "chxpckts.h"

#include "svrauth.h"

#include "hxplgsp.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

BEGIN_INTERFACE_LIST(CServerAuthenticator)
    INTERFACE_LIST_ENTRY
    (
	IID_IHXObjectConfiguration, 
	IHXObjectConfiguration
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXServerAuthConversation, 
	IHXServerAuthConversation
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXServerAuthResponse, 
	IHXServerAuthResponse
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXAuthenticationDBAccess, 
	IHXAuthenticationDBAccess
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXAuthenticationDBManager, 
	IHXAuthenticationDBManager
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXAuthenticationDBManagerResponse, 
	IHXAuthenticationDBManagerResponse
    )
END_INTERFACE_LIST

CServerAuthenticator::CServerAuthenticator()
    : m_HX_RESULTStatus(HXR_NOT_AUTHORIZED)
    , m_pClassFactory(NULL)
{
}

CServerAuthenticator::~CServerAuthenticator()
{
    HX_RELEASE(m_pClassFactory);
}

STDMETHODIMP
CServerAuthenticator::SetContext(IUnknown* pIUnknownContext)
{
    m_spIUnknownContext = pIUnknownContext;

    if (FAILED(pIUnknownContext->QueryInterface(IID_IHXCommonClassFactory,
        (void**)&m_pClassFactory)))
    {
        m_pClassFactory = NULL;
        return HXR_UNEXPECTED;
    }

    _TryToLoadPlugins();

    return HXR_OK;
}

STDMETHODIMP 
CServerAuthenticator::SetConfiguration
(
    IHXValues* pIHXValuesConfiguration
)
{
    m_spIHXValuesConfig = pIHXValuesConfiguration;

    _TryToLoadPlugins();

    return HXR_OK;
}

// IHXServerAuthConversation
STDMETHODIMP
CServerAuthenticator::MakeChallenge
(
    IHXServerAuthResponse* pIHXServerAuthResponseRequester,
    IHXRequest* pIHXRequestResponse
)
{
    HX_RESULT hr;

    if(!pIHXServerAuthResponseRequester || !m_pClassFactory)
    {
	return HXR_UNEXPECTED;
    }
    
    if(m_ListOfIUnknownPlugins.begin() == m_ListOfIUnknownPlugins.end() ||
	m_spIHXServerAuthResponseRequester.IsValid())
    {
	pIHXServerAuthResponseRequester->ChallengeReady(HXR_UNEXPECTED, NULL);
	return HXR_UNEXPECTED;
    }

    DECLARE_SMART_POINTER(IHXServerAuthConversation) 
        spIHXServerAuthConversationPlugin;

    m_HX_RESULTStatus = HXR_NOT_AUTHORIZED;
    m_spIHXServerAuthResponseRequester = pIHXServerAuthResponseRequester;
    m_spIHXRequestResponse = pIHXRequestResponse;
    m_ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();

    IHXValues* pValues = NULL;
    if (m_pClassFactory && SUCCEEDED(m_pClassFactory->CreateInstance(
        CLSID_IHXValues, (void**)&pValues)))
    {
        m_spIHXValuesChallengeHeaders = pValues;
        // the smart pointer creates its own ref on assign, so release
        pValues->Release();
    }

    spIHXServerAuthConversationPlugin = 
       (*m_ListOfIUnknownIteratorCurrent).wrapped_ptr();

    hr = spIHXServerAuthConversationPlugin->MakeChallenge(this, 
	    m_spIHXRequestResponse);
    if (FAILED(hr))
    {
	ChallengeReady(hr, m_spIHXRequestResponse);
    }

    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CServerAuthenticator::IsAuthenticated()
{
    _CListOfWrapped_IUnknown_::iterator ListOfIUnknownIteratorCurrent;
    DECLARE_SMART_POINTER
    (
	IHXServerAuthConversation
    )				    spIHXServerAuthConversationPlugin;
    HXBOOL			    bAuthenticated = FALSE;

    for
    (
	ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();
	ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end();
	++ListOfIUnknownIteratorCurrent
    )
    {
	spIHXServerAuthConversationPlugin = 
	(
	    *ListOfIUnknownIteratorCurrent
	).wrapped_ptr();

	bAuthenticated |= 
	    spIHXServerAuthConversationPlugin->IsAuthenticated();
    }

    return bAuthenticated;
}

STDMETHODIMP
CServerAuthenticator::GetUserContext(REF(IUnknown*) pIUnknownUser)
{
    return HXR_FAIL;
}

// IHXServerAuthResponse
STDMETHODIMP
CServerAuthenticator::ChallengeReady
(
    HX_RESULT	HX_RESULTStatus,
    IHXRequest* pIHXRequestChallenge
)
{
    DECLARE_SMART_POINTER(IHXServerAuthConversation)
                                        spIHXServerAuthConversationPlugin;
    DECLARE_SMART_POINTER(IHXBuffer)	spIHXBufferPropValue;
    DECLARE_SMART_POINTER(IHXValues)	spIHXValuesChallengeHeaders;
    HX_RESULT				HX_RESULTRet = HXR_FAIL;
    const char*				pcharPropName = NULL;

    if(FAILED(m_HX_RESULTStatus) && HX_RESULTStatus != HXR_FAIL)
    {
	m_HX_RESULTStatus = HX_RESULTStatus;
    }

    if(pIHXRequestChallenge)
    {
	pIHXRequestChallenge->GetResponseHeaders(
            spIHXValuesChallengeHeaders.ptr_reference());
    }

    if(SUCCEEDED(m_HX_RESULTStatus) && spIHXValuesChallengeHeaders.IsValid())
    {
	// Merge the Headers from this call
	CHXHeader::mergeHeaders(m_spIHXValuesChallengeHeaders,
	    spIHXValuesChallengeHeaders);
    }

    ++m_ListOfIUnknownIteratorCurrent;

    if(m_ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end())
    {
	HX_RESULT hr;

	// Call Next one..
	spIHXServerAuthConversationPlugin = 
            (*m_ListOfIUnknownIteratorCurrent).wrapped_ptr();

	hr = spIHXServerAuthConversationPlugin->MakeChallenge(
		this, m_spIHXRequestResponse);
	if (FAILED(hr))
	{
	    	ChallengeReady(hr, m_spIHXRequestResponse);
	}
    }
    else
    {
	m_spIHXRequestResponse->SetResponseHeaders(
            m_spIHXValuesChallengeHeaders);

	m_spIHXServerAuthResponseRequester->ChallengeReady(m_HX_RESULTStatus, 
	    m_spIHXRequestResponse);

	// We are done, cleanup..
	m_spIHXServerAuthResponseRequester.Release();
	m_spIHXRequestResponse.Release();
	m_spIHXValuesChallengeHeaders.Release();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXAuthenticationDBAccess::_NewEnum
 *	Purpose:
 *	    
 *	    Call this to make a new enumerator of this collection.
 *
 */
STDMETHODIMP
CServerAuthenticator::_NewEnum
(
    REF(IHXAsyncEnumAuthenticationDB*) pAsyncEnumAuthenticationDBNew
)
{
    if (m_ListOfIUnknownPlugins.begin() == m_ListOfIUnknownPlugins.end())
    {
	return HXR_UNEXPECTED;
    }

    DECLARE_SMART_POINTER
    (
	IHXAuthenticationDBAccess
    ) spAuthenticationDBAccessPlugin;
    _CListOfWrapped_IUnknown_::iterator ListOfIUnknownIteratorCurrent;
    HX_RESULT ResultStatus = HXR_FAIL;

    ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();
    while 
    (
	ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end()
	&&
	FAILED(ResultStatus)
    )
    {
	spAuthenticationDBAccessPlugin = 
	(
	    *ListOfIUnknownIteratorCurrent
	).wrapped_ptr();

	++ListOfIUnknownIteratorCurrent;

	if (!spAuthenticationDBAccessPlugin)
	{
	    ResultStatus = HXR_NOINTERFACE;
	    continue;
	}

	ResultStatus = spAuthenticationDBAccessPlugin->_NewEnum(pAsyncEnumAuthenticationDBNew);
    }

    return ResultStatus;
}

/************************************************************************
 *	Method:
 *	    IHXAuthenticationDBAccess::CheckExistence
 *	Purpose:
 *	    
 *	    Call this to verify the existance of a principal.
 *
 */
STDMETHODIMP
CServerAuthenticator::CheckExistence
(
    IHXAuthenticationDBAccessResponse* pAuthenticationDBAccessResponseNew,
    IHXBuffer*		pBufferPrincipalID
)
{
    if(!pAuthenticationDBAccessResponseNew)
    {
	return HXR_UNEXPECTED;
    }

    pAuthenticationDBAccessResponseNew->ExistenceCheckDone
    (
	HXR_NOTIMPL, 
	pBufferPrincipalID
    );

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXAuthenticationDBAccess::GetCredentials
 *	Purpose:
 *	    
 *	    Call this to access the credentials for the specified principal.
 *
 */
STDMETHODIMP
CServerAuthenticator::GetCredentials
(
    IHXAuthenticationDBAccessResponse* pAuthenticationDBAccessResponseNew,
    IHXBuffer*		pBufferPrincipalID
)
{
    if(!pAuthenticationDBAccessResponseNew)
    {
	return HXR_UNEXPECTED;
    }

    pAuthenticationDBAccessResponseNew->GetCredentialsDone
    (
	HXR_NOTIMPL, 
	pBufferPrincipalID,
	NULL
    );

    return HXR_OK;
}

// IHXAuthenticationDBManager
STDMETHODIMP
CServerAuthenticator::AddPrincipal
(
    IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
    IHXBuffer*		pBufferPrincipalID
)
{
//    printf("CServerAuthenticator: Entered - AddPrincipal\n");

    if(!pAuthenticationDBManagerResponseNew)
    {
//	printf("CServerAuthenticator: Exit - AddPrincipal (bad response interface)\n");
	return HXR_UNEXPECTED;
    }

    m_spAuthenticationDBManagerResponseRequester 
    = 
    pAuthenticationDBManagerResponseNew;

    if (m_ListOfIUnknownPlugins.begin() == m_ListOfIUnknownPlugins.end())
    {
	m_spAuthenticationDBManagerResponseRequester->AddPrincipalDone
	(
	    HXR_UNEXPECTED, 
	    pBufferPrincipalID
	);

	// Must explicitly release this to prevent possible circular 
	// references.
	m_spAuthenticationDBManagerResponseRequester.Release();

//	printf("CServerAuthenticator: Exit - AddPrincipal (no auth plugins loaded)\n");
	return HXR_UNEXPECTED;
    }

    DECLARE_SMART_POINTER
    (
	IHXAuthenticationDBManager
    ) spAuthenticationDBManagerPlugin;

    m_HX_RESULTStatus = HXR_FAIL;

    m_ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();

    spAuthenticationDBManagerPlugin = 
    (
	*m_ListOfIUnknownIteratorCurrent
    ).wrapped_ptr();

    if (!spAuthenticationDBManagerPlugin)
    {
	m_spAuthenticationDBManagerResponseRequester->AddPrincipalDone
	(
	    HXR_NOINTERFACE, 
	    pBufferPrincipalID
	);

	// Must explicitly release this to prevent possible circular 
	// references.
	m_spAuthenticationDBManagerResponseRequester.Release();

//	printf("CServerAuthenticator: Exit - AddPrincipal (this auth plugin does not support user management)\n");
	return HXR_NOINTERFACE;
    }

    spAuthenticationDBManagerPlugin->AddPrincipal
    (
	this,
	pBufferPrincipalID
    );

//    printf("CServerAuthenticator: Exit - AddPrincipal (Success)\n");
    return HXR_OK;
}

STDMETHODIMP
CServerAuthenticator::RemovePrincipal
(
    IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
    IHXBuffer*		pBufferPrincipalID
)
{
    if(!pAuthenticationDBManagerResponseNew)
    {
	return HXR_UNEXPECTED;
    }

    m_spAuthenticationDBManagerResponseRequester 
    = 
    pAuthenticationDBManagerResponseNew;

    if (m_ListOfIUnknownPlugins.begin() == m_ListOfIUnknownPlugins.end())
    {
	m_spAuthenticationDBManagerResponseRequester->RemovePrincipalDone
	(
	    HXR_UNEXPECTED, 
	    pBufferPrincipalID
	);

	// Must explicitly release this to prevent possible circular 
	// references.
	m_spAuthenticationDBManagerResponseRequester.Release();

	return HXR_UNEXPECTED;
    }

    DECLARE_SMART_POINTER
    (
	IHXAuthenticationDBManager
    ) spAuthenticationDBManagerPlugin;

    m_HX_RESULTStatus = HXR_FAIL;

    m_ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();

    spAuthenticationDBManagerPlugin = 
    (
	*m_ListOfIUnknownIteratorCurrent
    ).wrapped_ptr();

    if (!spAuthenticationDBManagerPlugin)
    {
	m_spAuthenticationDBManagerResponseRequester->RemovePrincipalDone
	(
	    HXR_NOINTERFACE, 
	    pBufferPrincipalID
	);

	// Must explicitly release this to prevent possible circular 
	// references.
	m_spAuthenticationDBManagerResponseRequester.Release();

	return HXR_NOINTERFACE;
    }

    spAuthenticationDBManagerPlugin->RemovePrincipal
    (
	this,
	pBufferPrincipalID
    );

    return HXR_OK;
}

STDMETHODIMP
CServerAuthenticator::SetCredentials
(
    IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
    IHXBuffer*		pBufferPrincipalID, 
    IHXBuffer*		pBufferCredentials
)
{
    if(!pAuthenticationDBManagerResponseNew)
    {
	return HXR_UNEXPECTED;
    }

    m_spAuthenticationDBManagerResponseRequester 
    = 
    pAuthenticationDBManagerResponseNew;

    if (m_ListOfIUnknownPlugins.begin() == m_ListOfIUnknownPlugins.end())
    {
	m_spAuthenticationDBManagerResponseRequester->SetCredentialsDone
	(
	    HXR_UNEXPECTED, 
	    pBufferPrincipalID
	);

	// Must explicitly release this to prevent possible circular 
	// references.
	m_spAuthenticationDBManagerResponseRequester.Release();

	return HXR_UNEXPECTED;
    }

    DECLARE_SMART_POINTER
    (
	IHXAuthenticationDBManager
    ) spAuthenticationDBManagerPlugin;

    m_HX_RESULTStatus = HXR_FAIL;

    m_spBufferCredentials = pBufferCredentials;

    m_ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();

    spAuthenticationDBManagerPlugin = 
    (
	*m_ListOfIUnknownIteratorCurrent
    ).wrapped_ptr();

    if (!spAuthenticationDBManagerPlugin)
    {
	m_spAuthenticationDBManagerResponseRequester->SetCredentialsDone
	(
	    HXR_NOINTERFACE, 
	    pBufferPrincipalID
	);

	// Must explicitly release this to prevent possible circular 
	// references.
	m_spAuthenticationDBManagerResponseRequester.Release();

	return HXR_NOINTERFACE;
    }

    spAuthenticationDBManagerPlugin->SetCredentials
    (
	this,
	pBufferPrincipalID,
	m_spBufferCredentials
    );

    return HXR_OK;
}

// IHXAuthenticationDBManagerResponse
STDMETHODIMP
CServerAuthenticator::AddPrincipalDone
(
    HX_RESULT		ResultStatus,
    IHXBuffer*		pBufferPrincipalID
)
{
//    printf("CServerAuthenticator: Entered - AddPrincipalDone (Result %lx)\n", ResultStatus);
    DECLARE_SMART_POINTER
    (
	IHXAuthenticationDBManager
    ) spAuthenticationDBManagerPlugin;
    HX_RESULT				HX_RESULTRet = HXR_FAIL;

    if(FAILED(m_HX_RESULTStatus) && ResultStatus != HXR_FAIL)
    {
	m_HX_RESULTStatus = ResultStatus;
    }

    ++m_ListOfIUnknownIteratorCurrent;

    if(m_ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end())
    {
	// Call Next one..
//	printf("CServerAuthenticator: Call - AddPrincipal on next plugin in list\n");
	spAuthenticationDBManagerPlugin = 
	(
	    *m_ListOfIUnknownIteratorCurrent
	).wrapped_ptr();

	spAuthenticationDBManagerPlugin->AddPrincipal
	(
	    this,
	    pBufferPrincipalID
	);
    }
    else
    {
	// We are done..
//	printf("CServerAuthenticator: Call - AddPrincipalDone on response interface (Result: %lx)\n", m_HX_RESULTStatus);
	m_spAuthenticationDBManagerResponseRequester->AddPrincipalDone
	(
	    m_HX_RESULTStatus, 
	    pBufferPrincipalID
	);
    }

    // Must explicitly release this to prevent possible circular references.
    m_spAuthenticationDBManagerResponseRequester.Release();

//    printf("CServerAuthenticator: Exit - AddPrincipalDone\n");
    return HXR_OK;
}

STDMETHODIMP
CServerAuthenticator::RemovePrincipalDone
(
    HX_RESULT		ResultStatus,
    IHXBuffer*		pBufferPrincipalID
)
{
    DECLARE_SMART_POINTER
    (
	IHXAuthenticationDBManager
    ) spAuthenticationDBManagerPlugin;
    HX_RESULT				HX_RESULTRet = HXR_FAIL;

    if(FAILED(m_HX_RESULTStatus) && ResultStatus != HXR_FAIL)
    {
	m_HX_RESULTStatus = ResultStatus;
    }

    ++m_ListOfIUnknownIteratorCurrent;

    if(m_ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end())
    {
	// Call Next one..
	spAuthenticationDBManagerPlugin = 
	(
	    *m_ListOfIUnknownIteratorCurrent
	).wrapped_ptr();

	spAuthenticationDBManagerPlugin->RemovePrincipal
	(
	    this,
	    pBufferPrincipalID
	);
    }
    else
    {
	// We are done..
	m_spAuthenticationDBManagerResponseRequester->RemovePrincipalDone
	(
	    m_HX_RESULTStatus, 
	    pBufferPrincipalID
	);
    }

    // Must explicitly release this to prevent possible circular references.
    m_spAuthenticationDBManagerResponseRequester.Release();

    return HXR_OK;
}

STDMETHODIMP
CServerAuthenticator::SetCredentialsDone
(
    HX_RESULT		ResultStatus,
    IHXBuffer*		pBufferPrincipalID
)
{
    DECLARE_SMART_POINTER
    (
	IHXAuthenticationDBManager
    ) spAuthenticationDBManagerPlugin;
    HX_RESULT				HX_RESULTRet = HXR_FAIL;

    if(FAILED(m_HX_RESULTStatus) && ResultStatus != HXR_FAIL)
    {
	m_HX_RESULTStatus = ResultStatus;
    }

    ++m_ListOfIUnknownIteratorCurrent;

    if(m_ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end())
    {
	// Call Next one..
	spAuthenticationDBManagerPlugin = 
	(
	    *m_ListOfIUnknownIteratorCurrent
	).wrapped_ptr();

	spAuthenticationDBManagerPlugin->SetCredentials
	(
	    this,
	    pBufferPrincipalID,
	    m_spBufferCredentials
	);
    }
    else
    {
	// We are done..
	m_spAuthenticationDBManagerResponseRequester->SetCredentialsDone
	(
	    m_HX_RESULTStatus, 
	    pBufferPrincipalID
	);
    }

    // Must explicitly release this to prevent possible circular references.
    m_spAuthenticationDBManagerResponseRequester.Release();

    return HXR_OK;
}

HX_RESULT 
CServerAuthenticator::_GetRealmSettings
(
    char*	    pcharRealm,
    IHXValues**    ppIHXValuesRealmProperties
)
{
    if(m_spIUnknownContext.IsValid())
    {
	DECLARE_SMART_POINTER(IHXRegistry)	    spregRegistry;
	DECLARE_SMART_POINTER(IHXBuffer)	    spIHXBufferRealmName;
	DECLARE_SMART_POINTER(IHXValues)	    spIHXValuesRealms;

	spregRegistry = m_spIUnknownContext;

	if
	(
	    FAILED
	    (
		spregRegistry->GetPropListByName
		(
		    "config.AuthenticationRealms", 
		    spIHXValuesRealms.ptr_reference()
		)
	    )
	)
	{
	    return HXR_FAIL;
	}

	HX_RESULT HX_RESULTRes;
	const char* pcharPropertyPath;
	UINT32 ulProperty_id;

	HX_RESULTRes = spIHXValuesRealms->GetFirstPropertyULONG32
	(
	    pcharPropertyPath, 
	    ulProperty_id
	);

	while(SUCCEEDED(HX_RESULTRes))
	{
	    spIHXBufferRealmName.Release();

	    HX_RESULTRes = _GetPluginDataFromID
	    (
		pcharPropertyPath,
		ulProperty_id,
		"Realm",
		&spIHXBufferRealmName,
		ppIHXValuesRealmProperties
	    );

	    if(SUCCEEDED(HX_RESULTRes))
	    {
//		cout << spIHXBufferRealmName->GetBuffer() << endl;

		if
		(
		    !strcasecmp
		    (
			pcharRealm, 
			(char*)spIHXBufferRealmName->GetBuffer()
		    )
		)
		{
		    return HX_RESULTRes;
		}
	    }

	    HX_RELEASE(*ppIHXValuesRealmProperties);

	    HX_RESULTRes = spIHXValuesRealms->GetNextPropertyULONG32
	    (
		pcharPropertyPath, 
		ulProperty_id
	    );
	}
    }

    return HXR_FAIL;
}

HX_RESULT 
CServerAuthenticator::_GetFirstPlugin
(
    IHXValues* pIHXValuesPluginList,
    IHXBuffer** ppIHXBufferPluginName, 
    IHXValues** ppIHXValuesPluginProperties
)
{
    if(!m_spIUnknownContext.IsValid())
    {
	return HXR_UNEXPECTED;
    }

    const char* pcharPropertyPath = NULL;
    UINT32 ulProperty_id;

    HX_RESULT HX_RESULTRes = pIHXValuesPluginList->GetFirstPropertyULONG32
    (
	pcharPropertyPath, 
	ulProperty_id
    );

    while(SUCCEEDED(HX_RESULTRes))
    {
	HX_RESULTRes = _GetPluginDataFromID
	(
	    pcharPropertyPath,
	    ulProperty_id,
	    "PluginID",
	    ppIHXBufferPluginName,
	    ppIHXValuesPluginProperties
	);

	if(FAILED(HX_RESULTRes))
	{
	    HX_RESULTRes = pIHXValuesPluginList->GetNextPropertyULONG32
	    (
		pcharPropertyPath, 
		ulProperty_id
	    );
	    HX_RELEASE(*ppIHXBufferPluginName);
	    HX_RELEASE(*ppIHXValuesPluginProperties);
	}
	else
	{
	    return HX_RESULTRes;
	}
    }

    return HXR_FAIL;
}

HX_RESULT 
CServerAuthenticator::_GetPluginDataFromID
(
    const char*	    pcharPropertyPath,
    UINT32	    ulProperty_id,
    const char*	    pcharRealPropertyNameLoc,
    IHXBuffer**    ppIHXBufferPluginName, 
    IHXValues**    ppIHXValuesPluginProperties
)
{
    DECLARE_SMART_POINTER(IHXRegistry)	spregRegistry;
    const char* pcharPropertyName = NULL;
    CHXString	sPluginIDPath;

    spregRegistry = m_spIUnknownContext;

    HXPropType ptCurrent = spregRegistry->GetTypeById
    (
	ulProperty_id
    );

    if(ptCurrent == PT_COMPOSITE)
    {
	sPluginIDPath = pcharPropertyPath;
	sPluginIDPath += ".";
	sPluginIDPath += pcharRealPropertyNameLoc;
	if 
	(
	    FAILED
	    (
		spregRegistry->GetStrByName
		(
		    sPluginIDPath, 
		    *ppIHXBufferPluginName
		)
	    )
	)
	{
	    pcharPropertyName = strrchr(pcharPropertyPath, '.');

	    if(!pcharPropertyName)
	    {
		pcharPropertyName = pcharPropertyPath;
	    }
	    else
	    {
		pcharPropertyName++;
	    }

	    CHXBuffer::FromCharArray
	    (
		pcharPropertyName, 
		ppIHXBufferPluginName
	    );
	}

	return spregRegistry->GetPropListById
	(
	    ulProperty_id, 
	    *ppIHXValuesPluginProperties
	);
    }

    return HXR_FAIL;
}

HX_RESULT 
CServerAuthenticator::_GetNextPlugin
(
    IHXValues* pIHXValuesPluginList,
    IHXBuffer** ppIHXBufferPluginName, 
    IHXValues** ppIHXValuesPluginProperties
)
{
    if(!m_spIUnknownContext.IsValid())
    {
	return HXR_UNEXPECTED;
    }

    const char* pcharPropertyPath;
    UINT32 ulProperty_id;

    HX_RESULT HX_RESULTRes = HXR_OK;

    while
    (
	SUCCEEDED
	(
	    HX_RESULTRes
	)
	&&
	SUCCEEDED
	(
	    pIHXValuesPluginList->GetNextPropertyULONG32
	    (
		pcharPropertyPath, 
		ulProperty_id
	    )
	)
    )
    {
	HX_RESULTRes = _GetPluginDataFromID
	(
	    pcharPropertyPath,
	    ulProperty_id,
	    "PluginID",
	    ppIHXBufferPluginName,
	    ppIHXValuesPluginProperties
	);

	if(SUCCEEDED(HX_RESULTRes))
	{
	    return HX_RESULTRes;
	}

	HX_RELEASE(*ppIHXBufferPluginName);
	HX_RELEASE(*ppIHXValuesPluginProperties);
    }

    return HXR_FAIL;
}

HX_RESULT 
CServerAuthenticator::_TryToLoadPlugins()
{
    if
    (
	m_spIUnknownContext.IsValid()
	&& 
	m_spIHXValuesConfig.IsValid()
    )
    {
	DECLARE_SMART_POINTER(IHXValues)	spIHXValuesPluginList;
	DECLARE_SMART_POINTER(IHXValues)	spIHXValuesPluginProperties;
	DECLARE_SMART_POINTER(IHXBuffer)	spIHXBufferPluginID;
	DECLARE_SMART_POINTER(IHXBuffer)	spIHXBufferRealm;
	DECLARE_SMART_POINTER_UNKNOWN		spIUnknownPlugin;

	if
	(
	    SUCCEEDED
	    (
		m_spIHXValuesConfig->GetPropertyCString
		(
		    "Realm",
		    spIHXBufferRealm.ptr_reference()
		)
	    )
	    &&
	    SUCCEEDED
	    (
		_GetRealmSettings
		(
		    (char*)spIHXBufferRealm->GetBuffer(), 
		    &spIHXValuesPluginList
		)
	    )
	)
	{

	    HX_RESULT HX_RESULTRet = _GetFirstPlugin
	    (
		spIHXValuesPluginList, 
		&spIHXBufferPluginID, 
		&spIHXValuesPluginProperties
	    );

	    while(SUCCEEDED(HX_RESULTRet))
	    {

//		cout << spIHXBufferPluginID->GetBuffer() << endl;

		spIUnknownPlugin.Release();

		_CreatePlugin
		(
		    spIHXBufferPluginID, 
		    spIHXValuesPluginProperties, 
		    &spIUnknownPlugin
		);

		if(spIUnknownPlugin.IsValid())
		{
		    // Insert at end!! Order matters!!
		    m_ListOfIUnknownPlugins.insert
		    (
			m_ListOfIUnknownPlugins.end(), 
			spIUnknownPlugin
		    );
		}

		spIHXValuesPluginProperties.Release();
		spIHXBufferPluginID.Release();

		HX_RESULTRet = _GetNextPlugin
		(
		    spIHXValuesPluginList, 
		    &spIHXBufferPluginID, 
		    &spIHXValuesPluginProperties
		);
	    }
	}
    }

    return HXR_OK;
}

HX_RESULT
CServerAuthenticator::_CreatePlugin
(
    IHXBuffer* pIHXBufferPluginID,
    IHXValues* pIHXValuesPluginProperties,
    IUnknown** ppIUnknownPlugin
)
{
    DECLARE_SMART_POINTER
    (
	IHXServerAuthConversation
    )					spIHXServerAuthConversationPlugin;
    DECLARE_SMART_POINTER
    (
	IHXObjectConfiguration
    )					spIHXObjectConfigurationPlugin;
    DECLARE_SMART_POINTER
    (
	IHXPluginEnumerator
    )					sppePlugins;
    DECLARE_SMART_POINTER(IHXValues)	spIHXValuesPluginOptions;
    DECLARE_SMART_POINTER(IHXValues)	spIHXValuesPluginValues;
    DECLARE_SMART_POINTER(IHXBuffer)	spIHXBufferPluginID;
    DECLARE_SMART_POINTER
    (
	IHXPluginProperties
    )					spIHXPluginPropertiesPlugin;
    UINT32				ulIndex;
    UINT32				ulNumPlugins;
    HX_RESULT				HX_RESULTRet = HXR_FAIL;

    sppePlugins = m_spIUnknownContext;

    if(sppePlugins.IsValid())
    {
	ulNumPlugins = sppePlugins->GetNumOfPlugins();

	// Load all auth plugins
	for(ulIndex = 0; ulIndex < ulNumPlugins; ++ulIndex)
	{
	    HX_RELEASE(*ppIUnknownPlugin);
	    sppePlugins->GetPlugin
	    (
		ulIndex, 
		(*ppIUnknownPlugin)
	    );

	    // Check to see if this is a ServerAuthenticator plugin
	    spIHXServerAuthConversationPlugin = (*ppIUnknownPlugin);
	    if(spIHXServerAuthConversationPlugin.IsValid())
	    {
		// Check to see if it has IHXObjectConfiguration
		spIHXObjectConfigurationPlugin = (*ppIUnknownPlugin);

		// Check to see if it has IHXPluginProperties
		spIHXPluginPropertiesPlugin = (*ppIUnknownPlugin);

		if
		(
		    spIHXPluginPropertiesPlugin.IsValid() 
		    && 
		    spIHXObjectConfigurationPlugin.IsValid()
		)
		{
		    // Initialize it
		    spIHXObjectConfigurationPlugin->SetContext
		    (
			m_spIUnknownContext
		    );

		    // Get the options from the plugin
		    spIHXValuesPluginOptions.Release();

		    spIHXPluginPropertiesPlugin->GetProperties
		    (
			spIHXValuesPluginOptions.ptr_reference()
		    );

		    spIHXBufferPluginID.Release();
		    spIHXValuesPluginOptions->GetPropertyCString
		    (
			"PluginID",
			spIHXBufferPluginID.ptr_reference()
		    );

		    // If this is the desired plugin
		    if
		    (
			spIHXBufferPluginID.IsValid()
			&&
			!strcasecmp
			(
			    (char*)spIHXBufferPluginID->GetBuffer(),
			    (char*)pIHXBufferPluginID->GetBuffer()
			)
		    )
		    {
			_ValuesFromHXRegList
			(
			    pIHXValuesPluginProperties,
			    &spIHXValuesPluginValues
			);
			spIHXObjectConfigurationPlugin->SetConfiguration
			(
			    spIHXValuesPluginValues
			);

			HX_RESULTRet = HXR_OK;

			break;
		    }
		}
	    }
	}

	if (FAILED(HX_RESULTRet))
	{
	    HX_RELEASE(*ppIUnknownPlugin);
	}
    }

    return HX_RESULTRet;
}

HX_RESULT
CServerAuthenticator::_ValuesFromHXRegList
(
    IHXValues* pIHXValuesPNRegList,
    IHXValues** ppIHXValuesValues
)
{
    DECLARE_SMART_POINTER(IHXBuffer) spIHXBufferRealm;
    DECLARE_SMART_POINTER(IHXRegistry)	    spregRegistry;
    IHXBuffer* pBuffer;
    const char* propSubName;
    const char* propName;
    UINT32 prop_id;
    INT32 val;
    HXPropType type;
    HX_RESULT res;

    spregRegistry = m_spIUnknownContext;

    res = m_pClassFactory->CreateInstance(CLSID_IHXValues, 
          (void**)ppIHXValuesValues);

    if (SUCCEEDED(res))
    {
        res = pIHXValuesPNRegList->GetFirstPropertyULONG32(propName, prop_id);
    }
    while(res == HXR_OK)
    {
	type = spregRegistry->GetTypeById(prop_id);
	propSubName = strrchr(propName, '.') + 1;

	switch(type)
	{
	    case PT_INTEGER:
	    {
		if(HXR_OK == spregRegistry->GetIntById(prop_id, val))
		{
		    (*ppIHXValuesValues)->SetPropertyULONG32(propSubName, val);
		}
		break;
	    }
	    case PT_STRING:
	    {
		if(HXR_OK == spregRegistry->GetStrById(prop_id, pBuffer))
		{
		    (*ppIHXValuesValues)->SetPropertyCString(propSubName,
			pBuffer);
		    HX_RELEASE(pBuffer);
		}
		break;
	    }
	    case PT_BUFFER:
	    {
		if(HXR_OK == spregRegistry->GetBufById(prop_id, pBuffer))
		{
		    (*ppIHXValuesValues)->SetPropertyBuffer(propSubName,
			pBuffer);
		    HX_RELEASE(pBuffer);
		}
		break;
	    }
	    default:
		break;
	}
	res = pIHXValuesPNRegList->GetNextPropertyULONG32(propName, prop_id);
    }

    // Pass on the realm
    if(SUCCEEDED(m_spIHXValuesConfig->GetPropertyCString("Realm",
		spIHXBufferRealm.ptr_reference())))
    {
	(*ppIHXValuesValues)->SetPropertyCString("Realm", spIHXBufferRealm);
    }

    return HXR_OK;
}

/* XXXkshoop version for new plugin handler!

HX_RESULT
CServerAuthenticator::_CreatePlugin
(
    IHXBuffer* pIHXBufferPluginID,
    IHXValues* pIHXValuesPluginProperties,
    IUnknown** ppIUnknownPlugin
)
{
    DECLARE_SMART_POINTER
    (
	IHXServerAuthConversation
    )					spIHXServerAuthConversationPlugin;
    DECLARE_SMART_POINTER
    (
	IHXObjectConfiguration
    )					spIHXObjectConfigurationPlugin;
    DECLARE_SMART_POINTER(IHXValues)	spIHXValuesPluginValues;
    IHXPlugin2Handler*			pIHXPlugin2HandlerSource = NULL;
    HX_RESULT				HX_RESULTRet = HXR_FAIL;

    m_spIUnknownContext->QueryInterface
    (
	IID_IHXPlugin2Handler,
	(void**)&pIHXPlugin2HandlerSource
    );

    if(pIHXPlugin2HandlerSource)
    {
	pIHXPlugin2HandlerSource->FindPluginUsingStrings
	(
	    "PluginID", 
	    (char*)pIHXBufferPluginID->GetBuffer(), 
	    NULL, 
	    NULL, 
	    NULL, 
	    NULL, 
	    (*ppIUnknownPlugin)
	);

	// Double Check to make sure this is a ServerAuthenticator plugin
	spIHXServerAuthConversationPlugin = (*ppIUnknownPlugin);
	if(spIHXServerAuthConversationPlugin.IsValid())
	{
	    spIHXObjectConfigurationPlugin = (*ppIUnknownPlugin);

	    // Initialize it
	    _ValuesFromHXRegList
	    (
		pIHXValuesPluginProperties,
		&spIHXValuesPluginValues
	    );

	    spIHXObjectConfigurationPlugin->SetConfiguration
	    (
		spIHXValuesPluginValues
	    );

	    HX_RESULTRet = HXR_OK;
	}
	else
	{
	    HX_RELEASE(*ppIUnknownPlugin);
	}
    }

    HX_RELEASE(pIHXPlugin2HandlerSource);

    return HX_RESULTRet;
}
*/
