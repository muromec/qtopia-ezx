/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"

#include "hxassert.h"
#include "hxslist.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxrendr.h"
#include "hxformt.h"
#include "hxengin.h"

#include "smppayld.h"


SimplePayloadFormat::SimplePayloadFormat()
    : m_lRefCount	    (0)
    , m_pStreamHeader	    (NULL)
    , m_pInputPackets	    (NULL)
    , m_bFlushed	    (FALSE)
{
    m_pInputPackets = new CHXSimpleList();
}

SimplePayloadFormat::~SimplePayloadFormat()
{
    HX_RELEASE(m_pStreamHeader);

    if (!m_pInputPackets->IsEmpty())
    {
	IHXPacket* pPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
	HX_RELEASE(pPacket);
    }
    HX_DELETE(m_pInputPackets);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP 
SimplePayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPayloadFormatObject))
    {
	AddRef();
	*ppvObj = (IHXPayloadFormatObject*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
SimplePayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
SimplePayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP 
SimplePayloadFormat::Init(IUnknown* pContext,
			  HXBOOL bPacketize)
{
    return HXR_OK;
}

STDMETHODIMP
SimplePayloadFormat::Reset()
{
    // Release all input packets we have stored
    if (!m_pInputPackets->IsEmpty())
    {
	IHXPacket* pPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
	HX_RELEASE(pPacket);
    }

    m_bFlushed = FALSE;

    return HXR_OK;
}

STDMETHODIMP 
SimplePayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);
    m_pStreamHeader = pHeader;
    m_pStreamHeader->AddRef();

    return HXR_OK;
}

STDMETHODIMP 
SimplePayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_ASSERT(m_pStreamHeader);
    pHeader = m_pStreamHeader;
    pHeader->AddRef();

    return HXR_OK;
}

STDMETHODIMP 
SimplePayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_ASSERT(pPacket);

    // Add this packet to our list of input packets
    pPacket->AddRef();
    m_pInputPackets->AddTail(pPacket);

    return HXR_OK;
}

STDMETHODIMP 
SimplePayloadFormat::GetPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT hResult = HXR_OK;
    pPacket = NULL;

    if (m_pInputPackets->IsEmpty())
    {
	if (m_bFlushed)
	{
	    // We have used up all available input
	    hResult = HXR_STREAM_DONE;
	}
	else
	{
	    // We don't have enough input 
	    // data to produce a packet
	    hResult = HXR_INCOMPLETE;
	}
    }
    else
    {
	pPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
    }

    return hResult;
}

STDMETHODIMP 
SimplePayloadFormat::Flush()
{
    m_bFlushed = TRUE;

    return HXR_OK;
}
