/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mixengine.cpp,v 1.23 2009/05/01 17:32:06 sfu Exp $
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


#include "hlxclib/limits.h"
#include "hlxclib/math.h"

#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"

#include "mixengine.h"

#ifdef HELIX_FEATURE_RESAMPLER
#include "RAResampler.h"
#endif
#ifdef HELIX_FEATURE_GAINTOOL
#include "gain.h"
#endif
#ifdef HELIX_FEATURE_CROSSFADE
#include "xfade.h"
#endif
#ifdef HELIX_FEATURE_LIMITER
#include "limiter.h"
#endif

#include "math64.h"

/*
   DSP is done in four stages:

   - input
       (1)
   - downmix
       (2)
   - resample
       (3)
   - mix into output buffer
       (4)

   In the general case, the number of samples/call in pipes (1)-(4) will be different.
   In the code below, nSamples_X denotes the number of samples flowing in pipe (X)

   For example, say the input is 44100 stereo, and the output is 22050 3-channel,
   and for some reason we want to downmix to mono before going through the resampler.

   if nSamples_4 == 3 * 22050 (1 seconds' worth of data), then
   ulSamples_3 = nSamples_4 / 3 = 22050
   nSamples_2 = ulSamples_3 * 44100/22050 = 44100
   nSamples_1 = nSamples_2 * 2 / 1 = 88200 (1 seconds' worth of data on the input);

   For convenience, we might also count sample frames; that's samples_X/nChannels_X

   All variables are postfixed by a indicator of where they are relevant; for example,
   m_nChannels_2 is the number of channels after downmixing, and m_pBuffer3 is the
   output buffer for the resampler.
 */

HXAudioSvcMixEngine::HXAudioSvcMixEngine()
: m_ulBytesPerSample(2)
, m_pBuffer_1(0)
, m_pBuffer_3(0)
, m_pResampler(0)
, m_eCrossFadeDirection(FADE_OUT)
, m_pXFader(0)
, m_pGaintool(0)
, m_pLimiter(0)
{}

HXAudioSvcMixEngine::~HXAudioSvcMixEngine()
{
    releaseResources() ;
}

void HXAudioSvcMixEngine::releaseResources()
{
    if (m_pBuffer_1) delete[] m_pBuffer_1 ; m_pBuffer_1 = 0 ;
    if (m_pBuffer_3) delete[] m_pBuffer_3 ; m_pBuffer_3 = 0 ;
#ifdef HELIX_FEATURE_RESAMPLER
    if (m_pResampler) delete m_pResampler ; m_pResampler = 0 ;
#endif /* HELIX_FEATURE_RESAMPLER */
#ifdef HELIX_FEATURE_GAINTOOL
    if (m_pGaintool) gainFree(m_pGaintool); m_pGaintool = 0 ;
#endif
#ifdef HELIX_FEATURE_CROSSFADE
    if (m_pXFader) XFader_free(m_pXFader) ; m_pXFader = 0 ;
#endif
#ifdef HELIX_FEATURE_LIMITER
    if (m_pLimiter) LimiterFree(m_pLimiter); m_pLimiter = 0 ;
#endif
}

HX_RESULT HXAudioSvcMixEngine::SetSampleConverter(CAudioSvcSampleConverter *pCvt)
{
    m_pCvt = pCvt ;
    return pCvt ? HXR_OK : HXR_FAIL ;
}

