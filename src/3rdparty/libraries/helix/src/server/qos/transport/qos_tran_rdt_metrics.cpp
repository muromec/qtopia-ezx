/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_rdt_metrics.cpp,v 1.13 2005/08/19 01:03:10 jgordon Exp $ 
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
#include "ihxpckts.h"
#include "hxassert.h"
#include "hxtick.h"
#include "errdbg.h"
#include "debug.h"
#include "hxqosinfo.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "hxmap.h"

#include "qos_tran_rdt_metrics.h"

QoSRDTMetrics::QoSRDTMetrics () :
    m_ulStartTime (HX_GET_TICKCOUNT()), 
    m_ulPktAcc (0),
    m_ulLossAcc (0),
    m_ulLocalLossAcc(0),
    m_fLossRate (0.0),
    m_fRTT (0.0)
{
}

QoSRDTMetrics::~QoSRDTMetrics ()
{
    RDTStream* pStreamInfo = NULL;
    CHXMapLongToObj::Iterator i;
    CHXMapLongToObj::Iterator end = m_StreamMap.End();

    for(i = m_StreamMap.Begin(); i != end; ++i)
    {
        pStreamInfo = (RDTStream*)(*i);
        HX_VECTOR_DELETE(pStreamInfo->m_pPktHistory);
        delete pStreamInfo;
    }
}

void
QoSRDTMetrics::Enqueue (UINT16 unStream,
			UINT16 nSeqNo, 
			UINT32 /* bytes */ ulSize)
{
    RDTStream* pStreamInfo = NULL;

    // Add this stream if we haven't yet
    if (!m_StreamMap.Lookup(unStream, (void*&)pStreamInfo))
    {
        pStreamInfo = new RDTStream;

        pStreamInfo->m_dRate = 0;
        pStreamInfo->m_ulLastACK = m_ulStartTime;
        pStreamInfo->m_pPktHistory = new PktHistoryElem [PKT_HISTORY_SIZE];
        pStreamInfo->m_ulLastACKCount = 0;
        pStreamInfo->m_ulLastNAKCount = 0;
        pStreamInfo->m_ulNAKCount = 0;

        m_StreamMap.SetAt(unStream, pStreamInfo);
    }
    
    UINT16 index = (nSeqNo%PKT_HISTORY_SIZE);

    pStreamInfo->m_pPktHistory[index].m_ulSize = ulSize;
    pStreamInfo->m_pPktHistory[index].m_bACKd = FALSE;
    pStreamInfo->m_pPktHistory[index].m_bNAKd = FALSE;
}

