/* ***** BEGIN LICENSE BLOCK *****
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
#include "streamselector.h"

#include "hxccf.h"
#include "hxstats.h"
#include "hxerror.h"

#include "ispifs.h"
#include "hxclientprofile.h"
#include "hxqos.h"
#include "hxqosinfo.h"
#include "qos_cfg_names.h"

#include "hxassert.h"

#include "timeval.h"
#include "base_callback.h"
#include "pcktstrm.h"

#include "uberstreammgr.h"

static const char* STRM_SELECT_ERR_STR = "Unable to stream %s to %s due to "
                                  "Insufficient Bandwidth or because Client's "
                                  "Buffer is too small for Predata.";
static UINT32 DEFAULT_INIT_TARGET_RATE = 1;

// Implements basic IUnknown functionality
BEGIN_INTERFACE_LIST(CStreamSelector)
END_INTERFACE_LIST



/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::CStreamSelector
// Purpose:
//  Constructor
CStreamSelector::CStreamSelector():
    m_pUberMgr(NULL)
    , m_pStats(NULL)
    , m_pQoSConfig(NULL)
    , m_pErrorMsg(NULL)
    , m_lDebugOutput(0)
    , m_bDoRel6RateSelection(FALSE)
{
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::~CStreamSelector
// Purpose:
//  Destructor
CStreamSelector::~CStreamSelector()
{
    HX_RELEASE(m_pUberMgr);
    HX_RELEASE(m_pStats);
    HX_RELEASE(m_pQoSConfig);
    HX_RELEASE(m_pErrorMsg);
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::Init
// Purpose:
//  Initializes the stream selector
HX_RESULT
CStreamSelector::Init(IHXUberStreamManager* pUberMgr, IUnknown* pContext, IHXSessionStats* pStats, IHXQoSProfileConfigurator* pConfig)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (m_pUberMgr || !pUberMgr || !pContext || !pStats || !pConfig)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    m_pUberMgr = pUberMgr;
    m_pUberMgr->AddRef();

    m_pStats = pStats;
    m_pStats->AddRef();

    m_pQoSConfig = pConfig;
    m_pQoSConfig->AddRef();


    res = pContext->QueryInterface(IID_IHXErrorMessages, (void **)&m_pErrorMsg);

    m_pQoSConfig->GetConfigInt(DEBUG_OUTPUT, m_lDebugOutput);

    INT32 lTemp = 0;
    if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_RA_USE_REL6_RATE_SEL, lTemp)))
    {
        m_bDoRel6RateSelection = (BOOL)lTemp;
    }

    res = GetSelectionParams();

    HX_ASSERT(SUCCEEDED(res));
    return res;
}


//  GetDefaultPreDecoderBufSize
// Purpose:
//  Determines the default pre-decode buffer size
UINT32
GetDefaultPreDecoderBufSize(UINT32 ulMaxRate)
{
    UINT32 ulPreDecodeBufSize = 0;
    if ((ulMaxRate == 0) || (ulMaxRate > 131072))
    {
        ulPreDecodeBufSize = 51200;
    }
    else if (ulMaxRate <= 65536)
    {
        ulPreDecodeBufSize = 20480;
    }
    else if (ulMaxRate <= 131072)
    {
        ulPreDecodeBufSize = 40960;
    }

    HX_ASSERT(ulPreDecodeBufSize);
    return ulPreDecodeBufSize;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::ExcludeUnusableStreams
// Purpose:
//  Uses m_selectionParam to determine which bandwidth groupings are valid or not
//
//  ulAdaptationBufSize: Used to update the VPDBSize with the buffer size received in *-Adaptation headers.
//                       Should be ZERO if we are not using *-Adaptation which is default value.
// Returns:
//  HXR_OK if there is at least one active stream.  Otherwise, returns HXR_FAIL.
HX_RESULT
CStreamSelector::ExcludeUnusableStreams(const StreamAdaptationParams* pStreamAdaptationParams)
{
    HX_RESULT res = HXR_OK;
    UINT16 unStreamNum = MAX_UINT16;

    HX_ASSERT(m_selectionParam.m_ulMaxTargetRate > 0);
    HX_ASSERT(m_selectionParam.m_ulMaxPreDecBufSize > 0);

    // Validate state
    if (!m_pUberMgr)
    {
    HX_ASSERT(FALSE);
    return HXR_FAIL;
    }

    if( pStreamAdaptationParams )
    {
        if (pStreamAdaptationParams->m_ulClientBufferSize > 0)
        {
            m_selectionParam.m_ulMaxPreDecBufSize = pStreamAdaptationParams->m_ulClientBufferSize;
        }
        
        unStreamNum = pStreamAdaptationParams->m_unStreamNum;
    }

    // If we are evaluating with per stream *-Adaptation headers, we should only be
    // checking all the physical streams present in that stream's stream group.
    if(unStreamNum != MAX_UINT16)
    {
        // Get the logical stream manager
        IHXRateDescEnumerator* pLogicalStream = NULL;
        res = m_pUberMgr->GetLogicalStream(unStreamNum, pLogicalStream);

        for (UINT32 i=0; SUCCEEDED(res) && i < pLogicalStream->GetNumRateDescriptions(); i++)
        {
            IHXRateDescription* pRateDesc = NULL;
            res = pLogicalStream->GetRateDescription(i, pRateDesc);

            if (SUCCEEDED(res))
            {
                UINT32 ulPredata = 0;
                UINT32 ulAvgRate = 0;
                pRateDesc->GetPredata(ulPredata);
                pRateDesc->GetAvgRate(ulAvgRate);

                if (pStreamAdaptationParams->m_ulClientBufferSize > 0 && 
                    pStreamAdaptationParams->m_ulClientBufferSize < ulPredata)
                {
                    pRateDesc->ExcludeFromSelection(TRUE, HX_SEL_INADEQUATE_CLIENT_CAPABILITIES);
                    pRateDesc->ExcludeFromSwitching(TRUE, HX_SWI_INADEQUATE_CLIENT_CAPABILITIES);

                    //Also exclude any bandwidth groupings containing this stream
                    res = ExcludeBandwidthGroupings(unStreamNum, ulAvgRate);
                }
            }

            HX_RELEASE(pRateDesc);
        }

        HX_RELEASE(pLogicalStream);

        if (SUCCEEDED(res) && m_selectionParam.m_ulMaxTargetRate > 0)
        {
            res = VerifyMaxTargetRate();
        }

        return res;
    }

    // Determine which bandwidth groupings are valid based on m_selectionParam
    UINT32 ulNumActiveRateDescriptions = 0;
    for (UINT32 i = 0; i < m_pUberMgr->GetNumRateDescriptions() && SUCCEEDED(res); i++)
    {
    IHXRateDescription* pBandwidthGrouping = NULL;
    res = m_pUberMgr->GetRateDescription(i, pBandwidthGrouping);

    if (SUCCEEDED(res))
    {
        HX_RESULT resVerify = VerifyBandwidthGrouping(pBandwidthGrouping);

        // Exclude any streams that failed to verify from selection/switching
        pBandwidthGrouping->ExcludeFromSelection(FAILED(resVerify), HX_SEL_INADEQUATE_CLIENT_CAPABILITIES);
        pBandwidthGrouping->ExcludeFromSwitching(FAILED(resVerify), HX_SWI_INADEQUATE_CLIENT_CAPABILITIES);

        if (SUCCEEDED(resVerify))
        ulNumActiveRateDescriptions++;
    }

    HX_RELEASE(pBandwidthGrouping);
    }

    if (SUCCEEDED(res) && ulNumActiveRateDescriptions == 0)
    res = HXR_NOTENOUGH_PREDECBUF;

    if (FAILED(res))
    LogClientError();

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::VerifyStreams
// Purpose:
//  Given a IHXRateDescription, checks that it is appropriate for m_selectionParam
HX_RESULT
CStreamSelector::VerifyBandwidthGrouping(IHXRateDescription* pBandwidthGrouping)
{
    HX_RESULT res = HXR_OK;
    if (m_pStats->IsUseMDP() == FALSE )
    {
        return res;
    }

    // Validate state
    if (!pBandwidthGrouping)
    {
    HX_ASSERT(FALSE);
    return HXR_FAIL;
    }

    UINT32 ulRate = 0;
    if (SUCCEEDED(res))
    res = pBandwidthGrouping->GetAvgRate(ulRate);

    // Check bandwidth
    if (SUCCEEDED(res) && m_selectionParam.m_ulMaxTargetRate > 0)
    {
    // we'll be aggressive not to use MaxRate here
    if (m_selectionParam.m_ulMaxTargetRate < ulRate)
    {
        res = HXR_NOTENOUGH_BANDWIDTH;
    }
    }

    UINT32 ulPredata = 0;
    if (SUCCEEDED(res))
    res = pBandwidthGrouping->GetPredata(ulPredata);
    
        // Check decode buffer size
        if (SUCCEEDED(res) && m_selectionParam.m_ulMaxPreDecBufSize > 0)
        {
        if (1 == m_selectionParam.m_ulMaxPreDecBufSize)
        {
            // depends on media rate
            if (GetDefaultPreDecoderBufSize(ulRate) < ulPredata)
            {
            res = HXR_NOTENOUGH_PREDECBUF;
            }
        }
        else if (m_selectionParam.m_ulMaxPreDecBufSize < ulPredata)
        {
            res = HXR_NOTENOUGH_PREDECBUF;
        }
    }

    // Log selection info
/*
    if (HXR_OK != res)
    {
    INT32 lTemp = 0;
    if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
    {
        if (1 == m_selectionParam.m_ulMaxPreDecBufSize)
        {
        printf("PreDecBuf: %u\n", GetDefaultPreDecoderBufSize(ulRate));
        }
        printf("Ignoring - ");
        //((CRateDescription*)pBandwidthGrouping)->Dump();
    }
    }
*/

    return res;
}



