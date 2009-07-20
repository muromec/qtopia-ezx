/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: yuv2rgb.c,v 1.5 2006/01/03 22:25:08 eluevano Exp $
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

#include <math.h>
#include "nostatic/yuv.h"
#include "nostatic/colorlib.h"

/* macros to check big/little endiannes of the system: */
//const static union {char c[4]; unsigned int l;} _1234 = {"\001\002\003\004"};

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
    return fabs(val) > 0.01;   
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
void SetSrcI420Colors (float brightness, float contrast, float saturation, float hue,
		       color_data_t* pcd)
{
    /* check if we need to generate new color conversion tables: */
    if (!pcd->color_conversion_tables_inited  ||
        is (pcd->cur_brightness - brightness) ||
        is (pcd->cur_contrast - contrast)     ||
        is (pcd->cur_saturation - saturation) ||
        is (pcd->cur_hue - hue)) {

        double alpha, beta, gamma, kappa, lambda;
        double cos_alpha, xi_sin_alpha, minus_zeta_sin_alpha;
        register int i;
        int default_palette_idx [RGB_MAX+1];

        //Generate palette index.
        for(i=0; i<=255; i++ )
            default_palette_idx[i] = i;


        /* hue: */
        alpha = chrange (hue, ALPHA_MIN, ALPHA_MED, ALPHA_MAX);
        cos_alpha = cos (alpha);
        xi_sin_alpha = XI * sin (alpha);
        minus_zeta_sin_alpha = -ZETA * sin (alpha);
	pcd->is_alpha = is (pcd->cur_hue = hue);

        /* saturation: */
        beta = chrange (saturation, BETA_MIN, BETA_MED, BETA_MAX);
        pcd->is_beta = is (pcd->cur_saturation = saturation);

        /* brightness: */
        gamma = chrange (brightness, GAMMA_MIN, GAMMA_MED, GAMMA_MAX);
        pcd->is_gamma = is (pcd->cur_brightness = brightness);

        /* contrast: */
        kappa = chrange (contrast, KAPPA_MIN, KAPPA_MED, KAPPA_MAX);
        lambda = LAMBDA (kappa);
        pcd->is_kappa = is (pcd->cur_contrast = contrast);

        /*
         * Check if we need to generate clip tables & default palette:
         */
        if (!pcd->color_conversion_tables_inited) {

            /* Init clip tables. CLIP4/5/6 includes chop to 3/5/6 bits */
            for (i = -CLIPNEG; i < CLIPRNG + CLIPPOS; i++) {

                if (i < 0)
                {
#ifdef HELIX_FEATURE_CC_RGB444out                    
                    CLIP4[i] = 0;
                    CLIP8[i] = 0;
#endif
#ifdef HELIX_FEATURE_CC_RGB565out                    
                    CLIP5[i] = 0;
                    CLIP6[i] = 0;           /* clip */
#endif          
#ifdef HELIX_FEATURE_CC_RGB24out
                    CLIP8[i] = 0;
                    CLIP8[i] = 0;           /* clip */
#endif
                }
                else if (i > 255)
                {
#ifdef HELIX_FEATURE_CC_RGB444out                    
                    CLIP4[i] = 255 >> 4;
                    CLIP8[i] = /*JW255*/243;
#endif                        
#ifdef HELIX_FEATURE_CC_RGB565out
                    CLIP5[i] = 255 >> 3;
                    CLIP6[i] = 255 >> 2;    /* clip */
#endif   
#ifdef HELIX_FEATURE_CC_RGB24out
                    CLIP8[i] = 255;
                    CLIP8[i] = 255;    /* clip */
#endif
                }
                else
                {
#ifdef HELIX_FEATURE_CC_RGB444out                    
                    CLIP4[i] = i >> 4;      /* chop to 4 bits */
		    if (i > 243)
		       CLIP8[i] = 243;
		    else
		       CLIP8[i] = i;        /* leave at 8 bits */
#endif
#ifdef HELIX_FEATURE_CC_RGB565out                    
                    CLIP5[i] = i >> 3;      /* chop to 5 bits */
                    CLIP6[i] = i >> 2;      /* chop to 6 bits */
#endif   
#ifdef HELIX_FEATURE_CC_RGB24out
                    CLIP8[i] = i;      /* chop to 8 bits */
                    CLIP8[i] = i;      /* chop to 8 bits */
#endif
                }
            }

            /* set indicator: */
            pcd->color_conversion_tables_inited ++;
        }

        /*
         * Initialize color conversion tables.
         */
        for (i = 0; i < 256; i++) {

            /* Y'Cr'Cb'->R'G'B' conversion tables: */
            double y = lambda + (i - CCIR601_YOFFSET) * kappa; /* adjust contrast */
            if (y < 0) y = 0; else if (y > CCIR601_YMAX) y = CCIR601_YMAX;
            pcd->ytab  [i] = INT(y * YCOEF * gamma);
            pcd->rvtab [i] = INT((i-CCIR601_VOFFSET) * cos_alpha * RVCOEF * beta * gamma);
            pcd->gvtab [i] = INT((i-CCIR601_VOFFSET) * GVCOEF * beta * gamma);
            pcd->bvtab [i] = INT((i-CCIR601_VOFFSET) * xi_sin_alpha * BUCOEF * beta * gamma);
            pcd->rutab [i] = INT((i-CCIR601_UOFFSET) * minus_zeta_sin_alpha * RVCOEF * beta * gamma);
            pcd->gutab [i] = INT((i-CCIR601_UOFFSET) * GUCOEF * beta * gamma);
            pcd->butab [i] = INT((i-CCIR601_UOFFSET) * cos_alpha * BUCOEF * beta * gamma);

        }
    }
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
int
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
    if (dest_dx == src_dx)          *p_scale_x = 1; /* full res */
    else if (dest_dx == 2 * src_dx) *p_scale_x = 2; /* double res */
    else if (src_dx == 2 * dest_dx) *p_scale_x = 0; /* half res */
    else goto fail;
    if (dest_dy == src_dy)          *p_scale_y = 1; /* full res */
    else if (dest_dy == 2 * src_dy) *p_scale_y = 2; /* double res */
    else if (src_dx == 2 * dest_dx) *p_scale_y = 0; /* half res */
    else goto fail;

    /* success: */
    return 1;

    /* failure: */
fail:
    return 0;
}

int SetI420ChromaResamplingMode(int new_mode)
{
    return 0;
}

void InitColorData(color_data_t* pcd)
{
    pcd->color_conversion_tables_inited = 0;

    pcd->is_alpha = 0;
    pcd->is_beta  = 0;
    pcd->is_gamma = 0;
    pcd->is_kappa = 0;
    
    pcd->cur_brightness = 0;
    pcd->cur_contrast   = 0;
    pcd->cur_saturation = 0;
    pcd->cur_hue        = 0;
}
