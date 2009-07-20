/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servrsnd.cpp,v 1.9 2005/08/02 18:00:42 albertofloyd Exp $
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
#include "hxmap.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "basepkt.h"
#include "servrsnd.h"
#include "rtspif.h"
#include "rtsptran.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

RTSPResendBuffer::RTSPResendBuffer
(
    UINT32 bufferDuration,
    UINT32 maxBufferDuration,
    UINT32 growthRate,
    UINT32 wrapSequenceNumber
) : m_bufferDuration(bufferDuration),
    m_maxBufferDuration(bufferDuration), // XXXGo Is this right?
    m_growthRate(growthRate),
    m_wrapSequenceNumber(wrapSequenceNumber),
    m_uFirstSequenceNumber(0),
    m_uForceSequenceNumber(0),
    m_ulResendSuccess(0),
    m_ulResendFailure(0),
    m_bSetFirstSequenceNumber(FALSE)
{
    m_pPacketDeque = new HX_deque;
    m_pTSWindowQ = new HX_deque;
}

RTSPResendBuffer::~RTSPResendBuffer()
{
    Clear();
    delete m_pPacketDeque;
    delete m_pTSWindowQ;
}

void
RTSPResendBuffer::Clear()
{
    BasePacket* pPacket;
    while(!m_pPacketDeque->empty())
    {
        m_pTSWindowQ->pop_front();
	pPacket = (BasePacket*)m_pPacketDeque->pop_front();

	if (pPacket)
	{
	    pPacket->Release();
	}

	m_uFirstSequenceNumber++;

	if (m_uFirstSequenceNumber == m_wrapSequenceNumber)
	{
	    m_uFirstSequenceNumber = 0;
	}
    }
}

void
RTSPResendBuffer::Add(BasePacket* pPacket)
{
    /* If this hasn't been set yet, set it (multicast) */
    if (m_bSetFirstSequenceNumber == FALSE)
    {
	SetFirstSequenceNumber(pPacket->m_uSequenceNumber);
    }

    /*
     * If the sequence number is already in the queue, then this is
     * a duplicate packet.
     */

    if (Find(pPacket->m_uSequenceNumber, FALSE))
    {
        return;
    }

    pPacket->AddRef();
    UINT32 index = GetPacketIndex(pPacket->GetSequenceNumber());
    UINT32 uTS = pPacket->GetTime();

    /* XXXJC - what if no packets come in for a long time ... the queue empties
     * Now we get a packet with a jump in the sequence number, how do we decide
     * to resync m_uFirstSequenceNumber? 
     * I think we should check TS and resync if its greater than the queue size 
     * but this needs more thought. */

    /* If this packet is too late discard it */
    if (index >= MAX_DEQUE_SIZE)
    {
        pPacket->Release();
        return;
    }

    while (m_pPacketDeque->size() < index)
    {
	m_pTSWindowQ->push_back((void*)uTS);
	m_pPacketDeque->push_back(0);
    }


    // If this falls at the end of the queue, its new, pop it on there,
    // otherwise its replacing an earlier gap (ie its a late packet)
    if (m_pPacketDeque->size() == index)
    {
        m_pTSWindowQ->push_back((void*)uTS);
        m_pPacketDeque->push_back(pPacket);
    }
    else
    {
        // If we are here this is a late packet. We need to queue it up but
        // only if its in our window of time stamps.
        HX_ASSERT(!(*m_pPacketDeque)[index]);

        if (index < m_pTSWindowQ->size())
        {
            (*m_pPacketDeque)[index] = pPacket;
        }
    }

#ifdef DEBUG
    UINT32 uTestIndex = GetPacketIndex(pPacket->GetSequenceNumber());
    BasePacket* pTestPacket = (BasePacket*)(*m_pPacketDeque)[uTestIndex];
    /* GoGoGadget Short-Circuit Boolean Eval! */
    HX_ASSERT(!pTestPacket ||
              pTestPacket->GetSequenceNumber() == pPacket->GetSequenceNumber());
#endif

}

BasePacket*
RTSPResendBuffer::Find(UINT16 uSeqNo, HXBOOL bIsNAK)
{
    BasePacket* pPacket;

    UINT32 index = GetPacketIndex(uSeqNo);

    /*
     * If the packet is not found, then the resend buffer is not big
     * enough, so grow it
     */

    if(((UINT32)index) >= m_pPacketDeque->size())
    {
	/*
	 * Only want to grow the buffer if a packet that has been NAKed
	 * was removed too soon
	 */

	if (bIsNAK)
	{
	    Grow();
	    m_ulResendFailure++;
	}

	return 0;
    }

    pPacket = (BasePacket*)(*m_pPacketDeque)[index];

    if (bIsNAK)
    {
	m_ulResendSuccess++;
    }
    return pPacket;
}

