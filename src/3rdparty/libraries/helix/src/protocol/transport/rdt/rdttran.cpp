/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rdttran.cpp,v 1.22 2007/01/11 21:19:42 milko Exp $
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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

#include "hlxclib/stdio.h"
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
#include "pckunpck.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxdeque.h"
#include "hxbitset.h"
#include "timebuff.h"
#include "timeval.h"
#include "tconverter.h"
#include "rtptypes.h"
#include "hxcorgui.h"

#include "ntptime.h"

#include "rtspif.h"
#include "rtsptran.h"
#include "rdttran.h"
#include "bufnum.h"

#include "tngpkt.h"
#include "basepkt.h"
#include "hxtbuf.h"
#include "hxtlogutil.h"
#include "transbuf.h"
#include "hxtick.h"
#include "random32.h"   // random32()
#include "hxprefs.h"    // IHXPreferences
#include "hxmime.h"
#include "hxcore.h"
#include "errdbg.h"

#include "hxheap.h"
#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#endif

#ifdef PAULM_TNGTCPTRANSTIMING
#include "classtimer.h"
ClassTimer g_TNGTCPTransTimer("TNGTCPTransport", 0, 3600);
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

static const UINT32 NORMAL_ACK_INTERVAL = 1000;        // 1/sec
static const UINT32 MINIMUM_ACK_INTERVAL = 200;        // wait 200msecs

static const UINT32 NORMAL_REPORT_INTERVAL = 5000;      // 1 per 5secs

static const UINT32 LATENCY_REPORT_INTERVAL_MS = 1000;

static UINT16
GetRDTBufferInfoForAllStreams(RDTTransportInfoResponsePacket& respPkt,
                              RTSPStreamHandler* pStreamHandler,
                              IHXSourceBufferingStats3* pSrcBufferStats)
{
    RTSPStreamData* pStreamData = pStreamHandler->firstStreamData();
    UINT16 ulIndex = 0;

    while(pStreamData && (ulIndex < respPkt.buffer_info_count))
    {
        UINT16 streamNumber = pStreamData->m_streamNumber;
        RTSPTransportBuffer* pTransBuf = pStreamData->m_pTransportBuffer;

        if (pTransBuf)
        {
            UINT32 ulLowTS = 0;
            UINT32 ulHighTS = 0;
            UINT32 ulNumBytes = 0;
            HXBOOL bDone = FALSE;

            if (HXR_OK == pSrcBufferStats->GetTotalBuffering(streamNumber,
                                                             ulLowTS,
                                                             ulHighTS,
                                                             ulNumBytes,
                                                             bDone))
            {
                RDTBufferInfo* pBufInfo = &respPkt.buffer_info[ulIndex];

                pBufInfo->stream_id = streamNumber;
                pBufInfo->lowest_timestamp = ulLowTS;
                pBufInfo->highest_timestamp = ulHighTS;
                pBufInfo->bytes_buffered = ulNumBytes;
                ulIndex++;
            }
        }

        pStreamData = pStreamHandler->nextStreamData();
    }

    return ulIndex;
}

static BYTE* PackTransportInfoResp(const RDTTransportInfoRequestPacket& reqPkt,
                                   UINT32 ulTime,
                                   RTSPStreamHandler* pStreamHandler,
                                   IHXSourceBufferingStats3* pSrcBufferStats,
                                   BYTE* packet)
{
    RDTTransportInfoResponsePacket respPkt;
    UINT16 streamCount = (UINT16)pStreamHandler->streamCount();

    respPkt.dummy0 = 0;
    respPkt.dummy1 = 0;
    respPkt.has_rtt_info = reqPkt.request_rtt_info;
    respPkt.is_delayed = 0;
    respPkt.has_buffer_info = reqPkt.request_buffer_info && (streamCount > 0);
    respPkt.packet_type = 0xff0a;

    if (respPkt.has_rtt_info)
    {
        respPkt.request_time_ms = reqPkt.request_time_ms;

        if (respPkt.is_delayed)
        {
            respPkt.response_time_ms = ulTime;
        }
    }

    if (respPkt.has_buffer_info)
    {
        respPkt.buffer_info_count = streamCount;
        respPkt.buffer_info = new RDTBufferInfo[respPkt.buffer_info_count];

        if (respPkt.buffer_info)
        {
            // Populate the buffer_info field with the stream data
            // and update the buffer_info_count with the number of
            // items filled
            respPkt.buffer_info_count =
                GetRDTBufferInfoForAllStreams(respPkt, pStreamHandler,
                                              pSrcBufferStats);

            if (respPkt.buffer_info_count == 0)
            {
                // No buffer info was copied.
                // Clear the indicators that signal
                // that buffer info will be in this
                // packet
                respPkt.has_buffer_info = 0;
            }
        }
        else
        {
            // We failed to allocate memory for
            // the buffer info. Clear the indicators
            // that say this packet will contain
            // buffer info data
            respPkt.buffer_info_count = 0;
            respPkt.has_buffer_info = 0;
        }
    }

    UINT32 packetLen = 0;
    respPkt.pack(packet, packetLen);

    HX_VECTOR_DELETE(respPkt.buffer_info);

    return packet + packetLen;
}

/*
 * TNGUDPTransport methods
 */

TNGUDPTransport::TNGUDPTransport(HXBOOL bIsServer, HXBOOL bIsSource,
                                 RTSPTransportTypeEnum lTransportType,
                                 UINT32 uFeatureLevel /* = 0 */,
                                 HXBOOL bDisableResend /* = FALSE */) :
    RTSPTransport(bIsSource),
    m_lRefCount(0),
    m_pUDPSocket(0),
    m_pFastPathNetWrite(0),
    m_pPeerAddr(NULL),
    m_ulNormalACKInterval(NORMAL_ACK_INTERVAL),
    m_ulMinimumACKInterval(MINIMUM_ACK_INTERVAL),
    m_ackTimeoutID(0),
    m_pACKCallback(0),
    m_pSeekCallback(0),
    m_bCallbackPending(FALSE),
    m_pBwMgrInput(0),
    m_lastLatencyReportTime(0),
    m_LatencyReportStartTime(0),
    m_ulBackToBackTime(0),
    m_ulLatencyReportPeriod(LATENCY_REPORT_INTERVAL_MS),
    m_lTransportType(lTransportType),
    m_pMulticastSocket(0),
    m_pMulticastAddr(0),
    m_bIsServer(bIsServer),
    m_uFeatureLevel(uFeatureLevel),
    m_pAccurateClock(NULL),
    m_bDisableResend(bDisableResend),
    m_bBCM(FALSE),
    m_pErrMsg(NULL)
{
    m_wrapSequenceNumber = TNG_WRAP_SEQ_NO;

    m_pLastACKTime = new Timeval;
    m_pLastACKTime->tv_sec = 0;
    m_pLastACKTime->tv_usec = 0;

#ifdef DEBUG
    m_packets_since_last_drop = 0;
    m_drop_packets = FALSE;
#endif /* DEBUG */
}

TNGUDPTransport::~TNGUDPTransport()
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::~TNGUDPTransport()", this);
    HX_RELEASE(m_pPeerAddr);
    Done();
}

STDMETHODIMP
TNGUDPTransport::QueryInterface(REFIID riid, void** ppvObj)
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

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
TNGUDPTransport::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
TNGUDPTransport::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (UINT32)m_lRefCount;
    }
    delete this;
    return 0;
}

void
TNGUDPTransport::Done()
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::Done()",this);

    destroyABDPktInfo();

    KillAck();
    HX_RELEASE(m_pACKCallback);

    if (m_pSeekCallback &&
        m_pSeekCallback->m_bIsCallbackPending &&
        m_pScheduler)
    {
        m_pSeekCallback->m_bIsCallbackPending = FALSE;
        m_pScheduler->Remove(m_pSeekCallback->m_Handle);
    }

    HX_RELEASE(m_pPacketFilter);
    HX_RELEASE(m_pSeekCallback);

    HX_RELEASE(m_pBwMgrInput);
    HX_RELEASE(m_pUDPSocket);
    HX_DELETE(m_pLastACKTime);
    HX_RELEASE(m_pMulticastSocket);
    HX_RELEASE(m_pMulticastAddr);
    HX_RELEASE(m_pFastPathNetWrite);
    HX_RELEASE(m_pAccurateClock);
    HX_RELEASE(m_pErrMsg);

    if (m_bBCM && m_pStreamHandler)
    {
        m_pStreamHandler->Release();
        m_pStreamHandler = NULL;
    }
    else
    {
        if (m_pStreamHandler)
        {
            m_pStreamHandler->Release();
            m_pStreamHandler = NULL;
        }
    }
}

STDMETHODIMP
TNGUDPTransport::InitBw(IHXBandwidthManagerInput* pBwMgr)
{
    HX_RELEASE(m_pBwMgrInput);

    m_pBwMgrInput = pBwMgr;
    pBwMgr->AddRef();

    return HXR_OK;
}

STDMETHODIMP
TNGUDPTransport::SetTransmitRate(UINT32 ulBitRate)
{
    return HXR_OK;
}

void
TNGUDPTransport::addStreamInfo(RTSPStreamInfo* pStreamInfo,
                               UINT32 ulBufferDepth)
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::addStreamInfo(): str = %u; buffer depth = %lu",this, pStreamInfo->m_streamNumber, ulBufferDepth);
    RTSPTransport::addStreamInfo(pStreamInfo, ulBufferDepth);
    if (m_bIsSource && pStreamInfo && !m_bDisableResend)
    {
        m_pStreamHandler->createResendBuffer(pStreamInfo->m_streamNumber,
                                             m_wrapSequenceNumber);
    }

    if (!m_bIsSource)
    {
        RTSPTransportBuffer* pTransportBuffer =
            getTransportBuffer(pStreamInfo->m_streamNumber);

        if (m_bMulticast)
            pTransportBuffer->SetMulticast();

        if (m_bIsServer)
            pTransportBuffer->SetLive();
    }
}

RTSPTransportTypeEnum
TNGUDPTransport::tag()
{
    return m_lTransportType;
}

HX_RESULT
TNGUDPTransport::init(IUnknown* pContext,
                      IHXSocket* pSocket,
                      IHXRTSPTransportResponse* pResp)
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::init()", this);

    m_pResp = pResp;

    if (m_pResp)
        m_pResp->AddRef();

    m_pUDPSocket = pSocket;
    m_pUDPSocket->AddRef();

    m_pFastPathNetWrite = NULL;

    if (m_bIsServer && m_bIsSource)
    {
        /* Only use fast write from within the server core */
        m_pUDPSocket->QueryInterface(IID_IHXFastPathNetWrite,
                                     (void **)&m_pFastPathNetWrite);

        /* Try to get the cheap, highly accurate clock from the server */
        pContext->QueryInterface(IID_IHXAccurateClock,
                                 (void **)&m_pAccurateClock);
    }

    if (pContext)
    {
        HX_RESULT hresult = Init(pContext);
        if(HXR_OK != hresult)
        {
            return hresult;
        }

        pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrMsg);
    }

    m_pACKCallback = new ACKCallback(this);
    m_pACKCallback->AddRef();

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
TNGUDPTransport::Reset()
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::Reset()", this);
    KillAck();
}

