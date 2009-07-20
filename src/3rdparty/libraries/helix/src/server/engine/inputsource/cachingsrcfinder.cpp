/* ***** BEGIN LICENSE BLOCK *****  
* Source last modified: $Id: cachingsrcfinder.cpp,v 1.2 2007/05/10 18:44:13 seansmith Exp $ 
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
#include "url.h"
#include "timerep.h"
#include "errmsg_macros.h"
#include "server_context.h"
#include "source_container.h"
#include "server_request.h"
#include "ff_source.h"
#include "static_source_container.h"

#include "config.h"
#include "hxstats.h"
#include "hxprot.h"
#include "hxxfile.h"
#include "bcastmgr.h"

#include "ispifs.h"
#include "isifs.h"

#include "hxqos.h"
#include "qos_cfg_names.h"

#include "fileformat_handler.h"
#include "asmstreamfilter.h"
#include "cachingsrcfinder.h"


/*** CachingSourceFinder ***/

// See GETTIMEOFDAY in server_engine.cpp

#ifdef _UNIX
#define GETTIME(t) (*g_bITimerAvailable) ? t = g_pNow->tv_sec : t = time(NULL)
#else
#define GETTIME(t) t = time(NULL);
#endif

// static initializations

#define DEFAULT_CACHE_PRUNEFREQ  5
#define DEFAULT_CACHE_MAXAGE     60

CHXMapStringToOb* CachingSourceFinder::zm_pCache = NULL;
CachingSourceFinder::CCachePruneCB* CachingSourceFinder::zm_pCachePruneCB = NULL;
CallbackHandle CachingSourceFinder::zm_hCachePruneCB = 0;
UINT32 CachingSourceFinder::zm_uCachePruneFreq = DEFAULT_CACHE_PRUNEFREQ;
UINT32 CachingSourceFinder::zm_uCacheMaxAge = DEFAULT_CACHE_MAXAGE;

CachingSourceFinder::CachingSourceFinder(Process* pProc,
        ClientSession* pSession) :
    BasicSourceFinder(pProc, pSession)
{
}

CachingSourceFinder::~CachingSourceFinder(void)
{
}

HX_RESULT
CachingSourceFinder::FindSource(URL* pURL, ServerRequest* pRequest)
{
    //XXXTDM: Call base method until we enable caching
    return BasicSourceFinder::FindSource(pURL, pRequest);
}

HX_RESULT
CachingSourceFinder::FindNextSource(void)
{
    //XXXTDM: Call base method until we enable caching
    return BasicSourceFinder::FindNextSource();
}

void
CachingSourceFinder::CacheSource(URL* pUrl)
{
    if (pUrl == NULL || pUrl->full == NULL)
    {
        HX_ASSERT(FALSE);
        return;
    }

    if (zm_pCache == NULL)
    {
        INT32 iVal;
        ServerRegistry* pReg = m_pProc->pc->registry;
        if ((iVal = m_pProc->pc->config->GetInt(m_pProc,
                                         "config.SourceCache.PruneFreq")) == 0)
        {
            iVal = DEFAULT_CACHE_PRUNEFREQ;
        }
        zm_uCachePruneFreq = (UINT32)iVal;
        if ((iVal = m_pProc->pc->config->GetInt(m_pProc,
                                            "config.SourceCache.MaxAge")) == 0)
        {
            iVal = DEFAULT_CACHE_MAXAGE;
        }
        zm_uCacheMaxAge = (UINT32)iVal;

        zm_pCache = new CHXMapStringToOb;
        HX_ASSERT(zm_pCachePruneCB == NULL);
        zm_pCachePruneCB = new CCachePruneCB(m_pProc);
        zm_pCachePruneCB->AddRef(); // Prevent destruction
        Timeval tvcb(m_pProc->pc->engine->now + Timeval(zm_uCachePruneFreq,0));
        zm_hCachePruneCB = m_pProc->pc->engine->schedule.enter(tvcb,
                                                             zm_pCachePruneCB);
    }
    CHXSimpleList* pList = NULL;
    if (!zm_pCache->Lookup(pUrl->full, (void*&)pList))
    {
        pList = new CHXSimpleList;
        zm_pCache->SetAt(pUrl->full, pList);
    }

    //XXXTDM: Get rid of the CacheEntry object if possible
    CacheEntry* pEntry = new CacheEntry;
    GETTIME(pEntry->m_tCacheEnter);
    pEntry->m_pFileObject =         m_pFileObject;          m_pFileObject->AddRef();
    pEntry->m_pFileRequest =        m_pFileRequest;         m_pFileRequest->AddRef();
    pEntry->m_pFileFormatObject =   m_pFileFormatObject;    m_pFileFormatObject->AddRef();
    pEntry->m_pFileFormatRequest =  m_pFileFormatRequest;   m_pFileFormatRequest->AddRef();
    pEntry->m_pSourceControl =      m_pSourceControl;       m_pSourceControl->AddRef();
    pList->AddTail(pEntry);
}

