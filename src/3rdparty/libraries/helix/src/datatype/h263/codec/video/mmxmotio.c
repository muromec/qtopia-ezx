/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmxmotio.c,v 1.3 2004/07/09 18:32:15 hubbe Exp $
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

#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"

extern int  g_saveMbHor;      // Hold on to size of Mb array (used by distUmv and
extern int  g_saveMbVert;     //  TryInter4V and its subroutines)
extern int  g_saveUmvMode;    // Hold on to unrestrictedMv (used by RefineMotionVector)
extern int  g_savePointOutside;   // Hold on to pointOutside (used by rmvDist)
extern int  g_distPixels;     // Number of pixels used in squared error computations





static const __int64 const_0x00ff00ff00ff00ff = 0x00ff00ff00ff00ff;		//0	2 4	6
static const __int64 const_0x000000ff000000ff = 0x000000ff000000ff;//0	4
static const __int64 const_0x00ff000000ff0000 = 0x00ff000000ff0000;//2	6
static const __int64 const_0x00ff00ff000000ff = 0x00ff00ff000000ff;	//0	 4	6 == shift 4 8 10

static const __int64 const_0x0001000100010001 = 0x0001000100010001;
static const __int64 const_0x0002000200020002 = 0x0002000200020002;


/*-- mcomp.c -----------------------------------------------------------------------------------*/

static void mc16pelsNoInterpolMMX( PIXEL *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	mov		ecx, vSize
	mov	ebx, hdim
	mov		esi, inpix
	neg		ebx
	mov		edi, outpix
mc16pelsNoInterpolMMX_loop:
		add		ebx, hdim
		dec		ecx
		movq	mm0, [esi + ebx]
		movq	mm1, [esi + ebx + 8]
		movq	[edi + ebx], mm0
		movq	[edi + ebx + 8], mm1
	jg	mc16pelsNoInterpolMMX_loop
	emms		
    }
	/*
    union { // Copy words to speed up routine
        PIXEL   *pix;
        U32     *word;
    } pIn, pOut;

    pIn.pix = inpix;
    pOut.pix = outpix;
    while (vSize > 0) {
        *(pOut.word + 0) = *(pIn.word + 0);
        *(pOut.word + 1) = *(pIn.word + 1);
        *(pOut.word + 2) = *(pIn.word + 2);
        *(pOut.word + 3) = *(pIn.word + 3);
        pIn.pix += hdim;
        pOut.pix += hdim;
        --vSize;
    }
	*/
}


static void mc8pelsNoInterpolMMX( PIXEL *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	mov		ecx, vSize
	mov	ebx, hdim
	mov		esi, inpix
	neg		ebx
	mov		edi, outpix
mc8pelsNoInterpolMMX_loop:
		add		ebx, hdim
		dec		ecx
		movq	mm0, [esi + ebx]
		movq	[edi + ebx], mm0
	jg	mc8pelsNoInterpolMMX_loop
	emms		
    }
/*
	union { // Copy words to speed up routine
        PIXEL   *pix;
        U32     *word;
    } pIn, pOut;
	
	pIn.pix = inpix;
    pOut.pix = outpix;
    while (vSize > 0) {
        *(pOut.word + 0) = *(pIn.word + 0);
        *(pOut.word + 1) = *(pIn.word + 1);
        pIn.pix += hdim;
        pOut.pix += hdim;
        --vSize;
    }
*/
}


static void mc4pelsNoInterpolMMX( PIXEL *inpix, PIXEL *outpix, int hdim, int vSize )
{
	int rowcount = 0;
    while (vSize > 0) {
         *((U32 *)(outpix + rowcount)) = *((U32 *)(inpix + rowcount));
		rowcount +=  hdim;
		vSize--;
    }
}


