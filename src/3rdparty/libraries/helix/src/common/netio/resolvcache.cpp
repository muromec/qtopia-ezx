/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resolvcache.cpp,v 1.8 2007/09/21 08:54:13 anshuman Exp $
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
#include "hlxclib/time.h"
#include "hlxclib/ctype.h"
#include "hxccf.h"
#include "hxengin.h"
#include "hxbuffer.h"
#include "ihxpckts.h"
#include "nettypes.h"
#include "netdrv.h"
#include "hxnet.h"
#include "sockimp.h"
#include "hxresult.h"
#include "hxerror.h"
#include "hxassert.h"

#include "hxmap.h"
#include "hxslist.h"

#include "resolvcache.h"

/*****************************************************************************
 *
 * CAddrInfo, CNodeInfo
 *
 *****************************************************************************/

CAddrInfo::CAddrInfo(void) :
    m_uVecAlloc(0),
    m_uVecLen(0),
    m_ptExpireVec(NULL),
    m_ppAddrVec(NULL)
{
    // Empty
}

CAddrInfo::~CAddrInfo(void)
{
    DelAll();
}

void
CAddrInfo::Add(UINT32 uVecLen, time_t* ptExpireVec, IHXSockAddr** ppAddrVec)
{
    UINT32 uV4cnt = 0;
    UINT32 uV6cnt = 0;
    UINT32 n;

    if (!uVecLen)
    {
        return;
    }

    if (m_uVecLen)
    {
        for (n = 0; n < uVecLen; n++)
        {
            HXSockFamily f = ppAddrVec[n]->GetFamily();
            switch (f)
            {
            case HX_SOCK_FAMILY_IN4:
                uV4cnt++;
                break;

            case HX_SOCK_FAMILY_IN6:
                uV6cnt++;
                break;
            }
        }

        if (uV4cnt && uV6cnt)
        {
            DelAll();
        }
        else
        {
            if (uV4cnt)
            {
                Del(HX_SOCK_FAMILY_IN4);
            }
            if (uV6cnt)
            {
                Del(HX_SOCK_FAMILY_IN6);
            }
        }
    }
        
    if (m_uVecAlloc < m_uVecLen + uVecLen)
    {
        time_t* ptNewExpireVec = new time_t[m_uVecLen + uVecLen];
        IHXSockAddr** ppNewAddrVec = new IHXSockAddr*[m_uVecLen + uVecLen];
        memcpy(ptNewExpireVec, m_ptExpireVec, m_uVecLen*sizeof(time_t));
        memcpy(ppNewAddrVec, m_ppAddrVec, m_uVecLen*sizeof(IHXSockAddr*));
        HX_VECTOR_DELETE(m_ptExpireVec);
        HX_VECTOR_DELETE(m_ppAddrVec);
        m_ptExpireVec = ptNewExpireVec;
        m_ppAddrVec = ppNewAddrVec;
        m_uVecAlloc = m_uVecLen + uVecLen;
    }
    for (n = 0; n < uVecLen; n++)
    {
        m_ptExpireVec[m_uVecLen+n] = ptExpireVec[n];
        m_ppAddrVec[m_uVecLen+n] = ppAddrVec[n];
        m_ppAddrVec[m_uVecLen+n]->AddRef();
    }
    m_uVecLen += uVecLen;
}

void
CAddrInfo::Del(HXSockFamily f)
{
    UINT32 nPos, nCur;
    nPos = 0;
    for (nCur = 0; nCur < m_uVecLen; nCur++)
    {
        if (m_ppAddrVec[nCur] != NULL)
        {
            if (m_ppAddrVec[nCur]->GetFamily() == f)
            {
                m_ptExpireVec[nCur] = 0;
                HX_RELEASE(m_ppAddrVec[nCur]);
            }
            else
            {
                if (nPos != nCur)
                {
                m_ptExpireVec[nPos] = m_ptExpireVec[nCur];
                m_ptExpireVec[nCur] = 0;
                m_ppAddrVec[nPos] = m_ppAddrVec[nCur];
                m_ppAddrVec[nCur] = NULL;
                }
                if (m_ppAddrVec[nPos])
                {
                nPos++;
            }
        }
    }
    }
    m_uVecLen = nPos;
    if (m_uVecLen == 0)
    {
        HX_VECTOR_DELETE(m_ptExpireVec);
        HX_VECTOR_DELETE(m_ppAddrVec);
        m_uVecAlloc = 0;
    }
}

