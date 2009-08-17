/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pullpcktflow.cpp,v 1.19 2006/12/21 05:06:06 tknox Exp $
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
#include "hxpiids.h"
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

#include "servbuffer.h"
#include "hxqossess.h"
#include "hxpcktflwctrl.h"
#include "pcktflowmgr.h"
#include "pullpcktflow.h"
#include "pcktstrm.h"
#include "pcktflowwrap.h"
#include "qos_sess_cbr_ratemgr.h"

#define FILTER_CONSTANT   0.01

STDMETHODIMP
PullPacketFlow::GetNextPacket(UINT16 unStreamNumber)
{
    PacketStream* pStream = &m_pStreams[unStreamNumber];

    if (!m_pSourcePackets)
        return HXR_OK;

    if (pStream->m_ulPacketsOutstanding != 0)
        return HXR_OK;

    // Errors trapped in FileFormatSource
    if (!pStream->m_bPacketRequested)
    {
        pStream->m_bPacketRequested = TRUE;
        m_pSourcePackets->GetPacket(unStreamNumber);
    }

    return HXR_OK;
}

PullPacketFlow::PullPacketFlow(Process* p,
                               IHXSessionStats* pSessionStats,
                               UINT16 unStreamCount,
                               PacketFlowManager* pFlowMgr,
                               IHXPSourcePackets* pSourcePackets,
                               BOOL bIsMulticast)
    : BasicPacketFlow(p, pSessionStats, unStreamCount, pFlowMgr, bIsMulticast)
{
    m_pSourcePackets = pSourcePackets;
    m_tLastSendTime = 0.0;
    m_tNextSendTime = 0.0;
    m_lBestStreamRatio = -0x0fffffff;

    m_uScheduledSendID = 0;
    m_pTimeCallback = new (m_pProc->pc->mem_cache) PacketFlowTimeCallback();
    m_pTimeCallback->m_pFlow = this;
    m_pTimeCallback->AddRef();

    if (pSourcePackets->IsThreadSafe() & HX_THREADSAFE_METHOD_FF_GETPACKET)
        m_bThreadSafeGetPacket = TRUE;
    else
        m_bThreadSafeGetPacket = FALSE;

    // Create the Rate Manager Object
    CCBRRateMgr* pRateMgr = new CCBRRateMgr(p, unStreamCount, pFlowMgr);
    pRateMgr->QueryInterface(IID_IHXQoSRateManager, (void**)&m_pRateManager);
}

PullPacketFlow::~PullPacketFlow()
{
    Done();
    HX_RELEASE(m_pRateManager);
}

void
PullPacketFlow::Done()
{
    if (IsDone())
        return;

    HX_RELEASE(m_pSourcePackets);

    if (m_uScheduledSendID)
    {
        m_pProc->pc->engine->ischedule.remove(m_uScheduledSendID);
        m_uScheduledSendID = 0;
    }
    HX_RELEASE(m_pTimeCallback);

    BasicPacketFlow::Done();
}

