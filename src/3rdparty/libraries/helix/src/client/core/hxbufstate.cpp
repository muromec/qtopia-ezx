/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbufstate.cpp,v 1.30 2007/01/11 19:53:31 milko Exp $
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
#include "hxbufstate.h"

#include "hxassert.h"
#include "hxplayvelocity.h"
#include "hxtlogutil.h"


#define MAX_UINT32_AS_DOUBLE	((double) MAX_UINT32)


HXBufferingState::HXBufferingState()
    : m_ulStreamNum(0)
    , m_ulPreroll(0)
    , m_ulPredata(0)
    , m_ulMinimumPrerollInMs(0)
    , m_ulMinimumPreroll(0)
    , m_ulMinimumBufferingInMs(0)
    , m_ulMinimumBuffering(0)
    , m_ulRemainingToBufferInMs(0)
    , m_ulRemainingToBuffer(0)
    , m_ulCurrentBufferingInMs(0)
    , m_ulCurrentBuffering(0)
    , m_bIsFirstPacket(TRUE)
    , m_preDataAtStart(FALSE)
    , m_prerollAtStart(FALSE)
    , m_preDataAfterSeek(FALSE)
    , m_prerollAfterSeek(FALSE)
    , m_wallClockAtStart(FALSE)
    , m_wallClockAfterSeek(FALSE)
    , m_bIsSeekPerformed(FALSE)
    , m_bSavedPreDataAtStart(FALSE)
    , m_bSavedPrerollAtStart(FALSE)
    , m_bSavedPreDataAfterSeek(FALSE)
    , m_bSavedPrerollAfterSeek(FALSE)
    , m_bSavedWallClockAtStart(FALSE)
    , m_bSavedWallClockAfterSeek(FALSE)
    , m_bForcedSwitchToTimeBasedBuffering(FALSE)
    , m_bKeyFrameMode(FALSE)
    , m_bIsAudio(FALSE)
    , m_bBufferedPlayMode(FALSE)
    , m_llLowestTimeStamp(0)
    , m_llHighestTimeStamp(0)
    , m_ulLowestTimestampAtTransport(0)
    , m_ulHighestTimestampAtTransport(0)
    , m_ulNumBytesAtTransport(0)
    , m_bDoneAtTransport(FALSE)
    , m_ulTSRollOver(0)
    , m_ulLastPacketTimeStamp(0)
    , m_ulAvgBandwidth(0)
    , m_ulMaxBandwidth(0)
    , m_pASMProps(NULL)
    , m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
    , m_lWallclockDelay(0)
    , m_pMimeTypeStr(NULL)
{}

HXBufferingState::~HXBufferingState()
{
    HX_RELEASE(m_pASMProps);
    HX_RELEASE(m_pMimeTypeStr);
}

