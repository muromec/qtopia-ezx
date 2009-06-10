/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_loss_event.h,v 1.4 2003/10/14 22:15:39 damonlan Exp $ 
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

#ifndef _QOS_TRAN_LOSS_EVENT_H_
#define _QOS_TRAN_LOSS_EVENT_H_

#define QOS_CC_LOSS_HIST_SZ 8 /* loss intervals */

class LossEventHistory 
{
 public:
    LossEventHistory ();
    LossEventHistory (UINT8 cHistorySize);
    ~LossEventHistory ();

    HX_RESULT AcknowledgePackets (UINT16 unACKCount);
    HX_RESULT OnNewInterval ();
    HX_RESULT ResetHistory ();
    HX_RESULT ResetCurrentInterval();
    HX_RESULT GetLossEventRate      (double& /*OUT*/ pLossRate,
				     BOOL bUseDiscounting);
    HX_RESULT EstimateLossEventRate (double& /*OUT*/ pLossRate, 
				     double fThroughput,
				     double fAvgPacketSize,
				     double fRTTEstimate,
				     double fRTOEstimate);
 private:
    void       Init();
    double     ThroughputEquation (double fAvgPktSize,
				   double fLossEventRate,
				   double fRTTEstimate,
				   double fRTOEstimate);

    UINT8      m_cLossHistorySize;
    UINT8      m_cCurrentLossWindow;
    UINT16*    m_pLossEventHistory;
    double*    m_pWeights;
    double*    m_pDiscountFactor;
    double     m_fGeneralDiscountFactor;
};

#endif /*_QOS_TRAN_RTCP_CC_H_ */
