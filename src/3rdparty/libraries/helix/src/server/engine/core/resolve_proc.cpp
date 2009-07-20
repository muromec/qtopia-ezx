/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resolve_proc.cpp,v 1.3 2006/05/22 19:22:20 dcollins Exp $
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

#if defined unix || defined _UNIX
#include <netdb.h>
#include "unix_misc.h"
#endif

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "server_engine.h"
#include "server_context.h"
#include "servsockimp.h"
#include "server_inetwork.h"
#include "dispatchq.h"
#include "core_proc.h"
#include "proc.h"
#include "proc_container.h"
#include "resolve_proc.h"

#include "_main.h" // MakeProcess(), pResolverInitMutex
#include "server_version.h"

#include "servresolvimp.h"

#ifdef _WIN32
#define getpid() (int)GetCurrentThreadId()
#endif

CResolveProcPool::CResolveProcPool(void)
{
    m_Mutex = HXMutexCreate();
    HXMutexInit(m_Mutex);
    m_nProcesses = 0;
    memset(m_pool, 0, MAX_RESOLVER_PROCESSES*sizeof(Process*));
}

CResolveProcPool::~CResolveProcPool(void)
{
    // Assume the shutdown interface will destroy all procs?
    // What about the waiters?
    HXMutexDestroy(m_Mutex);
}

void
CResolveProcPool::GetProc(CServNativeResolve* pResolve, Process* pCallingProc)
{
    HXMutexLock(m_Mutex);

    // Search for an idle process in the pool
    UINT32 n;
    for (n = 0; n < MAX_RESOLVER_PROCESSES; n++)
    {
        Process* pProc = m_pool[n];
        if (pProc != NULL)
        {
            m_pool[n] = NULL;
            HXMutexUnlock(m_Mutex);
            pResolve->ProcReady(pProc);
            return;
        }
    }

    pResolve->AddRef();
    m_waiters.AddTail(pResolve);

    // See if we can create a new process
    if (m_nProcesses < MAX_RESOLVER_PROCESSES)
    {
        ++m_nProcesses;
        HXMutexUnlock(m_Mutex);
        CreateResolveCallback* cb = new CreateResolveCallback;
        cb->m_pCallingProc = pCallingProc;
        cb->m_pCaller = this;
        pCallingProc->pc->dispatchq->send(pCallingProc, cb, PROC_RM_CONTROLLER);
        return;
    }

    // Must wait for a proc to become free
    HXMutexUnlock(m_Mutex);
}

void
CResolveProcPool::PutProc(Process* pProc)
{
    HX_ASSERT(pProc != NULL && pProc->pc->process_type == PTResolver);

    HXMutexLock(m_Mutex);

    // If there are any waiters, give the proc to the oldest one
    if (m_waiters.GetCount() > 0)
    {
        CServNativeResolve* pResolve = (CServNativeResolve*)m_waiters.RemoveHead();
        HXMutexUnlock(m_Mutex);
        pResolve->ProcReady(pProc);
        pResolve->Release();
        return;
    }

    // Put the proc back into the pool
    UINT32 n;
    for (n = 0; n < MAX_RESOLVER_PROCESSES; n++)
    {
        if (m_pool[n] == NULL)
        {
            m_pool[n] = pProc;
            HXMutexUnlock(m_Mutex);
            return;
        }
    }

    // Accounting error
    HXMutexUnlock(m_Mutex);
    HX_ASSERT(FALSE);
}

void
CreateResolveCallback::func(Process* pProc)
{
    char szMsg[80];
    sprintf(szMsg, "%s Resolver starting up\n", ServerVersion::ProductName());
    DPRINTF(D_INFO, (szMsg));
    ResolveProcessInitCallback* cb = new ResolveProcessInitCallback;
    cb->m_pProc = pProc; // Main's Process
    cb->m_pCallingProc = m_pCallingProc;
    cb->m_pCaller = m_pCaller;
    *pResolverProcInitMutex = 1;

    MakeProcess("resolver", cb, pProc->pc->dispatchq, 0);
    while (*pResolverProcInitMutex)
        microsleep(1);

    delete this;
}

