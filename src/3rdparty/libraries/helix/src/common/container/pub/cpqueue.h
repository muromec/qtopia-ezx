/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cpqueue.h,v 1.5 2007/07/06 20:35:02 jfinnecy Exp $
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
 *	NAME:	CPQueue.h
 *
 *	CLASS:
 *		CPtrQueue class declaration.
 *
 *	DESCRIPTION:
 *		Class declaration for a 'Queue of pointers' object.
 *		This object inherits from the CByteQueue object by
 *		overriding the GetElementSize() method, and
 *		defining it's own assignemt operator.
 *
 *	NOTES (from CByteQueue):
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
 *******************************************************************/
 
#if !defined( _CPQUEUE_H )
#define _CPQUEUE_H

#include "cbqueue.h"

class CPtrQueue : public CByteQueue
{
public:
   /*
    *	These are the functions we NEED to implement to get the
	*	desired functionality out of our base class CByteQueue.
	*/

   /*
	** CPtrQueue( nMaxPtrs )
	*
	*  PARAMETERS:
	*	nMaxPtrs		The maximum number of pointers we want to be able
	*					to enqueue.
	*
	*  DESCRIPTION:
	*	Parameterized constructor.
	*	This is the primary means of constructing a Pointer Queue.
	*
	*  RETURNS:
	*	void
	*/
	CPtrQueue( UINT16 nMaxPtrs ) :
	 CByteQueue( nMaxPtrs * sizeof( void * ), sizeof( void * ) )
	{
		//	Deliberately empty
	}
	
	/*
	** UINT16 GetElementSize()
	*
	*  PARAMETERS:
	*	void
	*
	*  DESCRIPTION:
	*	Returns the size of one of our queue elements.
	*	In this case it's the size of a pointer.
	*
	*  RETURNS:
	*	The size of a queue element.
	*/
	virtual UINT16 GetElementSize() const
	{
		return( sizeof( void * ) );
	}

	/*
	** CPtrQueue &operator=( rReferent )
	*
	*  PARAMETERS:
	*	Constant reference to a queue we want to copy.
	*
	*  DESCRIPTION:
	*	Custom assignment operator allows us to assign Queues.
	*
	*  RETURNS:
	*	A reference to the queue we've assigned into.
	*/
	CPtrQueue &operator=( const CPtrQueue &rReferent )
	{
		// return( CByteQueue::operator=( rReferent ) );
		this->CByteQueue::operator=( rReferent );
		return( *this );
	}

   /*
    *	These are the functions we've added to provide
	*	some of the semantics that are in the problem
	*	domain of a queue of pointers.
	*/
	/*
	** UINT16 EnQueuePtr( pPtr )
	*
	*  PARAMETERS:
	*	A pointer we want to enqueue.
	*
	*  DESCRIPTION:
	*	Takes the value assigned to pPtr and enqueues it.
	*	
	*  RETURNS:
	*	The number of bytes we put into the queue.
	*/
	UINT16 EnQueuePtr( void *pPtr )
	{
		return( Base_EnQueueBytes( &pPtr, sizeof( pPtr ) ) );
	}

	/*
	** void *DeQueuePtr( HXBOOL &bIsValid )
	*
	*  PARAMETERS:
	*	A reference to a boolean flag indicating if the return is valid.
	*
	*  DESCRIPTION:
	*	Dequeues the head pointer element in the queue.
	*
	*  RETURNS:
	*	The value of the head pointer element we've dequeue'd.
	*/
	void *DeQueuePtr( HXBOOL &bIsValid )
	{
		UINT16		iWrote;
		void	*pPtr;

		iWrote = Base_DeQueueBytes( &pPtr, sizeof( pPtr ) );
		if (iWrote)
		{
			bIsValid = TRUE;
			return( pPtr );
		}
		else
		{
			bIsValid = FALSE;
			return( NULL );
		}
	}

	/*
	** void *PeekPtrAt( nIndex, HXBOOL &bIsValid )
	*
	*  PARAMETERS:
	*	nIndex		The index of the element off of the head
	*				queue element we want to peek at.
	*
	*  DESCRIPTION:
	*	Peeks at an element offset from the queue head.
	*
	*  RETURNS:
	*	The value of the pointer element we're peeking at
	*	(if it's a valid element).
	*	bIsValid is set to true or false to indicate the
	*	validity of the return value.
	*	(since a NULL pointer might be a valid queue entry.
	*/
	void *PeekPtrAt( UINT16 nIndex, HXBOOL &bIsValid )
	{
		UINT16		iCount;
		void	*pPtr;

		iCount = PeekAt( nIndex, &pPtr );
		if (iCount)
		{
			bIsValid = TRUE;
			return( pPtr );
		}
		else
		{
			bIsValid = FALSE;
			return( NULL );
		}
	}
};

#endif	//	!defined( _CPQUEUE_H )
