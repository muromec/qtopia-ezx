/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rgb2yuv.c,v 1.4 2004/12/29 02:07:37 rascar Exp $
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

#include "env.h"
#include "rgb.h"    /* basic RGB-data definitions & macros */
#include "yuv.h"    /* YUV-to-RGB conversion tables & macros */
#include "clip.h"   /* macros for clipping & dithering */
#include "scale.h"  /* scale algorithms */
#include "colorlib.h"

/*** RGB to YUV color converters: **************************/

/*
 * These functions were borrowed from our "old" color conversion library,
 * and they do not support things like stretching or color adjustments.
 * Given some more time this module should be completely rewritten.
 *
 ***************/

/*-----------------2/24/2002 7:45AM-----------------
 * Replaced the old code with macros.
 * In-place resizing and color adjustments are not added since nobody
 * really asked for it, and it would have blown the code by factor of 6 (at least).
 * Yuriy.
 * --------------------------------------------------*/


/*-----------------9/19/2002 2:06PM-----------------
 * Added support for byte-swapped RGB32 (BGR_32) and BGR24 input formats.
 * To avoid changes (table reordering) in other modules, the definitions
 * related to these formats are placed here, instead of "rgb.h".
 * Yuriy.
 * --------------------------------------------------*/


/* new RGB format IDs: */
#define BGR_32_ID           RGB_FORMATS
#define BGR24_ID            (RGB_FORMATS + 1)


/* bytes per pixel: */
#define BGR_32_BPP          4
#define BGR24_BPP           3


/* local pixel representations: */
#define BGR_32_PIXEL(a)     RGBX_PIXEL(a)
#define BGR24_PIXEL(a)      RGB24_PIXEL(a)


/* load/store/copy pixel macros: */
#define BGR_32_LOAD(s,a)    RGBX_LOAD(s,a,unsigned int)
#define BGR_32_STORE(d,a)   RGBX_STORE(d,a,unsigned int)
#define BGR_32_COPY(da,sa)  RGBX_COPY(da,sa)


#define BGR24_LOAD(s,a)     a##_r=(s)[0], a##_g=(s)[1], a##_b=(s)[2]
#define BGR24_STORE(d,a)    (d)[0]=a##_r, (d)[1]=a##_g, (d)[2]=a##_b
#define BGR24_COPY(da,sa)   da##_r=sa##_r, da##_g=sa##_g, da##_b=sa##_b


/* BGR_32 bit fields: 0xBBGGRR00: */
#define BGR_32_R_START      8
#define BGR_32_R_BITS       8
#define BGR_32_G_START      16
#define BGR_32_G_BITS       8
#define BGR_32_B_START      24
#define BGR_32_B_BITS       8


/* fields extraction/assignment macros: */
#define BGR_32_GET_R(a)     RGBX_GET_X(BGR_32,R,a)
#define BGR_32_GET_G(a)     RGBX_GET_X(BGR_32,G,a)
#define BGR_32_GET_B(a)     RGBX_GET_X(BGR_32,B,a)
#define BGR_32_SET(a,r,g,b) RGBX_SET(  BGR_32,a,r,g,b)


#define BGR24_GET_R(a)      (a##_r)
#define BGR24_GET_G(a)      (a##_g)
#define BGR24_GET_B(a)      (a##_b)
#define BGR24_SET(a,r,g,b)  a##_b=(b), a##_g=(g), a##_r=(r)


/***************************************************************/


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
#define CHROMA_RESAMPLING_MODES     3

/*
 * 4x2-block load-convert-store macros:
 */

