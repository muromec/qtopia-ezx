/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: malloc.cpp,v 1.24 2007/03/05 23:24:05 atin Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#ifdef PAULM_ALLOCDUMP
#define PAULM_ALLOCDUMP_HERE
#endif
#include "proc.h"
#include "shmem.h"
#include "debug.h"
#include "hxassert.h"
#include "mutex.h"
#include "trace.h"


#ifdef _WIN32
#  include "hlxclib/windows.h"
#endif // _WIN32

extern BOOL* volatile g_pbTrackAllocs;
extern BOOL* volatile g_pbTrackFrees;
extern BOOL g_bLeakCheck;
extern BOOL g_bNoAutoRestart;

//#define JMEVISSEN_PRINTF
//#define SHMEM_VERIFY
//#define SHMEM_ASSERT(x) ASSERT(x)
#define SHMEM_ASSERT(x)

UINT32* volatile g_pTotalMemUsed;         /* what to you think? */
_FreeBlock** g_pBuckets;
UINT32* g_pPageFrees;
UINT32* g_pPagesAllocated;
UINT32* g_pFreeListEntriesSearched;
INT32* g_pFreePagesOutstanding;         // Mutex: FreeListLock
UINT32* g_pShortTermAlloc;
UINT32* g_pShortTermFree;

UINT16* g_pBucketBlocksPerPage;
UINT16* g_pBucketPageSize;
_Page** g_pFirstPage;                   // Only init'd and used by ValidateMemory()
_Page** g_pLastPage;                    // Mutex: FreeListLock
_FreePage** g_pFreePageListBySize;      // Mutex: FreeListLock

extern void* g_pSecondHeapBottom;
#ifdef _LINUX
extern void* g_pSbrkHeapBottom;
extern void* g_pSbrkHeapTop;
extern int   shared_ready;
#endif // _LINUX


#ifdef AREA51_ENABLED
Area51PageHeader** g_pArea51FreePages;  // Mutex: Area51Lock
HX_MUTEX g_pArea51Lock;
#endif //AREA51_ENABLED

HX_MUTEX g_pAllocLock;
unsigned long* g_pBucketToSize;
unsigned long* g_pSizeToBucket;

// To avoid deadlock, please grab locks according to the lock level below.
// (That is, don't attempt to grab a lower-number lock while holding a
//  higher-number lock.  You don't NEED to have any lower-number locks
//  to grab a higher number; that only depends on your operation.)
// shmem.h has some hints on which linked-list pointers in _Page, etc, are
// protected by which mutexes.
// (This is less of an issue since I dropped the reclaim list and its lock.)

HX_MUTEX g_pBucketLocks;                // Level 1
HX_MUTEX g_pFreeListLock;               // Level 2
HX_MUTEX g_pSbrkLock;                   // Level 3

#ifdef PAULM_ALLOCTRACK
CHXMapPtrToPtr* alloc_list = NULL;
BOOL g_allocating_tracker = FALSE;
BOOL g_freeing_tracker = FALSE;
BOOL g_first_check = TRUE;
HX_MUTEX g_pOgreLock;
volatile int* g_pLockedPid;
#endif

BOOL RestartOrKillServer();
void RestartOnFault(BOOL b);

// I seem to have deleted the functions that used to call these
// (inside a SHMEM_VERIFY ifdef, of course); but you can add them if
// changing low-level memory again.

static void _ValidateBuckets();
static void _ValidateFreeLists();
static void _ValidatePageList();

#ifdef MEMORY_CORRUPTION_CHECK

/* XXXJJ This is a technique for debugging memory corruption. Here is how it
works:

1. At the beginning, allocate a big chunk of memory(default is 100MB, but you
can alway increase it if you need to debug a very busy bucket.

2. In SharedMemory::free, for the bucket you are instersted, put the stacks
somewhere in that memory you preallocted, save the address in the block(You
can't save it in the first 8 bytes, which are used by hx_free for block
infomation. So I put the address in 9th-12th bytes from the beginning.

 -------
 | size|   ==> 4 bytes
 -------
 -------
 |guard|   ==> 4 bytes
 -------
 -------
 |addr |   ==> put the address of the stack here.
 -------

In addition, disable guard checking.

Evtually you will use up all preallocated memory, then you need to start over
from the beginning and overwrite the old stacks.  That time should be long
enough for the bad buy to come up: for bucket 4, it takes 20 minutes to use
up all 100MB in normal uptime test.

3. In sharedMemory::malloc, put the address in the guard position.

4. In you code, if you encounter memory corruption, the 4 bytes before "this"
pointer is the stack of previous owner of this piece of memory, hopefully it
is the culprit. Print the stack out, resolve it and good luck!!!

PropWatchCallback::~PropWatchCallback()
{
    if(!m_registry)
    {
       UINT32**  pStack = (UINT32**)((UINT32*)this - 1);
        if(*pStack)
        fprintf(stderr, "The corruptor stack  is\n0x%x\n0x%x\n0x%x\n0x%x\n0x"
        "%x\n0x%x\n0x%x\n0x%x\n",
       *(*pStack), *(*pStack+1), *(*pStack+2), *(*pStack+3),*(*pStack+4),
       *(*pStack+5),*(*pStack+6),*(*pStack+7));
    }

This technique has been used to catch a memory corruption setting m_registry
to NULL.

Conclusion: This technique can be used where the load to repro the bug too
heavy for efence build to run. get_stack is not expensive so the impact on
perfomance is minimal compared to efence.
*/

//here we watch bucket #4, you need to change it to you bucket
#define MEMORY_CORRUPTION_CHECK_MIN 48
#define MEMORY_CORRUPTION_CHECK_MAX 64
//the default size of the array is 100mb, you can alway change it
#define MEMORY_CORRUPTION_CHECK_BLOCK_SIZE 100 //megabytes

UINT32 * g_mcStacksArray = NULL;
UINT32 * g_mcStacksIndex = NULL;
#endif //MEMORY_CORRUPTION_CHECK

////////////////////////////////////////////////////////////////////////
//                      _SplitFreePage
////////////////////////////////////////////////////////////////////////
//
// Description:         Helper function to split one free page into two,
//                      with the caveat that one of them is to go into
//                      immediate use (and so is not put into any free-page
//                      lists).
//
//                      Caller should already have the free-page mutex.
//
// Parameters:
//              pFree           Pointer to existing free page
//              numPages        Pages needed
//
// Returns:
//              pPage           Pointer to split-off non-free page
//
// Implementation:
//
// Revisions:
//      jmevissen       11/16/2000      Initial version
//
////////////////////////////////////////////////////////////////////////
static _Page*
_SplitFreePage(_FreePage* pFree, UINT32 numPages)
{
#ifdef JMEVISSEN_PRINTF
    printf("jmev: splitting free page of size %d at %lx\n",
           pFree->ulNumPages, pFree);
#endif

    _FreePage* pFreeRemaining = (_FreePage*)((PTR_INT)pFree +
                                             (numPages << PAGESIZE_OFFSET));

    SHMEM_ASSERT(pFree->ulNumPages >= numPages);

    // is it really being split?

    if (pFree->ulNumPages > numPages)
    {
        // fix master page list
        pFreeRemaining->pPageNext = pFree->pPageNext;
        if (pFreeRemaining->pPageNext)
        {
            pFreeRemaining->pPageNext->pPagePrev = pFreeRemaining;
        }
        else
        {
            SHMEM_ASSERT(*g_pLastPage == pFree);
            *g_pLastPage = pFreeRemaining;
        }
        pFreeRemaining->pPagePrev = pFree;
        pFree->pPageNext = pFreeRemaining;
    }

    // remove original free page from old free list

    UINT32 oldNumPages = pFree->ulNumPages;
    UINT32 newNumPages = oldNumPages - numPages;
    int sizeInd = (int) min(oldNumPages, MAX_PAGE_SIZE_PAGES);

    if (pFree->pFreePagePrev)
    {
        pFree->pFreePagePrev->pFreePageNext = pFree->pFreePageNext;
    }
    else
    {
        SHMEM_ASSERT(g_pFreePageListBySize[sizeInd] == pFree);
        g_pFreePageListBySize[sizeInd] = pFree->pFreePageNext;
    }
    if (pFree->pFreePageNext)
    {
        pFree->pFreePageNext->pFreePagePrev = pFree->pFreePagePrev;
    }

    // add remaining free page to correct-size list

    if (newNumPages)
    {
        sizeInd = (UINT16) min(newNumPages, MAX_PAGE_SIZE_PAGES);

        pFreeRemaining->ulBucketNumber = 0;
        pFreeRemaining->ulNumPages = newNumPages;

        pFreeRemaining->pFreePagePrev = NULL;
        pFreeRemaining->pFreePageNext =
            g_pFreePageListBySize[sizeInd];
        if (g_pFreePageListBySize[sizeInd])
        {
            g_pFreePageListBySize[sizeInd]->pFreePagePrev =
                pFreeRemaining;
        }
        g_pFreePageListBySize[sizeInd] = pFreeRemaining;
    }

#ifdef JMEVISSEN_PRINTF
    printf("jmev: splitting free page\n");
    printf("   oldsize %d, free page ptr %lx\n", oldNumPages,
           g_pFreePageListBySize[oldNumPages]);
    printf("   newsize %d, free page ptr %lx\n", newNumPages,
           g_pFreePageListBySize[newNumPages]);
    if (newNumPages)
    {
        printf("   Remaining page at %lx\n", pFreeRemaining);
        printf("   PagePrev %lx, PageNext %lx\n", pFreeRemaining->pPagePrev,
               pFreeRemaining->pPageNext);
        printf("   FreePagePrev %lx, FreePageNext %lx\n",
               pFreeRemaining->pFreePagePrev, pFreeRemaining->pFreePageNext);
    }
#endif

    return (_Page*)pFree;
}


