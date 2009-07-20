/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbitset.h,v 1.5 2007/07/06 20:51:33 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _HXBITSET_H_
#define _HXBITSET_H_

/*
 * Freely adapted from the g++ BitSet library...
 */

typedef UINT32 _BS_word;

#define _BS_CHAR_BIT 8
#define _BS_BITS_PER_WORD (_BS_CHAR_BIT*sizeof(_BS_word))
#define _BS_WORDS_NEEDED(NBITS) ((NBITS+_BS_BITS_PER_WORD-1)/_BS_BITS_PER_WORD)

/* number the bits in a _BS_word in little-endian order */
#define _BS_BIGENDIAN 0

#if _BS_BIGENDIAN
#define _BS_LEFT >>
#define _BS_RIGHT <<
#else
#define _BS_LEFT <<
#define _BS_RIGHT >>
#endif

#if _BS_BIGENDIAN
#define _BS_BITMASK(BITNO) ((_BS_word)1 << (_BS_BITS_PER_WORD - 1 - (BITNO)))
#else
#define _BS_BITMASK(BITNO) ((_BS_word)1 << (BITNO))
#endif

#define _BS_INDEX(I) ((unsigned)(I) / _BS_BITS_PER_WORD)
#define _BS_POS(I) ((I) & (_BS_BITS_PER_WORD - 1))

#define _BS_SHORT_LEN 8


class CHXBitset
{
public:
    CHXBitset();
    CHXBitset(INT32 maxBit);
    CHXBitset(BYTE* pBitmap, INT32 nCount);
    ~CHXBitset();

    void set(INT32 pos);
    void set(INT32 from, INT32 to);
    void set();			// set all bits
    void clear(INT32 pos);
    void clear(INT32 from, INT32 to);
    void clear();		// clear all bits
    HXBOOL test(INT32 pos);
    INT32 toByteArray(BYTE** pBitmap);
    INT32 getSize();
    INT32 getNumBits();

    HXBOOL test(INT32 from, INT32 to);

    void growsize(INT32 maxBit);

private:
    void setBitsetSize(INT32 nBitsetSize);

    _BS_word*	m_pBitset;
    INT32	m_nBitsetSize;
    _BS_word    m_pShortBitset[_BS_SHORT_LEN];
};

inline INT32
CHXBitset::getSize()
{
    // returns size of bitset in bytes
    return (INT32)(m_nBitsetSize * sizeof(_BS_word));
}

inline INT32
CHXBitset::getNumBits()
{
    return (INT32)(m_nBitsetSize * _BS_BITS_PER_WORD);
}

#endif	/* _HXBITSET_H_ */