static void mc16pelsHorInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{

__asm {
	xor		ebx, ebx
	mov		ecx, vSize
	mov		esi, inpix
	mov		edi, outpix
	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0001000100010001
mc16pelsHorInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		movq			mm4, [esi + ebx +8]
		movq			mm3, mm1	;//save
		movq			mm5, mm4	;//save
		psrlq			mm3, 8			;//shift one byte
		movq			mm2, mm1
		psllq			mm4, 56			;//shift 7 bytes, LSB is in top pos of mm4
		punpcklbw	mm1, mm0	;//expand lower 4 pix	pos: + 0
		pxor			mm3, mm4	;//equivalent to read from +1
		punpckhbw	mm2, mm0	;//expand higher 4 pix pos: + 4

		movq			mm4, mm3
		punpcklbw	mm3, mm0	;//expand lower 4 pix pos: + 1
		paddw		mm1, mm7		;//add 1
		punpckhbw	mm4, mm0	;//expand higher 4 pix pos: + 5

		paddw		mm2, mm7		;//add 1

		paddw		mm1, mm3		;//pos: +0/+1	add
		paddw		mm2, mm4		;//pos: +4/+5	add


		psrlw			mm1, 1			;//pos: +0/+1	/2
		movq			mm6, mm5
		psrlw			mm2, 1			;//pos: +4/+5	/2
		packuswb	mm1, mm2
		movq			mm3, mm5	;//save

		punpcklbw	mm5, mm0	;//expand lower 4 pix pos: + 8
		movq	[edi + ebx], mm1
		punpckhbw	mm6, mm0	;//expand higher 4 pix pos: + 12

		movq			mm4, [esi + ebx + 16]
		psrlq			mm3, 8			;//shift one byte
		paddw		mm5, mm7		;//add 1
		psllq			mm4, 56			;//shift 7 bytes, LSB is in top pos of mm4
		paddw		mm6, mm7		;//add 1
		pxor			mm3, mm4	;//equivalent to read from +9
		movq			mm4, mm3
		punpcklbw	mm3, mm0	;//expand lower 4 pix pos: + 9
		punpckhbw	mm4, mm0	;//expand higher 4 pix pos: + 13
		

		paddw		mm5, mm3		;//pos: +8/+9	add
		paddw		mm6, mm4		;//pos: +12/+13	add

		psrlw			mm5, 1			;//pos: +8/+9	/2
		psrlw			mm6, 1			;//pos: +12/+13	/2
		packuswb	mm5, mm6

		movq	[edi + ebx + 8], mm5

		add		ebx, hdim
		dec		ecx
	jg	mc16pelsHorInterpolMMX_loop
	emms		
}

/*
    while (vSize > 0) {
        out[0] = (in[0] + in[1] + 1) >> 1;
        out[1] = (in[1] + in[2] + 1) >> 1;
        out[2] = (in[2] + in[3] + 1) >> 1;
        out[3] = (in[3] + in[4] + 1) >> 1;
        out[4] = (in[4] + in[5] + 1) >> 1;
        out[5] = (in[5] + in[6] + 1) >> 1;
        out[6] = (in[6] + in[7] + 1) >> 1;
        out[7] = (in[7] + in[8] + 1) >> 1;
        out[8] = (in[8] + in[9] + 1) >> 1;
        out[9] = (in[9] + in[10] + 1) >> 1;
        out[10] = (in[10] + in[11] + 1) >> 1;
        out[11] = (in[11] + in[12] + 1) >> 1;
        out[12] = (in[12] + in[13] + 1) >> 1;
        out[13] = (in[13] + in[14] + 1) >> 1;
        out[14] = (in[14] + in[15] + 1) >> 1;
        out[15] = (in[15] + in[16] + 1) >> 1;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}

static void mc8pelsHorInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		ecx, vSize
	mov		esi, inpix
	mov		edi, outpix
	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0001000100010001
mc8pelsHorInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		movq			mm4, [esi + ebx +8]
		movq			mm3, mm1
		psllq			mm4, 56			;//shift 7 bytes, LSB is in top pos of mm4
		movq			mm2, mm1
		psrlq			mm3, 8			;//shift one byte
		punpcklbw	mm1, mm0	;//expand lower 4 pix
		pxor			mm3, mm4	;
		punpckhbw	mm2, mm0	;//expand higher 4 pix

		movq			mm4, mm3
		punpcklbw	mm3, mm0	;//expand lower 4 pix
		paddw		mm1, mm7
		punpckhbw	mm4, mm0	;//expand higher 4 pix

		paddw		mm2, mm7

		paddw		mm1, mm3
		paddw		mm2, mm4

		psrlw			mm1, 1			
		psrlw			mm2, 1			
		packuswb	mm1, mm2

		movq	[edi + ebx], mm1
		add		ebx, hdim
		dec		ecx
	jg	mc8pelsHorInterpolMMX_loop
	emms		
	}

/*
    while (vSize > 0) {
        out[0] = (in[0] + in[1] + 1) >> 1;
        out[1] = (in[1] + in[2] + 1) >> 1;
        out[2] = (in[2] + in[3] + 1) >> 1;
        out[3] = (in[3] + in[4] + 1) >> 1;
        out[4] = (in[4] + in[5] + 1) >> 1;
        out[5] = (in[5] + in[6] + 1) >> 1;
        out[6] = (in[6] + in[7] + 1) >> 1;
        out[7] = (in[7] + in[8] + 1) >> 1;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}


static void mc4pelsHorInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		ecx, vSize
	mov		esi, inpix
	mov		edi, outpix
	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0001000100010001
mc4pelsHorInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		movq			mm3, mm1
		punpcklbw	mm1, mm0	;//expand lower 4 pix

		psrlq			mm3, 8			;//shift one byte
		punpcklbw	mm3, mm0	;//expand lower 4 pix

		paddw		mm1, mm7

		paddw		mm1, mm3

		psrlw			mm1, 1			
		packuswb	mm1, mm0

		movd	[edi + ebx], mm1
		add		ebx, hdim
		dec		ecx
	jg	mc4pelsHorInterpolMMX_loop
	emms		
	}

/*
    while (vSize > 0) {
        out[0] = (in[0] + in[1] + 1) >> 1;
        out[1] = (in[1] + in[2] + 1) >> 1;
        out[2] = (in[2] + in[3] + 1) >> 1;
        out[3] = (in[3] + in[4] + 1) >> 1;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}


static void mc16pelsVertInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		ecx, vSize
	mov		esi, inpix

	mov		edi, outpix
	sub		edi, hdim		;//edi = output - hdim

	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0001000100010001
mc16pelsVertInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		movq			mm2, mm1
		movq			mm5, [esi + ebx + 8]
		punpcklbw	mm1, mm0	;//expand lower 4 pix
		movq			mm6, mm5
		punpckhbw	mm2, mm0	;//expand higher 4 pix

		add		ebx, hdim
		punpcklbw	mm5, mm0	;//expand lower 4 pix
		paddw		mm1, mm7
		punpckhbw	mm6, mm0	;//expand higher 4 pix


		movq			mm3, [esi + ebx]
		paddw		mm2, mm7
		movq			mm4, mm3
		punpcklbw	mm3, mm0	;//expand lower 4 pix
		punpckhbw	mm4, mm0	;//expand higher 4 pix

		paddw		mm1, mm3
		paddw		mm2, mm4

		psrlw			mm1, 1			
		psrlw			mm2, 1			
		movq			mm3, [esi + ebx + 8]
		packuswb	mm1, mm2

		movq			mm4, mm3
		movq	[edi + ebx], mm1

		punpcklbw	mm3, mm0	;//expand lower 4 pix
		paddw		mm5, mm7
		punpckhbw	mm4, mm0	;//expand higher 4 pix

		paddw		mm6, mm7

		paddw		mm5, mm3
		paddw		mm6, mm4

		psrlw			mm5, 1			
		psrlw			mm6, 1			
		packuswb	mm5, mm6

		dec		ecx
		movq	[edi + ebx + 8], mm5

	jg	mc16pelsVertInterpolMMX_loop
	emms		
	}

/*
    while (vSize > 0) {
        out[0] = (in[0] + in[hdim+0] + 1) >> 1;
        out[1] = (in[1] + in[hdim+1] + 1) >> 1;
        out[2] = (in[2] + in[hdim+2] + 1) >> 1;
        out[3] = (in[3] + in[hdim+3] + 1) >> 1;
        out[4] = (in[4] + in[hdim+4] + 1) >> 1;
        out[5] = (in[5] + in[hdim+5] + 1) >> 1;
        out[6] = (in[6] + in[hdim+6] + 1) >> 1;
        out[7] = (in[7] + in[hdim+7] + 1) >> 1;
        out[8] = (in[8] + in[hdim+8] + 1) >> 1;
        out[9] = (in[9] + in[hdim+9] + 1) >> 1;
        out[10] = (in[10] + in[hdim+10] + 1) >> 1;
        out[11] = (in[11] + in[hdim+11] + 1) >> 1;
        out[12] = (in[12] + in[hdim+12] + 1) >> 1;
        out[13] = (in[13] + in[hdim+13] + 1) >> 1;
        out[14] = (in[14] + in[hdim+14] + 1) >> 1;
        out[15] = (in[15] + in[hdim+15] + 1) >> 1;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}


static void mc8pelsVertInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		ecx, vSize
	mov		esi, inpix

	mov		edi, outpix
	sub		edi, hdim		;//edi = output - hdim

	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0001000100010001
mc8pelsVertInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		add		ebx, hdim
		movq			mm2, mm1
		punpcklbw	mm1, mm0	;//expand lower 4 pix
		movq			mm3, [esi + ebx]
		punpckhbw	mm2, mm0	;//expand higher 4 pix

		movq			mm4, mm3
		punpcklbw	mm3, mm0	;//expand lower 4 pix
		punpckhbw	mm4, mm0	;//expand higher 4 pix

		paddw		mm1, mm7
		paddw		mm2, mm7

		paddw		mm1, mm3
		paddw		mm2, mm4

		psrlw			mm1, 1			
		psrlw			mm2, 1			
		dec		ecx
		packuswb	mm1, mm2

		movq	[edi + ebx], mm1
	jg	mc8pelsVertInterpolMMX_loop
	emms		
	}

/*
    while (vSize > 0) {
        out[0] = (in[0] + in[hdim+0] + 1) >> 1;
        out[1] = (in[1] + in[hdim+1] + 1) >> 1;
        out[2] = (in[2] + in[hdim+2] + 1) >> 1;
        out[3] = (in[3] + in[hdim+3] + 1) >> 1;
        out[4] = (in[4] + in[hdim+4] + 1) >> 1;
        out[5] = (in[5] + in[hdim+5] + 1) >> 1;
        out[6] = (in[6] + in[hdim+6] + 1) >> 1;
        out[7] = (in[7] + in[hdim+7] + 1) >> 1;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}


static void mc4pelsVertInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		ecx, vSize
	mov		esi, inpix

	mov		edi, outpix
	sub		edi, hdim		;//edi = output - hdim

	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0001000100010001
mc4pelsVertInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		punpcklbw	mm1, mm0	;//expand lower 4 pix
		add		ebx, hdim

		movq			mm3, [esi + ebx]
		punpcklbw	mm3, mm0	;//expand lower 4 pix

		paddw		mm1, mm7

		paddw		mm1, mm3

		psrlw			mm1, 1			
		packuswb	mm1, mm0

		movd	[edi + ebx], mm1
		dec		ecx
	jg	mc4pelsVertInterpolMMX_loop
	emms		
	}
/*
    while (vSize > 0) {
        out[0] = (in[0] + in[hdim+0] + 1) >> 1;
        out[1] = (in[1] + in[hdim+1] + 1) >> 1;
        out[2] = (in[2] + in[hdim+2] + 1) >> 1;
        out[3] = (in[3] + in[hdim+3] + 1) >> 1;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}


static void mc16pels2DInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		edx, hdim		;//one row ahead
	mov		ecx, vSize
	mov		esi, inpix
	mov		edi, outpix
	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0002000200020002
mc16pels2DInterpolMMX_loop:
		movq			mm1, [esi + ebx]									;//00
		movq			mm6, [esi + ebx +8]									;//05
		movq			mm5, mm1									;//01
		movq			mm2, mm1									;//02
		psrlq			mm5, 8			;//shift one byte									;//06
		psllq			mm6, 56			;//shift 7 bytes, LSB is in top pos of mm4									;//07
		punpcklbw	mm1, mm0	;//expand lower 4 pix									;//03
		pxor			mm5, mm6	;									;//08
		punpckhbw	mm2, mm0	;//expand higher 4 pix									;//04

		movq			mm6, mm5									;//09
		punpcklbw	mm5, mm0	;//expand lower 4 pix									;//10
		paddw		mm1, mm7		;//add 2									;//12
		punpckhbw	mm6, mm0	;//expand higher 4 pix									;//11

		paddw		mm2, mm7		;//add 2									;//12

		paddw		mm1, mm5									;//13
		movq			mm3, [esi + edx]									;//15
		paddw		mm2, mm6									;//14

		movq			mm5, mm3									;//16
		movq			mm4, mm3									;//18
		punpcklbw	mm3, mm0	;//expand lower 4 pix									;//18
		movq			mm6, [esi + edx + 8]									;//22
		psrlq			mm5, 8			;//shift one byte									;//23
		punpckhbw	mm4, mm0	;//expand higher 4 pix									;//19

		psllq			mm6, 56			;//shift 7 bytes, LSB is in top pos of mm4									;//24
		paddw		mm1, mm3									;//20
		pxor			mm5, mm6	;									;//25
		paddw		mm2, mm4									;//21

		movq			mm6, mm5									;//26
		punpcklbw	mm5, mm0	;//expand lower 4 pix									;//27
		punpckhbw	mm6, mm0	;//expand higher 4 pix									;//28

		paddw		mm1, mm5									;//29
		paddw		mm2, mm6									;//30

		psrlw			mm1, 2									;//31
		psrlw			mm2, 2											;//32
		movq			mm6, [esi + ebx + 16]									;//40
		packuswb	mm1, mm2											;//33

		psllq			mm6, 56			;//shift 7 bytes, LSB is in top pos of mm4								;//42
		movq	[edi + ebx], mm1											;//34

		//-----------------//--------------//----------------
		movq			mm1, [esi + ebx + 8]									;//35
		movq			mm5, mm1									;//36
		movq			mm2, mm1									;//37
		psrlq			mm5, 8			;//shift one byte											;//41
		punpcklbw	mm1, mm0	;//expand lower 4 pix									;//38
		pxor			mm5, mm6	;												;//43
		punpckhbw	mm2, mm0	;//expand higher 4 pix									;//39

		movq			mm6, mm5													;//44
		punpcklbw	mm5, mm0	;//expand lower 4 pix							;//45
		punpckhbw	mm6, mm0	;//expand higher 4 pix							;//46

		paddw		mm1, mm7		;//add 2							;//47
		paddw		mm2, mm7		;//add 2							;//48

		paddw		mm1, mm5							;//49
		paddw		mm2, mm6							;//50

		movq			mm3, [esi + edx + 8]							;//51
		movq			mm6, [esi + edx + 16]							;//58
		movq			mm5, mm3							;//52
		psllq			mm6, 56			;//shift 7 bytes, LSB is in top pos of mm4							;//60
		movq			mm4, mm3							;//53
		psrlq			mm5, 8			;//shift one byte							;//59
		punpcklbw	mm3, mm0	;//expand lower 4 pix							;//54
		pxor			mm5, mm6	;							;//61
		punpckhbw	mm4, mm0	;//expand higher 4 pix							;//55

		paddw		mm1, mm3							;//56
		paddw		mm2, mm4							;//57

		movq			mm6, mm5							;//62
		punpcklbw	mm5, mm0	;//expand lower 4 pix							;//63
		punpckhbw	mm6, mm0	;//expand higher 4 pix							;//64

		paddw		mm1, mm5							;//65
		paddw		mm2, mm6							;//66

		psrlw			mm1, 2										;//67
		psrlw			mm2, 2										;//68
		add		edx, hdim
		packuswb	mm1, mm2							;//69

		movq	[edi + ebx + 8], mm1						;//70


		add		ebx, hdim
		dec		ecx
	jg	mc16pels2DInterpolMMX_loop
	emms		
	}
/*
    while (vSize > 0) {
        out[0] = (in[0] + in[1] + in[hdim+0] + in[hdim+1] + 2) >> 2;
        out[1] = (in[1] + in[2] + in[hdim+1] + in[hdim+2] + 2) >> 2;
        out[2] = (in[2] + in[3] + in[hdim+2] + in[hdim+3] + 2) >> 2;
        out[3] = (in[3] + in[4] + in[hdim+3] + in[hdim+4] + 2) >> 2;
        out[4] = (in[4] + in[5] + in[hdim+4] + in[hdim+5] + 2) >> 2;
        out[5] = (in[5] + in[6] + in[hdim+5] + in[hdim+6] + 2) >> 2;
        out[6] = (in[6] + in[7] + in[hdim+6] + in[hdim+7] + 2) >> 2;
        out[7] = (in[7] + in[8] + in[hdim+7] + in[hdim+8] + 2) >> 2;
        out[8] = (in[8] + in[9] + in[hdim+8] + in[hdim+9] + 2) >> 2;
        out[9] = (in[9] + in[10] + in[hdim+9] + in[hdim+10] + 2) >> 2;
        out[10] = (in[10] + in[11] + in[hdim+10] + in[hdim+11] + 2) >> 2;
        out[11] = (in[11] + in[12] + in[hdim+11] + in[hdim+12] + 2) >> 2;
        out[12] = (in[12] + in[13] + in[hdim+12] + in[hdim+13] + 2) >> 2;
        out[13] = (in[13] + in[14] + in[hdim+13] + in[hdim+14] + 2) >> 2;
        out[14] = (in[14] + in[15] + in[hdim+14] + in[hdim+15] + 2) >> 2;
        out[15] = (in[15] + in[16] + in[hdim+15] + in[hdim+16] + 2) >> 2;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}


static void mc8pels2DInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		edx, hdim		;//one row ahead
	mov		ecx, vSize
	mov		esi, inpix
	mov		edi, outpix
	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0002000200020002
mc8pels2DInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		movq			mm6, [esi + ebx +8]
		movq			mm5, mm1
		movq			mm2, mm1
		psrlq			mm5, 8			;//shift one byte
		punpcklbw	mm1, mm0	;//expand lower 4 pix
		psllq			mm6, 56			;//shift 7 bytes, LSB is in top pos of mm4
		punpckhbw	mm2, mm0	;//expand higher 4 pix

		pxor			mm5, mm6	;
		paddw		mm1, mm7		;//add 2
		movq			mm6, mm5
		punpcklbw	mm5, mm0	;//expand lower 4 pix
		paddw		mm2, mm7		;//add 2
		punpckhbw	mm6, mm0	;//expand higher 4 pix

		paddw		mm1, mm5
		paddw		mm2, mm6

		movq			mm3, [esi + edx]
		movq			mm6, [esi + edx + 8]
		movq			mm5, mm3
		movq			mm4, mm3
		punpcklbw	mm3, mm0	;//expand lower 4 pix
		psrlq			mm5, 8			;//shift one byte
		punpckhbw	mm4, mm0	;//expand higher 4 pix
		psllq			mm6, 56			;//shift 7 bytes, LSB is in top pos of mm4

		paddw		mm1, mm3
		pxor			mm5, mm6	;
		paddw		mm2, mm4

		movq			mm6, mm5
		punpcklbw	mm5, mm0	;//expand lower 4 pix
		punpckhbw	mm6, mm0	;//expand higher 4 pix

		paddw		mm1, mm5
		paddw		mm2, mm6

		psrlw			mm1, 2			
		psrlw			mm2, 2			
		add		edx, hdim
		packuswb	mm1, mm2

		movq	[edi + ebx], mm1
		add		ebx, hdim
		dec		ecx
	jg	mc8pels2DInterpolMMX_loop
	emms		
	}

/*
    while (vSize > 0) {
        out[0] = (in[0] + in[1] + in[hdim+0] + in[hdim+1] + 2) >> 2;
        out[1] = (in[1] + in[2] + in[hdim+1] + in[hdim+2] + 2) >> 2;
        out[2] = (in[2] + in[3] + in[hdim+2] + in[hdim+3] + 2) >> 2;
        out[3] = (in[3] + in[4] + in[hdim+3] + in[hdim+4] + 2) >> 2;
        out[4] = (in[4] + in[5] + in[hdim+4] + in[hdim+5] + 2) >> 2;
        out[5] = (in[5] + in[6] + in[hdim+5] + in[hdim+6] + 2) >> 2;
        out[6] = (in[6] + in[7] + in[hdim+6] + in[hdim+7] + 2) >> 2;
        out[7] = (in[7] + in[8] + in[hdim+7] + in[hdim+8] + 2) >> 2;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}


static void mc4pels2DInterpolMMX( PIXEL const *inpix, PIXEL *outpix, int hdim, int vSize )
{
__asm {
	xor		ebx, ebx
	mov		edx, hdim		;//one row ahead
	mov		ecx, vSize
	mov		esi, inpix
	mov		edi, outpix
	pxor	mm0, mm0	;// mm0 = 0
	movq		mm7, const_0x0002000200020002
mc4pels2DInterpolMMX_loop:
		movq			mm1, [esi + ebx]
		movq			mm5, mm1
		punpcklbw	mm1, mm0	;//expand lower 4 pix

		psrlq			mm5, 8			;//shift one byte
		punpcklbw	mm5, mm0	;//expand lower 4 pix

		paddw		mm1, mm7		;//add 2

		paddw		mm1, mm5

		movq			mm3, [esi + edx]
		movq			mm5, mm3
		punpcklbw	mm3, mm0	;//expand lower 4 pix

		paddw		mm1, mm3

		psrlq			mm5, 8			;//shift one byte
		punpcklbw	mm5, mm0	;//expand lower 4 pix

		paddw		mm1, mm5

		psrlw			mm1, 2			
		packuswb	mm1, mm0

		movd	[edi + ebx], mm1
		add		ebx, hdim
		add		edx, hdim
		dec		ecx
	jg	mc4pels2DInterpolMMX_loop
	emms		
	}





/*
    while (vSize > 0) {
        out[0] = (in[0] + in[1] + in[hdim+0] + in[hdim+1] + 2) >> 2;
        out[1] = (in[1] + in[2] + in[hdim+1] + in[hdim+2] + 2) >> 2;
        out[2] = (in[2] + in[3] + in[hdim+2] + in[hdim+3] + 2) >> 2;
        out[3] = (in[3] + in[4] + in[hdim+3] + in[hdim+4] + 2) >> 2;
        in += hdim;
        out += hdim;
        --vSize;
    }
    return;
*/
}




