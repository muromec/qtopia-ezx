/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sink_container.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "sink_container.h"

SinkContainer::SinkContainer(IHXRawSinkObject* pSink):
    m_lRefCount(0),
    m_pSink(pSink)
{
    if (m_pSink)
    {
	m_pSink->AddRef();
    }
}

SinkContainer::~SinkContainer()
{
    if (m_pSink)
    {
	m_pSink->Release();
	m_pSink = 0;
    }
}

UINT32
SinkContainer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

UINT32
SinkContainer::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
SinkContainer::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXPSinkControl*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXPSinkControl))
    {
	AddRef();
	*ppvObj = (IHXPSinkControl*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXPSinkPackets))
    {
	AddRef();
	*ppvObj = (IHXPSinkPackets*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/* IHXPSinkControl methods */

STDMETHODIMP
SinkContainer::InitDone(HX_RESULT status)
{
    return m_pSink->InitDone(status);
}

STDMETHODIMP
SinkContainer::FileHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    return m_pSink->FileHeaderReady(status, pHeader);
}

STDMETHODIMP
SinkContainer::StreamHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    return m_pSink->StreamHeaderReady(status, pHeader);
}

STDMETHODIMP
SinkContainer::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    return m_pSink->PacketReady(status, pPacket);
}

STDMETHODIMP
SinkContainer::StreamDone(UINT16 unStreamNumber)
{
    return m_pSink->StreamDone(unStreamNumber);
}

STDMETHODIMP
SinkContainer::SeekDone(HX_RESULT status)
{
    return HXR_UNEXPECTED;
}

void
SinkContainer::Done()
{
    if (m_pSink)
    {
	m_pSink->Release();
	m_pSink = 0;
    }
}