/* CRM_11_11_Interlace  chroma resampling: */
#define CONVERT_4x2_CHROMA_11_11_Interlace(dy1,dy2,dy3,dy4,du,du2,dv,dv2,sf,s1,s2,s3,s4)  \
    {                                                               \
        /* local variables: */                                      \
        PIXEL(sf,x);                                                \
        register unsigned int r, b, y;                              \
        register unsigned int r41, b41, y41;   /* combined quant-s */  \
        register unsigned int r42, b42, y42;   /* combined quant-s */  \
                                                                    \
        /* process lumas: */                                        \
        LOAD(sf,s1,x);              /* [0,0] */                     \
        r41 = (r = GET_R(sf,x));                                    \
        b41 = (b = GET_B(sf,x));                                    \
        y41 = (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);    \
        dy1[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s1+BPP(sf),x);      /* [0,1] */                     \
        r41 += (r = GET_R(sf,x));                                   \
        b41 += (b = GET_B(sf,x));                                   \
        y41 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);   \
        dy1[1] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2,x);              /* [1,0] */                     \
        r42 = (r = GET_R(sf,x));                                    \
        b42 = (b = GET_B(sf,x));                                    \
        y42 = (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);    \
        dy2[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2+BPP(sf),x);      /* [1,1] */                     \
        r42 += (r = GET_R(sf,x));                                   \
        b42 += (b = GET_B(sf,x));                                   \
        y42 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);   \
        dy2[1] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s3,x);              /* [2,0] */                     \
        r41 += (r = GET_R(sf,x));                                   \
        b41 += (b = GET_B(sf,x));                                   \
        y41 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);   \
        dy3[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s3+BPP(sf),x);      /* [2,1] */                     \
        r41 += (r = GET_R(sf,x));                                   \
        b41 += (b = GET_B(sf,x));                                   \
        y41 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);   \
        dy3[1] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s4,x);              /* [3,0] */                     \
        r42 += (r = GET_R(sf,x));                                   \
        b42 += (b = GET_B(sf,x));                                   \
        y42 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);   \
        dy4[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s4+BPP(sf),x);      /* [3,1] */                     \
        r42 += (r = GET_R(sf,x));                                   \
        b42 += (b = GET_B(sf,x));                                   \
        y42 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);   \
        dy4[1] = yytab [y];                                         \
                                                                    \
        /* average chromas: */                                      \
        dv[0] = vrytab [VMIN+(r41-y41)/4];                          \
        du[0] = ubytab [UMIN+(b41-y41)/4];                          \
        /* average chromas: */                                      \
        dv2[0] = vrytab [VMIN+(r42-y42)/4];                         \
        du2[0] = ubytab [UMIN+(b42-y42)/4];                         \
    }

/*
 * 2x2-block load-convert-store macros:
 */


/* CRM_11_11 (both lines) chroma resampling: */
#define CONVERT_2x2_CHROMA_11_11(dy1,dy2,du,dv,sf,s1,s2)            \
    {                                                               \
        /* local variables: */                                      \
        PIXEL(sf,x);                                                \
        register unsigned int r, b, y;                              \
        register unsigned int r4, b4, y4;   /* combined quant-s */  \
                                                                    \
        /* process lumas: */                                        \
        LOAD(sf,s1,x);              /* [0,0] */                     \
        r4 = (r = GET_R(sf,x));                                     \
        b4 = (b = GET_B(sf,x));                                     \
        y4 = (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);     \
        dy1[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s1+BPP(sf),x);      /* [0,1] */                     \
        r4 += (r = GET_R(sf,x));                                    \
        b4 += (b = GET_B(sf,x));                                    \
        y4 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);    \
        dy1[1] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2,x);              /* [1,0] */                     \
        r4 += (r = GET_R(sf,x));                                    \
        b4 += (b = GET_B(sf,x));                                    \
        y4 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);    \
        dy2[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2+BPP(sf),x);      /* [1,1] */                     \
        r4 += (r = GET_R(sf,x));                                    \
        b4 += (b = GET_B(sf,x));                                    \
        y4 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);    \
        dy2[1] = yytab [y];                                         \
                                                                    \
        /* average chromas: */                                      \
        dv[0] = vrytab [VMIN+(r4-y4)/4];                            \
        du[0] = ubytab [UMIN+(b4-y4)/4];                            \
    }


/* CRM_11_00 (upper line) chroma resampling: */
#define CONVERT_2x2_CHROMA_11_00(dy1,dy2,du,dv,sf,s1,s2)            \
    {                                                               \
        /* local variables: */                                      \
        PIXEL(sf,x);                                                \
        register unsigned int r, b, y;                              \
        register unsigned int r2, b2, y2;   /* 1st line combined */ \
                                                                    \
        /* process lumas: */                                        \
        LOAD(sf,s1,x);              /* [0,0] */                     \
        r2 = (r = GET_R(sf,x));                                     \
        b2 = (b = GET_B(sf,x));                                     \
        y2 = (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);     \
        dy1[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s1+BPP(sf),x);      /* [0,1] */                     \
        r2 += (r = GET_R(sf,x));                                    \
        b2 += (b = GET_B(sf,x));                                    \
        y2 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);    \
        dy1[1] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2,x);              /* [1,0] */                     \
        r = GET_R(sf,x);                                            \
        b = GET_B(sf,x);                                            \
        y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b];            \
        dy2[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2+BPP(sf),x);      /* [1,1] */                     \
        r = GET_R(sf,x);                                            \
        b = GET_B(sf,x);                                            \
        y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b];            \
        dy2[1] = yytab [y];                                         \
                                                                    \
        /* average chromas: */                                      \
        dv[0] = vrytab [VMIN+(r2-y2)/2];                            \
        du[0] = ubytab [UMIN+(b2-y2)/2];                            \
    }



