/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: yuvalpha.c,v 1.4 2005/03/11 19:58:04 bobclark Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "env.h"
#include "yuvalpha.h"

#ifdef _USE_MMX_BLENDERS
//MMX alphablending only available on linux and windows right now.
//Should work just fine on any IA plaform though (intel solaris for example)
#include "mmx_util.h" //for checkMmxAvailablity()

HXBOOL z_bMMXAvailable = FALSE;
HXBOOL z_bCheckedForMMX = FALSE;

#endif

/////////////////////////////////////////////////////////////
// Note:  Output buffer can be the same as one of the input
//        buffers for I420 and YV12 output only.  Common 
//        input/output buffer must have same pitch and lines
//        width and height.
/////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////
//
// Define the bit offsets for components in both
// YUY2 and UYVU color formats.
//
/////////////////////////////////////////////////////////////

#if BYTE_ORDER == LITTLE_ENDIAN

// YUY2
#define YUY2_Y0_SHIFT 0
#define YUY2_U_SHIFT  8
#define YUY2_Y1_SHIFT 16
#define YUY2_V_SHIFT  24

// UYVY
#define UYVY_U_SHIFT  0
#define UYVY_Y0_SHIFT 8
#define UYVY_V_SHIFT  16
#define UYVY_Y1_SHIFT 24

#else /* BYTE_ORDER != LITTLE_ENDIAN */

// YUY2
#define YUY2_Y0_SHIFT 24
#define YUY2_U_SHIFT  16
#define YUY2_Y1_SHIFT 8
#define YUY2_V_SHIFT  0

// UYVY
#define UYVY_U_SHIFT  24
#define UYVY_Y0_SHIFT 16
#define UYVY_V_SHIFT  8
#define UYVY_Y1_SHIFT 0

#endif


/////////////////////////////////////////////////////////////
//
//	I420andYUVAtoI420orYV12
//
//	This function alpha-blends two I420 buffers into a third
//	I420 or YV12 buffer using the alpha info tacked to the 
//	end of the second I420 buffer
//
//	The alpha used for chroma is merely one (lower-right) of
//	the alpha used in the corresponding 2x2 luma pels.
//
/////////////////////////////////////////////////////////////

