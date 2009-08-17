/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 
/*
 * Fixed-point sampling rate conversion library
 * Developed by Ken Cooke (kenc@real.com)
 * May 2003
 *
 * Core filtering functions, generic version.
 * This version is fairly optimized for ARMv4.
 */

#include "core.h"

#if defined (_M_IX86) || (defined (_SYMBIAN) && defined (__WINS__))
__inline int MULHI(int a, int b) {
	__asm mov	eax, a
	__asm imul	b
	__asm mov	eax, edx
}

#elif defined (__CC_ARM)
__inline int MULHI(int a, int b) {
	int low;
	__asm { smull low, a, b, a }
	return a;
}

#else
/* a fast MULHI is important, so verify this compiles well */
#define MULHI(a,b) ((int)(((__int64)(a) * (__int64)(b)) >> 32))
#endif

short *
RATCoreMono(short *pcmptr, short *pcmend, short *outptr, state_t *s)
{
	short *rwgptr, *lwgptr, *revptr;
	int acc;
	int i, sign;
	int *tab;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	pcmptr += s->offset;

	while (pcmptr < pcmend) {

		revptr = pcmptr - 1;
		acc = 1 << 14;

		/* FIR filter */
		for (i = s->nwing >> 1; i != 0; i--) {
			acc += (*pcmptr++) * (*lwgptr++);
			acc += (*pcmptr++) * (*lwgptr++);
			acc += (*revptr--) * (*rwgptr++);
			acc += (*revptr--) * (*rwgptr++);
		}
		if (s->nwing & 0x1) {
			acc += (*pcmptr++) * (*lwgptr++);
			acc += (*revptr--) * (*rwgptr++);
		}
		acc >>= 15;

		/* saturate */
		if ((sign = (acc >> 31)) != (acc >> 15))
			acc = sign ^ ((1<<15)-1);

		*outptr++ = (short) acc;

		/* step phase by N */
		tab = (rwgptr > s->stepNptr ? s->stepNbak : s->stepNfwd);
		rwgptr += tab[0];
		lwgptr += tab[1];
		pcmptr += tab[2];
	}

	s->offset = pcmptr - pcmend;
	s->rwing = rwgptr;
	s->lwing = lwgptr;

	return outptr;
}

short *
RATCoreStereo(short *pcmptr, short *pcmend, short *outptr, state_t *s)
{
	short *rwgptr, *lwgptr, *revptr;
	int acc0, acc1;
	int i, sign;
	int *tab;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	pcmptr += s->offset;

	while (pcmptr+1 < pcmend) {

		revptr = pcmptr - 1;
		acc0 = acc1 = 1 << 14;

		/* FIR filter */
		for (i = s->nwing >> 1; i != 0; i--) {
			acc0 += (*pcmptr++) * (*lwgptr);
			acc1 += (*pcmptr++) * (*lwgptr++);
			acc0 += (*pcmptr++) * (*lwgptr);
			acc1 += (*pcmptr++) * (*lwgptr++);
			acc1 += (*revptr--) * (*rwgptr);
			acc0 += (*revptr--) * (*rwgptr++);
			acc1 += (*revptr--) * (*rwgptr);
			acc0 += (*revptr--) * (*rwgptr++);
		}
		if (s->nwing & 0x1) {
			acc0 += (*pcmptr++) * (*lwgptr);
			acc1 += (*pcmptr++) * (*lwgptr++);
			acc1 += (*revptr--) * (*rwgptr);
			acc0 += (*revptr--) * (*rwgptr++);
		}
		acc0 >>= 15;
		acc1 >>= 15;

		/* saturate */
		if ((sign = (acc0 >> 31)) != (acc0 >> 15))
			acc0 = sign ^ ((1<<15)-1);
		if ((sign = (acc1 >> 31)) != (acc1 >> 15))
			acc1 = sign ^ ((1<<15)-1);

		*outptr++ = (short) acc0;
		*outptr++ = (short) acc1;

		/* step phase by N */
		tab = (rwgptr > s->stepNptr ? s->stepNbak : s->stepNfwd);
		rwgptr += tab[0];
		lwgptr += tab[1];
		pcmptr += tab[2];
	}

	s->offset = pcmptr - pcmend;
	s->rwing = rwgptr;
	s->lwing = lwgptr;

	return outptr;
}

