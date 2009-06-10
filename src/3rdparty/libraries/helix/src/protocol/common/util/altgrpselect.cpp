/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altgrpselect.cpp,v 1.4 2007/07/06 20:51:32 jfinnecy Exp $
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
#include "altgrpselect.h"
#include "pckunpck.h"
#include "hxval2util.h"
#include "altidset.h"
#include "hlxclib/memory.h"
#include "altgrpitr.h"
#include "altidfilt.h"

CHXAltGroupSelector::CHXAltGroupSelector() :
    m_pCCF(NULL)
{}

CHXAltGroupSelector::~CHXAltGroupSelector()
{
    HX_RELEASE(m_pCCF);
    Reset();
}

HX_RESULT 
CHXAltGroupSelector::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (pContext)
    {
        res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                       (void**)&m_pCCF);
    }

    return res;
}

void CHXAltGroupSelector::Reset()
{
    // Destroy all the filter objects
    while(!m_filterList.IsEmpty())
    {
        CHXAltIDFilter* pFilter = (CHXAltIDFilter*)m_filterList.RemoveHead();
        HX_DELETE(pFilter);
    }
}

HX_RESULT 
CHXAltGroupSelector::AddFilter(CHXAltIDFilter* pFilter)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pFilter)
    {
        if (m_filterList.AddTail(pFilter))
        {
            res = HXR_OK;
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupSelector::Select(UINT32 uHdrCount, IHXValues** pInHdrs, 
                            CHXAltIDSet& altIDSet)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    // We need at least 1 stream header
    if ((uHdrCount > 1) && pInHdrs)
    {
        // Fill altIDSet with all the available
        // Alt IDs
        res = getAllAltIDs(uHdrCount, pInHdrs, altIDSet);
                    
        if (HXR_OK == res)
        {
            CHXSimpleList::Iterator itr = m_filterList.Begin();

            // Apply the filters to the AltID set. We apply the
            // filters in the order that they were added. If a
            // filter causes the set to become empty then we
            // exit the loop early since there is nothing left
            // to filter
            for(; ((HXR_OK == res) && (itr != m_filterList.End()) &&
                   altIDSet.GetCount()); ++itr)
            {
                CHXAltIDFilter* pFilter = (CHXAltIDFilter*)*itr;
                
                if (pFilter)
                {
                    res = pFilter->Filter(uHdrCount, pInHdrs, m_altIDMap,
                                          altIDSet);
                }
            }
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupSelector::Select(UINT32 uHdrCount, IHXValues** pInHdrs, 
                            IHXValues** pOutHdrs)
{

    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pOutHdrs)
    {
        CHXAltIDSet altIDSet;

        // Fill altIDSet with the selected AltIDs
        res = Select(uHdrCount, pInHdrs, altIDSet);

        if (HXR_OK == res)
        {
            // Construct the stream headers for the selected
            // AltIDs

            UINT32 uStreamCount = uHdrCount - 1;

            res = createDefaultProperties(uHdrCount, pInHdrs, pOutHdrs);

            // See if altIDSet only has one AltID for each
            // stream. Sometimes the filters provided can't
            // narrow down the altIDSet to a single AltID
            // for each stream. It is also possible that
            // altIDSet only contains AltIDs for some of the
            // streams. If either of these 2 conditions occur
            // then we just return the default properties.

            if ((HXR_OK == res) &&
                hasOneAltForEachStream(uStreamCount, altIDSet))
            {
                // We have one AltID for each stream. Time to
                // merge the headers for the AltIDs into the
                // appropriate stream headers
                for (UINT32 j = 0; ((HXR_OK == res) && 
                                    (j < altIDSet.GetCount())); j++)
                {
                    UINT32 uAltID;

                    res = altIDSet.GetAltID(j, uAltID);
                    if (HXR_OK == res)
                    {
                        res = mergeAltProperties(uAltID,
                                                      uHdrCount, pInHdrs, 
                                                      pOutHdrs);
                    }
                }

                if (HXR_OK != res)
                {
                    // We failed to merge all the alt info.
                    // destroy the headers and fallback to
                    // using the default headers
                    destroyHdrs(uHdrCount, pOutHdrs);

                    res = createDefaultProperties(uHdrCount, pInHdrs, 
                                                  pOutHdrs);
                }
            }

        }
    }
    return res;
}

HX_RESULT 
CHXAltGroupSelector::createDefaultProperties(UINT32 uHdrCount, 
                                             IHXValues** pInHdrs, 
                                             IHXValues** pOutHdrs)
{
    HX_RESULT  res = HXR_INVALID_PARAMETER;

    if (uHdrCount && pInHdrs && pOutHdrs)
    {
        if (m_pCCF)
        {
            // Clear all the pOutHdrs pointers
            memset(pOutHdrs, 0, sizeof(IHXValues*) * uHdrCount);

            res = HXR_OK;
            for (UINT32 i = 0; (HXR_OK == res) && (i < uHdrCount); i++)
            {
                res = CreateValuesCCF(pOutHdrs[i], m_pCCF);
                
                if (HXR_OK == res)
                {
                    res = copyProperties(pOutHdrs[i], pInHdrs[i]);
                }
            }

            if (HXR_OK != res)
            {
                // Destroy everything we've created before
                // the error occured
                destroyHdrs(uHdrCount, pOutHdrs);                
            }
        }
        else
        {
            res = HXR_UNEXPECTED;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupSelector::mergeAltProperties(UINT32 uAltID,
                                        UINT32 uHdrCount, IHXValues** pInHdrs, 
                                        IHXValues** pOutHdrs)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (uHdrCount && pInHdrs && pOutHdrs)
    {        
        UINT32 uStreamID;

        // Find the streamID for this AltID
        res = m_altIDMap.GetStreamID(uAltID, uStreamID);

        if (HXR_OK == res)
        {
            UINT32 uHdrID = uStreamID + 1;
            if (uHdrID < uHdrCount)
            {
                IHXValues* pDst = pOutHdrs[uHdrID];
                IHXValues2* pSrc = NULL;
                
                // Get the headers for this AltID
                res = getAltInfo(pInHdrs[uHdrID], uAltID, pSrc);

                // Only copy the properties if res == HXR_OK and
                // pSrc is a valid pointer. getAltInfo() will return
                // HXR_OK, but set pSrc to NULL if the AltID refers
                // to the default AltID. In this case no properties
                // need to be copied.
                if ((HXR_OK == res) && pSrc)
                {
                    res = copyProperties(pDst, pSrc);
                }
                HX_RELEASE(pSrc);
            }
            else
            {
                // We got an invalid headerID somehow
                res = HXR_UNEXPECTED;
            }
        }
    }

    return res;
}

HX_RESULT
CHXAltGroupSelector::getAltInfo(IHXValues* pHdr, UINT32 uAltID, 
                                REF(IHXValues2*)pAltInfo) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr)
    {
        ULONG32 uDefaultAltID;

        // Check to see if uAltID matches the default AltID
        if (HXR_OK == pHdr->GetPropertyULONG32("alt-default-id", 
                                               uDefaultAltID) &&
            (uDefaultAltID == uAltID))
        {
            // uAltID matches the default ID so we don't need to
            // copy anything
            pAltInfo = NULL;
            res = HXR_OK;
        }
        else
        {
            // Get the headers for the AltID

            IHXValues2* pAlt = NULL;
            res = CHXIHXValues2Util::GetIHXValues2Property(pHdr, "Alt", pAlt);

            if (HXR_OK == res)
            {
                CHXString key;

                key.AppendULONG(uAltID);
                res = CHXIHXValues2Util::GetIHXValues2Property(pAlt, key, 
                                                               pAltInfo);
            }
            HX_RELEASE(pAlt);
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupSelector::copyProperties(IHXValues* pDst, IHXValues* pSrc) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pDst && pSrc)
    {
        const char* pKey = NULL;
        ULONG32 ulVal;
        IHXBuffer* pVal = NULL;

        // Copy all the ULONG32 properties
        HX_RESULT tmpRes = pSrc->GetFirstPropertyULONG32(pKey, ulVal);

        res = HXR_OK;
        while ((HXR_OK == res) && (HXR_OK == tmpRes))
        {
            // Copy all fields except alt-default-id.
            // We do this because we don't want any
            // alt-group related info in the output
            if (strcasecmp(pKey, "alt-default-id"))
            {
                res = pDst->SetPropertyULONG32(pKey, ulVal);
            }

            tmpRes = pSrc->GetNextPropertyULONG32(pKey, ulVal);
        }

        if (HXR_OK == res)
        {
            // Copy all the CString properties
            tmpRes = pSrc->GetFirstPropertyCString(pKey, pVal);
            while ((HXR_OK == res) && (HXR_OK == tmpRes))
            {
                res = pDst->SetPropertyCString(pKey, pVal);
                HX_RELEASE(pVal);
                
                tmpRes = pSrc->GetNextPropertyCString(pKey, pVal);
            }
        }

        if (HXR_OK == res)
        {
            // Copy all the buffer properties
            tmpRes = pSrc->GetFirstPropertyBuffer(pKey, pVal);
            while ((HXR_OK == res) && (HXR_OK == tmpRes))
            {
                res = pDst->SetPropertyBuffer(pKey, pVal);
                HX_RELEASE(pVal);
                
                tmpRes = pSrc->GetNextPropertyBuffer(pKey, pVal);
            }
        }

        // We don't copy any object properties at this time because
        // the Alt and Alt-Group properties are the only ones that
        // show up in headers right now. We don't want those
        // to be copied in the output so there is no need for any
        // code for copying objects
    }

    return res;
}

HX_RESULT
CHXAltGroupSelector::getAllAltIDs(UINT32 uHdrCount, IHXValues** pInHdrs,
                                  CHXAltIDSet& altIDSet)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if ((uHdrCount > 1) && pInHdrs)
    {
        altIDSet.Clear();
        m_altIDMap.Clear(); // Clear the AltID -> StreamID map

        // We start at 1 because header[0] is the file header
        res = HXR_OK;
        for (UINT32 i = 1; (HXR_OK == res) && i < uHdrCount; i++)
        {
            UINT32 uStreamID = i - 1; // Stream ID is header index - 1

            ULONG32 ulDefaultAltID = 0;

            // Check of a default AltID and add it to the set
            if (HXR_OK == pInHdrs[i]->GetPropertyULONG32("alt-default-id",
                                                         ulDefaultAltID))
            {
                res = altIDSet.AddID(ulDefaultAltID);

                if (HXR_OK == res)
                {
                    res = m_altIDMap.AddID(ulDefaultAltID, uStreamID);

                    if (HXR_OK == res)
                    {
                        // Get all the other AltIDs for this stream. 
                        // NOTE: We only get these AltIDs if a
                        //       alt-default-id field is available.
                        //       We do this because the default stream
                        //       needs an AltID in the set. If we don't
                        //       have one, then the default stream can't
                        //       get selected unless there is an error.
                        res = getStreamAltIDs(uStreamID, pInHdrs[i], altIDSet);
                    }
                }
            }
        }

        res = HXR_OK;
    }

    return res;
}

HX_RESULT 
CHXAltGroupSelector::getStreamAltIDs(UINT32 uStreamID, IHXValues* pStreamHdr,
                                     CHXAltIDSet& altIDSet)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pStreamHdr)
    {
        IHXValues2* pAlt = NULL;

        res = CHXIHXValues2Util::GetIHXValues2Property(pStreamHdr, 
                                                       "Alt", pAlt);
        if (HXR_OK == res)
        {
            const char* pKey = NULL;
            IUnknown* pUnk = NULL;
            
            HX_RESULT tmpRes = pAlt->GetFirstPropertyObject(pKey, pUnk);
            
            while((HXR_OK == res) && (HXR_OK == tmpRes))
            {
                char* pEnd = NULL;
                UINT32 ulAltID = strtoul(pKey, &pEnd, 10);
                
                if (*pKey && !*pEnd)
                {
                    res = altIDSet.AddID(ulAltID);
                    
                    if (HXR_OK == res)
                    {
                        res = m_altIDMap.AddID(ulAltID, uStreamID);
                    }
                }
                else
                {
                    // We got an invalid AltID key
                    res = HXR_UNEXPECTED;
                }
                
                HX_RELEASE(pUnk);
                tmpRes = pAlt->GetNextPropertyObject(pKey, pUnk);
            }
        }
        else if (HXR_INVALID_PARAMETER == res)
        {
            // This likely means that the "Alt" property was
            // not present so there aren't any other AltIDs for
            // this stream
            res = HXR_OK;
        }
        HX_RELEASE(pAlt);
    }

    return res;
}

HXBOOL
CHXAltGroupSelector::hasOneAltForEachStream(UINT32 uStreamCount,
                                            const CHXAltIDSet& altIDSet) const
{
    HXBOOL bRet = FALSE;

    // Check to see if we have the same number
    // of altIDs as streams
    if (altIDSet.GetCount() == uStreamCount)
    {
        UCHAR* pHasID = new UCHAR[uStreamCount];

        if (pHasID)
        {
            memset(pHasID, 0, uStreamCount);

            bRet = TRUE;
            // Iterate over all the AltIDs in the set and keep track
            // of what streams we have seen AltIDs for.
            for (UINT32 i = 0 ; bRet && (i < altIDSet.GetCount()); i++)
            {
                UINT32 uAltID;
                UINT32 uStreamID;
                if ((HXR_OK == altIDSet.GetAltID(i, uAltID)) &&
                    (HXR_OK == m_altIDMap.GetStreamID(uAltID, uStreamID)))
                {
                    if ((uStreamID >= uStreamCount) ||
                        pHasID[uStreamID])
                    {
                        // We either got an invalid stream ID or
                        // we've come across a stream we've seen
                        // an ID for.
                        bRet = FALSE;
                    }
                    else
                    {
                        pHasID[uStreamID] = TRUE;
                    }
                }
            }

            delete [] pHasID;
            pHasID = NULL;
        }
    }

    return bRet;
}

void 
CHXAltGroupSelector::destroyHdrs(UINT32 uHdrCount, IHXValues** pHdrs) const
{
    if (pHdrs)
    {
        for (UINT32 i = 0; i < uHdrCount; i++)
        {
            HX_RELEASE(pHdrs[i]);
        }
    }
}
