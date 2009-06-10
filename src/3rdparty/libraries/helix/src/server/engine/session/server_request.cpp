/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_request.cpp,v 1.7 2008/03/09 12:18:34 npatil Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "hxcom.h"

#include "hxassert.h"
#include "ihxpckts.h"
#include "hxstrutl.h"
#include "chxpckts.h"
#include "server_request.h"
#include "clone_values.h"
#include "urlparser.h"

ServerRequest::ServerRequest()
    : m_lRefCount(0)
    , m_pURL(NULL)
    , m_pFSRequestHeaders(NULL)
    , m_pFSResponseHeaders(NULL)
    , m_pFFRequestHeaders(NULL)
    , m_pFFResponseHeaders(NULL)
    , m_pIUnknownUserContext(NULL)
    , m_pIUnknownRequester(NULL)
{
}

ServerRequest::~ServerRequest()
{
    HX_RELEASE(m_pURL);

    if (m_pFSRequestHeaders)
    {
	m_pFSRequestHeaders->Release();
    }

    if (m_pFSResponseHeaders)
    {
	m_pFSResponseHeaders->Release();
    }

    if (m_pFFRequestHeaders)
    {
	m_pFFRequestHeaders->Release();
    }

    if (m_pFFResponseHeaders)
    {
	m_pFFResponseHeaders->Release();
    }

    HX_RELEASE(m_pIUnknownUserContext);

    HX_RELEASE(m_pIUnknownRequester);
}

ServerRequest* ServerRequest::Clone(IHXCommonClassFactory* pCCF)
{
    const char* pEncUrl = NULL;
    ServerRequest* pNew = new ServerRequest();
    pNew->AddRef();

    m_pURL->GetEncodedUrl(pEncUrl);
    pNew->SetURL(pEncUrl);

    if(this->m_pFSRequestHeaders)
    {
        pNew->m_pFSRequestHeaders = this->m_pFSRequestHeaders;
        pNew->m_pFSRequestHeaders->AddRef();
    }

    if(this->m_pFSResponseHeaders)
    {
        pNew->m_pFSResponseHeaders = this->m_pFSResponseHeaders;
        pNew->m_pFSResponseHeaders->AddRef();
    }

    if(this->m_pFFRequestHeaders)
    {
        pNew->m_pFFRequestHeaders = this->m_pFFRequestHeaders;
        pNew->m_pFFRequestHeaders->AddRef();
    }

    if(this->m_pFFResponseHeaders)
    {
        pNew->m_pFFResponseHeaders = this->m_pFFResponseHeaders;
        pNew->m_pFFResponseHeaders->AddRef();
    }

    pNew->m_pIUnknownUserContext = this->m_pIUnknownUserContext;
    if(pNew->m_pIUnknownUserContext)
    {
        m_pIUnknownUserContext->AddRef();
    }

    pNew->m_pIUnknownRequester = this->m_pIUnknownRequester;
    if(pNew->m_pIUnknownRequester)
    {
        m_pIUnknownRequester->AddRef();
    }

    return pNew;
}

