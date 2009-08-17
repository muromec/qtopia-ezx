/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_cc_agg.cpp,v 1.4 2007/02/17 00:27:42 jzeng Exp $ 
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
#include "hxresult.h"
#include "hxassert.h"
#include "ihxpckts.h"
#include "hxqostran.h"

#include "hxtick.h"

#include "qos_tran_cc_agg.h"


QoSRateShapeAggregator::QoSRateShapeAggregator () :
    m_lRefCount (0),
    m_unStreamCount (0),
    m_unCurrentStream (0),
    m_ulTokens (0),
    m_ulMaxTokens (QOS_CC_AGG_MAX_TOKENS_DEFAULT),
    m_ppRateShapers (NULL),
    m_ulStartTime(HX_GET_BETTERTICKCOUNT())
{
}
 
QoSRateShapeAggregator::~QoSRateShapeAggregator ()
{
    Done ();
}

/*  IHXQoSRateShapeAggregator */
STDMETHODIMP
QoSRateShapeAggregator::Init (THIS_ UINT16 nStreamCount)
{
    HX_ASSERT(nStreamCount);
    HX_ASSERT(!m_ppRateShapers);

    if (!nStreamCount)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    if (m_ppRateShapers)
    {
        HX_ASSERT(0);
        return HXR_FAIL;
    }

    m_unStreamCount = nStreamCount;

    m_ppRateShapers = new RateShaper* [m_unStreamCount];

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        m_ppRateShapers [i] = new RateShaper (i);
    }

    return HXR_OK;
}

STDMETHODIMP
QoSRateShapeAggregator::Done (THIS)
{
    if (m_ppRateShapers)
    {
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            HX_DELETE(m_ppRateShapers [i]);
        }
        HX_VECTOR_DELETE(m_ppRateShapers);
    }

    return HXR_OK;
}

STDMETHODIMP
QoSRateShapeAggregator::AddRateShaper (THIS_ IHXQoSRateShaper* pShaper)
{
    if (!m_ppRateShapers)
    {
        HX_ASSERT(0);
        return HXR_NOT_INITIALIZED;
    }

    if (!pShaper)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    UINT16 unStream = 0;
    UINT32 ulMaxToks = 0;
    
    HX_VERIFY(SUCCEEDED(pShaper->GetStreamNumber(unStream)));

    if (unStream > m_unStreamCount)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    m_ppRateShapers [unStream]->SetShaper(pShaper);
    
    HX_VERIFY(SUCCEEDED(pShaper->GetMaxTokens(ulMaxToks)));
    m_ulMaxTokens += ulMaxToks;

    return HXR_OK;
}

STDMETHODIMP
QoSRateShapeAggregator::RemoveRateShaper (THIS_ IHXQoSRateShaper* pShaper)
{
    if (!m_ppRateShapers)
    {
        HX_ASSERT(0);
        return HXR_NOT_INITIALIZED;
    }

    if (!pShaper)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    UINT16 unStream = 0;
    UINT32 ulMaxToks = 0;
    
    HX_VERIFY(SUCCEEDED(pShaper->GetStreamNumber(unStream)));

    if (unStream > m_unStreamCount)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    if (pShaper != m_ppRateShapers [unStream]->m_pShaper)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_ppRateShapers [unStream]->m_pShaper);
    m_ppRateShapers [unStream]->m_bBlocked = FALSE;

    HX_VERIFY(SUCCEEDED(pShaper->GetMaxTokens(ulMaxToks)));
    m_ulMaxTokens -= ulMaxToks;

    return HXR_OK;
}

STDMETHODIMP
QoSRateShapeAggregator::AddTokens (THIS_ UINT16 nTokens, UINT16 nFromStream)
{
    if (!m_ppRateShapers)
    {
        HX_ASSERT(0);
        return HXR_NOT_INITIALIZED;
    }

    if (nFromStream > m_unStreamCount)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    m_ulMaxTokens = 0;
    UINT32 ulToks = 0;

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        if (m_ppRateShapers [i] && m_ppRateShapers [i]->m_pShaper)
        {
            m_ppRateShapers [i]->m_pShaper->GetMaxTokens(ulToks);
        }

        m_ulMaxTokens += ulToks;
        ulToks = 0;
    }

    m_ulTokens += nTokens;
    m_ulTokens = (m_ulTokens > m_ulMaxTokens) ? m_ulMaxTokens : m_ulTokens;

    /* 
       Round robin the streams until we run out of tokens, 
       or have cleared all the streams 
    */

    UINT16 nCurrentStream = nFromStream;

    if (m_ppRateShapers)
    {
        while (m_ulTokens)
        {
            if ((m_ppRateShapers [nCurrentStream]) && 
                (m_ppRateShapers [nCurrentStream]->m_bBlocked) &&
                m_ppRateShapers [nCurrentStream]->m_pShaper)
            {
                m_ppRateShapers [nCurrentStream]->m_bBlocked = FALSE;
                m_ppRateShapers [nCurrentStream]->m_pShaper->StreamCleared(nCurrentStream);
            }

            (++nCurrentStream) %= m_unStreamCount;

            if (nCurrentStream == nFromStream)
            {
                break;
            }
        }
    }

    return HXR_OK;
}

STDMETHODIMP
QoSRateShapeAggregator::RemoveTokens (THIS_ UINT16 nTokens, UINT16 nFromStream)
{
    if (!m_ppRateShapers)
    {
        HX_ASSERT(0);
        return HXR_NOT_INITIALIZED;
    }

    if (nFromStream > m_unStreamCount)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    if (nTokens > m_ulTokens)
    {
        m_ppRateShapers [nFromStream]->m_bBlocked = TRUE;
        return HXR_BLOCKED;
    }

    m_ulTokens -= nTokens;
    return HXR_OK;
}

STDMETHODIMP
QoSRateShapeAggregator::GetTokens (THIS_ REF(UINT16) /*OUT*/ nTokens)
{
    nTokens = (UINT16)m_ulTokens;
    return HXR_OK;
}

/* IUnknown */
STDMETHODIMP
QoSRateShapeAggregator::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXQoSRateShapeAggregator*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSRateShapeAggregator))
    {
        AddRef();
        *ppvObj = (IHXQoSRateShapeAggregator*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSRateShapeAggregator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSRateShapeAggregator::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

/* RateShaper container class: */
RateShaper::RateShaper (UINT16 nStreamNumber) :
    m_unStreamNumber (nStreamNumber),
    m_bBlocked (FALSE),
    m_pShaper (NULL)
{
}

RateShaper::~RateShaper ()
{
    HX_RELEASE(m_pShaper);
}

void 
RateShaper::SetShaper (IHXQoSRateShaper* pShaper)
{
    if (!pShaper)
    {
        HX_ASSERT(0);
        return;
    }

    HX_RELEASE(m_pShaper);
    m_pShaper = pShaper;
    m_pShaper->AddRef();
}

