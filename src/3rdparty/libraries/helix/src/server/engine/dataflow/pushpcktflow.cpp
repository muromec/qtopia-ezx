/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pushpcktflow.cpp,v 1.13 2006/12/21 05:06:06 tknox Exp $
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

#include "servbuffer.h"

#include "hxpcktflwctrl.h"
#include "hxqossess.h"
#include "pcktflowmgr.h"
#include "pushpcktflow.h"
#include "pcktstrm.h"
#include "pcktflowwrap.h"
#include "qos_sess_cbr_ratemgr.h"

#define PACKET_QUEUE_SIZE 12
#define FILTER_CONSTANT   0.01

PushPacketFlow::PushPacketFlow(Process* p,
                               IHXSessionStats* pSessionStats,
                               UINT16 unStreamCount,
                               PacketFlowManager* pFlowMgr,
                               IHXPSourceLivePackets* pSourceLivePackets,
                               BOOL bIsMulticast)
    : BasicPacketFlow(p, pSessionStats, unStreamCount, pFlowMgr, bIsMulticast)
{
    m_pSourceLivePackets = pSourceLivePackets;

    m_pASMSource = NULL;
    m_pSourceLivePackets->QueryInterface(IID_IHXASMSource,
                                         (void**)&m_pASMSource);

    // create a meter callback for each stream
    for (UINT16 iStream = 0; iStream < unStreamCount; iStream++)
	m_pStreams[iStream].CreateMeterCallback();

    // Create the Rate Manager Object
    CCBRRateMgr* pRateMgr = new CCBRRateMgr(p, unStreamCount, pFlowMgr);
    pRateMgr->QueryInterface(IID_IHXQoSRateManager, (void**)&m_pRateManager);
}

PushPacketFlow::~PushPacketFlow()
{
    Done();
}

void
PushPacketFlow::Done()
{
    if (IsDone())
        return;

    UINT16 i;
    for (i = 0; i < m_unStreamCount; i++)
    {
        m_pSourceLivePackets->StopPackets(i);
    }

    for (i = 0; i < m_unStreamCount; i++)
    {
	PacketStream* pStream = &m_pStreams[i];
	if (m_pASMSource)
	{
	    for (INT32 lRule = 0; lRule < pStream->m_lNumRules; lRule++)
	    {
		if (pStream->m_pRules[lRule].m_bRuleOn)
		    m_pASMSource->Unsubscribe(i, lRule);
	    }
	}

	if (pStream->m_ulMeterCallbackID)
	{
	    m_pProc->pc->engine->ischedule.remove(pStream->m_ulMeterCallbackID);
	    pStream->m_ulMeterCallbackID = 0;
	}
    }

    HX_RELEASE(m_pSourceLivePackets);
    HX_RELEASE(m_pASMSource);
    BasicPacketFlow::Done();
}

void
PushPacketFlow::HandleResume()
{
    for (UINT16 j = 0; j < m_unStreamCount; j++)
    {
        m_pStreams[j].HandleLiveResume();
    }
}

void
PushPacketFlow::InitialStartup()
{
    HX_ASSERT(m_pSourceLivePackets);
    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        m_pSourceLivePackets->StartPackets(i);
    }
}

STDMETHODIMP
PushPacketFlow::StartSeek(UINT32 ulTime)
{
    return HXR_OK;
}

STDMETHODIMP
PushPacketFlow::SeekDone()
{
    m_bInSeek = FALSE;
    return HXR_OK;
}

void
PushPacketFlow::Pause(BOOL bWouldBlock, UINT32 ulPausePoint)
{
    // call base class

    BasicPacketFlow::Pause(bWouldBlock, ulPausePoint);

    // do our custom processing

    if (!ulPausePoint)
    {
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            m_pSourceLivePackets->StopPackets(i);
        }
    }
}

STDMETHODIMP
PushPacketFlow::RegisterStream(Transport* pTransport,
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
                                              pHeader, uStreamNumber, TRUE);
    return m_pRateManager->RegisterStream(&(m_pStreams[uStreamNumber]), uStreamNumber);    

}

