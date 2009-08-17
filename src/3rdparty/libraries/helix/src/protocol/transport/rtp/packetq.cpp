/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: packetq.cpp,v 1.7 2005/08/02 18:00:48 albertofloyd Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxmap.h"	// CHXMapLongToObj
#include "netbyte.h"
#include "packetq.h"

#include "hxheap.h"
#ifdef _DEBUG
#include "hxtick.h"
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/******************************************************************
*   Defines
*/
#define MAX_GAP_SINCE_TERMINATION   3
#define MAX_EXPECTED_LOSS_GAP	    1000


/******************************************************************
*  PacketQueue
*
*/
PacketQueue::PacketQueue(const UINT32 ulWinSize,
			 const UINT16 unProbation,
			 const UINT32 ulWinTime,
			 const HXBOOL bUsesRTPPackets)
: m_pBuf(NULL)
, m_unInitProbation(unProbation)
, m_ulMinWindowSize(ulWinSize)
, m_ulMinWindowTime(ulWinTime)
, m_ulLate(0)
, m_ulLateSinceTermination(0)
, m_bIsFlexTimeWindow(FALSE)
, m_bUsesRTPPackets(bUsesRTPPackets)
, m_ulLastReturnedArrivalTime(0)
, m_unLastReturnedArrivalSeq(0)
, m_bLastReturnedArrivalSet(FALSE)
, m_unCurrent(0)
, m_bPacketReturned(FALSE)
, m_pClassFactory(NULL)
, m_bInitial(TRUE)
, m_unProbation(unProbation)
{
    HX_ASSERT(unProbation > 0);
#ifdef _PKTQ_DEBUG
    static INT32 lCount = 1;
    char cFileName[30]; /* Flawfinder: ignore */
    memset(cFileName, 0, 30);
    wsprintf(cFileName, "c:\\temp\\trans%d.txt", lCount++);
    m_pLogFile= fopen(cFileName, "wt"); /* Flawfinder: ignore */
#endif
}

PacketQueue::~PacketQueue()
{
    ReInitVars();

    HX_DELETE(m_pBuf);
    HX_RELEASE(m_pClassFactory);

#ifdef _PKTQ_DEBUG
    fclose(m_pLogFile);
#endif    
}

HX_RESULT	
PacketQueue::Init(IHXCommonClassFactory* pClassFactory)
{
    if (!pClassFactory)
    {
	HX_ASSERT(!"PacketQueue::Init(hey, classfactory is NULL)");
	return HXR_INVALID_PARAMETER;
    }
    
    m_pBuf = new CHXMapLongToObj();
    if (!m_pBuf)
    {
	return HXR_OUTOFMEMORY;
    }

    m_pClassFactory = pClassFactory;
    m_pClassFactory->AddRef();

    return HXR_OK;
}

void
PacketQueue::ReInitVars()
{
    CArrivedPacket* pDeadPacket;

    CHXMapLongToObj::Iterator i;
    for (i = m_pBuf->Begin(); i != m_pBuf->End(); ++i)
    {
	pDeadPacket = (CArrivedPacket*) (*i);
	delete pDeadPacket;
    }
    m_pBuf->RemoveAll();   
    
    m_bInitial = TRUE;
    m_bPacketReturned = FALSE;
    m_bLastReturnedArrivalSet = FALSE;

    // we've already received one pkt.
    m_unProbation = (UINT16)(m_unProbation - 1);
    
    // the same as ones in constractor...
    m_unCurrent		    = 0;
    m_ulLate		    = 0;
    m_ulLateSinceTermination = 0;
}

UINT16
PacketQueue::GetPercentDone(void)
{
    HX_ASSERT(m_pBuf);

    return (UINT16)((m_pBuf->GetCount() * 100) / 
	(m_ulMinWindowSize ? m_ulMinWindowSize : 1));
}


