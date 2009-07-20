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

#include "hxtypes.h"

#include "hxcom.h"
#include "hxengin.h"

#include "netdrv.h"
#include "hxnet.h"
#include "writequeue.h"
#include "sockimp.h"
#include "sockaddrimp.h"
#include "hxsockutil.h"
#include "hxassert.h"
#include "hxbuffer.h"
#include "timebuff.h"
#include "hxtick.h"

#include "hlxclib/time.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


/*** CHXSocketConnectEnumerator ***/

CHXSocketConnectEnumerator::CHXSocketConnectEnumerator(CHXSocket* pSock) :
    m_nRefCount(0),
    m_pSock(pSock),
    m_pOldResponse(NULL),
    m_nVecLen(0),
    m_ppAddrVec(NULL),
    m_nIndex(0)
{
    m_pSock->AddRef();
}

CHXSocketConnectEnumerator::~CHXSocketConnectEnumerator(void)
{
    if (m_ppAddrVec != NULL)
    {
        UINT32 n;
        for (n = 0; n < m_nVecLen; n++)
        {
            m_ppAddrVec[n]->Release();
        }
        HX_VECTOR_DELETE(m_ppAddrVec);
    }
    HX_RELEASE(m_pOldResponse);
    HX_RELEASE(m_pSock);
}

HX_RESULT
CHXSocketConnectEnumerator::Init(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    UINT32 n;
    HX_ASSERT(m_nVecLen == 0 && m_ppAddrVec == NULL);
    m_nVecLen = nVecLen;
    m_ppAddrVec = new IHXSockAddr*[nVecLen];
    if(!m_ppAddrVec)
    {
        return HXR_OUTOFMEMORY;
    }

    for (n = 0; n < nVecLen; n++)
    {
        m_ppAddrVec[n] = ppAddrVec[n];
        m_ppAddrVec[n]->AddRef();
    }

    m_pOldResponse = m_pSock->GetResponse();
    m_pSock->SetResponse(this);
    AttemptConnect(HXR_OK);
    return HXR_OK;
}

STDMETHODIMP
CHXSocketConnectEnumerator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXSocketConnectEnumerator::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXSocketConnectEnumerator::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

void
CHXSocketConnectEnumerator::FinishEnumeration(HX_RESULT status)
{
    // Use a local because the socket will likely release our last ref
    CHXSocket* pSock = m_pSock;
    m_pSock = NULL;
    IHXSocketResponse* pOldResponse = m_pOldResponse;
    m_pOldResponse = NULL;

    pSock->SetResponse(pOldResponse);
    pOldResponse->EventPending(HX_SOCK_EVENT_CONNECT, status);
    HX_RELEASE(pOldResponse);
    pSock->ConnectEnumeratorDone();
    pSock->Release();
}

STDMETHODIMP
CHXSocketConnectEnumerator::EventPending(UINT32 uEvent, HX_RESULT status)
{
    if (uEvent != HX_SOCK_EVENT_CONNECT)
    {
        //XXXTDM: can we get CLOSE here?
        HX_ASSERT(FALSE);
        return HXR_OK;
    }

    if(status == HXR_OK)
    {
        FinishEnumeration(HXR_OK);
    }
    else
    {
        AttemptConnect(status);
    }

    return HXR_OK;
}

void
CHXSocketConnectEnumerator::AttemptConnect(HX_RESULT status)
{
    for (;;)
    {
        if (m_nIndex >= m_nVecLen)
        {
            FinishEnumeration(status);
            break;
        }

        if (HXR_OK == m_pSock->ConnectToOne(m_ppAddrVec[m_nIndex++]))
        {
            break;
        } // else, ignore (hide) failure and continue
    }
}

#if defined(MISSING_DUALSOCKET)
/*
    return TRUE if address is a) IPv4 or b) IPv6 IPv4-mapped (bind/connect helper)
*/
static
HXBOOL RequiresSock4(IHXSockAddr* pAddr)
{
    if (pAddr->GetFamily() == HX_SOCK_FAMILY_IN4)
    {
        return TRUE;
    }

    HXBOOL requires = FALSE;

    IHXSockAddrNative* pNative = 0;
    HX_RESULT hxr = pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    if (SUCCEEDED(hxr))
    {
        sockaddr* psa;
        size_t salen;
        pNative->Get(&psa, &salen);
        if (psa->sa_family == AF_INET6)
        {
            sockaddr_in6* psa6 = (sockaddr_in6*)psa;
            requires = IN6_IS_ADDR_V4MAPPED(&psa6->sin6_addr);
        }
        HX_RELEASE(pNative);
    }
    return requires;
}
#endif // MISSING_DUALSOCKET

/*** CHXSocket ***/

CHXSocket::CHXSocket(CHXNetServices* pNetSvc, IUnknown* punkContext) :
    m_nRefCount(0),
    m_pNetSvc(pNetSvc),
    m_punkContext(punkContext),
    m_pCCF(NULL),
    m_pScheduler(NULL),
    m_family(HX_SOCK_FAMILY_NONE),
    m_type(HX_SOCK_TYPE_NONE),
    m_proto(HX_SOCK_PROTO_NONE),
    m_bufType(HX_SOCKBUF_DEFAULT),
    m_readBufAlloc(HX_SOCK_READBUF_SIZE_DEFAULT),
    m_sock(HX_SOCK_NONE),
#if defined(MISSING_DUALSOCKET)
    m_bV6ONLY(FALSE),
    m_pSock6(NULL),
    m_pSock4(NULL),
#endif
    m_uUserEventMask(0),
    m_uAllowedEventMask(0),
    m_uForcedEventMask(0),
    m_uPendingEventMask(0),
    m_mss(0),
    m_bBlocked(TRUE),
    m_uAggLimit(0),
    m_pWriteQueue(NULL),
    m_hIdleCallback(0),
    m_uIdleTimeout(0),
    m_tLastPacket(0),
    m_pResponse(NULL),
    m_pAccessControl(NULL),
    m_pEnumerator(NULL),
    m_pTotalNetReaders(NULL),
    m_pTotalNetWriters(NULL)
{
    HX_ADDREF(m_pNetSvc);
    if (m_punkContext != NULL)
    {
        m_punkContext->AddRef();
        m_punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
        m_punkContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    }
    HX_ASSERT(m_punkContext != NULL);
    HX_ASSERT(m_pCCF != NULL);
}

CHXSocket::CHXSocket(CHXNetServices* pNetSvc, IUnknown* punkContext,
        HXSockFamily f, HXSockType t, HXSockProtocol p, HX_SOCK sock) :
    m_nRefCount(0),
    m_pNetSvc(pNetSvc),
    m_punkContext(punkContext),
    m_pCCF(NULL),
    m_pScheduler(NULL),
    m_family(f),
    m_type(t),
    m_proto(p),
    m_bufType(HX_SOCKBUF_DEFAULT),
    m_readBufAlloc(HX_SOCK_READBUF_SIZE_DEFAULT),
    m_sock(sock),
#if defined(MISSING_DUALSOCKET)
    m_bV6ONLY(FALSE),
    m_pSock6(NULL),
    m_pSock4(NULL),
#endif
    m_uUserEventMask(0),
    m_uAllowedEventMask(0),
    m_uForcedEventMask(0),
    m_uPendingEventMask(0),
    m_mss(0),
    m_bBlocked(TRUE),
    m_uAggLimit(0),
    m_pWriteQueue(NULL),
    m_hIdleCallback(0),
    m_uIdleTimeout(0),
    m_tLastPacket(0),
    m_pResponse(NULL),
    m_pAccessControl(NULL),
    m_pEnumerator(NULL),
    m_pTotalNetReaders(NULL),
    m_pTotalNetWriters(NULL)
{
    HX_ADDREF(m_pNetSvc);
    if (m_punkContext != NULL)
    {
        m_punkContext->AddRef();
        m_punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
        m_punkContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    }
    HX_ASSERT(m_punkContext != NULL);
    HX_ASSERT(m_pCCF != NULL);

    if (IS_STREAM_TYPE(m_type))
    {
        m_pWriteQueue = new CHXStreamWriteQueue();
    }
    else
    {
        m_pWriteQueue = new CHXDatagramWriteQueue();
    }
    m_pWriteQueue->SetSize(MAX_IP_PACKET);
}

CHXSocket::~CHXSocket(void)
{
    HX_ASSERT(!HX_SOCK_VALID(m_sock)); // User must call Close()
    DoClose();
    HX_DELETE(m_pWriteQueue);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_punkContext);
    HX_RELEASE(m_pNetSvc);
}

IHXSocketResponse*
CHXSocket::GetResponse(void)
{
    if (m_pResponse != NULL)
    {
        m_pResponse->AddRef();
    }
    return m_pResponse;
}

/*
 * This function synthesizes ACCEPT, CONNECT and CLOSE events for Posix
 * platforms. Derived classes should only call this with WRITE and READ
 * events (similar to basic select() semantics), even if the underlying
 * select implementation provides the other events.
 */
