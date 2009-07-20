
/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rsdpacketq.h,v 1.4 2006/10/16 21:34:18 dcollins Exp $
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
/*
 *  Interfaces for the Player Packet Manager
 */

#ifndef _RSDPACKETQ_H_
#define _RSDPACKETQ_H_

_INTERFACE IHXLivePacketBufferQueue;
_INTERFACE IHXPacket;
class CHXSimpleList;

class CRSDPacketQueue
{
public:
    CRSDPacketQueue(UINT32 nsize);
    ~CRSDPacketQueue();

    HX_RESULT AddPacketBufferQueue(IHXLivePacketBufferQueue* pQueue);

    HX_RESULT AddPacket(IHXPacket* pPacket);

    HX_RESULT GetPacket(IHXPacket*& pPacket );

    HX_RESULT PeekPacket(IHXPacket*& pPacket );

    UINT32 GetQueueDuration();

    HX_RESULT GetHeadPacketTimeStamp(UINT32& ulTS);

    UINT32 GetQueuePacketNumber();

private:

    IHXLivePacketBufferQueue*           m_pPacketBufferQueue;
    UINT32                              m_ulPktBufQIndex;
    CHXSimpleList*                      m_packetList;
    UINT32                              m_ulTimeLineStartTS;
    IHXPacket*                          m_pFirstPacket;
    UINT32                              m_pTailPacketTS;
    BOOL                                m_bQueueDone;
    IHXPacket*                          m_pHeadPacket;
    UINT32                              m_ulSize;
    UINT32                              m_ulMaxSize;


    IHXPacket* GetHeadPacket();
};


#endif //_RSDPACKETQ_H_