/******************************************************************************
 * \brief SelectInitialRateDesc - Choose initial rate description.
 * \param pRateSelInfo      [in] Object with rate selection criteria info.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if NULL
 *                  rate selection info object, HXR_UNEXPECTED/HXR_IGNORE for
 *                  unexpected failure cases.
 *****************************************************************************/

HX_RESULT
CStreamSelector::SelectInitialRateDesc(IHXRateSelectionInfo* pRateSelInfo)
{
    // HXR_IGNORE in these cases means ignore this selection criteria.
    // HXR_OK means we successfully set criteria and we should not use lower pri
    // criteria.

    HX_RESULT hr = HXR_IGNORE;
    UINT32 ulValue = 0;
    UINT16 uiNumStreams = 0;
    UINT16* pStreamIds = NULL;

    if (!pRateSelInfo)
    {
        hr = HXR_INVALID_PARAMETER;
    }


    if (hr == HXR_IGNORE
    &&  SUCCEEDED(pRateSelInfo->GetInfo(RSI_QUERYPARAM_IR, ulValue))
    &&  ulValue)
    {
        if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
        {
            fprintf(stderr, "Initial Rate Selection: Using ir=: %u\n", ulValue);
            fflush(stderr);
        }
        SetInitialRateDesc(ulValue);

        // Duck out early. No further processing needed!
        return HXR_OK;
    }


    if (hr == HXR_IGNORE)
    {
        uiNumStreams = pRateSelInfo->GetNumRegisteredStreams();
    }

    // This would mean the client has not SETUP any streams!
    // Why bother picking a rate?
    if (uiNumStreams < 1)
    {
        hr = HXR_UNEXPECTED;
    }
    else
    {
        pStreamIds = new UINT16[uiNumStreams];

        if (FAILED(pRateSelInfo->GetRegisteredLogicalStreamIds(uiNumStreams, pStreamIds))
        ||  !pStreamIds)
        {
            HX_ASSERT(!"StreamSelector could not get registered stream ids!\n");
            hr = HXR_UNEXPECTED;
        }
    }


    if (hr == HXR_IGNORE)
    {
        hr = HandleRuleSubscriptions(pRateSelInfo, uiNumStreams, (const UINT16*&)pStreamIds);
    }

    UINT32 ulFirstStreamDefaultRule = 0;

    if (hr == HXR_IGNORE)
    {
        // If this is 3gp multirate content, then default rule should be set for all 
        // streams. It isn't set for any other kind of content, including 3gp single rate.
        hr = pRateSelInfo->GetInfo(RSI_DEFAULT_RULE, pStreamIds[0], ulFirstStreamDefaultRule);

        if (SUCCEEDED(hr))
        {
            hr = HXR_IGNORE; // Continue selection process.
        }
    }

    // Skip this branch only if 3gp rel6 multirate content!
    if (!m_bDoRel6RateSelection 
    ||  ulFirstStreamDefaultRule == INVALID_RULE_NUM)
    {
        if (hr == HXR_IGNORE)
        {
            hr = HandleStreamLinkChar(pRateSelInfo, uiNumStreams, (const UINT16*&)pStreamIds);
        }

        if (hr == HXR_IGNORE)
        {
            if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_LINKCHAR_GBW, ulValue))
            &&  ulValue)
            {
                if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
                {
                    fprintf(stderr, "Initial Rate Selection: Using Aggregate Link-Char GBW: %u\n", ulValue);
                    fflush(stderr);
                }

                hr = HXR_OK;
                SetInitialRateDesc(ulValue);
            }
        }

        if (hr == HXR_IGNORE)
        {
            if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_BANDWIDTH, ulValue))
            &&  ulValue)
            {
                if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
                {
                    fprintf(stderr, "Initial Rate Selection: Using Bandwidth: %u\n", ulValue);
                    fflush(stderr);
                }

                hr = HXR_OK;
                SetInitialRateDesc(ulValue);
            }
        }

        if (hr == HXR_IGNORE)
        {
            INT32 lTemp = 0;
            if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_RA_DEFAULT_MEDIARATE, lTemp))
            &&  lTemp)
            {
                if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
                {
                    fprintf(stderr, "Initial Rate Selection: DefaultMediaRate: %d\n", lTemp);
                    fflush(stderr);
                }
                hr = HXR_OK;
                SetInitialRateDesc((UINT32)lTemp);
            }
        }
    }

    // This is the last line of defense. We MUST be able to pick a default avgbitrate
    // for this stream at least. Also handles Rel6 multirate intelligent stream
    // selection.
    if (hr == HXR_IGNORE)
    {
        hr = HandleStreamRegistration(pRateSelInfo, uiNumStreams, (const UINT16*&)pStreamIds);
    }

    if (FAILED(hr))
    {
        // Choose the lowest rate.
        hr = SetInitialRateDesc(1);
    }

    HX_VECTOR_DELETE(pStreamIds);
    return hr;
}