void
CHXSocket::OnEvent(UINT32 uEvent)
{
    HX_ASSERT(HX_SOCK_EVENT_READ == uEvent || HX_SOCK_EVENT_WRITE == uEvent);
    HX_ASSERT(0 == m_uPendingEventMask); // HX_ASSERT during EventPending() (look higher up in callstack) on Windows?

    // If the socket is lingering, the write queue is being flushed
    if (m_sock.state == HX_SOCK_STATE_LINGERING)
    {
        HX_ASSERT(uEvent & HX_SOCK_EVENT_WRITE);
        HX_RESULT hxr = HandleWriteEvent();
        if (FAILED(hxr) || m_pWriteQueue->IsEmpty())
        {
            Close();
            Release();
        }
        return;
    }

    AddRef(); // Ensure object is valid for the duration of this function
    m_uPendingEventMask = uEvent;
    switch (m_sock.state)
    {
    case HX_SOCK_STATE_NORMAL:
        if ((uEvent & HX_SOCK_EVENT_READ) && m_pResponse != NULL)
        {
            m_uAllowedEventMask &= ~(HX_SOCK_EVENT_READ |HX_SOCK_EVENT_ACCEPT |HX_SOCK_EVENT_CLOSE);
            time(&m_tLastPacket);
            m_pResponse->EventPending(HX_SOCK_EVENT_READ, HXR_OK);
            Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask);
        }
        else if (uEvent & HX_SOCK_EVENT_WRITE)
        {
            if (!m_pWriteQueue->IsEmpty())
            {
                HX_RESULT hxr = HandleWriteEvent();
		if (FAILED(hxr))
		{
		    if (IS_STREAM_TYPE(m_type))
		    {
			if ((m_uUserEventMask & HX_SOCK_EVENT_CLOSE) && 
			    m_pResponse != NULL)
			{
			    m_pResponse->EventPending(HX_SOCK_EVENT_CLOSE, hxr);
			}
		    }
		    m_bBlocked = FALSE;
		}
            }
            if (m_bBlocked && m_pWriteQueue->IsEmpty())
            {
                m_bBlocked = FALSE;
                m_uAllowedEventMask &= ~HX_SOCK_EVENT_WRITE;
                Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask);
                if ((m_uUserEventMask & HX_SOCK_EVENT_WRITE) && m_pResponse != NULL)
                {
                    m_pResponse->EventPending(HX_SOCK_EVENT_WRITE, HXR_OK);
                }
            }
        }
        break;
    case HX_SOCK_STATE_CONNECTING:
        HX_ASSERT(uEvent & HX_SOCK_EVENT_WRITE);
        if (uEvent & HX_SOCK_EVENT_WRITE)
        {
            UINT32 err = 0;
            hx_getsockopt(&m_sock, HX_SOCKOPT_SOCKERR, &err);
            HX_RESULT status = ErrorToStatus(err);

#if defined(MISSING_DUALSOCKET)
            if (SUCCEEDED(status))
            {
                if (m_pSock6 != NULL)
                {
                    // Signal IPv4 only mode
                    CHXSocket* pSock = m_pSock6->m_pSock4;
                    m_pSock6->m_pSock4 = NULL;
                    m_pSock6->Close();
                    m_pSock6->m_pSock4 = pSock;
                    m_pSock6 = NULL; // Weak reference
                }
                if (m_pSock4 != NULL)
                {
                    // Signal IPv6 only mode
                    m_pSock4->Close();
                    HX_RELEASE(m_pSock4);
                }
            }
#endif

            // NB: we shouldn't send a close event unless the connection is successful
            m_sock.state = HX_SOCK_STATE_NORMAL;
            m_bBlocked = TRUE;
            m_uForcedEventMask &= ~HX_SOCK_EVENT_WRITE;
            Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask);

            if ((m_uUserEventMask & HX_SOCK_EVENT_CONNECT) && m_pResponse != NULL)
            {
                m_pResponse->EventPending(HX_SOCK_EVENT_CONNECT, status);
            }
        }
        break;
    case HX_SOCK_STATE_LISTENING:
        HX_ASSERT(uEvent & HX_SOCK_EVENT_READ);
        if ((uEvent & HX_SOCK_EVENT_READ) && m_pResponse != NULL)
        {
            m_uPendingEventMask = HX_SOCK_EVENT_ACCEPT;
            m_pResponse->EventPending(HX_SOCK_EVENT_ACCEPT, HXR_OK);
        }
        break;
    default:
        HX_ASSERT(FALSE);
    };

    if (m_sock.state == HX_SOCK_STATE_CLOSED)
    {
        // NB: SelectEvents cannot be factored out because it resets evmask
        if ((m_uUserEventMask & HX_SOCK_EVENT_CLOSE) && m_pResponse != NULL)
        {
            SelectEvents(HX_SOCK_EVENT_NONE);
            m_pResponse->EventPending(HX_SOCK_EVENT_CLOSE, HXR_OK);
        }
        else
        {
            SelectEvents(HX_SOCK_EVENT_NONE);
        }
    }
    m_uPendingEventMask = 0;
    Release();
}

void
CHXSocket::ConnectEnumeratorDone(void)
{
    HX_RELEASE(m_pEnumerator);
}

STDMETHODIMP
CHXSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocket))
    {
        AddRef();
        *ppvObj = (IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXMulticastSocket))
    {
        AddRef();
        *ppvObj = (IHXMulticastSocket*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXSocket::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(HXSockFamily)
CHXSocket::GetFamily(void)
{
    return m_family;
}

STDMETHODIMP_(HXSockType)
CHXSocket::GetType(void)
{
    return m_type;
}

STDMETHODIMP_(HXSockProtocol)
CHXSocket::GetProtocol(void)
{
    return m_proto;
}

HX_RESULT
CHXSocket::DoClose(void)
{
    HX_RESULT hxr = HXR_OK;
    HX_RELEASE(m_pEnumerator);  //XXXTDM: need to cancel
    HX_RELEASE(m_pResponse);
    if (m_hIdleCallback)
    {
        m_pScheduler->Remove(m_hIdleCallback);
        m_hIdleCallback = 0;
    }
    m_uIdleTimeout = 0;
#if defined(MISSING_DUALSOCKET)
    HX_RELEASE(m_pSock4);
    m_pSock6 = NULL; // Weak reference
#endif
    if (HX_SOCK_VALID(m_sock))
    {
	// remove all the socket events we previousely interested
	m_uForcedEventMask = HX_SOCK_EVENT_NONE;
	SelectEvents(HX_SOCK_EVENT_NONE);
        if (hx_close(&m_sock) != 0)
        {
            hxr = ErrorToStatus(hx_lastsockerr());
        }
	m_sock.state = HX_SOCK_STATE_CLOSED;        
    }
    return hxr;
}

HX_RESULT
CHXSocket::DoInit(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    if (hx_socket(&m_sock, f, t, p) != 0)
    {
        return ErrorToStatus(hx_lastsockerr());
    }

#if defined(MISSING_DUALSOCKET)
    HX_ASSERT(m_pSock4 == NULL);
    if (f == HX_SOCK_FAMILY_IN6)
    {
        HX_RESULT hxr = m_pNetSvc->CreateSocket((IHXSocket**)&m_pSock4);
        m_pSock4->SetAccessControl(m_pAccessControl);
        if (SUCCEEDED(hxr))
        {
            hxr = m_pSock4->Init(HX_SOCK_FAMILY_IN4, t, p);
            if (SUCCEEDED(hxr))
            {
                m_pSock4->m_pSock6 = this; // Weak reference
                if (m_pResponse != NULL)
                {
                    m_pSock4->SetResponse(m_pResponse);
                }
            }
            else
            {
                m_pSock4->Close();
                HX_RELEASE(m_pSock4);
                hx_close(&m_sock);
                return hxr;
            }
        }
    }
#endif

    m_family = f;
    m_type = t;
    m_proto = p;

    if (IS_STREAM_TYPE(m_type))
    {
        m_pWriteQueue = new CHXStreamWriteQueue();
    }
    else
    {
        m_pWriteQueue = new CHXDatagramWriteQueue();
    }
    m_pWriteQueue->SetSize(MAX_IP_PACKET);

    return HXR_OK;
}

STDMETHODIMP
CHXSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    if (HX_SOCK_VALID(m_sock))
    {
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }

    HX_RESULT hxr = m_netDrvLoader.EnsureLoaded(m_punkContext);
    if (FAILED(hxr))
    {
        return hxr;
    }

    if (f == HX_SOCK_FAMILY_INANY)
    {
        hxr = DoInit(HX_SOCK_FAMILY_IN6, t, p);
        if (hxr == HXR_SOCK_AFNOSUPPORT)
        {
            hxr = DoInit(HX_SOCK_FAMILY_IN4, t, p);
        }
        return hxr;
    }
    else
    {
        hxr = DoInit(f, t, p);
    }

    return hxr;
}

STDMETHODIMP
CHXSocket::SetResponse(IHXSocketResponse* pResponse)
{
    HX_RELEASE(m_pResponse);
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        m_pSock4->SetResponse(pResponse);
    }
#endif
    return HXR_OK;
}

STDMETHODIMP
CHXSocket::SetAccessControl(IHXSocketAccessControl* pControl)
{
    HX_RELEASE(m_pAccessControl);
    m_pAccessControl = pControl;
    HX_ADDREF(m_pAccessControl);
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        m_pSock4->SetAccessControl(pControl);
    }
#endif
    return HXR_OK;
}

STDMETHODIMP
CHXSocket::CreateSockAddr(IHXSockAddr** ppAddr)
{
    return m_pNetSvc->CreateSockAddr(m_family, ppAddr);
}

