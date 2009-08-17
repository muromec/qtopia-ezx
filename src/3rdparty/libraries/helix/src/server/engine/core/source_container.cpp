/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: source_container.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "hxengin.h"
#include "source_container.h"
#include "hxassert.h"

SourceContainer::SourceContainer(IUnknown* pContext, 
				 IHXPSourceControl* pSourceControl):
    m_lRefCount(0),
    m_pContext(pContext),
    m_pSourceControl(pSourceControl),
    m_pSourceLivePackets(0),
    m_pSink(0),
    m_bPacketsStarted(FALSE)
{
    if (m_pContext)
    {
	m_pContext->AddRef();
    }

    HX_VERIFY(HXR_OK == pSourceControl->
	      QueryInterface(IID_IHXPSourceLivePackets,
			     (void **)&m_pSourceLivePackets));

    m_pSourceControl->AddRef();
}

SourceContainer::~SourceContainer()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pSourceLivePackets);

    if (m_pSourceControl)
    {
	m_pSourceControl->Done();
	m_pSourceControl->Release();
	m_pSourceControl = 0;
    }

    if (m_pSink)
    {
	m_pSink->Done();
	m_pSink->Release();
	m_pSink = 0;
    }
}

/* IHXRawSourceObject methods */

STDMETHODIMP
SourceContainer::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXPSinkPackets*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXRawSourceObject))
    {
	AddRef();
	*ppvObj = (IHXRawSourceObject*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXASMSource))
    {
	return m_pSourceLivePackets->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
SourceContainer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
SourceContainer::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
SourceContainer::Init(IUnknown* pUnknown)
{
    IHXRawSinkObject*	pSink;
    HX_RESULT		h_result;

    h_result = pUnknown->QueryInterface(IID_IHXRawSinkObject,
                                        (void**)&pSink);

    if (HXR_OK == h_result)
    {
	m_pSink = new SinkContainer(pSink);
	m_pSink->AddRef();

	/* XXXJHUG
	 * If the SourceControl is ready to proceed with our Init.
	 * we could be released... so AddRef ourselves
	 */
	AddRef();
	h_result = m_pSourceControl->Init((IHXPSinkControl*)m_pSink);

	if (HXR_OK == h_result)
	{
	    if (m_pSourceLivePackets)
	    {
		h_result = m_pSourceLivePackets->Init(this);

		if (HXR_OK == h_result)
		{
		    goto InitOK;
		}
	    }
	    else
	    {
		// if there are no source packets, we must be done
		// so, proceed without error.
		goto InitOK;
	    }
	}

	m_pSink->Release();
	m_pSink = 0;

	Done();

InitOK:
	pSink->Release();
	// matches add ref above.
	Release();
    }

    return h_result;
}

STDMETHODIMP
SourceContainer::Done()
{
    if (m_pSourceControl)
    {
	m_pSourceControl->Done();
	m_pSourceControl->Release();
	m_pSourceControl = 0;
    }

    if (m_pSink)
    {
	m_pSink->Done();
	m_pSink->Release();
	m_pSink = 0;
    }

    /*
     * XXXJHUG We just removed our callback so we should be able to
     * release m_pSourcePackets - this way we don't create a Circular
     * reference if we are closed in our Init Function.
     * Normally this is prevented because Done is called on 
     * m_pSourceControl which releases us from this refference too.
     * but when done is called after we init the m_pSourceControl and
     * before we init m_pSourcePackets neither ever get released
     */
    HX_RELEASE(m_pSourceLivePackets);

    return HXR_OK;
}

STDMETHODIMP
SourceContainer::GetFileHeader()
{
    return m_pSourceControl->GetFileHeader(m_pSink);
}

STDMETHODIMP
SourceContainer::GetStreamHeader(UINT16 unStreamNumber)
{
    return m_pSourceControl->GetStreamHeader(m_pSink, unStreamNumber);
}

STDMETHODIMP
SourceContainer::PacketReady(HX_RESULT               ulStatus,
			     IHXPacket*             pPacket)
{
    if (m_bPacketsStarted)
    {
	m_pSink->PacketReady(HXR_OK, pPacket);
    }

    return HXR_OK;
}

STDMETHODIMP
SourceContainer::StartPackets(UINT16 unStreamNumber)
{
    m_bPacketsStarted = TRUE;
    
    if (m_pSourceLivePackets)
    {
	m_pSourceLivePackets->StartPackets(unStreamNumber);
    }
	  
    return HXR_OK;
}

STDMETHODIMP
SourceContainer::StopPackets(UINT16 unStreamNumber)
{
    m_bPacketsStarted = FALSE;

    if (m_pSourceLivePackets)
    {
	m_pSourceLivePackets->StopPackets(unStreamNumber);
    }

    return HXR_OK;
}


