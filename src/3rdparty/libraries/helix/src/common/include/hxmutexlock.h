/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmutexlock.h,v 1.11 2007/07/06 20:43:42 jfinnecy Exp $
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
/*
 *  Non-reentrant mutexes for various server platforms.
 */


/***********************************************************************
 *  THIS CODE IS HIGHLY CRITICAL TO THE SERVER'S STABILITY!!!
 *  DO NOT MAKE CHANGES TO THE ATOMIC OPERATORS OR TO THE
 *  MUTEX CODE WITHOUT A SERVER TEAM CODE-REVIEW! (dev@helix-server)
 */


/***********************************************************************
 *
 *  Platforms supported:
 *    Windows / x86
 *    Solaris / sparc 
 *    IRIX / MIPS
 *    AIX / PowerPC
 *    Tru64(OSF) / Alpha
 *    HP-UX / PA-RISC
 *    HP-UX / Itanium
 *    Misc. Unix / x86  (Linux, FreeBSD...)
 *
 *  XXXDC: So far, in nearly all cases, native mutexes provided by
 *  OS vendors have been buggy and/or very slow.  Well-written 
 *  assembly-language mutexes are strongly preferred.
 *  Be especially careful about byte-alignment issues.
 *
 */

/***********************************************************************
 *
 * Externally-available routines:
 *
 * inline void HXMutexLock(HX_MUTEX pLock, HXBOOL bWait = FALSE)
 *     Locks the mutex, blocking until available;  A delay
 *     between attempts to grab the lock is used if bWait is TRUE.
 *     Note that the default is a tight no-delay spinlock, so
 *     always set bWait to TRUE unless you know you should do otherwise.
 *
 * inline HXBOOL HXMutexTryLock(HX_MUTEX pLock)
 *     Locks the mutex if available, returning TRUE if the lock succeeded,
 *     and returning FALSE otherwise.
 *
 * inline void HXMutexUnlock(HX_MUTEX pLock)
 *     Unlocks the mutex.
 *
 * inline HX_MUTEX HXMutexCreate()
 *     Creates the mutex.
 *
 * inline void HXMutexInit(HX_MUTEX pLock)
 *     Initializes the mutex.
 *
 * inline void HXMutexDestroy(HX_MUTEX pLock)
 *     Destroys the mutex.
 *
 *
 *  Additionally, this file defines IHXMutex, a COM wrapper for the above.
 *
 *
 * NOTES:
 * The older HXxxxMutex variations of the above are depreciated
 * and should be converted over the the above variations for consistancy.
 * HXMutexUnInit() is also depreciated, it never did anything and
 * the purpose of it has been lost.  It appears useless.
 *
 */

/***********************************************************************
 *
 * On most platforms the above are implemented using the
 * following internal routines/#defines (do NOT call these directly!):
 *
 * int _HXMutexSetBit(HX_MUTEX pLock)
 *     Lock the lock and return the old value of the lock
 *     (0 meaning it was unlocked and is now locked, non-zero otherwise)
 *
 * _HXMutexClearBit(pLock)    
 *     Clears the mutex flag.
 *
 * HXBOOL _HXMutexYieldCPUIfNeeded()
 *     On some platforms we have to explicitly yield the CPU.
 *     This is not needed on most platforms.
 *
 * void _HXMutexInitBit(HX_MUTEX pLock)
 *     Initializes the mutex, typically just calls _HXMutexClearBit().
 *
 * void _HXMutexDestroyBit(HX_MUTEX pLock)
 *     Destroys the mutex, typically does nothing.
 */

#ifndef _HXMUTEXLOCK_H_
#define _HXMUTEXLOCK_H_

/***********************************************************************
 * Common defines/includes
 */

//system
#include  <stdio.h>

//include
#include "hxtypes.h"

//hxmisc
#include "microsleep.h"

/***********************************************************************
 * OSF(Tru64) / Alpha defines/includes
 */
