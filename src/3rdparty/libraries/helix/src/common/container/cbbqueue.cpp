/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cbbqueue.cpp,v 1.11 2007/07/06 20:34:58 jfinnecy Exp $
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

/*******************************************************************
 *
 *	NAME:	cbbqueue.h
 *
 *	CLASS:
 *		CBigByteQueue class declaration.
 *
 *	DESCRIPTION:
 *		Class declaration for a 'Queue of bytes' object.
 *		This object is meant to serve the needs of either clients as
 *		an abstract object, or of subclasses as a base object.
 *
 *		That is a client may use this instances of this class directly,
 *		or they may inherit from the class and provide expanded
 *
 *	NOTES:
 *		This is a re-implementation of CBigByteQueue using 32-bit pointers
 *
 *******************************************************************/

#include "cbbqueue.h"

#include "hlxclib/string.h"		//	for memcpy()

#include "hxassert.h"

#if defined( _WINDOWS ) || defined( _WIN32 )
#include <stdlib.h>		//	for __min()
#endif
#if !defined( __min )
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#endif	//	!defined( __min )

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

 /*
 ** CBigByteQueue::CBigByteQueue( ulSize, ulElementSize )
 *
 *  PARAMETERS:
 *		ulSize			Number of bytes we want to be able to put in queue
 *		ulElementSize		Make our buffer a multiple of this size.
 *							(for subclasses)
 *
 *  DESCRIPTION:
 *		Parameterized constructor.
 *
 *  RETURNS:
 *		void
 */
CBigByteQueue::CBigByteQueue( UINT32 ulSize, UINT32 ulElementSize ) :
	m_pData( 0 ),
	m_pHead( 0 ),
	m_pTail( 0 ),
	m_pMax( 0 ),
	m_ulElementSize( ulElementSize ),
	m_ulMaxSize(0)
{
	HX_ASSERT( this );

	//	We add one here because the queue MUST maintain at least one byte
	//	free in the allocated buffer (to distinguish full from empty states).
	m_ulSize = CBigByteQueue::Base_GranulatedSize( ulSize, ulElementSize ) + 1;
	m_pData = new UCHAR[m_ulSize];
	if (!m_pData)
	{
		//	If we used exceptions, now would be a good time
		//	to throw one.
		m_ulSize = 0;
		HX_ASSERT( 0 );
		return;
	}
	else
	{
		m_pMax = m_pData + Base_GetBufferSize();
		Base_SetEmpty();
#if defined( _DEBUG )
		HX_ASSERT( IsQueueValid() );
		//	Init our buffer w/ known garbage
		memset( m_pData, 0xfc, Base_GetBufferSize() );
#endif	//	_DEBUG
	}
	return;
}	//	CBigByteQueue() - Parameterized constructor


/*
 ** CBigByteQueue::CBigByteQueue( const CBigByteQueue &rReferent )
 *
 *  PARAMETERS:
 *		rReferent		A constant reference to the object we want to copy.
 *
 *  DESCRIPTION:
 *		Copy constructor.
 *
 *  RETURNS:
 *		void
 */
CBigByteQueue::CBigByteQueue( const CBigByteQueue &rReferent ) :
	m_pData( 0 ),
	m_pHead( 0 ),
	m_pTail( 0 ),
	m_pMax( 0 ),
	m_ulSize( 0 ),
	m_ulElementSize( 0 ),
	m_ulMaxSize(0)
{
	HX_ASSERT( this );

	//	Are we copying ourselves?  It's a nop if we are.
	if (&rReferent == this)
	{
		return;
	}

	//	Ok, figure out how large a buffer to get and allocate it
	m_pData = new UCHAR[rReferent.Base_GetBufferSize()];
	if (!m_pData)
	{
		//	If we had exceptions, now would be a good time
		//	to throw one.
		m_ulSize = 0;
		HX_ASSERT( 0 );
		return;
	}
	else
	{
		m_ulSize = rReferent.Base_GetBufferSize();
		m_pMax = m_pData + Base_GetBufferSize();
		m_ulElementSize = rReferent.m_ulElementSize;

		//	Get a copy of the referent's data into our buffer
		rReferent.Base_PeekBuff( m_pData + 1, Base_GetBufferSize() );
		m_pHead = m_pData;
		m_pTail = m_pData + rReferent.Base_GetUsedByteCount();
	}
	return;
}	//	CBigByteQueue() - Copy constructor


/*
 ** CBigByteQueue::~CBigByteQueue()
 *
 *  PARAMETERS:
 *		void
 *
 *  DESCRIPTION:
 *		Virtual destructor for the base class.
 *
 *  RETURNS:
 *		void
 */
