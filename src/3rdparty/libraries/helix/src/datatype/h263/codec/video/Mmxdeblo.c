/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: Mmxdeblo.c,v 1.2 2004/07/09 18:32:15 hubbe Exp $
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

/*
#ifdef __ICL
#pragma message ("Attention: Intel Compiler")
#else
#pragma message ("Attention: Non Intel Compiler")
#endif
*/
//disable no emms warning
#pragma warning(disable:4799)


//#include <string.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "h263plus.h"

#if defined(COMPILE_MMX)
#if (_MSC_VER>=1100)


// 4 * short  
extern __int64  g_qp;		
extern __int64  g_max_qp;	// max - pq
extern __int64  g_max_2qp;	// max - 2 * pq

void ApplyHorizontalDeblockingFilterMMX( PIXEL * top, PIXEL * bottom, int offset)
{
	//__m64 mm0, mm1, mm2, mm3, mm4, mm5, mm6;
	
    PIXEL *next_to_top;
    PIXEL *next_after_bottom;

    top += 7*offset;

    next_to_top = top - offset;

    next_after_bottom = bottom + offset;


	//_m_empty();
__asm {
	//unpack next_to_top -> short unsigned
	mov	eax, next_to_top
	pxor mm7, mm7; // mm7 == 0

	movq mm0, [eax]
	mov	eax, next_after_bottom;//--

	movq mm1, mm0
	punpcklbw mm0, mm7

	movq mm4, [eax];//--
	punpckhbw mm1, mm7

	movq mm5, mm4
	punpcklbw mm4, mm7

	mov	ebx, bottom;//--

	//unpack next_after_bottom -> short unsigned
	punpckhbw mm5, mm7
	//

	//next_after_bottom - next_after_bottom
	psubw mm0, mm4
	psubw mm1, mm5

	//multiply mm0,mm1 by 3
	movq mm2, mm0
	movq mm3, mm1

	psllw mm2, 1
	psllw mm3, 1

	paddw mm0, mm2
	paddw mm1, mm3

	/* values * 8 */
	//unpack bottom -> short unsigned
	movq mm4, [ebx]
	mov	ecx, top;//--


	movq mm5, mm4
	punpcklbw mm4, mm7

	movq mm2, [ecx];//--
	//

	punpckhbw mm5, mm7
	//

	//unpack top -> short unsigned
	movq mm3, mm2
	punpcklbw mm2, mm7

	punpckhbw mm3, mm7
	//

	//save top
	movq [ebx], mm2
	movq [ecx], mm3

	//bottom stays in mm4,mm5
	//top - bottom
	psubw mm2, mm4 
	psubw mm3, mm5 
	
	//multiply by 8
	psllw mm2, 3
	psllw mm3, 3

	//accumulate *3 - *8 
	psubw mm0, mm2
	psubw mm1, mm3

	//divide by 16
	psraw mm0, 4
	psraw mm1, 4

	/** DiffCutoff **/
	//cmp g_bBigDiffCutoffTable
	/** d = limit(-qp, 2*d, qp) - limit(-qp, d, qp) **/
	//d -> 2*d
	//single argument: d
	movq mm2, mm0
	movq mm3, mm1

	psllw mm0, 1;//--
	movq mm6, g_max_qp
	//

	//double argument: 2*d
	psllw mm1, 1

	/* limit to [-qp, +qp] */
	//add (max-qp) and saturate signed for upper limit
	paddsw mm0, mm6
	paddsw mm2, mm6

	movq mm7, g_max_2qp
	//

	paddsw mm1, mm6
	paddsw mm3, mm6


	//subtract (max-2qp) and saturate unsigned for lower limit
	psubusw mm0, mm7
	psubusw mm2, mm7

	movq mm6, g_qp
	//

	psubusw mm1, mm7
	psubusw mm3, mm7

	//correct: subtract (qp)
	psubw mm0, mm6
	psubw mm2, mm6

	psubw mm1, mm6
	psubw mm3, mm6

	//d = ... - ...
	psubw mm0, mm2
	psubw mm1, mm3

	//subtract from bottom
	//still in mm4,mm5
	psubw mm4, mm0
	psubw mm5, mm1

	paddw mm0, [ebx];//--

	//clip bottom
	packuswb mm4, mm5;//--

	//add to top
	paddw mm1, [ecx]

	movq [ebx], mm4;//--
	//clip top
	//convert word -> unsigned byte with saturation [0; 255]
	packuswb mm0, mm1


	//write back
	movq [ecx], mm0

	}
}



