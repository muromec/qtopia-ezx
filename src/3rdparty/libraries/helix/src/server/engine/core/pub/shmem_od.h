/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: shmem_od.h,v 1.2 2003/01/23 23:42:55 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _SHMEM_H_
#define _SHMEM_H_
#define OGREDEBUG

#include <string.h>

#include "shregion.h"
#include "debug.h"
#include "malloc.h"
#include "hxassert.h"

#ifdef OGREDEBUG
#include "hxmap.h"
#include <stdio.h>
#include <signal.h>
#endif

#ifdef WIN32 
#define hx_malloc_init()
#define hx_malloc(x) malloc(x)
#define hx_free(x) free(x)
#endif

#define SHARED_REGION_SIZE	(1024 * 1024 * 128)	// 128 Meg

#ifdef OGREDEBUG
extern "C" char* get_trace();
extern "C" void** get_stack(int*);
extern "C" char* get_trace_from_stack(void**, int);

class AllocTracker {
public:
    AllocTracker(void* p, UINT32 s, void** stk, int si, BOOL st) :
	ptr(p),
	size(s),
	startup(st)
    {
	if(stk)
	{
	    stacksize = si;
	    stack = new void*[si];
	    memcpy(stack, stk, si * sizeof(void*));
	}
	else
	{
	    stack = NULL;
	}
    };

    void* ptr;
    UINT32 size;
    void** stack;
    int stacksize;
    BOOL startup;
};

extern CHXMapPtrToPtr* alloc_list;
extern BOOL g_allocating_tracker;
extern BOOL g_freeing_tracker;
extern BOOL g_first_check;
#endif

class SharedMemory {
public:
			    	    ~SharedMemory();
    static void		    	    Create();
    static void*	    	    malloc(int size);
    static void		    	    free(char *ptr);
    static int		    	    getpagesize();
    static void*	    	    sbrk(int amt);

    static UINT32		    NumMalloc()
					{ return *self->num_malloc;      }
    static UINT32		    NumFree()
					{ return *self->num_free;        }
    static UINT32		    BytesInUse()
					{ return *self->bytes_in_use;    }
    static UINT32		    BytesAllocated()
					{ return *self->bytes_allocated; }
#ifdef OGREDEBUG
    static void                     checkguards();
    static void                     checkleaks();
#endif
private:
			    	    SharedMemory();
    static SharedMemory*    	    self;
    SharedRegion*	    	    region;
    UINT8* VOLATILE * VOLATILE	    end_of_used_space;	// value is *(end...)
    UINT32* VOLATILE		    num_malloc;
    UINT32* VOLATILE		    num_free;
    UINT32* VOLATILE		    bytes_in_use;
    UINT32* VOLATILE		    bytes_allocated;
};

inline
SharedMemory::SharedMemory()
{
#ifndef WIN32
    region = new SharedRegion(SHARED_REGION_SIZE);
    end_of_used_space = (UINT8 **)region->region();

    *end_of_used_space = region->region();
    *end_of_used_space += sizeof(UINT8*);

    num_malloc = (UINT32*)*end_of_used_space;
    *num_malloc = 0;
    *end_of_used_space += sizeof(UINT32);

    num_free = (UINT32*)*end_of_used_space;
    *num_free = 0;
    *end_of_used_space += sizeof(UINT32);

    bytes_in_use = (UINT32*)*end_of_used_space;
    *bytes_in_use = 0; //XXXSMP
    *end_of_used_space += sizeof(UINT32);

    bytes_allocated = (UINT32*)*end_of_used_space;
    *bytes_allocated = 0; //XXXSMP
    *end_of_used_space += sizeof(UINT32);
#endif
}

inline void
SharedMemory::Create()
{
#ifdef DEBUG
    if (self)
	PANIC(("Can only have one instance of SharedMemory!\n"));
#endif
    self = new SharedMemory;
    hx_malloc_init();
}


inline
SharedMemory::~SharedMemory()
{
#ifndef WIN32
    delete region;
#endif
}

