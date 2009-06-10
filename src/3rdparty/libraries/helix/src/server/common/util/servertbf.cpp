/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servertbf.cpp,v 1.4 2005/09/12 18:41:58 jc Exp $ 
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

#include "sink.h"
#include "servertbf.h"
#include "hxtick.h"
#include <stdio.h>
//#define RSD_LIVE_DEBUG 

CServerTBF::CServerTBF() :
  m_lTokens(0)
, m_lCeiling(0)
, m_ulRefCount(0)
, m_ulBandwidth(0)
, m_pScheduler(NULL)
{

}

CServerTBF::~CServerTBF()
{

}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      CServerTBF::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
CServerTBF::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXTokenBufferFilter))
    {
        AddRef();
        *ppvObj = (IHXTokenBufferFilter*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXTokenBufferFilter*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      CServerTBF::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
CServerTBF::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      CServerTBF::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
CServerTBF::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    
    delete this;
    return 0;
}   

STDMETHODIMP
CServerTBF::Init(IUnknown* pContext)
{
    pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    m_LastTimeStamp = HX_GET_BETTERTICKCOUNT(); 
    return HXR_OK;
}

STDMETHODIMP
CServerTBF::AddTokens(UINT32 ulTokens)
{
    m_lTokens += ulTokens;
    if(m_lTokens > m_lCeiling)
    {
#ifdef RSD_LIVE_DEBUG
        printf("CServerTBF::AddTokens, tokens %dBps clipped to ceiling, %dBps\n",
                                            m_lTokens, m_lCeiling);
#endif
        m_lTokens = m_lCeiling;
    }
#ifdef RSD_LIVE_DEBUG
    printf("CServerTBF::AddTokens %d ", ulTokens);
    printf("m_lTokens: %d, m_lCeiling: %d\n", m_lTokens, m_lCeiling);
#endif
    return HXR_OK; 
}

STDMETHODIMP
CServerTBF::RemoveTokens(UINT32 ulTokens)
{
    m_lTokens -= ulTokens;
    if(m_lTokens < 0)
    {
        return HXR_BLOCKED;
    }

    return HXR_OK;
}

STDMETHODIMP_(INT32)
CServerTBF::GetTokenCount()
{
    return m_lTokens;
}


STDMETHODIMP
CServerTBF::SetMinTokenCeiling(UINT32 ulMinCeiling)
{
#ifdef RSD_LIVE_DEBUG
    printf("CServerTBF::SetMinTokenCeiling, %dBps\n", ulMinCeiling/8);
#endif
    //convert to byte/s, for easy manipulation
    m_ulMinCeiling = ulMinCeiling/8;
    return HXR_OK;
}

STDMETHODIMP
CServerTBF::SetBandwidth(UINT32 ulBandwidth)
{
    //convert to byte/s, for easy manipulation
    ulBandwidth = ulBandwidth/8;

    // if we change bandwidth, the tokens should be shrinked or expanded according to
    // the bandwidth ratio.
    if(m_ulBandwidth)
    {
        float ratio = (float)ulBandwidth/(float)m_ulBandwidth;
        m_lTokens = (float)m_lTokens*ratio;
    }
    else
    {
        // initially the tokens worth .1 seconds. 
        m_lTokens = ulBandwidth/10;
    }
    m_ulBandwidth = ulBandwidth;

    //we set the ceiling as one second worth of tokens
    m_lCeiling = m_ulBandwidth;

    if (m_lCeiling < m_ulMinCeiling)
    {
        m_lCeiling = (INT32)m_ulMinCeiling;
    }

#ifdef RSD_LIVE_DEBUG 
    printf("CServerTBF::SetBW, bandwidth %dBps, ceiling %dBps\n", m_ulBandwidth, m_lCeiling);
#endif

    return HXR_OK;
}

STDMETHODIMP
CServerTBF::UpdateTokens()
{
    UINT32 tCurTime = HX_GET_BETTERTICKCOUNT();
    UINT32 timediff = tCurTime - m_LastTimeStamp;

    UINT64 ulTokens = m_ulBandwidth * timediff;

    ulTokens = ulTokens/1000;

#ifdef RSD_LIVE_DEBUG 
    printf("CServerTBF::UpdateTokens, timediff: %d, tokens added: %d\n", 
            timediff, (UINT32)ulTokens);
#endif
       
    AddTokens((UINT32)ulTokens);
    m_LastTimeStamp = tCurTime;
    return HXR_OK;
}

