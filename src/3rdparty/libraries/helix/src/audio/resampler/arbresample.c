/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: arbresample.c,v 1.10 2004/07/09 18:37:30 hubbe Exp $
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
 * Polyphase sampling rate conversion.
 * Arbitrary mode.
 *
 * by Ken Cooke
 */

#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"

#include "kaiser.h"
#include "mymath.h"
#include "allresamplers.h"

/* leave a trace of the C source version in the object code */
static const char VERSION[] = "$Revision: 1.10 $" ;

#undef ASSERT
#define ASSERT(x)

/* oversampling factor */
#define UPBITS		9
#define UP			(1 << UPBITS)
#define FRACBITS	(32 - UPBITS)
#define FRACMASK	((1 << FRACBITS) - 1)
#define FRAC2FLT	(1.0f / (1 << FRACBITS))

/* filter state */
typedef struct {
	int up;
	int dn;
	int inrate;
	int outrate;
	int nchans;
	int nwing;
	int nhist;
//	int phase;
	int offset;
	int isCloned; // if 1, some of this filter data is owned by another filter instance.
	float *histbuf;
	float *pcmbuf;
	float *filter;
//	uchar *pcmstep;
	unsigned int time_f;
	unsigned int step_i;
	unsigned int step_f;
} state_t;

void *
RAInitResamplerArb(int inrate, int outrate, int nchans,
			  float atten, float passband, float stopband, float dcgain)
{
	state_t *s;
//	int divisor;
	int up, dn;
	int ntaps, nwing, nfilter, nhist;
	float beta, fpass, fstop;
	double *lpfilt;
	int i, phase;

	/* use defaults for values <= 0 */
	if (atten <= 0.0f)
		atten = DEF_ATTEN;
	if (passband <= 0.0f)
		passband = DEF_PASSBAND;
	if (stopband <= 0.0f)
		stopband = DEF_STOPBAND;
	if (dcgain <= 0.0f)
		dcgain = DEF_DCGAIN;

	if (nchans < 1 || nchans > 2)
		return NULL;
	if (passband >= stopband)
		return NULL;

	/* reduce to smallest fraction */
//	divisor = gcd(inrate, outrate);
//	up = outrate / divisor;
//	dn = inrate / divisor;
	up = UP;
	dn = 1;

	/* compute the filter specs */
//	fstop = 1.0f / MAX(up, dn);
	fstop = MIN(1.0f, (float)outrate/inrate) / up;
	fpass = passband * fstop;
	fstop = stopband * fstop;
	KaiserEstim(fpass, fstop, atten, &nfilter, &beta);

	ntaps = (nfilter + up-1) / up;	/* length of each filter phase */
	nwing = (ntaps + 1) / 2;
	ntaps = nwing * 2;				/* update ntaps */
	nfilter = nwing * up;			/* update nfilter */
	nhist = nchans * ntaps;

//	printf("up=%d down=%d ntaps=%d beta=%.2f atten=%.1fdB pass=%.0fHz stop=%.0fHz\n",
//		up, dn, ntaps, beta, atten, 0.5*inrate*fpass*UP, 0.5*inrate*fstop*UP);

	/* malloc buffers */
	lpfilt = (double *) malloc(nfilter * sizeof(double));

	s = (state_t *) malloc(sizeof(state_t));
	s->filter = (float *) malloc(nfilter * sizeof(float));
	s->isCloned = 0;
//	s->pcmstep = (uchar *) malloc(up * sizeof(uchar));
	s->histbuf = (float *) calloc((NBLOCK + nhist), sizeof(float));
	s->pcmbuf = s->histbuf + nhist;

	/* create the lowpass filter */
	KaiserLowpass(nfilter, 0.5f * (fpass + fstop), beta, (dcgain * up), lpfilt);

	/* deinterleave into phases */
	for (phase = 0; phase < up; phase++) {
		for (i = 0; i < nwing; i++) {
			s->filter[phase*nwing + i] = (float) lpfilt[i*up + phase];
		}
	}
	
	/* filter init */
	s->inrate = inrate;
	s->outrate = outrate;
	s->up = up;
	s->dn = dn;
	s->nchans = nchans;
	s->nwing = nwing;
	s->nhist = nhist;
//	s->phase = 0;
	s->offset = 0;
	s->time_f = 0;
	s->step_i = (unsigned int)((double)inrate / outrate);
	s->step_f = (unsigned int)(4294967296.0 * ((double)inrate / outrate - s->step_i));
	
	free(lpfilt);

	return (void *) s;
}

