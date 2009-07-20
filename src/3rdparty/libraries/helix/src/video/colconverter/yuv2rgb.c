/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: yuv2rgb.c,v 1.5 2007/07/06 20:53:51 jfinnecy Exp $
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

/*** #includes: ********************************************/

#include "env.h"
#include "rgb.h"    /* basic RGB-data definitions & macros */
#include "yuv.h"    /* YUV-to-RGB conversion tables & macros */
#include "clip.h"   /* macros for clipping & dithering */
#include "scale.h"  /* scale algorithms */
#include "colorlib.h" /* ensure that prototypes get extern C'ed */

#ifdef _MACINTOSH
#pragma require_prototypes off
#endif

static int YUVtoRGB2 (
    int dest_format,
    unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *pY, unsigned char *pU, unsigned char *pV,
    int src_width, int src_height, int yPitch, int uPitch, int vPitch,
    int src_x, int src_y, int src_dx, int src_dy);

/*** Additional pixel-level macros: ************************/

/*
 * Add dither, clip and assign values to RGB pixels:
 */
#define RGBX_CLIP_X(f,rnd,x,v)  (CLIP(rnd,BITS(f,x),v) << START(f,x))
#define RGBX_CLIP_SET(f,rnd,a,r,g,b) \
    a##_rgb = RGBX_CLIP_X(f,rnd,R,r) | RGBX_CLIP_X(f,rnd,G,g) | RGBX_CLIP_X(f,rnd,B,b)
#define RGB32_CLIP_SET(rnd,a,r,g,b)  RGBX_CLIP_SET(RGB32,rnd,a,r,g,b)
#define BGR32_CLIP_SET(rnd,a,r,g,b)  RGBX_CLIP_SET(BGR32,rnd,a,r,g,b)
#define RGB24_CLIP_SET(rnd,a,r,g,b)  \
    a##_b = CLIP(rnd,8,b), a##_g = CLIP(rnd,8,g), a##_r = CLIP(rnd,8,r)
#define RGB565_CLIP_SET(rnd,a,r,g,b) RGBX_CLIP_SET(RGB565,rnd,a,r,g,b)
#define RGB555_CLIP_SET(rnd,a,r,g,b) RGBX_CLIP_SET(RGB555,rnd,a,r,g,b)
#define RGB444_CLIP_SET(rnd,a,r,g,b) RGBX_CLIP_SET(RGB444,rnd,a,r,g,b)
#define RGB8_CLIP_SET(rnd,a,r,g,b)   \
    a##_idx = pmap[(CLIP(rnd,4,r)<<8) | (CLIP(rnd,4,g)<<4) | CLIP(rnd,4,b)]

/*
 * Generic RGB clipping & assignment macro:
 */
#define CLIP_SET(f,rnd,a,r,g,b)      f##_CLIP_SET(rnd,a,r,g,b)

/*
 * YUV 2x1-block load and convert macros:
 */
#define YUV_LOAD_CONVERT_2x1_FAST(df,a1,a2,sy1,sy2,su,sv)   \
    {                                                       \
        register int y1, y2, rv, guv, bu;                   \
        bu = butab[su[0]];                                  \
        guv = gutab[su[0]] + gvtab[sv[0]];                  \
        rv = rvtab[sv[0]];                                  \
        y1 = ytab[sy1[0]];                                  \
        y2 = ytab[sy2[0]];                                  \
        CLIP_SET(df,ROUND,a1,y1+rv,y1+guv,y1+bu);           \
        CLIP_SET(df,ROUND,a2,y2+rv,y2+guv,y2+bu);           \
    }

/* with Hue rotation: */
#define YUV_LOAD_CONVERT_2x1_FULL(df,a1,a2,sy1,sy2,su,sv)   \
    {                                                       \
        register int y1, y2, ruv, guv, buv;                 \
        buv = butab[su[0]] + bvtab[sv[0]];                  \
        guv = gutab[su[0]] + gvtab[sv[0]];                  \
        ruv = rutab[su[0]] + rvtab[sv[0]];                  \
        y1 = ytab[sy1[0]];                                  \
        y2 = ytab[sy2[0]];                                  \
        CLIP_SET(df,ROUND,a1,y1+ruv,y1+guv,y1+buv);         \
        CLIP_SET(df,ROUND,a2,y2+ruv,y2+guv,y2+buv);         \
    }

/*
 * Generic YUV 2x1-block load & convert macro:
 */
#define YUV_LOAD_CONVERT_2x1(cc,df,a1,a2,sy1,sy2,su,sv)  \
    YUV_LOAD_CONVERT_2x1_##cc(df,a1,a2,sy1,sy2,su,sv)

/*
 * YUV 2x2-block load and convert macros:
 * (without dithering)
 */
#define YUV_LOAD_CONVERT_2x2_FAST(df,a11,a12,a21,a22,sy1,sy2,su,sv) \
    {                                                       \
        register int y11, y12, y21, y22, rv, guv, bu;       \
        bu = butab[su[0]];                                  \
        guv = gutab[su[0]] + gvtab[sv[0]];                  \
        rv = rvtab[sv[0]];                                  \
        y11 = ytab[sy1[0]];                                 \
        y21 = ytab[sy2[0]];                                 \
        y12 = ytab[sy1[1]];                                 \
        y22 = ytab[sy2[1]];                                 \
        CLIP_SET(df,ROUND,a11,y11+rv,y11+guv,y11+bu);       \
        CLIP_SET(df,ROUND,a21,y21+rv,y21+guv,y21+bu);       \
        CLIP_SET(df,ROUND,a12,y12+rv,y12+guv,y12+bu);       \
        CLIP_SET(df,ROUND,a22,y22+rv,y22+guv,y22+bu);       \
    }

/* with Hue rotation: */
#define YUV_LOAD_CONVERT_2x2_FULL(df,a11,a12,a21,a22,sy1,sy2,su,sv) \
    {                                                       \
        register int y11, y12, y21, y22, ruv, guv, buv;     \
        buv = butab[su[0]] + bvtab[sv[0]];                  \
        guv = gutab[su[0]] + gvtab[sv[0]];                  \
        ruv = rutab[su[0]] + rvtab[sv[0]];                  \
        y11 = ytab[sy1[0]];                                 \
        y21 = ytab[sy2[0]];                                 \
        y12 = ytab[sy1[1]];                                 \
        y22 = ytab[sy2[1]];                                 \
        CLIP_SET(df,ROUND,a11,y11+ruv,y11+guv,y11+buv);     \
        CLIP_SET(df,ROUND,a21,y21+ruv,y21+guv,y21+buv);     \
        CLIP_SET(df,ROUND,a12,y12+ruv,y12+guv,y12+buv);     \
        CLIP_SET(df,ROUND,a22,y22+ruv,y22+guv,y22+buv);     \
    }

/*
 * Generic YUV 2x1-block load & convert macro:
 */
#define YUV_LOAD_CONVERT_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv) \
    YUV_LOAD_CONVERT_2x2_##cc(df,a11,a12,a21,a22,sy1,sy2,su,sv)


/*
 * YUV 2x2-block load and convert macros:
 *  (adds symmetric 2x2 dither noise)
 */
#define YUV_LOAD_CONVERT_DITHER_2x2_FAST(df,a11,a12,a21,a22,sy1,sy2,su,sv) \
    {                                                       \
        register int y11, y12, y21, y22, rv, guv, bu;       \
        bu = butab[su[0]];                                  \
        guv = gutab[su[0]] + gvtab[sv[0]];                  \
        rv = rvtab[sv[0]];                                  \
        y11 = ytab[sy1[0]];                                 \
        y21 = ytab[sy2[0]];                                 \
        y12 = ytab[sy1[1]];                                 \
        y22 = ytab[sy2[1]];                                 \
        CLIP_SET(df,HIGH,a11,y11+rv,y11+guv,y11+bu);        \
        CLIP_SET(df,LOW ,a21,y21+rv,y21+guv,y21+bu);        \
        CLIP_SET(df,LOW ,a12,y12+rv,y12+guv,y12+bu);        \
        CLIP_SET(df,HIGH,a22,y22+rv,y22+guv,y22+bu);        \
    }

/* with Hue rotation: */
#define YUV_LOAD_CONVERT_DITHER_2x2_FULL(df,a11,a12,a21,a22,sy1,sy2,su,sv) \
    {                                                       \
        register int y11, y12, y21, y22, ruv, guv, buv;     \
        buv = butab[su[0]] + bvtab[sv[0]];                  \
        guv = gutab[su[0]] + gvtab[sv[0]];                  \
        ruv = rutab[su[0]] + rvtab[sv[0]];                  \
        y11 = ytab[sy1[0]];                                 \
        y21 = ytab[sy2[0]];                                 \
        y12 = ytab[sy1[1]];                                 \
        y22 = ytab[sy2[1]];                                 \
        CLIP_SET(df,HIGH,a11,y11+ruv,y11+guv,y11+buv);      \
        CLIP_SET(df,LOW ,a21,y21+ruv,y21+guv,y21+buv);      \
        CLIP_SET(df,LOW ,a12,y12+ruv,y12+guv,y12+buv);      \
        CLIP_SET(df,HIGH,a22,y22+ruv,y22+guv,y22+buv);      \
    }

/*
 * Generic YUV 2x1-block load & convert macro:
 */
#define YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv) \
    YUV_LOAD_CONVERT_DITHER_2x2_##cc(df,a11,a12,a21,a22,sy1,sy2,su,sv)


/*
 * Generic YUV load-convert-store macros:
 */
#define YUV_LOAD_CONVERT_STORE_2x1(cc,df,d1,d2,sy1,sy2,su,sv) \
    {                                                       \
        PIXEL(df,a1); PIXEL(df,a2);                         \
        YUV_LOAD_CONVERT_2x1(cc,df,a1,a2,sy1,sy2,su,sv);    \
        sy1++; sy2++; su++; sv++;                           \
        STORE(df,d1,a1);                                    \
        d1+=BPP(df);                                        \
        STORE(df,d2,a2);                                    \
        d2+=BPP(df);                                        \
    }

#define YUV_LOAD_CONVERT_STORE_2x2(cc,df,d1,d2,sy1,sy2,su,sv) \
    {                                                       \
        PIXEL(df,a11); PIXEL(df,a12);                       \
        PIXEL(df,a21); PIXEL(df,a22);                       \
        YUV_LOAD_CONVERT_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
        sy1+=2; sy2+=2; su++; sv++;                         \
        STORE(df,d1,a11);                                   \
        STORE(df,d1+BPP(df),a12);                           \
        d1+=2*BPP(df);                                      \
        STORE(df,d2,a21);                                   \
        STORE(df,d2+BPP(df),a22);                           \
        d2+=2*BPP(df);                                      \
    }

#define YUV_LOAD_CONVERT_DITHER_STORE_2x2(cc,df,d1,d2,sy1,sy2,su,sv) \
    {                                                       \
        PIXEL(df,a11); PIXEL(df,a12);                       \
        PIXEL(df,a21); PIXEL(df,a22);                       \
        YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
        sy1+=2; sy2+=2; su++; sv++;                         \
        STORE(df,d1,a11);                                   \
        STORE(df,d1+BPP(df),a12);                           \
        d1+=2*BPP(df);                                      \
        STORE(df,d2,a21);                                   \
        STORE(df,d2+BPP(df),a22);                           \
        d2+=2*BPP(df);                                      \
    }

/*
 * Generic YUV load-convert-average-store macros:
 *  [d1],[d2] = convert([s1],[s2]);
 *  [d01] = ([d0]+[d1])/2;
 *  [d12] = ([d1]+[d2])/2;
 */
#define YUV_LOAD_CONVERT_AVERAGE_STORE_2x1(cc,df,d0,d01,d1,d12,d2,sy1,sy2,su,sv) \
    {                                                       \
        PIXEL(df,a1); PIXEL(df,a2);                         \
        YUV_LOAD_CONVERT_2x1(cc,df,a1,a2,sy1,sy2,su,sv);    \
        sy1++; sy2++; su++; sv++;                           \
        STORE(df,d1,a1);                                    \
        d1+=BPP(df);                                        \
        STORE(df,d2,a2);                                    \
        d2+=BPP(df);                                        \
        AVERAGE(df,a2,a1,a2);                               \
        LOAD_AVERAGE(df,a1,a1,d0);                          \
        d0+=BPP(df);                                        \
        STORE(df,d01,a1);                                   \
        d01+=BPP(df);                                       \
        STORE(df,d12,a2);                                   \
        d12+=BPP(df);                                       \
    }

#define YUV_LOAD_CONVERT_AVERAGE_STORE_2x2(cc,df,d0,d01,d1,d12,d2,sy1,sy2,su,sv) \
    {                                                       \
        PIXEL(df,a11); PIXEL(df,a12);                       \
        PIXEL(df,a21); PIXEL(df,a22);                       \
        YUV_LOAD_CONVERT_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
        sy1+=2; sy2+=2; su++; sv++;                         \
        STORE(df,d1,a11);                                   \
        STORE(df,d1+BPP(df),a12);                           \
        d1+=2*BPP(df);                                      \
        STORE(df,d2,a21);                                   \
        STORE(df,d2+BPP(df),a22);                           \
        d2+=2*BPP(df);                                      \
        AVERAGE(df,a21,a11,a21);                            \
        AVERAGE(df,a22,a12,a22);                            \
        LOAD_AVERAGE(df,a11,a11,d0);                        \
        LOAD_AVERAGE(df,a12,a12,d0+BPP(df));                \
        d0+=2*BPP(df);                                      \
        STORE(df,d01,a11);                                  \
        STORE(df,d01+BPP(df),a12);                          \
        d01+=2*BPP(df);                                     \
        STORE(df,d12,a21);                                  \
        STORE(df,d12+BPP(df),a22);                          \
        d12+=2*BPP(df);                                     \
    }

#define YUV_LOAD_CONVERT_AVERAGE_DITHER_STORE_2x2(cc,df,d0,d01,d1,d12,d2,sy1,sy2,su,sv) \
    {                                                       \
        PIXEL(df,a11); PIXEL(df,a12);                       \
        PIXEL(df,a21); PIXEL(df,a22);                       \
        YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
        sy1+=2; sy2+=2; su++; sv++;                         \
        STORE(df,d1,a11);                                   \
        STORE(df,d1+BPP(df),a12);                           \
        d1+=2*BPP(df);                                      \
        STORE(df,d2,a21);                                   \
        STORE(df,d2+BPP(df),a22);                           \
        d2+=2*BPP(df);                                      \
        AVERAGE(df,a21,a11,a21);                            \
        AVERAGE(df,a22,a12,a22);                            \
        LOAD_AVERAGE(df,a11,a11,d0);                        \
        LOAD_AVERAGE(df,a12,a12,d0+BPP(df));                \
        d0+=2*BPP(df);                                      \
        STORE(df,d01,a11);                                  \
        STORE(df,d01+BPP(df),a12);                          \
        d01+=2*BPP(df);                                     \
        STORE(df,d12,a21);                                  \
        STORE(df,d12+BPP(df),a22);                          \
        d12+=2*BPP(df);                                     \
    }

/*** Generic YUVtoRGB double-row converters: ***************/

/*
 * Generic YUVtoRGB double-row shrinking converter:
 *  uses read-ahead optimization to process full 2x2 blocks
 *  whenever possible.
 */
#define DBLROW_SHRINK(cc,df,d1,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = dest_dx;                       \
        register int limit = src_dx >> 1; /* -1 */          \
        register int step = dest_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            /* check if we have an odd first block: */      \
            if (src_x & 1)                                  \
                goto start_odd;                             \
            /* process even pixels: */                      \
            do {                                            \
                PIXEL(df,a11); PIXEL(df,a12);               \
                PIXEL(df,a21); PIXEL(df,a22);               \
                /* make one Bresenham step ahead: */        \
                if ((limit -= step) < 0) {                  \
                    limit += src_dx;                        \
                    /* can we process 2x2 pixels? */        \
                    if (!--count)                           \
                        goto last_pixel;                    \
                    /* process full 2x2 block: */           \
                    YUV_LOAD_CONVERT_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                    sy1+=2; sy2+=2; su++; sv++;             \
                    STORE(df,d1,a11);                       \
                    STORE(df,d1+BPP(df),a12);               \
                    d1+=2*BPP(df);                          \
                    STORE(df,d2,a21);                       \
                    STORE(df,d2+BPP(df),a22);               \
                    d2+=2*BPP(df);                          \
                } else {                                    \
                    /* proc. first 2x1 block & skip next: */\
                    YUV_LOAD_CONVERT_2x1(cc,df,a11,a21,sy1,sy2,su,sv); \
                    sy1+=2; sy2+=2; su++; sv++;             \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                }                                           \
                /* inverted Bresenham stepping: */          \
                while ((limit -= step) >= 0) {              \
                    /* skip next even source pixel: */      \
                    sy1++; sy2++;                           \
                    if ((limit -= step) < 0)                \
                        goto cont_odd;                      \
                    /* skip odd source pixel: */            \
                    sy1++; sy2++;                           \
                    su++; sv++; /* next chroma: */          \
                }                                           \
cont_even:      /* continue loop with next even pixel: */   \
                limit += src_dx;                            \
            } while (--count);                              \
            goto done;                                      \
