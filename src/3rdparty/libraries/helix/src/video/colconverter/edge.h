/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: edge.h,v 1.2 2004/07/09 18:36:28 hubbe Exp $
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

//edge enhacement params
#define HELPER_MAXLEN 4096
#define OFFSET_SOFTCORE 255
#define OFFSET_CLIP_SOFTCORE 319
#define LUT_LENGTH 511
#define CLIP_TABLE_LENGTH 830

#define DEFT_C 10
#define DEFT_A 0
#define DEFT_B 25
#define MAX_B 50

#define ZOOM_BOOST 1.2

#define CONST_C 50
#define CONST_A 0
#define CONST_B 50


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*Edge enhancemnt functions*/
void HXEXPORT ENTRYPOINT(Enhance)(UCHAR *yuv420in, INT32 inheight, INT32  inwidth, INT32 pitchSrc, float Sharpness);
void HXEXPORT ENTRYPOINT(EnhanceUniform)(UCHAR *yuv420in, INT32 inheight, INT32  inwidth, INT32 pitchSrc, float Sharpness);
void HXEXPORT ENTRYPOINT(SetSharpnessAdjustments)(float Sharpness, INT16 nExpand);
void HXEXPORT ENTRYPOINT(GetSharpnessAdjustments)(float *pSharpness, INT16* pnExpand);

int Add2DHelper( int *cur, int *prev, unsigned char *pic, int n );
int DiffNonLin2D( unsigned char* pos, unsigned char* neg, int* out, int n );
int DiffNonLin2Duniform( unsigned char* pos, unsigned char* neg, int* out, int n );
int DiffNonLin2Dconst( unsigned char* pos, unsigned char* neg, int* out, int n );
void Initcliplut(void);
int Inittriangleluts(float c, float a, float b);
int Inittrianglelutsconst(void);
int soft_triangle_lut_2d(float input, float c, float a, float b);


#ifdef __cplusplus
}
#endif /* __cplusplus */ 