inline void
do_size_function()
{
    unsigned long i, j;
    unsigned long thresh = 1504 >> 4;
    for (i = 0; i < SHMEM_NUM_BUCKETS; i++)
    {
        if (i <= thresh)
        {
            g_pBucketToSize[i] = i << 4;
        }
        else
        {
            j = i - thresh;
            g_pBucketToSize[i] = (thresh <<4) +
                ((int)(j * j * g_AllocCoefficient) << 4);
        }
    }

    // Run a separate bucket-init loop, in case LAYOUT_BUCKET_SIZE
    // maps to g_pBucketToSize[i+1].

    for (i = 0; i < SHMEM_NUM_BUCKETS; i++)
    {
        // How many contiguous pages required to allocate a block?

        g_pBucketPageSize[i] =
            pageround(sizeof(_InUsePage) +
            LAYOUT_BUCKET_SIZE(i) + sizeof(_Block)) / _PAGESIZE;

        // How many blocks allocated each time we get free memory?

        g_pBucketBlocksPerPage[i] =
          (g_pBucketPageSize[i]*_PAGESIZE - sizeof(_InUsePage)) /
            (LAYOUT_BUCKET_SIZE(i) + sizeof(_Block));
    }

    j = 0;
    i = 0;
    while (i < 65536)
    {
        while (i <= (g_pBucketToSize[j] >> 4) && i < 65536)
        {
            // bucket # == 0 indicates a free page, so it's good that
            // this never assigns a size to bucket zero.
            ASSERT(j+1 < SHMEM_NUM_BUCKETS);
            g_pSizeToBucket[i] = j + 1;
            i++;
        }
        j++;
    }

#if defined(_LINUX) && defined(SHMEM_VERIFY)
    //debug: write-protect pages within read-only area
    char* p = (char*)(((PTR_INT)g_pBucketToSize + malloc_pagemask) &
                      (~malloc_pagemask));
    char* pEnd = (char*)((PTR_INT)(g_pBucketPageSize + 398) &
                         (~malloc_pagemask));
    printf("jmev: protecting memory from %lx to %lx\n", p, pEnd);
    if (mprotect(p, (PTR_INT)pEnd-(PTR_INT)p, PROT_READ))
    {
        perror("WARNING: mprotect returned error");
    }
#endif
}


void
SharedMemory::hx_malloc_init()
{
    g_pBuckets = (_FreeBlock **)SharedMemory::sbrk(sizeof(void *) * SHMEM_NUM_BUCKETS);
    memset(g_pBuckets, 0, sizeof(void *) * SHMEM_NUM_BUCKETS);

    g_pPageFrees = (UINT32 *)SharedMemory::sbrk(sizeof(UINT32));
    g_pPagesAllocated = (UINT32 *)SharedMemory::sbrk(sizeof(UINT32));
    g_pFreeListEntriesSearched = (UINT32 *)SharedMemory::sbrk(sizeof(UINT32));
    g_pFreePagesOutstanding =  (INT32 *)SharedMemory::sbrk(sizeof(INT32));
    g_pShortTermAlloc =  (UINT32 *)SharedMemory::sbrk(sizeof(UINT32));
    g_pShortTermFree =  (UINT32 *)SharedMemory::sbrk(sizeof(UINT32));
    g_pAllocLock = (HX_MUTEX)SharedMemory::sbrk(sizeof(HX_MUTEX_TYPE));
#ifdef _LONG_IS_64
    //needed for pointer alignment on 64-bit platforms
    if (sizeof(HX_MUTEX) > sizeof(HX_MUTEX_TYPE))
    {
        SharedMemory::sbrk(sizeof(HX_MUTEX) - sizeof(HX_MUTEX_TYPE));
    }
#endif
    g_pBucketToSize = (unsigned long*)SharedMemory::sbrk(
            sizeof(unsigned long) * (65536 + SHMEM_NUM_BUCKETS));
    g_pSizeToBucket = g_pBucketToSize + SHMEM_NUM_BUCKETS;

    // blocks per page (eg, one page has N 16-byte blocks allocated on it)

    g_pBucketBlocksPerPage = (UINT16 *)
      SharedMemory::sbrk(SHMEM_NUM_BUCKETS * sizeof(UINT16));

    // pages per page group (ie, a 7856 byte block requires 2 contiguous pages)

    g_pBucketPageSize = (UINT16 *)
      SharedMemory::sbrk(SHMEM_NUM_BUCKETS * sizeof(UINT16));

    do_size_function();

    // free-page list, indexed by size (in pages) of the contiguous free memory

    g_pFreePageListBySize = (_FreePage **)
        SharedMemory::sbrk((MAX_PAGE_SIZE_PAGES+1) * sizeof(_FreePage*));
    memset(g_pFreePageListBySize, 0, (MAX_PAGE_SIZE_PAGES+1) * sizeof(_FreePage*));

    // global page list
    g_pFirstPage = (_Page**) SharedMemory::sbrk(sizeof(_Page*));
    *g_pFirstPage = NULL;
    g_pLastPage = (_Page**) SharedMemory::sbrk(sizeof(_Page*));
    *g_pLastPage = NULL;

#ifdef AREA51_ENABLED
    // Area51 free list (development only)
    g_pArea51FreePages = (Area51PageHeader**) sbrk(sizeof(Area51PageHeader*));
    *g_pArea51FreePages = NULL;
#endif //AREA51_ENABLED


    g_pBucketLocks = (HX_MUTEX)SharedMemory::sbrk(sizeof(HX_MUTEX_TYPE) * SHMEM_NUM_BUCKETS);
    g_pFreeListLock = (HX_MUTEX)SharedMemory::sbrk(sizeof(HX_MUTEX_TYPE));
    g_pSbrkLock = (HX_MUTEX)SharedMemory::sbrk(sizeof(HX_MUTEX_TYPE));
#ifdef AREA51_ENABLED
    g_pArea51Lock = (HX_MUTEX)SharedMemory::sbrk(sizeof(HX_MUTEX_TYPE));
    HXInitMutex(g_pArea51Lock);
#endif //AREA51_ENABLED
    *g_pPageFrees = 0;
    *g_pPagesAllocated = 0;
    *g_pFreeListEntriesSearched = 0;
    *g_pFreePagesOutstanding = 0;
    *g_pShortTermAlloc = 0;
    *g_pShortTermFree = 0;
    int i;
    for (i = 0; i < SHMEM_NUM_BUCKETS; i++)
    {
        HXInitMutex(&g_pBucketLocks[i]);
    }
    HXInitMutex(g_pAllocLock);
    HXInitMutex(g_pFreeListLock);
    HXInitMutex(g_pSbrkLock);

#ifdef MEMORY_CORRUPTION_CHECK
    g_mcStacksArray = (UINT32*)SharedMemory::sbrk(
                                MEMORY_CORRUPTION_CHECK_BLOCK_SIZE*1024*1024);
    g_mcStacksIndex = (UINT32*)SharedMemory::sbrk(sizeof(UINT32));
    *g_mcStacksIndex = 0;
#endif //MEMORY_CORRUPTION_CHECK

#ifdef PAULM_ALLOCTRACK
    g_pOgreLock = (HX_MUTEX)SharedMemory::sbrk(sizeof(HX_MUTEX_TYPE));
    g_pLockedPid = (int*)SharedMemory::sbrk(sizeof(int));
    HXInitMutex(g_pOgreLock);
    *g_pLockedPid = MAX_THREADS + 1;
#endif
}


void
SharedMemory::hx_malloc_end()
{
    printf ("Yipes End\n");
}


