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
#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "amr_block_buf.h"
#include "amr_frame_info.h"

AMRBlockBuffer::AMRBlockBuffer() :
    m_ulBlockCount(0),
    m_ulBlockSize(0),
    m_ulMaxBlocks(0),
    m_pBlockBuf(0),
    m_pBlockSizes(0)
{}

AMRBlockBuffer::~AMRBlockBuffer()
{
    delete [] m_pBlockBuf;
    m_pBlockBuf = 0;

    delete [] m_pBlockSizes;
    m_pBlockSizes = 0;
}

void AMRBlockBuffer::Init(AMRFlavor flavor, ULONG32 ulChannels)
{
    m_ulBlockSize = 1 + ((CAMRFrameInfo::MaxFrameBits(flavor) + 7) >> 3);

    m_ulBlockCount = 0;
}

void AMRBlockBuffer::SetBlockSize(ULONG32 ulBlockIndex, ULONG32 ulBlockSize)
{
    if (ulBlockIndex < m_ulBlockCount)
	m_pBlockSizes[ulBlockIndex] = ulBlockSize;
}

ULONG32 AMRBlockBuffer::GetBlockSize(ULONG32 ulBlockIndex) const
{
    return (ulBlockIndex < m_ulBlockCount) ? m_pBlockSizes[ulBlockIndex] : 0;
}

UINT8* AMRBlockBuffer::GetBlockBuf(ULONG32 ulBlockIndex) const
{
    UINT8* pRet = 0;

    if (m_pBlockBuf && (ulBlockIndex < m_ulBlockCount))
    {
	pRet = m_pBlockBuf + ulBlockIndex * m_ulBlockSize;
    }

    return pRet;
}

void AMRBlockBuffer::SetBlockCount(ULONG32 ulBlockCount)
{
    if (ulBlockCount > m_ulMaxBlocks)
    {
	Resize(ulBlockCount);
    }

    m_ulBlockCount = ulBlockCount;

    // Clear sizes
    ::memset(m_pBlockSizes, 0, sizeof(ULONG32) * m_ulMaxBlocks);
}

ULONG32 AMRBlockBuffer::GetBlockCount() const
{
    return m_ulBlockCount;
}

void AMRBlockBuffer::Resize(ULONG32 ulBlockCount)
{
    m_ulMaxBlocks = ulBlockCount;
    
    delete [] m_pBlockBuf;
    m_pBlockBuf = new UINT8[m_ulMaxBlocks * m_ulBlockSize];
    
    delete [] m_pBlockSizes;
    m_pBlockSizes = new ULONG32[m_ulMaxBlocks];
    ::memset(m_pBlockSizes, 0, sizeof(ULONG32) * m_ulMaxBlocks);
}