/******************************************************************************
 * \brief HandleRuleSubscriptions - Handle subscribes for initial rate selection.
 * \param pRateSelInfo      [in] Object with rate selection criteria info.
 * \param uiNumStreams      [in] Number of SETUP streams.
 * \param pStreams          [in] Logical stream ids of SETUP streams.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if NULL
 *                  rate selection info object or if no streams SETUP,
 *                  HXR_IGNORE if not able to  subscribe to rules for all streams.
 *****************************************************************************/

HX_RESULT
CStreamSelector::HandleRuleSubscriptions(IHXRateSelectionInfo* pRateSelInfo,
                                         const UINT16 uiNumStreams,
                                         const UINT16*& pStreams)
{
    HX_RESULT hr = HXR_IGNORE;
    UINT16 uiNumSubscribedStreams = 0;

    if (!pRateSelInfo || uiNumStreams < 1 || !pStreams)
    {
        HX_ASSERT(!"Invalid params caught in CStreamSelector::HandleRuleSubscriptions() - should be caught earlier!");
        return HXR_INVALID_PARAMETER;
    }

    for (UINT16 i = 0; i < uiNumStreams; i++)
    {
        UINT16 uiNumRules = pRateSelInfo->GetNumSubscribedRules(pStreams[i]);

        if (uiNumRules < 1)
        {
            // Subscribe to rules for other streams if they exist. But these
            // subscriptions will probably get overwritten
            continue;
        }


        UINT16* pRules = new UINT16[uiNumRules];

        if (FAILED(pRateSelInfo->GetSubscribedRules(pStreams[i],
                                                    uiNumRules,
                                                    pRules)))
        {
            HX_ASSERT(!"IRS: Could not get subscribed rules list!");
            HX_VECTOR_DELETE(pRules);
            continue;
        }


        HX_ASSERT(pRules);

        if (pRules)
        {
            for (UINT16 j = 0; j < uiNumRules; j++)
            {
                if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
                {
                    fprintf(stderr, "Initial Rate Selection: Subscribe strm: %u rule: %u\n", pStreams[i], pRules[j]);
                    fflush(stderr);
                }
                m_pUberMgr->SubscribeLogicalStreamRule(pStreams[i], pRules[j], NULL);
            }
        }

        uiNumSubscribedStreams++;
        HX_VECTOR_DELETE(pRules);
    }

    HX_ASSERT(uiNumSubscribedStreams <= uiNumStreams);

    // If we subscribe to rules in less streams than we have SETUP, then
    // other lower-priority aggregate criteria may override the subscribes.
    if (uiNumSubscribedStreams == uiNumStreams)
    {
        hr = HXR_OK;
    }

    return hr;
}

