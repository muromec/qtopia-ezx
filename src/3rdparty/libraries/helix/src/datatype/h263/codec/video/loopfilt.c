/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: loopfilt.c,v 1.2 2004/07/09 18:32:15 hubbe Exp $
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

//#include <stdio.h>
//#include <stdlib.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "loopfilt.h"

extern void loopfilt8( PIXEL input[], int xdim, PIXEL output[] )
{
    union {
        U16     half[8][8]; // 16b take longer than 32b writes on 486/Pentium
        U32     word[8][4];
    } temp;     // Area to hold data after filtering in horizontal dimension
    union {
        PIXEL   * pix;
        U32     * word;
    } out_ptr;  // Access output as bytes or as words
    int row, col, a[7], out;
    
    // Perform horizontal filtering
    for (row = 0; row < 8; row++) {
        a[0] = input[0] + input[1];
        a[1] = input[1] + input[2];
        a[2] = input[2] + input[3];
        a[3] = input[3] + input[4];
#ifdef LITTLE_ENDIAN    // Avoid 16 b. Stores on X86
        temp.word[row][0] = (input[0] << 2) + ((a[1] + a[2]) << 16);    // temp[0], temp[2]
        temp.word[row][2] = a[0] + a[1]  +  ((a[2] + a[3]) << 16);      // temp[1], temp[3]
#elif defined BIG_ENDIAN
        temp.half[row][0] = input[0] << 2;  // temp[0]
        temp.half[row][1] = a[1] + a[2];    // temp[2]
        temp.half[row][4] = a[0] + a[1];    // temp[1]
        temp.half[row][5] = a[2] + a[3];    // temp[3]
#else
#   error
#endif
        a[4] = input[4] + input[5];
        a[5] = input[5] + input[6];
        a[6] = input[6] + input[7];
#ifdef LITTLE_ENDIAN
        temp.word[row][1] = a[3] + a[4]  +  ((a[5] + a[6]) << 16);  // temp[4], temp[6]
        temp.word[row][3] = a[4] + a[5]  +  (input[7] << (16 + 2)); // temp[5], temp[7]
#elif defined BIG_ENDIAN
        temp.half[row][2] = a[3] + a[4];    // temp[4]
        temp.half[row][3] = a[5] + a[6];    // temp[6]
        temp.half[row][6] = a[4] + a[5];    // temp[5]
        temp.half[row][7] = input[7] << 2;  // temp[7]
#else
#   error
#endif
        input += xdim;
    }
    
    // Perform vertical filtering on two columns at a time
    for (col = 0; col < 2; col++) {
        
        // col=0: Filter cols 0 and 2; col=1: Filter cols 4 and 6
        a[0] = temp.word[0][col] + temp.word[1][col];
        a[1] = temp.word[1][col] + temp.word[2][col] + 0x80008L;   // Round;
        a[2] = temp.word[2][col] + temp.word[3][col];
        a[3] = temp.word[3][col] + temp.word[4][col] + 0x80008L;   // Round;
        a[4] = temp.word[4][col] + temp.word[5][col];
        a[5] = temp.word[5][col] + temp.word[6][col] + 0x80008L;   // Round;
        a[6] = temp.word[6][col] + temp.word[7][col];
        out_ptr.pix = output + (col << 2);
#ifdef LITTLE_ENDIAN
        *(out_ptr.word) = (temp.word[0][col] + 0x20002L) >> 2; // Round
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[0] + a[1]) >> 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[1] + a[2]) >> 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[2] + a[3]) >> 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[3] + a[4]) >> 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[4] + a[5]) >> 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[5] + a[6]) >> 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (temp.word[7][col] + 0x20002L) >> 2; // Round
#elif defined BIG_ENDIAN
        *(out_ptr.word) = (temp.word[0][col] + 0x20002L) << 6; // Round
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[0] + a[1]) << 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[1] + a[2]) << 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[2] + a[3]) << 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[3] + a[4]) << 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[4] + a[5]) << 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (a[5] + a[6]) << 4;
        out_ptr.pix += xdim;
        *(out_ptr.word) = (temp.word[7][col] + 0x20002L) << 6; // Round
#else
#   error
#endif
        
        // col=0: Filter cols 1 and 3; col=1: Filter cols 5 and 7
        a[0] = temp.word[0][col+2] + temp.word[1][col+2];
        a[1] = temp.word[1][col+2] + temp.word[2][col+2] + 0x80008L;   // Round;
        a[2] = temp.word[2][col+2] + temp.word[3][col+2];
        a[3] = temp.word[3][col+2] + temp.word[4][col+2] + 0x80008L;   // Round;
        a[4] = temp.word[4][col+2] + temp.word[5][col+2];
        a[5] = temp.word[5][col+2] + temp.word[6][col+2] + 0x80008L;   // Round;
        a[6] = temp.word[6][col+2] + temp.word[7][col+2];
        out_ptr.pix = output + (col << 2) + 1;
        out = (temp.word[0][col+2] + 0x20002L) >> 2;    // Round
