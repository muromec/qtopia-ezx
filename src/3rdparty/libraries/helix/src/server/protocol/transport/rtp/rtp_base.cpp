/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: 
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

/****************************************************************************
 *  Defines
 */
#define ACCEPTABLE_SYNC_NOISE   3
#define STREAM_END_DELAY_RTP_TOLERANCE  500

/****************************************************************************
 *  Includes
 */
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
#include "hxtlogutil.h"
#include "hxqosinfo.h"
#include "hxqossig.h"
#include "hxqos.h"
//#include "hxcorgui.h"
#include "ihx3gpp.h"

#include "ntptime.h"

#include "rtspif.h"
#include "rtsptran.h"
#include "transport.h"
#include "rtp_base.h"

#include "bufnum.h"
#include "rtpwrap.h"    // Wrappers for PMC generated base classes
#include "basepkt.h"
#include "hxtbuf.h"
#include "transbuf.h"
#include "hxtick.h"
#include "random32.h"   // random32()
#include "pkthndlr.h"   // in rtpmisc for RTCP routine
#include "rtcputil.h"   // takes care of RTCP in RTP mode
#include "rtspmsg.h"
#include "hxprefs.h"    // IHXPreferences
#include "hxmime.h"
#include "hxcore.h"

#include "hxheap.h"
#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_STARTINFO_WAIT_TIME         20000           // in milliseconds
#define MIN_NUM_PACKETS_SCANNED_FOR_LIVE_START    5
#define MAX_NUM_PACKETS_GAPPED_FOR_LIVE_START     1

static const UINT32 NORMAL_ACK_INTERVAL = 1000;        // 1/sec
static const UINT32 MINIMUM_ACK_INTERVAL = 200;        // wait 200msecs

static const UINT32 NORMAL_REPORT_INTERVAL = 5000;      // 1 per 5secs

static const UINT32 TRANSPORT_BUF_GROWTH_RATE  = 1000;
static const UINT32 LATENCY_REPORT_INTERVAL_MS = 1000;

//For protocol overhead
//these represent normal/expected values. Actual values might vary.
const UINT32 IPV4_HEADER = 20;
const UINT32 IPV6_HEADER = 40;
const UINT32 TCP_HEADER  = 20;
const UINT32 UDP_HEADER  = 8;


/******************************************************************************
 *   RTP RTP RTP RTP RTP
 ******************************************************************************/

ServerRTPBaseTransport::ServerRTPBaseTransport(HXBOOL bIsSource)
    : Transport()
    , m_pSyncServer(NULL)
    , m_streamNumber(0)
    , m_rtpPayloadType(0xFF)
    , m_bHasMarkerRule(FALSE)
    , m_markerRuleNumber(0)
    , m_bIsSyncMaster(FALSE)
#ifdef RTP_MESSAGE_DEBUG
    , m_bMessageDebug(FALSE)
#endif  // RTP_MESSAGE_DEBUG
    , m_bHasRTCPRule(FALSE)
    , m_ulPayloadWirePacket(0)
    , m_RTCPRuleNumber(0)
    , m_lTimeOffsetRTP(0)
    , m_lSyncOffsetHX(0)
    , m_lSyncOffsetRTP(0)
    , m_lNTPtoHXOffset(0)
    , m_bNTPtoHXOffsetSet(FALSE)
    , m_bSSRCDetermined(FALSE)
    , m_ulSSRCDetermined(0)
    , m_ulAvgPktSz (0)
    , m_pReflectorInfo(NULL)
    , m_pReflectionHandler(NULL)
    , m_bSeqNoSet(FALSE)
    , m_bRTPTimeSet(FALSE)
    , m_pFirstPlayTime(NULL)
    , m_pLastPauseTime(NULL)
    , m_lRTPOffset(0)
    , m_pReportHandler(0)
    , m_pRTCPTran(0)
    , m_bIsLive(FALSE)
    , m_ulExtensionSupport(0)
    , m_bActive(TRUE)
    , m_bEmulatePVSession(FALSE)
    , m_pMBitHandler(NULL)
    , m_cLSRRead(0)
    , m_cLSRWrite(0)
    , m_pQoSInfo(NULL)
    , m_bDone(FALSE)
{
    m_wrapSequenceNumber = DEFAULT_WRAP_SEQ_NO;
}

ServerRTPBaseTransport::~ServerRTPBaseTransport()
{
    HXLOGL3(HXLOG_RTSP, "ServerRTPBaseTransport[%p]::~ServerRTPBaseTransport()",this);
    Done();
}

STDMETHODIMP
ServerRTPBaseTransport::QueryInterface(REFIID riid, void** ppvObj)
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

    return Transport::QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(UINT32)
ServerRTPBaseTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
ServerRTPBaseTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

void
ServerRTPBaseTransport::Done()
{
    m_bDone = TRUE;

    HXLOGL3(HXLOG_RTSP, "ServerRTPBaseTransport[%p]::Done()",this);
    HX_RELEASE(m_pQoSInfo);
    HX_RELEASE(m_pRTCPTran);
    HX_RELEASE(m_pPacketFilter);
    HX_RELEASE(m_pSyncServer);
    HX_DELETE(m_pFirstPlayTime);
    HX_DELETE(m_pLastPauseTime);
    HX_DELETE(m_pReflectorInfo);
    HX_DELETE(m_pReflectionHandler);
    // Note: do not delete m_pReportHandler here; ServerRTCPBaseTransport owns this.
}

HX_RESULT
ServerRTPBaseTransport::init()
{
    HXLOGL3(HXLOG_RTSP, "ServerRTPBaseTransport[%p]::init()",this);
    // m_pReportHandler will be freed in ServerRTCPBaseTransport::Done()...
    HX_ASSERT(!m_pReportHandler);
    m_pReportHandler =
        new ReportHandler(TRUE, FALSE, random32((int)HX_GET_TICKCOUNT()));
    HX_ASSERT(m_pReportHandler);
    if(!m_pReportHandler)
    {
        return HXR_OUTOFMEMORY;
    }

#ifdef RTP_MESSAGE_DEBUG
    IHXPreferences* pPreferences = NULL;

    if (m_pContext &&
        (HXR_OK == m_pContext->QueryInterface(IID_IHXPreferences,
                                              (void**) &pPreferences)))
    {
        IHXBuffer* pBuffer = NULL;

        if (HXR_OK == pPreferences->ReadPref("RTPMessageDebug", pBuffer))
        {
            m_bMessageDebug = atoi((const char*)pBuffer->GetBuffer()) ? TRUE : FALSE;
            HX_RELEASE(pBuffer);
            if (m_bMessageDebug)
            {
                if (HXR_OK == pPreferences->ReadPref("RTPMessageDebugFile", pBuffer))
                {
                    if (pBuffer->GetSize() <= 0)
                    {
                        // no file name, no log
                        m_bMessageDebug = FALSE;
                    }
                    else
                    {
                        m_messageDebugFileName = (const char*) pBuffer->GetBuffer();
                    }
                }
                HX_RELEASE(pBuffer);
            }
        }
    }

    HX_RELEASE(pPreferences);
#endif  // RTP_MESSAGE_DEBUG
    return HXR_OK;
}

void
ServerRTPBaseTransport::setLegacyRTPLive()
{
    if (isReflector())
    {
        HX_DELETE(m_pReflectorInfo); 
        m_pReflectorInfo = new ReflectorInfo();
    }
}

void
ServerRTPBaseTransport::addStreamInfo(RTSPStreamInfo* pStreamInfo, UINT32 ulBufferDepth)
{
    HXLOGL3(HXLOG_RTSP, "ServerRTPBaseTransport[%p]::addStreamInfo(): str %lu; buffer depth = %lu",this, pStreamInfo->m_streamNumber, ulBufferDepth);
    Transport::addStreamInfo(pStreamInfo, ulBufferDepth);

    // there better be only one stream
    m_streamNumber = pStreamInfo->m_streamNumber;

    // grab the SSRC, if it exists
    m_ulSSRCDetermined = pStreamInfo->m_ulSSRCFromSetup;

    // if pStreamInfo->m_rtpPayloadType is -1, it hasn't been set
    // by user, so just assign RTP_PAYLOAD_RTSP
    if (pStreamInfo->m_rtpPayloadType < 0)
    {
        m_rtpPayloadType = RTP_PAYLOAD_RTSP;
    }
    else
    {
        m_rtpPayloadType = (UINT8)pStreamInfo->m_rtpPayloadType;
    }

    if (pStreamInfo)
    {
        if (pStreamInfo->m_bHasMarkerRule)
        {
            m_bHasMarkerRule = pStreamInfo->m_bHasMarkerRule;
            m_markerRuleNumber = pStreamInfo->m_markerRule;
            // better be odd.
            HX_ASSERT(m_markerRuleNumber & 0x1);
        }

        m_bIsLive = pStreamInfo->m_bIsLive;
        m_ulExtensionSupport = (UINT32)pStreamInfo->m_bExtensionSupport;
        m_bActive = pStreamInfo->m_bActive;
        m_bIsSyncMaster = pStreamInfo->m_bIsSyncMaster;

        /*
         *  Reflection support
         */
        m_bHasRTCPRule = pStreamInfo->m_bHasRTCPRule;
        if (m_bHasRTCPRule)
        {
            m_RTCPRuleNumber = pStreamInfo->m_RTCPRule;
        }
        m_ulPayloadWirePacket = pStreamInfo->m_ulPayloadWirePacket;

        //XXX MN, addStreamInfo is called on RTCPtransport when it is created
        // in  RTSPServerProtocol::SetupTransportRTP
        //if (m_pRTCPTran)
        //{
        //  m_pRTCPTran->addStreamInfo(pStreamInfo, ulBufferDepth);
        //}

        if (m_bIsLive && m_ulPayloadWirePacket)
        {
            RTSPStreamData* pStreamData = NULL;
            pStreamData = m_pStreamHandler->getStreamData(pStreamInfo->m_streamNumber);
            m_pReflectionHandler = new CReflectionHandler(pStreamData->m_pTSConverter);
        }
    }
}

HX_RESULT
ServerRTPBaseTransport::getRTCPRule(REF(UINT16) unRTCPRule)
{
    if (!m_bHasRTCPRule)
    {
        return HXR_FAIL;
    }

    unRTCPRule = m_RTCPRuleNumber;
    return HXR_OK;
}

