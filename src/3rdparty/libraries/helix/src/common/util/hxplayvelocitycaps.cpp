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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxplayvelocity.h"
#include "hxplayvelocitycaps.h"
#include "hxassert.h"

#define INITIAL_ARRAY_SIZE 4

CHXPlaybackVelocityCaps::CHXPlaybackVelocityCaps()
{
    m_lRefCount   = 0;
    m_ulNumRanges = 0;
    m_ulMaxRanges = INITIAL_ARRAY_SIZE;
    m_plRangeMin  = new INT32 [m_ulMaxRanges];
    m_plRangeMax  = new INT32 [m_ulMaxRanges];
}

CHXPlaybackVelocityCaps::~CHXPlaybackVelocityCaps()
{
    HX_VECTOR_DELETE(m_plRangeMin);
    HX_VECTOR_DELETE(m_plRangeMax);
}

STDMETHODIMP CHXPlaybackVelocityCaps::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown*) (IHXPlaybackVelocityCaps*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXPlaybackVelocityCaps))
        {
            *ppvObj = (IHXPlaybackVelocityCaps*) this;
            retVal  = HXR_OK;
        }
        if (SUCCEEDED(retVal))
        {
            AddRef();
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityCaps::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityCaps::Release()
{
    HX_ASSERT(m_lRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP CHXPlaybackVelocityCaps::GetRange(UINT32 i, REF(INT32) rlMin, REF(INT32) rlMax)
{
    HX_RESULT retVal = HXR_FAIL;

    if (i < m_ulNumRanges)
    {
        rlMin  = m_plRangeMin[i];
        rlMax  = m_plRangeMax[i];
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityCaps::AddRange(INT32 lMin, INT32 lMax)
{
    HX_RESULT retVal = HXR_FAIL;

    if (lMax >= lMin)
    {
        // Make sure we don't overlap any existing range
        UINT32 i = 0;
        for (i = 0; i < m_ulNumRanges; i++)
        {
            if (RangeOverlap(lMin, lMax, m_plRangeMin[i], m_plRangeMax[i]))
            {
                break;
            }
        }
        if (i == m_ulNumRanges)
        {
            // We don't have any overlap
            //
            // Clear the return value
            retVal = HXR_OK;
            // Check if we have room
            if (m_ulNumRanges + 1 > m_ulMaxRanges)
            {
                // Double the size of the array
                UINT32 ulNewSize = m_ulMaxRanges * 2;
                // Need to reallocate the arrays
                UINT32 ulOldSize = m_ulMaxRanges;
                retVal = SetArray(ulNewSize, ulOldSize, m_plRangeMin);
                if (SUCCEEDED(retVal))
                {
                    ulOldSize = m_ulMaxRanges;
                    retVal = SetArray(ulNewSize, ulOldSize, m_plRangeMax);
                    if (SUCCEEDED(retVal))
                    {
                        m_ulMaxRanges = ulNewSize;
                    }
                }
            }
            if (SUCCEEDED(retVal))
            {
                // Add the range
                m_plRangeMin[m_ulNumRanges] = lMin;
                m_plRangeMax[m_ulNumRanges] = lMax;
                m_ulNumRanges++;
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityCaps::CombineCapsLogicalAnd(IHXPlaybackVelocityCaps* pCaps)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pCaps)
    {
        // This is a brute-force algorithm, but since we 
        // expect the number of ranges to be low, then it's
        // appropriate to do here. If ever the number of ranges
        // we will use here becomes large, we may want to
        // choose a more efficient algorithm.
        //
        if (m_ulNumRanges > 0 && pCaps->GetNumRanges() > 0)
        {
            // Compute the maximum size for the intermediate arrays
            UINT32 ulMaxTmp = m_ulNumRanges * pCaps->GetNumRanges();
            // Allocate a min array
            INT32* pTmpMin = new INT32 [ulMaxTmp];
            if (pTmpMin)
            {
                INT32* pTmpMax = new INT32 [ulMaxTmp];
                if (pTmpMax)
                {
                    // Clear the return value
                    retVal = HXR_OK;
                    // Null out both arrays
                    memset((void*) pTmpMin, 0, sizeof(INT32) * ulMaxTmp);
                    memset((void*) pTmpMax, 0, sizeof(INT32) * ulMaxTmp);
                    // Initialize the output number of ranges
                    UINT32 ulNumTmp = 0;
                    // Run through each range and attempt to AND it
                    // with every other range in the other caps
                    UINT32 i = 0;
                    UINT32 j = 0;
                    for (i = 0; i < m_ulNumRanges; i++)
                    {
                        for (j = 0; j < pCaps->GetNumRanges() && SUCCEEDED(retVal); j++)
                        {
                            INT32 lRangeMin = 0;
                            INT32 lRangeMax = 0;
                            retVal = pCaps->GetRange(j, lRangeMin, lRangeMax);
                            if (SUCCEEDED(retVal))
                            {
                                INT32 lOutputMin = 0;
                                INT32 lOutputMax = 0;
                                if (CombineRange(m_plRangeMin[i], m_plRangeMax[i],
                                                 lRangeMin,       lRangeMax,
                                                 lOutputMin,      lOutputMax))
                                {
                                    pTmpMin[ulNumTmp] = lOutputMin;
                                    pTmpMax[ulNumTmp] = lOutputMax;
                                    ulNumTmp++;
                                }
                            }
                        }
                    }
                    if (SUCCEEDED(retVal))
                    {
                        // Out with the old, in with the new
                        HX_VECTOR_DELETE(m_plRangeMin);
                        HX_VECTOR_DELETE(m_plRangeMax);
                        m_plRangeMin  = pTmpMin;
                        m_plRangeMax  = pTmpMax;
                        m_ulNumRanges = ulNumTmp;
                        m_ulMaxRanges = ulMaxTmp;
                    }
                }
                else
                {
                    retVal = HXR_OUTOFMEMORY;
                }
            }
            else
            {
                retVal = HXR_OUTOFMEMORY;
            }
        }
        else
        {
            // One of the caps doesn't have any ranges,
            // so an AND with that one will always be NULL
            m_ulNumRanges = 0;
        }
    }

    return retVal;
}

STDMETHODIMP_(HXBOOL) CHXPlaybackVelocityCaps::IsCapable(INT32 lVelocity)
{
    HXBOOL bRet = FALSE;

    // Check against the velocity ranges
    for (UINT32 i = 0; i < m_ulNumRanges; i++)
    {
        if (lVelocity >= m_plRangeMin[i] &&
            lVelocity <= m_plRangeMax[i])
        {
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}

HX_RESULT CHXPlaybackVelocityCaps::Create1xOnlyCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    return CreateCaps(HX_PLAYBACK_VELOCITY_NORMAL, HX_PLAYBACK_VELOCITY_NORMAL, rpCaps);
}

HX_RESULT CHXPlaybackVelocityCaps::CreateFullRangeCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    return CreateCaps(HX_PLAYBACK_VELOCITY_MIN, HX_PLAYBACK_VELOCITY_MAX, rpCaps);
}

HX_RESULT CHXPlaybackVelocityCaps::CreatePositiveRangeCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    return CreateCaps(HX_PLAYBACK_VELOCITY_NORMAL, HX_PLAYBACK_VELOCITY_MAX, rpCaps);
}

HX_RESULT CHXPlaybackVelocityCaps::SetArray(UINT32 ulNum, REF(UINT32) rulNum, REF(INT32*) rplArray)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ulNum)
    {
        INT32* plArray = new INT32 [ulNum];
        if (plArray)
        {
            // NULL out the array
            memset((void*) plArray, 0, ulNum * sizeof(INT32));
            // Copy the old velocities (if there are any)
            if (rulNum && rplArray)
            {
                UINT32 ulMin = HX_MIN(rulNum, ulNum);
                memcpy(plArray, rplArray, ulMin * sizeof(INT32));
            }
            // Assign to the member variables
            HX_VECTOR_DELETE(rplArray);
            rplArray = plArray;
            rulNum   = ulNum;
        }
    }

    return retVal;
}

HXBOOL CHXPlaybackVelocityCaps::CombineRange(INT32 lMin1, INT32 lMax1, INT32 lMin2, INT32 lMax2,
                                           REF(INT32) rlMinOut, REF(INT32) rlMaxOut)
{
    HXBOOL bRet = FALSE;

    if (lMax1 >= lMin1 &&
        lMax2 >= lMin2)
    {
        if (lMin1 <= lMax2 && lMin2 <= lMax1)
        {
            rlMinOut = HX_MAX(lMin1, lMin2);
            rlMaxOut = HX_MIN(lMax1, lMax2);
            bRet     = TRUE;
        }
    }

    return bRet;
}

HXBOOL CHXPlaybackVelocityCaps::RangeOverlap(INT32 lMin1, INT32 lMax1, INT32 lMin2, INT32 lMax2)
{
    HXBOOL bRet = FALSE;

    if (lMax1 >= lMin1 &&
        lMax2 >= lMin2)
    {
        if (lMin1 <= lMax2 && lMin2 <= lMax1)
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

HX_RESULT CHXPlaybackVelocityCaps::CreateCaps(INT32 lMin, INT32 lMax, REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    HX_RESULT retVal = HXR_FAIL;

    CHXPlaybackVelocityCaps* pCaps = new CHXPlaybackVelocityCaps();
    if (pCaps)
    {
        // Add entire range
        retVal = pCaps->AddRange(lMin, lMax);
        if (SUCCEEDED(retVal))
        {
            // Assign the out parameter
            HX_RELEASE(rpCaps);
            retVal = pCaps->QueryInterface(IID_IHXPlaybackVelocityCaps, (void**) &rpCaps);
        }
    }
    if (FAILED(retVal))
    {
        HX_DELETE(pCaps);
    }

    return retVal;
}