int I420andYUVAtoI420orYV12 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int output_format)
{
	int i, j;

	unsigned char *sy1, *su1, *sv1;
	unsigned char *sy2, *su2, *sv2, *sa2;
	unsigned char *dy, *du, *dv;

	// Initialize color plane pointers to beginning of image
	sy1 = src1_ptr;
	su1 = src1_ptr + src1_lines * src1_pitch;
	sv1 = su1 + src1_lines * src1_pitch / 4;

	sy2 = src2_ptr;
	su2 = src2_ptr + src2_lines * src2_pitch;
	sv2 = su2 + src2_lines * src2_pitch / 4;
	sa2 = sv2 + src2_lines * src2_pitch / 4;

	dy  = dest_ptr;

	if (output_format == CID_I420)
	{
		du  = dest_ptr + dest_lines * dest_pitch;
		dv  = du + dest_lines * dest_pitch / 4;
	}
	else if (output_format == CID_YV12)
	{
		dv  = dest_ptr + dest_lines * dest_pitch;
		du  = dv + dest_lines * dest_pitch / 4;
	}
	else
	{
		return -1;
	}

	// Move color plane pointers to start points
	sy1 += src1_starty * src1_pitch + src1_startx;
	su1 += (src1_starty * src1_pitch / 4) + src1_startx / 2;
	sv1 += (src1_starty * src1_pitch / 4) + src1_startx / 2;

	sy2 += src2_starty * src2_pitch + src2_startx;
	su2 += (src2_starty * src2_pitch / 4) + src2_startx / 2;
	sv2 += (src2_starty * src2_pitch / 4) + src2_startx / 2;
	sa2 += src2_starty * src2_pitch + src2_startx;

	dy += dest_starty * dest_pitch + dest_startx;
	du += (dest_starty * dest_pitch / 4) + dest_startx / 2;
	dv += (dest_starty * dest_pitch / 4) + dest_startx / 2;

	for (i = 0; i < height; i += 2)
	{
		// First line
		for (j = 0; j < width - 3; j += 4)
		{
			int x1, x2;
			
			// Y
			x1 = *(sy1 + 0);
			x2 = *(sy2 + 0);
			x1 -= x2;
			x1 *= *(sa2 + 0);
			x1 >>= 8;
			x1 += x2;
			*(dy + 0) = x1;

			x1 = *(sy1 + 1);
			x2 = *(sy2 + 1);
			x1 -= x2;
			x1 *= *(sa2 + 1);
			x1 >>= 8;
			x1 += x2;
			*(dy + 1) = x1;

			x1 = *(sy1 + 2);
			x2 = *(sy2 + 2);
			x1 -= x2;
			x1 *= *(sa2 + 2);
			x1 >>= 8;
			x1 += x2;
			*(dy + 2) = x1;

			x1 = *(sy1 + 3);
			x2 = *(sy2 + 3);
			x1 -= x2;
			x1 *= *(sa2 + 3);
			x1 >>= 8;
			x1 += x2;
			*(dy + 3) = x1;

			sy1 += 4;
			sy2 += 4;
			sa2 += 4;
			dy  += 4;
		}
		while (j < width)
		{
			int x1, x2;
			
			// Y
			x1 = *sy1++;
			x2 = *sy2++;
			x1 -= x2;
			x1 *= *sa2++;
			x1 >>= 8;
			x1 += x2;
			*dy++ = x1; 
			j++;
		}

		// move to next line
		sy1 += src1_pitch - width;
		sy2 += src2_pitch - width;
		sa2 += src2_pitch - width;
		dy  += dest_pitch - width;

		// Second line
		for (j = 0; j < width; j += 2)
		{
			int x1, x2, x1b, a;
			
			// Y
			x1 = *(sy1 + 0);
			x2 = *(sy2 + 0);
			x1 -= x2;
			x1 *= *(sa2 + 0);
			x1 >>= 8;
			x1 += x2;
			*(dy + 0) = x1;

			// Y
			x1 = *(sy1 + 1);
			x2 = *(sy2 + 1);
			x1 -= x2;
			a  = *(sa2 + 1);
			x1 *= a;
			x1 >>= 8;
			x1 += x2;
			*(dy + 1) = x1; 

			//We need to do something with the four alpha values
			//and this chroma value. We can average/mean/max/min?????
			//Avaerage alpha values....
			a += *(sa2+0);
			a += *((sa2-src2_pitch)+0);
			a += *((sa2-src2_pitch)+1);
			a >>= 2;
		
			//Max/Min?
			//a = max( *(sa2+0), *(sa2+1));

			sy1 += 2;
			sy2 += 2;
			sa2 += 2;
			dy  += 2;

			// U
			x1 = *su1++;
			x2 = *su2++;
			x1 -= x2;
			x1 *= a;
			x1 >>= 8;
			x1 += x2;

			// V
			x1b = *sv1++;
			x2  = *sv2++;
			x1b -= x2;
			x1b *= a;
			x1b >>= 8;
			x1b += x2;

			*du++ = x1;     // postpone store to allow inplace YV12
			*dv++ = x1b;

		}
		// move to next line
		sy1 += src1_pitch - width;
		su1 += (src1_pitch - width) / 2;
		sv1 += (src1_pitch - width) / 2;

		sy2 += src2_pitch - width;
		su2 += (src2_pitch - width) / 2;
		sv2 += (src2_pitch - width) / 2;
		sa2 += src2_pitch - width;

		dy  += dest_pitch - width;
		du  += (dest_pitch - width) / 2;
		dv  += (dest_pitch - width) / 2;

                
/*                  sy1 = src1_ptr; */
/*                  su1 = src1_ptr + src1_lines * src1_pitch; */
/*                  sv1 = su1 + src1_lines * src1_pitch / 4; */
                
/*                  sy2 = src2_ptr; */
/*                  su2 = src2_ptr + src2_lines * src2_pitch; */
/*                  sv2 = su2 + src2_lines * src2_pitch / 4; */
/*                  sa2 = sv2 + src2_lines * src2_pitch / 4; */
/*                  dy  = dest_ptr; */
                
/*                  if (output_format == CID_I420) */
/*                  { */
/*                      du  = dest_ptr + dest_lines * dest_pitch; */
/*                      dv  = du + dest_lines * dest_pitch / 4; */
/*                  } */
/*                  else if (output_format == CID_YV12) */
/*                  { */
/*                      dv  = dest_ptr + dest_lines * dest_pitch; */
/*                      du  = dv + dest_lines * dest_pitch / 4; */
/*                  } */
        
/*                  sy1 +=  (src1_starty+i) * src1_pitch + src1_startx; */
/*                  su1 += ((src1_starty+i) * src1_pitch / 4) + src1_startx / 2; */
/*                  sv1 += ((src1_starty+i) * src1_pitch / 4) + src1_startx / 2; */
                
/*                  sy2 +=  (src2_starty+i) * src2_pitch + src2_startx; */
/*                  su2 += ((src2_starty+i) * src2_pitch / 4) + src2_startx / 2; */
/*                  sv2 += ((src2_starty+i) * src2_pitch / 4) + src2_startx / 2; */
/*                  sa2 +=  (src2_starty+i) * src2_pitch + src2_startx; */
                
/*                  dy +=  (dest_starty+i) * dest_pitch + dest_startx; */
/*                  du += ((dest_starty+i) * dest_pitch / 4) + dest_startx / 2; */
/*                  dv += ((dest_starty+i) * dest_pitch / 4) + dest_startx / 2; */
	}

	return 0;
}



