/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_test_sink.cpp,v 1.4 2007/10/11 06:38:37 yphadke Exp $ 
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
#include "hxqossig.h"
#include "hxqos.h"

#include "mutex.h"
#include "chxmapstringtoob.h"
#include "hxbuffer.h"
#include "servlist.h"

#include "qos_signal.h"
#include "qos_sig_bus_ctl.h"
//#include "qos_sig_bus.h"
#include "qos_test_sink.h"
#include "qos_test_engine.h"

QoSTestSink::QoSTestSink(IHXQoSSignalSource* pSrc, IHXBuffer* pId) :
    //m_pSource(pSrc),
    m_pSessionId(pId),
    m_pSignal(NULL),
    m_lRefCount(0),
    m_ulFilter(0)
{
    //HX_ASSERT(m_pSource);
    //HX_ASSERT(m_pSessionId);

    //m_pSource->AddRef();
    //m_pSessionId->AddRef();

    //m_pSignal = (IHXQoSSignal*)(new QoSSignal());
    //m_pSignal->AddRef();

    srand((unsigned)this);
    UINT32 ulLayer = (rand() % HX_QOS_SIGNAL_LAYER_COUNT);
    UINT32 ulRel   = (rand() % HX_QOS_SIGNAL_RELEVANCE_COUNT);

    m_ulFilter = MAKE_HX_QOS_SIGNAL_ID( (1 << ulLayer), 
					(1 << ulRel), 0);

    //HX_ASSERT(SUCCEEDED(m_pSource->Attach(m_pSessionId, m_ulFilter, (IHXQoSSignalSink*)this)));
}

QoSTestSink::~QoSTestSink()
{
    //HX_RELEASE(m_pSource);
    HX_RELEASE(m_pSignal);
    HX_RELEASE(m_pSessionId);
}

/* QoSTestSink */
HX_RESULT
QoSTestSink::TestSignal(IHXQoSSignalBus* pBus)
{
    HX_ASSERT(pBus);
    HX_ASSERT(m_pSignal);

    m_pSignal->SetId(m_ulFilter);
    m_pSignal->SetValueUINT32((UINT32)this);

    return pBus->Send(m_pSignal);
}

HX_RESULT
QoSTestSink::Close()
{
    //HX_ASSERT(SUCCEEDED(m_pSource->Dettach(m_pSessionId, m_ulFilter, (IHXQoSSignalSink*)this)));
    return HXR_OK;
}


/* IHXQoSSignalSink */
STDMETHODIMP 
QoSTestSink::Signal(IHXQoSSignal* pSignal, IHXBuffer* pSessionId)
{
    HX_ASSERT(m_pSessionId);
    HX_ASSERT(pSignal);

    HX_ASSERT((memcmp((const void*)pSessionId->GetBuffer(), 
		      (const void*)m_pSessionId->GetBuffer(),
		      pSessionId->GetSize()) == 0));
	    
    /* Test for correct signal mask */
    HX_QOS_SIGNAL ulId = 0;
    pSignal->GetId(ulId);
 
    if (!m_ulFilter)
    {
	UINT8  cIdLayer = (ulId&HX_QOS_SIGNAL_LAYER_MASK);
	UINT8  cIdRel   = (ulId&HX_QOS_SIGNAL_RELEVANCE_MASK);
	UINT16 cIdId    = (ulId&HX_QOS_SIGNAL_ID_MASK);
	
	UINT8  cFiltLayer = (m_ulFilter&HX_QOS_SIGNAL_LAYER_MASK);
	UINT8  cFiltRel   = (m_ulFilter&HX_QOS_SIGNAL_RELEVANCE_MASK);
	UINT16 cFiltId    = (m_ulFilter&HX_QOS_SIGNAL_ID_MASK);

	if (cFiltLayer)
	{
	    HX_ASSERT(cFiltLayer == cIdLayer);
	    
	    if (cFiltRel)
	    {
		HX_ASSERT(cFiltRel == cIdRel);
		
		if (cFiltId)
		{
		    HX_ASSERT(cFiltId == cIdId);
		}
	    }
	}
	
    }

    
    return HXR_OK;
}

STDMETHODIMP 
QoSTestSink::ChannelClosed (IHXBuffer* pSessionId)
{
    //HX_ASSERT(pSessionId);
    //HX_ASSERT(m_pSource);

    //HX_ASSERT((memcmp((const void*)pSessionId->GetBuffer(), 
		  //    (const void*)m_pSessionId->GetBuffer(),
		  //    pSessionId->GetSize()) == 0));

    return HXR_OK;
}

/* IUnknown */
STDMETHODIMP
QoSTestSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXQoSSignalSink*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSSignalSink))
    {
	AddRef();
	*ppvObj = (IHXQoSSignalSink*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSTestSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSTestSink::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}


   
