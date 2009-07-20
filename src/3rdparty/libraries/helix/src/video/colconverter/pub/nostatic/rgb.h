/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rgb.h,v 1.4 2004/07/09 18:36:05 hubbe Exp $
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

#ifndef __RGB_H__
#define __RGB_H__   1

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RGB formats supported:
 *
 *  RGB32   - 32-bit 8:8:8 RGB
 *  BGR32   - 32-bit 8:8:8 RGB (R&B swapped)
 *  RGB24   - 24-bit 8:8:8 RGB
 *  RGB565  - 16-bit 5:6:5 RGB
 *  RGB555  - 16-bit 5:5:5 RGB
 *  RGB8    -  8-bit palettized RGB
 */
#define RGB32_ID            0
#define BGR32_ID            1
#define RGB24_ID            2
#define RGB565_ID           3
#define RGB555_ID           4
#define RGB8_ID             5

#define RGB_FORMATS         6

/*
 * Generic format ID:
 */
#define ID(f)               f##_ID

/*
 * Bytes per pixel (bpp) constants:
 */
#define RGB32_BPP           4
#define BGR32_BPP           4
#define RGB24_BPP           3
#define RGB565_BPP          2
#define RGB555_BPP          2
#define RGB8_BPP            1

/*
 * Generic pixel depth:
 */
#define BPP(f)              f##_BPP



/*
 * Internal pixel representations.
 * We will use 3 different types of temporary pixel variables
 * based on the following classification:
 *  1. Pixels with power of 2-aligned depth and bit-packed RGB fields:
 *      RGB32,BGR32,RGB565,RGB555 - use 1 full register to store RGB fields
 *      (we will use name RGBX for generalized macros dealing
 *       with these formats)
 *  2. Pixels with misaligned depth:
 *      RGB24 - use 3 8-bit registers to store R,G,and B components
 *  3. Paletized pixels:
 *      RGB8 - use 1 full register to store palette index
 */
#define RGBX_PIXEL(a)       register unsigned int a##_rgb
#define RGB32_PIXEL(a)      RGBX_PIXEL(a)
#define BGR32_PIXEL(a)      RGBX_PIXEL(a)
#define RGB24_PIXEL(a)      register unsigned char a##_b, a##_g, a##_r
#define RGB565_PIXEL(a)     RGBX_PIXEL(a)
#define RGB555_PIXEL(a)     RGBX_PIXEL(a)
#define RGB8_PIXEL(a)       register unsigned int a##_idx

/*
 * Generic pixel declaration:
 */
#define PIXEL(f,a)          f##_PIXEL(a)

/*
 * Load/store/copy pixel macros:
 * (s/d - source/destination pointers; sa/da - source/dest. pixel vars)
 */
#define RGBX_LOAD(s,a,t)    a##_rgb=*(t*)(s)
#define RGBX_STORE(d,a,t)   *(t*)(d)=a##_rgb
#define RGBX_COPY(da,sa)    da##_rgb=sa##_rgb

#define RGB32_LOAD(s,a)     RGBX_LOAD(s,a,unsigned int)
#define RGB32_STORE(d,a)    RGBX_STORE(d,a,unsigned int)
#define RGB32_COPY(da,sa)   RGBX_COPY(da,sa)

#define BGR32_LOAD(s,a)     RGBX_LOAD(s,a,unsigned int)
#define BGR32_STORE(d,a)    RGBX_STORE(d,a,unsigned int)
#define BGR32_COPY(da,sa)   RGBX_COPY(da,sa)

#define RGB24_LOAD(s,a)     a##_b=(s)[0], a##_g=(s)[1], a##_r=(s)[2]
#define RGB24_STORE(d,a)    (d)[0]=a##_b, (d)[1]=a##_g, (d)[2]=a##_r
#define RGB24_COPY(da,sa)   da##_b=sa##_b, da##_g=sa##_g, da##_r=sa##_r

#define RGB565_LOAD(s,a)    RGBX_LOAD(s,a,unsigned short)
#define RGB565_STORE(d,a)   RGBX_STORE(d,a,unsigned short)
#define RGB565_COPY(da,sa)  RGBX_COPY(da,sa)

#define RGB555_LOAD(s,a)    RGBX_LOAD(s,a,unsigned short)
#define RGB555_STORE(d,a)   RGBX_STORE(d,a,unsigned short)
#define RGB555_COPY(da,sa)  RGBX_COPY(da,sa)

