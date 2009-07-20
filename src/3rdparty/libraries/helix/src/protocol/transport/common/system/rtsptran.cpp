/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtsptran.cpp,v 1.53 2007/01/11 21:19:42 milko Exp $
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

#include "hxtypes.h"
#include "hxassert.h"
#include "debug.h"
#include "hxcom.h"
#include "hxmarsh.h"
#include "hxstrutl.h"
#include "netbyte.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "pckunpck.h"
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
#include "rtspmsg.h"
#include "hxcorgui.h"
#include "hxtlogutil.h"

#include "ntptime.h"

#include "rtspif.h"
#include "rtsptran.h"
//#include "rtpwrap.h"	// Wrappers for PMC generated base classes
#include "basepkt.h"
#include "hxtbuf.h"
#include "transbuf.h"
#include "hxtick.h"
#include "random32.h"	// random32()
#include "pkthndlr.h"	// in rtpmisc for RTCP routine
#include "rtcputil.h"	// takes care of RTCP in RTP mode
#include "hxprefs.h"	// IHXPreferences
#include "hxmime.h"
#include "hxcore.h"

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


#define STREAM_END_DELAY_BASE_TOLERANCE	3000

static const UINT32 TRANSPORT_BUF_GROWTH_RATE  = 1000;

void
dump(const char* pFile, const char* pc)
{
    FILE* fp = fopen(pFile, "a");
    if(!fp)
    {
	    return;
    }
    fprintf(fp, "%s", pc);
    fclose(fp);    
}


RTSPStreamData::RTSPStreamData(HXBOOL needReliable):
    m_seqNo(0),
    m_firstSeqNo(0),
    m_reliableSeqNo(0),
    m_bNeedReliable(needReliable),
    m_packetSent(FALSE),
    m_streamNumber(0),
    m_lastTimestamp(0),
    m_firstTimestamp(0),
    m_pTransportBuffer(0),
    m_pResendBuffer(0),
    m_pStreamStats(0),
    m_bReceivedAllPackets(FALSE),
    m_bNeedToACK(FALSE),
    m_bFirstPacket(TRUE),
    m_bUsesRTPPackets(FALSE),
    m_eMediaType(RTSPMEDIA_TYPE_UNKNOWN),
    m_pTSConverter(NULL),
    m_pTSOrderHack(NULL)
{
    ;
}

RTSPStreamData::~RTSPStreamData()
{
    if (m_pTransportBuffer)
    {
	delete m_pTransportBuffer;
    }

    if (m_pResendBuffer)
    {
	delete m_pResendBuffer;
    }

    HX_DELETE(m_pTSConverter);
    HX_DELETE(m_pTSOrderHack);
}

RTSPStreamHandler::RTSPStreamHandler(RTSPTransport* pOwner)
    : m_lRefCount(0)
    , m_pOwner(pOwner)
    , m_unMaxStreamGroup(0)
    , m_pStreamGroupToStreamNumMap(0)
{
    m_pStreamDataMap = new CHXMapLongToObj;
}

RTSPStreamHandler::~RTSPStreamHandler()
{
    CHXMapLongToObj::Iterator i;
    RTSPStreamData* pStreamData;

    for(i=m_pStreamDataMap->Begin();i!=m_pStreamDataMap->End();++i)
    {
	pStreamData = (RTSPStreamData*)(*i);
	delete pStreamData;
    }
    delete m_pStreamDataMap;

    HX_VECTOR_DELETE(m_pStreamGroupToStreamNumMap);
}


HX_RESULT
RTSPStreamHandler::initStreamData(
    UINT16 streamNumber, UINT16 streamGroupNumber,
    HXBOOL needReliable, HXBOOL bIsSource, INT16 rtpPayloadType,
    HXBOOL bRecordFlag, UINT32 wrapSequenceNumber, UINT32 ulBufferDepth,
    HXBOOL bHasOutOfOrderTS, CHXTimestampConverter* pTSConverter,
    RTSPMediaType eMediaType)
{
    RTSPStreamData* pStreamData = 0;
    HX_RESULT hr = HXR_OK;

    hr = initStreamData( streamNumber, needReliable, bIsSource, rtpPayloadType,
			bRecordFlag, wrapSequenceNumber, ulBufferDepth, bHasOutOfOrderTS,
			pTSConverter, eMediaType);
    if (SUCCEEDED(hr))
    {
	if(m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
	{
	    pStreamData->m_streamGroupNumber = streamGroupNumber;

	    //* m_pStreamGroupToStreamNumMap is currently recreated
	    //*  for every stream initialized. This should 
	    //*  eventually be replaced by adding mechanism for 
	    //*  passing StreamGroupCount to RTSPStreamHandler
	    //*  constructor
	    //*
	    //* Recreate map only if new Max 
	    if (!m_pStreamGroupToStreamNumMap || (streamGroupNumber > m_unMaxStreamGroup))
	    { 
		UINT16* pNewMap = 0;
		UINT16  unOldMax = m_unMaxStreamGroup;

		//* reset MaxStreamGroup
		m_unMaxStreamGroup =  streamGroupNumber;
		pNewMap = new UINT16[m_unMaxStreamGroup + 1];
		
		if (m_pStreamGroupToStreamNumMap)
		{
		    memcpy(pNewMap, m_pStreamGroupToStreamNumMap, (unOldMax + 1) * sizeof(UINT16));
		    HX_VECTOR_DELETE(m_pStreamGroupToStreamNumMap);
		}
		
		m_pStreamGroupToStreamNumMap = pNewMap;
	    }

	    //* initialize map entry
	    m_pStreamGroupToStreamNumMap[streamGroupNumber] = streamNumber;
	}
	else
	{
	    hr = HXR_FAIL;
	}
    }

    return hr;
}

HX_RESULT
RTSPStreamHandler::initStreamData(
    UINT16 streamNumber, HXBOOL needReliable, HXBOOL bIsSource, INT16 rtpPayloadType,
    HXBOOL bRecordFlag, UINT32 wrapSequenceNumber, UINT32 ulBufferDepth,
    HXBOOL bHasOutOfOrderTS, CHXTimestampConverter* pTSConverter,
    RTSPMediaType eMediaType)
{
    RTSPStreamData* pStreamData;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	pStreamData = new RTSPStreamData(needReliable);
        if( !pStreamData )
        {
            return HXR_OUTOFMEMORY;
        }
	pStreamData->m_streamNumber = streamNumber;
        pStreamData->m_pTSConverter = pTSConverter;
	pStreamData->m_eMediaType = eMediaType;
	pStreamData->m_lastSeqNo = 0;
	(*m_pStreamDataMap)[streamNumber] = pStreamData;

	if (!bIsSource)
	{
	    UINT32 ulSetMaximumBufferDepth;
	    UINT32 ulSetBufferDepth;

	    if (bRecordFlag && ulBufferDepth != TRANSPORT_BUF_DURATION_UNDEF)
	    {
	    	ulSetMaximumBufferDepth = ulBufferDepth;
	    	ulSetBufferDepth = ulBufferDepth;
	    }
	    else if (bRecordFlag)
	    {
	    	ulSetMaximumBufferDepth = MAX_TRANSPORT_BUF_DURATION;
	    	ulSetBufferDepth = MAX_TRANSPORT_BUF_DURATION;
	    }
	    else
	    {
	    	ulSetMaximumBufferDepth = MAX_TRANSPORT_BUF_DURATION;
	    	ulSetBufferDepth = TRANSPORT_BUF_DURATION;
	    }

	    RTSPTransportBuffer* pTransportBuffer = 
		new RTSPTransportBuffer(m_pOwner,
	                                streamNumber,
	                                ulSetBufferDepth,
	                                ulSetMaximumBufferDepth,
	                                TRANSPORT_BUF_GROWTH_RATE,
	                                wrapSequenceNumber);


	    pStreamData->m_pTransportBuffer = pTransportBuffer;
	    pStreamData->m_pStreamStats = NULL;
	    pStreamData->m_bUsesRTPPackets = bHasOutOfOrderTS;
	}

	return HXR_OK;
    }

    return HXR_FAIL;
}

