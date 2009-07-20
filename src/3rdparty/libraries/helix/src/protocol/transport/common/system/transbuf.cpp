/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: transbuf.cpp,v 1.36 2007/04/29 03:33:23 e3423c Exp $
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

#include "debug.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxdeque.h"
#include "pckunpck.h"
#include "hxbitset.h"
#include "hxmap.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "basepkt.h"
#include "mimehead.h"
#include "rtspmsg.h"
#include "servrsnd.h"
#include "transbuf.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "hxtick.h"
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

//These defines control when we stop waiting for an out of order
//packet and just send a NAK for it. The two conditions are a timeout
//and the number of packets, with higher sequence numbers, that come
//after it.  For the timeout we have choosen 500ms for now. This
//number should be based off of the RTT but we currently do not have
//that information.
#define NAK_TIMEOUT 500
#define REORDER_TOLERANCE 3

//This is how often to check to see if we have exceeded our NAK_TIMEOUT
//for the packets in our pending queue.
#define NAK_CHECK_INTERVAL 100

#ifdef HELIX_FEATURE_SERVER
// re-define HX_LOCK/HX_UNLOCK in common/system/pub/hxthread.h
// since the server doesn't need the mutex
#ifdef HX_LOCK
#undef HX_LOCK
#endif
#define HX_LOCK(p)   /*nothing*/

#ifdef HX_UNLOCK
#undef HX_UNLOCK
#endif
#define HX_UNLOCK(p) /*nothing*/
#endif

#define CLIPEND_WINDOW 3000

// Pending Packet Methods
PendingPacket::PendingPacket(UINT32 ulSeqNo, UINT32 arrivalTime)
    : m_ulSequenceNumber(ulSeqNo),
      m_ulNumPktsBehind(0),
      m_ulArrivalTime(arrivalTime)
{
}

PendingPacket::~PendingPacket()
{};

/*
 * Keep the deque under 16k
 */

const UINT16 MAX_BITSET_SIZE = 384;
const UINT16 MIN_NETWORK_JITTER_MSECS = 2000;
const UINT16 MAX_QUEUED_PACKETS = 500;

RTSPTransportBuffer::RTSPTransportBuffer(
    RTSPTransport* owner,
    UINT16 streamNumber,
    UINT32 bufferDuration,
    UINT32 maxBufferDuration,
    UINT32 growthRate,
    UINT32 wrapSequenceNumber
) : m_CallbackHandle(0),
    m_pCallBack(NULL),
    m_pScheduler(NULL),
    m_pFIFOCache(NULL),
    m_pOwner(owner),
    m_uStreamNumber(streamNumber),
    m_status(TRANSBUF_INITIALIZING),
    m_bPrefetch(FALSE),
    m_bFastStart(FALSE),
    m_bCacheIsEmpty(TRUE),
    m_bIsInitialized(FALSE),
    m_bWaitingForSeekFlush(FALSE),
    m_bWaitingForLiveSeekFlush(FALSE),
    m_bFlushHolding(FALSE),
    m_bIsEnded(FALSE),
    m_bStreamBegin(FALSE),
    m_bStreamDone(FALSE),
    m_bSourceStopped(FALSE),
    m_bACKDone(FALSE),
    m_bStreamDoneSent(FALSE),
    m_bPaused(FALSE),
    m_bPausedHack(FALSE),
    m_bufferDuration(bufferDuration),
    m_extraDelayDuringBuffering(MIN_NETWORK_JITTER_MSECS),
    m_maxBufferDuration(maxBufferDuration),
    m_growthRate(growthRate),
    m_wrapSequenceNumber(wrapSequenceNumber),
    m_ulFrontTimeStampCached(0),
    m_ulRearTimeStampCached(0),
    m_uReliableSeqNo(0),
    m_uEndReliableSeqNo(0),
    m_uFirstSequenceNumber(0),
    m_uLastSequenceNumber(0),
    m_uEndSequenceNumber(0),
    m_uSeekSequenceNumber(0),
    m_uSeekCount(0),
    m_uNormal(0),
    m_ulDuplicate(0),
    m_ulOutOfOrder(0),
    m_uLost(0),
    m_uLate(0),
    m_ulIndex30(0),
    m_uResendRequested(0),
    m_uResendReceived(0),
    m_uByteCount(0),
    m_uLastByteCount(0),
    m_uAvgBandwidth(0),
    m_uCurBandwidth(0),
    m_ulLastLost30(0),
    m_ulLastTotal30(0),
    m_uLastTimestamp(0),
    m_uStartTimestamp(0),
    m_uEndTimestamp(0),
    m_ulEndDelayTolerance(0),
    m_bExpectedTSRangeSet(FALSE),
    m_bPacketsStarted(FALSE),
    m_ulCurrentQueueByteCount(0),
    m_ulCurrentCacheByteCount(0),
    m_bAtLeastOnePacketReceived(FALSE),
    m_bAtLeastOneResetHandled(FALSE),
    m_ulFirstTimestampReceived(0),
    m_ulLastTimestampReceived(0),
    m_ulBufferingStartTime(0),
    m_ulLastGrowTime(HX_GET_TICKCOUNT()),
    m_ulEndReasonCode(0),
    m_bMulticast(FALSE),
    m_bMulticastReset(TRUE),
    m_bMulticastReliableSeqNoSet(FALSE),
    m_bIsLive(FALSE),
    m_bSparseStream(FALSE),
    m_bFirstPacketAdd(FALSE),
    m_bFirstPacketGet(FALSE),
    m_ulByteLimit(0),
    m_pPendingLock(NULL)
{
    InitTimer();

    int j = 0;
    for (j = 0; j < 30; j++)
    {
        m_ulTotal30[j] = 0;
        m_ulLost30[j] = 0;
    }

    m_pPacketDeque = new HX_deque(INITIAL_DEQUE_SIZE);

    m_pCallBack = new RTSPTransportBufferCallback(this);
    m_pCallBack->AddRef();

    IUnknown* pContext = NULL;
    m_pOwner->GetContext(pContext);

#if !defined(HELIX_FEATURE_SERVER)
    if (pContext)
    {
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pPendingLock, pContext);
	HX_RELEASE(pContext);
    }
#endif
}

RTSPTransportBuffer::~RTSPTransportBuffer()
{    
    CHXSimpleList::Iterator i;
    ClientPacket* pPacket;

    //Clean up our pending packet que.
    HX_LOCK(m_pPendingLock);
    while( !m_PendingPackets.IsEmpty() )
    {
        PendingPacket* pPend = (PendingPacket*)m_PendingPackets.RemoveHead();
        HX_DELETE(pPend);
    }
    //Get rid of any scheduler events...
    if (m_pScheduler && m_CallbackHandle)
    {
        m_pScheduler->Remove(m_CallbackHandle);
    }
    m_CallbackHandle = 0;
    if( m_pCallBack )
        m_pCallBack->Clear();
    
    HX_RELEASE( m_pCallBack );
    HX_UNLOCK(m_pPendingLock);

    for (i = m_pHoldList.Begin(); i != m_pHoldList.End(); ++i)
    {
        pPacket = (ClientPacket*)(*i);
        HX_RELEASE(pPacket);
    }

    m_pHoldList.RemoveAll();

    while(!m_pPacketDeque->empty())
    {
        pPacket = (ClientPacket*)m_pPacketDeque->pop_front();
        HX_RELEASE(pPacket);
    }

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pPendingLock);
    HX_DELETE(m_pPacketDeque);
#if defined(HELIX_FEATURE_FIFOCACHE)    
    HX_RELEASE(m_pFIFOCache);
#endif
}

void
RTSPTransportBuffer::Reset()
{
    m_status = TRANSBUF_INITIALIZING;

    if (m_bAtLeastOneResetHandled)
    {
        m_uSeekCount++;
        m_bIsEnded = FALSE;
        m_bStreamDone = FALSE;
        m_bStreamDoneSent = FALSE;
        m_bSourceStopped = FALSE;
    }
    else
    {
        m_bAtLeastOneResetHandled   = TRUE;
        m_ulBufferingStartTime      = HX_GET_TICKCOUNT();
    }

    m_bAtLeastOnePacketReceived = FALSE;
    m_uLastTimestamp         = 0;
    m_bExpectedTSRangeSet = FALSE;
    m_uStartTimestamp = 0;
    m_uEndTimestamp = 0;
    m_ulEndDelayTolerance = 0;
}

void
RTSPTransportBuffer::Grow()
{
    UINT32 ulCurrentTime = HX_GET_TICKCOUNT();

    /* Check to not grow fast in case we get multiple late
     * packets at around the same time.
    */
    if (CALCULATE_ELAPSED_TICKS(m_ulLastGrowTime, ulCurrentTime) >= 
        m_growthRate)
    {
        m_ulLastGrowTime = HX_GET_TICKCOUNT();

        if (m_bufferDuration + m_growthRate <= m_maxBufferDuration)
        {
            m_bufferDuration += m_growthRate;
        }
    }
}

