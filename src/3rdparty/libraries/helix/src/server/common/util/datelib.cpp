/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: datelib.cpp,v 1.4 2003/03/04 05:41:53 dcollins Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _UNIX
#include <time.h>
#endif

#include "datelib.h"
#include "hxstrutl.h"


time_t
convert_to_timet(int month, int day, int year)
{
    if (year == 0)
    {
        return 0;
    }
    struct tm time_struct;
    memset (&time_struct, 0, sizeof(struct tm));
    time_struct.tm_mon = month - 1;
    time_struct.tm_mday = day;
    time_struct.tm_year = year - 1900;
    if (time_struct.tm_year < 0)
    {
        return -2;
    }
    time_struct.tm_isdst = -1;
    time_struct.tm_wday = 0;
    time_struct.tm_yday = 0;
    time_struct.tm_sec = 0;
    time_struct.tm_min = 0;
    time_struct.tm_hour = 0;
    return mktime(&time_struct);
}

time_t 
string_date_to_time_t(const char* date_string)
{
    if(strcasecmp((const char*)UNLIMITED_DATE_STRING, date_string) == 0)
    {
        return 0;
    }

    struct tm t;
    memset (&t, 0, sizeof(struct tm));
    int r = sscanf(date_string, "%d/%d/%d", &t.tm_mon, &t.tm_mday, &t.tm_year);

    if (r != 3)
    {
	return -2;
    }

    t.tm_isdst = -1;
    t.tm_mon--;  // Months are zero indexed
    t.tm_year -= 1900;

    return mktime(&t);
}