HX_RESULT HXAudioSvcMixEngine::SetupResamplerAndBuffers(void)
{
    if (m_ulSampleRate_1_2 == m_ulSampleRate_3_4)
    {
        // no resampling.
        m_ulChunkSize_1 = BATCHSIZE ;
        m_ulChunkSize_1 -= m_ulChunkSize_1 % m_nChannels_1 ;

        m_ulChunkSize_3 = m_ulChunkSize_1 / m_nChannels_1 * m_nChannels_2_3 ;
    }
    else
    {
#ifdef HELIX_FEATURE_RESAMPLER
        HX_RESULT res = RAExactResampler::Create(&m_pResampler, m_ulSampleRate_1_2, m_ulSampleRate_3_4, m_nChannels_2_3, NBITS_PER_AUDIOSAMPLE == 32 ? RAExactResampler::_INT32 : RAExactResampler::_INT16) ;
        if (FAILED(res))
            return res ;

        // determine the chunk sizes on resampler input and output. The side with the higher
        // datarate limits the other side
        if (m_nChannels_1 * m_ulSampleRate_1_2 <= m_nChannels_2_3 * m_ulSampleRate_3_4)
        {
            // downstream (right) side limits size
            m_ulChunkSize_3 = BATCHSIZE ;
            m_ulChunkSize_3 -= m_ulChunkSize_3 % m_nChannels_2_3 ;

            m_ulChunkSize_1 = m_pResampler->GetMinInput(m_ulChunkSize_3) ;
            m_ulChunkSize_1 = m_ulChunkSize_1 / m_nChannels_2_3 * m_nChannels_1 ;
        }
        else
        {
            // upstream side limits size
            m_ulChunkSize_1 = BATCHSIZE ;
            m_ulChunkSize_1 -= m_ulChunkSize_1 % m_nChannels_1 ;

            m_ulChunkSize_3 = m_pResampler->GetMaxOutput(m_ulChunkSize_1 / m_nChannels_1 * m_nChannels_2_3) ;

            while ((unsigned)m_pResampler->GetMinInput(m_ulChunkSize_3) / m_nChannels_2_3 * m_nChannels_1 > m_ulChunkSize_1)
            {
                m_ulChunkSize_3 -= m_nChannels_2_3 ;
            }
        }
	m_ulResamplerMaxOutputPerInputFrame = m_pResampler->GetMaxOutput(m_nChannels_2_3);
        m_ulBufferSize_3 = m_ulChunkSize_3 + m_ulResamplerMaxOutputPerInputFrame;
#else
        return HXR_NOTIMPL ; // resampler not implemented
#endif
    }

    // delay allocation of sample buffers until they are really needed.

    return HXR_OK ;
}

HX_RESULT HXAudioSvcMixEngine::Init(INT32 sampleRateIn, INT32 sampleRateOut, INT32 nChannelsIn, INT32 nChannelsOut)
{
    HX_RESULT res = HXR_OK;

    // if we have any old resources, release them
    releaseResources() ;

    m_ulSampleRate_1_2 = sampleRateIn ;
    m_ulSampleRate_3_4 = sampleRateOut ;
    m_nChannels_1 = nChannelsIn ;
    m_nChannels_4 = nChannelsOut ;

    res = SetupUpDownmix() ;
    if (FAILED(res))
        return res ;

    res = SetupResamplerAndBuffers() ;
    if (FAILED(res))
        return res ;

#ifdef HELIX_FEATURE_GAINTOOL
    #ifdef HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION
	// create headroom for audio level normalization for mono or stereo input
	if ((m_nChannels_1 == 1 || m_nChannels_1 == 2) && m_nChannels_4 == 2)
	    m_pGaintool = gainInit(m_ulSampleRate_1_2, m_nChannels_2_3, ALN_HEADROOM) ;
	else
    #endif
	m_pGaintool = gainInit(m_ulSampleRate_1_2, m_nChannels_2_3, 0) ;

    gainSetTimeConstant(100, m_pGaintool) ;
    gainSetImmediate(0.0, m_pGaintool) ;
#endif

#ifdef HELIX_FEATURE_CROSSFADE
    m_pXFader = XFader_init(m_ulSampleRate_1_2, m_nChannels_2_3, XFader_sin2tab) ;
#endif
    ResetTimeLineInMillis(0) ;

    return HXR_OK ;
}

HX_RESULT HXAudioSvcMixEngine::ResetTimeLineInMillis(INT64 millis)
{
    m_nOutputSamplesLeft_3 = 0 ;
    m_ulResamplerPhase = 0;

    // set the cross fade state to fade in, and the time so that we are post the fade in.
    m_llFadeStart = INT_MIN ; // or something really small
    m_eCrossFadeDirection = FADE_OUT ;
    m_bPastXFade = FALSE ;

    // sample frames, output side
    m_llTimestamp_1 = m_llTimestamp_3 = millis * m_ulSampleRate_3_4 / 1000 ; // llBufTimeInSamples / m_nChannels_4 ;
    // correct for resampler delay
#ifdef HELIX_FEATURE_RESAMPLER
    if (m_pResampler) m_llTimestamp_1 -= m_pResampler->GetDelay() ;
#endif
    // convert to input side, samples
    m_llTimestamp_1 = m_llTimestamp_1 * m_ulSampleRate_1_2 / m_ulSampleRate_3_4 * m_nChannels_1 ;
    // convert to samples
    m_llTimestamp_3 *= m_nChannels_2_3 ;

    return HXR_OK ;
}

