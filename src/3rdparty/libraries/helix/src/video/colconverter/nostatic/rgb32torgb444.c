/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rgb32torgb444.c,v 1.7 2005/04/12 14:16:46 ehyche Exp $
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

#include "hlxclib/string.h"
#include "nostatic/yuv.h"
#include "nostatic/colorlib.h"
#include "nostatic/rgb.h"
#include "nostatic/rgbcommon.h"

void
RGB32toRGB444_ROW_SHRINK (unsigned char *dest_ptr,
                          int dest_dx, unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    register int limit = src_dx >> 1;
    register int step = dest_dx;
    if (count)
    {
        do 
        {
            register unsigned int da_rgb; 
            register unsigned int sa_rgb;   
            sa_rgb = *(unsigned int *) (s); 
            da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            *(unsigned short *) (d) = da_rgb;
            d += 2;
            do
            {
                s += 4;
            } while ((limit -= step) >= 0);
            limit += src_dx;
        } while (--count);
    }
}


void
RGB32toRGB444_ROW_COPY (unsigned char *dest_ptr, int dest_dx,
                        unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    while (((unsigned int) d & 3) && ((unsigned int) s & 3) && count)
    {
        register unsigned int da_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >> ((8 - (4 +4)) &0x1F)): (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) << (((4 + 4) -8) &0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((0 + 4) - 8) & 0x1F)));
        s += 4;
        *(unsigned short *) (d) = da_rgb;
        d += 2;
        count--;
    }
    while (count >= 4)
    {
        register unsigned int da_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((0)? (((((0)? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) >> ((8-(8+4))&0x1F)): (((((0)? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) <<(((8 + 4) -8) & 0x1F))) | ((0) ? (((((0)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F))& (0x100 -(1U << (8 - 8)))) : ((sa_rgb >>(((8 +8) -8) &0x1F)) &(0x100 -(1U <<(8 -8))))))& (0x100 - (1U << (8 - 4)))) >> ((8 -(4 +4)) &0x1F)): (((((0)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 - (1U << (8 - 4)))) << (((4 + 4) -8) & 0x1F)))| ((1)? (((((0)? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) >>((8 -(0 +4)) & 0x1F)) : (((((0) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F))& (0x100 -(1U << (8 - 8)))) : ((sa_rgb >>(((0 +8) -8) &0x1F)) &(0x100 -(1U <<(8 -8))))))& (0x100 - (1U << (8 - 4)))) << (((0 + 4) -8) &0x1F)));
        *(unsigned short *) (d) = da_rgb;

        sa_rgb = *(unsigned int *) (s + 4);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >> ((8 - (4 +4)) &0x1F)): (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) << (((4 + 4) -8) &0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((0 + 4) - 8) & 0x1F)));
        *(unsigned short *) (d + 2) = da_rgb;

        sa_rgb = *(unsigned int *) (s + 2 * 4);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >> ((8 - (4 +4)) &0x1F)): (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) << (((4 + 4) -8) &0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((0 + 4) - 8) & 0x1F)));
        *(unsigned short *) (d + 2 * 2) = da_rgb;

        sa_rgb = *(unsigned int *) (s + 3 * 4);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >> ((8 - (4 +4)) &0x1F)): (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) << (((4 + 4) -8) &0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((0 + 4) - 8) & 0x1F)));
        *(unsigned short *) (d + 3 * 2) = da_rgb;

        s += 4 * 4;
        d += 4 * 2;
        count -= 4;
    }

    while (count)
    {
        register unsigned int da_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >> ((8 - (4 +4)) &0x1F)): (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) << (((4 + 4) -8) &0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((0 + 4) - 8) & 0x1F)));
        s += 4;
        *(unsigned short *) (d) = da_rgb;
        d += 2;
        count--;
    }
}


