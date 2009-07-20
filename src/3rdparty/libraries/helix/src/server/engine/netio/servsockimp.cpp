/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servsockimp.cpp,v 1.59 2009/05/30 20:16:48 dcollins Exp $
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
#include "nettypes.h"
#include "hxcom.h"
#include "proc.h"
#include "proc_container.h"
#include "server_engine.h"
#include "server_context.h"
#include "servreg.h"
#include "hxnet.h"
#include "iresolv.h"
#include "servresolvimp.h"
#include "resolvcache.h"
#include "writequeue.h"
#include "servertrace.h"
#include "servsockimp.h"


#include "core_proc.h"

#include "listenresp.h"
#include "servbuffer.h"
#include "hxsbuffer.h"

#ifdef _SOLARIS
#include "core_container.h"
#include "conn_dispatch.h"
#include "conn_dispatch.h"
#endif


extern UINT32* g_pSocketAcceptCount;

static const UINT32 z_ulMaxLBWriteQueueSize = 100;

static const UINT32 DEFAULT_TCP_SNDBUF_SIZE = 32768;
static const UINT32 DEFAULT_UDP_SNDBUF_SIZE = 16834;
static const UINT32 DEFAULT_CONNECT_TIMEOUT_SEC = 30;

CServSockCB::CServSockCB(CHXServSocket* pSock, UINT32 event) :
    m_nRefCount(0),
    m_pSock(pSock),
    m_event(event)
{
    m_pSock->AddRef();
}

CServSockCB::~CServSockCB(void)
{
    HX_RELEASE(m_pSock);
}

STDMETHODIMP
CServSockCB::QueryInterface(REFIID riid, void** ppvObj)
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
CServSockCB::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CServSockCB::Release(void)
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
CServSockCB::Func(void)
{
    AddRef();
    m_pSock->OnEvent(m_event);
    Release();
    return HXR_OK;
}

// save proc where the sock was created and passed from, so that u can remove
// the socket passing cb from the proc's scheduler if need b.
// for example, in the case where the connection is dead b4 it reached the
// other proc and m_pProc == NULL, the CorePassSockCallback would fail.
CHXServSocket::CHXServSocket(Process* proc, BOOL bIN6) :
    CHXSocket(proc->pc->net_services,
              (IUnknown*)proc->pc->server_context),
    m_pProc(proc),
    m_pSockCreationProc(proc),
    m_bIN6(bIN6),
    m_pCBR(NULL),
    m_pCBW(NULL),
    m_uSelectedEventMask(0),
    m_hCorePassSockCbID(0),
    m_bDontDispatch(FALSE),
    m_bRemovedCallbacks(FALSE)
{
    m_pProc->pc->engine->RegisterSock();
    m_pTotalNetReaders = g_pTotalNetReaders;
    m_pTotalNetWriters = g_pTotalNetWriters;
}

CHXServSocket::CHXServSocket(HXSockFamily f,
                             HXSockType t,
                             HXSockProtocol p,
                             HX_SOCK sock,
                             Process* proc) :
    CHXSocket(proc->pc->net_services,
              (IUnknown*)proc->pc->server_context,
              f, t, p, sock),
    m_pProc(proc),
    m_pSockCreationProc(proc),
    m_bIN6(FALSE),
    m_pCBR(NULL),
    m_pCBW(NULL),
    m_uSelectedEventMask(0),
    m_hCorePassSockCbID(0),
    m_bDontDispatch(FALSE),
    m_bRemovedCallbacks(FALSE)
{
    m_pProc->pc->engine->RegisterSock();
    m_pCBR = new CServSockCB(this, HX_SOCK_EVENT_READ);
    m_pCBR->AddRef();
    m_pProc->pc->engine->callbacks.add(HX_READERS, m_sock.sock, m_pCBR, TRUE);
    m_pProc->pc->engine->callbacks.disable(HX_READERS, m_sock.sock);
    m_pCBW = new CServSockCB(this, HX_SOCK_EVENT_WRITE);
    m_pCBW->AddRef();
    m_pProc->pc->engine->callbacks.add(HX_WRITERS, m_sock.sock, m_pCBW, TRUE);
    m_pProc->pc->engine->callbacks.disable(HX_WRITERS, m_sock.sock);

#ifdef _WIN32
    m_pProc->pc->engine->callbacks.add(HX_CONNECTORS, m_sock.sock, m_pCBW, TRUE);
    m_pProc->pc->engine->callbacks.disable(HX_CONNECTORS, m_sock.sock);
#endif

    m_pTotalNetReaders = g_pTotalNetReaders;
    m_pTotalNetWriters = g_pTotalNetWriters;
}

