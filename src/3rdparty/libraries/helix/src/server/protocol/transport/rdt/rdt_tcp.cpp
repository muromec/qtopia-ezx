/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rdt_tcp.cpp,v 1.15 2008/03/27 23:08:50 dcollins Exp $
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
#include "rdt_base.h"
#include "rdt_tcp.h"
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

#include "servbuffer.h"
#include "qos_signal.h"
#include "abdbuf.h" //random data for ABD packets.


#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

/*
 * RDTTCPTransport methods
 */

RDTTCPTransport::RDTTCPTransport(BOOL bIsSource /* UNUSED */,
                BOOL bNoLostPackets /* = FALSE */,
                UINT32 uFeatureLevel /* = 0 */)
    : RDTBaseTransport(bIsSource, uFeatureLevel)
    , m_bNoPacketBuffering(FALSE)
    , m_bPacketsStarted(FALSE)
    , m_bNoLostPackets(bNoLostPackets)
    , m_bBlocked (FALSE)
    , m_ulPktsPerBufferRequest (RDT_DEFAULT_PKTS_PER_BUF_REQ)
    , m_ulPktCount (0)
    , m_pWouldBlockResponse (NULL)
{
    m_lTransportType = RTSP_TR_TNG_TCP;
    m_unBlockQueueSz            = MAX_BLOCK_QUEUE_SZ;
    m_unBlockQueueWrite         = 0;
    m_unBlockQueueRead          = 0;
    m_pBlockQueue               = new INT16 [m_unBlockQueueSz];
    m_pBlockMarker              = new BOOL [m_unBlockQueueSz];

    memset (m_pBlockQueue, -1, sizeof(INT16) * m_unBlockQueueSz);
    memset (m_pBlockMarker, 0, sizeof(BOOL) * m_unBlockQueueSz);
}

RDTTCPTransport::~RDTTCPTransport()
{
    HX_VECTOR_DELETE(m_pBlockQueue);
    HX_VECTOR_DELETE(m_pBlockMarker);
}

STDMETHODIMP
RDTTCPTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXWouldBlock))
    {
        AddRef();
        *ppvObj = (IHXWouldBlock*)this;
        return HXR_OK;
    }
    else
    {
        return RDTBaseTransport::QueryInterface (riid, ppvObj);
    }
}

STDMETHODIMP_(UINT32)
RDTTCPTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RDTTCPTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

HX_RESULT
RDTTCPTransport::init(IUnknown* pContext, IHXSocket* pSocket,
    INT8 interleave, IHXRTSPTransportResponse* pResp)
{
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(pSocket, 11);
#endif
    m_tcpInterleave = interleave;

    return RDTBaseTransport::init(pContext, pSocket, pResp);
}

/* IHXSocketResponse */
STDMETHODIMP
RDTTCPTransport::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_RESULT hRes = HXR_OK;

    switch (uEvent)
    {
    case HX_SOCK_EVENT_WRITE:
        if (!m_bBlocked)
        {
            hRes = HXR_OK;
            break;
        }

        m_bBlocked = FALSE;

        if (m_pWouldBlockResponse)
        {
            m_pWouldBlockResponse->WouldBlockCleared(0);
        }
        else
        {
            //HX_ASSERT (m_pBlockQueue [m_unBlockQueueRead] >= 0); //disabling noisy assert, tracking with PR 186027
            HX_ASSERT (m_pBlockMarker [m_pBlockQueue [m_unBlockQueueRead]]);

            while ((m_pBlockQueue [m_unBlockQueueRead] >= 0) && !m_bBlocked)
            {
                UINT16 unStream = (UINT16)m_pBlockQueue [m_unBlockQueueRead];
                m_pBlockMarker [unStream] = FALSE;

                m_pBlockQueue [m_unBlockQueueRead] = -1;
                (++m_unBlockQueueRead) %= m_unBlockQueueSz;

                if (m_pSource)
                {
                    m_pSource->SinkBlockCleared (unStream);
                }
            }
        }
        hRes = (m_bBlocked) ? HXR_BLOCKED : HXR_OK;
        break;

    case HX_SOCK_EVENT_CLOSE:
        m_pResp->OnProtocolError(HXR_NET_SOCKET_INVALID);
        break;

    default:
        break;
    }

    return hRes;
}