void *
RAInitResamplerCopyArb(int nchans, const void *inst)
{
	state_t *s_in = (state_t *)inst;
	state_t *s_out = (state_t *)malloc(sizeof(state_t));
	if (s_in == NULL || s_out == NULL)
		return NULL;

	*s_out = *s_in ;
	s_out->isCloned = 1;
	s_out->nchans = nchans;
	s_out->nhist = s_in->nhist / s_in->nchans * nchans;

	s_out->histbuf = (float *) calloc((NBLOCK + s_out->nhist), sizeof(float));
	if (s_out->histbuf == NULL)
		return NULL;
	s_out->pcmbuf = s_out->histbuf + s_out->nhist;

	return s_out;
}

void
RAFreeResamplerArb(void *inst)
{
	state_t *s = (state_t *)inst;

	if (s != NULL) {
		if (!s->isCloned && s->filter)
			free(s->filter);
//		free(s->pcmstep);
		if (s->histbuf)
			free(s->histbuf);
		free(s);
	}
}

int
RAResampleStereoArb(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst)
{
	state_t *s = (state_t *)inst;
	float *pcmptr, *pcmend, *pcmptr2;
	float *rwingptr, *lwingptr;
	short *outptr;
	float acc0, acc1, acc2, acc3, frac;
	int i;
	int nwing = s->nwing;
	unsigned int step_i = s->step_i;
	unsigned int step_f = s->step_f;
	unsigned int time_f = s->time_f;
	unsigned int phase, phase2;

//	TICK();
//	ASSERT(!(insamps & 0x1));	/* stereo must be even */

	/* convert input to float */
	insamps = pCvt->pfCvt(s->pcmbuf,inbuf,insamps,pCvt->pStateMachine) ;

	/* restore filter state */
	phase = time_f >> FRACBITS;
	phase2 = (phase + 1) & (UP-1);
	pcmptr = s->pcmbuf - STEREO * (nwing-1);
	pcmend = pcmptr + insamps;
	pcmptr += s->offset;
	pcmptr2 = pcmptr + STEREO * (phase2 < phase);
	outptr = outbuf;

	/* filter */
	while (pcmptr2 < pcmend) {

		rwingptr = s->filter + nwing * phase;
		lwingptr = s->filter + nwing * (UP-1-phase);
		acc0 = acc1 = 0.0f;

		for (i = 0; i < nwing; i++) {
			int j = STEREO * i;
			acc0 += pcmptr[-j-2] * rwingptr[i];
			acc1 += pcmptr[-j-1] * rwingptr[i];
			acc0 += pcmptr[+j+0] * lwingptr[i];
			acc1 += pcmptr[+j+1] * lwingptr[i];
		}

		rwingptr = s->filter + nwing * phase2;
		lwingptr = s->filter + nwing * (UP-1-phase2);
		acc2 = acc3 = 0.0f;

		for (i = 0; i < nwing; i++) {
			int j = STEREO * i;
			acc2 += pcmptr2[-j-2] * rwingptr[i];
			acc3 += pcmptr2[-j-1] * rwingptr[i];
			acc2 += pcmptr2[+j+0] * lwingptr[i];
			acc3 += pcmptr2[+j+1] * lwingptr[i];
		}

		frac = (time_f & FRACMASK) * FRAC2FLT;
		acc0 += frac * (acc2 - acc0);
		acc1 += frac * (acc3 - acc1);
	
		time_f += step_f;
		phase = time_f >> FRACBITS;
		phase2 = (phase + 1) & (UP-1);
		pcmptr += STEREO * (step_i + (time_f < step_f));
		pcmptr2 = pcmptr + STEREO * (phase2 < phase);

		outptr[0] = RoundFtoS(acc0);
		outptr[1] = RoundFtoS(acc1);
		outptr += outstride;
	}

	/* save filter state */
	s->time_f = time_f;
	s->offset = pcmptr - pcmend;
	memmove(s->histbuf, s->histbuf + insamps, s->nhist * sizeof(float));

//	TOCK(outptr - outbuf);
	return (outptr - outbuf);
}