void
PullPacketFlow::JumpStartTimeStampedPacket(PacketStream* pStream,
                                           ServerPacket* pNextPacket)
{
    Timeval tTimeStamp;
    ULONG32 ulPacketTimeStamp = pNextPacket->GetTime();

    if (!pStream->m_pTimeStampCallback)
        pStream->CreateTSCallback(this, pStream->m_unStreamNumber);

    
    if (m_fDeliveryRatio != 0.0)
    {
        // Take the difference between the TS on this packet and 
        // the TS of the last packet sent at a different speed
        // which is set in RescheduleTSDstream
        UINT32 ulScaledDiff =
            (ulPacketTimeStamp > pStream->m_ulLastScaledPacketTime) ?
            ulPacketTimeStamp - pStream->m_ulLastScaledPacketTime : 0;
        ulPacketTimeStamp =
            pStream->m_ulTSDMark + (ULONG32) (ulScaledDiff / m_fDeliveryRatio);
    }

    tTimeStamp.tv_sec  = ulPacketTimeStamp / 1000;
    tTimeStamp.tv_usec = (ulPacketTimeStamp % 1000) * 1000;
	    
    // Schedule the next packet based on it's delivery time with respect
    // to the current timeline.
    pStream->m_ulLastTSDTS = pNextPacket->GetTime();

    if ((m_tTimeLineStart + tTimeStamp) > m_pProc->pc->engine->now)
    {
        // early: schedule for some time in the future
        pStream->m_uTimeStampScheduledSendID = 
            m_pProc->pc->engine->ischedule.enter(
                m_tTimeLineStart + tTimeStamp,
                pStream->m_pTimeStampCallback);
        pStream->m_tLastScheduledTime = m_tTimeLineStart + tTimeStamp;
    }
    else if (((m_tTimeLineStart + tTimeStamp) <  m_pProc->pc->engine->now) &&
             (pStream->m_ulMaxBitRate > 500) )
        // The MaxBitRate > 500 constraint exists to account for event
        // streams wherein the current bitrate may exceed in the case 
        // of seek, etc. the max bit rate.
    {
        // late: send now! (unless we exceed max birate)
        UINT32 ulCurrentBitRate = pStream->m_pRules[pNextPacket->m_uASMRuleNumber].
            m_BitRate.GetAverageBandwidth();
		
        if ((ulCurrentBitRate != 0) && ulCurrentBitRate <= pStream->m_ulMaxBitRate)
        {
            pStream->m_uTimeStampScheduledSendID = 
                m_pProc->pc->engine->ischedule.enter(
                    m_pProc->pc->engine->now, pStream->m_pTimeStampCallback);
            pStream->m_tLastScheduledTime = m_pProc->pc->engine->now;
        }
        else
        {
            pStream->m_tLastScheduledTime = pStream->m_tLastScheduledTime +
                (1000 * (INT32)((1000 * pNextPacket->GetSize() * 8) /           
                                pStream->m_ulMaxBitRate));
            pStream->m_uTimeStampScheduledSendID = 
                m_pProc->pc->engine->ischedule.enter( 
                    pStream->m_tLastScheduledTime,
                    pStream->m_pTimeStampCallback);
        }
    }
    else                                
    {
        // on-time or no max bitrate:  schedule for "now"
        pStream->m_uTimeStampScheduledSendID = 
            m_pProc->pc->engine->ischedule.enter(m_pProc->pc->engine->now,
                                                 pStream->m_pTimeStampCallback);
        pStream->m_tLastScheduledTime = m_pProc->pc->engine->now;
    }
}

// Reschedule TS streams when delivery rate changes
void 
PullPacketFlow::RescheduleTSD(UINT16 unStream, float fOldRatio)
{
    HX_ASSERT(m_pStreams[unStream].m_uTimeStampScheduledSendID);

    m_pProc->pc->engine->ischedule.remove(
        m_pStreams[unStream].m_uTimeStampScheduledSendID);
    m_pStreams[unStream].m_uTimeStampScheduledSendID = 0;

    Timeval timeDiff; 
    
    if (m_pProc->pc->engine->now < m_pStreams[unStream].m_tLastScheduledTime)
    {
    	timeDiff = m_pStreams[unStream].m_tLastScheduledTime
            - m_pProc->pc->engine->now;
    }
    else
    {
	timeDiff.tv_sec = timeDiff.tv_usec = 0;
    }

    INT64 ulTimeDiffms = timeDiff.tv_sec * 1000 + timeDiff.tv_usec / 1000;

    ulTimeDiffms = (INT64) (ulTimeDiffms * fOldRatio);
    if (m_fDeliveryRatio != 0.0)
        ulTimeDiffms = (INT64) (ulTimeDiffms / m_fDeliveryRatio);

    m_pStreams[unStream].m_uTimeStampScheduledSendID = 
	m_pProc->pc->engine->ischedule.enter(m_tTimeLineStart +
            (int)ulTimeDiffms, m_pStreams[unStream].m_pTimeStampCallback);

    Timeval tOld = m_pStreams[unStream].m_tLastScheduledTime;

    ServerPacket* pNextPacket = m_pStreams[unStream].m_pPackets.PeekPacket();
    m_pStreams[unStream].m_ulLastTSDTS = pNextPacket->GetTime();

    m_pStreams[unStream].m_tLastScheduledTime = m_tTimeLineStart + (int)ulTimeDiffms;
}

