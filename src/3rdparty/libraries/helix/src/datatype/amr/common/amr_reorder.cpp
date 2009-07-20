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

#include "amr_reorder.h"

#include "hxassert.h"
#include "amr_frame_info.h"

#include "./reorder.tab"


const UINT16* const CAMRBitReorder::zm_pToCodecTbl[2][8] = {
    {z_pNBMode0DecTbl,  // AMR-NB
     z_pNBMode1DecTbl,
     z_pNBMode2DecTbl,
     z_pNBMode3DecTbl,
     z_pNBMode4DecTbl,
     z_pNBMode5DecTbl,
     z_pNBMode6DecTbl,
     z_pNBMode7DecTbl},
    {0, 0, 0, 0, 0, 0, 0, 0} // AMR-WB
};

CAMRBitReorder::CAMRBitReorder(AMRFlavor flavor) :
    m_pWorkspace(0),
    m_flavor(flavor)
{
    ULONG32 workspaceBytes = (CAMRFrameInfo::MaxFrameBits(m_flavor) + 7) >> 3;

    m_pWorkspace = new UINT8[workspaceBytes];

    for (ULONG32 i = 0; i < 2; i++)
	for (ULONG32 j = 0; j < 8; j++)
	    m_pToNetworkTbl[i][j] = 0;
}

CAMRBitReorder::~CAMRBitReorder()
{
    // Remove ToNetwork entries if they were created
    for (ULONG32 i = 0; i < 2; i++)
    {
	for (ULONG32 j = 0; j < 8; j++)
	{
	    delete [] m_pToNetworkTbl[i][j];
	    m_pToNetworkTbl[i][j] = 0;
	}
    }

    delete [] m_pWorkspace;
    m_pWorkspace = 0;
}

void CAMRBitReorder::ToCodec(ULONG32 frameType, UINT8* pInOut)
{
    HX_ASSERT(frameType < 8);
    HX_ASSERT(zm_pToCodecTbl[frameType]);

    ReorderBits(CAMRFrameInfo::FrameBits(m_flavor, frameType),
		zm_pToCodecTbl[m_flavor][frameType], 
		pInOut);
}

void CAMRBitReorder::ToNetwork(ULONG32 frameType, UINT8* pInOut)
{
    HX_ASSERT(frameType < 8);
    
    if (!m_pToNetworkTbl[frameType])
	GenerateTable(frameType);

    ReorderBits(CAMRFrameInfo::FrameBits(m_flavor, frameType),
		m_pToNetworkTbl[m_flavor][frameType], 
		pInOut);
}

void CAMRBitReorder::ReorderBits(ULONG32 bitCount, const UINT16* pTbl, 
				 UINT8* pInOut)
{    
    ULONG32 byteCount = (bitCount + 7) >> 3;
    const UINT8* pIn = pInOut;

    while (bitCount)
    {
	UINT8 ch = *pIn++;
	ULONG32 i = 8;

	while (bitCount && i)
	{
	    ULONG32 outBit = *pTbl++;

	    ULONG32 outByte = outBit >> 3;
	    
	    ULONG32 outCh = 1 << (7 - (outBit & 0x7));

	    if (ch & 0x80)
		m_pWorkspace[outByte] |= outCh;
	    else
		m_pWorkspace[outByte] &= ~outCh;

	    ch <<= 1;
	    i--;
	    bitCount--;
	}
    }

    ::memcpy(pInOut, m_pWorkspace, byteCount); /* Flawfinder: ignore */
}

// Generate the "to network" table which is the inverse of the 
// "to codec" table.
void CAMRBitReorder::GenerateTable(ULONG32 frameType)
{
    HX_ASSERT(zm_pToCodecTbl[frameType]);
    HX_ASSERT(!m_pToNetworkTbl[frameType]);

    ULONG32 bitCount = CAMRFrameInfo::FrameBits(m_flavor, frameType);
    
    const UINT16* pSrcTbl = zm_pToCodecTbl[m_flavor][frameType];
    UINT16* pDestTbl = new UINT16[bitCount];

    for (ULONG32 i = 0; i < bitCount; i++)
	pDestTbl[pSrcTbl[i]] = (UINT16)i;

    m_pToNetworkTbl[m_flavor][frameType] = pDestTbl;
}