STDMETHODIMP
CHXSocket::Bind(IHXSockAddr* pAddr)
{
    HX_RESULT hxr = HXR_OK;
    sockaddr_storage ss;
    sockaddr* psa;
    size_t salen;

    hxr = GetNativeAddr(pAddr, &ss, &psa, &salen);
    if (SUCCEEDED(hxr))
    {
#if defined(MISSING_DUALSOCKET)
        if (m_pSock4 != NULL && psa->sa_family == AF_INET6)
        {
            sockaddr_in sa4;
            sockaddr_in6* psa6 = (sockaddr_in6*)psa;
            // Accept IN6_ANY or V4MAPPED (but not V4MAPPED INADDR_ANY)
            if (IN6_IS_ADDR_V4MAPPED(&psa6->sin6_addr))
            {
                hx_map6to4(psa, &sa4);
                if (sa4.sin_addr.s_addr == INADDR_ANY)
                {
                    return HXR_FAIL;
                }
                if (hx_bind(&m_pSock4->m_sock, (sockaddr*)&sa4, sizeof(sa4)) != 0)
                {
                    return ErrorToStatus(hx_lastsockerr());
                }

                if (m_pSock4->m_pSock6 != NULL)
                {
                    // Binding is IPv4 only, close IPv6 side and return
                    //Following is done to close only ipv6 socket and keep ip4 socket as is.
                    //It is required to call m_pSock6->Close to remove socket descriptor
                    //from callbacks set.
                    HX_ASSERT(m_pSock4->m_pSock6 == this);
                    CHXSocket* pSock = m_pSock4;
                    IHXSocketResponse* pResponse = m_pResponse;
                    m_pSock4 = NULL;
                    m_pResponse = NULL;
                    Close();
                    m_pSock4 = pSock; //restore m_pSock4. Did this to prevent closing of ipv4 socket by Close.
                    m_pResponse = pResponse; //Restore response pointer, lost by Close
                    m_pSock4->m_pSock6 = NULL;
                }
                return HXR_OK;
            }
            if (IN6_IS_ADDR_UNSPECIFIED(&psa6->sin6_addr) && !m_bV6ONLY)
            {
                memset(&sa4, 0, sizeof(sockaddr_in));
                sa4.sin_family = AF_INET;
                sa4.sin_port = psa6->sin6_port;
                if (hx_bind(&m_pSock4->m_sock, (sockaddr*)&sa4, sizeof(sa4)) != 0)
                {
                    return ErrorToStatus(hx_lastsockerr());
                }
                // Binding is dual, fall through
            }
        }
#endif
        if (hx_bind(&m_sock, psa, salen) == 0)
        {
            hxr = HXR_OK;
        }
        else
        {
            hxr = ErrorToStatus(hx_lastsockerr());
        }
    }
    return hxr;
}

STDMETHODIMP
CHXSocket::ConnectToOne(IHXSockAddr* pAddr)
{
    HX_RESULT hxr = HXR_OK;
    sockaddr_storage ss;
    sockaddr sa_unspec;
    sockaddr* psa;
    size_t salen;

    /* we allow use of a NULL pAddr to "disconnect" UDP sockets
     * Posix.1g states that this can be done by setting the family
     * to AF_UNSPEC (the connect call may or may not return an
     * EAFNOSUPPORT error) */
    if (pAddr == NULL)
    {
        if (m_type != HX_SOCK_TYPE_UDP)
        {
            return HXR_UNEXPECTED;
        }

#if defined(MISSING_DUALSOCKET)
        // do the "disconnect" on m_pSock4 if we have closed the IPv6 side
        if (!HX_SOCK_VALID(m_sock) &&
             m_pSock4 != NULL && HX_SOCK_VALID(m_pSock4->m_sock))
        {
            return m_pSock4->ConnectToOne(pAddr);
        }
#endif

        memset(&sa_unspec, 0, sizeof(sa_unspec));
        sa_unspec.sa_family = AF_UNSPEC;

        psa = &sa_unspec;
        salen = sizeof(sa_unspec);
    }
    else   // pAddr != NULL, normal connect, get address from pAddr argument
    {
#if defined(MISSING_DUALSOCKET)
        if (m_pSock4 != NULL && RequiresSock4(pAddr) && !m_bV6ONLY)
        {
            // address is IPv4 or IPv6 V4MAPPED
            return m_pSock4->ConnectToOne(pAddr);
        }
#endif

        hxr = GetNativeAddr(pAddr, &ss, &psa, &salen);
    }

    if (SUCCEEDED(hxr))
    {
        switch (hx_connect(&m_sock, psa, salen))
        {
        case 0: // Succeeded
#if defined(MISSING_DUALSOCKET)
            /* If we reach here and m_pSock6 is not NULL, we know this is an
             * IPv4 or IPv4-mapped IPv6 address so we close m_pSock6
             */
            if (m_pSock6 != NULL)
            {
                //Following is done to close only ipv6 socket and keep ip4 socket as is.
                //It is required to call m_pSock6->Close to remove socket descriptor
                //from the callbacks set.
                CHXSocket* pSock = m_pSock6->m_pSock4;
                IHXSocketResponse* pResponse = m_pResponse;
                m_pSock6->m_pSock4 = NULL;
                m_pSock6->m_pResponse = NULL;
                m_pSock6->Close();
                m_pSock6->m_pSock4 = pSock;
                m_pSock6->m_pResponse = pResponse; //Restore response pointer, lost by Close
                m_pSock6 = NULL; // Weak reference
            }

            /* On the other hand, if  m_pSock4 is not NULL, we know this is an
             * IPv6 address and we can close m_pSock4
             */
            if (m_pSock4 != NULL)
            {
                // Signal IPv6 only mode
                m_pSock4->Close();
                HX_RELEASE(m_pSock4);
            }
#endif
            if ((m_uUserEventMask & HX_SOCK_EVENT_CONNECT) &&
                m_pResponse != NULL)
            {
                m_bBlocked = TRUE;
                m_pResponse->EventPending(HX_SOCK_EVENT_CONNECT, HXR_OK);
            }
            hxr = HXR_OK;
            break;
        case 1: // In progress
            m_uForcedEventMask |= HX_SOCK_EVENT_WRITE;
            Select(m_uForcedEventMask);
            hxr = HXR_OK; //HXR_SOCK_INPROGRESS;
            break;
        default:
            hxr = ErrorToStatus(hx_lastsockerr());
        }
    }

    return hxr;
}
STDMETHODIMP
CHXSocket::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    if (m_pEnumerator != NULL)
    {
        return HXR_SOCK_INPROGRESS;
    }
    m_pEnumerator = new CHXSocketConnectEnumerator(this);
    m_pEnumerator->AddRef();
    return m_pEnumerator->Init(nVecLen, ppAddrVec);
}

STDMETHODIMP
CHXSocket::GetLocalAddr(IHXSockAddr** ppAddr)
{
#if defined(MISSING_DUALSOCKET)
    // Return IPv4 addr only if we have closed the IPv6 side
    if (!HX_SOCK_VALID(m_sock) &&
         m_pSock4 != NULL && HX_SOCK_VALID(m_pSock4->m_sock))
    {
        return m_pSock4->GetLocalAddr(ppAddr);
    }
#endif

    HX_RESULT hxr = HXR_OK;
    IHXSockAddrNative* pNative = NULL;
    sockaddr* psa;
    size_t salen;

    hxr = m_pNetSvc->CreateSockAddr(m_family, ppAddr);
    if (SUCCEEDED(hxr))
    {
        hxr = (*ppAddr)->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
        if (SUCCEEDED(hxr))
        {
            pNative->Get(&psa, &salen);
            if (hx_getsockaddr(&m_sock, psa, salen) != 0)
            {
                HX_RELEASE(*ppAddr);
                hxr = ErrorToStatus(hx_lastsockerr());
            }
            HX_RELEASE(pNative);
        }
    }

    return hxr;
}

STDMETHODIMP
CHXSocket::GetPeerAddr(IHXSockAddr** ppAddr)
{
#if defined(MISSING_DUALSOCKET)
    // Return IPv4 addr only if we have closed the IPv6 side
    if (!HX_SOCK_VALID(m_sock) &&
         m_pSock4 != NULL && HX_SOCK_VALID(m_pSock4->m_sock))
    {
        return m_pSock4->GetPeerAddr(ppAddr);
    }
#endif

    HX_RESULT hxr = HXR_OK;
    IHXSockAddrNative* pNative = NULL;
    sockaddr* psa;
    size_t salen;

    hxr = m_pNetSvc->CreateSockAddr(m_family, ppAddr);
    if (SUCCEEDED(hxr))
    {
        hxr = (*ppAddr)->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
        if (SUCCEEDED(hxr))
        {
            pNative->Get(&psa, &salen);
            if (hx_getpeeraddr(&m_sock, psa, salen) != 0)
            {
                HX_RELEASE(*ppAddr);
                hxr = ErrorToStatus(hx_lastsockerr());
            }
            HX_RELEASE(pNative);
        }
    }

    return hxr;
}

STDMETHODIMP
CHXSocket::SelectEvents(UINT32 uEventMask)
{
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4)
    {
        m_pSock4->SelectEvents(uEventMask);
    }
#endif

    m_uAllowedEventMask = HX_SOCK_EVENT_ALL;
    m_uUserEventMask = uEventMask;
    return Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask, FALSE);
}

STDMETHODIMP
CHXSocket::Peek(IHXBuffer** ppBuf)
{
    return DoRead(ppBuf, NULL, TRUE);
}

STDMETHODIMP
CHXSocket::Read(IHXBuffer** ppBuf)
{
    return DoRead(ppBuf, NULL);
}

STDMETHODIMP
CHXSocket::Write(IHXBuffer* pBuf)
{
    return DoWrite(pBuf, NULL);
}

STDMETHODIMP
CHXSocket::Close(void)
{
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        m_pSock4->Close();
    }
