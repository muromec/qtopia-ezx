/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: static_pushpktflow.cpp,v 1.68 2007/02/15 19:19:40 jzeng Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <new.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "proc.h"
#include "server_engine.h"
#include "ihxpckts.h"
#include "hxdtcvt.h"
#include "hxformt.h"
#include "hxasm.h"
#include "source.h"
#include "hxassert.h"
#include "hxslist.h"
#include "hxmap.h"
#include "servpckts.h"
#include "transport.h"
#include "base_errmsg.h"
#include "asmrulep.h"
#include "server_info.h"
#include "servreg.h"
#include "bwcalc.h"
#include "bandcalc.h"
#include "mutex.h"
#include "loadinfo.h"

#include "dtcvtcon.h"
#include "mem_cache.h"
#include "hxpiids.h"
#include "globals.h"
#include "player.h"
#include "streamer_container.h"
#include "hxservpause.h"

#include "servbuffer.h"

#include "hxpcktflwctrl.h"
#include "hxqos.h"
#include "hxqossess.h"
#include "hxqostran.h"
#include "hxqossig.h"
#include "pcktflowmgr.h"
#include "static_pushpktflow.h"
#include "server_context.h"
#include "pcktstrm.h"
#include "pcktflowwrap.h"
#include "qos_sess_cbr_ratemgr.h"
#include "qos_cfg_names.h"
#include "isifs.h"
#include "hxstrutl.h"

#define DISABLE_ASM

STDMETHODIMP
StaticPushPacketFlow::GetNextPacket(UINT16 unStreamNumber, BOOL bAlwaysGet)
{
    HX_ASSERT(0);
    return HXR_FAIL;
}

StaticPushPacketFlow::StaticPushPacketFlow(Process* proc,
                               IHXSessionStats* pSessionStats,
                               UINT16 unStreamCount,
                               PacketFlowManager* pFlowMgr,
                               IHXServerPacketSource* pSource,
                               BOOL bIsMulticast,
                               BOOL bIsLive)
  : BasicPacketFlow(proc, pSessionStats, unStreamCount, pFlowMgr, bIsMulticast),
    m_pQoSConfig (NULL),
    m_pBufferVerifier (NULL),
    m_pResendWrite (NULL),
    m_pResendRead (NULL),
    m_ppResendQueue (NULL),
    m_pBlockQueue (NULL),
    m_pAggregator (NULL),
    m_bIsLive (bIsLive),
    m_pResendVerifier(NULL),
    m_pSignalBus(NULL),
    m_ulTotalRate (0),
    m_ulCurrentCumulativeMediaRate(0),
    m_enumStreamAdaptScheme(ADAPTATION_NONE),
    m_pSessionAggrLinkCharParams(NULL),
    m_bStreamLinkCharSet(FALSE),
    m_uRegisterStreamGroupNumber(0xFFFF),
    m_pAggRateAdaptParams(0)
{
#ifdef DISABLE_ASM
    m_bAutoSubscription = FALSE;
#endif    

    m_pSource = pSource;
    m_pSource->SetSink((IHXServerPacketSink*)this);

    m_pASMSource = NULL;
    m_pSource->QueryInterface(IID_IHXASMSource,
                              (void**)&m_pASMSource);

    m_ppResendQueue = new ServerPacket** [m_unStreamCount];
    
    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        m_ppResendQueue [i] = new ServerPacket* [RESEND_QUEUE_SIZE];
        memset(m_ppResendQueue [i], 0, sizeof(ServerPacket*) * RESEND_QUEUE_SIZE);
    }
    
    m_pResendWrite = new UINT8 [m_unStreamCount];
    m_pResendRead = new UINT8 [m_unStreamCount];
    m_pbPausedStreams = new BOOL [m_unStreamCount];
    m_pBlockQueue = new ServerPacket* [m_unStreamCount];

    memset(m_pResendWrite, 0, sizeof(UINT8) * m_unStreamCount);
    memset(m_pResendRead, 0, sizeof(UINT8) * m_unStreamCount);
    memset(m_pbPausedStreams, 0, sizeof(BOOL) * m_unStreamCount);
    memset(m_pBlockQueue, 0, sizeof(ServerPacket*) * m_unStreamCount);
}

StaticPushPacketFlow::~StaticPushPacketFlow()
{
    HX_RELEASE(m_pResendVerifier);
    HX_RELEASE(m_pASMSource);
    HX_RELEASE(m_pBufferVerifier);
    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pQoSConfig);
    HX_RELEASE(m_pAggregator);
    
    if (m_pSignalBus)
    {
        m_pSignalBus->DettachListener( MAKE_HX_QOS_SIGNAL_ID(
                HX_QOS_SIGNAL_LAYER_SESSION, 
                    HX_QOS_SIGNAL_RELEVANCE_METRIC,
                HX_QOS_SIGNAL_COMMON_STREAM_ADAPT_HDR), 
                (IHXQoSSignalSink*)this);

        m_pSignalBus->DettachListener(MAKE_HX_QOS_SIGNAL_ID(
                HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT, 
                HX_QOS_SIGNAL_RELEVANCE_METRIC, 
                HX_QOS_SIGNAL_COMMON_MEDIA_RATE), 
                (IHXQoSSignalSink*)this);
            
        HX_RELEASE(m_pSignalBus);
    }

    
    for (UINT16 j = 0; j < m_unStreamCount; j++)
    {
        HX_RELEASE(m_pBlockQueue [j]);

        for (UINT8 i = 0; i < RESEND_QUEUE_SIZE; i++)
        {
            HX_RELEASE(m_ppResendQueue [j][i]);
        }
        HX_VECTOR_DELETE(m_ppResendQueue [j]);
    }
    HX_VECTOR_DELETE(m_ppResendQueue);

    HX_VECTOR_DELETE(m_pResendWrite);
    HX_VECTOR_DELETE(m_pResendRead);
    HX_VECTOR_DELETE(m_pbPausedStreams);
    HX_VECTOR_DELETE(m_pBlockQueue);

    HX_DELETE(m_pSessionAggrLinkCharParams);
}

STDMETHODIMP
StaticPushPacketFlow::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXServerPacketSink))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPacketResend))
    {
        AddRef();
        *ppvObj = (IHXPacketResend*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXStreamAdaptationSetup))
    {
        AddRef();
        *ppvObj = (IHXStreamAdaptationSetup*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSLinkCharSetup))
    {
        AddRef();
        *ppvObj = (IHXQoSLinkCharSetup*)this;
        return HXR_OK;
    }

    return BasicPacketFlow::QueryInterface(riid, ppvObj);
}

void
StaticPushPacketFlow::Done()
{
    if (IsDone())
        return;

    SourceDone();

    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pASMSource);
    HX_RELEASE(m_pAggregator);

    if (m_pSignalBus)
    {
        m_pSignalBus->DettachListener( MAKE_HX_QOS_SIGNAL_ID(
                HX_QOS_SIGNAL_LAYER_SESSION, 
                    HX_QOS_SIGNAL_RELEVANCE_METRIC,
                HX_QOS_SIGNAL_COMMON_STREAM_ADAPT_HDR), 
                (IHXQoSSignalSink*)this);

        m_pSignalBus->DettachListener(MAKE_HX_QOS_SIGNAL_ID(
                HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT, 
                HX_QOS_SIGNAL_RELEVANCE_METRIC, 
                HX_QOS_SIGNAL_COMMON_MEDIA_RATE), 
                (IHXQoSSignalSink*)this);

        HX_RELEASE(m_pSignalBus);
    }



    BasicPacketFlow::Done();
}

STDMETHODIMP_(ULONG32) 
StaticPushPacketFlow::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) 
StaticPushPacketFlow::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

void
StaticPushPacketFlow::JumpStart(UINT16 uStreamNumber)
{
    HX_ASSERT(0);
}

void
StaticPushPacketFlow::HandleResume()
{
    for (UINT16 j = 0; j < m_unStreamCount; j++)
    {
        if (!m_pStreams[j].m_bStreamRegistered)
        {
            continue;
        }
        m_pStreams[j].HandleLiveResume();

        IHXServerPauseAdvise* pPauseAdvise = m_pStreams[j].m_pPauseAdvise;
        if (pPauseAdvise)
        {
            pPauseAdvise->OnPauseEvent(FALSE);  // false => resume
        }
    }

    if (!m_pRateManager)
    {
        UINT32 i = 0;
        for (i = 0; i < m_unStreamCount; i++)
        {
            if (m_pStreams[i].m_bStreamRegistered && m_pbPausedStreams [i])
            {
                m_pbPausedStreams [i] = FALSE;
                m_pSource->SinkBlockCleared(i);
            }
        }        
    }
}

