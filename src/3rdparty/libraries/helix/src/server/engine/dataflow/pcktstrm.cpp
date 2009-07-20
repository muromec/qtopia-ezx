/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pcktstrm.cpp,v 1.25 2007/05/23 19:03:42 seansmith Exp $
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
#include "servbuffer.h"
#include "hxpcktflwctrl.h"
#include "hxqossess.h"
#include "pcktflowwrap.h"
#include "basicpcktflow.h"
#include "pullpcktflow.h"
#include "pcktstrm.h"
#include "pcktflowmgr.h"
#include "sink.h"
#include "hxservpause.h"
#include "hxqossig.h"

PacketStream::PacketStream()
    : m_pPauseAdvise (NULL),
	m_enumStreamAdaptScheme(ADAPTATION_NONE),
	m_pStreamAdaptParams(NULL),
	m_pLinkCharParams(NULL)
{
    m_ulAvgBitRate	    	    = 0;
    m_ulVBRAvgBitRate	    	    = 0;
    m_ulMaxBitRate                  = 0;
    m_ulMinBitRate                  = 0;
    m_ulAvgPktSz                    = 0;
    m_ulPreroll                     = 0;
    m_bufMimeType                   = NULL; 
    m_pRules		    	    = 0;
    m_lNumRules			    = 0;
    m_unSequenceNumber	    	    = 0;
    m_unReliableSeqNo	    	    = 0;
    m_lBytesDueTimes10	    	    = 0;
    m_pTransport		    = 0;
    m_pFlow		    	    = NULL;
    m_pRVDrop		    	    = 0;
    m_ulPacketsOutstanding	    = 0;
    m_unStreamNumber                = 0xFFFF;
    m_uStreamGroupNumber            = 0xFFFF;
    m_unDefaultRuleNum              = INVALID_RULE_NUM;

    m_ulLastTSDTS			= 0;
    m_ulTSDMark			= 0;
    m_ulLastScaledPacketTime			= 0;

    m_pStreamDoneCallback 	    		= NULL;
    m_uScheduledStreamDoneID	    		= 0;

    m_bPacketRequested	    	    = FALSE;
    m_bStreamDone	    	    = FALSE;
    m_bSentStreamDone	    	    = FALSE;
    m_bStreamDonePending	    = FALSE;
    m_ulRatio                       = 0;
    m_bWouldBlocking		    = FALSE;
    m_bSetEncoderOffset             = TRUE;
    m_bGotSubscribe = FALSE;

    m_unMeterQueueHead  = 0;
    m_unMeterQueueTail  = 0;
    m_ulMeterCallbackID = 0;
    m_pMeterCallback    = NULL;
    m_fPktSizeEst       = 0;
    memset((void*)m_pMeterQueue, 0, sizeof(ServerPacket*) * MAX_METER_QUEUE);

    m_ulEncoderTimeMinusPlayerTimeOffset = 0L;
    m_ulFirstPacketTS = 0;
    m_bFirstPacketTSSet = FALSE;
    m_pSink = 0;
    m_pTimeStampCallback = NULL;
    m_uTimeStampScheduledSendID = 0;
    m_pTSConverter = NULL;

    m_bStreamRegistered = FALSE;

    // rate signal starts off with INIT_MEDIA_RATE
    m_mediaRateSignal = MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                              HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                              HX_QOS_SIGNAL_COMMON_INIT_MEDIA_RATE);
}

PacketStream::~PacketStream()
{
    delete[] m_pRules;

    HX_RELEASE(m_bufMimeType);
    if (m_uTimeStampScheduledSendID)
    {
        m_pFlow->m_pProc->pc->engine->
	    ischedule.remove(m_uTimeStampScheduledSendID);
	m_uTimeStampScheduledSendID=0;
    }

    HX_RELEASE(m_pTimeStampCallback);

    if (m_uScheduledStreamDoneID)
    {
        m_pFlow->m_pProc->pc->engine->
	    schedule.remove(m_uScheduledStreamDoneID);
    }

    if (m_ulMeterCallbackID)
    {
        m_pFlow->m_pProc->pc->engine->
	    ischedule.remove(m_ulMeterCallbackID);
	m_ulMeterCallbackID=0;
    }
    HX_RELEASE(m_pMeterCallback);

    for (UINT16 i = 0; i < MAX_METER_QUEUE; i++)
    {
	HX_RELEASE(m_pMeterQueue[i]);
    }

    HX_RELEASE(m_pStreamDoneCallback);
    HX_RELEASE(m_pTransport);
    HX_RELEASE(m_pPauseAdvise);
    HX_RELEASE(m_pSink);

    HX_DELETE(m_pTSConverter);
    HX_DELETE(m_pRVDrop);

    HX_DELETE(m_pStreamAdaptParams);
    HX_DELETE(m_pLinkCharParams);
}

