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

#include "proc.h"
#include "hxassert.h"

#include "timeval.h"
#include "base_callback.h"
#include "pcktstrm.h"

#include "uberstreammgr.h"

static const char* DUMP_SELECTION = "InputSource.StaticPushSource.DumpSelection";
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
CStreamSelector::CStreamSelector(BOOL bDumpSelectionParam):
    m_pUberMgr(NULL)
    , m_pStats(NULL)
    , m_pQoSConfig(NULL)
    , m_pErrorMsg(NULL)
    , m_bDumpSelection(FALSE)
    , m_bDumpSelectionParam(bDumpSelectionParam)
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
CStreamSelector::Init(IHXUberStreamManager* pUberMgr, Process* pProc, IHXSessionStats* pStats, IHXQoSProfileConfigurator* pConfig)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (m_pUberMgr || !pUberMgr || !pProc || !pStats || !pConfig)
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

    
    res = ((IUnknown*)pProc->pc->server_context)->QueryInterface(IID_IHXErrorMessages, (void **)&m_pErrorMsg);

    INT32 lTemp;
    if (m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp) == HXR_OK)
    {
        m_bDumpSelection = (BOOL)lTemp;
    }

    if (SUCCEEDED(res))
	res = GetSelectionParams();

    HX_ASSERT(SUCCEEDED(res));
    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::SetInitTargetRate
// Purpose:
//  Set Initial Target Rate iff Config is not set
HX_RESULT
CStreamSelector::SetInitTargetRate(UINT32 ulRate, BOOL bForce)
{
    INT32 lTemp = 0;

    if (bForce)
    {
	m_selectionParam.m_ulInitTargetRate = ulRate;
    }
    else
    {    	
    	if (m_pQoSConfig->GetConfigInt(QOS_CFG_IS_DEFAULT_MEDIARATE, lTemp) == HXR_OK)
    	{
	    // 0 is valid
	    m_selectionParam.m_ulInitTargetRate = lTemp;
    	}
	else
	{        
    m_selectionParam.m_ulInitTargetRate = ulRate;
	}    	    
    }    	

    
    lTemp = 0;
    if (m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp) == HXR_OK && lTemp)
    {
        printf("Setting Initial Media Rate: %u\n", ulRate);fflush(0);
    }
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
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
//  CStreamSelector::VerifyStreams
// Purpose:
//  Uses m_selectionParam to determine which bandwidth groupings are valid or not
// Returns:
//  HXR_OK if there is at least one active stream.  Otherwise, returns HXR_FAIL.
HX_RESULT
CStreamSelector::VerifyStreams()
{
    HX_RESULT res = HXR_OK;

    HX_ASSERT(m_selectionParam.m_ulMaxTargetRate > 0);
    HX_ASSERT(m_selectionParam.m_ulMaxPreDecBufSize > 0);

    // Validate state
    if (!m_pUberMgr)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
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
	res = HXR_FAIL;

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
	if (m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp) == HXR_OK && lTemp)
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

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::SelectInitialRateDesc
// Purpose:
//  Selects an initial rate description based on m_selectionParam
HX_RESULT
CStreamSelector::SelectInitialRateDesc()
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (!m_pUberMgr)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    HX_ASSERT(m_selectionParam.m_ulInitTargetRate > 0);

    // Find closest rate (lower or equal) to the initial target rate -- must be selectable
    IHXRateDescription* pBandwidthGrouping = NULL;
    if (SUCCEEDED(res))
    {
	res = m_pUberMgr->FindRateDescByClosestAvgRate(m_selectionParam.m_ulInitTargetRate, TRUE, FALSE, pBandwidthGrouping);

	// Just take the first rate available for selection
	if (FAILED(res))
	{
	    res = m_pUberMgr->GetSelectableRateDescription(0, pBandwidthGrouping);
	}
    }

    if (SUCCEEDED(res))
	res = m_pUberMgr->SetAggregateRateDesc(pBandwidthGrouping, NULL);

    HX_RELEASE(pBandwidthGrouping);

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

