/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdate.cpp,v 1.15 2008/01/18 07:35:17 vkathuria Exp $
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

#include "hxtypes.h"

#include "hlxclib/string.h"
#include "hlxclib/stdio.h"
#include "hlxclib/time.h"

#ifdef _MACINTOSH
#ifndef _MAC_MACHO
#include <OSUtils.h>
#endif
#ifdef _CARBON
#ifndef _MAC_MACHO
#include <stat.h>
#endif
#else
#include <stat.mac.h>
#endif
#else
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#endif
#if defined(_BREW)
#include "hxassert.h"
#include "hlxclib/ctype.h"
#endif
#if defined(_AIX)
#include <ctype.h>
#endif
#ifdef _WINDOWS
#include "hlxclib/windows.h"
#include <wininet.h>
#include "hlxclib/io.h"
#endif /* _WINDOWS */
#include "hxstrutl.h"
#include "hxdate.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_OPENWAVE)

HX_DATETIME 
HX_GET_DATETIME(void)
{
    op_tm tm;
    time_t t = op_time();
    HX_DATETIME dt;

    // XXXSAB - localtime?
    op_gmtime(t, &tm);
    dt.second = tm.tm_sec;
    dt.minute = tm.tm_min;
    dt.hour = tm.tm_hour;
    dt.dayofweek = tm.tm_wday;
    dt.dayofmonth = tm.tm_mday;
    dt.dayofyear = tm.tm_yday;
    dt.month = tm.tm_mon + 1; // 0 based (11 = December)
    dt.year = tm.tm_year;
    dt.gmtDelta = 0; // or something
    return dt;
}

#elif (defined(_WINDOWS) || defined(WIN32) || defined(_WIN32) || defined(_WINCE))

HX_DATETIME HX_GET_DATETIME(void)
{
	HX_DATETIME datetime;
#ifdef _WINCE
	TIME_ZONE_INFORMATION tzinfo;

	::GetTimeZoneInformation(&tzinfo);
#else	
	// init the timezone data
	_tzset();
#endif /* _WINCE */

	// Get the local time
	time_t lTime;
	struct tm tmDateTime;
	time(&lTime);
        hx_localtime_r(&lTime, &tmDateTime);

	datetime.second		= tmDateTime.tm_sec;
	datetime.minute		= tmDateTime.tm_min;
	datetime.hour		= tmDateTime.tm_hour;
	datetime.dayofweek	= tmDateTime.tm_wday;
	datetime.dayofmonth	= tmDateTime.tm_mday;
	datetime.dayofyear	= tmDateTime.tm_yday + 1;
	datetime.month		= tmDateTime.tm_mon + 1;
	datetime.year		= tmDateTime.tm_year;
#ifdef _WINCE
	datetime.gmtDelta	= -1 * (tzinfo.Bias / 60);  // minutes to hours
#else
	datetime.gmtDelta	= -1*(_timezone/3600); //seconds to hours
#endif /* _WINCE */

	return datetime;
}

#elif _MACINTOSH
#ifndef _MAC_MACHO
#include <datetimeutils.h>
#endif
HX_DATETIME HX_GET_DATETIME(void)
{
	HX_DATETIME datetime;

	ULONG32 secs;
	LongDateRec ldate;
	
	::GetDateTime(&secs);

	LongDateTime time = secs;

	::LongSecondsToDate(&time, &ldate);

	datetime.second		= ldate.ld.second;
	datetime.minute		= ldate.ld.minute;
	datetime.hour		= ldate.ld.hour;
	datetime.dayofweek	= ldate.ld.dayOfWeek - 1;
	datetime.dayofmonth	= ldate.ld.day;
	datetime.dayofyear	= ldate.ld.dayOfYear;
	datetime.month		= ldate.ld.month;
	datetime.year		= ldate.ld.year - HX_YEAR_OFFSET;
	
	// get time zone in seconds
	MachineLocation loc;
	::ReadLocation(&loc);

	// we need to extend the sign since the gmtDelta is a 3 byte value
	long gmtDelta = loc.u.gmtDelta;
	long internalGmtDelta = gmtDelta & 0x00FFFFFF;

	if(internalGmtDelta & 0x00800000)
	{
		internalGmtDelta = internalGmtDelta | 0xFF000000;
	}

	// convert to hours
	datetime.gmtDelta	= (short)internalGmtDelta/3600; 

	return datetime;
}

#elif defined(_UNIX) || defined(_SYMBIAN)

HX_DATETIME 
HX_GET_DATETIME(void)
{
    struct tm tm;
    time_t t = time(0);
    HX_DATETIME dt;

    hx_localtime_r(&t, &tm);

    dt.second           = tm.tm_sec;
    dt.minute           = tm.tm_min;
    dt.hour             = tm.tm_hour;
    dt.dayofweek        = tm.tm_wday;
    dt.dayofmonth       = tm.tm_mday;
    dt.dayofyear        = tm.tm_yday;
    dt.month            = tm.tm_mon + 1; // 0 based (11 = December)
    dt.year             = tm.tm_year;
    dt.gmtDelta         = 0; // or something
    return dt;
}

// unix code added here

#elif defined(_BREW)
HX_DATETIME 
HX_GET_DATETIME(void)

