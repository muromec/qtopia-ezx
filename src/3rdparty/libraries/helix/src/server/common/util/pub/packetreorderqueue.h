/****************************************************************************
 * 
 *  $Id: packetreorderqueue.h,v 1.3 2004/12/01 21:04:54 hwatson Exp $
 *
 *  Copyright (C) 2000 RealNetworks.
 *  All rights reserved.
 *  
 *  RealNetworks Confidential and Proprietary information.
 *  Do not redistribute.
 *
 *  RealSystem Broadcast Reception Plugin
 *
 */

#ifndef _PACKET_REORDER_QUEUE_H_
#define _PACKET_REORDER_QUEUE_H_

#include "hxcom.h"

#include "unkimp.h"

#include "timeval.h"
#include "seqno.h"
#include "pktreorderqueue.h"

class InOrderPacketDeliveryEntry 
    : public HXListElem
    , public CQueueEntry
{
public:
    InOrderPacketDeliveryEntry(IHXPacket* pPacket);
    ~InOrderPacketDeliveryEntry();

    IHXPacket* m_pPacket;
};

inline
InOrderPacketDeliveryEntry::InOrderPacketDeliveryEntry(IHXPacket* pPacket)
    : CQueueEntry(1)
    , m_pPacket(pPacket)
{
    HX_ADDREF(m_pPacket);
}

inline
InOrderPacketDeliveryEntry::~InOrderPacketDeliveryEntry()
{
    HX_RELEASE(m_pPacket);
}

class CInOrderPacketDeliveryQueue
    : public CInorderPacketQueue
{
public:
    virtual InOrderPacketDeliveryEntry* peek_front() 
    {
	if(CInorderPacketQueue::size())
	{
	    return (InOrderPacketDeliveryEntry*)CInorderPacketQueue::peek_front();
	}
	else
	{
	    return NULL;
	}
    }
    virtual InOrderPacketDeliveryEntry* peek(SequenceNumber& SeqNo) 
    {
	return (InOrderPacketDeliveryEntry*)CInorderPacketQueue::peek(SeqNo);
    }
    virtual InOrderPacketDeliveryEntry* peek(UINT32 uIndex) 
    {
	return (InOrderPacketDeliveryEntry*)CInorderPacketQueue::peek(uIndex);
    }
    virtual InOrderPacketDeliveryEntry* pop_front() 
    {
	return (InOrderPacketDeliveryEntry*)CInorderPacketQueue::pop_front();
    }
	
protected:
    virtual CQueueEntry* CreateLostPacket(SequenceNumber& SeqNo, Timeval& tvTime);
};    

inline CQueueEntry*
CInOrderPacketDeliveryQueue::CreateLostPacket(SequenceNumber& SeqNo, Timeval& tvTime)
{
    // we cheat and initialize the entry with RefCnt set to 1
    InOrderPacketDeliveryEntry* pTmpEntry = new InOrderPacketDeliveryEntry(NULL);
    
    pTmpEntry->m_SequenceNumber = SeqNo;
    pTmpEntry->m_tTime = tvTime;

    return pTmpEntry;
}


#endif
