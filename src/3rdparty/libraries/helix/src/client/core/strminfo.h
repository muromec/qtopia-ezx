/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: strminfo.h,v 1.13 2007/07/06 21:58:12 jfinnecy Exp $
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

#ifndef _STREAM_INFO_
#define _STREAM_INFO_

#include "hxmon.h"
#include "statinfo.h"
#include "hxbufstate.h"

struct	IHXASMProps;
class   HXStream;

struct PacketInfo // info about packets statistics requested
{
    PacketInfo()
    {
	m_bRequested	= FALSE;
	m_ulTimestamp	= 0;
	m_ulRequestTime	= 0;
	m_ulSequence	= 0;
	m_pPacket	= 0;
	m_bIsPreSeek	= FALSE;
    };

    HX_BITFIELD	m_bRequested : 1;
    HX_BITFIELD	m_bIsPreSeek : 1;
    UINT32	m_ulTimestamp;	
    UINT32	m_ulRequestTime;
    UINT32	m_ulSequence;
    IHXPacket*	m_pPacket;
};

// the STREAM_INFO struct stores the info for each media stream
struct STREAM_INFO
{
    STREAM_INFO();

    ~STREAM_INFO();

    void ResetPostEndTimeEventList();
    void ResetPreReconnectEventList();
    void ResetPosReconnectEventList();

    CHXEventList	m_EventList;			// list to store IHXPacket for each stream...
    CHXEventList*	m_pPostEndTimeEventList;
    IHXValues*		m_pHeader;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    STREAM_STATS*	m_pStats;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    // reconnect stuff
    HX_BITFIELD		m_bReconnectToBeDone : 1;
    UINT32		m_ulReconnectOverlappedPackets;
    CHXSimpleList*	m_pPreReconnectEventList;
    CHXEventList*	m_pPosReconnectEventList;
    
    ULONG32		m_ulReceived;
    ULONG32		m_ulLost;
    ULONG32		m_ulLastPacketTime;
   
    UINT32		m_ulDelay; 
    UINT32		m_lLoopCount;

    ULONG32		m_ulDuration;

    HXBufferingState& BufferingState() {return m_bufferingState;}
    void UpdateStartTimes();
    void OnStream(HXStream* pStream);
    void SetSeekPerformed() {m_bufferingState.SetSeekPerformed();}
    void UpdatePreroll(ULONG32 ulPreroll){m_bufferingState.UpdatePreroll(ulPreroll);UpdateStartTimes();}
private:
    HXBufferingState    m_bufferingState;

public:
    ULONG32		m_ulTimeBeforeSeek;
    ULONG32		m_ulTimeAfterSeek;
    
    IHXValues*		m_pStreamProps;
    HXStream*		m_pStream;

    UINT16		m_uStreamNumber;
    /* Buffer Status reported by the renderer*/
    UINT8		m_unNeeded;
    UINT8		m_unAvailable;

    HX_RESULT           m_streamEndReasonCode;

    HX_BITFIELD		m_bSrcStreamDone : 1;		// No more packets for this stream...
    HX_BITFIELD		m_bSrcInfoStreamDone : 1;
    HX_BITFIELD		m_bSrcStreamFillingDone : 1;	// Filling Done for this pass
    HX_BITFIELD		m_bSrcInfoStreamFillingDone : 1;
    HX_BITFIELD		m_bPacketRequested : 1;		// is the packet requested from 
    HX_BITFIELD		m_bCustomEndTime : 1;
    HX_BITFIELD		m_bCanBeStoppedAnyTime : 1;
    HX_BITFIELD		m_bClientRateAdapt : 1;
};

/*
 * used by RTSP to keep track of streams
 */

class RTSP_STREAM_INFO
{
public:
    RTSP_STREAM_INFO();
    ~RTSP_STREAM_INFO();

    // registry statistics
    UINT32		m_ulClipBandwidth;
    UINT16		m_uStreamNumber;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    STREAM_STATS*	m_pStreamStats;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
};

#endif // _STREAM_INFO_