RTSPStreamData*
RTSPStreamHandler::getStreamData(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData = 0;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	return 0;
    }

    return pStreamData;
}

RTSPStreamData*
RTSPStreamHandler::getStreamDataForStreamGroup(UINT16 streamGroupNumber)
{
    HX_ASSERT(streamGroupNumber < m_unMaxStreamGroup);

    return getStreamData( m_pStreamGroupToStreamNumMap[streamGroupNumber]);
}

RTSPStreamData*
RTSPStreamHandler::firstStreamData()
{
    streamIterator = m_pStreamDataMap->Begin();
    if(streamIterator == m_pStreamDataMap->End())
    {
	return 0;
    }
    return (RTSPStreamData*)(*streamIterator);
}

RTSPStreamData*
RTSPStreamHandler::nextStreamData()
{
    ++streamIterator;
    if(streamIterator == m_pStreamDataMap->End())
    {
	return 0;
    }
    return (RTSPStreamData*)(*streamIterator);
}

HXBOOL
RTSPStreamHandler::endStreamData()
{
    return (streamIterator == m_pStreamDataMap->End());
}

HX_RESULT
RTSPStreamHandler::createResendBuffer(UINT16 streamNumber,
				      UINT32 wrapSequenceNumber)
{
    RTSPStreamData* pStreamData;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	return HXR_FAILED;
    }

    RTSPResendBuffer* pResendBuffer;

    pResendBuffer = new RTSPResendBuffer(RESEND_BUF_DURATION,
                                         MAX_RESEND_BUF_DURATION,
                                         RESEND_BUF_GROWTH_RATE,
                                         wrapSequenceNumber);
    if( !pResendBuffer )
    {
        return HXR_OUTOFMEMORY;
    }

    pStreamData->m_pResendBuffer = pResendBuffer;
    return HXR_OK;
}

RTSPResendBuffer*
RTSPStreamHandler::getResendBuffer(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData = 0;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	return 0;
    }
    return pStreamData->m_pResendBuffer;
}

/*
 * RTSPTransport methods
 */

RTSPTransport::RTSPTransport(HXBOOL bIsSource):
    m_bHackedRecordFlag(FALSE),
    m_pPlayerState(NULL),
    m_pContext(NULL),
    m_pCommonClassFactory(0),
    m_pScheduler(0),
    m_pResp(0),
    m_pRegistry(0),
    m_pInternalReset(0),
    m_pSrcBufferStats(0),
    m_pSourceLatencyStats(0),
    m_pStreamHandler(0),
    m_bIsSource(bIsSource),
    m_ulRegistryID(0),
    m_ulPacketsSent(0),
    m_lBytesSent(0),
    m_ulPlayRangeFrom(RTSP_PLAY_RANGE_BLANK),
    m_ulPlayRangeTo(RTSP_PLAY_RANGE_BLANK),
    m_bIsInitialized(FALSE),
    m_bIsUpdated(FALSE),
    m_bPrefetch(FALSE),
    m_bFastStart(FALSE),
    m_bIsReceivedData(FALSE),
    m_bSourceDone(FALSE),
    m_wrapSequenceNumber(0),
    m_bMulticast(FALSE),
    m_pPacketFilter(NULL),
    m_pClientPacketList(NULL),
    m_pNoTransBufPktList(NULL),
    m_bPlayRequestSent(FALSE),
    m_ulTotalSuccessfulResends(0),
    m_ulTotalFailedResends(0),
    m_ulSendingTime(0),
    m_drop_packets(FALSE),
    m_packets_since_last_drop(0),
    m_bSkipTimeAdjustment(FALSE),
    m_nProbingPacketsRequested(0),
    m_nProbingPacketsReceived(0),
    m_pABDProbPktInfo(NULL)
#ifdef RDT_MESSAGE_DEBUG
    ,m_bRDTMessageDebug(FALSE)
#endif 
{
    m_ulStartTime = HX_GET_TICKCOUNT();
}

RTSPTransport::~RTSPTransport()
{
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pResp);
    HX_RELEASE(m_pRegistry);
    if (m_pStreamHandler)
    {
        m_pStreamHandler->Release();
        m_pStreamHandler = NULL;
    }
    HX_RELEASE(m_pInternalReset);
    HX_RELEASE(m_pSrcBufferStats);
    HX_RELEASE(m_pSourceLatencyStats);
    HX_RELEASE(m_pPlayerState);
    HX_RELEASE(m_pPacketFilter);
    HX_RELEASE(m_pContext);

    destroyPktList(m_pClientPacketList);
    destroyPktList(m_pNoTransBufPktList);
    destroyABDPktInfo();
}

void
RTSPTransport::addStreamInfo(RTSPStreamInfo* pStreamInfo, UINT32 ulBufferDepth)
{
    if (pStreamInfo)
    {
	if (!m_pStreamHandler)
	{
	    m_pStreamHandler = new RTSPStreamHandler(this);
            if( !m_pStreamHandler )
            {
                return;
            }
	    m_pStreamHandler->AddRef();
	}

	CHXTimestampConverter*  pTSConverter = NULL;

	if (pStreamInfo->m_HXFactor && pStreamInfo->m_RTPFactor)
	{
	    pTSConverter = new CHXTimestampConverter(CHXTimestampConverter::FACTORS,
						     pStreamInfo->m_HXFactor,
						     pStreamInfo->m_RTPFactor);
            if( !pTSConverter )
            {
                m_pStreamHandler->Release();
                return;
            }
	}
	else if (pStreamInfo->m_sampleRate)
	{
	     
	    pTSConverter =  new CHXTimestampConverter(CHXTimestampConverter::SAMPLES,
						       pStreamInfo->m_sampleRate);
            if( !pTSConverter )
            {
                m_pStreamHandler->Release();
                return;
            }
	 }	

	 m_pStreamHandler->initStreamData(
	     pStreamInfo->m_streamNumber,
	     pStreamInfo->m_uStreamGroupNumber,
	     pStreamInfo->m_bNeedReliablePackets,
	     m_bIsSource,
	     pStreamInfo->m_rtpPayloadType,
	     m_bHackedRecordFlag,
	     m_wrapSequenceNumber,
	     ulBufferDepth,
	     pStreamInfo->m_bHasOutOfOrderTS,
	     pTSConverter,
	     pStreamInfo->m_eMediaType);

        m_bSkipTimeAdjustment = pStreamInfo->m_bRealMedia;

        // Get the stream data for this stream
	RTSPStreamData* pStreamData = NULL;
	pStreamData = m_pStreamHandler->getStreamData(pStreamInfo->m_streamNumber);
	if (pStreamData && pStreamData->m_pTransportBuffer)
	{
            // Have we already sent a PLAY request?
            if (m_bPlayRequestSent)
            {
                // This stream's SETUP response arrived after we
                // have already sent the PLAY request. Therefore,
                // there is additional setup which needs to
                // be done to this stream. This was done in
                // playReset() and resumeBuffers() for the
                // streams which were already set up when
                // we sent the PLAY request.
                if (!m_bIsSource)
                {
                    pStreamData->m_pTransportBuffer->Reset();
                    pStreamData->m_bReceivedAllPackets = FALSE;
                }
                pStreamData->m_pTransportBuffer->Resume();
            }

            if (m_bPrefetch)
            {
                pStreamData->m_pTransportBuffer->EnterPrefetch();
            }
	}

	if (m_bFastStart)
	{
	    EnterFastStart();
	}
    }
}

