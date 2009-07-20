/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: engine_desc_reg.cpp,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
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
#include "hxassert.h"
#include "engine_desc_reg.h"

STDMETHODIMP
ServerEngineDescReg::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXDescriptorRegistration))
    {
	AddRef();
	*ppvObj = (IHXDescriptorRegistration*) this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerEngineDescReg::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
ServerEngineDescReg::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }
    ASSERT(FALSE);
    delete this;
    return 0;
}

STDMETHODIMP
ServerEngineDescReg::RegisterDescriptors(UINT32 ulCount)
{
    m_ulDescCount += ulCount;

    OnDescriptorsChanged();
    return HXR_OK;
}

STDMETHODIMP
ServerEngineDescReg::UnRegisterDescriptors(UINT32 ulCount)
{
    m_ulDescCount -= ulCount;
    ASSERT (m_ulDescCount >= DESCRIPTOR_START_VALUE);

    OnDescriptorsChanged();
    return HXR_OK;
}

STDMETHODIMP
ServerEngineDescReg::RegisterSockets(UINT32 ulCount)
{
#ifdef _WIN32
    m_ulSockCount += ulCount;
#else
    m_ulDescCount += ulCount;
#endif

    OnSocketsChanged();
    return HXR_OK;
}

STDMETHODIMP
ServerEngineDescReg::UnRegisterSockets(UINT32 ulCount)
{
#ifdef _WIN32
    m_ulSockCount -= ulCount;
    ASSERT (m_ulSockCount >= SOCK_START_VALUE);
#else
    m_ulDescCount -= ulCount;
    ASSERT (m_ulDescCount >= DESCRIPTOR_START_VALUE);
#endif

    OnSocketsChanged();
    return HXR_OK;
}


UINT32
ServerEngineDescReg::GetDescCapacity()
{
    INT32 lCapacity = ((INT32)DESCRIPTOR_CAPACITY_VALUE) - ((INT32)m_ulDescCount);

    if (lCapacity < 0)
    {
	lCapacity = 0;
    }

    return (UINT32)lCapacity;
}


UINT32
ServerEngineDescReg::GetSockCapacity()
{
    INT32 lCapacity = ((INT32)SOCK_CAPACITY_VALUE) - ((INT32)m_ulSockCount);

    if (lCapacity < 0)
    {
	lCapacity = 0;
    }

    return (UINT32)lCapacity;
}

UINT32
ServerEngineDescReg::NumFDs()
{
#ifdef _WIN32
    return m_ulSockCount;
#else
    return m_ulDescCount;
#endif
}