void
TNGUDPTransport::Restart()
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::Restart()", this);
    StartAck();
}

void
TNGUDPTransport::setPeerAddr(IHXSockAddr* pAddr)
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::setPeerAddr(): port = %u", this, pAddr->GetPort());
    HX_ASSERT(pAddr);
    if(!m_pPeerAddr)
    {
        m_pPeerAddr = pAddr;
        m_pPeerAddr->AddRef();

        // Set destination for Write()
        m_pUDPSocket->ConnectToOne(m_pPeerAddr);

        // Send a RTTRequest to poke a hole in any
        // NAT between the client and the server
        sendRTTRequestPacket();

        startScheduler();
    }
    else
    {
        HX_ASSERT(m_pPeerAddr->IsEqual(pAddr));
    }
}

void
TNGUDPTransport::StartAck()
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::StartAck()", this);
    startScheduler();
}

void
TNGUDPTransport::KillAck()
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::KillAck()", this);
    if(!m_bIsSource && m_bCallbackPending)
    {
        m_bCallbackPending = FALSE;
        m_pScheduler->Remove(m_ackTimeoutID);
        m_ackTimeoutID = 0;
    }
}

void
TNGUDPTransport::startScheduler()
{
    if(!m_bIsSource)
    {
        HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
        Timeval ackTime((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);
//      ackTime += HX_SAFEUINT(m_ulNormalACKInterval);
        ackTime += HX_SAFEINT(m_ulNormalACKInterval);
        HXTimeval timeout;
        timeout.tv_sec = (UINT32)ackTime.tv_sec;
        timeout.tv_usec = (UINT32)ackTime.tv_usec;
        if (!m_bCallbackPending)
        {
            m_bCallbackPending = TRUE;
            m_ackTimeoutID = m_pScheduler->AbsoluteEnter(m_pACKCallback,
                                                         timeout);
        }
    }
}

void
TNGUDPTransport::handleACKTimeout()
{
    HX_RESULT   rc = HXR_OK;

    if(m_bCallbackPending)
    {
        /*
         * XXX...this is an error condition
         */
        return;
    }
    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
    Timeval now((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);
    Timeval lastInterval = now - (*m_pLastACKTime);
    Timeval nextTimeout;
    if(lastInterval >= Timeval((INT32)(m_ulMinimumACKInterval*1000)))
    {
        // do the deed
        rc = sendACKPacket();

        if (HXR_OK != rc)
        {
            // instead send RTT request packet to keep the UDP channel alive
            sendRTTRequestPacket();
        }

        *m_pLastACKTime = now;
        nextTimeout = now + Timeval((INT32)(m_ulNormalACKInterval*1000));
    }
    else
    {
        nextTimeout = Timeval((INT32)(m_ulMinimumACKInterval*1000)) -
            lastInterval;
    }

    // schedule next one
    lTime.tv_sec = (UINT32)nextTimeout.tv_sec;
    lTime.tv_usec = (UINT32)nextTimeout.tv_usec;
    if (!m_bCallbackPending)
    {
        m_bCallbackPending = TRUE;
        m_ackTimeoutID = m_pScheduler->AbsoluteEnter(m_pACKCallback, lTime);
    }
}

HX_RESULT TNGUDPTransport::sendPackets(BasePacket** pPacket)
{
#ifndef _MACINTOSH
    HX_ASSERT(m_pFastPathNetWrite);

    UINT32 ulSize = 0;
    UINT8* pBuffer = new UINT8[32768];
    UINT32 ulTotalSize = 0;

    if (!pBuffer)
        return HXR_OUTOFMEMORY;

    while (*pPacket)
    {
        sendPacketFast(*(pPacket++), pBuffer + ulTotalSize, &ulSize);
        ulTotalSize += ulSize;
    }

    HX_RESULT hresult;
    if (ulTotalSize)
    {
        hresult = m_pFastPathNetWrite->FastWrite(pBuffer, ulTotalSize);
    }
    else
    {
        hresult = HXR_OK;
    }

    delete [] pBuffer;

    return hresult;
#else
    HX_ASSERT(0);
    return HXR_NOTIMPL;
#endif
}

HX_RESULT
TNGUDPTransport::sendPacket(BasePacket* pBasePacket)
{
    if (m_bMulticast)
    {
        return sendMulticastPacket(pBasePacket);
    }

#ifndef _MACINTOSH
    if (m_pFastPathNetWrite)
    {
        return sendPacketFast(pBasePacket);
    }
#endif
    RTSPStreamData* pStreamData;
    IHXPacket* pPacket;
    IHXBuffer* pBuffer;

    if(!m_bIsSource)
    {
        return HXR_UNEXPECTED;
    }

    pPacket = pBasePacket->PeekPacket();

    if (!pPacket)
    {
        return HXR_UNEXPECTED;
    }

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
        pResendBuffer->DiscardExpiredPackets(FALSE, pBasePacket->GetTime());
    }

    if (pPacket->IsLost())
    {
        return HXR_OK;
    }

    HX_RESULT hresult = HXR_OK;

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
                return HXR_INVALID_PARAMETER;
            }
        }
        HX_RELEASE(pRTPPacket);
        pStreamData->m_bFirstPacket = FALSE;
    }

    TNGDataPacket pkt;
    UINT32 report_offset = 0;
    TNGLatencyReportPacket rpt;

    BYTE* packet;
    if(createLatencyReportPacket(&rpt))
    {
        packet = new BYTE[rpt.static_size() + pkt.static_size() + pBuffer->GetSize()];
        /*
         * report_offset gets set in pack.
         */
        rpt.pack(packet, report_offset);
    }
    else
    {
        packet = new BYTE[pkt.static_size() + pBuffer->GetSize()];
    }
    UINT32 packetLen = 0;

    pkt.length_included_flag = 0;

    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;
        pkt.stream_id_expansion = streamNumber;
    }
    else
    {
        pkt.stream_id = (UINT8)HX_SAFEUINT(streamNumber);
    }

    /* We'll always include the reliable flag for now */
    pkt.need_reliable_flag = 1; // XXXSMP pStreamData->m_bNeedReliable;
    /*
     * XXXSMP - We don't need is_reliable.  We can imply this from the increase
     * of both sequence numbers.
     */
    pkt.is_reliable = (UINT8)(pBasePacket->m_uPriority == 10 ? 1 : 0);
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
    pkt.asm_rule_number = (UINT8)((ulRuleNumber > 63) ? 63 : ulRuleNumber);
    pkt.back_to_back_packet = (UINT8)pBasePacket->m_bBackToBack;

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

    m_ulPacketsSent++;
    m_lBytesSent += pBasePacket->GetSize();


    /*
     *  When sending out, the total size of the physical packet is
     *  to combination of the sizes of the logical packets.  In this
     *  case, the BasePacket size and report packet size.
     */

    // writePacket now owns 'packet', no need to free it in here
    hresult = writePacket(packet, packetLen + report_offset);

#ifdef DEBUG
 TNGsendContinue:
#endif

    pBuffer->Release();
    return hresult;
}

HX_RESULT
TNGUDPTransport::sendPacketFast(BasePacket* pBasePacket,
                                UINT8* pWriteHere,
                                UINT32* pSizeWritten)
{
#ifndef _MACINTOSH
    RTSPStreamData* pStreamData;
    IHXPacket* pPacket;
    IHXBuffer* pBuffer;

    HX_ASSERT(m_bIsSource == TRUE);

    pPacket = pBasePacket->PeekPacket();

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

    if (!pStreamData)
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
        pResendBuffer->DiscardExpiredPackets(FALSE, pBasePacket->GetTime());
    }

    if (pPacket->IsLost())
    {
        if (pSizeWritten != NULL)
        {
            *pSizeWritten = 0;
        }

        return HXR_OK;
    }

    HX_RESULT hresult = HXR_OK;

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
                return HXR_INVALID_PARAMETER;
            }
        }
        HX_RELEASE(pRTPPacket);
        pStreamData->m_bFirstPacket = FALSE;
    }


    TNGDataPacket pkt;
    UINT32 report_offset = 0;
    TNGLatencyReportPacket rpt;

    HX_ASSERT (rpt.static_size() + pkt.static_size() + pBuffer->GetSize()
               < 32768);

    BYTE* pData = new BYTE[32768];
    BYTE* packet;
    if (pWriteHere)
    {
        packet = pWriteHere;
    }
    else
    {
        packet = pData;
    }
    if(createLatencyReportPacket(&rpt))
    {
        /*
         * report_offset gets set in pack.
         */
        rpt.pack(packet, report_offset);
    }
    UINT32 packetLen = 0;

    /* pWriteHere indicates that we are aggregating */
    pkt.length_included_flag = (UINT8)(pWriteHere ? 1 : 0);

    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;
        pkt.stream_id_expansion = streamNumber;
    }
    else
    {
        pkt.stream_id = (UINT8)HX_SAFEUINT(streamNumber);
    }

    /* We'll always include the reliable flag for now */
    pkt.need_reliable_flag = 1; // XXXSMP pStreamData->m_bNeedReliable;
    /*
     * XXXSMP - We don't need is_reliable.  We can imply this from the increase
     * of both sequence numbers.
     */
    pkt.is_reliable = (UINT8)(pBasePacket->m_uPriority == 10 ? 1 : 0);
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
    pkt.asm_rule_number = (UINT8)((ulRuleNumber > 63) ? 63 : ulRuleNumber);
    pkt.back_to_back_packet = (UINT8)pBasePacket->m_bBackToBack;

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

    m_ulPacketsSent++;
    m_lBytesSent += pBasePacket->GetSize();


    /*
     *  When sending out, the total size of the physical packet is
     *  to combination of the sizes of the logical packets.  In this
     *  case, the BasePacket size and report packet size.
     */

    if (pWriteHere)
    {
        if (pSizeWritten != NULL)
        {
            *pSizeWritten = packetLen + report_offset;
        }
        hresult = HXR_OK;
    }
    else
    {
        hresult = m_pFastPathNetWrite->FastWrite(packet,
                                                 packetLen + report_offset);
    }

#ifdef DEBUG
 TNGsendContinue:
#endif

    delete [] pData;
    pBuffer->Release();
    return hresult;
#else
    HX_ASSERT(0);
    return HXR_NOTIMPL;
#endif
}


