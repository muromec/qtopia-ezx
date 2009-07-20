/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: buffmgr.cpp,v 1.41 2007/02/27 06:15:56 gbajaj Exp $
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

#include "hlxclib/stdio.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxslist.h"
#include "hxtick.h"
#include "hxcore.h"
#include "chxeven.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxntsrc.h"
#include "hxstrm.h"
#include "strminfo.h"
#include "buffmgr.h"
#include "hxsmbw.h"
#include "hxplayvelocity.h"
#include "hxprefutil.h"

#include "hxgroup.h"
#include "hxplay.h"
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(__TCS__)
#define MAX_ADDITIONAL_BUFFERING    3000
#else
#define MAX_ADDITIONAL_BUFFERING    10000
#endif /* __TCS__ */

#define DEFAULT_WALLCLOCK_PROOFING_DELAY 200 // ms

CBufferManager::CBufferManager(HXSource* pParent)
{
    m_ulMaxAdditionalBufferingInMs  = MAX_ADDITIONAL_BUFFERING;

    m_llHighestTimeStamp        = 0;
    m_llLowestTimeStamp         = MAX_UINT32;
    m_ulTotalPauseTime          = 0;
    m_ulLastPauseTick           = 0;
    m_ulAdditionalBufferingInMs = 0;
    m_bBufferStartTimeToBeSet   = TRUE;

    m_ulMinimumSourcePreroll    = 0;

    m_bPerfectPlay              = FALSE;     
    m_bPaused                   = FALSE;
    m_state                     = BUFFMGR_READY;

    m_pParent                   = pParent;
    m_pParent->AddRef();

    m_bLocalPlayback            = pParent->IsLocalSource();
    m_pStreamInfoTable          = pParent->GetStreamInfoTable();
    m_bFirstResumeDone          = FALSE;
    m_bIsSeekPerformed          = FALSE;
    m_bBufferedPlay             = FALSE;
    m_bBufferCalcToBeChanged    = FALSE;
    m_bIsInitialized            = FALSE;
    m_ulSeekTime                = 0;
    m_lPlaybackVelocity         = HX_PLAYBACK_VELOCITY_NORMAL;
    m_bKeyFrameMode             = FALSE;
    m_ulWallclockProofingDelay  = DEFAULT_WALLCLOCK_PROOFING_DELAY;

    ReadPrefs();
}

CBufferManager::~CBufferManager()
{
    HX_RELEASE(m_pParent);
}

HX_RESULT
CBufferManager::Init()
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::Init()", this);
    UINT32          ulPerfectPlayTime = 0;
    STREAM_INFO*    pStreamInfo = NULL;

    CHXMapLongToObj::Iterator i;

    // There is no buffered or perfect play with MIN_HEAP on.
#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    SetPerfectPlay(m_pParent->IsPerfectPlay());

    if (m_bPerfectPlay || m_bBufferedPlay)
    {
        // caculate how much extra preroll for the perfect play
        ulPerfectPlayTime = m_pParent->GetPerfectPlayTime();
    }
#endif

    // adjust buffering information of each stream
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);

        pStreamInfo->BufferingState().Init(ulPerfectPlayTime);
    }

    m_bIsInitialized = TRUE;

    /* Was there a Seek called before Initialization... may happen for a clip
     * with Start time
     */
    if (m_bIsSeekPerformed)
    {
        DoSeek(m_ulSeekTime);
    }

    return HXR_OK;
}

HX_RESULT
CBufferManager::SetMinimumPreroll(HXBOOL bPerfectPlay, UINT32 ulSourcePreroll, 
                                  HXBOOL   bModifyStartTime /* = TRUE */)
{
    m_ulMinimumSourcePreroll       = ulSourcePreroll;

    SetPerfectPlay(bPerfectPlay);

    UpdateMinimumPreroll(bModifyStartTime);

    return HXR_OK;
}

