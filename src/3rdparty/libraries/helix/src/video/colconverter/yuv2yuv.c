/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: yuv2yuv.c,v 1.8 2006/02/07 21:16:00 bobclark Exp $
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

/*** #includes: ********************************************/

#include "hxassert.h"

#include "env.h"
#include "yuv.h"    /* YUV-to-RGB conversion tables & macros */
#include "clip.h"   /* macros for clipping & dithering */

#include "hxassert.h"
#include "colorlib.h"


int PackedYUVMemcpyWithPitch( unsigned char *dest_ptr,
                              int dest_width, int dest_height,
                              int dest_pitch,
                              int dest_x, int dest_y, int dest_dx, int dest_dy,
                              unsigned char *src_ptr,
                              int src_width, int src_height, int src_pitch,
                              int src_x, int src_y, int src_dx, int src_dy);

/*** YUV to YUV color converters: **************************/

/*
 * These functions were borrowed from our "old" color conversion library,
 * and they do not support things like stretching or color adjustments.
 * Given some more time this module should be completely rewritten.
 *
 ***************/

static int YUY2toPlanarYUV(unsigned char *dY, unsigned char *dU, unsigned char *dV,
                           int dest_width, int dest_height,
                           int dyPitch, int duPitch, int dvPitch,
                           int dest_x, int dest_y, int dest_dx, int dest_dy,
                           unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                           int src_x, int src_y, int src_dx, int src_dy);

static int UYVYtoPlanarYUV(unsigned char *dY, unsigned char *dU, unsigned char *dV,
                           int dest_width, int dest_height,
                           int dyPitch, int duPitch, int dvPitch,
                           int dest_x, int dest_y, int dest_dx, int dest_dy,
                           unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                           int src_x, int src_y, int src_dx, int src_dy);
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


/*** "from I420" converters: *******************************/

/*
 * I420toI420x() converter:
 */
int I420toI420x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;
	/* Chroma Shifting Allowed */
	int OddPattern;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, pY, src_width, src_height,
        yPitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have matching chroma components: */
    //if ((src_x & 1) || (src_y & 1))
    //   OddCrop= true;                          /* can't shift chromas */
	//else
	//	OddCrop = false;

    /* check if bottop-up images: */
    if (dest_pitch <= 0 || yPitch <= 0)
        return -1;                          /* not supported for this format */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustmenst: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *s, *d, *v, *s2, *v2;
            int dest_uv_offs;
            register int i,j;

            /* copy Y plane: */
            s = pY + src_x + src_y * yPitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {
                memcpy (d, s, dest_dx); /* Flawfinder: ignore */
                s += yPitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* copy Cr/Cb planes: */
            s = pU + src_x/2 + src_y/2 * uPitch;
            v = pV + src_x/2 + src_y/2 * vPitch;
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;
			// extremely cheap way to handle 1 pel cropping
			OddPattern = ((src_y & 1) << 1) | (src_x & 1);
            for (i = 0; i < dest_dy/2; i++) 
			{						
				if((i+1) < dest_dy/2) {
					s2 = s+uPitch;
					v2 = v+vPitch;
				} else {
					s2 = s;
					v2 = v;
				}
				/* convert pixels: */
				switch(OddPattern)
				{
				case(0):
					memcpy (d, s, dest_dx/2); /* Flawfinder: ignore */
					memcpy (d + dest_uv_offs, v, dest_dx/2); /* Flawfinder: ignore */
					//memcpy (d + dest_uv_offs, s + src_uv_offs, dest_dx/2);
					break;
				case(1):					
					for (j = 0; j < dest_dx/2 -1; j ++) {
						d[j] = ((unsigned int)s[j] + (unsigned int)s[j+1] + 1)>>1;
						d[j + dest_uv_offs] = ((unsigned int)v[j] + (unsigned int)v[j+1] + 1)>>1;
					}
					d[j] = s[j];
					d[j + dest_uv_offs] = v[j];
					break;
				case(2):						
					for (j = 0; j < dest_dx/2; j ++) {
						d[j] = (s[j] + s2[j] + 1)>>1;
						d[j + dest_uv_offs] = ((unsigned int)v[j] + (unsigned int)v2[j] + 1)>>1;
					}					
					break;
				case(3):
					for (j = 0; j < dest_dx/2 -1; j ++) {
						d[j] = ((unsigned int)s[j] + (unsigned int)s[j+1] + (unsigned int)s2[j] + (unsigned int)s2[j+1] + 2)>>2;
						d[j + dest_uv_offs] = ((unsigned int)v[j] + (unsigned int)v[j+1] + (unsigned int)v2[j] + (unsigned int)v2[j+1] + 2)>>2;
					}
					d[j] = ((unsigned int)s[j]+ (unsigned int)s2[j] + 1)>>1;
					d[j + dest_uv_offs] = ((unsigned int)v[j] + (unsigned int)v2[j] + 1) >> 1;
					break;
				}
                s += uPitch;
                v += vPitch;
                d += (dest_pitch/2);				
            }

        } else {

            /* adjust colors: */
            unsigned char *s, *d, *v, *s2, *v2;
            int dest_uv_offs;
            register int i, j;

            /* convert Y plane: */
            s = pY + src_x + src_y * yPitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {

                /* convert pixels: */
                for (j = 0; j < dest_dx; j ++)
                    d[j] = _yytab[s[j]];

                s += yPitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* get chroma pointers: */
            s = pU + src_x/2 + src_y/2 * uPitch;
            v = pV + src_x/2 + src_y/2 * vPitch;
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;
			OddPattern = ((src_y & 1) << 1) | (src_x & 1);
            /* check if no hue adjustment: */
            if (!is_alpha) {				
                /* no chroma rotation: */
                for (i = 0; i < dest_dy/2; i ++) {
                    if((i+1) < dest_dy/2) {
						s2 = s+uPitch;
						v2 = v+vPitch;
					} else {
						s2 = s;
						v2 = v;
					}
					/* convert pixels: */									
					switch(OddPattern)
					{
					case(0):
						for (j = 0; j < dest_dx/2; j ++) {
							d[j] = _vvtab[s[j]];
							d[j + dest_uv_offs] = _uutab[v[j]];
						}
						break;
					case(1):					
						for (j = 0; j < dest_dx/2 -1; j ++) {
							register unsigned int s1 = ((unsigned int)s[j] + (unsigned int)s[j+1] + 1)>>1;
							register unsigned int v1 = ((unsigned int)v[j] + (unsigned int)v[j+1] + 1)>>1;
							d[j] = _vvtab[s1];							
							d[j + dest_uv_offs] = _uutab[v1];
						}
						d[j] = _vvtab[s[j]];
						d[j + dest_uv_offs] = _uutab[v[j]];
						break;
					case(2):						
						for (j = 0; j < dest_dx/2; j ++) {
							register unsigned int s1 = ((unsigned int)s[j] + (unsigned int)s2[j] + 1)>>1;							
							register unsigned int v1 = ((unsigned int)v[j] + (unsigned int)v2[j] + 1)>>1;
							d[j] = _vvtab[s1];							
							d[j + dest_uv_offs] = _uutab[v1];
						}					
						break;
					case(3):
						{
						register unsigned int s1, v1;
						for (j = 0; j < dest_dx/2 -1; j ++) {
							s1 = ((unsigned int)s[j] + (unsigned int)s[j+1] + (unsigned int)s2[j] + (unsigned int)s2[j+1] + 2)>>2;
							v1 = ((unsigned int)v[j] + (unsigned int)v[j+1] + (unsigned int)v2[j] + (unsigned int)v2[j+1] + 2)>>2; 
							d[j] = _vvtab[s1];							
							d[j + dest_uv_offs] = _uutab[v1];
						}
						s1 =((unsigned int)s[j]+ (unsigned int)s2[j] + 1) >> 1;
						v1 = ((unsigned int)v[j] + (unsigned int)v2[j] + 1) >> 1;
						d[j] = _vvtab[s1];						
						d[j + dest_uv_offs] = _uutab[v1];
						}
						break;
					}
                    s += uPitch;
                    v += vPitch;
                    d += dest_pitch/2;					
                }
            } else {
                /* adjust hue: */				
                for (i = 0; i < dest_dy/2; i ++) {
					if((i+1) < dest_dy/2) {
						s2 = s+uPitch;
						v2 = v+vPitch;
					} else {
						s2 = s;
						v2 = v;
					}
                    /* convert pixels: */
                    switch(OddPattern)
					{
					case(0):
						for (j = 0; j < dest_dx/2; j ++) {
							register unsigned v1 = v[j], u1 = s[j];
							d[j] = _CLIP(8,_vvtab[v1] + _vutab[u1]);
							d[j + dest_uv_offs] = _CLIP(8,_uutab[u1] + _uvtab[v1]);
						}
						break;
					case(1):					
						{
						register unsigned s1, v1;
						for (j = 0; j < dest_dx/2 -1; j ++) {
							s1 = ((unsigned int)s[j] + (unsigned int)s[j+1] + 1)>>1;
							v1 = ((unsigned int)v[j] + (unsigned int)v[j+1] + 1)>>1;
							d[j] = _CLIP(8,_vvtab[v1] + _vutab[s1]);							
							d[j + dest_uv_offs] = _CLIP(8,_uutab[s1] + _uvtab[v1]);
						}
						v1 = v[j];
						s1 = s[j];
						d[j] = _CLIP(8,_vvtab[v1] + _vutab[s1]);
						d[j + dest_uv_offs] = _CLIP(8,_uutab[s1] + _uvtab[v1]);
						}
						break;
					case(2):						
						for (j = 0; j < dest_dx/2; j ++) {
							register unsigned s1 = ((unsigned int)s[j] + (unsigned int)s2[j] + 1)>>1;
							register unsigned v1 = ((unsigned int)v[j] + (unsigned int)v2[j] + 1)>>1;							
							d[j] = _CLIP(8,_vvtab[v1] + _vutab[s1]);							
							d[j + dest_uv_offs] = _CLIP(8,_uutab[s1] + _uvtab[v1]);
						}					
						break;
					case(3):
						{
						register unsigned s1, v1;
						for (j = 0; j < dest_dx/2 -1; j ++) {
							s1 =((unsigned int)s[j] + (unsigned int)s[j+1] + (unsigned int)s2[j] + (unsigned int)s2[j+1] + 2)>>2;
							v1 =((unsigned int)v[j] + (unsigned int)v[j+1] + (unsigned int)v2[j] + (unsigned int)v2[j+1] + 2)>>2;
							d[j] = _CLIP(8,_vvtab[v1] + _vutab[s1]);							
							d[j + dest_uv_offs] = _CLIP(8,_uutab[s1] + _uvtab[v1]);
						}
						s1 = ((unsigned int)s[j]+ (unsigned int)s2[j] + 1)>>1;
						v1 = ((unsigned int)v[j] + (unsigned int)v2[j] + 1) >> 1;
						d[j] = _CLIP(8,_vvtab[v1] + _vutab[s1]);							
						d[j + dest_uv_offs] = _CLIP(8,_uutab[s1] + _uvtab[v1]);
						}
						break;
					}

                    s += uPitch;
                    v += vPitch;
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
 * I420toI420() converter:
 */
int I420toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *pU = src_ptr+src_height*src_pitch,
                  *pV = pU + src_height*src_pitch/4;
    return I420toI420x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
                    

#else

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
                        d[j] = _CLIP(8,_vvtab[v] + _vutab[u]);
                        d[j + dest_uv_offs] = _CLIP(8,_uutab[u] + _uvtab[v]);
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
#endif
}

int I420toYV12x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, pY, src_width, src_height,
        yPitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have matching chroma components: */
    if ((src_x & 1) || (src_y & 1))
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0 || yPitch <= 0)
        return -1;                          /* not supported for this format */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustments: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *sy, *su, *sv, *d;
            int dest_uv_offs;
            register int i;

            /* copy Y plane: */
            sy = pY + src_x + src_y * yPitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {
                memcpy (d, sy, dest_dx); /* Flawfinder: ignore */
                sy += yPitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* copy Cr/Cb planes: */
            su = pU + src_x/2 + src_y/2 * uPitch;
            sv = pV + src_x/2 + src_y/2 * vPitch;
            
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;
            for (i = 0; i < dest_dy/2; i ++) {
                memcpy (d, sv, dest_dx/2); /* Flawfinder: ignore */
                memcpy (d + dest_uv_offs, su, dest_dx/2); /* Flawfinder: ignore */
                su += uPitch;
                sv += vPitch;
                d += dest_pitch/2;
            }

        } else {

            /* adjust colors: */
            unsigned char *sy, *su, *sv, *d;
            int dest_uv_offs;
            register int i, j;

            /* convert Y plane: */
            sy = pY + src_x + src_y * yPitch;
            d = dest_ptr + dest_x + dest_y * dest_pitch;
            for (i = 0; i < dest_dy; i ++) {

                /* convert pixels: */
                for (j = 0; j < dest_dx; j ++)
                    d[j] = _yytab[sy[j]];

                sy += yPitch;
                d += dest_pitch;
            }

            /* get Cr/Cb offsets: */
            dest_uv_offs = dest_height * dest_pitch / 4;

            /* get chroma pointers: */
            su = pU + src_x/2 + src_y/2 * uPitch;
            sv = pV + src_x/2 + src_y/2 * vPitch;
            d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;

            /* check if no hue adjustment: */
            if (!is_alpha) {

                /* no chroma rotation: */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
                        d[j] = _vvtab[sv[j]];
                        d[j + dest_uv_offs] = _uutab[su[j]];
                    }

                    su += uPitch;
                    sv += vPitch;
                    d += dest_pitch/2;
                }

            } else {

                /* adjust hue: */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
                        register unsigned v = sv[j], u = su[j];
                        d[j] = _CLIP(8,_vvtab[v] + _vutab[u]);
                        d[j + dest_uv_offs] = _CLIP(8,_uutab[u] + _uvtab[v]);
                    }

                    su += uPitch;
                    sv += vPitch;
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
#if 1
    unsigned char *pU = src_ptr+src_height*src_pitch,
                  *pV = pU + src_height*src_pitch/4;
    return I420toYV12x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
                    

#else
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
                        d[j] = _CLIP(8,_vvtab[v] + _vutab[u]);
                        d[j + dest_uv_offs] = _CLIP(8,_uutab[u] + _uvtab[v]);
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
#endif
}