#define RGB8_LOAD(s,a)      a##_idx=(s)[0]
#define RGB8_STORE(d,a)     (d)[0]=a##_idx
#define RGB8_COPY(da,sa)    da##_idx=sa##_idx

/*
 * Generic pixel load/store/copy macros:
 */
#define LOAD(f,s,a)         f##_LOAD(s,a)
#define STORE(f,d,a)        f##_STORE(d,a)
#define COPY(f,da,sa)       f##_COPY(da,sa)

/*
 * R,G,B fields bit-packing:
 * (RGB32,BGR32,RGB565,and RGB555 formats only)
 */
#define RGB32_R_START       16
#define RGB32_R_BITS        8
#define RGB32_G_START       8
#define RGB32_G_BITS        8
#define RGB32_B_START       0
#define RGB32_B_BITS        8

#define BGR32_R_START       0
#define BGR32_R_BITS        8
#define BGR32_G_START       8
#define BGR32_G_BITS        8
#define BGR32_B_START       16
#define BGR32_B_BITS        8

#define RGB565_R_START      11
#define RGB565_R_BITS       5
#define RGB565_G_START      5
#define RGB565_G_BITS       6
#define RGB565_B_START      0
#define RGB565_B_BITS       5

#define RGB555_R_START      10
#define RGB555_R_BITS       5
#define RGB555_G_START      5
#define RGB555_G_BITS       5
#define RGB555_B_START      0
#define RGB555_B_BITS       5

/*
 * Generic bit-packing macros:
 */
#define START(f,x)          f##_##x##_START
#define BITS(f,x)           f##_##x##_BITS

/*
 * Fields extraction/assignment macros.
 *
 * Here we will largely rely on compiler's ability to get rid of
 * unexecuted branches and glue subsequent constant shifts and
 * masks together. This assumption works well for MSVC, Watcom,
 * and Zortech (Symantec) compilers, but some others may fail.
 *
 * If you are using compiler not listed above, please check the
 * assembly output to make sure that the code is well optimized.
 * If the compiler leaves comparisons and multiple shifts/masks
 * in places of RGBX_GET_X() and RGBX_SET(), you would have to
 * reimplement and optimize all these instances manually, using
 * the appropriate pixel/field-type related constants.
 */