void 
CBufferManager::UpdateMinimumPreroll(HXBOOL bModifyStartTime)
{
    UINT32          ulPerfectPlayTime = 0;
    STREAM_INFO*    pStreamInfo = NULL;

    CHXMapLongToObj::Iterator i;

    // There is no buffered or perfect play with MIN_HEAP on.
#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)

    if (m_bPerfectPlay || m_bBufferedPlay)
    {
        // calculate how much extra preroll for the perfect play
        ulPerfectPlayTime = m_pParent->GetPerfectPlayTime();
    }
    else
    {
        if (m_ulAdditionalBufferingInMs > m_ulMaxAdditionalBufferingInMs)
        {
            m_ulAdditionalBufferingInMs = m_ulMaxAdditionalBufferingInMs;
        }

        ulPerfectPlayTime               = m_ulAdditionalBufferingInMs;
    }
#endif

    // adjust buffering information of each stream
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);
    
        pStreamInfo->BufferingState().SetMinimumPreroll(
            m_ulMinimumSourcePreroll, 
            ulPerfectPlayTime,
            (m_state == BUFFMGR_REBUFFER));

        if (bModifyStartTime)
        {
            pStreamInfo->UpdateStartTimes();
        }
    }

    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::UpdateMinimumPreroll(): start = %lu; min src preroll = %lu; pp time = %lu", 
        this, m_pParent->GetStartTime(), m_ulMinimumSourcePreroll, ulPerfectPlayTime);

    m_bBufferCalcToBeChanged = FALSE;
}

HX_RESULT
CBufferManager::Stop(void)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::Stop()", this);

    STREAM_INFO*    pStreamInfo = NULL;

    CHXMapLongToObj::Iterator i;

    // stop buffering of each stream
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);
        pStreamInfo->BufferingState().Stop();
    }

    return HXR_OK;
}

HX_RESULT
CBufferManager::DoSeek(UINT32 ulSeekTime, HXBOOL bSeekInsideRecordBuffer)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::DoSeek(): to = %ul", this, ulSeekTime);

    m_state             = BUFFMGR_SEEK;
    m_bIsSeekPerformed  = TRUE;

    CHXMapLongToObj::Iterator i;
    STREAM_INFO*    pStreamInfo = NULL;
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);
        pStreamInfo->SetSeekPerformed();
    }

    m_ulSeekTime        = ulSeekTime;

    /* We will call Reset during Init() call */
    if (!m_bIsInitialized)
    {
        return HXR_OK;
    }

    // reset all the preroll attributes
    Reset(ulSeekTime, bSeekInsideRecordBuffer);

    return HXR_OK;
}

HX_RESULT
CBufferManager::DoPause(void)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::DoPause()", this);
    m_bPaused           = TRUE;
    m_ulLastPauseTick   = HX_GET_BETTERTICKCOUNT();
    return HXR_OK;
}

HX_RESULT
CBufferManager::DoResume(void)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::DoResume()", this);
    if (m_bPaused && m_state != BUFFMGR_SEEK && !m_bBufferStartTimeToBeSet)
    {
        m_ulTotalPauseTime += CALCULATE_ELAPSED_TICKS(m_ulLastPauseTick, 
                                                      HX_GET_BETTERTICKCOUNT());
        HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::DoResume(): total pause time = %lu ", this, m_ulTotalPauseTime);
    }

    m_bPaused = FALSE;

    if (!m_bFirstResumeDone)
    {
        HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::DoResume(): first resume", this);
        m_bFirstResumeDone = TRUE;

        UpdateMinimumPreroll(FALSE);
    }

    return HXR_OK;
}

HX_RESULT
CBufferManager::ReBuffer(UINT32 uPrerollIncrement)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::ReBuffer(): inc = %lu", this, uPrerollIncrement);

    // recaculate only if it is out of the buffering stage
    // and not in perfect play mode
    m_state = BUFFMGR_REBUFFER;
    
    /* go back in buffering mode...
     * each time we come in buffering state, increase the
     * number of packets we buffer by uPrerollIncrement worth
     * to a max of m_ulMaxAdditionalBuffering secs.
     */
#ifndef HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES
    m_ulAdditionalBufferingInMs += uPrerollIncrement;

    if (m_pParent->IsLive())
    {
        m_bPerfectPlay  = FALSE;
        m_bBufferedPlay = FALSE;
    }
