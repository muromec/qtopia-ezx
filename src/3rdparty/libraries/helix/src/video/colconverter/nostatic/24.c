/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: 24.c,v 1.2 2007/07/06 20:53:52 jfinnecy Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include "nostatic/yuv.h"
#include "nostatic/colorlib.h"

/*
 * Color conversion routines for converting YUV420 as produced by the
 * RealVideo decoders to RGB24.
 */

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

#define BYTESPERPIXEL 4
/* Indexing tables */
static const int next[3] = {1, 2, 0};                 /* (ibuf+1)%3 table */
static const int next2[3] = {2, 0, 1};                /* (ibuf+2)%3 table */

/* Half resolution converter.  */
static void halfI420toRGB24 (unsigned char *d1,
			      int dest_x, unsigned char *sy1,
			      unsigned char *sy2, unsigned char *su,
			      unsigned char *sv, int src_x, int dx, int isodd,
			      color_data_t* pcd)
{
  /* check if we have misaligned input/output: */
  if ((src_x ^ dest_x) & 1) {

    ; /* not implemented yet */

  } else {

    /* aligned input/output: */
    register int y11, y12;
    register int rv1, guv1, bu1, rv2, guv2, bu2;
    register int i;

    /* source width should always be multiples of 4 */
    /*    assert((src_x & 3)==0); */

    /* convert all integral 2x4 blocks: */
    if (isodd)
      for (i = 0; i < dx/4; i ++) {

	/* first 2x2 block */
	rv1 = pcd->rvtab[sv[0]];
	guv1 = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
	bu1 = pcd->butab[su[0]];
	y11 = (pcd->ytab[sy1[0]] + pcd->ytab[sy1[1]] + pcd->ytab[sy2[0]] + pcd->ytab[sy2[1]])>>2; /* avg */

	/* second 2x2 block */
	rv2  = pcd->rvtab[sv[1]];
	guv2 = pcd->gutab[su[1]] + pcd->gvtab[sv[1]];
	bu2  = pcd->butab[su[1]];
	y12 = (pcd->ytab[sy1[2]] + pcd->ytab[sy1[3]] + pcd->ytab[sy2[2]] + pcd->ytab[sy2[3]])>>2;

	/* output 4 bytes at a time */
	*(unsigned int *)(d1+0) =
	  (CLIP8[y11 + bu1  + DITH8L] << 0)  |
	  (CLIP8[y11 + guv1 + DITH8L] << 11) |
	  (CLIP8[y11 + rv1  + DITH8L] << 22) /*|
	  (CLIP8[y12 + bu2  + DITH8H] << 32) |
	  (CLIP8[y12 + guv2 + DITH8H] << 43) |
	  (CLIP8[y12 + rv2  + DITH8H] << 54) */;

	/* next 4x2 block */
	sy1 += 4; sy2 += 4;
	su += 2; sv += 2;
	d1 += 2*BPP2;
      }
    else
      for (i = 0; i < dx/4; i ++) {

	/* first 2x2 block */
	rv1 = pcd->rvtab[sv[0]];
	guv1 = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
	bu1 = pcd->butab[su[0]];
	y11 = (pcd->ytab[sy1[0]] + pcd->ytab[sy1[1]] + pcd->ytab[sy2[0]] + pcd->ytab[sy2[1]])>>2; /* avg */

	/* second 2x2 block */
	rv2  = pcd->rvtab[sv[1]];
	guv2 = pcd->gutab[su[1]] + pcd->gvtab[sv[1]];
	bu2  = pcd->butab[su[1]];
	y12 = (pcd->ytab[sy1[2]] + pcd->ytab[sy1[3]] + pcd->ytab[sy2[2]] + pcd->ytab[sy2[3]])>>2;

	/* output 4 bytes at a time */
	*(unsigned int *)(d1+0) =
	  (CLIP8[y11 + bu1  + DITH8H] << 0)  |
	  (CLIP8[y11 + guv1 + DITH8H] << 11) |
	  (CLIP8[y11 + rv1  + DITH8H] << 22) /*|
	  (CLIP8[y12 + bu2  + DITH8L] << 32) |
	  (CLIP8[y12 + guv2 + DITH8L] << 43) |
	  (CLIP8[y12 + rv2  + DITH8L] << 54) */;

	/* next 4x2 block */
	sy1 += 4; sy2 += 4;
	su += 2; sv += 2;
	d1 += 2*BPP2;
      }

  }
}

