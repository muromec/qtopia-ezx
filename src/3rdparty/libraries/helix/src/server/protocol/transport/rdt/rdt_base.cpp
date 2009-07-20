/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rdt_base.cpp,v 1.9 2007/02/06 22:20:03 tknox Exp $
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
#include "tngpkt.h"
#include "basepkt.h"
#include "hxtbuf.h"
#include "transbuf.h"
#include "hxtick.h"
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

#include "servbuffer.h"
#include "qos_signal.h"


#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

/*
 * RDTBaseTransport methods
 */

RDTBaseTransport::RDTBaseTransport(BOOL bIsSource,
                UINT32 uFeatureLevel /* = 0 */)
    : Transport()
    , m_pContext(NULL)
    , m_uFeatureLevel(uFeatureLevel)
    , m_pAccurateClock(NULL)
    , m_pSignalBus (NULL)
    , m_pQoSInfo (NULL)
    , m_pSessionId (NULL)
    , m_bDone(FALSE)
    , m_lastLatencyReportTime(0)
    , m_LatencyReportStartTime(0)
    , m_ulLatencyReportPeriod(LATENCY_REPORT_INTERVAL_MS)
    , m_ulBackToBackFreq(10)
    , m_ulBackToBackCounter(9)
    , m_bAttemptingBackToBack(0)
    , m_pSource(NULL)
    , m_ucPacketQueuePos(0)
    , m_ulPacketQueueSize(0)
    , m_ulBandwidth(0)
    , m_ulAggregateTo (0)
    , m_ulAggregateHighest (0)
    , m_pAggregStat (NULL)
    , m_pSocket(NULL)
    , m_bUseExtPkts(FALSE)
    , m_ulRDTSessionID(0)
{
    m_pPacketQueue = new ServerPacket*[PACKET_QUEUE_SIZE];

    m_wrapSequenceNumber = TNG_WRAP_SEQ_NO;
}

RDTBaseTransport::~RDTBaseTransport()
{
    Done();

    HX_RELEASE( m_pContext );
}

STDMETHODIMP
RDTBaseTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSourceBandwidthInfo))
    {
        AddRef();
        *ppvObj = (IHXSourceBandwidthInfo*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketSink))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerRDTTransport))
    {
        AddRef();
        *ppvObj = (IHXServerRDTTransport*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSSignalSourceResponse))
    {
        AddRef();
        *ppvObj = (IHXQoSSignalSourceResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }


    return Transport::QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(UINT32)
RDTBaseTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RDTBaseTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

RTSPTransportTypeEnum
RDTBaseTransport::tag()
{
    return m_lTransportType;
}

HX_RESULT
RDTBaseTransport::init(IUnknown* pContext,
                       IHXSocket* pSocket,
                       IHXRTSPTransportResponse* pResp)
{
    m_pSocket = pSocket;
    m_pSocket->AddRef();

    m_pSocket->SetResponse(static_cast<IHXSocketResponse*>(this));

    m_pContext = pContext;
    m_pContext->AddRef();

    m_pResp = pResp;
    HX_ADDREF( m_pResp );

    /* Set DiffServ Code Point */
    IHXQoSDiffServConfigurator* pCfg = NULL;
    if (SUCCEEDED(m_pContext->QueryInterface(IID_IHXQoSDiffServConfigurator, (void**)&pCfg)))
    {
        pCfg->ConfigureSocket(m_pSocket, HX_QOS_DIFFSERV_CLASS_MEDIA);
        HX_RELEASE( pCfg );
    }

    /* Try to get the cheap, highly accurate clock from the server */
    m_pContext->QueryInterface(IID_IHXAccurateClock,
        (void **)&m_pAccurateClock);

    return Init(pContext);
}

void
RDTBaseTransport::setSessionID(const char* pSessionID)
{
    HX_ASSERT(m_pCommonClassFactory);
    /* cache the session id for use in retrieving signal bus*/
    if(pSessionID && (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                                      (void**)&m_pSessionId))))
    {
        m_pSessionId->Set((UCHAR*)pSessionID,
                          strlen(pSessionID)+1);

        IHXQoSSignalSource* pSignalSrc = NULL;
        if (m_pSessionId && SUCCEEDED(m_pContext->QueryInterface(IID_IHXQoSSignalSource,
                                                                 (void**) &pSignalSrc)))
        {
            pSignalSrc->GetSignalBus(m_pSessionId, (IHXQoSSignalSourceResponse*)this);
            HX_RELEASE(pSignalSrc);
        }
        else
        {
            m_pSignalBus = NULL;
        }
    }

    m_sessionID = pSessionID;
}

