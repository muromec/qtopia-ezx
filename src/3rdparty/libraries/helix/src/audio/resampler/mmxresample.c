/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmxresample.c,v 1.9 2004/07/09 18:37:31 hubbe Exp $
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

#ifdef _M_IX86

/*
 * Polyphase sampling rate conversion.
 * by Ken Cooke
 */
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
//#include <stdio.h>
#include "allresamplers.h"

/* leave a trace of the C source version in the object code */
static const char VERSION[] = "$Revision: 1.9 $" ;

#define CLIP(s) ((s) > 32767 ? 32767 : ((s) < -32768 ? -32768 : (s)))

#define NWING	48
#define NTAPS	96
typedef short filtwing[NWING];

#define PCMBUFLEN (NBLOCK + 2*NTAPS)
#define ROFFSET (PCMBUFLEN / 2)
#define RBYTEOFF (ROFFSET * 2)

/* precomputed filter tables */
#define HELIX_FEATURE_MMXFILTER_6_1
#define HELIX_FEATURE_MMXFILTER_640_147
#define HELIX_FEATURE_MMXFILTER_3_1
#define HELIX_FEATURE_MMXFILTER_320_147
#define HELIX_FEATURE_MMXFILTER_3_2
#define HELIX_FEATURE_MMXFILTER_160_147
#define HELIX_FEATURE_MMXFILTER_441_80
#define HELIX_FEATURE_MMXFILTER_4_1
#define HELIX_FEATURE_MMXFILTER_441_160
#define HELIX_FEATURE_MMXFILTER_2_1
#define HELIX_FEATURE_MMXFILTER_441_320
#define HELIX_FEATURE_MMXFILTER_147_160

#include "mmxfilters.c"
//extern int updn_list[];
//extern filtwing *filter_list[];
//extern int *nextstate_list[];

/* filter state */
typedef struct {
	int up;
	int dn;
	int nchans;
	int phase;
	int offset;
	short *histbuf;
	short *pcmleft, *pcmrght;
	short *revbuf;
	short *revleft, *revrght;
	short *lwingptr;
	short *rwingptr;
	filtwing *filter;
	int *nextstate;
} state_t;

void *
RAInitResamplerMMX(int inrate, int outrate, int nchans)
{
	state_t *s;
	int divisor, up, dn;
	int idx;

	if (nchans != MONO && nchans != STEREO) {
//		printf("Error: must be mono or stereo\n");
		return NULL;
	}

	/* reduce to smallest fraction */
	divisor = gcd(inrate, outrate);
	up = outrate / divisor;
	dn = inrate / divisor;
//	printf("up = %d, down = %d\n", up, dn);

	/* find corresponding filter in tables */
	for (idx = 0; updn_list[idx]; idx++)
		if (updn_list[idx] == ((up << 16) | dn))
			break;

	if (!updn_list[idx]) {
//		printf("Error: conversion by %d/%d not supported\n", up, dn);
		return NULL;	/* unsupported rates */
	}

	/* malloc buffers */
	s = (state_t *) malloc(sizeof(state_t));
	if (s == NULL)
		return NULL;
	s->histbuf = (short *) calloc(PCMBUFLEN, sizeof(short));
	s->revbuf = (short *) calloc(PCMBUFLEN, sizeof(short));
	if (s->histbuf == NULL || s->revbuf == NULL)
		return NULL;

	/* filter init */
	s->filter = filter_list[idx];
	s->nextstate = nextstate_list[idx];
	s->up = up;
	s->dn = dn;
	s->nchans = nchans;
	s->phase = 0;
	s->offset = 0;
	s->pcmleft = s->histbuf + NTAPS;
	s->revleft = s->revbuf + NBLOCK/2;
	s->pcmrght = s->pcmleft + ROFFSET;
	s->revrght = s->revleft + ROFFSET;
	s->rwingptr = s->filter[0];
	s->lwingptr = s->filter[(s->nextstate[up-1]>>8) & 0xfff];
	
	return (void *) s;
}

