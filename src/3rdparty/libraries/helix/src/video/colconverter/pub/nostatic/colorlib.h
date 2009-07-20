/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: colorlib.h,v 1.8 2007/07/06 20:53:53 jfinnecy Exp $
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

#ifndef __COLORLIB_H__
#define __COLORLIB_H__  1

#include "yuv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initializes "from/to I420" converters.
 * Use:
 *  void SetSrcI420Colors (float Brightness, float Contrast, float Saturation, float Hue);
 * Input:
 *  Brightness - brightness adjustment [-1,1], 0 - default;
 *  Contrast   - contrast adjustment   [-1,1], 0 - default;
 *  Saturation - saturation adjustment [-1,1], 0 - default;
 *  Hue        - hue adjustment        [-1,1], 0 - default;
 * Returns:
 *  none.
 */
void SetSrcI420Colors (float Brightness, float Contrast, float Saturation, float Hue,
		       color_data_t* pcd);

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
#define CRM_11_11_Interlace     3       /* TF Chroma = (a+b+e+f) / 4 ; BF Chroma = (c+d+g+h) /4 */

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

  int I420toRGB24   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int, color_data_t* pcd);
  int I420toRGB565   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int, color_data_t* pcd);
  int I420toRGB444   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int, color_data_t* pcd);
  int I420toRGB444v565   (unsigned char *, int, int, int, int, int, int, int, unsigned char *, int, int, int, int, int, int, int, color_data_t* pcd);

  void InitI420toRGB24(float, float, float, float, color_data_t* pcd);
  void InitI420toRGB565(float, float, float, float, color_data_t* pcd);
  void InitI420toRGB444(float, float, float, float, color_data_t* pcd);
  void InitI420toRGB444v565(float, float, float, float, color_data_t* pcd);

  void UninitI420toRGB24( );
  void UninitI420toRGB565( );
  void UninitI420toRGB444( );
  void UninitI420toRGB444v565( );

#if defined(HELIX_FEATURE_SMIL_SITE)
    int RGB32toRGB444( unsigned char *dest_ptr,
                       int dest_width, int dest_height, int dest_pitch, int dest_x,
                       int dest_y, int dest_dx, int dest_dy,
                       unsigned char *src_ptr,
                       int src_width, int src_height, int src_pitch,
                       int src_x, int src_y, int src_dx, int src_dy,
                       color_data_t* not_used);
    int RGB444toRGB444(unsigned char *dest_ptr,
                       int dest_width, int dest_height, int dest_pitch, int dest_x,
                       int dest_y, int dest_dx, int dest_dy,
                       unsigned char *src_ptr,
                       int src_width, int src_height, int src_pitch,
                       int src_x, int src_y, int src_dx, int src_dy,
                       color_data_t* not_used);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __COLORLIB_H__ */

