/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bcastmgr.cpp,v 1.59 2007/02/21 17:45:35 srao Exp $ 
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
#define RSD_DEFAULT_EXTRA_PREROLL_PER 0
#define DEFAULT_PACKET_BUFFER_QUEUE_SIZE 8096
#define MAX_PACKET_BUFFER_QUEUE_SIZE 65536
#define DEFAULT_PACKET_BUFFER_QUEUE_DURATION 1000
#define RSD_MAX_DURATION_PACKET_BUFFER_QUEUE "config.LiveReducedStartupDelay.MaxDurationOfRSDPacketBufferQueue"
#define DEFAULT_MAX_DURATION_OF_RSD_PACKET_BUFFER_QUEUE 70
//#define RSD_LIVE_DEBUG


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
                            BOOL                    bBlocking,
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

    BroadcastStreamer_Base* pBroadcastStreamer = NULL;
    if (bBlocking)
    {
    pBroadcastStreamer = 
        new BroadcastStreamer_Blocking (pInfo, pFilename, pStreamerProc, 
                                            pSessionStats);
    }
    else
    {
    pBroadcastStreamer = 
        new BroadcastStreamer_Nonblocking (pInfo, pFilename, pStreamerProc,
                                               pSessionStats);
    }

    HX_ASSERT(pBroadcastStreamer);

    pBroadcastStreamer->
    QueryInterface(IID_IHXPSourceControl, (void**)&pControl);

    return HXR_OK;
}


BroadcastStreamer_Base::BroadcastStreamer_Base(BroadcastInfo*   pInfo,
                                               const char*      pFilename,
                                               Process*         pStreamerProc,
                                               IHXSessionStats* pSessionStats)
    : m_lRefCount(0)
    , m_pSessionStats(pSessionStats)
{
    m_pSinkControl                      = 0;
    m_pInfo                             = pInfo;
    m_bGatewayReady                     = FALSE;
    m_pInitPending                      = FALSE;
    m_GatewayStatus                     = HXR_FAIL;
    m_bSourceAborted                    = FALSE;
    m_ulGatewayCheckCBHandle            = 0;
    m_ulStreamDoneCBHandle              = 0;
    m_pProc                             = pStreamerProc;
    m_nRegEntryIndex                    = -1;
    m_pbPacketsStarted                  = NULL;

    m_bNeedXmit = FALSE;

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
}

BroadcastStreamer_Base::~BroadcastStreamer_Base()
{
    HX_ASSERT(!m_pSessionStats);
}

STDMETHODIMP        
BroadcastStreamer_Base::GatewayCheckCallback::Func()
{
    m_pBS->GatewayCheck();
    return HXR_OK;
}

void BroadcastStreamer_Base::GatewayCheck()
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


HX_RESULT
BroadcastStreamer_Base::QueryInterface(REFIID riid, void** ppvObj)
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
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


ULONG32
BroadcastStreamer_Base::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


