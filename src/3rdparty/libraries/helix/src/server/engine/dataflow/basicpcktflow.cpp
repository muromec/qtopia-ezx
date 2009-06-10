/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: basicpcktflow.cpp,v 1.43 2009/02/05 22:22:55 dcollins Exp $
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
#include "server_stats.h"
#include "server_context.h"

#include "dtcvtcon.h"
#include "mem_cache.h"
#include "hxpiids.h"
#include "globals.h"

#include "servbuffer.h"

#include "hxpcktflwctrl.h"
#include "hxqossess.h"
#include "hxqostran.h"
#include "pcktflowmgr.h"
#include "basicpcktflow.h"
#include "pushpacketflow.h"
#include "pullpcktflow.h"
#include "pcktstrm.h"
#include "pcktflowwrap.h"

#define PACKET_QUEUE_SIZE 12
#define FILTER_CONSTANT   0.01

BasicPacketFlow*
BasicPacketFlow::Create(Process* p,
                        IHXSessionStats* pSessionStats,
                        UINT16 unStreamCount,
                        PacketFlowManager* pFlowMgr,
                        IHXPSourcePackets* pSourcePackets,
                        BOOL bIsMulticast)
{
    return new PullPacketFlow(p, pSessionStats, unStreamCount, pFlowMgr,
                              pSourcePackets, bIsMulticast);
}

BasicPacketFlow*
BasicPacketFlow::Create(Process* p,
                        IHXSessionStats* pSessionStats,
			UINT16 unStreamCount,
			PacketFlowManager* pFlowMgr,
			IHXServerPacketSource* pSource,
			BOOL bIsMulticast,
			BOOL bIsLive,
            BOOL bIsFCS)
{
    return new PushPacketFlow(p, pSessionStats, unStreamCount, pFlowMgr,
				    pSource, bIsMulticast, bIsLive, bIsFCS);
}

BasicPacketFlow::BasicPacketFlow(Process* p,
                                 IHXSessionStats* pSessionStats,
                                 UINT16 unStreamCount,
                                 PacketFlowManager* pFlowMgr,
                                 BOOL bIsMulticast)
    : m_tvBankedPlayTime(0,0)
{
    m_pProc 		 	= p;
    m_pFlowMgr 		 	= pFlowMgr;
    m_unStreamCount 	 	= unStreamCount;
    m_uNumStreamsRegistered	= 0;
    m_ulRefCount 	 	= 0;
    m_bPaused		 	= FALSE;
    m_bInSeek		 	= FALSE;
    m_bInitialSubscriptionDone	= FALSE;
    m_bInitialPlayReceived	= FALSE;
    m_bGetPacketsOutstanding	= FALSE;
    m_tTimeLineStart.tv_sec	= 0;
    m_tTimeLineStart.tv_usec	= 0;
    m_bTimeLineSuspended	= TRUE;
    m_bIsMulticast		= bIsMulticast;
    m_ulPacketsOutstanding	= 0;
    m_fDeliveryRatio		= 1.0;
    m_bSourceIsDone		= FALSE;
    m_ulNumStreamDones		= 0;
    m_uEndPoint			= 0;
    m_bIsPausePointSet          = FALSE; 
    m_bIsDone			= FALSE;
    m_ulResendIDPosition	= 0;
    m_bSubscribed		= FALSE;
    m_bAutoSubscription		= TRUE;
    m_ulWouldBlocking		= 0;
    m_bWouldBlockAvailable      = FALSE;
    m_pConvertShim              = 0;
    m_pConvertingPacket		= NULL;
    m_bPlayPendingOnSeek	= FALSE;
    m_pClient            = NULL;
    m_pPlayerSessionId          = NULL;
    m_pRateManager              = NULL;
    m_bSeekPacketPending  = FALSE;
    m_bSessionPlaying     = FALSE;
    m_bDeliveryBandwidthSet = FALSE;
    m_pStats                    = pSessionStats;
    m_pStats->AddRef();
    m_bRTPInfoRequired          = TRUE;
    m_uFirstStreamRegistered	= 0xFFFF;

    m_ulSetDeliveryBandwidth = 0;
    m_ulBandwidth = 0;

    memset(m_pResendIDs, 0, sizeof(UINT32) * MAX_RESENDS_PER_SECOND);

    // Contorted initialization due to NT not able to intialize array
    // elements on creation even though it is standard C++
    m_pStreams = new PacketStream[unStreamCount];

    // then the intialization
    for (UINT16 iStream = 0; iStream < unStreamCount; iStream++)
	m_pStreams[iStream].SetFlow(this);

    // get clock for PlayTime tracking

    if (HXR_OK != m_pProc->pc->server_context->
        QueryInterface(IID_IHXAccurateClock, (void**) &m_pAccurateClock))
    {
        // system doesn't have accurate clock, use our own.
        AddRef();
        m_pAccurateClock = (IHXAccurateClock*) this;
    }

    // for safety, initialize this to current time, although we really
    // set it in Play().
    
    HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
    m_tvRTSPPlayTime.tv_sec = hxt.tv_sec;
    m_tvRTSPPlayTime.tv_usec = hxt.tv_usec;
}

BasicPacketFlow::~BasicPacketFlow()
{
    delete[] m_pStreams;
    HX_RELEASE(m_pStats);
}