CHXServSocket::~CHXServSocket(void)
{
    Select(HX_SOCK_EVENT_NONE);
    if (m_hCorePassSockCbID)
    {
        m_pSockCreationProc->pc->engine->schedule.remove(m_hCorePassSockCbID);
        m_hCorePassSockCbID = 0;
    }
    if (m_bRemovedCallbacks)
    {
    HX_RELEASE(m_pCBW);
    HX_RELEASE(m_pCBR);
    }
    m_pProc = NULL;
}

HX_RESULT
CHXServSocket::Select(UINT32 uEventMask, BOOL bImplicit /* = TRUE */)
{
    HX_ASSERT(m_pProc);
    if (!m_pProc)
    {
        return HXR_UNEXPECTED;
    }

#if defined(MISSING_DUALSOCKET)
    if (m_pSock4 != NULL)
    {
        m_pSock4->SelectEvents(uEventMask);
    }
#endif

    if (!HX_SOCK_VALID(m_sock))
    {
#if defined(MISSING_DUALSOCKET)
        if (m_pSock4)
        {
            return HXR_OK;
        }
#endif
        return HXR_FAIL;
    }

    if (uEventMask == m_uSelectedEventMask)
    {
        return HXR_OK;
    }

    m_uSelectedEventMask = uEventMask;

    if (!m_pProc)
    {
        return HXR_OK;
    }

    if (uEventMask & (HX_SOCK_EVENT_READ |
                      HX_SOCK_EVENT_ACCEPT |
                      HX_SOCK_EVENT_CLOSE))
    {
        m_pProc->pc->engine->callbacks.enable(HX_READERS, m_sock.sock, TRUE);
    }
    else
    {
        m_pProc->pc->engine->callbacks.disable(HX_READERS, m_sock.sock);
    }

    if (uEventMask & (HX_SOCK_EVENT_WRITE |
                      HX_SOCK_EVENT_CONNECT))
    {
        m_bBlocked = TRUE;
        m_pProc->pc->engine->callbacks.enable(HX_WRITERS, m_sock.sock, TRUE);
#ifdef _WIN32
        m_pProc->pc->engine->callbacks.enable(HX_CONNECTORS, m_sock.sock, TRUE);
#endif
    }
    else
    {
        m_pProc->pc->engine->callbacks.disable(HX_WRITERS, m_sock.sock);
#ifdef _WIN32
        m_pProc->pc->engine->callbacks.disable(HX_CONNECTORS, m_sock.sock);
#endif
    }

    return HXR_OK;
}


void
CHXServSocket::OnEvent(UINT32 ulEvent)
{
    CHXSocket::OnEvent(ulEvent);
}


