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
 * Core filtering functions, ARMv5E version.
 * This version requires an XScale, ARM9E, ARM10E, etc.
 * NOTE: intended to be compiled with ARM ADS1.2 or equivalent.
 */

#include "core.h"

#define ASSERT(x) { while (!(x)) ; }

__inline int MULHI(int a, int b) {
	int low;
	__asm { SMULL low, a, b, a }
	return a;
}

short *
RATCoreMono(short *pcmptr, short *pcmend, short *outptr, state_t *s)
{
	short *rwgptr, *lwgptr, *revptr;
	int acc;
	int i;
	int *tab;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	pcmptr += s->offset;

	/* these must be word-aligned */
	ASSERT(((int)rwgptr & 0x3) == 0);
	ASSERT(((int)lwgptr & 0x3) == 0);

	while (pcmptr < pcmend) {

		revptr = pcmptr - 1;
		acc = 1 << 14;

		/* FIR filter */
		for (i = s->nwing >> 1; i != 0; i--) {
			register int pcm0, pcm1, rev0, rev1, lwg, rwg;
			__asm {
				LDRH	pcm0, [pcmptr],#2
				LDR		lwg,  [lwgptr],#4
				LDRH	pcm1, [pcmptr],#2
				LDRH	rev1, [revptr],#-2
				LDR		rwg,  [rwgptr],#4
				LDRH	rev0, [revptr],#-2

				SMLABB	acc, pcm0, lwg, acc
				SMLABT	acc, pcm1, lwg, acc
				SMLABB	acc, rev1, rwg, acc
				SMLABT	acc, rev0, rwg, acc
			}				
		}
		if (s->nwing & 0x1) {
			register int pcm, rev, lwg, rwg;
			__asm {
				LDRH	pcm, [pcmptr],#2
				LDRH	lwg, [lwgptr],#2
				LDRH	rev, [revptr],#-2
				LDRH	rwg, [rwgptr],#2

				SMLABB	acc, pcm, lwg, acc
				SMLABB	acc, rev, rwg, acc
			}				
		}

		/* saturate */
		__asm { QADD acc, acc, acc }

		*outptr++ = (short) (acc >> 16);

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
	int i;
	int *tab;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	pcmptr += s->offset;

	/* these must be word-aligned */
	ASSERT(((int)rwgptr & 0x3) == 0);
	ASSERT(((int)lwgptr & 0x3) == 0);
	ASSERT(((int)pcmptr & 0x3) == 0);

	while (pcmptr+1 < pcmend) {

		revptr = pcmptr - 2;
		acc0 = acc1 = 1 << 14;

		/* FIR filter */
		for (i = s->nwing >> 1; i != 0; i--) {
			register int pcm0, pcm1, rev0, rev1, lwg, rwg;
			__asm {
				LDR		pcm0, [pcmptr],#4
				LDR		lwg,  [lwgptr],#4
				LDR		pcm1, [pcmptr],#4
				LDR		rev1, [revptr],#-4
				LDR		rwg,  [rwgptr],#4
				LDR		rev0, [revptr],#-4

				SMLABB	acc0, pcm0, lwg, acc0
				SMLATB	acc1, pcm0, lwg, acc1
				SMLABT	acc0, pcm1, lwg, acc0
				SMLATT	acc1, pcm1, lwg, acc1

				SMLATB	acc1, rev1, rwg, acc1
				SMLABB	acc0, rev1, rwg, acc0
				SMLATT	acc1, rev0, rwg, acc1
				SMLABT	acc0, rev0, rwg, acc0
			}
		}
		if (s->nwing & 0x1) {
			register int pcm, rev, lwg, rwg;
			__asm {
				LDR		pcm, [pcmptr],#4
				LDRH	lwg, [lwgptr],#2
				LDR		rev, [revptr],#-4
				LDRH	rwg, [rwgptr],#2

				SMLABB	acc0, pcm, lwg, acc0
				SMLATB	acc1, pcm, lwg, acc1
				SMLATB	acc1, rev, rwg, acc1
				SMLABB	acc0, rev, rwg, acc0
			}
		}

		/* saturate */
		__asm { QADD acc0, acc0, acc0 }
		__asm { QADD acc1, acc1, acc1 }

		*outptr++ = (short) (acc0 >> 16);
		*outptr++ = (short) (acc1 >> 16);

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
	register short *rwgptr, *lwgptr, *revptr;
	register short *rwgptr1, *lwgptr1;
	register int acc0, acc1;
	int pcmstep, i;
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

	/* these must be word-aligned */
	ASSERT(((int)rwgptr & 0x3) == 0);
	ASSERT(((int)lwgptr & 0x3) == 0);
	ASSERT(((int)rwgptr1 & 0x3) == 0);
	ASSERT(((int)lwgptr1 & 0x3) == 0);

	while (pcmptr+pcmstep < pcmend) {

		revptr = pcmptr - 1;
		acc0 = acc1 = 1 << 14;

		if (!pcmstep) {

			for (i = s->nwing >> 1; i != 0; i--) {
				register int pcm0, pcm1, wng0, wng1;
				__asm {
					LDRH	pcm0, [pcmptr],#2
					LDRH	pcm1, [pcmptr],#2
					LDR		wng0, [lwgptr],#4
					LDR		wng1, [lwgptr1],#4

					SMLABB	acc0, pcm0, wng0, acc0
					SMLABB	acc1, pcm0, wng1, acc1
					SMLABT	acc0, pcm1, wng0, acc0
					SMLABT	acc1, pcm1, wng1, acc1

					LDRH	pcm1, [revptr],#-2
					LDRH	pcm0, [revptr],#-2
					LDR		wng0, [rwgptr],#4
					LDR		wng1, [rwgptr1],#4

					SMLABB	acc0, pcm1, wng0, acc0
					SMLABB	acc1, pcm1, wng1, acc1
					SMLABT	acc0, pcm0, wng0, acc0
					SMLABT	acc1, pcm0, wng1, acc1
				}				
			}
			if (s->nwing & 0x1) {
				register int pcm, wng0, wng1;
				__asm {
					LDRH	pcm,  [pcmptr],#2
					LDRH	wng0, [lwgptr],#2
					LDRH	wng1, [lwgptr1],#2

					SMLABB	acc0, pcm, wng0, acc0
					SMLABB	acc1, pcm, wng1, acc1

					LDRH	pcm,  [revptr],#-2
					LDRH	wng0, [rwgptr],#2
					LDRH	wng1, [rwgptr1],#2

					SMLABB	acc0, pcm, wng0, acc0
					SMLABB	acc1, pcm, wng1, acc1
				}				
			}

		} else {

			for (i = s->nwing >> 1; i != 0; i--) {
				register int pcm0, pcm1, pcm2, wng0, wng1;
				__asm {
					LDRH	pcm0, [pcmptr],#2
					LDRH	pcm1, [pcmptr],#2
					LDRH	pcm2, [pcmptr,#0]
					LDR		wng0, [lwgptr],#4
					LDR		wng1, [lwgptr1],#4

					SMLABB	acc0, pcm0, wng0, acc0
					SMLABB	acc1, pcm1, wng1, acc1
					SMLABT	acc0, pcm1, wng0, acc0
					SMLABT	acc1, pcm2, wng1, acc1

					LDRH	pcm2, [revptr,#2]
					LDRH	pcm1, [revptr],#-2
					LDRH	pcm0, [revptr],#-2
					LDR		wng0, [rwgptr],#4
					LDR		wng1, [rwgptr1],#4

					SMLABB	acc0, pcm1, wng0, acc0
					SMLABB	acc1, pcm2, wng1, acc1
					SMLABT	acc0, pcm0, wng0, acc0
					SMLABT	acc1, pcm1, wng1, acc1
				}				
			}
			if (s->nwing & 0x1) {
				register int pcm0, pcm1, wng0, wng1;
				__asm {
					LDRH	pcm0, [pcmptr],#2
					LDRH	pcm1, [pcmptr,#0]
					LDRH	wng0, [lwgptr],#2
					LDRH	wng1, [lwgptr1],#2

					SMLABB	acc0, pcm0, wng0, acc0
					SMLABB	acc1, pcm1, wng1, acc1

					LDRH	pcm1, [revptr,#2]
					LDRH	pcm0, [revptr],#-2
					LDRH	wng0, [rwgptr],#2
					LDRH	wng1, [rwgptr1],#2

					SMLABB	acc0, pcm0, wng0, acc0
					SMLABB	acc1, pcm1, wng1, acc1
				}				
			}
		}

		/* interpolate and saturate */
		acc1 = MULHI(acc1 - acc0, phasef >> 1);
		__asm { QDADD acc0, acc0, acc1 }
		__asm { QADD  acc0, acc0, acc0 }

		*outptr++ = (short) (acc0 >> 16);

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
	int pcmstep, i;
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

	/* these must be word-aligned */
	ASSERT(((int)rwgptr & 0x3) == 0);
	ASSERT(((int)lwgptr & 0x3) == 0);
	ASSERT(((int)rwgptr1 & 0x3) == 0);
	ASSERT(((int)lwgptr1 & 0x3) == 0);
	ASSERT(((int)pcmptr & 0x3) == 0);

	while (pcmptr+pcmstep+1 < pcmend) {

		revptr = pcmptr - 2;
		acc0 = acc1 = acc2 = acc3 = 1 << 14;

		if (!pcmstep) {

			for (i = s->nwing; i != 0; i--) {
				register int pcm, wng, tmp;
				__asm {
					LDRH	wng, [lwgptr],#2
					LDRH	tmp, [lwgptr1],#2
					LDR		pcm, [pcmptr],#4
					ORR		wng, wng, tmp,LSL#16

					SMLABB	acc0, pcm, wng, acc0
					SMLATB	acc1, pcm, wng, acc1
					SMLABT	acc2, pcm, wng, acc2
					SMLATT	acc3, pcm, wng, acc3

					LDRH	wng, [rwgptr],#2
					LDRH	tmp, [rwgptr1],#2
					LDR		pcm, [revptr],#-4
					ORR		wng, wng, tmp,LSL#16

					SMLABB	acc0, pcm, wng, acc0
					SMLATB	acc1, pcm, wng, acc1
					SMLABT	acc2, pcm, wng, acc2
					SMLATT	acc3, pcm, wng, acc3
				}
			}

		} else {

			for (i = s->nwing; i != 0; i--) {
				register int pcm0, pcm1, wng, tmp;
				__asm {
					LDRH	wng,  [lwgptr],#2
					LDRH	tmp,  [lwgptr1],#2
					LDR		pcm0, [pcmptr],#4
					LDR		pcm1, [pcmptr,#0]
					ORR		wng, wng, tmp,LSL#16

					SMLABB	acc0, pcm0, wng, acc0
					SMLATB	acc1, pcm0, wng, acc1
					SMLABT	acc2, pcm1, wng, acc2
					SMLATT	acc3, pcm1, wng, acc3

					LDRH	wng,  [rwgptr],#2
					LDRH	tmp,  [rwgptr1],#2
					LDR		pcm1, [revptr,#4]
					LDR		pcm0, [revptr],#-4
					ORR		wng, wng, tmp,LSL#16

					SMLABB	acc0, pcm0, wng, acc0
					SMLATB	acc1, pcm0, wng, acc1
					SMLABT	acc2, pcm1, wng, acc2
					SMLATT	acc3, pcm1, wng, acc3
				}
			}
		}

		/* interpolate and saturate */
		acc2 = MULHI(acc2 - acc0, phasef >> 1);
		__asm { QDADD acc0, acc0, acc2 }
		__asm { QADD  acc0, acc0, acc0 }

		acc3 = MULHI(acc3 - acc1, phasef >> 1);
		__asm { QDADD acc1, acc1, acc3 }
		__asm { QADD  acc1, acc1, acc1 }

		*outptr++ = (short) (acc0 >> 16);
		*outptr++ = (short) (acc1 >> 16);

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