void
RTSPResendBuffer::Remove(UINT16 uSeqNo)
{
    BasePacket* pPacket = Find(uSeqNo, FALSE);

    if (pPacket)
    {
	(*m_pTSWindowQ)[GetPacketIndex(uSeqNo)] = 0;
	(*m_pPacketDeque)[GetPacketIndex(uSeqNo)] = 0;

	pPacket->Release();
    }
}

void
RTSPResendBuffer::Grow()
{
    if (m_bufferDuration + m_growthRate <= m_maxBufferDuration)
    {
	m_bufferDuration += m_growthRate;
    }
}

void
RTSPResendBuffer::DiscardExpiredPackets(HXBOOL bForce, UINT32 uParameter)
{
    UINT32 uLastTimeStamp = 0;

    if (bForce)
    {
        m_uForceSequenceNumber = (UINT16)uParameter;
    }
    else
    {
        uLastTimeStamp = uParameter;
    }

    BasePacket* pPacket;
    UINT32 uTS;

    while(!m_pPacketDeque->empty())
    {
        uTS = (UINT32)m_pTSWindowQ->front();
        pPacket = (BasePacket*)m_pPacketDeque->front();

        /*
         * The packet will be NULL if it has been removed by an ACK or
         * has not arrived yet. If it was removed by an ACK the TS will also be 0 
         * and its safe to advance the array index.
         */
        if (!pPacket && !uTS)
        {
            m_pTSWindowQ->pop_front();
            m_pPacketDeque->pop_front();

            m_uFirstSequenceNumber++;

            if (m_uFirstSequenceNumber == m_wrapSequenceNumber)
            {
                m_uFirstSequenceNumber = 0;
            }

            continue;
        }

        /*
         * 1) If we are not forcing the discard, quit if the packet
         *    A) has not been in the buffer long enough or
         *    B) is reliable
         * 2) If we are forcing the discard, quit when the loop reaches
         *    the discard sequence number
         */

        if ((!bForce && ((uLastTimeStamp - uTS <= m_bufferDuration) ||
                         (pPacket && (pPacket->m_uPriority == 10)))) ||
            (bForce && GetForceIndex(m_uFirstSequenceNumber) < MAX_DEQUE_SIZE))
        {
            break;
        }

        m_pTSWindowQ->pop_front();
        pPacket = (BasePacket*)m_pPacketDeque->pop_front();
        HX_RELEASE(pPacket);

        m_uFirstSequenceNumber++;

        if (m_uFirstSequenceNumber == m_wrapSequenceNumber)
        {
            m_uFirstSequenceNumber = 0;
        }
    }
}

void
RTSPResendBuffer::SetFirstSequenceNumber(UINT16 uSeqNo)
{
    /*
     * Only allow the first sequence number to be set once
     */
    if (!m_bSetFirstSequenceNumber)
    {
	m_bSetFirstSequenceNumber = TRUE;
	m_uFirstSequenceNumber = uSeqNo;
    }
}

UINT32
RTSPResendBuffer::GetIndex(UINT32 uBaseSequenceNumber, UINT16 uSeqNo)
{
    INT32 index = (INT32)(uSeqNo - uBaseSequenceNumber);

    if(index < 0)
    {
	index = (INT32)(m_wrapSequenceNumber - uBaseSequenceNumber + uSeqNo);
    }

    return (UINT32)index;
}

HX_RESULT
RTSPResendBuffer::UpdateStatistics(UINT32& ulResendSuccess,
                                   UINT32& ulResendFailure)
{
    ulResendSuccess = m_ulResendSuccess;
    ulResendFailure = m_ulResendFailure;

    return HXR_OK;
}

void
RTSPResendBuffer::SetBufferDepth(UINT32 uMilliseconds)
{
    m_bufferDuration = uMilliseconds;

    if (m_maxBufferDuration < uMilliseconds)
    {
	m_maxBufferDuration = uMilliseconds;
    }
}

void		
RTSPResendBuffer::SetMaxBufferDepth(UINT32 uMilliseconds)
{
    if (m_maxBufferDuration < uMilliseconds)
    {    
	m_maxBufferDuration = uMilliseconds;
    }
    else
    {
	HX_ASSERT(FALSE);
    }	
}