void
PullPacketFlow::JumpStart(UINT16 unStream)
{
    PacketStream* pStream = &m_pStreams[unStream];

    // Jump start the PacketFlowManager if it's idle or the next packet
    // is TS delivered
    ServerPacket* pNextPacket = pStream->m_pPackets.PeekPacket();
    if (!pNextPacket)
        return;

    if (!m_bSessionPlaying && !m_bTimeLineSuspended)
    {
        BOOL bIsFirstSeekPckt = m_bSeekPacketPending ? TRUE : FALSE;
        ResetSessionTimeline(pNextPacket,
                             pStream->m_unStreamNumber,
                             bIsFirstSeekPckt);
        m_bSeekPacketPending = FALSE;
        m_bSessionPlaying = TRUE;
    }

#if 0
    if (pStream->m_pRules[pNextPacket->m_uASMRuleNumber].m_bTimeStampDelivery &&
        !pStream->m_uTimeStampScheduledSendID &&
        !IsPaused())
    {
        JumpStartTimeStampedPacket(pStream, pNextPacket);
        return;
    }
#endif // 0

    // SendPacket is only used for CBR and if we have a CB outstanding
    // for CBR then we shouldn't call SendPacket anymore until that CB fires
    if (!m_uScheduledSendID)
        SendPacket();
}

void
PullPacketFlow::HandleResume()
{
}

void
PullPacketFlow::InitialStartup()
{
    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        GetNextPacket(i);
        JumpStart(i);
    }
}

STDMETHODIMP
PullPacketFlow::StartSeek(UINT32 ulTime)
{
    m_tTimeLineStart = m_pProc->pc->engine->now; //- (int)ulTime;
    m_bTimeLineSuspended = TRUE;
    m_bSeekPacketPending = TRUE;

    for (int i = 0; i < m_unStreamCount; i++)
    {
	m_pStreams[i].m_ulLastScaledPacketTime = ulTime;
	m_pStreams[i].m_ulTSDMark = ulTime;
	m_pStreams[i].Reset();
    }

    m_bInSeek = TRUE;
    m_bGetPacketsOutstanding = TRUE;
    return HXR_OK;
}

STDMETHODIMP
PullPacketFlow::SeekDone()
{
    m_bInSeek = FALSE;

    Activate(FALSE);

    if (m_bPlayPendingOnSeek)
    {
	m_bPlayPendingOnSeek = FALSE;
	Play();
    }

    return HXR_OK;
}