#endif
    if (HX_SOCK_VALID(m_sock) && !m_pWriteQueue->IsEmpty())
    {
        UINT32 uLinger;
        if (hx_getsockopt(&m_sock, HX_SOCKOPT_LINGER, &uLinger) != 0)
        {
            HX_ASSERT(FALSE);
            uLinger = LINGER_OFF;
        }
        if (uLinger == LINGER_OFF)
        {
            // Flush write queue before closing socket
            AddRef();
            m_sock.state = HX_SOCK_STATE_LINGERING;
            m_uForcedEventMask |= HX_SOCK_EVENT_WRITE;
            Select(m_uForcedEventMask);
            return HXR_OK;
        }
    }
    return DoClose();
}

STDMETHODIMP
CHXSocket::Listen(UINT32 uBackLog)
{
    HX_RESULT hxr = HXR_OK;
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL && HX_SOCK_VALID(m_pSock4->m_sock))
    {
        hxr = m_pSock4->Listen(uBackLog);
        if (FAILED(hxr))
        {
            return hxr;
        }
        // If we are in IPv4 only mode, we are done
        if (!HX_SOCK_VALID(m_sock))
        {
            return hxr;
        }
    }
#endif
    if (hx_listen(&m_sock, uBackLog) != 0)
    {
        hxr = ErrorToStatus(hx_lastsockerr());
    }
    return HXR_OK;
}

STDMETHODIMP
CHXSocket::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    HX_RESULT hxr = HXR_OK;
    IHXSockAddrNative* pNative = NULL;
    sockaddr* psa;
    size_t salen;
    HX_SOCK snew;

#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        if (m_pSock4->m_uPendingEventMask & HX_SOCK_EVENT_ACCEPT)
        {
            return m_pSock4->Accept(ppNewSock, ppSource);
        }
    }
#endif

    hxr = m_pNetSvc->CreateSockAddr(m_family, ppSource);
    if (SUCCEEDED(hxr))
    {
        hxr = (*ppSource)->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    }
    if (SUCCEEDED(hxr))
    {
        pNative->Get(&psa, &salen);
        if (hx_accept(&m_sock, &snew, psa, salen) == 0)
        {
            hxr = m_pNetSvc->CreateSocket(m_family, m_type, m_proto, snew, ppNewSock);
            if (FAILED(hxr))
            {
                hx_close(&snew);
            }
        }
        else
        {
            hxr = ErrorToStatus(hx_lastsockerr());
        }
    }
    HX_RELEASE(pNative);

    if (SUCCEEDED(hxr) && m_pAccessControl != NULL)
    {
        IHXSockAddr* pLocalAddr = NULL;
        (*ppNewSock)->GetLocalAddr(&pLocalAddr);
        hxr = m_pAccessControl->AddressCheck(pLocalAddr, *ppSource);
        if (FAILED(hxr))
        {
            (*ppNewSock)->Close();
            HX_RELEASE(*ppSource);
            HX_RELEASE(*ppNewSock);
        }
        HX_RELEASE(pLocalAddr);
    }

    return hxr;
}

STDMETHODIMP
CHXSocket::GetOption(HXSockOpt name, UINT32* pval)
{
#if defined(MISSING_DUALSOCKET)
    if (name == HX_SOCKOPT_IN6_V6ONLY)
    {
        if (m_family != HX_SOCK_FAMILY_IN6)
        {
            return HXR_SOCK_PROTONOSUPPORT; //XXX: Verify this
        }
        *pval = (UINT32)m_bV6ONLY;
        return HXR_OK;
    }
    if (m_pSock4 != NULL && HX_SOCK_VALID(m_pSock4->m_sock))
    {
        HX_RESULT hxr = m_pSock4->GetOption(name, pval);
        if (FAILED(hxr))
        {
            return hxr;
        }
        if (!HX_SOCK_VALID(m_sock))
        {
            return hxr;
        }
    }
#endif


    switch (name)
    {
    case HX_SOCKOPT_APP_IDLETIMEOUT:
        *pval = m_uIdleTimeout;
        return HXR_OK;
    case HX_SOCKOPT_APP_BUFFER_TYPE:
        *pval = m_bufType;
        return HXR_OK;
    case HX_SOCKOPT_APP_READBUF_ALLOC:
        *pval = m_readBufAlloc;
        return HXR_OK;
    case HX_SOCKOPT_APP_READBUF_MAX:
        *pval = m_mss;
        return HXR_OK;
    case HX_SOCKOPT_APP_SNDBUF:
        *pval = 0;
        if (m_pWriteQueue == NULL)
        {
            return HXR_UNEXPECTED;
        }
        *pval = m_pWriteQueue->GetSize();
        return HXR_OK;
    case HX_SOCKOPT_APP_AGGLIMIT:
        *pval = m_uAggLimit;
        return HXR_OK;
    default:
        // pass
        break;
    }


    if (hx_getsockopt(&m_sock, name, pval) != 0)
    {
        return ErrorToStatus(hx_lastsockerr());
    }
    return HXR_OK;
}

STDMETHODIMP
CHXSocket::SetOption(HXSockOpt name, UINT32 val)
{
#if defined(MISSING_DUALSOCKET)
    if (name == HX_SOCKOPT_IN6_V6ONLY)
    {
        if (m_family != HX_SOCK_FAMILY_IN6)
        {
            return HXR_SOCK_PROTONOSUPPORT; //XXX: Verify this
        }
        m_bV6ONLY = (HXBOOL)val;
        return HXR_OK;
    }
    if (m_pSock4 != NULL && HX_SOCK_VALID(m_pSock4->m_sock))
    {
        HX_RESULT hxr = m_pSock4->SetOption(name, val);
        if (FAILED(hxr))
        {
            return hxr;
        }
        if (!HX_SOCK_VALID(m_sock))
        {
            return hxr;
        }
    }
#endif

    switch (name)
    {
    case HX_SOCKOPT_RCVBUF:
        if (val == HX_SOCK_RCVBUF_MAX)
        {
            HX_RESULT hxr;

            // Try to find the largest allowed value between 1M and 64K
            val = 0x100000;
            do
            {
                hxr = (hx_setsockopt(&m_sock, name, val) == 0)
                    ? HXR_OK : ErrorToStatus(hx_lastsockerr());
                val >>= 1;
            }
            while (val > 0x10000 && FAILED(hxr));

            return HXR_OK;
        }
        break;
    case HX_SOCKOPT_APP_IDLETIMEOUT:
        HX_ASSERT(m_pScheduler);
        if (m_pScheduler == NULL)
        {
            return HXR_FAIL;
        }
        if (m_hIdleCallback != 0)
        {
            m_pScheduler->Remove(m_hIdleCallback);
            m_hIdleCallback = 0;
        }
        m_uIdleTimeout = val;
        if (m_uIdleTimeout != 0)
        {
            m_hIdleCallback = m_pScheduler->RelativeEnter(this, m_uIdleTimeout*1000);
        }
        return HXR_OK;
    case HX_SOCKOPT_APP_BUFFER_TYPE:
        if (val != HX_SOCKBUF_DEFAULT &&
            val != HX_SOCKBUF_TIMESTAMPED)
        {
            return HXR_FAIL;
        }
        m_bufType = (HXSockBufType)val;
        return HXR_OK;
    case HX_SOCKOPT_APP_READBUF_ALLOC:
        if (val != HX_SOCK_READBUF_SIZE_DEFAULT &&
            val != HX_SOCK_READBUF_SIZE_COPY)
        {
            return HXR_FAIL;
        }
        m_readBufAlloc = (HXSockReadBufAlloc)val;
        return HXR_OK;
    case HX_SOCKOPT_APP_READBUF_MAX:
        m_mss = val;
        return HXR_OK;
    case HX_SOCKOPT_APP_SNDBUF:
        if (m_pWriteQueue == NULL)
        {
            return HXR_UNEXPECTED;
        }
        return m_pWriteQueue->SetSize(val);
    case HX_SOCKOPT_APP_AGGLIMIT:
        m_uAggLimit = val;
        return HXR_OK;
    default:
        // pass
        break;
    }


    if (hx_setsockopt(&m_sock, name, val) != 0)
    {
        return ErrorToStatus(hx_lastsockerr());
    }
    return HXR_OK;
}

STDMETHODIMP
CHXSocket::PeekFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    return DoRead(ppBuf, ppAddr, TRUE);
}

STDMETHODIMP
CHXSocket::ReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    return DoRead(ppBuf, ppAddr);
}

STDMETHODIMP
CHXSocket::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    return DoWrite(pBuf, pAddr);
}

STDMETHODIMP
CHXSocket::ReadV(UINT32 nVecLen, UINT32* puLenVec,
                IHXBuffer** ppBufVec)
{
    return DoReadV(nVecLen, puLenVec, ppBufVec, NULL);
}

STDMETHODIMP
CHXSocket::ReadFromV(UINT32 nVecLen, UINT32* puLenVec,
                IHXBuffer** ppBufVec, IHXSockAddr** ppAddr)
{
    return DoReadV(nVecLen, puLenVec, ppBufVec, ppAddr);
}

STDMETHODIMP
CHXSocket::WriteV(UINT32 nVecLen, IHXBuffer** ppBufVec)
{
    return DoWriteV(nVecLen, ppBufVec, NULL);
}

STDMETHODIMP
CHXSocket::WriteToV(UINT32 nVecLen, IHXBuffer** ppBufVec, IHXSockAddr* pAddr)
{
    return DoWriteV(nVecLen, ppBufVec, pAddr);
}


HX_RESULT
CHXSocket::DoMulticastGroupOp(IHXSockAddr* pGroupAddr,IHXSockAddr* pInterface, HXBOOL bJoin)
{
    if (!pGroupAddr)
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        if (RequiresSock4(pGroupAddr))
        {
            // Join group/interface on ipv4 socket
            return m_pSock4->DoMulticastGroupOp(pGroupAddr, pInterface, bJoin);
        }
    }
