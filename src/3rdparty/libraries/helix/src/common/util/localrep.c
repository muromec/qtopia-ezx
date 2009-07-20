/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: localrep.c,v 1.14 2005/07/20 21:45:19 dcollins Exp $
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

// Includes for this file...
#include "hxtypes.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/time.h"
#include "localrep.h"

#if defined(_WINDOWS)
#include "hlxclib/windows.h"
#include <windowsx.h>
#endif

#if defined(_MACINTOSH)
#include <stdio.h>
#include <string.h>
#endif


// For debugging...
//#include "hxassert.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _OPENWAVE
#undef WIN32                    /* simulator build... */
#endif

// Local function prototypes...
ULONG32 TranslateLocalTimeStringFlags(ULONG32 flags);
ULONG32 TranslateLocalDateStringFlags(ULONG32 flags);


/*
 * HXGetLocalTimeString
 * --------------------
 * Returns in the buffer the formated string for the time given, or for the local time if the time
 * given is 0.
 */
INT32 HXGetLocalTimeString(char *buffer, INT32 sizeOfBuffer, const char *formatString, ULONG32 flags, time_t time, ULONG32 locale)
{
    INT32 result = 0;
#ifdef WIN32
	// Translate the flags for this system...    
	ULONG32 sysflags = TranslateLocalTimeStringFlags(flags);

	// Copy the time into the appropriate format for the routine to use...
    SYSTEMTIME sysTime;
    if (time > 0)
    {	
	struct tm tmtime;
        hx_localtime_r(&time, &tmtime);
	sysTime.wYear       = tmtime.tm_year + 1900;
	sysTime.wMonth      = tmtime.tm_mon + 1;
	sysTime.wDayOfWeek  = tmtime.tm_wday;
	sysTime.wDay        = tmtime.tm_mday;
	sysTime.wHour       = tmtime.tm_hour;
	sysTime.wMinute     = tmtime.tm_min;
	sysTime.wSecond     = tmtime.tm_sec;
	sysTime.wMilliseconds = 0;
    }

    // Get the time format now...
    result = GetTimeFormat(locale ? locale : LOCALE_SYSTEM_DEFAULT, 
			   sysflags, (time > 0)? &sysTime : NULL, 
			   OS_STRING(formatString), 
			   OS_STRING2(buffer, sizeOfBuffer), 
			   sizeOfBuffer);
#endif
#ifdef _MACINTOSH
    if (time > 0)
    {	
	struct tm *tmtime;
        hx_localtime_r(&time, &tmtime);
	char format[32]; /* Flawfinder: ignore */
	switch (flags)
	{
	    case HXLOCALTIMESTRING_24HOURFORMAT:
	    {
	        strcpy(format, "%H:%M"); /* Flawfinder: ignore */
	    }
	    break;
	    case HXLOCALTIMESTRING_NOMINUTESORSECONDS:
	    {
	    	sprintf(format, "%d", (((tmtime.tm_hour == 0) || (tmtime.tm_hour - 12 == 0))? 12 : ((tmtime.tm_hour > 12)? tmtime.tm_hour - 12 : tmtime.tm_hour))); /* Flawfinder: ignore */
	    	strcat(format, " %p"); /* Flawfinder: ignore */
	    }
	    break;
	    case HXLOCALTIMESTRING_NOSECONDS:
	    default:
	    {
	    	sprintf(format, "%d", (((tmtime.tm_hour == 0) || (tmtime.tm_hour - 12 == 0))? 12 : ((tmtime.tm_hour > 12)? tmtime.tm_hour - 12 : tmtime.tm_hour))); /* Flawfinder: ignore */
	    	strcat(format, ":%M %p"); /* Flawfinder: ignore */
	    }
	    break;
	}
	strftime(buffer, sizeOfBuffer, format, &tmtime);
    }
#endif

    return result;
}










#ifdef _WIN32
// Global string for formate date....
char gFormatString[255]; /* Flawfinder: ignore */
#endif


#ifdef WIN32


// Counts the number of instances of the given letter in a string...
INT32 CountChar(char *str, char find)
{
    int count, i;
    i = 0;
    count = 0;

    if (str == NULL) return 0;

    while (TRUE)
    {
	if (str[i] == '\0') break;
	if (find == str[i++]) count++;

    }
    return count;
}

