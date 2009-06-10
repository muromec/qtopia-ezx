/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: gettickcount.c,v 1.9 2007/07/06 20:41:57 jfinnecy Exp $
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

#ifndef _VXWORKS
#include <sys/time.h>
#ifdef HELIX_CONFIG_USE_CLOCK_GETTIME
#include <time.h>
#elif defined HELIX_CONFIG_USE_TIMES_SYSCALL
#include <sys/times.h>
#endif 
#endif
#include <unistd.h>
#include "hxtypes.h"
#include "hxheap.h"
#include "hxtick.h"

#include <stdio.h>

#ifdef _VXWORKS
void g_RegisterGlobalPtr(void *ptr);
#include <kernel/cClock.h>
#endif

#ifndef _VXWORKS

#ifdef HELIX_CONFIG_USE_TIMES_SYSCALL
inline ULONG32 GetClockTicksPerSec()
{
	static ULONG32 clockTicksPerSec = 0;
	if (clockTicksPerSec == 0)
		clockTicksPerSec = (ULONG32)sysconf(_SC_CLK_TCK);
	return clockTicksPerSec;
}
#endif
/*
*************************IMPORTANT******************************************
The current implementation of making use of external clock depends on setting
of these two variables g_ulHXExternalTimerTick, g_bExternalTimerUsed declared
in gettickcount.c. Since each shared library/dll gets its own version of these
vars, they need to be set in each of the shard libs/dll to turn on this code.

In the current incarnation, only the client core dll sets these. This is
because this support is currently used only by helixsim that makes use of
null renderer, so the only component that makes use of HX_GETTICKCOUNT
extensively is client core.

Please note that if this feature is used by some other app that uses actual
renderer and other plugins, calls to HX_GET_TICKCOUNT in those components
will still make use of gettimeofday() even if HELIX_FEATURE_USEEXTERNALCLOCK
is turned on.


The "proper fix" to propage use of externally set clock to all the plugins
is yet to be implemented.
*************************IMPORTANT*******************************************
*/
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
ULONG32 g_ulHXExternalTimerTick = 0;
HXBOOL g_bExternalTimerUsed = FALSE;
#endif //defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
ULONG32
GetTickCount()
{
#ifdef HELIX_CONFIG_USE_CLOCK_GETTIME
	struct timespec tp;
    static ULONG32 ulLastTick = 0;
    static ULONG32 ulBaseTick = 0;
    static ULONG32 ulRollTick = 0;
    ULONG32        ulNow      = 0;
    ULONG32        ulTmp      = 0;

    clock_gettime(CLOCK_MONOTONIC, &tp);
	ulNow = (ULONG32)(tp.tv_sec * 1000 + tp.tv_nsec / 1000000);
	ulTmp = ulNow-ulRollTick+ulBaseTick;
    if( ulTmp < ulLastTick )
    {
     //roll over or system clock was set back.
      ulRollTick = ulNow;
      ulBaseTick = ulLastTick;
      ulTmp = ulLastTick;
    }
    ulLastTick = ulTmp;
    return ulLastTick;

#elif defined HELIX_CONFIG_USE_TIMES_SYSCALL
	return (ULONG32)((((ULONG32)times(NULL)) * 1000)/ GetClockTicksPerSec());
#else
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
    if(g_bExternalTimerUsed)
    {
         return g_ulHXExternalTimerTick;
    }
#endif //defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
	struct timeval tv;
	gettimeofday( &tv, NULL );
    // this will rollover ~ every 49.7 days and 
	// is vulnerable to changes in system clock by user
    return (ULONG32)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif //HELIX_FEATURE_USE_CLOCK_GETTIME 
    
}

// return microseconds
UINT32
GetTickCountInUSec()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    // this will rollover ~ every 49.7 days
    return (ULONG32)(tv.tv_sec * 1000 * 1000 + tv.tv_usec);
}
#else

static Clock *_clock = NULL;

ULONG32 GetTickCount()
{
    TickSpec ticks;
    TimeSpec time;
    if (_clock == NULL)
    {
        _clock = ClockCreate(SYSTEM_CLOCK);
        if (_clock == NULL)
        {
            return 0;
        }
        g_RegisterGlobalPtr(&_clock);
    }
    ClockGetTime(_clock, &ticks);
    ClockConvertToTime(_clock, &ticks, &time);
    return (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
}

// return microseconds
UINT32
GetTickCountInUSec()
{
    TickSpec ticks;
    TimeSpec time;
    if (_clock == NULL)
    {
        _clock = ClockCreate(SYSTEM_CLOCK);
        if (_clock == NULL)
        {
            return 0;
        }
        g_RegisterGlobalPtr(&_clock);
    }
    ClockGetTime(_clock, &ticks);
    ClockConvertToTime(_clock, &ticks, &time);
    return (time.tv_sec * 1000 * 1000) + (time.tv_nsec / 1000);
}
#endif

