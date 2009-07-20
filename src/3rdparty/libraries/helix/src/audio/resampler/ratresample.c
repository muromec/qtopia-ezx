/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ratresample.c,v 1.10 2007/07/06 20:21:29 jfinnecy Exp $
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

/*
 * Polyphase sampling rate conversion.
 * Rational mode.
 *
 * by Ken Cooke
 */

#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
//#include <stdio.h>
#include "allresamplers.h"
#include "kaiser.h"
#include "mymath.h"

/* leave a trace of the C source version in the object code */
static const char VERSION[] = "$Revision: 1.10 $" ;

#undef ASSERT
#define ASSERT(x)

/* filter state */
typedef struct {
	int up;
	int dn;
	int inrate;
	int outrate;
	int nchans;
	int nwing;
	int nhist;
	int phase;
	int offset;
	int isCloned;
	float *histbuf;
	float *pcmbuf;
	float *filter;
	uchar *pcmstep;
	int *nextphase;
} state_t;


void *
RAInitResamplerRat(int inrate, int outrate, int nchans,
			  float atten, float passband, float stopband, float dcgain)
{
	state_t *s;
	int divisor, up, dn;
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
	divisor = gcd(inrate, outrate);
	up = outrate / divisor;
	dn = inrate / divisor;

	if (up > 1280)
		return NULL;	/* supports standard rates to 96000Hz */

	if (nchans * ((dn + up-1) / up) > 255)
		return NULL;	/* pcmstep exceeds uchar */

	/* compute the filter specs */
	fstop = 1.0f / MAX(up, dn);
	fpass = passband * fstop;
	fstop = stopband * fstop;
	KaiserEstim(fpass, fstop, atten, &nfilter, &beta);

	ntaps = (nfilter + up-1) / up;	/* length of each filter phase */
	nwing = (ntaps + 1) / 2;
	ntaps = nwing * 2;				/* update ntaps */
	nfilter = nwing * up;			/* update nfilter */
	nhist = nchans * ntaps;

//	printf("up=%d down=%d ntaps=%d beta=%.2f atten=%.1fdB pass=%.0fHz stop=%.0fHz\n",
//		up, dn, ntaps, beta, atten, 0.5*outrate*fpass*dn, 0.5*outrate*fstop*dn);

	/* malloc buffers */
	lpfilt = (double *) malloc(nfilter * sizeof(double));

	s = (state_t *) malloc(sizeof(state_t));
	s->filter = (float *) malloc(nfilter * sizeof(float));
	s->isCloned = 0;
	s->pcmstep = (uchar *) malloc(up * sizeof(uchar));
	s->nextphase = (int *) malloc(up * sizeof(int));
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

	/* lookup tables, driven by phase */
	for (i = 0; i < up; i++) {
		phase = (i * dn) % up;
		s->pcmstep[phase] = nchans * ((((i+1) * dn) / up) - ((i * dn) / up));
		s->nextphase[phase] = ((i+1) * dn) % up;
	}

	/* filter init */
	s->inrate = inrate;
	s->outrate = outrate;
	s->up = up;
	s->dn = dn;
	s->nchans = nchans;
	s->nwing = nwing;
	s->nhist = nhist;
	s->phase = 0;
	s->offset = 0;
	
	free(lpfilt);

	return (void *) s;
}

void *
RAInitResamplerCopyRat(int nchans, const void *inst)
{
	int i;
	state_t *s_in = (state_t *)inst;
	state_t *s_out = (state_t *)malloc(sizeof(state_t));
	if (s_in == NULL || s_out == NULL)
		return NULL;

	*s_out = *s_in ;
	s_out->isCloned = 1;
	s_out->nchans = nchans;
	s_out->nhist = s_in->nhist / s_in->nchans * nchans;

	s_out->histbuf = (float *) calloc((NBLOCK + s_out->nhist), sizeof(float));
	s_out->pcmstep = (uchar *) malloc(s_out->up * sizeof(uchar));
	if (s_out->histbuf == NULL || s_out->pcmstep == NULL)
		return NULL;

	s_out->pcmbuf = s_out->histbuf + s_out->nhist;

	for (i = 0; i < s_out->up; i++) {
		s_out->pcmstep[i] = s_in->pcmstep[i] / s_in->nchans * nchans ;
	}

	return s_out;
}

void
RAFreeResamplerRat(void *inst)
{
	state_t *s = (state_t *)inst;

	if (s != NULL) {
		if (!s->isCloned)
		{
			if (s->filter)
				free(s->filter);
			if (s->nextphase)
				free(s->nextphase);
		}
		if (s->pcmstep)
			free(s->pcmstep);
		if (s->histbuf)
			free(s->histbuf);
		free(s);
	}
}