STDMETHODIMP
CHXServSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    HX_RESULT hxr;
    if (!m_bIN6)
    {
        if (f == HX_SOCK_FAMILY_IN6)
        {
            return HXR_SOCK_AFNOSUPPORT;
        }
        if (f == HX_SOCK_FAMILY_INANY)
        {
            f = HX_SOCK_FAMILY_IN4;
        }
    }
    hxr = CHXSocket::Init(f, t, p);
    if (SUCCEEDED(hxr))
    {
        m_pCBR = new CServSockCB(this, HX_SOCK_EVENT_READ);
        m_pCBR->AddRef();
        m_pProc->pc->engine->callbacks.add(HX_READERS, m_sock.sock, m_pCBR, TRUE);
        m_pProc->pc->engine->callbacks.disable(HX_READERS, m_sock.sock);
        m_pCBW = new CServSockCB(this, HX_SOCK_EVENT_WRITE);
        m_pCBW->AddRef();
        m_pProc->pc->engine->callbacks.add(HX_WRITERS, m_sock.sock, m_pCBW, TRUE);
        m_pProc->pc->engine->callbacks.disable(HX_WRITERS, m_sock.sock);
#ifdef _WIN32
        m_pProc->pc->engine->callbacks.add(HX_CONNECTORS, m_sock.sock, m_pCBW, TRUE);
        m_pProc->pc->engine->callbacks.disable(HX_CONNECTORS, m_sock.sock);
#endif
        // Set the send buffer size, otherwise it is up to the platform.

        INT32 iSndBufSize = 0;

        switch(m_type)
        {
        case HX_SOCK_TYPE_UDP:
        case HX_SOCK_TYPE_MCAST:
            if (FAILED(m_pProc->pc->registry->GetInt("config.UDPSendBufferSize", &iSndBufSize, m_pProc)))
            {
                iSndBufSize = DEFAULT_UDP_SNDBUF_SIZE;
            }
            break;
        case HX_SOCK_TYPE_TCP:
            if (FAILED(m_pProc->pc->registry->GetInt("config.TCPSendBufferSize", &iSndBufSize, m_pProc)))
            {
                iSndBufSize = DEFAULT_TCP_SNDBUF_SIZE;
            }
            break;
        default:
            break;
        }

        HX_ASSERT(iSndBufSize >= 0);

        SetOption(HX_SOCKOPT_SNDBUF, (UINT32)iSndBufSize);

        // Set a default mss value for server sockets if a value is not
        // specified in the config
        if (m_type == HX_SOCK_TYPE_UDP && m_mss == 0)
        {
            INT32 iTmp;
            if (SUCCEEDED(m_pProc->pc->registry->GetInt("config.UDPRecvBufferSize", &iTmp, m_pProc)))
            {
                m_mss = (UINT32)iTmp;
            }
            else
            {
                m_mss = DEFAULT_SERV_UDP_READ_SIZE;
            }
        }
    }
    else
    {
        if (hxr == HXR_SOCK_MFILE)
        {
            char szErr[256];
            snprintf(szErr, sizeof(szErr), 
                    "The server has run out of file descriptors.  It is highly "
                    "recommended that you raise the file descriptor limit and "
                    "restart the server. (failed in socket)");
            m_pProc->pc->error_handler->Report(HXLOG_ERR, 0, 0, szErr, NULL);
        }
    }
    return hxr;
}