#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS)) && !defined(WINCE_EMULATOR)

//Our MMX optimized YUY2 and UYVY to YUV converters.
//No scaling supported yet.
extern void YUY2ToPlanarYUV_MMX( unsigned char* d1,      
                                 unsigned char* d2,      
                                 unsigned char* du,      
                                 unsigned char* dv,      
                                 INT32          dyPitch, 
                                 INT32          duPitch, 
                                 INT32          dvPitch, 
                                 INT32          dest_dx, 
                                 INT32          dest_dy, 
                                 unsigned char* s1,      
                                 unsigned char* s2,      
                                 INT32          src_pitch
                                 );
extern void UYVYToPlanarYUV_MMX( unsigned char* d1,      
                                 unsigned char* d2,      
                                 unsigned char* du,      
                                 unsigned char* dv,      
                                 INT32          dyPitch, 
                                 INT32          duPitch, 
                                 INT32          dvPitch, 
                                 INT32          dest_dx, 
                                 INT32          dest_dy, 
                                 unsigned char* s1,      
                                 unsigned char* s2,      
                                 INT32          src_pitch
                                 );

extern  YUY2toUYVY_MMX( unsigned char* src,
                        unsigned char* dest,
                        ULONG32        dx
                        );

/* MMX version of I420toYUY2 converter: */
extern void _MMX_lineI420toYUY2( unsigned char *sy,
                                 unsigned char *su,
                                 unsigned char *sv,
                                 unsigned char *d,
                                 int count
                                 );

/*  MMX version of I420toUYVY converter */
extern void _MMX_lineI420toUYVY( unsigned char *sy,
                                 unsigned char *su,
                                 unsigned char *sv,
                                 unsigned char *d,
                                 int count
                                 );

#endif

#if (defined(_MACINTOSH) || defined(_MAC_UNIX)) && !defined(__i386__)
#ifdef __cplusplus
extern "C"
#endif
void
AltiVec_lineI420toYUY2 (
	unsigned char *sy, 
	unsigned char *su, 
	unsigned char *sv,
	unsigned char *d,
	int dest_dx);
#endif


/*
 * Combine 4 bytes in a register for fast transfer to
 * the main memory:
 */
#if BYTE_ORDER == LITTLE_ENDIAN
#define COMBINE(b0,b1,b2,b3)  \
    ((unsigned int)(b0) | ((unsigned int)(b1) << 8) | \
    ((unsigned int)(b2) << 16) | ((unsigned int)(b3) << 24))
#else /* BYTE_ORDER != LITTLE_ENDIAN */
#define COMBINE(b0,b1,b2,b3)  \
    ((unsigned int)(b3) | ((unsigned int)(b2) << 8) | \
    ((unsigned int)(b1) << 16) | ((unsigned int)(b0) << 24))
#endif

int I420toYUY2x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, pY, src_width, src_height,
        yPitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination columns: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x))
        return 0;

    /* check if we have misaligned input: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (yPitch <= 0 ||
        uPitch <= 0 ||
        vPitch <= 0)
        return -1;          /* not supported */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustments: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = pY + src_x + src_y * yPitch;           /* luma offset */
            su = pU + (src_x/2 + src_y/2 * uPitch);
            sv = pV + (src_x/2 + src_y/2 * vPitch);
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS))  && !defined(WINCE_EMULATOR)
            /* check if we can use MMX-optimized code here: */
            if (_x86_MMX_Available)
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    d  += dest_pitch;
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                }
                /* end of MMX code */
#if defined(_M_IX86) && !defined(WINCE_EMULATOR)                
                __asm emms
#elif defined(_USE_MMX_BLENDERS) && !defined(WINCE_EMULATOR)
               __asm__ __volatile__ ( "emms" );
#endif                    
            } else
#endif
#if (defined(_MACINTOSH) || defined(_MAC_UNIX)) && !defined(__i386__)
            /* check if we can use MMX-optimized code here: */
            if (_AltiVec_Available)
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    AltiVec_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    AltiVec_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    d  += dest_pitch;
                    AltiVec_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    AltiVec_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                }
                /* end of MMX code */
            } else
#endif
            /* a generic "C" version: */
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                    sy += yPitch;
                    d  += dest_pitch;
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                }
            }

        } else

        /* check if no hue adjustment: */
        if (!is_alpha) {

            /* no chroma rotation: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = pY + src_x + src_y * yPitch;           /* luma offset */
            su = pU + (src_x/2 + src_y/2 * uPitch);
            sv = pV + (src_x/2 + src_y/2 * vPitch);
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
                sy += yPitch;
                su += uPitch;
                sv += vPitch;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
                sy += yPitch;
                d  += dest_pitch;
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
                sy += yPitch;
                su += uPitch;
                sv += vPitch;
                d  += dest_pitch;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
            }

        } else {

            /* the most complex case (w. hue adjustment): */
            unsigned char *sy, *sv, *su, *d;
            register int i, j, u, v;

            /* get pointers: */
            sy = pY + src_x + src_y * yPitch;           /* luma offset */
            su = pU + (src_x/2 + src_y/2 * uPitch);
            sv = pV + (src_x/2 + src_y/2 * vPitch);
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], u, _yytab[sy[j*2+1]], v);
                }
                sy += yPitch;
                su += uPitch;
                sv += vPitch;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], u, _yytab[sy[j*2+1]], v);
                    *(unsigned int *)(d + j*4 + dest_pitch) = COMBINE(_yytab[sy[j*2 + yPitch]], u, _yytab[sy[j*2+1+yPitch]], v);
                }
                sy += yPitch*2;
                su += uPitch;
                sv += vPitch;
                d  += dest_pitch*2;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], u, _yytab[sy[j*2+1]], v);
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
#if 1
    unsigned char *pU = src_ptr+src_height*src_pitch,
                  *pV = pU + src_height*src_pitch/4;
    return I420toYUY2x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
                    

#else

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

#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS)) && !defined(WINCE_EMULATOR)
            /* check if we can use MMX-optimized code here: */
            if (_x86_MMX_Available)
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += src_pitch;
                    su += src_pitch/2;
                    sv += src_pitch/2;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += src_pitch;
                    d  += dest_pitch;
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                    sy += src_pitch;
                    su += src_pitch/2;
                    sv += src_pitch/2;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    _MMX_lineI420toYUY2 (sy, su, sv, d, dest_dx);
                }
                /* end of MMX code */
#if defined(_M_IX86)  && !defined(WINCE_EMULATOR)                
                __asm emms
#elif defined(_USE_MMX_BLENDERS) && !defined(WINCE_EMULATOR)
                __asm__ __volatile__ ( "emms" );
#endif                    

            } else
#endif
            /* a generic "C" version: */
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                    sy += src_pitch;
                    su += src_pitch/2;
                    sv += src_pitch/2;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                    sy += src_pitch;
                    d  += dest_pitch;
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                    sy += src_pitch;
                    su += src_pitch/2;
                    sv += src_pitch/2;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = COMBINE(sy[j*2], su[j], sy[j*2+1], sv[j]);
                }
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
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
                sy += src_pitch;
                d  += dest_pitch;
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
                sy += src_pitch;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], _uutab[su[j]], _yytab[sy[j*2+1]], _vvtab[sv[j]]);
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
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], u, _yytab[sy[j*2+1]], v);
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
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], u, _yytab[sy[j*2+1]], v);
                    *(unsigned int *)(d + j*4 + dest_pitch) = COMBINE(_yytab[sy[j*2 + src_pitch]], u, _yytab[sy[j*2+1+src_pitch]], v);
                }
                sy += src_pitch*2;
                su += src_pitch/2;
                sv += src_pitch/2;
                d  += dest_pitch*2;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = COMBINE(_yytab[sy[j*2]], u, _yytab[sy[j*2+1]], v);
                }
            }
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
#endif //0
}


int I420toUYVYx (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, pY, src_width, src_height,
        yPitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination columns: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x))
        return 0;

    /* check if we have misaligned input: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;

    if (yPitch <= 0 ||
        uPitch <= 0 ||
        vPitch <= 0)
        return -1;          /* not supported */

    /* check if 1:1 scale: */
    if (scale_x == 1 && scale_y == 1) {

        /* check if no color adjustments: */
        if (!(is_alpha | is_beta | is_gamma | is_kappa)) {

            /* no color adjustments: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = pY + src_x + src_y * yPitch;           /* luma offset */
            su = pU + (src_x/2 + src_y/2 * uPitch);
            sv = pV + (src_x/2 + src_y/2 * vPitch);
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS)) && !defined(WINCE_EMULATOR)
            /* check if we can use MMX-optimized code here: */
            if (_x86_MMX_Available)
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    d  += dest_pitch;
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                }
                /* end of MMX code */