/*
 *  We need to set an initial SeqNo & timestamp for RTP
 */

HX_RESULT
ServerRTPBaseTransport::setFirstSeqNum(UINT16 uStreamNumber, UINT16 uSeqNum)
{
    HXLOGL3(HXLOG_RTSP, "ServerRTPBaseTransport[%p]::setFirstSeqNum(); str %u first seq = %u",this, uStreamNumber, uSeqNum);
    HX_RESULT theErr = HXR_UNEXPECTED;

    // On client we allow setting of sequence number only once not to cause
    // havoc in transport buffer
    if (!m_bSeqNoSet)
    {
        theErr = Transport::setFirstSeqNum(uStreamNumber, uSeqNum);

#ifdef RTP_MESSAGE_DEBUG
        messageFormatDebugFileOut("INIT: StartSeqNum=%u",
                                  uSeqNum);
#endif  // RTP_MESSAGE_DEBUG

        if (SUCCEEDED(theErr))
        {
            m_bSeqNoSet = TRUE;
        }

        if (m_pReflectorInfo)
        {
            HX_ASSERT(isReflector() && isRTP());
            RTSPStreamData* pStreamData =
                m_pStreamHandler->getStreamData(uStreamNumber);

           if (pStreamData)
     	    {
                m_pReflectorInfo->m_unSeqNoOffset =
                    m_pReflectorInfo->m_unNextSeqNo - pStreamData->m_firstSeqNo;

                // for RTP-Info
                pStreamData->m_firstSeqNo = m_pReflectorInfo->m_unNextSeqNo;
            }
        }
    }

    return theErr;
}


void
ServerRTPBaseTransport::SetFirstTSStatic(RTSPStreamData* pStreamData, UINT32 ulTS, HXBOOL bIsRaw)
{
    if (pStreamData->m_pTSConverter && !bIsRaw)
    {
	m_lRTPOffset = pStreamData->m_lastTimestamp - 
	    pStreamData->m_pTSConverter->hxa2rtp(ulTS);			    
    }
    else
    {
	m_lRTPOffset = pStreamData->m_lastTimestamp - ulTS;		
    }

    pStreamData->m_firstTimestamp =  pStreamData->m_lastTimestamp; 

    if (m_pFirstPlayTime && m_pLastPauseTime)
    {
	HX_ASSERT((m_pFirstPlayTime->tv_sec*1000+m_pFirstPlayTime->tv_usec/1000) >=
	    (m_pLastPauseTime->tv_sec*1000+m_pLastPauseTime->tv_usec/1000));

	UINT32 ulPauseDuration = (m_pFirstPlayTime->tv_sec*1000+m_pFirstPlayTime->tv_usec/1000) -
	    (m_pLastPauseTime->tv_sec*1000+m_pLastPauseTime->tv_usec/1000);

	pStreamData->m_firstTimestamp += ulPauseDuration;
	m_lRTPOffset += ulPauseDuration;
    }
}

void
ServerRTPBaseTransport::SetFirstTSLive(RTSPStreamData* pStreamData, UINT32 ulTS, HXBOOL bIsRaw)
{
    m_lRTPOffset = 0;
    if (pStreamData->m_pTSConverter && !bIsRaw)
    {
        pStreamData->m_firstTimestamp = pStreamData->m_lastTimestamp =
        pStreamData->m_pTSConverter->hxa2rtp(ulTS);
    }
    else
    {
        pStreamData->m_firstTimestamp = pStreamData->m_lastTimestamp = ulTS;
    }

    if (m_pReflectorInfo)
    {
        HX_ASSERT(isReflector() && isRTP());
        if (m_pReflectorInfo->m_bNeedTSOffset)
        {
            m_pReflectorInfo->m_bNeedTSOffset = FALSE;

            // exloiting the underflow.
            m_pReflectorInfo->m_ulRTPTSOffset = 0 - pStreamData->m_firstTimestamp;
        }

        // for RTP-Info
        pStreamData->m_firstTimestamp += m_pReflectorInfo->m_ulRTPTSOffset;
    }
}

void
ServerRTPBaseTransport::setFirstTimeStamp(UINT16 uStreamNumber, UINT32 ulTS,
                                    HXBOOL bIsRaw)
{
    HXLOGL3(HXLOG_RTSP, "ServerRTPBaseTransport[%p]::setFirstTimeStamp(); str %u first ts = %lu",this, uStreamNumber, ulTS);

    RTSPStreamData* pStreamData =
        m_pStreamHandler->getStreamData(uStreamNumber);

    if (pStreamData)
    {
        if (!m_bIsLive)
        {
            SetFirstTSStatic(pStreamData, ulTS, bIsRaw);
        }
        else
        {
            SetFirstTSLive(pStreamData, ulTS, bIsRaw);
        }

        m_bRTPTimeSet = TRUE;
    }
}

void
ServerRTPBaseTransport::OnPause(Timeval* pTv)
{

    if (!m_pLastPauseTime)
    {
        m_pLastPauseTime = new Timeval();
        if (!m_pLastPauseTime)
        {
            return;
        }
    }

    m_pLastPauseTime->tv_sec = pTv->tv_sec;
    m_pLastPauseTime->tv_usec = pTv->tv_usec;
}



HX_RESULT
ServerRTPBaseTransport::setFirstPlayTime(Timeval* pTv)
{
    if (!m_pFirstPlayTime)
    {
        m_pFirstPlayTime = new Timeval();
        if(!m_pFirstPlayTime)
        {
            return HXR_OUTOFMEMORY;
        }
    }

    m_pFirstPlayTime->tv_sec = pTv->tv_sec;
    m_pFirstPlayTime->tv_usec = pTv->tv_usec;
    return HXR_OK;
}

HX_RESULT
ServerRTPBaseTransport::reflectPacket(BasePacket* pBasePacket, REF(IHXBuffer*)pSendBuf)
{
    HX_ASSERT(pBasePacket);
    HX_ASSERT(m_bHasRTCPRule);
    HX_ASSERT(m_ulPayloadWirePacket==1);

    HX_RESULT theErr = HXR_OK;

    IHXPacket* pPacket = pBasePacket->GetPacket();
    IHXBuffer* pBuffer = NULL;
    UINT32      ulLen = 0;

    /*
     *  Sanity check
     */
    if (!pPacket)
    {
        return HXR_UNEXPECTED;
    }
    else if (pPacket->IsLost())
    {
        pPacket->Release();
        return HXR_IGNORE;
    }
    else
    {
        pBuffer = pPacket->GetBuffer();
        if (!pBuffer)
        {
            pPacket->Release();
            return HXR_UNEXPECTED;
        }
    }

    ulLen = pBuffer->GetSize();

    HX_ASSERT(pPacket->GetStreamNumber() == m_streamNumber);
    HX_ASSERT(pPacket->GetASMFlags());

    /*
     * RTP packet
     */
    UINT16 streamNumber = pPacket->GetStreamNumber();
    RTSPStreamData* pStreamData =
        m_pStreamHandler->getStreamData(streamNumber);

    if (isRTCPRule(pPacket->GetASMRuleNumber()))
    {
        theErr = HXR_OK;

        if (m_pReflectorInfo)
        {
            if (!pStreamData->m_bFirstPacket)
            {
                theErr = ServerFixRTCPSR(m_pCommonClassFactory,
                                   pBuffer,
                                   pSendBuf,
                                   m_pReflectorInfo->m_ulRTPTSOffset);
            }
            else
            {
                theErr = HXR_IGNORE;
            }
        }
        else
        {
        pSendBuf = pBuffer;
        pSendBuf->AddRef();
        }

        BYTE* pReport = pBuffer->GetBuffer();
        HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
        Timeval tvNow((INT32) rmatv.tv_sec, (INT32)rmatv.tv_usec);
        NTPTime ntpNow(tvNow);

        if ((pReport) && ((*(++pReport)) == 200))
        {
            pReport += 7;
            UINT32 ulSourceSec = GetDwordFromBufAndInc(pReport);
            UINT32 ulSourceFract = GetDwordFromBufAndInc(pReport);

            m_LSRHistory [m_cLSRWrite].m_ulSourceLSR = ulSourceSec  << 16;
            m_LSRHistory [m_cLSRWrite].m_ulSourceLSR |= (ulSourceFract >> 16);

            m_LSRHistory [m_cLSRWrite].m_ulServerLSR = ntpNow.m_ulSecond  << 16;
            m_LSRHistory [m_cLSRWrite].m_ulServerLSR |= (ntpNow.m_ulFraction >> 16);
            (++m_cLSRWrite) %= LSR_HIST_SZ;
        }

        if (HXR_OK == theErr)
        {
            theErr = m_pRTCPTran->reflectRTCP(pSendBuf);
            HX_RELEASE(pSendBuf);

            if (m_pReflectionHandler)
            {
                m_pReflectionHandler->OnRTCPPacket(pBuffer, tvNow);
            }
        }

        pPacket->Release();
        pBuffer->Release();

        if (HXR_OK == theErr)
        {
            return HXR_IGNORE;
        }
        else
        {
            return theErr;
        }
    }

    if (!pStreamData->m_packetSent)
    {
        pStreamData->m_packetSent = TRUE;
    }

    theErr = HXR_OK;

    if (m_pReflectorInfo)
    {
        pStreamData->m_bFirstPacket = FALSE;
        theErr = ServerFixRTPHeader(m_pCommonClassFactory,
                              pBuffer,
                              pSendBuf,
                              m_pReflectorInfo->m_unSeqNoOffset ,
                              m_pReflectorInfo->m_ulRTPTSOffset);
    }
    else
    {
    pSendBuf = pBuffer;
    pSendBuf->AddRef();
    }


    BYTE* pcOrig = pSendBuf->GetBuffer();
    UINT16 unSeqNo = 0;
    UINT32 ulRTPTS = 0;

    pcOrig += 2;
    unSeqNo = (UINT16)(*pcOrig++<<8);
    unSeqNo |= *pcOrig++;

    ulRTPTS = GetDwordFromBufAndInc(pcOrig);

    if (m_pReflectorInfo)
    {
        m_pReflectorInfo->m_unNextSeqNo = unSeqNo + 1;
    }

    // forever increasing
    pStreamData->m_seqNo = pBasePacket->m_uSequenceNumber;

    pStreamData->m_lastTimestamp = pPacket->GetTime();


    HX_ASSERT(pSendBuf);

    BYTE* pRawPkt = (BYTE*)pSendBuf->GetBuffer();
    UINT32 ulPayloadLen = ulLen;
    UINT32 ulRTPHeaderSize = 0;

    UINT8 uiCSRCCount = (UINT8)(pRawPkt[0] & 0x0F);

// We only want to count the payload, not the RTP headers.

    ulRTPHeaderSize += (4 * 3); // RTP fixed header size, not including CSRCs.
    ulRTPHeaderSize += (UINT32)(4 * uiCSRCCount); // CSRCs.

// Extension header present.
    if (pRawPkt[0] & 0x20)
    {
        HX_ASSERT(ulPayloadLen - ulRTPHeaderSize > 0);
        ulRTPHeaderSize += 2; // 16-bit profile-defined field

    // Overrun prevention.
        if (pSendBuf->GetSize() > ulRTPHeaderSize + 1)
        {
            // Extension length is last 16 bits of first word.
            UINT32 ulExtensionLength = (UINT32)((pRawPkt[ulRTPHeaderSize] << 8) + pRawPkt[ulRTPHeaderSize + 1]);
            ulRTPHeaderSize += 2; // 16-bit length field.
            ulRTPHeaderSize += (ulExtensionLength * 4); // Rest of extension header.
        }
    }


    ulPayloadLen -= ulRTPHeaderSize;

    updateQoSInfo(ulPayloadLen);

    if (m_pReflectionHandler)
    {
        m_pReflectionHandler->OnRTPPacket(ulPayloadLen, pStreamData->m_lastTimestamp, pStreamData->m_seqNo );
    }

    /*
     *  clean up
     */
    pPacket->Release();
    pBuffer->Release();
    return theErr;
}

