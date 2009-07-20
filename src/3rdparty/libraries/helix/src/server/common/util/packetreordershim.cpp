/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: packetreordershim.cpp,v 1.5 2009/04/07 19:19:31 jgordon Exp $ 
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
#include "debug.h"
#include "hxtypes.h"

#include "hxbuffer.h"
#include "hxdeque.h"

#include "hxccf.h"		//  IHXCommonClassFactory
#include "hxengin.h"		//  IHXScheduler
#include "hxsrc.h"		//  IHXRawSinkObject
#include "sink.h"		//  IHXPSinkPackets
#include "ihxpacketorderer.h"	//  IHXPacketOrderer

#include "pktreorderqueue.h"
#include "packetreordershim.h"

static const UINT32 kDefaultPacketSendInterval = 200;

BEGIN_INTERFACE_LIST(CPacketOrderShim)
    INTERFACE_LIST_ENTRY(IID_IHXPacketOrderer, IHXPacketOrderer)
    INTERFACE_LIST_ENTRY(IID_IHXPSinkPackets, IHXPSinkPackets)
END_INTERFACE_LIST

CPacketOrderShim::CPacketOrderShim()
    : m_bInitialized(FALSE)
    , m_pContext(NULL)
    , m_pClassFactory(NULL)
    , m_pScheduler(NULL)
    , m_pSinkObject(NULL)
    , m_ppPacketAgedCallback(NULL)
    , m_nAgingDuration(0)
    , m_nPacketSendInterval(kDefaultPacketSendInterval)
    , m_bFirstPacket(TRUE)
    , m_bDelayForOrdering(FALSE)
    , m_ulStreamCount(0)
    , m_ulStreamCountTerminated(0)
    , m_pqueueInOrderPacket(NULL)
#ifdef _LOG_PACKET_ORDERING
    , m_hLogFile(NULL)
#endif	//  _LOG_PACKET_ORDERING
{
#ifdef _LOG_PACKET_ORDERING
    m_hLogFile = fopen(_LOG_PACKET_ORDERING_FILENAME, "a+");
#endif	//  _LOG_PACKET_ORDERING
}

CPacketOrderShim::~CPacketOrderShim()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pSinkObject);

#ifdef _LOG_PACKET_ORDERING
    fclose(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING
}

