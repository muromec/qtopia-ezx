/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsmutil.cpp,v 1.5 2006/05/04 00:12:50 milko Exp $
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

/****************************************************************************
 *  Includes
 */
#include "hxsmutil.h"
#include "hxasm.h"

#include "hxbuffer.h"	/* CHXBuffer */
#include "chxpckts.h"	/* CHXHeader */
#include "safestring.h" /* SafeSprintf */
#include "asmrulep.h"	/* ASMRuleBook */
#include "pckunpck.h"

static 
HX_RESULT AddVarBuffer(IUnknown* pContext, IHXValues* pVars, const char* pVarName)
{
    HX_ASSERT(pContext);

    HX_RESULT res = HXR_OUTOFMEMORY;

    IHXBuffer* pBuf = NULL;
    CreateBufferCCF(pBuf, pContext);
    if (pBuf)
    {
	pBuf->SetSize(128);
	res = pVars->SetPropertyCString(pVarName,  pBuf);
	HX_RELEASE(pBuf);
    }

    return res;
}

HX_RESULT HXSMUpdateSubscriptionVars(IUnknown* pContext,
				     REF(IHXValues*) pVars,
				     UINT32 ulBandwidth,
				     HXBOOL bLoss,
				     float loss)
{
    HX_RESULT res = HXR_OK;

    if (!pVars)
    {
	CreateValuesCCF(pVars, pContext);
	if (pVars)
	{
	    res = AddVarBuffer(pContext, pVars, "Bandwidth");
	    if (HXR_OK == res && bLoss)
	    {
		res = AddVarBuffer(pContext, pVars, "PacketLoss");
	    }
	}
	else
	{
	    res = HXR_OUTOFMEMORY;
	}
    }

    if (HXR_OK == res)
    {
	IHXBuffer* pBandwidthBuffer = NULL;
	
	res = pVars->GetPropertyCString("Bandwidth", pBandwidthBuffer);

        if (pBandwidthBuffer)
	{
	    SafeSprintf ((char *)pBandwidthBuffer->GetBuffer(), 
			 pBandwidthBuffer->GetSize(), "%ld", ulBandwidth);

	    HX_RELEASE(pBandwidthBuffer);
	}

	if (HXR_OK == res && bLoss)
	{
	    IHXBuffer* pLossBuffer = NULL;

	    res = pVars->GetPropertyCString("PacketLoss", pLossBuffer);

	    if (pLossBuffer)
	    {
		SafeSprintf ((char *)pLossBuffer->GetBuffer(), 
			     pLossBuffer->GetSize(), "%0.3f", loss);
		HX_RELEASE(pLossBuffer);
	    }
	}
    }

    return res;
}

HX_RESULT HXASMGetBwThresholdInfo(IUnknown* pContext,
				  IHXValues* pHeader,
				  float* pThreshold, 
				  REF(UINT32) ulNumThreshold)
{
    IHXBuffer* pASMMasterRuleBookBuffer = NULL;
    HX_RESULT retVal = HXR_OK;

    if (pHeader == NULL)
    {
	retVal = HXR_FAIL;
    }

    if (SUCCEEDED(retVal))
    {
	pHeader->GetPropertyCString(
	    "ASMRuleBook", 
	    pASMMasterRuleBookBuffer);

	retVal = HXR_FAIL;
	
	if (pASMMasterRuleBookBuffer)
	{
	    IHXValues* pSubsVars = NULL;
	    ASMRuleBook *pMasterRuleBook = NULL;

	    retVal = HXR_OK;

	    if (SUCCEEDED(retVal))
	    {
		retVal = HXR_OUTOFMEMORY;

		pMasterRuleBook = new ASMRuleBook(pContext,
		    (char*) pASMMasterRuleBookBuffer->GetBuffer());
		if (pMasterRuleBook)
		{
		    retVal = HXR_OK;
		}
	    }

	    if (SUCCEEDED(retVal) && pThreshold)
	    {
		retVal = HXSMUpdateSubscriptionVars(
			    pContext,
			    pSubsVars, 
			    0,		// Bandwidth - must be zero for free var.
			    FALSE,	// Account for loss
			    0.0);	// Loss

		if (SUCCEEDED(retVal))
		{
		    retVal = pMasterRuleBook->GetPreEvaluate(
				pThreshold, 
				ulNumThreshold,
				pSubsVars, 
				"Bandwidth");
		}
	    }
	    else if (SUCCEEDED(retVal))
	    {
		ulNumThreshold = pMasterRuleBook->GetNumThresholds();
	    }

	    HX_DELETE(pMasterRuleBook);
	    HX_RELEASE(pSubsVars);
	}

	HX_RELEASE(pASMMasterRuleBookBuffer);
    }

    return retVal;
}

