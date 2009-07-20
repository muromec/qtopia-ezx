/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rdt_udp.cpp,v 1.10 2007/03/22 19:16:56 tknox Exp $
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
#include "hxtypes.h"
#include "hxassert.h"
#include "debug.h"
#include "hxcom.h"
#include "hxmarsh.h"
#include "hxstrutl.h"
#include "netbyte.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxsbuffer.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "netbyte.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxdeque.h"
#include "hxbitset.h"
#include "timebuff.h"
#include "timeval.h"
#include "tconverter.h"
#include "rtptypes.h"
#include "hxcorgui.h"
#include "hxqosinfo.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "qos_tran_rdt_metrics.h"
#include "qos_cfg_names.h"
#include "errdbg.h"
#include "nettypes.h"

#include "ntptime.h"

#include "rtspif.h"
#include "rtsptran.h"
#include "transport.h"
#include "rdt_base.h"
#include "rdt_udp.h"
#include "tngpkt.h"
#include "basepkt.h"
#include "hxtbuf.h"
#include "transbuf.h"
#include "pkthndlr.h"   // in rtpmisc for RTCP routine
#include "hxprefs.h"    // IHXPreferences
#include "hxmime.h"
#include "hxcore.h"
#include "source.h"
#include "servpckts.h"
#include "servbuffer.h"
#include "smallbuffer.h"
#include "globals.h"
#include "hxheap.h"
#include "servbuffer.h"
#include "abdbuf.h" //random data for ABD packets.

#include "servbuffer.h"
#include "qos_signal.h"


#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

/*
 * RDTUDPTransport methods
 */

RDTUDPTransport::RDTUDPTransport(BOOL bIsServer /* UNUSED */, BOOL bIsSource /* UNUSED */,
                                 RTSPTransportTypeEnum lTransportType,
                                 UINT32 uFeatureLevel /* = 0 */,
                                 BOOL bDisableResend /* = FALSE */)
    : RDTBaseTransport(bIsSource, uFeatureLevel)
    , m_pPeerAddr(NULL)
    , m_bCallbackPending(FALSE)
    , m_pSeekCallback(NULL)
    , m_pTransportInfoCallback (NULL)
    , m_ulBackToBackTime(0)
    , m_pMCastUDPSocket(0)
    , m_ulCurrentMulticastAddress(0)
    , m_ulCurrentMulticastPort(0)
    , m_bDisableResend(bDisableResend)
    , m_bBCM(FALSE)
    , m_fRTT (0)
    , m_bFirstPacket (FALSE)
    , m_ulRTTProbeFrequency (RDT_DEFAULT_RTT_FREQ)
    , m_bPktAggregationEnabled(TRUE)
    , m_bSuspendRTTProbeOnPause(TRUE)
{
    m_lTransportType = lTransportType;
    m_ucPacketQueuePos  = 0;
    m_ulPacketQueueSize = 0;
    m_pQoSMetrics = new QoSRDTMetrics();

#ifdef DEBUG
    m_packets_since_last_drop = 0;
    m_drop_packets = FALSE;
#endif /* DEBUG */
}

RDTUDPTransport::~RDTUDPTransport()
{
    Done();

    delete m_pQoSMetrics;
}

STDMETHODIMP
RDTUDPTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXServerPauseAdvise))
    {
        AddRef();
        *ppvObj = (IHXServerPauseAdvise*)this;
        return HXR_OK;
    }
    else
    {
        return RDTBaseTransport::QueryInterface (riid, ppvObj);
    }
}

STDMETHODIMP_(UINT32)
RDTUDPTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RDTUDPTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

// IHXServerPauseAdvise methods

STDMETHODIMP
RDTUDPTransport::OnPauseEvent(BOOL bPause)
{
    // suspend/resume the info packets

    if (m_pTransportInfoCallback && m_bSuspendRTTProbeOnPause)
    {
        if (bPause)
        {
            m_pTransportInfoCallback->Suspend();
        }
        else
        {
            m_pTransportInfoCallback->Resume();
        }
    }

    return HXR_OK;
}

void
RDTUDPTransport::Done()
{
    if (m_bDone)
        return;

    m_bDone = TRUE;

    if (m_pSeekCallback &&
        m_pSeekCallback->m_bIsCallbackPending &&
        m_pScheduler)
    {
        m_pSeekCallback->m_bIsCallbackPending = FALSE;
        m_pScheduler->Remove(m_pSeekCallback->m_Handle);
    }

    if (m_pTransportInfoCallback)
    {
        m_pTransportInfoCallback->Stop();
        HX_RELEASE( m_pTransportInfoCallback );
    }

    HX_RELEASE(m_pSeekCallback);

    HX_RELEASE(m_pMCastUDPSocket);

    RDTBaseTransport::Done();

    HX_RELEASE(m_pPeerAddr);
}

void
RDTUDPTransport::addStreamInfo(RTSPStreamInfo* pStreamInfo,
                               UINT32 ulBufferDepth)
{
    Transport::addStreamInfo(pStreamInfo, ulBufferDepth);
    if (pStreamInfo && !m_bDisableResend)
    {
        m_pStreamHandler->createResendBuffer(pStreamInfo->m_streamNumber,
                                             m_wrapSequenceNumber);
    }
}

HX_RESULT
RDTUDPTransport::init(IUnknown* pContext,
                       IHXSocket* pSocket,
                       IHXRTSPTransportResponse* pResp)
{
    HX_ASSERT( pContext );
    HX_ASSERT( pSocket );

    pSocket->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);

    // check registry to see if we suspend RTT on stream PAUSE

    IHXRegistry* pRegistry = NULL;
    if (HXR_OK == pContext->QueryInterface(IID_IHXRegistry,
                                           (void**) &pRegistry))
    {
        INT32 temp = 0;
        if (HXR_OK == pRegistry->GetIntByName("config.SuspendRTTOnPause",
                                              temp))
        {
            m_bSuspendRTTProbeOnPause = temp;
        }
        HX_RELEASE( pRegistry );
    }

    HX_RESULT hresult = RDTBaseTransport::init(pContext, pSocket, pResp);
    if (hresult != HXR_OK)
    {
        return hresult;
    }

    m_pSeekCallback = new SeekCallback(this);
    m_pSeekCallback->AddRef();

#ifdef DEBUG
    if (debug_func_level() & DF_DROP_PACKETS)
    {
        m_drop_packets = TRUE;
    }
#endif /* DEBUG */

    return HXR_OK;
}

void
RDTUDPTransport::Reset()
{
    // Do nothing
}

void
RDTUDPTransport::Restart()
{
    // Do nothing
}

