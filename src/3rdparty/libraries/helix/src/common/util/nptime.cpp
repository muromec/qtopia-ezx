/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: nptime.cpp,v 1.11 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "safestring.h"
#include "hlxclib/stdlib.h"
#include "hxtypes.h"
#include "hxstring.h"
#include "hxtime.h"
#include "nptime.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

const INT32 USECS_PER_SEC = 1000000;

NPTime::NPTime()
{
    // default to now
    HXTime now;
    gettimeofday(&now, 0);
    m_lSecond = now.tv_sec;
    m_lMicroSecond = now.tv_usec; 
}

NPTime::NPTime(INT32 sec, INT32 uSec):
    m_lSecond(sec),
    m_lMicroSecond(uSec)
{
}

NPTime::NPTime(INT32 mSec)
{
    fromMSec(mSec);
}

NPTime::NPTime(const char* pTimeString)
{
    fromString(pTimeString);
}

NPTime::NPTime(const char* pTimeString, HXBOOL bAllow_h_min_s_ms_units,
	       HXBOOL& bSucceeded)
{
    bSucceeded = TRUE;

    if (!pTimeString)
    {
	bSucceeded = FALSE;
    }
    else if(strchr(pTimeString, ':'))
    {
	fromString(pTimeString);
    }
    // /Make sure it starts with a number:
    else if ('.' == *pTimeString  ||
	    ('0' <= *pTimeString  &&  '9' >= *pTimeString))
    {
	// /Try h|min|s|ms, e.g., "23min" or "55s", where "s" is the default
	// if there is nothing after the number:
	m_lSecond = m_lMicroSecond = 0;
	char* pEndPtr = 0;
	double dVal = strtod(pTimeString, &pEndPtr);
	UINT32 ulTimeValue = 0;
	// /NOTE: default is seconds, so "23s" and "23" are the same:
	if (!strlen(pEndPtr))
	{
	    ulTimeValue = (UINT32)(dVal * 1000.0);
	}
	else if (bAllow_h_min_s_ms_units)
	{
	    if(strcmp(pEndPtr, "h") == 0)
	    {
		ulTimeValue = (UINT32)(dVal * 60.0 * 60.0 * 1000.0);
	    }
	    else if(strcmp(pEndPtr, "min") == 0)
	    {
		ulTimeValue = (UINT32)(dVal * 60.0 * 1000.0);
	    }
	    else if(strcmp(pEndPtr, "s") == 0)
	    {
		ulTimeValue = (UINT32)(dVal * 1000.0);
	    }
	    else if(strcmp(pEndPtr, "ms") == 0)
	    {
		ulTimeValue = (UINT32)(dVal);
	    }
	    // /else something other than "h", "min", "s", "ms", or ""
	    // was specified:
	    else
	    {
		bSucceeded = FALSE;
	    }
	}

	if (bSucceeded)
	{
	    m_lSecond = ulTimeValue / 1000;
	    m_lMicroSecond = (ulTimeValue % 1000) * 1000;
	}
    }
    else
    {
	bSucceeded = FALSE;
    }
}

NPTime::NPTime(const NPTime& lhs)
{
    m_lSecond = lhs.m_lSecond;
    m_lMicroSecond = lhs.m_lMicroSecond;
}

NPTime&
NPTime::operator=(const NPTime& lhs)
{
    m_lSecond = lhs.m_lSecond;
    m_lMicroSecond = lhs.m_lMicroSecond;
    return *this;
}

void
NPTime::normalize()
{
    while(m_lMicroSecond < 0)
    {
	m_lSecond--;
	m_lMicroSecond += USECS_PER_SEC;
    }
    while(m_lMicroSecond >= USECS_PER_SEC)
    {
	m_lSecond++;
	m_lMicroSecond -= USECS_PER_SEC;
    }
}

INT32
NPTime::compare(const NPTime& lhs) const
{
    // I'm sure there's a better way to do this...

    if(m_lSecond > lhs.m_lSecond)
    {
	return 1;
    }
    else if(m_lSecond == lhs.m_lSecond)
    {
	return m_lMicroSecond - lhs.m_lMicroSecond;
    }
    return -1;
}

