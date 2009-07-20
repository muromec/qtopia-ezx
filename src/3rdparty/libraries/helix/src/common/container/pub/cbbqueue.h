/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cbbqueue.h,v 1.7 2005/03/14 19:33:48 bobclark Exp $
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
 *	NAME:	CBBQueue.h
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
 *		functionality.
 *
 *	NOTES:
 *		This is a reimplementation of the existing CBigByteQueue
 *		class using 32-bit pointers instead of 16-bit pointers.
 *
 *******************************************************************/

#ifndef _CBBQUEUE_H_
#define _CBBQUEUE_H_
 
#include "hxtypes.h"
#include "hxassert.h"

#if !defined( NULL )
#define NULL	0
#endif

#if !defined( FALSE )
#define FALSE	0
#endif	//	!defined( FALSE )

#if !defined( TRUE )
#define TRUE	!FALSE
#endif	//	!defined( TRUE )

class CBigByteQueue
{
/*
 *	Our public interface.
 *	These are the methods we export to the world.
 *	These methods are primarily used by our clients.
 */
public:
   /*
	** CBigByteQueue( ulSize, ulElementSize )
	*
	*  PARAMETERS:
	*	ulSize			Size of the bytequeue in bytes.
	*	ulElementSize		For subclasses we ensure our size is a multiple of this.
	*
	*  DESCRIPTION:
	*	Parameterized constructor.
	*	This is the primary means of creating an instance of CBigByteQueue.
	*
	*  RETURNS:
	*	void
	*/
	CBigByteQueue( UINT32 ulSize, UINT32 ulElementSize = 1);

   /*
	** CBigByteQueue( rReferent )
	*
	*  PARAMETERS:
	*	rReferent	Constant reference to another CBigByteQueue object.
	*
	*  DESCRIPTION:
    	*	Copy constructor (ctor).  Copies a CBigByteQueue into
	*	another CBigByteQueue that is under construction.
	*	This guy is called in construction situations like this:
	*		CBigByteQueue		rQueueOrig( 10 );			//	Call param ctor
	*		CBigByteQueue		rQueueCopy = rQueueOrig;	//	Call copy ctor
	*
	*  RETURNS:
	*	void
	*/
	CBigByteQueue( const CBigByteQueue &rReferent );


   /*********************************************************
    *	Here are are non-virtual methods that provide
	*	primitive functionality to all queues.
	*********************************************************/

