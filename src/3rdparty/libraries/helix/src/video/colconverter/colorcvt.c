/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: colorcvt.c,v 1.5 2007/07/06 20:53:51 jfinnecy Exp $
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

/* standard libraries to use: */
#include <string.h>
#include <math.h>
#include "hxtypes.h"

/* some math. constants: */
#ifndef M_PI
#define M_PI      3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2   1.41421356237309504880
#endif

/* fast rounding to a nearest integer: */
#if _M_IX86
static __inline int INT(double a)
{
    int ia;
    __asm   fld     a
    __asm   fistp   ia
    return ia;
}
#elif defined(__i386) && defined(_LINUX) 
static inline int INT(double a)
{
    int ia;
    __asm__ (
        " fldl %1; "
        " fistpl %0; "
        : "=m"(ia)
        : "m"(a)
        : "memory", "st"
        );
       
    return ia;
}

#else
#define INT(a) ((int) (((a)<0)? ((a)-0.5): ((a)+0.5)))
#endif

/* macros to check big/little endiannes of the system: */
#ifdef _MACINTOSH
const static union {char c[4]; UINT32 l;} _1234 = {'\001\002\003\004'}; /* Flawfinder: ignore */
#else
const static union {char c[4]; UINT32 l;} _1234 = {"\001\002\003\004"}; /* Flawfinder: ignore */
#endif
#define BIG_ENDIAN      (_1234.l == 0x01020304)
#define LITTLE_ENDIAN   (_1234.l == 0x04030201)

/* get our interface: */
#include "colorcvt.h"

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
static int yrtab [RGB_MAX+1], ygtab [RGB_MAX+1], ybtab [RGB_MAX+1];
static int yytab [YMAX+1], vrytab [VMIN+VMAX+1], ubytab [UMIN+UMAX+1];

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
static int ytab  [RGB_MAX+1], rvtab [RGB_MAX+1], gvtab [RGB_MAX+1];
static int gutab [RGB_MAX+1], butab [RGB_MAX+1];
static int rutab [RGB_MAX+1], bvtab [RGB_MAX+1]; /* alpha != 0 */

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

/* current color adjustment parameters: */
static float cur_brightness, cur_contrast, cur_saturation, cur_hue;
static int is_alpha, is_beta, is_gamma, is_kappa;
static int color_conversion_tables_inited = 0;

/* Y'Cr'Cb'->Y'Cr'Cb' conversion tables: */
static int _yytab [RGB_MAX+1], _uutab [RGB_MAX+1], _vvtab [RGB_MAX+1];
static int _uvtab [RGB_MAX+1], _vutab [RGB_MAX+1]; /* alpha != 0: */

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

#define CLIP4 (clip4+CLIPNEG)       /* offset into clip4 */
#define CLIP5 (clip5+CLIPNEG)       /* offset into clip5 */
#define CLIP6 (clip6+CLIPNEG)       /* offset into clip6 */
#define CLIP8 (clip8+CLIPNEG)       /* offset into clip8 */

/* actual clip tables */
static unsigned char clip4 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip/round to 4 bits */ /* Flawfinder: ignore */
static unsigned char clip5 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip/round to 5 bits */ /* Flawfinder: ignore */
static unsigned char clip6 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip/round to 6 bits */ /* Flawfinder: ignore */
static unsigned char clip8 [CLIPNEG+CLIPRNG+CLIPPOS]; /* clip to 8 bits */ /* Flawfinder: ignore */

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

/* speeds-up calculation of distance between pixels: */
#define SQ  (sq+RGB_MAX+1)
static unsigned int sq [RGB_MAX+1+RGB_MAX];

/* palette color maps: */
extern unsigned char pmap [1U << (4+4+4)];  /* 4-bits per channel */ /* Flawfinder: ignore */

/* default palette to use: */
static unsigned int default_palette [RGB_MAX+1] = {
    0x000000, 0x000080, 0x008000, 0x008080, 0x800000, 0x800080, 0x808000, 0xC0C0C0,
    0xC0DCC0, 0xF0CAA6, 0x040404, 0x080808, 0x0C0C0C, 0x111111, 0x161616, 0x1C1C1C,
    0x222222, 0x292929, 0x555555, 0x4D4D4D, 0x424242, 0x393939, 0x818181, 0x000081,
    0x008100, 0x008181, 0x810000, 0x810081, 0x818100, 0x000033, 0x000066, 0x000099,
    0x0000CC, 0x003300, 0x003333, 0x003366, 0x003399, 0x0033CC, 0x0033FF, 0x006600,
    0x006633, 0x006666, 0x006699, 0x0066CC, 0x0066FF, 0x009900, 0x009933, 0x009966,
    0x009999, 0x0099CC, 0x0099FF, 0x00CC00, 0x00CC33, 0x00CC66, 0x00CC99, 0x00CCCC,
    0x00CCFF, 0x00FF66, 0x00FF99, 0x00FFCC, 0x330000, 0x330033, 0x330066, 0x330099,
    0x3300CC, 0x3300FF, 0x333300, 0x333333, 0x333366, 0x333399, 0x3333CC, 0x3333FF,
    0x336600, 0x336633, 0x336666, 0x336699, 0x3366CC, 0x3366FF, 0x339900, 0x339933,
    0x339966, 0x339999, 0x3399CC, 0x3399FF, 0x33CC00, 0x33CC33, 0x33CC66, 0x33CC99,
    0x33CCCC, 0x33CCFF, 0x33FF33, 0x33FF66, 0x33FF99, 0x33FFCC, 0x33FFFF, 0x660000,
    0x660033, 0x660066, 0x660099, 0x6600CC, 0x6600FF, 0x663300, 0x663333, 0x663366,
    0x663399, 0x6633CC, 0x6633FF, 0x666600, 0x666633, 0x666666, 0x666699, 0x6666CC,
    0x669900, 0x669933, 0x669966, 0x669999, 0x6699CC, 0x6699FF, 0x66CC00, 0x66CC33,
    0x66CC99, 0x66CCCC, 0x66CCFF, 0x66FF00, 0x66FF33, 0x66FF99, 0x66FFCC, 0xCC00FF,
    0xFF00CC, 0x999900, 0x993399, 0x990099, 0x9900CC, 0x990000, 0x993333, 0x990066,
    0x9933CC, 0x9900FF, 0x996600, 0x996633, 0x993366, 0x996699, 0x9966CC, 0x9933FF,
    0x999933, 0x999966, 0x999999, 0x9999CC, 0x9999FF, 0x99CC00, 0x99CC33, 0x66CC66,
    0x99CC99, 0x99CCCC, 0x99CCFF, 0x99FF00, 0x99FF33, 0x99CC66, 0x99FF99, 0x99FFCC,
    0x99FFFF, 0xCC0000, 0x990033, 0xCC0066, 0xCC0099, 0xCC00CC, 0x993300, 0xCC3333,
    0xCC3366, 0xCC3399, 0xCC33CC, 0xCC33FF, 0xCC6600, 0xCC6633, 0x996666, 0xCC6699,
    0xCC66CC, 0x9966FF, 0xCC9900, 0xCC9933, 0xCC9966, 0xCC9999, 0xCC99CC, 0xCC99FF,
    0xCCCC00, 0xCCCC33, 0xCCCC66, 0xCCCC99, 0xCCCCCC, 0xCCCCFF, 0xCCFF00, 0xCCFF33,
    0x99FF66, 0xCCFF99, 0xCCFFCC, 0xCCFFFF, 0xCC0033, 0xFF0066, 0xFF0099, 0xCC3300,
    0xFF3333, 0xFF3366, 0xFF3399, 0xFF33CC, 0xFF33FF, 0xFF6600, 0xFF6633, 0xCC6666,
    0xFF6699, 0xFF66CC, 0xCC66FF, 0xFF9900, 0xFF9933, 0xFF9966, 0xFF9999, 0xFF99CC,
    0xFF99FF, 0xFFCC00, 0xFFCC33, 0xFFCC66, 0xFFCC99, 0xFFCCCC, 0xFFCCFF, 0xFFFF33,
    0xCCFF66, 0xFFFF99, 0xFFFFCC, 0x6666FF, 0x66FF66, 0x66FFFF, 0xFF6666, 0xFF66FF,
    0xFFFF66, 0xC1C1C1, 0x5F5F5F, 0x777777, 0x868686, 0x969696, 0xCBCBCB, 0xB2B2B2,
    0xD7D7D7, 0xDDDDDD, 0xE3E3E3, 0xEAEAEA, 0xF1F1F1, 0xF8F8F8, 0xF0FBFF, 0xA4A0A0,
    0x808080, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
};
int default_palette_idx [RGB_MAX+1] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
    16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
    32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
    80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
    96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

/*
 * Bytes-per-pixel, for readability:
 */
#define BPP1    1
#define BPP2    2
#define BPP3    3
#define BPP4    4

#if defined (_MACINTOSH) && defined (_DEBUG)
#pragma global_optimizer on
#endif

/*
 * Convert a value in the range of [-1,1],
 * to a new range [l,h]; with a middle value equal to m.
 */
static double chrange (double x, double l, double m, double h)
{
    if (x >= 0.) {
        if (x > 1.) x = 1.;
        x = m + (h - m) * x;
    } else {
        if (x < -1.) x = -1.;
        x = m + (m - l) * x;
    }
    return x;
}

/*
 * Checks if variable is signicantly different from zero.
 */
static int is (double val)
{
    return fabs(val) > 0.01;    /* < 1% is not considered a change */
}


/*
 * Returns current state of "from/to I420" converters.
 * Use:
 *  void GetSrcI420Colors (float *brightness, float *contrast, float *saturation, float *hue);
 * Output:
 *  brightness - brightness adjustment [-1,1], 0 - default;
 *  contrast   - contrast adjustment   [-1,1], 0 - default;
 *  saturation - saturation adjustment [-1,1], 0 - default;
 *  hue        - hue adjustment        [-1,1], 0 - default;
 * Returns:
 *  none.
 */
void GetSrcI420Colors (float *brightness, float *contrast, float *saturation, float *hue)
{
    if (brightness && contrast && saturation && hue)
    {
	*brightness = cur_brightness;
	*contrast   = cur_contrast;
	*saturation = cur_saturation;
	*hue	    = cur_hue;
    }
}

/*
 * Initializes "from/to I420" converters.
 * Use:
 *  void SetSrcI420Colors (float brightness, float contrast, float saturation, float hue);
 *  void SetDestI420Colors (float brightness, float contrast, float saturation, float hue);
 * Input:
 *  brightness - brightness adjustment [-1,1], 0 - default;
 *  contrast   - contrast adjustment   [-1,1], 0 - default;
 *  saturation - saturation adjustment [-1,1], 0 - default;
 *  hue        - hue adjustment        [-1,1], 0 - default;
 * Returns:
 *  none.
 */
void SetSrcI420Colors (float brightness, float contrast, float saturation, float hue)
{
    /* check if we need to generate new color conversion tables: */
    if (!color_conversion_tables_inited  ||
        is (cur_brightness - brightness) ||
        is (cur_contrast - contrast)     ||
        is (cur_saturation - saturation) ||
        is (cur_hue - hue)) {

        double alpha, beta, gamma, kappa, lambda;
        double cos_alpha, xi_sin_alpha, minus_zeta_sin_alpha;
        register int i;

        /* hue: */
        alpha = chrange (hue, ALPHA_MIN, ALPHA_MED, ALPHA_MAX);
        cos_alpha = cos (alpha);
        xi_sin_alpha = XI * sin (alpha);
        minus_zeta_sin_alpha = -ZETA * sin (alpha);
        is_alpha = is (cur_hue = hue);

        /* saturation: */
        beta = chrange (saturation, BETA_MIN, BETA_MED, BETA_MAX);
        is_beta = is (cur_saturation = saturation);

        /* brightness: */
        gamma = chrange (brightness, GAMMA_MIN, GAMMA_MED, GAMMA_MAX);
        is_gamma = is (cur_brightness = brightness);

        /* contrast: */
        kappa = chrange (contrast, KAPPA_MIN, KAPPA_MED, KAPPA_MAX);
        lambda = LAMBDA (kappa);
        is_kappa = is (cur_contrast = contrast);

        /*
         * Check if we need to generate clip tables & default palette:
         */
        if (!color_conversion_tables_inited) {

            /* Init clip tables. CLIP4/5/6 includes chop to 3/5/6 bits */
            for (i = -CLIPNEG; i < CLIPRNG + CLIPPOS; i++) {

                if (i < 0) {
                    CLIP4[i] = 0;
                    CLIP5[i] = 0;
                    CLIP6[i] = 0;           /* clip */
                    CLIP8[i] = 0;
                } else if (i > 255) {

                    CLIP4[i] = 255 >> 4;
                    CLIP5[i] = 255 >> 3;
                    CLIP6[i] = 255 >> 2;    /* clip */
                    CLIP8[i] = 255;
                } else {
                    CLIP4[i] = i >> 4;      /* chop to 4 bits */
                    CLIP5[i] = i >> 3;      /* chop to 5 bits */
                    CLIP6[i] = i >> 2;      /* chop to 6 bits */
                    CLIP8[i] = i;           /* leave at 8 bits */
                }
            }

            /* square values: */
            for (i = -RGB_MAX; i <= RGB_MAX; i ++)
                SQ[i] = i * i;

            /* set default palette: */
            SetDestRGB8Palette (256, default_palette, default_palette_idx);

            /* set indicator: */
            color_conversion_tables_inited ++;
        }

        /*
         * Initialize color conversion tables.
         */
        for (i = 0; i < 256; i++) {

            /* Y'Cr'Cb'->R'G'B' conversion tables: */
            double y = lambda + (i - CCIR601_YOFFSET) * kappa; /* adjust contrast */
            if (y < 0) y = 0; else if (y > CCIR601_YMAX) y = CCIR601_YMAX;
            ytab  [i] = INT(y * YCOEF * gamma);
            rvtab [i] = INT((i-CCIR601_VOFFSET) * cos_alpha * RVCOEF * beta * gamma);
            gvtab [i] = INT((i-CCIR601_VOFFSET) * GVCOEF * beta * gamma);
            bvtab [i] = INT((i-CCIR601_VOFFSET) * xi_sin_alpha * BUCOEF * beta * gamma);
            rutab [i] = INT((i-CCIR601_UOFFSET) * minus_zeta_sin_alpha * RVCOEF * beta * gamma);
            gutab [i] = INT((i-CCIR601_UOFFSET) * GUCOEF * beta * gamma);
            butab [i] = INT((i-CCIR601_UOFFSET) * cos_alpha * BUCOEF * beta * gamma);

            /* Y'Cr'Cb'->Y'Cr'Cb' conversion tables: */
            y = lambda + (i - CCIR601_YOFFSET) * kappa;
            _yytab [i] = CLIP8 [CCIR601_YOFFSET + INT(y * gamma)];
            _vvtab [i] = CCIR601_VOFFSET + INT((i-CCIR601_VOFFSET) * cos_alpha * beta * gamma);
            _uutab [i] = CCIR601_UOFFSET + INT((i-CCIR601_UOFFSET) * cos_alpha * beta * gamma);
            _vutab [i] = INT((i-CCIR601_UOFFSET) * minus_zeta_sin_alpha * beta * gamma);
            _uvtab [i] = INT((i-CCIR601_VOFFSET) * xi_sin_alpha * beta * gamma);
            if (!is_alpha) {
                _vvtab [i] = CLIP8 [_vvtab [i]];
                _uutab [i] = CLIP8 [_uutab [i]];
            }
        }
    }
}

void SetDestI420Colors (float brightness, float contrast, float saturation, float hue)
{
    /* Currently, all color adjustment parameters are ignored,
     * and tables will always be initialized for CCIR 601-2 - compliant
     * color conversion. */

    register int i, j;

    /* initialize y*tab[] tables: */
    for (i = 0; i <= RGB_MAX; i++) {
        yrtab [i] = INT((double)i * CCIR601_YRCOEF);
        ygtab [i] = INT((double)i * CCIR601_YGCOEF);
        ybtab [i] = INT((double)i * CCIR601_YBCOEF);
    }

    /* initialize yytab[]: */
    for (i = 0; i <= YMAX; i++) {
        j = INT((double)i * YSCALE);
        if (j > CCIR601_YMAX) j = CCIR601_YMAX;
        yytab [i] = j + CCIR601_YOFFSET;
    }

    /* initialize vrytab[]: */
    for (i = -VMIN; i <= VMAX; i++) {
        j = INT((double)i * VSCALE);
        if (j < -CCIR601_VMAX/2) j = -CCIR601_VMAX/2;
        if (j >  CCIR601_VMAX/2) j =  CCIR601_VMAX/2;
        vrytab [VMIN+i] = j + CCIR601_VOFFSET;
    }

    /* initialize ubytab[]: */
    for (i = -UMIN; i <= UMAX; i++) {
        j = INT((double)i * USCALE);
        if (j < -CCIR601_UMAX/2) j = -CCIR601_UMAX/2;
        if (j >  CCIR601_UMAX/2) j =  CCIR601_UMAX/2;
        ubytab [UMIN+i] = j + CCIR601_UOFFSET;
    }
}

/*
 * Suggest palettes to use for 8-bit RGB formats.
 * Use:
 *  int SuggestSrcRGB8Palette (int nColors, unsigned int *lpRGBVals);
 *  int SuggestDestRGB8Palette (int nColors, unsigned int *lpRGBVals);
 * Input:
 *  nColors - the number of existing colors in palette
 *  lpRGBVals - RGB values for each palette entry
 *  lpIndices - pixel values (palette indices)
 * Returns:
 *  >0, the total number of colors to be used, if success
 *  -1, if error
 */
int SuggestSrcRGB8Palette (int nColors, unsigned int *lpRGBVals)
{
    return -1;    /* not implemented yet... */
}

int SuggestDestRGB8Palette (int nColors, unsigned int *lpRGBVals)
{
    unsigned int rgb;
    int r, g, b, i, j, delta;

    /* determine the quanization step size: */
    if      (256 - nColors >= 6*6*6)    delta = 0x33;   /* 6x6x6 = 216 */
    else if (256 - nColors >= 5*5*5)    delta = 0x40;   /* 5x5x5 = 125 */
    else if (256 - nColors >= 4*4*4)    delta = 0x55;   /* 4x4x4 = 64  */
    else return nColors;

    /* complement the set of palette entries with ours: */
    i = nColors;
    for (r = 0; r <= 256; r += delta) { if (r == 256) r = 255;
    for (g = 0; g <= 256; g += delta) { if (g == 256) g = 255;
    for (b = 0; b <= 256; b += delta) { if (b == 256) b = 255;

        /* form palette entry: */
        rgb = b | (g << 8) | (r << 16);

        /* check if we had this color before: */
        for (j = 0; j < nColors; j ++)  if (lpRGBVals[j] == rgb) goto skip;

        /* add color: */
        lpRGBVals[i] = rgb;
        i ++;

skip:   ;
    }}}

    /* return total # of entries used: */
    return i;
}

/*
 * Set palettes to use for 8-bit RGB formats.
 * Use:
 *  int SetSrcRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices);
 *  int SetDestRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices);
 * Input:
 *  nColors - the number of colors in palette
 *  lpRGBVals - RGB values for each palette entry
 *  lpIndices - pixel values (palette indices)
 * Returns:
 *  >0, the total number of colors set, if success
 *  -1, if error
 */
int SetSrcRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices)
{
    return -1;    /* not implemented yet... */
}

int SetDestRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices)
{
    register unsigned int r, g, b, delta = 1U << 4;

    /* perform the minimum energy mismatch mapping: */
    for (r = 0; r < 256; r += delta) {
    for (g = 0; g < 256; g += delta) {
    for (b = 0; b < 256; b += delta) {

        /* initialize search: */
        int idx, match_idx = 0;
        unsigned int j, diff, match_diff;
        j = lpRGBVals[0];
        match_diff =
            SQ[(int)(r - ((j >> 0)  & 0xFF))] +
            SQ[(int)(g - ((j >> 8)  & 0xFF))] +
            SQ[(int)(b - ((j >> 16) & 0xFF))];

        /* find minimum: */
        for (idx = 1; idx < nColors; idx ++) {
            j = lpRGBVals[idx];
            diff =
                SQ[(int)(r - ((j >> 0)  & 0xFF))] +
                SQ[(int)(g - ((j >> 8)  & 0xFF))] +
                SQ[(int)(b - ((j >> 16) & 0xFF))];

            if (diff < match_diff) {
                match_idx = idx;
                match_diff = diff;
            }
        }

        /* store the result: */
        pmap[(b >> 4) | g | (r << 4)] = lpIndices[match_idx];
    }}}

    return nColors;
}

/*
 * Checks format conversion parameters.
 * Use:
 *  int chk_args (unsigned char *dest_ptr, int dest_width, int dest_height,
 *      int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
 *      unsigned char *src_ptr, int src_width, int src_height,
 *      int src_pitch, int src_x, int src_y, int src_dx, int src_dy,
 *      int *p_scale_x, int *p_scale_y);
 * Input:
 *  dest_ptr - pointer to a destination buffer
 *  dest_width, dest_height - width/height of the destination image (pixels)
 *  dest_pitch - pitch of the dest. buffer (in bytes; <0 - if bottom up image)
 *  dest_x, dest_y, dest_dx, dest_dy - destination rectangle (pixels)
 *  src_ptr - pointer to an input image
 *  src_width, src_height - width/height of the input image (pixels)
 *  src_pitch - pitch of the source buffer (in bytes; <0 - if bottom up image)
 *  src_x, src_y, src_dx, src_dy - source rectangle (pixels)
 * Output:
 *  p_scale_x, p_scale_y - scale factors for x,y axes
 *      (currently only 1:1, and 2:1 scale factors are allowed)
 * Returns:
 *  0 - if success; -1 if failure.
 */