#endif

    if (pInterface != NULL && pInterface->GetFamily() != pGroupAddr->GetFamily())
    {
        // Interface and group families must match
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    // Ensure addresses are converted to match this socket's family
    IHXSockAddr* pGroupAddrConverted = NULL;
    IHXSockAddr* pInterfaceAddrConverted = NULL;
    HX_RESULT hxr = HXSockUtil::ConvertAddr(m_pNetSvc, m_family, pGroupAddr, pGroupAddrConverted);
    if (FAILED(hxr))
    {
        HX_ASSERT(FALSE);
        return hxr;
    }
    if (pInterface)
    {
        hxr = HXSockUtil::ConvertAddr(m_pNetSvc, m_family, pInterface, pInterfaceAddrConverted);
        if (FAILED(hxr))
        {
            HX_RELEASE(pGroupAddrConverted);
            HX_ASSERT(FALSE);
            return hxr;
        }
    }

    if (HX_SOCK_FAMILY_IN4 == m_family)
    {
        ip_mreq req;
        hxr = SetMulticastInfoIN4(pGroupAddrConverted, pInterfaceAddrConverted, req);
        if(SUCCEEDED(hxr))
        {
            UINT32 optionName = bJoin ? HX_SOCKOPT_IN4_ADD_MEMBERSHIP : HX_SOCKOPT_IN4_DROP_MEMBERSHIP;
            if (hx_setsockopt(&m_sock, optionName, &req, sizeof(req)) != 0)
            {
                hxr = ErrorToStatus(hx_lastsockerr());
            }
        }
    }
    else if (HX_SOCK_FAMILY_IN6 == m_family)
    {
        ipv6_mreq req;
        hxr = SetMulticastInfoIN6(pGroupAddrConverted, pInterfaceAddrConverted, req);
        if(SUCCEEDED(hxr))
        {
            UINT32 optionName = bJoin ? HX_SOCKOPT_IN6_JOIN_GROUP : HX_SOCKOPT_IN6_LEAVE_GROUP;
            if (hx_setsockopt(&m_sock, HX_SOCKOPT_IN6_JOIN_GROUP, &req, sizeof(req)) != 0)
            {
                hxr = ErrorToStatus(hx_lastsockerr());
            }
        }
    }
    else
    {
        HX_ASSERT(FALSE);
        hxr = HXR_FAIL; // 'unsupported family'
    }

    HX_RELEASE(pGroupAddrConverted);
    HX_RELEASE(pInterfaceAddrConverted);

    return hxr;
}

STDMETHODIMP
CHXSocket::JoinGroup(IHXSockAddr* pGroupAddr,
                     IHXSockAddr* pInterface)
{
    return DoMulticastGroupOp(pGroupAddr, pInterface, TRUE /*join*/);
}

STDMETHODIMP
CHXSocket::LeaveGroup(IHXSockAddr* pGroupAddr,
                      IHXSockAddr* pInterface)
{
    return DoMulticastGroupOp(pGroupAddr, pInterface, FALSE /*leave*/);
}

STDMETHODIMP
CHXSocket::SetSourceOption(HXMulticastSourceOption flag,
                IHXSockAddr* pSourceAddr,
                IHXSockAddr* pGroupAddr,
                IHXSockAddr* pInterface)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXSocket::Func(void)
{
    time_t now;
    UINT32 uNextSched;
    time(&now);
    if (m_tLastPacket+(time_t)m_uIdleTimeout < now)
    {
        if (m_pResponse != NULL)
        {
            m_pResponse->EventPending(HX_SOCK_EVENT_ERROR, HXR_SOCK_TIMEDOUT);
        }
        uNextSched = m_uIdleTimeout*1000;
    }
    else
    {
        uNextSched = (m_tLastPacket+m_uIdleTimeout-now)*1000;
    }
    HX_ASSERT(m_uIdleTimeout != 0);
    m_hIdleCallback = m_pScheduler->RelativeEnter(this, uNextSched);
    return HXR_OK;
}

