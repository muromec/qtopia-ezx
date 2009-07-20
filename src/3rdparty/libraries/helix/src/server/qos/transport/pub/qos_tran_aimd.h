/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_aimd.h,v 1.5 2005/01/18 21:33:51 damonlan Exp $ 
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

#ifndef _QOS_TRAN_RTCP_AIMD_H_
#define _QOS_TRAN_RTCP_AIMD_H_

typedef _INTERFACE IHXQoSCongestionEquation IHXQoSCongestionEquation;

/* General case memory-less rate based congestion control 
 * using additive increase and multiplicative decrease
 * with TCP friendly coefficients.
*/

/* From: Y. Richard Yang, Simon Lam; "General AIMD Congestion Control" */
#define QOS_CC_AIMD_INC_RATE 0.31
#define QOS_CC_AIMD_DEC_RATE 0.875  
#define QOS_CC_AIMD_MAX_RATE 0xFFFFFFFF

class QoSCongestionEqn_AIMD : public IHXQoSCongestionEquation
{
 public:
    QoSCongestionEqn_AIMD ();
    ~QoSCongestionEqn_AIMD ();

    /* IUnknown */
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)  ( THIS );
    STDMETHOD_(ULONG32,Release) ( THIS );

    /* IHXQoSCongestionEquation */
    STDMETHOD (Init)    (THIS_ IUnknown* pContext,
			 IHXBuffer* pSessionId,
			 UINT16 nStreamNumber);

    STDMETHOD (Update)   (THIS_ UINT32 ulNumRecvd,
			  UINT32 ulNumLoss,
			  double fInboundRate,
			  double fOutboundRate,
			  double fRawLoss, 
			  double fRawRTT);

    STDMETHOD (FeedbackTimeout) (THIS);

    STDMETHOD (SetMediaRate) (THIS_ UINT32 ulMediaRate, BOOL bReferenceRate);

    STDMETHOD (SetMediaPacketSize) (THIS_ UINT32 ulMediaPacketSize);

    STDMETHOD (SetMaximumRate) (THIS_ UINT32 ulMaxRate);

    STDMETHOD (GetRate) (THIS_ REF(UINT32) /* OUT */ ulRate);

    STDMETHOD (GetEquationType) (THIS_ REF(HX_QOS_CC_TYPE) /* OUT */ cType);

 private:
    LONG32    m_lRefCount;
    float     m_fIncreaseCoefficient;
    float     m_fDecreaseCoefficient;

    UINT32    m_ulRate;
    UINT32    m_ulMediaRate;
    UINT32    m_ulMaxRate;
    
    BOOL      m_bSlowStart;
};

#endif /*_QOS_TRAN_AIMD_H_*/
