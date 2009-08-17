/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rtspstats.cpp,v 1.14 2004/07/28 00:22:53 darrick Exp $ 
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

#include <stdlib.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "debug.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "hxtime.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "ihxpckts.h"
#include "hxerror.h"
#include "defslice.h"

#include "servsked.h"
#include "dispatchq.h"
#include "proc.h"
#include "proc_container.h"
#include "mutex.h"

#include "rtspstats.h"
#include "hxstats.h"


#if defined _WINDOWS && !defined snprintf
#define snprintf _snprintf
#endif


#define IfFailGo(x) { if (FAILED(x)) { goto cleanup; } }

/* Init static vars */
const char* RTSPStats::zm_szTStatTemplate = "[%u;%s%s;%u] ";

const char* szRegKeyProxyRoot = "Proxy";

static RTSPEventID z_EventIdList[] = {{RTSP_UNKNOWN,   "",   0x0},
                                      {RTSP_ANNOUNCE,  "ANNOUNCE", 0x0001},
                                      {RTSP_DESCRIBE,  "DESCRIBE", 0x0002},
                                      {RTSP_GET_PARAM, "GET_PARAMETER", 0x0004},
                                      {RTSP_OPTIONS,   "OPTIONS", 0x0008},
                                      {RTSP_PAUSE,     "PAUSE", 0x0010},
                                      {RTSP_PLAY,      "PLAY", 0x0020},
                                      {RTSP_RECORD,    "RECORD", 0x0040},
                                      {RTSP_REDIRECT,  "REDIRECT", 0x0080},
                                      {RTSP_SETUP,     "SETUP", 0x0100},
                                      {RTSP_SET_PARAM, "SET_PARAMETER", 0x0200},
                                      {RTSP_TEARDOWN,  "TEARDOWN", 0x0400},
                                      {RTSP_EXTENSION, "",   0x0800}};

RTSPStats::RTSPEventCode 
RTSPStats::zm_EventCodeList[] = {{CLIENT_REQUEST, "CQ"}, 
                                 {SERVER_REQUEST, "SQ"},
                                 {CLIENT_RESPONSE, "CR"},
                                 {SERVER_RESPONSE, "SR"}};


RTSPAggregateStats::RTSPStatInfo
RTSPAggregateStats::zm_StatInfoList[] = {
                            {(RTSPReqMethodType)0, RTSP_UNKNOWN,   NULL},
                            {CLIENT_REQUEST, RTSP_ANNOUNCE,  ".Announce"},
                            {SERVER_REQUEST, RTSP_ANNOUNCE,  ".Announce"},
                            {CLIENT_REQUEST, RTSP_DESCRIBE,  ".Describe"},
                            {CLIENT_REQUEST, RTSP_GET_PARAM, ".GetParameter"},
                            {SERVER_REQUEST, RTSP_GET_PARAM, ".GetParameter"},
                            {CLIENT_REQUEST, RTSP_OPTIONS,   ".Options"},
                            {SERVER_REQUEST, RTSP_OPTIONS,   ".Options"},
                            {CLIENT_REQUEST, RTSP_PAUSE,     ".Pause"},
                            {CLIENT_REQUEST, RTSP_PLAY,      ".Play"},
                            {CLIENT_REQUEST, RTSP_RECORD,    ".Record"},
                            {SERVER_REQUEST, RTSP_REDIRECT,  ".Redirect"},
                            {CLIENT_REQUEST, RTSP_SETUP,     ".Setup"},
                            {CLIENT_REQUEST, RTSP_SET_PARAM, ".SetParameter"},
                            {SERVER_REQUEST, RTSP_SET_PARAM, ".SetParameter"},
                            {CLIENT_REQUEST, RTSP_TEARDOWN,  ".Teardown"},
                            {(RTSPReqMethodType)0, RTSP_EXTENSION, NULL}};

const char*
RTSPAggregateStats::zm_StatCounters[NUM_RTSP_REQUEST_TYPES][NUM_COUNTERS] =
                                              {{".ClientRequestTotal", 
                                                ".ServerResponseSuccessCount", 
                                                ".ServerResponse4XXErrorCount", 
                                                ".ServerResponse5XXErrorCount"},
                                               {".ServerRequestTotal",
                                                ".ClientResponseSuccessCount", 
                                                ".ClientResponse4XXErrorCount", 
                                                ".ClientResponse5XXErrorCount"}};

const char* szServerStatsRegRoot = "Server.AggregateRTSPStatistics";
const char* szProxyStatsRegRoot = "proxy.AggregateRTSPStatistics";

const UINT32 MAX_ERROR_BUFF_SIZE = 256;
const UINT32 MAX_REGISTRY_NAME_SIZE = 256;

static const char* z_szConfigEventsEnabled = "config.RTSP Events.Enabled";
static const char* z_szConfigEventsMaxEvents = "config.RTSP Events.MaxEventsPerLine";
static const char* z_szConfigEventsEventMaskList = "config.RTSP Events.RTSPMessages";

static const char* z_szConfigAggregateStatsEnabled = "config.RTSP Aggregate Statistics.Enabled";
static const char* z_szConfigAggregateStatsEventMaskList = "config.RTSP Aggregate Statistics.RTSPMessages";

BOOL RTSPStats::zm_bConfigRead = FALSE;
BOOL RTSPStats::zm_bLicenseChecked = FALSE;
BOOL RTSPStats::zm_bEnabled = FALSE;
BOOL RTSPStats::zm_bLicensed = FALSE;
UINT32 RTSPStats::zm_ulEventMask = 0;
UINT32 RTSPStats::zm_ulMaxEvents = 0;

const UINT32 g_ulDefaultEventsPerLine = 5;
const UINT32 g_ulMaxPossibleEventsPerLine = 10;

///////////////////////////////////////////////////////////////////////////////
// Top level functions.
///////////////////////////////////////////////////////////////////////////////

static HX_RESULT
GetEventId(const char* szEventName,
           UINT32& ulEventId)
{
    HX_RESULT rc = HXR_OK;
    ulEventId = RTSP_UNKNOWN;

    if (!szEventName)
    {
        rc = HXR_UNEXPECTED;
        return rc;
    }

    if (!strncasecmp("AN", szEventName, 2))         ulEventId = RTSP_ANNOUNCE; 
    else if(!strncasecmp("DE", szEventName, 2))     ulEventId = RTSP_DESCRIBE;
    else if(!strncasecmp("OP", szEventName, 2))     ulEventId = RTSP_OPTIONS;
    else if(!strncasecmp("PA", szEventName, 2))     ulEventId = RTSP_PAUSE;
    else if(!strncasecmp("PL", szEventName, 2))     ulEventId = RTSP_PLAY;
    else if(!strncasecmp("TE", szEventName, 2))     ulEventId = RTSP_TEARDOWN;
    else if(!strncasecmp("REC", szEventName, 3))    ulEventId = RTSP_RECORD;
    else if(!strncasecmp("RED", szEventName, 3))    ulEventId = RTSP_REDIRECT;
    else if(!strncasecmp("SET_", szEventName, 4))   ulEventId = RTSP_SET_PARAM;
    else if(!strncasecmp("GET_", szEventName, 4))   ulEventId = RTSP_GET_PARAM;
    else if(!strncasecmp("SETU", szEventName, 4))   ulEventId = RTSP_SETUP;
    else
    {
        DPRINTF(D_INFO,("RTSP_EVENT_STATS: Ignoring unknown event %s\n",
                        szEventName));
        rc = HXR_FAIL;
    }

    return rc;
}