void
RDTBaseTransport::Done()
{
    HX_RELEASE( m_pPacketFilter );
    HX_RELEASE( m_pAccurateClock );

    IHXQoSSignalSource* pSignalSrc = NULL;
    if (m_pSessionId && m_pContext
        && SUCCEEDED(m_pContext->QueryInterface(IID_IHXQoSSignalSource,
                                                (void**) &pSignalSrc)))
    {
        pSignalSrc->ReleaseResponseObject(m_pSessionId,
                                            static_cast<IHXQoSSignalSourceResponse*>(this));
        HX_RELEASE(pSignalSrc);
    }

    HX_RELEASE( m_pSessionId );
    HX_RELEASE( m_pSignalBus );
    HX_RELEASE( m_pQoSInfo );

    ReleasePacketQueue();
    HX_VECTOR_DELETE( m_pPacketQueue );

    HX_RELEASE( m_pSource );

    if (m_pSocket)
    {
        /** If this transport is marked as an aggregate the socket can be
          * re-used and it is up to the owner to call the sockets Close()
          * method. */
        if (!m_bIsAggregate)
        {
            m_pSocket->Close();
        }

        HX_RELEASE( m_pSocket );
    }
}

HX_RESULT
RDTBaseTransport::sendStreamEndPacket(UINT16 streamNumber,
        UINT32 uReasonCode /* = 0 */, const char* pReasonText /* = NULL */)
{
    RTSPStreamData* pStreamData;
    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(!pStreamData)
    {
        return HXR_FAIL;
    }

    HX_RESULT hresult = HXR_OK;
    TNGStreamEndPacket pkt;

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
    
    if (m_bUseExtPkts)
    {
        pkt.packet_type = (UINT16)0xff0c;
        pkt.version = RDTV4;
        pkt.reserved = 0;
        pkt.rsid = m_ulRDTSessionID;
    }
    else
    {
        pkt.packet_type = (UINT16)0xff06;
    }

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

    /* We'll always include the reliable flag for now*/
    pkt.need_reliable_flag = 1;

    if(pkt.need_reliable_flag)
    {
        pkt.total_reliable = pStreamData->m_reliableSeqNo;
    }

    pkt.seq_no = pStreamData->m_seqNo;
    pkt.timestamp = pStreamData->m_lastTimestamp;
    pkt.packet_sent = pStreamData->m_packetSent;
    /*
     * The stream-end extension is only enabled for RDT v3 and above, even
     * though it was designed for backward compatibility.  This is because
     * (1) the only clients that can of understand it are v3, and (2) some
     * older versions of the microcore will enter an infinite loop if they
     * receive an unknown RDT packet type.  The affected microcores are v1
     * because they does not send an RDTFeatureLevel header.  -- TDM
     */
    if (m_uFeatureLevel >= 3 && uReasonCode != 0 && pReasonText != NULL)
    {
        pkt.ext_flag = 1;
        memset(pkt.reason_dummy, 0xff, sizeof(pkt.reason_dummy));
        pkt.reason_code = uReasonCode;
        pkt.reason_text = (char*)pReasonText;
    }
    else
    {
        pkt.ext_flag = 0;
    }
    pkt.pack(packet, packetLen);
    // writePacket will call pSendBuffer->Release()
    hresult = writePacket(pSendBuffer);

    return hresult;
}