void
BasicPacketFlow::ScheduleStreamDone(BasicPacketFlow* pFlow,
                                    Transport* pTransport, 
                                    PacketStream* pStream, 
                                    UINT16 unStreamNumber)
{
    /* 
     * Notify the protocol if we've reached the end of the stream following a
     * seek and have no packets to send.
     */
    if (!pStream->IsFirstPacketTSSet())
    {
        SetStreamStartTime(unStreamNumber, (UINT32)~0);
        pStream->SetFirstPacketTS((UINT32)~0);
    }

    if (m_pRateManager)
    {
	IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
						     (void**)&pRateMgrFlowControl))
	{
	    pRateMgrFlowControl->StreamDone(unStreamNumber);
	    HX_RELEASE(pRateMgrFlowControl);
	}    
    }

    HandleStreamDone(pStream);

    pStream->m_bSentStreamDone = TRUE;

    HX_ASSERT(!pStream->m_uScheduledStreamDoneID);

    if (pStream->m_uScheduledStreamDoneID)
    {
        return;
    }
    else if (!pStream->m_pStreamDoneCallback)
    {
        pStream->m_pStreamDoneCallback =
            new (m_pProc->pc->mem_cache) PacketStreamDoneCallback();

        pStream->m_pStreamDoneCallback->m_pFlow          = pFlow;
        pStream->m_pStreamDoneCallback->m_pTransport     = pTransport;
        pStream->m_pStreamDoneCallback->m_pStream        = pStream;
        pStream->m_pStreamDoneCallback->m_unStreamNumber = unStreamNumber;
        pStream->m_pStreamDoneCallback->AddRef();
    }

    Timeval tNow = m_pProc->pc->engine->now;
    Timeval tCallbackTime;

    pStream->m_uScheduledStreamDoneID =
        m_pProc->pc->engine->schedule.enter(tCallbackTime,
                                            pStream->m_pStreamDoneCallback);

    if (!pStream->m_uScheduledStreamDoneID)
        HX_RELEASE(pStream->m_pStreamDoneCallback);
}

void
BasicPacketFlow::HandleStreamDone(PacketStream* pStream)
{
    if (!pStream->m_bStreamRegistered)
    {
	return;
    }

    m_ulNumStreamDones++;
    ASSERT(m_ulNumStreamDones <= m_uNumStreamsRegistered);

    // enumerate the rules, subtract delivery rates that have been reported.

    for (INT32 lRule = 0; lRule < pStream->m_lNumRules; lRule++)
    {
	if (!pStream->m_pRules[lRule].m_bBitRateReported)
            continue;

        // Update delivery rate
        INT32 lChange = (-1) * (INT32)pStream->m_pRules[lRule].m_ulAvgBitRate;
        ASSERT(pStream->m_bGotSubscribe);
        pStream->ChangeDeliveryBandwidth(lChange);
        m_pFlowMgr->ChangeDeliveryBandwidth(
            lChange, !m_bIsMulticast && !pStream->m_bNullSetup);
        pStream->m_pRules[lRule].m_bBitRateReported = FALSE;
    }

    if (m_ulNumStreamDones == m_uNumStreamsRegistered)
    {
	m_bSourceIsDone = TRUE;
	m_pFlowMgr->RecalcConstantBitRate();
        /*
         * Stop accumulating PlayTime.  The Protocol won't send us a Pause()
         * even if it gets an RTSP PAUSE, because it thinks we're already 
         * paused.
         */
        Pause(FALSE, 0);
    }
}

void
BasicPacketFlow::HandleUnStreamDone(PacketStream* pStream)
{
    ASSERT(m_ulNumStreamDones);
    m_ulNumStreamDones--;

    // enumerate the rules, report delivery rates for active ones.

    for (INT32 lRule = 0; lRule < pStream->m_lNumRules; lRule++)
    {
	if (pStream->m_pRules[lRule].m_bRuleOn &&
	    !pStream->m_pRules[lRule].m_bBitRateReported)
	{
	    // Update delivery rate

            INT32 lChange = (INT32)pStream->m_pRules[lRule].m_ulAvgBitRate;
            ASSERT(pStream->m_bGotSubscribe);
            pStream->ChangeDeliveryBandwidth(lChange);
	    m_pFlowMgr->ChangeDeliveryBandwidth(
                lChange, !m_bIsMulticast && !pStream->m_bNullSetup);
	    pStream->m_pRules[lRule].m_bBitRateReported = TRUE;
	}
    }

    m_bSourceIsDone = FALSE;
    m_pFlowMgr->RecalcConstantBitRate();
    /* Unschedule any outstanding stream dones on this stream */
    if (pStream->m_uScheduledStreamDoneID)
    {
        m_pProc->pc->engine->schedule.remove(pStream->m_uScheduledStreamDoneID);
	pStream->m_uScheduledStreamDoneID = 0;
    }
}