UINT32 QuadrupleOrLess(UINT32 ulBW, UINT32 ulBR)
{
    if(ulBW < ulBR)
    {
        return ulBR;
    }

    return ulBW > ulBR*4 ? ulBR*4 : ulBW;
}

/**
 * \brief AdjustInitialSendingRate - adjust the initial sending rate of each stream based on bw info
 *
 * calculate initial sending rate based on bw info, with priority sdb > linkchar > bandwidth, the
 * max is capped at 4 times of media rate. There is a special case for per-stream linkchar, because
 * we don't know the total rate at the beginning, we will have it after we add each stream's
 * linkchar together. 
 *
 *
 * \param pRateDesc[in]: used to get the media rate
 * \param pUberStreamManager: used to get the selected streams
 *
 * \return HXR_OK
 */
UINT32 StaticPushPacketFlow::AdjustInitialSendingRate(IHXRateDescription*  pRateDesc, IHXUberStreamManager* pUberStreamManager)
{
    // priority SDB > linkchar > bandwidth
    UINT32 ulCumulativeAvgRate = 0;
    UINT32 ulInitialSendingRate = 0;
    BOOL bAdjustStreamLinkChar = FALSE;

    pRateDesc->GetAvgRate(ulCumulativeAvgRate);

    if(m_ulSetDeliveryBandwidth)
    {
        ulInitialSendingRate = m_ulSetDeliveryBandwidth;
    }
    else if(m_pSessionAggrLinkCharParams)
    {
        ulInitialSendingRate = m_pSessionAggrLinkCharParams->m_ulGuaranteedBW;
    }

    else if(m_bStreamLinkCharSet)
    {
        bAdjustStreamLinkChar = TRUE;
    }
    else if(m_ulBandwidth)
    {
        ulInitialSendingRate = m_ulBandwidth;
    }

    //adjust the initial sending rate to not greater than 4 times of clip bitrate
    //for per stream linkchar headers, we don't know the total yet.
    if(bAdjustStreamLinkChar == FALSE)
    {
        ulInitialSendingRate = QuadrupleOrLess(ulInitialSendingRate, ulCumulativeAvgRate);
    }

    UINT32 unBWAllocations = pRateDesc->GetNumBandwidthAllocations();
    UINT32 ulLogicalStream = 0;
    for (UINT32 i = 0; i < unBWAllocations; i++)
    {
        if (SUCCEEDED(pUberStreamManager->GetSelectedLogicalStreamNum(i, ulLogicalStream)) && 
            m_pStreams[ulLogicalStream].m_bStreamRegistered)
        { 
            // we adjust per stream initial sending rate according to per stream link char headers.
            if(bAdjustStreamLinkChar)
            {
                UINT32 ulStreamRate = pRateDesc->GetBandwidthAllocationArray()[i];                
                m_pStreams[ulLogicalStream].m_ulInitialRate = 
                    QuadrupleOrLess(m_pStreams[ulLogicalStream].m_pLinkCharParams->m_ulGuaranteedBW, ulStreamRate);
                ulInitialSendingRate += m_pStreams[ulLogicalStream].m_ulInitialRate;
            }
            // for other cases, we distribute the bandwidth to each stream accordint to the ratio of bw/br.  We do 
            // this for sdp, bw, agg linkchar headers, as well as no bw info headers are received(just for simplicity
            // so we do need to handle this special case).  The per stream bw will be saved in 
            // m_pStreams[ulLogicalStream].m_ulInitialRate, so StaticPushPacketFlow::InitialStartup Can use that. 
            else
            {
                UINT32 ulStreamRate = pRateDesc->GetBandwidthAllocationArray()[i];                
                m_pStreams[ulLogicalStream].m_ulInitialRate = 
                      ((UINT64)ulStreamRate*(UINT64)ulInitialSendingRate)/ulCumulativeAvgRate;
            }
        }
    }

    return ulInitialSendingRate; 
}

void
StaticPushPacketFlow::InitialStartup()
{
    if (!m_pResendVerifier && m_pRateManager)
    {
        m_pRateManager->QueryInterface(IID_IHXQoSClientBufferVerifier, (void**)&m_pResendVerifier);
    }


    /*
     * Signal the MediaRate
     */
    IHXUberStreamManager*   pUberStreamManager = NULL;
    if (SUCCEEDED(m_pSource->QueryInterface(IID_IHXUberStreamManager, (void**)&pUberStreamManager)))
    {
        IHXRateDescription* pRateDesc = NULL;
        
        UINT32 ulInitialCumulativeSendingRate = 0;
        UINT32 ulLogicalStream = 0;
        
        if (SUCCEEDED(pUberStreamManager->GetCurrentAggregateRateDesc(pRateDesc)))
        {
            ulInitialCumulativeSendingRate = AdjustInitialSendingRate(pRateDesc, pUberStreamManager);

            if (m_unStreamCount == 1)
            {
                SendMediaRateSignal(0, ulInitialCumulativeSendingRate, ulInitialCumulativeSendingRate);
            }
            else
            {
                UINT32 unBWAllocations = pRateDesc->GetNumBandwidthAllocations();

                for (UINT32 i = 0; i < unBWAllocations; i++)
                {
                    if (SUCCEEDED(pUberStreamManager->GetSelectedLogicalStreamNum(i, ulLogicalStream)) && 
                        m_pStreams[ulLogicalStream].m_bStreamRegistered)
                    {   
                        SendMediaRateSignal(ulLogicalStream, 
                                            m_pStreams[ulLogicalStream].m_ulInitialRate, 
                                            ulInitialCumulativeSendingRate);
                    }
                }
            }
        }
        HX_RELEASE(pRateDesc);
    }    
    HX_RELEASE(pUberStreamManager);

    HX_ASSERT(m_pSource);
    m_pSource->StartPackets();
}

void
StaticPushPacketFlow::SendMediaRateSignal(UINT16 unStreamNumber, 
                                          UINT32 ulRate, 
                                          UINT32 ulCumulativeRate,
                                          HXBOOL bInitialRateReset)
{
    HX_ASSERT(m_pStreams[unStreamNumber].m_bStreamRegistered);

    IHXQoSSignal* pSignal = NULL;
    IHXBuffer*   pBuffer = NULL;

    // We update Delivery bandwidth here only for initial rate.
    //subsequent rate changes are updated in the Signal handler.
    if (!m_bInitialPlayReceived && m_ulCurrentCumulativeMediaRate != ulCumulativeRate)
    {
        m_ulCurrentCumulativeMediaRate = ulCumulativeRate;
        m_pFlowMgr->ChangeDeliveryBandwidth(m_ulCurrentCumulativeMediaRate,TRUE);
    }

    if (SUCCEEDED(m_pProc->pc->common_class_factory->
                  CreateInstance(CLSID_IHXQoSSignal, (void**)&pSignal)) &&
        SUCCEEDED(m_pProc->pc->common_class_factory->
                  CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer)) &&
        m_pSignalBus)
    {
        pBuffer->SetSize(sizeof(RateSignal));

        if (bInitialRateReset)
        {
            pSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                 HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                 HX_QOS_SIGNAL_COMMON_INIT_MEDIA_RATE));
        }
        else
        {
            pSignal->SetId(m_pStreams[unStreamNumber].m_mediaRateSignal);
        }
        RateSignal* pRateSignal = (RateSignal*)pBuffer->GetBuffer();

        pRateSignal->m_unStreamNumber = unStreamNumber;
        pRateSignal->m_ulRate = ulRate;
        pRateSignal->m_ulCumulativeRate = ulCumulativeRate;

        pSignal->SetValue(pBuffer);
        m_pSignalBus->Send(pSignal);
    }
    
    HX_RELEASE(pSignal);
    HX_RELEASE(pBuffer);

    m_pStreams[unStreamNumber].m_mediaRateSignal = MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                   HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                   HX_QOS_SIGNAL_COMMON_MEDIA_RATE);
}