void HXAudioSvcMixEngine::GetMixRange(UINT32 nBytesToMix, INT64& llStart, INT64& llEnd) const
{
    llStart = m_llTimestamp_1 ;
    // number of samples at resampler output
    INT32 n = nBytesToMix / (m_ulBytesPerSample * m_nChannels_4) * m_nChannels_2_3 ;
#ifdef HELIX_FEATURE_RESAMPLER
    // We need to ask for number of samples at input that produces one frame 
    // more than we need at output.  The reason is that since the ressampler 
    // is fed in multiple batches, it can draw more samples than needed and 
    // and produce left over samples to use in next itteration.
    // This occurs when resampling is done accross sampling rates that are not
    // integer miltiples of each other.
    if (m_pResampler) n = m_pResampler->GetMinInput(n - m_nOutputSamplesLeft_3 + 
						    m_ulResamplerMaxOutputPerInputFrame) ;
#endif
    // number of samples at input
    n = n / m_nChannels_2_3 * m_nChannels_1 ;
    llEnd   = llStart + n ;
}

INT64 HXAudioSvcMixEngine::GetNextMixTimeMillis(void) const
{
    return INT64(1000) * m_llTimestamp_1 / (m_ulSampleRate_1_2 * m_nChannels_1) ;
}

HX_RESULT HXAudioSvcMixEngine::SetOutputBytesPerSample(UINT32 bps)
{
    switch (bps)
    {
    case 2: case 4:
        m_ulBytesPerSample = bps ; return HXR_OK ;
    default:
        return HXR_FAIL ;
    }
}

#ifdef HELIX_FEATURE_GAINTOOL
// set the volume. This is in tenth of a dB. 0 == unity gain, 6dB = twice as loud, -6 = half as loud
HX_RESULT HXAudioSvcMixEngine::SetVolume(INT32 tenthOfDB, HXBOOL bImmediate)
{
#ifndef HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION
    // currently, no amplification is allowed
    if (tenthOfDB > 0)
	return HXR_FAIL ;
#endif

    if (bImmediate)
	gainSetImmediate(0.1f * tenthOfDB, m_pGaintool) ;
    else
	gainSetSmooth(0.1f * tenthOfDB, m_pGaintool) ;

    return HXR_OK ;
}

INT32 HXAudioSvcMixEngine::HXVolume2TenthOfDB(INT32 vol)
{
    // if HX_MAX_VOLUME changes from 100, need to re-generate the table below.
    // here is the formula:
    //  if (vol > 0) return (INT32)(100.0 * log10((float)vol / HX_MAX_VOLUME )) ;
    //  else return -2000 ;

#define HX_MAX_VOLUME 100
    static const unsigned char vol2TenthOfDb[HX_MAX_VOLUME+1] = {
        255,
        200, 170, 152, 140, 130, 122, 115, 110, 105, 100, 96, 92,
        89, 85, 82, 80, 77, 74, 72, 70, 68, 66, 64, 62,
        60, 59, 57, 55, 54, 52, 51, 49, 48, 47, 46, 44,
        43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
        31, 30, 29, 28, 28, 27, 26, 25, 24, 24, 23, 22,
        21, 21, 20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
        14, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8,
        7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2,
        1, 1, 0, 0
    } ;

    if (vol > HX_MAX_VOLUME)
      return 0 ;
    else if (vol <= 0)
      return VOLUME_SILENT ;
    else
      return -(INT32)vol2TenthOfDb[vol] ;
}
#endif
HX_RESULT HXAudioSvcMixEngine::MixIntoBuffer(void* pPlayerbuf0, UINT32 ulBufSizeInBytes_4, 
                        HXBOOL &bIsMixBufferDirty, HXBOOL bOpaqueStream)
{
   HXBOOL bOptimizedMixing = FALSE;
   return MixIntoBuffer(pPlayerbuf0, ulBufSizeInBytes_4, bIsMixBufferDirty, bOptimizedMixing, bOpaqueStream);
}