STDMETHODIMP
RDTUDPTransport::SignalBusReady (HX_RESULT hResult, IHXQoSSignalBus* pBus,
                                 IHXBuffer* pSessionId)
{
    if (FAILED(hResult))
    {
        HX_ASSERT(0);
        return HXR_OK;
    }

    IHXQoSProfileConfigurator* pConfig = NULL;
    INT32 lTemp                        = 0;

    m_pSignalBus = pBus;
    m_pSignalBus->AddRef();

    if (FAILED(m_pSignalBus->QueryInterface(IID_IHXQoSTransportAdaptationInfo,
                                            (void**)&m_pQoSInfo)))
    {
        m_pQoSInfo = NULL;
    }


    HX_VERIFY(SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSProfileConfigurator,
                                                     (void**)&pConfig)));

    if (m_uFeatureLevel >= 3)
    {
        m_pTransportInfoCallback = new TransportInfoCallback (this);
        m_pTransportInfoCallback->AddRef();

        if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_TRAN_RDT_RTT, lTemp)))
        {
            m_ulRTTProbeFrequency = (UINT32)lTemp;
        }
        else
        {
            m_ulRTTProbeFrequency = RDT_DEFAULT_RTT_FREQ;
        }
    }

    lTemp = 0;
    if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_TRAN_RDT_PKT_AGG_ENABLE, lTemp)))
    {
        m_bPktAggregationEnabled = lTemp != 0 ? TRUE : FALSE;
    }
    else
    {
        m_bPktAggregationEnabled = TRUE;
    }

    HX_RELEASE(pConfig);
    return HXR_OK;
}

void
RDTUDPTransport::setPeerAddr(IHXSockAddr* pAddr)
{
    HX_ASSERT(!(m_pPeerAddr) && pAddr != NULL);
    m_pPeerAddr = pAddr;
    m_pPeerAddr->AddRef();
}

STDMETHODIMP
RDTUDPTransport::SetDeliveryBandwidth(UINT32 /* UNUSED */, UINT32 ulBandwidth)
{
    UINT32 ulMs = SetBandwidthAndLatency (ulBandwidth);
    setLatencyPeriod(ulMs);

    return HXR_OK;
}

STDMETHODIMP
RDTUDPTransport::SetAggregationLimits(UINT32* pAggregStat,
                                      UINT32 ulAggregateTo, UINT32 ulAggregateHighest)
{
    if (m_bPktAggregationEnabled)
    {
        m_pAggregStat        = pAggregStat;
        m_ulAggregateTo      = ulAggregateTo;
        m_ulAggregateHighest = ulAggregateHighest;
    }
    return HXR_OK;
}

STDMETHODIMP
RDTUDPTransport::SetSource(IHXServerPacketSource* pSource)
{
    if (!pSource)
        return HXR_INVALID_PARAMETER;

    HX_RELEASE(m_pSource);
    m_pSource = pSource;
    pSource->AddRef();

    return HXR_OK;
}

STDMETHODIMP
RDTUDPTransport::PacketReady(ServerPacket* pPacket)
{
    if (m_bDone)
    {
        return HXR_OK;
    }

    UpdatePacketStats(pPacket);

    if (!m_bFirstPacket)
    {
        m_bFirstPacket = TRUE;

        if (m_pTransportInfoCallback)
        {
            m_pTransportInfoCallback->Start(m_ulRTTProbeFrequency);
        }
    }

    // TimeStamp delivered sources don't currently support BackToBack pkts
    if (!pPacket->IsTSD())
    {
        m_ulBackToBackCounter++;
        if (m_ulBackToBackCounter >= m_ulBackToBackFreq)
        {
            m_ulBackToBackCounter = 0;
            m_bAttemptingBackToBack = TRUE;
        }
    }

    // In addition to packet aggregation, we use the PacketQueue for handling
    // back-to-back packets.  The second of any back-to-back packet pair MUST
    // not be aggregated (the client's RTSP layer can't handle that).
    // This allows us to send the 2 back-to-back packets if they are
    // available, but tells us to keep aggregating, if we can still fit
    // more data into the first of the back to back pair.
    UINT32 ulPacketSize = pPacket->GetSize();

    if (m_bAttemptingBackToBack && m_ucPacketQueuePos > 0 &&
        (m_ulPacketQueueSize > m_ulAggregateTo ||
         m_ulPacketQueueSize + ulPacketSize > m_ulAggregateHighest))
    {
        // send the packet queue including the first of the BTB pair
        m_pPacketQueue[m_ucPacketQueuePos - 1]->m_bBackToBack = TRUE;
        UINT32 ulQueueSize = SendPacketQueue();

        // now send the second of the BTB pair
        sendPacket(pPacket);

        ulPacketSize = ulQueueSize + ulPacketSize;

        m_bAttemptingBackToBack = FALSE;
    }
    else
    {
        // If queueing this packet will cause us to go over our maximum
        // allowed aggregation size, then we should flush the aggregation
        // queue and then queue this packet.
        // Don't do this if there is nothing in the packet queue.  This
        // case means that a single packet from the file format is larger
        // then ulAggregateHighest.  In this case, we allow the else code
        // to correctly handle this.
        if (!m_bAttemptingBackToBack && m_ulPacketQueueSize &&
            m_ulPacketQueueSize + ulPacketSize > m_ulAggregateHighest)
        {
            UINT32 ulQueueSize = SendPacketQueue();
            ASSERT(ulQueueSize);

            // QueuePacket will take control of pPacket (Screw COM)
            QueuePacket(pPacket, ulPacketSize);
            ulPacketSize = ulQueueSize;
        }
        else
        {
            QueuePacket(pPacket, ulPacketSize);

            // Flush the aggregation queue, if we have reached the target
            // size and we aren't trying to do any back-to-back-packets.
            if (m_ulPacketQueueSize > m_ulAggregateTo && !m_bAttemptingBackToBack)
            {
                ulPacketSize = SendPacketQueue();
                ASSERT(ulPacketSize);
            }
        }
    }

    return HXR_OK;
}

STDMETHODIMP
RDTUDPTransport::SourceDone()
{
    HX_RELEASE(m_pSource);
    return HXR_OK;
}

HX_RESULT
RDTUDPTransport::sendPackets(BasePacket** pPacket)
{
    return sendPacketsImpl(pPacket);
}

HX_RESULT
RDTUDPTransport::sendPackets(ServerPacket** pPacket)
{
    return sendPacketsImpl(pPacket);
}

template<typename PacketT>
HX_RESULT
RDTUDPTransport::sendPacketsImpl(PacketT** pPacket)
{
    // XXXAAK -- Wed Sep 10 16:34:44 PDT 2003
    // we need to figure out why this method gets called after Done()
    // this is a defensive check that prevents a CA
    if (m_bDone)
        return HXR_FAIL;

    HX_ASSERT (pPacket);
    HX_ASSERT ((*pPacket));

    HX_RESULT hresult = HXR_OK;
    IHXBuffer* WriteVec[HX_IOV_MAX];
    UINT32 ulSize = 0;
    IHXBuffer** ppCur = WriteVec;

    while (*pPacket && SUCCEEDED(hresult) && ulSize < HX_IOV_MAX)
    {
#ifdef _DEBUG
        if (!m_drop_packets || ++m_packets_since_last_drop % 10 != 0)
#endif /* _DEBUG */
        {
            hresult = VectorPacket(*(pPacket), ppCur);
            if(hresult == HXR_OK)
            {
                ulSize += 2;
                ppCur += 2;
            }
        }
        pPacket++;
    }

    // If this assert fires, it's because we fell off the loop due to
    // too many packets.
    HX_ASSERT(ulSize <= HX_IOV_MAX);

    if(ulSize > 0)
    {
        m_pSocket->WriteV(ulSize, WriteVec);
        for(UINT32 i = 0; i < ulSize; i++)
        {
            WriteVec[i]->Release();
        }
    }

    return hresult;
}

