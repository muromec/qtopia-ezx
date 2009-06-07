/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rn5auth.cpp, 2004/27/07 
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

//  $Id: rn5auth.cpp,v 1.11 2006/12/03 23:05:38 ehyche Exp $

// Significant rewrite was done 12/29/99 by SSH.
 
#include <stdio.h>

#include "hxtypes.h"

#include "rn5auth.ver"

#define INITGUID

#include "hxstrutl.h"
#include "hxcom.h"
#include "hxplugn.h"
#include "hxprefs.h"
#include "hxfiles.h"
#include "ihxpckts.h"
#include "ihxfgbuf.h"
#include "hxauthn.h"
#include "hxplgns.h"
#include "hxcomm.h"
#include "hxdb.h"
#include "hxengin.h"
#include "pckunpck.h"
#include "md5.h"

#undef INITGUID

#include "hxbuffer.h"
#include "chxpckts.h"
#include "hxmangle.h"

#include "hashauthbase.h"
#include "rn5auth.h"

#include "hxver.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _AIX
#include "hxtbuf.h"
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Rn5Auth);
#endif

const char* CRN5Authenticator::zm_pDescription = "RealNetworks RN5 Authenticator";
const char* CRN5Authenticator::zm_pCopyright	 = HXVER_COPYRIGHT;
const char* CRN5Authenticator::zm_pMoreInfoURL = HXVER_MOREINFO;

CRN5Authenticator::CRN5Authenticator()
    : m_pClientRequest(NULL)
    , m_pClientRespondee(NULL)
    , m_bFinished(FALSE)
{
}

CRN5Authenticator::~CRN5Authenticator()
{
    HX_RELEASE(m_pClientRequest);
    HX_RELEASE(m_pClientRespondee);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP CRN5Authenticator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin))
    {
        AddRef();
        *ppvObj = (IHXPlugin*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPluginProperties))
    {
        AddRef();
        *ppvObj = (IHXPluginProperties*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXClientAuthConversation))
    {
        AddRef();
        *ppvObj = (IHXClientAuthConversation*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCredRequestResponse))
    {
        AddRef();
        *ppvObj = (IHXCredRequestResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerAuthConversation))
    {
        AddRef();
        *ppvObj = (IHXServerAuthConversation*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAuthenticationDBAccessResponse))
    {
        AddRef();
        *ppvObj = (IHXAuthenticationDBAccessResponse*)this;
        return HXR_OK;
    }
    
    if (HXR_OK == CHashAuthenticatorBase::QueryInterface(riid, ppvObj))
    {
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;        
}