HX_RESULT
TNGUDPTransport::sendMulticastPacket(BasePacket* pBasePacket)
{
    RTSPStreamData* pStreamData;
    IHXPacket* pPacket;
    IHXBuffer* pBuffer;

    if(!m_bIsSource)
    {
        return HXR_UNEXPECTED;
    }

    pPacket = pBasePacket->GetPacket();

    if(!pPacket)
    {
        return HXR_UNEXPECTED;
    }

    // copied from :sendPacket()
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

    // copied from :sendPacket()
    if (pPacket->IsLost())
    {
        return HXR_OK;
    }


    HX_RESULT hresult = HXR_OK;

    TNGDataPacket pkt;
    UINT32 report_offset = 0;
    TNGLatencyReportPacket rpt;

    BYTE* packet;
    if(createLatencyReportPacket(&rpt))
    {
        packet = new BYTE[rpt.static_size() + pkt.static_size() + pBuffer->GetSize()];
        /*
         * report_offset gets set in pack.
         */
        rpt.pack(packet, report_offset);
    }
    else
    {
        packet = new BYTE[pkt.static_size() + pBuffer->GetSize()];
    }
    UINT32 packetLen = 0;

    pkt.length_included_flag = 0;

    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;
        pkt.stream_id_expansion = streamNumber;
    }
    else
    {
        pkt.stream_id = (UINT8)HX_SAFEUINT(streamNumber);
    }

    /* We'll always include the reliable flag for now */
    pkt.need_reliable_flag = 1; // XXXSMP pStreamData->m_bNeedReliable;
    /*
     * XXXSMP - We don't need is_reliable.  We can imply this from the increase
     * of both sequence numbers.
     */
    pkt.is_reliable = (UINT8)(pBasePacket->m_uPriority == 10 ? 1 : 0);
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
    pkt.asm_rule_number = (UINT8)((ulRuleNumber > 63) ? 63 : ulRuleNumber);
    pkt.back_to_back_packet = (UINT8)pBasePacket->m_bBackToBack;

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

    m_ulPacketsSent++;
    m_lBytesSent += pBasePacket->GetSize();


    /*
     *  When sending out, the total size of the physical packet is
     *  to combination of the sizes of the logical packets.  In this
     *  case, the BasePacket size and report packet size.
     */

    // writePacket now owns 'packet', no need to free it in here
    hresult = writePacket(packet, packetLen + report_offset);

#ifdef DEBUG
 TNGsendContinue:
#endif

    pBuffer->Release();
    pPacket->Release();
    return hresult;
}

HX_RESULT
TNGUDPTransport::sendToResendBuffer(BasePacket* pBasePacket)
{
    return HXR_OK;
    RTSPStreamData* pStreamData;
    IHXPacket* pPacket = pBasePacket->GetPacket();

    if(!pPacket)
    {
        return HXR_UNEXPECTED;
    }

    UINT16 streamNumber = pPacket->GetStreamNumber();

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    RTSPResendBuffer* pResendBuffer = pStreamData->m_pResendBuffer;

    if (pResendBuffer)
    {
        pResendBuffer->Add(pBasePacket);
        pResendBuffer->DiscardExpiredPackets(FALSE, pBasePacket->GetTime());
    }
}

void
TNGUDPTransport::setLatencyPeriod(UINT32 ulMs)
{
    m_ulLatencyReportPeriod = ulMs;
}

/**************************************************************************
 * TNGUDPTransport::createLatencyReportPacket(TNGLatencyReportPacket* pRpt
 *
 * Fills in the fields for the passed in TNGLatencyReportPacket with the
 * expectation that later pack will be called on it.
 */
HXBOOL
TNGUDPTransport::createLatencyReportPacket(TNGLatencyReportPacket* pRpt)
{
    UINT32 now;
    HXBOOL bNowIsReal = FALSE;

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

        now = (UINT32)(tv.tv_usec / 1000 + tv.tv_sec * 1000);
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

            now = (UINT32)(tv.tv_usec / 1000 + tv.tv_sec * 1000);
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

        pRpt->length = (UINT16)pRpt->static_size(); //we know static_size < 256
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

HX_RESULT
TNGUDPTransport::sendBWReportPacket(INT32 aveBandwidth, INT32 packetLoss,
                                    INT32 bandwidthWanted)
{
    HX_RESULT hresult = HXR_OK;
    TNGReportPacket pkt;

    UINT32 dataLen = sizeof(INT32) * 3;
    BYTE* packet = new BYTE[pkt.static_size() + dataLen];
    UINT32 packetLen = 0;

    pkt.length_included_flag = 0;
    pkt.dummy0 = 0;
    pkt.dummy1 = 0;
    pkt.dummy2 = 0;
    pkt.packet_type = (UINT16)0xff01;
    pkt.data.len = HX_SAFEUINT(dataLen);
    pkt.data.data = new INT8[dataLen];
    BYTE* pOff = (BYTE*)pkt.data.data;
    putlong(pOff, (UINT32)aveBandwidth);
    pOff += 4;
    putlong(pOff, (UINT32)packetLoss);
    pOff += 4;
    putlong(pOff, (UINT32)bandwidthWanted);
    pkt.pack(packet, packetLen);
    // writePacket now owns 'packet', no need to free it in here
    hresult = writePacket(packet, packetLen);

    return hresult;
}

HX_RESULT
TNGUDPTransport::sendACKPacket()
{
    HX_RESULT result = HXR_OK;

    if(!m_pStreamHandler)
    {
        return HXR_UNEXPECTED;
    }

    /*
     * Initialize the streams so that they all have the opportunity to take
     * part in at least one ACK packet
     */

    RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

    while(pStreamData)
    {
        pStreamData->m_bNeedToACK = TRUE;
        pStreamData = m_pStreamHandler->nextStreamData();
    }

    /*
     * Now do the ACKing
     */

    CHXSimpleList dataList;

 RestartACK:

    /*
     * Must create bitset here because if we are restarting the ACK, the
     * CHXBitset must also be resized
     */

    CHXBitset bitset;
    HXBOOL bSendPacket = FALSE;
    HXBOOL bRestartACK = FALSE;
    UINT32 dataLen = 0;
#ifndef XXXTEMPLOSTHIGH
    HXBOOL xxxTempLostHigh = FALSE;
#endif

    pStreamData = m_pStreamHandler->firstStreamData();

    while(pStreamData)
    {
        if (!pStreamData->m_bNeedToACK)
        {
            pStreamData = m_pStreamHandler->nextStreamData();
            continue;
        }

        bitset.clear();
        HXBOOL lostHigh = FALSE;
        HX_RESULT result = HXR_OK;
        UINT16 maxSeqNo = 0;
        UINT16 bitCount = 0;
        HXBOOL didACK = FALSE;
        HXBOOL bNeedAnotherACK = FALSE;

        pStreamData->m_bNeedToACK = FALSE;

        result =
            pStreamData->m_pTransportBuffer->SetupForACKPacket(maxSeqNo,
                                                               bitset,
                                                               bitCount,
                                                               didACK,
                                                               lostHigh,
                                                               bNeedAnotherACK);

        if (result != HXR_OK)
        {
            /*
             * Even if this stream has no information to report,
             * other streams may
             */

            if (result == HXR_NO_DATA)
            {
                pStreamData = m_pStreamHandler->nextStreamData();
                continue;
            }

            return result;
        }
        else if (bNeedAnotherACK)
        {
            bRestartACK = TRUE;
            pStreamData->m_bNeedToACK = TRUE;
        }

        BYTE* pByteArray = 0;
        INT32 nByteCount = 0;

        nByteCount = bitset.toByteArray(&pByteArray);

        if(didACK)
        {
            bSendPacket = TRUE; // there's something to send
#ifndef XXXTEMPLOSTHIGH
            INT32 nBitmapDataLen = (INT32)(nByteCount      +
                sizeof(INT16)*3 +
                sizeof(INT8));
#else
            INT32 nBitmapDataLen = nByteCount      +
                sizeof(INT16)*3 +
                sizeof(INT8)*2;
#endif
            BYTE* pBitmapData = new BYTE[nBitmapDataLen];
            BYTE* pOff = pBitmapData;

            putshort(pOff, pStreamData->m_streamNumber);
            pOff += sizeof(INT16);
            putshort(pOff, maxSeqNo);
            pOff += sizeof(INT16);
            putshort(pOff,bitCount);
            pOff += sizeof(INT16);
            *pOff++ = (BYTE)nByteCount;
#ifndef XXXTEMPLOSTHIGH
            if (lostHigh)
            {
                xxxTempLostHigh = TRUE;
            }
#else
            *pOff++ = (BYTE)lostHigh;
#endif
            for(UINT16 j=0;j<nByteCount;++j)
            {
                *pOff++ = pByteArray[j];
            }

            BitmapDataItem* pDataItem = new BitmapDataItem;
            pDataItem->m_pData = pBitmapData;
            pDataItem->m_dataLen = (UINT32)nBitmapDataLen;
            dataList.AddTail(pDataItem);
            dataLen += (UINT32)nBitmapDataLen;
            delete[] pByteArray;
        }

        pStreamData = m_pStreamHandler->nextStreamData();
    }

    if(bSendPacket)
    {
        TNGACKPacket pkt;

        pkt.length_included_flag = 0;
        pkt.dummy0 = 0;

        pkt.dummy1 = 0;
        pkt.packet_type = (UINT16)0xff02;

        BYTE* pMegaData = new BYTE[dataLen];
        LISTPOSITION pos = dataList.GetHeadPosition();
        UINT32 offset = 0;
        while(pos)
        {
            BitmapDataItem* pDataItem =
                (BitmapDataItem*)dataList.GetAt(pos);
            memcpy(&pMegaData[offset], pDataItem->m_pData, /* Flawfinder: ignore */
                   HX_SAFESIZE_T(pDataItem->m_dataLen));
            offset += pDataItem->m_dataLen;
            delete [] pDataItem->m_pData;
            delete pDataItem;   // don't need it now!
            pos = dataList.RemoveAt(pos);
        }
#ifndef XXXTEMPLOSTHIGH
        pkt.lost_high = (UINT8)xxxTempLostHigh;
#endif
        pkt.data.data = (INT8*)pMegaData;
        pkt.data.len = HX_SAFEUINT(dataLen);

        BYTE* packet = new BYTE[pkt.static_size() + dataLen];
        UINT32 packetLen = 0;
        pkt.pack(packet, packetLen);
        // writePacket now owns 'packet', no need to free it in here
        result = writePacket(packet, packetLen);

        delete[] pMegaData;
    }
    else
    {
        // Let the caller know that no data was sent
        result = HXR_NO_DATA;
    }

    /*
     * If there was a stream that needs to ACK some more, create another
     * ACK packet
     */

    if (bRestartACK)
    {
        goto RestartACK;
    }

    /*
     * Clean up and set the next ACK timeout
     */

    if (m_bCallbackPending)
    {
        m_pScheduler->Remove(m_ackTimeoutID);
    }

    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
    Timeval now((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);
    Timeval nextTimeout;
    *m_pLastACKTime = now;
    nextTimeout = now + Timeval((INT32)(m_ulNormalACKInterval*1000));
    lTime.tv_sec = (UINT32)nextTimeout.tv_sec;
    lTime.tv_usec = (UINT32)nextTimeout.tv_usec;
    m_bCallbackPending = TRUE;
    m_ackTimeoutID = m_pScheduler->AbsoluteEnter(m_pACKCallback, lTime);

    return result;
}

HX_RESULT
TNGUDPTransport::sendNAKPacket(UINT16 uStreamNumber,
                               UINT16 uBeginSeqNo,
                               UINT16 uEndSeqNo)
{
    UINT16 nBitCount  = (UINT16)(uEndSeqNo-uBeginSeqNo);
    UINT16 nByteCount = 0;
    UINT16 nDataLen   = 0;

    DEBUG_OUT(m_pErrMsg, DOL_RTSP,
              (s,"(%u, %p) RDTNAK %u %u %u",
               HX_GET_TICKCOUNT(), this,
               uStreamNumber,
               uBeginSeqNo,
               uEndSeqNo));

    if (m_pBwMgrInput && !m_bMulticast)
    {
        m_pBwMgrInput->ReportDataPacket(uBeginSeqNo,
                                        uEndSeqNo,
                                        REPORT_DATA_PACKET_LOST);
    }

    if( nBitCount > 0 )
        nByteCount = (UINT16)(nBitCount / 8 + 1);

    HX_RESULT result = HXR_OK;
    TNGACKPacket pkt;

    pkt.length_included_flag = 0;
    pkt.dummy0 = 0;
    pkt.dummy1 = 0;
    pkt.packet_type = (UINT16)0xff02;

#ifndef XXXTEMPLOSTHIGH
    nDataLen = (UINT16)(7 + nByteCount);
#else
    nDataLen = 8+nByteCount;
#endif
    BYTE* pData = new BYTE[nDataLen];
    memset( pData, 0, nDataLen );
    BYTE* pOff = pData;

    putshort(pOff, uStreamNumber);
    pOff += sizeof(INT16);
    putshort(pOff, uEndSeqNo);
    pOff += sizeof(INT16);
    putshort(pOff, nBitCount);
    pOff += sizeof(INT16);
    HX_ASSERT(nByteCount <= 0xff);
    *pOff++ = BYTE(nByteCount);
#if XXXTEMPLOSTHIGH
    *pOff++ = TRUE;
#endif

#ifndef XXXTEMPLOSTHIGH
    pkt.lost_high = 1;
#endif
    pkt.data.data = (INT8*)pData;
    pkt.data.len = HX_SAFEUINT(nDataLen);
    BYTE* packet = new BYTE[pkt.static_size() + nDataLen];

    UINT32 packetLen = 0;
    pkt.pack(packet, packetLen);
    // writePacket now owns 'packet', no need to free it in here
    result = writePacket(packet, packetLen);

    delete[] pData;

    return result;
}

HX_RESULT
TNGUDPTransport::sendRTTRequestPacket()
{
    HX_RESULT hresult = HXR_OK;
    TNGRTTRequestPacket pkt;

    BYTE* packet = new BYTE[pkt.static_size()];
    UINT32 packetLen = 0;

    pkt.dummy0 = 0;
    pkt.dummy1 = 0;
    pkt.dummy2 = 0;
    pkt.dummy3 = 0;
    pkt.packet_type = (UINT16)0xff03;
    pkt.pack(packet, packetLen);
    // writePacket now owns 'packet', no need to free it in here
    hresult = writePacket(packet, packetLen);

    return hresult;
}

HX_RESULT
TNGUDPTransport::sendRTTResponsePacket(UINT32 secs, UINT32 uSecs)
{
    HX_RESULT hresult = HXR_OK;
    TNGRTTResponsePacket pkt;

    BYTE* packet = new BYTE[pkt.static_size()];
    UINT32 packetLen = 0;

    pkt.dummy0 = 0;
    pkt.dummy1 = 0;
    pkt.dummy2 = 0;
    pkt.dummy3 = 0;
    pkt.packet_type = (UINT16)0xff04;
    pkt.timestamp_sec = secs;
    pkt.timestamp_usec = uSecs;
    pkt.pack(packet, packetLen);
    // writePacket now owns 'packet', no need to free it in here
    hresult = writePacket(packet, packetLen);

    return hresult;
}

HX_RESULT
TNGUDPTransport::sendCongestionPacket(INT32 xmitMultiplier,
                                      INT32 recvMultiplier)
{
    HX_RESULT hresult = HXR_OK;
    TNGCongestionPacket pkt;

    BYTE* packet = new BYTE[pkt.static_size()];
    UINT32 packetLen = 0;

    pkt.dummy0 = 0;
    pkt.dummy1 = 0;
    pkt.dummy2 = 0;
    pkt.dummy3 = 0;
    pkt.packet_type = (UINT16)0xff05;
    pkt.xmit_multiplier = xmitMultiplier;
    pkt.recv_multiplier = recvMultiplier;
    pkt.pack(packet, packetLen);
    // writePacket now owns 'packet', no need to free it in here
    hresult = writePacket(packet, packetLen);

    return hresult;
}

HX_RESULT
TNGUDPTransport::sendStreamEndPacket(UINT16 streamNumber,
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

    BYTE* packet = new BYTE[pkt.static_size()];
    UINT32 packetLen = 0;

    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;
        pkt.stream_id_expansion = streamNumber;
    }
    else
    {
        pkt.stream_id = (UINT8)HX_SAFEUINT(streamNumber);
    }

    /* We'll always include the reliable flag for now*/
    pkt.need_reliable_flag = 1;

    if(pkt.need_reliable_flag)
    {
        pkt.total_reliable = pStreamData->m_reliableSeqNo;
    }

    pkt.seq_no = pStreamData->m_seqNo;
    pkt.timestamp = pStreamData->m_lastTimestamp;
    pkt.packet_sent = (UINT8)pStreamData->m_packetSent;
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
    pkt.packet_type = (UINT16)0xff06;
    pkt.pack(packet, packetLen);
    // writePacket now owns 'packet', no need to free it in here
    hresult = writePacket(packet, packetLen);

    return hresult;
}

HX_RESULT
TNGUDPTransport::writePacket(BYTE* pData, UINT32 dataLen)
{
    HX_RESULT rc = HXR_OK;
    IHXBuffer* pSendBuffer = NULL;
    if (HXR_OK == CreateAndSetBufferCCF(pSendBuffer, pData, dataLen, m_pContext))
    {
	rc = m_pUDPSocket->Write(pSendBuffer);
	HX_RELEASE(pSendBuffer);
    }
    return rc;
}

#define DO_GENERATE_STAMP if (!ulTimeStamp) ulTimeStamp = GenerateTimeStamp(pBuffer);
inline UINT32
GenerateTimeStamp(IHXBuffer* pBuffer)
{
    UINT32 ulReturn;
    /*
     *  See if this buffer was marked with a timestamp.  If not, use now.
     */
    IHXTimeStampedBuffer* pTBuff;
    if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer,
                                         (void **)&pTBuff))
    {
        // The buffer timestamp is in microseconds and this function
        // needs to always return milliseconds
        ulReturn = pTBuff->GetTimeStamp() / 1000;
        pTBuff->Release();
    }
    else
    {
        ulReturn = HX_GET_TICKCOUNT();
    }
    return ulReturn;
}