HX_RESULT
RDTUDPTransport::MakeRDTHeader(BasePacket* pBasePacket, IHXBuffer* pHeader,
                               BOOL bAggregating)
{
    IHXPacket* pPacket = pBasePacket->PeekPacket();

    if (!pPacket)
    {
        HX_ASSERT(0);
        return HXR_UNEXPECTED;
    }

    if (pPacket->IsLost())
    {
        HX_ASSERT(0);
        return HXR_OK;
    }

    RTSPStreamData* pStreamData;
    TNGDataPacket pkt;
    TNGLatencyReportPacket rpt;
    IHXBuffer* pBuffer = pPacket->GetBuffer();
    UINT32 bufLen = pBuffer->GetSize();
    UINT32 offset = 0;
    UINT32 report_len = 0;
    UINT16 streamNumber = pPacket->GetStreamNumber();

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    /*
     *  IHXRTPPacket support:
     *    use rtp packets for presentation timestamp
     *  if available
     */
    if (pStreamData->m_bFirstPacket)
    {
        IHXRTPPacket* pRTPPacket = NULL;
        pStreamData->m_bUsesRTPPackets = (pPacket->QueryInterface(
                                            IID_IHXRTPPacket,
                                            (void**) &pRTPPacket)
                                          == HXR_OK);
        if (pStreamData->m_bUsesRTPPackets)
        {
            HX_ASSERT(pRTPPacket == pPacket);
            if (pRTPPacket != pPacket)
            {
                HX_ASSERT(0);
                HX_RELEASE(pBuffer);
                return HXR_INVALID_PARAMETER;
            }
        }
        HX_RELEASE(pRTPPacket);
        pStreamData->m_bFirstPacket = FALSE;
    }

    UINT32 ulReportPacketSize = (createLatencyReportPacket(&rpt)) ?
        rpt.static_size() : 0;

    if ((ulReportPacketSize + pkt.static_size()) > MAX_SMALLBUFFER_LENGTH)
    {
        HX_ASSERT(0);
        HX_RELEASE(pBuffer);
        return HXR_INVALID_PARAMETER;
    }

    pHeader->SetSize(ulReportPacketSize + pkt.static_size());

    UINT8* pHdr = (UINT8*)pHeader->GetBuffer();

    if (ulReportPacketSize != 0)
    {
        rpt.pack(pHdr, offset);
    }

    pHdr += offset;
    offset = 0;

    report_len =  pHdr - ((UINT8*)pHeader->GetBuffer());

    UINT32 packetLen = 0;
    pkt.length_included_flag = bAggregating;

    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;

        //XXXVS: RealPlayer requires the packet stream id be the
        // stream group number for contents where 
        // StreamGroupCount != StreamCount
        pkt.stream_id_expansion = pStreamData->m_streamGroupNumber;
    }
    else
    {
        //XXXVS: RealPlayer requires the packet stream id be the
        // stream group number for contents where 
        // StreamGroupCount != StreamCount
        pkt.stream_id = pStreamData->m_streamGroupNumber;
    }

    /* We'll always include the reliable flag for now */
    pkt.need_reliable_flag = 1; // XXXSMP pStreamData->m_bNeedReliable;
    /*
     * XXXSMP - We don't need is_reliable.  We can imply this from the increase
     * of both sequence numbers.
     */
    pkt.is_reliable = pBasePacket->m_uPriority == 10 ? 1 : 0;
    //XXXBAB

    pkt.seq_no = pStreamData->m_seqNo = pBasePacket->m_uSequenceNumber;
    if(pkt.need_reliable_flag)
    {
        pkt.total_reliable = pStreamData->m_reliableSeqNo
                           = pBasePacket->m_uReliableSeqNo;
    }

    /*
     *  Timestamp: use rtp timestamp as presentation
     *  timestamp, if available.
     */
    if ((pStreamData->m_bUsesRTPPackets) && (pStreamData->m_pTSConverter))
    {
        pkt.timestamp = pStreamData->m_lastTimestamp = pStreamData->m_pTSConverter->
          rtp2hxa( ((IHXRTPPacket*) pPacket)->GetRTPTime() );
    }
    else
    {
        pkt.timestamp = pStreamData->m_lastTimestamp = pPacket->GetTime();
    }

    UINT16 ulRuleNumber = pPacket->GetASMRuleNumber();
    pkt.asm_rule_number = (ulRuleNumber > 63) ? 63 : ulRuleNumber;
    pkt.back_to_back_packet = pBasePacket->m_bBackToBack;

    pkt.extension_flag = 0;
    if (m_bUseExtPkts)
    {
        pkt.extension_flag = 1;
        pkt.version = RDTV4;
        pkt.reserved = 0;
        pkt.rsid = m_ulRDTSessionID;
    }

    if (ulRuleNumber > 63)
    {
        pkt.asm_rule_number_expansion = ulRuleNumber;
    }

    pkt.data.data = 0;
    pkt.data.len = HX_SAFEUINT(pBuffer->GetSize());

    pkt.pack(pHdr, packetLen);

    packetLen += report_len;
    packetLen -= bufLen;
    pHeader->SetSize(packetLen);

    HX_RELEASE(pBuffer);

    return HXR_OK;
}

HX_RESULT
RDTUDPTransport::VectorPacket(BasePacket* pBasePacket, IHXBuffer** ppWriteHere)
{
    if (!m_bFirstPacket)
    {
        m_bFirstPacket = TRUE;

        if (m_pTransportInfoCallback)
        {
            m_pTransportInfoCallback->Start(m_ulRTTProbeFrequency);
        }
    }

    IHXPacket* pPacket = pBasePacket->PeekPacket();
    HX_ASSERT (pPacket);

    UINT16 streamNumber = pPacket->GetStreamNumber();
    RTSPStreamData* pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if (pPacket->IsLost())
    {
        return HXR_FAIL;
    }

    if(!pStreamData)
    {
        return HXR_UNEXPECTED;
    }

    IHXBuffer* pPayload = pPacket->GetBuffer();
    UINT32 bufLen = pPayload->GetSize();
    SmallBuffer* pHeaderBuffer = new SmallBuffer(TRUE);

    m_pQoSMetrics->Enqueue (streamNumber,
                          pBasePacket->m_uSequenceNumber,
                          bufLen);

    if (!pStreamData->m_packetSent)
    {
        pStreamData->m_packetSent = TRUE;
    }

    RTSPResendBuffer* pResendBuffer = pStreamData->m_pResendBuffer;

    if (pResendBuffer && (!pBasePacket->IsResendRequested()))
    {
        pResendBuffer->Add(pBasePacket);
        pResendBuffer->DiscardExpiredPackets(FALSE, pBasePacket->GetTime());
    }

    if (FAILED(MakeRDTHeader(pBasePacket, pHeaderBuffer, ppWriteHere != NULL)))
    {
        HX_ASSERT(0);
        HX_RELEASE(pHeaderBuffer);
        HX_RELEASE(pPayload);
        return HXR_FAIL;
    }

    if(ppWriteHere)
    {
        *(ppWriteHere) = pHeaderBuffer;
        *(ppWriteHere + 1) = pPayload;
    }
    else
    {

        IHXBuffer* pWriteVec[2];
        pWriteVec[0] = pHeaderBuffer;
        pWriteVec[1] = pPayload;

#ifdef _DEBUG
        if (!m_drop_packets || ++m_packets_since_last_drop % 10 != 0)
#endif /* _DEBUG */
        {
            m_pSocket->WriteV(2, pWriteVec);
        }

        HX_RELEASE(pHeaderBuffer);
        HX_RELEASE(pPayload);
    }
    updateQoSInfo(bufLen);
    return HXR_OK;
}