HX_RESULT HXASMGetSourceBwThresholdInfo(IUnknown* pContext,
					IHXValues** pSourceHeaders,
					UINT16 uNumHeaders,
					float* pThreshold, 
					REF(UINT32) ulNumThreshold)
{
    HX_RESULT retVal = HXR_OK;

    if ((pSourceHeaders == NULL) ||
	(uNumHeaders == 0) ||
	(pSourceHeaders[0] == NULL))
    {
	retVal = HXR_FAIL;
    }

    if (SUCCEEDED(retVal))
    {
	// Try to obtain thresholds from file (master) header
	retVal = HXASMGetBwThresholdInfo(pContext,
					 pSourceHeaders[0],
					 pThreshold,
					 ulNumThreshold);
				
	// If file header could not provide treashold info, use
	// use stream headers directly.
	if (FAILED(retVal))
	{
	    float** pPerStreamThreashold = NULL;
	    UINT32* pulPerStreamNumThreshold = NULL;
	    UINT16 uMaxThresholdsStrmIdx = 0;
	    UINT32 ulNumStreams = 0;
	    UINT16 uStrmIdx = 0;

	    // Find number of streams
	    pSourceHeaders[0]->GetPropertyULONG32("StreamGroupCount", 
						  ulNumStreams);
	    if (ulNumStreams == 0)
	    {
		pSourceHeaders[0]->GetPropertyULONG32("StreamCount", 
						      ulNumStreams);
	    }

	    if (ulNumStreams != 0)
	    {
		retVal = HXR_OK;
	    }

	    // Allocate storage to keet track of thresholds for each stream
	    if (SUCCEEDED(retVal))
	    {
		pPerStreamThreashold = new float* [ulNumStreams];
		pulPerStreamNumThreshold = new UINT32 [ulNumStreams];

		retVal = HXR_OUTOFMEMORY;
		if (pPerStreamThreashold && pulPerStreamNumThreshold)
		{
		    memset(pPerStreamThreashold, 0, ulNumStreams * sizeof(float*));
		    memset(pulPerStreamNumThreshold, 0, ulNumStreams * sizeof(UINT32));
		    retVal = HXR_OK;
		}
	    }

	    // Obtain per stream threasholds
	    if (SUCCEEDED(retVal))
	    {
		UINT16 uHdrIdx;
		UINT32 ulStreamNumber;

		for (uHdrIdx = 1; 
		     SUCCEEDED(retVal) && (uHdrIdx < uNumHeaders); 
		     uHdrIdx++)
		{
		    retVal = pSourceHeaders[uHdrIdx]->GetPropertyULONG32(
				"StreamGroupNumber", 
				ulStreamNumber);

		    if (FAILED(retVal))
		    {
			retVal = pSourceHeaders[uHdrIdx]->GetPropertyULONG32(
				    "StreamNumber", 
				    ulStreamNumber);
		    }

		    if (SUCCEEDED(retVal) && 
			(ulStreamNumber < ulNumStreams) && 
			(pulPerStreamNumThreshold[ulStreamNumber] == 0))
		    {
			retVal = HXASMGetBwThresholdInfo(pContext,
							 pSourceHeaders[uHdrIdx],
							 NULL, 
							 pulPerStreamNumThreshold[ulStreamNumber]);

			// If pThreshold is not passed in, compute just the max output size.
			if (pThreshold)
			{
			    if (SUCCEEDED(retVal))
			    {
				retVal = HXR_OUTOFMEMORY;
				pPerStreamThreashold[ulStreamNumber] = 
				    new float [pulPerStreamNumThreshold[ulStreamNumber]];
				if (pPerStreamThreashold[ulStreamNumber])
				{
				    retVal = HXR_OK;
				}
			    }
			    
			    if (SUCCEEDED(retVal))
			    {
				retVal = HXASMGetBwThresholdInfo(pContext,
								 pSourceHeaders[uHdrIdx],
								 pPerStreamThreashold[ulStreamNumber], 
								 pulPerStreamNumThreshold[ulStreamNumber]);
			    }
			}

			// Keep track of stream with most thresholds.
			if (SUCCEEDED(retVal))
			{
			    if (pulPerStreamNumThreshold[uMaxThresholdsStrmIdx] < 
				pulPerStreamNumThreshold[ulStreamNumber])
			    {
				uMaxThresholdsStrmIdx = (UINT16) ulStreamNumber;
			    }	
			}
		    }
		}
	    }

	    // Formulate the united threashold list for the source using
	    // the longest stream thresholds list as a template.
	    if (SUCCEEDED(retVal) && pThreshold)
	    {
		retVal = HXR_INVALID_PARAMETER;

		if (ulNumThreshold >= pulPerStreamNumThreshold[uMaxThresholdsStrmIdx])
		{
		    ulNumThreshold = pulPerStreamNumThreshold[uMaxThresholdsStrmIdx];
		    retVal = HXR_OK;
		}

		if (SUCCEEDED(retVal))
		{
		    HXBOOL bLastThreshold = FALSE;
		    UINT32 ulThreasholdIdx = 0;

		    while (pulPerStreamNumThreshold[uMaxThresholdsStrmIdx])
		    {
			ulThreasholdIdx = pulPerStreamNumThreshold[uMaxThresholdsStrmIdx] - 1;
			bLastThreshold = (ulThreasholdIdx <= 1);

			pThreshold[ulThreasholdIdx] = 0;

			for (uStrmIdx = 0; uStrmIdx < ulNumStreams; uStrmIdx++)
			{
			    if (pulPerStreamNumThreshold[uStrmIdx] > 0)
			    {
				pThreshold[ulThreasholdIdx] +=
				    pPerStreamThreashold[uStrmIdx]
							[pulPerStreamNumThreshold[uStrmIdx] - 1];
			    }

			    if ((pulPerStreamNumThreshold[uStrmIdx] > 1) ||
				bLastThreshold)
			    {	
				pulPerStreamNumThreshold[uStrmIdx]--;
			    }
			}
		    }
		    
		    HX_ASSERT(pThreshold[0] == 0.0);
		    
		    pThreshold[0] = 0.0;
		}
	    }
	    else if (SUCCEEDED(retVal))
	    {
		ulNumThreshold = pulPerStreamNumThreshold[uMaxThresholdsStrmIdx];
	    }

	    // Clean-up
	    for (uStrmIdx = 0; uStrmIdx < ulNumStreams; uStrmIdx++)
	    {
		if (pPerStreamThreashold)
		{
		    HX_VECTOR_DELETE(pPerStreamThreashold[uStrmIdx]);
		}
	    }
	    
	    HX_VECTOR_DELETE(pPerStreamThreashold);
	    HX_VECTOR_DELETE(pulPerStreamNumThreshold);
	}
    }

    return retVal;
}