/* CRM_00_11 (lower line) chroma resampling: */
#define CONVERT_2x2_CHROMA_00_11(dy1,dy2,du,dv,sf,s1,s2)            \
    {                                                               \
        /* local variables: */                                      \
        PIXEL(sf,x);                                                \
        register unsigned int r, b, y;                              \
        register unsigned int r2, b2, y2;   /* 2nd line combined */ \
                                                                    \
        /* process lumas: */                                        \
        LOAD(sf,s1,x);              /* [0,0] */                     \
        r = GET_R(sf,x);                                            \
        b = GET_B(sf,x);                                            \
        y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b];            \
        dy1[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s1+BPP(sf),x);      /* [0,1] */                     \
        r = GET_R(sf,x);                                            \
        b = GET_B(sf,x);                                            \
        y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b];            \
        dy1[1] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2,x);              /* [1,0] */                     \
        r2 = (r = GET_R(sf,x));                                     \
        b2 = (b = GET_B(sf,x));                                     \
        y2 = (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);     \
        dy2[0] = yytab [y];                                         \
                                                                    \
        LOAD(sf,s2+BPP(sf),x);      /* [1,1] */                     \
        r2 += (r = GET_R(sf,x));                                    \
        b2 += (b = GET_B(sf,x));                                    \
        y2 += (y = yrtab [r] + ygtab [GET_G(sf,x)] + ybtab [b]);    \
        dy2[1] = yytab [y];                                         \
                                                                    \
        /* average chromas: */                                      \
        dv[0] = vrytab [VMIN+(r2-y2)/2];                            \
        du[0] = ubytab [UMIN+(b2-y2)/2];                            \
    }


/*
 * Generic 4x2 block converter:
 */
#define CONVERT_4x2(dy1,dy2,dy3,dy4,du,du2,dv,dv2,sf,s1,s2,s3,s4,crm)   \
    CONVERT_4x2_##crm(dy1,dy2,dy3,dy4,du,du2,dv,dv2,sf,s1,s2,s3,s4)

/*
 * Generic 2x2 block converter:
 */
#define CONVERT_2x2(dy1,dy2,du,dv,sf,s1,s2,crm)                     \
    CONVERT_2x2_##crm(dy1,dy2,du,dv,sf,s1,s2)



/*
 * Generic RGBtoYUV quad-row converter:
 *
 *  YUV-image pointers are assumed to be 4x2-block aligned, as well as
 *  n -- the number of pixels to be converter, is assumed to be multiple of 2.
 *  crm is always CRM_11_11_Interlace
 */
#define QUADROW(dy1,dy2,dy3,dy4,du,du2,dv,dv2,sf,s1,s2,s3,s4,n,crm)  \
    {                                                               \
        register int n2 = n/2;                                      \
        /* convert 2x2 blocks: */                                   \
        while (n2) {                                                \
            CONVERT_4x2(dy1,dy2,dy3,dy4,du,du2,dv,dv2,sf,s1,s2,s3,s4,crm);  \
            dy1 += 2; dy2 += 2; du ++; dv ++;                       \
            dy3 += 2; dy4 += 2; du2++; dv2++;                       \
            s1 += BPP(sf) * 2; s2 += BPP(sf) * 2;                   \
            s3 += BPP(sf) * 2; s4 += BPP(sf) * 2;                   \
            n2 --;                                                  \
        }                                                           \
    }

 /*
 * Generic RGBtoYUV double-row converter:
 *
 *  YUV-image pointers are assumed to be 2x2-block aligned, as well as
 *  n -- the number of pixels to be converter, is assumed to be multiple of 2.
 */
#define DBLROW(dy1,dy2,du,dv,sf,s1,s2,n,crm)                        \
    {                                                               \
        register int n2 = n/2;                                      \
        /* convert 2x2 blocks: */                                   \
        while (n2) {                                                \
            CONVERT_2x2(dy1,dy2,du,dv,sf,s1,s2,crm);                \
            dy1 += 2; dy2 += 2; du ++; dv ++;                       \
            s1 += BPP(sf) * 2; s2 += BPP(sf) * 2;                   \
            n2 --;                                                  \
        }                                                           \
    }


