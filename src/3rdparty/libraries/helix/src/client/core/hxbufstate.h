/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbufstate.h,v 1.21 2007/01/11 19:53:31 milko Exp $
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
#ifndef HXBUFSTATE_H
#define HXBUFSTATE_H

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxsmbw.h" // IHXASMProps
#include "hxslist.h" // CHXSimpleList
#include "hxplayvelocity.h" // For HX_IS_1X_PLAYBACK() macro
#include "ihxpckts.h"

class HXBufferingState
{
public:
    HXBufferingState();
    ~HXBufferingState();

    void OnStreamHeader(UINT32     ulStreamNum,
                        UINT32     ulPreroll,
			UINT32     ulPredata,
			HXBOOL     preDataAtStart,
			HXBOOL     preDataAfterSeek,
			HXBOOL     prerollAtStart,
			HXBOOL     prerollAfterSeek,
			ULONG32    ulAvgBitRate,
                        ULONG32    ulMaxBitRate,
                        IHXBuffer* pMimeTypeStr);

    void OnStream(IUnknown* pStream);

    void Init(ULONG32 ulPerfectPlayTime);
    
    void SetMinimumPreroll(UINT32 ulSourcePreroll, 
			   UINT32 ulPerfectPlayTime,
			   HXBOOL   bIsRebuffering);
    void Stop();
    void Reset(HXBOOL bIsSeeking, UINT32 ulSeekTime);
    void GetRemainToBuffer(REF(UINT32)	ulRemainToBufferInMs,
			   REF(UINT32)	ulRemainToBuffer);

    ULONG32 GetMinPrerollInMs() const { return m_ulMinimumPrerollInMs;}
    ULONG32 GetMinBufferingInMs() const { return m_ulMinimumBufferingInMs;}
    UINT16 GetPercentDone();

    void UpdatePreroll(ULONG32 ulPreroll);

    INT64 CreateINT64Timestamp(UINT32 ulTime) const;

    void OnPacket(UINT32 ulTimestamp, UINT32 ulPacketSize, 
		  UINT32 ulElapsedTime, HXBOOL bIsLive,
		  HXBOOL bIsBufferedPlayMode);

    void UpdateBufferingInMs(INT64 llRefLowTimestamp,
			     INT64 llHighTimestamp, 
			     HXBOOL bIsBufferedPlayMode,
			     HXBOOL bIsTimestampDelivery,
			     UINT32 ulElapsedTime);

    void UpdateTransportStats(UINT32 ulLowTSAtTransport,
			      UINT32 ulHighTSAtTransport,
			      ULONG32 ulBytesAtTransport,
			      HXBOOL bDoneAtTransport);

    void GetExcessBufferInfo(UINT32 ulTheLowestTS,
			     UINT32 ulTheHighestTS,
			     REF(UINT32) ulRemainToBufferInMs,
			     REF(UINT32) ulRemainToBuffer,
			     REF(UINT32) ulExcessBufferInMs,
			     REF(UINT32) ulExcessBuffer,
			     REF(UINT32) ulExcessForThisStreamInMs,
			     REF(UINT32) ulExcessForThisStream);
    
    UINT32 LastPacketTimestamp() const {return m_ulLastPacketTimeStamp;}
    
    INT64 LowTS() { return m_llLowestTimeStamp;}
    INT64 HighTS() { return m_llHighestTimeStamp; }

    UINT32 AvgBandwidth();
    HXBOOL DoneAtTransport() const { return ((m_ulNumBytesAtTransport == 0) &&
                                           m_bDoneAtTransport);}
    void SetSeekPerformed() { m_bIsSeekPerformed = 1;}
    void SetVelocity(INT32 lVel)     { m_lPlaybackVelocity = lVel;  }
    void SetKeyFrameMode(HXBOOL bMode) { m_bKeyFrameMode     = bMode; }

    void OnResumed(void);
    void UpdateElapsedTime(UINT32 ulElapsedTime);
    HXBOOL AwaitingFirstPacket() { return m_bIsFirstPacket; }

    void SetWallclockDelay(INT32 lDelay) { m_lWallclockDelay = lDelay; }

private:
    HXBOOL hasPreroll();
    HXBOOL hasWallclockPreroll();
    HXBOOL hasPredata();

    void setupMinPrerollAndBuffering(ULONG32 ulMinPrerollInMs, 
                                     ULONG32 ulMinBufferingInMs);
    void updateCurrentBufferingInMs(INT64 llLowestTimeStamp, 
				    INT64 llHighestTimeStamp);

    HXBOOL dataBasedPreroll() { return (m_preDataAtStart || m_preDataAfterSeek);}