int
RAResampleStereoRat(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst)
{
	state_t *s = (state_t *)inst;
	float *pcmptr, *pcmend;
	float *rwingptr, *lwingptr;
	short *outptr;
	float acc0, acc1;
	int i;
	int up = s->up;
	int phase = s->phase;
	int nwing = s->nwing;
	uchar *pcmstep = s->pcmstep;
	int *nextphase = s->nextphase;

//	TICK();
	//	ASSERT(!(insamps & 0x1));	/* stereo must be even */

	/* convert input to float */
	insamps = pCvt->pfCvt(s->pcmbuf,inbuf,insamps,pCvt->pStateMachine) ;

	/* restore filter state */
	pcmptr = s->pcmbuf - STEREO * (nwing-1);
	pcmend = pcmptr + insamps;
	pcmptr += s->offset;
	outptr = outbuf;

	/* filter */
	while (pcmptr < pcmend) {

		rwingptr = s->filter + nwing * phase;
		lwingptr = s->filter + nwing * (up-1-phase);
		acc0 = acc1 = 0.0f;

		for (i = 0; i < nwing; i++) {
			int j = STEREO * i;
			acc0 += pcmptr[-j-2] * rwingptr[i];
			acc1 += pcmptr[-j-1] * rwingptr[i];
			acc0 += pcmptr[+j+0] * lwingptr[i];
			acc1 += pcmptr[+j+1] * lwingptr[i];
		}
	
		pcmptr += pcmstep[phase];
		phase = nextphase[phase];

		outptr[0] = RoundFtoS(acc0);
		outptr[1] = RoundFtoS(acc1);
		outptr += outstride;
	}

	/* save filter state */
	s->phase = phase;
	s->offset = pcmptr - pcmend;
	memmove(s->histbuf, s->histbuf + insamps, s->nhist * sizeof(float));

//	TOCK(outptr - outbuf);
	return (outptr - outbuf);
}

int
RAResampleMonoRat(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst)
{
    state_t *s = (state_t *)inst;
	float *pcmptr, *pcmend;
	float *rwingptr, *lwingptr;
	short *outptr;
	float acc0, acc1;
	int i;
	int up = s->up;
	int phase = s->phase;
	int nwing = s->nwing;
	uchar *pcmstep = s->pcmstep;
	int *nextphase = s->nextphase;

//	TICK();

	/* convert input to float */
	insamps = pCvt->pfCvt(s->pcmbuf,inbuf,insamps,pCvt->pStateMachine) ;

	/* restore filter state */
	pcmptr = s->pcmbuf - (nwing-1);
	pcmend = pcmptr + insamps;
	pcmptr += s->offset;
	outptr = outbuf;

	/* filter */
	while (pcmptr < pcmend) {

		rwingptr = s->filter + nwing * phase;
		lwingptr = s->filter + nwing * (up-1-phase);
		acc0 = acc1 = 0.0f;

		for (i = 0; i < nwing; i++) {
			acc0 += pcmptr[-i-1] * rwingptr[i];
			acc1 += pcmptr[+i+0] * lwingptr[i];
		}
		acc0 += acc1;
	
		pcmptr += pcmstep[phase];
		phase = nextphase[phase];

		outptr[0] = RoundFtoS(acc0);
		outptr += outstride;
	}

	/* save filter state */
	s->phase = phase;
	s->offset = pcmptr - pcmend;
	memmove(s->histbuf, s->histbuf + insamps, s->nhist * sizeof(float));

//	TOCK(outptr - outbuf);
	return (outptr - outbuf);
}

int
RAGetMaxOutputRat(int insamps, void *inst)
{
    state_t *s = (state_t *)inst;
	int inframes, outframes, outsamps;

	inframes = (insamps + (s->nchans-1)) / s->nchans;
	outframes = (int) myCeil((double)inframes * s->outrate / s->inrate);
	outsamps = outframes * s->nchans;

	return outsamps;
}

int
RAGetMinInputRat(int outsamps, void *inst)
{
    state_t *s = (state_t *)inst;
	int inframes, outframes, insamps;

	outframes = (outsamps + (s->nchans-1)) / s->nchans;
	inframes = (int) myCeil((double)outframes * s->inrate / s->outrate);
	insamps = inframes * s->nchans;

	return insamps;
}

int
RAGetDelayRat(void *inst)
{
	state_t *s = (state_t *)inst;
	return (int)(s->nwing * (float)s->outrate / s->inrate) ;
}