void *
SharedMemory::hx_malloc(size_t nbytes)
{
    *g_pShortTermAlloc += 1;

    // Special case: very big allocations get their own pages.
    // Bucket number for blocks between 65048 and 65535 (i.e for 
    // nbytes between than 1040768 and 1 Meg) is 339. And the 
    // number of free pages required to accommodate the maximum 
    // size of bucket 339, are 259 (according to g_pBucketPageSize[]). 
    // Since our g_pFreePageListBySize[] holds only 256 free pages
    // of normal size, we consider requests with block sizes greater
    // than 65047 as very big allocations. So, we handle such requests
    // differently by first looking in free page list at the 257th index
    // to find if we have enough free pages to accommodate this request.
    // If not, we proceed to allocate memory from the system.

    if (nbytes >> 4 > 65047)
    {
        size_t allocation = pageround(nbytes + sizeof(_InUseBigPage) + sizeof(_Block));
        UINT32 allocPages = allocation >> PAGESIZE_OFFSET;
        _InUseBigPage* pPage = NULL;

        HXMutexLock(g_pFreeListLock);

        // is there a big enough page in the free list already?

        _FreePage* pFree = g_pFreePageListBySize[MAX_PAGE_SIZE_PAGES];
        while (pFree)
        {
            ++(*g_pFreeListEntriesSearched);
            if (pFree->ulNumPages >= allocPages)
            {
                // big enough free page.

                pPage = (_InUseBigPage*) _SplitFreePage(pFree, allocPages);
                *g_pFreePagesOutstanding -= allocPages;
                break;
            }
            // else, keep looping
            pFree = pFree->pFreePageNext;
        }

        // allocate new memory if no pre-existing free page
        if (!pPage)
        {
            HXMutexLock(g_pSbrkLock);
            pPage = (_InUseBigPage*)(SharedMemory::sbrk(allocation));
            HXMutexUnlock(g_pSbrkLock);

            // link newly-allocated page into address-sorted global page list

            if (*g_pLastPage) (*g_pLastPage)->pPageNext = (_Page*)pPage;
            pPage->pPagePrev = *g_pLastPage;
            pPage->pPageNext = NULL;
            *g_pLastPage = (_Page*)pPage;
        }

        *g_pPagesAllocated += allocPages;

        pPage->ulBlocksInUse = 1;
        pPage->ulBucketNumber = LAST_BUCKET;
        pPage->pLastFreeBlock = NULL;
        pPage->ulNumPages = allocPages;

        _Block* pBlock = (_Block*)((PTR_INT)pPage + (int)sizeof(_InUseBigPage));

        pBlock->pPageHeader = (_InUsePage*) pPage;
        HXMutexUnlock(g_pFreeListLock);

        return (void *)((PTR_INT)pBlock + (int)sizeof(_Block));
    }

    // Normal case: use the pre-allocated buckets

    int bucket = get_bucket_from_size(nbytes);
    SHMEM_ASSERT(g_pBucketBlocksPerPage[bucket]);

    HXMutexLock(&g_pBucketLocks[bucket]);

    // If no block available, lay out a new page from either the free list
    // or from new (sbrk-returned) memory.

    if (g_pBuckets[bucket] == NULL)
    {
        _InUsePage* pPage;
        UINT32 ulAdjBytes = LAYOUT_BUCKET_SIZE(bucket);
        UINT32 ulSize = g_pBucketPageSize[bucket] << PAGESIZE_OFFSET;
        UINT32 ulPageInd = g_pBucketPageSize[bucket];

        // is there a free page of the correct size?

        HXMutexLock(g_pFreeListLock);

        _FreePage* pFree = g_pFreePageListBySize[ulPageInd];
        if (pFree)
        {
#ifdef JMEVISSEN_PRINTF
            printf("jmev: free page found at %lx, reusing.\n", pFree);
            printf("   PagePrev %lx, PageNext %lx\n", pFree->pPagePrev, pFree->pPageNext);
            printf("   FreePagePrev %lx, FreePageNext %lx\n", pFree->pFreePagePrev,
                   pFree->pFreePageNext);
#endif
            SHMEM_ASSERT(pFree->ulNumPages == ulPageInd);

            // Remove page from free list

            g_pFreePageListBySize[ulPageInd] = pFree->pFreePageNext;
            if (g_pFreePageListBySize[ulPageInd])
            {
                g_pFreePageListBySize[ulPageInd]->pFreePagePrev = NULL;
            }
        }

        // are there any free pages at all?

        else if (*g_pFreePagesOutstanding)
        {
            // search for a larger free page we can split off.
            // The final page list is guaranteed to hold pages larger than the
            // max needed by normal buckets.

            UINT32 pageInd = ulPageInd;
            while (++pageInd <= MAX_PAGE_SIZE_PAGES)
            {
                ++(*g_pFreeListEntriesSearched);

                if (g_pFreePageListBySize[pageInd])
                {
                    pFree = g_pFreePageListBySize[pageInd];
                    SHMEM_ASSERT(pageInd == MAX_PAGE_SIZE_PAGES ||
                           pFree->ulNumPages == pageInd);

                    pPage = (_InUsePage*)
                        _SplitFreePage(pFree,
                                       g_pBucketPageSize[bucket]);
                    break;
                }
            }
        }

        // did we find a free page to use?

        if (pFree)
        {
            pPage = (_InUsePage *)(pFree);
            *g_pFreePagesOutstanding -= g_pBucketPageSize[bucket];
            ASSERT(*g_pFreePagesOutstanding >= 0);
        }
        else
        {
#ifdef JMEVISSEN_PRINTF
            printf("jmev: requesting new sbrk memory.\n");
#endif
            // No, get a totally new one at the end of memory

            HXMutexLock(g_pSbrkLock);
            pPage = (_InUsePage *)SharedMemory::sbrk(ulSize);
            HXMutexUnlock(g_pSbrkLock);

            // link newly-allocated page into address-sorted global page list
            // (will only be address-sorted in big-block-of-shared-memory case,
            // but that's okay.

#ifdef _EFENCE_ALLOC
            BOOL bProtected = *g_pLastPage &&
                ((*g_pLastPage)->ulPageFlags & MEMORY_PAGE_PROTECTED);
            if (bProtected)
            {
                HX_ASSERT(!mprotect((char*) *g_pLastPage, _PAGESIZE,
                                    PROT_READ | PROT_WRITE));
            }
#endif // _EFENCE_ALLOC

            if (*g_pLastPage) (*g_pLastPage)->pPageNext = (_Page*)pPage;
            pPage->pPagePrev = *g_pLastPage;
            pPage->pPageNext = NULL;
            *g_pLastPage = (_Page*)pPage;

#ifdef _EFENCE_ALLOC
            pPage->ulPageFlags = 0;
            if (bProtected)
            {
                HX_ASSERT(!mprotect((char*)(pPage->pPagePrev), _PAGESIZE, PROT_READ));
            }

#endif // _EFENCE_ALLOC
        }

        *g_pPagesAllocated += g_pBucketPageSize[bucket];
        HXMutexUnlock(g_pFreeListLock);

        // Lay out unallocated blocks on page, and add them to bucket.
        // _InUsePage : _Block : memory : _Block : memory : ...

        // Page header init

        pPage->ulBlocksInUse = 0;
        pPage->ulBucketNumber = bucket;
#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
        HXMutexInit(&pPage->mValidateGuardLock);
#endif

        // _FreeBlock init

        _FreeBlock* pLast = NULL;
        _FreeBlock* pBlock = (_FreeBlock*)((PTR_INT)pPage + sizeof(_InUsePage));
        g_pBuckets[bucket] = pBlock;

        UINT16 numBlocks = g_pBucketBlocksPerPage[bucket];

        while (numBlocks--)
        {
#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
            pBlock->pBlockFlags = 0;
#endif
            pBlock->pPageHeader = pPage;

            pBlock->pPrevInBucket = pLast;
            pLast = pBlock;
            pBlock = pBlock->pNextInBucket =
                (_FreeBlock *)((char*)pBlock + ulAdjBytes + sizeof(_Block));
        }
        pLast->pNextInBucket = NULL;
        pPage->pLastFreeBlock = pLast;
    }

    // We have a block to allocate.

    _FreeBlock* pBlock = g_pBuckets[bucket];
    void* ret = (void *)((PTR_INT)pBlock + (int)sizeof(_Block));

    _InUsePage* pPage = pBlock->pPageHeader;
    ASSERT(pPage->ulBucketNumber == bucket);

#ifdef _EFENCE_ALLOC
    // In big allocations (only one block on the page group), we
    // have to unprotect the free memory we're giving out.

    if (g_bEFenceProtectFreedMemory && g_pBucketBlocksPerPage[bucket] == 1)
    {
        HX_ASSERT(0 == mprotect((char*)pPage, _PAGESIZE * g_pBucketPageSize[bucket],
                                PROT_READ | PROT_WRITE));
        pPage->ulPageFlags &= ~MEMORY_PAGE_PROTECTED;
    }
#endif

    // stamp the page with alloc time

    if (*self->m_pNow) pPage->ulTimestamp = (*self->m_pNow)->tv_sec;

    ++pPage->ulBlocksInUse;    // increment in-use block count

#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    HX_ASSERT(!(pBlock->pBlockFlags & MEMORY_BLOCK_IN_USE));
    pBlock->pBlockFlags |= MEMORY_BLOCK_IN_USE;
#endif

    // remove block from bucket list

    pBlock = pBlock->pNextInBucket;
    if (pBlock)
    {
#ifdef _EFENCE_ALLOC
        BOOL bProtected = g_bEFenceProtectFreedMemory &&
                          g_pBucketBlocksPerPage[bucket] == 1;
        if (bProtected)
        {
            // the block header will be on the first page, so _PAGESIZE is sufficient.
            HX_ASSERT(0 == mprotect((char*)(pBlock->pPageHeader), _PAGESIZE,
                                    PROT_READ | PROT_WRITE));
        }
#endif
        pBlock->pPrevInBucket = NULL;
#ifdef _EFENCE_ALLOC
        if (bProtected)
        {
            HX_ASSERT(0 == mprotect((char*)(pBlock->pPageHeader), _PAGESIZE, PROT_READ));
        }
#endif
    }
    g_pBuckets[bucket] = pBlock;

    HXMutexUnlock(&g_pBucketLocks[bucket]);
    return ret;
}

void
SharedMemory::hx_free(void *cp)
{
    *g_pShortTermFree += 1;

    _FreeBlock* pBlock = (_FreeBlock*)((PTR_INT)cp - (int)sizeof(_Block));
    _InUsePage* pPage = pBlock->pPageHeader;
    UINT32 ulBucketNumber = pPage->ulBucketNumber;

    SHMEM_ASSERT(g_pBucketBlocksPerPage[ulBucketNumber]);

    if (ulBucketNumber == LAST_BUCKET)
    {
        // Big pages are immediately reclaimed.  (There is no way
        // to fast-allocate it again.)

        SharedMemory::ReclaimPage(pPage);
        return;
    }

    HXMutexLock(&g_pBucketLocks[ulBucketNumber]);

#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    HX_ASSERT(pBlock->pBlockFlags & MEMORY_BLOCK_IN_USE);
    if (pBlock->pBlockFlags & MEMORY_BLOCK_IN_USE)
    {
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED

    // insert block next to page's other free blocks, if possible

    ASSERT(pPage->ulBlocksInUse);
    if (pPage->ulBlocksInUse < g_pBucketBlocksPerPage[ulBucketNumber])
    {
        // pLastFreeBlock is valid.  Add block after free block on page
        _FreeBlock* pFreeBlock = pPage->pLastFreeBlock;
        pBlock->pNextInBucket = pFreeBlock->pNextInBucket;
        pBlock->pPrevInBucket = pFreeBlock;
        pFreeBlock->pNextInBucket = pBlock;
        if (pBlock->pNextInBucket) pBlock->pNextInBucket->pPrevInBucket = pBlock;
    }
    else
    {
        ASSERT(pPage->ulBlocksInUse == g_pBucketBlocksPerPage[ulBucketNumber]);

        // prepend block to bucket list (is now first free block in bucket)

        if (g_pBuckets[ulBucketNumber])
        {
#ifdef _EFENCE_ALLOC
            // page at head of list might be protected.

            BOOL bProtected = g_bEFenceProtectFreedMemory &&
                                g_pBucketBlocksPerPage[ulBucketNumber] == 1;
            _InUsePage* pPageProt = g_pBuckets[ulBucketNumber]->pPageHeader;
            if (bProtected)
            {
                // the block header will be on the first page,
                // so _PAGESIZE is sufficient.
                HX_ASSERT(0 == mprotect((char*)pPageProt, _PAGESIZE,
                                        PROT_READ | PROT_WRITE));
            }
#endif
            g_pBuckets[ulBucketNumber]->pPrevInBucket = pBlock;
#ifdef _EFENCE_ALLOC
            if (bProtected)
            {
                HX_ASSERT(0 == mprotect((char*)pPageProt, _PAGESIZE, PROT_READ));
            }
#endif
        }
        pBlock->pPrevInBucket = NULL;
        pBlock->pNextInBucket = g_pBuckets[ulBucketNumber];
        g_pBuckets[ulBucketNumber] = pBlock;
    }
    // update free block ptr (last free block for this page in bucket list)
    pPage->pLastFreeBlock = pBlock;

    --pPage->ulBlocksInUse;

#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    pBlock->pBlockFlags &= ~MEMORY_BLOCK_IN_USE;
    }
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED
#ifdef _EFENCE_ALLOC
    if (g_bEFenceProtectFreedMemory &&
        g_pBucketBlocksPerPage[ulBucketNumber] == 1)
    {
        pPage->ulPageFlags |= MEMORY_PAGE_PROTECTED;
        HX_ASSERT(0 == mprotect((char*)pPage, _PAGESIZE *
                                g_pBucketPageSize[ulBucketNumber], PROT_READ));
    }
#endif //_EFENCE_ALLOC

    HXMutexUnlock(&g_pBucketLocks[ulBucketNumber]);
}