HX_RESULT
TNGUDPTransport::handlePacket(IHXBuffer* pBuffer)
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
            if (m_bMulticast)
            {
                DO_GENERATE_STAMP
                    rc = handleMulticastDataPacket(pBuffer, &pos, &len, ulTimeStamp);
            }
            else
            {
                DO_GENERATE_STAMP
                    rc = handleDataPacket(pBuffer, &pos, &len, ulTimeStamp);
            }
        }
        else
        {
            switch(packetType)
            {
            case 0xff00:        // ASM Action packet
                rc = handleASMActionPacket(pBuffer, &pos, &len);
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
                rc = handleStreamEndPacket(pBuffer, &pos, &len);
                break;
            case 0xff08:
                DO_GENERATE_STAMP
                    rc = handleLatencyReportPacket(pBuffer, &pos, &len,
                                                   ulTimeStamp);
                break;
            case 0xff09:
                DO_GENERATE_STAMP
                    rc = handleTransportInfoReqPacket(pBuffer, &pos,&len,
                                                      ulTimeStamp);
                break;
            case 0xff0a:
                rc = handleTransportInfoRespPacket(pBuffer, &pos, &len);
                break;
            case 0xff0b:
                rc = handleAutoBWDetectionPacket(pBuffer, &pos, &len);
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
TNGUDPTransport::releasePackets()
{
    /*
     * A source is not going to have a transport buffer
     */

    if (m_bIsSource)
    {
        return HXR_OK;
    }

    RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

    while(pStreamData)
    {
        if (pStreamData->m_pTransportBuffer)
        {
            pStreamData->m_pTransportBuffer->ReleasePackets();
        }

        pStreamData = m_pStreamHandler->nextStreamData();
    }

    return HXR_OK;
}

HX_RESULT
TNGUDPTransport::handleDataPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                  UINT32* pLen, UINT32 ulTimeStamp)
{
    TNGDataPacket pkt;
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
        printf("illegal length in data packet <%lu>\n", pkt.data.len);
        fflush(0);
#endif
        return HXR_UNEXPECTED;
    }

    if (pkt.length_included_flag)
    {
        /*
         * There was a length specified inside the TNGPacket.
         * We should only unpack this packet.
         */
        if((pOff = pkt.unpack(pData, pkt._packlenwhendone)) == 0)
        {
            *pLen = 0;
            return HXR_UNEXPECTED;
        }
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    UINT32 dataOffset;
    UINT16 streamNumber = (UINT16)((pkt.stream_id == 31) ? pkt.stream_id_expansion:
        pkt.stream_id);
    UINT16 seqNo = pkt.seq_no;
    UINT16 totalReliable = 0;
    UINT16 asmRuleNumber = (UINT16)((pkt.asm_rule_number == 63) ?
        pkt.asm_rule_number_expansion : pkt.asm_rule_number);
    if(pkt.need_reliable_flag == 1)
    {
        totalReliable = pkt.total_reliable;
    }

    CHXPacket* pPacket = new CHXPacket;
    pPacket->AddRef();

    dataOffset = (UINT32)pkt.data.data - (UINT32)pBuffer->GetBuffer();

    HX_RESULT hresult = HXR_OK;
    IHXBuffer* pPktBuffer = NULL; 
    hresult = CreateStaticBuffer(pBuffer,dataOffset,pkt.data.len,pPktBuffer);
    HX_ASSERT(HXR_OK == hresult);
    if(FAILED(hresult))
    {
        HX_RELEASE(pPacket);
        return hresult;
    } 
    // XXXSMP Transport flags over the wire?
    pPacket->Set(pPktBuffer, pkt.timestamp, streamNumber,
                 HX_ASM_SWITCH_ON, asmRuleNumber);

    if (m_ulBackToBackTime)
    {
        UINT32 uTS = ulTimeStamp * 1000;

        IHXTimeStampedBuffer* pTSBuffer = NULL;
        if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer, 
                                             (void **)&pTSBuffer))
        {
            uTS = pTSBuffer->GetTimeStamp();
            HX_RELEASE(pTSBuffer);
        }

        UINT32 ulTime = uTS - m_ulBackToBackTime;

        /* Sanity Check */
        if (ulTime < 5000000 && m_pBwMgrInput)
        {
            m_pBwMgrInput->ReportUpshiftInfo(ulTime, pkt.data.len);
        }

        m_ulBackToBackTime = 0;
    }

    if (pkt.back_to_back_packet)
    {
        IHXTimeStampedBuffer* pTSBuffer = NULL;
        if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer, 
                                             (void **)&pTSBuffer))
        {
            m_ulBackToBackTime = pTSBuffer->GetTimeStamp();
            HX_RELEASE(pTSBuffer);
        }
        else
        {
            m_ulBackToBackTime = ulTimeStamp * 1000;
        }
    }

    if (m_pBwMgrInput)
    {
        m_pBwMgrInput->ReportDataPacket
            (pkt.timestamp, ulTimeStamp, pkt.data.len);
    }


    /*
     * Record uses TransportBuffer but will push packets out
     */
    hresult = storePacket(pPacket,
                          streamNumber,
                          seqNo,
                          totalReliable,
                          pkt.is_reliable);

    pPktBuffer->Release();
    pPacket->Release();

    return hresult;
}