STDMETHODIMP
BasicPacketFlow::StreamDone(UINT16 unStreamNumber, BOOL bForce /* = FALSE */)
{
    if (m_bIsDone || !m_pStreams[unStreamNumber].m_bStreamRegistered)
	return HXR_OK;

    HX_ASSERT(unStreamNumber < m_unStreamCount);
    HX_ASSERT(m_pStreams);
    
    if (m_pStreams == NULL)
	return HXR_OK;

    if (unStreamNumber >= m_unStreamCount)
	return HXR_OK;
    
    PacketStream* pStream = &m_pStreams[unStreamNumber];
    if (pStream->m_bStreamDone)
	return HXR_OK;

    pStream->m_bStreamDone = TRUE;

    ServerPacket* pPacket = pStream->m_pPackets.PeekPacket();
    
    // Don't issue a stream done here if there are any outstanding packets.
    // We will issue it after the packet is sent if we fail to send it here.
    if (bForce || ((!pPacket) && (!pStream->m_uTimeStampScheduledSendID) &&
        pStream->IsStreamDone()))
    {
	ScheduleStreamDone(this, pStream->m_pTransport, pStream, unStreamNumber);
    }

    if (m_bSourceIsDone)
    {
        // the transport should stop holding any packets its holding
	pStream->m_pSink->Flush();
    }

    return HXR_OK;
}

void
BasicPacketFlow::SetStreamStartTime(UINT32 ulStreamNum, UINT32 ulTS)
{
    IHXBuffer* pFlowId = NULL;
    
    if (m_pClient && m_pPlayerSessionId)
    {
        const char* szSessionID = (const char*)m_pPlayerSessionId->GetBuffer();
        m_pClient->SetStreamStartTime(szSessionID, ulStreamNum, ulTS);
    }
}


STDMETHODIMP
BasicPacketFlow::ControlDone()
{
    if (m_pFlowMgr) m_pFlowMgr->SessionDone(this);

    return HXR_OK;
}

void
BasicPacketFlow::Done()
{
    
    if (m_bIsDone)
        return;

    m_bIsDone = TRUE;
    ULONG32 ulSubscribedRate = 0;

    // Update the PlayTime stat
    // we come here on both TEARDOWN and client socket reset

    HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
    Timeval t((long)hxt.tv_sec, (long)hxt.tv_usec);

    if (!m_bPaused) m_tvBankedPlayTime += (t - m_tvRTSPPlayTime);
    UpdatePlayTime();

    if (m_pRateManager)
    {
	IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
						     (void**)&pRateMgrFlowControl))
	{
	    ulSubscribedRate = pRateMgrFlowControl->GetDeliveryRate();
	    HX_RELEASE(pRateMgrFlowControl);
	}
    }

    if (ulSubscribedRate)
    {
	m_pFlowMgr->ChangeDeliveryBandwidth(-1 * ulSubscribedRate,
                                            !m_bIsMulticast);

	if (m_pRateManager)
	{
	    m_pRateManager->Done();
	}
    }
    
    m_pFlowMgr = 0;

    HX_VECTOR_DELETE(m_pStreams);

    for (int j = 0; j < MAX_RESENDS_PER_SECOND; j++)
    {
	if (m_pResendIDs[j])
	{
    	    m_pProc->pc->engine->schedule.remove(m_pResendIDs[j]);
	    m_pResendIDs[j] = 0;
	}
    }

    if (m_pConvertShim)
    {
	m_pConvertShim->Done();
	HX_RELEASE(m_pConvertShim);
    }

    HX_RELEASE(m_pConvertingPacket);

    HX_RELEASE(m_pClient);
    HX_RELEASE(m_pPlayerSessionId);
    HX_RELEASE(m_pAccurateClock);
}

// True means start timeline, False means stop it
void
BasicPacketFlow::HandleTimeLineStateChange(BOOL bStart)
{
    m_tTimeLineStart = m_pProc->pc->engine->now;
    if (m_bTimeLineSuspended && bStart)
    {
	m_bTimeLineSuspended = FALSE;
	m_bRTPInfoRequired   = TRUE;
    }
    else if (!m_bTimeLineSuspended && !bStart)
    {
	m_bTimeLineSuspended = TRUE;
	for (UINT16 i = 0; i < m_unStreamCount; i++)
	{
	    if (!m_pStreams[i].m_bStreamRegistered)
	    {
		continue;
	    }
	    m_pStreams[i].m_tLastScheduledTime = m_tTimeLineStart;
	    m_pStreams[i].m_ulTSDMark = 0;
	    m_pStreams[i].m_ulLastScaledPacketTime = 
		m_pStreams[i].m_ulLastTSDTS;
		m_pStreams[i].m_ulFirstPacketTS = 0 ;
		m_pStreams[i].m_bFirstPacketTSSet = FALSE;
	}
    }

}

void
BasicPacketFlow::ResetSessionTimeline(ServerPacket* pNextPkt,
                                   UINT16 usStreamNumber,
                                   BOOL bIsPostSeekReset)
{
/* Update session timeline after a stream is paused and resumed */

    if (!m_bSessionPlaying)
    {
        m_pStreams [pNextPkt->GetStreamNumber()].m_ulLastScaledPacketTime = pNextPkt->GetTime();
        m_tTimeLineStart = m_pProc->pc->engine->now;
    }
}

STDMETHODIMP
BasicPacketFlow::Play()
{
    Play(FALSE);

    return HXR_OK;
}

