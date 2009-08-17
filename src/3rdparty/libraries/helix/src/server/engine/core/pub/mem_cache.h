/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mem_cache.h,v 1.10 2006/07/14 20:46:19 ghori Exp $ 
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
#ifndef _MEM_CACHE_H_
#define _MEM_CACHE_H_

#define _USE_MEMCACHE  1                ///< Enable/Disable MemCache
//#define MEMCACHE_DEBUG 1                ///< Enable/Disable MemCache debugging
//#define MEMCACHE_DUMP 1                 ///< Enable/Disable MemCache detailed RSS output

#include "hxtypes.h"
#include "platform_config.h"
#include "hxcom.h"
#include "mutex.h"

#ifdef _WIN32
#include "llcrtsup.h"
#endif

class MemCache;

extern int shared_ready;                ///< Indicates whether the SharedMemory subsystem is ready or not
extern BOOL g_bFastMalloc;              ///< Enable/disable memory cache usage
extern MemCache** g_pMemCacheList;      ///< Global pointer to memory cache list

/**
 * \brief               MemCache - Implements a per-thread memory cache.
 * 
 *  Implements a per-thread memory cache (or per-process, depending on the
 *  platform) to reduce mutex contention in the shared memory allocator.
 *  Avoiding this mutex contention is critical for server performance.
 */
class MemCache
{
public:
                        MemCache(int procnum);
                        MemCache(int procnum, MemCache* pMemCache);
                        ~MemCache();

    static char*        CacheNew(size_t ulSize);
    static void         CacheDelete(char* pPtr);
    static char*        CacheCalloc(size_t ulSize);
    static char*        CacheRealloc(char* pPtr, size_t ulNewSize);
    static size_t       MSize(void* ptr);
    static BOOL         CacheBlockValidate(char* pPtr);
    static INT32        CacheBlockSize(char* pPtr);
    static void         GetAndResetStats(UINT32* pCachedMallocs,
                                         UINT32* pCachedMisses,
                                         UINT32* pCachedNew,
                                         UINT32* pCachedDelete);
    static void         GetStats(UINT32* pCachedMallocs,
                                 UINT32* pCachedMisses,
                                 UINT32* pCachedNew,
                                 UINT32* pCachedDelete);
#ifdef MEMCACHE_DUMP
    static void         CacheDump(void);
#endif

private:
    void                _Init(int procnum);
    void                _CacheDelete(char* pPtr);
    char*               _CacheNew(size_t ulSize);
    char*               _CacheRealloc(char* pPtr, size_t ulNewSize);
    BOOL                _RecyclerAdd(char* pPtr, UINT32 ulBucket);
    char*               _RecyclerGet(size_t ulSize, UINT32 ulBucket);

    static char*        _SharedAlloc(size_t ulSize);
    static void         _SharedFree(char* pPtr);

    UINT32*     m_pBlockCount;          ///< Array counting number of elements in each bucket
    UINT32*     m_pMaxBucketEntries;    ///< array of max bucket sizes per bucket, some buckets are allowed to be larger than others
    char***     m_pMemBlocks;           ///< The memory cache for this thread
    MemCache**  m_memCacheArray;        ///< Array of memory cache objects, one per thread
    UINT32      m_ulCreatingProcnum;    ///< The procnum of the process/thread which created this memory cache

    UINT32      m_ulCachedMallocs;      ///< RSS count of alloc requests that were fufilled from the cache
    UINT32      m_ulCachedMisses;       ///< RSS count of alloc requests that were *not* fufilled from the cache
    UINT32      m_ulCachedNew;          ///< RSS count of memory allocations requested from the cache
    UINT32      m_ulCachedDelete;       ///< RSS count of memory allocations that were released back to the cache