static int
chk_args (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height,
    int src_pitch, int src_x, int src_y, int src_dx, int src_dy,
    int *p_scale_x, int *p_scale_y)
{
    /* alignments: */
    if (((unsigned)dest_ptr & 3) || (dest_pitch & 3) ||
        ((unsigned)src_ptr  & 3) || (src_pitch  & 3) ||
    /* image sizes: */
        dest_width <= 0 || dest_height <= 0 ||
        src_width  <= 0 || src_height  <= 0 ||
    /* rectangles: */
        dest_x < 0 || dest_y < 0 || dest_dx <= 0 || dest_dy <= 0 ||
        src_x  < 0 || src_y  < 0 || src_dx  <= 0 || src_dy  <= 0 ||
    /* overlaps: */
        dest_width < dest_x + dest_dx || dest_height < dest_y + dest_dy ||
        src_width  < src_x  + src_dx  || src_height  < src_y  + src_dy)
        goto fail;

    /* scale factors: */
    if (dest_dx == src_dx)          *p_scale_x = 1;
    else if (dest_dx == 2 * src_dx) *p_scale_x = 2;
    else goto fail;
    if (dest_dy == src_dy)          *p_scale_y = 1;
    else if (dest_dy == 2 * src_dy) *p_scale_y = 2;
    else goto fail;

    /* success: */
    return 1;

    /* failure: */
fail:
    return 0;
}

static int adjust_range (int *z1, int *dz1, int *z2, int *dz2, int inc2)
{
    /* skip odd start pixel: */
    if (*z1 & 1) {
        *z1 += 1;
        *dz1 -= 1;
        *z2 += inc2;
        *dz2 -= inc2;
    }
    /* clip the range: */
    if (*dz1 & 1) {
        *dz1 -= 1;
        *dz2 -= inc2;
    }
    return (*dz1 > 0 && *dz2 > 0);
}

/*
 * Format-conversion routines.
 * Use:
 *  int XXXXtoYYYYEx (unsigned char *dest_ptr, int dest_width, int dest_height,
 *      int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
 *      unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
 *      int src_x, int src_y, int src_dx, int src_dy);
 * Input:
 *  dest_ptr - pointer to a destination buffer
 *  dest_width, dest_height - width/height of the destination image (pixels)
 *  dest_pitch - pitch of the dest. buffer (in bytes; <0 - if bottom up image)
 *  dest_x, dest_y, dest_dx, dest_dy - destination rectangle (pixels)
 *  src_ptr - pointer to an input image
 *  src_width, src_height - width/height of the input image (pixels)
 *  src_pitch - pitch of the source buffer (in bytes; <0 - if bottom up image)
 *  src_x, src_y, src_dx, src_dy - source rectangle (pixels)
 * Returns:
 *  0 - if success; -1 if failure.
 * Notes:
 *  a) In all cases, pointers to the source and destination buffers must be
 *     DWORD aligned, and both pitch parameters must be multiple of 4!!!
 *  b) Converters that deal with YUV 4:2:2, or 4:2:0 formats may also require
 *     rectangle parameters (x,y,dx,dy) to be multiple of 2. Failure to provide
 *     aligned rectangles will result in partially converted image.
 *  c) Currently only scale factors of 1:1 and 2:1 are supported; if the rates
 *     dest_dx/src_dx & dest_dy/src_dy are neither 1, or 2, the converters
 *     will fail.
 */


/*
 * I420toI420() converter:
 */
int I420toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have matching chroma components: */
    if ((src_x & 1) || (src_y & 1))
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0 || src_pitch <= 0)
        return -1;                          /* not supported for this format */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustmenst: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *s, *d;
            int src_uv_offs, dest_uv_offs;
            register int i;

            /* copy Y plane: */
            s = src_ptr + src_x + src_y * src_pitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {
                memcpy (d, s, dest_dx); /* Flawfinder: ignore */
                s += src_pitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            src_uv_offs = src_height * src_pitch / 4;
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* copy Cr/Cb planes: */
            s = (src_ptr + src_height * src_pitch) + src_x/2 + src_y/2 * src_pitch/2;
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;
            for (i = 0; i < dest_dy/2; i ++) {
                memcpy (d, s, dest_dx/2); /* Flawfinder: ignore */
                memcpy (d + dest_uv_offs, s + src_uv_offs, dest_dx/2); /* Flawfinder: ignore */
                s += src_pitch/2;
                d += dest_pitch/2;
            }

        } else {

            /* adjust colors: */
            unsigned char *s, *d;
            int src_uv_offs, dest_uv_offs;
            register int i, j;

            /* convert Y plane: */
            s = src_ptr + src_x + src_y * src_pitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {

                /* convert pixels: */
                for (j = 0; j < dest_dx; j ++)
                    d[j] = _yytab[s[j]];

                s += src_pitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            src_uv_offs = src_height * src_pitch / 4;
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* get chroma pointers: */
            s = (src_ptr + src_height * src_pitch) + src_x/2 + src_y/2 * src_pitch/2;
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;

            /* check if no hue adjustment: */
            if (!is_alpha) {

                /* no chroma rotation: */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
                        d[j] = _vvtab[s[j]];
                        d[j + dest_uv_offs] = _uutab[s[j + src_uv_offs]];
                    }

                    s += src_pitch/2;
                    d += dest_pitch/2;
                }

            } else {

                /* adjust hue: */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
                        register unsigned v = s[j], u = s[j + src_uv_offs];
                        d[j] = CLIP8[_vvtab[v] + _vutab[u]];
                        d[j + dest_uv_offs] = CLIP8[_uutab[u] + _uvtab[v]];
                    }

                    s += src_pitch/2;
                    d += dest_pitch/2;
                }
            }
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * I420toYV12() converter:
 */
int I420toYV12 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have matching chroma components: */
    if ((src_x & 1) || (src_y & 1))
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0 || src_pitch <= 0)
        return -1;                          /* not supported for this format */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustments: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *s, *d;
            int src_uv_offs, dest_uv_offs;
            register int i;

            /* copy Y plane: */
            s = src_ptr + src_x + src_y * src_pitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {
                memcpy (d, s, dest_dx); /* Flawfinder: ignore */
                s += src_pitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            src_uv_offs = src_height * src_pitch / 4;
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* copy Cr/Cb planes: */
            s = (src_ptr + src_height * src_pitch) + src_x/2 + src_y/2 * src_pitch/2;
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;
            for (i = 0; i < dest_dy/2; i ++) {
                memcpy (d, s + src_uv_offs, dest_dx/2); /* Flawfinder: ignore */
                memcpy (d + dest_uv_offs, s, dest_dx/2); /* Flawfinder: ignore */
                s += src_pitch/2;
                d += dest_pitch/2;
            }

        } else {

            /* adjust colors: */
            unsigned char *s, *d;
            int src_uv_offs, dest_uv_offs;
            register int i, j;

            /* convert Y plane: */
            s = src_ptr + src_x + src_y * src_pitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {

                /* convert pixels: */
                for (j = 0; j < dest_dx; j ++)
                    d[j] = _yytab[s[j]];

                s += src_pitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            src_uv_offs = src_height * src_pitch / 4;
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* get chroma pointers: */
            s = (src_ptr + src_height * src_pitch) + src_x/2 + src_y/2 * src_pitch/2;
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;

            /* check if no hue adjustment: */
            if (!is_alpha) {

                /* no chroma rotation: */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
                        d[j] = _vvtab[s[j + src_uv_offs]];
                        d[j + dest_uv_offs] = _uutab[s[j]];
                    }

                    s += src_pitch/2;
                    d += dest_pitch/2;
                }

            } else {

                /* adjust hue: */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
                        register unsigned v = s[j + src_uv_offs], u = s[j];
                        d[j] = CLIP8[_vvtab[v] + _vutab[u]];
                        d[j + dest_uv_offs] = CLIP8[_uutab[u] + _uvtab[v]];
                    }

                    s += src_pitch/2;
                    d += dest_pitch/2;
                }
            }
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * I420toYUY2() converter:
 */
int I420toYUY2 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination columns: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x))
        return 0;

    /* check if we have misaligned input: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (src_pitch <= 0) return -1;          /* not supported */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustments: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = src_ptr + src_x + src_y * src_pitch;           /* luma offset */
            su = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
            sv = su + src_height * src_pitch / 4;
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = sy[j*2] | (su[j] << 8) | (sy[j*2+1] << 16) | (sv[j] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = sy[j*2] | (su[j] << 8) | (sy[j*2+1] << 16) | (sv[j] << 24);
                sy += src_pitch;
                d  += dest_pitch;
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = sy[j*2] | (su[j] << 8) | (sy[j*2+1] << 16) | (sv[j] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = sy[j*2] | (su[j] << 8) | (sy[j*2+1] << 16) | (sv[j] << 24);
            }

        } else

        /* check if no hue adjustment: */
        if (!is_alpha) {

            /* no chroma rotation: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = src_ptr + src_x + src_y * src_pitch;           /* luma offset */
            su = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
            sv = su + src_height * src_pitch / 4;
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _yytab[sy[j*2]] | (_uutab[su[j]] << 8) | (_yytab[sy[j*2+1]] << 16) | (_vvtab[sv[j]] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _yytab[sy[j*2]] | (_uutab[su[j]] << 8) | (_yytab[sy[j*2+1]] << 16) | (_vvtab[sv[j]] << 24);
                sy += src_pitch;
                d  += dest_pitch;
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _yytab[sy[j*2]] | (_uutab[su[j]] << 8) | (_yytab[sy[j*2+1]] << 16) | (_vvtab[sv[j]] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _yytab[sy[j*2]] | (_uutab[su[j]] << 8) | (_yytab[sy[j*2+1]] << 16) | (_vvtab[sv[j]] << 24);
            }

        } else {

            /* the most complex case (w. hue adjustment): */
            unsigned char *sy, *sv, *su, *d;
            register int i, j, u, v;

            /* get pointers: */
            sy = src_ptr + src_x + src_y * src_pitch;           /* luma offset */
            su = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch/2);
            sv = su + src_height * src_pitch / 4;
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = CLIP8[_vvtab[sv[j]] + _vutab[su[j]]];
                    u = CLIP8[_uutab[su[j]] + _uvtab[sv[j]]];
                    *(unsigned int *)(d + j*4) = _yytab[sy[j*2]] | (u << 8) | (_yytab[sy[j*2+1]] << 16) | (v << 24);
                }
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = CLIP8[_vvtab[sv[j]] + _vutab[su[j]]];
                    u = CLIP8[_uutab[su[j]] + _uvtab[sv[j]]];
                    *(unsigned int *)(d + j*4) = _yytab[sy[j*2]] | (u << 8) | (_yytab[sy[j*2+1]] << 16) | (v << 24);
                    *(unsigned int *)(d + j*4 + dest_pitch) = _yytab[sy[j*2 + src_pitch]] | (u << 8) | (_yytab[sy[j*2+1+src_pitch]] << 16) | (v << 24);
                }
                sy += src_pitch*2;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch*2;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = CLIP8[_vvtab[sv[j]] + _vutab[su[j]]];
                    u = CLIP8[_uutab[su[j]] + _uvtab[sv[j]]];
                    *(unsigned int *)(d + j*4) = _yytab[sy[j*2]] | (u << 8) | (_yytab[sy[j*2+1]] << 16) | (v << 24);
                }
            }
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * I420toUYVY() converter:
 */
int I420toUYVY (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination columns: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x))
        return 0;

    /* check if we have misaligned input: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (src_pitch <= 0) return -1;          /* not supported */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustments: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = src_ptr + src_x + src_y * src_pitch;           /* luma offset */
            su = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
            sv = su + src_height * src_pitch / 4;
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
                sy += src_pitch;
                d  += dest_pitch;
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
            }

        } else

        /* check if no hue adjustment: */
        if (!is_alpha) {

            /* no chroma rotation: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = src_ptr + src_x + src_y * src_pitch;           /* luma offset */
            su = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
            sv = su + src_height * src_pitch / 4;
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _uutab[su[j]] | (_yytab[sy[j*2]] << 8) | (_vvtab[sv[j]] << 16) | (_yytab[sy[j*2+1]] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _uutab[su[j]] | (_yytab[sy[j*2]] << 8) | (_vvtab[sv[j]] << 16) | (_yytab[sy[j*2+1]] << 24);
                sy += src_pitch;
                d  += dest_pitch;
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _uutab[su[j]] | (_yytab[sy[j*2]] << 8) | (_vvtab[sv[j]] << 16) | (_yytab[sy[j*2+1]] << 24);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _uutab[su[j]] | (_yytab[sy[j*2]] << 8) | (_vvtab[sv[j]] << 16) | (_yytab[sy[j*2+1]] << 24);
            }

        } else {

            /* the most complex case (w. hue adjustement): */
            unsigned char *sy, *sv, *su, *d;
            register int i, j, u, v;

            /* get pointers: */
            sy = src_ptr + src_x + src_y * src_pitch;           /* luma offset */
            su = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch/2);
            sv = su + src_height * src_pitch / 4;
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = CLIP8[_vvtab[sv[j]] + _vutab[su[j]]];
                    u = CLIP8[_uutab[su[j]] + _uvtab[sv[j]]];
                    *(unsigned int *)(d + j*4) = u | (_yytab[sy[j*2]] << 8) | (v << 16) | (_yytab[sy[j*2+1]] << 24);
                }
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = CLIP8[_vvtab[sv[j]] + _vutab[su[j]]];
                    u = CLIP8[_uutab[su[j]] + _uvtab[sv[j]]];
                    *(unsigned int *)(d + j*4) = u | (_yytab[sy[j*2]] << 8) | (v << 16) | (_yytab[sy[j*2+1]] << 24);
                    *(unsigned int *)(d + j*4 + dest_pitch) = u | (_yytab[sy[j*2 + src_pitch]] << 8) | (v << 16) | (_yytab[sy[j*2+1 + src_pitch]] << 24);
                }
                sy += src_pitch*2;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch*2;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = CLIP8[_vvtab[sv[j]] + _vutab[su[j]]];
                    u = CLIP8[_uutab[su[j]] + _uvtab[sv[j]]];
                    *(unsigned int *)(d + j*4) = u | (_yytab[sy[j*2]] << 8) | (v << 16) | (_yytab[sy[j*2+1]] << 24);
                }
            }
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * I420->RGB* double-line converters:
 */
static void dblineI420toRGB32 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, rv, guv, bu;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu])       |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu])       |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 16);

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;           /* shift luma !!! */
        d1 += BPP4; d2 += BPP4;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu])       |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu])       |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 16);

        /* 2nd row: */
        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line BGR0 */
        *(unsigned int *)(d1+BPP4) =
            (CLIP8[y1 + bu])       |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+BPP4) =
            (CLIP8[y2 + bu])       |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 16);

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP4; d2 += 2*BPP4;
    }

    /* convert the last 2x1 block: */
    if (dx & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu])       |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu])       |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 16);
    }
}

static void dblineI420toRGB32alpha (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, ruv, guv, buv;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv])      |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv])      |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 16);

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;           /* shift luma !!! */
        d1 += BPP4; d2 += BPP4;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv])      |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv])      |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 16);

        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line BGR0 */
        *(unsigned int *)(d1+BPP4) =
            (CLIP8[y1 + buv])      |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+BPP4) =
            (CLIP8[y2 + buv])      |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 16);

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP4; d2 += 2*BPP4;
    }

    /* convert last 2x1 block: */
    if (dx & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv])      |
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 16);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv])      |
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 16);
    }
}

static void dblineI420toBGR32 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, rv, guv, bu;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu]  << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu]  << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 0);

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;           /* shift luma !!! */
        d1 += BPP4; d2 += BPP4;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu]  << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu]  << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 0);

        /* 2nd row: */
        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line BGR0 */
        *(unsigned int *)(d1+BPP4) =
            (CLIP8[y1 + bu]  << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+BPP4) =
            (CLIP8[y2 + bu]  << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 0);

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP4; d2 += 2*BPP4;
    }

    /* convert the last 2x1 block: */
    if (dx & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu]  << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + rv]  << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu]  << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + rv]  << 0);
    }
}

static void dblineI420toBGR32alpha (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, ruv, guv, buv;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv] << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv] << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 0);

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;           /* shift luma !!! */
        d1 += BPP4; d2 += BPP4;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv] << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv] << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 0);

        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line BGR0 */
        *(unsigned int *)(d1+BPP4) =
            (CLIP8[y1 + buv] << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+BPP4) =
            (CLIP8[y2 + buv] << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 0);

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP4; d2 += 2*BPP4;
    }

    /* convert last 2x1 block: */
    if (dx & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv] << 16)|
            (CLIP8[y1 + guv] << 8) |
            (CLIP8[y1 + ruv] << 0);

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv] << 16)|
            (CLIP8[y2 + guv] << 8) |
            (CLIP8[y2 + ruv] << 0);
    }
}

static void dblineI420toRGB24 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    /* check if we can use a fast 32-bit code: */
    if (LITTLE_ENDIAN && !((src_x ^ dest_x) & 1)) {

        /* aligned input/output on little-endian machine: */
        register int y11, y12, y13, y14, y21, y22, y23, y24;
        register int rv, rv2, guv, guv2, bu, bu2;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP3; d2 += BPP3;
            dest_x ++; dx --;
        }

        /* convert first 2x2 block: */
        if (dest_x & 2) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];

            y12 = ytab[sy1[1]];
            y22 = ytab[sy2[1]];

            /* first line BGR: */
            d1[0+BPP3] = CLIP8[y12 + bu];
            d1[1+BPP3] = CLIP8[y12 + guv];
            d1[2+BPP3] = CLIP8[y12 + rv];

            /* second line BGR: */
            d2[0+BPP3] = CLIP8[y22 + bu];
            d2[1+BPP3] = CLIP8[y22 + guv];
            d2[2+BPP3] = CLIP8[y22 + rv];

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP3; d2 += 2*BPP3;
            dest_x += 2; dx -= 2;
        }

        /* convert all integral 2x4 blocks: */
        for (i = 0; i < dx/4; i ++) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* first line BGRB */
            *(unsigned int *)(d1+0) =
                (CLIP8[y11 + bu])        |
                (CLIP8[y11 + guv] << 8)  |
                (CLIP8[y11 + rv]  << 16) |
                (CLIP8[y12 + bu]  << 24) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            /* second line BGRB */
            *(unsigned int *)(d2+0) =
                (CLIP8[y21 + bu])        |
                (CLIP8[y21 + guv] << 8)  |
                (CLIP8[y21 + rv]  << 16) |
                (CLIP8[y22 + bu]  << 24) ;

            bu2 = butab[su[1]];
            guv2 = gutab[su[1]] + gvtab[sv[1]];
            y13 = ytab[sy1[2]];

            /* first line GRBG */
            *(unsigned int *)(d1+4) =
                (CLIP8[y12 + guv])        |
                (CLIP8[y12 + rv]   << 8)  |
                (CLIP8[y13 + bu2]  << 16) |
                (CLIP8[y13 + guv2] << 24) ;

            y23 = ytab[sy2[2]];

            /* second line GRBG */
            *(unsigned int *)(d2+4) =
                (CLIP8[y22 + guv])        |
                (CLIP8[y22 + rv]   << 8)  |
                (CLIP8[y23 + bu2]  << 16) |
                (CLIP8[y23 + guv2] << 24) ;

            y14 = ytab[sy1[3]];
            rv2 = rvtab[sv[1]];

            /* first line RBGR */
            *(unsigned int *)(d1+8) =
                (CLIP8[y13 + rv2])        |
                (CLIP8[y14 + bu2]  << 8)  |
                (CLIP8[y14 + guv2] << 16) |
                (CLIP8[y14 + rv2]  << 24) ;

            y24 = ytab[sy2[3]];

            /* second line RBGR */
            *(unsigned int *)(d2+8) =
                (CLIP8[y23 + rv2])        |
                (CLIP8[y24 + bu2]  << 8)  |
                (CLIP8[y24 + guv2] << 16) |
                (CLIP8[y24 + rv2]  << 24) ;

            /* next 4x2 block */
            sy1 += 4; sy2 += 4;
            su += 2; sv += 2;
            d1 += 4*BPP3; d2 += 4*BPP3;
        }

        /* convert last 2x2 block: */
        if (dx & 2) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];

            y12 = ytab[sy1[1]];
            y22 = ytab[sy2[1]];

            /* first line BGR: */
            d1[0+BPP3] = CLIP8[y12 + bu];
            d1[1+BPP3] = CLIP8[y12 + guv];
            d1[2+BPP3] = CLIP8[y12 + rv];

            /* second line BGR: */
            d2[0+BPP3] = CLIP8[y22 + bu];
            d2[1+BPP3] = CLIP8[y22 + guv];
            d2[2+BPP3] = CLIP8[y22 + rv];

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP3; d2 += 2*BPP3;
            dx -= 2;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];
        }

    } else {

        /* more generic implementation: */
        register int y11, y12, y21, y22;
        register int bu, rv, guv;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP3; d2 += BPP3;
            dest_x ++; dx --;
        }

        /* convert all integral 2x2 blocks: */
        for (i = 0; i < dx/2; i ++) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* first line BGR,BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];
            d1[3] = CLIP8[y12 + bu];
            d1[4] = CLIP8[y12 + guv];
            d1[5] = CLIP8[y12 + rv];

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            /* second line BGR,BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];
            d2[3] = CLIP8[y22 + bu];
            d2[4] = CLIP8[y22 + guv];
            d2[5] = CLIP8[y22 + rv];

            /* next 2x2 block */
            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP3; d2 += 2*BPP3;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            bu = butab[su[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            rv = rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];
        }
    }
}

