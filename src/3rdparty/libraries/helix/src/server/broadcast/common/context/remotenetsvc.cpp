/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: remotenetsvc.cpp,v 1.8 2008/07/03 21:54:16 dcollins Exp $
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

#include "hlxclib/signal.h"

#include "hxtypes.h"
#include "nettypes.h"
#include "hxcom.h"
#include "hxnet.h"
#include "hxtime.h"
#include "sockimp.h"
#include "resolvcache.h"
#include "iresolv.h"
#include "callback_container.h"
#include "remotenetsvc.h"

CRemoteSockCB::CRemoteSockCB(CHXRemoteSocket* pSock, UINT32 event) :
    m_nRefCount(0),
    m_pSock(pSock),
    m_event(event)
{
    m_pSock->AddRef();
}

CRemoteSockCB::~CRemoteSockCB(void)
{
    HX_RELEASE(m_pSock);
}

STDMETHODIMP
CRemoteSockCB::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CRemoteSockCB::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CRemoteSockCB::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP
CRemoteSockCB::Func(void)
{
    m_pSock->OnEvent(m_event);
    return HXR_OK;
}

CHXRemoteSocket::CHXRemoteSocket(CHXNetServices* pNetSvc,
                                 IUnknown* punkContext,
                                 CallbackContainer* pCallbacks) :
    CHXSocket(pNetSvc, punkContext),
    m_uSelectedEventMask(0),
    m_pCallbacks(pCallbacks),
    m_pCBR(NULL),
    m_pCBW(NULL)
{
    // Empty
}

CHXRemoteSocket::CHXRemoteSocket(HXSockFamily f,
                             HXSockType t,
                             HXSockProtocol p,
                             HX_SOCK sock,
                             CHXNetServices* pNetSvc,
                             IUnknown* punkContext,
                             CallbackContainer* pCallbacks) :
    CHXSocket(pNetSvc,
              punkContext,
              f, t, p, sock),
    m_uSelectedEventMask(0),
    m_pCallbacks(pCallbacks),
    m_pCBR(NULL),
    m_pCBW(NULL)
#ifdef _WINDOWS
    ,m_bAttemptingConnect(FALSE)
#endif
{
    m_pCBR = new CRemoteSockCB(this, HX_SOCK_EVENT_READ);
    m_pCBR->AddRef();
    m_pCallbacks->add(HX_READERS, m_sock.sock, m_pCBR, TRUE);
    m_pCallbacks->disable(HX_READERS, m_sock.sock);

    m_pCBW = new CRemoteSockCB(this, HX_SOCK_EVENT_WRITE);
    m_pCBW->AddRef();
    m_pCallbacks->add(HX_WRITERS, m_sock.sock, m_pCBW, TRUE);
    m_pCallbacks->disable(HX_WRITERS, m_sock.sock);

#ifdef _WINDOWS
    m_pCBC = new CRemoteSockCB(this, HX_CONNECTORS);
    m_pCBC->AddRef();
    m_pCallbacks->add(HX_CONNECTORS, m_sock.sock, m_pCBC, TRUE);
    m_pCallbacks->disable(HX_CONNECTORS, m_sock.sock);
#endif
}

CHXRemoteSocket::~CHXRemoteSocket(void)
{
    SelectEvents(HX_SOCK_EVENT_NONE);
    HX_RELEASE(m_pCBW);
    HX_RELEASE(m_pCBR);
    m_pCallbacks = NULL;
}

HX_RESULT
CHXRemoteSocket::Select(UINT32 uEventMask, BOOL bImplicit /* = TRUE */)
{
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        m_pSock4->SelectEvents(uEventMask);
    }
#endif

    if (!HX_SOCK_VALID(m_sock))
    {
        return HXR_FAIL;
    }

    if (uEventMask == m_uSelectedEventMask)
    {
        return HXR_OK;
    }

    m_uSelectedEventMask = uEventMask;

    if (uEventMask & (HX_SOCK_EVENT_READ |
                      HX_SOCK_EVENT_ACCEPT |
                      HX_SOCK_EVENT_CLOSE))
    {
        m_pCallbacks->enable(HX_READERS, m_sock.sock, TRUE);
    }
    else
    {
        m_pCallbacks->disable(HX_READERS, m_sock.sock);
    }

    if (uEventMask & (HX_SOCK_EVENT_WRITE |
                      HX_SOCK_EVENT_CONNECT))
    {
        m_pCallbacks->enable(HX_WRITERS, m_sock.sock, TRUE);

#ifdef _WINDOWS
       if (m_bAttemptingConnect)
       {
	    m_pCallbacks->enable(HX_CONNECTORS, m_sock.sock, TRUE);
       }
#endif
    }
    else
    {
        m_pCallbacks->disable(HX_WRITERS, m_sock.sock);
#ifdef _WINDOWS
        m_pCallbacks->disable(HX_CONNECTORS, m_sock.sock);
#endif
    }

    return HXR_OK;
}

void
CHXRemoteSocket::OnEvent(UINT32 ulEvent)
{
#ifdef _WINDOWS
    if (m_bAttemptingConnect && (ulEvent & (HX_SOCK_EVENT_WRITE |
                      HX_SOCK_EVENT_CONNECT)))
    {
        m_pCallbacks->disable(HX_CONNECTORS, m_sock.sock);
	m_bAttemptingConnect = FALSE;
    }
#endif
    CHXSocket::OnEvent(ulEvent);
}