#endif // HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES

    // Set the flag saying that we need to reset the
    // initial starting wallclock time. Otherwise,
    // if we have a wallclock preroll stream, it
    // will probably instantly leave buffering, since
    // the elapsed time will be large if we have
    // been playing for some time before rebuffering.
    m_bBufferStartTimeToBeSet = TRUE;

    UpdateMinimumPreroll(FALSE);

    return HXR_OK;
}

HX_RESULT
CBufferManager::Reset(UINT32 ulSeekTime, HXBOOL bSeekInsideRecordBuffer)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::Reset(): seek time = %lu", this, ulSeekTime);

    STREAM_INFO*    pStreamInfo = NULL;

    CHXEventList*  pEventList = NULL;
    CHXEvent*       pEvent = NULL;

    CHXMapLongToObj::Iterator i;

    if (m_bBufferCalcToBeChanged)
    {
        UpdateMinimumPreroll(FALSE);
    }

    m_ulBufferingStartTick   = 0;
    m_ulBufferingStartTimeTS = 0;

    // reset each stream
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);
        
        // reset
        if (!bSeekInsideRecordBuffer)
        {
            pStreamInfo->m_bSrcStreamDone = FALSE;
            pStreamInfo->m_bPacketRequested = FALSE;
        }

        pStreamInfo->m_bSrcStreamFillingDone = FALSE;

        // Reset buffering state
        pStreamInfo->BufferingState().Reset((m_state == BUFFMGR_SEEK),
                                            ulSeekTime);

        

        HX_ASSERT(m_state == BUFFMGR_SEEK);

        if (m_state == BUFFMGR_SEEK)
        {
            // remove all pending packets 
            pEventList = &pStreamInfo->m_EventList;
            while (pEventList->GetNumEvents() > 0)
            {
                pEvent = pEventList->RemoveHead();

                // Mark it as a pre-seek event and send it to the player
                pEvent->SetPreSeekEvent();
                m_pParent->EventReady(pEvent);
            }
        }
    }

    m_llHighestTimeStamp        = 0;
    m_llLowestTimeStamp         = MAX_UINT32;
    m_ulTotalPauseTime          = 0;
    m_bBufferStartTimeToBeSet   = TRUE;
    return HXR_OK;
}
                                        