void
BasicPacketFlow::Play(BOOL bWouldBlock)
{
    // If this play is not really going to unpause us, becuase the other
    // pause is still in effect, then do nothing.
    if ((bWouldBlock && m_bPaused) || (!bWouldBlock && m_ulWouldBlocking))
	return;

    BOOL bInitialPlayReceived = m_bInitialPlayReceived;
    BOOL bWasPaused           = IsPaused();

    if (m_bIsMulticast)
	return;

    if (m_bInSeek)
    {
	m_bPlayPendingOnSeek = TRUE;
	return;
    }

    HX_ASSERT(m_uFirstStreamRegistered != 0xFFFF);
    if (m_pStreams[m_uFirstStreamRegistered].m_pTransport->isNullSetup())
	return;

    if (m_bAutoSubscription && !m_bSubscribed)
    {
    	// subscribe to all rules...
    	PacketStream* pStream = 0;
    	for (UINT16 unStreamNum = 0;
             unStreamNum < m_unStreamCount;
             unStreamNum++)
    	{
    	    pStream = &m_pStreams[unStreamNum];
	    if (!pStream->m_bStreamRegistered)
	    {
		continue;
	    }
    	    for (INT32 lRuleNumber = 0;
                 lRuleNumber < pStream->m_lNumRules;
                 lRuleNumber++)
    	    {    
    		HandleSubscribe(lRuleNumber, unStreamNum);
	    }
	}	
    }

    HandleTimeLineStateChange(TRUE);
    m_bInitialSubscriptionDone = TRUE; // It better be done by now!

    if (!bWouldBlock)
    {
        // can we get multiple PLAYs ?
        if (m_bPaused || !bInitialPlayReceived)
        {
            HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
            m_tvRTSPPlayTime.tv_sec = hxt.tv_sec;
            m_tvRTSPPlayTime.tv_usec = hxt.tv_usec;
        }
	m_bPaused = FALSE;
    }

    if (bWasPaused)
    {
        HandleResume();
    }

    m_pFlowMgr->RecalcConstantBitRate();

    if (m_pRateManager)
    {
	IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
						     (void**)&pRateMgrFlowControl))
	{
	    pRateMgrFlowControl->Play();
	    HX_RELEASE(pRateMgrFlowControl);
	}
    }

    InitialStartup();
 
    if (!m_bInitialPlayReceived)
    {
	m_bInitialPlayReceived = TRUE;

	// Until now, we have not added the bandwidth for this Flow to
	// the overall total, because we didn't know if the content was
	// being served from a downstream cache. Now, we know that's not
	// the case because the RealProxy wouldn't send a play request.
	m_pFlowMgr->CommitPendingBandwidth();

	// Tell the bandwidth calculators to commit their bandwidth as well
	for (UINT16 n = 0; n < m_unStreamCount; n++)
	{
	    if (m_pStreams[n].m_bStreamRegistered)
	    {
		m_pStreams[n].CommitPendingBandwidth();
	    }
	}
    }

 
    return;
}

STDMETHODIMP
BasicPacketFlow::SetStartingTimestamp(UINT32 ulStartingTimestamp)
{
    if (m_fDeliveryRatio)
    {
        for (int i = 0; i < m_unStreamCount; ++i)
	{
	    if (m_pStreams[i].m_bStreamRegistered)
	    {
	        m_pStreams[i].m_ulTSDMark = ulStartingTimestamp;
	    }
	}
    }

    m_tTimeLineStart = m_pProc->pc->engine->now; //- (int)ulTime;
    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::SetEndPoint(UINT32 uEndPoint, BOOL bPause)
{
    m_uEndPoint = uEndPoint;
    m_bIsPausePointSet = bPause;

    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::Pause(UINT32 ulPausePoint)
{
    Pause(FALSE, ulPausePoint);

    return HXR_OK;
}

void
BasicPacketFlow::Pause(BOOL bWouldBlock, UINT32 ulPausePoint)
{
    if (ulPausePoint > 0)
    {
        SetEndPoint(ulPausePoint, TRUE);
    }
    else
    {
        if (m_bIsPausePointSet)
        {
           m_bIsPausePointSet = FALSE;
           m_uEndPoint = 0;
        }
        m_bPlayPendingOnSeek = FALSE;
        m_bSessionPlaying = FALSE;
        HandleTimeLineStateChange(FALSE);

        if (!bWouldBlock)
        {
            // can we get multiple pauses?
            if (!m_bPaused)
            {
                m_bPaused = TRUE;  // must be set before UpdatePlayTime().

                HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
                Timeval t((long)hxt.tv_sec, (long)hxt.tv_usec);

                m_tvBankedPlayTime += (t - m_tvRTSPPlayTime);
                UpdatePlayTime();
            }            
        }

	if (m_pRateManager)
	{
	    IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	    if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
							 (void**)&pRateMgrFlowControl))
	    {
		pRateMgrFlowControl->Pause(ulPausePoint);
		HX_RELEASE(pRateMgrFlowControl);
	    }
	}
    }
}

