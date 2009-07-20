/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcolor.h,v 1.7 2007/07/06 20:53:53 jfinnecy Exp $
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

#ifndef _HXCOLOR_H_
#define _HXCOLOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ciddefs.h"

/*
 * component GUID support.
 */
HX_RESULT HXEXPORT ENTRYPOINT(GetHXColorGUID) (UCHAR* pGUID);

/*
 * Initialize color conversion library.
 */
void HXEXPORT ENTRYPOINT(InitColorConverter) ();

/*
 * Set YUV color adjustments.
 * Use:
 *  void SetColorAdjustments (float Brightness,
 *              float Contrast, float Saturation, float Hue);
 * Input:
 *  Brightness - brightness adjustment [-1,1], default - 0
 *  Contrast   - contrast adjustment [-1,1], default - 0
 *  Saturation - saturation adjustment [-1,1], default - 0
 *  Hue        - hue adjustment  [-1,1], default - 0
 * Returns:
 *  none.
 */
void HXEXPORT ENTRYPOINT(SetColorAdjustments) (float Brightness,
                float Contrast, float Saturation, float Hue);

/*
 * Get the current YUV color adjustments.
 * Use:
 *  void GetColorAdjustments (float *Brightness,
 *              float *Contrast, float *Saturation, float *Hue);
 * Output:
 *  Brightness - brightness adjustment [-1,1], default - 0
 *  Contrast   - contrast adjustment [-1,1], default - 0
 *  Saturation - saturation adjustment [-1,1], default - 0
 *  Hue        - hue adjustment  [-1,1], default - 0
 * Returns:
 *  none.
 */
void HXEXPORT ENTRYPOINT(GetColorAdjustments) (float* Brightness,
                float* Contrast, float* Saturation, float* Hue);

/*
 * Suggest palette to use for 8-bit RGB output.
 * Use:
 *  int SuggestRGB8Palette (int nColors, UINT32 *lpRGBVals);
 * Input:
 *  nColors - the number of existing colors in palette
 *  lpRGBVals - RGB values for each palette entry
 *  lpIndices - pixel values (palette indices)
 * Returns:
 *  >0, the total number of colors to be used, if success
 *  -1, if error
 */
#ifdef _FAT_HXCOLOR
int HXEXPORT ENTRYPOINT(SuggestRGB8Palette) (int nColors, UINT32 *lpRGBVals);
#endif

/*
 * Set palette to use for 8-bit RGB output.
 * Use:
 *  int SetRGB8Palette (int nColors, UINT32 *lpRGBVals, int *lpIndices);
 * Input:
 *  nColors - the number of colors in palette
 *  lpRGBVals - RGB values for each palette entry
 *  lpIndices - pixel values (palette indices)
 * Returns:
 *  >0, the total number of colors set, if success
 *  -1, if error
 */
#ifdef _FAT_HXCOLOR
int HXEXPORT ENTRYPOINT(SetRGB8Palette) (int nColors, UINT32 *lpRGBVals, int *lpIndices);
#endif
/*
 * Sets YUV 4:4:4 -> YUV 4:2:0 chroma resampling mode
 * Use:
 *  int SetChromaResamplingMode(int nNewMode);
 * Input:
 *  nNewMode - a chroma resampling mode to use
 *   E.g. given input   a b
 *   chroma samples:    c d
 *   we will produce:
 *      mode = 0:       (a+b+c+d) / 4
 *             1:       (a+b) / 2
 *             2:       (c+d) / 2
 * Returns:
 *  previously set resampling mode.
 */
#ifdef _FAT_HXCOLOR
int HXEXPORT ENTRYPOINT(SetChromaResamplingMode)(int nNewMode);
#endif

/*
 * Low-level format-conversion routines.
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
typedef int (* LPHXCOLORCONVERTER) \
            (unsigned char *, int, int, int, int, int, int, int, \
            unsigned char *, int, int, int, int, int, int, int);
    
#if defined(HELIX_CONFIG_NOSTATICS)
#include "nostatic/yuv.h" //for color_data_t
    typedef int (* LPHXCOLORCONVERTERNOSTATIC) \
          (unsigned char *, int, int, int, int, int, int, int, \
           unsigned char *, int, int, int, int, int, int, int, \
           color_data_t* );
    
#endif
    

HX_RESULT HXEXPORT ENTRYPOINT(GetCompatibleColorFormats)(INT32 cidIn, INT32* pcidOut, UINT32* pnSize);

/*
 * Find a converter to trasform data from format X to format Y.
 * Use:
 *  LPHXCOLORCONVERTER GetColorConverter (INT32 cidIn, INT32 cidOut);
 * Input:
 *  cidIn - input color format ID (!CID_UNKNOWN)
 *  cidOut - desirable output color format ID (!CID_UNKNOWN)
 * Returns:
 *  pointer to an appropriate color conversion routine, if success;
 *  NULL - conversion is not supported.
 */
LPHXCOLORCONVERTER HXEXPORT ENTRYPOINT(GetColorConverter) (INT32 cidIn, INT32 cidOut);

