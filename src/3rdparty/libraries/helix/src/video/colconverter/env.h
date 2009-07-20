/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: env.h,v 1.5 2004/07/09 18:36:28 hubbe Exp $
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

/* standard libraries: */
#include "hlxclib/limits.h"
#include "hlxclib/string.h"
#include "hlxclib/math.h"

/* some math. constants: */
#ifndef M_PI
#define M_PI      3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2   1.41421356237309504880
#endif

/* fast rounding to a nearest integer: */
#if defined(_M_IX86) && !defined(WINCE_EMULATOR)
static __inline int NEAREST_INT(double a)
{
    int ia;
    __asm   fld     a
    __asm   fistp   ia
    return ia;
}
#elif defined(__i386) && defined(__linux__)
static __inline int NEAREST_INT(double a)
{
    int ia;
    __asm__ __volatile__ (
        " fldl %1; "
        " fistpl %0; "
        : "=m"(ia)
        : "m"(a)
        : "memory", "st"
        );
       
    return ia; 
}
#else
#define NEAREST_INT(a) ((int) (((a)<0)? ((a)-0.5): ((a)+0.5)))
#endif

/* byte packing: */
#ifdef __linux__
#include <endian.h>
#endif
#ifndef BYTE_ORDER
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
/* make sure your system is being correctly identified here !!! */
#if defined(ultrix) || defined(__alpha) || defined(__i386__) || defined(__i486__) || defined(_X86_) || defined (_M_IX86)
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN
#endif
#endif

/* check platform specific constants: */
#if CHAR_BIT != 8 || (defined( _INTEGRAL_MAX_BIT) && _INTEGRAL_MAX_BITS < 32) \
|| (defined(WORD_BIT) && WORD_BIT < 32)
#error Error!!! This code is written for 32+ bit processors.
#endif

#ifndef ENV_MAIN
#define ENV_EXTERN  extern
#else
#define ENV_EXTERN  /**/
#endif

/* Intel MMX & SSE extensions: */
#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS))  && !defined(WINCE_EMULATOR)
ENV_EXTERN int _x86_MMX_Available;
ENV_EXTERN int _x86_SSE_Available;
#endif

/* AltiVec extensions */
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
ENV_EXTERN int _AltiVec_Available;
#endif