STDMETHODIMP
BasicPacketFlow::HandleSubscribe(INT32 lRuleNumber, UINT16 unStreamNumber)
{
    PacketStream* pStream = &m_pStreams[unStreamNumber];
    HX_ASSERT(pStream->m_bStreamRegistered);

    INT32 ret = pStream->SubscribeRule(lRuleNumber,
                                       m_bInitialSubscriptionDone,
                                       m_bIsMulticast,
                                       m_pFlowMgr);

    if (ret >= 0)
    {
        m_bSubscribed = TRUE;
	
	if (m_pRateManager)
	{
	    IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	    if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
							 (void**)&pRateMgrFlowControl))
	    {
		pRateMgrFlowControl->HandleSubscribe(lRuleNumber, unStreamNumber);
		HX_RELEASE(pRateMgrFlowControl);
	    }
	}
    }

    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::HandleUnSubscribe(INT32 lRuleNumber, UINT16 unStreamNumber)
{
    PacketStream* pStream = &m_pStreams[unStreamNumber];
    HX_ASSERT(pStream->m_bStreamRegistered);

    if (lRuleNumber >= pStream->m_lNumRules)
	return HXR_OK;

    pStream->m_pRules[lRuleNumber].m_bRuleOn = FALSE;

    if (!m_bInitialSubscriptionDone)
    {
        return HXR_OK;
    }

    if (pStream->m_pRules[lRuleNumber].m_bWaitForSwitchOffFlag)
    {
        pStream->m_pRules[lRuleNumber].m_PendingAction = ACTION_OFF;
    }
    else if (pStream->m_pRules[lRuleNumber].m_bBitRateReported)
    {
        
        // Update delivery rate
	if (m_pRateManager)
	{
	    IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	    if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
							 (void**)&pRateMgrFlowControl))
	    {
		pRateMgrFlowControl->HandleUnSubscribe(lRuleNumber, unStreamNumber);
		HX_RELEASE(pRateMgrFlowControl);
	    }
	}
        
	INT32 lChange = (-1) * (INT32)pStream->m_pRules[lRuleNumber].m_ulAvgBitRate;
        ASSERT(pStream->m_bGotSubscribe);
        pStream->ChangeDeliveryBandwidth(lChange);
        m_pFlowMgr->ChangeDeliveryBandwidth(
            lChange, !m_bIsMulticast && !pStream->m_bNullSetup);
        pStream->m_pRules[lRuleNumber].m_bBitRateReported = FALSE;
    }

    return HXR_OK;
}

STDMETHODIMP_(ULONG32) BasicPacketFlow:: GetDeliveryRate()
{
    ULONG32 ulSubscribedRate = 0;

    if (m_pRateManager)
    {
	IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
						     (void**)&pRateMgrFlowControl))
	{
	    ulSubscribedRate = pRateMgrFlowControl->GetDeliveryRate();
	    HX_RELEASE(pRateMgrFlowControl);
	}
    }

    return ulSubscribedRate;
}

STDMETHODIMP
BasicPacketFlow::GetSequenceNumber(UINT16 uStreamNumber, 
                                   UINT16& uSequenceNumber)
{
    PacketStream* pStream = &m_pStreams[uStreamNumber];
    ASSERT(pStream);
    if (pStream)
    {
	uSequenceNumber = pStream->m_unSequenceNumber;
	return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
BasicPacketFlow::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPSinkPackets*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkPackets))
    {
        AddRef();
        *ppvObj = (IHXPSinkPackets*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkInfo))
    {
        AddRef();
        *ppvObj = (IHXPSinkInfo*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPacketResend))
    {
	AddRef();
	*ppvObj = (IHXPacketResend*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXWouldBlockResponse))
    {
	AddRef();
	*ppvObj = (IHXWouldBlockResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAccurateClock))
    {
	AddRef();
	*ppvObj = (IHXAccurateClock*)this;
	return HXR_OK;
    }


    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
BasicPacketFlow::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) 
BasicPacketFlow::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
BasicPacketFlow::PacketReady(HX_RESULT  ulStatus, IHXPacket* pPacket)
{
    // update the PlayTime stat if not paused
    // (for some reason, we keep streaming when live pauses.)

    if (!m_bPaused) UpdatePlayTime();

    if (ulStatus == HXR_OK && m_pConvertShim)
    {
        HX_RELEASE(m_pConvertingPacket);
	m_pConvertingPacket = pPacket;
	m_pConvertingPacket->AddRef();
	m_pConvertShim->ConvertData(pPacket);

        return HXR_OK;
    }

    return SessionPacketReady(ulStatus, pPacket);
}


STDMETHODIMP_(ULONG32) 
BasicPacketFlow::GetBandwidth()
{
    UINT32 ulActualDeliveryRate = 0;

    if ((m_pRateManager) && (ulActualDeliveryRate = m_pRateManager->GetActualDeliveryRate()))
    {
	if (m_bSourceIsDone)
	    ulActualDeliveryRate = 0;
    }
    else if (m_pStreams)
    {
	for (int i = 0; i < m_unStreamCount; i++)
	{
	    PacketStream* pStream = &m_pStreams[i];

	    if (pStream->m_bStreamRegistered)
	    {
		if (pStream && !pStream->m_bSentStreamDone)
		    ulActualDeliveryRate += pStream->m_ulAvgBitRate;
	    }
	}
    }

    return ulActualDeliveryRate;
}

