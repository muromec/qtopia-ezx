/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mem_cache.cpp,v 1.15 2006/08/20 18:00:32 dcollins Exp $ 
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

#include "mem_cache.h"
#include "servbuffer.h"
#include "servpckts.h"
#include "shmem.h"

#define XXXinline /**/ /// \todo XXXDC why is the inlining nulled-out?!

/**
 * \brief               MemCache Constructor - Creates an array of memory cache objects, one per thread.
 * \param procnum [in]  Process number for the current thread.
 * \return              n/a
 */
XXXinline
MemCache::MemCache(int procnum)
    : m_ulCachedMallocs(0)
    , m_ulCachedMisses(0)
    , m_ulCachedNew(0)
    , m_ulCachedDelete(0)
    , m_pRecycler(0)
{
    m_memCacheArray = (MemCache**)_SharedAlloc(sizeof(MemCache*) * MAX_THREADS);
    memset(m_memCacheArray, 0, MAX_THREADS * sizeof(MemCache*));
    _Init(procnum);
}

/** 
 * \brief               MemCache Constructor - Initializes the memory cache for this thread.
 * \param procnum [in]  Process number for the current thread.
 * \param pMemCache [in] Pointer to a MemCache object.
 * \return              n/a
 */
XXXinline
MemCache::MemCache(int procnum, MemCache* pMemCache)
    : m_ulCachedMallocs(0)
    , m_ulCachedMisses(0)
    , m_ulCachedNew(0)
    , m_ulCachedDelete(0)
{
    m_memCacheArray = pMemCache->m_memCacheArray;
    _Init(procnum);
}

/**
 * \brief               MemCache Destructor - should not be called
 * \return              n/a
 */
MemCache::~MemCache()
{
    /// \b warning - should never delete these
    HX_ASSERT(0);
}

/**
 * \brief               _Init - Initialize the memory cache for this thread.
 * \param procnum [in]  Process number for the current thread.
 * \return              void
 */
XXXinline void
MemCache::_Init(int procnum)
{
    int i;

    m_memCacheArray[procnum] = this;
    g_pMemCacheList = m_memCacheArray;

    m_pBlockCount = (UINT32*)_SharedAlloc(sizeof(UINT32) * NUM_MC_BUCKETS);
    memset(m_pBlockCount, 0, NUM_MC_BUCKETS * sizeof(UINT32));

    /** \note
     * We use an array of sizes per bucket rather than a flat size because
     * packet related buckets are more heavily used than others. This is 
     * just an easy way to recognize that, class caches and an improved
     * memory management scheme should be investigated to replace this
     */
    m_pMaxBucketEntries = (UINT32*)_SharedAlloc(sizeof(UINT32) * NUM_MC_BUCKETS);
    for (i=0; i < NUM_MC_BUCKETS; ++i)
    {
        m_pMaxBucketEntries[i] = NUM_MC_BUCKET_ENTRIES;
    }

    size_t uSize = sizeof(ServerBuffer);
#ifdef _WIN32
    uSize += sizeof(size_t);
#endif
    uSize += sizeof(RECYCLER_PROCNUM_TYPE);
    uSize += (2 ^ BUCKET_FACTOR);

    UINT32 ulBucket = uSize >> BUCKET_FACTOR;

    m_pMaxBucketEntries[ulBucket] = NUM_MC_BUCKET_ENTRIES_EXTENDED;

    uSize = sizeof(ServerPacket) + sizeof(MemCache*);
#ifdef _WIN32
    uSize += sizeof(size_t);
#endif

    uSize += sizeof(RECYCLER_PROCNUM_TYPE);
    uSize += (2 ^ BUCKET_FACTOR);
    ulBucket = uSize >> BUCKET_FACTOR;

    m_pMaxBucketEntries[ulBucket] = NUM_MC_BUCKET_ENTRIES_EXTENDED;

    m_pMemBlocks = (char***)_SharedAlloc(sizeof(char**) * NUM_MC_BUCKETS);
    memset(m_pMemBlocks, 0, NUM_MC_BUCKETS * sizeof(char**));

    m_ulCreatingProcnum = procnum;

    m_pRecycler = (RecyclerQueue*)_SharedAlloc(
        sizeof(RecyclerQueue) * NUM_MC_BUCKETS);
    memset(m_pRecycler, 0, NUM_MC_BUCKETS * sizeof(RecyclerQueue));
    for (i=0; i < NUM_MC_BUCKETS; ++i)
    {
        m_pRecycler[i].m_pMutex = HXCreateMutex();
        // printf("MemCache(%p) proc %d, bucket %d, lock %p\n", 
        //          this, procnum, i, m_pRecycler[i].m_pMutex);
    }
}