#if defined _OSF1
#include <alpha/builtins.h>

typedef struct
{
   char          p;
   unsigned long for_alignment;
} _HXMutexLockType;

#define HX_MUTEX volatile _HXMutexLockType* volatile
#define HX_MUTEX_TYPE volatile _HXMutexLockType
#define HX_MUTEX_BASE_TYPE _HXMutexLockType


/***********************************************************************
 * HP-UX / Itanium defines/includes
 */
#elif defined _HPUX && defined _IA64

#define HX_MUTEX volatile int* volatile
#define HX_MUTEX_TYPE volatile int
#define HX_MUTEX_BASE_TYPE int


/***********************************************************************
 * HP-UX / PA-RISC defines/includes
 */
#elif defined _HPUX

extern "C" {
    int load_and_clear(volatile char* volatile p);
};

typedef struct
{
    char      p;
    long long for_alignment;

} _HXMutexLockType;

#define HX_MUTEX volatile _HXMutexLockType* volatile
#define HX_MUTEX_TYPE volatile _HXMutexLockType  
#define HX_MUTEX_BASE_TYPE _HXMutexLockType  

// for 64-bit or 32-bit
#define alignedaddr(x) (volatile int *)((PTR_INT)(x) + 15 & ~0xf)

#define release_spinlock(LockArea)  if (1) {  \
     (*alignedaddr(LockArea) = 1); }

/***********************************************************************
 * AIX / PowerPC defines/includes
 */
#elif defined _AIX
#include <errno.h>
// NATIVE MUTEX
#include <sys/mman.h>
#define HX_MUTEX msemaphore*
#define HX_MUTEX_TYPE msemaphore
#define HX_MUTEX_BASE_TYPE msemaphore
// ASSEMBLY MUTEX
//#define HX_MUTEX volatile int* volatile
//#define HX_MUTEX_TYPE volatile int
//#define HX_MUTEX_BASE_TYPE int

/***********************************************************************
 * IRIX / MIPS defines/includes
 */
#elif defined _IRIX
#include <abi_mutex.h>
#define HX_MUTEX abilock_t*
#define HX_MUTEX_TYPE abilock_t
#define HX_MUTEX_BASE_TYPE abilock_t

/***********************************************************************
 * Windows x86 defines/includes
 */
#elif defined _WIN32
#include <winbase.h>
#define HX_MUTEX volatile int* volatile
#define HX_MUTEX_TYPE volatile int
#define HX_MUTEX_BASE_TYPE int

/***********************************************************************
 * generic defines/includes
 */
#else
#define HX_MUTEX volatile int* volatile
#define HX_MUTEX_TYPE volatile int
#define HX_MUTEX_BASE_TYPE int
#endif

/***********************************************************************
 * misc globals
 *
 * TODO: analyze more closely what the optimal delay is.
 */
#ifndef HX_MUTEX_DELAY
#define HX_MUTEX_DELAY 10000
#endif


// Backoff Configs
// XXXDC - Needs to be fine-tuned on a platform-specific basis!!!
// XXXDC - highly experimental!
#ifdef XXXDC_USE_BACKOFF
#define BACKOFF_THRESHOLD  5     // # failed locks before delay starts
#define BACKOFF_START      100   // usec to initially delay
#define BACKOFF_FACTOR     10    // muliply each delay by this
#define BACKOFF_MAX        15000 // maximum usec to delay
#endif


/***********************************************************************/
/***********************************************************************
 * Windows x86
 *
 * Implementation Notes:
 *   Uses assembly mutex
 */
#if defined _WIN32

#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */
#define _HXMutexClearBit(pLock)    (*(pLock) = 0)

inline static int
_HXMutexSetBit(HX_MUTEX pLock)
{
    register int nOldBit;
    _asm
    {
        mov eax, 1
        mov ebx, pLock
        xchg dword ptr [ebx], eax
        mov nOldBit, eax
    }
    return nOldBit;
}