HX_RESULT
PushPacketFlow::SessionPacketReady(HX_RESULT ulStatus,
                                   IHXPacket* pPacket)
{
    HX_ASSERT(pPacket);

    if ((ulStatus != HXR_OK) || (pPacket == NULL))
	return HXR_OK;

    if (m_bIsDone)
	return HXR_UNEXPECTED;

    PacketStream*	pStream;
    UINT16		uStreamNumber;
    UINT16		unRule;
    UINT8		ucFlags;
    UINT8		i = 0;
    BOOL		bStreamDone = FALSE;

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
	HX_ASSERT(0);
	return HXR_FAIL;
    }

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

    if (pPacket->IsLost())
    {
	goto SkipAllASMProcessing;
    }

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
//                m_ulSubscribedRate -= pStream->m_pRules[unRule].m_ulAvgBitRate;
                IHXPacketFlowControl* pRateMgrFlowControl = NULL;
                if (HXR_OK == m_pRateManager->QueryInterface(IID_IHXPacketFlowControl,
                         (void**)&pRateMgrFlowControl))
                {
                    // May have to look at this again, if the implementation
                    // of HandleSubscribe changes in RateManager
                    pRateMgrFlowControl->HandleUnSubscribe(unRule, uStreamNumber);
                    HX_RELEASE(pRateMgrFlowControl);
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

            return HXR_OK;
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
	return HXR_OK;
    }

SkipAllASMProcessing:
    /* Prepare to Send this packet */
    ServerPacket* pServerPacket = HXPacketToServerPacket(pPacket);

    pServerPacket->m_uPriority =
	pStream->m_pRules[unRule].m_ulPriority;

    pServerPacket->m_uASMRuleNumber = unRule;

    pServerPacket->m_uSequenceNumber = pStream->m_unSequenceNumber++;

    if (pStream->m_unSequenceNumber >=
	pStream->m_pTransport->wrapSequenceNumber())
    {
        pStream->m_unSequenceNumber = 0;
    }

    if (pServerPacket->m_uPriority == 10)
    {
	pServerPacket->m_uReliableSeqNo  = ++pStream->m_unReliableSeqNo;
    }
    else
    {
	pServerPacket->m_uReliableSeqNo = pStream->m_unReliableSeqNo;
    }

    // ensure that this packet does not execede the transmission limits
    // of the outbound link
    MeterPacket(pServerPacket);

    if (pStream->IsStreamDone())
    {
	ScheduleStreamDone(this, pStream->m_pTransport, 
			   pStream, uStreamNumber);
    }

    return HXR_OK;
}

STDMETHODIMP
PushPacketFlow::Activate()
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

STDMETHODIMP
PushPacketFlow::HandleSubscribe(INT32 lRuleNumber, UINT16 unStreamNumber)
{
    if (lRuleNumber >= m_pStreams[unStreamNumber].m_lNumRules)
	return HXR_OK;

    if (m_pASMSource)
	m_pASMSource->Subscribe(unStreamNumber, lRuleNumber);

    return BasicPacketFlow::HandleSubscribe(lRuleNumber, unStreamNumber);
}

STDMETHODIMP
PushPacketFlow::HandleUnSubscribe(INT32 lRuleNumber, UINT16 unStreamNumber)
{
    if (lRuleNumber >= m_pStreams[unStreamNumber].m_lNumRules)
	return HXR_OK;

    if (m_pASMSource)
	m_pASMSource->Unsubscribe(unStreamNumber, lRuleNumber);
    
    return BasicPacketFlow::HandleUnSubscribe(lRuleNumber, unStreamNumber);
}