/* IHXWouldBlock */
STDMETHODIMP
RDTTCPTransport::WantWouldBlock (IHXWouldBlockResponse* pResp, UINT32 lId)
{
    if (!pResp)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pWouldBlockResponse);

    m_pWouldBlockResponse = pResp;
    m_pWouldBlockResponse->AddRef();

    return HXR_OK;
}

HX_RESULT
RDTTCPTransport::sendPackets(BasePacket** pPacket)
{
    return sendPacketsImpl(pPacket);
}

HX_RESULT
RDTTCPTransport::sendPacket(BasePacket* pBasePacket)
{
    return sendPacketImpl(pBasePacket);
}

HX_RESULT
RDTTCPTransport::sendPackets(ServerPacket** pPacket)
{
    return sendPacketsImpl(pPacket);
}

HX_RESULT
RDTTCPTransport::sendPacket(ServerPacket* pBasePacket)
{
    return sendPacketImpl(pBasePacket);
}

template<typename PacketT>
HX_RESULT
RDTTCPTransport::sendPacketsImpl(PacketT** pPacket)
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

template<typename PacketT>
HX_RESULT
RDTTCPTransport::sendPacketImpl(PacketT* pBasePacket)
{
    // XXXJJ. This is neccesary defensive code here.  
    // For example, if an error happens in RTSPServerProtocol, the protocol object
    // will call RDTTCPTransport::Done, which set m_pSocket to NULL. But the closing message
    // won't reach PPM::Session immediately, instead it goes throught protocol->client->player::session
    // ->PPM::Session, using 2-3 callbacks.  In the meantime PPM::Session will call sendPacket as usual,
    // and cause an CA because m_pSocket is NULL. 
    if(m_bDone)
    {
        return HXR_OK;
    }

    HX_ASSERT (pBasePacket);

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

    if (!pStreamData)
    {
        return HXR_UNEXPECTED;
    }
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

    if (!pStreamData->m_packetSent)
    {
        pStreamData->m_packetSent = TRUE;
    }
    pStreamData->m_seqNo = pBasePacket->m_uSequenceNumber;

    TNGDataPacket pkt;
    UINT32 offset = 0;
    UINT32 report_len = 0;
    HX_RESULT hresult = HXR_OK;

    ServerBuffer* pSendBuffer = new ServerBuffer(TRUE);

    TNGLatencyReportPacket rpt;
    RDTTransportInfoRequestPacket req;

    UINT32 ulReportPacketSize = (createLatencyReportPacket(&rpt)) ?
        rpt.static_size() : 0;

    UINT32 ulRequestPacketSize = (createInfoRequestPacket(&req)) ?
        req.static_size() : 0;

    if (!SUCCEEDED(pSendBuffer->SetSize(ulReportPacketSize
                    + ulRequestPacketSize + pkt.static_size() + 4)))
    {
        return HXR_OUTOFMEMORY;
    }

    UINT8* pBuf = (UINT8*)pSendBuffer->GetBuffer();
    // include seperator here
    pBuf[0] = '$';
    pBuf[1] = m_tcpInterleave;

    pBuf += 4; //Space for TCP Interleave delimiter

    if (ulReportPacketSize != 0)
    {
        rpt.pack(pBuf, offset);
    }

    pBuf += offset;
    offset = 0;

    if (ulRequestPacketSize)
    {
        req.pack(pBuf, offset);
    }

    pBuf += offset;
    report_len = pBuf - ((UINT8*)pSendBuffer->GetBuffer());

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

    /* We'll always include the reliable flag for now*/
    pkt.need_reliable_flag = 1;
    pkt.is_reliable = pBasePacket->m_uPriority == 10 ? 1: 0;
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

    pkt.extension_flag = 0;
    if (m_bUseExtPkts)
    {
        pkt.extension_flag = 1;
        pkt.version = RDTV4;
        pkt.reserved = 0;
        pkt.rsid = m_ulRDTSessionID;
    }

    UINT16 ulRuleNumber = pPacket->GetASMRuleNumber();
    pkt.asm_rule_number = (ulRuleNumber > 63) ? 63: ulRuleNumber;


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

    pkt.pack(pBuf, packetLen);
   //fprintf(stderr, "Generating packet %d %d %d %d\n", pSendBuffer->GetSize(), packetLen, pkt.data.len, report_offset);
    pBuf = pSendBuffer->GetBuffer();
    packetLen += report_len - 4;
    putshort(pBuf+2, (UINT16)packetLen);
    packetLen -= bufLen;
    if (!SUCCEEDED(pSendBuffer->SetSize(packetLen + 4)))
    {
        return HXR_OUTOFMEMORY;
    }

    IHXBuffer* pWriteVec [2];
    pWriteVec [0] = pSendBuffer;
    pWriteVec [1] = pBuffer;

    hresult = m_pSocket->WriteV(2, pWriteVec);
    if (FAILED(hresult) || (hresult == HXR_SOCK_BUFFERED))
    {
        m_bBlocked = TRUE;

        if (m_pWouldBlockResponse)
        {
            m_pWouldBlockResponse->WouldBlock(0);
        }

        hresult = HXR_BLOCKED;
    }

    updateQoSInfo(bufLen);

    /*
     *  When sending out, the total size of the physical packet is
     *  to combination of the sizes of the logical packets.  In this
     *  case, the BasePacket size and report packet size.
     */

    HX_RELEASE(pSendBuffer);
    HX_RELEASE(pBuffer);
    HX_RELEASE(pPacket);

    return hresult;
}