HX_RESULT
TNGUDPTransport::handleMulticastDataPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                           UINT32* pLen, UINT32 ulTimeStamp)
{
    TNGDataPacket pkt;
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
        printf("illegal length in multicast data packet <%lu>\n", pkt.data.len);
        fflush(0);
#endif
        return HXR_UNEXPECTED;
    }

    if (pkt.length_included_flag)
    {
        /*
         * There was a length specified inside the TNGPacket.
         * We should only unpack this packet.
         */
        if((pOff = pkt.unpack(pData, pkt._packlenwhendone)) == 0)
        {
            *pLen = 0;
            return HXR_UNEXPECTED;
        }
    }
    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    UINT32 dataOffset;
    UINT16 streamNumber = (UINT16)((pkt.stream_id == 31) ? pkt.stream_id_expansion:
        pkt.stream_id);
    UINT16 seqNo = pkt.seq_no;
    UINT16 totalReliable = 0;
    UINT16 asmRuleNumber = (UINT16)((pkt.asm_rule_number == 63) ?
        pkt.asm_rule_number_expansion : pkt.asm_rule_number);
    if(pkt.need_reliable_flag == 1)
    {
        totalReliable = pkt.total_reliable;
    }

    CHXPacket* pPacket = new CHXPacket;
    pPacket->AddRef();

    dataOffset = (UINT32)pkt.data.data - (UINT32)pBuffer->GetBuffer();

    HX_RESULT hresult = HXR_OK;
    IHXBuffer* pPktBuffer = NULL;
    hresult = CreateStaticBuffer(pBuffer,dataOffset,pkt.data.len,pPktBuffer);

    HX_ASSERT(HXR_OK == hresult);
    if(FAILED(hresult))
    {
        HX_RELEASE(pPacket);
        return hresult;
    }
    // XXXSMP Transport flags over the wire?
    pPacket->Set(pPktBuffer, pkt.timestamp, streamNumber,
                 HX_ASM_SWITCH_ON, asmRuleNumber);

    if (m_ulBackToBackTime)
    {
        m_ulBackToBackTime = 0;

#if 0
        UINT32 ulTime = ulTimeStamp - m_ulBackToBackTime;
        /* Don't do this stuff for Mcast */
        /* Sanity Check */
        if (ulTime < 5000 && m_pBwMgrInput)
        {
            m_pBwMgrInput->ReportUpshiftInfo(ulTime, pkt.data.len);
        }
#endif
    }

    if (pkt.back_to_back_packet)
    {
        m_ulBackToBackTime = ulTimeStamp;
    }

#if 0
/* Don't do this stuff for Mcast */
    if (m_pBwMgrInput)
    {
        m_pBwMgrInput->ReportDataPacket
            (pkt.timestamp, ulTimeStamp, pkt.data.len);
    }
#endif


    /*
     * Record uses TransportBuffer but will push packets out
     */
    hresult = storePacket(pPacket,
                          streamNumber,
                          seqNo,
                          totalReliable,
                          pkt.is_reliable);

    pPktBuffer->Release();
    pPacket->Release();

    return hresult;
}

HX_RESULT
TNGUDPTransport::handleLatencyReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
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
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    /* back out the 1.01 factor applied by the server 
     *  new_server_out_time = server_out_time / 1.01 
     *                      = x * 100 / 101 
     *                      = (101 * x  - x) / 101
     *                      = (101 * x / 101) - (x / 101)
     *                      = x - (x / 101)
     *  since we are doing integer math, we need to make
     *  sure that the (x / 101) is rounded up to simulate
     *  the integer truncation. That means we need to change
     *  (x / 101) to ((x + 100) / 101). Adding 100 can cause
     *  overflow problems so we change its form.
     *  (x + 100) / 101 = x / 101 + 100 / 101
     *                  = x / 101 + (101 / 101 - 1 / 101)
     *                  = 1 + (x - 1) / 101
     *  This provides the correct value for all x except 0
     */
    if (pkt.server_out_time)
    {
        pkt.server_out_time -= 1 + (pkt.server_out_time - 1) / 101;
    }

    /*
     * Tell the bandwidth manager about this.
     */
    if (m_pBwMgrInput && !m_bMulticast)
    {
        m_pBwMgrInput->ReportLatency(pkt.server_out_time, ulTime);
    }
    return HXR_OK;
}

HX_RESULT
TNGUDPTransport::handleASMActionPacket(IHXBuffer* pBuffer, UINT32* pPos,
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
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    if(pkt.stream_id == 31)
    {
        streamNumber = pkt.stream_id_expansion;
    }
    else
    {
        streamNumber = pkt.stream_id;
    }

    // call subscribe/unsubscribe/etc


    return HXR_UNEXPECTED;
}

HX_RESULT
TNGUDPTransport::handleTransportInfoReqPacket(IHXBuffer* pBuffer, UINT32* pPos,
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
        (*pLen) -= (UINT32)(pOff - pData);
        (*pPos) += (UINT32)(pOff - pData);

        UINT32 streamCount = m_pStreamHandler->streamCount();
        UINT32 pktBufSize = RDTTransportInfoResponsePacket().static_size();

        if (transportInfoPkt.request_buffer_info)
        {
            pktBufSize += RDTBufferInfo().static_size() * streamCount;
        }

        BYTE* packet = new BYTE[pktBufSize];

        if (packet && m_pStreamHandler)
        {
            // Pack the response into the buffer
            BYTE* pStart = PackTransportInfoResp(transportInfoPkt,
                                                 ulTime,
                                                 m_pStreamHandler,
                                                 m_pSrcBufferStats,
                                                 packet);

            // writePacket now owns 'packet', no need to free it in here
            res = writePacket(packet, (UINT32)(pStart - packet));
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        *pLen = 0;
    }

    return res;
}

HX_RESULT
TNGUDPTransport::handleTransportInfoRespPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen)
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
        (*pLen) -= (UINT32)(pOff - pData);
        (*pPos) += (UINT32)(pOff - pData);

        res = HXR_OK;
    }
    else
    {
        *pLen = 0;
    }

    return res;
}

HX_RESULT
TNGUDPTransport::handleAutoBWDetectionPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen)
{
    TNGBWProbingPacket pkt;
    BYTE* pOff = NULL;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;
    UINT32 ulTS = 0;
    IHXTimeStampedBuffer* pTSBuffer = NULL;

    if ((pOff = pkt.unpack(pData, *pLen)) != 0)
    {
        /*
         * Subtract out the length that we used for this packet.
         */
        (*pLen) -= (UINT32)(pOff - pData);
        (*pPos) += (UINT32)(pOff - pData);
    }
    else
    {
        *pLen = 0;
    }

    if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer, (void **)&pTSBuffer))
    {
        ulTS = pTSBuffer->GetTimeStamp();
        HX_RELEASE(pTSBuffer);
    }
    else
    {
        HX_ASSERT(FALSE);
    }

    return handleABDPktInfo(UDPMode, pkt.seq_no, pkt.timestamp, ulTS, pkt.length);
}

HX_RESULT
TNGUDPTransport::handleACKPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen)
{
    TNGACKPacket pkt;

    if(!m_bIsSource)
    {
        return HXR_UNEXPECTED;
    }

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

    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    /*
     * XXXGH...what's is the test for headerLen checking?
     */

    UINT32 headerLen = sizeof(INT16)*2 + sizeof(INT8);
    if(pkt.data.len > headerLen)
    {
        BYTE* pOff = (BYTE*)pkt.data.data;
        while(pOff - (BYTE*)pkt.data.data < (INT32)pkt.data.len)
        {
            UINT16 streamNumber = getshort(pOff);
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
                uSeqNo = (UINT16)((maxSeqNo - 1) - i);

                if (uSeqNo > maxSeqNo)
                {
                    uSeqNo = (UINT16) ((m_wrapSequenceNumber - 1) - j++);
                }

                if (i < bitset.getNumBits())
                {
                    if (bitset.test(i))
                    {
                        ackList[ackIdx++] = (UINT16)HX_SAFEUINT(uSeqNo);
                    }
                    else
                    {
                        nakList[nakIdx++] = (UINT16)HX_SAFEUINT(uSeqNo);
                    }
                }
            }

            RTSPResendBuffer* pResendBuffer = getResendBuffer(streamNumber);

            m_pResp->OnACK(HXR_OK, pResendBuffer, streamNumber, m_sessionID,
                           ackList, ackIdx, nakList, nakIdx);

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
TNGUDPTransport::handleRTTRequestPacket(IHXBuffer* pBuffer, UINT32* pPos,
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
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    return m_pResp->OnRTTRequest(HXR_OK, m_sessionID);
}

HX_RESULT
TNGUDPTransport::handleRTTResponsePacket(IHXBuffer* pBuffer, UINT32* pPos,
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
TNGUDPTransport::handleCongestionPacket(IHXBuffer* pBuffer, UINT32* pPos,
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
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    xmitMultiplier = pkt.xmit_multiplier;
    recvMultiplier = pkt.recv_multiplier;

    return m_pResp->OnCongestion(HXR_OK, m_sessionID, xmitMultiplier,
                                 recvMultiplier);

    return HXR_OK;
}

HX_RESULT
TNGUDPTransport::handleStreamEndPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                       UINT32* pLen)
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::handleStreamEndPacket()", this);
    TNGStreamEndPacket pkt;
    UINT16 streamNumber;
    UINT16 seqNo;
    UINT16 reliableCount;
    HXBOOL   packetSent;
    UINT32 lastTimestamp;
    UINT32 ulEndReasonCode = 0;
    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;

    //If we haven't sent a PLAY request for this stream yet, ignore
    //this packet. The server has a problem where it will sometimes
    //sent a end of stream packet for a stream even though we have
    //not sent a play request yet.
    if( !HasPlayRequestBeenSent() )
    {
#ifdef RDT_MESSAGE_DEBUG
        RDTmessageFormatDebugFileOut("UDP Rejected a stream end packet because no play sent. %s\n",
                                     (const char*)m_sessionID);
#endif
        m_pResp->OnProtocolError(HXR_UNEXPECTED_STREAM_END);
        return HXR_UNEXPECTED;
    }

    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        return HXR_UNEXPECTED;
    }
    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    streamNumber = (UINT16)((pkt.stream_id == 31) ? pkt.stream_id_expansion:
        pkt.stream_id);
    seqNo = pkt.seq_no;
    reliableCount = (UINT16)((pkt.need_reliable_flag) ? pkt.total_reliable : 0);
    lastTimestamp = pkt.timestamp;
    packetSent = pkt.packet_sent;

    RTSPTransportBuffer* pTransportBuffer = getTransportBuffer(streamNumber);

    if (!pTransportBuffer)
    {
        return HXR_UNEXPECTED;
    }

#ifdef RDT_MESSAGE_DEBUG
    RDTmessageFormatDebugFileOut("UDP handling a SteamEndPacket: %s\n", (const char*)m_sessionID);
#endif

    if (pkt.ext_flag)
    {
        ulEndReasonCode = pkt.reason_code;
    }

    pTransportBuffer->SetEndPacket(seqNo,
                                   reliableCount,
                                   packetSent,
                                   lastTimestamp,
                                   ulEndReasonCode);

    return HXR_OK;
}