HX_RESULT
RTSPTransportBuffer::Init(UINT16 uSeqNo, HXBOOL bOnPauseResume)
{
    /*
     * The server side of an encoding session will initialize the scheduler
     * here
     */

    if (!m_pScheduler)
    {
        InitTimer();
    }

    HX_ASSERT(m_pScheduler != NULL);

    if (!m_bIsInitialized)
    {
        m_bIsInitialized = TRUE;

        m_uACKSequenceNumber   =
        m_uFirstSequenceNumber =
        m_uLastSequenceNumber  = uSeqNo;

        if (m_uSeekCount > 0)
        {
            return HXR_OK;
        }
    }
    else if (m_uSeekCount)
    {
	// In is done is reponse to resumption after seek as well
	// as acknowledgement of resumption from pause by the server.
	// We do not want to confuse seek actions for acknowledgement
	// of resumption from pause.
	if (!bOnPauseResume)
	{
	    m_uSeekCount--;
	}
        if (m_uSeekCount > 0)
        {
            return HXR_OK;
        }

        m_uSeekSequenceNumber = uSeqNo;
        m_bWaitingForSeekFlush = TRUE;
    }

    m_status = TRANSBUF_READY;

    /*
     * Now add any packets that arrived before initialization
     */

    m_bStreamBegin = TRUE;

    CHXSimpleList::Iterator i;

    for (i = m_pHoldList.Begin(); i != m_pHoldList.End(); ++i)
    {
        ClientPacket* pPacket = (ClientPacket*)(*i);
        Add(pPacket);
    }

    m_pHoldList.RemoveAll();

    m_bStreamBegin = FALSE;

    return HXR_OK;
}

HX_RESULT
RTSPTransportBuffer::Add(ClientPacket* pPacket)
{
    if (pPacket && !m_bFirstPacketAdd)
    {
        m_bFirstPacketAdd = TRUE;
        HXLOGL1(HXLOG_TRAN, "RTSPTransportBuffer[%p]: First Packet Received %i", this, m_uStreamNumber);
    }

    if (!m_pPacketDeque)
    {
	HXLOGL4(HXLOG_TRAN, "RTSPTransportBuffer::Add() NoQueue: Discard");
        HX_RELEASE(pPacket);
        return HXR_FAIL;
    }
    else if (m_pPacketDeque->size() >= MAX_DEQUE_SIZE)
    {
	HXLOGL4(HXLOG_TRAN, "RTSPTransportBuffer::Add() QueueOverflow: Discard");
        m_pOwner->HandleBufferError();
        
        HX_RELEASE(pPacket);
        return HXR_FAIL;
    }
    else if (m_bStreamDone)
    {
	HXLOGL4(HXLOG_TRAN, "RTSPTransportBuffer::Add() StreamDone: Discard");
        /*
         * If we have already returned the last packet, then don't bother
         * trying to add anymore
         */
        HX_RELEASE(pPacket);
        return HXR_OK;
    }
    else if (!m_bIsInitialized || m_uSeekCount)
    {
	HXLOGL4(HXLOG_TRAN, "RTSPTransportBuffer::Add() NotReady: Queue: Initialized=%c SeekCount=%hu",
	    m_bIsInitialized ? 'T' : 'F',
	    m_uSeekCount);

        /*
         * Until the first sequence number is set, just hold the packets
         * as they arrive
         */
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
        if (m_bMulticast && m_bMulticastReset)
        {
            //We are going to destroy all our place holders so clean up the
            //pending queue as well.
            HX_LOCK(m_pPendingLock);
            while( !m_PendingPackets.IsEmpty() )
            {
                PendingPacket* pPend = (PendingPacket*)m_PendingPackets.RemoveHead();
                HX_DELETE(pPend);
            }
            //Get rid of any scheduler events...
            if (m_pScheduler && m_CallbackHandle)
            {
                m_pScheduler->Remove(m_CallbackHandle);
            }
            m_CallbackHandle = 0;
            if( m_pCallBack )
                m_pCallBack->Clear();
            HX_RELEASE( m_pCallBack );
            HX_UNLOCK(m_pPendingLock);

            /* Destruct and recreate the packet queue */
            /* XXXSMP ...but it works */

            ClientPacket *pTmpPacket = NULL;
            while(!m_pPacketDeque->empty())
            {
                pTmpPacket = (ClientPacket*)m_pPacketDeque->pop_front();
                HX_RELEASE(pTmpPacket);
            }
            HX_DELETE(m_pPacketDeque);
            m_pPacketDeque = new HX_deque(INITIAL_DEQUE_SIZE);

            m_bMulticastReset = FALSE;
            m_bIsInitialized = FALSE;
            m_bWaitingForSeekFlush = FALSE;
            m_bWaitingForLiveSeekFlush = FALSE;
            m_bFlushHolding = FALSE;
            m_bIsEnded = FALSE;
            m_bCacheIsEmpty = TRUE;
            m_bStreamBegin = FALSE;
            m_bStreamDone = FALSE;
            m_bStreamDoneSent = FALSE;
            m_bSourceStopped = FALSE;
            m_bExpectedTSRangeSet = FALSE;
            m_uStartTimestamp = 0;
            m_uEndTimestamp = 0;
            m_ulEndDelayTolerance = 0;
            m_bACKDone = FALSE;
            m_bPaused = FALSE;
            m_bPausedHack = FALSE;
            m_uReliableSeqNo = 0;
            m_uEndReliableSeqNo = 0;
            m_uFirstSequenceNumber = 0;
            m_uLastSequenceNumber = 0;
            m_uEndSequenceNumber = 0;
            m_uSeekSequenceNumber = 0;
            m_uSeekCount = 0;
            m_bAtLeastOnePacketReceived = FALSE;
            m_bAtLeastOneResetHandled = FALSE;
            Init(pPacket->GetSequenceNumber());
            
            Add(pPacket);
            return HXR_OK;
        }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
        m_pHoldList.AddTail(pPacket);

        return HXR_OK;
    }

    // initialize the reliableSeqNo on the first 
    // reliable multicast packet
    // SeqNo of reliable packets starts at 1
    if (m_bMulticast                    && 
        !m_bMulticastReliableSeqNoSet   &&
        pPacket                         &&
        pPacket->IsReliable())
    {
        m_uReliableSeqNo = (UINT16)(pPacket->GetReliableSeqNo() - 1);
        m_bMulticastReliableSeqNoSet = TRUE;
    }

    /*
     * Save away the sequence number because Insert() may delete the
     * ClientPacket
     */

    UINT16 uSequenceNumber = pPacket->GetSequenceNumber();


    HX_RESULT result = HXR_OK;

    /* We insert it AFTER flushing the packets in live flush case */
    if (!m_bWaitingForLiveSeekFlush)
    {
        result = Insert(pPacket);
    }
    else
    {
        //live streaming seek/pause case, just flush buffer
        m_bFlushHolding = TRUE; //set this flag to force flush in live streaming mode 
    }
    if (HXR_OK != result)
    {
        return result;
    }

    if (m_bWaitingForSeekFlush)
    {
        UINT32 uPacketSeekIndex = GetSeekIndex(uSequenceNumber);

        /*
         * If we have already tried to flush or this packet belongs
         * after the seek, then Flush()
         */

        if (m_bFlushHolding || uPacketSeekIndex < MAX_DEQUE_SIZE)
        {
            /*
             * This routine will clear out the old packets and will reset
             * the packet queue with the information provided by the seek
             */

            if (HXR_OK == Flush())
            {
                if (m_bFlushHolding)
                {
                    m_bFlushHolding = FALSE;
                }

                m_bWaitingForSeekFlush = FALSE;

                // Queue should not be empty at this point, since seek index
                // was contained within the queue prior to flush
                HX_ASSERT(!IsQueueEmpty());
            }
            else
            {
                m_bFlushHolding = TRUE;
            }
        }
    }

    if (m_bWaitingForLiveSeekFlush)
    {
        HX_ASSERT(!m_bWaitingForSeekFlush);

        m_bWaitingForLiveSeekFlush = FALSE;

        m_uACKSequenceNumber   =
        m_uFirstSequenceNumber =
        m_uLastSequenceNumber  = uSequenceNumber;

        result = Insert(pPacket);
    }   

    return HXR_OK;
}