STDMETHODIMP
RDTTCPTransport::SignalBusReady (HX_RESULT hResult, IHXQoSSignalBus* pBus,
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

    /* Configure congestion control: */
    HX_VERIFY(SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSProfileConfigurator,
                                                     (void**)&pConfig)));

    if (FAILED(m_pSignalBus->QueryInterface(IID_IHXQoSTransportAdaptationInfo,
                                            (void**)&m_pQoSInfo)))
    {
        m_pQoSInfo = NULL;
    }

    lTemp = 0;
    if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_BUFF, lTemp)))
    {
        m_ulPktsPerBufferRequest = (UINT32)lTemp;
    }
    else
    {
        m_ulPktsPerBufferRequest = RDT_DEFAULT_PKTS_PER_BUF_REQ;
    }

    HX_RELEASE(pConfig);
    return HXR_OK;
}

BOOL
RDTTCPTransport::createInfoRequestPacket(RDTTransportInfoRequestPacket* pReq)
{
    if (m_uFeatureLevel < 3)
    {
        return FALSE;
    }

    m_ulPktCount++;
    BOOL bReqBuf = (m_ulPktCount % m_ulPktsPerBufferRequest) ? FALSE : TRUE;

    if ((!bReqBuf))
    {
        return FALSE;
    }

    pReq->packet_type = 0xff09;
    pReq->request_rtt_info = FALSE;
    pReq->request_buffer_info = bReqBuf;

    return TRUE;
}