UINT32
BasicPacketFlow::GetConstantBitRate()
{
    ULONG32 ulSubscribedRate = 0;
    if (m_bTimeLineSuspended || m_bIsDone)
        return 0;

    if (m_pRateManager)
    {
	IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
						     (void**)&pRateMgrFlowControl))
	{
	    ulSubscribedRate = pRateMgrFlowControl->GetDeliveryRate();
	    HX_RELEASE(pRateMgrFlowControl);
	}
	
	UINT32 ulActualDeliveryRate = m_pRateManager->GetActualDeliveryRate();
    
	if (ulActualDeliveryRate)
	{
	    if (m_bSourceIsDone)
		return 0;
	    
	    if (ulSubscribedRate != 0.0)
	    {
		m_fDeliveryRatio = (float)ulActualDeliveryRate / 
		    (float)ulSubscribedRate;
	    }
	    else
	    {
		// This shouldn't really happen
		m_fDeliveryRatio = 0.0;
	    }
	    
	    return ulActualDeliveryRate;
	}
    }

    UINT32 ulConstantBitRate = 0;
    for (int j = 0; j < m_unStreamCount; j++)
    {
        PacketStream* pStream = &m_pStreams[j];

	if (!pStream->m_bStreamRegistered)
	{
	    continue;
	}

        UINT32 ulAdjustedRate = pStream->m_ulAvgBitRate;

        // If a delivery ratio has been set, adjust delivery rate
        ulAdjustedRate = (UINT32) ((float)ulAdjustedRate * m_fDeliveryRatio);

        if (!pStream->m_bSentStreamDone)
            ulConstantBitRate += ulAdjustedRate;
    }

    return ulConstantBitRate;
}

STDMETHODIMP
PacketFlowResendCallback::Func()
{
    m_pTransport->sendPacket(m_pPacket);
    *g_pBytesServed += m_pPacket->GetSize();
    (*g_pPPS)++;
    (*g_pResends)++;
    HX_RELEASE(m_pPacket);
    *m_pZeroMe = 0;

    return HXR_OK;
}

PacketFlowResendCallback::~PacketFlowResendCallback()
{
    HX_RELEASE(m_pPacket);
}