/***********************************************************************
 * Solaris / sparc / gcc
 *
 * Implementation Notes:
 *   Uses assembly mutex
 *   GCC inline-assembly syntax
 */
#elif defined _SOLARIS && defined __GNUC__

#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */
#define _HXMutexClearBit(pLock)    (*(pLock) = 0)

inline static int
_HXMutexSetBit(HX_MUTEX pLock)
{
    volatile int nOldBit;
    __asm__ __volatile__("swap [%0], %1"
                        : "=r" (pLock), "=&r" (nOldBit)
                        : "0" (pLock), "1" (1));
    return nOldBit;
}

/***********************************************************************
 * Solaris / sparc / native compiler
 *
 * Implementation Notes:
 *   Uses assembly mutex
 *   native-compiler syntax
 */
#elif defined _SOLARIS /* native compiler */

extern "C" int _HXMutexSetBitCAS(HX_MUTEX pLock);
extern "C" int _HXMutexSetBitSWAP(HX_MUTEX pLock);
extern "C" int _HXMutexClearBit(HX_MUTEX pLock);

#define _HXMutexSetBit(pLock)      _HXMutexSetBitCAS(pLock)
#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */


/***********************************************************************
 * IRIX / MIPS
 *
 * Implementation Notes:
 *   Uses native mutex calls acquire_lock/etc.
 *   On this platform we explicitly yield the processor
 */
#elif defined _IRIX

#define _HXMutexYieldCPUIfNeeded() sginap(0)
#define _HXMutexSetBit(pLock)      acquire_lock(pLock)
#define _HXMutexClearBit(pLock)    release_lock(pLock)
#define _HXMutexInitBit(pLock)     init_lock(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */


/***********************************************************************
 * AIX / PowerPC
 *
 * Implementation Notes:
 *   Uses native mutex calls msem_lock/etc.
 */
#elif defined _AIX

// NATIVE MUTEX
//this doesn't work cleanly, we had to customize HXMutexLock() for AIX
#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexSetBit(pLock)      msem_lock(pLock, MSEM_IF_NOWAIT)
#define _HXMutexClearBit(pLock)    msem_unlock(pLock, 0)
#define _HXMutexInitBit(pLock)     msem_init(pLock, MSEM_UNLOCKED)
#define _HXMutexDestroyBit(pLock)  msem_remove(pLock)

// ASSEMBLY MUTEX
//#include <sched.h>
//#define _HXMutexYieldCPUIfNeeded() sched_yield()
//extern "C" int _HXMutexSetBit(HX_MUTEX pLock);
//extern "C" int _HXMutexClearBit(HX_MUTEX pLock);
//#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
//#define _HXMutexDestroyBit(pLock)  /* not needed */


/***********************************************************************
 * HP-UX / Itanium
 *
 * Implementation Notes:
 *   Uses non-inline assembly mutex call _HXMutexSetBit(), defined
 *   in common/util/platform/hpux/spinlock-ia64.s.   XXX: make this inline.
 */
#elif defined _HPUX && defined _IA64

extern "C" int _HXMutexSetBit(HX_MUTEX pLock);
extern "C" int _HXMutexClearBit(HX_MUTEX pLock);

#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */


/***********************************************************************
 * HP-UX / PA-RISC
 *
 * Implementation Notes:
 *   Uses non-inline assembly mutex call load_and_clear(), defined
 *   in common/util/platform/hpux/spin.s.   XXX: make this inline.
 *   The logic of this mutex is backwards from most of our others,
 *   1 means clear, 0 means set.
 */
#elif defined _HPUX

#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexSetBit(pLock)      (load_and_clear((char* volatile)pLock) == 0)
#define _HXMutexClearBit(pLock)   release_spinlock(pLock)
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */

/***********************************************************************
 * Tru64 (OSF/1) / Alpha
 *
 * Implementation Notes:
 *   Uses native mutex calls __LOCK_LONG_RETRY/etc.
 *
 * TODO:
 *   This native mutex is much slower than on other platforms.
 *   Change this to use assembly.
 */
