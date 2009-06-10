/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servresolvimp.cpp,v 1.2 2007/08/30 17:29:50 seansmith Exp $
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
#include "hxnet.h"
#include "iresolv.h"
#include "resolvcache.h"
#include "servresolvimp.h"

#include "core_proc.h"
#include "resolve_proc.h"
#include "dispatchq.h"

#include "servbuffer.h"
#include "hxsbuffer.h"

#ifdef _SOLARIS
#include "core_container.h"
#include "conn_dispatch.h"
#endif

CServNativeResolve::CServNativeResolve(Process* pProc, CResolverCache* pCache) :
    m_nRefCount(0),
    m_pProc(pProc),
    m_pCCF(NULL),
    m_pNetSvc(NULL),
    m_pCache(NULL),
    m_pResponse(NULL),
    m_pRequest(NULL),
    m_pResolveProc(NULL)
{
    m_pCCF = pProc->pc->common_class_factory;
    m_pCCF->AddRef();
    m_pNetSvc = (IHXNetServices*)pProc->pc->net_services;
    m_pNetSvc->AddRef();
    m_pCache = pCache;
    m_pCache->AddRef();
}

CServNativeResolve::~CServNativeResolve(void)
{
    Close();
}

void
CServNativeResolve::ProcReady(Process* pResolveProc)
{
    m_pResolveProc = pResolveProc;
    if (m_pRequest->reqtype == ResolveRequest::addrinfo)
    {
        ResolveDispatchAddrRequest* pDisp = new ResolveDispatchAddrRequest;
        pDisp->m_pCallingProc = m_pProc;
        pDisp->m_pCaller = this;
        pDisp->m_pNode = m_pRequest->ai.pNode;
        pDisp->m_pService = m_pRequest->ai.pServ;
        pDisp->m_pHints = m_pRequest->ai.pHints;
        HX_ADDREF(pDisp->m_pHints);
        m_pProc->pc->dispatchq->send(m_pProc, pDisp, pResolveProc->procnum());
    }
    else
    {
        HX_ASSERT(FALSE);
    }
}

void
CServNativeResolve::OnResponse(ResolveDispatchAddrResponse* pResponse)
{
    UINT32 n;
    for (n = 0; n < pResponse->m_nVecLen; n++)
    {
        IHXBuffer* pAddrBuf = NULL;
        pResponse->m_ppAddrVec[n]->GetAddr(&pAddrBuf);
        pAddrBuf->Release();
    }

    if (SUCCEEDED(pResponse->m_hxr))
    {
        // We need to synthesize the expire vector since getaddrinfo does not
        // provide it.  Set the expiry to one hour to balance resolver load
        // with timely updates.
        time_t tExpire = time(NULL) + 60*60;
        time_t* ptExpireVec = new time_t[pResponse->m_nVecLen];
        UINT32 n;
        for (n = 0; n < pResponse->m_nVecLen; n++)
        {
            ptExpireVec[n] = tExpire;
        }
        m_pCache->AddAddrInfo(m_pRequest->ai.pNode, pResponse->m_nVecLen,
                              ptExpireVec, pResponse->m_ppAddrVec);
        delete[] ptExpireVec;
    }
    HX_DELETE(m_pRequest);

    m_pResponse->GetAddrInfoDone(pResponse->m_hxr,
                                 pResponse->m_nVecLen,
                                 pResponse->m_ppAddrVec);

    HX_ASSERT(m_pResolveProc != NULL);
    m_pProc->pc->resolver_pool->PutProc(m_pResolveProc);
    m_pResolveProc = NULL;
}

void
CServNativeResolve::OnResponse(ResolveDispatchNameResponse* pResponse)
{
    HX_ASSERT(FALSE); // Implement me
}

STDMETHODIMP
CServNativeResolve::QueryInterface(REFIID riid, void** ppvObj)
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
CServNativeResolve::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CServNativeResolve::Release(void)
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
CServNativeResolve::Init(IHXResolveResponse* pResponse)
{
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);
    return HXR_OK;
}

STDMETHODIMP
CServNativeResolve::Close(void)
{
    HX_DELETE(m_pRequest);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pCache);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pCCF);
    return HXR_OK;
}