void
PacketStream::SetFlow(BasicPacketFlow* pFlow)
{
    // Should only be done during initialization
    ASSERT(pFlow != NULL);
    m_pFlow = pFlow;
}

void
PacketStream::CreateMeterCallback()
{
    m_pMeterCallback = new MeterCallback;
    m_pMeterCallback->m_pSD = this;
    m_pMeterCallback->AddRef();
}

void
PacketStream::CreateTSCallback(PullPacketFlow* pFlow, UINT16 unStream)
{
    m_pTimeStampCallback = new PacketFlowTimeStampCallback();
    m_pTimeStampCallback->AddRef();
    m_pTimeStampCallback->m_pFlow = pFlow;
    m_pTimeStampCallback->m_unStreamNumber = unStream;
    m_uTimeStampScheduledSendID	= 0;
}

BOOL
PacketStream::IsStreamDone()
{
    return (m_pPackets.IsEmpty() && m_bStreamDone && (!m_bSentStreamDone));
}

BOOL
PacketStream::IsDependOk(BOOL bOn, UINT16 unRule)
{
    UINT16* pDeps = bOn ? m_pRules[unRule].m_pOnDepends
		        : m_pRules[unRule].m_pOffDepends;

    if (!pDeps)
	return TRUE;

    for (; *pDeps != 0xffff; pDeps++)
    {
	if (m_pRules[*pDeps].m_PendingAction != NONE)
	{
	    return FALSE;
	}
	if (m_pRules[*pDeps].m_bRuleOn != bOn)
	{
	    return FALSE;
	}
    }

    return TRUE;
}

void
PacketStream::Reset()
{
    m_pPackets.Clear();
    m_ulPacketsOutstanding = 0;
    m_bPacketRequested = FALSE;
    m_bSetEncoderOffset = TRUE;
    m_ulEncoderTimeMinusPlayerTimeOffset = 0;
    m_tLastScheduledTime = m_pFlow->m_tTimeLineStart;
    m_ulTSDMark = 0;
    if (m_uTimeStampScheduledSendID)
    {
	m_pFlow->m_pProc->pc->engine->ischedule.remove(
	    m_uTimeStampScheduledSendID);
	m_uTimeStampScheduledSendID = 0;
    }
}

void 
PacketStream::SetFirstPacketTS(UINT32 ulTimeStamp)
{
    m_ulFirstPacketTS = ulTimeStamp;
    m_bFirstPacketTSSet = TRUE;
}

BOOL 
PacketStream::IsFirstPacketTSSet()
{
    return (m_bFirstPacketTSSet);
}

PacketStream::RuleInfo::RuleInfo()
    : m_ulPriority(5)
    , m_ulAvgBitRate(0)
    , m_bBitRateReported(FALSE)
    , m_bTimeStampDelivery(FALSE)
    , m_pBWCalculator(NULL)
    , m_bWaitForSwitchOffFlag(TRUE)
    , m_bRuleOn(FALSE)
    , m_bSyncOk(TRUE)
    , m_PendingAction(NONE)
    , m_pOnDepends(0)
    , m_pOffDepends(0)
    , m_bActivateOnSeek(FALSE)
{
}

PacketStream::RuleInfo::~RuleInfo()
{
    HX_RELEASE(m_pBWCalculator);
    delete[] m_pOnDepends;
    delete[] m_pOffDepends;
}

PacketStream::Packets::Packets()
{
    m_ulPacketRingReaderPos = 0;
    m_ulPacketRingWriterPos = 0;
}

PacketStream::Packets::~Packets()
{
    Clear();
}

