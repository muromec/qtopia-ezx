/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cpuident.c,v 1.17 2005/04/28 23:40:17 ehyche Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#include "hlxclib/string.h" // memset

#include "cpuident.h"

/*
 * check if this cpu supports the cpuid instruction.
 * returns: 1 if support, 0 else
 */

int hasCPUID(void)
{
    int has_cpuid = 0 ;

#if defined(_M_IX86) && defined(_MSC_VER)
    /* we are compiling under MSVC, on x86 */
    __asm {
        pushfd                  // Get original EFLAGS
        pop eax                 // Get original EFLAGS
        mov ecx,    eax         // save original EFLAGS
        xor eax,    0x200000     // Flip ID bit in EFLAGS
        push        eax         // Save new EFLAGS value on stack
        popfd                   // Replace current EFLAGS value
        pushfd                  // Get new EFLAGS
        pop eax                 // Get new EFLAGS
        xor eax,    ecx         // did state toggle?
        and eax,    0x200000
        mov has_cpuid, eax
    }
#endif

#if defined(__i386) && defined(__GNUC__)
    /* we are compiling under gcc */
    __asm__ __volatile (
      "pushfl;"
      "popl %%eax;"
      "movl %%eax, %%ecx;"
      "xorl $0x200000, %%eax;"
      "pushl %%eax;"
      "popfl;"
      "pushfl;"
      "popl %%eax;"
      "xorl %%ecx, %%eax;"
      "andl $0x200000, %%eax;"
      "movl %%eax,%0;"
    :"=m" (has_cpuid)
    : /*no input*/
    : "eax","ecx","cc"
    );
#endif

    return (has_cpuid != 0) ;
}

/*
 * Issue a cpuid instruction
 */

#if defined(_M_IX86) || defined(__i386)
static void cpuid(long cmd, long* a, long *b, long *c, long *d)
{
    long la=0,lb=0,lc=0,ld=0 ;
#if defined(_MSC_VER)
    /* we are compiling under MSVC, on x86 */
    __asm {
        mov eax,cmd
        push ebx /* save ebx */
        __emit 0x0F /* cpuid was not supported by MSVC5, thus emit it */
        __emit 0xA2
        mov ebx,esi
        pop ebx /* restore ebx */
        mov la,eax
        mov lb,esi
        mov lc,ecx
        mov ld,edx
    }
#endif /* MSVC */

#if defined(__GNUC__)
    /* we are compiling under gcc */
    __asm__ __volatile (
      "movl %4, %%eax;"
      "push %%ebx;"
      "cpuid;"
      "movl %%ebx,%%esi;"
      "popl %%ebx;"
      "movl %%eax, %0;"
      "movl %%esi, %1;"
      "movl %%ecx, %2;"
      "movl %%edx, %3;"
    : "=m" (la), "=m" (lb), "=m" (lc), "=m" (ld)
    : "m" (cmd)
    : "eax","ecx","edx","esi","cc"
    );
#endif /* GNUC */

    *a = la ; *b = lb ; *c = lc ; *d = ld ;
}
#endif /* x86 architecture */

#if defined(_ARM)

#if defined(__linux)
#define has_cpuid() 0 /* we can't currently cpuid under ARM/linux */
#else
#define has_cpuid() 1
#endif

#if defined (__GNUC__)
static int cpu_id(void)
{
    int ret ;

#if defined(__MARM_THUMB__)
    ret = 0x4180720; // XXXNH: until we fix this assembly
#else
    __asm__ ("mrc p15, 0, %0, c0, c0, 0" : "=r" (ret)) ;
#endif
    return ret ;
}
#endif /* GNUC */

#if defined(_MSC_VER)
/* cpu_id is defined in an assembly file */
extern int cpu_id(void);
#endif

#endif /* ARM */

/*
 * routines to check for operating system support for the SSE floating point
 * instructions.
 * Intel recommends to check for OS support by executing an instruction which
 * will fault (SIGILL) and throw an exception in the absence of OS support.
 * Catching the exception signals lack of support.
 *
 * An alternative method would be to check CR4.OSFXSR, but alas, that can only
 * be done if you run in ring 0. Duh.
 */

