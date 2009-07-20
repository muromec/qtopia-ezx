/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: shmem.h,v 1.10 2007/03/05 23:24:06 atin Exp $ 
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

/*
 * Setup defines for heap level debugging.
 *      PAULM_ALLOCTRACK tracks allocs/frees, its on by default in unix builds
 *      that can support it. In windows this is not used because alloc tracking
 *      in the Win32 server still uses _CrtSetAllocHook.
 *
 *      HEAP_VALIDATE checks guard bits for validity after each alloc/free.
 *
 */
#if defined DEBUG
#if defined _UNIX
#if defined _IRIX
#define HEAP_VALIDATE
#elif defined _FREEBSD || defined _OPENBSD || defined _NETBSD || \
    defined _LINUX || defined _SUN || defined _SOLARIS || defined _HPUX || \
    defined _OSF1 || defined _AIX || defined _MAC_UNIX
#ifndef PAULM_ALLOCTRACK
#define PAULM_ALLOCTRACK
#define HEAP_VALIDATE
#endif
#endif
#elif defined _WIN32
#ifndef HEAP_VALIDATE
#define HEAP_VALIDATE
#endif
#ifndef PAULM_ALLOCTRACK
#define PAULM_ALLOCTRACK
#endif
#endif
#elif defined _UNIX    /* Release 'nix build, gets alloctrack, except for _IRIX */
#ifndef _IRIX
#ifndef PAULM_ALLOCTRACK
#define PAULM_ALLOCTRACK
#endif
#endif
#elif defined _WIN32
#ifndef PAULM_ALLOCTRACK
#define PAULM_ALLOCTRACK
#endif
#endif

/*
 * If PAULM_ALLOCTRACK is defined that implies PAULM_LEAKCHECK
 */
#ifdef PAULM_ALLOCTRACK
 #define PAULM_LEAKCHECK
#endif

#ifdef _LINUX
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#if defined HEAP_VALIDATE
#define MEM_GUARD_SIZE 8       /* Bytes */
#endif

/*
 * If _EFENCE_ALLOC is on, don't HEAP_VALIDATE.  Guard bits and EFENCE
 * don't mix.  Turn of ALLOC_TRACK too, as this uses a lot of mem and
 * you probably won't use it with efence.
 */
#if defined _EFENCE_ALLOC
#if defined HEAP_VALIDATE
#undef HEAP_VALIDATE
#endif
#if defined PAULM_ALLOC_TRACK
#undef PAULM_ALLOC_TRACK
#endif
extern int g_backfence_alloc;
extern int g_efence_alloc;
extern BOOL g_bEFenceProtectFreedMemory;
#endif

/*
 * End of heap level debugging defines.
 */

// Other defines
/*
 * On Solaris/HPUX, we must return 8 byte aligned memory locations because 
 * they may be used for (long long int*).
 */
#if defined _SUN || defined _SOLARIS || defined _IRIX || defined _HPUX || defined _OSF1
#define NEED_8_BYTE_ALIGNMENT
#endif

// This works for current platforms.  Add a real build flag if needed.

#if defined _LONG_IS_64 && !defined EIGHT_BYTE_POINTERS
#define EIGHT_BYTE_POINTERS
#endif

#if defined MEMORY_SCRIBBLE_VALIDATE_ENABLED || defined _EFENCE_ALLOC
#define MEMORY_VALIDATE_ABORT_ON_FAILURE 0x0001
#define MEMORY_VALIDATE_CHECK_INTERNALS  0x0002

#define MEMORY_BLOCK_IN_USE		0x0001
#define MEMORY_BLOCK_IN_USE_MEMCACHE	0x0002
#define MEMORY_BLOCK_GUARDS_SET		0x0004

#define MEMORY_PAGE_PROTECTED		0x0001
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED

#if defined NEED_8_BYTE_ALIGNMENT
#define ALIGNMENT_PADDING(foo) ((foo & 0x7) ? ((foo & 0x7) ^ 0x7) + 1 : 0)
#else
#define ALIGNMENT_PADDING(foo) 0
#endif

// Includes

#include <string.h>