// mc - Perform motion compensation for a hSize x vSize block
void mcMMX( int hSize, int vSize, 
                    PIXEL *in, PIXEL *out, int hdim,
                    int mvX, int mvY    // Motion vector
                    )
{
    int intX, intY, fracX, fracY;    

    intX = mvX >> 1;    // Integer part of motion vector
    intY = mvY >> 1;
    fracX = mvX & 0x1;  // Fractional part of motion vector
    fracY = mvY & 0x1;
    in += intX + intY * hdim;
    if (hSize != 16  &&  hSize != 8  &&  hSize != 4) {
        H261ErrMsg("mc -- hSize not supported");
        return;
    }

//#define MMX_MC_TEST
#ifdef MMX_MC_TEST
	{
	PIXEL in_org[17*352];
	PIXEL out_org[17*352];
	void mc( int hSize, int vSize, PIXEL in[], PIXEL out[], int hdim,int mvX, int mvY);
	int	bError = 0;

	int row, col;
	for(row=0; row<vSize+1; row++) {
		for(col=0; col<hSize+1; col++) {
			in_org[row*hdim + col] = in[row*hdim + col];
			//out_org[row*hdim + col] = out[row*hdim + col];
		}
	}
#endif


    if (fracY == 0) {
        if (fracX == 0) {
            // No interpolation
            if (hSize == 8) {
                mc8pelsNoInterpolMMX( in, out, hdim, vSize );
            } else if (hSize == 16) {
                mc16pelsNoInterpolMMX( in, out, hdim, vSize );
            } else {
                mc4pelsNoInterpolMMX( in, out, hdim, vSize );
            }
        } else {
            // Horizontal interpolation
            if (hSize == 8) {
                mc8pelsHorInterpolMMX( in, out, hdim, vSize );
            } else if (hSize == 16) {
                mc16pelsHorInterpolMMX( in, out, hdim, vSize );
            } else {
                mc4pelsHorInterpolMMX( in, out, hdim, vSize );
            }
        }
    } else if (fracX == 0) {
        // Vertical interpolation
        if (hSize == 8) {
            mc8pelsVertInterpolMMX( in, out, hdim, vSize );
        } else if (hSize == 16) {
            mc16pelsVertInterpolMMX( in, out, hdim, vSize );
        } else {
            mc4pelsVertInterpolMMX( in, out, hdim, vSize );
        }
    } else {    // Bilinear interpolation
        if (hSize == 8) {
            mc8pels2DInterpolMMX( in, out, hdim, vSize );
        } else if (hSize == 16) {
            mc16pels2DInterpolMMX( in, out, hdim, vSize );
        } else {
            mc4pels2DInterpolMMX( in, out, hdim, vSize );
        }
    }

#ifdef MMX_MC_TEST
	mc(hSize,vSize, in_org - intX - intY * hdim, out_org, hdim, mvX, mvY );
	for(row=0; row<vSize; row++) {
		for(col=0; col<hSize; col++) {
			if(in_org[row*hdim + col] != in[row*hdim + col]) {
				bError = 1;
			}
			if(out_org[row*hdim + col] != out[row*hdim + col]) {
				bError = 1;
			}
		}
	}

	}
#endif

    return;
}



