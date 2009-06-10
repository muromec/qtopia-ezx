/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rgbcommon.c,v 1.4 2007/07/06 20:53:52 jfinnecy Exp $
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

#include "hlxclib/string.h"
#include "nostatic/yuv.h"
#include "nostatic/colorlib.h"
#include "nostatic/rgb.h"
#include "nostatic/rgbcommon.h"


void IMAGE_SHRINK (unsigned char *dest_ptr, int dest_dx, int dest_dy,
                   int dest_pitch, int dest_bpp, unsigned char *src_ptr,
                   int src_dx, int src_dy, int src_pitch, int src_bpp,
                   void (*row_func) (unsigned char *, int, unsigned char *, int),
                   void (*row2x_func) (unsigned char *, unsigned char *,
                                       unsigned char *, int, unsigned char *, int))
{
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dy;
    register int limit = src_dy >> 1;
    register int step = dest_dy;

    if (count)
    {
        do
        {
            (*row_func) (d, dest_dx, s, src_dx);
            d += dest_pitch;

            do
            {
                s += src_pitch;
            }
            while ((limit -= step) >= 0);
            limit += src_dy;
        }
        while (--count);
    }
}




void
IMAGE_COPY (unsigned char *dest_ptr, int dest_dx, int dest_dy, int dest_pitch,
            int dest_bpp, unsigned char *src_ptr, int src_dx, int src_dy,
            int src_pitch, int src_bpp, void (*row_func) (unsigned char *,
                                                          int,
                                                          unsigned char *,
                                                          int),
            void (*row2x_func) (unsigned char *, unsigned char *,
                                unsigned char *, int, unsigned char *, int))
{

    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dy;

    if (count)
    {
        do
        {
            (*row_func) (d, dest_dx, s, src_dx);
            s += src_pitch;
            d += dest_pitch;
        }
        while (--count);
    }
}





void
IMAGE_STRETCH (unsigned char *dest_ptr, int dest_dx, int dest_dy,
               int dest_pitch, int dest_bpp, unsigned char *src_ptr,
               int src_dx, int src_dy, int src_pitch, int src_bpp,
               void (*row_func) (unsigned char *, int, unsigned char *, int),
               void (*row2x_func) (unsigned char *, unsigned char *,
                                   unsigned char *, int, unsigned char *,
                                   int))
{

    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = dest_dy;
    register int limit = dest_dy >> 1;
    register int step = src_dy;

    if (count)
    {
        goto start;

        do
        {

            if ((limit -= step) < 0)
            {
                limit += dest_dy;

              start:(*row_func) (d, dest_dx, s, src_dx);
                s += src_pitch;
                d += dest_pitch;
            }
            else
            {

                memcpy (d, d - dest_pitch, dest_dx_bytes);
                d += dest_pitch;
            }
        }
        while (--count);
    }
}





void
IMAGE_STRETCH2X (unsigned char *dest_ptr, int dest_dx, int dest_dy,
                 int dest_pitch, int dest_bpp, unsigned char *src_ptr,
                 int src_dx, int src_dy, int src_pitch, int src_bpp,
                 void (*row_func) (unsigned char *, int, unsigned char *,
                                   int), void (*row2x_func) (unsigned char *,
                                                             unsigned char *,
                                                             unsigned char *,
                                                             int,
                                                             unsigned char *,
                                                             int))
{

    register unsigned char *d1 = dest_ptr;
    register unsigned char *d12 = dest_ptr + dest_pitch;
    register unsigned char *d2 = dest_ptr + dest_pitch * 2;
    register unsigned char *s = src_ptr;
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = src_dy;

    if (count)
    {

        (*row_func) (d1, dest_dx, s, src_dx);
        s += src_pitch;

        while (--count)
        {

            (*row2x_func) (d1, d12, d2, dest_dx, s, src_dx);
            d1 += 2 * dest_pitch;
            d12 += 2 * dest_pitch;
            d2 += 2 * dest_pitch;
            s += src_pitch;
        }

        memcpy (d12, d1, dest_dx_bytes);
    }
}




void
IMAGE_STRETCH2XPLUS (unsigned char *dest_ptr, int dest_dx, int dest_dy,
                     int dest_pitch, int dest_bpp, unsigned char *src_ptr,
                     int src_dx, int src_dy, int src_pitch, int src_bpp,
                     void (*row_func) (unsigned char *, int, unsigned char *,
                                       int),
                     void (*row2x_func) (unsigned char *, unsigned char *,
                                         unsigned char *, int,
                                         unsigned char *, int))
{

    register unsigned char *d = dest_ptr, *d1;
    register unsigned char *s = src_ptr;
    register int dest_dx_bytes = dest_dx * dest_bpp;
    register int count = dest_dy;
    register int limit = dest_dy >> 1;
    register int step = src_dy << 1;

    register int remainder = (2 * dest_dy - limit) / step;

    if (count)
    {

        (*row_func) (d, dest_dx, s, src_dx);
        s += src_pitch;

        if (!(count -= remainder))
        {
            remainder--;
            goto end;
        }

        while (1)
        {

            goto start;
            do
            {

                memcpy (d + dest_pitch, d, dest_dx_bytes);
                d += dest_pitch;
              start:

                if (!(--count))
                    goto end;
            }
            while ((limit -= step) >= 0);
            limit += dest_dy;

            d1 = d;
            do
            {

                d += dest_pitch;

                if (!(--count))
                    goto end_2;
            }
            while ((limit -= step) >= 0);
            limit += dest_dy;

            (*row2x_func) (d1, d1 + dest_pitch, d + dest_pitch, dest_dx, s,
                           src_dx);
            d1 += dest_pitch;
            s += src_pitch;

            while (d1 != d)
            {

                memcpy (d1 + dest_pitch, d1, dest_dx_bytes);
                d1 += dest_pitch;
            }

            d += dest_pitch;
        }

      end_2:
        while (d1 != d)
        {
            memcpy (d1 + dest_pitch, d1, dest_dx_bytes);
            d1 += dest_pitch;
        }

      end:
        while (remainder--)
        {
            memcpy (d + dest_pitch, d, dest_dx_bytes);
            d += dest_pitch;
        }
    }
}