/////////////////////////////////////////////////////////////
//
//	I420andYUVAtoYUY2
//
//	This function alpha-blends two I420 buffers into a third
//	YUY2 buffer using the alpha info tacked to the 
//	end of the second I420 buffer
//
/////////////////////////////////////////////////////////////

int I420andYUVAtoYUY2 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height)
{
	int i, j;

	unsigned char *sy11, *sy12, *su1, *sv1;
	unsigned char *sy21, *sy22, *su2, *sv2, *sa21, *sa22;
	unsigned int  *dst1, *dst2;

	sy11 = src1_ptr;
	su1  = src1_ptr + src1_lines * src1_pitch;
	sv1  = su1 + src1_lines * src1_pitch / 4;

	sy21 = src2_ptr;
	su2  = src2_ptr + src2_lines * src2_pitch;
	sv2  = su2 + src2_lines * src2_pitch / 4;
	sa21 = sv2 + src2_lines * src2_pitch / 4;

	dst1 = (unsigned int *)(dest_ptr);

	// Move color planes to start point
	sy11 += src1_starty * src1_pitch + src1_startx;
	sy12  = sy11 + src1_pitch;
	su1  += src1_starty * src1_pitch / 4 + src1_startx / 2;
	sv1  += src1_starty * src1_pitch / 4 + src1_startx / 2;

	sy21 += src2_starty * src2_pitch + src2_startx;
	sy22  = sy21 + src2_pitch;
	su2  += src2_starty * src2_pitch / 4 + src2_startx / 2;
	sv2  += src2_starty * src2_pitch / 4 + src2_startx / 2;
	sa21 += src2_starty * src2_pitch + src2_startx;
	sa22  = sa21 + src2_pitch;

	dst1 += dest_starty * dest_pitch / 4 + dest_startx / 2;
	dst2  = dst1 + dest_pitch / 4;

	for (i = 0; i < height / 2; i++)
	{
		for (j = 0; j < width / 2; j++)
		{
			int x1, x2, a1, a2, d;
			int u1, u2, v1, v2;
			
			// First line

			// Y0
			x1 = *(sy11 + 0);
			x2 = *(sy21 + 0);
			x1 -= x2;
			a1 = *(sa21 + 0);
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d = (x1 << YUY2_Y0_SHIFT);

			// Y1
			x1 = *(sy11 + 1);
			x2 = *(sy21 + 1);
			x1 -= x2;
			a2 = *(sa21 + 1);
			x1 *= a2;
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << YUY2_Y1_SHIFT);

			sy11 += 2;
			sy21 += 2;
			sa21 += 2;

			// Average Alphas
			a1 += a2;
			a1 >>= 1;

			// U
			x1 = u1 = *su1;
			x2 = u2 = *su2;
			x1 -= x2;
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << YUY2_U_SHIFT);

			// V
			x1 = v1 = *sv1;
			x2 = v2 = *sv2;
			x1 -= x2;
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << YUY2_V_SHIFT);

			su1++;
			su2++;
			sv1++;
			sv2++;

			// Store YUY2 pel
			*dst1++ = d;

			// Second line

			// Y0
			x1 = *(sy12 + 0);
			x2 = *(sy22 + 0);
			x1 -= x2;
			a1 = *(sa22 + 0);
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d = (x1 << YUY2_Y0_SHIFT);

			// Y1
			x1 = *(sy12 + 1);
			x2 = *(sy22 + 1);
			x1 -= x2;
			a2 = *(sa22 + 1);
			x1 *= a2;
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << YUY2_Y1_SHIFT);

			sy12 += 2;
			sy22 += 2;
			sa22 += 2;

			// Average Alphas
			a1 += a2;
			a1 >>= 1;

			// U
			u1 -= u2;
			u1 *= a1;
			u1 >>= 8;
			u1 += u2;
			d |= (u1 << YUY2_U_SHIFT);

			// V
			v1 -= v2;
			v1 *= a1;
			v1 >>= 8;
			v1 += v2;
			d |= (v1 << YUY2_V_SHIFT);

			// Store YUY2 pel
			*dst2++ = d;

		}
		// move down two lines
		sy11 += 2 * src1_pitch - width;
		sy12 += 2 * src1_pitch - width;
		su1 += (src1_pitch - width) / 2;
		sv1 += (src1_pitch - width) / 2;

		sy21 += 2 * src2_pitch - width;
		sy22 += 2 * src2_pitch - width;
		su2 += (src2_pitch - width) / 2;
		sv2 += (src2_pitch - width) / 2;

		sa21 += 2 * src2_pitch - width;
		sa22 += 2 * src2_pitch - width;

		dst1 += (dest_pitch / 2) - (width / 2);
		dst2 += (dest_pitch / 2) - (width / 2);
	}

	return 0;
}