//
// Compare with the current "best" flow, making this one the current
// best if it's better.  As a side effect, this function also flushes
// left over aggregated or back-to-back packets (RDT only)
//
INT16
PullPacketFlow::BestStream(UINT32 ulMsecSinceLastSend)
{
    // reset
    m_lBestStreamRatio = -0x0fffffff;
    INT16 nBestStream = -1;

    if (IsPaused() || m_bInSeek)
        return -1;

    INT32            ulFlowRatio            = -0x0fffffff;
    BOOL             bAnyPacketsForThisFlow = FALSE;
    UINT32           ulFlowScaledAvgBPS     = 0;
    UINT32           ulStreamScaledAvgBPS   = 0;
    INT32            ulFlowBytesDueTimes10  = 0;
    BOOL             bSkipBestCalculation   = TRUE;
    BOOL             bLowStampInvalid       = TRUE;

    // update this flow's bitrate across all streams
    UINT32 j;
    for (j = 0; j < m_unStreamCount; j++)
    {
        PacketStream* pStream = &m_pStreams[j];
        ServerPacket* pPacket = pStream->m_pPackets.PeekPacket();
        ulStreamScaledAvgBPS =
            (UINT32)(pStream->m_ulAvgBitRate * m_fDeliveryRatio);

        // calculate how many bytes we need to send based on when we last sent
        // and how much we are sending per second
        // XXXtbradley shouldn't the time since last send be per stream????
        pStream->m_lBytesDueTimes10 +=
            (ulMsecSinceLastSend * ulStreamScaledAvgBPS) / 800;

        if (pPacket)
        {
            bAnyPacketsForThisFlow = TRUE;
            
            pStream->m_ulRatio =
                (INT32)(((double)pStream->m_lBytesDueTimes10 * 10000 /
                         (double)ulStreamScaledAvgBPS));

            ulFlowScaledAvgBPS += ulStreamScaledAvgBPS;
            ulFlowBytesDueTimes10 += pStream->m_lBytesDueTimes10;

            // if we've seen any packets for this flow
            bSkipBestCalculation = FALSE;
        }
    }

    // Left over packets in the queue implies that this is RDT.
    // These packets are either left over BackToBack packets or left over
    // aggregation packets.  Send them using any stream's transport
    if (!bAnyPacketsForThisFlow)
    {
	HX_ASSERT(m_uFirstStreamRegistered != 0xFFFF);
	m_pStreams[m_uFirstStreamRegistered].m_pSink->Flush();
    }

    // no packets or all timestamp-based delivery
    if (bSkipBestCalculation)
        return -1;

    if (ulFlowScaledAvgBPS)
    {
        ulFlowRatio = (INT32)(((double)ulFlowBytesDueTimes10
                                * 10000 / (double)ulFlowScaledAvgBPS));
    }

    UINT32 ulLowestTime = 0xffffffff;
    for (j = 0; j < m_unStreamCount; j++)
    {
        PacketStream* pStream = &m_pStreams[j];
        ServerPacket* pPacket = pStream->m_pPackets.PeekPacket();

        DPRINTF(D_PROT, ("Bytes Due: %ld (Stream: %d) [Highest %ld] Pkt %p\n",
                         pStream->m_lBytesDueTimes10, j,
                         m_lBestStreamRatio, pPacket));

        // if there's no packets for the stream or the stream is TSD then it
        // can't be best
        if (!pPacket)
            continue;

        UINT32 ulTime = pPacket->GetTime();

        // if this stream has the packet that needs to be sent the earliest and
        // the magic ratio for this whole flow is bigger than that of the
        // current best stream, this stream and flow become best
        if (ulTime < ulLowestTime)
        {
            ulLowestTime = ulTime;

            m_lBestStreamRatio = pStream->m_ulRatio;
            nBestStream = j;
        }
        else if (pStream->m_ulRatio > m_lBestStreamRatio)
        {
            // otherwise if the magic ratio for this stream is bigger than that
            // of the current best stream, this stream and flow become best
            m_lBestStreamRatio = pStream->m_ulRatio;
            nBestStream = j;
        }
    }

    return nBestStream;
}