ULONG32
BroadcastStreamer_Base::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
BroadcastStreamer_Base::Init(IHXPSinkControl* pSink)
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
BroadcastStreamer_Base::Done()
{
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
BroadcastStreamer_Base::UpdateRegClientsLeaving()
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

HX_RESULT
BroadcastStreamer_Base::GetFileHeader(IHXPSinkControl* pSink)
{
    ASSERT(pSink == m_pSinkControl);

    if (m_pGateway->m_pFileHeader)
    {
        m_pSinkControl->FileHeaderReady(HXR_OK, m_pGateway->m_pFileHeader);
    }
    else
    {
        ASSERT(0);
        m_pSinkControl->FileHeaderReady(HXR_FAIL, NULL);
        return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT
BroadcastStreamer_Base::GetStreamHeader(IHXPSinkControl* pSink,
                    UINT16 unStreamNumber)
{
    ASSERT(pSink == m_pSinkControl);

    if (m_pGateway->m_pStreamHeaders && 
        m_pGateway->m_pStreamHeaders[unStreamNumber])
    {
        m_pSinkControl->StreamHeaderReady(HXR_OK,
        m_pGateway->m_pStreamHeaders[unStreamNumber]);
    }
    else
    {
        ASSERT(0);
        m_pSinkControl->StreamHeaderReady(HXR_FAIL, NULL);
        return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT
BroadcastStreamer_Base::Seek(UINT32          ulSeekTime)
{
    return HXR_FAIL;
}

BOOL
BroadcastStreamer_Base::IsLive()
{
    return TRUE;
}

HX_RESULT
BroadcastStreamer_Base::SetLatencyParams(UINT32 ulLatency,
                    BOOL bStartAtTail,
                    BOOL bStartAtHead)
{
    /* XXXDWL TODO */
    return HXR_OK;
}


/* IHXASMSource */  
STDMETHODIMP
BroadcastStreamer_Base::Subscribe(UINT16 uStreamNumber, UINT16 uRuleNumber) 
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
        if ( m_pGateway->m_ppRuleData[uStreamNumber] && 
            (uStreamNumber == m_pGateway->m_unKeyframeStream) && 
                m_pGateway->m_pbIsSubscribed)
        {
            m_pGateway->m_pbIsSubscribed[uRuleNumber] = TRUE;
        }

        HXMutexUnlock(m_pGateway->m_RuleDataLock);
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
    
    return HXR_OK;
}
    
STDMETHODIMP
BroadcastStreamer_Base::Unsubscribe(UINT16 uStreamNumber, UINT16 uRuleNumber) 
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

    
    if (bASMupdate)
    {
        if (m_pGateway->m_ppRuleData[uStreamNumber] && 
            (uStreamNumber == m_pGateway->m_unKeyframeStream) && 
            m_pGateway->m_pbIsSubscribed )
        {
            m_pGateway->m_pbIsSubscribed[uRuleNumber] = FALSE;
        }
        HXMutexUnlock(m_pGateway->m_RuleDataLock);    
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
    
    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer_Base::GetPacketBufferQueue(UINT16 strmNum,
                                             UINT16 ruleNum,
                                             IHXLivePacketBufferQueue*& pQueue)
{
    pQueue = m_pGateway->GetPacketBufferQueue(strmNum, ruleNum);
    if (pQueue)
    {
        return HXR_OK;
    }
    return HXR_FAIL;
}
STDMETHODIMP        
BroadcastStreamer_Base::StreamDoneCallback::Func()
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
BroadcastStreamer_Base::CreateRegEntries()
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

/* Nonblocking Broadcast Streamer implementation: */
BroadcastStreamer_Nonblocking::BroadcastStreamer_Nonblocking(
                                                BroadcastInfo*   pInfo, 
                                                const char*      pFilename,
                                                Process*         pStreamerProc,
                                                IHXSessionStats* pSessionStats)
    : BroadcastStreamer_Base(pInfo, pFilename, pStreamerProc, pSessionStats)
    , m_pSinkPackets (NULL)
{
    BroadcastStreamer_Base::GatewayCheck();
}

BroadcastStreamer_Nonblocking::~BroadcastStreamer_Nonblocking()
{
}

STDMETHODIMP
BroadcastStreamer_Nonblocking::Done()
{
    HX_RELEASE(m_pSinkPackets);
    return BroadcastStreamer_Base::Done();
}    

HX_RESULT 
BroadcastStreamer_Nonblocking::SendPacket(IHXPacket* pPacket)
{
    return (m_pSinkPackets) ?
        m_pSinkPackets->PacketReady(HXR_OK, pPacket) : HXR_FAIL;
}

STDMETHODIMP
BroadcastStreamer_Nonblocking::Init(IHXPSinkPackets*       pSinkPackets)
{
    HX_RELEASE(m_pSinkPackets);
    pSinkPackets->AddRef();
    m_pSinkPackets = pSinkPackets;

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer_Nonblocking::StartPackets(UINT16 unStreamNumber)
{
    HX_ASSERT(unStreamNumber <= m_pGateway->m_ulStreamCount);

    if (m_pbPacketsStarted)
    {
        m_pbPacketsStarted[unStreamNumber] = TRUE;
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer_Nonblocking::StopPackets(UINT16 unStreamNumber)
{
    HX_ASSERT(unStreamNumber <= m_pGateway->m_ulStreamCount);

    if (m_pbPacketsStarted)
    {
        m_pbPacketsStarted[unStreamNumber] = FALSE;
    }

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer_Nonblocking::Resync()
{
    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer_Nonblocking::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXPSourceLivePackets))
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

    return BroadcastStreamer_Base::QueryInterface(riid, ppvObj);
}

ULONG32
BroadcastStreamer_Nonblocking::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


ULONG32
BroadcastStreamer_Nonblocking::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/* Blocking Broadcast Streamer implementation: */
BroadcastStreamer_Blocking::BroadcastStreamer_Blocking (
                                                BroadcastInfo*   pInfo, 
                                                const char*      pFilename,
                                                Process*         pStreamerProc,
                                                IHXSessionStats* pSessionStats)
    : m_pSink (NULL)
    , m_ppQueue (NULL)
    , m_ulSyncTime (0)
    , m_bBlockSync (FALSE)
    , BroadcastStreamer_Base (pInfo, pFilename, pStreamerProc, pSessionStats)
{
    m_QueueLock = HXCreateMutex();
    m_SyncLock = HXCreateMutex();
    HX_ASSERT(m_pGatewayCheckCallback);

    m_pGatewayCheckCallback->m_pBS = this;
    GatewayCheck();
}

BroadcastStreamer_Blocking::~BroadcastStreamer_Blocking()
{
    HXDestroyMutex(m_QueueLock);
    HXDestroyMutex(m_SyncLock);
}

void 
BroadcastStreamer_Blocking::GatewayCheck()
{
    m_ulGatewayCheckCBHandle = 0;

    HXMutexLock(m_pGateway->m_StateLock, TRUE);

    if (m_pGateway->m_State == BroadcastGateway::STREAMING)
    {
        HXMutexUnlock(m_pGateway->m_StateLock);

    m_GatewayStatus = HXR_OK;

    UINT32 ulQueueSize = BCAST_DEFAULT_MAX_CC_QUEUE;
    INT32 lTemp = 0;
    
    if (SUCCEEDED(m_pProc->pc->registry->
              GetInt("config.BroadcastCongestionQueueSize", &lTemp, m_pProc)))
    {
        ulQueueSize = (UINT32)lTemp;
    }

    HXMutexLock(m_QueueLock, TRUE);
    
    HX_ASSERT(m_pGateway);
    HX_ASSERT(m_pGateway->m_ulStreamCount);

    m_ppQueue = new CongestionQueue*[m_pGateway->m_ulStreamCount];
    
    for (UINT16 i = 0; i < m_pGateway->m_ulStreamCount; i++)
    {
        m_ppQueue[i] = new CongestionQueue(ulQueueSize);
    }
    
    HXMutexUnlock(m_QueueLock);
    }

    HXMutexUnlock(m_pGateway->m_StateLock);

    BroadcastStreamer_Base::GatewayCheck();
}


STDMETHODIMP
BroadcastStreamer_Blocking::Done()
{
    if (m_pSink)
    {
    m_pSink->SourceDone();
    m_pSink->Release();
    m_pSink = NULL;
    }

    HXMutexLock(m_QueueLock, TRUE);

    HX_ASSERT(m_ppQueue);
    HX_ASSERT(m_pGateway->m_ulStreamCount);

    if (m_ppQueue && m_pGateway->m_ulStreamCount)
    {
    for (UINT16 i = 0; i < m_pGateway->m_ulStreamCount; i++)
    {
        HX_DELETE(m_ppQueue[i]);
    }
    }

    HX_VECTOR_DELETE(m_ppQueue);
    HXMutexUnlock(m_QueueLock);

    return BroadcastStreamer_Base::Done();
}    

HX_RESULT 
BroadcastStreamer_Blocking::SendPacket(IHXPacket* pPacket)
{
    HX_ASSERT(SUCCEEDED(pPacket->QueryInterface(IID_ServerPacket, (void **)0xffffd00d)));
    
    UINT16 unStream = pPacket->GetStreamNumber();
    
    /* make a unique wrapper for this packet: */
    ServerPacket* pServerPacket = new ServerPacket(TRUE);
    pServerPacket->SetPacket(pPacket);
    
    HXMutexLock(m_QueueLock, TRUE);
    
    HX_ASSERT(m_ppQueue);
    if ((m_ppQueue == NULL) ||
        (m_ppQueue[unStream] == NULL))
    {
        HXMutexUnlock(m_QueueLock);
        return HXR_FAIL;
    }
    
    /* Queue this packet for transmission: */
    m_ppQueue[unStream]->Enqueue(pServerPacket);
    
    HXMutexUnlock(m_QueueLock);
    
    HX_RELEASE(pServerPacket);
    
    SendPacketFromQueue (unStream);

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer_Blocking::SinkBlockCleared (UINT32 ulStream)
{
    while (SUCCEEDED(SendPacketFromQueue ((UINT16)ulStream)))
    {
    }    

    return HXR_OK;
}

HX_RESULT
BroadcastStreamer_Blocking::SendPacketFromQueue(UINT16 unStream)
{
    ServerPacket* pPacket = NULL;
    HX_RESULT     hRes    = HXR_OK;

    HX_ASSERT(m_pSink);
    HX_ASSERT(m_pGateway);

    if ((!m_pSink) || (!m_pGateway))
    {
        return HXR_FAIL;
    }

    HXMutexLock(m_QueueLock, TRUE);

    if ((m_ppQueue == NULL) ||
        (m_ppQueue[unStream] == NULL))
    {
        HXMutexUnlock(m_QueueLock);
        return HXR_FAIL;
    }

    /* Get the next packet from the queue: */
    if (SUCCEEDED(m_ppQueue[unStream]->Peek(pPacket)))
    {
        pPacket->AddRef();
    
        if ((HXR_BLOCKED == SyncPacket(pPacket)) ||
            (HXR_BLOCKED == m_pSink->PacketReady(pPacket)))
        {
            HX_RELEASE(pPacket);
            HXMutexUnlock(m_QueueLock);
            return HXR_BLOCKED;
        }
    
        if (unStream == m_pGateway->m_unSyncStream)
        {
            HXMutexLock(m_SyncLock);
            m_ulSyncTime = pPacket->GetTime();
            HXMutexUnlock(m_SyncLock);
        }
    
        if (m_ppQueue)
        {
            m_ppQueue[unStream]->ReleaseHead();
        }

        HX_RELEASE(pPacket);
        hRes = HXR_OK;
    }
    else
    {
        hRes = HXR_FAIL;
    }
    
    HXMutexUnlock(m_QueueLock);
    return hRes;
}

HX_RESULT
BroadcastStreamer_Blocking::SyncPacket(ServerPacket* pPacket)
{
    if ((!pPacket) || (!m_pGateway))
    {
        HX_ASSERT(0);
        return HXR_FAIL;
    }
    
    UINT16 unStream          = pPacket->GetStreamNumber();

    if ((m_pGateway->m_pbTimeStampDelivery[unStream]) &&
    (!m_pGateway->m_pbTimeStampDelivery[unStream][pPacket->GetASMRuleNumber()]))
    {
        return HXR_OK;
    }

    UINT32 ulms              = pPacket->GetTime();
    HX_RESULT hRes           = HXR_OK;
    
    HXMutexLock(m_SyncLock);
    if (unStream == m_pGateway->m_unSyncStream)
    {
        if (m_bBlockSync)
        {
            hRes = HXR_BLOCKED;
        }
    }
    else 
    {
        if (m_ulSyncTime)
        {
            if (ulms > (m_ulSyncTime + 1000))
            {
                m_bBlockSync = FALSE;
                hRes = HXR_BLOCKED;
            }
            else if ((ulms + 1000) < m_ulSyncTime)
            {
                m_bBlockSync = TRUE;
            }
        }
    }

    HXMutexUnlock(m_SyncLock);
    return hRes;
}

STDMETHODIMP
BroadcastStreamer_Blocking::EnableTCPMode ()
{
    return HXR_OK;
}
  
STDMETHODIMP
BroadcastStreamer_Blocking::SetSink (IHXServerPacketSink* pSink)
{
    HX_ASSERT(pSink);

    if (!pSink)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pSink);

    m_pSink = pSink;
    m_pSink->AddRef();

    return HXR_OK;
}

STDMETHODIMP
BroadcastStreamer_Blocking::StartPackets ()
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
BroadcastStreamer_Blocking::GetPacket ()
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

STDMETHODIMP
BroadcastStreamer_Blocking::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXServerPacketSource))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSource*)this;
        return HXR_OK;
    }

    return BroadcastStreamer_Base::QueryInterface(riid, ppvObj);
}

ULONG32
BroadcastStreamer_Blocking::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


ULONG32
BroadcastStreamer_Blocking::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
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
    m_pCurrentSequenceNumbers   = NULL;
    m_nRegEntryIndex            = -1;
    m_ulGwayLatencyTotal        = 0;
    m_ulGwayLatencyReps         = 0;
    m_ulMaxGwayLatency          = 0;
    m_ulAvgGwayLatencyRegID     = 0;
    m_ulMaxGwayLatencyRegID     = 0;
    m_bSureStreamAware          = FALSE;

    m_pASMSource                = NULL;
    m_ppRuleData                = NULL;
    m_pbTimeStampDelivery       = NULL;
    m_unSyncStream              = 0xffff;
    m_pSyncPacket               = NULL;
    m_unKeyframeStream          = 0xffff;
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

    m_ppPacketBufferQueue       = NULL;
    m_ppFuturePacketBufferQueue = NULL;
    m_pbIsKeyframeRule          = NULL;
    m_pbIsSubscribed            = NULL;
    m_bQTrace                   = FALSE;
    m_bEnableRSDDebug           = FALSE; 
    m_bEnableRSDPerPacketLog    = FALSE;
    m_lMinPreroll               = 0;
    m_lExtraPrerollPercentage   = 0;
    m_ulLastPacketTS            = 0;
    
    m_ulNumofPktBufQ            = 0;
    m_bLowLatency               = FALSE;
    m_bQSizeTooSmallReported    = FALSE;
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

    m_pIsPayloadWirePacket = NULL;
    m_pRTCPRule = NULL;
    m_ppRTCPPacket = NULL;


    m_StateLock = HXCreateMutex();
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
    HX_VECTOR_DELETE(m_pCurrentSequenceNumbers);

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
       HX_VECTOR_DELETE(m_pbTimeStampDelivery[i]);
    }
    HX_VECTOR_DELETE(m_ppRuleData);
    HX_VECTOR_DELETE(m_pbTimeStampDelivery);

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

    if (m_ppPacketBufferQueue)
    {
        for (i = 0; i < m_ulNumofPktBufQ; i++)
        {
            HX_RELEASE(m_ppPacketBufferQueue[i]);
        }
        delete m_ppPacketBufferQueue;
    }

    if (m_ppFuturePacketBufferQueue)
    {
        for (i = 0; i < m_ulNumofPktBufQ; i++)
        {
            HX_RELEASE(m_ppFuturePacketBufferQueue[i]);
        }
        delete m_ppFuturePacketBufferQueue;
    }

    HX_VECTOR_DELETE(m_pbIsKeyframeRule);
    HX_VECTOR_DELETE(m_pbIsSubscribed);

    HXDestroyMutex(m_PacketBufferQueueLock);

    for (i = 0; i < m_ulStreamCount; i++)
    {
        HX_RELEASE(m_ppRTCPPacket[i]);
    }
    HX_RELEASE(m_pSyncPacket);

    HX_VECTOR_DELETE(m_pIsPayloadWirePacket);
    HX_VECTOR_DELETE(m_pRTCPRule);
    HX_VECTOR_DELETE(m_ppRTCPPacket);
    
    HXDestroyMutex(m_StateLock);
    HXDestroyMutex(m_BroadcastPacketManagerLock);
#ifdef DEBUG_IAT
    HXDestroyMutex(m_InterArrivalTimeLock);
#endif
    HXDestroyMutex(m_RuleDataLock);
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
                (m_pBG->m_pBroadcastPacketManagers[i])->SendDone(
                    m_pBG->m_pInfo->m_pProc);
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

    m_pBG->m_ulLatencyCalcCBHandle = 
        m_pBG->m_pInfo->m_pProc->pc->engine->ischedule.enter(
        m_pBG->m_pInfo->m_pProc->pc->engine->now + Timeval(15.0),
        m_pBG->m_pLatencyCalcCallback);

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
                                       BroadcastStreamer_Base* pStreamer)
{
    HX_ASSERT(pProc);
    HX_ASSERT(pStreamer);
    HX_ASSERT(pProc->procnum() < MAX_THREADS);

    if (pProc && pStreamer)
    {
        HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
        if (m_pBroadcastPacketManagers[pProc->procnum()] == NULL)
        {
            m_pBroadcastPacketManagers[pProc->procnum()] = 
                new BroadcastPacketManager (pProc, this);

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
            m_pBroadcastPacketManagers[pProc->procnum()]->
                AddBroadcastStreamer(pStreamer);
        }
        HXMutexUnlock(m_BroadcastPacketManagerLock);
    }
}

