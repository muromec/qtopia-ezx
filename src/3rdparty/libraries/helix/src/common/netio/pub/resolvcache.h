/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resolvcache.h,v 1.2 2005/03/14 19:36:36 bobclark Exp $
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

#ifndef _RESOLVCACHE_H
#define _RESOLVCACHE_H

#ifndef MAX_HOST_LEN
#define MAX_HOST_LEN 256
#endif

#include "hxmap.h"

// Cached info for forward mapping
class CAddrInfo
{
public:
    CAddrInfo(void);
    ~CAddrInfo(void);
    void    Add(UINT32 uVecLen, time_t* ptExpireVec, IHXSockAddr** ppAddrVec);
    void    Del(HXSockFamily f);
    void    DelAll(void);
    UINT32  Expire(time_t now);
    HXBOOL    IsEmpty(void) { return (m_uVecLen == 0); }

    void    Get(UINT32& uVecLen, IHXSockAddr**& ppVec)
            { uVecLen = m_uVecLen; ppVec = m_ppAddrVec; }

protected:
    UINT32          m_uVecAlloc;
    UINT32          m_uVecLen;
    time_t*         m_ptExpireVec;
    IHXSockAddr**   m_ppAddrVec;
};

// Cached info for inverse mapping and canonical names
class CNodeInfo
{
public:
    CNodeInfo(void)
    {
        m_szName[0] = '\0';
        m_tExpire = 0;
    }

    char            m_szName[MAX_HOST_LEN];
    time_t          m_tExpire;
};

class CResolverCache : public IHXCallback
{
private: // Unimplemented
    CResolverCache(void);
    CResolverCache(const CResolverCache&);
    CResolverCache& operator=(const CResolverCache&);

public:
    CResolverCache(IUnknown* punkContext);
    virtual ~CResolverCache(void);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXCallback
    STDMETHOD(Func) (THIS);

    HXBOOL Init(void);

    void AddAddrInfo(const char* pNode, UINT32 uVecLen,
                            time_t* ptExpireVec, IHXSockAddr** ppAddrVec);
    void ReplaceAddrInfo(const char* pNode, UINT32 uVecLen,
                            time_t* ptExpireVec, IHXSockAddr** ppAddrVec);
    HXBOOL FindAddrInfo(const char* pNode, UINT32& uVecLen, IHXSockAddr**& ppVec);

protected:
    HXBOOL ReadConfig(void);

protected:
    INT32                   m_nRefCount;
    IUnknown*               m_punkContext;
    IHXScheduler*           m_pScheduler;

    time_t                  m_tLastPrune;

    CHXMapStringToOb        m_mapAddrInfo;      // Node -> Addr cache
    // Support service names?

    // Reverse lookup members go here
};

#endif /* _RESOLVCACHE_H */
