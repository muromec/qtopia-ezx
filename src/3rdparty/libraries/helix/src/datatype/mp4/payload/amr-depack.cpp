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
 
#include "hlxclib/string.h"
#include "amr-depack.h"
#include "amr_toc.h"
#include "amr_rs_itr.h"
#include "bitstream.h"


AMRDepack::AMRDepack() :
    m_flavor(NarrowBand),
    m_ulTimestampInc(0),
    m_bOctetAlign(FALSE),
    m_ulModeSet(0),
    m_bHasCRC(FALSE),
    m_ulMaxInterleave(0),
    m_ulPtime(0),
    m_ulChannels(0),
    m_pUserData(0),
    m_pCallback(0),
    m_bPacketsLost(FALSE),
    m_bLastTimeValid(FALSE),
    m_ulLastTimestamp(0),
    m_ulIleavIndex(0),
    m_ulIleavLength(0),
    m_ulIleavBlockCount(0),
    m_ulIleavBaseTime(0)
{}

AMRDepack::~AMRDepack()
{}

HXBOOL AMRDepack::Init(AMRFlavor flavor,
		     HXBOOL    bOctetAlign,
		     ULONG32 ulModeSet,
		     ULONG32 ulChangePeriod,
		     HXBOOL    bChangeNeighbor,
		     ULONG32 ulMaxPtime,
		     HXBOOL    bHasCRC,
		     HXBOOL    bRobustSort,
		     ULONG32 ulMaxInterleave,
		     ULONG32 ulPtime,
		     ULONG32 ulChannels,
		     OnFrameCB pFrameCB,
		     void* pUserData)
{
    HXBOOL ret = FALSE;
    
    if ((pFrameCB) &&
	(ulChannels < 7) &&
	((flavor == NarrowBand) || (flavor == WideBand)))
    {
	m_flavor = flavor;

	if (ulChannels)
	    m_ulChannels = ulChannels;
	else
	    m_ulChannels = 1;
	
	if (ulModeSet)
	    m_ulModeSet = ulModeSet;
	else
	    m_ulModeSet = 0x81ff;
	
	m_ulPtime = ulPtime;

	m_pCallback = pFrameCB;
	m_pUserData = pUserData;
	
	m_blockBuf.Init(flavor, m_ulChannels);

	m_ulIleavIndex = 0;
	m_ulIleavLength = 0;

	if ((bOctetAlign) ||
	    (bHasCRC) ||
	    (bRobustSort) ||
	    (ulMaxInterleave))
	{
	    // Octet aligned mode
	    
	    m_bOctetAlign   = TRUE;
	    m_bHasCRC       = bHasCRC;
	    m_bRobustSort   = bRobustSort;
	    m_ulMaxInterleave = ulMaxInterleave;
	}
	else
	{
	    m_bOctetAlign     = FALSE;
	    m_bHasCRC         = FALSE;
	    m_bRobustSort     = FALSE;
	    m_ulMaxInterleave = 0;
	}

	ret = TRUE;
    }

    return ret;
}

HXBOOL AMRDepack::Reset() // Completely reset the depacketizer state
{
    m_bPacketsLost = FALSE;

    // Clear interleave state info
    m_ulIleavIndex = 0;
    m_ulIleavLength = 0;
    m_ulIleavBaseTime = 0;

    m_bLastTimeValid = FALSE;
    m_ulLastTimestamp = 0;

    return TRUE;
}

HXBOOL AMRDepack::Flush() // Indicates end of stream
{
    if (m_ulMaxInterleave > 0)
    {
	DispatchBlocks(m_ulIleavBaseTime);
    }

    return TRUE;
}