HX_RESULT
TNGUDPTransport::streamDone(UINT16 streamNumber,
                            UINT32 uReasonCode /* = 0 */,
                            const char* pReasonText /* = NULL */)
{
    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::streamDone(): str = %u; reason = %lu", this, streamNumber, uReasonCode);
    HX_RESULT hresult = HXR_OK;

    if (m_bIsSource)
    {
        for(int i=0;i<5;++i)
        {
            hresult = sendStreamEndPacket(streamNumber, uReasonCode, pReasonText);
        }
    }
    else
    {
        if (uReasonCode)
        {
            hresult = m_pResp->OnStreamDone(HXR_END_WITH_REASON, streamNumber);
        }
        else
        {
            hresult = m_pResp->OnStreamDone(HXR_OK, streamNumber);
        }
    }

    return hresult;
}


/*
 * ACKCallback methods
 */

TNGUDPTransport::ACKCallback::ACKCallback(TNGUDPTransport* pTransport):
    m_pTransport(pTransport),
    m_lAckRefCount(0)
{
    if(m_pTransport)
    {
        m_pTransport->AddRef();
    }
}
TNGUDPTransport::ACKCallback::~ACKCallback()
{
    HX_RELEASE(m_pTransport);
}

STDMETHODIMP
TNGUDPTransport::ACKCallback::QueryInterface(REFIID riid, void** ppvObj)
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
TNGUDPTransport::ACKCallback::AddRef()
{
    return InterlockedIncrement(&m_lAckRefCount);
}

STDMETHODIMP_(ULONG32)
TNGUDPTransport::ACKCallback::Release()
{
    if (InterlockedDecrement(&m_lAckRefCount) > 0)
    {
        return (ULONG32)m_lAckRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
TNGUDPTransport::ACKCallback::Func()
{
    if(m_pTransport)
    {
        m_pTransport->m_bCallbackPending = FALSE;
        m_pTransport->handleACKTimeout();
    }
    return HXR_OK;
}

void
TNGUDPTransport::setNormalACKInterval(UINT32 mSecs)
{
    m_ulNormalACKInterval = mSecs;
}

void
TNGUDPTransport::setMinimumACKInterval(UINT32 mSecs)
{
    m_ulMinimumACKInterval = mSecs;
}

/*
 * sort of like addStreamInfo()
 */
void
TNGUDPTransport::MulticastSetup(RTSPStreamHandler* pHandler)
{
    HX_ASSERT(pHandler);
    HX_ASSERT(!m_pStreamHandler);
    HX_ASSERT(m_bIsSource);

    m_bMulticast = TRUE;

    m_pStreamHandler = new RTSPStreamHandler(this);
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
            m_bIsSource,
            0, // not used
            m_bHackedRecordFlag,
            m_wrapSequenceNumber,
            0, // not used
            0, // not used
            pTSConverter,
            pStreamData->m_eMediaType);

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
TNGUDPTransport::JoinMulticast(IHXSockAddr* pAddr, IHXSocket* pSock)
{

    HXLOGL3(HXLOG_RTSP, "TNGUDPTransport[%p]::JoinMulticast(): port = %u", this, pAddr->GetPort());
    HX_ASSERT(pAddr);
    if (m_pMulticastAddr)
    {
        HX_ASSERT(m_pMulticastSocket);
        m_pMulticastSocket->LeaveGroup(m_pMulticastAddr, NULL);
        HX_RELEASE(m_pMulticastAddr);
    }
    else
    {
        if (pSock)
        {
            pSock->QueryInterface(IID_IHXMulticastSocket,
                                  (void**)&m_pMulticastSocket);
            HX_ASSERT(m_pMulticastSocket != NULL);
        }
        else
        {
            m_pUDPSocket->QueryInterface(IID_IHXMulticastSocket,
                                         (void**)&m_pMulticastSocket);
            HX_ASSERT(m_pMulticastSocket != NULL);
        }
    }

    m_pMulticastAddr = pAddr;
    m_pMulticastAddr->AddRef();
    m_bMulticast = TRUE;

    m_pMulticastSocket->JoinGroup(m_pMulticastAddr, NULL);

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
}

RTSPStreamHandler*
TNGUDPTransport::GetStreamHandler(void)
{
    m_pStreamHandler->AddRef();
    return m_pStreamHandler;
}

void
TNGUDPTransport::SetStreamHandler(RTSPStreamHandler* pHandler)
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
TNGUDPTransport::pauseBuffers()
{
    RTSPTransport::pauseBuffers();
    if (m_pMulticastAddr)
    {
        HX_ASSERT(m_pMulticastSocket);
        m_pMulticastSocket->LeaveGroup(m_pMulticastAddr, NULL);
    }


    return HXR_OK;
}

HX_RESULT
TNGUDPTransport::resumeBuffers()
{
    RTSPTransport::resumeBuffers();
    if (m_pMulticastAddr)
    {
        HX_ASSERT(m_pMulticastSocket);
        m_pMulticastSocket->JoinGroup(m_pMulticastAddr, NULL);
        m_bMulticast = TRUE;
    }


    return HXR_OK;
}

void TNGUDPTransport::setFeatureLevel(UINT32 uFeatureLevel)
{
    m_uFeatureLevel = uFeatureLevel;
}

/*
 * SeekCallback methods
 */

TNGUDPTransport::SeekCallback::SeekCallback(TNGUDPTransport* pTransport):
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

TNGUDPTransport::SeekCallback::~SeekCallback()
{
    HX_RELEASE(m_pTransport);
}

STDMETHODIMP
TNGUDPTransport::SeekCallback::QueryInterface(REFIID riid, void** ppvObj)
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
TNGUDPTransport::SeekCallback::AddRef()
{
    return InterlockedIncrement(&m_lAckRefCount);
}

STDMETHODIMP_(ULONG32)
TNGUDPTransport::SeekCallback::Release()
{
    if (InterlockedDecrement(&m_lAckRefCount) > 0)
    {
        return (ULONG32)m_lAckRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
TNGUDPTransport::SeekCallback::Func()
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
 * TNGTCPTransport methods
 */

TNGTCPTransport::TNGTCPTransport(HXBOOL bIsSource,
                HXBOOL bNoLostPackets /* = FALSE */,
                UINT32 uFeatureLevel /* = 0 */) :
    RTSPTransport(bIsSource),
    m_lRefCount(0),
    m_pTCPSocket(0),
    m_pFastSocket(0),
    m_bNoPacketBuffering(FALSE),
    m_bPacketsStarted(FALSE),
    m_pBwMgrInput(0),
    m_lastLatencyReportTime(0),
    m_LatencyReportStartTime(0),
    m_uFeatureLevel(uFeatureLevel),
    m_pAccurateClock(NULL),
    m_bNoLostPackets(bNoLostPackets)
{
#ifdef PAULM_TNGTCPTRANSTIMING
    g_TNGTCPTransTimer.Add(this);
#endif
    m_wrapSequenceNumber = TNG_WRAP_SEQ_NO;
}

TNGTCPTransport::~TNGTCPTransport()
{
    HXLOGL3(HXLOG_RTSP, "TNGTCPTransport[%p]::~TNGTCPTransport()", this);
#ifdef PAULM_TNGTCPTRANSTIMING
    g_TNGTCPTransTimer.Remove(this);
#endif
    HX_RELEASE(m_pBwMgrInput);

#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(m_pTCPSocket, 11);
#endif
    HX_RELEASE(m_pTCPSocket);
    HX_RELEASE(m_pFastSocket);
    HX_RELEASE(m_pAccurateClock);
}

STDMETHODIMP
TNGTCPTransport::QueryInterface(REFIID riid, void** ppvObj)
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
    else if (IsEqualIID(riid, IID_IHXWouldBlock) && m_pTCPSocket)
    {
        return m_pTCPSocket->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
TNGTCPTransport::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
TNGTCPTransport::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (UINT32)m_lRefCount;
    }
    delete this;
    return 0;
}


RTSPTransportTypeEnum
TNGTCPTransport::tag()
{
    return RTSP_TR_TNG_TCP;
}

HX_RESULT
TNGTCPTransport::init(IUnknown* pContext, IHXSocket* pSocket,
                      INT8 interleave, IHXRTSPTransportResponse* pResp)
{
    m_pTCPSocket = pSocket;
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(m_pTCPSocket, 11);
#endif
    m_pTCPSocket->AddRef();
    m_pResp = pResp;
    m_pResp->AddRef();
    m_tcpInterleave = interleave;

    /* Try to get the cheap, highly accurate clock from the server */
    pContext->QueryInterface(IID_IHXAccurateClock,
                             (void **)&m_pAccurateClock);

    m_pTCPSocket->QueryInterface(IID_IHXBufferedSocket,
                                 (void**)&m_pFastSocket);

    return Init(pContext);
}

HX_RESULT
TNGTCPTransport::sendPackets(BasePacket** pPacket)
{
    if(pPacket)
    {
        while(*pPacket)
        {
            sendPacket(*(pPacket++));
        }

    }
    return HXR_OK;
}

HX_RESULT
TNGTCPTransport::sendPacket(BasePacket* pBasePacket)
{
    if(!m_bIsSource)
    {
        return HXR_UNEXPECTED;
    }

    IHXPacket* pPacket = pBasePacket->GetPacket();
    IHXBuffer* pBuffer;
    UINT32 bufLen;

    if (!pPacket)
    {
        return HXR_UNEXPECTED;
    }
    else if (pPacket->IsLost())
    {
        pBuffer = 0;
        bufLen = 0;
        if (m_bNoLostPackets)
        {
            HX_RELEASE(pPacket);
            return HXR_OK;
        }
    }
    else
    {
        pBuffer = pPacket->GetBuffer();
        bufLen = pBuffer->GetSize();
    }

    UINT16 streamNumber = pPacket->GetStreamNumber();
    RTSPStreamData* pStreamData = m_pStreamHandler->getStreamData(streamNumber);

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
                return HXR_INVALID_PARAMETER;
            }
        }
        HX_RELEASE(pRTPPacket);
        pStreamData->m_bFirstPacket = FALSE;
    }

    if (!pStreamData)
    {
        return HXR_UNEXPECTED;
    }
    if (!pStreamData->m_packetSent)
    {
        pStreamData->m_packetSent = TRUE;
    }
    pStreamData->m_seqNo = pBasePacket->m_uSequenceNumber;

    TNGDataPacket pkt;
    UINT32 report_offset = 0;
    TNGLatencyReportPacket rpt;
    HX_RESULT hresult = HXR_OK;

    IHXBuffer* pSendBuffer = NULL;

    m_pCommonClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pSendBuffer);

    if(createLatencyReportPacket(&rpt))
    {
        // extra 4 bytes is for packet seperator
        pSendBuffer->SetSize(rpt.static_size() + pkt.static_size() + 4);

        /*
         * report_offset gets set in pack.
         */
        rpt.pack(pSendBuffer->GetBuffer()+4, report_offset);
    }
    else
    {
        pSendBuffer->SetSize(pkt.static_size()+4);
    }
    UINT32 packetLen = 0;
    pkt.length_included_flag = 0;
    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;
        pkt.stream_id_expansion = streamNumber;
    }
    else
    {
        pkt.stream_id = (UINT8)HX_SAFEUINT(streamNumber);
    }

    /* We'll always include the reliable flag for now*/
    pkt.need_reliable_flag = 1;
    pkt.is_reliable = (UINT8)(pBasePacket->m_uPriority == 10 ? 1: 0);
    pkt.seq_no = pStreamData->m_seqNo = pBasePacket->m_uSequenceNumber;
    if(pkt.need_reliable_flag)
    {
        pkt.total_reliable = pBasePacket->m_uReliableSeqNo;
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
    pkt.asm_rule_number = (UINT8)((ulRuleNumber > 63) ? 63: ulRuleNumber);


    if (ulRuleNumber > 63)
    {
        pkt.asm_rule_number_expansion = ulRuleNumber;
    }


    if (pBuffer)
    {
        // We will send payload seperately to avoid pack memcpy,
        // but include len so pack can fill out packet length field
        pkt.data.data = 0;
        pkt.data.len = HX_SAFEUINT(pBuffer->GetSize());
    }
    else
    {
        pkt.data.data = 0;
        pkt.data.len = 0;
    }

    /*
     * write to the buffer after the report.
     */

    pkt.pack(pSendBuffer->GetBuffer() + report_offset + 4, packetLen);
    //fprintf(stderr, "Generating packet %d %d %d %d\n", pSendBuffer->GetSize(), packetLen, pkt.data.len, report_offset);

    // include seperator here
    UINT8* packet = pSendBuffer->GetBuffer();
    packet[0] = '$';
    packet[1] = (UINT8)m_tcpInterleave;
    packetLen += report_offset;
    putshort(packet+2, (UINT16)packetLen);
    pSendBuffer->SetSize(packetLen - pBuffer->GetSize() + 4);

    if (m_pFastSocket) //XXXLCM figure out m_pFastSocket
    {
        hresult = writePacket(pSendBuffer);
    }
    else
    {
        hresult = m_pTCPSocket->Write(pSendBuffer);
    }


#if 0
    DPRINTF(D_INFO, ("Pkt: stream=%d seq=%u\n",
                     streamNumber, pkt.seq_no));
#endif


    m_ulPacketsSent++;
    m_lBytesSent += pBasePacket->GetSize();

    /*
     *  When sending out, the total size of the physical packet is
     *  to combination of the sizes of the logical packets.  In this
     *  case, the BasePacket size and report packet size.
     */
    if (m_pFastSocket)
    {
        hresult = writePacket(pBuffer);
    }
    else
    {
        hresult = m_pTCPSocket->Write(pBuffer);
    }



    HX_RELEASE(pSendBuffer);
    HX_RELEASE(pBuffer);
    HX_RELEASE(pPacket);


    return hresult;
}