void
ServerRTPBaseTransport::updateQoSInfo(UINT32 ulBytesSent)
{
    m_ulPacketsSent++;
    m_lBytesSent += ulBytesSent;

    if (!m_pQoSInfo)
    {
        return;
    }

    UINT64 ulSessionBytesSent = m_pQoSInfo->GetBytesSent();
    ulSessionBytesSent += ulBytesSent;
    m_pQoSInfo->SetBytesSent(ulSessionBytesSent);

    if (m_ulAvgPktSz != 0)
    {
        INT16 nDelta = 0;

        nDelta = (INT16)(ulBytesSent - 1 - (m_ulAvgPktSz >> RTP_FILTER_CONSTANT));

        if ((m_ulAvgPktSz += nDelta) <= 0)
        {
            m_ulAvgPktSz = 1;
        }
    }
    else
    {
        m_ulAvgPktSz = ulBytesSent << RTP_FILTER_CONSTANT;
    }


    UINT32 ulSessionPacketsSent = m_pQoSInfo->GetPacketsSent();
    ulSessionPacketsSent++;
    m_pQoSInfo->SetPacketsSent(ulSessionPacketsSent);
}

UINT32
ServerRTPBaseTransport::MapLSR(UINT32 ulSourceLSR)
{
    if (m_ulPayloadWirePacket == 0)
    {
        return ulSourceLSR;
    }

    UINT8  cSearchCursor = m_cLSRRead;

    while (cSearchCursor != m_cLSRWrite)
    {
        if (m_LSRHistory [cSearchCursor].m_ulSourceLSR == ulSourceLSR)
        {
            m_cLSRRead = cSearchCursor;
            return m_LSRHistory [cSearchCursor].m_ulServerLSR;
        }

        (++cSearchCursor) %= LSR_HIST_SZ;
    }

    return 0;
}

HX_RESULT
ServerFixRTPHeader(IHXCommonClassFactory* pCCF,
             IHXBuffer* pOrigBuf,
             REF(IHXBuffer*) pNewBuf,
             UINT16 unSeqNoOffset,
             UINT32 ulRTPTSOffset)
{
    if (pOrigBuf->GetSize() < 8)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RESULT theErr = pCCF->CreateInstance(IID_IHXBuffer, (void**) &pNewBuf);
    if (HXR_OK == theErr)
    {
        theErr = pNewBuf->Set(pOrigBuf->GetBuffer(), pOrigBuf->GetSize());
    }
    if (HXR_OK == theErr)
    {
        BYTE* pcOrig = pOrigBuf->GetBuffer();
        UINT16 unSeqNo = 0;
        UINT32 ulRTPTS = 0;

        pcOrig += 2;
        unSeqNo = (UINT16)(*pcOrig++<<8);
        unSeqNo |= *pcOrig++;

        ulRTPTS = GetDwordFromBufAndInc(pcOrig);

        // update
        unSeqNo += unSeqNoOffset;
        ulRTPTS += ulRTPTSOffset;

        BYTE* pc = pNewBuf->GetBuffer();
        pc += 2;
        *pc++ = (UINT8)(unSeqNo>>8);
        *pc++ = (UINT8)(unSeqNo);
        *pc++ = (UINT8)(ulRTPTS>>24);
        *pc++ = (UINT8)(ulRTPTS>>16);
        *pc++ = (UINT8)(ulRTPTS>>8);
        *pc++ = (UINT8)(ulRTPTS);
    }
    return theErr;
}

HX_RESULT
ServerFixRTCPSR(IHXCommonClassFactory* pCCF,
          IHXBuffer* pOrigBuf,
          REF(IHXBuffer*) pNewBuf,
          UINT32 ulRTPTSOffset)
{
    BYTE* pcOrig = pOrigBuf->GetBuffer();
    if (pOrigBuf->GetSize() < 20)
    {
        return HXR_INVALID_PARAMETER;
    }
    else
    {
        // make sure it's SR
        if (RTCP_SR != *(pcOrig+1))
        {
            return HXR_IGNORE;
        }
    }

    HX_RESULT theErr = pCCF->CreateInstance(IID_IHXBuffer, (void**) &pNewBuf);
    if (HXR_OK == theErr)
    {
        theErr = pNewBuf->Set(pOrigBuf->GetBuffer(), pOrigBuf->GetSize());
    }
    if (HXR_OK == theErr)
    {
        UINT32 ulRTPTS = 0;

        pcOrig += 16;
        ulRTPTS = GetDwordFromBufAndInc(pcOrig);

        // update
        ulRTPTS += ulRTPTSOffset;

        BYTE* pc = pNewBuf->GetBuffer();
        pc += 16;

        //RTP Timestamp
        *pc++ = (UINT8)(ulRTPTS>>24);
        *pc++ = (UINT8)(ulRTPTS>>16);
        *pc++ = (UINT8)(ulRTPTS>>8);
        *pc++ = (UINT8)(ulRTPTS);
    }
    return theErr;
}


void
ServerRTPBaseTransport::SyncTimestamp(IHXPacket* pPacket)
{

    IHXTimeStampSync* pTSSync = NULL;
    if (FAILED(
            m_pResp->QueryInterface(IID_IHXTimeStampSync, (void**)&pTSSync)))
    {
        // this shouldn't happen...
        HX_ASSERT(!"IHXTimeStampSync not implemented");
        return;
    }

    UINT32 ulInitialRefTime = 0;
    UINT32 ulInitialThisTime = pPacket->GetTime();

    if (pTSSync->NeedInitialTS(m_sessionID))
    {
        pTSSync->SetInitialTS(m_sessionID, pPacket->GetTime());
        ulInitialRefTime = ulInitialThisTime;
    }
    else
    {
        ulInitialRefTime = pTSSync->GetInitialTS(m_sessionID);
    }
    HX_RELEASE(pTSSync);

    RTSPStreamData* pStreamData =
        m_pStreamHandler->getStreamData(pPacket->GetStreamNumber());

    HX_ASSERT(pStreamData != NULL);
    if (pStreamData)
    {
        // calc the difference b/n reference stream
        if (ulInitialThisTime >= ulInitialRefTime)
        {
            // we want RTP time
            if (pStreamData->m_pTSConverter)
            {
                m_lTimeOffsetRTP =
                    (INT32)pStreamData->m_pTSConverter->hxa2rtp(ulInitialThisTime - ulInitialRefTime);
            }
            else
            {
                m_lTimeOffsetRTP = (INT32)(ulInitialThisTime - ulInitialRefTime);
            }
        }
        else
        {
            // we want RTP time
            if (pStreamData->m_pTSConverter)
            {
                m_lTimeOffsetRTP =
                    (INT32)pStreamData->m_pTSConverter->hxa2rtp(ulInitialRefTime - ulInitialThisTime);
            }
            else
            {
                m_lTimeOffsetRTP = (INT32)(ulInitialRefTime - ulInitialThisTime);
            }

            m_lTimeOffsetRTP *= -1;
        }
    }
}

// The pPacketBuf is returned with an AddRef(), as it must.