HXBOOL AMRDepack::OnPacket(ULONG32 ulTime, 
			 const UINT8* pData, ULONG32 ulSize, 
			 HXBOOL bMarker)
{
    HXBOOL bFailed = FALSE;

    Bitstream bs;

    bs.SetBuffer(pData);

    // Skip the CMR since we don't care about it
    if (m_bOctetAlign)
    {
	// 3GPP TS26.235 v5.0.0 Annex B Section B.4.4.1
	bs.GetBits(8);
    }
    else
    {
	// 3GPP TS26.235 v5.0.0 Annex B Section B.4.3.1
	bs.GetBits(4);
    }

    if (m_ulMaxInterleave > 0)
    {
	// 3GPP TS26.235 v5.0.0 Annex B Section B.4.4.1
	ULONG32 ulILL = bs.GetBits(4);
	ULONG32 ulILP = bs.GetBits(4);

	bFailed = !UpdateIleavInfo(ulILL, ulILP, ulTime);
    }
    
    if (bFailed == FALSE)
    {
	AMRTOCInfo tocInfo;

	GetTOCInfo(bs, tocInfo);

	if (m_bHasCRC)
	{
	    // 3GPP TS26.235 v5.0.0 Annex B Section B.4.4.2.1
	    SkipCRCInfo(bs, tocInfo);
	}

	// Update the block count to make sure we have
	// enough space in m_blockBuf for this data
	UpdateBlockCount(tocInfo.EntryCount() / m_ulChannels);


	ULONG32 ulStartBlock = 0;
	ULONG32 ulBlockInc = 1;

	// Adjust the ulStartBlock and ulBlockInc if the
	// data is interleaved
	if (m_ulMaxInterleave > 0)
	{
	    ulStartBlock = m_ulIleavIndex;
	    ulBlockInc = m_ulIleavLength;
	}

	if (m_bRobustSort)
	{
	    // Copy robust sorted frame data to m_blockBuf
	    SortedCopy(bs, ulStartBlock, ulBlockInc, tocInfo);
	}
	else
	{
	    // Copy linear frame data to m_blockBuf
	    LinearCopy(bs, ulStartBlock, ulBlockInc, tocInfo);
	}

	if (m_ulMaxInterleave > 0)
	{
	    // We are deinterleaving.
	    // Increment the interleave index now
	    // that we are done with this packet.
	    m_ulIleavIndex++;

	    // See if we have all the packets in this
	    // interleave block
	    if (m_ulIleavIndex == m_ulIleavLength)
	    {
		// Yep. Dispatch the frame blocks
		DispatchBlocks(m_ulIleavBaseTime);
	    }
	}
	else
	{
	    // This data is not interleaved so
	    // dispatch it now
	    DispatchBlocks(ulTime);
	}
    }

    return !bFailed;
}

HXBOOL AMRDepack::OnLoss(ULONG32 ulNumPackets) // called to indicate lost packets
{
    if (m_ulMaxInterleave > 0)
    {
	ULONG32 delta = m_ulIleavLength - m_ulIleavIndex;

	if (ulNumPackets < delta)
	{
	    // We have lost only a few blocks in the middle
	    m_ulIleavIndex += ulNumPackets;
	}
	else
	{
	    // We have lost all the remaining blocks
	    // in the current interleave block.
	    // Dispatch what we have
	    DispatchBlocks(m_ulIleavBaseTime);
	}
    }
    else
	m_bPacketsLost = TRUE;

    return TRUE;
}

void AMRDepack::SetTSSampleRate(ULONG32 ulTSSampleRate)
{
    // Determine the number of timestamp units to increment
    // per frame.
    m_ulTimestampInc = (CAMRFrameInfo::FrameDuration() * ulTSSampleRate) / 1000;
}

HXBOOL AMRDepack::UpdateIleavInfo(ULONG32 ulILL, ULONG32 ulILP, ULONG32 ulTime)
{
    HXBOOL bFailed = FALSE;

    if ((ulILP > ulILL) ||
	((m_ulIleavLength != 0) &&
	 (ulILP != m_ulIleavIndex)))
    {
	bFailed = TRUE;
    }
    else if (ulILP == 0)
    {
	// This is the expected case
	
	m_ulIleavIndex = 0;
	m_ulIleavLength = ulILL + 1;
	
	m_ulIleavBaseTime = ulTime;
    }
    else if (m_ulIleavLength == 0)
    {
	// This case occurs when loss happens
	
	m_ulIleavIndex = ulILP;
	m_ulIleavLength = ulILL + 1;
	
	// We need the base time to be valid even if we
	// do not start at the beginning of a superblock
	// Each packet starts 1 frame later than
	// the previous one.
	m_ulIleavBaseTime = ulTime - (m_ulTimestampInc) * m_ulIleavIndex;
    }

    return !bFailed;
}

