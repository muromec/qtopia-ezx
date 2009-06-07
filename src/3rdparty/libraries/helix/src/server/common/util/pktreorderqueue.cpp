/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pktreorderqueue.cpp,v 1.1 2004/07/30 17:17:00 ghori Exp $ 
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
#include "hxcom.h"
#include "hxtypes.h"
#include "hxdeque.h"
#include "hxengin.h"
#include "seqno.h"

#include "pktreorderqueue.h"


//#define DEBUG_PKT_ORDER_Q
const UINT16 MAX_DEQUE_SIZE = 0x8000;


CInorderPacketQueue::CInorderPacketQueue()
  : m_FirstSequenceNumber(0)
  , m_bSetFirstSequenceNumber(FALSE)
{
    m_pPacketDeque = new HX_deque;
}

CInorderPacketQueue::~CInorderPacketQueue()
{
    clear();
    HX_DELETE(m_pPacketDeque);
}

void
CInorderPacketQueue::clear()
{
    HX_ASSERT(m_pPacketDeque);
    CQueueEntry* pEntry;
    while(!m_pPacketDeque->empty())
    {	
        pEntry = pop_front();
	HX_RELEASE(pEntry);
    }
}

UINT32
CInorderPacketQueue::size()
{
    return (UINT32)m_pPacketDeque->size();
}

BOOL
CInorderPacketQueue::empty()
{
    return m_pPacketDeque->empty();
}


CQueueEntry*
CInorderPacketQueue::peek_front()
{
    return (CQueueEntry*)m_pPacketDeque->front();
}


CQueueEntry*
CInorderPacketQueue::peek(SequenceNumber& SeqNo)
{
    UINT32 index = GetPacketIndex(SeqNo);

    if (index >= (UINT32)m_pPacketDeque->size())
    {
        return NULL;
    }

    return (CQueueEntry*)(*m_pPacketDeque)[index];
}

CQueueEntry*
CInorderPacketQueue::peek(UINT32 uIndex)
{
    if (uIndex >= (UINT32)m_pPacketDeque->size())
    {
        return NULL;
    }

    return (CQueueEntry*)(*m_pPacketDeque)[uIndex];
}


CQueueEntry*
CInorderPacketQueue::pop_front()
{
    m_FirstSequenceNumber++;
    return (CQueueEntry*)m_pPacketDeque->pop_front();
}

HX_RESULT
CInorderPacketQueue::add(CQueueEntry* pEntry, SequenceNumber& SeqNo)
{
    HX_ASSERT(pEntry);

    /* If this hasn't been set yet, set it */
    if (m_bSetFirstSequenceNumber == FALSE)
    {
        m_bSetFirstSequenceNumber = TRUE;
        m_FirstSequenceNumber = SeqNo;
    }
    
    UINT32 index = GetPacketIndex(SeqNo);

#ifdef DEBUG_PKT_ORDER_Q
    printf("InOrderPktQ(%p) add entry %p, seq %u, index %u size %u\n", 
           this, pEntry, (long)SeqNo, index, m_pPacketDeque->size());
    fflush(0);
#endif

    if (index >= MAX_DEQUE_SIZE)
    {
#ifdef DEBUG_PKT_ORDER_Q
      printf("InOrderPktQ(%p) index too large, ignoring\n", this);
#endif
        return HXR_INVALID_PARAMETER;
    }
    
    // If this falls beyond the end of the queue, its new and its out-of-order
    // as well, fill the gap with NULLs.
    while (m_pPacketDeque->size() < index)
    {
	CQueueEntry* pTmpEntry = (CQueueEntry*)CreateLostPacket(SeqNo, pEntry->m_tTime);		
        m_pPacketDeque->push_back(pTmpEntry);
        
#ifdef DEBUG_PKT_ORDER_Q
        printf("InOrderPktQ(%p) added placeholder for late entry %p, seq %u, index %d size %d\n", this, pEntry, (long)SeqNo, index, m_pPacketDeque->size());
#endif
    }

    // If this falls at the end of the queue, its new, pop it on there,
    // otherwise it replacing an earlier gap (ie its a late packet)
    if (m_pPacketDeque->size() == index)
    {
	pEntry->AddRef();
        m_pPacketDeque->push_back(pEntry);
    }
    else
    {
        /*
         * our placeholder is already in the queue, we replace it
         * but we keep the time when it *should* have been received. */

        HX_ASSERT((*m_pPacketDeque)[index]);

        CQueueEntry* pTmpEntry = (CQueueEntry*)(*m_pPacketDeque)[index];
        pEntry->m_tTime = pTmpEntry->m_tTime;

	pEntry->AddRef();
        (*m_pPacketDeque)[index] = pEntry;
        
#ifdef DEBUG_PKT_ORDER_Q
        printf("InOrderPktQ(%p) replaced placeholder %p with entry %p, size %d\n", this, pTmpEntry, pEntry, m_pPacketDeque->size());
#endif
        HX_RELEASE(pTmpEntry);
    }

    return HXR_OK;
}