HX_RESULT
RDTUDPTransport::sendPacket(BasePacket* pBasePacket)
{
    return sendPacketImpl(pBasePacket);
}

HX_RESULT
RDTUDPTransport::sendPacket(ServerPacket* pBasePacket)
{
    return sendPacketImpl(pBasePacket);
}

template<typename PacketT>
HX_RESULT
RDTUDPTransport::sendPacketImpl(PacketT* pPacket)
{
    // XXXAAK -- Wed Sep 10 16:34:44 PDT 2003
    // this check is being done as a defensive check because a CA happens
    // inside sendPackets(BasePackets**)
    if (m_bDone)
        return HXR_FAIL;

    HX_ASSERT (pPacket);

    if (m_bMulticast)
    {
        return sendMulticastPacket(pPacket);
    }

    return VectorPacket(pPacket);
}

HX_RESULT
RDTUDPTransport::sendMulticastPacket(BasePacket* pBasePacket)
{
    RTSPStreamData* pStreamData;
    IHXPacket* pPacket;
    IHXBuffer* pBuffer;

    pPacket = pBasePacket->GetPacket();

    if(!pPacket)
    {
        return HXR_UNEXPECTED;
    }

    // copied from ::sendPacket()
    UINT32 bufLen;
    if (pPacket->IsLost())
    {
        pBuffer = 0;
        bufLen = 0;
    }
    else
    {
        pBuffer = pPacket->GetBuffer();
        bufLen = pBuffer->GetSize();
    }

    UINT16 streamNumber = pPacket->GetStreamNumber();

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(!pStreamData)
    {
        return HXR_UNEXPECTED;
    }

    if (!pStreamData->m_packetSent)
    {
        pStreamData->m_packetSent = TRUE;
    }

    RTSPResendBuffer* pResendBuffer = pStreamData->m_pResendBuffer;


    if (pResendBuffer && (!pBasePacket->IsResendRequested()))
    {
        pResendBuffer->Add(pBasePacket);
        pResendBuffer->DiscardExpiredPackets(FALSE, pPacket->GetTime());
    }

    // copied from ::sendPacket()
    if (pPacket->IsLost())
    {
        return HXR_OK;
    }


    HX_RESULT hresult = HXR_OK;

    TNGDataPacket pkt;
    UINT32 report_offset = 0;
    TNGLatencyReportPacket rpt;

    BYTE* packet;
    IHXBuffer* pSendBuffer = NULL;
    if(!(SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pSendBuffer))) || !pSendBuffer)
    {
        return HXR_OUTOFMEMORY;
    }

    if(createLatencyReportPacket(&rpt))
    {
        if (!SUCCEEDED(pSendBuffer->SetSize(rpt.static_size() + pkt.static_size() + pBuffer->GetSize())))
        {
            return HXR_OUTOFMEMORY;
        }
        packet = pSendBuffer->GetBuffer();
        /*
         * report_offset gets set in pack.
         */
        rpt.pack(packet, report_offset);
    }
    else
    {
        if (!SUCCEEDED(pSendBuffer->SetSize(pkt.static_size() + pBuffer->GetSize())))
        {
            return HXR_OUTOFMEMORY;
        }
        packet = pSendBuffer->GetBuffer();
    }
    UINT32 packetLen = 0;

    pkt.length_included_flag = 0;

    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;

        //XXXVS: RealPlayer requires the packet stream id be the
        // stream group number for contents where 
        // StreamGroupCount != StreamCount
        pkt.stream_id_expansion = pStreamData->m_streamGroupNumber;
    }
    else
    {
        //XXXVS: RealPlayer requires the packet stream id be the
        // stream group number for contents where 
        // StreamGroupCount != StreamCount
        pkt.stream_id = pStreamData->m_streamGroupNumber;
    }

    /* We'll always include the reliable flag for now */
    pkt.need_reliable_flag = 1; // XXXSMP pStreamData->m_bNeedReliable;
    /*
     * XXXSMP - We don't need is_reliable.  We can imply this from the increase
     * of both sequence numbers.
     */
    pkt.is_reliable = pBasePacket->m_uPriority == 10 ? 1 : 0;
    //XXXBAB

    pkt.seq_no = pStreamData->m_seqNo = pBasePacket->m_uSequenceNumber;
    if(pkt.need_reliable_flag)
    {
        pkt.total_reliable = pStreamData->m_reliableSeqNo
                           = pBasePacket->m_uReliableSeqNo;
    }

    /*
     *  Timestamp: use rtp timestamp as presentation
     *  timestamp, if available.
     */
    if ((pStreamData->m_bUsesRTPPackets) && (pStreamData->m_pTSConverter))
    {
        pkt.timestamp = pStreamData->m_lastTimestamp = pStreamData->m_pTSConverter->
            rtp2hxa( ((IHXRTPPacket*) pPacket)->GetRTPTime() );
    }
    else
    {
        pkt.timestamp = pPacket->GetTime();
    }

    UINT16 ulRuleNumber = pPacket->GetASMRuleNumber();
    pkt.asm_rule_number = (ulRuleNumber > 63) ? 63 : ulRuleNumber;
    pkt.back_to_back_packet = pBasePacket->m_bBackToBack;
    pkt.extension_flag = 0;

    if (ulRuleNumber > 63)
    {
        pkt.asm_rule_number_expansion = ulRuleNumber;
    }

    pkt.data.data = (INT8*)pBuffer->GetBuffer();
    pkt.data.len = HX_SAFEUINT(pBuffer->GetSize());
    /*
     * write to the buffer after the report.
     */
    pkt.pack(packet + report_offset, packetLen);

#if 0
    DPRINTF(D_INFO, ("Pkt: stream=%d seq=%d ts=%ld session=%s\n",
        pkt.stream_id, pkt.seq_no, pkt.timestamp, (const char*)m_sessionID));
#endif

#ifdef DEBUG
    if (m_drop_packets && ++m_packets_since_last_drop % 10 == 0)
    {
        goto TNGsendContinue;
    }
#endif /* DEBUG */

    updateQoSInfo(bufLen);
    /*
     *  When sending out, the total size of the physical packet is
     *  to combination of the sizes of the logical packets.  In this
     *  case, the BasePacket size and report packet size.
     */

    // writePacket will call pSendBuffer->Release()
    hresult = writePacket(pSendBuffer);

#ifdef DEBUG
TNGsendContinue:
#endif

    pBuffer->Release();
    pPacket->Release();
    return hresult;
}