////////////////////////////////////////////////////////////////////////////////
void ApplyVerticalDeblockingFilterMMX( PIXEL * left, PIXEL * right, int offset)
{
	//left[-1, 0, -1+offset, 0+offset, ..., -1+7*offset, 0+8*offset]
	//right[0, +1, 0+offset, +1+offset, ..., 0+7*offset, +1+8*offset]

	//get left[-1, -1+offset, -1+2*offset, -1+3*offset]

	
    //PIXEL *next_to_top, *top, *bottom, *next_after_bottom;

	__int64 qw0, qw1;
	__int64 qw2, qw3;
	PIXEL *tleft_1, *t2left, *tright, *t2right;
	S32 ii;

    left += 7;

	tleft_1 = left - 1;
	t2left = left;
	tright = right;
	t2right = right;

	//rearranging data into vectors of 8 bytes

	//arrange qw1 == h1h0 d1d0 f1f0 b1b0 and qw0 == g1g0 c1c0 e1e0 a1a0

	*((U32 *)&(((U16 *)&qw0)[0])) = *((U32 *)(tleft_1));//a
	tleft_1 += offset;

	*((U32 *)&(((U16 *)&qw1)[0])) = *((U32 *)(tleft_1));//b
	tleft_1 += offset;

	*((U32 *)&(((U16 *)&qw0)[2])) = *((U32 *)(tleft_1));//c
	tleft_1 += offset;
	*((U16 *)&(((U16 *)&qw1)[2])) = *((U16 *)(tleft_1));//d
	tleft_1 += offset;

	*((U16 *)&(((U16 *)&qw0)[1])) = *((U16 *)(tleft_1));//e
	tleft_1 += offset;
	*((U16 *)&(((U16 *)&qw1)[1])) = *((U16 *)(tleft_1));//f
	tleft_1 += offset;
	*((U16 *)&(((U16 *)&qw0)[3])) = *((U16 *)(tleft_1));//g
	tleft_1 += offset;
	*((U16 *)&(((U16 *)&qw1)[3])) = *((U16 *)(tleft_1));//h

	//arrange qw3 == h1h0 d1d0 f1f0 b1b0 and qw2 == g1g0 c1c0 e1e0 a1a0
	*((U32 *)&(((U16 *)&qw2)[0])) = *((U32 *)(tright));//a
	tright += offset;
	*((U32 *)&(((U16 *)&qw3)[0])) = *((U32 *)(tright));//b
	tright += offset;

	*((U32 *)&(((U16 *)&qw2)[2])) = *((U32 *)(tright));//c
	tright += offset;
	*((U16 *)&(((U16 *)&qw3)[2])) = *((U16 *)(tright));//d
	tright += offset;

	*((U16 *)&(((U16 *)&qw2)[1])) = *((U16 *)(tright));//e
	tright += offset;
	*((U16 *)&(((U16 *)&qw3)[1])) = *((U16 *)(tright));//f
	tright += offset;
	*((U16 *)&(((U16 *)&qw2)[3])) = *((U16 *)(tright));//g
	tright += offset;
	*((U16 *)&(((U16 *)&qw3)[3])) = *((U16 *)(tright));//h
	

__asm {

	/////////////////////////////////////////////////
	movq mm0, [qw2]
	movq mm1, [qw3]
	movq mm4, mm0
	//mm1==h1h0d1d0f1f0b1b0 and mm0==g1g0c1c0e1e0a1a0
	// to 
	//mm4==h1g1h0g0d1c1d0c0 and mm0==f1e1f0e0b1a1b0a0
	punpcklbw mm0, mm1
	punpckhbw mm4, mm1

	//mm4==h1g1h0g0d1c1d0c0 and mm0==f1e1f0e0b1a1b0a0
	// to 
	//mm1==h1g1f1e1h0g0f0e0 and mm0==d1c1b1a1d0c0b0a0
	movq mm1, mm0
	punpcklwd mm0, mm4
	punpckhwd mm1, mm4

	//mm1==h1g1f1e1h0g0f0e0 and mm0==d1c1b1a1d0c0b0a0
	// to 
	//mm4==h1g1f1e1d1c1b1a1==next_after_bottom and mm0==h0g0f0e0d0c0b0a0==bottom
	movq mm4, mm0
	punpckldq mm0, mm1
	punpckhdq mm4, mm1
	movq [qw2], mm0;//save bottom


	/////////////////////////////////////////////////
	movq mm0, [qw0]
	movq mm1, [qw1]
	movq mm6, mm0
	//mm1==h1h0d1d0f1f0b1b0 and mm0==g1g0c1c0e1e0a1a0
	// to 
	//mm6==h1g1h0g0d1c1d0c0 and mm0==f1e1f0e0b1a1b0a0
	punpcklbw mm0, mm1
	punpckhbw mm6, mm1

	//mm6==h1g1h0g0d1c1d0c0 and mm0==f1e1f0e0b1a1b0a0
	// to 
	//mm1==h1g1f1e1h0g0f0e0 and mm0==d1c1b1a1d0c0b0a0
	movq mm1, mm0
	punpcklwd mm0, mm6
	punpckhwd mm1, mm6

	//mm1==h1g1f1e1h0g0f0e0 and mm0==d1c1b1a1d0c0b0a0
	// to 
	//mm6==h1g1f1e1d1c1b1a1==top and mm0==h0g0f0e0d0c0b0a0==next_to_top
	movq mm6, mm0
	punpckldq mm0, mm1
	punpckhdq mm6, mm1

	/////////////////////////////////////////////////
	//unpack next_to_top -> short unsigned -> mm0
	pxor mm7, mm7; // mm7 == 0

	//mov	eax, next_after_bottom;//--

	movq mm1, mm0
	punpcklbw mm0, mm7

	//movq mm4, [eax];//--
	punpckhbw mm1, mm7

	movq mm5, mm4
	punpcklbw mm4, mm7

	//mov	ebx, bottom;//--

	//unpack next_after_bottom -> short unsigned
	punpckhbw mm5, mm7
	//

	//next_after_bottom - next_after_bottom
	psubw mm0, mm4
	psubw mm1, mm5

	//multiply mm0,mm1 by 3
	movq mm2, mm0
	movq mm3, mm1

	psllw mm2, 1
	psllw mm3, 1

	paddw mm0, mm2
	paddw mm1, mm3

	/* values * 8 */
	//unpack bottom -> short unsigned
	movq mm4, [qw2]
	//movq mm4, [ebx]
	//mov	ecx, top;//--


	movq mm5, mm4
	punpcklbw mm4, mm7

	//movq mm2, [ecx];//--
	//

	punpckhbw mm5, mm7
	//

	//unpack top -> short unsigned
	movq mm3, mm6
	punpcklbw mm6, mm7

	punpckhbw mm3, mm7
	//

	//save top
	movq [qw0], mm6
	movq [qw1], mm3

	//bottom stays in mm4,mm5
	//top - bottom
	psubw mm6, mm4 
	psubw mm3, mm5 
	
	//multiply by 8
	psllw mm6, 3
	psllw mm3, 3

	//accumulate *3 - *8 
	psubw mm0, mm6
	psubw mm1, mm3

	//divide by 16
	psraw mm0, 4
	psraw mm1, 4


	/** DiffCutoff **/
	//cmp g_bBigDiffCutoffTable
	/** d = limit(-qp, 2*d, qp) - limit(-qp, d, qp) **/
	//d -> 2*d
	//single argument: d
	movq mm2, mm0
	movq mm3, mm1

	psllw mm0, 1;//--
	movq mm6, g_max_qp
	//

	//double argument: 2*d
	psllw mm1, 1

	/* limit to [-qp, +qp] */
	//add (max-qp) and saturate signed for upper limit
	paddsw mm0, mm6
	paddsw mm2, mm6

	movq mm7, g_max_2qp
	//

	paddsw mm1, mm6
	paddsw mm3, mm6


	//subtract (max-2qp) and saturate unsigned for lower limit
	psubusw mm0, mm7
	psubusw mm2, mm7

	movq mm6, g_qp
	//

	psubusw mm1, mm7
	psubusw mm3, mm7

	//correct: subtract (qp)
	psubw mm0, mm6
	psubw mm2, mm6

	psubw mm1, mm6
	psubw mm3, mm6

	//d = ... - ...
	psubw mm0, mm2
	psubw mm1, mm3

	//subtract from bottom
	//still in mm4,mm5
	psubw mm4, mm0
	psubw mm5, mm1

	paddw mm0, [qw0];//--

	//clip bottom
	packuswb mm4, mm5;//--

	//add to top
	paddw mm1, [qw1]


	//write back bottom
	movq [qw0], mm4;//--
	//clip top
	//convert word -> unsigned byte with saturation [0; 255]
	packuswb mm0, mm1


	//write back top
	movq [qw1], mm0

	}


	//rearrange data back
	for(ii=0; ii<8; ii+=2, t2left+=offset, t2right+=offset) {
		*t2right = ((U8 *)&qw0)[ii];
		*t2left = ((U8 *)&qw1)[ii];
		
		//unroll loop
		t2left+=offset;
		t2right+=offset;

		*t2right = ((U8 *)&qw0)[ii+1];
		*t2left = ((U8 *)&qw1)[ii+1];
		
	}


}

#endif
#endif


//default no emms warning
#pragma warning(default:4799)
