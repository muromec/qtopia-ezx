/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: avyuv2yuv.c,v 1.6 2007/07/06 20:53:51 jfinnecy Exp $
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

#if (defined(_MACINTOSH) || defined(_MAC_UNIX)) && !defined(__i386__)

#pragma altivec_model on

#define VecI8 vector signed char
#define VecU8 vector unsigned char
#define VecI16 vector signed short
#define VecU16 vector unsigned short
#define VecI32 vector signed int
#define VecU32 vector unsigned int

#define U32 unsigned int

#define COMBINE(b0,b1,b2,b3)  \
    ((unsigned int)(b3) | ((unsigned int)(b2) << 8) | \
    ((unsigned int)(b1) << 16) | ((unsigned int)(b0) << 24))
    
    
#ifdef _MACINTOSH
#ifdef __cplusplus
extern "C"
#endif
void
AltiVec_lineI420toYUY2 (
	unsigned char *sy, 
	unsigned char *su, 
	unsigned char *sv,
	unsigned char *d,
	int dest_dx);
#endif

void
AltiVec_lineI420toYUY2 (
	unsigned char *sy, 
	unsigned char *su, 
	unsigned char *sv,
	unsigned char *d,
	int dest_dx)
{
	VecU8	v0, v1;
	VecU8	v2, v3;
	VecU8	v4, v5;
	VecU8	v6, v7;
	VecU8	vPY, vPU, vPV;
	
	// convert until *d is 16-byte aligned
	while (((int)d & 15) != 0)
	{
		*(U32*)d = COMBINE(sy[0], su[0], sy[1], sv[0]);
		
		sy += 2;
		su++;
		sv++;
		d += 4;
		dest_dx -= 2;
	}
	
	// Special case for all pointers 16-byte aligned
	if ((((int)sy | (int)su | (int)sv) & 15) == 0)
	{
		while (dest_dx >= 32)
		{
			v0  = vec_ld(0, sy);			// v0 = y0|y1|y2|y3|y4|y5|y6|y7|y8|y9|ya|yb|yc|yd|ye|yf
			v2  = vec_ld(0, su);			// v2 = u0|u1|u2|u3|u4|u5|u6|u7|u8|u9|ua|ub|uc|ud|ue|uf
			v4  = vec_ld(0, sv);			// v4 = v0|v1|v2|v3|v4|v5|v6|v7|v8|v9|va|vb|vc|vd|ve|vf
			
			v6  = vec_mergeh(v2, v4);		// v6 = u0|v0|u1|v1|u2|v2|u3|v3|u4|v4|u5|v5|u6|v6|u7|v7
			v7  = vec_mergeh(v0, v6);		// v7 = y0|u0|y1|v0|y2|u1|y3|v1|y4|u2|y5|v2|y6|u3|y7|v3
			
			vec_st(v7, 0, d);				// store
			
			v7  = vec_mergel(v0, v6);		// v7 = y8|u4|y9|v4|ya|u5|yb|v5|yc|u6|yd|v6|ye|u7|yf|v7
			
			vec_st(v7, 16, d);				// store
			
			v0  = vec_ld(16, sy);			// v0 = y0|y1|y2|y3|y4|y5|y6|y7|y8|y9|ya|yb|yc|yd|ye|yf
			
			v6  = vec_mergel(v2, v4);		// v6 = u8|v8|u9|v9|ua|va|ub|vb|uc|vc|ud|vd|ue|ve|uf|vf
			v7  = vec_mergeh(v0, v6);		// v7 = y0|u8|y1|v8|y2|u9|y3|v9|y4|ua|y5|va|y6|ub|y7|vb
			
			vec_st(v7, 32, d);				// store
			
			v7  = vec_mergel(v0, v6);		// v7 = y8|uc|y9|vc|ya|ud|yb|vd|yc|ue|yd|ve|ye|uf|yf|vf
			
			vec_st(v7, 48, d);				// store
			
			sy += 32;
			su += 16;
			sv += 16;
			d  += 64;
			dest_dx -= 32;		
		}
		while (dest_dx >= 16)
		{
			v0  = vec_ld(0, sy);			// v0 = y0|y1|y2|y3|y4|y5|y6|y7|y8|y9|ya|yb|yc|yd|ye|yf
			v2  = vec_ld(0, su);			// v2 = u0|u1|u2|u3|u4|u5|u6|u7|u8|u9|ua|ub|uc|ud|ue|uf
			v4  = vec_ld(0, sv);			// v4 = v0|v1|v2|v3|v4|v5|v6|v7|v8|v9|va|vb|vc|vd|ve|vf
			
			v6  = vec_mergeh(v2, v4);		// v6 = u0|v0|u1|v1|u2|v2|u3|v3|u4|v4|u5|v5|u6|v6|u7|v7
			v7  = vec_mergeh(v0, v6);		// v7 = y0|u0|y1|v0|y2|u1|y3|v1|y4|u2|y5|v2|y6|u3|y7|v3
			
			vec_st(v7, 0, d);				// store
			
			v7  = vec_mergel(v0, v6);		// v7 = y8|u4|y9|v4|ya|u5|yb|v5|yc|u6|yd|v6|ye|u7|yf|v7
			
			vec_st(v7, 16, d);				// store
			
			sy += 16;
			su += 8;
			sv += 8;
			d  += 32;
			dest_dx -= 16;
		}
	}
	else
	{
		vPY = vec_lvsl(0, sy);
		vPU = vec_lvsl(0, su);
		vPV = vec_lvsl(0, sv);
		
		v0  = vec_ld(0, sy);
		v2  = vec_ld(0, su);
		v4  = vec_ld(0, sv);
		
		while (dest_dx >= 32)
		{
			// Load Y - 16 pels
			v1  = vec_ld(16, sy);
			v0  = vec_perm(v0, v1, vPY);	// v0 = y0|y1|y2|y3|y4|y5|y6|y7|y8|y9|ya|yb|yc|yd|ye|yf
			
			// Load U - 16 pels
			v3  = vec_ld(16, su);
			v2  = vec_perm(v2, v3, vPU);	// v2 = u0|u1|u2|u3|u4|u5|u6|u7|u8|u9|ua|ub|uc|ud|ue|uf
			
			// Load V - 16 pels
			v5  = vec_ld(16, sv);
			v4  = vec_perm(v4, v5, vPV);	// v4 = v0|v1|v2|v3|v4|v5|v6|v7|v8|v9|va|vb|vc|vd|ve|vf
			
			v6  = vec_mergeh(v2, v4);		// v6 = u0|v0|u1|v1|u2|v2|u3|v3|u4|v4|u5|v5|u6|v6|u7|v7
			v7  = vec_mergeh(v0, v6);		// v7 = y0|u0|y1|v0|y2|u1|y3|v1|y4|u2|y5|v2|y6|u3|y7|v3
			
			// Store 8 YUYV pels
			vec_st(v7, 0, d);
			
			v7  = vec_mergel(v0, v6);		// v7 = y8|u4|y9|v4|ya|u5|yb|v5|yc|u6|yd|v6|ye|u7|yf|v7
			
			// Store 8 YUYV pels
			vec_st(v7, 16, d);
			
			// Load Y - 16 pels
			v0  = vec_ld(32, sy);
			v1  = vec_perm(v1, v0, vPY);	// v1 = y0|y1|y2|y3|y4|y5|y6|y7|y8|y9|ya|yb|yc|yd|ye|yf
			
			v6  = vec_mergel(v2, v4);		// v6 = u8|v8|u9|v9|ua|va|ub|vb|uc|vc|ud|vd|ue|ve|uf|vf
			v7  = vec_mergeh(v1, v6);		// v7 = y0|u8|y1|v8|y2|u9|y3|v9|y4|ua|y5|va|y6|ub|y7|vb
			
			// Store 8 YUYV pels
			vec_st(v7, 32, d);
			
			v7  = vec_mergel(v1, v6);		// v7 = y8|uc|y9|vc|ya|ud|yb|vd|yc|ue|yd|ve|ye|uf|yf|vf
			
			// Store 8 YUYV pels
			vec_st(v7, 48, d);
			
			v2  = v3;
			v4  = v5;
			
			sy += 32;
			su += 16;
			sv += 16;
			d  += 64;
			dest_dx -= 32;		
		}
		while (dest_dx >= 16)
		{
			// Load Y - 16 pels
			v1  = vec_ld(16, sy);
			v0  = vec_perm(v0, v1, vPY);	// v0 = y0|y1|y2|y3|y4|y5|y6|y7|y8|y9|ya|yb|yc|yd|ye|yf
			
			// Load U - 16 pels
			v3  = vec_ld(16, su);
			v2  = vec_perm(v2, v3, vPU);	// v2 = u0|u1|u2|u3|u4|u5|u6|u7|u8|u9|ua|ub|uc|ud|ue|uf
			
			// Load V - 16 pels
			v5  = vec_ld(16, sv);
			v4  = vec_perm(v4, v5, vPV);	// v4 = v0|v1|v2|v3|v4|v5|v6|v7|v8|v9|va|vb|vc|vd|ve|vf
			
			v6  = vec_mergeh(v2, v4);		// v6 = u0|v0|u1|v1|u2|v2|u3|v3|u4|v4|u5|v5|u6|v6|u7|v7
			v7  = vec_mergeh(v0, v6);		// v7 = y0|u0|y1|v0|y2|u1|y3|v1|y4|u2|y5|v2|y6|u3|y7|v3
			
			// Store 8 YUYV pels
			vec_st(v7, 0, d);
			
			v7  = vec_mergel(v0, v6);		// v7 = y8|u4|y9|v4|ya|u5|yb|v5|yc|u6|yd|v6|ye|u7|yf|v7
			
			// Store 8 YUYV pels
			vec_st(v7, 16, d);
			
			sy += 16;
			su += 8;
			sv += 8;
			d  += 32;
			dest_dx -= 16;		
		}
	}
	while (dest_dx)
	{
		*(U32*)d = COMBINE(sy[0], su[0], sy[1], sv[0]);
		
		sy += 2;
		su++;
		sv++;
		d += 4;
		dest_dx -= 2;
	}
}


#pragma altivec_model off

#endif // _MACINTOSH





























