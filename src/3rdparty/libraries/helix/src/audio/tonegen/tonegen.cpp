/* ***** BEGIN LICENSE BLOCK *****
 *
 * Source last modified: $Id:
 *
 * Copyright Notices:
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 *
 * Patent Notices: This file may contain technology protected by one or
 * more of the patents listed at www.helixcommunity.org
 *
 * 1.   The contents of this file, and the files included with this file,
 * are protected by copyright controlled by RealNetworks and its
 * licensors, and made available by RealNetworks subject to the current
 * version of the RealNetworks Public Source License (the "RPSL")
 * available at  * http://www.helixcommunity.org/content/rpsl unless
 * you have licensed the file under the current version of the
 * RealNetworks Community Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply.  You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * 2.  Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above.  Please note that RealNetworks and its
 * licensors disclaim any implied patent license under the GPL.
 * If you wish to allow use of your version of this file only under
 * the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting Paragraph 1 above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete Paragraph 1 above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 *
 * This file is part of the Helix DNA Technology.  RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.   Copying, including reproducing, storing,
 * adapting or translating, any or all of this material other than
 * pursuant to the license terms referred to above requires the prior
 * written consent of RealNetworks and its licensors
 *
 * This file, and the files included with this file, is distributed
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * Fixed-point tone generator for JSR135
 * 2006 Ken Cooke (kenc@real.com)
 */

#include "tonegen.h"
#include "hlxclib/stdlib.h"

#define NBUF 65536
/*
 * 1.0 to 0.5 in -0.5dB steps, in Q16
 *
 * for (i = 0; i < 12; i++)
 *     vol_tab[i] = 0xffff * pow(0.5, i/12.0);
 */
static const unsigned short vol_tab[12] = {
	0xffff, 0xf1a0, 0xe411, 0xd744, 0xcb2f, 0xbfc7,
	0xb504, 0xaadb, 0xa144, 0x9837, 0x8fac, 0x879b,
};

/*"raised cosine table" .5 + .5 cos(0..pi)*/
static const unsigned short cos_tble[221]={
	32765,   32760,   32752,   32740,   32725,   32707,   32685,   32661,   32633,   32601,
	32567,   32529,   32488,   32443,   32395,   32345,   32290,   32233,   32173,   32109,
	32042,   31972,   31899,   31822,   31743,   31660,   31574,   31486,   31394,   31299,
	31201,   31100,   30997,   30890,   30780,   30668,   30552,   30434,   30313,   30189,
	30062,   29932,   29800,   29665,   29527,   29387,   29244,   29098,   28950,   28799,
	28646,   28491,   28332,   28172,   28009,   27844,   27676,   27506,   27334,   27160,
	26983,   26805,   26624,   26441,   26256,   26069,   25881,   25690,   25497,   25303,
	25107,   24909,   24709,   24507,   24304,   24100,   23893,   23686,   23477,   23266,
	23054,   22840,   22626,   22410,   22193,   21974,   21755,   21534,   21313,   21090,
	20867,   20642,   20417,   20191,   19964,   19736,   19508,   19279,   19049,   18819,
	18589,   18358,   18126,   17895,   17663,   17430,   17198,   16965,   16732,   16499,
	16267,   16034,   15801,   15568,   15336,   15103,   14871,   14640,   14408,   14177,
	13947,   13717,   13487,   13258,   13030,   12802,   12575,   12349,   12124,   11899,
	11676,   11453,   11232,   11011,   10792,   10573,   10356,   10140,   9926 ,   9712 ,
	9500 ,   9289 ,   9080 ,   8873 ,   8666 ,   8462 ,   8259 ,   8057 ,   7857 ,   7659 ,
	7463 ,   7269 ,   7076 ,   6885 ,   6697 ,   6510 ,   6325 ,   6142 ,   5961 ,   5783 ,
	5606 ,   5432 ,   5260 ,   5090 ,   4922 ,   4757 ,   4594 ,   4434 ,   4275 ,   4120 ,
	3967 ,   3816 ,   3668 ,   3522 ,   3379 ,   3239 ,   3101 ,   2966 ,   2834 ,   2704 ,
	2577 ,   2453 ,   2332 ,   2214 ,   2098 ,   1986 ,   1876 ,   1769 ,   1666 ,   1565 ,
	1467 ,   1372 ,   1280 ,   1192 ,   1106 ,   1023 ,   944  ,   867  ,   794  ,   724  ,
	657  ,   593  ,   533  ,   476  ,   421  ,   371  ,   323  ,   278  ,   237  ,   199  ,
	165  ,   133  ,   105  ,   81   ,   59   ,   41   ,   26   ,   14   ,   6    ,   1    ,
	0
};

/*
 * Frequency for notes C9 to B9, in Q16
 * A9 is exactly 32*440 Hz
 *
 * for (i = -9; i <= 2; i++)
 *     note_tab[i+9] = (1<<16) * 14080.0 * pow(2.0, i/12.0);
 */
static const int note_tab[12] = {
	0x20b404a1, 0x22a5d81c, 0x24b545c7, 0x26e41040, 0x293414f2, 0x2ba74dac,
	0x2e3fd24f, 0x30ffda9c,	0x33e9c015, 0x37000000, 0x3a453d88, 0x3dbc4400,
};


/*
 * Minimax polynomial for cos(x*pi/2) = 1+P(x^2) over x=[-1,1]
 * coefs scaled by 8, 4, 2, 1, .5, .25
 */
static const int poly_tab[6] = {
	0xfff9c121, 0x00784a27, 0xfaa8bdc3, 0x20783ca5, 0xb10b0ce6, 0x1fffffff
};

