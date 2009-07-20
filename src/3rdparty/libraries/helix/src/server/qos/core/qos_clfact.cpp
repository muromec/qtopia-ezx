/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_clfact.cpp,v 1.12 2004/07/08 22:07:27 ghori Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "timeval.h"
#include "hxerror.h"
#include "sink.h"
#include "source.h"

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxqossig.h"
#include "hxstats.h"
#include "hxqos.h"
#include "hxqostran.h"
#include "hxqossess.h"
#include "hxpcktflwctrl.h"
#include "server_engine.h"

#ifdef HELIX_FEATURE_QOS
#include "qos_clfact_rn.h"
#endif

#include "qos_clfact.h"
#include "qos_tran_cc.h"
#include "qos_tran_aimd.h"
#include "qos_tran_cc_agg.h"
#include "qos_cfg_names.h"
#include "qos_sess_cbr_ratemgr.h"

HXQoSClassFactory::HXQoSClassFactory():
    m_lRefCount(0), 
    m_proc(NULL)
{
#ifdef HELIX_FEATURE_QOS
     m_pRNQoSClassFactory = new RNQoSClassFactory();
     m_pRNQoSClassFactory->AddRef();
#endif
}

HXQoSClassFactory::HXQoSClassFactory(Process* proc)
		    : m_lRefCount(0), m_proc(proc)
{
#ifdef HELIX_FEATURE_QOS
     m_pRNQoSClassFactory = new RNQoSClassFactory(proc);
     m_pRNQoSClassFactory->AddRef();
#endif
}

HXQoSClassFactory::~HXQoSClassFactory()
{
#ifdef HELIX_FEATURE_QOS
     HX_RELEASE(m_pRNQoSClassFactory);
#endif     
}

STDMETHODIMP 
HXQoSClassFactory::QueryInterface(REFIID riid, void** ppvObj)
{
#ifdef HELIX_FEATURE_QOS
    return m_pRNQoSClassFactory->QueryInterface(riid, ppvObj);
#else
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSClassFactory))
    {
	AddRef();
	*ppvObj = (IHXQoSClassFactory*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
#endif
}

STDMETHODIMP_(ULONG32) 
HXQoSClassFactory::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
HXQoSClassFactory::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP 
HXQoSClassFactory::CreateInstance(THIS_ 
				  IHXQoSProfileConfigurator* pConfig,
				  REFCLSID /*IN*/ rclsid,
				  void** /*OUT*/ ppUnknown)
{
    if (!pConfig)
    {
	*ppUnknown = NULL;
	return HXR_INVALID_PARAMETER;
    }

#ifdef HELIX_FEATURE_QOS
    return m_pRNQoSClassFactory->CreateInstance(pConfig, rclsid, ppUnknown);
#else
    if (IsEqualCLSID(rclsid, CLSID_IHXQoSCongestionControl))
    {
	*ppUnknown = (IUnknown*)(IHXQoSCongestionControl*)
	    (new QoSCongestionCtl((IUnknown*)m_proc->pc->server_context));
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXQoSRateShapeAggregator))
    {
	*ppUnknown = (IUnknown*)(IHXQoSRateShapeAggregator*)
	    (new QoSRateShapeAggregator());
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXQoSCongestionEquation))
    {
	HX_RESULT hRes = HXR_NOINTERFACE; 
      	IHXBuffer* pCCType = NULL;

	if ((SUCCEEDED(pConfig->GetConfigBuffer(QOS_CFG_CC_TYPE, pCCType))) &&
	    pCCType && pCCType->GetBuffer())
	{
	    if (!strcasecmp("AIMD", (const char*)pCCType->GetBuffer()))
	    {
		*ppUnknown = (IUnknown*)(IHXQoSCongestionEquation*) (new QoSCongestionEqn_AIMD());
		((IUnknown*)*ppUnknown)->AddRef();
		hRes = HXR_OK;
	    }
	    else
	    {
		*ppUnknown = NULL;
		hRes = HXR_NOINTERFACE; 
	    }
	}

	HX_RELEASE(pCCType);
	return hRes;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXQoSRateManager))
    {
	HX_RESULT hRes = HXR_NOINTERFACE; 
      	IHXBuffer* pType = NULL;

	if ((SUCCEEDED(pConfig->GetConfigBuffer(QOS_CFG_RM_TYPE, pType))) &&
	    pType && pType->GetBuffer())
	{
	    if (!strcasecmp("CBR", (const char*)pType->GetBuffer()))
	    {
		*ppUnknown = (IUnknown*)(IHXQoSRateManager*) 
		    (new CCBRRateMgr(m_proc));
		((IUnknown*)*ppUnknown)->AddRef();
		hRes = HXR_OK;
	    }
	    else
	    {
		*ppUnknown = NULL;
		hRes = HXR_NOINTERFACE; 
	    }
	}

	HX_RELEASE(pType);
	return hRes;
    }

    *ppUnknown = NULL;
    return HXR_NOINTERFACE; 
#endif
}