HX_RESULT
CBufferManager::UpdateCounters(IHXPacket* pPacket, INT32 lPacketTimeOffSet)
{
    HX_RESULT       hr                = HXR_OK;
    UINT32          ulStreamNum       = 0;
    UINT32          ulBufferSize      = 0;
    UINT32          ulBufferTime      = 0;
    UINT32          ulElapsedTime     = 0;
    UINT32          ulTmpElapsedTime  = 0;
    INT64           llActualTimeStamp = 0;
    IHXBuffer*      pBuffer           = NULL;
    STREAM_INFO*    pStreamInfo       = NULL;
    STREAM_INFO*    pThisStreamInfo   = NULL;
    UINT32          ulCurrentTick     = HX_GET_BETTERTICKCOUNT();


    CHXMapLongToObj::Iterator i;
   
    if (!pPacket)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (pPacket->IsLost() &&
        !(pPacket->GetASMFlags() & HX_ASM_DROPPED_PKT))
    {
        // Ignore all lost packets that aren't dropped packets
        return hr;
    }

    ulStreamNum = pPacket->GetStreamNumber();

    if (!m_pStreamInfoTable->Lookup(ulStreamNum, (void*&)pStreamInfo))
    {
        return HXR_INVALID_PARAMETER;
    }

    pThisStreamInfo = pStreamInfo;

    pBuffer = pPacket->GetBuffer();
    if (pBuffer)
    {
        ulBufferSize = pBuffer->GetSize();
    }
    HX_RELEASE(pBuffer);

    HX_ASSERT(pPacket->GetTime() + lPacketTimeOffSet >= 0);
    ulBufferTime = pPacket->GetTime() + lPacketTimeOffSet;

    // Do we need to set the time and tick of the
    // first overall packet to arrive?
    HXBOOL bFirstPacketOverall = FALSE;
    if (m_bBufferStartTimeToBeSet)
    {
        // This is the first packet to arrive from any
        // stream, so save the time and tick that the
        // first arriving packet over all streams.
        m_bBufferStartTimeToBeSet = FALSE;
        m_ulBufferingStartTick    = ulCurrentTick;
        m_ulBufferingStartTimeTS  = ulBufferTime;
        bFirstPacketOverall       = TRUE;
        // If we are live, then we will use the timestamp
        // of the first packet received as the start time.
        // So in order to communicate the start time to all
        // streams, we call HXBufferingState::Reset() on all streams.
        // This will force HXBufferingState::m_llLowestTimeStamp to be
        // equal to the timestamp of the first packet received
        // across ALL streams.
        if (m_pParent->IsLive())
        {
            for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
            {
                pStreamInfo = (STREAM_INFO*) (*i);
                if (pStreamInfo)
                {
                    pStreamInfo->BufferingState().Reset(TRUE, m_ulBufferingStartTimeTS);
                }
            }
        }
    }

    // Is this the first packet received for this stream?
    if (pThisStreamInfo->BufferingState().AwaitingFirstPacket())
    {
        // Compute the wallclock delay for this stream
        INT32 lDelay = ComputeWallclockDelay(ulBufferTime, ulCurrentTick);
        // Set the wallclock delay for this stream
        pThisStreamInfo->BufferingState().SetWallclockDelay(lDelay);
        // Log a message
        HXLOGL2(HXLOG_CORE, "CBufferManager[%p] Wallclock delay for stream %lu set to %ld",
                this, ulStreamNum, lDelay);
        // Are we supposed to set this wallclock delay
        // on all streams or just this packet's stream?
        if (bFirstPacketOverall)
        {
            // If we received the very first packet over
            // all streams, then we should set the wallclock
            // delay on all streams as a "best guess".
            for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
            {
                pStreamInfo = (STREAM_INFO*) (*i);
                if (pStreamInfo && pStreamInfo != pThisStreamInfo)
                {
                    pStreamInfo->BufferingState().SetWallclockDelay(lDelay);
                    // Log a message
                    HXLOGL2(HXLOG_CORE, "CBufferManager[%p] Wallclock delay for stream %u set to %ld",
                            this, pStreamInfo->m_uStreamNumber, lDelay);
                }
            }
        }
    }

    // Determine if we are in buffered play mode or not
    HXBOOL bIsBufferedPlayMode = m_bPerfectPlay || m_bBufferedPlay;

    // Get the elapsed time
    ulElapsedTime = GetElapsedTime(ulCurrentTick);
    // Determine whether or not to pass in elapsed time. If a stream
    // still has not received its first packet and the raw
    // elapsed time is still less than the wallclock proofing delay,
    // then pass in 0 for the elapsed time
    ulTmpElapsedTime = ulElapsedTime;
    if (pThisStreamInfo->BufferingState().AwaitingFirstPacket() &&
        ulElapsedTime < m_ulWallclockProofingDelay)
    {
        ulTmpElapsedTime = 0;
    }

    pThisStreamInfo->BufferingState().OnPacket(ulBufferTime,
                                               ulBufferSize,
                                               ulTmpElapsedTime,
                                               m_pParent->IsLive(),
                                               bIsBufferedPlayMode);

    llActualTimeStamp = 
        pThisStreamInfo->BufferingState().CreateINT64Timestamp(ulBufferTime);


    // time based preroll by default
    INT64 llLoTimeStamp = 0;
    INT64 llHiTimeStamp = 0;
    if (m_lPlaybackVelocity < 0)
    {
        UpdateLowestTimestamps(llActualTimeStamp, pThisStreamInfo);
        llLoTimeStamp = m_llLowestTimeStamp;
        llHiTimeStamp = pThisStreamInfo->BufferingState().HighTS();
    }
    else
    {
        UpdateHighestTimestamps(llActualTimeStamp, pThisStreamInfo);
        llLoTimeStamp = pThisStreamInfo->BufferingState().LowTS();
        llHiTimeStamp = m_llHighestTimeStamp;
    }

    // adjust each stream preroll based on the hightest time stamp
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);
        
        HXBOOL bIsTimestampDelivery = FALSE;

        if (pStreamInfo->m_pStream &&
            pStreamInfo->m_pStream->IsTimeStampDelivery())
        {
            bIsTimestampDelivery = TRUE;
        }
            
        // Determine whether or not to pass in elapsed time. If a stream
        // still has not received its first packet and the raw
        // elapsed time is still less than the wallclock proofing delay,
        // then pass in 0 for the elapsed time
        ulTmpElapsedTime = ulElapsedTime;
        if (pStreamInfo->BufferingState().AwaitingFirstPacket() &&
            ulElapsedTime < m_ulWallclockProofingDelay)
        {
            ulTmpElapsedTime = 0;
        }
        pStreamInfo->BufferingState().UpdateBufferingInMs(llLoTimeStamp,
                                                          llHiTimeStamp,
                                                          bIsBufferedPlayMode,
                                                          bIsTimestampDelivery,
                                                          ulTmpElapsedTime);
    }

    return hr;
}