CTONEGen::CTONEGen()
{
    m_ulSamprate = 0;
}

CTONEGen::~CTONEGen()
{

}

/*
 * Initialize the tone generator state.
 * Parameters as defined in JSR135.
 */
void CTONEGen::SetSamprate(int samplerate)
{
    m_ulSamprate = samplerate;
}

void CTONEGen::ToneInit(ts_t *ts, char vol, char note, int samprate)
{

    int ampl, w, cosw, sinw;
    unsigned freq, fnyq;

    vol = MAX(vol, 0);
    vol = MIN(vol, 100);
    SetSamprate(samprate);

    if(note == -1)
    {
        /* generate silence */
        ts->x = 0;
        ts->y = 0;
        ts->k = 0;
    }
    else
    {
        note = MAX(note, 0);

        samprate = MAX(samprate, 1);
        samprate = MIN(samprate, 131071);

        ampl = VolToAmpl(vol);		/* amplitude in Q31 */
        freq = NoteToFreq(note);	/* tone freq in Q16 */
        fnyq = samprate << 15;		/* Nyquist freq in Q16 */

        if (freq < fnyq) {

	        w = udivhi(freq, fnyq) >> 1;	/* freq/fnyq in Q31 */
	        cosw = fixcos(w);
	        sinw = fixsin(w);

	        ts->x = 0;
	        ts->y = mulhi(ampl, cosw);	/* ampl * cos(w) in Q30 */
	        ts->k = sinw;				/* 2.0 * sin(w) in Q30 */

        } else {

	        /* generate silence */
	        ts->x = 0;
	        ts->y = 0;
	        ts->k = 0;
        }
    }
}

/*
 * Translate volume to amplitude, in Q31
 *
 * Uses logarithmic scale with 0.5dB steps:
 *     100 ->   0dB
 *      50 -> -25dB
 *       0 -> -50dB
 */
int CTONEGen::VolToAmpl(char vol)
{
	int atten, shift, index;

	/* attenuation, in -0.5dB steps */
	atten = 100 - vol;

	/* factor into shifts and remainder */
	shift = atten / 12;
	index = atten - (shift * 12);

	return (MAX_AMPL * vol_tab[index]) >> shift;
}

/*
 * Translate note to frequency, in Q16
 */
unsigned int CTONEGen::NoteToFreq(char note)
{
	int shift, index;

	/* factor into octaves and remainder */
	shift = note / 12;
	index = note - (shift * 12);

	return note_tab[index] >> (10 - shift);
}

/*
 * Computes cos(w*pi/2) for w=[-1,1] in Q31
 * error max=0 min=-11
 */
int CTONEGen::fixcos(int w)
{
	int w2, acc;

	w2 = mulhi(w, w) << 1;
	w2 ^= w2 >> 31;		/* fix 0x80000000 */

	acc = poly_tab[0];
	acc = mulhi(acc, w2) + poly_tab[1];
	acc = mulhi(acc, w2) + poly_tab[2];
	acc = mulhi(acc, w2) + poly_tab[3];
	acc = mulhi(acc, w2) + poly_tab[4];
	acc = mulhi(acc, w2) + poly_tab[5];

	return acc << 2;	/* Q31 */
}

/*
 * Computes sin(w*pi/2) for w=[-1,1] in Q31
 * error max=10 min=-10
 */
int CTONEGen::fixsin(int w)
{
	if (w < 0)
		return -fixcos(0x7fffffff + w);
	else
		return fixcos(0x7fffffff - w);
}

/*
 * Generate tone into pcm[] for nsamples.
 *
 * Implemented as a digital oscillator in modified coupled form, because
 * the digital waveguide oscillator seems to be patented (US 5,701,393)
 */
void CTONEGen::ToneGen(ts_t *ts, short pcm[], int nsamples)
{
	int i, x, y, k;

	x = ts->x;
	y = ts->y;
	k = ts->k;

	for (i = 0; i < nsamples; i++) {

		pcm[i] = x >> 15;

		x += mulhi(k, y) << 2;	/* Q30 */
		y -= mulhi(k, x) << 2;	/* Q30 */
	}

	ts->x = x;
	ts->y = y;
	ts->k = k;
#ifdef Tone_Debug_Log
    ToneDebug(pcm,nsamples,m_ulSamprate);
#endif
}

void CTONEGen::ToneDebug(short pcm[], int nsamples, int samprate)
{
	int i, peak, last, zc;

	peak = 0;
	last = 0;
	zc = 0;

	for (i = 0; i < nsamples; i++) {

		/* find peak value */
		if (abs(pcm[i]) > peak)
			peak = abs(pcm[i]);

		/* count zero-crossings to estimate freq */
		if ((pcm[i] >> 16) ^ (last >> 16))
			zc++;

		last = pcm[i];
	}
	printf("freq = %5.0f ", 0.5 * samprate * zc / nsamples);
	printf("ampl = %d (%0.1fdB) \n", peak, 20*log10(peak/32767.0));
}

void CTONEGen::fadein_loop(short *output)
{
    UINT32 lfadein = 0;
    UINT16 i=0;
    for(i=0;i<221;i++)
    {
	    lfadein = (UINT32)(output[i] * cos_tble[i]);
	    output[i]=(short)(lfadein>>15);
	}
}

void CTONEGen::fadeout_loop(short *output)
{
    long lfadeout = 0;
    UINT16 i=0;
    for(i=0;i<221;i++)
    {
	    lfadeout= (long)(output[i] * cos_tble[221-i]);
	    output[i]=(short)(lfadeout>>15);
    }

}

