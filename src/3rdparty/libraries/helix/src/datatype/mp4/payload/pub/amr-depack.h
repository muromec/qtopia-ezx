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
 
#ifndef AMR_DEPACK_H
#define AMR_DEPACK_H

#include "hxtypes.h"
#include "amr_flavor.h"
#include "amr_frame_info.h"
#include "amr_block_buf.h"

class Bitstream;
class AMRTOCInfo;

typedef void (*OnFrameCB)(void* pUserData, 
			  ULONG32 ulTime,
			  const UINT8* pData, ULONG32 ulSize,
			  HXBOOL bPreviousLoss);

class AMRDepack
{
public:
    AMRDepack();
    ~AMRDepack();

    HXBOOL Init(AMRFlavor flavor,
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
	      void* pUserData);

    HXBOOL Reset(); // Completely reset the depacketizer state

    HXBOOL Flush(); // Indicates end of stream

    HXBOOL OnPacket(ULONG32 ulTime, 
		  const UINT8* pData, ULONG32 ulSize, 
		  HXBOOL bMarker);

    HXBOOL OnLoss(ULONG32 ulNumPackets); // called to indicate lost packets

    void SetTSSampleRate(ULONG32 ulTSSampleRate);

protected:
    HXBOOL UpdateIleavInfo(ULONG32 ulILL, ULONG32 ulILP, ULONG32 ulTime);
    HXBOOL GetTOCInfo(Bitstream& bs, AMRTOCInfo& tocInfo);
    HXBOOL SkipCRCInfo(Bitstream& bs, const AMRTOCInfo& tocInfo);

    void UpdateBlockCount(ULONG32 ulBlockCount);

    // Copies linear frame data to m_blockBuf
    HXBOOL LinearCopy(Bitstream& bs, ULONG32 ulStartBlock, ULONG32 ulBlockInc,
		    const AMRTOCInfo& tocInfo);
    HXBOOL GetFrameBlock(Bitstream& bs,
			  UINT8* pStart, 
			  const AMRTOCInfo& tocInfo,
			  ULONG32 ulStartEntry,
			  ULONG32 ulChannels,
			  ULONG32& ulRet);

    // Copies robust sorted frame data to m_blockBuf
    HXBOOL SortedCopy(Bitstream& bs, ULONG32 ulStartBlock, ULONG32 ulBlockInc,
		    const AMRTOCInfo& tocInfo);

    void DispatchBlocks(ULONG32 ulTimestamp);
    void DispatchFrameBlock(ULONG32 ulTimestamp,
			    const UINT8* pFrame, ULONG32 ulFrameSize);
    
    ULONG32 GetFrameBits(UINT32 frameType) const;
    ULONG32 GetMaxFrameBits() const;

private:
    AMRFlavor m_flavor;
    ULONG32   m_ulTimestampInc;
    HXBOOL      m_bOctetAlign;
    ULONG32   m_ulModeSet;
    HXBOOL      m_bHasCRC;
    HXBOOL      m_bRobustSort;
    ULONG32   m_ulMaxInterleave;
    ULONG32   m_ulPtime;
    ULONG32   m_ulChannels;

    void*     m_pUserData;
    OnFrameCB m_pCallback;

    HXBOOL      m_bPacketsLost;
    HXBOOL      m_bLastTimeValid;
    ULONG32   m_ulLastTimestamp;

    ULONG32   m_ulIleavIndex;
    ULONG32   m_ulIleavLength;
    ULONG32   m_ulIleavBlockCount;
    ULONG32   m_ulIleavBaseTime;

    AMRBlockBuffer m_blockBuf;
};

inline
ULONG32 AMRDepack::GetFrameBits(UINT32 frameType) const
{
    return CAMRFrameInfo::FrameBits(m_flavor, frameType);
}

inline
ULONG32 AMRDepack::GetMaxFrameBits() const
{
    return CAMRFrameInfo::MaxFrameBits(m_flavor);
}

#endif // AMR_DEPACK_H