CBigByteQueue::~CBigByteQueue()
{
	HX_ASSERT( this );

	if (m_pData)
	{
		HX_ASSERT( IsQueueValid() );    
		delete [] m_pData;
	}
	m_pData = NULL;
	m_pTail = NULL;
	m_pHead = NULL;
	m_pMax = NULL;
	m_ulSize = 0;
	m_ulElementSize = 0;

#if defined( _DEBUG )
	memset( this, FILLER_BYTE, sizeof( *this ) );
#endif

}	//	~CBigByteQueue() - Destructor


/*
 ** CBigByteQueue & CBigByteQueue::operator=( const CBigByteQueue &rReferent )
 *
 *  PARAMETERS:
 *		rReferant		Constant reference to an object to assign from (rValue).
 *
 *  DESCRIPTION:
 *		This is our assignment operator.  It assigns from rReferent to
 *		an existing object.
 *
 *  RETURNS:
 *		A reference to ourselves.
 */
CBigByteQueue & CBigByteQueue::operator=( const CBigByteQueue &rReferent )
{
	HX_ASSERT( this );
	HX_ASSERT( rReferent.IsQueueValid() );
	HX_ASSERT( &rReferent );

	//	Do we need to allocate a new buffer?
	if (rReferent.Base_GetBufferSize() != Base_GetBufferSize())
	{
		//	Yes, Allocate the new buffer & copy into it.
		UCHAR *	pByte;

		//	Ok, figure out how large a buffer to get and allocate it
		pByte = new UCHAR[rReferent.Base_GetBufferSize()];
		if (pByte)
		{
			if (m_pData)
			{
				delete [] m_pData;
			}
			m_pData = NULL;
			m_pData = pByte;
		}
		else
		{
			//	Failed buffer allocataion request....
			//	It would be nice if we could fail gracefully or throw
			//	an exception.
			HX_ASSERT( 0 );
			return( *this );
		}
	}

	//	Now need to copy over all of the other elements
	//	Or at least set our elements to the correct data
	m_ulSize = rReferent.Base_GetBufferSize();
	m_pMax = m_pData + m_ulSize;
	m_ulElementSize = rReferent.m_ulElementSize;

	//	Get a copy of the referent's data into our buffer
	rReferent.Base_PeekBuff( m_pData + 1, Base_GetBufferSize() );
	m_pHead = m_pData;
	m_pTail = m_pData + rReferent.Base_GetUsedByteCount();

	HX_ASSERT( IsQueueValid() );    
	return( *this );
}	//	operator=()


/*
 ** HXBOOL CBigByteQueue::IsQueueValid()
 *
 *  PARAMETERS:
 *		void
 *
 *  DESCRIPTION:
 *		This is meant to validate our queue either for debugging
 *		purposes, or to ensure a queue was correctly created.
 *		(A memory allocation at create time didn't occur).
 *
 *  RETURNS:
 *		void
 */
HXBOOL 
CBigByteQueue::IsQueueValid() const
{
	HX_ASSERT( this );

	//	Ensure we have no NULL pointers & we have a size
	if (!m_pData || !m_pTail || !m_pHead || !m_pMax || !m_ulSize ||
	 !m_ulElementSize)
	{
		return( FALSE );
	}
	//	Ensure m_pTail is in range
	if (m_pTail < m_pData || m_pTail >= m_pMax)
	{
		return( FALSE );
	}
	//	Ensure m_pHead is in range
	if (m_pHead < m_pData || m_pHead >= m_pMax)
	{
		return( FALSE );
	}
	//	Ensure m_pMax is the correct value for our size
	if (m_pMax != m_pData + m_ulSize)
	{
		return( FALSE );
	}

    //	Everything looks good!
	return( TRUE );
}	//	IsQueueValid()


/*
 ** UINT32 CBigByteQueue::Base_DeQueueBytes( pOutBuffer, ulByteCount )
 *
 *  PARAMETERS:
 *		void 	*pOutBuffer		Pointer  to buffer to receive data.
 *		UINT32		ulAmount	Number of bytes desired.
 *
 *  DESCRIPTION:
 *		Attempts to dequeue nAmount bytes from the Queue and transfers them 
 *		to pOutBuffer.
 *
 *  RETURNS:
 *		Number of bytes written to pOutBuffer.
 */
UINT32 
CBigByteQueue::Base_DeQueueBytes( void *pOutBuffer, UINT32 ulByteCount )
{
	UINT32		ulRead;

	HX_ASSERT( this );
	HX_ASSERT( IsQueueValid() );
	HX_ASSERT( pOutBuffer );

	//	First read the Queue into pOutBuffer	
	ulRead = Base_PeekBuff( pOutBuffer, ulByteCount );
	
	//	Now update m_pHead which is our read pointer
	m_pHead = Base_Normalize( m_pHead, ulRead );
	HX_ASSERT( IsQueueValid() );
	return( ulRead );
}	//	Base_DeQueueBytes()


