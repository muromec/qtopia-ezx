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

#ifndef SMOOTHTIME_H
#define SMOOTHTIME_H

#include "hxtick.h"

// Defines
#define SYNC_GOAL_SMOOTHING_DEPTH      30  // number of samples
#define SPEEDUP_GOAL_SMOOTHING_DEPTH    8  // number of samples
#define MAX_UINT32_AS_DOUBLE           ((double) ((UINT32) 0xFFFFFFFF))


class CHXTimeSyncSmoother
{
public:
    CHXTimeSyncSmoother();
    virtual ~CHXTimeSyncSmoother();

    // CHXTimeSyncSmoother methods
    void   ResetSampleCounts();
    void   ResetTimes();
    void   ResetOffset();
    void   OnTimeSync(UINT32 ulTime);
    UINT32 GetTimeNow(UINT32* pulTick = NULL);
    void   SetTimelineOffset(INT32 lOffset);
    INT32  GetTimelineOffset();
    void   SetMaxBadSeqSamples(UINT32 ulNum);
    UINT32 GetMaxBadSeqSamples();
    void   SetSyncGoalSmoothingDepth(UINT32 ulNum);
    UINT32 GetSyncGoalSmoothingDepth();
    void   SetSpeedupGoalSmoothingDepth(UINT32 ulNum);
    UINT32 GetSpeedupGoalSmoothingDepth();
    HXBOOL   IsBaseTimeSet();
    void   SetBaseTimeFlag();
    void   ClearBaseTimeFlag();
    void   SetBaseTime(UINT32 ulTime);
    UINT32 GetBaseTime();
    UINT32 GetLastTickCount();
    HXBOOL   WasLastSampleGood();
    void   UpdateTimelineOffset(INT32 lOffset);
    void   SetVelocity(INT32 lVelocity);
    INT32  GetVelocity();
    UINT32 ScaleTickCount(UINT32 ulTick);
    // These are for unittest purposes (where we
    // must provide the tick count rather than get
    // it on the fly)
    void   OnTimeSyncTick(UINT32 ulTime, UINT32 ulTick);
    UINT32 GetTimeNowTick(UINT32 ulTick);
protected:
    UINT32 m_ulSyncSmoothingDepth;
    UINT32 m_ulSyncGoalSmoothingDepth;
    UINT32 m_ulSpeedupGoalSmoothingDepth;
    UINT32 m_ulBadSeqSampleCount;
    UINT32 m_ulGoodSeqSampleCount;
    UINT32 m_ulMaxBadSeqSamples;
    INT32  m_lTimelineOffset;
    UINT32 m_ulStreamBaseTime;
    UINT32 m_ulLastTickCount;
    UINT32 m_ulBaseTime;
    UINT32 m_ulTimeNormalizationOffset;
    INT32  m_lPlaybackVelocity;
    HXBOOL   m_bBaseTimeSet;
    HXBOOL   m_bLastSampleGood;
    double m_dTrendSyncDelta;
#ifdef ENABLE_SYNC_TRACE
    UINT32 m_ulSyncTraceIdx;
    UINT32 m_ulSyncTraceArray[MAX_SYNC_TRACE_ENTRIES][3];
    void   DumpSyncEntries();
#endif /* #ifdef ENABLE_SYNC_TRACE */

};

inline UINT32 CHXTimeSyncSmoother::GetTimeNow(UINT32* pulTick)
{
    UINT32 ulTickNow = HX_GET_BETTERTICKCOUNT();
    if (pulTick)
    {
        *pulTick = ulTickNow;
    }
    return GetTimeNowTick(ulTickNow);
}

inline UINT32 CHXTimeSyncSmoother::GetTimeNowTick(UINT32 ulTick)
{
    return ScaleTickCount(ulTick) - m_ulTimeNormalizationOffset;
}

inline void CHXTimeSyncSmoother::SetTimelineOffset(INT32 lOffset)
{
    m_lTimelineOffset = lOffset;
}

inline INT32 CHXTimeSyncSmoother::GetTimelineOffset()
{
    return m_lTimelineOffset;
}

inline void CHXTimeSyncSmoother::SetMaxBadSeqSamples(UINT32 ulNum)
{
    m_ulMaxBadSeqSamples = ulNum;
}

inline UINT32 CHXTimeSyncSmoother::GetMaxBadSeqSamples()
{
    return m_ulMaxBadSeqSamples;
}

inline void CHXTimeSyncSmoother::SetSyncGoalSmoothingDepth(UINT32 ulNum)
{
    m_ulSyncGoalSmoothingDepth = ulNum;
}

inline UINT32 CHXTimeSyncSmoother::GetSyncGoalSmoothingDepth()
{
    return m_ulSyncGoalSmoothingDepth;
}

inline void CHXTimeSyncSmoother::SetSpeedupGoalSmoothingDepth(UINT32 ulNum)
{
    m_ulSpeedupGoalSmoothingDepth = ulNum;
}

inline UINT32 CHXTimeSyncSmoother::GetSpeedupGoalSmoothingDepth()
{
    return m_ulSpeedupGoalSmoothingDepth;
}

inline HXBOOL CHXTimeSyncSmoother::IsBaseTimeSet()
{
    return m_bBaseTimeSet;
}

inline void CHXTimeSyncSmoother::SetBaseTimeFlag()
{
    m_bBaseTimeSet = TRUE;
}

inline void CHXTimeSyncSmoother::ClearBaseTimeFlag()
{
    m_bBaseTimeSet = FALSE;
}

inline void CHXTimeSyncSmoother::SetBaseTime(UINT32 ulTime)
{
    m_ulBaseTime = ulTime;
}

inline UINT32 CHXTimeSyncSmoother::GetBaseTime()
{
    return m_ulBaseTime;
}

inline UINT32 CHXTimeSyncSmoother::GetLastTickCount()
{
    return m_ulLastTickCount;
}

inline HXBOOL CHXTimeSyncSmoother::WasLastSampleGood()
{
    return m_bLastSampleGood;
}

inline void CHXTimeSyncSmoother::UpdateTimelineOffset(INT32 lOffset)
{
    m_lTimelineOffset -= lOffset;
}

inline INT32 CHXTimeSyncSmoother::GetVelocity()
{
    return m_lPlaybackVelocity;
}

inline UINT32 CHXTimeSyncSmoother::ScaleTickCount(UINT32 ulTick)
{
    UINT32 ulRet = ulTick;
    // If we are not playing back at normal velocity,
    // then scale the tick count
    if (m_lPlaybackVelocity != 100)
    {
        INT64 llScaledTick = ((INT64) ulTick) * ((INT64) m_lPlaybackVelocity) / ((INT64) 100);
        ulRet = INT64_TO_UINT32(llScaledTick);
    }

    return ulRet;
}

#endif /* #ifndef SMOOTHTIME_H */