/////////////////////////////////////////////////////////////
//
//	I420andYUVAtoUYVY
//
//	This function alpha-blends two I420 buffers into a third
//	UYVY buffer using the alpha info tacked to the 
//	end of the second I420 buffer
//
/////////////////////////////////////////////////////////////

int I420andYUVAtoUYVY (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height)
{
	int i, j;

	unsigned char *sy11, *sy12, *su1, *sv1;
	unsigned char *sy21, *sy22, *su2, *sv2, *sa21, *sa22;
	unsigned int  *dst1, *dst2;

	// Initialize color plane pointers
	sy11 = src1_ptr;
	su1  = src1_ptr + src1_lines * src1_pitch;
	sv1  = su1 + src1_lines * src1_pitch / 4;

	sy21 = src2_ptr;
	su2  = src2_ptr + src2_lines * src2_pitch;
	sv2  = su2 + src2_lines * src2_pitch / 4;
	sa21 = sv2 + src2_lines * src2_pitch / 4;

	dst1 = (unsigned int *)(dest_ptr);

	// Move color planes to start point
	sy11 += src1_starty * src1_pitch + src1_startx;
	sy12  = sy11 + src1_pitch;
	su1  += src1_starty * src1_pitch / 4 + src1_startx / 2;
	sv1  += src1_starty * src1_pitch / 4 + src1_startx / 2;

	sy21 += src2_starty * src2_pitch + src2_startx;
	sy22  = sy21 + src2_pitch;
	su2  += src2_starty * src2_pitch / 4 + src2_startx / 2;
	sv2  += src2_starty * src2_pitch / 4 + src2_startx / 2;
	sa21 += src2_starty * src2_pitch + src2_startx;
	sa22  = sa21 + src2_pitch;

	dst1 += dest_starty * dest_pitch / 4 + dest_startx / 2;
	dst2  = dst1 + dest_pitch / 4;

	for (i = 0; i < height / 2; i++)
	{
		for (j = 0; j < width / 2; j++)
		{
			int x1, x2, a1, a2, d;
			int u1, u2, v1, v2;
			
			// First line

			// Y0
			x1 = *(sy11 + 0);
			x2 = *(sy21 + 0);
			x1 -= x2;
			a1 = *(sa21 + 0);
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d = (x1 << UYVY_Y0_SHIFT);

			// Y1
			x1 = *(sy11 + 1);
			x2 = *(sy21 + 1);
			x1 -= x2;
			a2 = *(sa21 + 1);
			x1 *= a2; 
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << UYVY_Y1_SHIFT);

			sy11 += 2;
			sy21 += 2;
			sa21 += 2;

			// Average Alphas
			a1 += a2;
			a1 >>= 1;

			// U
			x1 = u1 = *su1;
			x2 = u2 = *su2;
			x1 -= x2;
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << UYVY_U_SHIFT);

			// V
			x1 = v1 = *sv1;
			x2 = v2 = *sv2;
			x1 -= x2;
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << UYVY_V_SHIFT);

			su1++;
			su2++;
			sv1++;
			sv2++;

			// Store YUY2 pel
			*dst1++ = d;

			// Second line

			// Y0
			x1 = *(sy12 + 0);
			x2 = *(sy22 + 0);
			x1 -= x2;
			a1 = *(sa22 + 0);
			x1 *= a1;
			x1 >>= 8;
			x1 += x2;
			d = (x1 << UYVY_Y0_SHIFT);

			// Y1
			x1 = *(sy12 + 1);
			x2 = *(sy22 + 1);
			x1 -= x2;
			a2 = *(sa22 + 1);
			x1 *= a2;
			x1 >>= 8;
			x1 += x2;
			d |= (x1 << UYVY_Y1_SHIFT);

			sy12 += 2;
			sy22 += 2;
			sa22 += 2;

			// Average Alphas
			a1 += a2;
			a1 >>= 1;

			// U
			u1 -= u2;
			u1 *= a1;
			u1 >>= 8;
			u1 += u2;
			d |= (u1 << UYVY_U_SHIFT);

			// V
			v1 -= v2;
			v1 *= a1;
			v1 >>= 8;
			v1 += v2;
			d |= (v1 << UYVY_V_SHIFT);

			// Store YUY2 pel
			*dst2++ = d;

		}
		// move down two lines
		sy11 += 2 * src1_pitch - width;
		sy12 += 2 * src1_pitch - width;
		su1 += (src1_pitch - width) / 2;
		sv1 += (src1_pitch - width) / 2;

		sy21 += 2 * src2_pitch - width;
		sy22 += 2 * src2_pitch - width;
		su2 += (src2_pitch - width) / 2;
		sv2 += (src2_pitch - width) / 2;

		sa21 += 2 * src2_pitch - width;
		sa22 += 2 * src2_pitch - width;

		dst1 += (dest_pitch / 2) - (width / 2);
		dst2 += (dest_pitch / 2) - (width / 2);
	}

	return 0;
}