//	*** IHXPacketOrderer methods ***
STDMETHODIMP
CPacketOrderShim::Initialize(
    IUnknown* pContext, 
    IHXRawSinkObject* pSinkObject, 
    UINT32 ulStreamCount,
    UINT32 nAgingMS,
    UINT32 nPacketSendInterval)
{
    HX_RESULT res = HXR_OK;

    if(!pContext || !pSinkObject)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else if(!ulStreamCount)
    {
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    else if(m_bInitialized)
    {
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    else
    {
	m_pContext = pContext;
	HX_ADDREF(m_pContext);
	res = pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
	HX_ASSERT(SUCCEEDED(res));
	if(SUCCEEDED(res))
	{
	    res = pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pClassFactory);
	    HX_ASSERT(SUCCEEDED(res));
	}
	m_pSinkObject = pSinkObject;
	HX_ADDREF(m_pSinkObject);

	//  Set up for queuing packets by stream
	m_ulStreamCount = ulStreamCount;
	m_pqueueInOrderPacket = new CInorderPacketQueue[m_ulStreamCount];
	if(!m_pqueueInOrderPacket)
	{
	    res = HXR_OUTOFMEMORY;
	}

	if(SUCCEEDED(res))
	{
	    m_ppPacketAgedCallback = new CPacketAgedCallback*[m_ulStreamCount];
	    if(!m_ppPacketAgedCallback)
	    {
		res = HXR_OUTOFMEMORY;
	    }

	    for(UINT32 ulStream = 0; ulStream < m_ulStreamCount && SUCCEEDED(res); ulStream++)
	    {
		m_ppPacketAgedCallback[ulStream] = new CPacketAgedCallback(this);
		HX_ADDREF(m_ppPacketAgedCallback[ulStream]);
		if(!m_ppPacketAgedCallback[ulStream])
		{
		    res = HXR_OUTOFMEMORY;
		}
	    }
	}

	m_nAgingDuration = nAgingMS * 1000;
	m_nPacketSendInterval = nPacketSendInterval;

	m_bInitialized = TRUE;
    }

    return res;
}

STDMETHODIMP
CPacketOrderShim::Terminate(INT32 nStreamNumber)
{
    HX_RESULT res = HXR_OK;

    if(!m_bInitialized)
    {
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    else
    {
#ifdef _LOG_PACKET_ORDERING
	fprintf(m_hLogFile, "Terminate Packet Ordering for streams %d\n", nStreamNumber);
	fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING

	if(nStreamNumber == ALL_STREAMS)
	{

	    for(UINT32 ulStream = 0; ulStream < m_ulStreamCount; ulStream++)
	    {
#ifdef _LOG_PACKET_ORDERING
		fprintf(m_hLogFile, "Terminate Scheduler for stream %d\n", ulStream);
		fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING

		if(m_ppPacketAgedCallback[ulStream]->m_hCB)
		{
#ifdef _LOG_PACKET_ORDERING
		    fprintf(m_hLogFile, "Remove Scheduler Handle for stream#: %d, handle: %d\n", ulStream, m_ppPacketAgedCallback[ulStream]->m_hCB);
		    fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING
		    m_pScheduler->Remove(m_ppPacketAgedCallback[ulStream]->m_hCB);
		    m_ppPacketAgedCallback[ulStream]->m_hCB = NULL; 
		}

		HX_RELEASE(m_ppPacketAgedCallback[ulStream]);
	    }

	    m_ulStreamCountTerminated = m_ulStreamCount;
	    //	Flush out all the packets from the queues
	    res = SendPackets(TRUE, nStreamNumber);
	}
	else
	{
#ifdef _LOG_PACKET_ORDERING
	    fprintf(m_hLogFile, "Terminate Scheduler for stream %d\n", nStreamNumber);
	    fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING

	    if(m_ppPacketAgedCallback[nStreamNumber]->m_hCB)
	    {
#ifdef _LOG_PACKET_ORDERING
		fprintf(m_hLogFile, "Remove Scheduler Handle for stream#: %d, handle: %d\n", nStreamNumber, m_ppPacketAgedCallback[nStreamNumber]->m_hCB);
		fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING
		m_pScheduler->Remove(m_ppPacketAgedCallback[nStreamNumber]->m_hCB);
		m_ppPacketAgedCallback[nStreamNumber]->m_hCB = NULL;
	    }

	    HX_RELEASE(m_ppPacketAgedCallback[nStreamNumber]);

	    m_ulStreamCountTerminated++;
	    //	Flush out all the packets from the queues
	    res = SendPackets(TRUE, nStreamNumber);
	}

	if(m_ulStreamCountTerminated == m_ulStreamCount)
	{
#ifdef _LOG_PACKET_ORDERING
	    fprintf(m_hLogFile, "Terminate - Destroy all stream based objects\n");
	    fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING

	    HX_VECTOR_DELETE(m_ppPacketAgedCallback);
	    HX_VECTOR_DELETE(m_pqueueInOrderPacket);

	    m_bInitialized = FALSE;
	}
    }

    return res;
}

STDMETHODIMP
CPacketOrderShim::GetContext(IUnknown** ppContext)
{
    HX_RESULT res = HXR_OK;

    if(!ppContext)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else if(!m_pContext)
    {
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    else
    {
	*ppContext = m_pContext;
	(*ppContext)->AddRef();
    }

    return res;
}

STDMETHODIMP
CPacketOrderShim::GetOrderingDuration(UINT32* pnAgingMS)
{
    HX_RESULT res = HXR_OK;

    if(!pnAgingMS)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else
    {
	*pnAgingMS = m_nAgingDuration;
    }

    return res;
}

STDMETHODIMP
CPacketOrderShim::GetPacketSendInterval(UINT32* pnPacketSendInterval)
{
    HX_RESULT res = HXR_OK;

    if(!pnPacketSendInterval)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else
    {
	*pnPacketSendInterval = m_nPacketSendInterval;
    }

    return res;
}

STDMETHODIMP
CPacketOrderShim::GetStreamCount(UINT32* pulStreamCount)
{
    HX_RESULT res = HXR_OK;

    if(!pulStreamCount)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else
    {
	*pulStreamCount = m_ulStreamCount;
    }

    return res;
}


STDMETHODIMP
CPacketOrderShim::GetPacketSink(IHXRawSinkObject** ppSinkObject)
{
    HX_RESULT res = HXR_OK;

    if(!ppSinkObject)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else if(!m_pSinkObject)
    {
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    else
    {
	*ppSinkObject = m_pSinkObject;
	(*ppSinkObject)->AddRef();
    }

    return res;
}

//	*** IHXPSinkPackets ***
STDMETHODIMP
CPacketOrderShim::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    HX_RESULT res = status;

    if(FAILED(res))
    {
	HX_ASSERT(FALSE);
    }
    else if(!pPacket)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else if(!m_bInitialized)
    {
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    else
    {
	//  Check to see if this is a low latency stream
	if(m_bFirstPacket)
	{
	    IHXServerPacketExt* spServerPacketExt = NULL;
	    res = pPacket->QueryInterface(IID_IHXServerPacketExt, (void**)&spServerPacketExt);
	    if(SUCCEEDED(res))
	    {
		m_bDelayForOrdering = spServerPacketExt->SupportsLowLatency();

#ifdef _LOG_PACKET_ORDERING
		fprintf(m_hLogFile, "Delay Packets for Ordering = %d\n", m_bDelayForOrdering);
		fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING

		HX_RELEASE(spServerPacketExt);
	    }

	    m_bFirstPacket = FALSE;
	}

	if(m_bDelayForOrdering)
	{
	    res = DelayPacketForOrdering(pPacket);
	}
	else
	{
	    res = m_pSinkObject->PacketReady(res, pPacket);
	}
    }

    return res;
}

HX_RESULT
CPacketOrderShim::DelayPacketForOrdering(IHXPacket* pPacket)
{
    HX_RESULT res = HXR_OK;

    //  Check to see that this packet is actually unordered before going 
    //  through the steps of ordering.
    IHXServerPacketExt* spServerPacketExt = NULL;
    res = pPacket->QueryInterface(IID_IHXServerPacketExt, (void**)&spServerPacketExt);
    if(SUCCEEDED(res))
    {
	CQueueEntry* pNewQueueEntry = new CQueueEntry(spServerPacketExt, 1);
	if(pNewQueueEntry)
	{
	    HXTimeval thxNow = m_pScheduler->GetCurrentSchedulerTime();

	    //  Entry Time is now
	    pNewQueueEntry->m_tTime.tv_sec = thxNow.tv_sec;
	    pNewQueueEntry->m_tTime.tv_usec = thxNow.tv_usec;
	    pNewQueueEntry->m_SequenceNumber = spServerPacketExt->GetStreamSeqNo();

	    //  Add to queues on a per stream basis
	    SequenceNumber nNewSeqNo = spServerPacketExt->GetStreamSeqNo();
	    res = m_pqueueInOrderPacket[pPacket->GetStreamNumber()].add(pNewQueueEntry, nNewSeqNo);

#ifdef _LOG_PACKET_ORDERING
	    fprintf(m_hLogFile, "Queuing packet for ordering time: %d, aging duration: %d, packet time: %d, stream#: %d, Stream Seq#: %d, packet depth: %d\n",
		(pNewQueueEntry->m_tTime.tv_sec * 1000) + pNewQueueEntry->m_tTime.tv_usec,
		m_nAgingDuration,
		pPacket->GetTime(), pPacket->GetStreamNumber(), pNewQueueEntry->m_SequenceNumber,
		m_pqueueInOrderPacket[pPacket->GetStreamNumber()].size());
	    fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING

	    //  If this is the first packet on the queue, then set the scheduler
	    //  to expire this packet in the aging duration.
	    if(SUCCEEDED(res) && 
		!m_ppPacketAgedCallback[pPacket->GetStreamNumber()]->m_hCB)
	    {
		// No callback currently scheduled, so schedule one
		m_ppPacketAgedCallback[pPacket->GetStreamNumber()]->m_hCB = m_pScheduler->RelativeEnter(m_ppPacketAgedCallback[pPacket->GetStreamNumber()], m_nPacketSendInterval);
#ifdef _LOG_PACKET_ORDERING
		fprintf(m_hLogFile, "Delay: Schedule Callback stream#: %d, handle: %d\n", pPacket->GetStreamNumber(), m_ppPacketAgedCallback[pPacket->GetStreamNumber()]->m_hCB);
		fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING
	    }
	}
	else
	{
	    res = HXR_OUTOFMEMORY;
	}

	HX_RELEASE(spServerPacketExt);
    }
    else
    {
	//  If using this class, then expect that the stream is unordered
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    
    return res;
}

HX_RESULT
CPacketOrderShim::SendPackets(BOOL bSendAll, INT32 nStreamNumber)
{
    HX_RESULT res = HXR_OK;

    if(nStreamNumber == ALL_STREAMS)
    {
	for(UINT32 ulStream = 0; ulStream < m_ulStreamCount && SUCCEEDED(res); ulStream++)
	{
	    res = SendPacketsForStream(bSendAll, ulStream);
	}
    }
    else
    {
	res = SendPacketsForStream(bSendAll, nStreamNumber);
    }

    return res;
}

HX_RESULT
CPacketOrderShim::SendPacketsForStream(BOOL bSendAll, INT32 nStreamNumber)
{
    HX_RESULT res = HXR_OK;

    //  Pull all packets from the queue that have sufficiently aged
    //  Aging is when we've come off the scheduler and the aged time is equal to or
    //  exceeds the aging time (e.g. the time we told the scheduler to fire on)
    BOOL bSentAllAged = FALSE;
    CQueueEntry* pQueueEntry = m_pqueueInOrderPacket[nStreamNumber].peek_front();
    while(SUCCEEDED(res) && pQueueEntry && !bSentAllAged)
    {
	Timeval tTimeAgedOnQueue = pQueueEntry->m_tTime;
	tTimeAgedOnQueue += m_nAgingDuration;

	HXTimeval thxNow = m_pScheduler->GetCurrentSchedulerTime();
	Timeval tNow;
	tNow.tv_sec = thxNow.tv_sec;
	tNow.tv_usec = thxNow.tv_usec;

	if(bSendAll || tTimeAgedOnQueue <= tNow)
	{
	    m_pqueueInOrderPacket[nStreamNumber].pop_front();

	    //  If we've aged and the packet still hasn't arrived then create
	    //  A lost packet!
	    if(!pQueueEntry->m_pPacket)
	    {
		res = CreateLostPacket(nStreamNumber, pQueueEntry);
	    }

	    if(SUCCEEDED(res))
	    {
#ifdef _LOG_PACKET_ORDERING
		fprintf(m_hLogFile, "Send ordered packet time: %d, stream#: %d, stream seq#: %d, queued time: %d, current time: %d, packet depth: %d, flushing=%d\n",
		    pQueueEntry->m_pPacket->GetTime(),
		    pQueueEntry->m_pPacket->GetStreamNumber(),
		    pQueueEntry->m_SequenceNumber, 
		    (pQueueEntry->m_tTime.tv_sec * 1000) + pQueueEntry->m_tTime.tv_usec,
		    (tNow.tv_sec * 1000) + tNow.tv_usec,
		    m_pqueueInOrderPacket[nStreamNumber].size(),
		    bSendAll);
		fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING
		HX_ASSERT(pQueueEntry->m_pPacket);
		res = m_pSinkObject->PacketReady(res, pQueueEntry->m_pPacket);
	    }
	    HX_DELETE(pQueueEntry);

	    pQueueEntry = m_pqueueInOrderPacket[nStreamNumber].peek_front();
	}
	else
	{
	    bSentAllAged = TRUE;
	}
    }

    //	*** Set the scheduler again if there's a packet in the queue for this stream
    //	Otherwise, if no more packets, will be scheduled when next packet is added
    //	for the stream
    if(pQueueEntry && !m_ppPacketAgedCallback[nStreamNumber]->m_hCB)
    {
	//  Schedule for the calculated time in the future
	m_ppPacketAgedCallback[nStreamNumber]->m_hCB = m_pScheduler->RelativeEnter(m_ppPacketAgedCallback[nStreamNumber], m_nPacketSendInterval);
#ifdef _LOG_PACKET_ORDERING
	fprintf(m_hLogFile, "Send Packets: Schedule Callback stream#: %d, handle: %d\n", nStreamNumber, m_ppPacketAgedCallback[nStreamNumber]->m_hCB);
	fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING
    }
	
    return res;
}

HX_RESULT
CPacketOrderShim::CreateLostPacket(UINT32 ulStream, CQueueEntry* pQueueEntry)
{
    HX_RESULT res = HXR_OK;

    if(!pQueueEntry)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
    }
    else 
    {
	res = m_pClassFactory->CreateInstance(CLSID_IHXPacket, (void **)&pQueueEntry->m_pPacket);
	if(SUCCEEDED(res))
	{
	    IHXServerPacketExt* pPacketExt = NULL;
	    res = pQueueEntry->m_pPacket->QueryInterface(IID_IHXServerPacketExt, (void **)&pPacketExt);
	    if(SUCCEEDED(res))
	    {
		pPacketExt->SetSeqNo(0);
		pPacketExt->SetStreamSeqNo(pQueueEntry->m_SequenceNumber);
		HX_RELEASE(pPacketExt);

		//UINT nTime = (pQueueEntry->m_tTime.tv_sec * 1000) + pQueueEntry->m_tTime.tv_usec;
		pQueueEntry->m_pPacket->Set(NULL, 0 /*nTime*/, ulStream, 0, 0);
		pQueueEntry->m_pPacket->SetAsLost();

#ifdef _LOG_PACKET_ORDERING
		fprintf(m_hLogFile, "Creating lost packet - stream#: %d, stream seq#: %d, packet depth: %d\n",
		    ulStream,
		    pQueueEntry->m_SequenceNumber,
		    m_pqueueInOrderPacket[ulStream].size());
		fflush(m_hLogFile);
#endif	//  _LOG_PACKET_ORDERING
	    }
	}
    }

    return res;
}

BEGIN_INTERFACE_LIST(CPacketOrderShim::CPacketAgedCallback)
    INTERFACE_LIST_ENTRY(IID_IHXCallback, IHXCallback)
END_INTERFACE_LIST

CPacketOrderShim::CPacketAgedCallback::CPacketAgedCallback(CPacketOrderShim* pPacketOrderShim)
: m_pPacketOrderShim(pPacketOrderShim)
, m_hCB(NULL)
{
    HX_ASSERT(m_pPacketOrderShim);
}

STDMETHODIMP
CPacketOrderShim::CPacketAgedCallback::Func()
{
    HX_RESULT res = HXR_OK;

    m_hCB = NULL;
    res = m_pPacketOrderShim->SendPackets();

    return res;
}