HX_RESULT
CBufferManager::GetStatus(REF(UINT16)       uStatusCode,
                          REF(IHXBuffer*)  pStatusDesc,
                          REF(UINT16)       uPercentDone)
{
    STREAM_INFO*        pStreamInfo = NULL;

    CHXMapLongToObj::Iterator i;

    uStatusCode = HX_STATUS_READY;
    pStatusDesc = NULL;
    uPercentDone = 0;

    // Update the elapsed time on all the streams.
    // Elapsed time is also updated when we receive a packet,
    // but since there could be significant gaps between
    // packets, we also update it here in GetStatus(), which
    // is called regularly by the HXPlayer.
    UpdateElapsedTimeAllStreams();

    // collect from each streams
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);

        uPercentDone += pStreamInfo->BufferingState().GetPercentDone();
    }

    // average them
    uPercentDone = uPercentDone / m_pStreamInfoTable->GetCount();

    if (uPercentDone >= 100)
    {
        uPercentDone = 100;
        m_state = BUFFMGR_READY;
    }
    else
    {
        uStatusCode = HX_STATUS_BUFFERING;
    }

    return HXR_OK;
}

HX_RESULT
CBufferManager::GetRemainToBuffer(REF(UINT32)   ulRemainToBufferInMs,
                                  REF(UINT32)   ulRemainToBuffer)
{
    STREAM_INFO*    pStreamInfo = NULL;
    
    ulRemainToBufferInMs = 0;
    ulRemainToBuffer = 0;

    CHXMapLongToObj::Iterator i;

    // Set ulRemainToBufferInMs to the largest ulRemainInMs across all streams
    // Sum ulRemain for all streams and assign it to ulRemainToBuffer
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);

        UINT32 ulRemainInMs;
        UINT32 ulRemain;
        pStreamInfo->BufferingState().GetRemainToBuffer(ulRemainInMs, 
                                                        ulRemain);

        if (ulRemainToBufferInMs < ulRemainInMs)
        {
            ulRemainToBufferInMs = ulRemainInMs;
        }
        
        ulRemainToBuffer += ulRemain;
    }

    return HXR_OK;
}

HX_RESULT       
CBufferManager::GetMaximumPreroll(REF(UINT32)   ulMaximumPrerollInMs)
{
    STREAM_INFO*    pStreamInfo = NULL;

    CHXMapLongToObj::Iterator i;

    ulMaximumPrerollInMs = 0;

    // get max. preroll among the streams
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);
    
        UINT32 ulMinPrerollInMs = 
            pStreamInfo->BufferingState().GetMinPrerollInMs();

        if (ulMaximumPrerollInMs < ulMinPrerollInMs)
        {
            ulMaximumPrerollInMs = ulMinPrerollInMs;
        }
    }
    
    return HXR_OK;
}

