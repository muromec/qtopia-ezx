/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pcktflowmgr.cpp,v 1.23 2009/01/08 01:38:04 ckarusala Exp $
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
#include "hxpcktflwctrl.h"
#include "hxqossess.h"
#include "basicpcktflow.h"
#include "pcktflowmgr.h"
#include "pcktflowwrap.h"
#include "pcktstrm.h"

PacketFlowManager::PacketFlowManager(Process* pProc, BOOL bIsRTP)
    : m_pProc(pProc)
    , m_bInitialPlayReceived(FALSE)
    , m_bIsRTP(bIsRTP)
{
    /* 
     * All delivery rates subscribed to before the initial play request are
     * stored here until the play is received. This allows us to not show
     * bandwidth for content that is being served from a downstream cache,
     * because in that case, the RealProxy never sends a play request.
     */
    m_ulPendingBitRate = 0;

    /* This value is the sum of the delivery rates of all flows / streams */
    m_ulDeliveryBitRate	= 0;

    /*
     * This is the rate that the player told us to send (higher than
     * m_ulDeliveryBitRate for oversend, lower for buffered play)
     */
    m_ulActualDeliveryRate		= 0;

    /* The last time we ran SendNextPacket() */
    m_tLastSendTime 	    		= 0.0;

    /* The next time we call SendNextPacket(), used for timing the next pkt */
    m_tNextSendTime			= 0.0;

    m_bInformationalAggregatable	= FALSE;
    m_bDidLock				= FALSE;
}

PacketFlowManager::~PacketFlowManager()
{
    HXList_iterator	i(&m_PacketFlows);

    while (m_PacketFlows.peek_head())
    {
	SessionDone((BasicPacketFlow*) m_PacketFlows.peek_head());
    }

    // Zero the bandwidth so the ServerInfo class gets an update
    //JMEV  SessionDone should have already  subtracted each session's bandwidth

    if (m_ulDeliveryBitRate) 
	ChangeDeliveryBandwidth(-1 * (INT32)m_ulDeliveryBitRate, TRUE);

    if (m_ulActualDeliveryRate)
	HXAtomicSubUINT32(g_pAggregateRequestedBitRate, m_ulActualDeliveryRate);

    if (m_bInformationalAggregatable)
	*g_pAggregatablePlayers -= 1;
}

void
PacketFlowManager::RegisterSource(IUnknown* pSourceCtrl,
                                  IHXPacketFlowControl** ppPacketFlowControl,
                                  IHXSessionStats* pSessionStats,
                                  UINT16 unStreamCount,
                                  BOOL bIsLive,
                                  BOOL bIsMulticast,
                                  DataConvertShim* pDataConv,
                                  BOOL bIsFCS)
{
    BasicPacketFlow* pPacketFlow;
    IHXServerPacketSource* pSource = NULL;

    if (SUCCEEDED(pSourceCtrl->QueryInterface(IID_IHXServerPacketSource,
						   (void **)&pSource)))
    {
	// MDP
	pPacketFlow = BasicPacketFlow::Create(m_pProc, pSessionStats, unStreamCount, this,
					      pSource, bIsMulticast, bIsLive, bIsFCS);
    }
    else
    {
	PANIC(("No Source for Packets\n"));
    }

    pPacketFlow->AddRef();
    if (pDataConv)
        pPacketFlow->SetConverter(pDataConv);
        
    m_PacketFlows.insert(pPacketFlow);

    *ppPacketFlowControl = pPacketFlow;
    pPacketFlow->AddRef();
}

void
PacketFlowManager::RegisterSource(IUnknown* pSourceCtrl,
                                  IHXPacketFlowControl** ppPacketFlowControl,
                                  IHXSessionStats* pSessionStats,
                                  UINT16 unStreamCount,
                                  BOOL bIsLive,
                                  BOOL bIsMulticast,
                                  DataConvertShim* pDataConv,
                                  Client* pClient,
                                  const char* szPlayerSessionId,
                                  BOOL bIsFCS)
{
    HX_ASSERT(pClient);
    
    RegisterSource(pSourceCtrl, ppPacketFlowControl, pSessionStats, unStreamCount, bIsLive, 
                   bIsMulticast, pDataConv, bIsFCS);

    if (*ppPacketFlowControl)
    {
        BasicPacketFlow *pPacketFlow = (BasicPacketFlow*)(*ppPacketFlowControl);
        pPacketFlow->SetPlayerInfo(pClient, szPlayerSessionId, pSessionStats);
    }
}

void
PacketFlowManager::SessionDone(BasicPacketFlow* pPacketFlow)
{
    ASSERT(pPacketFlow);

    m_PacketFlows.remove(pPacketFlow);
    pPacketFlow->Done();
    pPacketFlow->Release();
}

void
PacketFlowManager::ChangeDeliveryBandwidth(INT32 lChange, BOOL bReportChange)
{
    // Don't record this bandwidth if we are not supposed to report it
    // or we've received a NULL setup (meaning the content itself is
    // being served from a downstream cache).
    if (bReportChange)
    {
	if (m_bInitialPlayReceived)
	    m_ulDeliveryBitRate += lChange;
	else
	    m_ulPendingBitRate += lChange;
    }

    RecalcConstantBitRate();

    // Don't record this bandwidth if we are not supposed to report it
    // or we've received a NULL setup (meaning the content itself is
    // being served from a downstream cache).
    if (bReportChange && m_bInitialPlayReceived)
    {
        m_pProc->pc->server_info->ChangeBandwidthUsage(lChange, m_pProc);
    }
}

void
PacketFlowManager::CommitPendingBandwidth()
{
    if (m_ulPendingBitRate && !m_bInitialPlayReceived)
    {
	m_bInitialPlayReceived = TRUE;
	m_ulDeliveryBitRate += m_ulPendingBitRate;
	m_pProc->pc->server_info->ChangeBandwidthUsage(m_ulPendingBitRate, m_pProc);
	m_ulPendingBitRate = 0;
    }
}

// RecalcConstantBitRate
//
// A PacketFlow may contain CBR and VBR streams.  The m_ulActualDeliveryRate
// computed here is consumed only by the CBR streams.  New VBR streams will
// include AvgBitrate in the ASM Rulebook, so they appear to players as CBR
// streams, but in fact are TimeStampDelivery.  TSD streams are adjusted
// through the ratio of AvgBitrate and actual bit rate.  This routine converts
// the delivery rate into the delivery ratio.
//
// There is a second method to set delivery rate.  This is through the RTSP
// speed parameter.  In this case, the procedure is reversed. For CBR streams,
// the acceleration is converted from the ratio into an actual delivery rate
void
PacketFlowManager::RecalcConstantBitRate()
{
    HXList_iterator     i(&m_PacketFlows);
    BasicPacketFlow*	pPacketFlow;

    UINT32 ulOld = m_ulActualDeliveryRate;
    m_ulActualDeliveryRate = 0;
    UINT32 ulVBRRate = 0;

    for (; *i != 0; ++i)
    {
	pPacketFlow = (BasicPacketFlow*) (*i);
        m_ulActualDeliveryRate += pPacketFlow->GetConstantBitRate();
    }

    HXAtomicSubUINT32(g_pAggregateRequestedBitRate, ulOld);
    HXAtomicAddUINT32(g_pAggregateRequestedBitRate, m_ulActualDeliveryRate);
}

STDMETHODIMP
PacketStreamDoneCallback::Func()
{
    m_pStream->m_uScheduledStreamDoneID = 0;
    m_pTransport->streamDone(m_unStreamNumber);

    return HXR_OK;
}