   /*
	** GetQueuedItemCount()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns a count of the items we have queue'd up.
	*	This function is accurate even in subclasses with
	*	elements that are a different size from a UCHAR.
	*
	*  RETURNS:
	*	Returns the number of ITEMS we have queued up.
	*
	*/
	UINT32 GetQueuedItemCount() const
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		return( Base_GetUsedByteCount() / m_ulElementSize );
	}

   /*
	** UINT32 GetAvailableElements()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns the number of ITEMS we can EnQueue w/o failing.
	*
	*  RETURNS:
	*	0 if the queue is full
	*	non-zero to indicate how many ITEMS we can EnQueue.
	*/
	UINT32 GetAvailableElements() const
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		return( Base_GetAvailableBytes() / m_ulElementSize );
	}

   /*
	** UINT32 GetMaxAvailableElements()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns the number of ITEMS we can EnQueue w/o failing AFTER 
	*	we have grown the queue to its Maximum size.
	*
	*  RETURNS:
	*	0 if the queue is full AND there is no more room to grow
	*	beyond the max size
	*	non-zero to indicate how many ITEMS we can EnQueue.
	*/
	UINT32 GetMaxAvailableElements() const
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		return( Base_GetMaxAvailableBytes() / m_ulElementSize );
	}
   /*
	** HXBOOL IsEmpty()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Tells us if the queue is empty.
	*
	*  RETURNS:
	*	Returns TRUE if the queue is empty.
	*
	*/
	HXBOOL IsEmpty() const
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		return( m_pTail == m_pHead );
	}

   /*
	** CBigByteQueue &operator=( rReferent )
	*
	*  PARAMETERS:
	*	Constant reference to the CBigByteQueue object we are assigning from
	*	(the rValue).
	*
	*  DESCRIPTION:
	*	Assignment operator.
	*	This guy gets called when we assign a CBigByteQueue.
	*	This guy creates a fully functional copy of the source queue.
	*
	*	Subclasses that want an assignment operator SHOULD redefine
	*	this guy, but they should use the base method to copy the 
	*	bits of the base class.
	*
	*  RETURNS:
	*	A reference to the object we are assigning into.
	*/
	CBigByteQueue &operator=( const CBigByteQueue &rReferent );

   /*
	** UINT32 PeekAt( ulIndex, pOutBuffer )
	*
	*  PARAMETERS:
	*	ulIndex		The ulIndex'th object from the head of the queue
	*				that we are interested in.
	*	pOutBuffer	Pointer to the buffer to receive the contents of the
	*				element.
	*
	*  DESCRIPTION:
   	*	Peeks at a particular index off of the first element in the queue.
   	*	The index is 0 based, hence an index of 0 will indicate the queue
   	*	Head element.
	*	Will copy the element of size GetElementSize() into the pOutBuffer.
	*	*pbIsValid is set to FALSE if the element is not valid data.
	*	Notice that the client needn't redifine this guy if the default
	*	is satisfactory.
	*	In particular this method will remain valid even across changes
	*	of object size in the subclass.
	*	The client will only NEED to imlement an override if they need
	*	to provide another level of indirection.
	*
	*  RETURNS:
	*		Returns the number of bytes copied into pOutBuffer.
	*		0 if nIndex specifies an invalid position in the queue.
	*		(for instance if nIndex is 3, but there are only 2 elements
	*		queued up we wil return 0)
	*/
	UINT32 PeekAt( UINT32 ulIndex, void *pOutBuffer ) const;

   /*
	** FlushQueue()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Instantly flush all elements from the queue.
	*
	*  RETURNS:
	*	void
	*/
	void FlushQueue()
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		Base_SetEmpty();
	}

   /*********************************************************
    *	The rest of these public methods are virtual.
	*
	*	HOWEVER, the default behavior is will remain fully
	*	functional across all changes in object size.
	*
	*	The only reason to provide overrides in subclasses
	*	is to provide additional behavior.  If you do
	*	implement an override make sure it calls the base
	*	virtual method.
	*********************************************************/

   /*
	** ~CBigByteQueue()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Destructor
	*	Notice that this is a virtual destructor.  
	*	The base class CBigByteQueue will delete the buffer.
	*	The subclass need only implement on override if they
	*	need additional cleanup besides the buffer.
	*
	*  RETURNS:
	*	void
	*/
	virtual ~CBigByteQueue();
	
   /*
	** HXBOOL IsQueueValid()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	This method allows the client to test the queue object for
	*	validity.  The base class implements default behavior that
	*	tests it's internal buffer pointers. 
	*	The subclass will only need to implement an override if they
	*	have additional validity checks.
	*
	*	Any override of this funcion MUST return the logical AND of
	*	it's validity checks and the checks of the base method.
	*	Sort of like:
	*		return( CBigByteQueue::IsQueueValid() && CSubClass::IsQueueValid() )
	*
	*  RETURNS:
	*	TRUE	If the queue is valid.
	*	FALSE	If there is an error in the queue members.
	*/
	virtual HXBOOL IsQueueValid() const;

   /*
	** UINT32 DeQueue( pOutBuffer, ulItemCount )
	*
	*  PARAMETERS:
	*	pOutBuffer		Pointer to buffer to receive bytes we're pulling
	*					out of the queue.
	*	ulItemCount		Number of items we want to dequeue.
	*
	*  DESCRIPTION:
	*	One of our primary operations.
	*	The client can redefine this function, but it is NOT necessary
	*	as the default implementation will suffice for most cases.
	*	In particular this method will remain valid even across changes
	*	of object size in subclasses.
	*	The client will only NEED to imlement an override if they need
	*	to perform additional processing besides the block move of
	*	bits.
	*
	*  RETURNS:
	*	Number of bytes read out of the queue.
	*/
	virtual UINT32 DeQueue( void *pOutBuffer, UINT32 ulItemCount )
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );
		HX_ASSERT( pOutBuffer );

		return( Base_DeQueueBytes( pOutBuffer, ulItemCount * m_ulElementSize ) );
	}

   /*
	** UINT32 EnQueue( pInBuffer, ulItemCount )
	*
	*  PARAMETERS:
	*	pInBuffer		Pointer to bytes we want to enqueue.
	*	ulItemCount		Number of items we want to enqueue.
	*
	*  DESCRIPTION:
	*	One of our primary operations.
	*	The client can redefine this function, but it is NOT necessary
	*	as the default implementation will suffice for most cases.
	*	In particular this method will remain valid even across changes
	*	of object size in subclasses.
	*	The client will only NEED to imlement an override if they need
	*	to perform additional processing besides the block move of
	*	bits.
	*
	*  RETURNS:
	*	0			If there was not enough room to EnQueue() all the items 
	*				specified.
	*	Non-Zero 	To indicate that all items specified were enqueue'd.
	*
	*/
	virtual UINT32 EnQueue( const void *pInBuffer, UINT32 ulItemCount )
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );
		HX_ASSERT( pInBuffer );

		return( Base_EnQueueBytes( pInBuffer, ulItemCount * m_ulElementSize ) );
	}


	/*
	 * Grow the queue to twice its size or at least big enough to hold n more,
	 * whichever is greater.  Returns 1 for good, 0 for bad.
	 */
	int Grow(UINT32 ulItems);

	void SetMaxSize(UINT32 ulMax);


	