/**************************************************************************
 * RDTTCPTransport::createLatencyReportPacket(TNGLatencyReportPacket* pRpt
 *
 * Fills in the fields for the passed in TNGLatencyReportPacket with the
 * expectation that later pack will be called on it.
 */
BOOL
RDTBaseTransport::createLatencyReportPacket(TNGLatencyReportPacket* pRpt)
{
    UINT32 now;
    BOOL bNowIsReal = FALSE;

    if (m_pAccurateClock)
    {
        HXTimeval tv = m_pAccurateClock->GetTimeOfDay();
        now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        bNowIsReal = TRUE;
    }
    else if (m_pScheduler)
    {
        HXTimeval tv = m_pScheduler->GetCurrentSchedulerTime();
        now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    }
    else
    {
        Timeval tv;
        (void)gettimeofday(&tv, 0);

        now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        bNowIsReal = TRUE;
    }

    UINT32 interval_in_ms;

    /*
     * If this is our first time, make sure we send out a report right now.
     */
    if(!m_lastLatencyReportTime)
    {
        interval_in_ms = m_ulLatencyReportPeriod;
    }
    else
    {
        /*
         * handle wrap around of the tick count.
         */
        if(now < m_lastLatencyReportTime)
        {
            interval_in_ms = 0xffffffff - m_lastLatencyReportTime + now;
        }
        else
        {
            interval_in_ms = now - m_lastLatencyReportTime;
        }
    }

    if(interval_in_ms >= m_ulLatencyReportPeriod)
    {
        UINT32 timeoff;
        /*
         *  Update the last time we sent out a report.
         */
        m_lastLatencyReportTime = now;

        if (!bNowIsReal)
        {
            Timeval tv;
            (void)gettimeofday(&tv, 0);

            now = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        }
        if (!m_LatencyReportStartTime)
        {
            m_LatencyReportStartTime = now;
            timeoff = 0;
        }
        else if (now < m_LatencyReportStartTime)
        {
            timeoff = 0xffffffff - m_LatencyReportStartTime + now;
        }
        else
        {
            timeoff = now - m_LatencyReportStartTime;
        }

        timeoff = (UINT32)(timeoff * 1.01);
        pRpt->server_out_time = timeoff;

        pRpt->length = (short)pRpt->static_size(); //we know static_size < 256
        /*
         *  Must include the length in all but the last logical packet in
         *  a multipart physical packet.
         */
        pRpt->length_included_flag = 1;
        pRpt->packet_type = 0xff08;

        return TRUE;
    }
    return FALSE;
}

UINT32
RDTBaseTransport::SendPacketQueue()
{
    if (m_bDone)
        return 0;

    /*
     * It should be noted that we aggregate packets across streams in
     * a single session.  Because of this, we make the assumption that
     * the underlying pTransport is irrelevant (which is true for RDT but NOT
     * true for RTP).  I.e., all aggregated transports must support this
     * concept.
     *
     * When the queue only has 1 packet, the pTransport is correct.
     */
    if (m_ucPacketQueuePos == 0)
    {
        return 0;
    }

    m_pPacketQueue[m_ucPacketQueuePos] = NULL;
    if (m_ucPacketQueuePos == 1)
    {
        sendPacket(*m_pPacketQueue);
    }
    else
    {
        sendPackets(m_pPacketQueue);
    }

    if ((m_ucPacketQueuePos > 1) && (m_pAggregStat))
    {
        (*m_pAggregStat) += m_ucPacketQueuePos;
    }

    ReleasePacketQueue();
    UINT32 ulPacketSize = m_ulPacketQueueSize;
    m_ulPacketQueueSize = 0;
    m_ucPacketQueuePos = 0;

    return ulPacketSize;
}

