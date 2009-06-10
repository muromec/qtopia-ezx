/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pthread_compat.cpp,v 1.4 2004/07/17 18:18:24 dcollins Exp $ 
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
/*
 *  pthread_compat.cpp
 *
 *  This file provides compatibility with pthreads on linux.
 *  It contains pthread-like stubs/wrappers to make the server more
 *  compatable with the linux glibc which assumes non-pthreads means
 *  non-threaded and doesn't properly mutex protect internal data
 *  structures when clone() is used directly.  These stubs/wrappers are not
 *  needed if pthreads is used for our threading model rather than clone(),
 *  and are not linked into the server if PTHREADS_SUPPORTED is defined_.
 *  The two are mutually-exclusive.
 */

#include <pthread.h>
#include <errno.h>

// some versions of pthread.h don't define these unless __UNIX_98 is defined
// I tried defining that but I still couldn't get it to work, so I'm just
// going to define them, here
extern "C" int __pthread_mutexattr_settype(pthread_mutexattr_t* pAttr,
                                           int kind);
extern "C" int __pthread_mutexattr_gettype(const pthread_mutexattr_t* pAttr,
                                           int* pKind);
extern "C" void _pthread_cleanup_push_defer(struct _pthread_cleanup_buffer* pBuffer,
                                            void (*pRoutine)(void*),
                                            void* pArg);
extern "C" void _pthread_cleanup_pop_restore(struct _pthread_cleanup_buffer* pBuffer,
                                             int bExecute);

#include "hxtypes.h"
#include "hxcom.h"
#include "hxslist.h"
#include "mutex.h"
#include "proc.h"

extern "C" {
    /*
     * this structure provides maximum binary compatibilty with what glibc
     * expects to be in the pthread_mutex_t struct, but it uses our mutex as
     * the spinlock
     */
    typedef struct hx_pthread_mutex_t
    {
        int m_unused1;
        int m_count;
        int m_owner;
        int m_kind;
        HX_MUTEX_TYPE lock;
        int m_unused2;
    } hx_pthread_mutex_t;
}

// we have to define these this way since various versions of pthread.h
// define different constants and we need to work with both.
#define PTHREAD_MUTEX_FAST_NP_HX 0
#define PTHREAD_MUTEX_ADAPTIVE_NP_HX 3

CHXSimpleList *g_PThreadMutexCleanupStack[MAX_THREADS];

int
__pthread_mutex_init(pthread_mutex_t* pPthrMutex,
                     const pthread_mutexattr_t* pAttr)
{
    hx_pthread_mutex_t* pMutex = (hx_pthread_mutex_t*)pPthrMutex;
    if (pAttr)
        pMutex->m_kind = pAttr->__mutexkind;
    else
        pMutex->m_kind = PTHREAD_MUTEX_FAST_NP_HX;

    pMutex->m_count = 0;
    pMutex->m_owner = -1;
    HXMutexInit(&pMutex->lock);

    return 0;
}

int
__pthread_mutex_destroy(pthread_mutex_t* pPthrMutex)
{
    hx_pthread_mutex_t* pMutex = (hx_pthread_mutex_t*)pPthrMutex;
    if (HXMutexTryLock(&pMutex->lock))
        return 0;

    return EBUSY;
}

int
__pthread_mutex_trylock(pthread_mutex_t* pPthrMutex)
{
    hx_pthread_mutex_t* pMutex = (hx_pthread_mutex_t*)pPthrMutex;
    switch (pMutex->m_kind)
    {
    case PTHREAD_MUTEX_FAST_NP_HX:
    case PTHREAD_MUTEX_ADAPTIVE_NP_HX:
        if (HXMutexTryLock(&pMutex->lock))
            return 0;
        else
            return EBUSY;

    case PTHREAD_MUTEX_RECURSIVE_NP:
    {
        int procnum = Process::get_procnum();

        if (pMutex->m_owner == procnum)
        {
            pMutex->m_count++;
            return 0;
        }

        if (HXMutexTryLock(&pMutex->lock))
        {
            pMutex->m_owner = procnum;
            pMutex->m_count = 0;
            return 0;
        }

        return EBUSY;
    }

    case PTHREAD_MUTEX_ERRORCHECK_NP:
        if (HXMutexTryLock(&pMutex->lock))
        {
            pMutex->m_owner = Process::get_procnum();
            return 0;
        }
        return EBUSY;

    default:
        return EINVAL;
    }
}

int
__pthread_mutex_lock(pthread_mutex_t* pPthrMutex)
{
    hx_pthread_mutex_t* pMutex = (hx_pthread_mutex_t*)pPthrMutex;
    switch (pMutex->m_kind)
    {
    case PTHREAD_MUTEX_FAST_NP_HX:
    case PTHREAD_MUTEX_ADAPTIVE_NP_HX:
        HXMutexLock(&pMutex->lock, TRUE);
        return 0;

    case PTHREAD_MUTEX_RECURSIVE_NP:
    {
        int procnum = Process::get_procnum();
        if (pMutex->m_owner == procnum)
        {
            pMutex->m_count++;
        }
        else
        {
            HXMutexLock(&pMutex->lock, TRUE);
            pMutex->m_owner = procnum;
            pMutex->m_count = 0;
        }

        return 0;
    }

    case PTHREAD_MUTEX_ERRORCHECK_NP:
    {
        int procnum = Process::get_procnum();
        if (pMutex->m_owner == procnum)
            return EDEADLK;

        HXMutexLock(&pMutex->lock, TRUE);
        pMutex->m_owner = procnum;

        return 0;
    }

    default:
        return EINVAL;
    }
}