/**
 * \brief               _CacheNew - Allocate memory via the cache.
 * \param ulSize [in]   Size of requested memory allocation.
 * \return              Pointer to allocated memory.
 */
XXXinline char*
MemCache::_CacheNew(size_t ulSize)
{
    ASSERT(Process::get_procnum() == m_ulCreatingProcnum);

#ifdef _WIN32
    ulSize += sizeof(size_t);
#endif
    ulSize += sizeof(RECYCLER_PROCNUM_TYPE);

    m_ulCachedNew += 1;
    ulSize += (2 ^ BUCKET_FACTOR);
    UINT32 ulBucket = ulSize >> BUCKET_FACTOR;

    char* pReturn;

    // check the Recycler first!
    pReturn = _RecyclerGet(ulSize, ulBucket);
    if (pReturn != 0)
    {
        char* pPtr = pReturn - sizeof(RECYCLER_PROCNUM_TYPE);
        //printf ("CacheNew: got ptr %p for proc %d from %d\n",
        //        pPtr, m_ulCreatingProcnum, *(RECYCLER_PROCNUM_TYPE*)pPtr);
        return pReturn;
    }

    UINT32 ulNumBlocksInBucket = 0;
    if (ulBucket < NUM_MC_BUCKETS)
    {
        ulNumBlocksInBucket = m_pBlockCount[ulBucket];
    }

    if ((ulBucket < NUM_MC_BUCKETS) && ulNumBlocksInBucket)
    {
	m_ulCachedMallocs += 1;
	m_pBlockCount[ulBucket]--;
	ulNumBlocksInBucket--;
	pReturn = m_pMemBlocks[ulBucket][ulNumBlocksInBucket];
#ifdef _WIN32
	*((size_t*)pReturn) = ulSize;
	pReturn += sizeof(size_t);
#else
	ASSERT((SharedMemory::getsize(pReturn) >> BUCKET_FACTOR) == (ulBucket));
#endif

	*((RECYCLER_PROCNUM_TYPE*)pReturn) = m_ulCreatingProcnum | 0xabba0000;
	pReturn += sizeof(RECYCLER_PROCNUM_TYPE);

        //printf ("new(cached): ptr=%p size=%lu bucket=%lu proc=%d\n",
        //        pReturn - sizeof(RECYCLER_PROCNUM_TYPE),
        //        ulSize, ulBucket, m_ulCreatingProcnum);
	return pReturn;
    }
    else
    {
#ifdef MEMCACHE_DEBUG
        if (ulBucket >= NUM_MC_BUCKETS)
        {
            printf("MemCache: Bucket list exceeded for size %lu "
                   "[proc=%d, bucket=%lu]\n",
                   ulSize, m_ulCreatingProcnum, ulBucket);
        }
        //if (ulNumBlocksInBucket == 0)
        //{
        //    printf("MemCache::_CacheNew: bucket %d empty [size=%d, proc=%d]\n",
        //           ulBucket, ulSize, m_ulCreatingProcnum);
        //}
#endif

	m_ulCachedMisses += 1;
	pReturn = _SharedAlloc(ulSize);
#ifdef _WIN32
	*((size_t*)pReturn) = ulSize;
	pReturn += sizeof(size_t);
#endif

	*((RECYCLER_PROCNUM_TYPE*)pReturn) = m_ulCreatingProcnum | 0xabba0000;
	pReturn += sizeof(RECYCLER_PROCNUM_TYPE);

        //printf ("new: ptr=%p size=%lu bucket=%lu proc=%d\n",
        //        pReturn - sizeof(RECYCLER_PROCNUM_TYPE),
        //        ulSize, ulBucket, m_ulCreatingProcnum);
	return pReturn;
    }
}

/**
 * \brief               CacheNew - Allocate memory via the cache.
 * \param ulSize [in]   Size of requested memory allocaiton.
 * \return              Pointer to allocated memory.
 */