/*
 * Low-level format-conversion routines.
 * Use:
 *  int I420toYYYY (unsigned char *dest_ptr, int dest_width, int dest_height,
 *      int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
 *      unsigned char *pY, unsigned char *pU, unsigned char *pV,
 *      int src_width, int src_height, int yPitch, int uPitch, int vPitch,
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
 *  yPitch - pitch of the y source buffer (in bytes; <0 - if bottom up image)
 *  uPitch - pitch of the u source buffer (in bytes; <0 - if bottom up image)
 *  vPitch - pitch of the v source buffer (in bytes; <0 - if bottom up image)
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
typedef int (* LPHXCOLORCONVERTER2) \
                (unsigned char *, int, int, int, int, int, int, int, \
                 unsigned char *, unsigned char *, unsigned char *,  \
                 int, int, int, int, int, int, int, int, int);

LPHXCOLORCONVERTER2 HXEXPORT ENTRYPOINT(GetColorConverter2) (INT32 cidIn, INT32 cidOut);

/*
 * Try selected compatible color formats.
 * Use:
 *  HXBOOL ScanCompatibleColorFormats (INT32 cidIn, INT32 cidOutMask, void *pParam,
 *      HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC));
 * Input:
 *  cidIn - input color format ID (!CID_UNKNOWN)
 *  cidOutMask - masks output formats to try (use ~0 to scan all formats)
 *  pParam - pointer to a parameter block to pass to fpTryIt ()
 *  pfnTryIt - pointer to a function, which will be called for each
 *          compatible output format;
 * Special Inputs:
 *  Last INT32 argument is dummy unused one created just for avoiding ADS 1.2 ARM compiler's crash due to 
 *  not being able handle nested function ptr as the last argument. It should be taken out once we
 *  are sure that is not the case such as compiler update from current 1.2 to 2.0.
 *
 * Returns:
 *  TRUE, if fpTryIt() has exited with TRUE status;
 *  FALSE, if non of the compatible formats has been accepted by fpTryIt().
 */
HXBOOL HXEXPORT ENTRYPOINT(ScanCompatibleColorFormats) (INT32 cidIn, INT32 cidOutMask, void *pParam,
    HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC),INT32=0);

/*
 * Try all compatible color formats.
 */
HXBOOL HXEXPORT ENTRYPOINT(ScanAllCompatibleColorFormats) (INT32 cidIn, void *pParam,
    HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC),INT32=0);


/***********************
 * Old HXCOLOR.DLL interface:
 ****************************************************/

#define T_RGB888 7
#define T_RGB555 8
#define T_RGB565 9

/*
 * Main color-conversion function.
 *
 * Input:
 *  ysrc, usrc, vsrc - pointers to planar YUV420
 *  dst - pointer to RGB output buffer
 *  width, height - size of input image, in pixels
 *  pitch - horizontal pitch of RGB output buffer, in pixels
 *          negative pitch toggles inverted output
 *          usually, pitch = width (or 2*width if expanding)
 *  format - format of RGB output buffer
 *  expand - flag to enable bilinear-interpolated output
 *           output buffer must be twice as large
 *
 * Output:
 *  dst filled with RGB output.
 */
#ifdef _FAT_HXCOLOR

void HXEXPORT ENTRYPOINT(ConvertYUVtoRGB)   (UCHAR* ySrc,
                 UCHAR* uSrc,
                 UCHAR* vSrc,
                 INT32  nPitchSrc,
                 UCHAR* Dst,
                 INT32  nWidth,
                 INT32  nHeight,
                 INT32  nPitchDst,
                 INT16  nFormat,
                 INT16  nExpand);

#endif
/*
 * Mac color-conversion function.
 *
 * Input:
 *  ysrc, usrc, vsrc - pointers to planar YUV420
 *  dst - pointer to RGB output buffer
 *  width, height - size of input image, in pixels
 *  pitch - horizontal pitch of RGB output buffer, in pixels
 *          negative pitch toggles inverted output
 *          usually, pitch = width (or 2*width if expanding)
 *  format - format of RGB output buffer
 *  expand - flag to enable bilinear-interpolated output
 *           output buffer must be twice as large
 *
 * Output:
 *  dst filled with RGB output.
 */
#ifdef _FAT_HXCOLOR

#ifdef _MACINTOSH
#pragma export on
#endif //_MACINTOSH

void HXEXPORT ENTRYPOINT(ConvertYUVtoMacRGB32)   (UCHAR* ySrc,
                 UCHAR* uSrc,
                 UCHAR* vSrc,
                 INT32  nPitchSrc,
                 UCHAR* Dst,
                 INT32  nWidth,
                 INT32  nHeight,
                 INT32  nPitchDst,
                 INT16  nFormat,
                 INT16  nExpand);

#ifdef _MACINTOSH
#pragma export off
#endif //_MACINTOSH

#endif
/*
 * Converts a RGB3 buffer into a YUV buffer in Y'CbCr 4:2:0 format.
 */
#ifdef _FAT_HXCOLOR
void HXEXPORT ENTRYPOINT(ConvertRGBtoYUV)   (UCHAR* pInput,
                 UCHAR* pOutput,
                 INT32  nWidth,
                 INT32  nHeight,
                 HXBOOL   bBGR);
#endif
/*
 * Converts a RGB3 buffer into an XRGB buffer.
 */
#ifdef _FAT_HXCOLOR
void HXEXPORT ENTRYPOINT(ConvertRGB24toXRGB) (UCHAR* pSrc, UCHAR* pDest,
    ULONG32 srcSize, ULONG32 destSize, INT32 nWidth, INT32 nHeight);
#endif

#ifdef __cplusplus
}
#endif

#endif // _HXCOLOR_H_
