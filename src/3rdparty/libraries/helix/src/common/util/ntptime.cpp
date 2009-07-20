/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ntptime.cpp,v 1.5 2004/07/09 18:23:51 hubbe Exp $
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

// #include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hxtypes.h"
#include "hxstring.h"
#include "hxtime.h"
#include "timeval.h"
#include "ntptime.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

NTPTime::NTPTime()
{
    // default to now
    Timeval now;
    gettimeofday(&now, 0);
    fromTimeval(now);
}

NTPTime::NTPTime(UINT32 ulSec, UINT32 ulFrac):
    m_ulSecond(ulSec),
    m_ulFraction(ulFrac)
{
}

NTPTime::NTPTime(UINT32 mSec)
{
    fromMSec(mSec);
}

NTPTime::NTPTime(Timeval tv)
{
    fromTimeval(tv);
}

NTPTime::NTPTime(const NTPTime& lhs)
{
    m_ulSecond = lhs.m_ulSecond;
    m_ulFraction = lhs.m_ulFraction;
}

NTPTime&
NTPTime::operator=(const NTPTime& lhs)
{
    m_ulSecond = lhs.m_ulSecond;
    m_ulFraction = lhs.m_ulFraction;
    return *this;
}

INT32
NTPTime::compare(const NTPTime& lhs) const
{
    // I'm sure there's a better way to do this...

    if(m_ulSecond > lhs.m_ulSecond)
    {
	return 1;
    }
    else if(m_ulSecond == lhs.m_ulSecond)
    {
	return (INT32) ((m_ulFraction >> 1) - (lhs.m_ulFraction >> 1));
    }
    return -1;
}

UINT32
NTPTime::toMSec()
{
    // no checking for overflow
    double ms = (m_ulSecond * 1000.0);
    ms += ((double) m_ulFraction / (double) MAX_UINT32) * 1000.0;

    return (UINT32) ms;
}

void
NTPTime::fromMSec(INT32 mSec)
{
    Timeval tv;

    tv.tv_sec = mSec / 1000;
    tv.tv_usec = (mSec % 1000) * 1000;
    fromTimeval(tv);
}

NTPTime::operator UINT32()
{
    return toMSec();
}

NTPTime&
NTPTime::operator+=(const NTPTime& lhs)
{
    m_ulSecond += lhs.m_ulSecond;

    if ((MAX_UINT32 - m_ulFraction + 1) <= lhs.m_ulFraction)
    {
	// No overflow checking
	m_ulSecond++;
	m_ulFraction = lhs.m_ulFraction - (MAX_UINT32 - m_ulFraction + 1);
    }
    else
    {
	m_ulFraction += lhs.m_ulFraction;
    }
    
    return *this;
} 

NTPTime&
NTPTime::operator-=(const NTPTime& lhs)
{
    m_ulSecond -= lhs.m_ulSecond;

    if (m_ulFraction < lhs.m_ulFraction)
    {
	// No underflow checking
	m_ulSecond--;
    }

    m_ulFraction = m_ulFraction - lhs.m_ulFraction;
    
    return *this;
}

NTPTime
NTPTime::operator+(const NTPTime& lhs)
{
    NTPTime tmpTime(*this);
    tmpTime += lhs;
    return tmpTime;
}

NTPTime
NTPTime::operator-(const NTPTime& lhs)
{
    NTPTime tmpTime(*this);
    tmpTime -= lhs;
    return tmpTime;
}

