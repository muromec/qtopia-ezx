/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bwcalc.cpp,v 1.4 2004/12/06 03:37:05 jc Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "proc.h"
#include "server_engine.h"
#include "server_info.h"

#include "bwcalc.h"

BWCalculator::BWCalculator(Process* pProc, BOOL bNullSetup)
    : m_lRefCount(0)
    , m_bBandwidthPending(TRUE)
    , m_bNullSetup(bNullSetup)
    , m_ulTotalBytesSent(0)
    , m_ulElapsedTime(0)
    , m_ulAvgBandwidth(0)
    , m_ulScheduledSendID(0)
    , m_pRecalcCallback(NULL)
    , m_pProc(pProc)
{
    m_pRecalcCallback = new BWCalculator::RecalcCallback(this);
    m_pRecalcCallback->AddRef();

    Timeval tCallback;
    Timeval tNow = m_pProc->pc->engine->now;
    tCallback.tv_sec = tNow.tv_sec + BANDWIDTH_RECALC_RATE;
    tCallback.tv_usec = tNow.tv_usec;

    m_ulScheduledSendID = m_pProc->pc->engine->
	ischedule.enter(tCallback, m_pRecalcCallback);
}

BWCalculator::~BWCalculator()
{
    if (m_ulScheduledSendID)
    {
        m_pProc->pc->engine->ischedule.remove(m_ulScheduledSendID);
    }
    HX_RELEASE(m_pRecalcCallback);

    if (!m_bNullSetup && !m_bBandwidthPending)
    {
	m_pProc->pc->server_info->
	    ChangeBandwidthUsage(-1 * (INT32)m_ulAvgBandwidth, m_pProc);
    }
}

STDMETHODIMP_(ULONG32) 
BWCalculator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
BWCalculator::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

void
BWCalculator::CommitPendingBandwidth()
{
    m_bBandwidthPending = FALSE;

    m_pProc->pc->server_info->ChangeBandwidthUsage(m_ulAvgBandwidth, m_pProc);
}

void
BWCalculator::PacketSent(ServerPacket* pPacket)
{
    HX_ASSERT(pPacket);
    pPacket->AddRef();

    m_ulTotalBytesSent += pPacket->GetSize();

    HX_RELEASE(pPacket);
}

void 
BWCalculator::BytesSent	    (UINT32 ulBytes)
{
    m_ulTotalBytesSent += ulBytes;
}

void
BWCalculator::RecalcAvgBandwidth()
{
    m_ulScheduledSendID = 0;

    // Don't adjust our bandwidth if we received a NULL setup (RTSP),
    // or we are still waiting for a play request. If we never receive
    // a play request, the content is being served from a downstream
    // cache via PNA.
    if (!m_bNullSetup && !m_bBandwidthPending)
    {
	m_pProc->pc->server_info->
	    ChangeBandwidthUsage(-1 * (INT32)m_ulAvgBandwidth, m_pProc);
    }

    if(m_ulElapsedTime)
    {
	m_ulAvgBandwidth = (m_ulTotalBytesSent * 8) / 
			   (m_ulElapsedTime / 1000);
    }

    // Don't report this bandwidth if we received a NULL setup (RTSP),
    // or we are still waiting for a play request. If we never receive
    // a play request, the content is being served from a downstream
    // cache via PNA.
    if (!m_bNullSetup && !m_bBandwidthPending)
    {
	m_pProc->pc->server_info->ChangeBandwidthUsage(m_ulAvgBandwidth, m_pProc);
    }

    // Schedule another callback
    Timeval tCallback;
    Timeval tNow = m_pProc->pc->engine->now;
    tCallback.tv_sec = tNow.tv_sec + BANDWIDTH_RECALC_RATE;
    tCallback.tv_usec = tNow.tv_usec;

    m_ulScheduledSendID = m_pProc->pc->engine->
	ischedule.enter(tCallback, m_pRecalcCallback);
}

BWCalculator::RecalcCallback::RecalcCallback(BWCalculator* pBWCalculator)
{
    m_pBWCalculator = pBWCalculator;
}

BWCalculator::RecalcCallback::~RecalcCallback()
{
}

STDMETHODIMP
BWCalculator::RecalcCallback::Func()
{
    m_pBWCalculator->m_ulElapsedTime += (BANDWIDTH_RECALC_RATE * 1000);
    m_pBWCalculator->RecalcAvgBandwidth();

    return HXR_OK;
}