void
PacketStream::Packets::Clear()
{
    while (m_ulPacketRingReaderPos != m_ulPacketRingWriterPos)
    {
	HX_RELEASE(m_pPacketRing[m_ulPacketRingReaderPos]);
	Inc(m_ulPacketRingReaderPos);
    }
    m_ulPacketRingReaderPos = 0;
    m_ulPacketRingWriterPos = 0;
}

ServerPacket*
PacketStream::Packets::GetPacket()
{
    if (m_ulPacketRingReaderPos == m_ulPacketRingWriterPos)
    {
	return 0;
    }

    ServerPacket* pRet = m_pPacketRing[m_ulPacketRingReaderPos];
    // One AddRef for the outgoing packet.
    // One Release because we don't care about this packet anymore.

    Inc(m_ulPacketRingReaderPos);

    ASSERT(pRet);

    return pRet;
}

ServerPacket*
PacketStream::Packets::PeekPacket()
{
    /* This function violates COM Reference Rules */

    if (m_ulPacketRingReaderPos == m_ulPacketRingWriterPos)
    {
	return 0;
    }

    ServerPacket* pRet = m_pPacketRing[m_ulPacketRingReaderPos];

    ASSERT(pRet);

    return pRet;
}

HX_RESULT
PacketStream::Packets::PutPacket(ServerPacket* pPacket)
{
    UINT32 ulTemp = m_ulPacketRingWriterPos;

    Inc(ulTemp);

    if (ulTemp == m_ulPacketRingReaderPos)
    {
	ASSERT(0);
	return HXR_BUFFERTOOSMALL;
    }

    ASSERT(pPacket);
    m_pPacketRing[m_ulPacketRingWriterPos] = pPacket;
    pPacket->AddRef();

    m_ulPacketRingWriterPos = ulTemp;

    return HXR_OK;
}

BOOL
PacketStream::Packets::IsEmpty()
{
    return (m_ulPacketRingReaderPos == m_ulPacketRingWriterPos);
}

STDMETHODIMP
PacketStream::MeterCallback::Func()
{
    HX_ASSERT(m_pSD);
    m_pSD->MeterCallbackFunc();
    return HXR_OK;
}

void
PacketStream::MeterCallbackFunc()
{
    m_ulMeterCallbackID = 0;

    m_pFlow->TransmitPacket(m_pMeterQueue[m_unMeterQueueHead]);
    HX_RELEASE(m_pMeterQueue[m_unMeterQueueHead]);
    
    if (++m_unMeterQueueHead >= MAX_METER_QUEUE)
    {
	 m_unMeterQueueHead = 0;
    }
    
    if (m_pMeterQueue[m_unMeterQueueHead])
    {
	HX_ASSERT(m_pFlow->m_fDeliveryRatio != 0.0);

	if (m_ulVBRAvgBitRate)
	{
	    m_ulMeterCallbackID = m_pFlow->m_pProc->pc->engine->ischedule.enter
		(m_pFlow->m_pProc->pc->engine->now + 
		 Timeval(m_fPktSizeEst/(m_ulVBRAvgBitRate*m_pFlow->m_fDeliveryRatio)), 
		 m_pMeterCallback);
	}
	else if (m_ulAvgBitRate)
	{
	    m_ulMeterCallbackID = m_pFlow->m_pProc->pc->engine->ischedule.enter
		(m_pFlow->m_pProc->pc->engine->now + 
		 Timeval(m_fPktSizeEst/(m_ulAvgBitRate*m_pFlow->m_fDeliveryRatio)), 
		 m_pMeterCallback);
	}
	else
	{
	     m_ulMeterCallbackID = m_pFlow->m_pProc->pc->engine->ischedule.enter
		 (m_pFlow->m_pProc->pc->engine->now, 
		  m_pMeterCallback);
	}
    }
}

void
PacketStream::CommitPendingBandwidth()
{
    for (INT32 m = 0; m < m_lNumRules; m++)
    {
        if (m_pRules[m].m_pBWCalculator)
            m_pRules[m].m_pBWCalculator->CommitPendingBandwidth();
    }
}

void
PacketStream::HandleLiveResume()
{
    // A live pause/resume requires a flush
    Reset();

    for (INT32 i = 0; i < m_lNumRules; i++)
    {
        m_pRules[i].m_bSyncOk = FALSE;
        switch (m_pRules[i].m_PendingAction)
        {
        case NONE:
        case ACTION_ON:
        case SYNC:
            m_pRules[i].m_PendingAction = SYNC;
            break;
        default:
            m_pRules[i].m_PendingAction = NONE;
            break;
        }
    }
}