#define NORM(s) ((s) & 0x1F) /* shuts compiler up */
#define RGBX_GET_X(f,x,a)   \
    ((START(f,x)+BITS(f,x)<8)   \
    ? ((a##_rgb << NORM(8-(START(f,x)+BITS(f,x)))) & (0x100-(1U<<(8-BITS(f,x))))) \
    : ((a##_rgb >> NORM((START(f,x)+BITS(f,x))-8)) & (0x100-(1U<<(8-BITS(f,x))))))
#define RGBX_X(f,x,v)       \
    ((START(f,x)+BITS(f,x)<8)   \
    ? (((v) & (0x100-(1U<<(8-BITS(f,x))))) >> NORM(8-(START(f,x)+BITS(f,x))))   \
    : (((v) & (0x100-(1U<<(8-BITS(f,x))))) << NORM((START(f,x)+BITS(f,x))-8)))
#define RGBX_SET(f,a,r,g,b) \
    a##_rgb = RGBX_X(f,R,r) | RGBX_X(f,G,g) | RGBX_X(f,B,b)

#define RGB32_GET_R(a)      RGBX_GET_X(RGB32,R,a)
#define RGB32_GET_G(a)      RGBX_GET_X(RGB32,G,a)
#define RGB32_GET_B(a)      RGBX_GET_X(RGB32,B,a)
#define RGB32_SET(a,r,g,b)  RGBX_SET(  RGB32,a,r,g,b)

#define BGR32_GET_R(a)      RGBX_GET_X(BGR32,R,a)
#define BGR32_GET_G(a)      RGBX_GET_X(BGR32,G,a)
#define BGR32_GET_B(a)      RGBX_GET_X(BGR32,B,a)
#define BGR32_SET(a,r,g,b)  RGBX_SET(  BGR32,a,r,g,b)

#define RGB24_GET_R(a)      (a##_r)
#define RGB24_GET_G(a)      (a##_g)
#define RGB24_GET_B(a)      (a##_b)
#define RGB24_SET(a,r,g,b)  a##_b=(b), a##_g=(g), a##_r=(r)

#define RGB565_GET_R(a)     RGBX_GET_X(RGB565,R,a)
#define RGB565_GET_G(a)     RGBX_GET_X(RGB565,G,a)
#define RGB565_GET_B(a)     RGBX_GET_X(RGB565,B,a)
#define RGB565_SET(a,r,g,b) RGBX_SET(  RGB565,a,r,g,b)

#define RGB555_GET_R(a)     RGBX_GET_X(RGB555,R,a)
#define RGB555_GET_G(a)     RGBX_GET_X(RGB555,G,a)
#define RGB555_GET_B(a)     RGBX_GET_X(RGB555,B,a)
#define RGB555_SET(a,r,g,b) RGBX_SET(  RGB555,a,r,g,b)

#define RGB8_GET_R(a)       PAL_GET_R(a##_idx)
#define RGB8_GET_G(a)       PAL_GET_G(a##_idx)
#define RGB8_GET_B(a)       PAL_GET_B(a##_idx)
#define RGB8_SET(a,r,g,b)   a##_idx=PMAP_GET(r,g,b)

/*
 * Generic field extraction/assignment macros:
 */
#define GET_R(f,a)          f##_GET_R(a)
#define GET_G(f,a)          f##_GET_G(a)
#define GET_B(f,a)          f##_GET_B(a)
#define SET(f,a,r,g,b)      f##_SET(a,r,g,b)

/*
 * Generic pixel-format converter:
 * (sf/sa - source pixel format/name; df/da - dest. pixel format/name)
 */
#define CONVERT(df,da,sf,sa) SET(df,da,GET_R(sf,sa),GET_G(sf,sa),GET_B(sf,sa))

/*
 * Generic pixel load and convert macro:
 */
#define LOAD_CONVERT(df,da,sf,s)            \
    /* do we have the same format? */       \
    if (ID(df) == ID(sf)) {                 \
        /* just load pixel */               \
        LOAD(df,s,da);                      \
    } else {                                \
        /* load & convert pixel: */         \
        PIXEL(sf,sa);                       \
        LOAD(sf,s,sa);                      \
        CONVERT(df,da,sf,sa);               \
    }

/*
 * Get the average value of two pixels:
 * (da=(sa+sb)/2)
 */
#define RGBX_AVERAGE(f,da,sa,sb)    \
    da##_rgb = (((sa##_rgb^sb##_rgb)>>1)    \
        & (((1U<<(START(f,R)+BITS(f,R)-1))-(1U<<START(f,R)))  \
          |((1U<<(START(f,G)+BITS(f,G)-1))-(1U<<START(f,G)))  \
          |((1U<<(START(f,B)+BITS(f,B)-1))-(1U<<START(f,B)))))\
        + (sa##_rgb&sb##_rgb)
#define RGB32_AVERAGE(da,sa,sb) RGBX_AVERAGE(RGB32,da,sa,sb)
#define BGR32_AVERAGE(da,sa,sb) RGBX_AVERAGE(BGR32,da,sa,sb)
#define RGB24_AVERAGE(da,sa,sb) \
    SET(RGB24,da,((GET_R(RGB24,sa)+GET_R(RGB24,sb))>>1),    \
                 ((GET_G(RGB24,sa)+GET_G(RGB24,sb))>>1),    \
                 ((GET_B(RGB24,sa)+GET_B(RGB24,sb))>>1))
#define RGB565_AVERAGE(da,sa,sb) RGBX_AVERAGE(RGB565,da,sa,sb)
#define RGB555_AVERAGE(da,sa,sb) RGBX_AVERAGE(RGB555,da,sa,sb)
#define RGB8_AVERAGE(da,sa,sb)  \
    SET(RGB8,da,((GET_R(RGB8,sa)+GET_R(RGB8,sb))>>1),       \
                ((GET_G(RGB8,sa)+GET_G(RGB8,sb))>>1),       \
                ((GET_B(RGB8,sa)+GET_B(RGB8,sb))>>1))

/*
 * Generic two pixels' average:
 */
#define AVERAGE(f,da,sa,sb) f##_AVERAGE(da,sa,sb)

/*
 * Loads a pixel and averages it with another one:
 * (da = (sa+[s])/2)
 */
#define LOAD_AVERAGE(f,da,sa,s)             \
    {                                       \
        /* load & convert pixel: */         \
        PIXEL(f,sb);                        \
        LOAD(f,s,sb);                       \
        AVERAGE(f,da,sa,sb);                \
    }

#ifdef __cplusplus
}
#endif

#endif /* __RGB_H__ */