STDMETHODIMP_(ULONG32)
ServerRequest::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
ServerRequest::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
ServerRequest::SetRequestHeaders(REQUEST_HEADER_TYPE HeaderType,
    IHXValues* pRequestHeaders)
{

    if(HeaderType == FS_HEADERS)
    {
	if(m_pFSRequestHeaders)
	{
	    m_pFSRequestHeaders->Release();
	}
	m_pFSRequestHeaders = pRequestHeaders;
	m_pFSRequestHeaders->AddRef();
    }
    else if(HeaderType == FF_HEADERS)
    {
	if(m_pFFRequestHeaders)
	{
	    m_pFFRequestHeaders->Release();
	}
	m_pFFRequestHeaders = pRequestHeaders;
	m_pFFRequestHeaders->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
ServerRequest::GetRequestHeaders(REQUEST_HEADER_TYPE HeaderType,
    REF(IHXValues*) pRequestHeaders)
{
    IHXValues* pHeaders;

    pHeaders = _GetRequestHeaders(HeaderType);

    if (pHeaders)
    {
	pHeaders->AddRef();
    }

    pRequestHeaders = pHeaders;

    return HXR_OK;
}

STDMETHODIMP
ServerRequest::SetResponseHeaders(REQUEST_HEADER_TYPE HeaderType,
    IHXValues* pResponseHeaders)
{
    if(HeaderType == FS_HEADERS)
    {
	if(m_pFSResponseHeaders)
	{
	    m_pFSResponseHeaders->Release();
	}
	m_pFSResponseHeaders = pResponseHeaders;
	m_pFSResponseHeaders->AddRef();
    }
    else if(HeaderType == FF_HEADERS)
    {
	if(m_pFFResponseHeaders)
	{
	    m_pFFResponseHeaders->Release();
	}
	m_pFFResponseHeaders = pResponseHeaders;
	m_pFFResponseHeaders->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
ServerRequest::GetResponseHeaders(REQUEST_HEADER_TYPE HeaderType,
    REF(IHXValues*) pResponseHeaders)
{
    IHXValues* pHeaders;

    pHeaders = _GetResponseHeaders(HeaderType);

    if (pHeaders)
    {
	pHeaders->AddRef();
    }

    pResponseHeaders = pHeaders;

    return HXR_OK;
}

STDMETHODIMP
ServerRequest::SetURL(const char* pURL)
{
    HX_RELEASE(m_pURL);

    if (pURL)
    {
        m_pURL = new CHXURLParser(pURL,strlen(pURL));
        m_pURL->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
ServerRequest::GetURL(REF(const char*) pURL)
{
    m_pURL->GetDecodedUrl(pURL);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *      IHXURLParser::GetHXURL
 *  Purpose:
 *      Gets the IHXURL object 
 */
STDMETHODIMP
ServerRequest::GetHXURL(REF(IHXURL*) pURL)
{
    pURL = m_pURL;
    if(pURL)
    {
        pURL->AddRef();
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *	    IHXRequest::SetUserContext
 *	Purpose:
 *	    Sets the Authenticated users Context.
 */
STDMETHODIMP
ServerRequest::SetUserContext(IUnknown* pIUnknownNewContext)
{
    HX_RELEASE(m_pIUnknownUserContext);

    m_pIUnknownUserContext = pIUnknownNewContext;
    if(m_pIUnknownUserContext)
    {
	m_pIUnknownUserContext->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXRequest::GetUserContext
 *	Purpose:
 *	    Gets the Authenticated users Context.
 */
STDMETHODIMP
ServerRequest::GetUserContext(REF(IUnknown*) pIUnknownCurrentContext)
{
    pIUnknownCurrentContext = m_pIUnknownUserContext;
    if(pIUnknownCurrentContext)
    {
	pIUnknownCurrentContext->AddRef();
	return HXR_OK;
    }

    return HXR_FAIL;
}

/************************************************************************
 *	Method:
 *	    IHXRequest::SetRequester
 *	Purpose:
 *	    Sets the Object that made the request.
 */
STDMETHODIMP
ServerRequest::SetRequester(IUnknown* pIUnknownNewRequester)
{
    // Don't allow the Requester object to be erased or reset.
    if (!pIUnknownNewRequester || m_pIUnknownRequester)
    {
	return HXR_FAIL;
    }

    m_pIUnknownRequester = pIUnknownNewRequester;
    if(m_pIUnknownRequester)
    {
	m_pIUnknownRequester->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXRequest::GetRequester
 *	Purpose:
 *	    Gets the Object that made the request.
 */
STDMETHODIMP
ServerRequest::GetRequester(REF(IUnknown*) pIUnknownCurrentRequester)
{
    pIUnknownCurrentRequester = m_pIUnknownRequester;
    if(pIUnknownCurrentRequester)
    {
	pIUnknownCurrentRequester->AddRef();
	return HXR_OK;
    }

    return HXR_FAIL;
}


ServerRequestWrapper::ServerRequestWrapper(REQUEST_HEADER_TYPE pHeaderType,
    ServerRequest* pServerRequest)
{
    m_lRefCount			= 0;
    m_HeaderType		= pHeaderType;
    m_pServerRequest		= pServerRequest;

    if (m_pServerRequest)
    {
	m_pServerRequest->AddRef();
    }
}

ServerRequestWrapper::~ServerRequestWrapper()
{
    if (m_pServerRequest)
    {
	m_pServerRequest->Release();
    }
}

STDMETHODIMP_(ULONG32)
ServerRequestWrapper::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
ServerRequestWrapper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
ServerRequestWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXRequest*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRequest))
    {
	AddRef();
	*ppvObj = (IHXRequest*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRequestContext))
    {
	AddRef();
	*ppvObj = (IHXRequestContext*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXURLParser))
    {
        AddRef();
        *ppvObj = (IHXURLParser*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

HX_RESULT
ServerRequestWrapper::RemoveRequest(void)
{
    HX_RELEASE(m_pServerRequest);
    return HXR_OK;
}

HX_RESULT
ServerRequestWrapper::ReplaceRequest(ServerRequest* pRequest)
{
    RemoveRequest();
    if (pRequest != NULL)
    {
        pRequest->AddRef();
        m_pServerRequest = pRequest;
    }
    return HXR_OK;
}

STDMETHODIMP
ServerRequestWrapper::SetRequestHeaders(IHXValues* pRequestHeaders)
{
    return m_pServerRequest->SetRequestHeaders(m_HeaderType,
                                               pRequestHeaders);
}

STDMETHODIMP
ServerRequestWrapper::GetRequestHeaders(REF(IHXValues*) pRequestHeaders)
{
    return m_pServerRequest->GetRequestHeaders(m_HeaderType,
                                               pRequestHeaders);
}

STDMETHODIMP
ServerRequestWrapper::SetResponseHeaders(IHXValues* pResponseHeaders)
{
    return m_pServerRequest->SetResponseHeaders(m_HeaderType,
                                                pResponseHeaders);
}

STDMETHODIMP
ServerRequestWrapper::GetResponseHeaders(REF(IHXValues*) pResponseHeaders)
{
    return m_pServerRequest->GetResponseHeaders(m_HeaderType,
                                                pResponseHeaders);
}

STDMETHODIMP
ServerRequestWrapper::SetURL(const char* pURL)
{
    return m_pServerRequest->SetURL(pURL);
}

STDMETHODIMP
ServerRequestWrapper::GetURL(REF(const char*) pURL)
{
    return m_pServerRequest->GetURL(pURL);
}

/************************************************************************
 *	Method:
 *      IHXURLParser::GetHXURL
 *  Purpose:
 *      Gets the IHXURL object 
 */
STDMETHODIMP
ServerRequestWrapper::GetHXURL(REF(IHXURL*) pURL)
{
    return m_pServerRequest->GetHXURL(pURL);
}

/************************************************************************
 *  Method:
 *	    IHXRequest::SetUserContext
 *	Purpose:
 *	    Sets the Authenticated users Context.
 */
STDMETHODIMP
ServerRequestWrapper::SetUserContext(IUnknown* pIUnknownNewContext)
{
    return m_pServerRequest->SetUserContext(pIUnknownNewContext);
}

/************************************************************************
 *	Method:
 *	    IHXRequest::GetUserContext
 *	Purpose:
 *	    Gets the Authenticated users Context.
 */
STDMETHODIMP
ServerRequestWrapper::GetUserContext(REF(IUnknown*) pIUnknownCurrentContext)
{
    return m_pServerRequest->GetUserContext(pIUnknownCurrentContext);
}

/************************************************************************
 *	Method:
 *	    IHXRequest::SetRequester
 *	Purpose:
 *	    Sets the Object that made the request.
 */
STDMETHODIMP
ServerRequestWrapper::SetRequester(IUnknown* pIUnknownNewRequester)
{
    return m_pServerRequest->SetRequester(pIUnknownNewRequester);
}

/************************************************************************
 *	Method:
 *	    IHXRequest::GetRequester
 *	Purpose:
 *	    Gets the Object that made the request.
 */
STDMETHODIMP
ServerRequestWrapper::GetRequester(REF(IUnknown*) pIUnknownCurrentRequester)
{
    return m_pServerRequest->GetRequester(pIUnknownCurrentRequester);
}
