/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmovmed.cpp,v 1.2 2005/05/26 17:06:31 ping Exp $
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
#include "hxmovmed.h"
#include "hlxclib/memory.h"
#include "hlxclib/stdlib.h"

HXMovingMedian::HXMovingMedian() :
    m_pSamples(NULL),
    m_pSortBuf(NULL),
    m_uSize(0),
    m_uIndex(0),
    m_bFull(FALSE),
    m_uMedian(0),
    m_bCached(FALSE)
{}

HXMovingMedian::~HXMovingMedian()
{
    delete [] m_pSamples;
    m_pSamples = NULL;

    delete [] m_pSortBuf;
    m_pSortBuf = NULL;
}

HXBOOL HXMovingMedian::Init(UINT32 uSize)
{
    HXBOOL bRet = FALSE;

    if (uSize)
    {
        delete [] m_pSamples;
        m_pSamples = new UINT32[uSize];

        delete [] m_pSortBuf;
        m_pSortBuf = new UINT32[uSize];

        if (m_pSamples && m_pSortBuf)
        {
            m_uSize = uSize;

            Reset();

            bRet = TRUE;
        }
        else
        {
            delete [] m_pSamples;
            m_pSamples = NULL;

            delete [] m_pSortBuf;
            m_pSortBuf = NULL;
        }
    }

    return bRet;
}

HXBOOL HXMovingMedian::AddSample(UINT32 uSample)
{
    HXBOOL bRet = FALSE;

    if (m_pSamples)
    {
        m_pSamples[m_uIndex++] = uSample;
        m_bCached = FALSE; // The cached value is now invalid

        if (m_uIndex == m_uSize)
        {
            m_uIndex = 0;
            m_bFull = TRUE;
        }

        bRet = TRUE;
    }

    return bRet;
}

UINT32 HXMovingMedian::Median()
{
    UINT32 uRet = 0;

    if (m_bCached)
    {
        uRet = m_uMedian;
    }
    else if (m_pSamples && m_pSortBuf)
    {
        UINT32 uCount = m_uSize;

        if (!m_bFull)
        {
            uCount = m_uIndex;
        }
        
        if (uCount)
        {
            // Copy samples into sort buffer
            for (UINT32 i = 0; i < uCount; i++)
            {
                m_pSortBuf[i] = m_pSamples[i];
            }
            
            // Sort samples
            qsort(m_pSortBuf, uCount, sizeof(UINT32), 
                  &HXMovingMedian::compare);
            
            UINT32 uMiddle = uCount / 2;

            if (uCount & 0x1)
            {
                // uCount is odd
                uRet = m_pSortBuf[uMiddle];
            }
            else
            {
                // uCount is even
                uRet = (m_pSortBuf[uMiddle - 1] + m_pSortBuf[uMiddle]) / 2;
            }
            
            m_uMedian = uRet; 
            m_bCached = TRUE;
        }
    }

    return uRet;
}

void HXMovingMedian::Reset()
{
    if (m_pSamples)
    {
        memset(m_pSamples, 0, m_uSize * sizeof(UINT32));
    }
    
    m_uIndex = 0;
    m_bFull = FALSE;
    m_uMedian = 0;
    m_bCached = FALSE;
}

int HXMovingMedian::compare(const void* pA, const void* pB)
{
    UINT32* puA = (UINT32*)pA;
    UINT32* puB = (UINT32*)pB;
    int ret = 0;

    if (*puA > *puB)
    {
        ret = 1;
    }
    else if (*puA < *puB)
    {
        ret = -1;
    }

    return ret;
}
