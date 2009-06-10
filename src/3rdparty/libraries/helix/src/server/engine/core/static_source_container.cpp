/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: static_source_container.cpp,v 1.4 2007/08/18 00:21:11 dcollins Exp $ 
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
/*  RealSystem Server: Static File-to-Live Stream Bridge
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "static_source_container.h"
#include "hxassert.h"
#include "timeval.h"

StaticSourceContainer::StaticSourceContainer(IUnknown*           pContext,
					     IHXPSourceControl* pSourceControl)
    : m_bReady(FALSE)
    , m_bPacketsStarted(FALSE)
    , m_bAdvanceSchedulerTime(FALSE)
    , m_bOneStreamStarted(FALSE)
    , m_bGetPacketOutstanding(FALSE)
    , m_bDone(FALSE)

    , m_lRefCount(0)
    , m_ulStopPacketsSeen(0)
    , m_nStreamCount(0)

    , m_pSourcePackets(NULL)
    , m_pScheduler(NULL)
    , m_pThreadSafeScheduler(NULL)
    , m_pSink(NULL)
    , m_ppPacketStreams(NULL)

    , m_pContext(pContext)
    , m_pSourceControl(pSourceControl)
{
    if (m_pContext)
    {
	m_pContext->AddRef();

	HX_VERIFY(HXR_OK == m_pContext->QueryInterface(IID_IHXScheduler,
						       (void **)&m_pScheduler));
	
	HX_VERIFY(HXR_OK == m_pContext->QueryInterface(IID_IHXThreadSafeScheduler,
						       (void **)&m_pThreadSafeScheduler));
    }

    if (m_pSourceControl)
    {
	m_pSourceControl->AddRef();

	HX_VERIFY(HXR_OK == pSourceControl->QueryInterface(IID_IHXPSourcePackets,
							   (void **)&m_pSourcePackets));
    }
}

StaticSourceContainer::~StaticSourceContainer()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pSourcePackets);

    if (m_pSourceControl)
    {
	m_pSourceControl->Done();
	HX_RELEASE(m_pSourceControl);
    }

    if (m_pSink)
    {
	m_pSink->Done();
	HX_RELEASE(m_pSink);
    }

    if (m_ppPacketStreams)
    {
	for (UINT32 i = 0; i < m_nStreamCount; i++)
	{
	    if (m_ppPacketStreams [i]->m_Handle)
	    {
		m_pThreadSafeScheduler->Remove(m_ppPacketStreams[i]->m_Handle);
	    }

	    HX_RELEASE(m_ppPacketStreams [i]);
	}
	HX_VECTOR_DELETE(m_ppPacketStreams);
    }

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pThreadSafeScheduler);
}

/* IHXRawSourceObject methods */