////////////////////////////////////////////////////////////////////////
//                      _ConsolidatePages
////////////////////////////////////////////////////////////////////////
//
// Description:         Helper function to consolidate two pages into
//                      one free page.
//                      Caller should already have free-list mutex.
//
// Parameters:
//              pFree           Free page
//              pFirst          First of two adjacent pages
//              pSecond         Second of two adjacent pages
//
//              (That is, pFirst and pSecond are to be consolidated,
//              and pFree is pointing to the one that's already free.)
//
// Returns:
//
// Implementation:
//              Free page is usually pristine: correct size, free-page
//              pointers, etc.  The other page is usually an in-use page
//              being freed, so we trust the master page pointers, but
//              not the size (from the bucket number).  Fortunately, we
//              don't need that to do the consolidation.  (Caller remembers
//              how big the initial reclaimed page was.)
//
//              The final, accumulated free page is put on the proper
//              free-page list by the caller.
//
// Revisions:
//      jmevissen       11/14/2000      Initial version
//
////////////////////////////////////////////////////////////////////////
static void
_ConsolidatePages(_FreePage* pFree, _Page* pFirst, _Page* pSecond)
{
#ifdef JMEVISSEN_PRINTF
    printf("jmev: consolidating free pages\n");
    printf("      Pages at %lx and %lx\n", pFirst, pSecond);
#endif

    // update master page list

    pFirst->pPageNext = pSecond->pPageNext;
    if (pSecond->pPageNext)
    {
        pSecond->pPageNext->pPagePrev = pFirst;
    }
    else
    {
        SHMEM_ASSERT(*g_pLastPage == pSecond);
        *g_pLastPage = pFirst;
    }

    // remove existing free page from free-list-by-size list

    UINT32 sizeInd = min(pFree->ulNumPages, MAX_PAGE_SIZE_PAGES);

    if (pFree->pFreePagePrev)
    {
        pFree->pFreePagePrev->pFreePageNext = pFree->pFreePageNext;
    }
    else
    {
        SHMEM_ASSERT(g_pFreePageListBySize[sizeInd] == pFree);
        g_pFreePageListBySize[sizeInd] = pFree->pFreePageNext;
    }
    if (pFree->pFreePageNext)
    {
        pFree->pFreePageNext->pFreePagePrev = pFree->pFreePagePrev;
    }
}

////////////////////////////////////////////////////////////////////////
//                      SharedMemory::ReclaimPage
////////////////////////////////////////////////////////////////////////
//
// Description:         Add page to the free list.  Remove its blocks
//                      from the bucket.
//
//                      Caller should already have the bucket mutex
//                      (if not the last bucket).
//
// Parameters:
//              pPage           Pointer to page to reclaim
//
// Returns:
//
// Implementation:
//
//      - remove free blocks on page from bucket list
//      - set bucket number in page header to zero (free page indicator)
//      - consolidate with adjacent free pages (if any)
//      - add to free list (or move consolidated pages to free list)
//
//      On some systems (win32, e.g.), we don't necessarily have
//      one big shared memory block.  We do split pages ourselves,
//      though, so sometimes pages can be (re-) joined and sometimes
//      we have two different allocations that can't be joined.  Thus,
//      verify pages really are contiguous before joining.
//
//      This is a superfluous check on one-big-block-of-shared-memory systems,
//      but I haven't ifdef'd the check at all.
//
//
// Revisions:
//      jmevissen       11/09/2000      Initial version
//
////////////////////////////////////////////////////////////////////////
void
SharedMemory::ReclaimPage(_InUsePage* pPage)
{
    UINT16      bucket = pPage->ulBucketNumber;

    ASSERT(bucket);

#ifdef JMEVISSEN_PRINTF
    printf("reclaiming page.\n");
#endif

    HXMutexLock(g_pFreeListLock);

    // the last bucket doesn't have a bucket list... it's for big allocations
    // that are done case-by-case outside the fast-allocation scheme.

    if (bucket != LAST_BUCKET)
    {
        // Remove free blocks on page from bucket list

        // ... get pointers to first and last blocks on page in bucket list
        // NB: there's no guarantee the blocks are in the bucket list in
        // memory order, which is why we have to walk the linked list.

        UINT16 numBlocks = g_pBucketBlocksPerPage[bucket];
        ASSERT(numBlocks);
        _FreeBlock* pFirstBlock;
        _FreeBlock* pLastBlock = pPage->pLastFreeBlock;

        pFirstBlock = pLastBlock;
        SHMEM_ASSERT(pFirstBlock->pPageHeader == pPage);
#ifdef JMEVISSEN_PRINTF
        printf("jmev: releasing %lx ", pFirstBlock);
#endif
        while (--numBlocks)
        {
            pFirstBlock = pFirstBlock->pPrevInBucket;
            SHMEM_ASSERT(pFirstBlock->pPageHeader == pPage);
#ifdef JMEVISSEN_PRINTF
            printf("%lx ", pFirstBlock);
#endif
        }
#ifdef JMEVISSEN_PRINTF
        printf("\n");
#endif
        // ... join the remaining ends of the bucket list together

        if (pFirstBlock->pPrevInBucket)
        {
            pFirstBlock->pPrevInBucket->pNextInBucket = pLastBlock->pNextInBucket;
        }
        else
        {
            g_pBuckets[bucket] = pLastBlock->pNextInBucket;
        }
        if (pLastBlock->pNextInBucket)
        {
            pLastBlock->pNextInBucket->pPrevInBucket = pFirstBlock->pPrevInBucket;
        }
    }

    // Update free page count

    UINT32 newNumPages;
    if (bucket == LAST_BUCKET)
    {
        newNumPages = ((_InUseBigPage*)pPage)->ulNumPages;
    }
    else
    {
        newNumPages = g_pBucketPageSize[bucket];
    }
    ASSERT(newNumPages);
    *g_pFreePagesOutstanding += newNumPages;
    *g_pPageFrees += newNumPages;

    //
    // Does page consolidate with prev/next page in memory?
    //

    if (pPage->pPagePrev &&
        pPage->pPagePrev->ulBucketNumber == 0)
    {
        _FreePage* pFree = (_FreePage*) pPage->pPagePrev;

        // can these pages really be joined?

        if ((PTR_INT)pFree + (pFree->ulNumPages << PAGESIZE_OFFSET) ==
            (PTR_INT)pPage)
        {
            newNumPages += pFree->ulNumPages;

            _ConsolidatePages(pFree,
                              pPage->pPagePrev,
                              pPage);

            pPage = (_InUsePage*) pFree;        // pPage now points to entire
                                                // (free) page in memory
        }
#ifdef JMEVISSEN_PRINTF
        else
        {
            printf("jmev: cannot join adjacent free pages at %lx and %lx.\n",
                   pFree, pPage);
        }
#endif
    }

    // check next page for consolidation:

    if (pPage->pPageNext &&
        pPage->pPageNext->ulBucketNumber == 0)
    {
        _FreePage* pFree = (_FreePage*) pPage->pPageNext;

        // can these pages really be joined?

        if ((PTR_INT)pPage + (newNumPages << PAGESIZE_OFFSET) ==
            (PTR_INT)pFree)
        {
            newNumPages += pFree->ulNumPages;

            _ConsolidatePages(pFree,
                              pPage,
                              pPage->pPageNext);
        }
#ifdef JMEVISSEN_PRINTF
        else
        {
            printf("jmev: cannot join adjacent free pages at %lx and %lx.\n",
                   pPage, pFree);
        }
#endif
    }

    // insert this page into the correct free list

    _FreePage* pFree = (_FreePage*) pPage;
    UINT32 sizeInd = newNumPages;
    if (sizeInd > MAX_PAGE_SIZE_PAGES) sizeInd = MAX_PAGE_SIZE_PAGES;

    pFree->ulBucketNumber = 0;
    pFree->ulNumPages = newNumPages;

    pFree->pFreePageNext = g_pFreePageListBySize[sizeInd];
    pFree->pFreePagePrev = NULL;
    if (g_pFreePageListBySize[sizeInd])
    {
        g_pFreePageListBySize[sizeInd]->pFreePagePrev = pFree;
    }
    g_pFreePageListBySize[sizeInd] = pFree;

    HXMutexUnlock(g_pFreeListLock);

#ifdef JMEVISSEN_PRINTF
    printf("Free page at %lx:\n", pFree);
    printf("   PagePrev %lx, PageNext %lx\n", pFree->pPagePrev, pFree->pPageNext);
    printf("   FreePagePrev %lx, FreePageNext %lx\n", pFree->pFreePagePrev, pFree->pFreePageNext);

    printf("jmev: page reclaim complete\n");
#endif
}