STDMETHODIMP
CHXServSocket::Close(void)
{
    //HX_ASSERT(m_pProc && m_sock.sock);
    BOOL bRemovedCbs = FALSE;
    
    /*
     * remove the valid fd from the fd set only if m_pProc and the calling
     * proc are the same.
     */
    if (m_pSockCreationProc->procnum() == Process::get_procnum())
    {
        if (!m_bRemovedCallbacks && !m_hCorePassSockCbID && m_pProc == m_pSockCreationProc)
        {  
            // core thread: b4 Dispatch()
            m_pProc->pc->engine->callbacks.remove(HX_READERS, m_sock.sock);
            m_pProc->pc->engine->callbacks.remove(HX_WRITERS, m_sock.sock);
#ifdef _WIN32
            m_pProc->pc->engine->callbacks.remove(HX_CONNECTORS, m_sock.sock);
#endif
            m_pProc->pc->engine->UnRegisterSock();
            m_bDontDispatch = TRUE;
            m_bRemovedCallbacks = TRUE;
        }
    }
    else
    {
        /*
         * when the caller of this method is in another thread other than
         * m_pProc then removing callbacks should not be allowed due to
         * re-entrancy issues and the possibility of a deadlock as was
         * happening in pr# 178507 on solaris with /dev/poll.
         * 
         * NOTE:
         *    as a quick workaround such callback removal has been disallowed
         *    thereby introducing a leak of around 332 bytes per connection
         *    and 664 bytes per cloaking connection. here are the objects that
         *    leak:
         *    1 CHXServSocket (172 bytes)
         *    2 CServSockCB (40 * 2 bytes)
         *    1 CHXStreamWriteQueue (40 bytes)
         *    CHXStreamWriteQueue::IHXBuffer*[] (72 bytes)
         */
        if (m_pProc && m_pProc->procnum() == Process::get_procnum())
        {
            // streamer thread: after EnterProc()
            if (!m_bRemovedCallbacks)
            {
                m_pProc->pc->engine->callbacks.remove(HX_READERS, m_sock.sock);
                m_pProc->pc->engine->callbacks.remove(HX_WRITERS, m_sock.sock);
#ifdef _WIN32
                m_pProc->pc->engine->callbacks.remove(HX_CONNECTORS, m_sock.sock);
#endif
                m_pProc->pc->engine->UnRegisterSock();
                m_bRemovedCallbacks = TRUE;
            }
        }
        else
        {
            //This prevents the leaks described above.
            //Live (BCNG) was triggering this leak as well.
            //If we don't release these we have a circular reference.
            HX_RELEASE(m_pCBW);
            HX_RELEASE(m_pCBR);
        }
    }

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
        // Close() was called between ExitProc() and EnterProc()
        if (m_pProc == NULL)
            return hxr;

        if (m_bRemovedCallbacks)
        {
            HX_RELEASE(m_pCBW);
            HX_RELEASE(m_pCBR);
        }
    }
    else
    {
        // add the fd back in so that it can get notification of a read/write
        // operation when ready.
        if (m_bRemovedCallbacks && m_pProc && m_pProc->procnum() == Process::get_procnum())
        {
            if (m_pCBW && m_pCBR)
            {
                m_pProc->pc->engine->RegisterSock();
                m_pProc->pc->engine->callbacks.add(HX_READERS, m_sock.sock, m_pCBR, TRUE);
                m_pProc->pc->engine->callbacks.add(HX_WRITERS, m_sock.sock, m_pCBW, TRUE);
#ifdef _WIN32
                m_pProc->pc->engine->callbacks.add(HX_CONNECTORS, m_sock.sock, m_pCBW, TRUE);
#endif
                m_bRemovedCallbacks = FALSE;
            }
        }
    }
    return hxr;
}

STDMETHODIMP 
CHXServSocket::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    HX_RESULT hxr = HXR_OK;
    
    if (HXR_SOCK_MFILE == (hxr = CHXSocket::Accept(ppNewSock, ppSource)))
    {
        char szErr[256];
        snprintf(szErr, sizeof(szErr), 
                "The server has run out of file descriptors.  It is highly "
                "recommended that you raise the file descriptor limit and "
                "restart the server. (failed in accept)");
        m_pProc->pc->error_handler->Report(HXLOG_ERR, 0, 0, szErr, NULL);
    }
    else if (ppNewSock && *ppNewSock)
    {
        (*g_pSocketAcceptCount)++;
        INT32 nConnectTimeout = DEFAULT_CONNECT_TIMEOUT_SEC;
        m_pProc->pc->registry->GetInt("config.ClientConnectionTimeout", &nConnectTimeout, m_pProc);
        if (nConnectTimeout)
        {
            // Set the inactivity timer on this socket.  Note that this will
            // be reset back to zero in HXSocketConnection::EventPending() 
            // when handing the socket off to the protocol object.
            (*ppNewSock)->SetOption(HX_SOCKOPT_APP_IDLETIMEOUT, (UINT32)nConnectTimeout);
        }
    }

    return hxr;
}

