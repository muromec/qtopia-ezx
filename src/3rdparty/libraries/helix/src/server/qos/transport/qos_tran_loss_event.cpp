/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_loss_event.cpp,v 1.4 2003/10/07 18:55:52 damonlan Exp $ 
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
#include "hxtypes.h"
#include "hxresult.h"
#include "string.h"
#include "qos_tran_loss_event.h"

#include <float.h>
#include <math.h>

#define ACKED_PKTS_PER_ACK           1

LossEventHistory::LossEventHistory () :
  m_cLossHistorySize (QOS_CC_LOSS_HIST_SZ),
  m_cCurrentLossWindow (0),
  m_pLossEventHistory (NULL),
  m_pWeights (NULL),
  m_pDiscountFactor (NULL),
  m_fGeneralDiscountFactor (0)
{
    Init();
}

LossEventHistory::LossEventHistory (UINT8 cHistorySize)  :
  m_cLossHistorySize (cHistorySize),
  m_cCurrentLossWindow (0),
  m_pLossEventHistory (NULL),
  m_pWeights (NULL),
  m_pDiscountFactor (NULL),
  m_fGeneralDiscountFactor (0)
{
    Init();
}

LossEventHistory::~LossEventHistory ()
{
    HX_VECTOR_DELETE(m_pLossEventHistory);
    HX_VECTOR_DELETE(m_pWeights);
    HX_VECTOR_DELETE(m_pDiscountFactor);
}

void
LossEventHistory::Init()
{
    m_pLossEventHistory = new UINT16 [m_cLossHistorySize];
    m_pWeights = new double [m_cLossHistorySize];
    m_pDiscountFactor = new double [m_cLossHistorySize];

    double fWeightDelta = 1.0 / ((m_cLossHistorySize+1) / 2 + 1);
    
    for (UINT8 i = 0; i < m_cLossHistorySize/2; ++i)
    {
	m_pWeights [i] = 1.0;
    }
    
    for (UINT8 j =  m_cLossHistorySize/2; j < m_cLossHistorySize; ++j)
    {
	m_pWeights [j] = 
	    m_pWeights [j-1] - fWeightDelta;
    }

    memset(m_pLossEventHistory, 0, sizeof(UINT16) * m_cLossHistorySize);
    memset(m_pDiscountFactor, 1, sizeof(double) * m_cLossHistorySize);
}

HX_RESULT 
LossEventHistory::AcknowledgePackets (UINT16 unACKCount)
{
    m_pLossEventHistory [m_cCurrentLossWindow] += unACKCount;
    return HXR_OK;
}

HX_RESULT
LossEventHistory::OnNewInterval ()
{
    (++m_cCurrentLossWindow) %= m_cLossHistorySize;
    m_pLossEventHistory [m_cCurrentLossWindow] = 0;
    return HXR_OK;
}

HX_RESULT
LossEventHistory::ResetCurrentInterval ()
{
    m_pLossEventHistory [m_cCurrentLossWindow] = 0;
    return HXR_OK;
}

HX_RESULT
LossEventHistory::ResetHistory ()
{
    memset(m_pLossEventHistory, 0, sizeof(UINT16) * m_cLossHistorySize);
    return HXR_OK;
}

HX_RESULT 
LossEventHistory::GetLossEventRate (double& /*OUT*/ fLossRate,
				    BOOL bUseDiscounting)
{
    double fIntervalAvg_0     = 0;
    double fWeightTotal_0     = 0;
    
    double fIntervalAvg_1     = 0;
    double fWeightTotal_1     = 0;

    double fCurrentInterval   = 0;

    UINT16 i                  = 0;

    /* compute loss interval without current interval */
    for (i = 0; i < m_cLossHistorySize; i++)
    {
	if (m_cCurrentLossWindow != ((m_cCurrentLossWindow - i + m_cLossHistorySize)%
				      (m_cLossHistorySize)) )
	{
	    fCurrentInterval = 
		(double)m_pLossEventHistory [(m_cCurrentLossWindow - i + m_cLossHistorySize)%
					 (m_cLossHistorySize)];
	    
	    if (fCurrentInterval > 0)
	    {
		fWeightTotal_0   += m_pWeights[i];
		fIntervalAvg_0   += fCurrentInterval * m_pWeights[i];
	    }
	}
    }
    
    fIntervalAvg_0 /= fWeightTotal_0;
    
    /* compute loss interval with current interval */
    for (i = 1; i < m_cLossHistorySize+1; i++)
    {
	fCurrentInterval = 
	    m_pLossEventHistory [(m_cCurrentLossWindow - i + m_cLossHistorySize + 1)%
			     (m_cLossHistorySize)];

	if (fCurrentInterval > 0)
	{
	    fWeightTotal_1   += m_pWeights[i-1];
	    fIntervalAvg_1   += fCurrentInterval * m_pWeights[i-1];
	}
    }

    fIntervalAvg_1 /= fWeightTotal_1;
    
    fLossRate = (1.0 / max(fIntervalAvg_0, fIntervalAvg_1));
    return HXR_OK;
}

HX_RESULT 
LossEventHistory::EstimateLossEventRate (double& /*OUT*/ fLossRate, 
					 double fThroughput,
					 double fAvgPktSize,
					 double fRTTEstimate,
					 double fRTOEstimate)
{
    double fLoss1 = 1.0;
    double fLoss2 = DBL_EPSILON;
    double fLoss3 = DBL_EPSILON;
    
    double fBitrate1 = ThroughputEquation (fAvgPktSize, fLoss1, 
					   fRTTEstimate, fRTOEstimate);
    double fBitrate2 = ThroughputEquation (fAvgPktSize, fLoss2, 
					   fRTTEstimate, fRTOEstimate);
    double fBitrate3 = fBitrate2;
    UINT16 count     = 100;


    if (fBitrate1 > fThroughput)
    {
	fLossRate = 1.0;
	return HXR_OK;
    }

    if (fBitrate2 < fThroughput)
    {
	fLossRate = 0.0;
	return HXR_OK;
    }
    
    while (--count)
    {
	fLoss2 = fLoss1 + (fLoss3 - fLoss1) / 2;

	fBitrate2 =  ThroughputEquation (fAvgPktSize, fLoss2, 
					 fRTTEstimate, fRTOEstimate);

	if (fBitrate2 > fThroughput + 0.5)
	{
	    fLoss3 = fLoss2;
	    fBitrate3 = fBitrate2;
	}
	else if (fBitrate2 < fThroughput - 0.5)
	{
	    fLoss1 = fLoss2;
	    fBitrate1 = fBitrate2;
	}
	else
	{
	    break;
	}
    }

    fLossRate = fLoss2;
    return HXR_OK;
}

double
LossEventHistory::ThroughputEquation(double fAvgPktSize,
				     double fLossEventRate,
				     double fRTTEstimate,
				     double fRTOEstimate)
{
    //from draft-ietf-tsvwg-tfrc-03
    double fRTT_seconds = fRTTEstimate / 1000;
    double fRTO_seconds = fRTOEstimate / 1000;
    double fTemp        = min(3 * sqrt(3*ACKED_PKTS_PER_ACK*fLossEventRate/8), 
			      1.0);
    return ( fAvgPktSize /
	     
	     (fRTT_seconds * 
	      sqrt(2*ACKED_PKTS_PER_ACK*fLossEventRate/3) +
	      fRTO_seconds   * 
	      fTemp          * 
	      fLossEventRate * 
	      (1 + 32 * fLossEventRate * fLossEventRate)));
}