STDMETHODIMP_(UINT32) CRN5Authenticator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32) CRN5Authenticator::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP 
CRN5Authenticator::InitPlugin(IUnknown* /*IN*/ pContext)
{
    m_pContext = pContext;

    if (m_pContext)
    {
	m_pContext->AddRef();
    }


    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the RN5 information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP 
CRN5Authenticator::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

// IHXCredRequestResponse
STDMETHODIMP
CRN5Authenticator::CredentialsReady
(
    HX_RESULT	Status,
    IHXValues* pCredentials
)
{
    HX_RESULT Ret = HXR_FAIL;

    if(!m_pClientRespondee)
    {
	return HXR_UNEXPECTED;
    }

    if(!m_pRequestContext)
    {
	m_pClientRespondee->ResponseReady(HXR_UNEXPECTED, NULL);
	HX_RELEASE(m_pClientRespondee);
	return HXR_UNEXPECTED;
    }

    if(FAILED(Status))
    {
	m_pClientRespondee->ResponseReady(Status, m_pClientRequest);
	HX_RELEASE(m_pClientRespondee);
	return Status;
    }

    if(pCredentials)
    {
	IHXValues* pResponseHeaders = NULL;
	IHXBuffer* pMunge = NULL;

	Ret = _MungeUserRealmPassFromValues(pCredentials, &pMunge);

	if(SUCCEEDED(Ret))
	{
	    Ret = _CreateQuotedHeader(pCredentials, pMunge, &pResponseHeaders);
	}

	m_pClientRequest->SetRequestHeaders(pResponseHeaders);
	m_bFinished = TRUE;
	m_pClientRespondee->ResponseReady(HXR_OK, m_pClientRequest);

	HX_RELEASE(pMunge);
	HX_RELEASE(pResponseHeaders);
    }
    else
    {
	m_pClientRespondee->ResponseReady(HXR_FAIL, m_pClientRequest);
    }

    HX_RELEASE(m_pClientRespondee);

    return Ret;
}

// IHXClientAuthConversation
STDMETHODIMP 
CRN5Authenticator::MakeResponse
(
    IHXClientAuthResponse* pClientRespondee,
    IHXRequest*            pClientRequest
)
{
    if(!pClientRespondee || !pClientRequest)
    {
	return HXR_UNEXPECTED;
    }
    
    m_pClientRequest = pClientRequest;
    m_pClientRequest->AddRef();
    m_pClientRequest->QueryInterface(IID_IHXRequestContext,
	(void **)&m_pRequestContext);

    m_pClientRespondee = pClientRespondee;
    m_pClientRespondee->AddRef();

    IHXValues* pChallengeHeaders = NULL;

    m_pClientRequest->GetResponseHeaders(pChallengeHeaders);

    if(!pChallengeHeaders)
    {
	m_pClientRespondee->ResponseReady(HXR_UNEXPECTED, pClientRequest);
	HX_RELEASE(m_pClientRespondee);
	return HXR_UNEXPECTED;
    }

    HX_RESULT Ret = HXR_FAIL;

    IHXBuffer* pChallengeBuf = NULL;

    UINT32 ulStatusCode = 0;
    pChallengeHeaders->GetPropertyULONG32("_statuscode", ulStatusCode);

    // be sure that this IHXValues value has been set.
    // This is how we distinguish between a 401 response status code
    // (in which case we watch for a WWW-Authenticate header) and a
    // 407 response status code (in which case we watch for a Proxy-
    // Authenticate header).
    HX_ASSERT((ulStatusCode == 401) || (ulStatusCode == 407));

    // it is possible to have multiple WWW-Authenticate headers
    // (or Proxy-Authenticate headers) in a single response, so
    // we must iterate over all headers to see if we can handle
    // any of the authentication schemes.

    HXBOOL bFoundCompatibleAuthenticationScheme = FALSE;
    const char* pPropertyName;

    HX_RESULT res = pChallengeHeaders->GetFirstPropertyCString(pPropertyName, pChallengeBuf);

    while (SUCCEEDED(res))
    {
        HXBOOL bIsAuthenticationHeader = FALSE;

        if ((ulStatusCode == 401 || ulStatusCode == 0) &&
            (strncasecmp(pPropertyName, "WWW-Authenticate", 16) == 0))
        {
            bIsAuthenticationHeader = TRUE;
            m_bIsProxyAuthentication = FALSE;
        }
        else if ((ulStatusCode == 407 || ulStatusCode == 0) &&
            (strncasecmp(pPropertyName, "Proxy-Authenticate", 18) == 0))
        {
            bIsAuthenticationHeader = TRUE;
            m_bIsProxyAuthentication = TRUE;
        }

        if (bIsAuthenticationHeader && pChallengeBuf)
        {
            const char* pszChallenge = (const char*)pChallengeBuf->GetBuffer();

            if (strncasecmp(pszChallenge, "RN5", 3) == 0)
            {
                bFoundCompatibleAuthenticationScheme = TRUE;

                IHXCredRequest* pCredRequest = NULL;
                IHXValues* pCredentials = NULL;

                _DescribeCredentials(pChallengeHeaders, &pCredentials);

                m_pClientRespondee->QueryInterface(IID_IHXCredRequest, (void**)&pCredRequest);

                Ret = pCredRequest->GetCredentials(this, pCredentials);

                // flow continues in CredentialsReady()

                HX_RELEASE(pCredRequest);
                HX_RELEASE(pCredentials);

                goto cleanup;
            }
        }
        HX_RELEASE(pChallengeBuf);

        res = pChallengeHeaders->GetNextPropertyCString(pPropertyName, pChallengeBuf);
    }

    if (!bFoundCompatibleAuthenticationScheme)
    {
        // let the respondee know that we did now find a compatible authentication scheme

        m_pClientRespondee->ResponseReady(HXR_FAIL, NULL);
        HX_RELEASE(m_pClientRespondee);
    }

cleanup:
    HX_RELEASE(pChallengeHeaders);
    HX_RELEASE(pChallengeBuf);
    return Ret;
}

HXBOOL 
CRN5Authenticator::IsDone()
{
    return m_bFinished;
}

STDMETHODIMP
CRN5Authenticator::Authenticated(HXBOOL bAuthenticated)
{
    return HXR_OK;
}

// IHXPluginProperties
STDMETHODIMP
CRN5Authenticator::GetProperties(REF(IHXValues*) pOptions)
{
    if (HXR_OK == CreateValuesCCF(pOptions, m_pContext))
    {
	_SetPropertyFromCharArray(pOptions, PLUGIN_CLASS, PLUGIN_CLASS_AUTH_TYPE);
	_SetPropertyFromCharArray(pOptions, "PluginID", "rn-auth-rn5");
	_SetPropertyFromCharArray(pOptions, "AuthenticationProtocolID", "RN5");
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
    return HXR_OK;
}

HX_RESULT 
CRN5Authenticator::_DescribeCredentials
(
    IHXValues*  pChallengeHeaders,
    IHXValues** ppParms   
)
{
    HX_RESULT Ret = _ChallengeToCredentials(pChallengeHeaders, ppParms);

    if (SUCCEEDED(Ret) && (*ppParms))
    {
	// XXXSSH - does this msg ever actually shown anywhere??
	_SetPropertyFromCharArray(*ppParms, "Prompt",
	    "The Realm %Realm% has indicated that %URI% is secure \
	    content. Please fill out the credentials requested below \
	    to gain access. ");
	_SetPropertyFromCharArray(*ppParms, "User", "?");
	_SetPropertyFromCharArray(*ppParms, "Password", "?*");

	// Now fill in the rest of pChallengeHeaders' stuff, so some
	// IHXAuthenticationManager2 implementor has the luxury of
	// picking and choosing what's appropriate.
	
	IHXBuffer* pBuffer = NULL;
	const char * pName;
	
	HX_RESULT res = pChallengeHeaders->GetFirstPropertyCString(pName, pBuffer);
	while (res == HXR_OK)
	{
	    (*ppParms)->SetPropertyCString(pName, pBuffer);
	    
	    pBuffer->Release();
	    res = pChallengeHeaders->GetNextPropertyCString(pName, pBuffer);
	}
    }

    return Ret;
}

// IHXServerAuthConversation
STDMETHODIMP 
CRN5Authenticator::MakeChallenge
(
    IHXServerAuthResponse* pServerRespondee,
    IHXRequest*            pServerRequest
)
{
    if(!pServerRespondee || !pServerRequest)
    {
	HX_ASSERT(0);
	return HXR_UNEXPECTED;
    }

    m_pServerRequest = pServerRequest;
    m_pServerRequest->AddRef();
    m_pServerRequest->QueryInterface(IID_IHXRequestContext,
	(void **)&m_pRequestContext);

    m_pServerRespondee = pServerRespondee;
    m_pServerRespondee->AddRef();

    if(!m_pRequestContext)
    {
	m_pServerRespondee->ChallengeReady(HXR_UNEXPECTED, pServerRequest);
	HX_ASSERT(0);
	HX_RELEASE(m_pServerRequest);
	HX_RELEASE(m_pServerRespondee);
	return HXR_UNEXPECTED;
    }

    IHXValues* pResponseHeaders = NULL;

    pServerRequest->GetRequestHeaders(pResponseHeaders);

    if(!pResponseHeaders)
    {
	m_pServerRespondee->ChallengeReady(HXR_UNEXPECTED, pServerRequest);
	HX_ASSERT(0);
	HX_RELEASE(m_pServerRequest);
	HX_RELEASE(m_pRequestContext);
	HX_RELEASE(m_pServerRespondee);
	return HXR_UNEXPECTED;
    }

    IHXBuffer* pAuthBuf = NULL;

    pResponseHeaders->GetPropertyCString("Authorization", pAuthBuf);

    HX_RELEASE(pResponseHeaders);

    if (pAuthBuf && !strncasecmp((char*)pAuthBuf->GetBuffer(), "RN5", 3))
    {
	HX_RELEASE(m_pCredentials);
	HX_RELEASE(m_pPrincipalID);
	_HeaderToCredentials(pAuthBuf, &m_pCredentials);
	m_pCredentials->GetPropertyCString("Username", m_pPrincipalID);

	HX_RESULT Ret = HXR_OK;

	if (m_pAuthDBAccess)
	{
	    Ret = m_pAuthDBAccess->GetCredentials(this, m_pPrincipalID);
	    // Flow continues in GetCredentialsDone()
	}

	HX_RELEASE(pAuthBuf);
	return Ret;
    }
    else
    {
	// no Authorization header yet, we have to challenge for it
	HX_RELEASE(pAuthBuf); // just in case
        return _SendChallengeResponse();
    }
}

HX_RESULT
CRN5Authenticator::_SendChallengeResponse()
{
    if (!m_bAuthenticated && m_pRealm)
    {
	CHXString str;
	str = "RN5 realm=\"";
	str += m_pRealm->GetBuffer();
	str += "\"";
	str += ", nonce=\"";

	IHXScheduler* pSchedulerContext = NULL;
	m_pContext->QueryInterface(IID_IHXScheduler, (void**)&pSchedulerContext);
	if (pSchedulerContext)
	{
	    HXTimeval TimeNow;
	    TimeNow = pSchedulerContext->GetCurrentSchedulerTime();
	    str.AppendULONG(TimeNow.tv_sec);
	    str.AppendULONG(TimeNow.tv_usec);
	}
	else
	{
	    HX_ASSERT(0);  // Crappy Nonce?? I'm just porting it as I see it. -- SSH
	    str += "Crappy_Nonce";
	}

	str += "\"";
 
	IHXValues* pChallengeHeaders = _GetResponseHeaders();
	if (!pChallengeHeaders)
	{
	    HX_ASSERT(0);
	    return HXR_UNEXPECTED;
	}
	else
	{
	    if (m_bIsProxyAuthentication)
	    {
		_SetPropertyFromCharArray(pChallengeHeaders, "Proxy-Authenticate", 
			(const char*) str);
	    }
	    else
	    {
		_SetPropertyFromCharArray(pChallengeHeaders, "WWW-Authenticate", 
			(const char*) str);
	    }
	    HX_RELEASE(pChallengeHeaders);
	}

	HX_RELEASE(pSchedulerContext);
    }

    m_pServerRespondee->ChallengeReady(HXR_OK, m_pServerRequest);

    HX_RELEASE(m_pServerRequest);
    HX_RELEASE(m_pRequestContext);
    HX_RELEASE(m_pServerRespondee);

    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) 
CRN5Authenticator::IsAuthenticated()
{
    return m_bAuthenticated;
}

STDMETHODIMP 
CRN5Authenticator::GetUserContext(REF(IUnknown*) pIUnknownUser)
{
    pIUnknownUser = (IUnknown*)(IHXObjectConfiguration*)(CHashAuthenticatorBase*)this;
    pIUnknownUser->AddRef();

    return HXR_OK;
}

// IHXAuthenticationDBAccessResponse
STDMETHODIMP
CRN5Authenticator::ExistenceCheckDone
(
    HX_RESULT		ResultStatus,
    IHXBuffer*		pBufferPrincipalID
)
{
    // XXXSSH - why not implemented?
    return HXR_NOTIMPL;
}

STDMETHODIMP
CRN5Authenticator::GetCredentialsDone
(
    HX_RESULT		ResultStatus,
    IHXBuffer*		pBufferPrincipalID,
    IHXBuffer*		pBufferCredentials
)
{
    m_bAuthenticated = FALSE; // till we know better

    if (SUCCEEDED(ResultStatus)	&& pBufferCredentials)
    {
	IHXBuffer* pServerToken = NULL;
	IHXBuffer* pClientToken = NULL;

	// We run MD5 against password, nonce, etc. on server side, should
	// get same result as what client sent us.
	_StorageToToken(m_pCredentials, pBufferCredentials, &pServerToken);
	m_pCredentials->GetPropertyCString("Response", pClientToken);

	if (pClientToken && pServerToken &&
	    !strcasecmp((char*) pServerToken->GetBuffer(), 
			(char*) pClientToken->GetBuffer()))
	{
	    m_bAuthenticated = TRUE;
	    m_pRequestContext->SetUserContext((IUnknown*)(IHXObjectConfiguration*)(CHashAuthenticatorBase*)this);  
	}
	HX_RELEASE(pClientToken);
	HX_RELEASE(pServerToken);
    }

    if (!m_bAuthenticated)
    {
	HX_RELEASE(m_pPrincipalID);  
    }

    _SendChallengeResponse();

    return HXR_OK;
}

HX_RESULT
CRN5Authenticator::_MungeUserRealmPassFromValues
(
    IHXValues* pCredentials,
    IHXBuffer** ppStorageKey
)
{
    IHXBuffer*  pUserName = NULL;
    IHXBuffer*  pRealm = NULL;
    IHXBuffer*  pPassword = NULL;
    HX_RESULT    Ret = HXR_OK;  

    if (SUCCEEDED(pCredentials->GetPropertyCString("UserName", pUserName)) &&
	SUCCEEDED(pCredentials->GetPropertyCString("Realm", pRealm)) &&
	SUCCEEDED(pCredentials->GetPropertyCString("Password", pPassword)))
    {
	_MungeUserRealmPass(pUserName, pRealm, pPassword, ppStorageKey);
    }
    else
    {
	Ret = HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(pUserName);
    HX_RELEASE(pRealm);
    HX_RELEASE(pPassword);

    return Ret;
}


HX_RESULT
CRN5Authenticator::_StorageToToken
(
    IHXValues* pCredentials,
    IHXBuffer* pStoredPassword,
    IHXBuffer** ppToken
)
{
    HX_RESULT Ret = HXR_FAIL;
    IHXBuffer* pNonce = NULL;
    IHXBuffer* pGUID  = NULL;

    *ppToken = NULL;

    if (pStoredPassword &&
	SUCCEEDED(pCredentials->GetPropertyCString("GUID", pGUID)) &&
	SUCCEEDED(pCredentials->GetPropertyCString("Nonce", pNonce)))
    {
	char resbuf[1024]; /* Flawfinder: ignore */

	sprintf(resbuf, /* Flawfinder: ignore */
	    "%-.200s%-.200s%-.200sCopyright (C) 1995,1996,1997 RealNetworks, Inc.",
	    (char*)pStoredPassword->GetBuffer(), 
	    (char*)pNonce->GetBuffer(),
	    (char*)pGUID->GetBuffer());

	Ret = CreateBufferCCF(*ppToken, m_pContext);
	if (HXR_OK == Ret)
	{
	    (*ppToken)->SetSize(64);
	    char* sToken = (char*)(*ppToken)->GetBuffer();
	    MD5Data(sToken, (const UCHAR*)resbuf, strlen(resbuf));
	}
    }

    HX_RELEASE(pNonce);
    HX_RELEASE(pGUID);
    return Ret;
}

HX_RESULT
CRN5Authenticator::_CreateQuotedHeader
(
    IHXValues*  pCredentials,
    IHXBuffer*  pStoredPassword,
    IHXValues** ppResponseHeaders
)
{
    IHXBuffer* pUser = NULL;
    IHXBuffer* pNonce = NULL;
    IHXBuffer* pGUID = NULL;
    IHXBuffer* pRealm = NULL;
    IHXBuffer* pToken = NULL;
    HX_RESULT   Ret = HXR_FAIL;

    *ppResponseHeaders = NULL;

    if (SUCCEEDED(_StorageToToken(pCredentials, pStoredPassword, &pToken)) &&
	SUCCEEDED(pCredentials->GetPropertyCString("GUID", pGUID)) &&
	SUCCEEDED(pCredentials->GetPropertyCString("Nonce", pNonce)))

    {
	IHXBuffer* pHeader = NULL;
	
	if (HXR_OK == CreateBufferCCF(pHeader, m_pContext))
	{
	    pHeader->SetSize(1024);

	    char* sHeader = (char*) pHeader->GetBuffer();

	    if (HXR_OK == CreateValuesCCF(*ppResponseHeaders, m_pContext))
	    {
		INT32 lBytes = SafeSprintf(sHeader, 1024,"RN5 ");

		if (SUCCEEDED(pCredentials->GetPropertyCString("UserName", pUser)))
		{
		    lBytes += SafeSprintf(sHeader+lBytes,1024-lBytes,"username=\"%-.200s\",", pUser->GetBuffer());
		}

		lBytes += SafeSprintf(sHeader+lBytes,1024-lBytes, " GUID=\"%s\",", pGUID->GetBuffer());

		if (SUCCEEDED(pCredentials->GetPropertyCString("Realm", pRealm)))
		{
		    lBytes += SafeSprintf(sHeader+lBytes,1024-lBytes, "realm=\"%-.200s\",", pRealm->GetBuffer());
		}

		lBytes += SafeSprintf(sHeader+lBytes,1024-lBytes, "nonce=\"%s\",", pNonce->GetBuffer());

		lBytes += SafeSprintf(sHeader+lBytes,1024-lBytes, "response=\"%-.200s\"", pToken->GetBuffer());

		if (m_bIsProxyAuthentication)
		{
		    Ret = (*ppResponseHeaders)->SetPropertyCString("Proxy-Authorization", pHeader);
		}
		else
		{
		    Ret = (*ppResponseHeaders)->SetPropertyCString("Authorization", pHeader);
		}
	    }
        	
	    HX_RELEASE(pHeader);
	}
    }

    HX_RELEASE(pUser);
    HX_RELEASE(pNonce);
    HX_RELEASE(pGUID);
    HX_RELEASE(pRealm);
    HX_RELEASE(pToken);

    return Ret;
}

HX_RESULT
CRN5Authenticator::_ChallengeToCredentials
(
    IHXValues* pChallengeHeaders, 
    IHXValues** ppCredentials
)
{
    IHXBuffer* pChallengeBuf = NULL;

    const char* pPropertyName;

    HX_RESULT res = pChallengeHeaders->GetFirstPropertyCString(pPropertyName, pChallengeBuf);

    while (SUCCEEDED(res))
    {
        HXBOOL bIsAuthenticationHeader = FALSE;
        if (!m_bIsProxyAuthentication &&
            (strncasecmp(pPropertyName, "WWW-Authenticate", 16) == 0))
        {
            bIsAuthenticationHeader = TRUE;
        }
        else if (m_bIsProxyAuthentication &&
            (strncasecmp(pPropertyName, "Proxy-Authenticate", 18) == 0))
        {
            bIsAuthenticationHeader = TRUE;
        }

        if (bIsAuthenticationHeader && pChallengeBuf)
        {
            const char* pszChallenge = (const char*)pChallengeBuf->GetBuffer();

            if (strncasecmp(pszChallenge, "RN5", 3) == 0)
            {
		res = _HeaderToCredentials(pChallengeBuf, ppCredentials);
		HX_RELEASE(pChallengeBuf);
		return res;
            }
        }
        HX_RELEASE(pChallengeBuf);

        res = pChallengeHeaders->GetNextPropertyCString(pPropertyName, pChallengeBuf);
    }

    HX_RELEASE(pChallengeBuf);
    return HXR_FAIL;
}

HX_RESULT
CRN5Authenticator::_HeaderToCredentials
(
    IHXBuffer* pHeader,
    IHXValues** ppCredentials
)
{
    HX_ASSERT(pHeader);
    HX_RESULT retVal = HXR_FAIL;

    char* sChallenge = NULL;

    if (pHeader)
    {
	sChallenge = (char*) pHeader->GetBuffer();
	if (sChallenge)
	{
	    retVal = HXR_OK;
	}
    }

    if(SUCCEEDED(retVal) && strncasecmp(sChallenge, "RN5", 3) == 0)
    {
	retVal = CreateValuesCCF(*ppCredentials, m_pContext);
	if (HXR_OK == retVal)
	{
	    IHXBuffer* pCipheredGUID = NULL;
	    char* sGUID = NULL;

	    if (m_pPreferencesCore &&
		m_pPreferencesCore->ReadPref(CLIENT_GUID_REGNAME, pCipheredGUID) == HXR_OK)
	    {
		sGUID = DeCipher((char*)pCipheredGUID->GetBuffer());

		_SetPropertyFromCharArray(*ppCredentials, "GUID", sGUID);
	    }
	    else
	    {
		_SetPropertyFromCharArray(*ppCredentials, "GUID", "GUIDLESS_CLIENT");
	    }

	    HX_RELEASE(pCipheredGUID);
	    HX_VECTOR_DELETE(sGUID);

	    sChallenge += 3;

	    _GetQuotedFields(sChallenge, *ppCredentials);
	    retVal = HXR_OK;    
	}
    }

    return retVal;
}

HX_RESULT STDAPICALLTYPE CRN5Authenticator::HXCreateInstance
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CRN5Authenticator();
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        return HXR_OK;
    }
    return HXR_OUTOFMEMORY;    
}

