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

#ifndef HX_STRINGUTIL_H__
#define HX_STRINGUTIL_H__

#include "hxstring.h"

class CHXStringList;

struct HXStringUtil
{
    static CHXString Join(const CHXStringList& list, const CHXString& sep);
    static void Split(const CHXString& str, const CHXString& sep, CHXStringList& list /*out*/);

    static UINT32 FindFirstMisMatch(const char* pch, char ch);
    static void Replace(CHXString& str, char chOld, char chNew);
};

// returns count of successive chars matching ch
inline
UINT32 HXStringUtil::FindFirstMisMatch(const char* pch, char ch)
{
    HX_ASSERT(pch);
    HX_ASSERT(ch!= '\0');

    UINT32 count = 0;
    while (*pch++ == ch)
    {
        ++count;
    }
    return count;
}

// replace 'chOld' with 'chNew'
inline
void HXStringUtil::Replace(CHXString& str, char chOld, char chNew)
{
    UINT32 cch = str.GetLength();
    for (UINT32 idx = 0; idx < cch; ++idx)
    {
        if (str[idx] == chOld)
        {
            // replace
            str.SetAt(idx, chNew);
        }
    }
}

#endif /* HX_STRINGUTIL_H__ */