HX_RESULT HXASMSubscribeSourceToBandwidth(IUnknown* pContext,
					  IHXValues** pSourceHeaders,
					  UINT16 uNumHeaders,
					  ULONG32 ulBandwidth,
					  IUnknown* pASMProvider,
					  HXBOOL bSubscribe)
{
    IHXASMSource *pASMSource = NULL;
    IHXBuffer* pASMMasterRuleBookBuffer = NULL;
    HX_RESULT retVal = HXR_OK;

    if ((pContext == NULL) ||
	(pSourceHeaders == NULL) ||
	(pASMProvider == NULL) ||
	(uNumHeaders == 0) ||
	(pSourceHeaders[0] == NULL))
    {
	return HXR_FAIL;
    }

    retVal = pASMProvider->QueryInterface(IID_IHXASMSource, (void **) &pASMSource);

    if (SUCCEEDED(retVal))
    {
	if (pASMSource == NULL)
	{
	    retVal = HXR_FAIL;
	}
    }

    if (SUCCEEDED(retVal))
    {
	pSourceHeaders[0]->GetPropertyCString("ASMRuleBook", pASMMasterRuleBookBuffer);
	
	if (pASMMasterRuleBookBuffer)
	{
	    UINT16 idxRule;
	    IHXValues* pSubsVars = NULL;
	    UINT32 ulRuleCount = 0;
	    ASMRuleBook *pMasterRuleBook = NULL;
	    HXBOOL* pCurrentSubInfo = NULL;

	    if (SUCCEEDED(retVal))
	    {
		retVal = HXR_OUTOFMEMORY;

		pMasterRuleBook = new ASMRuleBook(pContext,
		    (char*) pASMMasterRuleBookBuffer->GetBuffer());
		if (pMasterRuleBook)
		{
		    ulRuleCount = pMasterRuleBook->GetNumRules();
		    retVal = HXR_OK;
		}
	    }

	    if (SUCCEEDED(retVal))
	    {
		pCurrentSubInfo = new HXBOOL[ulRuleCount];
		
		retVal = HXR_OUTOFMEMORY;
		if (pCurrentSubInfo)
		{
		    retVal = HXR_OK;
		}
	    }

	    // Form the subscription parameters
	    if (SUCCEEDED(retVal))
	    {
		retVal = HXSMUpdateSubscriptionVars(pContext,
						    pSubsVars, 
						    ulBandwidth, 
						    FALSE,  // TRUE if loss present
						    0.0);   // loss
	    }

	    // Set the subscription
	    if (SUCCEEDED(retVal))
	    {
		retVal = pMasterRuleBook->GetSubscription(pCurrentSubInfo, 
							  pSubsVars);
	    }
	    
	    // Extract bandwidth partitioning and execute stream subscription for
	    // partitioned bandwidth.
	    if (SUCCEEDED(retVal))
	    {
		for (idxRule = 0; 
		     idxRule < ulRuleCount; 
		     idxRule++)
		{
		    if (pCurrentSubInfo[idxRule])
		    {
			UINT32 uHdrIdx;
			UINT32 ulStrmIdx;
			IHXValues* pProps = 0;
			HX_RESULT status = HXR_OK;
			
			pMasterRuleBook->GetProperties(idxRule, pProps);
			
			for (uHdrIdx = 1; 
			     uHdrIdx < uNumHeaders; 
			     uHdrIdx++)
			{
			    UINT32 ulStreamBw;
			    
			    if (pSourceHeaders[uHdrIdx]->GetPropertyULONG32("StreamGroupNumber",
				ulStrmIdx)
				!= HXR_OK)
			    {
				status = pSourceHeaders[uHdrIdx]->GetPropertyULONG32(
				    "StreamNumber",
				    ulStrmIdx);
			    }
			    
			    if (SUCCEEDED(status))
			    {
				ulStreamBw = HXASMGetStreamBw(pProps, (UINT16) ulStrmIdx);
				
				status = HXASMSubscribeStreamToBandwidth(pContext,
									 pSourceHeaders[uHdrIdx],
									 ulStreamBw,
									 pASMProvider,
									 bSubscribe);
			    }

			    if (SUCCEEDED(retVal))
			    {
				retVal = status;
			    }
			}
			
			HX_RELEASE(pProps);
			break;
		    }
		}
	    }
	    
	    HX_RELEASE(pSubsVars);
	    HX_DELETE(pMasterRuleBook);
	    HX_VECTOR_DELETE(pCurrentSubInfo);
	}
	else
	{
	    // No master ASM Rule Book - just evenly divide the bandwidth
	    // accross the streams.
	    UINT16 uHdrIdx;
	    UINT32 ulNumStreams = 0;
	    UINT32 ulBandwidthPartition = 0;

	    pSourceHeaders[0]->GetPropertyULONG32("StreamGroupCount", ulNumStreams);

	    if (ulNumStreams == 0)
	    {
		pSourceHeaders[0]->GetPropertyULONG32("StreamCount", ulNumStreams);
	    }

	    retVal = HXR_FAIL;
	    if (ulNumStreams != 0)
	    {
		retVal = HXR_OK;
	    }

	    if (SUCCEEDED(retVal))
	    {
		ulBandwidthPartition = ulBandwidth / ulNumStreams;
	    }

	    for (uHdrIdx = 1; 
		 SUCCEEDED(retVal) && (uHdrIdx < uNumHeaders); 
		 uHdrIdx++)
	    {
		retVal = HXASMSubscribeStreamToBandwidth(pContext,
							 pSourceHeaders[uHdrIdx],
							 ulBandwidthPartition,
							 pASMProvider,
							 bSubscribe);
	    }
	}
    }

    HX_RELEASE(pASMSource);
    HX_RELEASE(pASMMasterRuleBookBuffer);

    return retVal;
}

