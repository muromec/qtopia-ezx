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

#include "./amr_nb_codec.h"
#include "amr_frame_hdr.h"

extern "C" {
#include "interf_dec.h"
};

static const UINT32 MaxSamples = 160;

inline
static UINT8 FlipNibble(UINT8 ch)
{
    static const UINT8 z_flipTbl[16] = {
	0x0, 0x8, 0x4, 0xc, 
	0x2, 0xa, 0x6, 0xe,
	0x1, 0x9, 0x5, 0xd, 
	0x3, 0xb, 0x7, 0xf
    };

    return z_flipTbl[ch];
}

static void Convert2IF2(const UINT8* pSrc, UINT8* pDst)
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

    // 3GPP TS 26.101 Annex A IF2 format
    //   7   6   5   4   3   2   1   0   7   6   5   4   3   2   1   0
    // +---------------+---------------+-------------------------------+----
    // | d(3) - d(0)   |       FT      |       d(11) - d(4)            | ...
    // +---------------+---------------+-------------------------------+----
    //
    // Note : The conversion needs to change the endian-ness of the payload
    //        bits and shift the data by 4 bits
    int frameType = (*pSrc++ >> 3) & 0xf;
    int frameBytes = (CAMRFrameInfo::FrameBits(NarrowBand,frameType) + 7) >> 3;

    UINT8 outCh = (UINT8)frameType;

    if (frameBytes)
    {
	for (int i = 0; i < frameBytes; i++)
	{
	    UINT8 inCh = *pSrc++;
	    *pDst++ = outCh | (FlipNibble(inCh >> 4) << 4);
	    outCh = FlipNibble(inCh & 0xf);
	}
    }
    else
	*pDst = outCh;
}

CAMRCodec::CAMRCodec() :
    m_lRefCount(0),
    m_pState(0),
    m_nConcealSamples(0),
    m_pFrameBuf(new UINT8[1 + ((CAMRFrameInfo::MaxFrameBits(NarrowBand) + 7) >> 3)])
{}

CAMRCodec::~CAMRCodec()
{
    if (m_pState)
    {
	Decoder_Interface_exit(m_pState);
	m_pState = 0;
    }
    
    delete [] m_pFrameBuf;
}

/*
 *	IUnknown methods
 */
STDMETHODIMP CAMRCodec::QueryInterface(REFIID riid, void** ppvObj)
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

STDMETHODIMP_(ULONG32) CAMRCodec::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CAMRCodec::Release()
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

STDMETHODIMP CAMRCodec::OpenDecoder(UINT32 cfgType, const void* config,
				    UINT32 nBytes)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pState)
    {
	Decoder_Interface_exit(m_pState);
    }
    
    m_pState = Decoder_Interface_init();

    if (m_pState)
	res = HXR_OK;

    return res;
}

STDMETHODIMP CAMRCodec::Reset()
{
    HX_RESULT res = HXR_OK;

    if (m_pState)
    {
	Decoder_Interface_exit(m_pState);
	m_pState = Decoder_Interface_init();

	if (!m_pState)
	    res = HXR_FAILED;
    }
    
    m_nConcealSamples = 0;

    return res;
}


STDMETHODIMP CAMRCodec::GetMaxSamplesOut(UINT32& nSamples) CONSTMETHOD
{
    nSamples = MaxSamples;
    return HXR_OK;
}

STDMETHODIMP CAMRCodec::GetNChannels(UINT32& nChannels) CONSTMETHOD
{
    nChannels = 1;
    return HXR_OK;
}

STDMETHODIMP CAMRCodec::GetSampleRate(UINT32& sampleRate) CONSTMETHOD
{
    sampleRate = 8000;
    return HXR_OK;
}

STDMETHODIMP CAMRCodec::GetDelay(UINT32& nSamples) CONSTMETHOD
{
    nSamples = 0;
    return HXR_OK;
}

STDMETHODIMP CAMRCodec::Decode(const UCHAR* data, UINT32 nBytes, 
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

        // Convert the RFC3267 Section 5.3 speech frame
	    // format into 3GPP TS 26.101 Annex A IF2 format
	    Convert2IF2(data, m_pFrameBuf);

	    // Get frame type.
	    int frameType = *m_pFrameBuf & 0xf;
	    int frameBytes = 
		1 + ((CAMRFrameInfo::FrameBits(NarrowBand, frameType) + 7) >> 3);

	    if ((int)nBytes >= frameBytes)
	    {
		Decoder_Interface_Decode(m_pState, 
					 m_pFrameBuf, 
					 samplesOut, 0);
		
		nBytesConsumed = frameBytes;

		nSamplesOut = MaxSamples;
		res = HXR_OK;
	    }
	}
    }
    
    return res;
}

STDMETHODIMP CAMRCodec::Conceal(UINT32 nSamples)
{
    // We want to conceal only in MaxSamples size blocks.
    UINT32 nFrameCount = (nSamples + MaxSamples - 1) / MaxSamples;
    UINT32 nConcealSamples = nFrameCount * MaxSamples;

    m_nConcealSamples += nConcealSamples;

    return HXR_OK;
}