XXXinline char*
MemCache::CacheNew(size_t ulSize)
{

#if _USE_MEMCACHE
    if (g_bFastMalloc == TRUE && g_pMemCacheList)
    {
        int pnum = Process::get_procnum();
        if(pnum > 1)
        {
            MemCache* pMemCache = g_pMemCacheList[pnum];
            if (pMemCache)
                return pMemCache->_CacheNew(ulSize);
        }
    }
#endif

    return _SharedAlloc(ulSize);

}

/**
 * \brief               MSize - Return size of alloc at the given pointer
 * \param ptr [in]      Pointer to a block of memory allocated via MemCache
 * \return              Size of memory allocation for ptr
 */
size_t
MemCache::MSize(void* ptr)
{
    char* pPtr = (char*)ptr;

    pPtr -= sizeof(RECYCLER_PROCNUM_TYPE);
    UINT32 ulSignature = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pPtr) & 0xffff0000);

    if (ulSignature != 0xabba0000)
        pPtr += sizeof(RECYCLER_PROCNUM_TYPE);

    return *(size_t*)(pPtr - sizeof(size_t));
}

/**
 * \brief               _CacheDelete - Delete memory allocated via the cache
 * \param pPtr [in]     Pointer to a block of memory allocated via MemCache that should be deleted
 * \return              void
 */
XXXinline void
MemCache::_CacheDelete(char* pPtr)
{
    ASSERT(Process::get_procnum() == m_ulCreatingProcnum);

    m_ulCachedDelete += 1;

    pPtr -= sizeof(RECYCLER_PROCNUM_TYPE);
    UINT32 ulSignature = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pPtr) & 0xffff0000);
    UINT32 ulMemProcnum = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pPtr) & 0x0000ffff);

    if (ulMemProcnum > MAX_THREADS || ulSignature != 0xabba0000)
    {
        pPtr += sizeof(RECYCLER_PROCNUM_TYPE);
        _SharedFree(pPtr);
       return;
    }

#ifdef _WIN32
    pPtr -= sizeof(size_t);
    UINT32 ulSize = *((size_t*)pPtr);
#else
    UINT32 ulSize = SharedMemory::getsize(pPtr);
#endif
    UINT32 ulBucket = ulSize >> BUCKET_FACTOR;
    //printf ("delete: ptr=%p size=%lu bucket=%lu proc=%d memproc=%d\n",
    //        pPtr, ulSize, ulBucket, m_ulCreatingProcnum, ulMemProcnum);

    if (ulMemProcnum != m_ulCreatingProcnum)
    {
        // This block was allocated by another process, so try to pass
        // it over to that process's MemCache, adding the block to
        // it's "recycle bin".

        //printf("MC:_CD: proc %d deleting %p size %d allocated from proc %d\n",
        //       m_ulCreatingProcnum, pPtr, ulSize, ulMemProcnum);
        if (m_memCacheArray[ulMemProcnum] &&
            m_memCacheArray[ulMemProcnum]->_RecyclerAdd(pPtr, ulBucket))
        {
            //printf ("CacheDelete: passing ptr %p from proc %d to proc %d\n",
            //        pPtr, m_ulCreatingProcnum, ulMemProcnum);
            return;
        }
    }

    if (ulBucket < NUM_MC_BUCKETS &&
        (m_pBlockCount[ulBucket] < m_pMaxBucketEntries[ulBucket] - 1))
    {
        if (m_pMemBlocks[ulBucket])
        {
	    m_pMemBlocks[ulBucket][m_pBlockCount[ulBucket]] = pPtr;
            m_pBlockCount[ulBucket]++;
        }
        else
        {
#ifdef MEMCACHE_DEBUG
            printf("MemCache: alloc memblocks bucket=%d ptr=%p size=%d proc=%d\n",
                   ulBucket, pPtr, ulSize, m_ulCreatingProcnum);
#endif
            m_pMemBlocks[ulBucket] = (char**)_SharedAlloc(sizeof(char*) *
                                                   m_pMaxBucketEntries[ulBucket]);
            memset(m_pMemBlocks[ulBucket], 0,
                   m_pMaxBucketEntries[ulBucket] * sizeof(char*));
	    m_pMemBlocks[ulBucket][m_pBlockCount[ulBucket]] = pPtr;
            m_pBlockCount[ulBucket]++;
        }
    }
    else
    {
#ifdef MEMCACHE_DEBUG
	printf ("MemCache: bucket %d full [ptr=%p size=%lu nb=%lu]\n",
                ulBucket, pPtr, ulSize, 
                (ulBucket < NUM_MC_BUCKETS) ? m_pBlockCount[ulBucket] : 0);
#endif

	/*
	 * If this happens a lot you might expand the upper bound
         * of bucket sizes to include the current size
	 */
	_SharedFree(pPtr);
    }
}

