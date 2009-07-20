/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: RAResampler.cpp,v 1.19 2007/07/19 19:05:29 ping Exp $
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

#include "hlxclib/string.h" // memcpy

#include "RAResampler.h"
#include "allresamplers.h"
#include "cpuident.h"


static void cvtShortShortSimple(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    memcpy(d,s,n*sizeof(short)) ; /* Flawfinder: ignore */
}

static void cvtFloatFloatSimple(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    memcpy(d,s,n*sizeof(float)) ; /* Flawfinder: ignore */
}

static void cvtIntFloatSimple(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    int i ;
    const int* src = (const int*)s ;
    float* dst = (float*)d ;
    for (i = 0 ; i < n ; i++)
        *dst++ = (1.0f/65536.0f)*(float)*src++ ;
}

static void cvtIntShortSimple(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    int i ;
    const int* src = (const int*)s ;
    short* dst = (short*)d ;
    for (i = 0 ; i < n ; i++)
    {
        int t = *src++ ;
        if (t < 0x7fff8000L) t += 0x8000L ; // rounding
        *dst++ = (short)(t >> 16);
    }
}

static void cvtShortFloatSimple(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    int i ;
    const short* src = (const short*)s ;
    float* dst = (float*)d ;
    for (i = 0 ; i < n ; i++)
        *dst++ = (float)*src++ ;
}

// generic convert functions for multichannel operation

static int cvtShortShort(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    const short* src = (const short *)s ;
    const short* srcEnd = src + n ;
    short* dst = (short *)d ;

    while (src != srcEnd)
    {
        *dst = *src ;
        src += pState->incInput ;
        dst += pState->incOutput ;
        pState = pState->pNext ;
    }

    return dst - (short *)d ;
}

static int cvtFloatFloat(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    const float* src = (const float *)s ;
    const float* srcEnd = src + n ;
    float* dst = (float *)d ;

    while (src != srcEnd)
    {
        *dst = *src ;
        src += pState->incInput ;
        dst += pState->incOutput ;
        pState = pState->pNext ;
    }

    return dst - (float *)d ;
}

static int cvtIntFloat(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    const int* src = (const int *)s ;
    const int* srcEnd = src + n ;
    float* dst = (float *)d ;

    while (src != srcEnd)
    {
        *dst = (1.0f/65536.0f) * (*src) ;
        src += pState->incInput ;
        dst += pState->incOutput ;
        pState = pState->pNext ;
    }
    return dst - (float *)d ;
}

static int cvtIntShort(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    const int* src = (const int *)s ;
    const int* srcEnd = src + n ;
    short* dst = (short *)d ;

    while (src != srcEnd)
    {
        int t = (int)(*src) ;
        if (t < 0x7fff8000L) t += 0x8000L ; // rounding
        *dst = (short)(t >> 16) ;
        src += pState->incInput ;
        dst += pState->incOutput ;
        pState = pState->pNext ;
    }
    return dst - (short *)d ;
}

static int cvtShortFloat(void *d, const void *s, int n, const struct CVTSTATEMACHINE *pState)
{
    const short* src = (const short *)s ;
    const short* srcEnd = src + n ;
    float* dst = (float *)d ;

    while (src != srcEnd)
    {
        *dst = (float)(*src) ;
        src += pState->incInput ;
        dst += pState->incOutput ;
        pState = pState->pNext ;
    }

    return dst - (float *)d ;
}


typedef int  (*tfResample)(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst);
typedef void* (*tfInitResamplerCopy)(int nchans, const void *inst);
typedef void (*tfFreeResampler)(void *inst);
typedef int  (*tfGetDelay)(void *inst);
typedef int  (*tfGetDelay)(void *inst);
typedef int  (*tfGetMinInput)(int nSamples, void *inst);
typedef int  (*tfGetMaxOutput)(int nSamples, void *inst);

static cvtFunctionType const cvtFunc[3][3] = {
    {cvtShortShort,0,cvtShortFloat},
    {cvtIntShort,  0,cvtIntFloat},
    {0,            0,cvtFloatFloat}
} ;

RAExactResampler::RAExactResampler() {}
RAExactResampler::~RAExactResampler() {}