/*
 * Function names:
 */
#define FN(df,sf)               sf##to##df
#define DBLROW_FN(df,sf,crm)    sf##to##df##_DBLROW_##crm
#define QUADROW_FN(df,sf,crm)    sf##to##df##_QUADROW_##crm


/*
 * Function replication macros:
 *  (quadrow converters)
 */
#define QUADROW_FUNC(df,sf,crm)                                     \
    static void QUADROW_FN(df,sf,crm) (                             \
        unsigned char *dy1, unsigned char *dy2,                     \
        unsigned char *dy3, unsigned char *dy4,                     \
        unsigned char *du, unsigned char *dv,                       \
        unsigned char *du2, unsigned char *dv2,                     \
        unsigned char *s1, unsigned char *s2,                       \
        unsigned char *s3, unsigned char *s4, int n)                \
        QUADROW(dy1,dy2,dy3,dy4,du,du2,dv,dv2,sf,s1,s2,s3,s4,n,crm)

 /*
 * Function replication macros:
 *  (dblrow converters)
 */
#define DBLROW_FUNC(df,sf,crm)                                      \
    static void DBLROW_FN(df,sf,crm) (                              \
        unsigned char *dy1, unsigned char *dy2,                     \
        unsigned char *du, unsigned char *dv,                       \
        unsigned char *s1, unsigned char *s2, int n)                \
        DBLROW(dy1,dy2,du,dv,sf,s1,s2,n,crm)



/***********************************************************/


/*
 * Actual double-row converters:
 */
DBLROW_FUNC(I420, RGB32,  CHROMA_11_11)
DBLROW_FUNC(I420, RGB32,  CHROMA_11_00)
DBLROW_FUNC(I420, RGB32,  CHROMA_00_11)
QUADROW_FUNC(I420, RGB32,  CHROMA_11_11_Interlace)


DBLROW_FUNC(I420, BGR_32, CHROMA_11_11)
DBLROW_FUNC(I420, BGR_32, CHROMA_11_00)
DBLROW_FUNC(I420, BGR_32, CHROMA_00_11)
QUADROW_FUNC(I420, BGR_32, CHROMA_11_11_Interlace)


DBLROW_FUNC(I420, RGB24,  CHROMA_11_11)
DBLROW_FUNC(I420, RGB24,  CHROMA_11_00)
DBLROW_FUNC(I420, RGB24,  CHROMA_00_11)
QUADROW_FUNC(I420, RGB24,  CHROMA_11_11_Interlace)


DBLROW_FUNC(I420, BGR24,  CHROMA_11_11)
DBLROW_FUNC(I420, BGR24,  CHROMA_11_00)
DBLROW_FUNC(I420, BGR24,  CHROMA_00_11)
QUADROW_FUNC(I420, BGR24,  CHROMA_11_11_Interlace)


DBLROW_FUNC(I420, RGB565, CHROMA_11_11)
DBLROW_FUNC(I420, RGB565, CHROMA_11_00)
DBLROW_FUNC(I420, RGB565, CHROMA_00_11)
QUADROW_FUNC(I420, RGB565, CHROMA_11_11_Interlace)


DBLROW_FUNC(I420, RGB555, CHROMA_11_11)
DBLROW_FUNC(I420, RGB555, CHROMA_11_00)
DBLROW_FUNC(I420, RGB555, CHROMA_00_11)
QUADROW_FUNC(I420, RGB555, CHROMA_11_11_Interlace)


DBLROW_FUNC(I420, RGB8,   CHROMA_11_11)
DBLROW_FUNC(I420, RGB8,   CHROMA_11_00)
DBLROW_FUNC(I420, RGB8,   CHROMA_00_11)
QUADROW_FUNC(I420, RGB8,   CHROMA_11_11_Interlace)



/*
 * Double-row scale function selection tables:
 *  [source format][chroma resampling type]
 */