///////////////////////////////////////////////////////////////////////////////
// RTSPEventsManagerProcWrapper implementation
///////////////////////////////////////////////////////////////////////////////

RTSPEventsManagerProcWrapper::RTSPEventsManagerProcWrapper(RTSPEventsManager* pEventsMgr,
                                                           Process* pProc)
: m_ulRefCount(0)
, m_pEventsMgr(pEventsMgr)
, m_pProc(pProc)
{
}


RTSPEventsManagerProcWrapper::~RTSPEventsManagerProcWrapper()
{
}


STDMETHODIMP 
RTSPEventsManagerProcWrapper::RegisterSink(IHXRTSPEventsSink* pSink)
{
    HX_ASSERT(m_pEventsMgr);
    return m_pEventsMgr->RegisterSink(pSink, m_pProc);
}


STDMETHODIMP 
RTSPEventsManagerProcWrapper::RemoveSink(IHXRTSPEventsSink* pSink)
{    
    HX_ASSERT(m_pEventsMgr);
    return m_pEventsMgr->RemoveSink(pSink);
}


STDMETHODIMP_(IHXRTSPAggregateEventStats*)
RTSPEventsManagerProcWrapper::GetAggregateStats()
{
    HX_ASSERT(m_pEventsMgr);
    return m_pEventsMgr->GetAggregateStats();
}


STDMETHODIMP
RTSPEventsManagerProcWrapper::OnRTSPEvents(IHXClientStats* pClientStats,
                                           IHXSessionStats* pSessionStats,
                                           IHXBuffer* pEvents,
                                           IHXBuffer* pRTSPSessionID)
{
    HX_ASSERT(m_pEventsMgr);
    return m_pEventsMgr->OnRTSPEvents(pClientStats,
                                      pSessionStats,
                                      pEvents,
                                      pRTSPSessionID);
}


STDMETHODIMP
RTSPEventsManagerProcWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPEventsManager))
    {
        AddRef();
        *ppvObj = (IHXRTSPEventsManager*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(UINT32)
RTSPEventsManagerProcWrapper::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(UINT32)
RTSPEventsManagerProcWrapper::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// RTSPEventsManager implementation
///////////////////////////////////////////////////////////////////////////////

RTSPEventsManager::RTSPEventsManager()
: m_bInitDone(FALSE)
, m_pAggStats(NULL)
{
}


RTSPEventsManager::~RTSPEventsManager()
{
// Users should call cleanup when done with this manager.
}


HX_RESULT 
RTSPEventsManager::Init(IUnknown* pContext)
{
    HX_RESULT hr = HXR_OK;

    if (m_bInitDone)
    {
        return HXR_OK;
    }

    m_pAggStats = new RTSPAggregateStats;

    // Catastrophic failure. This should never happen.
    if (!m_pAggStats)
    {
        return HXR_UNEXPECTED;
    }

    m_pAggStats->AddRef();

    // Even if this init fails, this object still needs to exist.
    hr = m_pAggStats->Init(pContext);

    m_bInitDone = TRUE;

    return hr;
}


// This object should exist throughout the life of the server.
// So currently nothing calls this method.
void
RTSPEventsManager::Cleanup()
{
    
    CHXSimpleList::Iterator itr = m_SinkList.Begin();
    for ( ; itr != m_SinkList.End(); ++itr)
    {
        SinkListElem* pSink = (SinkListElem*)*itr;
        HX_DELETE(pSink);
    }   

    m_SinkList.RemoveAll();

    HX_RELEASE(m_pAggStats);    
}


HX_RESULT
RTSPEventsManager::RegisterSink(IHXRTSPEventsSink* pSink,
                                Process* pProc)
{
    if (!pSink || !pProc)
    {
        return HXR_FAIL;
    }

    SinkListElem* pElem = new SinkListElem();

    pElem->m_pSink = pSink;
    pElem->m_pSink->AddRef();

    pElem->m_pProc = pProc;

    m_SinkList.AddTail((void*)pElem);

    return HXR_OK;
}


HX_RESULT
RTSPEventsManager::RemoveSink(IHXRTSPEventsSink* pSink)
{    
    BOOL bFound = FALSE;
    SinkListElem* pElem = NULL;

    HX_RESULT hr = HXR_OK;

    if (!pSink)
    {
        return HXR_OK;
    }

    // We can't just Find() pSink, we're looking for a
    // SinkListElem object.
    CHXSimpleList::Iterator itr = m_SinkList.Begin();
    for ( ; itr != m_SinkList.End(); ++itr)
    {
        pElem = (SinkListElem*)*itr;
        
        if (pElem->m_pSink == pSink)
        {
            bFound = TRUE;
            break;
        }
    }   

    if (bFound)
    {
        HX_ASSERT(pElem);

        LISTPOSITION pos = m_SinkList.Find((void*)pElem);

        HX_ASSERT(pos);

 
#ifdef _DEBUG
        SinkListElem* pTest = NULL;

        pTest = (SinkListElem*)m_SinkList.GetAt(pos);

        HX_ASSERT(pTest == pElem);
#endif

        m_SinkList.RemoveAt(pos);
        HX_DELETE(pElem);
    }
    else
    {
        hr = HXR_FAIL;
    }

    return HXR_FAIL;
}


IHXRTSPAggregateEventStats*
RTSPEventsManager::GetAggregateStats()
{
    IHXRTSPAggregateEventStats* pAggStats = m_pAggStats;
    if (pAggStats)
    {
        pAggStats->AddRef();
    }

    return pAggStats;
}


HX_RESULT
RTSPEventsManager::OnRTSPEvents(IHXClientStats* pClientStats,
                                IHXSessionStats* pSessionStats,
                                IHXBuffer* pEvents,
                                IHXBuffer* pRTSPSessionID)
{
    CHXSimpleList::Iterator itr = m_SinkList.Begin();
    for ( ; itr != m_SinkList.End(); ++itr)
    {
        SinkListElem* pElem = (SinkListElem*)*itr;

        HX_ASSERT(pElem->m_pSink);

        pElem->m_pSink->OnRTSPEvents(pClientStats, 
                                     pSessionStats, 
                                     pEvents, 
                                     pRTSPSessionID);
    }   

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// RTSPAggregateStats Implementation
///////////////////////////////////////////////////////////////////////////////

RTSPAggregateStats::RTSPAggregateStats()
: m_ulRefCount(0)
, m_pRegistry(NULL)
, m_pErrorLog(NULL)
, m_ulEventMask(0)
//, m_pPropWatch(NULL)
, m_szStatsRegRoot(NULL)
, m_bIsProxy(FALSE)
, m_bLicensed(FALSE)
, m_bLicenseChecked(FALSE)
, m_bInitDone(FALSE)
, m_bEnabled(FALSE)
{
    EventCounterInfo* pEventCounter = NULL;

    for (UINT32 i = 0; i < NUM_RTSP_REQUEST_TYPES; ++i)
    {
        for (UINT32 j = 0; j < RTSPStats::MAX_RTSP_METHODS; ++j)
        {
            for (UINT32 k = 0; k < NUM_COUNTERS; ++k)
            {
                pEventCounter = &(m_EventCounterVec[i][j][k]);
                pEventCounter->bOverflow = FALSE;
                pEventCounter->ulRegId = 0;
                pEventCounter->nCounter = 0;
            }
        }
    }
    
    /*
    memset(m_EventMaskRegIdVec, 0, 
          sizeof(UINT32) * RTSPStats::MAX_RTSP_METHODS);
    */
}

RTSPAggregateStats::~RTSPAggregateStats()
{
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pErrorLog);

  /*
    if (m_pPropWatch)
    {
        ClearRegistryWatches();
        HX_RELEASE(m_pPropWatch);
    }
    */
}

STDMETHODIMP
RTSPAggregateStats::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPAggregateEventStats))
    {
        AddRef();
        *ppvObj = (IHXRTSPAggregateEventStats*)this;
        return HXR_OK;
    }
    /*
    else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
        AddRef();
        *ppvObj = (IHXPropWatchResponse*)this;
        return HXR_OK;
    }
    */

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
RTSPAggregateStats::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RTSPAggregateStats::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