HX_RESULT
CBufferManager::GetExcessBufferInfo(REF(UINT32) ulRemainToBufferInMs,
				    REF(UINT32) ulRemainToBuffer,
				    REF(UINT32) ulExcessBufferInMs,
				    REF(UINT32) ulExcessBuffer,
				    REF(HXBOOL) bValidInfo,
				    REF(UINT32) ulActualExcessBufferInMs,
				    REF(UINT32) ulActualExcessBuffer)
{
    STREAM_INFO*    pStreamInfo         = NULL;
    UINT32	    ulTheLowestTS       = 0;
    UINT32	    ulTheHighestTS      = 0;
    HXBOOL          bIsFirst            = TRUE;
    STREAM_INFO*    pNonEmptyStreamInfo = NULL;
    
    ulRemainToBufferInMs    = 0;
    ulRemainToBuffer        = 0;
    ulExcessBufferInMs      = 0;        
    ulExcessBuffer          = 0;
    bValidInfo              = FALSE;
    ulActualExcessBufferInMs= 0;        
    ulActualExcessBuffer    = 0;

    CHXMapLongToObj::Iterator i;

    /*
     * - Update transport stats for each stream.
     * - Find the lowest and highest timestamps across all streams
     * - Keep track of the stream with the lowest timestamp and is 
     *   buffering data
     */
    IHXSourceBufferingStats3* pSrcBufStats = NULL;
    
    m_pParent->QueryInterface(IID_IHXSourceBufferingStats3,
                              (void**)&pSrcBufStats);

    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);

	UINT32 ulLowestTimestampAtTransport = 0;
	UINT32 ulHighestTimestampAtTransport = 0;
        ULONG32 ulNumBytesAtTransport = 0;
        HXBOOL bDoneAtTransport = FALSE;

        if (pSrcBufStats)
        {
            pSrcBufStats->GetCurrentBuffering(pStreamInfo->m_uStreamNumber, 
                                              ulLowestTimestampAtTransport,
                                              ulHighestTimestampAtTransport,
                                              ulNumBytesAtTransport,
                                              bDoneAtTransport);
        }
            
        pStreamInfo->BufferingState().UpdateTransportStats(
            ulLowestTimestampAtTransport,
            ulHighestTimestampAtTransport,
            ulNumBytesAtTransport,
            bDoneAtTransport);

        if (ulNumBytesAtTransport > 0)
        {           
            if (bIsFirst)
            {
                bIsFirst            = FALSE;
                ulTheLowestTS       = ulLowestTimestampAtTransport;
                ulTheHighestTS      = ulHighestTimestampAtTransport;
                pNonEmptyStreamInfo = pStreamInfo;
            }
            else 
            {
                if (((LONG32) (ulTheLowestTS - ulLowestTimestampAtTransport)) > 0)
                {
                    ulTheLowestTS = ulLowestTimestampAtTransport;

                    /* This is the stream with the lowest timestamp */
                    pNonEmptyStreamInfo = pStreamInfo;
                }

                if (((LONG32) (ulTheHighestTS - ulHighestTimestampAtTransport)) < 0)
                {
                    ulTheHighestTS = ulHighestTimestampAtTransport;
                }
            }

        }
    }
    HX_RELEASE(pSrcBufStats);

    /* If none of the streams have any data buffered at the transport layer,
     * return.
     */
    if (!pNonEmptyStreamInfo)
    {
        return HXR_OK;
    }

    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        pStreamInfo = (STREAM_INFO*) (*i);

        UINT32 ulExcessForThisStreamInMs = 0;
        UINT32 ulExcessForThisStream     = 0;

        pStreamInfo->BufferingState().GetExcessBufferInfo(ulTheLowestTS,
                                                          ulTheHighestTS,
                                                          ulRemainToBufferInMs,
                                                          ulRemainToBuffer,
                                                          ulExcessBufferInMs,
                                                          ulExcessBuffer,
                                                          ulExcessForThisStreamInMs,
                                                          ulExcessForThisStream);

        /* Update Actual Values Regardless of Preroll / PreData */
        ulActualExcessBuffer += ulExcessForThisStream;
        if (ulActualExcessBufferInMs < ulExcessForThisStreamInMs)
        {
            ulActualExcessBufferInMs = ulExcessForThisStreamInMs;
        }
    }

    bValidInfo = TRUE;

    if (ulRemainToBufferInMs >= ulExcessBufferInMs)
    {
        ulRemainToBufferInMs -= ulExcessBufferInMs;
        ulExcessBufferInMs    = 0;
    }
    else 
    {
        ulExcessBufferInMs      -= ulRemainToBufferInMs;
        ulRemainToBufferInMs     = 0;
    }

    if (ulRemainToBuffer > 0)
    {
        ulExcessBuffer    = 0;
    }

    return HXR_OK;
}