/******************************************************************************
 * \brief HandleStreamLinkChar - Handle stream link char info for initial rate selection.
 * \param pRateSelInfo      [in] Object with rate selection criteria info.
 * \param uiNumStreams      [in] Number of SETUP streams.
 * \param pStreams          [in] Logical stream ids of SETUP streams.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if NULL
 *                  rate selection info object or if no streams SETUP,
 *                  HXR_IGNORE if not able to  subscribe to rules for all streams.
 *****************************************************************************/

HX_RESULT
CStreamSelector::HandleStreamLinkChar(IHXRateSelectionInfo* pRateSelInfo,
                                      const UINT16 uiNumStreams,
                                      const UINT16*& pStreams)
{
    HX_RESULT hr = HXR_IGNORE;
    UINT16 uiNumStreamLinkChars = 0;
    UINT32 ulStreamGroupId = 0;

    if (!pRateSelInfo || uiNumStreams < 1 || !pStreams)
    {
        HX_ASSERT(!"Invalid params caught in CStreamSelector::HandleStreamLinkChar() - should be caught earlier!");
        return HXR_INVALID_PARAMETER;
    }

    IHXRateDescEnumerator* pLogicalStreamEnum = NULL;
    IHXRateDescription* pRateDesc = NULL;

    for (UINT16 i = 0; i < uiNumStreams; i++)
    {
        UINT32 ulGBW = 0;

        if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_LINKCHAR_GBW, pStreams[i], ulGBW))
        &&  ulGBW)
        {
            if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
            {
                fprintf(stderr, "Initial Rate Selection: Stream %u using Link-Char GBW: %u\n", i, ulGBW);
                fflush(stderr);
            }

            if (FAILED(m_pUberMgr->GetLogicalStream(pStreams[i],
                                                    pLogicalStreamEnum)))
            {
                HX_RELEASE(pLogicalStreamEnum);
                return HXR_UNEXPECTED;
            }

            if (FAILED(pRateSelInfo->GetInfo(RSI_STREAMGROUPID,
                                            pStreams[i],
                                            ulStreamGroupId)))
            {
                HX_ASSERT(!"RateSelInfo fatal error, could not get stream group id!");
                HX_RELEASE(pLogicalStreamEnum);
                continue;
            }

            if (FAILED(pLogicalStreamEnum->FindRateDescByClosestAvgRate(ulGBW,
                                                                        TRUE,
                                                                        FALSE,
                                                                        pRateDesc))
            &&  FAILED(pLogicalStreamEnum->FindRateDescByMidpoint(ulGBW,
                                                                  TRUE,
                                                                  FALSE,
                                                                  pRateDesc)))
            {
                // Badness!
                continue;
            }

            if (pRateDesc)
            {
                if (SUCCEEDED(m_pUberMgr->SetStreamGroupRateDesc(ulStreamGroupId,
                                                                 pStreams[i],
                                                                 pRateDesc,
                                                                 NULL)))
                {
                    uiNumStreamLinkChars++;
                }
                HX_RELEASE(pRateDesc);
            }

            HX_RELEASE(pLogicalStreamEnum);
        }
    }

    // If we get Link-Char for less streams than we have SETUP, then
    // other lower-priority aggregate criteria may override the subscribes.
    if (uiNumStreamLinkChars == uiNumStreams)
    {
        hr = HXR_OK;
    }

    return hr;
}