void
RTSPTransport::setFirstTimeStamp(UINT16 uStreamNumber, UINT32 ulTS, 
                                 HXBOOL bIsRaw, HXBOOL bOnPauseResume)
{
    RTSPStreamData* pStreamData = 
	m_pStreamHandler->getStreamData(uStreamNumber);

    HXLOGL3(HXLOG_RTSP, "RTSPTransport[%p]::setFirstTimeStamp(uStreamNumber=%hu, ulTS=%hu, bIsRaw=%c)", 
	    this, 
	    uStreamNumber,
	    ulTS,
	    bIsRaw ? 'T' : 'F');

    if (pStreamData)
    {
	if (pStreamData->m_pTSConverter)
	{
	    pStreamData->m_pTSConverter->setHXAnchor(ulTS);
	}
	    	    
	HX_DELETE(pStreamData->m_pTSOrderHack);
    }

    if (!m_bIsSource)
    {
	RTSPTransportBuffer* pTransportBuffer = pStreamData->m_pTransportBuffer;
		
	if (pTransportBuffer && pStreamData)
	{
	    if ((m_ulPlayRangeFrom != RTSP_PLAY_RANGE_BLANK) &&
		(m_ulPlayRangeTo != RTSP_PLAY_RANGE_BLANK))
	    {
		if ((pStreamData->m_eMediaType == RTSPMEDIA_TYPE_AUDIO) ||
		    (pStreamData->m_eMediaType == RTSPMEDIA_TYPE_VIDEO))
		{
		    // For audio & video media, we'll inform transport of
		    // the media duration to help determining stream termination
		    pStreamData->m_pTransportBuffer->InformTimestampRange(
			m_ulPlayRangeFrom,
			m_ulPlayRangeTo,
			STREAM_END_DELAY_BASE_TOLERANCE);
		}
	    }
	}
    }
}

void 
RTSPTransport::setPlayRange(UINT32 ulFrom, UINT32 ulTo)
{
    // this is the Range values in PLAY request in RMA time (ms) called on PLAY 
    // request
    m_ulPlayRangeFrom = ulFrom; 
    m_ulPlayRangeTo = ulTo;

    HXLOGL3(HXLOG_RTSP, "RTSPTransport[%p]::setPlayRange(From=%lu, To=%lu)", 
	    this, 
	    m_ulPlayRangeFrom, 
	    m_ulPlayRangeTo);
}

void
RTSPTransport::setSessionID(const char* pSessionID)
{
    m_sessionID = pSessionID;
}

UINT16
RTSPTransport::getSeqNum(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	return pStreamData->m_seqNo;
    }
    else
    {
	return 0;	//XXXBAB - 0xffff?
    }
}

UINT16
RTSPTransport::getFirstSeqNum(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	return pStreamData->m_firstSeqNo;
    }
    else
    {
	return 0;	//XXXBAB - 0xffff?
    }
}

UINT32
RTSPTransport::getTimestamp(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	// XXXGo - this is RTP time w/ offset (NOT RMA time) if RTPUDPTransprot...
	return pStreamData->m_lastTimestamp;
    }
    else
    {
	return 0;
    }
}

HX_RESULT
RTSPTransport::getPacket(UINT16 uStreamNumber, IHXPacket*& pPacket)
{
    UINT32 uSeqNum;
    return getPacket(uStreamNumber, pPacket, uSeqNum);
}

HX_RESULT
RTSPTransport::getPacket(UINT16 uStreamNumber, IHXPacket*& pPacket,
                         REF(UINT32) uSeqNum)
{
    RTSPTransportBuffer* pTransportBuffer = getTransportBuffer(uStreamNumber);
    RTSPStreamData* pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);

    if ((!pTransportBuffer) || (!pStreamData))
    {
	return HXR_FAIL;
    }

    ClientPacket* clientPacket;

    HX_RESULT result = pTransportBuffer->GetPacket(clientPacket);

    if (result != HXR_OK)
    {
	return result;
    }

    pPacket = clientPacket->GetPacket();
    uSeqNum = clientPacket->GetSequenceNumber();

    if (!pPacket)
    {
	/*
	 * This is a lost packet
	 */
	UINT8 unASMFlags = 0;
	UINT32 ulTime = 0;

	if (clientPacket->IsDroppedPacket())
	{
	    // Preserve dropped flag as an ASM flag.
	    // This allows other code along the packet
	    // path to differentiate this packet from a 
	    // true lost packet.
	    unASMFlags |= HX_ASM_DROPPED_PKT;

	    // We have a valid timestamp for a dropped
	    // packet so lets put it in the IHXPacket
	    ulTime = clientPacket->GetTime();
	}

	if (pStreamData->m_bUsesRTPPackets)
	{
	    IHXRTPPacket* pRTPPacket = NULL;

	    result = m_pCommonClassFactory->CreateInstance(CLSID_IHXRTPPacket,
							   (void**)&pRTPPacket);

	    if (result != HXR_OK)
	    {
		return result;
	    }

	    pRTPPacket->SetRTP(0,
			       ulTime,
			       0,	// We do not set RTP time-stamp for lost packet
			       uStreamNumber,
			       unASMFlags,
			       0);

	    pPacket = (IHXPacket*) pRTPPacket;
	}
	else
	{
	    result = m_pCommonClassFactory->CreateInstance(CLSID_IHXPacket,
							   (void**)&pPacket);

	    if (result != HXR_OK)
	    {
		return result;
	    }

	    pPacket->Set(0, ulTime, uStreamNumber, unASMFlags, 0);
	}

	pPacket->SetAsLost();
    }
    else if (pStreamData->m_bUsesRTPPackets)
    {
	if (!pStreamData->m_pTSOrderHack)
	{
	    pStreamData->m_pTSOrderHack = new RTSPStreamData::TSOrderHackInfo();
            if( !pStreamData->m_pTSOrderHack )
            {
                return HXR_OUTOFMEMORY;
            }
	    
	    if (pStreamData->m_pTSOrderHack)
	    {
		pStreamData->m_pTSOrderHack->m_ulLastSentTS = 
		    pPacket->GetTime();
		pStreamData->m_pTSOrderHack->m_ulLastRecvTS = 
		    pStreamData->m_pTSOrderHack->m_ulLastSentTS;
	    }
	}

	if (pStreamData->m_pTSOrderHack)
	{
	    IHXBuffer* pBuf=NULL;	    
	    UINT32 ulHX;
	    UINT32 ulRTP;
	    UINT16 unStrmNo;
	    UINT8  uchASMFlag;
	    UINT16 unRuleNo;

	    // pkts've been sorted based on seq_no
	    IHXRTPPacket* pRTPPacket = NULL;

	    pPacket->QueryInterface(IID_IHXRTPPacket, (void**) &pRTPPacket);

	    if (pRTPPacket)
	    {
		// RTP transport generates RTP packets
		result = pRTPPacket->GetRTP(pBuf, ulHX, ulRTP, 
					    unStrmNo, uchASMFlag, unRuleNo);
	    }
	    else
	    {
		// RDT transport generates RMA packets
		result = pPacket->Get(pBuf, ulHX,
				      unStrmNo, uchASMFlag, unRuleNo);

		if (pStreamData->m_pTSConverter)
		{
		    ulRTP = pStreamData->m_pTSConverter->hxa2rtp(ulHX);
		}
		else
		{
		    ulRTP = ulHX;
		}
	    }

	    HX_ASSERT(result == HXR_OK);

	    if (result == HXR_OK)
	    {
		if (((LONG32) (ulHX - pStreamData->m_pTSOrderHack->m_ulLastSentTS)) > 0)
		{
		    pStreamData->m_pTSOrderHack->m_ulLastSentTS = ulHX;
		    pStreamData->m_pTSOrderHack->m_ulLastRecvTS = ulHX;
		}
		else if (ulHX == pStreamData->m_pTSOrderHack->m_ulLastRecvTS)
		{
		    ulHX = pStreamData->m_pTSOrderHack->m_ulLastSentTS;	    	    
		}
		else
		{
		    pStreamData->m_pTSOrderHack->m_ulLastRecvTS = ulHX;
		    ulHX = (++pStreamData->m_pTSOrderHack->m_ulLastSentTS);
		}
		
		HX_RELEASE(pRTPPacket);
		HX_RELEASE(pPacket);
		pRTPPacket = new CHXRTPPacket;
                if( !pRTPPacket )
                {
                    return HXR_OUTOFMEMORY;
                }
		pRTPPacket->AddRef();
		result = pRTPPacket->SetRTP(pBuf,
					    ulHX,
					    ulRTP,
					    unStrmNo,
					    uchASMFlag,
					    unRuleNo);
		HX_ASSERT(result == HXR_OK);		
		pRTPPacket->QueryInterface(IID_IHXPacket, (void**) &pPacket);
		HX_ASSERT(pPacket);
	    }
	    HX_RELEASE(pBuf);

	    HX_RELEASE(pRTPPacket);
	}
    }

    /*
     * No longer need the ClientPacket wrapper
     */
    HX_RELEASE(clientPacket);

    HXLOGL4(HXLOG_TRAN, "RTSPTransport::getPacket() GotPacket: StrmNum=%hu SeqNo=%hu TS=%lu ASMRule=%hu",
	    pPacket->GetStreamNumber(),
	    uSeqNum,
	    pPacket->GetTime(),
	    pPacket->GetASMRuleNumber());

    return HXR_OK;
}