/**
 * \brief               CacheDelete - Delete memory allocated via the cache
 * \param pPtr [in]     Pointer to a block of memory allocated via MemCache that should be deleted
 * \return              void
 */
XXXinline void
MemCache::CacheDelete(char* pPtr)
{
    if (!pPtr)
        return;

#if _USE_MEMCACHE
    if (g_bFastMalloc == TRUE && g_pMemCacheList)
    {
        int pnum = Process::get_procnum();
        if(pnum > 1)
        {
            MemCache* pMemCache = g_pMemCacheList[pnum];
            if (pMemCache)
            {
                pMemCache->_CacheDelete(pPtr);
	        return;
            }
        }
    }
#endif

    char* pOrigPtr = pPtr;
    pPtr -= sizeof(RECYCLER_PROCNUM_TYPE);
    UINT32 ulSignature = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pPtr) & 0xffff0000);
    UINT32 ulMemProcnum = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pPtr) & 0x0000ffff);
#ifdef _WIN32
    pPtr -= sizeof(size_t);
#endif
    if (ulMemProcnum > MAX_THREADS || ulSignature != 0xabba0000)
    {
        pPtr = pOrigPtr;
    }

    _SharedFree(pPtr);

}

/**
 * \brief               CacheCalloc - Allocate and null-fill memory
 * \param ulSize [in]   Requested size of new allocaiton
 * \return              Pointer to new memory block, zero-filled
 */
XXXinline char*
MemCache::CacheCalloc(size_t ulSize)
{
    char* pRet = CacheNew(ulSize);
    if (pRet)
        memset(pRet, 0, ulSize);
    return pRet;
}

/**
 * \brief                _CacheRealloc - Reallocate memory if necessary
 * \param pPtr [in]      Pointer to a block of memory allocated via MemCache that should be resized
 * \param ulNewSize [in] Requested size of new allocaiton
 * \return               Pointer to new memory block, possibly the same as pPtr
 */
XXXinline char*
MemCache::_CacheRealloc(char* pPtr, size_t ulNewSize)
{
    ASSERT(Process::get_procnum() == m_ulCreatingProcnum);

    char* pOrigPtr = pPtr;
    pPtr -= sizeof(RECYCLER_PROCNUM_TYPE);
    UINT32 ulSignature = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pPtr) & 0xffff0000);
    UINT32 ulMemProcnum = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pPtr) & 0x0000ffff);

    if (ulMemProcnum > MAX_THREADS || ulSignature != 0xabba0000)
    {
        pPtr += sizeof(RECYCLER_PROCNUM_TYPE);
        return (char*)SharedMemory::realloc(pPtr, ulNewSize);
    }

#ifdef _WIN32
    pPtr -= sizeof(size_t);
    UINT32 ulOldSize = *((size_t*)pPtr);
#else
    UINT32 ulOldSize = SharedMemory::getsize(pPtr);
#endif

    char* pRet = NULL;
    if (ulNewSize > ulOldSize)
    {
        pRet = CacheNew(ulNewSize);
        memcpy (pRet, pOrigPtr, ulOldSize);
    }
    else
    {
        pRet = pOrigPtr;
    }

    return pRet;
}

/**
 * \brief               CacheRealloc - Reallocate memory if necessary
 * \param pPtr [in]     Pointer to a block of memory allocated via MemCache that should be resized
 * \param ulSize [in]   Requested size of new allocaiton
 * \return              Pointer to new memory block, possibly the same as pPtr
 */
