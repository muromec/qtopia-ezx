/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmx_util.c,v 1.7 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "mmx_util.h"

/*
 * Check if current CPU supports Intel MMX instruction set.
 * Use:
 *  int checkMmxAvailablity ();
 * Returns:
 *
 *    Bit mask for the above defines.
 *
 * Side effects:
 */
int checkMmxAvailablity()
{
    /* 1 if MMX instruction set is available: */
    int  mmxAvailable = 0;
    int  kniAvailable = 0;
    int  nRetMask = 0;

#ifdef _M_IX86
    __asm {
        pushfd                  // Get original EFLAGS
        pop     eax             // Get original EFLAGS
        mov     ecx,eax         // Save original EFLAGS
        xor     eax,240000h     // Flip ID & AC bits in EFLAGS
        push    eax             // Save new EFLAGS value on  stack
        popfd                   // Replace current EFLAGS value
        pushfd                  // Get new EFLAGS
        pop     eax             // Store new EFLAGS in EAX
        xor     eax,ecx         // Can not toggle ID & AC bits
        cmp     eax,240000h     // check if both ID & AC were inversed
        jnz     no_mmx
        push    ebx             // save ebx
        mov     eax,1
        _emit   0x0f            // CPUID
        _emit   0xa2
        shr     edx,23          // shift to MM FLAG: 800000h
        pop     ebx             // restore ebx
        mov     eax,edx
        shr     edx,2           // shift to KNI FLAG
        and     eax,1
        and     edx,1
        jmp     set_flags
no_mmx:
        xor     eax,eax
        xor     edx,edx
set_flags:
        mov     mmxAvailable,eax
        mov     kniAvailable,edx
    }

#elif defined __i386 && ( defined __linux || defined __svr4__ || defined VXWORKS )
        int family = 0;
        int model = 0, stepping = 0;
        int has_mmx = 0, has_kni = 0;

    __asm__ __volatile (
        "       pushl   %%eax               \n"
        "       pushl   %%ebx               \n"
        "       pushl   %%ecx               \n"
        "       pushl   %%edx               \n"
        "       pushfl                      \n"
        "       popl    %%eax               \n"
        "       movl    %%eax, %%ebx        \n"
        "       xorl    $0x200000, %%eax    \n"
        "       pushl   %%eax               \n"
        "       popfl                       \n"
        "       pushfl                      \n"
        "       popl    %%eax               \n"
        "       xorl    %%ebx, %%eax        \n"
        "       andl    $0x200000, %%eax    \n"
        "       jz      jumphere            \n"
        "       xorl    %%eax, %%eax        \n"
        "       cpuid                       \n"
        "       or      %%eax, %%eax        \n"
        "       jz      jumphere            \n"
        "       movl    $1, %%eax           \n"
        "       cpuid                       \n"
        "       movl    %%eax, %%ecx        \n"
        "       andl    $0xf00, %%ecx       \n"
        "       shrl    $8, %%ecx           \n"
        "       movl    %%ecx, %0           \n"
        "       movl    %%eax, %%ecx        \n"
        "       andl    $0xf0, %%ecx        \n"
        "       shrl    $4, %%ecx           \n"
        "       movl    %%ecx, %1           \n"
        "       movl    %%eax, %%ecx        \n"
        "       andl    $0x0f, %%ecx        \n"
        "       movl    %%ecx, %2           \n"
        "       movl    %%edx, %%eax        \n"
        "       andl    $0x800000, %%eax    \n"
        "       jz      jumphere            \n"
        "       mov     $1, %%eax           \n"
        "       mov     %%eax, %3           \n"
        "       and     $0x2000000, %%edx   \n"
        "       mov     %%edx, %4           \n"
        "jumphere:                          \n"
        "       popl    %%edx               \n"
        "       popl    %%ecx               \n"
        "       popl    %%ebx               \n"
        "       popl    %%eax               \n"
        :"=m" (family),"=m" (model), "=m" (stepping), "=m" (has_mmx), "=m" (has_kni)
        : /*no input*/
    );

    mmxAvailable = has_mmx;
    kniAvailable = has_kni;

#else // defined(_WIN32) && defined(_M_IX86)


#endif /* _M_IX86 */

    if( mmxAvailable )
        nRetMask |= CPU_HAS_MMX;

    if( kniAvailable )
        nRetMask |= CPU_HAS_KNI;

    return nRetMask;
}

