
/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cbqueue.cpp,v 1.11 2005/03/24 00:01:36 liam_murray Exp $
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

/*******************************************************************
 *
 *	NAME:	CBQueue.h
 *
 *	CLASS:
 *		CByteQueue class declaration.
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
 *		The only time a subclass MUST provide a virtual override
 *		is for the GetElementSize() method when the subclass changes
 *		the size of queued elements.  All other virtual methods
 *		provide fully functional default behavior that will ramain
 *		functional even when the ElementSize changes.
 *
 *		The assignment operator is one of the few cases where a
 *		subclass will need to provide new functionality by virtue of
 *		inheriting from the base.  Subclasses should use the base
 *		method for assigning the bits of the base class prior to
 *		performing their own assignment related operations.
 *
 *
 *******************************************************************/


#include "hlxclib/string.h"		//	for memcpy()
#include "hxassert.h"
#include "cbqueue.h"

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
 ** CByteQueue::CByteQueue( nSize, nGranularity )
 *
 *  PARAMETERS:
 *		nSize			Number of bytes we want to be able to put in queue
 *		nGranularity	Make our buffer a multiple of this size.
 *							(for subclasses)
 *
 *  DESCRIPTION:
 *		Parameterized constructor.
 *
 *  RETURNS:
 *		void
 */