#include "shregion.h"
#include "debug.h"
#include "malloc.h"
#include "hxassert.h"
#include "hxmarsh.h"
#include "timeval.h"
#include "proc.h"

#if defined MEMORY_SCRIBBLE_VALIDATE_ENABLED || defined _EFENCE_ALLOC
#include "mutex.h"
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED || _EFENCE_ALLOC

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef PAULM_ALLOCTRACK
#include "hxmap.h"
#include <stdio.h>
#include <signal.h>
#endif

#ifdef _EFENCE_ALLOC
#include <sys/mman.h>
#endif

#ifdef NEED_8_BYTE_ALIGNMENT
   #define BYTE_ALIGNMENT 8 
   #define BYTE_ALIGN_SIZE(mem_size) ( (mem_size) + (BYTE_ALIGNMENT - (mem_size))) 
#else
   #define BYTE_ALIGN_SIZE(mem_size) mem_size
#endif 

#ifdef PAULM_ALLOCDUMP
#include "allocdump.h"
#ifdef PAULM_ALLOCDUMP_HERE
FILE* g_fp_alloc = 0;
INT32 g_allocnum = 0;
INT32 g_freenum = 0;
BOOL m_bAllocDump = 0;
#endif
#endif

#if defined _SUN || defined _SOLARIS || defined _OSF1
#define malloc_pagemask        ((8192)-1)
#define _PAGESIZE		8192
#define PAGESIZE_OFFSET		13
#define LAYOUT_BUCKET_SIZE(x) get_size_from_bucket(x)
#else
#define malloc_pagemask        ((4096)-1)
#define _PAGESIZE		4096
#define PAGESIZE_OFFSET		12
#define LAYOUT_BUCKET_SIZE(x) get_size_from_bucket(x)
#endif

#define pageround(foo) (((foo) + (malloc_pagemask))&(~(malloc_pagemask)))

#define SHMEM_NUM_BUCKETS		400
#define LAST_BUCKET			399
    // 256 = (65536*16) / 4096 (= [max allocation] / [min pagesize])
    // (That is, normal buckets will fit on allocations of 256 contiguous pages.
    //  The 257th list holds all the oversize pages.)
#define MAX_PAGE_SIZE_PAGES		257

#if defined _UNIX
#define SHARED_REGION_SIZE	(1024 * 1024 * 256)	// 256 Meg
#elif defined _WIN32
#define SHARED_REGION_SIZE	32768
#endif

extern unsigned long* g_pBucketToSize;
extern unsigned long* g_pSizeToBucket;
extern float g_AllocCoefficient;




inline int
get_size_from_bucket(int bucket)
{
    return g_pBucketToSize[bucket];
}

inline int
get_bucket_from_size(int size)
{
    return g_pSizeToBucket[size >> 4];
}


class _InUsePage;

class _Block
{
public:
    _InUsePage*		pPageHeader;
#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    UINT32		pBlockFlags;
#else
#if defined NEED_8_BYTE_ALIGNMENT && !defined EIGHT_BYTE_POINTERS
    UINT32		ulLongLongIntsMustBe8ByteAlignedOnSolaris;
#endif
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED
};

class _FreeBlock : public _Block
{
public:
    /* 
     * These data extend into the user's memory block.  As long as (summed)
     * they're no more than the smallest bucket (16 bytes), we're okay.
     */
    _FreeBlock*    pNextInBucket;		// (Mutex: BucketLock[])
    _FreeBlock*    pPrevInBucket;		// (Mutex: BucketLock[])
};

// A bucket-ized (allocated) page starts with an _InUsePage;
// a free page starts with a _FreePage.
// (A "page" is possibly a contiguous block of multiple physical pages, if
// required by the buffer size.)
// An _InUsePage/_InUseBigPage must also be 8-byte aligned on Solaris.
// (Note that each struct is 4-byte padded by the compiler.)

class _Page
{
public:
    _Page*		pPageNext;		// (Mutex: FreeListLock)
    _Page*		pPagePrev;		// (Mutex: FreeListLock)

