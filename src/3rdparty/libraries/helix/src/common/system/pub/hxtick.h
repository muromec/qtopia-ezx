/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxtick.h,v 1.9 2004/07/15 23:21:34 ping Exp $
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

#ifndef _HXTICK_H_
#define _HXTICK_H_

#include "hlxclib/windows.h"
#ifdef _WIN16
#include <mmsystem.h>
#endif

#include "hxtypes.h"

/* We have added a HX_GET_BETTERTICKCOUNT which uses highly accurate timers
 * when the hardware and OS will allow and defaults back to the timers used
 * by HX_GET_TICKCOUNT when they are not available. Code which requires
 * accurate timing (i.e. better than 10-20ms) should probably use the
 * HX_GET_BETTERTICKCOUNT call. The wraparound and precision of both calls
 * should be the same - HX_GET_BETTERTICKCOUNT should always be more ACCURATE.
 */

#if defined(_MACINTOSH) || defined(_UNIX)
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
ULONG32 GetTickCount(void);
UINT32 GetTickCountInUSec(void);
#define HX_GET_TICKCOUNT() GetTickCount()
#define HX_GET_BETTERTICKCOUNT() GetTickCount()
#define HX_GET_TICKCOUNT_IN_USEC() GetTickCountInUSec()
#ifdef __cplusplus
}
#endif /* __cplusplus */

#elif defined(_SYMBIAN)
# ifdef __cplusplus
extern "C"
{
# endif /* __cplusplus */
ULONG32 GetTickCount(void);
UINT32 GetTickCountInUSec(void);
# define HX_GET_TICKCOUNT() GetTickCount()
# define HX_GET_BETTERTICKCOUNT() GetTickCount()
# define HX_GET_TICKCOUNT_IN_USEC() GetTickCountInUSec()
# ifdef __cplusplus
}
# endif /* __cplusplus */ /* SYMBIAN */

#elif defined(_OPENWAVE)
# ifdef __cplusplus
extern "C"
{
# endif /* __cplusplus */

ULONG32 GetTickCount(void);
UINT32 GetTickCountInUSec(void);
#define HX_GET_TICKCOUNT() GetTickCount()
#define HX_GET_BETTERTICKCOUNT() GetTickCount()
#define HX_GET_TICKCOUNT_IN_USEC() GetTickCountInUSec()

# ifdef __cplusplus
}
# endif /* __cplusplus */ /* SYMBIAN */

#elif defined(_WIN32)
#define HX_GET_TICKCOUNT() GetTickCount()
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
ULONG32 GetBetterTickCount();

// Returns a ms system tick with double precision accuracy.
// Wraps at 2^32 -1 milliseconds.
double GetMSTickDouble32();
UINT32 GetTickCountInUSec(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*
 * In 32bit Windows we have a HX_GET_BETTERTICKCOUNT()
 * which uses the queryPerformance interface for highly accurate times.
 * XXXKB: This implementation is now thread safe;
 */
#define HX_GET_BETTERTICKCOUNT() GetBetterTickCount()
#define HX_GET_TICKCOUNT_IN_USEC() GetTickCountInUSec()

/* WINDOWS DEFAULT */
#elif defined(_WINDOWS)
#define HX_GET_TICKCOUNT() GetTickCount()
#define HX_GET_BETTERTICKCOUNT() GetTickCount()
#define HX_GET_TICKCOUNT_IN_USEC() GetTickCountInUSec()
#else
#error hxtick.h::Undefined platform!
#endif // #ifdef __MWERKS__

#define MILLISECS_PER_SECOND	((ULONG32)1000)
#define CALCULATE_ELAPSED_TICKS(t1,t2)	((t2) - (t1))

// use this to measure a result from calculate_elapsed to ensure that
// time didn't go backwards
#define	SOME_VERY_LARGE_VALUE	 86400000 /* 1 day */ 

// Timestamp comparisons
#define IsTimeGreater(a, b)					\
    (((LONG32) ((a) - (b))) > 0)

#define IsTimeGreaterOrEqual(a, b)				\
    (((LONG32) ((a) - (b))) >= 0)

#define IsTimeLess(a, b)					\
    (((LONG32) ((a) - (b))) < 0)

#define IsTimeLessOrEqual(a, b)					\
    (((LONG32) ((a) - (b))) <= 0)
#endif //_HXTICK_H_