UINT32 HXASMGetStreamBw(IHXValues* pProps, 
			UINT32 ulStreamIndex)
{
    UINT32	ulRet = 0;
    char        pTemp[128];
    IHXBuffer*	pBw = NULL;
    HX_RESULT   hxResult;
    
    sprintf(pTemp, 
	    "Stream%ldBandwidth", 
	    ulStreamIndex); /* Flawfinder: ignore */

    //If the Stream?Bandwidth property isn't found for
    //all streams don't worry about it. Not all streams
    //may have rule books.
    hxResult = pProps->GetPropertyCString(pTemp, pBw);
    if ((HXR_OK == hxResult) && pBw)
    {
	ulRet = atoi((char*) pBw->GetBuffer());
    }

    HX_RELEASE(pBw);

    return ulRet;
}

HX_RESULT HXASMSubscribeStreamToBandwidth(IUnknown* pContext,
					  IHXValues* pStreamHeader,
					  ULONG32 ulBandwidth,
					  IUnknown *pASMProvider,
					  HXBOOL bSubscribe)
{   
    IHXASMSource *pASMSource = NULL;
    ASMRuleBook *pASMRuleBook = NULL;
    IHXValues *pSubsVars = NULL;
    IHXBuffer* pASMRuleBookBuffer = NULL;
    char *pASMRuleBookText = NULL;
    HXBOOL *pSubscriptionInfo = NULL;
    UINT16 uStreamNumber = 0;
    UINT16 uRuleCount = 0;
    HX_RESULT RetVal = HXR_OK;

    if ((pContext == NULL) ||
	(pStreamHeader == NULL) ||
	(pASMProvider == NULL))
    {
	return HXR_FAIL;
    }

    RetVal = pASMProvider->QueryInterface(IID_IHXASMSource, (void **) &pASMSource);

    if (SUCCEEDED(RetVal))
    {
	if (pASMSource == NULL)
	{
	    RetVal = HXR_FAIL;
	}
    }

    if (SUCCEEDED(RetVal))
    {
        RetVal = pStreamHeader->GetPropertyCString("ASMRuleBook", pASMRuleBookBuffer);
	
	if (SUCCEEDED(RetVal))
	{
	    pASMRuleBookText = (char*) pASMRuleBookBuffer->GetBuffer();
	}
    }

    if (SUCCEEDED(RetVal))
    {
	ULONG32 ulVal = 0;

	RetVal = pStreamHeader->GetPropertyULONG32("StreamNumber", ulVal);

	uStreamNumber = (UINT16) ulVal;
    }

    if (SUCCEEDED(RetVal))
    {
	RetVal = HXR_OUTOFMEMORY;

	pASMRuleBook = new ASMRuleBook(pContext, pASMRuleBookText);
	if (pASMRuleBook != NULL)
	{
	    uRuleCount = pASMRuleBook->GetNumRules();
	    RetVal = HXR_OK;
	}
    }

    if (SUCCEEDED(RetVal))
    {
	RetVal = HXSMUpdateSubscriptionVars(pContext,
					    pSubsVars, 
					    ulBandwidth, 
					    FALSE,  // TRUE if loss present
					    0.0);   // loss
    }

    if (SUCCEEDED(RetVal))
    {
	if (uRuleCount > 0)
	{
	    pSubscriptionInfo = new HXBOOL[uRuleCount];
	}

	if (pSubscriptionInfo == NULL)
	{
	    RetVal = HXR_FAIL;
	}
    }

    if (SUCCEEDED(RetVal))
    {
	RetVal = pASMRuleBook->GetSubscription(pSubscriptionInfo, pSubsVars);
    }

    if (SUCCEEDED(RetVal))
    {
	UINT16 ui;
	HX_RESULT status;

	for (ui = 0; ui < uRuleCount; ui++)
	{
	    if (pSubscriptionInfo[ui])
	    {
		if (bSubscribe)
		{
		    status = pASMSource->Subscribe(uStreamNumber, ui);
		}
		else
		{
		    status = pASMSource->Unsubscribe(uStreamNumber, ui);
		}

		if (SUCCEEDED(RetVal))
		{
		    RetVal = status;
		}
	    }
	}
    }

    HX_VECTOR_DELETE(pSubscriptionInfo);
    HX_RELEASE(pSubsVars);
    HX_RELEASE(pASMRuleBookBuffer);
    HX_DELETE(pASMRuleBook);
    HX_RELEASE(pASMSource);

    return RetVal;
}