#if defined(_M_IX86) && !defined(WINCE_EMULATOR)                
                __asm emms
#elif defined(_USE_MMX_BLENDERS) && !defined(WINCE_EMULATOR)
                __asm__ __volatile__ ( "emms" );
#endif                    
            } else
#endif
            /* a generic "C" version: */
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
                    sy += yPitch;
                    d  += dest_pitch;
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
                    sy += yPitch;
                    su += uPitch;
                    sv += vPitch;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    for (j = 0; j < dest_dx/2; j ++)
                        *(unsigned int *)(d + j*4) = su[j] | (sy[j*2] << 8) | (sv[j] << 16) | (sy[j*2+1] << 24);
                }
            }

        } else

        /* check if no hue adjustment: */
        if (!is_alpha) {

            /* no chroma rotation: */
            unsigned char *sy, *sv, *su, *d;
            register int i, j;

            /* get pointers: */
            sy = pY + src_x + src_y * yPitch;           /* luma offset */
            su = pU + (src_x/2 + src_y/2 * uPitch);
            sv = pV + (src_x/2 + src_y/2 * vPitch);
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _uutab[su[j]] | (_yytab[sy[j*2]] << 8) | (_vvtab[sv[j]] << 16) | (_yytab[sy[j*2+1]] << 24);
                sy += yPitch;
                su += uPitch;
                sv += vPitch;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _uutab[su[j]] | (_yytab[sy[j*2]] << 8) | (_vvtab[sv[j]] << 16) | (_yytab[sy[j*2+1]] << 24);
                sy += yPitch;
                d  += dest_pitch;
                for (j = 0; j < dest_dx/2; j ++)
                    *(unsigned int *)(d + j*4) = _uutab[su[j]] | (_yytab[sy[j*2]] << 8) | (_vvtab[sv[j]] << 16) | (_yytab[sy[j*2+1]] << 24);
                sy += yPitch;
                su += uPitch;
                sv += vPitch;
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
            sy = pY + src_x + src_y * yPitch;           /* luma offset */
            su = pU + (src_x/2 + src_y/2 * uPitch);
            sv = pV + (src_x/2 + src_y/2 * vPitch);
            d  = dest_ptr + dest_x * 2 + dest_y * dest_pitch;   /* 2 bytes/pixel */

            /* process top line (if chroma is not pairable): */
            if (dest_y & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = u | (_yytab[sy[j*2]] << 8) | (v << 16) | (_yytab[sy[j*2+1]] << 24);
                }
                sy += yPitch;
                su += uPitch;
                sv += vPitch;
                d  += dest_pitch;
                dest_dy --;
            }
            /* the main loop (processes two lines a time): */
            for (i = 0; i < dest_dy/2; i ++) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = u | (_yytab[sy[j*2]] << 8) | (v << 16) | (_yytab[sy[j*2+1]] << 24);
                    *(unsigned int *)(d + j*4 + dest_pitch) = u | (_yytab[sy[j*2 + yPitch]] << 8) | (v << 16) | (_yytab[sy[j*2+1 + yPitch]] << 24);
                }
                sy += yPitch*2;
                su += uPitch;
                sv += vPitch;
                d  += dest_pitch*2;
            }
            /* process the last line (if dy is odd): */
            if (dest_dy & 1) {
                for (j = 0; j < dest_dx/2; j ++) {
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
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
 * I420toUYVY() converter:
 */
int I420toUYVY (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *pU = src_ptr+src_height*src_pitch,
                  *pV = pU + src_height*src_pitch/4;
    return I420toUYVYx(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
                    

#else

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

#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS))  && !defined(WINCE_EMULATOR)
            /* check if we can use MMX-optimized code here: */
            if (_x86_MMX_Available)
            {
                /* process top line (if chroma is not pairable): */
                if (dest_y & 1) {
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                    sy += src_pitch;
                    su += src_pitch/2;
                    sv += src_pitch/2;
                    d  += dest_pitch;
                    dest_dy --;
                }
                /* the main loop (processes two lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                    sy += src_pitch;
                    d  += dest_pitch;
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                    sy += src_pitch;
                    su += src_pitch/2;
                    sv += src_pitch/2;
                    d  += dest_pitch;
                }
                /* process the last line (if dy is odd): */
                if (dest_dy & 1) {
                    _MMX_lineI420toUYVY (sy, su, sv, d, dest_dx);
                }
                /* end of MMX code */
#if defined(_M_IX86) && !defined(WINCE_EMULATOR)                
                __asm emms
#elif defined(_USE_MMX_BLENDERS) && !defined(WINCE_EMULATOR)
                __asm__ __volatile__ ( "emms" );
#endif                    
            } else
#endif
            /* a generic "C" version: */
            {
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
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
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
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
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
                    v = _CLIP(8,_vvtab[sv[j]] + _vutab[su[j]]);
                    u = _CLIP(8,_uutab[su[j]] + _uvtab[sv[j]]);
                    *(unsigned int *)(d + j*4) = u | (_yytab[sy[j*2]] << 8) | (v << 16) | (_yytab[sy[j*2+1]] << 24);
                }
            }
        }
        return 0;
    }

    /* conversion is not supported */
    return -1;
#endif //0
}


/*** "to I420" converters: *********************************/

int YV12toI420x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;
	/* Chroma Shifting Allowed */
	int OddPattern;

    /* check arguments: */
    if (!chk_args (dest_ptr, dest_width, dest_height, dest_pitch,
        dest_x, dest_y, dest_dx, dest_dy, pY, src_width, src_height,
        yPitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have matching chroma components: */
    //if ((src_x & 1) || (src_y & 1))
    //    return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0 || yPitch <= 0)
        return -1;                          /* not supported */
/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
        unsigned char *sy, *su, *sv, *d, *su2, *sv2;
        int dest_uv_offs;
        register int i,j;

        /* copy Y plane: */
        sy = pY + src_x + src_y * yPitch;
        d = dest_ptr + dest_x + dest_y * dest_pitch;
        for (i = 0; i < dest_dy; i ++) {
            memcpy (d, sy, dest_dx); /* Flawfinder: ignore */
            sy += yPitch;
            d += dest_pitch;
        }

        /* get Cr/Cb offsets: */
        dest_uv_offs = dest_height * dest_pitch / 4;

        /* copy & flip Cr/Cb planes: */
        su = pU + src_x/2 + src_y/2 * uPitch;
        sv = pV + src_x/2 + src_y/2 * vPitch;
        d = (dest_ptr + dest_height * dest_pitch) + dest_x/2 + dest_y/2 * dest_pitch/2;
		OddPattern = ((src_y & 1) << 1) | (src_x & 1);
		// extremely cheap way to handle 1 pel cropping
        for (i = 0; i < dest_dy/2; i ++) {
			if((i+1) < dest_dy/2) {
				su2 = su+uPitch;
				sv2 = sv+vPitch;
			} else {
				su2 = su;
				sv2 = sv;
			}
			/* convert pixels: */
			switch(OddPattern)
			{
			case(0):
				memcpy (d, su, dest_dx/2); /* Flawfinder: ignore */
				memcpy (d + dest_uv_offs, sv, dest_dx/2); /* Flawfinder: ignore */
				break;
			case(1):					
				for (j = 0; j < dest_dx/2 -1; j ++) {
					d[j] = ((unsigned int)su[j] + (unsigned int)su[j+1] + 1)>>1;
					d[j + dest_uv_offs] = ((unsigned int)sv[j] + (unsigned int)sv[j+1] + 1)>>1;
				}
				d[j] = su[j];
				d[j + dest_uv_offs] = sv[j];
				break;
			case(2):						
				for (j = 0; j < dest_dx/2; j ++) {
					d[j] = ((unsigned int)su[j] + (unsigned int)su2[j] + 1)>>1;
					d[j + dest_uv_offs] = ((unsigned int)sv[j] + (unsigned int)sv2[j] + 1)>>1;
				}					
				break;
			case(3):
				for (j = 0; j < dest_dx/2 -1; j ++) {
					d[j] = ((unsigned int)su[j] + (unsigned int)su[j+1] + (unsigned int)su2[j] + (unsigned int)su2[j+1] + 2)>>2;
					d[j + dest_uv_offs] = ((unsigned int)sv[j] + (unsigned int)sv[j+1] + (unsigned int)sv2[j] + (unsigned int)sv2[j+1] + 2)>>2;
				}
				d[j] = ((unsigned int)su[j]+ (unsigned int)su2[j] + 1)>>1;
				d[j + dest_uv_offs] = ((unsigned int)sv[j] + (unsigned int)sv2[j] + 1) >> 1;
				break;
			}
			su += uPitch;
			sv += vPitch;
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

int YV12toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *pV = src_ptr+src_height*src_pitch,
                  *pU = pV + src_height*src_pitch/4;

    return YV12toI420x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
#else    
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
#endif
}

int YV12toYV12x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toYV12x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       pY, pU, pV, 
                       src_width, src_height, yPitch, uPitch, vPitch,
                       src_x, src_y, src_dx, src_dy);
}

int YV12toYV12 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    unsigned char *pV = src_ptr+src_height*src_pitch,
                  *pU = pV + src_height*src_pitch/4;

    return I420toYV12x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
}

int YV12toYUY2x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toYUY2x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       pY, pU, pV, 
                       src_width, src_height, yPitch, uPitch, vPitch,
                       src_x, src_y, src_dx, src_dy);
}

int YV12toYUY2 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    unsigned char *pV = src_ptr+src_height*src_pitch,
                  *pU = pV + src_height*src_pitch/4;

    return I420toYUY2x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
}

int YV12toUYVYx (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    return I420toUYVYx(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       pY, pU, pV, 
                       src_width, src_height, yPitch, uPitch, vPitch,
                       src_x, src_y, src_dx, src_dy);
}

int YV12toUYVY (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    unsigned char *pV = src_ptr+src_height*src_pitch,
                  *pU = pV + src_height*src_pitch/4;

    return I420toUYVYx(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch/2, src_pitch/2,
                       src_x, src_y, src_dx, src_dy);
}