HX_RESULT
ServerRTPBaseTransport::makePacket(BasePacket* pBasePacket,
                             REF(IHXBuffer*) pPacketBuf)
{
    IHXPacket* pPacket = pBasePacket->GetPacket();

    if (!pPacket)
    {
        return HXR_UNEXPECTED;
    }
    else if (pPacket->IsLost())
    {
        pPacket->Release();
        return HXR_OK;
    }

    IHXBuffer* pBuffer = pPacket->GetBuffer();
    UINT32 bufLen = pBuffer->GetSize();
    UINT16 streamNumber = pPacket->GetStreamNumber();
    UINT16 ruleNumber = pPacket->GetASMRuleNumber();
    UINT8  ruleFlags = pPacket->GetASMFlags();

    // it better be the same
    HX_ASSERT(m_streamNumber == streamNumber);

    RTSPStreamData* pStreamData =
        m_pStreamHandler->getStreamData(streamNumber);

    //XXXBAB
    if (!pStreamData->m_packetSent)
    {
        pStreamData->m_packetSent = TRUE;
    }

    pStreamData->m_seqNo = pBasePacket->m_uSequenceNumber;

    /*
     *  Make RTP Packet
     */
    RTPPacket pkt;
    HX_RESULT hresult = HXR_OK;
    UINT32 packetLen = 0;
    pkt.version_flag = 2;
    pkt.padding_flag = 0;
    pkt.csrc_len = 0;

    /*
     *  Basics
     */
    pkt.seq_no = pStreamData->m_seqNo;
    pkt.data.data = (INT8*)pBuffer->GetBuffer();
    pkt.data.len = HX_SAFEUINT(pBuffer->GetSize());
    pkt.ssrc = m_pReportHandler->GetSSRC();
    pkt.extension_flag = 0;
    pkt.payload = m_rtpPayloadType;

    /*
     *  IHXRTPPacket support
     */
    if (pStreamData->m_bFirstPacket)
    {
        // figure out pkt type
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

        // figure out marker bit handling routine
        if (NULL == m_pMBitHandler)
        {
            IHXRTPPacketInfo* pRTPPacketInfo = NULL;
            if (pPacket->QueryInterface(IID_IHXRTPPacketInfo, (void**) &pRTPPacketInfo) == HXR_OK)
            {
                m_pMBitHandler = &ServerRTPBaseTransport::MBitRTPPktInfo;
                pRTPPacketInfo->Release();
            }
            else
            {
                m_pMBitHandler = &ServerRTPBaseTransport::MBitASMRuleNo;
            }
        }
    }

    /*
     * Marker Bit
     */
    (this->*m_pMBitHandler)(pkt.marker_flag, pPacket, ruleNumber);

    if (m_bRTPTimeSet)
    {
        SyncTimestamp(pPacket);
    }

    /*
     *  Timestamp
     */
    if (pStreamData->m_bUsesRTPPackets)
    {
        pkt.timestamp = ((IHXRTPPacket*) pPacket)->GetRTPTime();
    }
    else if (pStreamData->m_pTSConverter)
    {
        pkt.timestamp =
            pStreamData->m_pTSConverter->hxa2rtp(pPacket->GetTime());
    }
    else
    {
        pkt.timestamp = pPacket->GetTime();
    }

     pkt.timestamp += (UINT32)m_lRTPOffset;
    
    /*
     *  Extension and asm rule
     */
    if (RTP_OP_ASMRULES == m_ulExtensionSupport)
    {
        // this is the only one right now.
        pkt.extension_flag = 1;

        pkt.op_code = RTP_OP_ASMRULES;
        pkt.op_code_data_length = 1;
        pkt.asm_flags = ruleFlags;
        pkt.asm_rule = ruleNumber;
    }
    else
    {
        pkt.extension_flag = 0;
    }

    if (pStreamData->m_bFirstPacket)
    {
        m_pRTCPTran->startScheduler();
        m_pRTCPTran->m_bSendBye = TRUE;
        pStreamData->m_bFirstPacket = FALSE;

        // init report handler with starting NTP time and
        // 0 RTP time as the reference point.
        m_pReportHandler->Init(*m_pFirstPlayTime,
                               pStreamData->m_firstTimestamp,
                               pStreamData->m_pTSConverter);

        // at this point, it should have the same stream number
        HX_ASSERT(m_streamNumber == m_pRTCPTran->m_streamNumber);
        HX_ASSERT(m_bRTPTimeSet);
    }

    // externally, we need to offset the timestamp...
    //
    // XXXGo
    // In RTSP PLAY Response msg, there is a RTP-Info header in which there
    // is a rtp timestap that corresponds to NTP time spesified in PLAY Request.
    // Since PLAY Response goes out before we ever receive the first pkt, it
    // always returns RTP time equivalent of NTP time in a Range header as a timestamp.
    // So, we need to offset the timestamp here.
    // In future, we might want to change the calling sequence so we don't have to do this...
    if (m_bRTPTimeSet)
    {
        INT64 nNewRTPOffset = 0;
        UINT32 ulRawRTPTime = 0;
        HXTimeval hxNow = m_pScheduler->GetCurrentSchedulerTime();
        Timeval tvNow;
        NTPTime ntpNow;

        // Convert scheduler time to something we can use.
        tvNow.tv_sec = (LONG32)hxNow.tv_sec;
        tvNow.tv_usec = (LONG32)hxNow.tv_usec;

        ntpNow = NTPTime(tvNow);


        if (pStreamData->m_pTSConverter)
        {
            ulRawRTPTime = pStreamData->m_pTSConverter->hxa2rtp((UINT32)(ntpNow - m_pReportHandler->GetNTPBase()));
        }
        else
        {
            ulRawRTPTime = (UINT32)(ntpNow - m_pReportHandler->GetNTPBase());
        }

        nNewRTPOffset = CAST_TO_INT64 pkt.timestamp - CAST_TO_INT64 ulRawRTPTime;

        //m_pReportHandler->SetRTPBase(nNewRTPOffset);

        // if this is true, there was a Range header in a PLAY request
        m_bRTPTimeSet = FALSE;
    }

    pStreamData->m_lastTimestamp = pkt.timestamp;

    /*
     * Create enough space to account for the op code and
     * op code data if the extension bit is set
     */
    packetLen = pkt.static_size() + pBuffer->GetSize() +
        (pkt.extension_flag
         ? sizeof(UINT16) + (pkt.op_code_data_length * sizeof(UINT32))
         : 0);

    IHXBuffer* pPacketOut = NULL;
    m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                          (void**)&pPacketOut);
    if(pPacketOut)
    {
        pPacketOut->SetSize(packetLen);
        pkt.pack(pPacketOut->GetBuffer(), packetLen);
        pPacketOut->SetSize(packetLen);  //update with actual packed length

#ifdef DEBUG
        if (m_drop_packets && ++m_packets_since_last_drop % 10 == 0)
        {
            goto RTPsendContinue;
        }
#endif /* DEBUG */

        updateQoSInfo(bufLen);

        // out params...
        pPacketBuf = pPacketOut;

        /* update */
        m_pReportHandler->OnRTPSend(pkt.seq_no, 1, pBasePacket->GetSize(), pkt.timestamp);
    }
    else
    {
        hresult = HXR_OUTOFMEMORY;
    }

#ifdef DEBUG
  RTPsendContinue:
#endif

    pBuffer->Release();
    pPacket->Release();
    return hresult;
}

HX_RESULT
ServerRTPBaseTransport::handlePacket(IHXBuffer* pBuffer)
{
    if (m_bDone)
    {
        return HXR_UNEXPECTED;
    }

    m_pResp->PacketReady(HXR_OK, m_sessionID, NULL);

    if (!m_ulPacketsSent && m_bEmulatePVSession)
    {
        /* XXXMC
         * Special-case handling for PV clients
         */
        UINT8* pUDPPktPayload = pBuffer->GetBuffer();
        UINT8 ucRTPVersion  = (UINT8)((*pUDPPktPayload & 0xc0)>>6);

        if(ucRTPVersion != 2)
        {
            DPRINTF(D_INFO, ("RTP: PV CLIENT PKT RECVD\n"));
            this->sendPVHandshakeResponse(pUDPPktPayload);
            return HXR_OK;
        }
    }

    return _handlePacket(pBuffer, TRUE);
}

HX_RESULT
ServerRTPBaseTransport::_handlePacket(IHXBuffer* pBuffer, HXBOOL bIsRealTime)
{
    return HXR_OK;
}

HX_RESULT
ServerRTPBaseTransport::handleRTCPSync(NTPTime ntpTime, ULONG32 ulRTPTime)
{
    HX_RESULT retVal = HXR_IGNORE;

    // We use RTCP synchronization on live streams only.
    // Static streams have no reason not to be synchronzied in RTP time.
    // Making use of RTCP for static streams may result in unwanted sync.
    // noise/error for servers who do not generate proper RTCP ntp-rtp
    // mapping.  RealServers prior to RealServer9 had error in RTCP reported
    // ntp-rtp mapping (max. error 1s, avg 500ms).
    if (m_bIsLive && !m_bSkipTimeAdjustment)
    {
        ULONG32 ulNtpHX = ntpTime.toMSec();
        RTSPStreamData* pStreamData =
            m_pStreamHandler->getStreamData(m_streamNumber);

#ifdef RTP_MESSAGE_DEBUG
        messageFormatDebugFileOut("RTCP-SYNC: Received NTPTime=%u RTPTime=%u",
                                  ulNtpHX, ulRTPTime);
#endif  // RTP_MESSAGE_DEBUG

        // We ignore the RTCP sync until we can compute npt (m_bRTPTimeSet) or
        // if the RTCP packet contains no synchronization information
        // (ulNtpHX == 0)
        if (pStreamData && (ulNtpHX != 0) && m_bRTPTimeSet && pStreamData->m_pTSConverter )
        {
            // Npt time can be computed (ulHXTime)            
            ULONG32 ulHXTime = pStreamData->m_pTSConverter->rtp2hxa(ulRTPTime);

            retVal = HXR_OK;

            if ((!m_pSyncServer) && m_pResp)
            {
                m_pResp->QueryInterface(IID_IHXTransportSyncServer,
                                        (void**) &m_pSyncServer);
            }

            if (m_bNTPtoHXOffsetSet)
            {
                // We can sync - NTP to NPT offset is known
                ULONG32 ulExpectedHXTime = ulNtpHX + m_lNTPtoHXOffset;
                LONG32 lSyncOffsetHX = (LONG32)(ulExpectedHXTime - ulHXTime);
                LONG32 lSyncOffsetChange = lSyncOffsetHX - m_lSyncOffsetHX;

                if ((lSyncOffsetChange > ACCEPTABLE_SYNC_NOISE) ||
                    (lSyncOffsetChange < (-ACCEPTABLE_SYNC_NOISE)))
                {
                    if (m_bIsSyncMaster && m_pSyncServer)
                    {
#ifdef RTP_MESSAGE_DEBUG
                        messageFormatDebugFileOut("RTCP-SYNC: Distribute Master Sync NPTTime=%u SyncOffset=%d",
                                                  ulHXTime, -lSyncOffsetHX);
#endif  // RTP_MESSAGE_DEBUG
                        m_pSyncServer->DistributeSync(ulHXTime, -lSyncOffsetHX);
                    }
                    else
                    {
                        m_lSyncOffsetHX = lSyncOffsetHX;
                        if (lSyncOffsetHX >= 0)
                        {
                            m_lSyncOffsetRTP = (LONG32)
                                (pStreamData->m_pTSConverter->hxa2rtp_raw((ULONG32) lSyncOffsetHX));
                        }
                        else
                        {
                            m_lSyncOffsetRTP = -(LONG32)
                                (pStreamData->m_pTSConverter->hxa2rtp_raw((ULONG32) (-lSyncOffsetHX)));
                        }
#ifdef RTP_MESSAGE_DEBUG
                        messageFormatDebugFileOut("RTCP-SYNC: Self-Sync SyncOffset=%d SyncOffsetRTP=%d",
                                                  m_lSyncOffsetHX, m_lSyncOffsetRTP);
#endif  // RTP_MESSAGE_DEBUG
                    }
                }
            }
            else
            {
                // This the first RTCP sync accross all streams, anchor sync
                if (m_pSyncServer)
                {
#ifdef RTP_MESSAGE_DEBUG
                    messageFormatDebugFileOut("RTCP-SYNC: Distribute NTP-NPT Mapping NTPTime=%u NPTTime=%u",
                                              ulNtpHX, ulHXTime);
#endif  // RTP_MESSAGE_DEBUG
                    m_pSyncServer->DistributeSyncAnchor(ulHXTime, ulNtpHX);
                }
            }
        }
    }

    return retVal;
}