UINT32
RTSPTransport::getFirstTimestamp(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	// XXXGo - this is RTP time w/ offset (NOT RMA time) if RTPUDPTransprot...
	return pStreamData->m_firstTimestamp;
    }
    else
    {
	return 0;
    }
}

HX_RESULT
RTSPTransport::startPackets(UINT16 uStreamNumber)
{
    RTSPTransportBuffer* pTransportBuffer = getTransportBuffer(uStreamNumber);

    if (!pTransportBuffer)
    {
	return HXR_FAIL;
    }

    return pTransportBuffer->StartPackets();
}

HX_RESULT
RTSPTransport::stopPackets(UINT16 uStreamNumber)
{
    RTSPTransportBuffer* pTransportBuffer = getTransportBuffer(uStreamNumber);

    if (!pTransportBuffer)
    {
	return HXR_FAIL;
    }

    return pTransportBuffer->StopPackets();
}

void
RTSPTransport::SetFilterResponse(RawPacketFilter* pResp)
{
    HX_RELEASE(m_pPacketFilter);
    m_pPacketFilter = pResp;
    if (m_pPacketFilter)
    {
	m_pPacketFilter->AddRef();
    }
}

void
RTSPTransport::FilterPacket(IHXPacket* pPacket)
{
    UINT16 uStreamNumber;

    ClientPacket* clientPacket = 0;
    clientPacket = (ClientPacket*)m_pClientPacketList->RemoveTail();
    clientPacket->SetPacket(pPacket);
    
    uStreamNumber = clientPacket->GetStreamNumber();
    RTSPTransportBuffer* pTransportBuffer = getTransportBuffer(uStreamNumber);

    // We are transfering ownership here so we
    // don't want to HX_RELEASE(clientPacket).
    // pTransportBuffer->Add() expects the
    // caller to AddRef() the packet so we
    // just use the AddRef() that occurred
    // when the packet was put in m_pClientPacketList
    pTransportBuffer->Add(clientPacket);
}

void RTSPTransport::GetContext(IUnknown*& pContext)   
{   
	pContext = m_pContext;   

	if (pContext)   
	{   
		pContext->AddRef();   
	}   
} 

void 
RTSPTransport::LeavePrefetch(void)
{
    RTSPStreamData* pStreamData = NULL; 
    
    m_bPrefetch = FALSE;

    HX_ASSERT(m_pStreamHandler);

    pStreamData = m_pStreamHandler->firstStreamData();
    while(pStreamData)    
    {
	if (pStreamData->m_pTransportBuffer)
	{
	    pStreamData->m_pTransportBuffer->LeavePrefetch();
	}
	pStreamData = m_pStreamHandler->nextStreamData();
    }

    return;
}

void 
RTSPTransport::EnterFastStart(void)
{
    RTSPStreamData* pStreamData = NULL; 
    
    m_bFastStart = TRUE;

    // For protocols that use one channel per stream (e.g. RTP)
    // as opposed multiplexing streams into a single channel (e.g. RDT) it
    // is possible for this call to arrive before the transport has
    // been initialized via addStreamInfo and thus m_pStreamHandler
    // created.  This occurs when SETUP pipelining is used where
    // only the first SETUP is awaited before proceeding with initialization
    // of the playback engine structures.
    // In such case, we'll simply remember fast start state and
    // re-execute this code once the transport information is added.
    if (m_pStreamHandler)
    {
	pStreamData = m_pStreamHandler->firstStreamData();
	while(pStreamData)    
	{
	    if (pStreamData->m_pTransportBuffer)
	    {
		pStreamData->m_pTransportBuffer->EnterFastStart();
	    }
	    pStreamData = m_pStreamHandler->nextStreamData();
	}
    }

    return;
}

void 
RTSPTransport::LeaveFastStart(void)
{
    RTSPStreamData* pStreamData = NULL; 
    
    m_bFastStart = FALSE;

    HX_ASSERT(m_pStreamHandler);

    pStreamData = m_pStreamHandler->firstStreamData();
    while(pStreamData)    
    {
	if (pStreamData->m_pTransportBuffer)
	{
	    pStreamData->m_pTransportBuffer->LeaveFastStart();
	}
	pStreamData = m_pStreamHandler->nextStreamData();
    }

    return;
}

HXBOOL 
RTSPTransport::isSparseStream(UINT16 uStreamNumber)
{
    HXBOOL		bResult = FALSE;
    const char*		pMimeType = NULL;
    IUnknown*		pUnknown = NULL;
    IHXStreamSource*	pStreamSource = NULL;    
    IHXStream*		pStream = NULL;

    if (m_pContext  &&
	HXR_OK == m_pContext->QueryInterface(IID_IHXStreamSource, (void**)&pStreamSource))
    {
	if (HXR_OK == pStreamSource->GetStream(uStreamNumber, pUnknown))
	{
	    if (HXR_OK == pUnknown->QueryInterface(IID_IHXStream, (void**)&pStream))
	    {
		pMimeType = pStream->GetStreamType();

		// special handling for sparsed streams for multicast
		if (pMimeType							&&
		    (strcasecmp(SYNCMM_MIME_TYPE, pMimeType) == 0		||
		     strcasecmp(REALEVENT_MIME_TYPE, pMimeType) == 0		||
		     strcasecmp("application/vnd.rn-realtext", pMimeType) == 0	||
		     strcasecmp("application/x-pn-realtext", pMimeType) == 0))
		{
		    bResult = TRUE;
		}
	    }
	    HX_RELEASE(pStream);
	}
	HX_RELEASE(pUnknown);
    }
    HX_RELEASE(pStreamSource);

    return bResult;
}