/////////////////////////////////////////////////////////////
//
//	I420andYUVAtoI420
//
//	This function alpha-blends two I420 buffers into a third
//	I420 buffer using the alpha info tacked to the 
//	end of the second I420 buffer
//
/////////////////////////////////////////////////////////////

int I420andYUVAtoI420 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height)
{
	
	return I420andYUVAtoI420orYV12 (
		src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
		src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
		dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
		width, height, CID_I420);
}


/////////////////////////////////////////////////////////////
//
//	I420andYUVAtoYV12
//
//	This function alpha-blends two I420 buffers into a third
//	YV12 buffer using the alpha info tacked to the 
//	end of the second I420 buffer
//
/////////////////////////////////////////////////////////////

int I420andYUVAtoYV12 (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height)
{
	
	return I420andYUVAtoI420orYV12 (
		src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
		src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
		dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
		width, height, CID_YV12);
}


/////////////////////////////////////////////////////////////
//
//	I420andYUVA
//
//	This function alpha-blends two I420 buffers into a third
//	YV12 buffer using the alpha info tacked to the 
//	end of the second I420 buffer
//
/////////////////////////////////////////////////////////////
int HXEXPORT ENTRYPOINT(I420andYUVA) (
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
	                         int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
	                         int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
	                         int dest_startx, int dest_starty,
	int width,  int height,  int color_format)
{
#ifdef _USE_MMX_BLENDERS
    if( !z_bCheckedForMMX )
    {
        z_bMMXAvailable = (checkMmxAvailablity()&CPU_HAS_MMX)?1:0;
        z_bCheckedForMMX = TRUE;
    }
    
    if( z_bMMXAvailable )
    {
        switch (color_format)
        {
           case CID_I420:
               return I420andYUVAtoI420_MMX (
                   src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
                   src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
                   dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
                   width, height);
           case CID_YV12:
               return I420andYUVAtoYV12_MMX (
                   src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
                   src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
                   dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
                   width, height);
           case CID_YUY2:
               return I420andYUVAtoYUY2_MMX (
                   src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
                   src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
                   dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
                   width, height);
           case CID_UYVY:
               return I420andYUVAtoUYVY_MMX (
                   src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
                   src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
                   dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
                   width, height);
           default:
               return -1;
        }
    }
#endif
    switch (color_format)
    {
       case CID_I420:
       case CID_YV12:
           return I420andYUVAtoI420orYV12 (
               src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
               src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
               dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
               width, height, color_format);
       case CID_YUY2:
           return I420andYUVAtoYUY2 (
               src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
               src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
               dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
               width, height);
       case CID_UYVY:
           return I420andYUVAtoUYVY (
               src1_ptr, src1_pels, src1_lines, src1_pitch, src1_startx, src1_starty,
               src2_ptr, src2_pels, src2_lines, src2_pitch, src2_startx, src2_starty,
               dest_ptr, dest_pels, dest_lines, dest_pitch, dest_startx, dest_starty,
               width, height);
       default:
           return -1;
    }
    
}