HX_RESULT
ServerRTPBaseTransport::streamDone(UINT16 streamNumber,
                             UINT32 uReasonCode /* = 0 */,
                             const char* pReasonText /* = NULL */)
{
    HXLOGL3(HXLOG_RTSP, "ServerRTPBaseTransport[%p]::streamDone(); str %u; reason %lu",this, streamNumber, uReasonCode);

    HX_ASSERT(m_streamNumber == streamNumber);
    HX_ASSERT(m_streamNumber == m_pRTCPTran->m_streamNumber);
    HX_RESULT hresult = HXR_OK;

    HX_ASSERT(m_pRTCPTran);

    if (m_bActive)
    {
        hresult = m_pRTCPTran->streamDone(streamNumber, uReasonCode, pReasonText);
    }

    return hresult;
}

/*
 * XXXMC
 * Special-case handling for PV clients
 */
void
ServerRTPBaseTransport::setPVEmulationMode(HXBOOL bPVSessionFlag)
{
    m_bEmulatePVSession = bPVSessionFlag;
}

void
ServerRTPBaseTransport::setRTCPTransport(ServerRTCPBaseTransport* pRTCPTran)
{
    HX_ASSERT(pRTCPTran);
    HX_ASSERT(!m_pRTCPTran);
    m_pRTCPTran = pRTCPTran;
    m_pRTCPTran->AddRef();

    // pointing to the same instatnce
    HX_ASSERT(m_pReportHandler);
    m_pRTCPTran->m_pReportHandler = m_pReportHandler;
}

void
ServerRTPBaseTransport::MBitRTPPktInfo(REF(UINT8)bMBit, IHXPacket* pPkt, UINT16 unRuleNo)
{
    HXBOOL b = FALSE;
    IHXRTPPacketInfo* pRTPPacketInfo = NULL;
    if (pPkt->QueryInterface(IID_IHXRTPPacketInfo, (void**) &pRTPPacketInfo) == HXR_OK)
    {
        if (pRTPPacketInfo->GetMarkerBit(b) == HXR_OK && b)
        {
            bMBit = TRUE;
        }
        else
        {
            bMBit = FALSE;
        }
        pRTPPacketInfo->Release();
    }
    else
    {
        bMBit = FALSE;
    }
}

void
ServerRTPBaseTransport::MBitASMRuleNo(REF(UINT8)bMBit, IHXPacket* pPkt, UINT16 unRuleNo)
{
    bMBit = m_bHasMarkerRule && ((unRuleNo & 0x1) == m_markerRuleNumber);
}

HX_RESULT
ServerRTPBaseTransport::getSSRC(UINT32& uSSRC)
{
    if (m_pReportHandler)
    {
        uSSRC = m_pReportHandler->GetSSRC();
        return HXR_OK;
    }

    return HXR_FAIL;
}

#ifdef RTP_MESSAGE_DEBUG
void
ServerRTPBaseTransport::messageFormatDebugFileOut(const char* fmt, ...)
{
    if(m_bMessageDebug)
    {
        va_list args;
        char buf[4096]; /* Flawfinder: ignore */

        SafeSprintf(buf, 4096, "%s.%d", (const char*) m_messageDebugFileName,
                    m_streamNumber);

        va_start(args, fmt);

        FILE* fp = fopen(buf, "a");
        if (fp)
        {
            vsprintf(buf, fmt, args);
            fprintf(fp, "%s\n", buf);
            fclose(fp);
        }

        va_end(args);
    }
}
#endif  // RTP_MESSAGE_DEBUG

/******************************************************************************
 *   RTCP RTCP RTCP RTCP RTCP
 ******************************************************************************/

ServerRTCPBaseTransport::ServerRTCPBaseTransport(HXBOOL bIsSource):
    Transport(),
    m_pDataTransport(NULL),
    m_streamNumber(0xffff),
    m_bSendBye(FALSE),
    m_bSendReport(FALSE),
    m_pReportCallback(0),
    m_bCallbackPending(FALSE),
    m_reportTimeoutID(0),
    m_bSchedulerStarted(FALSE),
    m_bSendRTCP(TRUE),
    m_ulProtocolOverhead(0),
    m_pcCNAME(NULL),
    m_pReportHandler(NULL),
    m_pTSConverter(NULL),
    m_pTSScheduler (NULL),
    m_pSignalBus(NULL),
    m_pQoSSignal_RR(NULL),
    m_pQoSSignal_APP(NULL),
    m_pQoSSignal_NADU(NULL),
    m_pSessionId(NULL),
    m_ulLastRR (HX_GET_BETTERTICKCOUNT()),
    m_ulLastSeq (0),
    m_ulLastLoss (0),
    m_ulLastRate (0),
    m_ulRRIntvl(0),
    m_pNADU(NULL),
    m_ulNADURRCount(0),
    m_bDone(FALSE)
{
}

ServerRTCPBaseTransport::~ServerRTCPBaseTransport()
{
    HXLOGL3(HXLOG_RTSP, "ServerRTCPBaseTransport[%p]::~ServerRTCPBaseTransport()",this);
    HX_DELETE(m_pTSConverter);
    HX_RELEASE(m_pTSScheduler);
}

STDMETHODIMP
ServerRTCPBaseTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSSignalSourceResponse))
    {
        AddRef();
        *ppvObj = (IHXQoSSignalSourceResponse*)this;
        return HXR_OK;
    }

    return Transport::QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(UINT32)
ServerRTCPBaseTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
ServerRTCPBaseTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

void
ServerRTCPBaseTransport::Done()
{
    m_bDone = TRUE;

    stopScheduler();
    HX_RELEASE(m_pPacketFilter);
    HX_VECTOR_DELETE(m_pcCNAME);
    HX_DELETE(m_pReportHandler);

    HX_RELEASE(m_pQoSSignal_RR);
    HX_RELEASE(m_pQoSSignal_APP);
    HX_RELEASE(m_pQoSSignal_NADU);
    HX_RELEASE(m_pSignalBus);
    HX_RELEASE(m_pSessionId);

    HX_RELEASE(m_pNADU);
}


void ServerRTCPBaseTransport::getProtocolOverhead(IHXSocket* pSocket)
{
    HX_ASSERT(pSocket);
    if (pSocket == NULL)
    {
        return;
    }

    if (pSocket->GetType() == HX_SOCK_TYPE_TCP)
    {
        m_ulProtocolOverhead = TCP_HEADER;  
    }
    else
    {
        m_ulProtocolOverhead = UDP_HEADER;  
    }

    if (pSocket->GetFamily() == HX_SOCK_FAMILY_IN6)
    {
        m_ulProtocolOverhead += IPV6_HEADER; 
    }
    else
    {
        m_ulProtocolOverhead += IPV4_HEADER; 
    } 
}


HX_RESULT
ServerRTCPBaseTransport::init()
{
    HX_ASSERT(!m_pReportCallback);
    HX_ASSERT(!m_pcCNAME);

    m_pReportCallback = new ReportCallback(this);
    if(!m_pReportCallback)
    {
        return HXR_OUTOFMEMORY;
    }
    m_pReportCallback->AddRef();

    char cname[16] = {0}; /* Flawfinder: ignore */
    itoa((int)random32((int)HX_GET_TICKCOUNT()), cname, 10);
    m_pcCNAME = (BYTE*)new_string(cname);
    HX_ASSERT(m_pcCNAME);

    HX_RELEASE(m_pNADU);
    if (m_pContext)
    {
        m_pContext->QueryInterface(IID_IHX3gppNADU,
                                   (void**)&m_pNADU);
        m_ulNADURRCount = 0;
    }

    return HXR_OK;
}

void
ServerRTCPBaseTransport::addStreamInfo (RTSPStreamInfo* pStreamInfo,
                                  UINT32 ulBufferDepth)
{
    UINT32 ulInvalidRate = (UINT32)-1;

    UINT32 ulAvgBitRate = pStreamInfo->m_ulAvgBitRate;
    UINT32 ulRRBitRate = pStreamInfo->m_ulRtpRRBitRate;
    UINT32 ulRSBitRate = pStreamInfo->m_ulRtpRSBitRate;
    HXBOOL   bUseRFC1889MinTime = FALSE;

    if (!ulAvgBitRate)
    {
        // We don't know the average bitrate.
        // Make something up
        ulAvgBitRate = 20000;
    }
    else
    {
        UINT32 ulRTCPBw = ulAvgBitRate / 20; // 5% of AvgBitRate

        if ((ulRRBitRate == ulInvalidRate) &&
            (ulRSBitRate != ulInvalidRate) &&
            (ulRTCPBw > ulRSBitRate))
        {
            ulRRBitRate = ulRTCPBw - ulRSBitRate;
        }
        else if ((ulRRBitRate != ulInvalidRate) &&
                 (ulRSBitRate == ulInvalidRate) &&
                 (ulRTCPBw > ulRRBitRate))
        {
            ulRSBitRate = ulRTCPBw - ulRRBitRate;
        }
    }

    if ((ulRRBitRate == ulInvalidRate) ||
        (ulRSBitRate == ulInvalidRate))
    {
        // If one of the bitrates is still
        // invalid at this point we just
        // default to the RFC 1889 behavior.
        // RS = 1.25% of the average bitrate
        // RR = 3.75% of the average bitrate

        bUseRFC1889MinTime = TRUE;
        m_bSendRTCP = TRUE;

        ulRSBitRate = ulAvgBitRate / 80; // 1.25%
        ulRRBitRate = ((ulAvgBitRate / 80) * 3 +
                       ((ulAvgBitRate % 80) * 3) / 80); // 3.75%
    }
    else if (ulRRBitRate == 0)
    {
        // We have been told not
        // to send RTCP reports
        m_bSendRTCP = FALSE;
    }

    if (m_pReportHandler)
    {
        // Get the minimum RTCP report interval
        UINT32 ulMinIntervalMs = (UINT32)((bUseRFC1889MinTime) ? 5000 : 1);

        // Pass the report interval parameters to
        // the report handler
        m_pReportHandler->SetRTCPIntervalParams(ulRSBitRate, ulRRBitRate,
                                                ulMinIntervalMs);
    }
}

