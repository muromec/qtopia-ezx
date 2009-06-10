/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: time.h,v 1.21 2008/01/18 09:17:27 vkathuria Exp $
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

#ifndef HLXSYS_TIME_H
#define HLXSYS_TIME_H

#if defined(_SYMBIAN)
# include <sys/time.h>
#endif 

#if defined(WIN32_PLATFORM_PSPC)
# include "hxtypes.h"
# include "hlxclib/windows.h"
#if _WIN32_WCE >= 420
#include <time.h>
#endif
#elif !defined(WIN32_PLATFORM_PSPC) && !defined(_OPENWAVE) && !defined(_BREW)
# include <time.h>
#endif /* !defined(WIN32_PLATFORM_PSPC) && !defined(_OPENWAVE) */

#if defined(_OPENWAVE)
# include "platform/openwave/hx_op_timeutil.h"
#endif

#if defined(_BREW)
#include "hxtypes.h"
#include "AEETime.h"
#include "hxassert.h"
#endif

#if !defined(_REENTRANT) || defined(_WIN32)
# include "hlxclib/string.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*******************************
 * Types
 */

#if defined(_OPENWAVE)

#define NO_TM_ISDST
typedef U32 time_t;
#define tm op_tm                // XXXSAB any other way for 'struct tm' to
                                // work in a C-includeable file?
struct timeval {
	time_t tv_sec;
	time_t tv_usec;
};

#elif defined(_BREW) || (defined(WIN32_PLATFORM_PSPC) && (_WIN32_WCE < 420))

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#endif /* defined(_BREW) || defined(WIN32_PLATFORM_PSPC) && (_WIN32_WCE < 420) */

#if defined(WIN32_PLATFORM_PSPC)
#define timezone _timezone
extern long _timezone;

#endif /* defined(WIN32_PLATFORM_PSPC) */


/*******************************
 * Helix declarations
 */
long __helix_time(long *t);
struct tm* __helix_localtime(long* timep);
void __helix_tzset();
long __helix_mktime(struct tm* tm);
struct tm *__helix_gmtime(long *timep);
int __helix_gettimeofday(struct timeval *tv, void *tz);
char * __helix_ctime(long *timer);

#if defined(_WINCE)
char * __helix_asctime (struct tm *tm);

/*******************************
 * platform specifics declarations
 */

_inline
void _tzset()
{
    __helix_tzset();
}

// 4.2 has a time.h file, so we import that earlier
// then we need to make our defs compatible with those
// using long/unsigned long instead of time_t is bad
// by using time_t here we don't care how it's defined
// however, we leave the old defs alone for compatibility

#if _WIN32_WCE >= 420

_inline char * ctime(const time_t *timp)
{
    return __helix_ctime((long*)timp);
}

_inline char * asctime (const struct tm *tm)
{
    return __helix_asctime((struct tm*)tm);
}

_inline
time_t time(time_t *t) 
{
    return __helix_time((long *)t);
}


_inline
time_t mktime(struct tm* tm)
{
    return __helix_mktime(tm);
}

_inline
struct tm* localtime(const time_t* timep)
{
    return __helix_localtime((long *)timep);
}

_inline
struct tm* gmtime(const time_t *timep)
{
    return __helix_gmtime((long*)timep);
}

#else // WinCE < 4.2 doesn't include time.h and uses the older defs

_inline char * ctime(time_t *timp)
{
    return __helix_ctime((long*)timp);
}

_inline char * asctime (struct tm *tm)
{
    return __helix_asctime(tm);
}

_inline
struct tm* localtime(time_t* timep)
{
    return __helix_localtime((long *)timep);
}

_inline
long time(time_t *t) 
{
    return __helix_time((long *)t);
}


_inline
long mktime(struct tm* tm)
{
    return __helix_mktime(tm);
}

_inline
struct tm* gmtime(time_t *timep)
{
    return __helix_gmtime((long*)timep);
}


#endif

#elif defined(_OPENWAVE)
#define time(t)			__helix_time(t)
#define ctime(t)		__helix_ctime(t)
#define gmtime(t)		__helix_gmtime(t)
#define localtime(t)	__helix_gmtime(t) // XXXSAB is there a _local_ time call?
#define mktime(tm)		__helix_mktime(tm)
#define gettimeofday	__helix_gettimeofday

#define strftime op_strftime

#endif /* defined(WIN32_PLATFORM_PSPC) */

#if defined(_REENTRANT) && !defined(_WIN32)
#define hx_localtime_r localtime_r
#define hx_gmtime_r    gmtime_r
#define hx_asctime_r   asctime_r
#define hx_ctime_r     ctime_r

//#define localtime NON_REENTRANT_localtime_CALLED
//#define gmtime    NON_REENTRANT_gmtime_CALLED
//#define asctime   NON_REENTRANT_asctime_CALLED
//#define ctime     NON_REENTRANT_ctime_CALLED

#else
#define hx_localtime_r(pClock,pRes) ((struct tm*)memcpy((pRes), localtime(pClock), sizeof(struct tm)))
#define hx_gmtime_r(pClock,pRes)    ((struct tm*)memcpy((pRes), gmtime(pClock), sizeof(struct tm)))
#define hx_asctime_r(tm,pRes)       ((char*)memcpy(pRes, asctime(tm), 26))
#define hx_ctime_r(pClock,pRes)     ((char*)memcpy(pRes, asctime(localtime(pClock)), 26))
#endif /* _REENTRANT */

#if defined (_BREW)

#define localtime(t)	__helix_gmtime(t) 
typedef unsigned long time_t;
typedef struct _SYSTEMTIME { 
    UINT16 wYear; 
    UINT16 wMonth; 
    UINT16 wDayOfWeek; 
    UINT16 wDay; 
    UINT16 wHour; 
    UINT16 wMinute; 
    UINT16 wSecond; 
    UINT16 wMilliseconds; 
}SYSTEMTIME;

#ifndef _CLOCK_T_DEFINED
typedef long clock_t;
#define _CLOCK_T_DEFINED
#endif
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC  1000
#endif

#define strftime __helix_strftime

HLX_INLINE long
mktime(struct tm* tm)
{
    return __helix_mktime(tm);
}

long __helix_convertTime(SYSTEMTIME *pS, int *pDayOfYear);

HLX_INLINE long
time(long *t) 
{
    HX_ASSERT(0);
    return 0;
}

HLX_INLINE
struct tm* gmtime(time_t *timep)
{
    return __helix_gmtime((long*)timep);
}

size_t __helix_strftime(char *ptr,size_t maxsize,const char *format,const struct tm *timeptr);

#endif //_BREW

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* HLXSYS_TIME_H */