////////////////////////////////////////////////////////////////////////
//                      SharedMemory::ReclaimByAge
////////////////////////////////////////////////////////////////////////
//
// Description:         Reclaim pages by allocation age.
//
// Parameters:
//                      INT32 age       age (in seconds) of pages to reclaim.
//                      int bucket      bucket to reclaim; do all buckets if
//                                      not supplied
//
// Returns:             Number of pages reclaimed.
//
// Implementation:
//              Page age is measured since time of last allocation.  Passing
//              (age == 0) will cause all unallocated pages to be reclaimed.
//              Passing (age < 0) is undefined, since the type of tv_sec_t
//              may or may not be unsigned.
//
// Revisions:
//      jmevissen       12/21/2000      Initial version
//
////////////////////////////////////////////////////////////////////////

// Reclaim-all-buckets version

UINT32
SharedMemory::ReclaimByAge(INT32 age)
{
    UINT32 retval = 0;

    // Loop through all the buckets, finding reclaimable pages

    for (INT32 bucket=0; bucket<SHMEM_NUM_BUCKETS; bucket++)
    {
        retval += ReclaimByAge(age, bucket);
    }
    return retval;
}

UINT32
SharedMemory::ReclaimByAge(INT32 age, INT32 bucket)
{

    // On windows, it's possible we're not using buckets -- see hx_malloc_init()

#ifdef _WIN32
    if (!g_pBuckets)
    {
        DPRINTF(D_INFO, ("SharedMemory::ReclaimByAge skipped because no buckets.\n"));
        return 0;
    }
#endif

    // check bucket number for sanity
    // (LAST_BUCKET are custom big allocs that are reclaimed at free() time.)

    if (bucket < 0 ||
        bucket >= LAST_BUCKET) return 0;

    UINT32 retval = 0;

    // Get the current time

    if (! *self->m_pNow) return retval;
    tv_sec_t now = (*self->m_pNow)->tv_sec;
    ASSERT(now);

    HXMutexLock(&g_pBucketLocks[bucket]);

    _FreeBlock* block = g_pBuckets[bucket];
    while (block)
    {
        // get ptr for next iteration (a block on the next page)

        _FreeBlock* next = block->pPageHeader->pLastFreeBlock->pNextInBucket;

        // Is page reclaimable?  (ie, has no blocks in use)
        // If so, is it stale?

        if (block->pPageHeader->ulBlocksInUse == 0 &&
            now - block->pPageHeader->ulTimestamp >= (tv_sec_t)age)
        {
            retval += g_pBucketPageSize[bucket];
                ReclaimPage(block->pPageHeader);
        }

        block = next;
    }

    HXMutexUnlock(&g_pBucketLocks[bucket]);
    return retval;
}

SharedMemory::SharedMemory()
{
    region = new SharedRegion(SHARED_REGION_SIZE);
    end_of_used_space = (UINT8 **)region->region();

    *end_of_used_space = region->region();
    *end_of_used_space = *end_of_used_space + sizeof(UINT8*);

    num_malloc = (UINT32*)*end_of_used_space;
    *num_malloc = 0;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(UINT32));

    num_fastmalloc = (UINT32*)*end_of_used_space;
    *num_fastmalloc = 0;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(UINT32));

    num_free = (UINT32*)*end_of_used_space;
    *num_free = 0;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(UINT32));

    bytes_in_use_counters = (UINT32*)*end_of_used_space;
    memset(bytes_in_use_counters, 0, MAX_THREADS);
    *end_of_used_space = *end_of_used_space + (sizeof(UINT32) * MAX_THREADS);

    bytes_in_use = (UINT32*)*end_of_used_space;
    *bytes_in_use = 0;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(UINT32));

    bytes_allocated = (UINT32*)*end_of_used_space;
    *bytes_allocated = 0;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(UINT32));

    in_first_heap = (BOOL*)*end_of_used_space;
    *in_first_heap = TRUE;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(BOOL));

    second_heap_size = (UINT32*)*end_of_used_space;
    *second_heap_size = region->GetSecondHeapSize();
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(UINT32));

    m_pNow = (Timeval**)*end_of_used_space;
    *m_pNow = 0;
    *end_of_used_space = *end_of_used_space + sizeof(Timeval*);

#ifdef PAULM_ALLOCTRACK
    ogre_debug = (BOOL*)*end_of_used_space;
    *ogre_debug = FALSE;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(BOOL));
#endif

    bytes_remaining = (UINT32*)*end_of_used_space;
    *end_of_used_space = *end_of_used_space + BYTE_ALIGN_SIZE(sizeof(UINT32));
    *bytes_remaining = region->GetFirstHeapSize() -
        (*end_of_used_space - region->region());  // memory already used

    g_pTotalMemUsed = (UINT32*)*end_of_used_space;
    *end_of_used_space = *end_of_used_space + sizeof (long int);
    *bytes_allocated = *bytes_allocated + sizeof (long int);
    *bytes_remaining -= sizeof(long int);
    *g_pTotalMemUsed = 0;

    // on linux, we reserve 16k of memory for glibc calls after running
    // out of memory.
#ifdef _LINUX
    if (!g_pSecondHeapBottom && *bytes_remaining > 16384)
    {
        *bytes_remaining -= 16384;
    }
#endif
}

void
SharedMemory::Create()
{
#ifdef DEBUG
    if (self)
        PANIC(("Can only have one instance of SharedMemory!\n"));
#endif

    self = new SharedMemory;
    hx_malloc_init();

    /* Round to an exact page */
    *self->end_of_used_space =
        (UINT8* VOLATILE)pageround((PTR_INT)*self->end_of_used_space);

#ifdef PAULM_ALLOCTRACK
    {
        extern int   shared_ready;

        shared_ready = 1;
        g_allocating_tracker = TRUE;
        alloc_list = new CHXMapPtrToPtr;
        alloc_list->InitHashTable(3011);
        g_allocating_tracker = FALSE;
    }
#endif
}


SharedMemory::~SharedMemory()
{
    hx_malloc_end();
    delete region;
}

void*
SharedMemory::malloc(INT32 size)
{
    if (g_pSizes)
    {
        if (size > 65535)
        {
            g_pSizes[65535]++;
        }
        else
            g_pSizes[size]++;
    }
#if defined HEAP_VALIDATE
    size = size + 2 * MEM_GUARD_SIZE;
#else
    /*
     *  We really only need 1 int, but on Solaris we need 8 byte alignment.
     */
    size += sizeof(INT32) * 2;
#endif

#ifdef _EFENCE_ALLOC
    char* ret;
    char* guard = 0;
    if (g_efence_alloc < 0 || g_efence_alloc == get_bucket_from_size(size))
    {
        int size_padding = ALIGNMENT_PADDING(size);
        int efence_offset = 0;
        int fence_alloc_size = size + 2 * _PAGESIZE;
        fence_alloc_size += ALIGNMENT_PADDING(fence_alloc_size);
        ret = (char *)hx_malloc(fence_alloc_size);
        int pagenum = 0;

        if (!g_backfence_alloc)
        {
            pagenum = (int)(ret + size + size_padding) / _PAGESIZE;
            if ((int)(ret + size + size_padding) % _PAGESIZE)
                pagenum++;
            guard = (char*)(_PAGESIZE * pagenum);
            efence_offset = (guard - size) - ret;
            // remember to align the ret ptr by 8 bytes
            efence_offset = guard - (size + size_padding) - ret;
            ASSERT(efence_offset);
            ret = guard - (size + size_padding);
        }
        else
        {
            pagenum = (int)ret / _PAGESIZE;
            if ((int)ret % _PAGESIZE)
                pagenum++;
            guard = (char*)(_PAGESIZE * pagenum);
            efence_offset = (guard + _PAGESIZE) - ret;
            ASSERT(efence_offset);
            ret = guard + _PAGESIZE;
        }

        putlong((BYTE*)ret - sizeof(int), efence_offset);
    }
    else
    {
        ret = (char*)hx_malloc(size);
    }
#else
    char* ret = (char *)hx_malloc(size);
#endif

#if defined PAULM_ALLOCTRACK
    if(*self->ogre_debug && *g_pbTrackAllocs)
    {
        int locked = 0;
        if (*g_pLockedPid != Process::get_procnum())
        {
           HXMutexLock(g_pOgreLock);
           locked = 1;
           *g_pLockedPid = Process::get_procnum();
        }
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
            void** stack = (void**)get_stack(&stacksize);
            AllocTracker* track = new AllocTracker(ret, size,
                                                   stack, (INT32)stacksize,
                                                   FALSE);
            (*alloc_list)[ret] = track;
            g_allocating_tracker = FALSE;
        }
        if (locked)
        {
            *g_pLockedPid = MAX_THREADS + 1;
            HXMutexUnlock(g_pOgreLock);
        }
    }
#endif
#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    char* ptr = ret;
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED
#if defined HEAP_VALIDATE
    memset(ret, 0xd0, size);
    memset(ret, '$', MEM_GUARD_SIZE);
    memset(ret - MEM_GUARD_SIZE + size, '&', MEM_GUARD_SIZE);
    memcpy(ret, &size, sizeof(INT32));
    ret += MEM_GUARD_SIZE;
#else
    putlong((BYTE*)ret, size);
#if defined MEMORY_CORRUPTION_CHECK
    if(size >= MEMORY_CORRUPTION_CHECK_MIN &&
                        size < MEMORY_CORRUPTION_CHECK_MAX )
    {
        //move the address for the stack up to the guard position
        memcpy((BYTE*)ret + sizeof(INT32), (BYTE*)ret + 8, 4);
    }
#else
    putlong((BYTE*)ret + sizeof(INT32), 0xcdd0d0cd);
#endif //MEMORY_CORRUPTION_CHECK

    ret += sizeof(INT32) * 2;