void
StaticPushPacketFlow::UpdateMediaRate(BOOL bInitialRateReset)
{
    /*
     * Signal the MediaRate
     */
    IHXUberStreamManager*   pUberStreamManager = NULL;
    if (SUCCEEDED(m_pSource->QueryInterface(IID_IHXUberStreamManager, (void**)&pUberStreamManager)))
    {
        IHXRateDescription* pRateDesc = NULL;
        UINT32 ulCumulativeAvgRate = 0;
        UINT32 ulLogicalStream = 0;
        
        if (SUCCEEDED(pUberStreamManager->GetCurrentAggregateRateDesc(pRateDesc)))
        {
            pRateDesc->GetAvgRate(ulCumulativeAvgRate);
            if (m_unStreamCount == 1)
            {
                SendMediaRateSignal(0,
                                    ulCumulativeAvgRate, 
                                    ulCumulativeAvgRate,
                                    bInitialRateReset);
            }
            else
            {
                UINT32 unBWAllocations = pRateDesc->GetNumBandwidthAllocations();
                for (UINT32 i = 0; i < unBWAllocations; i++)
                {
                    if (SUCCEEDED(pUberStreamManager->GetSelectedLogicalStreamNum(i, ulLogicalStream)) && 
                        m_pStreams[ulLogicalStream].m_bStreamRegistered)
                    {   
                        SendMediaRateSignal(ulLogicalStream, 
                                            pRateDesc->GetBandwidthAllocationArray()[i], 
                                            ulCumulativeAvgRate,
                                            bInitialRateReset);
                    }
                }
            }
        }
        HX_RELEASE(pRateDesc);
    }    
    HX_RELEASE(pUberStreamManager);
}


STDMETHODIMP
StaticPushPacketFlow::StartSeek(UINT32 ulTime)
{
    HX_RESULT hr = HXR_OK;
    m_bInSeek = TRUE;

    if (m_pRateManager)
    {
        IHXPacketFlowControl* pRateMgrFlowControl = NULL;
        if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                                                     (void**)&pRateMgrFlowControl))
        {
            hr = pRateMgrFlowControl->StartSeek(ulTime);
            HX_RELEASE(pRateMgrFlowControl);
        }
    }
    
    return hr;
}

STDMETHODIMP
StaticPushPacketFlow::SeekDone()
{
    HX_RESULT hr = HXR_OK;
    m_bInSeek = FALSE;
    
    if (m_pStreams)
    {
        PacketStream* pStream = NULL;
        for (UINT16 j = 0; j < m_unStreamCount; j++)
        {
            pStream = &m_pStreams[j];

            if (!pStream->m_bStreamRegistered)
            {
                continue;
            }
            if (pStream->m_bSentStreamDone)
                HandleUnStreamDone(pStream);
            
            pStream->m_bStreamDonePending = FALSE;
            pStream->m_bStreamDone = FALSE;
            pStream->m_bSentStreamDone = FALSE;
        }
    }

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        PacketStream*        pStream;
        if(m_pBlockQueue [i])
        {
            // XXXJJ. This is a hack, which is the best solution I can think of.
            /*Here is the bug I try to fix:
            Before a packet is sent to m_pBufferVerifier(in ::PacketReady), it needs to be 
            assigned a sequence number for m_pBufferVerifier to keep tracks.  But this packet 
            may end up in m_pBlockQueue and not be sent out immediately. If it ends up here
            then we are in trouble before it has been assigned a sequence number and the 
            stream sequence number has been increased by 1 because of this packet.
            Now we decides to discard this packet, and the player will see a sequence gap
            and report lost packets.

            This hack tries to do damage control. If this packet is the last one arrived in
            ::packetReady while Seek is called, we can safely decrease the stream sequence
            number by 1 because we won't send out this packet. If not, then we won't do anything
            because a lost packet is not that bad compared to screw up the whole thing.*/

            pStream = & m_pStreams[i];
            if(m_pBlockQueue [i]->m_uSequenceNumber == pStream->m_unSequenceNumber - 1)
            {
                pStream->m_unSequenceNumber--;
            }
            HX_RELEASE(m_pBlockQueue [i]);
        }
    }
    
    if (m_pRateManager)
    {
        IHXPacketFlowControl* pRateMgrFlowControl = NULL;
        if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                                                     (void**)&pRateMgrFlowControl))
        {
            hr = pRateMgrFlowControl->SeekDone();
            HX_RELEASE(pRateMgrFlowControl);
        }
    }

    if (m_bPlayPendingOnSeek)
    {
        m_bPlayPendingOnSeek = FALSE;
        Play();
    }

    return hr;
}

void 
StaticPushPacketFlow::SetPlayerInfo(Player* pPlayerControl, 
                                    const char* szPlayerSessionId,
                                    IHXSessionStats* pSessionStats)

{
    BasicPacketFlow::SetPlayerInfo(pPlayerControl, szPlayerSessionId,
                                   pSessionStats);

    HX_ASSERT(m_pPlayerSessionId);

    if (m_pProc->pc->process_type == PTStreamer)
    {
        StreamerContainer* pStreamer = ((StreamerContainer*)(m_pProc->pc));

        HXMutexLock(pStreamer->m_BusMapLock, TRUE);
        pStreamer->m_BusMap.Lookup(((const char*)(m_pPlayerSessionId->GetBuffer())), 
                                   (void*&)m_pSignalBus);
        HXMutexUnlock(pStreamer->m_BusMapLock);

        if (m_pSignalBus)
        {
            m_pSignalBus->AddRef();
            m_pSignalBus->AttachListener( MAKE_HX_QOS_SIGNAL_ID(
                    HX_QOS_SIGNAL_LAYER_SESSION, 
                        HX_QOS_SIGNAL_RELEVANCE_METRIC,
                    HX_QOS_SIGNAL_COMMON_STREAM_ADAPT_HDR), 
                    (IHXQoSSignalSink*)this);

            //get Signal for media rate changes due to upshift/downshift.
            m_pSignalBus->AttachListener(MAKE_HX_QOS_SIGNAL_ID(
                    HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT, 
                    HX_QOS_SIGNAL_RELEVANCE_METRIC, 
                    HX_QOS_SIGNAL_COMMON_MEDIA_RATE), 
                    (IHXQoSSignalSink*)this);

            HX_VERIFY(SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSProfileConfigurator, 
                                                     (void**)&m_pQoSConfig)));
        }
    }
    
}