static void lineYVU9toI420 (unsigned char *d, unsigned char *s, int x, int dx)
{
    register int i;

    /* first pixel: */
	// extremely cheap way to handle 1 pel cropping
    switch(x&3)
	{
	case(0):
	case(1):
		break;    
	case(2):	
        d[0] = s[0];
        s += 1;
        d += 1;
        dx -= 2;
		break;
	case(3):
		//d[0] = s[0];
		s += 1;
		//d += 1;
        //dx -= 2;
		break;
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

#if 1

/*-----------------2/24/2002 2:01PM-----------------
 * fixes:
 *  - switched u and v pointers
 * --------------------------------------------------*/
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
    //if ((src_x & 1) || (src_y & 1))
    //return -1;                          /* can't shift chromas */

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
		// extremely cheap way to handle 1 pel cropping
        switch(src_y & 3) 
		{
		case(2):
		case(3):
            lineYVU9toI420 (d, s + src_uv_offs, src_x, dest_dx);   /* Cr */
            lineYVU9toI420 (d + dest_uv_offs, s, src_x, dest_dx);  /* Cb */
            s += src_pitch/4;
            d += dest_pitch/2;
            dest_dy -= 2;
			break;
		case(0):
		case(1):
			break;
        }
        /* the main loop (processes two lines a time): */
        for (j = 0; j < dest_dy/4; j ++) {
            lineYVU9toI420 (d, s + src_uv_offs, src_x, dest_dx);   /* Cr */
            memcpy (d + dest_pitch/2, d, dest_dx/2); /* Flawfinder: ignore */
            lineYVU9toI420 (d + dest_uv_offs, s, src_x, dest_dx);  /* Cb */
            memcpy (d + dest_pitch/2 + dest_uv_offs, d + dest_uv_offs, dest_dx/2); /* Flawfinder: ignore */
            s += src_pitch/4;
            d += dest_pitch;
        }
        /* bottom lines: */
        if (dest_dy & 2) {
            lineYVU9toI420 (d, s + src_uv_offs, src_x, dest_dx);   /* Cr */
            lineYVU9toI420 (d + dest_uv_offs, s, src_x, dest_dx);  /* Cb */
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}


#else

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

#endif /* YUV9 */



static int YUY2toPlanarYUV(unsigned char *dY, unsigned char *dU, unsigned char *dV,
                           int dest_width, int dest_height,
                           int dyPitch, int duPitch, int dvPitch,
                           int dest_x, int dest_y, int dest_dx, int dest_dy,
                           unsigned char *src_ptr,
                           int src_width, int src_height, int src_pitch,
                           int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;
    unsigned char *s1, *s2, *d1, *d2, *dv, *du;
    register int i, j;

    /* check arguments: */
    if (!chk_args (dY, dest_width, dest_height, dyPitch,
        dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
        src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y))
        return -1;

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have misaligned source: */
    if (src_x & 1)
        return -1; /* can't shift chromas */

    /* check if bottop-up images: */
    if (dyPitch <= 0)
        return -1;     /* not supported */
    if (src_pitch < 0)
        src_ptr -= (src_height-1) * src_pitch;

    /* just move data in, no color adjustments: */
    /* get pointers: */
    s1 = src_ptr + src_x * 2 + src_y * src_pitch;    /* 2 bytes/pixel */
    s2 = s1 + src_pitch;
    d1 = dY + dest_x + dest_y * dyPitch;   /* luma offsets  */
    d2 = d1 + dyPitch;
    
    du = dU + (dest_x/2 + dest_y/2 * duPitch);
    dv = dV + (dest_x/2 + dest_y/2 * dvPitch);

#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS)) && !defined(WINCE_EMULATOR)
    if (_x86_MMX_Available )
    {
        //Use our MMX versions.....
        HX_ASSERT( src_dx==dest_dx && src_dy==dest_dy );
        YUY2ToPlanarYUV_MMX( d1, d2, du, dv,
                             dyPitch, duPitch, dvPitch,
                             dest_dx, dest_dy, 
                             s1, s2, src_pitch
                             );
    }
    else
#endif    
    {
        /* the main loop (processes lines a time): */
        for (i = 0; i < dest_dy/2; i ++)
        {
            /* copy 2x2 pixels: */
            for (j = 0; j < dest_dx/2; j ++)
            {
                /* copy luma components: */
                d1[j*2]   = s1[j*4];
                d1[j*2+1] = s1[j*4+2];
                d2[j*2]   = s2[j*4];
                d2[j*2+1] = s2[j*4+2];
                
                /* average chromas: */
                du[j] = ((unsigned int) s1[j*4+1] + (unsigned int)s2[j*4+1]) >> 1;
                dv[j] = ((unsigned int) s1[j*4+3] + (unsigned int)s2[j*4+3]) >> 1;
            }
            
            s1 += src_pitch*2;  s2 += src_pitch*2;
            d1 += dyPitch*2;    d2 += dyPitch*2;
            du += duPitch;      dv += dvPitch;
        }
    }

    return 0;
}

int YUY2toI420x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    // No need for seperate yuv pointers since yuy2 is packed
    return YUY2toI420(dest_ptr, dest_width, dest_height,
                      dest_pitch, dest_x, dest_y, dest_dx, dest_dy,  
                      pY, src_width, src_height, yPitch,
                      src_x, src_y, src_dx, src_dy);
}

/*
 * Mac YUVU/YVU2 formats: 
 * 8-bit 4:2:2 Component Y’CbCr format. 
 * Each 16 bit pixel is represented by an unsigned eight bit luminance 
 * component and two two’s complement signed eight bit chroma components. 
 * Each pair of pixels shares a common set of chroma values. 
 * The components are ordered in memory; Y0, Cb, Y1, Cr. 
 * The luminance components have a range of [0, 255], 
 * the chroma values have a range of [-127, +127]. 
 * This format is equivalent to the ‘yuv2’ file format.
 * 
 * YUVUtoI420 converter.
 * Scale factors:
 *  Y:     256 -> 220  => 128/110 => 64/55
 *  Cr/Cb: 256 -> 224  => 128/112 => 64/56 => 32/28 => 16/14 => 8/7
 */
int YUVUtoI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;
	int OddPattern;

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
	// extremely cheap way to handle 1 pel cropping
    if (src_x & 1)
		OddPattern = 1;
	else
		OddPattern = 0;
        //return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0) return -1;     /* not supported */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;

/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
		signed	char *s1, *s2, *s3, *s4;
        unsigned char *d1, *d2, *d3, *d4, *dv, *du, *dv2, *du2;
        register int i, j;


        /* get pointers: */
        s1 = (signed char*)src_ptr + (src_x - OddPattern)* 2 + src_y * src_pitch;       /* 2 bytes/pixel */
        s2 = s1 + src_pitch;
        s3 = s2 + src_pitch;
        s4 = s3 + src_pitch;
        d1 = dest_ptr + dest_x + dest_y * dest_pitch;       /* luma offsets  */
        d2 = d1 + dest_pitch;
        d3 = d2 + dest_pitch;
        d4 = d3 + dest_pitch;
        du = dest_ptr + dest_height * dest_pitch + (dest_x/2 + dest_y/2 * dest_pitch/2);
        du2 = du + dest_pitch/2;
        dv = du + dest_height * dest_pitch/4;
        dv2 = dv + dest_pitch/2;


        /*
         * YUV 4:2:0 chroma resampling modes:
         * Input chroma     a b
         *  samples:        c d
         *                  e f
         *                  g h
         *
         * CRM_11_11:               (a+b+c+d) / 4
         * CRM_11_00:               (a+b) / 2
         * CRM_00_11:               (c+d) / 2
         * CRM_11_11_Interlace:     TF Chroma = (a+b+e+f) / 4 ; BF Chroma = (c+d+g+h) /4
         */
        switch (chroma_resampling_mode)
        {
            case CRM_11_11:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
						
						/* copy luma components: */
						if(!OddPattern)
						{

							d1[j*2]   = s1[j*4];		/* y[0,0] */
							d1[j*2+1] = s1[j*4+2];		/* y[0,1] */
							d2[j*2]   = s2[j*4];		/* y[1,0] */
							d2[j*2+1] = s2[j*4+2];		/* y[1,1] */

							/* average chromas: */
							/* here cromas are signed [-127,127]: */

							du[j] = 128 + ( (int)s1[j*4+1] + (int)s2[j*4+1] ) *7 /16;

							dv[j] = 128 + ( (int)s1[j*4+3] + (int)s2[j*4+3] ) *7 /16;

						} else {
							d1[j*2]   = s1[j*4+2];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4];      /* y[0,1] */
							d2[j*2]   = s2[j*4+2];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4];      /* y[1,1] */

							/* average chromas: */
							/* here cromas are signed: */
							/* average chromas: */
							du[j] = 128 + ((int)s1[j*4+1]   + (int)s2[j*4+1] + (int)s1[(j+1)*4+1] + (int)s2[(j+1)*4+1]) * 7/32;  /* u */
							dv[j] = 128 + ((int)s1[j*4+3] +  (int)s2[j*4+3] + (int)s1[(j+1)*4+3] + (int)s2[(j+1)*4+3]) *7/32;  /* v */

						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;

            case CRM_11_00:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        if(!OddPattern)
						{
							/* copy luma components: */
							d1[j*2]   = s1[j*4];            /* y[0,0] */
							d1[j*2+1] = s1[j*4+2];          /* y[0,1] */
							d2[j*2]   = s2[j*4];            /* y[1,0] */
							d2[j*2+1] = s2[j*4+2];          /* y[1,1] */

							/* use 1st row for chroma: */
							du[j] = 128 + ((int)s1[j*4+1])*7/8;              /* u */
							dv[j] = 128 + ((int)s1[j*4+3])*7/8;              /* v */


						} else {
							d1[j*2]   = s1[j*4+2];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4];      /* y[0,1] */
							d2[j*2]   = s2[j*4+2];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4];      /* y[1,1] */
							/* use 1st row for chroma: */
							du[j] = ((int)s1[j*4+1] + (int)s1[(j+1)*4+1]) * 7 / 16;  /* u */
							dv[j] = ((int)s1[j*4+3] + (int)s1[(j+1)*4+3]) * 7 / 16; /* v */
						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;

            case CRM_00_11:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        if(!OddPattern)
						{
							/* copy luma components: */
							d1[j*2]   = s1[j*4];            /* y[0,0] */
							d1[j*2+1] = s1[j*4+2];          /* y[0,1] */
							d2[j*2]   = s2[j*4];            /* y[1,0] */
							d2[j*2+1] = s2[j*4+2];          /* y[1,1] */

							/* use 2nd row for chroma: */
							du[j] = ((int)s2[j*4+1]) *7/8;              /* u */
							dv[j] = ((int)s2[j*4+3]) *7/8;              /* v */

						} else {
							d1[j*2]   = s1[j*4+2];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4];      /* y[0,1] */
							d2[j*2]   = s2[j*4+2];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4];      /* y[1,1] */
							/* use 2nd row for chroma: */
							du[j] = ((int)s2[j*4+1] + (int)s2[(j+1)*4+1]) *7/ 16;  /* u */
							dv[j] = ((int)s2[j*4+3] + (int)s2[(j+1)*4+3]) *7/ 16; /* v */

						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;
            case CRM_11_11_Interlace:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/4; i ++) {

                    /* copy 4x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
						
                        /* copy luma components: */
                        if(!OddPattern)
                        {

                            d1[j*2]   = s1[j*4];		/* y[0,0] */
                            d1[j*2+1] = s1[j*4+2];		/* y[0,1] */
                            d2[j*2]   = s2[j*4];		/* y[1,0] */
                            d2[j*2+1] = s2[j*4+2];		/* y[1,1] */
                            d3[j*2]   = s3[j*4];		/* y[2,0] */
                            d3[j*2+1] = s3[j*4+2];		/* y[2,1] */
                            d4[j*2]   = s4[j*4];		/* y[3,0] */
                            d4[j*2+1] = s4[j*4+2];		/* y[3,1] */

                            /* average chromas: */
                            /* here cromas are signed [-127,127]: */
                            /* Top Field */
                            du[j] = 128 + ( (int)s1[j*4+1] + (int)s3[j*4+1] ) *7 /16;
                            dv[j] = 128 + ( (int)s1[j*4+3] + (int)s3[j*4+3] ) *7 /16;
                            /* Bottom Field */
                            du2[j] = 128 + ( (int)s2[j*4+1] + (int)s4[j*4+1] ) *7 /16;
                            dv2[j] = 128 + ( (int)s2[j*4+3] + (int)s4[j*4+3] ) *7 /16;


                        } else {
                            d1[j*2]   = s1[j*4+2];          /* y[0,0] */
                            d1[j*2+1] = s1[(j+1)*4];        /* y[0,1] */
                            d2[j*2]   = s2[j*4+2];          /* y[1,0] */
                            d2[j*2+1] = s2[(j+1)*4];        /* y[1,1] */
                            d3[j*2]   = s3[j*4+2];          /* y[2,0] */
                            d3[j*2+1] = s3[(j+1)*4];        /* y[2,1] */
                            d4[j*2]   = s4[j*4+2];          /* y[3,0] */
                            d4[j*2+1] = s4[(j+1)*4];        /* y[3,1] */

                            /* average chromas: */
                            /* here cromas are signed: */
                            /* top Field */
                            du[j] = 128 + ((int)s1[j*4+1]   + (int)s3[j*4+1] + (int)s1[(j+1)*4+1] + (int)s3[(j+1)*4+1]) * 7/32;  /* u */
                            dv[j] = 128 + ((int)s1[j*4+3] +  (int)s3[j*4+3] + (int)s1[(j+1)*4+3] + (int)s3[(j+1)*4+3]) *7/32;  /* v */
                            /* Bottom Field */
                            du2[j] = 128 + ((int)s2[j*4+1]   + (int)s4[j*4+1] + (int)s2[(j+1)*4+1] + (int)s4[(j+1)*4+1]) * 7/32;  /* u */
                            dv2[j] = 128 + ((int)s2[j*4+3] +  (int)s4[j*4+3] + (int)s2[(j+1)*4+3] + (int)s4[(j+1)*4+3]) *7/32;  /* v */

                        }
                    }

                    s1 += src_pitch*4;  s2 += src_pitch*4;
                    s3 += src_pitch*4;  s4 += src_pitch*4;
                    d1 += dest_pitch*4; d2 += dest_pitch*4;
                    d3 += dest_pitch*4; d4 += dest_pitch*4;
                    du += dest_pitch; dv += dest_pitch;
                    du2 += dest_pitch; dv2 += dest_pitch;
                }
                break;
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

