/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rvdrop.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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

#include "rvdrop.h"

RVDrop::RVDrop()
{
    m_drop_rate = 0;
    Reset();
}

RVDrop::~RVDrop()
{
}

void
RVDrop::Reset()
{
    m_last_time_stamp_valid = FALSE;
    m_last_time_stamp_sent = 0;
    m_sent_last_packet = FALSE;
    m_packets_dropped = 0;
}

BOOL
RVDrop::PacketApproved(IHXPacket* packet)
{
    if (m_sent_last_packet)
    {
	if (packet->GetTime() == m_last_time_stamp_sent)
	{
	    return TRUE;
	}
    }

    if (0 == m_drop_rate)
    {
	m_last_time_stamp_sent = packet->GetTime();
	m_sent_last_packet = FALSE;
	return TRUE;
    }

    if (!m_last_time_stamp_valid)
    {
	m_last_time_stamp_sent = packet->GetTime();
	m_sent_last_packet = FALSE;
	m_last_time_stamp_valid = TRUE;
	return FALSE;
    }

    if (m_last_time_stamp_sent != packet->GetTime())
    {
	m_last_time_stamp_sent = packet->GetTime();
	if (m_packets_dropped >= m_drop_rate - 1)
	{
	    m_packets_dropped = 0;
	    m_sent_last_packet = TRUE;
	    return TRUE;
	}
	else
	{
	    ++m_packets_dropped;
	    m_sent_last_packet = FALSE;
	    return FALSE;
	}
    }

    return FALSE;
}


