/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: gain.c,v 1.4 2007/07/06 20:21:24 jfinnecy Exp $
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

#include <stdlib.h>
#include <math.h>
#include "hxassert.h"

#include "gain.h"
#include "math64.h"

struct GAIN_STATE
{
    int sampleRate ;
    int nChannels ;
    int headRoom ;
    INT32 instGain ; /* gain applied right now */
    INT32 tgtGain ;  /* in a smooth gain change, the gain we are aiming for */
    int shift ;
} ;

enum
{
    zeroDBGain = (1L<<30)
} ;

GAIN_STATE* gainInit(int sampleRate, int nChannels, int headRoom)
{
    GAIN_STATE* g = (GAIN_STATE*) calloc(1,sizeof(GAIN_STATE)) ;
    if (g)
    {
        g->sampleRate = sampleRate ;
        g->nChannels = nChannels ;
        g->headRoom = headRoom ;
        gainSetTimeConstant(0.1f, g) ;
    }

    return g ;
}

void gainFree(GAIN_STATE* g)
{
    if (g) free(g) ;
}

float gainSetSmooth(float dB, GAIN_STATE* g)
{
    unsigned int gain = (unsigned int)(0.5 + (double)(1UL<<(30-g->headRoom)) * pow(10.0, 0.05*dB)) ;

    if (dB > 20.0 * log10(1<<g->headRoom))
    {
        gain = zeroDBGain; dB = 20.0f * (float)log10(1<<g->headRoom) ; // avoid overflow
    }

    g->tgtGain = (INT32)gain ;

    return dB ;
}

float gainSetImmediate(float dB, GAIN_STATE* g)
{
    dB = gainSetSmooth(dB, g) ;

    g->instGain = g->tgtGain ; // make it instantaneous

    return dB ;
}

int gainSetTimeConstant(float millis, GAIN_STATE* g)
{
    // we define the time constant millis so that the signal has decayed to 1/2 (-6dB) after
    // millis milliseconds have elapsed.
    // Let T[sec] = millis/1000 = time constant in units of seconds
    //
    // => (1-2^-s)^(T[sec]*sr) = 1/2
    // => 1-2^-s = (1/2)^(1/(T[sec]*sr))
    // => 2^-s = 1 - (1/2)^(1/(T[sec]*sr))
    // => s = -log2(1 - (1/2)^(1 / (T[sec]*sr)))

    // first 0.5 is rounding constant
    g->shift = (int)(0.5 - 1.0/log(2.0)*log(1.0 - pow(0.5, 1000.0/(millis * g->sampleRate)))) ;
    if (g->shift < 1)
        g->shift = 1 ;
    if (g->shift > 31)
        g->shift = 31 ;

    return 1 ; // OK
}

static void gainFeedMono(INT32* signal, int nSamples, GAIN_STATE *g)
{
    INT32 tgtGain = g->tgtGain ;
    INT32 gain = g->instGain ;
    INT32 *bufferEnd = signal + nSamples ;

    if (gain == tgtGain)
    { // steady state
	while (signal != bufferEnd)
	{
	    *signal = MulShift30(*signal, gain) ; signal++ ;
	}
    }
    else
    { // while we are still ramping the gain
        int shift = g->shift ;
	while (signal != bufferEnd)
	{
	    int rc = (tgtGain > gain) - (tgtGain < gain) ; // -1,0,1 for x<y, x=y, x>y
	    *signal = MulShift30(*signal, gain) ; signal++ ;
	    gain += ((tgtGain-gain) >> shift) + rc ;
	}
        g->instGain = gain ;
    }
}

static void gainFeedStereo(INT32* signal, int nSamples, GAIN_STATE *g)
{
    INT32 tgtGain = g->tgtGain ;
    INT32 gain = g->instGain ;
    INT32 *bufferEnd = signal + nSamples ;

    HX_ASSERT(nSamples % 2 == 0);

    if (gain == tgtGain)
    { // steady state
	while (signal != bufferEnd)
	{
	    *signal = MulShift30(*signal, gain) ; signal++ ;
	    *signal = MulShift30(*signal, gain) ; signal++ ;
	}
    }
    else
    { // while we are still ramping the gain
        int shift = g->shift ;
	while (signal != bufferEnd)
	{
	    int rc = (tgtGain > gain) - (tgtGain < gain) ; // -1,0,1 for x<y, x=y, x>y
	    *signal = MulShift30(*signal, gain) ; signal++ ;
	    *signal = MulShift30(*signal, gain) ; signal++ ;
	    gain += ((tgtGain-gain) >> shift) + rc ;
	}
        g->instGain = gain ;
    }
}

static void gainFeedMulti(INT32* signal, int nSamples, GAIN_STATE *g)
{
    INT32 tgtGain = g->tgtGain ;
    INT32 gain = g->instGain ;
    INT32 *bufferEnd = signal + nSamples ;

    HX_ASSERT(nSamples % g->nChannels == 0);

    if (gain == tgtGain)
    { // steady state
	while (signal != bufferEnd)
	{
	    int i ;
	    for (i = 0 ; i < g->nChannels ; i++)
	    {
		*signal = MulShift30(*signal, gain) ; signal++ ;
	    }
	}
    }
    else
    { // while we are still ramping the gain
        int shift = g->shift ;
	while (signal != bufferEnd)
	{
	    int rc = (tgtGain > gain) - (tgtGain < gain) ; // -1,0,1 for x<y, x=y, x>y
	    int i ;
	    for (i = 0 ; i < g->nChannels ; i++)
	    {
		*signal = MulShift30(*signal, gain) ; signal++ ;
	    }
	    gain += ((tgtGain-gain) >> shift) + rc ;
	}
        g->instGain = gain ;
    }
}

void gainFeed(INT32* signal, int nSamples, GAIN_STATE* g)
{
    /* if the gain is 0dB, and we are not currently ramping, shortcut. */
    if (g->instGain == zeroDBGain && g->instGain == g->tgtGain)
    {
        return ;
    }
    switch (g->nChannels)
    {
    case 1:
        gainFeedMono(signal, nSamples, g) ;
        break ;
    case 2:
        gainFeedStereo(signal, nSamples, g) ;
        break ;
    default:
        gainFeedMulti(signal, nSamples, g) ;
        break ;
    }
}