    ULONG32 msToBytes(UINT32 ulValueInMs, UINT32 ulBw) const;
    ULONG32 bytesToMs(UINT32 ulValueInBytes, UINT32 ulBw) const;
    
    void calcRemainingToBufferInMs(ULONG32 ulElapsedTime);
    void calcRemainingToBuffer(ULONG32 ulDenom);

    UINT32 calcRemaining(UINT32 ulMinimumBuffering,
			 UINT32 ulCurrentBuffering,
			 UINT32 ulMinimumPreroll,
			 UINT32 ulDenom) const;

    void startBuffering(HXBOOL bIsRebuffering);
    void stopBuffering();
    UINT32 getPredata();
    UINT32 maxBandwidth();

    void setRemainingToBufferInMs(UINT32 uRemainingToBufferInMs);
    void setRemainingToBuffer(UINT32 uRemainingToBuffer);
    void forceSwitchToTimeBasedBuffering(HXBOOL bSwitchTo);

    UINT32              m_ulStreamNum; 
    UINT32		m_ulPreroll;
    UINT32		m_ulPredata;

    UINT32		m_ulMinimumPrerollInMs;
    UINT32		m_ulMinimumPreroll;

    UINT32		m_ulMinimumBufferingInMs;
    UINT32		m_ulMinimumBuffering;

    UINT32		m_ulRemainingToBufferInMs;
    UINT32		m_ulRemainingToBuffer;

    UINT32		m_ulCurrentBufferingInMs;
    UINT32		m_ulCurrentBuffering;

    HX_BITFIELD		m_bIsFirstPacket : 1;
    HX_BITFIELD		m_preDataAtStart : 1;
    HX_BITFIELD		m_prerollAtStart : 1;
    HX_BITFIELD		m_preDataAfterSeek : 1;
    HX_BITFIELD		m_prerollAfterSeek : 1;
    HX_BITFIELD		m_wallClockAtStart : 1;
    HX_BITFIELD		m_wallClockAfterSeek : 1;
    HX_BITFIELD		m_bIsSeekPerformed : 1;
    HX_BITFIELD         m_bSavedPreDataAtStart : 1;
    HX_BITFIELD         m_bSavedPrerollAtStart : 1;
    HX_BITFIELD         m_bSavedPreDataAfterSeek : 1;
    HX_BITFIELD         m_bSavedPrerollAfterSeek : 1;
    HX_BITFIELD         m_bSavedWallClockAtStart : 1;
    HX_BITFIELD         m_bSavedWallClockAfterSeek : 1;
    HX_BITFIELD         m_bForcedSwitchToTimeBasedBuffering : 1;
    HX_BITFIELD         m_bKeyFrameMode : 1;
    HX_BITFIELD         m_bIsAudio : 1;
    HX_BITFIELD         m_bBufferedPlayMode : 1;

    INT64		m_llLowestTimeStamp;
    INT64		m_llHighestTimeStamp;

    UINT32		m_ulLowestTimestampAtTransport; 
    UINT32		m_ulHighestTimestampAtTransport; 
    UINT32		m_ulNumBytesAtTransport;
    HXBOOL		m_bDoneAtTransport;

    UINT32		m_ulTSRollOver;
    ULONG32		m_ulLastPacketTimeStamp;
    ULONG32		m_ulAvgBandwidth;
    ULONG32		m_ulMaxBandwidth;
    IHXASMProps*	m_pASMProps;
    INT32               m_lPlaybackVelocity;
    INT32               m_lWallclockDelay;
    IHXBuffer*          m_pMimeTypeStr;
};

inline
HXBOOL HXBufferingState::hasPreroll() 
{
    HXBOOL bRet = (m_bIsSeekPerformed ? m_prerollAfterSeek : m_prerollAtStart);
    bRet       |= hasWallclockPreroll();
    return bRet;
}

inline
HXBOOL HXBufferingState::hasWallclockPreroll()
{
    HXBOOL bRet = FALSE;

    // Buffered-play mode or any TrickPlay mode effectively 
    // disables wallclock-based buffering
    if (!m_bBufferedPlayMode && HX_IS_1X_PLAYBACK(m_lPlaybackVelocity))
    {
        bRet = (m_bIsSeekPerformed ? m_wallClockAfterSeek : m_wallClockAtStart);
    }

    return bRet;
}

inline
HXBOOL HXBufferingState::hasPredata() 
{
    return (m_bIsSeekPerformed) ? (m_preDataAfterSeek) : (m_preDataAtStart);
}


#endif /* HXBUFSTATE_H */
