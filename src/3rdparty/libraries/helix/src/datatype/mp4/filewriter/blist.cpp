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
/*
 * blist.cpp: block list
 */
#include "hxstring.h"
#include "blist.h"

CBList::_memblock_t* CBList::NewBlock()
{
    _memblock_t* ret = new _memblock_t;
    if( ret )
    {
	ret->ppBlock = new void*[m_iBlockSize];
	if( ret->ppBlock )
	{
	    memset( ret->ppBlock, 0, m_iBlockSize * sizeof(void*) );
	    ret->iBlockSize = m_iBlockSize;
	    ret->iBlockLen  = 0;
	    ret->pNext = NULL;
	    
	    m_iTotalSize += m_iBlockSize;
	}
	else
	{
	    delete ret;
	    ret = NULL;
	}
    }
    return ret;
}
		
CBList::CBList( int iBlockSize )
{
    m_iBlockSize = iBlockSize;
    m_iSize = 0;
    m_iTotalSize = 0;
    m_pHead = NewBlock();
}

CBList::~CBList()
{
    _memblock_t* pCur;
    while( (pCur = m_pHead) )
    {
	m_pHead = m_pHead->pNext;
	
	delete [] pCur->ppBlock;
	delete pCur;
    }
}

void* CBList::GetAt( int idx ) const
{
    _memblock_t* pCur = m_pHead;

    while( pCur && ( idx > pCur->iBlockSize ) )
    {
	idx -= pCur->iBlockSize;
	pCur = pCur->pNext;
    }
    if( pCur && ( idx < pCur->iBlockLen ) )
    {
	return pCur->ppBlock[idx];
    }
    return NULL;
}

void CBList::SetAt( int idx, void* value )
{
    _memblock_t* pCur = m_pHead;

    while( pCur->iBlockSize == pCur->iBlockLen )
    {
	idx -= pCur->iBlockSize;
	// We hit the end and all the blocks are full; grow
	if( pCur->pNext == NULL )
	{
	    pCur->pNext = NewBlock();
	}
	pCur = pCur->pNext;
    }
    // expanding
    if( idx >= pCur->iBlockLen )
    {
	int diff = (idx+1) - pCur->iBlockLen;
	pCur->iBlockLen += diff;
	m_iSize += diff;
    }
    pCur->ppBlock[ idx ] = value;
}

void *& CBList::operator[](int idx)
{
    _memblock_t* pCur = m_pHead;

    while( pCur && ( idx >= pCur->iBlockSize ) )
    {
	idx -= pCur->iBlockSize;

	// small threshhold to allow expanding the array here
	if( pCur->pNext == NULL && idx < 10 )
	{
	    pCur->pNext = NewBlock();
	}
	
	pCur = pCur->pNext;
    }
    if( pCur )
    {
	if( idx >= pCur->iBlockLen )
	{
	    int diff = (idx+1) - pCur->iBlockLen;
	    pCur->iBlockLen += diff;
	    m_iSize += diff;
	}
	return pCur->ppBlock[idx];
    }

    HX_ASSERT("invalid indexing" && 0);
    // this generates a warning that can't be easily worked around
}

void CBList::DumpToBuffer( void* pBuf, bool bWordSwap )
{
    _memblock_t* pCur = m_pHead;

    UCHAR* p = (UCHAR*) pBuf;
    while( pCur )
    {
	UINT32 size = pCur->iBlockLen * sizeof(void*);

	if( bWordSwap )
	{
	    UCHAR* pBlock = (UCHAR*) pCur->ppBlock;
	    while( size > 0 )
	    {
		*p++ = pBlock[3];
		*p++ = pBlock[2];
		*p++ = pBlock[1];
		*p++ = pBlock[0];

		pBlock += 4;
		size -= 4;
	    }
	}
	else {
	    memcpy( p, pCur->ppBlock, size );
	    p += size;
	}
	
	pCur = pCur->pNext;
    }
}