HX_RESULT
CHXSocket::ErrorToStatus(int err)
{
    switch (err)
    {
    case SOCKERR_NONE:                  return HXR_OK;
    case SOCKERR_INTR:                  return HXR_SOCK_INTR;
    case SOCKERR_BADF:                  return HXR_SOCK_BADF;
    case SOCKERR_ACCES:                 return HXR_SOCK_ACCES;
    case SOCKERR_FAULT:                 return HXR_SOCK_FAULT;
    case SOCKERR_INVAL:                 return HXR_SOCK_INVAL;
    case SOCKERR_MFILE:                 return HXR_SOCK_MFILE;
    case SOCKERR_WOULDBLOCK:            return HXR_SOCK_WOULDBLOCK;
    case SOCKERR_INPROGRESS:            return HXR_SOCK_INPROGRESS;
    case SOCKERR_ALREADY:               return HXR_SOCK_ALREADY;
    case SOCKERR_NOTSOCK:               return HXR_SOCK_NOTSOCK;
    case SOCKERR_DESTADDRREQ:           return HXR_SOCK_DESTADDRREQ;
    case SOCKERR_MSGSIZE:               return HXR_SOCK_MSGSIZE;
    case SOCKERR_PROTOTYPE:             return HXR_SOCK_PROTOTYPE;
    case SOCKERR_NOPROTOOPT:            return HXR_SOCK_NOPROTOOPT;
    case SOCKERR_PROTONOSUPPORT:        return HXR_SOCK_PROTONOSUPPORT;
    case SOCKERR_SOCKTNOSUPPORT:        return HXR_SOCK_SOCKTNOSUPPORT;
    case SOCKERR_OPNOTSUPP:             return HXR_SOCK_OPNOTSUPP;
    case SOCKERR_PFNOSUPPORT:           return HXR_SOCK_PFNOSUPPORT;
    case SOCKERR_AFNOSUPPORT:           return HXR_SOCK_AFNOSUPPORT;
    case SOCKERR_ADDRINUSE:             return HXR_SOCK_ADDRINUSE;
    case SOCKERR_ADDRNOTAVAIL:          return HXR_SOCK_ADDRNOTAVAIL;
    case SOCKERR_NETDOWN:               return HXR_SOCK_NETDOWN;
    case SOCKERR_NETUNREACH:            return HXR_SOCK_NETUNREACH;
    case SOCKERR_NETRESET:              return HXR_SOCK_NETRESET;
    case SOCKERR_CONNABORTED:           return HXR_SOCK_CONNABORTED;
    case SOCKERR_CONNRESET:             return HXR_SOCK_CONNRESET;
    case SOCKERR_NOBUFS:                return HXR_SOCK_NOBUFS;
    case SOCKERR_ISCONN:                return HXR_SOCK_ISCONN;
    case SOCKERR_NOTCONN:               return HXR_SOCK_NOTCONN;
    case SOCKERR_SHUTDOWN:              return HXR_SOCK_SHUTDOWN;
    case SOCKERR_TOOMANYREFS:           return HXR_SOCK_TOOMANYREFS;
    case SOCKERR_TIMEDOUT:              return HXR_SOCK_TIMEDOUT;
    case SOCKERR_CONNREFUSED:           return HXR_SOCK_CONNREFUSED;
    case SOCKERR_LOOP:                  return HXR_SOCK_LOOP;
    case SOCKERR_NAMETOOLONG:           return HXR_SOCK_NAMETOOLONG;
    case SOCKERR_HOSTDOWN:              return HXR_SOCK_HOSTDOWN;
    case SOCKERR_HOSTUNREACH:           return HXR_SOCK_HOSTUNREACH;
#if defined(_WIN32)
    case SOCKERR_NOTEMPTY:              return HXR_SOCK_NOTEMPTY;
    case SOCKERR_PROCLIM:               return HXR_SOCK_PROCLIM ;
    case SOCKERR_USERS:                 return HXR_SOCK_USERS;
    case SOCKERR_DQUOT:                 return HXR_SOCK_DQUOT;
    case SOCKERR_STALE:                 return HXR_SOCK_STALE;
    case SOCKERR_REMOTE:                return HXR_SOCK_REMOTE;
    case SOCKERR_SYSNOTREADY:           return HXR_SOCK_SYSNOTREADY;
    case SOCKERR_VERNOTSUPPORTED:       return HXR_SOCK_VERNOTSUPPORTED;
    case SOCKERR_NOTINITIALISED:        return HXR_SOCK_NOTINITIALISED;
    case SOCKERR_DISCON:                return HXR_SOCK_DISCON;
    case SOCKERR_NOMORE:                return HXR_SOCK_NOMORE;
    case SOCKERR_CANCELLED:             return HXR_SOCK_CANCELLED;
    case SOCKERR_INVALIDPROCTABLE:      return HXR_SOCK_INVALIDPROCTABLE;
    case SOCKERR_INVALIDPROVIDER:       return HXR_SOCK_INVALIDPROVIDER;
    case SOCKERR_PROVIDERFAILEDINIT:    return HXR_SOCK_PROVIDERFAILEDINIT;
    case SOCKERR_SYSCALLFAILURE:        return HXR_SOCK_SYSCALLFAILURE;
    case SOCKERR_SERVICE_NOT_FOUND:     return HXR_SOCK_SERVICE_NOT_FOUND;
    case SOCKERR_TYPE_NOT_FOUND:        return HXR_SOCK_TYPE_NOT_FOUND;
    case SOCKERR_E_NO_MORE:             return HXR_SOCK_E_NO_MORE;
    case SOCKERR_E_CANCELLED:           return HXR_SOCK_E_CANCELLED;
    case SOCKERR_REFUSED:               return HXR_SOCK_REFUSED;
    case SOCKERR_HOST_NOT_FOUND:        return HXR_SOCK_HOST_NOT_FOUND;
    case SOCKERR_TRY_AGAIN:             return HXR_SOCK_TRY_AGAIN;
    case SOCKERR_NO_RECOVERY:           return HXR_SOCK_NO_RECOVERY;
    case SOCKERR_NO_DATA:               return HXR_SOCK_NO_DATA;
#endif /* defined(_WIN32) */

#if defined(_UNIX)
    case SOCKERR_PIPE:                  return HXR_SOCK_PIPE;
#endif
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_UNEXPECTED;
}

/*
 * This method takes an IHXSockAddr and fills in the kernel level sockaddr
 * parameters for use with the netdrv functions.  It converts between IPv4
 * and IPv6 as appropriate.
 *
 * pss       - provides space for converted data when needed (pass stack-allocated variable)
 *
 * *ppsa     - updated to point to sockaddr*
 * *psalen   - updated to point to size_t
 *
 * A converion occurs if
 *
 * a) the socket is IPv4 and the address is IPv6 V4MAPPED
 * b) the socket is IPv6 and the address is IPv4
 *
 */
HX_RESULT
CHXSocket::GetNativeAddr(IHXSockAddr* pAddr,
                        sockaddr_storage* pss,
                        sockaddr** ppsa,
                        size_t* psalen)
{
    HX_ASSERT(pss);
    IHXSockAddrNative* pNative = NULL;
    HX_RESULT hxr = pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    if (SUCCEEDED(hxr))
    {
        pNative->Get(ppsa, psalen);

        switch (m_family)
        {
        case HX_SOCK_FAMILY_IN4:
            if ((*ppsa)->sa_family == AF_INET6)
            {
                sockaddr_in* psa4 = (struct sockaddr_in*) pss;
                if (hx_map6to4(*ppsa, psa4))
                {
                    *ppsa = (sockaddr*) pss;
                    *psalen = sizeof(sockaddr_in);
                }
                else
                {
                    hxr = HXR_SOCK_AFNOSUPPORT;
                }
            }
            else if ((*ppsa)->sa_family != AF_INET)
            {
                // Can't convert to IN4 addr
                hxr = HXR_SOCK_AFNOSUPPORT;
            }
            break;
        case HX_SOCK_FAMILY_IN6:
            if ((*ppsa)->sa_family == AF_INET)
            {
                sockaddr_in6* psa6 =  (sockaddr_in6*) pss;
                hx_map4to6(*ppsa, psa6);

                *ppsa = (sockaddr*) pss;
                *psalen = sizeof(sockaddr_in6);
            }
            else if ((*ppsa)->sa_family != AF_INET6)
            {
                // Can't convert to IN6 addr
                hxr = HXR_SOCK_AFNOSUPPORT;
            }
            break;
        default:
            // No conversion
            break;
        }
        HX_RELEASE(pNative);
    }
    return hxr;
}


HX_RESULT
CHXSocket::SetReadBufferSize(IHXBuffer** ppBuf /*modified*/, UINT32 cbRead)
{
    HX_ASSERT(ppBuf && *ppBuf);
    HX_ASSERT(cbRead != 0);

    HX_RESULT hxr = HXR_OK;

    //XXXLCM If we get rid of the following line and 'copy' is specified we can allocate
    //       one temp buffer into which we read and avoid some extra allocations.
    if (cbRead != (*ppBuf)->GetSize())
    {
        HX_ASSERT(cbRead < (*ppBuf)->GetSize());
        if (HX_SOCK_READBUF_SIZE_COPY == m_readBufAlloc)
        {
            // allocate a new buffer so buffer does not waste unused memory space
            IHXBuffer* pResizedBuf = 0;
            hxr = CreateReadBuffer(&pResizedBuf);
            if (HXR_OK == hxr)
            {
                hxr = pResizedBuf->Set((*ppBuf)->GetBuffer(), cbRead);
                if (HXR_OK == hxr)
                {
                    HX_RELEASE(*ppBuf);
                    *ppBuf = pResizedBuf;
                    (*ppBuf)->AddRef();
                }
                HX_RELEASE(pResizedBuf);
            }
        }
        else
        {
            HX_ASSERT(HX_SOCK_READBUF_SIZE_DEFAULT == m_readBufAlloc);
            // just re-specify the size and avoid a copy
            hxr = (*ppBuf)->SetSize(cbRead);
        }
    }

    return hxr;
}


HX_RESULT
CHXSocket::CreateReadBuffer(IHXBuffer** ppBuf)
{
    HX_RESULT hxr = HXR_OK;
    IHXTimeStampedBuffer* pTSBuf = NULL;
    switch (m_bufType)
    {
    case HX_SOCKBUF_DEFAULT:
        hxr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)ppBuf);
        break;
    case HX_SOCKBUF_TIMESTAMPED:
        hxr = m_pCCF->CreateInstance(CLSID_IHXTimeStampedBuffer, (void**)&pTSBuf);
        HX_ASSERT(SUCCEEDED(hxr));
        if (SUCCEEDED(hxr))
        {
            pTSBuf->SetTimeStamp(HX_GET_TICKCOUNT_IN_USEC());
            hxr = pTSBuf->QueryInterface(IID_IHXBuffer, (void**)ppBuf);
            HX_ASSERT(hxr == HXR_OK);
            pTSBuf->Release();
        }
        break;
    default:
        HX_ASSERT(FALSE);
        hxr = HXR_UNEXPECTED;
        break;
    }
    return hxr;
}

HX_RESULT
CHXSocket::DoRead(IHXBuffer** ppBuf, IHXSockAddr** ppAddr /* = NULL */,
                HXBOOL bPeek /* = FALSE */)
{
    HX_RESULT hxr = HXR_OK;
    sockaddr* psa = NULL;
    size_t salen = 0;
    ssize_t n = 0;

#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        hxr = m_pSock4->DoRead(ppBuf, ppAddr, bPeek);
        if (hxr != HXR_SOCK_WOULDBLOCK || !HX_SOCK_VALID(m_sock))
        {
            return hxr;
        }
    }
#endif

    if (m_pTotalNetReaders)
    {
        InterlockedIncrement(m_pTotalNetReaders);
    }

    if (m_pCCF == NULL || ppBuf == NULL || *ppBuf != NULL)
    {
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }

    if (ppAddr != NULL)
    {
        HX_ASSERT(*ppAddr == NULL);
        IHXSockAddrNative* pNative = NULL;
        hxr = m_pNetSvc->CreateSockAddr(m_family, ppAddr);
        if (FAILED(hxr))
        {
            return hxr;
        }
        hxr = (*ppAddr)->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
        if (FAILED(hxr))
        {
            return hxr;
        }
        pNative->Get(&psa, &salen);
        HX_RELEASE(pNative);
    }

    *ppBuf = NULL;
     m_uAllowedEventMask |= (HX_SOCK_EVENT_READ |HX_SOCK_EVENT_ACCEPT |HX_SOCK_EVENT_CLOSE);
    Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask);

    if (m_mss == 0)
    {
        hx_getsockopt(&m_sock, HX_SOCKOPT_TCP_MAXSEG, &m_mss);
        if (m_mss == 0)
        {
            // Socket is not connected or kernel is uncooperative
            m_mss = (m_type == HX_SOCK_TYPE_TCP)
                    ? DEFAULT_TCP_READ_SIZE
                    : DEFAULT_UDP_READ_SIZE;
        }
    }

    if (SUCCEEDED(hxr = CreateReadBuffer(ppBuf)) &&
        SUCCEEDED(hxr = (*ppBuf)->SetSize(m_mss)))
    {
        n = hx_readfrom(&m_sock, (*ppBuf)->GetBuffer(), m_mss, psa, salen, bPeek);
        if (n > 0)
        {
            hxr = SetReadBufferSize(ppBuf, n);
        }
        else if (n == 0)
        {
            if (m_uPendingEventMask & HX_SOCK_EVENT_READ)
            {
                m_sock.state = HX_SOCK_STATE_CLOSED;
            }
            hxr = HXR_SOCK_ENDSTREAM;
            HX_RELEASE(*ppBuf);
        }
        else
        {
            hxr = ErrorToStatus(hx_lastsockerr());
            HX_RELEASE(*ppBuf);
            if (ppAddr != NULL)
            {
                HX_RELEASE(*ppAddr);
            }
            // if we don't flush the write queue on a socket error then
            // we wind up stuck in a lingering state after Close()
            if (hxr == HXR_SOCK_CONNRESET)
            {
                m_pWriteQueue->Discard();
            }
        }
    }
    return hxr;
}

HX_RESULT
CHXSocket::DoWrite(IHXBuffer* pBuf, IHXSockAddr* pAddr /* = NULL */)
{
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        if (pAddr != NULL)
        {
            // Choose socket based on address type
            if (RequiresSock4(pAddr))
            {
                return m_pSock4->DoWrite(pBuf, pAddr);
            }
        }
        else
        {
            // Socket must be bound/connected in IPv4 only mode
            HX_ASSERT(m_pSock4->m_pSock6 == NULL);
            return m_pSock4->DoWrite(pBuf, pAddr);
        }
    }