HX_RESULT
RTSPAggregateStats::Init(IUnknown* pContext)
{
    HX_RESULT hr = HXR_OK;
    BOOL bDummy = TRUE;

    if (m_bInitDone)
    {        
        return HXR_OK;
    }

    HX_ASSERT(pContext);

    IfFailGo(hr = pContext->QueryInterface(IID_IHXRegistry2, (void**)&m_pRegistry));
    IfFailGo(hr = pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrorLog));
    IfFailGo(hr = ReadConfig());

    if (!m_bEnabled)
    {
        hr = HXR_OK;
        goto cleanup;
    }

    IfFailGo(hr = GetLicense(bDummy));
    IfFailGo(hr = SetupRegistryEntries());

    m_bInitDone = TRUE;

    return HXR_OK;

cleanup:

    m_bEnabled = FALSE;
    m_bInitDone = TRUE;

    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pErrorLog);

    return hr;
}


HX_RESULT
RTSPAggregateStats::ReadConfig()
{
    HX_RESULT hr = HXR_OK;
    INT32 nTemp = 0;
    IHXValues* pEventList = NULL;
    const char* szPropName = NULL;
    const char* szEventName = NULL;
    UINT32 ulPropId = 0;
    INT32 lEnabled = 0;
    UINT32 ulEventId = RTSP_UNKNOWN;
    UINT32 ulEventBitMask = 0;
    
    HX_ASSERT(m_pRegistry);

    if (SUCCEEDED(m_pRegistry->GetIntByName(z_szConfigAggregateStatsEnabled, nTemp)))
    {
        m_bEnabled = nTemp == 1 ? TRUE : FALSE;
    }
    else
    {
        m_bEnabled = FALSE;
    }

    if (!m_bEnabled)
    {
        return HXR_OK;
    }

    if(SUCCEEDED(m_pRegistry->GetPropListByName(z_szConfigAggregateStatsEventMaskList, pEventList))
    && SUCCEEDED(pEventList->GetFirstPropertyULONG32(szPropName, ulPropId)))
    {
        do
        {
            ulEventId = RTSP_UNKNOWN;
            szEventName = strrchr(szPropName, '.');

            if (!szEventName) 
            {
                continue;
            }
            
            ++szEventName;
            HX_RESULT status = m_pRegistry->GetIntById(ulPropId, lEnabled);

            if (FAILED(status)) 
            {
                continue;
            }
           
            status = GetEventId(szEventName, ulEventId);
            
            if ((HXR_OK == status) && (ulEventId < RTSPStats::MAX_RTSP_METHODS))
            {
                if ((lEnabled > 0) && ulEventId != RTSP_UNKNOWN)
                {
                    ulEventBitMask = z_EventIdList[ulEventId].ulEventMask;
                    m_ulEventMask |= ulEventBitMask;
                }
            }

        } while(SUCCEEDED(pEventList->GetNextPropertyULONG32(szPropName, ulPropId)));

        DPRINTF(D_INFO, ("RTSP_AGGREGATE_STATS: Event mask -> (0x%x)\n", m_ulEventMask));        
    }
    else
    {
        m_bEnabled = FALSE;
        m_pErrorLog->Report(HXLOG_ERR,
                            HXR_OK,
                            0,
                            "RTSP_AGGREGATE_STATS: Config error - could not find event reporting flags.",
                            NULL);
        hr = HXR_FAIL;        
    }

    HX_RELEASE(pEventList);

    return hr;    
}


HX_RESULT
RTSPAggregateStats::SetupRegistryEntries()
{
    HX_RESULT rc = HXR_OK;
    EventCounterInfo* pEC = NULL; 
    const char* szCounterPropName = NULL;
    const char* szMethodPropName = NULL;
    RTSPMethod ulMethodId = RTSP_UNKNOWN;
    RTSPReqMethodType ulMethodType;

    HX_ASSERT(m_bEnabled);

#ifdef _DEBUG
    // We can't tell if we're a proxy until the license tree exists.     
    if (m_pRegistry->GetId("license.Summary") == 0)
    {
        HX_ASSERT(!"License registry tree required to init aggregate stats!");
        // In fail case, fall through in release case 
        // and write to server.AggregateRTSPStatistics instead.
    }
#endif
   
    INT32 nProxy;
    if(SUCCEEDED(m_pRegistry->GetIntByName(REGISTRY_RTSPPROXY_ENABLED, nProxy)) && nProxy)
    {
        m_bIsProxy = TRUE;
        m_szStatsRegRoot = szProxyStatsRegRoot;
        UINT32 ulProxyRoot = m_pRegistry->GetId(szRegKeyProxyRoot);
        // Proxy root hasn't been created in registry yet.
        if (!ulProxyRoot)
        {
            ulProxyRoot = m_pRegistry->AddComp("Proxy");
        }
    }
    else
    {
        m_szStatsRegRoot = szServerStatsRegRoot;
    }

    UINT32 ulRegRootLen = strlen(m_szStatsRegRoot);

    // Initialize composite property (server.AggregateRTSPStatistics)
    if (m_pRegistry->AddComp(m_szStatsRegRoot) > 0)
    {
        DPRINTF(D_INFO,("RTSP-Agg-Stat: Initializing registry entry %s\n",
                         m_szStatsRegRoot)); 
    }

    for (UINT32 i = 0; i < NUM_AGGREGATE_RTSP_STATS; ++i)
    {
        ulMethodId = zm_StatInfoList[i].ulMethodId;
        ulMethodType = zm_StatInfoList[i].ulMethodType;
        szMethodPropName = zm_StatInfoList[i].szRegPropName;

        if (!szMethodPropName)
        {
            continue;
        }
        
        pEC = m_EventCounterVec[ulMethodType][ulMethodId];
        UINT32 ulCounterId = 0; 
        UINT32 ulMethodPropLen = strlen(szMethodPropName);

        // Initialize composite property for current method
        NEW_FAST_TEMP_STR(szRegEntry, 512, ulRegRootLen + ulMethodPropLen + 1);
        sprintf(szRegEntry, "%s%s",  m_szStatsRegRoot, szMethodPropName);
        
        if (m_pRegistry->AddComp(szRegEntry) > 0)
        {
            DPRINTF(D_INFO,("RTSP-Agg-Stat: Initializing registry entry %s\n",
                            szRegEntry)); 
        }
        DELETE_FAST_TEMP_STR(szRegEntry);
        
        UINT32 ulRegId = 0;
        for (; ulCounterId < NUM_COUNTERS; ++ulCounterId)
        {
            
            pEC[ulCounterId].bOverflow = FALSE;
            pEC[ulCounterId].nCounter = 0;

            szCounterPropName = zm_StatCounters[ulMethodType][ulCounterId];
            UINT32 ulPropBufLen = ulRegRootLen + ulMethodPropLen + 
                                  strlen(szCounterPropName) + 1;
            
            NEW_FAST_TEMP_STR(szRegEntry, 512, ulPropBufLen);
            snprintf(szRegEntry, ulPropBufLen, "%s%s%s", 
                 m_szStatsRegRoot, szMethodPropName, szCounterPropName);

            if ((ulRegId = m_pRegistry->AddInt(szRegEntry, 0)) == 0)
            {
                if ((ulRegId = m_pRegistry->GetId(szRegEntry)) > 0)
                {
                    if (m_pRegistry->GetTypeById(ulRegId) != PT_INTEGER)
                    {
                        ulRegId = 0;
                    }
                }
            }

            if (!ulRegId)
            {
                rc = HXR_FAIL;
                char szErrMsg[MAX_ERROR_BUFF_SIZE];
                snprintf(szErrMsg, MAX_ERROR_BUFF_SIZE,
                         "ERROR: Unable to get/create registry entry for %s\n", szRegEntry); 
                szErrMsg[MAX_ERROR_BUFF_SIZE - 1] = '\0';
                m_pErrorLog->Report(HXLOG_ERR, 0, 0, szErrMsg, NULL);
            }
            else
            {
                pEC[ulCounterId].ulRegId = ulRegId;
            }
            
            DELETE_FAST_TEMP_STR(szRegEntry);
            
            if (FAILED(rc)) break;
        }
    }

    return rc;
}