void
CHXServSocket::Dispatch(int iNewProc /* = -1 */)
{
    // Can only dispatch from the coreproc
    HX_ASSERT(m_pProc->pc->process_type == PTCore);

#if ENABLE_LATENCY_STATS
    TCorePassCB();
#endif

    if (!m_bDontDispatch && HX_SOCK_VALID(m_sock))
    {
        if (!m_bRemovedCallbacks)
        {
            m_pProc->pc->engine->callbacks.remove(HX_READERS, m_sock.sock);
            m_pProc->pc->engine->callbacks.remove(HX_WRITERS, m_sock.sock);
#ifdef _WIN32
            m_pProc->pc->engine->callbacks.remove(HX_CONNECTORS, m_sock.sock);
#endif
            m_pProc->pc->engine->UnRegisterSock();
            m_bRemovedCallbacks = TRUE;
        }

        CorePassSockCallback* cb = new CorePassSockCallback;
        cb->m_pSock = this;
        cb->m_pProc = m_pProc;
        cb->m_iNewProc = iNewProc;
        m_hCorePassSockCbID = m_pProc->pc->engine->schedule.enter(0.0, cb);
    }
}

void
CHXServSocket::ExitProc(void)
{
    HX_ASSERT(m_pProc->pc->engine != NULL && HX_SOCK_VALID(m_sock));

    m_hCorePassSockCbID = 0;

    if (m_hIdleCallback)
    {
        m_pScheduler->Remove(m_hIdleCallback);
        m_hIdleCallback = 0;
    }
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pNetSvc);

    m_pProc = NULL;
}

void
CHXServSocket::EnterProc(Process* proc)
{
    // means that Close() was already called on this socket
    if (!m_pResponse)
        return;

    HX_ASSERT(m_pProc == NULL && HX_SOCK_VALID(m_sock));

    m_pProc = proc;
    if (m_pCBW && m_pCBR)
    {
        m_pProc->pc->engine->RegisterSock();
        m_pProc->pc->engine->callbacks.add(HX_READERS, m_sock.sock, m_pCBR, TRUE);
        m_pProc->pc->engine->callbacks.add(HX_WRITERS, m_sock.sock, m_pCBW, TRUE);
#ifdef _WIN32
        m_pProc->pc->engine->callbacks.add(HX_CONNECTORS, m_sock.sock, m_pCBW, TRUE);
#endif
        m_bRemovedCallbacks = FALSE;
    }

    m_pNetSvc = proc->pc->net_services;
    m_pNetSvc->AddRef();
    m_pScheduler = (IHXScheduler*)proc->pc->scheduler;
    m_pScheduler->AddRef();
    if (m_uIdleTimeout != 0)
    {
        m_hIdleCallback = m_pScheduler->RelativeEnter(this, m_uIdleTimeout*1000);
    }

    UINT32 uMask = m_uSelectedEventMask;
    m_uSelectedEventMask = 0;
    Select(uMask);

    // Set the send buffer size, otherwise it is up to the platform.

    INT32 iSndBufSize = 0;

    switch(m_type)
    {
    case HX_SOCK_TYPE_UDP:
    case HX_SOCK_TYPE_MCAST:
        if (FAILED(m_pProc->pc->registry->GetInt("config.UDPSendBufferSize", &iSndBufSize, m_pProc)))
        {
            iSndBufSize = DEFAULT_UDP_SNDBUF_SIZE;
        }
        break;
    case HX_SOCK_TYPE_TCP:
        if (FAILED(m_pProc->pc->registry->GetInt("config.TCPSendBufferSize", &iSndBufSize, m_pProc)))
        {
            iSndBufSize = DEFAULT_TCP_SNDBUF_SIZE;
        }
        break;
    default:
        break;
    }

    HX_ASSERT(iSndBufSize >= 0);

    SetOption(HX_SOCKOPT_SNDBUF, (UINT32)iSndBufSize);

    HX_ASSERT(m_pResponse);
    if (m_pResponse)
        m_pResponse->EventPending(HX_SOCK_EVENT_DISPATCH, HXR_OK);
}

