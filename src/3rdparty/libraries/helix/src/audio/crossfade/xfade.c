/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xfade.c,v 1.4 2007/07/06 20:21:11 jfinnecy Exp $
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

#include "xfade.h"
#include "hxassert.h"
#include "math64.h"

#ifndef MIN
#define MIN(a,b) ((a)<=(b)?(a):(b))
#endif

#define HEADROOM 0

#define NALPHA 256
#define FRACBITS 16
#define EXTRABITS 8
#define FRACMASK ((1<<FRACBITS) - 1)
#define FRACROUND (1<<(FRACBITS-1))

#define BATCHSIZE 512

struct XFADER_STATE
{
    int nChannels ;
    int batchSize ;
    unsigned int tabacc ;
    unsigned int tabstep ;
    const struct COEFF *coeff ;
};

XFADER_STATE* XFader_init(int sampleRate, int nChannels, const struct COEFF* coeff)
{
    XFADER_STATE* instance = (XFADER_STATE*) calloc(1,sizeof(XFADER_STATE)) ;

    if (instance)
    {
        instance->nChannels = nChannels ;
        instance->coeff = coeff ;
        instance->tabacc = NALPHA << FRACBITS ; // end of fade

        // make sure batchsize is divisible by nChannels
        instance->batchSize = BATCHSIZE - BATCHSIZE % nChannels ;
    }

    return instance ;
}

void XFader_free(XFADER_STATE* instance)
{
    if (instance) free(instance) ;
}

void XFader_start(int nSamples, XFADER_STATE* instance)
{
    nSamples /= instance->nChannels ;
    instance->tabacc = 0 ;

    if (nSamples < 1) nSamples = 1 ;

    // (tabstep * nSamples) >> FRACBITS = NALPHA

//    instance->tabstep = ((NALPHA << FRACBITS) + nSamples - 1) / nSamples ; // round up
    instance->tabstep = ((NALPHA << FRACBITS) + nSamples/2) / nSamples ; // round nearest
}

int XFader_active(XFADER_STATE* instance)
{
    unsigned int tabacc = instance->tabacc ;
    unsigned int tabint = tabacc >> FRACBITS ;
    return (tabint < NALPHA) ;
}

/* In-place crossfading functionality currently untested */
#if 0
static int XFader_feed_mono(const INT32* in, INT32* inout, int nSamples, XFADER_STATE* instance)
{
    int i ;
    unsigned int tabacc = instance->tabacc ;
    unsigned int tabstep = instance->tabstep ;
    unsigned int tabint = tabacc >> FRACBITS ;

    for (i = 0 ; i < nSamples && tabint < NALPHA ; i++)
    {
        INT32 gain1, gain2 ;

        /* interpolate new alpha */
        gain1 = instance->coeff[tabint].gain ;
        gain1 += (instance->coeff[tabint].delta * (signed)(tabacc & FRACMASK)) >> (FRACBITS - EXTRABITS) ;

        gain2 = instance->coeff[tabint+NALPHA+1].gain ;
        gain2 += (instance->coeff[tabint+NALPHA+1].delta * (signed)(tabacc & FRACMASK)) >> (FRACBITS - EXTRABITS) ;

        /* next table step */
        tabacc += tabstep;
        tabint = tabacc >> FRACBITS;

        // creates one guard bit
        *inout = MulShift31(*in++, gain1) + MulShift31(*inout, gain2) ;
        inout++ ;
    }

    instance->tabacc = tabacc ;

    return nSamples ;
}

static int XFader_feed_stereo(const INT32* in, INT32* inout, int nSamples, XFADER_STATE* instance)
{
    int i ;
    unsigned int tabacc = instance->tabacc ;
    unsigned int tabstep = instance->tabstep ;
    unsigned int tabint = tabacc >> FRACBITS ;

    for (i = 0 ; i < nSamples && tabint < NALPHA ; i+=2)
    {
        INT32 gain1, gain2 ;

        /* interpolate new alpha */
        gain1 = instance->coeff[tabint].gain ;
        gain1 += (instance->coeff[tabint].delta * (signed)(tabacc & FRACMASK)) >> (FRACBITS - EXTRABITS) ;

        gain2 = instance->coeff[tabint+NALPHA+1].gain ;
        gain2 += (instance->coeff[tabint+NALPHA+1].delta * (signed)(tabacc & FRACMASK)) >> (FRACBITS - EXTRABITS) ;

        /* next table step */
        tabacc += tabstep;
        tabint = tabacc >> FRACBITS;

        // creates one guard bit
        *inout = MulShift31(*in++, gain1) + MulShift31(*inout, gain2) ;
        inout++ ;
        *inout = MulShift31(*in++, gain1) + MulShift31(*inout, gain2) ;
        inout++ ;
    }

    instance->tabacc = tabacc ;

    return nSamples ;
}

static const INT32 silence[BATCHSIZE] ; // a bunch of zeros

int XFader_feed(const INT32* in, INT32* inout, int nSamples, XFADER_STATE* instance)
{
    int nRead = 0 ;

    while (nRead < nSamples)
    {
        int nBatch = MIN(nSamples - nRead, instance->batchSize) ;
        const INT32 *in1 = in ? (in + nRead) : silence ;
        int i ;

        switch (instance->nChannels)
        {
        case 1:
            i = XFader_feed_mono  (in1, inout + nRead, nBatch, instance) ;
            break ;
        case 2:
            i = XFader_feed_stereo(in1, inout + nRead, nBatch, instance) ;
            break ;
        }

#if HEADROOM > 0
        for (; i < nBatch ; i++)
        {
            inout[nRead + i] >>= HEADROOM ;
        }
#endif

        nRead += nBatch ;
    }

    return nSamples ;
}
#endif