static void dblineI420toRGB24alpha (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    /* check if we can use a fast 32-bit code: */
    if (LITTLE_ENDIAN && !((src_x ^ dest_x) & 1)) {

        /* aligned input/output on little-endian machine: */
        register int y11, y12, y13, y14, y21, y22, y23, y24;
        register int ruv, ruv2, guv, guv2, buv, buv2;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR */
            d1[0] = CLIP8[y11 + buv];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + ruv];

            /* second line BGR */
            d2[0] = CLIP8[y21 + buv];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + ruv];

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP3; d2 += BPP3;
            dest_x ++; dx --;
        }

        /* convert first 2x2 block: */
        if (dest_x & 2) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR */
            d1[0] = CLIP8[y11 + buv];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + ruv];

            /* second line BGR */
            d2[0] = CLIP8[y21 + buv];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + ruv];

            y12 = ytab[sy1[1]];
            y22 = ytab[sy2[1]];

            /* first line BGR */
            d1[0+BPP3] = CLIP8[y12 + buv];
            d1[1+BPP3] = CLIP8[y12 + guv];
            d1[2+BPP3] = CLIP8[y12 + ruv];

            /* second line BGR */
            d2[0+BPP3] = CLIP8[y22 + buv];
            d2[1+BPP3] = CLIP8[y22 + guv];
            d2[2+BPP3] = CLIP8[y22 + ruv];

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP3; d2 += 2*BPP3;
            dest_x += 2; dx -= 2;
        }

        /* convert all integral 2x4 blocks: */
        for (i = 0; i < dx/4; i ++) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* first line BGRB */
            *(unsigned int *)(d1+0) =
                (CLIP8[y11 + buv])       |
                (CLIP8[y11 + guv] << 8)  |
                (CLIP8[y11 + ruv] << 16) |
                (CLIP8[y12 + buv] << 24) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            /* second line BGRB */
            *(unsigned int *)(d2+0) =
                (CLIP8[y21 + buv])       |
                (CLIP8[y21 + guv] << 8)  |
                (CLIP8[y21 + ruv] << 16) |
                (CLIP8[y22 + buv] << 24) ;

            buv2 = butab[su[1]] + bvtab[sv[1]];
            guv2 = gutab[su[1]] + gvtab[sv[1]];
            y13 = ytab[sy1[2]];

            /* first line GRBG */
            *(unsigned int *)(d1+4) =
                (CLIP8[y12 + guv])        |
                (CLIP8[y12 + ruv]  << 8)  |
                (CLIP8[y13 + buv2] << 16) |
                (CLIP8[y13 + guv2] << 24) ;

            y23 = ytab[sy2[2]];

            /* second line GRBG */
            *(unsigned int *)(d2+4) =
                (CLIP8[y22 + guv])        |
                (CLIP8[y22 + ruv]  << 8)  |
                (CLIP8[y23 + buv2] << 16) |
                (CLIP8[y23 + guv2] << 24) ;

            y14 = ytab[sy1[3]];
            ruv2 = rutab[su[1]]+rvtab[sv[1]];

            /* first line RBGR */
            *(unsigned int *)(d1+8) =
                (CLIP8[y13 + ruv2])       |
                (CLIP8[y14 + buv2] << 8)  |
                (CLIP8[y14 + guv2] << 16) |
                (CLIP8[y14 + ruv2] << 24) ;

            y24 = ytab[sy2[3]];

            /* second line RBGR */
            *(unsigned int *)(d2+8) =
                (CLIP8[y23 + ruv2])       |
                (CLIP8[y24 + buv2] << 8)  |
                (CLIP8[y24 + guv2] << 16) |
                (CLIP8[y24 + ruv2] << 24) ;

            /* next 4x2 block */
            sy1 += 4; sy2 += 4;
            su += 2; sv += 2;
            d1 += 4*BPP3; d2 += 4*BPP3;
        }

        /* convert last 2x2 block: */
        if (dx & 2) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR */
            d1[0] = CLIP8[y11 + buv];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + ruv];

            /* second line BGR */
            d2[0] = CLIP8[y21 + buv];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + ruv];

            y12 = ytab[sy1[1]];
            y22 = ytab[sy2[1]];

            /* first line BGR */
            d1[0+BPP3] = CLIP8[y12 + buv];
            d1[1+BPP3] = CLIP8[y12 + guv];
            d1[2+BPP3] = CLIP8[y12 + ruv];

            /* second line BGR */
            d2[0+BPP3] = CLIP8[y22 + buv];
            d2[1+BPP3] = CLIP8[y22 + guv];
            d2[2+BPP3] = CLIP8[y22 + ruv];

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP3; d2 += 2*BPP3;
            dx -= 2;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR */
            d1[0] = CLIP8[y11 + buv];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + ruv];

            /* second line BGR */
            d2[0] = CLIP8[y21 + buv];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + ruv];
        }

    } else {

        /* a more generic implentation */
        register int y11, y12, y21, y22;
        register int ruv, guv, buv;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR */
            d1[0] = CLIP8[y11 + buv];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + ruv];

            /* second line BGR */
            d2[0] = CLIP8[y21 + buv];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + ruv];

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP3; d2 += BPP3;
            dest_x ++; dx --;
        }

        /* convert all integral 2x4 blocks: */
        for (i = 0; i < dx/2; i ++) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* first line BGR,BGR: */
            d1[0] = CLIP8[y11 + buv];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + ruv];
            d1[3] = CLIP8[y12 + buv];
            d1[4] = CLIP8[y12 + guv];
            d1[5] = CLIP8[y12 + ruv];

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            /* second line BGR,BGR: */
            d2[0] = CLIP8[y21 + buv];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + ruv];
            d2[3] = CLIP8[y22 + buv];
            d2[4] = CLIP8[y22 + guv];
            d2[5] = CLIP8[y22 + ruv];

            /* next 2x2 block */
            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP3; d2 += 2*BPP3;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            buv = butab[su[0]] + bvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            ruv = rutab[su[0]] + rvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* first line BGR */
            d1[0] = CLIP8[y11 + buv];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + ruv];

            /* second line BGR */
            d2[0] = CLIP8[y21 + buv];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + ruv];
        }
    }
}

static void dblineI420toRGB565 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    /* check if we have misaligned input/output: */
    if ((src_x ^ dest_x) & 1) {

        ; /* not implemented yet */

    } else {

        /* aligned input/output: */
        register int y11, y12, y21, y22;
        register int rv, guv, bu;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            rv  = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu  = butab[su[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 11) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 11) ;

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP2; d2 += BPP2;
            dest_x ++; dx --;
        }

        /* convert first 2x2 block: */
        if (dest_x & 2) {

            rv  = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu  = butab[su[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 11) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 27) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 11) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 27) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dest_x += 2; dx -= 2;
        }

        /* convert all integral 2x4 blocks: */
        for (i = 0; i < dx/4; i ++) {

            /* first 2x2 block */
            rv = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu = butab[su[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 11) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 27) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 11) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 27) ;

            /* second 2x2 block */
            rv  = rvtab[sv[1]];
            guv = gutab[su[1]] + gvtab[sv[1]];
            bu  = butab[su[1]];
            y11 = ytab[sy1[2]];
            y12 = ytab[sy1[3]];

            *(unsigned int *)(d1+2*BPP2) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 11) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 27) ;

            y21 = ytab[sy2[2]];
            y22 = ytab[sy2[3]];

            *(unsigned int *)(d2+2*BPP2) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 11) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 27) ;

            /* next 4x2 block */
            sy1 += 4; sy2 += 4;
            su += 2; sv += 2;
            d1 += 4*BPP2; d2 += 4*BPP2;
        }

        /* convert last 2x2 block: */
        if (dx & 2) {

            rv = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu = butab[su[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 11) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 27) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 11) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 27) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dx -= 2;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            rv  = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu  = butab[su[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 11) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 11) ;
        }
    }
}

static void dblineI420toRGB565alpha (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    /* check if we have misaligned input/output: */
    if ((src_x ^ dest_x) & 1) {

        ; /* not implemented yet */

    } else {

        /* aligned input/output: */
        register int y11, y12, y21, y22;
        register int ruv, guv, buv;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 11) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 11) ;

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP2; d2 += BPP2;
            dest_x ++; dx --;
        }

        /* convert first 2x2 block: */
        if (dest_x & 2) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 11) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 27) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 11) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 27) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dest_x += 2; dx -= 2;
        }

        /* convert all integral 2x4 blocks: */
        for (i = 0; i < dx/4; i ++) {

            /* first 2x2 block */
            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 11) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 27) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 11) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 27) ;

            /* second 2x2 block */
            ruv = rutab[su[1]] + rvtab[sv[1]];
            guv = gutab[su[1]] + gvtab[sv[1]];
            buv = butab[su[1]] + bvtab[sv[1]];
            y11 = ytab[sy1[2]];
            y12 = ytab[sy1[3]];

            *(unsigned int *)(d1+2*BPP2) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 11) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 27) ;

            y21 = ytab[sy2[2]];
            y22 = ytab[sy2[3]];

            *(unsigned int *)(d2+2*BPP2) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 11) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 27) ;

            /* next 4x2 block */
            sy1 += 4; sy2 += 4;
            su += 2; sv += 2;
            d1 += 4*BPP2; d2 += 4*BPP2;
        }

        /* convert last 2x2 block: */
        if (dx & 2) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 11) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP6[y12 + guv + DITH6H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 27) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 11) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP6[y22 + guv + DITH6L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 27) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dx -= 2;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP6[y11 + guv + DITH6L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 11) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP6[y21 + guv + DITH6H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 11) ;
        }
    }
}

static void dblineI420toRGB555 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    /* check if we have misaligned input/output: */
    if ((src_x ^ dest_x) & 1) {

        ; /* not implemented yet */

    } else {

        /* aligned input/output: */
        register int y11, y12, y21, y22;
        register int rv, guv, bu;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            rv  = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu  = butab[su[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 10) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 10) ;

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP2; d2 += BPP2;
            dest_x ++; dx --;
        }

        /* convert first 2x2 block: */
        if (dest_x & 2) {

            rv  = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu  = butab[su[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 10) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 26) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 10) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 26) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dest_x += 2; dx -= 2;
        }

        /* convert all integral 2x4 blocks: */
        for (i = 0; i < dx/4; i ++) {

            /* first 2x2 block */
            rv = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu = butab[su[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 10) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 26) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 10) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 26) ;

            /* second 2x2 block */
            rv  = rvtab[sv[1]];
            guv = gutab[su[1]] + gvtab[sv[1]];
            bu  = butab[su[1]];
            y11 = ytab[sy1[2]];
            y12 = ytab[sy1[3]];

            *(unsigned int *)(d1+2*BPP2) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 10) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 26) ;

            y21 = ytab[sy2[2]];
            y22 = ytab[sy2[3]];

            *(unsigned int *)(d2+2*BPP2) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 10) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 26) ;

            /* next 4x2 block */
            sy1 += 4; sy2 += 4;
            su += 2; sv += 2;
            d1 += 4*BPP2; d2 += 4*BPP2;
        }

        /* convert last 2x2 block: */
        if (dx & 2) {

            rv = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu = butab[su[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 10) |
                (CLIP5[y12 + bu  + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + rv  + DITH5H] << 26) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 10) |
                (CLIP5[y22 + bu  + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + rv  + DITH5L] << 26) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dx -= 2;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            rv  = rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            bu  = butab[su[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + bu  + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + rv  + DITH5L] << 10) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + bu  + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + rv  + DITH5H] << 10) ;
        }
    }
}

static void dblineI420toRGB555alpha (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    /* check if we have misaligned input/output: */
    if ((src_x ^ dest_x) & 1) {

        ; /* not implemented yet */

    } else {

        /* aligned input/output: */
        register int y11, y12, y21, y22;
        register int ruv, guv, buv;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 10) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 10) ;

            sy1 += 1; sy2 += 1;
            su += 1; sv += 1;
            d1 += BPP2; d2 += BPP2;
            dest_x ++; dx --;
        }

        /* convert first 2x2 block: */
        if (dest_x & 2) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 10) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 26) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 10) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 26) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dest_x += 2; dx -= 2;
        }

        /* convert all integral 2x4 blocks: */
        for (i = 0; i < dx/4; i ++) {

            /* first 2x2 block */
            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 10) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 26) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 10) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 26) ;

            /* second 2x2 block */
            ruv = rutab[su[1]] + rvtab[sv[1]];
            guv = gutab[su[1]] + gvtab[sv[1]];
            buv = butab[su[1]] + bvtab[sv[1]];
            y11 = ytab[sy1[2]];
            y12 = ytab[sy1[3]];

            *(unsigned int *)(d1+2*BPP2) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 10) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 26) ;

            y21 = ytab[sy2[2]];
            y22 = ytab[sy2[3]];

            *(unsigned int *)(d2+2*BPP2) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 10) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 26) ;

            /* next 4x2 block */
            sy1 += 4; sy2 += 4;
            su += 2; sv += 2;
            d1 += 4*BPP2; d2 += 4*BPP2;
        }

        /* convert last 2x2 block: */
        if (dx & 2) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y12 = ytab[sy1[1]];

            /* output 4 bytes at a time */
            *(unsigned int *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 10) |
                (CLIP5[y12 + buv + DITH5H] << 16) |
                (CLIP5[y12 + guv + DITH5H] << 21) |
                (CLIP5[y12 + ruv + DITH5H] << 26) ;

            y21 = ytab[sy2[0]];
            y22 = ytab[sy2[1]];

            *(unsigned int *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 10) |
                (CLIP5[y22 + buv + DITH5L] << 16) |
                (CLIP5[y22 + guv + DITH5L] << 21) |
                (CLIP5[y22 + ruv + DITH5L] << 26) ;

            sy1 += 2; sy2 += 2;
            su += 1; sv += 1;
            d1 += 2*BPP2; d2 += 2*BPP2;
            dx -= 2;
        }

        /* convert last 2x1 block: */
        if (dx & 1) {

            ruv = rutab[su[0]] + rvtab[sv[0]];
            guv = gutab[su[0]] + gvtab[sv[0]];
            buv = butab[su[0]] + bvtab[sv[0]];
            y11 = ytab[sy1[0]];
            y21 = ytab[sy2[0]];

            /* output 2 bytes at a time */
            *(unsigned short *)(d1+0) =
                (CLIP5[y11 + buv + DITH5L] << 0)  |
                (CLIP5[y11 + guv + DITH5L] << 5)  |
                (CLIP5[y11 + ruv + DITH5L] << 10) ;

            *(unsigned short *)(d2+0) =
                (CLIP5[y21 + buv + DITH5H] << 0)  |
                (CLIP5[y21 + guv + DITH5H] << 5)  |
                (CLIP5[y21 + ruv + DITH5H] << 10) ;
        }
    }
}

static void dblineI420toRGB8 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, rv, guv, bu;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(d1+0) = pmap[
            (CLIP4[y1 + bu  + DITH4H] << 0) |
            (CLIP4[y1 + guv + DITH4H] << 4) |
            (CLIP4[y1 + rv  + DITH4H] << 8)];

        /* second line BGR0 */
        *(d2+0) = pmap[
            (CLIP4[y2 + bu  + DITH4L] << 0) |
            (CLIP4[y2 + guv + DITH4L] << 4) |
            (CLIP4[y2 + rv  + DITH4L] << 8)];

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;           /* shift luma !!! */
        d1 += BPP1; d2 += BPP1;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(d1+0) = pmap[
            (CLIP4[y1 + bu  + DITH4L] << 0) |
            (CLIP4[y1 + guv + DITH4L] << 4) |
            (CLIP4[y1 + rv  + DITH4L] << 8)];

        /* second line BGR0 */
        *(d2+0) = pmap[
            (CLIP4[y2 + bu  + DITH4H] << 0) |
            (CLIP4[y2 + guv + DITH4H] << 4) |
            (CLIP4[y2 + rv  + DITH4H] << 8)];

        /* 2nd column: */
        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line BGR0 */
        *(d1+BPP1) = pmap[
            (CLIP4[y1 + bu  + DITH4H] << 0) |
            (CLIP4[y1 + guv + DITH4H] << 4) |
            (CLIP4[y1 + rv  + DITH4H] << 8)];

        /* second line BGR0 */
        *(d2+BPP1) = pmap[
            (CLIP4[y2 + bu  + DITH4L] << 0) |
            (CLIP4[y2 + guv + DITH4L] << 4) |
            (CLIP4[y2 + rv  + DITH4L] << 8)];

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP1; d2 += 2*BPP1;
    }

    /* convert the last 2x1 block: */
    if (dx & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(d1+0) = pmap[
            (CLIP4[y1 + bu  + DITH4L] << 0) |
            (CLIP4[y1 + guv + DITH4L] << 4) |
            (CLIP4[y1 + rv  + DITH4L] << 8)];

        /* second line BGR0 */
        *(d2+0) = pmap[
            (CLIP4[y2 + bu  + DITH4H] << 0) |
            (CLIP4[y2 + guv + DITH4H] << 4) |
            (CLIP4[y2 + rv  + DITH4H] << 8)];
    }
}

static void dblineI420toRGB8alpha (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, ruv, guv, buv;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(d1+0) = pmap[
            (CLIP4[y1 + buv + DITH4H] << 0) |
            (CLIP4[y1 + guv + DITH4H] << 4) |
            (CLIP4[y1 + ruv + DITH4H] << 8)];

        /* second line BGR0 */
        *(d2+0) = pmap[
            (CLIP4[y2 + buv + DITH4L] << 0) |
            (CLIP4[y2 + guv + DITH4L] << 4) |
            (CLIP4[y2 + ruv + DITH4L] << 8)];

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;           /* shift luma !!! */
        d1 += BPP1; d2 += BPP1;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(d1+0) = pmap[
            (CLIP4[y1 + buv + DITH4L] << 0) |
            (CLIP4[y1 + guv + DITH4L] << 4) |
            (CLIP4[y1 + ruv + DITH4L] << 8)];

        /* second line BGR0 */
        *(d2+0) = pmap[
            (CLIP4[y2 + buv + DITH4H] << 0) |
            (CLIP4[y2 + guv + DITH4H] << 4) |
            (CLIP4[y2 + ruv + DITH4H] << 8)];

        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line BGR0 */
        *(d1+BPP1) = pmap[
            (CLIP4[y1 + buv + DITH4H] << 0) |
            (CLIP4[y1 + guv + DITH4H] << 4) |
            (CLIP4[y1 + ruv + DITH4H] << 8)];

        /* second line BGR0 */
        *(d2+BPP1) = pmap[
            (CLIP4[y2 + buv + DITH4L] << 0) |
            (CLIP4[y2 + guv + DITH4L] << 4) |
            (CLIP4[y2 + ruv + DITH4L] << 8)];

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP1; d2 += 2*BPP1;
    }

    /* convert last 2x1 block: */
    if (dx & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line BGR0 */
        *(d1+0) = pmap[
            (CLIP4[y1 + buv + DITH4L] << 0) |
            (CLIP4[y1 + guv + DITH4L] << 4) |
            (CLIP4[y1 + ruv + DITH4L] << 8)];

        /* second line BGR0 */
        *(d2+0) = pmap[
            (CLIP4[y2 + buv + DITH4H] << 0) |
            (CLIP4[y2 + guv + DITH4H] << 4) |
            (CLIP4[y2 + ruv + DITH4H] << 8)];
    }
}

/*
 * Convert two YUV lines into RGB linebufs.
 * Produces two RGB lines per call.
 * Output in padded RGB format, needed for SIMD interpolation.
 */
static void dblineI420toXRGB (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, rv, guv, bu;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu])        |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + rv]  << 22) ;

        /* second line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu])        |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + rv]  << 22) ;

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;
        d1 += BPP4; d2 += BPP4;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu])        |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + rv]  << 22) ;

        /* second line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu])        |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + rv]  << 22) ;

        /* 2nd row: */
        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+BPP4) =
            (CLIP8[y1 + bu])        |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + rv]  << 22) ;

        /* second line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d2+BPP4) =
            (CLIP8[y2 + bu])        |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + rv]  << 22) ;

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP4; d2 += 2*BPP4;
    }

    /* convert last 2x1 block: */
    if (dx & 1) {

        bu = butab[su[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        rv = rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + bu])        |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + rv]  << 22) ;

        /* second line BGR0 */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + bu])        |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + rv]  << 22) ;
    }
}

static void dblineI420toXRGBalpha (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx)
{
    register int y1, y2, ruv, guv, buv;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv])       |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + ruv] << 22);

        /* second line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv])       |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + ruv] << 22) ;

        sy1 += 1; sy2 += 1;
        su += 1; sv += 1;
        d1 += BPP4; d2 += BPP4;
        dx --;
    }

    /* convert all integral 2x2 blocks: */
    for (i = 0; i < dx/2; i ++) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv])       |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + ruv] << 22) ;

        /* second line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv])       |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + ruv] << 22) ;

        y1 = ytab[sy1[1]];
        y2 = ytab[sy2[1]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+BPP4) =
            (CLIP8[y1 + buv])       |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + ruv] << 22) ;

        /* second line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d2+BPP4) =
            (CLIP8[y2 + buv])       |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + ruv] << 22) ;

        /* next 2x2 block */
        sy1 += 2; sy2 += 2;
        su += 1; sv += 1;
        d1 += 2*BPP4; d2 += 2*BPP4;
    }

    /* convert last 2x1 block: */
    if (dx & 1) {

        buv = butab[su[0]] + bvtab[sv[0]];
        guv = gutab[su[0]] + gvtab[sv[0]];
        ruv = rutab[su[0]] + rvtab[sv[0]];
        y1 = ytab[sy1[0]];
        y2 = ytab[sy2[0]];

        /* first line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d1+0) =
            (CLIP8[y1 + buv])       |
            (CLIP8[y1 + guv] << 11) |
            (CLIP8[y1 + ruv] << 22) ;

        /* second line 00rrrrrr.rr000ggg.ggggg000.bbbbbbbb: */
        *(unsigned int *)(d2+0) =
            (CLIP8[y2 + buv])       |
            (CLIP8[y2 + guv] << 11) |
            (CLIP8[y2 + ruv] << 22) ;
    }
}

/*
 * Interpolate and pack RGB lines into final output
 * Produces two output lines per call.
 * Requires padded RGB for SIMD interpolation.
 */
#define ROUND888 0x00400801