void
RGB32toRGB444_ROW_STRETCH (unsigned char *dest_ptr, int dest_dx,
                           unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    register int limit = dest_dx >> 1;
    register int step = src_dx;
    if (count)
    {
        goto start;
        do
        {
            register unsigned int da_rgb;
            if ((limit -= step) < 0)
            {
                limit += dest_dx;
              start:
                register unsigned int sa_rgb;
                sa_rgb = *(unsigned int *) (s);
                da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) >> ((8 -(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) << (((8+4)-8)&0x1F)))| ((4 + 4 < 8)? (((((8 + 8 <8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 - (1U << (8 - 4)))) >> ((8 -(4 +4)) & 0x1F)): (((((8 + 8 <8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 - (1U << (8 - 4)))) << (((4 + 4) -8) & 0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100-(1U <<(8 -8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) >> ((8 -(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100-(1U <<(8 -8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) << (((0+4)-8)&0x1F)));
                s += 4;
            }
            *(unsigned short *) (d) = da_rgb;
            d += 2;
        }
        while (--count);
    }
}


void
RGB32toRGB444_ROW_STRETCH2X (unsigned char *dest_ptr, int dest_dx,
                             unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = src_dx;
    if (count)
    {
        register unsigned int da_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >>((8 -(8 +4)) & 0x1F)) : (((((16 + 8 <8) ? ((sa_rgb <<((8 -(16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (4 + 4)) & 0x1F)) : (((((8 + 8 < 8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        s += 4;
        count--;
        *(unsigned short *) (d) = da_rgb;
        d += 2;
        while (count >= 2)
        {
            register unsigned int db_rgb;
            register unsigned int sa_rgb;
            sa_rgb = *(unsigned int *) (s);
            db_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            da_rgb =(((da_rgb ^ db_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & db_rgb);
            *(unsigned short *) (d) = da_rgb;
            *(unsigned short *) (d + 2) = db_rgb;
            sa_rgb = *(unsigned int *) (s + 4);
            da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            db_rgb =(((db_rgb ^ da_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (db_rgb & da_rgb);
            *(unsigned short *) (d + 2 * 2) = db_rgb;
            *(unsigned short *) (d + 3 * 2) = da_rgb;
            s += 2 * 4;
            d += 4 * 2;
            count -= 2;
        }
        if (count)
        {
            register unsigned int db_rgb;
            register unsigned int sa_rgb;
            sa_rgb = *(unsigned int *) (s);
            db_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            da_rgb =(((da_rgb ^ db_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & db_rgb);
            *(unsigned short *) (d) = da_rgb;
            *(unsigned short *) (d + 2) = db_rgb;
            *(unsigned short *) (d + 2 * 2) = db_rgb;
        }
        else
        {
            *(unsigned short *) (d) = da_rgb;
        }
    }
}


void
RGB32toRGB444_ROW_STRETCH2XPLUS (unsigned char *dest_ptr, int dest_dx,
                                 unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d = dest_ptr;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    register int limit = dest_dx >> 1;
    register int step = src_dx << 1;
    register int remainder = (2 * dest_dx - limit) / step;
    if (count)
    {
        register unsigned int da_rgb;
        register unsigned int db_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >>((8 -(8 +4)) & 0x1F)) : (((((16 + 8 <8) ? ((sa_rgb <<((8 -(16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (4 + 4)) & 0x1F)) : (((((8 + 8 < 8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        s += 4;
        if (!(count -= remainder))
            goto end_of_row;
        while (1)
        {
            do
            {
                *(unsigned short *) (d) = da_rgb;
                d += 2;
                if (!(--count))
                    goto end_of_row;
            }
            while ((limit -= step) >= 0);
            limit += dest_dx;
            register unsigned int sa_rgb;
            sa_rgb = *(unsigned int *) (s);
            db_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            da_rgb =(((da_rgb ^ db_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & db_rgb);
            do
            {
                *(unsigned short *) (d) = da_rgb;
                d += 2;
                if (!(--count))
                    goto end_of_row;
            }
            while ((limit -= step) >= 0);
            limit += dest_dx;
            do
            {
                *(unsigned short *) (d) = db_rgb;
                d += 2;
                if (!(--count))
                    goto end_of_row_2;
            }
            while ((limit -= step) >= 0);
            limit += dest_dx;
            sa_rgb = *(unsigned int *) (s + 4);
            da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            s += 2 * 4;
            db_rgb =(((db_rgb ^ da_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (db_rgb & da_rgb);
            do
            {
                *(unsigned short *) (d) = db_rgb;
                d += 2;
                if (!(--count))
                    goto end_of_row_2;
            } while ((limit -= step) >= 0);
            limit += dest_dx;
        }
      end_of_row_2:
        da_rgb = db_rgb;
      end_of_row:
        while (remainder--)
        {
            *(unsigned short *) (d) = da_rgb;
            d += 2;
        }
    }
}





void
RGB32toRGB444_ROW2X_SHRINK (unsigned char *dest_ptr_1,
                            unsigned char *dest_ptr_12,
                            unsigned char *dest_ptr_2, int dest_dx,
                            unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d1 = dest_ptr_1;
    register unsigned char *d12 = dest_ptr_12;
    register unsigned char *d2 = dest_ptr_2;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    register int limit = src_dx >> 1;
    register int step = dest_dx;
    if (count)
    {
        do
        {
            register unsigned int da_rgb;
            register unsigned int sa_rgb;
            sa_rgb = *(unsigned int *) (s);
            da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            *(unsigned short *) (d2) = da_rgb;
            d2 += 2;
            register unsigned int sb_rgb;
            sb_rgb = *(unsigned short *) (d1);
            da_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U <<0)))) +(da_rgb & sb_rgb);
            d1 += 2;
            *(unsigned short *) (d12) = da_rgb;
            d12 += 2;
            do
            {
                s += 4;
            }
            while ((limit -= step) >= 0);
            limit += src_dx;
        }
        while (--count);
    }
}



void
RGB32toRGB444_ROW2X_COPY (unsigned char *dest_ptr_1,
                          unsigned char *dest_ptr_12,
                          unsigned char *dest_ptr_2, int dest_dx,
                          unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d1 = dest_ptr_1;
    register unsigned char *d12 = dest_ptr_12;
    register unsigned char *d2 = dest_ptr_2;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    while (((unsigned int) d2 & 3) && ((unsigned int) s & 3) && count)
    {
        register unsigned int da_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F))& (0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8 +8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        s += 4;
        *(unsigned short *) (d2) = da_rgb;
        d2 += 2;
        register unsigned int sb_rgb;
        sb_rgb = *(unsigned short *) (d1);
        da_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
        d1 += 2;
        *(unsigned short *) (d12) = da_rgb;
        d12 += 2;
        count--;
    }
    while (count >= 4)
    {
        register unsigned int da_rgb;
        register unsigned int sa_rgb;
        register unsigned int sb_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F))& (0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8 +8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        *(unsigned short *) (d2) = da_rgb;
        sb_rgb = *(unsigned short *) (d1);
        da_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
        *(unsigned short *) (d12) = da_rgb;
        sa_rgb = *(unsigned int *) (s + 4);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F))& (0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8 +8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        *(unsigned short *) (d2 + 2) = da_rgb;
        sb_rgb = *(unsigned short *) (d1 + 2);
        da_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
        *(unsigned short *) (d12 + 2) = da_rgb;
        sa_rgb = *(unsigned int *) (s + 2 * 4);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F))& (0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8 +8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        *(unsigned short *) (d2 + 2 * 2) = da_rgb;
        sb_rgb = *(unsigned short *) (d1 + 2 * 2);
        da_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
        *(unsigned short *) (d12 + 2 * 2) = da_rgb;
        sa_rgb = *(unsigned int *) (s + 3 * 4);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F))& (0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8 +8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        *(unsigned short *) (d2 + 3 * 2) = da_rgb;
        sb_rgb = *(unsigned short *) (d1 + 3 * 2);
        da_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
        *(unsigned short *) (d12 + 3 * 2) = da_rgb;
        s += 4 * 4;
        d1 += 4 * 2;
        d2 += 4 * 2;
        d12 += 4 * 2;
        count -= 4;
    }
    while (count)
    {
        register unsigned int da_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F))& (0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8 +8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        s += 4;
        *(unsigned short *) (d2) = da_rgb;
        d2 += 2;
        register unsigned int sb_rgb;
        sb_rgb = *(unsigned short *) (d1);
        da_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
        d1 += 2;
        *(unsigned short *) (d12) = da_rgb;
        d12 += 2;
        count--;
    }
}



void
RGB32toRGB444_ROW2X_STRETCH (unsigned char *dest_ptr_1,
                             unsigned char *dest_ptr_12,
                             unsigned char *dest_ptr_2, int dest_dx,
                             unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d1 = dest_ptr_1;
    register unsigned char *d12 = dest_ptr_12;
    register unsigned char *d2 = dest_ptr_2;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    register int limit = dest_dx >> 1;
    register int step = src_dx;
    if (count)
    {
        goto start;
        do
        {
            register unsigned int da_rgb;
            register unsigned int dc_rgb;
            if ((limit -= step) < 0)
            {
                limit += dest_dx;
              start:if (5 == 0)
              {
                  da_rgb = *(unsigned short *) (s);
              }
              else
              {
                  register unsigned int sa_rgb;
                  sa_rgb = *(unsigned int *) (s);
                  da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) >> ((8 -(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) << (((8+4)-8)&0x1F)))| ((4 + 4 < 8)? (((((8 + 8 <8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 - (1U << (8 - 4)))) >> ((8 -(4 +4)) & 0x1F)): (((((8 + 8 <8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) &(0x100 -(1U << (8 - 8)))) : ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U << (8 - 8))))))& (0x100 - (1U << (8 - 4)))) << (((4 + 4) -8) & 0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100-(1U <<(8 -8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) >> ((8 -(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100-(1U <<(8 -8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) << (((0+4)-8)&0x1F)));
              }
                s += 4;
                register unsigned int sb_rgb;
                sb_rgb = *(unsigned short *) (d1);
                dc_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 +4 -1)) -(1U <<0))))+ (da_rgb & sb_rgb);
            }
            d1 += 2;
            *(unsigned short *) (d2) = da_rgb;
            d2 += 2;
            *(unsigned short *) (d12) = dc_rgb;
            d12 += 2;
        }
        while (--count);
    }
}


void
RGB32toRGB444_ROW2X_STRETCH2X (unsigned char *dest_ptr_1,
                               unsigned char *dest_ptr_12,
                               unsigned char *dest_ptr_2, int dest_dx,
                               unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d1 = dest_ptr_1;
    register unsigned char *d12 = dest_ptr_12;
    register unsigned char *d2 = dest_ptr_2;
    register unsigned char *s = src_ptr;
    register int count = src_dx;
    if (count)
    {
        register unsigned int da_rgb;
        register unsigned int dc_rgb;
        register unsigned int sa_rgb;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >>((8 -(8 +4)) & 0x1F)) : (((((16 + 8 <8) ? ((sa_rgb <<((8 -(16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (4 + 4)) & 0x1F)) : (((((8 + 8 < 8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
        s += 4;
        register unsigned int sb_rgb;
        sb_rgb = *(unsigned short *) (d1);
        dc_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 - 1)) -(1U << 0)))) +(da_rgb & sb_rgb);
        d1 += 2 * 2;
        count--;
        *(unsigned short *) (d2) = da_rgb;
        d2 += 2;
        *(unsigned short *) (d12) = dc_rgb;
        d12 += 2;
        while (count >= 2)
        {
            register unsigned int db_rgb;
            register unsigned int dd_rgb;
            register unsigned int sa_rgb;
            sa_rgb = *(unsigned int *) (s);
            db_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            da_rgb =(((da_rgb ^ db_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & db_rgb);
            *(unsigned short *) (d2) = da_rgb;
            *(unsigned short *) (d2 + 2) = db_rgb;
            register unsigned int sb_rgb;
            sb_rgb = *(unsigned short *) (d1);
            dd_rgb =(((db_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U <<0)))) +(db_rgb & sb_rgb);
            dc_rgb =(((dc_rgb ^ dd_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (dc_rgb & dd_rgb);
            *(unsigned short *) (d12) = dc_rgb;
            *(unsigned short *) (d12 + 2) = dd_rgb;
            sa_rgb = *(unsigned int *) (s + 4);
            da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            db_rgb =(((db_rgb ^ da_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (db_rgb & da_rgb);
            *(unsigned short *) (d2 + 2 * 2) = db_rgb;
            *(unsigned short *) (d2 + 3 * 2) = da_rgb;
            sb_rgb = *(unsigned short *) (d1 + 2 * 2);
            dc_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U <<0)))) +(da_rgb & sb_rgb);
            dd_rgb =(((dd_rgb ^ dc_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (dd_rgb & dc_rgb);
            *(unsigned short *) (d12 + 2 * 2) = dd_rgb;
            *(unsigned short *) (d12 + 3 * 2) = dc_rgb;
            s += 2 * 4;
            d1 += 4 * 2;
            d2 += 4 * 2;
            d12 += 4 * 2;
            count -= 2;
        }
        if (count)
        {
            register unsigned int db_rgb;
            register unsigned int dd_rgb;
            register unsigned int sa_rgb;
            sa_rgb = *(unsigned int *) (s);
            db_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))>> ((8 - (8 + 4)) & 0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4))))<< (((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) &0x1F)) &(0x100 -(1U <<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) &(0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >>((8 -(4 +4)) & 0x1F)) : (((((8+8<8)?((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            da_rgb =(((da_rgb ^ db_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & db_rgb);
            *(unsigned short *) (d2) = da_rgb;
            *(unsigned short *) (d2 + 2) = db_rgb;
            *(unsigned short *) (d2 + 2 * 2) = db_rgb;
            register unsigned int sb_rgb;
            sb_rgb = *(unsigned short *) (d1);
            dd_rgb =(((db_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U <<0)))) +(db_rgb & sb_rgb);
            dc_rgb =(((dc_rgb ^ dd_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (dc_rgb & dd_rgb);
            *(unsigned short *) (d12) = dc_rgb;
            *(unsigned short *) (d12 + 2) = dd_rgb;
            *(unsigned short *) (d12 + 2 * 2) = dd_rgb;
        }
        else
        {
            *(unsigned short *) (d2) = da_rgb;
            *(unsigned short *) (d12) = dc_rgb;
        }
    }
}



void
RGB32toRGB444_ROW2X_STRETCH2XPLUS (unsigned char *dest_ptr_1,
                                   unsigned char *dest_ptr_12,
                                   unsigned char *dest_ptr_2,
                                   int dest_dx,
                                   unsigned char *src_ptr, int src_dx)
{
    register unsigned char *d1 = dest_ptr_1;
    register unsigned char *d12 = dest_ptr_12;
    register unsigned char *d2 = dest_ptr_2;
    register unsigned char *s = src_ptr;
    register int count = dest_dx;
    register int limit = dest_dx >> 1;
    register int step = src_dx << 1;
    register int remainder = (2 * dest_dx - limit) / step;
    if (count)
    {
        register unsigned int da_rgb = 0;
        register unsigned int db_rgb = 0;
        register unsigned int dc_rgb = 0;
        register unsigned int dd_rgb = 0;
        register unsigned int sa_rgb = 0;
        sa_rgb = *(unsigned int *) (s);
        da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(8+4))&0x1F)): (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) >> ((8 - (4 +4)) &0x1F)): (((((8 + 8 <8) ? ((sa_rgb <<((8 -(8 +8)) & 0x1F)) & (0x100-(1U<<(8 -8)))): ((sa_rgb >>(((8 + 8) -8) & 0x1F)) & (0x100 -(1U <<(8 -8)))))) &(0x100 -(1U << (8 - 4)))) << (((4 + 4) -8) &0x1F))) |((0 + 4 <8)? (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >> ((8-(0+4))&0x1F)): (((((0 + 8 <8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 -(1U << (8 - 8)))): ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) <<(((0 + 4) - 8) & 0x1F)));
        s += 4;
        if (!(count -= remainder))
            goto end_of_row;
        while (1)
        {
            register unsigned int sb_rgb;
            sb_rgb = *(unsigned short *) (d1);
            dc_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
            do
            {
                d1 += 2;
                *(unsigned short *) (d2) = da_rgb;
                d2 += 2;
                *(unsigned short *) (d12) = dc_rgb;
                d12 += 2;
                if (!(--count))
                    goto end_of_row;

            }
            while ((limit -= step) >= 0);
            limit += dest_dx;
            register unsigned int sa_rgb;
            sa_rgb = *(unsigned int *) (s);
            db_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >>((8 -(8 +4)) & 0x1F)) : (((((16 + 8 <8) ? ((sa_rgb <<((8 -(16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (4 + 4)) & 0x1F)) : (((((8 + 8 < 8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            da_rgb =(((da_rgb ^ db_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & db_rgb);
            sb_rgb = *(unsigned short *) (d1);
            dc_rgb =(((da_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (da_rgb & sb_rgb);
            do
            {
                d1 += 2;
                *(unsigned short *) (d2) = da_rgb;
                d2 += 2;
                *(unsigned short *) (d12) = dc_rgb;
                d12 += 2;
                if (!(--count))
                    goto end_of_row;
            }
            while ((limit -= step) >= 0);
            limit += dest_dx;
            sb_rgb = *(unsigned short *) (d1);
            dd_rgb =(((db_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (db_rgb & sb_rgb);
            do
            {
                d1 += 2;
                *(unsigned short *) (d2) = db_rgb;
                d2 += 2;
                *(unsigned short *) (d12) = dd_rgb;
                d12 += 2;
                if (!(--count))
                    goto end_of_row_2;
            }
            while ((limit -= step) >= 0);
            limit += dest_dx;
            sa_rgb = *(unsigned int *) (s + 4);
            da_rgb =((8 + 4 <8)? (((((16 + 8 <8) ? ((sa_rgb << ((8 - (16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 -4)))) >>((8 -(8 +4)) & 0x1F)) : (((((16 + 8 <8) ? ((sa_rgb <<((8 -(16 + 8)) & 0x1F)) & (0x100 -(1U <<(8 -8)))): ((sa_rgb >> (((16 + 8) - 8) & 0x1F)) &(0x100 -(1U << (8 - 8)))))) & (0x100 -(1U <<(8 -4)))) <<(((8 + 4) - 8) & 0x1F))) | ((4 + 4 <8)? (((((8 + 8 <8)? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (4 + 4)) & 0x1F)) : (((((8 + 8 < 8) ? ((sa_rgb << ((8 - (8 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((8 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((4 + 4) - 8) & 0x1F))) | ((0 + 4 < 8) ? (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) >> ((8 - (0 + 4)) & 0x1F)) : (((((0 + 8 < 8) ? ((sa_rgb << ((8 - (0 + 8)) & 0x1F)) & (0x100 - (1U << (8 - 8)))) : ((sa_rgb >> (((0 + 8) - 8) & 0x1F)) & (0x100 - (1U << (8 - 8)))))) & (0x100 - (1U << (8 - 4)))) << (((0 + 4) - 8) & 0x1F)));
            s += 2 * 4;
            db_rgb =(((db_rgb ^ da_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (db_rgb & da_rgb);
            sb_rgb = *(unsigned short *) (d1);
            dd_rgb =(((db_rgb ^ sb_rgb) >> 1) &(((1U << (8 + 4 - 1)) - (1U << 8)) | ((1U << (4 + 4 - 1)) -(1U << 4)) | ((1U <<(0 + 4 -1)) -(1U << 0))))+ (db_rgb & sb_rgb);
            do
            {
                d1 += 2;
                *(unsigned short *) (d2) = db_rgb;
                d2 += 2;
                *(unsigned short *) (d12) = dd_rgb;
                d12 += 2;
                if (!(--count))
                    goto end_of_row_2;
            }
            while ((limit -= step) >= 0);
            limit += dest_dx;
        }
      end_of_row_2:
        da_rgb = db_rgb;
        dc_rgb = dd_rgb;
      end_of_row:
        while (remainder--)
        {
            *(unsigned short *) (d2) = da_rgb;
            d2 += 2;
            *(unsigned short *) (d12) = dc_rgb;
            d12 += 2;
        }
    }
}

int
RGBtoRGB (int dest_format, unsigned char *dest_ptr, int dest_width,
          int dest_height, int dest_pitch, int dest_x, int dest_y,
          int dest_dx, int dest_dy, int src_format, unsigned char *src_ptr,
          int src_width, int src_height, int src_pitch, int src_x, int src_y,
          int src_dx, int src_dy)
{
    
    void (*ImageFuncs[5]) (unsigned char *dest_ptr, int dest_dx, int dest_dy,
                           int dest_pitch, int dest_bpp, unsigned char *src_ptr,
                           int src_dx, int src_dy, int src_pitch, int src_bpp,
                           void (*row_func) (unsigned char *, int,
                                             unsigned char *, int),
                           void (*row2x_func) (unsigned char *, unsigned char *,
                                               unsigned char *, int,
                                               unsigned char *, int)) =
        {
            IMAGE_SHRINK,
            IMAGE_COPY,
            IMAGE_STRETCH,
            IMAGE_STRETCH2X,
            IMAGE_STRETCH2XPLUS
        };
    
    void (*RowFuncs[5]) (unsigned char *dest_ptr, int dest_dx,
                         unsigned char *src_ptr, int src_dx) =
        {
            RGB32toRGB444_ROW_SHRINK,
            RGB32toRGB444_ROW_COPY,
            RGB32toRGB444_ROW_STRETCH,
            RGB32toRGB444_ROW_STRETCH2X,
            RGB32toRGB444_ROW_STRETCH2XPLUS
        };


    void (*Row2xFuncs[5]) (unsigned char *dest_ptr_1, unsigned char *dest_ptr_12,
                           unsigned char *dest_ptr_2, int dest_dx,
                           unsigned char *src_ptr, int src_dx) =
        {
            RGB32toRGB444_ROW2X_SHRINK,
            RGB32toRGB444_ROW2X_COPY,
            RGB32toRGB444_ROW2X_STRETCH,
            RGB32toRGB444_ROW2X_STRETCH2X,
            RGB32toRGB444_ROW2X_STRETCH2XPLUS
        };


    void (*row_proc) (unsigned char *, int, unsigned char *, int);
    void (*row2x_proc) (unsigned char *, unsigned char *, unsigned char *, int,
                        unsigned char *, int);
    void (*image_proc) (unsigned char *, int, int, int, int,
                        unsigned char *, int, int, int, int,
                        void (*)(unsigned char *, int, unsigned char *, int),
                        void (*)(unsigned char *, unsigned char *,
                                 unsigned char *, int, unsigned char *, int));

    register int scale_x, scale_y;


    register int dest_bpp, src_bpp;
    register unsigned char *d, *s;


    if (((unsigned) dest_ptr & 3) || (dest_pitch & 1) ||
        ((unsigned) src_ptr & 3) || (src_pitch & 3) ||
        dest_width <= 0 || dest_height <= 0 ||
        src_width <= 0 || src_height <= 0 ||
        dest_x < 0 || dest_y < 0 || dest_dx <= 0 || dest_dy <= 0 ||
        src_x < 0 || src_y < 0 || src_dx <= 0 || src_dy <= 0 ||
        dest_width < dest_x + dest_dx || dest_height < dest_y + dest_dy ||
        src_width < src_x + src_dx || src_height < src_y + src_dy)
    {

        return -1;
    }


    scale_x = 1;
    if (dest_dx != src_dx)
    {
        if (dest_dx < src_dx)
            scale_x--;
        else
        {
            scale_x++;
            if (dest_dx >= 2 * src_dx)
            {
                scale_x++;
                if (dest_dx > 2 * src_dx)
                    scale_x++;
            }
        }
    }

    scale_y = 1;
    if (dest_dy != src_dy)
    {
        if (dest_dy < src_dy)
            scale_y--;
        else
        {
            scale_y++;
            if (dest_dy >= 2 * src_dy)
            {
                scale_y++;
                if (dest_dy > 2 * src_dy)
                    scale_y++;
            }
        }
    }


    row_proc   = RowFuncs[scale_x];
    row2x_proc = Row2xFuncs[scale_x];
    image_proc = ImageFuncs[scale_y];

    if (!row_proc || !row2x_proc || !image_proc)
    {
        return -1;
    }

    dest_bpp = 2;
    src_bpp = 4;

    if (dest_pitch < 0)
        dest_ptr -= (dest_height - 1) * dest_pitch;
    if (src_pitch < 0)
        src_ptr -= (src_height - 1) * src_pitch;


    d = dest_ptr + dest_x * dest_bpp + dest_y * dest_pitch;
    s = src_ptr + src_x * src_bpp + src_y * src_pitch;


    (*image_proc) (d, dest_dx, dest_dy, dest_pitch, dest_bpp,
                   s, src_dx, src_dy, src_pitch, src_bpp, row_proc, row2x_proc);


    return 0;
}





int
RGB32toRGB444 (unsigned char *dest_ptr,
               int dest_width, int dest_height, int dest_pitch,
               int dest_x, int dest_y, int dest_dx, int dest_dy,
               unsigned char *src_ptr,
               int src_width, int src_height, int src_pitch,
               int src_x, int src_y, int src_dx, int src_dy,
               color_data_t * not_used)
{
    const int dest_format = 18;	/* RGB444 */
    const int src_format = 5;	/* RGB32 */
    return RGBtoRGB (dest_format, dest_ptr, dest_width, dest_height, dest_pitch,
                     dest_x, dest_y, dest_dx, dest_dy, src_format,
                     src_ptr, src_width, src_height, src_pitch,
                     src_x, src_y, src_dx, src_dy);
}