HX_RESULT
PacketStream::Register(Transport* pTransport,
                       ASMRuleBook* pRuleBook,
                       IHXValues* pHeader,
                       UINT16 uStreamNumber,
                       BOOL bIsLive,
                       UINT16 uStreamGroupNumber)
{
    m_bStreamRegistered = TRUE;

    m_pTransport = pTransport;
    m_bSupportsPacketAggregation = pTransport->SupportsPacketAggregation();
    m_bNullSetup = pTransport->isNullSetup();
    m_pTransport->AddRef();
    m_unStreamNumber = uStreamNumber;
    m_uStreamGroupNumber = uStreamGroupNumber;

    // See if transport has a pause advise

    m_pTransport->QueryInterface(IID_IHXServerPauseAdvise, 
                                 (void**) &m_pPauseAdvise);

    UINT32 ulTemp = 0;
    UINT32 ulPktSize = 0;

    if (pHeader)
    {
	UINT32 sampleRate = 0;
	UINT32 RTPFactor = 0;
	UINT32 HXFactor = 0;

	pHeader->GetPropertyULONG32("AvgBitRate", ulTemp);
	m_ulAvgBitRate = ulTemp;
	ulTemp = 0;

	pHeader->GetPropertyULONG32("MaxBitRate", ulTemp);
	m_ulMaxBitRate = ulTemp;
	ulTemp = 0;
	
	pHeader->GetPropertyULONG32("MinBitRate", ulTemp);
	m_ulMinBitRate = ulTemp; 
	ulTemp = 0;

	pHeader->GetPropertyULONG32("AvgPacketSize", ulTemp);
	m_ulAvgPktSz = ulTemp; 
	ulTemp = 0;

        pHeader->GetPropertyULONG32("ServerPreroll", ulTemp);
        m_ulPreroll = ulTemp;
        ulTemp = 0;

        if (SUCCEEDED(pHeader->GetPropertyULONG32("BaseRule", ulTemp)))
        {
            m_unDefaultRuleNum = (UINT16)ulTemp;
        }

        pHeader->GetPropertyCString("MimeType", m_bufMimeType);

	/* Set up time stamp converter for rtp-info header handling: */
	pHeader->GetPropertyULONG32("SamplesPerSecond", sampleRate);
	pHeader->GetPropertyULONG32("RTPTimestampConversionFactor",
				    RTPFactor);
	pHeader->GetPropertyULONG32("HXTimestampConversionFactor",
				    HXFactor);

	HX_DELETE(m_pTSConverter);

	if (HXFactor && RTPFactor)
	{
	    m_pTSConverter = 
		new CHXTimestampConverter(CHXTimestampConverter::FACTORS,
					  HXFactor,
					  RTPFactor);
	}
	else if (sampleRate)
	{
	    m_pTSConverter =  
		new CHXTimestampConverter(CHXTimestampConverter::SAMPLES,
					  sampleRate);
	}
    }

    if (!pRuleBook)
    {
	pHeader->GetPropertyULONG32("AvgBitRate", ulTemp);

	m_pRules = new PacketStream::RuleInfo[1];
	m_lNumRules = 1;
	m_pRules[0].m_ulPriority = 5;
	m_pRules[0].m_ulAvgBitRate = ulTemp;
    }
    else
    {
	UINT16 i;
	int nNumRules = pRuleBook->GetNumRules();
	m_pRules = new PacketStream::RuleInfo[nNumRules];

	m_lNumRules = pRuleBook->GetNumRules();

	for (i = 0; i < pRuleBook->GetNumRules(); i++)
	{
            // If this is a live stream, set the pending action to SYNC so
            // that the stream will start at the beginning of a keyframe
	    if (bIsLive)
	    {
		m_pRules[i].m_bSyncOk = FALSE;
		m_pRules[i].m_PendingAction = SYNC;
	    }

	    IHXValues* pRuleProps;
	    pRuleBook->GetProperties(i, pRuleProps);

	    IHXBuffer* pBuffer = 0;
	    pRuleProps->GetPropertyCString("Priority", pBuffer);
	    if (pBuffer)
	    {
		m_pRules[i].m_ulPriority =
                    atoi((const char *)pBuffer->GetBuffer());
		pBuffer->Release();
	    }

	    pBuffer = 0;
	    pRuleProps->GetPropertyCString("TimeStampDelivery", pBuffer);
	    if (pBuffer)
	    {
		m_pRules[i].m_bTimeStampDelivery =
		    (pBuffer->GetBuffer()[0] == 'T') ||
		    (pBuffer->GetBuffer()[0] == 't');
		pBuffer->Release();
	    }

	    pBuffer = 0;
	    pRuleProps->GetPropertyCString("WaitForSwitchOff", pBuffer);
	    if (pBuffer)
	    {
		m_pRules[i].m_bWaitForSwitchOffFlag = TRUE;
		    (pBuffer->GetBuffer()[0] == 'F') ||
		    (pBuffer->GetBuffer()[0] == 'f');
		pBuffer->Release();
	    }

	    pBuffer = 0;
	    pRuleProps->GetPropertyCString("OnDepend", pBuffer);
	    if (pBuffer)
	    {
		UINT16 pOnDeps[4096] = { 0 };
		UINT16 ulNum = 0;
		const UINT8*  pTemp = pBuffer->GetBuffer();

		while (*pTemp)
		{
		    if (*pTemp == ',')
		    {
			ulNum++;
			pOnDeps[ulNum] = 0;
		    }
		    else
		    {
			if ((*pTemp >= '0') && (*pTemp <= '9'))
			{
			    pOnDeps[ulNum] *= 10;
			    pOnDeps[ulNum] += *pTemp - '0';
			}
		    }

		    pTemp++;
		}
		ulNum++;

		m_pRules[i].m_pOnDepends = new UINT16[ulNum + 1];
		memcpy(m_pRules[i].m_pOnDepends, pOnDeps,
		    (ulNum + 1) * sizeof(UINT16));

		m_pRules[i].m_pOnDepends[ulNum] = 0xffff;

		pBuffer->Release();
	    }

	    pBuffer = 0;
	    pRuleProps->GetPropertyCString("OffDepend", pBuffer);
	    if (pBuffer)
	    {
		UINT16 pOffDeps[4096] = { 0 };
		UINT16 ulNum = 0;
		const UINT8*  pTemp = pBuffer->GetBuffer();

		while (*pTemp)
		{
		    if (*pTemp == ',')
		    {
			ulNum++;
			pOffDeps[ulNum] = 0;
		    }
		    else
		    {
			if ((*pTemp >= '0') && (*pTemp <= '9'))
			{
			    pOffDeps[ulNum] *= 10;
			    pOffDeps[ulNum] += *pTemp - '0';
			}
		    }

		    pTemp++;
		}
		ulNum++;

		m_pRules[i].m_pOffDepends = new UINT16[ulNum + 1];
		memcpy(m_pRules[i].m_pOffDepends, pOffDeps,
		    (ulNum + 1) * sizeof(UINT16));

		m_pRules[i].m_pOffDepends[ulNum] = 0xffff;

		pBuffer->Release();
	    }

	    pBuffer = 0;
	    pRuleProps->GetPropertyCString("AverageBandwidth", pBuffer);

	    if (pBuffer)
	    {
		m_pRules[i].m_ulAvgBitRate =
		atoi((const char *)pBuffer->GetBuffer());
		pBuffer->Release();
	    }

	    if (m_pRules[i].m_bTimeStampDelivery)
	    {
		m_pRules[i].m_pBWCalculator = 
		    new BWCalculator(m_pFlow->m_pProc, m_bNullSetup);
		m_pRules[i].m_pBWCalculator->AddRef();
	    }

	    pBuffer = 0;

	    pRuleProps->Release();
	}
    }

    return HXR_OK;
}

