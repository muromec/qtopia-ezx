/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmm.h,v 1.7 2007/07/06 20:44:06 jfinnecy Exp $
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

#pragma once

#ifndef _HXMM_
#define _HXMM_
#endif


//#include "pn_macmemory.h"
//#include "pnmm_c.h"

#ifdef _CARBON
#ifdef BUILDING_PNCRT
#include <Carbon.h>
#endif
#ifndef _MAC_MACHO
#include <size_t.h>
#endif
#else
#ifndef size_t
typedef unsigned long size_t;
#endif
#endif

#include "hxtypes.h"

#define		kTempMemPoolHandle	'temp'
#define		kPoolSign  			'POOL'
#define		kBlockSignature		'BLOK'

extern	Boolean	_gInterruptCodeEnabled;
extern  long	g_InterruptsOn;


#define USE_PNMM 1		// uncomment this line to enable PNMM code in Player

typedef	enum PNMM_MEMORYTYPE_TAG
{
	PNMM_MEMORYTYPE_ALL=0,
	PNMM_MEMORYTYPE_EXTERNAL,
	PNMM_MEMORYTYPE_ALLPOOLS,
	PNMM_MEMORYTYPE_FREEPOOLS,
	PNMM_MEMORYTYPE_INTERRUPTPOOLS,
	PNMM_MEMORYTYPE_NONINTERRUPTPOOLS
} PNMM_MEMORYTYPE;


/*
typedef struct MemPool {
	long				signature;			//  POOL SIGNATURE contains kPoolSign
	struct MemPool		*next;				//	pointer to next pool
	struct MemPool		*prev;				//  previous memory pool.
	long				left;				// number of bytes left
	long				leftlast;			// number of bytes after last update.
	long				locked;				// is this block locked?
	long*				lastfree;
	char*				endofpool;
//	DEALLOCMEM*			fpDeleteFunction;	//	the delete function associated with this pool.
	long				interrupt_only;		//	the pool is locked out from non interrupt 
											//	allocations.
	long				allocatedblocks;	//  Number of allocated blocks in this pool.										
	long*				record;
	size_t				size;				//	number of bytes in pool (including header)
	char				data[];				//	variable size user data section
											//  time only.  
}	MemPool;
*/

/*
typedef	struct	PNMM_MEMORY_INFO
{
	long		numberOfPools;		// Number of pools allocated
	MemPool*	firstPool;			// Ptr to first pool struct.
	long		totalAllocated;		// Total Memory allocated for all pools.
	long		totalLeft;			// Total memory free in all pools.
	long		totalLocked;		// Total Number of pools that are locked.

	long		totalAllocated_Interrupt;	// Total Memory allocated for interrupt time.
	long		totalLeft_Interrupt;		// Total Memory free in interrupt pools.

	//
	//	To get the following information you must have "extendedinfo" set to true
	//	when you call PNMM_INFO;
	//	
	
	long		totalblocks;		// Total Number of blocks that are constructed in
									// all pools.
									
	long		largestblock;		// Largest Unused block, in any block, of any pool.
	
	
	
}	PNMM_MEMORY_INFO;
*/


//
//	This is the block header, it is an enumerated structure.
//	Note that the ExternalPtr and PoolPtr, both use the same 4 bytes.
//
#define		kBlockHeaderSize		16
enum 
{
	kBlockHeader_Signature = 0,
	kBlockHeader_DeleteFunction,
	kBlockHeader_ExtraSize=kBlockHeader_DeleteFunction,	
	kBlockHeader_PoolPtr,
	kBlockHeader_ExternalPtr=kBlockHeader_PoolPtr,
	kBlockHeader_BlockSize
};

//
//
//	This is a structure used for keeping track of pools.
//	This allows us to make the best choice possible when using Virtual Memory,
//	allowing us to not cause page faults when we don't have to.
//
//

/*
typedef	struct	tag_PoolRecord
{
	MemPool*	pool;
	long		poolleft;
	long		poolend;
	long		pool_interrupt;
} PNMM_POOLRECORD;
*/


typedef void (LOWMEMORYNOTIFIER)(void);

#ifdef __cplusplus
extern "C" {
#endif


/*
//
//	Override function.
//
Boolean					PNMM_OVERRIDE_ALLOC(ALLOCMEM*	pNew,	DEALLOCMEM*		pDelete);
*/

//
//	Block allocation functions
//
Boolean					PNMM_NEWBLOCK(size_t newsize, Boolean	interrupt_only);

//
//	Clean up function. 
//
void					PNMM_FORCECLEANUP(PNMM_MEMORYTYPE	MemType);


//
//	Getting information about the state of the memory allocator.
//
void					HXMM_COMPACT(void);
//PNMM_MEMORY_INFO*		PNMM_INFO(Boolean extendedinfo);
//HXBOOL					HXMM_ATINTERRUPT(void);
HXBOOL					HXMM_ATINTERRUPT(void);




//
//	State information to give us an idea of what kind of memory to be using.
//

void					HXMM_INTERRUPTON(void);
void					HXMM_INTERRUPTOFF(void);
//
//	Tells memory allocator to mark the memory as being held.
//	In so doing when the program is in virtual memory mode, it marks
//	blocks of memory as being held.  This makes access at interrupt time,
//	completely safe when in virtual memory mode.
//
void					PNMM_HOLDMEMORY_ON(void);
void					PNMM_HOLDMEMORY_OFF(void);
HXBOOL					HXMM_VIRTUAL_MEMORY_ON(void);


//
//	Allows the user to set a bare minimum which must be left in the application's heap.
//
void					PNMM_SETAPPLICATIONMINIMUM(size_t size);

//
//	Debugging use only.
//
void					HXMM_POOLCHECK(void);
void					PNMM_OUTPUTMEMSTATS(void);
void 					PNMM_ASSERTPTR(void*	ptr);
void					PNMM_DUMP_ALLOCATED_BLOCKS(void);


//
//	FEATURE CHECKING
//

ULONG32	 				PNMM_VERSION(void);

//
//	Low-memory notification
//

HXBOOL					PNMM_LOWMEMORY(void);
void					PNMM_REGISTER_NOTIFICATION_FUNCTION(LOWMEMORYNOTIFIER* theNotifier);
void					PNMM_UNREGISTER_NOTIFICATION_FUNCTION(LOWMEMORYNOTIFIER* theNotifier);

//
//	INTERNAL USE ONLY
//

void  PNMM_INIT(void);
void  PNMM_SHUTDOWN(void);



#ifdef __cplusplus
}
#endif



