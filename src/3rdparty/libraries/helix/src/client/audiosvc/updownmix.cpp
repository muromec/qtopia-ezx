/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: updownmix.cpp,v 1.7 2006/04/17 23:32:52 milko Exp $
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

#include "hxassert.h"
#include "mixengine.h"
#include "math64.h"
#ifdef HELIX_FEATURE_LIMITER
#include "limiter.h"
#endif

#define MINUS3DB .707106781186547524 /* sqrt(2)/2 */

/*****************************************************************************
 *
 * The upmix and downmix machinery.
 *
 ****************************************************************************/

typedef struct UpMixMachine {
  int offsetIn, offsetOut ;
  const struct UpMixMachine *nextState ;
} tUpMixMachine ;

static const tUpMixMachine up_id[1] =
{
    {
        1,1,
        &up_id[0]
    }
} ;

static const tUpMixMachine up_1_2[2] =
{
    {
        0,1,
        &up_1_2[1]
    },
    {
        1,1,
        &up_1_2[0]
    }
} ;

/* mix mono into L,R instead of C */
static const tUpMixMachine up_1_5[2] =
{
    {
        0,1,
        &up_1_5[1]
    },
    {
        1,4,
        &up_1_5[0]
    }
} ;

/* mix mono into L,R instead of C */
static const tUpMixMachine up_1_6[2] =
{
    {
        0,1,
        &up_1_6[1]
    },
    {
        1,5,
        &up_1_6[0]
    }
} ;

static const tUpMixMachine up_2_5[2] =
{
    {
        1,1,
        &up_2_5[1]
    },
    {
        1,4,
        &up_2_5[0]
    }
} ;

static const tUpMixMachine up_4_6[5] =
{
    {
        1,1,
        &up_4_6[1]
    },
    {
        1,1,
        &up_4_6[2]
    },
    {
        1,2,
        &up_4_6[3]
    },
    {
        0,1,
        &up_4_6[4]
    },
    {
        1,1,
        &up_4_6[0]
    }
} ;

static const tUpMixMachine up_5_6[5] =
{
    {
        1,1,
        &up_5_6[1]
    },
    {
        1,1,
        &up_5_6[2]
    },
    {
        1,2,
        &up_5_6[3]
    },
    {
        1,1,
        &up_5_6[4]
    },
    {
        1,1,
        &up_5_6[0]
    }
} ;

HX_RESULT HXAudioSvcMixEngine::SetupUpDownmix(void)
{
    // we'll need something more intelligent for multichannel
    m_nChannels_2_3 = MIN(m_nChannels_1, m_nChannels_4) ;

    // initialize downmix
    if (m_nChannels_1 != m_nChannels_2_3)
    {
      if (m_nChannels_1 == 2 && m_nChannels_2_3 == 1)
          m_pfDownmix = &HXAudioSvcMixEngine::downmix2_1 ;
      else if (m_nChannels_1 == 5 && m_nChannels_2_3 == 1)
          m_pfDownmix = &HXAudioSvcMixEngine::downmix5_1 ;
      else if (m_nChannels_1 == 6 && m_nChannels_2_3 == 2)
          m_pfDownmix = &HXAudioSvcMixEngine::downmix6_2_matrix ;
      else if (m_nChannels_1 == 5 && m_nChannels_2_3 == 2)
          m_pfDownmix = &HXAudioSvcMixEngine::downmix5_2_matrix ;
      else if (m_nChannels_1 == 4 && m_nChannels_2_3 == 2)
          m_pfDownmix = &HXAudioSvcMixEngine::downmix4_2_matrix ;
      else
          return HXR_FAIL ;

      /* If we downmix, we limit. Special-case stereo to mono: create one bit of
         headroom, don't limit */
#if HELIX_FEATURE_LIMITER
      if (!(m_nChannels_1 == 2 && m_nChannels_2_3 == 1))
          m_pLimiter = LimiterInit(m_ulSampleRate_3_4, m_nChannels_2_3, HEADROOM) ;
      else
#endif
          m_pLimiter = 0 ;
    }
#ifdef HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION
    else if ((m_nChannels_1 == 1 || m_nChannels_1 == 2) && m_nChannels_4 == 2)
	// create headroom for audio level normalization for mono or stereo input
	m_pLimiter = LimiterInit(m_ulSampleRate_3_4, m_nChannels_2_3, ALN_HEADROOM) ;
#endif

    // initialize upmix
    if (m_nChannels_2_3 == m_nChannels_4)
        m_upmixMachine = up_id ;
    else if (m_nChannels_2_3 == 1 && m_nChannels_4 == 2)
        m_upmixMachine = up_1_2 ;
    else if (m_nChannels_2_3 == 1 && m_nChannels_4 == 5)
        m_upmixMachine = up_1_5 ;
    else if (m_nChannels_2_3 == 1 && m_nChannels_4 == 6)
        m_upmixMachine = up_1_6 ;
    else if (m_nChannels_2_3 == 2 && m_nChannels_4 == 5)
        m_upmixMachine = up_2_5 ;
    else if (m_nChannels_2_3 == 5 && m_nChannels_4 == 6)
        m_upmixMachine = up_5_6 ;
    else if (m_nChannels_2_3 == 4 && m_nChannels_4 == 6)
        m_upmixMachine = up_4_6 ;
    else
        return HXR_FAIL ;

    m_clev = m_slev = (int)((1UL<<31) * MINUS3DB) ; // -3 dB is default attenuation

    return HXR_OK ;
}