/**************************************************************************
 * TNGTCPTransport::createLatencyReportPacket(TNGLatencyReportPacket* pRpt
 *
 * Fills in the fields for the passed in TNGLatencyReportPacket with the
 * expectation that later pack will be called on it.
 */

HXBOOL
TNGTCPTransport::createLatencyReportPacket(TNGLatencyReportPacket* pRpt)
{
    UINT32 now;
    HXBOOL bNowIsReal = FALSE;

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

        now = (UINT32)(tv.tv_usec / 1000 + tv.tv_sec * 1000);
        bNowIsReal = TRUE;
    }

    UINT32 interval_in_ms;

    /*
     * If this is our first time, make sure we send out a report right now.
     */
    if(!m_lastLatencyReportTime)
    {
        interval_in_ms = LATENCY_REPORT_INTERVAL_MS;
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

    if(interval_in_ms >= LATENCY_REPORT_INTERVAL_MS)
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

            now = (UINT32)(tv.tv_usec / 1000 + tv.tv_sec * 1000);
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

        pRpt->length = (UINT16)pRpt->static_size(); //we know static_size < 256
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

HX_RESULT
TNGTCPTransport::sendStreamEndPacket(UINT16 streamNumber,
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
    IHXBuffer* pBuffer = 0;

    BYTE* packet = NULL;
    BYTE* header = NULL;

    if (m_pFastSocket)
    {
        m_pCommonClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pBuffer);
        pBuffer->SetSize(pkt.static_size()+4);
        header = pBuffer->GetBuffer();
        packet = header + 4;
    }
    else
    {
        packet = new BYTE[pkt.static_size()];
    }

    UINT32 packetLen = 0;

    if(streamNumber >= 31)
    {
        pkt.stream_id = 31;
        pkt.stream_id_expansion = streamNumber;
    }
    else
    {
        pkt.stream_id = (UINT8)HX_SAFEUINT(streamNumber);
    }

    /* We'll always include the reliable flag for now*/
    pkt.need_reliable_flag = 1;

    if(pkt.need_reliable_flag)
    {
        pkt.total_reliable = pStreamData->m_reliableSeqNo;
    }

    pkt.seq_no = pStreamData->m_seqNo;
    pkt.timestamp = pStreamData->m_lastTimestamp;
    pkt.packet_sent = (UINT8)pStreamData->m_packetSent;
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
    pkt.packet_type = (UINT16)0xff06;
    pkt.pack(packet, packetLen);
    // writePacket now owns 'packet', no need to free it in here
    if (m_pFastSocket)
    {
        header[0] = '$';
        header[1] = (BYTE)m_tcpInterleave;
        putshort(&header[2], (UINT16)packetLen);
        pBuffer->SetSize(packetLen+4);
        hresult = m_pFastSocket->BufferedWrite(pBuffer);
        m_pFastSocket->FlushWrite();
        HX_RELEASE(pBuffer);
    }
    else
    {
        hresult = writePacket(packet, packetLen);
    }

    return hresult;
}

HX_RESULT
TNGTCPTransport::writePacket(IHXBuffer* pBuffer)
{
    HX_ASSERT(m_pFastSocket);
    HX_RESULT rc =
        m_pFastSocket->BufferedWrite(pBuffer);
    if(rc)
    {
        m_pResp->OnProtocolError(HXR_NET_SOCKET_INVALID);
    }

    return rc;
}

HX_RESULT
TNGTCPTransport::writePacket(BYTE* pData, UINT32 dataLen)
{
    HX_RESULT rc = HXR_OK;
    // need to put $\000[datalen] in front of packet data

    if(dataLen > 0xffff)
    {
        return HXR_FAIL;
    }

    BYTE* pPacketData = new BYTE[dataLen+4];

    pPacketData[0] = '$';
    pPacketData[1] = (BYTE)m_tcpInterleave;
    putshort(&pPacketData[2], (UINT16)dataLen);
    memcpy(&pPacketData[4], pData, HX_SAFESIZE_T(dataLen)); /* Flawfinder: ignore */
    IHXBuffer* pSendBuffer = NULL;
    if (HXR_OK == CreateAndSetBufferCCF(pSendBuffer, pPacketData, 
					dataLen + 4, m_pContext))
    {
	rc = m_pTCPSocket->Write(pSendBuffer);
	if (FAILED(rc))
	{
	    m_pResp->OnProtocolError(HXR_NET_SOCKET_INVALID);
	}
	HX_RELEASE(pSendBuffer);
    }
    delete[] pPacketData;
    delete[] pData;

    return rc;
}