BOOL
CHXServSocket::IsValid(void)
{
    return HX_SOCK_VALID(m_sock);
}

BOOL
CHXServSocket::InDispatch(void)
{
    return (m_pProc == NULL);
}

/*** CHXServerListeningSocket ***/

CHXServerListeningSocket::CHXServerListeningSocket(IHXNetServices* pNetServices, Process* proc) :
    m_nRefCount(0),
    m_pResponse(NULL),
    m_pSockList(NULL),
    m_family(HX_SOCK_FAMILY_NONE),
    m_type(HX_SOCK_TYPE_NONE),
    m_protocol(HX_SOCK_PROTO_NONE),
    m_pNetServices(pNetServices),
    m_pAddrAny(NULL),
    m_proc(proc),
    m_pAccessControl(NULL)
{
    m_pNetServices->AddRef();
    m_pNetServices->CreateSockAddr(HX_SOCK_FAMILY_INANY, &m_pAddrAny);

    // Setup access control
    m_pAccessControl = new CServerAccessControl(m_proc->pc->error_handler);
    m_pAccessControl->AddRef();
    m_pAccessControl->Init(m_proc);

    m_pSockList = new CHXSimpleList;
}

CHXServerListeningSocket::~CHXServerListeningSocket(void)
{
    Close();
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pAccessControl);
}

STDMETHODIMP
CHXServerListeningSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXListeningSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListeningSocket))
    {
        AddRef();
        *ppvObj = (IHXListeningSocket*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXServerListeningSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXServerListeningSocket::Release(void)
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
CHXServerListeningSocket::GetFamily(void)
{
    return m_family;
}

STDMETHODIMP_(HXSockType)
CHXServerListeningSocket::GetType(void)
{
    return m_type;
}

STDMETHODIMP_(HXSockProtocol)
CHXServerListeningSocket::GetProtocol(void)
{
    return m_protocol;
}

STDMETHODIMP
CHXServerListeningSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p,
        IHXListeningSocketResponse* pResponse)
{
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);

    m_family = f;
    m_type = t;
    m_protocol = p;

    return HXR_OK;
}