last_pixel: /* use this branch to process last pixel:*/     \
            count++;                                        \
start_odd:  /* process odd pixels: */                       \
            do {                                            \
                PIXEL(df,a11); PIXEL(df,a21);               \
                YUV_LOAD_CONVERT_2x1(cc,df,a11,a21,sy1,sy2,su,sv); \
                STORE(df,d1,a11);                           \
                d1+=BPP(df);                                \
                STORE(df,d2,a21);                           \
                d2+=BPP(df);                                \
                /* inverted Bresenham stepping: */          \
                do {                                        \
                    /* skip odd source pixel: */            \
                    sy1++; sy2++;                           \
                    su++; sv++; /* next chroma: */          \
                    if ((limit -= step) < 0)                \
                        goto cont_even;                     \
                    /* skip even source pixel: */           \
                    sy1++; sy2++;                           \
                } while ((limit -= step) >= 0);             \
cont_odd:       limit += src_dx;                            \
            } while (--count);                              \
done:       ;                                               \
        }                                                   \
    }

/*
 * Generic YUVtoRGB double-row copy converter:
 */
#define DBLROW_COPY(cc,df,d1,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        register int count = dest_dx;                       \
        /* convert first 2x1 block: */                      \
        if ((src_x & 1) && count) {                         \
            YUV_LOAD_CONVERT_STORE_2x1(cc,df,d1,d2,sy1,sy2,su,sv); \
            count--;                                        \
        }                                                   \
        /* convert all integral 2x2 blocks: */              \
        while (count >= 2) {                                \
            YUV_LOAD_CONVERT_DITHER_STORE_2x2(cc,df,d1,d2,sy1,sy2,su,sv); \
            count -= 2;                                     \
        }                                                   \
        /* convert last 2x1 block: */                       \
        if (count) {                                        \
            YUV_LOAD_CONVERT_STORE_2x1(cc,df,d1,d2,sy1,sy2,su,sv); \
        }                                                   \
    }


/*
 * Generic YUVtoRGB double row stretching converter:
 */
#define DBLROW_STRETCH(cc,df,d1,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx;                         \
        /* # of pixels to be processed separately: */       \
        int remainder = dest_dx - limit;                    \
        if ((src_x + src_dx) & 1) remainder += dest_dx;     \
        remainder /= step;                                  \
        /* check row length: */                             \
        if (count) {                                        \
            PIXEL(df,a11); PIXEL(df,a12);                   \
            PIXEL(df,a21); PIXEL(df,a22);                   \
            /* update count: */                             \
            if ((count -= remainder) <= 0)                  \
                goto convert_last;                          \
            /* check if we have an odd first block: */      \
            if (src_x & 1) {                                \
                /* convert first 2x1 block: */              \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                sy1++; sy2++; su++; sv++;                   \
                goto rep_odd;                               \
            }                                               \
            /* the main loop: */                            \
            while (1) {                                     \
                /* load & convert next 2x2 pixels: */       \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* replicate even pixels: */                \
                do {                                        \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;                      \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
rep_odd:        /* replicate odd pixels: */                 \
                do {                                        \
                    STORE(df,d1,a12);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a22);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto check_last;                    \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
            }                                               \
check_last: /* check if we need to convert one more pixel:*/\
            if ((src_x + src_dx) & 1) {                     \
convert_last:   /* last 2x1 block: */                       \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
            }                                               \
            /* restore the number of remaining pixels: */   \
rep_last:   count += remainder;                             \
            while (count --) {                              \
                /* replicate them: */                       \
                STORE(df,d1,a12);                           \
                d1+=BPP(df);                                \
                STORE(df,d2,a22);                           \
                d2+=BPP(df);                                \
            }                                               \
        }                                                   \
    }

/*
 * Generic row 2x-stretching converter:
 */
#define DBLROW_STRETCH2X(cc,df,d1,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = src_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            PIXEL(df,a11); PIXEL(df,a12);                   \
            PIXEL(df,a21); PIXEL(df,a22);                   \
            /* check if we have an odd or single pixel: */  \
            if ((src_x & 1) || count < 2) {                 \
                /* process first 2x1 block: */              \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                sy1++; sy2++; su++; sv++;                   \
                STORE(df,d1,a12);                           \
                STORE(df,d2,a22);                           \
                d1 += BPP(df);                              \
                d2 += BPP(df);                              \
                count -= 1;                                 \
            } else {                                        \
                /* process first 2x2 block: */              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                STORE(df,d1,a11);                           \
                STORE(df,d2,a21);                           \
                /* calculate & store half-pixels: */        \
                AVERAGE(df,a11,a11,a12);                    \
                AVERAGE(df,a21,a21,a22);                    \
                STORE(df,d1+BPP(df),a11);                   \
                STORE(df,d1+2*BPP(df),a12);                 \
                STORE(df,d2+BPP(df),a21);                   \
                STORE(df,d2+2*BPP(df),a22);                 \
                d1 += 3*BPP(df);                            \
                d2 += 3*BPP(df);                            \
                count -= 2;                                 \
            }                                               \
            /* process all internal 4x2 blocks: */          \
            while (count >= 4) {                            \
                /* process second 2x2 block: */             \
                PIXEL(df,a13); PIXEL(df,a23);               \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a13,a21,a23,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* calculate & store first half-pixels: */  \
                AVERAGE(df,a12,a12,a11);                    \
                AVERAGE(df,a22,a22,a21);                    \
                STORE(df,d1+0*BPP(df),a12);                 \
                STORE(df,d1+1*BPP(df),a11);                 \
                STORE(df,d2+0*BPP(df),a22);                 \
                STORE(df,d2+1*BPP(df),a21);                 \
                /* calculate & store second half-pixels: */ \
                AVERAGE(df,a11,a11,a13);                    \
                AVERAGE(df,a21,a21,a23);                    \
                STORE(df,d1+2*BPP(df),a11);                 \
                STORE(df,d1+3*BPP(df),a13);                 \
                STORE(df,d2+2*BPP(df),a21);                 \
                STORE(df,d2+3*BPP(df),a23);                 \
                /* process third 2x2 block: */              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* calculate & store third half-pixels: */  \
                AVERAGE(df,a13,a13,a11);                    \
                AVERAGE(df,a23,a23,a21);                    \
                STORE(df,d1+4*BPP(df),a13);                 \
                STORE(df,d1+5*BPP(df),a11);                 \
                STORE(df,d2+4*BPP(df),a23);                 \
                STORE(df,d2+5*BPP(df),a21);                 \
                /* calculate & store fourth half-pixels: */ \
                AVERAGE(df,a11,a11,a12);                    \
                AVERAGE(df,a21,a21,a22);                    \
                STORE(df,d1+6*BPP(df),a11);                 \
                STORE(df,d1+7*BPP(df),a12);                 \
                STORE(df,d2+6*BPP(df),a21);                 \
                STORE(df,d2+7*BPP(df),a22);                 \
                d1 += 8*BPP(df);                            \
                d2 += 8*BPP(df);                            \
                count -= 4;                                 \
            }                                               \
            /* check if we have one more 2x2 block: */      \
            if (count >= 2) {                               \
                /* process last 2x2 block: */               \
                PIXEL(df,a13); PIXEL(df,a23);               \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a13,a21,a23,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* calculate & store first half-pixels: */  \
                AVERAGE(df,a12,a12,a11);                    \
                AVERAGE(df,a22,a22,a21);                    \
                STORE(df,d1+0*BPP(df),a12);                 \
                STORE(df,d1+1*BPP(df),a11);                 \
                STORE(df,d2+0*BPP(df),a22);                 \
                STORE(df,d2+1*BPP(df),a21);                 \
                /* calculate & store second half-pixels: */ \
                AVERAGE(df,a11,a11,a13);                    \
                AVERAGE(df,a21,a21,a23);                    \
                STORE(df,d1+2*BPP(df),a11);                 \
                STORE(df,d1+3*BPP(df),a13);                 \
                STORE(df,d2+2*BPP(df),a21);                 \
                STORE(df,d2+3*BPP(df),a23);                 \
                /* move last converted pixels to a12/22: */ \
                COPY(df,a12,a13);                           \
                COPY(df,a22,a23);                           \
                d1 += 4*BPP(df);                            \
                d2 += 4*BPP(df);                            \
                count -= 2;                                 \
            }                                               \
            /* check if we have one more 2x1 block: */      \
            if (count >= 1) {                               \
                /* process last 2x1 block: */               \
                YUV_LOAD_CONVERT_2x1(cc,df,a11,a21,sy1,sy2,su,sv); \
                /* calculate & store last half-pixels: */   \
                AVERAGE(df,a12,a12,a11);                    \
                AVERAGE(df,a22,a22,a21);                    \
                STORE(df,d1+0*BPP(df),a12);                 \
                STORE(df,d1+1*BPP(df),a11);                 \
                STORE(df,d1+2*BPP(df),a11);                 \
                STORE(df,d2+0*BPP(df),a22);                 \
                STORE(df,d2+1*BPP(df),a21);                 \
                STORE(df,d2+2*BPP(df),a21);                 \
            } else {                                        \
                /* just replicate last pixels: */           \
                STORE(df,d1,a12);                           \
                STORE(df,d2,a22);                           \
            }                                               \
        }                                                   \
    }

/*
 * Generic row 2x+ stretching converter:
 *  "???" comments mean that under normal conditions these jumps
 *  should never be executed; nevertheless, I left these checks
 *  in place to guarantee the correct termination of the algorithm
 *  in all possible scenarios.
 */
#define DBLROW_STRETCH2XPLUS(cc,df,d1,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx << 1;  /* !!! */         \
        /* # of half-pixels to be processed separately: */  \
        int remainder = 3*dest_dx - limit;                  \
        if ((src_x + src_dx) & 1) remainder += 2*dest_dx;   \
        remainder /= step;                                  \
        /* check row length: */                             \
        if (count) {                                        \
            PIXEL(df,a11); PIXEL(df,a12);                   \
            PIXEL(df,a21); PIXEL(df,a22);                   \
            PIXEL(df,a13); PIXEL(df,a23);                   \
            /* check if an odd or single 2x1 block: */      \
            if ((src_x & 1) || src_dx < 2) {                \
                /* convert first 2x1 block: */              \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                sy1++; sy2++; su++; sv++;                   \
                /* update count: */                         \
                if ((count -= remainder) <= 0)              \
                    goto rep_last;                          \
                goto rep_odd;                               \
            } else {                                        \
                /* convert first 2x2 block: */              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* update count: */                         \
                if ((count -= remainder) <= 0)              \
                    goto rep_last_2;        /* ??? */       \
                goto rep_even;                              \
            }                                               \
            /* the main loop: */                            \
            while (1) {                                     \
                /* load & convert second 2x2 block: */      \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a13,a21,a23,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* calc. & replicate first half-pixels: */  \
                AVERAGE(df,a12,a12,a11);                    \
                AVERAGE(df,a22,a22,a21);                    \
                do {                                        \
                    STORE(df,d1,a12);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a22);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;      /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* replicate second even integral pixels: */\
                do {                                        \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_2;    /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* calc. & replicate second half-pixels: */ \
                AVERAGE(df,a11,a11,a13);                    \
                AVERAGE(df,a21,a21,a23);                    \
                do {                                        \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_3;    /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* replicate second odd integral pixels: */ \
                do {                                        \
                    STORE(df,d1,a13);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a23);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto last_pixel_2;  /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* load & convert third 2x2 block: */       \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* calc. & replicate third half-pixels: */  \
                AVERAGE(df,a13,a13,a11);                    \
                AVERAGE(df,a23,a23,a21);                    \
                do {                                        \
                    STORE(df,d1,a13);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a23);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_3;    /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
rep_even:       /* replicate third even integral pixels: */ \
                do {                                        \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_2;    /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* calc. & replicate fourth half-pixels: */ \
                AVERAGE(df,a11,a11,a12);                    \
                AVERAGE(df,a21,a21,a22);                    \
                do {                                        \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;      /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
rep_odd:        /* replicate third odd integral pixels: */  \
                do {                                        \
                    STORE(df,d1,a12);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a22);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto last_pixel;    /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
            }                                               \
last_pixel_2:/* store last integral pixels in a11/21: */    \
            COPY(df,a11,a13);                               \
            COPY(df,a21,a23);                               \
last_pixel: /* check if we need to convert one more pixel:*/\
            if ((src_x + src_dx) & 1) {                     \
                /* update count & remainder: */             \
                register int r2 = remainder >> 1;           \
                count += r2; remainder -= r2;               \
                if (count <= 0)                             \
                    goto rep_last;                          \
                /* load & convert last 2x1 block: */        \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                /* calc. & replicate last half-pixels: */   \
                AVERAGE(df,a11,a11,a12);                    \
                AVERAGE(df,a21,a21,a22);                    \
                do {                                        \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;      /* !!! */       \
                } while ((limit -= step) >= 0);             \
            }                                               \
            goto rep_last;                                  \
rep_last_3: /* store last converted pixels in a12/22: */    \
            COPY(df,a12,a13);                               \
            COPY(df,a22,a23);                               \
            goto rep_last;                                  \
rep_last_2: /* store last converted pixels in a12/22: */    \
            COPY(df,a12,a11);                               \
            COPY(df,a22,a21);                               \
            /* restore the number of remaining pixels: */   \
rep_last:   count += remainder;                             \
            while (count --) {                              \
                /* replicate them: */                       \
                STORE(df,d1,a12);                           \
                d1+=BPP(df);                                \
                STORE(df,d2,a22);                           \
                d2+=BPP(df);                                \
            }                                               \
        }                                                   \
    }

/*** Generic YUVtoRGB double-row 2x converters: ************/

/*
 * Generic YUVtoRGB double-row shrinking converter:
 *  uses read-ahead optimization to process full 2x2 blocks
 *  whenever possible.
 */
#define DBLROW2X_SHRINK(cc,df,d0,d01,d1,d12,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = dest_dx;                       \
        register int limit = src_dx >> 1; /* -1 */          \
        register int step = dest_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            /* check if we have an odd first block: */      \
            if (src_x & 1)                                  \
                goto start_odd;                             \
            /* process even pixels: */                      \
            do {                                            \
                PIXEL(df,a11); PIXEL(df,a12);               \
                PIXEL(df,a21); PIXEL(df,a22);               \
                /* make one Bresenham step ahead: */        \
                if ((limit -= step) < 0) {                  \
                    limit += src_dx;                        \
                    /* can we process 2x2 pixels? */        \
                    if (!--count)                           \
                        goto last_pixel;                    \
                    /* process full 2x2 block: */           \
                    YUV_LOAD_CONVERT_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                    sy1+=2; sy2+=2; su++; sv++;             \
                    STORE(df,d1,a11);                       \
                    STORE(df,d1+BPP(df),a12);               \
                    d1+=2*BPP(df);                          \
                    STORE(df,d2,a21);                       \
                    STORE(df,d2+BPP(df),a22);               \
                    d2+=2*BPP(df);                          \
                    /* process average pixels: */           \
                    AVERAGE(df,a21,a11,a21);                \
                    AVERAGE(df,a22,a12,a22);                \
                    LOAD_AVERAGE(df,a11,a11,d0);            \
                    LOAD_AVERAGE(df,a12,a12,d0+BPP(df));    \
                    d0+=2*BPP(df);                          \
                    STORE(df,d01,a11);                      \
                    STORE(df,d01+BPP(df),a12);              \
                    d01+=2*BPP(df);                         \
                    STORE(df,d12,a21);                      \
                    STORE(df,d12+BPP(df),a22);              \
                    d12+=2*BPP(df);                         \
                } else {                                    \
                    /* proc. first 2x1 block & skip next: */\
                    YUV_LOAD_CONVERT_2x1(cc,df,a11,a21,sy1,sy2,su,sv); \
                    sy1+=2; sy2+=2; su++; sv++;             \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    /* process average pixels: */           \
                    AVERAGE(df,a21,a11,a21);                \
                    LOAD_AVERAGE(df,a11,a11,d0);            \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a11);                      \
                    d01+=BPP(df);                           \
                    STORE(df,d12,a21);                      \
                    d12+=BPP(df);                           \
                }                                           \
                /* inverted Bresenham stepping: */          \
                while ((limit -= step) >= 0) {              \
                    /* skip next even source pixel: */      \
                    sy1++; sy2++;                           \
                    if ((limit -= step) < 0)                \
                        goto cont_odd;                      \
                    /* skip odd source pixel: */            \
                    sy1++; sy2++;                           \
                    su++; sv++; /* next chroma: */          \
                }                                           \
cont_even:      /* continue loop with next even pixel: */   \
                limit += src_dx;                            \
            } while (--count);                              \
            goto done;                                      \
last_pixel: /* use this branch to process last pixel:*/     \
            count++;                                        \
start_odd:  /* process odd pixels: */                       \
            do {                                            \
                /* convert 2x1 block: */                    \
                PIXEL(df,a11); PIXEL(df,a21);               \
                YUV_LOAD_CONVERT_2x1(cc,df,a11,a21,sy1,sy2,su,sv); \
                STORE(df,d1,a11);                           \
                d1+=BPP(df);                                \
                STORE(df,d2,a21);                           \
                d2+=BPP(df);                                \
                /* process average pixels: */               \
                AVERAGE(df,a21,a11,a21);                    \
                LOAD_AVERAGE(df,a11,a11,d0);                \
                d0+=BPP(df);                                \
                STORE(df,d01,a11);                          \
                d01+=BPP(df);                               \
                STORE(df,d12,a21);                          \
                d12+=BPP(df);                               \
                /* inverted Bresenham stepping: */          \
                do {                                        \
                    /* skip odd source pixel: */            \
                    sy1++; sy2++;                           \
                    su++; sv++; /* next chroma: */          \
                    if ((limit -= step) < 0)                \
                        goto cont_even;                     \
                    /* skip even source pixel: */           \
                    sy1++; sy2++;                           \
                } while ((limit -= step) >= 0);             \
cont_odd:       limit += src_dx;                            \
            } while (--count);                              \
done:       ;                                               \
        }                                                   \
    }