HX_RESULT
RDTUDPTransport::sendRTTResponsePacket(UINT32 secs, UINT32 uSecs)
{
    HX_RESULT hresult = HXR_OK;
    TNGRTTResponsePacket pkt;

    IHXBuffer* pSendBuffer = NULL;
    if(!(SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pSendBuffer))) || !pSendBuffer)
    {
        return HXR_OUTOFMEMORY;
    }

    if (!SUCCEEDED(pSendBuffer->SetSize(pkt.static_size())))
    {
        return HXR_OUTOFMEMORY;
    }
    BYTE* packet = pSendBuffer->GetBuffer();
    UINT32 packetLen = 0;

    pkt.dummy0 = 0;
    pkt.dummy1 = 0;
    pkt.dummy2 = 0;
    pkt.dummy3 = 0;
    pkt.packet_type = (UINT16)0xff04;
    pkt.timestamp_sec = secs;
    pkt.timestamp_usec = uSecs;
    pkt.pack(packet, packetLen);
    // writePacket will call pSendBuffer->Release()
    hresult = writePacket(pSendBuffer);

    return hresult;
}

HX_RESULT
RDTUDPTransport::sendCongestionPacket(INT32 xmitMultiplier,
    INT32 recvMultiplier)
{
    HX_RESULT hresult = HXR_OK;
    TNGCongestionPacket pkt;

    IHXBuffer* pSendBuffer = NULL;
    if(!(SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pSendBuffer))) || !pSendBuffer)
    {
        return HXR_OUTOFMEMORY;
    }

    if (!SUCCEEDED(pSendBuffer->SetSize(pkt.static_size())))
    {
        return HXR_OUTOFMEMORY;
    }
    BYTE* packet = pSendBuffer->GetBuffer();
    UINT32 packetLen = 0;

    pkt.dummy0 = 0;
    pkt.dummy1 = 0;
    pkt.dummy2 = 0;
    pkt.dummy3 = 0;
    pkt.packet_type = (UINT16)0xff05;
    pkt.xmit_multiplier = xmitMultiplier;
    pkt.recv_multiplier = recvMultiplier;
    pkt.pack(packet, packetLen);
    // writePacket will call pSendBuffer->Release()
    hresult = writePacket(pSendBuffer);

    return hresult;
}

HX_RESULT
RDTUDPTransport::sendBWProbingPackets(UINT32 ulCount, UINT32 ulSize, REF(INT32) lSeqNo)
{
    HX_ASSERT (ulCount >= MIN_ABD_PACKETS && ulCount <= MAX_ABD_PACKETS);
    HX_ASSERT (ulSize >= MIN_ABD_PACKET_SIZE && ulSize <= MAX_ABD_PACKET_SIZE);

    HX_RESULT hresult = HXR_FAIL;
    TNGBWProbingPacket pkt;
    CHXStaticBuffer* pBuffer = NULL;
    CHXStaticBuffer* pSendBuf = NULL;
    BYTE* pc = NULL;
    UINT32 ulTemp = 0;

    UINT16 ulPacketSize, ulPayloadSize, ulPacketCount;

    ulPacketCount = ulCount;
    ulPacketSize = ulSize;
    ulPayloadSize = ulPacketSize - (UINT16)pkt.static_size();
    lSeqNo = -1;

    pBuffer = new CHXStaticBuffer((UCHAR*)g_pABDBuf,(ulPacketSize+ulPacketCount));
    if (pBuffer == NULL)
    {
        return HXR_FAIL;
    }
    pBuffer->AddRef();

    pc = pBuffer->GetBuffer();

    for (UINT8 i = 0 ; i < ulPacketCount; i++)
    {
        pkt.length_included_flag = 1;
        pkt.dummy0 = 0;
        pkt.dummy1 = 0;
        pkt.dummy2 = 0;
        pkt.packet_type = (UINT16)0xff0b;
        pkt.length = ulPacketSize;
        pkt.seq_no = i;
        pkt.data.data = NULL;
        pkt.data.len = ulPayloadSize;

        pkt.pack(pc, ulTemp);

        HX_ASSERT(ulPacketSize == ulTemp);

        pSendBuf = new CHXStaticBuffer(pc, ulPacketSize);
        pSendBuf->AddRef();

        pc++; //just to make data in each packet to be different
                     //The g_pABDBUF is big enough that this pointer will not 
                     //go out if bounds. 

        if (SUCCEEDED(m_pSocket->Write(pSendBuf)))
        {
            pSendBuf->Release();
            lSeqNo = i; //keep track of the seq_no of last packet successfully sent.
                         //In case we fail to send all the requested packets we pass this
                         //to the client, -1 incase we are not able to send any packets.
            hresult = HXR_OK;
        }
        else
        {
            pSendBuf->Release();
            hresult = HXR_FAIL;
            break;
        }
    }

    pBuffer->Release();
    return hresult;
}

HX_RESULT
RDTUDPTransport::writePacket(IHXBuffer* pSendBuffer)
{
    HX_ASSERT(m_pSocket);
    HX_RESULT rc = HXR_OK;

    if (m_bMulticast)
    {
        rc = m_pSocket->WriteTo(pSendBuffer, m_pPeerAddr);
    }
    else
    {
        rc = m_pSocket->Write(pSendBuffer);
    }
    pSendBuffer->Release();

    return rc;
}

STDMETHODIMP
RDTUDPTransport::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_ASSERT(m_pSocket);

    HX_RESULT rc = HXR_OK;
    IHXBuffer* pBuf = NULL;
    switch (uEvent)
    {
    case  HX_SOCK_EVENT_READ:
        if (SUCCEEDED(m_pSocket->Read(&pBuf)))
        {
            m_pResp->PacketReady(HXR_OK, m_sessionID, NULL);

            rc = handlePacket(pBuf);
        }

        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
        m_pResp->OnProtocolError(HXR_NET_SOCKET_INVALID);
        break;
    default:
        HX_ASSERT(FALSE);
    }

    return HXR_OK;
}


HX_RESULT
RDTUDPTransport::handlePacket(IHXBuffer* pBuffer)
{
    HX_RESULT rc = HXR_OK;

    pBuffer->AddRef();
    UINT32 len = pBuffer->GetSize();
    UINT32 pos = 0;
    UINT16 packetType;
    UINT32 ulTimeStamp = 0;

    while(len && rc == HXR_OK)
    {
        packetType = (UINT16)getshort(pBuffer->GetBuffer() + pos + 1);
        if(packetType < 0xff00)
        {
            // data packets only go from S->C
            rc = HXR_UNEXPECTED;
        }
        else
        {
            switch(packetType)
            {
                case 0xff00:
                    // ASM Action packets are unexpected (see 
                    // handleASMActionPacket) but we will leave this in until
                    // we are sure it can be removed.
                    HX_ASSERT(FALSE);                    
                    rc = handleASMActionPacket(pBuffer, &pos, &len);
                    break;
                case 0xff01:
                    // BW Report packets are deprecated
                    HX_ASSERT(FALSE);   
                    rc = HXR_UNEXPECTED;
                    break;
                case 0xff02:
                    rc = handleACKPacket(pBuffer, &pos, &len);
                    break;
                case 0xff03:
                    rc = handleRTTRequestPacket(pBuffer, &pos, &len);
                    break;
                case 0xff04:
                    rc = handleRTTResponsePacket(pBuffer, &pos, &len);
                    break;
                case 0xff05:
                    rc = handleCongestionPacket(pBuffer, &pos, &len);
                    break;
                case 0xff06:
                    // stream End packets are S->C only.
                    HX_ASSERT(FALSE);
                    rc = HXR_UNEXPECTED;
                    break;
                case 0xff08:
                    if (!ulTimeStamp) ulTimeStamp = GenerateTimeStamp(pBuffer);
                    rc = handleLatencyReportPacket(pBuffer, &pos, &len,
                        ulTimeStamp);
                    break;
                case 0xff09:
                    if (!ulTimeStamp) ulTimeStamp = GenerateTimeStamp(pBuffer);
                    rc = handleTransportInfoReqPacket(pBuffer, &pos,&len,
                                                      ulTimeStamp);
                    break;
                case 0xff0a:
                    rc = handleTransportInfoRespPacket(pBuffer, &pos, &len);
                    break;
            default:
                // boo-boo packet
                rc = HXR_UNEXPECTED;
                break;
            }
        }
    }
    pBuffer->Release();
    return rc;
}