HX_RESULT
RTSPTransport::storePacket(IHXPacket* pPacket,
                           UINT16 uStreamNumber,
                           UINT16 uSeqNo,
                           UINT16 uReliableSeqNo,
                           HXBOOL isReliable)
{
    m_bIsReceivedData = TRUE;

    ULONG32 ulPktSize = 0;
    IHXPacket* pPkt = NULL;

    if (pPacket && !pPacket->IsLost())
    {
        if (m_pSourceLatencyStats)
        {
            m_pSourceLatencyStats->NewPacketTimeStamp(pPacket->GetTime());
        }
        
        IHXBuffer* pBuffer = pPacket->GetBuffer();

        if (pBuffer)
        {
            ulPktSize = pBuffer->GetSize();
        }

        HX_RELEASE(pBuffer);

        pPkt = pPacket;
    }

    RTSPTransportBuffer* pTransportBuffer = getTransportBuffer(uStreamNumber);
    Timeval startTime;

    if (pTransportBuffer)
    {
        startTime = pTransportBuffer->GetTime();
    }

    HX_RESULT res = HXR_OUTOFMEMORY;
    ClientPacket* pClientPacket = new ClientPacket(uSeqNo,
                                                   uReliableSeqNo,
                                                   pPacket->GetTime(),
                                                   ulPktSize,
                                                   isReliable,
                                                   pPkt,
                                                   startTime,
                                                   FALSE);

    HXLOGL4(HXLOG_TRAN, "RTSPTransport::storePacket() Strm=%hu SeqNo=%hu ReliableSeqNo=%hu Reliable=%c TS=%lu ASMRule=%hu", 
	uStreamNumber,
	uSeqNo,
	uReliableSeqNo,
	isReliable ? 'T' : 'F',
	pPacket->GetTime(),
	pPacket->GetASMRuleNumber());

    if (pClientPacket)
    {
        pClientPacket->AddRef();

        if(pTransportBuffer)
        {
            if (m_pNoTransBufPktList)
            {
                // We handle any packets received
                // when we didn't have a transport buffer
                // first.
                res = handleNoTransBufList(pTransportBuffer);
            }
            else
            {
                res = HXR_OK;
            }

            if (HXR_OK == res)
            {
                // Now store the packet for this call
                res = storeClientPacket(pTransportBuffer, pClientPacket);
            }
        }
        else
        {
            // We don't have a transport buffer yet.
            // Queue the packet until we have one.
            if (!m_pNoTransBufPktList)
            {
                m_pNoTransBufPktList = new CHXSimpleList;
            }

            if (m_pNoTransBufPktList &&
                m_pNoTransBufPktList->AddTail(pClientPacket))
            {
                // We successfully queued the packet
                pClientPacket->AddRef();
                res = HXR_OK;
            }
        }

        HX_RELEASE(pClientPacket);
    }

    return res;
}
    
HX_RESULT 
RTSPTransport::storeClientPacket(RTSPTransportBuffer* pTransportBuffer,
                                 ClientPacket* pClientPkt)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    IHXPacket* pPkt = pClientPkt->GetPacket();

    if (!pPkt)
    {
        // This is a lost packet

        // pTransportBuffer->Add() expects the
        // caller to AddRef() the packet.
        pClientPkt->AddRef();
        res = pTransportBuffer->Add(pClientPkt);
    }
    else if (m_pPacketFilter)
    {
        if (!m_pClientPacketList)
        {
            m_pClientPacketList = new CHXSimpleList;

            if (m_pClientPacketList)
            {
                m_pPacketFilter->SetFilterResponse(this);
            }
        }

        if (m_pClientPacketList &&
            m_pClientPacketList->AddHead((void*)pClientPkt))
        {
            // Successfully added the client packet to the list
            pClientPkt->AddRef();

            m_pPacketFilter->FilterPacket(pPkt);
            res = HXR_OK;
        }
    }
    else
    {
        // pTransportBuffer->Add() expects the
        // caller to AddRef() the packet.
        pClientPkt->AddRef();
        res = pTransportBuffer->Add(pClientPkt);
    }

    HX_RELEASE(pPkt);

    return res;
}
 

HX_RESULT			
RTSPTransport::packetReady(HX_RESULT status, RTSPStreamData* pStreamData, IHXPacket* pPacket)
{
    HX_RESULT result = HXR_OK;

    ASSERT(!m_bIsSource);

    if (!pStreamData)
    {
	return HXR_UNEXPECTED;
    }

    HX_ASSERT(pStreamData);
    
    if (pPacket)
    {
	if (pStreamData->m_bUsesRTPPackets)
	{
	    IHXRTPPacket* pRTPPacket = NULL;

	    pPacket->QueryInterface(IID_IHXRTPPacket, (void**) &pRTPPacket);

	    if (pRTPPacket)
	    {
		// This already is RTP Packet - proceed
		pRTPPacket->Release();
	    }
	    else
	    {
		// Must Transfer to an RTP Packet
		result = m_pCommonClassFactory->CreateInstance(
				CLSID_IHXRTPPacket,
				(void**) &pRTPPacket);

		if (pRTPPacket)
		{
		    ULONG32 ulTime;
		    ULONG32 ulRTPTime;
		    UINT16 uStreamNumber;
		    UINT8 unASMFlags;
		    UINT16 unASMRuleNumber;
		    HXBOOL bIsLost = pPacket->IsLost();
		    IHXBuffer* pBuffer = NULL;

		    pPacket->Get(pBuffer,
				 ulTime,
				 uStreamNumber,
				 unASMFlags,
				 unASMRuleNumber);

		    if (pStreamData->m_pTSConverter)
		    {
			ulRTPTime = pStreamData->m_pTSConverter->hxa2rtp(ulTime);
		    }
		    else
		    {
			ulRTPTime = ulTime;
		    }

		    pRTPPacket->SetRTP(pBuffer,
				       ulTime,
				       ulRTPTime,
				       uStreamNumber,
				       unASMFlags,
				       unASMRuleNumber);

		    if (bIsLost)
		    {
			pRTPPacket->SetAsLost();
		    }

		    HX_RELEASE(pBuffer);

		    result = m_pResp->PacketReady(status, m_sessionID, pRTPPacket);
		    pRTPPacket->Release();
		}

		return result;
	    }
	}
    }
    else
    {
	/*
	 * This is a lost packet
	 */
	if (pStreamData->m_bUsesRTPPackets)
	{
	    result = m_pCommonClassFactory->CreateInstance(
			CLSID_IHXRTPPacket,
			(void**) &pPacket);
	}
	else
	{
	    result = m_pCommonClassFactory->CreateInstance(
			CLSID_IHXPacket,
			(void**) &pPacket);
	}
	    
	if (pPacket)
	{
	    pPacket->Set(0, 0, pStreamData->m_streamNumber, 0, 0);
	    pPacket->SetAsLost();
	    result = m_pResp->PacketReady(status, m_sessionID, pPacket);
	    pPacket->Release();
	}
	return result;
    }

    return m_pResp->PacketReady(status, m_sessionID, pPacket);
}

HX_RESULT
RTSPTransport::packetReady(HX_RESULT status, UINT16 uStreamNumber, IHXPacket* pPacket)
{
    RTSPStreamData* pStreamData = NULL;

    ASSERT(!m_bIsSource);

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);

    HX_ASSERT(pStreamData);

    return packetReady(status, pStreamData, pPacket);
}

HX_RESULT
RTSPTransport::getStatus
(
    UINT16& uStatusCode, 
    IHXBuffer*& pStatusDesc, 
    UINT16& ulPercentDone
)
{
#if 0
    if (!m_pStreamHandler)
    {
	uStatusCode = HX_STATUS_INITIALIZING;
	pStatusDesc = 0;
	ulPercentDone = 0;

	return HXR_OK;
    }

    uStatusCode = HX_STATUS_READY;
    pStatusDesc = 0;
    ulPercentDone = 100;

    RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

    ASSERT(pStreamData);

    while(pStreamData)
    {
	UINT16 tempStatusCode;
	UINT16 tempPercentDone;

	pStreamData->m_pTransportBuffer->GetStatus(tempStatusCode,
	                                           tempPercentDone);

	/*
	 * The status is always that of the stream which is least ready
	 */

	if (tempStatusCode < uStatusCode)
	{
	    uStatusCode	    = tempStatusCode;
	    ulPercentDone   = tempPercentDone;
	}
	else if (tempStatusCode == uStatusCode && 
		 tempPercentDone < ulPercentDone)
	{
	    ulPercentDone = tempPercentDone;
	}

	pStreamData = m_pStreamHandler->nextStreamData();
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif
}

HX_RESULT
RTSPTransport::GetCurrentBuffering(UINT16 uStreamNumber,
				   UINT32& ulLowestTimestamp, 
				   UINT32& ulHighestTimestamp,
				   UINT32& ulNumBytes,
				   HXBOOL& bDone)
{
    if (!m_pStreamHandler)
    {
	return HXR_OK;
    }

    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);

    HX_ASSERT(pStreamData);

    return pStreamData ? 
           pStreamData->m_pTransportBuffer->GetCurrentBuffering(
                                            ulLowestTimestamp,
                                            ulHighestTimestamp,
                                            ulNumBytes,
                                            bDone) : HXR_OK;
}