/******************************************************************************
 * \brief HandleStreamRegistration - Handle rate selection based on SETUP id
 *                                   default avg bitrate.
 *
 * \param pRateSelInfo      [in] Object with rate selection criteria info.
 * \param uiNumStreams      [in] Number of SETUP streams.
 * \param pStreams          [in] Logical stream ids of SETUP streams.
 *
 * \return          HXR_OK if successful, HXR_INVALID_PARAMETER if NULL
 *                  rate selection info object or if no streams SETUP,
 *                  HXR_UNEXPECTED if unexpected errors occur.
 *****************************************************************************/

HX_RESULT
CStreamSelector::HandleStreamRegistration(IHXRateSelectionInfo* pRateSelInfo,
                                          const UINT16 uiNumStreams,
                                          const UINT16*& pStreams)
{
    UINT32 ulDefaultRule = 0;
    UINT32 ulAvgBitrate = 0;
    UINT32 ulTrackId =  0;
    UINT32 ulStreamGroupId = 0;
    BOOL bDefaultStream = 0;
    IHXRateDescEnumerator* pLogicalStreamEnum = NULL;
    IHXRateDescription* pRateDesc = NULL;

    if (!pRateSelInfo || uiNumStreams < 1 || !pStreams)
    {
        HX_ASSERT(!"Invalid params caught in CStreamSelector::HandleStreamLinkChar() - should be caught earlier!");
        return HXR_INVALID_PARAMETER;
    }

    for (UINT16 i = 0; i < uiNumStreams; i++)
    {

        // These should never fail, even if they return 0.

        if (FAILED(pRateSelInfo->GetInfo(RSI_DEFAULT_RULE,
                                         pStreams[i],
                                         ulDefaultRule)))
        {
            HX_ASSERT(!"RateSelInfo fatal error, could not get default rule!");
            return HXR_UNEXPECTED;
        }

        if (FAILED(pRateSelInfo->GetInfo(RSI_AVGBITRATE,
                                         pStreams[i],
                                         ulAvgBitrate)))
        {
            HX_ASSERT(!"RateSelInfo fatal error, could not get avg bitrate!");
            return HXR_UNEXPECTED;
        }

        if (FAILED(pRateSelInfo->GetInfo(RSI_STREAMGROUPID,
                                         pStreams[i],
                                         ulStreamGroupId)))
        {
            HX_ASSERT(!"RateSelInfo fatal error, could not get stream group id!");
            return HXR_UNEXPECTED;
        }

        if (FAILED(m_pUberMgr->GetLogicalStream(pStreams[i],
                                                pLogicalStreamEnum)))
        {
            HX_RELEASE(pLogicalStreamEnum);
            return HXR_UNEXPECTED;
        }

        if (ulDefaultRule != INVALID_RULE_NUM
            && SUCCEEDED(pLogicalStreamEnum->FindRateDescByRule(ulDefaultRule,
                                                                TRUE,
                                                                FALSE,
                                                                pRateDesc)))
        {
            if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
            {
                fprintf(stderr, "Initial Rate Selection: Subscribe strm: %u default rule: %u\n", pStreams[i], ulDefaultRule);
                fflush(stderr);
            }
        }
        else if (ulAvgBitrate)
        {
            if (SUCCEEDED(pLogicalStreamEnum->FindRateDescByClosestAvgRate(ulAvgBitrate,
                                                                        TRUE,
                                                                        FALSE,
                                                                        pRateDesc))
            ||  SUCCEEDED(pLogicalStreamEnum->FindRateDescByMidpoint(ulAvgBitrate,
                                                                    TRUE,
                                                                    FALSE,
                                                                    pRateDesc)))
            {
                if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
                {
                    fprintf(stderr, "Initial Rate Selection: Using strm: %u default avgbitrate: %u\n", pStreams[i], ulAvgBitrate);
                    fflush(stderr);
                }
            }
        }

        if (pRateDesc)
        {
            if (SUCCEEDED(m_pUberMgr->SetStreamGroupRateDesc(ulStreamGroupId, 
                                                             pStreams[i], 
                                                             pRateDesc, 
                                                             NULL)))
            pRateDesc->Release();
        }
        HX_RELEASE(pLogicalStreamEnum);
    }

    return HXR_OK;
}

