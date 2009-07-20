/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rgb2rgb.c,v 1.8 2007/07/06 20:53:51 jfinnecy Exp $
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

#ifdef _MACINTOSH
#pragma require_prototypes off
#endif

/*** #includes: ********************************************/

#include "env.h"
#include "rgb.h"    /* basic RGB-data definitions & macros */
#include "scale.h"  /* scale algorithms */
#include "colorlib.h" /* ensure that prototypes get extern C'ed */
/*** Additional pixel-level macros: ************************/

/*
 * Generic pixel load-convert-store macro:
 */
#define LOAD_CONVERT_STORE(df,d,sf,s)       \
    {                                       \
        PIXEL(df,da);                       \
        LOAD_CONVERT(df,da,sf,s);           \
        s+=BPP(sf);                         \
        STORE(df,d,da);                     \
        d+=BPP(df);                         \
    }

/*
 * Generic 4-pixels load-convert-store macro
 * (XXX: this is just a lazy implementation of
 *  4-times unrolled load-convert-store;
 *  later, we should rewrite it for better
 *  efficiency !!!)
 */
#define LOAD_CONVERT_STORE_4(df,d,sf,s)     \
    {                                       \
        PIXEL(df,da);                       \
        LOAD_CONVERT(df,da,sf,s);           \
        STORE(df,d,da);                     \
        LOAD_CONVERT(df,da,sf,s+BPP(sf));   \
        STORE(df,d+BPP(df),da);             \
        LOAD_CONVERT(df,da,sf,s+2*BPP(sf)); \
        STORE(df,d+2*BPP(df),da);           \
        LOAD_CONVERT(df,da,sf,s+3*BPP(sf)); \
        STORE(df,d+3*BPP(df),da);           \
        s+=4*BPP(sf);                       \
        d+=4*BPP(df);                       \
    }

/*
 * Generic pixel load-convert-average-store macro:
 * ([d2] = convert([s]); [d12] = ([d1]+[d2])/2)
 */
#define LOAD_CONVERT_AVERAGE_STORE(df,d1,d12,d2,sf,s) \
    {                                       \
        PIXEL(df,da);                       \
        LOAD_CONVERT(df,da,sf,s);           \
        s+=BPP(sf);                         \
        STORE(df,d2,da);                    \
        d2+=BPP(df);                        \
        LOAD_AVERAGE(df,da,da,d1);          \
        d1+=BPP(df);                        \
        STORE(df,d12,da);                   \
        d12+=BPP(df);                       \
    }

/*
 * Generic 4-pixels load-convert-average-store macro:
 * (again, very lazy implementation; ultimately it should
 *  be rewritten for all possible combinations of pixel formats !!)
 */
#define LOAD_CONVERT_AVERAGE_STORE_4(df,d1,d12,d2,sf,s) \
    {                                       \
        PIXEL(df,da);                       \
        /* first column: */                 \
        LOAD_CONVERT(df,da,sf,s);           \
        STORE(df,d2,da);                    \
        LOAD_AVERAGE(df,da,da,d1);          \
        STORE(df,d12,da);                   \
        /* second column: */                \
        LOAD_CONVERT(df,da,sf,s+BPP(sf));   \
        STORE(df,d2+BPP(df),da);            \
        LOAD_AVERAGE(df,da,da,d1+BPP(df));  \
        STORE(df,d12+BPP(df),da);           \
        /* third column: */                 \
        LOAD_CONVERT(df,da,sf,s+2*BPP(sf)); \
        STORE(df,d2+2*BPP(df),da);          \
        LOAD_AVERAGE(df,da,da,d1+2*BPP(df));\
        STORE(df,d12+2*BPP(df),da);         \
        /* fourth column: */                \
        LOAD_CONVERT(df,da,sf,s+3*BPP(sf)); \
        STORE(df,d2+3*BPP(df),da);          \
        LOAD_AVERAGE(df,da,da,d1+3*BPP(df));\
        STORE(df,d12+3*BPP(df),da);         \
        /* update pointers: */              \
        s +=4*BPP(sf);                      \
        d1+=4*BPP(df);                      \
        d2+=4*BPP(df);                      \
        d12+=4*BPP(df);                     \
    }


/*** Generic RGB single-row converters: ********************/

/*
 * Generic row shrinking converter:
 */
#define ROW_SHRINK(df,dest_ptr,dest_dx,sf,src_ptr,src_dx)   \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d = dest_ptr;               \
        register unsigned char *s = src_ptr;                \
        register int count = dest_dx;                       \
        register int limit = src_dx >> 1; /* -1 */          \
        register int step = dest_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            do {                                            \
                /* convert and copy a nearest pixel: */     \
                PIXEL(df,da);                               \
                LOAD_CONVERT(df,da,sf,s);                   \
                STORE(df,d,da);                             \
                d+=BPP(df);                                 \
                /* inverted Bresenham stepping: */          \
                do {                                        \
                    /* skip source pixel: */                \
                    s+=BPP(sf);                             \
                } while ((limit -= step) >= 0);             \
                limit += src_dx;                            \
            } while (--count);                              \
        }                                                   \
    }

/*
 * Generic row copy converter:
 */
#define ROW_COPY(df,dest_ptr,dest_dx,sf,src_ptr,src_dx)     \
    {                                                       \
        /* do we have the same format? */                   \
        if (ID(df) == ID(sf)) {                             \
            /* just use memcopy: */                         \
            memcpy(dest_ptr, src_ptr, dest_dx*BPP(df)); /* Flawfinder: ignore */    \
        } else {                                            \
            /* initialize local variables: */               \
            register unsigned char *d = dest_ptr;           \
            register unsigned char *s = src_ptr;            \
            register int count = dest_dx;                   \
            /* convert misaligned pixels first: */          \
            while (((unsigned int)d & 3)                    \
                && ((unsigned int)s & 3) && count) {        \
                LOAD_CONVERT_STORE(df,d,sf,s);              \
                count --;                                   \
            }                                               \
            /* the main loop (convert 4 pixels a time): */  \
            while (count >= 4) {                            \
                LOAD_CONVERT_STORE_4(df,d,sf,s);            \
                count -= 4;                                 \
            }                                               \
            /* convert the remaining pixels: */             \
            while (count) {                                 \
                LOAD_CONVERT_STORE(df,d,sf,s);              \
                count --;                                   \
            }                                               \
        }                                                   \
    }

/*
 * Generic row stretching converter:
 *  (shall not be used when dest_dx/2 >= src_dx!!!)
 */
#define ROW_STRETCH(df,dest_ptr,dest_dx,sf,src_ptr,src_dx)  \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d = dest_ptr;               \
        register unsigned char *s = src_ptr;                \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx;                         \
        /* check row length: */                             \
        if (count) {                                        \
            goto start;                                     \
            /* the main loop: */                            \
            do {                                            \
                PIXEL(df,da);                               \
                /* Bresenham stepping: */                   \
                if ((limit -= step) < 0) {                  \
                    limit += dest_dx;                       \
                    /* load & convert pixel: */             \
    start:          LOAD_CONVERT(df,da,sf,s);               \
                    s+=BPP(sf);                             \
                }                                           \
                /* replicate pixel: */                      \
                STORE(df,d,da);                             \
                d+=BPP(df);                                 \
            } while (--count);                              \
        }                                                   \
    }

/*
 * Generic row 2x-stretching converter:
 */
#define ROW_STRETCH2X(df,dest_ptr,dest_dx,sf,src_ptr,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d = dest_ptr;               \
        register unsigned char *s = src_ptr;                \
        register int count = src_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            /* process first integral pixel: */             \
            PIXEL(df,da);                                   \
            LOAD_CONVERT(df,da,sf,s);                       \
            s+=BPP(sf);                                     \
            count --;                                       \
            STORE(df,d,da);                                 \
            d+=BPP(df);                                     \
            /* main loop (process 2 pixels a time): */      \
            while (count >= 2) {                            \
                /* load & convert second integral pixel: */ \
                PIXEL(df,db);                               \
                LOAD_CONVERT(df,db,sf,s);                   \
                /* calculate first half-pixel: */           \
                AVERAGE(df,da,da,db);                       \
                /* store both pixels: */                    \
                STORE(df,d,da);                             \
                STORE(df,d+BPP(df),db);                     \
                /* load & convert third integral pixel: */  \
                LOAD_CONVERT(df,da,sf,s+BPP(sf));           \
                /* calculate second half-pixel: */          \
                AVERAGE(df,db,db,da);                       \
                /* store both pixels: */                    \
                STORE(df,d+2*BPP(df),db);                   \
                STORE(df,d+3*BPP(df),da);                   \
                /* shift pointers: */                       \
                s+=2*BPP(sf);                               \
                d+=4*BPP(df);                               \
                count -= 2;                                 \
            }                                               \
            /* is there any more pixels to convert? */      \
            if (count) {                                    \
                /* load & convert last integral pixel: */   \
                PIXEL(df,db);                               \
                LOAD_CONVERT(df,db,sf,s);                   \
                /* calculate last half-pixel: */            \
                AVERAGE(df,da,da,db);                       \
                /* store pixels: */                         \
                STORE(df,d,da);                             \
                STORE(df,d+BPP(df),db);                     \
                STORE(df,d+2*BPP(df),db);                   \
            } else {                                        \
                /* replicate last pixel: */                 \
                STORE(df,d,da);                             \
            }                                               \
        }                                                   \
    }

/*
 * Generic row 2x+ stretching converter:
 */
#define ROW_STRETCH2XPLUS(df,dest_ptr,dest_dx,sf,src_ptr,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d = dest_ptr;               \
        register unsigned char *s = src_ptr;                \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx << 1;  /* !!! */         \
        /* # of pixels mapped outside source image: */      \
        register int remainder = (2*dest_dx - limit) / step;\
        /* check row length: */                             \
        if (count) {                                        \
            /* load & convert first pixel: */               \
            PIXEL(df,da); PIXEL(df,db);                     \
            LOAD_CONVERT(df,da,sf,s);                       \
            s+=BPP(sf);                                     \
            /* update counter: */                           \
            if (!(count -= remainder))                      \
                goto end_of_row;                            \
            /* main loop: */                                \
            while (1) {                                     \
                /* replicate first integral pixel: */       \
                do {                                        \
                    STORE(df,d,da);                         \
                    d+=BPP(df);                             \
                    if (!(--count))                         \
                        goto end_of_row;                    \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* load & convert second pixel: */          \
                LOAD_CONVERT(df,db,sf,s);                   \
                /* calc & replicate first half-pixel: */    \
                AVERAGE(df,da,da,db);                       \
                do {                                        \
                    STORE(df,d,da);                         \
                    d+=BPP(df);                             \
                    if (!(--count))                         \
                        goto end_of_row;                    \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* replicate second integral pixel: */      \
                do {                                        \
                    STORE(df,d,db);                         \
                    d+=BPP(df);                             \
                    if (!(--count))                         \
                        goto end_of_row_2;                  \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* load & convert third pixel: */           \
                LOAD_CONVERT(df,da,sf,s+BPP(sf));           \
                s+=2*BPP(sf);                               \
                /* calc & replicate second half-pixel: */   \
                AVERAGE(df,db,db,da);                       \
                do {                                        \
                    STORE(df,d,db);                         \
                    d+=BPP(df);                             \
                    if (!(--count))                         \
                        goto end_of_row_2;                  \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
            }                                               \
            /* replicate the remaining pixels: */           \
end_of_row_2: COPY(df,da,db);                               \
end_of_row: while (remainder--) {                           \
                STORE(df,d,da);                             \
                d+=BPP(df);                                 \
            }                                               \
        }                                                   \
    }

/*** Generic RGB 1.5-row converters: ***********************/

/*
 * Generic 1.5-row shrinking converter:
 *  dest_ptr_1  - a previously converted row in destination image
 *  dest_ptr_12 - an area to store half-pixel average row
 *  dest_ptr_2  - an area to store next converted row from source image
 */
#define ROW2X_SHRINK(df,dest_ptr_1,dest_ptr_12,dest_ptr_2,dest_dx,sf,src_ptr,src_dx)   \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d1 = dest_ptr_1;            \
        register unsigned char *d12 = dest_ptr_12;          \
        register unsigned char *d2 = dest_ptr_2;            \
        register unsigned char *s = src_ptr;                \
        register int count = dest_dx;                       \
        register int limit = src_dx >> 1; /* -1 */          \
        register int step = dest_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            do {                                            \
                /* load & convert pixel from second row: */ \
                PIXEL(df,da);                               \
                LOAD_CONVERT(df,da,sf,s);                   \
                STORE(df,d2,da);                            \
                d2+=BPP(df);                                \
                /* average it with pixel from first row: */ \
                LOAD_AVERAGE(df,da,da,d1);                  \
                d1+=BPP(df);                                \
                STORE(df,d12,da);                           \
                d12+=BPP(df);                               \
                /* inverted Bresenham stepping: */          \
                do {                                        \
                    /* skip source pixel: */                \
                    s+=BPP(sf);                             \
                } while ((limit -= step) >= 0);             \
                limit += src_dx;                            \
            } while (--count);                              \
        }                                                   \
    }

/*
 * Generic 1.5-row copy converter:
 */
#define ROW2X_COPY(df,dest_ptr_1,dest_ptr_12,dest_ptr_2,dest_dx,sf,src_ptr,src_dx)     \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d1 = dest_ptr_1;            \
        register unsigned char *d12 = dest_ptr_12;          \
        register unsigned char *d2 = dest_ptr_2;            \
        register unsigned char *s = src_ptr;                \
        register int count = dest_dx;                       \
        /* convert misaligned pixels first: */              \
        while (((unsigned int)d2 & 3)                       \
            && ((unsigned int)s & 3) && count) {            \
            LOAD_CONVERT_AVERAGE_STORE(df,d1,d12,d2,sf,s);  \
            count --;                                       \
        }                                                   \
        /* the main loop (convert 4 pixels a time): */      \
        while (count >= 4) {                                \
            LOAD_CONVERT_AVERAGE_STORE_4(df,d1,d12,d2,sf,s);\
            count -= 4;                                     \
        }                                                   \
        /* convert the remaining pixels: */                 \
        while (count) {                                     \
            LOAD_CONVERT_AVERAGE_STORE(df,d1,d12,d2,sf,s);  \
            count --;                                       \
        }                                                   \
    }