STDMETHODIMP
StaticPushPacketFlow::RegisterStream(Transport* pTransport,
                                     UINT16 uStreamNumber,
                                     ASMRuleBook* pRuleBook,
                                     IHXValues* pHeader)
{
    IHXQoSClassFactory* pQoSCCF = NULL;
    IHXQoSCongestionControl* pCongestionCtl = NULL;
    IHXUberStreamManager*   pUberStreamManager = NULL;

    if (m_uFirstStreamRegistered == 0xFFFF)
    {
        m_uFirstStreamRegistered = uStreamNumber;
    }

    m_uNumStreamsRegistered++;
    m_pStreams[uStreamNumber].Register(pTransport, pRuleBook,
                                       pHeader, uStreamNumber, 
                                       TRUE, m_uRegisterStreamGroupNumber);

    m_ulTotalRate += m_pStreams[uStreamNumber].m_ulAvgBitRate;
   

     /**************************************************************
     * Set Inputsource to select the Logical stream being registered 
     *    for corresponding StreamGroup 
     ***************************************************************/
    if (m_pSource)
    {
        if (SUCCEEDED(m_pSource->QueryInterface(IID_IHXUberStreamManager, 
                                            (void**)&pUberStreamManager)))
        {
            IHXRateDescEnumerator* pLogicalStreamEnum = NULL;
            IHXRateDescription* pRateDesc = NULL;

            // Get the RateDescription for this logical stream 
            // and set it as the RateDescription for the StreamGroup.
            // Find the right RateDescription using the default stream 
            // rules if present, otherwise use the stream's AvgBitRate 
            if (SUCCEEDED(pUberStreamManager->GetLogicalStream(uStreamNumber
                                                        , pLogicalStreamEnum)))
            {
                if ((m_pStreams[uStreamNumber].m_unDefaultRuleNum != 
                        INVALID_RULE_NUM &&
                    SUCCEEDED(pLogicalStreamEnum->FindRateDescByRule(
                        m_pStreams[uStreamNumber].m_unDefaultRuleNum, TRUE, 
                        FALSE, pRateDesc))) ||
                    SUCCEEDED(pLogicalStreamEnum->FindRateDescByClosestAvgRate(
                        m_pStreams[uStreamNumber].m_ulAvgBitRate, TRUE, FALSE, 
                        pRateDesc)) ||
                    SUCCEEDED(pLogicalStreamEnum->FindRateDescByMidpoint(
                        m_pStreams[uStreamNumber].m_ulAvgBitRate, TRUE, FALSE, 
                        pRateDesc)))
                {
                    if (pRateDesc)
                    {
                        DPRINTF(0x02000000, 
                            ("StaticPushPacketFlow::RegisterStream(), "
                            "found RateDesc for uStreamNumber: %u, "
                            "m_uRegisterStreamGroupNumber: %u, "
                            "ulAvgRate: %u, pRateDesc: %p\n",
                            uStreamNumber, m_uRegisterStreamGroupNumber, 
                            m_pStreams[uStreamNumber].m_ulAvgBitRate, 
                            pRateDesc));

                        pUberStreamManager->SetStreamGroupRateDesc( 
                            m_uRegisterStreamGroupNumber, uStreamNumber, 
                            pRateDesc, NULL);
                        HX_RELEASE(pRateDesc);
                    }
                }

                HX_RELEASE(pLogicalStreamEnum);
            }

            HX_RELEASE(pUberStreamManager);
        }
    }

    if (!m_pRateManager)
    {
        IHXQoSRateMgrClassFactory* pRateMgrClassFactory = NULL;
        m_pProc->pc->qos_class_factory->QueryInterface(IID_IHXQoSRateMgrClassFactory,
                                                        (void **)&pRateMgrClassFactory);

        if (pRateMgrClassFactory && SUCCEEDED(pRateMgrClassFactory->
                                                CreateInstance(m_pQoSConfig, 
                                                       m_enumStreamAdaptScheme,
                                                       CLSID_IHXQoSRateManager,
                                                       (void**)&m_pRateManager)))
            {
            HX_RELEASE(pRateMgrClassFactory);
            m_pRateManager->Init(m_pStats, m_pPlayerSessionId, m_unStreamCount, m_pSource);

            //* Set the Aggregate Adaptation parameteres for Aggregate Helix-Adaptation
            if (m_enumStreamAdaptScheme == ADAPTATION_HLX_AGGR)
            {
                IHXStreamAdaptationSetup* pStreamAdaptSetup;
                if (SUCCEEDED(m_pRateManager->QueryInterface(IID_IHXStreamAdaptationSetup, 
                                                                  (void**)&pStreamAdaptSetup)))
                {
                    pStreamAdaptSetup->SetStreamAdaptationParams(m_pAggRateAdaptParams);
                    HX_RELEASE(pStreamAdaptSetup);
                }
            }

            if (FAILED(m_pRateManager->QueryInterface(IID_IHXServerPacketSink, 
                                                 (void**)&m_pBufferVerifier)))
            {
                m_pBufferVerifier = NULL;
            }

            if (m_pBufferVerifier)
            {
                m_pBufferVerifier->SetSource((IHXServerPacketSource*)this);
            }
        }
    }
    
    if (!m_bIsLive)
    {
        if ((!m_pAggregator) && (SUCCEEDED(m_pProc->pc->qos_class_factory->
                                           CreateInstance(m_pQoSConfig, 
                                                          CLSID_IHXQoSRateShapeAggregator,
                                                          (void**)&m_pAggregator))))
        {
            HX_VERIFY(SUCCEEDED(m_pAggregator->Init(m_unStreamCount)));
        }
    }
        
    if (SUCCEEDED(m_pProc->pc->server_context->QueryInterface(IID_IHXQoSClassFactory,
                                                              (void**)&pQoSCCF)))
    {
        if (SUCCEEDED(pQoSCCF->CreateInstance(m_pQoSConfig, 
                                              CLSID_IHXQoSCongestionControl,
                                              (void**)&pCongestionCtl)))
        {
            IHXServerPacketSource* pSource = NULL;
            IHXServerPacketSink* pSink = NULL;

            if (m_pAggregator)
            {
                IHXQoSRateShaper*    pShaper = NULL;
                
                HX_VERIFY(SUCCEEDED(pCongestionCtl->
                                    QueryInterface(IID_IHXQoSRateShaper, (void**)&pShaper)));
                
                if (pShaper)
                {
                    HX_VERIFY(SUCCEEDED(pShaper->Init(m_pAggregator)));
                }
                
                HX_RELEASE(pShaper);
            }
            
            HX_ASSERT(m_pPlayerSessionId);
            pCongestionCtl->Init(m_pPlayerSessionId, 
                                 uStreamNumber, 
                                 m_pStreams[uStreamNumber].m_ulAvgBitRate, 
                                 m_pStreams[uStreamNumber].m_ulAvgPktSz);

            //Pass Link Characteristics, if available, down to Congestion Control
            IHXQoSLinkCharSetup* pQoSLinkCharSetup = NULL;
            LinkCharParams* pLinkCharParams = m_pSessionAggrLinkCharParams
                                                ? m_pSessionAggrLinkCharParams
                                                : m_pStreams[uStreamNumber].m_pLinkCharParams;
            if (pLinkCharParams &&
                        SUCCEEDED(pCongestionCtl->QueryInterface(IID_IHXQoSLinkCharSetup, 
                                                                    (void**)&pQoSLinkCharSetup)))
            {
                pQoSLinkCharSetup->SetLinkCharParams(pLinkCharParams); 
                HX_RELEASE(pQoSLinkCharSetup);
            }

            pCongestionCtl->QueryInterface(IID_IHXServerPacketSink, 
                                           (void**)&(m_pStreams[uStreamNumber].m_pSink));
            pCongestionCtl->QueryInterface(IID_IHXServerPacketSource, (void**)&pSource);
            HX_VERIFY((SUCCEEDED(pTransport->
                                 QueryInterface(IID_IHXServerPacketSink, (void**)&pSink))));

            m_pStreams[uStreamNumber].m_pSink->SetSource((IHXServerPacketSource*)this);
            pSource->SetSink(pSink);
            
            HX_RELEASE(pSource);
            HX_RELEASE(pSink);
        }
    }

    if (!pCongestionCtl)
    {
        pTransport->QueryInterface(IID_IHXServerPacketSink,
                                   (void**)&(m_pStreams[uStreamNumber].m_pSink));
    }

    HX_RELEASE(pCongestionCtl);
    HX_RELEASE(pQoSCCF);

    return (m_pRateManager) ? m_pRateManager->RegisterStream(&(m_pStreams[uStreamNumber]), 
                                                             uStreamNumber) : HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::RegisterStream(Transport* pTransport,
                                     UINT16 uStreamGroupNumber,
                                     UINT16 uStreamNumber,
                                     ASMRuleBook* pRuleBook,
                                     IHXValues* pHeader)
{
    m_uRegisterStreamGroupNumber = uStreamGroupNumber;

    HX_RESULT hr = RegisterStream(pTransport, uStreamNumber, pRuleBook, pHeader);

    //m_uRegisterStreamGroupNumber is transient and should be Reset
    // as it could be changed by subsequent RegisterStream calls
    m_uRegisterStreamGroupNumber = 0xFFFF;

    return hr;
}

STDMETHODIMP
StaticPushPacketFlow::SetSource(IHXServerPacketSource* pSource)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

STDMETHODIMP
StaticPushPacketFlow::SourceDone()
{
    HX_RELEASE(m_pResendVerifier);
    if (m_pStreams)
    {
        for (UINT16 j = 0; j < m_unStreamCount; j++)
        {
            if (m_pStreams[j].m_bStreamRegistered && m_pStreams[j].m_pSink)
            {
                m_pStreams[j].m_pSink->SourceDone();
            }
        }
    }

    if (m_pBufferVerifier)
    {
        m_pBufferVerifier->SourceDone();
        m_pBufferVerifier->Release();
        m_pBufferVerifier = NULL;
    }

    if (m_pRateManager)
    {
        m_pRateManager->Done();
        m_pRateManager->Release();
        m_pRateManager = NULL;
    }
    
    return HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::Flush()
{
    return HXR_OK;
}

void
StaticPushPacketFlow::Pause(BOOL bWouldBlock, UINT32 ulPausePoint)
{
    // call base class

    BasicPacketFlow::Pause(bWouldBlock, ulPausePoint);

    // do our custom processing:
    // advise all the stream transports of the pause

    if (!ulPausePoint)
    {
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            if (!m_pStreams[i].m_bStreamRegistered)
            {
                continue;
            }

            IHXServerPauseAdvise* pPauseAdvise = m_pStreams[i].m_pPauseAdvise;
            if (pPauseAdvise)
            {
                HX_ASSERT(m_pStreams[i].m_bStreamRegistered);
                pPauseAdvise->OnPauseEvent(TRUE);
            }
        }
    }
}

STDMETHODIMP
StaticPushPacketFlow::StartPackets()
{
    HX_ASSERT(m_pSource);    
    return m_pSource->StartPackets();
}

STDMETHODIMP
StaticPushPacketFlow::PacketReady(ServerPacket* pPacket)
{
    HX_ASSERT(pPacket);

    if (pPacket == NULL)
    {
        return HXR_OK;
    }

    if (m_bIsDone)
    {
        return HXR_UNEXPECTED;
    }

    PacketStream*        pStream = NULL;
    UINT16               uStreamNumber = pPacket->GetStreamNumber();
    UINT16               unRule = 0;
    UINT8                ucFlags = 0;
    BOOL                 bIsLost = FALSE;

    pStream = &m_pStreams[uStreamNumber];
    if (!pStream->m_bStreamRegistered)
    {
        return HXR_OK;
    }

    if ((!m_pRateManager) && (m_bPaused || m_bInSeek))
    {
        m_pbPausedStreams[uStreamNumber] = TRUE;
        return HXR_BLOCKED;
    }

    bIsLost = pPacket->IsLost();
    if (!bIsLost)
    {
        unRule = pPacket->GetASMRuleNumber();
        ucFlags = pPacket->GetASMFlags();
    }

    pStream->m_bPacketRequested = FALSE;

    if (m_uEndPoint > 0 && pPacket->GetTime() >= m_uEndPoint)
    {
        if (m_bIsPausePointSet)
        {
            //Use pause-stream semantics... don't unsubscribe rules
            Pause(FALSE);
        }
        else if (!pStream->m_bStreamDonePending)
        {
            pStream->m_bStreamDonePending = TRUE;
            pStream->m_bFirstPacketTSSet = FALSE;
            pStream->m_ulFirstPacketTS = 0;
            
            StreamDone(uStreamNumber);
            return HXR_BLOCKED;
        }
    }

    /* assert if this packet has already be processed */
    HX_ASSERT(!pPacket->m_bRateMgrBlocked);
    HX_ASSERT(!pPacket->m_bTransportBlocked);

    // Have to take care of the Seek
    if (!m_bSessionPlaying && !m_bTimeLineSuspended)
    {
        BOOL bIsFirstSeekPckt = m_bSeekPacketPending ? TRUE : FALSE;
        ResetSessionTimeline(pPacket, uStreamNumber,                             
                             bIsFirstSeekPckt);

        HX_ASSERT(!pStream->IsFirstPacketTSSet());
        pStream->SetFirstPacketTS(pPacket->GetTime());                    

        m_bSeekPacketPending = FALSE;
        m_bSessionPlaying = TRUE;
    }

    DPRINTF(0x02000000, ("Live Session %p: sent packet. stream: %d time %ld "
                         "asm rule: %d\n", this, uStreamNumber,
                         pPacket->GetTime(), unRule));

    if (pStream->m_bStreamDone)
    {
        /* 
         * We may get PacketReady calls for pending GetPacket request on 
         * other streams. This should ONLY happen when there is an end time
         * associated with this source (refer to the next if condition). 
         */
        return HXR_FAIL;
    }
    
#ifndef DISABLE_ASM
    //
    // the rule is outside of our rulebook so the packet is bad
    //
    if (unRule >= pStream->m_lNumRules)
    {
        DPRINTF(0x02000000, ("Live Session %p: dropped packet with bad rule. stream:"
                             " %d time %ld asm rule: %d\n", this, uStreamNumber,
                             pPacket->GetTime(), unRule));
        return HXR_FAIL;
    }
#endif

    HX_ASSERT(pStream->m_pRules);
    if (!pStream->m_pRules)
    {
        return HXR_FAIL;
    }

    if (bIsLost)
    {
//        goto SkipAllASMProcessing;
        return SendToTransport(pPacket, pStream, unRule);
    }

#ifdef ENABLE_MDP_FOR_LIVE
    /*
     * Sync live streams to keyframe
     */
    if (!pStream->m_pRules[unRule].m_bSyncOk)
    {
        if (ucFlags & HX_ASM_SWITCH_ON)
        {
            pStream->m_pRules[unRule].m_bSyncOk = TRUE;
            pStream->m_pRules[unRule].m_PendingAction = NONE;
        }
        else
        {
            return HXR_OK;
        }
    }
#endif

#ifndef DISABLE_ASM
    /*
     * Sync live streams to keyframe
     */
    if (!pStream->m_pRules[unRule].m_bSyncOk)
    {
        if (!pStream->m_pRules[unRule].m_bRuleOn)
        {
            pStream->m_pRules[unRule].m_bSyncOk = TRUE;
            pStream->m_pRules[unRule].m_PendingAction = NONE;

            return HXR_OK;
        }
        else if (pStream->IsDependOk(TRUE, unRule) &&
                 (ucFlags & HX_ASM_SWITCH_ON))
        {
            pStream->m_pRules[unRule].m_bSyncOk = TRUE;
            pStream->m_pRules[unRule].m_PendingAction = NONE;
        }
        else
        {
            return HXR_OK;
        }
    }

    if ((pStream->m_pRVDrop) &&
        (!pStream->m_pRVDrop->PacketApproved(pPacket)))
    {
        return HXR_OK;
    }

    if (pStream->m_pRules[unRule].m_PendingAction == NONE)
    {
        /*
         * No rule transition is in progress.  Attempt normal filtering
         * of packets whose rules are not on.
         */
        if (!pStream->m_pRules[unRule].m_bRuleOn)
        {
            return HXR_OK;
        }
    }
    else
    {
        /*
         * Rule transition is in progress.  Handle SwitchOn and SwitchOff
         * flags correctly here.
         */
        if (pStream->m_pRules[unRule].m_PendingAction == ACTION_ON)  
        {
            if ((!(ucFlags & HX_ASM_SWITCH_ON)) ||
                (!pStream->IsDependOk(TRUE, unRule)))
            {
                return HXR_OK;
            }
            else
            {
                // Update Delivery rate
                if (!pStream->m_pRules[unRule].m_bBitRateReported)
                {
//                    m_ulSubscribedRate += pStream->m_pRules[unRule].m_ulAvgBitRate;
                    if (m_pRateManager)
                    {
                        IHXPacketFlowControl* pRateMgrFlowControl = NULL;
                        if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                                                                     (void**)&pRateMgrFlowControl))
                        {
                            // May have to look at this again, if the implementation
                            // of HandleSubscribe changes in RateManager
                            pRateMgrFlowControl->HandleSubscribe(unRule, uStreamNumber);
                            HX_RELEASE(pRateMgrFlowControl);
                        }  
                    }
                        
                    INT32 lChange = pStream->m_pRules[unRule].m_ulAvgBitRate;
                    ASSERT(pStream->m_bGotSubscribe);
                    pStream->ChangeDeliveryBandwidth(lChange);
                    m_pFlowMgr->ChangeDeliveryBandwidth(lChange, 
                                                        !m_bIsMulticast && !pStream->m_bNullSetup);
/*
                    if (pStream->m_pRules[unRule].m_bTimeStampDelivery)
                    {
                        pStream->m_ulVBRAvgBitRate += pStream->m_pRules[unRule].m_ulAvgBitRate;
                    }
*/
                    pStream->m_pRules[unRule].m_bBitRateReported = TRUE;
                }
                pStream->m_pRules[unRule].m_PendingAction = NONE;
            }
        }
        else if (pStream->m_pRules[unRule].m_PendingAction == ACTION_OFF)
        {
            if ((ucFlags & HX_ASM_SWITCH_OFF) &&
                (pStream->IsDependOk(FALSE, unRule)))
            {
                if (pStream->m_pRules[unRule].m_bBitRateReported)
                {
//                    m_ulSubscribedRate -= pStream->m_pRules[unRule].m_ulAvgBitRate;
                    if (m_pRateManager)
                    {
                        IHXPacketFlowControl* pRateMgrFlowControl = NULL;
                        if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                                                                     (void**)&pRateMgrFlowControl))
                        {
                            // May have to look at this again, if the implementation
                            // of HandleSubscribe changes in RateManager
                            pRateMgrFlowControl->HandleUnSubscribe(unRule, uStreamNumber);
                            HX_RELEASE(pRateMgrFlowControl);
                        }               
                    }

                    INT32 lChange =
                        (-1) * (INT32)pStream->m_pRules[unRule].m_ulAvgBitRate;
                    ASSERT(pStream->m_bGotSubscribe);
                    pStream->ChangeDeliveryBandwidth(lChange);
                    m_pFlowMgr->ChangeDeliveryBandwidth(
                        lChange, !m_bIsMulticast && !pStream->m_bNullSetup);
/*
                    if (pStream->m_pRules[unRule].m_bTimeStampDelivery)
                    {
                        pStream->m_ulVBRAvgBitRate -= pStream->m_pRules[unRule].m_ulAvgBitRate;
                    }
*/
                    pStream->m_pRules[unRule].m_bBitRateReported = FALSE;
                }

                pStream->m_pRules[unRule].m_PendingAction = NONE;

                if (pStream->m_bStreamDonePending)
                {
                    bStreamDone = TRUE;

                    for (i = 0; i < pStream->m_lNumRules; i++)
                    {
                        if (pStream->m_pRules[unRule].m_PendingAction != NONE)
                        {
                            bStreamDone = FALSE;
                        }
                    }

                    if (bStreamDone)
                    {
                        // all the rules are unsubscribed and 
                        // we are done with this stream
                        StreamDone(uStreamNumber);
                    }
                }

                return HXR_OK;
            }
        }
    }