#elif defined _OSF1
#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexSetBit(pLock)      (__LOCK_LONG_RETRY(pLock, 1) == 0)
#define _HXMutexClearBit(pLock)    __UNLOCK_LONG(pLock)
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */


/***********************************************************************
 * Macintosh OS X
 *
 * Implementation Notes:
 *   Uses assembly mutex
 *   GCC inline-assembly syntax
 *
 */
#elif defined _MAC_UNIX

inline static int
_HXMutexSetBit(int* pNum)
{
	volatile int result;
	volatile int hold;
        volatile int newval;
	__asm__ __volatile__ (
"1:           lwarx %3, 0, %2;\n"    // hold = *pNum
"             li %4, 1;\n"           // newval = 1
"             stwcx. %4, 0, %2;\n"   // *pNum = newval (if pNum has not been
                                     // corrupted by another thread between
                                     // lwarx and stwcx.)
"             bne- 1b;\n"            // if pNum HAS been corrupted by another thread, retry.
"             mr %0, %3;\n"          // result = hold (i.e. old *pNum)
              : "=b" (result)
              : "0" (result), "b" (pNum), "b" (hold), "b" (newval)
              : "cc", "memory"
              );

   return result;
}

#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexClearBit(pLock)    (*(int*)pLock = 0)
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */

/***********************************************************************
 * Misc. Unix / x86  (Linux, FreeBSD...)
 *
 * Implementation Notes:
 *   Uses assembly mutex
 *   GCC inline-assembly syntax
 *
 * TODO: add non-x86 support for Linux/FreeBSD/etc.
 */
#else

#define _HXMutexYieldCPUIfNeeded() /* not needed */
#define _HXMutexInitBit(pLock)     _HXMutexClearBit(pLock)
#define _HXMutexDestroyBit(pLock)  /* not needed */
#define _HXMutexClearBit(pLock)    (*(pLock) = 0)

/* 64-bit x86-64 */
#if defined(__amd64__) || defined(__x86_64__)
inline static int
_HXMutexSetBit(HX_MUTEX pLock)
{
    volatile int nOldBit;
    __asm__ __volatile__("xchg %%eax, (%%rbx)"
                        : "=a" (nOldBit)
                        : "a" (1), "b" (pLock));

    return nOldBit;
}

#else

/* 32-bit x86 */
/*
 * GCC gets really mad if we use eax / ebx in the assembly below.
 * We use ecx / edx instead (cross your fingers =)
 */
inline static int
_HXMutexSetBit(HX_MUTEX pLock)
{
    volatile int nOldBit;
    __asm__ __volatile__("xchg	%%ecx, (%%edx)"
			: "=c" (nOldBit)
			: "c" (1), "d" (pLock));

    return nOldBit;
}

#endif //defined(__amd64__) || defined(__x86_64__)

#endif


/***********************************************************************
 * Common code, shared by multiple platforms
 *
 */

#if !defined(HELIX_FEATURE_SERVER) && !defined(HX_MUTEX_PRIVATE_COLLISION_COUNTERS) && !defined(HX_MUTEX_PRIVATE_COLLISION_COUNTERS)
//default for non-server products is to not use the collision counters
#define HX_MUTEX_NO_COLLISION_COUNTERS
#endif
#if defined HX_MUTEX_NO_COLLISION_COUNTERS
#define HXMutexCollision(p)    /* nothing */
#elif defined HX_MUTEX_PRIVATE_COLLISION_COUNTERS
/* User must define their own HXMutexCollision(p) */
#else
extern HX_MUTEX g_pServerMainLock;
extern UINT32* g_pConcurrentOps;
extern UINT32* g_pConcurrentMemOps;