#endif

    if (m_pTotalNetWriters)
    {
        InterlockedIncrement(m_pTotalNetWriters);
    }

    HXBOOL bQueueWasEmpty = m_pWriteQueue->IsEmpty();

    HX_RESULT hxr = m_pWriteQueue->Enqueue(1, &pBuf, pAddr);
    if (SUCCEEDED(hxr))
    {
	// pBuf buffered
	hxr = HXR_SOCK_BUFFERED;

        if (bQueueWasEmpty && m_pWriteQueue->GetQueuedBytes() >= m_uAggLimit)
        {
            hxr = HandleWriteEvent();
	    if (FAILED(hxr))
	    {
		if (IS_STREAM_TYPE(m_type))
		{
		    if ((m_uUserEventMask & HX_SOCK_EVENT_CLOSE)
			&& m_pResponse != NULL)
		    {
			m_pResponse->EventPending(HX_SOCK_EVENT_CLOSE, hxr);
		    }
		}
	    }
        }
    }
    else
    {
        if (!m_bBlocked)
        {
            m_bBlocked = TRUE;
            m_uAllowedEventMask |= HX_SOCK_EVENT_WRITE;
            HX_ASSERT(m_uForcedEventMask & HX_SOCK_EVENT_WRITE);
        }
    }

    return hxr;
}

HX_RESULT
CHXSocket::DoReadV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppBufVec,
                IHXSockAddr** ppAddr /* = NULL */)
{
    HX_RESULT hxr;
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        hxr = m_pSock4->DoReadV(nVecLen, puLenVec, ppBufVec, ppAddr);
        if (hxr == HXR_SOCK_ENDSTREAM || !HX_SOCK_VALID(m_sock))
        {
            return hxr;
        }
    }
#endif

    if (m_pTotalNetReaders)
    {
        InterlockedIncrement(m_pTotalNetReaders);
    }

    UINT32 n;
    ssize_t len;
    sockaddr* psa = NULL;
    size_t salen = 0;
    hx_iov vec[HX_IOV_MAX];

    m_uAllowedEventMask |= HX_SOCK_EVENT_READ;
    Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask);

    if (nVecLen > HX_IOV_MAX)
    {
        return HXR_INVALID_PARAMETER;
    }
    if (ppAddr != NULL)
    {
        HX_ASSERT(*ppAddr == NULL);
        IHXSockAddrNative* pNative = NULL;
        hxr = m_pNetSvc->CreateSockAddr(m_family, ppAddr);
        if (FAILED(hxr))
        {
            return hxr;
        }
        hxr = (*ppAddr)->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
        if (FAILED(hxr))
        {
            return hxr;
        }
        pNative->Get(&psa, &salen);
        HX_RELEASE(pNative);
    }
    for (n = 0; n < nVecLen; n++)
    {
        if (FAILED(hxr = CreateReadBuffer(&ppBufVec[n])))
        {
            return hxr;
        }
        if (FAILED(hxr = ppBufVec[n]->SetSize(puLenVec[n])))
        {
            return hxr;
        }
        vec[n].set_buf(ppBufVec[n]->GetBuffer());
        vec[n].set_len(ppBufVec[n]->GetSize());
    }
    len = hx_readfromv(&m_sock, nVecLen, vec, psa, salen);
    if (len < 0)
    {
        return ErrorToStatus(hx_lastsockerr());
    }
    if (len == 0 && (m_uPendingEventMask & HX_SOCK_EVENT_READ))
    {
        //XXXTDM: hxr = HXR_SOCK_ENDSTREAM
        m_sock.state = HX_SOCK_STATE_CLOSED;
    }
    m_uPendingEventMask &= ~HX_SOCK_EVENT_READ;
    for (n = 0; n < nVecLen; n++)
    {
        if (vec[n].get_len() == 0)
        {
            HX_RELEASE(ppBufVec[n]);
            // Verify there are no holes in the read vector
            HX_ASSERT(n == 0 || ppBufVec[n-1] != NULL);
        }
        else if (vec[n].get_len() < ppBufVec[n]->GetSize())
        {
            hxr = SetReadBufferSize(&ppBufVec[n], vec[n].get_len());
            if (FAILED(hxr))
            {
                return hxr;
            }
        }
    }
    return HXR_OK;
}

HX_RESULT
CHXSocket::DoWriteV(UINT32 nVecLen, IHXBuffer** ppVec,
                IHXSockAddr* pAddr /* = NULL */)
{
#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        if (pAddr != NULL)
        {
            // Choose socket based on address type
            if (RequiresSock4(pAddr))
            {
                return m_pSock4->DoWriteV(nVecLen, ppVec, pAddr);
            }
        }
        else
        {
            // Socket must be bound/connected in IPv4 only mode
            HX_ASSERT(m_pSock4->m_pSock6 == NULL);
            return m_pSock4->DoWriteV(nVecLen, ppVec, pAddr);
        }
    }
#endif

    if (m_pTotalNetWriters)
    {
        InterlockedIncrement(m_pTotalNetWriters);
    }

    if (nVecLen > HX_IOV_MAX)
    {
        return HXR_INVALID_PARAMETER;
    }

    HXBOOL bQueueWasEmpty = m_pWriteQueue->IsEmpty();

    HX_RESULT hxr = m_pWriteQueue->Enqueue(nVecLen, ppVec, pAddr);
    if (SUCCEEDED(hxr))
    {
	// pBuf buffered
	hxr = HXR_SOCK_BUFFERED;

        if (bQueueWasEmpty && m_pWriteQueue->GetQueuedBytes() >= m_uAggLimit)
        {
            hxr = HandleWriteEvent();
	    if (FAILED(hxr))
	    {
		if (IS_STREAM_TYPE(m_type))
		{
		    if ((m_uUserEventMask & HX_SOCK_EVENT_CLOSE) && 
			m_pResponse != NULL)
		    {
			m_pResponse->EventPending(HX_SOCK_EVENT_CLOSE, hxr);
		    }
		}
	    }
        }
    }
    else
    {
        if (!m_bBlocked)
        {
            m_bBlocked = TRUE;
            m_uAllowedEventMask |= HX_SOCK_EVENT_WRITE;
            HX_ASSERT(m_uForcedEventMask & HX_SOCK_EVENT_WRITE);
        }
    }

    return hxr;
}

HX_RESULT
CHXSocket::HandleWriteEvent(void)
{
    HX_ASSERT(m_pWriteQueue != NULL && !m_pWriteQueue->IsEmpty());

    HX_RESULT hxr = HXR_OK;
    ssize_t len = 0;
    hx_iov vec[HX_IOV_MAX];
    UINT32 vec_len;
    IHXSockAddr* pAddr;
    sockaddr_storage ss;
    sockaddr* psa = NULL;
    size_t salen = 0;

    m_pWriteQueue->FillVector(&vec_len, vec, &pAddr);
    if (pAddr != NULL)
    {
        GetNativeAddr(pAddr, &ss, &psa, &salen);
    }
    len = hx_writetov(&m_sock, vec_len, vec, psa, salen);
    if (len < 0)
    {
        int err = hx_lastsockerr();
        if (err == SOCKERR_WOULDBLOCK)
        {
            hxr = HXR_SOCK_BUFFERED;
            m_bBlocked = TRUE;
        }
        else
	{
            hxr = ErrorToStatus(err);
	    m_pWriteQueue->Discard();
        }
    }
    else
    {
        m_pWriteQueue->Dequeue(len);
	if (!m_pWriteQueue->IsEmpty())
	{
            hxr = HXR_SOCK_BUFFERED;
            m_bBlocked = TRUE;
	}
    }

    if (m_pWriteQueue->IsEmpty())
    {
        if (m_uForcedEventMask & HX_SOCK_EVENT_WRITE)
        {
            m_uForcedEventMask &= ~HX_SOCK_EVENT_WRITE;
            Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask);
        }
    }
    else
    {
        if (!(m_uForcedEventMask & HX_SOCK_EVENT_WRITE))
        {
            m_uForcedEventMask |= HX_SOCK_EVENT_WRITE;
            Select((m_uUserEventMask & m_uAllowedEventMask) | m_uForcedEventMask);
        }
    }

    return hxr;
}