/* RGB32 version: */
static void dblineXRGBtoRGB32x2 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *s1, unsigned char *s2, int src_x, int dx)
{
    register unsigned int a, b, c, d, e, f;
    register unsigned int w, x, y, z;

    /* convert first 2x1 block: */
    if (dest_x & 1) {

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(unsigned int *)(d1+0) =
            ((w & 0x000000ff) >> 0) |
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 6);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) >> 1) |
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 7);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) >> 2) |
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 8);

        /* bump pointers to next block */
        s1 += BPP4; s2 += BPP4;
        d1 += 2*BPP4; d2 += 2*BPP4;
        dx -= 1;
    }

    /* process all integral 2x2 blocks: */
    while (dx > 2) {    /* we need to leave at least one block */

        /*
         * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
         *
         * Input pels  =>   Output pels
         *  a b e           w  x  y  z
         *  c d f           w' x' y' z'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = *(unsigned int *)(s1+2*BPP4);

        w = a;
        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000ff) >> 0) |
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 6);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7);
        *(unsigned int *)(d1+2*BPP4) =
            ((y & 0x000000ff) >> 0) |
            ((y & 0x0007f800) >> 3) |
            ((y & 0x3fc00000) >> 6);
        *(unsigned int *)(d1+3*BPP4) =
            ((z & 0x000001fe) >> 1) |
            ((z & 0x000ff000) >> 4) |
            ((z & 0x7f800000) >> 7);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = *(unsigned int *)(s2+2*BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) >> 1) |
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 7);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) >> 2) |
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 8);
        *(unsigned int *)(d2+2*BPP4) =
            ((y & 0x000001fe) >> 1) |
            ((y & 0x000ff000) >> 4) |
            ((y & 0x7f800000) >> 7);
        *(unsigned int *)(d2+3*BPP4) =
            ((z & 0x000003fc) >> 2) |
            ((z & 0x001fe000) >> 5) |
            ((z & 0xff000000) >> 8);

        /* bump pointers to next 2x2 input */
        s1 += 2*BPP4; s2 += 2*BPP4;
        d1 += 4*BPP4; d2 += 4*BPP4;
        dx -= 2;
    }

    /* check if this is the last 2x2 block: */
    if (dx > 1) {

        /*
         * For last 4 output pels, repeat final input pel
         * for offscreen input.  Equivalent to pixel-doubling the
         * last output pel.
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = b;      /* repeat last input pel */
        w = a;

        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000ff) >> 0) |
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 6);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7);
        *(unsigned int *)(d1+2*BPP4) =
            ((y & 0x000000ff) >> 0) |
            ((y & 0x0007f800) >> 3) |
            ((y & 0x3fc00000) >> 6);
        *(unsigned int *)(d1+3*BPP4) =
            ((z & 0x000001fe) >> 1) |
            ((z & 0x000ff000) >> 4) |
            ((z & 0x7f800000) >> 7);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = d;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) >> 1) |
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 7);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) >> 2) |
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 8);
        *(unsigned int *)(d2+2*BPP4) =
            ((y & 0x000001fe) >> 1) |
            ((y & 0x000ff000) >> 4) |
            ((y & 0x7f800000) >> 7);
        *(unsigned int *)(d2+3*BPP4) =
            ((z & 0x000003fc) >> 2) |
            ((z & 0x001fe000) >> 5) |
            ((z & 0xff000000) >> 8);

    } else {
        /* last 2x1 block: */

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = a;      /* repeat last input pel */

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(unsigned int *)(d1+0) =
            ((w & 0x000000ff) >> 0) |
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 6);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = c;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) >> 1) |
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 7);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) >> 2) |
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 8);
    }
}

/* BGR32 version: */

static void dblineXRGBtoBGR32x2 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *s1, unsigned char *s2, int src_x, int dx)
{
    register unsigned int a, b, c, d, e, f;
    register unsigned int w, x, y, z;

    /* convert first 2x1 block: */
    if (dest_x & 1) {

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(unsigned int *)(d1+0) =
            ((w & 0x000000ff) << 16)|
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 22);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) << 15)|
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 23);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) << 15)|
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 23);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) << 14)|
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 24);

        /* bump pointers to next block */
        s1 += BPP4; s2 += BPP4;
        d1 += 2*BPP4; d2 += 2*BPP4;
        dx -= 1;
    }

    /* process all integral 2x2 blocks: */
    while (dx > 2) {    /* we need to leave at least one block */

        /*
         * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
         *
         * Input pels  =>   Output pels
         *  a b e           w  x  y  z
         *  c d f           w' x' y' z'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = *(unsigned int *)(s1+2*BPP4);

        w = a;
        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000ff) << 16)|
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 22);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) << 15)|
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 23);
        *(unsigned int *)(d1+2*BPP4) =
            ((y & 0x000000ff) << 16)|
            ((y & 0x0007f800) >> 3) |
            ((y & 0x3fc00000) >> 22);
        *(unsigned int *)(d1+3*BPP4) =
            ((z & 0x000001fe) << 15)|
            ((z & 0x000ff000) >> 4) |
            ((z & 0x7f800000) >> 23);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = *(unsigned int *)(s2+2*BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) << 15)|
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 23);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) << 14)|
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 24);
        *(unsigned int *)(d2+2*BPP4) =
            ((y & 0x000001fe) << 15)|
            ((y & 0x000ff000) >> 4) |
            ((y & 0x7f800000) >> 23);
        *(unsigned int *)(d2+3*BPP4) =
            ((z & 0x000003fc) << 14)|
            ((z & 0x001fe000) >> 5) |
            ((z & 0xff000000) >> 24);

        /* bump pointers to next 2x2 input */
        s1 += 2*BPP4; s2 += 2*BPP4;
        d1 += 4*BPP4; d2 += 4*BPP4;
        dx -= 2;
    }

    /* check if this is the last 2x2 block: */
    if (dx > 1) {

        /*
         * For last 4 output pels, repeat final input pel
         * for offscreen input.  Equivalent to pixel-doubling the
         * last output pel.
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = b;      /* repeat last input pel */
        w = a;

        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000ff) << 16)|
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 22);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) << 15)|
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 23);
        *(unsigned int *)(d1+2*BPP4) =
            ((y & 0x000000ff) << 16)|
            ((y & 0x0007f800) >> 3) |
            ((y & 0x3fc00000) >> 22);
        *(unsigned int *)(d1+3*BPP4) =
            ((z & 0x000001fe) << 15)|
            ((z & 0x000ff000) >> 4) |
            ((z & 0x7f800000) >> 23);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = d;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) << 15)|
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 23);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) << 14)|
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 24);
        *(unsigned int *)(d2+2*BPP4) =
            ((y & 0x000001fe) << 15)|
            ((y & 0x000ff000) >> 4) |
            ((y & 0x7f800000) >> 23);
        *(unsigned int *)(d2+3*BPP4) =
            ((z & 0x000003fc) << 14)|
            ((z & 0x001fe000) >> 5) |
            ((z & 0xff000000) >> 24);

    } else {
        /* last 2x1 block: */

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = a;      /* repeat last input pel */

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(unsigned int *)(d1+0) =
            ((w & 0x000000ff) << 16)|
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 22);
        *(unsigned int *)(d1+BPP4) =
            ((x & 0x000001fe) << 15)|
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 23);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = c;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001fe) << 15)|
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 23);
        *(unsigned int *)(d2+BPP4) =
            ((x & 0x000003fc) << 14)|
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 24);
    }
}

/* RGB24 version: */
static void dblineXRGBtoRGB24x2 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *s1, unsigned char *s2, int src_x, int dx)
{
    register unsigned int a, b, c, d, e, f;
    register unsigned int w, x, y, z;

    /* convert first 2x1 block: */
    if (dest_x & 1) {

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(unsigned int *)d1 =           /* brgB */
            ((w & 0x000000ff) >> 0)  |
            ((w & 0x0007f800) >> 3)  |
            ((w & 0x3fc00000) >> 6)  |
            ((x & 0x000001fe) << 23);
        *(unsigned short *)(d1+4) =     /* GR */
            ((x & 0x000ff000) >> 12) |
            ((x & 0x7f800000) >> 15);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =           /* bgrB */
            ((w & 0x000001fe) >> 1)  |
            ((w & 0x000ff000) >> 4)  |
            ((w & 0x7f800000) >> 7)  |
            ((x & 0x000003fc) << 22);
        *(unsigned short *)(d2+4) =     /* GR */
            ((x & 0x001fe000) >> 13) |
            ((x & 0xff000000) >> 16);

        /* bump pointers to next block */
        s1 += BPP4; s2 += BPP4;
        d1 += 2*BPP3; d2 += 2*BPP3;
        dx -= 1;
    }

    /* process all integral 2x2 blocks: */
    while (dx > 2) {    /* we need to leave at least one block */

        /*
         * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
         *
         * Input pels  =>   Output pels
         *  a b e           w  x  y  z
         *  c d f           w' x' y' z'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = *(unsigned int *)(s1+2*BPP4);

        w = a;
        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)d1 =           /* brgB */
            ((w & 0x000000ff) >> 0)  |
            ((w & 0x0007f800) >> 3)  |
            ((w & 0x3fc00000) >> 6)  |
            ((x & 0x000001fe) << 23);
        *(unsigned int *)(d1+4) =       /* GRbg */
            ((x & 0x000ff000) >> 12) |
            ((x & 0x7f800000) >> 15) |
            ((y & 0x000000ff) << 16) |
            ((y & 0x0007f800) << 13);
        *(unsigned int *)(d1+8) =       /* rBGR */
            ((y & 0x3fc00000) >> 22) |
            ((z & 0x000001fe) << 7)  |
            ((z & 0x000ff000) << 4)  |
            ((z & 0x7f800000) << 1);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = *(unsigned int *)(s2+2*BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =           /* bgrB */
            ((w & 0x000001fe) >> 1)  |
            ((w & 0x000ff000) >> 4)  |
            ((w & 0x7f800000) >> 7)  |
            ((x & 0x000003fc) << 22);
        *(unsigned int *)(d2+4) =       /* GRbg */
            ((x & 0x001fe000) >> 13) |
            ((x & 0xff000000) >> 16) |
            ((y & 0x000001fe) << 15) |
            ((y & 0x000ff000) << 12);
        *(unsigned int *)(d2+8) =       /* rBGR */
            ((y & 0x7f800000) >> 23) |
            ((z & 0x000003fc) << 6)  |
            ((z & 0x001fe000) << 3)  |
            ((z & 0xff000000) << 0);

        /* next 2x2 input block */
        s1 += 2*BPP4; s2 += 2*BPP4;
        d1 += 4*BPP3; d2 += 4*BPP3;
        dx -= 2;
    }

    /* check if this is the last 2x2 block: */
    if (dx > 1) {

        /*
         * For last 4 output pels, repeat final input pel
         * for offscreen input.  Equivalent to pixel-doubling the
         * last output pel.
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = b;      /* repeat last input pel */

        w = a;
        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)(d1+0) =       /* bgrB */
            ((w & 0x000000ff) >> 0)  |
            ((w & 0x0007f800) >> 3)  |
            ((w & 0x3fc00000) >> 6)  |
            ((x & 0x000001fe) << 23);
        *(unsigned int *)(d1+4) =       /* GRbg */
            ((x & 0x000ff000) >> 12) |
            ((x & 0x7f800000) >> 15) |
            ((y & 0x000000ff) << 16) |
            ((y & 0x0007f800) << 13);
        *(unsigned int *)(d1+8) =       /* rBGR */
            ((y & 0x3fc00000) >> 22) |
            ((z & 0x000001fe) << 7)  |
            ((z & 0x000ff000) << 4)  |
            ((z & 0x7f800000) << 1);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = d;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)(d2+0) =       /* bgrB */
            ((w & 0x000001fe) >> 1)  |
            ((w & 0x000ff000) >> 4)  |
            ((w & 0x7f800000) >> 7)  |
            ((x & 0x000003fc) << 22);
        *(unsigned int *)(d2+4) =       /* GRbg */
            ((x & 0x001fe000) >> 13) |
            ((x & 0xff000000) >> 16) |
            ((y & 0x000001fe) << 15) |
            ((y & 0x000ff000) << 12);
        *(unsigned int *)(d2+8) =       /* rBGR */
            ((y & 0x7f800000) >> 23) |
            ((z & 0x000003fc) << 6)  |
            ((z & 0x001fe000) << 3)  |
            ((z & 0xff000000) << 0);

    } else {
        /* last 2x1 block: */

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = a;      /* repeat last input pel */

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(unsigned int *)d1 =           /* brgB */
            ((w & 0x000000ff) >> 0)  |
            ((w & 0x0007f800) >> 3)  |
            ((w & 0x3fc00000) >> 6)  |
            ((x & 0x000001fe) << 23);
        *(unsigned short *)(d1+4) =     /* GR */
            ((x & 0x000ff000) >> 12) |
            ((x & 0x7f800000) >> 15);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = c;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)d2 =           /* bgrB */
            ((w & 0x000001fe) >> 1)  |
            ((w & 0x000ff000) >> 4)  |
            ((w & 0x7f800000) >> 7)  |
            ((x & 0x000003fc) << 22);
        *(unsigned short *)(d2+4) =     /* GR */
            ((x & 0x001fe000) >> 13) |
            ((x & 0xff000000) >> 16);
    }
}

/* RGB565 version: */
static void dblineXRGBtoRGB565x2 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *s1, unsigned char *s2, int src_x, int dx)
{
    register unsigned int a, b, c, d, e, f;
    register unsigned int w, x, y, z;

    /* convert first 2x1 block: */
    if (dest_x & 1) {

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);

        w = a;
        x = a + b;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007e000) >> 8)  |
            ((w & 0x3e000000) >> 14) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000fc000) << 7)  |
            ((x & 0x7c000000) << 1);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);

        w = a + c;
        x = a + b + c + d;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000fc000) >> 9)  |
            ((w & 0x7c000000) >> 15) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f8000) << 6)  |
            ((x & 0xf8000000) << 0);

        /* bump pointers to next block */
        s1 += BPP4; s2 += BPP4;
        d1 += 2*BPP2; d2 += 2*BPP2;
        dx -= 1;
    }

    /* process all integral 2x2 blocks: */
    while (dx > 2) {    /* we need to leave at least one block */

        /*
         * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
         *
         * Input pels       Output pels
         *  a b e           w  x  y  z
         *  c d f           w' x' y' z'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = *(unsigned int *)(s1+2*BPP4);

        w = a;
        x = a + b;
        y = b;
        z = b + e;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007e000) >> 8)  |
            ((w & 0x3e000000) >> 14) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000fc000) << 7)  |
            ((x & 0x7c000000) << 1);
        *(unsigned int *)(d1+2*BPP2) =
            ((y & 0x000000f8) >> 3)  |
            ((y & 0x0007e000) >> 8)  |
            ((y & 0x3e000000) >> 14) |
            ((z & 0x000001f0) << 12) |
            ((z & 0x000fc000) << 7)  |
            ((z & 0x7c000000) << 1);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = *(unsigned int *)(s2+2*BPP4);

        w = a + c;
        x = a + b + c + d;
        y = b + d;
        z = b + e + d + f;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000fc000) >> 9)  |
            ((w & 0x7c000000) >> 15) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f8000) << 6)  |
            ((x & 0xf8000000) << 0);
        *(unsigned int *)(d2+2*BPP2) =
            ((y & 0x000001f0) >> 4)  |
            ((y & 0x000fc000) >> 9)  |
            ((y & 0x7c000000) >> 15) |
            ((z & 0x000003e0) << 11) |
            ((z & 0x001f8000) << 6)  |
            ((z & 0xf8000000) << 0);

        /* next 2x2 input block */
        s1 += 2*BPP4; s2 += 2*BPP4;
        d1 += 4*BPP2; d2 += 4*BPP2;
        dx -= 2;
    }

    /* check if this is the last 2x2 block: */
    if (dx > 1) {

        /*
         * For last 4 output pels, repeat final input pel
         * for offscreen input.  Equivalent to pixel-doubling the
         * last output pel.
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = b;      /* repeat last input pel */

        w = a;
        x = a + b;
        y = b;
        z = b + e;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007e000) >> 8)  |
            ((w & 0x3e000000) >> 14) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000fc000) << 7)  |
            ((x & 0x7c000000) << 1);
        *(unsigned int *)(d1+2*BPP2) =
            ((y & 0x000000f8) >> 3)  |
            ((y & 0x0007e000) >> 8)  |
            ((y & 0x3e000000) >> 14) |
            ((z & 0x000001f0) << 12) |
            ((z & 0x000fc000) << 7)  |
            ((z & 0x7c000000) << 1);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = d;      /* repeat last input pel */

        w = a + c;
        x = a + b + c + d;
        y = b + d;
        z = b + e + d + f;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000fc000) >> 9)  |
            ((w & 0x7c000000) >> 15) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f8000) << 6)  |
            ((x & 0xf8000000) << 0);
        *(unsigned int *)(d2+2*BPP2) =
            ((y & 0x000001f0) >> 4)  |
            ((y & 0x000fc000) >> 9)  |
            ((y & 0x7c000000) >> 15) |
            ((z & 0x000003e0) << 11) |
            ((z & 0x001f8000) << 6)  |
            ((z & 0xf8000000) << 0);

    } else {
        /* last 2x1 block: */

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = a;      /* repeat last input pel */

        w = a;
        x = a + b;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007e000) >> 8)  |
            ((w & 0x3e000000) >> 14) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000fc000) << 7)  |
            ((x & 0x7c000000) << 1);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = c;      /* repeat last input pel */

        w = a + c;
        x = a + b + c + d;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000fc000) >> 9)  |
            ((w & 0x7c000000) >> 15) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f8000) << 6)  |
            ((x & 0xf8000000) << 0);
    }
}

/* RGB555 version: */
static void dblineXRGBtoRGB555x2 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *s1, unsigned char *s2, int src_x, int dx)
{
    register unsigned int a, b, c, d, e, f;
    register unsigned int w, x, y, z;

    /* convert first 2x1 block: */
    if (dest_x & 1) {

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);

        w = a;
        x = a + b;

        /* pack and store */
        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007c000) >> 9)  |
            ((w & 0x3e000000) >> 15) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000f8000) << 6)  |
            ((x & 0x7c000000) << 0);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);

        w = a + c;
        x = a + b + c + d;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000f8000) >> 10) |
            ((w & 0x7c000000) >> 16) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f0000) << 5)  |
            ((x & 0xf8000000) >> 1);

        /* bump pointers to next block */
        s1 += BPP4; s2 += BPP4;
        d1 += 2*BPP2; d2 += 2*BPP2;
        dx -= 1;
    }

    /* process all integral 2x2 blocks: */
    while (dx > 2) {    /* we need to leave at least one block */

        /*
         * Input pels       Output pels
         *  a b e           w  x  y  z
         *  c d f           w' x' y' z'
         *
         * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = *(unsigned int *)(s1+2*BPP4);

        w = a;
        x = a + b;
        y = b;
        z = b + e;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007c000) >> 9)  |
            ((w & 0x3e000000) >> 15) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000f8000) << 6)  |
            ((x & 0x7c000000) << 0);
        *(unsigned int *)(d1+BPP4) =
            ((y & 0x000000f8) >> 3)  |
            ((y & 0x0007c000) >> 9)  |
            ((y & 0x3e000000) >> 15) |
            ((z & 0x000001f0) << 12) |
            ((z & 0x000f8000) << 6)  |
            ((z & 0x7c000000) << 0);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = *(unsigned int *)(s2+2*BPP4);

        w = a + c;
        x = a + b + c + d;
        y = b + d;
        z = b + e + d + f;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000f8000) >> 10) |
            ((w & 0x7c000000) >> 16) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f0000) << 5)  |
            ((x & 0xf8000000) >> 1);
        *(unsigned int *)(d2+BPP4) =
            ((y & 0x000001f0) >> 4)  |
            ((y & 0x000f8000) >> 10) |
            ((y & 0x7c000000) >> 16) |
            ((z & 0x000003e0) << 11) |
            ((z & 0x001f0000) << 5)  |
            ((z & 0xf8000000) >> 1);

        /* next 2x2 input block */
        s1 += 2*BPP4; s2 += 2*BPP4;
        d1 += 4*BPP2; d2 += 4*BPP2;
        dx -= 2;
    }

    /* check if this is the last 2x2 block: */
    if (dx > 1) {

        /*
         * For last 4 output pels, repeat final input pel
         * for offscreen input.  Equivalent to pixel-doubling the
         * last output pel.
         */

        /* top line */
        a = *(unsigned int *)(s1+0);
        b = *(unsigned int *)(s1+4);
        e = b;      /* repeat last input pel */
        w = a;

        x = a + b;
        y = b;
        z = b + e;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007c000) >> 9)  |
            ((w & 0x3e000000) >> 15) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000f8000) << 6)  |
            ((x & 0x7c000000) << 0);
        *(unsigned int *)(d1+BPP4) =
            ((y & 0x000000f8) >> 3)  |
            ((y & 0x0007c000) >> 9)  |
            ((y & 0x3e000000) >> 15) |
            ((z & 0x000001f0) << 12) |
            ((z & 0x000f8000) << 6)  |
            ((z & 0x7c000000) << 0);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = d;      /* repeat last input pel */
        w = a + c;

        x = a + b + c + d;
        y = b + d;
        z = b + e + d + f;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000f8000) >> 10) |
            ((w & 0x7c000000) >> 16) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f0000) << 5)  |
            ((x & 0xf8000000) >> 1);
        *(unsigned int *)(d2+2*BPP2) =
            ((y & 0x000001f0) >> 4)  |
            ((y & 0x000f8000) >> 10) |
            ((y & 0x7c000000) >> 16) |
            ((z & 0x000003e0) << 11) |
            ((z & 0x001f0000) << 5)  |
            ((z & 0xf8000000) >> 1);

    } else {
        /* last 2x1 block: */

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = a;      /* repeat last input pel */

        w = a;
        x = a + b;

        /* pack and store */
        *(unsigned int *)d1 =
            ((w & 0x000000f8) >> 3)  |
            ((w & 0x0007c000) >> 9)  |
            ((w & 0x3e000000) >> 15) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000f8000) << 6)  |
            ((x & 0x7c000000) << 0);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = c;      /* repeat last input pel */

        w = a + c;
        x = a + b + c + d;

        /* pack and store */
        *(unsigned int *)d2 =
            ((w & 0x000001f0) >> 4)  |
            ((w & 0x000f8000) >> 10) |
            ((w & 0x7c000000) >> 16) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f0000) << 5)  |
            ((x & 0xf8000000) >> 1);
    }
}

