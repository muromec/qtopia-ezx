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
 * Main resampling functions.
 */

#ifdef _OPENWAVE
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#else
#include <stdlib.h>
#include <string.h>
#endif
#include "resample.h"
#include "core.h"
#include "filter.h"

#ifndef _OPENWAVE
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_RATE	192000
#define MAX_CHANS	2

#define UP_MAX		640	/* ARB is enabled when up > UP_MAX */
#define UP_ARB		128	/* ARB oversampling factor */

#define NWING_ALIGN 2	/* 2 required for ARMv5E, otherwise 1 */

/* unsigned 32x32->hi32, using mulhi */
#define UMULHI(a,b) (uint)(mulhi(a, b) + (((int)a>>31) & b) + (((int)b>>31) & a));

/* 
__inline int mulhi(int a, int b) {
	return (int) (((__int64)a * (__int64)b) >> 32);
}
__inline uint udivhi(uint num, uint den) {
	return (uint) (((unsigned __int64)num << 32) / den);
}
*/

/* signed 32x32->hi32 */
static int mulhi(int a, int b)
{
	int ah, bh, lh, hl, hh;
	uint al, bl, ll;

	al = a & 0xffff;
	ah = a >> 16;
	bl = b & 0xffff;
	bh = b >> 16;

	ll = al * bl;
	lh = al * bh + (ll >> 16);
	hl = ah * bl + (lh & 0xffff);
	hh = ah * bh + (lh >> 16) + (hl >> 16);

	return hh;
}

/* unsigned (num<<32)/den, undefined for overflow */
static uint udivhi(uint num, uint den)
{
	uint i, carry, quo = 0;

	/* bitwise shift-and-subtract */
	for (i = 0; i < 32; i++) {
		carry = (int)num >> 31;
		num <<= 1;
		quo <<= 1;
		if ((carry | num) >= den) {
			num -= den;
			quo += 1;
		}
	}
	return quo;
}

/* greatest common divisor */
static int gcd(int a, int b)
{
	while (a != b) {
		if (a > b)
			a -= b;
		else
			b -= a;
	}
	return a;
}

static void
Interpolate(int *inbuf, int inlen, int *outbuf, int outlen, int gain)
{
	uint stepi, stepf, halfstepi, halfstepf, f;
	int x0, x1, x2, x3, x0_3, x3_3, acc, frac;
	int i, j, sign;

	stepi = inlen / outlen;
	stepf = udivhi(inlen - stepi * outlen, outlen);

	/* i.f = 0.5 + step/2 */
	halfstepf = (stepi << 31) | (stepf >> 1);
	halfstepi = stepi >> 1;
	f = 0x80000000 + halfstepf;
	i = halfstepi + (f < halfstepf);	/* add with carry */

	for (j = 0; j < outlen; j++) {

		x3 = (i-2 < 0 ? inbuf[-i+1] >> 2 : inbuf[i-2] >> 2);	/* reflect */
		x2 = (i-1 < 0 ? inbuf[-i+0] >> 2 : inbuf[i-1] >> 2);	/* reflect */
		x1 = (i+0 < inlen ? inbuf[i+0] >> 2 : 0);				/* zero pad */
		x0 = (i+1 < inlen ? inbuf[i+1] >> 2 : 0);				/* zero pad */

		x0_3 = mulhi(x0, 0x55555556);	/* x0/3 */
		x3_3 = mulhi(x3, 0x55555556);	/* x3/3 */
		frac = f >> 1;

		/*
		 * 4-tap Lagrange interpolation using Farrow structure.
		 * Requires 2 guardbits to prevent overflow.
		 */
		acc = (x0_3 - x3_3 + x2 - x1) >> 1;
		acc = mulhi(acc, frac) << 1;
		acc += ((x1 + x3) >> 1) - x2;
		acc = mulhi(acc, frac) << 1;
		acc += x1 - x3_3 - ((x2 + x0_3) >> 1);
		acc = mulhi(acc, frac) << 1;
		acc += x2;
		acc = mulhi(acc, gain) << 1;

		f += stepf;
		i += stepi + (f < stepf);	/* add with carry */

		/* clip to 30-bit */
		if ((sign = (acc >> 31)) != (acc >> 29))
			acc = sign ^ ((1<<29)-1);

		outbuf[j] = acc << 2;
	}
}

