/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: setpal.c,v 1.5 2007/07/06 20:53:51 jfinnecy Exp $
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

#include "hlxclib/stdlib.h"
#include "env.h"    /* standard libraries & #defines */
#define  PAL_MAIN   /* declare all palette-specific data here */
#include "rgb.h"    /* basic RGB-data definitions & macros */
#include "colorlib.h" /* ensure that prototypes get extern C'ed */

#if defined _FAT_HXCOLOR || defined HELIX_FEATURE_CC_RGB8out

/* speeds-up calculation of the distance between pixels: */
#define SQ  (sq + 256)
static unsigned int sq [256 + 256 -1];

/*
 * Suggest palettes to use for 8-bit RGB formats.
 * Use:
 *  int SuggestSrcRGB8Palette (int nColors, unsigned int *lpRGBVals);
 *  int SuggestDestRGB8Palette (int nColors, unsigned int *lpRGBVals);
 * Input:
 *  nColors - the number of existing colors in palette
 *  lpRGBVals - RGB values for each palette entry
 *  lpIndices - pixel values (palette indices)
 * Returns:
 *  >0, the total number of colors to be used, if success
 *  -1, if error
 */
static int SuggestPalette (int nColors, unsigned int *lpRGBVals)
{
    unsigned int rgb;
    int r, g, b, i, j, delta;

    /* determine the quanization step size: */
    if      (256 - nColors >= 6*6*6)    delta = 0x33;   /* 6x6x6 = 216 */
    else if (256 - nColors >= 5*5*5)    delta = 0x40;   /* 5x5x5 = 125 */
    else if (256 - nColors >= 4*4*4)    delta = 0x55;   /* 4x4x4 = 64  */
    else return nColors;

    /* complement the set of palette entries with ours: */
    i = nColors;
    for (r = 0; r <= 256; r += delta) { if (r == 256) r = 255;
    for (g = 0; g <= 256; g += delta) { if (g == 256) g = 255;
    for (b = 0; b <= 256; b += delta) { if (b == 256) b = 255;

        /* form palette entry: */
        rgb = (b << 16) | (g << 8) | r;

        /* check if we had this color before: */
        for (j = 0; j < nColors; j ++)
            if ((lpRGBVals[j] & 0x00FFFFFF) == rgb)
                goto skip;

        /* add color: */
        lpRGBVals[i] = rgb;
        i ++;

skip:   ;
    }}}

    /* return total # of entries used: */
    return i;
}

/* public functions: */
int SuggestSrcRGB8Palette (int nColors, unsigned int *lpRGBVals)
{
    return SuggestPalette (nColors, lpRGBVals);
}

int SuggestDestRGB8Palette (int nColors, unsigned int *lpRGBVals)
{
    return SuggestPalette (nColors, lpRGBVals);
}

/*
 * Set palettes to use for 8-bit RGB formats.
 * Use:
 *  int SetSrcRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices);
 *  int SetDestRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices);
 * Input:
 *  nColors - the number of colors in palette
 *  lpRGBVals - RGB values for each palette entry
 *  lpIndices - pixel values (palette indices)
 * Returns:
 *  >0, the total number of colors set, if success
 *  -1, if error
 */
int SetSrcRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices)
{
    register int i, j, k;

	/* mark all entries as empty: */
    memset(palette, 0xFF, sizeof(palette));

    /* scan palette entries: */
    for (j = 0; j < nColors; j ++) {

        /* check if index is valid: */
        i = lpIndices[j];
        if (i & ~0xFF)          /* a fast version of (i < 0 || i > 255) */
            break;

		/* get pixel value: */
		k = lpRGBVals[j] & 0x00FFFFFF;		/* remove palette flags !!! */

		/* check if palette entry is available: */
    	if (palette[i] != 0xFFFFFFFF && palette[i] != (unsigned int)k)
			continue; //break;

        /* set palette entry: */
        palette[i] = k;
    }

    /* return # of entries copied or error: */
    return (j == nColors)? j: -1;
}

int SetDestRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices)
{
    register unsigned int r, g, b, delta = 1U << 4;

    /* perform the minimum energy mismatch mapping: */
    for (r = 0; r < 256; r += delta) {
    for (g = 0; g < 256; g += delta) {
    for (b = 0; b < 256; b += delta) {

        /* initialize search: */
        int idx, match_idx = 0;
        unsigned int j, diff, match_diff;
        j = lpRGBVals[0];
        match_diff =
            SQ[(int)(r - ((j >> 0)  & 0xFF))] +
            SQ[(int)(g - ((j >> 8)  & 0xFF))] +
            SQ[(int)(b - ((j >> 16) & 0xFF))];

        /* find minimum: */
        for (idx = 1; idx < nColors; idx ++) {
            j = lpRGBVals[idx];
            diff =
                SQ[(int)(r - ((j >> 0)  & 0xFF))] +
                SQ[(int)(g - ((j >> 8)  & 0xFF))] +
                SQ[(int)(b - ((j >> 16) & 0xFF))];

            if (diff < match_diff) {
                match_idx = idx;
                match_diff = diff;
            }
        }

        /* store the result: */
        pmap[(b >> 4) | g | (r << 4)] = lpIndices[match_idx];
    }}}

    return nColors;
}