/*
 * Generic YUVtoRGB double-row copy converter:
 */
#define DBLROW2X_COPY(cc,df,d0,d01,d1,d12,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        register int count = dest_dx;                       \
        /* convert first 2x1 block: */                      \
        if ((src_x & 1) && count) {                         \
            YUV_LOAD_CONVERT_AVERAGE_STORE_2x1(cc,df,d0,d01,d1,d12,d2,sy1,sy2,su,sv); \
            count--;                                        \
        }                                                   \
        /* convert all integral 2x2 blocks: */              \
        while (count >= 2) {                                \
            YUV_LOAD_CONVERT_AVERAGE_DITHER_STORE_2x2(cc,df,d0,d01,d1,d12,d2,sy1,sy2,su,sv); \
            count -= 2;                                     \
        }                                                   \
        /* convert last 2x1 block: */                       \
        if (count) {                                        \
            YUV_LOAD_CONVERT_AVERAGE_STORE_2x1(cc,df,d0,d01,d1,d12,d2,sy1,sy2,su,sv); \
        }                                                   \
    }

/*
 * Generic YUVtoRGB double row stretching converter:
 */
#define DBLROW2X_STRETCH(cc,df,d0,d01,d1,d12,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx;                         \
        /* # of pixels to be processed separately: */       \
        int remainder = dest_dx - limit;                    \
        if ((src_x + src_dx) & 1) remainder += dest_dx;     \
        remainder /= step;                                  \
        /* check row length: */                             \
        if (count) {                                        \
            PIXEL(df,a11); PIXEL(df,a12);                   \
            PIXEL(df,a21); PIXEL(df,a22);                   \
            PIXEL(df,a01x);PIXEL(df,a12x);                  \
            /* update count: */                             \
            if ((count -= remainder) <= 0)                  \
                goto convert_last;                          \
            /* check if we have an odd first block: */      \
            if (src_x & 1) {                                \
                /* convert first 2x1 block: */              \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                sy1++; sy2++; su++; sv++;                   \
                goto rep_odd;                               \
            }                                               \
            /* the main loop: */                            \
            while (1) {                                     \
                /* load & convert next 2x2 pixels: */       \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* average and replicate even pixels: */    \
                LOAD_AVERAGE(df,a01x,a11,d0);               \
                AVERAGE(df,a12x,a11,a21);                   \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;                      \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
rep_odd:        /* average & replicate odd pixels: */       \
                LOAD_AVERAGE(df,a01x,a12,d0);               \
                AVERAGE(df,a12x,a12,a22);                   \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a12);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a22);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto check_last;                    \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
            }                                               \
check_last: /* check if we need to convert one more pixel:*/\
            if ((src_x + src_dx) & 1) {                     \
convert_last:   /* last 2x1 block: */                       \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                /* calc. average pixels: */                 \
                LOAD_AVERAGE(df,a01x,a12,d0);               \
                AVERAGE(df,a12x,a12,a22);                   \
            }                                               \
            /* restore the number of remaining pixels: */   \
rep_last:   count += remainder;                             \
            while (count --) {                              \
                /* replicate them: */                       \
                STORE(df,d01,a01x);                         \
                d01+=BPP(df);                               \
                STORE(df,d1,a12);                           \
                d1+=BPP(df);                                \
                STORE(df,d12,a12x);                         \
                d12+=BPP(df);                               \
                STORE(df,d2,a22);                           \
                d2+=BPP(df);                                \
            }                                               \
        }                                                   \
    }

/*
 * Generic row 2x-stretching converter:
 */
#define DBLROW2X_STRETCH2X(cc,df,d0,d01,d1,d12,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = src_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            PIXEL(df,a011);PIXEL(df,a012);                  \
            PIXEL(df,a11); PIXEL(df,a12);                   \
            PIXEL(df,a121);PIXEL(df,a122);                  \
            PIXEL(df,a21); PIXEL(df,a22);                   \
            /* check if we have an odd or single pixel: */  \
            if ((src_x & 1) || count < 2) {                 \
                /* process first 2x1 block: */              \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                sy1++; sy2++; su++; sv++;                   \
                STORE(df,d1+0*BPP(df),a12);                 \
                STORE(df,d2+0*BPP(df),a22);                 \
                /* process vertical half-pixels: */         \
                LOAD_AVERAGE(df,a012,a12,d0);               \
                STORE(df,d01+0*BPP(df),a012);               \
                AVERAGE(df,a122,a12,a22);                   \
                STORE(df,d12+0*BPP(df),a122);               \
                /* shift pointers: */                       \
                d0  += BPP(df);                             \
                d01 += BPP(df);                             \
                d1  += BPP(df);                             \
                d12 += BPP(df);                             \
                d2  += BPP(df);                             \
                count -= 1;                                 \
            } else {                                         \
                /* process first 2x2 block: */              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                STORE(df,d1+0*BPP(df),a11);                 \
                STORE(df,d2+0*BPP(df),a21);                 \
                STORE(df,d1+2*BPP(df),a12);                 \
                STORE(df,d2+2*BPP(df),a22);                 \
                /* process vertical half-pixels: */         \
                LOAD_AVERAGE(df,a011,a11,d0);               \
                STORE(df,d01+0*BPP(df),a011);               \
                AVERAGE(df,a121,a11,a21);                   \
                STORE(df,d12+0*BPP(df),a121);               \
                LOAD_AVERAGE(df,a012,a12,d0+2*BPP(df));     \
                STORE(df,d01+2*BPP(df),a012);               \
                AVERAGE(df,a122,a12,a22);                   \
                STORE(df,d12+2*BPP(df),a122);               \
                /* process horisontal half-pixels: */       \
                AVERAGE(df,a011,a011,a012);                 \
                STORE(df,d01+1*BPP(df),a011);               \
                AVERAGE(df,a11,a11,a12);                    \
                STORE(df,d1+1*BPP(df),a11);                 \
                AVERAGE(df,a121,a121,a122);                 \
                STORE(df,d12+1*BPP(df),a121);               \
                AVERAGE(df,a21,a21,a22);                    \
                STORE(df,d2+1*BPP(df),a21);                 \
                /* shift pointers: */                       \
                d0  += 3*BPP(df);                           \
                d01 += 3*BPP(df);                           \
                d1  += 3*BPP(df);                           \
                d12 += 3*BPP(df);                           \
                d2  += 3*BPP(df);                           \
                count -= 2;                                 \
            }                                               \
            /* process all internal 4x2 blocks: */          \
            while (count >= 4) {                            \
                /* process second 2x2 block: */             \
                PIXEL(df,a013); PIXEL(df,a13);              \
                PIXEL(df,a123); PIXEL(df,a23);              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a13,a21,a23,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                STORE(df,d1+1*BPP(df),a11);                 \
                STORE(df,d2+1*BPP(df),a21);                 \
                STORE(df,d1+3*BPP(df),a13);                 \
                STORE(df,d2+3*BPP(df),a23);                 \
                /* process vertical half-pixels: */         \
                LOAD_AVERAGE(df,a011,a11,d0+1*BPP(df));     \
                STORE(df,d01+1*BPP(df),a011);               \
                AVERAGE(df,a121,a11,a21);                   \
                STORE(df,d12+1*BPP(df),a121);               \
                LOAD_AVERAGE(df,a013,a13,d0+3*BPP(df));     \
                STORE(df,d01+3*BPP(df),a013);               \
                AVERAGE(df,a123,a13,a23);                   \
                STORE(df,d12+3*BPP(df),a123);               \
                /* process horisontal half-pixels: */       \
                AVERAGE(df,a012,a012,a011);                 \
                STORE(df,d01+0*BPP(df),a012);               \
                AVERAGE(df,a12,a12,a11);                    \
                STORE(df,d1+0*BPP(df),a12);                 \
                AVERAGE(df,a122,a122,a121);                 \
                STORE(df,d12+0*BPP(df),a122);               \
                AVERAGE(df,a22,a22,a21);                    \
                STORE(df,d2+0*BPP(df),a22);                 \
                AVERAGE(df,a011,a011,a013);                 \
                STORE(df,d01+2*BPP(df),a011); /*!!!*/       \
                AVERAGE(df,a11,a11,a13);                    \
                STORE(df,d1+2*BPP(df),a11);                 \
                AVERAGE(df,a121,a121,a123);                 \
                STORE(df,d12+2*BPP(df),a121); /*!!!*/       \
                AVERAGE(df,a21,a21,a23);                    \
                STORE(df,d2+2*BPP(df),a21);                 \
                /* process third 2x2 block: */              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                STORE(df,d1+5*BPP(df),a11);                 \
                STORE(df,d2+5*BPP(df),a21);                 \
                STORE(df,d1+7*BPP(df),a12);                 \
                STORE(df,d2+7*BPP(df),a22);                 \
                /* process vertical half-pixels: */         \
                LOAD_AVERAGE(df,a011,a11,d0+5*BPP(df));     \
                STORE(df,d01+5*BPP(df),a011);               \
                AVERAGE(df,a121,a11,a21);                   \
                STORE(df,d12+5*BPP(df),a121);               \
                LOAD_AVERAGE(df,a012,a12,d0+7*BPP(df));     \
                STORE(df,d01+7*BPP(df),a012);               \
                AVERAGE(df,a122,a12,a22);                   \
                STORE(df,d12+7*BPP(df),a122);               \
                /* process horisontal half-pixels: */       \
                AVERAGE(df,a013,a013,a011);                 \
                STORE(df,d01+4*BPP(df),a013);               \
                AVERAGE(df,a13,a13,a11);                    \
                STORE(df,d1+4*BPP(df),a13);                 \
                AVERAGE(df,a123,a123,a121);                 \
                STORE(df,d12+4*BPP(df),a123);               \
                AVERAGE(df,a23,a23,a21);                    \
                STORE(df,d2+4*BPP(df),a23);                 \
                AVERAGE(df,a011,a011,a012);                 \
                STORE(df,d01+6*BPP(df),a011);               \
                AVERAGE(df,a11,a11,a12);                    \
                STORE(df,d1+6*BPP(df),a11);                 \
                AVERAGE(df,a121,a121,a122);                 \
                STORE(df,d12+6*BPP(df),a121);               \
                AVERAGE(df,a21,a21,a22);                    \
                STORE(df,d2+6*BPP(df),a21);                 \
                /* shift pointers: */                       \
                d0  += 8*BPP(df);                           \
                d01 += 8*BPP(df);                           \
                d1  += 8*BPP(df);                           \
                d12 += 8*BPP(df);                           \
                d2  += 8*BPP(df);                           \
                count -= 4;                                 \
            }                                               \
            /* check if we have one more 2x2 block: */      \
            if (count >= 2) {                               \
                /* process last 2x2 block: */               \
                PIXEL(df,a013); PIXEL(df,a13);              \
                PIXEL(df,a123); PIXEL(df,a23);              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a13,a21,a23,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                STORE(df,d1+1*BPP(df),a11);                 \
                STORE(df,d2+1*BPP(df),a21);                 \
                STORE(df,d1+3*BPP(df),a13);                 \
                STORE(df,d2+3*BPP(df),a23);                 \
                /* process vertical half-pixels: */         \
                LOAD_AVERAGE(df,a011,a11,d0+1*BPP(df));     \
                STORE(df,d01+1*BPP(df),a011);               \
                AVERAGE(df,a121,a11,a21);                   \
                STORE(df,d12+1*BPP(df),a121);               \
                LOAD_AVERAGE(df,a013,a13,d0+3*BPP(df));     \
                STORE(df,d01+3*BPP(df),a013);               \
                AVERAGE(df,a123,a13,a23);                   \
                STORE(df,d12+3*BPP(df),a123);               \
                /* process horisontal half-pixels: */       \
                AVERAGE(df,a012,a012,a011);                 \
                STORE(df,d01+0*BPP(df),a012);               \
                AVERAGE(df,a12,a12,a11);                    \
                STORE(df,d1+0*BPP(df),a12);                 \
                AVERAGE(df,a122,a122,a121);                 \
                STORE(df,d12+0*BPP(df),a122);               \
                AVERAGE(df,a22,a22,a21);                    \
                STORE(df,d2+0*BPP(df),a22);                 \
                AVERAGE(df,a011,a011,a013);                 \
                STORE(df,d01+2*BPP(df),a011); /*!!!*/       \
                AVERAGE(df,a11,a11,a13);                    \
                STORE(df,d1+2*BPP(df),a11);                 \
                AVERAGE(df,a121,a121,a123);                 \
                STORE(df,d12+2*BPP(df),a121); /*!!!*/       \
                AVERAGE(df,a21,a21,a23);                    \
                STORE(df,d2+2*BPP(df),a21);                 \
                /* move last converted pixels to a12/22: */ \
                COPY(df,a012,a013);                         \
                COPY(df,a12,a13);                           \
                COPY(df,a122,a123);                         \
                COPY(df,a22,a23);                           \
                /* shift pointers: */                       \
                d0  += 4*BPP(df);                           \
                d01 += 4*BPP(df);                           \
                d1  += 4*BPP(df);                           \
                d12 += 4*BPP(df);                           \
                d2  += 4*BPP(df);                           \
                count -= 2;                                 \
            }                                               \
            /* check if we have one more 2x1 block: */      \
            if (count >= 1) {                               \
                /* process last 2x1 block: */               \
                YUV_LOAD_CONVERT_2x1(cc,df,a11,a21,sy1,sy2,su,sv); \
                STORE(df,d1+1*BPP(df),a11);                 \
                STORE(df,d1+2*BPP(df),a11);                 \
                STORE(df,d2+1*BPP(df),a21);                 \
                STORE(df,d2+2*BPP(df),a21);                 \
                /* process vertical half-pixels: */         \
                LOAD_AVERAGE(df,a011,a11,d0+1*BPP(df));     \
                STORE(df,d01+1*BPP(df),a011);               \
                STORE(df,d01+2*BPP(df),a011);               \
                AVERAGE(df,a121,a11,a21);                   \
                STORE(df,d12+1*BPP(df),a121);               \
                STORE(df,d12+2*BPP(df),a121);               \
                /* process horisontal half-pixels: */       \
                AVERAGE(df,a012,a012,a011);                 \
                STORE(df,d01+0*BPP(df),a012);               \
                AVERAGE(df,a12,a12,a11);                    \
                STORE(df,d1+0*BPP(df),a12);                 \
                AVERAGE(df,a122,a122,a121);                 \
                STORE(df,d12+0*BPP(df),a122);               \
                AVERAGE(df,a22,a22,a21);                    \
                STORE(df,d2+0*BPP(df),a22);                 \
            } else {                                        \
                /* just replicate last column: */           \
                STORE(df,d01,a012);                         \
                STORE(df,d1,a12);                           \
                STORE(df,d12,a122);                         \
                STORE(df,d2,a22);                           \
            }                                               \
        }                                                   \
    }