void HXBufferingState::OnStreamHeader(UINT32     ulStreamNum,
                                      UINT32     ulPreroll,
				      UINT32     ulPredata,
				      HXBOOL     preDataAtStart,
				      HXBOOL     preDataAfterSeek,
				      HXBOOL     prerollAtStart,
				      HXBOOL     prerollAfterSeek,
				      ULONG32    ulAvgBitRate,
                                      ULONG32    ulMaxBitRate,
                                      IHXBuffer* pMimeTypeStr)
{
    HXLOGL1(HXLOG_CORE, "HXBufferingState[%s-%p]::OnStreamHeader()\n"
                        "\t\tStreamNum=%lu PreRoll=%lu PreData=%lu PDStart=%d PDSeek=%d "
                        "PRStart=%d PRSeek=%d AvgBitRate=%lu MaxBitRate=%lu mimeType=%s",
            (pMimeTypeStr ? (const char*) pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulStreamNum, ulPreroll, ulPredata, preDataAtStart, preDataAfterSeek,
            prerollAtStart, prerollAfterSeek, ulAvgBitRate, ulMaxBitRate,
			(pMimeTypeStr ? (const char*) pMimeTypeStr->GetBuffer() : "(null)"));

    m_ulStreamNum = ulStreamNum;
    m_ulPreroll = ulPreroll;
    m_ulPredata = ulPredata;

    // These are used by GIF, JPEG, PNA, and some VBR RV
    // GIF and JPEG set m_preDataAtStart and m_prerollAfterSeek
    // PNA uses different settings for different mimetypes
    //   - Flash sets m_preDataAtStart, m_prerollAtStart, and m_prerollAfterSeek
    //   - RA sets m_preDataAtStart
    //   - RV sets m_preDataAtStart and m_prerollAtStart
    //   - other mimetypes set m_prerollAtStart and m_prerollAfterSeek
    m_preDataAtStart = preDataAtStart;
    m_preDataAfterSeek = preDataAfterSeek;
    m_prerollAtStart = prerollAtStart;
    m_prerollAfterSeek = prerollAfterSeek;
    m_ulAvgBandwidth = ulAvgBitRate;
    m_ulMaxBandwidth = ulMaxBitRate;

    if (m_ulAvgBandwidth > m_ulMaxBandwidth)
    {
        m_ulMaxBandwidth = m_ulAvgBandwidth;
    }

    // Use wallclock preroll always unless:
    // a) predata was explicitly requested; and
    // b) preroll was explicitly NOT requested; and
    // c) stream is constant bitrate (maxBandwidth == avgBandwidth)
    m_wallClockAtStart   = TRUE;
    m_wallClockAfterSeek = TRUE;
    if (m_ulAvgBandwidth == m_ulMaxBandwidth)
    {
        if (m_preDataAtStart && !m_prerollAtStart)
        {
            m_wallClockAtStart = FALSE;
        }
        if (m_preDataAfterSeek && !m_prerollAfterSeek)
        {
            m_wallClockAfterSeek = FALSE;
        }
    }

    // Determine if this is an audio stream
    if (pMimeTypeStr)
    {
        // Save the mime type string
        HX_RELEASE(m_pMimeTypeStr);
        m_pMimeTypeStr = pMimeTypeStr;
        m_pMimeTypeStr->AddRef();
        // Determine if this strema is audio. For TrickPlay-mode
        // buffering, we need to ignore the normal buffering requirements
        // for audio streams. Therefore, we need to know
        // if a stream is audio.
        const char* pszAudioPrefix = "audio";
        const char* pszMimeType    = (const char*) pMimeTypeStr->GetBuffer();
        if (pszMimeType &&
            !strncmp(pszMimeType, pszAudioPrefix, strlen(pszAudioPrefix)))
        {
            // The mime type begins with "audio/", so 
            // we assume this is an audio stream
            m_bIsAudio = TRUE;
        }
    }
}

void HXBufferingState::OnStream(IUnknown* pStream)
{
    HXLOGL2(HXLOG_CORE, "HXBufferingState[%s-%p]::OnStream(pStream=0x%08x)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, pStream);
    HX_RELEASE(m_pASMProps);

    if (pStream)
    {
	pStream->QueryInterface(IID_IHXASMProps, (void**) &m_pASMProps);
    }
}

void HXBufferingState::Init(ULONG32 ulPerfectPlayTime)
{
    HXLOGL2(HXLOG_CORE, "HXBufferingState[%s-%p]::Init(ulPerfectPlayTime=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulPerfectPlayTime);
    UINT32 ulMinBufferingInMs = m_ulPreroll;

#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    // Perfect play factor is only added for 
    // non-MIN_HEAP builds
    ulMinBufferingInMs += ulPerfectPlayTime;
#endif

    setupMinPrerollAndBuffering(m_ulPreroll, ulMinBufferingInMs);
    startBuffering(FALSE);
}


void HXBufferingState::SetMinimumPreroll(UINT32 ulSourcePreroll, 
					 UINT32 ulPerfectPlayTime,
					 HXBOOL   bIsRebuffering)
{
    HXLOGL2(HXLOG_CORE, "HXBufferingState[%s-%p]::SetMinimumPreroll(ulSourcePreroll=%lu,ulPerfectPlayTime=%lu,bIsRebuffering=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulSourcePreroll, ulPerfectPlayTime, bIsRebuffering);
    UINT32 ulMinimumPreroll = m_ulPreroll;

    if (ulMinimumPreroll < ulSourcePreroll)
    {
	ulMinimumPreroll = ulSourcePreroll;
    }
    
    UINT32 ulMinBufferingInMs = ulMinimumPreroll;

#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    // Perfect play factor is only added for 
    // non-MIN_HEAP builds
    ulMinBufferingInMs += ulPerfectPlayTime;
#endif

    setupMinPrerollAndBuffering(ulMinimumPreroll, ulMinBufferingInMs);
    startBuffering(bIsRebuffering);
}

void HXBufferingState::Stop()
{
    HXLOGL2(HXLOG_CORE, "HXBufferingState[%s-%p]::Stop()",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"), this);
    stopBuffering();
}

void HXBufferingState::Reset(HXBOOL bIsSeeking, UINT32 ulSeekTime)
{
    HXLOGL2(HXLOG_CORE, "HXBufferingState[%s-%p]::Reset(bIsSeeking=%lu,ulSeekTime=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, bIsSeeking, ulSeekTime);
    startBuffering(FALSE);

    m_ulTSRollOver = 0;
    m_ulLastPacketTimeStamp = 0;
    m_bIsFirstPacket = TRUE;

    if (bIsSeeking)
    {
	m_llLowestTimeStamp = CreateINT64Timestamp(ulSeekTime);
	m_llHighestTimeStamp = CreateINT64Timestamp(ulSeekTime);
    }

    // If we get a Reset() and we are in non-1x playback AND
    // in keyframe mode AND we are in data-based buffering, then
    // we should switch to timestamp-based buffering. If we had
    // switched to timestamp-based buffering and we are now
    // back in all-frames mode, then switch back.
    // XXXMEH - this assumes that we always seek when we change
    // velocities, and that we have set the velocity before the
    // actual seek occurs.
    HXBOOL bNeedTimeBasedBuffering = ((m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL &&
                                     m_bKeyFrameMode) ? TRUE : FALSE);
    if (bNeedTimeBasedBuffering && hasPredata() && !m_bForcedSwitchToTimeBasedBuffering)
    {
        // We need time-based buffering but don't have it,
        // so force a switch to time-based buffering
        forceSwitchToTimeBasedBuffering(TRUE);
    }
    else if (!bNeedTimeBasedBuffering && m_bForcedSwitchToTimeBasedBuffering)
    {
        // We have time-based buffering but don't need it,
        // so switch back from time-based buffering
        forceSwitchToTimeBasedBuffering(FALSE);
    }
}

void HXBufferingState::GetRemainToBuffer(REF(UINT32) ulRemainToBufferInMs,
					 REF(UINT32) ulRemainToBuffer)
{
    HXBOOL bHasPreroll = hasPreroll();
    HXBOOL bHasPredata = hasPredata();
    
    ulRemainToBufferInMs = 0;
    ulRemainToBuffer = 0;

    // Handle timestamp based buffering
    if (!bHasPredata || bHasPreroll)
    {
        ulRemainToBufferInMs = m_ulRemainingToBufferInMs;
    }

    // Handle data based buffering
    if (bHasPredata)
    {
        ulRemainToBuffer = m_ulRemainingToBuffer;
    }
    // If this is an audio stream and we are in
    // non-1X playback, then currently the audio renderer
    // will not decode any data, so we don't have any
    // buffering requirements for audio
    if (m_bIsAudio && HX_IS_NON1X_PLAYBACK(m_lPlaybackVelocity))
    {
        ulRemainToBufferInMs = 0;
        ulRemainToBuffer = 0;
    }
    HXLOGL4(HXLOG_CORE, "HXBufferingState[%s-%p]::GetRemainToBuffer(ulRemainToBufferInMs=%lu,ulRemainToBuffer=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulRemainToBufferInMs, ulRemainToBuffer);
}

UINT16 HXBufferingState::GetPercentDone()
{
    UINT32  ulTotalPercentDone   = 100;
    UINT32  ulPreDataPercentDone = 100;
    UINT32  ulPrerollPercentDone = 100;

    HXBOOL bHasPreroll = hasPreroll();
    HXBOOL bHasPredata = hasPredata();
    HXBOOL bNeedsData  = FALSE;

    // Compute percent done via data-based buffering
    if (m_ulMinimumBuffering)
    {
        ulPreDataPercentDone = 
            ((m_ulMinimumBuffering - m_ulRemainingToBuffer) * 100) /
            m_ulMinimumBuffering;
    }
    // Compute percent done via preroll-based buffering
    if (m_ulMinimumBufferingInMs)
    {
        ulPrerollPercentDone =
            ((m_ulMinimumBufferingInMs-m_ulRemainingToBufferInMs ) * 100) /
            m_ulMinimumBufferingInMs;
    }

    if (bHasPredata && !bHasPreroll && m_ulAvgBandwidth == m_ulMaxBandwidth)
    {
        // These are constant-bitrate streams for which
        // have explicit instructions to use PreData only. For these
        // streams, we will ONLY use predata and not any
        // form of preroll-based buffering (timestamp or
        // wallclock).
        ulTotalPercentDone = ulPreDataPercentDone;
        // Was there any data remaining to buffer?
        bNeedsData = (m_ulRemainingToBuffer ? TRUE : FALSE);
    }
    else if (!bHasPredata && (0 == m_ulMaxBandwidth) && (0 == m_ulAvgBandwidth))        
    {
        // If bandwidth is not available for minimum buffering data to be
        // computed based on preroll time and pre-data consideration is not
        // a requirement, we will ONLY use preroll based buffering 
        HX_ASSERT(0 == m_ulMinimumBuffering);
        ulTotalPercentDone = ulPrerollPercentDone;
        bNeedsData         = (m_ulRemainingToBufferInMs ? TRUE : FALSE);
    }
    else
    {
        // We will use the max of preroll and predata
        if (ulPrerollPercentDone > ulPreDataPercentDone)
        {
            ulTotalPercentDone = ulPrerollPercentDone;
            bNeedsData         = (m_ulRemainingToBufferInMs ? TRUE : FALSE);
        }
        else
        {
            ulTotalPercentDone = ulPreDataPercentDone;
            bNeedsData         = (m_ulRemainingToBuffer ? TRUE : FALSE);
        }
    }

    if (m_bIsSeekPerformed && ulTotalPercentDone == 100 && bNeedsData)
    {
        ulTotalPercentDone = 99;
    }

    // If this is an audio stream and we are in
    // non-1X playback, then currently the audio renderer
    // will not decode any data, so we don't have any
    // buffering requirements for audio
    if (m_bIsAudio && HX_IS_NON1X_PLAYBACK(m_lPlaybackVelocity))
    {
        ulTotalPercentDone = 100;
    }
    if (ulTotalPercentDone == 100)
    {
        // When we reach 100 percent, we want
        // to make sure all the m_ulRemainingToBufferXXX 
        // variables are set to 0. This prevents code that
        // is only looking at these variables from 
        // thinking we are buffering when we actually 
        // aren't
        stopBuffering();
    }

    HXLOGL3(HXLOG_CORE, "HXBufferingState[%s-%p]::GetPercentDone() return %lu",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulTotalPercentDone);
    return (UINT16) ulTotalPercentDone;
}

void HXBufferingState::UpdatePreroll(ULONG32 ulPreroll)
{
    HXLOGL2(HXLOG_CORE, "HXBufferingState[%s-%p]::UpdatePreroll(ulPreroll=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulPreroll);
    UINT32 ulExtraPrerollInMs   = m_ulMinimumPrerollInMs - m_ulPreroll;
    UINT32 ulExtraBufferingInMs = m_ulMinimumBufferingInMs - m_ulPreroll;
    
    m_ulPreroll = ulPreroll;
    setupMinPrerollAndBuffering(ulPreroll + ulExtraPrerollInMs,
                                ulPreroll + ulExtraBufferingInMs);

    // Notice that we don't call startBuffering() here.
    // This allows us to count any data we have already 
    // received as part of the new preroll

    calcRemainingToBufferInMs(0);
    calcRemainingToBuffer(0);
}

INT64 HXBufferingState::CreateINT64Timestamp(UINT32 ulTime) const
{
    return CAST_TO_INT64 (m_ulTSRollOver) * CAST_TO_INT64 MAX_UINT32 + CAST_TO_INT64 ulTime;
}

void HXBufferingState::OnPacket(UINT32 ulTimestamp, UINT32 ulPacketSize,
				UINT32 ulElapsedTime,
				HXBOOL bIsLive, HXBOOL bIsBufferedPlayMode)
{
    HXLOGL4(HXLOG_CORE, "HXBufferingState[%s-%p]::OnPacket(ts=%lu,size=%lu,elaptime=%lu,live=%lu,bufferedPlayMode=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulTimestamp, ulPacketSize, ulElapsedTime, bIsLive, bIsBufferedPlayMode);
    //  0xFA .. 0xFF [roll over] (0x01)
    if (m_ulLastPacketTimeStamp > ulTimestamp &&
	((m_ulLastPacketTimeStamp - ulTimestamp) > MAX_TIMESTAMP_GAP))
    {
	m_ulTSRollOver++;
    }
    
    INT64 llActualTimeStamp = CreateINT64Timestamp(ulTimestamp);
    m_ulLastPacketTimeStamp = ulTimestamp;

    // Save the buffered play mode. We need to
    // know this since being in buffered play
    // mode effectively disables wallclock-based
    // buffering.
    m_bBufferedPlayMode = bIsBufferedPlayMode;

    if (m_bIsFirstPacket)
    {
        if (m_lPlaybackVelocity < 0)
        {
            m_llLowestTimeStamp = llActualTimeStamp;
        }
        else
        {
	    m_llHighestTimeStamp = llActualTimeStamp;
        }
	m_bIsFirstPacket = FALSE;
    }

    if (m_lPlaybackVelocity < 0)
    {
        if (llActualTimeStamp < m_llLowestTimeStamp)
        {
	    m_llLowestTimeStamp = llActualTimeStamp;
        }
    }
    else
    {
        if (llActualTimeStamp >= m_llHighestTimeStamp)
        {
	    m_llHighestTimeStamp = llActualTimeStamp;
        }
    }

    // Handle data based buffering stuff
    // Only count this packet if the timestamp is
    // greater than the m_llLowestTimeStamp (for forward
    // velocity) or less than m_llHighestTimeStamp
    // for reverse playback). This prevents packets
    // which have a timestamp less than the seek
    // time from being counted towards data-based
    // buffering. When we seek, both m_llLowestTimeStamp
    // and m_llHighestTimeStamp are set to the 
    // seek time.
    HXBOOL bAddPacket = TRUE;
    if (m_lPlaybackVelocity < 0)
    {
	if (llActualTimeStamp > m_llHighestTimeStamp)
	{
	    bAddPacket = FALSE;
	}
    }
    else
    {
	if (llActualTimeStamp < m_llLowestTimeStamp)
	{
	    bAddPacket = FALSE;
	}
    }
    if (bAddPacket)
    {
        m_ulCurrentBuffering += ulPacketSize;
    }

    UINT32 ulDenom = 0;

    if (bIsBufferedPlayMode)
    {
        /* 
         * We wait to have at least 1 second worth of data
         * before doing any calculations. This may need some
         * tweaking.
         */
        UINT32 ulBandwidth = AvgBandwidth();

        if (m_ulRemainingToBuffer &&
            (m_ulCurrentBuffering >= ulBandwidth / 8))
        {
            /* 
             * Highly unlikely, but may happen from a server on the 
             * local machine.
             */
            if (!ulElapsedTime)
            {
                if (m_ulCurrentBuffering >= m_ulMinimumPreroll)
                {
                    setRemainingToBuffer(0);
                }
            }
            else
            {
                ulDenom = msToBytes(ulElapsedTime, ulBandwidth);
            
                /* Sanity check - may be 0 only when bandwidth
                 * is really low 
                 */
                if (!ulDenom)
                {
                    ulDenom = 1;
                }
            }
        }
    }

    if (m_ulRemainingToBuffer)
    {
        calcRemainingToBuffer(ulDenom);
    }
    HXLOGL4(HXLOG_CORE, "\tcurBuf=%lu curBufMs=%lu remBuf=%lu remBufMs=%lu",
            m_ulCurrentBuffering, m_ulCurrentBufferingInMs,
            m_ulRemainingToBuffer, m_ulRemainingToBufferInMs);
}

void HXBufferingState::UpdateBufferingInMs(INT64 llRefLowTimestamp,
					   INT64 llHighTimestamp, 
					   HXBOOL bIsBufferedPlayMode,
					   HXBOOL bIsTimestampDelivery,
					   UINT32 ulElapsedTime)
{
    HXLOGL4(HXLOG_CORE, "HXBufferingState[%s-%p]::UpdateBufferingInMs(loTS=%I64d,hiTS=%I64d,bufferedPlayMode=%lu,tsDeliv=%lu,elaptime=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, HX_I64d_ARG(llRefLowTimestamp), HX_I64d_ARG(llHighTimestamp), bIsBufferedPlayMode, bIsTimestampDelivery, ulElapsedTime);
    UINT32 ulCalcElapsedTime = 0;

    if (hasWallclockPreroll())
    {
        INT32 lBufferingInMs = ulElapsedTime - m_lWallclockDelay;
        if (lBufferingInMs < 0)
        {
            lBufferingInMs = 0;
        }
        // In wallclock mode the current buffering is equal
        // the the elapsed buffering time minus the wallclock delay
        m_ulCurrentBufferingInMs = (UINT32) lBufferingInMs;
    }
    else
    {
        // Update m_ulCurrentBufferingInMs based on packet timestamps
        updateCurrentBufferingInMs(llRefLowTimestamp, llHighTimestamp);
    }

    if (bIsBufferedPlayMode && m_ulRemainingToBufferInMs)
    {
	/* We handle time stamp delivered streams differently
	 * in case of Buffered/PerfectPlay. This is because
	 * server sends timestamp delivered stream based
	 * on the timestamps on the packet and pretty
	 * much streams in realtime. 
	 * So even if we are on LAN and streaming a RealText
	 * file, we will get packets in realtime even though
	 * we have enough bandwidth available
	 * 
	 * For timestamp delivered streams, we just fulfill 
	 * preroll.
	 */
	if (bIsTimestampDelivery &&
	    m_ulCurrentBufferingInMs > m_ulMinimumPrerollInMs)
	{
            setRemainingToBufferInMs(0);
	}
	else if (m_ulRemainingToBufferInMs &&
		 (m_ulCurrentBufferingInMs >= 1000))
	{
            ulCalcElapsedTime = ulElapsedTime;

	    /* 
	     * Highly unlikely, but may happen from a 
	     * server on the local machine.
	     */

	    if ((ulElapsedTime == 0) &&
                (m_ulCurrentBufferingInMs >= m_ulMinimumPrerollInMs))
            {
                setRemainingToBufferInMs(0);
	    }
	}
    }

    if (m_ulRemainingToBufferInMs)
    {
        calcRemainingToBufferInMs(ulCalcElapsedTime);
    }
    HXLOGL4(HXLOG_CORE, "\tcurBuf=%lu curBufMs=%lu remBuf=%lu remBufMs=%lu",
            m_ulCurrentBuffering, m_ulCurrentBufferingInMs,
            m_ulRemainingToBuffer, m_ulRemainingToBufferInMs);
}

void HXBufferingState::UpdateTransportStats(UINT32 ulLowTSAtTransport,
					    UINT32 ulHighTSAtTransport,
					    ULONG32 ulBytesAtTransport,
					    HXBOOL bDoneAtTransport)
{
    HXLOGL4(HXLOG_CORE, "HXBufferingState[%s-%p]::UpdateTransportStats(loTS=%lu,hiTS=%lu,bytes=%lu,bDone=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulLowTSAtTransport, ulHighTSAtTransport, ulBytesAtTransport, bDoneAtTransport);
    m_ulLowestTimestampAtTransport = ulLowTSAtTransport; 
    m_ulHighestTimestampAtTransport = ulHighTSAtTransport; 
    m_ulNumBytesAtTransport = ulBytesAtTransport;
    m_bDoneAtTransport = bDoneAtTransport;
}

void HXBufferingState::GetExcessBufferInfo(UINT32 ulTheLowestTS,
					   UINT32 ulTheHighestTS,
					   REF(UINT32) ulRemainToBufferInMs,
					   REF(UINT32) ulRemainToBuffer,
					   REF(UINT32) ulExcessBufferInMs,
					   REF(UINT32) ulExcessBuffer,
					   REF(UINT32) ulExcessForThisStreamInMs,
					   REF(UINT32) ulExcessForThisStream)
{
    HXLOGL4(HXLOG_CORE, "HXBufferingState[%s-%p]::GetExcessBufferInfo(loTS=%lu,hiTS=%lu,rtbMs=%lu,rtb=%lu,xsbufMs=%lu,xsbuf=%lu,xsstrmMs=%lu,xsstrm=%lu)",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, ulTheLowestTS, ulTheHighestTS, ulRemainToBufferInMs, ulRemainToBuffer,
            ulExcessBufferInMs, ulExcessBuffer, ulExcessForThisStreamInMs, ulExcessForThisStream);
    UINT32 ulCurBufInMs	             = 0;
    UINT32 ulRemainForThisStreamInMs = 0;
    UINT32 ulRemainForThisStream     = 0;
    
    ulExcessForThisStreamInMs = 0;
    ulExcessForThisStream     = 0;	
    
    if (m_ulNumBytesAtTransport > 0)
    {	
	ulCurBufInMs = ulTheHighestTS - m_ulLowestTimestampAtTransport;
    }
    else
    {	
	ulCurBufInMs = ulTheHighestTS - ulTheLowestTS;
    }

    if (ulCurBufInMs <= m_ulMinimumBufferingInMs)
    {
	ulRemainForThisStreamInMs = m_ulMinimumBufferingInMs - ulCurBufInMs;
    }
    else
    {
	ulExcessForThisStreamInMs = ulCurBufInMs - m_ulMinimumBufferingInMs;
    }

    if (m_ulNumBytesAtTransport <= m_ulMinimumBuffering)
    {
	ulRemainForThisStream = 
	    m_ulMinimumBuffering - m_ulNumBytesAtTransport;
    }
    else
    {
	ulExcessForThisStream = m_ulNumBytesAtTransport - m_ulMinimumBuffering;
    }

    
    // Update global valued based on values from 
    // this stream
    HXBOOL bHasPreroll = hasPreroll();
    HXBOOL bHasPredata = hasPredata();
	
    // Handle timestamp based buffering variables
    if (!bHasPredata || bHasPreroll)
    {
	if (ulRemainToBufferInMs < ulRemainForThisStreamInMs)
	{
	    ulRemainToBufferInMs = ulRemainForThisStreamInMs;
	}
	
	if (ulExcessBufferInMs < ulExcessForThisStreamInMs)
	{
	    ulExcessBufferInMs = ulExcessForThisStreamInMs;
	}
    }

    // Handle predata based buffering variables
    if (bHasPredata)
    {
	ulRemainToBuffer += ulRemainForThisStream;
	ulExcessBuffer   += ulExcessForThisStream;
    }
}

UINT32 HXBufferingState::AvgBandwidth()
{
    UINT32 ulBandwidth;

    if (m_pASMProps &&
	(HXR_OK == m_pASMProps->GetBandwidth(ulBandwidth)) &&
        ulBandwidth)
    {
	m_ulAvgBandwidth = ulBandwidth;

        // Update the max if needed
        if (m_ulAvgBandwidth > m_ulMaxBandwidth)
        {
            m_ulMaxBandwidth = m_ulAvgBandwidth;
        }
    }

    return m_ulAvgBandwidth;
}

void 
HXBufferingState::OnResumed(void)
{
    HXLOGL2(HXLOG_CORE, "HXBufferingState[%s-%p]::OnResumed %i %lu",
            (m_pMimeTypeStr ? (const char*) m_pMimeTypeStr->GetBuffer() : "(null)"),
            this, m_ulStreamNum, (m_ulNumBytesAtTransport + m_ulCurrentBuffering));
}

void HXBufferingState::UpdateElapsedTime(UINT32 ulElapsedTime)
{
    // Updating elapsed time only matters if
    // we are using wallclock preroll
    if (hasWallclockPreroll())
    {
        INT32 lBufferingInMs = ulElapsedTime - m_lWallclockDelay;
        if (lBufferingInMs < 0)
        {
            lBufferingInMs = 0;
        }
        // In wallclock mode the current buffering is equal
        // the the elapsed buffering time
        m_ulCurrentBufferingInMs = (INT32) lBufferingInMs;
        // Currently wallclock preroll and buffered play mode
        // are mutually exclusive. If that ever changes, then
        // we will need the block of code from UpdateBufferingInMs()
        // which begins:
        //
        // if (bIsBufferedPlayMode && m_ulRemainingToBufferInMs)
        //
        // to be here as well.
        if (m_ulRemainingToBufferInMs)
        {
            calcRemainingToBufferInMs(0);
        }
    }
}

void HXBufferingState::setupMinPrerollAndBuffering(ULONG32 ulMinPrerollInMs, 
                                                   ULONG32 ulMinBufferingInMs)
{
    m_ulMinimumPreroll = getPredata();
    m_ulMinimumBuffering = m_ulMinimumPreroll;

    m_ulMinimumPrerollInMs    = m_ulPreroll;
    m_ulMinimumBufferingInMs  = m_ulMinimumPrerollInMs;

    // Streams without buffering requirement, always stay without buffering
    // requirement.
    // For example, a very sparse streams such as event stream cannot have its
    // buffering modified (increased) as the concept of buffering does not apply.
    if (m_ulMinimumPreroll || m_ulMinimumBufferingInMs)
    {
	UINT32 ulMaxBandwidth     = maxBandwidth();
	UINT32 ulAvgBandwidth     = AvgBandwidth();
    
	// If we have bandwidth, we make sure data based and time based
	// buffering is consistent and modifed the data based buffering based
	// on the passed in time buffering values.
	if (ulAvgBandwidth && ulMaxBandwidth)
	{
	    UINT32 ulPredataFromPrerollInMs = msToBytes(m_ulMinimumPrerollInMs,
							ulMaxBandwidth);

	    if (ulPredataFromPrerollInMs > m_ulMinimumPreroll)
	    {
		m_ulMinimumPreroll = ulPredataFromPrerollInMs;
		m_ulMinimumBuffering = ulPredataFromPrerollInMs;
	    }
	    else
	    {
		UINT32 ulPrerollInMsFromPredata = bytesToMs(m_ulMinimumPreroll,
							    ulMaxBandwidth);

		if (ulPrerollInMsFromPredata > MAX_INT32)
		{
		    // Keep time based properties under max signed size to
		    // allow proper time expiration detection and disambiguate
		    // wrap-around for potential reative time comparisons.
		    ulPrerollInMsFromPredata = MAX_INT32;
		}

		if (ulPrerollInMsFromPredata > m_ulMinimumPrerollInMs)
		{
		    m_ulMinimumPrerollInMs = ulPrerollInMsFromPredata;
		    m_ulMinimumBufferingInMs = ulPrerollInMsFromPredata;
		}
	    }

	    UINT32 ulExtraPrerollInMs = 0;
	    UINT32 ulExtraBufferingInMs = 0;

	    // This is the media preroll that must be delivered to
	    // the renderer can start.
	    if (ulMinPrerollInMs > m_ulMinimumPrerollInMs)
	    {
		ulExtraPrerollInMs = ulMinPrerollInMs - m_ulMinimumPrerollInMs;
		ulExtraBufferingInMs = ulMinPrerollInMs - m_ulMinimumBufferingInMs;

		m_ulMinimumPrerollInMs = ulMinPrerollInMs;
		m_ulMinimumBufferingInMs = ulMinPrerollInMs;
	    }

	    // Minimum buffering is always greater or equeal than media preroll
	    // above and indicates the total amount of buffering desired.
	    if (ulMinBufferingInMs > m_ulMinimumBufferingInMs)
	    {
		ulExtraBufferingInMs = ulMinBufferingInMs - m_ulMinimumBufferingInMs;

		m_ulMinimumBufferingInMs = ulMinBufferingInMs;
	    }

	    // Increase the predata values correspondingly to the time based
	    // preroll increase.
	    m_ulMinimumPreroll += msToBytes(ulExtraPrerollInMs, 
					    ulAvgBandwidth);
        
	    m_ulMinimumBuffering += msToBytes(ulExtraBufferingInMs, 
					      ulAvgBandwidth);
	}
	else
	{
	    m_ulMinimumPrerollInMs    = ulMinPrerollInMs;
	    m_ulMinimumBufferingInMs  = ulMinBufferingInMs;
	}
    }
}

void HXBufferingState::updateCurrentBufferingInMs(INT64 llLowestTimeStamp, 
						  INT64 llHighestTimeStamp)
{
    INT64 llLowTimestamp = m_llLowestTimeStamp;
    INT64 llHighTimestamp = m_llHighestTimeStamp;
    
    if (m_bIsFirstPacket)
    {
	/* use the reference stream's lowest timestamp for calculation */
        if (m_lPlaybackVelocity < 0)
        {
            llHighTimestamp = llHighestTimeStamp;
        }
        else
        {
	    llLowTimestamp = llLowestTimeStamp;
        }
    }

    if (llHighestTimeStamp > llLowTimestamp)
    {
	// if the stream has been continuesly playing for 49 days
	// we will set m_ulCurrentBufferingInMs to MAX_UINT32
	if (llHighestTimeStamp - llLowTimestamp > MAX_UINT32)
	{
	    m_ulCurrentBufferingInMs = MAX_UINT32;
	}
	else
	{
	    m_ulCurrentBufferingInMs = 
		INT64_TO_UINT32(llHighestTimeStamp - llLowTimestamp);
	}
    }
}

ULONG32 HXBufferingState::msToBytes(ULONG32 ulValueInMs, ULONG32 ulBw) const
{
    UINT32 ulRetBytes = MAX_UINT32;
    double dBytes = ((double) ulValueInMs) * ((double) ulBw) / 8000.0;

    if (dBytes < MAX_UINT32_AS_DOUBLE)
    {
	ulRetBytes = (UINT32) dBytes;
    }

    return ulRetBytes;
}

ULONG32 HXBufferingState::bytesToMs(ULONG32 ulValueInBytes, ULONG32 ulBw) const
{
    UINT32 ulRetMs = MAX_UINT32;
    double dMs = (((double) ulValueInBytes) * 8000.0) / ((double) ulBw);

    if (dMs < MAX_UINT32_AS_DOUBLE)
    {
	ulRetMs = (UINT32) dMs;
    }

    return ulRetMs;
}

void HXBufferingState::calcRemainingToBufferInMs(ULONG32 ulElapsedTime)
{
    setRemainingToBufferInMs(calcRemaining(m_ulMinimumBufferingInMs,
                                           m_ulCurrentBufferingInMs,
                                           m_ulMinimumPrerollInMs,
                                           ulElapsedTime));
}

void HXBufferingState::calcRemainingToBuffer(ULONG32 ulDenom)
{
    setRemainingToBuffer(calcRemaining(m_ulMinimumBuffering,
                                       m_ulCurrentBuffering,
                                       m_ulMinimumPreroll,
                                       ulDenom));
}

UINT32 HXBufferingState::calcRemaining(UINT32 ulMinimumBuffering,
				       UINT32 ulCurrentBuffering,
				       UINT32 ulMinimumPreroll,
				       UINT32 ulDenom) const
{
    UINT32 ulRemainingToBuffer = 0;

    if (ulMinimumBuffering > ulCurrentBuffering)
    {
	ulRemainingToBuffer = ulMinimumBuffering - ulCurrentBuffering;
    }

    if (ulDenom)
    {
	UINT32 ulBufferWhilePlaying = ulRemainingToBuffer;

	/*
	 * If ulDenom != 0, we compute remaining buffering in
	 * buffered playback mode.  Buffered playback mode is assumed
	 * in the data delivery modes that are not real-time
	 * delivery modes but rather download type modes and thus in
	 * which data can be delivered fast or slower than real-time.
	 * In such case buffering needs to meet the following criteria:
	 *  --> Current rate of data arrival at the current level of 
	 *      buffering must be sufficient for buffering level to be
	 *	above ulMinimumPreroll level for the specified ulMinimumBuffering 
	 *	interval assuming start of playback at current time.
	 */
	if (ulCurrentBuffering >= ulMinimumPreroll)
	{
	    UINT32 ulBufferingAboveMinPreroll = ulCurrentBuffering - ulMinimumPreroll;
	    double yNow = ((double) ulCurrentBuffering);
	    double xNow = ((double) ulDenom);
	    double playRate = (m_lPlaybackVelocity > 0) ? ((double) m_lPlaybackVelocity) : ((double) (-m_lPlaybackVelocity));
	    double depletionRate = playRate - yNow / xNow;

	    ulRemainingToBuffer = 0;
	    if (depletionRate > 0.0)
	    {
		ulCurrentBuffering = ((UINT32) (ulBufferingAboveMinPreroll / depletionRate));

		if (ulMinimumBuffering > ulCurrentBuffering)
		{
		    ulRemainingToBuffer = ulMinimumBuffering - ulCurrentBuffering;
		}
	    }
	}
	else
	{
	    HX_ASSERT(ulRemainingToBuffer != 0);
	    if (ulRemainingToBuffer == 0)
	    {
		ulRemainingToBuffer = 1;
	    }
	}
    }

    return ulRemainingToBuffer;
}

void HXBufferingState::startBuffering(HXBOOL bIsRebuffering)
{
    setRemainingToBufferInMs(m_ulMinimumBufferingInMs);
    setRemainingToBuffer(m_ulMinimumBuffering);
    m_ulCurrentBufferingInMs = 0;
    m_ulCurrentBuffering = 0;

    /* If we have received at least one packet for this stream,
     * mark the lowest timestamp to be the
     * last packet timstamp to reset buffering calculations
     */
    if (bIsRebuffering && !m_bIsFirstPacket)
    {
	m_bIsFirstPacket = TRUE;
        if (m_lPlaybackVelocity < 0)
        {
            m_llHighestTimeStamp = CreateINT64Timestamp(m_ulLastPacketTimeStamp); 
        }
        else
        {
	    m_llLowestTimeStamp = CreateINT64Timestamp(m_ulLastPacketTimeStamp);
        }
    }

}

void HXBufferingState::stopBuffering()
{
    m_ulRemainingToBufferInMs = 0;
    m_ulRemainingToBuffer = 0;
}

UINT32 HXBufferingState::getPredata()
{
    UINT32 ulTmp = 0;
    if (m_pASMProps &&
        (HXR_OK == m_pASMProps->GetPreData(ulTmp)) &&
        ulTmp)
    {
        m_ulPredata = ulTmp;
    }

    return m_ulPredata;
}

UINT32 HXBufferingState::maxBandwidth()
{
    UINT32 uAvgBandwidth = AvgBandwidth();
            
    if (uAvgBandwidth > m_ulMaxBandwidth)
    {
        m_ulMaxBandwidth = uAvgBandwidth;
    }

    return m_ulMaxBandwidth;
}

void HXBufferingState::setRemainingToBufferInMs(UINT32 uRemainingToBufferInMs)
{
    if (m_ulRemainingToBufferInMs && !uRemainingToBufferInMs &&
        hasWallclockPreroll())
    {
        // In the wallclock based buffering case we want to
        // stop all buffering when m_ulRemainingToBufferInMs
        // transitions from non-zero to zero
        stopBuffering();
    }
    else
    {
        m_ulRemainingToBufferInMs = uRemainingToBufferInMs;
    }
}

void HXBufferingState::setRemainingToBuffer(UINT32 uRemainingToBuffer)
{
    if (m_ulRemainingToBuffer && !uRemainingToBuffer &&
        hasWallclockPreroll())
    {
        // In the wallclock based buffering case we want to
        // stop all buffering when m_ulRemainingToBuffer
        // transitions from non-zero to zero
        stopBuffering();
    }
    else
    {
        m_ulRemainingToBuffer = uRemainingToBuffer;
    }
}

void HXBufferingState::forceSwitchToTimeBasedBuffering(HXBOOL bSwitchTo)
{
    if (bSwitchTo && !m_bForcedSwitchToTimeBasedBuffering)
    {
        // We want to switch to time-based buffering and
        // we haven't already done so.
        //
        // Save a copy of all the relevant HXBOOL members
        m_bSavedPrerollAtStart     = m_prerollAtStart;
        m_bSavedPrerollAfterSeek   = m_prerollAfterSeek;
        m_bSavedPreDataAtStart     = m_preDataAtStart;
        m_bSavedPreDataAfterSeek   = m_preDataAfterSeek;
        m_bSavedWallClockAtStart   = m_wallClockAtStart;
        m_bSavedWallClockAfterSeek = m_wallClockAfterSeek;
        // Set them to use time based buffering
        m_prerollAtStart     = TRUE;
        m_prerollAfterSeek   = TRUE;
        m_preDataAtStart     = FALSE;
        m_preDataAfterSeek   = FALSE;
        m_wallClockAtStart   = FALSE;
        m_wallClockAfterSeek = FALSE;
        // Set the flag saying we've switched
        m_bForcedSwitchToTimeBasedBuffering = TRUE;
    }
    else if (!bSwitchTo && m_bForcedSwitchToTimeBasedBuffering)
    {
        // We want to switch out of time-based buffering and
        // we have previously switched to time-based buffering
        //
        // Return all the relevant HXBOOL members to the original values
        m_prerollAtStart     = m_bSavedPrerollAtStart;
        m_prerollAfterSeek   = m_bSavedPrerollAfterSeek;
        m_preDataAtStart     = m_bSavedPreDataAtStart;
        m_preDataAfterSeek   = m_bSavedPreDataAfterSeek;
        m_wallClockAtStart   = m_bSavedWallClockAtStart;
        m_wallClockAfterSeek = m_bSavedWallClockAfterSeek;
        // Clear all the saved values. Not really necessary,
        // but we'll do it for consistency
        m_bSavedPrerollAtStart     = FALSE;
        m_bSavedPrerollAfterSeek   = FALSE;
        m_bSavedPreDataAtStart     = FALSE;
        m_bSavedPreDataAfterSeek   = FALSE;
        m_bSavedWallClockAtStart   = FALSE;
        m_bSavedWallClockAfterSeek = FALSE;
        // Set them to use time based buffering
        // Clear the flag saying we've switched
        m_bForcedSwitchToTimeBasedBuffering = FALSE;
    }
}