STDMETHODIMP
BasicPacketFlow::OnPacket(UINT16 uStreamNumber, BasePacket** ppPacket)
{
    if (m_bIsDone)
	return HXR_UNEXPECTED;

    PacketStream* pStream = &m_pStreams[uStreamNumber];
    HX_ASSERT(pStream->m_bStreamRegistered);

    BasePacket* pPacket;

    for (; (pPacket = *ppPacket); ppPacket++)
    {
        
        // Do not check for IsStreamDone() because this is part of the
	// resend mechanism
	if (pPacket->IsResendRequested() && pPacket->m_uPriority != 10)
            continue;

        if (!pPacket->IsResendRequested())
            pPacket->SetResendRequested();

        UINT32 ulPos = m_ulResendIDPosition + 1;
        if (ulPos == MAX_RESENDS_PER_SECOND)
            ulPos = 0;

        while (m_pResendIDs[ulPos] && ulPos != m_ulResendIDPosition)
        {
            ulPos++;
            if (ulPos == MAX_RESENDS_PER_SECOND)
                ulPos = 0;
        }

        if (m_pResendIDs[m_ulResendIDPosition = ulPos])
            continue;

        PacketFlowResendCallback* pResendCB =
            new (m_pProc->pc->mem_cache) PacketFlowResendCallback();
        pResendCB->AddRef();
        pResendCB->m_pTransport   = pStream->m_pTransport;
        pResendCB->m_pPacket      = pPacket;
        pResendCB->m_pPacket->AddRef();
        pResendCB->m_pZeroMe	  = m_pResendIDs + m_ulResendIDPosition;

        UINT32 ulMinWaitTime;
        if ((m_pRateManager) && (m_pRateManager->GetActualDeliveryRate() > 60000))
            ulMinWaitTime = 500;
        else
            ulMinWaitTime = 10;

        UINT32 ulTime = rand() % 300 + ulMinWaitTime;

//        printf("%p %p %p %p %p\n", this, m_pProc, m_pProc->pc, m_pProc->pc->engine,
//               m_pProc->pc->engine->schedule);
//        fflush(stdout);

        m_pResendIDs[m_ulResendIDPosition] =
            m_pProc->pc->engine->schedule.enter(
                m_pProc->pc->engine->now + Timeval(0, ulTime * 1000),
                pResendCB);
        pResendCB->Release();
    }

    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::SetDropRate(UINT16 uStreamNumber, UINT32 uDropRate)
{
    PacketStream* pStream = &m_pStreams[uStreamNumber];

    ASSERT(pStream->m_pRVDrop);

    /* 
     * Note: the Fiji preview client sends a drop rate of 0 and we
     * don't want to wrap around or we'll never send a packet
     * JEFFA 4/24/98
     */
    if (pStream && pStream->m_pRVDrop)
        pStream->m_pRVDrop->m_drop_rate = (uDropRate > 0) ? uDropRate - 1 : 0;

    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::SetDropToBandwidthLimit(UINT16 uStreamNumber,
                                         UINT32 ulBandwidthLimit)
{
    //XXXSMP Sujal please implement.

    return HXR_OK;
}

ServerPacket*
BasicPacketFlow::HXPacketToServerPacket(IHXPacket* pPacket)
{
    ServerPacket* pServerPacket;

    // This is a travesty of justice, but it's REALLY fast
    if (pPacket->QueryInterface(IID_ServerPacket,(void **)0xffffd00d) == HXR_OK)
    {
	pServerPacket = (ServerPacket *)pPacket;
    }
    else
    {
	IHXRTPPacket *pRTPPacket;

	PANIC(("Old-Style IHXPacket Wrapping Used\n"));
	if (pPacket->QueryInterface(IID_IHXRTPPacket, 
				    (void**) &pRTPPacket) == HXR_OK)
	{
	    pServerPacket = new ServerRTPPacket();
	    pRTPPacket->Release();
	}
	else
	{
	    pServerPacket = new ServerPacket();
	}

	pServerPacket->SetPacket(pPacket);
    }

    // set the TSD flag
    UINT16 unRule = pPacket->GetASMRuleNumber();
    PacketStream* pStream = &m_pStreams[pPacket->GetStreamNumber()];
    BOOL bIsTSD = pStream->m_pRules[unRule].m_bTimeStampDelivery;
    pServerPacket->SetTSD(bIsTSD);
    
    return pServerPacket;
}

void
BasicPacketFlow::HandleSpeedChange(float fOldRatio)
{
    for (int i = 0; i < m_unStreamCount; ++i)
    {
	if (!m_pStreams[i].m_bStreamRegistered)
	{
	    continue;
	}

	if (m_pStreams[i].m_tLastScheduledTime > m_tTimeLineStart)
	{
	    Timeval t = m_pStreams[i].m_tLastScheduledTime - m_tTimeLineStart;
	    m_pStreams[i].m_ulTSDMark = t.tv_sec * 1000 + t.tv_usec / 1000;
	    m_pStreams[i].m_ulLastScaledPacketTime = m_pStreams[i].m_ulLastTSDTS;
	}

	if (m_pStreams[i].m_uTimeStampScheduledSendID)
	{
	    RescheduleTSD(i, fOldRatio);
	}
	else
	{
	    // If No packet is scheduled, update timeline so that
	    // time until now is in old speed and after is new speed
	    Timeval t = m_pProc->pc->engine->now -
                m_pStreams[i].m_tLastScheduledTime;
	    INT64 ulTimeDiffms = t.tv_sec * 1000 + t.tv_usec / 1000;
	    ulTimeDiffms = (INT64) (ulTimeDiffms * fOldRatio);
	    m_pStreams[i].m_ulTSDMark += (UINT32)ulTimeDiffms;
	    m_pStreams[i].m_ulLastScaledPacketTime += ulTimeDiffms;
	}
    }
}

//
// XXXtbradley much of this method has been moved to the transport
//
STDMETHODIMP
BasicPacketFlow::SetDeliveryBandwidth(UINT32 ulBackOff, UINT32 ulBandwidth)
{
    HX_RESULT hr = HXR_FAIL;
    float fOldRatio = m_fDeliveryRatio;

    m_ulSetDeliveryBandwidth = ulBandwidth;

    /* XXXSMP, Sanity Gaurd */
    if (ulBandwidth < 100)
	ulBandwidth = 100;

    if (m_pRateManager)
    {
	IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
						     (void**)&pRateMgrFlowControl))
	{
	    hr = pRateMgrFlowControl->SetDeliveryBandwidth(ulBackOff, ulBandwidth);
	    m_pFlowMgr->RecalcConstantBitRate();
	    HX_RELEASE(pRateMgrFlowControl);
	}
    }

    // since the latency period was only being set on the stream 0's
    // transport, I'm going to call SDB only on stream 0's transport
    HX_ASSERT(m_uFirstStreamRegistered != 0xFFFF);
    m_pStreams[m_uFirstStreamRegistered].m_pTransport->SetDeliveryBandwidth(ulBackOff, ulBandwidth);
    if (HXR_OK == hr)
    {
        HandleSpeedChange(fOldRatio);
    }

    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::SetBandwidth(UINT32 ulBandwidth)
{
    m_ulBandwidth = ulBandwidth;
    return HXR_OK;
}

STDMETHODIMP_(float)
BasicPacketFlow::SetSpeed(float fSpeed)
{
    float fOldSpeed = m_fDeliveryRatio;
    
    m_fDeliveryRatio = fSpeed;

    // If speed is zero, pause and suspend the timeline
    if (m_fDeliveryRatio == 0.0)
    {
	Pause(0);
    }
    else if (fOldSpeed == 0.0 && m_bPaused)
    {
	Play();
    }

    // Call the corresponding function on the RateManager
    if (m_pRateManager)
    {
	IHXPacketFlowControl* pRateMgrFlowControl = NULL;
	if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
						     (void**)&pRateMgrFlowControl))
	{
	    pRateMgrFlowControl->SetSpeed(fSpeed);
	    m_pFlowMgr->RecalcConstantBitRate();
	    HX_RELEASE(pRateMgrFlowControl);
	}
    }

    if (m_fDeliveryRatio != fOldSpeed)
    {
	HandleSpeedChange(fOldSpeed);
    }
    return fSpeed;
}

STDMETHODIMP
BasicPacketFlow::WantWouldBlock()
{
    if (m_pStreams)
    {
        IHXWouldBlock* pWouldBlock = 0;

        HX_ASSERT(m_uFirstStreamRegistered != 0xFFFF);
	if (HXR_OK == m_pStreams[m_uFirstStreamRegistered].m_pTransport->QueryInterface(
		    IID_IHXWouldBlock, (void**)&pWouldBlock))
        {
	    m_bWouldBlockAvailable = TRUE;
	    pWouldBlock->WantWouldBlock(this, 0 /* bogus ID */);
	    pWouldBlock->Release();
        }
    }

    return HXR_OK;
}