// averageBlock - compute average of two hSize*vSize pixel arrays
void averageBlockMMX( PIXEL forPred[], int hSize, int vSize, int forOffset,
                          PIXEL backPred[], int backOffset )
{
    int row, col;
	int	nbPerRow = (hSize>>2) ;

	//do row size multiples of 4
	if(nbPerRow==2) {
		__asm {//8
		mov				esi, forPred
		mov				edi, backPred
		mov				ecx, vSize
		pxor			mm0, mm0;		// mm0 = 0
aver_8:
			movq			mm1, [esi]		
			movq			mm3, [edi]		
			movq			mm2, mm1
			movq			mm4, mm3
			punpcklbw	mm1, mm0	;// expand lower 4 pix
			punpckhbw	mm2, mm0	;// expand higher 4 pix
			punpcklbw	mm3, mm0	;// expand lower 4 pix
			punpckhbw	mm4, mm0	;// expand higher 4 pix
			paddw			mm1, mm3
			paddw			mm2, mm4
			psrlw			mm1, 1
			psrlw			mm2, 1
			packuswb	mm1, mm2
			add				edi, backOffset
			movq			[esi], mm1
			add				esi, forOffset
			dec				ecx
		jg aver_8
		emms
		}
	} else if(nbPerRow>=4) {
		nbPerRow = 4;
		__asm {//16
		mov				esi, forPred
		mov				edi, backPred
		mov				ecx, vSize
		pxor			mm0, mm0;		// mm0 = 0
aver_16:
			movq			mm1, [esi]	;//00
			movq			mm5, [esi+8]			;//10
			movq			mm2, mm1	;//00
			punpcklbw	mm1, mm0	;// expand lower 4 pix	;//00
			movq			mm6, mm5			;//10
			punpckhbw	mm2, mm0	;// expand higher 4 pix	;//00

			movq			mm3, [edi]			;//00
			punpcklbw	mm5, mm0	;// expand lower 4 pix			;//10
			movq			mm4, mm3	;//00
			punpckhbw	mm6, mm0	;// expand higher 4 pix			;//10
			movq			mm7, [edi+8]					;//10
			punpcklbw	mm3, mm0	;// expand lower 4 pix	;//00
			punpckhbw	mm4, mm0	;// expand higher 4 pix	;//00
			paddw			mm2, mm4	;//00
			paddw			mm1, mm3	;//00
			movq			mm4, mm7			;//10
			punpcklbw	mm7, mm0	;// expand lower 4 pix			;//10
			punpckhbw	mm4, mm0	;// expand higher 4 pix			;//10
			paddw			mm5, mm7			;//10
			psrlw			mm1, 1	;//00
			paddw			mm6, mm4			;//10
			psrlw			mm2, 1	;//00
			psrlw			mm5, 1			;//10
			packuswb	mm1, mm2	;//00
			psrlw			mm6, 1			;//10
			packuswb	mm5, mm6			;//10
			movq			[esi], mm1	;//00
			add				edi, backOffset
			movq			[esi+8], mm5			;//10
			add				esi, forOffset
			dec				ecx
		jg aver_16
		emms
		}
	} else if(nbPerRow==1) {
		__asm {//4
		mov				esi, forPred
		mov				edi, backPred
		mov				ecx, vSize
		pxor			mm0, mm0;		// mm0 = 0
aver_4:
			movq			mm1, [esi]		
			movq			mm3, [edi]		
			punpcklbw	mm1, mm0	;// expand lower 4 pix
			punpcklbw	mm3, mm0	;// expand lower 4 pix
			paddw			mm1, mm3
			psrlw			mm1, 1
			packuswb	mm1, mm0
			add				edi, backOffset
			movd			[esi], mm1
			add				esi, forOffset
			dec				ecx
		jg aver_4
		emms
		}
	} else if(nbPerRow==3) {
		__asm {//12
		mov				esi, forPred
		mov				edi, backPred
		mov				ecx, vSize
		pxor			mm0, mm0;		// mm0 = 0
aver_12:
			movq			mm1, [esi]	;//00
			movq			mm5, [esi+8]			;//10
			movq			mm2, mm1	;//00
			punpcklbw	mm1, mm0	;// expand lower 4 pix	;//00
			punpckhbw	mm2, mm0	;// expand higher 4 pix	;//00
			movq			mm3, [edi]			;//00
			punpcklbw	mm5, mm0	;// expand lower 4 pix			;//10
			movq			mm4, mm3	;//00
			punpcklbw	mm3, mm0	;// expand lower 4 pix	;//00
			punpckhbw	mm4, mm0	;// expand higher 4 pix	;//00
			movq			mm7, [edi+8]					;//10
			paddw			mm2, mm4	;//00
			punpcklbw	mm7, mm0	;// expand lower 4 pix			;//10
			paddw			mm1, mm3	;//00
			psrlw			mm2, 1	;//00
			paddw			mm5, mm7			;//10
			psrlw			mm1, 1	;//00
			psrlw			mm5, 1			;//10
			packuswb	mm1, mm2	;//00
			packuswb	mm5, mm0			;//10
			movq			[esi], mm1	;//00
			add				edi, backOffset
			movd			[esi+8], mm5			;//10
			add				esi, forOffset
			dec				ecx
		jg aver_12
		emms
		}
	}
	
    for (row = 0; row < vSize; ++row) {
        for (col = nbPerRow<<2; col < hSize; ++col) {//from smaller multiple of 4
            forPred[col] = (forPred[col] + backPred[col]) >> 1;
        }
        forPred  += forOffset;
        backPred += backOffset;
    }
}