//
// send next packet.  return whether or not the next packet send should
// be schedulled
//
BOOL
PullPacketFlow::SendPacket()
{
    Timeval tNow;
    Timeval tNextTime;
    UINT32 ulPacketSize;

    // XXXSMP Handle roundoff errors upwards (or maybe it doesn't matter with
    //	    adaptive bandwidth adjustments).
    tNow = m_pProc->pc->engine->now;
    Timeval tTimeSinceLastSend = tNow - m_tLastSendTime;
    UINT32  ulMsecSinceLastSend = tTimeSinceLastSend.tv_sec * 1000 + 
        tTimeSinceLastSend.tv_usec / 1000;

    // are we more than 1 second behind? or is this the first packet
    // (next send time == 0)
    if (m_tNextSendTime.tv_sec == 0 || tNow >= m_tNextSendTime + Timeval(1,0))
    {
        // XXXSMP This is basically a hack that should be reconsidered.
        ulMsecSinceLastSend = 250;
    }

    INT16 nBestStream = BestStream(ulMsecSinceLastSend);

    //
    // if there are no suitable streams to send from or the delivery rate
    // is 0 then idle this packet flow manager
    //
    if (nBestStream < 0 || !(m_pRateManager->GetActualDeliveryRate()))
    {
        m_tLastSendTime = tNow;

        // Don't go idle, just remove the packet on the queue because there
        // is no delivery bit rate.
        if (nBestStream >= 0)
            DropAndGetPacket(nBestStream);
 
        return FALSE;
    }
    
    PacketStream* pStream = &m_pStreams[nBestStream];
    ServerPacket* pPacket= pStream->m_pPackets.GetPacket();
    m_ulPacketsOutstanding--;
    pStream->m_ulPacketsOutstanding--;
    
    // Notify the player if this is the first packet for the most recent
    // playback request.
    if (!m_bTimeLineSuspended && !pStream->IsFirstPacketTSSet())
    {
        SetStreamStartTime(nBestStream, pPacket->GetTime());
        pStream->SetFirstPacketTS(pPacket->GetTime());
    }
    ulPacketSize = pPacket->GetSize();

    pPacket->m_uSequenceNumber = pStream->m_unSequenceNumber++;

    if (pStream->m_unSequenceNumber >=
        pStream->m_pTransport->wrapSequenceNumber())
    {
        pStream->m_unSequenceNumber = 0;
    }

    if (pPacket->m_uPriority == 10)
    {
	pPacket->m_uReliableSeqNo = ++pStream->m_unReliableSeqNo;
    }
    else
    {
	pPacket->m_uReliableSeqNo = pStream->m_unReliableSeqNo;
    }

    pStream->m_lBytesDueTimes10 -= ulPacketSize * 10;

    if (pStream->m_pSink->PacketReady(pPacket) == HXR_BLOCKED)
    {
        WouldBlock(nBestStream);
        return FALSE;
    }

    if (pStream->IsStreamDone())
	ScheduleStreamDone(this, pStream->m_pTransport, pStream, nBestStream);

    m_pRateManager->VerifyDelivery(pPacket, tNextTime);

    pPacket->Release();

    m_uScheduledSendID = m_pProc->pc->engine->ischedule.enter(
            tNextTime, m_pTimeCallback);
    m_tLastSendTime = tNow;

    // Get the next packet from the same flow we just sent from
    if (!m_bThreadSafeGetPacket && !m_pProc->pc->engine->m_bMutexProtection)
    {
        // XXXSMP Can't get a lock? Schedule GNP
        HXMutexLock(g_pServerMainLock);
        m_pProc->pc->engine->m_bMutexProtection = TRUE;
        m_bDidLock = TRUE;
        GetNextPacket(nBestStream);
        if (m_bDidLock)
        {
            m_pProc->pc->engine->m_bMutexProtection = FALSE;
            HXMutexUnlock(g_pServerMainLock);
            m_bDidLock = FALSE;
        }
    }
    else
    {
        GetNextPacket(nBestStream);
    }

    return FALSE;
}

STDMETHODIMP
PullPacketFlow::RegisterStream(Transport* pTransport,
                               UINT16 uStreamNumber,
                               ASMRuleBook* pRuleBook,
                               IHXValues* pHeader)
{
    m_uNumStreamsRegistered++;
    if (m_uFirstStreamRegistered == 0xFFFF)
    {
	m_uFirstStreamRegistered = uStreamNumber;
    }

    m_pStreams[uStreamNumber].Register(pTransport, pRuleBook,
                                              pHeader, uStreamNumber, FALSE);
    return m_pRateManager->RegisterStream(&(m_pStreams[uStreamNumber]), uStreamNumber);    
}

