/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_cc_agg.h,v 1.2 2003/09/11 23:39:46 damonlan Exp $ 
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

#ifndef _QOS_TRAN_CC_AGG_H_
#define _QOS_TRAN_CC_AGG_H_

typedef _INTERFACE IHXQoSRateShaper IHXQoSRateShaper;
typedef _INTERFACE IHXQoSRateShapeAggregator IHXQoSRateShapeAggregator;

#define QOS_CC_AGG_MAX_TOKENS_DEFAULT 5000 /* bytes */

class RateShaper
{
 public:
    RateShaper  (UINT16 nStreamNumber);
    ~RateShaper ();
    
    void SetShaper   (IHXQoSRateShaper* pShaper);

    UINT16 m_unStreamNumber;
    BOOL   m_bBlocked;
    IHXQoSRateShaper* m_pShaper;
};

class QoSRateShapeAggregator : public IHXQoSRateShapeAggregator
{
 public:
    QoSRateShapeAggregator ();
    ~QoSRateShapeAggregator ();
    
    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*  IHXQoSRateShapeAggregator */
    STDMETHOD (Init)             (THIS_ UINT16 nStreamCount);
    STDMETHOD (Done)             (THIS);
    STDMETHOD (AddRateShaper)    (THIS_ IHXQoSRateShaper* pShaper);
    STDMETHOD (RemoveRateShaper) (THIS_ IHXQoSRateShaper* pShaper);
    STDMETHOD (AddTokens)        (THIS_ UINT16 nTokens, UINT16 nFromStream);
    STDMETHOD (RemoveTokens)     (THIS_ UINT16 nTokens, UINT16 nFromStream);
    STDMETHOD (GetTokens)        (THIS_ REF(UINT16) /*OUT*/ nTokens);

 private:
    /* IUnknown */
    LONG32                        m_lRefCount;

    UINT16                        m_unStreamCount;
    UINT16                        m_unCurrentStream;
    UINT32                        m_ulTokens;
    UINT32                        m_ulMaxTokens;
    RateShaper**                  m_ppRateShapers; 

    UINT32                        m_ulStartTime;
};

#endif /*_QOS_TRAN_CC_AGG_H_ */
