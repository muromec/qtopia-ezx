/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mixengine.h,v 1.16 2005/05/05 21:27:08 kross Exp $
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

#ifndef _AFMTCVT_H_
#define _AFMTCVT_H_

#include "hxtypes.h"
#include "hxresult.h"

#include "hlxclib/string.h" // memcpy/memset

// define the "native audio data type"
// on memory- and resource-constrained devices, use 16-bit processing
#ifdef HELIX_FEATURE_16BIT_MIXENGINE
typedef INT16 tAudioSample ;

#if defined(HELIX_FEATURE_GAINTOOL) || defined(HELIX_FEATURE_CROSSFADE)\
 || defined(HELIX_FEATURE_LIMITER)
#error "gain tool, crossfader and limiter do not work on 16-bit data types yet"
#endif

#else // all other platforms use 32-bit processing
typedef INT32 tAudioSample ;
#endif

#define NBITS_PER_AUDIOSAMPLE (sizeof(tAudioSample)<<3)

// derive your class from this. This will be used as a callback to convert samples from
// the renderer input queues into the HXAudioSvcMixEngine source buffer
class CAudioSvcSampleConverter
{
public:
    // As a client of the MixEngine, you need to implement this function. Its
    // purpose is to fill a mixengine-owned buffer with samples representing the
    // time between llStartTime and llStartTime + nSamples.
    // If you have data for all the buffer, convert it all and return 1.
    // If you have partial data, convert what you have, and silence out the rest. Return 1.
    // If you have no data at all, just return 0. You may silence the buffer, but don't have to.

    virtual HXBOOL ConvertIntoBuffer(tAudioSample* buffer, UINT32 nSamples, INT64 llStartTimeInSamples) = 0;

protected:
    // using these utility functions.
    static void cvt8 (const void *in, tAudioSample* out, int nSamples)
    {
        for (int i=0; i < nSamples; i++) out[i] = (((const UINT8*)in)[i] - 128) << (NBITS_PER_AUDIOSAMPLE-8) ;
    }
    static void cvt16(const void *in, tAudioSample* out, int nSamples)
    {
        for (int i=0; i < nSamples; i++) out[i] = ((const INT16*)in)[i] << (NBITS_PER_AUDIOSAMPLE-16) ;
    }
    static void cvt32(const void *in, tAudioSample* out, int nSamples)
    {
        if (NBITS_PER_AUDIOSAMPLE == 32)
            memcpy(out, in, nSamples * sizeof(*out));
        else
            for (int i=0; i < nSamples; i++) out[i] = (INT16)(((const INT32*)in)[i] >> (NBITS_PER_AUDIOSAMPLE-16)) ;
    }
    static void silence(tAudioSample* out, int nSamples)
    {
        memset(out, 0, nSamples * sizeof(*out)) ;
    }
} ;

// forward and other definitions

struct COEFF ;
class RAExactResampler ;
typedef struct GAIN_STATE GAIN_STATE ;
typedef struct XFADER_STATE XFADER_STATE ;
typedef struct LIMSTATE LIMSTATE ;

typedef struct UpMixMachine  tUpMixMachine ;

class HXAudioSvcMixEngine
{
public:
    HXAudioSvcMixEngine() ;
    ~HXAudioSvcMixEngine() ;

    // set the parameters. You can call this (and change the parameters) in operation,
    // but it will reset the time line.
    HX_RESULT Init(INT32 sampleRateIn, INT32 sampleRateOut, INT32 nChannelsIn, INT32 nChannelsOut) ;

    // set the output bytes per sample. Set it to 2 for 16-bit output, 4 for 32-bit
    // output. Note that this does not influence the datatype of internal computations.
    HX_RESULT SetOutputBytesPerSample(UINT32 bps) ;

    // Set the sample converter. The mix engine uses the sample converter to read
    // new samples.
    HX_RESULT SetSampleConverter(CAudioSvcSampleConverter *pCvt) ;

    // set the volume/gain. This is in tenth of a dB. 0 == unity gain, 6dB = twice as loud, -6dB = half as loud
    // set bImmediate if you want the gain change to be immediate
    // (if you don't know what this means, then you don't want it to be immediate)
    HX_RESULT SetVolume(INT32 tenthOfDB, HXBOOL bImmediate = FALSE) ;

    // use this to convert from "Helix Volume Scale" to tenth of dB.
    static INT32 HXVolume2TenthOfDB(INT32 vol) ;

    enum
    {
        VOLUME_SILENT = -200 * 10 // -200 dB is as good as silent.
    } ;

    // set the downmix matrix. There are default downmix matrices, so you don't need to 
    // call this function. In fact, it is not yet spec'ed out.
    // SetDownmixMatrix() ;

