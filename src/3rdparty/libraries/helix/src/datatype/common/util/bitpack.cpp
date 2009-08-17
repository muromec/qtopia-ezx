/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bitpack.cpp,v 1.4 2004/07/09 18:31:05 hubbe Exp $
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

#include "bitpack.h"

#include "hlxclib/string.h"
#include "hxassert.h"
#include "bitstream.h"

static const UINT32 z_mask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f,
				 0x1f, 0x3f, 0x7f, 0xff};
BitPacker::BitPacker(UINT8* pBuf, ULONG32 ulBufSize) :
    m_pBuf(pBuf),
    m_ulBufSize(ulBufSize),
    m_pCur(pBuf),
    m_bitsLeft(8)
{
    ::memset(m_pCur, 0, ulBufSize);
}

void BitPacker::PackBits(UINT32 bits, ULONG32 bitCount)
{
    HX_ASSERT(bitCount <= 32);

    if (bitCount > m_bitsLeft)
    {
	// Finish up this byte
	bitCount -= m_bitsLeft;
	*m_pCur++ |= (UINT8)((bits >> bitCount) & z_mask[m_bitsLeft]);

	m_bitsLeft = 8;

	while(bitCount >= 8)
	{
	    bitCount -= 8;
	    *m_pCur++ = (UINT8)((bits >> bitCount) & 0xff);
	}
    }

    if (bitCount)
    {
    	m_bitsLeft -= bitCount;
	*m_pCur |= (bits & z_mask[bitCount]) << m_bitsLeft;
	
	if (m_bitsLeft == 0)
	{
	    m_bitsLeft = 8;
	    m_pCur++;
	}
    }
}

void BitPacker::PackBits(const UINT8* pBuf, ULONG32 bitCount,
			 ULONG32 ulStartOffset)
{
    // Reference implementation
    Bitstream bs;
    bs.SetBuffer(pBuf);

    bs.GetBits(ulStartOffset);

    while(bitCount)
    {
	int bits = (bitCount > 8) ? 8 : bitCount;
	PackBits(bs.GetBits(bits), bits);
	bitCount -= bits;
    }
}

UINT32 BitPacker::ByteAlign()
{
  UINT32 bits = m_bitsLeft & 7 ;
  PackBits(0, bits) ;
  return bits ;
}

ULONG32 BitPacker::BytesUsed() const
{
    return (m_pCur - m_pBuf);
}