HX_RESULT
RTSPTransportBuffer::Insert(ClientPacket* pPacket)
{
    ClientPacket* tempPacket = NULL;

    if (IsQueueEmpty())
    {
        SanitizePacketQueue();
    }

    UINT32 uTailIndex = GetPacketIndex(m_uLastSequenceNumber);

    if (uTailIndex >= MAX_DEQUE_SIZE)
    {
        /*
         * Somebody had better be getting packets from this buffer
         */
        HX_RELEASE(pPacket);
        return HXR_FAIL;
    }

    UINT32 uTimestamp = pPacket->GetTime();

    m_uByteCount                += pPacket->GetByteCount();
    m_ulCurrentQueueByteCount   += pPacket->GetByteCount();;

    UINT16 uSequenceNumber = pPacket->GetSequenceNumber();
    UINT32 uPacketIndex = GetPacketIndex(uSequenceNumber);

    // Send NAK iff at least REORDER_TOLERANCE packets with higher
    // seqno's than the lost packet arrive.  increases robustness if
    // reordering occurs. There is a trade off between loss detection
    // accuracy and the time of the retransmission window being made
    // here. Loss detection becomes inaccurate when we count reordered
    // packets as lost. But we can't determine if packets are
    // reordered without waiting for subsequent pkts to arrive.
    // Something to consider is determining whether to NAK early or
    // not based on the avg. Time for the current index to be
    // retrieved by the higher level transpor object (avg time in
    // queue) and the estimated RTT.
    HX_LOCK(m_pPendingLock);
    LISTPOSITION pos    = m_PendingPackets.GetHeadPosition();
    int          nCount = m_PendingPackets.GetCount();
    for( int nTmp=0 ; pos && nTmp<nCount ; nTmp++  )
    {
        HXBOOL bDeleted=FALSE;
        PendingPacket* pPend = (PendingPacket*)m_PendingPackets.GetAt(pos);
        if(uSequenceNumber > pPend->m_ulSequenceNumber)
        {
            pPend->m_ulNumPktsBehind++;
            if( pPend->m_ulNumPktsBehind>REORDER_TOLERANCE )
            {
                UINT32 tempIndex = GetPacketIndex((UINT16)pPend->m_ulSequenceNumber);
                
                //Send a NAK and increment resend requested count.
                m_pOwner->sendNAKPacket( m_uStreamNumber,
                                         (UINT16)pPend->m_ulSequenceNumber,
                                         (UINT16)pPend->m_ulSequenceNumber);
                if( tempIndex<m_pPacketDeque->size())
                    ((ClientPacket*)(*m_pPacketDeque)[tempIndex])->SetResendRequested();
                m_uResendRequested++;
                //Remove this packet from our pending list
                pos = m_PendingPackets.RemoveAt(pos);
                HX_DELETE(pPend);
                bDeleted=TRUE;
            }
        }
        else if( uSequenceNumber==pPend->m_ulSequenceNumber )
        {
            //This packet arrived, remove it from the pending list.
            pos = m_PendingPackets.RemoveAt(pos);
            HX_DELETE(pPend);
            bDeleted=TRUE;
            m_ulOutOfOrder++;
        }
        
        //If we deleted,  RemoveAt() updated the pos.
        if(!bDeleted)
        {
            m_PendingPackets.GetNext(pos);
        }
    }
    HX_UNLOCK(m_pPendingLock);

    if (uPacketIndex == uTailIndex + 1)
    {
        /*
         * If the only packet in the queue is the sanitize packet, then we
         * have lost a packet
         */

        tempPacket = (ClientPacket*)(*m_pPacketDeque)[uTailIndex];

        if (tempPacket->IsSanitizePacket())
        {
//{FILE* f1 = ::fopen("c:\\loss.txt", "a+"); ::fprintf(f1, "this: %p Lost the sanitized packet uPacketIndex == uTailIndex + 1\n", this);::fclose(f1);}  
            goto HandleLostPacket;
        }

        /*
         * Packet has arrived in order so put it in the queue
         */
        if (OverByteLimit())
        {
            ConvertToDroppedPkt(pPacket);
        }

        m_pPacketDeque->push_back(pPacket);
        m_uLastSequenceNumber++;

        if (m_uLastSequenceNumber == m_wrapSequenceNumber)
        {
            m_uLastSequenceNumber = 0;
        }

        m_uNormal++;
    }
    else if (uPacketIndex <= uTailIndex)
    {
        /*
         * Check to see that the packet queue is in a sane state
         */

        if (uPacketIndex >= m_pPacketDeque->size())
        {
            ASSERT(0);
            
            HX_RELEASE(pPacket);
            return HXR_UNEXPECTED;
        }
            
        /*
         * This is a valid out of order packet that belongs somewhere
         * in the queue
         */

        tempPacket = (ClientPacket*)(*m_pPacketDeque)[uPacketIndex];

        if (tempPacket->IsLostPacket())
        {
            if (tempPacket->IsResendRequested())
            {
                m_uResendReceived++;
            }
            else
            {
                m_uNormal++;
            }

            /*
             * This was a place holder packet, so replace it with the
             * valid packet
             */

            if (OverByteLimit())
            {
                ConvertToDroppedPkt(pPacket);
            }

            (*m_pPacketDeque)[uPacketIndex] = pPacket;
            HX_RELEASE(tempPacket);
        }
        else
        {
            // could be actually duplicate (rare) OR 
            // because of resends for out-of-order packets (more likely)
            m_ulDuplicate++; 
            /*
             * We've received a duplicate packet so get rid of it
             */
            HX_RELEASE(pPacket);
        }
    }
    else if (uPacketIndex > MAX_DEQUE_SIZE)
    {
        //XXXGH...don't count late packets because they've already been
        //        been accounted for as lost packets
        // m_uLate++;

        /*
         * If the stream is not being reset and this packet is either
         * too early or too late to be placed in the queue, then Grow().
         * If the stream just starting or ending then don't bother growing
         * because packet funkiness may occur
         */

        if (!m_bStreamBegin && !m_bIsEnded)
        {
            Grow();
        }

        HX_RELEASE(pPacket);
    }
    else
    {
        /*
         * Check to see that the packet queue is in a sane state
         */

        if (uTailIndex >= m_pPacketDeque->size())
        {
            ASSERT(0);
            
            HX_RELEASE(pPacket);
            return HXR_UNEXPECTED;
        }
            
        /*
         * This is a valid out of order packet that belongs somewhere
         * after the last packet in the queue, so fill in any missing
         * packets
         */

        tempPacket = (ClientPacket*)(*m_pPacketDeque)[uTailIndex];

HandleLostPacket:

        /*
         * Use the reliable count from the incoming packet to keep track
         * of lost reliable packets and use the the timestamp from the
         * incoming packet to give the transport a little longer to recover
         * missing packets
         */

        UINT16 uReliableSeqNo = pPacket->GetReliableSeqNo();

        /*
         * If the last packet in the queue is the sanitize packet, then
         * it must get replaced with a proper lost packet
         */

        UINT16 uSeqNo;
        UINT32 uFillIndex;

        if (tempPacket->IsSanitizePacket())
        {
            uSeqNo = tempPacket->GetSequenceNumber();
            uFillIndex = uTailIndex;
            tempPacket = (ClientPacket*)m_pPacketDeque->pop_front();
            HX_RELEASE(tempPacket);
        }
        else
        {
            uSeqNo = (UINT16)(tempPacket->GetSequenceNumber() + 1);
            uFillIndex = uTailIndex + 1;
        }

        if (uSeqNo == m_wrapSequenceNumber)
        {
            uSeqNo = 0;
        }

        /*
         * Fill in lost packets from the end of the queue to this packet
         */

        UINT32 i;

        //For each missing packet sequence number, make a dummy packet
        //and stick it in the queue as a place holder. If we are
        //looking at a 'large' gap in sequence numbers here then there
        //is a good chance we are looking at real loss and not
        //out-of-order packets.  In that case, lets forget about the
        //out-of-order work and send an immediate NAK here instead of
        //waiting for the out of order limit and sending indivudual
        //NAKs for each packet. This will lessen the server and
        //network load as well (no NAK spam).
        HXBOOL   bUseOOPQueue       = TRUE;
        INT32  nNumToFill         = (INT32)(uPacketIndex - uFillIndex);
        UINT16 uStartNAKSeqNumber = uSeqNo;

        //If we are missing more then REORDER_TOLERANCE packets then
        //this has to be real loss and not out of order packets. In
        //this case we just want to add these packets to the Packets
        //queue, mark them as resend-requested and then send a single
        //NAK for the bunch.
        if( nNumToFill >= REORDER_TOLERANCE )
            bUseOOPQueue = FALSE; 
        
        for( i=uFillIndex; i<uPacketIndex; i++ )
        {
            //Add a new filler packet..
            tempPacket = new ClientPacket(uSeqNo++,
                                         uReliableSeqNo,
                                         uTimestamp,
                                         0,
                                         0,
                                         0,
                                         GetTime(),
                                         FALSE);
           
            tempPacket->AddRef();
            m_pPacketDeque->push_back(tempPacket);

            //Don't add to OOP queue if we think this is real loss.
            if( bUseOOPQueue )
            {
                //Track this place holder packet by inserting it into our
                //pending packet list.
                HX_LOCK(m_pPendingLock);
                PendingPacket* pPend = new PendingPacket((UINT32)(uSeqNo - 1), HX_GET_TICKCOUNT());
                if( pPend )
                    m_PendingPackets.AddTail( pPend );

                //If this is the first packet found out of order start our callback.
                if(m_pScheduler)
                {
                    if( !m_pCallBack)
                    {
                        m_pCallBack = new RTSPTransportBufferCallback(this);
                        m_pCallBack->AddRef();
                    }
                    m_CallbackHandle = m_pScheduler->RelativeEnter(m_pCallBack, NAK_CHECK_INTERVAL);
                }
                HX_UNLOCK(m_pPendingLock);
            }
            else
            {
                //Mark it, we will NAK for all packets outside of loop.
                tempPacket->SetResendRequested();
                m_uResendRequested++;
            }
            
        }
        if( !bUseOOPQueue )
        {
            //send out the NAK right now...
            m_pOwner->sendNAKPacket( m_uStreamNumber,
                                     uStartNAKSeqNumber,
                                     (UINT16)(uSeqNo - 1));
        }
        

        if (OverByteLimit())
        {
            ConvertToDroppedPkt(pPacket);
        }

        m_pPacketDeque->push_back(pPacket);

        /*
         * Carefully bump m_uLastSequenceNumber
         */

        UINT16 uTestSequenceNumber = (UINT16)(m_uLastSequenceNumber +
                                              (uPacketIndex - uTailIndex));

        if (uTestSequenceNumber < m_uLastSequenceNumber ||
            uTestSequenceNumber >= m_wrapSequenceNumber)
        {
            for (i = 0; i < uPacketIndex - uTailIndex; i++)
            {
                m_uLastSequenceNumber++;

                if (m_uLastSequenceNumber == m_wrapSequenceNumber)
                {
                    m_uLastSequenceNumber = 0;
                }
            }
        }
        else
        {
            m_uLastSequenceNumber = uTestSequenceNumber;
        }

        /*
         * We did receive a valid packet
         */

        m_uNormal++;
    }

    if (m_uSeekCount == 0 && pPacket && !pPacket->IsLostPacket())
    {
        if (!m_bAtLeastOnePacketReceived)
        {
            m_bAtLeastOnePacketReceived = TRUE;
            m_ulFirstTimestampReceived  = uTimestamp;
            m_ulLastTimestampReceived   = m_ulFirstTimestampReceived;
        }
        else
        {
            m_ulLastTimestampReceived   = uTimestamp;
        }
    }

    // prefetch if we have received enough data(0x200)
    // NOTE: we only cache half of the packets(0x100) in order to
    //       give enough time to recover lost packets
    if (m_bPrefetch)
    {
        DoPrefetch();
    }

    CheckForSourceDone();

    return HXR_OK;
}