/* saturated 16-bit add */
INT16 adds16(INT16 a, INT16 b)
{
    INT32 s = (INT32)a+b ;
    if (s > 0x7fff) s = 0x7fff ;
    else if (s < -0x8000) s = -0x8000 ;

    return (INT16)s ;
}

/* saturated 32-bit add */
INT32 adds32(INT32 a, INT32 b)
{
    INT32 s = a+b ;
    INT32 ov = ~(a ^ b) & (a ^ s) & 0x80000000 ; // overflow
    if (ov)
        s = (signed)((1UL<<31)-1)-((s ^ ov) >> 31) ;
    return s ;
}

/* mix into a 32-bit buffer. Returns the number of samples written (more precisely,
   if we mix 2 channels into a 5 channel buffer, and feed 2*n samples, then this
   function will return 5*n samples, regardless into how many channels the 2 original
   channels are mixed. */

UINT32 HXAudioSvcMixEngine::upmix(const tAudioSample *pIn, INT32 *pOut0, const tUpMixMachine *pUpmixMachine, int nSamples, HXBOOL isDirty)
{
    INT32 *pOut = pOut0 ;
    HX_ASSERT(nSamples % m_nChannels_2_3 == 0) ;

    const tAudioSample *pEnd = pIn + nSamples ;

    if (!isDirty)
    {
        while (pIn != pEnd)
        {
            *pOut = adds32(*pIn << (32-NBITS_PER_AUDIOSAMPLE), *pOut);

            pIn  += pUpmixMachine->offsetIn  ;
            pOut += pUpmixMachine->offsetOut ;
            pUpmixMachine = pUpmixMachine->nextState ;
        }
    }
    else
    {
        while (pIn != pEnd)
        {
            INT32 t = (INT32)*pIn << (32-NBITS_PER_AUDIOSAMPLE) ; // -MulShift31(*pIn,pUpmixMachine->gain)
            *pOut = t ;
            pIn  += pUpmixMachine->offsetIn  ;
            pOut += pUpmixMachine->offsetOut ;
            pUpmixMachine = pUpmixMachine->nextState ;
        }
    }

    return pOut - pOut0 ;
}

/* mix into a 16-bit buffer. Returns the number of samples written (more precisely,
   if we mix 2 channels into a 5 channel buffer, and feed 2*n samples, then this
   function will return 5*n samples, regardless into how many channels the 2 original
   channels are mixed. */

UINT32 HXAudioSvcMixEngine::upmix(const tAudioSample *pIn, INT16 *pOut0, const tUpMixMachine *pUpmixMachine, int nSamples, HXBOOL isDirty)
{
    INT16 *pOut = pOut0 ;
    HX_ASSERT(nSamples % m_nChannels_2_3 == 0) ;

    const tAudioSample *pEnd = pIn + nSamples ;

    if (!isDirty)
    {
        while (pIn != pEnd)
        {
            *pOut = adds16((INT16)(*pIn >> (NBITS_PER_AUDIOSAMPLE-16)), *pOut);

            pIn  += pUpmixMachine->offsetIn  ;
            pOut += pUpmixMachine->offsetOut ;
            pUpmixMachine = pUpmixMachine->nextState ;
        }
    }
    else
    {
        while (pIn != pEnd)
        {
            *pOut = (INT16)(*pIn >> (NBITS_PER_AUDIOSAMPLE-16)) ;
            pIn  += pUpmixMachine->offsetIn  ;
            pOut += pUpmixMachine->offsetOut ;
            pUpmixMachine = pUpmixMachine->nextState ;
        }
    }

    return pOut - pOut0 ;
}

void HXAudioSvcMixEngine::downmix2_1(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;
    while (pBuffer != pEnd)
    {
        *pOut++ = (pBuffer[L]>>1) + (pBuffer[R]>>1) ;
        pBuffer += strideIn ;
    }
}

void HXAudioSvcMixEngine::downmix5_1(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,C,Ls,Rs,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;

    const int clev = m_clev >> (HEADROOM-1);
    const int slev = m_slev >> (HEADROOM-2);

    while (pBuffer != pEnd)
    {
        INT32 c  = MulShift32(pBuffer[C], clev) ;
        INT32 cs = MulShift32((pBuffer[Ls]>>1) + (pBuffer[Rs]>>1), slev) ;
        *pOut++  = (pBuffer[L]>>HEADROOM) + c + (pBuffer[R]>>HEADROOM) + cs ;
        pBuffer += strideIn ;
    }
}

