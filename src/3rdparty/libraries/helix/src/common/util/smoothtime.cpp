/* ***** BEGIN LICENSE BLOCK *****
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

#include "hxtypes.h"
#include "hxtick.h"
#include "smoothtime.h"

//#define ENABLE_SYNC_TRACE
//#define SYNC_SMOOTHING_OLD_SCHEME
#ifdef HELIX_FEATURE_VIDREND_SYNCSMOOTHING
#define SYNC_PRE_SMOOTHING
#define SYNC_SMOOTHING
#endif /* #ifdef HELIX_FEATURE_VIDREND_SYNCSMOOTHING */
#define MAX_INT32_AS_DOUBLE            ((double) ((INT32)  0x7FFFFFFF))
#define MIN_INT32_AS_DOUBLE            ((double) ((INT32)  0x80000000))
#define MAX_BAD_SAMPLE_INTERVAL      1000  // in milliseconds
#define AUDIO_SKEW_POWER                4  // Maximum Audio Speedup Slope as
                                           // 1/(2^AUDIO_SKEW_POWER)
#define MIN_BAD_PERSISTENCE_COUNT       0  // Minimum number of consecutive
                                           // bad sync samples for the
                                           // resync probation period to start
#define MIN_GOOD_PERSISTENCE_COUNT      0  // Minimum number of consecutive
                                           // good sync samples for the
                                           // resync probation period to be
                                           // cancelled
#define MAX_SYNC_TRACE_ENTRIES	    10000

CHXTimeSyncSmoother::CHXTimeSyncSmoother()
{
    m_ulSyncSmoothingDepth        = 0;
    m_ulSyncGoalSmoothingDepth    = SYNC_GOAL_SMOOTHING_DEPTH;
    m_ulSpeedupGoalSmoothingDepth = SPEEDUP_GOAL_SMOOTHING_DEPTH;
    m_ulBadSeqSampleCount         = 0;
    m_ulGoodSeqSampleCount        = 0;
    m_ulMaxBadSeqSamples          = MIN_BAD_PERSISTENCE_COUNT;
    m_lTimelineOffset             = 0;
    m_ulStreamBaseTime            = 0;
    m_ulLastTickCount             = 0;
    m_ulBaseTime                  = 0;
    m_ulTimeNormalizationOffset   = 0;
    m_lPlaybackVelocity           = 100;
    m_bBaseTimeSet                = FALSE;
    m_bLastSampleGood             = TRUE;
    m_dTrendSyncDelta             = 0.0;
#ifdef ENABLE_SYNC_TRACE
    m_ulSyncTraceIdx              = 0;
    for (UINT32 i = 0; i < MAX_SYNC_TRACE_ENTRIES; i++)
    {
        for (UINT32 j = 0; j < 3; j++)
        {
            m_ulSyncTraceArray[i][j] = 0;
        }
    }
#endif /* #ifdef ENABLE_SYNC_TRACE */
}

CHXTimeSyncSmoother::~CHXTimeSyncSmoother()
{
}

void CHXTimeSyncSmoother::ResetSampleCounts()
{
    m_ulSyncSmoothingDepth = 0;
    m_ulBadSeqSampleCount  = 0;
    m_ulGoodSeqSampleCount = 0;
}

void CHXTimeSyncSmoother::ResetTimes()
{
    m_ulStreamBaseTime          = 0;
    m_ulBaseTime                = 0;
    m_ulTimeNormalizationOffset = 0;
}

void CHXTimeSyncSmoother::ResetOffset()
{
    m_ulTimeNormalizationOffset = m_ulStreamBaseTime - m_ulBaseTime - m_lTimelineOffset;
}

void CHXTimeSyncSmoother::OnTimeSync(UINT32 ulTime)
{
    OnTimeSyncTick(ulTime, HX_GET_BETTERTICKCOUNT());
}

