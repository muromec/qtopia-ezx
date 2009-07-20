/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: shregion.cpp,v 1.8 2007/03/05 23:24:05 atin Exp $ 
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

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#if !defined _WIN32
#include <sys/mman.h>
#endif

#include "hxtypes.h"
#include "microsleep.h"
#include "_main.h"
#include "shregion.h"


extern UINT32 g_ulBackOffPercentage;
extern UINT32 g_ulSizemmap;

extern void* g_pSecondHeapBottom;
#ifdef _LINUX
extern void* g_pSbrkHeapBottom;
extern void* g_pSbrkHeapTop;
extern void* g_pDataSegmentBottom;
#endif

#ifdef _WIN32
void* g_pHeapBottom = NULL;
void* g_pHeapTop = NULL;
#endif

#ifdef _FREEBSD
#define START	1024		// 1 Gig
#else
#define START	256
#endif

SharedRegion::SharedRegion(INT32 _size)
    : m_region(0)
    , m_size(0)
    , m_second_heap_size(0)
{
    UINT32 ulStartSize = g_ulSizemmap ? g_ulSizemmap : START;
    UINT32 ulSize = ulStartSize;
    double lfBackOffFraction = (double)g_ulBackOffPercentage / 100.0;
    UINT32 ulDecrement = (UINT32)((double)ulSize * lfBackOffFraction);

    // try the requested mmap size twice with a small time interval between
    // them, in case some other app was running and it took up some mem for a
    // short time which caused the first mmap to fail. i know this is kinda 
    // hacky but its a small price to pay for only one iteration

    create_mmap(ulSize); // x-platform mmap'ing functionality in this method
    if (!m_region)
    {
	microsleep(1000000); // 1 sec

        while (ulSize >= MIN_SHMEM_SIZE)
        {
            create_mmap(ulSize);
            if (m_region)
                break;

            ulSize -= ulDecrement;
	}

	if (!m_region)
	{
            ulSize = (UINT32)((double)ulStartSize * lfBackOffFraction);
            ulDecrement = (UINT32)((double)ulSize * lfBackOffFraction);

            ulSize -= ulDecrement;
            while (ulSize >= MIN_SHMEM_SIZE)
            {
                create_mmap(ulSize);
                if (m_region)
                    break;

                ulSize -= ulDecrement;
            }

            // try the min allowable size for the server at least once
            if (!m_region)
            {
                ulSize = MIN_SHMEM_SIZE; // MB
                create_mmap(ulSize);
            }
	}
    }

#ifdef _WIN32
     g_pHeapBottom = m_region;
     g_pHeapTop = (char*)g_pHeapBottom + m_size;
#endif

    if (m_region && ulSize >= MIN_SHMEM_SIZE)
    {
#if !defined(_FREEBSD)
        suprintf("Server has allocated %d megabytes of memory\n", ulSize);
#endif
    }
    else
    {
        suprintf("Server failed to allocate at least %d Megs of memory\n", MIN_SHMEM_SIZE);
        exit(-1);
    }
}

SharedRegion::~SharedRegion()
{
#if defined _WIN32
    VirtualFree(m_region, 0, MEM_RELEASE);
#else
    munmap((char*)m_region, m_size);
#endif
}

void
SharedRegion::create_mmap(UINT32 ulSize)
{
#ifdef _IRIX
    m_size = 1024 * 1024 * ulSize;
    INT32 f1;
    f1 = open("/dev/zero", O_RDWR);
    m_region = (BYTE*)mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, f1, 
	0);
    if (m_region == MAP_FAILED)
    {
	m_region = NULL;
    }
#elif defined(_LINUX)

    m_size = 1024 * 1024 * ulSize;

    int f1 = open("/dev/zero", O_RDWR);
    int f2 = -1;

    if (f1 < 0)
        perror("failed to open /dev/zero");