// MeterPacket
//
// Queue packets that are out of profile 
// Send packets immediately that are in profile
// Perform queue management when queue overflows
// Profile is determined by TCP socket (WouldBlock/WouldBlockCleared) in TCP case
// or SetDeliveryBandwidth messages, and a token bucket filter in UDP case.
//
void
PushPacketFlow::MeterPacket(ServerPacket* pPacket)
{
    if (!pPacket)
	return;

    PacketStream* pStream = &m_pStreams[pPacket->GetStreamNumber()];
    
    if (pStream->m_pMeterQueue[pStream->m_unMeterQueueTail])
    {
	// Ran out of buffer space - drop-front
	(*g_pBroadcastPPMOverflows)++;
	HX_RELEASE(pStream->m_pMeterQueue[pStream->m_unMeterQueueHead]);

	if (++pStream->m_unMeterQueueHead >= MAX_METER_QUEUE)
	{
	    pStream->m_unMeterQueueHead = 0;
	}

	pStream->m_pMeterQueue[pStream->m_unMeterQueueTail] = pPacket;

	if (++pStream->m_unMeterQueueTail >= MAX_METER_QUEUE)
	{
	    pStream->m_unMeterQueueTail = 0;
	}
    }
    else
    {
	pStream->m_pMeterQueue[pStream->m_unMeterQueueTail] = pPacket;
	
	if (++pStream->m_unMeterQueueTail >= MAX_METER_QUEUE)
	{
	    pStream->m_unMeterQueueTail = 0;
	}
    }

    pPacket->AddRef();

    if (m_bWouldBlockAvailable)
    {
	while((pStream->m_unMeterQueueHead != pStream->m_unMeterQueueTail) &&
	      (pStream->m_pMeterQueue[pStream->m_unMeterQueueHead]) &&
	      (!pStream->m_bWouldBlocking))
	{
	    TransmitPacket(pStream->m_pMeterQueue[pStream->m_unMeterQueueHead]);
	    HX_RELEASE(pStream->m_pMeterQueue[pStream->m_unMeterQueueHead]);
	    
	    if (++pStream->m_unMeterQueueHead >= MAX_METER_QUEUE)
	    {
		pStream->m_unMeterQueueHead = 0;
	    }
	}
    }
    else
    {
	if (pPacket != NULL)
	{
	    /* update the pkt size estimate */
	    pStream->m_fPktSizeEst = (pStream->m_fPktSizeEst != 0) ?
		((1 - FILTER_CONSTANT) * pStream->m_fPktSizeEst + FILTER_CONSTANT * pPacket->GetSize() * 8) :
		pPacket->GetSize() * 8;
	}

	if ((pStream->m_ulMeterCallbackID == 0) && (pStream->m_pMeterCallback))
	{
	    HX_ASSERT(m_fDeliveryRatio != 0.0);

	    if (pStream->m_ulVBRAvgBitRate)
	    {
		pStream->m_ulMeterCallbackID = m_pProc->pc->engine->ischedule.enter
		    (m_pProc->pc->engine->now + 
		     Timeval(pStream->m_fPktSizeEst/(pStream->m_ulVBRAvgBitRate*m_fDeliveryRatio)), 
		     pStream->m_pMeterCallback);
	    }
	    else if (pStream->m_ulAvgBitRate)
	    {
		pStream->m_ulMeterCallbackID = m_pProc->pc->engine->ischedule.enter
		    (m_pProc->pc->engine->now + 
		     Timeval(pStream->m_fPktSizeEst/(pStream->m_ulAvgBitRate*m_fDeliveryRatio)), 
		     pStream->m_pMeterCallback);
	    }
	    else
	    {
		pStream->m_ulMeterCallbackID = m_pProc->pc->engine->ischedule.enter
		    (m_pProc->pc->engine->now, 
		     pStream->m_pMeterCallback);
	    }
	}
    }
}

void
PushPacketFlow::TransmitPacket(ServerPacket* pPacket)
{
    if (!pPacket)
	return;
    
    PacketStream* pStream = &m_pStreams[pPacket->GetStreamNumber()];

    pStream->m_bPacketRequested = FALSE;

    if (pStream->m_pSink->PacketReady(pPacket) == HXR_BLOCKED)
    {
        WouldBlock(pPacket->GetStreamNumber());
    }
}

