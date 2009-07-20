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

#ifndef HXPLAYVELOCITY_H
#define HXPLAYVELOCITY_H

// Defines
#define HX_PLAYBACK_VELOCITY_NORMAL      100
#define HX_PLAYBACK_VELOCITY_MIN      -10000
#define HX_PLAYBACK_VELOCITY_MAX       10000
#define HX_PLAYBACK_VELOCITY_REVERSE_1X -100

/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXPlaybackVelocityCaps
 *
 *  Purpose:
 *
 *      Simple interface for expressing velocity ranges
 *
 *  IID_IHXPlaybackVelocityCaps:
 *
 *      {DADB9ABF-549E-4a6a-A36A-597EE270C6A0}
 *
 */
DEFINE_GUID(IID_IHXPlaybackVelocityCaps, 0xdadb9abf, 0x549e, 0x4a6a, 0xa3, 0x6a, 0x59,
                                         0x7e, 0xe2, 0x70, 0xc6, 0xa0);

#undef  INTERFACE
#define INTERFACE IHXPlaybackVelocityCaps

DECLARE_INTERFACE_(IHXPlaybackVelocityCaps, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXPlaybackVelocityCaps methods
    STDMETHOD_(UINT32,GetNumRanges)  (THIS) PURE;
    STDMETHOD(GetRange)              (THIS_ UINT32 i, REF(INT32) rlMin, REF(INT32) rlMax) PURE;
    STDMETHOD(AddRange)              (THIS_ INT32 lMin, INT32 lMax) PURE;
    STDMETHOD(CombineCapsLogicalAnd) (THIS_ IHXPlaybackVelocityCaps* pCaps) PURE;
    STDMETHOD_(HXBOOL,IsCapable)       (THIS_ INT32 lVelocity) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXPlaybackVelocityResponse
 *
 *  Purpose:
 *
 *      Enable asynchronous reporting of changing velocity conditions
 *
 *  IID_IHXPlaybackVelocityResponse:
 *
 *      {A4F87CBB-8EC5-4d87-908D-1A98DBBDE4DF}
 *
 */
DEFINE_GUID(IID_IHXPlaybackVelocityResponse, 0xa4f87cbb, 0x8ec5, 0x4d87, 0x90, 0x8d, 0x1a,
                                             0x98, 0xdb, 0xbd, 0xe4, 0xdf);

#undef  INTERFACE
#define INTERFACE IHXPlaybackVelocity

DECLARE_INTERFACE_(IHXPlaybackVelocityResponse, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXPlaybackVelocityResponse methods
    STDMETHOD(UpdateVelocityCaps) (THIS_ IHXPlaybackVelocityCaps* pCaps) PURE;
    STDMETHOD(UpdateVelocity)     (THIS_ INT32 lVelocity) PURE;
    STDMETHOD(UpdateKeyFrameMode) (THIS_ HXBOOL bKeyFrameMode) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXPlaybackVelocity
 *
 *  Purpose:
 *
 *      Control forward and backward non-1x speed playback.
 *      In the SetVelocity call, if bKeyFrameMode is TRUE, then
 *      bAutoSwitch is ignored. If bKeyFrameMode is FALSE, then
 *      the engine looks at the value of bAutoSwitch. If bAutoSwitch
 *      is TRUE, then the engine will attempt to switch from all-frames
 *      mode to keyframe mode automatically. If bAutoSwitch is FALSE,
 *      then the engine will attempt to play all frames and will
 *      not attempt to determine when to switch to keyframe mode.
 *
 *  IID_IHXPlaybackVelocity:
 *
 *      {42BD6E35-20AC-4f7e-9D9B-B0BB3ECB1BE9}
 *
 */
DEFINE_GUID(IID_IHXPlaybackVelocity, 0x42bd6e35, 0x20ac, 0x4f7e, 0x9d, 0x9b, 0xb0,
                                     0xbb, 0x3e, 0xcb, 0x1b, 0xe9);

#undef  INTERFACE
#define INTERFACE IHXPlaybackVelocity

DECLARE_INTERFACE_(IHXPlaybackVelocity, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse) PURE;
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps) PURE;
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch) PURE;
    STDMETHOD_(INT32,GetVelocity)          (THIS) PURE;
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode) PURE;
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS) PURE;
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS) PURE;
    STDMETHOD(CloseVelocityControl)        (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXPlaybackVelocityTimeRegulator
 *
 *  Purpose:
 *
 *      Interface for communicating stream adjustments to the timeline
 *
 *  IID_IHXPlaybackVelocityTimeRegulator:
 *
 *      {A0A5D241-C387-4f85-9BE6-9BA4A0F18C94}
 *
 */
DEFINE_GUID(IID_IHXPlaybackVelocityTimeRegulator, 0xa0a5d241, 0xc387, 0x4f85, 0x9b, 0xe6, 0x9b,
                                                  0xa4, 0xa0, 0xf1, 0x8c, 0x94);

#undef  INTERFACE
#define INTERFACE IHXPlaybackVelocityTimeRegulator

DECLARE_INTERFACE_(IHXPlaybackVelocityTimeRegulator, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXPlaybackVelocityTimeRegulator methods
    STDMETHOD(SetCurrentTimeWarp)      (THIS_ UINT32 ulOrigTime, UINT32 ulWarpTime) PURE;
    STDMETHOD(GetLastTimeWarp)         (THIS_ REF(UINT32) rulOrigTime, REF(UINT32) rulWarpTime) PURE;
    STDMETHOD_(UINT32,GetOriginalTime) (THIS_ UINT32 ulWarpTime) PURE;
    STDMETHOD_(UINT32,GetWarpedTime)   (THIS_ UINT32 ulOrigTime) PURE;
};

// This method determines "later" in terms of
// playback direction. For instance, for forward
// playback, later means a higher timestamp. But
// for reverse playback, later means a lower timestamp.
#define HX_LATER_USING_VELOCITY(t1, t2, vel) \
    ((vel) < 0 ? (((LONG32) (t1 - t2)) < 0) : (((LONG32) (t1 - t2)) > 0))

#define HX_LATER_OR_SAME_USING_VELOCITY(t1, t2, vel) \
    ((vel) < 0 ? (((LONG32) (t1 - t2)) <= 0) : (((LONG32) (t1 - t2)) >= 0))

#define HX_EARLIER_USING_VELOCITY(t1, t2, vel) \
    ((vel) < 0 ? (((LONG32) (t1 - t2)) > 0) : (((LONG32) (t1 - t2)) < 0))

#define HX_EARLIER_OR_SAME_USING_VELOCITY(t1, t2, vel) \
    ((vel) < 0 ? (((LONG32) (t1 - t2)) >= 0) : (((LONG32) (t1 - t2)) <= 0))

#define HX_ADD_USING_VELOCITY(t1, off, vel) \
    (((vel) < 0) ? ((((LONG32) (t1 - off)) > 0)? ((t1) - (off)) : 0) : ((t1) + (off)))

#define HX_SCALE_BY_VELOCITY(val, vel) \
    ((val) * (vel) / HX_PLAYBACK_VELOCITY_NORMAL)

#define HX_ABSVAL(val) (((val) < 0) ? (-(val)) : (val))

#define HX_IS_ACCELERATED_PLAYBACK(vel) \
    (((vel) > HX_PLAYBACK_VELOCITY_NORMAL) || ((vel) < -HX_PLAYBACK_VELOCITY_NORMAL))

#define HX_IS_NON1X_PLAYBACK(vel) ((vel) != HX_PLAYBACK_VELOCITY_NORMAL)

#define HX_IS_1X_PLAYBACK(vel) ((vel) == HX_PLAYBACK_VELOCITY_NORMAL)

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXPlaybackVelocityCaps)
DEFINE_SMART_PTR(IHXPlaybackVelocityResponse)
DEFINE_SMART_PTR(IHXPlaybackVelocity)
DEFINE_SMART_PTR(IHXPlaybackVelocityTimeRegulator)

#endif /* #ifndef HXPLAYVELOCITY_H */
