/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbitset.cpp,v 1.12 2007/07/06 20:51:32 jfinnecy Exp $
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

#include "hlxclib/string.h"
#include "debug.h"
#include "hxassert.h"
#include "hxtypes.h"
#include "hxbitset.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXBitset::CHXBitset():
    m_pBitset(m_pShortBitset),
    m_nBitsetSize(_BS_SHORT_LEN)
{
    memset(m_pShortBitset, 0, HX_SAFESIZE_T(sizeof(_BS_word) * _BS_SHORT_LEN));
}

CHXBitset::CHXBitset(INT32 maxBit):
    m_pBitset(m_pShortBitset),
    m_nBitsetSize(0)
{
    HX_ASSERT(maxBit > 0);
    setBitsetSize((INT32)_BS_WORDS_NEEDED(maxBit));
    clear();
}

CHXBitset::CHXBitset(BYTE* pBitmap, INT32 nCount):
    m_pBitset(m_pShortBitset),
    m_nBitsetSize(0)
{
    if (nCount <= 0)
    {
        return;
    }

    setBitsetSize((nCount / 4) + 1); //setBitsetSize also clears the bitset

    INT32 i, j;
    for(i=0, j=0; i<m_nBitsetSize; ++i, j+=4)
    {
	if(nCount - j > 3)
	{
	    m_pBitset[i] = (UINT32)(pBitmap[j+3] _BS_LEFT 24 |
			   pBitmap[j+2] _BS_LEFT 16 |
			   pBitmap[j+1] _BS_LEFT 8 |
			   pBitmap[j]);
	}
	else if(nCount - j > 2)
	{
	    m_pBitset[i] = (UINT32)(pBitmap[j+2] _BS_LEFT 24 |
			   pBitmap[j+1] _BS_LEFT 16 |
			   pBitmap[j] _BS_LEFT 8);
	}
	else if(nCount - j > 1)
	{
	    m_pBitset[i] = (UINT32)(pBitmap[j+1] _BS_LEFT 24 |
			   pBitmap[j] _BS_LEFT 16);
	}
	else if(nCount - j > 0)
	{
	    m_pBitset[i] = (UINT32)(pBitmap[j] _BS_LEFT 24);
	}
    }
}

CHXBitset::~CHXBitset()
{
    if (m_pBitset != m_pShortBitset)
    {
        delete[] m_pBitset;
    }

}

void
CHXBitset::set(INT32 pos)
{
    HX_ASSERT(pos >= 0);

    setBitsetSize((INT32)_BS_WORDS_NEEDED(pos+1));
    INT32 idx = (INT32)_BS_INDEX(pos);
    m_pBitset[idx] |= _BS_BITMASK(_BS_POS(pos));
}

void
CHXBitset::set(INT32 from, INT32 to)
{
    HX_ASSERT(from >= 0);

    for(INT32 i=from; i<to; ++i)
    {
	set(i);
    }
}

void
CHXBitset::set()
{
    memset(m_pBitset, 0xff, HX_SAFESIZE_T(sizeof(_BS_word)*(m_nBitsetSize)));
}

void
CHXBitset::clear(INT32 pos)
{
    HX_ASSERT(pos >= 0);

    INT32 idx = (INT32)_BS_INDEX(pos);
    HX_ASSERT(idx < m_nBitsetSize);
    m_pBitset[idx] &= ~(_BS_BITMASK(_BS_POS(pos)));
}

void
CHXBitset::clear(INT32 from, INT32 to)
{
    HX_ASSERT(from >= 0);

    for(INT32 i=from; i<to; ++i)
    {
	clear(i);
    }
}

void
CHXBitset::clear()
{
    if (m_nBitsetSize == 0)
	return;

    memset(m_pBitset, 0, HX_SAFESIZE_T(sizeof(_BS_word)*m_nBitsetSize));
}

HXBOOL
CHXBitset::test(INT32 pos)
{
    HX_ASSERT(pos >= 0);

    INT32 idx = (INT32)_BS_INDEX(pos);

    HX_ASSERT(idx < m_nBitsetSize);
    if (!(idx < m_nBitsetSize))
    {
	// changed %d to %ld for long int (WIN16 condsideration)
	// as all values are INT32
	DPRINTF(D_INFO, ("test failure %ld %ld %ld\n", pos, idx, m_nBitsetSize));
    }

    return (m_pBitset[idx] & _BS_BITMASK(_BS_POS(pos))) != 0;
}

INT32
CHXBitset::toByteArray(BYTE** pBitmap)
{
    INT32 nCount = m_nBitsetSize * 4;

    if (nCount == 0)
    {
        return 0;
    }
    *pBitmap = new BYTE[nCount];
    if( !*pBitmap )
    {
        return 0;
    }

    INT32 i,j;
    for(i=0, j=0; i<m_nBitsetSize; ++i, j+=4)
    {
	(*pBitmap)[j+3] = (BYTE)((m_pBitset[i] _BS_RIGHT 24) & 0xffff);
	(*pBitmap)[j+2] = (BYTE)((m_pBitset[i] _BS_RIGHT 16) & 0xffff);
	(*pBitmap)[j+1] = (BYTE)((m_pBitset[i] _BS_RIGHT 8) & 0xffff);
	(*pBitmap)[j] = (BYTE)(m_pBitset[i] & 0xffff);
    }
    return nCount;
}

HXBOOL
CHXBitset::test(INT32 from, INT32 to)
{
    HX_ASSERT(from >= 0);

    for(INT32 i=from; i<to; ++i)
    {
	if (!test(i))
	    return(FALSE);
    }

    return(TRUE);
}


void
CHXBitset::growsize(INT32 maxBit)
{
    setBitsetSize((INT32)_BS_WORDS_NEEDED(maxBit+1));
}


void
CHXBitset::setBitsetSize(INT32 nBitsetSize)
{

    if (nBitsetSize > m_nBitsetSize)
    {
        if (nBitsetSize > _BS_SHORT_LEN)
        {
            _BS_word* pTempBitset = new _BS_word[nBitsetSize];
            if( !pTempBitset )
            {
                return;
            }

            memcpy(pTempBitset, m_pBitset, (size_t)m_nBitsetSize  * sizeof(_BS_word)); /* Flawfinder: ignore */
            memset(&(pTempBitset[m_nBitsetSize]), 0,
                   HX_SAFESIZE_T(sizeof(_BS_word) * (nBitsetSize - m_nBitsetSize)));
            if (m_pBitset != m_pShortBitset)
            {
                delete[] m_pBitset;
            }
            m_pBitset = pTempBitset;
            m_nBitsetSize = nBitsetSize;
        }
        else
        {
            if (m_nBitsetSize == 0)
            {
                memset(m_pShortBitset, 0, HX_SAFESIZE_T(sizeof(_BS_word) * _BS_SHORT_LEN));
            }
            m_nBitsetSize = nBitsetSize;
        }
    }
}

