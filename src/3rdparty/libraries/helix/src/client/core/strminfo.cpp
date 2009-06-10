/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: strminfo.cpp,v 1.13 2007/07/06 21:58:12 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#include "hxcom.h"
#include "hlxclib/stdio.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "chxeven.h"
#include "chxelst.h"
#include "strminfo.h"
#include "hxsmbw.h"
#include "hxcomm.h"
#include "hxcore.h"
#include "hxstrm.h"
#include "hxbsrc.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

/*
 * STREAM_INFO
 */

STREAM_INFO::STREAM_INFO()
{
    m_pHeader			= NULL;
    m_bSrcStreamDone		= FALSE;
    m_bSrcInfoStreamDone	= FALSE;
    m_bSrcStreamFillingDone	= FALSE;
    m_bSrcInfoStreamFillingDone	= FALSE;
    m_bPacketRequested		= FALSE;
    m_bCustomEndTime		= FALSE;
    m_unNeeded			= 0;
    m_unAvailable		= 0;
    m_ulReceived		= 0;
    m_ulLost			= 0;
    m_ulLastPacketTime          = 0;
    m_pPostEndTimeEventList	= NULL;    

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)    
    m_pStats			= NULL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    m_ulDelay			= 0;
    m_pStreamProps		= NULL;

    m_uStreamNumber		= 0;
    m_ulDuration		= 0;
    m_ulTimeBeforeSeek		= 0;
    m_ulTimeAfterSeek		= 0;

    m_pStream			    = NULL;
    m_bCanBeStoppedAnyTime	    = FALSE;
    m_bClientRateAdapt = TRUE;

    // reconnect stuff
    m_pPreReconnectEventList = NULL;
    m_pPosReconnectEventList = NULL;
    m_bReconnectToBeDone = FALSE;
    m_ulReconnectOverlappedPackets = 0;

    m_streamEndReasonCode = HXR_OK;
};

STREAM_INFO::~STREAM_INFO()
{
    ResetPostEndTimeEventList();
    ResetPreReconnectEventList();
    ResetPosReconnectEventList();

    if (m_pStreamProps)
    {
	m_pStreamProps->Release();
	m_pStreamProps = 0;
    }

    if (m_pHeader)
    {
	m_pHeader->Release();
	m_pHeader = 0;
    }

    HX_RELEASE(m_pStream);

    /* Reason we do not delete m_pStats here is because
     * in FileSource we create/delete it while in NetSource
     * case, Protocol creates/deletes it
     */
    /*
    if (m_pStats)
    {
	delete m_pStats;
	m_pStats = 0;
    }
    */

    while (m_EventList.GetNumEvents() > 0)
    {
	CHXEvent* pEvent = m_EventList.RemoveHead();
	HX_DELETE (pEvent);
    }   
};

void
STREAM_INFO::ResetPostEndTimeEventList()
{
    if (m_pPostEndTimeEventList)
    {
	while (m_pPostEndTimeEventList->GetNumEvents() != 0)
	{
	    CHXEvent*  pEvent = (CHXEvent*)m_pPostEndTimeEventList->RemoveHead();
	    HX_DELETE(pEvent);
	}
	m_pPostEndTimeEventList->RemoveAll();

	HX_DELETE(m_pPostEndTimeEventList);
    }

    return;
}

void 
STREAM_INFO::ResetPreReconnectEventList()
{
    if (m_pPreReconnectEventList)
    {
	while (m_pPreReconnectEventList->GetCount() > 0)
	{
	    UINT32* pPacketTime = (UINT32*)m_pPreReconnectEventList->RemoveHead();
	    HX_DELETE (pPacketTime);
	}
	HX_DELETE(m_pPreReconnectEventList);
    }

    return;
}

void 
STREAM_INFO::ResetPosReconnectEventList()
{
    if (m_pPosReconnectEventList)
    {
	while (m_pPosReconnectEventList->GetNumEvents() > 0)
	{
	    CHXEvent* pEvent = m_pPosReconnectEventList->RemoveHead();
	    HX_DELETE (pEvent);
	}
	HX_DELETE(m_pPosReconnectEventList);
    }

    return;
}



void STREAM_INFO::UpdateStartTimes()
{
    LISTPOSITION position = m_EventList.GetHeadPosition();
    // HXSource::CalcEventTime makes use of m_bufferingState.GetMinBufferingInMs().
    // It is important to use the same value here to maintain the event order
    // for any packets that are added after the adjustment of timestartPos below.

    HXSource* pSource = NULL;
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    IHXPlayer* pPlayer = NULL;
    IHXPlaybackVelocity* pPlayBackVelocity = NULL;
#endif
    INT32 lPlaybackVelocity = HX_PLAYBACK_VELOCITY_NORMAL;

    if (m_pStream != NULL)
    {
        pSource = m_pStream->GetHXSource();
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
        if (pSource != NULL)
        {
            HRESULT hr = HXR_OK;
            hr = pSource->QueryInterface(IID_IHXPlayer, (void**)&pPlayer);
            if (SUCCEEDED(hr))
            {
                hr = pPlayer->QueryInterface(IID_IHXPlaybackVelocity, (void**)&pPlayBackVelocity);
                if (SUCCEEDED(hr))
                {
                    lPlaybackVelocity = pPlayBackVelocity->GetVelocity();
                }
            }
        }
#endif
    }

    while (position != NULL)
    {
        CHXEvent* theEvent = m_EventList.GetNext(position);
        IHXPacket* pPacket = theEvent->GetPacket();
        UINT32 ulEventStartTime = 0;
        if (pSource != NULL)
        {
            ulEventStartTime = pSource->CalcEventTime(this, pPacket->GetTime(), TRUE, lPlaybackVelocity);
        }
        theEvent->SetTimeStartPos(ulEventStartTime);
    }

    HX_RELEASE(pSource);
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    HX_RELEASE(pPlayer);
    HX_RELEASE(pPlayBackVelocity);
#endif
}

void STREAM_INFO::OnStream(HXStream* pStream)
{
    m_pStream = pStream;
    m_pStream->AddRef();

    m_bufferingState.OnStream((IHXStream*)pStream);
}

/*
 * RTSP_STREAM_INFO
 */

RTSP_STREAM_INFO::RTSP_STREAM_INFO()
{
    m_ulClipBandwidth 		= 0;
    m_uStreamNumber		= 0;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    m_pStreamStats		= 0;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

RTSP_STREAM_INFO::~RTSP_STREAM_INFO()
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    HX_DELETE(m_pStreamStats);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}