    UINT16		ulBucketNumber;		// 0 => free page
    UINT16		ulBlocksInUse;		// Really part of InUsePage,
						//  but, due to padding, we save 
						//  space by tucking it in here.
#if defined MEMORY_SCRIBBLE_VALIDATE_ENABLED || defined _EFENCE_ALLOC
    HX_MUTEX_TYPE	mValidateGuardLock;
    UINT32		ulPageFlags;
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED || _EFENCE_ALLOC
    // (size mod 8) = 4 bytes in this struct
};

class _InUsePage : public _Page
{
public:
    _FreeBlock*		pLastFreeBlock;
    tv_sec_t		ulTimestamp;		// 4 bytes

    // (size mod 8) = sizeof (void *) in this struct

#if defined NEED_8_BYTE_ALIGNMENT && !defined EIGHT_BYTE_POINTERS
    UINT32		ulLongLongIntsMustBe8ByteAlignedOnSolaris;
#endif
};

// "Big" pages aren't bucket-ized: they are one-offs given for any request
// bigger than the largest bucket.  Hence, we also remember its size (in pages).

class _InUseBigPage : public _InUsePage
{
public:
    UINT32		ulNumPages;
#if defined NEED_8_BYTE_ALIGNMENT
    UINT32		ulLongLongIntsMustBe8ByteAlignedOnSolaris;
#endif
};

// We don't care about the alignment of free pages (we don't use long
// longs internally)

class _FreePage : public _Page
{
public:
    UINT32		ulNumPages;

    // Free pages are linked with others of the same size.
    _FreePage*		pFreePagePrev;		// (Mutex: FreeListLock)
    _FreePage*		pFreePageNext;		// (Mutex: FreeListLock)
};


#ifdef AREA51_ENABLED
// Area51PageHeader is for debugging only.
// Such pages are tracked separate from the rest of the memory system.

class Area51PageHeader
{
public:
    Area51PageHeader*	pNextPage;
    BOOL		bInUse;
#if defined NEED_8_BYTE_ALIGNMENT && defined EIGHT_BYTE_POINTERS
    UINT32		ulLongLongIntsMustBe8ByteAlignedOnSolaris;
#endif
};
#endif //AREA51_ENABLED

#ifdef PAULM_ALLOCTRACK
extern "C" char* get_trace();
extern "C" char* get_trace_from_stack(void**, int);
#ifndef _HPUX
extern "C" void** get_stack(int*);
#endif

class AllocTracker {
public:
    AllocTracker(void* p, UINT32 s, void** stk, INT32 si, BOOL st) :
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
    INT32 stacksize;
    BOOL startup;
};

extern CHXMapPtrToPtr* alloc_list;
extern BOOL g_allocating_tracker;
extern BOOL g_freeing_tracker;
extern BOOL g_first_check;

#endif

extern UINT32* g_pSizes;
extern BOOL    g_bDoSizes;
extern UINT32* volatile g_pTotalMemUsed;
extern UINT32* g_pPageFrees;
extern UINT32* g_pPagesAllocated;
extern UINT32* g_pFreeListEntriesSearched;
extern INT32* g_pFreePagesOutstanding;
extern UINT32* g_pShortTermAlloc;
extern UINT32* g_pShortTermFree;
extern BOOL g_bFastMalloc;
extern BOOL g_bCrashAvoidancePrint;


class SharedMemory {
public:
			    	    ~SharedMemory();
    static void		    	    Create();
    static void*	    	    malloc(INT32 size);
    static void*	    	    calloc(INT32 num, INT32 size);
    static void*	    	    realloc(char* ptr, INT32 size);
    static void		    	    free(char *ptr);
    inline static INT32	    	    getpagesize() {return _PAGESIZE;};
    static void*	    	    sbrk(UINT32 amt);
    static INT32	    	    checkblock(char* ptr);
    static INT32	    	    blocksize(char* ptr);
    inline static size_t            getsize(char* ptr);

    inline static void              SetTimePtr(Timeval* pNow)
					{ *self->m_pNow = pNow; }
    static UINT32		    ReclaimByAge(INT32 age);
    static UINT32		    ReclaimByAge(INT32 age, INT32 bucket);