IUnknown*
CachingSourceFinder::FindCachedSource(URL* pUrl, IHXRequest* pRequest)
{
    IUnknown* pSource = NULL;
    if (zm_pCache != NULL)
    {
        CHXSimpleList* pList = NULL;
        if (zm_pCache->Lookup(pUrl->full, (void*&)pList))
        {
            if (!pList->IsEmpty())
            {
                CacheEntry* pEntry = (CacheEntry*)pList->RemoveHead();

                // Release for list, AddRef for save
                m_pFileObject =         pEntry->m_pFileObject;
                m_pFileRequest =        pEntry->m_pFileRequest;
                m_pFileFormatObject =   pEntry->m_pFileFormatObject;
                m_pFileFormatRequest =  pEntry->m_pFileFormatRequest;
                m_pSourceControl =      pEntry->m_pSourceControl;

                pSource = m_pSourceControl;
                pSource->AddRef(); // for out-param

                delete pEntry;
            }
        }
    }
    return pSource;
}

void
CachingSourceFinder::CachePrune(void)
{
    HX_ASSERT(zm_pCache != NULL);

    POSITION pos;
    CHXString strUrl;
    CHXSimpleList* pList;
    CacheEntry* pEntry;

    time_t tNow;
    GETTIME(tNow);

    // Calculate memory usage percentage
    // Note we always use KB to prevent overflow
    UINT32 uMemUsePct = (100*(SharedMemory::BytesInUse()/1024)) /
                             (SharedMemory::BytesInPool()/1024);

    // If memory usage is:
    //    0% to 74% : Normal cache aging
    //   75% to 94% : Linear scale from CACHE_MAX_AGE to zero
    //   95% and up : Zero (immediately remove all items)
    UINT32 uMaxAge = zm_uCacheMaxAge;
    if (uMemUsePct >= 75)
    {
        if (uMemUsePct >= 95)
        {
            uMaxAge = 0;
        }
        else
        {
            uMaxAge = (95-uMemUsePct) * zm_uCacheMaxAge / (95-75);
        }
    }

    // Iterate the cache map, tossing out entries over uMaxAge
    pos = zm_pCache->GetStartPosition();
    while (pos)
    {
        zm_pCache->GetNextAssoc(pos, strUrl, (void*&)pList);
        while (!pList->IsEmpty())
        {
            pEntry = (CacheEntry*)pList->GetHead();
            HX_ASSERT(pEntry->m_tCacheEnter != INVALID_TIME_T);
            if (pEntry->m_tCacheEnter+uMaxAge > tNow)
            {
                break;
            }
            HX_RELEASE(pEntry->m_pSourceControl);
            delete pEntry;
            pList->RemoveHead();
        }
        if (pList->IsEmpty())
        {
            delete pList;
            zm_pCache->RemoveKey(strUrl);
        }
    }

    if (zm_pCache->IsEmpty())
    {
        HX_RELEASE(zm_pCachePruneCB);
        zm_hCachePruneCB = 0;
        HX_DELETE(zm_pCache);
    }
    else
    {
        // Schedule next callback
        Process* pProc = zm_pCachePruneCB->m_pProc;
        Timeval tvcb(pProc->pc->engine->now + Timeval(zm_uCachePruneFreq,0));
        zm_hCachePruneCB = pProc->pc->engine->schedule.enter(tvcb,
                                                             zm_pCachePruneCB);
    }
}


CachingSourceFinder::CCachePruneCB::CCachePruneCB(Process* pProc) :
    m_ulRefCount(0),
    m_pProc(pProc)
{
    // Empty
}

CachingSourceFinder::CCachePruneCB::~CCachePruneCB(void)
{
    // Empty
}

STDMETHODIMP
CachingSourceFinder::CCachePruneCB::QueryInterface(REFIID riid, void** ppvObj)
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

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CachingSourceFinder::CCachePruneCB::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CachingSourceFinder::CCachePruneCB::Release(void)
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CachingSourceFinder::CCachePruneCB::Func(void)
{
    CachingSourceFinder::CachePrune();
    return HXR_OK;
}
