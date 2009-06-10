/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: langaltfilt.cpp,v 1.5 2007/07/06 20:51:32 jfinnecy Exp $
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
#include "langaltfilt.h"

#include "altidset.h"
#include "altgrpitr.h"
#include "hxstring.h"

const char LanguageDelim = ',';

CHXLanguageAltIDFilter::CHXLanguageAltIDFilter()
{}

CHXLanguageAltIDFilter::~CHXLanguageAltIDFilter()
{}

HX_RESULT 
CHXLanguageAltIDFilter::Init(const char* pLanguages)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pLanguages)
    {
        const char* pCur = pLanguages;

        m_langList.RemoveAll();

        if (isAlphaOrAsterisk(*pCur))
        {
            res = HXR_OK;
            while((HXR_OK == res) && isAlphaOrAsterisk(*pCur))
            {
                res = parseLangTag(pCur);

                if (HXR_OK == res)
                {
                    if (*pCur == LanguageDelim)
                    {
                        pCur++; // skip LanguageDelim

                        // Skip any whitespace
                        while (*pCur && isWhiteSpace(*pCur))
                        {
                            pCur++;
                        }

                        if (!isAlphaOrAsterisk(*pCur))
                        {
                            // We happened upon an unexpected
                            // character
                            res = HXR_INVALID_PARAMETER;
                        }
                    }
                }
            }

            if ((HXR_OK == res) && *pCur)
            {
                res = HXR_INVALID_PARAMETER;
            }
        }
        else if (!*pCur)
        {
            // An empty string means that there
            // aren't any language constraints.
            res = HXR_OK;
        }
    }

    return res;
}

HX_RESULT 
CHXLanguageAltIDFilter::Filter(UINT32 uHdrCount, IHXValues** pInHdrs,
                               const CHXAltIDMap& altIDMap,
                               CHXAltIDSet& /* in/out */ altIDSet)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if ((uHdrCount > 1) && pInHdrs)
    {
        // Assume that we are going to succeed.
        // This function won't modify the set
        // if there aren't any language constraints
        // or if language information alt info is 
        // not present
        res = HXR_OK;

        // See if we have any language constraints.
        if (m_langList.GetCount())
        {
            CHXAltIDSet newIDSet;

            CHXAltGroupIterator itr;

            res = itr.Init(pInHdrs[0], "LANG:RFC3066");

            if (HXR_OK == res)
            {
                CHXAltIDSet newIDSet;

                // Iterate over all the group values
                for(;(HXR_OK == res) && itr.More(); itr.Next())
                {
                    CHXString lang;
                    lang = itr.GroupValue();

                    // See if the language for this group
                    // matches one of the language constraints
                    if (matchesALanguage(lang))
                    {
                        CHXAltIDSet langSet;
                        res = itr.GetAltIDSet(langSet);

                        if (HXR_OK == res)
                        {
                            // Take the intersection of the
                            // current set and the language set
                            res = langSet.Intersect(altIDSet);

                            if (HXR_OK == res)
                            {
                                // Merge the intersection into
                                // our new set
                                res = newIDSet.Union(langSet);
                            }
                        }
                    }
                }

                if (HXR_OK == res)
                {
                    // Copy the new set into the
                    // out parameter
                    res = altIDSet.Copy(newIDSet);
                }
            }
            else if (HXR_INVALID_PARAMETER == res)
            {
                // This is likely because there isn't any language
                // specific alt-group info. Just return the altIDSet
                // unmodified
                res = HXR_OK;
            }
        }
    }

    return res;
}

HXBOOL 
CHXLanguageAltIDFilter::isAlpha(char ch) const
{
    return ((('a' <= ch) && (ch <= 'z')) ||
            (('A' <= ch) && (ch <= 'Z')));
}

HXBOOL 
CHXLanguageAltIDFilter::isAlphaDigit(char ch) const
{
    return (isAlpha(ch) ||
            (('0' <= ch) && (ch <= '9')));
}

HXBOOL
CHXLanguageAltIDFilter::isWhiteSpace(char ch) const
{
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

HXBOOL
CHXLanguageAltIDFilter::isAsterisk(char ch) const
{
    return (ch == '*');
}

HXBOOL
CHXLanguageAltIDFilter::isAlphaOrAsterisk(char ch) const
{
    return (isAlpha(ch) || isAsterisk(ch));
}

HX_RESULT 
CHXLanguageAltIDFilter::parseLangTag(const char*& pCur)
{
    // This function parses RFC3066 language tags and
    // adds them to m_langList

    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pCur && isAlphaOrAsterisk(*pCur))
    {
        const char* pStart = pCur;
                
        // Collect the primary-subtag
        for(;isAlphaOrAsterisk(*pCur); pCur++)
            ;
    
        // Verify that we have a valid primary-subtag length
        if ((pCur - pStart) <= 8)
        {
            res = HXR_OK;

            if (*pCur)
            {
                if (*pCur == '-')
                {
                    // There are subtags
                    pCur++; // skip '-'
                    const char* pSubTagStart = pCur;
                    
                    if (isAlphaDigit(*pCur))
                    {
                        // Collect the subtags
                        for(;((HXR_OK == res) && *pCur && 
                              (*pCur != LanguageDelim)); pCur++)
                        {
                            if (!isAlphaDigit(*pCur))
                            {
                                if (*pCur == '-')
                                {
                                    // Verify that the subtag's length is
                                    // valid
                                    if ((pCur - pSubTagStart) <= 8)
                                    {
                                        pCur++; // skip '-'
                                        pSubTagStart = pCur;
                                        if (!isAlphaDigit(*pCur))
                                        {
                                            res = HXR_INVALID_PARAMETER;
                                        }
                                    }
                                    else
                                    {
                                        res = HXR_INVALID_PARAMETER;
                                    }
                                }
                                else 
                                {
                                    res = HXR_INVALID_PARAMETER;
                                }
                            }
                        }
                    }
                    else
                    {
                        res = HXR_INVALID_PARAMETER;
                    }
                }
                else if (*pCur != LanguageDelim)
                {
                    res = HXR_INVALID_PARAMETER;
                }
            }
            
            if ((HXR_OK == res) &&
                (!m_langList.AddTailString(CHXString(pStart,
                                                     pCur - pStart))))
            {
                res = HXR_OUTOFMEMORY;
            }
        }
    }

    return res;
}

HXBOOL 
CHXLanguageAltIDFilter::matchesALanguage(const CHXString& lang) const
{
    HXBOOL bRet = FALSE;

    if (lang.GetLength())
    {
        LISTPOSITION pos = m_langList.GetHeadPosition();

        while(!bRet && pos)
        {
            CHXString* pStr = m_langList.GetNext(pos);

            if (pStr)
            {
                if (!strcasecmp(*pStr, lang))
                {
                    // The whole string matches
                    bRet = TRUE;
                }
                else
                {
                    // Test to see if *pStr plus a '-' is
                    // a prefix of lang. This handles the
                    // case where *pStr is 'en' and lang
                    // is 'en-US'
                    CHXString subtype = *pStr + '-';
                    if (!strcasecmp(lang.Left((INT32)subtype.GetLength()),
                                    subtype))
                    {
                        bRet = TRUE;
                    }
                }   
            }
        }
    }

    return bRet;
}