STDMETHODIMP
CHXServerListeningSocket::Listen(IHXSockAddr* pAddr)
{
    HX_RESULT hxr = HXR_OK;
    HX_RESULT hRet = HXR_OK;

    if(pAddr->IsEqualAddr(m_pAddrAny) == FALSE)
    {
        // if users want to listen on a specific address, we will only bind to
        // that one.
        IHXListeningSocket* pListeningSocket = NULL;
        hxr = CreateOneListeningSocket(pAddr, &pListeningSocket, 
                    pAddr->GetFamily() == HX_SOCK_FAMILY_IN6);
        if (SUCCEEDED(hxr))
        {
            m_pSockList->AddTail(pListeningSocket);
        }
        return hxr;
    }

    // the user doesn't pick an address to bind to.  Check "server.ipbinding" in the
    //  registry, it should exist since core process create it.
    IHXValues* pAddrList = NULL;
    IHXBuffer* pAddrBuf = NULL;
    IHXSockAddr* pBindingAddr = NULL;
    int addrCount = 0;
    HX_RESULT reS = HXR_OK;

    const char* addr_prop_name;
    UINT32      addr_prop_id;
    int         num_props = 0;
    BOOL        bIPv6Only = FALSE;
    
    m_proc->pc->registry->GetPropList("server.ipbinding", pAddrList, m_proc);
    
    HX_ASSERT(pAddrList != NULL);
    
    hxr = pAddrList->GetFirstPropertyULONG32(addr_prop_name,
                addr_prop_id);
    while(hxr == HXR_OK)
    {
        hxr = m_proc->pc->registry->GetStr(addr_prop_id, pAddrBuf, m_proc);
        if (FAILED(hxr))
        {
            hRet = hxr;
        }
        else
        {
            if (!strcmp((const char*)pAddrBuf->GetBuffer(), "*"))
            {
                m_pNetServices->CreateSockAddr(HX_SOCK_FAMILY_INANY, &pBindingAddr);
            }
            else
            {
                m_pNetServices->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pBindingAddr);
                if (pBindingAddr->SetAddr(pAddrBuf) != HXR_OK)
                {
                    pBindingAddr->Release();
                    m_pNetServices->CreateSockAddr(HX_SOCK_FAMILY_IN6, &pBindingAddr);
                    if (pBindingAddr->SetAddr(pAddrBuf) != HXR_OK)
                    {
                        pBindingAddr->Release();
                        pBindingAddr = NULL;
                    }
                    bIPv6Only = TRUE;
                }
            }
            HX_RELEASE(pAddrBuf);

            //not a valid address
            if (pBindingAddr == NULL)
            {
                hRet = HXR_FAIL;
            }
            else
            {
                pBindingAddr->SetPort(pAddr->GetPort());
                
                IHXListeningSocket* pListeningSocket = NULL;
                hxr = CreateOneListeningSocket(pBindingAddr, &pListeningSocket, bIPv6Only);

                if (SUCCEEDED(hxr))
                {
                    m_pSockList->AddTail(pListeningSocket);
                }
                else
                {
                    // notify the user that at least one of the bindings failed
                    hRet = hxr;
                }
                pBindingAddr->Release();
            }
        }
        hxr = pAddrList->GetNextPropertyULONG32(addr_prop_name,
                                                addr_prop_id);
        bIPv6Only = FALSE;
    }

    return hRet;
}

HX_RESULT
CHXServerListeningSocket::CreateOneListeningSocket(IHXSockAddr* pAddr, IHXListeningSocket** ppSock, BOOL bIPv6Only)
{
    IHXSocket* pActualSock = NULL;
    HX_RESULT hxr = HXR_OK;

    hxr = m_pNetServices->CreateSocket(&pActualSock);
    if (FAILED(hxr))
    {
        *ppSock = NULL;
        return hxr;
    }

    // we only do access control for the listeners in the core process
    if (m_proc->procnum() == PROC_RM_CORE)
    {
        pActualSock->SetAccessControl(m_pAccessControl);
    }

    *ppSock = new CHXListeningSocket(pActualSock);
    if (*ppSock == NULL)
    {
        pActualSock->Release();
        return NULL;
    }

    (*ppSock)->AddRef();
    hxr = (*ppSock)->Init(pAddr->GetFamily(), m_type, m_protocol, m_pResponse);
    if (FAILED(hxr))
    {
        HX_RELEASE(*ppSock);
        return hxr;
    }

    if (bIPv6Only)
    {
        (*ppSock)->SetOption(HX_SOCKOPT_IN6_V6ONLY, 1);
    }

    hxr = (*ppSock)->Listen(pAddr);
    if (FAILED(hxr))
    {
        char szErr[256];
        const char* pAddrStr = "ALL_INTERFACES";
        IHXBuffer* pAddrBuf = NULL;
        pAddr->GetAddr(&pAddrBuf);
        if(pAddrBuf)
        {
            pAddrStr = (const char*)pAddrBuf->GetBuffer();
        }
        snprintf(szErr, sizeof(szErr), "could not listen on <%s>:<%u>\n", pAddrStr, pAddr->GetPort());
        m_proc->pc->error_handler->Report(HXLOG_ERR, 0, 0, szErr, NULL);

        HX_RELEASE(*ppSock);
    }
    return hxr;
}

STDMETHODIMP
CHXServerListeningSocket::Close(void)
{
    if(m_pSockList)
    {
        CHXListeningSocket* pSock = NULL;
        while(!m_pSockList->IsEmpty())
        {
            pSock = (CHXListeningSocket*)m_pSockList->RemoveHead();
            pSock->Close();
            pSock->Release();
        }
        HX_DELETE(m_pSockList);

    }
    HX_RELEASE(m_pResponse);
    return HXR_OK;
}