HX_RESULT
RDTTCPTransport::sendBWProbingPackets(UINT32 ulCount, UINT32 ulSize, REF(INT32) lSeqNo)
{
    HX_ASSERT (ulCount >= MIN_ABD_PACKETS && ulCount <= MAX_ABD_PACKETS);
    HX_ASSERT (ulSize >= MIN_ABD_PACKET_SIZE && ulSize <= MAX_ABD_PACKET_SIZE);

    HX_RESULT hresult = HXR_FAIL;

    // need to send them right away 
    m_pSocket->SetOption(HX_SOCKOPT_TCP_NODELAY, 1);
    // keep going even if SetOption failed...

    TNGBWProbingPacket pkt;
    CHXStaticBuffer* pBuffer = NULL;
    IHXBuffer* pSendBuf = NULL;
    BYTE* pc = NULL;
    UINT32 ulTemp = 0;

    UINT16 ulPacketSize, ulPayloadSize, ulPacketCount;

    ulPacketCount = ulCount;
    ulPacketSize = ulSize;
    ulPayloadSize = ulPacketSize - (UINT16)pkt.static_size();
    lSeqNo = -1;

    pBuffer = new CHXStaticBuffer((UCHAR*)g_pABDBuf, (ulPacketSize + ulPacketCount));
    if (NULL == pBuffer)
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

        hresult = m_pCommonClassFactory->CreateInstance(
                    CLSID_IHXBuffer, (void**)&pSendBuf);
        if (FAILED(hresult))
        {
            break;
        }

        pSendBuf->SetSize(ulPacketSize + 4);
        BYTE* pPacketData = (BYTE*)pSendBuf->GetBuffer();

        pPacketData[0] = '$';
        pPacketData[1] = m_tcpInterleave;
        putshort(&pPacketData[2], (UINT16)ulPacketSize);
        memcpy(&pPacketData[4], pc, HX_SAFESIZE_T(ulPacketSize));

        pc++; //offset one byte each time so they're not exactly the same


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

    m_pSocket->SetOption(HX_SOCKOPT_TCP_NODELAY, 0);
    return hresult;
}

HX_RESULT
RDTTCPTransport::writePacket(IHXBuffer* pBuffer)
{
    // need to put $\000[datalen] in front of packet data
    if (pBuffer == NULL)
    {
        return HXR_FAIL;
    }
    
    BYTE* pData = pBuffer->GetBuffer();
    UINT32 dataLen = pBuffer->GetSize();

    HX_ASSERT(pData);

    if(dataLen > 0xffff || dataLen == 0)
    {
        pBuffer->Release();
        return HXR_FAIL;
    }

    UINT32 ulPacketLen = dataLen + 4;
    ServerBuffer* pSendBuffer = NULL;

    if((FAILED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pSendBuffer))) || !pSendBuffer)
    {
        return HXR_OUTOFMEMORY;
    }
    if (!SUCCEEDED(pSendBuffer->SetSize(ulPacketLen)))
    {
        return HXR_OUTOFMEMORY;
    }
    BYTE* pPacketData = pSendBuffer->GetBuffer();

    pPacketData[0] = '$';
    pPacketData[1] = m_tcpInterleave;
    putshort(&pPacketData[2], (UINT16)dataLen);
    memcpy(&pPacketData[4], pData, HX_SAFESIZE_T(dataLen));

    HX_RESULT rc =
        m_pSocket->Write(pSendBuffer);
    if(FAILED(rc))
    {
        m_pResp->OnProtocolError(HXR_NET_SOCKET_INVALID);
    }
    pSendBuffer->Release();
    pBuffer->Release();

    return rc;
}