STDMETHODIMP
CHXRemoteSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    HX_RESULT hxr;
    hxr = CHXSocket::Init(f, t, p);
    if (SUCCEEDED(hxr))
    {
        m_pCBR = new CRemoteSockCB(this, HX_SOCK_EVENT_READ);
        m_pCBR->AddRef();
        (*m_pCallbacks).add(HX_READERS, m_sock.sock, m_pCBR, TRUE);
        m_pCallbacks->disable(HX_READERS, m_sock.sock);

        m_pCBW = new CRemoteSockCB(this, HX_SOCK_EVENT_WRITE);
        m_pCBW->AddRef();
        m_pCallbacks->add(HX_WRITERS, m_sock.sock, m_pCBW, TRUE);
        m_pCallbacks->disable(HX_WRITERS, m_sock.sock);

#ifdef _WINDOWS
        m_pCBC = new CRemoteSockCB(this, HX_CONNECTORS);
        m_pCBC->AddRef();
        m_pCallbacks->add(HX_CONNECTORS, m_sock.sock, m_pCBC, TRUE);
        m_pCallbacks->disable(HX_CONNECTORS, m_sock.sock);
        m_bAttemptingConnect = FALSE;
#endif 

    }
    return hxr;
}

STDMETHODIMP
CHXRemoteSocket::Close(void)
{
    /*
     * The only reliable indication of whether the event callbacks should be
     * removed is that the socket object is valid before calling the base
     * Close and invalid after.  This is because the write queue linger
     * logic may keep the socket open while it is flushing the queue.
     *
     * Also note we must always call the base Close method regardless of
     * whether our m_sock is valid to allow for IPv4 mode on Win32.
     */
    HX_SOCK s = m_sock;
    HX_RESULT hxr = CHXSocket::Close();
    if (HX_SOCK_VALID(s) && !HX_SOCK_VALID(m_sock))
    {
        m_pCallbacks->remove(HX_READERS, s.sock);
        m_pCallbacks->remove(HX_WRITERS, s.sock);
#ifdef _WINDOWS
	m_pCallbacks->remove(HX_CONNECTORS, s.sock);
        m_bAttemptingConnect = FALSE;
#endif
        HX_RELEASE(m_pCBW);
        HX_RELEASE(m_pCBR);
#ifdef _WINDOWS
        HX_RELEASE(m_pCBC);
#endif
    }
    return hxr;   
}

CRemoteNetServicesContext::CRemoteNetServicesContext(CallbackContainer* pCallback) :
    CHXNetServices(),
    m_pCallback (pCallback),
    m_pResolverCache(NULL)
{
    //Empty
}

CRemoteNetServicesContext::~CRemoteNetServicesContext(void)
{
    m_pCallback = NULL;
    HX_RELEASE(m_pResolverCache);
}

HX_RESULT
CRemoteNetServicesContext::Init (IUnknown* punkContext)
{
    // Initialize base with the driver
    HX_RESULT hr = CHXNetServices::Init(punkContext);

    if (SUCCEEDED(hr))
    {
        // Create resolver cache for this proc
        HX_ASSERT(m_pResolverCache == NULL);
        m_pResolverCache = new CResolverCache(punkContext);
        m_pResolverCache->AddRef();
    }
    return hr;
}

HX_RESULT
CRemoteNetServicesContext::CreateSocket(HXSockFamily f, HXSockType t,
        HXSockProtocol p, HX_SOCK sock, IHXSocket** ppSock)
{
    *ppSock = new CHXRemoteSocket(f, t, p, sock,
                                (CHXNetServices*)this,
                                m_punkContext,
                                m_pCallback);
    if (*ppSock == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppSock)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CRemoteNetServicesContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXNetServices*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetServices))
    {
        AddRef();
        *ppvObj = (IHXNetServices*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CRemoteNetServicesContext::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CRemoteNetServicesContext::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP
CRemoteNetServicesContext::CreateSocket(IHXSocket** ppSock)
{
    *ppSock = new CHXRemoteSocket((CHXNetServices*)this,
                                m_punkContext,
                                m_pCallback);

    if (*ppSock == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppSock)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CRemoteNetServicesContext::CreateResolver(IHXResolve** ppResolver)
{
    *ppResolver = new CIntResolver(m_punkContext, m_pResolverCache);
    if (*ppResolver == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppResolver)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CRemoteNetServicesContext::CreateSockAddr(HXSockFamily f,
        IHXSockAddr** ppAddr)
{
    return CHXNetServices::CreateSockAddr(f, ppAddr);
}

STDMETHODIMP
CRemoteNetServicesContext::CreateListeningSocket(IHXListeningSocket** ppSock)
{
    IHXSocket* pActualSock = new CHXRemoteSocket((CHXNetServices*)this,
                                               m_punkContext,
                                               m_pCallback);
    if (pActualSock == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    *ppSock = new CHXListeningSocket(pActualSock);
    if (*ppSock == NULL)
    {
        delete pActualSock;
        return HXR_OUTOFMEMORY;
    }
    (*ppSock)->AddRef();
    return HXR_OK;
}