void            
CBufferManager::EnterBufferedPlay(void)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::EnterBufferedPlay()", this);

#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    if (!m_bBufferedPlay && !m_bPerfectPlay)
    {
        m_bBufferedPlay             = TRUE;

        UINT32  ulRemainToBufferInMs    = 0;
        UINT32  ulRemainToBuffer        = 0;
        
        GetRemainToBuffer(ulRemainToBufferInMs, ulRemainToBuffer);
        
        /* If we are not done with buffering yet, update the buffering 
         * calc now!
         */ 
        if (ulRemainToBufferInMs > 0 ||
            ulRemainToBuffer > 0)
        {
            UpdateMinimumPreroll(FALSE);
        }
        else
        {
            m_bBufferCalcToBeChanged    = TRUE;
        }
    }
#endif // (HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
}

void            
CBufferManager::LeaveBufferedPlay(void)
{
    HXLOGL4(HXLOG_CORE, "CBufferManager[%p]::LeaveBufferedPlay()", this);
#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    if (m_bBufferedPlay)
    {
        m_bBufferCalcToBeChanged    = TRUE;
        m_bBufferedPlay             = FALSE;
    }
#endif // (HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
}

void        
CBufferManager::OnResumed(void)
{
    IHXSourceBufferingStats3* pSrcBufStats = NULL;
    CHXMapLongToObj::Iterator i;
    
    m_pParent->QueryInterface(IID_IHXSourceBufferingStats3,
                              (void**)&pSrcBufStats);

    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*i);

        UINT32 ulLowestTimestampAtTransport = 0;
        UINT32 ulHighestTimestampAtTransport = 0;
        ULONG32 ulNumBytesAtTransport = 0;
        HXBOOL bDoneAtTransport = FALSE;

        if (pSrcBufStats)
        {
            pSrcBufStats->GetCurrentBuffering(pStreamInfo->m_uStreamNumber, 
                                              ulLowestTimestampAtTransport,
                                              ulHighestTimestampAtTransport,
                                              ulNumBytesAtTransport,
                                              bDoneAtTransport);
        }

        pStreamInfo->BufferingState().UpdateTransportStats(
            ulLowestTimestampAtTransport,
            ulHighestTimestampAtTransport,
            ulNumBytesAtTransport,
            bDoneAtTransport);

        pStreamInfo->BufferingState().OnResumed();
    }

    HX_RELEASE(pSrcBufStats);

    return;
}

UINT32 CBufferManager::GetElapsedTime(UINT32 ulCurrentTick)
{
    UINT32 ulRet = 0;

    // If the start time has not been set, then
    // we cannot compute against it
    if (!m_bBufferStartTimeToBeSet)
    {
        // Compute the difference in time
        ulRet = CALCULATE_ELAPSED_TICKS(m_ulBufferingStartTick, ulCurrentTick);
        // Subtract out the paused time (if any)
        if (m_ulTotalPauseTime)
        {
            // Subtract off the amount of time we have been paused
            INT32 lRet = ulRet - m_ulTotalPauseTime;
            ulRet = (UINT32) (lRet > 0 ? lRet : 0);
        }
    }

    return ulRet;
}

void CBufferManager::UpdateHighestTimestamps(INT64 llActualTimeStamp,
                                             STREAM_INFO* pStreamInfo)
{
    // we apply the highest packet time-stamp to every stream
    // to solve infinite buffering
    if (m_llHighestTimeStamp < llActualTimeStamp)
    {
        m_llHighestTimeStamp = llActualTimeStamp;
    }
}

void CBufferManager::UpdateLowestTimestamps(INT64 llActualTimeStamp,
                                            STREAM_INFO* pStreamInfo)
{
    if (llActualTimeStamp < m_llLowestTimeStamp)
    {
        m_llLowestTimeStamp = llActualTimeStamp;
    }
}

