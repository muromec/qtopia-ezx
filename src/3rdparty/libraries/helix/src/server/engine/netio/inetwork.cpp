/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: inetwork.cpp,v 1.19 2005/06/30 00:20:25 dcollins Exp $
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
#include "fsio.h"
#include "tcpio.h"
#include "udpio.h"
#include "vio.h"
#include "sockio.h"
#include "netbyte.h"
#include "cbqueue.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "chxpckts.h"
#include "dispatchq.h"
#include "inetwork_acceptor.h"
#include "inetwork.h"
#include "base_errmsg.h"
#include "access_ctrl.h"
#include "proc.h"
#include "proc_container.h"
#include "lbound_listenresp.h"  //for lbound connections
#include "servbuffer.h"
#include "shared_udp.h"
#include "nettypes.h" // for IOV_MAX

#include "server_engine.h"
#include "server_context.h"
#include "hxpiids.h"

/* XXXTDM
 * We must do recursive reads for now in order to keep performance acceptable.
 * Specifically, this minimizes the delay in responding to the initial request
 * on client sockets.  We eventually want to do the following:
 *
 *   1. Remove recursive reads.  This eliminates 50% of our TCP reads in the
 *      typical case, because the second read almost always returns EAGAIN.
 *
 *   2. Switch to the "thundering herd" listening socket model.  That is, put
 *      the listening socket into every streamer process and let them decide
 *      between themselves who is going to accept.  This eliminates several
 *      dispatches on the scheduler and a bunch of code.
 *
 *   3. Make the newclient code read a full packet on initial read.  This
 *      depends on (2) above because the current dispatch architecture only
 *      works when the select loop detects a read in the streamer proc after
 *      the client is dispatched.
 */
#define DO_RECURSIVE_READS 1

#ifdef PAULM_CLIENTAR
#include "objdbg.h"
#endif

#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#ifdef PAULM_TCPSC_LOSTADDREF
extern void handleLostAddref();
#endif
#endif

#ifdef PAULM_TCPSCTIMING
#include "classtimer.h"

void
_ExpiredTCPSocketContext(void* p)
{
    IHXTCPSocketContext* pp = (IHXTCPSocketContext*)p;

    printf("\tm_pTCPResponse(client) 0x%x\n", pp->m_pTCPResponse);

#ifdef PAULM_IHXTCPSCAR
    pp->DumpState();
#endif
}

ClassTimer g_TCPSCTimer("IHXTCPSocketContext",
    _ExpiredTCPSocketContext , 3600);
#endif

#ifdef PAULM_INTCPCTIMING
#include "classtimer.h"
ClassTimer g_INTCPCTimer("INetworkTCPSocketContext", 0, 3600);
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

#define UDP_MAX_VECTORS 16

const int UDP_CHUNK_SIZE = 1024;
const UINT32 MAX_UNDO_BYTES = 1024;

const char* LBOUNDSTR_CLIENT = "Client";
const char* LBOUNDSTR_SERVER = "Server";
const char* LBOUNDSTR_NOTLBOUND = "Not localbound";

const UINT32 MAX_LBOUND_READ_DEPTH = 100;

#define LBTYPE(x) \
    ( ((x) == LBOUNDT_CLIENT) ? LBOUNDSTR_CLIENT : ( ((x) == LBOUNDT_SERVER) ? LBOUNDSTR_SERVER : LBOUNDSTR_NOTLBOUND )  )
#define LBOUND_DEBUG 0x00200000

IHXNetworkServicesContext::IHXNetworkServicesContext() :
    m_lRefCount(0),
    m_pUnknown(0),
    m_pEngine(0),
    m_pMessages(0)
{
}

IHXNetworkServicesContext::~IHXNetworkServicesContext()
{
    if (m_pUnknown)
    {
        m_pUnknown->Release();
        m_pUnknown = 0;
    }

    if (m_pMessages)
    {
        m_pMessages->Release();
        m_pMessages = 0;
    }
}