/*
 * Generic 1.5-row stretching converter:
 *  (shall not be used when dest_dx/2 >= src_dx!!!)
 */
#define ROW2X_STRETCH(df,dest_ptr_1,dest_ptr_12,dest_ptr_2,dest_dx,sf,src_ptr,src_dx)  \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d1 = dest_ptr_1;            \
        register unsigned char *d12 = dest_ptr_12;          \
        register unsigned char *d2 = dest_ptr_2;            \
        register unsigned char *s = src_ptr;                \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx;                         \
        /* check row length: */                             \
        if (count) {                                        \
            goto start;                                     \
            /* the main loop: */                            \
            do {                                            \
                PIXEL(df,da); PIXEL(df,dc);                 \
                /* Bresenham stepping: */                   \
                if ((limit -= step) < 0) {                  \
                    limit += dest_dx;                       \
                    /* load & convert pixel: */             \
    start:          LOAD_CONVERT(df,da,sf,s);               \
                    s+=BPP(sf);                             \
                    /* calculate a half-pixel above: */     \
                    LOAD_AVERAGE(df,dc,da,d1);              \
                }                                           \
                /* replicate pixels: */                     \
                d1+=BPP(df);                                \
                STORE(df,d2,da);                            \
                d2+=BPP(df);                                \
                STORE(df,d12,dc);                           \
                d12+=BPP(df);                               \
            } while (--count);                              \
        }                                                   \
    }

/*
 * Generic 1.5-row 2x-stretching converter:
 */
#define ROW2X_STRETCH2X(df,dest_ptr_1,dest_ptr_12,dest_ptr_2,dest_dx,sf,src_ptr,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d1 = dest_ptr_1;            \
        register unsigned char *d12 = dest_ptr_12;          \
        register unsigned char *d2 = dest_ptr_2;            \
        register unsigned char *s = src_ptr;                \
        register int count = src_dx;                        \
        /* check row length: */                             \
        if (count) {                                        \
            /* load and convert first pixel: */             \
            PIXEL(df,da); PIXEL(df,dc);                     \
            LOAD_CONVERT(df,da,sf,s);                       \
            s+=BPP(sf);                                     \
            /* calculate a half-pixel above it: */          \
            LOAD_AVERAGE(df,dc,da,d1);                      \
            d1+=2*BPP(df);                                  \
            count --;                                       \
            /* store both: */                               \
            STORE(df,d2,da);                                \
            d2+=BPP(df);                                    \
            STORE(df,d12,dc);                               \
            d12+=BPP(df);                                   \
            /* main loop (process 2 pixels a time): */      \
            while (count >= 2) {                            \
                /* load & convert second integral pixel: */ \
                PIXEL(df,db); PIXEL(df,dd);                 \
                LOAD_CONVERT(df,db,sf,s);                   \
                /* calculate a half-pixel on the left: */   \
                AVERAGE(df,da,da,db);                       \
                /* store both pixels: */                    \
                STORE(df,d2,da);                            \
                STORE(df,d2+BPP(df),db);                    \
                /* calculate a half-pixel above: */         \
                LOAD_AVERAGE(df,dd,db,d1);                  \
                /* calculate a half-pixel above and left: */\
                AVERAGE(df,dc,dc,dd);                       \
                /* store both: */                           \
                STORE(df,d12,dc);                           \
                STORE(df,d12+BPP(df),dd);                   \
                /* load & convert third integral pixel: */  \
                LOAD_CONVERT(df,da,sf,s+BPP(sf));           \
                /* calculate a half-pixel on the left: */   \
                AVERAGE(df,db,db,da);                       \
                /* store both pixels: */                    \
                STORE(df,d2+2*BPP(df),db);                  \
                STORE(df,d2+3*BPP(df),da);                  \
                /* calculate a half-pixel above: */         \
                LOAD_AVERAGE(df,dc,da,d1+2*BPP(df));        \
                /* calculate a half-pixel above and left: */\
                AVERAGE(df,dd,dd,dc);                       \
                /* store both: */                           \
                STORE(df,d12+2*BPP(df),dd);                 \
                STORE(df,d12+3*BPP(df),dc);                 \
                /* shift pointers: */                       \
                s+=2*BPP(sf);                               \
                d1+=4*BPP(df);                              \
                d2+=4*BPP(df);                              \
                d12+=4*BPP(df);                             \
                count -= 2;                                 \
            }                                               \
            /* is there any more pixels to convert? */      \
            if (count) {                                    \
                /* load & convert last integral pixel: */   \
                PIXEL(df,db); PIXEL(df,dd);                 \
                LOAD_CONVERT(df,db,sf,s);                   \
                /* calculate a half-pixel on the left: */   \
                AVERAGE(df,da,da,db);                       \
                /* store pixels: */                         \
                STORE(df,d2,da);                            \
                STORE(df,d2+BPP(df),db);                    \
                STORE(df,d2+2*BPP(df),db);                  \
                /* calculate a half-pixel above: */         \
                LOAD_AVERAGE(df,dd,db,d1);                  \
                /* calculate a half-pixel above and left: */\
                AVERAGE(df,dc,dc,dd);                       \
                /* store pixels: */                         \
                STORE(df,d12,dc);                           \
                STORE(df,d12+BPP(df),dd);                   \
                STORE(df,d12+2*BPP(df),dd);                 \
            } else {                                        \
                /* replicate last pixels: */                \
                STORE(df,d2,da);                            \
                STORE(df,d12,dc);                           \
            }                                               \
        }                                                   \
    }

/*
 * Generic 1.5-row 2x+ stretching converter:
 */