#if 1
/*-----------------2/24/2002 11:32AM----------------
 * things fixed and changed:
 *  - both src_pitch and dest_pitch correspond now to the widths in bytes
 *    of the correponding buffers; i.e: yuy2_pitch := yuy2_width * 2; i420_pitch := i420_luma_width;
 *  - added 3 types of chroma processing based on SetChromaResamplingMode() parameter.
 * --------------------------------------------------*/
int YUY2toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;
	int OddPattern;

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
	// extremely cheap way to handle 1 pel cropping
    if (src_x & 1)
		OddPattern = 1;
	else
		OddPattern = 0;
        //return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0) return -1;     /* not supported */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;

/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
        unsigned char *s1, *s2, *d1, *d2, *dv, *du;
        unsigned char *s3, *s4, *d3, *d4, *dv2, *du2;
        register int i, j;

        /* get pointers: */
        s1 = src_ptr + (src_x - OddPattern)* 2 + src_y * src_pitch;       /* 2 bytes/pixel */
        s2 = s1 + src_pitch;
        s3 = s2 + src_pitch;
        s4 = s3 + src_pitch;
        d1 = dest_ptr + dest_x + dest_y * dest_pitch;       /* luma offsets  */
        d2 = d1 + dest_pitch;
        d3 = d2 + dest_pitch;
        d4 = d3 + dest_pitch;
        du = dest_ptr + dest_height * dest_pitch + (dest_x/2 + dest_y/2 * dest_pitch/2);
        du2 = du + dest_pitch/2;
        dv = du + dest_height * dest_pitch/4;
        dv2 = dv + dest_pitch/2;

        /*
         * YUV 4:2:0 chroma resampling modes:
         * Input chroma     a b
         *  samples:        c d
         *                  e f
         *                  g h
         *
         * CRM_11_11:              (a+b+c+d) / 4
         * CRM_11_00:              (a+b) / 2
         * CRM_00_11:              (c+d) / 2
         * CRM_11_11_Interlace:    TF Chroma = (a+b+e+f) / 4 ; BF Chroma = (c+d+g+h) /4
         */
        switch (chroma_resampling_mode)
        {
            case CRM_11_11:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
						
						/* copy luma components: */
						if(!OddPattern)
						{
							d1[j*2]   = s1[j*4];            /* y[0,0] */
							d1[j*2+1] = s1[j*4+2];          /* y[0,1] */
							d2[j*2]   = s2[j*4];            /* y[1,0] */
							d2[j*2+1] = s2[j*4+2];          /* y[1,1] */

							/* average chromas: */
							du[j] = ((unsigned int) s1[j*4+1] + (unsigned int)s2[j*4+1]) / 2;   /* u */
							dv[j] = ((unsigned int) s1[j*4+3] + (unsigned int)s2[j*4+3]) / 2;   /* v */
						} else {
							d1[j*2]   = s1[j*4+2];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4];      /* y[0,1] */
							d2[j*2]   = s2[j*4+2];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4];      /* y[1,1] */

							/* average chromas: */
							du[j] = ((unsigned int)s1[j*4+1]   + (unsigned int)s2[j*4+1] + (unsigned int)s1[(j+1)*4+1] + (unsigned int)s2[(j+1)*4+1]) / 4;  /* u */
							dv[j] = ((unsigned int)s1[j*4+3] + (unsigned int)s2[j*4+3] + (unsigned int)s1[(j+1)*4+3] + (unsigned int)s2[(j+1)*4+3]) / 4;  /* v */
						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;

            case CRM_11_00:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        if(!OddPattern)
						{
							/* copy luma components: */
							d1[j*2]   = s1[j*4];            /* y[0,0] */
							d1[j*2+1] = s1[j*4+2];          /* y[0,1] */
							d2[j*2]   = s2[j*4];            /* y[1,0] */
							d2[j*2+1] = s2[j*4+2];          /* y[1,1] */

							/* use 1st row for chroma: */
							du[j] = s1[j*4+1];              /* u */
							dv[j] = s1[j*4+3];              /* v */
						} else {
							d1[j*2]   = s1[j*4+2];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4];      /* y[0,1] */
							d2[j*2]   = s2[j*4+2];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4];      /* y[1,1] */
							/* use 1st row for chroma: */
							du[j] = ((unsigned int)s1[j*4+1] + (unsigned int)s1[(j+1)*4+1]) / 2;  /* u */
							dv[j] = ((unsigned int)s1[j*4+3] + (unsigned int)s1[(j+1)*4+3]) / 2; /* v */
						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;

            case CRM_00_11:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        if(!OddPattern)
						{
							/* copy luma components: */
							d1[j*2]   = s1[j*4];            /* y[0,0] */
							d1[j*2+1] = s1[j*4+2];          /* y[0,1] */
							d2[j*2]   = s2[j*4];            /* y[1,0] */
							d2[j*2+1] = s2[j*4+2];          /* y[1,1] */

							/* use 2nd row for chroma: */
							du[j] = s2[j*4+1];              /* u */
							dv[j] = s2[j*4+3];              /* v */
						} else {
							d1[j*2]   = s1[j*4+2];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4];        /* y[0,1] */
							d2[j*2]   = s2[j*4+2];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4];        /* y[1,1] */
							/* use 2nd row for chroma: */
							du[j] = ((unsigned int)s2[j*4+1] + (unsigned int)s2[(j+1)*4+1]) / 2;  /* u */
							dv[j] = ((unsigned int)s2[j*4+3] + (unsigned int)s2[(j+1)*4+3]) / 2; /* v */
						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;
            case CRM_11_11_Interlace:
                /* the main loop (processes 4 lines at a time): */
                for (i = 0; i < dest_dy/4; i ++) {

                    /* copy 4x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
						
						/* copy luma components: */
                        if(!OddPattern)
                        {
                            d1[j*2]   = s1[j*4];            /* y[0,0] */
                            d1[j*2+1] = s1[j*4+2];          /* y[0,1] */
                            d2[j*2]   = s2[j*4];            /* y[1,0] */
                            d2[j*2+1] = s2[j*4+2];          /* y[1,1] */
                            d3[j*2]   = s3[j*4];            /* y[2,0] */
                            d3[j*2+1] = s3[j*4+2];          /* y[2,1] */
                            d4[j*2]   = s4[j*4];            /* y[3,0] */
                            d4[j*2+1] = s4[j*4+2];          /* y[3,1] */

							/* average chromas: */
                            /* Top Field */
                            du[j] = ((unsigned int) s1[j*4+1] + (unsigned int)s3[j*4+1]) / 2;   /* u */
                            dv[j] = ((unsigned int) s1[j*4+3] + (unsigned int)s3[j*4+3]) / 2;   /* v */
                            /* Bottom Field */
                            du2[j] = ((unsigned int) s2[j*4+1] + (unsigned int)s4[j*4+1]) / 2;   /* u */
                            dv2[j] = ((unsigned int) s2[j*4+3] + (unsigned int)s4[j*4+3]) / 2;   /* v */

                        } else {
                            d1[j*2]   = s1[j*4+2];          /* y[0,0] */
                            d1[j*2+1] = s1[(j+1)*4];        /* y[0,1] */
                            d2[j*2]   = s2[j*4+2];          /* y[1,0] */
                            d2[j*2+1] = s2[(j+1)*4];        /* y[1,1] */
                            d3[j*2]   = s3[j*4+2];          /* y[2,0] */
                            d3[j*2+1] = s3[(j+1)*4];        /* y[2,1] */
                            d4[j*2]   = s4[j*4+2];          /* y[3,0] */
                            d4[j*2+1] = s4[(j+1)*4];        /* y[3,1] */

							/* average chromas: */
                            /* Top Field */
                            du[j] = ((unsigned int)s1[j*4+1]   + (unsigned int)s3[j*4+1] + (unsigned int)s1[(j+1)*4+1] + (unsigned int)s3[(j+1)*4+1]) / 4;  /* u */
                            dv[j] = ((unsigned int)s1[j*4+3] + (unsigned int)s3[j*4+3] + (unsigned int)s1[(j+1)*4+3] + (unsigned int)s3[(j+1)*4+3]) / 4;  /* v */
                            /* Bottom Field */
                            du2[j] = ((unsigned int)s2[j*4+1]   + (unsigned int)s4[j*4+1] + (unsigned int)s2[(j+1)*4+1] + (unsigned int)s4[(j+1)*4+1]) / 4;  /* u */
                            dv2[j] = ((unsigned int)s2[j*4+3] + (unsigned int)s4[j*4+3] + (unsigned int)s2[(j+1)*4+3] + (unsigned int)s4[(j+1)*4+3]) / 4;  /* v */
                        }
                    }

                    s1 += src_pitch*4;  s2 += src_pitch*4;
                    s3 += src_pitch*4;  s4 += src_pitch*4;
                    d1 += dest_pitch*4; d2 += dest_pitch*4;
                    d3 += dest_pitch*4; d4 += dest_pitch*4;
                    du += dest_pitch; dv += dest_pitch;
                    du2 += dest_pitch; dv2 += dest_pitch;
                }
                break;
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

