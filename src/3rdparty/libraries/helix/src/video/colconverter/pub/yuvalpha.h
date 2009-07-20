/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: yuvalpha.h,v 1.3 2007/07/06 20:53:53 jfinnecy Exp $
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

#ifndef _YUVALPHA_H_
#define _YUVALPHA_H_

#include "ciddefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int HXEXPORT ENTRYPOINT(I420andI420toI420) (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int alpha );

int HXEXPORT ENTRYPOINT(I420andYUVA) (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int color_format);

int I420andYUVAtoYUY2 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height);

int I420andYUVAtoUYVY (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height);

int I420andYUVAtoI420 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height);

int I420andYUVAtoYV12 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height);

int I420andYUVAtoI420orYV12 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int output_format);


int I420andYUVAtoI420_MMX(
    unsigned char* src,  int src_pels,    int src_lines,  int src_pitch,
                         int src_startx,  int src_starty,
    unsigned char* yuva, int yuva_pels,   int yuva_lines, int yuva_pitch,
                         int yuva_startx, int yuva_starty,
    unsigned char* dst,  int dst_pels,    int dst_lines,  int dst_pitch,
                         int dst_startx,  int dst_starty,
    int width,  int height);

int I420andYUVAtoYV12_MMX(
    unsigned char* src,  int src_pels,    int src_lines,  int src_pitch,
                         int src_startx,  int src_starty,
    unsigned char* yuva, int yuva_pels,   int yuva_lines, int yuva_pitch,
                         int yuva_startx, int yuva_starty,
    unsigned char* dst,  int dst_pels,    int dst_lines,  int dst_pitch,
                         int dst_startx,  int dst_starty,
    int width,  int height);

int I420andYUVAtoYUY2_MMX (
    unsigned char* src,  int src_pels,    int src_lines,  int src_pitch,
                         int src_startx,  int src_starty,
    unsigned char* yuva, int yuva_pels,   int yuva_lines, int yuva_pitch,
                         int yuva_startx, int yuva_starty,
    unsigned char* dst,  int dst_pels,    int dst_lines,  int dst_pitch,
                         int dst_startx,  int dst_starty,
    int width,  int height);

int I420andYUVAtoUYVY_MMX (
    unsigned char* src,  int src_pels,    int src_lines,  int src_pitch,
                         int src_startx,  int src_starty,
    unsigned char* yuva, int yuva_pels,   int yuva_lines, int yuva_pitch,
                         int yuva_startx, int yuva_starty,
    unsigned char* dst,  int dst_pels,    int dst_lines,  int dst_pitch,
                         int dst_startx,  int dst_starty,
    int width,  int height);

int I420andI420toI420_MMX_sub (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int alpha );

int HXEXPORT ENTRYPOINT(I420andI420toI420_MMX) (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int alpha );

int HXEXPORT ENTRYPOINT(I420andYUVA_MMX) (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int color_format);

#ifdef __cplusplus
}
#endif /* __cplusplus */ 

#endif /* _YUVALPHA_H_ */