void
ServerRTCPBaseTransport::setSessionID(const char* pSessionID)
{
    /* cache the session id for use in retrieving signal bus*/
    HX_RELEASE(m_pSessionId);
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

    Transport::setSessionID(pSessionID);
}

STDMETHODIMP
ServerRTCPBaseTransport::SignalBusReady (HX_RESULT hResult, IHXQoSSignalBus* pBus,
                                   IHXBuffer* pSessionId)
{
    if (FAILED(hResult))
    {
        HX_ASSERT(0);
        return HXR_OK;
    }

    HX_RELEASE(m_pSignalBus);
    m_pSignalBus = pBus;
    m_pSignalBus->AddRef();

    if (m_pDataTransport)
    {
        HX_RELEASE(m_pDataTransport->m_pQoSInfo);
        if (FAILED(m_pSignalBus->QueryInterface(IID_IHXQoSTransportAdaptationInfo,
                                                (void**)&m_pDataTransport->m_pQoSInfo)))
        {
            m_pDataTransport->m_pQoSInfo = NULL;
        }
    }
    else
    {
        HX_ASSERT(0);
    }

    HX_RELEASE(m_pQoSSignal_RR);
    if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXQoSSignal,
                                                        (void**)&m_pQoSSignal_RR)))
    {
        m_pQoSSignal_RR->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                     HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                     HX_QOS_SIGNAL_RTCP_RR));
    }
    else
    {
        HX_ASSERT(0);
        m_pQoSSignal_RR = NULL;
    }

    HX_RELEASE(m_pQoSSignal_APP);
    if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXQoSSignal,
                                                        (void**)&m_pQoSSignal_APP)))
    {
        m_pQoSSignal_APP->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                      HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                      HX_QOS_SIGNAL_COMMON_BUFSTATE));
    }
    else
    {
        HX_ASSERT(0);
        m_pQoSSignal_APP = NULL;
    }

    if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXQoSSignal,
                                                        (void**)&m_pQoSSignal_NADU)))
    {
        m_pQoSSignal_NADU->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                       HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                       HX_QOS_SIGNAL_RTCP_NADU));
    }
    else
    {
        HX_ASSERT(0);
        m_pQoSSignal_NADU = NULL;
    }

    return HXR_OK;
}

HX_RESULT
ServerRTCPBaseTransport::SetTSConverter(CHXTimestampConverter::ConversionFactors conversionFactors)
{
    HX_DELETE(m_pTSConverter);

    m_pTSConverter = new CHXTimestampConverter(conversionFactors);

    return m_pTSConverter ? HXR_OK : HXR_OUTOFMEMORY;
}

HX_RESULT
ServerRTCPBaseTransport::startScheduler()
{
    if (!m_pTSScheduler && m_pContext)
    {
        m_pContext->QueryInterface(IID_IHXThreadSafeScheduler, (void**)&m_pTSScheduler);
    }

    if(!m_bSchedulerStarted && m_bSendRTCP)
    {
        HX_ASSERT(!m_bCallbackPending);
        m_bSchedulerStarted = TRUE;
        if (!m_bMulticast)
        {
            // we wanna send the report right away!
            m_bSendReport = TRUE;
        }
        else
        {
            if (!m_bCallbackPending)
            {
                scheduleNextReport();
            }
        }
    }

    return HXR_OK;
}

HX_RESULT
ServerRTCPBaseTransport::stopScheduler()
{
    if(m_bCallbackPending)
    {
        HX_ASSERT(m_pScheduler);
        (m_pTSScheduler) ? m_pTSScheduler->Remove(m_reportTimeoutID) :
            m_pScheduler->Remove(m_reportTimeoutID);
        m_bCallbackPending = FALSE;
    }
    HX_RELEASE(m_pReportCallback);

    return HXR_OK;
}

void
ServerRTCPBaseTransport::scheduleNextReport()
{
    if (m_bSendRTCP)
    {
        HX_ASSERT(!m_bSendReport);
        HX_ASSERT(!m_bCallbackPending);
        HX_ASSERT(m_pReportCallback);

        HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
        Timeval tvNow((INT32)rmatv.tv_sec, (INT32)rmatv.tv_usec);

        tvNow += Timeval(m_pReportHandler->GetRTCPInterval());

        rmatv.tv_sec = (UINT32)tvNow.tv_sec;
        rmatv.tv_usec = (UINT32)tvNow.tv_usec;
        m_reportTimeoutID = (m_pTSScheduler) ?
            m_pTSScheduler->AbsoluteEnter(m_pReportCallback, rmatv) :
            m_pScheduler->AbsoluteEnter(m_pReportCallback, rmatv);
        m_bCallbackPending = TRUE;
    }
}

HX_RESULT
ServerRTCPBaseTransport::HandleNADUPacket(RTCPPacket* pNADUPacket)
{
    IHXBuffer* pTmp = NULL;
    UINT16  unNADUInfoCount = (pNADUPacket->length - 2)/3;
    if((m_pSignalBus) && SUCCEEDED(m_pCommonClassFactory->
                                           CreateInstance(CLSID_IHXBuffer,
                                                            (void**)&pTmp)))
    {
        pTmp->SetSize(sizeof(NADUSignalData));
        NADUSignalData* pNADUSigData =
                           (NADUSignalData*)pTmp->GetBuffer();

        for (int n = 0; n < unNADUInfoCount; ++n)
        {
           pNADUSigData->m_unStreamNumber = m_streamNumber;
           pNADUSigData->m_ulSSRC = pNADUPacket->m_pNADUInfo[n].m_ulSSRC;
           pNADUSigData->m_unPlayoutDelay = pNADUPacket->m_pNADUInfo[n].m_unPlayoutDelay;
           pNADUSigData->m_unHSNR = (UINT16)m_ulLastSeq;  //"Extended Highest Seq. No. received" from RR
           pNADUSigData->m_unNSN = pNADUPacket->m_pNADUInfo[n].m_unNSN;
           pNADUSigData->m_uNUN = pNADUPacket->m_pNADUInfo[n].m_uNUN;
           pNADUSigData->m_unFBS = pNADUPacket->m_pNADUInfo[n].m_unFBS;
           m_pQoSSignal_NADU->SetValue(pTmp);
           m_pSignalBus->Send(m_pQoSSignal_NADU);
        }
        HX_RELEASE(pTmp);
    }

    return HXR_OK;
}

/*
 *   we don't have a table of sender or receivers because we don't yet
 *   support multicast.  i.e. only one sender, one receiver
 */
HX_RESULT
ServerRTCPBaseTransport::handlePacket(IHXBuffer* pBuffer)
{
    if (m_bDone)
    {
        return HXR_UNEXPECTED;
    }

    // we need to deal with a compund packet
    RTCPUnPacker unpacker;

    if (HXR_OK != unpacker.UnPack(pBuffer))
    {
        // failed...don't do anything more...still ok to return HXR_OK;
        return HXR_OK;
    }
    RTCPPacket* pPkt = NULL;
	CHXSimpleList pktList;
    while (SUCCEEDED(unpacker.Get(pPkt)))
    {
		pktList.AddTail(pPkt);
    }

	return handlePacketList(&pktList, pBuffer);
}