static void (* DblRowFuncs [RGB_FORMATS+2][CHROMA_RESAMPLING_MODES]) (
    unsigned char *, unsigned char *, unsigned char *, unsigned char *,
    unsigned char *, unsigned char *, int) =
{
    {
        DBLROW_FN(I420, RGB32,  CHROMA_11_11),
        DBLROW_FN(I420, RGB32,  CHROMA_11_00),
        DBLROW_FN(I420, RGB32,  CHROMA_00_11)
    },
    {   /* BGR32: */
        0, 0, 0
    },
    {
        DBLROW_FN(I420, RGB24,  CHROMA_11_11),
        DBLROW_FN(I420, RGB24,  CHROMA_11_00),
        DBLROW_FN(I420, RGB24,  CHROMA_00_11)
    },
    {
        DBLROW_FN(I420, RGB565, CHROMA_11_11),
        DBLROW_FN(I420, RGB565, CHROMA_11_00),
        DBLROW_FN(I420, RGB565, CHROMA_00_11)
    },
    {
        DBLROW_FN(I420, RGB555, CHROMA_11_11),
        DBLROW_FN(I420, RGB555, CHROMA_11_00),
        DBLROW_FN(I420, RGB555, CHROMA_00_11)
    },
    {   /* RGB444: */
        0, 0, 0
    },
    {
        DBLROW_FN(I420, RGB8,   CHROMA_11_11),
        DBLROW_FN(I420, RGB8,   CHROMA_11_00),
        DBLROW_FN(I420, RGB8,   CHROMA_00_11)
    },
    {
        DBLROW_FN(I420, BGR_32,  CHROMA_11_11),
        DBLROW_FN(I420, BGR_32,  CHROMA_11_00),
        DBLROW_FN(I420, BGR_32,  CHROMA_00_11)
    },
    {
        DBLROW_FN(I420, BGR24,  CHROMA_11_11),
        DBLROW_FN(I420, BGR24,  CHROMA_11_00),
        DBLROW_FN(I420, BGR24,  CHROMA_00_11)
    }
};

/*
 * Quad-row scale function selection tables:
 *  [source format][CRM_11_11_Interlace]
 */
static void (* QuadRowFuncs [RGB_FORMATS+2]) (
    unsigned char *, unsigned char *, unsigned char *, unsigned char *,
    unsigned char *, unsigned char *, unsigned char *, unsigned char *,
    unsigned char *, unsigned char *, unsigned char *, unsigned char *, int) =
{
    QUADROW_FN(I420, RGB32,  CHROMA_11_11_Interlace),
    /* BGR32: */
    0,
    QUADROW_FN(I420, RGB24,  CHROMA_11_11_Interlace),
    QUADROW_FN(I420, RGB565, CHROMA_11_11_Interlace),
    QUADROW_FN(I420, RGB555, CHROMA_11_11_Interlace),
    /* RGB444: */
    0,
    QUADROW_FN(I420, RGB8,   CHROMA_11_11_Interlace),
    QUADROW_FN(I420, BGR_32, CHROMA_11_11_Interlace),
    QUADROW_FN(I420, BGR24,  CHROMA_11_11_Interlace)
};


/*
 * Bytes per pixel (bpp) table:
 */
static int bpp [RGB_FORMATS+2] =
{
    BPP(RGB32),
    BPP(BGR32),
    BPP(RGB24),
    BPP(RGB565),
    BPP(RGB555),
    BPP(RGB444),
    BPP(RGB8),
    BPP(BGR_32),
    BPP(BGR24)
};


/*
 * The main RGBtoYUV converter:
 */