HX_RESULT
TNGTCPTransport::handlePacket(IHXBuffer* pBuffer)
{
    HX_RESULT rc = HXR_OK;

    pBuffer->AddRef();
    UINT32 len = pBuffer->GetSize();
    UINT32 pos = 0;
    UINT16 packetType;
    UINT32 ulTimeStamp;

    //XXXPM All of this stuff looks just like TNGUDPTransport and
    //should be hoisted.
    /*
     *  See if this buffer was marked with a timestamp.  If not, use now.
     */
    IHXTimeStampedBuffer* pTBuff;
    if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer,
                                         (void **)&pTBuff))
    {
        ulTimeStamp = pTBuff->GetTimeStamp();
        pTBuff->Release();
    }
    else
    {
        ulTimeStamp = HX_GET_TICKCOUNT();
    }

    while(len && rc == HXR_OK)
    {
        packetType = (UINT16)getshort(pBuffer->GetBuffer() + pos + 1);
        if(packetType < 0xff00)
        {
            rc = handleDataPacket(pBuffer, &pos, &len, ulTimeStamp);
        }
        else
        {
            switch(packetType)
            {
            case 0xff06:
                rc = handleStreamEndPacket(pBuffer, &pos, &len);
                break;
            case 0xff08:
                rc = handleLatencyReportPacket(pBuffer, &pos, &len,
                                               ulTimeStamp);
                break;
            case 0xff09:
                DO_GENERATE_STAMP
                    rc = handleTransportInfoReqPacket(pBuffer, &pos,&len,
                                                      ulTimeStamp);
                break;
            case 0xff0a:
                rc = handleTransportInfoRespPacket(pBuffer, &pos, &len);
                break;
            case 0xff0b:
                rc = handleAutoBWDetectionPacket(pBuffer, &pos, &len);
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
TNGTCPTransport::handleDataPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                  UINT32* pLen, UINT32 ulTimeStamp)
{
    TNGDataPacket pkt;
    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;

    if((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        *pLen = 0;
        return HXR_UNEXPECTED;
    }

    if (pkt.length_included_flag)
    {
        /*
         * There was a length specified inside the TNGPacket.
         * We should only unpack this packet.
         */
        if((pOff = pkt.unpack(pData, pkt._packlenwhendone)) == 0)
        {
            *pLen = 0;
            return HXR_UNEXPECTED;
        }
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    UINT16 streamNumber = (UINT16)((pkt.stream_id == 31) ? pkt.stream_id_expansion:
        pkt.stream_id);
    UINT16 seqNo = pkt.seq_no;
    UINT16 totalReliable = 0;
    HX_RESULT hresult = HXR_OK;
    UINT16 asmRuleNumber = (UINT16)((pkt.asm_rule_number == 63) ?
        pkt.asm_rule_number_expansion : pkt.asm_rule_number);
    if(pkt.need_reliable_flag == 1)
    {
        totalReliable = pkt.total_reliable;
    }

    CHXPacket* pPacket = new CHXPacket;
    pPacket->AddRef();

    if (pkt.data.len)  /*if we actually have some payload */
    {
        UINT32 dataOffset = (UINT32)pkt.data.data -
            (UINT32)pBuffer->GetBuffer();

    	IHXBuffer* pPktBuffer = NULL;
    	hresult = CreateStaticBuffer(pBuffer,dataOffset,pkt.data.len,pPktBuffer);
    	HX_ASSERT(HXR_OK == hresult);
	if(FAILED(hresult))
    	{
	    HX_RELEASE(pPacket);
	    return hresult;
    	}
        // XXXSMP Transport flags over the wire?
        pPacket->Set(pPktBuffer, pkt.timestamp, streamNumber,
                     HX_ASM_SWITCH_ON, asmRuleNumber);

        HX_RELEASE(pPktBuffer);
    }
    else /* no payload, build a lost packet */
    {
        pPacket->Set(0, pkt.timestamp, streamNumber, 0, 0);
        pPacket->SetAsLost();
    }

    if (m_pBwMgrInput)
    {
        m_pBwMgrInput->ReportDataPacket
            (pkt.timestamp, ulTimeStamp, pkt.data.len);
    }


    /*
     * Record and Pull-Splitter do not use a TransportBuffer for TCP
     */

    if (m_bNoPacketBuffering && m_bPacketsStarted)
    {
        hresult = packetReady(HXR_OK, streamNumber, pPacket);
    }
    else if (!m_bNoPacketBuffering)
    {
        hresult = storePacket(pPacket,
                              streamNumber,
                              seqNo,
                              totalReliable,
                              pkt.is_reliable);
    }

    pPacket->Release();

    return hresult;
}

HX_RESULT
TNGTCPTransport::handleLatencyReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
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

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    /*
     * Tell the bandwidth manager about this.
     */
    if (m_pBwMgrInput)
    {
        m_pBwMgrInput->ReportLatency(pkt.server_out_time, ulTime);
    }
    return HXR_OK;
}

HX_RESULT
TNGTCPTransport::handleTransportInfoReqPacket(IHXBuffer* pBuffer, UINT32* pPos,
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
        (*pLen) -= (UINT32)(pOff - pData);
        (*pPos) += (UINT32)(pOff - pData);

        UINT32 streamCount = m_pStreamHandler->streamCount();
        UINT32 pktBufSize = RDTTransportInfoResponsePacket().static_size();

        if (transportInfoPkt.request_buffer_info)
        {
            pktBufSize += RDTBufferInfo().static_size() * streamCount;
        }

        BYTE* packet = new BYTE[pktBufSize];

        if (packet && m_pStreamHandler)
        {
            // Pack the response into the buffer
            BYTE* pStart = PackTransportInfoResp(transportInfoPkt,
                                                 ulTime,
                                                 m_pStreamHandler,
                                                 m_pSrcBufferStats,
                                                 packet);

            // writePacket now owns 'packet', no need to free it in here
            res = writePacket(packet, (UINT32)(pStart - packet));
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        *pLen = 0;
    }

    return res;
}

HX_RESULT
TNGTCPTransport::handleTransportInfoRespPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen)
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
        (*pLen) -= (UINT32)(pOff - pData);
        (*pPos) += (UINT32)(pOff - pData);

        res = HXR_OK;
    }
    else
    {
        *pLen = 0;
    }

    return res;
}

HX_RESULT
TNGTCPTransport::handleAutoBWDetectionPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen)
{
    TNGBWProbingPacket pkt;
    BYTE* pOff = NULL;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;
    UINT32 ulTS = 0;
    IHXTimeStampedBuffer* pTSBuffer = NULL;    

    if ((pOff = pkt.unpack(pData, *pLen)) != 0)
    {
        /*
         * Subtract out the length that we used for this packet.
         */
        (*pLen) -= (UINT32)(pOff - pData);
        (*pPos) += (UINT32)(pOff - pData);
    }
    else
    {
        *pLen = 0;
    }

    if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer, (void **)&pTSBuffer))
    {
        ulTS = pTSBuffer->GetTimeStamp();
        HX_RELEASE(pTSBuffer);
    }
    else
    {
        HX_ASSERT(FALSE);
    }

    return handleABDPktInfo(TCPMode, pkt.seq_no, pkt.timestamp, ulTS, pkt.length);
}

HX_RESULT
TNGTCPTransport::handleStreamEndPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                       UINT32* pLen)
{
    TNGStreamEndPacket pkt;
    UINT16 streamNumber;
    UINT16 seqNo;
    UINT16 reliableCount;
    HXBOOL   packetSent;
    UINT32 lastTimestamp;
    UINT32 ulEndReasonCode = 0;
    BYTE* pOff;
    BYTE* pData = pBuffer->GetBuffer() + *pPos;

    //If we haven't sent a PLAY request for this stream yet, ignore
    //this packet. The server has a problem where it will sometimes
    //sent a end of stream packet for a stream even though we have
    //not sent a play request yet.
    if (!HasPlayRequestBeenSent())
    {
#ifdef RDT_MESSAGE_DEBUG
        RDTmessageFormatDebugFileOut("TCP Rejecting an endofstream packet because no PLAY sent: %s\n",
                                     (const char*)m_sessionID);
#endif
        m_pResp->OnProtocolError(HXR_UNEXPECTED_STREAM_END);
        return HXR_UNEXPECTED;
    }

    if ((pOff = pkt.unpack(pData, *pLen)) == 0)
    {
        return HXR_UNEXPECTED;
    }

    /*
     * Subtract out the length that we used for this packet.
     */
    (*pLen) -= (UINT32)(pOff - pData);
    (*pPos) += (UINT32)(pOff - pData);

    if (m_bNoPacketBuffering)
    {
        return HXR_OK;
    }

    streamNumber = (UINT16)((pkt.stream_id == 31) ? pkt.stream_id_expansion:
        pkt.stream_id);
    seqNo = pkt.seq_no;
    reliableCount = (UINT16)((pkt.need_reliable_flag) ? pkt.total_reliable : 0);
    lastTimestamp = pkt.timestamp;
    packetSent = pkt.packet_sent;

    RTSPTransportBuffer* pTransportBuffer = getTransportBuffer(streamNumber);

    if (!pTransportBuffer)
    {
        return HXR_FAIL;
    }

#ifdef RDT_MESSAGE_DEBUG
    RDTmessageFormatDebugFileOut("TCP handling a SteamEndPacket: %s\n", (const char*)m_sessionID);
#endif

    if (pkt.ext_flag)
    {
        ulEndReasonCode = pkt.reason_code;
    }

    pTransportBuffer->SetEndPacket(seqNo,
                                   reliableCount,
                                   packetSent,
                                   lastTimestamp,
                                   ulEndReasonCode);

    return HXR_OK;
}

HX_RESULT
TNGTCPTransport::startPackets(UINT16 uStreamNumber)
{
    /*
     * The Pull-Splitter does not use a TransportBuffer for TCP
     */

    RTSPStreamData* pStreamData =
        m_pStreamHandler->getStreamData(uStreamNumber);

    if (pStreamData->m_pTransportBuffer)
    {
        delete pStreamData->m_pTransportBuffer;
        pStreamData->m_pTransportBuffer = 0;
    }

    m_bNoPacketBuffering = TRUE;
    m_bPacketsStarted = TRUE;

    return HXR_OK;
}

HX_RESULT
TNGTCPTransport::stopPackets(UINT16 uStreamNumber)
{
    m_bPacketsStarted = FALSE;

    return HXR_OK;
}

void
TNGTCPTransport::Done()
{
    destroyABDPktInfo();

    HX_RELEASE(m_pPacketFilter);
}

void
TNGTCPTransport::setTransportInterleave(INT8 interleave)
{
    /*
     * We only set the transport interleave for the record case
     */

    m_tcpInterleave = interleave;
}

HX_RESULT
TNGTCPTransport::streamDone(UINT16 streamNumber,
                            UINT32 uReasonCode /* = 0 */,
                            const char* pReasonText /* = NULL */)
{
    HX_RESULT hresult = HXR_OK;

    if (m_bIsSource)
    {
        hresult = sendStreamEndPacket(streamNumber, uReasonCode, pReasonText);
    }
    else
    {
        if (uReasonCode)
        {
            hresult = m_pResp->OnStreamDone(HXR_END_WITH_REASON, streamNumber);
        }
        else
        {
            hresult = m_pResp->OnStreamDone(HXR_OK, streamNumber);
        }
    }

    return hresult;
}

STDMETHODIMP
TNGTCPTransport::InitBw(IHXBandwidthManagerInput* pBwMgr)
{
    HX_RELEASE(m_pBwMgrInput);

    m_pBwMgrInput = pBwMgr;
    m_pBwMgrInput->AddRef();
    m_pBwMgrInput->SetTransportType(TNG_TCP);

    return HXR_OK;
}

STDMETHODIMP
TNGTCPTransport::SetTransmitRate(UINT32 ulBitRate)
{
    return HXR_OK;
}

void TNGTCPTransport::setFeatureLevel(UINT32 uFeatureLevel)
{
    m_uFeatureLevel = uFeatureLevel;
}
