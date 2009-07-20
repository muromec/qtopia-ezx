/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: digestauth.cpp, 2004/27/07 
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

#include "hxcom.h"
#include "hxplugn.h"
#include "hxplgns.h"
#include "hxfiles.h"
#include "hxauthn.h"
#include "hxdb.h"
#include "pckunpck.h"
#include "hxengin.h"

#include "md5.h"

#include "chxpckts.h"
#include "hashauthbase.h"
#include "digestauth.h"

#include "hxver.h"
#include "rn5auth.ver"

const char* CDigestAuthenticator::zm_pDescription = "RealNetworks Digest Authenticator";
const char* CDigestAuthenticator::zm_pCopyright	 = HXVER_COPYRIGHT;
const char* CDigestAuthenticator::zm_pMoreInfoURL = HXVER_MOREINFO;

HX_RESULT STDAPICALLTYPE CDigestAuthenticator::HXCreateInstance(IUnknown**  /*OUT*/	ppIUnknown)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CDigestAuthenticator();
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        return HXR_OK;
    }
    return HXR_OUTOFMEMORY;    
}

CDigestAuthenticator::CDigestAuthenticator():m_algorithm(ALGO_UNKNOWN)
{
}

CDigestAuthenticator::~CDigestAuthenticator()
{
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP CDigestAuthenticator::QueryInterface(REFIID riid, void** ppvObj)
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

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CDigestAuthenticator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CDigestAuthenticator::Release()
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
STDMETHODIMP CDigestAuthenticator::InitPlugin(IUnknown* /*IN*/ pContext)
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
CDigestAuthenticator::GetPluginInfo
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

// IHXPluginProperties
STDMETHODIMP
CDigestAuthenticator::GetProperties(REF(IHXValues*) pOptions)
{
    if (HXR_OK == CreateValuesCCF(pOptions, m_pContext))
    {
	_SetPropertyFromCharArray(pOptions, PLUGIN_CLASS, PLUGIN_CLASS_AUTH_TYPE);
	_SetPropertyFromCharArray(pOptions, "PluginID", "rn-auth-digest");
	_SetPropertyFromCharArray(pOptions, "AuthenticationProtocolID", "Digest");
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
    return HXR_OK;
}

// IHXServerAuthConversation
STDMETHODIMP 
CDigestAuthenticator::MakeChallenge(IHXServerAuthResponse* pServerRespondee,
    IHXRequest*            pServerRequest)
{
    if(!pServerRespondee || !pServerRequest)
    {
	    HX_ASSERT(0);
	    return HXR_UNEXPECTED;
    }

    HX_RELEASE(m_pServerRequest);
    HX_RELEASE(m_pRequestContext);

    m_pServerRequest = pServerRequest;
    m_pServerRequest->AddRef();
    m_pServerRequest->QueryInterface(IID_IHXRequestContext, (void **)&m_pRequestContext);

    HX_RELEASE(m_pServerRespondee);
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

    if (pAuthBuf && !strncasecmp((char*)pAuthBuf->GetBuffer(), "Digest", 6))
    {
	    HX_RELEASE(m_pCredentials);
	    HX_RELEASE(m_pPrincipalID);
	    ParseCredentials(pAuthBuf, &m_pCredentials);
	    m_pCredentials->GetPropertyCString("username", m_pPrincipalID);

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
        return SendChallengeResponse();
    }
}

STDMETHODIMP CDigestAuthenticator::GetUserContext(REF(IUnknown*) pIUnknownUser)
{
    pIUnknownUser = (IUnknown*)(IHXPlugin*)this;
    pIUnknownUser->AddRef();

    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) CDigestAuthenticator::IsAuthenticated()
{
    return m_bAuthenticated;
}

// IHXAuthenticationDBAccessResponse
STDMETHODIMP CDigestAuthenticator::ExistenceCheckDone(HX_RESULT hr,
    IHXBuffer*		pBufPrincipalID)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CDigestAuthenticator::GetCredentialsDone(HX_RESULT hr,
    IHXBuffer* pBufferPrincipalID,IHXBuffer* pBufferCredentials)
{
    m_bAuthenticated = FALSE; // till we know better

    if (SUCCEEDED(hr) && pBufferCredentials)
    {
	    IHXBuffer* pServerToken = NULL;
	    IHXBuffer* pClientToken = NULL;

	    // We run MD5 against password, nonce, etc. on server side, should
	    // get same result as what client sent us.
	    if (FAILED(ComputeServerToken(m_pCredentials, pBufferCredentials, &pServerToken)))
            m_pServerRespondee->ChallengeReady(HXR_UNEXPECTED, m_pServerRequest);

	    m_pCredentials->GetPropertyCString("Response", pClientToken);

	    if (pClientToken && pServerToken &&
	        !strcasecmp((char*) pServerToken->GetBuffer(), 
			    (char*) pClientToken->GetBuffer()))
	    {
	        m_bAuthenticated = TRUE;
	        m_pRequestContext->SetUserContext((IUnknown*)(IHXPlugin*)this);  
	    }

	    HX_RELEASE(pClientToken);
	    HX_RELEASE(pServerToken);
    }

    if (!m_bAuthenticated)
    {
	HX_RELEASE(m_pPrincipalID);  
    }

    SendChallengeResponse();

    return HXR_OK;
}

HX_RESULT
CDigestAuthenticator::SendChallengeResponse()
{
    if (!m_bAuthenticated && m_pRealm)
    {
	    CHXString str;
	    str = "Digest qop=\"auth\", algorith=MD5-sess, realm=\"";
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
	        HX_ASSERT(0);  
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

HX_RESULT CDigestAuthenticator::ParseCredentials(IHXBuffer* pAuthHeader, 
                                                 IHXValues** ppCredentials)
{
    char* szAuthHdr = (char*)pAuthHeader->GetBuffer();

    szAuthHdr += 6;   // Move past "Digest"

    if (HXR_OK == CreateValuesCCF(*ppCredentials, m_pContext))
    {
	return _GetQuotedFields(szAuthHdr, *ppCredentials);    
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
}

HX_RESULT CDigestAuthenticator::ComputeServerToken(IHXValues* pCredentials, 
                               IHXBuffer* pbufPassword, IHXBuffer** ppToken)
{
    HX_RESULT hr = HXR_OK;

    IHXValues* pResponseHeaders = NULL;
    IHXBuffer* pbufHdr = NULL;
    IHXBuffer* pbufnonceval = NULL;
    IHXBuffer* pbufcnonceval = NULL;
    IHXBuffer* pbufurival = NULL;
    IHXBuffer* pbufnccount = NULL;
    IHXBuffer* pbufA1 = NULL;
    IHXBuffer* pbufA2 = NULL;
    char szresbuf[1024];
    char* sToken = NULL;
    HXBOOL bQoPPresent = FALSE;

    // Calculate response

    //Calculate A1
    if (SUCCEEDED(pCredentials->GetPropertyCString("algorithm", pbufHdr)))
    {
        if (!strncasecmp((const char*)pbufHdr->GetBuffer(), "MD5-sess", 8))
            m_algorithm = MD5_SESS;
        else
            m_algorithm = MD5;       
        HX_RELEASE(pbufHdr);
    }
    else
    {
        hr = HXR_FAIL;
        goto errorfound;
    }

    if (FAILED(pCredentials->GetPropertyCString("nonce", pbufnonceval)))
    {
        hr = HXR_FAIL;
        goto errorfound;
    }

    if (FAILED(pCredentials->GetPropertyCString("cnonce", pbufcnonceval)))
    {
        hr = HXR_FAIL;
        goto errorfound;
    }
    
    switch (m_algorithm)
    {
    case MD5:
        pbufA1 = pbufPassword;
        pbufA1->AddRef();
        break;

    case MD5_SESS:
        sprintf(szresbuf, "%-.200s:%-.200s:%-.200s",pbufPassword->GetBuffer(), 
            pbufnonceval->GetBuffer(), pbufcnonceval->GetBuffer());

	if (HXR_OK == CreateBufferCCF(pbufA1, m_pContext))
	{
	    pbufA1->SetSize(64);
	    sToken = (char*)pbufA1->GetBuffer();
     
	    MD5Data(sToken, (const UCHAR*)szresbuf, strlen(szresbuf));    
	}
        break;

    case ALGO_UNKNOWN:
        hr = HXR_FAIL;
        goto errorfound;
    }
    
    m_pServerRequest->GetRequestHeaders(pResponseHeaders);

    if (FAILED(pResponseHeaders->GetPropertyCString("Method", pbufHdr)))
    {
	if (HXR_OK == CreateBufferCCF(pbufHdr, m_pContext))
	{
	    pbufHdr->SetSize(128);
	    strcpy((char*)pbufHdr->GetBuffer(), "POST");
	}
    }

    if (FAILED(pCredentials->GetPropertyCString("uri", pbufurival)))
    {
        hr = HXR_FAIL;
        goto errorfound;    
    }

    if (FAILED(pCredentials->GetPropertyCString("nc", pbufnccount)))
    {
        hr = HXR_FAIL;
        goto errorfound;
    }

    // We may need to add check for request-uri here.
    sprintf(szresbuf, "%-.200s:%-.200s", pbufHdr->GetBuffer(), pbufurival->GetBuffer());
    if (HXR_OK == CreateBufferCCF(pbufA2, m_pContext))
    {
	pbufA2->SetSize(64);

	sToken = (char*)pbufA2->GetBuffer();
	MD5Data(sToken, (const UCHAR*)szresbuf, strlen(szresbuf));    
    }

    HX_RELEASE(pbufHdr);

    if (SUCCEEDED(pCredentials->GetPropertyCString("qop", pbufHdr)))
    {
        if (strlen((char*)pbufHdr->GetBuffer()) > 4)
        {
            hr = HXR_FAIL;
            goto errorfound;
        }
    // Compute full string to be hashed
        sprintf(szresbuf, "%-.64s:%-.200s:%-.8s:%-.200s:%-.200s:%-.64s",
        pbufA1->GetBuffer(), pbufnonceval->GetBuffer(), pbufnccount->GetBuffer(),
        pbufcnonceval->GetBuffer(), pbufHdr->GetBuffer(), pbufA2->GetBuffer());    
    }
    else
    {
        sprintf(szresbuf, "%-.64s:%-.200s:%-.64s", pbufA1->GetBuffer(), 
            pbufnonceval->GetBuffer(), pbufA2->GetBuffer());
    }
    
    if (HXR_OK == CreateBufferCCF(*ppToken, m_pContext))
    {
	(*ppToken)->SetSize(64);
	sToken = (char*)(*ppToken)->GetBuffer();

	MD5Data(sToken, (const UCHAR*)szresbuf, strlen(szresbuf));    
    }

errorfound: 
        HX_RELEASE(pbufnonceval);
        HX_RELEASE(pbufcnonceval);
        HX_RELEASE(pbufHdr);
        HX_RELEASE(pResponseHeaders);
        HX_RELEASE(pbufurival);
        HX_RELEASE(pbufA1);
        HX_RELEASE(pbufA2);
        HX_RELEASE(pbufnccount);
        return hr;
}
