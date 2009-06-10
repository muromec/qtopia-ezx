/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsocketdata.cpp,v 1.8 2007/07/06 21:57:58 jfinnecy Exp $ 
 *   
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.  
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
 * Alternatively, the contents of this file may be used under the 
 * terms of the GNU General Public License Version 2 (the 
 * "GPL") in which case the provisions of the GPL are applicable 
 * instead of those above. If you wish to allow use of your version of 
 * this file only under the terms of the GPL, and not to allow others  
 * to use your version of this file under the terms of either the RPSL 
 * or RCSL, indicate your decision by deleting the provisions above 
 * and replace them with the notice and other provisions required by 
 * the GPL. If you do not delete the provisions above, a recipient may 
 * use your version of this file under the terms of any one of the 
 * RPSL, the RCSL or the GPL. 
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
#include "hxassert.h"
#include "hxbuffer.h"
#include "hxnet.h"
#include "hxscope_lock.h"
#include "cringbuf.h"
#include "hxsocketdata.h"
#include "pckunpck.h"
#include "hxtlogutil.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXSocketData::HXSocketData(IUnknown* pContext)
: m_pRing(0)
, m_cbTotal(0)
, m_cbMax(0)
, m_pMutex(0)
, m_emptySignal(true)
, m_fullSignal(false)
, m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
}

HXSocketData::~HXSocketData()
{
    CleanUp();
}

bool HXSocketData::IsFull() const
{
    // return 'true' if we have exceeded inbound data limit or space for storing data
    HXScopeLock lock(m_pMutex);
    return (0 == GetFree() || (m_cbMax != 0 && m_cbTotal > m_cbMax));
}

// reset empty signal if we are not empty; return 'true' if we do a reset
bool HXSocketData::TryResetEmptySignal()
{
    HXScopeLock lock(m_pMutex);
    if (m_emptySignal && m_cbTotal > 0)
    {
        // we are not empty; it is ok to reset
        m_emptySignal = false;
        return true;
    }
    return false;
}

// reset full signal if we are empty; return 'true' if we do a reset
bool HXSocketData::TryResetFullSignal()
{
    HXScopeLock lock(m_pMutex);
    if (m_fullSignal && !IsFull())
    {
        // we are not full; it is ok to reset
        m_fullSignal = false;
        return true;
    }
    return false;
}

HX_RESULT HXSocketData::Init(UINT32 slotCount)
{
    HXLOGL3(HXLOG_NETW, "HXSocketData::Init(): init %lu slots; %lu bytes limit", slotCount, m_cbMax);

    HX_ASSERT(slotCount > 1);
    if (0 == slotCount)
    {
        return HXR_INVALID_PARAMETER;
    }

    m_pRing = new CRingBuffer(slotCount);

    // The ring buffer is thread safe as long as there is only one writer
    // and one reader thread; this mutex is needed because we track other
    // state as we add/remove items; the add/remove plus state update must
    // be atomic
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);	
    if (m_pRing && m_pMutex)
    {
        return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

HX_RESULT HXSocketData::Init(UINT32 cbMax, UINT32 cbAvgPacket)
{
    HX_ASSERT(cbMax != 0);
    HX_ASSERT(cbAvgPacket != 0);
    HX_ASSERT(cbMax >= cbAvgPacket);
    if (0 == cbMax || 0 == cbAvgPacket || cbMax < cbAvgPacket)
    {
        return HXR_INVALID_PARAMETER;
    }
    m_cbMax = cbMax;

    UINT32 slotCount = cbMax / cbAvgPacket;
    return Init(slotCount); 
}

bool HXSocketData::IsInitialized() const
{
    return m_pRing != 0;
}

// free slots
UINT32 HXSocketData::GetFree() const
{
    return m_pRing ? (m_pRing->Size() - GetCount()) : 0;
}

// items in container
UINT32 HXSocketData::GetCount() const
{
    return m_pRing ? m_pRing->Count() : 0;
}


void HXSocketData::CleanUp()
{
    if (m_pRing)
    {
        Data* pData;
        while (pData = reinterpret_cast<Data*>(m_pRing->Get()))
        {
	    HX_DELETE(pData);
	}

        HX_DELETE(m_pRing);
    }

    m_cbTotal = 0;
    m_emptySignal = true;
    m_fullSignal = false;
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
}

HX_RESULT HXSocketData::AddTail(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    HX_ASSERT(pBuf);
    HXSocketData::Data* pData = new Data(pBuf, pAddr);
    if(pData)
    { 
        HXScopeLock lock(m_pMutex);

        HX_ASSERT(!m_pRing->IsFull());
        m_pRing->Put(pData);
        m_cbTotal += pBuf->GetSize();
        // set full signal when buffer becomes full
        if (IsFull())
        {
            m_fullSignal = true;
        }
        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}



void HXSocketData::RemoveHead(IHXBuffer*& pBuf)
{
    IHXSockAddr* pAddr = 0;
    RemoveHead(pBuf, pAddr);
    HX_RELEASE(pAddr);
}

void HXSocketData::RemoveHead(IHXBuffer*& pBuf, IHXSockAddr*& pAddr)
{
    HX_ASSERT(m_pRing->Count() > 0);
    
    HXScopeLock lock(m_pMutex);

    Data* pData = reinterpret_cast<Data*>(m_pRing->Get());
    pData->Get(pBuf, pAddr);
    HX_DELETE(pData);
    HX_ASSERT(m_cbTotal >= pBuf->GetSize());
    m_cbTotal -= pBuf->GetSize();
    // set empty signal when buffer becomes empty
    if (m_pRing->Count() == 0)
    {
        m_emptySignal = true;
    }
}

void HXSocketData::PeekHead(IHXBuffer*& pBuf)
{
    IHXSockAddr* pAddr = 0;
    PeekHead(pBuf, pAddr);
    HX_RELEASE(pAddr);
}


void HXSocketData::PeekHead(IHXBuffer*& pBuf, IHXSockAddr*& pAddr)
{
    HX_ASSERT(m_pRing->Count() > 0);
    
    HXSocketData::Data* pData = reinterpret_cast<Data*>(m_pRing->Peek());
    HX_ASSERT(pData);
    pData->Get(pBuf, pAddr);
}




HXSocketData::Data::Data(IHXBuffer* pBuf, IHXSockAddr* pAddr)
: m_pBuf(pBuf)
, m_pAddr(pAddr)
{
    HX_ASSERT(m_pBuf);
    m_pBuf->AddRef();
    if(m_pAddr)
    {
        m_pAddr->AddRef();
    }
}

void HXSocketData::Data::Get(IHXBuffer*& pBuf, IHXSockAddr*& pAddr)
{
    pBuf = m_pBuf;
    HX_ASSERT(pBuf);
    pBuf->AddRef();

    pAddr = m_pAddr;
    if (pAddr)
    {
        pAddr->AddRef();
    }
}


HXSocketData::Data::~Data()
{
    HX_RELEASE(m_pBuf);
    HX_RELEASE(m_pAddr);
}