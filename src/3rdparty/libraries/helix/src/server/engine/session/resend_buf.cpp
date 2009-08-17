/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: resend_buf.cpp,v 1.3 2003/09/04 22:35:35 dcollins Exp $ 
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
#include "ihxpckts.h"

#include "hxdeque.h"

#include "debug.h"

#include "resend_buf.h"

ResendBuffer::ResendBuffer(UINT32 buffer_duration, UINT32 max_buffer_duration,
			   UINT32 growth_rate)
{
    m_buffer_duration 		= buffer_duration;
    m_max_buffer_duration 	= max_buffer_duration;
    m_growth_rate 		= growth_rate;
    m_rear_seq_num 		= 0;
    m_deque 			= new HX_deque();
}

ResendBuffer::~ResendBuffer()
{
    Clear();
    delete m_deque;
}

void
ResendBuffer::Clear()
{
    HX_deque::Iterator	i;
    Packet*		current_packet;

    while (!m_deque->empty())
    {
	current_packet = (Packet*) m_deque->pop_back();
	delete current_packet;
	++m_rear_seq_num;
    }
}

void
ResendBuffer::Add(IHXBuffer* buf, UINT32 time_stamp)
{
    Packet*	new_packet;

    new_packet = new Packet(buf, time_stamp);
    m_deque->push_back(new_packet);

    if (time_stamp)
    {
	DiscardExpiredPackets(time_stamp);
    }
}

ResendBuffer::Packet*
ResendBuffer::Find(UINT16 sequence_num)
{
    INT32	index;
    Packet*	return_packet;

    index = sequence_num - m_rear_seq_num;
    if (index < 0)
    {
	index = 0x10000 - m_rear_seq_num + sequence_num;
    }
    if (((UINT32)index) >= m_deque->size())
    {
	DPRINTF(0x00800000, ("resend packet %d not found. index %ld. rear seq: "
			     "%d size %ld.\n", sequence_num, index, 
			     m_rear_seq_num, m_deque->size()));
	Grow();
	return 0;
    }
    return_packet = (Packet*) (*m_deque)[index];
    ASSERT(return_packet != 0);
    DPRINTF(0x00800000, ("resend packet %d found at index %ld. rear seq: " 
			 "%d size %ld.\n", sequence_num, index, m_rear_seq_num,
			 m_deque->size()));

    return return_packet;
}

void
ResendBuffer::Grow()
{
    if (m_buffer_duration + m_growth_rate <= m_max_buffer_duration)
    {
	m_buffer_duration += m_growth_rate;
    }
}

void
ResendBuffer::DiscardExpiredPackets(UINT32 latest_time_stamp)
{
    Packet*	current_packet;

    current_packet = (Packet*) m_deque->front();
    while (current_packet != 0 && ((!current_packet->m_buf) || 
	   (latest_time_stamp - current_packet->m_time_stamp > m_buffer_duration)))
    {
	Packet* front_packet;

	front_packet = (Packet*) m_deque->pop_front();
	if (front_packet)
	{
	    delete front_packet;
	}
	++m_rear_seq_num;
	current_packet = (Packet*) m_deque->front();
    }
}

ResendBuffer::Packet::Packet(IHXBuffer* buf, UINT32 time_stamp)
{
    m_buf 		= buf;
    m_time_stamp 	= time_stamp;
    m_times_sent	= 0;
    if (m_buf)
    {
	m_buf->AddRef();
    }
}

ResendBuffer::Packet::~Packet()
{
    HX_RELEASE(m_buf);
}
