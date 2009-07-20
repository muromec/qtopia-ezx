/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef CTRANGE_H
#define CTRANGE_H

#define NO_TIME_SET 0xFFFFFFFF

class CTimeRange
{
public:
    CTimeRange();

    HXBOOL   TimeInRange(UINT32 ulTime);
    HXBOOL   IsEmpty();

    UINT32 m_ulStart;
    UINT32 m_ulEnd;
};

inline CTimeRange::CTimeRange()
{
    m_ulStart = m_ulEnd = NO_TIME_SET;
}

inline HXBOOL CTimeRange::TimeInRange(UINT32 ulTime)
{
    if (m_ulStart == NO_TIME_SET)
    {
        return FALSE;
    }

    // check for roll over
    if (m_ulEnd >= m_ulStart)
    {
        return (ulTime >= m_ulStart && ulTime <= m_ulEnd);
    }
    else
    {
        return (ulTime >= m_ulStart || ulTime <= m_ulEnd);
    }
}

inline HXBOOL CTimeRange::IsEmpty()
{
    return NO_TIME_SET == m_ulStart;
}

#endif /* #ifndef CTRANGE_H */