HX_RESULT HXAudioSvcMixEngine::MixIntoBuffer(void* pPlayerbuf0, UINT32 ulBufSizeInBytes_4, 
                        HXBOOL &bIsMixBufferDirty, HXBOOL& bOptimizedMixing, HXBOOL bOpaqueStream)
{
    if (bOptimizedMixing)
    {
       bOptimizedMixing = FALSE;
       //we'll enable optimized mixing (caller can directly return the data in the range)
       //when we see fit
       if (m_pResampler == NULL && m_ulSampleRate_1_2 == m_ulSampleRate_3_4 && m_nChannels_1 == m_nChannels_4)
       {
          //XXFXD: TODO -- should dynamically detect conditions
#if !defined(HELIX_FEATURE_CROSSFADE) && !defined(HELIX_FEATURE_GAINTOOL) && !defined(HELIX_FEATURE_LIMITER)
           bOptimizedMixing = TRUE;
#endif
       }
    }

    // our caller's sense of "dirty" is inverted
    bIsMixBufferDirty = !bIsMixBufferDirty ;

    char *pPlayerbuf = (char*)pPlayerbuf0 ; // keep the original around
    HXBOOL bHadInput = FALSE ;

    // optimization lazy-init of buffers.
    // We only allocate the sample buffers when we really need them (the first
    // time MixIntoBuffer() is called)

    if (!m_pBuffer_1 && !bOptimizedMixing)
    {
        // allocate both buffers
        m_pBuffer_1 = new tAudioSample[m_ulChunkSize_1] ;
        if (!m_pBuffer_1)
            return HXR_OUTOFMEMORY ;

        if (m_pResampler)
        {
            m_pBuffer_3 = new tAudioSample[m_ulBufferSize_3] ;
            if (!m_pBuffer_3)
                return HXR_OUTOFMEMORY ;
        }
    }

    UINT32 nSamplesOutput_4 = ulBufSizeInBytes_4 / m_ulBytesPerSample ;

    // make sure we are being handed right-sized buffers.
    if (nSamplesOutput_4 / m_nChannels_4 * (m_ulBytesPerSample * m_nChannels_4) != ulBufSizeInBytes_4)
    {
        HX_ASSERT(0) ;
        return HXR_FAIL ;
    }

    // tile output into chunks
    while (nSamplesOutput_4)
    {
        // Figure out how many samples we need on the resampler output.
        // If we have left-overs from last time, adjust the tile size
        UINT32 ulSamples_3 = MIN(nSamplesOutput_4 / m_nChannels_4 * m_nChannels_2_3, m_ulChunkSize_3) ;

        // how many samples in do we need on input?
        UINT32 nSamples_2 ;
#ifdef HELIX_FEATURE_RESAMPLER
        if (m_pResampler)
            nSamples_2 = m_pResampler->GetMinInput(ulSamples_3 - m_nOutputSamplesLeft_3) ;
        else
#endif
        nSamples_2 = (ulSamples_3 - m_nOutputSamplesLeft_3) ;
        UINT32 nSamples_1 = nSamples_2 * m_nChannels_1 / m_nChannels_2_3 ;

        // make sure that we don't overflow the input buffer (if we did, that would
        // be a design error)
        HX_ASSERT(nSamples_1 <= m_ulChunkSize_1) ;

        //
        // Phase 1: Get the input
        //

        // get input
        HXBOOL bHaveInput = TRUE;
        if (!bOptimizedMixing)
        {
            bHaveInput = m_pCvt->ConvertIntoBuffer(m_pBuffer_1, nSamples_1, m_llTimestamp_1);
        }

        // update the time stamp.
        m_llTimestamp_1 += nSamples_1 ;

        //
        // Phase 2: Downmix if necessary. This might need headroom and create overgain
        // (not implemented yet)
        //

        // downmix if necessary (creates nSamples_2 samples)
        if (bHaveInput && m_nChannels_2_3 != m_nChannels_1 && !bOpaqueStream)
            (*this.*m_pfDownmix)(m_pBuffer_1, nSamples_1) ;

        //
        // apply any volume changes
        //

#ifdef HELIX_FEATURE_GAINTOOL
        if (bHaveInput && !bOpaqueStream)
            gainFeed(m_pBuffer_1, nSamples_2, m_pGaintool) ;
#endif

        //
        // Phase 3: Resample
        //

        // resample, but only if we have data. This is a hack -- it ignores
        // the buffers in the resamplers, and thus looses some data, and time-
        // shifts other data. This needs to be worked out.

        tAudioSample *pResampOutput_3 ;
        if (m_pResampler && bHaveInput && !bOpaqueStream)
        {
#ifdef HELIX_FEATURE_RESAMPLER
// the resampler function prototype is different for 16-bit and 32-bit resampler.
#ifndef HELIX_FEATURE_16BIT_MIXENGINE
                m_nOutputSamplesLeft_3 += m_pResampler->Resample(m_pBuffer_1, nSamples_2, (signed int*)(m_pBuffer_3 + m_nOutputSamplesLeft_3) ) ;
#else
                m_nOutputSamplesLeft_3 += m_pResampler->Resample(m_pBuffer_1, nSamples_2, (signed short*)(m_pBuffer_3 + m_nOutputSamplesLeft_3) ) ;
#endif

            // assert that the resampler did not write out-of-bounds
            HX_ASSERT(m_nOutputSamplesLeft_3 <= m_ulBufferSize_3) ;

            // assert that we got at least ulSamples_3 samples
            HX_ASSERT(m_nOutputSamplesLeft_3 >= ulSamples_3) ;

            pResampOutput_3 = m_pBuffer_3 ;
#endif
        }
        else // estimate the resampler output.
        {
            m_ulResamplerPhase += (nSamples_2 / m_nChannels_2_3) * m_ulSampleRate_3_4 ;
            int sampleFramesOut = m_ulResamplerPhase / m_ulSampleRate_1_2 ;
            m_ulResamplerPhase -= sampleFramesOut * m_ulSampleRate_1_2 ;

            m_nOutputSamplesLeft_3 += sampleFramesOut * m_nChannels_2_3 ;

            //make sure there are enough 'fake' resampled samples
            m_nOutputSamplesLeft_3 = MAX(m_nOutputSamplesLeft_3, ulSamples_3);

            pResampOutput_3 = m_pResampler ? m_pBuffer_3 : m_pBuffer_1 ; // pass-through
        }

        // m_nOutputSamplesLeft_3 is the total number of resampled samples (including leftovers
        // from the last time around). Do all further DSP only on ulSamples_3 samples, leaving
        // any leftovers for the next time.

#ifdef HELIX_FEATURE_CROSSFADE
        // We apply the crossfade even if we won't use the data, in order to
        // kick its timestamp keeping forward.
        // The performance impact should be negligible, though, since we won't be
        // in crossfades most of the time.

        //
        // if we are at the start of a fade, notify the xfader
        //

        //
        // m_llTimestamp                         ts+nsamples
        // +-------------------------------------+ incoming
        //                  XFade
        //                   |
        // llSamplesBeforeFade llSamplesInFade

        //
        //       m_llTimestamp                         ts+nsamples
        //       +-------------------------------------+ incoming
        // XFade
        //   |------V----------------------------------|
        //       llSamplesInFade
        //  llSamplesBeforeFade  < 0

	INT64 llSamplesBeforeFade = m_llFadeStart - m_llTimestamp_3 ;
	INT64 llSamplesInFade     = (INT64)ulSamples_3 - llSamplesBeforeFade ;

	if (llSamplesBeforeFade >= 0 // fade starts after this segment start
	    && llSamplesInFade > 0 // fade starts before this segment end
	    && !bOpaqueStream)
	{
	    // time to start an XFade
	    m_bPastXFade = TRUE ;
	    XFader_start(m_ulXFadeSamples, m_pXFader) ;
	}

	// if we have passed the X-Fade point, we always run the signal through
	// the XFader. Since it has a fast path when the XFade is done, this is
	// not a resource drain.

	if (m_bPastXFade && !bOpaqueStream)
	{
	    if (llSamplesBeforeFade < 0) // fade was started earlier
	    {
		llSamplesInFade += llSamplesBeforeFade ; // == ulSamples_3
		llSamplesBeforeFade = 0 ;
	    }
	    HX_ASSERT( llSamplesInFade > 0 );

	    if (XFader_active(m_pXFader))
		Fader_feed(pResampOutput_3 + INT64_TO_INT32(llSamplesBeforeFade), INT64_TO_INT32(llSamplesInFade), m_eCrossFadeDirection == FADE_OUT, m_pXFader) ;
	}
#endif

        //
        // Phase 3.5: Run the limiter if needed
        //

#ifdef HELIX_FEATURE_LIMITER
        if (bHaveInput && m_pLimiter && NBITS_PER_AUDIOSAMPLE == 32 && !bOpaqueStream)
        {
            LimiterProcess((int*)pResampOutput_3, ulSamples_3, m_pLimiter);
        }
        else
#endif
        {
            // TODO: insert clipping code
        }

        //
        // Phase 4: Mix into the output buffer.
        //

        UINT32 nSamples_4 = ulSamples_3 / m_nChannels_2_3 * m_nChannels_4 ;

        if (bHaveInput)
        {
            if (!bHadInput && bIsMixBufferDirty)
            {
                // if we did not have input earlier, but we now receive data, we need to clean out
                // the parts that have not been touched so far.
                if (!bOptimizedMixing)
                {
                   memset(pPlayerbuf0,0,pPlayerbuf - (char*)pPlayerbuf0) ;
                }
            }

            // and mix into output (mix) buffer
            if (!bOptimizedMixing)
            {
                switch (m_ulBytesPerSample)
                {
                case 2:
                    upmix(pResampOutput_3, (INT16*)pPlayerbuf, m_upmixMachine, ulSamples_3, bOpaqueStream ? TRUE : bIsMixBufferDirty) ;
                    break ;
                case 4:
                    upmix(pResampOutput_3, (INT32*)pPlayerbuf, m_upmixMachine, ulSamples_3, bOpaqueStream ? TRUE : bIsMixBufferDirty) ;
                    break ;
                }
            }
            
            // if we have input anywhere, the buffer is not "dirty" anymore.
            bHadInput = TRUE ;
        }
        else
        {
            if (bHadInput && bIsMixBufferDirty)
            {
                // if we did have input earlier, but do not now, we need to clean the output
                // buffer (because it will not be marked "dirty" anymore).
                if (!bOptimizedMixing)
                {
                    memset(pPlayerbuf, 0, nSamples_4 * m_ulBytesPerSample) ;
                }
            }
        }

        // save left-over samples
        m_nOutputSamplesLeft_3 -= ulSamples_3 ;
        m_llTimestamp_3        += ulSamples_3 ;

        // if there is no resampler, there should be no left-over samples
        if (!m_pResampler) HX_ASSERT(m_nOutputSamplesLeft_3 == 0) ;
        

        // if left-over samples
        if (m_nOutputSamplesLeft_3)
        {
            // there should be no left-over in optimized mixing
            HX_ASSERT(!bOptimizedMixing);
            memcpy(m_pBuffer_3, m_pBuffer_3 + ulSamples_3, m_nOutputSamplesLeft_3 * sizeof(*m_pBuffer_3)) ;
        }

        nSamplesOutput_4 -= nSamples_4 ;
        pPlayerbuf += nSamples_4 * m_ulBytesPerSample ;
    }

    // if we had input anywhere within this function, the buffer is not dirty anymore.

    bIsMixBufferDirty &= !bHadInput ;

    bIsMixBufferDirty = !bIsMixBufferDirty ;

    return HXR_OK ;
}

HX_RESULT HXAudioSvcMixEngine::SetCrossFade(
    enum eCrossfadeDirection inOut, // FADE_IN and FADE_OUT
    INT64 llStarttimeInSamples, // output side!
    INT64 llEndtimeInSamples
)
{
#if defined(HELIX_FEATURE_CROSSFADE)
    m_eCrossFadeDirection = inOut ;

    HX_ASSERT(llStarttimeInSamples % m_nChannels_4 == 0 &&
              llEndtimeInSamples   % m_nChannels_4 == 0 ) ;

    m_llFadeStart = llStarttimeInSamples ; // both are pre-resampler

    if (llEndtimeInSamples - llStarttimeInSamples > INT_MAX ||
        llEndtimeInSamples - llStarttimeInSamples < 0)
    {
        // we don't support such long fades
        return HXR_FAIL ;
    }
    // duration is in part 3 samples
    m_ulXFadeSamples = INT64_TO_INT32(llEndtimeInSamples - llStarttimeInSamples) / m_nChannels_4 * m_nChannels_2_3 ;

    m_bPastXFade = FALSE ;

    return HXR_OK ;
#else
    return HXR_NOTIMPL ;
#endif
}