void *
RAInitResamplerCopyMMX(int nchans, void *inst)
{
	state_t *s_in = (state_t *)inst;
	state_t *s_out = (state_t *) malloc(sizeof(state_t));

	if (s_in == NULL || s_out == NULL)
		return NULL;

	s_out->histbuf = (short *) calloc(PCMBUFLEN, sizeof(short));
	s_out->revbuf = (short *) calloc(PCMBUFLEN, sizeof(short));
	if (s_out->histbuf == NULL || s_out->revbuf == NULL)
		return NULL;

	/* filter init */
	s_out->filter = s_in->filter;
	s_out->nextstate = s_in->nextstate;
	s_out->up = s_in->up;
	s_out->dn = s_in->dn;
	s_out->nchans = nchans;
	s_out->phase = 0;
	s_out->offset = 0;
	s_out->pcmleft = s_out->histbuf + NTAPS;
	s_out->revleft = s_out->revbuf + NBLOCK/2;
	s_out->pcmrght = s_out->pcmleft + ROFFSET;
	s_out->revrght = s_out->revleft + ROFFSET;
	s_out->rwingptr = s_out->filter[0];
	s_out->lwingptr = s_out->filter[(s_out->nextstate[s_out->up-1]>>8) & 0xfff];

	return s_out ;
}

void
RAFreeResamplerMMX(void *inst)
{
	state_t *s = (state_t *)inst;

	if (s != NULL) {
		if (s->revbuf)
			free(s->revbuf);
		if (s->histbuf)
			free(s->histbuf);
		free(s);
	}
}