#if defined __i386 && defined __linux
/* to catch the exception caused by SSE instructions on an OS that does not support
   saving the additional SSE CPU state, we need a signal handler here. If called
   as a result of SIGILL, it will modify eax, and return to the code that caused
   the trap, jumping past the offending instruction.
   */

#include <signal.h>

static void myHandler(int signal, struct sigcontext sc)
{
    // signal "no SSE support" by modifying eax (it is 1 on entry)
    sc.eax = 0 ;
    // we get here because orps xmm0,xmm0 has raised an exception. To avoid
    // repeating this instruction, we increment the PC by three before
    // returning.
    sc.eip += 3 ;
}
#endif

int bOSHasSSEsupport(void)
{
#if defined(_WIN32) && defined(_M_IX86)
    int bSSE = 1 ;
    // this is a little tricky. For Windows, we use __try/__except.
    // Stack unwinding does not need to be enabled because we
    // catch all exceptions; you can ignore the compiler warning.
#pragma warning(disable:4530)
    __try {
        // Use opcode bytes for the instruction because MSVC and gcc
        // compilers can't compile it yet.
        // __asm orps xmm0,xmm0

        __asm {
	    _emit 0x0f
	    _emit 0x56
	    _emit 0xc0
	      }
    }
    __except ( 1 /* EXCEPTION_EXECUTE_HANDLER */ )
    {
        // if an exception occurred, no OS support for xmm register save
        // and restore so don't use SSE nor SSE2 instructions
        bSSE = 0 ;
    }
#pragma warning(default:4530)
    return bSSE ;

#elif defined(__i386) && defined(__linux)
    // On linux, we use an exception handler

    struct sigaction savedSIGILLHandler ;
    struct sigaction newSIGILLHandler ;

    int bSSE ;

    memset (&newSIGILLHandler, 0, sizeof newSIGILLHandler);
    sigemptyset (&newSIGILLHandler.sa_mask);
    newSIGILLHandler.sa_handler =  (void (*)(int))myHandler;

    // install new signal handler
    sigaction(SIGILL, &newSIGILLHandler, &savedSIGILLHandler) ;

    // execute an MME instruction. If it fails, the exception handler
    // will be called, which will modify eax (bSSE)

    __asm__ __volatile ("mov $1,%0\n\t"
			".byte 0x0f,0x56,0xc0\n\t" /* xorps xmm0,xmm0 */
			: "=a" (bSSE)) ;

    // restore signal handler
    sigaction(SIGILL,&savedSIGILLHandler,NULL) ;
    return bSSE ;
#else
    return 0 ; // no OS SSE support detectable
#endif
}


enum
{
    ulBitMM = 1<<23,
    ulBitEM = 1<<2, /* emulation enabled / disabled */
    ulBitMSR = 1<<5,
    ulBitTSC = 1<<4,
    ulBitXMM = 1<<25,/* Streaming SIMD Extensions-enabled? */
    ulBitSSE2 = 1<<26
} ;