HX_RESULT
PullPacketFlow::SessionPacketReady(HX_RESULT ulStatus,
                                   IHXPacket* pPacket)
{
    PacketStream*	pStream;
    UINT16		uStreamNumber;
    UINT16		unRule;
    UINT8		ucFlags;

    //XXXGH...how about checking the status Sujal?
    if (m_bIsDone)
	return HXR_UNEXPECTED;

    uStreamNumber = pPacket->GetStreamNumber();
    if (pPacket->IsLost())
    {
	unRule = 0;
	ucFlags = 0;
    }
    else
    {
	unRule = pPacket->GetASMRuleNumber();
	ucFlags = pPacket->GetASMFlags();
    }
    pStream = &m_pStreams[uStreamNumber];
    pStream->m_bPacketRequested = FALSE;

    if (pStream->m_bStreamDone)
    {
	/* 
	 * We may get PacketReady calls for pending GetPacket request on 
	 * other streams. This should ONLY happen when there is an end time
	 * associated with this source (refer to the next if condition). 
	 */
	HX_ASSERT(m_uEndPoint > 0);

	// keep calling GetNextPacket() so that the other stream
	// will not be blocked by the done stream in RMFFPLIN
	return GetNextPacket(uStreamNumber);
    }

    //
    // the rule is outside of our rulebook so the packet is bad
    //
    if (unRule >= pStream->m_lNumRules)
    {
        DPRINTF(0x02000000, ("Session %p: dropped packet with bad rule. stream:"
                             " %d time %ld asm rule: %d\n", this, uStreamNumber,
                             pPacket->GetTime(), unRule));
        return GetNextPacket(uStreamNumber);
    }

    /* 
     * If we have received a packet past the end time, 
     * mark the source as done. This assumes that the fileformat
     * always returns packets in increasing timestamp order across
     * streams.
     */
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
	    
            // unsubscribe any active rules of this stream
            for (UINT8 i = 0; i < pStream->m_lNumRules; i++)
            {
                if (pStream->m_pRules[i].m_bRuleOn)
                {
                    pStream->m_pRules[i].m_bActivateOnSeek = TRUE;
                    HandleUnSubscribe(i, uStreamNumber);
                }
            }
        }
    }

    if (pPacket->IsLost())
    {
	goto SkipAllASMProcessing;
    }

    if ((pStream->m_pRVDrop) && (!pStream->m_pRVDrop->PacketApproved(pPacket)))
    {
	return GetNextPacket(uStreamNumber);
    }

    if (pStream->m_pRules[unRule].m_PendingAction == NONE)
    {
	/*
	 * No rule transition is in progress.  Attempt normal filtering
	 * of packets whose rules are not on.
	 */
	if (!pStream->m_pRules[unRule].m_bRuleOn)
	{
	    return GetNextPacket(uStreamNumber);
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
		return GetNextPacket(uStreamNumber);
	    }

            // Update Delivery rate
            if (!pStream->m_pRules[unRule].m_bBitRateReported)
            {
//                m_ulSubscribedRate += pStream->m_pRules[unRule].m_ulAvgBitRate;
                IHXPacketFlowControl* pRateMgrFlowControl = NULL;
                if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                        (void**)&pRateMgrFlowControl))
                {
                    // May have to look at this again, if the implementation
                    // of HandleSubscribe changes in RateManager
                    pRateMgrFlowControl->HandleSubscribe(unRule, uStreamNumber);
                    HX_RELEASE(pRateMgrFlowControl);
                }               

                INT32 lChange = pStream->m_pRules[unRule].m_ulAvgBitRate;
                ASSERT(pStream->m_bGotSubscribe);
                pStream->ChangeDeliveryBandwidth(lChange);
                m_pFlowMgr->ChangeDeliveryBandwidth(
                    lChange, !m_bIsMulticast && !pStream->m_bNullSetup);
