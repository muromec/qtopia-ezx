/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stringtostringlistmap.cpp,v 1.1 2007/01/11 21:45:04 ehyche Exp $
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

#include "stringtostringlistmap.h"

CHXStringToStringListMap::CHXStringToStringListMap(HXBOOL bAllowDuplicates, HXBOOL bCaseSensitive)
    : m_bAllowDuplicates(bAllowDuplicates)
    , m_bCaseSensitive(bCaseSensitive)
    , m_pMap(NULL)
{
}

CHXStringToStringListMap::~CHXStringToStringListMap()
{
    RemoveAll();
    HX_DELETE(m_pMap);
}

int CHXStringToStringListMap::GetCount() const
{
    return (m_pMap ? m_pMap->GetCount() : 0);
}

int CHXStringToStringListMap::IsKeyPresent(const char* pszKey) const
{
    return (GetStringCount(pszKey) > 0 ? TRUE : FALSE);
}

int CHXStringToStringListMap::GetStringCount(const char* pszKey) const
{
    int iRet = 0;
    
    void* pValue = NULL;
    if (m_pMap && m_pMap->Lookup(pszKey, pValue))
    {
        CHXStringList* pList = (CHXStringList*) pValue;
        iRet = (pList ? pList->GetCount() : 0);
    }

    return iRet;
}

HXBOOL CHXStringToStringListMap::IsEmpty() const
{
    return (m_pMap ? m_pMap->IsEmpty() : TRUE);
}

HXBOOL CHXStringToStringListMap::IsEmpty(const char* pszKey) const
{
    return (GetStringCount(pszKey) == 0 ? TRUE : FALSE);
}

HX_RESULT CHXStringToStringListMap::AddStringToList(const char* pszKey, const char* pszStr)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pszKey && pszStr)
    {
        // Set the return value
        retVal = HXR_OUTOFMEMORY;
        // Do we have a map yet?
        if (!m_pMap)
        {
            // Create the map
            m_pMap = new CHXMapStringToOb();
        }
        if (m_pMap)
        {
            // Do we have a string list for this key yet?
            CHXStringList* pList = NULL;
            void*          pObj  = NULL;
            HXBOOL         bList = m_pMap->Lookup(pszKey, pObj);
            if (!bList)
            {
                // We don't have a list yet, so create one
                pList = new CHXStringList();
                if (pList)
                {
                    // Add it to the map
                    m_pMap->SetAt(pszKey, (void*) pList);
                }
            }
            else
            {
                pList = (CHXStringList*) pObj;
            }
            if (pList)
            {
                // Clear the return value
                retVal = HXR_OK;
                // Do we allow duplicates?
                HXBOOL bAddString = TRUE;
                if (!m_bAllowDuplicates)
                {
                    // We don't allow duplicates so we have to 
                    // check and see if this string is already in
                    // the list.
                    LISTPOSITION pos = pList->FindString(pszStr, NULL, m_bCaseSensitive);
                    if (pos)
                    {
                        // The string is already in the list, so don't add it
                        bAddString = FALSE;
                    }
                }
                if (bAddString)
                {
                    // Add the string to the list
                    pList->AddTailString(pszStr);
                }
            }
        }
    }

    return retVal;
}

HX_RESULT CHXStringToStringListMap::GetFirstString(const char* pszKey, CHXString* pStr, LISTPOSITION& rPos)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pszKey && pStr)
    {
        // Set the return value
        retVal = HXR_FAIL;
        // Look up the CHXStringList
        void* pValue = NULL;
        if (m_pMap && m_pMap->Lookup(pszKey, pValue))
        {
            CHXStringList* pList = (CHXStringList*) pValue;
            if (pList && pList->GetCount() > 0)
            {
                // Initialize the list position
                rPos = pList->GetHeadPosition();
                if (rPos)
                {
                    // Get the first CHXString
                    CHXString* pListStr = pList->GetNext(rPos);
                    if (pListStr)
                    {
                        // Copy the CHXString
                        *pStr = *pListStr;
                        // Clear the return value
                        retVal = HXR_OK;
                    }
                }
            }
        }
    }

    return retVal;
}

HX_RESULT CHXStringToStringListMap::GetNextString(const char* pszKey, CHXString* pStr, LISTPOSITION& rPos)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pszKey && pStr)
    {
        // Set the return value
        retVal = HXR_FAIL;
        // Look up the CHXStringList
        void* pValue = NULL;
        if (m_pMap && m_pMap->Lookup(pszKey, pValue))
        {
            CHXStringList* pList = (CHXStringList*) pValue;
            if (pList && pList->GetCount() > 0 && rPos)
            {
                // Get the next CHXString
                CHXString* pListStr = pList->GetNext(rPos);
                if (pListStr)
                {
                    // Copy the CHXString
                    *pStr = *pListStr;
                    // Clear the return value
                    retVal = HXR_OK;
                }
            }
        }
    }

    return retVal;
}

void CHXStringToStringListMap::RemoveAll()
{
    if (m_pMap)
    {
        // Delete all the string lists
        CHXMapStringToOb::Iterator itr;
        for (itr = m_pMap->Begin(); itr != m_pMap->End(); itr++)
        {
            CHXStringList* pList = (CHXStringList*) *itr;
            HX_DELETE(pList);
        }
        // Remove all the elements
        m_pMap->RemoveAll();
    }
}