void
CAddrInfo::DelAll(void)
{
    UINT32 n;
    for (n = 0; n < m_uVecLen; n++)
    {
        HX_RELEASE(m_ppAddrVec[n]);
    }
    HX_VECTOR_DELETE(m_ptExpireVec);
    HX_VECTOR_DELETE(m_ppAddrVec);
    m_uVecLen = m_uVecAlloc = 0;
}

UINT32
CAddrInfo::Expire(time_t now)
{
    UINT32 nPos, nCur;
    nPos = 0;
    for (nCur = 0; nCur < m_uVecLen; nCur++)
    {
        if (m_ppAddrVec[nCur] != NULL)
        {
            if (m_ptExpireVec[nCur] < now)
            {
                m_ptExpireVec[nCur] = 0;
                HX_RELEASE(m_ppAddrVec[nCur]);
            }
            else
            {
                m_ptExpireVec[nPos] = m_ptExpireVec[nCur];
                m_ppAddrVec[nPos] = m_ppAddrVec[nCur];
                nPos++;
            }
        }
    }
    m_uVecLen = nPos;
    if (m_uVecLen == 0)
    {
        HX_VECTOR_DELETE(m_ptExpireVec);
        HX_VECTOR_DELETE(m_ppAddrVec);
        m_uVecAlloc = 0;
    }
    return m_uVecLen;
}

/*****************************************************************************
 *
 * CResolverCache
 *
 *****************************************************************************/

CResolverCache::CResolverCache(IUnknown* punkContext) :
    m_nRefCount(0),
    m_punkContext(punkContext),
    m_pScheduler(NULL)
{
    m_punkContext->AddRef();

    m_punkContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    HX_ASSERT(m_pScheduler != NULL);

    ReadConfig();
}

CResolverCache::~CResolverCache(void)
{
    POSITION pos;

    pos = m_mapAddrInfo.GetStartPosition();
    while(pos)
    {
        const char* pHost = NULL;
        CAddrInfo* pInfo = NULL;
        m_mapAddrInfo.GetNextAssoc(pos, pHost, (void*&)pInfo);
        delete pInfo;
    }

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_punkContext);
}