HX_RESULT
RTSPTransportBuffer::GetPacket(ClientPacket*& pPacket)
{
    HX_RESULT   rc = HXR_OK;

    pPacket = NULL;

    if( m_bStreamDone )
    {
	HXLOGL4(HXLOG_TRAN, "RTSPTransportBuffer::GetPacket() StreamDone");
        if( !m_bStreamDoneSent )
        {
            m_bStreamDoneSent = TRUE;
            m_pOwner->streamDone(m_uStreamNumber, m_ulEndReasonCode);
        }
        return HXR_AT_END;
    }
    else if (!m_bIsInitialized || m_uSeekCount || m_bWaitingForSeekFlush ||
             (m_bPaused && !m_bIsEnded))
    {
	HXLOGL4(HXLOG_TRAN, "RTSPTransportBuffer::GetPacket() NoData: Initialized=%c SeekCount=%lu WaitingForSeekFlush=%c Paused=%c Ended=%c", 
	    m_bIsInitialized ? 'T' : 'F',
	    m_uSeekCount,
	    m_bWaitingForSeekFlush ? 'T' : 'F',
	    m_bPaused ? 'T' : 'F',
	    m_bIsEnded ? 'T' : 'F');
        return HXR_NO_DATA;
    }

    if (!m_bCacheIsEmpty)
    {
        GetPacketFromCache(pPacket);
    }

    if (!pPacket)
    {
        rc = GetPacketFromQueue(pPacket);
    }

    if( IsQueueEmpty() && m_bCacheIsEmpty )
    {
        if (m_bIsEnded)
        {
            m_bStreamDone = TRUE;
        }
        else 
        {
            // Check if projected packet time-stamp is past expected 
            // end-time stamp, if yes we no longer expect packets.
            // If m_uLastTimestamp is 0, we haven't read any packets
            // and should not try considering source completion.
            //
            // XXX HP we only enable this logic when the last packet
            // we received is very close to the end of the duration since
            // we don't want to be confused with situations such as dropping
            // of UDP during the middle of the playback which should trigger
            // server timeout in this case.
            if ( m_bExpectedTSRangeSet && (m_uLastTimestamp != 0) &&
                ((m_uLastTimestamp + CLIPEND_WINDOW) > m_uEndTimestamp) )
            {
                ULONG32 ulCurrentDuration = m_uLastTimestamp - 
                                            m_uStartTimestamp;
                ULONG32 ulExpectedDuration = m_uEndTimestamp - 
                                             m_uStartTimestamp;

                // For content longer than 24 days, do not use time-range based
                // stream stoppage logic as it becomes more difficult to distinguish 
                // between post-end and pre-start timestamps
                if (((LONG32) ulExpectedDuration) > 0)
                {
                    UpdateTime(&m_PacketTime);

                    ULONG32 ulLastPacketTime =
                        m_LastPacketTime.m_LastTime.tv_sec * 1000 +
                        m_LastPacketTime.m_LastTime.tv_usec / 1000;

                    ULONG32 ulCurrentTime = 
                        m_PacketTime.m_LastTime.tv_sec * 1000 + 
                        m_PacketTime.m_LastTime.tv_usec / 1000;

                    ULONG32 ulTransportWaitTime = ulCurrentTime-ulLastPacketTime;

                    HXLOGL4( HXLOG_TRAN,
                             "[%p] TransTime: %lu  CurDur: %lu  ExpDur: %lu",
                             this, ulTransportWaitTime, ulCurrentDuration, ulExpectedDuration
                             );

                    if ((((LONG32) ulCurrentDuration) >= 0) &&
                        ((ulCurrentDuration + ulTransportWaitTime) >= 
                         (ulExpectedDuration + m_ulEndDelayTolerance)) &&
                        (ulTransportWaitTime >= m_ulEndDelayTolerance))
                    {
                        m_bSourceStopped = TRUE;
                    }
                }
            }

            if( m_bSourceStopped )
            {
                m_bIsEnded = TRUE;
                m_bStreamDone = TRUE;
                m_bStreamDoneSent = TRUE;
                m_pOwner->streamDone(m_uStreamNumber);
            }
        }
    }

    if (pPacket)
    {
	if (!m_bFirstPacketGet)
	{
            if(!pPacket->IsLostPacket()) {
	        m_bFirstPacketGet = TRUE;
	        HXLOGL1(HXLOG_TRAN,
                    "RTSPTransportBuffer[%p]: First Packet Retrieved %i",
                    this, m_uStreamNumber);
            } else {
                 //Return HXR_NO_DATA, if the first packet is a lost packet.
                 //This is to avoid the situation of the base time being set to 0,
                 //which could be far from real time-range of the packets and cause the player 
                 //to "freeze"
                 HX_RELEASE(pPacket);
                 rc = HXR_NO_DATA;
            }
	}
    }

    return rc;
}

HX_RESULT
RTSPTransportBuffer::StartPackets()
{
    ASSERT(!m_bPacketsStarted);

    m_bPacketsStarted = TRUE;

    return HXR_OK;
}

HX_RESULT
RTSPTransportBuffer::StopPackets()
{
    ASSERT(m_bPacketsStarted);

    m_bPacketsStarted = FALSE;

    return HXR_OK;
}

HX_RESULT
RTSPTransportBuffer::GetStatus
(
    UINT16& uStatusCode, 
    UINT16& ulPercentDone
)
{
    return HXR_NOTIMPL;
}

