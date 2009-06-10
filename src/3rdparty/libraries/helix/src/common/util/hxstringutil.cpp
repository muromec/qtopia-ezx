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

#include "hxdir.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxstringutil.h"


//
// Behaves like python string.split()
//
// Splits 'str' into components (stored in 'list') delimited by 'sep'
//
// Split("/foo/", '/') -> ("", "foo", "")
void HXStringUtil::Split(const CHXString& str, const CHXString& sep, CHXStringList& list /*out*/)
{
    
    HX_ASSERT(!sep.IsEmpty());

    UINT32 cchSep = sep.GetLength();

    const char* pBegin = str;
    const char* pEnd = pBegin + str.GetLength();

    bool endsOnSep = false;

    while(pBegin < pEnd)
    {
        // find termination for current token (starts at pBegin)
        const char* pTerm = strstr(pBegin, sep);

        // find start of next token
        const char* pNextBegin;
        if(pTerm)
        {
            // starts just past delimiter
            pNextBegin = pTerm + cchSep;
            if(pNextBegin == pEnd)
            {
                endsOnSep = true;
            }
        }
        else
        {
            // delimiter is EOS
            pTerm = pEnd;

            // no more tokens after this
            pNextBegin = pEnd;
        }

        // add token
        INT32 cch = pTerm - pBegin;
        HX_ASSERT(cch >= 0);
        CHXString item(pBegin, cch);
        list.AddTailString(item);

        // advance to next token
        pBegin = pNextBegin;
    }

    if(endsOnSep)
    {
        list.AddTailString("");
    }

}
    

//
// Behaves like python string.join()
//
// Join strings from 'list' into one string, with items separated by 'sep'
//
// e.g., ["boo", "bam", "", "bop], "/" -> boo/bam//bop
//
//
CHXString HXStringUtil::Join(const CHXStringList& list, const CHXString& sep)
{
    CHXString joined;

    bool bFirst = true;

    // cast away const (no const_iterator)
    CHXStringList& list_ = (CHXStringList&)list;

    CHXSimpleList::Iterator begin = list_.Begin();
    CHXSimpleList::Iterator end = list_.End();
    for(;begin != end; ++begin)
    {
        CHXString* pElement = (CHXString*)*begin;
        HX_ASSERT(pElement);
        if(pElement)
        {
            if( !bFirst )
            {
                joined += sep;
            }
            else
            {
                bFirst = false;
            }

            joined += *pElement;
        }  
    }

    return joined;
}


