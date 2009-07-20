/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bcastmgr.cpp,v 1.89 2009/05/21 15:11:01 svaidhya Exp $ 
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
/*
 *  Broadcast Management & Distribution
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hlxclib/time.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxpiids.h"
#include "hxerror.h"
#include "proc.h"
#include "server_engine.h"
#include "ihxpckts.h"
#include "source.h"
#include "hxassert.h"
#include "hxformt.h"
#include "hxstats.h"
#include "servlist.h"
#include "bcastfilter.h"
#include "bcastmgr.h"
#include "hxmap.h"
#include "simple_callback.h"
#include "dispatchq.h"
#include "hxstrutl.h"
#include "plgnhand.h"
#include "base_errmsg.h"
#include "config.h"
#include "servpckts.h"
#include "loadinfo.h"
#include "globals.h"
#include "servreg.h"
#include "asmrulep.h"
#include "server_stats.h"
#include "sdptools.h"
#include "bufnum.h"
#include "rtppkt.h"
#include "hxmime.h"
#include "livekeyframe.h"
#include "server_context.h"
#include "header_helper.h"
#include "pckunpck.h" 	/* CloneHeader() */

#include "errdbg.h"
#include "hxtick.h"
#define TIMER_FILE "C:\\broadcast_core_out.txt"
#define DISABLE_TURBO_PLAY "config.DisableTurboPlay"
#define PACKET_BUFFER_QUEUE_SIZE "config.LiveReducedStartupDelay.PacketBufferQueueSize"
#define PACKET_BUFFER_QUEUE_DURATION "config.LiveReducedStartupDelay.PacketBufferQueueDuration"
#define PACKET_BUFFER_QUEUE_TRACE "config.LiveReducedStartupDelay.PacketBufferQueueTrace"
#define ENABLE_RSD_SERVER_LOG "config.LiveReducedStartupDelay.EnableLiveRSDServerLog"
#define ENABLE_RSD_PER_PACKET_LOG "config.LiveReducedStartupDelay.EnableLiveRSDPerPacketLog"
#define RSD_MINIMUM_PREROLL "config.LiveReducedStartupDelay.MinimumPreroll"
#define RSD_EXTRA_PREROLL_PERCENTAGE "config.LiveReducedStartupDelay.ExtraPrerollPercentage"
#define RSD_DEFAULT_MINIMUM_PREROLL 1000
#define RSD_DEFAULT_EXTRA_PREROLL_PER 20
#define DEFAULT_PACKET_BUFFER_QUEUE_SIZE 8096
#define MAX_PACKET_BUFFER_QUEUE_SIZE 65536
#define DEFAULT_PACKET_BUFFER_QUEUE_DURATION 2500
#define RSD_MAX_DURATION_PACKET_BUFFER_QUEUE "config.LiveReducedStartupDelay.MaxDurationOfRSDPacketBufferQueue"
#define DEFAULT_MAX_DURATION_OF_RSD_PACKET_BUFFER_QUEUE 120
//#define RSD_LIVE_DEBUG
#define BROADCAST_GATEWAY_STATE_BUG_FIXED

#ifdef _WIN32
extern __declspec(thread) Process* g_pTLSProc;
extern __declspec(thread) int g_nTLSProcnum;
#elif ((defined _LINUX && LINUX_EPOLL_SUPPORT) || (defined _SOLARIS && defined DEV_POLL_SUPPORT)) && defined PTHREADS_SUPPORTED
extern __thread Process* g_pTLSProc;
extern __thread int g_nTLSProcnum;
#else
extern Process* g_pTLSProc;
extern int g_nTLSProcnum;
#endif // _WIN32

BroadcastManager::BroadcastManager()
{
    m_pBroadcastManagers = new CHXMapStringToOb;
}


BroadcastManager::~BroadcastManager()
{
    PANIC(("BroadcastManager should never destruct\n"));
}


void
BroadcastManager::Register(IUnknown* pPluginInstance,
                           UINT32    ulProcnum,
                           Process*  pLiveProc)
{
    IHXBroadcastFormatObject*  pBroadcastObject = 0;
    const char* pType;
    HX_RESULT ulResult;

    ulResult = pPluginInstance->QueryInterface(IID_IHXBroadcastFormatObject,
            (void**)&pBroadcastObject);

    ASSERT (HXR_OK == ulResult);

    pBroadcastObject->GetBroadcastFormatInfo(pType);

    BroadcastInfo* pInfo = new BroadcastInfo(pLiveProc);

    pInfo->m_ulProcnum = ulProcnum;
    pInfo->m_pType = pType;

    m_pBroadcastManagers->SetAt(pType, (void *)pInfo);
    pBroadcastObject->Release();
}


HX_RESULT
BroadcastManager::GetStream(const char*             pType,
                            const char*             pFilename,
                            REF(IHXPSourceControl*) pControl,
                            Process*                pStreamerProc,
                            IHXSessionStats*        pSessionStats /* =NULL */)
{
    void* pVoid = NULL;
    BroadcastInfo* pInfo = NULL;

    m_pBroadcastManagers->Lookup(pType, pVoid);

    if (pVoid == NULL)
    {
        PANIC(("How the hell did that happen??\n"));
    }
    else
    {
        pInfo = (BroadcastInfo*)pVoid;
    }

    BroadcastStreamer* pBroadcastStreamer = 
        new BroadcastStreamer(pInfo, pFilename, pStreamerProc, 
                    pSessionStats);

    HX_ASSERT(pBroadcastStreamer);

    pBroadcastStreamer->
        QueryInterface(IID_IHXPSourceControl, (void**)&pControl);

    return HXR_OK;
}

BroadcastStreamer::BroadcastStreamer(BroadcastInfo*   pInfo,
                                     const char*      pFilename,
                                     Process*         pStreamerProc,
                                     IHXSessionStats* pSessionStats)
    : m_lRefCount(0)
    , m_pSessionStats(pSessionStats)
    , m_pSinkControl(0)
    , m_pInfo(pInfo)
    , m_bGatewayReady(FALSE)
    , m_pInitPending(FALSE)
    , m_GatewayStatus(HXR_FAIL)
    , m_bSourceAborted(FALSE)
    , m_ulGatewayCheckCBHandle(0)
    , m_ulStreamDoneCBHandle(0)
    , m_pProc(pStreamerProc)
    , m_nRegEntryIndex(-1)
    , m_pbPacketsStarted(NULL)
    , m_bNeedXmit(FALSE)
    , m_pSinkPackets(NULL)
    , m_pServerPacketSink(NULL)
    , m_pPacketFilter(NULL)
    , m_bIsMDPSink(TRUE)
{
    m_pGatewayCheckCallback             = new GatewayCheckCallback;
    m_pGatewayCheckCallback->m_pBS      = this;
    m_pGatewayCheckCallback->AddRef();

    m_pStreamDoneCallback               = new StreamDoneCallback;
    m_pStreamDoneCallback->m_pBS        = this;
    m_pStreamDoneCallback->AddRef();

    void* pVoid                         = NULL;
    INT32 nNumPlayers                   = 1;
    char szBuffer[128];

    const char* pQueryParams = strchr((char*)pFilename, '?');
    if (pQueryParams)
    {
        UINT32 ulQuerySize = pQueryParams - pFilename;
        NEW_FAST_TEMP_STR(pStripped, 1024, ulQuerySize+1);

        memcpy(pStripped, pFilename, ulQuerySize);
        pStripped[ulQuerySize] = '\0';

        HXMutexLock(pInfo->m_GatewayLock, TRUE);
        // find gateway object for this file (minus the URL params)
        pInfo->m_pCurrentBroadcasts->Lookup(pStripped, pVoid);

        DELETE_FAST_TEMP_STR(pStripped);
    }
    else
    {
        HXMutexLock(pInfo->m_GatewayLock, TRUE);
        // find gateway object for this file
        pInfo->m_pCurrentBroadcasts->Lookup(pFilename, pVoid);
    }

    if (pVoid)
    {
        HXMutexUnlock(pInfo->m_GatewayLock);
        m_pGateway = (BroadcastGateway*)pVoid;
        HXAtomicIncUINT32(&m_pGateway->m_ulPlayerCount);

        // update registry's count of active players for the stream assoc. with this gateway
        m_nRegEntryIndex = m_pGateway->GetRegEntryIndex();
        if (m_nRegEntryIndex >= 0)
        {
            sprintf(szBuffer, "LiveConnections.Entry%d.NumPlayers",
                    m_nRegEntryIndex);

            // make sure prop already exists...
            if (HXR_OK == m_pProc->pc->registry->GetInt(szBuffer, &nNumPlayers, m_pProc))
            {
                nNumPlayers = m_pGateway->m_ulPlayerCount;
                m_pProc->pc->registry->SetInt(szBuffer, nNumPlayers, m_pProc);
            }
        }
    }
    else
    {
        m_pGateway = new BroadcastGateway(pInfo, pFilename, pStreamerProc);
        pInfo->m_pCurrentBroadcasts->SetAt(pFilename, (void *)m_pGateway);

        // The first gateway for this stream has been created. Setup reg 
        // entries used by Gateway before unlocking.

        m_nRegEntryIndex = m_pGateway->GetRegEntryIndex();
        if (m_nRegEntryIndex >= 0)
        {
            CreateRegEntries();
        }

        HXMutexUnlock(pInfo->m_GatewayLock);
    }

    // update count of client requests

    if (m_pSessionStats)
    {
        m_pSessionStats->AddRef();

        IHXCheckRetainEntityForSetup* pCheckRetainEntityForSetup = NULL;
        m_pSessionStats->QueryInterface(IID_IHXCheckRetainEntityForSetup, (void**)&pCheckRetainEntityForSetup);
        HX_ASSERT(pCheckRetainEntityForSetup);

        if (m_nRegEntryIndex >= 0 && pCheckRetainEntityForSetup->GetUpdateRegistryForLive())
        {
            INT32 val;
            snprintf(szBuffer, 128, "LiveConnections.Entry%d.ClientRequests", 
                    m_nRegEntryIndex);
            m_pProc->pc->registry->ModifyInt(szBuffer, 1, &val, m_pProc);
        }
        HX_RELEASE(pCheckRetainEntityForSetup);
    }

    m_pGateway->AddBroadcastStreamer(m_pProc, this);

    GatewayCheck();
}

BroadcastStreamer::~BroadcastStreamer()
{
}