// This routine will return the shortest date string format for the
// default system default into the global gFormatString buffer.
HXBOOL CALLBACK EnumDateFormatsCallBack(LPTSTR formatString)
{
    // Make some assumptions....
    //HX_ASSERT(gFormatString != NULL);
    strncpy(gFormatString, formatString, 254); /* Flawfinder: ignore */
    gFormatString[254] = '\0';

    // If the format string has two digits for year, return false right away...
    if ((CountChar(gFormatString, 'y') == 2) && (CountChar(gFormatString, 'M') == 1) && (CountChar(gFormatString, 'd') == 1)) return FALSE;
    return TRUE;
}

#endif // WIN32





/*
 * HXGetLocalDataFormatString
 * --------------------------
 * Returns a format string for the local machine.  This function needs to be expanded at some point...right now it will only return a short format.
 *
 * input:
 * char *buffer		- Buffer.
 * INT32 length		- Size of buffer.
 *
 */
void HXGetLocalDateFormatString(char *buffer, INT32 length)
{
#ifdef WIN32
    // Create a new format string globally...
    *gFormatString = '\0';
    EnumDateFormats((DATEFMT_ENUMPROC)EnumDateFormatsCallBack, LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE);

    // Save the global string onto the buffer and release the global's memory...
    strncpy(buffer, gFormatString, length); /* Flawfinder: ignore */
    buffer[length-1] = '\0';
    *gFormatString = '\0';
#else
	//HX_ASSERT(!"HXGetLocalDateFormatString is not defined for this platform.");
#endif
}






/*
 * HXGetLocalDateString
 * --------------------
 * Returns in the buffer the formated string for the date given, or for the current date if the date 
 * given is 0.
 */
INT32 HXGetLocalDateString(char *buffer, INT32 sizeOfBuffer, const char *formatString, ULONG32 flags, time_t time, ULONG32 locale)
{
    INT32 result = 0;


#ifdef WIN32
    // Translate the flags for this system...
    ULONG32 sysflags = TranslateLocalDateStringFlags(flags);

	char tempFormatString[256]; /* Flawfinder: ignore */

    // Copy the time into the appropriate format for the routine to use...
    SYSTEMTIME sysTime;
    if (time > 0)
    {	
	struct tm tmtime;
        hx_localtime_r(&time, &tmtime);
	sysTime.wYear       = tmtime.tm_year + 1900;
	sysTime.wMonth      = tmtime.tm_mon + 1;
	sysTime.wDayOfWeek  = tmtime.tm_wday;
	sysTime.wDay        = tmtime.tm_mday;
	sysTime.wHour       = tmtime.tm_hour;
	sysTime.wMinute     = tmtime.tm_min;
	sysTime.wSecond     = tmtime.tm_sec;
	sysTime.wMilliseconds = 0;
    }


    if (formatString == NULL) HXGetLocalDateFormatString(tempFormatString, 255); 

    // Get the time format now...
    result = GetDateFormat( locale ? locale : LOCALE_SYSTEM_DEFAULT, 
			    sysflags, 
			    (time > 0)? &sysTime : NULL, 
			    (formatString)? formatString : tempFormatString, 
			    buffer, 
			    sizeOfBuffer);
#endif
#ifdef _MACINTOSH
    if (time >  0)
    {	
	struct tm tmtime;
        hx_localtime_r(&time, &tmtime);
	char format[32];/* Flawfinder: ignore */
	switch (flags)
	{
	    case HXLOCALDATESTRING_SHORTDATE:
	        strcpy(format, "%x"); /* Flawfinder: ignore */ // 10 Aug, 1999
	    	break;
	    case HXLOCALDATESTRING_LONGDATE:
	    	strcpy(format, "%B %d, %Y"); /* Flawfinder: ignore */ // August 10, 1999
	    	break;
	    case HXLOCALDATESTRING_DEFAULT:
	    default:
	    	strcpy(format, "%m/%d"); /* Flawfinder: ignore */ // 10/8
	    	break;
	}
	strftime(buffer, sizeOfBuffer, format, &tmtime);
    }
#endif

    return result;
}





/*
 * TranslateLocalTimeStringFlags
 * -----------------------------
 * Translage the flags to their system specific counterparts.
 *
 * input:
 * ULONG32 flags	- Flags to translate.
 *
 * output:
 * ULONG32		- Resultant flags, maybe system specific.
 *
 */
