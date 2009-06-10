/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: yuv.h,v 1.6 2007/07/06 20:53:53 jfinnecy Exp $
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
 * yuv.h: constants, prototypes, and macros used by the color
 * conversion routines.  This header should not be included in any
 * files that are not part of the color conversion library.
 */
#ifndef YUV_H_
#define YUV_H_

/* 
 * This constant defines the maximum width of a video frame that will
 * be supported by the color convert routines.  Setting this value to
 * 640 will allow all RealVideo encodes to work..  If you explicitly
 * disallow (and test for) frame widths larger than this value
 * elsewhere in the player, then this value can be set to a smaller
 * value.  The allocated memory is 12 * MAXWIDTH bytes.
 */

/* This was increased to 720 to handle half D1 video. */
#define MAXWIDTH 720


/*******************************************************************
 * Don't change values below this line.
 *******************************************************************/

/* some math. constants: */
#ifndef M_PI
#define M_PI      3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2   1.41421356237309504880
#endif

/* fast rounding to a nearest integer: */
#if _M_IX86
#define INT(a) DoubleToInt(a)
static __inline int DoubleToInt(double a)
{
    int ia;
    __asm   fld     a
    __asm   fistp   ia
    return ia;
}
#else
#define INT(a) ((int) (((a)<0)? ((a)-0.5): ((a)+0.5)))
#endif

extern   int ytab  [], rvtab [], gvtab [];
extern   int gutab [], butab [];
extern   int rutab [], bvtab []; /* alpha != 0 */

/*
 * Bytes-per-pixel, for readability:
 */
#define BPP1    1
#define BPP2    2
#define BPP3    3
#define BPP4    4

/*
 * Clipping & dithering:
 *
 * Note: the CLIPNEG/CLIPPOS numbers are somewhat rounded values
 *  that correspond to current GAMMA_MAX,KAPPA_MAX, and BETA_MAX
 *  parameters. If you are planning to modify any of these, the
 *  ranges of the clip tables shall be adjusted as well.
 */
#define CLIPRNG 256                 /* unclipped range */
#define CLIPNEG (288 * 4)           /* rounded max neg. value */
#define CLIPPOS (288 * 4)           /* rounded max shot over 256 */


#if defined(HELIX_FEATURE_CC_RGB444out)
//These 2 handle all the RGB444 conversions
#define CLIP4 (pcd->clip4+CLIPNEG)       /* offset into clip4 */
#define CLIP8 (pcd->clip8+CLIPNEG)       /* offset into clip8 */
#endif

#if defined(HELIX_FEATURE_CC_RGB565out)            
//These 2 handle all the RGB565 conversions
#define CLIP5 (pcd->clip5+CLIPNEG)       /* offset into clip5 */
#define CLIP6 (pcd->clip6+CLIPNEG)       /* offset into clip6 */
#endif
            
/* 2-level, 2x2 dither noise, added before rounding to 333/555/565.
 * Define NO_DITHER to disable. */
#ifndef NO_DITHER
#define DITH3L  8   /* 16 - 8 */
#define DITH3H  24  /* 16 + 8 */
#define DITH4L  4   /* 8 - 4 */
#define DITH4H  12  /* 8 + 4 */
#define DITH5L  2   /* 4 - 2 */
#define DITH5H  6   /* 4 + 2 */
#define DITH6L  1   /* 2 - 1 */
#define DITH6H  3   /* 2 + 1 */
#define DITH8L  0
#define DITH8H  0
#else /* disabled */
#define DITH3L  16  /* does (i+16)>>5 for proper rounding */
#define DITH3H  16
#define DITH4L  8   /* does (i+8)>>4 for proper rounding */
#define DITH4H  8
#define DITH5L  4   /* does (i+4)>>3 for proper rounding */
#define DITH5H  4
#define DITH6L  2   /* does (i+2)>>2 for proper rounding */
#define DITH6H  2
#define DITH8L  0
#define DITH8H  0
#endif

extern int chk_args (unsigned char *, int, int,
		     int, int, int, int, int,
		     unsigned char *, int, int,
		     int, int, int, int, int,
		     int *, int *);

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
 *
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
#define RGB_MAX 255

/*
 * Forward R'G'B'->Y'Cr'Cb' conversion:
 *  a) calculate Y":
 *      Y" = 0.299 R' + 0.587 G' + 0.114 B';            => 3 muls/lookups
 *  b) calculate Y',Cr',Cb' values:
 *      Y'  = 219/255 Y" + 16;                          => 1 mul/lookup
 *      Cr' = 224/255 * 0.5/0.701 (R'-Y") + 128;        => 1 mul/lookup
 *      Cb' = 224/255 * 0.5/0.886 (B'-Y") + 128;        => 1 mul/lookup
 */
#define YSCALE  ((double)CCIR601_YMAX/RGB_MAX)
#define VSCALE  ((double)CCIR601_VMAX/RGB_MAX * 0.5/(1.-CCIR601_YRCOEF))
#define USCALE  ((double)CCIR601_UMAX/RGB_MAX * 0.5/(1.-CCIR601_YBCOEF))

#define YMAX    (RGB_MAX + 3)       /* include max. rounding error */
#define VMAX    (int)((double)(1.-CCIR601_YRCOEF) * RGB_MAX + 1)
#define VMIN    VMAX
#define UMAX    (int)((double)(1.-CCIR601_YBCOEF) * RGB_MAX + 1)
#define UMIN    UMAX

/*
 * Backward Y'Cr'Cb'->R'G'B' conversion:
 *  a) calculate Y":
 *      Y"  = 1/(219/255) (Y'-16);                                              => 1 mul/lookup
 *  b) calculate R'B':
 *      R'  = Y" + 1/(224/255 * 0.5/0.701) (Cr'-128);   => 1 mul/lookup
 *      B'  = Y" + 1/(224/255 * 0.5/0.886) (Cb'-128);   => 1 mul/lookup
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


/*
 * color_data_t: This struct contains all variables that need to be
 * persistant across calls the the color conversion routines. These
 * were formerly stored in global values. These needed to be removed
 * due to Symbian dll restrictions.
 */
typedef struct color_data_t_ {
    int color_conversion_tables_inited;
  
    int is_alpha;
    int is_beta;
    int is_gamma;
    int is_kappa;

    float cur_brightness;
    float cur_contrast;
    float cur_saturation;
    float cur_hue; 

    unsigned char linebuf[3 * MAXWIDTH * BPP4];

    /* Y'Cr'Cb'->R'G'B' conversion tables: */
    int ytab  [RGB_MAX+1];
    int rvtab [RGB_MAX+1];
    int gvtab [RGB_MAX+1];
    int gutab [RGB_MAX+1];
    int butab [RGB_MAX+1];
    int rutab [RGB_MAX+1];
    int bvtab [RGB_MAX+1]; /* alpha != 0 */

    /* actual clip tables */
#if defined(HELIX_FEATURE_CC_RGB444out)
    unsigned char clip4 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip/round to 4 bits */
    unsigned char clip8 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip to 8 bits */
#endif    
#if defined(HELIX_FEATURE_CC_RGB565out)
    unsigned char clip5 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip/round to 5 bits */
    unsigned char clip6 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip/round to 6 bits */
#endif
    
} color_data_t;

void InitColorData(color_data_t* pcd);

#endif /* YUV_H_ */