/* RGB8 version: */
static void dblineXRGBtoRGB8x2 (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *s1, unsigned char *s2, int src_x, int dx)
{
    register unsigned int a, b, c, d, e, f;
    register unsigned int w, x, y, z;

    /* convert first 2x1 block: */
    if (dest_x & 1) {

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(d1+0) = pmap[
            ((w & 0x000000f0) >> 4)  |
            ((w & 0x00078000) >> 11) |
            ((w & 0x3c000000) >> 18)];
        *(d1+BPP1) = pmap[
            ((x & 0x000001e0) >> 5)  |
            ((x & 0x000f0000) >> 12) |
            ((x & 0x78000000) >> 19)];

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(d2+0) = pmap[
            ((w & 0x000001e0) >> 5)  |
            ((w & 0x000f0000) >> 12) |
            ((w & 0x78000000) >> 19)];
        *(d2+BPP1) = pmap[
            ((x & 0x000003c0) >> 6)  |
            ((x & 0x001e0000) >> 13) |
            ((x & 0xf0000000) >> 20)];

        /* bump pointers to next block */
        s1 += BPP4; s2 += BPP4;
        d1 += 2*BPP1; d2 += 2*BPP1;
        dx -= 1;
    }

    /* process all integral 2x2 blocks: */
    while (dx > 2) {    /* we need to leave at least one block */

        /*
         * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
         *
         * Input pels  =>   Output pels
         *  a b e           w  x  y  z
         *  c d f           w' x' y' z'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = *(unsigned int *)(s1+2*BPP4);

        w = a;
        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(d1+0) = pmap[
            ((w & 0x000000f0) >> 4)  |
            ((w & 0x00078000) >> 11) |
            ((w & 0x3c000000) >> 18)];
        *(d1+BPP1) = pmap[
            ((x & 0x000001e0) >> 5)  |
            ((x & 0x000f0000) >> 12) |
            ((x & 0x78000000) >> 19)];
        *(d1+2*BPP1) = pmap[
            ((y & 0x000000f0) >> 4)  |
            ((y & 0x00078000) >> 11) |
            ((y & 0x3c000000) >> 18)];
        *(d1+3*BPP1) = pmap[
            ((z & 0x000001e0) >> 5)  |
            ((z & 0x000f0000) >> 12) |
            ((z & 0x78000000) >> 19)];

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = *(unsigned int *)(s2+2*BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(d2+0) = pmap[
            ((w & 0x000001e0) >> 5)  |
            ((w & 0x000f0000) >> 12) |
            ((w & 0x78000000) >> 19)];
        *(d2+BPP1) = pmap[
            ((x & 0x000003c0) >> 6)  |
            ((x & 0x001e0000) >> 13) |
            ((x & 0xf0000000) >> 20)];
        *(d2+2*BPP1) = pmap[
            ((y & 0x000001e0) >> 5)  |
            ((y & 0x000f0000) >> 12) |
            ((y & 0x78000000) >> 19)];
        *(d2+3*BPP1) = pmap[
            ((z & 0x000003c0) >> 6)  |
            ((z & 0x001e0000) >> 13) |
            ((z & 0xf0000000) >> 20)];

        /* bump pointers to next 2x2 input */
        s1 += 2*BPP4; s2 += 2*BPP4;
        d1 += 4*BPP1; d2 += 4*BPP1;
        dx -= 2;
    }

    /* check if this is the last 2x2 block: */
    if (dx > 1) {

        /*
         * For last 4 output pels, repeat final input pel
         * for offscreen input.  Equivalent to pixel-doubling the
         * last output pel.
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = *(unsigned int *)(s1+BPP4);
        e = b;      /* repeat last input pel */
        w = a;

        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(d1+0) = pmap[
            ((w & 0x000000f0) >> 4)  |
            ((w & 0x00078000) >> 11) |
            ((w & 0x3c000000) >> 18)];
        *(d1+BPP1) = pmap[
            ((x & 0x000001e0) >> 5)  |
            ((x & 0x000f0000) >> 12) |
            ((x & 0x78000000) >> 19)];
        *(d1+2*BPP1) = pmap[
            ((y & 0x000000f0) >> 4)  |
            ((y & 0x00078000) >> 11) |
            ((y & 0x3c000000) >> 18)];
        *(d1+3*BPP1) = pmap[
            ((z & 0x000001e0) >> 5)  |
            ((z & 0x000f0000) >> 12) |
            ((z & 0x78000000) >> 19)];

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = d;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(d2+0) = pmap[
            ((w & 0x000001e0) >> 5)  |
            ((w & 0x000f0000) >> 12) |
            ((w & 0x78000000) >> 19)];
        *(d2+BPP1) = pmap[
            ((x & 0x000003c0) >> 6)  |
            ((x & 0x001e0000) >> 13) |
            ((x & 0xf0000000) >> 20)];
        *(d2+2*BPP1) = pmap[
            ((y & 0x000001e0) >> 5)  |
            ((y & 0x000f0000) >> 12) |
            ((y & 0x78000000) >> 19)];
        *(d2+3*BPP1) = pmap[
            ((z & 0x000003c0) >> 6)  |
            ((z & 0x001e0000) >> 13) |
            ((z & 0xf0000000) >> 20)];

    } else {
        /* last 2x1 block: */

        /* Input pels  =>   Output pels
         *  a b             w  x
         *  c d             w' x'
         */

        /* top line */
        a = *(unsigned int *)s1;
        b = a;      /* repeat last input pel */

        w = a;
        x = a + b + ROUND888;

        /* pack and store */
        *(d1+0) = pmap[
            ((w & 0x000000f0) >> 4)  |
            ((w & 0x00078000) >> 11) |
            ((w & 0x3c000000) >> 18)];
        *(d1+BPP1) = pmap[
            ((x & 0x000001e0) >> 5)  |
            ((x & 0x000f0000) >> 12) |
            ((x & 0x78000000) >> 19)];

        /* bottom line */
        c = *(unsigned int *)s2;
        d = c;      /* repeat last input pel */

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);

        /* pack and store */
        *(d2+0) = pmap[
            ((w & 0x000001e0) >> 5)  |
            ((w & 0x000f0000) >> 12) |
            ((w & 0x78000000) >> 19)];
        *(d2+BPP1) = pmap[
            ((x & 0x000003c0) >> 6)  |
            ((x & 0x001e0000) >> 13) |
            ((x & 0xf0000000) >> 20)];
    }
}


/* color converter tables: */
static void (* cc []) (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su,
    unsigned char *sv, int src_x, int dx) = {
    dblineI420toRGB32, dblineI420toRGB24,
    dblineI420toRGB565, dblineI420toRGB555,
    dblineI420toRGB8, dblineI420toBGR32
};
static void (* ccalpha []) (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su,
    unsigned char *sv, int src_x, int dx) = {
    dblineI420toRGB32alpha, dblineI420toRGB24alpha,
    dblineI420toRGB565alpha, dblineI420toRGB555alpha,
    dblineI420toRGB8alpha, dblineI420toBGR32alpha
};
static void (* ccx2 []) (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *s1, unsigned char *s2, int src_x, int dx) = {
    dblineXRGBtoRGB32x2, dblineXRGBtoRGB24x2,
    dblineXRGBtoRGB565x2, dblineXRGBtoRGB555x2,
    dblineXRGBtoRGB8x2, dblineXRGBtoBGR32x2
};
static int bpp [] = {4, 3, 2, 2, 1, 4};

/* RGB linebuffers */
static int next[3] = {1, 2, 0};                 /* (ibuf+1)%3 table */
static int next2[3] = {2, 0, 1};                /* (ibuf+2)%3 table */
#define MAXWIDTH 640                            /* max input width */
static unsigned char linebuf [3*MAXWIDTH*BPP4]; /* space for 3 RGB linebuffers */ /* Flawfinder: ignore */

/*
 * I420->RGB* driver:
 */
static int I420toRGB (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy, int ccidx)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (src_pitch <= 0) return -1;          /* not supported */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* color converter to use: */
        void (*dbline) (unsigned char *d1, unsigned char *d2, int dest_x,
            unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
            int src_x, int dx) = is_alpha? ccalpha [ccidx]: cc [ccidx];

        /* local variables: */
        unsigned char *sy1, *sy2, *su, *sv, *d1, *d2;
        register int j;

        /* get pointers: */
        sy1 = src_ptr + (src_x + src_y * src_pitch);        /* luma offset */
        sy2 = sy1 + src_pitch;
        su  = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
        sv  = su + src_height * src_pitch / 4;
        d1  = dest_ptr + dest_x * bpp [ccidx] + dest_y * dest_pitch; /* RGB offset */
        d2  = d1 + dest_pitch;

        /* convert a top line: */
        if (src_y & 1) {                        /* chroma shift */

            /* this is a bit inefficient, but who cares??? */
            (* dbline) (d1, d1, dest_x, sy1, sy1, su, sv, src_x, src_dx);

            sy1 += src_pitch;   sy2 += src_pitch;
            su  += src_pitch/2; sv  += src_pitch/2;
            d1  += dest_pitch;  d2  += dest_pitch;
            dest_dy --;
        }

        /* convert aligned portion of the image: */
        for (j = 0; j < dest_dy/2; j ++) {

            /* convert two lines a time: */
            (* dbline) (d1, d2, dest_x, sy1, sy2, su, sv, src_x, src_dx);

            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;
            d1  += dest_pitch*2; d2 += dest_pitch*2;
        }

        /* convert bottom line (if dest_dy is odd): */
        if (dest_dy & 1) {

            /* convert one line only: */
            (* dbline) (d1, d1, dest_x, sy1, sy1, su, sv, src_x, src_dx);
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* color converter & interpolator to use: */
        void (*cvt) (unsigned char *d1, unsigned char *d2, int dest_x,
            unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
            int src_x, int dx) = is_alpha? dblineI420toXRGBalpha: dblineI420toXRGB;
        void (*x2) (unsigned char *d1, unsigned char *d2, int dest_x,
            unsigned char *s1, unsigned char *s2, int src_x, int dx) = ccx2 [ccidx];

        /* local variables: */
        unsigned char *sy1, *sy2, *su, *sv, *d1, *d2;
        register dy = src_dy;

        /* line buffers (we want them to be as compact as possible): */
        int ibuf = 0;                           /* circular buffer index */
        unsigned char * buf[3];                 /* actual pointers  */ /* Flawfinder: ignore */
        buf[0] = linebuf;
        buf[1] = linebuf + src_dx * BPP4;
        buf[2] = linebuf + 2 * src_dx * BPP4;

        /* get pointers: */
        sy1 = src_ptr + (src_x + src_y * src_pitch);        /* luma offset */
        sy2 = sy1 + src_pitch;
        su  = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
        sv  = su + src_height * src_pitch / 4;
        d1  = dest_ptr + dest_x * bpp[ccidx] + dest_y * dest_pitch; /* RGB offset */
        d2  = d1 + dest_pitch;

        /* check if we have misaligned top line: */
        if (src_y & 1) {

            /* convert an odd first line: */
            (*cvt) (buf[ibuf], buf[ibuf], 0, sy1, sy1, su, sv, src_x, src_dx);
            sy1 += src_pitch;   sy2 += src_pitch;
            su  += src_pitch/2; sv  += src_pitch/2;
            dy --;

        } else {

            /* otherwise, convert first two lines: */
            (*cvt) (buf[next[ibuf]], buf[next2[ibuf]], 0, sy1, sy2, su, sv, src_x, src_dx);
            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;
            ibuf = next[ibuf];      /* skip first interpolation: */

            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx);
            d1  += dest_pitch*2; d2  += dest_pitch*2;
            ibuf = next[ibuf];
            dy -= 2;
        }

        /*
         * Convert & interpolate the main portion of image:
         *
         *  source:      temp.store:                destination:
         *
         *               buf[ibuf] -------\    /--> d1
         *                                  x2  --> d2
         *  s1 --\   /-> buf[next[ibuf]] -<    /--> d1'=d1+2*pitch
         *        cvt                       x2 ---> d2'=d2+2*pitch
         *  s2 --/   \-> buf[next2[ibuf]] /
         */
        while (dy >= 2) {

            /* convert two lines into XRGB buffers: */
            (*cvt) (buf[next[ibuf]], buf[next2[ibuf]], 0, sy1, sy2, su, sv, src_x, src_dx);
            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;

            /* interpolate first line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx);
            d1  += dest_pitch*2; d2  += dest_pitch*2;
            ibuf = next[ibuf];

            /* interpolate second one: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx);
            d1  += dest_pitch*2; d2  += dest_pitch*2;
            ibuf = next[ibuf];
            dy -= 2;
        }

        /* check the # of remaining rows: */
        if (dy & 1) {

            /* convert the last odd line: */
            (*cvt) (buf[next[ibuf]], buf[next[ibuf]], 0, sy1, sy1, su, sv, src_x, src_dx);

            /* interpolate first line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx);
            d1  += dest_pitch*2; d2  += dest_pitch*2;
            ibuf = next[ibuf];

            /* replicate the last line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[ibuf], 0, src_dx);

        } else {

            /* replicate the last line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[ibuf], 0, src_dx);
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * I420->RGB* converters:
 */
int I420toRGB32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toRGB (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, 0);
}

int I420toRGB24 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toRGB (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, 1);
}

int I420toRGB565 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toRGB (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, 2);
}

int I420toRGB555 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toRGB (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, 3);
}

int I420toRGB8 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toRGB (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, 4);
}

int I420toBGR32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toRGB (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, 5);
}


/*
 * "to I420" converters.
 *
 */

int YV12toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have matching chroma components: */
    if ((src_x & 1) || (src_y & 1))
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0 || src_pitch <= 0)
        return -1;                          /* not supported */
/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
        unsigned char *s, *d;
        int src_uv_offs, dest_uv_offs;
        register int i;

        /* copy Y plane: */
        s = src_ptr + src_x + src_y * src_pitch;
        d = dest_ptr + dest_x + dest_y * dest_pitch;
        for (i = 0; i < dest_dy; i ++) {
            memcpy (d, s, dest_dx); /* Flawfinder: ignore */
            s += src_pitch;
            d += dest_pitch;
        }

        /* get Cr/Cb offsets: */
        src_uv_offs = src_height * src_pitch / 4;
        dest_uv_offs = dest_height * dest_pitch / 4;

        /* copy & flip Cr/Cb planes: */
        s = (src_ptr + src_height * src_pitch) + src_x/2 + src_y/2 * src_pitch/2;
        d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;
        for (i = 0; i < dest_dy/2; i ++) {
            memcpy (d, s + src_uv_offs, dest_dx/2); /* Flawfinder: ignore */
            memcpy (d + dest_uv_offs, s, dest_dx/2); /* Flawfinder: ignore */
            s += src_pitch/2;
            d += dest_pitch/2;
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

static void lineYVU9toI420 (unsigned char *d, unsigned char *s, int x, int dx)
{
    register int i;

    /* first pixel: */
    if (x & 2) {
        d[0] = s[0];
        s += 1;
        d += 1;
        dx -= 2;
    }
    /* the main loop: */
    for (i = 0; i < dx/4; i ++) {
        d[i*2] = s[i];
        d[i*2+1] = s[i];
    }
    /* last pixel: */
    if (dx & 2)
        d[dx/2] = s[dx/4];
}

int YVU9toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have matching chroma components: */
    if ((src_x & 1) || (src_y & 1))
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0 || src_pitch <= 0)
        return -1;                          /* not supported */
/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* no color adjustments: */
        unsigned char *s, *d;
        int src_uv_offs, dest_uv_offs;
        register int j;

        /* copy Y plane: */
        s = src_ptr + src_x + src_y * src_pitch;
        d = dest_ptr + dest_x + dest_y * dest_pitch;
        for (j = 0; j < dest_dy; j ++) {
            memcpy (d, s, dest_dx); /* Flawfinder: ignore */
            s += src_pitch;
            d += dest_pitch;
        }

        /* get Cr/Cb offsets: */
        src_uv_offs = src_height * src_pitch / 16;
        dest_uv_offs = dest_height * dest_pitch / 4;

        /* get pointers: */
        s = (src_ptr + src_height * src_pitch) + src_x/4 + src_y/4 * src_pitch/4;
        d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;

        /* top lines: */
        if (src_y & 2) {
            lineYVU9toI420 (d, s, src_x, dest_dx);   /* Cr */
            lineYVU9toI420 (d + dest_uv_offs, s + src_uv_offs, src_x, dest_dx);  /* Cb */
            s += src_pitch/4;
            d += dest_pitch/2;
            dest_dy -= 2;
        }
        /* the main loop (processes two lines a time): */
        for (j = 0; j < dest_dy/4; j ++) {
            lineYVU9toI420 (d, s, src_x, dest_dx);   /* Cr */
            memcpy (d + dest_pitch/2, d, dest_dx/2); /* Flawfinder: ignore */
            lineYVU9toI420 (d + dest_uv_offs, s + src_uv_offs, src_x, dest_dx);  /* Cb */
            memcpy (d + dest_pitch/2 + dest_uv_offs, d + dest_uv_offs, dest_dx/2); /* Flawfinder: ignore */
            s += src_pitch/4;
            d += dest_pitch;
        }
        /* bottom lines: */
        if (dest_dy & 2) {
            lineYVU9toI420 (d, s, src_x, dest_dx);   /* Cr */
            lineYVU9toI420 (d + dest_uv_offs, s + src_uv_offs, src_x, dest_dx);  /* Cb */
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

int YUY2toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have misaligned source: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0) return -1;     /* not supported */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;

/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
        unsigned char *s1, *s2, *d1, *d2, *dv, *du;
        register int i, j;

        /* get pointers: */
        s1 = src_ptr + src_x * 2 + src_y * src_pitch;    /* 2 bytes/pixel */
        s2 = s1 + src_pitch * 2;
        d1 = dest_ptr + dest_x + dest_y * dest_pitch;   /* luma offsets  */
        d2 = d1 + dest_pitch;
        du = dest_ptr + dest_height * dest_pitch + (dest_x/2 + dest_y/2 * dest_pitch/2);
        dv = du + dest_height * dest_pitch/4;

        /* the main loop (processes lines a time): */
        for (i = 0; i < dest_dy/2; i ++) {

            /* copy 2x2 pixels: */
            for (j = 0; j < dest_dx/2; j ++) {

                /* copy luma components: */
                d1[j*2]   = s1[j*4];
                d1[j*2+1] = s1[j*4+2];
                d2[j*2]   = s2[j*4];
                d2[j*2+1] = s2[j*4+2];

                /* average chromas: */
                du[j] = (s1[j*4+1] + s2[j*4+1]) / 2;
                dv[j] = (s1[j*4+3] + s2[j*4+3]) / 2;
            }

            s1 += src_pitch*2;  s2 += src_pitch*2;
            d1 += dest_pitch*2; d2 += dest_pitch*2;
            du += dest_pitch/2; dv += dest_pitch/2;
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

int UYVYtoI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have misaligned source: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0) return -1;     /* not supported */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;

/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
        unsigned char *s1, *s2, *d1, *d2, *dv, *du;
        register int i, j;

        /* get pointers: */
        s1 = src_ptr + src_x * 2 + src_y * src_pitch;    /* 2 bytes/pixel */
        s2 = s1 + src_pitch * 2;
        d1 = dest_ptr + dest_x + dest_y * dest_pitch;   /* luma offsets  */
        d2 = d1 + dest_pitch;
        du = dest_ptr + dest_height * dest_pitch + (dest_x/2 + dest_y/2 * dest_pitch/2);
        dv = du + dest_height * dest_pitch/4;

        /* the main loop (processes lines a time): */
        for (i = 0; i < dest_dy/2; i ++) {

            /* copy 2x2 pixels: */
            for (j = 0; j < dest_dx/2; j ++) {

                /* copy luma components: */
                d1[j*2]   = s1[j*4+1];
                d1[j*2+1] = s1[j*4+3];
                d2[j*2]   = s2[j*4+1];
                d2[j*2+1] = s2[j*4+3];

                /* average chromas: */
                du[j] = (s1[j*4]   + s2[j*4])   / 2;
                dv[j] = (s1[j*4+2] + s2[j*4+2]) / 2;
            }

            s1 += src_pitch*2;  s2 += src_pitch*2;
            d1 += dest_pitch*2; d2 += dest_pitch*2;
            du += dest_pitch/2; dv += dest_pitch/2;
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

int RGB32toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB24toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* align destination rectangle: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch <= 0) return -1;     /* not supported */

/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
        unsigned char *s1, *s2, *d1, *d2, *dv, *du;
        register int i, j;

        /* get pointers: */
        s1 = src_ptr + src_x * BPP3 + src_y * src_pitch; /* 3 bytes/pixel */
        s2 = s1 + src_pitch;
        d1 = dest_ptr + dest_x + dest_y * dest_pitch;    /* luma offsets  */
        d2 = d1 + dest_pitch;
        du = dest_ptr + dest_height * dest_pitch + (dest_x/2 + dest_y/2 * dest_pitch/2); /* chroma offset */
        dv = du + dest_height * dest_pitch/4;

        /* the main loop (processes 2 lines a time): */
        for (i = 0; i < dest_dy/2; i ++) {

            /* convert 2x2 block: */
            for (j = 0; j < dest_dx/2; j ++) {

                int r4, b4, y4;

                /* process lumas: */
                {
                    register unsigned int r, b, y;

                    r4 = (r = s1[2]);       /* BGR */
                    b4 = (b = s1[0]);
                    y4 = (y = yrtab [r] + ygtab [s1[1]] + ybtab [b]);
                    d1[0] = yytab [y];

                    r4 += (r = s1[5]);      /* bgrBGR */
                    b4 += (b = s1[3]);
                    y4 += (y = yrtab [r] + ygtab [s1[4]] + ybtab [b]);
                    d1[1] = yytab [y];

                    r4 += (r = s2[2]);      /* BGR */
                    b4 += (b = s2[0]);
                    y4 += (y = yrtab [r] + ygtab [s2[1]] + ybtab [b]);
                    d2[0] = yytab [y];

                    r4 += (r = s2[5]);      /* bgrBGR */
                    b4 += (b = s2[3]);
                    y4 += (y = yrtab [r] + ygtab [s2[4]] + ybtab [b]);
                    d2[1] = yytab [y];
                }

                /* average chromas: */
                dv[0] = vrytab [VMIN+(r4-y4)/4];
                du[0] = ubytab [UMIN+(b4-y4)/4];

                /* go to the next block: */
                s1 += 2 * BPP3; s2 += 2 * BPP3;
                d1 += 2; d2 += 2;
                du += 1; dv += 1;
            }

            /* switch to the next two lines: */
            s1 += src_pitch * 2 - dest_dx * BPP3;
            s2 += src_pitch * 2 - dest_dx * BPP3;
            d1 += dest_pitch * 2 - dest_dx;
            d2 += dest_pitch * 2 - dest_dx;
            du += (dest_pitch - dest_dx)/2;
            dv += (dest_pitch - dest_dx)/2;
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

int RGB565toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB555toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB8toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}