HX_RESULT
CStreamSelector::SelectBetterInitialRateDesc(IHXRateSelectionInfo* pRateSelInfo)
{
    UINT32 ulValue = 0;

    if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_QUERYPARAM_IR, ulValue))
    &&  ulValue)
    {
        if (m_bDumpSelection)
        {
            fprintf(stderr, "Initial Rate Selection: Using ir=: %u\n", ulValue);
            fflush(stderr);
        }
        //SetInitialRateDesc(ulValue0;
        //return HXR_OK;
    }

    //if (RateSelectionTypes == Client or All)
    {
        UINT16 uiNumStreams = pRateSelInfo->GetNumRegisteredStreams();
        if (uiNumStreams > 0)
        {
            UINT16* pStreamIds = new UINT16[uiNumStreams];
            if (SUCCEEDED(pRateSelInfo->GetRegisteredLogicalStreamIds(uiNumStreams, pStreamIds)))
            for (UINT16 i = 0; i < uiNumStreams; i++)
            {
                if (FAILED(HandleRuleSubscriptions(pRateSelInfo, pStreamIds[i])))
                {
                    if (SUCCEEDED(HandleStreamRegistration(pRateSelInfo, pStreamIds[i])))
                    {
                        //HX_VECTOR_DELETE(pStreamIds);
                        // return HXR_OK;
                    }
                }
            }
            HX_VECTOR_DELETE(pStreamIds);
        }
    }

    // if (RateSelectionTypes == Server or All)
    {
        if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_LINKCHAR_MBW, ulValue))
        &&  ulValue)
        {
            if (m_bDumpSelection)
            {
                fprintf(stderr, "Initial Rate Selection: Using Link-Char MBW: %u\n", ulValue);
                fflush(stderr);
            }
            //SetInitialRateDesc(ulValue);
            //return HXR_OK;
        }

        if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_BANDWIDTH, ulValue))
        &&  ulValue)
        {
            if (m_bDumpSelection)
            {
                fprintf(stderr, "Initial Rate Selection: Using Bandwidth: %u\n", ulValue);
                fflush(stderr);
            }
            //SetInitialRateDesc(ulValue);
            //return HXR_OK;
        }

        INT32 lTemp = 0;
        if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_IS_DEFAULT_MEDIARATE, lTemp))
        &&  lTemp)
        {
            if (m_bDumpSelection)
            {
                fprintf(stderr, "Initial Rate Selection: DefaultMediaRate: %d\n", lTemp);
                fflush(stderr);
            }
            //SetInitialRateDesc(ulValue);
            //return HXR_OK;
        }
    }

    return HXR_OK;
    //return HXR_FAIL;
}

HX_RESULT
CStreamSelector::HandleRuleSubscriptions(IHXRateSelectionInfo* pRateSelInfo, 
                                         UINT16 ulLogicalStreamId)
{
    UINT16 uiNumRules = pRateSelInfo->GetNumSubscribedRules(ulLogicalStreamId);
    if (uiNumRules < 1)
    {
        return HXR_FAIL;
    }

    UINT16* pRules = new UINT16[uiNumRules];

    if (FAILED(pRateSelInfo->GetSubscribedRules(ulLogicalStreamId, 
                                                uiNumRules, 
                                                pRules)))
    {
        HX_VECTOR_DELETE(pRules);
        return HXR_FAIL;
    }
     
    for (UINT16 j = 0; j < uiNumRules; j++)
    {
        if (m_bDumpSelection)
        {
            fprintf(stderr, "Initial Rate Selection: Subscribe strm: %u rule: %u\n", ulLogicalStreamId, pRules[j]);
            fflush(stderr);
        }
        //m_pUberMgr->SubscribeLogicalStreamRule(ulLogicalStreamId, pRules[j], NULL);
    }

    HX_VECTOR_DELETE(pRules);

    return HXR_OK;
}


