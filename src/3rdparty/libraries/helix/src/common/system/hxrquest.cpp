/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrquest.cpp,v 1.7 2006/01/31 23:35:06 ping Exp $
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

#include "hxtypes.h"
#include "hxassert.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxstrutl.h"
#include "chxpckts.h"
#include "hxrquest.h"
#include "pckunpck.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

STDMETHODIMP_(ULONG32)
CHXRequest::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CHXRequest::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CHXRequest::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXRequest*)this },
		{ GET_IIDHANDLE(IID_IHXRequest), (IHXRequest*) this },
		{ GET_IIDHANDLE(IID_IHXRequestContext), (IHXRequestContext*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

CHXRequest::CHXRequest()
    : m_lRefCount(0)
    , m_pURL(NULL)
    , m_pRequestHeaders(NULL)
    , m_pResponseHeaders(NULL)
    , m_pIUnknownUserContext(NULL)
    , m_pIUnknownRequester(NULL)
{
}

CHXRequest::~CHXRequest()
{
    if (m_pURL)
    {
	delete[] m_pURL;
    }

    if (m_pRequestHeaders)
    {
	m_pRequestHeaders->Release();
    }

    if (m_pResponseHeaders)
    {
	m_pResponseHeaders->Release();
    }

    HX_RELEASE(m_pIUnknownUserContext);
    HX_RELEASE(m_pIUnknownRequester);
}

STDMETHODIMP
CHXRequest::SetRequestHeaders(IHXValues* pRequestHeaders)
{
    if (m_pRequestHeaders)
    {
	m_pRequestHeaders->Release();
	m_pRequestHeaders = 0;
    }

    m_pRequestHeaders = pRequestHeaders;

    if (m_pRequestHeaders)
    {
	m_pRequestHeaders->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
CHXRequest::GetRequestHeaders(REF(IHXValues*) pRequestHeaders)
{
    if (m_pRequestHeaders)
    {
	m_pRequestHeaders->AddRef();
    }

    pRequestHeaders = m_pRequestHeaders;

    return HXR_OK;
}

STDMETHODIMP
CHXRequest::SetResponseHeaders(IHXValues* pResponseHeaders)
{
    if (m_pResponseHeaders)
    {
	m_pResponseHeaders->Release();
	m_pResponseHeaders = 0;
    }

    m_pResponseHeaders = pResponseHeaders;

    if (m_pResponseHeaders)
    {
	m_pResponseHeaders->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
CHXRequest::GetResponseHeaders(REF(IHXValues*) pResponseHeaders)
{
    if (m_pResponseHeaders)
    {
	m_pResponseHeaders->AddRef();
    }

    pResponseHeaders = m_pResponseHeaders;

    return HXR_OK;
}

STDMETHODIMP
CHXRequest::SetURL(const char* pURL)
{
    if (m_pURL)
    {
	delete[] m_pURL;
	m_pURL = 0;
    }

    if (pURL)
    {
	m_pURL = new_string(pURL);
    }

    return HXR_OK;
}

STDMETHODIMP
CHXRequest::GetURL(REF(const char*) pURL)
{
    pURL = m_pURL;

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXRequest::SetUserContext
 *	Purpose:
 *	    Sets the Authenticated users Context.
 */
STDMETHODIMP
CHXRequest::SetUserContext
(
    IUnknown* pIUnknownNewContext
)
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
CHXRequest::GetUserContext
(
    REF(IUnknown*) pIUnknownCurrentContext
)
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
CHXRequest::SetRequester
(
    IUnknown* pIUnknownNewRequester
)
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
CHXRequest::GetRequester
(
    REF(IUnknown*) pIUnknownCurrentRequester
)
{
    pIUnknownCurrentRequester = m_pIUnknownRequester;
    if(pIUnknownCurrentRequester)
    {
	pIUnknownCurrentRequester->AddRef();
	return HXR_OK;
    }

    return HXR_FAIL;
}

#ifndef HELIX_FEATURE_CLIENT
void
CHXRequest::CreateFrom(IHXRequest* pRequestOld, IHXRequest** ppRequestNew)
{
    IHXRequestContext* pRequestContextOld = NULL;
    CHXRequest* pRequestNew = new CHXRequest();

    pRequestOld->GetRequestHeaders(pRequestNew->m_pRequestHeaders);
    pRequestOld->GetResponseHeaders(pRequestNew->m_pResponseHeaders);
    pRequestOld->QueryInterface
    (
	IID_IHXRequestContext, 
	(void**)&pRequestContextOld
    );
    if (pRequestContextOld)
    {
	pRequestContextOld->GetUserContext
	(
	    pRequestNew->m_pIUnknownUserContext
	);
	pRequestContextOld->GetRequester
	(
	    pRequestNew->m_pIUnknownRequester
	);
    }
    HX_RELEASE(pRequestContextOld);

    (*ppRequestNew) = pRequestNew;
    (*ppRequestNew)->AddRef();
}

void
CHXRequest::CreateFromWithRequestHeaderOnly(IHXRequest* pRequestOld, IHXRequest** ppRequestNew)
{
    IHXValues*          pRequestHeaderSrc = NULL;
    IHXRequestContext*  pRequestContextOld = NULL;
    CHXRequest*         pRequestNew = new CHXRequest();

    pRequestNew->m_pResponseHeaders = NULL;

    if (HXR_OK == pRequestOld->GetRequestHeaders(pRequestHeaderSrc) &&
        pRequestHeaderSrc)
    {
	pRequestNew->m_pRequestHeaders = new CHXHeader();
	if (pRequestNew->m_pRequestHeaders)
	{
	    pRequestNew->m_pRequestHeaders->AddRef();
	    CHXHeader::mergeHeaders(pRequestNew->m_pRequestHeaders,
				    pRequestHeaderSrc);
        }
        HX_RELEASE(pRequestHeaderSrc);
    }

    pRequestOld->QueryInterface
    (
	IID_IHXRequestContext, 
	(void**)&pRequestContextOld
    );
    if (pRequestContextOld)
    {
	pRequestContextOld->GetUserContext
	(
	    pRequestNew->m_pIUnknownUserContext
	);
	pRequestContextOld->GetRequester
	(
	    pRequestNew->m_pIUnknownRequester
	);
    }
    HX_RELEASE(pRequestContextOld);

    (*ppRequestNew) = pRequestNew;
    (*ppRequestNew)->AddRef();
}
#endif

void
CHXRequest::CreateFromCCF(IHXRequest* pRequestOld, IHXRequest** ppRequestNew, IUnknown* pContext)
{
    IHXRequestContext* pRequestContextOld = NULL;
    IHXRequestContext* pRequestContextNew = NULL;
    IHXValues* pValues = NULL;
    IUnknown* pUnknown = NULL;
    IHXCommonClassFactory* pCCF = NULL;

    *ppRequestNew = NULL;

    if (HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF))
    {
	if (HXR_OK == pCCF->CreateInstance(CLSID_IHXRequest, (void**)ppRequestNew))
	{   
	    pRequestOld->GetRequestHeaders(pValues);
	    (*ppRequestNew)->SetRequestHeaders(pValues);
	    HX_RELEASE(pValues);

	    pRequestOld->GetResponseHeaders(pValues);
	    (*ppRequestNew)->SetResponseHeaders(pValues);
	    HX_RELEASE(pValues);

	    pRequestOld->QueryInterface
	    (
		IID_IHXRequestContext, 
		(void**)&pRequestContextOld
	    );

	    (*ppRequestNew)->QueryInterface(IID_IHXRequestContext, 
					   (void**)&pRequestContextNew);

	    if (pRequestContextOld && pRequestContextNew)
	    {
		pRequestContextOld->GetUserContext(pUnknown);
		pRequestContextNew->SetUserContext(pUnknown);
		HX_RELEASE(pUnknown);

		pRequestContextOld->GetRequester(pUnknown);
		pRequestContextNew->SetRequester(pUnknown);
		HX_RELEASE(pUnknown);
	    }
	    HX_RELEASE(pRequestContextOld);
	    HX_RELEASE(pRequestContextNew);
	}   
    }
    HX_RELEASE(pCCF);
}

void
CHXRequest::CreateFromCCFWithRequestHeaderOnly(IHXRequest* pRequestOld, IHXRequest** ppRequestNew,
					       IUnknown* pContext)
{
    IHXValues*          pRequestHeaderSrc = NULL;
    IHXValues*		pValues = NULL;
    IUnknown*		pUnknown = NULL;
    IHXRequestContext*  pRequestContextOld = NULL;
    IHXRequestContext*  pRequestContextNew = NULL;
    IHXCommonClassFactory* pCCF = NULL;

    *ppRequestNew = NULL;

    if (HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF))
    {
	if (HXR_OK == pCCF->CreateInstance(CLSID_IHXRequest, (void**)ppRequestNew))
	{    
	    if (HXR_OK == pRequestOld->GetRequestHeaders(pRequestHeaderSrc) &&
		pRequestHeaderSrc)
	    {
		CreateValuesCCF(pValues, pContext);
		if(pValues)
		{
		    CHXHeader::mergeHeaders(pValues, pRequestHeaderSrc);
		    (*ppRequestNew)->SetRequestHeaders(pValues);
		    HX_RELEASE(pValues);    
		}
		HX_RELEASE(pRequestHeaderSrc);
	    }

	    pRequestOld->QueryInterface
	    (
		IID_IHXRequestContext, 
		(void**)&pRequestContextOld
	    );

	    (*ppRequestNew)->QueryInterface(IID_IHXRequestContext, 
					   (void**)&pRequestContextNew);

	    if (pRequestContextOld && pRequestContextNew)
	    {
		pRequestContextOld->GetUserContext(pUnknown);
		pRequestContextNew->SetUserContext(pUnknown);
		HX_RELEASE(pUnknown);

		pRequestContextOld->GetRequester(pUnknown);
		pRequestContextNew->SetRequester(pUnknown);
		HX_RELEASE(pUnknown);
	    }
	    HX_RELEASE(pRequestContextOld);
	    HX_RELEASE(pRequestContextNew);
	}
    }
    HX_RELEASE(pCCF);
}
