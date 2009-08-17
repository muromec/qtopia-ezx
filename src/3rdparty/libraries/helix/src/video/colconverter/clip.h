/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clip.h,v 1.3 2004/07/09 18:36:28 hubbe Exp $
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

#ifndef __CLIP_H__
#define __CLIP_H__   1

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Clipping constants:
 *
 * Note: the CLIPNEG/CLIPPOS numbers are somewhat rounded values
 *  that correspond to settings of GAMMA_MAX, KAPPA_MAX, and BETA_MAX
 *  parameters defined in "yuv.h". If you are planning to modify any
 *  of these constants, the sizes of the clip tables shall be
 *  adjusted as well.
 */
#define CLIPRNG 256                 /* unclipped range */
#define CLIPNEG (288 * 4)           /* rounded max neg. value */
#define CLIPPOS (288 * 4)           /* rounded max shot over 256 */

/* ## of bits to be left after clipping: */
#define MIN_CLIP_BITS   4
#define MAX_CLIP_BITS   8

/*
 * Rounding & dithering constants:
 */
#define _ROUND(b)           ((1U << (MAX_CLIP_BITS-(b))) >> 1)
#ifndef NO_DITHER
#define DITHER_LOW(b)       ((1U << (MAX_CLIP_BITS-(b))) >> 2)
#define DITHER_HIGH(b)      ((3U << (MAX_CLIP_BITS-(b))) >> 2)
#else   /* just correct rounding: */
#define DITHER_LOW(b)       _ROUND(b)
#define DITHER_HIGH(b)      _ROUND(b)
#endif

/*
 * Actual clip tables:
 */
#ifndef CLIP_MAIN
#define CLIP_EXTERN  extern
#else
#define CLIP_EXTERN  /**/
#endif
CLIP_EXTERN unsigned char clip[MAX_CLIP_BITS-MIN_CLIP_BITS+1][CLIPNEG+CLIPRNG+CLIPPOS]; /* Flawfinder: ignore */

/*
 * Clipping & dithering macros:
 */
#define _CLIP(b,v)          clip[(b)-MIN_CLIP_BITS][(v)+CLIPNEG]
#define CLIP_ROUND(b,v)     _CLIP(b,v+_ROUND(b))
#define CLIP_LOW(b,v)       _CLIP(b,v+DITHER_LOW(b))
#define CLIP_HIGH(b,v)      _CLIP(b,v+DITHER_HIGH(b))

/*
 * Generic clipping macro:
 */
#define CLIP(r,b,v)         CLIP_##r(b,v)

#ifdef __cplusplus
}
#endif

#endif /* __CLIP_H__ */