class RAAnyResampler : public RAExactResampler
{
public:

    int Resample(void *_inbuf, int insamps, signed short *outbuf) ;
    int Resample(void *_inbuf, int insamps, signed int *outbuf) ;

    int GetMinInput(int insamps)
    {
        int t = insamps / m_nChannelsTotal * m_nChannels[0] ;
        t = m_pfGetMinInput(t, m_pResampler[0]) ;
        return t / m_nChannels[0] * m_nChannelsTotal ;
    }

    int GetMaxOutput(int insamps)
    {
        int t = insamps / m_nChannelsTotal * m_nChannels[0] ;
        t = m_pfGetMaxOutput(t, m_pResampler[0]) ;
        return t / m_nChannels[0] * m_nChannelsTotal ;
    }

    int GetDelay()
    {
        return m_pfGetDelay(m_pResampler[0]) ;
    }

protected:
    RAAnyResampler() ;
    virtual ~RAAnyResampler();

    /*
     * function pointers
     */
    tfResample      m_pfResample[2] ; // one for mono, one for stereo
    tfGetMinInput   m_pfGetMinInput ;
    tfGetMaxOutput  m_pfGetMaxOutput ;
    tfGetDelay      m_pfGetDelay ;
    tfFreeResampler m_pfFreeResampler ;
    tfInitResamplerCopy m_pfInitResamplerCopy ;
    tConverter m_converter[2] ; // one for mono, one for stereo

    int intype, resType;

    enum {
        MAX_RESAMPLERS = 5
    } ;

    int m_nResamplers ;
    int m_nBytesPerSampleIn ;
    int m_nChannelsTotal ;
    void *m_pResampler[MAX_RESAMPLERS] ;
    int m_nChannels[MAX_RESAMPLERS] ;
    int m_nBlock[MAX_RESAMPLERS] ;
    int m_nOffsetIn[MAX_RESAMPLERS] ;

    HX_RESULT Init(int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain) ;

    // these need to be supplied by the base classes
    virtual int GetResamplerInputType(void) = 0 ;
    virtual HX_RESULT SetupFunctionPointers(void) = 0 ;
    virtual void* InitResampler(int inrate, int outrate, int nchans, float atten, float passband, float stopband, float dcgain) = 0;

private:
    HX_RESULT SetupNumChannels(int chans) ;
    HX_RESULT SetupConverter(int intype, int restype) ;
} ;

RAAnyResampler::RAAnyResampler()
: m_nResamplers(0)
{
    memset(m_pResampler, 0, sizeof(m_pResampler)) ;
}

HX_RESULT RAAnyResampler::Init(int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain)
{
    HX_RESULT res = HXR_OK ;
    int i ;

    if (SUCCEEDED(res))
        res = SetupFunctionPointers() ;

    if (SUCCEEDED(res))
        res = SetupNumChannels(chans) ;

    m_converter[0].pStateMachine = 0 ;
    m_converter[1].pStateMachine = 0 ;
    if (SUCCEEDED(res))
        res = SetupConverter(intype, GetResamplerInputType()) ;

    for (i = 0 ; i < m_nResamplers; i++)
    {
        m_pResampler[i] = 0 ;
        if (SUCCEEDED(res))
        {
            if (i == 0)
                m_pResampler[i] = InitResampler(inrate, outrate, m_nChannels[i],  atten, passband, stopband, dcgain) ;
            else
                m_pResampler[i] = m_pfInitResamplerCopy(m_nChannels[i], m_pResampler[0]) ;
        }

        if (!m_pResampler[i])
            res = HXR_OUTOFMEMORY ;
    }

    return res ;
}

RAAnyResampler::~RAAnyResampler()
{
    int i ;
    for (i = 0 ; i < MAX_RESAMPLERS ; i++)
    {
        if (m_pResampler[i]) m_pfFreeResampler(m_pResampler[i]) ;
        m_pResampler[i] = 0; // paranoia code.
    }
    HX_VECTOR_DELETE(m_converter[0].pStateMachine) ;
    HX_VECTOR_DELETE(m_converter[1].pStateMachine) ;
}