int
__pthread_mutex_unlock(pthread_mutex_t* pPthrMutex)
{
    hx_pthread_mutex_t* pMutex = (hx_pthread_mutex_t*)pPthrMutex;
    switch (pMutex->m_kind)
    {
    case PTHREAD_MUTEX_FAST_NP_HX:
    case PTHREAD_MUTEX_ADAPTIVE_NP_HX:
        HXMutexUnlock(&pMutex->lock);
        return 0;

    case PTHREAD_MUTEX_RECURSIVE_NP:
        if (pMutex->m_owner != Process::get_procnum())
            return EPERM;

        if (pMutex->m_count > 0)
        {
            pMutex->m_count--;
        }
        else
        {
            pMutex->m_owner = -1;
            HXMutexUnlock(&pMutex->lock);
        }

        return 0;

    case PTHREAD_MUTEX_ERRORCHECK_NP:
        if (pMutex->m_owner != Process::get_procnum())
            return EPERM;

        pMutex->m_owner = -1;
        HXMutexUnlock(&pMutex->lock);

        return 0;

    default:
        return EINVAL;
    }
}

int
__pthread_mutexattr_init(pthread_mutexattr_t* pAttr)
{
    pAttr->__mutexkind = PTHREAD_MUTEX_FAST_NP_HX;
    return 0;
}

int
__pthread_mutexattr_destroy(pthread_mutexattr_t* pAttr)
{
    return 0;
}

int
__pthread_mutexattr_settype (pthread_mutexattr_t* pAttr, int kind)
{
    pAttr->__mutexkind = kind;
    return 0;
}

int
__pthread_mutexattr_gettype (const pthread_mutexattr_t* pAttr, int* pKind)
{
    *pKind = pAttr->__mutexkind;
    return 0;
}

void
_pthread_cleanup_push_defer(struct _pthread_cleanup_buffer* pBuffer,
                            void (*pRoutine)(void*),
                            void* pArg)
{
    pBuffer->__routine = pRoutine;
    pBuffer->__arg = pArg;
    UINT32 procnum = Process::get_procnum();

    if (!g_PThreadMutexCleanupStack[procnum])
    {
         g_PThreadMutexCleanupStack[procnum] = new CHXSimpleList;
    }
    (g_PThreadMutexCleanupStack[procnum])->AddHead((void*)pBuffer);
}

void
_pthread_cleanup_pop_restore(struct _pthread_cleanup_buffer* pBuffer,
                             int bExecute)
{
    UINT32 procnum = Process::get_procnum();
    
    if (!g_PThreadMutexCleanupStack[procnum])
    {
         return;
    }
    *pBuffer = *(_pthread_cleanup_buffer*)((g_PThreadMutexCleanupStack[procnum])->RemoveHead());
 
    if (bExecute)
    {
        pBuffer->__routine(pBuffer->__arg);
    }
} 


//////////////////////////////////////////////////////////////////////
// The rest of these are necessary because the common/system library
// (syslib.a) defines routines (when _UNIX_THREADS_SUPPORTED is
// defined) which use pthreads and sem_* routines.  While these
// routines are unused in the server, they result in unresolved
// symbols with gcc 3.x unless they are defined here.  (We can't
// and don't want to link in -lpthreads for various reasons.)
//
// Even though they're unused, we go ahead and define those
// that we can just in case, the rest are defined to assert
// and return a failure code.
//

int
pthread_create(pthread_t* thread, const pthread_attr_t* attr,
               void *(*start_routine) (void *), void* arg)
{
    HX_ASSERT(0); 
    return -1;
}

int
pthread_cancel(pthread_t thread)
{
    HX_ASSERT(0); 
    return -1;
}

int
pthread_join(pthread_t th, void** thread_return)
{
    HX_ASSERT(0); 
    return -1;
}

int
pthread_mutexattr_init(pthread_mutexattr_t* attr)
{
   return __pthread_mutexattr_init(attr);
}

int
pthread_mutexattr_destroy(pthread_mutexattr_t* attr)
{
    return __pthread_mutexattr_destroy(attr);
}

int
pthread_mutexattr_settype(pthread_mutexattr_t* attr, int kind)
{
    return __pthread_mutexattr_settype(attr, kind);
}

int
pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int* kind)
{
    return __pthread_mutexattr_gettype(attr, kind);
}

int
pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr)
{
    return __pthread_mutex_init(mutex, mutexattr);
}

int
pthread_mutex_lock(pthread_mutex_t* mutex)
{
    return __pthread_mutex_lock(mutex);
}

int
pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    return __pthread_mutex_trylock(mutex);
}

int
pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    return __pthread_mutex_unlock(mutex);
}

int
pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    return __pthread_mutex_destroy(mutex);
}

// These sem_* operators wouldn't be difficult to define using our
// atomic operators and mutex, but nobody should be calling them
// anyhow, so there's no point.
#include <semaphore.h>
int sem_init(sem_t* sem, int pshared, unsigned int value) { HX_ASSERT(0); return -1; }
int sem_wait(sem_t* sem)    { HX_ASSERT(0); return -1; }
int sem_trywait(sem_t* sem) { HX_ASSERT(0); return -1; }
int sem_post(sem_t* sem)    { HX_ASSERT(0); return -1; }
int sem_destroy(sem_t* sem) { HX_ASSERT(0); return -1; }
int sem_getvalue(sem_t* sem, int* sval) { HX_ASSERT(0); return -1; }

//////////////////////////////////////////////////////////////////////
