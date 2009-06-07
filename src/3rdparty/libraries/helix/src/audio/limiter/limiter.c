/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: limiter.c,v 1.7 2005/10/14 22:47:14 gwright Exp $
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
 * Fixed-point lookahead peak-limiter
 * by Ken Cooke
 */
#include "hlxclib/math.h"
#include "hlxclib/stdlib.h"

#include "limiter.h"
#include "math64.h"
#include "hxassert.h"

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define INCMOD(a,b)	((a) = ((a) + (b)) & 0x1ff)

#define LEFT 0
#define RGHT 1
#define MONO 0

#define FIXSCALE	2147483648.0	/* convert to S.31 fixed-point */
#define FLT_ONE		2147483647.0
#define	FIX_ONE		0x7fffffff

static const double def_dbgain = 0.0;
static const double def_outceil = -0.1;
static const double def_release = 250.0;
static const double def_rms_attack = 25.0;
static const double def_rms_release = 250.0;
static const double cicgain = (1 << 12) / (53.0 * 76.0);

#define RAND16(r) (((r) = (r) * 69069U + 1U) >> 16)
static unsigned char randseed[32] = { /* Flawfinder: ignore */
	0x00, 0x0d, 0x0a, 0x50, 0x65, 0x61, 0x6b, 0x5f,
	0x4c, 0x69, 0x6d, 0x69, 0x74, 0x65, 0x72, 0x20,
	0x62, 0x79, 0x20, 0x4b, 0x65, 0x6e, 0x20, 0x43,
	0x6f, 0x6f, 0x6b, 0x65, 0x0d, 0x0a, 0x00, 0x00,
};

/*
 * Utility functions
 */

double
DbToGain(double db) {
	return pow(10.0, db / 20.0);
}

double
GainToDb(double gain) {
	HX_ASSERT(gain > 0.0);
	return (20.0 * log10(gain));
}

double
MsToTc(double ms, double samprate) {
	HX_ASSERT(ms > 0.0);
	HX_ASSERT(samprate > 0.0);
	return exp(-1000.0 / (ms * samprate));
}

double
TcToMs(double tc, double samprate) {
	HX_ASSERT(tc > 0.0);
	HX_ASSERT(samprate > 0.0);
	return (-1000.0 / (log(tc) * samprate));
}

double
Log2(double x) {
	HX_ASSERT(x > 0.0);
	return (log(x) / log(2.0));
}

void
LimiterSetGain(double dbgain, LIMSTATE *lim)
{
	double mkupgain;
	double outgain;

	lim->dbgain = dbgain;

	/* compute threshold */
	mkupgain = (1 << lim->headroom) * DbToGain(dbgain);
	lim->threshold = (int)(FLT_ONE / MAX(mkupgain, 1.0));

	/* compute output gain */
	outgain = DbToGain(def_outceil) * mkupgain * cicgain;

	/* normalize outgain into shift and fraction */
	lim->outshift = (int) (ceil(Log2(outgain)));
	lim->outshift = MAX(lim->outshift, 0);
	lim->outfract = (int) ((outgain / (1 << lim->outshift)) * FLT_ONE);
	HX_ASSERT(lim->outfract >= 0);	/* should never overflow */
	lim->outshift = 31 - lim->outshift;
}

void
LimiterSetRelease(double release, LIMSTATE *lim)
{
	double x, ms, tc;
	double peakval, rmsval;
	int n;

	/* compute time constants */
	lim->reltc = (int)(FIXSCALE * MsToTc(release, lim->samprate));
	lim->rmsatktc = (int)(FIXSCALE * MsToTc(def_rms_attack, lim->samprate));
	lim->rmsreltc = (int)(FIXSCALE * MsToTc(def_rms_release, lim->samprate));

	/* compute adaptive release tables */
	for (n = 0; n < 256; n++) {

		/* rms saturation curve */
		x = n * (1.0 / 256.0);
		x = x * x * x;
		x = 1.0 - x;
		x = x * x * x;		/* cubic "S" curve */

		ms = x * release;
		ms = MAX(ms, 0.01);	/* clamp small values */
		tc = MsToTc(ms, lim->samprate);

		/* peak-over-rms acceleration */
		peakval = pow(0.5, 1.0 / (n + 1));
		rmsval = tc / peakval;

		lim->peaktab[n] = (int)(FIXSCALE * peakval);
		lim->rmstab[n] = (int)(0.5 * FIXSCALE * rmsval);	/* shifted >> 1 */
	}
}