int Fader_feed_mono(INT32 *inout, int nSamples, int fadeout, XFADER_STATE* instance)
{
    int i ;
    unsigned int tabacc = instance->tabacc ;
    unsigned int tabstep = instance->tabstep ;
    unsigned int tabint = tabacc >> FRACBITS ;
    const struct COEFF *coeff = instance->coeff + (fadeout ? 0 : (NALPHA+1)) ;

    for (i = 0 ; i < nSamples && tabint < NALPHA ; i++)
    {
        /* interpolate new alpha */
        INT32 gain = coeff[tabint].gain ;
        gain += (coeff[tabint].delta * (signed)(tabacc & FRACMASK)) >> (FRACBITS - EXTRABITS) ;

        /* next table step */
        tabacc += tabstep;
        tabint = tabacc >> FRACBITS;

        inout[0] = MulShift30(inout[0], gain) ;
        inout++ ;
    }

    instance->tabacc = tabacc ;

    return i ;
}

int Fader_feed_stereo(INT32 *inout, int nSamples, int fadeout, XFADER_STATE* instance)
{
    int i ;
    unsigned int tabacc = instance->tabacc ;
    unsigned int tabstep = instance->tabstep ;
    unsigned int tabint = tabacc >> FRACBITS ;
    const struct COEFF *coeff = instance->coeff + (fadeout ? 0 : (NALPHA+1)) ;

    for (i = 0 ; i < nSamples && tabint < NALPHA ; i+=2)
    {
        /* interpolate new alpha */
        INT32 gain = coeff[tabint].gain ;
        gain += (coeff[tabint].delta * (signed)(tabacc & FRACMASK)) >> (FRACBITS - EXTRABITS) ;

        /* next table step */
        tabacc += tabstep;
        tabint = tabacc >> FRACBITS;

        inout[0] = MulShift30(inout[0], gain) ;
        inout[1] = MulShift30(inout[1], gain) ;
        inout+=2 ;
    }

    instance->tabacc = tabacc ;

    return i ;
}

int Fader_feed(INT32 *inout, int nSamples, int fadeout, XFADER_STATE* instance)
{
    int i ;
    HX_ASSERT(fadeout ==1 || fadeout==0);

    switch (instance->nChannels)
    {
    case 1:
        i = Fader_feed_mono  (inout, nSamples, fadeout, instance) ;
        break ;
    case 2:
        i = Fader_feed_stereo(inout, nSamples, fadeout, instance) ;
        break ;
    }

    if (fadeout)
        for (; i < nSamples ; i++) inout[i] = 0 ;
#if HEADROOM > 0
    else
        for (; i < nSamples ; i++) inout[i] >>= HEADROOM ;
#endif

    return nSamples ;
}

#if 0 // use just for table initialization
static void setupDelta(struct COEFF* c)
{
    int i ;

    for (i = 0 ; i < NALPHA ; i++)
    {
        c[i].delta = (c[i+1].gain - c[i].gain + (1<<(EXTRABITS-1))) >> EXTRABITS ;
        assert(c[i].delta < (1<<(31-FRACBITS)));
        c[i+NALPHA+1].delta = (c[i+NALPHA+2].gain - c[i+NALPHA+1].gain + (1<<(EXTRABITS-1))) >> EXTRABITS ;
        assert(c[i+NALPHA+1].delta < (1<<(31-FRACBITS)));
    }
    c[i].delta = c[i+NALPHA+1].delta = 0 ;
}

#include <math.h>
#include <assert.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct COEFF* sintab(void)
{
    int i ;
    struct COEFF* c = (struct COEFF*) calloc(NALPHA+1, 2*sizeof(struct COEFF)) ;

    for (i = 0 ; i <= NALPHA ; i++)
    {
        double x = i * 0.5 * M_PI / NALPHA ;
        double x1 = cos(x) ;
        double x2 = sin(x) ;

        x1 *= x1 ;
        x2 *= x2 ;

        c[i].gain          = (int)floor(0.5 + x1*(1L<<(30-HEADROOM))) ;
        c[i+NALPHA+1].gain = (int)floor(0.5 + x2*(1L<<(30-HEADROOM))) ;
    }
    setupDelta(c) ;


    {
      FILE *f = fopen("c:\\temp\\table.c","w") ;

      fprintf(f,"const struct COEFF XFader_sin2tab[2*%d] = {\n /* fade out table */\n",
        NALPHA+1) ;
      for (i=0; i<NALPHA; i+=2)
      {
        fprintf(f," 0x%08lx, -0x%08lx, 0x%08lx, -0x%08lx,\n",
          c[i+0].gain, -c[i+0].delta,
          c[i+1].gain, -c[i+1].delta
          );
      }
      fprintf(f," 0x%08lx, -0x%08lx,\n /* fade in table */\n",
          c[NALPHA].gain, -c[NALPHA].delta) ;

      for (i=NALPHA+1; i<2*NALPHA+1; i+=2)
      {
        fprintf(f," 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n",
          c[i+0].gain, c[i+0].delta,
          c[i+1].gain, c[i+1].delta
          );
      }
      fprintf(f," 0x%08lx, 0x%08lx,\n",
          c[2*NALPHA+1].gain, c[2*NALPHA+1].delta) ;
      fprintf(f,"};\n");
      fclose(f) ;
    }

    return c ;
}
#endif