HX_RESULT
RTSPAggregateStats::GetLicense(REF(BOOL) bIsLicensed)
{
    HX_RESULT rc = HXR_OK;

    if (!m_pRegistry)
    {
        bIsLicensed = FALSE;
        rc = HXR_UNEXPECTED;
        return rc;
    }

    INT32 nLicEnabled = 0;
    if (!m_bLicenseChecked)
    {
        if (SUCCEEDED(m_pRegistry->GetIntByName(REGISTRY_RTSP_EVENTSTATS_ENABLED,
                                                nLicEnabled)))
        {
            if (nLicEnabled > 0)
            {
                m_bLicensed = TRUE;
            }
            else 
            {
                m_bLicensed = FALSE;
            }
        }
        else
        {
            m_bLicensed = FALSE;
        }
    }
    
    bIsLicensed = m_bLicensed;

    if (!m_bLicensed)
    {
        rc = HXR_NOT_LICENSED;
        if (m_pErrorLog && !m_bLicenseChecked)
        {
            m_pErrorLog->Report(HXLOG_ERR, 
                                rc, 
                                0, 
                                "This server is not licensed to use the "
                                "RTSP Aggregate Statistics feature.", NULL);
        }
    }

    m_bLicenseChecked = TRUE;

    return rc;
}


STDMETHODIMP
RTSPAggregateStats::UpdateClientRequestCount(INT32 lCount, RTSPMethod ulMethodId)
{
    return UpdateCounter(lCount, REQUEST_TOTAL_COUNT, CLIENT_REQUEST, ulMethodId);
}

STDMETHODIMP
RTSPAggregateStats::UpdateServerRequestCount(INT32 lCount, RTSPMethod ulMethodId)
{
    return UpdateCounter(lCount, REQUEST_TOTAL_COUNT, SERVER_REQUEST, ulMethodId);
}

STDMETHODIMP
RTSPAggregateStats::UpdateClientResponseCount(INT32 lCount, 
                                              RTSPMethod ulMethodId,
                                              UINT32 ulStatusCode)
{
    HX_RESULT rc = HXR_OK;

    UINT32 ulCounterId;
    BOOL bTmp = FALSE;

    rc = GetResponseCounterId(ulStatusCode, ulCounterId, bTmp);

    if (SUCCEEDED(rc))
    {
        rc = UpdateCounter(lCount, ulCounterId, SERVER_REQUEST, ulMethodId);
    }
    return rc;
}

STDMETHODIMP
RTSPAggregateStats::UpdateServerResponseCount(INT32 lCount,
                                              RTSPMethod ulMethodId,
                                              UINT32 ulStatusCode)
{
    HX_RESULT rc = HXR_OK;

    UINT32 ulCounterId;
    BOOL bIsRedirectReq = FALSE;

    rc = GetResponseCounterId(ulStatusCode, ulCounterId, bIsRedirectReq);

    if (SUCCEEDED(rc))
    {
        rc = UpdateCounter(lCount, ulCounterId, CLIENT_REQUEST, ulMethodId);

        if (bIsRedirectReq)
        {
            //XXXMAC:  Include implicit redirect requests in redirect counter?
            rc = UpdateCounter(lCount, REQUEST_TOTAL_COUNT, SERVER_REQUEST, 
                               RTSP_REDIRECT);
        }
    }

    return rc;
}

HX_RESULT
RTSPAggregateStats::GetResponseCounterId(UINT32 ulStatusCode, 
                                         UINT32& ulCounterId,
                                         BOOL& bIsRedirect)
{
    HX_RESULT rc = HXR_OK;

    if (!m_bEnabled || !m_bLicensed)
    {
        return HXR_UNEXPECTED;
    }
    
    if (ulStatusCode < 400)
    {
        ulCounterId = RESPONSE_SUCCESS_COUNT;

        if (ulStatusCode >= 300)
        {
            bIsRedirect = TRUE;
        }
    }
    else if (ulStatusCode >= 400 && ulStatusCode < 500)
    {
        ulCounterId = RESPONSE_ERR4XX_COUNT;
    }
    else if (ulStatusCode >= 500 && ulStatusCode < 600)
    {
        ulCounterId = RESPONSE_ERR5XX_COUNT;
    }
    else
    {
        rc = HXR_FAIL;
    }

    return rc;
}


HX_RESULT
RTSPAggregateStats::UpdateCounter(INT32 lCount, UINT32 ulCounterId,
                                  RTSPReqMethodType ulMethodType, 
                                  RTSPMethod ulMethodId)
{
    HX_RESULT rc = HXR_OK;
    EventCounterInfo* pEC = m_EventCounterVec[ulMethodType][ulMethodId];

    if (!m_bLicensed)
    {
        rc = HXR_NOT_LICENSED;
        return rc;
    }

    if (!m_bEnabled
    ||  !(m_ulEventMask & z_EventIdList[ulMethodId].ulEventMask))
    {
        return rc;
    }
    
    if (!pEC[ulCounterId].ulRegId)
    {
        return rc;
    }

    if (pEC[ulCounterId].bOverflow)
    {
        rc = HXR_FAIL;
        return rc;
    }

    INT32 nNewVal = HXAtomicAddRetINT32(&(pEC[ulCounterId].nCounter), lCount);

    if (SUCCEEDED(rc) && nNewVal < 0)
    { //Overflow

        HXAtomicSubINT32(&(pEC[ulCounterId].nCounter), lCount);
        pEC[ulCounterId].bOverflow = TRUE;
        char szErrMsg[128];
        snprintf(szErrMsg, 127,
                 "ERROR: INTEGER OVERFLOW in RTSP Stats registry entry (%d)\n",
                 pEC[ulCounterId].ulRegId);
        szErrMsg[127] = '\0';
        
        if (m_pErrorLog)
        {
             m_pErrorLog->Report(HXLOG_ERR, 0, 0, szErrMsg, NULL);
        }
        rc = HXR_FAIL;
    }
    else
    {
        rc = m_pRegistry->SetIntById(pEC[ulCounterId].ulRegId, nNewVal);
    }


    return rc;
}