HX_RESULT
RTSPTransport::SeekFlush(UINT16 uStreamNumber)
{
    if (!m_pStreamHandler)
    {
	return HXR_OK;
    }

    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);

    HX_ASSERT(pStreamData);

    if (pStreamData)
    {
	pStreamData->m_pTransportBuffer->SeekFlush();
    }

    return HXR_OK;
}

HXBOOL
RTSPTransport::IsSourceDone(void)
{
    return m_bSourceDone;
}

void      
RTSPTransport::CheckForSourceDone(UINT16 uStreamNumber)
{
    if (m_bSourceDone)
    {
        return;
    }

    HX_ASSERT(m_pStreamHandler);

    if (!m_pStreamHandler)
    {
        return;
    }

    RTSPStreamData* pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);
    HX_ASSERT(pStreamData);

    if (pStreamData)
    {
        pStreamData->m_bReceivedAllPackets = TRUE;

        m_bSourceDone = TRUE;
        pStreamData = m_pStreamHandler->firstStreamData();      
        while(pStreamData)
        {
            if (!pStreamData->m_bReceivedAllPackets)
            {
                m_bSourceDone = FALSE;
                return;
            }
            pStreamData = m_pStreamHandler->nextStreamData();
        }
    }

    /* We have received all the packets... Tell the reposnse object */
    if (!m_bIsSource && m_bSourceDone)
    {
        m_pResp->OnSourceDone();
    }
}    

void      
RTSPTransport::HandleBufferError()
{
    if (m_pResp)
    {
	m_pResp->OnProtocolError(HXR_BUFFERING);
    }
}

RTSPResendBuffer*
RTSPTransport::getResendBuffer(UINT16 uStreamNumber)
{
    return m_pStreamHandler->getResendBuffer(uStreamNumber);
}

HX_RESULT
RTSPTransport::initializeStatistics
(
    UINT32 ulRegistryID
)
{
    m_bIsInitialized = TRUE;

    if (m_bIsSource)
    {
	m_ulRegistryID = ulRegistryID;
    }
    else
    {
	// XXX HP
	// rather than create a new copy of STREAM_STATS, we simply
	// obtain the same STREAM_STATS* from RTSPProtocol via SetStatistics().
	// this could save us on both the memory and several ms during the startup
/*
	IHXBuffer* pParentName = NULL;
	CHAR RegKeyName[MAX_DISPLAY_NAME] = {0};

	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

	ASSERT(pStreamData);

	if (!m_pRegistry)
	{
	    return HXR_FAIL;
	}

	HX_RESULT result = m_pRegistry->GetPropName(ulRegistryID, pParentName);

	if (result != HXR_OK)
	{
	    return result;
	}

	// create registry entries of each stream
	while (pStreamData)
	{
	    UINT16 uStreamNumber = pStreamData->m_streamNumber;

	    sprintf(RegKeyName, "%s.Stream%d", pParentName->GetBuffer(),
	                                       uStreamNumber);
	
	    UINT32 uStreamId = m_pRegistry->GetId(RegKeyName);

	    // create stream reg key if it has not been created
	    // by the source yet
	    if (!uStreamId)
	    {
		return HXR_FAIL;
	    }

	    pStreamData->m_pStreamStats = new STREAM_STATS(m_pRegistry,
	                                                   uStreamId);
							   
	    pStreamData = m_pStreamHandler->nextStreamData();
	}

	pParentName->Release();
*/
    }

    return HXR_OK;
}

HX_RESULT
RTSPTransport::SetStatistics(UINT16 uStreamNumber, STREAM_STATS* pStats)
{
    HX_RESULT	    rc = HXR_OK;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    RTSPStreamData* pStreamData = NULL;

    if (!m_pStreamHandler)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);
    if (!pStreamData)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pStreamData->m_pStreamStats = pStats;

cleanup:
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return rc;
}