void
CBigByteQueue::SetMaxSize(UINT32 ulMax)
{
    m_ulMaxSize = ulMax;
}
/*
 * Grow the queue to twice its size or at least big enough to hold n more,
 * whichever is greater.  returns 1 for good, 0 for bad.
 */
int
CBigByteQueue::Grow(UINT32 ulItems)
{
    if (m_ulSize == m_ulMaxSize)
    {
	return 0;
    }
    /*
     * Set our initial guess for the new target size by doubling the
     * current size.
     */
    UINT32 ulUsedBytes = Base_GetUsedByteCount();
    UINT32 ulMinFinalCapacity = ulUsedBytes  + ulItems * m_ulElementSize;
    UINT32 ulNewSize = m_ulSize * 2;

    if (m_ulMaxSize && ulMinFinalCapacity > m_ulMaxSize)
    {
	return 0;
    }

    /*
     * Keep doubling until we can hold at least ulFinalMinCapacity.
     */
    while (ulNewSize < ulMinFinalCapacity)
    {
	ulNewSize *= 2;
    }

    if (m_ulMaxSize && ulNewSize > m_ulMaxSize)
    {
	ulNewSize = m_ulMaxSize;
    }
    UCHAR* pNewBuf = new UCHAR[ulNewSize];

    /*
     * Let the queue copy every thing over for us.
     * +1 because its best to start out with head pointing at 0,
     * and data starting at 1.
     */
    Base_DeQueueBytes((void*)(pNewBuf + 1), ulUsedBytes);

    /*
     * Destroy current structure and re-create with new buffer.
     */
    delete[] m_pData;
    m_pData = pNewBuf;
    m_ulSize = ulNewSize;
    //max points one past the end.
    m_pMax = m_pData + m_ulSize;
    //head points at spot before first queued data
    m_pHead = m_pData;
    //tail points at last used byte
    m_pTail = m_pData + ulUsedBytes;

    return 1;

}

/*
 ** UINT32 CBigByteQueue::Base_EnQueueBytes( pInBuffer, ulByteCount )
 *
 *  PARAMETERS:
 *		void	*pInBuffer		Pointer to buffer containing data to EnQueue
 *		UINT32	ulByteCount		Number of bytes items in buffer for EnQueue.
 *
 *  DESCRIPTION:
 *		Attempts to put nByteCount bytes into queue.  If insufficient room, will
 *		not enqueue anything.
 *
 *  RETURNS:
 *		Number of bytes written to pInBuffer.
 *		Should be nByteCount or 0 because we fail if we don't have 
 *		room for ALL data
 */
UINT32 
CBigByteQueue::Base_EnQueueBytes( const void *pInBuffer, UINT32 ulByteCount )
{
	HX_ASSERT( this );
	HX_ASSERT( IsQueueValid() );
	HX_ASSERT( pInBuffer );

	if (!ulByteCount || Base_GetAvailableBytes() < ulByteCount)
	{
		return( 0 );
	}

	//	Ok, we've guaranteed that we have enough room to enqueue nAmount items

	//	Now switch on the state of our head & tail pointers
	if (m_pTail < m_pHead)
	{
		//	No need to normalize pointers, because we're guaranteed
		//	that we have room, hence m_pTail + nAmount HAS to be remain < m_pHead

		//	Remember that m_pTail points at the postion just BEFORE our next
		//	empty spot in the queue
		memcpy( m_pTail + 1, pInBuffer, ulByteCount ); /* Flawfinder: ignore */
		m_pTail += ulByteCount;
	}
	else
	{
		//	m_pTail >= m_pHead
		//	This may require a copy in two passes if we have to wrap around the buffer
		UINT32		ulCopy;
		UINT32		ulPrevCopy;
		void		*pDest;

		//	Copying from (m_pTail + 1) to the end of the allocated buffer or nAmount
		//	which ever comes first.
		pDest = Base_Normalize( m_pTail, 1);
		ulCopy = __min((UINT32)(m_pMax - (UCHAR*)pDest), ulByteCount );
		memcpy( pDest, pInBuffer, ulCopy ); /* Flawfinder: ignore */

		m_pTail = (UCHAR *)pDest + ulCopy - 1;

		//	Figure out how much more we have to copy (if any)
		ulPrevCopy = ulCopy;
		ulCopy = ulByteCount - ulCopy;
		if (ulCopy)
		{
			//	Now we're copying into the base of the allocated array
			//	whatever we didn't copy the first pass around
			memcpy( m_pData, (UCHAR *)pInBuffer + ulPrevCopy, ulCopy ); /* Flawfinder: ignore */
			m_pTail = m_pData + ulCopy - 1;
		}
	}
	HX_ASSERT( IsQueueValid() );
	return( ulByteCount );
}	//	Base_EnQueueBytes()