static short *
MakeFilter(int up, int dn, int quality, int *pnwing)
{
	int nwing, nwingk, i, phase, gain;
	int nkernel, nfilter, nwinga;
	int *kernel, *filtbuf;
	short *filter;

	/* choose the filter kernel */
	switch (quality) {
	case 0:
		kernel = (int *)kernel8;
		nwingk = 4;
		break;
	case 1:
		kernel = (int *)kernel16;
		nwingk = 8;
		break;
	case 2:
		kernel = (int *)kernel24;
		nwingk = 12;
		break;
	case 3:
		kernel = (int *)kernel32;
		nwingk = 16;
		break;
	default:
		return NULL;
	}

	/* compute the filter specs */
	nkernel = nwingk * UP_KERNEL;
	nfilter = nwingk * MAX(up, dn);
	nwing = nfilter / up;
	nfilter = nwing * up;

	/* adjust gain when downsampling */
	if (nwing > nwingk)
		gain = udivhi(nwingk, nwing) >> 1;
	else
		gain = 0x7fffffff;	/* unity */

	/* allocate buffers */
	nwinga = (nwing + NWING_ALIGN-1) & ~(NWING_ALIGN-1);	/* aligned size */
	filter = (short *) calloc(nwinga * up, sizeof(short));
	if (!filter)
		return NULL;
	filtbuf = (int *) calloc(nfilter, sizeof(int));
	if (!filtbuf) {
		free(filter);
		return NULL;
	}

	/* interpolate the full filter */
	Interpolate(kernel, nkernel, filtbuf, nfilter, gain);

	/* quantize the filter coefs */
	for (i = 0; i < nfilter; i++) {
		filtbuf[i] = (filtbuf[i] + (1<<15)) >> 16;
	}

	/* deinterleave into phases */
	for (phase = 0; phase < up; phase++) {
		for (i = 0; i < nwing; i++) {
			filter[phase*nwinga + i] = (short) filtbuf[i*up + phase];
		}
	}

	free(filtbuf);
	*pnwing = nwing;
	return filter;
}

/*
 * Creates a new resampler.
 *
 * inrate and outrate can be arbitrary sampling rates.
 *
 * nchans must be 1 (mono) or 2 (stereo).
 *
 * quality parameter ranges from 0 (low) to 3 (high).
 * lower quality is faster, and uses less memory.
 *
 * returns the resampler instance, or NULL on error.
 */
void *
InitResampler(int inrate, int outrate, int nchans, int quality)
{
	state_t *s;
	int divisor, up, dn;
	int nwing, nwinga, nhist;
	int pcmstep, fltstep;
	uint uprate, stepf;
	short *filter;

	if (inrate < 1 || inrate > MAX_RATE)
		return NULL;
	if (outrate < 1 || outrate > MAX_RATE)
		return NULL;
	if (nchans < 1 || nchans > MAX_CHANS)
		return NULL;

	/* reduce to smallest fraction */
	divisor = gcd(inrate, outrate);
	up = outrate / divisor;
	dn = inrate / divisor;
	stepf = 0;	/* RAT mode */

	/* when too many phases, ARB mode */
	if (up > UP_MAX) {
		uprate = UP_ARB * inrate;
		up = UP_ARB;
		dn = uprate / outrate;
		stepf = udivhi(uprate - dn * outrate, outrate);
	}

	/* create the polyphase filter */
	filter = MakeFilter(up, dn, quality, &nwing);
	if (!filter)
		return NULL;

	//printf("mode=%s qual=%d ", stepf ? "ARB" : "RAT", quality);
	//printf("up=%d down=%.3f taps=%d\n", up, dn + stepf/4294967296.0, 2*nwing);

	nwinga = (nwing + NWING_ALIGN-1) & ~(NWING_ALIGN-1);	/* aligned size */
	nhist = nchans * (2 * nwing + (stepf != 0));

	/* allocate buffers */
	s = (state_t *) calloc(1, sizeof(state_t));
	if (!s) {
		free(filter);
		return NULL;
	}
	s->histbuf = (short *) calloc(2 * nhist, sizeof(short));
	if (!s->histbuf) {
		free(s);
		free(filter);
		return NULL;
	}

	/* filter init */
	s->filter = filter;
	s->up = up;
	s->dn = dn;
	s->nchans = nchans;
	s->nwing = nwing;
	s->nhist = nhist;
	s->nstart = nhist - nchans * (nwing-1);
	s->offset = 0;
	s->phasef = 0;
	s->stepf = stepf;
	s->rwing = s->filter;
	s->lwing = s->filter + nwinga * (up-1);

	/*
	 * Create the polyphase stepping tables.
	 * NOTE: step-by-N includes -nwing to rewind pointers.
	 */

	/* step phase by N */
	pcmstep = dn / up;
	fltstep = dn - up * pcmstep;	/* dn % up */
	s->stepNptr = s->filter + nwinga * (up - fltstep);	/* fwd/bak threshold */
	s->stepNfwd[0] = nwinga * fltstep - nwing;			/* rwgptr step */
	s->stepNfwd[1] = -nwinga * fltstep - nwing;			/* lwgptr step */
	s->stepNfwd[2] = nchans * (pcmstep - nwing);		/* pcmptr step */

	pcmstep = (dn + up-1) / up;
	fltstep = (dn + up-1) - up * pcmstep;
	s->stepNbak[0] = nwinga * (fltstep - (up-1)) - nwing;
	s->stepNbak[1] = -nwinga * (fltstep - (up-1)) - nwing;
	s->stepNbak[2] = nchans * (pcmstep - nwing);

	/* step phase by 1 */
	pcmstep = 1 / up;
	fltstep = 1 - up * pcmstep;
	s->step1ptr = s->filter + nwinga * (up - fltstep);
	s->step1fwd[0] = nwinga * fltstep;
	s->step1fwd[1] = -nwinga * fltstep;
	s->step1fwd[2] = nchans * pcmstep;

	pcmstep = (1 + up-1) / up;
	fltstep = (1 + up-1) - up * pcmstep;
	s->step1bak[0] = nwinga * (fltstep - (up-1));
	s->step1bak[1] = -nwinga * (fltstep - (up-1));
	s->step1bak[2] = nchans * pcmstep;

	/* set the core function pointer */
	if (stepf != 0)
		s->ResampleCore = (nchans == 1 ? ARBCoreMono : ARBCoreStereo);
	else
		s->ResampleCore = (nchans == 1 ? RATCoreMono : RATCoreStereo);

	return (void *)s;
}