HX_RESULT 
RTSPTransport::updateStatistics(HXBOOL bUseRegistry)
{
    m_bIsUpdated = TRUE;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_bIsSource)
    {
        if (!m_pStreamHandler)
        {
            HX_ASSERT(m_pStreamHandler);
            return HXR_FAIL;
        }

	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

	if (!pStreamData)
	{
	    return HXR_FAIL;
	}

	/*
	 * XXXGH...streams are managed by the PPM and should not show up
	 *         under the transport
	 */
	while (pStreamData)
	{
	    UINT32 ulResendSuccess = 0;
	    UINT32 ulResendFailure = 0;

	    if (pStreamData->m_pResendBuffer)
	    {
		pStreamData->m_pResendBuffer->UpdateStatistics(ulResendSuccess,
		                                               ulResendFailure);
	    }

	    m_ulTotalSuccessfulResends += ulResendSuccess;
	    m_ulTotalFailedResends += ulResendFailure;

	    pStreamData = m_pStreamHandler->nextStreamData();
	}

        m_ulSendingTime = CALCULATE_ELAPSED_TICKS(m_ulStartTime, HX_GET_TICKCOUNT()) / 1000;

	IHXBuffer* pName;

	if (bUseRegistry
        &&  m_pRegistry 
        &&  HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pName))
	{
	    char str[512]; /* Flawfinder: ignore */
	    const char* name = (const char*)pName->GetBuffer();
	    char pTemp[32]; /* Flawfinder: ignore */

	    i64toa(m_lBytesSent, pTemp, 10);

	    sprintf(str, "%-.400s.PacketsSent", name); /* Flawfinder: ignore */
	    m_pRegistry->AddInt(str, m_ulPacketsSent);
	    sprintf(str, "%-.400s.BytesSent", name); /* Flawfinder: ignore */
	    m_pRegistry->AddInt(str, INT64_TO_INT32(m_lBytesSent));

	    // Add the total bytes sent as a string because we don't want
	    // to have to truncate the INT64 value that is storing it
	    sprintf(str, "%-.400s.TotalBytesSent", name); /* Flawfinder: ignore */
	    IHXBuffer* pBuffer = NULL;
	    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)pTemp, 
						strlen(pTemp) + 1, m_pContext))
	    {
		m_pRegistry->AddStr(str, pBuffer);
		HX_RELEASE(pBuffer);
	    }

	    sprintf(str, "%-.400s.SendingTime", name); /* Flawfinder: ignore */
	    m_pRegistry->AddInt(str, m_ulSendingTime); 
	    sprintf(str, "%-.400s.ResendSuccess", name); /* Flawfinder: ignore */
	    m_pRegistry->AddInt(str, m_ulTotalSuccessfulResends);
	    sprintf(str, "%-.400s.ResendFailure", name); /* Flawfinder: ignore */
	    m_pRegistry->AddInt(str, m_ulTotalFailedResends);
	    pName->Release();
	}
    }
    else
    {
	ULONG32 ulNormal = 0;
	ULONG32 ulReceived = 0;
	ULONG32 ulLost = 0;
	ULONG32 ulLate = 0;
	ULONG32 ulTotal = 0;
	ULONG32 ulResendRequested = 0;
	ULONG32 ulResendReceived = 0;
	ULONG32 ulAvgBandwidth = 0;
	ULONG32 ulCurBandwidth = 0;

	UINT32	ulTotal30 = 0;
	UINT32	ulLost30 = 0;
	UINT32  ulDuplicate = 0;
	UINT32	ulOutOfOrder = 0;

	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

	if (!pStreamData)
	{
	    return HXR_FAIL;
	}

	while (pStreamData)
	{
	    STREAM_STATS* pStreamStats = pStreamData->m_pStreamStats;

            if (!pStreamStats || !pStreamStats->m_bInitialized)
	    {
		goto updateContinue;
	    }

	    pStreamData->m_pTransportBuffer->UpdateStatistics(ulNormal,
	                                                      ulLost,
	                                                      ulLate,
	                                                      ulResendRequested,
	                                                      ulResendReceived,
	                                                      ulAvgBandwidth,
	                                                      ulCurBandwidth,
							      ulTotal30,
							      ulLost30,
							      ulDuplicate,
							      ulOutOfOrder);
	                  
	    ulReceived = ulNormal + ulResendReceived;
	    ulTotal = ulReceived + ulLost + ulLate;

	    pStreamStats->m_pNormal->SetInt((INT32)ulNormal);
	    pStreamStats->m_pRecovered->SetInt((INT32)ulResendReceived);
	    pStreamStats->m_pReceived->SetInt((INT32)ulReceived);
	    pStreamStats->m_pLost->SetInt((INT32)ulLost);
	    pStreamStats->m_pLate->SetInt((INT32)ulLate);
	    pStreamStats->m_pDuplicate->SetInt((INT32)ulDuplicate);
	    pStreamStats->m_pOutOfOrder->SetInt((INT32)ulOutOfOrder);
	    pStreamStats->m_pTotal->SetInt((INT32)ulTotal);
	    pStreamStats->m_pLost30->SetInt((INT32)ulLost30);
	    pStreamStats->m_pTotal30->SetInt((INT32)ulTotal30);
	    pStreamStats->m_pResendRequested->SetInt((INT32)ulResendRequested);
	    pStreamStats->m_pResendReceived->SetInt((INT32)ulResendReceived);
	    pStreamStats->m_pAvgBandwidth->SetInt((INT32)ulAvgBandwidth);
	    pStreamStats->m_pCurBandwidth->SetInt((INT32)ulCurBandwidth);

            if (m_pSrcBufferStats)
            {
                UINT32 ulLowestTimeStamp;
                UINT32 ulHighestTimeStamp;
                UINT32 ulNumberOfBytes;
                HXBOOL bDone;
                HX_RESULT hxr = m_pSrcBufferStats->GetTotalBuffering(pStreamData->m_streamNumber,
                        ulLowestTimeStamp, ulHighestTimeStamp, ulNumberOfBytes, bDone);

                if (SUCCEEDED(hxr))
                {
                    pStreamStats->m_pFirstTimestamp->SetInt(ulLowestTimeStamp);
                    pStreamStats->m_pLastTimestamp->SetInt(ulHighestTimeStamp);
                    pStreamStats->m_pFilledBufferSize->SetInt((INT32)ulNumberOfBytes);
                }
            }

updateContinue:

	    pStreamData = m_pStreamHandler->nextStreamData();
	}
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return HXR_OK;
}

HX_RESULT 
RTSPTransport::UpdateRegistry(UINT32 ulStreamNumber,
			      UINT32 ulRegistryID)
{
    if (m_bIsSource)
    {
	m_ulRegistryID = ulRegistryID;
    }
    else
    {
	// XXX HP
	// rather than create a new copy of STREAM_STATS, we simply
	// obtain the same STREAM_STATS* from RTSPProtocol via SetStatistics().
	// this could save us on both the memory and several ms during the startup
/*
	if (!m_pRegistry)
	{
	    return HXR_FAIL;
	}

	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();
	while (pStreamData)
	{
	    if (pStreamData->m_streamNumber == (UINT16)ulStreamNumber)
	    {
		HX_DELETE(pStreamData->m_pStreamStats);
		pStreamData->m_pStreamStats = new STREAM_STATS(m_pRegistry,
							       ulRegistryID);
		break;
	    }							   
	    pStreamData = m_pStreamHandler->nextStreamData();
	}
*/
    }

    return HXR_OK;
}

RTSPTransportBuffer*
RTSPTransport::getTransportBuffer(UINT16 uStreamNumber)
{
    if (!m_pStreamHandler)
    {
	return NULL;
    }

    RTSPStreamData* pStreamData;	    

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);

    if (!pStreamData)
    {
	return NULL;
    }

    return pStreamData->m_pTransportBuffer;
}

HX_RESULT
RTSPTransport::playReset()
{
    m_bSourceDone = FALSE;

    if(m_pStreamHandler)
    {
	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

	while(pStreamData)
	{
	    if (m_bIsSource)
	    {
		pStreamData->m_packetSent = FALSE;
	    }
	    else
	    {
		pStreamData->m_pTransportBuffer->Reset();
		pStreamData->m_bReceivedAllPackets = FALSE;
	    }

	    pStreamData = m_pStreamHandler->nextStreamData();
	}
    }
    return HXR_OK;
}

HX_RESULT
RTSPTransport::pauseBuffers()
{
    if (m_pStreamHandler)
    {
	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

	ASSERT(pStreamData);

	while(pStreamData)
	{
	    pStreamData->m_pTransportBuffer->Pause();

	    pStreamData = m_pStreamHandler->nextStreamData();
	}
    }

    return HXR_OK;
}

HX_RESULT
RTSPTransport::resumeBuffers()
{
    m_bIsReceivedData = FALSE;

    if (m_pStreamHandler)
    {
	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

	ASSERT(pStreamData);

	while(pStreamData)
	{
	    pStreamData->m_pTransportBuffer->Resume();
	    pStreamData = m_pStreamHandler->nextStreamData();
	}
    }

    return HXR_OK;
}

HX_RESULT
RTSPTransport::setFirstSeqNum(UINT16 uStreamNumber, UINT16 uSeqNum, HXBOOL bOnPauseResume)
{
    RTSPStreamData* pStreamData;

    HXLOGL3(HXLOG_RTSP, "RTSPTransport[%p]::setFirstSeqNum(uStreamNumber=%hu, uSeqNum=%hu)", 
	    this, 
	    uStreamNumber, 
	    uSeqNum);

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);

    if(pStreamData)
    {
	if (m_bIsSource)
	{	    
	    pStreamData->m_firstSeqNo = pStreamData->m_seqNo = uSeqNum;

	    if(pStreamData->m_pResendBuffer)
	    {
		pStreamData->m_pResendBuffer->SetFirstSequenceNumber(uSeqNum);
	    }
	}
	else
	{
	    if (!pStreamData->m_pTransportBuffer)
	    {
		return HXR_FAIL;
	    }

	    if (!m_bMulticast)
	    {
		pStreamData->m_lastSeqNo = uSeqNum;
    		pStreamData->m_pTransportBuffer->Init(uSeqNum, bOnPauseResume);
	    }
	}
    }

    return HXR_OK;
}

