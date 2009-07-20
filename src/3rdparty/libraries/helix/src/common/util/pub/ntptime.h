/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ntptime.h,v 1.7 2009/01/13 18:13:40 jgordon Exp $
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

#ifndef _NTPTIME_H_
#define _NTPTIME_H_

#include "timeval.h"

/*
 * Number of seconds between 1-Jan-1900 and 1-Jan-1970
 */
#define GETTIMEOFDAY_TO_NTP_OFFSET 2208988800UL

#if !defined(MAX_UINT32)
#define MAX_UINT32		    0xFFFFFFFF
#endif

class NTPTime
{
public:

    // constructors
    NTPTime();
    NTPTime(UINT32 ulSec, UINT32 ulFraction);
    NTPTime(UINT32 mSec);
    NTPTime(Timeval tv);
    NTPTime(const NTPTime& lhs);

    // assignment
    NTPTime& operator=(const NTPTime& lhs);

    // conversion to ms
    operator UINT32();

    // arithmetic
    NTPTime operator+(const NTPTime& lhs);
    NTPTime operator-(const NTPTime& lhs);
    NTPTime& operator+=(const NTPTime& lhs);
    NTPTime& operator-=(const NTPTime& lhs);

    // comparison
    INT32 compare(const NTPTime& lhs) const;
    UINT32 toMSec();
    Timeval toTimeval();

    UINT32 m_ulSecond;
    UINT32 m_ulFraction;

private:
    UINT32 usec2ntp(UINT32 uSec);
    void fromMSec(INT32 mSec);
    void fromTimeval(Timeval tv);
};

/*
 * convert microseconds to fraction of second * 2^32 (i.e., the lsw of
 * a 64-bit ntp timestamp).  This routine uses the factorization
 * 2^32/10^6 = 4096 + 256 - 1825/32 which results in a max conversion
 * error of 3 * 10^-7 and an average error of half that.
 */
inline UINT32 NTPTime::usec2ntp(UINT32 uSec)
{
	UINT32 t = (uSec * 1825) >> 5;
	return ((uSec << 12) + (uSec << 8) - t);
}

inline void NTPTime::fromTimeval(Timeval tv)
{
    m_ulSecond = (UINT32)tv.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
    m_ulFraction = usec2ntp((UINT32)tv.tv_usec);
}

inline Timeval NTPTime::toTimeval()
{
    Timeval tv  ((long)m_ulSecond,
                (long)(((double)m_ulFraction * 1000000.0) / (double)MAX_UINT32));
    return tv;
}

inline HXBOOL operator==(const NTPTime& t1, const NTPTime& t2)
{
    return t1.compare(t2) == 0;
}

inline HXBOOL operator!=(const NTPTime& t1, const NTPTime& t2)
{
    return t1.compare(t2) != 0;
}

inline HXBOOL operator<(const NTPTime& t1, const NTPTime& t2)
{
    return t1.compare(t2) < 0;
}

inline HXBOOL operator>(const NTPTime& t1, const NTPTime& t2)
{
    return t1.compare(t2) > 0;
}

inline HXBOOL operator<=(const NTPTime& t1, const NTPTime& t2)
{
    return t1.compare(t2) <= 0;
}

inline HXBOOL operator>=(const NTPTime& t1, const NTPTime& t2)
{
    return t1.compare(t2) >= 0;
}

#endif /* _NTPTIME_H_ */