HX_RESULT
ServerRTCPBaseTransport::handlePacketList(CHXSimpleList* pList, IHXBuffer* pBuffer)
{
    if (m_bDone)
    {
        return HXR_UNEXPECTED;
    }

    m_pResp->PacketReady(HXR_OK, m_sessionID, NULL);

    /* update */
    m_pReportHandler->UpdateAvgRTCPSize(pBuffer->GetSize()+ m_ulProtocolOverhead);

    HX_RESULT theErr = HXR_OK;
    RTCPPacket* pPkt = NULL;
    HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
    UINT32 ulNow = rmatv.tv_sec * 1000 + rmatv.tv_usec / 1000;

    // for EOS support
    HXBOOL bBye = FALSE;
    APPItem* pAppPkt = NULL;

	while (pList->IsEmpty() == FALSE)
    {
		pPkt = (RTCPPacket*)pList->RemoveHead();

        // deal with it!
        switch(pPkt->packet_type)
        {
            case RTCP_SR:
            {
                // client shouldn't send a SR, we're the sender!
                HX_ASSERT(FALSE);
            }
            break;

            case RTCP_RR:
            {
                DPRINTF(D_INFO, ("RTCP: ReceiverReport received\n"));
                m_pReportHandler->OnRTCPReceive(pPkt, ulNow);
                UINT32 ulInterval = ulNow - m_ulLastRR;

                if (m_ulRRIntvl != 0)
                {
                    INT16 nDelta = 0;
                    nDelta = (INT16)(ulInterval - 1 - (m_ulRRIntvl >> RTP_FILTER_CONSTANT));

                    if ((m_ulRRIntvl += nDelta) <= 0)
                    {
                        m_ulRRIntvl = 1;
                    }
                }
                else
                {
                    m_ulRRIntvl = ulInterval << RTP_FILTER_CONSTANT; 
                }

                m_ulLastRR = ulNow;

                if (m_pDataTransport && m_pDataTransport->m_pQoSInfo)
                {
                    m_pDataTransport->m_pQoSInfo->SetRRFrequency(m_ulRRIntvl >> RTP_FILTER_CONSTANT);
                }

                IHXBuffer* pTmp = NULL;
                if((m_pSignalBus) && SUCCEEDED(m_pCommonClassFactory->
                                                CreateInstance(CLSID_IHXBuffer,
                                                                (void**)&pTmp)))
                {
                    for (UINT32 i = 0; i < pPkt->count; i++)
                    {
                        // For RTP live we need to check the ReflectionHandler
                        if ((m_pDataTransport && m_pDataTransport->m_ulPayloadWirePacket && 
                            m_pDataTransport->m_pReflectionHandler &&
                            pPkt->rr_data[i].ssrc == m_pDataTransport->m_pReflectionHandler->GetSSRC()) ||
                            (pPkt->rr_data[i].ssrc == m_pReportHandler->GetSSRC()))
                        {
                            pTmp->SetSize(sizeof(ReceptionReport));
                            ReceptionReport* pRR = (ReceptionReport*)pTmp->GetBuffer();

                            HX_ASSERT(pRR);

                            //replace the ssrc with the server specific stream number
                            pRR->ssrc     = m_streamNumber;
                            //copy in the remaining receiver report data
                            pRR->fraction = pPkt->rr_data[i].fraction;
                            pRR->lost     = pPkt->rr_data[i].lost;
                            pRR->last_seq = pPkt->rr_data[i].last_seq;
                            pRR->lsr      = (m_pDataTransport) ?
                                m_pDataTransport->MapLSR(pPkt->rr_data[i].lsr) : 0;
                            pRR->dlsr     = pPkt->rr_data[i].dlsr;

                            if (m_pDataTransport && m_pDataTransport->m_pQoSInfo)
                            {
                                /* record loss statistic: */
                                UINT32 ulCurrentLoss =
                                    m_pDataTransport->m_pQoSInfo->GetPacketLoss();

                                UINT32 ulNewLoss =
                                    (ulCurrentLoss &&
                                    (ulCurrentLoss >= m_ulLastLoss)) ?
                                    ((ulCurrentLoss - m_ulLastLoss) + pRR->lost) :
                                    pRR->lost;

                                m_pDataTransport->m_pQoSInfo->
                                    SetPacketLoss(ulNewLoss);

                                /* get an RTT estimate sec 6.3.1 of RFC 1889 */
                                if (pRR->lsr != 0)
                                {
                                    UINT32 ulRTT      = 0;
                                    UINT32 ulNowTrunc = 0;

                                    Timeval tvArrivalTime((INT32) rmatv.tv_sec,
                                                            (INT32)rmatv.tv_usec);
                                    NTPTime ntpNow = NTPTime (tvArrivalTime);
                                    ulNowTrunc     = ntpNow.m_ulSecond  << 16;
                                    ulNowTrunc    |= (ntpNow.m_ulFraction >> 16);

                                    ulRTT = (UINT32)abs((INT32)
                                                (ulNowTrunc - pRR->lsr - pRR->dlsr));
                                    NTPTime ntpRTT = NTPTime(((ulRTT & 0xffff0000) >> 16),
                                                            ((ulRTT & 0xffff) << 16));

                                    ulRTT = (ntpRTT.m_ulSecond * 1000);
                                    ulRTT += (UINT32)
                                        (((double) ntpRTT.m_ulFraction /
                                            (double) MAX_UINT32) * 1000.0);

                                    m_pDataTransport->m_pQoSInfo->SetRTT(ulRTT);
                                }

                                /* compute the estimated received throughput */
                                if (pRR->last_seq != 0)
                                {
                                    UINT32 ulSz =
                                        m_pDataTransport->m_ulAvgPktSz >> RTP_FILTER_CONSTANT;
                                    UINT32 ulPPS = (ulInterval) ?
                                        (((pRR->last_seq - m_ulLastSeq - (pRR->lost - m_ulLastLoss)) * 1000)/
                                        (ulInterval)) :
                                        ((pRR->last_seq - m_ulLastSeq - (pRR->lost - m_ulLastLoss)) * 1000);


                                    UINT32 ulRate = ulPPS*ulSz;

                                    UINT32 ulCurrentRate =
                                        m_pDataTransport->m_pQoSInfo->GetReceivedThroughput();

                                    UINT32 ulNewRate =
                                        (ulCurrentRate &&
                                        (ulCurrentRate >= m_ulLastRate)) ?
                                        ((ulCurrentRate - m_ulLastRate) + ulRate) :
                                        ulRate;

                                    m_pDataTransport->m_pQoSInfo->
                                        SetReceivedThroughput(ulNewRate);

                                    m_ulLastRate = ulRate;
                                }

                                m_ulLastLoss = (pRR->lost > 0) ? pRR->lost : 0;
                                m_ulLastSeq = pRR->last_seq;
                            }

                            m_pQoSSignal_RR->SetValue(pTmp);
                            m_pSignalBus->Send(m_pQoSSignal_RR);
                        }
                    }
                }
                HX_RELEASE(pTmp);
            }
            break;

            case RTCP_SDES:
            {
                DPRINTF(D_INFO, ("RTCP: SDESReport received\n"));
                m_pReportHandler->OnRTCPReceive(pPkt, ulNow);
            }
            break;

            case RTCP_BYE:
            {
                DPRINTF(D_INFO, ("RTCP: BYE received\n"));
                m_pReportHandler->OnRTCPReceive(pPkt, ulNow);

                bBye = TRUE;
            }
            break;

            case RTCP_APP:
            {
                DPRINTF(D_INFO, ("RTCP: APP received\n"));

                //NADU packets have name field set to "PSS0"
                //and subtype = 0
                if ((0 == strncmp((const char*)pPkt->app_name, "PSS0", 4))
                    && pPkt->count == 0)
                {
                    if (!(pPkt->m_pNADUInfo))
                    {
                        break;
                    }

                    HandleNADUPacket(pPkt);
                }
                else
                {
                    // make sure this APP is from RS.
                    // Hmmm...This is a security risk...Anyone can send APP pkt
                    // with "RNWK"...
                    if ((0 != strncmp((const char*)pPkt->app_name, "RNWK", 4)) &&
                        (0 != strncmp((const char*)pPkt->app_name, "HELX", 4)))
                    {
                        // unknown APP, ignore it.
                        break;
                    }

                    if (!(pPkt->m_pAPPItems))
                    {
                        break;
                    }

                    HX_ASSERT(1 == pPkt->count);
                    pAppPkt = new APPItem();
                    if(!pAppPkt)
                    {
                        theErr = HXR_OUTOFMEMORY;
                        break;
                    }
                    
                    if ((pPkt->m_pAPPItems[0]).app_type == APP_BUFINFO)
                    {
                        IHXBuffer* pTmp = NULL;
                        if((m_pSignalBus) && SUCCEEDED(m_pCommonClassFactory->
                                                        CreateInstance(CLSID_IHXBuffer,
                                                                        (void**)&pTmp)))
                        {
                            //pTmp->Set((UCHAR*)(&pPkt->m_pAPPItems[0]), sizeof(APPItem));
                            pTmp->SetSize(sizeof(BufferMetricsSignal));
                            BufferMetricsSignal* pbufSig =
                                (BufferMetricsSignal*)pTmp->GetBuffer();
                                
                            // The correct mapping from SSRC to stream number
                            // relies on the port multiplexing used by the
                            // current implementation. If we use SSRC-based
                            // multiplexing, we will need to take a look
                            // again at this mapping.
                            pbufSig->m_ulStreamNumber = m_streamNumber;
                            pbufSig->m_ulLowTimestamp = pPkt->m_pAPPItems[0].lowest_timestamp;
                            pbufSig->m_ulHighTimestamp = pPkt->m_pAPPItems[0].highest_timestamp;
                            pbufSig->m_ulBytes = pPkt->m_pAPPItems[0].bytes_buffered;
                            m_pQoSSignal_APP->SetValue(pTmp);
                            m_pSignalBus->Send(m_pQoSSignal_APP);
                            HX_RELEASE(pTmp);
                        }
                    }
                    else
                    {
                        memcpy(pAppPkt, &pPkt->m_pAPPItems[0], sizeof(APPItem)); /* Flawfinder: ignore */
                    }
                }
            }
            break;

            default:
            {
                DPRINTF(D_INFO, ("RTCP: Don't know this pkt type\n"));
            }
        }

        // Im responsible of freeing the pkt
        HX_DELETE(pPkt);
    }

    HX_DELETE(pAppPkt);
    return theErr;
}

/*
 *
 *
 */
HX_RESULT
ServerRTCPBaseTransport::makeBye(REF(IHXBuffer*) pSendBuf)
{
    HX_ASSERT(m_pDataTransport->m_streamNumber == m_streamNumber);
    HX_ASSERT(m_bSendBye);

    // consider it sent...
    m_bSendBye = FALSE;

    HX_RESULT   theErr = HXR_FAIL;
    RTCPPacket* pPktSDES = NULL;
    RTCPPacket* pPktBYE  = NULL;
    RTCPPacket* pPktAPP  = NULL;

    HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
    Timeval tvNow((INT32) rmatv.tv_sec, (INT32)rmatv.tv_usec);

    RTCPPacker  packer;

    RTCPPacket* pPktR = new RTCPPacket();
    if(!pPktR)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }

    if (m_pDataTransport->m_bIsLive && m_pDataTransport->m_pReflectionHandler)
    {
        theErr = m_pDataTransport->m_pReflectionHandler->MakeSR(pPktR, tvNow);
    }
    else
    {
        theErr = m_pReportHandler->MakeSR(pPktR, tvNow);
    }

    if (HXR_OK != theErr)
    {
        // no SR/RR, no RTCP
        goto bail;
    }

    pPktSDES = new RTCPPacket();
    if(!pPktSDES)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }

    if (m_pDataTransport->m_bIsLive && m_pDataTransport->m_pReflectionHandler)
    {
        theErr = m_pDataTransport->m_pReflectionHandler->MakeSDES(pPktSDES);
    }
    else
    {
        theErr = m_pReportHandler->MakeSDES(pPktSDES, m_pcCNAME);
    }

    if (HXR_OK != theErr)
    {
        goto bail;
    }

    pPktBYE = new RTCPPacket();
    if(!pPktBYE)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }

    if (m_pDataTransport->m_bIsLive && m_pDataTransport->m_pReflectionHandler)
    {
        theErr = m_pDataTransport->m_pReflectionHandler->MakeBye(pPktBYE);
    }
    else
    {
        theErr = m_pReportHandler->MakeBye(pPktBYE);
    }

    if (HXR_OK != theErr)
    {
        goto bail;
    }

    pPktAPP = new RTCPPacket();
    if(!pPktAPP)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }
    
    if (m_pDataTransport->m_bIsLive && m_pDataTransport->m_pReflectionHandler)
    {
            theErr = m_pDataTransport->m_pReflectionHandler->MakeEOSApp(pPktAPP);
    }
    else
    {
            theErr = m_pReportHandler->MakeEOSApp(pPktAPP);
    }

    if (HXR_OK != theErr)
    {
        goto bail;
    }
    // pack them up!
    theErr = m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                                   (void**)&pSendBuf);
    if (HXR_OK != theErr)
    {
        HX_ASSERT(!pSendBuf);
        goto bail;
    }

    packer.Set(pPktR);
    packer.Set(pPktSDES);
    packer.Set(pPktBYE);
    packer.Set(pPktAPP);
    theErr = packer.Pack(pSendBuf);

    if (HXR_OK != theErr)
    {
        HX_ASSERT(FALSE && "failed to create Report/BYE RTCP pkt");
        goto bail;
    }
  bail:
    HX_DELETE(pPktR);
    HX_DELETE(pPktSDES);
    HX_DELETE(pPktBYE);
    HX_DELETE(pPktAPP);
    return theErr;
}