#endif

    /*
     * If the packet is NOT IsLost() but we don't have a buffer, then we
     * must should have only needed this packet for updating the ASM state
     * machine, but the state machine must be messed up somehow.
     * In this case, don't queue the packet!
     */

    /* Take advantage of pPacket is a ServerPacket, instead of IHXPacket
     * We can get GetSize to check the buffer, instead of calling GetBuffer
     * which we need to release it.
     */
    
    if(pPacket->GetSize() == 0)
    {
        return HXR_OK;
    }
    
    // XXXVS: Packet Sequence Number needs to be initialized 
    // before handing the packet to BufferVerifier. BufferVerifier
    // maintains a Packet map keyed on Packet Sequence Number
    if (!pPacket->m_uSequenceNumber)
    {
        pPacket->m_uSequenceNumber = pStream->m_unSequenceNumber++;
    }

    if ((!pPacket->m_bRateMgrBlocked) && 
        m_pBufferVerifier)
    {
        HX_RESULT hr = m_pBufferVerifier->PacketReady(pPacket);
        
        if (hr == HXR_BLOCKED)
        {
            UINT16 unStream = pPacket->GetStreamNumber();
            HX_ASSERT(!m_pBlockQueue [unStream]);
            HX_RELEASE(m_pBlockQueue [unStream]);
            m_pBlockQueue [unStream] = pPacket;
            pPacket->AddRef();
            return hr;
        }
        else if (!SUCCEEDED(hr))
        {
            Player::Session* pSession = m_pPlayerControl->FindSession((const char *)m_pPlayerSessionId->GetBuffer());
            HX_ASSERT(pSession);

            if (HXR_NOTENOUGH_PREDECBUF == hr)
            {
                IHXSessionStats* pSessionStats = pSession->GetSessionStats();
                IHXBuffer* pURL = pSessionStats->GetURL();
                UINT32 unMsgLen = 256;

                if (pURL)
                {
                    unMsgLen += pURL->GetSize();
                }

                IHXErrorMessages* pErrorMessages = 0;
                m_pProc->pc->server_context->QueryInterface(IID_IHXErrorMessages,
                                                        (void **)&pErrorMessages);

                NEW_FAST_TEMP_STR(szErrStr, 1024, unMsgLen);
                sprintf(szErrStr, "Unable to stream %s, predecode buffer size is insufficient for pre-roll.", 
                                            pURL ? (const char*)pURL->GetBuffer() : "");
                pErrorMessages->Report(HXLOG_ERR, HXR_NOTENOUGH_PREDECBUF, 
                                                    0, szErrStr, NULL);
                IHXBuffer* pAlertBuf = 0;
                if (SUCCEEDED(m_pProc->pc->common_class_factory->CreateInstance( CLSID_IHXBuffer,
                                                                                       (void **)&pAlertBuf)))
                {
                    pAlertBuf->Set((const UCHAR *)szErrStr, strlen(szErrStr) + 1);
                    pSession->SendAlert(pAlertBuf);
                    HX_RELEASE(pAlertBuf);
                }

                DELETE_FAST_TEMP_STR(szErrStr);

                HX_RELEASE(pURL);
                HX_RELEASE(pErrorMessages);
            }

            if (pSession)
            {
                pSession->Done(HXR_UNEXPECTED);
            }

            return hr;
        }

        /*
        if (hr == HXR_FAIL)
        {
            return hr;
        }
        */
    }
    
    return SendToTransport(pPacket, pStream, unRule);
}

