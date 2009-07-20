/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: iresolv.h,v 1.7 2005/11/08 22:44:51 ckarusala Exp $
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

#ifndef _IRESOLV_H
#define _IRESOLV_H

#ifndef MAX_HOST_LEN
#define MAX_HOST_LEN 256
#endif

#include "hxmap.h"

struct dns_hdr
{
    UINT16 id;
    UINT16 flags; // QR:1, OPCODE:4, AA:1, TC:1, RD:1, RA:1, Z:3, RCODE:4
    UINT16 qdcnt;
    UINT16 ancnt;
    UINT16 nscnt;
    UINT16 arcnt;
};
struct dns_qr_hdr
{
    char    szHost[MAX_HOST_LEN];
    UINT16  qtype;
    UINT16  qclass;
};
struct dns_rr_hdr
{
    UINT16  rtype;
    UINT16  rclass;
    UINT32  ttl;
    UINT16  rdlen;
};

// This should be <= 40 bytes (assuming sockaddr_in6 is <= 32).
struct query_res
{
    UINT32  ttl;
    UINT16  rtype;
    union
    {
        sockaddr_in     a4;
        sockaddr_in6    a6;
    };
};

class CResolverCache;
class CIntResolver;

class CNodeQuery : public IHXCallback
{
private: // Unimplemented
    CNodeQuery(void);
    CNodeQuery(const CNodeQuery& other);
    CNodeQuery& operator=(const CNodeQuery& other);

public:
    CNodeQuery(HXSockFamily f, const char* pNode);
    virtual ~CNodeQuery(void);

    void Init(CIntResolver* pOwner, IUnknown* punkContext, IHXSocket* pSock);

    void SendRequest(HXSockFamily f);
    void HandleResponse(IHXBuffer* pBuf, IHXSockAddr* pSource);
    void RemoveCallback();

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXCallback
    STDMETHOD(Func) (THIS);

protected:
    HXBOOL EncodeName(const char* szName, BYTE*& rpbuf, size_t& rlen);

    HXBOOL ParseQuestionHeader(dns_qr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos);
    HXBOOL ParseAnswerHeader(dns_rr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos);
    HXBOOL DecodeName(char* pname, const BYTE* pbuf, size_t buflen, size_t& rpos);

protected:
    INT32                   m_nRefCount;
    CIntResolver*           m_pOwner;
    IHXCommonClassFactory*  m_pCCF;
    IHXScheduler*           m_pScheduler;
    IHXSocket*              m_pSock;
    UINT16                  m_qidIN4;
    UINT16                  m_qidIN6;

    UINT                    m_nDelta;
    UINT                    m_nRetries;
    HXSockFamily            m_family;
    char                    m_szNode[MAX_HOST_LEN];
    int                     m_nCurDomain;
    int                     m_nCurServer;
    CallbackHandle          m_CallbackHandle;
};

class CIntResolver : public IHXResolve,
                     public IHXSocketResponse
{
private: // Unimplemented
    CIntResolver(void);
    CIntResolver(const CIntResolver&);
    CIntResolver& operator=(const CIntResolver&);

public:
    CIntResolver(IUnknown* punkContext, CResolverCache* pCache = NULL);
    virtual ~CIntResolver(void);

    UINT16 NextQueryID(void);

    void AddQuery(UINT16 qid, CNodeQuery* pQuery);
    void DelQuery(UINT16 qid, CNodeQuery* pQuery);

    void QueryResult(const char* pNode, UINT32 rescnt, query_res* resvec);
    void QueryDone(HX_RESULT status, const char* pNode);

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

    // IHXSocketResponse
    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

protected:
    HXBOOL ReadConfig(void);

    void HandleResponse(IHXBuffer* pBuf, IHXSockAddr* pSource);

protected:
    INT32                   m_nRefCount;
    IUnknown*               m_punkContext;
    IHXCommonClassFactory*  m_pCCF;
    IHXNetServices*         m_pNetSvc;

    UINT16                  m_usQueryID;
    UINT16                  m_port;
    IHXSocket*              m_pSock;
    IHXResolveResponse*     m_pResponse;

    CResolverCache*         m_pCache;

    CHXMapLongToObj         m_mapHostQueries;   // Pending host queries
    // Reverse lookup members go here
};

#endif /* _IRESOLV_H */
