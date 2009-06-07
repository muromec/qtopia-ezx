/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cringbuf.h,v 1.7 2006/07/20 20:27:58 milko Exp $
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

#ifndef _CRINGBUF_
#define _CRINGBUF_

/****************************************************************************
 *  Includes
 */
#define RNGBF_FULL_SIZE	    0x7FFFFFFF


/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"


/****************************************************************************
 *  CRingBuffer
 *
 *  Implements a genreic ring buffer.
 *  Properties:
 *  - ring buffer stores *void
 *  - ring buffer does not perform any allocation/deallocation upon insertion
 *    or removal of data
 *  - ring buffer differentiates between concept of size and max_count
 *    Size     = maximum potential capacity of ring buffer as set upon creation
 *    MaxCount = currently set maximum occupancy of the ring buffer (<= Size)
 *  - MaxCount is adjustable at any time. If already inserted items exceed
 *    MacCount, they will NOT be purged.  They remain in reserve storage and
 *    can still be retrieved.
 *  - When inserting items, capacity beyond MaxCount can be utilized if
 *    if bUseReserve is set in Put method.
 */
class CRingBuffer
{
public:
    CRingBuffer(ULONG32 ulSize)
	: m_ulSize(ulSize)
	, m_lMaxCount((LONG32) ulSize)
	, m_pDataStart(NULL)
	, m_pDataEnd(NULL)
	, m_pHead(NULL)
	, m_pTail(NULL)
    {
	HX_ASSERT(ulSize);

	m_pDataStart = new void* [ulSize + 1];

	HX_ASSERT(m_pDataStart);

	m_pDataEnd = &(m_pDataStart[ulSize]);

	m_pHead = m_pDataStart;
	m_pTail = m_pHead;
    }

    ~CRingBuffer()
    {
	delete [] m_pDataStart;
    }

    HXBOOL Put(void* pData, HXBOOL bUseReserve = FALSE)
    {	
	if (Count() < (LONG32) (bUseReserve ? m_ulSize : m_lMaxCount))
	{
	    *m_pHead = pData;
	    m_pHead = Advance(m_pHead);
	    
	    return TRUE;
	}

	return FALSE;
    }

    void* Get(void)
    {
	void* pData = NULL;

	if (m_pTail != m_pHead)
	{
	    pData = *m_pTail;
	    m_pTail = Advance(m_pTail);
	}

	return pData;
    }

    void* Peek(void)
    {
	void* pData = NULL;

	if (m_pTail != m_pHead)
	{
	    pData = *m_pTail;
	}

	return pData;
    }

    void* PeekTail(UINT32 i = 0)
    {
        void* pRet = NULL;

        if (((LONG32) i) < Count())
        {
            void** ppTmp = m_pTail + i;
            if (ppTmp > m_pDataEnd)
            {
                ppTmp -= m_ulSize + 1;
            }
            pRet = *ppTmp;
        }

        return pRet;
    }

    void* PeekHead(UINT32 i = 0)
    {
        void* pRet = NULL;

        if (((LONG32) i) < Count())
        {
            void** ppTmp = m_pHead - 1 - i;
            if (ppTmp < m_pDataStart)
            {
                ppTmp += m_ulSize + 1;
            }
            pRet = *ppTmp;
        }

        return pRet;
    }

    LONG32 Count(void)
    {
	LONG32 lCount = m_pHead - m_pTail;

	if (lCount < 0)
	{
	    lCount += (m_ulSize + 1);
	}

	return lCount;
    }

    HXBOOL IsFull(HXBOOL bUseReserve = FALSE)
    {
	return (Count() >= (LONG32) (bUseReserve ? m_ulSize : m_lMaxCount));
    }

    void SetMaxCount(LONG32 lMaxCount)
    {
	if (lMaxCount <= ((LONG32) m_ulSize))
	{
	    m_lMaxCount = lMaxCount;
	}
	else
	{
	    m_lMaxCount = (LONG32) m_ulSize;
	}
    }

    LONG32 MaxCount(void)
    {
	return m_lMaxCount;
    }

    ULONG32 Size(void)
    {
	return m_ulSize;
    }

private:
    void** Advance(void** pDataPtr)
    {
	if (pDataPtr != m_pDataEnd)
	{
	    pDataPtr++;
	}
	else
	{
	    pDataPtr = m_pDataStart;
	}

	return pDataPtr;
    }

    ULONG32 m_ulSize;
    LONG32 m_lMaxCount;
    void** m_pDataStart;
    void** m_pDataEnd;
    void** m_pHead;
    void** m_pTail;
};

#endif	// _CRINGBUF_