HX_RESULT
RTSPTransportBuffer::SetupForACKPacket
(    
    UINT16& uSeqNo,
    CHXBitset& pBitset,
    UINT16& uBitCount,
    HXBOOL& didACK,
    HXBOOL& bLostHigh,
    HXBOOL& bNeedAnotherACK
)
{
    if (m_bACKDone || !m_bIsInitialized)
    {
        return HXR_NO_DATA;
    }

    UINT16 uLastSequenceNumber = m_uLastSequenceNumber;
    HXBOOL bAllACK = FALSE;

    /*
     * The start and end indexes must be INT32 or the loop will not
     * terminate properly
     */

    ClientPacket* pPacket = 0;
    INT32 iPacketIndex = (INT32)GetPacketIndex(uLastSequenceNumber);
    INT32 iStartIndex = (INT32)GetACKIndex(uLastSequenceNumber);
    INT32 iEndIndex = 0;

    /*
     * 1) If the start index > MAX_DEQUE_SIZE then we have ACKed all the
     *    current packets
     * 2) If iPacketIndex = 0 AND
     *     A) the queue is empty, then we have never entered a packet into
     *        the queue
     *     B) the only packet is a sanitization packet, then we sanitized
     *        for a late packet
     */

    if (iStartIndex > MAX_DEQUE_SIZE)
    {
        return HXR_NO_DATA;
    }
    else if (iPacketIndex == 0)
    {
        if (!IsQueueEmpty())
        {
            pPacket = (ClientPacket*)(*m_pPacketDeque)[0];

            if (!pPacket->IsSanitizePacket())
            {
                goto SanitizeContinue;
            }
        }

        return HXR_NO_DATA;
    }

SanitizeContinue:

    INT32 i;

    /*
     * If we can't fit all the ACK/NAKs in one ACK packet, then start
     * ACK/NAKing from the beginning of the transport buffer
     */

    if (iStartIndex > MAX_BITSET_SIZE)
    {
        /*
         * Carefully set uLastSequenceNumber
         */

        uLastSequenceNumber = m_uACKSequenceNumber;

        for (i = 0; i < MAX_BITSET_SIZE; i++)
        {
            uLastSequenceNumber++;

            if (uLastSequenceNumber == m_wrapSequenceNumber)
            {
                uLastSequenceNumber = 0;
            }
        }

        /*
         * Reset the indexes with the last packet we will ACK/NAK
         */

        iPacketIndex = (INT32)GetPacketIndex(uLastSequenceNumber);
        iStartIndex = MAX_BITSET_SIZE;

        /*
         * Since the number of packets > the amount we can ACK, we may need
         * another ACK packet to fully clean up the ACK wait list. However,
         * if we run into a NAK, abort the back-to-back ACK because we
         * would just repeat the information going out in this ACK packet
         */

        bNeedAnotherACK = TRUE;
    }

    /*
     * We may have released more packets than can fit in an ACK packet or
     * the queue may be empty
     */

    if (iPacketIndex > MAX_DEQUE_SIZE)
    {
        bAllACK = TRUE;
        iPacketIndex = 0;
    }

    ASSERT(IsQueueEmpty() ? bAllACK : TRUE);

    UINT32 uLastNAKSequenceNumber = 0;
    HXBOOL bNAKFound = FALSE;
    uBitCount = HX_SAFEUINT16(iStartIndex);
    bLostHigh = FALSE;

    /*
     * We loop iStartIndex+1 times because we also need to set uSeqNo
     */

    for (i = iStartIndex; i >= iEndIndex; i--)
    {
        /*
         * We may have released this packet already
         */

        if (iPacketIndex < 0)
        {
            HX_ASSERT(i < iStartIndex);
            pBitset.set((iStartIndex - 1) - i);
            continue;
        }
        else if (iPacketIndex == 0)
        {
            /*
             * We may have released all the packets before ACKing them
             */

            if (bAllACK)
            {
                iPacketIndex--;
                uSeqNo = uLastSequenceNumber;
                didACK = TRUE;
                continue;
            }
        }

        pPacket = (ClientPacket*)(*m_pPacketDeque)[(u_long32)iPacketIndex--];

        /*
         * If the last packet is not valid, flag it for a NAK
         */

        if (i == iStartIndex)
        {
            if (pPacket->IsLostPacket())
            {
                bLostHigh = TRUE;
                bNeedAnotherACK = FALSE;

                if (!pPacket->IsResendRequested())
                {
                    pPacket->SetResendRequested();
                    m_uResendRequested++;
                }
            }

            uSeqNo = pPacket->GetSequenceNumber();
            didACK = TRUE;
            continue;
        }
        else if (pPacket->IsLostPacket())
        {
            bNAKFound = TRUE;
            bNeedAnotherACK = FALSE;
            uLastNAKSequenceNumber = pPacket->GetSequenceNumber();
            pBitset.set((iStartIndex - 1) - i);
            pBitset.clear((iStartIndex - 1) - i);

            if (!pPacket->IsResendRequested())
            {
                pPacket->SetResendRequested();
                m_uResendRequested++;
            }
        }
        else
        {
            pBitset.set((iStartIndex - 1) - i);
        }
    }

    /*
     * Bump the ACK counter
     */

    INT32 iACKCount;

    if (bNAKFound)
    {
        iACKCount = (INT32)GetACKIndex((UINT16) uLastNAKSequenceNumber);
    }
    else
    {
        iACKCount = (INT32)GetACKIndex((UINT16) uLastSequenceNumber) + 1;
    }

    /*
     * Carefully bump m_uACKSequenceNumber
     */

    UINT16 uTestSequenceNumber = (UINT16)(m_uACKSequenceNumber + iACKCount);

    if (m_bIsEnded                                 ||
        uTestSequenceNumber < m_uACKSequenceNumber ||
        uTestSequenceNumber >= m_wrapSequenceNumber)
    {
        for (i = 0; i < iACKCount; i++)
        {
            if (m_bIsEnded && m_uACKSequenceNumber == m_uEndSequenceNumber)
            {
                m_bACKDone = TRUE;
                break;
            }

            m_uACKSequenceNumber++;

            if (m_uACKSequenceNumber == m_wrapSequenceNumber)
            {
                m_uACKSequenceNumber = 0;
            }
        }
    }
    else
    {
        m_uACKSequenceNumber = uTestSequenceNumber;
    }

    return HXR_OK;
}

UINT32
RTSPTransportBuffer::GetIndex(UINT32 uBaseSequenceNumber, UINT16 uSeqNo)
{
    INT32 index = (INT32)(uSeqNo - uBaseSequenceNumber);

    if(index < 0)
    {
        index = (INT32)(m_wrapSequenceNumber - uBaseSequenceNumber + uSeqNo);
    }

    return (UINT32)index;
}

void
RTSPTransportBuffer::SetEndPacket
(
    UINT16 uSeqNo,
    UINT16 uReliableSeqNo,
    HXBOOL   bPacketSent,
    UINT32 uTimestamp,
    UINT32 ulReasonCode
)
{
    if (m_bIsEnded)
    {
        return;
    }

    m_ulEndReasonCode = ulReasonCode;

    //We have just received the last packet. Since we are getting no
    //more packets, make sure we go through the pending packets list
    //and send NAKs for each packet we have not recieved or got out
    //of order.
    HX_LOCK(m_pPendingLock);
    while(!m_PendingPackets.IsEmpty())
    {
        PendingPacket* pPend = (PendingPacket*)m_PendingPackets.RemoveHead();
        UINT32 tempIndex = GetPacketIndex((UINT16)pPend->m_ulSequenceNumber);
                
        //Send a NAK and increment resend requested count.
        m_pOwner->sendNAKPacket(m_uStreamNumber,
                                (UINT16)pPend->m_ulSequenceNumber,
                                (UINT16)pPend->m_ulSequenceNumber);
        if( tempIndex<m_pPacketDeque->size())
            ((ClientPacket*)(*m_pPacketDeque)[tempIndex])->SetResendRequested();
        m_uResendRequested++;
        //Clean up.
        HX_DELETE(pPend);
    }
    //We also don't need to call func anymore for this object.
    if (m_pScheduler && m_CallbackHandle)
    {
        m_pScheduler->Remove(m_CallbackHandle);
    }
    m_CallbackHandle = 0;
    if( m_pCallBack )
        m_pCallBack->Clear();
    
    HX_RELEASE( m_pCallBack );
    HX_UNLOCK(m_pPendingLock);

    m_bIsEnded = TRUE;
    m_uEndSequenceNumber = uSeqNo;

    UINT32 uEndIndex = GetPacketIndex(m_uEndSequenceNumber);

    // XXX HP we have too many empty queue determination
    // i.e. IsQueueEmpty()
    //      uEndIndex > MAX_DEQUEUE_SIZE
    //      m_pPacketDeque->empty() == TRUE
    //      m_pPacketDeque->size() == 0
    if (!bPacketSent || (uEndIndex > MAX_DEQUE_SIZE && m_bCacheIsEmpty))
    {
        /*
         * Either no packets were sent or the end packet has come
         * after the last packet has been released, so just send
         * stream done notification
         */

        m_bStreamDone = TRUE;
        m_bStreamDoneSent = TRUE;
        m_pOwner->streamDone(m_uStreamNumber);

        return;
    }

    /*
     * Since the buffer duration restriction is now lifted, the player
     * can get all the packets in the buffer. That means all the packets
     * must exist, so fill in the end of the queue with temporary "lost"
     * packets
     */

    ClientPacket* pPacket = new ClientPacket(uSeqNo,
                                             uReliableSeqNo,
                                             uTimestamp,
                                             0,
                                             0,
                                             0,
                                             GetTime(),
                                             FALSE);

    pPacket->AddRef();
    Add(pPacket);

    m_uEndReliableSeqNo = uReliableSeqNo;

    CheckForSourceDone();
}

void
RTSPTransportBuffer::InformSourceStopped
(
    void
)
{
    m_bSourceStopped = TRUE;
}

void
RTSPTransportBuffer::InformTimestampRange
(
    UINT32 ulStartTimestamp,
    UINT32 ulEndTimestamp,
    UINT32 ulEndDelayTolerance
)
{
    m_uStartTimestamp = ulStartTimestamp;
    m_uEndTimestamp = ulEndTimestamp;
    m_ulEndDelayTolerance = ulEndDelayTolerance;
    m_bExpectedTSRangeSet = TRUE;
}
            