STDMETHODIMP
StaticSourceContainer::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXRawSourceObject*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXRawSourceObject))
    {
	AddRef();
	*ppvObj = (IHXRawSourceObject*)this;
	return HXR_OK;
    }
    else if ( (IsEqualIID(riid, IID_IHXPacketTimeOffsetHandler)) && m_pSourcePackets)
    {
	return m_pSourcePackets->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXLiveFileFormatInfo)) && m_pSourcePackets)
    {
	return m_pSourcePackets->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
StaticSourceContainer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
StaticSourceContainer::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
StaticSourceContainer::Init(IUnknown* pUnknown)
{
    IHXRawSinkObject*	pSink;
    HX_RESULT		h_result;

    h_result = pUnknown->QueryInterface(IID_IHXRawSinkObject,
                                        (void**)&pSink);
    if (HXR_OK == h_result)
    {
	m_pSink = new SinkContainer(pSink);
	m_pSink->AddRef();

	/*
	 * If the SourceControl is ready to proceed with our Init.
	 * we could be released... so AddRef ourselves
	 */
	AddRef();

	if (m_pSourcePackets)
	{
	    h_result = m_pSourcePackets->Init(this);
	}
	else
	{
	    // if there are no source packets, we must be done
	    // so, proceed without error.
	    goto InitOK;
	}

	h_result = m_pSourceControl->Init((IHXPSinkControl*)m_pSink);

	if (HXR_OK == h_result)
	{
	    goto InitOK;
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
StaticSourceContainer::Done()
{
    if (m_bDone)
    {
	return HXR_OK;
    }

    m_bDone = TRUE;

    for (UINT32 i = 0; i < m_nStreamCount; i++)
    {
	if ( (m_ppPacketStreams) && (m_ppPacketStreams [i]) &&
	     (m_ppPacketStreams [i]->m_Handle) )
	{
            m_pThreadSafeScheduler->Remove(m_ppPacketStreams[i]->m_Handle);
	}

        HX_RELEASE(m_ppPacketStreams [i]);
    }
    HX_VECTOR_DELETE(m_ppPacketStreams);

    if (m_pSourceControl)
    {
	m_pSourceControl->Done();
	HX_RELEASE(m_pSourceControl);
    }

    if (m_pSink)
    {
	m_pSink->Done();
	HX_RELEASE(m_pSink);
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

    HX_RELEASE (m_pSourcePackets);
    return HXR_OK;
}

STDMETHODIMP
StaticSourceContainer::GetFileHeader()
{
    return m_pSourceControl->GetFileHeader(m_pSink);
}

STDMETHODIMP
StaticSourceContainer::GetStreamHeader(UINT16 unStreamNumber)
{
    return m_pSourceControl->GetStreamHeader(m_pSink, unStreamNumber);
}

STDMETHODIMP
StaticSourceContainer::PacketReady(HX_RESULT               ulStatus,
				   IHXPacket*             pPacket)
{
    if ( (ulStatus == HXR_OK) && (pPacket) && m_bPacketsStarted)
    {
	m_pSink->PacketReady(HXR_OK, pPacket);

	HXTime tmp;
	gettimeofday(&tmp, 0);
	Timeval tNow;

	tNow.tv_sec = tmp.tv_sec;
	tNow.tv_usec = tmp.tv_usec;

	Timeval tPacket;
	UINT32 ulPacketTime = pPacket->GetTime();
	tPacket.tv_sec = ulPacketTime / 1000;
	tPacket.tv_usec = (ulPacketTime % 1000) * 1000;

	/*
	 * Put packet time on scheduler timeline.
	 */
	tPacket += m_StartTime;
	UINT16 uStreamNo = pPacket->GetStreamNumber();

	/*
	 * If it is not time yet for this packet....
	 */
	if(tNow < tPacket)
	{
	    /*
	     * convert tPacket to scheduler time.
	     */
	    if(m_bAdvanceSchedulerTime)
	    {
		tPacket += m_SchedulerTimeOffset;
	    }
	    else
	    {
		tPacket -= m_SchedulerTimeOffset;
	    }

	    m_ppPacketStreams[uStreamNo]->m_pTimeToSchedule.tv_sec =  tPacket.tv_sec;
	    m_ppPacketStreams[uStreamNo]->m_pTimeToSchedule.tv_usec = tPacket.tv_usec;
	    m_ppPacketStreams[uStreamNo]->ScheduleNextPacket();
	}
	else
	{
	    m_pSourcePackets->GetPacket(uStreamNo);
	}
    }

    return HXR_OK;
}

STDMETHODIMP
StaticSourceContainer::StartPackets(UINT16 unStreamNumber)
{
    /* this class never sees the stream headers
     * so we need to grow the vector of
     * GetPackeCallbacks as needed.
     */
    m_nStreamCount++;

    GetPacketCallback** pOld = m_ppPacketStreams;

    m_ppPacketStreams = new GetPacketCallback* [m_nStreamCount];
    memset(m_ppPacketStreams, 0, sizeof(GetPacketCallback*) * m_nStreamCount);

    if (pOld != NULL)
    {
	memcpy(m_ppPacketStreams, pOld, sizeof(GetPacketCallback*) * (m_nStreamCount-1));
    }

    HX_VECTOR_DELETE(pOld);

    m_ppPacketStreams [unStreamNumber] = new GetPacketCallback (this, unStreamNumber);
    m_ppPacketStreams [unStreamNumber]->AddRef();

    /* Setup timing adjustments for GetPacket scheduling */
    if(!m_bOneStreamStarted)
    {
	m_bOneStreamStarted = TRUE;
	
	HXTime tmp;
	gettimeofday(&tmp, 0);
	
	m_StartTime.tv_sec  = tmp.tv_sec;
	m_StartTime.tv_usec = tmp.tv_usec;

	HXTimeval tv = m_pScheduler->GetCurrentSchedulerTime();
	Timeval tSched((INT32)tv.tv_sec, (INT32)tv.tv_usec);
	
	if(tSched > m_StartTime)
	{
	    m_bAdvanceSchedulerTime = TRUE;
	    m_SchedulerTimeOffset = tSched - m_StartTime;
	}
	else
	{
	    m_bAdvanceSchedulerTime = FALSE;
	    m_SchedulerTimeOffset = m_StartTime - tSched;
	}
    }

    /* start the flow of packets */
    m_ppPacketStreams[unStreamNumber]->m_bPacketsStarted = m_bPacketsStarted = TRUE;

    m_pSourcePackets->GetPacket(unStreamNumber);

    return HXR_OK;
}

STDMETHODIMP
StaticSourceContainer::StopPackets(UINT16 unStreamNumber)
{
    m_bPacketsStarted = FALSE;

    if ( (m_ppPacketStreams) && (m_ppPacketStreams [unStreamNumber]) )
    {
	if  (m_ppPacketStreams [unStreamNumber]->m_Handle)
	{
            m_pThreadSafeScheduler->Remove(m_ppPacketStreams [unStreamNumber]->m_Handle);
	}

	m_ppPacketStreams [unStreamNumber]->m_bPacketsStarted = FALSE;
    }

    return HXR_OK;
}


/************************************************************************
 *  GetPacketCallback class
 ***********************************************************************/

GetPacketCallback::GetPacketCallback(StaticSourceContainer* pSource, UINT16 unStreamNumber)
    : m_pOwner	                 (NULL)
    , m_lRefCount	         (0)
    , m_Handle		         (0)
    , m_unStreamNo               (unStreamNumber)
    , m_bPacketsStarted	         (FALSE)
{
    if (pSource)
    {
	m_pOwner = pSource;
	m_pOwner->AddRef();
    }

    m_pTimeToSchedule.tv_sec = m_pTimeToSchedule.tv_usec = 0;
}

GetPacketCallback::~GetPacketCallback()
{
    HX_RELEASE(m_pOwner);
}

STDMETHODIMP
GetPacketCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IHXCallback*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXCallback))
    {
	AddRef();
	*ppvObj = (IHXCallback*)this;
	return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
GetPacketCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
GetPacketCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
GetPacketCallback::Func()
{
    m_Handle = 0;

    m_pOwner->m_pSourcePackets->GetPacket(m_unStreamNo);

    return HXR_OK;
}

void
GetPacketCallback::ScheduleNextPacket()
{
    m_Handle = m_pOwner->m_pThreadSafeScheduler->AbsoluteEnter(this, m_pTimeToSchedule);
}