/////////////////////////////////////////////////////////////
//
//	I420andI420toI420
//
//	This function alpha-blends two I420 buffers into a third
//	I420 buffer using the constant alpha given in the params.
//
/////////////////////////////////////////////////////////////

int HXEXPORT ENTRYPOINT(I420andI420toI420) (
    unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
    int src1_startx, int src1_starty,
    unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
    int src2_startx, int src2_starty,
    unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
    int dest_startx, int dest_starty,
    int width,  int height,  int alpha )
{
    int ALPHA_tab[511];
    
    int i, j, c;
    unsigned char *s1, *s2, *d;
    int s1_lineskip, s2_lineskip, d_lineskip;

#ifdef _USE_MMX_BLENDERS
    if( !z_bCheckedForMMX )
    {
        z_bMMXAvailable = (checkMmxAvailablity()&CPU_HAS_MMX)?1:0;
        z_bCheckedForMMX = TRUE;
    }

    if( z_bMMXAvailable )
    {
        return I420andI420toI420_MMX_sub (
            src1_ptr, src1_pels,   src1_lines,  src1_pitch,
            src1_startx, src1_starty,
            src2_ptr, src2_pels,   src2_lines,  src2_pitch,
            src2_startx, src2_starty,
            dest_ptr, dest_pels,   dest_lines,  dest_pitch,
            dest_startx, dest_starty,
            width,  height,  alpha );
    }
#endif
    for (i = 0; i < 511; i++)
    {
        ALPHA_tab[i] = (i - 255) * alpha;
    }
    
    for (c = 0; c < 3; c++)
    {
        switch (c)
        {
           case 0:
               // Y
               s1 = src1_ptr + src1_starty * src1_pitch + src1_startx;
               s2 = src2_ptr + src2_starty * src2_pitch + src2_startx;
               d  = dest_ptr + dest_starty * dest_pitch + dest_startx;
               s1_lineskip = src1_pitch - width;
               s2_lineskip = src2_pitch - width;
               d_lineskip  = dest_pitch - width; 
               break;
           case 1:
               // U
               s1  = src1_ptr + src1_pitch * src1_lines;
               s1 += (src1_starty * src1_pitch / 4) + src1_startx / 2;

               s2  = src2_ptr + src2_pitch * src2_lines;
               s2 += (src2_starty * src2_pitch / 4) + src2_startx / 2;

               d   = dest_ptr + dest_pitch * dest_lines;
               d  += (dest_starty * dest_pitch / 4) + dest_startx / 2;

               s1_lineskip = (src1_pitch - width) / 2;
               s2_lineskip = (src2_pitch - width) / 2;
               d_lineskip  = (dest_pitch - width) / 2;
               width >>= 1;
               height >>= 1;
               break;
           case 2:
               // V
               s1  = src1_ptr + 5 * src1_pitch * src1_lines / 4;
               s1 += (src1_starty * src1_pitch / 4) + src1_startx / 2;

               s2  = src2_ptr + 5 * src2_pitch * src2_lines / 4;
               s2 += (src2_starty * src2_pitch / 4) + src2_startx / 2;

               d   = dest_ptr + 5 * dest_pitch * dest_lines / 4;
               d  += (dest_starty * dest_pitch / 4) + dest_startx / 2;
               break;
        }

        for (i = 0; i < height; i++)
        {
            for (j = 0; j < (width-3); j += 4)
            {
                int x1, x2;

                x1 = *(s1 + 0);
                x2 = *(s2 + 0);
                x1 -= x2;
                x1 = *(ALPHA_tab + x1 + 255);
                x1 >>= 8;
                x1 += x2;
                *(d + 0) = (unsigned char)x1;

                x1 = *(s1 + 1);
                x2 = *(s2 + 1);
                x1 -= x2;
                x1 = *(ALPHA_tab + x1 + 255);
                x1 >>= 8;
                x1 += x2;
                *(d + 1) = (unsigned char)x1;

                x1 = *(s1 + 2);
                x2 = *(s2 + 2);
                x1 -= x2;
                x1 = *(ALPHA_tab + x1 + 255);
                x1 >>= 8;
                x1 += x2;
                *(d + 2) = (unsigned char)x1;

                x1 = *(s1 + 3);
                x2 = *(s2 + 3);
                x1 -= x2;
                x1 = *(ALPHA_tab + x1 + 255);
                x1 >>= 8;
                x1 += x2;
                *(d + 3) = (unsigned char)x1;

                s1 += 4;
                s2 += 4;
                d  += 4;
            }
            while (j < width)
            {
                int x1, x2;

                x1 = *s1;
                x2 = *s2;
                x1 -= x2;
                x1 = *(ALPHA_tab + x1 + 255);
                x1 >>= 8;
                x1 += x2;
                *d++ = (unsigned char)x1;
                s1++;
                s2++;
                j++;
            }

            s1 += s1_lineskip;
            s2 += s2_lineskip;
            d  += d_lineskip;
        }
    }
    return 0;
}


#if 0

void main (void)
{
	unsigned char *s1, *s2, *dst;
	int pels = 64;
	int lines = 32;
	int pitch = 64;

	s1  = malloc (lines*pitch*3/2);
	s2  = malloc (lines*pitch*5/2);
	dst = malloc (lines*pitch*3/2);

	memset (s1, 64, lines*pitch*3/2);
	memset (s2, 192, lines*pitch*3/2);
	memset (s2 + lines*pitch*3/2, 64, lines*pitch);

	I420andYUVAtoI420orYV12 (
		s1, pels, lines, pitch, pels, lines,
		s2, pels, lines, pitch, pels, lines,
		dst, pels, lines, pitch, pels, lines,
		pels, lines, CID_I420);

	free (s1);
	free (s2);
	free (dst);
}


#endif