inline HX_RESULT 
StaticPushPacketFlow::SendToTransport(ServerPacket* pPacket, PacketStream* pStream, UINT16 unRule)
{
    HX_ASSERT(pStream->m_pRules);

    pPacket->m_uPriority =
        pStream->m_pRules[unRule].m_ulPriority;

    pPacket->m_uASMRuleNumber = unRule;


    if (pStream->m_unSequenceNumber >=
        pStream->m_pTransport->wrapSequenceNumber())
    {
        pStream->m_unSequenceNumber = 0;
    }

    if (m_bRTPInfoRequired) 
    {
        Transport*    pTransport = pStream->m_pTransport;
        IHXRTPPacket* pRTPPacket = NULL;
        UINT32 ulBaseTime = 0;
        UINT16 unStreamNumber = pPacket->GetStreamNumber();
        UINT16 i = 0;

        if(pPacket->QueryInterface(IID_IHXRTPPacket, 
                                   (void**) &pRTPPacket) == HXR_OK)
        {
            UINT32 ulRTPTime = pRTPPacket->GetRTPTime();

            ulBaseTime =  (pStream->m_pTSConverter) ?
                pStream->m_pTSConverter->rtp2hxa_raw(ulRTPTime)
                : ulRTPTime;

            pTransport->setTimeStamp(unStreamNumber, 
                                     ulRTPTime, 
                                     TRUE);
            HX_RELEASE(pRTPPacket);
        }
        else
        {
            ulBaseTime = pPacket->GetTime();
            pTransport->setTimeStamp(unStreamNumber, ulBaseTime);
        }
        
        pTransport->setSequenceNumber(unStreamNumber, pPacket->m_uSequenceNumber);
        SetStreamStartTime(unStreamNumber, pPacket->GetTime());

        for (i = 0; i < m_unStreamCount; i++)
        {
            if (i != unStreamNumber && m_pStreams[i].m_bStreamRegistered)
            { 
                UINT32 ulRTPInfoTime =         (m_pStreams [i].m_pTSConverter) ?
                    m_pStreams [i].m_pTSConverter->hxa2rtp_raw(ulBaseTime) :
                    ulBaseTime;

                m_pStreams [i].m_pTransport->setTimeStamp(i, ulRTPInfoTime, TRUE);
                m_pStreams [i].m_pTransport->setSequenceNumber(i, m_pStreams [i].m_unSequenceNumber);

                HX_ASSERT(i == m_pStreams[i].m_unStreamNumber);
                SetStreamStartTime(m_pStreams[i].m_unStreamNumber, pPacket->GetTime());
            }
        }
        
        m_bRTPInfoRequired = FALSE;
    }

    if (pPacket->m_uPriority == 10)
    {
        pPacket->m_uReliableSeqNo  = ++pStream->m_unReliableSeqNo;
    }
    else
    {
        pPacket->m_uReliableSeqNo = pStream->m_unReliableSeqNo;
    }

    pStream->m_bPacketRequested = FALSE;

    HX_ASSERT(pPacket->GetStreamNumber() == pStream->m_unStreamNumber);
    if (pStream->m_pSink->PacketReady(pPacket) == HXR_BLOCKED)
    {
        UINT16 unStream = pStream->m_unStreamNumber;
        HX_ASSERT(!m_pBlockQueue [unStream]);
        HX_RELEASE(m_pBlockQueue [unStream]);
        m_pBlockQueue [unStream] = pPacket;
        pPacket->AddRef();

        return HXR_BLOCKED;
    }

    if (pStream->IsStreamDone())
    {
        ScheduleStreamDone(this, pStream->m_pTransport, 
                           pStream, pStream->m_unStreamNumber);
    }

    return HXR_OK;
}