void AMRDepack::GetTOCInfo(Bitstream& bs, AMRTOCInfo& tocInfo)
{
    ULONG32 ulTocBits;
    ULONG32 ulTocMask;
    
    ULONG32 ulShift;

    if (m_bOctetAlign)
    {
	// This header is defined in
	// 3GPP TS26.235 v5.0.0 Annex B Section B.4.4.2
	//   7   6   5   4   3   2   1   0
	// +---+---------------+---+---+---+
	// | F |       FT      | Q | P | P |
	// +---+---------------+---+---+---+
	//
	// F  = more frames flag
	// FT = frame type
	// Q  = quality indicator
	// P  = padding

	ulTocBits = 8;
	ulTocMask = 0x80;
	ulShift = 2;
    }
    else
    {
	// This header is defined in
	// 3GPP TS26.235 v5.0.0 Annex B Section B.4.3.2
	//   5   4   3   2   1   0
	// +---+---------------+---+
	// | F |       FT      | Q |
	// +---+---------------+---+
	//
	// F  = more frames flag
	// FT = frame type
	// Q  = quality indicator
	ulTocBits = 6;
	ulTocMask = 0x20;
	ulShift = 0;
    }
    
    HXBOOL bDone = FALSE;

    while(!bDone)
    {
	ULONG32 entry = bs.GetBits(ulTocBits);
	UINT8 type = (UINT8)(entry >> (ulShift + 1)) & 0x0f;
	UINT8 quality = (UINT8)(entry >> ulShift) & 0x01;
	
	tocInfo.AddInfo(type, quality);
	
	if ((entry & ulTocMask) == 0)
	    bDone = TRUE;
    }    
}

void AMRDepack::SkipCRCInfo(Bitstream& bs, const AMRTOCInfo& tocInfo)
{
    // Skip CRCs if they are present since we don't
    // care about them.
    for (ULONG32 i = 0; i < tocInfo.EntryCount(); i++)
    {
	if (tocInfo.GetType(i)  < 14)
	    bs.GetBits(8);
    }
}


void AMRDepack::UpdateBlockCount(ULONG32 ulBlockCount)
{
    if (m_ulMaxInterleave > 0)
    {
	if (m_ulIleavIndex == 0)
	{
	    m_ulIleavBlockCount = ulBlockCount;
	    
	    // Make sure we have enough block space in the
	    // buffer
	    m_blockBuf.SetBlockCount(m_ulIleavBlockCount * m_ulIleavLength);
	}
    }
    else
    {
	// Make sure we have enough block space in the
	// buffer for this packet
	m_blockBuf.SetBlockCount(ulBlockCount);
    }
}

void AMRDepack::LinearCopy(Bitstream& bs, 
			   ULONG32 ulStartBlock, ULONG32 ulBlockInc,
			   const AMRTOCInfo& tocInfo)
{
    ULONG32 ulBlockIndex = ulStartBlock;
    ULONG32 ulFrameIndex = 0;
    
    for (ULONG32 j = 0; j < m_blockBuf.GetBlockCount(); j++)
    {
	UINT8* pStart = m_blockBuf.GetBlockBuf(ulBlockIndex);
	
	ULONG32 ulFrameSize = GetFrameBlock(bs,
					    pStart, 
					    tocInfo, ulFrameIndex,
					    m_ulChannels);
	
	if (ulFrameSize)
	{
	    // Store frame size information
	    m_blockBuf.SetBlockSize(ulBlockIndex, ulFrameSize);
	    
	    // Update the block index
	    ulBlockIndex += ulBlockInc;
	    
	    // Update the frame index. Each frame block
	    // consumes m_ulChannels frames
	    ulFrameIndex += m_ulChannels;
	}
	else
	    break;
    }
}

