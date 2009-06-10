/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rateselinfo.cpp,v 1.4 2007/05/23 00:43:01 darrick Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#include "hxcom.h"
#include "hxengin.h"
#include "hxqosinfo.h"

#include "hxslist.h"

#include "timeval.h"
#include "base_callback.h"

#include "pcktstrm.h"
#include "rateselinfo.h"

#define   DEFAULT_RULELIST_SIZE   4


RateSelectionInfo::RateSelectionInfo()
: m_ulRefCount(0)
{
    memset(m_Info, 0, sizeof(UINT32) * (int)RSI_MAX);
}

RateSelectionInfo::~RateSelectionInfo()
{
    CHXMapLongToObj::Iterator i;
    for (i = m_StreamMap.Begin(); 
         i != m_StreamMap.End();
         ++i)
    {        
        StreamRateSelectionInfo* pStreamInfo = (StreamRateSelectionInfo*)*i;
        HX_DELETE(pStreamInfo);
    }

    m_StreamMap.RemoveAll();

    
}

STDMETHODIMP
RateSelectionInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXRateSelectionInfo*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRateSelectionInfo))
    {
        AddRef();
        *ppvObj = (IHXRateSelectionInfo*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(UINT32) 
RateSelectionInfo::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(UINT32) 
RateSelectionInfo::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


/******************************************************************************
 * \brief SetInfo - Set rate selection info field.
 * \param Id        [in] Field ID of info to set.
 * \param ulInfo    [in] Value of info to set.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if invalid Id
 *                  supplied for this method.
 *****************************************************************************/

STDMETHODIMP
RateSelectionInfo::SetInfo(RateSelInfoId Id, 
                           UINT32 ulInfo)
{
    HX_RESULT hr = HXR_OK;

    switch (Id)
    {
    case RSI_QUERYPARAM_IR:
    case RSI_BANDWIDTH:
    case RSI_SDB:
    case RSI_LINKCHAR_MBW:
    case RSI_LINKCHAR_GBW:
    case RSI_LINKCHAR_MTD:

        m_Info[Id] = ulInfo;
        break;

    default:
        hr = HXR_INVALID_PARAMETER;
    }

    return hr;
}

/******************************************************************************
 * \brief SetInfo - Set rate selection info field for a logical stream.
 * \param Id                    [in] Field ID of info to set.
 * \param ulLogicalStreamId     [in] Logical stream id for stream this info is for.
 * \param ulInfo                [in] Value of info to set.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if invalid Id
 *                  supplied for this method.
 *****************************************************************************/

STDMETHODIMP
RateSelectionInfo::SetInfo(RateSelInfoId Id, 
                           UINT16 ulLogicalStreamId, 
                           UINT32 ulInfo)
{
    StreamRateSelectionInfo* pStreamInfo = NULL;
    if (!m_StreamMap.Lookup(ulLogicalStreamId, (void*&)pStreamInfo))
    {
        pStreamInfo = new StreamRateSelectionInfo();
        m_StreamMap.SetAt(ulLogicalStreamId, (void*)pStreamInfo);
    }

    return pStreamInfo->SetInfo(Id, ulInfo);
}


/******************************************************************************
 * \brief GetInfo - Get rate selection info field.
 * \param Id                    [in] Field ID of info to get.
 * \param ulInfo                [out] Value of info to get.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if invalid Id
 *                  supplied for this method.
 *****************************************************************************/

STDMETHODIMP
RateSelectionInfo::GetInfo(RateSelInfoId Id, 
                           UINT32& ulInfo)
{
    HX_RESULT hr = HXR_OK;
    switch (Id)
    {
    case RSI_QUERYPARAM_IR:
    case RSI_BANDWIDTH:
    case RSI_SDB:
    case RSI_LINKCHAR_MBW:
    case RSI_LINKCHAR_GBW:
    case RSI_LINKCHAR_MTD:

        ulInfo = m_Info[Id];
        break;

    default:
        hr = HXR_INVALID_PARAMETER;
    }

    return hr;
}


/******************************************************************************
 * \brief GetInfo - Get rate selection info field.
 * \param Id                    [in] Field ID of info to get.
 * \param ulLogicalStreamId     [in] Logical stream id for stream this info is for.
 * \param ulInfo                [out] Value of info to get.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if invalid Id
 *                  supplied for this method.
 *****************************************************************************/

STDMETHODIMP
RateSelectionInfo::GetInfo(RateSelInfoId Id, 
                           UINT16 ulLogicalStreamId, 
                           UINT32& ulInfo)
{
    StreamRateSelectionInfo* pStreamInfo = NULL;
    if (!m_StreamMap.Lookup(ulLogicalStreamId, (void*&)pStreamInfo))
    {
        return HXR_FAIL;
    }

    return pStreamInfo->GetInfo(Id, ulInfo);
}


/******************************************************************************
 * \brief GetNumSubscribedRules - Get number of subscribed rules for a stream.
 * \param ulLogicalStreamId     [in] Logical stream to retrieve rule count for.
 *
 * \return          Number of subscribed rules, or 0 if stream not registered.
 * \note            A valid, registered stream can be subscribed to 0 rules.
 *****************************************************************************/

STDMETHODIMP_(UINT16)
RateSelectionInfo::GetNumSubscribedRules(UINT16 ulLogicalStreamId)
{
    StreamRateSelectionInfo* pStreamInfo = NULL;
    if (!m_StreamMap.Lookup(ulLogicalStreamId, (void*&)pStreamInfo))
    {
        return 0;
    }

    return pStreamInfo->GetNumSubscribedRules();
}

/******************************************************************************
 * \brief GetNumRegisteredStreams - Get number of registered streams.
 *
 * \return          Number of registered streams (streams we have SETUPs for).
 *****************************************************************************/

STDMETHODIMP_(UINT16)
RateSelectionInfo::GetNumRegisteredStreams()
{
    return m_StreamMap.GetCount();
}


/******************************************************************************
 * \brief GetSubscribedRules - Get list of subscribed rules for a stream.
 * \param ulLogicalStreamId     [in] Logical stream to retrieve rules for.
 * \param ulArraySize           [in] Size of 'Rules' array. (Max boundary.)
 * \param Rules                 [in/out] Array to receive rules.
 *
 * \return          HXR_OK if successful, HXR_FAIL if stream not registered.
 *****************************************************************************/

STDMETHODIMP 
RateSelectionInfo::GetSubscribedRules(UINT16 ulLogicalStreamId, 
                                      UINT16 ulArraySize, 
                                      UINT16* Rules)
{
    StreamRateSelectionInfo* pStreamInfo = NULL;
    if (!m_StreamMap.Lookup(ulLogicalStreamId, (void*&)pStreamInfo))
    {
        return HXR_FAIL;
    }

    return pStreamInfo->GetSubscribedRules(ulArraySize, Rules);
}


/******************************************************************************
 * \brief GetRegisteredLogicalStreamIds - Get list of registered stream 
 *                                        ids for a session.
 * \param ulArraySize           [in] Size of 'StreamIds' array. (Max boundary.)
 * \param StreamIds             [in/out] Array to receive stream ids.
 *
 * \return          HXR_OK - failure not possible.
 *****************************************************************************/

STDMETHODIMP 
RateSelectionInfo::GetRegisteredLogicalStreamIds(UINT16 ulArraySize,
                                                 UINT16* StreamIds)
{
    UINT16 j = 0;
    CHXMapLongToObj::Iterator i;
    for (i = m_StreamMap.Begin(); 
         i != m_StreamMap.End() && j < ulArraySize;
         ++i)
    {        
        StreamIds[j++] = (UINT16)i.get_key();
    }    
    return HXR_OK;
}



void
RateSelectionInfo::Dump()
{
    printf("************** RATESELINFO DUMP ***************\n");
    printf("---Aggregate Info---\n");
    printf("bandwidth: %u   gbw: %u   ir: %u\n",
           m_Info[RSI_BANDWIDTH], m_Info[RSI_LINKCHAR_GBW], m_Info[RSI_QUERYPARAM_IR]);
    CHXMapLongToObj::Iterator i;
    for (i = m_StreamMap.Begin(); 
         i != m_StreamMap.End();
         ++i)
    {        
        StreamRateSelectionInfo* pInfo = (StreamRateSelectionInfo*)*i;
        printf("---Stream %u Info---\n", (UINT16)i.get_key());
        pInfo->Dump();
    }    
    fflush(stdout);
}

StreamRateSelectionInfo::StreamRateSelectionInfo()
: m_bGotTrackId(FALSE)
{
    memset(m_Info, 0, sizeof(UINT32) * (int)RSI_MAX);
    m_Info[RSI_DEFAULT_RULE] = INVALID_RULE_NUM;
}

StreamRateSelectionInfo::~StreamRateSelectionInfo()
{    
    // No need to delete individually since the elements are 
    // void*-casted UINT16s. Yes, this is ugly.
    m_RuleSubs.RemoveAll();
}


/******************************************************************************
 * \brief GetInfo - Get rate selection info field for stream.
 * \param Id                    [in] Field ID of info to get.
 * \param ulInfo                [out] Value of info to get.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if invalid Id
 *                  supplied for this method.
 *****************************************************************************/

HX_RESULT
StreamRateSelectionInfo::GetInfo(RateSelInfoId Id, UINT32& ulInfo)
{
    HX_RESULT hr = HXR_OK;

    switch (Id)
    {
    case RSI_TRACKID:

        if (!m_bGotTrackId)
        {
            hr = HXR_FAIL;
            break;
        }
        // Else fall through.

    case RSI_LINKCHAR_MBW:
    case RSI_LINKCHAR_GBW:
    case RSI_LINKCHAR_MTD:
    case RSI_DEFAULT_RULE:
    case RSI_AVGBITRATE:
    case RSI_STREAMGROUPID:
    case RSI_ISDEFAULTSTREAM:

        ulInfo = m_Info[Id];
        break;

    default:

        hr = HXR_INVALID_PARAMETER;
    }

    return hr;
}


/******************************************************************************
 * \brief SetInfo - Set rate selection info field for stream.
 * \param Id                    [in] Field ID of info to set.
 * \param ulInfo                [in] Value of info to set.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if invalid Id
 *                  supplied for this method. HXR_UNEXPECTED if attempting to
 *                  double-subscribe to a rule. HXR_FAIL if unable to subscribe
 *                  to or unsubscribe from rule.
 *****************************************************************************/

HX_RESULT
StreamRateSelectionInfo::SetInfo(RateSelInfoId Id, UINT32 ulInfo)
{
    HX_RESULT hr = HXR_OK;

    LISTPOSITION pos;
    switch (Id)
    {
    case RSI_TRACKID:

        m_bGotTrackId = TRUE;
        // Fall through.

    case RSI_LINKCHAR_MBW:
    case RSI_LINKCHAR_GBW:
    case RSI_LINKCHAR_MTD:
    case RSI_DEFAULT_RULE:
    case RSI_AVGBITRATE:
    case RSI_STREAMGROUPID:
    case RSI_ISDEFAULTSTREAM:

        m_Info[Id] = ulInfo;
        break;

    case RSI_SUBSCRIBE_RULE:

        //XXXDPL Ugly cast!
        pos = m_RuleSubs.Find((void*)ulInfo);

        if (pos)
        {
            hr = HXR_UNEXPECTED;
            break;
        }

        pos = m_RuleSubs.AddTail((void*)ulInfo);
        
        if (!pos)
        {
            HX_ASSERT(!"RateInfo: Could not add rule sub to sub list!");
            hr = HXR_FAIL;
        }

        break;

    case RSI_UNSUBSCRIBE_RULE:

        //XXXDPL Ugly cast!
        pos = m_RuleSubs.Find((void*)ulInfo);
        if (!pos)
        {
            hr = HXR_FAIL;
        }

        m_RuleSubs.RemoveAt(pos);

        break;

    default:

        hr = HXR_INVALID_PARAMETER;
    }
    return hr;
}


/******************************************************************************
 * \brief GetNumSubscribedRules - Get number of subscribed rules for stream.
 *
 * \return          Number of subscribed rules, or 0 if stream not registered.
 * \note            A valid, registered stream can be subscribed to 0 rules.
 *****************************************************************************/

UINT16
StreamRateSelectionInfo::GetNumSubscribedRules()
{
    return (UINT16)m_RuleSubs.GetCount();
}


/******************************************************************************
 * \brief GetSubscribedRules - Get list of subscribed rules for stream.
 * \param ulArraySize           [in] Size of 'Rules' array. (Max boundary.)
 * \param Rules                 [in/out] Array to receive rules.
 *
 * \return          HXR_OK if successful, HXR_FAIL if stream not registered.
 *****************************************************************************/

HX_RESULT
StreamRateSelectionInfo::GetSubscribedRules(UINT16 ulArraySize, 
                                            UINT16* Rules)
{
    CHXSimpleList::Iterator i;
    UINT16 j = 0;

    for (i = m_RuleSubs.Begin(); i != m_RuleSubs.End(); ++i)
    {
        if (j >= ulArraySize)
        {
            return HXR_OK;
        }

        Rules[j++] = (UINT32)*i;
    }
         

    return HXR_OK;
}

void 
StreamRateSelectionInfo::Dump()
{
    CHXSimpleList::Iterator i;
    printf("gbw %u   def rule: %u   avg bitrate: %u   rules: [ ",
           m_Info[RSI_LINKCHAR_GBW], m_Info[RSI_DEFAULT_RULE], m_Info[RSI_AVGBITRATE]);
    for (i = m_RuleSubs.Begin(); i != m_RuleSubs.End(); ++i)
    {
        printf("%u ", (UINT32)*i);
    }
    printf("]\n");

}