HX_RESULT RAAnyResampler::SetupNumChannels(int chans)
{
    int i ;

    if (chans > 2*MAX_RESAMPLERS)
        return HXR_FAIL ;

    m_nChannelsTotal = chans ;
    m_nResamplers = 0 ;
    for (i = 0 ; i < chans ; i+=2)
    {
        m_nChannels[m_nResamplers] = (i + 2 > chans) ? 1 : 2 ;
        m_nBlock[m_nResamplers] = NBLOCK - NBLOCK % m_nChannels[m_nResamplers] ;
        m_nOffsetIn[m_nResamplers] = 2 * m_nResamplers ;
        m_nResamplers++ ;
    }

    return HXR_OK ;
}

HX_RESULT RAAnyResampler::SetupConverter(int _intype, int _restype)
{
    static const int bps[] = {2,4,4} ;

    intype = _intype;
    resType = _restype ;
    m_converter[0].pfCvt         = cvtFunc[intype][resType] ;
    m_converter[1].pfCvt         = cvtFunc[intype][resType] ;
    m_converter[0].pStateMachine = new struct CVTSTATEMACHINE[1] ;
    m_converter[1].pStateMachine = new struct CVTSTATEMACHINE[2] ;

    // mono converter
    m_converter[0].pStateMachine[0].pNext = m_converter[0].pStateMachine ;
    m_converter[0].pStateMachine[0].incInput = m_nChannelsTotal ;
    m_converter[0].pStateMachine[0].incOutput = 1 ;

    // stereo converter
    m_converter[1].pStateMachine[0].pNext = &(m_converter[1].pStateMachine[1]) ;
    m_converter[1].pStateMachine[0].incInput = 1 ;
    m_converter[1].pStateMachine[0].incOutput = 1 ;
    m_converter[1].pStateMachine[1].pNext = &(m_converter[1].pStateMachine[0]) ;
    m_converter[1].pStateMachine[1].incInput = m_nChannelsTotal - 1 ;
    m_converter[1].pStateMachine[1].incOutput = 1 ;

    m_nBytesPerSampleIn = bps[intype] ;

    return HXR_OK ;
}

int RAAnyResampler::Resample(void *_inbuf, int _insamps, signed short *_outbuf)
{
    int i ;
    int outsamps ;

    for (i = 0 ; i < m_nResamplers; i++)
    {
        int insamps = _insamps ;
        int nChannels = m_nChannels[i] ;
        char* inbuf = (char*)_inbuf + m_nOffsetIn[i] * m_nBytesPerSampleIn ;
        signed short* outbuf = _outbuf + m_nOffsetIn[i] ;
        outsamps = 0 ;
        while (insamps)
        {
            int nin = HX_MIN(insamps, m_nBlock[i]);
            int nout = m_pfResample[nChannels-1](inbuf, nin, &m_converter[nChannels-1], outbuf, m_nChannelsTotal, m_pResampler[i]);
            inbuf    += nin * m_nBytesPerSampleIn ;
            insamps  -= nin;
            outbuf   += nout;
            outsamps += nout;
        }
    }

    return outsamps;
}

int RAAnyResampler::Resample(void *_inbuf, int _insamps, signed int *_outbuf)
{
    // fake it!
    int outsamps = Resample(_inbuf, _insamps, (signed short*) _outbuf) ;

    for (int j=outsamps-1; j>=0; --j)
    {
      _outbuf[j] = ((signed short*)_outbuf)[j] << 16 ;
    }

    return outsamps;
}

/*
 * Arbitrary resampler -- any rate to any rate
 */