#define SBRK_GUARDSIZE 65536

        // grabs the current data segment extent + room for a guard
        sbrk(SBRK_GUARDSIZE);
        g_pSbrkHeapBottom = sbrk(0);

        g_pSbrkHeapTop = g_pSbrkHeapBottom;

        size_t bestSize = m_size;
        g_pSbrkHeapTop = sbrk(bestSize);
        while (g_pSbrkHeapTop == (BYTE*)-1)	// keep looping until it works.
        {
            if (bestSize < 32 * 1024*1024)	// decrement 32 MB per try
            {
                break;
            }
            bestSize -= 32 * 1024*1024;
            g_pSbrkHeapTop = sbrk(bestSize);
        }

        g_pSbrkHeapTop = sbrk(0);
        g_pSecondHeapBottom = 0;

        // we need to create a secondary heap if we couldn't get enough memory
        // in the main one
        // Only do this if we need at least 16k more memory.
        if ((size_t)g_pSbrkHeapTop - (size_t)g_pSbrkHeapBottom < m_size - 16384)
        {
            size_t currSize = (size_t)g_pSbrkHeapTop - (size_t)g_pSbrkHeapBottom;
            m_second_heap_size = m_size - currSize;
            g_pSecondHeapBottom = mmap(0, m_second_heap_size,
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANON, -1, 0);
        }

        if (g_pSecondHeapBottom != MAP_FAILED)
        {
            m_region = (BYTE*)g_pSbrkHeapBottom;
        }
        else
        {
            m_region = (BYTE*)MAP_FAILED;
            m_second_heap_size = 0;
            // reset the sbrk'd heap to 0 size so on the next try everything
            // still works
            if (brk((BYTE*)g_pSbrkHeapBottom - SBRK_GUARDSIZE) == -1)
                perror("brk failed to reset heap size");
        }

        if (m_region != (BYTE*)MAP_FAILED)
        {
            // set up the guard
            BYTE* startProtect = (BYTE*)g_pSbrkHeapBottom - SBRK_GUARDSIZE + 16384; 
            size_t sizeProtect = 16384;
            mprotect(startProtect, sizeProtect, PROT_NONE);
        }

    if (m_region == MAP_FAILED)
    {
        perror("mmap failed");
        m_region = NULL;
        close(f1);
        if (f2 >= 0)
            close(f2);
    }

#elif defined (_SOLARIS)
    m_size = 1024 * 1024 * ulSize;
    INT32 f1;
    f1 = open("/dev/zero", O_RDWR);
    m_region = (BYTE*)mmap((caddr_t)0, m_size, (PROT_READ | PROT_WRITE),
	MAP_SHARED | MAP_NORESERVE, f1, 0);
    if(m_region == MAP_FAILED)
    {
	m_region = NULL;
    }
#elif defined(_AIX)
    m_size = 1024 * 1024 * ulSize;
    m_region = (BYTE*)mmap((caddr_t)0, m_size, (PROT_READ | PROT_WRITE), 
	(MAP_SHARED | MAP_ANONYMOUS), -1, 0);
    if(m_region == (BYTE *)MAP_FAILED)
    {
	suprintf("mmap failed %d\n", errno);
	m_region = NULL;
    }
#elif defined (_HPUX)
    m_size = 1024 * 1024 * ulSize;

    m_region = (BYTE*)mmap((caddr_t)0, m_size, (PROT_READ | PROT_WRITE),
	(MAP_ANONYMOUS | MAP_SHARED), -1, 0);

    if(m_region == (BYTE *)MAP_FAILED)
    {
	m_region = NULL;
    }          
#elif defined (_OSF1)
    m_size = (UINT64)1024 * (UINT64)1024 * (UINT64)ulSize;
    // INT32 f1;
    // f1 = open("/dev/zero", O_RDWR);
    m_region = (BYTE*)mmap((caddr_t)0, m_size, (PROT_READ | PROT_WRITE),
	MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    // check for -1 return value, which is 0xffffffffffffffff
    if(m_region == (BYTE *)0xffffffffffffffff)
    {
	m_region = NULL;
    }
#elif defined _WIN32
    m_size = 1024 * 1024 * ulSize;
    m_region = (BYTE*)malloc(32768);
#else
    m_size = 1024 * 1024 * ulSize;
    m_region = (BYTE*)mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_ANON |
	MAP_SHARED | MAP_HASSEMAPHORE, -1, 0);
    if(m_region == MAP_FAILED)
    {
	m_region = NULL;
    }
#endif

}

UINT32
SharedRegion::GetFirstHeapSize()
{
#ifdef _LINUX
    if (g_pSbrkHeapBottom)
    {
        return (size_t)g_pSbrkHeapTop - (size_t)g_pSbrkHeapBottom;
    }
    else
    {
        return m_size;
    }
#else
    return m_size;
#endif
}