/*
 *	Protected primitives for accessing the buffer and it's 
 *	pointers directly.
 *	These methods are available to our subclasses, but NOT to our
 *	clients.
 */
protected:
   /*
	** UINT32 Base_GetBufferSize()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
    *	Returns the actual allocated size of the buffer.
	*
	*  RETURNS:
	*	Size of the allocated buffer.
	*/
	UINT32 Base_GetBufferSize() const
	{
		HX_ASSERT( this );

		return( m_ulSize );
	}

   /*
	** UINT16 Base_GetMaxBufferSize()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns the max size of the queue.
	*
	*  RETURNS:
	*	Returns the max size of the queue.
	*/
	UINT32 Base_GetMaxBufferSize() const
	{
		HX_ASSERT( this );

		return( m_ulMaxSize );
	}

   /*
	** UINT32 Base_GetUsedByteCount()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns the actual number of bytes we've enqueued.
	*
	*  RETURNS:
	*	Number of bytes in USE in the queue.
	*/
	UINT32 Base_GetUsedByteCount() const
	{
		LONG32		iItemCount;

		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		iItemCount = (LONG32)(m_pTail - m_pHead);

		//	If iItemCount < 0 then we need to add m_nSize
		iItemCount += (iItemCount < 0) ? Base_GetBufferSize() : 0;
		HX_ASSERT(iItemCount <= (LONG32)Base_GetBufferSize());
		return( (UINT32)iItemCount );
	}

   /*
	** UINT32 Base_GetAvailableBytes()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns the number of bytes we can enqueue w/o failing.
	*
	*  RETURNS:
	*	Returns the number of bytes we can enqueue w/o failing.
	*/
	UINT32 Base_GetAvailableBytes() const
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		return( Base_GetBufferSize() - Base_GetUsedByteCount() - 1 );
	}

   /*
	** UINT32 Base_GetMaxAvailableBytes()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns the number of bytes we can enqueue w/o failing AFTER
	*	the queue has been grown to its maximum capacity.
	*
	*  RETURNS:
	*	Returns the number of bytes we can enqueue w/o failing.
	*/
	UINT32 Base_GetMaxAvailableBytes() const
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );    

		return( Base_GetMaxBufferSize() - Base_GetUsedByteCount() - 1 );
	}

   /*
	** UINT32 Base_EnQueueBytes( pInBuffer, ulByteCount )
	*
	*  PARAMETERS:
	*	pInBuffer		Pointer to bytes to enqueue.
	*	ulByteCount		Number of bytes to enqueue.
	*
	*  DESCRIPTION:
	*	Enqueue's a stream of bytes.
	*	(Puts bytes INTO the queue)
	*
	*  RETURNS:
	*	0 if there was insufficient room to enqueue nByteCount bytes.
	*	Number of bytes enqueued.
	*
	*/
	UINT32 Base_EnQueueBytes( const void *pInBuffer, UINT32 ulByteCount );

   /*
	** UINT32 Base_DeQueueBytes( pOutBuffer, ulByteCount )
	*
	*  PARAMETERS:
	*	pOutBuffer		Pointer to buffer to receive bytes from queue.
	*	ulByteCount		Number of bytes to remove from queue.
	*
	*  DESCRIPTION:
	*	DeQueue's a stream of bytes.
	*	(Takes bytes OUT of the queue)
	*
	*  RETURNS:
	*	The number of bytes dequeued from the queue.
	*/
	UINT32 Base_DeQueueBytes( void *pOutBuffer, UINT32 ulByteCount );