STDMETHODIMP
CResolverCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXResolve*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXResolve))
    {
        AddRef();
        *ppvObj = (IHXResolve*)this;
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
CResolverCache::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CResolverCache::Release(void)
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
CResolverCache::Func(void)
{
    time_t tNow = time(NULL);

    // Prune cache once per hour
    if(tNow >= m_tLastPrune + 3600)
    {
        m_tLastPrune = tNow;
        POSITION pos = m_mapAddrInfo.GetStartPosition();
        while(pos)
        {
            const char* pHost = NULL;
            CAddrInfo* pInfo = NULL;
            m_mapAddrInfo.GetNextAssoc(pos, pHost, (void*&)pInfo);
            pInfo->Expire(tNow);
            if (pInfo->IsEmpty())
            {
                m_mapAddrInfo.RemoveKey(pHost);
                delete pInfo;
            }
        }
    }
    return HXR_OK;
}

HXBOOL
CResolverCache::Init(void)
{
    return TRUE;
}

void
CResolverCache::AddAddrInfo(const char* pNode, UINT32 uVecLen,
                time_t* ptExpireVec, IHXSockAddr** ppAddrVec)
{
    CAddrInfo* pInfo;

    if (!m_mapAddrInfo.Lookup(pNode, (void*&)pInfo))
    {
        pInfo = new CAddrInfo();
        m_mapAddrInfo.SetAt(pNode, pInfo);
    }

    pInfo->Add(uVecLen, ptExpireVec, ppAddrVec);
}

void
CResolverCache::ReplaceAddrInfo(const char* pNode, UINT32 uVecLen,
                time_t* ptExpireVec, IHXSockAddr** ppAddrVec)
{
    CAddrInfo* pInfo = NULL;
    if (m_mapAddrInfo.Lookup(pNode, (void*&)pInfo))
    {
        m_mapAddrInfo.RemoveKey(pNode);
        delete pInfo;
    }

    AddAddrInfo(pNode, uVecLen, ptExpireVec, ppAddrVec);
}

HXBOOL
CResolverCache::FindAddrInfo(const char* pNode, UINT32& uVecLen, IHXSockAddr**& ppVec)
{
    CAddrInfo* pInfo = NULL;
    if (!m_mapAddrInfo.Lookup(pNode, (void*&)pInfo))
    {
        return FALSE;
    }

    time_t tNow = time(NULL);
    pInfo->Expire(tNow);
    if (pInfo->IsEmpty())
    {
        m_mapAddrInfo.RemoveKey(pNode);
        delete pInfo;
        return FALSE;
    }

    pInfo->Get(uVecLen, ppVec);
    return TRUE;
}

HXBOOL
CResolverCache::ReadConfig(void)
{
    FILE* fp;
    char linebuf[1024];
    char* pval;
    char* p;

    IHXNetServices* pNetSvc = NULL;
    m_punkContext->QueryInterface(IID_IHXNetServices, (void**)&pNetSvc);
    if (pNetSvc == NULL)
    {
        return FALSE;
    }

#if defined(_UNIX)
    fp = fopen("/etc/hosts", "r");
#elif defined(_WIN32)
#ifndef UNDER_CE
    char szPath[255];
    ExpandEnvironmentStrings("%windir%\\system32\\drivers\\etc\\hosts",
                    szPath, sizeof(szPath));
    fp = fopen(szPath, "r");
#endif //!UNDER_CE
#else
#error "Don't know how to open platform hosts file"
#endif
    if (fp != NULL)
    {
        while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
        {
            time_t tExpire = (time_t)0x7fffffff; // XXXTDM: MAX_TIME_T
            IHXBuffer* pBuf = NULL;
            IHXSockAddr* pAddr = NULL;
            char szhost[MAX_HOST_LEN];
            CAddrInfo* pInfo = NULL;

            // First get the addr
            p = linebuf;
            if (*p == '\0' || *p == '#')
                continue;
            while (*p && !isspace(*p))
                p++;

            if (p - linebuf > HX_ADDRSTRLEN)
            {
                continue;
            }

            memcpy(szhost, linebuf, p-linebuf);
            szhost[p-linebuf] = '\0';

            CHXBuffer::FromCharArray(szhost, &pBuf);
            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
            if (pAddr->SetAddr(pBuf) != HXR_OK)
            {
                pAddr->Release();
                pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN6, &pAddr);
                if (pAddr->SetAddr(pBuf) != HXR_OK)
                {
                    pAddr->Release();
                    pBuf->Release();
                    continue;
                }
            }

            pval = p;
            while (*pval)
            {
                while (isspace(*pval)) pval++;
                if (!*pval)
                {
                    break;
                }
                p = pval;
                while (*p && !isspace(*p))
                {
                    p++;
                }
                if (p - pval >= MAX_HOST_LEN)
                {
                    break;
                }
                if (p > pval)
                {
                    memcpy(szhost, pval, p-pval);
                    szhost[p-pval] = '\0';
                    m_mapAddrInfo.Lookup(szhost, (void*&)pInfo);
                    AddAddrInfo(szhost, 1, &tExpire, &pAddr);
                    pval = p;
                }
            }
            HX_RELEASE(pAddr);
            HX_RELEASE(pBuf);
        }

        fclose(fp);
    }

    HX_RELEASE(pNetSvc);

    return TRUE;
}