#ifdef LITTLE_ENDIAN       
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[0] + a[1]) >> 4;
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[1] + a[2]) >> 4;
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[2] + a[3]) >> 4;
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[3] + a[4]) >> 4;
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[4] + a[5]) >> 4;
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[5] + a[6]) >> 4;
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (temp.word[7][col+2] + 0x20002L) >> 2; // Round
        *(out_ptr.pix) = out;
        *(out_ptr.pix + 2) = out >> 16;
#elif defined BIG_ENDIAN
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[0] + a[1]) >> 4;
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[1] + a[2]) >> 4;
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[2] + a[3]) >> 4;
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[3] + a[4]) >> 4;
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[4] + a[5]) >> 4;
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (a[5] + a[6]) >> 4;
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
        
        out_ptr.pix += xdim;
        out = (temp.word[7][col+2] + 0x20002L) >> 2; // Round
        *(out_ptr.pix + 2) = out;
        *(out_ptr.pix) = out >> 16;
#else
#   error
#endif
    }
    return;
}


//  LoopFilter - assumes same line offset for input and output
extern int  LoopFilter( MACROBLOCK_DESCR *mb, PICTURE *prev_pic, PICTURE *pic )
{
    int     row, col, status, cx, cy, pic_offset, prev_offset;
    PIXEL   * source, * dest;

    status = OK;
    // Wrap motion vectors to allowed range
    while (mb->mv_x < MV_MIN) {
        mb->mv_x += MV_WRAP;
    }
    while (mb->mv_x > MV_MAX) {
        mb->mv_x -= MV_WRAP;
    }
    while (mb->mv_y < MV_MIN) {
        mb->mv_y += MV_WRAP;
    }
    while (mb->mv_y > MV_MAX) {
        mb->mv_y -= MV_WRAP;
    }
    // Compute pointers
    col = 16 * mb->x;
    row = 16 * mb->y;
    if (col == 0  &&  mb->mv_x < 0) {    // Pointing left of first col?
        mb->mv_x = 0, status = H261_ERROR;
    }
    if (col == pic->y.nhor - 16  &&  mb->mv_x > 0) {  // Right of last col?
        mb->mv_x = 0, status = H261_ERROR;
    }
    if (row == 0  &&  mb->mv_y < 0) {    // Pointing above first row?
        mb->mv_y = 0, status = H261_ERROR;
    }
    if (row == pic->y.nvert - 16  &&  mb->mv_y > 0) {  // Below last row?
        mb->mv_y = 0, status = H261_ERROR;
    }
    dest = pic->y.ptr + col + row * pic->y.hoffset;
    source = prev_pic->y.ptr + col + mb->mv_x
                    + (row + mb->mv_y) * pic->y.hoffset;
    // Filter luminance
    loopfilt8( source, (int)pic->y.hoffset, dest );
    loopfilt8( source + 8, (int)pic->y.hoffset, dest + 8);
    source += pic->y.hoffset << 3;  // Advance 8 lines
    dest += pic->y.hoffset << 3;
    loopfilt8( source, (int)pic->y.hoffset, dest );
    loopfilt8( source + 8, (int)pic->y.hoffset, dest + 8);
    // Filter chrominance
    if (pic->color) {
        col = 8 * mb->x;
        row = 8 * mb->y;
        // Truncate motion vectors for chroma towards zero
        if (mb->mv_x < 0) {
            cx = (mb->mv_x + 1) >> 1;
        } else {
            cx = mb->mv_x >> 1;
        }
        if (mb->mv_y < 0) {
            cy = (mb->mv_y + 1) >> 1;
        } else {
            cy = mb->mv_y >> 1;
        }
        // Assuming same offset for Cr and Cb
        pic_offset = col + row * pic->cb.hoffset;
        prev_offset = col + cx + (row + cy) * pic->cb.hoffset;
        dest = pic->cb.ptr + pic_offset;
        source = prev_pic->cb.ptr + prev_offset;
        loopfilt8( source, (int)pic->cb.hoffset, dest );
        dest = pic->cr.ptr + pic_offset;
        source = prev_pic->cr.ptr + prev_offset;
        loopfilt8( source, (int)pic->cr.hoffset, dest );
    }
    return (status);
}