// IUnknown methods
HX_RESULT
BroadcastStreamer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();                                                    
        *ppvObj = (IUnknown*)(IHXPSourceControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSourceControl))
    {
        AddRef();
        *ppvObj = (IHXPSourceControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXASMSource))
    {
        AddRef();
        *ppvObj = (IHXASMSource*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXLivePacketBufferProvider))
    {
        AddRef();
        *ppvObj = (IHXLivePacketBufferProvider*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSourceLivePackets))
    {
        AddRef();
        *ppvObj = (IHXPSourceLivePackets*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSourceLiveResync))
    {
        AddRef();
        *ppvObj = (IHXPSourceLiveResync*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketSource))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSource*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXSyncHeaderSource))
    {
        AddRef();
        *ppvObj = (IHXSyncHeaderSource*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

ULONG32
BroadcastStreamer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32
BroadcastStreamer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

// IHXPSourceControl methods
HX_RESULT
BroadcastStreamer::Init(IHXPSinkControl* pSink)
{
    pSink->AddRef();
    m_pSinkControl = pSink;

    if (m_bGatewayReady)
    {
        m_pSinkControl->InitDone(m_GatewayStatus);
    }
    else
    {
        m_pInitPending = TRUE;
    }

    return HXR_OK;
}

HX_RESULT
BroadcastStreamer::Done()
{
    HX_RELEASE(m_pSinkPackets);

    if (m_pServerPacketSink)
    {
        m_pServerPacketSink->SourceDone();
        HX_RELEASE(m_pServerPacketSink);
    }

    HX_DELETE(m_pPacketFilter);

    if (m_ulGatewayCheckCBHandle)
    {
        m_pProc->pc->engine->schedule.remove(m_ulGatewayCheckCBHandle);
        m_ulGatewayCheckCBHandle = 0;
    }

    if (m_ulStreamDoneCBHandle)
    {
        m_pProc->pc->engine->schedule.remove(m_ulStreamDoneCBHandle);
        m_ulStreamDoneCBHandle = 0;
    }

    HX_RELEASE(m_pSinkControl);
    HX_RELEASE(m_pGatewayCheckCallback);
    HX_RELEASE(m_pStreamDoneCallback);
    HX_VECTOR_DELETE(m_pbPacketsStarted);

    HXAtomicDecUINT32(&m_pGateway->m_ulPlayerCount);
    m_pGateway->RemoveBroadcastStreamer(m_pProc, this);
    m_bSourceAborted = TRUE;

    // update registry's count of active players for the stream assoc. with this gateway
    if (m_nRegEntryIndex >= 0)
    {
        char szBuffer[128];
        sprintf(szBuffer, "LiveConnections.Entry%d.NumPlayers", m_nRegEntryIndex);

        // get existing count and decrement
        INT32 nNumPlayers = m_pGateway->m_ulPlayerCount;
        m_pProc->pc->registry->SetInt(szBuffer, nNumPlayers, m_pProc);

        // update client count
        if (m_pSessionStats)
        {
            IHXCheckRetainEntityForSetup* pCheckRetainEntityForSetup = NULL;
            m_pSessionStats->QueryInterface(IID_IHXCheckRetainEntityForSetup, (void**)&pCheckRetainEntityForSetup);
            HX_ASSERT(pCheckRetainEntityForSetup);
            if(pCheckRetainEntityForSetup->GetUpdateRegistryForLive())
            {
                UpdateRegClientsLeaving();
            }
            HX_RELEASE(pCheckRetainEntityForSetup);
        }
    }
    HX_RELEASE(m_pSessionStats);

    return HXR_OK;
}

HX_RESULT
BroadcastStreamer::GetFileHeader(IHXPSinkControl* pSink)
{
    HX_ASSERT(pSink == m_pSinkControl);

    if (m_pGateway->m_pFileHeader)
    {
        IHXValues* pHeader = CloneHeader(m_pGateway->GetFileHeader(),
                                         g_pTLSProc->pc->server_context);
        m_pSinkControl->FileHeaderReady(HXR_OK, pHeader);
        HX_RELEASE(pHeader);
    }
    else
    {
        HX_ASSERT(0);
        m_pSinkControl->FileHeaderReady(HXR_FAIL, NULL);
        return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT
BroadcastStreamer::GetStreamHeader(IHXPSinkControl* pSink,
                    UINT16 unStreamNumber)
{
    HX_ASSERT(pSink == m_pSinkControl);

    if (m_pGateway->m_pStreamHeaders && 
        m_pGateway->m_pStreamHeaders[unStreamNumber])
    {
        IHXValues* pHeader = CloneHeader(m_pGateway->GetStreamHeaders(unStreamNumber),
                                         g_pTLSProc->pc->server_context);
        m_pSinkControl->StreamHeaderReady(HXR_OK, pHeader);
        HX_RELEASE(pHeader);
    }
    else
    {
        HX_ASSERT(0);
        m_pSinkControl->StreamHeaderReady(HXR_FAIL, NULL);
        return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT
BroadcastStreamer::Seek(UINT32          ulSeekTime)
{
    return HXR_FAIL;
}

BOOL
BroadcastStreamer::IsLive()
{
    return TRUE;
}

HX_RESULT
BroadcastStreamer::SetLatencyParams(UINT32 ulLatency,
                    BOOL bStartAtTail,
                    BOOL bStartAtHead)
{
    /* XXXDWL TODO */
    return HXR_OK;
}

// IHXASMSource methods
STDMETHODIMP
BroadcastStreamer::Subscribe(UINT16 uStreamNumber, UINT16 uRuleNumber) 
{
    HX_ASSERT(m_pGateway != NULL);
    HX_ASSERT(m_pGateway->m_ppRuleData != NULL);
    HX_ASSERT(uStreamNumber <= m_pGateway->m_ulStreamCount);     

    // Send updates to ASM Source only when this is the first 
    // subscribe for this rule or there is no rulebook
    HXBOOL bASMupdate = FALSE;    
    HXMutexLock(m_pGateway->m_RuleDataLock, TRUE);

    if (m_pGateway->m_ppRuleData[uStreamNumber])
    {        
        m_pGateway->m_ppRuleData[uStreamNumber]->m_pActiveRules[uRuleNumber]++;
        if (m_pGateway->m_ppRuleData[uStreamNumber]->m_pActiveRules[uRuleNumber] == 1)
        {
            bASMupdate = TRUE;
        }
    }
    else
    {           
        bASMupdate = TRUE;        
    }

    if (bASMupdate)
    {
        m_pGateway->m_bIsSubscribed = TRUE;
    }

    HXMutexUnlock(m_pGateway->m_RuleDataLock);
    
    if (bASMupdate)
    {
        ASMUpdateCallback* pCB = new ASMUpdateCallback;
        if (m_pGateway->m_pASMSource)
        {
            pCB->m_pASMSource = m_pGateway->m_pASMSource;
            pCB->m_pASMSource->AddRef();
        }
        else
        {
            pCB->m_pASMSource = NULL;
        }

        pCB->m_cAction         = ASM_SUBSCRIBE;
        pCB->m_unRuleNumber    = uRuleNumber;
        pCB->m_unStreamNumber  = uStreamNumber; 

        /* pass control from the Streamer to the Live Process */
        m_pProc->pc->dispatchq->send(m_pProc, pCB, 
                m_pGateway->m_pInfo->m_ulProcnum);
    }
    else
    {
        HXMutexUnlock(m_pGateway->m_RuleDataLock);
    }

    if (m_bIsMDPSink && m_pPacketFilter)
    {
        //MDP handling
        m_pPacketFilter->HandleSubscribe(uStreamNumber, uRuleNumber);
    }

    return HXR_OK;
}
    
STDMETHODIMP
BroadcastStreamer::Unsubscribe(UINT16 uStreamNumber, UINT16 uRuleNumber) 
{
    HX_ASSERT(m_pGateway != NULL);
    HX_ASSERT(m_pGateway->m_ppRuleData != NULL);
    HX_ASSERT(uStreamNumber <= m_pGateway->m_ulStreamCount);     

    // Send updates to ASM Source only when all the active rules are 
    // unsubscribed or there is no rulebook
    HXBOOL bASMupdate = FALSE;

    HXMutexLock(m_pGateway->m_RuleDataLock, TRUE);

    if (m_pGateway->m_ppRuleData[uStreamNumber])
    {
        m_pGateway->m_ppRuleData[uStreamNumber]->m_pActiveRules[uRuleNumber]--;
        HX_ASSERT(m_pGateway->m_ppRuleData[uStreamNumber]->m_pActiveRules[uRuleNumber] < 0xffff);
        if (m_pGateway->m_ppRuleData[uStreamNumber]->m_pActiveRules[uRuleNumber] == 0)
        {
            bASMupdate = TRUE;
        }
    }
    else
    {
        bASMupdate = TRUE;
    }

    // Mark the feed as unsubscribed when all the active rules for the feed
    // are unsubscribed
    if (bASMupdate && m_pGateway->m_ppRuleData)
    {
        m_pGateway->m_bIsSubscribed = FALSE;
        for (UINT16 i = 0; i < m_pGateway->m_ulStreamCount && m_pGateway->m_ppRuleData[i]; i++)
        {
            for (UINT j = 0; j < m_pGateway->m_ppRuleData[i]->m_unNumRules; j++)
            {
                if (m_pGateway->m_ppRuleData[i]->m_pActiveRules[j] != 0)
                {
                    m_pGateway->m_bIsSubscribed = TRUE;
                    break;
                }
            }

            if (m_pGateway->m_bIsSubscribed)
            {
                break;
            }
        }
    }

    HXMutexUnlock(m_pGateway->m_RuleDataLock);    
    
    if (bASMupdate)
    {
        ASMUpdateCallback* pCB = new ASMUpdateCallback;

        if (m_pGateway->m_pASMSource)
        {
            pCB->m_pASMSource = m_pGateway->m_pASMSource;
            pCB->m_pASMSource->AddRef();
        }
        else
        {
            pCB->m_pASMSource = NULL;
        }

        pCB->m_cAction         = ASM_UNSUBSCRIBE;
        pCB->m_unRuleNumber    = uRuleNumber;
        pCB->m_unStreamNumber  = uStreamNumber; 

        /* pass control from the Streamer to the Live Process */
        m_pProc->pc->dispatchq->send(m_pProc, pCB, m_pGateway->m_pInfo->m_ulProcnum);
    }
    else
    {
        HXMutexUnlock(m_pGateway->m_RuleDataLock);
    }

    if (m_bIsMDPSink && m_pPacketFilter)
    {
        //MDP handling
        m_pPacketFilter->HandleUnsubscribe(uStreamNumber, uRuleNumber);
    }

    return HXR_OK;
}

 // IHXLivePacketBufferProvider methods
STDMETHODIMP
BroadcastStreamer::GetPacketBufferQueue(IHXLivePacketBufferQueue*& pQueue)
{
    pQueue = m_pGateway->GetPacketBufferQueue();
    if (pQueue)
    {
        return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
BroadcastStreamer::GetLongPacketBufferQueue(IHXLivePacketBufferQueue*& pQueue)
{
    pQueue = m_pGateway->GetLongPacketBufferQueue();
    if (pQueue)
    {
        return HXR_OK;
    }
    return HXR_FAIL;
}

// IHXPSourceLivePackets methods
STDMETHODIMP
BroadcastStreamer::Init(IHXPSinkPackets*       pSinkPackets)
{
    HX_RELEASE(m_pSinkPackets);
    pSinkPackets->AddRef();
    m_pSinkPackets = pSinkPackets;

    m_bIsMDPSink = FALSE;

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer::StartPackets(UINT16 unStreamNumber)
{
    HX_ASSERT(unStreamNumber <= m_pGateway->m_ulStreamCount);

    if (m_pbPacketsStarted)
    {
        m_pbPacketsStarted[unStreamNumber] = TRUE;
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer::StopPackets(UINT16 unStreamNumber)
{
    HX_ASSERT(unStreamNumber <= m_pGateway->m_ulStreamCount);

    if (m_pbPacketsStarted)
    {
        m_pbPacketsStarted[unStreamNumber] = FALSE;
    }

    return HXR_OK;
}

// IHXPSourceLiveResync methods
STDMETHODIMP
BroadcastStreamer::Resync()
{
    return HXR_OK;
}

// IHXServerPacketSource methods
STDMETHODIMP
BroadcastStreamer::SetSink (IHXServerPacketSink* pSink)
{
    HX_ASSERT(pSink);

    if (!pSink)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pServerPacketSink);

    m_pServerPacketSink = pSink;
    m_pServerPacketSink->AddRef();

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer::StartPackets ()
{
    HX_ASSERT(m_pbPacketsStarted);
    if (m_pbPacketsStarted)
    {
        for (UINT16 i = 0; i < m_pGateway->m_ulStreamCount; i++)
        {
            m_pbPacketsStarted[i] = TRUE;
        }
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer::GetPacket ()
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

STDMETHODIMP
BroadcastStreamer::SinkBlockCleared (UINT32 ulStream)
{
    HX_ASSERT(m_pPacketFilter);
    HX_ASSERT(m_pServerPacketSink);

    if (m_pPacketFilter && m_pServerPacketSink)
    {
        m_pPacketFilter->SetStreamBlocked(ulStream, FALSE);
        PushPacketsFromQueue();
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer::EnableTCPMode ()
{
    return HXR_OK;
}

// IHXSyncHeaderSource methods
STDMETHODIMP
BroadcastStreamer::GetFileHeader(REF(IHXValues*)pHeader)
{
    if (m_pGateway->m_pFileHeader)
    {
        pHeader = CloneHeader(m_pGateway->GetFileHeader(),
                              g_pTLSProc->pc->server_context);
        return HXR_OK;
    }

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}
STDMETHODIMP
BroadcastStreamer::GetStreamHeader(UINT32 ulStreamNo, REF(IHXValues*)pHeader)
{
     if (m_pGateway->m_pStreamHeaders && m_pGateway->m_pStreamHeaders[ulStreamNo])
    {
        pHeader = CloneHeader(m_pGateway->GetStreamHeaders(ulStreamNo),
                              g_pTLSProc->pc->server_context);
        return HXR_OK;
    }

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

// Helper functions
STDMETHODIMP        
BroadcastStreamer::GatewayCheckCallback::Func()
{
    m_pBS->GatewayCheck();
    return HXR_OK;
}

void BroadcastStreamer::GatewayCheck()
{
    m_ulGatewayCheckCBHandle = 0;

    HXMutexLock(m_pGateway->m_StateLock, TRUE);
    if (m_pGateway->m_State == BroadcastGateway::INIT)
    {
        HXMutexUnlock(m_pGateway->m_StateLock);
        m_ulGatewayCheckCBHandle = 
            m_pProc->pc->engine->schedule.enter(
                    m_pProc->pc->engine->now + Timeval(0.50),
                    m_pGatewayCheckCallback);
    }
    else if (m_pGateway->m_State == BroadcastGateway::STREAMING)
    {
        HXMutexUnlock(m_pGateway->m_StateLock);
        m_GatewayStatus = HXR_OK;
        m_pbPacketsStarted = new UINT8[m_pGateway->m_ulStreamCount];
        memset(m_pbPacketsStarted, 0, sizeof(UINT8) *
                m_pGateway->m_ulStreamCount);

        if (m_pGateway->m_bUseLatencyRequirements)
        {

        }

        if (NULL == m_pPacketFilter)
        {
            m_pPacketFilter = new BroadcastPacketFilter();
            m_pPacketFilter->Init((IHXSyncHeaderSource*)this, m_pProc);
        }

        goto ContinueInit;
    }
    else
    {
        HXMutexUnlock(m_pGateway->m_StateLock);
        m_GatewayStatus = HXR_FAIL;
        goto ContinueInit;
    }

    return;

ContinueInit:
    m_bGatewayReady = TRUE;

    if (m_pInitPending)
        m_pSinkControl->InitDone(m_GatewayStatus);
}

STDMETHODIMP        
BroadcastStreamer::StreamDoneCallback::Func()
{
    m_pBS->m_ulStreamDoneCBHandle = 0;

    UINT32 ulStreamCount = m_pBS->m_pGateway->m_ulStreamCount;
    for (UINT16 i = 0; i < ulStreamCount; i++)
    {
        m_pBS->m_pSinkControl->StreamDone(i);
    }

    return HXR_OK;
}

HX_RESULT 
BroadcastStreamer::SendPacket(IHXPacket* pPacket)
{
    // PPM
    if (m_pSinkPackets)
    {
        return m_pSinkPackets->PacketReady(HXR_OK, pPacket);
    }
    
    // MDP
    if (m_pPacketFilter && m_pServerPacketSink)
    {
        m_pPacketFilter->OnPacket(pPacket);
        PushPacketsFromQueue();
    }

    return HXR_OK;
}

// Pushes packets to packet sink until there are no more packets 
// in the queue or if the next packet's stream is blocked.
HX_RESULT
BroadcastStreamer::PushPacketsFromQueue()
{
    ServerPacket* pServerPacket = NULL;
    HX_RESULT     hRes    = HXR_OK;

    HX_ASSERT(m_pServerPacketSink);
    HX_ASSERT(m_pPacketFilter);

    //Checking for m_pPacketFilter in every iteration because, When Switching(SSPL/FCS) from one stream to another
    //the m_pServerPacketSink->PacketReady calls BroadcastStreamer::Done (down the stack) which in turn destroys
    //m_pPacketFilter
    while (m_pPacketFilter && HXR_OK == m_pPacketFilter->GetNextPacket(pServerPacket) && pServerPacket)
    {
        if (HXR_BLOCKED == m_pServerPacketSink->PacketReady(pServerPacket))
        {
            m_pPacketFilter->SetStreamBlocked(pServerPacket->GetStreamNumber(), TRUE);
        }

        HX_RELEASE(pServerPacket);
    }

    return hRes;
}

HX_RESULT
BroadcastStreamer::CreateRegEntries()
{
    HX_ASSERT(m_pGateway);

    if (m_nRegEntryIndex < 0) return HXR_FAIL;

    char szBuffer[128];
    INT32 nNumPlayers = m_pGateway->m_ulPlayerCount;

    snprintf(szBuffer, 128, "LiveConnections.Entry%d.NumPlayers", m_nRegEntryIndex);
    m_pProc->pc->registry->AddInt(szBuffer, nNumPlayers, m_pProc);

    snprintf(szBuffer, 128, "LiveConnections.Entry%d.ClientRequests", 
            m_nRegEntryIndex);
    m_pProc->pc->registry->AddInt(szBuffer, 0, m_pProc);

    snprintf(szBuffer, 128, "LiveConnections.Entry%d.SuccessfulClientRequests", 
            m_nRegEntryIndex);
    m_pProc->pc->registry->AddInt(szBuffer, 0, m_pProc);

    snprintf(szBuffer, 128, "LiveConnections.Entry%d.UnsuccessfulClientRequests", 
            m_nRegEntryIndex);
    m_pProc->pc->registry->AddInt(szBuffer, 0, m_pProc);

    snprintf(szBuffer, 128, "LiveConnections.Entry%d.ClientsLeaving", 
            m_nRegEntryIndex);
    m_pProc->pc->registry->AddInt(szBuffer, 0, m_pProc);

    snprintf(szBuffer, 128, "LiveConnections.Entry%d.ClientsLeavingDueToServerError", 
            m_nRegEntryIndex);
    m_pProc->pc->registry->AddInt(szBuffer, 0, m_pProc);

    snprintf(szBuffer, 128, "LiveConnections.Entry%d.ClientsLeavingDueToClientError", 
            m_nRegEntryIndex);
    m_pProc->pc->registry->AddInt(szBuffer, 0, m_pProc);


    return HXR_OK;
}

HX_RESULT
BroadcastStreamer::UpdateRegClientsLeaving()
{
    if (m_pSessionStats)
    {
        INT32 val;
        char szBuffer[128];

        snprintf(szBuffer, 128, 
                "LiveConnections.Entry%d.ClientsLeaving", m_nRegEntryIndex);
        m_pProc->pc->registry->ModifyInt(szBuffer, 1, &val, m_pProc);

        // check the error code

        SessionStatsEndStatus ulEndStatus = m_pSessionStats->GetEndStatus();
        if (ulEndStatus == SSES_NOT_ENDED || SSES_SUCCESS(ulEndStatus))
        {

            snprintf(szBuffer, 128, 
                    "LiveConnections.Entry%d.SuccessfulClientRequests", 
                    m_nRegEntryIndex);
            m_pProc->pc->registry->ModifyInt(szBuffer, 1, &val, m_pProc);
        }
        else
        {
            snprintf(szBuffer, 128, 
                    "LiveConnections.Entry%d.UnsuccessfulClientRequests", 
                    m_nRegEntryIndex);
            m_pProc->pc->registry->ModifyInt(szBuffer, 1, &val, m_pProc);

            if (SSES_FAILURE_SERVER(ulEndStatus))
            {
                snprintf(szBuffer, 128, 
                        "LiveConnections.Entry%d.ClientsLeavingDueToServerError", 
                        m_nRegEntryIndex);
                m_pProc->pc->registry->ModifyInt(szBuffer, 1, &val, m_pProc);
            }
            else
            {
                snprintf(szBuffer, 128, 
                        "LiveConnections.Entry%d.ClientsLeavingDueToClientError", 
                        m_nRegEntryIndex);
                m_pProc->pc->registry->ModifyInt(szBuffer, 1, &val, m_pProc);
            }
        }
    }
    return HXR_OK;
}

BroadcastGateway::BroadcastGateway(BroadcastInfo* pInfo,
                                   const char*    pFilename,
                                   Process*       pStreamerProc)
{
    m_lRefCount                 = 1;
    m_pInfo                     = pInfo;
    m_State                     = INIT;
    m_ulPlayerCount             = 1;
    m_pBCastObj                 = 0;
    m_pFilename                 = new_string(pFilename);
    m_pFileHeader               = 0;
    m_pStreamHeaders            = 0;
    m_ulStreamCount             = 0;
    m_ulStreamGroupCount        = 0;
    m_ulSwitchGroupCount        = 0;
    m_aulLogicalStreamToStreamGroup = NULL;
    m_aulLogicalStreamToSwitchGroup = NULL;

    m_ulStreamHeadersSeen       = 0;
    m_ulStreamDonesSeen         = 0;

    m_pDestructCallback         = new DestructCallback;
    m_pDestructCallback->m_pBG  = this;
    m_pDestructCallback->AddRef();
    m_ulDestructCBHandle        = 0;

    m_pIdleStopCallback         = new IdleStopCallback;
    m_pIdleStopCallback->m_pBG  = this;
    m_pIdleStopCallback->AddRef();
    m_ulIdleStopCBHandle        = 0;

    m_pLatencyCalcCallback      = new LatencyCalcCallback;
    m_pLatencyCalcCallback->m_pBG   = this;
    m_pLatencyCalcCallback->AddRef();
    m_ulLatencyCalcCBHandle     = 0;

    m_pStreamDoneTable          = NULL;
    m_nRegEntryIndex            = -1;
    m_ulGwayLatencyTotal        = 0;
    m_ulGwayLatencyReps         = 0;
    m_ulMaxGwayLatency          = 0;
    m_ulAvgGwayLatencyRegID     = 0;
    m_ulMaxGwayLatencyRegID     = 0;
    m_bSureStreamAware          = FALSE;

    m_pASMSource                = NULL;
    m_bIsSubscribed             = FALSE;
    m_ppRuleData                = NULL;
    m_unKeyframeStreamGroup     = 0xffff;
    m_ulAudioStreamGroup        = 0xffff;
#ifdef DEBUG_IAT
    m_fAvgInterArrivalTime      = 0;
#endif
    m_fx = 1.0;
    m_fy = 0.25;
    m_fz = 2.0;

    m_tLastPacketArrival.tv_sec = 0;
    m_tLastPacketArrival.tv_usec = 0;

    m_ulRequestedBackOff        = 0;
    m_bUsePreBuffer             = FALSE;
    m_bUseLatencyRequirements   = FALSE;
    
    m_unNumStreamers            = 0;

    m_bDisableLiveTurboPlay     = FALSE;

    m_pPacketBufferQueue       = NULL;
    m_pLongPacketBufferQueue   = NULL;
    m_pFuturePacketBufferQueue = NULL;
    m_bQTrace                   = FALSE;
    m_bEnableRSDDebug           = FALSE; 
    m_bEnableRSDPerPacketLog    = FALSE;
    m_lMinPreroll               = 0;
    m_lExtraPrerollPercentage   = 0;
    m_bLowLatency               = FALSE;
    m_bQSizeTooSmallReported    = FALSE;
    m_ppSwitchGroupRSDInfo      = NULL;
    m_aulTmpSwitchGroupMap        = NULL;
    m_ppAudioSwitchGroupInfo    = NULL;
    INT32 lBuffering            = 0;
    
    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(DISABLE_TURBO_PLAY,
                                            &lBuffering, pStreamerProc))
    {
        m_bDisableLiveTurboPlay = (lBuffering != 0);
    }

    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(PACKET_BUFFER_QUEUE_SIZE,
                                            &m_lQueueSize, pStreamerProc))
    {
        if (m_lQueueSize <= 0)
        {
            m_lQueueSize = DEFAULT_PACKET_BUFFER_QUEUE_SIZE;
        }
    }
    else
    {
        m_lQueueSize = DEFAULT_PACKET_BUFFER_QUEUE_SIZE;
    }


    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(PACKET_BUFFER_QUEUE_DURATION,
                                            &m_lQueueDuration, pStreamerProc))
    {
        if (m_lQueueDuration <= 0)
        {
            m_lQueueDuration = DEFAULT_PACKET_BUFFER_QUEUE_DURATION;
        }
    }
    else
    {
        m_lQueueDuration = DEFAULT_PACKET_BUFFER_QUEUE_DURATION;
    }
    
    INT32 lInt = 0;
    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(PACKET_BUFFER_QUEUE_TRACE,
                                            &lInt, pStreamerProc))
    {
    m_bQTrace = (lInt != 0);
    }
    else
    {
    m_bQTrace = FALSE;
    }

    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(ENABLE_RSD_SERVER_LOG,
                                            &lInt, pStreamerProc))
    {
    m_bEnableRSDDebug = (lInt != 0);
    }
    else
    {
        m_bEnableRSDDebug = FALSE;
    }

    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(ENABLE_RSD_PER_PACKET_LOG,
                                            &lInt, pStreamerProc))
    {
        m_bEnableRSDPerPacketLog = (lInt != 0);
    }
    else
    {
        m_bEnableRSDPerPacketLog = FALSE;
    }

    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(RSD_MINIMUM_PREROLL,
                                            &lInt, pStreamerProc))
    {
        m_lMinPreroll = lInt;
    }
    else
    {
        m_lMinPreroll = RSD_DEFAULT_MINIMUM_PREROLL;
    }

    if (HXR_OK ==
        pStreamerProc->pc->registry->GetInt(RSD_EXTRA_PREROLL_PERCENTAGE,
                                            &lInt, pStreamerProc))
    {
        m_lExtraPrerollPercentage = lInt;
    }
    else
    {
        m_lExtraPrerollPercentage = RSD_DEFAULT_EXTRA_PREROLL_PER;
    }
    
    if (HXR_OK ==
    pStreamerProc->pc->registry->GetInt(RSD_MAX_DURATION_PACKET_BUFFER_QUEUE,
                                &lInt, pStreamerProc))
    {
        m_lMaxDurationOfPacketBufferQueue = lInt;
    }
    else
    {
        m_lMaxDurationOfPacketBufferQueue = DEFAULT_MAX_DURATION_OF_RSD_PACKET_BUFFER_QUEUE;
    }
    //Convert into milli seconds
    m_lMaxDurationOfPacketBufferQueue = m_lMaxDurationOfPacketBufferQueue * 1000;

    m_PacketBufferQueueLock = HXCreateMutex();
    m_LongPacketBufferQueueLock = HXCreateMutex();

    m_StateLock = HXCreateMutex();
    m_pPerStreamerFileHeaderLock = HXCreateMutex();
    m_pPerStreamerStreamHeaderLock = HXCreateMutex();
    m_BroadcastPacketManagerLock = HXCreateMutex();
#ifdef DEBUG_IAT
    m_InterArrivalTimeLock = HXCreateMutex();
#endif
    m_RuleDataLock = HXCreateMutex();

    m_tMaxQueueOccupancy.tv_sec = BCAST_MAX_QUEUE_SEC;
    m_tMaxQueueOccupancy.tv_usec = BCAST_MAX_QUEUE_USEC;

    InitCallback* pCB = new InitCallback;
    pCB->m_pGateway = this;

    /* A gateway cannot be established for a wildcard URL. */
    if (*pFilename == '*')
    {
        HXMutexLock(m_StateLock, TRUE);
        m_State = DONE;
        HXMutexUnlock(m_StateLock);
        return;
    }

    /* We need to pass control from the Streamer to the Live Process */
    pStreamerProc->pc->dispatchq->send(
        pStreamerProc, pCB, m_pInfo->m_ulProcnum);

    /* locate and save the live connect entry index for "LiveConnection.EntryN" */
    IHXBuffer* pBuffer = NULL;
    char szBuffer[128];
    IHXValues* pLiveConnections = 0;

    if (HXR_OK == pStreamerProc->pc->registry->GetPropList("LiveConnections",
                                                           pLiveConnections,
                                                           pStreamerProc))
    {
        const char* pProperty;
        const char* pIndex;
        UINT32 uId = 0;

        HX_RESULT hresult =
            pLiveConnections->GetFirstPropertyULONG32(pProperty, uId);

        while (HXR_OK == hresult)
        {
            if (!strncasecmp(pProperty, "LiveConnections.Entry", 21))
            {
                sprintf(szBuffer, "%s.FileName", pProperty);
                if (HXR_OK ==
                    pStreamerProc->pc->registry->GetBuf(szBuffer, &pBuffer,
                                                        pStreamerProc))
                {
                    // bingo - found it! 
                    if (strcmp((const char*) pBuffer->GetBuffer(), m_pFilename) == 0)
                    {
                        pIndex = pProperty + 21;
                        m_nRegEntryIndex = atoi(pIndex);
                        HX_RELEASE(pBuffer);

                        sprintf(szBuffer, "%s.GatewayLatencyAvg", pProperty);
                        m_ulAvgGwayLatencyRegID = pStreamerProc->pc->registry->GetId(szBuffer, pStreamerProc);
                        if (!m_ulAvgGwayLatencyRegID)
                        {
                            m_ulAvgGwayLatencyRegID = pStreamerProc->pc->registry->AddInt(szBuffer, 0, pStreamerProc);
                        }

                        sprintf(szBuffer, "%s.GatewayLatencyMax", pProperty);
                        m_ulMaxGwayLatencyRegID = pStreamerProc->pc->registry->GetId(szBuffer, pStreamerProc);
                        if (!m_ulMaxGwayLatencyRegID)
                        {
                            m_ulMaxGwayLatencyRegID = pStreamerProc->pc->registry->AddInt(szBuffer, 0, pStreamerProc);
                        }

                        break;
                    }

                    HX_RELEASE(pBuffer);
                }
            }

            hresult = pLiveConnections->GetNextPropertyULONG32(pProperty, uId);
        }

        HX_RELEASE(pLiveConnections);
    }

    IHXValues* pBCastDrainParams = 0;
    HX_RESULT hr;

    if (HXR_OK == pStreamerProc->pc->registry->GetPropList(
                   "config.BroadcastDrainParameters", pBCastDrainParams, pStreamerProc))
    {
        const char* pProperty;
        UINT32 uId = 0;

        hr = pBCastDrainParams->GetFirstPropertyULONG32(pProperty, uId);

        while (HXR_OK == hr)
        {
            hr = pStreamerProc->pc->registry->GetStr(pProperty, pBuffer, pStreamerProc);

            if (HXR_OK == hr)
            {
                if (!strcasecmp(pProperty, "config.BroadcastDrainParameters.X"))
                {
                    m_fx = (float)atof((const char *)pBuffer->GetBuffer());
                }
                else if (!strcasecmp(pProperty, "config.BroadcastDrainParameters.Y"))
                {
                    m_fy = (float)atof((const char *)pBuffer->GetBuffer());
                }
                else if (!strcasecmp(pProperty, "config.BroadcastDrainParameters.Z"))
                {
                    m_fz = (float)atof((const char *)pBuffer->GetBuffer());
                }
            }

            HX_RELEASE(pBuffer);

            hr = pBCastDrainParams->GetNextPropertyULONG32(pProperty, uId);
        }

        HX_RELEASE(pBCastDrainParams);
    }

    //printf("bcast gway x %f, y %f, z %f\n", m_fx, m_fy, m_fz);

    m_ppPerStreamerFileHeaders = new IHXValues*[MAX_THREADS];
    memset(m_ppPerStreamerFileHeaders, 0, sizeof(IHXValues *) * MAX_THREADS);

    m_pppPerStreamerStreamHeaders = new IHXValues**[MAX_THREADS];
    memset(m_pppPerStreamerStreamHeaders, 0, sizeof(IHXValues **) * MAX_THREADS);

    memset(m_pBroadcastPacketManagers, 0, (sizeof(BroadcastPacketManager*) * MAX_THREADS));
    memset(&m_pStreamers, 0, (sizeof(UINT16) * MAX_THREADS));


    /*
     * Do not store pStreamerProc since this class will execute in the
     * live process space and this is the streamer's proc
     */
}

BroadcastGateway::~BroadcastGateway()
{
    UINT32 i;

    HX_VECTOR_DELETE(m_pFilename);
    HX_VECTOR_DELETE(m_pStreamDoneTable);

    HX_RELEASE(m_pFileHeader);
    HX_RELEASE(m_pASMSource);


    for (i = 0; i < m_ulStreamCount; i++)
    {
        HX_RELEASE(m_pStreamHeaders[i]);
    }
    HX_VECTOR_DELETE(m_pStreamHeaders);

    HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
    for (UINT16 j = 0; j < MAX_THREADS; j++)
    {
        HX_RELEASE(m_pBroadcastPacketManagers[j]);
    }
    HXMutexUnlock(m_BroadcastPacketManagerLock);

    HXMutexLock(m_RuleDataLock, TRUE);

    for (i = 0; i < m_ulStreamCount; i++)
    {
        HX_DELETE(m_ppRuleData[i]);
    }
    HX_VECTOR_DELETE(m_ppRuleData);

    if (m_ppSwitchGroupRSDInfo)
    {
        for (i = 0; i < m_ulSwitchGroupCount+1; i++)
        {
            HX_DELETE(m_ppSwitchGroupRSDInfo[i]);
        }

        HX_VECTOR_DELETE(m_ppSwitchGroupRSDInfo);
    }

    HXMutexUnlock(m_RuleDataLock);

    if (m_ulDestructCBHandle)
    {
        m_pInfo->m_pProc->pc->engine->schedule.remove(m_ulDestructCBHandle);
        m_ulDestructCBHandle = 0;
    }
    HX_RELEASE(m_pDestructCallback);

    if (m_ulIdleStopCBHandle)
    {
        m_pInfo->m_pProc->pc->engine->schedule.remove(m_ulIdleStopCBHandle);
        m_ulIdleStopCBHandle = 0;
    }
    HX_RELEASE(m_pIdleStopCallback);

    if (m_ulLatencyCalcCBHandle)
    {
        m_pInfo->m_pProc->pc->engine->ischedule.remove(m_ulLatencyCalcCBHandle);
        m_ulLatencyCalcCBHandle = 0;
    }
    HX_RELEASE(m_pLatencyCalcCallback);

    HX_VECTOR_DELETE(m_aulLogicalStreamToStreamGroup);
    HX_VECTOR_DELETE(m_aulLogicalStreamToSwitchGroup);

    HX_RELEASE(m_pPacketBufferQueue);
    HX_RELEASE(m_pLongPacketBufferQueue);
    HX_RELEASE(m_pFuturePacketBufferQueue);

    HXDestroyMutex(m_PacketBufferQueueLock);
    HXDestroyMutex(m_LongPacketBufferQueueLock);


    HXDestroyMutex(m_StateLock);
    HXDestroyMutex(m_BroadcastPacketManagerLock);
#ifdef DEBUG_IAT
    HXDestroyMutex(m_InterArrivalTimeLock);
#endif
    HXDestroyMutex(m_RuleDataLock);

    if (m_ppPerStreamerFileHeaders)
    {
	HXMutexLock(m_pPerStreamerFileHeaderLock, TRUE);
	for (i = 0; i < MAX_THREADS; i++)
	    HX_RELEASE(m_ppPerStreamerFileHeaders[i]);
	HX_VECTOR_DELETE(m_ppPerStreamerFileHeaders);
	HXMutexUnlock(m_pPerStreamerFileHeaderLock);
    }

    if (m_pppPerStreamerStreamHeaders)
    {
	HXMutexLock(m_pPerStreamerStreamHeaderLock, TRUE);
	for (i = 0; i < MAX_THREADS; i++)
	{
	    if (m_pppPerStreamerStreamHeaders[i])
	    {
		for (int j = 0; j < m_ulStreamCount; j++)
		    HX_RELEASE(m_pppPerStreamerStreamHeaders[i][j]);
		HX_VECTOR_DELETE(m_pppPerStreamerStreamHeaders[i]);
	    }
	}
	HX_VECTOR_DELETE(m_pppPerStreamerStreamHeaders);
	HXMutexUnlock(m_pPerStreamerStreamHeaderLock);
    }

    HXDestroyMutex(m_pPerStreamerFileHeaderLock);
    HXDestroyMutex(m_pPerStreamerStreamHeaderLock);
    HX_VECTOR_DELETE(m_aulTmpSwitchGroupMap);

    if(m_ppAudioSwitchGroupInfo)
    {
        for (i = 0; i < m_ulSwitchGroupCount+1; i++)
        {
            HX_DELETE(m_ppAudioSwitchGroupInfo[i]);
        }

        HX_VECTOR_DELETE(m_ppAudioSwitchGroupInfo);
    }
}

BroadcastPacketManager::DestructCallback::DestructCallback(BroadcastPacketManager* pManager)
    : m_pPacketManager(pManager)
{
    m_pPacketManager->AddRef();
}

void
BroadcastPacketManager::DestructCallback::func(Process* pProc)
{
    m_pPacketManager->Done();
    HX_RELEASE(m_pPacketManager);
    delete this;
}
    
void
BroadcastPacketManager::SendDone(Process* pProc)
{
    SimpleCallback* cb = new DestructCallback(this);
    pProc->pc->dispatchq->send(pProc, cb, m_pProc->procnum());
}

STDMETHODIMP
BroadcastGateway::DestructCallback::Func()
{
    m_pBG->m_ulDestructCBHandle = 0;

    if (m_pBG->m_ulIdleStopCBHandle)
    {
        m_pBG->m_pInfo->m_pProc->pc->engine->schedule.remove(m_pBG->m_ulIdleStopCBHandle);
        m_pBG->m_ulIdleStopCBHandle = 0;
    }

    /* Don't need underlying broadcast object anymore */
    HX_RELEASE(m_pBG->m_pBCastObj);

    if (m_pBG->m_ulPlayerCount == 0)
    {
        HXMutexLock(m_pBG->m_BroadcastPacketManagerLock, TRUE);
        for (UINT16 i=0; i < MAX_THREADS; i++)
        {
            if (m_pBG->m_pBroadcastPacketManagers[i])
            {
                (m_pBG->m_pBroadcastPacketManagers[i])->SendDone(m_pBG->m_pInfo->m_pProc);
                HX_RELEASE((m_pBG->m_pBroadcastPacketManagers[i]));
            }
        }
        HXMutexUnlock(m_pBG->m_BroadcastPacketManagerLock);

        HX_RELEASE(m_pBG->m_pASMSource);
        HX_RELEASE(m_pBG);
    }
    else
    {
        m_pBG->m_ulDestructCBHandle = m_pBG->m_pInfo->m_pProc->pc->engine->schedule.enter(
                m_pBG->m_pInfo->m_pProc->pc->engine->now + Timeval(10.0),
                m_pBG->m_pDestructCallback);
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastGateway::IdleStopCallback::Func()
{
    if (m_pBG->m_ulPlayerCount == 0)
    {
        m_pBG->m_ulIdleStopCBHandle = 0;
        for (UINT16 i = 0; i < m_pBG->m_ulStreamCount; i++)
        {
            m_pBG->m_pBCastObj->StopPackets(i);
            m_pBG->StreamDone(i);
        }
    }
    else
    {
        m_pBG->m_ulIdleStopCBHandle = 
            m_pBG->m_pInfo->m_pProc->pc->engine->schedule.enter(
                    m_pBG->m_pInfo->m_pProc->pc->engine->now + Timeval(30.0),
                    m_pBG->m_pIdleStopCallback);
    }
    return HXR_OK;
}

STDMETHODIMP
BroadcastGateway::LatencyCalcCallback::Func()
{
    INT32 ulAvgLatency = 0;

    if (m_pBG->m_ulGwayLatencyReps)
    {
        ulAvgLatency = (INT32)(m_pBG->m_ulGwayLatencyTotal / m_pBG->m_ulGwayLatencyReps);
    }

    if (m_pBG->m_ulAvgGwayLatencyRegID)
    {
        m_pBG->m_pInfo->m_pProc->pc->registry->SetInt(
                m_pBG->m_ulAvgGwayLatencyRegID, (INT32)ulAvgLatency, m_pBG->m_pInfo->m_pProc);
    }

    if (m_pBG->m_ulMaxGwayLatencyRegID)
    {
        m_pBG->m_pInfo->m_pProc->pc->registry->SetInt(
                m_pBG->m_ulMaxGwayLatencyRegID, m_pBG->m_ulMaxGwayLatency, m_pBG->m_pInfo->m_pProc);
    }

    m_pBG->m_ulGwayLatencyTotal = 0;
    m_pBG->m_ulGwayLatencyReps = 0;
    // m_pBG->m_ulMaxGwayLatency = 0;

    if (m_pBG->m_pLatencyCalcCallback)
    {
        m_pBG->m_ulLatencyCalcCBHandle = 
            m_pBG->m_pInfo->m_pProc->pc->engine->ischedule.enter(
                    m_pBG->m_pInfo->m_pProc->pc->engine->now + Timeval(15.0),
                    m_pBG->m_pLatencyCalcCallback);
    }

    return HXR_OK;
}

void
BroadcastGateway::Init(Process* pProc)
{
    PluginHandler::Errors       ulResult;       
    PluginHandler::Plugin*      pPlugin;
    IUnknown*                   pUnknown;

    ulResult = pProc->pc->plugin_handler->
        m_broadcast_handler->Find(m_pInfo->m_pType, pPlugin);

    if (PluginHandler::NO_ERRORS != ulResult)
    {
        ASSERT(0);
        return;
    }

    // Must be called in the correct process space (which we are in hopefully)
    if (HXR_OK != pPlugin->GetInstance(&pUnknown))
    {
        ASSERT(0);
        return;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXBroadcastFormatObject,
                                          (void**)&m_pBCastObj))
    {
        ASSERT(0);
        return;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXASMSource,
                       (void**)&m_pASMSource))
    {
        m_pASMSource = NULL;
    }
    
    pUnknown->Release();

    m_pBCastObj->InitBroadcastFormat(m_pFilename, this);
}

void
BroadcastGateway::AddBroadcastStreamer(Process*                pProc,
                                       BroadcastStreamer* pStreamer)
{
    HX_ASSERT(pProc);
    HX_ASSERT(pStreamer);
    HX_ASSERT(pProc->procnum() < MAX_THREADS);

    if (pProc && pStreamer)
    {
        HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
        if (m_pBroadcastPacketManagers[pProc->procnum()] == NULL)
        {
            m_pBroadcastPacketManagers[pProc->procnum()] = new BroadcastPacketManager (pProc, this);
            m_pBroadcastPacketManagers[pProc->procnum()]->AddRef();

            m_pStreamers[m_unNumStreamers] = pProc->procnum();
            m_unNumStreamers++;

            if (m_State == STREAMING)
            {
                m_pBroadcastPacketManagers[pProc->procnum()]->Init(pProc);
            }
        }

        /* add this streamer to the streamer list for this procnum */

        if (m_pBroadcastPacketManagers[pProc->procnum()])
        {
            m_pBroadcastPacketManagers[pProc->procnum()]->AddBroadcastStreamer(pStreamer);
        }
        HXMutexUnlock(m_BroadcastPacketManagerLock);
    }
}

void
BroadcastGateway::RemoveBroadcastStreamer(Process*                pProc, 
                                          BroadcastStreamer* pStreamer)
{
    HX_ASSERT(pProc);
    HX_ASSERT(pStreamer);
    HX_ASSERT(pProc->procnum() < MAX_THREADS);

    if (pProc && pStreamer)
    {
        HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
        if (m_pBroadcastPacketManagers[pProc->procnum()])
        {
            m_pBroadcastPacketManagers[pProc->procnum()]->RemoveBroadcastStreamer(pStreamer);
        }
        HXMutexUnlock(m_BroadcastPacketManagerLock);
    }
}

void
ASMUpdateCallback::func(Process* pProc)
{
    if (m_pASMSource)
    {
        switch (m_cAction)
        {
            case ASM_SUBSCRIBE:
            {
                m_pASMSource->Subscribe(m_unStreamNumber, m_unRuleNumber);
                break;
            }
            case ASM_UNSUBSCRIBE:
            {
                m_pASMSource->Unsubscribe(m_unStreamNumber, m_unRuleNumber);
                break;
            }
        };
    }

    HX_RELEASE(m_pASMSource);
    delete this;
}

void
BroadcastGateway::InitCallback::func(Process* pProc)
{
    m_pGateway->Init(pProc);

    delete this;
}

STDMETHODIMP
BroadcastGateway::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();                                                    
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFormatResponse))
    {
        AddRef();
        *ppvObj = (IHXFormatResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


ULONG32
BroadcastGateway::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32
BroadcastGateway::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/* XXXSMP Handle all of these error conditions */
STDMETHODIMP
BroadcastGateway::InitDone(HX_RESULT ulStatus)
{
    if (ulStatus != HXR_OK)
    {
        Done();
        return HXR_OK;
    }

    IHXBroadcastLatency* pBroadcastLatency = NULL;

    if (HXR_OK == m_pBCastObj->QueryInterface(IID_IHXBroadcastLatency, (void**)&pBroadcastLatency))
    {
        if (HXR_OK == pBroadcastLatency->GetLatencyRequirements(m_ulRequestedBackOff, m_bUsePreBuffer))
        {
            m_bUseLatencyRequirements = TRUE;
        }
    }

    HX_RELEASE(pBroadcastLatency);

    m_pBCastObj->GetFileHeader();

    return HXR_OK;
}

STDMETHODIMP
BroadcastGateway::FileHeaderReady(HX_RESULT ulStatus, IHXValues* pHeader)
{
    if (ulStatus != HXR_OK)
    {
        Done();
        return HXR_OK;
    }

    FixupFileHeader(pHeader);
    pHeader->SetPropertyULONG32("LiveStream", 1);

#ifdef BCASTMGR_HEADER_DEBUG
    printf("\nFile Header:\n\n");
    DumpHeader(pHeader);
#endif //BCASTMGR_HEADER_DEBUG

    pHeader->GetPropertyULONG32("StreamCount", m_ulStreamCount);
    pHeader->GetPropertyULONG32("StreamGroupCount", m_ulStreamGroupCount);

    UINT32 ulTemp = 0;
    pHeader->GetPropertyULONG32("LatencyMode", ulTemp);
    m_bLowLatency = (ulTemp> 0) ? TRUE : FALSE;

    // Instant-on is currently disabled for low latency streams.
    if(m_bLowLatency)
    {
        m_bDisableLiveTurboPlay = TRUE;
    }

    ulTemp = 0;
    pHeader->GetPropertyULONG32("SureStreamAware", ulTemp);
    m_bSureStreamAware = (ulTemp> 0) ? TRUE : FALSE;

    m_pFileHeader = pHeader;
    pHeader->AddRef();

    m_pStreamHeaders = new IHXValues*[m_ulStreamCount];
    m_pStreamDoneTable = new UINT32[m_ulStreamCount];
    m_ppRuleData = new RuleData*[m_ulStreamCount];
    m_aulLogicalStreamToStreamGroup = new UINT32[m_ulStreamCount];
    m_aulLogicalStreamToSwitchGroup = new UINT32[m_ulStreamCount];
    m_aulTmpSwitchGroupMap = new UINT32[m_ulStreamCount];

    for (UINT16 i = 0; i < m_ulStreamCount; i++)
    {
        m_pStreamHeaders[i] = 0;
        m_pStreamDoneTable[i] = BCAST_STREAMDONE_TABLE_OK;
        m_ppRuleData[i] = NULL;
        m_aulLogicalStreamToStreamGroup[i] = 0;
        m_aulLogicalStreamToSwitchGroup[i] = 0;
        m_aulTmpSwitchGroupMap[i] = 0;
    }

    for (i = 0; i < m_ulStreamCount; i++)
    {
        m_pBCastObj->GetStreamHeader(i);
    }
    return HXR_OK;
}

STDMETHODIMP
BroadcastGateway::StreamHeaderReady(HX_RESULT ulStatus, IHXValues* pHeader)
{
    if (ulStatus != HXR_OK)
    {
        Done();
        return HXR_OK;
    }

    UINT32 ulSN = 0;
    UINT16 i    = 0;
    UINT16 j    = 0;

    pHeader->GetPropertyULONG32("StreamNumber", ulSN);
    FixupStreamHeader(ulSN, pHeader);
    m_pStreamHeaders[ulSN] = pHeader;
    pHeader->AddRef();

#ifdef BCASTMGR_HEADER_DEBUG
    printf("\nStream %u:\n\n", ulSN);
    DumpHeader(pHeader);
#endif //BCASTMGR_HEADER_DEBUG

    UINT32 ulStreamGroupNum = 0;
    pHeader->GetPropertyULONG32("StreamGroupNumber", ulStreamGroupNum);
    m_aulLogicalStreamToStreamGroup[ulSN] = ulStreamGroupNum;

    // SwitchGroupIDs are not always sequential and can be random integers between
    // 1 to n for the live streams from SLTA. This is an attempt to identify the 
    // number of unique SwitchGroupIDs and map them to contiguous numbers starting
    // with 1 for easier processing in rest of the BroadcastGateway.
    // m_aulTmpSwitchGroupMap is temporary array that holds these mappings. Ideally
    // this should be a CHXMapLongtoLong map, but we don't have such map available.
    UINT32 ulSwitchGroupNum = 0;
    pHeader->GetPropertyULONG32("SwitchGroupID", ulSwitchGroupNum);

    for(i=0; i < m_ulSwitchGroupCount; i++)
    {
        if (m_aulTmpSwitchGroupMap[i] == ulSwitchGroupNum)
        {
            break;
        }
    }

    if (i == m_ulSwitchGroupCount)
    {
        m_aulTmpSwitchGroupMap[i] = ulSwitchGroupNum;
        m_ulSwitchGroupCount++;
    }

    m_aulLogicalStreamToSwitchGroup[ulSN] = i+1;

    m_ulStreamHeadersSeen++;

    if (m_ulStreamHeadersSeen == m_ulStreamCount)
    {
        /* setup asm rule subscription tracking vector: */
        IHXBuffer* pRuleBook = NULL;
        IHXBuffer* pMimeType = NULL;
        IHXBuffer* pRule     = NULL;
        ASMRuleBook* pParsedRuleBook = NULL;
        IHXRegistry* pReg = NULL;

        m_pInfo->m_pProc->pc->server_context->QueryInterface(IID_IHXRegistry, (void **)&pReg);

        /*we want to detect keyframe stream for the following contents:
          1. video(rm and 3gpp) stream with any other streams.
          2. only audio(rm) stream
          3. only audio(rm) and event streams.
         */
        // Because we want to detect audio only content, which may contain two streams: audio and 
        // event streams. We need an extra loop here.
        UINT32 unVideoKeyframeStrmGroup = 0xffff;
        UINT32 unAudioKeyframeStrmGroup = 0xffff;
        UINT32 unEventStreamStrmGroup = 0xffff;
        for (i = 0; i < m_ulStreamCount; i++)
        {
            if (SUCCEEDED(m_pStreamHeaders[i]->GetPropertyCString ("MimeType", pMimeType)) && pMimeType)
            {
                const char* szMimeType = (const char*)pMimeType->GetBuffer();
                if (szMimeType)
                {
                    if (IsVideoKeyframeStream(pReg, szMimeType))
                    {
                        unVideoKeyframeStrmGroup = m_aulLogicalStreamToStreamGroup[i];
                    }
                    else if (IsAudioKeyframeStream(pReg, szMimeType))
                    {
                        unAudioKeyframeStrmGroup = m_aulLogicalStreamToStreamGroup[i];
                    }
                    else if (IsRealEventStream(pReg, szMimeType))
                    {
                        unEventStreamStrmGroup = m_aulLogicalStreamToStreamGroup[i];
                    }
                    
                    if(strncmp(szMimeType, "audio/", 6) == 0)
                    {
                        m_ulAudioStreamGroup = m_aulLogicalStreamToStreamGroup[i];
                    }
                }
                HX_RELEASE(pMimeType);
            }
        }
        HX_RELEASE(pReg);

        if (unVideoKeyframeStrmGroup != 0xffff)
        {
            m_unKeyframeStreamGroup = unVideoKeyframeStrmGroup;
        }
        // only one audio stream
        else if (m_ulStreamGroupCount == 1 && unAudioKeyframeStrmGroup != 0xffff)
        {
            m_unKeyframeStreamGroup = unAudioKeyframeStrmGroup;
        }
        // if the content contains only audio and event streams
        else if (m_ulStreamGroupCount == 2 && unAudioKeyframeStrmGroup != 0xffff && unEventStreamStrmGroup != 0xffff)
        {
            m_unKeyframeStreamGroup = unAudioKeyframeStrmGroup;
        }

        if (m_unKeyframeStreamGroup == 0xffff)
        {
            m_bDisableLiveTurboPlay = TRUE;
        }

        if (!m_bDisableLiveTurboPlay)
        {
            m_ppSwitchGroupRSDInfo = new SwitchGroupRSDInfo*[m_ulSwitchGroupCount+1];
            memset(m_ppSwitchGroupRSDInfo, 0, sizeof(SwitchGroupRSDInfo*) * (m_ulSwitchGroupCount+1));
        }

        if (!m_bDisableLiveTurboPlay && m_ulAudioStreamGroup != 0xffff 
            && m_ulAudioStreamGroup != m_unKeyframeStreamGroup)
        {
            m_ppAudioSwitchGroupInfo = new AudioSwitchGroupInfo*[m_ulSwitchGroupCount+1];
            memset(m_ppAudioSwitchGroupInfo, 0, sizeof(AudioSwitchGroupInfo*) * (m_ulSwitchGroupCount+1));
        }

        for (i = 0; i < m_ulStreamCount; i++)
        {
            m_pStreamHeaders[i]->GetPropertyCString("ASMRulebook", pRuleBook);
            UINT32 ulStreamGroupNum = m_aulLogicalStreamToStreamGroup[i];
            UINT32 ulSwitchGroupNum = m_aulLogicalStreamToSwitchGroup[i];

            if (pRuleBook)
            {
                pParsedRuleBook = new ASMRuleBook((const char*)pRuleBook->GetBuffer());
                HX_ASSERT(SUCCEEDED(pParsedRuleBook->m_LastError));

                UINT16 unNumRules = pParsedRuleBook->GetNumRules();
                IHXValues* pRuleProps = NULL;
                BOOL bOnDepend = FALSE;

                HX_DELETE(m_ppRuleData[i]);
                m_ppRuleData[i] = new RuleData (unNumRules);
                
                if (!m_bDisableLiveTurboPlay && m_unKeyframeStreamGroup == ulStreamGroupNum)
                {
                    if (m_ppSwitchGroupRSDInfo[ulSwitchGroupNum] == NULL)
                    {
                        m_ppSwitchGroupRSDInfo[ulSwitchGroupNum] = new SwitchGroupRSDInfo(unNumRules);
                    }
                
                    UINT32 ulPreroll = 0;
                    if (m_pStreamHeaders[i]->GetPropertyULONG32("Preroll", ulPreroll) == HXR_OK && 
                        ulPreroll > 0 && ulPreroll > m_lQueueDuration)
                    {
                        m_lQueueDuration = ulPreroll;
                    }
                }

                if(!m_bDisableLiveTurboPlay && m_ulAudioStreamGroup == ulStreamGroupNum &&
                    m_ppAudioSwitchGroupInfo)
                {
                    if(m_ppAudioSwitchGroupInfo[ulSwitchGroupNum] == NULL)
                    {
                        m_ppAudioSwitchGroupInfo[ulSwitchGroupNum] = new AudioSwitchGroupInfo(unNumRules);
                    }
                }

                for (j = 0; j < unNumRules; j++)
                {
                    pRule = NULL;
                    pParsedRuleBook->GetProperties(j, pRuleProps);

                    if (pRuleProps)
                    {
                        pRuleProps->GetPropertyCString("OnDepend", pRule);
                        if (pRule)
                        {
                            bOnDepend = TRUE;
                            pRule->Release();
                            pRule = NULL;
                        }

                        //For 3GP live, key frame can be received on either of the two rules.
                        //So receving a key frame packet on either of the two interdependent 
                        //rules is sufficient.
                        INT32 lInterDepend = -1;
                        pRuleProps->GetPropertyCString("InterDepend", pRule);

                        if (pRule)
                        {
                            lInterDepend = atoi((const char*)pRule->GetBuffer());
                            HX_RELEASE(pRule);
                        }

                        // XXXJJ if this rule has "OnDepend", that means it depends on some other rule(s).
                        // Therefore we will mark it as dependent rule, and not key frame rule.  
                        // The reason is that even if the players get the keyframe of this rule, they 
                        // can't begin the playback because this rule depends on other rules for rendering.
                        if (!m_bDisableLiveTurboPlay && 
                            m_unKeyframeStreamGroup == ulStreamGroupNum &&
                            m_ppSwitchGroupRSDInfo[ulSwitchGroupNum])
                        {
                            m_ppSwitchGroupRSDInfo[ulSwitchGroupNum]->SetKeyFrameRule(j, !bOnDepend);
                            m_ppSwitchGroupRSDInfo[ulSwitchGroupNum]->SetInterDependency(j, lInterDepend);
                        }

                        if(m_ppAudioSwitchGroupInfo && m_ppAudioSwitchGroupInfo[ulSwitchGroupNum])
                        {
                            m_ppAudioSwitchGroupInfo[ulSwitchGroupNum]->SetOnDepend(j, bOnDepend);
                            m_ppAudioSwitchGroupInfo[ulSwitchGroupNum]->SetInterDepend(j, lInterDepend);
                        }

                        bOnDepend = FALSE;
                    }

                    HX_RELEASE(pRuleProps);
                }

                HX_DELETE(pParsedRuleBook);
                HX_RELEASE(pRuleBook);
            }
        }

        if (!m_bDisableLiveTurboPlay)
        {
            // enforce minimum queue duration
            if (m_lMinPreroll && m_lQueueDuration < m_lMinPreroll)
            {
                m_lQueueDuration = m_lMinPreroll;
            }
        }

        /* don't start the streams until we're completelysetup: */
        for (i = 0; i < m_ulStreamCount; i++)
        {
            if (!m_bDisableLiveTurboPlay)
            {
                // Add ServerPreroll to stream headers.
                // ServerPreroll is an internal preroll value not to be communicated
                UINT32 ulPreroll = 0;
                if (m_pStreamHeaders[i]->GetPropertyULONG32("Preroll", ulPreroll) != HXR_OK)
                {
                    m_pStreamHeaders[i]->SetPropertyULONG32("Preroll", m_lQueueDuration);
                }
                m_pStreamHeaders[i]->SetPropertyULONG32("ServerPreroll", m_lQueueDuration);
            }

            m_pBCastObj->StartPackets(i);
        }

        if (!m_bDisableLiveTurboPlay && m_lExtraPrerollPercentage)
        {
            UINT64 ulExtraDuration = m_lQueueDuration * m_lExtraPrerollPercentage;
            ulExtraDuration  /= 100;
            m_lQueueDuration += (UINT32)ulExtraDuration;
        }

        // This is where the state transition should happen! Can't make the change
        // until all live sources tested (only RBS live has been tested to date)
#ifdef BROADCAST_GATEWAY_STATE_BUG_FIXED
        /* Initialize the packet managers */
        HXMutexLock(m_StateLock, TRUE);
        if (m_State == INIT)
        {
            m_State = STREAMING;

            m_ulIdleStopCBHandle =
                m_pInfo->m_pProc->pc->engine->schedule.enter(
                        m_pInfo->m_pProc->pc->engine->now + Timeval(30.0),
                        m_pIdleStopCallback);

            m_ulLatencyCalcCBHandle =
                m_pInfo->m_pProc->pc->engine->ischedule.enter(
                        m_pInfo->m_pProc->pc->engine->now + Timeval(15.0),
                        m_pLatencyCalcCallback);

            HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
            for (UINT16 i = 0; i < m_unNumStreamers; i++)
            {
                if (m_pBroadcastPacketManagers[m_pStreamers[i]] != NULL)
                {
                    m_pBroadcastPacketManagers[m_pStreamers[i]]->
                        Init(m_pInfo->m_pProc);
                }
            }
            HXMutexUnlock(m_BroadcastPacketManagerLock);
        }
        HXMutexUnlock(m_StateLock);
#endif
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastGateway::PacketReady(HX_RESULT ulStatus, IHXPacket* pPacket)
{
    HX_ASSERT(pPacket->GetStreamNumber() < m_ulStreamCount); 

    if ( (ulStatus != HXR_OK) || (pPacket == NULL) )
    {
        Done();
        return HXR_OK;
    }

    *g_pLiveIncomingPPS += 1;

#ifdef DEBUG_IAT
    Timeval tInterArrivalTime = m_pInfo->m_pProc->pc->engine->now - m_tLastPacketArrival;

    if ((m_tLastPacketArrival.tv_sec == 0) && 
            (m_tLastPacketArrival.tv_usec == 0)) 
    {
        tInterArrivalTime.tv_sec = 0;
        tInterArrivalTime.tv_usec = 20000;
    }

    float fInterArrivalTime = tInterArrivalTime.tv_usec / 1000 +
        tInterArrivalTime.tv_sec * 1000;

    HXMutexLock(m_InterArrivalTimeLock, TRUE);
    m_fAvgInterArrivalTime = (m_fAvgInterArrivalTime != 0) ?
        (0.99 * m_fAvgInterArrivalTime +
         0.01 * fInterArrivalTime) : fInterArrivalTime;
    HXMutexUnlock(m_InterArrivalTimeLock);

#if 0
    printf("interarr time %ld.%06ld, fAvgIAT %f\n", tInterArrivalTime.tv_sec, 
            tInterArrivalTime.tv_usec, m_fAvgInterArrivalTime);    
#endif

#endif
    UINT32 uStreamNumber;
    UINT16 unRule;

    uStreamNumber = pPacket->GetStreamNumber();
    unRule = pPacket->GetASMRuleNumber();

    
    if (!m_bDisableLiveTurboPlay)
    {
        HandlePacketBufferQueue(pPacket, uStreamNumber, unRule);
    }

    m_tLastPacketArrival = m_pInfo->m_pProc->pc->engine->now;

    /* Queue the Packet in each streamer proc*/
    HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
    for (UINT16 i = 0; i < m_unNumStreamers; i++)
    {
        if (m_pBroadcastPacketManagers[m_pStreamers[i]] != NULL)
        {
            m_pBroadcastPacketManagers[m_pStreamers[i]]->
                QueuePacket(pPacket);
        }
    }
    HXMutexUnlock(m_BroadcastPacketManagerLock);

    /* This code is currently active but needs to be fixed. We don't go 
     * into STREAMING state until we get the first packet. But this is 
     * not a good design : it means sources have to send packets before
     * they get subscribes from clients because the gateway setup won't
     * complete until we go into STREAMING. The fix moves the transition
     * from PacketReady to StreamHeaderReady, its been tested with RBS live
     * but not MMS, RTP live or legacy encoplin sources.
     * Look for BROADCAST_GATEWAY_STATE_BUG_FIXED here and in broadcast receiver
     */
#ifndef BROADCAST_GATEWAY_STATE_BUG_FIXED
    /* Start Players: */
    HXMutexLock(m_StateLock, TRUE);
    if (m_State == INIT)
    {
        m_State = STREAMING;

        m_ulIdleStopCBHandle =
            m_pInfo->m_pProc->pc->engine->schedule.enter(
                    m_pInfo->m_pProc->pc->engine->now + Timeval(30.0),
                    m_pIdleStopCallback);

        m_ulLatencyCalcCBHandle = 
            m_pInfo->m_pProc->pc->engine->ischedule.enter(
                    m_pInfo->m_pProc->pc->engine->now + Timeval(15.0),
                    m_pLatencyCalcCallback);

        HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
        for (UINT16 i = 0; i < m_unNumStreamers; i++)
        {
            if (m_pBroadcastPacketManagers[m_pStreamers[i]] != NULL)
            {
                m_pBroadcastPacketManagers[m_pStreamers[i]]->Init(m_pInfo->m_pProc);
            }
        }
        HXMutexUnlock(m_BroadcastPacketManagerLock);
    }
    HXMutexUnlock(m_StateLock);
#endif

    return HXR_OK;
}

STDMETHODIMP
BroadcastGateway::StreamDone(UINT16 unStreamNumber)
{
    HX_ASSERT(unStreamNumber < m_ulStreamCount);

    HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
    for (UINT16 i = 0; i < m_unNumStreamers; i++)
    {
        if (m_pBroadcastPacketManagers[m_pStreamers[i]] != NULL)
        {
            m_pBroadcastPacketManagers[m_pStreamers[i]]->
                SendStreamDone(unStreamNumber);
        }
    }
    HXMutexUnlock(m_BroadcastPacketManagerLock);

    m_ulStreamDonesSeen++;
    if (m_ulStreamDonesSeen == m_ulStreamCount)
    {
        m_ulMaxGwayLatency = 0;

        Done();
    }
  
    return HXR_OK;
}

STDMETHODIMP
BroadcastGateway::SeekDone(HX_RESULT ulStatus)
{
    return HXR_OK;
}

IHXValues*
BroadcastGateway::GetFileHeader()
{
    HX_ASSERT(g_nTLSProcnum < MAX_THREADS);

    if (!m_ppPerStreamerFileHeaders[g_nTLSProcnum])
    {
	HXMutexLock(m_pPerStreamerFileHeaderLock, TRUE);
	m_ppPerStreamerFileHeaders[g_nTLSProcnum] = CloneHeader(m_pFileHeader,
	    g_pTLSProc->pc->server_context);
	HXMutexUnlock(m_pPerStreamerFileHeaderLock);
    }
    return m_ppPerStreamerFileHeaders[g_nTLSProcnum];
}

IHXValues*
BroadcastGateway::GetStreamHeaders(UINT32 ulStreamNumber)
{
    HX_ASSERT(g_nTLSProcnum < MAX_THREADS);

    if (!m_pppPerStreamerStreamHeaders[g_nTLSProcnum])
    {
	HXMutexLock(m_pPerStreamerStreamHeaderLock, TRUE);
	m_pppPerStreamerStreamHeaders[g_nTLSProcnum] = new IHXValues*[m_ulStreamCount];
	for (int i = 0; i < m_ulStreamCount; i++)
	{
	    m_pppPerStreamerStreamHeaders[g_nTLSProcnum][i] =
		CloneHeader(m_pStreamHeaders[i], g_pTLSProc->pc->server_context);
	}
	HXMutexUnlock(m_pPerStreamerStreamHeaderLock);
    }
    return m_pppPerStreamerStreamHeaders[g_nTLSProcnum][ulStreamNumber];
}

HX_RESULT
BroadcastGateway::Done()
{
    int i=0;
    HXMutexLock(m_StateLock, TRUE);
    m_State = DONE;
    HXMutexUnlock(m_StateLock);

    HXMutexLock(m_pInfo->m_GatewayLock, TRUE);
    m_pInfo->m_pCurrentBroadcasts->RemoveKey(m_pFilename);
    HXMutexUnlock(m_pInfo->m_GatewayLock);

    m_ulDestructCBHandle = m_pInfo->m_pProc->pc->engine->schedule.enter(
        m_pInfo->m_pProc->pc->engine->now + Timeval(10.0),
        m_pDestructCallback);

    return HXR_OK;
}

IHXLivePacketBufferQueue* 
BroadcastGateway::GetPacketBufferQueue(void)
{
    IHXLivePacketBufferQueue* pQueue = NULL;

    if (!m_bDisableLiveTurboPlay && m_pPacketBufferQueue)
    {
        HXMutexLock(m_PacketBufferQueueLock, TRUE);
        pQueue = (IHXLivePacketBufferQueue*)m_pPacketBufferQueue;
        if (pQueue)
        {
            pQueue->AddRef();
        }
        HXMutexUnlock(m_PacketBufferQueueLock);
    }

    return pQueue;
}

IHXLivePacketBufferQueue* 
BroadcastGateway::GetLongPacketBufferQueue(void)
{
    IHXLivePacketBufferQueue* pQueue = NULL;

    if (!m_bDisableLiveTurboPlay && m_pLongPacketBufferQueue)
    {
        HXMutexLock(m_LongPacketBufferQueueLock, TRUE);
        pQueue = (IHXLivePacketBufferQueue*)m_pLongPacketBufferQueue;
        if (pQueue)
        {
            pQueue->AddRef();
        }
        HXMutexUnlock(m_LongPacketBufferQueueLock);
    }

    return pQueue;
}
void BroadcastGateway::AddPacketToQueue(CPacketBufferQueue*& pQueue,
                                        IHXPacket* pPacket,
                                        BOOL& bIncQSize,
                                        HX_MUTEX queueLock)
{
    if(!pQueue)
    {
        return;
    }

    UINT32 ulCurrentQSize = 0;
    UINT32 ulRefCount = 0;

    if (pQueue->EnQueue(pPacket) != HXR_OK)
    {
        ulCurrentQSize = pQueue->GetSize();

        if(queueLock)
        {
            HXMutexLock(queueLock, TRUE);
            ulRefCount = pQueue->Release();
            pQueue = NULL;
            HXMutexUnlock(queueLock);
        }
        else
        {
            ulRefCount = pQueue->Release();
            pQueue = NULL;
        }


        //we only increase the size of the queue under the two conditions:
        //1. somebody is using the queue, judged  by refcount > 0 after we release it.
        //  it does no harm when we reach the max queue size while nobody is using //  it.
        //2. the queue size hasn't been increased yet, judged by ulCurrentQSize >= m_lQueueSize.
        // it is possible the queue size had been increased by other queues.
        if (ulRefCount > 0 && ulCurrentQSize >= m_lQueueSize)
        {
            bIncQSize = TRUE;
        }
    }
}

HX_RESULT
BroadcastGateway::HandlePacketBufferQueue(IHXPacket* pPacket, 
                                          UINT32 uStreamNumber, 
                                          UINT16 unRule)
{
    char szTime[128] = {0};
    Timeval tNow = 0;

    if(m_bEnableRSDDebug || m_bEnableRSDPerPacketLog)
    {
        tNow = m_pInfo->m_pProc->pc->engine->now;
        struct tm localTime;
        hx_localtime_r(&tNow.tv_sec, &localTime);
        strftime(szTime, 128, "%d-%b-%y %H:%M:%S", &localTime);
    }

    if (m_bSureStreamAware && !m_bIsSubscribed)
    {
        if (m_bEnableRSDDebug && 
            (m_pPacketBufferQueue || m_pLongPacketBufferQueue || m_pFuturePacketBufferQueue))
        {
            fprintf(stderr, "%s.%03d RSDLive(File: %s) Flushing RSD queues\n",
                    szTime, tNow.tv_usec/1000, m_pFilename);
            fflush(0);
        }

        //Flush queues and return
        if (m_pPacketBufferQueue)
        {
            HXMutexLock(m_PacketBufferQueueLock, TRUE);
            HX_RELEASE(m_pPacketBufferQueue);
            HXMutexUnlock(m_PacketBufferQueueLock);
        }

        if (m_pLongPacketBufferQueue)
        {
            HXMutexLock(m_LongPacketBufferQueueLock, TRUE);
            HX_RELEASE(m_pLongPacketBufferQueue);
            HXMutexUnlock(m_LongPacketBufferQueueLock);
        }

        HX_RELEASE(m_pFuturePacketBufferQueue);
        ResetSwitchGroupRSDInfo();
        return HXR_OK;
    }

    BOOL            bKeyFrame = FALSE;
    UINT32          ulRefCount = 0;
    BOOL            bAddedToFQ = FALSE;
    BOOL            bIncQSize = FALSE;
    UINT            ulDuration = 0;
    UINT32          ulStreamGroupNum = m_aulLogicalStreamToStreamGroup[uStreamNumber];
    UINT32          ulSwitchGroupNum = m_aulLogicalStreamToSwitchGroup[uStreamNumber];

    /*
    * ASMFlags with HX_ASM_SWITCH_ON bit set means this packet is a keyframe.
    * But we only count keyframes from independent rules as "real" keyframes
    * for our buffering purpose.
    *
    * XXXJJ Intentionally keep the above inaccurate comments. Most people perceived
    * the same wrong concept as me.  Below is the correct keyframe definition from Larry.
    * 
    * HX_ASM_SWITCH_ON does not indicate a keyframe.  It only indicates a potential
    * starting point.  For video, that could be the start of an I, B, or P frame.  
    * E.g., if an I/B/P frame is large enough to be fragmented across multiple packets,
    * only the first packet of the frame would be marked with HX_ASM_SWITCH_ON (since 
    * you don't want to send a partial frame...).  Only the last packet of the frame
    * would be marked with HX_ASM_SWITCH_OFF (again, since you don't want to send a 
    * partial frame). A keyframe is indicated by the ASM rule not having any OnDepend 
    * directives.  The first packet of a particular keyframe is indicated by 
    * HX_ASM_SWITCH_ON.
    *
    * XXX AAK: part of the fix for pr# 215519 - where the server was
    * incorrectly syncing on the end of the keyframe (in addition to
    * the correct sync at the start of the keyframe).
    * the new flag HX_ASM_SIDE_EFFECT signifies the end of the
    * keyframe, so make sure that we find HX_ASM_SWITCH_ON and
    * NOT HX_ASM_SIDE_EFFECT to indicate the start of the keyframe.
    */

    bKeyFrame = ((pPacket->GetASMFlags() & HX_ASM_SWITCH_ON) &&
                 !(pPacket->GetASMFlags() & HX_ASM_SIDE_EFFECT) &&
                 IsKeyFrameRule(uStreamNumber, unRule));


    if (m_bEnableRSDPerPacketLog)
    {
        IHXServerPacketExt* pPacketExt = NULL;
        pPacket->QueryInterface(IID_IHXServerPacketExt, (void **)&pPacketExt);
                            
        UINT32 ulStrmSeqNo= 0;
        if (pPacketExt)
        {        
            ulStrmSeqNo = pPacketExt->GetStreamSeqNo();
            pPacketExt->Release();
        }

        fprintf(stderr, "%s.%03d RSDLive(File: %s) Packet received: T=%d stream=%d rule=%d "
                "Keyframe=%d strmseq=%d\n", szTime, tNow.tv_usec/1000, m_pFilename, pPacket->GetTime(),
                uStreamNumber, unRule, bKeyFrame, ulStrmSeqNo);
        fflush(0);
    }

    // Create a future queue if not present
    if (bKeyFrame)
    {
        if (NULL == m_pFuturePacketBufferQueue)
        {
            m_pFuturePacketBufferQueue = new CPacketBufferQueue(m_lQueueSize, m_bQTrace, 0);
                                                //GetNumOfReservedAudioPackets());
            m_pFuturePacketBufferQueue->AddRef();
            m_pFuturePacketBufferQueue->AddKeyframe(pPacket);
        
            bAddedToFQ = TRUE;
        
            if (m_bEnableRSDDebug)
            {
                fprintf(stderr, "%s.%03d RSDLive(File: %s, stream %d, rule %d) Created Future Queue 0x%x, "
                        "starting timestamp: %d\n", szTime, tNow.tv_usec/1000, m_pFilename, uStreamNumber,
                        unRule, m_pFuturePacketBufferQueue, pPacket->GetTime());
                fflush(0);
            }
        }

        HX_ASSERT(m_ppSwitchGroupRSDInfo[ulSwitchGroupNum]);
        m_ppSwitchGroupRSDInfo[ulSwitchGroupNum]->OnKeyFramePacket(pPacket, unRule);
    }

    AddPacketToQueue(m_pLongPacketBufferQueue, pPacket, bIncQSize, m_LongPacketBufferQueueLock);

    //Queue the packet to current queue, if present
    AddPacketToQueue(m_pPacketBufferQueue, pPacket, bIncQSize, m_PacketBufferQueueLock);

    //Queue the packet to furture queue, if present
    if (!bAddedToFQ)
    {
        AddPacketToQueue(m_pFuturePacketBufferQueue, pPacket, bIncQSize, NULL);
    }
 
    //Check the minimum queue duration of the future queue
    if (IsMinRSDQueueDuration(pPacket))
    {
        if (m_bEnableRSDDebug)
        {
            fprintf(stderr, "%s.%03d RSDLive(File: %s) Moving Future Queue to Current Queue. "
                    "CurrentQDepth=%d FutureQDepth=%d CurrentQTS=%d FutureQTS=%d\n",
                    szTime, tNow.tv_usec/1000, m_pFilename, 
                    m_pPacketBufferQueue ? m_pPacketBufferQueue->GetSize() : 0,
                    m_pFuturePacketBufferQueue ?  m_pFuturePacketBufferQueue->GetSize() :0,
                    m_pPacketBufferQueue ? m_pPacketBufferQueue->GetStartTime() : 0,
                    m_pFuturePacketBufferQueue ?  m_pFuturePacketBufferQueue->GetStartTime() :0);
                    
            fflush(0);
        }

#if 0
        //fill up the audio packets before the video keyframe
        //we need the current queues to find the audio packets right before the video keyframe
        if(m_ppAudioSwitchGroupInfo && m_pPacketBufferQueue)
        {
            IHXPacket* pVideoKeyFrame = m_pFuturePacketBufferQueue->GetKeyFramePacket();
            UINT32 i = 0;
            for(i = 1; i < m_ulSwitchGroupCount+1; i++)
            {
                if(m_ppAudioSwitchGroupInfo[i])
                {
                    m_ppAudioSwitchGroupInfo[i]->SetVideoKeyframePacket(pVideoKeyFrame);
                }
            }
            HX_RELEASE(pVideoKeyFrame);

            UINT32 ulIndex = m_pPacketBufferQueue->GetSize() - 1;
            while(!HasAllAudioPackets())
            {
                IHXPacket* pQueuedPacket = NULL;
                if(FAILED(m_pPacketBufferQueue->GetPacket(ulIndex, pQueuedPacket)))
                {
                    break;
                }

                UINT32 ulSwitchGroup = m_aulLogicalStreamToSwitchGroup[pQueuedPacket->GetStreamNumber()];
                if(m_ppAudioSwitchGroupInfo[ulSwitchGroup])
                {
                    m_ppAudioSwitchGroupInfo[ulSwitchGroup]->OnPacket(pQueuedPacket);
                }
                ulIndex--;
                if(ulIndex == 0)
                {
                    break;
                }
            }

            for(i = 1; i < m_ulSwitchGroupCount+1; i++)
            {
                if(m_ppAudioSwitchGroupInfo[i])
                {
                    IHXPacket** ppPacket = NULL;
                    UINT32 ulCount = 0;
                    m_ppAudioSwitchGroupInfo[i]->GetAudioPackets(ppPacket, ulCount);
                    for(UINT32 j = 0;  j < ulCount; j++)
                    {
                        if(ppPacket[j])
                        {
                            m_pFuturePacketBufferQueue->AddAudioPacket(ppPacket[j]);
                        }
                    }
                    m_ppAudioSwitchGroupInfo[i]->Reset();
                }
                
            }
        }
#endif

        HXMutexLock(m_LongPacketBufferQueueLock, TRUE);
        HX_RELEASE(m_pLongPacketBufferQueue);
        // taking AddRef()
        m_pLongPacketBufferQueue = m_pPacketBufferQueue;
        HXMutexUnlock(m_LongPacketBufferQueueLock);

        HXMutexLock(m_PacketBufferQueueLock, TRUE);
        // taking AddRef()
        m_pPacketBufferQueue = m_pFuturePacketBufferQueue;
        m_pFuturePacketBufferQueue = NULL;                
        HXMutexUnlock(m_PacketBufferQueueLock);

        //Reset SwitchGroupRSDInfo array.
        ResetSwitchGroupRSDInfo();
    }

    //Increase the queue size, if required
    if (bIncQSize)
    {
        // the failed return code means the queue size is too small to buffer up the packets 
        // needed.  We can't do anything about the current queues, but we can increase the 
        // queue size for queues created hereafter.
        
        m_lQueueSize *= 2;

        // if we exceed the max queue size, something must be wrong here, we better disable this 
        // feature for current streams.
        if (m_lQueueSize > MAX_PACKET_BUFFER_QUEUE_SIZE)
        {
            m_bDisableLiveTurboPlay = TRUE;
            NEW_FAST_TEMP_STR(errmsg, 512, strlen(m_pFilename)+256);
            sprintf(errmsg, "Exceeding the max packet queue size %d for %s, the buffering of packets "
                    "is disabled.", MAX_PACKET_BUFFER_QUEUE_SIZE, m_pFilename);
            printf("%s\n", errmsg);
            m_pInfo->m_pProc->pc->error_handler->Report(HXLOG_WARNING, 0, 0, errmsg, 0);
            DELETE_FAST_TEMP_STR(errmsg);
        }
    }

    //Check and remove PacketBufferQueue if duration is greater than max duration
    if (m_pPacketBufferQueue)
    {
        if((ulDuration = m_pPacketBufferQueue->GetDuration()) > m_lMaxDurationOfPacketBufferQueue)
        {
            if (m_pPacketBufferQueue->HandlePacketInvalidTimeStamp())
            {
                if (m_bEnableRSDDebug)
                {
                    NEW_FAST_TEMP_STR(errmsg, 1024, strlen(m_pFilename)+256);
                    sprintf(errmsg, "%s.%03d RSDLive(File: %s) Current Queue with size %d and "
                            "duration %.2f exceeded the max duration %d. Flusing the Current Queue \n",
                            szTime, tNow.tv_usec/1000, m_pFilename, m_pPacketBufferQueue->GetSize(),
                            (float)ulDuration/1000, m_lMaxDurationOfPacketBufferQueue/1000);
                    printf("%s\n", errmsg);
                    fflush(stdout);
                    m_pInfo->m_pProc->pc->error_handler->Report(HXLOG_WARNING, 0, 0, errmsg, 0);
                    DELETE_FAST_TEMP_STR(errmsg);
                }

                HXMutexLock(m_PacketBufferQueueLock, TRUE);
                HX_RELEASE(m_pPacketBufferQueue);
                HXMutexUnlock(m_PacketBufferQueueLock);
            }
        }
        else
        {
            m_pPacketBufferQueue->ResetPacketInvalidTimeStamp();
        }
    }

    if (m_bQTrace)
    {
        if (m_pPacketBufferQueue)
        {
            printf("*** CQ ");
            m_pPacketBufferQueue->Dump();
        }

        if (m_pFuturePacketBufferQueue)
        {
            printf("*** FQ ");
            m_pFuturePacketBufferQueue->Dump();            
        }
    }

    return HXR_OK;
}

HX_RESULT
BroadcastGateway::FixupFileHeader(IHXValues* pFileHeader)
{
    HX_RESULT res = HXR_OK;

    // Ensure that there is a stream group count
    UINT32 ulNumStreamGroups = 0;
    res = pFileHeader->GetPropertyULONG32("StreamGroupCount", ulNumStreamGroups);

    // If there was no stream group count, set it to be the number of logical streams
    if (FAILED(res))
    {
        res = pFileHeader->GetPropertyULONG32("StreamCount", ulNumStreamGroups);

        if (SUCCEEDED(res))
        {
            res = pFileHeader->SetPropertyULONG32("StreamGroupCount", ulNumStreamGroups);
        }
    }

    return res;
}

HX_RESULT
BroadcastGateway::FixupStreamHeader(UINT32 ulLogicalStreamNum, IHXValues* pStreamHeader)
{
    HX_RESULT res = HXR_OK;

    // Ensure that there is a stream group number
    UINT32 ulStreamGroupNum = 0;
    res = pStreamHeader->GetPropertyULONG32("StreamGroupNumber", ulStreamGroupNum);

    // If there was no stream group number, set it equal to the logical stream number
    if (FAILED(res))
    {
        res = pStreamHeader->SetPropertyULONG32("StreamGroupNumber", ulLogicalStreamNum);
    }

    // Ensure that there is a switch group number
    UINT32 ulSwitchGroupNum = 0;
    res = pStreamHeader->GetPropertyULONG32("SwitchGroupID", ulSwitchGroupNum);

    // If there was no switch group number, set it equal to the logical stream number + 1
    // Note that this assumes that all (or none) of the logical streams have a switch group number
    if (FAILED(res))
    {
        res = pStreamHeader->SetPropertyULONG32("SwitchGroupID", ulLogicalStreamNum + 1);
    }

    return res;
}

BOOL 
BroadcastGateway::IsKeyFrameStream(UINT32 uStreamNumber)
{
    return (m_unKeyframeStreamGroup == m_aulLogicalStreamToStreamGroup[uStreamNumber]);
}

BOOL
BroadcastGateway::IsKeyFrameRule(UINT32 uStreamNumber, UINT16 unRule)
{
    if (IsKeyFrameStream(uStreamNumber))
    {
        UINT32 ulSwitchGroupNum = m_aulLogicalStreamToSwitchGroup[uStreamNumber];
        HX_ASSERT(m_ppSwitchGroupRSDInfo[ulSwitchGroupNum]);

        if (m_ppSwitchGroupRSDInfo[ulSwitchGroupNum])
        {
            return m_ppSwitchGroupRSDInfo[ulSwitchGroupNum]->IsKeyFrameRule(unRule);
        }
    }

    return FALSE;
}

BOOL
BroadcastGateway::IsMinRSDQueueDuration(IHXPacket* pPacket)
{
    for(int i=1; i < m_ulSwitchGroupCount+1; i++)
    {
        if (m_ppSwitchGroupRSDInfo[i] && 
            !m_ppSwitchGroupRSDInfo[i]->IsPrerollSatisfied(pPacket, m_lQueueDuration))
        {
            return FALSE;
        }
    }

    return TRUE;
}

void
BroadcastGateway::ResetSwitchGroupRSDInfo()
{
    for(int i=1; i < m_ulSwitchGroupCount+1; i++)
    {
        if (m_ppSwitchGroupRSDInfo[i])
        {
            m_ppSwitchGroupRSDInfo[i]->ResetInfo();
        }
    }
}

/*
  BroadcastPacketManager:
  Handles Broadcast Data Flow on a per Process basis
*/

BroadcastPacketManager::BroadcastPacketManager(Process* pProc, BroadcastGateway* pGateway)
    : m_pProc(pProc)
    , m_pGateway(NULL)
    , m_pStreamQueue(NULL)
    , m_lRefCount(0)
    , m_ulCallbackHandle(0)
    , m_bCleanedUp(FALSE)
#ifdef DEBUG_PKTMGR
    , m_ulCount(0)
#endif
    , m_uNumCallBacks(1)
    , m_uMaxPackets(0)
    , m_fFilter(1.0)
    , m_pCurrentPacket(NULL)
{
    HX_ASSERT(pGateway);
    m_pGateway = pGateway;
    m_pGateway->AddRef();
}

BroadcastPacketManager::~BroadcastPacketManager()
{
    Done();
}

HX_RESULT
BroadcastPacketManager::DispatchPacket(UINT32 uMaxSinkSends, 
                                       REF(UINT32)uTotalSinkSends)
{
    HX_ASSERT(m_pStreamQueue);

    BroadcastStreamer*     pBroadcastStreamer  = NULL;
    IHXPacket*                  pHeadPacket         = NULL;
    Timeval                     tHeadTime;
    UINT8                       bIsStreamDoneMarker = FALSE;
    UINT16                      unStreamNumberDone;
    IHXPacket*                  pSendPacket = NULL;
    HXList_iterator             i(&m_BroadcastStreamerList);

    BOOL                        bResumeInterruptedTransmit = FALSE;
    UINT32                      uSinkSends = 0;

    if (!m_pCurrentPacket)
    {
        if (m_pStreamQueue)
        {
            pHeadPacket = m_pStreamQueue->Dequeue(tHeadTime, 
                bIsStreamDoneMarker, unStreamNumberDone);
        }
    }
    else
    {
        pHeadPacket = m_pCurrentPacket;
        m_pCurrentPacket = NULL;
        bResumeInterruptedTransmit = TRUE;
    }

    /* If we just pulled a StreamDone marker off the queue, send stream dones */
    //XXXDWL TODO: don't destroy/call done() until all streams have streamdone
    if (bIsStreamDoneMarker)
    {
        for (; *i; ++i)
        {
            pBroadcastStreamer = (BroadcastStreamer *)*i;
            if (pBroadcastStreamer && pBroadcastStreamer->m_pSinkControl)
            {
                pBroadcastStreamer->m_pSinkControl->
                StreamDone(unStreamNumberDone);
            }
        }

        return HXR_STREAM_DONE;
    }
    
    if (pHeadPacket == NULL)
    {
        return HXR_UNEXPECTED;
    }
     
    /* iterate over BroadcastStreamers, sending the packet to each */
    for (; *i; ++i)
    {
        pBroadcastStreamer = (BroadcastStreamer *)*i;

        if ((pBroadcastStreamer) && (pBroadcastStreamer->m_pbPacketsStarted) && 
            pBroadcastStreamer->m_pbPacketsStarted[pHeadPacket->GetStreamNumber()])
        {
            if (uTotalSinkSends >= uMaxSinkSends)
            {
                pBroadcastStreamer->m_bNeedXmit = TRUE;
                if (!m_pCurrentPacket)
                {
                    m_pCurrentPacket = pHeadPacket;
                }
            }
            else if (!bResumeInterruptedTransmit || pBroadcastStreamer->m_bNeedXmit)
            {
                /* store ptr to the packet if BroadcastStreamer NULLs it */
                pSendPacket = pHeadPacket;
                pSendPacket->AddRef();

                if (!(m_uTotalSinkSends % 40) && !bResumeInterruptedTransmit)
                {
                    Timeval tLatency = m_pProc->pc->engine->now - tHeadTime;
                    UINT32 msDiff = tLatency.tv_sec * 1000 + tLatency.tv_usec / 1000;

                    m_pGateway->LatencyCalc(msDiff);
                }
                m_uTotalSinkSends++;

                pBroadcastStreamer->SendPacket(pSendPacket); 

                pBroadcastStreamer->m_bNeedXmit = FALSE;
                HX_RELEASE(pSendPacket);

                uTotalSinkSends++;
                uSinkSends++;
           }
       }
   }

    if (!m_pCurrentPacket)
    {
        if (!uSinkSends)
        {
            // if we get here, either there aren't any streamers or none of them
            // have started sending packets yet. Consume this packet.
            uTotalSinkSends++;
        }

        HX_RELEASE(pHeadPacket);
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastPacketManager::Func()
{
    m_ulCallbackHandle = 0;

    if (m_bCleanedUp || !m_pStreamQueue)
    {
        return HXR_OK;
    }

    int iSec, iUSec;
    UINT32 uPacketsSent = 0;
    float fTargetIDT;
    HX_RESULT hr = HXR_OK;
    BOOL bKeepSending = TRUE;
    UINT32 uMaxPackets;
    float fDepth = m_pStreamQueue->QueueDepthInSeconds();
    float fMultiplier;
    float fBase = m_pGateway->m_fx * (m_pGateway->m_fy * fDepth);

    fMultiplier = 1.0 + pow(fBase, m_pGateway->m_fz);
    uMaxPackets = (UINT32)(fMultiplier * m_fAvgPktsPerCallback);

    if (uMaxPackets < 10)
    {
        uMaxPackets = 10;
    }

#if 0  // not tested ... might help prevent q growth in the first place
    if (uMaxPackets < (m_fAvgPktsPerCallback + 2))
    {
       uMaxPackets = m_fAvgPktsPerCallback + 2; 
    }
#endif

#ifdef DEBUG_PKTMGR
    if (m_ulCount >= 75)
    {

        Timeval tTemp = m_pProc->pc->engine->now - m_tLastDepartureTime;

        HXMutexLock(m_pGateway->m_InterArrivalTimeLock, TRUE);
        printf("%p, %ld.%06ld, fAvgIAT %f QS %f QP %d "
               "m %f, max pkts %d AvgPkts %f\n", 
               this, m_pProc->pc->engine->now.tv_sec, m_pProc->pc->engine->now.tv_usec, 
               m_pGateway->m_fAvgInterArrivalTime, fDepth,  
               m_pStreamQueue->QueueDepth(), fMultiplier, 
               uMaxPackets, m_fAvgPktsPerCallback);

        m_pszDebug[m_ulCount] = '\n';
        m_pszDebug[m_ulCount+1] = '\0';
        printf("%s\n", m_pszDebug);
        HXMutexUnlock(m_pGateway->m_InterArrivalTimeLock);

        m_ulCount = 0;
    }
#endif   //  DEBUG_PKTMGR

    m_tLastDepartureTime = m_pProc->pc->engine->now;

    if (m_pStreamQueue->Empty())
    {
#ifdef DEBUG_PKTMGR
        if (m_ulCount < 200)
        {
            m_pszDebug[m_ulCount++] = 'o';
        }
#endif   //  DEBUG_PKTMGR

        goto ScheduleNext;
    }

    //XXXDWL TODO: record relevant stats

    while ((uPacketsSent < uMaxPackets) && (hr == HXR_OK) && 
           !m_pStreamQueue->Empty())
    {

        hr = DispatchPacket(uMaxPackets, uPacketsSent);

#ifdef DEBUG_PKTMGR
        if (m_ulCount < 200)
        {
            sprintf(m_pszDebug + m_ulCount, " %d ", uPacketsSent);
            m_ulCount += strlen(m_pszDebug + m_ulCount);
        }
        if (hr != HXR_OK)
        {
            m_pszDebug[m_ulCount++] = '?';
        }
        
        if (m_pStreamQueue->Empty())
        {
            m_pszDebug[m_ulCount++] = 'o';
        }
        else
        {   
            m_pszDebug[m_ulCount++] = 'x';
        }

#endif   //  DEBUG_PKTMGR
    }

ScheduleNext:

    if (m_uNumCallBacks <= 100)
    {
        m_fFilter = (float)(1.0 / m_uNumCallBacks);
        m_uNumCallBacks++;
    }

    m_fAvgPktsPerCallback = (m_fAvgPktsPerCallback != 0) ?
                ((1 - m_fFilter) * m_fAvgPktsPerCallback +
                m_fFilter * (float)uPacketsSent) :
                (float)uPacketsSent;

    m_tNextSchedDepartureTime = m_pProc->pc->engine->now + Timeval(0,10000);

    m_ulCallbackHandle = m_pProc->pc->engine->ischedule.enter(
                                               m_tNextSchedDepartureTime, this);

    return HXR_OK;
}

void 
BroadcastPacketManager::Init(Process* pProc)
{
    HX_ASSERT(pProc);

    if (m_pStreamQueue == NULL)
    {
        m_pStreamQueue = new BroadcastStreamQueue;
    }

    m_pStreamQueue->Init(this);

    m_tLastDepartureTime = Timeval(0.0);
    m_tNextSchedDepartureTime = Timeval(0.0);
    
    /* jumpstart the scheduler */
    if (pProc->procnum() == m_pProc->procnum())
    {
        /* we're in the correct proc */
        Func();
    }
    else
    {
        /* wrong proc; pass control to the streamer proc  */
        JumpStartCallback* pJumpStartCallback = new JumpStartCallback;
        pJumpStartCallback->m_pPacketManager = this;
        pProc->pc->dispatchq->send(pProc, pJumpStartCallback, 
            m_pProc->procnum());
    }
}

void
BroadcastPacketManager::JumpStartCallback::func(Process* pProc)
{
    m_pPacketManager->Func();
    delete this;
}

void 
BroadcastPacketManager::Done()
{
    if (m_bCleanedUp)
    {
        return;
    }

    BroadcastPacketListEntry* pPacket = NULL;
    BroadcastStreamer* pStreamer = NULL;
    
    if (m_ulCallbackHandle)
    {
        m_pProc->pc->engine->ischedule.remove(m_ulCallbackHandle);
        m_ulCallbackHandle = 0;
    }

    while (pStreamer = 
       (BroadcastStreamer*)m_BroadcastStreamerList.remove_head())
    {
        HX_RELEASE(pStreamer);
    }

    HX_DELETE(m_pStreamQueue);
    
    HX_RELEASE(m_pGateway);

    if (m_pCurrentPacket)
    {
        HX_RELEASE(m_pCurrentPacket);
    }

    m_bCleanedUp = TRUE;
}

void
BroadcastPacketManager::QueuePacket(IHXPacket* pPacket)
{    
    if (m_pStreamQueue)
    {
        m_pStreamQueue->Enqueue(pPacket);
    }
}

void
BroadcastPacketManager::SendStreamDone(UINT16 unStreamNumber)
{
    if (m_pStreamQueue)
    {
        m_pStreamQueue->TerminateQueue(unStreamNumber);
    }
}

void 
BroadcastPacketManager::AddBroadcastStreamer(BroadcastStreamer* pStreamer)
{
    pStreamer->AddRef();
    m_BroadcastStreamerList.insert(pStreamer);
}

void 
BroadcastPacketManager::RemoveBroadcastStreamer(BroadcastStreamer* pStreamer)
{
    pStreamer->Release();
    m_BroadcastStreamerList.remove(pStreamer);
}

STDMETHODIMP
BroadcastPacketManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();                                                    
        *ppvObj = (IUnknown*)this;
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

ULONG32
BroadcastPacketManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32
BroadcastPacketManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/* BroadcastStreamQueue implementation */

BroadcastStreamQueue::BroadcastStreamQueue()
    : m_pParent(NULL)
    , m_pQueue(NULL)
    , m_uHead(0)
    , m_uTail(0)
    , m_uMaxSize(0)
    , m_uReaders(0)
    , m_uWriters(0)
{
}

BroadcastStreamQueue::~BroadcastStreamQueue()
{
    HX_ASSERT(m_uWriters == 0);
    HX_ASSERT(m_uReaders == 0);

    while (m_uHead != m_uTail)
    {
        HX_RELEASE(m_pQueue[m_uHead].m_pPacket);
        m_pQueue[m_uHead].m_bIsStreamDoneMarker = FALSE;
        m_uHead = (m_uHead + 1) % m_uMaxSize;
    }
    
    HX_DELETE(m_pQueue);
    HX_RELEASE(m_pParent);
}

void
BroadcastStreamQueue::Init(BroadcastPacketManager* pParent)
{
    HX_RELEASE(m_pParent);
    
    if (pParent)
    {
        m_pParent = pParent;
        m_pParent->AddRef();
    }

    m_pQueue = (BroadcastPacketListEntry *) new 
                                  BroadcastPacketListEntry[BCAST_QUEUE_MAX_SIZE];

    memset(m_pQueue, 0, BCAST_QUEUE_MAX_SIZE * sizeof(BroadcastPacketListEntry));

    m_uMaxSize = BCAST_QUEUE_MAX_SIZE;
}

BOOL
BroadcastStreamQueue::Empty()
{
    return (m_uHead == m_uTail);
}

void
BroadcastStreamQueue::TerminateQueue(UINT16 unStreamNumber)
{
    InterlockedIncrement(&m_uWriters);
    HX_ASSERT(m_uWriters == 1);

    /* Place a stream done marker in the packet Queue */
    
    UINT32 uNewTail = (m_uTail + 1) % m_uMaxSize;

    if (uNewTail != m_uHead)
    {
        m_pQueue[m_uTail].m_tTime = m_pParent->m_pProc->pc->engine->now;
        m_pQueue[m_uTail].m_unStreamNumberDone = unStreamNumber;
        m_pQueue[m_uTail].m_bIsStreamDoneMarker = TRUE;
        m_pQueue[m_uTail].m_pPacket = NULL;
        m_uTail = uNewTail;
    }
    else
    {
        // very unlikely to happen, should probably bump the last entry
        // off the queue and stick the stream done on
        (*g_pBroadcastPacketsDropped)++;
    }
    
    InterlockedDecrement(&m_uWriters);
}

HX_RESULT
BroadcastStreamQueue::Enqueue(IHXPacket* pPacket)
{
    
    InterlockedIncrement(&m_uWriters);
    HX_ASSERT(m_uWriters == 1);

    Timeval tHeadTime   = m_pParent->m_pProc->pc->engine->now;
    
    /* Queue this packet if the queue occupancy isn't too long */
    // Note that if we're getting behind this drops the newest
    // packets rather than the oldest which seems backwards.

    UINT32 uNewTail = (m_uTail + 1) % m_uMaxSize;

    if (m_uTail != m_uHead)
    {
        tHeadTime = m_pQueue[m_uHead].m_tTime;
    }
    
    if ((uNewTail == m_uHead) || 
            ((m_pParent->m_pProc->pc->engine->now - tHeadTime) >= 
             m_pParent->m_pGateway->m_tMaxQueueOccupancy) )
    {
        (*g_pBroadcastPacketsDropped)++;
        InterlockedDecrement(&m_uWriters);
        return HXR_FAIL;
    }

    m_pQueue[m_uTail].m_tTime = m_pParent->m_pProc->pc->engine->now;
    m_pQueue[m_uTail].m_bIsStreamDoneMarker = FALSE;
    m_pQueue[m_uTail].m_pPacket = pPacket;
    pPacket->AddRef();

    m_uTail = uNewTail;
    InterlockedDecrement(&m_uWriters);
    return HXR_OK;
}

IHXPacket*
BroadcastStreamQueue::Dequeue(Timeval&  tHead,
                              UINT8&    bIsStreamDoneMarker,
                              UINT16&   unStreamNumberDone)
{
    InterlockedIncrement(&m_uReaders);
    HX_ASSERT(m_uReaders == 1);

    if (m_uHead == m_uTail)
    {
        // empty queue
        return NULL;
    }

    tHead = m_pQueue[m_uHead].m_tTime;
    unStreamNumberDone = m_pQueue[m_uHead].m_unStreamNumberDone;
    bIsStreamDoneMarker = m_pQueue[m_uHead].m_bIsStreamDoneMarker;

    IHXPacket* pPacket = m_pQueue[m_uHead].m_pPacket;
    m_uHead = (m_uHead + 1) % m_uMaxSize;

    // caller inherits queue's AddRef()
    InterlockedDecrement(&m_uReaders);
    return pPacket;
}

//XXX FIXME
float
BroadcastStreamQueue::QueueDepthInSeconds()
{
    float fSecsInQ = 0;
    Timeval tHead;

    if (m_uHead != m_uTail)
    {
        tHead = m_pQueue[m_uHead].m_tTime;
    }

    fSecsInQ = m_pParent->m_pProc->pc->engine->now.tv_sec - tHead.tv_sec;
    fSecsInQ += (float)(m_pParent->m_pProc->pc->engine->now.tv_usec -
            tHead.tv_usec) / 1000000.0;

    if (fSecsInQ > 0)
    {
        return (fSecsInQ);
    }
    else
    {
        return (0.0);
    }
}

/* Broadcast Info  */
BroadcastInfo::BroadcastInfo(Process* pProc)
{
    // We need to provide a lock to synch gateway lookups from the
    // m_pCurrentBroadcasts map so we don't create and use multiple
    // gateways for the same feed
    m_GatewayLock = HXCreateMutex();
    m_pCurrentBroadcasts = new CHXMapStringToOb;
    m_pCurrentBroadcasts->SetCaseSensitive(FALSE);
    m_pProc = pProc;
}

BroadcastInfo::~BroadcastInfo()
{
    PANIC(("BroadcastInfo should never destruct\n"));
}

//CPacketBufferQueue

CPacketBufferQueue::CPacketBufferQueue(UINT32 ulQueueSize, BOOL bTrace, UINT32 ulReservedForAudioPacket)
{
    m_ulRefCount = 0;
    m_ulInsertPosition = ulReservedForAudioPacket;
    m_ulStartingPosition = ulReservedForAudioPacket;
    m_ulQueueSize = ulQueueSize;
    m_PacketBufferQueue = new IHXPacket*[m_ulQueueSize];
    m_ulQueueBytes = 0;
    m_ulStartTS = 0;
    m_ulInvalidPacketCount = 0;
    m_bTrace = bTrace;
    m_pKeyFrame = NULL;
}

CPacketBufferQueue::~CPacketBufferQueue()
{
    for (int i = m_ulStartingPosition; i < m_ulInsertPosition; i++)
    {
        m_PacketBufferQueue[i]->Release();
    }
    delete m_PacketBufferQueue;
    HX_RELEASE(m_pKeyFrame);
}

HX_RESULT
CPacketBufferQueue::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();                                                    
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXLivePacketBufferQueue))
    {
        AddRef();                                                    
        *ppvObj = (IHXLivePacketBufferQueue*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


ULONG32
CPacketBufferQueue::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


ULONG32
CPacketBufferQueue::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

UINT32
CPacketBufferQueue::GetDuration()
{
    IHXPacket* pTail = NULL;
    HX_RESULT theErr = HXR_OK;
    UINT ulDuration = 0;
    INT32 timeDiff = 0;

    theErr = GetPacket(m_ulInsertPosition - 1, pTail);
    if (SUCCEEDED(theErr))
    {
        timeDiff = pTail->GetTime() - m_ulStartTS;
        ulDuration = (timeDiff < 0) ? 0 : timeDiff;
    }

    HX_RELEASE(pTail);

    return ulDuration;
}

void
CPacketBufferQueue::Dump()
{
    //    printf("CPacketBufferQueue: %p ", this);
    printf("MaxSz=%u ", m_ulQueueSize);    
    printf("ActSz=%u ", m_ulInsertPosition);
    printf("Bytes=%u ", m_ulQueueBytes);

    HX_RESULT theErr = HXR_OK;
    IHXPacket* pHead = NULL;
    IHXPacket* pTail = NULL;

    if (SUCCEEDED(theErr))
    {
        theErr = GetPacket(0, pHead);
    }
    if (SUCCEEDED(theErr))
    {    
        theErr = GetPacket(m_ulInsertPosition - 1, pTail);
    }
    if (SUCCEEDED(theErr))
    {
        printf("H=%u T=%u ", pHead->GetTime(), pTail->GetTime());
        printf("D=%u", pTail->GetTime() - pHead->GetTime());
    }
    printf("\n");

    for(UINT32 i = m_ulStartingPosition; i < m_ulInsertPosition; i++)
    {
        DumpPacket(m_PacketBufferQueue[i]);
    }
    fflush(0);
}

HX_RESULT 
CPacketBufferQueue::EnQueue(IHXPacket*  pPacket)
{
    if (m_ulInsertPosition >= m_ulQueueSize)
    {
        //we can't grow the queue because of race conditions 
        // between Enqueue and GetPacket, so we just stop enqueuing
        // the packet and wait for next keyframe.
        // we should set the the queue size big enough for this not to happen

        return HXR_FAIL;
    }

    m_PacketBufferQueue[m_ulInsertPosition] = pPacket;
    pPacket->AddRef();
    m_ulInsertPosition++;

    if (m_bTrace)
    {
        IHXBuffer* pBuf = pPacket->GetBuffer();    
        m_ulQueueBytes += pBuf->GetSize();
        pBuf->Release();    
    }
    return HXR_OK;
}

HX_RESULT 
CPacketBufferQueue::AddKeyframe(IHXPacket*  pPacket)
{
    HX_RESULT rc = HXR_OK;
    m_ulStartTS = pPacket->GetTime();
    m_pKeyFrame = pPacket;
    m_pKeyFrame->AddRef();
    rc = EnQueue(pPacket);
    HX_ASSERT(rc == HXR_OK);

    return rc;
}

HX_RESULT
CPacketBufferQueue::GetPacket(UINT32 ulIndex, IHXPacket*&  pPacket)
{
    if (ulIndex + m_ulStartingPosition < m_ulInsertPosition)
    {
        pPacket = m_PacketBufferQueue[ulIndex + m_ulStartingPosition];
        pPacket->AddRef();
        return HXR_OK;
    }
    return HXR_FAIL;
}

UINT32
CPacketBufferQueue::GetSize()
{
    return m_ulInsertPosition - m_ulStartingPosition;
}

IHXPacket* CPacketBufferQueue::GetKeyFramePacket()
{
    if(m_pKeyFrame)
    {
        m_pKeyFrame->AddRef();
    }
    return m_pKeyFrame;
}

void CPacketBufferQueue::AddAudioPacket(IHXPacket*  pPacket)
{
    m_ulStartingPosition--;
    m_PacketBufferQueue[m_ulStartingPosition] = pPacket;
    pPacket->AddRef();

    // if pPacket is in the queue, we need to delete it and move the q up
    for(UINT32 i = m_ulStartingPosition + 1; i < m_ulInsertPosition; i++)
    {
        if(m_PacketBufferQueue[i] == pPacket)
        {
            m_PacketBufferQueue[i]->Release();
            memmove(m_PacketBufferQueue + i, m_PacketBufferQueue + i + 1, m_ulInsertPosition - 1 -i);
            m_ulInsertPosition--;
        }
    }
}


// SwitchGroupRSDInfo methods
void
SwitchGroupRSDInfo::SetKeyFrameRule(UINT16 ulRuleNum, BOOL bIsKeyFrameRule)
{
    HX_ASSERT(ulRuleNum < m_unNumRules);
    m_pbIsKeyFrameRule[ulRuleNum] = bIsKeyFrameRule;
}

void
SwitchGroupRSDInfo::SetInterDependency(UINT16 ulRuleNum, INT16 lInterDepend)
{
    HX_ASSERT(ulRuleNum < m_unNumRules);
    HX_ASSERT(lInterDepend < m_unNumRules);

    m_plInterDependent[ulRuleNum] = lInterDepend;
}

void
SwitchGroupRSDInfo::OnKeyFramePacket(IHXPacket* pPacket, UINT16 unRule)
{
    HX_ASSERT(m_pbIsKeyFrameRule[unRule]);

    if (!m_bAllKeyFramesReceived && !m_pbKeyFrameReceived[unRule])
    {
        m_pbKeyFrameReceived[unRule] = TRUE;
        m_ulLastKeyFrameTS = pPacket->GetTime();

        //For 3GP live, key frame can be received on either of the two rules.
        //So receving a key frame packet on either of the two interdependent 
        //rules is sufficient.
        if (m_plInterDependent[unRule] != -1)
        {
            INT32 lInterDependentRule = m_plInterDependent[unRule];
            HX_ASSERT(lInterDependentRule < m_unNumRules);
            m_pbKeyFrameReceived[lInterDependentRule] = TRUE;
        }

        //Check if we recevied key frame packets for all the key frame rules.
        m_bAllKeyFramesReceived = TRUE;
        for (int i=0; i < m_unNumRules; i++)
        {
            if (m_pbIsKeyFrameRule[i] &&
                !m_pbKeyFrameReceived[i])
            {
                m_bAllKeyFramesReceived = FALSE;
                break;
            }
        }
    }
}

BOOL
SwitchGroupRSDInfo::IsPrerollSatisfied(IHXPacket* pPacket, INT32 lPreroll)
{
    BOOL bResult = FALSE;

    if (m_bAllKeyFramesReceived)
    {
        UINT32 ulCurTime = pPacket->GetTime();

        if (ulCurTime >= m_ulLastKeyFrameTS)
        {
            bResult = (ulCurTime - m_ulLastKeyFrameTS) >= lPreroll; 
        }
        else if (m_ulLastKeyFrameTS - ulCurTime > 0x7f000000)
        {
            bResult = (MAX_UINT32 - m_ulLastKeyFrameTS + ulCurTime) >= lPreroll;
        }
    }

    return bResult;
}

void
SwitchGroupRSDInfo::ResetInfo()
{
    m_bAllKeyFramesReceived = FALSE;
    m_ulLastKeyFrameTS = 0;
    memset(m_pbKeyFrameReceived, 0, sizeof(BOOL) * m_unNumRules);
}

BOOL 
SwitchGroupRSDInfo::IsKeyFrameRule(UINT16 unRule)
{
    HX_ASSERT(unRule < m_unNumRules);
    return m_pbIsKeyFrameRule[unRule];
}

//AudioSwitchGroupInfo////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
AudioSwitchGroupInfo::AudioSwitchGroupInfo(UINT32 ulNumOfRules)
{
    m_ulNumOfRules = ulNumOfRules;
    m_ppPackets= new IHXPacket*[ulNumOfRules];
    memset(m_ppPackets, 0, sizeof(IHXPacket*) * ulNumOfRules);
    m_pVideoKeyframe = NULL;
    m_pOnDepend = new BOOL[ulNumOfRules];
    memset(m_pOnDepend, FALSE, sizeof(BOOL) * ulNumOfRules);
    m_pInterDepend = new UINT16[ulNumOfRules];
    memset(m_pInterDepend, -1, sizeof(UINT16) * ulNumOfRules);
}

AudioSwitchGroupInfo::~AudioSwitchGroupInfo()
{
    Reset();
    HX_DELETE(m_ppPackets);
    HX_DELETE(m_pOnDepend);
    HX_DELETE(m_pInterDepend);
}

void AudioSwitchGroupInfo::SetVideoKeyframePacket(IHXPacket* pVideoKeyFrame)
{
    HX_RELEASE(m_pVideoKeyframe);
    m_pVideoKeyframe = pVideoKeyFrame;
    pVideoKeyFrame->AddRef();
}

void AudioSwitchGroupInfo::OnPacket(IHXPacket* pPacket)
{
   if(!IsEarlier(pPacket, m_pVideoKeyframe))
   {
       return;
   }

   UINT16 ulRule = pPacket->GetASMRuleNumber(); 

   if(m_pOnDepend[ulRule])
   {
       return;
   }

   if(m_ppPackets[ulRule])
   {
       //we want the packets as close to the keyframe as possible
       if(!IsEarlier(pPacket, m_ppPackets[ulRule]))
       {
           HX_RELEASE(m_ppPackets[ulRule]);
           m_ppPackets[ulRule] = pPacket;
           m_ppPackets[ulRule]->AddRef();
       }
   }
   else if(m_pInterDepend[ulRule] != -1 && m_ppPackets[m_pInterDepend[ulRule]])
   {
        // for inter-dependent rules, we will keep the best packet between them.
        // here "best" means earlier than the video keyframe, and the closest to
        // it.
        if(!IsEarlier(pPacket, m_ppPackets[m_pInterDepend[ulRule]]))
        {
            m_ppPackets[ulRule] = pPacket;
            m_ppPackets[ulRule]->AddRef();

            HX_RELEASE(m_ppPackets[m_pInterDepend[ulRule]]);
        }
   }
   else
   {
        m_ppPackets[ulRule] = pPacket;
        m_ppPackets[ulRule]->AddRef();
   }
}

void AudioSwitchGroupInfo::Reset()
{
    for(UINT16 i = 0; i < m_ulNumOfRules; i++)
    {
        HX_RELEASE(m_ppPackets[i]);
    }
    HX_RELEASE(m_pVideoKeyframe);
}

BOOL AudioSwitchGroupInfo::IsEarlier(IHXPacket* pPacket, IHXPacket* pPacketCompared)
{
    if(pPacket->GetTime() > pPacketCompared->GetTime())
    {
        return FALSE;
    }

    if(((ServerPacket*)pPacket)->GetMediaTimeInMs() >
        ((ServerPacket*)pPacketCompared)->GetMediaTimeInMs() )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL AudioSwitchGroupInfo::HasAllAudioPackets()
{
    for(UINT16 i = 0; i < m_ulNumOfRules; i++)
    {
        //if the rule depend on other rules, we can start with the packets from 
        //this rule. So we just skip this rule.
        if(m_pOnDepend[i])
        {
            continue;
        }

        if(!m_ppPackets[i] && (m_pInterDepend[i] == -1 || !m_ppPackets[m_pInterDepend[i]]))
        {
            return FALSE;
        }
    }
    return TRUE;
}

void AudioSwitchGroupInfo::GetAudioPackets(IHXPacket**& ppPackets, UINT32& ulCount)
{
    ppPackets = m_ppPackets;
    ulCount = m_ulNumOfRules;
}

UINT32 AudioSwitchGroupInfo::GetNumOfPacket()
{
    return m_ulNumOfRules;
}


void AudioSwitchGroupInfo::SetOnDepend(UINT16 ulRuleNum, BOOL bOnDepend)
{
    m_pOnDepend[ulRuleNum] = bOnDepend;
}

void AudioSwitchGroupInfo::SetInterDepend(UINT16 unRuleNum, UINT16 nInterDepend)
{
    m_pInterDepend[unRuleNum] = nInterDepend;
}

BOOL BroadcastGateway::HasAllAudioPackets()
{
    for(UINT32 i = 1; i < m_ulSwitchGroupCount+1; i++)
    {
        if(m_ppAudioSwitchGroupInfo[i])
        {
            if(m_ppAudioSwitchGroupInfo[i]->HasAllAudioPackets() == FALSE)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

UINT32 BroadcastGateway::GetNumOfReservedAudioPackets()
{
    if(!m_ppAudioSwitchGroupInfo)
    {
        return 0;
    }

    UINT32 ulTotal = 0;
    for(UINT32 i = 1; i < m_ulSwitchGroupCount+1; i++)
    {
        if(m_ppAudioSwitchGroupInfo[i])
        {
            ulTotal += m_ppAudioSwitchGroupInfo[i]->GetNumOfPacket();
        }
    }
    return ulTotal;
}