//
// unsubscribe to any rules marked activate on seek and then
// subscribe to them
//
BOOL
PacketStream::HandleSeekSubscribes(BOOL bInitialSubscriptionDone,
                                   BOOL bIsMulticast,
                                   REF(UINT32) ulDeliveryRate,
                                   PacketFlowManager* pFlowMgr)
{
    BOOL bSubscribed = FALSE;

    // unsubscribe any active rules of this stream
    for (int j = 0; j < m_lNumRules; j++)
    {
        if (!m_pRules[j].m_bActivateOnSeek)
            continue;

        if (m_pRules[j].m_PendingAction == ACTION_OFF)
        {
            if (m_pRules[j].m_bBitRateReported)
            {
                ulDeliveryRate -= m_pRules[j].m_ulAvgBitRate;

                INT32 lChange = (-1) * (INT32)m_pRules[j].m_ulAvgBitRate;
                ASSERT(m_bGotSubscribe);
                ChangeDeliveryBandwidth(lChange);
                pFlowMgr->ChangeDeliveryBandwidth(
                    lChange, !bIsMulticast && !m_bNullSetup);

                if (m_pRules[j].m_bTimeStampDelivery)
                {
                    m_ulVBRAvgBitRate -= m_pRules[j].m_ulAvgBitRate;
                }

                m_pRules[j].m_bBitRateReported = FALSE;
            }

            m_pRules[j].m_PendingAction = NONE;
            m_pRules[j].m_bActivateOnSeek = FALSE;
        }

        m_pRules[j].m_bActivateOnSeek = FALSE;

        INT32 ret = SubscribeRule(j,
                                  bInitialSubscriptionDone,
                                  bIsMulticast,
                                  pFlowMgr);
        if (ret >= 0)
        {
            bSubscribed = TRUE;
            ulDeliveryRate += ret;
        }
    }

    return bSubscribed;
}