inline void*
SharedMemory::malloc(int size)
{
#ifndef WIN32
#ifdef DEBUG
    size = size + 20;
#endif
    char* ret = (char *)hx_malloc(size);
#ifdef DEBUG
#ifdef OGREDEBUG
    
    if(!alloc_list && !g_allocating_tracker)
    {
	g_allocating_tracker = TRUE;
	alloc_list = new CHXMapPtrToPtr;
	alloc_list->InitHashTable(3011);
	g_allocating_tracker = FALSE;
    }

    if(!g_allocating_tracker)
    {
	g_allocating_tracker = TRUE;
	int stacksize;
	void** stack = get_stack(&stacksize);
	AllocTracker* track = new AllocTracker(ret, size, 
					       stack, stacksize,
					       FALSE);
	(*alloc_list)[ret] = track;
	g_allocating_tracker = FALSE;
    }
#endif
    memset(ret, 0xd0, size);
    memset(ret, '$', 10);
    memset(ret - 10 + size, '&', 10);
    memcpy(ret, &size, sizeof(int));
    ret += 10;
#endif
    (*self->num_malloc)++;
    DPRINTF(D_ALLOC, ("Malloc() %p [%d bytes]\n", ret, size));
    (*self->bytes_in_use) += size;

    return ret;
#else
    return 0;
#endif
}

inline void
SharedMemory::free(char *ptr)
{
#ifndef WIN32
    int size = 0;
    static char* pDollar = "$$$$$$$$$$";
    static char* pAmp    = "&&&&&&&&&&";

#ifdef DEBUG
    ptr -= 10;
#ifdef OGREDEBUG
    CHXMapPtrToPtr::Iterator i;
    if(!g_freeing_tracker)
    {
	AllocTracker* track = 0;

	if(alloc_list->Lookup(ptr, (void*&)track))
	{
	    g_freeing_tracker = TRUE;
	    track->ptr = 0;
	    alloc_list->RemoveKey(ptr);
	    delete [] track->stack;
	    delete track;
	    g_freeing_tracker = FALSE;
	    break;
	}
    }
#endif
    memcpy(&size, ptr, sizeof(int));
    ASSERT(!memcmp(ptr + sizeof(int), pDollar, 10 - sizeof(int)));
    ASSERT(!memcmp(ptr + size - 10, pAmp, 10));
    memset(ptr, 0xcd, size);
#endif
    hx_free(ptr);
    DPRINTF(D_ALLOC, ("Free() %p [%d bytes]\n", ptr, size));

    (*self->num_free)++;
    (*self->bytes_in_use) -= size;
#endif
}

#ifdef OGREDEBUG
inline void
SharedMemory::checkguards()
{
    CHXMapPtrToPtr::Iterator i;
    static char* pDollar = "$$$$$$$$$$";
    static char* pAmp    = "&&&&&&&&&&";

    int checked = 0, badhead = 0, badtail = 0;
    for(i = alloc_list->Begin(); i != alloc_list->End(); ++i)
    {
	AllocTracker* track = (AllocTracker*)(*i);
	char* ptr = (char*)track->ptr;
	UINT32 size = track->size;

	if(g_first_check)
	    track->startup = TRUE;

	if(ptr)
	{
	    checked++;
	    if(memcmp(ptr + sizeof(int), pDollar, 10 - sizeof(int)) != 0)
	    {
		badhead++;
		printf("Bad head guard at %p (size %ld)\nStack:\n%s", 
		       ptr, size,
		       get_trace_from_stack(track->stack,
					    track->stacksize));
	    }
	    if(memcmp(ptr + size - 10, pAmp, 10) != 0)
	    {
		badtail++;
		printf("Bad tail guard at %p (size %ld)\nStack:\n%s", 
		       ptr, size, 
		       get_trace_from_stack(track->stack,
					    track->stacksize));
	    }
	}
    }
    g_first_check = FALSE;
    printf("Checked %d blocks, found %d bad heads, %d bad tails\n",
	   checked, badhead, badtail);
}

inline void
SharedMemory::checkleaks()
{
    CHXMapPtrToPtr::Iterator i;
    for(i = alloc_list->Begin(); i != alloc_list->End(); ++i)
    {
	AllocTracker* track = (AllocTracker*)(*i);

	if(!track->startup)
	    printf("======================================================================\n%s======================================================================\n",
		   get_trace_from_stack(track->stack,
					track->stacksize));

    }
}
#endif

inline int
SharedMemory::getpagesize()
{
    return 4096;
}

inline void*
SharedMemory::sbrk(int amt)
{
#ifndef WIN32
    DPRINTF(D_ALLOC, ("Malloc() requested %d bytes\n", amt));
    void* out = *self->end_of_used_space;
    (*self->end_of_used_space) += amt;
    (*self->bytes_allocated) += amt;

    ASSERT(*self->end_of_used_space < (self->region->region()
	+ self->region->size()));

    return out;
#else
    // need to return something!
    return NULL;
#endif
}
#endif