#else
int YUY2toI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *dU = dest_ptr+dest_height*dest_pitch,
                  *dV = dU + dest_height*dest_pitch/4;

    return YUY2toPlanarYUV(dest_ptr, dU, dV,
                           dest_width, dest_height,
                           dest_pitch, dest_pitch/2, dest_pitch/2,
                           dest_x, dest_y, dest_dx, dest_dy,
                           src_ptr, src_width, src_height, src_pitch,
                           src_x, src_y, src_dx, src_dy);
#else    
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
        s2 = s1 + src_pitch;
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
                du[j] = ((unsigned int) s1[j*4+1] + (unsigned int)s2[j*4+1]) >> 1;
                dv[j] = ((unsigned int) s1[j*4+3] + (unsigned int)s2[j*4+3]) >> 1;
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
#endif
}
#endif /* YUY2 */ 


int YUY2toYV12x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    // No need for seperate yuv pointers since yuy2 is packed
    return YUY2toYV12(dest_ptr, dest_width, dest_height,
                      dest_pitch, dest_x, dest_y, dest_dx, dest_dy,  
                      pY, src_width, src_height, yPitch,
                      src_x, src_y, src_dx, src_dy);
}

int YUY2toYV12 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    unsigned char *dV = dest_ptr+dest_height*dest_pitch,
                  *dU = dV + dest_height*dest_pitch/4;

    return YUY2toPlanarYUV(dest_ptr, dU, dV,
                           dest_width, dest_height,
                           dest_pitch, dest_pitch/2, dest_pitch/2,
                           dest_x, dest_y, dest_dx, dest_dy,
                           src_ptr, src_width, src_height, src_pitch,
                           src_x, src_y, src_dx, src_dy);
}

int YUY2toYUY2( unsigned char *dest_ptr, int dest_width, int dest_height,
                int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                int src_x, int src_y, int src_dx, int src_dy)
{
    return PackedYUVMemcpyWithPitch( dest_ptr, dest_width, dest_height,
                                     dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                                     src_ptr, src_width, src_height, src_pitch,
                                     src_x, src_y, src_dx, src_dy);
    
}
int UYVYtoUYVY( unsigned char *dest_ptr, int dest_width, int dest_height,
                int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                int src_x, int src_y, int src_dx, int src_dy)
{
    return PackedYUVMemcpyWithPitch( dest_ptr, dest_width, dest_height,
                                     dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                                     src_ptr, src_width, src_height, src_pitch,
                                     src_x, src_y, src_dx, src_dy);
        
}

int PackedYUVMemcpyWithPitch( unsigned char *dest_ptr,
                              int dest_width, int dest_height,
                              int dest_pitch,
                              int dest_x, int dest_y, int dest_dx, int dest_dy,
                              unsigned char *src_ptr,
                              int src_width, int src_height, int src_pitch,
                              int src_x, int src_y, int src_dx, int src_dy)
{
    int result = 0;
    int nLine  = 0;
    int nPixel = 0;
    int scale_x = 1;
    int scale_y = 1;
    UCHAR* pucSrc  = src_ptr;
    UCHAR* pucDest = dest_ptr;

    
    /* No stretching allowed for now*/
    HX_ASSERT( dest_dx==src_dx && dest_dy==src_dy );
    if(  dest_dx!=src_dx || dest_dy!=src_dy )
    {
        return -1;
    }

    result = chk_args( dest_ptr, dest_width, dest_height, dest_pitch,
                       dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
                       src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y);
    if( !result )
    {
        return -1;
    }

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have misaligned source: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    /* not supported right now */
    HX_ASSERT( dest_pitch>0 );
    HX_ASSERT( src_pitch>0 );

    if (dest_pitch <= 0)
        return -1;
    if (src_pitch < 0)
        return -1;

    /* Make sure src/dest is always 32 byte aligned */
    HX_ASSERT( !((ULONG32)pucSrc&0x01F) );
    HX_ASSERT( !((ULONG32)pucDest&0x01F) );

    /* Make sure pitches keep the pointers aligned to 32byte boundries */
    HX_ASSERT( src_pitch%4 == 0 );
    HX_ASSERT( dest_pitch%4 == 0 );
    
    /* Just a memcpy with pitch... */
    for( nLine=0; nLine<src_height; nLine++ )
    {
        /* Make sure src/dest is always 32 byte aligned */
        HX_ASSERT( !((ULONG32)pucSrc&0x01F) );
        HX_ASSERT( !((ULONG32)pucDest&0x01F) );
        
        memcpy( pucDest, pucSrc, src_dx*2 ); /* Flawfinder: ignore */
        pucDest += dest_pitch;
        pucSrc  += src_pitch;
    }

    return 0;
}

int UYVYtoYUY2( unsigned char *dest_ptr, int dest_width, int dest_height,
                int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                int src_x, int src_y, int src_dx, int src_dy)
{
    /* This is the exact same byte swapping memcpy with pitch.... */
    return YUY2toUYVY( dest_ptr, dest_width, dest_height,
                       dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, src_width, src_height, src_pitch,
                       src_x, src_y, src_dx, src_dy
                       );
}

int YUY2toUYVY( unsigned char *dest_ptr, int dest_width, int dest_height,
                int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                int src_x, int src_y, int src_dx, int src_dy)
{
    int result = 0;
    int nLine  = 0;
    int nPixel = 0;
    int scale_x = 1;
    int scale_y = 1;
    ULONG32* pulSrc  = (ULONG32*)src_ptr;
    ULONG32* pulDest = (ULONG32*)dest_ptr;
    ULONG32  ulSrc = 0;
    ULONG32  ulTmp1 = 0;
    ULONG32  ulTmp2 = 0;
    
    /* No stretching allowed for now*/
    HX_ASSERT( dest_dx==src_dx && dest_dy==src_dy );
    if(  dest_dx!=src_dx || dest_dy!=src_dy )
    {
        return -1;
    }

    result = chk_args( dest_ptr, dest_width, dest_height, dest_pitch,
                       dest_x, dest_y, dest_dx, dest_dy, src_ptr, src_width, src_height,
                       src_pitch, src_x, src_y, src_dx, src_dy, &scale_x, &scale_y);
    if( !result )
    {
        return -1;
    }

    /* remove odd destination pixels: */
    if (!adjust_range (&dest_x, &dest_dx, &src_x, &src_dx, scale_x) ||
        !adjust_range (&dest_y, &dest_dy, &src_y, &src_dy, scale_y))
        return 0;

    /* check if we have misaligned source: */
    if (src_x & 1)
        return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    /* not supported right now */
    HX_ASSERT( dest_pitch>0 );
    HX_ASSERT( src_pitch>0 );

    /* Make sure source and Dest are 32 byte aligned */
    HX_ASSERT( !((ULONG32)src_ptr&0x1F) );
    HX_ASSERT( !((ULONG32)dest_ptr&0x1F) );

    /* Verify pitches keep us on 32-byte boundries. */
    HX_ASSERT( src_pitch%4==0 );
    HX_ASSERT( dest_pitch%4==0 );
    
    if (dest_pitch <= 0)
        return -1;
    if (src_pitch < 0)
        return -1;
    
    /* Just a bype swap memcpy with pitch... */
    for( nLine=0; nLine<src_height; nLine++ )
    {
#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS)) && !defined(WINCE_EMULATOR)
        /* Still need to made this handle sizes that are not multiples of 4. */
        if(_x86_MMX_Available && src_dx%4==0 )
        {
            //Use our MMX versions to copy one line.
            YUY2ToUYVY_MMX( pulSrc, pulDest, src_dx );
        }
        else
#endif
        { 
            for( nPixel=0 ; nPixel<src_dx/2; nPixel++ )
            {
                ulSrc  = *(pulSrc+nPixel);
                ulTmp1 = (ulSrc&0xFF00FF00)>>8; /* Shift the Luma */
                ulTmp2 = (ulSrc&0x00FF00FF)<<8; /* Shift the Chroma */
                *(pulDest+nPixel) = (ulTmp1|ulTmp2);
            }
        }
        pulDest = (ULONG32*)((UCHAR*)pulDest + dest_pitch);
        pulSrc  = (ULONG32*)((UCHAR*)pulSrc + src_pitch);
    }
    
    return 0;
}

static int UYVYtoPlanarYUV(unsigned char *dY, unsigned char *dU, unsigned char *dV,
                           int dest_width, int dest_height,
                           int dyPitch, int duPitch, int dvPitch,
                           int dest_x, int dest_y, int dest_dx, int dest_dy,
                           unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                           int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;
    /* just move data in, no color adjustments: */
    unsigned char *s1, *s2, *d1, *d2, *dv, *du;
    register int i, j;


    /* check arguments: */
    if (!chk_args (dY, dest_width, dest_height, dyPitch,
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
    if (dyPitch <= 0) return -1;     /* not supported */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;


    /* get pointers: */
    s1 = src_ptr + src_x * 2 + src_y * src_pitch;    /* 2 bytes/pixel */
    s2 = s1 + src_pitch;
    d1 = dY + dest_x + dest_y * dyPitch;   /* luma offsets  */
    d2 = d1 + dyPitch;
    du = dU + (dest_x/2 + dest_y/2 * duPitch);
    dv = dV + (dest_x/2 + dest_y/2 * dvPitch);

#if (defined(_M_IX86) || defined(_USE_MMX_BLENDERS)) && !defined(WINCE_EMULATOR)
    if(_x86_MMX_Available )
    {
        //Use our MMX versions.....
        HX_ASSERT( src_dx==dest_dx && src_dy==dest_dy ); //No scaling supported...
        UYVYToPlanarYUV_MMX( d1, d2, du, dv,
                             dyPitch, duPitch, dvPitch,
                             dest_dx, dest_dy, 
                             s1, s2, src_pitch
                             );
    }
    else
#endif
    {
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
                du[j] = ((unsigned int)s1[j*4]   + (unsigned int)s2[j*4])   >> 1;
                dv[j] = ((unsigned int)s1[j*4+2] + (unsigned int)s2[j*4+2]) >> 1;
            }

            s1 += src_pitch*2;  s2 += src_pitch*2;
            d1 += dyPitch*2;    d2 += dyPitch*2;
            du += duPitch;      dv += dvPitch;
        }
    }
    return 0;
}

int UYVYtoI420x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    // No need for seperate yuv pointers since yuy2 is packed
    return UYVYtoI420(dest_ptr, dest_width, dest_height,
                      dest_pitch, dest_x, dest_y, dest_dx, dest_dy,  
                      pY, src_width, src_height, yPitch,
                      src_x, src_y, src_dx, src_dy);
}


#if 1
/*-----------------2/24/2002 11:32AM----------------
 * things fixed and changed:
 *  - both src_pitch and dest_pitch correspond now to the widths in bytes
 *    of the correponding buffers; i.e: yuy2_pitch := yuy2_width * 2; i420_pitch := i420_luma_width;
 *  - added 3 types of chroma processing based on SetChromaResamplingMode() parameter.
 * --------------------------------------------------*/
int UYVYtoI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    /* scale factors: */
    int scale_x, scale_y;
	int OddPattern;

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
	// extremely cheap way to handle 1 pel cropping
    if (src_x & 1)
        OddPattern = 1;
	else
		OddPattern = 0;
		//return -1;                          /* can't shift chromas */

    /* check if bottop-up images: */
    if (dest_pitch <= 0) return -1;     /* not supported */
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;