// This function violates COM Reference Rules
void
RDTBaseTransport::QueuePacket(ServerPacket* pPacket, UINT32 ulPacketSize)
{
    if (m_bDone)
    {
        return;
    }

    /*
     * Reserve one for the NULL, one for zero indexing,
     * and one for good luck.
     */
    if (m_ucPacketQueuePos >= (PACKET_QUEUE_SIZE - 3))
    {
        SendPacketQueue();
    }
    m_ulPacketQueueSize += ulPacketSize;
    m_pPacketQueue[m_ucPacketQueuePos++] = pPacket;
    pPacket->AddRef();
}

STDMETHODIMP
RDTBaseTransport::Flush()
{
    if (m_bAttemptingBackToBack)
        SendPacketQueue();

    return HXR_OK;
}

void
RDTBaseTransport::ReleasePacketQueue()
{
    if (!m_pPacketQueue)
    {
        return;     // Nothing to do
    }

    while (m_ucPacketQueuePos > 0)
    {
        m_pPacketQueue[--m_ucPacketQueuePos]->Release();
    }
}

void
RDTBaseTransport::updateQoSInfo(UINT64 ulBytesSent)
{
    m_ulPacketsSent++;
    m_lBytesSent += ulBytesSent;

    if (m_pQoSInfo)
    {
        UINT64 ulSessionBytesSent = m_pQoSInfo->GetBytesSent();
        ulSessionBytesSent += ulBytesSent;
        m_pQoSInfo->SetBytesSent(ulSessionBytesSent);

        UINT32 ulSessionPacketsSent = m_pQoSInfo->GetPacketsSent();
        ulSessionPacketsSent++;
        m_pQoSInfo->SetPacketsSent(ulSessionPacketsSent);
    }
}

void
RDTBaseTransport::setLatencyPeriod(UINT32 ulMs)
{
    m_ulLatencyReportPeriod = ulMs;
}

UINT32
RDTBaseTransport::SetBandwidthAndLatency (UINT32 ulBandwidth)
{
    if (ulBandwidth < 5000 )
        m_ulBackToBackFreq = 3;
    else if (ulBandwidth < 20000)
        m_ulBackToBackFreq = 5;
    else if (ulBandwidth < 40000)
        m_ulBackToBackFreq = 10;
    else if (ulBandwidth < 60000)
        m_ulBackToBackFreq = 15;
    else
        m_ulBackToBackFreq = 20;

    m_ulBandwidth = ulBandwidth;

    UINT32 ulMs;
    if (ulBandwidth < 20000)
        ulMs = 1000;
    else
        ulMs = (UINT32)(1000 - ((ulBandwidth - 20000) / 100));

    if ((ulMs > 1000) || (ulMs < 100))
        ulMs = 100;

    return ulMs;
}

HX_RESULT
RDTBaseTransport::handleLatencyReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                           UINT32* pLen, UINT32 ulTime)
{
    TNGLatencyReportPacket pkt;
    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;
    /*
     *  Make sure there is enough data to create the packet, and
     *  create the packet.
     */
    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        *pLen = 0;
        return HXR_UNEXPECTED;
    }

    if (pkt.length > 0x7fff)
    {
#ifdef DEBUG
        printf("illegal length in latency report packet <%lu>\n", pkt.length);
        fflush(0);
#endif
        return HXR_UNEXPECTED;
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= pOff - pData;
    (*pPos) += pOff - pData;

    return HXR_OK;
}