HX_RESULT
PacketQueue::AddPacket(UINT16 unSeq, 
		       IHXPacket* pPacket, 
		       ULONG32 ulArrivalTime)
{
    // RTP sort of sanity check
    // Don't add until MIN_SEQUENTIAL sequential pkts are received
    if (m_unProbation)
    {
	void* pVoid = NULL;
	if (m_pBuf->Lookup(unSeq - 1, (void*&) pVoid))
	{
	    HX_ASSERT(pVoid);
	    // in seq.
	    m_unProbation--;
	}
	else
	{
	    ReInitVars();
	}
    }

    if (m_bPacketReturned &&
	IsSeqNumGT(m_unCurrent, unSeq))
    {
	// this packet is too late, don't add it
	void* pDummyVoid;
	if (!m_pBuf->Lookup(unSeq, pDummyVoid))
	{
	    m_ulLate++;

	    #ifdef _PKTQ_DEBUG	
	    if (m_pLogFile)
	    {
	    	fprintf(m_pLogFile, "LATE #%u <= #%u at %u\n", unSeq, m_unCurrent, HX_GET_TICKCOUNT());
	    	fflush(m_pLogFile);
	    }
    	    #endif
	}
#ifdef _PKTQ_DEBUG	
	else
	{
	    if (m_pLogFile)
	    {
		fprintf(m_pLogFile, "DUP #%u > #%u (%u) at %u\n", unSeq, m_unCurrent,
		    pPacket->GetTime(), HX_GET_TICKCOUNT());
		fflush(m_pLogFile);						    
	    }						    
	}
#endif	
	
	return HXR_OK;
    }

    void* pDummyVoid;
    if (!m_pBuf->Lookup(unSeq, pDummyVoid))
    {
	CArrivedPacket* pArrivedPacket = new CArrivedPacket(pPacket,
							    ulArrivalTime);
#ifdef _PKTQ_DEBUG	
	if (m_pLogFile)
	{
	    fprintf(m_pLogFile, "ADD #%u > #%u (%u) at %u: %u\n", unSeq, m_unCurrent, 
		pPacket->GetTime(), HX_GET_TICKCOUNT(), m_pBuf->GetCount()+1);
	    fflush(m_pLogFile);					    
	}					   
#endif	
	
	if (pArrivedPacket)
	{
	    (*m_pBuf)[unSeq] = pArrivedPacket;
	
	    if (m_bInitial)
	    {
		m_bInitial = FALSE;
		m_unCurrent = unSeq;

		m_ulLastReturnedArrivalTime = ulArrivalTime;
		m_unLastReturnedArrivalSeq = unSeq;
		m_bLastReturnedArrivalSet = TRUE;
	    }
	    else if (IsSeqNumGT(m_unCurrent, unSeq))
	    {
		m_unCurrent = unSeq;
	    }
	
	    return HXR_OK;
	}
    }
 #ifdef _PKTQ_DEBUG	
    else
    {
	if (m_pLogFile)
	{
	    fprintf(m_pLogFile, "DUP #%u > #%u (%u) at %u\n", unSeq, m_unCurrent,
		pPacket->GetTime(), HX_GET_TICKCOUNT());
	    fflush(m_pLogFile);						    
	}						    
    }
 #endif	
    
    return HXR_FAIL;
}

HX_RESULT
PacketQueue::GetPacket(REF(IHXPacket*) pPacket, ULONG32 ulTimeNow)
{
#ifdef _PKTQ_DEBUG
    if (m_pLogFile)    
    {
	fprintf(m_pLogFile, "GetPacket(): PktCount %u, WindowSize: %u, CurrentSeq: %u\n", 
	    m_pBuf->GetCount(), m_ulMinWindowSize, m_unCurrent);
	fflush(m_pLogFile);
    }
#endif

    if (!IsBufferingForSure())
    {
	CArrivedPacket* pArrivedPacket;

	if (m_pBuf->Lookup(m_unCurrent, (void*&) pArrivedPacket))
	{
	    if (m_bIsFlexTimeWindow ||
		PacketSufficientlyAged(pArrivedPacket, ulTimeNow))
	    {
		m_pBuf->RemoveKey(m_unCurrent);
		m_ulLateSinceTermination = 0;

		m_ulLastReturnedArrivalTime = pArrivedPacket->m_ulArrivalTS;
		m_unLastReturnedArrivalSeq = m_unCurrent;
		m_bLastReturnedArrivalSet = TRUE;

		pPacket = pArrivedPacket->m_pPacket;
		pPacket->AddRef();

		delete pArrivedPacket;

#ifdef _PKTQ_DEBUG
		if (m_pLogFile)
		{
		    fprintf(m_pLogFile, "\tFOUND TS: %u\n", pPacket->GetTime());
		    fflush(m_pLogFile);							
		}
#endif
	    }
	    else
	    {
		return HXR_NO_DATA;
	    }
	}
	else
	{
	    // When we recieve RTCP_BYE, window size is set to 0.
	    if ((0 == m_ulMinWindowSize) &&
		((m_pBuf->GetCount() == 0) ||
		 (m_ulLateSinceTermination > MAX_GAP_SINCE_TERMINATION)))
	    {
		// this stream is done
#ifdef _PKTQ_DEBUG
		if (m_pLogFile)
		{
		    fprintf(m_pLogFile, "\tSTREAM_DONE\n");
		    fflush(m_pLogFile);							
		}
#endif
		ReInitVars();

		return HXR_STREAM_DONE;
	    }

	    /*
	     * The packet hasn't been received yet so create a new
	     * one and mark it as lost
	     */
#ifdef _PKTQ_DEBUG
	    if (m_pLogFile)
	    {
		fprintf(m_pLogFile, "\tLOST\n");
		fflush(m_pLogFile);
	    }
#endif
	    if (PacketSufficientlyAged(NULL, ulTimeNow))
	    {
		if (0 == m_ulMinWindowSize)
		{
		    m_ulLateSinceTermination++;
		}

		if (m_bUsesRTPPackets)
		{
		    m_pClassFactory->CreateInstance(CLSID_IHXRTPPacket, (void**) &pPacket);
		}
		else
		{
		    m_pClassFactory->CreateInstance(CLSID_IHXPacket, (void**) &pPacket);
		}
	    
		if (pPacket)
		{
		    pPacket->SetAsLost();
		}
		else
		{
		    return HXR_OUTOFMEMORY;
		}
	    }
	    else
	    {
		return HXR_NO_DATA;
	    }
	}
	
	m_bPacketReturned = TRUE;
	m_unCurrent++;
	
        return HXR_OK;
    }

    return HXR_NO_DATA;
}