INT32
PacketStream::SubscribeRule(INT32 lRuleNumber,
                            BOOL bInitialSubscriptionDone,
                            BOOL bIsMulticast,
                            PacketFlowManager* pFlowMgr)
{
    UINT32 ulDeliveryRate = 0;

    if (lRuleNumber >= m_lNumRules)
	return -1;

    m_pRules[lRuleNumber].m_bRuleOn = TRUE;

    if (bInitialSubscriptionDone)
    {
	m_pRules[lRuleNumber].m_PendingAction = ACTION_ON;
        return 0;
    }

    // Update delivery rate
    if (!m_pRules[lRuleNumber].m_bBitRateReported)
    {
        ulDeliveryRate += m_pRules[lRuleNumber].m_ulAvgBitRate;

        INT32 lChange = m_pRules[lRuleNumber].m_ulAvgBitRate;
        ChangeDeliveryBandwidth(lChange);
        pFlowMgr->ChangeDeliveryBandwidth(
            lChange, !bIsMulticast && !m_bNullSetup);
/*
        if (m_pRules[lRuleNumber].m_bTimeStampDelivery)
        {
            m_ulVBRAvgBitRate += m_pRules[lRuleNumber].m_ulAvgBitRate;
        }
*/
        if (!m_bGotSubscribe)
            m_bGotSubscribe = TRUE;

        m_pRules[lRuleNumber].m_bBitRateReported = TRUE;
    }

    return ulDeliveryRate;
}

BOOL
PacketStream::Activate(BOOL bReset,
                       BOOL bInitialSubscriptionDone,
                       BOOL bIsMulticast,
                       REF(UINT32) ulDeliveryRate,
                       PacketFlowManager* pFlowMgr)
{
    if (m_bSentStreamDone)
        m_pFlow->HandleUnStreamDone(this);

    m_bStreamDonePending = FALSE;
    m_bStreamDone = FALSE;
    m_bSentStreamDone = FALSE;	

    if (bReset)
        Reset();

    return HandleSeekSubscribes(bInitialSubscriptionDone,
                                bIsMulticast,
                                ulDeliveryRate,
                                pFlowMgr);
}

void
PacketStream::ChangeDeliveryBandwidth(INT32 lChange)
{
    if (!m_bGotSubscribe)
        m_ulAvgBitRate = lChange;
    else
        m_ulAvgBitRate += lChange;
}

HX_RESULT
PacketStream::SetStreamAdaptation (StreamAdaptationSchemeEnum enumAdaptScheme,
							StreamAdaptationParams* pStreamAdaptParams)
{
	m_enumStreamAdaptScheme = enumAdaptScheme;

	if (!m_pStreamAdaptParams)
	{
		m_pStreamAdaptParams = new StreamAdaptationParams;
		if (!m_pStreamAdaptParams)
		{
			return HXR_OUTOFMEMORY;
		}
	}

	*m_pStreamAdaptParams = *pStreamAdaptParams;

	return HXR_OK;
}

