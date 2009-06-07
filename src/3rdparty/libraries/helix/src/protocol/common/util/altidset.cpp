/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altidset.cpp,v 1.2 2005/03/10 20:59:16 bobclark Exp $
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
#include "altidset.h"

CHXAltIDSet::CHXAltIDSet() :
    m_uCount(0),
    m_pAltIDs(NULL)
{}

CHXAltIDSet::~CHXAltIDSet()
{
    Clear();
}

HX_RESULT 
CHXAltIDSet::AddID(UINT32 uAltID)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    // Create a bigger buffer for the new AltID
    UINT32* pNew = new UINT32[m_uCount + 1];

    if (pNew)
    {
        HXBOOL bAlreadyInSet = FALSE;

        // Copy existing info and check to see if uAltID is
        // already in the set
        for (UINT32 i = 0; !bAlreadyInSet && (i < m_uCount); i++)
        {
            if (uAltID == m_pAltIDs[i])
            {
                bAlreadyInSet = TRUE;
            }
            else
            {
                pNew[i] = m_pAltIDs[i];
            }
        }

        if (!bAlreadyInSet)
        {
            // Add new info
            pNew[m_uCount] = uAltID;

            // destroy old info
            delete [] m_pAltIDs;
        
            // update member variables
            m_pAltIDs = pNew;
            m_uCount++;
        }
        else
        {
            // We didn't need to insert the
            // ID because it was already in
            // the set
            delete [] pNew;
            pNew = NULL;
        }

        res = HXR_OK;
    }

    return res;
}

HX_RESULT CHXAltIDSet::Copy(const CHXAltIDSet& rhs)
{
    HX_RESULT res = HXR_OK;

    if (&rhs != this)
    {
        Clear();
        m_uCount = rhs.m_uCount;

        if (m_uCount)
        {
            m_pAltIDs = new UINT32[m_uCount];

            if (m_pAltIDs)
            {
                for (UINT32 i = 0; i < rhs.m_uCount; i++)
                {
                    m_pAltIDs[i] = rhs.m_pAltIDs[i];
                }

                res = HXR_OK;
            }
            else
            {
                res = HXR_OUTOFMEMORY;
            }
        }
        else
        {
            res = HXR_OK;
        }
    }

    return res;
}

HX_RESULT 
CHXAltIDSet::Union(const CHXAltIDSet& rhs)
{
    HX_RESULT res = HXR_OK;
    UINT32 uCount = rhs.GetCount();

    // Check to see if rhs is not empty
    if (uCount)
    {
        // Get all the AltIDs from rhs and add them to this
        // set. Note that AddID() checks for AltIDs that are
        // already in the set
        for (UINT32 i = 0; (HXR_OK == res) && (i < uCount); i++)
        {
            UINT32 uAltID;

            res = rhs.GetAltID(i, uAltID);

            if (HXR_OK == res)
            {
                res = AddID(uAltID);
            }
        }
    }

    return res;
}

HX_RESULT 
CHXAltIDSet::Intersect(const CHXAltIDSet& rhs)
{
    HX_RESULT res = HXR_OK;
    UINT32 uCount = rhs.GetCount();

    if (uCount)
    {
        // Create a new set that contains the common
        // AltIDs
        CHXAltIDSet newSet;

        // Get all the AltIDs from rhs and see if they are
        // in this set. If so add the AltID to newSet
        for (UINT32 i = 0; (HXR_OK == res) && (i < uCount); i++)
        {
            UINT32 uAltID;

            res = rhs.GetAltID(i, uAltID);

            if ((HXR_OK == res) && HasAltID(uAltID))
            {
                res = newSet.AddID(uAltID);
            }
        }

        if (HXR_OK == res)
        {
            // Copy the new set into this object
            res = Copy(newSet);
        }
    }

    return res;
}

void CHXAltIDSet::Clear()
{
    m_uCount = 0;
    delete [] m_pAltIDs;
    m_pAltIDs = NULL;
}

UINT32 CHXAltIDSet::GetCount() const
{
    return m_uCount;
}

HX_RESULT 
CHXAltIDSet::GetAltID(UINT32 uIndex, UINT32& uAltID) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (uIndex < m_uCount)
    {
        uAltID = m_pAltIDs[uIndex];
        res = HXR_OK;
    }

    return res;
}

HXBOOL 
CHXAltIDSet::HasAltID(UINT32 uAltID) const
{
    HXBOOL bRet = FALSE;

    // Scan through the AltIDs in this set to
    // see if we can find a match
    for (UINT32 i = 0; !bRet && (i < m_uCount); i++)
    {
        if (uAltID == m_pAltIDs[i])
        {
            bRet = TRUE;
        }
    }
    
    return bRet;
}

HXBOOL 
CHXAltIDSet::IsSubset(const CHXAltIDSet& rhs)
{
    HXBOOL bRet = TRUE;

    if (rhs.GetCount())
    {
        // Iterate over all the AltIDs in this set
        for (UINT32 i = 0; bRet && (i < m_uCount); i++)
        {
            if (!rhs.HasAltID(m_pAltIDs[i]))
            {
                // We have found an AltID in this set
                // that is not in the rhs set. That 
                // means that this set isn't
                // a subset of rhs
                bRet = FALSE;
            }
        }
    }
    else if (m_uCount)
    {
        // rhs is an empty set
        // and this set AltIDs so this
        // object isn't a subset of
        // rhs
        bRet = FALSE;
    }

    return bRet;
}
