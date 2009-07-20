/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: kaiser.c,v 1.3 2004/07/09 18:37:31 hubbe Exp $
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

#include "hlxclib/math.h"

#include "kaiser.h"
#include "allresamplers.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define IZEROEPS (1E-21)	/* max error in Izero */

/*
 * KaiserEstim() estimates the window length and beta needed to
 * meet the given filter specifications.
 *
 * fpass, fstop are normalized freq (1.0 == Nyquist).
 * atten is stopband attenuation in dB.
 */
void
KaiserEstim(float fpass, float fstop, float atten, int *length, float *beta)
{
	double d, b;

	/* estimate the required beta */
	if (atten < 21.0f) {
		b = 0.0;
	} else if (atten <= 50.0f) {
		b = 0.5842 * pow((atten-21.0), 0.4) + 0.07886 * (atten-21.0);
	} else {	/* atten > 50 */
		b = 0.1102 * (atten-8.7);
	}
	*beta = (float)b;

	/* estimate the required length */
	d = (atten - 7.95) / (M_PI * 2.285);
	*length = 1 + (int)(d / (fstop - fpass));
}

/*
 *             inf
 * Io(x) = 1 + sum(((x/2)^r / r!)^2)
 *             r=1
 */
double
Izero(double x)
{
	double halfx, term, sum, r;
	double temp;

	halfx = 0.5 * x;
	term = sum = r = 1.0;

	do {
		temp = halfx / r;
		term *= temp * temp;
		sum += term;
		r += 1.0;
	} while (term >= (sum * IZEROEPS));

	return sum;
}

/*
 * KaiserLowpass() creates a Kaiser-windowed lowpass filter.
 *
 * length is length of filter wing.
 * cutoff is normalized cutoff freq (-6dB down).
 * beta is the Kaiser window parameter.
 * gain is the desired dc gain.
 *
 * The Kaiser window is given by:
 * w[n] = Io(beta * sqrt(1 - (n/M)^2)) / Io(beta)	(0 <= n <= M)
 *
 * Note: sampling of the "analog" lowpass is offset by 0.5 sample,
 * so that all phases are an even length.
 */
void 
KaiserLowpass(int length, float cutoff, float beta, float gain, double *filter)
{
	double ibeta, ilength, x, w;
	int i;

	ibeta = 1.0 / Izero(beta);
	ilength = 1.0 / (length - 0.5);		/* last window value should be ibeta */

	for (i = 0; i < length; i++) {
		x = i + 0.5;

		/* Kaiser window */
		w = x * ilength;
		w = 1.0 - w * w;
		w = MAX(w, 0.0);
		w = Izero(beta * sqrt(w)) * ibeta;

		/* windowed ideal lowpass filter */
		filter[i] = w * gain * sin(cutoff * M_PI * x) / (M_PI * x);
	}
}