STDMETHODIMP
CServNativeResolve::GetAddrInfo(const char* pNode, const char* pService,
        IHXAddrInfo* pHints)
{
    if (m_pRequest != NULL)
    {
        return HXR_UNEXPECTED;
    }

    /*
     * Note the bulk of this function is dedicated to bypassing the actual
     * resolver query for numeric hostnames.  This code was lifted from
     * CIntResolver.  It is probably worth consolidating the identical code,
     * eg. into a utility function in netdrv.
     */

    IHXSockAddr* pAddr = NULL;
    UINT32 n = 0;

    // XXX allow symbolic port names
    UINT16 port = 0;
    if (pService != NULL)
    {
        port = (UINT16)atoi(pService);
    }

    // Determine the desired socket family
    HXSockFamily f = HX_SOCK_FAMILY_INANY;
    if (pHints != NULL)
    {
        switch (pHints->GetFamily())
        {
        case AF_INET:
            f = HX_SOCK_FAMILY_IN4;
            break;
        case AF_INET6:
            f = HX_SOCK_FAMILY_IN6;
            break;
        default:
            f = HX_SOCK_FAMILY_INANY;
        }
    }

    // Check for an unspecified nodename
    if (pNode == NULL)
    {
        m_pNetSvc->CreateSockAddr(f, &pAddr);
        if (pHints != NULL && !(pHints->GetFlags() & HX_ADDRFLAGS_PASSIVE))
        {
            IHXBuffer* pNodeBuf = NULL;
            m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pNodeBuf);
            switch (pHints->GetFamily())
            {
            case AF_INET:
                pNode = "127.0.0.1";
                break;
            case AF_INET6:
                pNode = "::1";
                break;
            default:
                HX_ASSERT(FALSE);
            }
            pNodeBuf->Set((UCHAR*)pNode, strlen(pNode)+1);
            pAddr->SetAddr(pNodeBuf);
            HX_RELEASE(pNodeBuf);
        }
        m_pResponse->GetAddrInfoDone(HXR_OK, 1, &pAddr);
        HX_RELEASE(pAddr);
        return HXR_OK;
    }

    // Check for a numeric nodename.  Try IPv4 first, then IPv6
    if (f == HX_SOCK_FAMILY_INANY || f == HX_SOCK_FAMILY_IN4)
    {
        IHXBuffer* pNodeBuf = NULL;
        m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pNodeBuf);
        pNodeBuf->Set((UCHAR*)pNode, strlen(pNode)+1);

        m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
        if (pAddr->SetAddr(pNodeBuf) == HXR_OK)
        {
            pAddr->SetPort(port);
            m_pResponse->GetAddrInfoDone(HXR_OK, 1, &pAddr);
            HX_RELEASE(pAddr);
            HX_RELEASE(pNodeBuf);
            return HXR_OK;
        }
        HX_RELEASE(pAddr);
        HX_RELEASE(pNodeBuf);
    }
    if (f == HX_SOCK_FAMILY_INANY || f == HX_SOCK_FAMILY_IN6)
    {
        IHXBuffer* pNodeBuf = NULL;
        m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pNodeBuf);
        pNodeBuf->Set((UCHAR*)pNode, strlen(pNode)+1);

        m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN6, &pAddr);
        if (pAddr->SetAddr(pNodeBuf) == HXR_OK)
        {
            pAddr->SetPort(port);
            m_pResponse->GetAddrInfoDone(HXR_OK, 1, &pAddr);
            HX_RELEASE(pAddr);
            HX_RELEASE(pNodeBuf);
            return HXR_OK;
        }
        HX_RELEASE(pAddr);
        HX_RELEASE(pNodeBuf);
    }

    // Address is not numeric, verify AI_NUMERICHOST is not set
    if (pHints != NULL && (pHints->GetFlags() & HX_ADDRFLAGS_NUMERICHOST))
    {
        m_pResponse->GetAddrInfoDone(HXR_FAIL, 0, NULL);
        return HXR_OK;
    }

    /*
     * At this point we have a non-numeric hostname that needs resolved.  If
     * we have a cache, search for the given name.
     */
    UINT32 uAddrVecLen = 0;
    IHXSockAddr** ppAddrVec = NULL;
    if (m_pCache != NULL)
    {
        m_pCache->FindAddrInfo(pNode, uAddrVecLen, ppAddrVec);
    }
    if (uAddrVecLen > 0)
    {
        IHXSockAddr** ppAnswerVec = new IHXSockAddr*[uAddrVecLen];
        for (n = 0; n < uAddrVecLen; n++)
        {
            ppAddrVec[n]->Clone(&ppAnswerVec[n]);
            ppAnswerVec[n]->SetPort(port);
        }
        m_pResponse->GetAddrInfoDone(HXR_OK, uAddrVecLen, ppAnswerVec);
        for (n = 0; n < uAddrVecLen; n++)
        {
            HX_RELEASE(ppAnswerVec[n]);
        }
        delete[] ppAnswerVec;
        return HXR_OK;
    }

    // Looks like we have to send a query
    m_pRequest = new ResolveRequest(pNode, pService, pHints);

    m_pProc->pc->resolver_pool->GetProc(this, m_pProc);

    return HXR_OK;
}

STDMETHODIMP
CServNativeResolve::GetNameInfo(IHXSockAddr* pAddr, UINT32 uFlags)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}