/*
 * "RGBx to BGR32" converters.
 *
 */

int RGB32toBGR32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP4 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert image: */
        for (i = 0; i < dest_dy; i ++) {

            /* convert a line: */
            register int ddx = dest_dx;
            while (ddx) {
                register unsigned int a;
                a = *(unsigned int *)s;     /* 0x00RRGGBB */
                *(unsigned int *)d =
                    ((a & 0x000000ff) << 16) |
                    ((a & 0x0000ff00) << 0)  |
                    ((a & 0x00ff0000) >> 16);
                s += BPP4;
                d += BPP4;
                ddx --;
            }
            s += src_pitch - dest_dx * BPP4;
            d += dest_pitch - dest_dx * BPP4;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP4 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert & stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* convert & stretch a line: */
            register int sdx = src_dx;
            while (sdx) {
                register unsigned int a, c;
                a = *(unsigned int *)s;
                c = ((a & 0x000000ff) << 16) |
                    ((a & 0x0000ff00) << 0)  |
                    ((a & 0x00ff0000) >> 16);
                *(unsigned int *)d = c;
                *(unsigned int *)(d+BPP4) = c;
                d += 2*BPP4; s += BPP4;
                sdx --;
            }
            s -= src_dx * BPP4;
            d -= src_dx * 2*BPP4;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP4); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

int RGB24toBGR32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert image: */
        for (i = 0; i < dest_dy; i ++) {

            /* convert a line: */
            register int ddx = dest_dx;
            while (ddx) {
                /* RGB -> 0BGR: */
                *(unsigned int *)d = (unsigned int)
                    (s[0] << 16) | (s[1] << 8) | s[2];
                d += BPP4; s += BPP3;
                ddx --;
            }
            s += src_pitch - dest_dx * BPP3;
            d += dest_pitch - dest_dx * BPP4;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert & stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* convert & stretch a line: */
            register int sdx = src_dx;
            while (sdx) {
                /* RGB -> 0BGR: */
                register unsigned int a;
                a = (unsigned int)
                    (s[0] << 16) | (s[1] << 8) | s[2];
                *(unsigned int *)d = a;
                *(unsigned int *)(d+BPP4) = a;
                d += 2*BPP4; s += BPP3;
                sdx --;
            }
            s -= src_dx * BPP3;
            d -= src_dx * 2*BPP4;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP4); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

int RGB565toBGR32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert image: */
        for (i = 0; i < dest_dy; i ++) {

            /* convert a line: */
            register int ddx = dest_dx;
            while (ddx) {
                /* rrrr,rggg,gggb,bbbb -> 0BGR: */
                register unsigned short a;
                a = *(unsigned short *)s;
                *(unsigned int *)d = (unsigned int)
                    ((a & 0x001F) << 19)|
                    ((a & 0x07E0) << 5) |
                    ((a & 0xF800) >> 8);
                d += BPP4; s += BPP2;
                ddx --;
            }
            s += src_pitch - dest_dx * BPP2;
            d += dest_pitch - dest_dx * BPP4;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert & stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* convert & stretch a line: */
            register int sdx = src_dx;
            while (sdx) {
                register unsigned short a;
                register unsigned int c;
                a = *(unsigned short *)s;
                c = ((a & 0x001F) << 19)|
                    ((a & 0x07E0) << 5) |
                    ((a & 0xF800) >> 8);
                *(unsigned int *)d = c;
                *(unsigned int *)(d+BPP4) = c;
                d += 2*BPP4; s += BPP2;
                sdx --;
            }
            s -= src_dx * BPP2;
            d -= src_dx * 2*BPP4;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP4); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

int RGB555toBGR32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert image: */
        for (i = 0; i < dest_dy; i ++) {

            /* convert a line: */
            register int ddx = dest_dx;
            while (ddx) {
                /* 0rrrr,rgg,gggb,bbbb -> 0BGR: */
                register unsigned short a;
                a = *(unsigned short *)s;
                *(unsigned int *)d = (unsigned int)
                    ((a & 0x001F) << 19)|
                    ((a & 0x03E0) << 6) |
                    ((a & 0x7C00) >> 7);
                d += BPP4; s += BPP2;
                ddx --;
            }
            s += src_pitch - dest_dx * BPP2;
            d += dest_pitch - dest_dx * BPP4;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* convert & stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* convert & stretch a line: */
            register int sdx = src_dx;
            while (sdx) {
                register unsigned short a;
                register unsigned int c;
                a = *(unsigned short *)s;
                c = ((a & 0x001F) << 19)|
                    ((a & 0x03E0) << 6) |
                    ((a & 0x7C00) >> 7);
                *(unsigned int *)d = c;
                *(unsigned int *)(d+BPP4) = c;
                d += 2*BPP4; s += BPP2;
                sdx --;
            }
            s -= src_dx * BPP2;
            d -= src_dx * 2*BPP4;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP4); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

int RGB8toBGR32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}


/********************************
 *
 * "RGBx to RGBy" converters.
 *
 ********************************/

/*
 * RGB32toRGB32() converter:
 *  1:1, 2:1
 */
int RGB32toRGB32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP4 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* copy image: */
        for (i = 0; i < dest_dy; i ++) {
            memcpy (d, s, dest_dx * BPP4);  /* copy dest_dx pixels */ /* Flawfinder: ignore */
            s += src_pitch;
            d += dest_pitch;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP4 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sdx = src_dx;
            while (sdx) {
                register unsigned int a;
                a = *(unsigned int *)s;
                *(unsigned int *)d = a;
                *(unsigned int *)(d+BPP4) = a;
                d += 2*BPP4; s += BPP4;
                sdx --;
            }
            s -= src_dx * BPP4;
            d -= src_dx * 2*BPP4;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP4); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

int RGB32toRGB24 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB32toRGB565 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB32toRGB555 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

/*
 * No dithering yet.
 */
int RGB32toRGB8 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP4 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            /* convert a line: */
            register int ddx = dest_dx;
            while (ddx) {
                register unsigned int a;
                a = *(unsigned int *)s;             /* BGR0 */
                *d = pmap[
                    ((a & 0x000000f0) >>  4) |
                    ((a & 0x0000f000) >>  8) |
                    ((a & 0x00f00000) >> 12)];
                d += BPP1; s += BPP4;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP4;
            d += dest_pitch - dest_dx * BPP1;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP4 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sdx = src_dx;
            while (sdx) {
                register unsigned int a;
                register unsigned char c;
                a = *(unsigned int *)s;             /* BRG0 */
                c = pmap[
                    ((a & 0x000000f0) >>  4) |
                    ((a & 0x0000f000) >>  8) |
                    ((a & 0x00f00000) >> 12)];
                *(d+0) = c;
                *(d+BPP1) = c;
                d += 2*BPP1; s += BPP4;
                sdx --;
            }
            s -= src_dx * BPP4;
            d -= src_dx * 2*BPP1;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP1); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * RGB24toRGB32() converter:
 *  1:1, 2:1
 */
int RGB24toRGB32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            register int sx = src_x, ddx = dest_dx;

            /* process misaligned pixels first: */
            while ((sx & 3) && ddx) {
                /* BGR -> BGR0: */
                *(unsigned int *)d = (unsigned int)s[0] | (s[1] << 8) | (s[2] << 16);
                d += BPP4; s += BPP3;
                sx ++; ddx --;
            }

            /* main loop: process 4 pixels a time: */
            while (ddx >= 4) {
                register unsigned int a, b;
                a = *(unsigned int *)s;             /* BGR.B */
                b = *(unsigned int *)(s+4);         /* GR.BG */
                *(unsigned int *)d      = a & 0x00FFFFFF;
                *(unsigned int *)(d+4)  = ((a >> 24) | (b << 8)) & 0x00FFFFFF;
                a = *(unsigned int *)(s+8);         /* R.BGR */
                *(unsigned int *)(d+8)  = ((b >> 16) | (a << 16)) & 0x00FFFFFF;
                *(unsigned int *)(d+12) = a >> 8;
                d += BPP4*4; s += BPP3*4;
                ddx -= 4;
            }

            /* process the remaining 1..3 pixels: */
            while (ddx) {
                /* RGB -> RGB0: */
                *(unsigned int *)d = (unsigned int)s[0] | (s[1] << 8) | (s[2] << 16);
                d += BPP4; s += BPP3;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP3;
            d += dest_pitch - dest_dx * BPP4;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP4 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sx = src_x, sdx = src_dx;

            /* misaligned pixels first: */
            while ((sx & 3) && sdx) {
                /* BGR -> BGR0: */
                register unsigned int a;
                a = (unsigned int)s[0] | (s[1] << 8) | (s[2] << 16);
                *(unsigned int *)d = a;
                *(unsigned int *)(d+BPP4) = a;
                d += 2*BPP4; s += BPP3;
                sx ++; sdx --;
            }
            /* main bulk of data: */
            while (sdx >= 4) {
                register unsigned int a, b, c;
                a = *(unsigned int *)s;             /* bgrB */
                b = *(unsigned int *)(s+4);         /* GRbg */
                c = a & 0x00FFFFFF;
                *(unsigned int *)(d) = c;
                *(unsigned int *)(d+BPP4) = c;
                c = ((a >> 24) | (b << 8)) & 0x00FFFFFF;
                *(unsigned int *)(d+2*BPP4) = c;
                *(unsigned int *)(d+3*BPP4) = c;
                a = *(unsigned int *)(s+8);         /* rBGR */
                c = ((b >> 16) | (a << 16)) & 0x00FFFFFF;
                *(unsigned int *)(d+4*BPP4) = c;
                *(unsigned int *)(d+5*BPP4) = c;
                c = a >> 8;
                *(unsigned int *)(d+6*BPP4) = c;
                *(unsigned int *)(d+7*BPP4) = c;
                d += 4*2*BPP4; s += 4*BPP3;
                sdx -= 4;
            }
            /* the remaining 1..3 pixels: */
            while (sdx) {
                /* BGR -> BGR0: */
                register unsigned int a;
                a = (unsigned int)s[0] | (s[1] << 8) | (s[2] << 16);
                *(unsigned int *)d = a;
                *(unsigned int *)(d+BPP4) = a;
                d += 2*BPP4; s += BPP3;
                sdx --;
            }
            s -= src_dx * BPP3;
            d -= src_dx * 2*BPP4;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP4); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * RGB24toRGB24() converter:
 *  1:1, 2:1
 */
int RGB24toRGB24 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP3 + dest_y * dest_pitch;

        /* copy image: */
        for (i = 0; i < dest_dy; i ++) {
            memcpy (d, s, dest_dx * BPP3);  /* copy dest_dx pixels */ /* Flawfinder: ignore */
            s += src_pitch;
            d += dest_pitch;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP3 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sx = src_x, sdx = src_dx;

            /* misaligned pixels first: */
            while ((sx & 3) && sdx) {
                register unsigned char a;
                a = *s; *d = a; *(d+BPP3) = a;
                a = *(s+1); *(d+1) = a; *(d+1+BPP3) = a;
                a = *(s+2); *(d+2) = a; *(d+2+BPP3) = a;
                d += 2*BPP3; s += BPP3;
                sx ++; sdx --;
            }
            /* main bulk of data: */
            while (sdx >= 4) {
                register unsigned int a, b;
                a = *(unsigned int *)s;             /* bgrB */
                b = *(unsigned int *)(s+4);         /* GRbg */
                *(unsigned int *)(d) = (a & 0x00FFFFFF) | (a << 24);    /* bgrb */
                *(unsigned int *)(d+4) = (a >> 8) | (b << 24);          /* grBG */
                *(unsigned int *)(d+2*4) = ((b & 0xFF00) >> 8) |        /* RBGR */
                                           ((a & 0xFF000000) >> 16) | (b << 16);
                a = *(unsigned int *)(s+8);         /* rBGR */
                *(unsigned int *)(d+3*4) = (b >> 16) |                  /* bgrb */
                                           ((a & 0xFF) << 16) | ((b & 0xFF0000) << 8);
                *(unsigned int *)(d+4*4) = (b >> 24) | (a << 8);        /* grBG */
                *(unsigned int *)(d+5*4) = (a >> 24) | (a & 0xFFFFFF00);/* RBGR */
                d += 4*2*BPP3; s += 4*BPP3;
                sdx -= 4;
            }
            /* the remaining 1..3 pixels: */
            while (sdx) {
                register unsigned char a;
                a = *s; *d = a; *(d+BPP3) = a;
                a = *(s+1); *(d+1) = a; *(d+1+BPP3) = a;
                a = *(s+2); *(d+2) = a; *(d+2+BPP3) = a;
                d += 2*BPP3; s += BPP3;
                sdx --;
            }
            s -= src_dx * BPP3;
            d -= src_dx * 2*BPP3;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP3); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * RGB24toRGB565() converter:
 *  1:1, 2:1
 */
int RGB24toRGB565 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            register int sx = src_x, ddx = dest_dx;

            /* process misaligned pixels first: */
            while ((sx & 3) && ddx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.rrrrrggg: */
                *(unsigned short *)d =
                    (s[0] >> 3) | ((s[1] & 0xFC) << 3) | ((s[2] & 0xF8) << 8);
                d += BPP2;  s += BPP3;
                sx ++; ddx --;
            }

            /* main loop: process 4 pixels a time: */
            while (ddx >= 4) {

                register unsigned int a, b;

                a = *(unsigned int *)s;             /* BGR.B */
                b = *(unsigned int *)(s+4);         /* GR.BG */
                *(unsigned int *)d =
                    ((a & 0xF8) >> 3) | ((a & 0xFC00) >> 5) | ((a & 0xF80000) >> 8) |
                    ((a & 0xF8000000) >> 11) | ((b & 0xFC) << 19) | ((b & 0xF800) << 16);
                a = *(unsigned int *)(s+8);         /* R.BGR */
                *(unsigned int *)(d+4) =
                    ((b & 0xF80000) >> 19) | ((b & 0xFC000000) >> 21) | ((a & 0xF8) << 8) |
                    ((a & 0xF800) << 5) | ((a & 0xFC0000) << 3) | (a & 0xF8000000);

                d += BPP2*4; s += BPP3*4; ddx -= 4;
            }

            /* process the remaining 1..3 pixels: */
            while (ddx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.rrrrrggg: */
                *(unsigned short *)d =
                    (s[0] >> 3) | ((s[1] & 0xFC) << 3) | ((s[2] & 0xF8) << 8);
                d += BPP2;  s += BPP3;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP3;
            d += dest_pitch - dest_dx * BPP2;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sx = src_x, sdx = src_dx;

            /* misaligned pixels first: */
            while ((sx & 3) && sdx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.0rrrrrgg: */
                register unsigned short a;
                a = (s[0] >> 3) | ((s[1] & 0xFC) << 3) | ((s[2] & 0xF8) << 8);
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2;  s += BPP3;
                sx ++; sdx --;
            }

            /* main bulk of data: */
            while (sdx >= 4) {
                register unsigned int a, b, c;
                a = *(unsigned int *)s;             /* bgrB */
                b = *(unsigned int *)(s+4);         /* GRbg */
                c = ((a & 0xF8) >> 3) | ((a & 0xFC00) >> 5) | ((a & 0xF80000) >> 8);
                *(unsigned int *)d = c | (c << 16);
                c = ((a & 0xF8000000) >> 11) | ((b & 0xFC) << 19) | ((b & 0xF800) << 16);
                *(unsigned int *)(d+2*BPP2) = c | (c >> 16);
                a = *(unsigned int *)(s+8);         /* rBGR */
                c = ((b & 0xF80000) >> 19) | ((b & 0xFC000000) >> 21) | ((a & 0xF8) << 8);
                *(unsigned int *)(d+2*2*BPP2) = c | (c << 16);
                c = ((a & 0xF800) << 5) | ((a & 0xFC0000) << 3) | (a & 0xF8000000);
                *(unsigned int *)(d+3*2*BPP2) = c | (c >> 16);
                d += 2*4*BPP2; s += 4*BPP3;
                sdx -= 4;
            }

            /* the remaining pixels: */
            while (sdx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.0rrrrrgg: */
                register unsigned short a;
                a = (s[0] >> 3) | ((s[1] & 0xFC) << 3) | ((s[2] & 0xF8) << 8);
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2;  s += BPP3;
                sdx --;
            }
            s -= src_dx * BPP3;
            d -= src_dx * 2*BPP2;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP2); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * RGB24toRGB555() converter:
 *  1:1, 2:1
 */
int RGB24toRGB555 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            register int sx = src_x, ddx = dest_dx;

            /* process misaligned pixels first: */
            while ((sx & 3) && ddx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.0rrrrrgg: */
                *(unsigned short *)d =
                    (s[0] >> 3) | ((s[1] & 0xF8) << 2) | ((s[2] & 0xF8) << 7);
                d += BPP2;  s += BPP3;
                sx ++; ddx --;
            }

            /* main loop: process 4 pixels a time: */
            while (ddx >= 4) {
                register unsigned int a, b;
                a = *(unsigned int *)s;             /* bgrB */
                b = *(unsigned int *)(s+4);         /* GRbg */
                *(unsigned int *)d =
                    ((a & 0xF8) >> 3) | ((a & 0xF800) >> 6) | ((a & 0xF80000) >> 9) |
                    ((a & 0xF8000000) >> 11) | ((b & 0xF8) << 18) | ((b & 0xF800) << 15);
                a = *(unsigned int *)(s+8);         /* rBGR */
                *(unsigned int *)(d+4) =
                    ((b & 0xF80000) >> 19) | ((b & 0xF8000000) >> 22) | ((a & 0xF8) << 7) |
                    ((a & 0xF800) << 5) | ((a & 0xF80000) << 2) | ((a & 0xF8000000) >> 1);
                d += BPP2*4; s += BPP3*4; ddx -= 4;
            }

            /* process the remaining 1..3 pixels: */
            while (ddx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.rrrrrggg: */
                *(unsigned short *)d =
                    (s[0] >> 3) | ((s[1] & 0xF8) << 2) | ((s[2] & 0xF8) << 7);
                d += BPP2;  s += BPP3;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP3;
            d += dest_pitch - dest_dx * BPP2;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sx = src_x, sdx = src_dx;

            /* misaligned pixels first: */
            while ((sx & 3) && sdx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.0rrrrrgg: */
                register unsigned short a;
                a = (s[0] >> 3) | ((s[1] & 0xF8) << 2) | ((s[2] & 0xF8) << 7);
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2;  s += BPP3;
                sx ++; sdx --;
            }

            /* main bulk of data: */
            while (sdx >= 4) {
                register unsigned int a, b, c;
                a = *(unsigned int *)s;             /* bgrB */
                b = *(unsigned int *)(s+4);         /* GRbg */
                c = ((a & 0xF8) >> 3) | ((a & 0xF800) >> 6) | ((a & 0xF80000) >> 9);
                *(unsigned int *)d = c | (c << 16);
                c = ((a & 0xF8000000) >> 11) | ((b & 0xF8) << 18) | ((b & 0xF800) << 15);
                *(unsigned int *)(d+2*BPP2) = c | (c >> 16);
                a = *(unsigned int *)(s+8);         /* rBGR */
                c = ((b & 0xF80000) >> 19) | ((b & 0xF8000000) >> 22) | ((a & 0xF8) << 7);
                *(unsigned int *)(d+2*2*BPP2) = c | (c << 16);
                c = ((a & 0xF800) << 5) | ((a & 0xF80000) << 2) | ((a & 0xF8000000) >> 1);
                *(unsigned int *)(d+3*2*BPP2) = c | (c >> 16);
                d += 2*4*BPP2; s += 4*BPP3;
                sdx -= 4;
            }

            /* the remaining pixels: */
            while (sdx) {
                /* bbbbbbbb.gggggggg.rrrrrrrr -> gggbbbbb.0rrrrrgg: */
                register unsigned short a;
                a = (s[0] >> 3) | ((s[1] & 0xF8) << 2) | ((s[2] & 0xF8) << 7);
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2;  s += BPP3;
                sdx --;
            }
            s -= src_dx * BPP3;
            d -= src_dx * 2*BPP2;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP2); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * No dithering yet.
 */