void CBufferManager::SetPerfectPlay(HXBOOL bPerfectPlay)
{
    m_bPerfectPlay = bPerfectPlay;

    if (m_pParent->IsLive())
    {
        m_bPerfectPlay  = FALSE;
        m_bBufferedPlay = FALSE;
    }
}

void CBufferManager::ReadPrefs()
{
#if defined(HELIX_FEATURE_PREFERENCES)
    if (m_pParent && m_pParent->m_pPlayer)
    {
        // QI for IHXPreferences
        IHXPreferences* pPrefs = NULL;
        m_pParent->m_pPlayer->QueryInterface(IID_IHXPreferences, (void**) &pPrefs);
        if (pPrefs)
        {
            ReadPrefUINT32(pPrefs, "WallclockProofingDelay", m_ulWallclockProofingDelay);
        }
        HX_RELEASE(pPrefs);
    }
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */
}

INT32 CBufferManager::ComputeWallclockDelay(UINT32 ulFirstPacketTimeStamp, UINT32 ulFirstPacketTick)
{
    INT32 lRet = 0;

    if (m_pParent && !m_bBufferStartTimeToBeSet)
    {
        // First determine the presentation "start time"
        UINT32 ulPresentationStartTime = 0;
        // Is the presentation live or are we rebuffering?
        if (m_pParent->IsLive() || m_state == BUFFMGR_REBUFFER)
        {
            // For live, use a presentation start time of
            // the first packet received over all streams
            ulPresentationStartTime = m_ulBufferingStartTimeTS;
        }
        else
        {
            // For on-demand, we use the seek time if
            // a seek has been performed, and if not,
            // we use the start time.
            ulPresentationStartTime = (m_bIsSeekPerformed ? m_ulSeekTime : m_pParent->GetStartTime());
        }
        // Now compute the wallclock delay
        lRet = ulPresentationStartTime - ulFirstPacketTimeStamp +
               ulFirstPacketTick       - m_ulBufferingStartTick;
    }

    return lRet;
}

void CBufferManager::SetVelocity(INT32 lVel)
{
    // Set the buffer manager velocity
    m_lPlaybackVelocity = lVel;
    // Update the velocity for all HXBufState objects
    CHXMapLongToObj::Iterator i;
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*i);
        if (pStreamInfo)
        {
            pStreamInfo->BufferingState().SetVelocity(lVel);
        }
    }
}

void CBufferManager::SetKeyFrameMode(HXBOOL bMode)
{
    // Set the buffer manager keyframe mode
    m_bKeyFrameMode = bMode;
    // Update the velocity for all HXBufState objects
    CHXMapLongToObj::Iterator i;
    for (i = m_pStreamInfoTable->Begin(); i != m_pStreamInfoTable->End(); ++i)
    {
        STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*i);
        if (pStreamInfo)
        {
            pStreamInfo->BufferingState().SetKeyFrameMode(bMode);
        }
    }
}

void  CBufferManager::SetMaxAdditionalBuffering(UINT32 ulMaxInMs)
{
    m_ulMaxAdditionalBufferingInMs = ulMaxInMs;
}

void CBufferManager::UpdateElapsedTimeAllStreams()
{
    if (m_pStreamInfoTable)
    {
        // Get the current time
        UINT32 ulCurrentTick = HX_GET_BETTERTICKCOUNT();
        // Get the elapsed time
        UINT32 ulElapsedTime = GetElapsedTime(ulCurrentTick);
        // Run through all the streams, updating the elapsed time
        CHXMapLongToObj::Iterator itr;
        for (itr = m_pStreamInfoTable->Begin(); itr != m_pStreamInfoTable->End(); ++itr)
        {
            STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*itr);
            if (pStreamInfo)
            {
                UINT32 ulTmpElapsedTime = ulElapsedTime;
                if (pStreamInfo->BufferingState().AwaitingFirstPacket() &&
                    ulElapsedTime < m_ulWallclockProofingDelay)
                {
                    ulTmpElapsedTime = 0;
                }
                pStreamInfo->BufferingState().UpdateElapsedTime(ulTmpElapsedTime);
            }
        }
    }
}