STDMETHODIMP
IHXNetworkServicesContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetworkServices))
    {
        AddRef();
        *ppvObj = (IHXNetworkServices*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetworkServices2))
    {
        AddRef();
        *ppvObj = (IHXNetworkServices2*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
IHXNetworkServicesContext::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
IHXNetworkServicesContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
IHXNetworkServicesContext::Init
(
    IUnknown* pUnknown,
    Engine* pEngine,
    ServerAccessControl* accessCtrl
)
{
    m_pUnknown = pUnknown;

    if (m_pUnknown)
    {
        m_pUnknown->AddRef();

        /*
         * Query for the error message handler
         */
        m_pUnknown->QueryInterface(IID_IHXErrorMessages,
                                   (void**)&m_pMessages);
    }

    m_pEngine = pEngine;
    m_pAccessCtrl = accessCtrl;

    return HXR_OK;
}

STDMETHODIMP
IHXNetworkServicesContext::CreateUDPSocket(IHXUDPSocket** ppUDPSocket)
{
    *ppUDPSocket = new IHXUDPSocketContext(m_pEngine);
    if(*ppUDPSocket == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppUDPSocket)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
IHXNetworkServicesContext::CreateLBoundTCPSocket(IHXTCPSocket** ppTCPSocket)
{
    *ppTCPSocket = new IHXLBoundTCPSocketContext(m_pEngine);

    if(*ppTCPSocket == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppTCPSocket)->AddRef();

    return HXR_OK;
}


STDMETHODIMP
IHXNetworkServicesContext::CreateListenSocket(
    IHXListenSocket** ppListenSocket)
{
    *ppListenSocket = new IHXListenSocketContext(m_pEngine, m_pMessages,
                          m_pAccessCtrl);
    if(*ppListenSocket == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppListenSocket)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
INetworkServicesContext::CreateTCPSocket(IHXTCPSocket** ppTCPSocket)
{
    IHXResolver* pResolver;

    pResolver = new INetworkResolverContext();
    *ppTCPSocket = new INetworkTCPSocketContext(m_pEngine, pResolver);
    if(*ppTCPSocket == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppTCPSocket)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
INetworkServicesContext::CreateResolver(IHXResolver** ppResolver)
{
    *ppResolver = new INetworkResolverContext();
    if(*ppResolver == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppResolver)->AddRef();
    return HXR_OK;
}

IHXResolverContext::IHXResolverContext():
    m_lRefCount(0),
    m_pResp(0)
{
}

IHXResolverContext::~IHXResolverContext()
{
    if (m_pResp)
    {
        m_pResp->Release();
    }
}

STDMETHODIMP
IHXResolverContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXResolver))
    {
        AddRef();
        *ppvObj = (IHXResolver*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
IHXResolverContext::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
IHXResolverContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
IHXResolverContext::Init(IHXResolverResponse* pResp)
{
    m_pResp = pResp;
    m_pResp->AddRef();
    return HXR_OK;
}

STDMETHODIMP
INetworkResolverContext::GetHostByName(const char* pHostName)
{
    struct hostent* h = gethostbyname(pHostName);
    if (h)
    {
        m_pResp->GetHostByNameDone(HXR_OK, DwToHost(*(ULONG32*)h->h_addr));
    }
    else
    {
        m_pResp->GetHostByNameDone(HXR_FAIL, (ULONG32)-1);
    }

    return HXR_OK;
}

IHXTCPSocketContext::IHXTCPSocketContext(Engine* pEngine,
                                           IHXResolver* pResolver):
    m_lRefCount(0),
    m_pTCPResponse(0),
    m_pReadCallback(0),
    m_pWriteCallback(0),
    m_pConnectCallback(0),
    m_pEngine(pEngine),
    m_pCmd(0),
    m_pVio(0),
    m_pCtrl(0),
    m_bConnectCallbackPending(0),
    m_bWantWriteCallbackPending(0),
    m_bWriteFlushCallbackPending(0),
    m_bReadCallbackPending(0),
    m_bReadCallbackEnabled(0),
    m_nRequired(0),
    m_nConnectTimeoutID(0),
    m_nEnableReadID(0),
    m_nWriteFlushTimeoutID(0),
    m_bConnected(FALSE),
    m_lForeignAddress(0),
    m_sForeignPort(0),
    m_pResolver(pResolver),
    m_pUndoBuf(0),
    m_bTCPIOInitialized(FALSE),
    m_bInitCalled(FALSE),
    m_bReuseAddr(TRUE),
    m_bReusePort(FALSE),
    m_pWouldBlockResponse(NULL),
    m_bBufferedSocketClosed(FALSE),
    m_bWantWriteFromWB(FALSE),
    m_bWantWriteVFromWB(FALSE),
    m_bWantWriteVCallbackPending(FALSE),
    m_ulBufferedDataSize(0),
    m_ulBufferCount(0),
    m_nIOVectorSize(0),
    m_ulPacketSize(1200),
    m_ulFlushCount(0),
    m_bSupportsBufferedSocket(TRUE),
    m_bUseThreadSafeReadDone(TRUE),
    m_pVectorBuffers(NULL),
    m_pWriteVectors(NULL)
{
#ifdef PAULM_TCPSCTIMING
    g_TCPSCTimer.Add(this);
#endif
    if (m_pResolver)
    {
        m_pResolver->AddRef();
    }
}

IHXTCPSocketContext::IHXTCPSocketContext(Engine* pEngine,
                                           IHXResolver* pResolver,
                                           TCPIO* sock,
                                           UINT32 lForeignAddr, UINT16 nPort):
    m_lRefCount(0),
    m_pTCPResponse(0),
    m_pEngine(pEngine),
    m_pCmd(0),
    m_pVio(0),
    m_pCtrl(sock),
    m_pReadCallback(0),
    m_pWriteCallback(0),
    m_pConnectCallback(0),
    m_bConnectCallbackPending(0),
    m_bWantWriteCallbackPending(0),
    m_bWriteFlushCallbackPending(0),
    m_bReadCallbackPending(0),
    m_bReadCallbackEnabled(0),
    m_nRequired(0),
    m_nConnectTimeoutID(0),
    m_nEnableReadID(0),
    m_nWriteFlushTimeoutID(0),
    m_bConnected(TRUE),
    m_sForeignPort(nPort),
    m_lForeignAddress(lForeignAddr),
    m_pResolver(pResolver),
    m_pUndoBuf(0),
    m_bTCPIOInitialized(FALSE),
    m_bInitCalled(FALSE),
    m_bReuseAddr(TRUE),
    m_bReusePort(FALSE),
    m_pWouldBlockResponse(NULL),
    m_bBufferedSocketClosed(FALSE),
    m_bWantWriteFromWB(FALSE),
    m_bWantWriteVFromWB(FALSE),
    m_bWantWriteVCallbackPending(FALSE),
    m_ulBufferedDataSize(0),
    m_ulBufferCount(0),
    m_nIOVectorSize(0),
    m_ulPacketSize(1200),
    m_ulFlushCount(0),
    m_bSupportsBufferedSocket(TRUE),
    m_bUseThreadSafeReadDone(TRUE),
    m_pVectorBuffers(NULL),
    m_pWriteVectors(NULL)
{
#ifdef PAULM_TCPSCTIMING
    g_TCPSCTimer.Add(this);
#endif
    if (m_pCtrl)
    {
        m_pCmd = new FSIO(m_pCtrl, 256);
        m_pVio = new VIO(m_pCtrl);
    }

    if (m_pResolver)
    {
        m_pResolver->AddRef();
    }

    if (m_pCtrl)
    {
        if (m_pCtrl->get_mss(m_ulPacketSize) < 0)
        {
            //pick some default
            //this may result in a lot of partial writes
            //which will mean additional overhead on
            //certain types of links (PPPoE, PPP, L2TP tunnels, etc.)
            m_ulPacketSize = 1200;
        }
    }
}

IHXTCPSocketContext::~IHXTCPSocketContext()
{
#ifdef PAULM_TCPSCTIMING
    g_TCPSCTimer.Remove(this);
#endif

    if (m_pCtrl)
    {
        m_pEngine->callbacks.remove(HX_READERS, m_pCtrl);
        m_pEngine->callbacks.remove(HX_WRITERS, m_pCtrl);
        m_pEngine->UnRegisterSock();
        m_pCtrl = 0;
    }

    if (m_pTCPResponse)
    {
#ifdef PAULM_CLIENTAR
        REL_NOTIFY(m_pTCPResponse, 6);
#endif
        m_pTCPResponse->Release();
        m_pTCPResponse = 0;
    }

    if (m_pReadCallback)
    {
        m_pReadCallback->Release();
        m_pReadCallback = 0;
    }

    if (m_pWriteCallback)
    {
        m_pWriteCallback->Release();
        m_pWriteCallback = 0;
    }

    if (m_pConnectCallback)
    {
        m_pConnectCallback->Release();
        m_pConnectCallback = 0;
    }

    if (m_nConnectTimeoutID > 0)
    {
        m_pEngine->schedule.remove(m_nConnectTimeoutID);
    }

    if (m_nEnableReadID > 0)
    {
        if (m_bUseThreadSafeReadDone)
        {
            m_pEngine->ischedule.remove(m_nEnableReadID);
        }
        else
        {
            m_pEngine->schedule.remove(m_nEnableReadID);
        }
        m_nEnableReadID = 0;
    }

    if (m_nWriteFlushTimeoutID > 0)
    {
        m_pEngine->schedule.remove(m_nWriteFlushTimeoutID);
    }

    if (m_pResolver)
    {
        m_pResolver->Release();
        m_pResolver = 0;
    }

    if (m_pCmd)
    {
        delete m_pCmd;
        m_pCmd = 0;
    }

    if (m_pVio)
    {
        delete m_pVio;
        m_pVio = 0;
    }

    UINT32 i = 0;
    while (i < m_ulBufferCount)
    {
        HX_RELEASE(m_pVectorBuffers[i]);
        i++;
    }

    delete [] m_pVectorBuffers;
    delete [] m_pWriteVectors;

    if (m_pUndoBuf)
    {
        delete m_pUndoBuf;
        m_pUndoBuf = 0;
    }

    HX_RELEASE(m_pWouldBlockResponse);

    DPRINTF(0x10000000, ("IHXTCPSC(%p)::~IHXSTCPSC() end\n", this));
}

STDMETHODIMP
IHXTCPSocketContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXTCPSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXTCPSocket))
    {
        AddRef();
        *ppvObj = (IHXTCPSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXResolverResponse))
    {
        AddRef();
        *ppvObj = (IHXResolverResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSetSocketOption))
    {
        AddRef();
        *ppvObj = (IHXSetSocketOption*)this;
        return HXR_OK;
    }

#ifdef PAULM_IHXTCPSCAR
    else if (IsEqualIID(riid, IID_IHXObjDebugger))
    {
        *ppvObj = (IHXObjDebugger*)this;
        return HXR_OK;
    }
#endif

    else if (IsEqualIID(riid, IID_IHXBufferedSocket) &&
         m_bSupportsBufferedSocket)
    {
        AddRef();
        *ppvObj = (IHXBufferedSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXWouldBlock))
    {
        IHXTCPSocketContext* pNext = GetNextTCPSocketContext();
        if (pNext)
        {
            return pNext->QueryInterface(IID_IHXWouldBlock, ppvObj);
        }
        AddRef();
        *ppvObj = (IHXWouldBlock*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
IHXTCPSocketContext::AddRef()
{
#ifdef PAULM_IHXTCPSCAR
#ifdef PAULM_TCPSC_LOSTADDREF
    if (m_ulNextAddRef == 0)
    {
        handleLostAddref();
    }
#endif
    ((ObjDebugger*)this)->NotifyAddRef();
#endif
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
IHXTCPSocketContext::Release()
{
#ifdef PAULM_IHXTCPSCAR
    ((ObjDebugger*)this)->NotifyRelease();
#endif
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
IHXTCPSocketContext::Init(IHXTCPResponse* pTCPResponse)
{
    if(m_pTCPResponse)
    {
        m_pTCPResponse->Release();
    }
    m_pTCPResponse = pTCPResponse;

    IHXThreadSafeMethods* pTSMethods;
    if (HXR_OK == m_pTCPResponse->QueryInterface(IID_IHXThreadSafeMethods,
                                                 (void**)&pTSMethods))
    {
        m_bUseThreadSafeReadDone = HX_THREADSAFE_METHOD_SOCKET_READDONE &
                                   pTSMethods->IsThreadSafe();
        HX_RELEASE(pTSMethods);
    }
    else
    {
        m_bUseThreadSafeReadDone = FALSE;
    }

    Process* proc = (Process*)m_pEngine->get_proc();

    if (((IUnknown*)proc->pc->server_context)->QueryInterface(
        IID_IHXRegistry, (void**)&m_pReg) != HXR_OK)
    {
        return HXR_FAIL;
    }

#ifdef PAULM_CLIENTAR
    ADDR_NOTIFY(m_pTCPResponse, 2);
#endif
    m_pTCPResponse->AddRef();

    // XXXSMP What if this thing already exists?
    m_pUndoBuf = new CByteQueue((UINT16) MAX_UNDO_BYTES);

    // XXXSMP What if this thing is already inited?
    if (m_pCtrl)
    {
        if (m_pCtrl->get_mss(m_ulPacketSize) < 0)
        {
             INT32 nMaxPktSize = 0;

            //pick some default unless user has configured a default
            //this may result in a lot of partial writes
            //which will mean additional overhead on
            //certain types of links (PPPoE, PPP, L2TP tunnels, etc.)
            if (FAILED(m_pReg->GetIntByName("config.MaxPacketSize", nMaxPktSize)))
            {
                m_ulPacketSize = 1200;
            }
            else
            {
                m_ulPacketSize = nMaxPktSize;
            }
        }

        init();

        INT32 nSndBufSize = 0;

        if (FAILED(m_pReg->GetIntByName("config.TCPSendBufferSize", nSndBufSize)))
        {
            // 32K buffer size seems to fix the issue.
            nSndBufSize = 32768;

        }
        SetOption(HX_SOCKOPT_SET_SENDBUF_SIZE, nSndBufSize);

        m_bTCPIOInitialized = TRUE;
    }
    m_bInitCalled = TRUE;
    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::SetResponse(IHXTCPResponse* pTCPResponse)
{
    if (!m_bInitCalled)
    {
        Init(pTCPResponse);
    }
    else
    {
        if (m_pTCPResponse == pTCPResponse)
            return HXR_OK;
        else if (m_pTCPResponse && m_pTCPResponse != pTCPResponse)
        {
            m_pTCPResponse->Release();
        }
        m_pTCPResponse = pTCPResponse;
        m_pTCPResponse->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::Bind(UINT32 ulLocalAddr, UINT16 nPort)
{
    DPRINTF(D_INFO, ("IHXTCPSC::Bind(localAddr(%lu), port(%d))\n",
        ulLocalAddr, nPort));
    if (m_bTCPIOInitialized)
        return HXR_UNEXPECTED;

    if (m_pCtrl == 0)
    {
        m_pCtrl = new TCPIO;
        if (m_pCtrl == 0)
            return HXR_OUTOFMEMORY;
        m_pEngine->RegisterSock();

    INT32 ret = 0;
    if (ulLocalAddr == HXR_INADDR_ANY)
    {
        ret = m_pCtrl->init(INADDR_ANY, nPort, TRUE, m_bReuseAddr, m_bReusePort);
    }
    else
    {
        ret = m_pCtrl->init(ulLocalAddr, nPort, TRUE, m_bReuseAddr, m_bReusePort);
    }

    if (ret == -1)
    {
        m_bTCPIOInitialized = FALSE;
        delete m_pCtrl;
        m_pCtrl = 0;
        m_pEngine->UnRegisterSock();
        return HXR_FAIL;
    }
    else
    {
        init();
        m_bTCPIOInitialized = TRUE;
    }
    }
    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::Connect(const char*    pDestination,
                              UINT16        nPort)
{
    if (!m_bTCPIOInitialized)
    {
         if (m_pCtrl == 0)
         {
           m_pCtrl = new TCPIO;
           if (m_pCtrl == 0)
               return HXR_OUTOFMEMORY;
           m_pEngine->RegisterSock();

           INT32 ret = 0;
             ret = m_pCtrl->init(TRUE, FALSE, FALSE);

           if (ret == -1)
           {
               m_bTCPIOInitialized = FALSE;
               delete m_pCtrl;
               m_pCtrl = 0;
               m_pEngine->UnRegisterSock();
               return HXR_FAIL;
           }
           else
           {
               init();
               m_bTCPIOInitialized = TRUE;
           }
         }
    }

    m_sForeignPort = nPort;

    if(IsNumericAddr(pDestination, strlen(pDestination)))
    {
    UINT32 addr = inet_addr(pDestination);
        GetHostByNameDone(HXR_OK, DwToHost(addr));
    }
    else if (strcmp(pDestination, "localhost") == 0)
    {
        GetHostByNameDone(HXR_OK, DwToHost(inet_addr("127.0.0.1")));
    }
    else
    {
    if(!m_pResolver || m_bConnectCallbackPending)
        return HXR_UNEXPECTED;

    m_bConnectCallbackPending = TRUE;

    m_pResolver->Init((IHXResolverResponse*)this);
    m_pResolver->GetHostByName(pDestination);
    }
    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::GetHostByNameDone(HX_RESULT status, ULONG32 ulAddr)
{
    HX_RESULT result = HXR_OK;

    // the reason we need this is to prevent circular reference
    // the reason to put it at the begining instead of the end is
    // some ConnectDone call will result in "this" object being deleted.
    if (m_pResolver)
    {
    m_pResolver->Release();
    m_pResolver = 0;
    }

    if(status == HXR_OK)
    {
    m_lForeignAddress = ulAddr;

    // remove the existing Write callback
    m_pEngine->callbacks.remove(HX_WRITERS, m_pCtrl);
    // reuse descriptor for Connect callback
    m_pEngine->callbacks.add(HX_WRITERS, m_pCtrl,
        m_pConnectCallback);
//  m_pEngine->callbacks.disable(HX_WRITERS, m_pCtrl);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(m_lForeignAddress);
    addr.sin_port = htons(m_sForeignPort);

    if ((m_pCtrl->connect(&addr))
        && (m_pCtrl->error() != EWOULDBLOCK)
        && (m_pCtrl->error() != EINPROGRESS))
    {
        m_pTCPResponse->ConnectDone(HXR_FAILED);
        result = HXR_FAIL;
        goto GetHostDone;
    }

    // set connect timeout for 1 minute
    // XXXBAB - pass this as parameter?

    TCPSocketConnectTimeoutCallback* pCB =
        new TCPSocketConnectTimeoutCallback;
    pCB->m_pContext = this;

    // XXXGH...upped timeout to 10 seconds for splayer

    if(m_nConnectTimeoutID > 0)
    {
        m_pEngine->schedule.remove(m_nConnectTimeoutID);
    }

    m_nConnectTimeoutID = m_pEngine->schedule.enter(
        m_pEngine->now + Timeval(10.0), pCB);

    // enable callback for connection complete
    m_pEngine->callbacks.enable(HX_WRITERS, m_pCtrl);
    }
    else
    {
    m_pTCPResponse->ConnectDone(HXR_FAILED);
    }

GetHostDone:

    return result;
}

STDMETHODIMP
IHXTCPSocketContext::GetForeignAddress(UINT32& lAddress)
{
    if(m_bConnected && m_lForeignAddress)
    {
    lAddress = m_lForeignAddress;
    return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
IHXTCPSocketContext::GetLocalAddress(UINT32& lAddress)
{
    sockaddr_in addr;
    INT32 addrLen = sizeof(sockaddr_in);
    if(m_pCtrl->getsockname(&addr, &addrLen) == 0)
    {
    lAddress = DwToHost(addr.sin_addr.s_addr);
    return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
IHXTCPSocketContext::GetForeignPort(UINT16& port)
{
    if(m_bConnected)
    {
    port = m_sForeignPort;
    return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
IHXTCPSocketContext::GetLocalPort(UINT16& port)
{
    sockaddr_in addr;
    INT32 addrLen = sizeof(sockaddr_in);
    if(m_pCtrl->getsockname(&addr, &addrLen) == 0)
    {
    port = ntohs(addr.sin_port);
    return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
IHXTCPSocketContext::TCPSocketEnableReadCallback::Func()
{
    if(m_pContext)
    {
    /*
     * Must protect the m_pContext object from being deleted while
     * it's doing business
     */
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(m_pContext, 5);
#endif
    m_pContext->AddRef();
    m_pContext->m_nEnableReadID = 0;
    m_pContext->DoRead();
#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(m_pContext, 4);
#endif
    m_pContext->Release();
    }
    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::TCPSocketReadCallback::Func()
{
    if(m_pContext)
    {
    HX_RESULT result;

    /*
     * Must protect the m_pContext object from being deleted while
     * it's doing business
     */
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(m_pContext, 7);
#endif
    m_pContext->AddRef();
    m_pContext->m_bReadCallbackPending = FALSE;

    result = m_pContext->DoRead();

        if (!m_pContext->m_bReadCallbackPending)
        {
        m_pContext->m_pEngine->callbacks.disable(HX_READERS,
                                                 m_pContext->m_pCtrl);
            m_pContext->m_bReadCallbackEnabled = FALSE;
        }

#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(m_pContext, 5);
#endif
    m_pContext->Release();
    return result;
    }
    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::TCPSocketWriteCallback::Func()
{
    if(m_pContext)
    {
    /*
     * Must protect the m_pContext object from being deleted while
     * it's doing business
     */
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(m_pContext, 17);
#endif
    m_pContext->AddRef();
    m_pContext->DoWrite();
#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(m_pContext, 16);
#endif
    m_pContext->Release();
    }
    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::TCPSocketConnectCallback::Func()
{
  if(m_pContext)
  {
    /*
     * Must protect the m_pContext object from being deleted while
     * it's doing business
     */
    m_pContext->AddRef();
    m_pContext->disableConnectCallback();
    
    INT32 rc = HXR_FAILED;
    UINT32 my_err = HXR_FAILED;

    rc = m_pContext->m_pCtrl->get_err(my_err);

    if (rc == HXR_OK && my_err != HXR_OK)
    {
      m_pContext->m_nConnectTimeoutID = 0;
      m_pContext->m_bConnected = FALSE;
      m_pContext->m_pTCPResponse->ConnectDone(HXR_FAILED);
      m_pContext->Release();
      return HXR_OK;
    }
    m_pContext->m_bConnected = TRUE;
    m_pContext->m_pTCPResponse->ConnectDone(HXR_OK);
    m_pContext->Release();
  }
  return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::TCPSocketConnectTimeoutCallback::Func()
{
    if(m_pContext)
    {
    /*
     * Must protect the m_pContext object from being deleted while
     * it's doing business
     */
    m_pContext->AddRef();
        // Null ID so disableConnectCallback won't remove it
    m_pContext->m_nConnectTimeoutID = 0;
    m_pContext->disableConnectCallback();
    m_pContext->m_bConnected = FALSE;
    m_pContext->m_pTCPResponse->ConnectDone(HXR_FAILED);
    m_pContext->Release();
    }
    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::TCPSocketWriteFlushTimeoutCallback::Func()
{
    if(m_pContext)
    {
    m_pContext->m_bWriteFlushCallbackPending = FALSE;
    m_pContext->m_nWriteFlushTimeoutID = 0;
#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(m_pContext, 17);
#endif
    m_pContext->Release();
    }
    return HXR_OK;
}

void
IHXTCPSocketContext::disableConnectCallback()
{
    m_bConnectCallbackPending = FALSE;
    m_pEngine->callbacks.remove(HX_WRITERS, m_pCtrl);
    // re-add the Write callback
    m_pEngine->callbacks.add(HX_WRITERS, m_pCtrl,
    m_pWriteCallback);
    m_pEngine->callbacks.disable(HX_WRITERS, m_pCtrl);
    if(m_nConnectTimeoutID > 0)
    {
    m_pEngine->schedule.remove(m_nConnectTimeoutID);
    m_nConnectTimeoutID = 0;
    }
}

void
IHXTCPSocketContext::init()
{
    m_pCtrl->nonblocking();

    if (m_pCmd == NULL)
    {
    m_pCmd = new FSIO(m_pCtrl, 256);
    }
    if (m_pVio == NULL)
    {
    m_pVio = new VIO(m_pCtrl);
    }
    m_pReadCallback = new TCPSocketReadCallback;
    m_pReadCallback->AddRef();
    m_pReadCallback->m_pContext = this;
    // setup select callback
    m_pEngine->callbacks.add(HX_READERS, m_pCtrl,
    m_pReadCallback, m_bUseThreadSafeReadDone);
    m_pEngine->callbacks.disable(HX_READERS, m_pCtrl);

    m_pWriteCallback = new TCPSocketWriteCallback;
    m_pWriteCallback->AddRef();
    m_pWriteCallback->m_pContext = this;
    // setup select callback
    m_pEngine->callbacks.add(HX_WRITERS, m_pCtrl,
    m_pWriteCallback);
    m_pEngine->callbacks.disable(HX_WRITERS, m_pCtrl);

    // NOTE - if Connect is called, the WriteCallback
    // is removed from the select and the ConnectCallback
    // is added. When the connect completes (or timeout occurs),
    // the ConnectCallback is removed and the WriteCallback added
    // again.
    m_pConnectCallback = new TCPSocketConnectCallback;
    m_pConnectCallback->AddRef();
    m_pConnectCallback->m_pContext = this;
}

STDMETHODIMP
IHXTCPSocketContext::SetOption(HX_SOCKET_OPTION option, UINT32 ulValue)
{
    INT32 ret = 0;
    switch(option)
    {
    case HX_SOCKOPT_REUSE_ADDR:
    m_bReuseAddr = (BOOL)ulValue;
    if (m_pCtrl)
        ret = m_pCtrl->reuse_addr(m_bReuseAddr);
    break;
    case HX_SOCKOPT_REUSE_PORT:
    m_bReusePort = (BOOL)ulValue;
    if (m_pCtrl)
        ret = m_pCtrl->reuse_port(m_bReusePort);
    break;
    case HX_SOCKOPT_SET_SENDBUF_SIZE:
        DPRINTF(D_INFO, ("IRMATCPSC - setting sndbufsize - %d\n", ulValue));

        if (m_pCtrl)
            ret = m_pCtrl->set_send_size(ulValue);
    break;
    case  HX_SOCKOPT_IP_TOS:
    if (m_pCtrl)
    {
        ret = m_pCtrl->set_ip_tos(ulValue);
    }
    break;
    default:
    HX_ASSERT(!"I don't know this option");
    ret = -1;
    }

    if (ret < 0)
    {
    return HXR_FAILED;
    }
    else
    {
    return HXR_OK;
    }
}

STDMETHODIMP
IHXTCPSocketContext::WantWouldBlock(IHXWouldBlockResponse* pResp,
                UINT32 ID)
{
    HX_ASSERT(!m_pWouldBlockResponse || m_pWouldBlockResponse == pResp);
    HX_RELEASE(m_pWouldBlockResponse);
    m_pWouldBlockResponse = pResp;
    m_pWouldBlockResponse->AddRef();
    m_ulWouldBlockResponseID = ID;
    return HXR_OK;
}


/*
 * INetworkTCPSocketContext methods
 */
INetworkTCPSocketContext::INetworkTCPSocketContext(Engine* pEngine,
                                                   IHXResolver* pResolver):
    IHXTCPSocketContext(pEngine, pResolver)
{
#ifdef PAULM_INTCPCTIMING
    g_INTCPCTimer.Add(this);
#endif
}

INetworkTCPSocketContext::INetworkTCPSocketContext(Engine* pEngine,
                                                   IHXResolver* pResolver,
                                                   TCPIO* sock,
                                                   UINT32 lForeignAddr,
                                                   UINT16 nPort):
    IHXTCPSocketContext(pEngine, pResolver, sock, lForeignAddr, nPort)
{
#ifdef PAULM_INTCPCTIMING
    g_INTCPCTimer.Add(this);
#endif
}

INetworkTCPSocketContext::~INetworkTCPSocketContext()
{
#ifdef PAULM_INTCPCTIMING
    g_INTCPCTimer.Remove(this);
#endif
}

STDMETHODIMP
INetworkTCPSocketContext::Read(UINT16   Size)
{
    if(m_bReadCallbackPending)
    {
    return HXR_UNEXPECTED;
    }

    m_nRequired = Size;

#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(this, 4);
#endif
    AddRef();

#ifdef DO_RECURSIVE_READS
    HX_RESULT ret = DoRead();
#else
    // Don't recurse for TCP reads
    m_bReadCallbackPending = TRUE;
    if (!m_bReadCallbackEnabled)
    {
        m_bReadCallbackEnabled = TRUE;
        m_pEngine->callbacks.enable(HX_READERS, m_pCtrl, m_bUseThreadSafeReadDone);
    }
    HX_RESULT ret = HXR_OK;
#endif

#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(this, 1);
#endif
    Release();

    return ret;
}

STDMETHODIMP
INetworkTCPSocketContext::Write(IHXBuffer* pBuffer)
{
    BYTE*   pData;
    int  writeLen;
    int  allocLen = writeLen = pBuffer->GetSize();

#ifdef SCRVERBOSE
    fprintf(stderr, "YIPES wrong write!\n");
#endif
    pData = m_pCmd->write_alloc(allocLen);
    if ( (pData == 0) || (allocLen < writeLen) )
    {
    DPRINTF(D_INFO, ("write_alloc() failed\n"));
    return HXR_FAIL;
    }

    memcpy(pData, pBuffer->GetBuffer(), writeLen);

    if (m_pCmd->write_free(pData) != 0)
    {
    DPRINTF(D_INFO, ("write_free() failed\n"));
    return HXR_FAIL;
    }

    // Schedule flush_tcp if there is any unwritten data.

    if (!m_bWriteFlushCallbackPending && m_pCmd->write_flush_needed())
    {
    if (write_flush() < 0)
    {
        DPRINTF(D_INFO, ("write_flush() failed\n"));
        return HXR_FAIL;
    }
   }
   if (m_pCmd->get_would_block())
   {
       HXAtomicIncUINT32(g_pWouldBlockCount);
       if (m_pWouldBlockResponse)
       {
       if (HXR_OK == m_pWouldBlockResponse->WouldBlock(
               m_ulWouldBlockResponseID))
       {
           m_bWantWriteFromWB = TRUE;
           WantWrite();
       }
       }
   }

    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::SetDesiredPacketSize(UINT32 ulPacketSize)
{
    m_ulPacketSize = ulPacketSize;

    if (m_bBufferedSocketClosed)
    {
        return HXR_FAIL;
    }

    if (m_ulPacketSize < m_ulBufferedDataSize && m_ulBufferedDataSize)
    {
    DoWriteV();
    }

    return HXR_OK;
}

STDMETHODIMP
IHXTCPSocketContext::FlushWrite()
{
#ifdef SCRVERBOSE
    fprintf(stderr, "FlushWrite (%d, %d)\n", m_ulBufferCount, m_ulBufferedDataSize);
#endif
    m_ulFlushCount = m_ulBufferCount;
    if (m_ulBufferCount)
    {
    DoWriteV();
    }

    return HXR_OK;
}

#define FIRST_ALLOC_SIZE 28

STDMETHODIMP
IHXTCPSocketContext::BufferedWrite(IHXBuffer* pBuffer)
{
    if (m_bBufferedSocketClosed)
    {
        return HXR_FAIL;
    }

    if (m_ulBufferCount == m_nIOVectorSize)
    {
        /*
         * allocate buffers only when needed.
         * increase allocation in two steps - the first step should
         * be enough for most cases, I think.
         */

        if (m_nIOVectorSize == 0)
        {
            m_nIOVectorSize = FIRST_ALLOC_SIZE;
            m_pVectorBuffers = (IHXBuffer**) new IHXBuffer* [m_nIOVectorSize];
            m_pWriteVectors = (HX_IOVEC*) new HX_IOVEC [m_nIOVectorSize];
        }
        else if (m_nIOVectorSize == FIRST_ALLOC_SIZE)
        {
            // increase vector storage to HX_IOV_MAX

            m_nIOVectorSize = HX_IOV_MAX;

            IHXBuffer** pBuffers =
                (IHXBuffer**) new IHXBuffer* [m_nIOVectorSize];
            HX_IOVEC* pVectors =
                (HX_IOVEC*) new HX_IOVEC [m_nIOVectorSize];

            memcpy(pBuffers, m_pVectorBuffers,
                   FIRST_ALLOC_SIZE * sizeof(IHXBuffer*));
            memcpy(pVectors, m_pWriteVectors,
                   FIRST_ALLOC_SIZE * sizeof(HX_IOVEC));

            delete [] m_pVectorBuffers;
            delete [] m_pWriteVectors;

            m_pVectorBuffers = pBuffers;
            m_pWriteVectors = pVectors;
        }
        else
        {
            // okay, we've blown out the vector storage.  Mark the error state
            // and let the error take its course.

            m_bBufferedSocketClosed = TRUE;
            FlushWrite();
        }
    }

    if (m_ulBufferCount >= m_nIOVectorSize)
    {
#ifdef DEBUG
    UINT32 i = 0;
    fprintf(stderr, "_______OVERFLOW__________\n");
    fprintf(stderr, "%p %d %d %d %d\n", this, m_bWantWriteCallbackPending,
        m_ulBufferCount, m_ulBufferedDataSize, m_ulPacketSize);

    while (i < m_ulBufferCount)
    {
        fprintf(stderr, "%d ", m_pWriteVectors[i].iov_len);
        i++;
    }
    fprintf(stderr, "\n");
    HX_ASSERT(0);
#endif
    // Someone's ignoring WouldBlock most likely...
    m_pTCPResponse->Closed(HXR_BLOCKED);
    return HXR_FAIL;
    }
#ifdef SCRVERBOSE
    fprintf(stderr, "BufferedWrite (%d)\n", pBuffer->GetSize());
#endif
    m_pVectorBuffers[m_ulBufferCount] = pBuffer;
    pBuffer->AddRef();

    m_pWriteVectors[m_ulBufferCount].iov_base = (char*)pBuffer->GetBuffer();
    m_pWriteVectors[m_ulBufferCount].iov_len = pBuffer->GetSize();

    m_ulBufferCount++;
    m_ulBufferedDataSize += pBuffer->GetSize();

    if (m_ulBufferedDataSize >= m_ulPacketSize ||
        m_ulBufferCount >= HX_IOV_MAX)
    {
    DoWriteV();
    }

    return HXR_OK;
}

void
IHXTCPSocketContext::DoWriteV(BOOL bOnCleared)
{
    UINT32 ulPartialBufferSize = 0;
    UINT32 ulRemainingBufferSize = 0;
    UINT32 ulVectorCount = m_ulBufferCount;
    UINT32 ulByteCount = m_ulBufferedDataSize;
    BOOL bCleared = TRUE;
    int bytesWritten = 0;

#ifdef SCRVERBOSE
    if (m_bWantWriteCallbackPending)
    {
        fprintf(stderr, "%p:DoWriteV, write callback %s\n", this,
                                     bOnCleared ?  "cleared" : "pending");
    }
    else
    {
        fprintf(stderr, "%p:DoWriteV\n", this);
    }
#endif

    while (((m_ulBufferedDataSize > m_ulPacketSize) ||
            (m_ulBufferCount >= HX_IOV_MAX) ||
            (m_ulBufferedDataSize && m_ulFlushCount)) &&
           (!m_bWantWriteCallbackPending || bOnCleared))
    {
        ulPartialBufferSize = 0;
        ulRemainingBufferSize = 0;
        ulVectorCount = m_ulBufferCount;
        ulByteCount = m_ulBufferedDataSize;

        if (m_ulPacketSize && m_ulBufferedDataSize > m_ulPacketSize)
        {
            ulByteCount = 0;
            ulVectorCount = 0;

            // Even though m_nIOVectorSize can be less than HX_IOV_MAX,
            // if m_ulBufferedDataSize > m_ulPacketSize then we shouldn't
            // exceed m_nIOVectorSize here.

            while (ulByteCount < m_ulPacketSize &&
                   ulVectorCount < HX_IOV_MAX)
            {
                ulByteCount += m_pWriteVectors[ulVectorCount].iov_len;
                ulVectorCount++;
            }

            if (ulByteCount > m_ulPacketSize)
            {
                ulRemainingBufferSize = ulByteCount - m_ulPacketSize;

                ulPartialBufferSize =
                    m_pWriteVectors[ulVectorCount-1].iov_len -
                    ulRemainingBufferSize;

                m_pWriteVectors[ulVectorCount-1].iov_len = ulPartialBufferSize;
                ulByteCount -= ulRemainingBufferSize;
            }
        }
        else if (m_ulBufferCount >= HX_IOV_MAX)
        {
            ulVectorCount = HX_IOV_MAX;

            UINT32 i = 0;
            ulByteCount = 0;

            while (i < ulVectorCount)
            {
                ulByteCount += m_pWriteVectors[i].iov_len;
                i++;
            }
        }

#ifdef SCRVERBOSE
    UINT32 i = 0;
    while (i < ulVectorCount)
    {
        fprintf(stderr, "%d ", m_pWriteVectors[i].iov_len);
        i++;
    }
#endif
    if (ulVectorCount > m_ulBufferCount)
    {
        //HX_ASSERT(0);
#ifdef SCRVERBOSE
        fprintf(stderr, "Bad mismatch in DoWriteV\n");
#endif
        m_bBufferedSocketClosed = TRUE;
        m_pTCPResponse->Closed(HXR_FAIL);
        return;
    }
#ifdef SCRVERBOSE
        fprintf(stderr, "\n");
        fprintf(stderr, "DoWriteV (%d, %d, %d, %d, %d, %d) MAX %d\n", ulVectorCount, ulByteCount, ulPartialBufferSize, ulRemainingBufferSize, m_ulBufferCount, m_ulBufferedDataSize, HX_IOV_MAX);
#endif
        bytesWritten = m_pVio->writevec(m_pWriteVectors, ulVectorCount);
    if (bytesWritten < 0)
    {
#ifdef SCRVERBOSE
        fprintf(stderr, "Closing socket, writev returned %d\n", bytesWritten);
#endif
        m_bBufferedSocketClosed = TRUE;
        m_pTCPResponse->Closed(HXR_FAIL);
        return;
    }

        // update the book keeping now.
        if (bytesWritten)
        {
            //check for partial write first
            if(bytesWritten != ulByteCount)
            {
                if(ulRemainingBufferSize)
                {
                    //restore the last buffer to original length
                    m_pWriteVectors[ulVectorCount-1].iov_len =
                        ulRemainingBufferSize + ulPartialBufferSize;

                    ulPartialBufferSize = 0;
                    ulRemainingBufferSize = 0;
                }

                //recalculate ulByteCount and ulVectorCount
                ulByteCount = 0;
                ulVectorCount = 0;
                while (ulByteCount < bytesWritten)
                {
                    ulByteCount += m_pWriteVectors[ulVectorCount].iov_len;
                    ulVectorCount++;
                }

                if (ulByteCount != bytesWritten)
                {
                    ulRemainingBufferSize = ulByteCount - bytesWritten;

                    ulPartialBufferSize =
                         m_pWriteVectors[ulVectorCount-1].iov_len -
                         ulRemainingBufferSize;

                    ulByteCount = bytesWritten;
                }
            }

        if (ulPartialBufferSize)
        {
            m_pWriteVectors[ulVectorCount-1].iov_base
            = (char*) m_pWriteVectors[ulVectorCount-1].iov_base +
                      ulPartialBufferSize;
            m_pWriteVectors[ulVectorCount-1].iov_len
            = ulRemainingBufferSize;
            ulVectorCount--;
        }

        UINT32 i = 0;
        while (i < ulVectorCount)
        {
            HX_RELEASE(m_pVectorBuffers[i]);
            i++;
        }

        if (m_ulBufferCount > ulVectorCount)
        {
            memmove(m_pWriteVectors, m_pWriteVectors + ulVectorCount,
            sizeof(HX_IOVEC) * (m_ulBufferCount - ulVectorCount));
            memmove(m_pVectorBuffers, m_pVectorBuffers + ulVectorCount,
            sizeof(IHXBuffer*) * (m_ulBufferCount - ulVectorCount));
        }
        else
        {
        //Shouldn't get here
        ulVectorCount = m_ulBufferCount;
        }

        m_ulBufferedDataSize -= ulByteCount;
        m_ulBufferCount -= ulVectorCount;

        if (m_ulFlushCount > ulVectorCount)
        {
            m_ulFlushCount -= ulVectorCount;
        }
        else
        {
            m_ulFlushCount = 0;
        }
        }
        else
        {
            if(ulPartialBufferSize)
            {
                //restore the last buffer to original length
                m_pWriteVectors[ulVectorCount-1].iov_len +=
                    ulRemainingBufferSize;
            }
        }

        if (m_pVio->would_block())
    {
            HXAtomicIncUINT32(g_pWouldBlockCount);
#ifdef SCRVERBOSE
        fprintf(stderr, "%p:DoWriteV would block, pWBResponse %p\n ",
                this, m_pWouldBlockResponse);
#endif
        if (m_pWouldBlockResponse)
        {
           if (HXR_OK == m_pWouldBlockResponse->WouldBlock(
               m_ulWouldBlockResponseID))
           {
           m_bWantWriteVFromWB = TRUE;
           }
        }
        WantWrite();
        m_bWantWriteVCallbackPending = TRUE;
            bCleared = FALSE;
            bOnCleared = FALSE;
    }
    }
#ifdef SCRVERBOSE
    if (m_ulBufferCount)
    {
    fprintf(stderr, "0buf - %d\n", m_pWriteVectors[0].iov_len);
    }
#endif
    if (bOnCleared && bCleared && m_bWantWriteVCallbackPending)
    {
#ifdef SCRVERBOSE
        fprintf(stderr, "%p:DoWriteV, WriteCallback received, WBResponse %p\n",
                               this, m_pWouldBlockResponse);
#endif
    m_pEngine->callbacks.disable(HX_WRITERS, m_pCtrl);
    m_bWantWriteVCallbackPending = FALSE;
    m_bWantWriteCallbackPending = FALSE;

    if (m_bWantWriteVFromWB)
    {
        m_bWantWriteVFromWB = FALSE;
        if (m_pWouldBlockResponse)
        {
        m_pWouldBlockResponse->WouldBlockCleared(
            m_ulWouldBlockResponseID);
        }
    }
    else if (m_bWantWriteVCallbackPending)
    {
        m_pTCPResponse->WriteReady(HXR_OK);
    }
    }


}

STDMETHODIMP
INetworkTCPSocketContext::WantWrite()
{
    if(m_bWantWriteCallbackPending)
    return HXR_UNEXPECTED;
    m_bWantWriteCallbackPending = TRUE;
    m_pEngine->callbacks.enable(HX_WRITERS, m_pCtrl);
    return HXR_OK;
}

HX_RESULT
INetworkTCPSocketContext::DoRead()
{
    BYTE* pData;
    int NumRead = m_nRequired;

    if ((pData = m_pCmd->read_alloc(NumRead)) != NULL)
    {
        if (NumRead > 0)
        {
            Process* proc = (Process*)m_pEngine->get_proc();
            IHXBuffer* pBuffer = new ServerBuffer(TRUE);

            UINT32 undoByteLen = m_pUndoBuf->GetQueuedItemCount();
            if(undoByteLen > 0)
            {
                pBuffer->SetSize(undoByteLen + NumRead);
                m_pUndoBuf->DeQueue(pBuffer->GetBuffer(), (UINT16) undoByteLen);
                memcpy(pBuffer->GetBuffer() + undoByteLen, pData, NumRead);
            }
            else
            {
                pBuffer->Set(pData, NumRead);
            }
#ifdef PAULM_IHXTCPSCAR
            ADDR_NOTIFY(this, 15);
#endif
            m_pTCPResponse->ReadDone(HXR_OK, pBuffer);
#ifdef PAULM_IHXTCPSCAR
            /*
             * Nullify last if ReadDone does not addref.
             */
            ADDR_NOTIFY(this, 0);
#endif
            pBuffer->Release();
            pBuffer = 0;
        }
        else
        {
            m_pCmd->read_undo(pData, NumRead);
            m_bReadCallbackPending = TRUE;
            if (!m_bReadCallbackEnabled)
            {
                m_bReadCallbackEnabled = TRUE;
                m_pEngine->callbacks.enable(HX_READERS, m_pCtrl, m_bUseThreadSafeReadDone);
            }
            return HXR_OK;
        }

        if(m_pCmd)
            m_pCmd->read_free(pData);

        return HXR_OK;
    }
    else
    {
    /*
     * If we are waiting to flush a Write(), then clear it because it
     * will never happen
     */

    if (m_bWriteFlushCallbackPending)
    {
        m_bWriteFlushCallbackPending = FALSE;
        m_pEngine->callbacks.disable(HX_WRITERS, m_pCtrl);

        if (m_nWriteFlushTimeoutID > 0)
        {
        m_pEngine->schedule.remove(m_nWriteFlushTimeoutID);
        m_nWriteFlushTimeoutID = 0;
        }

#ifdef PAULM_IHXTCPSCAR
        REL_NOTIFY(this, 14);
#endif
        Release();
    }

    m_pEngine->callbacks.disable(HX_READERS, m_pCtrl);
#if notyet
    m_pTCPResponse->Closed(HXR_FAILED);
#endif

#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(this, 16);
#endif
    m_pTCPResponse->ReadDone(HXR_FAILED, 0);
#ifdef PAULM_IHXTCPSCAR
    /*
     * Nullify last if ReadDone does not addr
     */
    ADDR_NOTIFY(this, 0);
#endif
    }
    return HXR_FAIL;
}

/*
 *        WouldBlock in Write()             WriteFlush
 *           m_bWantWriteFromWB = 1        / (AddRef)
 *                |                      /
 *                |                    /
 *        WantWrite                  /
 *               \                 /
 *                \              /
 *                    DoWrite
 *                             (Release)
 */
void
INetworkTCPSocketContext::DoWrite()
{
    /*
     * It is possible for both write callbacks to be pending
     */

    if (m_bWantWriteVCallbackPending)
    {
#ifdef SCRVERBOSE
        fprintf(stderr, "%p:DoWrite calling DoWriteV after write callback\n", this);
#endif
        DoWriteV(TRUE);
        return;
    }

    if (m_bWriteFlushCallbackPending || m_bWantWriteCallbackPending)
    {
    BOOL bRelease = FALSE;
    /*
     * First clear any pending timeouts
     */

    if (m_nWriteFlushTimeoutID > 0)
    {
        m_pEngine->schedule.remove(m_nWriteFlushTimeoutID);
        m_nWriteFlushTimeoutID = 0;
    }

    if (m_bWriteFlushCallbackPending)
    {
        bRelease = TRUE;
        m_bWriteFlushCallbackPending = FALSE;
    }

    if (m_pCmd->write_flush_needed())
    {
        int ret = write_flush();
        /*
         * if we are just stuck on a would block, then
         * return now.
         */
        if (m_pCmd->get_would_block())
        {
                HXAtomicIncUINT32(g_pWouldBlockCount);
        WriteFlushTimeout();
        if( bRelease ) Release();
        return;
        }

        if (ret < 0)
        {
        m_pTCPResponse->Closed(HXR_FAILED);
        }
    }

#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(this, 15);
#endif
    /*
     * This Release() matches up with the AddRef() in write_flush()
     */
    if (bRelease)
    {
        Release();
    }
    }

    /*
     * If we get here we know that we have handled all pending
     * data for the write.
     */
    m_pEngine->callbacks.disable(HX_WRITERS, m_pCtrl);


    /*
     * XXX PM These two methods of requesting WriteReady should
     * NEVER by used on the same object!
     */
    /* XXXJC : m_bWantWriteFromWB is not needed : it gets set to true and
     * never cleared. For the next release it should be removed and this
     * logic changed to be :
     *
     * if (m_bWantWriteCallbackPending)
     * {
     *     m_bWantWriteCallbackPending = FALSE;
     *
     *     if (m_pWouldBlockResponse)
     *     {
     *         m_pWouldBlockResponse->WouldBlockCleared(m_ulWouldBlockResponseID);
     *     }
     *     else
     *     {
     *         m_bWantWriteCallbackPending = FALSE;
     *         m_pTCPResponse->WriteReady(HXR_OK);
     *     }
     * }
     */
    if (m_bWantWriteFromWB)
    {
        m_bWantWriteCallbackPending = FALSE;

    if (m_pWouldBlockResponse)
    {
        m_pWouldBlockResponse->WouldBlockCleared(m_ulWouldBlockResponseID);
    }
    }
    else if (m_bWantWriteCallbackPending)
    {
    m_bWantWriteCallbackPending = FALSE;
    m_pTCPResponse->WriteReady(HXR_OK);
    }
}

void
INetworkTCPSocketContext::enableRead()
{
    /*XXXJJ comment it out. TCPSocketEnableReadCallback will call it
      eventually, if there are no data on the stack*/
    //m_pEngine->callbacks.enable(HX_READERS, m_pCtrl);

    /*
     * there may be data left in the SIO buffer, so schedule
     * a callback to read it.
     */

    if (m_nEnableReadID > 0)
    {
        if (m_bUseThreadSafeReadDone)
        {
            m_pEngine->ischedule.remove(m_nEnableReadID);
        }
        else
        {
            m_pEngine->schedule.remove(m_nEnableReadID);
        }
    }

    TCPSocketEnableReadCallback* pCB = new TCPSocketEnableReadCallback;
    pCB->m_pContext = this;

    if (m_bUseThreadSafeReadDone)
    {
        //callback is threadsafe so put it on the scheduler that
        //won't lock the mutex before making the callback
        m_nEnableReadID = m_pEngine->ischedule.enter(0.0, pCB);
    }
    else
    {
        m_nEnableReadID = m_pEngine->schedule.enter(0.0, pCB);
    }
}

void
INetworkTCPSocketContext::disableRead()
{
    m_bReadCallbackEnabled = FALSE;
    m_pEngine->callbacks.disable(HX_READERS, m_pCtrl);
}

int
INetworkTCPSocketContext::readUndo(BYTE* pData, UINT32 nDataLen)
{
    return m_pUndoBuf->EnQueue(pData, (UINT16) nDataLen);
}

void
INetworkTCPSocketContext::disconnect()
{
    m_pEngine->callbacks.remove(HX_READERS, m_pCtrl);
    m_pEngine->callbacks.remove(HX_WRITERS, m_pCtrl);
    m_pEngine->UnRegisterSock();
}

void
INetworkTCPSocketContext::reconnect(Engine* pEngine)
{
    m_pEngine = pEngine;

    m_pEngine->callbacks.add(HX_WRITERS, m_pCtrl, m_pWriteCallback);
    m_pEngine->callbacks.disable(HX_WRITERS, m_pCtrl);
    m_pEngine->callbacks.add(HX_READERS, m_pCtrl, m_pReadCallback, m_bUseThreadSafeReadDone);
    m_pEngine->callbacks.disable(HX_READERS, m_pCtrl);
    m_pEngine->RegisterSock();
}

int
INetworkTCPSocketContext::write_flush()
{
    if (m_pCmd->write_flush() < 0)
    {
        return -1;
    }

    /*
     * Do not let this socket context be killed while there is still
     * data to write
     */

    if (m_pCmd->write_flush_needed())
    {
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(this, 13);
#endif
    AddRef();
    m_bWriteFlushCallbackPending = TRUE;
    m_pEngine->callbacks.enable(HX_WRITERS, m_pCtrl);

    WriteFlushTimeout();

    }

    return 0;
}

void
INetworkTCPSocketContext::WriteFlushTimeout()
{
    /*
     * Set a timeout so that we don't hang waiting for the write
     * to flush
     */
    if (m_nWriteFlushTimeoutID == 0)
    {
    TCPSocketWriteFlushTimeoutCallback* pCB =
        new TCPSocketWriteFlushTimeoutCallback;
    pCB->m_pContext = this;

    m_nWriteFlushTimeoutID =
        m_pEngine->schedule.enter(m_pEngine->now + Timeval(300.0), pCB);
    }

}

/*
 * IHXListenSocketContext methods
 */

IHXListenSocketContext::IHXListenSocketContext
(
    Engine* pEngine,
    IHXErrorMessages* pMessages,
    ServerAccessControl* pAccessCtrl
):  m_lRefCount(0),
    m_pEngine(pEngine),
    m_pMessages(pMessages),
    m_pListenResponse(0),
    m_ppAcceptors(0),
    m_ulNumAcceptors(0),
    m_pAccessCtrl(pAccessCtrl)
{
    if (m_pMessages)
    {
        m_pMessages->AddRef();
    }
}

IHXListenSocketContext::~IHXListenSocketContext()
{
    if (m_pMessages)
    {
        m_pMessages->Release();
        m_pMessages = 0;
    }

    if (m_pListenResponse)
    {
        m_pListenResponse->Release();
        m_pListenResponse = 0;
    }

    UINT32 ul;
    for (ul = 0; ul < m_ulNumAcceptors; ul++)
    {
        delete m_ppAcceptors[ul];
    }
    delete[] m_ppAcceptors;
}

STDMETHODIMP
IHXListenSocketContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListenSocket))
    {
        AddRef();
        *ppvObj = (IHXListenSocket*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
IHXListenSocketContext::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
IHXListenSocketContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return HXR_OK;
}

STDMETHODIMP
IHXListenSocketContext::Init(UINT32 ulLocalAddr, UINT16 port,
                              IHXListenResponse* pListenResponse)
{
    return _Init(ulLocalAddr, port, pListenResponse);
}

HX_RESULT
IHXListenSocketContext::_Init(UINT32 ulLocalAddr, UINT32 port,
                   IHXListenResponse* pListenResponse)
{
    HX_RELEASE(m_pListenResponse);
    m_pListenResponse = pListenResponse;
    m_pListenResponse->AddRef();
    /*
     * If the addr is HX_INADDR_IPBINDINGS then create
     * an acceptor for each ip in the list.
     */

    IHXRegistry* preg = 0;
    IHXValues* pValues = 0;
    UINT32 ulNumAcceptors = 0;
    const char* pName;
    UINT32 ul;
    HX_RESULT res;
    Process* proc = (Process*)m_pEngine->get_proc();
    INT32 bBindToAll = FALSE;

    if (!proc)
    {
        /*
         * Everyone has a proc!
         */
        return HXR_FAIL;
    }

    if (ulLocalAddr == HX_INADDR_IPBINDINGS)
    {
    ((IUnknown*)proc->pc->server_context)->QueryInterface(
        IID_IHXRegistry, (void**)&preg);
    if (preg)
    {
        preg->GetIntByName("Server.BindToAll", bBindToAll);
        preg->GetPropListByName("Server.IPBinding", pValues);
        if (pValues)
        {
        res = pValues->GetFirstPropertyULONG32(pName, ul);
        while (res == HXR_OK)
        {
            ulNumAcceptors++;
            res = pValues->GetNextPropertyULONG32(pName, ul);
        }
        }
    }
    if (bBindToAll || !ulNumAcceptors)
    {
        ulLocalAddr = INADDR_ANY;
    }
    }

    if (bBindToAll || !ulNumAcceptors)
    {
    HX_RELEASE(preg);
    HX_RELEASE(pValues);
    m_ulNumAcceptors = 1;
    m_ppAcceptors = new INetworkAcceptor*;
    m_ppAcceptors[0] = new INetworkAcceptor(proc, m_pListenResponse);
    if (m_ppAcceptors[0]->init(ulLocalAddr, port, 64) < 0)  // XXXSMP Backlog
    {
        ERRMSG(m_pMessages, "could not open address(%s) and port(%d)\n",
                  HXInetNtoa(ulLocalAddr), port); //XXXSMP
        delete m_ppAcceptors[0];
        delete[] m_ppAcceptors;
        m_ppAcceptors = 0;
        m_ulNumAcceptors = 0;
        return HXR_FAIL;
    }

    m_ppAcceptors[0]->enable();
    }
    else
    {
    int gotone = 0;
    m_ppAcceptors = new INetworkAcceptor*[ulNumAcceptors];
    res = pValues->GetFirstPropertyULONG32(pName, ul);
    while (res == HXR_OK)
    {
        if (HXR_OK == preg->GetIntById(ul, (INT32 &)ulLocalAddr))
        {
        ulLocalAddr = ntohl(ulLocalAddr);
        m_ppAcceptors[m_ulNumAcceptors] = new
            INetworkAcceptor(proc, m_pListenResponse);
        if (m_ppAcceptors[m_ulNumAcceptors]->init(ulLocalAddr, port, 64) < 0)
        {
            in_addr addr;
            addr.s_addr = ulLocalAddr;
            ERRMSG(m_pMessages, "could not open address(%s) and"
            " port(%d)\n", HXInetNtoa(ulLocalAddr), port);
            delete m_ppAcceptors[m_ulNumAcceptors];
        }
        else
        {
            m_ppAcceptors[m_ulNumAcceptors]->enable();
            m_ulNumAcceptors++;
            gotone = 1;
        }
        }
        res = pValues->GetNextPropertyULONG32(pName, ul);
    }
    HX_RELEASE(pValues);
    HX_RELEASE(preg);
    if (!gotone)
    {
        delete[] m_ppAcceptors;
        m_ppAcceptors = 0;
        return HXR_FAIL;
    }

    }

    return HXR_OK;
}

/*
 * IHXUDPSocketContext methods
 */

BufArrayQueue::BufArrayQueue()
    : m_ulTotal(0)
    , m_pLast(NULL)
    , m_LastPosition(0)
    , m_ulLastRemove(0)
{
}

BufArrayQueue::~BufArrayQueue()
{
    ReadData** pDel;
    UINT32 i;

    while (!m_List.IsEmpty())
    {
    pDel = (ReadData**)m_List.RemoveHead();
    for (i = 0; (i < ARRAY_QUEUE_ARRAY_SIZE) && m_ulTotal; i++)
    {
        m_ulTotal--;
        delete pDel[i];
    }
    delete pDel;
    }
}

void
BufArrayQueue::Add(ReadData* pData)
{
    if (!m_pLast)
    {
    m_pLast = new ReadData*[ARRAY_QUEUE_ARRAY_SIZE];
    m_List.AddTail((void*)m_pLast);
    m_LastPosition = m_List.GetTailPosition();
    }
    m_pLast[m_ulTotal % ARRAY_QUEUE_ARRAY_SIZE] = pData;
    m_ulTotal++;
    if (m_ulTotal % ARRAY_QUEUE_ARRAY_SIZE == 0)
    {
    if (!m_LastPosition)
    {
        m_LastPosition = m_List.GetHeadPosition();
    }
    else
    {
        m_List.GetNext(m_LastPosition);
    }
    if (!m_LastPosition)
    {
        m_pLast = NULL;
        return;
    }
    m_pLast = (ReadData**)m_List.GetAt(m_LastPosition);
    }
}

ReadData*
BufArrayQueue::Remove()
{
    ReadData* pRet = NULL;
    if (!m_ulTotal || m_ulLastRemove == m_ulTotal)
    {
    goto reset;
    }
    if (!m_ulLastRemove)
    {
    m_LastPosition = m_List.GetHeadPosition();
    if (!m_LastPosition)
    {
        goto reset;
    }
    m_pLast = (ReadData**)m_List.GetAt(m_LastPosition);
    }
    pRet = m_pLast[m_ulLastRemove % ARRAY_QUEUE_ARRAY_SIZE];
    m_pLast[m_ulLastRemove % ARRAY_QUEUE_ARRAY_SIZE] = 0;
    m_ulLastRemove++;
    if (m_ulLastRemove % ARRAY_QUEUE_ARRAY_SIZE == 0)
    {
    m_List.GetNext(m_LastPosition);
    if (!m_LastPosition)
    {
        goto reset;
    }
    m_pLast = (ReadData**)m_List.GetAt(m_LastPosition);
    }

    return pRet;

reset:;
    m_LastPosition = m_List.GetHeadPosition();
    if (m_LastPosition)
    {
    m_pLast = (ReadData**)m_List.GetAt(m_LastPosition);
    }
    m_ulLastRemove = 0;
    m_ulTotal = 0;
    return pRet;
}

IHXUDPSocketContext::IHXUDPSocketContext(Engine* pEngine):
    m_lRefCount(0),
    m_pUDPResponse(0),
    m_pEngine(pEngine),
    m_bReadCallbackPending(FALSE),
    m_nRequired(0),
    m_nRead(0),
    m_pReadCallback(0),
    m_ppUDPIO(NULL),
    m_ulNumUDPIO(0),
    m_ulRoundRobinReads(0),
    m_ulDestAddr(0),
    m_nDestPort(0),
    m_UDPIOState(INIT),
    m_bReuseAddr(FALSE),
    m_bReusePort(FALSE),
    m_pReadStore(NULL),
    m_ulNextSample(0),
    m_ulCurrentReadIterations(0),
    m_bSampleOn(TRUE),
    m_bSendingReadDones(FALSE),
    m_ulLastMainloopIteration(0),
    m_ulSuccessiveMainloopCount(0),
    m_bSocketIsConnected(FALSE),
    m_bSocketShouldBeConnected(TRUE),
    m_pSharedUDPReader(0),
    m_bUseThreadSafeReadDone(FALSE),
    m_pReg(NULL),
    m_pVio (NULL),
    m_pWriteVectors (NULL),
    m_pVectorBuffers (NULL),
    m_unNumVectors (0)
{
}

IHXUDPSocketContext::IHXUDPSocketContext(Engine* pEngine, UDPIO* pUDPIO):
    m_lRefCount(0),
    m_pUDPResponse(0),
    m_pEngine(pEngine),
    m_bReadCallbackPending(FALSE),
    m_nRequired(0),
    m_nRead(0),
    m_pReadCallback(0),
    m_ulNumUDPIO(0),
    m_ulRoundRobinReads(0),
    m_ulDestAddr(0),
    m_nDestPort(0),
    m_UDPIOState(INIT),
    m_bReuseAddr(FALSE),
    m_bReusePort(FALSE),
    m_pReadStore(NULL),
    m_ulNextSample(0),
    m_ulCurrentReadIterations(0),
    m_bSampleOn(TRUE),
    m_bSendingReadDones(FALSE),
    m_ulLastMainloopIteration(0),
    m_ulSuccessiveMainloopCount(0),
    m_bSocketIsConnected(FALSE),
    m_bSocketShouldBeConnected(TRUE),
    m_pSharedUDPReader(0),
    m_bUseThreadSafeReadDone(FALSE),
    m_pReg(NULL),
    m_pWriteVectors (NULL),
    m_pVectorBuffers (NULL),
    m_unNumVectors (0)
{
    m_ppUDPIO = new UDPIO*;
    m_ppUDPIO[0] = pUDPIO;
    m_ulNumUDPIO = 1;
    m_pVio = new VIO (m_ppUDPIO [0]);
}

IHXUDPSocketContext::~IHXUDPSocketContext()
{
    HX_RELEASE(m_pReg);
    tini();
}

void
IHXUDPSocketContext::tini()
{
    UINT32 ul;

    m_UDPIOState = DEAD;

    if (m_pVio)
    {
    delete m_pVio;
    m_pVio = NULL;
    }

    if (m_pWriteVectors)
    {
    delete[] m_pWriteVectors;
    m_pWriteVectors = NULL;
    }

    if (m_pVectorBuffers)
    {
    for (ul = 0; ul < UDP_MAX_VECTORS; ul++)
    {
        HX_RELEASE(m_pVectorBuffers [ul]);
    }

    delete [] m_pVectorBuffers;
    m_pVectorBuffers = NULL;
    }


    if(m_ppUDPIO)
    {
    for (ul = 0; ul < m_ulNumUDPIO; ul++)
    {
        m_pEngine->callbacks.remove(HX_READERS, m_ppUDPIO[ul]);
        m_pEngine->UnRegisterSock();
    }
    }
    if (m_pReadCallback)
    {
    m_pReadCallback->Release();
    m_pReadCallback = NULL;
    }

    delete m_pReadStore;
    m_pReadStore = NULL;
    /*
     * I Assume that this is supposed to be deleted at the
     * end for a reason (ie. not in the upper if), so
     * so I will respect that when I do this loop.
     */
    if (m_ppUDPIO)
    {
    for (ul = 0; ul < m_ulNumUDPIO; ul++)
    {
        delete m_ppUDPIO[ul];
    }
    }
    delete[] m_ppUDPIO;
    m_ppUDPIO = NULL;

    // force unregister from shared port, if any

    ((IHXSharedUDPServices*)this)->UnregisterSharedResponse();

    /*
     * This is to prevent recursing in here with m_pUDPResponse
     * not set to null (ie. m_pUDPResponse destructor releases
     * us which calls our destructor which calls tini...
     */
    if (m_pUDPResponse)
    {
    IHXUDPResponse* pUDPResponse = m_pUDPResponse;
    m_pUDPResponse = NULL;
    pUDPResponse->Release();
    }
}

STDMETHODIMP
IHXUDPSocketContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXUDPSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXUDPSocket))
    {
        AddRef();
        *ppvObj = (IHXUDPSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXUDPMulticastInit))
    {
        AddRef();
        *ppvObj = (IHXUDPMulticastInit*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSetSocketOption))
    {
        AddRef();
        *ppvObj = (IHXSetSocketOption*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFastPathNetWrite))
    {
        AddRef();
        *ppvObj = (IHXFastPathNetWrite*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXUDPConnectedSocket))
    {
        AddRef();
        *ppvObj = (IHXUDPConnectedSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSharedUDPServices))
    {
        AddRef();
        *ppvObj = (IHXSharedUDPServices*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXBufferedSocket))
    {
    AddRef();
    *ppvObj = (IHXBufferedSocket*)this;
    return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
IHXUDPSocketContext::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
IHXUDPSocketContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
IHXUDPSocketContext::Init(ULONG32 ulAddr,
    UINT16 nPort, IHXUDPResponse* pUDPResponse)
{
    if (pUDPResponse != NULL)
    {
        HX_RELEASE(m_pUDPResponse);
        m_pUDPResponse = pUDPResponse;
        m_pUDPResponse->AddRef();

    IHXThreadSafeMethods* pTSMethods;
    if (HXR_OK == m_pUDPResponse->QueryInterface(IID_IHXThreadSafeMethods, (void**)&pTSMethods))
    {
        m_bUseThreadSafeReadDone = HX_THREADSAFE_METHOD_SOCKET_READDONE &
                               pTSMethods->IsThreadSafe();

        HX_RELEASE(pTSMethods);
    }
    else
    {
        m_bUseThreadSafeReadDone = FALSE;
    }

    }

    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_addr.s_addr = htonl(ulAddr);
    m_sockAddr.sin_port = htons(nPort);
    m_ulDestAddr = ulAddr;
    m_nDestPort = nPort;

    if (m_ulNumUDPIO)
    {
    init();
    m_UDPIOState = ALIVE;
    }

    HX_RELEASE(m_pReg);
    Process* proc = (Process*)m_pEngine->get_proc();
    if (((IUnknown*)proc->pc->server_context)->QueryInterface(
        IID_IHXRegistry, (void**)&m_pReg) != HXR_OK)
    {
        return HXR_FAIL;
    }

    INT32 bConnectUDP;
    m_bSocketShouldBeConnected = TRUE;
    // config variable present and set to false?
    if (m_pReg->GetIntByName("config.ConnectUDP", bConnectUDP) == HXR_OK && !bConnectUDP)
    {
        m_bSocketShouldBeConnected = FALSE;
    }

    return HXR_OK;
}

STDMETHODIMP
IHXUDPSocketContext::Bind(UINT32 ulLocalAddr, UINT16 nPort)
{
    DPRINTF(D_INFO, ("IHXUDPSC::Bind(localAddr(%lu), port(%d)\n",
        ulLocalAddr, nPort));

    INT32 nSndBufSize = 0;

    if (m_UDPIOState != INIT)
    {
    return HXR_UNEXPECTED;
    }

    if (m_ppUDPIO != 0)
    {
        return FinishBind(0);
    }

    if (ulLocalAddr == HX_INADDR_IPBINDINGS)
    {
        m_bSocketShouldBeConnected = FALSE;
        if (UseIPBinding(ulLocalAddr, nPort))
        {
            return FinishBind(1);
        }

        return FinishBind(-1);
    }

    /*
     * The system will choose a free local port
     * if nPort is 0
     */

    // XXXGo - use the var's - they default to FALSE
    m_ppUDPIO = new UDPIO*;
    m_ppUDPIO[0] = new UDPIO;
    m_ulNumUDPIO = 1;

    UINT32 ulAddr = (ulLocalAddr == HXR_INADDR_ANY) ? INADDR_ANY : ulLocalAddr;
    INT32 ret = m_ppUDPIO[0]->init(ulAddr, nPort, TRUE, m_bReuseAddr,
                                   m_bReusePort);

    if (FAILED(m_pReg->GetIntByName("config.UDPSendBufferSize", nSndBufSize)))
    {
    // 16K buffer size seems to fix the issue.
        nSndBufSize = 16834;

    }

    SetOption(HX_SOCKOPT_SET_SENDBUF_SIZE, nSndBufSize);

    m_pEngine->RegisterSock();

    return FinishBind(ret);
}

inline HX_RESULT
IHXUDPSocketContext::FinishBind(INT32 ret)
{
    if (ret == -1)
    {
    m_UDPIOState = INIT;
        UINT32 ul;
        for (ul = 0; ul < m_ulNumUDPIO; ul++)
        {
            delete m_ppUDPIO[ul];
            m_pEngine->UnRegisterSock();
        }
        delete[] m_ppUDPIO;
        m_ppUDPIO = 0;
        m_ulNumUDPIO = 0;

    // Releasing the UDPResponse can cause us to be deleted, so we have to
    // zero out m_pUDPResponse so tini() doesn't double-release it.

    if (m_pUDPResponse)
    {
        IHXUDPResponse* pUDPResponse = m_pUDPResponse;
        m_pUDPResponse = NULL;
        pUDPResponse->Release();
    }

        return HXR_FAIL;
    }

    init();
    m_UDPIOState = ALIVE;

    return HXR_OK;
}

int
IHXUDPSocketContext::UseIPBinding(UINT32 ulLocalAddr, UINT16 nPort)
{
    Process* proc = (Process*)m_pEngine->get_proc();
    if (!proc)
    {
        return 0;
    }

    /*
     * Look through ipbindings to see what to bind to.
     */
    UINT32 ul;
    const char* pName;
    IHXBuffer* pAddr;
    IHXRegistry* preg;
    ((IUnknown*)proc->pc->server_context)->QueryInterface(
        IID_IHXRegistry, (void**)&preg);
    IHXValues* pValues = 0;
    preg->GetPropListByName("config.IPBinding", pValues);
    if (!pValues)
    {
        preg->GetPropListByName("config.IPBindings", pValues);
    }
    if (!pValues)
    {
    // If IPBindings not there, use INADDR_ANY
    ulLocalAddr = INADDR_ANY;
    }
    else
    {
        /*
         * Get the first address to bind to .
         */
        if (HXR_OK == pValues->GetFirstPropertyULONG32(pName, ul))
        {
            if (HXR_OK == preg->GetStrByName(pName, pAddr))
            {
                /*
                 * First addr is the first one from ipbindings.
                 */
                ulLocalAddr = inet_addr((const char*)pAddr->GetBuffer());
                pAddr->Release();
                ulLocalAddr = ntohl(ulLocalAddr);
            }
            else
            {
                /*
                 * For some reason the string is not there.
                 * use HXR_INADDR_ANY.
                 */
                ulLocalAddr = INADDR_ANY;
                pValues->Release();
                pValues = 0;
            }
        }
        /*
         * If ipbindings is empty, use INADDR_ANY
         */
        else
        {
            ulLocalAddr = INADDR_ANY;
            pValues->Release();
            pValues = 0;
        }
    }

    /*
     * Init for the first addr.
     * If there is a bindings list, see how many elements
     * are in it so we can make the m_ppUDPIO array.
     */
    m_ulNumUDPIO = 1;
    UINT32 ulNumUDPIO = 1;
    if (pValues)
    {
        while (HXR_OK == pValues->GetNextPropertyULONG32(pName, ul))
        {
            ulNumUDPIO++;
        }
        /*
         * Ok, got the count, now need to set it back to one
         * past the first.
         */
        pValues->GetFirstPropertyULONG32(pName, ul);
    }

    m_ppUDPIO = new UDPIO*[ulNumUDPIO];

    m_ppUDPIO[0] = new UDPIO();
    m_ulNumUDPIO = 1;
    int gotone = 1;
    if (m_ppUDPIO[0]->init(ulLocalAddr, nPort, TRUE, m_bReuseAddr,
                           m_bReusePort) < 0)
    {
        gotone = 0;
    }
    m_pEngine->RegisterSock();

    if (pValues)
    {
        while (pValues->GetNextPropertyULONG32(pName, ul) == HXR_OK)
        {
            if (HXR_OK == preg->GetStrByName(pName, pAddr))
            {
                ulLocalAddr = inet_addr((const char*)pAddr->GetBuffer());
                ulLocalAddr = ntohl(ulLocalAddr);
                m_ppUDPIO[m_ulNumUDPIO] = new UDPIO;
                m_ulNumUDPIO++;
                if (m_ppUDPIO[m_ulNumUDPIO - 1]->init(ulLocalAddr,
                        nPort, TRUE, m_bReuseAddr, m_bReusePort) >= 0)
                {
                    m_pEngine->RegisterSock();
                    pAddr->Release();
                }
                else
                {
                    gotone = 0;
                    m_pEngine->RegisterSock();
                    pAddr->Release();
                    break;
                }
            }
        }
        pValues->Release();
    }
    preg->Release();

    return gotone;
}

STDMETHODIMP
IHXUDPSocketContext::GetLocalPort(UINT16& nPort)
{
    if(!m_ppUDPIO)
    {
    return HXR_FAIL;
    }

    nPort = m_ppUDPIO[0]->port();

    return HXR_OK;
}

STDMETHODIMP
IHXUDPSocketContext::Read(UINT16 nBytes)
{
    if (m_UDPIOState == DEAD)
    {
    return HXR_UNEXPECTED;
    }
    HX_RESULT ret = HXR_OK;
    if (!m_pReadStore)
    {
    m_pReadStore = new BufArrayQueue;
    }

    if (m_UDPIOState == INIT)
    {
        ret = Bind(HX_INADDR_IPBINDINGS, m_nDestPort);
        if (ret != HXR_OK)
            return ret;
    }

    m_nRequired = nBytes;

    /*
     * In server, once they call read they are in the engine fd_set
     * and continually get ReadDone callbacks.
     */
    if (!m_bReadCallbackPending)
    {
    UINT32 ul;
    for (ul = 0; ul < m_ulNumUDPIO; ul++)
    {
        m_pEngine->callbacks.enable(HX_READERS, m_ppUDPIO[ul], m_bUseThreadSafeReadDone);
    }
    m_bReadCallbackPending = TRUE;
    }
    AddRef();
    ret = DoRead();
    Release();

    return ret;
}

STDMETHODIMP
IHXUDPSocketContext::Write(IHXBuffer* pBuffer)
{
    if (m_UDPIOState == DEAD)
    {
    return HXR_UNEXPECTED;
    }
    if (m_UDPIOState == INIT)
    {
        HX_RESULT ret = Bind(HX_INADDR_IPBINDINGS, m_nDestPort);
        if (ret != HXR_OK)
        {
            return ret;
        }
    }

    if (m_bSocketIsConnected)
    {
        if (m_ppUDPIO[0]->write(pBuffer->GetBuffer(), pBuffer->GetSize()) > 0)
        {
            return HXR_OK;
        }
    }
    else
    {
        INT32 nAddrSize = sizeof(struct sockaddr);
        if(m_ppUDPIO[0]->sendto(pBuffer->GetBuffer(), pBuffer->GetSize(), 0,
                                &m_sockAddr, nAddrSize) > 0)
        {
            return HXR_OK;
        }
    }

    return HXR_FAIL;
}

extern UINT32* g_pNoBufs;
extern UINT32* g_pOtherUDPErrs;

STDMETHODIMP
IHXUDPSocketContext::FastWrite(const UINT8* pBuffer, UINT32 ulLen)
{
    if (!m_ppUDPIO || !m_ppUDPIO[0])
    {
    return HXR_FAIL;
    }

    if (m_bSocketShouldBeConnected)
    {
        if (!m_bSocketIsConnected)
        {
            if (ConnectSockets() == HXR_FAIL)
            {
                // never try to connect again
                m_bSocketShouldBeConnected = FALSE;
                goto doUnconnectedWrite;
            }
        }

        if (m_ppUDPIO[0]->write(pBuffer, ulLen) > 0)
        {
            return HXR_OK;
        }

        goto failure;
    }

doUnconnectedWrite:
    if (m_ppUDPIO[0]->sendto(pBuffer, ulLen, 0, &m_sockAddr,
                             sizeof(struct sockaddr)) > 0)
    {
        return HXR_OK;
    }

failure:
#ifdef _UNIX
    if (m_ppUDPIO[0]->error() == ENOBUFS)
    {
        *g_pNoBufs += 1;
    }
    else
    {
        *g_pOtherUDPErrs += 1;
    }
#endif

    return HXR_FAIL;
}


STDMETHODIMP
IHXUDPSocketContext::WriteTo(ULONG32 ulAddr,
    UINT16 nPort, IHXBuffer* pBuffer)
{
    if (m_UDPIOState == DEAD)
    {
    return HXR_UNEXPECTED;
    }
    /*
     * Disconnect the socket if it's currently connected and we're trying to
     * write to a different address, otherwise just call write.  We assume that
     * m_UDPIOInitialized is true if m_bSocketIsConnected is true
     */
    if (m_bSocketIsConnected)
    {
    if (m_sockAddr.sin_addr.s_addr == htonl(ulAddr) &&
        m_sockAddr.sin_port == htons(nPort))
    {
        if (m_ppUDPIO[0]->write(pBuffer->GetBuffer(),
                    pBuffer->GetSize()) > 0)
        {
        return HXR_OK;
        }
        goto cleanup;
    }

    // XXXtbradley should log a warning here if this returns an error.
    DisconnectSockets();
        m_bSocketShouldBeConnected = FALSE;
    }

    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_addr.s_addr = htonl(ulAddr);
    m_sockAddr.sin_port = htons(nPort);

    if (m_UDPIOState == INIT)
    {
    HX_RESULT ret = Bind(HX_INADDR_IPBINDINGS, nPort);
    if (HXR_OK != ret)
        return ret;
    }

    if (m_ppUDPIO[0]->sendto(pBuffer->GetBuffer(),
    pBuffer->GetSize(), 0, &m_sockAddr, sizeof(m_sockAddr)) > 0)
    {
    return HXR_OK;
    }

cleanup:
#ifdef _UNIX
    if (m_ppUDPIO[0]->error() == ENOBUFS)
    {
        *g_pNoBufs += 1;
    }
    else
    {
        *g_pOtherUDPErrs += 1;
    }
#endif

    return HXR_FAIL;
}

STDMETHODIMP
IHXUDPSocketContext::BufferedWrite(IHXBuffer* pBuffer)
{
    if (!pBuffer || !(pBuffer->GetBuffer()))
    {
    return HXR_INVALID_PARAMETER;
    }

    if (!m_ppUDPIO || !m_ppUDPIO[0] || !m_pVio || !m_pWriteVectors)
    {
    return HXR_NOT_INITIALIZED;
    }

    if (!m_bSocketShouldBeConnected)
    {
    return HXR_INVALID_OPERATION;
    }

    if (!m_bSocketIsConnected)
    {
    if (ConnectSockets() == HXR_FAIL)
    {
        m_bSocketShouldBeConnected = FALSE;
        return HXR_INVALID_OPERATION;
    }
    }

    if (m_unNumVectors + 1 > UDP_MAX_VECTORS)
    {
    return HXR_BUFFERTOOSMALL;
    }

    pBuffer->AddRef();
    m_pVectorBuffers[m_unNumVectors] = pBuffer;
    m_pWriteVectors[m_unNumVectors].iov_base = (char*)pBuffer->GetBuffer();
    m_pWriteVectors[m_unNumVectors].iov_len = pBuffer->GetSize();

    m_unNumVectors++;

    return HXR_OK;
}

STDMETHODIMP
IHXUDPSocketContext::FlushWrite()
{
    if (!m_ppUDPIO || !m_ppUDPIO[0] || !m_pVio
    || !m_pWriteVectors || !m_unNumVectors)
    {
    HX_ASSERT(0);
    return HXR_NOT_INITIALIZED;
    }

    HX_RESULT hRes = HXR_OK;

    if (m_pVio->writevec(m_pWriteVectors, m_unNumVectors) < 0)
    {
#ifdef _UNIX
    if (m_pVio->error() == ENOBUFS)
    {
        *g_pNoBufs += 1;
    }
    else
    {
        *g_pOtherUDPErrs += 1;
    }

#endif
    hRes = HXR_FAIL;
    }

    for (UINT16 i = 0; i < m_unNumVectors; i++)
    {
    HX_RELEASE(m_pVectorBuffers [i]);
    }

    m_unNumVectors = 0;

    return hRes;
}

STDMETHODIMP
IHXUDPSocketContext::SetDesiredPacketSize(UINT32 ulPacketSize)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
IHXUDPSocketContext::JoinMulticastGroup(
    ULONG32 ulMulticastAddr, ULONG32 ulInterfaceAddr)
{
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = htonl(ulMulticastAddr);
    mreq.imr_interface.s_addr = htonl(ulInterfaceAddr);
    UINT32 ul;

    HX_RESULT ret = HXR_FAIL;
    for (ul = 0; ul < m_ulNumUDPIO; ul++)
    {
    if (m_ppUDPIO[ul]->join_group(mreq) >= 0)
    {
        ret = HXR_OK;
    }
    }

    return ret;

}

STDMETHODIMP
IHXUDPSocketContext::LeaveMulticastGroup(
    ULONG32 ulMulticastAddr, ULONG32 ulInterfaceAddr)
{
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = htonl(ulMulticastAddr);
    mreq.imr_interface.s_addr = htonl(ulInterfaceAddr);

    UINT32 ul;
    HX_RESULT ret = HXR_FAIL;
    for (ul = 0; ul < m_ulNumUDPIO; ul++)
    {
    if (m_ppUDPIO[ul]->leave_group(mreq) >= 0)
    {
        ret = HXR_OK;
    }
    }

    return ret;
}

STDMETHODIMP
IHXUDPSocketContext::UDPSocketReadCallback::Func()
{
    if (m_pContext)
    {
    /*
     * Must protect the m_pContext object from being deleted while
     * it's doing business
     */
    AddRef();
    m_pContext->AddRef();
    m_pContext->DoRead();
    m_pContext->Release();
    Release();
    }
    return HXR_OK;
}

HX_RESULT
IHXUDPSocketContext::DoRead()
{
    if (!m_pUDPResponse)
    {
    tini();
    return HXR_OK;
    }
    if (!m_pReadStore)
    {
       m_pReadStore = new BufArrayQueue;
    }
    if (!m_pUDPResponse)
    {
    return HXR_OK;
    }
    ReadData* pNewReadData = NULL;
    /*
     * Don't let us recurse into here
     */
    if (m_bSendingReadDones)
    {
    return HXR_OK;
    }
    /*
     * Why we are iterative:
     *
     *  We can't be recursive because users
     *    should not call us back with a read
     *    unless they know they want us to
     *    call recvfrom.
     */
    if (m_ulLastMainloopIteration + 1 == m_pEngine->m_ulMainloopIterations)
    {
    m_ulSuccessiveMainloopCount++;
    }
    else if (m_ulLastMainloopIteration != m_pEngine->m_ulMainloopIterations)
    {
    m_ulSuccessiveMainloopCount = 0;
    }
    if (m_ulSuccessiveMainloopCount == SUCCESSIVE_MAINLOOP_ITERATIONS_TRIGGER)
    {
    m_bSampleOn = TRUE;
    m_ulSuccessiveMainloopCount = 0;
    }
    m_ulLastMainloopIteration = m_pEngine->m_ulMainloopIterations;

    UINT32 ulTimes = 0;
    UINT32 ulMaxTimes;

    if (m_bSampleOn)
    {
    ulMaxTimes = MAX_PACKETS_PER_READ;
    }
    else
    {
    ulMaxTimes = m_ulCurrentReadIterations;
    }
    while (ulMaxTimes--)
    {
    /*
     * This does the job of adding our callback if no
     * data is there.
     */
    pNewReadData = GetReadData();
    if (pNewReadData)
    {
        ulTimes++;
        m_pReadStore->Add(pNewReadData);
    }
    else
    {
        break;
    }
    }
    if (m_bSampleOn)
    {
    if (ulTimes != 0)
    {
        m_pSamples[m_ulNextSample % SAMPLE_SET_SIZE] = ulTimes;
        m_ulNextSample++;
        if (m_ulNextSample % SAMPLE_SET_SIZE == 0)
        {
        ulTimes = 0;
        for (ulMaxTimes = 0; ulMaxTimes < SAMPLE_SET_SIZE;
            ulMaxTimes++)
        {
            ulTimes += m_pSamples[ulMaxTimes];
        }
        m_ulCurrentReadIterations = ulTimes / SAMPLE_SET_SIZE;
        if (m_ulCurrentReadIterations < 1)
        {
            m_ulCurrentReadIterations = 1;
        }
        m_bSampleOn = FALSE;
        }
    }
    }
    else if (++m_ulNextSample % RESAMPLE_FREQ == 0)
    {
    m_bSampleOn = TRUE;
    }

    m_bSendingReadDones = TRUE;
    while (m_pUDPResponse && (pNewReadData = m_pReadStore->Remove()))
    {
    if (HXR_OK !=
        m_pUDPResponse->ReadDone(HXR_OK,
        pNewReadData->m_pBuffer,
        pNewReadData->m_ulAddr,
        pNewReadData->m_nPort))
    {
        m_pUDPResponse->ReadDone(HXR_FAIL, 0, 0, 0);
        HX_RELEASE(m_pUDPResponse);
    }

    delete pNewReadData;
    }
    /*
     * We MUST clear this structure!
     */
    if (m_pReadStore->m_ulTotal != 0)
    {
    while (pNewReadData = m_pReadStore->Remove())
    {
        delete pNewReadData;
    }
    }
    m_bSendingReadDones = FALSE;
    return HXR_OK;
}

ReadData*
IHXUDPSocketContext::GetReadData()
{
    UINT32 ul;
    INT32 nAddrSize = sizeof(struct sockaddr);
    sockaddr_in sockAddr = m_sockAddr;
    UINT32 ulStart = m_ulRoundRobinReads;
    ReadData* pNewReadData = NULL;
    Process* proc = (Process*)m_pEngine->get_proc();
    IHXBuffer* pBuffer = new ServerBuffer(TRUE);
    pBuffer->SetSize(m_nRequired);

    BYTE* pData = pBuffer->GetBuffer();
    while (1)
    {
    /*
     * Read from each of our sockets in round robin fashion.
     */
        if (m_bSocketIsConnected)
        {
            m_nRead = m_ppUDPIO[m_ulRoundRobinReads]->read(pData, m_nRequired);
        }
        else
        {
            m_nRead = m_ppUDPIO[m_ulRoundRobinReads]->recvfrom(
                pData, m_nRequired, 0, &sockAddr, &nAddrSize);
        }
    ul = m_ulRoundRobinReads;
    m_ulRoundRobinReads++;
    if (m_ulRoundRobinReads >= m_ulNumUDPIO)
    {
        m_ulRoundRobinReads = 0;
    }
    if (m_nRead > 0)
    {
        break;
    }
    else
    {
        if(m_ppUDPIO[ul]->error() == EWOULDBLOCK)
        {
        m_bReadCallbackPending = TRUE;
        m_pEngine->callbacks.enable(HX_READERS, m_ppUDPIO[ul], m_bUseThreadSafeReadDone);
        }
    }

    if (ulStart == m_ulRoundRobinReads)
    {
        break;
    }
    }

    if (m_nRead > 0)
    {
    pBuffer->SetSize(m_nRead);
    pNewReadData = new (proc->pc->mem_cache) ReadData(pBuffer, ntohl(sockAddr.sin_addr.s_addr),
        ntohs(sockAddr.sin_port));
    }

    pBuffer->Release();
    return pNewReadData;
}

void
IHXUDPSocketContext::init()
{
    if (m_pReadCallback != NULL)
    {
        // callbacks can only be added once.
        return;
    }

    m_pReadCallback = new UDPSocketReadCallback;
    m_pReadCallback->AddRef();
    m_pReadCallback->m_pContext = this;

    if (!m_pVio && m_ppUDPIO [0])
    {
    m_pVio = new VIO (m_ppUDPIO [0]);
    }

    if (!m_pWriteVectors)
    {
    m_pWriteVectors = (HX_IOVEC*) new HX_IOVEC [UDP_MAX_VECTORS];
    }

    if (!m_pVectorBuffers)
    {
    m_pVectorBuffers = (IHXBuffer**) new IHXBuffer* [UDP_MAX_VECTORS];
    memset(m_pVectorBuffers, 0, sizeof(IHXBuffer*) * UDP_MAX_VECTORS);
    }

    // setup select callback
    UINT32 ul;
    for (ul = 0; ul < m_ulNumUDPIO; ul++)
    {
    m_ppUDPIO[ul]->nonblocking();

    m_pEngine->callbacks.add(HX_READERS, m_ppUDPIO[ul],
        m_pReadCallback, m_bUseThreadSafeReadDone);
    m_pEngine->callbacks.disable(HX_READERS, m_ppUDPIO[ul]);
    }
}

HX_RESULT
IHXUDPSocketContext::ConnectSockets()
{
    if (m_sockAddr.sin_addr.s_addr == INADDR_ANY)
    {
        /*
         * if there's more than 1 network interface and we're allowed to connect
         * to any of them, connect my bind the socket to the wrong NIC, so
         * we won't connect in this case
         */
        return HXR_FAIL;
    }

    for (UINT32 ul = 0; ul < m_ulNumUDPIO; ul++)
    {
        if (m_ppUDPIO[ul]->connect(&m_sockAddr) == -1)
        {
            return HXR_FAIL;
        }
    }

    m_bSocketIsConnected = TRUE;

    return HXR_OK;
}

HX_RESULT
IHXUDPSocketContext::DisconnectSockets()
{
    for (UINT32 ul = 0; ul < m_ulNumUDPIO; ul++)
    {
        if (m_ppUDPIO[ul]->disconnect() == -1)
        {
            return HXR_FAIL;
        }
    }

    m_bSocketIsConnected = FALSE;

    return HXR_OK;
}

    /*
     * IHXUDPConnectedSocket methods
     */

/* IHXUDPConnectedSocket::UDPConnect
 *
 * Connect to the addr:port specified in the last Init(), WriteTo(), or
 * Connect(addr, port) call.
 */
STDMETHODIMP
IHXUDPSocketContext::UDPConnect()
{
    return ConnectSockets();
}

STDMETHODIMP
IHXUDPSocketContext::UDPConnect(ULONG32 ulAddr, UINT16 nPort)
{
    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_addr.s_addr = htonl(ulAddr);
    m_sockAddr.sin_port = htons(nPort);

    return ConnectSockets();
}

STDMETHODIMP
IHXUDPSocketContext::UDPDisconnect()
{
    return DisconnectSockets();
}

STDMETHODIMP_(BOOL)
IHXUDPSocketContext::IsUDPConnected()
{
    return m_bSocketIsConnected;
}

STDMETHODIMP_(BOOL)
IHXUDPSocketContext::IsUDPConnected(REF(ULONG32) ulAddr,
                                    REF(UINT16) nPort)
{
    ulAddr = ntohl(m_sockAddr.sin_addr.s_addr);
    nPort =  ntohs(m_sockAddr.sin_port);

    return m_bSocketIsConnected;
}

STDMETHODIMP
IHXUDPSocketContext::InitMulticast(UINT8 uTTL)
{
    // Better return codes - convert from socket err to HX_RESULT
    HX_RESULT       theErr = HXR_FAIL;

    UINT32 ul;

    for (ul = 0; ul < m_ulNumUDPIO; ul++)
    {
    if (m_ppUDPIO[ul]->set_multicast() >= 0)
    {
        theErr = HXR_OK;
    }

    if (m_ppUDPIO[ul]->set_multicast_ttl(uTTL) >= 0)
    {
        theErr = HXR_OK;
    }
    }

    return theErr;
}

STDMETHODIMP
IHXUDPSocketContext::SetOption(HX_SOCKET_OPTION option, UINT32 ulValue)
{
    HX_RESULT res = HXR_OK;
    UINT32 ul;
    switch(option)
    {
    case HX_SOCKOPT_REUSE_ADDR:
    m_bReuseAddr = (BOOL)ulValue;
    if (m_ppUDPIO)
    {
        for (ul = 0; ul < m_ulNumUDPIO; ul++)
        {
        if (m_ppUDPIO[ul]->reuse_addr(m_bReuseAddr) < 0)
            {
            res = HXR_FAIL;
        }
        }
    }
    break;
    case HX_SOCKOPT_REUSE_PORT:
    m_bReusePort = (BOOL)ulValue;
    if (m_ppUDPIO)
    {
        for (ul = 0; ul < m_ulNumUDPIO; ul++)
        {
        if (m_ppUDPIO[ul]->reuse_port(m_bReusePort) < 0)
        {
            res = HXR_FAIL;
        }
        }
    }
    break;
    case HX_SOCKOPT_SET_RECVBUF_SIZE:
    if (m_ppUDPIO)
    {
        for (ul = 0; ul < m_ulNumUDPIO; ul++)
        {
        if (m_ppUDPIO[ul]->set_recv_size(ulValue) < 0)
        {
            res = HXR_FAIL;
        }
        }
    }
    break;
    case HX_SOCKOPT_SET_SENDBUF_SIZE:
    if (m_ppUDPIO)
    {
        for (ul = 0; ul < m_ulNumUDPIO; ul++)
        {
        if (m_ppUDPIO[ul]->set_send_size(ulValue) < 0)
        {
            res = HXR_FAIL;
        }
        }
    }
    break;
    case  HX_SOCKOPT_IP_TOS:
    if (m_ppUDPIO)
    {
        for (ul = 0; ul < m_ulNumUDPIO; ul++)
        {
        if (m_ppUDPIO[ul]->set_ip_tos(ulValue) < 0)
        {
            res = HXR_FAIL;
        }
        }
    }
    break;
    default:
    HX_ASSERT(!"I don't know this option");
    res = HXR_FAIL;
    }

    return res;
}

/*
 * IHXUDPSocketContext::RegisterSharedResponse
 *
 * Tell the shared port where to send replies sent from the addr at the other
 * end of this socket.
 *
 * pUDPResponse:    UDP response object to get the datagrams
 * sPortEnum:       0 = shared UDP port
 *          1 = shared RTSP port
 *
 * jmevissen, 1/25/2001
 */
STDMETHODIMP
IHXUDPSocketContext::RegisterSharedResponse(IHXUDPResponse* pUDPResponse,
                         UINT16 sPortEnum)
{
    HX_RESULT hr = HXR_FAIL;

    // This can only be done once.

    if (m_pSharedUDPReader) return HXR_FAIL;

    // Does our engine have a shared port reader?

    if (m_pEngine)
    {
    m_pSharedUDPReader = m_pEngine->GetSharedUDPReader();
    if (m_pSharedUDPReader)
    {
        m_pSharedUDPReader->AddRef();
        hr = m_pSharedUDPReader->
        RegisterForeignAddress(m_ulDestAddr,
                       m_nDestPort,
                       pUDPResponse,
                       sPortEnum);
        if (hr == HXR_OK)
        {
        m_sPortEnum = sPortEnum;
        }
        else
        {
        m_pSharedUDPReader->Release();
        m_pSharedUDPReader = 0;
        }
    }
    }
    return hr;
}

/*
 * IHXUDPSocketContext::UnregisterSharedResponse
 *
 * Unregister for replies from the foreign host.
 * Usually called when this object is deleted (the socket is closed).
 *
 *
 * jmevissen, 1/25/2001
 */
STDMETHODIMP
IHXUDPSocketContext::UnregisterSharedResponse()
{
    if (m_pSharedUDPReader)
    {
    m_pSharedUDPReader->UnregisterForeignAddress(m_ulDestAddr,
                             m_nDestPort,
                             m_sPortEnum);
    m_pSharedUDPReader->Release();
    m_pSharedUDPReader = 0;
    }
    return HXR_OK;
}

/*
 * IHXUDPSocketContext::GetSharedPort
 *
 * Return the shared port number the server is bound to (listening on).
 *
 * jmevissen, 1/25/2001
 */
STDMETHODIMP_(UINT16)
IHXUDPSocketContext::GetSharedPort()
{
    if (m_pSharedUDPReader)
    {
    return m_pSharedUDPReader->GetPort(m_sPortEnum);
    }
    else
    {
    return 0;
    }
}

IHXLBoundTCPSocketContext::IHXLBoundTCPSocketContext(Engine* pEngine, LBOUND_TYPE etype /*= LBOUNDT_CLIENT*/)
:  INetworkTCPSocketContext(pEngine, NULL),
    m_eLBoundType(etype),
    m_nLBoundReadSize(-1),
    m_bLBoundRequestClose(FALSE),
    m_bLBoundReadEnabled(FALSE),
    m_pLBRemoteSocket(NULL),
    m_bLBoundWriteEnabled(FALSE),
    m_ulRecursionLevel(0)
{
    m_bSupportsBufferedSocket = FALSE;
    m_lLBoundAddress=DwToHost(inet_addr("127.0.0.1"));
    DPRINTF(LBOUND_DEBUG, ("%p LBSC::IHXLBoundTCPSocketContext\n", this));
}

IHXLBoundTCPSocketContext::~IHXLBoundTCPSocketContext()
{
    DPRINTF(LBOUND_DEBUG, ("%p %s LBSC::~IHXLBoundTCPSocketContext\n", this, LBTYPE(m_eLBoundType)));

    LBoundClose();
}

STDMETHODIMP
IHXLBoundTCPSocketContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXPLocalBoundSocket))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXTCPSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXWouldBlockResponse))
    {
        AddRef();
        *ppvObj = (IHXWouldBlockResponse*)this;
        return HXR_OK;
    }

    return INetworkTCPSocketContext::QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG32)
IHXLBoundTCPSocketContext::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
IHXLBoundTCPSocketContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::Read(UINT16  Size)
{
    HX_RESULT ret = HXR_OK;

#ifdef _DEBUG
    Process* proc = (Process*)m_pEngine->get_proc();
    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::Read--size:%d, procnum=%d\n",
          this, LBTYPE(m_eLBoundType), Size, m_eLBoundType,proc->procnum() ) );
#endif
    m_nLBoundReadSize = Size;
    m_bLBoundReadEnabled = TRUE;

    if(m_ulRecursionLevel > MAX_LBOUND_READ_DEPTH)
    {
        LBoundTCPSocketServerReadCallback* pCB = new LBoundTCPSocketServerReadCallback;
        pCB->AddRef();
        this->AddRef();
        pCB->m_pContext = this;
        m_pEngine->schedule.enter(0.0, pCB);
    }
    else
    {
        LBoundCheckRead();
    }

    return ret;
}


STDMETHODIMP
IHXLBoundTCPSocketContext::Write(IHXBuffer* pBuffer)
{
    HX_RESULT hr = HXR_OK;
    UINT16 nPort=0;

    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::Write, size: %d\n", this,
          LBTYPE(m_eLBoundType), pBuffer->GetSize() ));

    if ( m_pLBRemoteSocket == NULL )
        return HXR_UNEXPECTED;

    if (m_bLBoundWriteEnabled)
    {
        pBuffer->AddRef();   //addref for lbound4
        m_pLBRemoteSocket->m_LBoundToRead.AddTail(pBuffer);

        if(m_eLBoundType == LBOUNDT_SERVER)
        {
            m_pLBRemoteSocket->LBoundCheckRead();
        }
        else if(m_eLBoundType == LBOUNDT_CLIENT)
        {
            LBoundTCPSocketServerReadCallback* pCB = new LBoundTCPSocketServerReadCallback;
            pCB->AddRef();
            m_pLBRemoteSocket->AddRef();
            pCB->m_pContext = m_pLBRemoteSocket;
            m_pEngine->schedule.enter(0.0, pCB);
        }

    }
    else
    {
        // must have gotten closed
        DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::Write - discarding packet\n",
           this, LBTYPE(m_eLBoundType)));

        return HXR_UNEXPECTED;
    }

    return HXR_OK;
}


STDMETHODIMP
IHXLBoundTCPSocketContext::WantWrite()
{
    if (m_pTCPResponse)
        m_pTCPResponse->WriteReady(HXR_OK);

    return HXR_OK;
}

void
IHXLBoundTCPSocketContext::enableRead()
{
    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::enableRead entered\n",
                this, LBTYPE(m_eLBoundType) ));

    HX_ASSERT(0);
}

void
IHXLBoundTCPSocketContext::disableRead()
{
    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::disableRead entered\n",
                this, LBTYPE(m_eLBoundType) ));

    HX_ASSERT(0);
}


STDMETHODIMP
IHXLBoundTCPSocketContext::Bind(UINT32 ulLocalAddr, UINT16 nPort)
{
    return HXR_OK;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::Connect(const char*  pDestination,UINT16 nPort)
{
    HX_RESULT hr = HXR_OK;

    if (m_pLBRemoteSocket)
    { //already connected
        return HXR_FAIL;
    }

    m_eLBoundType = LBOUNDT_CLIENT;

    Process* proc = (Process*)m_pEngine->get_proc();

    IHXLBoundTCPSocketContext* pConn = new IHXLBoundTCPSocketContext( (Engine*) proc->pc->engine, LBOUNDT_SERVER);

    pConn->m_bLBoundWriteEnabled = TRUE;
    m_bLBoundWriteEnabled = TRUE;

    m_pLBRemoteSocket = pConn;
    pConn->m_pLBRemoteSocket = this;

    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::Connect (proc %p) - to server socket %p\n",
           this, LBTYPE(m_eLBoundType), proc, pConn));

    //XXXTDM: this is busted
    HX_ASSERT(FALSE);
    proc->pc->lbound_tcp_listenRTSPResponse->OnConnection(NULL, NULL);

    m_pTCPResponse->ConnectDone(HXR_OK);

    return hr;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::GetForeignAddress(UINT32& lAddress)
{
    lAddress=m_lLBoundAddress;
    DPRINTF(LBOUND_DEBUG, ("%p LBSC::GetForeignAddress: %x\n", this, lAddress));

    return HXR_OK;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::GetLocalAddress(UINT32& lAddress)
{
    lAddress=m_lLBoundAddress;

    return HXR_OK;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::GetForeignPort(UINT16& port)
{
    port = 0;
    return HXR_OK;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::GetLocalPort(UINT16& port)
{
    port = 0;
    return HXR_OK;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::WouldBlock(UINT32 id)
{
    if(m_pLBRemoteSocket->m_pWouldBlockResponse)
    {
        return m_pLBRemoteSocket->m_pWouldBlockResponse->WouldBlock(
            m_pLBRemoteSocket->m_ulWouldBlockResponseID);
    }
    return HXR_OK;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::WouldBlockCleared(UINT32 id)
{
    if(m_pLBRemoteSocket->m_pWouldBlockResponse)
    {
        return m_pLBRemoteSocket->m_pWouldBlockResponse->WouldBlockCleared(
            m_pLBRemoteSocket->m_ulWouldBlockResponseID);
    }
    return HXR_OK;
}


STDMETHODIMP
IHXLBoundTCPSocketContext::SetOption(HX_SOCKET_OPTION option, UINT32 ulValue)
{
    return HXR_OK;
}

STDMETHODIMP
IHXLBoundTCPSocketContext::LBoundTCPSocketServerReadCallback::Func(void)
{
    if (m_pContext)
    {
        m_pContext->LBoundCheckRead();
        m_pContext->Release();
        Release();               //corresponding AddRef()'s are in ::Write()
    }

    return HXR_OK;
}

void
IHXLBoundTCPSocketContext::LBoundCheckRead()
{
    if ( m_nLBoundReadSize > 0 )
    {  //there's a local bound pending read, so kick off read
         LBoundProcessReadQueue();
    }
}


HX_RESULT
IHXLBoundTCPSocketContext::LBoundProcessReadQueue()
{
    if ( !m_bLBoundReadEnabled )
         return HXR_UNEXPECTED;

    LISTPOSITION pos = m_LBoundToRead.GetHeadPosition();
    if (pos)
    {
        // just send it, we send already parsed messages, don't worry about
        // the read size, server has to ask for more by design

        IHXBuffer* pBuf= (IHXBuffer*) m_LBoundToRead.RemoveHead();

        DPRINTF(LBOUND_DEBUG, ("-%p %s-LBSC::LBoundProcessReadQueue.  buf size: %d\n",
                  this, LBTYPE(m_eLBoundType), pBuf->GetSize() ));

#ifdef _DEBUG
        char buf[20];
        memset(buf, 20, 0);
        strncpy(buf, (const char*) pBuf->GetBuffer(), 19 );
        DPRINTF(LBOUND_DEBUG, ("-%p %s-LBSC::LBoundProcessReadQueue, ReadDone size %d, (%s...)\n",
                    this, LBTYPE(m_eLBoundType), pBuf->GetSize(), buf));
#endif

        m_nLBoundReadSize = -1;

        m_ulRecursionLevel++;
        m_pTCPResponse->ReadDone( HXR_OK, pBuf );
        m_ulRecursionLevel--;
        pBuf->Release();
    }
    else
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}


HX_RESULT
IHXLBoundTCPSocketContext::LBoundClose()
{
    HX_RESULT hr = HXR_OK;

    if ( m_eLBoundType == LBOUNDT_NONE )
        return HXR_UNEXPECTED;

    if (m_pLBRemoteSocket)
        m_pLBRemoteSocket->LBoundRequestClose();

    m_bLBoundWriteEnabled = FALSE;
    m_bLBoundReadEnabled = FALSE;

    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::LBoundClose-Set LBoundState=CLOSED; empty ToRead and ToWrite tables\n",
        this, LBTYPE(m_eLBoundType)));

    CHXSimpleList::Iterator i;

    for (i = m_LBoundToRead.Begin(); i != m_LBoundToRead.End(); ++i)
        ((IUnknown*)(*i))->Release();
    m_LBoundToRead.RemoveAll();

    m_pLBRemoteSocket = 0;
    return hr;
}


HX_RESULT
IHXLBoundTCPSocketContext::LBoundRequestClose()
{
    HX_RESULT hr = HXR_OK;

    // remote socket is closing me and will be deleted
    m_pLBRemoteSocket = 0;

    if ( ! m_bLBoundRequestClose )
    {
        if (m_bLBoundReadEnabled)
            m_pTCPResponse->ReadDone(HXR_FAILED, 0);
        else
            m_bLBoundRequestClose=TRUE;
    }
    return hr;
}


void
IHXLBoundTCPSocketContext::disconnect()
{
    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::disconnect\n", this, LBTYPE(m_eLBoundType) ));

    HX_ASSERT(0);
}


void
IHXLBoundTCPSocketContext::reconnect(Engine* pEngine)
{
    DPRINTF(LBOUND_DEBUG, ("%p %s-LBSC::reconnect from procnum: %d to procnum: %d\n",
                this,  LBTYPE(m_eLBoundType), ((Process*)m_pEngine->get_proc())->procnum(), ((Process*)pEngine->get_proc())->procnum() ));

    HX_ASSERT(0);
}