int RGB24toRGB8 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            register int sx = src_x, ddx = dest_dx;

            /* process misaligned pixels first: */
            while ((sx & 3) && ddx) {
                /* BGR -> palette index: */
                *d = pmap[(unsigned int)
                    ((s[0] & 0xf0) >> 4) |
                    ((s[1] & 0xf0)     ) |
                    ((s[2] & 0xf0) << 4)];
                d += BPP1; s += BPP3;
                sx ++; ddx --;
            }

            /* main loop: process 4 pixels a time: */
            while (ddx >= 4) {
                register unsigned int a, b;
                a = *(unsigned int *)s;             /* BGR.B */
                b = *(unsigned int *)(s+4);         /* GR.BG */
                *d = pmap[(unsigned int)
                    ((a & 0x000000f0) >>  4) |
                    ((a & 0x0000f000) >>  8) |
                    ((a & 0x00f00000) >> 12)];
                *(d+BPP1) = pmap[(unsigned int)
                    ((a & 0xf0000000) >> 28) |
                    ((b & 0x000000f0)      ) |
                    ((b & 0x0000f000) >>  4)];
                a = *(unsigned int *)(s+8);         /* R.BGR */
                *(d+2*BPP1) = pmap[(unsigned int)
                    ((b & 0x00f00000) >> 20) |
                    ((b & 0xf0000000) >> 24) |
                    ((a & 0x000000f0) <<  4)];
                *(d+3*BPP1) = pmap[(unsigned int)
                    ((a & 0x0000f000) >> 12) |
                    ((a & 0x00f00000) >> 16) |
                    ((a & 0xf0000000) >> 20)];
                d += BPP1*4; s += BPP3*4;
                ddx -= 4;
            }

            /* process the remaining 1..3 pixels: */
            while (ddx) {
                /* RGB -> palette index: */
                *d = pmap[(unsigned int)
                    ((s[0] & 0xf0) >> 4) |
                    ((s[1] & 0xf0)     ) |
                    ((s[2] & 0xf0) << 4)];
                d += BPP1; s += BPP3;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP3;
            d += dest_pitch - dest_dx * BPP1;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP3 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sx = src_x, sdx = src_dx;

            /* misaligned pixels first: */
            while ((sx & 3) && sdx) {
                /* BGR -> palette index: */
                register unsigned char c;
                c = pmap[(unsigned int)
                    ((s[0] & 0xf0) >> 4) |
                    ((s[1] & 0xf0)     ) |
                    ((s[2] & 0xf0) << 4)];
                *d = c;
                *(d+BPP1) = c;
                d += 2*BPP1; s += BPP3;
                sx ++; sdx --;
            }

            /* main bulk of data: */
            while (sdx >= 4) {
                register unsigned int a, b;
                register unsigned char c;
                a = *(unsigned int *)s;             /* bgrB */
                b = *(unsigned int *)(s+4);         /* GRbg */
                c = pmap[(unsigned int)
                    ((a & 0x000000f0) >>  4) |
                    ((a & 0x0000f000) >>  8) |
                    ((a & 0x00f00000) >> 12)];
                *(d+0) = c;
                *(d+BPP1) = c;
                c = pmap[(unsigned int)
                    ((a & 0xf0000000) >> 28) |
                    ((b & 0x000000f0)      ) |
                    ((b & 0x0000f000) >>  4)];
                *(d+2*BPP1) = c;
                *(d+3*BPP1) = c;
                a = *(unsigned int *)(s+8);         /* rBGR */
                c = pmap[(unsigned int)
                    ((b & 0x00f00000) >> 20) |
                    ((b & 0xf0000000) >> 24) |
                    ((a & 0x000000f0) <<  4)];
                *(d+4*BPP1) = c;
                *(d+5*BPP1) = c;
                c = pmap[(unsigned int)
                    ((a & 0x0000f000) >> 12) |
                    ((a & 0x00f00000) >> 16) |
                    ((a & 0xf0000000) >> 20)];
                *(d+6*BPP1) = c;
                *(d+7*BPP1) = c;
                d += 4*2*BPP1; s += 4*BPP3;
                sdx -= 4;
            }
            /* the remaining 1..3 pixels: */
            while (sdx) {
                /* BGR -> palette index: */
                register unsigned char c;
                c = pmap[(unsigned int)
                    ((s[0] & 0xf0) >> 4) |
                    ((s[1] & 0xf0)     ) |
                    ((s[2] & 0xf0) << 4)];
                *d = c;
                *(d+BPP1) = c;
                d += 2*BPP1; s += BPP3;
                sdx --;
            }
            s -= src_dx * BPP3;
            d -= src_dx * 2*BPP1;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP1); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

int RGB565toRGB32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB565toRGB24 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

/*
 * RGB565toRGB565() converter:
 *  1:1, 2:1
 */
int RGB565toRGB565 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* copy image: */
        for (i = 0; i < dest_dy; i ++) {
            memcpy (d, s, dest_dx * BPP2);  /* copy dest_dx pixels */ /* Flawfinder: ignore */
            s += src_pitch;
            d += dest_pitch;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sdx = src_dx;

            /* misaligned pixel first: */
            if (src_x & 1) {
                register unsigned short a;
                a = *(unsigned short *)s;
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2; s += BPP2;
                sdx --;
            }
            /* main bulk of data: */
            while (sdx >= 2) {
                register unsigned int a;
                a = *(unsigned int *)s;
                *(unsigned int *)d = (a & 0xFFFF) | (a << 16);
                *(unsigned int *)(d+2*BPP2) = (a & 0xFFFF0000) | (a >> 16);
                d += 2*2*BPP2; s += 2*BPP2;
                sdx -= 2;
            }
            /* the remaining odd pixel: */
            if (sdx) {
                register unsigned short a;
                a = *(unsigned short *)s;
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2; s += BPP2;
            }
            s -= src_dx * BPP2;
            d -= src_dx * 2*BPP2;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP2); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

int RGB565toRGB555 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

/*
 * No dithering yet.
 */
int RGB565toRGB8 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            register int ddx = dest_dx;

            /* first odd pixel: */
            if (src_x & 1) {
                /* gggbbbbb.rrrrrggg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                *d = pmap[
                    ((a & 0x001E) >> 1) |
                    ((a & 0x0780) >> 3) |
                    ((a & 0xf000) >> 4)];
                d += BPP1; s += BPP2;
                ddx --;
            }

            /* main loop: process 2 pixels a time: */
            while (ddx >= 2) {
                /* gggbbbbb.rrrrrggg.gggbbbbb.rrrrrggg -> 2 palette indices */
                register unsigned int a;
                a = *(unsigned int *)s;
                *d = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x0780) >> 3) |
                    ((a & 0xf000) >> 4)];
                *(d+BPP1) = pmap[
                    ((a & 0x001e0000) >> 17) |
                    ((a & 0x07800000) >> 19) |
                    ((a & 0xf0000000) >> 20)];
                d += BPP1*2; s += BPP2*2;
                ddx -= 2;
            }

            /* the remaining odd pixel: */
            if (ddx) {
                /* gggbbbbb.rrrrrggg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                *d = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x0780) >> 3) |
                    ((a & 0xf000) >> 4)];
                d += BPP1; s += BPP2;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP2;
            d += dest_pitch - dest_dx * BPP1;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sdx = src_dx;

            /* first odd pixel: */
            if (src_x & 1) {
                /* gggbbbbb.rrrrrggg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                register unsigned char c;
                c = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x0780) >> 3) |
                    ((a & 0xf000) >> 4)];
                *d = c;
                *(d+BPP1) = c;
                d += 2*BPP1; s += BPP2;
                sdx --;
            }

            /* main bulk of data: */
            while (sdx >= 2) {
                /* gggbbbbb.rrrrrggg.gggbbbbb.rrrrrggg -> 2 palette indices */
                register unsigned int a;
                register unsigned char c;
                a = *(unsigned int *)s;
                c = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x0780) >> 3) |
                    ((a & 0xf000) >> 4)];
                *(d+0) = c;
                *(d+BPP1) = c;
                c = pmap[
                    ((a & 0x001e0000) >> 17) |
                    ((a & 0x07800000) >> 19) |
                    ((a & 0xf0000000) >> 20)];
                *(d+2*BPP1) = c;
                *(d+3*BPP1) = c;
                d += 2*2*BPP1; s += 2*BPP2;
                sdx -= 2;
            }

            /* the remaining odd pixel: */
            if (sdx) {
                /* gggbbbbb.rrrrrggg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                register unsigned char c;
                c = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x0780) >> 3) |
                    ((a & 0xf000) >> 4)];
                *d = c;
                *(d+BPP1) = c;
                d += 2*BPP1; s += BPP2;
            }
            s -= src_dx * BPP2;
            d -= src_dx * 2*BPP1;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP1); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}


int RGB555toRGB32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB555toRGB24 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

/*
 * RGB555toRGB565() converter:
 *  1:1, 2:1
 */
int RGB555toRGB565 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            register int sx = src_x, ddx = dest_dx;

            /* process misaligned pixels first: */
            while ((sx & 3) && ddx) {
                /* gggbbbbb.0rrrrrgg -> gg0bbbbb.rrrrrggg : */
                register unsigned short c = *(unsigned short *)s;
                *(unsigned short *)d = (c & 0x1F) | ((c & 0x7FE0) << 1);
                d += BPP2;  s += BPP2;
                sx ++; ddx --;
            }

            /* main loop: process 4 pixels a time: */
            while (ddx >= 4) {
                register unsigned int a, b;
                a = *(unsigned int *)s;     /* gggbbbbb.0rrrrrgg.gggbbbbb.0rrrrrgg */
                b = *(unsigned int *)(s+4);
                *(unsigned int *)d = (a & 0x1F001F) | ((a & 0x7FE07FE0) << 1);
                *(unsigned int *)(d+4) = (b & 0x1F001F) | ((b & 0x7FE07FE0) << 1);
                d += BPP2*4; s += BPP2*4; ddx -= 4;
            }

            /* process the remaining 1..3 pixels: */
            while (ddx) {
                /* gggbbbbb.0rrrrrgg -> gg0bbbbb.rrrrrggg : */
                register unsigned short c = *(unsigned short *)s;
                *(unsigned short *)d = (c & 0x1F) | ((c & 0x7FE0) << 1);
                d += BPP2;  s += BPP2;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP2;
            d += dest_pitch - dest_dx * BPP2;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sx = src_x, sdx = src_dx;

            /* misaligned pixels first: */
            while ((sx & 3) && sdx) {
                /* gggbbbbb.0rrrrrgg -> gg0bbbbb.rrrrrggg : */
                register unsigned short a, b;
                a = *(unsigned short *)s;
                b = (a & 0x1F) | ((a & 0x7FE0) << 1);
                *(unsigned short *)d = b;
                *(unsigned short *)(d+BPP2) = b;
                d += 2*BPP2; s += BPP2;
                sx ++; sdx --;
            }
            /* main bulk of data: */
            while (sdx >= 4) {
                register unsigned int a, b;
                a = *(unsigned int *)s;     /* gggbbbbb.0rrrrrgg.gggbbbbb.0rrrrrgg */
                b = (a & 0x1F001F) | ((a & 0x7FE07FE0) << 1);
                *(unsigned int *)d = (b & 0xFFFF) | (b << 16);
                *(unsigned int *)(d+2*BPP2) = (b & 0xFFFF0000) | (b >> 16);
                a = *(unsigned int *)(s+2*BPP2);
                b = (a & 0x1F001F) | ((a & 0x7FE07FE0) << 1);
                *(unsigned int *)(d+2*2*BPP2) = (b & 0xFFFF) | (b << 16);
                *(unsigned int *)(d+3*2*BPP2) = (b & 0xFFFF0000) | (b >> 16);
                d += 2*4*BPP2; s += 4*BPP2;
                sdx -= 4;
            }
            /* the remaining pixels: */
            while (sdx) {
                /* gggbbbbb.0rrrrrgg -> gg0bbbbb.rrrrrggg : */
                register unsigned short a, b;
                a = *(unsigned short *)s;
                b = (a & 0x1F) | ((a & 0x7FE0) << 1);
                *(unsigned short *)d = b;
                *(unsigned short *)(d+BPP2) = b;
                d += 2*BPP2; s += BPP2;
                sdx --;
            }
            s -= src_dx * BPP2;
            d -= src_dx * 2*BPP2;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP2); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * RGB555toRGB555() converter:
 *  1:1, 2:1
 */
int RGB555toRGB555 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* copy image: */
        for (i = 0; i < dest_dy; i ++) {
            memcpy (d, s, dest_dx * BPP2);  /* copy dest_dx pixels */ /* Flawfinder: ignore */
            s += src_pitch;
            d += dest_pitch;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP2 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sdx = src_dx;

            /* misaligned pixel first: */
            if (src_x & 1) {
                register unsigned short a;
                a = *(unsigned short *)s;
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2; s += BPP2;
                sdx --;
            }
            /* main bulk of data: */
            while (sdx >= 2) {
                register unsigned int a;
                a = *(unsigned int *)s;
                *(unsigned int *)d = (a & 0xFFFF) | (a << 16);
                *(unsigned int *)(d+2*BPP2) = (a & 0xFFFF0000) | (a >> 16);
                d += 2*2*BPP2; s += 2*BPP2;
                sdx -= 2;
            }
            /* the remaining odd pixel: */
            if (sdx) {
                register unsigned short a;
                a = *(unsigned short *)s;
                *(unsigned short *)d = a;
                *(unsigned short *)(d+BPP2) = a;
                d += 2*BPP2; s += BPP2;
            }
            s -= src_dx * BPP2;
            d -= src_dx * 2*BPP2;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP2); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}

/*
 * No dithering yet.
 */
int RGB555toRGB8 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* copy rows: */
        for (i = 0; i < dest_dy; i ++) {

            register int ddx = dest_dx;

            /* first odd pixel: */
            if (src_x & 1) {
                /* gggbbbbb.0rrrrrgg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                *d = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x03c0) >> 2) |
                    ((a & 0x7800) >> 3)];
                d += BPP1; s += BPP2;
                ddx --;
            }

            /* main loop: process 2 pixels a time: */
            while (ddx >= 2) {
                /* gggbbbbb.0rrrrrgg.gggbbbbb.0rrrrrgg -> 2 palette indices */
                register unsigned int a;
                a = *(unsigned int *)s;
                *d = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x03c0) >> 2) |
                    ((a & 0x7800) >> 3)];
                *(d+BPP1) = pmap[
                    ((a & 0x001e0000) >> 17) |
                    ((a & 0x03c00000) >> 18) |
                    ((a & 0x78000000) >> 19)];
                d += BPP1*2; s += BPP2*2;
                ddx -= 2;
            }

            /* the remaining odd pixel: */
            if (ddx) {
                /* gggbbbbb.0rrrrrgg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                *d = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x03c0) >> 2) |
                    ((a & 0x7800) >> 3)];
                d += BPP1; s += BPP2;
                ddx --;
            }

            /* bump pointers to the next row: */
            s += src_pitch - dest_dx * BPP2;
            d += dest_pitch - dest_dx * BPP1;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP2 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sdx = src_dx;

            /* first odd pixel: */
            if (src_x & 1) {
                /* gggbbbbb.0rrrrrgg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                register unsigned char c;
                c = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x03c0) >> 2) |
                    ((a & 0x7800) >> 3)];
                *d = c;
                *(d+BPP1) = c;
                d += 2*BPP1; s += BPP2;
                sdx --;
            }

            /* main bulk of data: */
            while (sdx >= 2) {
                /* gggbbbbb.0rrrrrgg.gggbbbbb.0rrrrrgg -> 2 palette indices */
                register unsigned int a;
                register unsigned char c;
                a = *(unsigned int *)s;
                c = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x03c0) >> 2) |
                    ((a & 0x7800) >> 3)];
                *(d+0) = c;
                *(d+BPP1) = c;
                c = pmap[
                    ((a & 0x001e0000) >> 17) |
                    ((a & 0x03c00000) >> 18) |
                    ((a & 0x78000000) >> 19)];
                *(d+2*BPP1) = c;
                *(d+3*BPP1) = c;
                d += 2*2*BPP1; s += 2*BPP2;
                sdx -= 2;
            }

            /* the remaining odd pixel: */
            if (sdx) {
                /* gggbbbbb.0rrrrrgg -> palette index: */
                unsigned short a = *(unsigned short *)s;
                register unsigned char c;
                c = pmap[
                    ((a & 0x001e) >> 1) |
                    ((a & 0x03c0) >> 2) |
                    ((a & 0x7800) >> 3)];
                *d = c;
                *(d+BPP1) = c;
                d += 2*BPP1; s += BPP2;
            }
            s -= src_dx * BPP2;
            d -= src_dx * 2*BPP1;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP1); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}


int RGB8toRGB32 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB8toRGB24 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB8toRGB565 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

int RGB8toRGB555 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return -1;   /* not implemented yet... */
}

/*
 * RGB8toRGB8() converter:
 *  1:1, 2:1
 */
int RGB8toRGB8 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* check if bottom-up bitmaps: */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP1 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* copy image: */
        for (i = 0; i < dest_dy; i ++) {
            memcpy (d, s, dest_dx * BPP1);  /* copy dest_dx pixels */ /* Flawfinder: ignore */
            s += src_pitch;
            d += dest_pitch;
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {

        /* local variables: */
        unsigned char *s, *d;
        register int i;

        /* get pointers: */
        s = src_ptr + src_x * BPP1 + src_y * src_pitch;
        d = dest_ptr + dest_x * BPP1 + dest_y * dest_pitch;

        /* stretch image: */
        for (i = 0; i < src_dy; i ++) {

            /* stretch a line: */
            register int sx = src_x, sdx = src_dx;

            /* misaligned pixels first: */
            while ((sx & 3) && sdx) {
                register unsigned char a;
                a = *s; *d = a; *(d+BPP1) = a;
                d += 2*BPP1; s += BPP1;
                sx ++; sdx --;
            }
            /* main bulk of data: */
            while (sdx >= 4) {
                register unsigned int a;
                a = *(unsigned int *)s;
                *(unsigned int *)d = (a & 0xFF) | ((a & 0xFFFF) << 8) | ((a & 0xFF00) << 16);
                *(unsigned int *)(d+4*BPP1) = (a & 0xFF000000) | ((a & 0xFFFF0000) >> 8) | ((a & 0xFF0000) >> 16);
                d += 2*4*BPP1; s += 4*BPP1;
                sdx -= 4;
            }
            /* the remaining 1..3 pixels: */
            while (sdx) {
                register unsigned char a;
                a = *s; *d = a; *(d+BPP1) = a;
                d += 2*BPP1; s += BPP1;
                sdx --;
            }
            s -= src_dx * BPP1;
            d -= src_dx * 2*BPP1;

            /* replicate a line (vertical stretching): */
            memcpy (d + dest_pitch, d, src_dx * 2 * BPP1); /* Flawfinder: ignore */

            /* bump pointers to the next row: */
            s += src_pitch;
            d += dest_pitch * 2;
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
}


/*
 * Old I420->RGB converters:
 * Use:
 *  void oldI420toRGBXXX (unsigned char *ysrc, unsigned char *usrc,
 *          unsigned char *vsrc, int pitchSrc, unsigned char *dst,
 *          int width, int height, int pitchDst);
 * Input:
 *  ysrc, usrc, vsrc - pointers to Y, Cr, and Cb components of the frame
 *  pitchSrc - pitch of the input frame (luminance)
 *  dst - pointer to an output buffer
 *  width, height - the size of frame to convert
 *  pitchDst - pitch of the output buffer (in RGB pixels!!!)
 * Returns:
 *  none.
 */

/* the driver function: */
static int oldI420toRGB (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst, int bpp,
    void (* dbline) (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
    int src_x, int dx))
{
    unsigned char *sy1, *sy2, *sv, *su, *d1, *d2;
    register int j, pitch = pitchDst * bpp;

    /* bump dest to other end, if bottom-up output: */
    if (pitch < 0)
        dst += -pitch * (height-1);                     /* start of last line */

    /* get pointers: */
    sy1 = ysrc;                                         /* luma offset */
    sy2 = sy1  + pitchSrc;
    su  = usrc;                                         /* chroma offset */
    sv  = vsrc;
    d1  = dst;                                          /* RGB offset */
    d2  = d1 + pitch;

    /* convert aligned portion of the image: */
    for (j = 0; j < height/2; j ++) {

        /* convert two lines a time: */
        (* dbline) (d1, d2, 0, sy1, sy2, su, sv, 0, width);

        sy1 += pitchSrc*2; sy2 += pitchSrc*2;
        su  += pitchSrc/2; sv  += pitchSrc/2;
        d1  += pitch*2; d2  += pitch*2;
    }

    return 0;
}

/* actual converters: */
void oldI420toRGB32 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGB (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 4,
        is_alpha? dblineI420toRGB32alpha: dblineI420toRGB32);
}

void oldI420toRGB24 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGB (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 3,
        is_alpha? dblineI420toRGB24alpha: dblineI420toRGB24);
}

void oldI420toRGB565 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGB (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 2,
        is_alpha? dblineI420toRGB565alpha: dblineI420toRGB565);
}

void oldI420toRGB555 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGB (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 2,
        is_alpha? dblineI420toRGB555alpha: dblineI420toRGB555);
}

/*
 * Convert two YUV lines into RGB linebufs.
 * Produces two RGB lines per call.
 * Output in padded RGB format, needed for SIMD interpolation.
 */
static void convertI420toXRGB (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    unsigned char *buf1, unsigned char *buf2, int width, int pitchSrc)
{
    unsigned char * ysrc2 = ysrc + pitchSrc;

    if (is_alpha) {

        /* use full matrix: */
        for (; width; width -= 2) {

            int ruv, guv, buv, y;

            buv = butab[usrc[0]] + bvtab[vsrc[0]];
            guv = gutab[usrc[0]] + gvtab[vsrc[0]];
            ruv = rutab[usrc[0]] + rvtab[vsrc[0]];

            /* store as |00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB| */
            y = ytab[ysrc[0]];
            *(int *)(buf1+0) =
                (CLIP8[y + buv] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + ruv] << 22);

            y = ytab[ysrc[1]];
            *(int *)(buf1+4) =
                (CLIP8[y + buv] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + ruv] << 22);

            y = ytab[ysrc2[0]];
            *(int *)(buf2+0) =
                (CLIP8[y + buv] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + ruv] << 22);


            y = ytab[ysrc2[1]];
            *(int *)(buf2+4) =
                (CLIP8[y + buv] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + ruv] << 22);

            /* next 2x2 block */
            ysrc += 2; ysrc2 += 2;
            usrc += 1; vsrc += 1;
            buf1 += 8; buf2 += 8;
        }

    } else {

        /* no chroma rotation: */
        for (; width; width -= 2) {

            int rv, guv, bu, y;

            bu = butab[usrc[0]];
            guv = gutab[usrc[0]] + gvtab[vsrc[0]];
            rv = rvtab[vsrc[0]];

            /* store as |00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB| */
            y = ytab[ysrc[0]];
            *(int *)(buf1+0) =
                (CLIP8[y + bu] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + rv] << 22);

            y = ytab[ysrc[1]];
            *(int *)(buf1+4) =
                (CLIP8[y + bu] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + rv] << 22);

            y = ytab[ysrc2[0]];
            *(int *)(buf2+0) =
                (CLIP8[y + bu] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + rv] << 22);

            y = ytab[ysrc2[1]];
            *(int *)(buf2+4) =
                (CLIP8[y + bu] << 0) |
                (CLIP8[y + guv] << 11) |
                (CLIP8[y + rv] << 22);

            /* next 2x2 block */
            ysrc += 2; ysrc2 += 2;
            usrc += 1; vsrc += 1;
            buf1 += 8; buf2 += 8;
        }
    }
}