int
LimiterGetDelay(LIMSTATE *lim)
{
	return 128;
}

double
LimiterGetAtten(LIMSTATE *lim)
{
	return -GainToDb(lim->peak_z / FLT_ONE);	// fixme
}

LIMSTATE *
LimiterInit(int samprate, int channels, int headroom)
{
	LIMSTATE *lim;
	int i, n, dcinput;

	lim = (LIMSTATE *) calloc(1, sizeof(LIMSTATE));
	if (!lim)
		return NULL;

	HX_ASSERT(headroom >= 0);	/* required for abs(pcm) */

	lim->samprate = samprate;
	lim->channels = channels;
	lim->headroom = headroom;

	/* init history */
	lim->idx = 0;
	lim->peak_z = FIX_ONE;
	lim->rms_z = FIX_ONE;
	lim->randsave = randseed[19] | 0x1;

	i = 0;
	/* peak-hold lookahead */
	for (n = 0; n < 128; n++)
		lim->delay[i++] = FIX_ONE;

	/* CIC filter history */
	dcinput = FIX_ONE >> 6;
	lim->acc1 = 53 * dcinput;
	for (n = 52; n > 0; n--)
		lim->delay[i++] = n * dcinput;

	dcinput = (dcinput * 53) >> 6;
	lim->acc2 = 76 * dcinput;
	for (n = 75; n > 0; n--)
		lim->delay[i++] = n * dcinput;

	lim->delay[i++] = 0xbadf00d;	/* unused tap */

	/* pcm delay */
	for (n = 0; n < 256; n++)
		lim->delay[i++] = 0;

	HX_ASSERT(i == 512);

	/* default settings */
	LimiterSetGain(def_dbgain, lim);
	LimiterSetRelease(def_release, lim);

	return lim;
}

void
LimiterFree(LIMSTATE *lim)
{
	free(lim);
}

void
LimiterStereo(int *pcmbuf, int nsamples, LIMSTATE *lim)
{
	int *pcmptr, *delay;
	int atten, rms, peak;
	int i, tc;

//	TICK();

	HX_ASSERT(!(nsamples & 0x1));	/* must be even */

	delay = lim->delay;
	i = lim->idx;

	for (pcmptr = pcmbuf; pcmptr < pcmbuf + nsamples; pcmptr += 2) {
		unsigned int pcmleft  = abs(pcmptr[LEFT]) ;
		unsigned int pcmright = abs(pcmptr[RGHT]) ;

		/* peak detect */
		unsigned int pcmpeak  = MAX(pcmleft, pcmright) ;

		/* compute the required attenuation */
		if (pcmpeak == 0x80000000UL)
			atten = MulShift31(FIX_ONE, lim->threshold) ;
		else if (pcmpeak > lim->threshold)
			atten = MulDiv64(FIX_ONE, lim->threshold, pcmpeak);
		else
			atten = FIX_ONE;

		/* peak-hold lookahead */
		delay[i] = atten;
		INCMOD(i, 1);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 2);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 4);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 8);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 16);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 32);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 64);
		atten = MIN(atten, delay[i]);
		/* atten is now min value over lookahead */

		/* release filter */
		peak = atten;
		if (peak > lim->peak_z)
			peak -= MulShift31((peak - lim->peak_z), lim->reltc);
		lim->peak_z = peak;
		atten = peak;

		/* adapt release */
		rms = atten;
		tc = (rms < lim->rms_z ? lim->rmsatktc : lim->rmsreltc);
		rms -= MulShift31((rms - lim->rms_z), tc);
		lim->rms_z = rms;

		peak = MIN(peak, rms);
		lim->reltc = MulShiftN(lim->peaktab[peak>>23], lim->rmstab[rms>>23], 30);

		/*
		 * FIR attack/lowpass filter, via 2-stage CIC
		 *
		 * H(z) = 1 (1 - z^-53) (1 - z^-76)
		 *        - ---------- ----------
		 *    53*76 (1 - z^-1)  (1 - z^-1)
		 */
		delay[i] = lim->acc1;
		lim->acc1 += atten >> 6;
		INCMOD(i, 52);
		atten = lim->acc1 - delay[i];

		delay[i] = lim->acc2;
		lim->acc2 += atten >> 6;
		INCMOD(i, 75);
		atten = lim->acc2 - delay[i];

		atten = MulShift31(atten, lim->outfract);
		INCMOD(i, 1);	/* skip unused taps */

		/* pcm delay */
		delay[i] = pcmptr[LEFT];
		INCMOD(i, 128);
		pcmptr[LEFT] = delay[i];

		delay[i] = pcmptr[RGHT];
		INCMOD(i, 128);
		pcmptr[RGHT] = delay[i];

		/* modulate pcm */
		pcmptr[LEFT] = MulShiftN(pcmptr[LEFT], atten, lim->outshift);
		pcmptr[RGHT] = MulShiftN(pcmptr[RGHT], atten, lim->outshift);
	}
	lim->idx = i;