HX_RESULT
RDTBaseTransport::handleTransportInfoReqPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                              UINT32* pLen, UINT32 ulTime)
{
    HX_RESULT res = HXR_UNEXPECTED;
    RDTTransportInfoRequestPacket transportInfoPkt;
    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;

    if ((m_uFeatureLevel > 2) &&
        ((pOff = transportInfoPkt.unpack(pData, *pLen)) != 0))
    {
        /*
         * Subtract out the length that we used for this packet.
         */
        (*pLen) -= pOff - pData;
        (*pPos) += pOff - pData;

        if (transportInfoPkt.request_rtt_info)
        {
            RDTTransportInfoResponsePacket respPkt;
            IHXBuffer* pSendBuffer = NULL;
            if(!(SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                    (void**)&pSendBuffer))) || !pSendBuffer)
            {
                return HXR_OUTOFMEMORY;
            }

            if (!SUCCEEDED(pSendBuffer->SetSize(respPkt.static_size())))
            {
                return HXR_OUTOFMEMORY;
            }
            BYTE* packet = pSendBuffer->GetBuffer();

            respPkt.extension_flag = 0;
            if (m_bUseExtPkts)
            {
                respPkt.extension_flag = 1;
                respPkt.version = RDTV4;
                respPkt.reserved = 0;
                respPkt.rsid = m_ulRDTSessionID;
            }
            respPkt.dummy = 0;
            respPkt.has_rtt_info = 1;
            respPkt.has_buffer_info = 0;
            respPkt.is_delayed = 0;
            respPkt.request_time_ms = transportInfoPkt.request_time_ms;
            respPkt.packet_type = 0xff0a;

            UINT32 packetLen = 0;
            respPkt.pack(packet, packetLen);
            // writePacket will call pSendBuffer->Release()
            res = writePacket(pSendBuffer);
        }
    }
    else
    {
        *pLen = 0;
    }

    return res;
}

HX_RESULT
RDTBaseTransport::handleTransportInfoRespPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen)
{
    HX_RESULT res = HXR_UNEXPECTED;
    RDTTransportInfoResponsePacket pkt;
    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;

    if ((m_uFeatureLevel > 2) &&
        ((pOff = pkt.unpack(pData, *pLen)) != 0))
    {
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
        if (pkt.has_rtt_info)
        {
            // only RDTUDPTransport does anything here
            setRTTInfo(pkt.request_time_ms);
        }
        if (pkt.has_buffer_info && m_pSignalBus)
        {
            QoSSignal* pSignal;
            ServerBuffer* pTmpBuf;

            for (UINT16 i = 0; i < pkt.buffer_info_count; i++)
            {
                pTmpBuf = new ServerBuffer(FALSE);
                pTmpBuf->SetSize(sizeof(BufferMetricsSignal));
                BufferMetricsSignal* pbufSig = (BufferMetricsSignal*)pTmpBuf->GetBuffer();

                //XXXVS: stream_id needs to be translated from StreamGroupNumber to StreamNumber
                // for content where StreamGroupCount != StreamCount
                pbufSig->m_ulStreamNumber = m_pStreamHandler->getStreamNumForStreamGroup( 
                                                                    pkt.buffer_info[i].stream_id);
                pbufSig->m_ulLowTimestamp = pkt.buffer_info [i].lowest_timestamp;
                pbufSig->m_ulHighTimestamp = pkt.buffer_info [i].highest_timestamp;
                pbufSig->m_ulBytes = pkt.buffer_info [i].bytes_buffered;

                // DEBUG_OUTF("buffer_metrics_tcp.txt",
                //           (s, "%u strm: %d time: %d space: %d\n",
                //            HX_GET_BETTERTICKCOUNT() - m_ulStartTime,
                //            m_pStreamHandler->getStreamNumForStreamGroup(
                //                              pkt.buffer_info [i].stream_id),
                //            (pkt.buffer_info [i].highest_timestamp -
                //             pkt.buffer_info [i].lowest_timestamp),
                //            pkt.buffer_info [i].bytes_buffered));

                pSignal = new QoSSignal(TRUE, (IHXBuffer*)pTmpBuf,
                    MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                          HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                          HX_QOS_SIGNAL_COMMON_BUFSTATE));

                m_pSignalBus->Send(pSignal);

                //do not release pTmpBuf since pSignal now owns it
                pSignal->Release();
            }

            HX_VECTOR_DELETE(pkt.buffer_info);
        }

        res = HXR_OK;
    }
    else
    {
        *pLen = 0;
    }

    return res;
}

void
RDTBaseTransport::SetRDTSessionID(UINT32 ulRSID)
{
    m_ulRDTSessionID = ulRSID;
}

void
RDTBaseTransport::UseExtendedPackets(BOOL bExt)
{
    m_bUseExtPkts = bExt;
}