HX_RESULT
CHXSocket::SetMulticastTTL(UINT32 ttl)
{
    int name = HX_SOCKOPT_NONE;
    if(m_family == HX_SOCK_FAMILY_IN4)
    {
        name = HX_SOCKOPT_IN4_MULTICAST_TTL;
    }
    else if(m_family == HX_SOCK_FAMILY_IN6)
    {
        name = HX_SOCKOPT_IN6_MULTICAST_HOPS;
    }
    else
    {
        return HXR_SOCK_AFNOSUPPORT;
    }

    if (hx_setsockopt(&m_sock, name, &ttl, sizeof(ttl)) != 0)
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT
CHXSocket::SetMulticastInfoIN4(IHXSockAddr* pGroupAddr,
                               IHXSockAddr* pInterface,
                               ip_mreq& req)
{
    HX_ASSERT(pGroupAddr != NULL);
    HX_ASSERT(HX_SOCK_FAMILY_IN4 == pGroupAddr->GetFamily());

    IHXSockAddrNative* pNative = NULL;
    HX_RESULT hxr = pGroupAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    if (SUCCEEDED(hxr))
    {
        // get and set group addr
        sockaddr_in* psa = NULL;
        size_t salen = 0;
        pNative->Get((sockaddr**)&psa, &salen);

        memset(&req, 0, sizeof(req));
        req.imr_multiaddr = psa->sin_addr;
        if (pInterface != NULL)
        {
            // get and set interface addr
            if (pInterface->GetFamily() == HX_SOCK_FAMILY_IN4)
            {
		IHXSockAddrNative* pInterfaceNative = NULL;
		HX_RESULT hxr = pInterface->QueryInterface(IID_IHXSockAddrNative, (void**)&pInterfaceNative);
		if (SUCCEEDED(hxr))
		{
		    pInterfaceNative->Get((sockaddr**)&psa, &salen);
		    req.imr_interface = psa->sin_addr;
		}
		HX_RELEASE(pInterfaceNative);
	    }
            else
            {
                // interface family does not match group family
                HX_ASSERT(false);
                hxr  = HXR_INVALID_PARAMETER;
            }
            HX_RELEASE(pNative);
        }

    }

    return hxr;
}

HX_RESULT
CHXSocket::SetMulticastInfoIN6(IHXSockAddr* pGroupAddr,
                               IHXSockAddr* pInterface,
                               ipv6_mreq& req)
{
    HX_ASSERT(pGroupAddr);
    HX_ASSERT(HX_SOCK_FAMILY_IN6 == pGroupAddr->GetFamily());

    IHXSockAddrNative* pNative = NULL;
    HX_RESULT hxr = pGroupAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    if (SUCCEEDED(hxr))
    {
        // get and set group addr
        sockaddr_in6* psa = NULL;
        size_t salen = 0;
        pNative->Get((sockaddr**)&psa, &salen);

        memset(&req, 0, sizeof(req));
        req.ipv6mr_multiaddr = psa->sin6_addr;

        if (pInterface != NULL)
        {
            // XXXLCM need to map address to interface index
            HX_ASSERT(false);
            hxr = HXR_INVALID_PARAMETER;
#if (0)
            // Windows uses GetAdaptersAddresses() (from IPHLPAPI.DLL) for index; Unix netdevice (net/if.h)
            if (pInterface->GetFamily() == HX_SOCK_FAMILY_IN6)
            {
                req.ipv6mr_interface = HXPosixSockUtil::InterfaceAddrToIndex(pInterface);
            }
            else
            {
                // interface family does not match group family
                HX_ASSERT(false);
                hxr = HXR_INVALID_PARAMETER;
            }
#endif
        }

    }

    return hxr;
}

/*** CHXListeningSocket ***/

CHXListeningSocket::CHXListeningSocket(IHXSocket* pSock) :
    m_nRefCount(0),
    m_pResponse(NULL),
    m_pSock(pSock)
{
    HX_ADDREF(m_pSock);
}

CHXListeningSocket::~CHXListeningSocket(void)
{
    Close();
}

STDMETHODIMP
CHXListeningSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocket))
    {
        AddRef();
        *ppvObj = (IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXListeningSocket))
    {
        AddRef();
        *ppvObj = (IHXListeningSocket*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXListeningSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXListeningSocket::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(HXSockFamily)
CHXListeningSocket::GetFamily(void)
{
    return m_pSock->GetFamily();
}

STDMETHODIMP_(HXSockType)
CHXListeningSocket::GetType(void)
{
    return m_pSock->GetType();
}

STDMETHODIMP_(HXSockProtocol)
CHXListeningSocket::GetProtocol(void)
{
    return m_pSock->GetProtocol();
}

STDMETHODIMP
CHXListeningSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p,
        IHXListeningSocketResponse* pResponse)
{
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);

    m_pSock->SetResponse(this);
    return m_pSock->Init(f, t, p);
}

STDMETHODIMP
CHXListeningSocket::Listen(IHXSockAddr* pAddr)
{
    HX_RESULT hxr = HXR_OK;

#ifndef _WIN32
    // We do this to make sure that REUSEADDR is off for NT because
    // REUSEADDR for NT means REUSEPORT as well.
    hxr = m_pSock->SetOption(HX_SOCKOPT_REUSEADDR, 1);
#endif

    if (hxr == HXR_OK)
    {
        hxr = m_pSock->Bind(pAddr);
    }
    if (hxr == HXR_OK)
    {
        hxr = m_pSock->SelectEvents(HX_SOCK_EVENT_ACCEPT|HX_SOCK_EVENT_CLOSE);
    }
    if (hxr == HXR_OK)
    {
        hxr = m_pSock->Listen(SOMAXCONN);
    }
    return hxr;
}

STDMETHODIMP
CHXListeningSocket::Close(void)
{
    if (m_pSock != NULL)
    {
        m_pSock->Close();
        HX_RELEASE(m_pSock);
    }
    HX_RELEASE(m_pResponse);
    return HXR_OK;
}

STDMETHODIMP
CHXListeningSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    return m_pSock->GetOption(name, pval);
}

STDMETHODIMP
CHXListeningSocket::SetOption(HXSockOpt name, UINT32 val)
{
    return m_pSock->SetOption(name, val);
}

STDMETHODIMP
CHXListeningSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_RESULT hxr;
    IHXSocket* pNewSock = NULL;
    IHXSockAddr* pSource = NULL;
    switch (uEvent)
    {
    case HX_SOCK_EVENT_ACCEPT:
        if (SUCCEEDED(hxr = m_pSock->Accept(&pNewSock, &pSource)))
        {
            m_pResponse->OnConnection(pNewSock, pSource);
        }
        else
        {
            m_pResponse->OnError(hxr);
        }
        HX_RELEASE(pNewSock);
        HX_RELEASE(pSource);
        break;
    case HX_SOCK_EVENT_CLOSE:
	if (m_pResponse != NULL)
	{
	    m_pResponse->OnError(HXR_SOCK_NETDOWN /* XXX */);
	}
        break;
    default:
        HX_ASSERT(FALSE);
    };
    return HXR_OK;
}

/*** CHXAddrInfo ***/
#if(0)

CHXAddrInfo::CHXAddrInfo(void)
: m_nRefCount(0)
{
}


STDMETHODIMP_(UINT32)
CHXAddrInfo::GetFlags) ()
{
}

STDMETHODIMP
CHXAddrInfo::SetFlags(UINT32 uFlags)
{
}

STDMETHODIMP_(UINT32)
CHXAddrInfo::GetFamily()
{
}

STDMETHODIMP
CHXAddrInfo::SetFamily(UINT32 uFamily)
{
}
STDMETHODIMP_(UINT32)
CHXAddrInfo::GetType()
{
}

STDMETHODIMP
CHXAddrInfo::SetType(UINT32 uType)
{
}

STDMETHODIMP_
CHXAddrInfo::GetProtocol()
{
}

STDMETHODIMP
CHXAddrInof::SetProtocol(UINT32 uProtocol)
{
}


STDMETHODIMP
CHXAddrInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXAddrInfo*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXAddrInfo))
    {
        AddRef();
        *ppvObj = (IHXAddrInfo*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXAddrInfo::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXAddrInfo::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

#endif

/*** CHXNetServices ***/

CHXNetServices::CHXNetServices(void) :
    m_nRefCount(0),
    m_punkContext(NULL),
    m_bIN6(FALSE)
{
    // Empty
}

CHXNetServices::~CHXNetServices(void)
{
    Close();
}

HX_RESULT
CHXNetServices::Init(IUnknown* punkContext)
{
    m_punkContext = punkContext;
    HX_ADDREF(m_punkContext);

    //XXXLCM this loads then unloads the winsock dll (undesireable)
    HXNetDrvLoader loader;
    HX_RESULT hr = loader.EnsureLoaded(punkContext);
    if (HXR_OK == hr)
    {
        m_bIN6 = hx_netdrv_familyavail(HX_SOCK_FAMILY_IN6);
    }
    return hr;
}

STDMETHODIMP
CHXNetServices::RegisterContext(IUnknown* pIContext)
{
    return Init(pIContext);
}

STDMETHODIMP
CHXNetServices::Close(void)
{
    HX_RELEASE(m_punkContext);
    return HXR_OK;
}

STDMETHODIMP
CHXNetServices::QueryInterface(REFIID riid, void** ppvObj)
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
    else if (IsEqualIID(riid, IID_IHXNetServices2))
    {
        AddRef();
        *ppvObj = (IHXNetServices2*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXContextUser))
    {
        AddRef();
        *ppvObj = (IHXContextUser*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXNetServices::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXNetServices::Release(void)
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
CHXNetServices::CreateSockAddr(HXSockFamily f, IHXSockAddr** ppAddr)
{
    IHXSockAddr* pAddr = NULL;
    switch (f)
    {
    case HX_SOCK_FAMILY_LOCAL:
        pAddr = new CHXSockAddrLocal(m_punkContext);
        break;
    case HX_SOCK_FAMILY_IN4:
        pAddr = new CHXSockAddrIN4(m_punkContext);
        break;
    case HX_SOCK_FAMILY_IN6:
        pAddr = new CHXSockAddrIN6(m_punkContext);
        break;
    case HX_SOCK_FAMILY_INANY:
        if (m_bIN6)
        {
            pAddr = new CHXSockAddrIN6(m_punkContext);
        }
        else
        {
            pAddr = new CHXSockAddrIN4(m_punkContext);
        }
        break;
    default:
        HX_ASSERT(FALSE);
        *ppAddr = NULL;
        return HXR_UNEXPECTED;
    }
    if (pAddr == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    pAddr->QueryInterface(IID_IHXSockAddr, (void**)ppAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXNetServices::CreateListeningSocket(IHXListeningSocket** ppSock)
{
    HX_RESULT hxr = HXR_OK;
    IHXSocket* pActualSock = NULL;
    if (SUCCEEDED(CreateSocket(&pActualSock)))
    {
        *ppSock = new CHXListeningSocket(pActualSock);
        if (*ppSock != NULL)
        {
            (*ppSock)->AddRef();
        }
        else
        {
            HX_RELEASE(pActualSock);
            hxr = HXR_OUTOFMEMORY;
        }
    }
    return hxr;
}

