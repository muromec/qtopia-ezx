/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_rdt_metrics.h,v 1.8 2005/07/05 20:32:54 jgordon Exp $ 
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

#ifndef _QOS_TRAN_RDT_METRICS_H_
#define _QOS_TRAN_RDT_METRICS_H_

#define PKT_HISTORY_SIZE 1024
#define LOSS_FILTER_COEF 0.1

typedef _INTERFACE IHXQoSTransportAdaptationInfo IHXQoSTransportAdaptationInfo;

struct RDTTransportMetrics
{
    UINT16 m_nStreamNumber;
    UINT32 m_ulNumRecvd;
    UINT32 m_ulNumLost;
    double m_fRTT;
    double m_fLoss;
    double m_fRecvRate;
};

struct RDTBufferMetricsSignal
{
    UINT32 m_ulStreamNumber;
    UINT32 m_ulLowTimestamp;
    UINT32 m_ulHighTimestamp;
    UINT32 m_ulBytes;
};

struct PktHistoryElem
{
    UINT32 m_ulSize;         /* bytes */
    BOOL   m_bACKd;
    BOOL   m_bNAKd;
};

class QoSRDTMetrics
{
 public:
    QoSRDTMetrics ();
    ~QoSRDTMetrics ();

    void Enqueue (UINT16 unStream,
		  UINT16 nSeqNo, 
		  UINT32 /* bytes */ ulSize);

    void Update (UINT32 /* msec */ ulNow, 
		 UINT16 unStream,
		 double fRTT,
		 UINT16* nAckList, UINT32 ackLen,
		 UINT16* nNakList, UINT32 nakLen);

    HX_RESULT CreateReport(UINT16 nStreamNumber, IHXBuffer* pReport);
    HX_RESULT UpdateStatistics (IHXQoSTransportAdaptationInfo* pInfo);

 private:
    struct RDTStream
    {
        PktHistoryElem* m_pPktHistory;
        UINT32          m_ulLastACK; /* msec */
        UINT32          m_ulLastACKCount;
        UINT32          m_ulLastNAKCount;
        UINT32          m_ulNAKCount;
        double          m_dRate;
    };

    CHXMapLongToObj m_StreamMap;

    UINT32      m_ulStartTime;  /* msec */
    UINT32      m_ulLossAcc;
    UINT32      m_ulLocalLossAcc;
    UINT32      m_ulPktAcc;
    double      m_fLossRate;

    double      m_fRTT;
};


#endif /*_QOS_TRAN_RDT_METRICS_H_ */