static void dblineI420toRGB24 (unsigned char *d1, unsigned char *d2,
				int dest_x, unsigned char *sy1,
				unsigned char *sy2, unsigned char *su,
				unsigned char *sv, int src_x, int dx,
				color_data_t* pcd)
{
    /* check if we can use a fast 32-bit code: */
    if (!((src_x ^ dest_x) & 1)) {

        /* aligned input/output on little-endian machine: */
        register int y11, y12, y13, y14, y21, y22, y23, y24;
        register int rv, rv2, guv, guv2, bu, bu2;
        register int i;

        /* convert first 2x1 block: */
        if (dest_x & 1) {

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y21 = pcd->ytab[sy2[0]];

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

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y21 = pcd->ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];

            y12 = pcd->ytab[sy1[1]];
            y22 = pcd->ytab[sy2[1]];

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

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y12 = pcd->ytab[sy1[1]];

            /* first line BGRB */
            *(unsigned int *)(d1+0) =
                (CLIP8[y11 + bu])        |
                (CLIP8[y11 + guv] << 8)  |
                (CLIP8[y11 + rv]  << 16) |
                (CLIP8[y12 + bu]  << 24) ;

            y21 = pcd->ytab[sy2[0]];
            y22 = pcd->ytab[sy2[1]];

            /* second line BGRB */
            *(unsigned int *)(d2+0) =
                (CLIP8[y21 + bu])        |
                (CLIP8[y21 + guv] << 8)  |
                (CLIP8[y21 + rv]  << 16) |
                (CLIP8[y22 + bu]  << 24) ;

            bu2 = pcd->butab[su[1]];
            guv2 = pcd->gutab[su[1]] + pcd->gvtab[sv[1]];
            y13 = pcd->ytab[sy1[2]];

            /* first line GRBG */
            *(unsigned int *)(d1+4) =
                (CLIP8[y12 + guv])        |
                (CLIP8[y12 + rv]   << 8)  |
                (CLIP8[y13 + bu2]  << 16) |
                (CLIP8[y13 + guv2] << 24) ;

            y23 = pcd->ytab[sy2[2]];

            /* second line GRBG */
            *(unsigned int *)(d2+4) =
                (CLIP8[y22 + guv])        |
                (CLIP8[y22 + rv]   << 8)  |
                (CLIP8[y23 + bu2]  << 16) |
                (CLIP8[y23 + guv2] << 24) ;

            y14 = pcd->ytab[sy1[3]];
            rv2 = pcd->rvtab[sv[1]];

            /* first line RBGR */
            *(unsigned int *)(d1+8) =
                (CLIP8[y13 + rv2])        |
                (CLIP8[y14 + bu2]  << 8)  |
                (CLIP8[y14 + guv2] << 16) |
                (CLIP8[y14 + rv2]  << 24) ;

            y24 = pcd->ytab[sy2[3]];

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

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y21 = pcd->ytab[sy2[0]];

            /* first line BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];

            /* second line BGR: */
            d2[0] = CLIP8[y21 + bu];
            d2[1] = CLIP8[y21 + guv];
            d2[2] = CLIP8[y21 + rv];

            y12 = pcd->ytab[sy1[1]];
            y22 = pcd->ytab[sy2[1]];

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

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y21 = pcd->ytab[sy2[0]];

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

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y21 = pcd->ytab[sy2[0]];

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

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y12 = pcd->ytab[sy1[1]];

            /* first line BGR,BGR: */
            d1[0] = CLIP8[y11 + bu];
            d1[1] = CLIP8[y11 + guv];
            d1[2] = CLIP8[y11 + rv];
            d1[3] = CLIP8[y12 + bu];
            d1[4] = CLIP8[y12 + guv];
            d1[5] = CLIP8[y12 + rv];

            y21 = pcd->ytab[sy2[0]];
            y22 = pcd->ytab[sy2[1]];

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

            bu = pcd->butab[su[0]];
            guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
            rv = pcd->rvtab[sv[0]];
            y11 = pcd->ytab[sy1[0]];
            y21 = pcd->ytab[sy2[0]];

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

#define ROUND888 0x00400801

/* RGB24 version: */
static void dblineXRGBtoRGB24x2 (unsigned char *d1, unsigned char *d2, int dest_x,
				  unsigned char *s1, unsigned char *s2, int src_x, int dx,
				  color_data_t* pcd)
{
    register unsigned int a, b, c, d, e, f;
    register unsigned int w, x, y, z;
/*	FILE *RGB888;
	RGB888 = fopen("C:/RGB888.txt", "a+");

	register int red,green,blue;*/
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
            ((0 & 0x000001fe) << 23);
        *(unsigned short *)(d1+4) =     /* GR */
			((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7) ;

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
            ((0 & 0x000003fc) << 22);
        *(unsigned short *)(d2+4) =     /* GR */
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
	/*
		blue = ((w & 0x000000ff) >> 0);
		green = ((w & 0x0007f800) >> 11);
		red = ((w & 0x3fc00000) >> 22);

		fprintf(RGB888,"%d %d %d ",red, green, blue);

		blue = (((x & 0x000001fe) >> 1));
		green = ((x & 0x000ff000) >> 12);
		red = ((x & 0x7f800000) >> 23);

		fprintf(RGB888,"%d %d %d ",red, green, blue);

		blue = ((y & 0x000000ff) << 0);
		green = ((y & 0x0007f800) >> 11);
		red = ((y & 0x3fc00000) >> 22);

		fprintf(RGB888,"%d %d %d ",red, green, blue);

		blue = ((z & 0x000001fe) >> 1);
		green = ((z & 0x000ff000) >> 12);
		red = ((z & 0x7f800000) >> 23);

		fprintf(RGB888,"%d %d %d ",red, green, blue);

*/
        /* pack and store */
        *(unsigned int *)d1 =           /* brgB */
            ((w & 0x000000ff) >> 0)  |
            ((w & 0x0007f800) >> 3)  |
            ((w & 0x3fc00000) >> 6)  |
            ((0 & 0x000001fe) << 23);
        *(unsigned int *)(d1+4) =       /* GRbg */
			((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7) |
            ((0 & 0x000000ff) << 16) ;
        *(unsigned int *)(d1+8) =       /* rBGR */
			(y & 0x000000ff) |
            ((y & 0x0007f800) >> 3) |
            ((y & 0x3fc00000) >> 6) ;
		*(unsigned int *)(d1+12) =
            ((z & 0x000001fe) >> 1)  |
            ((z & 0x000ff000) >> 4)  |
            ((z & 0x7f800000) >> 7);

        /* bottom line */
        c = *(unsigned int *)s2;
        d = *(unsigned int *)(s2+BPP4);
        f = *(unsigned int *)(s2+2*BPP4);

        w = a + c + ROUND888;
        x = a + b + c + d + (ROUND888<<1);
        y = b + d + ROUND888;
        z = b + e + d + f + (ROUND888<<1);
/*
		blue = ((w & 0x000001fe) >> 1);
		green = ((w & 0x000ff000) >> 12);
		red = ((w & 0x7f800000) >> 23);

		fprintf(RGB888,"%d %d %d ",red, green, blue);

		blue = ((x & 0x000003fc) >> 2);
		green =  ((x & 0x001fe000) >> 13);
		red = ((x & 0xff000000) >> 24);

		fprintf(RGB888,"%d %d %d ",red, green, blue);

		blue =  ((y & 0x000001fe) >> 1);
		green = ((y & 0x000ff000) >> 12);
		red =  ((y & 0x7f800000) >> 23);

		fprintf(RGB888,"%d %d %d ",red, green, blue);

		blue =  ((z & 0x000003fc) >> 2);
		green = ((z & 0x001fe000) >> 13);
		red = ((z & 0xff000000) >> 24);

		fprintf(RGB888,"%d %d %d ",red, green, blue);
*/
        /* pack and store */
        *(unsigned int *)d2 =           /* bgrB */
            ((w & 0x000001fe) >> 1)  |
            ((w & 0x000ff000) >> 4)  |
            ((w & 0x7f800000) >> 7);

        *(unsigned int *)(d2+4) =       /* GRbg */
            ((x & 0x000003fc) >> 2) |
			((x & 0x001fe000) >> 5) |
            ((x & 0xff000000) >> 8);
        *(unsigned int *)(d2+8) =       /* rBGR */
			((y & 0x000001fe) >> 1) |
            ((y & 0x000ff000) >> 4) |
            ((y & 0x7f800000) >> 7) ;
	    *(unsigned int *)(d2+12) =       /* rBGR */
            ((z & 0x000003fc) >> 2)  |
            ((z & 0x001fe000) << 5)  |
            ((z & 0xff000000) >> 8);

        /* next 2x2 input block */
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
        *(unsigned int *)(d1+0) =       /* bgrB */
            ((w & 0x000000ff) >> 0)  |
            ((w & 0x0007f800) >> 3)  |
            ((w & 0x3fc00000) >> 6) ;
          
        *(unsigned int *)(d1+4) =       /* GRbg */
    		((x & 0x000001fe) >> 1) |
            ((x & 0x000ff000) >> 4) |
            ((x & 0x7f800000) >> 7) ;
          *(unsigned int *)(d1+8) =       /* rBGR */
    		((y & 0x000000ff)) |
            ((y & 0x0007f800) >> 3)  |
            ((y & 0x3fc00000) >> 6) ;
		*(unsigned int *)(d1+12) =
            ((z & 0x000001fe) >> 1)  |
            ((z & 0x000ff000) >> 4)  |
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
	/*fprintf(RGB888,"\n");
	fclose(RGB888);*/
}

/*
 * Convert two YUV lines into RGB linebufs.
 * Produces two RGB lines per call.
 * Output in padded RGB format, needed for SIMD interpolation.
 */
static void dblineI420toXRGB (unsigned char *d1, unsigned char *d2, int dest_x,
    unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
			      int src_x, int dx,
			      color_data_t* pcd)
{
    register int y1, y2, rv, guv, bu;
    register int i;

    /* convert first 2x1 block: */
    if (src_x & 1) {

        bu = pcd->butab[su[0]];
        guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
        rv = pcd->rvtab[sv[0]];
        y1 = pcd->ytab[sy1[0]];
        y2 = pcd->ytab[sy2[0]];

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

        bu = pcd->butab[su[0]];
        guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
        rv = pcd->rvtab[sv[0]];
        y1 = pcd->ytab[sy1[0]];
        y2 = pcd->ytab[sy2[0]];

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
        y1 = pcd->ytab[sy1[1]];
        y2 = pcd->ytab[sy2[1]];

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

        bu = pcd->butab[su[0]];
        guv = pcd->gutab[su[0]] + pcd->gvtab[sv[0]];
        rv = pcd->rvtab[sv[0]];
        y1 = pcd->ytab[sy1[0]];
        y2 = pcd->ytab[sy2[0]];

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

/*
 * I420->RGB24 converter:
 */
int I420toRGB24 (unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
    int src_x, int src_y, int src_dx, int src_dy,
		  color_data_t* pcd)
{
/*	FILE *BTMP;
	BTMP = fopen("C:/RGB888.txt","a+");*/
    /* scale factors: */
    int scale_x, scale_y;
	dest_pitch = dest_pitch/3*4;

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
            int src_x, int dx, color_data_t* pcd) =
            dblineI420toRGB24;

        /* local variables: */
        unsigned char *sy1, *sy2, *su, *sv, *d1, *d2;
        register int j;

        /* get pointers: */
        sy1 = src_ptr + (src_x + src_y * src_pitch);        /* luma offset */
        sy2 = sy1 + src_pitch;
        su  = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
        sv  = su + src_height * src_pitch / 4;
        d1  = dest_ptr + dest_x * 2 + dest_y * dest_pitch; /* RGB offset */
        d2  = d1 + dest_pitch;

        /* convert a top line: */
        if (src_y & 1) {                        /* chroma shift */

            /* this is a bit inefficient, but who cares??? */
            (* dbline) (d1, d1, dest_x, sy1, sy1, su, sv, src_x, src_dx, pcd);

            sy1 += src_pitch;   sy2 += src_pitch;
            su  += src_pitch/2; sv  += src_pitch/2;
            d1  += dest_pitch;  d2  += dest_pitch;
            dest_dy --;
        }

        /* convert aligned portion of the image: */
        for (j = 0; j < dest_dy/2; j ++) {

            /* convert two lines a time: */
            (* dbline) (d1, d2, dest_x, sy1, sy2, su, sv, src_x, src_dx, pcd);

            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;
            d1  += dest_pitch*2; d2 += dest_pitch*2;
        }

        /* convert bottom line (if dest_dy is odd): */
        if (dest_dy & 1) {

            /* convert one line only: */
            (* dbline) (d1, d1, dest_x, sy1, sy1, su, sv, src_x, src_dx, pcd);
        }
        return 0;
    }

    /* check if 2:1 scale: */
    if (scale_x == 2 && scale_y == 2) {
        /* color converter & interpolator to use: */
        void (*cvt) (unsigned char *d1, unsigned char *d2, int dest_x,
            unsigned char *sy1, unsigned char *sy2, unsigned char *su, unsigned char *sv,
            int src_x, int dx, color_data_t* pcd) = dblineI420toXRGB;
        void (*x2) (unsigned char *d1, unsigned char *d2, int dest_x,
            unsigned char *s1, unsigned char *s2, int src_x, int dx, color_data_t* pcd) = dblineXRGBtoRGB24x2;

        /* local variables: */
        unsigned char *sy1, *sy2, *su, *sv, *d1, *d2;
        register int dy = src_dy;

        /* line buffers (we want them to be as compact as possible): */
        int ibuf = 0;                           /* circular buffer index */
        unsigned char * buf[3];                 /* actual pointers  */
        buf[0] = pcd->linebuf;
        buf[1] = pcd->linebuf + src_dx * BPP4;
        buf[2] = pcd->linebuf + 2 * src_dx * BPP4;

        /* get pointers: */
        sy1 = src_ptr + (src_x + src_y * src_pitch);        /* luma offset */
        sy2 = sy1 + src_pitch;
        su  = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
        sv  = su + src_height * src_pitch / 4;
        d1  = dest_ptr + dest_x * BYTESPERPIXEL + dest_y * dest_pitch; /* RGB offset */
        d2  = d1 + dest_pitch;

        /* check if we have misaligned top line: */
        if (src_y & 1) {

            /* convert an odd first line: */
            (*cvt) (buf[ibuf], buf[ibuf], 0, sy1, sy1, su, sv, src_x, src_dx, pcd);
            sy1 += src_pitch;   sy2 += src_pitch;
            su  += src_pitch/2; sv  += src_pitch/2;
            dy --;

        } else {

            /* otherwise, convert first two lines: */
            (*cvt) (buf[next[ibuf]], buf[next2[ibuf]], 0, sy1, sy2, su, sv, src_x, src_dx, pcd);
            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;
            ibuf = next[ibuf];      /* skip first interpolation: */

            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx, pcd);
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
            (*cvt) (buf[next[ibuf]], buf[next2[ibuf]], 0, sy1, sy2, su, sv, src_x, src_dx, pcd);
            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;

            /* interpolate first line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx, pcd);
            d1  += dest_pitch*2; d2  += dest_pitch*2;
            ibuf = next[ibuf];

            /* interpolate second one: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx, pcd);
            d1  += dest_pitch*2; d2  += dest_pitch*2;
            ibuf = next[ibuf];
            dy -= 2;
        }

		/*fprintf(BTMP, "\n\n SIGUIENTE BITMAP\n\n");
		fclose(BTMP);
*/
        /* check the # of remaining rows: */
        if (dy & 1) {

            /* convert the last odd line: */
            (*cvt) (buf[next[ibuf]], buf[next[ibuf]], 0, sy1, sy1, su, sv, src_x, src_dx, pcd);

            /* interpolate first line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[next[ibuf]], 0, src_dx, pcd);
            d1  += dest_pitch*2; d2  += dest_pitch*2;
            ibuf = next[ibuf];

            /* replicate the last line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[ibuf], 0, src_dx, pcd);

        } else {

            /* replicate the last line: */
            (*x2) (d1, d2, dest_x, buf[ibuf], buf[ibuf], 0, src_dx, pcd);
        }
		//fclose(BTMP);
        return 0;
    }

    /* check for 1/2 X scaling */
    if (scale_x == 0 && scale_y == 0) {

        /* local variables: */
        unsigned char *sy1, *sy2, *su, *sv, *d1, *d2;
        register int j;

        /* get pointers: */
        sy1 = src_ptr + (src_x + src_y * src_pitch);        /* luma offset */
        sy2 = sy1 + src_pitch;
        su  = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch);
        sv  = su + src_height * src_pitch / 4;
        d1  = dest_ptr + dest_x * 2 + dest_y * dest_pitch; /* RGB offset */
        d2  = d1 + dest_pitch;

        /* convert aligned portion of the image: */
        for (j = 0; j < dest_dy>>1; j ++) {

            /* convert two lines a time: */
            halfI420toRGB24 (d1, dest_x, sy1, sy2, su, sv, src_x, src_dx, 0, pcd);
            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;
            d1  += dest_pitch;

            halfI420toRGB24 (d1, dest_x, sy1, sy2, su, sv, src_x, src_dx, 1, pcd);
            sy1 += src_pitch*2; sy2 += src_pitch*2;
            su  += src_pitch/2; sv  += src_pitch/2;
            d1  += dest_pitch;
        }

        return 0;
    }

    /* conversion is not supported */

    return -1;
}

/*
 * InitI420toRGB24 initializes the minimum set of clip tables necessary to perform
 * color conversion to 16 bit RGB.  The 8-bit clip table is necessary for 2x
 * interpolation.
 *
 * This routine also calls SetSrcI420Colors, which initializes color conversion tables
 * and adds color balance.
 */
void InitI420toRGB24(float brightness, float contrast, float saturation, float hue,
		      color_data_t* pcd)
{
    InitColorData(pcd);
    SetSrcI420Colors (brightness, contrast, saturation, hue, pcd);
}

void UninitI420toRGB24( ){
    /* noop */
}
