/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: server_inetwork.cpp,v 1.4 2004/07/24 23:10:19 tmarshall Exp $
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
#include "sockio.h"
#include "netbyte.h"
#include "cbqueue.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "chxpckts.h"
#include "debug.h"
#include "base_errmsg.h"
#include "load_balanced_listener.h"
#include "server_inetwork.h"
#include "inetwork_acceptor.h"
#include "resolver_dispatch.h"
#include "server_engine.h"
#include "dispatchq.h"

#include "proc.h"
#include "client.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_RESERVE_DESCRIPTORS             4
#define DEFAULT_RESERVE_SOCKETS                 4

#ifdef _UNIX
Resolver2CoreCallback::Resolver2CoreCallback(const char* pHostname,
                                                Process* pOrigProc,
                                                CServerResolverContext* pCon)
: m_state(NONE)
, m_pHostname(NULL)
, m_pCoreProc(NULL)
, m_pOrigProc(pOrigProc)
, m_ulAddr(0)
, m_status(HXR_FAIL)
{
    if (pHostname)
    {
        m_pHostname = new char[strlen(pHostname) + 1];
        strcpy(m_pHostname, pHostname);
    }
    else
    {
        m_pHostname = new char[1];
        m_pHostname[0] = 0;
    }
    m_pContext = pCon;
    m_pContext->AddRef();
}

Resolver2CoreCallback::~Resolver2CoreCallback()
{
    delete[] m_pHostname;
    m_pHostname = NULL;
    HX_RELEASE(m_pContext);
}

void
Resolver2CoreCallback::func(Process* proc)
{
    if (m_state == NONE)
    {
        m_state = IN_CORE;
    }
    else if (m_state == IN_CORE)
    {
        m_state = BACK;
    }

    if (m_state == IN_CORE)
    {
        m_pCoreProc = proc;
        proc->pc->m_pResMUX->GetHostByName(m_pHostname, this);
    }
    else if (m_state == BACK)
    {
        m_pContext->m_bResolverCallbackPending = FALSE;
        CServerResolverContext* pResp = m_pContext;
        m_pContext = NULL;
        pResp->m_pResp->GetHostByNameDone(m_status, m_ulAddr);
        pResp->Release();
        /*
         * Don't access any members after this!!!
         */
        delete this;
    }
}

HX_RESULT
Resolver2CoreCallback::GetHostByNameDone(HX_RESULT status, ULONG32 ulAddr)
{
    if(ulAddr != (ULONG32)-1)
    {
        m_status = status;
        m_ulAddr = ulAddr;
    }
    else
    {
        m_status = HXR_FAIL;
        m_ulAddr = 0;
    }

    m_pCoreProc->pc->dispatchq->send(m_pCoreProc, this,
            m_pOrigProc->procnum());

    return HXR_OK;
}

#endif

CServerResolverContext::CServerResolverContext(Process* proc):
    m_proc(proc),
    m_bResolverCallbackPending(FALSE)
{
    m_pResolverCallback = new ResolverContextCallback();
    m_pResolverCallback->AddRef();
}

CServerResolverContext::~CServerResolverContext()
{
    if (m_pResolverCallback)
    {
        m_pResolverCallback->Release();
    }
}

STDMETHODIMP
CServerResolverContext::GetHostByName(const char* pHostName)
{
    if(m_bResolverCallbackPending)
    {
        return HXR_UNEXPECTED;
    }

    m_bResolverCallbackPending = TRUE;

#ifdef _UNIX
    Resolver2CoreCallback* cb = new Resolver2CoreCallback(pHostName, m_proc,
                                                            this);
    m_proc->pc->dispatchq->send(m_proc, cb, PROC_RM_CORE);
#else
    /*
     * Can't use the return value from rdispatch->Send because -1 is an OK
     * value
     */

    m_pResolverCallback->ArmCallback(this);
    m_proc->pc->rdispatch->Send(m_proc, (char*)pHostName, m_pResolverCallback);
#endif

    return HXR_OK;
}

