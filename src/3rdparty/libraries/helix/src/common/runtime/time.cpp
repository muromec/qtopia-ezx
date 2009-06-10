/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: time.cpp,v 1.13 2008/01/18 09:17:26 vkathuria Exp $
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

#include "hlxclib/time.h"
#include "hlxclib/assert.h"
#include "hlxclib/limits.h"
#include "hlxclib/stdio.h"

#if !defined(_WINCE) && !defined(_BREW)
struct tm* __helix_localtime(time_t* timep)
{
    assert(!"__helix_localtime(): Not implemented\n");
    return 0;
}
#endif /* _WINCE */


#ifdef _WINCE

struct tm* __helix_localtime(long* pTime);
char * __helix_asctime(struct tm *pTm);
long __helix_mktime(struct tm* pTm);

void __helix_tzset()
{
    // not needed for this wince time implementation
    return;
}

char * __helix_ctime(long *pTime)
{
    struct tm *pTm;

    pTm = __helix_localtime(pTime);
    if (!pTm)
	return NULL;

    return __helix_asctime(pTm);
}

char *DAY_NAMES[] = {
    "Sun"
    , "Mon"
    , "Tue"
    , "Wed"
    , "Thu"
    , "Fri"
    , "Sat"
};
char *MONTH_NAMES[] = {
    "Jan"
    ,"Feb"
    ,"Mar"
    ,"Apr"
    ,"May"
    ,"Jun"
    ,"Jul"
    ,"Aug"
    ,"Sep"
    ,"Oct"
    ,"Nov"
    ,"Dec"
};
//--------------------------------------------------------
// build date string in the form ddd mmm dd hh:mm:ss yyyy

char * __helix_asctime(struct tm *pTm)
{
    static char buf[80];

    if (!pTm)
	return NULL;

    strcpy(buf,DAY_NAMES[pTm->tm_wday]);
    buf[3] = ' ';
    strcpy(&buf[4],MONTH_NAMES[pTm->tm_mon]);
    buf[7] = ' ';

    sprintf(&buf[8],"%02d %02d:%02d:%02d %04d\n",pTm->tm_mday,
	pTm->tm_hour,
	pTm->tm_min,
	pTm->tm_sec,
	pTm->tm_year+1970);
    return buf;

}


int SECONDS_IN_MONTH[12] = {
    31*24*60*60		// jan
    ,28*24*60*60 	// feb
    ,31*24*60*60	// mar
    ,30*24*60*60	// apr
    ,31*24*60*60	// may
    ,30*24*60*60	// jun
    ,31*24*60*60	// jul
    ,31*24*60*60	// aug
    ,30*24*60*60	// sep
    ,31*24*60*60	// oct
    ,30*24*60*60	// nov
    ,31*24*60*60	// dec
};

//--------------------------------------------------------
struct tm *__helix_gmtime(long *pTime)
{
    static tm tm;
    
    memset(&tm,0,sizeof(tm));
    if ( !pTime || *pTime < 0)
	return &tm;

    long t = *pTime;
    tm.tm_wday = (t/(24*60*60) + 4) % 7; // day of the week

    tm.tm_year = 1970;
    while (t > 0)
    {
	t -= 365L * 24L * 60L * 60L; // seconds/non leap year
	if (t < 0)
	{
	    t += 365L * 24L * 60L * 60L;
	    break;
	}
	if ((tm.tm_year & 3) == 0) // leap year
	    t -= 24L * 60L * 60L; // leap year day
	if (t < 0)
	{
	    t += 24L * 60L * 60L;
	    t += 365L * 24L * 60L * 60L;
	    break;
	}
	++tm.tm_year;
    }
    tm.tm_yday = t/(24L * 60L * 60L); //day of the year

    while (t > 0 && tm.tm_mon < 11)
    {
	t -= SECONDS_IN_MONTH[tm.tm_mon];
	if (t < 0)
	{
	    t += SECONDS_IN_MONTH[tm.tm_mon];
	    break;
	}
	if ((tm.tm_year & 3) == 0 && tm.tm_mon == 1) // leap year and feb
	    t -= 24*60*60;
	if (t < 0)
	{
	    t += 24*60*60;
	    t += SECONDS_IN_MONTH[tm.tm_mon];
	    break;
	}
	++tm.tm_mon;
    }