/* IHXWouldBlockResponse methods */
STDMETHODIMP
BasicPacketFlow::WouldBlock(UINT32 id)
{
    PacketStream* pStream = &m_pStreams[id];
    HX_ASSERT(pStream->m_bStreamRegistered);

    if (m_pStreams && !pStream->m_bWouldBlocking)
    {
	pStream->m_bWouldBlocking = TRUE;
	m_ulWouldBlocking++;
	if (m_ulWouldBlocking == 1)
	{
	    Pause(TRUE);
	}
    }
    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::WouldBlockCleared(UINT32 id)
{
    PacketStream* pStream = &m_pStreams[id];
    if (!m_bIsDone && pStream && pStream->m_bStreamRegistered 
	    && pStream->m_bWouldBlocking)
    {
	pStream->m_bWouldBlocking = FALSE;
	m_ulWouldBlocking--;
	if (m_ulWouldBlocking == 0)
	{
	    Play(TRUE);
	}
    }
    return HXR_OK;
}

void
BasicPacketFlow::SetPlayerInfo(Client* pClient,
                               const char* szSessionId,
                               IHXSessionStats* pSessionStats)
{
    // we already have the session stats; but we have to use the same
    // function signature as the PPM.

    HX_RELEASE(m_pClient);
    HX_RELEASE(m_pPlayerSessionId);

    m_pClient = pClient;
    m_pClient->AddRef();

    UINT32 ulBuffLen = strlen(szSessionId) + 1;
    m_pPlayerSessionId = new ServerBuffer(TRUE);
    m_pPlayerSessionId->Set((UCHAR*)szSessionId, ulBuffLen);
}
void
BasicPacketFlow::SetConverter(DataConvertShim* pShim)
{
    HX_RELEASE(m_pConvertShim);
    m_pConvertShim = pShim;
    m_pConvertShim->AddRef();
    m_pConvertShim->SetDataResponse(this);
}

HX_RESULT
BasicPacketFlow::UpdatePlayTime()
{
    /*
     * put the number of elapsed ticks into the session stats.
     *
     * I don't want to call a system call (gettimeofday) for every packet
     * sent; engine->now, updated every mainloop iteration, should be good
     * enough for this.
     *
     * Note that Pause() or Done() effectively wipes out the running
     * calculation done on every packet get, which was a rough calc anyway.
     */

    UINT32 ticks;
    Timeval t(m_tvBankedPlayTime);

    if (!m_bPaused && !m_bIsDone)
    {
        t += (m_pProc->pc->engine->now - m_tvRTSPPlayTime);
    }

    // store elapsed time, t, as number of ticks in session stats

    ticks = t.tv_sec * 1000 + t.tv_usec / 1000;

    if (m_pStats) m_pStats->SetPlayTime(ticks);

    return HXR_OK;
}

// IHXAccurateClock method

STDMETHODIMP_(HXTimeval)
BasicPacketFlow::GetTimeOfDay()
{
    HXTime now;
    HXTimeval hxnow;

    gettimeofday(&now, NULL);
    hxnow.tv_sec = now.tv_sec;
    hxnow.tv_usec = now.tv_usec;

    return hxnow;
}

/***********************************************************************
 *  IHXDataConvertResponse
 */
STDMETHODIMP
BasicPacketFlow::DataConvertInitDone(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
BasicPacketFlow::ConvertedFileHeaderReady(HX_RESULT status,
					IHXValues* pFileHeader)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
BasicPacketFlow::ConvertedStreamHeaderReady(HX_RESULT status,
					    IHXValues* pStreamHeader)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
BasicPacketFlow::ConvertedDataReady(HX_RESULT status, IHXPacket* pPacket)
{
    if (status == HXR_OK && !pPacket)
    {
	pPacket = m_pConvertingPacket;
        pPacket->AddRef();
    }
    else
    {
	pPacket->AddRef();
    }
    
    SessionPacketReady(status, pPacket);
    pPacket->Release();
    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::SendControlBuffer(IHXBuffer* pBuffer)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
BasicPacketFlow::StartPackets()
{
    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::SetSink(IHXServerPacketSink* pSink)
{
    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::GetPacket()
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
BasicPacketFlow::SinkBlockCleared(UINT32 ulStream)
{
    WouldBlockCleared(ulStream);
    return HXR_OK;
}

STDMETHODIMP
BasicPacketFlow::EnableTCPMode()
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
BasicPacketFlow::RegisterStream(Transport* pTransport,
                              UINT16 uStreamGroupNumber,
			      UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
			      IHXValues* pHeader)
{
    return RegisterStream(pTransport, uStreamNumber, pRuleBook, pHeader);
}

UINT16
BasicPacketFlow::TranslateStreamNumToStreamGroupNum(UINT16 uStreamNum)
{
    for (unsigned int i = 0; i < m_unStreamCount; ++i)
    {
	if (m_pStreams[i].m_unStreamNumber == uStreamNum)
	{
	    return m_pStreams[i].m_uStreamGroupNumber;
	}
    }

    return 0xFFFF;
}