/*
 * Generic row 2x+ stretching converter:
 *  "???" comments mean that under normal conditions these jumps
 *  should never be executed; nevertheless, I left these checks
 *  in place to guarantee the correct termination of the algorithm
 *  in all possible scenarios.
 */
#define DBLROW2X_STRETCH2XPLUS(cc,df,d0,d01,d1,d12,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx << 1;  /* !!! */         \
        /* # of half-pixels to be processed separately: */  \
        int remainder = 3*dest_dx - limit;                  \
        if ((src_x + src_dx) & 1) remainder += 2*dest_dx;   \
        remainder /= step;                                  \
        /* check row length: */                             \
        if (count) {                                        \
            PIXEL(df,a11); PIXEL(df,a12);                   \
            PIXEL(df,a21); PIXEL(df,a22);                   \
            PIXEL(df,a13); PIXEL(df,a23);                   \
            PIXEL(df,a01x);PIXEL(df,a12x);                  \
            /* check if an odd or single 2x1 block: */      \
            if ((src_x & 1) || src_dx < 2) {                \
                /* convert first 2x1 block: */              \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                sy1++; sy2++; su++; sv++;                   \
                /* update count: */                         \
                if ((count -= remainder) <= 0)              \
                    goto rep_last;                          \
                goto rep_odd;                               \
            } else {                                        \
                /* convert first 2x2 block: */              \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* update count: */                         \
                if ((count -= remainder) <= 0)              \
                    goto rep_last_2;        /* ??? */       \
                goto rep_even;                              \
            }                                               \
            /* the main loop (a11,a12-last conv.pixels): */ \
            while (1) {                                     \
                /* load & convert second 2x2 block: */      \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a13,a21,a23,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* calc. & replicate first half-pixels: */  \
                AVERAGE(df,a12,a12,a11);                    \
                LOAD_AVERAGE(df,a01x,a12,d0);               \
                AVERAGE(df,a22,a22,a21);                    \
                AVERAGE(df,a12x,a12,a22);                   \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a12);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a22);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;      /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* get vertical half-pixels:*/              \
                LOAD_AVERAGE(df,a01x,a11,d0);               \
                AVERAGE(df,a12x,a11,a21);                   \
                /* replicate second even integral pixels: */\
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_2;    /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* calc. & replicate second half-pixels: */ \
                AVERAGE(df,a11,a11,a13);                    \
                LOAD_AVERAGE(df,a01x,a11,d0);               \
                AVERAGE(df,a21,a21,a23);                    \
                AVERAGE(df,a12x,a11,a21);                   \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_3;    /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* get vertical half-pixels:*/              \
                LOAD_AVERAGE(df,a01x,a13,d0);               \
                AVERAGE(df,a12x,a13,a23);                   \
                /* replicate second odd integral pixels: */ \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a13);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a23);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto last_pixel_2;  /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* load & convert third 2x2 block: */       \
                YUV_LOAD_CONVERT_DITHER_2x2(cc,df,a11,a12,a21,a22,sy1,sy2,su,sv); \
                sy1+=2; sy2+=2; su++; sv++;                 \
                /* calc. & replicate third half-pixels: */  \
                AVERAGE(df,a13,a13,a11);                    \
                LOAD_AVERAGE(df,a01x,a13,d0);               \
                AVERAGE(df,a23,a23,a21);                    \
                AVERAGE(df,a12x,a13,a23);                   \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a13);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a23);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_3;    /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
rep_even:       /* get vertical half-pixels:*/              \
                LOAD_AVERAGE(df,a01x,a11,d0);               \
                AVERAGE(df,a12x,a11,a21);                   \
                /* replicate third even integral pixels: */ \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last_2;    /* ??? */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* calc. & replicate fourth half-pixels: */ \
                AVERAGE(df,a11,a11,a12);                    \
                LOAD_AVERAGE(df,a01x,a11,d0);               \
                AVERAGE(df,a21,a21,a22);                    \
                AVERAGE(df,a12x,a11,a21);                   \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;      /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
rep_odd:        /* get vertical half-pixels:*/              \
                LOAD_AVERAGE(df,a01x,a12,d0);               \
                AVERAGE(df,a12x,a12,a22);                   \
                /* replicate third odd integral pixels: */  \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a12);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a22);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto last_pixel;    /* !!! */       \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
            }                                               \
last_pixel_2:/* store last integral pixels in a11/21: */    \
            COPY(df,a11,a13);                               \
            COPY(df,a21,a23);                               \
last_pixel: /* check if we need to convert one more pixel:*/\
            if ((src_x + src_dx) & 1) {                     \
                /* update count & remainder: */             \
                register int r2 = remainder >> 1;           \
                count += r2; remainder -= r2;               \
                if (count <= 0)                             \
                    goto rep_last;                          \
                /* load & convert last 2x1 block: */        \
                YUV_LOAD_CONVERT_2x1(cc,df,a12,a22,sy1,sy2,su,sv); \
                /* calc. & replicate last half-pixels: */   \
                AVERAGE(df,a11,a11,a12);                    \
                LOAD_AVERAGE(df,a01x,a11,d0);               \
                AVERAGE(df,a21,a21,a22);                    \
                AVERAGE(df,a12x,a11,a21);                   \
                do {                                        \
                    d0+=BPP(df);                            \
                    STORE(df,d01,a01x);                     \
                    d01+=BPP(df);                           \
                    STORE(df,d1,a11);                       \
                    d1+=BPP(df);                            \
                    STORE(df,d12,a12x);                     \
                    d12+=BPP(df);                           \
                    STORE(df,d2,a21);                       \
                    d2+=BPP(df);                            \
                    if (!(--count))                         \
                        goto rep_last;      /* ??? */       \
                } while ((limit -= step) >= 0);             \
                /* get last vertical half-pixels:*/         \
                LOAD_AVERAGE(df,a01x,a12,d0);               \
                AVERAGE(df,a12x,a12,a22);                   \
            }                                               \
            goto rep_last;                                  \
rep_last_3: /* store last converted pixels in a12/22: */    \
            COPY(df,a12,a13);                               \
            COPY(df,a22,a23);                               \
            goto rep_last;                                  \
rep_last_2: /* store last converted pixels in a12/22: */    \
            COPY(df,a12,a11);                               \
            COPY(df,a22,a21);                               \
            /* restore the number of remaining pixels: */   \
rep_last:   count += remainder;                             \
            /* get vertical half-pixels:*/                  \
            LOAD_AVERAGE(df,a01x,a12,d0);                   \
            AVERAGE(df,a12x,a12,a22);                       \
            /* replicate them: */                           \
            while (count --) {                              \
                STORE(df,d01,a01x);                         \
                d01+=BPP(df);                               \
                STORE(df,d1,a12);                           \
                d1+=BPP(df);                                \
                STORE(df,d12,a12x);                         \
                d12+=BPP(df);                               \
                STORE(df,d2,a22);                           \
                d2+=BPP(df);                                \
            }                                               \
        }                                                   \
    }

/***********************************************************/

/*
 * Function names:
 */
#define FN(df,sf)               sf##to##df
#define FN2(df,sf)              sf##to##df##x
#define DBLROW_FN(df,sf,cc,t)   sf##to##df##_DBLROW_##cc##_##t
#define DBLROW2X_FN(df,sf,cc,t) sf##to##df##_DBLROW2X_##cc##_##t

/*
 * Function replication macros:
 *  (dblrow- and dblrow2x- converters)
 */
#define DBLROW_FUNC(df,sf,cc,t)   \
    static void DBLROW_FN(df,sf,cc,t) (unsigned char *d1, unsigned char *d2,\
        int dest_x, int dest_dx, unsigned char *sy1, unsigned char *sy2,    \
        unsigned char *su, unsigned char *sv, int src_x, int src_dx)        \
        DBLROW_##t(cc,df,d1,d2,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx)

#define DBLROW2X_FUNC(df,sf,cc,t)   \
    static void DBLROW2X_FN(df,sf,cc,t) (unsigned char *d1, unsigned char *d12,\
        unsigned char *d2, unsigned char *d23, unsigned char *d3,           \
        int dest_x, int dest_dx, unsigned char *sy1, unsigned char *sy2,    \
        unsigned char *su, unsigned char *sv, int src_x, int src_dx)        \
        DBLROW2X_##t(cc,df,d1,d12,d2,d23,d3,dest_x,dest_dx,sy1,sy2,su,sv,src_x,src_dx)


/***********************************************************/

/*
 * Actual double-row functions:
 */

DBLROW_FUNC(RGB32,  I420 ,FAST, SHRINK)
DBLROW_FUNC(RGB32,  I420 ,FAST, COPY)
DBLROW_FUNC(RGB32,  I420 ,FAST, STRETCH)
DBLROW_FUNC(RGB32,  I420 ,FAST, STRETCH2X)
DBLROW_FUNC(RGB32,  I420 ,FAST, STRETCH2XPLUS)

DBLROW_FUNC(BGR32,  I420 ,FAST, SHRINK)
DBLROW_FUNC(BGR32,  I420 ,FAST, COPY)
DBLROW_FUNC(BGR32,  I420 ,FAST, STRETCH)
DBLROW_FUNC(BGR32,  I420 ,FAST, STRETCH2X)
DBLROW_FUNC(BGR32,  I420 ,FAST, STRETCH2XPLUS)

DBLROW_FUNC(RGB24,  I420 ,FAST, SHRINK)
DBLROW_FUNC(RGB24,  I420 ,FAST, COPY)
DBLROW_FUNC(RGB24,  I420 ,FAST, STRETCH)
DBLROW_FUNC(RGB24,  I420 ,FAST, STRETCH2X)
DBLROW_FUNC(RGB24,  I420 ,FAST, STRETCH2XPLUS)

DBLROW_FUNC(RGB565, I420 ,FAST, SHRINK)
DBLROW_FUNC(RGB565, I420 ,FAST, COPY)
DBLROW_FUNC(RGB565, I420 ,FAST, STRETCH)
DBLROW_FUNC(RGB565, I420 ,FAST, STRETCH2X)
DBLROW_FUNC(RGB565, I420 ,FAST, STRETCH2XPLUS)

DBLROW_FUNC(RGB555, I420 ,FAST, SHRINK)
DBLROW_FUNC(RGB555, I420 ,FAST, COPY)
DBLROW_FUNC(RGB555, I420 ,FAST, STRETCH)
DBLROW_FUNC(RGB555, I420 ,FAST, STRETCH2X)
DBLROW_FUNC(RGB555, I420 ,FAST, STRETCH2XPLUS)

DBLROW_FUNC(RGB444, I420 ,FAST, SHRINK)
DBLROW_FUNC(RGB444, I420 ,FAST, COPY)
DBLROW_FUNC(RGB444, I420 ,FAST, STRETCH)
DBLROW_FUNC(RGB444, I420 ,FAST, STRETCH2X)
DBLROW_FUNC(RGB444, I420 ,FAST, STRETCH2XPLUS)

DBLROW_FUNC(RGB8,   I420 ,FAST, SHRINK)
DBLROW_FUNC(RGB8,   I420 ,FAST, COPY)
DBLROW_FUNC(RGB8,   I420 ,FAST, STRETCH)
DBLROW_FUNC(RGB8,   I420 ,FAST, STRETCH2X)
DBLROW_FUNC(RGB8,   I420 ,FAST, STRETCH2XPLUS)

/* converters with hue correction: */
DBLROW_FUNC(RGB32,  I420 ,FULL, SHRINK)
DBLROW_FUNC(RGB32,  I420 ,FULL, COPY)
DBLROW_FUNC(RGB32,  I420 ,FULL, STRETCH)
DBLROW_FUNC(RGB32,  I420 ,FULL, STRETCH2X)
DBLROW_FUNC(RGB32,  I420 ,FULL, STRETCH2XPLUS)

DBLROW_FUNC(BGR32,  I420 ,FULL, SHRINK)
DBLROW_FUNC(BGR32,  I420 ,FULL, COPY)
DBLROW_FUNC(BGR32,  I420 ,FULL, STRETCH)
DBLROW_FUNC(BGR32,  I420 ,FULL, STRETCH2X)
DBLROW_FUNC(BGR32,  I420 ,FULL, STRETCH2XPLUS)

DBLROW_FUNC(RGB24,  I420 ,FULL, SHRINK)
DBLROW_FUNC(RGB24,  I420 ,FULL, COPY)
DBLROW_FUNC(RGB24,  I420 ,FULL, STRETCH)
DBLROW_FUNC(RGB24,  I420 ,FULL, STRETCH2X)
DBLROW_FUNC(RGB24,  I420 ,FULL, STRETCH2XPLUS)

DBLROW_FUNC(RGB565, I420 ,FULL, SHRINK)
DBLROW_FUNC(RGB565, I420 ,FULL, COPY)
DBLROW_FUNC(RGB565, I420 ,FULL, STRETCH)
DBLROW_FUNC(RGB565, I420 ,FULL, STRETCH2X)
DBLROW_FUNC(RGB565, I420 ,FULL, STRETCH2XPLUS)

DBLROW_FUNC(RGB555, I420 ,FULL, SHRINK)
DBLROW_FUNC(RGB555, I420 ,FULL, COPY)
DBLROW_FUNC(RGB555, I420 ,FULL, STRETCH)
DBLROW_FUNC(RGB555, I420 ,FULL, STRETCH2X)
DBLROW_FUNC(RGB555, I420 ,FULL, STRETCH2XPLUS)

DBLROW_FUNC(RGB444, I420 ,FULL, SHRINK)
DBLROW_FUNC(RGB444, I420 ,FULL, COPY)
DBLROW_FUNC(RGB444, I420 ,FULL, STRETCH)
DBLROW_FUNC(RGB444, I420 ,FULL, STRETCH2X)
DBLROW_FUNC(RGB444, I420 ,FULL, STRETCH2XPLUS)

DBLROW_FUNC(RGB8,   I420 ,FULL, SHRINK)
DBLROW_FUNC(RGB8,   I420 ,FULL, COPY)
DBLROW_FUNC(RGB8,   I420 ,FULL, STRETCH)
DBLROW_FUNC(RGB8,   I420 ,FULL, STRETCH2X)
DBLROW_FUNC(RGB8,   I420 ,FULL, STRETCH2XPLUS)


/*
 * Actual double-row 2x functions:
 */

DBLROW2X_FUNC(RGB32,  I420 ,FAST, SHRINK)
DBLROW2X_FUNC(RGB32,  I420 ,FAST, COPY)
DBLROW2X_FUNC(RGB32,  I420 ,FAST, STRETCH)
DBLROW2X_FUNC(RGB32,  I420 ,FAST, STRETCH2X)
DBLROW2X_FUNC(RGB32,  I420 ,FAST, STRETCH2XPLUS)

DBLROW2X_FUNC(BGR32,  I420 ,FAST, SHRINK)
DBLROW2X_FUNC(BGR32,  I420 ,FAST, COPY)
DBLROW2X_FUNC(BGR32,  I420 ,FAST, STRETCH)
DBLROW2X_FUNC(BGR32,  I420 ,FAST, STRETCH2X)
DBLROW2X_FUNC(BGR32,  I420 ,FAST, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB24,  I420 ,FAST, SHRINK)
DBLROW2X_FUNC(RGB24,  I420 ,FAST, COPY)
DBLROW2X_FUNC(RGB24,  I420 ,FAST, STRETCH)
DBLROW2X_FUNC(RGB24,  I420 ,FAST, STRETCH2X)
DBLROW2X_FUNC(RGB24,  I420 ,FAST, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB565, I420 ,FAST, SHRINK)
DBLROW2X_FUNC(RGB565, I420 ,FAST, COPY)
DBLROW2X_FUNC(RGB565, I420 ,FAST, STRETCH)
DBLROW2X_FUNC(RGB565, I420 ,FAST, STRETCH2X)
DBLROW2X_FUNC(RGB565, I420 ,FAST, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB555, I420 ,FAST, SHRINK)
DBLROW2X_FUNC(RGB555, I420 ,FAST, COPY)
DBLROW2X_FUNC(RGB555, I420 ,FAST, STRETCH)
DBLROW2X_FUNC(RGB555, I420 ,FAST, STRETCH2X)
DBLROW2X_FUNC(RGB555, I420 ,FAST, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB444, I420 ,FAST, SHRINK)
DBLROW2X_FUNC(RGB444, I420 ,FAST, COPY)
DBLROW2X_FUNC(RGB444, I420 ,FAST, STRETCH)
DBLROW2X_FUNC(RGB444, I420 ,FAST, STRETCH2X)
DBLROW2X_FUNC(RGB444, I420 ,FAST, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB8,   I420 ,FAST, SHRINK)
DBLROW2X_FUNC(RGB8,   I420 ,FAST, COPY)
DBLROW2X_FUNC(RGB8,   I420 ,FAST, STRETCH)
DBLROW2X_FUNC(RGB8,   I420 ,FAST, STRETCH2X)
DBLROW2X_FUNC(RGB8,   I420 ,FAST, STRETCH2XPLUS)