XXXinline char*
MemCache::CacheRealloc(char* pPtr, size_t ulSize)
{
    if (!ulSize)
    {
        CacheDelete(pPtr);
        return NULL;
    }
    if (!pPtr)
    {
        return CacheNew(ulSize);
    }

    if (g_bFastMalloc == TRUE && g_pMemCacheList)
    {
        int pnum = Process::get_procnum();
        if(pnum > 1)
        {
            MemCache* pMemCache = g_pMemCacheList[pnum];
            if (pMemCache)
            {
                return pMemCache->_CacheRealloc(pPtr, ulSize);
            }
    
        }
    }

    if (shared_ready)
    {
        return (char*)SharedMemory::realloc(pPtr, ulSize);
    }
    else
    {
        return (char*)realloc(pPtr, ulSize);
    }
}

/**
 * \brief               CacheBlockValidate - Validate memory guard bits for a given pointer
 * \param pPtr [in]     Pointer to a block of memory allocated via MemCache that should be validated.
 * \return              Boolean check of whether the guard bits are still valid for this memory block.
 */
XXXinline BOOL
MemCache::CacheBlockValidate(char* pPtr)
{

    /// \warning This check is disabled and always returns TRUE for performance reasons, it will be validated when the memory is returned to the SharedMemory pool.
    return TRUE;

#if 0 /// \todo XXXDC FIX OR DELETE THIS

#if _USE_MEMCACHE
    if (g_bFastMalloc == TRUE)
    {
#ifdef _WIN32
        pPtr -= sizeof(size_t);
#endif
        pPtr -= sizeof(RECYCLER_PROCNUM_TYPE);
    }
#endif

    if (shared_ready)
    {
        return SharedMemory::checkblock(pPtr);
    }
    else
    {
        return TRUE;
    }
#endif
}

/**
 * \brief               CacheBlockSize - Return the size of the underlying SharedMemory alloc at the given pointer
 * \param pPtr [in]     Pointer to a block of memory allocated via MemCache.
 * \return              Size of the underlying SharedMemory allocation used for this memory block.
 */
XXXinline INT32
MemCache::CacheBlockSize(char* pPtr)
{
#if _USE_MEMCACHE
    if (g_bFastMalloc == TRUE && g_pMemCacheList && Process::get_procnum() > 1)
    {
        pPtr += sizeof(size_t);
        pPtr += sizeof(RECYCLER_PROCNUM_TYPE);
    }
#endif

    if (shared_ready)
    {
        return SharedMemory::blocksize(pPtr);
    }
    else
    {
        return 0;
    }
}

/**
 * \brief               _SharedAlloc - Allocate memory from the SharedMemory subsystem
 * \param ulSize [in]   Requested size of new allocation.
 * \return              Pointer to new memory block.
 */
XXXinline char*
MemCache::_SharedAlloc(size_t ulSize)
{
    if (shared_ready)
    {
        return (char*)SharedMemory::malloc(ulSize);
    }
    else
    {
        return (char*)malloc(ulSize);
    }
}

/**
 * \brief               _SharedFree - Return memory to the SharedMemory subsystem
 * \param pPtr [in]     Pointer to a block of memory allocated via MemCache that should be deleted.
 * \return              void
 */
XXXinline void
MemCache::_SharedFree(char* pPtr)
{
    if (shared_ready)
    {
        if (pPtr)
        {
            SharedMemory::free((char *)pPtr);
        }
    }
    else
    {
        if (pPtr)
        {
            free(pPtr);
        }
    }
}


/**
 * \brief                       GetAndResetStats - \b DEPRICATED
 */
XXXinline void
MemCache::GetAndResetStats(UINT32* pCachedMallocs,
                           UINT32* pCachedMisses,
                           UINT32* pCachedNew,
                           UINT32* pCachedDelete)
{
    *pCachedMallocs = 0;
    *pCachedMisses  = 0;
    *pCachedNew     = 0;
    *pCachedDelete  = 0;
    MemCache* pMemCache;

    if (g_pMemCacheList)
    {
        for (int i=0; i < MAX_THREADS; ++i)
        {
            pMemCache = g_pMemCacheList[i];
            if (pMemCache)
            {
                *pCachedMallocs += pMemCache->m_ulCachedMallocs;
                *pCachedMisses  += pMemCache->m_ulCachedMisses;
                *pCachedNew     += pMemCache->m_ulCachedNew;
                *pCachedDelete  += pMemCache->m_ulCachedDelete;
                pMemCache->m_ulCachedMallocs = 0;
                pMemCache->m_ulCachedMisses  = 0;
                pMemCache->m_ulCachedNew     = 0;
                pMemCache->m_ulCachedDelete  = 0;
            }
        }
    }
}