CByteQueue::CByteQueue( UINT16 nSize, UINT16 nGranularity ) :
	m_pData( 0 ),
	m_pHead( 0 ),
	m_pTail( 0 ),
	m_pMax( 0 ),
	m_nGranularity( nGranularity ),
	m_nMaxSize(0)
{
	HX_ASSERT( this );

	//	We add one here because the queue MUST maintain at least one byte
	//	free in the allocated buffer (to distinguish full from empty states).
	m_nSize = CByteQueue::Base_GranulatedSize( nSize, nGranularity ) + 1;
	m_pData = new UCHAR[m_nSize];
	if (!m_pData)
	{
		//	If we used exceptions, now would be a good time
		//	to throw one.
		m_nSize = 0;
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
}	//	CByteQueue() - Parameterized constructor


/*
 ** CByteQueue::CByteQueue( const CByteQueue &rReferent )
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
CByteQueue::CByteQueue( const CByteQueue &rReferent ) :
	m_pData( 0 ),
	m_pHead( 0 ),
	m_pTail( 0 ),
	m_pMax( 0 ),
	m_nSize( 0 ),
	m_nGranularity( 0 ),
	m_nMaxSize(0)
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
		m_nSize = 0;
		HX_ASSERT( 0 );
		return;
	}
	else
	{
		m_nSize = rReferent.Base_GetBufferSize();
		m_pMax = m_pData + Base_GetBufferSize();
		m_nGranularity = rReferent.m_nGranularity;

		//	Get a copy of the referent's data into our buffer
		rReferent.Base_PeekBuff( m_pData + 1, Base_GetBufferSize() );
		m_pHead = m_pData;
		m_pTail = m_pData + rReferent.Base_GetUsedByteCount();

#if defined( _DEBUG )
		HX_ASSERT( IsQueueValid() );
		//	Init our buffer w/ known garbage
		memset( m_pData, 0xfc, rReferent.Base_GetBufferSize() );
#endif	//	_DEBUG
	}
	return;
}	//	CByteQueue() - Copy constructor


/*
 ** CByteQueue::~CByteQueue()
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
CByteQueue::~CByteQueue()
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
	m_nSize = 0;
	m_nGranularity = 0;

#if defined( _DEBUG )
	memset( this, FILLER_BYTE, sizeof( *this ) );
#endif

}	//	~CByteQueue() - Destructor


/*
 ** CByteQueue & CByteQueue::operator=( const CByteQueue &rReferent )
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
CByteQueue & CByteQueue::operator=( const CByteQueue &rReferent )
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
	m_nSize = rReferent.Base_GetBufferSize();
	m_pMax = m_pData + m_nSize;
	m_nGranularity = rReferent.m_nGranularity;

	//	Get a copy of the referent's data into our buffer
	rReferent.Base_PeekBuff( m_pData + 1, Base_GetBufferSize() );
	m_pHead = m_pData;
	m_pTail = m_pData + rReferent.Base_GetUsedByteCount();

	HX_ASSERT( IsQueueValid() );    
	return( *this );
}	//	operator=()


/*
 ** HXBOOL CByteQueue::IsQueueValid()
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
HXBOOL CByteQueue::IsQueueValid() const
{
	HX_ASSERT( this );

	//	Ensure we have no NULL pointers & we have a size
	if (!m_pData || !m_pTail || !m_pHead || !m_pMax || !m_nSize ||
	 !m_nGranularity)
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
	if (m_pMax != m_pData + m_nSize)
	{
		return( FALSE );
	}

    //	Everything looks good!
	return( TRUE );
}	//	IsQueueValid()


/*
 ** UINT16 CByteQueue::Base_DeQueueBytes( pOutBuffer, nByteCount )
 *
 *  PARAMETERS:
 *		void 	*pOutBuffer		Pointer  to buffer to receive data.
 *		UINT16		nAmount			Number of bytes desired.
 *
 *  DESCRIPTION:
 *		Attempts to dequeue nAmount bytes from the Queue and transfers them 
 *		to pOutBuffer.
 *
 *  RETURNS:
 *		Number of bytes written to pOutBuffer.
 */
UINT16 CByteQueue::Base_DeQueueBytes( void *pOutBuffer, UINT16 nByteCount )
{
	UINT16		nRead;

	HX_ASSERT( this );
	HX_ASSERT( IsQueueValid() );
	HX_ASSERT( pOutBuffer );

	//	First read the Queue into pOutBuffer	
	nRead = Base_PeekBuff( pOutBuffer, nByteCount );
	
	//	Now update m_pHead which is our read pointer
	m_pHead = Base_Normalize( m_pHead, nRead );
	HX_ASSERT( IsQueueValid() );
	return( nRead );
}	//	Base_DeQueueBytes()


void
CByteQueue::SetMaxSize(UINT16 ulMax)
{
    m_nMaxSize = ulMax;
}
/*
 * Grow the queue to twice its size or at least big enough to hold n more,
 * whichever is greater.  returns 1 for good, 0 for bad.
 */
int
CByteQueue::Grow(UINT16 nItems)
{
    //XXXPM Need to check for wrap around on these UINT16s

    if (m_nSize == m_nMaxSize)
    {
	return 0;
    }
    /*
     * Set our initial guess for the new target size by doubling the
     * current size.
     */
    UINT16 ulUsedBytes = Base_GetUsedByteCount();
    UINT16 ulMinFinalCapacity = ulUsedBytes  + nItems * GetElementSize() + 1;
    UINT16 ulNewSize;

    // check ulMinFinalCapacity for rollover 
    if (ulMinFinalCapacity < m_nSize)
    {
	return 0;
    }

    if (m_nMaxSize && ulMinFinalCapacity > m_nMaxSize)
    {
	return 0;
    }

    for (ulNewSize = 0xFFFF; ulNewSize && (ulNewSize >= ulMinFinalCapacity); ulNewSize = ulNewSize >> 1)
    {
	;
    }

    if (!ulNewSize)
        return 0;

    ulNewSize = (ulNewSize << 1) + 1;

    if (m_nMaxSize && ulNewSize > m_nMaxSize)
    {
	ulNewSize = m_nMaxSize;
    }
    UCHAR* pNewBuf = new UCHAR[ulNewSize];
    if( !pNewBuf )
    {
        // It would be nice to be able to return HXR_OUTOFMEMORY.
        return 0;
    }

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
    m_nSize = ulNewSize;
    //max points one past the end.
    m_pMax = m_pData + m_nSize;
    //head points at spot before first queued data
    m_pHead = m_pData;
    //tail points at last used byte
    m_pTail = m_pData + ulUsedBytes;

    return 1;

}

/*
 ** UINT16 CByteQueue::Base_EnQueueBytes( pInBuffer, nByteCount )
 *
 *  PARAMETERS:
 *		void	*pInBuffer		Pointer to buffer containing data to EnQueue
 *		UINT16		nByteCount		Number of bytes items in buffer for EnQueue.
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
UINT16 CByteQueue::Base_EnQueueBytes( const void *pInBuffer, UINT16 nByteCount )
{
	HX_ASSERT( this );
	HX_ASSERT( IsQueueValid() );
	HX_ASSERT( pInBuffer );

	if (!nByteCount || Base_GetAvailableBytes() < nByteCount)
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
		memcpy( m_pTail + 1, pInBuffer, nByteCount ); /* Flawfinder: ignore */
		m_pTail += nByteCount;
	}
	else
	{
		//	m_pTail >= m_pHead
		//	This may require a copy in two passes if we have to wrap around the buffer
		UINT16		nCopy;
		UINT16		nPrevCopy;
		void	*pDest;

		//	Copying from (m_pTail + 1) to the end of the allocated buffer or nAmount
		//	which ever comes first.
		pDest = Base_Normalize( m_pTail, 1);
		nCopy = __min( (UINT16)(m_pMax - (UCHAR *)pDest), nByteCount );
		memcpy( pDest, pInBuffer, nCopy ); /* Flawfinder: ignore */

		m_pTail = (UCHAR *)pDest + nCopy - 1;

		//	Figure out how much more we have to copy (if any)
		nPrevCopy = nCopy;
		nCopy = nByteCount - nCopy;
		if (nCopy)
		{
			//	Now we're copying into the base of the allocated array
			//	whatever we didn't copy the first pass around
			memcpy( m_pData, (UCHAR *)pInBuffer + nPrevCopy, nCopy ); /* Flawfinder: ignore */
			m_pTail = m_pData + nCopy - 1;
		}
	}
	HX_ASSERT( IsQueueValid() );
	return( nByteCount );
}	//	Base_EnQueueBytes()


