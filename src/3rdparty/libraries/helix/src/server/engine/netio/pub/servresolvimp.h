/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servresolvimp.h,v 1.1 2004/11/15 22:39:58 tmarshall Exp $
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

#ifndef _SERVRESOLVIMP_H_
#define _SERVRESOLVIMP_H_

#include "hxcom.h"
#include "hxslist.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxmon.h"
#include "base_callback.h"
#include "simple_callback.h"
#include "mem_cache.h"

#include "hxnet.h"
#include "hxservnet.h"
#include "netdrv.h"
#include "sockimp.h"

#include "access_ctrl.h"

/*
 * This is the implementation of the native resolver.  It creates resolver
 * processes to perform resolver lookups because the getaddrinfo/gethostinfo
 * functions are synchronous (blocking).
 *
 * Each resolver process is given a single request.  New resolvers are created
 * if all current resolvers are servicing requests.  If the maximum number of
 * resolver processes has been reached, the request is queued until a resolver
 * process becomes free to handle it.
 */

#define MAX_RESOLVER_PROCESSES 16

class ResolveRequest
{
private:
    ResolveRequest(void);
    ResolveRequest(const ResolveRequest&);
    ResolveRequest& operator=(const ResolveRequest&);

public:
    ResolveRequest(const char* pNode,
                   const char* pServ,
                   IHXAddrInfo* pHints)
    {
        reqtype = addrinfo;
        ai.pNode = (pNode ? new_string(pNode) : NULL);
        ai.pServ = (pServ ? new_string(pServ) : NULL);
        ai.pHints = pHints;
        HX_ADDREF(ai.pHints);
    }
    ResolveRequest(IHXSockAddr* pAddr, UINT32 uFlags)
    {
        reqtype = nameinfo;
        ni.pAddr = pAddr;
        HX_ADDREF(ni.pAddr);
        ni.uFlags = uFlags;
    }
    ~ResolveRequest(void)
    {
        switch (reqtype)
        {
        case addrinfo:
            HX_VECTOR_DELETE(ai.pNode);
            HX_VECTOR_DELETE(ai.pServ);
            HX_RELEASE(ai.pHints);
            break;
        case nameinfo:
            HX_RELEASE(ni.pAddr);
            break;
        default:
            HX_ASSERT(FALSE);
        }
    }

    enum { none, addrinfo, nameinfo } reqtype;
    union
    {
        struct
        {
            const char* pNode;
            const char* pServ;
            IHXAddrInfo* pHints;
        } ai;
        struct
        {
            IHXSockAddr* pAddr;
            UINT32 uFlags;
        } ni;
    };
};

struct ResolveRequestInfo
{
    ResolveRequest      request;
    Process*            pProc;
};

class ResolveDispatchAddrResponse;
class ResolveDispatchNameResponse;

class CServNativeResolve : public IHXResolve
{
public:
    CServNativeResolve(Process* pProc, CResolverCache* pCache);
    virtual ~CServNativeResolve(void);

    void ProcReady(Process* pResolverProc);

    void OnResponse(ResolveDispatchAddrResponse* pResponse);
    void OnResponse(ResolveDispatchNameResponse* pResponse);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXResolve
    STDMETHOD(Init)                 (THIS_ IHXResolveResponse* pResponse);
    STDMETHOD(Close)                (THIS);
    STDMETHOD(GetAddrInfo)          (THIS_ const char* pNode,
                                           const char* pService,
                                           IHXAddrInfo* pHints);
    STDMETHOD(GetNameInfo)          (THIS_ IHXSockAddr* pAddr,
                                           UINT32 uFlags);

protected:
    INT32                       m_nRefCount;
    Process*                    m_pProc;

    IHXCommonClassFactory*      m_pCCF;
    IHXNetServices*             m_pNetSvc;

    CResolverCache*             m_pCache;

    IHXResolveResponse*         m_pResponse;
    ResolveRequest*             m_pRequest;
    Process*                    m_pResolveProc;
};

#endif /* _SERVRESOLVIMP_H_ */