void
CPUIdentify(CPUInformation* pInfo)
{
    int id;
    memset(pInfo, 0, sizeof(*pInfo)) ;
    pInfo->architecture = ulArchitectureUNKNOWN ;

#if defined(_M_IX86) || defined(__i386)
    /* we assume we are running at least on 386 or compatible */
    pInfo->architecture = ulArchitectureIntel ;
    pInfo->specific.m_x86.family = ulFamily80386 ;

    if (hasCPUID())
    {
        long a,b,c,d ;

        /* if we have cpuid, we are at least 486 */
        pInfo->specific.m_x86.family = ulFamily80486 ;

        /* First, invoke cpuid(0, ....). eax (returned in a) tells us
           the maximum p which we can call cpuid(p, ...) with. Pentium
           and later will have p >= 1. */
        cpuid (0,&a,&b,&c,&d) ;

        /* cpuid(1,...) checks for additional processor features. */
        if (a >= 1)
        {
            cpuid (1,&a,&b,&c,&d) ;

            pInfo->specific.m_x86.family   = (IntelCPUFamily)((a & 0xf00) >> 8) ;
            pInfo->specific.m_x86.model    = (a & 0x0f0) >> 4 ;
            pInfo->specific.m_x86.stepping = (a & 0x00f) ;
            pInfo->specific.m_x86.hasPMC = (pInfo->specific.m_x86.family == 6)  ;
            pInfo->specific.m_x86.hasMSR = (d & ulBitMSR) ;
            pInfo->specific.m_x86.hasTSC = (d & ulBitTSC) ;

            if (d & ulBitMM)
            {
                pInfo->specific.m_x86.hasPMC  = 1 ;
                pInfo->specific.m_x86.hasMMX  = 1 ;
                pInfo->specific.m_x86.hasSSE  = (d & ulBitXMM)  ;
                pInfo->specific.m_x86.hasSSE2 = (d & ulBitSSE2) ;
            }
        }
    }

	if (pInfo->specific.m_x86.hasSSE || pInfo->specific.m_x86.hasSSE2)
	{
		// SSE / SSE2 floating point instructions require OS FXSAVE
        // and FXRSTOR support.

        pInfo->specific.m_x86.hasSSE_OSSupport = bOSHasSSEsupport() ;
	}
#endif // if defined i386

#ifdef _ARM
    if (!has_cpuid())
        return ;

    id = cpu_id() ;

    switch (id & 0xf000)
    {
    case 0x0<<12: // pre-ARM7
        pInfo->specific.m_arm.family = ulFamily_PREARM7 ;
	pInfo->specific.m_arm.implementor = ARM_IMPL_ARM ;
	switch (id >> 4) {
	case 0x4156030: pInfo->architecture = ulArchitectureARM_ARCH2 ; break ;
	case 0x4156060:
	case 0x4156061:
	case 0x4156062:
	    pInfo->architecture = ulArchitectureARM_ARCH3 ; break ;
	default:
	    pInfo->architecture = ulArchitectureUNKNOWN ; break ;
	}
	break ;

    case 0x7<<12: // ARM7
        pInfo->specific.m_arm.family        = ulFamily_ARM7 ;
	pInfo->architecture  = ((id >> 23) & 1) ? ulArchitectureARM_ARCH4T : ulArchitectureARM_ARCH3 ;
	pInfo->specific.m_arm.implementor   = id >> 24 ;
	pInfo->specific.m_arm.variant       = (id >> 16) & 0x7f ;
	pInfo->specific.m_arm.primaryPartNo = (id >> 4) & 0xfff ;
	pInfo->specific.m_arm.revision      = id & 0xf ;

    default: // post-ARM7
	pInfo->specific.m_arm.family        = ulFamily_POSTARM7 ;
	pInfo->specific.m_arm.implementor   = id >> 24 ;
	pInfo->specific.m_arm.variant       = (id >> 20) & 0xf ;
	pInfo->architecture  = (CPUArchitecture)(((id >> 16) & 0xf) - 1 + ulArchitectureARM_ARCH4);
	pInfo->specific.m_arm.primaryPartNo = (id >> 4) & 0xfff ;
	pInfo->specific.m_arm.revision      = id & 0xf ;
	if (pInfo->specific.m_arm.implementor == ARM_IMPL_Intel &&
            pInfo->architecture == ulArchitectureARM_ARCH5TE &&
            (id >> 13) & 7 == 1)
	{
            // XScale
            pInfo->specific.m_arm.specific.XScale.coreGeneration = (id >> 13) & 7 ; // "1"
            pInfo->specific.m_arm.specific.XScale.coreRevision   = (id >> 10) & 7 ;
            pInfo->specific.m_arm.specific.XScale.productNumber  = (id >> 4) & 63 ;
	}
	break ;
    }
#endif /* _ARM */
}



