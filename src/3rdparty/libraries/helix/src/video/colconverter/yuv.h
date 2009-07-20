/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: yuv.h,v 1.4 2004/07/09 18:36:29 hubbe Exp $
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

#ifndef __YUV_H__
#define __YUV_H__   1

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header contains a number of data declarations that supposed
 * to be shared between several modules. There should be only one module
 * that would own these data (i.e. declare them locally as public data),
 * while the others just should be able to access them (i.e. they would
 * need to see them declared as "extern"). The following macros would
 * allow us to combine both types of data declarations in a single place.
 */
#ifndef YUV_MAIN
#define YUV_EXTERN  extern
#else
#define YUV_EXTERN  /**/
#endif

/*
 * CCIR Rec. 601-2 definition of YCrCb color space.
 *
 * Given a primary analogue RGB signal (R,G,and B components are in the
 * range of 0..1 volt), the luminance signal is defined as:
 *      Y = 0.299 R + 0.587 G + 0.114 B.
 * Signals of color-defferences, are therefore:
 *      (R-Y) = 0.701 R - 0.587 G - 0.114 B, and
 *      (B-Y) = -0.299 R - 0.587 G + 0.886 B.
 * Using normalization (we want (R-Y) and (B-Y) to be in the range of
 * -0.5..0.5 volt), we will obtain:
 *      Cr = 0.5/0.701 (R-Y), and
 *      Cb = 0.5/0.886 (B-Y).
 * Finally, the quantized versions of Y,Cr,and Cb are defined as:
 *      Y'  = 219 Y + 16,
 *      Cr' = 224 Cr + 128, and
 *      Cb' = 224 Cb + 128.
 * For convenience, we will use the following names for these constants
 * (here and below: V = Cr, and U = Cb):
 */
#define CCIR601_YRCOEF  0.299
#define CCIR601_YGCOEF  0.587
#define CCIR601_YBCOEF  0.114

#define CCIR601_YMAX    219
#define CCIR601_YMED    ((double)CCIR601_YMAX/2.)
#define CCIR601_YOFFSET 16

#define CCIR601_VMAX    224
#define CCIR601_VOFFSET 128

#define CCIR601_UMAX    224
#define CCIR601_UOFFSET 128

/*
 * Also, we typically will deal with quantized R'G'B' signal, represented
 * by 8-bit quantities for R', G' and B' components:
 */
#define RGB_MAX ((1U << 8) - 1)

/*
 * Forward R'G'B'->Y'Cr'Cb' conversion:
 *  a) calculate Y":
 *      Y" = 0.299 R' + 0.587 G' + 0.114 B';		=> 3 muls/lookups
 *  b) calculate Y',Cr',Cb' values:
 *      Y'  = 219/255 Y" + 16;				   		=> 1 mul/lookup
 *      Cr' = 224/255 * 0.5/0.701 (R'-Y") + 128;	=> 1 mul/lookup
 *      Cb' = 224/255 * 0.5/0.886 (B'-Y") + 128;	=> 1 mul/lookup
 */
#define YSCALE  ((double)CCIR601_YMAX/RGB_MAX)
#define VSCALE  ((double)CCIR601_VMAX/RGB_MAX * 0.5/(1.-CCIR601_YRCOEF))
#define USCALE  ((double)CCIR601_UMAX/RGB_MAX * 0.5/(1.-CCIR601_YBCOEF))

#define YMAX    (RGB_MAX + 3)       /* include max. rounding error */
#define VMAX    (int)((double)(1.-CCIR601_YRCOEF) * RGB_MAX + 1)
#define VMIN    VMAX
#define UMAX    (int)((double)(1.-CCIR601_YBCOEF) * RGB_MAX + 1)
#define UMIN    UMAX

/* R'G'B' -> Y'Cr'Cb' conversion tables: */
YUV_EXTERN int yrtab [RGB_MAX+1], ygtab [RGB_MAX+1], ybtab [RGB_MAX+1];
YUV_EXTERN int yytab [YMAX+1], vrytab [VMIN+VMAX+1], ubytab [UMIN+UMAX+1];