/******************************************************************************
 * \brief SetInitialRateDesc - Set initial rate desc to specified rate (approx).
 *
 * \param ulRate        [in] Rate to set aggregate rate desc to.
 *
 * \return          HXR_OK if successful, or failure result from UberMgr.
 *****************************************************************************/

HX_RESULT
CStreamSelector::SetInitialRateDesc(UINT32 ulRate)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(!"StrmSelector expected uber mgr!\n");
        return HXR_FAIL;
    }


    // Find closest rate (lower or equal) to the initial target rate -- must be selectable
    IHXRateDescription* pBandwidthGrouping = NULL;

    res = m_pUberMgr->FindRateDescByClosestAvgRate(ulRate,
                                                   TRUE,
                                                   FALSE,
                                                   pBandwidthGrouping);

    // Just take the first rate available for selection
    if (FAILED(res))
    {
        res = m_pUberMgr->GetSelectableRateDescription(0, pBandwidthGrouping);
    }

    if (SUCCEEDED(res))
    {
        res = m_pUberMgr->SetAggregateRateDesc(pBandwidthGrouping, NULL);
    }

    HX_RELEASE(pBandwidthGrouping);

    HX_ASSERT(SUCCEEDED(res));

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::GetSelectionParams
// Purpose:
//  Determine stream selection params -- populates m_selectionParam
HX_RESULT
CStreamSelector::GetSelectionParams()
{
    HX_ASSERT(!m_selectionParam.m_ulMaxTargetRate);
    HX_ASSERT(!m_selectionParam.m_ulMaxPreDecBufSize);

    HX_RESULT theErr = HXR_FAIL;
    IHXClientProfileInfo* pProfile = NULL;
    BOOL bProfileFound = TRUE;
    BOOL bConfigFound = TRUE;
    UINT32 ulMaxRateConfig = 0;     // max rate from config file
    UINT32 ulPreDecodeBufSize = 0;  // profile if LocalRDF is provided else config
    UINT32 ulDecodeByteRate = 0;    // profile if LocalRDF is provided else config
    UINT32 ulPostDecBufPeriod = 0;  // profile if LocalRDF is provided else zero

    if(m_pQoSConfig)
    {
        if(FAILED(m_pQoSConfig->GetConfigInt(QOS_CFG_CC_CE_VPREDEC_BUF_SIZE, (INT32&)ulPreDecodeBufSize)))
        {
            ulPreDecodeBufSize = DEFAULT_VPREDEC_BUF_SIZE; //Set the default value
        }
        if(SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_CC_CE_VDEC_BYTE_RATE, (INT32&)ulDecodeByteRate)))
        {
            if(ulDecodeByteRate < MIN_VDEC_BYTE_RATE && ulDecodeByteRate != 0)
            {
                ulDecodeByteRate = MIN_VDEC_BYTE_RATE;    //Default minimum value
            }
            // unit conversion: bitspersec
            ulDecodeByteRate *= 8;
       }
    }

    if (m_pStats)
    {
    theErr = m_pStats->GetClientProfileInfo(pProfile);
    }

    // Get stream selection params (pre/decode buffer size, etc.) from client profile
    if (HXR_OK == theErr)
    {
        HXCPAttribute profAttribute;
    UINT32 attribtype = HX_CP_TYPE_INT;

        profAttribute.ulInt = 0;
        if (SUCCEEDED(pProfile->GetAttribute(HX_CP_ATTR_VPREDEC_BUF_SIZE,
            attribtype, profAttribute)))
        {
            //  better be >= preroll * avgbitrate
            ulPreDecodeBufSize = profAttribute.ulInt;
        }

        profAttribute.ulInt = 0;
        if (SUCCEEDED(pProfile->GetAttribute(HX_CP_ATTR_VDEC_BYTE_RATE,
            attribtype, profAttribute)))
        {
            //Video Decoding Byte Rate is in Bytes/sec
            ulDecodeByteRate = profAttribute.ulInt;
            // unit conversion: bitspersec
            ulDecodeByteRate *= 8;
        }

        profAttribute.ulInt = 0;
        if (SUCCEEDED(pProfile->GetAttribute(HX_CP_ATTR_VPOSTDEC_BUF_PERIOD,
            attribtype, profAttribute)))
        {
            ulPostDecBufPeriod = profAttribute.ulInt;
        }

    attribtype = HX_CP_TYPE_BOOL;
        profAttribute.bBool = 0;
        if (SUCCEEDED(pProfile->GetAttribute(HX_CP_ATTR_SOUND_CAP, attribtype,
            profAttribute)))
        {
        /* */
        }
        if (SUCCEEDED(pProfile->GetAttribute(HX_CP_ATTR_IMAGE_CAP, attribtype,
            profAttribute)))
        {
        /* */
        }

        HX_RELEASE(pProfile);
    }
    else
    {
    // fine
    bProfileFound = FALSE;
    theErr = HXR_OK;
    }

    if (m_pQoSConfig)
    {
    INT32 lTemp = 0;
    if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_RC_MAX_SENDRATE, lTemp)))
    {
        ulMaxRateConfig = (UINT32)lTemp;
        }
    }
    else
    {
    bConfigFound = FALSE;
    }


    /*
     * find m_selectionParam.m_ulMaxTargetRate
     */
    if (ulMaxRateConfig && ulDecodeByteRate)
    {
    m_selectionParam.m_ulMaxTargetRate = MIN(ulMaxRateConfig, ulDecodeByteRate);
    }
    else if (ulDecodeByteRate)
    {
    m_selectionParam.m_ulMaxTargetRate = ulDecodeByteRate;
    }
    else if (ulMaxRateConfig)
    {
    m_selectionParam.m_ulMaxTargetRate = ulMaxRateConfig;
    }
    else
    {
    // sky is the limit!
    m_selectionParam.m_ulMaxTargetRate = MAX_UINT32;
    }

    //BufferUsageLimit - Sets the upper limit on the percentage of the Client’s 
    //buffer the Server will attempt to fill.  The actual usage state of the client 
    //buffer is not known exactly by the Server.  This provides a margin of safety 
    //to prevent the Server from overflowing the Client buffer. [Default value - 98%]
    INT32 lBufferUsageLimit = DEF_BUF_USAGE_LIMIT;
    INT32 lTemp = 0;
    IHXBuffer* pIHXBuff = NULL;
    if (SUCCEEDED(m_pQoSConfig->GetConfigBuffer(QOS_CFG_CC_BUF_USAGE_LIMIT, pIHXBuff))
        && pIHXBuff && pIHXBuff->GetBuffer())
    {
        lBufferUsageLimit = (atoi((const char*) pIHXBuff->GetBuffer()));
    }
    else if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_CC_BUF_USAGE_LIMIT, lTemp)))
    {
        lBufferUsageLimit = (INT32)lTemp;
    }
    HX_RELEASE(pIHXBuff);

    //Valid value for ulBufferUsageLimit is (0-100)
    if (lBufferUsageLimit < 0 || lBufferUsageLimit > 100)
    {
    lBufferUsageLimit = DEF_BUF_USAGE_LIMIT;
    }

    //Multiply ulPreDecodeBufSize by ulBufferUsageLimit/100 to set the actual buffer size with 
    //usage limit
    ulPreDecodeBufSize = ulPreDecodeBufSize * lBufferUsageLimit / 100;

    /*
     * if ulPreDecodeBufSize == 1, it'll depend on a media rate, which we do not
     * know right now.  it will be calculated once a stream is chosen.
     */
    if (ulPreDecodeBufSize)
    {
    m_selectionParam.m_ulMaxPreDecBufSize = ulPreDecodeBufSize;
    }
    else
    {
    m_selectionParam.m_ulMaxPreDecBufSize = MAX_UINT32;
    }

    m_selectionParam.m_ulPostDecBufPeriod = ulPostDecBufPeriod;

    HX_ASSERT(m_selectionParam.m_ulMaxTargetRate);
    HX_ASSERT(m_selectionParam.m_ulMaxPreDecBufSize);

    if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
    {
        printf("***** Stream Selection Parameters *****\n");
        if (!bProfileFound)
        {
            printf("Client Profile Not Found\n");
        }
        if (!bConfigFound)
        {
            printf("QoS Config Not Found\n");
        }
        printf("InitTargetRate:%6u\n", m_selectionParam.m_ulInitTargetRate);
        printf("MaxTargetRate: %6u\n", m_selectionParam.m_ulMaxTargetRate);
        printf("MaxPreDecBuf : %6u\n", m_selectionParam.m_ulMaxPreDecBufSize);
        fflush(0);
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::LogClientError
// Purpose:
//  Logs client error
HX_RESULT
CStreamSelector::LogClientError()
{
    if (!m_pErrorMsg)
    {
    return HXR_FAIL;
    }

    // No streams were appropriate for the client.  Log the error.

    UINT32 ulMsgLen = 512;
    IHXBuffer* pURL = NULL;
    IHXBuffer* pClient = NULL;

    if (m_pStats)
    {
    pURL = m_pStats->GetURL();

    IHXClientStats* pClientStats = m_pStats->GetClient();
    if (pClientStats)
    {
        pClient = pClientStats->GetIPAddress();
        pClientStats->Release();
    }

    if (pURL)
    {
        ulMsgLen += pURL->GetSize();
    }
    if (pClient)
    {
        ulMsgLen += pClient->GetSize();
    }
    }

    NEW_FAST_TEMP_STR(szErrStr, 1024, ulMsgLen);
    sprintf(szErrStr, STRM_SELECT_ERR_STR,
    pURL ? (char*)pURL->GetBuffer() : "<Unknown>",
    pClient ? (char*)pClient->GetBuffer() : "<Unknown>");
    m_pErrorMsg->Report(HXLOG_INFO, HXR_OK, 0, szErrStr, NULL);

    DELETE_FAST_TEMP_STR(szErrStr);
    HX_RELEASE(pURL);
    HX_RELEASE(pClient);

    return HXR_OK;
}

// Excludes any Bandwidth groupings from selection & swithching if they contain
// this stream
HX_RESULT
CStreamSelector::ExcludeBandwidthGroupings(UINT16 unStreamNum, UINT32 ulRate)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    UINT32 ulStreamGroupNum = 0;
    res = m_pUberMgr->FindStreamGroupByLogicalStream(unStreamNum, ulStreamGroupNum);

    UINT32 ulNumRateDescriptions = m_pUberMgr->GetNumRateDescriptions();
    UINT32 ulNumExcludedRateDescriptions = 0;
    
    for (UINT32 i = 0; i < ulNumRateDescriptions && SUCCEEDED(res); i++)
    {
        IHXRateDescription* pBandwidthGrouping = NULL;
        UINT32* aulBandwidthAllocation = NULL;
        res = m_pUberMgr->GetRateDescription(i, pBandwidthGrouping);

        if (SUCCEEDED(res))
        {
            aulBandwidthAllocation = pBandwidthGrouping->GetBandwidthAllocationArray();
        }

        if (aulBandwidthAllocation &&
            aulBandwidthAllocation[ulStreamGroupNum] == ulRate)
        {
            pBandwidthGrouping->ExcludeFromSelection(TRUE, HX_SEL_INADEQUATE_CLIENT_CAPABILITIES);
            pBandwidthGrouping->ExcludeFromSwitching(TRUE, HX_SWI_INADEQUATE_CLIENT_CAPABILITIES);
            ulNumExcludedRateDescriptions++;
        }

        HX_RELEASE(pBandwidthGrouping);
    }

    if (ulNumRateDescriptions == ulNumExcludedRateDescriptions)
    {
        res = HXR_NOTENOUGH_PREDECBUF;
        LogClientError();
    }

    return res;
}