/**
 * \brief                       Aggregate RSS counters into a single set.
 * \param pCachedMallocs [in]   Pointer to a counter to update with the number of allocs that were fulfilled from the cache.
 * \param pCachedMisses [in]    Pointer to a counter to update with the number of allocs that were not fulfilled from the cache.
 * \param pCachedNew [in]       Pointer to a counter to update with the number of allocs requested from the cache.
 * \param pCachedDelete [in]    Pointer to a counter to update with the number of deletes requested.
 */
XXXinline void
MemCache::GetStats(UINT32* pCachedMallocs,
                   UINT32* pCachedMisses,
                   UINT32* pCachedNew,
                   UINT32* pCachedDelete)
{
    *pCachedMallocs = 0;
    *pCachedMisses  = 0;
    *pCachedNew     = 0;
    *pCachedDelete  = 0;
    MemCache* pMemCache;

    if (g_pMemCacheList)
    {
        for (int i=0; i < MAX_THREADS; ++i)
        {
            pMemCache = g_pMemCacheList[i];
            if (pMemCache)
            {
                *pCachedMallocs += pMemCache->m_ulCachedMallocs;
                *pCachedMisses  += pMemCache->m_ulCachedMisses;
                *pCachedNew     += pMemCache->m_ulCachedNew;
                *pCachedDelete  += pMemCache->m_ulCachedDelete;
            }
        }
    }
}


/**
 * \brief               _RecyclerAdd - Add a pointer to recycler for this bucket/cache
 * \param pPtr [in]     Pointer to a block of memory allocated via MemCache 
 * \param ulBucket [in] Bucket that the memory block should be added to.
 * \return              Success/Failure.
 *
 * _RecyclerAdd - Add a block to the Recycler for this MemCache.
 *
 * \attention Important: this is called by other MemCache objects, not us.
 *
 * The pPtr block should include the procnum bytes, and the bucket number
 * should be computed using the full block size including the saved
 * procnum.  Althouth there is a mutex here the caller does not collide
 * with us, only with other callers passing in blocks to the same bucket.
 * This is because of the circular queue we're using.  In the situation
 * where proc A alloctes and proc B deallocates there should be few if
 * any actual mutex collisions.  Additinally, the mutex is held for
 * a very brief time, reducing contention even further.
 */
XXXinline BOOL
MemCache::_RecyclerAdd(char* pPtr, UINT32 ulBucket)
{
    if (ulBucket >= NUM_MC_BUCKETS)
    {
        // too big, just return
        return FALSE;
    }

    RecyclerQueue* pQueue = &m_pRecycler[ulBucket];
    HXMutexLock(pQueue->m_pMutex);
    UINT32 nNewTail = (pQueue->m_ulRecyclerTail + 1) % RECYCLER_QLEN;
    if (nNewTail != pQueue->m_ulRecyclerHead)
    {
        if (!pQueue->m_pRecyclerQueue)
            pQueue->m_pRecyclerQueue = (char**)_SharedAlloc(sizeof(char*) * RECYCLER_QLEN);
        pQueue->m_pRecyclerQueue[pQueue->m_ulRecyclerTail] = pPtr;
        pQueue->m_ulRecyclerTail = nNewTail;
        HXMutexUnlock(pQueue->m_pMutex);
        return TRUE;
    }
    else
    {
        HXMutexUnlock(pQueue->m_pMutex);
        //printf("MC:_RecyclerAdd: passing ptr %p to proc %d [bucket=%d]: "
        //       "queue full\n", pPtr, m_ulCreatingProcnum, ulBucket);
        return FALSE;
    }
}


/**
 * \brief               _RecyclerGet - Allocate memory from the recycler if available
 * \param ulSize [in]   Size of block to retrieve from the Recycler for this MemCache.
 * \param ulBucket [in] Bucket that the memory block should be retrieved from.
 * \return              Success/Failure.
 *
 * _RecyclerGet - look for a block in our Recycler, which contains
 * blocks handed to us by other MemCache instances running in other procs.
 *
 * \par
 * Note how we can grab blocks from the queue without the need for
 * using a mutex.  This is very important!  Only the owner of this 
 * MemCache object is allowed to modify m_ulRecyclerHead.
 */