    tm.tm_mday = t/(24*60*60);
    t -= tm.tm_mday * 24*60*60;
    ++tm.tm_mday; // day is 1..31

    tm.tm_hour = t/(60*60); //0..23
    t -= tm.tm_hour * 60*60; 

    tm.tm_min = t/60; // 0..60
    t -= tm.tm_min * 60;

    tm.tm_sec = t;

    tm.tm_year -= 1900;

    return &tm;
}
//--------------------------------------------------------
struct tm* __helix_localtime(long* pTime)
{
    tm *pTm = NULL;
    long t;
    TIME_ZONE_INFORMATION tz;
    tm s,e; // start and end date of dst
    long sabs,eabs; // absolute time of start/end dst

    if (!pTime || *pTime < 0)
	return NULL;

    ::GetTimeZoneInformation(&tz);
    t = *pTime - tz.Bias * 60;	// backup to gmt

    pTm = __helix_gmtime(&t); // get local time
    if (!pTm)
	return NULL;

    pTm->tm_isdst = 0; 
    if (tz.StandardDate.wMonth == 0)
	return pTm;

    memset(&s,0,sizeof(tm));
    s.tm_year = pTm->tm_year;
    s.tm_mon = tz.DaylightDate.wMonth-1;
    s.tm_mday = 1;
    s.tm_hour = tz.DaylightDate.wHour;
    sabs = __helix_mktime(&s);
    sabs += (7-s.tm_wday)*24*60*60; // days to 1st sunday

    memset(&e,0,sizeof(tm));
    e.tm_year = pTm->tm_year;
    e.tm_mon = tz.StandardDate.wMonth-1;
    e.tm_mday = 31;
    e.tm_hour = tz.StandardDate.wHour;
    eabs = __helix_mktime(&e);
    eabs -= e.tm_wday*24*60*60; // days to last sunday

    if (((t > sabs) && (t < eabs) && (eabs > sabs)) ||
	(((t > sabs) || (t < eabs)) && (eabs < sabs)))
    { // in dst
	t -= tz.DaylightBias * 60; // take out daylight savings time
	pTm = __helix_gmtime(&t);
	pTm->tm_isdst = 1;
    }

    return pTm;
   
}

//--------------------------------------------------------
long __helix_convertTime(SYSTEMTIME *pS, int *pDayOfYear)
{
    long t = 0;
    int y = pS->wYear;

    for (int i = 0; i < pS->wMonth-1; i++)
	t += SECONDS_IN_MONTH[i];
    if ((pS->wYear & 3) == 0 && pS->wMonth > 2)
	t += 24*60*60;
    t += (pS->wDay - 1)*24*60*60; 
    t += pS->wHour * 60*60;
    t += pS->wMinute * 60;
    t += pS->wSecond;

    *pDayOfYear = t/(24L * 60L * 60L); //day of the year

    while (y > 1970)
    {
	--y;
	t += 365*24*60*60;
	if ((y & 3) == 0)
	    t += 24*60*60;
    }
    pS->wDayOfWeek = (t/(24*60*60) + 4) % 7; // day of the week

    return t;
}

//--------------------------------------------------------
long __helix_time(long *pTime)
{
    long t;
    SYSTEMTIME s;
    int idoy;

    ::GetSystemTime(&s);
    t = __helix_convertTime(&s,&idoy);
    if (pTime)
	*pTime = t;
    return t;
}

//--------------------------------------------------------
long __helix_mktime(struct tm* pTm)
{
    long t;
    SYSTEMTIME s;

    if (!pTm)
	return 0;

    s.wYear = pTm->tm_year+1900;
    s.wMonth = pTm->tm_mon+1;
    s.wDay = pTm->tm_mday;
    s.wHour = pTm->tm_hour;
    s.wMinute = pTm->tm_min;
    s.wSecond = pTm->tm_sec;
    t = __helix_convertTime(&s,&pTm->tm_yday);
    pTm->tm_wday = s.wDayOfWeek;

    return t;

}


#endif /* _WINCE */

#ifdef _OPENWAVE
#include "platform/openwave/hx_OpUtils.h"

time_t __helix_time(time_t* p)
{
    time_t t = op_time();
    if (p) *p = t;
    return t;
}

struct tm *__helix_gmtime(const time_t *timep)
{
    static struct tm structTM;
    op_gmtime(op_time(), &structTM);
    return &structTM;
}