/*
 *	Private Implementation data.  We don't share this stuff w/ our subclasses.
 *	this way we can enforce our public and protected interface.
 */
private:
	UCHAR	*m_pData;   // the actual buffer pointer.
	UCHAR	*m_pHead;	// points one byte before the next bytes to be
						//	dequeue'd() from the queue (if !Empty).
	UCHAR	*m_pTail;	// points at last byte of valid data in queue.
						//	actually one byte before the next byte to receive new queue'd data
	UCHAR	*m_pMax;	// pointer to one position beyond what we've allocated
						//	helps us limit check m_pHead & mpTail.

	UINT32		m_ulSize;	// # of bytes in alloacated buffer
	
	UINT32		m_ulElementSize;	//	For our subclasses it's the size of an element
						//	We'll make our buffer a multiple of this
	UINT32		m_ulMaxSize;		// if set, max size queue can grow to.

	enum
	{
		FILLER_BYTE = 0xCC
	};
								
   /*
	** void Base_SetEmpty()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Instantly empty the queue.
	*
	*  RETURNS:
	*
	*/
	void Base_SetEmpty()
	{
		HX_ASSERT( this );

		m_pTail = m_pHead = m_pMax - 1;
	}
	
   /*
	** PBYTE Base_Normalize( pBuffer )
	*
	*  PARAMETERS:
	*	pBuffer		Pointer to our buffer that we want to normalize.
	*
	*  DESCRIPTION:
	*	Used to keep buffer pointer elements (m_pHead & m_pTail) in range 
	*	of m_pData to m_pMax-1.
	*	Basically this method helps us implement the mod function except
	*	we work w/ pointers, and we don't actually divide.
	*
	*  RETURNS:
	*	Normalized pointer.
	*/
	UCHAR * Base_Normalize( UCHAR * pBuffer, UINT32 offset ) const
	{
		HX_ASSERT( this );
		HX_ASSERT( IsQueueValid() );
		HX_ASSERT( pBuffer );

		pBuffer += offset;
		while (pBuffer >= m_pMax)
		{
			pBuffer -= m_ulSize;
		}
		return( pBuffer );
	}

   /*
	** UINT32 Base_GranulatedSize( ulSize, ulGranularity )
	*
	*  PARAMETERS:
	*	ulSize			A "proposed" size for our buffer.
	*	ulGranularity	The multiplier (for subclasses this is the size
	*					of one of our elements).
	*
	*  DESCRIPTION:
	*	Performs calcs to ensure our size is a multiple of our granularity.
	*	This is done by rounding UP to the next even multiple of nGranularity
	*	that is >= nSize.
	*
	*  RETURNS:
	*	A rounded up quantity.
	*/
	static UINT32 Base_GranulatedSize( UINT32 ulSize, UINT32 ulGranularity = 1 )
	{
		return( ((ulSize + ulGranularity - 1) / ulGranularity) * ulGranularity );
	}

   /*
	** CBigByteQueue()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Default constructor:  We hide this guy because we want to enforce
	*	the parameterized constructor.
	*	We might at some later time relax that restriction and allow
	*	a two step creation process, but not for now.
	*
	*  RETURNS:
	*	void
	*/
	CBigByteQueue()	{}

   /*
	** UINT32 Base_PeekBuff( pOutBuffer, ulByteCount )
	*
	*  PARAMETERS:
	*	pOutBuffer		Pointer to buffer to receive bytes.
	*	ulByteCount		Desired max bytes to copy out of queue.
	*
	*  DESCRIPTION:
	*	Copies bytes (nByteCount) from the Queue head to pOutBuffer.
	*	returns the number of bytess actually copied into pOutBuffer.
	*	This function does NOT modify the queue.  It is strictly a 
	*	peek of the queue bytes specified.
	*	Our limiting factor is:
	*		 HX_MIN( ulByteCount, Base_GetUsedByteCount() ).
	*
	*  RETURNS:
	*	Number of bytes copied into pOutBuffer.
	*/
	UINT32 Base_PeekBuff( void *pOutBuffer, UINT32 ulByteCount ) const;


};	//	class CBigByteQueue

#endif	//	if !defined( _CBBQUEUE_H_ )