#endif
#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    ((_Block*)(ptr - sizeof(_Block)))->pBlockFlags |= MEMORY_BLOCK_GUARDS_SET;
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED

#ifdef _EFENCE_ALLOC
    if (guard && mprotect((char*)guard, _PAGESIZE, PROT_NONE))
    {
        printf("_EFENC_ALLOC mprotect failed!\n");
    }
#endif //_EFENC_ALLOC

    (*self->num_malloc)++;
    DPRINTF(D_ALLOC, ("Malloc() %p [%d bytes]\n", ret, size));
    self->bytes_in_use_counters[Process::get_procnum()] += size;

#if defined PAULM_ALLOCDUMP
    if (m_bAllocDump)
        track_alloc(size, ret);
#endif
    return ret;
}

/*
 * WARNING!!  This calloc is ONLY called from the NT CRT's calloc() which
 * already multiplies number of elements * size.  This function IGNORES the
 * "num" == junk parameter.
 */
void*
SharedMemory::calloc(INT32 junk, INT32 size)
{
    char* ret = (char*)SharedMemory::malloc(size);

    if (ret)
    {
        memset(ret, 0, size);
    }

    return ret;
}

extern "C" void* hx_realloc(void* cp, size_t nbytes);

void*
hx_realloc(void* cp, size_t nbytes)
{
    return SharedMemory::realloc((char*)cp, (INT32)nbytes);
}

void*
SharedMemory::realloc(char* ptr, INT32 size)
{
    if (!ptr)
    {
        return SharedMemory::malloc(size);
    }

    else if (ptr && !size)
    {
        SharedMemory::free(ptr);
        return NULL;
    }

    char* ret = (char*)SharedMemory::malloc(size);

    if (ret)
    {
        char* temptr = ptr;
        INT32 tempsize = 0;
#if defined HEAP_VALIDATE
        temptr -= MEM_GUARD_SIZE;
        memcpy(&tempsize, temptr, sizeof(INT32));
        tempsize -= MEM_GUARD_SIZE;
#else
        temptr -= sizeof(INT32) * 2;
        tempsize = getlong((BYTE*)temptr);
        tempsize -= sizeof(INT32) * 2;
#endif
        if (tempsize < size)
        {
            size = tempsize;
        }

        memcpy(ret, ptr, size);
        SharedMemory::free(ptr);
    }

    return ret;
}

void
SharedMemory::free(char *ptr)
{
    INT32 size = 0;
    static char* pDollar = (char*)"$$$$$$$$";
    static char* pAmp    = (char*)"&&&&&&&&";

#if defined PAULM_ALLOCDUMP
    if (m_bAllocDump)
        track_free(ptr);
#endif

#ifdef HEAP_VALIDATE
    ptr -= MEM_GUARD_SIZE;
    memcpy(&size, ptr, sizeof(INT32));
#else
    ptr -= sizeof(INT32) * 2;
    size = getlong((BYTE*)ptr);
#endif

#if defined _EFENCE_ALLOC
    int efence_offset = 0;
    int bkt = get_bucket_from_size(size);
    if (g_efence_alloc < 0 || g_efence_alloc == bkt)
    {
        if (g_backfence_alloc)
        {
            if (mprotect((char*)(((PTR_INT)ptr / _PAGESIZE -1) * _PAGESIZE), _PAGESIZE,
                         PROT_READ | PROT_WRITE))
            {
                printf("_EFENCE_ALLOC Failed to unprotect free mem!\n");
            }
        }
        else
        {
            // remember to byte align the ptr as was done during 
            // SharedMemory::malloc()
            if (mprotect(ptr + size + ALIGNMENT_PADDING(size), _PAGESIZE,
                        PROT_READ | PROT_WRITE))
            {
                printf("_EFENCE_ALLOC Failed to unprotect free mem!\n");
            }
        }
        efence_offset = getlong((BYTE*)ptr - sizeof(int));  // look after unprotecting
    }
#endif

#if defined PAULM_ALLOCTRACK
    if(*self->ogre_debug && *g_pbTrackFrees)
    {
        int locked = 0;
        if (*g_pLockedPid != Process::get_procnum())
        {
            HXMutexLock(g_pOgreLock);
            *g_pLockedPid = Process::get_procnum();
            locked = 1;
        }

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
            }
        }
        if (locked)
        {
            *g_pLockedPid = MAX_THREADS + 1;
            HXMutexUnlock(g_pOgreLock);
        }
    }
#endif
#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
#ifdef _EFENCE_ALLOC
    _Block* pBlock = (_Block*)(ptr - efence_offset - sizeof(_Block));
#else
    _Block* pBlock = (_Block*)(ptr - sizeof(_Block));
#endif //_EFENCE_ALLOC
    _InUsePage* pPage = pBlock->pPageHeader;

    HXMutexLock(&pPage->mValidateGuardLock);
    pBlock->pBlockFlags &= ~MEMORY_BLOCK_GUARDS_SET;
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED

#if defined HEAP_VALIDATE
    if (memcmp(ptr + sizeof(INT32), pDollar, MEM_GUARD_SIZE - sizeof(INT32)))
    {
        printf ("Front Guard Failure, %p, %d (%x %x %x %x %x %x %x %x)\n",
            ptr, size,
            (ptr + sizeof(INT32))[0],
            (ptr + sizeof(INT32))[1],
            (ptr + sizeof(INT32))[2],
            (ptr + sizeof(INT32))[3],
            (ptr + sizeof(INT32))[4],
            (ptr + sizeof(INT32))[5],
            (ptr + sizeof(INT32))[6],
            (ptr + sizeof(INT32))[7]);
        fflush(0);
        ASSERT(0);
    }
    if (memcmp(ptr + size - MEM_GUARD_SIZE, pAmp, MEM_GUARD_SIZE))
    {
        printf ("Rear Guard Failure, %p, %d (%x %x %x %x %x %x %x %x)\n",
            ptr, size,
            (ptr + size - MEM_GUARD_SIZE)[0],
            (ptr + size - MEM_GUARD_SIZE)[1],
            (ptr + size - MEM_GUARD_SIZE)[2],
            (ptr + size - MEM_GUARD_SIZE)[3],
            (ptr + size - MEM_GUARD_SIZE)[4],
            (ptr + size - MEM_GUARD_SIZE)[5],
            (ptr + size - MEM_GUARD_SIZE)[6],
            (ptr + size - MEM_GUARD_SIZE)[7]);
        fflush(0);
        ASSERT(0);
    }
    memset(ptr, 0xcd, size);
#elif defined MEMORY_CORRUPTION_CHECK
    if(size >= MEMORY_CORRUPTION_CHECK_MIN &&
                        size < MEMORY_CORRUPTION_CHECK_MAX )
    {
        int mcsize = 0;
        void ** mcstack = get_stack(&mcsize);
        int mcIndex = HXAtomicIncRetUINT32(g_mcStacksIndex);

        //if we reach the bottom, we need to start over from the top again
        // there may be race condition here, but it is just debugging code
        // and we don't care.
        if(mcIndex >= 3276800 )
        {
            mcIndex = *g_mcStacksIndex = 0;
        }
        mcIndex <<= 3;
        UINT32* mcPosition = g_mcStacksArray + mcIndex;

        //the top 3 stacks are useless for our purpose, omit them.
        memcpy(mcPosition, mcstack + 3, 8*sizeof(INT32));
        //we cannot put it at higher places, which are used to store block info.
        putlong((BYTE*)ptr + 2*sizeof(INT32), (INT32)mcPosition);
    }
#else
    int guard = getlong((BYTE*)ptr + sizeof(INT32));
    if (guard != 0xcdd0d0cd)
    {
        if (g_bCrashAvoidancePrint)
        {
#ifdef _WIN32
            DWORD tid = GetCurrentThreadId();
#else
            pthread_t tid = pthread_self();
#endif
            printf ("\nGuard failure on free(): Size: %d Guard: %x Thread: %lu\n", size, guard, tid);

        }
        volatile int* x = 0;
        *x = 5;
    }
    putlong((BYTE*)ptr + sizeof(INT32), 0);
#endif
#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
    HXMutexUnlock(&pPage->mValidateGuardLock);
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED
#ifdef _EFENCE_ALLOC
    ptr -= efence_offset;
#endif
    hx_free(ptr);
    DPRINTF(D_ALLOC, ("Free() %p [%d bytes]\n", ptr, size));

    (*self->num_free)++;
    self->bytes_in_use_counters[Process::get_procnum()] -= size;

    if (g_pSizes)
    {
#if defined HEAP_VALIDATE
        size = size - 2 * MEM_GUARD_SIZE;
#endif

        if (size > 65535)
        {
            size = 65535;
        }

        if (g_pSizes[size])
        {
            g_pSizes[size]--;
        }
    }
}

INT32
SharedMemory::checkblock(char *ptr)
{
    INT32 size = 0;
    static char* pDollar = (char*)"$$$$$$$$";
    static char* pAmp    = (char*)"&&&&&&&&";

#if defined HEAP_VALIDATE
    ptr -= MEM_GUARD_SIZE;
    memcpy(&size, ptr, sizeof(INT32));
    if (!memcmp(ptr + sizeof(INT32), pDollar, MEM_GUARD_SIZE - sizeof(INT32)) &&
        !memcmp(ptr + size - MEM_GUARD_SIZE, pAmp, MEM_GUARD_SIZE))
    {
        return TRUE;
    }

    return FALSE;
#else
    return TRUE;
#endif
}

INT32
SharedMemory::blocksize(char *ptr)
{
    INT32 size = 0;

#if defined HEAP_VALIDATE
    ptr -= MEM_GUARD_SIZE;
    memcpy(&size, ptr, sizeof(INT32));
    size -= MEM_GUARD_SIZE;
#else
    ptr -= sizeof(INT32) * 2;
    size = getlong((BYTE*)ptr);
    size -= sizeof(INT32) * 2;
#endif

    return size;
}