HX_RESULT
RDTUDPTransport::handleASMActionPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                       UINT32* pLen)
{
    //
    // THIS IS CURRENTLY IMPLEMENTED AS AN RTSP CONTROL MESSAGE
    // SO THIS PACKET TYPE SHOULD NOT BE EXPECTED
    //

    UINT16 streamNumber;
    TNGASMActionPacket pkt;
    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;

    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        *pLen = 0;
        return HXR_UNEXPECTED;
    }

    if (pkt.data.len > 0xffff)
    {
#ifdef DEBUG
        printf("illegal length/s in asm action packet <%lu, %lu>\n",
            pkt.length, pkt.data.len);
        fflush(0);
#endif
        return HXR_UNEXPECTED;
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= pOff - pData;
    (*pPos) += pOff - pData;

    //XXXVS: stream_id needs to be translated from StreamGroupNumber to StreamNumber
    // for content where StreamGroupCount != StreamCount
    if(pkt.stream_id == 31)
    {
        streamNumber = m_pStreamHandler->getStreamNumForStreamGroup( pkt.stream_id_expansion);
    }
    else
    {
        streamNumber = m_pStreamHandler->getStreamNumForStreamGroup( pkt.stream_id);
    }

    // call subscribe/unsubscribe/etc

    return HXR_UNEXPECTED;
}

UINT32* g_pNAKCounter = 0;

HX_RESULT
RDTUDPTransport::handleACKPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen)
{
    TNGACKPacket pkt;

    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;
    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        *pLen = 0;
        return HXR_UNEXPECTED;
    }

    if (pkt.data.len > 0xffff)
    {
#ifdef DEBUG
        printf("illegal length/s in ack packet <%lu - %lu, %lu>\n",
            *pLen, pkt.length, pkt.data.len);
        fflush(0);
#endif
        return HXR_UNEXPECTED;
    }

    /*
     * Subtract out the length that we used for this packet.
     */

    (*pLen) -= pOff - pData;
    (*pPos) += pOff - pData;

    if (m_bUseExtPkts && pkt.extension_flag 
        && pkt.rsid != m_ulRDTSessionID)
    {
        // packet is from an old session, ignore it.
        return HXR_OK;
    }

    if (g_pNAKCounter)
    {
        *g_pNAKCounter += 1;
    }

    /*
     * XXXGH...what's is the test for headerLen checking?
     */

    UINT32 headerLen = sizeof(INT16)*2 + sizeof(INT8);
    if(pkt.data.len > headerLen)
    {
        BYTE* pOff = (BYTE*)pkt.data.data;
        while(pOff - (BYTE*)pkt.data.data < (INT32)pkt.data.len)
        {
            //XXXVS: StreamGroupNum is always put on the wire
            // for content where StreamGroupCount != StreamCount
            // and needs to be converted to StreamNum before use
            UINT16 streamNumber = m_pStreamHandler->getStreamNumForStreamGroup(
                                                                  getshort(pOff));
            pOff += sizeof(UINT16);
            UINT16 maxSeqNo = getshort(pOff);
            pOff += sizeof(UINT16);
            UINT16 bitCount = getshort(pOff);
            pOff += sizeof(UINT16);
            UINT8 nByteCount = (UINT8)(*pOff++);
#if XXXTEMPLOSTHIGH
            UINT8 bLostHigh = (UINT8)(*pOff++);
#endif
            CHXBitset bitset(pOff, bitCount ? nByteCount : 0);

            /*
             * Note that both lists get an extra slot to handle the maxSeqNo
             */

            NEW_FAST_TEMP_STR(ackListPtrBuf, 400 * sizeof(UINT16*), (bitCount + 1) * sizeof(UINT16*));
            NEW_FAST_TEMP_STR(nakListPtrBuf, 400 * sizeof(UINT16*), (bitCount + 1) * sizeof(UINT16*));
            UINT16* ackList = (UINT16*)(ackListPtrBuf);
            UINT16* nakList = (UINT16*)(nakListPtrBuf);
            UINT32 ackIdx = 0;
            UINT32 nakIdx = 0;

#ifndef XXXTEMPLOSTHIGH
            if (pkt.lost_high)
#else
            if (bLostHigh)
#endif
            {
                /*
                 * The maxSeqNo value is an NAK
                 */

                nakList[nakIdx++] = maxSeqNo;
            }
            else
            {
                /*
                 * The maxSeqNo value is an ACK
                 */

                ackList[ackIdx++] = maxSeqNo;
            }

            /*
             * j is used for backwards wrap-around cases
             */

            UINT16 j = 0;
            UINT16 uSeqNo = 0;

            for (UINT16 i = 0; i < bitCount; i++)
            {
                uSeqNo = (maxSeqNo - 1) - i;

                if (uSeqNo > maxSeqNo)
                {
                    uSeqNo = (UINT16) ((m_wrapSequenceNumber - 1) - j++);
                }

                if (i < bitset.getNumBits())
                {
                    if (bitset.test(i))
                    {
                        ackList[ackIdx++] = HX_SAFEUINT(uSeqNo);
                    }
                    else
                    {
                        nakList[nakIdx++] = HX_SAFEUINT(uSeqNo);
                    }
                }
            }

            RTSPResendBuffer* pResendBuffer = getResendBuffer(streamNumber);

            m_pResp->OnACK(HXR_OK, pResendBuffer, streamNumber, m_sessionID,
                           ackList, ackIdx, nakList, nakIdx);

            if (m_pSignalBus)
            {
                m_pQoSMetrics->Update(HX_GET_TICKCOUNT(),
                                    streamNumber,
                                    m_fRTT, ackList, ackIdx, nakList, nakIdx);

                HX_VERIFY(SUCCEEDED(m_pQoSMetrics->UpdateStatistics (m_pQoSInfo)));

                ServerBuffer* pReport = new ServerBuffer(TRUE);

                if(SUCCEEDED(m_pQoSMetrics->CreateReport(streamNumber, pReport)))
                {
                    QoSSignal* pSignal = new QoSSignal(TRUE, pReport,
                        MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                              HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                              HX_QOS_SIGNAL_RDT_METRICS));

                    m_pSignalBus->Send(pSignal);

                    pSignal->Release();
                }
                pReport->Release();
            }

            /*
             * Do not force discard for NAK packets
             * Do not force discard if this is from BCM client because we are sharing resend buf
             */

            if (bitCount && pResendBuffer && !isBCM())
            {
                pResendBuffer->DiscardExpiredPackets(TRUE, uSeqNo);
            }

            DELETE_FAST_TEMP_STR(ackListPtrBuf);
            DELETE_FAST_TEMP_STR(nakListPtrBuf);

            pOff += nByteCount;
        }
    }
    return HXR_OK;
}