STDMETHODIMP
CHXServerListeningSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    // we may have multiple sockets in the list, it will be ambiguous if
    // we return a value.

    // In addition, the method is currently not called in the server code.

    return HXR_UNEXPECTED;
}

STDMETHODIMP
CHXServerListeningSocket::SetOption(HXSockOpt name, UINT32 val)
{
     if(m_pSockList)
    {
        CHXListeningSocket* pSock = NULL;
        CHXSimpleList::Iterator i;
        for (i = m_pSockList->Begin(); i != m_pSockList->End(); ++i)
        {
            pSock = (CHXListeningSocket*)*i;
            pSock->SetOption(name, val);
        }
    }
    return HXR_OK;
}

CServNetServices::CServNetServices(Process* proc) :
    CHXNetServices(),
    m_proc(proc),
    m_pResolverCache(NULL)
{
    // Empty
}

HX_RESULT
CServNetServices::Init(IUnknown* punkContext)
{
    // Initialize base with the driver
    HX_RESULT hxr = CHXNetServices::Init(punkContext);
    if (SUCCEEDED(hxr))
    {
        // Create resolver cache for this proc
        HX_ASSERT(m_pResolverCache == NULL);
        m_pResolverCache = new CResolverCache(m_proc->pc->server_context);
        m_pResolverCache->AddRef();
    }

    /* XXXTDM: This is a bit hackish, the base Init() should take a flag
     * The base class Init() sets m_bIN6 based on whether it can create an
     * IPv6 socket.  We also need to disable IPv6 if UseIPv4Only is set in
     * the server config.
     */
    INT32 iTmp = 0;
    if (SUCCEEDED(m_proc->pc->registry->GetInt("config.UseIPv4Only", &iTmp, m_proc)))
    {
        if (iTmp != 0)
        {
            m_bIN6 = FALSE;
        }
    }

    return hxr;
}

CServNetServices::~CServNetServices(void)
{
    HX_RELEASE(m_pResolverCache);
}

STDMETHODIMP
CServNetServices::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
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
CServNetServices::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CServNetServices::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

HX_RESULT
CServNetServices::CreateSocket(HXSockFamily f, HXSockType t,
        HXSockProtocol p, HX_SOCK sock, IHXSocket** ppSock)
{
    *ppSock = new CHXServSocket(f, t, p, sock, m_proc);
    if (*ppSock == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppSock)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CServNetServices::CreateSocket(IHXSocket** ppSock)
{
    *ppSock = new CHXServSocket(m_proc, m_bIN6);
    if (*ppSock == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppSock)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CServNetServices::CreateResolver(IHXResolve** ppResolver)
{
    HX_ASSERT(m_pResolverCache != NULL);

    BOOL bUseInternalResolver = TRUE;
    INT32 iTmp = 0;
    if (SUCCEEDED(m_proc->pc->registry->GetInt("config.UseInternalResolver", &iTmp, m_proc)))
    {
        bUseInternalResolver = (BOOL)iTmp;
    }

    if (bUseInternalResolver)
    {
        *ppResolver = new CIntResolver(m_proc->pc->server_context, m_pResolverCache);
    }
    else
    {
        *ppResolver = new CServNativeResolve(m_proc, m_pResolverCache);
    }
    if (*ppResolver == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppResolver)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CServNetServices::CreateSockAddr(HXSockFamily f,
        IHXSockAddr** ppAddr)
{
    return CHXNetServices::CreateSockAddr(f, ppAddr);
}

STDMETHODIMP
CServNetServices::CreateListeningSocket(IHXListeningSocket** ppSock)
{
    *ppSock = new CHXServerListeningSocket(this, m_proc);
    if (*ppSock == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppSock)->AddRef();
    return HXR_OK;
}