int
RAResampleMonoArb(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst)
{
    state_t *s = (state_t *)inst;
	float *pcmptr, *pcmend, *pcmptr2;
	float *rwingptr, *lwingptr;
	short *outptr;
	float acc0, acc1, acc2, acc3, frac;
	int i;
	int nwing = s->nwing;
	unsigned int step_i = s->step_i;
	unsigned int step_f = s->step_f;
	unsigned int time_f = s->time_f;
	unsigned int phase, phase2;

//	TICK();

	/* convert input to float */
	insamps = pCvt->pfCvt(s->pcmbuf,inbuf,insamps,pCvt->pStateMachine) ;

	/* restore filter state */
	phase = time_f >> FRACBITS;
	phase2 = (phase + 1) & (UP-1);
	pcmptr = s->pcmbuf - (nwing-1);
	pcmend = pcmptr + insamps;
	pcmptr += s->offset;
	pcmptr2 = pcmptr + (phase2 < phase);
	outptr = outbuf;

	/* filter */
	while (pcmptr2 < pcmend) {

		rwingptr = s->filter + nwing * phase;
		lwingptr = s->filter + nwing * (UP-1-phase);
		acc0 = acc1 = 0.0f;

		for (i = 0; i < nwing; i++) {
			acc0 += pcmptr[-i-1] * rwingptr[i];
			acc1 += pcmptr[+i+0] * lwingptr[i];
		}

		rwingptr = s->filter + nwing * phase2;
		lwingptr = s->filter + nwing * (UP-1-phase2);
		acc2 = acc3 = 0.0f;

		for (i = 0; i < nwing; i++) {
			acc2 += pcmptr2[-i-1] * rwingptr[i];
			acc3 += pcmptr2[+i+0] * lwingptr[i];
		}

		acc0 += acc1;
		acc2 += acc3;
		frac = (time_f & FRACMASK) * FRAC2FLT;
		acc0 += frac * (acc2 - acc0);

		time_f += step_f;
		phase = time_f >> FRACBITS;
		phase2 = (phase + 1) & (UP-1);
		pcmptr += step_i + (time_f < step_f);
		pcmptr2 = pcmptr + (phase2 < phase);

		outptr[0] = RoundFtoS(acc0);
		outptr += outstride;
	}

	/* save filter state */
	s->time_f = time_f;
	s->offset = pcmptr - pcmend;
	memmove(s->histbuf, s->histbuf + insamps, s->nhist * sizeof(float));

//	TOCK(outptr - outbuf);
	return (outptr - outbuf);
}

/*
 * The maximum output, in samples, that could be produced by
 * insamps input samples.
 */
int
RAGetMaxOutputArb(int insamps, void *inst)
{
    state_t *s = (state_t *)inst;
	int inframes, outframes;
	double step;

	inframes = (insamps + (s->nchans-1)) / s->nchans;
	step = s->step_i + (s->step_f / 4294967296.0);
	outframes = (int) myCeil((double)inframes / step);
	return outframes * s->nchans;
}

/*
 * The minimum input, in samples, that will produce at least
 * outsamps output samples.
 */
int
RAGetMinInputArb(int outsamps, void *inst)
{
    state_t *s = (state_t *)inst;
	int inframes, outframes;
	double step;

	outframes = (outsamps + (s->nchans-1)) / s->nchans;
	step = s->step_i + (s->step_f / 4294967296.0);
	inframes = (int) myCeil((double)outframes * step);

	return inframes * s->nchans;
}

int
RAGetDelayArb(void *inst)
{
	state_t *s = (state_t *)inst;
	return (int)(s->nwing * (float)s->outrate / s->inrate) ;
}