int
RAResampleStereoMMX(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst)
{
    state_t *s = (state_t *)inst;
	short *pcmptr, *revptr, *pcmend;
	short *rwingptr, *lwingptr;
	int i, outsamps;
	/* local copies */
	filtwing *filter = s->filter;
	int *nextstate = s->nextstate;
	int phase = s->phase;
	int round13[2] = { 1<<13, 1<<13 };

//	TICK();
//	ASSERT(!(insamps & 0x1));	/* stereo must be even */

	outstride *= sizeof(short) ;

	// hack the state machine to point to pcmleft and pcmright buffers.
	pCvt->pStateMachine[0].incOutput = s->pcmrght - s->pcmleft ;
	pCvt->pStateMachine[1].incOutput = s->pcmleft - s->pcmrght + 1 ;
	insamps = 2*pCvt->pfCvt(s->pcmleft,inbuf,insamps,pCvt->pStateMachine) ;

#if 0
	/* deinterleave new input */
	for (i = 0; i < insamps/2; i++) {
		s->pcmleft[i] = inbuf[0];
		s->pcmrght[i] = inbuf[1];
		inbuf += 2;
	}
#endif
	/* create revbufs */
	for (i = -insamps/2; i < NTAPS; i++) {
		s->revleft[i] = s->pcmleft[-i-1];
		s->revrght[i] = s->pcmrght[-i-1];
	}

	/* restore filter state */
	pcmptr = s->pcmleft + s->offset - (NTAPS-1);
	revptr = s->revleft - s->offset - 1;
	pcmend = s->pcmleft + insamps/2 - (NTAPS-1);
	rwingptr = s->rwingptr;
	lwingptr = s->lwingptr;

	__asm {
			mov		esi, pcmptr
			mov		edi, revptr
			mov		ebx, phase
			mov		ecx, outbuf
			mov		eax, rwingptr
			mov		edx, lwingptr
			movq	mm5, round13

conv_loop:	/* while (pcmptr < pcmend) */
			cmp		esi, pcmend
			jae		conv_done

/* LEFT CHANNEL */

			/* prime */
			movq	mm6, [esi+0]
			pmaddwd	mm6, [eax+0]
			movq	mm1, [edi+0]
			pmaddwd	mm1, [edx+0]

			movq	mm2, [esi+8]
			psrad	mm6, 8
			pmaddwd	mm2, [eax+8]
			//paddd	mm6, mm0
			movq	mm3, [edi+8]
			psrad	mm1, 8
			pmaddwd	mm3, [edx+8]
			paddd	mm6, mm1

			movq	mm0, [esi+16]
			psrad	mm2, 6
			pmaddwd	mm0, [eax+16]
			paddd	mm6, mm2
			movq	mm1, [edi+16]
			psrad	mm3, 6
			pmaddwd	mm1, [edx+16]
			paddd	mm6, mm3

			movq	mm2, [esi+24]
			psrad	mm0, 4
			pmaddwd	mm2, [eax+24]
			paddd	mm6, mm0
			movq	mm3, [edi+24]
			psrad	mm1, 4
			pmaddwd	mm3, [edx+24]
			paddd	mm6, mm1

			movq	mm0, [esi+32]
			psrad	mm2, 3
			pmaddwd	mm0, [eax+32]
			paddd	mm6, mm2
			movq	mm1, [edi+32]
			psrad	mm3, 3
			pmaddwd	mm1, [edx+32]
			paddd	mm6, mm3

			movq	mm2, [esi+40]
			psrad	mm0, 2
			pmaddwd	mm2, [eax+40]
			paddd	mm6, mm0
			movq	mm3, [edi+40]
			psrad	mm1, 2
			pmaddwd	mm3, [edx+40]
			paddd	mm6, mm1

			movq	mm0, [esi+48]
			psrad	mm2, 1
			pmaddwd	mm0, [eax+48]
			paddd	mm6, mm2
			movq	mm1, [edi+48]
			psrad	mm3, 1
			pmaddwd	mm1, [edx+48]
			paddd	mm6, mm3

			/* acc shift */
			psrad	mm6, 8

			movq	mm2, [esi+56]
			psrad	mm0, 8
			pmaddwd	mm2, [eax+56]
			paddd	mm6, mm0
			movq	mm3, [edi+56]
			psrad	mm1, 8
			pmaddwd	mm3, [edx+56]
			paddd	mm6, mm1

			movq	mm0, [esi+64]
			psrad	mm2, 7
			pmaddwd	mm0, [eax+64]
			paddd	mm6, mm2
			movq	mm1, [edi+64]
			psrad	mm3, 7
			pmaddwd	mm1, [edx+64]
			paddd	mm6, mm3

			movq	mm2, [esi+72]
			psrad	mm0, 6
			pmaddwd	mm2, [eax+72]
			paddd	mm6, mm0
			movq	mm3, [edi+72]
			psrad	mm1, 6
			pmaddwd	mm3, [edx+72]
			paddd	mm6, mm1

			movq	mm0, [esi+80]
			psrad	mm2, 5
			pmaddwd	mm0, [eax+80]
			paddd	mm6, mm2
			movq	mm1, [edi+80]
			psrad	mm3, 5
			pmaddwd	mm1, [edx+80]
			paddd	mm6, mm3

			movq	mm2, [esi+88]
			psrad	mm0, 4
			pmaddwd	mm2, [eax+88]
			paddd	mm6, mm0
			movq	mm3, [edi+88]
			psrad	mm1, 4
			pmaddwd	mm3, [edx+88]
			paddd	mm6, mm1

			/* flush */
			psrad	mm2, 1
			paddd	mm6, mm2
			psrad	mm3, 1
			paddd	mm6, mm3

/* RIGHT CHANNEL */

			/* prime */
			movq	mm7, [esi+RBYTEOFF+0]
			pmaddwd	mm7, [eax+0]
			movq	mm1, [edi+RBYTEOFF+0]
			pmaddwd	mm1, [edx+0]

			movq	mm2, [esi+RBYTEOFF+8]
			psrad	mm7, 8
			pmaddwd	mm2, [eax+8]
			//paddd	mm7, mm0
			movq	mm3, [edi+RBYTEOFF+8]
			psrad	mm1, 8
			pmaddwd	mm3, [edx+8]
			paddd	mm7, mm1

			movq	mm0, [esi+RBYTEOFF+16]
			psrad	mm2, 6
			pmaddwd	mm0, [eax+16]
			paddd	mm7, mm2
			movq	mm1, [edi+RBYTEOFF+16]
			psrad	mm3, 6
			pmaddwd	mm1, [edx+16]
			paddd	mm7, mm3

			movq	mm2, [esi+RBYTEOFF+24]
			psrad	mm0, 4
			pmaddwd	mm2, [eax+24]
			paddd	mm7, mm0
			movq	mm3, [edi+RBYTEOFF+24]
			psrad	mm1, 4
			pmaddwd	mm3, [edx+24]
			paddd	mm7, mm1

			movq	mm0, [esi+RBYTEOFF+32]
			psrad	mm2, 3
			pmaddwd	mm0, [eax+32]
			paddd	mm7, mm2
			movq	mm1, [edi+RBYTEOFF+32]
			psrad	mm3, 3
			pmaddwd	mm1, [edx+32]
			paddd	mm7, mm3

			movq	mm2, [esi+RBYTEOFF+40]
			psrad	mm0, 2
			pmaddwd	mm2, [eax+40]
			paddd	mm7, mm0
			movq	mm3, [edi+RBYTEOFF+40]
			psrad	mm1, 2
			pmaddwd	mm3, [edx+40]
			paddd	mm7, mm1

			movq	mm0, [esi+RBYTEOFF+48]
			psrad	mm2, 1
			pmaddwd	mm0, [eax+48]
			paddd	mm7, mm2
			movq	mm1, [edi+RBYTEOFF+48]
			psrad	mm3, 1
			pmaddwd	mm1, [edx+48]
			paddd	mm7, mm3

			/* acc shift */
			psrad	mm7, 8

			movq	mm2, [esi+RBYTEOFF+56]
			psrad	mm0, 8
			pmaddwd	mm2, [eax+56]
			paddd	mm7, mm0
			movq	mm3, [edi+RBYTEOFF+56]
			psrad	mm1, 8
			pmaddwd	mm3, [edx+56]
			paddd	mm7, mm1

			movq	mm0, [esi+RBYTEOFF+64]
			psrad	mm2, 7
			pmaddwd	mm0, [eax+64]
			paddd	mm7, mm2
			movq	mm1, [edi+RBYTEOFF+64]
			psrad	mm3, 7
			pmaddwd	mm1, [edx+64]
			paddd	mm7, mm3

			movq	mm2, [esi+RBYTEOFF+72]
			psrad	mm0, 6
			pmaddwd	mm2, [eax+72]
			paddd	mm7, mm0
			movq	mm3, [edi+RBYTEOFF+72]
			psrad	mm1, 6
			pmaddwd	mm3, [edx+72]
			paddd	mm7, mm1

			movq	mm0, [esi+RBYTEOFF+80]
			psrad	mm2, 5
			pmaddwd	mm0, [eax+80]
			paddd	mm7, mm2
			movq	mm1, [edi+RBYTEOFF+80]
			psrad	mm3, 5
			pmaddwd	mm1, [edx+80]
			paddd	mm7, mm3

			movq	mm2, [esi+RBYTEOFF+88]
			psrad	mm0, 4
			pmaddwd	mm2, [eax+88]
			paddd	mm7, mm0
			movq	mm3, [edi+RBYTEOFF+88]
			psrad	mm1, 4
			pmaddwd	mm3, [edx+88]
			paddd	mm7, mm1

			/* flush */
			psrad	mm2, 1
			paddd	mm7, mm2
			psrad	mm3, 1
			paddd	mm7, mm3

/* BOTH CHANNELS */

			/* horizontal add */
			movq	mm0, mm6
			punpckhdq	mm6, mm7
			punpckldq	mm0, mm7
			paddd	mm6, mm0

			/* round, shift, clip */
			paddd	mm6, mm5
			psrad	mm6, 14
			packssdw mm6, mm6

			/* bump to next state */
			mov		edx, nextstate
			mov		ebx, [edx+4*ebx]	// phase = nextstate[phase]

			lea		edx, [ebx+ebx]
			and		edx, 0xff<<1
			add		esi, edx			// pcmptr += pcmstep
			sub		edi, edx			// revptr -= pcmstep

			mov		edx, ebx
			shr		ebx, 20				// phase

			shr		edx, 3
			and		edx, 0xfff<<5		// 32 * idx
			lea		edx, [edx+2*edx]	// 96 * idx
			add		edx, filter			// lwingptr = filter[idx]

			movd	[ecx], mm6			// *outptr++

//			add		ecx, 4
			add   ecx,outstride

			lea		eax, [ebx+2*ebx]	// 3 * idx
			shl		eax, 5				// 96 * idx
			add		eax, filter			// rwingptr = filter[idx]
			jmp		conv_loop

conv_done:
			sub		ecx, outbuf			// outsamps = outptr - outbuf
			shr		ecx, 1
			mov		outsamps, ecx

			mov		pcmptr, esi
			mov		revptr, edi
			mov		phase, ebx
			mov		rwingptr, eax
			mov		lwingptr, edx
			emms
	}

	/* save filter state */
	s->phase = phase;
	s->offset = pcmptr - pcmend;
	s->rwingptr = rwingptr;
	s->lwingptr = lwingptr;

	memmove(s->pcmleft-NTAPS, s->pcmleft-NTAPS + insamps/2, NTAPS * sizeof(short));
	memmove(s->pcmrght-NTAPS, s->pcmrght-NTAPS + insamps/2, NTAPS * sizeof(short));

//	TOCK(outsamps);
	return outsamps;
}

