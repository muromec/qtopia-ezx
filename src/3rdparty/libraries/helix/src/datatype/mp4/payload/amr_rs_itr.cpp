/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "amr_rs_itr.h"
#include "amr_toc.h"
#include "amr_frame_info.h"

AMRRobustSortingItr::AMRRobustSortingItr() :
    m_bMore(FALSE),
    m_ulFrameIndex(0),
    m_ulOffset(0),
    m_ulFrameCount(0),
    m_pFrameInfo(0)
{}

AMRRobustSortingItr::~AMRRobustSortingItr()
{
    delete [] m_pFrameInfo;
    m_pFrameInfo = 0;
}

HXBOOL AMRRobustSortingItr::Init(AMRFlavor flavor, ULONG32 ulChannels, 
			       ULONG32 ulStartBlock, ULONG32 ulBlockInc,
			       const AMRTOCInfo& tocInfo)
{
    m_bMore = FALSE;
    m_ulFrameCount = tocInfo.EntryCount();
    m_ulFrameIndex = 0;
    m_ulOffset = 0;

    if (m_ulFrameCount)
    {
	delete [] m_pFrameInfo;
	m_pFrameInfo = new FrameInfo[m_ulFrameCount];
	
	ULONG32 ulBlockCount = m_ulFrameCount / ulChannels;

	if (m_pFrameInfo && ulBlockCount)
	{
	    ULONG32 ulFrameIndex = 0;
	    ULONG32 ulBlockIndex = ulStartBlock;
	    
	    for (ULONG32 i = 0; i < ulBlockCount; i++)
	    {
		ULONG32 ulOffset = 0;
		
		for(ULONG32 j = 0; j < ulChannels; ulFrameIndex++, j++)
		{
		    ULONG32 frameType = tocInfo.GetType(ulFrameIndex);
		    ULONG32 ulFrameBits = CAMRFrameInfo::FrameBits(flavor, 
								   frameType);
		    ULONG32 ulFrameBytes = 1 + ((ulFrameBits + 7) >> 3);
		    
		    m_pFrameInfo[ulFrameIndex].Init(ulBlockIndex, ulOffset, 
						    ulFrameBytes);
		    
		    ulOffset += ulFrameBytes;
		}

		ulBlockIndex += ulBlockInc;
	    }
	    m_bMore = TRUE;
	}
    }

    return m_bMore;
}

ULONG32 AMRRobustSortingItr::Block() const
{
    return m_pFrameInfo[m_ulFrameIndex].Block();
}

ULONG32 AMRRobustSortingItr::Offset() const
{
    return m_ulOffset + m_pFrameInfo[m_ulFrameIndex].Offset();
}

HXBOOL AMRRobustSortingItr::More() const
{
    return m_bMore;
}

void AMRRobustSortingItr::Next()
{
    if (m_bMore)
    {
	m_pFrameInfo[m_ulFrameIndex].DecSize();

	ULONG32 ulStartIdx = m_ulFrameIndex;

	IncFrameIndex();
	
	// Find the next valid index
	while((m_ulFrameIndex != ulStartIdx) &&
	      (m_pFrameInfo[m_ulFrameIndex].Size() == 0))
	{
	    IncFrameIndex();
	}
	
	// Determine if we have more to go
	m_bMore = (m_pFrameInfo[m_ulFrameIndex].Size() != 0);
    }
}

void AMRRobustSortingItr::IncFrameIndex()
{
    m_ulFrameIndex++;

    // See if we need to loop
    if (m_ulFrameIndex >= m_ulFrameCount)
    {
	m_ulFrameIndex = 0;

	// Increment the offset since we have looped
	m_ulOffset++;
    }
}

AMRRobustSortingItr::FrameInfo::FrameInfo()
{}

void AMRRobustSortingItr::FrameInfo::Init(ULONG32 ulBlock, 
					  ULONG32 ulOffset, 
					  ULONG32 ulSize)
{
    m_ulBlock = ulBlock;
    m_ulOffset = ulOffset;
    m_ulSize = ulSize;
}

ULONG32 AMRRobustSortingItr::FrameInfo::Block() const
{
    return m_ulBlock;
}

ULONG32 AMRRobustSortingItr::FrameInfo::Offset() const
{
    return m_ulOffset;
}

ULONG32 AMRRobustSortingItr::FrameInfo::Size() const
{
    return m_ulSize;
}

void AMRRobustSortingItr::FrameInfo::DecSize()
{
    m_ulSize--;
}