/*
 * Resamples inbuf into outbuf.
 *
 * insamps can be any number of samples, including zero.
 *
 * outbuf must be sufficiently large to hold the output.
 * use GetMaxOutput() to allocate properly.
 *
 * returns the number of output samples.
 */
int
Resample(short *inbuf, int insamps, short *outbuf, void *inst)
{
	state_t *s = (state_t *)inst;
	short *pcmptr, *pcmend, *outptr;

	/* fill history buffer */
	memcpy(s->histbuf + s->nhist, inbuf, MIN(s->nhist, insamps) * sizeof(short));

	/* process history buffer */
	pcmptr = s->histbuf + s->nstart;
	pcmend = pcmptr + MIN(s->nhist, insamps);
	outptr = s->ResampleCore(pcmptr, pcmend, outbuf, s);

	/* process input buffer */
	if (insamps > s->nhist) {
		pcmptr = inbuf + s->nstart;
		pcmend = pcmptr + (insamps - s->nhist);
		outptr = s->ResampleCore(pcmptr, pcmend, outptr, s);
	}

	/* save history buffer */
	if (insamps > s->nhist)
		memcpy(s->histbuf, inbuf + (insamps - s->nhist), s->nhist * sizeof(short));
	else
		memmove(s->histbuf, s->histbuf + insamps, s->nhist * sizeof(short));

	return (outptr - outbuf);
}

void
FreeResampler(void *inst)
{
	state_t *s = (state_t *)inst;

	if (s) {
		if (s->filter)
			free(s->filter);
		if (s->histbuf)
			free(s->histbuf);
		free(s);
	}
}

/*
 * Computes the maximum output samples that could be produced by insamps.
 */
int
GetMaxOutput(int insamps, void *inst)
{
	state_t *s = (state_t *)inst;
	int i, outsamps;
	uint f;

	if (s->nchans == 2)
		insamps = (insamps + 1) >> 1;	/* frames */

	/* outsamps = ceil(insamps * up / dn.stepf) */
	i = f = outsamps = 0;
	while (i < s->up * insamps) {
		f += s->stepf;
		i += s->dn + (f < s->stepf);	/* add with carry */
		outsamps++;
	}

	return outsamps * s->nchans;
}

/* faster version, using __int64
int
GetMaxOutput(int insamps, void *inst)
{
	state_t *s = (state_t *)inst;
	__int64 dn64, out64;

	if (s->nchans == 2)
		insamps = (insamps + 1) >> 1;

	dn64 = ((__int64)s->dn << 32) + s->stepf;
	out64 = (__int64)(insamps * s->up) << 32;
	out64 = (out64 + (dn64 - 1)) / dn64;

	return (int)out64 * s->nchans;
}
*/

/*
 * Computes the minimum input samples that will produce at least outsamps.
 */
int
GetMinInput(int outsamps, void *inst)
{
	state_t *s = (state_t *)inst;
	uint numhi, numlo, quo, rem;
	int insamps;

	if (s->nchans == 2)
		outsamps = (outsamps + 1) >> 1;	/* frames */

	/* insamps = ceil(outsamps * dn.stepf / up) */
	numhi = outsamps * s->dn + UMULHI(outsamps, s->stepf);
	numlo = outsamps * s->stepf;
	quo = numhi / s->up;
	rem = numhi - quo * s->up;
	insamps = quo + (rem || numlo);	/* round up */

	return insamps * s->nchans;
}

/*
 * Computes the approximate group delay.
 * NOTE: return value is frames, not samples...
 */
int
GetDelay(void *inst)
{
	state_t *s = (state_t *)inst;
	int delay;

	delay = GetMaxOutput(s->nwing * s->nchans, inst);

	if (s->nchans == 2)
		delay >>= 1;	/* frames */

	return delay;
}
