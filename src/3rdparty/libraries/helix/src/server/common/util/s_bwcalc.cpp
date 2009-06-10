/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: s_bwcalc.cpp,v 1.3 2003/08/09 17:59:53 dcollins Exp $ 
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

#include "hlxclib/string.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxengin.h"
#include "hxassert.h"
#include "s_bwcalc.h"

SBandwidthCalculator::SBandwidthCalculator(IHXScheduler* pSched,
					   UINT32 ulPackets,
					   UINT32 ulMinPackets)
: m_ulGran(ulPackets)
, m_pTable(0)
, m_ulMinPackets(ulMinPackets)
, m_ulHeadIndex(0)
, m_ulTailIndex(0)
, m_ulCount(0)
, m_dFirstTime(0.0)
, m_dLastTime(0.0)
, m_ulTotalSize(0)
, AllocGran(256)
, m_ulQueueSize(0)
{
    HX_ASSERT(ulPackets > 1);
    m_pSched = pSched;
    m_pSched->AddRef();
    m_ulQueueSize = SizeRound(m_ulGran);
    m_pTable = new BWNode[m_ulQueueSize];
}

SBandwidthCalculator::~SBandwidthCalculator()
{
    delete[] m_pTable;
    m_pTable = 0;
    HX_RELEASE(m_pSched);
}

void
SBandwidthCalculator::SetGranularity(UINT32 ulPackets)
{
    HX_ASSERT(ulPackets > 1);
    if (ulPackets == m_ulGran)
	return;
    
    /*
     * If we are shrinking the table and there will not
     * be enough room for some of these, kill them now.
     */
    while (m_ulCount > ulPackets)
    {
	m_ulTotalSize -= m_pTable[m_ulHeadIndex].ulSize;
	m_ulHeadIndex++;
	if (m_ulHeadIndex == m_ulGran)
	{
	    m_ulHeadIndex = 0;
	}
	m_ulCount--;
	m_dFirstTime = m_pTable[m_ulHeadIndex].dTime;
    }

    /*
     * If the queue is not split...
     */
    if (m_ulHeadIndex < m_ulTailIndex)
    {
	/*
	 * Do we need to allocate more?
	 */
	if (ulPackets > m_ulQueueSize)
	{
	    m_ulQueueSize = SizeRound(ulPackets);
	    BWNode* pNewTable = new BWNode[m_ulQueueSize];
	    memcpy(pNewTable, m_pTable + m_ulHeadIndex,
		(m_ulTailIndex - m_ulHeadIndex) * sizeof(BWNode));
	    delete[] m_pTable;
	    m_pTable = pNewTable;
	    m_ulTailIndex -= m_ulHeadIndex;
	    m_ulHeadIndex = 0;
	}
	else
	{
	    memmove(m_pTable, m_pTable + m_ulHeadIndex,
		    (m_ulTailIndex - m_ulHeadIndex) * sizeof(BWNode));
	    m_ulTailIndex -= m_ulHeadIndex;
	    m_ulHeadIndex = 0;
	}
	/*
	 * If we don't need to allocate more then nothing to do
	 */
    }
    /*
     * Queue is split.
     */
    else
    {
	/*
	 * Do we need to allocate more?
	 */
	if (ulPackets > m_ulQueueSize)
	{
	    m_ulQueueSize = SizeRound(ulPackets);
	    BWNode* pNewTable = new BWNode[m_ulQueueSize];

	    /*
	     * Copy over first chunk.
	     */
	    memcpy(pNewTable, m_pTable + m_ulHeadIndex,
		(m_ulGran - m_ulHeadIndex) * sizeof(BWNode));
	    /*
	     * Copy over second chunk.
	     */
	    memcpy(pNewTable + (m_ulGran - m_ulHeadIndex),
		m_pTable, m_ulTailIndex * sizeof(BWNode));
	    
	    delete[] m_pTable;
	    m_pTable = pNewTable;
	    m_ulHeadIndex = 0;
	    m_ulTailIndex = m_ulCount;
	}
	else
	{
	    /*
	     * We have enough space so just copy the end chunk
	     * to the new end. For the queue to be split we
	     * know that the bottom chunk starts at 0.
	     */
	    memmove(m_pTable + (ulPackets - (m_ulGran - m_ulHeadIndex)),
		    m_pTable + m_ulHeadIndex,
		    (m_ulGran - m_ulHeadIndex) * sizeof(BWNode));
	    
	    m_ulHeadIndex += (ulPackets - m_ulGran);
	}
    }

    m_ulGran = ulPackets;
    if (m_ulTailIndex == m_ulGran)
    {
	m_ulTailIndex = 0;
    }
}

void
SBandwidthCalculator::AddBytes(UINT32 ulSize)
{
    /*
     * If we have filled the queue then go ahead and overwrite
     * onto the head.
     */
    if (m_ulTailIndex == m_ulHeadIndex && m_ulCount != 0)
    {
	/*
	 * We have lost a packet, so remote his size from our count and...
	 */
	m_ulTotalSize -= m_pTable[m_ulHeadIndex].ulSize;
	m_ulHeadIndex ++;
	m_ulCount --;
	if (m_ulHeadIndex == m_ulGran)
	{
	    m_ulHeadIndex = 0;
	}
	/*
	 * ... Set the first time to the next time in the list.
	 */
	m_dFirstTime = m_pTable[m_ulHeadIndex].dTime;
    }

    /*
     * Set the time and size for this node.
     */
    m_pTable[m_ulTailIndex].ulSize = ulSize;
    HXTimeval tv = m_pSched->GetCurrentSchedulerTime();
    m_pTable[m_ulTailIndex].dTime =
	(double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);

    /*
     * If we are adding our first one then now is the time
     * to set the initial first time.
     */
    if (m_ulCount == 0)
    {
	m_dFirstTime = m_pTable[m_ulTailIndex].dTime;
    }
    m_dLastTime = m_pTable[m_ulTailIndex].dTime;
    m_ulTotalSize += m_pTable[m_ulTailIndex].ulSize;
    m_ulCount++;

    /*
     * Move the tail to the next insertion point.
     */
    m_ulTailIndex++;
    if (m_ulTailIndex == m_ulGran)
    {
	m_ulTailIndex = 0;
    }
}

BOOL
SBandwidthCalculator::Valid()
{
    return (m_ulCount >= m_ulMinPackets && m_dFirstTime < m_dLastTime);
}

UINT32
SBandwidthCalculator::GetBps()
{
    if (m_dFirstTime >= m_dLastTime)
    {
	return 0;
    }

    UINT32 ulBytesPS =
	(UINT32)((float)m_ulTotalSize / (m_dLastTime - m_dFirstTime));

    /* 
     * 8 bits / byte
     */
    return ulBytesPS * 8;
}

UINT32
SBandwidthCalculator::GetPps()
{
    if (m_dFirstTime >= m_dLastTime)
    {
	return 0;
    }

    UINT32 ulPacketsPS =
	(UINT32)((float)m_ulCount / (m_dLastTime - m_dFirstTime));

    return ulPacketsPS;
}