HX_RESULT
CStreamSelector::VerifyMaxTargetRate(void)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    UINT32 ulNumRateDescriptions = m_pUberMgr->GetNumRateDescriptions();
    UINT32 ulNumExcludedRateDescriptions = 0;

    for (UINT32 i = 0; i < ulNumRateDescriptions && SUCCEEDED(res); i++)
    {
        IHXRateDescription* pBandwidthGrouping = NULL;
        res = m_pUberMgr->GetRateDescription(i, pBandwidthGrouping);

        if (SUCCEEDED(res))
        {
            UINT32 ulRate = 0;
            res = pBandwidthGrouping->GetAvgRate(ulRate);
            if (SUCCEEDED(res) && m_selectionParam.m_ulMaxTargetRate < ulRate)
            {
                pBandwidthGrouping->ExcludeFromSelection(TRUE, HX_SEL_INADEQUATE_CLIENT_CAPABILITIES);
                pBandwidthGrouping->ExcludeFromSwitching(TRUE, HX_SWI_INADEQUATE_CLIENT_CAPABILITIES);
                ulNumExcludedRateDescriptions++;
            }
        }

        HX_RELEASE(pBandwidthGrouping);
    }

    if (ulNumRateDescriptions == ulNumExcludedRateDescriptions)
    {
        res = HXR_NOTENOUGH_BANDWIDTH;
        LogClientError();
    }

    return res;
}