/*
// IHXPropWatchResponse implementation

STDMETHODIMP
RTSPAggregateStats::AddedProp(const UINT32 id,
                              const HXPropType propType,
                              const UINT32 uParentID)
{
    return HXR_OK;
}

STDMETHODIMP
RTSPAggregateStats::ModifiedProp(const UINT32 ulId,
                                 const HXPropType propType,
                                 const UINT32 ulParentID)
{
    HX_RESULT rc = HXR_OK;
    if (propType != PT_INTEGER)
    {
        return rc;
    }

    INT32 lEnabled = 0;
    IHXBuffer *pPropName = NULL;

    HX_RESULT status = m_pRegistry->GetIntById(ulId, lEnabled);
    if (SUCCEEDED(status))
    {
        status = m_pRegistry->GetPropName(ulId, pPropName);
        if (SUCCEEDED(status))
        {
            const char* szPropName = (char*)pPropName->GetBuffer();
            const char* szEventName = strrchr(szPropName, '.');
            UINT32 ulEventId = 0;
            UINT32 ulEventMask = 0;

            if (szEventName)
            {
                ++szEventName;
            }
            
            status = GetEventId(szEventName, ulEventId);
            
            if ((HXR_OK == status) &&(ulEventId < RTSPStats::MAX_RTSP_METHODS))
            {
                ulEventMask = RTSPStats::m_EventIdList[ulEventId].ulEventMask;
                if (lEnabled > 0)
                {
                    m_ulEventMask |= ulEventMask;
                }
                else
                {
                    m_ulEventMask &= ~ulEventMask;
                }
            }
        }
    }
    HX_RELEASE(pPropName);
    
    return rc;
}

STDMETHODIMP
RTSPAggregateStats::DeletedProp(const UINT32 ulId,
                                const UINT32 ulParentID)
{
    HX_RESULT rc = HXR_OK;

    INT32 lEnabled = 0;
    IHXBuffer *pPropName = NULL;

    HX_RESULT status = m_pRegistry->GetPropName(ulId, pPropName);
    if (SUCCEEDED(status))
    {
        const char* szPropName = (char*)pPropName->GetBuffer();
        const char* szEventName = strrchr(szPropName, '.');
        UINT32 ulEventId = 0;
        if (szEventName)
        {
            ++szEventName;
        }

        status = GetEventId(szEventName, ulEventId);
            
        if ((HXR_OK == status) &&(ulEventId < RTSPStats::MAX_RTSP_METHODS))
        {
            UINT32 ulEventMask = RTSPStats::m_EventIdList[ulEventId].ulEventMask;
            m_ulEventMask &= ~ulEventMask;
            m_EventMaskRegIdVec[ulEventId] = 0;
        }
    }
    HX_RELEASE(pPropName);
    m_pPropWatch->ClearWatchById(ulId);

    return HXR_OK;
}

HX_RESULT
RTSPAggregateStats::ClearRegistryWatches()
{
    HX_RESULT rc = HXR_OK;
    if (!m_pPropWatch)
    {
        return rc;
    }

    UINT32 ulWatchPropId = 0;
    UINT32 ulEventId = 0;
    for ( ; ulEventId < RTSPStats::MAX_RTSP_METHODS; ++ulEventId)
    {
        ulWatchPropId = m_EventMaskRegIdVec[ulEventId];

        if (ulWatchPropId > 0)
        {
            m_pPropWatch->ClearWatchById(ulWatchPropId);
            m_EventMaskRegIdVec[ulEventId] = 0;
        }
    }
    return rc;
}
*/

/*****************************************************************************
 * 
 * RTSPStats Implementation
 *
 *****************************************************************************/

RTSPStats::RTSPStats()
: m_ulRefCount(0)
, m_ulMaxEvents(0)
, m_pRegistry(NULL)
, m_pClassFactory(NULL)
, m_pScheduler(NULL)
, m_pErrorLog(NULL)
, m_pEventsMgr(NULL)
, m_pClientStatsManager(NULL)
, m_pClientStats(NULL)
{
}


RTSPStats::~RTSPStats()
{
    HX_ASSERT(m_OutputQueue.IsEmpty());
    HX_ASSERT(m_EventsPendingResponse.IsEmpty());

    if (!m_OutputQueue.IsEmpty() || !m_EventsPendingResponse.IsEmpty())
    {
        DumpEvents(TRUE);
    }    

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pRegistry);

    HX_RELEASE(m_pScheduler); 
    HX_RELEASE(m_pErrorLog);
    HX_RELEASE(m_pEventsMgr);
    HX_RELEASE(m_pClientStatsManager);
    HX_RELEASE(m_pClientStats);
}


HX_RESULT
RTSPStats::Init(IUnknown* pContext)
{
    HX_RESULT hr = HXR_OK;

    IfFailGo(hr = pContext->QueryInterface(IID_IHXCommonClassFactory, 
                                           (void**)&m_pClassFactory));

    IfFailGo(hr = pContext->QueryInterface(IID_IHXRegistry2, 
                                           (void**)&m_pRegistry));

    IfFailGo(hr = pContext->QueryInterface(IID_IHXScheduler, 
                                           (void**)&m_pScheduler));
    
    IfFailGo(hr = pContext->QueryInterface(IID_IHXErrorMessages, 
                                           (void**)&m_pErrorLog));

    IfFailGo(hr = pContext->QueryInterface(IID_IHXRTSPEventsManager, 
                                           (void**)&m_pEventsMgr));

    IfFailGo(hr = ReadConfig());

    if (!zm_bEnabled)
    {
        zm_ulEventMask = 0;
        zm_ulMaxEvents = 0;
        hr = HXR_FAIL;
        goto cleanup;
    }

    IfFailGo(hr = CheckLicense());

    IfFailGo(hr = pContext->QueryInterface(IID_IHXClientStatsManager,
                                           (void**)&m_pClientStatsManager));

cleanup:

    zm_bConfigRead = TRUE;

    // RTSPServerProtocol releases this object on failure, 
    // so all cleanup happens in the dtor.

    return hr;
}