/* converters with hue correction: */
DBLROW2X_FUNC(RGB32,  I420 ,FULL, SHRINK)
DBLROW2X_FUNC(RGB32,  I420 ,FULL, COPY)
DBLROW2X_FUNC(RGB32,  I420 ,FULL, STRETCH)
DBLROW2X_FUNC(RGB32,  I420 ,FULL, STRETCH2X)
DBLROW2X_FUNC(RGB32,  I420 ,FULL, STRETCH2XPLUS)

DBLROW2X_FUNC(BGR32,  I420 ,FULL, SHRINK)
DBLROW2X_FUNC(BGR32,  I420 ,FULL, COPY)
DBLROW2X_FUNC(BGR32,  I420 ,FULL, STRETCH)
DBLROW2X_FUNC(BGR32,  I420 ,FULL, STRETCH2X)
DBLROW2X_FUNC(BGR32,  I420 ,FULL, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB24,  I420 ,FULL, SHRINK)
DBLROW2X_FUNC(RGB24,  I420 ,FULL, COPY)
DBLROW2X_FUNC(RGB24,  I420 ,FULL, STRETCH)
DBLROW2X_FUNC(RGB24,  I420 ,FULL, STRETCH2X)
DBLROW2X_FUNC(RGB24,  I420 ,FULL, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB565, I420 ,FULL, SHRINK)
DBLROW2X_FUNC(RGB565, I420 ,FULL, COPY)
DBLROW2X_FUNC(RGB565, I420 ,FULL, STRETCH)
DBLROW2X_FUNC(RGB565, I420 ,FULL, STRETCH2X)
DBLROW2X_FUNC(RGB565, I420 ,FULL, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB555, I420 ,FULL, SHRINK)
DBLROW2X_FUNC(RGB555, I420 ,FULL, COPY)
DBLROW2X_FUNC(RGB555, I420 ,FULL, STRETCH)
DBLROW2X_FUNC(RGB555, I420 ,FULL, STRETCH2X)
DBLROW2X_FUNC(RGB555, I420 ,FULL, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB444, I420 ,FULL, SHRINK)
DBLROW2X_FUNC(RGB444, I420 ,FULL, COPY)
DBLROW2X_FUNC(RGB444, I420 ,FULL, STRETCH)
DBLROW2X_FUNC(RGB444, I420 ,FULL, STRETCH2X)
DBLROW2X_FUNC(RGB444, I420 ,FULL, STRETCH2XPLUS)

DBLROW2X_FUNC(RGB8,   I420 ,FULL, SHRINK)
DBLROW2X_FUNC(RGB8,   I420 ,FULL, COPY)
DBLROW2X_FUNC(RGB8,   I420 ,FULL, STRETCH)
DBLROW2X_FUNC(RGB8,   I420 ,FULL, STRETCH2X)
DBLROW2X_FUNC(RGB8,   I420 ,FULL, STRETCH2XPLUS)


/*
 * Double-row scale function selection tables:
 *  [conversion type][source format][row scale type]
 */