UINT32
CBigByteQueue::PeekAt( UINT32 ulIndex, void *pOutBuffer ) const
{
	UINT32		ulCopy;
	UINT32		ulByteCount;
	void	*pHead;
	void	*pTail;

	HX_ASSERT( pOutBuffer );
	HX_ASSERT( this );
	HX_ASSERT( IsQueueValid() );

	if (ulIndex >= GetQueuedItemCount())
	{
		return( 0 );
	}

	//	We don't want to modify m_pTail or m_pHead here, so copy them
	//	and use our copies to manipulate the buffer.
	pTail = m_pTail;

	//	Advance pHead till it points at the correct position
	//	relative to the index we want.
	ulByteCount = m_ulElementSize;
	pHead = Base_Normalize( m_pHead,  (ulIndex * ulByteCount + 1) );

	if (pHead < pTail)
	{
		memcpy( pOutBuffer, (UCHAR *)pHead, ulByteCount ); /* Flawfinder: ignore */
		return( ulByteCount );
	}
	else
	{
		//	pHead > pTail
		UINT32		ulPrevCopy;

		//	Copying from (pHead + 1) to the end of the allocated buffer or
		//	nByteCount which ever comes first.
		ulCopy = __min( (UINT32)(m_pMax - (UCHAR *)pHead), ulByteCount );
		memcpy( pOutBuffer, pHead, ulCopy ); /* Flawfinder: ignore */
		
		//	Figure out how much more we have to copy (if any)
		ulPrevCopy = ulCopy;
		ulCopy = ulByteCount - ulCopy;
		if (ulCopy)
		{
			//	Now we're copying from the base of the allocated array
			//	whatever we didn't copy the first pass around
			memcpy( (UCHAR *)pOutBuffer + ulPrevCopy, m_pData, ulCopy ); /* Flawfinder: ignore */
		}
		return( ulCopy + ulPrevCopy );
	}
}

/*
 ** UINT32 CBigByteQueue::Base_PeekBuff( pOutBuffer, ulByteCount )
 *
 *  PARAMETERS:
 *		pOutBuffer		Pointer to buffer to receive data in queue.
 *		ulByteCount		Number of bytes to copy out of queue.
 *
 *  DESCRIPTION:
 *		Private primitive used to copy data out of a queue buffer.
 *		This is a workhorse function used in DeQueue(), operator=(),
 *		and our copy constructor.
 *
 *  RETURNS:
 *		The number of bytes copied out of the buffer.
 */
UINT32
CBigByteQueue::Base_PeekBuff( void *pOutBuffer, UINT32 ulByteCount ) const
{
	UINT32	ulCopy;
	void	*pHead;
	void	*pTail;

	HX_ASSERT( this );
	HX_ASSERT( IsQueueValid() );    

	//	if the Queue is empty, then we can't get anything
	if (IsEmpty())
	{
		return( 0 );
	}

	//	We don't want to modify m_pTail or m_pHead here, so copy them
	//	and use our copies to manipulate the buffer.
	pTail = m_pTail;
	pHead = m_pHead;

	if (pHead < pTail)
	{
		//	We can do the copy in one pass w/o having to Normalize() the pointer
		ulCopy = __min( ulByteCount, Base_GetUsedByteCount() );
		memcpy( pOutBuffer, (UCHAR *)pHead + 1, ulCopy ); /* Flawfinder: ignore */
		return( ulCopy );
	}
	else
	{
		//	pHead > pTail
		UINT32		ulPrevCopy;
		UCHAR *	pSrc;

		//	Copying from (pHead + 1) to the end of the allocated buffer or
		//	nByteCount which ever comes first.
		pSrc = Base_Normalize( (UCHAR *)pHead, 1 );
		ulCopy = __min( (UINT32)(m_pMax - pSrc), ulByteCount );
		memcpy( pOutBuffer, pSrc, ulCopy ); /* Flawfinder: ignore */
		
		//	The __min() above ensures we don't need to Normalize the pointer
		pHead = pSrc + ulCopy - 1;

		//	Figure out how much more we have to copy (if any)
		ulPrevCopy = ulCopy;
		ulCopy = ulByteCount - ulCopy;
		if (ulCopy)
		{
			//	Now we're copying from the base of the allocated array
			//	whatever we didn't copy the first pass around
			memcpy( (UCHAR *)pOutBuffer + ulPrevCopy, m_pData, ulCopy ); /* Flawfinder: ignore */
		}
		return( ulCopy + ulPrevCopy );
	}
}	//	Base_PeekBuff()