/* default palette to use: */
static unsigned int default_palette [256] = {
    0x000000, 0x000080, 0x008000, 0x008080, 0x800000, 0x800080, 0x808000, 0xC0C0C0,
    0xC0DCC0, 0xF0CAA6, 0x040404, 0x080808, 0x0C0C0C, 0x111111, 0x161616, 0x1C1C1C,
    0x222222, 0x292929, 0x555555, 0x4D4D4D, 0x424242, 0x393939, 0x818181, 0x000081,
    0x008100, 0x008181, 0x810000, 0x810081, 0x818100, 0x000033, 0x000066, 0x000099,
    0x0000CC, 0x003300, 0x003333, 0x003366, 0x003399, 0x0033CC, 0x0033FF, 0x006600,
    0x006633, 0x006666, 0x006699, 0x0066CC, 0x0066FF, 0x009900, 0x009933, 0x009966,
    0x009999, 0x0099CC, 0x0099FF, 0x00CC00, 0x00CC33, 0x00CC66, 0x00CC99, 0x00CCCC,
    0x00CCFF, 0x00FF66, 0x00FF99, 0x00FFCC, 0x330000, 0x330033, 0x330066, 0x330099,
    0x3300CC, 0x3300FF, 0x333300, 0x333333, 0x333366, 0x333399, 0x3333CC, 0x3333FF,
    0x336600, 0x336633, 0x336666, 0x336699, 0x3366CC, 0x3366FF, 0x339900, 0x339933,
    0x339966, 0x339999, 0x3399CC, 0x3399FF, 0x33CC00, 0x33CC33, 0x33CC66, 0x33CC99,
    0x33CCCC, 0x33CCFF, 0x33FF33, 0x33FF66, 0x33FF99, 0x33FFCC, 0x33FFFF, 0x660000,
    0x660033, 0x660066, 0x660099, 0x6600CC, 0x6600FF, 0x663300, 0x663333, 0x663366,
    0x663399, 0x6633CC, 0x6633FF, 0x666600, 0x666633, 0x666666, 0x666699, 0x6666CC,
    0x669900, 0x669933, 0x669966, 0x669999, 0x6699CC, 0x6699FF, 0x66CC00, 0x66CC33,
    0x66CC99, 0x66CCCC, 0x66CCFF, 0x66FF00, 0x66FF33, 0x66FF99, 0x66FFCC, 0xCC00FF,
    0xFF00CC, 0x999900, 0x993399, 0x990099, 0x9900CC, 0x990000, 0x993333, 0x990066,
    0x9933CC, 0x9900FF, 0x996600, 0x996633, 0x993366, 0x996699, 0x9966CC, 0x9933FF,
    0x999933, 0x999966, 0x999999, 0x9999CC, 0x9999FF, 0x99CC00, 0x99CC33, 0x66CC66,
    0x99CC99, 0x99CCCC, 0x99CCFF, 0x99FF00, 0x99FF33, 0x99CC66, 0x99FF99, 0x99FFCC,
    0x99FFFF, 0xCC0000, 0x990033, 0xCC0066, 0xCC0099, 0xCC00CC, 0x993300, 0xCC3333,
    0xCC3366, 0xCC3399, 0xCC33CC, 0xCC33FF, 0xCC6600, 0xCC6633, 0x996666, 0xCC6699,
    0xCC66CC, 0x9966FF, 0xCC9900, 0xCC9933, 0xCC9966, 0xCC9999, 0xCC99CC, 0xCC99FF,
    0xCCCC00, 0xCCCC33, 0xCCCC66, 0xCCCC99, 0xCCCCCC, 0xCCCCFF, 0xCCFF00, 0xCCFF33,
    0x99FF66, 0xCCFF99, 0xCCFFCC, 0xCCFFFF, 0xCC0033, 0xFF0066, 0xFF0099, 0xCC3300,
    0xFF3333, 0xFF3366, 0xFF3399, 0xFF33CC, 0xFF33FF, 0xFF6600, 0xFF6633, 0xCC6666,
    0xFF6699, 0xFF66CC, 0xCC66FF, 0xFF9900, 0xFF9933, 0xFF9966, 0xFF9999, 0xFF99CC,
    0xFF99FF, 0xFFCC00, 0xFFCC33, 0xFFCC66, 0xFFCC99, 0xFFCCCC, 0xFFCCFF, 0xFFFF33,
    0xCCFF66, 0xFFFF99, 0xFFFFCC, 0x6666FF, 0x66FF66, 0x66FFFF, 0xFF6666, 0xFF66FF,
    0xFFFF66, 0xC1C1C1, 0x5F5F5F, 0x777777, 0x868686, 0x969696, 0xCBCBCB, 0xB2B2B2,
    0xD7D7D7, 0xDDDDDD, 0xE3E3E3, 0xEAEAEA, 0xF1F1F1, 0xF8F8F8, 0xF0FBFF, 0xA4A0A0,
    0x808080, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
};
int default_palette_idx [256] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
    16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
    32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
    80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
    96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

/*
 * Set default palettes for use.
 * Use:
 *  void InitializePalettes ();
 * Returns:
 *  none
 */
void InitializePalettes ()
{
    register int i;

    /* initialize square difference values: */
    for (i = -255; i <= 255; i ++)
        SQ[i] = i * i;

    /* set default destination palette: */
    SetDestRGB8Palette (256, default_palette, default_palette_idx);

    /* set default source palette: */
    SetDestRGB8Palette (256, default_palette, default_palette_idx);
}

#endif

/* rgbpal.c -- end of file */