time_t __helix_mktime(struct tm* tm)
{
    // XXXSAB Assume that we're always dealing with UTC time?
    return op_timegm(tm);
}

int __helix_gettimeofday(struct timeval *tv, void *tz)
{
	int ret = 0;
	static U32 carry = 0;
    static const U32 carryIncrement = (UINT_MAX / 1000) + 1;
    static U32 lastT = 0;

    U32 t = op_getMSecs();
    if (t < lastT)
	{
		carry += carryIncrement;
	}
    lastT = t;

	U32 ts = t / 1000;
    tv->tv_sec = ts + carry;
    tv->tv_usec = 1000 * (t - ts * 1000);
	return ret;
}

char * __helix_ctime(const time_t *timer)
{
	char *st = NULL; 
	
	/*
	 	struct op_tm {
			S8 tm_sec;       seconds (0 - 61)         
			S8 tm_min;       minutes (0 - 59)         
			S8 tm_hour;      hours (0 - 23)           
			S8 tm_mday;      day of month (1 - 31)    
			S8 tm_mon;       month of year (0 - 11)   
			S8 tm_wday;      day of week (Sunday = 0) 
			S16 tm_year;     year - 1900              
			S16 tm_yday;     day of year (0 - 365)    
	*/
	op_tm optm;
	if (kTrue == op_gmtime(*timer, &optm))
	{

		/*
		 * based on iso c, return string is precisely 26 chars
		 * in the form of: Wed Jan 02 02:03:55 1980\n\0
		 */
		st = new char[26];
		char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
		char *months[] = {"Jan", "Feb", "Mar", "Api", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		snprintf(st, 26, "%s %s %2u:%2u:%2u:%2u %4u\n\0", weekdays[optm.tm_wday], 
					months[optm.tm_mon],optm.tm_mday, optm.tm_hour, optm.tm_min, optm.tm_sec, optm.tm_year);
	}
	return st;

}
#endif /* _OPENWAVE */

#if defined(_BREW)

time_t time(time_t *t) 
{
    return __helix_time((long *)t);
}

long __helix_time(long *pTime)
{
    HX_ASSERT(0);
    return 0; 
}

long __helix_mktime(struct tm* pTm)
{
    long t;
    SYSTEMTIME s;

    if (!pTm)
	return 0;

    s.wYear = pTm->tm_year+1900;
    s.wMonth = pTm->tm_mon+1;
    s.wDay = pTm->tm_mday;
    s.wHour = pTm->tm_hour;
    s.wMinute = pTm->tm_min;
    s.wSecond = pTm->tm_sec;
    t = __helix_convertTime(&s,&pTm->tm_yday);
    pTm->tm_wday = s.wDayOfWeek;

    return t;
}

int SECONDS_IN_MONTH[12] = {
    31*24*60*60		// jan
    ,28*24*60*60 	// feb
    ,31*24*60*60	// mar
    ,30*24*60*60	// apr
    ,31*24*60*60	// may
    ,30*24*60*60	// jun
    ,31*24*60*60	// jul
    ,31*24*60*60	// aug
    ,30*24*60*60	// sep
    ,31*24*60*60	// oct
    ,30*24*60*60	// nov
    ,31*24*60*60	// dec
};

long __helix_convertTime(SYSTEMTIME *pS, int *pDayOfYear)
{
    long t = 0;
    int y = pS->wYear;

    for (int i = 0; i < pS->wMonth-1; i++)
	t += SECONDS_IN_MONTH[i];

    if ((pS->wYear & 3) == 0 && pS->wMonth > 2)
	t += 24*60*60;

    t += (pS->wDay - 1)*24*60*60; 
    t += pS->wHour * 60*60;
    t += pS->wMinute * 60;
    t += pS->wSecond;

    *pDayOfYear = t/(24L * 60L * 60L); //day of the year

    while (y > 1970)
    {
	--y;
	t += 365*24*60*60;
	if ((y & 3) == 0)
	    t += 24*60*60;
    }
    pS->wDayOfWeek = (t/(24*60*60) + 4) % 7; // day of the week

    return t;
}

struct tm *__helix_gmtime(long *pTime)
{
    HX_ASSERT(0);
    return 0; 
}

size_t __helix_strftime(char *ptr,size_t maxsize,const char *format,const struct tm *timeptr)
{
    HX_ASSERT(0);
    return 0;
}

#endif/*_BREW */