void
QoSRDTMetrics::Update (UINT32 /* msec */ ulNow, 
		       UINT16 unStream,
		       double fRTT,
		       UINT16* nAckList, UINT32 ackLen,
		       UINT16* nNakList, UINT32 nakLen)
{
    RDTStream* pStreamInfo = NULL;
    if (!m_StreamMap.Lookup(unStream, (void*&)pStreamInfo) ||
        (nAckList == NULL) ||
	(nNakList == NULL))
    {
	DPRINTF(D_ERROR, ("Invalid RDT ACK data received\n"));
	return;
    }

    m_ulLossAcc += nakLen;
    m_ulLocalLossAcc += nakLen;
    m_ulPktAcc  += (nakLen + ackLen);

    m_fRTT = fRTT;
    pStreamInfo->m_ulLastACKCount = 0;
    pStreamInfo->m_ulLastNAKCount = 0;

    UINT16 unIdx = 0;
    if (ackLen)
    {
        UINT16 nDataACKd = 0;
        UINT16 nNumACKd = 0;
        
        /* Loss Metric: The computed value is useless for TFRC, but it is ok for session stats: */
        double fLocalLoss = (double)((double)m_ulLocalLossAcc / (double)(ackLen+m_ulLocalLossAcc));

        m_fLossRate = min(fLocalLoss, (double)((double)(m_ulLossAcc) / (double)(m_ulPktAcc)));

        /* Rate Metric: */
        for (UINT16 i = 0; i < ackLen; i++)
        {
            unIdx = nAckList[i]%PKT_HISTORY_SIZE;
            if (!(pStreamInfo->m_pPktHistory[unIdx].m_bACKd))
            {
                nDataACKd += (UINT16)pStreamInfo->m_pPktHistory[unIdx].m_ulSize;
                nNumACKd ++;
            }
            
            pStreamInfo->m_pPktHistory[unIdx].m_bACKd = TRUE;
        }
        
        pStreamInfo->m_dRate = ((double)(nDataACKd * 1000)/
                                (double)(ulNow - pStreamInfo->m_ulLastACK));

        pStreamInfo->m_ulLastACKCount = nNumACKd;
        pStreamInfo->m_ulLastACK = ulNow;
        m_ulLocalLossAcc = 0;
    }

    if (nakLen)
    {
        UINT16 nNumNAKd = 0;

        for (UINT16 j = 0; j < nakLen; j++)
        {
            unIdx = nNakList[j]%PKT_HISTORY_SIZE;
            if (!(pStreamInfo->m_pPktHistory[unIdx].m_bNAKd) &&
                !(pStreamInfo->m_pPktHistory[unIdx].m_bACKd))
            {
                nNumNAKd++;
            }
            
            pStreamInfo->m_pPktHistory[unIdx].m_bNAKd = TRUE;
        }

        pStreamInfo->m_ulLastNAKCount = nNumNAKd;
        pStreamInfo->m_ulNAKCount += nNumNAKd;
    }
}

HX_RESULT
QoSRDTMetrics::UpdateStatistics(IHXQoSTransportAdaptationInfo* pInfo)
{
    if (!pInfo)
    {
        return HXR_INVALID_PARAMETER;
    }

    UINT32 ulLoss = 0;
    UINT32 ulRate = 0;
    RDTStream* pStreamInfo = NULL;

    CHXMapLongToObj::Iterator i;
    CHXMapLongToObj::Iterator end = m_StreamMap.End();

    for(i = m_StreamMap.Begin(); i != end; ++i)
    {
        pStreamInfo = (RDTStream*)(*i);
        ulLoss += pStreamInfo->m_ulNAKCount;
        ulRate += (UINT32)pStreamInfo->m_dRate;
    }

    pInfo->SetRRFrequency(0);
    pInfo->SetRTT((UINT32)m_fRTT);
    pInfo->SetPacketLoss(ulLoss);
    pInfo->SetReceivedThroughput(ulRate);
    
    return HXR_OK;
}

HX_RESULT 
QoSRDTMetrics::CreateReport(UINT16 nStreamNumber, IHXBuffer* pReport)
{
    RDTStream* pStreamInfo = NULL;
    if (!m_StreamMap.Lookup(nStreamNumber, (void*&)pStreamInfo))
    {
	DPRINTF(D_ERROR, ("Invalid RDT report request\n"));
	return HXR_INVALID_PARAMETER;
    }

    if ((!m_fRTT) || (pStreamInfo->m_dRate == 0))
    {
	return HXR_NOT_INITIALIZED;
    }
    
    pReport->SetSize(sizeof(RDTTransportMetrics));
    RDTTransportMetrics* pMetrics = (RDTTransportMetrics*)pReport->GetBuffer();

    pMetrics->m_nStreamNumber = nStreamNumber;
    pMetrics->m_ulNumRecvd    = pStreamInfo->m_ulLastACKCount;
    pMetrics->m_ulNumLost     = pStreamInfo->m_ulLastNAKCount;
    pMetrics->m_fRTT          = m_fRTT;
    pMetrics->m_fLoss         = m_fLossRate;
    pMetrics->m_fRecvRate     = pStreamInfo->m_dRate;

    return HXR_OK;
}