inline void
HXMutexCollision(HX_MUTEX pLock)
{
    ++(*g_pConcurrentOps);
    if (pLock != g_pServerMainLock)
        ++(*g_pConcurrentMemOps);
}
#endif

#ifndef HX_MUTEX_CUSTOM_COLLISION_DELAY
#define HXMutexCollisionDelay(n) microsleep(n)
#else
/* User must define their own HXMutexCollisionDelay() */
#endif

//will obsolete these old names shortly...
#define HXCreateMutex()   HXMutexCreate()
#define HXInitMutex(p)    HXMutexInit(p)
#define HXUnInitMutex(p)  HXMutexUnInit(p)
#define HXDestroyMutex(p) HXMutexDestroy(p)

#ifdef HX_MUTEX_USE_MANAGED_LOCKS
HX_RESULT AddManagedMutex(HX_MUTEX pMutex);
HX_RESULT RemoveManagedMutex(HX_MUTEX pMutex);
#endif

inline void
HXMutexLock(HX_MUTEX pLock, HXBOOL bWait = FALSE)
{
#ifdef XXXDC_USE_BACKOFF
    if (_HXMutexSetBit(pLock))
    {
        register int nInitialTries = 1;
 	register int nDelay = BACKOFF_START;
   
        //spin on reads for a bit (not writes)
        while (*pLock && nInitialTries < BACKOFF_THRESHOLD)
        {
            HXMutexCollision(pLock);
            ++nInitialTries;
        }
        while(_HXMutexSetBit(pLock))
        {
            _HXMutexYieldCPUIfNeeded();
            if (bWait)
            {
		HXMutexCollisionDelay(nDelay);
		nDelay = nDelay * BACKOFF_FACTOR;
		if (nDelay > BACKOFF_MAX)
		    nDelay = BACKOFF_MAX;
            }
            HXMutexCollision(pLock);
        }
    }
#elif defined _AIX
    do
    {
        if (msem_lock(pLock, 0) == 0) break;
    } while (errno == EINTR);
#else
    while(_HXMutexSetBit(pLock))
    {
        _HXMutexYieldCPUIfNeeded();
        if (bWait)
        {
            HXMutexCollisionDelay(HX_MUTEX_DELAY);
        }
        HXMutexCollision(pLock);
    }
#endif

#ifdef HX_MUTEX_USE_MANAGED_LOCKS
    AddManagedMutex(pLock);
#endif

}

inline HXBOOL
HXMutexTryLock(HX_MUTEX pLock)
{
#ifdef _AIX
    HXBOOL bRet = (msem_lock(pLock, 0) == 0);
#ifdef HX_MUTEX_USE_MANAGED_LOCKS
    if (bRet)
        AddManagedMutex(pLock);
#endif
    return bRet;
#else
    if (_HXMutexSetBit(pLock))
    {
        HXMutexCollision(pLock);
        return FALSE;
    }
#ifdef HX_MUTEX_USE_MANAGED_LOCKS
    AddManagedMutex(pLock);
#endif
    return TRUE;
#endif
}

inline void
HXMutexUnlock(HX_MUTEX pLock)
{
#ifdef HX_MUTEX_USE_MANAGED_LOCKS
    RemoveManagedMutex(pLock);
#endif
    _HXMutexClearBit(pLock);
}

inline HX_MUTEX
HXMutexCreate()
{
    HX_MUTEX pLock = (HX_MUTEX) new HX_MUTEX_BASE_TYPE;
    _HXMutexInitBit(pLock);
    return pLock;
}

inline void
HXMutexInit(HX_MUTEX pLock)
{
    _HXMutexClearBit(pLock);
}

inline void
HXMutexUnInit(HX_MUTEX pLock)
{
   /* no-op */
}

inline void
HXMutexDestroy(HX_MUTEX pLock)
{
    _HXMutexDestroyBit(pLock);
#if defined _WINDOWS
    delete (void*)pLock;
#else
    delete pLock;
#endif
}

#endif /* _HXMUTEXLOCK_H_ */