static int RGBtoYUV (
    /* destination image parameters: */
    unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    /* source format: */
    int src_format,
    /* source image parametersBto: */
    unsigned char *src_ptr, int src_width, int src_height,
    int src_pitch, int src_x, int src_y, int src_dx, int src_dy)
{
    /* Drive the conversion */

    /* pointer to a double-row converter to use: */
    void (*dblrow_proc) (unsigned char *, unsigned char *,
        unsigned char *, unsigned char *, unsigned char *, unsigned char *, int);

    /* pointer to a quad-row converter to use: */
    void (*quadrow_proc) (unsigned char *, unsigned char *, unsigned char *, unsigned char *, 
            unsigned char *, unsigned char *, unsigned char *, unsigned char *, 
            unsigned char *, unsigned char *,
            unsigned char *, unsigned char *, int);    

    /* pointers and source pixel depth: */
    register unsigned char *dy1, *dy2, *du, *dv, *s1, *s2;
    register unsigned char *dy3, *dy4, *du2, *dv2, *s3, *s4;
    register int src_bpp, i;


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
fail:   /* fail: */
        return -1;
    }


    /* check scale factors: */
    if (dest_dx != src_dx || dest_dy != src_dy)
        /* no in-place resizing !!! */
        goto fail;


    /* skip odd start column: */
    if (dest_x & 1) { dest_x ++; dest_dx --; src_x ++; src_dx --; }
    /* clip horisontal range: */
    if (dest_dx & 1) { dest_dx --; src_dx --; if (dest_dx <= 0) goto fail;}


    /* skip odd start row: */
    if (dest_y & 1) { dest_y ++; dest_dy --; src_y ++; src_dy --; }
    /* clip vertical range: */
    if (dest_dy & 1) { dest_dy --; src_dy --; if (dest_dy <= 0) goto fail;}


    /* get source pixel depth: */
    src_bpp = bpp [src_format];

    /* check if bottop-up images: */
    if (src_pitch < 0) src_ptr -= (src_height-1) * src_pitch;
    if (dest_pitch <= 0) return -1;                     /* not supported */

    if(chroma_resampling_mode != CRM_11_11_Interlace)
    {
        /* select row and image converters: */
        dblrow_proc = DblRowFuncs [src_format] [chroma_resampling_mode];

        /* get pointers: */
        s1  = src_ptr + src_x * src_bpp + src_y * src_pitch;
        s2  = s1 + src_pitch;
        dy1 = dest_ptr + dest_x + dest_y * dest_pitch;      /* luma offsets  */
        dy2 = dy1 + dest_pitch;
        du  = dest_ptr + dest_height * dest_pitch
              + (dest_x/2 + dest_y/2 * dest_pitch / 2);     /* chroma offset */
        dv  = du + dest_height * dest_pitch / 4;

        /* the main loop (processes 2 lines a time): */
        for (i = 0; i < dest_dy/2; i ++) {
            (*dblrow_proc) (dy1, dy2, du, dv, s1, s2, dest_dx);

            /* switch to the next two lines: */
            s1 += src_pitch * 2;    s2 += src_pitch * 2;
            dy1 += dest_pitch * 2;  dy2 += dest_pitch * 2;
            du += dest_pitch / 2;   dv += dest_pitch / 2;
        }
    } 
    else
    {
        /* CRM_11_11_Interlace */
        /* select row and image converters: */
        quadrow_proc = QuadRowFuncs [src_format];

        /* get pointers: */
        s1  = src_ptr + src_x * src_bpp + src_y * src_pitch;
        s2  = s1 + src_pitch;
        s3  = s2 + src_pitch;
        s4  = s3 + src_pitch;
        dy1 = dest_ptr + dest_x + dest_y * dest_pitch;      /* luma offsets  */
        dy2 = dy1 + dest_pitch;
        dy3 = dy2 + dest_pitch;
        dy4 = dy3 + dest_pitch;
        du  = dest_ptr + dest_height * dest_pitch
              + (dest_x/2 + dest_y/2 * dest_pitch / 2);     /* chroma offset */
        du2 = du + dest_pitch / 2;   
        dv  = du + dest_height * dest_pitch / 4;
        dv2 = dv + dest_pitch / 2;

        /* the main loop (processes 4 lines a time): */
        for (i = 0; i < dest_dy/4; i ++) {
            (*quadrow_proc) (dy1,dy2,dy3,dy4,du,du2,dv,dv2,s1,s2,s3,s4,dest_dx);

            /* switch to the next four lines: */
            s1 += src_pitch * 4;    s2 += src_pitch * 4;
            s3 += src_pitch * 4;    s4 += src_pitch * 4;
            dy1 += dest_pitch * 4;  dy2 += dest_pitch * 4;
            dy3 += dest_pitch * 4;  dy4 += dest_pitch * 4;
            du += dest_pitch;       dv += dest_pitch;
            du2 += dest_pitch;      dv2 += dest_pitch;
        }    
    }
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


#define RGBTOYUV_FUNC(sf)                                   \
    int FN(I420,sf) (unsigned char *dest_ptr,               \
        int dest_width, int dest_height, int dest_pitch,    \
        int dest_x, int dest_y, int dest_dx, int dest_dy,   \
        unsigned char *src_ptr,                             \
        int src_width, int src_height, int src_pitch,       \
        int src_x, int src_y, int src_dx, int src_dy)       \
    {                                                       \
        return RGBtoYUV(                                  \
            dest_ptr, dest_width, dest_height,              \
            dest_pitch, dest_x, dest_y, dest_dx, dest_dy,   \
            ID(sf), src_ptr, src_width, src_height,         \
            src_pitch, src_x, src_y, src_dx, src_dy);       \
    }


RGBTOYUV_FUNC(RGB32 )
RGBTOYUV_FUNC(RGB24 )
RGBTOYUV_FUNC(RGB565)
RGBTOYUV_FUNC(RGB555)
RGBTOYUV_FUNC(RGB8)