    inline static UINT32            NumMalloc()
                                        { return *self->num_malloc;       }
    inline static UINT32            NumFastMalloc()
                                        { return *self->num_fastmalloc;   }
    inline static UINT32            NumFree()
                                        { return *self->num_free;         }

    inline static UINT32            BytesInUse()
                                        { return *self->bytes_in_use;     }
    inline static UINT32            UpdateBytesInUse()
    {
        /*
         * This uses a "distributed counter" to be used for tracking memory
         * in use rather than having multiple processes updating a single int.
         * This provides the same accuracy as a mutex-protected counter without
         * the high overhead that a mutex would introduce in such a rapidly
         * changing value.  UpdateBytesInUse() needs to be called periodically
         * to update the aggregate counter that (for performance reasons)
         * BytesInUse() returns.
         */
        register UINT32 ulSum = 0;
        register UINT32 ulNumProcs = Process::numprocs();
        for (register UINT32 i=0; i < ulNumProcs; i++)
        {
            ulSum += self->bytes_in_use_counters[i];
        }
        *self->bytes_in_use = ulSum;
        return ulSum;
    }
    inline static UINT32            BytesAllocated()
                                        { return *g_pTotalMemUsed;        }
#if defined _OSF1
    inline static UINT64            BytesInPool()
                                        { return self->region->size();    }
#else
    inline static UINT32            BytesInPool()
                                        { return self->region->size();    }
#endif
#ifdef PAULM_ALLOCTRACK
    static void                     checkguards();
    static void                     checkleaks();
    static void                     resetleaks();
    static void			    suspendleaks();
    static void                     setogredebug(BOOL);
#else
    static void                     checkguards() {};
    static void                     checkleaks() {};
    static void                     resetleaks() {};
    static void			    suspendleaks() {};
    static void                     setogredebug(BOOL set) {};
#endif
    static void			    reportsizes();
    static void			    resetsizes();

#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    static HX_RESULT		    ValidateMemory(INT32 lStartPage, INT32 lPages, 
						   UINT32 ulFlags);
    static int			    ValidateSinglePage(_InUsePage* pPage, 
						       INT32 nPageNum, UINT32 ulFlags);
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED

#ifdef AREA51_ENABLED
    // protected-memory services (development only)

    static void*		    Area51Alloc(INT32 size);
    static void			    Area51Free(char* ptr);
    static void			    Area51Protect(char* ptr);
    static void			    Area51UnProtect(char* ptr);
#endif //AREA51_ENABLED

private:
			    	    SharedMemory();
    static void			    ReclaimPage(_InUsePage* pPage);

    static void			    hx_malloc_init();
    static void			    hx_malloc_end();
    static void*		    hx_malloc(size_t nbytes);
    static void			    hx_free(void *cp);


    static SharedMemory*    	    self;

    SharedRegion*	    	    region;
    UINT8* VOLATILE * VOLATILE	    end_of_used_space;	// value is *(end...)
    UINT32* VOLATILE		    num_malloc;
    UINT32* VOLATILE		    num_fastmalloc;
    UINT32* VOLATILE		    num_free;
    UINT32* VOLATILE		    bytes_in_use_counters;
    UINT32* VOLATILE		    bytes_in_use;
    UINT32* VOLATILE		    bytes_allocated;
    UINT32* VOLATILE		    bytes_remaining;
    BOOL* VOLATILE		    in_first_heap;
    UINT32* VOLATILE		    second_heap_size;
    Timeval**		    	    m_pNow;
#ifdef PAULM_ALLOCTRACK
    BOOL* VOLATILE                  ogre_debug;
#endif
};

inline size_t
SharedMemory::getsize(char *ptr)
{
    INT32 size = 0;

#if defined HEAP_VALIDATE
    ptr -= MEM_GUARD_SIZE;
    memcpy(&size, ptr, sizeof(INT32));
    size -= 2 * MEM_GUARD_SIZE;

    return size;
#endif
    ptr -= sizeof(INT32) * 2;
    size = getlong((BYTE*)ptr);
    size -= sizeof(INT32) * 2;

    return size;
}

#endif