CServerResolverContext::ResolverContextCallback::ResolverContextCallback()
: m_pContext(0)
{
    m_lRefCount = 0;
}

void
CServerResolverContext::ResolverContextCallback::
    ArmCallback(CServerResolverContext* pContext)
{
    if(m_pContext)
    {
        m_pContext->Release();
    }
    m_pContext = pContext;
    if(m_pContext)
    {
        m_pContext->AddRef();
    }
}

CServerResolverContext::ResolverContextCallback::~ResolverContextCallback()
{
    if (m_pContext)
    {
        m_pContext->Release();
    }
}

STDMETHODIMP_(ULONG32)
CServerResolverContext::ResolverContextCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CServerResolverContext::ResolverContextCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

void
CServerResolverContext::ResolverContextCallback::func(Process* pProc)
{
    /*
     * To prevent this object from being destroyed when the
     * CServerResolverContext releases it, we have it AddRef()
     * and Release() itself
     */

    AddRef();

    if(host_ip != (ULONG32)-1)
    {
        m_pContext->m_pResp->GetHostByNameDone(HXR_OK, host_ip);
    }
    else
    {
        m_pContext->m_pResp->GetHostByNameDone(HXR_FAILED, 0);
    }
    m_pContext->m_bResolverCallbackPending = FALSE;
    m_pContext->Release();
    m_pContext = 0;

    Release();
}

CServerListenSocketContext::CServerListenSocketContext(Process* pProc,
                                Engine* pEngine,
                                IHXErrorMessages* pMessages,
                                ServerAccessControl* pAccessCtrl)
    : IHXListenSocketContext(pEngine, pMessages, pAccessCtrl)
{
    m_pProc = pProc;
    m_bLoadBalancedListener = FALSE;

    m_ulReserveDescriptors = DEFAULT_RESERVE_DESCRIPTORS;
    m_ulReserveSockets = DEFAULT_RESERVE_SOCKETS;
}

CServerListenSocketContext::~CServerListenSocketContext()
{
    if (!m_bLoadBalancedListener)
    {
        IUnknown* pUnk = 0;
        m_pListenResponse->QueryInterface(IID_IUnknown,
            (void**)&pUnk);
        HX_RELEASE(m_pListenResponse);
        pUnk->Release();
    }
    else
    {
        m_pProc->pc->load_listen_mgr->RemoveListener(m_pProc,
            m_LoadBalancedListenerID,
            m_pLoadBalancedListenerHandle);
    }
}


STDMETHODIMP
CServerListenSocketContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXListenSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListenSocket))
    {
        AddRef();
        *ppvObj = (IHXListenSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXLoadBalancedListen))
    {
        AddRef();
        *ppvObj = (IHXLoadBalancedListen*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CServerListenSocketContext::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CServerListenSocketContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CServerListenSocketContext::SetID(REFIID ID)
{
    m_bLoadBalancedListener = TRUE;
    SetGUID(m_LoadBalancedListenerID, ID);

    return HXR_OK;
}

STDMETHODIMP
CServerListenSocketContext::SetReserveLimit(UINT32 ulDescriptors,
                                            UINT32 ulSockets)
{
    m_ulReserveDescriptors = ulDescriptors;
    m_ulReserveSockets = ulSockets;

    return HXR_OK;
}

STDMETHODIMP
CServerListenSocketContext::Init(UINT32 ulLocalAddr, UINT16 port,
                              IHXListenResponse* pListenResponse)
{
    if (!m_bLoadBalancedListener)
    {
        return _Init(ulLocalAddr, port,
            pListenResponse);
    }
    else
    {
        m_pLoadBalancedListenerHandle =
            m_pProc->pc->load_listen_mgr->AddListener(m_pProc,
                                                   m_LoadBalancedListenerID,
                                                   ulLocalAddr,
                                                   port,
                                                   pListenResponse,
                                                   m_ulReserveDescriptors,
                                                   m_ulReserveSockets);

        return HXR_OK;
    }
}
