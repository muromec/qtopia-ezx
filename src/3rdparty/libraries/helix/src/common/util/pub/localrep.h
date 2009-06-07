/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: localrep.h,v 1.4 2004/07/09 18:23:37 hubbe Exp $
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

/*
 *
 *
 * Abstraction:
 * This file contains the declaration of routines that will enable the extraction of data
 * used in elements of the ui in their appropriate localized form.
 *
 * Targets:	Windows 95/NT, 3.1, MacOS, X-Windows
 * Module:	HXwindows Cross-Platform Development Framework
 *
 */
#ifndef _LOCALREP_H_
#define _LOCALREP_H_

// Includes for this file...
#include <stdlib.h>
#include "hlxclib/time.h"

#ifdef __cplusplus		
extern "C" {
#endif  /* __cplusplus */



// Flags to use with HXGetLocalTime...
#define HXLOCALTIMESTRING_NOSECONDS		0x00000001
#define HXLOCALTIMESTRING_NOMINUTESORSECONDS	0x00000002
#define HXLOCALTIMESTRING_NOTIMEMARKER		0x00000004
#define HXLOCALTIMESTRING_24HOURFORMAT		0x00000008
#define HXLOCALTIMESTRING_DEFAULT		0x00000000


/*
 * HXGetLocalTimeString
 * --------------------
 * Returns in the buffer the formated string for the time given, or for the local time if the time
 * given is 0.
 *
 * input:
 * char *buffer			- Buffer to copy string to.
 * INT32 sizeOfBuffer		- Size of buffer to copy string to.
 * const char *formatString	- String that contains the format of the resultant time string.  The allowed
 *				  formatting commands are:  
 *					h   Hours with no leading zero for single-digit hours; 12-hour clock 
 *					hh  Hours with leading zero for single-digit hours; 12-hour clock 
 *					H   Hours with no leading zero for single-digit hours; 24-hour clock 
 *					HH  Hours with leading zero for single-digit hours; 24-hour clock 
 *					m   Minutes with no leading zero for single-digit minutes 
 *					mm  Minutes with leading zero for single-digit minutes 
 *					s   Seconds with no leading zero for single-digit seconds 
 *					ss  Seconds with leading zero for single-digit seconds 
 *					t   One character time marker string, such as A or P 
 *					tt  Multicharacter time marker string, such as AM or PM 
 *				  For example, to get the time string “11:29:40 PM” use the following picture string: “hh':'mm':'ss tt” 
 * ULONG32 flags		- Flags are listed above.
 * time_t time			- Number of milliseconds since 1970.
 * ULONG32 locale		- Optional local id,  if not included, the default system locale will be used.
 *
 * output:
 * INT32			- Number of characters written to the buffer.  0 if failed.
 * 
 */
INT32 HXGetLocalTimeString(char *buffer, INT32 sizeOfBuffer, const char *formatString, ULONG32 flags, time_t time, ULONG32 locale);



// Constants to be used as flags for the following routine...
#define HXLOCALDATESTRING_SHORTDATE	   0x00000001
#define HXLOCALDATESTRING_LONGDATE	   0x00000002
#define HXLOCALDATESTRING_DEFAULT	   0x00000000


/*
 * HXGetLocalDateString
 * --------------------
 * Returns in the buffer the formated string for the given date, or for the current date if the time
 * given is 0.
 *
 * input:
 * char *buffer			- Buffer to copy string to.
 * INT32 sizeOfBuffer		- Size of buffer to copy string to.
 * const char *formatString	- String that contains the format of the resultant time string.  The allowed
 *				  formatting commands are:  
 *				    d Day of month as digits with no leading zero for single-digit days. 
 *				    dd Day of month as digits with leading zero for single-digit days. 
 *				    ddd Day of week as a three-letter abbreviation. The function uses the LOCALE_SABBREVDAYNAME value associated with the specified locale. 
 *				    dddd Day of week as its full name. The function uses the LOCALE_SDAYNAME value associated with the specified locale. 
 *				    M Month as digits with no leading zero for single-digit months. 
 *				    MM Month as digits with leading zero for single-digit months. 
 *				    MMM Month as a three-letter abbreviation. The function uses the LOCALE_SABBREVMONTHNAME value associated with the specified locale. 
 *				    MMMM Month as its full name. The function uses the LOCALE_SMONTHNAME value associated with the specified locale. 
 *				    y Year as last two digits, but with no leading zero for years less than 10. 
 *				    yy Year as last two digits, but with leading zero for years less than 10. 
 *				    yyyy Year represented by full four digits. 
 *				    gg Period/era string. The function uses the CAL_SERASTRING value associated with the specified locale. This element is ignored if the date to be formatted does not have an associated era or period string. 
 *				    For example, to get the date string “Wed, Aug 31 94” use the following picture string: “ddd',' MMM dd yy” 
 * ULONG32 flags		- Flags are listed above.
 * time_t time			- Number of seconds since 1970.
 * ULONG32 locale		- Optional locale id, if not included, the default system locale will be used.
 *
 * output:
 * INT32			- Number of characters written to the buffer.  0 if failed.
 * 
 */
INT32 HXGetLocalDateString(char *buffer, INT32 sizeOfBuffer, const char *formatString, ULONG32 flags, time_t time, ULONG32 locale);
void HXGetLocalDateFormatString(char *buffer, INT32 length);

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
char HXGetLocalDecimalPoint(ULONG32 locale);


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
char HXGetLocalTimeSeparator(ULONG32 locale);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif // _LOCALREP_H_
