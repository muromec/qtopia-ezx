/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_aimd.cpp,v 1.8 2007/07/31 06:24:27 yphadke Exp $ 
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
#include "ihxpckts.h"
#include "hxengin.h"
#include "hxassert.h"

#include "hxqossig.h"
#include "hxqos.h"
#include "hxqostran.h"

#include "qos_tran_aimd.h"

QoSCongestionEqn_AIMD::QoSCongestionEqn_AIMD() :
    m_lRefCount(0),
    m_fIncreaseCoefficient (QOS_CC_AIMD_INC_RATE),
    m_fDecreaseCoefficient (QOS_CC_AIMD_DEC_RATE),
    m_ulRate(0),
    m_ulMediaRate(0),
    m_bSlowStart(TRUE)
{
}

QoSCongestionEqn_AIMD::~QoSCongestionEqn_AIMD()
{
}
    
/* IHXQoSCongestionEquation */
STDMETHODIMP
QoSCongestionEqn_AIMD::Init (IUnknown* pContext,
			     IHXBuffer* pSessionId,
			     UINT16 nStreamNumber)
{
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionEqn_AIMD::Update (UINT32 ulNumRecvd,
			       UINT32 ulNumLoss,
			       double fInboundRate,
			       double fOutboundRate,
			       double fRawLoss, 
			       double fRawRTT)
{
    if (fRawLoss != 0)
    {
	m_ulRate = (UINT32)((m_bSlowStart) ? (m_ulRate / 2) :
			    (m_ulRate*m_fDecreaseCoefficient));

	m_bSlowStart = FALSE;
    }
    else
    {
	m_ulRate = (UINT32)((m_bSlowStart) ? (m_ulRate * 2) : 
			    (m_ulRate + (m_ulRate*m_fIncreaseCoefficient)));
    }

    
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionEqn_AIMD::FeedbackTimeout()
{
    m_ulRate = m_ulMediaRate;
    m_bSlowStart = TRUE;
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionEqn_AIMD::SetMediaRate (UINT32 ulMediaRate, BOOL bReferenceRate)
{
    m_ulMediaRate = m_ulRate = ulMediaRate;
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionEqn_AIMD::SetMediaPacketSize (UINT32 ulMediaPacketSize)
{
    /* NO-OP: AIMD doesn't use pkt size */
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionEqn_AIMD::GetRate (REF(UINT32) /* OUT */ ulRate)
{
    ulRate = m_ulRate;
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionEqn_AIMD::SetRate (REF(UINT32) ulRate)
{
    m_ulRate = ulRate;
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionEqn_AIMD::GetEquationType (REF(HX_QOS_CC_TYPE) /* OUT */ cType)
{
    cType = HX_QOS_CC_TYPE_AIMD;
    return HXR_OK;
}

/* IUnknown */
STDMETHODIMP
QoSCongestionEqn_AIMD::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXQoSCongestionEquation*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSCongestionEquation))
    {
	AddRef();
	*ppvObj = (IHXQoSCongestionEquation*)this;
	return HXR_OK;
    }
   
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSCongestionEqn_AIMD::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSCongestionEqn_AIMD::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}


  