HX_RESULT
CStreamSelector::HandleStreamRegistration(IHXRateSelectionInfo* pRateSelInfo, 
                                          UINT16 ulLogicalStreamId)
{
    UINT32 ulDefaultRule = 0;
    UINT32 ulAvgBitrate = 0;
    UINT32 ulTrackId =  0;
    UINT32 ulStreamGroupId = 0;
    BOOL bDefaultStream = 0;

    // These should never fail, even if they return 0.

    if (FAILED(pRateSelInfo->GetInfo(RSI_DEFAULT_RULE,
                                     ulLogicalStreamId,
                                     ulDefaultRule)))
    {
        return HXR_FAIL;
    }

    if (FAILED(pRateSelInfo->GetInfo(RSI_AVGBITRATE,
                                     ulLogicalStreamId,
                                     ulAvgBitrate)))
    {
        return HXR_FAIL;
    }

    if (FAILED(pRateSelInfo->GetInfo(RSI_STREAMGROUPID,
                                     ulLogicalStreamId,
                                     ulStreamGroupId)))
    {
        return HXR_FAIL;
    }

    // Most clients today always SETUP default alt-stream. We shouldn't trust
    // the client in cases where this happens and should use other selection
    // criteria.
    if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_TRACKID,
                                        ulLogicalStreamId,
                                        ulTrackId)))
    {
        if (SUCCEEDED(pRateSelInfo->GetInfo(RSI_ISDEFAULTSTREAM,
                                            ulLogicalStreamId,
                                            (UINT32&)bDefaultStream))
        &&  bDefaultStream)
        {
            return HXR_IGNORE;
        }
    }


    if (ulDefaultRule != INVALID_RULE_NUM)
    {
        if (m_bDumpSelection)
        {
            fprintf(stderr, "Initial Rate Selection: Subscribe strm: %u default rule: %u\n", ulLogicalStreamId, ulDefaultRule);
            fflush(stderr);
        }
    }
    
    if (ulAvgBitrate)
    {
        if (m_bDumpSelection)
        {
            fprintf(stderr, "Initial Rate Selection: Using strm: %u default avgbitrate: %u\n", ulLogicalStreamId, ulAvgBitrate);
            fflush(stderr);
        }
    }

    /*
    if ( (ulDefaultRule != INVALID_RULE_NUM 
          && SUCCEEDED(pLogicalStreamEnum->FindRateDescByRule(ulDefaultRule, 
                                                            TRUE, 
                                                            FALSE, 
                                                            pRateDesc))) 

    ||  SUCCEEDED(pLogicalStreamEnum->FindRateDescByClosestAvgRate(ulAvgBitrate, 
                                                                   TRUE, 
                                                                   FALSE, 
                                                                   pRateDesc)) 
    ||  SUCCEEDED(pLogicalStreamEnum->FindRateDescByMidpoint(ulAvgBitrate, 
                                                             TRUE, 
                                                             FALSE, 
                                                             pRateDesc)))
    {
        if (pRateDesc)
        {
            pUberStreamManager->SetStreamGroupRateDesc(ulStreamGroupId, 
                                                       ulLogicalStreamId, 
                                                       pRateDesc, 
                                                       NULL);
            HX_RELEASE(pRateDesc);
        }
    }
    */

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamSelector::SetInitialRateDesc
// Purpose:
//  Set an initial aggregate rate desc (for use by internal selection
//  criteria). 
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
    UINT32 ulPreDecodeBufSize = 0;  // profile
    UINT32 ulDecodeByteRate = 0;    // profile
    UINT32 ulPostDecBufPeriod = 0;  // profile
    UINT32 ulMaxRateConfig = 0;	    // max rate from config file

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
    	    //	better be >= preroll * avgbitrate
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
	if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_CC_MAX_SENDRATE, lTemp)))
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

    /*
     * Initial Target Rate - 
     */
    INT32 lTemp = 0;
    if (m_pQoSConfig->GetConfigInt(QOS_CFG_IS_DEFAULT_MEDIARATE, lTemp) == HXR_OK)
    {
	SetInitTargetRate((UINT32)lTemp, TRUE);
    }
    else
    {
    	SetInitTargetRate(DEFAULT_INIT_TARGET_RATE, TRUE);
    }    	    
    
    
    HX_ASSERT(m_selectionParam.m_ulMaxTargetRate);
    HX_ASSERT(m_selectionParam.m_ulMaxPreDecBufSize);

    if (m_bDumpSelectionParam)	
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