HX_RESULT
StaticPushPacketFlow::SessionPacketReady(HX_RESULT ulStatus,
                                   IHXPacket* pPacket)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

STDMETHODIMP
StaticPushPacketFlow::SinkBlockCleared(UINT32 ulStream)
{
    if (SendResendPackets(ulStream) == HXR_BLOCKED)
    {
        return HXR_BLOCKED;
    }

    if (m_pBlockQueue [ulStream])
    {
        HX_RESULT hRes = HXR_OK;
        ServerPacket* pPacket = m_pBlockQueue [ulStream];
        pPacket->AddRef();

        if (pPacket->m_bTransportBlocked)
        {
            hRes = m_pStreams [ulStream].m_pSink->PacketReady(pPacket);

            if (hRes == HXR_OK)
            {
                HX_RELEASE(m_pBlockQueue [ulStream]);
            }
        }
        else
        {
            HX_RELEASE(m_pBlockQueue [ulStream]);
            
            if ((pPacket->m_bRateMgrBlocked) && 
                m_pBufferVerifier)
            {
                HX_RESULT hr = m_pBufferVerifier->PacketReady(pPacket);

                if (hr == HXR_BLOCKED)
                {
                    HX_ASSERT(!m_pBlockQueue [ulStream]);
                    m_pBlockQueue [ulStream] = pPacket;
                    m_pBlockQueue [ulStream]->AddRef();
                    HX_RELEASE(pPacket);
                    return hr;
                }
                
                if (hr == HXR_FAIL)
                {
                    return hr;
                }
            }
                
            hRes = SendToTransport(pPacket, 
                                   &m_pStreams [ulStream], 
                                   pPacket->GetASMRuleNumber());
        }


        HX_RELEASE(pPacket);

        if (hRes == HXR_BLOCKED)
        {

            return hRes;
        }
    }

    if (m_pSource)
    {
        return m_pSource->SinkBlockCleared(ulStream);
    }
        
    return HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::EnableTCPMode()
{
    return (m_pSource) ? m_pSource->EnableTCPMode() : HXR_OK;
}

/* Resend Management */
STDMETHODIMP
StaticPushPacketFlow::OnPacket(UINT16 uStreamNumber, BasePacket** ppPacket)
{
    if (uStreamNumber >= m_unStreamCount || m_bIsDone)
    {
        return HXR_FAIL;
    }
    
    HX_ASSERT(m_pStreams[uStreamNumber].m_bStreamRegistered);
    
    BasePacket* pPacket;
    ServerPacket* pServerPacket;

    BOOL bBlocked = FALSE;

    for (; (pPacket = *ppPacket); ppPacket++)
    {
        HX_ASSERT(pPacket);

        if (pPacket->IsResendRequested() && pPacket->m_uPriority != 10)
        {
            continue;
        }

        if (!pPacket->IsResendRequested())
        {
            pPacket->SetResendRequested();
        }

        HX_VERIFY(SUCCEEDED(pPacket->QueryInterface(IID_ServerPacket, (void **)0xffffd00d)));
        pServerPacket = (ServerPacket *)pPacket;

        if (!m_pResendVerifier || (HXR_OK == m_pResendVerifier->VerifyResend(pServerPacket)))
        {
            if ((bBlocked) || (FAILED((&m_pStreams[uStreamNumber])->m_pSink->
                                      PacketReady(pServerPacket))))
            {
                bBlocked = TRUE;

                HX_RELEASE(m_ppResendQueue [uStreamNumber][m_pResendWrite [uStreamNumber]]);
                m_ppResendQueue [uStreamNumber] [m_pResendWrite [uStreamNumber]] = 
                    pServerPacket; pServerPacket->AddRef();
                (++(m_pResendWrite [uStreamNumber])) %= RESEND_QUEUE_SIZE;
            }
        }
    }

    return HXR_OK;
}

inline HX_RESULT
StaticPushPacketFlow::SendResendPackets(UINT16 unStreamNumber)
{
    HX_ASSERT(m_pStreams[unStreamNumber].m_bStreamRegistered);
    if (unStreamNumber >= m_unStreamCount)
    {
        return HXR_FAIL;
    }
    
    if (!m_ppResendQueue [unStreamNumber][m_pResendRead[unStreamNumber]])
    {
        return HXR_OK;
    }

    HX_RESULT hRes = HXR_OK;

    while (SUCCEEDED(hRes) && m_ppResendQueue [unStreamNumber][m_pResendRead[unStreamNumber]])
    {
        if (!m_pResendVerifier || HXR_OK == m_pResendVerifier->VerifyResend(m_ppResendQueue [unStreamNumber][m_pResendRead[unStreamNumber]]))
        {
            hRes = (&m_pStreams[unStreamNumber])->
                m_pSink->PacketReady(m_ppResendQueue [unStreamNumber][m_pResendRead[unStreamNumber]]);
        }

        if (SUCCEEDED(hRes))
        {
            HX_RELEASE(m_ppResendQueue [unStreamNumber][m_pResendRead[unStreamNumber]]);
            (++(m_pResendRead[unStreamNumber])) %= RESEND_QUEUE_SIZE;
        }
    }

    return hRes;
}

STDMETHODIMP
StaticPushPacketFlow::Activate()
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

STDMETHODIMP
StaticPushPacketFlow::HandleSubscribe(INT32 lRuleNumber, UINT16 unStreamNumber)
{
    if (m_pStreams && (lRuleNumber >= m_pStreams[unStreamNumber].m_lNumRules))
    {
        return HXR_OK;
    }            

    if (m_pASMSource)
    {
        m_pASMSource->Subscribe(unStreamNumber, (UINT16)lRuleNumber);
    }

    return HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::HandleUnSubscribe(INT32 lRuleNumber, UINT16 unStreamNumber)
{
    if (m_pStreams && (lRuleNumber >= m_pStreams[unStreamNumber].m_lNumRules))
    {
        return HXR_OK;
    }            
    if (m_pASMSource)
    {
        m_pASMSource->Unsubscribe(unStreamNumber, (UINT16)lRuleNumber);
    }        

    return HXR_OK;
}

/* IHXPacketFlowControl Methods */
STDMETHODIMP
StaticPushPacketFlow::WantWouldBlock ()
{
    // The transport layer will handle would blocks itself.
    return HXR_OK;
}

/* IHXQoSSignalSink */
STDMETHODIMP 
StaticPushPacketFlow::Signal(THIS_ IHXQoSSignal* pSignal, IHXBuffer* pSessionId)
{
    if (!pSignal)
        return HXR_INVALID_PARAMETER;

    HX_QOS_SIGNAL ulId     = 0;
    IHXBuffer* pBuffer     = NULL;
    HX_RESULT rc = HXR_OK;
    StreamAdaptSignalData* pStreamAdaptSigData;

    pSignal->GetId(ulId);

    switch(ulId & HX_QOS_SIGNAL_ID_MASK)
    {
    case HX_QOS_SIGNAL_COMMON_STREAM_ADAPT_HDR:
        pSignal->GetValue(pBuffer);
        pStreamAdaptSigData = (StreamAdaptSignalData*)pBuffer->GetBuffer();
        HX_RELEASE(pBuffer);

        rc = UpdateTargetProtectionTime((StreamAdaptationParams *)pStreamAdaptSigData);
        break;

    case HX_QOS_SIGNAL_COMMON_MEDIA_RATE:
        {
            pBuffer = NULL;
            pSignal->GetValue(pBuffer);
            if (pBuffer)
            {
                RateSignal* pRateSignal = (RateSignal*)pBuffer->GetBuffer();
                if (m_ulCurrentCumulativeMediaRate != pRateSignal->m_ulCumulativeRate)
                {
                    m_pFlowMgr->ChangeDeliveryBandwidth(-1 * m_ulCurrentCumulativeMediaRate,TRUE);
                    m_ulCurrentCumulativeMediaRate = pRateSignal->m_ulCumulativeRate;
                    m_pFlowMgr->ChangeDeliveryBandwidth(m_ulCurrentCumulativeMediaRate,TRUE);
                }
            }
            HX_RELEASE(pBuffer);
        }
        break;

    default:
        break;
    }
    return rc;
}


STDMETHODIMP 
StaticPushPacketFlow::ChannelClosed(THIS_ IHXBuffer* pSessionId)
{
    HX_RELEASE(m_pSignalBus);
        return HXR_OK;
}

/* IHXStreamAdaptationSetup */
STDMETHODIMP
StaticPushPacketFlow::GetStreamAdaptationScheme (THIS_
                                 REF(StreamAdaptationSchemeEnum) /* OUT */ enumAdaptScheme )
{
    enumAdaptScheme = m_enumStreamAdaptScheme;

    return HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::SetStreamAdaptationScheme (THIS_
                                StreamAdaptationSchemeEnum enumAdaptScheme)
{
    m_enumStreamAdaptScheme = enumAdaptScheme;

    switch (enumAdaptScheme)
    {
    case ADAPTATION_NONE:
    {
        IHXBuffer* pBufferMode = NULL;

        if (SUCCEEDED(m_pQoSConfig->GetConfigBuffer(QOS_CFG_RM_BUFF_MOD, pBufferMode))
                && pBufferMode && pBufferMode->GetBuffer())
        {
            if (!strncasecmp((const char*)pBufferMode->GetBuffer(), "ANNEXG", pBufferMode->GetSize()))
            {
                m_enumStreamAdaptScheme = ADAPTATION_ANNEXG;
            }
        }

        break;
    }
    default:
        break;
    }

    return HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::SetStreamAdaptationParams (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams)
{
    if (m_enumStreamAdaptScheme == ADAPTATION_HLX_AGGR)
    {
        m_pAggRateAdaptParams = pStreamAdaptParams;
    }
    else
    {
        if (pStreamAdaptParams->m_unStreamNum < m_unStreamCount)
        {
            return m_pStreams[pStreamAdaptParams->m_unStreamNum].SetStreamAdaptation(
                                                    m_enumStreamAdaptScheme,
                                                    pStreamAdaptParams);
        }
        else
        {
            return HXR_FAIL;
        }
    }

    return HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::GetStreamAdaptationParams (THIS_
                                 UINT16 unStreamNum,
                                 REF(StreamAdaptationParams) /* OUT */ streamAdaptParams)
{
    if (unStreamNum < m_unStreamCount)
    {
        if (m_pStreams[unStreamNum].m_pStreamAdaptParams)
        {
            streamAdaptParams = *(m_pStreams[unStreamNum].m_pStreamAdaptParams);
            return HXR_OK;
        }
        else
        {
            return HXR_FAIL;
        }

    }
    else
    {
        return HXR_FAIL;
    }
}

STDMETHODIMP
StaticPushPacketFlow::UpdateStreamAdaptationParams (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams)
{
    StreamAdaptationParams* pSendParams = NULL;
    HX_RESULT rc = HXR_FAIL;

    if (m_enumStreamAdaptScheme == ADAPTATION_HLX_AGGR)
    {
        if (!m_pAggRateAdaptParams)
        {
            HX_ASSERT(!"SPPF: Aggregate adaptation but no aggregate params!");
            return HXR_FAIL;
        }

        *m_pAggRateAdaptParams = *pStreamAdaptParams;
        pSendParams = m_pAggRateAdaptParams;
    }
    else 
    {
        UINT16  unStreamNum = pStreamAdaptParams->m_unStreamNum;

        if (unStreamNum < m_unStreamCount 
        &&  m_pStreams[unStreamNum].m_pStreamAdaptParams && pStreamAdaptParams)
        {
            *(m_pStreams[unStreamNum].m_pStreamAdaptParams)
                                            = *pStreamAdaptParams;
        
            pSendParams = m_pStreams[unStreamNum].m_pStreamAdaptParams;
        }
    }

    if (pSendParams)
    {
        IHXStreamAdaptationSetup* pStreamAdaptSetup = NULL;
        if (m_pRateManager 
        &&  SUCCEEDED(rc = m_pRateManager->QueryInterface(IID_IHXStreamAdaptationSetup,
                                                         (void**)&pStreamAdaptSetup)))
        {
            rc = pStreamAdaptSetup->UpdateStreamAdaptationParams(pSendParams);
            HX_RELEASE(pStreamAdaptSetup);
        }
    }

    return rc;
}

STDMETHODIMP
StaticPushPacketFlow::UpdateTargetProtectionTime (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams)
{
    StreamAdaptationParams* pSendParams = NULL;
    HX_RESULT rc = HXR_FAIL;

    if (m_enumStreamAdaptScheme == ADAPTATION_HLX_AGGR)
    {
        if (!m_pAggRateAdaptParams)
        {
            HX_ASSERT(!"SPPF: Aggregate adaptation but no aggregate params!");
            return HXR_FAIL;
        }

        m_pAggRateAdaptParams->m_ulTargetProtectionTime = pStreamAdaptParams->m_ulTargetProtectionTime;
        pSendParams = m_pAggRateAdaptParams;
    }
    else 
    {
        UINT16  unStreamNum = pStreamAdaptParams->m_unStreamNum;

        if (unStreamNum < m_unStreamCount 
        &&  m_pStreams[unStreamNum].m_pStreamAdaptParams && pStreamAdaptParams)
        {
            m_pStreams[unStreamNum].m_pStreamAdaptParams->m_ulTargetProtectionTime
                                            = pStreamAdaptParams->m_ulTargetProtectionTime;
            pSendParams = m_pStreams[unStreamNum].m_pStreamAdaptParams;
        }
    }

    if (pSendParams)
    {
        IHXStreamAdaptationSetup* pStreamAdaptSetup = NULL;
        if (m_pRateManager 
        &&  SUCCEEDED(rc = m_pRateManager->QueryInterface(IID_IHXStreamAdaptationSetup,
                                                         (void**)&pStreamAdaptSetup)))
        {
            rc = pStreamAdaptSetup->UpdateTargetProtectionTime(pSendParams);
            HX_RELEASE(pStreamAdaptSetup);
        }
    }

    return rc;
}

/* IHXQoSLinkCharSetup methods */
STDMETHODIMP
StaticPushPacketFlow::SetLinkCharParams (THIS_
                                 LinkCharParams* /* IN */ pLinkCharParams)
{
    UINT16 unStreamNum = 0;

    if (pLinkCharParams->m_bSessionAggregate)
    {
        if (!m_pSessionAggrLinkCharParams)
        {
            m_pSessionAggrLinkCharParams = new LinkCharParams;
        }
        *m_pSessionAggrLinkCharParams = *pLinkCharParams;
    }
    else if ( (unStreamNum = pLinkCharParams->m_unStreamNum) < m_unStreamCount )
    {
        m_bStreamLinkCharSet = TRUE;
        if (!m_pStreams[unStreamNum].m_pLinkCharParams)
        {
            m_pStreams[unStreamNum].m_pLinkCharParams = new LinkCharParams;
        }

        *m_pStreams[unStreamNum].m_pLinkCharParams = *pLinkCharParams;
    }
    else
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}

STDMETHODIMP
StaticPushPacketFlow::GetLinkCharParams (THIS_
                                 UINT16 unStreamNum,
                                 REF(LinkCharParams) /* OUT */ linkCharParams)
{
    if (m_pSessionAggrLinkCharParams)
    {
        linkCharParams = *m_pSessionAggrLinkCharParams;
        return HXR_OK;
    }
    else if (unStreamNum < m_unStreamCount)
    {
        if (m_pStreams[unStreamNum].m_pLinkCharParams)
        {
            linkCharParams = *(m_pStreams[unStreamNum].m_pLinkCharParams);
            return HXR_OK;
        }
        else
        {
            return HXR_FAIL;
        }
    }

    return HXR_FAIL;
}

//
// Live packet flows don't compete with others for schedulling so
// we exclude ourselves
//
BOOL
StaticPushPacketFlow::Compare(UINT32 /*ulMsecSinceLastRecalc*/,
                        REF(BasicPacketFlow*) /*pBestFlow*/)
{
    return FALSE;
}

// StaticPushPacketFlow::SendPacket
//
// this is only for flows that pull packets, we should really
// merge SendPacket and TransmitPacket, former for Pulling, latter
// for pushing
BOOL
StaticPushPacketFlow::SendPacket(UINT32 ulActualDeliveryRate,
                           REF(UINT32) ulPacketSize)
{

    ASSERT(0);
    return FALSE;
}