/*
 * Backward Y'Cr'Cb'->R'G'B' conversion:
 *  a) calculate Y":
 *      Y"  = 1/(219/255) (Y'-16);				   		=> 1 mul/lookup
 *  b) calculate R'B':
 *      R'  = Y" + 1/(224/255 * 0.5/0.701) (Cr'-128);	=> 1 mul/lookup
 *      B'  = Y" + 1/(224/255 * 0.5/0.886) (Cb'-128); 	=> 1 mul/lookup
 *  c) calculate G':
 *      G'  = 1/0.587 (Y" - 0.299 R' - 0.114 B') =
 *          = Y" - 0.299/0.587/(224/255 * 0.5/0.701) (Cr'-128)
 *               - 0.114/0.587/(224/255 * 0.5/0.886) (Cb'-128); => 2 muls/luts
 */
#define YCOEF   (1/YSCALE)
#define RVCOEF  (1/VSCALE)
#define GVCOEF  (-CCIR601_YRCOEF/CCIR601_YGCOEF/VSCALE)
#define GUCOEF  (-CCIR601_YBCOEF/CCIR601_YGCOEF/USCALE)
#define BUCOEF  (1/USCALE)

/* Y'Cr'Cb'->R'G'B' conversion tables: */
YUV_EXTERN int ytab  [RGB_MAX+1], rvtab [RGB_MAX+1], gvtab [RGB_MAX+1];
YUV_EXTERN int gutab [RGB_MAX+1], butab [RGB_MAX+1];
YUV_EXTERN int rutab [RGB_MAX+1], bvtab [RGB_MAX+1]; /* alpha != 0 */

/*
 * Color adjustments.
 *  a) hue: in NTSC YUV color space, the hue adjustment is equivalent
 *    to the UV axes rotation:
 *      V' = V cos(alpha) - U sin(alpha);
 *      U' = V sin(alpha) + U cos(alpha);
 *    where:
 *      V = (R-Y)/1.14;
 *      U = (B-Y)/2.03;
 *    In YCrCb color space, this will be equivalent to:
 *      Cr' - 128 = (Cr - 128) cos(alpha) - zeta (Cb - 128) sin(alpha));.
 *      Cb' - 128 = xi (Cr - 128) sin(alpha) + (Cb - 128) cos(alpha);
 *    where:
 *      xi   = (1./(0.5/0.701)/1.14)/(1./(0.5/0.886)/2.03);
 *      zeta = 1/xi;
 *  b) saturation:
 *      Cr' - 128 = beta (Cr - 128);
 *      Cb' - 128 = beta (Cb - 128);
 *  c) brightness:
 *      Y' - 16 = gamma (Y - 16);
 *      Cr' - 128 = gamma (Cr - 128);
 *      Cb' - 128 = gamma (Cb - 128);
 *  d) contrast:
 *      Y' - 16 = lambda + kappa (Y - 16);
 *  e) all together:
 *      Y' - 16   = gamma (lambda + kappa (Y - 16));
 *      Cr' - 128 = gamma beta ((Cr - 128) cos(alpha) - zeta (Cb - 128) sin(alpha));
 *      Cb' - 128 = gamma beta (xi (Cr - 128) sin(alpha) + (Cb - 128) cos(alpha));
 */
#define XI      (((1.-CCIR601_YRCOEF)/0.5/1.14)/((1.-CCIR601_YBCOEF)/0.5/2.03))
#define ZETA    (1./XI)

/* empirically choosen limits for color adjustments: */
#define ALPHA_MAX   (M_PI*3/4)
#define ALPHA_MED   0.              /* hue */
#define ALPHA_MIN   (-M_PI*3/4)

#define BETA_MAX    M_SQRT2
#define BETA_MED    1.              /* saturation */
#define BETA_MIN    (M_SQRT2/4)

#define GAMMA_MAX   M_SQRT2
#define GAMMA_MED   1.              /* brightness */
#define GAMMA_MIN   0.5

#define KAPPA_MIN   0.5
#define KAPPA_MED   1.              /* contrast */
#define KAPPA_MAX   2.

#define LAMBDA(kappa)   (CCIR601_YMED * (1. - kappa))

/* indicators of current color adjustment coefficients: */
YUV_EXTERN int is_alpha, is_beta, is_gamma, is_kappa;

/* Y'Cr'Cb'->Y'Cr'Cb' conversion tables: */
YUV_EXTERN int _yytab [RGB_MAX+1], _uutab [RGB_MAX+1], _vvtab [RGB_MAX+1];
YUV_EXTERN int _uvtab [RGB_MAX+1], _vutab [RGB_MAX+1]; /* alpha != 0: */

/* chroma resampling mode: */
YUV_EXTERN int chroma_resampling_mode;

#ifdef __cplusplus
}
#endif

#endif /* __YUV_H__ */
