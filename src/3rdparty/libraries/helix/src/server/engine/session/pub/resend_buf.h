/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: resend_buf.h,v 1.2 2003/01/23 23:42:57 damonlan Exp $ 
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

#ifndef _RESEND_BUF_H_
#define _RESEND_BUF_H_

struct IHXBuffer;
class HX_deque;

class ResendBuffer
{
public:
    class		Packet;

			ResendBuffer(UINT32 buffer_duration,
				     UINT32 max_buffer_duration,
				     UINT32 growth_rate);
			~ResendBuffer();

    void		Clear();
    void		Grow();
    void	    	Add(IHXBuffer* buf, UINT32 time_stamp);
    Packet*	    	Find(UINT16 sequence_num);

    class Packet
    {
    public:
			Packet(IHXBuffer* buf, UINT32 time_stamp);
			~Packet();

	IHXBuffer*	m_buf;
	UINT32		m_time_stamp;
	UINT32		m_times_sent;
    };

private:
    void		DiscardExpiredPackets(UINT32 latest_time_stamp);

    HX_deque*		m_deque;
    UINT32		m_buffer_duration;
    UINT16		m_rear_seq_num;
    UINT32		m_max_buffer_duration;
    UINT32		m_growth_rate;
};

#endif /* _RESEND_BUF_H_ */