short *
ARBCoreMono(short *pcmptr, short *pcmend, short *outptr, state_t *s)
{
	short *rwgptr, *lwgptr, *revptr;
	short *rwgptr1, *lwgptr1;
	int acc0, acc1;
	int pcmstep, i, sign;
	int *tab;
	uint phasef;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	phasef = s->phasef;
	pcmptr += s->offset;

	/* phase+1 */
	tab = (rwgptr >= s->step1ptr ? s->step1bak : s->step1fwd);
	rwgptr1 = rwgptr + tab[0];
	lwgptr1 = lwgptr + tab[1];
	pcmstep = tab[2];

	while (pcmptr+pcmstep < pcmend) {

		revptr = pcmptr - 1;
		acc0 = acc1 = 1 << 14;

		if (!pcmstep) {

			for (i = s->nwing; i != 0; i--) {
				register short pcm, rev;

				pcm = (*pcmptr++);
				rev = (*revptr--);

				acc0 += pcm * (*lwgptr++);
				acc1 += pcm * (*lwgptr1++);
				acc0 += rev * (*rwgptr++);
				acc1 += rev * (*rwgptr1++);
			}

		} else {

			for (i = s->nwing; i != 0; i--) {
				register short pcm0, pcm1;

				pcm0 = (*pcmptr++);
				pcm1 = *(pcmptr+0);

				acc0 += pcm0 * (*lwgptr++);
				acc1 += pcm1 * (*lwgptr1++);

				pcm1 = *(revptr+1);
				pcm0 = (*revptr--);

				acc0 += pcm0 * (*rwgptr++);
				acc1 += pcm1 * (*rwgptr1++);
			}
		}

		/* interpolate */
		acc0 = (acc0 >> 1) + MULHI(acc1 - acc0, phasef >> 1);
		acc0 >>= 14;

		/* saturate */
		if ((sign = (acc0 >> 31)) != (acc0 >> 15))
			acc0 = sign ^ ((1<<15)-1);

		*outptr++ = (short) acc0;

		/* step phase fraction */
		phasef += s->stepf;
		if (phasef < s->stepf) {
			rwgptr = rwgptr1;
			lwgptr = lwgptr1;
			pcmptr += pcmstep;
		}

		/* step phase by N */
		tab = (rwgptr > s->stepNptr ? s->stepNbak : s->stepNfwd);
		rwgptr += tab[0];
		lwgptr += tab[1];
		pcmptr += tab[2];

		/* phase+1 */
		tab = (rwgptr >= s->step1ptr ? s->step1bak : s->step1fwd);
		rwgptr1 = rwgptr + tab[0];
		lwgptr1 = lwgptr + tab[1];
		pcmstep = tab[2];
	}

	s->offset = pcmptr - pcmend;
	s->rwing = rwgptr;
	s->lwing = lwgptr;
	s->phasef = phasef;

	return outptr;
}

short *
ARBCoreStereo(short *pcmptr, short *pcmend, short *outptr, state_t *s)
{
	register short *rwgptr, *lwgptr, *revptr;
	register short *rwgptr1, *lwgptr1;
	register int acc0, acc1, acc2, acc3;
	int pcmstep, i, sign;
	int *tab;
	uint phasef;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	phasef = s->phasef;
	pcmptr += s->offset;

	/* phase+1 */
	tab = (rwgptr >= s->step1ptr ? s->step1bak : s->step1fwd);
	rwgptr1 = rwgptr + tab[0];
	lwgptr1 = lwgptr + tab[1];
	pcmstep = tab[2];

	while (pcmptr+pcmstep+1 < pcmend) {

		revptr = pcmptr - 1;
		acc0 = acc1 = acc2 = acc3 = 1 << 14;

		if (!pcmstep) {

			for (i = s->nwing; i != 0; i--) {
				register short pcm0, pcm1;

				pcm0 = (*pcmptr++);
				pcm1 = (*pcmptr++);

				acc0 += pcm0 * (*lwgptr);
				acc1 += pcm1 * (*lwgptr++);
				acc2 += pcm0 * (*lwgptr1);
				acc3 += pcm1 * (*lwgptr1++);

				pcm1 = (*revptr--);
				pcm0 = (*revptr--);

				acc3 += pcm1 * (*rwgptr1);
				acc2 += pcm0 * (*rwgptr1++);
				acc1 += pcm1 * (*rwgptr);
				acc0 += pcm0 * (*rwgptr++);
			}

		} else {

			for (i = s->nwing; i != 0; i--) {	
				acc0 += (*pcmptr++) * (*lwgptr);
				acc1 += (*pcmptr++) * (*lwgptr++);
				acc2 += *(pcmptr+0) * (*lwgptr1);
				acc3 += *(pcmptr+1) * (*lwgptr1++);

				acc3 += *(revptr+2) * (*rwgptr1);
				acc2 += *(revptr+1) * (*rwgptr1++);
				acc1 += (*revptr--) * (*rwgptr);
				acc0 += (*revptr--) * (*rwgptr++);
			}
		}

		/* interpolate */
		acc0 = (acc0 >> 1) + MULHI(acc2 - acc0, phasef >> 1);
		acc0 >>= 14;
		acc1 = (acc1 >> 1) + MULHI(acc3 - acc1, phasef >> 1);
		acc1 >>= 14;

		/* saturate */
		if ((sign = (acc0 >> 31)) != (acc0 >> 15))
			acc0 = sign ^ ((1<<15)-1);
		if ((sign = (acc1 >> 31)) != (acc1 >> 15))
			acc1 = sign ^ ((1<<15)-1);

		*outptr++ = (short) acc0;
		*outptr++ = (short) acc1;

		/* step phase fraction */
		phasef += s->stepf;
		if (phasef < s->stepf) {
			rwgptr = rwgptr1;
			lwgptr = lwgptr1;
			pcmptr += pcmstep;
		}

		/* step phase by N */
		tab = (rwgptr > s->stepNptr ? s->stepNbak : s->stepNfwd);
		rwgptr += tab[0];
		lwgptr += tab[1];
		pcmptr += tab[2];

		/* phase+1 */
		tab = (rwgptr >= s->step1ptr ? s->step1bak : s->step1fwd);
		rwgptr1 = rwgptr + tab[0];
		lwgptr1 = lwgptr + tab[1];
		pcmstep = tab[2];
	}

	s->offset = pcmptr - pcmend;
	s->rwing = rwgptr;
	s->lwing = lwgptr;
	s->phasef = phasef;

	return outptr;
}