HX_RESULT
PacketQueue::GetNextTS(REF(UINT32)ulTS, ULONG32 ulTimeNow)
{
#ifdef _PKTQ_DEBUG
    if (m_pLogFile)    
    {
	fprintf(m_pLogFile, "GetNextTS(): PktCount %u, WindowSize: %u, CurrentSeq: %u\n", 
	    m_pBuf->GetCount(), m_ulMinWindowSize, m_unCurrent);
	fflush(m_pLogFile);
    }
#endif

    if (!IsBufferingForSure())
    {
	CArrivedPacket* pArrivedPacket;
	if (m_pBuf->Lookup(m_unCurrent, (void*&) pArrivedPacket))
	{
	    if (m_bIsFlexTimeWindow ||
		PacketSufficientlyAged(pArrivedPacket, ulTimeNow))
	    {
		ulTS = pArrivedPacket->m_pPacket->GetTime();

#ifdef _PKTQ_DEBUG
		fprintf(m_pLogFile, "\tFOUND\n");
		fflush(m_pLogFile);
#endif
		return HXR_OK;
	    }
	}
	else
	{
	    // When we recieve RTCP_BYE, window size is set to 0.
	     if ((0 == m_ulMinWindowSize) &&
		 ((m_pBuf->GetCount() == 0) ||
		  (m_ulLateSinceTermination > MAX_GAP_SINCE_TERMINATION)))
	    {
		// this stream is about to end
		HX_ASSERT(!m_pBuf->GetCount());

#ifdef _PKTQ_DEBUG
		if (m_pLogFile)
		{
		    fprintf(m_pLogFile, "\tSTREAM_DONE\n");
		    fflush(m_pLogFile);							
		}
#endif

		ReInitVars();

		return HXR_STREAM_DONE;
	    }

	    if (PacketSufficientlyAged(NULL, ulTimeNow))
	    {
		HX_ASSERT(m_bLastReturnedArrivalSet);

		ulTS = m_ulLastReturnedArrivalTime;
#ifdef _PKTQ_DEBUG
		if (m_pLogFile)
		{
		    fprintf(m_pLogFile, "\tLOST\n");
		    fflush(m_pLogFile);
		}
#endif	    

		return HXR_FAIL;
	    }
	}		
    }    

    #ifdef _PKTQ_DEBUG
    if (m_pLogFile)
    {
	fprintf(m_pLogFile, "\tBUFFERING\n");
	fflush(m_pLogFile);
    }
    #endif    
    
    return HXR_BUFFERING;    
}


HXBOOL PacketQueue::PacketSufficientlyAged(CArrivedPacket* pArrivedPacket,
					 ULONG32 ulTimeNow)
{
    HXBOOL bIsSufficientlyAged = TRUE;

    if (m_ulMinWindowTime != 0)
    {
	if (pArrivedPacket)
	{
	    if (((ULONG32) (ulTimeNow - pArrivedPacket->m_ulArrivalTS)) <
		m_ulMinWindowTime)
	    {
		bIsSufficientlyAged = FALSE;
	    }
	}
	else
	{
	    // We are answering for a lost packet
	    if (m_bLastReturnedArrivalSet &&
		(m_pBuf->GetCount() > 0))
	    {
		if (((ULONG32) (ulTimeNow - m_ulLastReturnedArrivalTime)) <
		    m_ulMinWindowTime)
		{
		    bIsSufficientlyAged = FALSE;
		}
	    }
	    else
	    {
		// Lost packet is never sufficienly aged unless
		// there is a non-lost packet in the buffer
		bIsSufficientlyAged = FALSE;
	    }
	}
    }

    return bIsSufficientlyAged;
}


ULONG32 PacketQueue::GetAge(ULONG32 ulTimeNow)
{
    ULONG32 ulAge = 0;
    CArrivedPacket* pArrivedPacket = NULL;

    if (m_pBuf->Lookup(m_unCurrent, (void*&) pArrivedPacket))
    {
	ulAge = ((ULONG32) (ulTimeNow - pArrivedPacket->m_ulArrivalTS));
    }
    else if (m_bLastReturnedArrivalSet &&
	     (m_pBuf->GetCount() > 0))
    {
	ulAge = ((ULONG32) (ulTimeNow - m_ulLastReturnedArrivalTime));
    }

    return ulAge;
}

