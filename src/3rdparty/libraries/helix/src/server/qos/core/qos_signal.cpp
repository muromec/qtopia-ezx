/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_signal.cpp,v 1.4 2005/06/29 14:58:38 dcollins Exp $ 
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
#include "hxqossig.h"
#include "hxqos.h"

#include "qos_signal.h"

/* QoSSignalBusController Implementation */
QoSSignal::QoSSignal():
    m_lRefCount(0),
    m_ulId(0),
    m_ulValue(0),
    m_pValue(NULL)
{
}


QoSSignal::QoSSignal(BOOL bRef, IHXBuffer* pBuffer, HX_QOS_SIGNAL ulSignalId):
    m_lRefCount(bRef ? 1 : 0),
    m_ulId(ulSignalId),
    m_ulValue(0),
    m_pValue(pBuffer)
{
    pBuffer->AddRef();
}


QoSSignal::~QoSSignal()
{
    HX_RELEASE(m_pValue);
}

/* IHXQoSSignal */
STDMETHODIMP
QoSSignal::GetId (REF(HX_QOS_SIGNAL) ulSignalId)
{
    if (!m_ulId)
    {
	return HXR_NOT_INITIALIZED;
    }
	
    ulSignalId = m_ulId;
    return HXR_OK;
}

STDMETHODIMP
QoSSignal::SetId (HX_QOS_SIGNAL ulSignalId)
{
    m_ulId = ulSignalId;
    return HXR_OK;
}

STDMETHODIMP
QoSSignal::GetValueUINT32 (REF(UINT32) ulValue)
{
    ulValue = m_ulValue;
    return HXR_OK;
}

STDMETHODIMP
QoSSignal::SetValueUINT32 (UINT32 ulValue)
{
    m_ulValue = ulValue;
    return HXR_OK;
}

STDMETHODIMP
QoSSignal::GetValue(REF(IHXBuffer*) pBuffer)
{
    if (!m_pValue)
    {
	return HXR_NOT_INITIALIZED;
    }

    pBuffer = m_pValue;
    m_pValue->AddRef();
    return HXR_OK;
}

STDMETHODIMP
QoSSignal::SetValue(IHXBuffer* pBuffer)
{
    if (!pBuffer)
    {
	return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pValue);

    pBuffer->AddRef();
    m_pValue = pBuffer;
    return HXR_OK;
}

STDMETHODIMP
QoSSignal::WriteSignal(UINT32 ulFileDescriptor)
{
    /*XXXDWL TODO*/
    return HXR_FAIL;
}

/* IUnknown */
STDMETHODIMP
QoSSignal::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXQoSSignal*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSSignal))
    {
	AddRef();
	*ppvObj = (IHXQoSSignal*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSSignal::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSSignal::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}