static void (* DblRowFuncs [2][RGB_FORMATS][SCALE_FUNCS]) (
    unsigned char *d1, unsigned char *d2, int dest_x, int dest_dx,
    unsigned char *sy1, unsigned char *sy2,
    unsigned char *su, unsigned char *sv, int src_x, int src_dx) =
{
    {   {        
#if defined (HELIX_FEATURE_CC_RGB32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB32 ,I420 ,FAST, SHRINK),
    #else   
            0,
    #endif //HXCOLOR_SHRINK
            
            DBLROW_FN(RGB32 ,I420 ,FAST, COPY),
            
    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB32 ,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB32 ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB32 ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB32out
        },{
#if defined (HELIX_FEATURE_CC_BGR32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(BGR32 ,I420 ,FAST, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK
            
            DBLROW_FN(BGR32 ,I420 ,FAST, COPY),
    
    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(BGR32 ,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(BGR32 ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(BGR32 ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS

#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_BGR32out
        },{
#if defined (HELIX_FEATURE_CC_RGB24out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB24 ,I420 ,FAST, SHRINK),
    #else   
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB24 ,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB24 ,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB24 ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB24 ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif  //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB24out

        },{
#if defined (HELIX_FEATURE_CC_RGB565out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB565,I420 ,FAST, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB565,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB565,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB565,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB565,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB565out
        },{
#if defined (HELIX_FEATURE_CC_RGB555out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB555,I420 ,FAST, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB555,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB555,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB555,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)            
            DBLROW_FN(RGB555,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB555out
        },{
#if defined (HELIX_FEATURE_CC_RGB444out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB444,I420 ,FAST, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB444,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB444,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB444,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)            
            DBLROW_FN(RGB444,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB444out
        },{
#if defined (HELIX_FEATURE_CC_RGB8out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB8  ,I420 ,FAST, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB8  ,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB8  ,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB8  ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS) 
            DBLROW_FN(RGB8  ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif//HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB8out
        }
    },{ {
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB32 ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB32 ,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB32 ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB32 ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB32 ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_BGR32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(BGR32 ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK
            
            DBLROW_FN(BGR32 ,I420 ,FULL, COPY),
            
    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(BGR32 ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(BGR32 ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X

    #if defined (HXCOLOR_STRETCH2XPLUS)            
            DBLROW_FN(BGR32 ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB24out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB24 ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB24 ,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB24 ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB24 ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X
                 
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB24 ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB565out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB565,I420 ,FULL, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB565,I420 ,FULL, COPY),
    
    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB565,I420 ,FULL, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH
    
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB565,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB565,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB555out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB555,I420 ,FULL, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB555,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB555,I420 ,FULL, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB555,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB555,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB444out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB444,I420 ,FULL, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB444,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB444,I420 ,FULL, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB444,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB444,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB8out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW_FN(RGB8  ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK

            DBLROW_FN(RGB8  ,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW_FN(RGB8  ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW_FN(RGB8  ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW_FN(RGB8  ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
            
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        }
    }
};

/*
 * Double-row scale function selection tables:
 *  [conversion type][source format][row scale type]
 */
static void (* DblRow2xFuncs [2][RGB_FORMATS][SCALE_FUNCS]) (
    unsigned char *d0, unsigned char *d01, unsigned char *d1,
    unsigned char *d12, unsigned char *d2, int dest_x, int dest_dx,
    unsigned char *sy1, unsigned char *sy2,
    unsigned char *su, unsigned char *sv, int src_x, int src_dx) =
{
    {   {
#if defined (HELIX_FEATURE_CC_RGB32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB32 ,I420 ,FAST, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB32 ,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB32 ,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH

    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB32 ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB32 ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB32out

        },{
#if defined (HELIX_FEATURE_CC_BGR32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(BGR32 ,I420 ,FAST, SHRINK),
    #else
            0,
    #endif //HXCOLOR_SHRINK
            
            DBLROW2X_FN(BGR32 ,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(BGR32 ,I420 ,FAST, STRETCH),
    #else
            0,
    #endif //HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(BGR32 ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif //HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(BGR32 ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif //HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_BGR32out

        },{
#if defined (HELIX_FEATURE_CC_RGB24out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB24 ,I420 ,FAST, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB24 ,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB24 ,I420 ,FAST, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB24 ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB24 ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB24out

        },{
#if defined (HELIX_FEATURE_CC_RGB565out)
            
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB565,I420 ,FAST, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB565,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB565,I420 ,FAST, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB565,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB565,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB565out

        },{
#if defined (HELIX_FEATURE_CC_RGB555out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB555,I420 ,FAST, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB555,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB555,I420 ,FAST, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB555,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB555,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif // STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB555out

        },{
#if defined (HELIX_FEATURE_CC_RGB444out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB444,I420 ,FAST, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB444,I420 ,FAST, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB444,I420 ,FAST, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB444,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB444,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif // STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB444out

        },{
#if defined (HELIX_FEATURE_CC_RGB8out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB8  ,I420 ,FAST, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB8  ,I420 ,FAST, COPY),

    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB8  ,I420 ,FAST, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
    #if defined (HXCOLOR_SHRINK2X)
            DBLROW2X_FN(RGB8  ,I420 ,FAST, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_SHRINK2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB8  ,I420 ,FAST, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif //HELIX_FEATURE_CC_RGB8out

        }
    },{ {
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB32 ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB32 ,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB32 ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB32 ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB32 ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_BGR32out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(BGR32 ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(BGR32 ,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(BGR32 ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(BGR32 ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(BGR32 ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB24out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB24 ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB24 ,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB24 ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB24 ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB24 ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB565out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB565,I420 ,FULL, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB565,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB565,I420 ,FULL, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB565,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB565,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB555out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB555,I420 ,FULL, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB555,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB555,I420 ,FULL, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB555,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB555,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB444out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB444,I420 ,FULL, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB444,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB444,I420 ,FULL, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB444,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB444,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#if defined _PLUS_HXCOLOR && defined (HELIX_FEATURE_CC_RGB8out)
    #if defined (HXCOLOR_SHRINK)
            DBLROW2X_FN(RGB8  ,I420 ,FULL, SHRINK),
    #else
            0,
    #endif // HXCOLOR_SHRINK
            
            DBLROW2X_FN(RGB8  ,I420 ,FULL, COPY),

    #if defined (HXCOLOR_STRETCH)
            DBLROW2X_FN(RGB8  ,I420 ,FULL, STRETCH),
    #else
            0,
    #endif // HXCOLOR_STRETCH
            
    #if defined (HXCOLOR_STRETCH2X)
            DBLROW2X_FN(RGB8  ,I420 ,FULL, STRETCH2X),
    #else
            0,
    #endif // HXCOLOR_STRETCH2X
            
    #if defined (HXCOLOR_STRETCH2XPLUS)
            DBLROW2X_FN(RGB8  ,I420 ,FULL, STRETCH2XPLUS)
    #else
            0
    #endif // HXCOLOR_STRETCH2XPLUS
            
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        }
    }
};


/*** YUV image converters: *********************************/

/*
 * Temporary RGB row buffers:
 */
#define MAXWIDTH 2048   /* should be larger that max.hor.res. of a monitor */
static unsigned char tmp1 [MAXWIDTH * BPP(RGB32)]; /* Flawfinder: ignore */
static unsigned char tmp2 [MAXWIDTH * BPP(RGB32)]; /* Flawfinder: ignore */

/*
 * Image shrink converter:
 *  (uses readahead optimization to process all aligned
 *  pairs of rows simultaneously (in a single dblrow_func() call))
 */
static void IMAGE_SHRINK (unsigned char *dest_ptr,
    int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
    int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
    void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
    void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int))
{
    /* initialize local variables: */
    register unsigned char *d1  = dest_ptr;
    register unsigned char *d2  = d1 + dest_pitch;
    register unsigned char *sy1 = src_y_ptr;
    register unsigned char *sy2 = sy1 + src_pitch;
    register unsigned char *su  = src_u_ptr;
    register unsigned char *sv  = src_v_ptr;
    register int count = dest_dy;
    register int limit = src_dy >> 1; /* -1 */
    register int step = dest_dy;
    /* check image height: */
    if (count) {
        /* check if top row is misaligned: */
        if (src_y & 1)
            goto start_odd;
        /* process even rows: */
        do {
            /* make one Bresenham step ahead: */
            if ((limit -= step) < 0) {
                limit += src_dy;
                /* can we process 2 rows? */
                if (!--count)
                    goto last_pixel;
                /* process two rows: */
                (* dblrow_func) (d1, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
                d1  += dest_pitch*2; d2  += dest_pitch*2;
                sy1 += src_pitch*2;  sy2 += src_pitch*2;
                su  += src_pitch_2;  sv  += src_pitch_2;
            } else {
                /* proc. first row & skip next: */
                (* dblrow_func) (d1, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
                d1  += dest_pitch;   d2  += dest_pitch;
                sy1 += src_pitch*2;  sy2 += src_pitch*2;
                su  += src_pitch_2;  sv  += src_pitch_2;
            }
            /* inverted Bresenham stepping: */
            while ((limit -= step) >= 0) {
                /* skip next even source row: */
                sy1 += src_pitch;    sy2 += src_pitch;
                if ((limit -= step) < 0)
                    goto cont_odd;
                /* skip odd source row: */
                sy1 += src_pitch;    sy2 += src_pitch;
                su  += src_pitch_2;  sv  += src_pitch_2; /* next chroma */
            }
            /* continue loop with next even row: */
cont_even:
            limit += src_dy;
        } while (--count);
        goto done;
        /* use this branch to process last row as well:*/
last_pixel:
        count++;
        /* process odd rows: */
start_odd:
        do {
            /* convert a single row: */
            (* dblrow_func) (d1, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
            d1  += dest_pitch;   d2  += dest_pitch;
            /* inverted Bresenham stepping: */
            do {
                /* skip odd source row: */
                sy1 += src_pitch;    sy2 += src_pitch;
                su  += src_pitch_2;  sv  += src_pitch_2; /* next chroma */
                if ((limit -= step) < 0)
                    goto cont_even;
                /* skip even source row: */
                sy1 += src_pitch;    sy2 += src_pitch;
            } while ((limit -= step) >= 0);
cont_odd:
            limit += src_dy;
        } while (--count);
done:   ;
    }
}

/*
 * Image copy converter:
 */
static void IMAGE_COPY (
    unsigned char *dest_ptr,
    int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
    int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
    void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
    void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int))
{
    /* initialize local variables: */
    register unsigned char *d1  = dest_ptr;
    register unsigned char *d2  = d1 + dest_pitch;
    register unsigned char *sy1 = src_y_ptr;
    register unsigned char *sy2 = sy1 + src_pitch;
    register unsigned char *su  = src_u_ptr;
    register unsigned char *sv  = src_v_ptr;
    register int count = dest_dy;
    /* check if top row is misaligned: */
    if ((src_y & 1) && count) {
        /* convert a single row: */
        (* dblrow_func) (d1, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
        /* shift pointers: */
        d1  += dest_pitch;  d2  += dest_pitch;
        sy1 += src_pitch;   sy2 += src_pitch;
        su  += src_pitch_2; sv  += src_pitch_2;
        count --;
    }
    /* convert the 2-row-aligned portion of the image: */
    while (count >= 2) {
        /* convert two rows: : */
        (* dblrow_func) (d1, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
        /* shift pointers: */
        d1  += dest_pitch*2; d2  += dest_pitch*2;
        sy1 += src_pitch*2;  sy2 += src_pitch*2;
        su  += src_pitch_2;  sv  += src_pitch_2;
        count -= 2;
    }
    /* convert the bottom row (if # of rows is odd): */
    if (count) {
        /* convert a single row: */
        (* dblrow_func) (d1, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
    }
}

/*
 * Image stretching converter:
 *  (uses readahead optimization to perform in-place 2 rows conversion:)
 */
static void IMAGE_STRETCH (unsigned char *dest_ptr,
    int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
    int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
    void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
    void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int))
{
    /* load image pointers: */
    register unsigned char *d1  = dest_ptr;
    register unsigned char *d2  = dest_ptr; /* !!! */
    register unsigned char *sy1 = src_y_ptr;
    register unsigned char *sy2 = sy1 + src_pitch;
    register unsigned char *su  = src_u_ptr;
    register unsigned char *sv  = src_v_ptr;
    /* initialize other variables: */
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = dest_dy;
    register int limit = dest_dy >> 1; /* !!! */
    register int step = src_dy;
    /* # of rows to be processed separately: */
    int remainder = dest_dy - limit;
    if ((src_y + src_dy) & 1) remainder += dest_dy;
    remainder /= step;
    /* check image height: */
    if (count) {
        /* update count: */
        if ((count -= remainder) <= 0)
            goto convert_last;
        /* check if we have an odd first row: */
        if (src_x & 1) {
            /* convert first row: */
            (* dblrow_func) (d2, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
            sy1 += src_pitch;   sy2 += src_pitch;
            su  += src_pitch_2; sv  += src_pitch_2;
            goto rep_odd;
        }
        /* the main loop: */
        while (1) {
            /* find second destination row: */
            do {
                /* just shift a pointer: */
                d2 += dest_pitch;
                /* check if all rows are filled up: */
                if (!(--count))
                    goto convert_last_2;    /* !!! */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* convert two rows: */
            (* dblrow_func) (d1, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
            sy1 += src_pitch*2;  sy2 += src_pitch*2;
            su  += src_pitch_2;  sv  += src_pitch_2;
            /* replicate first (even) row: */
            while (d1 + dest_pitch != d2) { /* can't use < pitch can be negative !!! */
                memcpy(d1 + dest_pitch, d1, dest_dx_bytes); /* Flawfinder: ignore */
                d1 += dest_pitch;
            }
            /* replicate second (odd) row: */
            goto rep_odd;
            do {
                /* replicate a row: */
                memcpy(d2 + dest_pitch, d2, dest_dx_bytes); /* Flawfinder: ignore */
                d2 += dest_pitch;
rep_odd:
                /* check if all rows are filled up: */
                if (!(--count))
                    goto check_last;
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            d1 = d2 += dest_pitch;
        }
        /* convert last two rows: */
convert_last_2:
        d2 += dest_pitch;
        count --;
        (* dblrow_func) (d1, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
        /* replicate even row: */
        while (d1 + dest_pitch != d2) { /* can't use < pitch can be negative !!! */
            memcpy(d1 + dest_pitch, d1, dest_dx_bytes); /* Flawfinder: ignore */
            d1 += dest_pitch;
        }
        goto rep_last;
        /* check if we need to convert one more row: */
check_last:
        if ((src_y + src_dy) & 1) {
            /* convert the last row: */
            d2 += dest_pitch;
convert_last:
            count --;
            (* dblrow_func) (d2, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
        }
        /* restore the number of remaining row: */
rep_last:
        count += remainder;
        while (count-- > 0) {
            /* replicate a row: */
            memcpy(d2 + dest_pitch, d2, dest_dx_bytes); /* Flawfinder: ignore */
            d2 += dest_pitch;
        }
    }
}

/*
 * Image 2x-stretching converter:
 */
static void IMAGE_STRETCH2X (unsigned char *dest_ptr,
    int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
    int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
    void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
    void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int))
{
    /* load image pointers: */
    register unsigned char *d0, *d01;
    register unsigned char *d1  = dest_ptr;
    register unsigned char *d12 = d1 + dest_pitch;
    register unsigned char *d2  = d12 + dest_pitch;
    register unsigned char *sy1 = src_y_ptr;
    register unsigned char *sy2 = sy1 + src_pitch;
    register unsigned char *su  = src_u_ptr;
    register unsigned char *sv  = src_v_ptr;
    /* initialize other variables: */
    register int count = src_dy;                /* !!! */
    /* check image height: */
    if (count) {
        /* check if top row is misaligned or we have a single-line image: */
        if ((src_y & 1) || count < 2) {
            /* convert a single row: */
            (* dblrow_func) (d1, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
            sy1 += src_pitch;    sy2 += src_pitch;
            su  += src_pitch_2;  sv  += src_pitch_2;
            d0  =  d1;           d01 = d12;
            d1  += dest_pitch*2; d12 += dest_pitch*2;
            d2  += dest_pitch*2;
            count --;
        } else {
            /* convert & interpolate first two rows: */
            (* dblrow2x_func) (tmp1, tmp2, d1, d12, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
            sy1 += src_pitch*2;  sy2 += src_pitch*2;
            su  += src_pitch_2;  sv  += src_pitch_2;
            d0  =  d2;           d01 =  d0 + dest_pitch;
            d1  += dest_pitch*4; d12 += dest_pitch*4;
            d2  += dest_pitch*4;
            count -= 2;
        }
        /* convert the 2-row-aligned portion of the image: */
        while (count >= 2) {
            /* convert & interpolate two rows a time: */
            (* dblrow2x_func) (d0, d01, d1, d12, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
            sy1 += src_pitch*2;  sy2 += src_pitch*2;
            su  += src_pitch_2;  sv  += src_pitch_2;
            d0  += dest_pitch*4; d01 += dest_pitch*4;
            d1  += dest_pitch*4; d12 += dest_pitch*4;
            d2  += dest_pitch*4;
            count -= 2;
        }
        /* convert the bottom row (if # of rows is odd): */
        if (count) {
            /* convert & interpolate the last row: */
            (* dblrow2x_func) (d0, d01, d1, tmp1, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
            d0  += dest_pitch*2; d01 += dest_pitch*2;
        }
        /* replicate the last converted row: */
        memcpy(d01, d0, dest_dx * dest_bpp); /* Flawfinder: ignore */
    }
}

/*
 * Image 2x+ stretching converter:
 */
static void IMAGE_STRETCH2XPLUS (unsigned char *dest_ptr,
    int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
    int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
    void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
    void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int))
{
    /* load image pointers: */
    register unsigned char *d0, *d01, *d1, *d12, *d2, *d;
    register unsigned char *sy1 = src_y_ptr;
    register unsigned char *sy2 = sy1 + src_pitch;
    register unsigned char *su  = src_u_ptr;
    register unsigned char *sv  = src_v_ptr;
    /* initialize other variables: */
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = dest_dy;
    register int limit = dest_dy >> 1;
    register int step = src_dy << 1;
    /* # of rows to be processed separately: */
    int remainder = 3 * dest_dy - limit;
    if ((src_y + src_dy) & 1) remainder += 2 * dest_dy;
    remainder /= step;
    /* check destination image height: */
    if (count) {
        /* check if top row is misaligned or we have a single-line image: */
        if ((src_y & 1) || src_dy < 2) {
            /* convert a single row: */
convert_last_0:
            d2 = dest_ptr;
            (* dblrow_func) (d2, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
            sy1 += src_pitch;    sy2 += src_pitch;
            su  += src_pitch_2;  sv  += src_pitch_2;
            /* update count: */
            if ((count -= remainder) <= 0)
                goto rep_last;
        } else {
            register int count2 = count;
            /* set pointer to first integral-pixel row (d1): */
            d1 = dest_ptr;
            /* find half-pixel row (d12): */
            d12 = d1;
            do {
                d12 += dest_pitch;
                if (!(--count2))
                    goto convert_last_0;            /* ??? */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* find second integral-pixel row (d2): */
            d2 = d12;
            do {
                d2 += dest_pitch;
                if (!(--count2))
                    goto convert_last_0;            /* ??? */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* convert & interpolate first two rows: */
            (* dblrow2x_func) (tmp1, tmp2, d1, d12, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
            sy1 += src_pitch*2;  sy2 += src_pitch*2;
            su  += src_pitch_2;  sv  += src_pitch_2;
            /* update count: */
            count = count2;
            if ((count -= remainder) <= 0)
                goto rep_d1_d12_d2;                 /* ??? */
            goto rep_even;
        }
        /* the main loop: */
        while (1) {
            /* get last converted integral row (d0): */
            d0 = d2;
            /* find first half-pixel row (d01): */
            d01 = d0;
            do {
                d01 += dest_pitch;
                if (!(--count))
                    goto rep_last;                  /* ??? */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* find first integral-pixel row (d1): */
            d1 = d01;
            do {
                d1 += dest_pitch;
                if (!(--count))
                    goto rep_last;                  /* ??? */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* find second half-pixel row (d12): */
            d12 = d1;
            do {
                d12 += dest_pitch;
                if (!(--count))
                    goto convert_last_2_rows;       /* !!! */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* find second integral-pixel row (d2): */
            d2 = d12;
            do {
                d2 += dest_pitch;
                if (!(--count))
                    goto convert_last_2_or_3_rows;  /* !!! */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* convert & interpolate next two rows: */
            (* dblrow2x_func) (d0, d01, d1, d12, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
            sy1 += src_pitch*2;  sy2 += src_pitch*2;
            su  += src_pitch_2;  sv  += src_pitch_2;
            /* replicate a previous integral row (d0): */
            d = d0 + dest_pitch;
            while (d != d01) {
                memcpy(d, d0, dest_dx_bytes); /* Flawfinder: ignore */
                d += dest_pitch;
            }
            /* replicate the first half-pixel row (d01): */
            d = d01 + dest_pitch;
            while (d != d1) {
                memcpy(d, d01, dest_dx_bytes); /* Flawfinder: ignore */
                d += dest_pitch;
            }
rep_even:
            /* replicate the first integral row (d1): */
            d = d1 + dest_pitch;
            while (d != d12) {
                memcpy(d, d1, dest_dx_bytes); /* Flawfinder: ignore */
                d += dest_pitch;
            }
            /* replicate the second half-pixel row (d12): */
            d = d12 + dest_pitch;
            while (d != d2) {
                memcpy(d, d12, dest_dx_bytes); /* Flawfinder: ignore */
                d += dest_pitch;
            }
        }
        /* convert & replicate last two rows: */
convert_last_2_or_3_rows:
        /* convert & interpolate next two rows: */
        (* dblrow2x_func) (d0, d01, d1, d12, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
        sy1 += src_pitch*2;  sy2 += src_pitch*2;
        su  += src_pitch_2;  sv  += src_pitch_2;
        /* replicate a previous integral row (d0): */
        d = d0 + dest_pitch;
        while (d != d01) {
            memcpy(d, d0, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
        /* replicate the first half-pixel row (d01): */
        d = d01 + dest_pitch;
        while (d != d1) {
            memcpy(d, d01, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
        /* replicate the first integral row (d1): */
        d = d1 + dest_pitch;
        while (d != d12) {
            memcpy(d, d1, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
        /* replicate the second half-pixel row (d12): */
        d = d12 + dest_pitch;
        while (d != d2) {
            memcpy(d, d12, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
        /* check if we need to convert one more row: */
        if ((src_y + src_dy) & 1) {
            /* update count & remainder: */
            register int r2 = remainder >> 1;
            count += r2; remainder -= r2;
            if (count <= 0)
                goto rep_last;
            /* get last converted row (d1): */
            d1 = d2;
            /* find last half-pixel row (d01): */
            d12 = d1;
            do {
                d12 += dest_pitch;
                if (!(--count))
                    goto convert_last_row_1;  /* ??? */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* find last integral-pixel row (d2): */
            d2 = d12;
            do {
                d2 += dest_pitch;
                if (!(--count))
                    goto convert_last_row;    /* !!! */
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            goto convert_last_row;
            /* set pointer to the last integral row: */
convert_last_row_1:
            d2 = d12 + dest_pitch;
            count --;
            /* convert & interpolate the last row: */
convert_last_row:
            (* dblrow2x_func) (d1, d12, d2, tmp1, tmp2, dest_x, dest_dx, sy1, sy1, su, sv, src_x, src_dx);
            goto rep_d1_d12_d2;
        }
        goto rep_last;
        /* convert & replicate the last two rows: */
convert_last_2_rows:
        /* convert & interpolate next two rows: */
        (* dblrow2x_func) (d0, d01, d1, d12, d2, dest_x, dest_dx, sy1, sy2, su, sv, src_x, src_dx);
        sy1 += src_pitch*2;  sy2 += src_pitch*2;
        su  += src_pitch_2;  sv  += src_pitch_2;
        /* replicate a previous integral row (d0): */
        d = d0 + dest_pitch;
        while (d != d01) {
            memcpy(d, d0, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
        /* replicate the first half-pixel row (d01): */
        d = d01 + dest_pitch;
        while (d != d1) {
            memcpy(d, d01, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
rep_d1_d12_d2:
        /* replicate the first integral row (d1): */
        d = d1 + dest_pitch;
        while (d != d12) {
            memcpy(d, d1, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
        /* replicate the second half-pixel row (d12): */
        d = d12 + dest_pitch;
        while (d != d2) {
            memcpy(d, d12, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
        /* restore the number of remaining rows: */
rep_last:
        count += remainder;
        /* replicate last converted row: */
		if (count > 0) {
			d = d2 + dest_pitch;
			while (--count) {
				memcpy(d, d2, dest_dx_bytes); /* Flawfinder: ignore */
				d += dest_pitch;
			}
		}
    }
}

/*
 * Image scale functions table:
 *  [vertical scale type]
 */
static void (* ImageFuncs [SCALE_FUNCS]) (unsigned char *dest_ptr,
    int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
    int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
    void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
    void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int)) =
{
    IMAGE_SHRINK,
    IMAGE_COPY,
    IMAGE_STRETCH,
    IMAGE_STRETCH2X,
    IMAGE_STRETCH2XPLUS
};

/*
 * Bytes per pixel (bpp) table:
 */
static int bpp [RGB_FORMATS] =
{
    BPP(RGB32),
    BPP(BGR32),
    BPP(RGB24),
    BPP(RGB565),
    BPP(RGB555),
    BPP(RGB444),
    BPP(RGB8)
};

/*
 * The main YUVtoRGB converter:
 */
static int YUVtoRGB (
    /* destination format: */
    int dest_format,
    /* destination image parameters: */
    unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    /* source image parameters: */
    unsigned char *src_ptr, int src_width, int src_height,
    int src_pitch, int src_x, int src_y, int src_dx, int src_dy)
{
#if 1
    unsigned char *pU = src_ptr + src_height * src_pitch;
    unsigned char *pV = pU + src_height * src_pitch / 4;

    return YUVtoRGB2(dest_format, dest_ptr, dest_width, dest_height, dest_pitch,
                     dest_x, dest_y, dest_dx, dest_dy,
                     src_ptr, pU, pV, src_width, src_height,
                     src_pitch, src_pitch/2, src_pitch/2, src_x, src_y, src_dx, src_dy);

#else    
    /* pointers to low-level converters to use: */
    void (*dblrow_proc) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int);
    void (*dblrow2x_proc) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int);
    void (*image_proc) (unsigned char *dest_ptr,
        int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
        unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
        int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
        void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
            unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
        void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
            unsigned char *, unsigned char *, int, int, unsigned char *,
            unsigned char *, unsigned char *, unsigned char *, int, int));

    /* scale types: */
    register int scale_x, scale_y;

    /* pointers and destination pixel depth: */
    register unsigned char *d, *sy, *su, *sv;
    register int dest_bpp;

    /* check arguments: */
    if (
        /* alignments: */
        ((unsigned)dest_ptr & 3) || (dest_pitch & 3) ||
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
    {
        /* fail: */
        return -1;
    }

    /* get scale types: */
    GET_SCALE_TYPE(dest_dx, src_dx, scale_x);
    GET_SCALE_TYPE(dest_dy, src_dy, scale_y);

    /* select row and image converters: */
#ifdef _PLUS_HXCOLOR
    dblrow_proc   = DblRowFuncs [is_alpha][dest_format][scale_x];
    dblrow2x_proc = DblRow2xFuncs [is_alpha][dest_format][scale_x];
#else
    dblrow_proc   = DblRowFuncs [0][dest_format][scale_x];
    dblrow2x_proc = DblRow2xFuncs [0][dest_format][scale_x];
#endif
    image_proc    = ImageFuncs [scale_y];

    /* get destination pixel depth: */
    dest_bpp = bpp [dest_format];

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (src_pitch <= 0) return -1;          /* not supported */

    /* get pointers: */
    d = dest_ptr + dest_x * dest_bpp + dest_y * dest_pitch;
    sy = src_ptr + (src_x + src_y * src_pitch); /* luminance */
    su = src_ptr + src_height * src_pitch + (src_x/2 + src_y/2 * src_pitch/2); /* !!! */
    sv = su + src_height * src_pitch / 4;

    /* pass control to appropriate lower-level converters: */
    (* image_proc) (
        d, dest_x, dest_y, dest_dx, dest_dy, dest_pitch, dest_bpp,
        sy, su, sv, src_x, src_y, src_dx, src_dy, src_pitch, src_pitch/2,
        dblrow_proc, dblrow2x_proc);

    /* success: */
    return 0;
#endif
}


/*
 * Public format-conversion routines.
 * Use:
 *  int XXXXtoYYYY (unsigned char *dest_ptr, int dest_width, int dest_height,
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
 * Note:
 *  Both source and destination buffers must be 4-bytes aligned,
 *  and their pitches (#of bytes in row) shall be multiple of 4!!!
 */

#if 1

#define YUVTORGB_FUNC(df, sf)                               \
    int FN(df,sf) (unsigned char *dest_ptr,                 \
        int dest_width, int dest_height, int dest_pitch,    \
        int dest_x, int dest_y, int dest_dx, int dest_dy,   \
        unsigned char *src_ptr,                             \
        int src_width, int src_height, int src_pitch,       \
        int src_x, int src_y, int src_dx, int src_dy)       \
    {                                                       \
    unsigned char *pU = src_ptr + src_height * src_pitch;   \
    unsigned char *pV = pU + src_height * src_pitch / 4;    \
                                                            \
    if (ID(sf) == YV12_ID)                                  \
    {                                                       \
        unsigned char *pTemp = pU;                          \
        pU = pV;                                            \
        pV = pTemp;                                         \
    }                                                       \
                                                            \
    return YUVtoRGB2(ID(df), dest_ptr, dest_width, dest_height, dest_pitch, \
                     dest_x, dest_y, dest_dx, dest_dy,      \
                     src_ptr, pU, pV, src_width, src_height,\
                     src_pitch, src_pitch/2, src_pitch/2, src_x, src_y, src_dx, src_dy); \
                                                            \
                                                            \
    }

    /*        return YUVtoRGB(                                    \
            ID(df), dest_ptr, dest_width, dest_height,      \
            dest_pitch, dest_x, dest_y, dest_dx, dest_dy,   \
            src_ptr, src_width, src_height,                 \
            src_pitch, src_x, src_y, src_dx, src_dy);       \
*/


YUVTORGB_FUNC(RGB32, I420)
YUVTORGB_FUNC(BGR32, I420)
YUVTORGB_FUNC(RGB24, I420)                     
YUVTORGB_FUNC(RGB565, I420)
YUVTORGB_FUNC(RGB555, I420)
YUVTORGB_FUNC(RGB444, I420)
YUVTORGB_FUNC(RGB8, I420)

YUVTORGB_FUNC(RGB32, YV12)
YUVTORGB_FUNC(BGR32, YV12)
YUVTORGB_FUNC(RGB24, YV12)
YUVTORGB_FUNC(RGB565, YV12)
YUVTORGB_FUNC(RGB555, YV12)
YUVTORGB_FUNC(RGB444, YV12)
YUVTORGB_FUNC(RGB8, YV12)


#else                  

#define YUVTORGB_FUNC(df)                                   \
    int FN(df,I420) (unsigned char *dest_ptr,               \
        int dest_width, int dest_height, int dest_pitch,    \
        int dest_x, int dest_y, int dest_dx, int dest_dy,   \
        unsigned char *src_ptr,                             \
        int src_width, int src_height, int src_pitch,       \
        int src_x, int src_y, int src_dx, int src_dy)       \
    {                                                       \
        return YUVtoRGB(                                    \
            ID(df), dest_ptr, dest_width, dest_height,      \
            dest_pitch, dest_x, dest_y, dest_dx, dest_dy,   \
            src_ptr, src_width, src_height,                 \
            src_pitch, src_x, src_y, src_dx, src_dy);       \
    }

YUVTORGB_FUNC(RGB32 )
YUVTORGB_FUNC(BGR32 )
YUVTORGB_FUNC(RGB24 )
YUVTORGB_FUNC(RGB565)
YUVTORGB_FUNC(RGB555)
YUVTORGB_FUNC(RGB444)
YUVTORGB_FUNC(RGB8  )

#endif

/*
 * The main YUVtoRGB2 converter:
 */
static int YUVtoRGB2 (
    /* destination format: */
    int dest_format,
    unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    unsigned char *pY, unsigned char *pU, unsigned char *pV,
    int src_width, int src_height, int yPitch, int uPitch, int vPitch,
    int src_x, int src_y, int src_dx, int src_dy)

{
    /* pointers to low-level converters to use: */
    void (*dblrow_proc) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int);
    void (*dblrow2x_proc) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int);
    void (*image_proc) (unsigned char *dest_ptr,
        int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
        unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
        int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
        void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
            unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
        void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
            unsigned char *, unsigned char *, int, int, unsigned char *,
            unsigned char *, unsigned char *, unsigned char *, int, int));

    /* scale types: */
    register int scale_x, scale_y;

    /* pointers and destination pixel depth: */
    register unsigned char *d, *sy, *su, *sv;
    register int dest_bpp;

    /* check arguments: */
    if (
        /* alignments: */
        ((unsigned)dest_ptr & 3) || (dest_pitch & 3) ||
        ((unsigned)pY  & 3) || (yPitch & 3) ||
        /* image sizes: */
        dest_width <= 0 || dest_height <= 0 ||
        src_width  <= 0 || src_height  <= 0 ||
        /* rectangles: */
        dest_x < 0 || dest_y < 0 || dest_dx <= 0 || dest_dy <= 0 ||
        src_x  < 0 || src_y  < 0 || src_dx  <= 0 || src_dy  <= 0 ||
        /* overlaps: */
        dest_width < dest_x + dest_dx || dest_height < dest_y + dest_dy ||
        src_width  < src_x  + src_dx  || src_height  < src_y  + src_dy)
    {
        /* fail: */
        return -1;
    }

    /* get scale types: */
    GET_SCALE_TYPE(dest_dx, src_dx, scale_x);
    GET_SCALE_TYPE(dest_dy, src_dy, scale_y);

    /* select row and image converters: */
#ifdef _PLUS_HXCOLOR
    dblrow_proc   = DblRowFuncs [is_alpha][dest_format][scale_x];
    dblrow2x_proc = DblRow2xFuncs [is_alpha][dest_format][scale_x];

#else
    dblrow_proc   = DblRowFuncs [0][dest_format][scale_x];
    dblrow2x_proc = DblRow2xFuncs [0][dest_format][scale_x];
#endif
    image_proc    = ImageFuncs [scale_y];

    /* get destination pixel depth: */
    dest_bpp = bpp [dest_format];

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (yPitch <= 0 ||
        uPitch <= 0 ||
        vPitch <= 0)
        return -1;          /* not supported */

    /* get pointers: */
    d = dest_ptr + dest_x * dest_bpp + dest_y * dest_pitch;
    sy = pY + (src_x + src_y * yPitch); /* luminance */
    su = pU + (src_x/2 + src_y/2 * uPitch); /* !!! */
    sv = pV + (src_x/2 + src_y/2 * vPitch); /* !!! */

    /* pass control to appropriate lower-level converters: */
    (* image_proc) (
        d, dest_x, dest_y, dest_dx, dest_dy, dest_pitch, dest_bpp,
        sy, su, sv, src_x, src_y, src_dx, src_dy, yPitch, uPitch,
        dblrow_proc, dblrow2x_proc);

    /* success: */
    return 0;
}


/*
 * Public format-conversion routines.
 * Use:
 *  int XXXXtoYYYY (unsigned char *dest_ptr, int dest_width, int dest_height,
 *      int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
 *      unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
 *      int src_x, int src_y, int src_dx, int src_dy);
 * Input:
 *  dest_ptr - pointer to a destination buffer
 *  dest_width, dest_height - width/height of the destination image (pixels)
 *  dest_pitch - pitch of the dest. buffer (in bytes; <0 - if bottom up image)
 *  dest_x, dest_y, dest_dx, dest_dy - destination rectangle (pixels)
 *  pY - pointer to the y plane
 *  pU - pointer to the u plane
 *  pV - pointer to the v plane
 *  src_width, src_height - width/height of the input image (pixels)
 *  yPitch - pitch of the y buffer (in bytes; <0 - if bottom up image)
 *  uPitch - pitch of the u buffer (in bytes; <0 - if bottom up image)
 *  vPitch - pitch of the v buffer (in bytes; <0 - if bottom up image)
 *  src_x, src_y, src_dx, src_dy - source rectangle (pixels)
 * Returns:
 *  0 - if success; -1 if failure.
 * Note:
 *  Both source and destination buffers must be 4-bytes aligned,
 *  and their pitches (#of bytes in row) shall be multiple of 4!!!
 */

#if 1

#define YUVTORGB2_FUNC(df, sf)                                   \
    int FN2(df, sf) (unsigned char *dest_ptr, int dest_width,   \
    int dest_height, int dest_pitch, int dest_x, int dest_y,    \
    int dest_dx, int dest_dy, unsigned char *pY,                \
    unsigned char *pU, unsigned char *pV, int src_width,        \
    int src_height, int yPitch, int uPitch, int vPitch,         \
    int src_x, int src_y, int src_dx, int src_dy)               \
    {                                                           \
        return YUVtoRGB2(                                       \
            ID(df), dest_ptr, dest_width, dest_height,          \
            dest_pitch, dest_x, dest_y, dest_dx, dest_dy,       \
            pY, pU, pV, src_width, src_height, yPitch,          \
            uPitch, vPitch, src_x, src_y, src_dx, src_dy);      \
    }

YUVTORGB2_FUNC(RGB32, I420)
YUVTORGB2_FUNC(BGR32, I420)
YUVTORGB2_FUNC(RGB24, I420)
YUVTORGB2_FUNC(RGB565, I420)
YUVTORGB2_FUNC(RGB555, I420)
YUVTORGB2_FUNC(RGB444, I420)
YUVTORGB2_FUNC(RGB8, I420)

YUVTORGB2_FUNC(RGB32, YV12)
YUVTORGB2_FUNC(BGR32, YV12)
YUVTORGB2_FUNC(RGB24, YV12)
YUVTORGB2_FUNC(RGB565, YV12)
YUVTORGB2_FUNC(RGB555, YV12)
YUVTORGB2_FUNC(RGB444, YV12)
YUVTORGB2_FUNC(RGB8, YV12)


#else

#define YUVTORGB2_FUNC(df)                                      \
    int FN2(df,I420) (unsigned char *dest_ptr, int dest_width,  \
    int dest_height, int dest_pitch, int dest_x, int dest_y,    \
    int dest_dx, int dest_dy, unsigned char *pY,                \
    unsigned char *pU, unsigned char *pV, int src_width,        \
    int src_height, int yPitch, int uPitch, int vPitch,         \
    int src_x, int src_y, int src_dx, int src_dy)               \
    {                                                           \
        return YUVtoRGB2(                                       \
            ID(df), dest_ptr, dest_width, dest_height,          \
            dest_pitch, dest_x, dest_y, dest_dx, dest_dy,       \
            pY, pU, pV, src_width, src_height, yPitch,          \
            uPitch, vPitch, src_x, src_y, src_dx, src_dy);      \
    }

YUVTORGB2_FUNC(RGB32 )
YUVTORGB2_FUNC(BGR32 )
YUVTORGB2_FUNC(RGB24 )
YUVTORGB2_FUNC(RGB565)
YUVTORGB2_FUNC(RGB555)
YUVTORGB2_FUNC(RGB444)
YUVTORGB2_FUNC(RGB8  )

#endif

#if 0
/***********************************************************/

/*
 * New XINGtoRGB converter:
 */
static int XINGtoRGB (
    /* destination format: */
    int dest_format,
    /* destination image parameters: */
    unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    /* source image parameters: */
    unsigned char *src_ptr, int src_width, int src_height,
    int src_pitch, int src_x, int src_y, int src_dx, int src_dy)
{
    /* pointers to low-level converters to use: */
    void (*dblrow_proc) (unsigned char *, unsigned char *, int, int,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int);
    void (*dblrow2x_proc) (unsigned char *, unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, int, int, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, int, int);
    void (*image_proc) (unsigned char *dest_ptr,
        int dest_x, int dest_y, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
        unsigned char *src_y_ptr, unsigned char *src_u_ptr, unsigned char *src_v_ptr,
        int src_x, int src_y, int src_dx, int src_dy, int src_pitch, int src_pitch_2,
        void (*dblrow_func) (unsigned char *, unsigned char *, int, int,
            unsigned char *, unsigned char *, unsigned char *, unsigned char *, int, int),
        void (*dblrow2x_func) (unsigned char *, unsigned char *, unsigned char *,
            unsigned char *, unsigned char *, int, int, unsigned char *,
            unsigned char *, unsigned char *, unsigned char *, int, int));

    /* scale types: */
    register int scale_x, scale_y;

    /* pointers and destination pixel depth: */
    register unsigned char *d, *sy, *su, *sv;
    register int dest_bpp;

    /* check arguments: */
    if (
        /* alignments: */
        ((unsigned)dest_ptr & 3) || (dest_pitch & 3) ||
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
    {
        /* fail: */
        return -1;
    }

    /* get scale types: */
    GET_SCALE_TYPE(dest_dx, src_dx, scale_x);
    GET_SCALE_TYPE(dest_dy, src_dy, scale_y);

    /* select row and image converters: */
#ifdef _PLUS_HXCOLOR
    dblrow_proc   = DblRowFuncs [is_alpha][dest_format][scale_x];
    dblrow2x_proc = DblRow2xFuncs [is_alpha][dest_format][scale_x];
#else
    dblrow_proc   = DblRowFuncs [0][dest_format][scale_x];
    dblrow2x_proc = DblRow2xFuncs [0][dest_format][scale_x];
#endif
    image_proc    = ImageFuncs [scale_y];

    /* get destination pixel depth: */
    dest_bpp = bpp [dest_format];

    /* check if bottop-up images: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (src_pitch <= 0) return -1;          /* not supported */

    /*
     * XING is essentially a planar YUV 4:2:0 format
     * with common pitch and interleaved UV planes:
     *
     *  +-------------+
     *  |      Y      |
     *  |             |  next_line = Y/U/V + pitch;
     *  +------+------+
     *  |  U   |  V   |  V = U + pitch / 2;
     *  +------+------+
     */

    /* get pointers: */
    d = dest_ptr + dest_x * dest_bpp + dest_y * dest_pitch;
    sy = src_ptr + (src_x + src_y * src_pitch); /* luminance */
    
    su = src_ptr + (src_height+15 & 0xFFF0) * src_pitch + (src_x/2 + src_y/2 * src_pitch);
    sv = su + src_pitch / 2;

    /* pass control to appropriate lower-level converters: */
    (* image_proc) (
        d, dest_x, dest_y, dest_dx, dest_dy, dest_pitch, dest_bpp,
        sy, su, sv, src_x, src_y, src_dx, src_dy, src_pitch, src_pitch,
        dblrow_proc, dblrow2x_proc);

    /* success: */
    return 0;
}

#endif //0

/*
 * Public format-conversion routines.
 */
#define XINGTORGB_FUNC(df)                                  \
    int FN(df,XING) (unsigned char *dest_ptr,               \
        int dest_width, int dest_height, int dest_pitch,    \
        int dest_x, int dest_y, int dest_dx, int dest_dy,   \
        unsigned char *src_ptr,                             \
        int src_width, int src_height, int src_pitch,       \
        int src_x, int src_y, int src_dx, int src_dy)       \
    {                                                       \
        unsigned char *pU = src_ptr + (src_height+15 & 0xFFF0) * src_pitch; \
        unsigned char *pV = pU + (src_pitch>>1);                    \
                                                                    \
        return YUVtoRGB2(                                           \
            ID(df), dest_ptr, dest_width, dest_height,              \
            dest_pitch, dest_x, dest_y, dest_dx, dest_dy,           \
            src_ptr, pU, pV, src_width, src_height, src_pitch,      \
            src_pitch, src_pitch, src_x, src_y, src_dx, src_dy);    \
    } 

        //return XINGtoRGB(                                   \
        //    ID(df), dest_ptr, dest_width, dest_height,      \
        //    dest_pitch, dest_x, dest_y, dest_dx, dest_dy,   \
        //    src_ptr, src_width, src_height,                 \
        //    src_pitch, src_x, src_y, src_dx, src_dy);       \

    //}

XINGTORGB_FUNC(RGB32 )
XINGTORGB_FUNC(BGR32 )
XINGTORGB_FUNC(RGB24 )
XINGTORGB_FUNC(RGB565)
XINGTORGB_FUNC(RGB555)
XINGTORGB_FUNC(RGB8  )

/***********************************************************/

/*
 * Obsolete I420->RGB converters:
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

#ifdef _PLUS_HXCOLOR

#define OLD_YUVTORGB_FUNC(df)                               \
    void oldI420to##df (unsigned char *ysrc,                \
        unsigned char *usrc, unsigned char *vsrc, int pitchSrc, \
        unsigned char *dst, int width, int height, int pitchDst)\
    {                                                       \
        /* correct pointer if bottom-up bitmap: */          \
        if (pitchDst < 0)                                   \
            dst += BPP(df) * -pitchDst * (height - 1);      \
        /* invoke color converter: */                       \
        IMAGE_COPY (dst, 0, 0, width, height,               \
            pitchDst*BPP(df), BPP(df),                      \
            ysrc, usrc, vsrc, 0, 0, width, height,          \
            pitchSrc, pitchSrc / 2,                         \
            is_alpha? DBLROW_FN(df,I420,FULL,COPY)          \
                    : DBLROW_FN(df,I420,FAST,COPY),         \
            is_alpha? DBLROW2X_FN(df,I420,FULL,COPY)        \
                    : DBLROW2X_FN(df,I420,FAST,COPY));      \
    }

#define OLD_YUVTORGB_X2_FUNC(df)                            \
    void oldI420to##df##x2 (unsigned char *ysrc,            \
        unsigned char *usrc, unsigned char *vsrc, int pitchSrc, \
        unsigned char *dst, int width, int height, int pitchDst)\
    {                                                       \
        /* correct pointer if bottom-up bitmap: */          \
        if (pitchDst < 0)                                   \
            dst += BPP(df) * -pitchDst * (2*height - 1);    \
        /* invoke color converter: */                       \
        IMAGE_STRETCH2X (dst, 0, 0, 2*width, 2*height,      \
            pitchDst*BPP(df), BPP(df),                      \
            ysrc, usrc, vsrc, 0, 0, width, height,          \
            pitchSrc, pitchSrc / 2,                         \
            is_alpha? DBLROW_FN(df,I420,FULL,STRETCH2X)     \
                    : DBLROW_FN(df,I420,FAST,STRETCH2X),    \
            is_alpha? DBLROW2X_FN(df,I420,FULL,STRETCH2X)   \
                    : DBLROW2X_FN(df,I420,FAST,STRETCH2X)); \
    }

#else

#define OLD_YUVTORGB_FUNC(df)                               \
    void oldI420to##df (unsigned char *ysrc,                \
        unsigned char *usrc, unsigned char *vsrc, int pitchSrc, \
        unsigned char *dst, int width, int height, int pitchDst)\
    {                                                       \
        /* correct pointer if bottom-up bitmap: */          \
        if (pitchDst < 0)                                   \
            dst += BPP(df) * -pitchDst * (height - 1);      \
        /* invoke color converter: */                       \
        IMAGE_COPY (dst, 0, 0, width, height,               \
            pitchDst*BPP(df), BPP(df),                      \
            ysrc, usrc, vsrc, 0, 0, width, height,          \
            pitchSrc, pitchSrc / 2,                         \
	    DBLROW_FN(df,I420,FAST,COPY),		    \
            DBLROW2X_FN(df,I420,FAST,COPY));		    \
    }

#define OLD_YUVTORGB_X2_FUNC(df)                            \
    void oldI420to##df##x2 (unsigned char *ysrc,            \
        unsigned char *usrc, unsigned char *vsrc, int pitchSrc, \
        unsigned char *dst, int width, int height, int pitchDst)\
    {                                                       \
        /* correct pointer if bottom-up bitmap: */          \
        if (pitchDst < 0)                                   \
            dst += BPP(df) * -pitchDst * (2*height - 1);    \
        /* invoke color converter: */                       \
        IMAGE_STRETCH2X (dst, 0, 0, 2*width, 2*height,      \
            pitchDst*BPP(df), BPP(df),                      \
            ysrc, usrc, vsrc, 0, 0, width, height,          \
            pitchSrc, pitchSrc / 2,                         \
	    DBLROW_FN(df,I420,FAST,STRETCH2X),		    \
            DBLROW2X_FN(df,I420,FAST,STRETCH2X));	    \
    }

#endif



OLD_YUVTORGB_FUNC(RGB32 )
OLD_YUVTORGB_FUNC(RGB24 )
OLD_YUVTORGB_FUNC(RGB565)
OLD_YUVTORGB_FUNC(RGB555)

OLD_YUVTORGB_X2_FUNC(RGB32 )
OLD_YUVTORGB_X2_FUNC(RGB24 )
OLD_YUVTORGB_X2_FUNC(RGB565)
OLD_YUVTORGB_X2_FUNC(RGB555)


/***********************************************************/
#ifdef TEST
/*
 * YUV2RGB: converts QCIF YUV image to an RGB bitmap
 * Use:
 *  YUV2RGB <output file> <input file> [<new width> <new height>] [<new format>]
 */

/* Note: use /Zp1 (no structure alignments) to compile it !!! */

/* Windows Header Files: */
#include <windows.h>

/* C RunTime Header Files */
#include <stdio.h>
#include <stdlib.h>

/* SetDestI420Colors() interface: */
#include "colorlib.h"

/* Main program: */
int main (int argc, char *argv[])
{
	/* frame buffers: */
#	define	QCIF_HEIGHT	144
#	define	QCIF_WIDTH	176
#	define	QCIF_FRAME_SIZE	\
			(QCIF_HEIGHT * QCIF_WIDTH + QCIF_HEIGHT * QCIF_WIDTH / 2)
#   define  MAX_HEIGHT 1024
#   define  MAX_WIDTH  1280
#   define  MAX_RGB_FRAME_SIZE \
            (MAX_HEIGHT * MAX_WIDTH * 4)
    static unsigned char yuv [QCIF_FRAME_SIZE]; /* Flawfinder: ignore */
    static struct BMP {
        BITMAPFILEHEADER hdr;           /* bitmap file-header */
        BITMAPINFOHEADER bihdr;         /* bitmap info-header */
        unsigned char rgb [MAX_RGB_FRAME_SIZE + 256*4 + 3]; /* Flawfinder: ignore */
    } bmp;

    /* local variables: */
    FILE *ifp, *ofp;
    int width = QCIF_WIDTH, height = QCIF_HEIGHT, format = ID(RGB32);
    int offs, colors, sz;

    /* process program arguments: */
    if (argc < 3) {
        fprintf (stderr, "Use:\n\tYUV2RGB <output file> <input file> [width] [height] [format]\n");
		exit (EXIT_FAILURE);
	}
    /* open work files: */
    if ((ofp = fopen (argv[1], "wb+")) == NULL) { /* Flawfinder: ignore */
        fprintf (stderr, "Cannot open output file.\n");
		exit (EXIT_FAILURE);
	}
	if ((ifp = fopen (argv[2], "rb")) == NULL) { /* Flawfinder: ignore */
        fprintf (stderr, "Cannot open input file.\n");
		exit (EXIT_FAILURE);
	}
    /* retrieve optional arguments: */
    if ((argc >= 4)
     && (width = atoi(argv[3])) == 0) {
        fprintf (stderr, "Invalid image width.\n");
		exit (EXIT_FAILURE);
    }
    if ((argc >= 5)
     && (height = atoi(argv[4])) == 0) {
        fprintf (stderr, "Invalid image height.\n");
		exit (EXIT_FAILURE);
    }
    if ((argc >= 6)
     && (format = atoi(argv[5])) == 0) {
        fprintf (stderr, "Invalid image format.\n");
		exit (EXIT_FAILURE);
     }

	/* read YVU12 frame: */
    if (fread (yuv, 1, QCIF_FRAME_SIZE, ifp) != QCIF_FRAME_SIZE) {
		fprintf (stderr, "Cannot read input file.\n");
		exit (EXIT_FAILURE);
	}

    /* initialize I420toRGB converters: */
    SetSrcI420Colors (0., 0., 0., 0.);

    /* calculate size of bitmap: */
    sz = height * width * bpp [format];     /* image size in bytes */
    offs = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); /* offset to the array of color indices */
    colors = 0;
    if (format == ID(RGB8)) {
        /* generate palette: */
#if 0
        static unsigned int pal_ptr[256], pal_idx[256];
        unsigned int *pal_bmp_ptr = (unsigned int *)((unsigned char *)&bmp + offs);
        register int i;
        for (i = 0; i < 256; i++) pal_idx[i] = i;
        colors = SuggestDestRGB8Palette (0, pal_ptr);
#else
        /* palette from AIX player: */
        static unsigned int pal_ptr[256] = {
            0x000000, 0xffffff, 0x737373, 0xa8b2c6, 0x70a8ed, 0x5f8ec9,
            0xc0d9f7, 0x3b597d, 0xffffff, 0x999999, 0x828282, 0xd2d1d1,
            0x4e4e4e, 0xffffff, 0xaa9889, 0x908174, 0xd9d1ca, 0x564d46,
            0xffffff, 0x655650, 0x554944, 0xb7b0ad, 0x2e2724, 0xffffff,
            0xa8b2c6, 0x8f98a8, 0xdadee7, 0x5a606a, 0x000000, 0xa79249,
            0x8d7c3e, 0xd7cead, 0x310800, 0x800000, 0x840204, 0xce2900,
            0xff0000, 0x0c0f0c, 0x3c3700, 0x6b2900, 0x943900, 0xce2900,
            0xf75a00, 0x008000, 0x3c3700, 0x675f1a, 0x997f19, 0xde6300,
            0xff6300, 0x008000, 0x008000, 0x808000, 0x808000, 0xff9200,
            0xff9200, 0x00ff00, 0x55ff24, 0x55ff24, 0x808000, 0xff9200,
            0xff9200, 0x00ff00, 0x55ff24, 0x55ff24, 0x55ff24, 0xcece5a,
            0xffff00, 0x000029, 0x311818, 0x6f2529, 0x840204, 0xff0029,
            0xff0029, 0x002929, 0x2e333b, 0x663333, 0x8b3d48, 0xbd425a,
            0xff0029, 0x005a39, 0x424a42, 0x636339, 0x9c6329, 0xd66b18,
            0xd66b18, 0x299c5a, 0x299c5a, 0x887e2f, 0xa79a42, 0xa79a42,
            0xff9200, 0x00ff00, 0x299c5a, 0x55ff24, 0xa79a42, 0xcece5a,
            0xcece5a, 0x00ff00, 0x55ff24, 0x55ff24, 0x55ff24, 0xcece5a,
            0xffff00, 0x080852, 0x080852, 0x660099, 0x8b3d48, 0xbd425a,
            0xff0029, 0x004263, 0x313163, 0x655650, 0x8b3d48, 0xbd425a,
            0xbd425a, 0x004263, 0x29635a, 0x6d6d6d, 0xa0766f, 0xcc6666,
            0xcc6666, 0x008080, 0x299c5a, 0x6b736b, 0x908174, 0xd6a58c,
            0xff9999, 0x299c5a, 0x299c5a, 0x5a9c9c, 0xcece5a, 0xcece5a,
            0xf7ce8c, 0x00ff00, 0x55ff24, 0x55ff24, 0xcece5a, 0xcece5a,
            0xffff9c, 0x000080, 0x29299c, 0x660099, 0x660099, 0xbd425a,
            0xff0029, 0x101884, 0x29299c, 0x4a42bd, 0x660099, 0xbd425a,
            0xcc6666, 0x00639c, 0x29638c, 0x6b849c, 0x848284, 0xcc6666,
            0xff9999, 0x009cad, 0x427ba5, 0x5a9c9c, 0xd6a58c, 0xff9999,
            0x009cad, 0x5a9c9c, 0x5a9c9c, 0xc6ce94, 0xcece9c, 0xf7ce8c,
            0x04fefc, 0x5a9c9c, 0x5a9c9c, 0xceff9c, 0xceff9c, 0xffff9c,
            0x0000ff, 0x29299c, 0x660099, 0x660099, 0x660099, 0xbd425a,
            0x29299c, 0x4a42bd, 0x4a42bd, 0x4a42bd, 0xcc6666, 0xff9999,
            0x00639c, 0x4a42bd, 0x5f8ec9, 0x8f98a8, 0xceadad, 0xff9999,
            0x009cad, 0x5a9cce, 0x639cc6, 0x9cb5ce, 0xceadad, 0xff9999,
            0x00ceff, 0x5a9cce, 0x639cc6, 0x9cb5ce, 0xcccccc, 0xd9d1ca,
            0x04fefc, 0x04fefc, 0x9cb5ce, 0xc0d9f7, 0xe4e4e4, 0xffffce,
            0x0000ff, 0x0000ff, 0x4a42bd, 0x660099, 0x660099, 0xff00ff,
            0x0000ff, 0x4a42bd, 0x4a42bd, 0x4a42bd, 0xff00ff, 0xff00ff,
            0x009cef, 0x009cef, 0x5f8ec9, 0x70a8ed, 0xa8b2c6, 0xff00ff,
            0x009cef, 0x009cef, 0x70a8ed, 0x70a8ed, 0xc0d9f7, 0xdadee7,
            0x00ceff, 0x00ceff, 0x70a8ed, 0xc0d9f7, 0xc0d9f7, 0xe4e4e4,
            0x00ffff, 0x04fefc, 0x70a8ed, 0xccffff, 0xccffff
        },
        pal_idx[256] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
            19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 157, 239, 250,
            163, 240, 104, 124, 182, 156, 35, 158, 231, 38, 131, 133, 187, 166,
            43, 43, 135, 51, 141, 53, 230, 144, 56, 51, 53, 53, 230, 56, 56,
            56, 215, 54, 218, 244, 97, 34, 243, 71, 220, 105, 252, 88, 226, 71,
            208, 184, 132, 227, 229, 83, 217, 85, 134, 120, 88, 53, 55, 85, 56,
            88, 65, 65, 55, 56, 56, 56, 65, 66, 203, 103, 248, 76, 77, 71, 225,
            206, 19, 76, 77, 77, 109, 207, 142, 121, 253, 119, 234, 85, 160,
            15, 235, 254, 85, 85, 209, 65, 65, 214, 55, 56, 56, 65, 65, 211,
            232, 192, 248, 105, 77, 71, 204, 140, 205, 105, 77, 119, 221, 193,
            195, 247, 119, 126, 224, 197, 129, 125, 254, 157, 129, 129, 216,
            212, 132, 251, 129, 129, 213, 171, 138, 233, 140, 105, 105, 105,
            77, 140, 147, 147, 147, 119, 126, 151, 147, 5, 25, 228, 126, 157,
            202, 201, 196, 190, 126, 222, 193, 194, 195, 113, 16, 168, 168,
            195, 6, 255, 210, 233, 174, 147, 105, 105, 55, 174, 147, 147, 147,
            215, 215, 223, 222, 5, 4, 3, 215, 222, 222, 4, 4, 6, 26, 198, 198,
            4, 6, 6, 208, 53, 168, 4, 249, 249
        };
        unsigned int *pal_bmp_ptr = (unsigned int *)((unsigned char *)&bmp + offs);
        register int i;
        colors = 256;
#endif
        SetSrcRGB8Palette (colors, pal_ptr, pal_idx);
        SetDestRGB8Palette (colors, pal_ptr, pal_idx);
        /* copy it to bitmap: */
        for (i = 0; i < colors; i++) {
            register int j = palette[i];
            pal_bmp_ptr[i] = ((j & 0xFF) << 16) | (j & 0xFF00) | ((j & 0xFF0000) >> 16);
        }
        /* shift bitmap offset: */
        offs += colors * 4;
    }
    offs = (offs + 3) & ~3; /* round to the nearest dword */

	/* create BMP file header */
    bmp.hdr.bfType = 0x4d42;            // "BM"
    bmp.hdr.bfSize = sz;
    bmp.hdr.bfReserved1 = 0;
    bmp.hdr.bfReserved2 = 0;
    bmp.hdr.bfOffBits = offs;

    /* create bitmap header: */
    bmp.bihdr.biSize = sizeof(BITMAPINFOHEADER);
    bmp.bihdr.biWidth = width;
    bmp.bihdr.biHeight = height;
    bmp.bihdr.biPlanes = 1;            // must be 1.
    bmp.bihdr.biCompression = BI_RGB;
    bmp.bihdr.biBitCount = bpp [format] * 8;
    bmp.bihdr.biSizeImage = sz;
    bmp.bihdr.biXPelsPerMeter = 0;
    bmp.bihdr.biYPelsPerMeter = 0;
    bmp.bihdr.biClrUsed = colors;
    bmp.bihdr.biClrImportant = colors;

    /* convert image: */
    YUVtoRGB (format,
        (unsigned char *)&bmp + offs, width, height, -width * bpp[format], 0, 0, width, height,
        yuv, QCIF_WIDTH, QCIF_HEIGHT, QCIF_WIDTH, 0, 0, QCIF_WIDTH, QCIF_HEIGHT);

	/* write bitmap file: */
    if ((int)fwrite (&bmp, 1, sz+offs, ofp) != sz+offs) {
		fprintf (stderr, "Cannot create BMP file.\n");
		exit (EXIT_FAILURE);
	}

	/* close files & exit: */
	fclose (ifp);
	fclose (ofp);

    return EXIT_SUCCESS;
}
#endif

/* yuv2rgb.c -- end of file */