#define ROW2X_STRETCH2XPLUS(df,dest_ptr_1,dest_ptr_12,dest_ptr_2,dest_dx,sf,src_ptr,src_dx) \
    {                                                       \
        /* initialize local variables: */                   \
        register unsigned char *d1 = dest_ptr_1;            \
        register unsigned char *d12 = dest_ptr_12;          \
        register unsigned char *d2 = dest_ptr_2;            \
        register unsigned char *s = src_ptr;                \
        register int count = dest_dx;                       \
        register int limit = dest_dx >> 1; /* !!! */        \
        register int step = src_dx << 1;  /* !!! */         \
        /* # of pixels mapped outside source image: */      \
        register int remainder = (2*dest_dx - limit) / step;\
        /* check row length: */                             \
        if (count) {                                        \
            /* load & convert first pixel: */               \
            PIXEL(df,da); PIXEL(df,db);                     \
            PIXEL(df,dc); PIXEL(df,dd);                     \
            LOAD_CONVERT(df,da,sf,s);                       \
            s+=BPP(sf);                                     \
            /* update counter: */                           \
            if (!(count -= remainder))                      \
                goto end_of_row;                            \
            /* main loop: */                                \
            while (1) {                                     \
                /* calculate a half-pixel above: */         \
                LOAD_AVERAGE(df,dc,da,d1);                  \
                /* replicate first pair of pixels: */       \
                do {                                        \
                    d1+=BPP(df);                            \
                    STORE(df,d2,da);                        \
                    d2+=BPP(df);                            \
                    STORE(df,d12,dc);                       \
                    d12+=BPP(df);                           \
                    if (!(--count))                         \
                        goto end_of_row;                    \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* load & convert second pixel: */          \
                LOAD_CONVERT(df,db,sf,s);                   \
                /* calculate half-pixel on the left: */     \
                AVERAGE(df,da,da,db);                       \
                /* calculate half-pixel above and left: */  \
                LOAD_AVERAGE(df,dc,da,d1);                  \
                /* replicate first pair of half-pixels: */  \
                do {                                        \
                    d1+=BPP(df);                            \
                    STORE(df,d2,da);                        \
                    d2+=BPP(df);                            \
                    STORE(df,d12,dc);                       \
                    d12+=BPP(df);                           \
                    if (!(--count))                         \
                        goto end_of_row;                    \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* calculate half-pixel above: */           \
                LOAD_AVERAGE(df,dd,db,d1);                  \
                /* replicate second pair of pixels: */      \
                do {                                        \
                    d1+=BPP(df);                            \
                    STORE(df,d2,db);                        \
                    d2+=BPP(df);                            \
                    STORE(df,d12,dd);                       \
                    d12+=BPP(df);                           \
                    if (!(--count))                         \
                        goto end_of_row_2;                  \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
                /* load & convert third pixel: */           \
                LOAD_CONVERT(df,da,sf,s+BPP(sf));           \
                s+=2*BPP(sf);                               \
                /* calculate half-pixel on the left: */     \
                AVERAGE(df,db,db,da);                       \
                /* calculate half-pixel above and left: */  \
                LOAD_AVERAGE(df,dd,db,d1);                  \
                /* replicate second pair of half-pixels: */ \
                do {                                        \
                    d1+=BPP(df);                            \
                    STORE(df,d2,db);                        \
                    d2+=BPP(df);                            \
                    STORE(df,d12,dd);                       \
                    d12+=BPP(df);                           \
                    if (!(--count))                         \
                        goto end_of_row_2;                  \
                } while ((limit -= step) >= 0);             \
                limit += dest_dx;                           \
            }                                               \
            /* replicate the remaining pixels: */           \
end_of_row_2:                                               \
            COPY(df,da,db);                                 \
            COPY(df,dc,dd);                                 \
end_of_row: while (remainder--) {                           \
                STORE(df,d2,da);                            \
                d2+=BPP(df);                                \
                STORE(df,d12,dc);                           \
                d12+=BPP(df);                               \
            }                                               \
        }                                                   \
    }

/***********************************************************/

/*
 * Function names:
 */
#define FN(df,sf)           sf##to##df
#define ROW_FN(df,sf,t)     sf##to##df##_ROW_##t
#define ROW2X_FN(df,sf,t)   sf##to##df##_ROW2X_##t

/*
 * Function replication macros:
 *  (row- and 1.5 row- converters)
 */
#define ROW_FUNC(df,sf,t)   \
    void ROW_FN(df,sf,t) (unsigned char *dest_ptr,          \
        int dest_dx, unsigned char *src_ptr, int src_dx)    \
        ROW_##t(df,dest_ptr,dest_dx,sf,src_ptr,src_dx)

#define ROW2X_FUNC(df,sf,t) \
    void ROW2X_FN(df,sf,t) (unsigned char *dest_ptr_1,      \
        unsigned char *dest_ptr_12, unsigned char *dest_ptr_2,\
        int dest_dx, unsigned char *src_ptr, int src_dx)    \
        ROW2X_##t(df,dest_ptr_1,dest_ptr_12,dest_ptr_2,dest_dx,sf,src_ptr,src_dx)


/***********************************************************/

/*
 * Actual row functions:
 */

ROW_FUNC(RGB32 ,RGB32 ,SHRINK)
ROW_FUNC(RGB32 ,RGB32 ,COPY)
ROW_FUNC(RGB32 ,RGB32 ,STRETCH)
ROW_FUNC(RGB32 ,RGB32 ,STRETCH2X)
ROW_FUNC(RGB32 ,RGB32 ,STRETCH2XPLUS)

ROW_FUNC(RGB32 ,BGR32 ,SHRINK)
ROW_FUNC(RGB32 ,BGR32 ,COPY)
ROW_FUNC(RGB32 ,BGR32 ,STRETCH)
ROW_FUNC(RGB32 ,BGR32 ,STRETCH2X)
ROW_FUNC(RGB32 ,BGR32 ,STRETCH2XPLUS)

ROW_FUNC(RGB32 ,RGB24 ,SHRINK)
ROW_FUNC(RGB32 ,RGB24 ,COPY)
ROW_FUNC(RGB32 ,RGB24 ,STRETCH)
ROW_FUNC(RGB32 ,RGB24 ,STRETCH2X)
ROW_FUNC(RGB32 ,RGB24 ,STRETCH2XPLUS)

ROW_FUNC(RGB32 ,RGB565,SHRINK)
ROW_FUNC(RGB32 ,RGB565,COPY)
ROW_FUNC(RGB32 ,RGB565,STRETCH)
ROW_FUNC(RGB32 ,RGB565,STRETCH2X)
ROW_FUNC(RGB32 ,RGB565,STRETCH2XPLUS)

ROW_FUNC(RGB32 ,RGB555,SHRINK)
ROW_FUNC(RGB32 ,RGB555,COPY)
ROW_FUNC(RGB32 ,RGB555,STRETCH)
ROW_FUNC(RGB32 ,RGB555,STRETCH2X)
ROW_FUNC(RGB32 ,RGB555,STRETCH2XPLUS)

ROW_FUNC(RGB32 ,RGB8  ,SHRINK)
ROW_FUNC(RGB32 ,RGB8  ,COPY)
ROW_FUNC(RGB32 ,RGB8  ,STRETCH)
ROW_FUNC(RGB32 ,RGB8  ,STRETCH2X)
ROW_FUNC(RGB32 ,RGB8  ,STRETCH2XPLUS)


ROW_FUNC(BGR32 ,RGB32 ,SHRINK)
ROW_FUNC(BGR32 ,RGB32 ,COPY)
ROW_FUNC(BGR32 ,RGB32 ,STRETCH)
ROW_FUNC(BGR32 ,RGB32 ,STRETCH2X)
ROW_FUNC(BGR32 ,RGB32 ,STRETCH2XPLUS)

ROW_FUNC(BGR32 ,BGR32 ,SHRINK)
ROW_FUNC(BGR32 ,BGR32 ,COPY)
ROW_FUNC(BGR32 ,BGR32 ,STRETCH)
ROW_FUNC(BGR32 ,BGR32 ,STRETCH2X)
ROW_FUNC(BGR32 ,BGR32 ,STRETCH2XPLUS)

ROW_FUNC(BGR32 ,RGB24 ,SHRINK)
ROW_FUNC(BGR32 ,RGB24 ,COPY)
ROW_FUNC(BGR32 ,RGB24 ,STRETCH)
ROW_FUNC(BGR32 ,RGB24 ,STRETCH2X)
ROW_FUNC(BGR32 ,RGB24 ,STRETCH2XPLUS)

ROW_FUNC(BGR32 ,RGB565,SHRINK)
ROW_FUNC(BGR32 ,RGB565,COPY)
ROW_FUNC(BGR32 ,RGB565,STRETCH)
ROW_FUNC(BGR32 ,RGB565,STRETCH2X)
ROW_FUNC(BGR32 ,RGB565,STRETCH2XPLUS)

ROW_FUNC(BGR32 ,RGB555,SHRINK)
ROW_FUNC(BGR32 ,RGB555,COPY)
ROW_FUNC(BGR32 ,RGB555,STRETCH)
ROW_FUNC(BGR32 ,RGB555,STRETCH2X)
ROW_FUNC(BGR32 ,RGB555,STRETCH2XPLUS)

ROW_FUNC(BGR32 ,RGB8  ,SHRINK)
ROW_FUNC(BGR32 ,RGB8  ,COPY)
ROW_FUNC(BGR32 ,RGB8  ,STRETCH)
ROW_FUNC(BGR32 ,RGB8  ,STRETCH2X)
ROW_FUNC(BGR32 ,RGB8  ,STRETCH2XPLUS)


ROW_FUNC(RGB24 ,RGB32 ,SHRINK)
ROW_FUNC(RGB24 ,RGB32 ,COPY)
ROW_FUNC(RGB24 ,RGB32 ,STRETCH)
ROW_FUNC(RGB24 ,RGB32 ,STRETCH2X)
ROW_FUNC(RGB24 ,RGB32 ,STRETCH2XPLUS)

ROW_FUNC(RGB24 ,BGR32 ,SHRINK)
ROW_FUNC(RGB24 ,BGR32 ,COPY)
ROW_FUNC(RGB24 ,BGR32 ,STRETCH)
ROW_FUNC(RGB24 ,BGR32 ,STRETCH2X)
ROW_FUNC(RGB24 ,BGR32 ,STRETCH2XPLUS)

ROW_FUNC(RGB24 ,RGB24 ,SHRINK)
ROW_FUNC(RGB24 ,RGB24 ,COPY)
ROW_FUNC(RGB24 ,RGB24 ,STRETCH)
ROW_FUNC(RGB24 ,RGB24 ,STRETCH2X)
ROW_FUNC(RGB24 ,RGB24 ,STRETCH2XPLUS)

ROW_FUNC(RGB24 ,RGB565,SHRINK)
ROW_FUNC(RGB24 ,RGB565,COPY)
ROW_FUNC(RGB24 ,RGB565,STRETCH)
ROW_FUNC(RGB24 ,RGB565,STRETCH2X)
ROW_FUNC(RGB24 ,RGB565,STRETCH2XPLUS)

ROW_FUNC(RGB24 ,RGB555,SHRINK)
ROW_FUNC(RGB24 ,RGB555,COPY)
ROW_FUNC(RGB24 ,RGB555,STRETCH)
ROW_FUNC(RGB24 ,RGB555,STRETCH2X)
ROW_FUNC(RGB24 ,RGB555,STRETCH2XPLUS)

ROW_FUNC(RGB24 ,RGB8  ,SHRINK)
ROW_FUNC(RGB24 ,RGB8  ,COPY)
ROW_FUNC(RGB24 ,RGB8  ,STRETCH)
ROW_FUNC(RGB24 ,RGB8  ,STRETCH2X)
ROW_FUNC(RGB24 ,RGB8  ,STRETCH2XPLUS)


ROW_FUNC(RGB565,RGB32 ,SHRINK)
ROW_FUNC(RGB565,RGB32 ,COPY)
ROW_FUNC(RGB565,RGB32 ,STRETCH)
ROW_FUNC(RGB565,RGB32 ,STRETCH2X)
ROW_FUNC(RGB565,RGB32 ,STRETCH2XPLUS)

ROW_FUNC(RGB565,BGR32 ,SHRINK)
ROW_FUNC(RGB565,BGR32 ,COPY)
ROW_FUNC(RGB565,BGR32 ,STRETCH)
ROW_FUNC(RGB565,BGR32 ,STRETCH2X)
ROW_FUNC(RGB565,BGR32 ,STRETCH2XPLUS)

ROW_FUNC(RGB565,RGB24 ,SHRINK)
ROW_FUNC(RGB565,RGB24 ,COPY)
ROW_FUNC(RGB565,RGB24 ,STRETCH)
ROW_FUNC(RGB565,RGB24 ,STRETCH2X)
ROW_FUNC(RGB565,RGB24 ,STRETCH2XPLUS)

ROW_FUNC(RGB565,RGB565,SHRINK)
ROW_FUNC(RGB565,RGB565,COPY)
ROW_FUNC(RGB565,RGB565,STRETCH)
ROW_FUNC(RGB565,RGB565,STRETCH2X)
ROW_FUNC(RGB565,RGB565,STRETCH2XPLUS)

ROW_FUNC(RGB565,RGB555,SHRINK)
ROW_FUNC(RGB565,RGB555,COPY)
ROW_FUNC(RGB565,RGB555,STRETCH)
ROW_FUNC(RGB565,RGB555,STRETCH2X)
ROW_FUNC(RGB565,RGB555,STRETCH2XPLUS)

ROW_FUNC(RGB565,RGB8  ,SHRINK)
ROW_FUNC(RGB565,RGB8  ,COPY)
ROW_FUNC(RGB565,RGB8  ,STRETCH)
ROW_FUNC(RGB565,RGB8  ,STRETCH2X)
ROW_FUNC(RGB565,RGB8  ,STRETCH2XPLUS)


ROW_FUNC(RGB555,RGB32 ,SHRINK)
ROW_FUNC(RGB555,RGB32 ,COPY)
ROW_FUNC(RGB555,RGB32 ,STRETCH)
ROW_FUNC(RGB555,RGB32 ,STRETCH2X)
ROW_FUNC(RGB555,RGB32 ,STRETCH2XPLUS)

ROW_FUNC(RGB555,BGR32 ,SHRINK)
ROW_FUNC(RGB555,BGR32 ,COPY)
ROW_FUNC(RGB555,BGR32 ,STRETCH)
ROW_FUNC(RGB555,BGR32 ,STRETCH2X)
ROW_FUNC(RGB555,BGR32 ,STRETCH2XPLUS)

ROW_FUNC(RGB555,RGB24 ,SHRINK)
ROW_FUNC(RGB555,RGB24 ,COPY)
ROW_FUNC(RGB555,RGB24 ,STRETCH)
ROW_FUNC(RGB555,RGB24 ,STRETCH2X)
ROW_FUNC(RGB555,RGB24 ,STRETCH2XPLUS)

ROW_FUNC(RGB555,RGB565,SHRINK)
ROW_FUNC(RGB555,RGB565,COPY)
ROW_FUNC(RGB555,RGB565,STRETCH)
ROW_FUNC(RGB555,RGB565,STRETCH2X)
ROW_FUNC(RGB555,RGB565,STRETCH2XPLUS)

ROW_FUNC(RGB555,RGB555,SHRINK)
ROW_FUNC(RGB555,RGB555,COPY)
ROW_FUNC(RGB555,RGB555,STRETCH)
ROW_FUNC(RGB555,RGB555,STRETCH2X)
ROW_FUNC(RGB555,RGB555,STRETCH2XPLUS)

ROW_FUNC(RGB555,RGB8  ,SHRINK)
ROW_FUNC(RGB555,RGB8  ,COPY)
ROW_FUNC(RGB555,RGB8  ,STRETCH)
ROW_FUNC(RGB555,RGB8  ,STRETCH2X)
ROW_FUNC(RGB555,RGB8  ,STRETCH2XPLUS)


ROW_FUNC(RGB8  ,RGB32 ,SHRINK)
ROW_FUNC(RGB8  ,RGB32 ,COPY)
ROW_FUNC(RGB8  ,RGB32 ,STRETCH)
ROW_FUNC(RGB8  ,RGB32 ,STRETCH2X)
ROW_FUNC(RGB8  ,RGB32 ,STRETCH2XPLUS)

ROW_FUNC(RGB8  ,BGR32 ,SHRINK)
ROW_FUNC(RGB8  ,BGR32 ,COPY)
ROW_FUNC(RGB8  ,BGR32 ,STRETCH)
ROW_FUNC(RGB8  ,BGR32 ,STRETCH2X)
ROW_FUNC(RGB8  ,BGR32 ,STRETCH2XPLUS)

ROW_FUNC(RGB8  ,RGB24 ,SHRINK)
ROW_FUNC(RGB8  ,RGB24 ,COPY)
ROW_FUNC(RGB8  ,RGB24 ,STRETCH)
ROW_FUNC(RGB8  ,RGB24 ,STRETCH2X)
ROW_FUNC(RGB8  ,RGB24 ,STRETCH2XPLUS)

ROW_FUNC(RGB8  ,RGB565,SHRINK)
ROW_FUNC(RGB8  ,RGB565,COPY)
ROW_FUNC(RGB8  ,RGB565,STRETCH)
ROW_FUNC(RGB8  ,RGB565,STRETCH2X)
ROW_FUNC(RGB8  ,RGB565,STRETCH2XPLUS)

ROW_FUNC(RGB8  ,RGB555,SHRINK)
ROW_FUNC(RGB8  ,RGB555,COPY)
ROW_FUNC(RGB8  ,RGB555,STRETCH)
ROW_FUNC(RGB8  ,RGB555,STRETCH2X)
ROW_FUNC(RGB8  ,RGB555,STRETCH2XPLUS)

ROW_FUNC(RGB8  ,RGB8  ,SHRINK)
ROW_FUNC(RGB8  ,RGB8  ,COPY)
ROW_FUNC(RGB8  ,RGB8  ,STRETCH)
ROW_FUNC(RGB8  ,RGB8  ,STRETCH2X)
ROW_FUNC(RGB8  ,RGB8  ,STRETCH2XPLUS)


/*
 * Actual 1.5-row conversion functions:
 */

ROW2X_FUNC(RGB32 ,RGB32 ,SHRINK)
ROW2X_FUNC(RGB32 ,RGB32 ,COPY)
ROW2X_FUNC(RGB32 ,RGB32 ,STRETCH)
ROW2X_FUNC(RGB32 ,RGB32 ,STRETCH2X)
ROW2X_FUNC(RGB32 ,RGB32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB32 ,BGR32 ,SHRINK)
ROW2X_FUNC(RGB32 ,BGR32 ,COPY)
ROW2X_FUNC(RGB32 ,BGR32 ,STRETCH)
ROW2X_FUNC(RGB32 ,BGR32 ,STRETCH2X)
ROW2X_FUNC(RGB32 ,BGR32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB32 ,RGB24 ,SHRINK)
ROW2X_FUNC(RGB32 ,RGB24 ,COPY)
ROW2X_FUNC(RGB32 ,RGB24 ,STRETCH)
ROW2X_FUNC(RGB32 ,RGB24 ,STRETCH2X)
ROW2X_FUNC(RGB32 ,RGB24 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB32 ,RGB565,SHRINK)
ROW2X_FUNC(RGB32 ,RGB565,COPY)
ROW2X_FUNC(RGB32 ,RGB565,STRETCH)
ROW2X_FUNC(RGB32 ,RGB565,STRETCH2X)
ROW2X_FUNC(RGB32 ,RGB565,STRETCH2XPLUS)

ROW2X_FUNC(RGB32 ,RGB555,SHRINK)
ROW2X_FUNC(RGB32 ,RGB555,COPY)
ROW2X_FUNC(RGB32 ,RGB555,STRETCH)
ROW2X_FUNC(RGB32 ,RGB555,STRETCH2X)
ROW2X_FUNC(RGB32 ,RGB555,STRETCH2XPLUS)

ROW2X_FUNC(RGB32 ,RGB8  ,SHRINK)
ROW2X_FUNC(RGB32 ,RGB8  ,COPY)
ROW2X_FUNC(RGB32 ,RGB8  ,STRETCH)
ROW2X_FUNC(RGB32 ,RGB8  ,STRETCH2X)
ROW2X_FUNC(RGB32 ,RGB8  ,STRETCH2XPLUS)


ROW2X_FUNC(BGR32 ,RGB32 ,SHRINK)
ROW2X_FUNC(BGR32 ,RGB32 ,COPY)
ROW2X_FUNC(BGR32 ,RGB32 ,STRETCH)
ROW2X_FUNC(BGR32 ,RGB32 ,STRETCH2X)
ROW2X_FUNC(BGR32 ,RGB32 ,STRETCH2XPLUS)

ROW2X_FUNC(BGR32 ,BGR32 ,SHRINK)
ROW2X_FUNC(BGR32 ,BGR32 ,COPY)
ROW2X_FUNC(BGR32 ,BGR32 ,STRETCH)
ROW2X_FUNC(BGR32 ,BGR32 ,STRETCH2X)
ROW2X_FUNC(BGR32 ,BGR32 ,STRETCH2XPLUS)

ROW2X_FUNC(BGR32 ,RGB24 ,SHRINK)
ROW2X_FUNC(BGR32 ,RGB24 ,COPY)
ROW2X_FUNC(BGR32 ,RGB24 ,STRETCH)
ROW2X_FUNC(BGR32 ,RGB24 ,STRETCH2X)
ROW2X_FUNC(BGR32 ,RGB24 ,STRETCH2XPLUS)

ROW2X_FUNC(BGR32 ,RGB565,SHRINK)
ROW2X_FUNC(BGR32 ,RGB565,COPY)
ROW2X_FUNC(BGR32 ,RGB565,STRETCH)
ROW2X_FUNC(BGR32 ,RGB565,STRETCH2X)
ROW2X_FUNC(BGR32 ,RGB565,STRETCH2XPLUS)

ROW2X_FUNC(BGR32 ,RGB555,SHRINK)
ROW2X_FUNC(BGR32 ,RGB555,COPY)
ROW2X_FUNC(BGR32 ,RGB555,STRETCH)
ROW2X_FUNC(BGR32 ,RGB555,STRETCH2X)
ROW2X_FUNC(BGR32 ,RGB555,STRETCH2XPLUS)

ROW2X_FUNC(BGR32 ,RGB8  ,SHRINK)
ROW2X_FUNC(BGR32 ,RGB8  ,COPY)
ROW2X_FUNC(BGR32 ,RGB8  ,STRETCH)
ROW2X_FUNC(BGR32 ,RGB8  ,STRETCH2X)
ROW2X_FUNC(BGR32 ,RGB8  ,STRETCH2XPLUS)


ROW2X_FUNC(RGB24 ,RGB32 ,SHRINK)
ROW2X_FUNC(RGB24 ,RGB32 ,COPY)
ROW2X_FUNC(RGB24 ,RGB32 ,STRETCH)
ROW2X_FUNC(RGB24 ,RGB32 ,STRETCH2X)
ROW2X_FUNC(RGB24 ,RGB32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB24 ,BGR32 ,SHRINK)
ROW2X_FUNC(RGB24 ,BGR32 ,COPY)
ROW2X_FUNC(RGB24 ,BGR32 ,STRETCH)
ROW2X_FUNC(RGB24 ,BGR32 ,STRETCH2X)
ROW2X_FUNC(RGB24 ,BGR32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB24 ,RGB24 ,SHRINK)
ROW2X_FUNC(RGB24 ,RGB24 ,COPY)
ROW2X_FUNC(RGB24 ,RGB24 ,STRETCH)
ROW2X_FUNC(RGB24 ,RGB24 ,STRETCH2X)
ROW2X_FUNC(RGB24 ,RGB24 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB24 ,RGB565,SHRINK)
ROW2X_FUNC(RGB24 ,RGB565,COPY)
ROW2X_FUNC(RGB24 ,RGB565,STRETCH)
ROW2X_FUNC(RGB24 ,RGB565,STRETCH2X)
ROW2X_FUNC(RGB24 ,RGB565,STRETCH2XPLUS)

ROW2X_FUNC(RGB24 ,RGB555,SHRINK)
ROW2X_FUNC(RGB24 ,RGB555,COPY)
ROW2X_FUNC(RGB24 ,RGB555,STRETCH)
ROW2X_FUNC(RGB24 ,RGB555,STRETCH2X)
ROW2X_FUNC(RGB24 ,RGB555,STRETCH2XPLUS)

ROW2X_FUNC(RGB24 ,RGB8  ,SHRINK)
ROW2X_FUNC(RGB24 ,RGB8  ,COPY)
ROW2X_FUNC(RGB24 ,RGB8  ,STRETCH)
ROW2X_FUNC(RGB24 ,RGB8  ,STRETCH2X)
ROW2X_FUNC(RGB24 ,RGB8  ,STRETCH2XPLUS)


ROW2X_FUNC(RGB565,RGB32 ,SHRINK)
ROW2X_FUNC(RGB565,RGB32 ,COPY)
ROW2X_FUNC(RGB565,RGB32 ,STRETCH)
ROW2X_FUNC(RGB565,RGB32 ,STRETCH2X)
ROW2X_FUNC(RGB565,RGB32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB565,BGR32 ,SHRINK)
ROW2X_FUNC(RGB565,BGR32 ,COPY)
ROW2X_FUNC(RGB565,BGR32 ,STRETCH)
ROW2X_FUNC(RGB565,BGR32 ,STRETCH2X)
ROW2X_FUNC(RGB565,BGR32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB565,RGB24 ,SHRINK)
ROW2X_FUNC(RGB565,RGB24 ,COPY)
ROW2X_FUNC(RGB565,RGB24 ,STRETCH)
ROW2X_FUNC(RGB565,RGB24 ,STRETCH2X)
ROW2X_FUNC(RGB565,RGB24 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB565,RGB565,SHRINK)
ROW2X_FUNC(RGB565,RGB565,COPY)
ROW2X_FUNC(RGB565,RGB565,STRETCH)
ROW2X_FUNC(RGB565,RGB565,STRETCH2X)
ROW2X_FUNC(RGB565,RGB565,STRETCH2XPLUS)

ROW2X_FUNC(RGB565,RGB555,SHRINK)
ROW2X_FUNC(RGB565,RGB555,COPY)
ROW2X_FUNC(RGB565,RGB555,STRETCH)
ROW2X_FUNC(RGB565,RGB555,STRETCH2X)
ROW2X_FUNC(RGB565,RGB555,STRETCH2XPLUS)

ROW2X_FUNC(RGB565,RGB8  ,SHRINK)
ROW2X_FUNC(RGB565,RGB8  ,COPY)
ROW2X_FUNC(RGB565,RGB8  ,STRETCH)
ROW2X_FUNC(RGB565,RGB8  ,STRETCH2X)
ROW2X_FUNC(RGB565,RGB8  ,STRETCH2XPLUS)


ROW2X_FUNC(RGB555,RGB32 ,SHRINK)
ROW2X_FUNC(RGB555,RGB32 ,COPY)
ROW2X_FUNC(RGB555,RGB32 ,STRETCH)
ROW2X_FUNC(RGB555,RGB32 ,STRETCH2X)
ROW2X_FUNC(RGB555,RGB32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB555,BGR32 ,SHRINK)
ROW2X_FUNC(RGB555,BGR32 ,COPY)
ROW2X_FUNC(RGB555,BGR32 ,STRETCH)
ROW2X_FUNC(RGB555,BGR32 ,STRETCH2X)
ROW2X_FUNC(RGB555,BGR32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB555,RGB24 ,SHRINK)
ROW2X_FUNC(RGB555,RGB24 ,COPY)
ROW2X_FUNC(RGB555,RGB24 ,STRETCH)
ROW2X_FUNC(RGB555,RGB24 ,STRETCH2X)
ROW2X_FUNC(RGB555,RGB24 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB555,RGB565,SHRINK)
ROW2X_FUNC(RGB555,RGB565,COPY)
ROW2X_FUNC(RGB555,RGB565,STRETCH)
ROW2X_FUNC(RGB555,RGB565,STRETCH2X)
ROW2X_FUNC(RGB555,RGB565,STRETCH2XPLUS)

ROW2X_FUNC(RGB555,RGB555,SHRINK)
ROW2X_FUNC(RGB555,RGB555,COPY)
ROW2X_FUNC(RGB555,RGB555,STRETCH)
ROW2X_FUNC(RGB555,RGB555,STRETCH2X)
ROW2X_FUNC(RGB555,RGB555,STRETCH2XPLUS)

ROW2X_FUNC(RGB555,RGB8  ,SHRINK)
ROW2X_FUNC(RGB555,RGB8  ,COPY)
ROW2X_FUNC(RGB555,RGB8  ,STRETCH)
ROW2X_FUNC(RGB555,RGB8  ,STRETCH2X)
ROW2X_FUNC(RGB555,RGB8  ,STRETCH2XPLUS)


ROW2X_FUNC(RGB8  ,RGB32 ,SHRINK)
ROW2X_FUNC(RGB8  ,RGB32 ,COPY)
ROW2X_FUNC(RGB8  ,RGB32 ,STRETCH)
ROW2X_FUNC(RGB8  ,RGB32 ,STRETCH2X)
ROW2X_FUNC(RGB8  ,RGB32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB8  ,BGR32 ,SHRINK)
ROW2X_FUNC(RGB8  ,BGR32 ,COPY)
ROW2X_FUNC(RGB8  ,BGR32 ,STRETCH)
ROW2X_FUNC(RGB8  ,BGR32 ,STRETCH2X)
ROW2X_FUNC(RGB8  ,BGR32 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB8  ,RGB24 ,SHRINK)
ROW2X_FUNC(RGB8  ,RGB24 ,COPY)
ROW2X_FUNC(RGB8  ,RGB24 ,STRETCH)
ROW2X_FUNC(RGB8  ,RGB24 ,STRETCH2X)
ROW2X_FUNC(RGB8  ,RGB24 ,STRETCH2XPLUS)

ROW2X_FUNC(RGB8  ,RGB565,SHRINK)
ROW2X_FUNC(RGB8  ,RGB565,COPY)
ROW2X_FUNC(RGB8  ,RGB565,STRETCH)
ROW2X_FUNC(RGB8  ,RGB565,STRETCH2X)
ROW2X_FUNC(RGB8  ,RGB565,STRETCH2XPLUS)

ROW2X_FUNC(RGB8  ,RGB555,SHRINK)
ROW2X_FUNC(RGB8  ,RGB555,COPY)
ROW2X_FUNC(RGB8  ,RGB555,STRETCH)
ROW2X_FUNC(RGB8  ,RGB555,STRETCH2X)
ROW2X_FUNC(RGB8  ,RGB555,STRETCH2XPLUS)

ROW2X_FUNC(RGB8  ,RGB8  ,SHRINK)
ROW2X_FUNC(RGB8  ,RGB8  ,COPY)
ROW2X_FUNC(RGB8  ,RGB8  ,STRETCH)
ROW2X_FUNC(RGB8  ,RGB8  ,STRETCH2X)
ROW2X_FUNC(RGB8  ,RGB8  ,STRETCH2XPLUS)


/*
 * Row scale function selection tables:
 *  [destination format][source format][row scale type]
 */
void (* RowFuncs [RGB_FORMATS][RGB_FORMATS][SCALE_FUNCS]) (
    unsigned char *dest_ptr, int dest_dx, unsigned char *src_ptr, int src_dx) =
{
    {   {
            ROW_FN(RGB32 ,RGB32 ,SHRINK),
            ROW_FN(RGB32 ,RGB32 ,COPY),
            ROW_FN(RGB32 ,RGB32 ,STRETCH),
            ROW_FN(RGB32 ,RGB32 ,STRETCH2X),
            ROW_FN(RGB32 ,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB32 ,BGR32 ,SHRINK),
            ROW_FN(RGB32 ,BGR32 ,COPY),
            ROW_FN(RGB32 ,BGR32 ,STRETCH),
            ROW_FN(RGB32 ,BGR32 ,STRETCH2X),
            ROW_FN(RGB32 ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB32 ,RGB24 ,SHRINK),
            ROW_FN(RGB32 ,RGB24 ,COPY),
            ROW_FN(RGB32 ,RGB24 ,STRETCH),
            ROW_FN(RGB32 ,RGB24 ,STRETCH2X),
            ROW_FN(RGB32 ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
            0,
            0,
	    0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB32 ,RGB565,SHRINK),
            ROW_FN(RGB32 ,RGB565,COPY),
            ROW_FN(RGB32 ,RGB565,STRETCH),
            ROW_FN(RGB32 ,RGB565,STRETCH2X),
            ROW_FN(RGB32 ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB32 ,RGB555,SHRINK),
            ROW_FN(RGB32 ,RGB555,COPY),
            ROW_FN(RGB32 ,RGB555,STRETCH),
            ROW_FN(RGB32 ,RGB555,STRETCH2X),
            ROW_FN(RGB32 ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB32 ,RGB8  ,SHRINK),
            ROW_FN(RGB32 ,RGB8  ,COPY),
            ROW_FN(RGB32 ,RGB8  ,STRETCH),
            ROW_FN(RGB32 ,RGB8  ,STRETCH2X),
            ROW_FN(RGB32 ,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
            ROW_FN(BGR32 ,RGB32 ,SHRINK),
            ROW_FN(BGR32 ,RGB32 ,COPY),
            ROW_FN(BGR32 ,RGB32 ,STRETCH),
            ROW_FN(BGR32 ,RGB32 ,STRETCH2X),
            ROW_FN(BGR32 ,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(BGR32 ,BGR32 ,SHRINK),
            ROW_FN(BGR32 ,BGR32 ,COPY),
            ROW_FN(BGR32 ,BGR32 ,STRETCH),
            ROW_FN(BGR32 ,BGR32 ,STRETCH2X),
            ROW_FN(BGR32 ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(BGR32 ,RGB24 ,SHRINK),
            ROW_FN(BGR32 ,RGB24 ,COPY),
            ROW_FN(BGR32 ,RGB24 ,STRETCH),
            ROW_FN(BGR32 ,RGB24 ,STRETCH2X),
            ROW_FN(BGR32 ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(BGR32 ,RGB565,SHRINK),
            ROW_FN(BGR32 ,RGB565,COPY),
            ROW_FN(BGR32 ,RGB565,STRETCH),
            ROW_FN(BGR32 ,RGB565,STRETCH2X),
            ROW_FN(BGR32 ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(BGR32 ,RGB555,SHRINK),
            ROW_FN(BGR32 ,RGB555,COPY),
            ROW_FN(BGR32 ,RGB555,STRETCH),
            ROW_FN(BGR32 ,RGB555,STRETCH2X),
            ROW_FN(BGR32 ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
	    ROW_FN(BGR32 ,RGB8  ,SHRINK),
            ROW_FN(BGR32 ,RGB8  ,COPY),
            ROW_FN(BGR32 ,RGB8  ,STRETCH),
            ROW_FN(BGR32 ,RGB8  ,STRETCH2X),
            ROW_FN(BGR32 ,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
            ROW_FN(RGB24 ,RGB32 ,SHRINK),
            ROW_FN(RGB24 ,RGB32 ,COPY),
            ROW_FN(RGB24 ,RGB32 ,STRETCH),
            ROW_FN(RGB24 ,RGB32 ,STRETCH2X),
            ROW_FN(RGB24 ,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB24 ,BGR32 ,SHRINK),
            ROW_FN(RGB24 ,BGR32 ,COPY),
            ROW_FN(RGB24 ,BGR32 ,STRETCH),
            ROW_FN(RGB24 ,BGR32 ,STRETCH2X),
            ROW_FN(RGB24 ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB24 ,RGB24 ,SHRINK),
            ROW_FN(RGB24 ,RGB24 ,COPY),
            ROW_FN(RGB24 ,RGB24 ,STRETCH),
            ROW_FN(RGB24 ,RGB24 ,STRETCH2X),
            ROW_FN(RGB24 ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB24 ,RGB565,SHRINK),
            ROW_FN(RGB24 ,RGB565,COPY),
            ROW_FN(RGB24 ,RGB565,STRETCH),
            ROW_FN(RGB24 ,RGB565,STRETCH2X),
            ROW_FN(RGB24 ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB24 ,RGB555,SHRINK),
            ROW_FN(RGB24 ,RGB555,COPY),
            ROW_FN(RGB24 ,RGB555,STRETCH),
            ROW_FN(RGB24 ,RGB555,STRETCH2X),
            ROW_FN(RGB24 ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB24 ,RGB8  ,SHRINK),
            ROW_FN(RGB24 ,RGB8  ,COPY),
            ROW_FN(RGB24 ,RGB8  ,STRETCH),
            ROW_FN(RGB24 ,RGB8  ,STRETCH2X),
            ROW_FN(RGB24 ,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        }
    },{ {
            ROW_FN(RGB565,RGB32 ,SHRINK),
            ROW_FN(RGB565,RGB32 ,COPY),
            ROW_FN(RGB565,RGB32 ,STRETCH),
            ROW_FN(RGB565,RGB32 ,STRETCH2X),
            ROW_FN(RGB565,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB565,BGR32 ,SHRINK),
            ROW_FN(RGB565,BGR32 ,COPY),
            ROW_FN(RGB565,BGR32 ,STRETCH),
            ROW_FN(RGB565,BGR32 ,STRETCH2X),
            ROW_FN(RGB565,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB565,RGB24 ,SHRINK),
            ROW_FN(RGB565,RGB24 ,COPY),
            ROW_FN(RGB565,RGB24 ,STRETCH),
            ROW_FN(RGB565,RGB24 ,STRETCH2X),
            ROW_FN(RGB565,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB565,RGB565,SHRINK),
            ROW_FN(RGB565,RGB565,COPY),
            ROW_FN(RGB565,RGB565,STRETCH),
            ROW_FN(RGB565,RGB565,STRETCH2X),
            ROW_FN(RGB565,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB565,RGB555,SHRINK),
            ROW_FN(RGB565,RGB555,COPY),
            ROW_FN(RGB565,RGB555,STRETCH),
            ROW_FN(RGB565,RGB555,STRETCH2X),
            ROW_FN(RGB565,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB565,RGB8  ,SHRINK),
            ROW_FN(RGB565,RGB8  ,COPY),
            ROW_FN(RGB565,RGB8  ,STRETCH),
            ROW_FN(RGB565,RGB8  ,STRETCH2X),
            ROW_FN(RGB565,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
            ROW_FN(RGB555,RGB32 ,SHRINK),
            ROW_FN(RGB555,RGB32 ,COPY),
            ROW_FN(RGB555,RGB32 ,STRETCH),
            ROW_FN(RGB555,RGB32 ,STRETCH2X),
            ROW_FN(RGB555,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB555,BGR32 ,SHRINK),
            ROW_FN(RGB555,BGR32 ,COPY),
            ROW_FN(RGB555,BGR32 ,STRETCH),
            ROW_FN(RGB555,BGR32 ,STRETCH2X),
            ROW_FN(RGB555,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB555,RGB24 ,SHRINK),
            ROW_FN(RGB555,RGB24 ,COPY),
            ROW_FN(RGB555,RGB24 ,STRETCH),
            ROW_FN(RGB555,RGB24 ,STRETCH2X),
            ROW_FN(RGB555,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB555,RGB565,SHRINK),
            ROW_FN(RGB555,RGB565,COPY),
            ROW_FN(RGB555,RGB565,STRETCH),
            ROW_FN(RGB555,RGB565,STRETCH2X),
            ROW_FN(RGB555,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW_FN(RGB555,RGB555,SHRINK),
            ROW_FN(RGB555,RGB555,COPY),
            ROW_FN(RGB555,RGB555,STRETCH),
            ROW_FN(RGB555,RGB555,STRETCH2X),
            ROW_FN(RGB555,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB555,RGB8  ,SHRINK),
            ROW_FN(RGB555,RGB8  ,COPY),
            ROW_FN(RGB555,RGB8  ,STRETCH),
            ROW_FN(RGB555,RGB8  ,STRETCH2X),
            ROW_FN(RGB555,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
            0,
            0,
            0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        }
    },{ {
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB8  ,RGB32 ,SHRINK),
            ROW_FN(RGB8  ,RGB32 ,COPY),
            ROW_FN(RGB8  ,RGB32 ,STRETCH),
            ROW_FN(RGB8  ,RGB32 ,STRETCH2X),
            ROW_FN(RGB8  ,RGB32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB8  ,BGR32 ,SHRINK),
            ROW_FN(RGB8  ,BGR32 ,COPY),
            ROW_FN(RGB8  ,BGR32 ,STRETCH),
            ROW_FN(RGB8  ,BGR32 ,STRETCH2X),
            ROW_FN(RGB8  ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB8  ,RGB24 ,SHRINK),
            ROW_FN(RGB8  ,RGB24 ,COPY),
            ROW_FN(RGB8  ,RGB24 ,STRETCH),
            ROW_FN(RGB8  ,RGB24 ,STRETCH2X),
            ROW_FN(RGB8  ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB8  ,RGB565,SHRINK),
            ROW_FN(RGB8  ,RGB565,COPY),
            ROW_FN(RGB8  ,RGB565,STRETCH),
            ROW_FN(RGB8  ,RGB565,STRETCH2X),
            ROW_FN(RGB8  ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB8  ,RGB555,SHRINK),
            ROW_FN(RGB8  ,RGB555,COPY),
            ROW_FN(RGB8  ,RGB555,STRETCH),
            ROW_FN(RGB8  ,RGB555,STRETCH2X),
            ROW_FN(RGB8  ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW_FN(RGB8  ,RGB8  ,SHRINK),
            ROW_FN(RGB8  ,RGB8  ,COPY),
            ROW_FN(RGB8  ,RGB8  ,STRETCH),
            ROW_FN(RGB8  ,RGB8  ,STRETCH2X),
            ROW_FN(RGB8  ,RGB8  ,STRETCH2XPLUS)
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
 * 1.5-row scale function selection tables:
 *  [destination format][source format][row scale type]
 */
void (* Row2xFuncs [RGB_FORMATS][RGB_FORMATS][SCALE_FUNCS]) (
    unsigned char *dest_ptr_1, unsigned char *dest_ptr_12,
    unsigned char *dest_ptr_2, int dest_dx, unsigned char *src_ptr, int src_dx) =
{
    {   {
            ROW2X_FN(RGB32 ,RGB32 ,SHRINK),
            ROW2X_FN(RGB32 ,RGB32 ,COPY),
            ROW2X_FN(RGB32 ,RGB32 ,STRETCH),
            ROW2X_FN(RGB32 ,RGB32 ,STRETCH2X),
            ROW2X_FN(RGB32 ,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB32 ,BGR32 ,SHRINK),
            ROW2X_FN(RGB32 ,BGR32 ,COPY),
            ROW2X_FN(RGB32 ,BGR32 ,STRETCH),
            ROW2X_FN(RGB32 ,BGR32 ,STRETCH2X),
            ROW2X_FN(RGB32 ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB32 ,RGB24 ,SHRINK),
            ROW2X_FN(RGB32 ,RGB24 ,COPY),
            ROW2X_FN(RGB32 ,RGB24 ,STRETCH),
            ROW2X_FN(RGB32 ,RGB24 ,STRETCH2X),
            ROW2X_FN(RGB32 ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB32 ,RGB565,SHRINK),
            ROW2X_FN(RGB32 ,RGB565,COPY),
            ROW2X_FN(RGB32 ,RGB565,STRETCH),
            ROW2X_FN(RGB32 ,RGB565,STRETCH2X),
            ROW2X_FN(RGB32 ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB32 ,RGB555,SHRINK),
            ROW2X_FN(RGB32 ,RGB555,COPY),
            ROW2X_FN(RGB32 ,RGB555,STRETCH),
            ROW2X_FN(RGB32 ,RGB555,STRETCH2X),
            ROW2X_FN(RGB32 ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT

            ROW2X_FN(RGB32 ,RGB8  ,SHRINK),
            ROW2X_FN(RGB32 ,RGB8  ,COPY),
            ROW2X_FN(RGB32 ,RGB8  ,STRETCH),
            ROW2X_FN(RGB32 ,RGB8  ,STRETCH2X),
            ROW2X_FN(RGB32 ,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
            ROW2X_FN(BGR32 ,RGB32 ,SHRINK),
            ROW2X_FN(BGR32 ,RGB32 ,COPY),
            ROW2X_FN(BGR32 ,RGB32 ,STRETCH),
            ROW2X_FN(BGR32 ,RGB32 ,STRETCH2X),
            ROW2X_FN(BGR32 ,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(BGR32 ,BGR32 ,SHRINK),
            ROW2X_FN(BGR32 ,BGR32 ,COPY),
            ROW2X_FN(BGR32 ,BGR32 ,STRETCH),
            ROW2X_FN(BGR32 ,BGR32 ,STRETCH2X),
            ROW2X_FN(BGR32 ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(BGR32 ,RGB24 ,SHRINK),
            ROW2X_FN(BGR32 ,RGB24 ,COPY),
            ROW2X_FN(BGR32 ,RGB24 ,STRETCH),
            ROW2X_FN(BGR32 ,RGB24 ,STRETCH2X),
            ROW2X_FN(BGR32 ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(BGR32 ,RGB565,SHRINK),
            ROW2X_FN(BGR32 ,RGB565,COPY),
            ROW2X_FN(BGR32 ,RGB565,STRETCH),
            ROW2X_FN(BGR32 ,RGB565,STRETCH2X),
            ROW2X_FN(BGR32 ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(BGR32 ,RGB555,SHRINK),
            ROW2X_FN(BGR32 ,RGB555,COPY),
            ROW2X_FN(BGR32 ,RGB555,STRETCH),
            ROW2X_FN(BGR32 ,RGB555,STRETCH2X),
            ROW2X_FN(BGR32 ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(BGR32 ,RGB8  ,SHRINK),
            ROW2X_FN(BGR32 ,RGB8  ,COPY),
            ROW2X_FN(BGR32 ,RGB8  ,STRETCH),
            ROW2X_FN(BGR32 ,RGB8  ,STRETCH2X),
            ROW2X_FN(BGR32 ,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
            ROW2X_FN(RGB24 ,RGB32 ,SHRINK),
            ROW2X_FN(RGB24 ,RGB32 ,COPY),
            ROW2X_FN(RGB24 ,RGB32 ,STRETCH),
            ROW2X_FN(RGB24 ,RGB32 ,STRETCH2X),
            ROW2X_FN(RGB24 ,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB24 ,BGR32 ,SHRINK),
            ROW2X_FN(RGB24 ,BGR32 ,COPY),
            ROW2X_FN(RGB24 ,BGR32 ,STRETCH),
            ROW2X_FN(RGB24 ,BGR32 ,STRETCH2X),
            ROW2X_FN(RGB24 ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB24 ,RGB24 ,SHRINK),
            ROW2X_FN(RGB24 ,RGB24 ,COPY),
            ROW2X_FN(RGB24 ,RGB24 ,STRETCH),
            ROW2X_FN(RGB24 ,RGB24 ,STRETCH2X),
            ROW2X_FN(RGB24 ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB24 ,RGB565,SHRINK),
            ROW2X_FN(RGB24 ,RGB565,COPY),
            ROW2X_FN(RGB24 ,RGB565,STRETCH),
            ROW2X_FN(RGB24 ,RGB565,STRETCH2X),
            ROW2X_FN(RGB24 ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB24 ,RGB555,SHRINK),
            ROW2X_FN(RGB24 ,RGB555,COPY),
            ROW2X_FN(RGB24 ,RGB555,STRETCH),
            ROW2X_FN(RGB24 ,RGB555,STRETCH2X),
            ROW2X_FN(RGB24 ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB24 ,RGB8  ,SHRINK),
            ROW2X_FN(RGB24 ,RGB8  ,COPY),
            ROW2X_FN(RGB24 ,RGB8  ,STRETCH),
            ROW2X_FN(RGB24 ,RGB8  ,STRETCH2X),
            ROW2X_FN(RGB24 ,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
            ROW2X_FN(RGB565,RGB32 ,SHRINK),
            ROW2X_FN(RGB565,RGB32 ,COPY),
            ROW2X_FN(RGB565,RGB32 ,STRETCH),
            ROW2X_FN(RGB565,RGB32 ,STRETCH2X),
            ROW2X_FN(RGB565,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB565,BGR32 ,SHRINK),
            ROW2X_FN(RGB565,BGR32 ,COPY),
            ROW2X_FN(RGB565,BGR32 ,STRETCH),
            ROW2X_FN(RGB565,BGR32 ,STRETCH2X),
            ROW2X_FN(RGB565,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB565,RGB24 ,SHRINK),
            ROW2X_FN(RGB565,RGB24 ,COPY),
            ROW2X_FN(RGB565,RGB24 ,STRETCH),
            ROW2X_FN(RGB565,RGB24 ,STRETCH2X),
            ROW2X_FN(RGB565,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB565,RGB565,SHRINK),
            ROW2X_FN(RGB565,RGB565,COPY),
            ROW2X_FN(RGB565,RGB565,STRETCH),
            ROW2X_FN(RGB565,RGB565,STRETCH2X),
            ROW2X_FN(RGB565,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB565,RGB555,SHRINK),
            ROW2X_FN(RGB565,RGB555,COPY),
            ROW2X_FN(RGB565,RGB555,STRETCH),
            ROW2X_FN(RGB565,RGB555,STRETCH2X),
            ROW2X_FN(RGB565,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB565,RGB8  ,SHRINK),
            ROW2X_FN(RGB565,RGB8  ,COPY),
            ROW2X_FN(RGB565,RGB8  ,STRETCH),
            ROW2X_FN(RGB565,RGB8  ,STRETCH2X),
            ROW2X_FN(RGB565,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
            ROW2X_FN(RGB555,RGB32 ,SHRINK),
            ROW2X_FN(RGB555,RGB32 ,COPY),
            ROW2X_FN(RGB555,RGB32 ,STRETCH),
            ROW2X_FN(RGB555,RGB32 ,STRETCH2X),
            ROW2X_FN(RGB555,RGB32 ,STRETCH2XPLUS)
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB555,BGR32 ,SHRINK),
            ROW2X_FN(RGB555,BGR32 ,COPY),
            ROW2X_FN(RGB555,BGR32 ,STRETCH),
            ROW2X_FN(RGB555,BGR32 ,STRETCH2X),
            ROW2X_FN(RGB555,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB555,RGB24 ,SHRINK),
            ROW2X_FN(RGB555,RGB24 ,COPY),
            ROW2X_FN(RGB555,RGB24 ,STRETCH),
            ROW2X_FN(RGB555,RGB24 ,STRETCH2X),
            ROW2X_FN(RGB555,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB555,RGB565,SHRINK),
            ROW2X_FN(RGB555,RGB565,COPY),
            ROW2X_FN(RGB555,RGB565,STRETCH),
            ROW2X_FN(RGB555,RGB565,STRETCH2X),
            ROW2X_FN(RGB555,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _FAT_HXCOLOR
            ROW2X_FN(RGB555,RGB555,SHRINK),
            ROW2X_FN(RGB555,RGB555,COPY),
            ROW2X_FN(RGB555,RGB555,STRETCH),
            ROW2X_FN(RGB555,RGB555,STRETCH2X),
            ROW2X_FN(RGB555,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB555,RGB8  ,SHRINK),
            ROW2X_FN(RGB555,RGB8  ,COPY),
            ROW2X_FN(RGB555,RGB8  ,STRETCH),
            ROW2X_FN(RGB555,RGB8  ,STRETCH2X),
            ROW2X_FN(RGB555,RGB8  ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        }
    },{ {
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        },{
            0,
            0,
            0,
            0,
            0
        },{
	    0,
	    0,
	    0,
            0,
            0
        }
    },{ {
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB8  ,RGB32 ,SHRINK),
            ROW2X_FN(RGB8  ,RGB32 ,COPY),
            ROW2X_FN(RGB8  ,RGB32 ,STRETCH),
            ROW2X_FN(RGB8  ,RGB32 ,STRETCH2X),
            ROW2X_FN(RGB8  ,RGB32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB8  ,BGR32 ,SHRINK),
            ROW2X_FN(RGB8  ,BGR32 ,COPY),
            ROW2X_FN(RGB8  ,BGR32 ,STRETCH),
            ROW2X_FN(RGB8  ,BGR32 ,STRETCH2X),
            ROW2X_FN(RGB8  ,BGR32 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB8  ,RGB24 ,SHRINK),
            ROW2X_FN(RGB8  ,RGB24 ,COPY),
            ROW2X_FN(RGB8  ,RGB24 ,STRETCH),
            ROW2X_FN(RGB8  ,RGB24 ,STRETCH2X),
            ROW2X_FN(RGB8  ,RGB24 ,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB8  ,RGB565,SHRINK),
            ROW2X_FN(RGB8  ,RGB565,COPY),
            ROW2X_FN(RGB8  ,RGB565,STRETCH),
            ROW2X_FN(RGB8  ,RGB565,STRETCH2X),
            ROW2X_FN(RGB8  ,RGB565,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB8  ,RGB555,SHRINK),
            ROW2X_FN(RGB8  ,RGB555,COPY),
            ROW2X_FN(RGB8  ,RGB555,STRETCH),
            ROW2X_FN(RGB8  ,RGB555,STRETCH2X),
            ROW2X_FN(RGB8  ,RGB555,STRETCH2XPLUS)
#else
	    0,
	    0,
	    0,
            0,
            0
#endif
        },{ /* rgb444 stub */
            0,
            0,
            0,
            0,
            0
        },{
#ifdef _8_BIT_SUPPORT
            ROW2X_FN(RGB8  ,RGB8  ,SHRINK),
            ROW2X_FN(RGB8  ,RGB8  ,COPY),
            ROW2X_FN(RGB8  ,RGB8  ,STRETCH),
            ROW2X_FN(RGB8  ,RGB8  ,STRETCH2X),
            ROW2X_FN(RGB8  ,RGB8  ,STRETCH2XPLUS)
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


/*** RGB image converters: *********************************/

/*
 * Image shrink converter:
 */
void IMAGE_SHRINK (
    unsigned char *dest_ptr, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_ptr, int src_dx, int src_dy, int src_pitch, int src_bpp,
    void (*row_func) (unsigned char *, int, unsigned char *, int),
    void (*row2x_func) (unsigned char *, unsigned char *, unsigned char *,
        int, unsigned char *, int))
{
    /* initialize local variables: */
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dy;
    register int limit = src_dy >> 1; /* -1 */
    register int step = dest_dy;
    /* check image height: */
    if (count) {
        /* the main loop: */
        do {
            /* call row-converter: */
            (* row_func) (d, dest_dx, s, src_dx);
            d += dest_pitch;
            /* inverted Bresenham stepping: */
            do {
                /* skip rows: */
                s += src_pitch;
            } while ((limit -= step) >= 0);
            limit += src_dy;
        } while (--count);
    }
}

/*
 * Image copy converter:
 */
void IMAGE_COPY (
    unsigned char *dest_ptr, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_ptr, int src_dx, int src_dy, int src_pitch, int src_bpp,
    void (*row_func) (unsigned char *, int, unsigned char *, int),
    void (*row2x_func) (unsigned char *, unsigned char *, unsigned char *,
        int, unsigned char *, int))
{
    /* initialize local variables: */
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dy;
    /* check image height: */
    if (count) {
        /* the main loop: */
        do {
            /* call row-converter: */
            (* row_func) (d, dest_dx, s, src_dx);
            /* shift pointers: */
            s += src_pitch;
            d += dest_pitch;
        } while (--count);
    }
}

/*
 * Image stretching converter:
 *  (shall not be used when dest_dy/2 >= src_dy!!!)
 */
void IMAGE_STRETCH (
    unsigned char *dest_ptr, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_ptr, int src_dx, int src_dy, int src_pitch, int src_bpp,
    void (*row_func) (unsigned char *, int, unsigned char *, int),
    void (*row2x_func) (unsigned char *, unsigned char *, unsigned char *,
        int, unsigned char *, int))
{
    /* initialize local variables: */
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = dest_dy;
    register int limit = dest_dy >> 1; /* !!! */
    register int step = src_dy;
    /* check image height: */
    if (count) {
        goto start;
        /* the main loop: */
        do {
            /* Bresenham stepping: */
            if ((limit -= step) < 0) {
                limit += dest_dy;
                /* convert a new row: */
    start:      (* row_func) (d, dest_dx, s, src_dx);
                s += src_pitch;
                d += dest_pitch;
            } else {
                /* replicate last row: */
                memcpy(d, d - dest_pitch, dest_dx_bytes); /* Flawfinder: ignore */
                d += dest_pitch;
            }
        } while (--count);
    }
}


/*
 * Image 2x-stretching converter:
 */
void IMAGE_STRETCH2X (
    unsigned char *dest_ptr, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_ptr, int src_dx, int src_dy, int src_pitch, int src_bpp,
    void (*row_func) (unsigned char *, int, unsigned char *, int),
    void (*row2x_func) (unsigned char *, unsigned char *, unsigned char *,
        int, unsigned char *, int))
{
    /* initialize local variables: */
    register unsigned char *d1 = dest_ptr;
    register unsigned char *d12 = dest_ptr + dest_pitch;
    register unsigned char *d2 = dest_ptr + dest_pitch * 2;
    register unsigned char *s = src_ptr;
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = src_dy;
    /* check image height: */
    if (count) {
        /* convert first row: */
        (* row_func) (d1, dest_dx, s, src_dx);
        s += src_pitch;
        /* the main loop: */
        while (--count) {
            /* process second & half-pixel rows: */
            (* row2x_func) (d1, d12, d2, dest_dx, s, src_dx);
            d1 += 2 * dest_pitch;
            d12 += 2 * dest_pitch;
            d2 += 2 * dest_pitch;
            s += src_pitch;
        }
        /* replicate the last converted row: */
        memcpy (d12, d1, dest_dx_bytes); /* Flawfinder: ignore */
    }
}

/*
 * Image 2x+ stretching converter:
 */
void IMAGE_STRETCH2XPLUS (
    unsigned char *dest_ptr, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_ptr, int src_dx, int src_dy, int src_pitch, int src_bpp,
    void (*row_func) (unsigned char *, int, unsigned char *, int),
    void (*row2x_func) (unsigned char *, unsigned char *, unsigned char *,
        int, unsigned char *, int))
{
    /* initialize local variables: */
    register unsigned char *d = dest_ptr, *d1;
    register unsigned char *s = src_ptr;
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = dest_dy;
    register int limit = dest_dy >> 1; /* !!! */
    register int step = src_dy << 1;   /* !!! */
    /* # of rows mapped outside source image: */
    register int remainder = (2*dest_dy - limit) / step;
    /* check destination image height: */
    if (count) {
        /* convert first row: */
        (* row_func) (d, dest_dx, s, src_dx);
        s += src_pitch;
        /* update count: */
        if (!(count -= remainder)) {
            remainder --;
            goto end;
        }
        /* main loop: */
        while (1) {
            /* replicate last converted integral row: */
            goto start;
            do {
                /* replicate a row: */
                memcpy(d + dest_pitch, d, dest_dx_bytes); /* Flawfinder: ignore */
                d += dest_pitch;
start:
                /* check if all rows are filled up: */
                if (!(--count))
                    goto end;
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* store current location, and scan until next integral row: */
            d1 = d;
            do {
                /* just shift a pointer: */
                d += dest_pitch;
                /* check if all rows are filled up: */
                if (!(--count))
                    goto end_2;
            } while ((limit -= step) >= 0);
            limit += dest_dy;
            /* convert next integral & previous half-pixel rows: */
            (* row2x_func) (d1, d1 + dest_pitch, d + dest_pitch, dest_dx, s, src_dx);
            d1 += dest_pitch;
            s += src_pitch;
            /* replicate half-pixel row: */
            while (d1 != d) {   /* can't use < pitch can be negative!!! */
                /* now, we can copy it: */
                memcpy(d1 + dest_pitch, d1, dest_dx_bytes); /* Flawfinder: ignore */
                d1 += dest_pitch;
            }
            /* select last converted row: */
            d += dest_pitch;
        }
        /* fill space reserved for half-pixel row: */
end_2:
        while (d1 != d) {   /* can't use < pitch can be negative!!! */
            memcpy(d1 + dest_pitch, d1, dest_dx_bytes); /* Flawfinder: ignore */
            d1 += dest_pitch;
        }
        /* copy the remaining rows: */
end:
        while (remainder--) {
            memcpy(d + dest_pitch, d, dest_dx_bytes); /* Flawfinder: ignore */
            d += dest_pitch;
        }
    }
}

/*
 * Image scale functions table:
 *  [vertical scale type]
 */
void (* ImageFuncs [SCALE_FUNCS]) (
    unsigned char *dest_ptr, int dest_dx, int dest_dy, int dest_pitch, int dest_bpp,
    unsigned char *src_ptr, int src_dx, int src_dy, int src_pitch, int src_bpp,
    void (*row_func) (unsigned char *, int, unsigned char *, int),
    void (*row2x_func) (unsigned char *, unsigned char *, unsigned char *,
        int, unsigned char *, int)) =
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
int bpp [RGB_FORMATS] =
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
 * The main RGBtoRGB converter:
 */
int RGBtoRGB (
    /* destination image parameters: */
    int dest_format, unsigned char *dest_ptr, int dest_width, int dest_height,
    int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
    /* source image parameters: */
    int src_format, unsigned char *src_ptr, int src_width, int src_height,
    int src_pitch, int src_x, int src_y, int src_dx, int src_dy)
{
    /* pointers to low-level converters to use: */
    void (*row_proc) (unsigned char *, int, unsigned char *, int);
    void (*row2x_proc) (unsigned char *, unsigned char *, unsigned char *, int,
        unsigned char *, int);
    void (*image_proc) (unsigned char *, int, int, int, int,
        unsigned char *, int, int, int, int,
        void (*) (unsigned char *, int, unsigned char *, int),
        void (*) (unsigned char *, unsigned char *, unsigned char *, int,
            unsigned char *, int));
    /* scale types: */
    register int scale_x, scale_y;

    /* pointers and pixel depths: */
    register int dest_bpp, src_bpp;
    register unsigned char *d, *s;

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
    row_proc   = RowFuncs [dest_format][src_format][scale_x];
    row2x_proc = Row2xFuncs [dest_format][src_format][scale_x];
    image_proc = ImageFuncs [scale_y];

    if (!row_proc  || !row2x_proc || !image_proc)
    {
        return 1;
    }

    /* get pixel depths: */
    dest_bpp = bpp [dest_format];
    src_bpp = bpp [src_format];

    /* check if bottom-up bitmaps: */
    if (dest_pitch < 0) dest_ptr -= (dest_height-1) * dest_pitch;
    if (src_pitch < 0)  src_ptr -= (src_height-1) * src_pitch;

    /* get pointers: */
    d = dest_ptr + dest_x * dest_bpp + dest_y * dest_pitch;
    s = src_ptr + src_x * src_bpp + src_y * src_pitch;

    /* pass control to appropriate lower-level converters: */
    (* image_proc) (d, dest_dx, dest_dy, dest_pitch, dest_bpp,
        s, src_dx, src_dy, src_pitch, src_bpp, row_proc, row2x_proc);

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

#define RGBTORGB_FUNC(df,sf)                                \
    int FN(df,sf) (unsigned char *dest_ptr,                 \
        int dest_width, int dest_height, int dest_pitch,    \
        int dest_x, int dest_y, int dest_dx, int dest_dy,   \
        unsigned char *src_ptr,                             \
        int src_width, int src_height, int src_pitch,       \
        int src_x, int src_y, int src_dx, int src_dy)       \
    {                                                       \
        return RGBtoRGB(                                    \
            ID(df), dest_ptr, dest_width, dest_height,      \
            dest_pitch, dest_x, dest_y, dest_dx, dest_dy,   \
            ID(sf), src_ptr, src_width, src_height,         \
            src_pitch, src_x, src_y, src_dx, src_dy);       \
    }


RGBTORGB_FUNC(RGB32 ,RGB32 )
RGBTORGB_FUNC(RGB32 ,BGR32 )
RGBTORGB_FUNC(RGB32 ,RGB24 )
RGBTORGB_FUNC(RGB32 ,RGB565)
RGBTORGB_FUNC(RGB32 ,RGB555)
RGBTORGB_FUNC(RGB32 ,RGB8  )

RGBTORGB_FUNC(BGR32 ,RGB32 )
RGBTORGB_FUNC(BGR32 ,BGR32 )
RGBTORGB_FUNC(BGR32 ,RGB24 )
RGBTORGB_FUNC(BGR32 ,RGB565)
RGBTORGB_FUNC(BGR32 ,RGB555)
RGBTORGB_FUNC(BGR32 ,RGB8  )

RGBTORGB_FUNC(RGB24 ,RGB32 )
RGBTORGB_FUNC(RGB24 ,BGR32 )
RGBTORGB_FUNC(RGB24 ,RGB24 )
RGBTORGB_FUNC(RGB24 ,RGB565)
RGBTORGB_FUNC(RGB24 ,RGB555)
RGBTORGB_FUNC(RGB24 ,RGB8  )

RGBTORGB_FUNC(RGB565,RGB32 )
RGBTORGB_FUNC(RGB565,BGR32 )
RGBTORGB_FUNC(RGB565,RGB24 )
RGBTORGB_FUNC(RGB565,RGB565)
RGBTORGB_FUNC(RGB565,RGB555)
RGBTORGB_FUNC(RGB565,RGB8  )

RGBTORGB_FUNC(RGB555,RGB32 )
RGBTORGB_FUNC(RGB555,BGR32 )
RGBTORGB_FUNC(RGB555,RGB24 )
RGBTORGB_FUNC(RGB555,RGB565)
RGBTORGB_FUNC(RGB555,RGB555)
RGBTORGB_FUNC(RGB555,RGB8  )

RGBTORGB_FUNC(RGB8  ,RGB32 )
RGBTORGB_FUNC(RGB8  ,BGR32 )
RGBTORGB_FUNC(RGB8  ,RGB24 )
RGBTORGB_FUNC(RGB8  ,RGB565)
RGBTORGB_FUNC(RGB8  ,RGB555)
RGBTORGB_FUNC(RGB8  ,RGB8  )


/***********************************************************/
#ifdef TESTALL

/*
 * Enumerates all possible formats/sizes and checks
 * if conversion is performed correctly.
 */

/* C RunTime Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* image buffers: */
#define MAX_WIDTH   176
#define MAX_PITCH   ((MAX_WIDTH + 1) * 4)
#define MAX_HEIGHT  144

static char src_image [(MAX_HEIGHT+1) * MAX_PITCH]; /* Flawfinder: ignore */
static char dest_image [(MAX_HEIGHT+1) * MAX_PITCH]; /* Flawfinder: ignore */

/*
 * Unified Set/Get pixel functions:
 */
static void GetPixel (int format, unsigned char *ptr, int *pr, int *pg, int *pb)
{
#   define  GET_PIXEL(f,s,r,g,b)    \
        {                           \
            PIXEL(f,a);             \
            LOAD(f,s,a);            \
            r = GET_R(f,a);         \
            g = GET_G(f,a);         \
            b = GET_B(f,a);         \
        }

    switch (format) {
        case ID(RGB32) : GET_PIXEL(RGB32 ,ptr,*pr,*pg,*pb); break;
        case ID(BGR32) : GET_PIXEL(BGR32 ,ptr,*pr,*pg,*pb); break;
        case ID(RGB24) : GET_PIXEL(RGB24 ,ptr,*pr,*pg,*pb); break;
        case ID(RGB565): GET_PIXEL(RGB565,ptr,*pr,*pg,*pb); break;
        case ID(RGB555): GET_PIXEL(RGB555,ptr,*pr,*pg,*pb); break;
        case ID(RGB8)  : GET_PIXEL(RGB8  ,ptr,*pr,*pg,*pb); break;
    }
}

static void SetPixel (int format, unsigned char *ptr, int r, int g, int b)
{
#   define  SET_PIXEL(f,d,r,g,b)    \
        {                           \
            PIXEL(f,a);             \
            SET(f,a,r,g,b);         \
            STORE(f,d,a);           \
        }

    switch (format) {
        case ID(RGB32) : SET_PIXEL(RGB32 ,ptr,r,g,b); break;
        case ID(BGR32) : SET_PIXEL(BGR32 ,ptr,r,g,b); break;
        case ID(RGB24) : SET_PIXEL(RGB24 ,ptr,r,g,b); break;
        case ID(RGB565): SET_PIXEL(RGB565,ptr,r,g,b); break;
        case ID(RGB555): SET_PIXEL(RGB555,ptr,r,g,b); break;
        case ID(RGB8)  : SET_PIXEL(RGB8  ,ptr,r,g,b); break;
    }
}

/*
 * Test pixel value:
 */
#define TEST_R    0x80
#define TEST_G    0x80
#define TEST_B    0x80

/*
 * The test program:
 */
int main (int argc, char *argv[])
{
    int src_format, src_width, src_height;
    int dest_format, dest_width, dest_height;

	/* register color in the palette: */
	PAL_SET(1,TEST_R,TEST_G,TEST_B);
    PMAP_SET(1,TEST_R,TEST_G,TEST_B);

    /* the main loop: */
    for (src_format=0; src_format<RGB_FORMATS; src_format++)
    for (dest_format=1; dest_format<RGB_FORMATS; dest_format++)
    for (src_width=1; src_width<=MAX_WIDTH; src_width++)
    for (src_height=1; src_height<=MAX_HEIGHT; src_height++)
    for (dest_width=1; dest_width<=MAX_WIDTH; dest_width++)
    for (dest_height=1; dest_height<=MAX_HEIGHT; dest_height++)
    {
        register unsigned char *s, *d ;
        register int i, j;

        /* print diagnostic message: */
        printf ("testing conversion: (%d,%d,%d) -> (%d,%d,%d)... ",
            src_format, src_height, src_width, dest_format, dest_height, dest_width);

        /* clear buffers: */
        memset (src_image, 0, sizeof(src_image));
        memset (dest_image, 0, sizeof(dest_image));

        /* create source image: */
        s = src_image;
        for (i=0; i<src_height; i++) {
            for (j=0; j<src_width; j++) {
                SetPixel(src_format, s, TEST_R, TEST_G, TEST_B);
                s += bpp[src_format];
            }
            s += MAX_PITCH - src_width * bpp[src_format];
        }

        /* call color converter: */
        if (RGBtoRGB (dest_format, dest_image, dest_width, dest_height,
            MAX_PITCH, 0, 0, dest_width, dest_height, src_format, src_image,
            src_width, src_height, MAX_PITCH, 0, 0, src_width, src_height)) {
            printf ("RGBtoRGb error!!");
            getch();
        }

        /* test converted image: */
        d = dest_image;
        for (i=0; i<dest_height; i++) {
            int r,g,b;
            for (j=0; j<dest_width; j++) {
                GetPixel(dest_format, d, &r, &g, &b);
                if (r != TEST_R || g != TEST_G || b != TEST_B) {
                    printf ("{%d,%d}",i,j);
                    getch();
                }
                d += bpp[dest_format];
            }
            /* check pixels outside the frame !!!*/
            GetPixel(dest_format, d, &r, &g, &b);
            if (r != 0 || g != 0 || b != 0) {
                printf ("!!!}%d,%d{",i,j);
                getch();
            }
            d += MAX_PITCH - dest_width * bpp[dest_format];
        }
        printf ("done.\n");
    }
}
#endif

/***********************************************************/
#ifdef TEST
/*
 * Test program.
 * Use:
 *  RGB2RGB <output file> <input file> [<new width> <new height>] [<new format>]
 */

/* Note: use /Zp1 (no structure alignments) to compile it !!! */

/* Windows Header Files: */
#include <windows.h>

/* C RunTime Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Main program: */
int main (int argc, char *argv[])
{
    /* local variables: */
    FILE *ifp, *ofp;
    int width = 0, height = 0, format = 0;
    static struct BMPHDR {
        BITMAPFILEHEADER hdr;               /* bitmap file-header */
        BITMAPINFOHEADER bihdr;             /* bitmap info-header */
    } src_bmphdr, dest_bmphdr;
    int src_image_size, dest_image_size;
    unsigned char *src_image, *dest_image;
	register int i;

    int (* func) (unsigned char *dest_ptr, int dest_width, int dest_height,
        int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
        unsigned char *src_ptr, int src_width, int src_height, int src_pitch,       \
        int src_x, int src_y, int src_dx, int src_dy);

    /* process program arguments: */
    if (argc < 3) {
        fprintf (stderr, "Use:\n\tRGB2RGB <output file> <input file> [width] [height] [format]\n");
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

    /* read input file header: */
	i = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    if (fread ((void *)&src_bmphdr, 1, i, ifp) != i) {
		fprintf (stderr, "Cannot read input file.\n");
		exit (EXIT_FAILURE);
	}

    /* here we will deal with simplest BMPs only: */
    if (src_bmphdr.bihdr.biCompression != BI_RGB
     || src_bmphdr.hdr.bfOffBits != i) {
        fprintf (stderr, "Sorry, I cannot understand this type of bitmap.\n");
		exit (EXIT_FAILURE);
    }

    /* prepare output file header: */
    memcpy((void *)&dest_bmphdr, (void *)&src_bmphdr, i); /* Flawfinder: ignore */
    if (width)
        dest_bmphdr.bihdr.biWidth = width;
    if (height)
        dest_bmphdr.bihdr.biHeight = height;

    /* allocate memory for source/dest. images: */
    src_image_size = src_bmphdr.bihdr.biWidth * src_bmphdr.bihdr.biHeight * src_bmphdr.bihdr.biBitCount / 8;
    dest_image_size = dest_bmphdr.bihdr.biWidth * dest_bmphdr.bihdr.biHeight * dest_bmphdr.bihdr.biBitCount / 8;
    if ((src_image = (unsigned char *)malloc(src_image_size)) == NULL ||
        (dest_image = (unsigned char *)malloc(dest_image_size)) == NULL) {
        fprintf (stderr, "Out of memory.\n");
		exit (EXIT_FAILURE);
    }

    /* read input frame: */
    if (fread (src_image, 1, src_image_size, ifp) != src_image_size) {
		fprintf (stderr, "Cannot read input file.\n");
        goto free_exit;
    }

    /* select color converter to use: */
    switch (src_bmphdr.bihdr.biBitCount)
    {
        case 32: func = RGB32toRGB32;   break;
        case 24: func = RGB24toRGB24;   break;
        case 16: func = RGB555toRGB555; break;
        case 8:  func = RGB8toRGB8;     break;
        default:
            fprintf (stderr, "Unsupported pixel depth.\n");
            goto free_exit;
    }

    /* call it: */
    if ((* func)
        (dest_image, dest_bmphdr.bihdr.biWidth, dest_bmphdr.bihdr.biHeight,
        dest_bmphdr.bihdr.biWidth * dest_bmphdr.bihdr.biBitCount / 8,
        0, 0, dest_bmphdr.bihdr.biWidth, dest_bmphdr.bihdr.biHeight,
        src_image, src_bmphdr.bihdr.biWidth, src_bmphdr.bihdr.biHeight,
        src_bmphdr.bihdr.biWidth * src_bmphdr.bihdr.biBitCount / 8,
        0, 0, src_bmphdr.bihdr.biWidth, src_bmphdr.bihdr.biHeight))
    {
        fprintf (stderr, "Format conversion error.\n");
        goto free_exit;
    }

    /* save converted image: */
    if (fwrite ((void *)&dest_bmphdr, 1, i, ofp) != i
     || fwrite (dest_image, 1, dest_image_size, ofp) != dest_image_size) {
        fprintf (stderr, "Cannot save file.\n");
    }

free_exit:
    /* free memory: */
    free (src_image);
    free (dest_image);

	/* close files & exit: */
	fclose (ifp);
	fclose (ofp);

    return EXIT_SUCCESS;
}
#endif

/* rgb2rgb.c -- end of file */

