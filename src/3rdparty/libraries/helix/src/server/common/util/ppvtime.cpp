/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ppvtime.cpp,v 1.3 2005/07/20 21:46:59 dcollins Exp $ 
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

////////////////////////////////////////////////////////////////////////////////
//
// File: 	ppv_time.c
//
// Abstract: 	Implementation of time-to-string and string-to-time conversion 
//		routines used initially by the mSQL 2.0 pay-per-view library.
//		This was needed because of mSQL's lack of support for a time 
//		datatype.
//
// Revision 	1.0 1997/08/01 11:27:45 danr
// Created to assist pay-per-view library for mSQL 2.0.
//
////////////////////////////////////////////////////////////////////////////////

#include "hlxclib/time.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "ppvtime.h"


int ppvTimeToString(const time_t* tTime, char* szTime, int nLen)
// converts a time data type to a string in the following format
// MM/DD/YYYY:HH:MM:SS
{
   struct tm 	tmTime;
   int 		nResult;

   // check for bad length
   if ( PPV_TIMESTRING_LEN > nLen )
   {
	// clear the string, if length will allow
	if (nLen > 1)
	{
	   strcpy(szTime, "");
	}
	return 0;
   }
 
   // convert to time stucture
   hx_localtime_r(tTime, &tmTime);

   // print the string
   nResult = strftime(szTime, PPV_TIMESTRING_LEN, "%m/%d/%Y:%H:%M:%S", &tmTime);
   
   if ( !nResult )
   {
	// clear the string, if length will allow
	if (nLen > 1)
 	{
	   strcpy(szTime, "");
	}
	return 0;
   }

   return 1;

}

// faster null-adding replacements for dangerous strncpy
void strn2cpy(char* t, const char* s) 
{                   
    t[0]=s[0];
    t[1]=s[1];
    t[2]='\0';
}

void strn4cpy(char* t, const char* s) 
{                   
    t[0]=s[0];
    t[1]=s[1];
    t[2]=s[2];
    t[3]=s[3];
    t[4]='\0';
}

int ppvStringToTime(const char* szTime, time_t* tTime)
// converts a string to the time data type.  the string must 
// be in the following format:
// MM/DD/YYYY:HH:MM:SS 
// MM/DD/YYYY HH:MM:SS  (undocumented but accepted)
{
   struct tm	tmTime;
   //struct tm*	tmGetDST;
   time_t	lTime;
   int 		nResult;
   char		szBuffer[5] = "";

   time(&lTime);
   //tmGetDST = localtime(&lTime); XXX Dead code or bug?
 
   // check to make sure it is a good time string
   if ( PPV_TIMESTRING_LEN > strlen(szTime)+1 )
   {
	*tTime = 0;
	return 0;
   } else if ( ('/' != szTime[2]) ||
	       ('/' != szTime[5]) ||
	       (':' != szTime[10] && ' ' != szTime[10]) ||
	       (':' != szTime[13]) ||
	       (':' != szTime[16]) )
   {
	*tTime = 0;
	return 0;
   }

   // get the various values along the way, if they 
   // are bad just bail out....

   // get month
   strn2cpy(szBuffer, szTime);
   nResult = atoi(szBuffer);
   if ( (1 > nResult) ||
 	(12 < nResult) )
   {
	*tTime = 0;
	return 0;
   } else {
	tmTime.tm_mon = nResult-1;
   }

   // get day
   strn2cpy(szBuffer, szTime+3);
   nResult = atoi(szBuffer);
   if ( (1 > nResult) ||
	(31 < nResult) )
   {
	*tTime = 0;
	return 0;
   } else {
	tmTime.tm_mday = nResult;
   }

   // get year
   strn4cpy(szBuffer, szTime+6);
   nResult = atoi(szBuffer);
   if ( 1900 > nResult )
   {
	*tTime = 0;
	return 0;
   } else {
	tmTime.tm_year = (nResult-1900);
   }

   // get hour
   strn2cpy(szBuffer, szTime+11);
   nResult = atoi(szBuffer);
   if ( (0 > nResult) ||
	(23 < nResult) )
   {
	*tTime = 0;
	return 0;
   } else {
	tmTime.tm_hour = nResult;
   }

   // get minute
   strn2cpy(szBuffer, szTime+14);
   nResult = atoi(szBuffer);
   if ( (0 > nResult) ||
	(59 < nResult) )
   {
	*tTime = 0;
	return 0;
   } else {
	tmTime.tm_min = nResult;
   }

   // get second
   strn2cpy(szBuffer, szTime+17);
   nResult = atoi(szBuffer);
   if ( (0 > nResult) ||
    	(61 < nResult) ) // astronomers adding leap seconds??
   {
	*tTime = 0;
	return 0;
   } else { 
	tmTime.tm_sec = nResult;
   }

   // get daylight savings time
   tmTime.tm_isdst = -1;	 //tmGetDST->tm_isdst;

   *tTime = mktime(&tmTime);
   return 1;

}




