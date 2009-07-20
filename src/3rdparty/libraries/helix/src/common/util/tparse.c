/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tparse.c,v 1.11 2007/10/17 15:53:01 praveenkumar Exp $
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
#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
//#include "hlxclib/stdio.h"
#include "safestring.h"

#include "hxtypes.h"
#include "tparse.h"
#include "localrep.h"

#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 


unsigned long TimeParse(const char *s) {
	int colons = 0;
	char *cat;
	char *str;
	const char *ts;
	unsigned long result = 0;
	/* Count colons */
	ts = s;
	while ((cat = strchr(ts, ':')) != 0) {
		colons++;
		ts = cat+1;
	}
	switch(colons) {
		/* We deliberately fall through. */
		case 3:
		/* Days */
		result += atol(s);
		s = strchr(s, ':') + 1;
		case 2:
		/* Hours */
		result *= 24;
		result += atol(s);
		s = strchr(s, ':') + 1;
		case 1:
		/* Minutes  */
		result *= 60;
		result += atol(s);
		s = strchr(s, ':') + 1;
		case 0:
		/* Seconds   */
		result *= 60;
		result += atol(s);
		result *= 10;
		cat = strchr(s, '.');
		if (cat) {
			/* take care of filename $xxx.3 case */
			str = cat+1;
			while(*str) {
			    if(isalpha(*str)) {
			        return 0;
			    }
			    else {
			        str++;
			    }
			} // end of while
			s = cat+1;
			if (*s) {
				/* Quietly drop anything beyond tenths */
				char tenths[2]; /* Flawfinder: ignore */
				tenths[0] = *s;
				tenths[1] = '\0';
				result += atol(tenths);
			}
		}
		/* Finally break out */
		break;
		default:
		/* Invalid */
		return 0;
	}	
	return result;
}	

void TimeOutput(unsigned long value, char *buffer, INT32 nLocaleID)
{
    TimeOutputEx(value, buffer, 128, nLocaleID);
}

void TimeOutputEx(unsigned long value, char *buffer, UINT32 maxlen, INT32 nLocaleID) {
	unsigned long days, hours, minutes, seconds;
	char s[32]; /* Flawfinder: ignore */
	UINT32 currlen = 0;
	UINT32 seglen = 0;
	buffer[0] = '\0';
	days = value / 864000L;
	value -= days * 864000L;
	if (days) {
		seglen = SafeSprintf(s, sizeof(s), "%02d:", (int)days);
		if (seglen + currlen > maxlen)
		    return;
                strcat(buffer, s); /* Flawfinder: ignore */
		currlen += seglen;
	}
	hours = value / 36000L;
	value -= hours * 36000L;
	if (hours || days) {
		seglen = SafeSprintf(s, sizeof(s), "%02d:", (int)hours);
		// the minus 1 accounts for the separator
		if ((seglen - 1) + currlen > maxlen)
		{
		    buffer[currlen - 1] = 0;
		    return;
		}
                strcat(buffer, s); /* Flawfinder: ignore */
		currlen += seglen;
	}
	minutes = value / 600L;
	value -= minutes * 600L;
	seglen = SafeSprintf(s, sizeof(s), "%02d:", (int)minutes);
	// the minus 1 accounts for the separator
	if ((seglen - 1) + currlen > maxlen)
	{
	    buffer[currlen - 1] = 0;
	    return;
	}
        strcat(buffer, s); /* Flawfinder: ignore */
	currlen += seglen;
	seconds = value / 10L;
	value -= seconds * 10L;

	seglen = SafeSprintf(s, sizeof(s), "%02d", (int)seconds);
	s[seglen++] = HXGetLocalDecimalPoint(nLocaleID);
 	s[seglen] = 0;

	// the minus 1 accounts for the separator
	if ((seglen - 1) + currlen > maxlen)
	{
	    buffer[currlen - 1] = 0;
	    return;
	}

        strcat(buffer, s); /* Flawfinder: ignore */
	currlen += seglen;

	seglen = SafeSprintf(s, sizeof(s), "%d", (int)value);
	if (seglen + currlen > maxlen)
	{
	    buffer[currlen - 1] = 0;
	    return;
	}
	currlen += seglen;
        strcat(buffer, s); /* Flawfinder: ignore */
}