/*
 *  if (!is_dest_alpha && !is_dest_beta && !is_dest_gamma && !is_dest_kappa)
 */
    {
        /* just move data in, no color adjustments: */
        unsigned char *s1, *s2, *s3, *s4, *d1, *d2, *d3, *d4, *dv, *du, *du2, *dv2;
        register int i, j;

        /* get pointers: */
        s1 = src_ptr + (src_x - OddPattern)* 2 + src_y * src_pitch;    /* 2 bytes/pixel */
        s2 = s1 + src_pitch;
        d1 = dest_ptr + dest_x + dest_y * dest_pitch;   /* luma offsets  */
        d2 = d1 + dest_pitch;
        du = dest_ptr + dest_height * dest_pitch + (dest_x/2 + dest_y/2 * dest_pitch/2);
        dv = du + dest_height * dest_pitch/4;

        s3 = s2 + src_pitch;
        s4 = s3 + src_pitch;
        d3 = d2 + dest_pitch;
        d4 = d3 + dest_pitch;        
        du2 = du + dest_pitch/2;
        dv2 = dv + dest_pitch/2;

        /* select chroma resampling mode: */
        switch (chroma_resampling_mode)
        {
            case CRM_11_11:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {
					
                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        /* copy luma components: */
						if(!OddPattern)
						{
							d1[j*2]   = s1[j*4+1];          /* y[0,0] */
							d1[j*2+1] = s1[j*4+3];          /* y[0,1] */
							d2[j*2]   = s2[j*4+1];          /* y[1,0] */
							d2[j*2+1] = s2[j*4+3];          /* y[1,1] */
                        
							/* average chromas: */
							du[j] = ((unsigned int)s1[j*4]   + (unsigned int)s2[j*4])   / 2;    /* u */
							dv[j] = ((unsigned int)s1[j*4+2] + (unsigned int)s2[j*4+2]) / 2;    /* v */

						} else {
							d1[j*2]   = s1[j*4+3];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4+1];      /* y[0,1] */
							d2[j*2]   = s2[j*4+3];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4+1];      /* y[1,1] */

							/* average chromas: */
							du[j] = ((unsigned int)s1[j*4]   + (unsigned int)s2[j*4] + (unsigned int)s1[(j+1)*4] + (unsigned int)s2[(j+1)*4]) / 4;  /* u */
							dv[j] = ((unsigned int)s1[j*4+2] + (unsigned int)s2[j*4+2] + (unsigned int)s1[(j+1)*4+2] + (unsigned int)s2[(j+1)*4+2]) / 4;  /* v */
						}


                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;

            case CRM_11_00:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {
						if(!OddPattern)
						{
							/* copy luma components: */
							d1[j*2]   = s1[j*4+1];          /* y[0,0] */
							d1[j*2+1] = s1[j*4+3];          /* y[0,1] */
							d2[j*2]   = s2[j*4+1];          /* y[1,0] */
							d2[j*2+1] = s2[j*4+3];          /* y[1,1] */

							/* use 1st row for chroma: */
							du[j] = s1[j*4];                /* u */
							dv[j] = s1[j*4+2];              /* v */
						} else {
							d1[j*2]   = s1[j*4+3];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4+1];      /* y[0,1] */
							d2[j*2]   = s2[j*4+3];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4+1];      /* y[1,1] */
							/* use 1st row for chroma: */
							du[j] = ((unsigned int)s1[j*4] + (unsigned int)s1[(j+1)*4]) / 2;  /* u */
							dv[j] = ((unsigned int)s1[j*4+2] + (unsigned int)s1[(j+1)*4+2]) / 2; /* v */
						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;

            case CRM_00_11:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* copy 2x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        if(!OddPattern)
						{
							/* copy luma components: */
							d1[j*2]   = s1[j*4+1];          /* y[0,0] */
							d1[j*2+1] = s1[j*4+3];          /* y[0,1] */
							d2[j*2]   = s2[j*4+1];          /* y[1,0] */
							d2[j*2+1] = s2[j*4+3];          /* y[1,1] */

							/* use 2nd row for chroma: */
							du[j] = s2[j*4];                /* u */
							dv[j] = s2[j*4+2];              /* v */
						} else {
							d1[j*2]   = s1[j*4+3];          /* y[0,0] */
							d1[j*2+1] = s1[(j+1)*4+1];      /* y[0,1] */
							d2[j*2]   = s2[j*4+3];          /* y[1,0] */
							d2[j*2+1] = s2[(j+1)*4+1];      /* y[1,1] */
							/* use 2nd row for chroma: */
							du[j] = ((unsigned int)s2[j*4] + (unsigned int)s2[(j+1)*4]) / 2;  /* u */
							dv[j] = ((unsigned int)s2[j*4+2] + (unsigned int)s2[(j+1)*4+2]) / 2; /* v */
						}
                    }

                    s1 += src_pitch*2;  s2 += src_pitch*2;
                    d1 += dest_pitch*2; d2 += dest_pitch*2;
                    du += dest_pitch/2; dv += dest_pitch/2;
                }
                break;
            
            case CRM_11_11_Interlace:
                /* the main loop (processes lines a time): */
                for (i = 0; i < dest_dy/4; i ++) {
					
                    /* copy 4x2 pixels: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        /* copy luma components: */
                        if(!OddPattern)
                        {
                            d1[j*2]   = s1[j*4+1];          /* y[0,0] */
                            d1[j*2+1] = s1[j*4+3];          /* y[0,1] */
                            d2[j*2]   = s2[j*4+1];          /* y[1,0] */
                            d2[j*2+1] = s2[j*4+3];          /* y[1,1] */
                            d3[j*2]   = s3[j*4+1];          /* y[2,0] */
                            d3[j*2+1] = s3[j*4+3];          /* y[2,1] */
                            d4[j*2]   = s4[j*4+1];          /* y[3,0] */
                            d4[j*2+1] = s4[j*4+3];          /* y[3,1] */
                        
                            /* average chromas: */
                            /* Top Field */
                            du[j] = ((unsigned int)s1[j*4]   + (unsigned int)s3[j*4])   / 2;    /* u */
                            dv[j] = ((unsigned int)s1[j*4+2] + (unsigned int)s3[j*4+2]) / 2;    /* v */
                            /* Bottom Field */                            
                            du2[j] = ((unsigned int)s2[j*4]   + (unsigned int)s4[j*4])   / 2;    /* u */
                            dv2[j] = ((unsigned int)s2[j*4+2] + (unsigned int)s4[j*4+2]) / 2;    /* v */

                        } else {
                            d1[j*2]   = s1[j*4+3];          /* y[0,0] */
                            d1[j*2+1] = s1[(j+1)*4+1];      /* y[0,1] */
                            d2[j*2]   = s2[j*4+3];          /* y[1,0] */
                            d2[j*2+1] = s2[(j+1)*4+1];      /* y[1,1] */                            
                            d3[j*2]   = s3[j*4+3];          /* y[2,0] */
                            d3[j*2+1] = s3[(j+1)*4+1];      /* y[2,1] */
                            d4[j*2]   = s4[j*4+3];          /* y[3,0] */
                            d4[j*2+1] = s4[(j+1)*4+1];      /* y[3,1] */

                            /* average chromas: */
                            du[j] = ((unsigned int)s1[j*4]   + (unsigned int)s3[j*4] + (unsigned int)s1[(j+1)*4] + (unsigned int)s3[(j+1)*4]) / 4;  /* u */
                            dv[j] = ((unsigned int)s1[j*4+2] + (unsigned int)s3[j*4+2] + (unsigned int)s1[(j+1)*4+2] + (unsigned int)s3[(j+1)*4+2]) / 4;  /* v */

                            du2[j] = ((unsigned int)s2[j*4]   + (unsigned int)s4[j*4] + (unsigned int)s2[(j+1)*4] + (unsigned int)s4[(j+1)*4]) / 4;  /* u */
                            dv2[j] = ((unsigned int)s2[j*4+2] + (unsigned int)s4[j*4+2] + (unsigned int)s2[(j+1)*4+2] + (unsigned int)s4[(j+1)*4+2]) / 4;  /* v */
                        }
                    }

                    s1 += src_pitch*4;  s2 += src_pitch*4;
                    s3 += src_pitch*4;  s4 += src_pitch*4;
                    d1 += dest_pitch*4; d2 += dest_pitch*4;
                    d3 += dest_pitch*4; d4 += dest_pitch*4;
                    du += dest_pitch; dv += dest_pitch;
                    du2 += dest_pitch; dv2 += dest_pitch;
                }
                break;
        }
    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}
#else

int UYVYtoI420 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *dU = dest_ptr+dest_height*dest_pitch,
                  *dV = dU + dest_height*dest_pitch/4;

    return UYVYtoPlanarYUV(dest_ptr, dU, dV,
                           dest_width, dest_height,
                           dest_pitch, dest_pitch/2, dest_pitch/2,
                           dest_x, dest_y, dest_dx, dest_dy,
                           src_ptr, src_width, src_height, src_pitch,
                           src_x, src_y, src_dx, src_dy);

#else    
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
        s2 = s1 + src_pitch;
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
                du[j] = ((unsigned int)s1[j*4]   + (unsigned int)s2[j*4])   >> 1;
                dv[j] = ((unsigned int)s1[j*4+2] + (unsigned int)s2[j*4+2]) >> 1;
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
#endif
}
#endif

int UYVYtoYV12x (unsigned char *dest_ptr, int dest_width, int dest_height,
                 int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                 unsigned char *pY, unsigned char *pU, unsigned char *pV,
                 int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                 int src_x, int src_y, int src_dx, int src_dy)
{
    // No need for seperate yuv pointers since yuy2 is packed
    return UYVYtoYV12(dest_ptr, dest_width, dest_height,
                      dest_pitch, dest_x, dest_y, dest_dx, dest_dy,  
                      pY, src_width, src_height, yPitch,
                      src_x, src_y, src_dx, src_dy);
}


int UYVYtoYV12 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    unsigned char *dV = dest_ptr+dest_height*dest_pitch,
                  *dU = dV + dest_height*dest_pitch/4;

    return UYVYtoPlanarYUV(dest_ptr, dU, dV,
                           dest_width, dest_height,
                           dest_pitch, dest_pitch/2, dest_pitch/2,
                           dest_x, dest_y, dest_dx, dest_dy,
                           src_ptr, src_width, src_height, src_pitch,
                           src_x, src_y, src_dx, src_dy);
}

/*** from XING" converters: Will only work on Win32 with _IX86 *******************************/

#if defined(_WIN32) && defined(_M_IX86) && !defined(WINCE_EMULATOR)

int XINGtoYV12  (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *pU = src_ptr + (src_height+15 & 0xFFF0) *src_pitch,
                  *pV = pU + src_pitch/2;

    return I420toYV12x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch, src_pitch,
                       src_x, src_y, src_dx, src_dy);