XXXinline char*
MemCache::_RecyclerGet(size_t ulSize, UINT32 ulBucket)
{
    if (ulBucket >= NUM_MC_BUCKETS)
    {
        // too big, just return
        return 0;
    }


    RecyclerQueue* pQueue = &m_pRecycler[ulBucket];
#if defined PTHREADS_SUPPORTED && _LINUX
    // XXXJJ This is a temporary fix for beta2
    /* We have unexplainable memory corruption on linux 2.6 platform using pthread.
    
       It looks to me after a thread put a memory block back to another thread's recycler,
       it keeps operate on the block for a very brief period. So if the other thread gets
       the block right away, we have a memory corruption.
       
       The fix here is to avoid getting the "hot potato" other threads just put in. Unless
       the recycler has more than one item, we will return NULL.
    */

    if (pQueue->m_ulRecyclerHead  + 1 == pQueue->m_ulRecyclerTail
        || pQueue->m_ulRecyclerHead  == pQueue->m_ulRecyclerTail)
#else
    if (pQueue->m_ulRecyclerHead == pQueue->m_ulRecyclerTail)
#endif //PTHREADS_SUPPORTED && _LINUX
    {
        //printf("MC:_RecyclerGet: recycler empty for proc %d [bucket=%d]\n",
        //   m_ulCreatingProcnum, ulBucket);
        // empty queue
        return 0;
    }

    ASSERT(pQueue->m_pRecyclerQueue);

    char* pReturn = pQueue->m_pRecyclerQueue[pQueue->m_ulRecyclerHead];
    pQueue->m_ulRecyclerHead = (pQueue->m_ulRecyclerHead + 1) % RECYCLER_QLEN;

    char* pPtr = pReturn;

#ifdef _WIN32
    *((size_t*)pReturn) = ulSize;
    pReturn += sizeof(size_t);
#endif

    UINT32 ulSignature = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pReturn) & 0xffff0000);
    UINT32 ulMemProcnum = (UINT32)(*((RECYCLER_PROCNUM_TYPE*)pReturn) & 0x0000ffff);
    ASSERT(m_ulCreatingProcnum == ulMemProcnum && ulSignature == 0xabba0000);

    pReturn += sizeof(RECYCLER_PROCNUM_TYPE);

    //printf("MC:_RecyclerGet: got ptr %p for proc %d [bucket=%d]\n",
    //       pPtr, m_ulCreatingProcnum, ulBucket);

    return pReturn;

}



#ifdef MEMCACHE_DUMP
/**
 * \brief               CacheDump - Dump detailed counters for the cache.
 * \param               none
 * \return              void
 *
 * CacheDump - Dump detailed counters for the cache.
 * \par
 * This gets called by MemCacheSigHandler() in _main.cpp (if enabled)
 * and is used to dump the number of entries in each bucket for each
 * process.  It can be called at any time to dump the cache bucket info.
 */
XXXinline void
MemCache::CacheDump(void)
{
    int i=0, j=0;
    for (i=0; i < MAX_THREADS; ++i)
    {
        if (g_pMemCacheList && g_pMemCacheList[i] && g_pMemCacheList[i]->m_pBlockCount)
        {
            int nTot = 0;

            for (j=0; j < NUM_MC_BUCKETS; ++j)
            {
                if (g_pMemCacheList[i]->m_pBlockCount[j])
                {
                    nTot += g_pMemCacheList[i]->m_pBlockCount[j];
                }
            }

            printf("    MemCache: Proc:%-2d  Blocks:%-5d Buckets: ", 
                   g_pMemCacheList[i]->m_ulCreatingProcnum, nTot);

            for (j=0; j < NUM_MC_BUCKETS; ++j)
            {
                if (g_pMemCacheList[i]->m_pBlockCount[j])
                {
                    printf(" %d:%lu", j, g_pMemCacheList[i]->m_pBlockCount[j]);
                }
            }
            printf("\n");
        }
    }
}
#endif /* MEMCACHE_DUMP */