HX_RESULT
RDTUDPTransport::handleRTTRequestPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                        UINT32* pLen)
{
    TNGRTTRequestPacket pkt;

    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;
    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        *pLen = 0;
        return HXR_UNEXPECTED;
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= pOff - pData;
    (*pPos) += pOff - pData;

    return m_pResp->OnRTTRequest(HXR_OK, m_sessionID);
}

HX_RESULT
RDTUDPTransport::handleRTTResponsePacket(IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen)
{
    TNGRTTResponsePacket pkt;
    UINT32 secs;
    UINT32 uSecs;

    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;
    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        *pLen = 0;
        return HXR_UNEXPECTED;
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    secs = pkt.timestamp_sec;
    uSecs = pkt.timestamp_usec;

    m_bIsReceivedData = TRUE;

    return m_pResp->OnRTTResponse(HXR_OK, m_sessionID, secs, uSecs);
}

HX_RESULT
RDTUDPTransport::handleCongestionPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                        UINT32* pLen)
{
    TNGCongestionPacket pkt;
    INT32 xmitMultiplier;
    INT32 recvMultiplier;

    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;
    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        *pLen = 0;
        return HXR_UNEXPECTED;
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= pOff - pData;
    (*pPos) += pOff - pData;

    xmitMultiplier = pkt.xmit_multiplier;
    recvMultiplier = pkt.recv_multiplier;

    return m_pResp->OnCongestion(HXR_OK, m_sessionID, xmitMultiplier,
                                 recvMultiplier);

    return HXR_OK;
}

HX_RESULT
RDTUDPTransport::streamDone(UINT16 streamNumber,
        UINT32 uReasonCode /* = 0 */, const char* pReasonText /* = NULL */)
{
    HX_RESULT hresult = HXR_OK;

    for(int i=0;i<5;++i)
    {
        hresult = sendStreamEndPacket(streamNumber, uReasonCode, pReasonText);
    }

    return hresult;
}

/*
 * sort of like addStreamInfo()
 */
void
RDTUDPTransport::MulticastSetup(TransportStreamHandler* pHandler)
{
    HX_ASSERT(pHandler);
    HX_ASSERT(!m_pStreamHandler);

    m_bMulticast = TRUE;

    m_pStreamHandler = new TransportStreamHandler(this);
    m_pStreamHandler->AddRef();

    CHXTimestampConverter*  pTSConverter = NULL;
    RTSPStreamData* pStreamData = pHandler->firstStreamData();
    RTSPResendBuffer* pResendBuf = NULL;

    while (pStreamData)
    {
        if (pStreamData->m_pTSConverter)
        {
            pTSConverter = new CHXTimestampConverter();
            *pTSConverter = *pStreamData->m_pTSConverter;
        }
        else
        {
            pTSConverter = NULL;
        }

        m_pStreamHandler->initStreamData(
             pStreamData->m_streamNumber,
             pStreamData->m_streamGroupNumber,
             pStreamData->m_bNeedReliable,
             TRUE /* m_bIsSource */,
             0, // not used
             m_bHackedRecordFlag,
             m_wrapSequenceNumber,
             0, // not used
             0, // not used
             pTSConverter);

        if (!m_bDisableResend)
        {
            m_pStreamHandler->createResendBuffer(pStreamData->m_streamNumber,
                                                 m_wrapSequenceNumber);
            pResendBuf = m_pStreamHandler->getResendBuffer(pStreamData->m_streamNumber);
            if (pResendBuf)
            {
                pResendBuf->SetMaxBufferDepth(MAX_RESEND_BUF_DURATION);
            }
        }

        pStreamData = pHandler->nextStreamData();
    }
}

void
RDTUDPTransport::JoinMulticast(IHXSockAddr* pAddr, IHXSocket* pSock)
{
    //XXXTDM: multicast
    HX_ASSERT(FALSE);
#if NOTYET
    if (m_ulCurrentMulticastAddress)
    {
        m_pMCastUDPSocket->LeaveMulticastGroup(
            m_ulCurrentMulticastAddress, HXR_INADDR_ANY);
    }
    else
    {
        m_pMCastUDPSocket = pUDP;
        m_pMCastUDPSocket->AddRef();
    }

    m_pMCastUDPSocket->JoinMulticastGroup(ulAddress, HXR_INADDR_ANY);
    m_bMulticast = TRUE;
    m_ulCurrentMulticastAddress = ulAddress;
    m_ulCurrentMulticastPort = ulPort;

    if (m_pStreamHandler)
    {
        RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

        ASSERT(pStreamData);

        while(pStreamData)
        {
            pStreamData->m_pTransportBuffer->SetMulticast();

            pStreamData = m_pStreamHandler->nextStreamData();
        }
    }

    if (m_pSeekCallback &&
        !m_pSeekCallback->m_bIsCallbackPending &&
        m_pInternalReset)
    {
        m_pSeekCallback->m_bIsCallbackPending = TRUE;
        m_pSeekCallback->m_Handle = m_pScheduler->RelativeEnter(
                                                m_pSeekCallback, 0);
    }
#endif
}

TransportStreamHandler*
RDTUDPTransport::GetStreamHandler(void)
{
    m_pStreamHandler->AddRef();
    return m_pStreamHandler;
}

void
RDTUDPTransport::SetStreamHandler(TransportStreamHandler* pHandler)
{
    HX_ASSERT(pHandler);
    HX_ASSERT(!m_bMulticast);

    m_bBCM = TRUE;
    if (m_pStreamHandler)
    {
        m_pStreamHandler->Release();
    }
    pHandler->AddRef();
    m_pStreamHandler = pHandler;
}

HX_RESULT
RDTUDPTransport::pauseBuffers()
{
    Transport::pauseBuffers();
    if (m_ulCurrentMulticastAddress)
    {
        //XXXTDM: multicast
        HX_ASSERT(FALSE);
#if NOTYET
        m_pMCastUDPSocket->LeaveMulticastGroup(
            m_ulCurrentMulticastAddress, HXR_INADDR_ANY);
#endif
    }


    return HXR_OK;
}

HX_RESULT
RDTUDPTransport::resumeBuffers()
{
    Transport::resumeBuffers();
    if (m_ulCurrentMulticastAddress)
    {
        //XXXTDM: multicast
        HX_ASSERT(FALSE);
#if NOTYET
        m_pMCastUDPSocket->JoinMulticastGroup(m_ulCurrentMulticastAddress,
            HXR_INADDR_ANY);
#endif
        m_bMulticast = TRUE;
    }


    return HXR_OK;
}

/*
 * SeekCallback methods
 */

RDTUDPTransport::SeekCallback::SeekCallback(RDTUDPTransport* pTransport):
    m_pTransport(pTransport),
    m_lAckRefCount(0),
    m_bIsCallbackPending(FALSE),
    m_Handle(0)
{
    if(m_pTransport)
    {
        m_pTransport->AddRef();
    }
}

RDTUDPTransport::SeekCallback::~SeekCallback()
{
    HX_RELEASE(m_pTransport);
}

STDMETHODIMP
RDTUDPTransport::SeekCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
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
RDTUDPTransport::SeekCallback::AddRef()
{
    return InterlockedIncrement(&m_lAckRefCount);
}

