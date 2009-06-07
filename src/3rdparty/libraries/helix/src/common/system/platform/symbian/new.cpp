/*============================================================================*
 *
 * (c) 1995-2002 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *============================================================================*/
 
// We can only overload new/malloc on the platform; dll stuff on emulator prevents this.


#ifdef __MARM__

#include <e32base.h>
#include <estlib.h>
#include <string.h>
#include "memorymonitor.h"

#ifndef __NO_THROW
#define __NO_THROW
#endif

void OutOfMemory()
{
    SymbianMemoryMonitor *pMonitor = SymbianMemoryMonitor::Instance();
    if (pMonitor)
    {
        pMonitor->SendEvent(SymbianMemoryMonitor::EOutOfMemory);
    }
}

void* operator new(size_t size) __NO_THROW
{
    /* based on Symbian document, passing 
       size > KMaxTInt/2 causes the Alloc function
       to throw USER 47 exception.  In order to avoid
       the exception such that the player will gracefully 
       shutdown with out of memory condition,  the following 
       check is added.*/
    
    void *p = NULL;
    if (size < (KMaxTInt/2))
    {
        p = User::Alloc(size);
    }

    if (!p)
    {
        OutOfMemory();
    }
    return p;
}

void* operator new[](size_t size) __NO_THROW
{
    void *p = NULL;
    if (size < (KMaxTInt/2))
    {
        p = User::Alloc(size);
    }

    if (!p)
    {
        OutOfMemory();
    }
    return p;
}

void operator delete(void* p) __NO_THROW
{
    if (p)
	User::Free(p);
}

void operator delete[](void* p) __NO_THROW
{
    if (p)
	User::Free(p);
}

extern "C"
{
    void* malloc(size_t size);
    void* realloc(void* pOld, size_t size);
    void free(void* p);
}

void* malloc(size_t size)
{
    void *p = NULL;
    if (size < (KMaxTInt/2))
    {
        p = User::Alloc(size);
    }

    if (!p)
    {
        OutOfMemory();
    }

    return p;
}

void* realloc(void* pOld, size_t size)
{
    void *p = NULL;
    if (size < (KMaxTInt/2))
    {
        p = User::ReAlloc(pOld, size);
    }

    if (p == NULL)			// allocation failure
	{
        OutOfMemory();
    }

    return p;
}

void free(void* p)
{
    if (p)
	User::Free(p);
}


#endif // __MARM__