INT32
NPTime::toMSec()
{
    // no checking for overflow
    return (m_lSecond * 1000) + (m_lMicroSecond / 1000);
}

void
NPTime::fromMSec(INT32 mSec)
{
    m_lSecond = mSec / 1000;
    m_lMicroSecond = (mSec % 1000) * 1000;
    normalize();
}

const char*
NPTime::toString()
{
    char strBuf[80]; /* Flawfinder: ignore */

    if(m_lMicroSecond > 0)
    {
        HX_ASSERT(m_lMicroSecond < USECS_PER_SEC);
        SafeSprintf(strBuf, sizeof(strBuf), "%ld.%06d", m_lSecond, m_lMicroSecond);
    }
    else
    {
	SafeSprintf(strBuf, sizeof(strBuf), "%ld", m_lSecond);
    }
    m_asString = strBuf;
    return m_asString;
}

void
NPTime::fromString(const char* pTimeString)
{
    // format is [dd:[hh:[mm:]]]ssss[.uuuuu]

    double placeholder[4];
    char* pTmp = new char[strlen(pTimeString)+1];
    strcpy(pTmp, pTimeString); /* Flawfinder: ignore */

    char* pTok = strtok(pTmp, ":");
    int idx = 0;
    char* pChar;

    if (!pTok)
    {
	m_lSecond = 0;
	m_lMicroSecond = 0;
    }
    
    while (pTok && (idx < 4))
    {
	placeholder[idx++] = strtod(pTok, &pChar);
	pTok = strtok(NULL, ":");
    }

    if (pTok != NULL)
    {
	m_lSecond = 0;
	m_lMicroSecond = 0;
    }
    else if (idx == 1)
    {
	m_lSecond = (UINT32)placeholder[0];
	m_lMicroSecond = (UINT32)((placeholder[0] - (double)m_lSecond) *
	    USECS_PER_SEC);
    }
    else if (idx == 2)
    {
	m_lSecond = (UINT32)placeholder[1];
	m_lMicroSecond = (UINT32)((placeholder[1] - (double)m_lSecond) *
	    USECS_PER_SEC);
	m_lSecond += (UINT32)(placeholder[0] * 60.0);
    }
    else if (idx == 3)
    {
	m_lSecond = (UINT32)placeholder[2];
	m_lMicroSecond = (UINT32)((placeholder[2] - (double)m_lSecond) *
	    USECS_PER_SEC);
	m_lSecond += (UINT32)(placeholder[1] * 60.0);
	m_lSecond += (UINT32)(placeholder[0] * (60.0 * 60.0));
    }
    else if (idx == 4)
    {
	m_lSecond = (UINT32)placeholder[3];
	m_lMicroSecond = (UINT32)((placeholder[3] - (double)m_lSecond) *
	    USECS_PER_SEC);
	m_lSecond += (UINT32)(placeholder[2] * 60.0);
	m_lSecond += (UINT32)(placeholder[1] * (60.0 * 60.0));
	m_lSecond += (UINT32)(placeholder[0] * ((60.0 * 60.0) * 24.0));
    }

    delete[] pTmp;

    normalize();
}

NPTime::operator const char*()
{
    return toString();
}

NPTime::operator UINT32()
{
    return toMSec();
}

NPTime&
NPTime::operator+=(const NPTime& lhs)
{
    m_lSecond += lhs.m_lSecond;
    m_lMicroSecond += lhs.m_lSecond;
    normalize();
    return *this;
} 

NPTime&
NPTime::operator-=(const NPTime& lhs)
{
    m_lSecond -= lhs.m_lSecond;
    m_lMicroSecond -= lhs.m_lMicroSecond;
    normalize();
    return *this;
}

NPTime
NPTime::operator+(const NPTime& lhs)
{
    NPTime tmpTime = *this;
    tmpTime += lhs;
    return tmpTime;
}

NPTime
NPTime::operator-(const NPTime& lhs)
{
    NPTime tmpTime = *this;
    tmpTime -= lhs;
    return tmpTime;
}