int
RAResampleMonoMMX(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst)
{
    state_t *s = (state_t *)inst;
	short *pcmptr, *revptr, *pcmend;
	short *rwingptr, *lwingptr;
	int i, outsamps;
	/* local copies */
	filtwing *filter = s->filter;
	int *nextstate = s->nextstate;
	int phase = s->phase;
	int round13[2] = { 1<<13, 1<<13 };

//	TICK();

	outstride *= sizeof(short) ;

	/* copy/convert new input */
	insamps = pCvt->pfCvt(s->pcmleft,inbuf,insamps,pCvt->pStateMachine) ;

	/* create revbuf */
	for (i = -insamps; i < NTAPS; i++)
		s->revrght[i] = s->pcmleft[-i-1];

	/* restore filter state */
	pcmptr = s->pcmleft + s->offset - (NTAPS-1);
	revptr = s->revrght - s->offset - 1;
	pcmend = s->pcmleft + insamps - (NTAPS-1);
	rwingptr = s->rwingptr;
	lwingptr = s->lwingptr;

	__asm {
			mov		esi, pcmptr
			mov		edi, revptr
			mov		ebx, phase
			mov		ecx, outbuf
			mov		eax, rwingptr
			mov		edx, lwingptr
			movq	mm5, round13

conv_loop:	/* while (pcmptr < pcmend) */
			cmp		esi, pcmend
			jae		conv_done

			/* prime */
			movq	mm7, [esi+0]
			pmaddwd	mm7, [eax+0]
			movq	mm1, [edi+0]
			pmaddwd	mm1, [edx+0]

			movq	mm2, [esi+8]
			psrad	mm7, 8
			pmaddwd	mm2, [eax+8]
			//paddd	mm7, mm0
			movq	mm3, [edi+8]
			psrad	mm1, 8
			pmaddwd	mm3, [edx+8]
			paddd	mm7, mm1

			/* kernel */
			movq	mm0, [esi+16]
			psrad	mm2, 6
			pmaddwd	mm0, [eax+16]
			paddd	mm7, mm2
			movq	mm1, [edi+16]
			psrad	mm3, 6
			pmaddwd	mm1, [edx+16]
			paddd	mm7, mm3

			movq	mm2, [esi+24]
			psrad	mm0, 4
			pmaddwd	mm2, [eax+24]
			paddd	mm7, mm0
			movq	mm3, [edi+24]
			psrad	mm1, 4
			pmaddwd	mm3, [edx+24]
			paddd	mm7, mm1
			/* end kernel */

			movq	mm0, [esi+32]
			psrad	mm2, 3
			pmaddwd	mm0, [eax+32]
			paddd	mm7, mm2
			movq	mm1, [edi+32]
			psrad	mm3, 3
			pmaddwd	mm1, [edx+32]
			paddd	mm7, mm3

			movq	mm2, [esi+40]
			psrad	mm0, 2
			pmaddwd	mm2, [eax+40]
			paddd	mm7, mm0
			movq	mm3, [edi+40]
			psrad	mm1, 2
			pmaddwd	mm3, [edx+40]
			paddd	mm7, mm1

			movq	mm0, [esi+48]
			psrad	mm2, 1
			pmaddwd	mm0, [eax+48]
			paddd	mm7, mm2
			movq	mm1, [edi+48]
			psrad	mm3, 1
			pmaddwd	mm1, [edx+48]
			paddd	mm7, mm3

			/* acc shift */
			psrad	mm7, 8

			movq	mm2, [esi+56]
			psrad	mm0, 8
			pmaddwd	mm2, [eax+56]
			paddd	mm7, mm0
			movq	mm3, [edi+56]
			psrad	mm1, 8
			pmaddwd	mm3, [edx+56]
			paddd	mm7, mm1

			movq	mm0, [esi+64]
			psrad	mm2, 7
			pmaddwd	mm0, [eax+64]
			paddd	mm7, mm2
			movq	mm1, [edi+64]
			psrad	mm3, 7
			pmaddwd	mm1, [edx+64]
			paddd	mm7, mm3

			movq	mm2, [esi+72]
			psrad	mm0, 6
			pmaddwd	mm2, [eax+72]
			paddd	mm7, mm0
			movq	mm3, [edi+72]
			psrad	mm1, 6
			pmaddwd	mm3, [edx+72]
			paddd	mm7, mm1

			movq	mm0, [esi+80]
			psrad	mm2, 5
			pmaddwd	mm0, [eax+80]
			paddd	mm7, mm2
			movq	mm1, [edi+80]
			psrad	mm3, 5
			pmaddwd	mm1, [edx+80]
			paddd	mm7, mm3

			movq	mm2, [esi+88]
			psrad	mm0, 4
			pmaddwd	mm2, [eax+88]
			paddd	mm7, mm0
			movq	mm3, [edi+88]
			psrad	mm1, 4
			pmaddwd	mm3, [edx+88]
			paddd	mm7, mm1

			/* flush */
			psrad	mm2, 1
			paddd	mm7, mm2
			psrad	mm3, 1
			paddd	mm7, mm3

			/* horizontal add */
			movq	mm0, mm7
			psrlq	mm7, 32
			paddd	mm7, mm0

			/* round, shift, clip */
			paddd	mm7, mm5
			psrad	mm7, 14
			packssdw mm7, mm7
			movd	eax, mm7

			/* bump to next state */
			mov		edx, nextstate
			mov		ebx, [edx+4*ebx]	// phase = nextstate[phase]

			lea		edx, [ebx+ebx]
			and		edx, 0xff<<1
			add		esi, edx			// pcmptr += pcmstep
			sub		edi, edx			// revptr -= pcmstep

			mov		edx, ebx
			shr		ebx, 20				// phase

			shr		edx, 3
			and		edx, 0xfff<<5		// 32 * idx
			lea		edx, [edx+2*edx]	// 96 * idx
			add		edx, filter			// lwingptr = filter[idx]

			mov		word ptr [ecx], ax	// *outptr++
//			add		ecx, 2
			add   ecx, outstride

			lea		eax, [ebx+2*ebx]	// 3 * idx
			shl		eax, 5				// 96 * idx
			add		eax, filter			// rwingptr = filter[idx]
			jmp		conv_loop

conv_done:
			sub		ecx, outbuf			// outsamps = outptr - outbuf
			shr		ecx, 1
			mov		outsamps, ecx

			mov		pcmptr, esi
			mov		revptr, edi
			mov		phase, ebx
			mov		rwingptr, eax
			mov		lwingptr, edx
			emms
	}

	/* save filter state */
	s->phase = phase;
	s->offset = pcmptr - pcmend;
	s->rwingptr = rwingptr;
	s->lwingptr = lwingptr;

	memmove(s->pcmleft-NTAPS, s->pcmleft-NTAPS + insamps, NTAPS * sizeof(short));

//	TOCK(outsamps);
	return outsamps;
}

#include "mymath.h"

int
RAGetMaxOutputMMX(int insamps, void *inst)
{
    state_t *s = (state_t *)inst;
	int inframes, outframes, outsamps;

	inframes = (insamps + (s->nchans-1)) / s->nchans;
	outframes = (int) myCeil((double)inframes * s->up / s->dn);
	outsamps = outframes * s->nchans;

	return outsamps;
}

int
RAGetMinInputMMX(int outsamps, void *inst)
{
    state_t *s = (state_t *)inst;
	int inframes, outframes, insamps;

	outframes = (outsamps + (s->nchans-1)) / s->nchans;
	inframes = (int) myCeil((double)outframes * s->dn / s->up);
	insamps = inframes * s->nchans;

	return insamps;
}

int
RAGetDelayMMX(void *inst)
{
	state_t *s = (state_t *)inst;
	return (int)(NWING * (float)s->up / s->dn) ;
}

#endif
