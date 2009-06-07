/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: loadinfo.cpp,v 1.4 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "server_engine.h"
#include "server_info.h"
#include "base_errmsg.h"
#include "loadinfo.h"


extern UINT32* g_pAggregateRequestedBitRate;
extern UINT32* g_pNAKCounter;


LoadInfo::LoadInfo(Process* pProc)
{
    m_pProc = pProc;
    m_LoadState = NormalLoad;
    m_ulStreamerForceSelect = 0;
    m_ulStreamerIterations = 0;
    m_ulFuncCounter = 0;
    m_ulOverload = 0;
    m_tLastForce = Timeval(0);
    m_tLastOverload = Timeval(0);
    m_ulExtremeStartedSeconds = 0;
    m_bNAKImplosionState = FALSE;
    m_ulNumberOfExtremeLoads = 0;
    m_bReportedExtremeLoad = FALSE;
}


/* This callback is automatically run by OncePerSecond::Func() */  
STDMETHODIMP
LoadInfo::Func()
{
    m_ulFuncCounter++;

    /* Run only every 3 seconds */
    if (m_ulFuncCounter >= 3)
    {
	/* Handle Server Load */
	UINT32 ulStreamerCount = m_pProc->pc->streamer_info->Number();

	BOOL bForce = ((m_ulStreamerForceSelect / ulStreamerCount) >= 5) ||
	    (((m_ulStreamerIterations / ulStreamerCount) < 7) &&
	    m_ulStreamerForceSelect >= 2);

	BOOL bOverload = m_ulOverload > 2 && m_ulOverload >=
	    (m_pProc->pc->server_info->m_ClientCount * 0.05);

	Timeval now = m_pProc->pc->engine->now;

	if (bForce)
	{
	    m_tLastForce = now;
	}

	if (bOverload)
	{
	    m_tLastOverload = now;
	}

	INT32 lTotalTrans =
	    m_pProc->pc->server_info->m_TCPTransCount +
	    m_pProc->pc->server_info->m_UDPTransCount +
	    m_pProc->pc->server_info->m_MulticastTransCount;
	INT32 lTotalClient =
	    m_pProc->pc->server_info->m_PNAClientCount +
	    m_pProc->pc->server_info->m_RTSPClientCount +
	    m_pProc->pc->server_info->m_MMSClientCount +
	    m_pProc->pc->server_info->m_HTTPClientCount;

	if (bOverload && bForce)
	{
	    m_ulNumberOfExtremeLoads++;
	    if (m_ulNumberOfExtremeLoads == 3 &&
		m_bReportedExtremeLoad == FALSE)
	    {
#ifndef DISABLE_LOADSTATE_MESSAGES
		ERRMSG(m_pProc->pc->error_handler,
"The server has entered High Capacity mode, this may result in lowered \
quality of service for some players.  The current load is %0.01f Mbps total \
output to %ld players (%ld%% PNA, %ld%% RTSP, %ld%% MMS, %ld%% HTTP, %ld%% TCP/Cloaked, \
%ld%% UDP, %ld%% MCast)\n",
		    *g_pAggregateRequestedBitRate / (1000.0 * 1024.0),
		    m_pProc->pc->server_info->m_ClientCount,
		    lTotalClient ?
			100 * m_pProc->pc->server_info->m_PNAClientCount /
			lTotalClient : 0,
		    lTotalClient ?
			100 * m_pProc->pc->server_info->m_RTSPClientCount /
			lTotalClient : 0,
		    lTotalClient ?
			100 * m_pProc->pc->server_info->m_MMSClientCount /
			lTotalClient : 0,
		    lTotalClient ?
			100 * m_pProc->pc->server_info->m_HTTPClientCount /
			lTotalClient : 0,
		    lTotalTrans ? 100 * m_pProc->pc->server_info->m_TCPTransCount / lTotalTrans : 0,
		    lTotalTrans ? 100 * m_pProc->pc->server_info->m_UDPTransCount / lTotalTrans : 0,
		    lTotalTrans ? 100 * m_pProc->pc->server_info->m_MulticastTransCount
			/ lTotalTrans : 0);
#endif //DISABLE_LOADSTATE_MESSAGES
		m_bReportedExtremeLoad = TRUE;
	    }
	}
	else
	{
	    m_ulNumberOfExtremeLoads = 0;
	}

	if (bOverload && bForce && m_LoadState != ExtremeLoad)
	{
	    m_LoadState = ExtremeLoad;

	    m_ulExtremeStartedSeconds = now.tv_sec;
	}
	else if (bForce && m_LoadState == NormalLoad)
	{
	    m_LoadState = HighLoad;
	}
	else if (m_LoadState == ExtremeLoad &&
		 m_tLastOverload < now - Timeval(300.0))
	{
	    m_LoadState = HighLoad;

	    if (m_bReportedExtremeLoad)
	    {
#ifndef DISABLE_LOADSTATE_MESSAGES
		ERRMSG(m_pProc->pc->error_handler,
"The server has exited High Capacity mode after %ld minutes.  If your server \
was running in this mode for a significant period we recommend you upgrade \
your system hardware to better support the load.",
		(now.tv_sec - m_ulExtremeStartedSeconds) / 60);
#endif //DISABLE_LOADSTATE_MESSAGES
		m_bReportedExtremeLoad = FALSE;
	    }
	}
	else if (m_LoadState == HighLoad &&
		 m_tLastForce < now - Timeval(330.0) &&
		 m_tLastOverload < now - Timeval(330.0))
	{
	    m_LoadState = NormalLoad;
	}

	m_ulStreamerForceSelect = 0;
	m_ulStreamerIterations = 0;
	m_ulFuncCounter = 0;
	m_ulOverload = 0;

	/* Handle NAK Implosion State */
	if (*g_pNAKCounter > (UINT32)lTotalClient * 9)
	{
	    /*
	     * If we see about 3 incoming NAKs per second per player,
	     * then we should consider the possibility of NAK implosion
	     * for live players going off the deep end of the live ring buffer.
	     */
	    m_bNAKImplosionState = TRUE;
	}
	else
	{
	    m_bNAKImplosionState = FALSE;
	}
	*g_pNAKCounter = 0;
    }

    return HXR_OK;
}
