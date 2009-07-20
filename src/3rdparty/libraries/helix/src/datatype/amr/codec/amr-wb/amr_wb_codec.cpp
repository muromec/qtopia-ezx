/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "./amr_wb_codec.h"
#include "amr_frame_hdr.h"

extern "C" {
#include "dec_if.h"
};

static const UINT32 MaxSamples = 320;

#ifdef IF2

static void Convert2IF2(AMRFlavor flavor, const UINT8* pSrc, UINT8* pDst)
{
    // RFC 3267 Section 5.3 Speech Frame
    //   7   6   5   4   3   2   1   0   7   6   5   4   3   2   1   0
    // +---+---------------+---+---+---+-------------------------------+----
    // | P |       FT      | Q | P | P |        d(0) - d(7)            | ...
    // +---+---------------+---+---+---+-------------------------------+----
    //
    // P  = padding
    // FT = frame type
    // Q  = quality indicator

    // 3GPP TS 26.201 Annex A IF2 format
    //   7   6   5   4   3   2   1   0   7   6   5   4   3   2   1   0
    // +---------------+---------------+-------------------------------+----
    // |       FT      | Q | d(0)-d(2) |           d(3)-d(10)          | ...
    // +---------------+---------------+-------------------------------+----
    //
    // Note : The conversion needs to shift
    int frameType = (pSrc[0] >> 3) & 0xf;
    int frameBytes = (CAMRFrameInfo::FrameBits(flavor, frameType) + 7) >> 3;

    if (frameBytes)
    {
	// Copy header info and d(0) - d(2)
	*pDst++ = ((pSrc[0] << 1) & 0xf8) | (pSrc[1] >> 5);
    
	UINT32 tmpBuf = pSrc[1] & 0x1f;
    
	pSrc += 2;
    
	// Copy d(3) - d(N - 7)
	for (int i = 0; i < (frameBytes - 1); i++)
	{
	    tmpBuf <<= 8;
	    tmpBuf |= *pSrc++;
	    
	    *pDst++ = (UINT8)((tmpBuf >> 5) & 0xff);
	}

	// Copy d(N-6) - d(N-1)
	*pDst = (UINT8)((tmpBuf << 3) & 0xff);
    }
    else
    {
	*pDst++ = ((pSrc[0] << 1) & 0xf8);
    }
}

#endif /* #ifdef IF2 */

CAMRWideBandCodec::CAMRWideBandCodec() :
    m_lRefCount(0),
    m_pState(0),
    m_nConcealSamples(0),
    m_pFrameBuf(new UINT8[1 + ((CAMRFrameInfo::MaxFrameBits(WideBand) + 7) >> 3)])
{}

CAMRWideBandCodec::~CAMRWideBandCodec()
{
    if (m_pState)
    {
	D_IF_exit(m_pState);
	m_pState = 0;
    }
    
    delete [] m_pFrameBuf;
}

/*
 *	IUnknown methods
 */
STDMETHODIMP CAMRWideBandCodec::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT res = HXR_NOINTERFACE;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	res = HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAudioDecoder))
    {
	AddRef();
	*ppvObj = (IHXAudioDecoder*)this;
	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP_(ULONG32) CAMRWideBandCodec::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CAMRWideBandCodec::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXAudioDecoder methods
 */

STDMETHODIMP CAMRWideBandCodec::OpenDecoder(UINT32 cfgType, const void* config,
				    UINT32 nBytes)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pState)
    {
	D_IF_exit(m_pState);
    }

    m_pState = D_IF_init();

    if (m_pState)
	res = HXR_OK;

    return res;
}

STDMETHODIMP CAMRWideBandCodec::Reset()
{
    HX_RESULT res = HXR_OK;

    if (m_pState)
    {
	D_IF_exit(m_pState);

	m_pState = D_IF_init();

	if (!m_pState)
	    res = HXR_FAILED;
    }

    m_nConcealSamples = 0;

    return res;
}


STDMETHODIMP CAMRWideBandCodec::GetMaxSamplesOut(UINT32& nSamples) CONSTMETHOD
{
    nSamples = MaxSamples;
    return HXR_OK;
}

STDMETHODIMP CAMRWideBandCodec::GetNChannels(UINT32& nChannels) CONSTMETHOD
{
    nChannels = 1;
    return HXR_OK;
}

STDMETHODIMP CAMRWideBandCodec::GetSampleRate(UINT32& sampleRate) CONSTMETHOD
{
    sampleRate = 16000;
    return HXR_OK;
}

STDMETHODIMP CAMRWideBandCodec::GetDelay(UINT32& nSamples) CONSTMETHOD
{
    nSamples = 0;
    return HXR_OK;
}

STDMETHODIMP CAMRWideBandCodec::Decode(const UCHAR* data, UINT32 nBytes, 
			       UINT32 &nBytesConsumed, INT16 *samplesOut, 
			       UINT32& nSamplesOut, HXBOOL eof)
{
    HX_RESULT res = HXR_UNEXPECTED;

    nSamplesOut = 0;
    nBytesConsumed = 0;

    if (m_pState)
    {
	if (m_nConcealSamples)
	{	    
	    // Currently we do not do anything special for concealment
	    // so we will just output silence for the concealment
	    // samples.
	    if (m_nConcealSamples > MaxSamples)
		nSamplesOut = MaxSamples;
	    else
		nSamplesOut = m_nConcealSamples;
	    
	    res = HXR_OK;

	    for (UINT32 i = 0; i < nSamplesOut; i++)
		*samplesOut++ = 0;

	    m_nConcealSamples -= nSamplesOut;
	}
	else if (nBytes)
	{
	    VerifyInput((UCHAR*)data, nBytes, nBytes);

	    UCHAR* pData;
	    int frameType;
#ifdef IF2
				// Convert the RFC3267 Section 5.3 speech frame
				// format into 3GPP TS 26.101 Annex A IF2 format
	    Convert2IF2(WideBand, data, m_pFrameBuf);
	    pData = m_pFrameBuf;
	    frameType = (*pData >> 4) & 0xf;
#else
				// no conversion necessary
	    pData = (UCHAR*)data;
	    frameType = (*pData >> 3) & 0xf;
#endif

	    int frameBytes = 
		1 + ((CAMRFrameInfo::FrameBits(WideBand, frameType) + 7) >> 3);

	    if ((int)nBytes >= frameBytes)
	    {
		D_IF_decode(m_pState, pData, samplesOut, _good_frame);
		
		nBytesConsumed = frameBytes;

		nSamplesOut = MaxSamples;
		res = HXR_OK;
	    }
	}
    }
    
    return res;
}

STDMETHODIMP CAMRWideBandCodec::Conceal(UINT32 nSamples)
{
    // We want to conceal only in MaxSamples size blocks.
    UINT32 nFrameCount = (nSamples + MaxSamples - 1) / MaxSamples;
    UINT32 nConcealSamples = nFrameCount * MaxSamples;

    m_nConcealSamples += nConcealSamples;

    return HXR_OK;
}