HX_RESULT
RTSPStats::CheckLicense()
{
    INT32 nLicense;

    if (zm_bLicenseChecked)
    {
        // If we've already checked for license and we're not licensed,
        // then there should be a check in the protocol code so we don't
        // have to call this function.
        HX_ASSERT(zm_bLicensed);

        // ...but just in case...
        return zm_bLicensed ? HXR_OK : HXR_NOT_LICENSED;
    }

    HX_ASSERT(zm_bEnabled);
    HX_ASSERT(m_pRegistry);

    if (SUCCEEDED(m_pRegistry->GetIntByName(REGISTRY_RTSP_EVENTSTATS_ENABLED,
                                            nLicense)))
    {
        if (nLicense > 0)
        {
            zm_bLicensed = TRUE;
        }
        else 
        {
            zm_bLicensed = FALSE;
        }
    }
    else
    {
        zm_bLicensed = FALSE;
    }

    if (!zm_bLicensed)
    {
        ReportError(HXLOG_ERR, 
                    HXR_NOT_LICENSED,
                    "This server is not licensed to use the "
                    "RTSP Events feature.");
    }

    zm_bLicenseChecked = TRUE;

    return zm_bLicensed ? HXR_OK : HXR_NOT_LICENSED;
}


HX_RESULT
RTSPStats::ReadConfig()
{
    HX_RESULT hr = HXR_OK;
    INT32 nTemp = 0;
    IHXValues* pEventList = NULL;
    const char* szPropName = NULL;
    const char* szEventName = NULL;
    UINT32 ulPropId = 0;
    INT32 lEnabled = 0;
    UINT32 ulEventId = RTSP_UNKNOWN;
    UINT32 ulEventBitMask = 0;
    
    if (zm_bConfigRead)
    {
        return HXR_OK;
    }

    HX_ASSERT(m_pRegistry);

    if (SUCCEEDED(m_pRegistry->GetIntByName(z_szConfigEventsEnabled, nTemp)))
    {
        zm_bEnabled = nTemp == 1 ? TRUE : FALSE;
    }
    else
    {
        zm_bEnabled = FALSE;
    }

    if (!zm_bEnabled)
    {
        return HXR_OK;
    }

    if (SUCCEEDED(m_pRegistry->GetIntByName(z_szConfigEventsMaxEvents, nTemp)))
    {
        if (nTemp > 10)        
        {
            ReportError(HXLOG_WARNING,
                        HXR_OK,
                        "RTSP_EVENTS: (Warning) Cannot configure RTSPEvents to log more than "
                        "%u events/line. RTSPEvents will log %u events/line.",
                        g_ulMaxPossibleEventsPerLine,
                        g_ulMaxPossibleEventsPerLine);
            nTemp = g_ulMaxPossibleEventsPerLine;
        }

        zm_ulMaxEvents = (UINT32)nTemp;

        if (nTemp < 1)
        {
            zm_bEnabled = FALSE;
            zm_ulEventMask = 0;
            return HXR_FAIL;
        }
    }
    else
    {
        zm_ulMaxEvents = g_ulDefaultEventsPerLine;
    }


    if(SUCCEEDED(m_pRegistry->GetPropListByName(z_szConfigEventsEventMaskList, pEventList))
    && SUCCEEDED(pEventList->GetFirstPropertyULONG32(szPropName, ulPropId)))
    {
        do
        {
            ulEventId = RTSP_UNKNOWN;
            szEventName = strrchr(szPropName, '.');

            if (!szEventName) 
            {
                continue;
            }
            
            ++szEventName;
            HX_RESULT status = m_pRegistry->GetIntById(ulPropId, lEnabled);

            if (FAILED(status)) 
            {
                continue;
            }
           
            status = GetEventId(szEventName, ulEventId);
            
            if ((HXR_OK == status) && (ulEventId < RTSPStats::MAX_RTSP_METHODS))
            {
                if ((lEnabled > 0) && ulEventId != RTSP_UNKNOWN)
                {
                    ulEventBitMask = z_EventIdList[ulEventId].ulEventMask;
                    zm_ulEventMask |= ulEventBitMask;
                }
            }

        } while(SUCCEEDED(pEventList->GetNextPropertyULONG32(szPropName, ulPropId)));

        DPRINTF(D_INFO, ("RTSP_EVENTS: Event mask -> (0x%x)\n", zm_ulEventMask));        
    }
    else
    {
        zm_bEnabled = FALSE;
        zm_ulEventMask = 0;
        ReportError(HXLOG_ERR,
                    HXR_OK,
                    "RTSP_EVENTS: Config error - could not find event reporting flags.");
        
        hr = HXR_FAIL;        
    }


    HX_RELEASE(pEventList);

    zm_bConfigRead = TRUE;

    return hr;    
}

HX_RESULT
RTSPStats::SetClientStatsObj(IHXClientStats* pClientStats)
{
    if (!zm_bLicensed)
    {
        return HXR_OK;
    }

    if (m_pClientStats)
    {
        HX_ASSERT(0);
        return HXR_UNEXPECTED;
    }

    if (!pClientStats)
    {
        return HXR_UNEXPECTED;
    }

    m_pClientStats = pClientStats;
    m_pClientStats->AddRef();

    return HXR_OK;
}


BOOL
RTSPStats::FeatureConfigured()
{
    return (zm_bConfigRead && zm_bLicenseChecked) ? TRUE : FALSE;
}


BOOL
RTSPStats::FeatureEnabled()
{
    return zm_bEnabled;
}


BOOL
RTSPStats::FeatureLicensed()
{
    return zm_bLicensed;
}


HX_RESULT
RTSPStats::UpdateRequestStats(RTSPMethod ulEventType, 
                              UINT32 ulCSeq, 
                              BOOL bServerOrigin)
{
    HX_RESULT rc = HXR_OK;

    if (!zm_bLicensed)
    {
        HX_ASSERT(!"License check should occur earlier!");
        return HXR_NOT_LICENSED;
    }

    if (!(zm_ulEventMask &
        z_EventIdList[ulEventType].ulEventMask))
    {
        DPRINTF(D_INFO, ("RTSP_EVENTS: IGNORING request event (Type: %d) (CSeq: %u)\n",
                         ulEventType, ulCSeq));
        return rc;
    }

    DPRINTF(D_INFO, ("RTSP_EVENTS: Request event (Type: %d) (CSeq: %u)\n",
                     ulEventType, ulCSeq));
    
    HX_ASSERT((UINT32)ulEventType < MAX_RTSP_METHODS);

    HXTime currTime;
    gettimeofday(&currTime, NULL);
    UINT16 usReqSrc = (bServerOrigin)? (SERVER_REQUEST) : (CLIENT_REQUEST);

    RTSPEventInfo* pInfo = new RTSPEventInfo;
    pInfo->usType = (UINT16)ulEventType;
    pInfo->usSrc = usReqSrc;
    pInfo->ulTs = currTime.tv_sec;
    pInfo->ulCSeq = ulCSeq;
    pInfo->ulStatus = 0;

    m_EventsPendingResponse.SetAt(ulCSeq, (void*)pInfo);

    return rc;
}


HX_RESULT
RTSPStats::UpdateResponseStats(UINT32 ulStatusCode,
                               UINT32 ulCSeq, 
                               BOOL bServerOrigin)
                                 
