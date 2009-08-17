/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: colorlib.h,v 1.5 2004/12/29 02:08:59 rascar Exp $
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

#ifndef __COLORLIB_H__
#define __COLORLIB_H__  1

#ifdef __cplusplus
extern "C" {
#endif

#if defined (HXCOLOR_ALLYUV2RGB)

#define HELIX_FEATURE_CC_RGB32out
#define HELIX_FEATURE_CC_BGR32out
#define HELIX_FEATURE_CC_RGB24out
#define HELIX_FEATURE_CC_RGB565out
#define HELIX_FEATURE_CC_RGB555out
#define HELIX_FEATURE_CC_RGB8out

#endif //HXCOLOR_ALLYUV2RGB

/* setclrs.c: */

/*
 * Returns current state of "from/to I420" converters.
 * Use:
 *  void GetSrcI420Colors (float *brightness, float *contrast, float *saturation, float *hue);
 * Output:
 *  brightness - brightness adjustment [-1,1], 0 - default;
 *  contrast   - contrast adjustment   [-1,1], 0 - default;
 *  saturation - saturation adjustment [-1,1], 0 - default;
 *  hue        - hue adjustment        [-1,1], 0 - default;
 * Returns:
 *  none.
 */
void GetSrcI420Colors (float *brightness, float *contrast, float *saturation, float *hue);

/*
 * Initializes "from/to I420" converters.
 * Use:
 *  void SetSrcI420Colors (float Brightness, float Contrast, float Saturation, float Hue);
 *  void SetDestI420Colors (float Brightness, float Contrast, float Saturation, float Hue);
 * Input:
 *  Brightness - brightness adjustment [-1,1], 0 - default;
 *  Contrast   - contrast adjustment   [-1,1], 0 - default;
 *  Saturation - saturation adjustment [-1,1], 0 - default;
 *  Hue        - hue adjustment        [-1,1], 0 - default;
 * Returns:
 *  none.
 */
void SetSrcI420Colors (float Brightness, float Contrast, float Saturation, float Hue);
void SetDestI420Colors (float Brightness, float Contrast, float Saturation, float Hue);

/*
 * YUV 4:2:0 chroma resampling mode:
 * Input chroma     a b
 *  samples:        c d
 *                  e f
 *                  g h
 */
#define CRM_11_11               0       /* (a+b+c+d) / 4 */
#define CRM_11_00               1       /* (a+b) / 2     */
#define CRM_00_11               2       /* (c+d) / 2     */
#define CRM_11_11_Interlace     3       /* TF Chroma = (a+b+e+f) / 4 BF Chroma = (c+d+g+h) /4 */

/*
 * Sets RGB->I420 chroma resampling mode
 * Use:
 *  int SetI420ChromaResamplingMode(int nNewMode);
 * Input:
 *  nNewMode - a new chroma resampling mode to use
 * Returns:
 *  previously set mode.
 */
int SetI420ChromaResamplingMode(int nNewMode);

/* setpal.c: */

/*
 * Suggest palettes to use for 8-bit RGB formats.
 * Use:
 *  int SuggestSrcRGB8Palette (int nColors, unsigned long *lpRGBVals);
 *  int SuggestDestRGB8Palette (int nColors, unsigned long *lpRGBVals);
 * Input:
 *  nColors - the number of existing colors in palette
 *  lpRGBVals - RGB values for each palette entry
 *  lpIndices - pixel values (palette indices)
 * Returns:
 *  >0, the total number of colors to be used, if success
 *  -1, if error
 */
int SuggestSrcRGB8Palette (int nColors, unsigned int *lpRGBVals);
int SuggestDestRGB8Palette (int nColors, unsigned int *lpRGBVals);

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
int SetSrcRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices);
int SetDestRGB8Palette (int nColors, unsigned int *lpRGBVals, int *lpIndices);


/*
 * Format-conversion routines.
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

/* yuv2yuv.c: */

int I420toI420     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toYV12     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toYUY2     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toUYVY     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int I420toI420x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toYV12x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toYUY2x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toUYVYx    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);

int YV12toI420     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toYV12     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toYUY2     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toUYVY     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int YV12toI420x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toYV12x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toYUY2x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toUYVYx    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);

int YVU9toI420     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YUY2toI420     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int UYVYtoI420     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int YUVUtoI420     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int YUY2toI420x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int UYVYtoI420x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);

int YUY2toYV12     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int UYVYtoYV12     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YUY2toYUY2     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int UYVYtoUYVY     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YUY2toUYVY     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int UYVYtoYUY2     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int YUY2toYV12x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int UYVYtoYV12x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);

int XINGtoYV12     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int XINGtoYUY2     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int XINGtoUYVY     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int XINGtoRGB32    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int XINGtoRBG32    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int XINGtoRGB24    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int XINGtoRGB565   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int XINGtoRGB555   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int XINGtoRGB8     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

/* yuv2rgb.c: */

int I420toRGB32    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toBGR32    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toRGB24    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toRGB565   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toRGB555   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toRGB444   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int I420toRGB8     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int I420toRGB32x   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toBGR32x   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toRGB24x   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toRGB565x  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toRGB555x  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toRGB444x  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int I420toRGB8x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);

int YV12toRGB32    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toBGR32    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toRGB24    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toRGB565   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toRGB555   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int YV12toRGB8     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int YV12toRGB32x   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toBGR32x   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toRGB24x   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toRGB565x  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toRGB555x  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);
int YV12toRGB8x    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, unsigned char *, unsigned char *, int, int, int, int, int, int, int, int, int);

/* rgb2yuv.c: */

int RGB32toI420   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB24toI420   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB565toI420  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB555toI420  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB8toI420    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int ARGBtoYUVA    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int BGR_32toI420  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int BGR24toI420   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

/* rgb2rgb.c: */

int RGB32toRGB32   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB32toRGB24   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB32toRGB565  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB32toRGB555  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB32toRGB8    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB24toRGB32   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB24toRGB24   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB24toRGB565  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB24toRGB555  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB24toRGB8    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB565toRGB32  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB565toRGB24  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB565toRGB565 (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB565toRGB555 (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB565toRGB8   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB555toRGB32  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB555toRGB24  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB555toRGB565 (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB555toRGB555 (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB555toRGB8   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB8toRGB32    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB8toRGB24    (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB8toRGB565   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB8toRGB555   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB8toRGB8     (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

int  RGB32toBGR32  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int  RGB24toBGR32  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB565toBGR32  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int RGB555toBGR32  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);
int   RGB8toBGR32  (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int);

/* yuv2rgb.c: */

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
void oldI420toRGB32  (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);
void oldI420toRGB24  (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);
void oldI420toRGB565 (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);
void oldI420toRGB555 (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);

void oldI420toRGB32x2  (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);
void oldI420toRGB24x2  (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);
void oldI420toRGB565x2 (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);
void oldI420toRGB555x2 (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int);

#ifdef __cplusplus
}
#endif

#endif /* __COLORLIB_H__ */