ULONG32 TranslateLocalTimeStringFlags(ULONG32 flags)
{
#ifdef WIN32
    ULONG32 systemSpecificFlags = 0;
#else
    ULONG32 systemSpecificFlags = flags;
#endif

    if (flags & HXLOCALTIMESTRING_NOSECONDS)
    {
#ifdef WIN32
	systemSpecificFlags |= TIME_NOSECONDS;
#endif
    }
    if (flags & HXLOCALTIMESTRING_NOMINUTESORSECONDS)
    {
#ifdef WIN32
	systemSpecificFlags |= TIME_NOMINUTESORSECONDS;
#endif
    }
    if (flags & HXLOCALTIMESTRING_NOTIMEMARKER)
    {
#ifdef WIN32
	systemSpecificFlags |= TIME_NOTIMEMARKER;
#endif
    }
    if (flags & HXLOCALTIMESTRING_24HOURFORMAT)
    {
#ifdef WIN32
	systemSpecificFlags |= TIME_FORCE24HOURFORMAT;
#endif
    }
    if (flags == HXLOCALTIMESTRING_DEFAULT)
    {
#ifdef WIN32
	systemSpecificFlags = 0;
#endif
    }
    return systemSpecificFlags;
}





/*
 * TranslateLocalDateStringFlags
 * -----------------------------
 * Translage the flags to their system specific counterparts.
 *
 * input:
 * ULONG32 flags	- Flags to translate.
 *
 * output:
 * ULONG32		- Resultant flags, maybe system specific.
 *
 */
ULONG32 TranslateLocalDateStringFlags(ULONG32 flags)
{
#ifdef WIN32
    ULONG32 systemSpecificFlags = 0;
#else
    ULONG32 systemSpecificFlags = flags;
#endif

    if (flags & HXLOCALDATESTRING_SHORTDATE)
    {
#ifdef WIN32
	systemSpecificFlags = DATE_SHORTDATE;
#endif 
    }
    if (flags & HXLOCALDATESTRING_LONGDATE) 
    {
#ifdef WIN32
	systemSpecificFlags = DATE_LONGDATE;
#endif 
    }
    if (flags == HXLOCALDATESTRING_DEFAULT)
    {
#ifdef WIN32
	systemSpecificFlags = 0;
#endif 
    }
    return systemSpecificFlags;
}

/*
 * HXGetLocalDecimalPoint
 * --------------------
 * Returns decimal point representation in the locale selected.
 *
 * input:
 * ULONG32 locale	- Optional locale id, if not included, the default system locale will be used.
 *
 * output:
 * char			- Decimal point representation.  
 * 
 */
char HXGetLocalDecimalPoint(ULONG32 locale)
{
        char result = '.';

#ifdef WIN32
	char buffer[3]; /* Flawfinder: ignore */
	if(GetLocaleInfo(locale ? locale : LOCALE_SYSTEM_DEFAULT, LOCALE_SDECIMAL, buffer, 2) > 1)
	    result = buffer[0];
#elif defined(_MACINTOSH)
	{
		Intl0Hndl intlHandle;
		
		check(locale == 0);

		intlHandle = (Intl0Hndl) GetIntlResource(0);	
		if (intlHandle)
		{
			result = (**intlHandle).decimalPt;
		}
	}

#endif 
	return result;
}


/*
 * HXGetLocalTimeSeparator
 * --------------------
 * Returns time separator representation in the locale selected.
 *
 * input:
 * ULONG32 locale	- Optional locale id, if not included, the default system locale will be used.
 *
 * output:
 * char			- Time separator representation.  
 * 
 */
char HXGetLocalTimeSeparator(ULONG32 locale)
{
        char result = ':';

#if defined(_MACINTOSH)

	{
		Intl0Hndl intlHandle;
		
		check(locale == 0);
		
		intlHandle = (Intl0Hndl) GetIntlResource(0);	
		if (intlHandle)
		{
			result = (**intlHandle).timeSep;
		}
	}
	
#endif

#ifdef WIN32
	char buffer[3]; /* Flawfinder: ignore */
	if(GetLocaleInfo(locale ? locale : LOCALE_SYSTEM_DEFAULT, LOCALE_SDECIMAL, buffer, 2) > 1)
	    result = buffer[0];
#endif 
	return result;
}