// Matrix surround downmix
void HXAudioSvcMixEngine::downmix5_2_matrix(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,C,Ls,Rs,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;

    const int clev = m_clev >> (HEADROOM-1);
    const int slev = m_slev >> (HEADROOM-2);

    while (pBuffer != pEnd)
    {
        INT32 c  = MulShift32(pBuffer[C], clev) ; // creates two bits headroom
        INT32 cs = MulShift32((pBuffer[Ls]>>1) + (pBuffer[Rs]>>1), slev) ; // two bits hr
        pOut[0]  = (pBuffer[L]>>HEADROOM) + c + cs ;
        pOut[1]  = (pBuffer[R]>>HEADROOM) + c - cs ;

        pBuffer += strideIn ;
        pOut    += 2 ;
    }
}

// Stereo downmix
void HXAudioSvcMixEngine::downmix5_2_stereo(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,C,Ls,Rs,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;

    const int clev = m_clev >> (HEADROOM-1);
    const int slev = m_slev >> (HEADROOM-1);

    while (pBuffer != pEnd)
    {
        INT32 c = MulShift32(pBuffer[C], clev) ;
        pOut[0] = (pBuffer[L]>>HEADROOM) + c + MulShift32(pBuffer[Ls], slev) ;
        pOut[1] = (pBuffer[R]>>HEADROOM) + c + MulShift32(pBuffer[Rs], slev) ;

        pBuffer += strideIn ;
        pOut    += 2 ;
    }
}

// Matrix surround downmix
void HXAudioSvcMixEngine::downmix6_2_matrix(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,C,LFE,Ls,Rs,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;

    const int clev = m_clev >> (HEADROOM-1);
    const int slev = m_slev >> (HEADROOM-2);

    while (pBuffer != pEnd)
    {
        INT32 c  = MulShift32(pBuffer[C], clev) ; // creates two bits headroom
        INT32 cs = MulShift32((pBuffer[Ls]>>1) + (pBuffer[Rs]>>1), slev) ; // two bits hr
        pOut[0]  = (pBuffer[L]>>HEADROOM) + c + cs ;
        pOut[1]  = (pBuffer[R]>>HEADROOM) + c - cs ;

        pBuffer += strideIn ;
        pOut    += 2 ;
    }
}

// Stereo downmix
void HXAudioSvcMixEngine::downmix6_2_stereo(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,C,LFE,Ls,Rs,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;

    const int clev = m_clev >> (HEADROOM-1);
    const int slev = m_slev >> (HEADROOM-1);

    while (pBuffer != pEnd)
    {
        INT32 c = MulShift32(pBuffer[C], clev) ;
        pOut[0] = (pBuffer[L]>>HEADROOM) + c + MulShift32(pBuffer[Ls], slev) ;
        pOut[1] = (pBuffer[R]>>HEADROOM) + c + MulShift32(pBuffer[Rs], slev) ;

        pBuffer += strideIn ;
        pOut    += 2 ;
    }
}

// Matrix surround downmix
void HXAudioSvcMixEngine::downmix4_2_matrix(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,C,Cs,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;

    const int clev = m_clev >> (HEADROOM-1);
    const int slev = m_slev >> (HEADROOM-1);

    while (pBuffer != pEnd)
    {
        INT32 c  = MulShift32(pBuffer[C], clev) ; // creates two bits headroom
        INT32 cs = MulShift32(pBuffer[Cs], slev) ; // two bits hr
        pOut[0]  = (pBuffer[L]>>HEADROOM) + c + cs ;
        pOut[1]  = (pBuffer[R]>>HEADROOM) + c - cs ;

        pBuffer += strideIn ;
        pOut    += 2 ;
    }
}

// Stereo downmix
void HXAudioSvcMixEngine::downmix4_2_stereo(tAudioSample *pBuffer, int nSamples)
{
    enum { L,R,C,Cs,strideIn } ; // input channel ordering

    HX_ASSERT(nSamples % strideIn == 0) ;

    const tAudioSample *pEnd = pBuffer + nSamples ;
    tAudioSample *pOut = pBuffer;

    const int clev = m_clev >> (HEADROOM-1);
    const int slev = m_slev >> (HEADROOM-1);

    while (pBuffer != pEnd)
    {
        INT32 c = MulShift32(pBuffer[C], clev) ;
        INT32 cs = MulShift32(pBuffer[Cs], slev) ; // two bits hr
        pOut[0] = (pBuffer[L]>>HEADROOM) + c + cs ;
        pOut[1] = (pBuffer[R]>>HEADROOM) + c + cs ;

        pBuffer += strideIn ;
        pOut    += 2 ;
    }
}