/*
                if (pStream->m_pRules[unRule].m_bTimeStampDelivery)
                {
                    pStream->m_ulVBRAvgBitRate +=
                        pStream->m_pRules[unRule].m_ulAvgBitRate;
                }
*/
                pStream->m_pRules[unRule].m_bBitRateReported = TRUE;
            }
            pStream->m_pRules[unRule].m_PendingAction = NONE;

	}
	else if (pStream->m_pRules[unRule].m_PendingAction == ACTION_OFF &&
                 (ucFlags & HX_ASM_SWITCH_OFF) &&
                 (pStream->IsDependOk(FALSE, unRule)))
	{
            if (pStream->m_pRules[unRule].m_bBitRateReported)
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

//                m_ulSubscribedRate -= pStream->m_pRules[unRule].m_ulAvgBitRate;
                INT32 lChange =
                    (-1) * (INT32)pStream->m_pRules[unRule].m_ulAvgBitRate;
                ASSERT(pStream->m_bGotSubscribe);
                pStream->ChangeDeliveryBandwidth(lChange);
                m_pFlowMgr->ChangeDeliveryBandwidth(
                    lChange, !m_bIsMulticast && !pStream->m_bNullSetup);
/*
                if (pStream->m_pRules[unRule].m_bTimeStampDelivery)
                {
                    pStream->m_ulVBRAvgBitRate -=
                        pStream->m_pRules[unRule].m_ulAvgBitRate;
                }
*/
                pStream->m_pRules[unRule].m_bBitRateReported = FALSE;
            }

            pStream->m_pRules[unRule].m_PendingAction = NONE;

            if (pStream->m_bStreamDonePending)
            {
                // all the rules are unsubscribed and 
                // we are done with this stream
                StreamDone(uStreamNumber);
            }

            return GetNextPacket(uStreamNumber);
	}
    }

    /*
     * If the packet is NOT IsLost() but we don't have a buffer, then we
     * must should have only needed this packet for updating the ASM state
     * machine, but the state machine must be messed up somehow.
     * In this case, don't queue the packet!
     */
    IHXBuffer* pTmpBuffer;
    pTmpBuffer = pPacket->GetBuffer();
    if (pTmpBuffer != NULL)
    {
	pTmpBuffer->Release();
    }
    else
    {
	return GetNextPacket(uStreamNumber);
    }

SkipAllASMProcessing:

    ServerPacket* pServerPacket = HXPacketToServerPacket(pPacket);

    pServerPacket->m_uPriority =
	pStream->m_pRules[unRule].m_ulPriority;

    pServerPacket->m_uASMRuleNumber = unRule;

    pStream->m_pPackets.PutPacket(pServerPacket);
    m_ulPacketsOutstanding++;
    pStream->m_ulPacketsOutstanding++;

    return HXR_OK;
}

//
// IHXFlowControl interface method
//
STDMETHODIMP
PullPacketFlow::Activate()
{
    ASSERT(!IsPaused());

    return Activate(TRUE);
}

//
// internal method
//
STDMETHODIMP
PullPacketFlow::Activate(BOOL bResetStreams)
{
    HandleTimeLineStateChange(TRUE);
    m_pFlowMgr->RecalcConstantBitRate();

    IHXPacketFlowControl* pRateMgrFlowControl = NULL;
    if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                        (void**)&pRateMgrFlowControl))
    {
        pRateMgrFlowControl->Activate();
        HX_RELEASE(pRateMgrFlowControl);
    }

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        IHXPacketFlowControl* pRateMgrFlowControl = NULL;
        ULONG32 ulSubscribedRate = 0;
        if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                 (void**)&pRateMgrFlowControl))
        {
           // May have to look at this again, if the implementation
           // of HandleSubscribe changes in RateManager
           ulSubscribedRate = pRateMgrFlowControl->GetDeliveryRate();
           HX_RELEASE(pRateMgrFlowControl);
        }               

        BOOL bSubscribed = m_pStreams[i].Activate(bResetStreams,
                                                  m_bInitialSubscriptionDone,
                                                  m_bIsMulticast,
                                                  ulSubscribedRate,
                                                  m_pFlowMgr);

        m_bSubscribed = m_bSubscribed || bSubscribed;

        if (!IsPaused())
        {
            ASSERT(m_pSourcePackets);
            GetNextPacket(i);
        }
    }

    return HXR_OK;
}