void
BroadcastGateway::RemoveBroadcastStreamer(Process*                pProc, 
                                          BroadcastStreamer_Base* pStreamer)
{
    HX_ASSERT(pProc);
    HX_ASSERT(pStreamer);
    HX_ASSERT(pProc->procnum() < MAX_THREADS);

    if (pProc && pStreamer)
    {
        HXMutexLock(m_BroadcastPacketManagerLock, TRUE);
        if (m_pBroadcastPacketManagers[pProc->procnum()])
        {
            m_pBroadcastPacketManagers[pProc->procnum()]->
                RemoveBroadcastStreamer(pStreamer);
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

    pHeader->SetPropertyULONG32("LiveStream", 1);

    pHeader->GetPropertyULONG32("StreamCount", m_ulStreamCount);

    UINT32 ulTemp = 0;
    pHeader->GetPropertyULONG32("LatencyMode", ulTemp);
    m_bLowLatency = (ulTemp> 0) ? TRUE : FALSE;

    ulTemp = 0;
    pHeader->GetPropertyULONG32("SureStreamAware", ulTemp);
    m_bSureStreamAware = (ulTemp> 0) ? TRUE : FALSE;

    m_pFileHeader = pHeader;
    pHeader->AddRef();

    m_pStreamHeaders = new IHXValues*[m_ulStreamCount];
    m_pStreamDoneTable = new UINT32[m_ulStreamCount];
    m_pCurrentSequenceNumbers = new UINT32[m_ulStreamCount];
    m_ppRuleData = new RuleData*[m_ulStreamCount];
    m_pbTimeStampDelivery = new BOOL*[m_ulStreamCount];

    m_pIsPayloadWirePacket = new BOOL[m_ulStreamCount];
    m_pRTCPRule = new UINT32[m_ulStreamCount];
    m_ppRTCPPacket = new IHXPacket*[m_ulStreamCount];

    for (UINT16 i = 0; i < m_ulStreamCount; i++)
    {
        m_pStreamHeaders[i] = 0;
        m_pStreamDoneTable[i] = BCAST_STREAMDONE_TABLE_OK;
        m_pCurrentSequenceNumbers[i] = 0;
        m_ppRuleData[i] = NULL;
        m_pbTimeStampDelivery[i] = NULL;
        m_pIsPayloadWirePacket[i] = FALSE;
        m_pRTCPRule[i] = 0xffffffff;
        m_ppRTCPPacket[i] = NULL;
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
    m_pStreamHeaders[ulSN] = pHeader;
    pHeader->AddRef();

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
        UINT16 unVideoKeyframe = 0xffff, unAudioKeyframe = 0xffff, unEventStream = 0xffff;
        for (i = 0; i < m_ulStreamCount; i++)
        {
            if (SUCCEEDED(m_pStreamHeaders[i]->GetPropertyCString ("MimeType", pMimeType)) && pMimeType)
            {
                const char* szMimeType = (const char*)pMimeType->GetBuffer();
                if (szMimeType)
                {
                    if (IsVideoKeyframeStream(pReg, szMimeType))
                    {
                        unVideoKeyframe = i;
                    }
                    else if (IsAudioKeyframeStream(pReg, szMimeType))
                    {
                        unAudioKeyframe = i;
                    }
                    else if (IsRealEventStream(pReg, szMimeType))
                    {
                        unEventStream = i;
                    }
                }
                HX_RELEASE(pMimeType);
            }
        }
        HX_RELEASE(pReg);

        if (unVideoKeyframe != 0xffff)
        {
            m_unKeyframeStream = unVideoKeyframe;
        }
        // only one audio stream
        else if (m_ulStreamCount == 1 && unAudioKeyframe != 0xffff)
        {
             m_unKeyframeStream = unAudioKeyframe;
        }
        // if the content contains only audio and event streams
        else if (m_ulStreamCount == 2 && unAudioKeyframe != 0xffff && unEventStream != 0xffff)
        {
            m_unKeyframeStream = unAudioKeyframe;
        }

        for (i = 0; i < m_ulStreamCount; i++)
        {
            if (SUCCEEDED(m_pStreamHeaders[i]->GetPropertyCString ("MimeType", pMimeType)) && pMimeType)
            {
                const char* szMimeType = (const char*)pMimeType->GetBuffer();
                if (szMimeType && RTSPMEDIA_TYPE_AUDIO == SDPMapMimeToMediaType (szMimeType))
                {
                    m_unSyncStream = i;
                }
                HX_RELEASE(pMimeType);
            }

            IHXBuffer* pPayloadWirePacket = NULL;
            m_pStreamHeaders[i]->GetPropertyCString("PayloadWirePacket", pPayloadWirePacket);
            if (pPayloadWirePacket)
            {
                if (0 == strcasecmp("rtp", (const char *)pPayloadWirePacket->GetBuffer()))
                {
                    m_pIsPayloadWirePacket[i] = TRUE;
                }
                pPayloadWirePacket->Release();
            }
                

            m_pStreamHeaders[i]->GetPropertyCString("ASMRulebook", pRuleBook);
        
            if (pRuleBook)
            {
                pParsedRuleBook = new ASMRuleBook((const char*)pRuleBook->GetBuffer());
                HX_ASSERT(SUCCEEDED(pParsedRuleBook->m_LastError));

                UINT16 unNumRules = pParsedRuleBook->GetNumRules();
                IHXValues* pRuleProps = NULL;
                BOOL bOnDepend = FALSE;
        
                HX_DELETE(m_ppRuleData[i]);
                m_ppRuleData[i] = new RuleData (unNumRules);
                m_pbTimeStampDelivery[i] = new BOOL[unNumRules];

                if (m_ppPacketBufferQueue == NULL && !m_bDisableLiveTurboPlay && m_unKeyframeStream == i)
                {
                    m_ulNumofPktBufQ = unNumRules;
                    m_ppPacketBufferQueue = new CPacketBufferQueue*[m_ulNumofPktBufQ];
                    memset(m_ppPacketBufferQueue, 0, sizeof(CPacketBufferQueue*)*m_ulNumofPktBufQ);

                    m_ppFuturePacketBufferQueue = new CPacketBufferQueue*[m_ulNumofPktBufQ];
                    memset(m_ppFuturePacketBufferQueue, 0, sizeof(CPacketBufferQueue*)*m_ulNumofPktBufQ); 

                    m_pbIsKeyframeRule = new BOOL[m_ulNumofPktBufQ];
                    memset(m_pbIsKeyframeRule, 0, sizeof(BOOL)*m_ulNumofPktBufQ); 
                    
                    m_pbIsSubscribed = new BOOL[m_ulNumofPktBufQ];
                    memset(m_pbIsSubscribed, 0, sizeof(BOOL)*m_ulNumofPktBufQ); 

                    UINT32 ulPreroll = 0;
                    if (m_pStreamHeaders[i]->GetPropertyULONG32("Preroll", ulPreroll) == HXR_OK && ulPreroll > 0)
                    {
                        m_lQueueDuration = ulPreroll;
                    }
                    else
                    {
                        m_pStreamHeaders[i]->SetPropertyULONG32("Preroll", m_lQueueDuration);
                    }

                    if (m_lExtraPrerollPercentage)
                    {
                        UINT64 ulExtraDuration = m_lQueueDuration * m_lExtraPrerollPercentage;
                        ulExtraDuration  /= 100;
                        m_lQueueDuration += (UINT32)ulExtraDuration;
                    }
                
                    // enforce minimum queue duration
                    if (m_lMinPreroll && m_lQueueDuration < m_lMinPreroll)
                    {
                        // ServerPreroll is an internal preroll value not to be communicated
                        m_lQueueDuration = m_lMinPreroll;
                        m_pStreamHeaders[i]->SetPropertyULONG32("ServerPreroll", m_lQueueDuration);
                    }
                }
                
                for (j = 0; j < unNumRules; j++)
                {
                    m_pbTimeStampDelivery[i][j] = FALSE;
                    pRule = NULL;

                    pParsedRuleBook->GetProperties(j, pRuleProps);

                    if (pRuleProps)
                    {
                        pRuleProps->GetPropertyCString("TimeStampDelivery", pRule);
                        
                        if (pRule)
                        {
                            m_pbTimeStampDelivery[i][j] =
                                (pRule->GetBuffer()[0] == 'T') ||
                                (pRule->GetBuffer()[0] == 't');
                
                            pRule->Release();
                            pRule = NULL;
                        }

                        pRuleProps->GetPropertyCString("RTCPRule", pRule);
                        if (pRule)
                        {
                            if (pRule->GetBuffer()[0] == '1' ||
                                pRule->GetBuffer()[0] == 't')
                            {
                                m_pRTCPRule[i] = j;
                            }
                            pRule->Release();
                            pRule = NULL;
                        }

                        pRuleProps->GetPropertyCString("OnDepend", pRule);
                        if (pRule)
                        {
                            bOnDepend = TRUE;
                            pRule->Release();
                            pRule = NULL;
                        }

                        // XXXJJ if this rule has "OnDepend", that means it depends on some other rule(s).
                        // Therefore we will mark it as dependent rule, and not create a bufferQ for it.  
                        // The reason is that even if the players get the keyframe of this rule, they 
                        // can't begin the playback because this rule depends on other rules for rendering.

                        // no queue for rtcp rules, because we will put rtcp packets at the
                        // beginning of the each queue for streams from qtbcplin.

                        if (m_unKeyframeStream == i && m_ppPacketBufferQueue && !bOnDepend
                            && m_pRTCPRule[i] != j) 
                        {
                            m_pbIsKeyframeRule[j] = TRUE;
                        }
                        bOnDepend = FALSE;
                    }

                    HX_RELEASE(pRuleProps);
                }

                HX_DELETE(pParsedRuleBook);
                HX_RELEASE(pRuleBook);
            }
        }
    
        /* don't start the streams until we're completelysetup: */
        for (i = 0; i < m_ulStreamCount; i++)
        {
            m_pBCastObj->StartPackets(i);
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
    UINT16 uStreamNumber;
    UINT16 unRule;

    uStreamNumber = pPacket->GetStreamNumber();
    unRule = pPacket->GetASMRuleNumber();

    if (m_pIsPayloadWirePacket[uStreamNumber] == TRUE)
    {    
        if (unRule == m_pRTCPRule[uStreamNumber])
        {
            IHXBuffer* pBuffer = pPacket->GetBuffer();
            if (pBuffer && pBuffer->GetSize() >= 20 )
            {
                UCHAR* pRTCP = pBuffer->GetBuffer();

                if (pRTCP && RTCP_SR == *(pRTCP+1))
                {
                    HX_RELEASE(m_ppRTCPPacket[uStreamNumber]);
                    m_ppRTCPPacket[uStreamNumber] = pPacket;
                    m_ppRTCPPacket[uStreamNumber]->AddRef();
                }
            }
            HX_RELEASE(pBuffer);
        }
        else if (m_unSyncStream == uStreamNumber)
        {
            HX_RELEASE(m_pSyncPacket);
            m_pSyncPacket = pPacket;
            m_pSyncPacket->AddRef();
        }
    }

    // Instant-on is currently disabled for low latency streams.
    if (!m_bDisableLiveTurboPlay && !m_bLowLatency)
    {
        // for normal latency streams, we queue packets for fast playback
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

HX_RESULT
BroadcastGateway::Done()
{
    HXMutexLock(m_StateLock, TRUE);
    m_State = DONE;
    HXMutexUnlock(m_StateLock);

    m_pInfo->m_pCurrentBroadcasts->RemoveKey(m_pFilename);

    m_ulDestructCBHandle = m_pInfo->m_pProc->pc->engine->schedule.enter(
        m_pInfo->m_pProc->pc->engine->now + Timeval(10.0),
        m_pDestructCallback);

    return HXR_OK;
}

IHXLivePacketBufferQueue* 
BroadcastGateway::GetPacketBufferQueue(UINT16 strmNum, UINT16 ruleNum)
{
    IHXLivePacketBufferQueue* pQueue = NULL;

    if (!m_bDisableLiveTurboPlay && strmNum == m_unKeyframeStream && m_ppPacketBufferQueue)
    {
        HXMutexLock(m_PacketBufferQueueLock, TRUE);
        pQueue = (IHXLivePacketBufferQueue*)m_ppPacketBufferQueue[ruleNum];
        if (pQueue)
        {
            pQueue->AddRef();
        }
        HXMutexUnlock(m_PacketBufferQueueLock);
    }

    return pQueue;
}


BOOL
IsMinQueueDuration(IHXPacket* pPacket, CPacketBufferQueue* pQ, UINT32 ulMinDur)
{
    HX_ASSERT(pPacket && pQ);

    UINT32 ulCurTime = pPacket->GetTime();
    UINT32 ulQTime = pQ->GetStartTime();

    if (ulCurTime >= ulQTime)
    {
        return (ulCurTime - ulQTime) >= ulMinDur;
    }
    else //ulCurTime < ulQTime, check for roll over
    {
        if (ulQTime - ulCurTime > 0x7f000000)
        {
#ifdef RSD_LIVE_DEBUG
            fprintf(stderr, "roll over detected, QTime = %u, Cur = %u\n", ulQTime, ulCurTime);
#endif
            return (MAX_UINT32 - ulQTime + ulCurTime) >= ulMinDur;
        }
    }
    return FALSE;
}

HX_RESULT
BroadcastGateway::HandlePacketBufferQueue(IHXPacket* pPacket, 
                                          UINT16 uStreamNumber, 
                                          UINT16 unRule)
{
    if (m_pIsPayloadWirePacket && m_pIsPayloadWirePacket[uStreamNumber] == TRUE)
    {
        if (m_ulLastPacketTS == 0)
        {
            m_ulLastPacketTS = pPacket->GetTime();
        }
        else
        {
            UINT32 ulCurTS = pPacket->GetTime();
            //if the timestamp difference between two sibling packets is greater than 10s,
            //then something must be wrong(one case is that the live stream is from legacy
            //transmitter(pre 11.1), which have a bug to do incorrect timestamp conversion),
            //we better disable RSD from now on.
            if (ulCurTS > m_ulLastPacketTS && ulCurTS - m_ulLastPacketTS > 10000)
            {
                m_bDisableLiveTurboPlay = TRUE;      
                NEW_FAST_TEMP_STR(errmsg, 512, strlen(m_pFilename)+256);
                sprintf(errmsg, "The packets' timestamps are incorrect for %s, the buffering of packets "
                        "is disabled.", m_pFilename);
                printf("%s\n", errmsg);
                m_pInfo->m_pProc->pc->error_handler->Report(HXLOG_WARNING, 0, 0, errmsg, 0);
                DELETE_FAST_TEMP_STR(errmsg);
                return HXR_FAIL;
            }
            else
            {
                m_ulLastPacketTS = ulCurTS;
            }
        }
    }

    if (m_ppPacketBufferQueue && m_unKeyframeStream != 0xffff)
    {
        BOOL            bKeyFrame = FALSE;
        UINT32          i;
        UINT32          ulRefCount = 0;
        UINT32          ulCurrentQSize = 0;
        BOOL            bAddedToFQ = FALSE;
        BOOL            bIncQSize = FALSE;
        UINT            ulDuration = 0;

        if (uStreamNumber == m_unKeyframeStream)
        {
            // ASMFlags with HX_ASM_SWITCH_ON bit set means this packet is a keyframe.
            // But we only count keyframes form independent rules as "real" keyframes
            // for our buffering purpose.
            //
            // XXXJJ Intentionally keep the aboved inaccurate comments. Most people perceived
            // the same wrong concept as me.  Below is the correct keyframe definition from Larry.
            // 
            // HX_ASM_SWITCH_ON does not indicate a keyframe.  It only indicates a potential
            // starting point.  For video, that could be the start of an I, B, or P frame.  
            // E.g., if an I/B/P frame is large enough to be fragmented across multiple packets,
            // only the first packet of the frame would be marked with HX_ASM_SWITCH_ON (since 
            // you don't want to send a partial frame...).  Only the last packet of the frame
            // would be marked with HX_ASM_SWITCH_OFF (again, since you don't want to send a 
            // partial frame). A keyframe is indicated by the ASM rule not having any OnDepend 
            // directives.  The first packet of a particular keyframe is indicated by 
            // HX_ASM_SWITCH_ON.
 
            bKeyFrame = ((pPacket->GetASMFlags() & HX_ASM_SWITCH_ON) != 0);

            if (m_bEnableRSDPerPacketLog)
            {
                IHXBroadcastDistPktExt* pPacketExt = NULL;
                pPacket->QueryInterface(IID_IHXBroadcastDistPktExt, (void **)&pPacketExt);
                                    
                UINT32 ulStrmSeqNo= 0;
                if (pPacketExt)
                {        
                    ulStrmSeqNo = pPacketExt->GetStreamSeqNo();
                    pPacketExt->Release();
                }       
                fprintf(stderr, "RSDLive T=%d STR=%s(%d:%d) Keyframe=%d, strmseq; %d\n",
                pPacket->GetTime(), m_pFilename, uStreamNumber, unRule, bKeyFrame, ulStrmSeqNo);
                fflush(0);
            }

            // create a future queue if not present
            if (bKeyFrame && 
                m_pbIsKeyframeRule[unRule] && 
                !m_ppFuturePacketBufferQueue[unRule] &&
                //we don't create a new queue when the feed is sureStreamAware and no body
                //subscribe to this rule, because the packets for this rule will stop soon.
                (m_bSureStreamAware == FALSE || m_pbIsSubscribed[unRule] == TRUE))
            {
                m_ppFuturePacketBufferQueue[unRule] = new CPacketBufferQueue(m_lQueueSize, m_bQTrace);
                m_ppFuturePacketBufferQueue[unRule]->AddRef();
#ifdef FIXED_RTP_ORDERING
                m_ppFuturePacketBufferQueue[unRule]->Init(m_ppPacketBufferQueue[unRule]);
#endif
    
                // for packets from rtp live, we need rtcp packets from all streams, plus a packet
                // from a master stream(m_unSyncStream)
                for (i = 0; i < m_ulStreamCount; i++)
                {
                    // we need to send these rtcp packets first only the cients connect,
                    // because they will need them to sync up ntp time.
                    if (m_ppRTCPPacket[i])
                    {
                        m_ppFuturePacketBufferQueue[unRule]->EnQueue(m_ppRTCPPacket[i]);
                    }
                }
                //we need to put one packet from the master stream(normally audio) into the queue
                //because RTPInfoSync needs RTCP SR from all streams and one RTP packet from the
                //master stream to calculate ntp time.  Normally the queue will be like:
                // RTCPSR-videoStream, RTCPSR-AudioStream, RTP-AudioStream, RTP-VideoKeyframe....
                if (m_pSyncPacket)
                {
                    m_ppFuturePacketBufferQueue[unRule]->EnQueue(m_pSyncPacket);
                }
    
                m_ppFuturePacketBufferQueue[unRule]->AddKeyframe(pPacket); 
    
                bAddedToFQ = TRUE;
                
                if (m_bEnableRSDPerPacketLog)
                {
                    fprintf(stderr, "RSDLive(stream %d, rule %d) Create a future queue %d, starting timestamp: %d\n",
                     uStreamNumber, unRule, m_ppFuturePacketBufferQueue[unRule], pPacket->GetTime());
                    fflush(0);
                }
            }

            for (i = 0; i <  m_ulNumofPktBufQ; i++)
            {
                if (m_pbIsKeyframeRule[i])
                {
                    //for SureStreamAware feed, if nobody subscribe to this rule, the packets 
                    //will stop soon. We need to flush these queues so we won't server stale packets
                    //once the subscribe comes in again.
                    if (m_bSureStreamAware && m_pbIsSubscribed[i] == FALSE)
                    {
#ifdef RSD_LIVE_DEBUG
                        if (m_ppPacketBufferQueue[i] || m_ppFuturePacketBufferQueue[i])
                        {
                            printf("flush q: %p, fq: %p, rule: %d\n", m_ppPacketBufferQueue[i],
                            m_ppFuturePacketBufferQueue[i], i);
                        }
#endif
                        if (m_ppPacketBufferQueue[i])
                        {
                            HXMutexLock(m_PacketBufferQueueLock, TRUE);
                            HX_RELEASE(m_ppPacketBufferQueue[i]);
                            HXMutexUnlock(m_PacketBufferQueueLock);
                        }
    
                        HX_RELEASE(m_ppFuturePacketBufferQueue[i]);
                    }
    
                    if (m_ppPacketBufferQueue[i])
                    {
                        if (m_ppPacketBufferQueue[i]->EnQueue(pPacket) != HXR_OK)
                        {
                            ulCurrentQSize = m_ppPacketBufferQueue[i]->GetSize();

                            HXMutexLock(m_PacketBufferQueueLock, TRUE);
                            ulRefCount = m_ppPacketBufferQueue[i]->Release();
                            m_ppPacketBufferQueue[i] = NULL;
                            HXMutexUnlock(m_PacketBufferQueueLock);

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
                        else
                        {
                            //check and remove PacketBufferQueue if duration is greater than max duration
                            if (m_ppPacketBufferQueue[i]->GetSize() > 100 &&
                                ((ulDuration = m_ppPacketBufferQueue[i]->GetDuration()) > 
                                            m_lMaxDurationOfPacketBufferQueue))
                            {
                                NEW_FAST_TEMP_STR(errmsg, 1024, strlen(m_pFilename)+256);
                                sprintf(errmsg, "RSDLive: Packet Queue with size %d and duration %.2f for %s,"
                                    "stream: %d, rule: %d exceeded the max duration %d. Flushing Packet Queue",
                                    m_ppPacketBufferQueue[i]->GetSize(), (float)ulDuration/1000,
                                    m_pFilename, uStreamNumber, i,
                                    m_lMaxDurationOfPacketBufferQueue/1000);
                                printf("%s\n", errmsg);
                                fflush(0);
                                HXMutexLock(m_PacketBufferQueueLock, TRUE);
                                HX_RELEASE(m_ppPacketBufferQueue[i]);
                                HXMutexUnlock(m_PacketBufferQueueLock);
                                m_pInfo->m_pProc->pc->error_handler->Report(HXLOG_WARNING, 0, 0, errmsg, 0);
                                DELETE_FAST_TEMP_STR(errmsg);
                            }
                        }
                    }
    
                    if (m_ppFuturePacketBufferQueue[i] && !bAddedToFQ)
                    {
                        if (m_ppFuturePacketBufferQueue[i]->EnQueue(pPacket) != HXR_OK)
                        {
                            ulCurrentQSize = m_ppFuturePacketBufferQueue[i]->GetSize();
                            HX_RELEASE(m_ppFuturePacketBufferQueue[i]);
                            //we only increase the size of the queue under the condition that the queue
                            //size hasn't been increased yet, judged by ulCurrentQSize >= m_lQueueSize.
                            //It is possible the queue size had been increased by other queues.
                            //We don't need to check the refcount as the current queue, becasue no one
                            //esle is using this queue.
                            if (ulCurrentQSize >= m_lQueueSize)
                            {
                                bIncQSize = TRUE;
                            }
                        }
                        
                        //we need to do the queue length check here because those keyframe
                        //rules only have sparse packets.
                        if (IsMinQueueDuration(pPacket, m_ppFuturePacketBufferQueue[i], m_lQueueDuration))
                        {
                            if (m_bEnableRSDDebug)
                            {
                                char szTime[128];
                                Timeval tNow = m_pInfo->m_pProc->pc->engine->now;
                                struct tm localTime;
                                hx_localtime_r(&tNow.tv_sec, &localTime);
                                strftime(szTime, 128, "%d-%b-%y %H:%M:%S", &localTime);
                        
                                fprintf(stderr, "%s.%03d RSDLive T=%d STR=%s(%d:%d) CurrentQDepth=%d "
                                "NextQDepth=%d CurrentQTS=%d NextQTS=%d\n",
                                    szTime, tNow.tv_usec/1000, 
                                    pPacket->GetTime(), m_pFilename, m_unKeyframeStream, i, 
                                    m_ppPacketBufferQueue[i] ? m_ppPacketBufferQueue[i]->GetSize() : 0,
                                    m_ppFuturePacketBufferQueue[i] ?  m_ppFuturePacketBufferQueue[i]->GetSize() :0,
                                    m_ppPacketBufferQueue[i] ? m_ppPacketBufferQueue[i]->GetStartTime() : 0,
                                    m_ppFuturePacketBufferQueue[i] ?  m_ppFuturePacketBufferQueue[i]->GetStartTime() :0);
                                fflush(0);
                            }
    
                            HXMutexLock(m_PacketBufferQueueLock, TRUE);
                
                            HX_RELEASE(m_ppPacketBufferQueue[i]);
                            // taking AddRef()
                            m_ppPacketBufferQueue[i] = m_ppFuturePacketBufferQueue[i];
                            m_ppFuturePacketBufferQueue[i] = NULL;                
                            HXMutexUnlock(m_PacketBufferQueueLock);
                        }
                    }                        
    
                    if (m_bQTrace)
                    {
                        printf("*** CQ ");
                        m_ppPacketBufferQueue[i]->Dump();            
                        printf("*** FQ ");
                        if (m_ppFuturePacketBufferQueue[i])
                        {
                            m_ppFuturePacketBufferQueue[i]->Dump();            
                        }
                        else
                        {
                            printf("\tNULL\n");
                        }
                    }
                }
            }
        }
        else //uStreamNumber != m_unKeyframeStream 
        {
            //enqueue all audio and appl streams
            for (i = 0; i <  m_ulNumofPktBufQ; i++)
            {
                if (m_ppPacketBufferQueue[i] != NULL)
                {
                    if (m_ppPacketBufferQueue[i]->EnQueue(pPacket) != HXR_OK)
                    {
                        ulCurrentQSize = m_ppPacketBufferQueue[i]->GetSize();

                        HXMutexLock(m_PacketBufferQueueLock, TRUE);
                        ulRefCount = m_ppPacketBufferQueue[i]->Release();
                        m_ppPacketBufferQueue[i] = NULL;
                        HXMutexUnlock(m_PacketBufferQueueLock);

                        //we only increase the size of the queue under the two conditions:
                        //1. somebody is using the queue, judged  by refcount > 0 after we release it.
                        //  it does no harm when we reach the max queue size while nobody is using //  it.
                        //2. the queue size hasn't been increased yet, judged by ulCurrentQSize >= m_lQueueSize.
                        // it is possible the queue size had been increased by other queues.
                        if (ulRefCount != 0 && ulCurrentQSize >= m_lQueueSize)
                        {
                            bIncQSize = TRUE;
                        }
                    }
                    else
                    {
                        //check and remove PacketBufferQueues if duration is greater than max duration
                        if (m_ppPacketBufferQueue[i]->GetSize() > 100 && 
                            ((ulDuration = m_ppPacketBufferQueue[i]->GetDuration()) > m_lMaxDurationOfPacketBufferQueue))
                        {
                            NEW_FAST_TEMP_STR(errmsg, 1024, strlen(m_pFilename)+256);
                            sprintf(errmsg, "RSDLive: Packet Queue with size %d and duration %.2f for %s,"
                                        "stream: %d, rule: %d exceeded the max duration %d. Flushing Packet Queue",
                                        m_ppPacketBufferQueue[i]->GetSize(), (float)ulDuration/1000,
                                        m_pFilename, uStreamNumber, i,
                                        m_lMaxDurationOfPacketBufferQueue/1000);
                            printf("%s\n", errmsg);
                            fflush(0);
                            HXMutexLock(m_PacketBufferQueueLock, TRUE);
                            HX_RELEASE(m_ppPacketBufferQueue[i]);
                            HXMutexUnlock(m_PacketBufferQueueLock);
                            m_pInfo->m_pProc->pc->error_handler->Report(HXLOG_WARNING, 0, 0, errmsg, 0);
                            DELETE_FAST_TEMP_STR(errmsg);
                        }
                    }
                }
                if (m_ppFuturePacketBufferQueue[i])
                {
                    if (m_ppFuturePacketBufferQueue[i]->EnQueue(pPacket) != HXR_OK)
                    {
                        ulCurrentQSize = m_ppFuturePacketBufferQueue[i]->GetSize();
                        HX_RELEASE(m_ppFuturePacketBufferQueue[i]);
                        //we only increase the size of the queue under the condition that the queue
                        //size hasn't been increased yet, judged by ulCurrentQSize >= m_lQueueSize.
                        //It is possible the queue size had been increased by other queues.
                        //We don't need to check the refcount as the current queue, becasue no one
                        //esle is using this queue.
                        if (ulCurrentQSize >= m_lQueueSize)
                        {
                            bIncQSize = TRUE;
                        }
                    }
        }                        
            }
        }

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


    }

    return HXR_OK;
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

    BroadcastStreamer_Base*     pBroadcastStreamer  = NULL;
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
            pBroadcastStreamer = (BroadcastStreamer_Base *)*i;
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
        pBroadcastStreamer = (BroadcastStreamer_Base *)*i;

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
    BroadcastStreamer_Base* pStreamer = NULL;
    
    if (m_ulCallbackHandle)
    {
        m_pProc->pc->engine->ischedule.remove(m_ulCallbackHandle);
        m_ulCallbackHandle = 0;
    }

    while (pStreamer = 
       (BroadcastStreamer_Base*)m_BroadcastStreamerList.remove_head())
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
BroadcastPacketManager::AddBroadcastStreamer(BroadcastStreamer_Base* pStreamer)
{
    pStreamer->AddRef();
    m_BroadcastStreamerList.insert(pStreamer);
}

void 
BroadcastPacketManager::RemoveBroadcastStreamer(BroadcastStreamer_Base* pStreamer)
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
{
    m_QueueLock = HXCreateMutex();
}

BroadcastStreamQueue::~BroadcastStreamQueue()
{
    HXMutexLock(m_QueueLock, TRUE);

    while (m_uHead != m_uTail)
    {
        HX_RELEASE(m_pQueue[m_uHead].m_pPacket);
        m_pQueue[m_uHead].m_bIsStreamDoneMarker = FALSE;
        m_uHead = (m_uHead + 1) % m_uMaxSize;
    }
    
    HXMutexUnlock(m_QueueLock);
    
    HXDestroyMutex(m_QueueLock);

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
    /* Place a stream done marker in the packet Queue */
    
    HXMutexLock(m_QueueLock, TRUE);
    
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
    
    HXMutexUnlock(m_QueueLock);
}

HX_RESULT
BroadcastStreamQueue::Enqueue(IHXPacket* pPacket)
{
    
    Timeval tHeadTime   = m_pParent->m_pProc->pc->engine->now;
    
    /* Queue this packet if the queue occupancy isn't too long */
    HXMutexLock(m_QueueLock, TRUE);

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
        HXMutexUnlock(m_QueueLock);
        return HXR_FAIL;
    }

    m_pQueue[m_uTail].m_tTime = m_pParent->m_pProc->pc->engine->now;
    m_pQueue[m_uTail].m_bIsStreamDoneMarker = FALSE;
    m_pQueue[m_uTail].m_pPacket = pPacket;
    pPacket->AddRef();

    m_uTail = uNewTail;
    HXMutexUnlock(m_QueueLock);
    return HXR_OK;
}

IHXPacket*
BroadcastStreamQueue::Dequeue(Timeval&  tHead,
                              UINT8&    bIsStreamDoneMarker,
                              UINT16&   unStreamNumberDone)
{
    IHXPacket* pPacket = NULL;

    /* DeQueue the Packet :  this queue is designed to not need to lock on 
     *  dequeue. Due to a linux bug(see MemCache::_RecyclerGet) we still 
     * need to lock on linux at least. So just lock it and work out the
     * linux issue later. */
    HXMutexLock(m_QueueLock, TRUE);

    if (m_uHead == m_uTail)
    {
        // empty queue
        HXMutexUnlock(m_QueueLock);
        return NULL;
    }

    tHead = m_pQueue[m_uHead].m_tTime;
    unStreamNumberDone = m_pQueue[m_uHead].m_unStreamNumberDone;
    bIsStreamDoneMarker = m_pQueue[m_uHead].m_bIsStreamDoneMarker;

    pPacket = m_pQueue[m_uHead].m_pPacket;
    m_uHead = (m_uHead + 1) % m_uMaxSize;
    
    HXMutexUnlock(m_QueueLock);

    // caller inherits queue's AddRef()
    return pPacket;
}

float
BroadcastStreamQueue::QueueDepthInSeconds()
{
    float fSecsInQ = 0;
    Timeval tHead;
       
    if (m_uHead != m_uTail)
    {
        HXMutexLock(m_QueueLock, TRUE);
        tHead = m_pQueue[m_uHead].m_tTime;
        HXMutexUnlock(m_QueueLock);
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

/* CongestionQueue: */
CongestionQueue::CongestionQueue(UINT32 ulQueueSize)
    : m_pCongestionQueue(NULL)
    , m_ulQueueWrite(0)
    , m_ulQueueRead(0)
    , m_ulQueueSize(ulQueueSize)
{
    m_pCongestionQueue = new ServerPacket*[m_ulQueueSize];
    memset(m_pCongestionQueue, 0, sizeof(ServerPacket*) * m_ulQueueSize);
}

CongestionQueue::~CongestionQueue()
{
    for (UINT16 i = 0; i < m_ulQueueSize; i++)
    {
        HX_RELEASE(m_pCongestionQueue[i]);
    }
    
    HX_VECTOR_DELETE(m_pCongestionQueue);
}

HX_RESULT
CongestionQueue::Enqueue(ServerPacket* pPacket)
{
    HX_ASSERT(pPacket);
    HX_ASSERT(m_ulQueueWrite < m_ulQueueSize);

    if (!pPacket)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (m_pCongestionQueue == NULL)
    {
        (*g_pBroadcastPacketsDropped)++;
        return HXR_NOT_INITIALIZED;
    }

    if (m_pCongestionQueue[m_ulQueueWrite])
    {
        (*g_pBroadcastPacketsDropped)++;
        m_pCongestionQueue[m_ulQueueWrite]->Release();
        m_pCongestionQueue[m_ulQueueWrite] = NULL;
    }
    
    m_pCongestionQueue[m_ulQueueWrite] = pPacket; 
    pPacket->AddRef();
    (++m_ulQueueWrite) %= m_ulQueueSize;

    return HXR_OK;
}

HX_RESULT
CongestionQueue::Peek(ServerPacket*& pPacket)
{
    pPacket = (m_pCongestionQueue[m_ulQueueRead]) ? 
    m_pCongestionQueue[m_ulQueueRead] : NULL;

    return (pPacket) ? HXR_OK : HXR_FAIL;
}


HX_RESULT
CongestionQueue::ReleaseHead()
{
    if (m_pCongestionQueue[m_ulQueueRead])
    {
        m_pCongestionQueue[m_ulQueueRead]->Release();
        m_pCongestionQueue[m_ulQueueRead] = NULL;
        (++m_ulQueueRead) %= m_ulQueueSize;
    }

    return HXR_OK;
}

//CPacketBufferQueue

CPacketBufferQueue::CPacketBufferQueue(UINT32 ulQueueSize, BOOL bTrace)
{
    m_ulRefCount = 0;
    m_ulInsertPosition = 0;
    m_ulQueueSize = ulQueueSize;
    m_PacketBufferQueue = new IHXPacket*[m_ulQueueSize];
    m_ulQueueBytes = 0;
    m_ulStartTS = 0;

#ifdef FIXED_RTP_ORDERING
    m_pPrevQ = NULL;
    m_bJustAddedKeyFrame = FALSE;
    m_ulKeyFrameStrmSeqNo = 0;
    m_ulKeyFrameStream = 0xffff;
#endif

    m_bTrace = bTrace;
}

CPacketBufferQueue::~CPacketBufferQueue()
{
    for (int i = 0; i < m_ulInsertPosition; i++)
    {
        m_PacketBufferQueue[i]->Release();
    }
    delete m_PacketBufferQueue;
#ifdef FIXED_RTP_ORDERING
    HX_RELEASE(m_pPrevQ);
#endif
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
    IHXPacket* pHead = NULL;
    IHXPacket* pTail = NULL;
    HX_RESULT theErr = HXR_OK;
    UINT ulDuration = 0;
    INT32 timeDiff = 0;

    theErr = GetPacket(0, pHead);
    if (SUCCEEDED(theErr))
    {    
       theErr = GetPacket(m_ulInsertPosition - 1, pTail);
    }

    if (SUCCEEDED(theErr))
    {
        timeDiff = pTail->GetTime() - pHead->GetTime();
        ulDuration = (timeDiff < 0) ? 0 : timeDiff;
    }

    HX_RELEASE(pHead);
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

#ifdef FIXED_RTP_ORDERING
    if (m_bJustAddedKeyFrame)
    {
        IHXBroadcastDistPktExt* pPacketExt = NULL;
        pPacket->QueryInterface(IID_IHXBroadcastDistPktExt,
                            (void **)&pPacketExt);
        
        UINT32 ulStrmSeqNo= 0;
        UINT16 usStream = 0xffff;
        if (pPacketExt)
        {
            ulStrmSeqNo = pPacketExt->GetStreamSeqNo();
            usStream = pPacket->GetStreamNumber();
            pPacketExt->Release();
        }

        // this is for packets from qtbcplin only, because there is no a jtter
        //buffer in qtbcplin, so the packets are not sorted.

        // for packets from brcvplin, the packets are reordered in a jitter buffer
        // so we won't have a gap for them.

        // for packets from other live sources(wm encoder), there won't come in here
        // because we don't have CPacketBufferQueue for them.(7/2005, this might change 
        // later).
        if (usStream == m_ulKeyFrameStream) 
        {
            if (ulStrmSeqNo - m_ulKeyFrameStrmSeqNo > 1)
            {
                UINT32 ulPrevQPos = m_pPrevQ->m_ulInsertPosition - 1 ;
                UINT32 ulSearchEnd = ulPrevQPos > 10 ? ulPrevQPos - 10 : 0;
                for (UINT32 i = ulPrevQPos; i > ulSearchEnd; i--)
                {
                    IHXPacket* pCurPacket = m_pPrevQ->m_PacketBufferQueue[i];
                    pCurPacket->QueryInterface(
                         IID_IHXBroadcastDistPktExt, (void **)&pPacketExt);
                    UINT32 ulCurStrmSeqNo = pPacketExt->GetStreamSeqNo();
                    pPacketExt->Release();
    
                    if (ulCurStrmSeqNo > m_ulKeyFrameStrmSeqNo &&
                        ulCurStrmSeqNo < ulStrmSeqNo)
                    {
                        m_PacketBufferQueue[m_ulInsertPosition] = pCurPacket;
                        pPacket->AddRef();
                        m_ulInsertPosition++;

                        if (m_bTrace)
                        {
                            IHXBuffer* pBuf = pPacket->GetBuffer();    
                            m_ulQueueBytes += pBuf->GetSize();
                            pBuf->Release(); 
                        }
                    }
                }
            }
            m_bJustAddedKeyFrame = FALSE;
            HX_RELEASE(m_pPrevQ);
        }
    }
#endif

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

#ifdef FIXED_RTP_ORDERING
void 
CPacketBufferQueue::Init(CPacketBufferQueue* pPrevQ)
{
    HX_ASSERT(m_pPrevQ == NULL);
    HX_RELEASE(m_pPrevQ);
    m_pPrevQ = pPrevQ;
    if (m_pPrevQ)
    {
        m_pPrevQ->AddRef();
    }
}
#endif

HX_RESULT 
CPacketBufferQueue::AddKeyframe(IHXPacket*  pPacket)
{
    HX_RESULT rc = HXR_OK;
    IHXBroadcastDistPktExt* pPacketExt = NULL;
    m_ulStartTS = pPacket->GetTime();
    rc = EnQueue(pPacket);
    HX_ASSERT(rc == HXR_OK);

#ifdef FIXED_RTP_ORDERING
    pPacket->QueryInterface(IID_IHXBroadcastDistPktExt, (void **)&pPacketExt);
    if (pPacketExt && m_pPrevQ)
    {
        m_bJustAddedKeyFrame = TRUE;

        //these two are for out of order packet handling.
        m_ulKeyFrameStrmSeqNo = pPacketExt->GetStreamSeqNo();
        m_ulKeyFrameStream = pPacket->GetStreamNumber();
        pPacketExt->Release();
    }
#endif

    return rc;
}

HX_RESULT
CPacketBufferQueue::GetPacket(UINT32 ulIndex, IHXPacket*&  pPacket)
{
    if (ulIndex < m_ulInsertPosition)
    {
        pPacket = m_PacketBufferQueue[ulIndex];
        pPacket->AddRef();
        return HXR_OK;
    }
    return HXR_FAIL;
}