    /**
     * \brief           RecyclerQueue - used to pass blocks back to the process/thread that allocated them 
     *
     *  This "recycle bin" is used to pass blocks back to the process/thread
     *  that allocated them so that we don't "churn" the cache by repeatedly
     *  allocating in one and deallocating in another, which negates the
     *  usefulness of using the cache for the given bucket size.  It is best
     *  to avoid this pointer passing if at all possible, but at least with
     *  the recycler it will be less painful than hitting the main allocator.
     *
     *  \par
     *  Each instance of MemCache has an array of these, one per bucket.
     *  Each of these is a circular queue, so we have one queue per
     *  bucket, per instance of MemCache.  For each if these queues,
     *  the following rules apply:
     * 
     *   \li   empty: head == tail
     *   \li   full:  head == (tail + 1) mod queue_size 
     *
     */
    typedef struct 
    {
        HX_MUTEX m_pMutex;              ///< Mutex to protect this bucket's recycle bin
        UINT32   m_ulRecyclerHead;      ///< Where new additions to the recycle bin go
        UINT32   m_ulRecyclerTail;      ///< Where things removed from the recycle bin come from
        char**   m_pRecyclerQueue;      ///< The recycle bin itself, an array of pointers to allocs from this bucket
    } RecyclerQueue;
    RecyclerQueue* m_pRecycler;         ///< The "recycle bin" for this thread's cache, one per bucket.

};

/**
 * \def MEM_CACHE_MEM
 * Override the new/delete operators for a given class
 *
 * \note        This is not really that necessary any more due how we now
 *              override the general new/delete operators for everything.
 */
#define MEM_CACHE_MEM\
    void* operator new(size_t size, MemCache* pMalloc = NULL)\
    {\
        void* pMem;\
	if (pMalloc)\
	{\
	    pMem = pMalloc->CacheNew(size + sizeof(MemCache*));\
	}\
	else\
	{\
	    pMem = (void*)::new char[size + sizeof(MemCache*)];\
	}\
        *(MemCache**)pMem = pMalloc;\
        return ((unsigned char*)pMem + sizeof(MemCache*));\
    }\
\
    void operator delete(void* pMem)\
    {\
        pMem = (unsigned char*)pMem - sizeof(MemCache*);\
        MemCache* pMalloc = *(MemCache**)pMem;\
	if (pMalloc)\
	{\
	    pMalloc->CacheDelete((char *)pMem);\
	}\
	else\
	{\
	    ::delete[] (char*)pMem;\
	}\
    }\

/**
 * \def BUCKET_FACTOR 
 * Used to lump differently-sized allocs into the same bucket
 *
 * \def NUM_MC_BUCKETS
 * Number of buckets in a thread's memory cache
 *
 * \def NUM_MC_BUCKET_ENTRIES
 * Max number of entries in each bucket
 */


//#define BUCKET_FACTOR 4
//#define NUM_MC_BUCKETS 7
//#define NUM_MC_BUCKET_ENTRIES 6000

//#define BUCKET_FACTOR 4
//#define NUM_MC_BUCKETS 12
//#define NUM_MC_BUCKET_ENTRIES 6000

//#define BUCKET_FACTOR 2
//#define NUM_MC_BUCKETS 32
//#define NUM_MC_BUCKET_ENTRIES 6000

//#define BUCKET_FACTOR 1
//#define NUM_MC_BUCKETS 128
//#define NUM_MC_BUCKET_ENTRIES 6000

#define BUCKET_FACTOR 0
#define NUM_MC_BUCKETS 256
#define NUM_MC_BUCKET_ENTRIES 6000
/// a select few buckets (related to packet xmit) need a bigger bucket
#define NUM_MC_BUCKET_ENTRIES_EXTENDED 48000


#define RECYCLER_QLEN 2048              ///< Max size of each recycler queue

/// \todo XXXDC some platforms can use 32-bit alignment, optimize this:
#if 1
#define RECYCLER_PROCNUM_TYPE INT64
#else
#define RECYCLER_PROCNUM_TYPE INT32
#endif


#endif