void
PullPacketFlow::SendTimeStampedPacket(UINT16 unStreamNumber)
{
    PacketStream* pStream = &m_pStreams[unStreamNumber];

    ServerPacket* pPacket = pStream->m_pPackets.GetPacket();
    m_ulPacketsOutstanding--;
    pStream->m_ulPacketsOutstanding--;
    ASSERT(pPacket);

    /* 
     * Notify the player if this is the first packet for the most recent
     * playback request.
     */
    if (!m_bTimeLineSuspended && !pStream->IsFirstPacketTSSet())
    {
        SetStreamStartTime(unStreamNumber, pPacket->GetTime());
        pStream->SetFirstPacketTS(pPacket->GetTime());
    }
    
    pStream->m_uTimeStampScheduledSendID = 0;

    pPacket->m_uSequenceNumber = pStream->m_unSequenceNumber++;

    if (pStream->m_unSequenceNumber >= pStream->m_pTransport->wrapSequenceNumber())
    {
        pStream->m_unSequenceNumber = 0;
    }

    if (pPacket->m_uPriority == 10)
	pPacket->m_uReliableSeqNo  = ++pStream->m_unReliableSeqNo;
    else
	pPacket->m_uReliableSeqNo  = pStream->m_unReliableSeqNo;

    /*
     * XXXSMP
     * We should support BackToBack packets & aggregation here.  But we never
     * did before, so...
     */

    pStream->m_pSink->PacketReady(pPacket);
    *g_pBytesServed += pPacket->GetSize();
    (*g_pPPS)++;

    if (pStream->IsStreamDone())
    {
	ScheduleStreamDone(this, pStream->m_pTransport, pStream, unStreamNumber);
    }

    // Notify the appropriate Bandwidth Calculator
    UINT16  uStreamNumber   = pPacket->GetStreamNumber();
    ULONG32 unRule	    = pPacket->m_uASMRuleNumber;
    PacketStream* pStream2 = &m_pStreams[uStreamNumber];
    pStream2->m_pRules[unRule].m_pBWCalculator->PacketSent(pPacket);
    HXTimeval tHXNow;
    tHXNow.tv_sec = m_pProc->pc->engine->now.tv_sec;
    tHXNow.tv_usec = m_pProc->pc->engine->now.tv_usec;
    pStream2->m_pRules[unRule].m_BitRate.SentPacket(pPacket->GetSize(), tHXNow);

    pPacket->Release();

    GetNextPacket(uStreamNumber);

    JumpStart(uStreamNumber);
}

STDMETHODIMP
PullPacketFlow::DropAndGetPacket(UINT16 unStream)
{
    ServerPacket* pPacket= m_pStreams[unStream].m_pPackets.GetPacket();
    m_ulPacketsOutstanding--;
    m_pStreams[unStream].m_ulPacketsOutstanding--;

    HX_RELEASE(pPacket);

    return GetNextPacket(unStream);
}

//
// the only reason this is even defined is to get pcktstrm.cpp compiling
// eventually we need to move this out of here and only define it in
// PushPacketFlow
//
void
PullPacketFlow::TransmitPacket(ServerPacket* pPacket)
{
    ASSERT(0);
}

STDMETHODIMP
PacketFlowTimeCallback::Func()
{
    m_pFlow->m_uScheduledSendID = 0;
    m_pFlow->SendPacket();

    return HXR_OK;
}

STDMETHODIMP
PacketFlowTimeStampCallback::Func()
{
    m_pFlow->m_pStreams[m_unStreamNumber].m_uTimeStampScheduledSendID = 0;
    m_pFlow->SendTimeStampedPacket(m_unStreamNumber);

    return HXR_OK;
}