/*
 * Interpolate and pack RGB lines into final output
 * Produces two output lines per call.
 * Requires padded RGB for SIMD interpolation.
 */
#define ROUND888 0x00400801

/* RGB32 version: */
static void interpRGB32 (unsigned char *src1, unsigned char *src2,
    unsigned char *dst1, unsigned char *dst2, int width)
{
    unsigned int a, b, c, d, e, f;
    unsigned int w, x, y, z;

    width >>= 1;                /* two per pass */
    while (--width) {           /* do all but last pair */

    /*
     * Input pels       Output pels
     *  a b e           w  x  y  z
     *  c d f           w' x' y' z'
     *
     * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
     */
        /* top line */
        a = *(unsigned int *)(src1+0);
        b = *(unsigned int *)(src1+4);
        e = *(unsigned int *)(src1+8);

        w = a;
        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)(dst1+0) =
            ((w & 0x000000ff) >> 0) |
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 6);
        *(unsigned int *)(dst1+4) =
            ((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7);
        *(unsigned int *)(dst1+8) =
            ((y & 0x000000ff) >> 0) |
            ((y & 0x0007f800) >> 3) |
            ((y & 0x3fc00000) >> 6);
        *(unsigned int *)(dst1+12) =
            ((z & 0x000001fe) >> 1) |
            ((z & 0x000ff000) >> 4) |
            ((z & 0x7f800000) >> 7);

        /* bottom line */
        c = *(unsigned int *)(src2+0);
        d = *(unsigned int *)(src2+4);
        f = *(unsigned int *)(src2+8);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)(dst2+0) =
            ((w & 0x000001fe) >> 1) |
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 7);
        *(unsigned int *)(dst2+4) =
            ((x & 0x000003fc) >> 2) |
            ((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 8);
        *(unsigned int *)(dst2+8) =
            ((y & 0x000001fe) >> 1) |
            ((y & 0x000ff000) >> 4) |
            ((y & 0x7f800000) >> 7);
        *(unsigned int *)(dst2+12) =
            ((z & 0x000003fc) >> 2) |
            ((z & 0x001fe000) >> 5) |
            ((z & 0xff000000) >> 8);

        /* bump pointers to next 2x2 input */
        src1 += 8; src2 += 8;
        dst1 += 16; dst2 += 16;
    }

    /*
     * For last 4 output pels, repeat final input pel
     * for offscreen input.  Equivalent to pixel-doubling the
     * last output pel.
     */
    /* top line */
    a = *(unsigned int *)(src1+0);
    b = *(unsigned int *)(src1+4);
    e = b;      /* repeat last input pel */

    w = a;
    x = a + b + ROUND888;
    y = b;
    z = b + e + ROUND888;

    /* pack and store */
    *(unsigned int *)(dst1+0) =
        ((w & 0x000000ff) >> 0) |
        ((w & 0x0007f800) >> 3) |
        ((w & 0x3fc00000) >> 6);
    *(unsigned int *)(dst1+4) =
        ((x & 0x000001fe) >> 1) |
        ((x & 0x000ff000) >> 4) |
        ((x & 0x7f800000) >> 7);
    *(unsigned int *)(dst1+8) =
        ((y & 0x000000ff) >> 0) |
        ((y & 0x0007f800) >> 3) |
        ((y & 0x3fc00000) >> 6);
    *(unsigned int *)(dst1+12) =
        ((z & 0x000001fe) >> 1) |
        ((z & 0x000ff000) >> 4) |
        ((z & 0x7f800000) >> 7);

    /* bottom line */
    c = *(unsigned int *)(src2+0);
    d = *(unsigned int *)(src2+4);
    f = d;      /* repeat last input pel */

    w = a + c + ROUND888;
    x = a + b + c + d + (ROUND888<<1);
    y = b + d + ROUND888;
    z = b + e + d + f + (ROUND888<<1);

    /* pack and store */
    *(unsigned int *)(dst2+0) =
        ((w & 0x000001fe) >> 1) |
        ((w & 0x000ff000) >> 4) |
        ((w & 0x7f800000) >> 7);
    *(unsigned int *)(dst2+4) =
        ((x & 0x000003fc) >> 2) |
        ((x & 0x001fe000) >> 5) |
        ((x & 0xff000000) >> 8);
    *(unsigned int *)(dst2+8) =
        ((y & 0x000001fe) >> 1) |
        ((y & 0x000ff000) >> 4) |
        ((y & 0x7f800000) >> 7);
    *(unsigned int *)(dst2+12) =
        ((z & 0x000003fc) >> 2) |
        ((z & 0x001fe000) >> 5) |
        ((z & 0xff000000) >> 8);
}

/* RGB24 version: */
static void interpRGB24 (unsigned char *src1, unsigned char *src2,
    unsigned char *dst1, unsigned char *dst2, int width)
{
    unsigned int a, b, c, d, e, f;
    unsigned int w, x, y, z;

    width >>= 1;        /* two per pass */
    while (--width) {   /* do all but last pair */

    /*
     * Input pels       Output pels
     *  a b e           w  x  y  z
     *  c d f           w' x' y' z'
     *
     * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
     */
        /* top line */
        a = *(unsigned int *)(src1+0);
        b = *(unsigned int *)(src1+4);
        e = *(unsigned int *)(src1+8);

        w = a;
        x = a + b + ROUND888;
        y = b;
        z = b + e + ROUND888;

        /* pack and store */
        *(unsigned int *)(dst1+0) =
            ((w & 0x000000ff) >> 0) |
            ((w & 0x0007f800) >> 3) |
            ((w & 0x3fc00000) >> 6) |
            ((x & 0x000001fe) << 23);
        *(unsigned int *)(dst1+4) =
            ((x & 0x000ff000) >> 12) |
            ((x & 0x7f800000) >> 15) |
            ((y & 0x000000ff) << 16) |
            ((y & 0x0007f800) << 13);
        *(unsigned int *)(dst1+8) =
            ((y & 0x3fc00000) >> 22) |
            ((z & 0x000001fe) << 7) |
            ((z & 0x000ff000) << 4) |
            ((z & 0x7f800000) << 1);

        /* bottom line */
        c = *(unsigned int *)(src2+0);
        d = *(unsigned int *)(src2+4);
        f = *(unsigned int *)(src2+8);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);

        /* pack and store */
        *(unsigned int *)(dst2+0) =
            ((w & 0x000001fe) >> 1) |
            ((w & 0x000ff000) >> 4) |
            ((w & 0x7f800000) >> 7) |
            ((x & 0x000003fc) << 22);
        *(unsigned int *)(dst2+4) =
            ((x & 0x001fe000) >> 13) |
            ((x & 0xff000000) >> 16) |
            ((y & 0x000001fe) << 15) |
            ((y & 0x000ff000) << 12);
        *(unsigned int *)(dst2+8) =
            ((y & 0x7f800000) >> 23) |
            ((z & 0x000003fc) << 6) |
            ((z & 0x001fe000) << 3) |
            ((z & 0xff000000) << 0);

        /* next 2x2 input block */
        src1 += 8; src2 += 8;
        dst1 += 12; dst2 += 12;
    }

    /*
     * For last 4 output pels, repeat the final input pel for
     * for missing input.  Equivalent to pixel-doubling the
     * last output pel.
     */
    /* top line */
    a = *(unsigned int *)(src1+0);
    b = *(unsigned int *)(src1+4);
    e = b;      /* repeat last input pel */

    w = a;
    x = a + b + ROUND888;
    y = b;
    z = b + e + ROUND888;

    /* pack and store */
    *(unsigned int *)(dst1+0) =
        ((w & 0x000000ff) >> 0) |
        ((w & 0x0007f800) >> 3) |
        ((w & 0x3fc00000) >> 6) |
        ((x & 0x000001fe) << 23);
    *(unsigned int *)(dst1+4) =
        ((x & 0x000ff000) >> 12) |
        ((x & 0x7f800000) >> 15) |
        ((y & 0x000000ff) << 16) |
        ((y & 0x0007f800) << 13);
    *(unsigned int *)(dst1+8) =
        ((y & 0x3fc00000) >> 22) |
        ((z & 0x000001fe) << 7) |
        ((z & 0x000ff000) << 4) |
        ((z & 0x7f800000) << 1);

    /* bottom line */
    c = *(unsigned int *)(src2+0);
    d = *(unsigned int *)(src2+4);
    f = d;      /* repeat last input pel */

    w = a + c + ROUND888;
    x = a + b + c + d + (ROUND888<<1);
    y = b + d + ROUND888;
    z = b + e + d + f + (ROUND888<<1);

    /* pack and store */
    *(unsigned int *)(dst2+0) =
        ((w & 0x000001fe) >> 1) |
        ((w & 0x000ff000) >> 4) |
        ((w & 0x7f800000) >> 7) |
        ((x & 0x000003fc) << 22);
    *(unsigned int *)(dst2+4) =
        ((x & 0x001fe000) >> 13) |
        ((x & 0xff000000) >> 16) |
        ((y & 0x000001fe) << 15) |
        ((y & 0x000ff000) << 12);
    *(unsigned int *)(dst2+8) =
        ((y & 0x7f800000) >> 23) |
        ((z & 0x000003fc) << 6) |
        ((z & 0x001fe000) << 3) |
        ((z & 0xff000000) << 0);
}

/* RGB565 version: */
static void interpRGB565 (unsigned char *src1, unsigned char *src2,
    unsigned char *dst1, unsigned char *dst2, int width)
{
    unsigned int a, b, c, d, e, f;
    unsigned int w, x, y, z;

    width >>= 1;        /* two per pass */
    while (--width) {   /* do all but last pair */

    /*
     * Input pels       Output pels
     *  a b e           w  x  y  z
     *  c d f           w' x' y' z'
     *
     * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
     */
        /* top line */
        a = *(unsigned int *)(src1+0);
        b = *(unsigned int *)(src1+4);
        e = *(unsigned int *)(src1+8);

        w = a;
        x = a + b;
        y = b;
        z = b + e;

        /* pack and store */
        *(unsigned int *)(dst1+0) =
            ((w & 0x000000f8) >> 3) |
            ((w & 0x0007e000) >> 8) |
            ((w & 0x3e000000) >> 14) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000fc000) << 7) |
            ((x & 0x7c000000) << 1);
        *(unsigned int *)(dst1+4) =
            ((y & 0x000000f8) >> 3) |
            ((y & 0x0007e000) >> 8) |
            ((y & 0x3e000000) >> 14) |
            ((z & 0x000001f0) << 12) |
            ((z & 0x000fc000) << 7) |
            ((z & 0x7c000000) << 1);

        /* bottom line */
        c = *(unsigned int *)(src2+0);
        d = *(unsigned int *)(src2+4);
        f = *(unsigned int *)(src2+8);

        w = a + c;
        x = a + b + c + d;
        y = b + d;
        z = b + e + d + f;

        /* pack and store */
        *(unsigned int *)(dst2+0) =
            ((w & 0x000001f0) >> 4) |
            ((w & 0x000fc000) >> 9) |
            ((w & 0x7c000000) >> 15) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f8000) << 6) |
            ((x & 0xf8000000) << 0);
        *(unsigned int *)(dst2+4) =
            ((y & 0x000001f0) >> 4) |
            ((y & 0x000fc000) >> 9) |
            ((y & 0x7c000000) >> 15) |
            ((z & 0x000003e0) << 11) |
            ((z & 0x001f8000) << 6) |
            ((z & 0xf8000000) << 0);

        /* next 2x2 input block */
        src1 += 8; src2 += 8;
        dst1 += 8; dst2 += 8;
    }

    /*
     * For last 4 output pels, repeat the final input pel for
     * for missing input.  Equivalent to pixel-doubling the
     * last output pel.
     */
    /* top line */
    a = *(unsigned int *)(src1+0);
    b = *(unsigned int *)(src1+4);
    e = b;      /* repeat last input pel */

    w = a;
    x = a + b;
    y = b;
    z = b + e;

    /* pack and store */
    *(unsigned int *)(dst1+0) =
        ((w & 0x000000f8) >> 3) |
        ((w & 0x0007e000) >> 8) |
        ((w & 0x3e000000) >> 14) |
        ((x & 0x000001f0) << 12) |
        ((x & 0x000fc000) << 7) |
        ((x & 0x7c000000) << 1);
    *(unsigned int *)(dst1+4) =
        ((y & 0x000000f8) >> 3) |
        ((y & 0x0007e000) >> 8) |
        ((y & 0x3e000000) >> 14) |
        ((z & 0x000001f0) << 12) |
        ((z & 0x000fc000) << 7) |
        ((z & 0x7c000000) << 1);

    /* bottom line */
    c = *(unsigned int *)(src2+0);
    d = *(unsigned int *)(src2+4);
    f = d;      /* repeat last input pel */

    w = a + c;
    x = a + b + c + d;
    y = b + d;
    z = b + e + d + f;

    /* pack and store */
    *(unsigned int *)(dst2+0) =
        ((w & 0x000001f0) >> 4) |
        ((w & 0x000fc000) >> 9) |
        ((w & 0x7c000000) >> 15) |
        ((x & 0x000003e0) << 11) |
        ((x & 0x001f8000) << 6) |
        ((x & 0xf8000000) << 0);
    *(unsigned int *)(dst2+4) =
        ((y & 0x000001f0) >> 4) |
        ((y & 0x000fc000) >> 9) |
        ((y & 0x7c000000) >> 15) |
        ((z & 0x000003e0) << 11) |
        ((z & 0x001f8000) << 6) |
        ((z & 0xf8000000) << 0);
}

/* RGB555 version: */
static void interpRGB555 (unsigned char *src1, unsigned char *src2,
    unsigned char *dst1, unsigned char *dst2, int width)
{
    unsigned int a, b, c, d, e, f;
    unsigned int w, x, y, z;

    width >>= 1;        /* two per pass */
    while (--width) {   /* do all but last pair */

    /*
     * Input pels       Output pels
     *  a b e           w  x  y  z
     *  c d f           w' x' y' z'
     *
     * Input stored as 00 RRRRRRRR 000 GGGGGGGG 000 BBBBBBBB
     */
        /* top line */
        a = *(unsigned int *)(src1+0);
        b = *(unsigned int *)(src1+4);
        e = *(unsigned int *)(src1+8);

        w = a;
        x = a + b;
        y = b;
        z = b + e;

        /* pack and store */
        *(unsigned int *)(dst1+0) =
            ((w & 0x000000f8) >> 3) |
            ((w & 0x0007c000) >> 9) |
            ((w & 0x3e000000) >> 15) |
            ((x & 0x000001f0) << 12) |
            ((x & 0x000f8000) << 6) |
            ((x & 0x7c000000) << 0);
        *(unsigned int *)(dst1+4) =
            ((y & 0x000000f8) >> 3) |
            ((y & 0x0007c000) >> 9) |
            ((y & 0x3e000000) >> 15) |
            ((z & 0x000001f0) << 12) |
            ((z & 0x000f8000) << 6) |
            ((z & 0x7c000000) << 0);

        /* bottom line */
        c = *(unsigned int *)(src2+0);
        d = *(unsigned int *)(src2+4);
        f = *(unsigned int *)(src2+8);

        w = a + c;
        x = a + b + c + d;
        y = b + d;
        z = b + e + d + f;

        /* pack and store */
        *(unsigned int *)(dst2+0) =
            ((w & 0x000001f0) >> 4) |
            ((w & 0x000f8000) >> 10) |
            ((w & 0x7c000000) >> 16) |
            ((x & 0x000003e0) << 11) |
            ((x & 0x001f0000) << 5) |
            ((x & 0xf8000000) >> 1);
        *(unsigned int *)(dst2+4) =
            ((y & 0x000001f0) >> 4) |
            ((y & 0x000f8000) >> 10) |
            ((y & 0x7c000000) >> 16) |
            ((z & 0x000003e0) << 11) |
            ((z & 0x001f0000) << 5) |
            ((z & 0xf8000000) >> 1);

        /* next 2x2 input block */
        src1 += 8; src2 += 8;
        dst1 += 8; dst2 += 8;
    }

    /*
     * For last 4 output pels, repeat the final input pel for
     * for missing input.  Equivalent to pixel-doubling the
     * last output pel.
     */
    /* top line */
    a = *(unsigned int *)(src1+0);
    b = *(unsigned int *)(src1+4);
    e = b;      /* repeat last input pel */

    w = a;
    x = a + b;
    y = b;
    z = b + e;

    /* pack and store */
    *(unsigned int *)(dst1+0) =
        ((w & 0x000000f8) >> 3) |
        ((w & 0x0007c000) >> 9) |
        ((w & 0x3e000000) >> 15) |
        ((x & 0x000001f0) << 12) |
        ((x & 0x000f8000) << 6) |
        ((x & 0x7c000000) << 0);
    *(unsigned int *)(dst1+4) =
        ((y & 0x000000f8) >> 3) |
        ((y & 0x0007c000) >> 9) |
        ((y & 0x3e000000) >> 15) |
        ((z & 0x000001f0) << 12) |
        ((z & 0x000f8000) << 6) |
        ((z & 0x7c000000) << 0);

    /* bottom line */
    c = *(unsigned int *)(src2+0);
    d = *(unsigned int *)(src2+4);
    f = d;      /* repeat last input pel */

    w = a + c;
    x = a + b + c + d;
    y = b + d;
    z = b + e + d + f;

    /* pack and store */
    *(unsigned int *)(dst2+0) =
        ((w & 0x000001f0) >> 4) |
        ((w & 0x000f8000) >> 10) |
        ((w & 0x7c000000) >> 16) |
        ((x & 0x000003e0) << 11) |
        ((x & 0x001f0000) << 5) |
        ((x & 0xf8000000) >> 1);
    *(unsigned int *)(dst2+4) =
        ((y & 0x000001f0) >> 4) |
        ((y & 0x000f8000) >> 10) |
        ((y & 0x7c000000) >> 16) |
        ((z & 0x000003e0) << 11) |
        ((z & 0x001f0000) << 5) |
        ((z & 0xf8000000) >> 1);
}

/* the driver function: */
void oldI420toRGBx2 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst, int bpp,
    void (* interp) (unsigned char *, unsigned char *, unsigned char *, unsigned char *, int))
{
    /* Pointers to the three RGB linebuffers */
    unsigned char *buf[3]; /* Flawfinder: ignore */
    int ibuf = 0;                           /* circular buffer index */

    int dstinc = bpp * 2 * pitchDst;        /* offset to next output line */
    unsigned char *dst2 = dst + bpp * pitchDst;    /* second output line */

    buf[0] = linebuf;
    buf[1] = linebuf + 4 * width;
    buf[2] = linebuf + 8 * width;

    /* Bump dst to other end, if inverting ouput */
    if (pitchDst < 0) {
        dst += bpp * -pitchDst * (2*height - 1);
        dst2 += bpp * -pitchDst * (2*height - 1);
    }

    /* Special case for first 2 lines */
    convertI420toXRGB (ysrc, usrc, vsrc, buf[next[ibuf]], buf[next2[ibuf]], width, pitchSrc);
    ysrc += pitchSrc << 1;                  /* next 2 lines */
    usrc += pitchSrc >> 1;                  /* next line */
    vsrc += pitchSrc >> 1;                  /* next line */

    /* skip first interp */
    ibuf = next[ibuf];                      /* shift buffers */

    (* interp) (buf[ibuf], buf[next[ibuf]], dst, dst2, width);
    dst += dstinc;                          /* next 2 lines */
    dst2 += dstinc;
    ibuf = next[ibuf];                      /* shift buffers */

    height >>= 1;       /* two rows per pass */
    while (--height) {  /* do all but last pair */

        /* Convert 2 lines into bufs */
        convertI420toXRGB (ysrc, usrc, vsrc, buf[next[ibuf]], buf[next2[ibuf]], width, pitchSrc);
        ysrc += pitchSrc << 1;              /* next 2 lines */
        usrc += pitchSrc >> 1;              /* next line */
        vsrc += pitchSrc >> 1;              /* next line */

        /* Interp 2 lines into dst */
        (* interp) (buf[ibuf], buf[next[ibuf]], dst, dst2, width);
        dst += dstinc;                      /* next 2 lines */
        dst2 += dstinc;
        ibuf = next[ibuf];                  /* shift buffers */

        /* Interp 2 lines into dst */
        (* interp) (buf[ibuf], buf[next[ibuf]], dst, dst2, width);
        dst += dstinc;                      /* next 2 lines */
        dst2 += dstinc;
        ibuf = next[ibuf];                  /* shift buffers */
    }

    /* Last 2 lines, repeating last input line */
    (* interp) (buf[ibuf], buf[ibuf], dst, dst2, width);

}

/* actual converters: */
void oldI420toRGB32x2 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGBx2 (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 4, interpRGB32);
}

void oldI420toRGB24x2 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGBx2 (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 3, interpRGB24);
}

void oldI420toRGB565x2 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGBx2 (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 2, interpRGB565);
}

void oldI420toRGB555x2 (unsigned char *ysrc, unsigned char *usrc, unsigned char *vsrc,
    int pitchSrc, unsigned char *dst, int width, int height, int pitchDst)
{
    oldI420toRGBx2 (ysrc, usrc, vsrc, pitchSrc, dst, width, height, pitchDst, 2, interpRGB555);
}

#if defined (_MACINTOSH) && defined (_DEBUG)
#pragma global_optimizer off
#endif

/* colorcvt.c -- end of file */