{
    HX_RESULT rc = HXR_OK;

    if (!zm_bLicensed)
    {
        HX_ASSERT(!"License check should occur earlier!");
        return HXR_NOT_LICENSED;
    }

    DPRINTF(D_INFO, ("RTSP_EVENTS: Response event (CSeq: %u)\n", 
                     ulCSeq));

    BOOL bRequestFound = FALSE;
    RTSPEventInfo* pInfo = NULL;

    bRequestFound = m_EventsPendingResponse.Lookup(ulCSeq, (void*&)pInfo);

    if (bRequestFound && pInfo)
    {
        HX_ASSERT(pInfo->ulCSeq == ulCSeq);
        HX_ASSERT(pInfo->ulStatus == 0);


        pInfo->ulStatus = ulStatusCode;

        m_EventsPendingResponse.Remove(ulCSeq);
        m_OutputQueue.AddTail((void*)pInfo);

        // Events on this list are output immediately.
        HX_ASSERT(ulStatusCode > 0);

        DumpEvents();
    }
    else
    {
        rc = HXR_FAIL;
    }

    return rc;
}


HX_RESULT
RTSPStats::DumpEvents(BOOL bDumpPendingEvents)
{
    HX_RESULT hr = HXR_OK;
    IHXBuffer* pEventBuff = NULL;

    if (!zm_bLicensed)
    {
        HX_ASSERT(!"License check should occur earlier!");
        return HXR_NOT_LICENSED;
    }

    HX_ASSERT(zm_bEnabled || m_OutputQueue.GetCount() == 0);

    // Throughout the life of this object there should only be one output queued, 
    // since we output right away.

    HX_ASSERT(m_OutputQueue.GetCount() <= 1);

    // This call CLEARS the eventinfolist as well!
    IfFailGo(hr = BuildEventString(&m_OutputQueue, pEventBuff));
    IfFailGo(hr = ReportEvents(pEventBuff, NULL, NULL)); 

    HX_RELEASE(pEventBuff);

    // For final event dump (session is ending) we need to dump all events,
    // even those waiting for a response. (This should only happen on the
    // proxy case.)
    if (bDumpPendingEvents)
    {
        CHXMapLongToObj::Iterator i;
        for (i = m_EventsPendingResponse.Begin(); i != m_EventsPendingResponse.End(); ++i)
        {
            RTSPEventInfo* pInfo = (RTSPEventInfo*)*i;
            m_EventsPendingResponse.RemoveKey(i.get_key());
            m_OutputQueue.AddTail((void*)pInfo);

            IfFailGo(hr = BuildEventString(&m_OutputQueue, pEventBuff));
            IfFailGo(hr = ReportEvents(pEventBuff, NULL, NULL));
            HX_RELEASE(pEventBuff);
        }

        HX_ASSERT(m_EventsPendingResponse.IsEmpty());
    }
    
    HX_ASSERT(m_OutputQueue.IsEmpty());

cleanup:

    HX_RELEASE(pEventBuff);
    return hr;
}


HX_RESULT
RTSPStats::BuildEventString(CHXSimpleList* pEventInfoList, IHXBuffer*& pEventBuff)
{
    // Expected EventInfo format string: 
    // [<10 byte timestamp>;<4 byte event type>;<4 byte status code>] <space>

    // Make sure we aren't blowing anything away.
    HX_ASSERT(!pEventBuff);
    
    UINT32 ulNumEvents = pEventInfoList->GetCount();
    if (ulNumEvents == 0)
    {
        return HXR_OK;
    }

    UINT32 ulMaxBuffSize = (ulNumEvents * RTSPStats::MAX_EVENT_INFO_BUFFSIZE) + 1; 
    
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pEventBuff);
    HX_ASSERT(pEventBuff);


    pEventBuff->SetSize(ulMaxBuffSize);
    INT32 nAvailBytes = ulMaxBuffSize - 1;
    UINT32 ulEventInfoSize = 0;
    UINT32 ulSaveCount = 0;
    char* szBuff = (char*)pEventBuff->GetBuffer();
    
    // This iteration is technically unnecessary since there should be only one msg in the 
    // list at any one time, but just in case...
    CHXSimpleList::Iterator itr = pEventInfoList->Begin();
    for ( ; itr != pEventInfoList->End() && nAvailBytes > 0; ++itr)
    {
        RTSPEventInfo* pInfo = (RTSPEventInfo*)(*itr);

        INT32 nWriteSize = snprintf(szBuff, 
                                    nAvailBytes, 
                                    zm_szTStatTemplate,
                                    pInfo->ulTs, 
                                    zm_EventCodeList[pInfo->usSrc].szShortSrcName,
                                    z_EventIdList[pInfo->usType].szShortEventName,
                                    pInfo->ulStatus);
        szBuff[nAvailBytes] = '\0';

        ulEventInfoSize = strlen(szBuff);
        nAvailBytes -= ((nWriteSize > 0) ? nWriteSize : nAvailBytes); 
        szBuff += ulEventInfoSize; 
        ++ulSaveCount; 
        delete pInfo; 
    }

    pEventInfoList->RemoveAll();

    szBuff[nAvailBytes] = '\0';

    if (ulSaveCount != ulNumEvents) 
    {
        ReportError(HXLOG_ERR,
                    HXR_UNEXPECTED,
                    "RTSP_EVENTS: Failed to save %u of %u events for client %u-- buffer overflow",
                    ulNumEvents - ulSaveCount, 
                    ulNumEvents, 
                    m_pClientStats->GetID());
    }

    return HXR_OK;
}


HX_RESULT
RTSPStats::ReportEvents(IHXBuffer* pEventBuff,                  
                        IHXSessionStats* pSessionStats,
                        IHXBuffer* pSessionID)
{
    if (!pEventBuff)
    {
        return HXR_OK;
    }

    m_pEventsMgr->OnRTSPEvents(m_pClientStats, pSessionStats, pEventBuff, pSessionID);

    return HXR_OK;
}


void 
RTSPStats::ReportError(const UINT8 uiSeverity, 
                       HX_RESULT result, 
                       const char* pFormat, 
                       ...)
{
    va_list args;
    va_start(args, pFormat);

// We shouldn't ever use more than this to report a one-line error.
    char pError[MAX_ERROR_BUFF_SIZE]; 

    INT16 nWritten = vsnprintf(pError, 
	                       MAX_ERROR_BUFF_SIZE - 1, 
	 		       pFormat, 
			       args);
    pError[(MAX_ERROR_BUFF_SIZE) - 2] = '\0';
    
    va_end(args);

// If we overwrote past end of buffer, NULL out end of buffer.
    if (nWritten == -1)
    {
	nWritten = (MAX_ERROR_BUFF_SIZE) - 1;
    }

    pError[nWritten] = '\0';

    HX_ASSERT(m_pErrorLog);

    m_pErrorLog->Report(uiSeverity, result, 0, pError, NULL);
}


STDMETHODIMP
RTSPStats::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(UINT32)
RTSPStats::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(UINT32)
RTSPStats::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}


RTSPSessionEventsList::RTSPSessionEventsList()
: m_pSessionStats(NULL)
, m_pRTSPEventMgr(NULL)
, m_pSessionID(NULL)
{
}


RTSPSessionEventsList::~RTSPSessionEventsList()
{
    // DumpEvents() should've been called prior to dtor.
    HX_ASSERT(m_OutputQueue.IsEmpty());
    HX_ASSERT(m_EventsPendingResponse.IsEmpty());

    if (!m_OutputQueue.IsEmpty() || !m_EventsPendingResponse.IsEmpty())
    {
        DumpEvents(TRUE);
    }    
    
    HX_RELEASE(m_pSessionStats);
    HX_RELEASE(m_pRTSPEventMgr);
    HX_RELEASE(m_pSessionID);
}

