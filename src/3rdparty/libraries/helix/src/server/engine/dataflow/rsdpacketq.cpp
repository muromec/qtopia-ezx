/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rsdpacketq.cpp,v 1.6 2006/12/21 05:06:06 tknox Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <new.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "ihxpckts.h"
#include "hxassert.h"
#include "hxslist.h"
#include "hxdeque.h"
#include "hxmap.h"
#include "servpckts.h"
#include "base_errmsg.h"
#include "ihxlist.h"
#include "hxlistp.h"
#include "sink.h"

#include "servbuffer.h"
#include "livekeyframe.h"
#include "rsdpacketq.h"

CRSDPacketQueue::CRSDPacketQueue(UINT32 ulSize)
{
    m_pPacketBufferQueue = NULL;
    m_ulPktBufQIndex = 0;
    m_ulTimeLineStartTS = 0;
    m_pFirstPacket = NULL;
    m_pTailPacketTS = 0;
    m_bQueueDone = FALSE;
    m_pHeadPacket = NULL;
    m_packetList = new CHXSimpleList();
    m_ulSize = ulSize;
    m_ulMaxSize = ulSize*2;
}

CRSDPacketQueue::~CRSDPacketQueue()
{
    if(m_packetList)
    {
        IHXPacket* pPacket = NULL;

        while(m_packetList->IsEmpty() == FALSE)
        {
            pPacket = (IHXPacket*)m_packetList->RemoveHead();
            pPacket->Release();
        }
        delete m_packetList;
    }
    HX_RELEASE(m_pPacketBufferQueue);
    HX_RELEASE(m_pHeadPacket);
}

HX_RESULT CRSDPacketQueue::AddPacketBufferQueue(IHXLivePacketBufferQueue* pQueue)
{
    if(!pQueue)
    {
        return HXR_FAIL;
    }

    //normally it is one packet out, and one packet in.  So the maxsize of double initial
    //queue size should be enough. 
    if(pQueue->GetSize() > m_ulSize)
    {
        m_ulSize = pQueue->GetSize();
        m_ulMaxSize = m_ulSize*2;
    }
    
    m_pPacketBufferQueue = pQueue;
    pQueue->AddRef();
    return HXR_OK;
}

HX_RESULT CRSDPacketQueue::AddPacket(IHXPacket* pPacket)
{
    if(!pPacket || m_ulSize > m_ulMaxSize)
    {
        return HXR_FAIL;
    }
    
    if(!m_pFirstPacket)
    {
        m_pFirstPacket = pPacket;
    }

    m_ulSize++;
    m_packetList->AddTail(pPacket);
    pPacket->AddRef();
    m_pTailPacketTS = pPacket->GetTime();
}

HX_RESULT CRSDPacketQueue::PeekPacket(IHXPacket*& pPacket)
{
    if(!m_pHeadPacket)
    {
        m_pHeadPacket = GetHeadPacket();
    }
    
    if(!m_pHeadPacket)
    {
        return HXR_FAIL;
    }

    pPacket = m_pHeadPacket;
    pPacket->AddRef();
    return HXR_OK;
}

HX_RESULT CRSDPacketQueue::GetPacket(IHXPacket*& pPacket)
{
    if(m_pHeadPacket)
    {
        pPacket = m_pHeadPacket;
        m_pHeadPacket = NULL;
    }
    else
    {
        pPacket = GetHeadPacket();
    }
    
    if(!pPacket)
    {
        return HXR_FAIL;
    }
    m_ulSize--;

    return HXR_OK;
}

IHXPacket* CRSDPacketQueue::GetHeadPacket()
{
    IHXPacket* pPacket = NULL;

    if(m_bQueueDone)
    {
        if(m_packetList->IsEmpty() == FALSE)
        {
            pPacket = (IHXPacket*)m_packetList->RemoveHead();
        }
    }
    else if(m_pPacketBufferQueue)
    {
        m_pPacketBufferQueue->GetPacket(m_ulPktBufQIndex, pPacket); 
        m_ulPktBufQIndex++;
        if(pPacket == NULL)
        {
#ifdef RSD_LIVE_DEBUG
            printf("\n");
#endif
            //the q is done, time to precess the packets saved while processing the q
            m_bQueueDone = TRUE;
            pPacket = (IHXPacket*)m_packetList->RemoveHead();
        }
    }

    if(m_bQueueDone == FALSE)
    {

        if(pPacket == m_pFirstPacket)
        {
            // we reach the LIVE packet saved in the list 
            m_bQueueDone = TRUE;
        }
    }
    return pPacket;
}

UINT32 CRSDPacketQueue::GetQueueDuration()
{
    if(!m_pHeadPacket)
    {
        m_pHeadPacket = GetHeadPacket();
    }
    
    if(!m_pHeadPacket)
    {
        return 0;
    }

    return m_pTailPacketTS - m_pHeadPacket->GetTime();
}
    
HX_RESULT CRSDPacketQueue::GetHeadPacketTimeStamp(UINT32& ulTS)
{
    if(!m_pHeadPacket)
    {
        m_pHeadPacket = GetHeadPacket();
    }

    if(!m_pHeadPacket)
    {
        return HXR_FAIL;
    }

    ulTS = m_pHeadPacket->GetTime();
    return HXR_OK;
}

UINT32 CRSDPacketQueue::GetQueuePacketNumber()
{
    if(m_bQueueDone)
    {
        return m_packetList->GetCount();
    }
    else
    {
        return m_pPacketBufferQueue->GetSize() - m_ulPktBufQIndex;
    }
}
    