{
    HX_DATETIME dt;
    time_t t = GETTIMESECONDS();
    JulianType jtcurrentDate = {0,};
    GETJULIANDATE(t, &jtcurrentDate);

    dt.second           = jtcurrentDate.wSecond;
    dt.minute           = jtcurrentDate.wMinute;
    dt.hour             = jtcurrentDate.wHour;
    dt.dayofweek        = (jtcurrentDate.wWeekDay + 1) % 7;
    //BREW: (0=Monday, 6=Sunday), Helix((0=Sunday, 6=Saturday)
    dt.dayofmonth       = jtcurrentDate.wDay;
    dt.month            = jtcurrentDate.wMonth;
    dt.year             = jtcurrentDate.wYear;
    dt.gmtDelta         = 0; // or something

    dt.dayofyear = jtcurrentDate.wDay; // for current month
    int i = 1;
    while (i < dt.month)
    {
	switch(i)
	{
	    case 1: // jan, mar, may, july, aug, oct, dec
	    case 3: 
	    case 5: 
	    case 7: 
	    case 8: 
	    case 10: 
		    dt.dayofyear += 31;
		    break;
    	    case 2: 
		    dt.dayofyear += 28;
		    if (dt.year % 400 == 0 || (dt.year % 4 == 0 && dt.year % 100 != 0)) 
		    {
			dt.dayofyear++;
		    }
		    break;
		    dt.dayofyear += 31;
		    break;
	    case 4: 
	    case 6: 
	    case 9: 
	    case 11: 
		    dt.dayofyear += 30;
		    break;
	    default:
	    HX_ASSERT(0);
	    break;
	}
	i++;
    }

    return dt;
}
#endif // _WIN32 

//  Returns the number of the month given or -1 on error.
int MonthNo (char * month)
{
    int ret = -1;
    
    if (!strncasecmp(month, "JAN", 3))
        ret = 0;
    else if (!strncasecmp(month, "FEB", 3))
        ret = 1;
    else if (!strncasecmp(month, "MAR", 3))
        ret = 2;
    else if (!strncasecmp(month, "APR", 3))
        ret = 3;
    else if (!strncasecmp(month, "MAY", 3))
        ret = 4;
    else if (!strncasecmp(month, "JUN", 3))
        ret = 5;
    else if (!strncasecmp(month, "JUL", 3))
        ret = 6;
    else if (!strncasecmp(month, "AUG", 3))
        ret = 7;
    else if (!strncasecmp(month, "SEP", 3))
        ret = 8;
    else if (!strncasecmp(month, "OCT", 3))
        ret = 9;
    else if (!strncasecmp(month, "NOV", 3))
        ret = 10;
    else if (!strncasecmp(month, "DEC", 3))
        ret = 11;
    else 
    {
        ret = -1;
    }

    return ret;
}

time_t ParseDate(char *date_string)
{
#ifdef _OPENWAVE
    struct op_tm tm;
    if (op_time_parse_http_date(date_string, &tm))
    {
        return op_timegm(&tm);
    }
    return 0;
#else
    struct  tm time_info;         // Points to static tm structure
    char*   ip = NULL;
    char    mname[256] = {0}; /* Flawfinder: ignore */
    time_t  rv;

    memset(&time_info, 0, sizeof(struct tm));

    // Whatever format we're looking at, it will start with weekday.
    // Skip to first space
    if(!(ip = strchr(date_string,' ')))
    {
        return 0;
    }
    else
    {
	while(IS_SPACE(*ip))
	{
            ++ip;
	}
    }

    /* make sure that the date is less than 256 
     * That will keep mname from ever overflowing 
     */
    if(255 < strlen(ip))
    {
	return 0;
    }

    if(isalpha(*ip)) 
    {
	// ctime
	sscanf(ip, (strstr(ip, "DST") ? "%s %d %d:%d:%d %*s %d"
					: "%s %d %d:%d:%d %d"),
		   mname,
		   &time_info.tm_mday,
		   &time_info.tm_hour,
		   &time_info.tm_min,
		   &time_info.tm_sec,
		   &time_info.tm_year);
	time_info.tm_year -= 1900;
    }
    else if(ip[2] == '-') 
    {
        // RFC 850 (normal HTTP)
        char t[256] = {0}; /* Flawfinder: ignore */
        sscanf(ip,"%s %d:%d:%d", t,
		    &time_info.tm_hour,
		    &time_info.tm_min,
		    &time_info.tm_sec);
        t[2] = '\0';
        time_info.tm_mday = atoi(t);
        t[6] = '\0';
        SafeStrCpy(mname, &t[3], 256);
        time_info.tm_year = atoi(&t[7]);
        // Prevent wraparound from ambiguity
        if(time_info.tm_year < 70)
	{
            time_info.tm_year += 100;
	}
	else if(time_info.tm_year > 1900)
	{
	    time_info.tm_year -= 1900;
	}
    }
    else 
    {
        // RFC 822
        sscanf(ip,"%d %s %d %d:%d:%d",&time_info.tm_mday,
				    mname,
				    &time_info.tm_year,
				    &time_info.tm_hour,
				    &time_info.tm_min,
				    &time_info.tm_sec);

	// since tm_year is years since 1900 and the year we parsed
 	// is absolute, we need to subtract 1900 years from it
	time_info.tm_year -= 1900;
    }
    time_info.tm_mon = MonthNo(mname);

    if(time_info.tm_mon == -1)
    {
	return 0;
    }

    rv = mktime(&time_info);
	
#ifndef NO_TM_ISDST
    if(time_info.tm_isdst)
    {
	rv -= 3600;
    }
#endif /* NO_TM_ISDST */

    if(rv == -1)
    {
        return(0);
    }
    else
    {
	return(rv);
    }
#endif /* _OPENWAVE */
}