//	TOCK();
}

void
LimiterMono(int *pcmbuf, int nsamples, LIMSTATE *lim)
{
	int *pcmptr, *delay;
	int atten, rms, peak;
	int i, tc;

//	TICK();

	delay = lim->delay;
	i = lim->idx;

	for (pcmptr = pcmbuf; pcmptr < pcmbuf + nsamples; pcmptr += 1) {

		/* peak detect */
		unsigned int pcmpeak = abs(pcmptr[MONO]);

		/* compute the required attenuation */
		if (pcmpeak == 0x80000000UL)
			atten = MulShift31(FIX_ONE, lim->threshold) ;
		else if (pcmpeak > lim->threshold)
			atten = MulDiv64(FIX_ONE, lim->threshold, pcmpeak);
		else
			atten = FIX_ONE;

		/* peak-hold lookahead */
		delay[i] = atten;
		INCMOD(i, 1);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 2);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 4);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 8);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 16);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 32);
		atten = MIN(atten, delay[i]);
		delay[i] = atten;
		INCMOD(i, 64);
		atten = MIN(atten, delay[i]);
		/* atten is now min value over lookahead */

		/* release filter */
		peak = atten;
		if (peak > lim->peak_z)
			peak -= MulShift31((peak - lim->peak_z), lim->reltc);
		lim->peak_z = peak;
		atten = peak;

		/* adapt release */
		rms = atten;
		tc = (rms < lim->rms_z ? lim->rmsatktc : lim->rmsreltc);
		rms -= MulShift31((rms - lim->rms_z), tc);
		lim->rms_z = rms;

		peak = MIN(peak, rms);
		lim->reltc = MulShiftN(lim->peaktab[peak>>23], lim->rmstab[rms>>23], 30);

		/*
		 * FIR attack/lowpass filter, via 2-stage CIC
		 *
		 * H(z) = 1 (1 - z^-53) (1 - z^-76)
		 *        - ---------- ----------
		 *    53*76 (1 - z^-1)  (1 - z^-1)
		 */
		delay[i] = lim->acc1;
		lim->acc1 += atten >> 6;
		INCMOD(i, 52);
		atten = lim->acc1 - delay[i];

		delay[i] = lim->acc2;
		lim->acc2 += atten >> 6;
		INCMOD(i, 75);
		atten = lim->acc2 - delay[i];

		atten = MulShift31(atten, lim->outfract);
		INCMOD(i, 129);	/* skip unused taps */

		/* pcm delay */
		delay[i] = pcmptr[MONO];
		INCMOD(i, 128);
		pcmptr[MONO] = delay[i];

		/* modulate pcm */
		pcmptr[MONO] = MulShiftN(pcmptr[MONO], atten, lim->outshift);
	}
	lim->idx = i;

//	TOCK();
}

void
LimiterProcess(int *pcmbuf, int nsamples, LIMSTATE *lim)
{
	if (lim->channels == 1)
		LimiterMono(pcmbuf, nsamples, lim);
	else
		LimiterStereo(pcmbuf, nsamples, lim);
}

void
LimiterOutput16(int *pcmbuf, short *outbuf, int nsamples, LIMSTATE *lim)
{
	unsigned int randsave;
	int i, r0, r1, dither;

//	TICK();
	HX_ASSERT(def_outceil < -0.0004);	/* guarantees no clipping */

	randsave = lim->randsave;

	for (i = 0; i < nsamples; i++) {
		r0 = RAND16(randsave);
		r1 = RAND16(randsave);
		dither = r0 - r1;	/* flat TPDF dither */
		outbuf[i] = (short) ((pcmbuf[i] + dither + (1<<15)) >> 16);
	}

	lim->randsave = randsave;
//	TOCK();
}