HX_RESULT
RTSPTransport::Init(IUnknown* pContext)
{
    HX_RESULT			hresult;
    
    if (!m_pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }

    hresult = pContext->QueryInterface(IID_IHXCommonClassFactory,
				       (void**)&m_pCommonClassFactory);

    if (HXR_OK != hresult)
    {
	return hresult;
    }


    IHXThreadSafeScheduler* pIScheduler;
    if (SUCCEEDED(pContext->QueryInterface(IID_IHXThreadSafeScheduler,
                                       (void**)&pIScheduler)))
    {
        hresult = pIScheduler->QueryInterface(IID_IHXScheduler,
                                           (void**)&m_pScheduler);
        HX_RELEASE(pIScheduler);
    }
    else
    {
        hresult = pContext->QueryInterface(IID_IHXScheduler,
                                           (void**)&m_pScheduler);

    }
    if (HXR_OK != hresult)
    {
	return hresult;
    }

    pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);

    pContext->QueryInterface(IID_IHXInternalReset,
	                               (void**)&m_pInternalReset);

    pContext->QueryInterface(IID_IHXPlayerState,
	                               (void**)&m_pPlayerState);

    pContext->QueryInterface(IID_IHXSourceBufferingStats3,
			     (void**)&m_pSrcBufferStats);

    pContext->QueryInterface(IID_IHXSourceLatencyStats,
                             (void**)&m_pSourceLatencyStats);

#ifdef RDT_MESSAGE_DEBUG
    IHXPreferences* pPreferences = NULL;
    
    if (pContext &&
	(HXR_OK == pContext->QueryInterface(IID_IHXPreferences,
                                            (void**) &pPreferences)))
    {
	IHXBuffer* pBuffer = NULL;

        ReadPrefBOOL(pPreferences, "RDTMessageDebug", m_bRDTMessageDebug);
	if (m_bRDTMessageDebug)
	{
	    if (HXR_OK == pPreferences->ReadPref("RDTMessageDebugFile", pBuffer))
	    {
		if (pBuffer->GetSize() <= 0)
		{
		    // no file name, no log
		    m_bRDTMessageDebug = FALSE;
		}
		else
		{
		    m_RDTmessageDebugFileName = (const char*) pBuffer->GetBuffer();
		}			
	    }
            HX_RELEASE(pBuffer);
	}
    }

    HX_RELEASE(pPreferences);
#endif	// RDT_MESSAGE_DEBUG
    
    return HXR_OK;
}

HX_RESULT
RTSPTransport::SetResendBufferDepth(UINT32 uMilliseconds)
{
    RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

    ASSERT(pStreamData);

    while (pStreamData)
    {
	if (m_bIsSource)
	{
	    if (pStreamData->m_pResendBuffer)
	    {
		pStreamData->m_pResendBuffer->SetBufferDepth(uMilliseconds);
	    }
	}
	else
	{
	    if (pStreamData->m_pTransportBuffer)
	    {
		pStreamData->m_pTransportBuffer->SetBufferDepth(uMilliseconds);
	    }
	}

	pStreamData = m_pStreamHandler->nextStreamData();
    }

    return HXR_OK;
}

HX_RESULT 
RTSPTransport::SetResendBufferParameters(UINT32 uMinimumDelay,             /* ms */
                                         UINT32 uMaximumDelay,             /* ms */
                                         UINT32 uExtraDelayDuringBuffering /* ms */)
{
    RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

    ASSERT(pStreamData);

    while (pStreamData)
    {
	if (!m_bIsSource && pStreamData->m_pTransportBuffer)
        {
            pStreamData->m_pTransportBuffer->SetBufferParameters(uMinimumDelay,
                                                                 uMaximumDelay,
                                                                 uExtraDelayDuringBuffering);
	}

	pStreamData = m_pStreamHandler->nextStreamData();
    }

    return HXR_OK;
}

#ifdef RDT_MESSAGE_DEBUG
void RTSPTransport::RDTmessageFormatDebugFileOut(const char* fmt, ...)
{
    if(m_bRDTMessageDebug)
    {
        char buf[4096]; /* Flawfinder: ignore */
	va_list args;
	va_start(args, fmt);
	FILE* fp = fopen((const char*)m_RDTmessageDebugFileName, "a");
	if (fp)
	{
	    vsprintf(buf, fmt, args);
	    fprintf(fp, "%s\n", buf);
	    fclose(fp);
	}

	va_end(args);
    }
}
#endif	// RDT_MESSAGE_DEBUG

HX_RESULT 
RTSPTransport::handleNoTransBufList(RTSPTransportBuffer* pTransportBuffer)
{
    HX_RESULT res = HXR_OK;

    while ((HXR_OK == res) && !m_pNoTransBufPktList->IsEmpty())
    {
        ClientPacket* pTmpClientPkt = 
            (ClientPacket*)m_pNoTransBufPktList->RemoveHead();
        
        // Set the start time now that we have a 
        // transport buffer
        pTmpClientPkt->SetStartTime(pTransportBuffer->GetTime());
        
        // Store the packet
        res = storeClientPacket(pTransportBuffer, pTmpClientPkt);
        
        HX_RELEASE(pTmpClientPkt);
    }
    
    if (HXR_OK == res)
    {
        // Destroy the packet list now that it is
        // empty and we already have a transport 
        // buffer
        destroyPktList(m_pNoTransBufPktList);
    }

    return res;
}

void RTSPTransport::destroyPktList(REF(CHXSimpleList*) pList)
{
    if (pList)
    {
        while(!pList->IsEmpty())
        {
            ClientPacket* pPkt = 
                (ClientPacket*)pList->RemoveTail();

            HX_RELEASE(pPkt);
        }

        HX_DELETE(pList);
    }
}

HX_RESULT 
RTSPTransport::SetProbingPacketsRequested(UINT8 nPacketsRequested)
{
    HX_RESULT res = HXR_FAILED;

    destroyABDPktInfo();

    m_pABDProbPktInfo = new ABD_PROBPKT_INFO*[nPacketsRequested];

    if (m_pABDProbPktInfo)
    {
        memset(m_pABDProbPktInfo, 0, 
               sizeof(ABD_PROBPKT_INFO*) * nPacketsRequested);

        m_nProbingPacketsRequested = nPacketsRequested;
        m_nProbingPacketsReceived = 0;

        res = HXR_OK;
    }
    else
    {
        res = HXR_OUTOFMEMORY;
    }

    return res;
}

void RTSPTransport::destroyABDPktInfo()
{
    if (m_pABDProbPktInfo)
    {
        for (int i = 0; i < m_nProbingPacketsRequested; i++)
        {
            HX_DELETE(m_pABDProbPktInfo[i]);
        }
        HX_VECTOR_DELETE(m_pABDProbPktInfo);
    }

    m_nProbingPacketsRequested = 0;
    m_nProbingPacketsReceived = 0;
}

HX_RESULT 
RTSPTransport::handleABDPktInfo(TransportMode mode, UINT32 uSeq, 
                                UINT32 uSendTime, UINT32 uRecvTime, 
                                UINT32 uPktSize)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pABDProbPktInfo && 
        (m_nProbingPacketsReceived < m_nProbingPacketsRequested))
    {
        m_pABDProbPktInfo[m_nProbingPacketsReceived] = new ABD_PROBPKT_INFO;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->seq = (UINT8)uSeq;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->sendTime = uSendTime;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->recvTime = uRecvTime;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->dwSize = uPktSize;

        m_nProbingPacketsReceived++;

        // XXX HP still need to handle loss case
        if (m_nProbingPacketsReceived == m_nProbingPacketsRequested)
        {
            double dwResult = CalculateABD(1, mode, 
                                           &m_pABDProbPktInfo[0], 
                                           m_nProbingPacketsReceived);
            
            destroyABDPktInfo();

            IHXAutoBWDetectionAdviseSink* pSink = NULL;
            if (HXR_OK == m_pResp->QueryInterface(IID_IHXAutoBWDetectionAdviseSink, (void**)&pSink))
            {
                pSink->AutoBWDetectionDone(HXR_OK, (UINT32)dwResult);
            }
            else
            {
                // the owner has to support IHXAutoBWDetectionAdviseSink when
                // it initiates ABD
                HX_ASSERT(FALSE);
            }
            HX_RELEASE(pSink);
        }
    }

    return res;
}