STDMETHODIMP_(ULONG32)
RDTUDPTransport::SeekCallback::Release()
{
    if (InterlockedDecrement(&m_lAckRefCount) > 0)
    {
        return m_lAckRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
RDTUDPTransport::SeekCallback::Func()
{
    m_bIsCallbackPending = FALSE;
    m_Handle             = 0;

#if 0
    // Commented out for now, causes problems with audio/video sync.
    // Needed for Multicast Switching
    if(m_pTransport && m_pTransport->m_pInternalReset)
    {
        m_pTransport->m_pInternalReset->InternalReset();
    }
#endif

    return HXR_OK;
}

/*
 * UDPOutputCallback methods
 */

RDTUDPTransport::UDPOutputCallback::UDPOutputCallback(
        RDTUDPTransport* pTransport, IHXSockAddr* pPeerAddr,
        IHXBuffer* pBuffer, IHXSocket* pUDPSocket) :
    m_lRefCount(0),
    m_pTransport(pTransport),
    m_pPeerAddr(pPeerAddr),
    m_pBuffer(pBuffer),
    m_pSocket(pUDPSocket),
    m_bDone(TRUE)
{
    if (m_pTransport)
        m_pTransport->AddRef();
    if (m_pPeerAddr)
        m_pPeerAddr->AddRef();
    if (m_pBuffer)
        m_pBuffer->AddRef();
    if (m_pSocket)
        m_pSocket->AddRef();
}

RDTUDPTransport::UDPOutputCallback::~UDPOutputCallback(void)
{
    HX_RELEASE(m_pTransport);
    HX_RELEASE(m_pPeerAddr);
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pSocket);
}

STDMETHODIMP
RDTUDPTransport::UDPOutputCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
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
RDTUDPTransport::UDPOutputCallback::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
RDTUDPTransport::UDPOutputCallback::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
RDTUDPTransport::UDPOutputCallback::Func(void)
{
    if (m_bDone == FALSE)
    {
        return HXR_OK;
    }

    m_bDone = FALSE;

    if (m_pTransport)
    {
        if (FAILED(m_pSocket->Write(m_pBuffer)))
        {
            m_pTransport->m_udpOutputStatus = HXR_OK;
        }
        else
        {
            m_pTransport->m_udpOutputStatus = HXR_FAIL;
        }
    }
    m_bDone = TRUE;

    return HXR_OK;
}

/*
 * TransportInfoCallback methods
 */

RDTUDPTransport::TransportInfoCallback::TransportInfoCallback(RDTUDPTransport* pTransport)
  : m_pTransport(pTransport)
    , m_lRefCount (0)
    , m_ulFrequency(0)
    , m_Handle (0)
{
    HX_ASSERT(m_pTransport);
    HX_ASSERT(m_pTransport->m_pScheduler);

    if(m_pTransport)
    {
        m_pTransport->AddRef();
    }

    if (m_pTransport->m_pScheduler)
    {
        m_pScheduler = m_pTransport->m_pScheduler;
        m_pScheduler->AddRef();
    }
}

RDTUDPTransport::TransportInfoCallback::~TransportInfoCallback()
{
    Stop();
}

void
RDTUDPTransport::TransportInfoCallback::Start(UINT32 ulFrequency)
{
    HX_ASSERT(m_pScheduler);
    HX_ASSERT(ulFrequency);

    if (!m_pScheduler)
    {
        return;
    }

    if (m_Handle)
    {
        m_pScheduler->Remove(m_Handle);
        m_Handle = 0;
    }

    m_ulFrequency = ulFrequency;
    m_Handle = m_pScheduler->RelativeEnter((IHXCallback*)this, m_ulFrequency);
}

void
RDTUDPTransport::TransportInfoCallback::Stop()
{
    if (m_Handle && m_pScheduler)
    {
        m_pScheduler->Remove(m_Handle);
        m_Handle = 0;
    }

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pTransport);
}

void
RDTUDPTransport::TransportInfoCallback::Suspend()
{
    if (m_Handle)
    {
        m_pScheduler->Remove(m_Handle);
        m_Handle = 0;
    }
}

void
RDTUDPTransport::TransportInfoCallback::Resume()
{
    // we can only Resume() if we've Start()ed at least once.
    // I assume frequency of 0 is disallowed.

    if (!m_Handle && m_ulFrequency)
    {
        m_Handle = m_pScheduler->RelativeEnter((IHXCallback*)this, m_ulFrequency);
    }
}

STDMETHODIMP
RDTUDPTransport::TransportInfoCallback::Func()
{
    m_Handle = 0;

    HX_ASSERT(m_pTransport);
    if (!m_pTransport)
    {
        return HXR_OK;
    }

    RDTTransportInfoRequestPacket req;
    IHXBuffer* pSendBuffer = NULL;
    if(!(SUCCEEDED(m_pTransport->m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pSendBuffer))) || !pSendBuffer)
    {
        return HXR_OUTOFMEMORY;
    }

    if (!SUCCEEDED(pSendBuffer->SetSize(req.static_size())))
    {
        return HXR_OUTOFMEMORY;
    }
    BYTE* packet = pSendBuffer->GetBuffer();
    UINT32 packetLen = 0;

    req.packet_type = 0xff09;
    req.request_rtt_info = 1;
    req.request_buffer_info = 1;

    UINT32 now = 0;

    if (m_pTransport->m_pAccurateClock)
    {
        HXTimeval tv = m_pTransport->m_pAccurateClock->GetTimeOfDay();
        now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    }
    else
    {
        Timeval tv;
        (void)gettimeofday(&tv, 0);
            now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    }

    req.request_time_ms = now;
    req.pack(packet, packetLen);
    // writePacket will call pSendBuffer->Release()
    m_pTransport->writePacket(pSendBuffer);

    m_Handle = m_pScheduler->RelativeEnter((IHXCallback*)this, m_ulFrequency);

    return HXR_OK;
}

STDMETHODIMP
RDTUDPTransport::TransportInfoCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
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
RDTUDPTransport::TransportInfoCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
RDTUDPTransport::TransportInfoCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

void 
RDTUDPTransport::setRTTInfo(UINT32 uReqTimeMs)
{
    UINT32 now;

    if (m_pAccurateClock)
    {
        HXTimeval tv = m_pAccurateClock->GetTimeOfDay();
        now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    }
    else
    {
        Timeval tv;
        (void)gettimeofday(&tv, 0);
        now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    }

    UINT32 ulRTTDiff = (now <= uReqTimeMs) ? 1 :
    (now - uReqTimeMs);

    m_fRTT = (m_fRTT) ? (double)((1 - RDT_RTT_GAIN) * m_fRTT +
        RDT_RTT_GAIN * ((double)(ulRTTDiff))) :
    (double)(ulRTTDiff);

    if (m_pSignalBus)
    {
        ServerBuffer* pRTT = new ServerBuffer(FALSE);
        pRTT->SetSize(sizeof(double));
        *((double*)pRTT->GetBuffer()) = m_fRTT;

        QoSSignal* pSignal = new QoSSignal(TRUE, (IHXBuffer*)pRTT,
            MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
            HX_QOS_SIGNAL_RELEVANCE_METRIC,
            HX_QOS_SIGNAL_RDT_RTT));

        m_pSignalBus->Send(pSignal);

        //do not release pRTT since pSignal now owns it
        pSignal->Release();
    }
}
