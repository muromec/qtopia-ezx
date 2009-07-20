/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httppost.cpp,v 1.6 2007/07/06 20:43:53 jfinnecy Exp $
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
#include "hxresult.h"
#include "hxcom.h"

#include "hxstring.h"
#include "hxbuffer.h"
#include "hxengin.h"
#include "pckunpck.h"
#include "hxsockutil.h"
#include "httppost.h"

CHTTPPost::CHTTPPost()
    : m_pContext(NULL)
    , m_pScheduler(NULL)
    , m_pNetServices(NULL)
    , m_pPostMsg(NULL)
    , m_pSocket(NULL)
    , m_pResolver(NULL)
    , m_lRefCount(0)
    , m_bReleasePending(FALSE)
    , m_ulConnTimeout(3000)
    , m_ulCallbackID(0)
    , m_pCallback(NULL)
{
}

CHTTPPost::~CHTTPPost()
{
    Close();
}

STDMETHODIMP 
CHTTPPost::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXResolveResponse))
    {
	AddRef();
	*ppvObj = (IHXResolveResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
	AddRef();
	*ppvObj = (IHXSocketResponse*)this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32) 
CHTTPPost::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32) 
CHTTPPost::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
CHTTPPost::Init(IUnknown* pContext, UINT32 ulTimeout)
{
    HX_RESULT       rc = HXR_OK;

    if (!pContext || m_pContext)
    {
	return HXR_FAILED;	
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    if (0 != ulTimeout)
    {
	m_ulConnTimeout = ulTimeout;
    }
    
    if (HXR_OK != m_pContext->QueryInterface(IID_IHXScheduler, (void **)&m_pScheduler) ||
        HXR_OK != m_pContext->QueryInterface(IID_IHXNetServices, (void **)&m_pNetServices))
    {
	rc = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    rc = m_pNetServices->CreateResolver(&m_pResolver);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    rc = m_pResolver->Init((IHXResolveResponse*)this);

cleanup:

    return rc;
}

HX_RESULT
CHTTPPost::Post(const char* pHost, UINT32 ulPort, BYTE* pMsg, UINT32 ulMsgSize)
{
    HX_RESULT   rc = HXR_OK;

    if (!pMsg || !pHost || !ulPort) 
    {
	rc = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (!m_pScheduler || !m_pNetServices || !m_pResolver)
    {
        rc = HXR_NOT_INITIALIZED;
        goto cleanup;
    }

    rc = CreateBufferCCF(m_pPostMsg, m_pContext);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    m_pPostMsg->Set(pMsg, ulMsgSize);

    char szPort[HX_PORTSTRLEN]; /* Flawfinder: ignore */
    sprintf(szPort, "%lu", ulPort);

    rc = m_pResolver->GetAddrInfo(pHost, szPort, NULL);

    // Ask the scheduler for callback to catch connection
    // timeout
    if (HXR_OK == rc)	
    {
    	m_pCallback = new CHTTPPostTimeoutCallback(this);
	if (m_pCallback)
	{
	    m_pCallback->AddRef();
	    m_ulCallbackID = m_pScheduler->RelativeEnter(m_pCallback, m_ulConnTimeout);
	}	    
    }

cleanup:

    if (HXR_OK == rc)
    {
	AddRef();
        m_bReleasePending = TRUE;
        rc = HXR_WOULD_BLOCK;
    }

    return rc;
}

HX_RESULT   
CHTTPPost::Post(const char* pHost, UINT32 ulPort, const char* resource,
                IHXValues* pHeader, IHXBuffer* pBody)
{
    HX_RESULT   rc = HXR_OK;
    UINT32      ulMsgSize = 0;
    BYTE*       pMsg = NULL;
    const char* pName = NULL;
    IHXBuffer*  pValue = NULL;
    CHXString   header;

    if (!pHost || !ulPort || !resource || !pBody)
    {
        if (m_bReleasePending)
        {
            m_bReleasePending = FALSE;
            Release();
        }
	return HXR_INVALID_PARAMETER;
    }

    header = "POST ";
    if ('/' != *resource)
    {
        // '/' is expected before the URL path
        pMsg += '/';
    }
    header += resource;
    header += " HTTP/1.0";

    // construct the headers
    if (pHeader)
    {
        HX_RESULT hr = HXR_OK;

        hr = pHeader->GetFirstPropertyCString(pName, pValue);
        while (HXR_OK == hr)
        {
            header += "\r\n";
            header += pName;
            header += ": ";
            header += (const char*)pValue->GetBuffer();

            HX_RELEASE(pValue);
            hr = pHeader->GetNextPropertyCString(pName, pValue);
        }
        HX_RELEASE(pValue);
    }

    header += "\r\nContent-Length: ";
    header.AppendULONG(pBody->GetSize());

    header += "\r\n\r\n";

    ulMsgSize = header.GetLength() + pBody->GetSize();
    
    pMsg = new BYTE[ulMsgSize];
    if (!pMsg)
    {
        rc = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    memcpy(pMsg, (const char*)header, header.GetLength());
    memcpy(pMsg+header.GetLength(), pBody->GetBuffer(), pBody->GetSize());

    rc = Post(pHost, ulPort, pMsg, ulMsgSize);
    
cleanup:

    HX_VECTOR_DELETE(pMsg);
    return rc;
}

STDMETHODIMP 
CHTTPPost::GetAddrInfoDone(HX_RESULT        status, 
                           UINT32           nVecLen, 
                           IHXSockAddr**    ppAddrVec)
{
    HX_RESULT       rc = status;
    IHXSockAddr**   ppConvSockAddr = NULL;

    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    if (!nVecLen || !ppAddrVec)
    {
        rc = HXR_UNEXPECTED;
        goto cleanup;
    }

    rc = m_pNetServices->CreateSocket(&m_pSocket);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    rc = m_pSocket->SetResponse((IHXSocketResponse*)this);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    rc = m_pSocket->Init(HX_SOCK_FAMILY_INANY,
                         HX_SOCK_TYPE_TCP,
                         HX_SOCK_PROTO_ANY);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    m_pSocket->SelectEvents(HX_SOCK_EVENT_READ    |
                            HX_SOCK_EVENT_WRITE   |
                            HX_SOCK_EVENT_CONNECT |
                            HX_SOCK_EVENT_CLOSE);

    rc = HXSockUtil::AllocAddrVec(ppAddrVec,
                                  nVecLen,
                                  ppConvSockAddr,
                                  m_pSocket->GetFamily(),
                                  true,
                                  m_pNetServices);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    // Now call ConnectToAny    
    rc = m_pSocket->ConnectToAny(nVecLen, ppConvSockAddr);

    // Free the addresses we converted
    HXSockUtil::FreeAddrVec(ppConvSockAddr, nVecLen);

cleanup:

    return rc;
}

STDMETHODIMP 
CHTTPPost::GetNameInfoDone(HX_RESULT status, 
                           const char* pszNode, 
                           const char* pszService)
{
    Close();
    return HXR_UNEXPECTED;
}

STDMETHODIMP 
CHTTPPost::EventPending(UINT32 uEvent, HX_RESULT status)
{    
    HX_RESULT rc = HXR_OK;

    switch (uEvent)
    {
        case HX_SOCK_EVENT_READ:
            {
                rc = ReadDone(status, NULL);
            }
            break;
        case HX_SOCK_EVENT_WRITE:
            {
                rc = WriteReady(status);
            }
            break;
        case HX_SOCK_EVENT_CONNECT:
            {
                rc = ConnectDone(status);
            }
            break;
        case HX_SOCK_EVENT_CLOSE:
            {
                rc = Closed(status);
            }
            break;
        default:
            break;
    }

    return rc;
}

HX_RESULT
CHTTPPost::ConnectDone(HX_RESULT status)
{
    RemoveCallback();

    if (HXR_OK == status && !m_bReleasePending)
    {
        HX_ASSERT(m_pSocket);
        if (m_pSocket)
        {
            m_pSocket->Write(m_pPostMsg);
        }
    }
    
    Close();

    return HXR_OK;
}

HX_RESULT
CHTTPPost::ReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    Close();

    return HXR_OK;    
}

HX_RESULT
CHTTPPost::WriteReady(HX_RESULT   status)
{
    Close();

    return HXR_OK;
}

HX_RESULT
CHTTPPost::Closed(HX_RESULT status)
{
    Close();

    return HXR_OK;
}

void
CHTTPPost::Close(void)
{
    RemoveCallback();

    if (m_pSocket)
    {
        m_pSocket->Close();
        HX_RELEASE(m_pSocket);
    }
    
    HX_RELEASE(m_pResolver);
    HX_RELEASE(m_pPostMsg);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pContext);

    if (m_bReleasePending)
    {
	m_bReleasePending = FALSE;
	Release();
    }	
}

void 
CHTTPPost::RemoveCallback(void)
{
    if (m_ulCallbackID && m_pScheduler)
    {
    	m_pScheduler->Remove(m_ulCallbackID);
    	m_ulCallbackID = 0;
    }

    HX_RELEASE(m_pCallback);
}

			
/**************
*   Timeout
*/
CHTTPPost::CHTTPPostTimeoutCallback::CHTTPPostTimeoutCallback(CHTTPPost* pOwner)
    : m_lRefCount(0)
    , m_pOwner(pOwner)
{
}

CHTTPPost::CHTTPPostTimeoutCallback::~CHTTPPostTimeoutCallback()
{
}

STDMETHODIMP
CHTTPPost::CHTTPPostTimeoutCallback::Func()
{
    m_pOwner->ConnectDone(HXR_FAIL);
    return HXR_OK;
}

STDMETHODIMP 
CHTTPPost::CHTTPPostTimeoutCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXCallback))
    {
	AddRef();
	*ppvObj = (IHXCallback*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32) 
CHTTPPost::CHTTPPostTimeoutCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32) 
CHTTPPost::CHTTPPostTimeoutCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

