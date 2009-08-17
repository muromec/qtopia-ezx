/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bitstream.cpp,v 1.6 2006/01/04 07:12:44 pankajgupta Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#include "hxassert.h"
#include "bitstream.h"

static const ULONG32 z_mask[] = { 0x00000000, 0x00000001,
			    0x00000003, 0x00000007,
			    0x0000000f, 0x0000001f,
			    0x0000003f, 0x0000007f,
			    0x000000ff, 0x000001ff,
			    0x000003ff, 0x000007ff,
			    0x00000fff, 0x00001fff,
			    0x00003fff, 0x00007fff,
			    0x0000ffff, 0x0001ffff,
			    0x0003ffff, 0x0007ffff,
			    0x000fffff, 0x001fffff,
			    0x003fffff, 0x007fffff,
			    0x00ffffff, 0x01ffffff,
			    0x03ffffff, 0x07ffffff,
			    0x0fffffff, 0x1fffffff,
			    0x3fffffff, 0x7fffffff,
			    0xffffffff};
Bitstream::Bitstream() :
    m_pBuf(0),
    m_pCur(0),
    m_bitBuf(0),
    m_bitCount(0),
    m_ulBufSize(0)
{}

void Bitstream::SetBuffer(const UINT8* pBuf)
{
    m_pBuf = pBuf;
    m_pCur = m_pBuf;
}

const UINT8* Bitstream::GetBuffer(void)
{
    return m_pBuf;
}

void Bitstream::SetBufSize(ULONG32 ulBufSize)
{
    m_ulBufSize = ulBufSize;
}

ULONG32 Bitstream::GetBufSize(void)
{
    return m_ulBufSize;
}


ULONG32 Bitstream::GetBits(ULONG32 bitCount)
{
    HX_ASSERT(bitCount <= 32);

    ULONG32 ret = PeekBits(bitCount);

    if (bitCount <= m_bitCount)
	m_bitCount -= bitCount;
    else
    {	
	m_bitBuf = *m_pCur++;
	m_bitCount = 8 - (bitCount - m_bitCount);
    }

    return ret;
}

void Bitstream::GetBits(ULONG32 bitCount, UINT8* pOut)
{
    for(;bitCount >= 8; bitCount -= 8)
	*pOut++ = (UINT8)GetBits(8);

    if (bitCount)
	*pOut = (UINT8)(GetBits(bitCount) << (8 - bitCount));
}

ULONG32 Bitstream::PeekBits(ULONG32 bitCount)
{
    HX_ASSERT(m_pCur);
    HX_ASSERT(bitCount <= 32);

    ULONG32 ret = 0;

    while (bitCount > m_bitCount && m_bitCount < 24)
    {
	m_bitBuf = (m_bitBuf << 8) | (ULONG32) *m_pCur++;
	m_bitCount += 8;
    }

    if (bitCount <= m_bitCount)
    {
	ULONG32 shift = m_bitCount - bitCount;
    
	ret = (m_bitBuf >> shift) & z_mask[bitCount];
    }
    else
    {	
	ULONG32 needed = bitCount - m_bitCount;

	HX_ASSERT(needed <= 8);

	ret = (m_bitBuf & z_mask[m_bitCount]) << needed;
	
	ULONG32 shift = 8 - needed;
	ret |= (*m_pCur >> shift) & z_mask[needed];
    }

    return ret;
}

void Bitstream::FlushBits(ULONG32 bitCount)
{
    for(;bitCount >= 8; bitCount -= 8)
	GetBits(8);

    GetBits(bitCount);
}
