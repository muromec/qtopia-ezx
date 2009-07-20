/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altgrpitr.cpp,v 1.3 2005/03/10 20:59:16 bobclark Exp $
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
#include "hxval2util.h"
#include "netbyte.h"
#include "altidset.h"
#include "altgrpitr.h"

CHXAltGroupIterator::CHXAltGroupIterator() :
    m_pGroupInfo(NULL),
    m_pItr(NULL)
{}

CHXAltGroupIterator::~CHXAltGroupIterator()
{
    HX_RELEASE(m_pGroupInfo);
    HX_RELEASE(m_pItr);
}

HX_RESULT
CHXAltGroupIterator::Init(IHXValues* pFileHdr, const char* pGroupType)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pFileHdr && pGroupType)
    {
        HX_RELEASE(m_pGroupInfo);
        HX_RELEASE(m_pItr);

        IHXValues2* pAltGroups = NULL;

        if ((HXR_OK == CHXIHXValues2Util::GetIHXValues2Property(pFileHdr,
                                                                "Alt-Group",
                                                                pAltGroups)) &&
            (HXR_OK == CHXIHXValues2Util::GetIHXValues2Property(pAltGroups,
                                                                pGroupType,
                                                                m_pGroupInfo)))
        {
            const char* pKey = NULL;
            IUnknown* pUnk = NULL;

            res = m_pGroupInfo->GetFirstPropertyObject(pKey, pUnk);

            if (HXR_OK == res)
            {
                m_groupValue = pKey;

                res = getIterator(pUnk);
            }
            HX_RELEASE(pUnk);
        }

        if (HXR_OK != res)
        {
            m_groupValue = "";
            HX_RELEASE(m_pGroupInfo);
            HX_RELEASE(m_pItr);
        }

        HX_RELEASE(pAltGroups);
    }

    return res;
}

const CHXString&
CHXAltGroupIterator::GroupValue() const
{
    return m_groupValue;
}

HX_RESULT
CHXAltGroupIterator::GetAltIDSet(CHXAltIDSet& altIDSet) const
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pItr)
    {
        altIDSet.Clear();

        IUnknown* pUnk = m_pItr->GetItem();
        IHXBuffer* pBuf = NULL;

        if (pUnk &&
            (HXR_OK == pUnk->QueryInterface(IID_IHXBuffer,
                                            (void**)&pBuf)))
        {
            // AltIDs are stored in network byte order in the
            // IHXBuffer
            UINT32 uIDCount = pBuf->GetSize() / 4;
            UINT32* pIDs = (UINT32*)pBuf->GetBuffer();

            // Add the AltIDs to altIDSet
            res = HXR_OK;
            for (UINT32 i = 0; (HXR_OK == res) && (i < uIDCount); i++)
            {
                UINT32 uAltID = DwToHost(pIDs[i]);

                res = altIDSet.AddID(uAltID);
            }
        }
        HX_RELEASE(pBuf);
        HX_RELEASE(pUnk);
    }

    return res;
}

HXBOOL CHXAltGroupIterator::More() const
{
    return (m_pGroupInfo != NULL);
}

void CHXAltGroupIterator::Next()
{
    if (m_pGroupInfo)
    {
        if (m_pItr)
        {
            if (!m_pItr->MoveNext())
            {
                // There are no more items.
                // Destroy the iterator so
                // we can move onto the next
                // property
                HX_RELEASE(m_pItr);
            }
        }

        if (!m_pItr)
        {
            // We need the next iterator
            const char* pKey = NULL;
            IUnknown* pUnk = NULL;

            HX_RESULT res = m_pGroupInfo->GetNextPropertyObject(pKey, pUnk);

            if (HXR_OK == res)
            {
                m_groupValue = pKey;

                res = getIterator(pUnk);
            }
            else
            {
                // We've reached the end
                HX_RELEASE(m_pGroupInfo);
            }
            HX_RELEASE(pUnk);
        }
    }
}

HX_RESULT
CHXAltGroupIterator::getIterator(IUnknown* pUnk)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pUnk)
    {
        IHXList* pList = NULL;
        res = pUnk->QueryInterface(IID_IHXList, (void**)&pList);

        if (HXR_OK == res)
        {
            HX_RELEASE(m_pItr);

            m_pItr = pList->Begin();

            if (!m_pItr)
            {
                res = HXR_UNEXPECTED;
            }
        }
        HX_RELEASE(pList);
    }

    return res;
}

CHXBWASAltGroupIterator::CHXBWASAltGroupIterator()
{}

CHXBWASAltGroupIterator::~CHXBWASAltGroupIterator()
{}

HX_RESULT
CHXBWASAltGroupIterator::Init(IHXValues* pFileHdr)
{
    return m_itr.Init(pFileHdr, "BW:AS");
}

HX_RESULT
CHXBWASAltGroupIterator::GetBandwidth(UINT32& uBandwidth) const
{
    HX_RESULT res = HXR_UNEXPECTED;

    const char* pStart = m_itr.GroupValue();
    char* pEnd = NULL;
    uBandwidth = strtoul(pStart, &pEnd, 10);

    if (*pStart && !*pEnd)
    {
        // Convert to bits per second
        uBandwidth *= 1000;
        res = HXR_OK;
    }

    return res;
}

HX_RESULT
CHXBWASAltGroupIterator::GetAltIDSet(CHXAltIDSet& altIDSet) const
{
    return m_itr.GetAltIDSet(altIDSet);
}

HXBOOL CHXBWASAltGroupIterator::More() const
{
    return m_itr.More();
}

void CHXBWASAltGroupIterator::Next()
{
    m_itr.Next();
}