HX_RESULT
RTSPTransportBuffer::UpdateStatistics
(
    ULONG32& ulNormal,
    ULONG32& ulLost,
    ULONG32& ulLate,
    ULONG32& ulResendRequested,
    ULONG32& ulResendReceived,
    ULONG32& ulAvgBandwidth,
    ULONG32& ulCurBandwidth,
    ULONG32& ulTotal30,
    ULONG32& ulLost30,
    ULONG32& ulDuplicate,
    ULONG32& ulOutOfOrder
)
{
    if (!m_bIsInitialized)
    {
        return HXR_NO_DATA;
    }

    HXLOGL2(HXLOG_TRAN, "RTSPTransportBuffer[%p]: Bytes Received %i %lu", this, m_uStreamNumber, HX_I64d_ARG(m_uByteCount));
    
    ulNormal            = m_uNormal;
    ulLost              = m_uLost;
    ulLate              = m_uLate;
    ulResendRequested   = m_uResendRequested;
    ulResendReceived    = m_uResendReceived;
    ulLost30            = m_ulLastLost30;
    ulTotal30           = m_ulLastTotal30;
    ulAvgBandwidth      = m_uAvgBandwidth;
    ulCurBandwidth      = m_uCurBandwidth;
    ulDuplicate         = m_ulDuplicate;
    ulOutOfOrder        = m_ulOutOfOrder;

    if (m_bIsEnded)
    {
        ulAvgBandwidth  = m_uAvgBandwidth = 0;
        ulCurBandwidth  = m_uCurBandwidth = 0;
        return HXR_OK;
    }

    if (m_bPaused || m_bPausedHack)
    {
        /*
         * This hack is needed because the server may send out an
         * extra packet when the stream is paused, and this unsettles
         * the bandwidth statistics when the stream is resumed
         */

        if (!m_bPaused && m_bPausedHack)
        {
            m_bPausedHack = FALSE;
        }

        return HXR_OK;
    }

    // caculate the lost/total packets during the last 30 seconds   
    m_ulLost30[m_ulIndex30 % 30] = m_uLost;
    m_ulTotal30[m_ulIndex30 % 30] = m_uNormal + m_uLost + m_uLate + m_uResendReceived;

    ulLost30 = m_ulLost30[m_ulIndex30 % 30] - 
               m_ulLost30[(m_ulIndex30 + 1) % 30];
    ulTotal30 = m_ulTotal30[m_ulIndex30 % 30] -
                m_ulTotal30[(m_ulIndex30 + 1) % 30];

    m_ulLastLost30  = ulLost30;
    m_ulLastTotal30 = ulTotal30;

    m_ulIndex30++;

    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
    Timeval now((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);

    /*
     * Must adjust m_StartTime and m_LastTime for the amount of time the
     * client has been paused
     */

    Timeval TotalTime = now - AdjustedStartTime(&m_StatisticsTime);
    Timeval TimeSlice = now - AdjustedLastTime(&m_StatisticsTime);

    UpdateTime(&m_StatisticsTime);

    if (TotalTime <= 0.0 || TimeSlice <= 0.0)
    {
        /*
         * This should not happen
         */

        return HXR_UNEXPECTED;
    }

    double uTotalSeconds = TotalTime.tv_sec + (TotalTime.tv_usec / 1000000.0);
    double uRecentSeconds = TimeSlice.tv_sec + (TimeSlice.tv_usec / 1000000.0);
    INT64 uBitCount = m_uByteCount * 8;
    INT64 uRecentBitCount = (m_uByteCount - m_uLastByteCount) * 8;

    m_uAvgBandwidth = INT64_TO_UINT32(uBitCount / uTotalSeconds);
    m_uCurBandwidth = INT64_TO_UINT32(uRecentBitCount / uRecentSeconds);

    ulAvgBandwidth = m_uAvgBandwidth;
    ulCurBandwidth = m_uCurBandwidth;

    m_uLastByteCount = m_uByteCount;

    return HXR_OK;
}

void
RTSPTransportBuffer::InitializeTime(BufferTimer* Timer)
{
    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();

    Timer->m_StartTime = Timeval((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);
    Timer->m_PreviousTime = Timeval((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);
    Timer->m_LastTime = Timeval((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);    
    Timer->m_PauseTime = Timeval(0.0);
}

void
RTSPTransportBuffer::UpdateTime(BufferTimer* Timer)
{
    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
    Timeval now((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);

    Timer->m_LastTime += now - Timer->m_PreviousTime;
    Timer->m_PreviousTime = now;
}

Timeval
RTSPTransportBuffer::GetTime(BufferTimer* Timer)
{
    return Timer->m_LastTime;
}

Timeval
RTSPTransportBuffer::GetTime()
{
    /*
     * Use m_PacketTime for GetTime() references
     */

    UpdateTime(&m_PacketTime);

    return GetTime(&m_PacketTime);
}

Timeval
RTSPTransportBuffer::AdjustedStartTime(BufferTimer* Timer)
{
    return Timer->m_StartTime + Timer->m_PauseTime;
}

Timeval
RTSPTransportBuffer::AdjustedLastTime(BufferTimer* Timer)
{
    return Timer->m_LastTime + Timer->m_PauseTime;
}

void
RTSPTransportBuffer::SetMulticast() 
{ 
    m_bMulticast = TRUE; 

    m_bSparseStream = m_pOwner->isSparseStream(m_uStreamNumber);
}

void
RTSPTransportBuffer::Pause()
{    
    HXLOGL3(HXLOG_TRAN, "RTSPTransportBuffer::Pause()");

    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
    Timeval now((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);

    m_bPaused = TRUE;

    m_StatisticsTime.m_LastTime += now - m_StatisticsTime.m_PreviousTime;
    m_StatisticsTime.m_PreviousTime = now;
    
    m_PacketTime.m_LastTime += now - m_PacketTime.m_PreviousTime;
    m_PacketTime.m_PreviousTime = now;
}

void
RTSPTransportBuffer::Resume()
{
    HXLOGL3(HXLOG_TRAN, "RTSPTransportBuffer::Resume() Paused=%c", m_bPaused ? 'T' : 'F');

    if (m_bPaused)
    {
        HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
        Timeval now((INT32)lTime.tv_sec, (INT32)lTime.tv_usec);

        m_bPaused = FALSE;
        m_bPausedHack = TRUE;

        m_StatisticsTime.m_PauseTime += now - m_StatisticsTime.m_PreviousTime;
        m_StatisticsTime.m_PreviousTime = now;
        
        m_PacketTime.m_PauseTime += now - m_PacketTime.m_PreviousTime;
        m_PacketTime.m_PreviousTime = now;
        
        m_ulBufferingStartTime  = HX_GET_TICKCOUNT();
        m_uLastByteCount        = m_uByteCount;
    }
}

void
RTSPTransportBuffer::SanitizePacketQueue()
{
    m_uLastSequenceNumber = m_uFirstSequenceNumber;

    /*
     * Put a temporary "lost" packet at the head of the queue to make
     * it sane. The timestamp must be set to that of the last packet
     * removed from the buffer. This prevents the early releasing of a
     * true lost packet
     */

    ClientPacket* pPacket = new ClientPacket(m_uFirstSequenceNumber,
                                             m_uReliableSeqNo,
                                             m_uLastTimestamp,
                                             0,
                                             0,
                                             0,
                                             GetTime(),
                                             TRUE);

    pPacket->AddRef();
    m_pPacketDeque->push_back(pPacket);
}

HX_RESULT
RTSPTransportBuffer::Flush()
{
    ClientPacket* pPacket;

    HXLOGL3(HXLOG_TRAN, "RTSPTransportBuffer::Flush()");

    //We are flushing all the packets. Empty out pending list.
    HX_LOCK(m_pPendingLock);
    while( !m_PendingPackets.IsEmpty() )
    {
        PendingPacket* pPend = (PendingPacket*)m_PendingPackets.RemoveHead();
        HX_DELETE(pPend);
    }
    //Get rid of any scheduler events...
    if (m_pScheduler && m_CallbackHandle)
    {
        m_pScheduler->Remove(m_CallbackHandle);
    }
    m_CallbackHandle = 0;
    if( m_pCallBack )
        m_pCallBack->Clear();
    HX_RELEASE( m_pCallBack );
    HX_UNLOCK(m_pPendingLock);

    while(!m_pPacketDeque->empty())
    {
        pPacket = (ClientPacket*)m_pPacketDeque->front();

        if (pPacket)
        {
            /*
             * Check to see that we are not waiting for a missing pre-seek
             * reliable packet
             */

            if (m_uReliableSeqNo !=
                pPacket->GetReliableSeqNo() - pPacket->IsReliable())
            {
                return HXR_INCOMPLETE;
            }

            UINT32 uSeekIndex = GetSeekIndex(pPacket->GetSequenceNumber());

            if (uSeekIndex == 0)
            {
                m_uLastTimestamp = pPacket->GetTime();
                return HXR_OK;
            }

            pPacket = (ClientPacket*)m_pPacketDeque->pop_front();

            IHXPacket* pIHXPacket = pPacket->GetPacket();

            m_pOwner->packetReady(HXR_OK,
                                  m_uStreamNumber,
                                  pIHXPacket);

            if (pIHXPacket)
            {
                pIHXPacket->Release();      
            }

            UpdateStatsFromPacket(pPacket);

            HX_RELEASE(pPacket);
        }
    }

/*
 * XXXGH...Do I really need to do this?
 *  InitializeTime(&m_PacketTime);
 */

    m_ulCurrentQueueByteCount   = 0;

    /*
     * It's possible that there are missing pre-seek packets that haven't
     * been marked as lost yet...they will be marked as lost after the
     * Insert(), so wait for the next incoming data packet before flushing
     * the queue
     */

    if (m_uFirstSequenceNumber != m_uSeekSequenceNumber)
    {
        return HXR_INCOMPLETE;
    }

    return HXR_OK;
}

HX_RESULT
RTSPTransportBuffer::GetCurrentBuffering(UINT32& ulLowestTimestamp, 
                                         UINT32& ulHighestTimestamp,
                                         UINT32& ulNumBytes,
                                         HXBOOL& bDone)
{
    UINT32  ulFrontTimeStamp = 0;
    UINT32  ulRearTimeStamp = 0;

    ulLowestTimestamp = 0;
    ulHighestTimestamp = 0;
    ulNumBytes = 0;
    bDone = m_bIsEnded;

    if (m_pPacketDeque && m_uSeekCount == 0 && !m_bWaitingForSeekFlush)
    {
        if (!m_bCacheIsEmpty && IsQueueEmpty())    
        {
            ulFrontTimeStamp = m_ulFrontTimeStampCached;
            ulRearTimeStamp = m_ulRearTimeStampCached;
        }
        else if (m_bCacheIsEmpty && !IsQueueEmpty())
        {
            ClientPacket* frontPacket   = (ClientPacket*)m_pPacketDeque->front();
            ClientPacket* rearPacket    = (ClientPacket*)m_pPacketDeque->back();

            ulFrontTimeStamp = frontPacket->GetTime();      
            ulRearTimeStamp = rearPacket->GetTime();
        }
        else if (!m_bCacheIsEmpty && !IsQueueEmpty())
        {
            ClientPacket* rearPacket    = (ClientPacket*)m_pPacketDeque->back();

            ulFrontTimeStamp = m_ulFrontTimeStampCached;            
            ulRearTimeStamp = rearPacket->GetTime();
        }
        else
        {
            goto cleanup;
        }

	ulLowestTimestamp = ulFrontTimeStamp;
	ulHighestTimestamp = ulRearTimeStamp;

        ulNumBytes = m_ulCurrentQueueByteCount + m_ulCurrentCacheByteCount;
    }

cleanup:

    return HXR_OK;
}

void
RTSPTransportBuffer::CheckForSourceDone()
{
    if (m_bIsEnded              &&
        m_bIsInitialized        &&
        m_uSeekCount == 0       &&
        !m_bWaitingForSeekFlush &&
        m_uEndReliableSeqNo == m_uReliableSeqNo)
    {
        m_pOwner->CheckForSourceDone(m_uStreamNumber);
    }
}

void
RTSPTransportBuffer::UpdateStatsFromPacket(ClientPacket* pPacket)
{
    m_uFirstSequenceNumber++;

    if (m_uFirstSequenceNumber == m_wrapSequenceNumber)
    {
        m_uFirstSequenceNumber = 0;
    }

    if (pPacket->IsReliable())
    {
        m_uReliableSeqNo++;
    }

    if (pPacket->IsLostPacket())
    {
        m_uLost++;
    }

    m_uLastTimestamp = pPacket->GetTime();
    
    m_ulCurrentQueueByteCount = m_ulCurrentQueueByteCount > pPacket->GetByteCount() ? 
                                m_ulCurrentQueueByteCount - pPacket->GetByteCount() :0;
}

void
RTSPTransportBuffer::SeekFlush()
{
    if (m_bMulticast)
    {
        m_bMulticastReset = TRUE;
        m_bMulticastReliableSeqNoSet = FALSE;
        m_uSeekCount = 1;
        Reset();

        return;
    }

    /* We use this to re-initialize the first sequence number 
     * since we do not get this information in live pause case.
     */
    m_bWaitingForLiveSeekFlush = TRUE;

    /*
     * If we're empty, there's nothing to flush
     */

    if (IsQueueEmpty())
    {
        return;
    }

    /*
     * In the seek flush case there will be no initialization packet,
     * so use the sequence number of the last packet in the buffer + 1
     * as the beginning sequence number of the post-seek packets
     */

    UINT32 uTailIndex = GetPacketIndex(m_uLastSequenceNumber);
    ClientPacket* tempPacket = (ClientPacket*)(*m_pPacketDeque)[uTailIndex];
    m_uSeekSequenceNumber = (UINT16)(tempPacket->GetSequenceNumber() + 1);

    if (m_uSeekSequenceNumber == m_wrapSequenceNumber)
    {
        m_uSeekSequenceNumber = 0;
    }
    
    m_bWaitingForSeekFlush = TRUE;
}

void
RTSPTransportBuffer::ReleasePackets()
{
    /*
     * If this is a live session try to send packets up to client
     */

    if (m_bIsLive)
    {
        HX_RESULT hresult;

        do
        {
            ClientPacket* pPacket = 0;

            hresult = GetPacket(pPacket);

            if (hresult == HXR_AT_END  ||
                hresult == HXR_NO_DATA ||
                hresult == HXR_BUFFERING)
            {
                break;
            }

            IHXPacket* pIHXPacket = pPacket->GetPacket();

            if (m_bPacketsStarted)
            {
                m_pOwner->packetReady(hresult,
                                      m_uStreamNumber,
                                      pIHXPacket);
            }

            HX_RELEASE(pIHXPacket);
            HX_RELEASE(pPacket);
        } while (hresult == HXR_OK);
    }
}

void
RTSPTransportBuffer::SetBufferDepth(UINT32 uMilliseconds)
{
    m_bufferDuration = uMilliseconds;

    if (m_maxBufferDuration < uMilliseconds)
    {
        m_maxBufferDuration = uMilliseconds;
    }
}

void 
RTSPTransportBuffer::SetBufferParameters(UINT32 uMinimumDelay,             /* ms */
                                         UINT32 uMaximumDelay,             /* ms */
                                         UINT32 uExtraDelayDuringBuffering /* ms */)
{
    if (uMaximumDelay < uMinimumDelay)
    {
        uMaximumDelay = uMinimumDelay + uExtraDelayDuringBuffering;
    }

    m_bufferDuration = uMinimumDelay;
    m_maxBufferDuration = uMaximumDelay;
    m_extraDelayDuringBuffering = uExtraDelayDuringBuffering;
}

void
RTSPTransportBuffer::EnterPrefetch(void)
{
#if defined(HELIX_FEATURE_FIFOCACHE) && defined(HELIX_FEATURE_PREFETCH)
    m_bPrefetch = TRUE;

    if (m_bPrefetch)
    {
        IUnknown* pContext = NULL;
        IHXCommonClassFactory* pClassFactory = NULL;

        m_pOwner->GetContext(pContext);

        if (pContext && 
            HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory,
                                               (void**)&pClassFactory))
        {
            HX_RELEASE(m_pFIFOCache);

            pClassFactory->CreateInstance(CLSID_IHXFIFOCache, 
                                          (void**)&m_pFIFOCache);
        }
        HX_RELEASE(pClassFactory);
        HX_RELEASE(pContext);
    }
#endif /* HELIX_FEATURE_FIFOCACHE && HELIX_FEATURE_PREFETCH */
    return;
}

void
RTSPTransportBuffer::LeavePrefetch(void)
{
    m_bPrefetch = FALSE;

    return;
}

void            
RTSPTransportBuffer::DoPrefetch(void)
{
#if defined(HELIX_FEATURE_FIFOCACHE) && defined(HELIX_FEATURE_PREFETCH)
    UINT32          i = 0;
    ClientPacket*   pClientPacket = NULL;

    if (m_pFIFOCache)
    {
        while (HXR_OK == GetPacketFromQueue(pClientPacket) && pClientPacket)
        {
            if (m_bCacheIsEmpty)
            {
                m_bCacheIsEmpty = FALSE;
                m_ulFrontTimeStampCached = m_ulRearTimeStampCached = pClientPacket->GetTime();
            }
            else
            {
                m_ulRearTimeStampCached = pClientPacket->GetTime();
            }

            m_pFIFOCache->Cache((IUnknown*)pClientPacket);
            m_ulCurrentCacheByteCount += pClientPacket->GetByteCount();

            HX_RELEASE(pClientPacket);
        }
    }
#endif /* HELIX_FEATURE_FIFOCACHE && HELIX_FEATURE_PREFETCH */
    return;
}

HX_RESULT
RTSPTransportBuffer::GetPacketFromCache(ClientPacket*& pPacket)
{
    pPacket = NULL;

#if defined(HELIX_FEATURE_FIFOCACHE) && defined(HELIX_FEATURE_PREFETCH)
    if (m_pFIFOCache)
    {
        m_pFIFOCache->Retrieve((IUnknown*&)pPacket);
        
        // no more cached packets left
        if (pPacket)
        {
            m_ulCurrentCacheByteCount = m_ulCurrentCacheByteCount > pPacket->GetByteCount() ? 
                                        m_ulCurrentCacheByteCount - pPacket->GetByteCount() :0;
        }
        else
        {
            HX_ASSERT(m_ulCurrentCacheByteCount == 0);
            m_bCacheIsEmpty = TRUE;
        }
    }
#endif /* HELIX_FEATURE_FIFOCACHE && HELIX_FEATURE_PREFETCH */

    return HXR_OK;
}

HX_RESULT
RTSPTransportBuffer::GetPacketFromQueue(ClientPacket*& pPacket)
{
    UINT32          ulTimeInQueue = 0;
    ClientPacket*   frontPacket = NULL;
    ClientPacket*   rearPacket = NULL;

    pPacket = NULL;

    if (IsQueueEmpty())
    {
        return HXR_NO_DATA;
    }

    frontPacket = (ClientPacket*)m_pPacketDeque->front();
    rearPacket = (ClientPacket*)m_pPacketDeque->back();

    /*
     * The transport buffer should NEVER send a sanitization packet to the
     * core
     */

    if (frontPacket->IsSanitizePacket())
    {
        return HXR_NO_DATA;
    }

    UINT32 ulFrontTimeStamp = frontPacket->GetTime();
    UINT32 ulRearTimeStamp = rearPacket->GetTime();

    if (ulFrontTimeStamp > ulRearTimeStamp &&
        ((ulFrontTimeStamp - ulRearTimeStamp) > MAX_TIMESTAMP_GAP))
    {
        ulTimeInQueue = INT64_TO_UINT32(CAST_TO_INT64 ulRearTimeStamp + MAX_UINT32 -
                                        CAST_TO_INT64 ulFrontTimeStamp);
    }
    else
    {
        ulTimeInQueue = ulRearTimeStamp - ulFrontTimeStamp;
    }

    Timeval TimeInBuffer;

    UpdateTime(&m_PacketTime);

    TimeInBuffer = m_PacketTime.m_LastTime - frontPacket->GetStartTime();

    /*
     * If...
     *
     * 1) the server is still sending packets    AND
     *    the first packet is lost               AND
     *    there are less than MAX_QUEUED_PACKETS AND
     *    there is not enough data in the buffer AND
     *    the first packet has not been in the buffer too long
     * 2) there was a reliable packet lost before this one
     *
     * then return HXR_BUFFERING
     */
    
    /*
     * If we are still in a buffering state AND the resend depth 
     * is not set to zero (to minimize latency), do not deplete the
     * network jitter buffer. 
     */
    UINT32 ulMinimumToBuffer = m_bufferDuration;

    HXBOOL bPlaying = FALSE;

    if (m_pOwner && m_pOwner->m_pPlayerState)
    {
        bPlaying = m_pOwner->m_pPlayerState->IsPlaying();

        if (!bPlaying)
            ulMinimumToBuffer += m_extraDelayDuringBuffering;
    }

    // Enforce the maximum buffer duration
    if (ulMinimumToBuffer > m_maxBufferDuration)
    {
        ulMinimumToBuffer = m_maxBufferDuration;
    }

    // We only want to get packets as soon as possible for FastStart when 
    // before starting playback. If already playing getting lost packets 
    // faster then usual prevents resent packets from being processed.
    if ((!m_bFastStart || bPlaying)                         &&
        (!m_bIsEnded                                        &&
         m_pPacketDeque->size() < MAX_QUEUED_PACKETS        &&
         frontPacket->IsLostPacket()                        &&
         ulTimeInQueue < ulMinimumToBuffer                  &&
         TimeInBuffer < Timeval((float)ulMinimumToBuffer / 1000.0)) ||
        (frontPacket->GetReliableSeqNo() != 
            (UINT16)(m_uReliableSeqNo + frontPacket->IsReliable())))
    {
        pPacket = 0;

        m_status = TRANSBUF_FILLING;
        return HXR_BUFFERING;
    }

    if (m_status != TRANSBUF_READY)
    {
        m_status = TRANSBUF_READY;
    }

    pPacket = (ClientPacket*)m_pPacketDeque->pop_front();

    //Remove this packet if it is in our pending packet list
    HX_LOCK(m_pPendingLock);
    LISTPOSITION pos      = m_PendingPackets.GetHeadPosition();
    UINT32       ulSeqNum = pPacket->GetSequenceNumber();
    while(pos)
    {
        PendingPacket* pPend = (PendingPacket*)m_PendingPackets.GetAt(pos);
        if( pPend->m_ulSequenceNumber == ulSeqNum )
        {
            m_PendingPackets.RemoveAt(pos);
            HX_DELETE( pPend );
            break;
        }
        m_PendingPackets.GetNext(pos);
    }
    HX_UNLOCK(m_pPendingLock);

    UpdateStatsFromPacket(pPacket);
    m_LastPacketTime = m_PacketTime;

    return HXR_OK;
}

void
RTSPTransportBuffer::InitTimer()
{
    m_pScheduler = m_pOwner->GetScheduler();

    if (m_pScheduler)
    {
        m_pScheduler->AddRef();
        InitializeTime(&m_StatisticsTime);
        InitializeTime(&m_PacketTime);
        m_LastPacketTime = m_PacketTime;
    }
}

HX_RESULT
RTSPTransportBuffer::GetTransportBufferInfo(UINT32& ulLowestTimestamp,
                                            UINT32& ulHighestTimestamp,
                                            UINT32& ulBytesBuffered)
{
    UINT32 ulLowTS;
    UINT32 ulHighTS;
    HXBOOL bDone;

    HX_RESULT res = GetCurrentBuffering(ulLowTS, ulHighTS, 
                                        ulBytesBuffered, bDone);

    if (HXR_OK == res)
    {
        if (ulBytesBuffered)
        {
            ulLowestTimestamp = ulLowTS;
            ulHighestTimestamp = ulHighTS;
        }
        else
        {
            // There isn't any data in the buffer. Set
            // the timestamps to the last timestamp received.
            // This allows the server to keep track of what
            // has been received when no data is in the buffer.
            ulLowestTimestamp = m_ulLastTimestampReceived;
            ulHighestTimestamp = m_ulLastTimestampReceived;
        }
    }

    return res;
}

void RTSPTransportBuffer::Func(void)
{
    UINT32 now = HX_GET_TICKCOUNT();

    //See if we should even be here.
    if( NULL==m_pCallBack || 0==m_CallbackHandle)
        return;

    HX_LOCK(m_pPendingLock);
    m_CallbackHandle = 0;
    
    //If this Func fired we have run out of time to wait for
    //more packets. We have to go through our pending packet
    //list and send NAKs for each one.
    LISTPOSITION pos = m_PendingPackets.GetHeadPosition();
    int nCount = m_PendingPackets.GetCount();
    while(pos && nCount)
    {
        PendingPacket* pPend = (PendingPacket*)m_PendingPackets.GetAt(pos);
        //Check and see how long this packet has been on the pending queue.
        if( now-(pPend->m_ulArrivalTime) > NAK_TIMEOUT )
        {
            //Send a NAK and increment resend requested count.
            UINT32 tempIndex = GetPacketIndex((UINT16)pPend->m_ulSequenceNumber);
            m_pOwner->sendNAKPacket(m_uStreamNumber,
                                    (UINT16)pPend->m_ulSequenceNumber,
                                    (UINT16)pPend->m_ulSequenceNumber);
            if( tempIndex<m_pPacketDeque->size())
                ((ClientPacket*)(*m_pPacketDeque)[tempIndex])->SetResendRequested();
            m_uResendRequested++;
            pos = m_PendingPackets.RemoveAt(pos);
            HX_DELETE(pPend);
        }
        else
            m_PendingPackets.GetNext(pos);
        
        nCount--;
    }
    //Schedule our next callback
    if( m_pScheduler && m_pCallBack )
        m_CallbackHandle = m_pScheduler->RelativeEnter(m_pCallBack, NAK_CHECK_INTERVAL);
    
    HX_UNLOCK(m_pPendingLock);
}


////////////////////////////////////////////////
//
//    basesite callback
//
////////////////////////////////////////////////
RTSPTransportBuffer::RTSPTransportBufferCallback::RTSPTransportBufferCallback(RTSPTransportBuffer* pTransBuff) :
    m_pTransBuff(pTransBuff),
    m_lRefCount (0)
{
}

RTSPTransportBuffer::RTSPTransportBufferCallback::~RTSPTransportBufferCallback()
{
    m_pTransBuff = NULL;
}

STDMETHODIMP RTSPTransportBuffer::RTSPTransportBufferCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
   
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

ULONG32 RTSPTransportBuffer::RTSPTransportBufferCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 RTSPTransportBuffer::RTSPTransportBufferCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (ULONG32)m_lRefCount;
    }
    
    delete this;
    return 0;
}


HX_RESULT RTSPTransportBuffer::RTSPTransportBufferCallback::Func(void)
{
    if(m_pTransBuff)
    {
        m_pTransBuff->Func();
    }
    return HXR_OK;
}

void RTSPTransportBuffer::RTSPTransportBufferCallback::Clear(void)
{
    m_pTransBuff=NULL;
} 

HXBOOL                
RTSPTransportBuffer::OverByteLimit() const
{
    HXBOOL bRet = FALSE;

    if (m_ulByteLimit && (m_ulCurrentQueueByteCount > m_ulByteLimit))
    {
        bRet = TRUE;
    }

    return bRet;
}

void 
RTSPTransportBuffer::ConvertToDroppedPkt(ClientPacket*& pPacket)
{
    ClientPacket* pLostPkt = new ClientPacket(pPacket->GetSequenceNumber(),
                                              pPacket->GetReliableSeqNo(),
                                              pPacket->GetTime(),
                                              0,
                                              0,
                                              0,
                                              pPacket->GetStartTime(),
                                              FALSE,
                                              TRUE);
    if (pLostPkt)
    {
        // Update queue byte count since this packet won't be counted
        // anymore
        m_ulCurrentQueueByteCount -= pPacket->GetByteCount();
        
        // Destroy the original packet
        HX_RELEASE(pPacket);
        
        pPacket = pLostPkt;
        pLostPkt->AddRef();
    }
}