void
ResolveProcessInitCallback::func(Process* pProc)
{
    pProc->pc = new ProcessContainer(pProc, m_pProc->pc);
    pProc->pc->dispatchq->init(pProc);
    pProc->pc->process_type = PTResolver;

    /*
     * Now that the proc->pc is established, it is safe to initialize the
     * IHXNetworkServicesContext
     */

    pProc->pc->network_services->Init(pProc->pc->server_context,
                                      pProc->pc->engine, NULL);

    pProc->pc->net_services->Init(pProc->pc->server_context);

    ResolveProcessReadyCallback* cb = new ResolveProcessReadyCallback;
    cb->m_pCallingProc = m_pCallingProc;
    cb->m_pCaller = m_pCaller;
    cb->m_pResolveProc = pProc;
    pProc->pc->dispatchq->send(pProc, cb, m_pCallingProc->procnum());

    *pResolverProcInitMutex = 0;

    pProc->pc->engine->mainloop();
}

void
ResolveProcessReadyCallback::func(Process* pProc)
{
    m_pCaller->PutProc(m_pResolveProc);

    delete this;
}

void
ResolveDispatchAddrRequest::func(Process* pProc)
{
    struct addrinfo ai;
    struct addrinfo* aires;
    memset(&ai, 0, sizeof(ai));
    if (m_pHints != NULL)
    {
        ai.ai_flags = m_pHints->GetFlags();
        ai.ai_family = m_pHints->GetFamily();
        ai.ai_socktype = m_pHints->GetType();
        ai.ai_protocol = m_pHints->GetProtocol();
    }

    // Avoid duplicate answers per socktype (eg. TCP, UDP, SCTP)
    ai.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(m_pNode, m_pService, &ai, &aires);

    ResolveDispatchAddrResponse* cb = new ResolveDispatchAddrResponse;
    cb->m_pCallingProc = m_pCallingProc;
    cb->m_pCaller = m_pCaller;
    cb->m_hxr = HXR_FAIL;
    cb->m_nVecLen = 0;
    cb->m_ppAddrVec = NULL;
    if (rc == 0)
    {
        struct addrinfo* aicur;

        cb->m_hxr = HXR_OK;
        cb->m_nVecLen = 0;

        // Count elements in result list and create result vector
        aicur = aires;
        while (aicur != NULL)
        {
            switch (aicur->ai_family)
            {
            case AF_INET:
            case AF_INET6:
                cb->m_nVecLen++;
                break;
            default:
                // Ignore unknown address types
                break;
            }
            aicur = aicur->ai_next;
        }
        cb->m_ppAddrVec = new IHXSockAddr*[cb->m_nVecLen];

        // Fill in result vector
        aicur = aires;
        IHXSockAddr** ppCur = cb->m_ppAddrVec;
        while (aicur != NULL)
        {
            switch (aicur->ai_family)
            {
            case AF_INET:
                *ppCur = new CHXSockAddrIN4(pProc->pc->server_context, (sockaddr_in*)aicur->ai_addr);
                (*ppCur)->AddRef();
                break;
            case AF_INET6:
                *ppCur = new CHXSockAddrIN6(pProc->pc->server_context, (sockaddr_in6*)aicur->ai_addr);
                (*ppCur)->AddRef();
                break;
            default:
                // Ignore unknown address types
                break;
            }
            aicur = aicur->ai_next;
            ppCur++;
        }
    }
    freeaddrinfo(aires);

    pProc->pc->dispatchq->send(pProc, cb, m_pCallingProc->procnum());

    HX_RELEASE(m_pHints);
    delete this;
}

void
ResolveDispatchAddrResponse::func(Process* pProc)
{
    m_pCaller->OnResponse(this);
    delete this;
}

void
ResolveDispatchNameRequest::func(Process* pProc)
{
    int rc;
    ResolveDispatchNameResponse* cb = new ResolveDispatchNameResponse;

    struct sockaddr* psa;
    size_t salen;

    IHXSockAddrNative* pNative = NULL;
    m_pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    pNative->Get(&psa, &salen);
    HX_RELEASE(pNative);

    rc = getnameinfo(psa, salen,
                     cb->m_szNode, sizeof(cb->m_szNode),
                     cb->m_szService, sizeof(cb->m_szService),
                     0 /* XXX: flags? */);
    if (rc == 0)
    {
        cb->m_hxr = HXR_OK;
    }
    else
    {
        cb->m_hxr = HXR_FAIL;
    }

    pProc->pc->dispatchq->send(pProc, cb, m_pCallingProc->procnum());
}

void
ResolveDispatchNameResponse::func(Process* pProc)
{
    m_pCaller->OnResponse(this);
    delete this;
}