void CHXTimeSyncSmoother::OnTimeSyncTick(UINT32 ulTime, UINT32 ulTick)
{
    UINT32 ulStreamBaseTime = ulTick;
    UINT32 ulBaseTime       = ulTime;
    HXBOOL   bGoodSample      = TRUE;

    // Save the last tick count
    m_ulLastTickCount = ulStreamBaseTime;

    // Scale the tick count (if necessary)
    ulStreamBaseTime = ScaleTickCount(ulStreamBaseTime);

#ifdef ENABLE_SYNC_TRACE
    UINT32 ulOrigStreamBaseTime = ulStreamBaseTime;
#endif /* #ifdef ENABLE_SYNC_TRACE */

#ifdef SYNC_SMOOTHING_OLD_SCHEME
    INT32 lNewSyncDelta = (INT32) (ulStreamBaseTime - m_lTrendSyncDelta - ulTime);
    m_lTrendSyncDelta  += lNewSyncDelta / ((INT32) (m_ulSyncSmoothingDepth + 1));
    ulStreamBaseTime    = ulTime + m_lTrendSyncDelta;
    if (m_ulSyncSmoothingDepth < m_ulSyncGoalSmoothingDepth)
    {
        m_ulSyncSmoothingDepth++;
    }
#endif /* #ifdef SYNC_SMOOTHING_OLD_SCHEME */

#ifdef SYNC_PRE_SMOOTHING
    if (m_ulSyncSmoothingDepth > 0)
    {
        // This is difference in tick counts. They always move forward.
        INT32  lStreamBaseDelta = (INT32) (m_lPlaybackVelocity < 0 ?
                                           (m_ulStreamBaseTime - ulStreamBaseTime) :
                                           (ulStreamBaseTime - m_ulStreamBaseTime));
        // This is difference in time syncs. They can move forwards or
        // backward, depending on playback velocity. However, we will
        // always keep the deltas positive.
        UINT32 ulBaseDelta      = (m_lPlaybackVelocity < 0 ?
                                   (m_ulBaseTime - ulBaseTime) :
                                   (ulBaseTime - m_ulBaseTime));
        HXBOOL bSkipSmoothing     = FALSE;

        bGoodSample = ((lStreamBaseDelta <= 0) ||
                       (((UINT32) lStreamBaseDelta) <=
                        (ulBaseDelta + (ulBaseDelta >> AUDIO_SKEW_POWER))) ||
                       (bSkipSmoothing = ((m_ulBadSeqSampleCount++) > m_ulMaxBadSeqSamples)));

        if (bSkipSmoothing)
        {
            m_ulSyncSmoothingDepth = 0;
            m_dTrendSyncDelta      = 0.0;
        }
    }
#endif /* #ifdef SYNC_PRE_SMOOTHING */

    if (bGoodSample)
    {
#ifdef SYNC_SMOOTHING
        double dNewSyncDelta = (((double) ((UINT32) (ulStreamBaseTime - ulTime))) -
                               m_dTrendSyncDelta);

        // If we have a m_dTrendSyncDelta, make sure we consider
        // m_dTrendSyncDelta wrap-around in relation to dNewSyncDelta
        if (m_ulSyncSmoothingDepth > 0)
        {
            if (dNewSyncDelta > MAX_INT32_AS_DOUBLE)
            {
                dNewSyncDelta = MAX_UINT32_AS_DOUBLE - dNewSyncDelta;
            }
            else if (dNewSyncDelta < MIN_INT32_AS_DOUBLE)
            {
                dNewSyncDelta += MAX_UINT32_AS_DOUBLE;
            }
        }

        if (dNewSyncDelta < 0.0)
        {
            // We are trying to speed up: use speed up smoothing criteria
            m_dTrendSyncDelta += (dNewSyncDelta /
                                  ((m_ulSyncSmoothingDepth >= m_ulSpeedupGoalSmoothingDepth) ?
                                   m_ulSpeedupGoalSmoothingDepth + 1 : m_ulSyncSmoothingDepth + 1));
        }
        else
        {
            m_dTrendSyncDelta += dNewSyncDelta / (m_ulSyncSmoothingDepth + 1);
        }

        if (m_dTrendSyncDelta > MAX_UINT32_AS_DOUBLE)
        {
            m_dTrendSyncDelta -= MAX_UINT32_AS_DOUBLE;
        }
        else if (m_dTrendSyncDelta < 0)
        {
             m_dTrendSyncDelta += MAX_UINT32_AS_DOUBLE;
        }

        ulStreamBaseTime = ulTime + ((UINT32) (m_dTrendSyncDelta));

        if (m_ulSyncSmoothingDepth < m_ulSyncGoalSmoothingDepth)
        {
            m_ulSyncSmoothingDepth++;
        }
#endif /* #ifdef SYNC_SMOOTHING */

        m_ulGoodSeqSampleCount++;

        m_ulStreamBaseTime          = ulStreamBaseTime;
        m_ulBaseTime                = ulBaseTime;
        m_ulTimeNormalizationOffset = m_ulStreamBaseTime - m_ulBaseTime - m_lTimelineOffset;

        if (m_ulGoodSeqSampleCount >= MIN_GOOD_PERSISTENCE_COUNT)
        {
            m_ulBadSeqSampleCount = 0;
        }
    }
    else
    {
        // This is a bad sample
        if (m_ulBadSeqSampleCount >= MIN_BAD_PERSISTENCE_COUNT)
        {
            m_ulGoodSeqSampleCount = 0;
        }
    }

    // Save the last sample flag
    m_bLastSampleGood = bGoodSample;

#ifdef ENABLE_SYNC_TRACE
    if (m_ulSyncTraceIdx < MAX_SYNC_TRACE_ENTRIES)
    {
        m_ulSyncTraceArray[m_ulSyncTraceIdx][0] = m_ulBaseTime;
        m_ulSyncTraceArray[m_ulSyncTraceIdx][1] = m_ulStreamBaseTime;
        m_ulSyncTraceArray[m_ulSyncTraceIdx][2] = ulOrigStreamBaseTime;
        m_ulSyncTraceIdx++;
    }
#endif /* #ifdef ENABLE_SYNC_TRACE */
}

void CHXTimeSyncSmoother::SetVelocity(INT32 lVelocity)
{
    if (lVelocity != m_lPlaybackVelocity)
    {
        // Get the current time
        UINT32 ulTimeNow = GetTimeNow();
        // Clear out the current state
        ResetSampleCounts();
        ResetTimes();
        // Save the playback velocity
        m_lPlaybackVelocity = lVelocity;
        // Since we have reset all the sample counts, then
        // if we were asked for the current time now, we
        // could not provide one. Therefore, we need to
        // supply a fake timesync.
        OnTimeSync(ulTimeNow);
    }
}

#ifdef ENABLE_SYNC_TRACE

void CHXTimeSyncSmoother::DumpSyncEntries()
{
    FILE*   pFile = NULL;
    ULONG32 ulIdx = 0;

    if (m_ulSyncTraceIdx > 0)
    {
	pFile = fopen("\\helix\\sync.txt", "wb");
    }

    if (pFile)
    {
	for (ulIdx = 0; ulIdx < m_ulSyncTraceIdx; ulIdx++)
	{
	    fprintf(pFile, "%u\t%u\t%u\n", m_ulSyncTraceArray[ulIdx][0],
					   m_ulSyncTraceArray[ulIdx][1],
					   m_ulSyncTraceArray[ulIdx][2]);
	}
	fclose(pFile);
    }

    m_ulSyncTraceIdx = 0;
}

#endif /* #ifdef ENABLE_SYNC_TRACE */