    // This will issue a series of Convert::ConvertIntoBuffer() callbacks,
    // and will return with a full buffer of resampled/channel converted/mixed data.
    HX_RESULT MixIntoBuffer(
        void* pPlayerBuf,
        UINT32 ulBufSizeInBytes,
        HXBOOL&    bIsMixBufferDirty,
	HXBOOL bOpaqueStream = FALSE
    ) ;

    // guess what.
    enum eCrossfadeDirection
    {
      FADE_IN  = 0,
      FADE_OUT = 1
    } ;

    // This will register a cross fade. The cross fade will be unregistered only
    // when ResetTimeline() is called or when the fade is done.
    // the time stamps are in units of samples of the output signal.
    HX_RESULT SetCrossFade(
        enum eCrossfadeDirection inOut, // 0 for fade in, 1 for fade out.
        INT64 llStarttimeInSamples,
        INT64 llEndtimeInSamples
    ) ;

    // reset the time line. Call this whenever the next call to MixIntoBuffer()
    // will have a time stamp that is non-contiguous with the previous mix.
    // This should only be the case after Seek/Resume
    HX_RESULT ResetTimeLineInMillis(INT64 millis) ;

    // the time of the next mix, in ms.
    INT64 GetNextMixTimeMillis(void) const;

    // ask the mixer which range of input samples it will request in a mix operation
    // of size nBytesToMix.
    void GetMixRange(UINT32 nBytesToMix, INT64& llStart, INT64& llEnd) const;

protected:
    // the sample converter we call back into
    CAudioSvcSampleConverter *m_pCvt ;

private:
    // we process audio in batches, so that we don't have to dynamically
    // allocate memory and can work inside of the cache. This is the number
    // of samples we process at any one time.
    enum
    {
        BATCHSIZE = 2048
    } ;

    // helper functions
    void releaseResources() ;
    HX_RESULT SetupResamplerAndBuffers(void) ;
    HX_RESULT SetupUpDownmix(void) ;

    // timekeeping & other variables
    INT64 m_llTimestamp_1, m_llTimestamp_3 ; // time on input & output

    UINT32 m_ulSampleRate_1_2, m_ulSampleRate_3_4 ;
    UINT32 m_nChannels_1, m_nChannels_2_3, m_nChannels_4 ;
    UINT32 m_ulChunkSize_1,m_ulChunkSize_3 ;
    UINT32 m_ulBytesPerSample ; // bytes per sample on the output

    UINT32 m_nOutputSamplesLeft_3 ;
    UINT32 m_ulBufferSize_3 ;
    tAudioSample *m_pBuffer_1, *m_pBuffer_3 ; // buffer pre- and post resampler

    // resampler
    RAExactResampler *m_pResampler ;
    UINT32 m_ulResamplerPhase ;
    UINT32 m_ulResamplerMaxOutputPerInputFrame ;

    // XFader
    INT64 m_llFadeStart ;
    UINT32 m_ulXFadeSamples ;
    HXBOOL m_bPastXFade ; // set to TRUE if we have passed the X-Fade point.

    enum eCrossfadeDirection m_eCrossFadeDirection ;
    XFADER_STATE *m_pXFader ;

    // gain tool
    GAIN_STATE *m_pGaintool ;

    // limiter
    LIMSTATE *m_pLimiter ;

    // downmix/upmix
    enum { HEADROOM = 3, ALN_HEADROOM = 3 } ;
    int m_slev, m_clev ;
    UINT32 upmix(const tAudioSample *pIn, INT32 *pOut, const tUpMixMachine *pUpmixMachine, int nSamples, HXBOOL isDirty) ;
    UINT32 upmix(const tAudioSample *pIn, INT16 *pOut, const tUpMixMachine *pUpmixMachine, int nSamples, HXBOOL isDirty) ;
    const tUpMixMachine *m_upmixMachine ; // how to "up"-mix the audio

    void downmix2_1(tAudioSample *pBuffer, int nSamples) ;
    void downmix5_1(tAudioSample *pBuffer, int nSamples) ;
    void downmix4_2_stereo(tAudioSample *pBuffer, int nSamples) ;
    void downmix4_2_matrix(tAudioSample *pBuffer, int nSamples) ;
    void downmix5_2_stereo(tAudioSample *pBuffer, int nSamples) ;
    void downmix5_2_matrix(tAudioSample *pBuffer, int nSamples) ;
    void downmix6_1(tAudioSample *pBuffer, int nSamples) ;
    void downmix6_2_stereo(tAudioSample *pBuffer, int nSamples) ;
    void downmix6_2_matrix(tAudioSample *pBuffer, int nSamples) ;
    void downmix6_5(tAudioSample *pBuffer, int nSamples) ;

    typedef void (HXAudioSvcMixEngine::*tDownmixfunc)(tAudioSample *pBuffer, int nSamples) ;
    tDownmixfunc m_pfDownmix ; // function to downmix the audio
} ;

#endif /* _AFMTCVT_H_ */
