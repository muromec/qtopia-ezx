/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qos_tranadapt_info.cpp,v 1.2 2005/07/22 18:55:45 richardjones Exp $
 *
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
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

#include <signal.h>

#include <stdio.h>
#include <string.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "debug.h"
#include "qos_tranadapt_info.h"


// static defines

HTTPQoSTranAdaptInfo::HTTPQoSTranAdaptInfo(void)
    : m_nRefCount(0)
    , m_pTxRateRange(NULL)
    , m_ulRRFrequency(0)
    , m_ulRTT(0)
    , m_ulPacketLoss(0)
    , m_ulReceivedThroughput(0)
    , m_ulBytesSent(0)
    , m_ulPacketsSent(0)
{
    // Empty
}

HTTPQoSTranAdaptInfo::~HTTPQoSTranAdaptInfo(void)
{
}

STDMETHODIMP
HTTPQoSTranAdaptInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXQoSTransportAdaptationInfo*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSTransportAdaptationInfo))
    {
        AddRef();
        *ppvObj = (IHXQoSTransportAdaptationInfo*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HTTPQoSTranAdaptInfo::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(UINT32)
HTTPQoSTranAdaptInfo::Release(void)
{
    if (InterlockedDecrement(&m_nRefCount) > 0)
    {
        return m_nRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP_(UINT32) 
HTTPQoSTranAdaptInfo::GetRRFrequency()
{
    return m_ulRRFrequency;
}

STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetRRFrequency(UINT32 ulRRFrequency)
{
    m_ulRRFrequency = ulRRFrequency;
    return HXR_OK;
}

STDMETHODIMP_(UINT32) 
HTTPQoSTranAdaptInfo::GetRTT()
{
    return m_ulRTT;
}

STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetRTT(UINT32 ulRTT)
{
    m_ulRTT = ulRTT;
    return HXR_OK;
}

STDMETHODIMP_(UINT32) 
HTTPQoSTranAdaptInfo::GetPacketLoss()
{
    return m_ulPacketLoss;
}

STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetPacketLoss(UINT32 ulPacketLoss)
{
    m_ulPacketLoss = ulPacketLoss;
    return HXR_OK;
}

STDMETHODIMP_(UINT32) 
HTTPQoSTranAdaptInfo::GetReceivedThroughput()
{
    return m_ulReceivedThroughput;
}

STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetReceivedThroughput(UINT32 ulReceivedThroughput)
{
    m_ulReceivedThroughput = ulReceivedThroughput;
    return HXR_OK;
}

STDMETHODIMP_(UINT32) 
HTTPQoSTranAdaptInfo::GetSuccessfulResends() 
{ 
    return m_ulSuccessfulResends; 
}

STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetSuccessfulResends(UINT32 ulSuccessfulResends) 
{ 
    m_ulSuccessfulResends = ulSuccessfulResends; 
    return HXR_OK; 
}

STDMETHODIMP_(UINT32) 
HTTPQoSTranAdaptInfo::GetFailedResends() 
{ 
    return m_ulFailedResends; 
}

STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetFailedResends(UINT32 ulFailedResends) 
{ 
    m_ulFailedResends = ulFailedResends; 
    return HXR_OK; 
}

STDMETHODIMP_(IHXBuffer*) 
HTTPQoSTranAdaptInfo::GetTxRateRange() 
{ 
    if (m_pTxRateRange)
    {
        m_pTxRateRange->AddRef();
    }
    return m_pTxRateRange; 
}

STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetTxRateRange(IHXBuffer* pTxRateRange) 
{ 
    IHXBuffer* pTemp = m_pTxRateRange;
    m_pTxRateRange = pTxRateRange; 
    if (m_pTxRateRange)
    {
        m_pTxRateRange->AddRef();
    }

    HX_RELEASE(pTemp);
    return HXR_OK; 
}

STDMETHODIMP_(UINT32) 
HTTPQoSTranAdaptInfo::GetPacketsSent() 
{ 
    return m_ulPacketsSent; 
}



STDMETHODIMP
HTTPQoSTranAdaptInfo::SetPacketsSent(UINT32 ulPacketsSent) 
{ 
    m_ulPacketsSent = ulPacketsSent; 
    return HXR_OK; 
}


STDMETHODIMP_(UINT64) 
HTTPQoSTranAdaptInfo::GetBytesSent() 
{ 
    return m_ulBytesSent; 
}


STDMETHODIMP 
HTTPQoSTranAdaptInfo::SetBytesSent(UINT64 ulBytesSent) 
{ 
    m_ulBytesSent = ulBytesSent; 
    return HXR_OK; 
}

