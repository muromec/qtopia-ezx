/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: setclrs.c,v 1.6 2007/07/06 20:53:51 jfinnecy Exp $
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

#include "env.h"    /* standard libraries & #defines */
#define  YUV_MAIN   /* declare all yuv-specific data here */
#include "yuv.h"    /* YUV-to-RGB conversion tables & macros */
#include "clip.h"   /* macros for clipping & dithering */
#include "colorlib.h" /* ensure that prototypes get extern C'ed */

/* from env.c: */
extern int CheckCPUType ();

/* from rgbpal.c: */
extern void InitializePalettes ();

/* from clip.c: */
extern void InitializeClipTables ();

/* current color adjustment parameters: */
static int color_conversion_tables_inited = 0;
static float cur_brightness, cur_contrast, cur_saturation, cur_hue;

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
    /* check if we have never been called: */
    if (!color_conversion_tables_inited) {

        /* identify type of CPU used: */
        CheckCPUType ();

        /* initialize clipping tables: */
        InitializeClipTables ();

        /* set default palettes for RGB8 formats: */
#if defined _FAT_HXCOLOR || defined HELIX_FEATURE_CC_RGB8out
        InitializePalettes ();
#endif
        /* set indicator: */
        color_conversion_tables_inited ++;
        goto regenerate_tables;
    }

    /* check if we need to re-generate color conversion tables: */
    if (is (cur_brightness - brightness) ||
        is (cur_contrast - contrast)     ||
        is (cur_saturation - saturation) ||
        is (cur_hue - hue)) {

        /* process color-adjustment parameters: */
        double alpha, beta, gamma, kappa, lambda;
        double cos_alpha, xi_sin_alpha, minus_zeta_sin_alpha;
        register int i;

regenerate_tables:

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

        /* generate color conversion tables: */
        for (i = 0; i < 256; i++) {

            /* Y'Cr'Cb'->R'G'B' conversion tables: */
            double y = lambda + (i - CCIR601_YOFFSET) * kappa; /* adjust contrast */
            if (y < 0) y = 0; else if (y > CCIR601_YMAX) y = CCIR601_YMAX;
            ytab  [i] = NEAREST_INT(y * YCOEF * gamma);
            rvtab [i] = NEAREST_INT((i-CCIR601_VOFFSET) * cos_alpha * RVCOEF * beta * gamma); 
            gvtab [i] = NEAREST_INT((i-CCIR601_VOFFSET) * GVCOEF * beta * gamma);
            bvtab [i] = NEAREST_INT((i-CCIR601_VOFFSET) * xi_sin_alpha * BUCOEF * beta * gamma);
            rutab [i] = NEAREST_INT((i-CCIR601_UOFFSET) * minus_zeta_sin_alpha * RVCOEF * beta * gamma);
            gutab [i] = NEAREST_INT((i-CCIR601_UOFFSET) * GUCOEF * beta * gamma);
            butab [i] = NEAREST_INT((i-CCIR601_UOFFSET) * cos_alpha * BUCOEF * beta * gamma);

            /* Y'Cr'Cb'->Y'Cr'Cb' conversion tables: */
            y = lambda + (i - CCIR601_YOFFSET) * kappa;
            _yytab [i] = _CLIP(8,CCIR601_YOFFSET + NEAREST_INT(y * gamma));
            _vvtab [i] = CCIR601_VOFFSET + NEAREST_INT((i-CCIR601_VOFFSET) * cos_alpha * beta * gamma);
            _uutab [i] = CCIR601_UOFFSET + NEAREST_INT((i-CCIR601_UOFFSET) * cos_alpha * beta * gamma);
            _vutab [i] = NEAREST_INT((i-CCIR601_UOFFSET) * minus_zeta_sin_alpha * beta * gamma);
            _uvtab [i] = NEAREST_INT((i-CCIR601_VOFFSET) * xi_sin_alpha * beta * gamma);
            if (!is_alpha) {
                _vvtab [i] = _CLIP(8,_vvtab [i]);
                _uutab [i] = _CLIP(8,_uutab [i]);
            }
        }
    }
}

void SetDestI420Colors (float brightness, float contrast, float saturation, float hue)
{
    /* currently, we do not support color adjustments when destination
     * format is I420 (YCrCb 4:1:1), and conversion is always CCIR 601-2 -
     * compliant (regardless of values of papameters). */

    register int i, j;

    /* initialize y*tab[] tables: */
    for (i = 0; i <= RGB_MAX; i++) {
        yrtab [i] = NEAREST_INT((double)i * CCIR601_YRCOEF);
        ygtab [i] = NEAREST_INT((double)i * CCIR601_YGCOEF);
        ybtab [i] = NEAREST_INT((double)i * CCIR601_YBCOEF);
    }

    /* initialize yytab[]: */
    for (i = 0; i <= YMAX; i++) {
        j = NEAREST_INT((double)i * YSCALE);
        if (j > CCIR601_YMAX) j = CCIR601_YMAX;
        yytab [i] = j + CCIR601_YOFFSET;
    }

    /* initialize vrytab[]: */
    for (i = -VMIN; i <= VMAX; i++) {
        j = NEAREST_INT((double)i * VSCALE);
        if (j < -CCIR601_VMAX/2) j = -CCIR601_VMAX/2;
        if (j >  CCIR601_VMAX/2) j =  CCIR601_VMAX/2;
        vrytab [VMIN+i] = j + CCIR601_VOFFSET;
    }

    /* initialize ubytab[]: */
    for (i = -UMIN; i <= UMAX; i++) {
        j = NEAREST_INT((double)i * USCALE);
        if (j < -CCIR601_UMAX/2) j = -CCIR601_UMAX/2;
        if (j >  CCIR601_UMAX/2) j =  CCIR601_UMAX/2;
        ubytab [UMIN+i] = j + CCIR601_UOFFSET;
    }
}

/*
 * Sets RGB->I420 chroma resampling mode
 * Use:
 *  int SetI420ChromaResamplingMode(int nNewMode);
 * Input:
 *  nNewMode - a new chroma resampling mode to use
 * Returns:
 *  previously set mode.
 */
int SetI420ChromaResamplingMode(int new_mode)
{
    register int m = chroma_resampling_mode;
    chroma_resampling_mode = new_mode;
    return m;
}

/* yuvctrl.c -- end of file */