#ifdef PAULM_ALLOCTRACK
void
SharedMemory::checkguards()
{
#if defined HEAP_VALIDATE
    if(*self->ogre_debug)
    {
        CHXMapPtrToPtr::Iterator i;
        static char* pDollar = (char*)"$$$$$$$$$$";
        static char* pAmp    = (char*)"&&&&&&&&&&";

        INT32 checked = 0, badhead = 0, badtail = 0;
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
                if(memcmp(ptr + sizeof(INT32), pDollar, MEM_GUARD_SIZE - sizeof(INT32)) != 0)
                {
                    badhead++;
                    printf("Bad head guard at %p (size %ld)\nStack:\n%s",
                           ptr, size,
                           get_trace_from_stack(track->stack,
                                                track->stacksize));
                }
                if(memcmp(ptr + size - MEM_GUARD_SIZE, pAmp, MEM_GUARD_SIZE) != 0)
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
#else
        printf("checkguards disabled(), define HEAP_VALIDATE\n");
#endif
}

void
SharedMemory::resetleaks()
{
    if(*self->ogre_debug)
    {
        int locked = 0;
    printf("SharedMemory::resetleaks: tracking allocs...\n");
    fflush(stdout);
        if (*g_pLockedPid != Process::get_procnum())
        {
            HXMutexLock(g_pOgreLock);
            locked = 1;
            *g_pLockedPid = Process::get_procnum();
        }
        CHXMapPtrToPtr::Iterator i;

        for(i = alloc_list->Begin(); i != alloc_list->End(); ++i)
        {
            AllocTracker* track = (AllocTracker*)(*i);
            track->startup = TRUE;
        }
    *g_pbTrackAllocs = TRUE;
    *g_pbTrackFrees= TRUE;
        if (locked)
        {
            *g_pLockedPid = MAX_THREADS + 1;
            HXMutexUnlock(g_pOgreLock);
        }
    }
    else
    {
        printf("SharedMemory::resetleaks: AllocTrack option not set\n");
        fflush(stdout);
    }
}

void
SharedMemory::suspendleaks()
{
    printf("SharedMemory::suspendleaks: alloc tracking suspended\n");
    *g_pbTrackAllocs = FALSE;
}

void
SharedMemory::checkleaks()
{
    if(*self->ogre_debug)
    {
        int locked = 0;
        printf("SharedMemory::checkleaks: dumping leaks...\n");
        if (*g_pLockedPid != Process::get_procnum())
        {
            HXMutexLock(g_pOgreLock);
            locked = 1;
            *g_pLockedPid = Process::get_procnum();
        }
        CHXMapPtrToPtr::Iterator i;
        for(i = alloc_list->Begin(); i != alloc_list->End(); ++i)
        {
            AllocTracker* track = (AllocTracker*)(*i);

            if(!track->startup)
            {
                printf("======================================================================\n"
                       "ptr: %p size: %d\n"
                       "%s"
                       "======================================================================\n",
                       track->ptr, track->size,
                       get_trace_from_stack(track->stack, track->stacksize));
            track->startup = TRUE;
            }
        }

        *g_pbTrackAllocs = FALSE;
        *g_pbTrackFrees= FALSE;

        if (locked)
        {
            *g_pLockedPid = MAX_THREADS + 1;
            HXMutexUnlock(g_pOgreLock);
        }

        printf("Done with alloc traces. \n\n\n");
    }
    else
    {
        printf("SharedMemory::checkleaks: AllocTrack option not set\n");
    }
    fflush(stdout);
}

void
SharedMemory::setogredebug(BOOL set)
{
    *self->ogre_debug = set;
}

#endif

void
SharedMemory::reportsizes()
{
    if (!g_pSizes)
    {
        return;
    }

    FILE* f = fopen("memoryleak.log", "a");

    if (!f)
    {
        return;
    }

    fprintf (f, "=========================================================\n");
    fprintf (f, "Allocation SizeMap:\n");

    for (INT32 i = 0; i < 65536; i++)
    {
        if (g_pSizes[i])
        {
            fprintf (f, "%5d\t%06ld\n", i, g_pSizes[i]);
        }
    }
    fprintf (f, "=========================================================\n");
    fclose(f);
}

void
SharedMemory::resetsizes()
{
    if (!g_pSizes)
    {
        return;
    }

    for (INT32 i = 0; i < 65536; i++)
    {
        g_pSizes[i] = 0;
    }
    g_bDoSizes = TRUE;
}

void*
SharedMemory::sbrk(UINT32 amt)
{
    BOOL bHaveMemory = TRUE;

#ifdef _WIN32
    void* pMem = ::malloc(amt);
    if (pMem)
    {
        DPRINTF(D_ALLOC, ("Malloc() requested %d bytes\n", amt));
        bHaveMemory = (amt <= *self->bytes_remaining);
        if (bHaveMemory)
        {
            // Everything's okay, go ahead and return the memory.
            HXAtomicAddUINT32(g_pTotalMemUsed, amt);
            *self->bytes_allocated = *self->bytes_allocated + amt;
            *self->bytes_remaining -= amt;
            return pMem;
        }
    }
#else
    if (amt > *self->bytes_remaining)
    {
        // on linux, we might have a second heap

        if (*self->in_first_heap && g_pSecondHeapBottom)
        {
            *self->end_of_used_space = (BYTE*)g_pSecondHeapBottom;
            *self->bytes_remaining = *self->second_heap_size;

            // we reserve 16k for glibc at out-of-memory.
            // the second heap is supposed to be at least 16k, but check anyway.

            if (*self->bytes_remaining > 16384)
            {
                *self->bytes_remaining -= 16384;
            }
            *self->in_first_heap = FALSE;

            bHaveMemory = (amt <= *self->bytes_remaining);
        }
        else
        {
            bHaveMemory = FALSE;
        }
    }

    if (bHaveMemory)
    {
        // Everything's okay, go ahead and return the memory.

        HXAtomicAddUINT32(g_pTotalMemUsed, amt);

        DPRINTF(D_ALLOC, ("Malloc() requested %d bytes\n", amt));
        void* out = *self->end_of_used_space;

        *self->end_of_used_space = *self->end_of_used_space +  amt;
        *self->bytes_allocated = *self->bytes_allocated + amt;
        *self->bytes_remaining -= amt;

        return out;
    }
#endif //_WIN32

    // we're out of memory.

#ifdef _WIN32
    DWORD pid = GetCurrentThreadId();
#else
    pid_t pid = getpid();
#endif

    RestartOnFault(TRUE);  // get_trace faults, sometimes.

    printf("%d: FATAL ERROR:  The server has run out of "
           "memory!\n", pid);
    printf("FATAL ERROR:  Last request was rounded up to %ld bytes\n", (long) amt);
    printf("Trace:\n%s", get_trace());
    printf("FATAL ERROR:  Server Terminated\n");
    fflush(0);

    RestartOrKillServer();

    return 0; // suppress compiler warning; not reached.
}

#ifdef MEMORY_SCRIBBLE_VALIDATE_ENABLED
/*
 * Memory validation
 *
 * Look for scribble errors in the shared-memory area.
 * Not compatible with EFence, I believe.  (Don't run both at once.)
 *
 * lStartPage, lPages
 *
 * Specifies a page range to allow smaller searches, if you have a
 * repro case and want to fail as soon as possible after the scribble.
 * Use lPages == 0 to check to the last page.  (Pages in the shared
 * space are numbered starting from zero.  You can decide on a range
 * based on the printout of previous errors.)
 *
 * ulFlags
 *
 * 0x00000001   fail -- abort() -- on finding an error.
 * 0x00000002   do rudimentary checks on internal SharedMemory data arrays.
 *              (not yet implemented)
 *
 */
HX_RESULT
SharedMemory::ValidateMemory(INT32 lStartPage, INT32 lPages, UINT32 ulFlags)
{
    INT32 lPageNum;
    HX_RESULT status = HXR_OK;

    _Page* pPage;

    // init the first-page pointer

    if (!(*g_pFirstPage))
    {
        pPage = *g_pLastPage;
        while (pPage->pPagePrev) pPage = pPage->pPagePrev;
        *g_pFirstPage = pPage;
    }

    // check the internal structures
    // if (ulFlags & _CHECK_INTERNALS) ValidateInternals(ulFlags);

    // find the first page to check

    lPageNum = lStartPage;
    pPage = *g_pFirstPage;
    while (--lStartPage >= 0 && pPage) pPage = pPage->pPageNext;

    // walk through the pages

    do
    {
        int code;
        if (code = ValidateSinglePage((_InUsePage*)pPage, lPageNum, ulFlags))
        {
            status = HXR_FAIL;
            printf("***\n*** Scribbled memory on page %d\n***\n",
                   (int) lPageNum);
            if (code & 0x1) printf("*** Invalid page header bucket size\n");
            if (code & 0x2) printf("*** Invalid area between blocks\n");
            if (code & 0x4) printf("*** Alloc size inconsistent with page header\n");
            if (code & 0x8) printf("*** Invalid front guard\n");
            if (code & 0x10) printf("*** Invalid rear guard\n");
            if (code & 0x20) printf("*** Blocks in use inconsistent with page header\n");
        }
        pPage = pPage->pPageNext;
        ++lPageNum;
        --lPages;
    }
    while (lPages && pPage);

    return status;
}

// macro for testing expressions: if d is false, check to see whether
// we abort on the spot or whether we return an error code.
#define VERIFY(a, b, c, d) \
        if (!(d)) \
        { \
            if ((a) & MEMORY_VALIDATE_ABORT_ON_FAILURE) \
            { \
                abort(); \
            } \
            (b) |= (c); \
        }

int
SharedMemory::ValidateSinglePage(_InUsePage* pPage, INT32 nPageNum, UINT32 ulFlags)
{
    int blocksize;
    int blocksInUse;
    int ret = 0;
    int bucket = pPage->ulBucketNumber;

    VERIFY(ulFlags, ret, 0x1, (bucket >= 0));
    VERIFY(ulFlags, ret, 0x1, (bucket <= LAST_BUCKET));

    // bucket zero is a free page, so there's nothing to check.
    // I'm not sure whether this works for big pages, so skip those too.

    if (!bucket || bucket == LAST_BUCKET) return ret;

    // else, step through the blocks

    blocksize = LAYOUT_BUCKET_SIZE(bucket);

    _Block* pBlock = (_Block*)((PTR_INT)pPage + sizeof(_InUsePage));
    UINT32 numBlocks = g_pBucketBlocksPerPage[bucket];
    char* ptr;
    UINT32 size;

    HXMutexLock(&g_pBucketLocks[bucket]);

    blocksInUse = pPage->ulBlocksInUse;

    while (numBlocks--)
    {
        // The block header always points to the start of the page.

        VERIFY(ulFlags, ret, 0x2, (pBlock->pPageHeader == pPage));

        // if the block is not free, try to check the guards.

        if (pBlock->pBlockFlags & MEMORY_BLOCK_IN_USE)
        {
            --blocksInUse;

            HXMutexLock(&pPage->mValidateGuardLock);
            if (pBlock->pBlockFlags & MEMORY_BLOCK_GUARDS_SET)
            {
                ptr = (char*)((PTR_INT)pBlock + sizeof(_Block));

                // the size should be appropriate for the page block
                // we seem to sometimes allocate one bucket larger than necessary.

#if defined HEAP_VALIDATE
                size = *(UINT32*)(ptr);
#else
                size = getlong((BYTE*)ptr);
#endif
                if (bucket > 1)
                    VERIFY(ulFlags, ret, 0x4, (size >= LAYOUT_BUCKET_SIZE(bucket-2)));
                VERIFY(ulFlags, ret, 0x4, (size < blocksize));

#if defined HEAP_VALIDATE
                VERIFY(ulFlags, ret, 0x8, (*(UINT32*)(ptr+4) == 0x24242424));
                ptr += size - 8;
                VERIFY(ulFlags, ret, 0x10, (ptr[0] == '&' && ptr[1] == '&' &&
                                            ptr[2] == '&' && ptr[3] == '&' &&
                                            ptr[4] == '&' && ptr[5] == '&' &&
                                            ptr[6] == '&' && ptr[7] == '&'));
#else
                VERIFY(ulFlags, ret, 0x8, (*(UINT32*)(ptr+4) == 0xcdd0d0cd));
#endif
            }
            HXMutexUnlock(&pPage->mValidateGuardLock);
        }

        // advance to the next block

        pBlock = (_Block*)((PTR_INT)pBlock + sizeof(_Block) + blocksize);
    }

    HXMutexUnlock(&g_pBucketLocks[bucket]);

    // did the free-block count agree?

    VERIFY(ulFlags, ret, 0x20, (blocksInUse == 0));

    return ret;
}
#endif //MEMORY_SCRIBBLE_VALIDATE_ENABLED

#ifdef AREA51_ENABLED
/*
 * Protected-memory services.
 *
 * Area 51 is like EFence, but I didn't want to confuse the issue
 * by calling it anything like EFence.
 *
 * Returns a page of memory you can place an object or buffer on, then
 * write-protect.
 *
 * Intended for DEVELOPMENT ONLY.  Freed pages are not returned to
 * the main memory system, but rather kept for use again by Area51.
 * Size is limited to (one page) - (header size) -- this could be
 * extended to multi-page allocations if you need it.
 *
 * You can use this in modules outside the server if you hack as follows,
 * for example:  (void* used for pVoid because a void* was needed later)
 *
 *      #include "../servsup/pub/shmem.h"
 *      extern void* operator new(size_t size, void* ptr);
 *      [...]
 *
 *      void* pVoid = SharedMemory::Area51Alloc(sizeof(CHXString));
 *      new (pVoid) CHXString((const char*)tok.value());
 *      SharedMemory::Area51Protect((char*)pVoid);
 *      [...]
 *
 *      SharedMemory::Area51UnProtect((char*)str);
 *      str->~CHXString();
 *      SharedMemory::Area51Free((char*)str);
 *
 * jmevissen  6/2001
 */
void*
SharedMemory::Area51Alloc(INT32 size)
{
    INT32 sbrkSize;
    Area51PageHeader* pPage;

    // larger pages could be implemented, but they haven't.
    // (just need to add page size to header data, search for
    // pages of correct size in free list, etc.)

    if (size + sizeof(Area51PageHeader) > _PAGESIZE)
    {
        printf("Protected memory: Size %d is too big to protect.\n", (int)size);
        HX_ASSERT(0);
        return NULL;
    }

    // lock mutex

    HXMutexLock(g_pArea51Lock);

    // get a free page

    if (*g_pArea51FreePages)
    {
        pPage = *g_pArea51FreePages;
        *g_pArea51FreePages = pPage->pNextPage;
    }
    else
    {
        // will sbrk be giving us an aligned page?
#ifndef _WIN32
            sbrkSize = _PAGESIZE;
#else // WIN32
        sbrkSize = _PAGESIZE * 2;
#endif

        // align it on page boundary

        pPage = (Area51PageHeader*) sbrk(sbrkSize);
        pPage = (Area51PageHeader*) pageround((int)pPage);
    }

    // fill in our header

    pPage->bInUse = TRUE;

    HXMutexUnlock(g_pArea51Lock);

    return (void*)((char*)pPage + sizeof(Area51PageHeader));
}

void
SharedMemory::Area51Free(char* ptr)
{
    Area51PageHeader* pPage = (Area51PageHeader*)
        (ptr - sizeof(Area51PageHeader));

    // pointer possibly valid?

    if ((int)pPage & malloc_pagemask)
    {
        printf("Protected memory: %p is not a valid ptr to free.\n", ptr);
        HX_ASSERT(0);
        return;
    }

    // lock mutex
    // (if you don't care about race conditions in freeing a ptr twice,
    // the mutex could be locked further down, just before manipulating
    // the free list)

    HXMutexLock(g_pArea51Lock);

    // has it been freed already?

    if (!pPage->bInUse)
    {
        printf("Protected memory: memory at %p has been freed already.\n", ptr);
        HX_ASSERT(0);
    }

    // make sure it's unprotected

    Area51UnProtect(ptr);

    // mark as free

    pPage->bInUse = FALSE;

    // return to free list.

    pPage->pNextPage = *g_pArea51FreePages;
    *g_pArea51FreePages = pPage;

    HXMutexUnlock(g_pArea51Lock);
}

void
SharedMemory::Area51Protect(char* ptr)
{
    Area51PageHeader* pPage = (Area51PageHeader*)
        (ptr - sizeof(Area51PageHeader));
    HX_ASSERT(!((int)pPage & malloc_pagemask));

    int status = mprotect((char*)pPage, _PAGESIZE, PROT_READ);
    HX_ASSERT(!status);
}

void
SharedMemory::Area51UnProtect(char* ptr)
{
    Area51PageHeader* pPage = (Area51PageHeader*)
        (ptr - sizeof(Area51PageHeader));
    HX_ASSERT(!((int)pPage & malloc_pagemask));

    int status = mprotect((char*)pPage, _PAGESIZE,
                          PROT_WRITE | PROT_READ | PROT_EXEC);
    HX_ASSERT(!status);
}
#endif //AREA51_ENABLED

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// Debug shme

#ifdef DEBUG

static void
_ValidateBuckets()
{
    // validate the unallocated buckets are on pages with the
    // correct bucket numbers.
    // Also verify that FreeBlockOffset points to last block on page
    // (last in terms of the bucket list)

    for (int i=0; i<SHMEM_NUM_BUCKETS; i++)
    {
        HXMutexLock(&g_pBucketLocks[i]);

        _FreeBlock* pBlock = g_pBuckets[i];
        _InUsePage* pLastPage = NULL;

        if (pBlock)
        {
            ASSERT(pBlock->pPrevInBucket == NULL);
        }
        while (pBlock)
        {
            _InUsePage* pPage = pBlock->pPageHeader;

            ASSERT(pPage->ulBucketNumber == i);

            if (pPage->pLastFreeBlock != pBlock)
            {
                pLastPage = pPage;
            }
            else
            {
                ASSERT(pLastPage == pPage || pLastPage == NULL);
                pLastPage = NULL;
            }
            pBlock = pBlock->pNextInBucket;
        }
        HXMutexUnlock(&g_pBucketLocks[i]);
    }
}

static void
_ValidateFreeLists()
{
    // Validate that the free pages on the free lists match their size.

    HXMutexLock(g_pFreeListLock);
    for (unsigned int i=0; i<=MAX_PAGE_SIZE_PAGES; i++)
    {

        _FreePage* pPage = g_pFreePageListBySize[i];
        if (pPage)
        {
            ASSERT(pPage->pFreePagePrev == NULL);
        }

        while (pPage)
        {
            if (i == MAX_PAGE_SIZE_PAGES)
            {
                ASSERT(pPage->ulNumPages >= i);
            }
            else
            {
                ASSERT(pPage->ulNumPages == i);
            }
            pPage = pPage->pFreePageNext;
        }
    }
    HXMutexUnlock(g_pFreeListLock);
}

static void
_ValidatePageList()
{
    // only works in shared-memory case.  Validate linked-list of all pages.

    HXMutexLock(g_pFreeListLock);

    _Page* pPage = *g_pLastPage;
    UINT32 ulSize;

    while (pPage)
    {
        _Page* pPrev = pPage->pPagePrev;

        if (pPrev)
        {
            ASSERT(pPrev->pPageNext == pPage);

            if (pPrev->ulBucketNumber)
            {
                ulSize = g_pBucketPageSize[pPrev->ulBucketNumber] << PAGESIZE_OFFSET;
            }
            else
            {
                ulSize = ((_FreePage*)pPrev)->ulNumPages << PAGESIZE_OFFSET;
            }
            ASSERT((PTR_INT)pPrev + ulSize == (PTR_INT)pPage);
        }

        pPage = pPrev;
    }
    HXMutexUnlock(g_pFreeListLock);
}
#endif  // DEBUG