RGBTOYUV_FUNC(BGR_32 )
RGBTOYUV_FUNC(BGR24 )

/*********************************
 * Back to the old stuff: */

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

//////////////////////////////////////////////////////
//
//	ARGBtoYUVA
//
//	Note: YUVA is identical to I420 but has an additional
//	  planar field at the end the contains "alpha" info.
//	  This "alpha" field is the size of the Y component
//
//////////////////////////////////////////////////////

int ARGBtoYUVA (unsigned char *dest_ptr, int dest_width, int dest_height,
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
        unsigned char *s1, *s2, *d1, *d2, *dv, *du, *d3, *d4;
        register int i, j;

        /* get pointers: */
        s1 = src_ptr + src_x * BPP(RGB32) + src_y * src_pitch; /* 3 bytes/pixel */
        s2 = s1 + src_pitch;
        d1 = dest_ptr + dest_x + dest_y * dest_pitch;    /* luma offsets  */
        d2 = d1 + dest_pitch;
        du = dest_ptr + dest_height * dest_pitch + (dest_x/2 + dest_y/2 * dest_pitch/2); /* chroma offset */
        dv = du + dest_height * dest_pitch/4;
        d3 = dv + dest_height * dest_pitch/4;
        d4 = d3 + dest_pitch;

        switch (chroma_resampling_mode)
        {
            case CRM_00_11:

                /* the main loop (processes 2 lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert 2x2 block: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        int r2, b2, y2;

                        /* process lumas: */
                        {
                            PIXEL(RGB32,x);
                            register unsigned int r, b, y;

                            LOAD(RGB32,s1,x);               /* [0,0] */
                            r = GET_R(RGB32,x);
                            b = GET_B(RGB32,x);
                            y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b];
                            d1[0] = yytab [y];
							d3[0] = GET_A(RGB32,x);

                            LOAD(RGB32,s1+BPP(RGB32),x);    /* [0,1] */
                            r = GET_R(RGB32,x);
                            b = GET_B(RGB32,x);
                            y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b];
                            d1[1] = yytab [y];
							d3[1] = GET_A(RGB32,x);

                            LOAD(RGB32,s2,x);               /* [1,0] */
                            r2 = (r = GET_R(RGB32,x));
                            b2 = (b = GET_B(RGB32,x));
                            y2 = (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d2[0] = yytab [y];
							d4[0] = GET_A(RGB32,x);

                            LOAD(RGB32,s2+BPP(RGB32),x);    /* [1,1] */
                            r2 += (r = GET_R(RGB32,x));
                            b2 += (b = GET_B(RGB32,x));
                            y2 += (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d2[1] = yytab [y];
							d4[1] = GET_A(RGB32,x);
                        }

                        /* average chromas: */
                        dv[0] = vrytab [VMIN+(r2-y2)/2];
                        du[0] = ubytab [UMIN+(b2-y2)/2];

                        /* go to the next block: */
                        s1 += 2 * BPP(RGB32); s2 += 2 * BPP(RGB32);
                        d1 += 2; d2 += 2;
                        d3 += 2; d4 += 2;
                        du += 1; dv += 1;
                    }

                    /* switch to the next two lines: */
                    s1 += src_pitch * 2 - dest_dx * BPP(RGB32);
                    s2 += src_pitch * 2 - dest_dx * BPP(RGB32);
                    d1 += dest_pitch * 2 - dest_dx;
                    d2 += dest_pitch * 2 - dest_dx;
                    d3 += dest_pitch * 2 - dest_dx;
                    d4 += dest_pitch * 2 - dest_dx;
                    du += (dest_pitch - dest_dx)/2;
                    dv += (dest_pitch - dest_dx)/2;
                }
                break;

            case CRM_11_00:

                /* the main loop (processes 2 lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert 2x2 block: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        int r2, b2, y2;

                        /* process lumas: */
                        {
                            PIXEL(RGB32,x);
                            register unsigned int r, b, y;

                            LOAD(RGB32,s1,x);               /* [0,0] */
                            r2 = (r = GET_R(RGB32,x));
                            b2 = (b = GET_B(RGB32,x));
                            y2 = (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d1[0] = yytab [y];
							d3[0] = GET_A(RGB32,x);

                            LOAD(RGB32,s1+BPP(RGB32),x);    /* [0,1] */
                            r2 += (r = GET_R(RGB32,x));
                            b2 += (b = GET_B(RGB32,x));
                            y2 += (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d1[1] = yytab [y];
							d3[1] = GET_A(RGB32,x);

                            LOAD(RGB32,s2,x);               /* [1,0] */
                            r = GET_R(RGB32,x);
                            b = GET_B(RGB32,x);
                            y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b];
                            d2[0] = yytab [y];
							d4[0] = GET_A(RGB32,x);

                            LOAD(RGB32,s2+BPP(RGB32),x);    /* [1,1] */
                            r = GET_R(RGB32,x);
                            b = GET_B(RGB32,x);
                            y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b];
                            d2[1] = yytab [y];
 							d4[1] = GET_A(RGB32,x);
                       }

                        /* average chromas: */
                        dv[0] = vrytab [VMIN+(r2-y2)/2];
                        du[0] = ubytab [UMIN+(b2-y2)/2];

                        /* go to the next block: */
                        s1 += 2 * BPP(RGB32); s2 += 2 * BPP(RGB32);
                        d1 += 2; d2 += 2;
                        d3 += 2; d4 += 2;
                        du += 1; dv += 1;
                    }

                    /* switch to the next two lines: */
                    s1 += src_pitch * 2 - dest_dx * BPP(RGB32);
                    s2 += src_pitch * 2 - dest_dx * BPP(RGB32);
                    d1 += dest_pitch * 2 - dest_dx;
                    d2 += dest_pitch * 2 - dest_dx;
                    d3 += dest_pitch * 2 - dest_dx;
                    d4 += dest_pitch * 2 - dest_dx;
                    du += (dest_pitch - dest_dx)/2;
                    dv += (dest_pitch - dest_dx)/2;
                }
                break;

            case CRM_11_11:
            default:

                /* the main loop (processes 2 lines a time): */
                for (i = 0; i < dest_dy/2; i ++) {

                    /* convert 2x2 block: */
                    for (j = 0; j < dest_dx/2; j ++) {

                        int r4, b4, y4;

                        /* process lumas: */
                        {
                            PIXEL(RGB32,x);
                            register unsigned int r, b, y;

                            LOAD(RGB32,s1,x);               /* [0,0] */
                            r4 = (r = GET_R(RGB32,x));
                            b4 = (b = GET_B(RGB32,x));
                            y4 = (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d1[0] = yytab [y];
							d3[0] = GET_A(RGB32,x);

                            LOAD(RGB32,s1+BPP(RGB32),x);    /* [0,1] */
                            r4 += (r = GET_R(RGB32,x));
                            b4 += (b = GET_B(RGB32,x));
                            y4 += (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d1[1] = yytab [y];
							d3[1] = GET_A(RGB32,x);

                            LOAD(RGB32,s2,x);               /* [1,0] */
                            r4 += (r = GET_R(RGB32,x));
                            b4 += (b = GET_B(RGB32,x));
                            y4 += (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d2[0] = yytab [y];
							d4[0] = GET_A(RGB32,x);

                            LOAD(RGB32,s2+BPP(RGB32),x);    /* [1,1] */
                            r4 += (r = GET_R(RGB32,x));
                            b4 += (b = GET_B(RGB32,x));
                            y4 += (y = yrtab [r] + ygtab [GET_G(RGB32,x)] + ybtab [b]);
                            d2[1] = yytab [y];
							d4[1] = GET_A(RGB32,x);
                        }

                        /* average chromas: */
                        dv[0] = vrytab [VMIN+(r4-y4)/4];
                        du[0] = ubytab [UMIN+(b4-y4)/4];

                        /* go to the next block: */
                        s1 += 2 * BPP(RGB32); s2 += 2 * BPP(RGB32);
                        d1 += 2; d2 += 2;
                        d3 += 2; d4 += 2;
                        du += 1; dv += 1;
                    }

                    /* switch to the next two lines: */
                    s1 += src_pitch * 2 - dest_dx * BPP(RGB32);
                    s2 += src_pitch * 2 - dest_dx * BPP(RGB32);
                    d1 += dest_pitch * 2 - dest_dx;
                    d2 += dest_pitch * 2 - dest_dx;
                    d3 += dest_pitch * 2 - dest_dx;
                    d4 += dest_pitch * 2 - dest_dx;
                    du += (dest_pitch - dest_dx)/2;
                    dv += (dest_pitch - dest_dx)/2;
                }
        }

    }
/*
 *  else {
 *      put all the color-dependent stuff here ...
 *  }
 */
    return 0;
}

/* rgb2yuv.c -- end of file */