UINT16 CByteQueue::PeekAt( UINT16 nIndex, void *pOutBuffer ) const
{
	UINT16		nCopy;
	UINT16		nByteCount;
	void	*pHead;
	void	*pTail;

	HX_ASSERT( pOutBuffer );
	HX_ASSERT( this );
	HX_ASSERT( IsQueueValid() );

	if (nIndex >= GetQueuedItemCount())
	{
		return( 0 );
	}

	//	We don't want to modify m_pTail or m_pHead here, so copy them
	//	and use our copies to manipulate the buffer.
	pTail = m_pTail;

	//	Advance pHead till it points at the correct position
	//	relative to the index we want.
	nByteCount = GetElementSize();
	pHead = Base_Normalize( m_pHead,  (nIndex * nByteCount + 1) );

	if (pHead < pTail)
	{
		memcpy( pOutBuffer, (UCHAR *)pHead, nByteCount ); /* Flawfinder: ignore */
		return( nByteCount );
	}
	else
	{
		//	pHead > pTail
		UINT16		nPrevCopy;

		//	Copying from (pHead + 1) to the end of the allocated buffer or
		//	nByteCount which ever comes first.
		nCopy = __min( (UINT16)(m_pMax - (UCHAR *)pHead), nByteCount );
		memcpy( pOutBuffer, pHead, nCopy ); /* Flawfinder: ignore */
		
		//	Figure out how much more we have to copy (if any)
		nPrevCopy = nCopy;
		nCopy = nByteCount - nCopy;
		if (nCopy)
		{
			//	Now we're copying from the base of the allocated array
			//	whatever we didn't copy the first pass around
			memcpy( (UCHAR *)pOutBuffer + nPrevCopy, m_pData, nCopy ); /* Flawfinder: ignore */
		}
		return( nCopy + nPrevCopy );
	}
}

/*
 ** UINT16 CByteQueue::Base_PeekBuff( pOutBuffer, nByteCount )
 *
 *  PARAMETERS:
 *		pOutBuffer		Pointer to buffer to receive data in queue.
 *		nByteCount		Number of bytes to copy out of queue.
 *
 *  DESCRIPTION:
 *		Private primitive used to copy data out of a queue buffer.
 *		This is a workhorse function used in DeQueue(), operator=(),
 *		and our copy constructor.
 *
 *  RETURNS:
 *		The number of bytes copied out of the buffer.
 */
UINT16 CByteQueue::Base_PeekBuff( void *pOutBuffer, UINT16 nByteCount ) const
{
	UINT16		nCopy;
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
		nCopy = __min( nByteCount, Base_GetUsedByteCount() );
		memcpy( pOutBuffer, (UCHAR *)pHead + 1, nCopy ); /* Flawfinder: ignore */
		return( nCopy );
	}
	else
	{
		//	pHead > pTail
		UINT16		nPrevCopy;
		UCHAR *	pSrc;

		//	Copying from (pHead + 1) to the end of the allocated buffer or
		//	nByteCount which ever comes first.
		pSrc = Base_Normalize( (UCHAR *)pHead, 1 );
		nCopy = __min( (UINT16)(m_pMax - pSrc), nByteCount );
		memcpy( pOutBuffer, pSrc, nCopy ); /* Flawfinder: ignore */
		
		//	The __min() above ensures we don't need to Normalize the pointer
		pHead = pSrc + nCopy - 1;

		//	Figure out how much more we have to copy (if any)
		nPrevCopy = nCopy;
		nCopy = nByteCount - nCopy;
		if (nCopy)
		{
			//	Now we're copying from the base of the allocated array
			//	whatever we didn't copy the first pass around
			memcpy( (UCHAR *)pOutBuffer + nPrevCopy, m_pData, nCopy ); /* Flawfinder: ignore */
		}
		return( nCopy + nPrevCopy );
	}
}	//	Base_PeekBuff()