HX_RESULT
ServerRTCPBaseTransport::makeSenderReport(REF(IHXBuffer*) pSendBuf)
{
    // create SR
    // create SDES
    // create compound RTCP
    // send
    // no reception report on the server

    // if no pkt has been sent, wait!
    HX_ASSERT(m_pDataTransport->m_streamNumber == m_streamNumber);
    RTSPStreamData* pStreamData =
        m_pDataTransport->m_pStreamHandler->getStreamData(m_pDataTransport->m_streamNumber);

    if(!pStreamData || !pStreamData->m_packetSent)
    {
        return HXR_FAIL;
    }

    HX_ASSERT(pStreamData->m_streamNumber == m_streamNumber);

    HX_RESULT theErr = HXR_FAIL;
    RTCPPacket* pPktSDES = NULL;

    HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
    Timeval tvNow((INT32) rmatv.tv_sec, (INT32)rmatv.tv_usec);

    RTCPPacker packer;

    RTCPPacket* pPktSR = new RTCPPacket();
    if(!pPktSR)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }

    theErr = m_pReportHandler->MakeSR(pPktSR, tvNow);
    if (HXR_OK != theErr)
    {
        goto bail;
    }

    pPktSDES = new RTCPPacket();
    if(!pPktSDES)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }
    theErr = m_pReportHandler->MakeSDES(pPktSDES, m_pcCNAME);
    if (HXR_OK != theErr)
    {
        goto bail;
    }

    // pack them up!
    theErr = m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                                   (void**)&pSendBuf);
    if (HXR_OK != theErr)
    {
        HX_ASSERT(!pSendBuf);
        goto bail;
    }

    packer.Set(pPktSR);
    packer.Set(pPktSDES);
    theErr = packer.Pack(pSendBuf);

    if (HXR_OK != theErr)
    {
        HX_ASSERT(FALSE && "failed to create SR/SDES RTCP pkt");
        goto bail;
    }

//    theErr = m_pSocket->WriteTo(m_foreignAddr, m_foreignPort, pSendBuf);
    m_pReportHandler->UpdateAvgRTCPSize(pSendBuf->GetSize()+ m_ulProtocolOverhead);
    m_bSendBye = TRUE;
  bail:
    HX_DELETE(pPktSR);
    HX_DELETE(pPktSDES);
    return theErr;
}

HX_RESULT
ServerRTCPBaseTransport::makeReceiverReport(REF(IHXBuffer*) pSendBuf)
{
    // create RR
    // create SDES
    // create BufferInfo
    // create compund RTCP
    // send

    HX_ASSERT(m_pDataTransport->m_streamNumber == m_streamNumber);

    HX_RESULT theErr = HXR_FAIL;
    RTCPPacket* pPktSDES = NULL;
    RTCPPacket* pPktBufInfo = NULL;
    RTCPPacket* pPktNADU = NULL;

    HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
    UINT32 ulNow = rmatv.tv_sec*1000 + rmatv.tv_usec/1000;

    INT64 llLowTS = 0;
    INT64 llHighTS = 0;
    UINT32 ulBytesBuffered = 0;
    HXBOOL bDone = FALSE;
    HXBOOL bNeedBufInfoPkt = TRUE; // Assume we need to send BufInfo pkts

    RTCPPacker packer;

    RTCPPacket* pPktRR = new RTCPPacket();

    if(!pPktRR)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }

    theErr = m_pReportHandler->MakeRR(pPktRR, ulNow);
    if (HXR_OK != theErr)
    {
        goto bail;
    }

    pPktSDES = new RTCPPacket();
    if(!pPktSDES)
    {
        theErr = HXR_OUTOFMEMORY;
        goto bail;
    }
    theErr = m_pReportHandler->MakeSDES(pPktSDES, m_pcCNAME);
    if (HXR_OK != theErr)
    {
        goto bail;
    }

    if (m_pNADU && m_pReportHandler)
    {
        UINT32 uFreq = 0;

        // increment RR counter
        m_ulNADURRCount++;

        // Get NADU report frequency
        m_pNADU->GetNADUFrequency(m_streamNumber, uFreq);

        if (uFreq)
        {
            // We are sending NADU packets so
            // we don't beed to generate BufInfo APP
            // packets.
            bNeedBufInfoPkt = FALSE;

            if (m_ulNADURRCount >= uFreq)
            {
                // Time to send an NADU packet
                
                UINT16 uPlayoutDelay;
                UINT16 uNextSeqNumber;
                UINT16 uNextUnitNumber;
                UINT16 uFreeBufferSpace;
                
                pPktNADU = new RTCPPacket();
                
                HX_RESULT tmpRes = m_pNADU->GetNADUInfo(m_streamNumber,
                                                        uPlayoutDelay,
                                                        uNextSeqNumber,
                                                        uNextUnitNumber,
                                                        uFreeBufferSpace);

                HXBOOL bNextSeqValid = (HXR_OK == tmpRes) ? TRUE : FALSE;

                if ((bNextSeqValid || (HXR_NO_DATA == tmpRes)) &&
                    (HXR_OK == m_pReportHandler->MakeNADU(pPktNADU,
                                                          bNextSeqValid,
                                                          uPlayoutDelay,
                                                          uNextSeqNumber,
                                                          uNextUnitNumber,
                                                          uFreeBufferSpace)))
                {
                    // We successfully created an NADU
                    // packet.
                    
                    // Reset our RR counter to 0
                    m_ulNADURRCount = 0;
                }
                else
                {
                    // If we failed for some reason,
                    // just delete the packet so that
                    // it doesn't get included in the
                    // compound packet. Failing to
                    // send an NADU is not a
                    // critical error
                    HX_DELETE(pPktNADU);
                }
            }
        }
    }
    
    if (bNeedBufInfoPkt)
    {
        pPktBufInfo = new RTCPPacket();
        
        // Get buffer info from m_pSrcBufferStats
        
        if (!m_pSrcBufferStats || !m_pReportHandler ||
            HXR_OK != m_pSrcBufferStats->GetTotalBuffering(m_streamNumber,
                                                           llLowTS, llHighTS,
                                                           ulBytesBuffered,
                                                           bDone) ||
            HXR_OK != m_pReportHandler->MakeBufInfoApp(pPktBufInfo,
                                                       INT64_TO_UINT32(llLowTS),
                                                       INT64_TO_UINT32(llHighTS),
                                                       ulBytesBuffered))
        {
            // If we failed for some reason,
            // just delete the packet so that
            // it doesn't get included in the
            // compound packet. Failing to
            // report buffer info is not a
            // critical error
            HX_DELETE(pPktBufInfo);
        }
    }


    // pack them up!
    theErr = m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                                   (void**)&pSendBuf);
    if (HXR_OK != theErr)
    {
        HX_ASSERT(!pSendBuf);
        goto bail;
    }

    packer.Set(pPktRR);
    packer.Set(pPktSDES);

    if (pPktBufInfo)
    {
        packer.Set(pPktBufInfo);
    }

    if (pPktNADU)
    {
        packer.Set(pPktNADU);
    }

    theErr = packer.Pack(pSendBuf);

    if (HXR_OK != theErr)
    {
        HX_ASSERT(FALSE && "failed to create SR/SDES RTCP pkt");
        goto bail;
    }

//    theErr = m_pSocket->WriteTo(m_foreignAddr, m_foreignPort, pSendBuf);
    m_pReportHandler->UpdateAvgRTCPSize(pSendBuf->GetSize()+ m_ulProtocolOverhead);
    m_bSendBye = TRUE;

  bail:
    HX_DELETE(pPktRR);
    HX_DELETE(pPktSDES);
    HX_DELETE(pPktBufInfo);
    HX_DELETE(pPktNADU);
    return theErr;
}

ServerRTCPBaseTransport::ReportCallback::ReportCallback(ServerRTCPBaseTransport* pTransport):
    m_pTransport(pTransport),
    m_lReportRefCount(0)
{
    if(m_pTransport)
    {
        m_pTransport->AddRef();
    }
}

ServerRTCPBaseTransport::ReportCallback::~ReportCallback()
{
    HX_RELEASE(m_pTransport);
}

STDMETHODIMP
ServerRTCPBaseTransport::ReportCallback::QueryInterface(REFIID riid, void** ppvObj)
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

STDMETHODIMP_(UINT32)
    ServerRTCPBaseTransport::ReportCallback::AddRef()
{
    return InterlockedIncrement(&m_lReportRefCount);
}

STDMETHODIMP_(UINT32)
    ServerRTCPBaseTransport::ReportCallback::Release()
{
    if(InterlockedDecrement(&m_lReportRefCount) > 0)
    {
        return (UINT32)m_lReportRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
ServerRTCPBaseTransport::ReportCallback::Func()
{
    HX_ASSERT(!m_pTransport->m_bSendReport);
    HX_ASSERT(m_pTransport->m_bCallbackPending);
    m_pTransport->m_bCallbackPending = FALSE;

    if (m_pTransport->m_bSendRTCP)
    {
        m_pTransport->m_bSendReport = TRUE;
    }

    return HXR_OK;
}

HX_RESULT
ServerRTCPBaseTransport::getSSRC(UINT32& uSSRC)
{
    if (m_pReportHandler)
    {
        uSSRC = m_pReportHandler->GetSSRC();
        return HXR_OK;
    }

    return HXR_FAIL;
}