ULONG32 AMRDepack::GetFrameBlock(Bitstream& bs,
				 UINT8* pStart, 
				 const AMRTOCInfo& tocInfo,
				 ULONG32 ulStartEntry,
				 ULONG32 ulChannels)
{
    ULONG32 ulRet = 0;

    ULONG32 ulEnd = ulChannels + ulStartEntry;

    if (ulEnd <= tocInfo.EntryCount())
    {
	HXBOOL bFailed = FALSE;
	UINT8* pCurrent = pStart;

	for (ULONG32 i = ulStartEntry; i < ulEnd; i++)
	{
	    ULONG32 ulFrameType = tocInfo.GetType(i);
	    
	    if ((ulFrameType < 9) || (ulFrameType == 15))
	    {
		ULONG32 ulFrameBits = GetFrameBits(ulFrameType);
		ULONG32 ulFrameBytes = (ulFrameBits + 7) >> 3;
		
		if (m_bOctetAlign)
		{
		    // Octet Aligned contains padding already
		    ulFrameBits = ulFrameBytes << 3;
		}

		// Create frame header.
		// This header is defined in
                // 3GPP TS26.235 v5.0.0 Annex B Section B.5.3
                //   7   6   5   4   3   2   1   0
                // +---+---------------+---+---+---+
                // | P |       FT      | Q | P | P |
                // +---+---------------+---+---+---+
                //
                // P  = padding
                // FT = frame type
                // Q  = quality indicator
		*pCurrent++ = (UINT8)((ulFrameType << 3) | 
				      (tocInfo.GetQuality(i) << 2));

		if (ulFrameBytes > 0)
		{
		    // Set last byte of frame to 0 for padding
		    pCurrent[ulFrameBytes - 1] = 0;
		    
		    // Copy frame bits into the buffer
		    bs.GetBits(ulFrameBits, pCurrent);

		    // Move current pointer to just past this frame
		    pCurrent += ulFrameBytes;
		}
	    }
	    else
	    {
		bFailed = TRUE;
		break;
	    }
	}

	if (bFailed == FALSE)
	    ulRet = pCurrent - pStart;
    }

    return ulRet;
}

void AMRDepack::SortedCopy(Bitstream& bs, 
			   ULONG32 ulStartBlock, ULONG32 ulBlockInc,
			   const AMRTOCInfo& tocInfo)
{
    // This function copies robust sorted data from the packet.
    // Unfortunately this is not very efficient because the bytes
    // of each frame are interleaved with eachother. The
    // AMRRobustSortingItr class provides us with the block number
    // and block byte offset for each byte in the sorted data.

    AMRRobustSortingItr itr;

    itr.Init(m_flavor, m_ulChannels, ulStartBlock, ulBlockInc, tocInfo);

    for (;itr.More(); itr.Next())
    {
	UINT8* pDest = m_blockBuf.GetBlockBuf(itr.Block()) + itr.Offset();

	// Copy the byte
	*pDest = (UINT8)bs.GetBits(8);
    }
}

void AMRDepack::DispatchFrameBlock(ULONG32 ulTimestamp,
				   const UINT8* pFrame, ULONG32 ulFrameSize)
{
    // We can get duplicate frames so we need to make
    // a check to see if we have already sent this frame
    if (((m_bLastTimeValid == FALSE) ||
	 (ulTimestamp > m_ulLastTimestamp)) &&
	(m_pCallback))
    {
	m_pCallback(m_pUserData,
		    ulTimestamp,
		    pFrame, ulFrameSize,
		    m_bPacketsLost);
	
	// Update last timestamp
	m_ulLastTimestamp = ulTimestamp;
	m_bLastTimeValid = TRUE;
    }
    
    // Clear packet flag
    m_bPacketsLost = FALSE;
}

void AMRDepack::DispatchBlocks(ULONG32 ulTimestamp)
{
    for (ULONG32 i = 0; i < m_blockBuf.GetBlockCount(); i++)
    {
	ULONG32 ulFrameSize = m_blockBuf.GetBlockSize(i);
	
	if (ulFrameSize)
	{
	    DispatchFrameBlock(ulTimestamp, 
			       m_blockBuf.GetBlockBuf(i), ulFrameSize);
	}
	else
	{
	    // Signal that loss has occured
	    m_bPacketsLost = TRUE;
	}

	// Increment the timestamp
	ulTimestamp += m_ulTimestampInc;
    }
    
    // Clear interleave state info
    m_ulIleavIndex = 0;
    m_ulIleavLength = 0;
}