class RAArbitraryResampler : public RAAnyResampler
{
public:
    static HX_RESULT Create(RAExactResampler **pRes, int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain)
    {
        HX_RESULT res = HXR_OK ;

        RAArbitraryResampler *pr = new RAArbitraryResampler() ;
        if (!pr)
            res = HXR_OUTOFMEMORY ;

        if (SUCCEEDED(res))
            res = pr->Init(inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;

        if (FAILED(res))
        {
            delete pr ; pr = 0 ;
        }

        *pRes = pr ;
        return res ;
    }

    HX_RESULT SetupFunctionPointers(void)
    {
        m_pfResample[0] = &RAResampleMonoArb ; m_pfResample[1] = &RAResampleStereoArb ;
        m_pfGetMaxOutput = &RAGetMaxOutputArb ; m_pfGetMinInput = &RAGetMinInputArb ;
        m_pfGetDelay = &RAGetDelayArb ; m_pfFreeResampler = &RAFreeResamplerArb ;
        m_pfInitResamplerCopy = &RAInitResamplerCopyArb;

        return HXR_OK ;
    }

    int GetResamplerInputType(void) {return _FLOAT; }

    void* InitResampler(int inrate, int outrate, int nchans, float atten, float passband, float stopband, float dcgain)
    {
        return RAInitResamplerArb(inrate, outrate, nchans, atten, passband, stopband, dcgain) ;
    }
} ;

/*
 * Rational resampler -- many rates to many rates, as long as the fraction
 * inrate/outrate is reducible to "small numbers"
 */

class RARationalResampler : public RAAnyResampler
{
public:
    static HX_RESULT Create(RAExactResampler **pRes, int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain)
    {
        HX_RESULT res = HXR_OK ;

        RARationalResampler *pr = new RARationalResampler() ;
        if (!pr)
            res = HXR_OUTOFMEMORY ;

        if (SUCCEEDED(res))
            res = pr->Init(inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;

        if (FAILED(res))
        {
            delete pr ; pr = 0 ;
        }

        *pRes = pr ;
        return res ;
    }

    int GetResamplerInputType(void) {return _FLOAT; }

    HX_RESULT SetupFunctionPointers(void)
    {
        m_pfResample[0] = &RAResampleMonoRat ; m_pfResample[1] = &RAResampleStereoRat ;
        m_pfGetMaxOutput = &RAGetMaxOutputRat ; m_pfGetMinInput = &RAGetMinInputRat ;
        m_pfGetDelay = &RAGetDelayRat ; m_pfFreeResampler = &RAFreeResamplerRat ;
        m_pfInitResamplerCopy = &RAInitResamplerCopyRat;
        return HXR_OK ;
    }

    void* InitResampler(int inrate, int outrate, int nchans, float atten, float passband, float stopband, float dcgain)
    {
        return RAInitResamplerRat(inrate, outrate, nchans, atten, passband, stopband, dcgain) ;
    }
} ;

#if defined(_M_IX86) /* || defined(__i386__) */ /* no unix mmx code yet */

class RAMMXResampler : public RAAnyResampler
{
public:
    static HX_RESULT Create(RAExactResampler **pRes, int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain)
    {
        HX_RESULT res = HXR_OK ;

        RAMMXResampler *pr = new RAMMXResampler() ;
        if (!pr)
            res = HXR_OUTOFMEMORY ;

        if (SUCCEEDED(res))
            res = pr->Init(inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;

        if (FAILED(res))
        {
            delete pr ; pr = 0 ;
        }

        *pRes = pr ;
        return res ;
    }

    int GetResamplerInputType(void) {return _INT16; }

    HX_RESULT SetupFunctionPointers(void)
    {
        m_pfResample[0]  = &RAResampleMonoMMX ; m_pfResample[1] = &RAResampleStereoMMX ;
        m_pfGetMaxOutput = &RAGetMaxOutputMMX ; m_pfGetMinInput = &RAGetMinInputMMX ;
        m_pfGetDelay     = &RAGetDelayMMX ;   m_pfFreeResampler = &RAFreeResamplerMMX ;
        m_pfInitResamplerCopy = &RAInitResamplerCopyMMX;
        return HXR_OK ;
    }

    void* InitResampler(int inrate, int outrate, int nchans, float atten, float passband, float stopband, float dcgain)
    {
        // should maybe check for atten etc.
        return RAInitResamplerMMX(inrate, outrate, nchans) ;
    }
} ;

#endif /* defined(_M_IX86) */

#if defined(HELIX_CONFIG_FIXEDPOINT)
class RAHermiteResampler : public RAAnyResampler
{
public:
    static HX_RESULT Create(RAExactResampler **pRes, int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain)
    {
        HX_RESULT res = HXR_OK ;

        /* intype must be _INT16 for the Hermite resampler */
        if (intype != _INT16 || chans > 2)
            return HXR_FAIL ;

        RAHermiteResampler *pr = new RAHermiteResampler() ;
        if (!pr)
            res = HXR_OUTOFMEMORY ;

        if (SUCCEEDED(res))
            res = pr->Init(inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;

        if (FAILED(res))
        {
            delete pr ; pr = 0 ;
        }

        *pRes = pr ;
        return res ;
    }

    int GetResamplerInputType(void) {return _INT16; }

    HX_RESULT SetupFunctionPointers(void)
    {
        m_pfResample[0]  = &RAResampleMonoHermite ; m_pfResample[1] = &RAResampleStereoHermite ;
        m_pfGetMaxOutput = &RAGetMaxOutputHermite ; m_pfGetMinInput = &RAGetMinInputHermite ;
        m_pfGetDelay     = &RAGetDelayHermite ;   m_pfFreeResampler = &RAFreeResamplerHermite ;
        m_pfInitResamplerCopy = &RAInitResamplerCopyHermite;
        return HXR_OK ;
    }

    void* InitResampler(int inrate, int outrate, int nchans, float atten, float passband, float stopband, float dcgain)
    {
        // should maybe check for atten etc.
        return RAInitResamplerHermite(inrate, outrate, nchans) ;
    }
};
#endif /* defined(HELIX_CONFIG_FIXEDPOINT) */

HX_RESULT RAExactResampler::Create(RAExactResampler** pRes, int inrate, int outrate, int chans, int intype, int quality)
{
    float dcgain   = DEF_DCGAIN ;
    float atten    ;
    float passband ;
    float stopband ;

    switch(quality)
    {
    case qualityVeryLow:
        passband = 0.77f ; // 32 tap
        stopband = 1.09f ;
        atten    = 80.0f ;
        break ;
    case qualityLow:
        passband = 0.82f ; // 48 tap
        stopband = 1.05f ;
        atten    = 85.0f ;
        break ;
    case qualityMedium:
        passband = 0.85f ; // 64 tap
        stopband = 1.03f ;
        atten    = 90.0f ;
        break ;
    case qualityHigh:
        passband = 0.88f ; // 96 tap
        stopband = 1.00f ;
        atten    = 90.0f ;
        break ;
    case qualityHyper:
        passband = 0.904f ; // 128 tap
        stopband = 1.000f ;
        atten    = 96.00f ;
        break ;
    default:
        return HXR_INVALID_PARAMETER ;
    }

    return Create(pRes, inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;
}

HX_RESULT RAExactResampler::Create(RAExactResampler** pRes, int inrate, int outrate, int chans, int intype, float atten, float trans, float dcgain)
{
    return Create(pRes, inrate, outrate, chans, intype, atten, 1.0f-trans, 1.0f, dcgain) ;
}

HX_RESULT RAExactResampler::Create(RAExactResampler** pRes, int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain)
{
    HX_RESULT res = HXR_FAIL ;

#if defined (HELIX_CONFIG_FIXEDPOINT)
    if (FAILED(res))
        res = RAHermiteResampler::Create(pRes, inrate, outrate, chans, intype, atten, passband, stopband, dcgain);
#else

    /* only instantiate the MMX resampler if we are compiling on an x86 architecture
       and running on a machine with MMX */
#ifndef HELIX_CONFIG_NIMBUS
#if defined(_M_IX86) /* || defined(__i386__) */ /* no unix mmx code yet */
    
    CPUInformation info ;
    CPUIdentify(&info) ;

    /* don't use the MMX resampler if the quality is "hyper" */
    if (atten <= 90.0 &&
        info.architecture == ulArchitectureIntel &&
        info.specific.m_x86.hasMMX)
    {
        res = RAMMXResampler::Create(pRes, inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;
    }
#endif
#endif /* HELIX_CONFIG_NIMBUS */

    if (FAILED(res))
    {
        res = RARationalResampler::Create(pRes, inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;
    }

    if (FAILED(res))
    {
        res = RAArbitraryResampler::Create(pRes, inrate, outrate, chans, intype, atten, passband, stopband, dcgain) ;
    }
#endif /*HELIX_CONFIG_FIXEDPOINT*/
    return res ;
}