HX_RESULT
RDTTCPTransport::handlePacket(IHXBuffer* pBuffer)
{
    HX_RESULT rc = HXR_OK;

    pBuffer->AddRef();
    UINT32 len = pBuffer->GetSize();
    UINT32 pos = 0;
    UINT16 packetType;
    UINT32 ulTimeStamp;

    //XXXPM All of this stuff looks just like RDTUDPTransport and
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
            // data packets only go from S->C
            HX_ASSERT(FALSE);
            rc = HXR_UNEXPECTED;
        }
        else
        {
            switch(packetType)
            {
                case 0xff06:
                    // stream end packets are S->C only.
                    HX_ASSERT(FALSE);
                    rc = HXR_UNEXPECTED;
                    break;
                case 0xff08:
                    rc = handleLatencyReportPacket(pBuffer, &pos, &len,
                        ulTimeStamp);
                    break;
                case 0xff09:
                    if (!ulTimeStamp) ulTimeStamp = GenerateTimeStamp(pBuffer);
                    rc = handleTransportInfoReqPacket(pBuffer, &pos,&len, ulTimeStamp);
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
RDTTCPTransport::startPackets(UINT16 uStreamNumber)
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
RDTTCPTransport::stopPackets(UINT16 uStreamNumber)
{
    m_bPacketsStarted = FALSE;

    return HXR_OK;
}

void
RDTTCPTransport::Done()
{
    if (m_bDone)
        return;

    m_bDone = TRUE;

    HX_RELEASE(m_pWouldBlockResponse);

    RDTBaseTransport::Done();
}

void
RDTTCPTransport::setTransportInterleave(INT8 interleave)
{
    /*
     * We only set the transport interleave for the record case
     */

    m_tcpInterleave = interleave;
}

HX_RESULT
RDTTCPTransport::streamDone(UINT16 streamNumber,
        UINT32 uReasonCode /* = 0 */, const char* pReasonText /* = NULL */)
{
    HX_RESULT hresult = HXR_OK;

    hresult = sendStreamEndPacket(streamNumber, uReasonCode, pReasonText);

    return hresult;
}

STDMETHODIMP
RDTTCPTransport::SetSource(IHXServerPacketSource* pSource)
{
    if (!pSource)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pSource);
    m_pSource = pSource;
    pSource->AddRef();
    m_pSource->EnableTCPMode();

    HX_ASSERT(m_pSocket);

    IHXWouldBlock* pWouldBlock = NULL;
    if (m_pSocket && SUCCEEDED(m_pSocket->QueryInterface(IID_IHXWouldBlock,
                                                               (void**)&pWouldBlock)))
    {
        HX_VERIFY(SUCCEEDED(pWouldBlock->WantWouldBlock((IHXWouldBlockResponse*)this, 0)));
    }
    HX_RELEASE(pWouldBlock);

    return HXR_OK;
}

STDMETHODIMP
RDTTCPTransport::PacketReady(ServerPacket* pPacket)
{
    if (!pPacket)
    {
        HX_ASSERT(0);
        return HXR_OK;
    }

    if (m_bDone)
    {
        return HXR_OK;
    }

    UpdatePacketStats(pPacket);

    if (m_bBlocked)
    {
        pPacket->m_bTransportBlocked = TRUE;

        /* Enter this packet's stream number in the blocked queue */
        UINT16 unStream = pPacket->GetStreamNumber ();
        HX_ASSERT(unStream < MAX_BLOCK_QUEUE_SZ);

        if (!m_pBlockMarker [unStream])
        {
            m_pBlockMarker [unStream] = TRUE;

            m_pBlockQueue [m_unBlockQueueWrite] = unStream;
            (++m_unBlockQueueWrite) %= m_unBlockQueueSz;
        }
        return HXR_BLOCKED;
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
RDTTCPTransport::SourceDone()
{
    if (m_bAttemptingBackToBack)
    {
        SendPacketQueue();
    }

    HX_RELEASE(m_pSource);
    return HXR_OK;
}

STDMETHODIMP
RDTTCPTransport::SetDeliveryBandwidth(UINT32 /* UNUSED */, UINT32 ulBandwidth)
{
    SetBandwidthAndLatency (ulBandwidth);

    return HXR_OK;
}

STDMETHODIMP
RDTTCPTransport::SetAggregationLimits (UINT32* pAggregStat,
                                       UINT32 ulAggregateTo, UINT32 ulAggregateHighest)
{
    m_pAggregStat = pAggregStat;
    m_ulAggregateTo = ulAggregateTo;
    m_ulAggregateHighest = ulAggregateHighest;
    return HXR_OK;
}
