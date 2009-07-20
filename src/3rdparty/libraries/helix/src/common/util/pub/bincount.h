/* ***** BEGIN LICENSE BLOCK *****  
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

#ifndef BINCOUNT_H
#define BINCOUNT_H

// This class records a history of binary events, and
// can count the number of TRUE events in the last
// 32 events.
//
// This class can be improved in the following ways
//
// a) Make the size of sliding event window not fixed
//    at a size of 32;
// b) handle the default being set or not set
// c) make the LUT static const so that we only have
//    one copy for all instances of the class
// d) keep a running count of bits by examining the 
//    bits sliding in and out of the window.

class CHXBinaryHistoryCounter : public CHXBaseCountingObject
{
public:
    CHXBinaryHistoryCounter();
    virtual ~CHXBinaryHistoryCounter() {};

    UINT32 GetHistoryWindowSize() const { return 32; }
    void   SetNext(HXBOOL bNext);
    UINT32 GetNumSetInWindow();
    void   Reset();
protected:
    UINT32 m_ulWindowBuffer;
    BYTE   m_pucCountingLUT[256];
};

inline void CHXBinaryHistoryCounter::SetNext(HXBOOL bNext)
{
    // Shift the window left by 1 bit. We assume
    // that 0's are shifted in from the right.
    m_ulWindowBuffer <<= 1;
    // If the next event is TRUE, then set the
    // rightmost bit.
    if (bNext)
    {
        m_ulWindowBuffer |= 1;
    }
}

inline UINT32 CHXBinaryHistoryCounter::GetNumSetInWindow()
{
    UINT32 ulRet = 0;

    BYTE* pucWindow = (BYTE*) &m_ulWindowBuffer;
    for (UINT32 i = 0; i < 4; i++)
    {
        ulRet += m_pucCountingLUT[pucWindow[i]];
    }

    return ulRet;
}


#endif /* #ifndef BINCOUNT_H */