HX_RESULT
RTSPSessionEventsList::Init(RTSPStats* pRTSPEventMgr,
                            const CHXString& SessionID)
{
    if (!pRTSPEventMgr)
    {
        HX_ASSERT(!"NULL RTSPEventMgr!");
        return HXR_UNEXPECTED;
    }

    m_pRTSPEventMgr = pRTSPEventMgr;    
    m_pRTSPEventMgr->AddRef();

    HX_ASSERT(m_pRTSPEventMgr->m_pClassFactory);

    m_pRTSPEventMgr->m_pClassFactory->CreateInstance(IID_IHXBuffer, (void**)&m_pSessionID);
    HX_ASSERT(m_pSessionID);

    m_pSessionID->Set((const UCHAR*)(const char*)SessionID, SessionID.GetLength() + 1);

    return HXR_OK;
}


void
RTSPSessionEventsList::SetSessionStats(IHXSessionStats* pSessionStats)
{
    if (!RTSPStats::FeatureLicensed())
    {
        HX_ASSERT(!"License check should occur earlier!");
        return;
    }

    HX_RELEASE(m_pSessionStats);
    m_pSessionStats = pSessionStats;

    if (m_pSessionStats)
    {
        m_pSessionStats->AddRef();
    }
}


HX_RESULT
RTSPSessionEventsList::UpdateRequestStats(RTSPMethod ulEventType, 
                                          UINT32 ulCSeq, 
                                          BOOL bServerOrigin)
{
    HX_RESULT rc = HXR_OK;
    
    if (!RTSPStats::FeatureLicensed())
    {
        HX_ASSERT(!"License check should occur earlier!");
        return HXR_NOT_LICENSED;
    }

    if (!(RTSPStats::zm_ulEventMask &
        z_EventIdList[ulEventType].ulEventMask))
    {
        DPRINTF(D_INFO, ("RTSP_EVENTS: IGNORING request event (Type: %d) (CSeq: %u)\n",
                         ulEventType, ulCSeq));
        return rc;
    }

    DPRINTF(D_INFO, ("RTSP_EVENTS: Request event (Type: %d) (CSeq: %u)\n",
                     ulEventType, ulCSeq));
    
    HX_ASSERT((UINT32)ulEventType < RTSPStats::MAX_RTSP_METHODS);

    HX_ASSERT(ulEventType != RTSP_DESCRIBE);
    HX_ASSERT(ulEventType != RTSP_ANNOUNCE);

    HXTime currTime;
    gettimeofday(&currTime, NULL);
    UINT16 usReqSrc = (bServerOrigin)? (RTSPStats::SERVER_REQUEST) : (RTSPStats::CLIENT_REQUEST);

    RTSPEventInfo* pInfo = new RTSPEventInfo;
    pInfo->usType = (UINT16)ulEventType;
    pInfo->usSrc = usReqSrc;
    pInfo->ulTs = currTime.tv_sec;
    pInfo->ulCSeq = ulCSeq;
    pInfo->ulStatus = 0;

    m_EventsPendingResponse.SetAt(ulCSeq, (void*)pInfo);


    return rc;
}


HX_RESULT
RTSPSessionEventsList::UpdateResponseStats(UINT32 ulStatusCode,
                                           UINT32 ulCSeq, 
                                           BOOL bServerOrigin)
                                 
{
    HX_RESULT rc = HXR_OK;

    if (!RTSPStats::FeatureLicensed())
    {
        HX_ASSERT(!"License check should occur earlier!");
        return rc;
    } 

    DPRINTF(D_INFO, ("RTSP_EVENTS: Response event (CSeq: %u)\n",
                     ulCSeq));

    RTSPStats::RTSPEventSource usReqSrc = (bServerOrigin) ? RTSPStats::CLIENT_REQUEST : RTSPStats::SERVER_REQUEST;
    BOOL bRequestFound = FALSE;
    RTSPEventInfo* pInfo = NULL;

    bRequestFound = m_EventsPendingResponse.Lookup(ulCSeq, (void*&)pInfo);

    if (bRequestFound && pInfo)
    {
        HX_ASSERT(pInfo->ulCSeq == ulCSeq);
        HX_ASSERT(pInfo->ulStatus == 0);
        HX_ASSERT(pInfo->usSrc == usReqSrc);

        pInfo->ulStatus = ulStatusCode;

        HX_ASSERT(pInfo->usType != RTSP_DESCRIBE);
        HX_ASSERT(pInfo->usType != RTSP_ANNOUNCE);
        HX_ASSERT(ulStatusCode > 0);

        m_EventsPendingResponse.Remove(ulCSeq);
        m_OutputQueue.AddTail((void*)pInfo);

        if (m_OutputQueue.GetCount() >= (INT32)RTSPStats::zm_ulMaxEvents)            
        {
            // We check for >=, but this count should never exceed the max.
            HX_ASSERT(m_OutputQueue.GetCount() == (INT32)RTSPStats::zm_ulMaxEvents);
            DumpEvents();
        }
    }
    else
    {
        rc = HXR_FAIL;
    }

    return rc;
}


HX_RESULT 
RTSPSessionEventsList::DumpEvents(BOOL bDumpPendingEvents)
{
    HX_RESULT hr = HXR_OK;

    if (!RTSPStats::FeatureLicensed())
    {
        HX_ASSERT(!"License check should occur earlier!");
        return hr;
    }

    HX_ASSERT(RTSPStats::FeatureLicensed() || m_OutputQueue.GetCount() == 0);

    IHXBuffer* pEventBuff = NULL;

    IfFailGo(hr = m_pRTSPEventMgr->BuildEventString(&m_OutputQueue, pEventBuff));
    IfFailGo(hr = m_pRTSPEventMgr->ReportEvents(pEventBuff, m_pSessionStats, m_pSessionID)); 
    HX_RELEASE(pEventBuff);

    if (bDumpPendingEvents)
    {
        CHXMapLongToObj::Iterator i;
        for (i = m_EventsPendingResponse.Begin(); i != m_EventsPendingResponse.End(); ++i)
        {
            RTSPEventInfo* pInfo = (RTSPEventInfo*)*i;
            m_EventsPendingResponse.RemoveKey(i.get_key());
            m_OutputQueue.AddTail((void*)pInfo);

            if (m_OutputQueue.GetCount() >= (INT32)RTSPStats::zm_ulMaxEvents)            
            {
                IfFailGo(hr = m_pRTSPEventMgr->BuildEventString(&m_OutputQueue, pEventBuff));
                IfFailGo(hr = m_pRTSPEventMgr->ReportEvents(pEventBuff, NULL, m_pSessionID));
                HX_RELEASE(pEventBuff);
            }
        }

        if (!m_OutputQueue.IsEmpty())            
        {
            IfFailGo(hr = m_pRTSPEventMgr->BuildEventString(&m_OutputQueue, pEventBuff));
            IfFailGo(hr = m_pRTSPEventMgr->ReportEvents(pEventBuff, NULL, m_pSessionID));
            HX_RELEASE(pEventBuff);
        }

        HX_ASSERT(m_OutputQueue.IsEmpty());
        HX_ASSERT(m_EventsPendingResponse.IsEmpty());
    }

cleanup:

    HX_RELEASE(pEventBuff);
    return hr;
}