#else
    unsigned char *pDestY = dest_ptr;
    unsigned char *pDestV = pDestY + (dest_pitch * src_height);
    unsigned char *pDestU = pDestY + (dest_pitch * src_height * 5 / 4);

    //XING_Opaque_Data*	pData	= (XING_Opaque_Data*)src_ptr;
    UCHAR*		pSrcY	= src_ptr;//pData->pY;
    UCHAR*		pSrcUV	= src_ptr + (src_height+15 & 0xFFF0) * src_pitch + (src_x/2 + src_y/2 * src_pitch);//pData->pUV;

    put_yuv_init(src_x, src_y, src_width, src_height, dest_width, dest_height);

    if (_x86_MMX_Available)
    {
        put_planar_mmx((DWORD)pSrcY, (DWORD)pSrcUV, (DWORD)pDestY, (DWORD)pDestU, (DWORD)pDestV, (DWORD)1, (DWORD)dest_pitch);
        
#if defined(_M_IX86) && !defined(WINCE_EMULATOR)               
                __asm emms
#elif defined(_USE_MMX_BLENDERS) && !defined(WINCE_EMULATOR)
                __asm__ __volatile__ ( "emms" );
#endif                    
    }
    else
        put_planar((DWORD) 0, (LONG) pSrcY, (LONG) pSrcUV, (DWORD)0, (DWORD)dest_ptr, (DWORD)dest_pitch);


    return 0;
#endif
}

int XINGtoYUY2 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *pU = src_ptr + (src_height+15 & 0xFFF0) *src_pitch,
                  *pV = pU + src_pitch/2;

    return I420toYUY2x(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch, src_pitch,
                       src_x, src_y, src_dx, src_dy);
#else

    unsigned char *pDestY = dest_ptr;
    unsigned char *pDestV = pDestY + (dest_pitch * src_height);
    unsigned char *pDestU = pDestY + (dest_pitch * src_height * 5 / 4);
    
    //XING_Opaque_Data*	pData	= (XING_Opaque_Data*)src_ptr;
    UCHAR*		pSrcY	= src_ptr;//pData->pY;
    UCHAR*		pSrcUV	= src_ptr + (src_height+15 & 0xFFF0) * src_pitch + (src_x/2 + src_y/2 * src_pitch);//pData->pUV;

    /* MessageBeep(-1); */

    put_yuv_init(src_x, src_y, src_width, src_height, dest_width, dest_height);
    
    if (_x86_MMX_Available)
    {
        put_yuy2_mmx((DWORD)pSrcY, (DWORD)pSrcUV, (DWORD)pDestY, (DWORD)pDestU, (DWORD)pDestV, (DWORD)1, (DWORD)dest_pitch);
#if defined(_M_IX86) && !defined(WINCE_EMULATOR)                
                __asm emms
#elif defined(_USE_MMX_BLENDERS) && !defined(WINCE_EMULATOR)
                __asm__ __volatile__ ( "emms" );
#endif                    
    }
    else
        put_yuy2((DWORD)0, (LONG) pSrcY, (LONG) pSrcUV, (DWORD)0, (DWORD)dest_ptr, (DWORD)dest_pitch);

    return 0;
#endif
}

int XINGtoUYVY  (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *pU = src_ptr + (src_height+15 & 0xFFF0) *src_pitch,
                  *pV = pU + src_pitch/2;

    return I420toUYVYx(dest_ptr, dest_width, dest_height, dest_pitch, dest_x, dest_y, dest_dx, dest_dy,
                       src_ptr, pU, pV, 
                       src_width, src_height, src_pitch, src_pitch, src_pitch,
                       src_x, src_y, src_dx, src_dy);
#else
    
    unsigned char *pDestY = dest_ptr;
    unsigned char *pDestV = pDestY + (dest_pitch * src_height);
    unsigned char *pDestU = pDestY + (dest_pitch * src_height * 5 / 4);

    //XING_Opaque_Data*	pData	= (XING_Opaque_Data*)src_ptr;
    UCHAR*		pSrcY	= src_ptr;//pData->pY;
    UCHAR*		pSrcUV	= src_ptr + (src_height+15 & 0xFFF0) * src_pitch + (src_x/2 + src_y/2 * src_pitch);//pData->pUV;

    /* MessageBox(NULL, "yuy2","",0);    */
    put_yuv_init(src_x, src_y, src_width, src_height, dest_width, dest_height);

    if (_x86_MMX_Available)
    {
        put_uyvy_mmx((DWORD)pSrcY, (DWORD)pSrcUV, (DWORD)pDestY, (DWORD)pDestU, (DWORD)pDestV, (DWORD)1, (DWORD)dest_pitch);
#if defined(_M_IX86) && !defined(WINCE_EMULATOR)                
                __asm emms
#elif defined(_USE_MMX_BLENDERS) && !defined(WINCE_EMULATOR)
                __asm__ __volatile__ ( "emms" );
#endif                    
    }
    else
        put_uyvy((DWORD)0, (LONG) pSrcY, (LONG) pSrcUV, (DWORD)0, (DWORD)dest_ptr, (DWORD)dest_pitch);
    
    return 0;
#endif
}
#else
int XINGtoYV12  (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return 0;
}

int XINGtoYUY2 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return 0;
}

int XINGtoUYVY  (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy)
{
    return 0;
}
#endif

/*
 * Test program and demo:
 */
#ifdef TEST

/* get some libraries: */
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <ddraw.h>

#include "colorlib.h"

/* internal Pentium timer: */
#define	CPU_FREQ	200000000		/* Hz */
static __int64 rdtsc ()
{
	__asm _emit 0x0f
	__asm _emit 0x31
}

/* DirectDraw objects: */
LPDIRECTDRAW 		lpDD = 0;
LPDIRECTDRAWSURFACE lpDDSPrimary = 0;
LPDIRECTDRAWSURFACE lpDDSBack = 0;

static void Exit (char *msg, int errorlevel)
{
	/* shutdown whatever is opened: */
    if (lpDDSBack)
        IDirectDrawSurface_Release (lpDDSBack);
	if (lpDDSPrimary)
        IDirectDrawSurface_Release (lpDDSPrimary);
	if (lpDD)
        IDirectDraw_Release (lpDD);

	/* print message and exit: */
	puts (msg);
	exit(errorlevel);
}

/*
 * Retrieves DirectDraw capabilities and dumps them on the screen.
 */
void main ()
{
    /* frame buffer: */
#	define	QCIF_HEIGHT	144
#	define	QCIF_WIDTH	176
    static unsigned char buf [QCIF_WIDTH * QCIF_HEIGHT * 3 / 2]; /* Flawfinder: ignore */

    /* time counters: */
	__int64 tm = 0, tm2;

	/* DirectDraw structures: */
    DDCAPS 		 	ddCaps;
    DDSURFACEDESC 	ddsd;
    DWORD           ddsCaps;
    DWORD           ddrval, ccrval;

	/* FOURCC codes: */
#   define MAXFOURCCCODES   20  /* nobody has >5; but just in case... */
    DWORD lpFourCCCodes [MAXFOURCCCODES];
    DWORD nNumFourCCCodes, n;

    /* try to load DirectDraw library: */
    if (DirectDrawCreate (NULL, &lpDD, NULL) != DD_OK)
		Exit ("Cannot create DirectDraw object!", EXIT_FAILURE);

    /* set normal cooperative level  */
    if (IDirectDraw_SetCooperativeLevel (lpDD, NULL, DDSCL_NORMAL) != DD_OK)
		Exit ("Cannot set cooperative level!", EXIT_FAILURE);

    /* get DirectDraw driver capabilities: */
    memset (&ddCaps, 0, sizeof (DDCAPS));
    ddCaps.dwSize = sizeof (DDCAPS_DX3);
    if (IDirectDraw_GetCaps(lpDD, &ddCaps, NULL) != DD_OK)
		Exit ("Cannot retrieve DirectDraw capabilities!", EXIT_FAILURE);

	/* check if hardware present: */
	if (ddCaps.dwCaps & DDCAPS_NOHARDWARE)
		Exit ("DirectDraw has no hardware support.", EXIT_SUCCESS);

    /* create primary surface */
    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (IDirectDraw_CreateSurface (lpDD, &ddsd, &lpDDSPrimary, NULL) != DD_OK)
		Exit ("Cannot create a primary surface!", EXIT_FAILURE);

    /* retrieve hardware supported FourCC codes: */
    nNumFourCCCodes = ddCaps.dwNumFourCCCodes;
    if (nNumFourCCCodes > 0	&& nNumFourCCCodes < MAXFOURCCCODES) {
        if (IDirectDraw_GetFourCCCodes (lpDD, &nNumFourCCCodes, lpFourCCCodes) != DD_OK)
			Exit ("Cannot retrieve FOURCC codes!", EXIT_FAILURE);
        /* check FOURCCs: */
		for (n=0; n<nNumFourCCCodes; n++)
            if (!memcmp(&(lpFourCCCodes[n]), "YUY2", 4))
                goto cont;

    }
    printf ("Warning: FOURCC code \"YUY2\" may not be supported.");

cont:
    /* check type of an offscreen surface we can create: */
    if (ddCaps.dwCaps & DDCAPS_OVERLAY)  ddsCaps = DDSCAPS_OVERLAY;
    else if (ddCaps.dwCaps & DDCAPS_BLT) ddsCaps = DDSCAPS_OFFSCREENPLAIN;
    else Exit ("Neither overlays or offscreen surfaces are supported.", EXIT_FAILURE);

    /* prepares surface descriptor: */
    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = ddsCaps;
    ddsd.dwWidth = QCIF_WIDTH;
    ddsd.dwHeight = QCIF_HEIGHT;
    /* set pixel format: */
    ddsd.dwFlags |= DDSD_PIXELFORMAT;
    ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    ddsd.ddpfPixelFormat.dwFourCC = (DWORD)'2YUY';
    ddsd.ddpfPixelFormat.dwRGBBitCount = 16;

    /* create an offscreen surface: */
    if (IDirectDraw_CreateSurface (lpDD, &ddsd, &lpDDSBack, NULL) != DD_OK)
        Exit ("Cannot create an offscreen surface!", EXIT_FAILURE);

	/* initialize color conversion library: */
	SetSrcI420Colors (0., 0., 0., 0.);

    /* lock an offscreen surface: */
    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize  = sizeof (ddsd);
    if ((ddrval = IDirectDrawSurface_Lock (lpDDSBack, NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)) != DD_OK) {

        /* if surface is lost, restore and try again */
        if (ddrval == DDERR_SURFACELOST) {
            IDirectDrawSurface_Restore (lpDDSPrimary);
            IDirectDrawSurface_Restore (lpDDSBack);
            ddrval = IDirectDrawSurface_Lock (lpDDSBack, NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
        }
    }
    if (ddrval != DD_OK)
        Exit ("Cannot lock an offscreen surface.", EXIT_FAILURE);

    /* fill the DirectDraw surface: */
    tm2 = rdtsc();
    ccrval = I420toYUY2 (
        ddsd.lpSurface, QCIF_WIDTH, QCIF_HEIGHT, ddsd.lPitch, 0, 0, QCIF_WIDTH, QCIF_HEIGHT,
        buf, QCIF_WIDTH, QCIF_HEIGHT, QCIF_WIDTH, 0, 0, QCIF_WIDTH, QCIF_HEIGHT);
    tm += rdtsc() - tm2;

    /* unlock the surface: */
    IDirectDrawSurface_Unlock (lpDDSBack, NULL);

    /* check error code from color converter: */
    if (ccrval)
        Exit ("Color converter has failed.", EXIT_FAILURE);

    /* dump statistics: */
    printf ("Conversion took: %g sec (%d clocks overall, %d clocks per pixel)\n",
        (double)tm /CPU_FREQ, (unsigned int)tm,
        (unsigned int) (tm / (QCIF_WIDTH * QCIF_HEIGHT)));

	/* done */
    Exit ("All done\n", EXIT_SUCCESS);
}

#endif /* TEST */

/* yuv2yuv.c -- end of file */
