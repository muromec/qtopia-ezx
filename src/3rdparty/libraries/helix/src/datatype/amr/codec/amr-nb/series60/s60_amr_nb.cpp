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

#include "./s60_amr_nb.h"
#include "amr_frame_hdr.h"

const UINT32 MaxSamples = 160;

CS60AMRCodec::CS60AMRCodec() :
    m_lRefCount(0),
    m_pDecoder(0),
    m_nConcealSamples(0)
{}

CS60AMRCodec::~CS60AMRCodec()
{
    if (m_pDecoder)
    {
	delete m_pDecoder;
	m_pDecoder = 0;
    }
}

/*
 *	IUnknown methods
 */
STDMETHODIMP CS60AMRCodec::QueryInterface(REFIID riid, void** ppvObj)
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

STDMETHODIMP_(ULONG32) CS60AMRCodec::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CS60AMRCodec::Release()
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

STDMETHODIMP CS60AMRCodec::OpenDecoder(UINT32 cfgType, const void* config,
				    UINT32 nBytes)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pDecoder)
    {
	delete m_pDecoder;
	m_pDecoder = 0;
    }

    TInt exitValue;
    TAmrDecParams conf;

    // Init call for EPOC AMR codec has to be inside a trap to avoid 
    // a kernel panic
    conf.iConcealment = 0;
    TRAP( exitValue, m_pDecoder = CAmrToPcmDecoder::NewL(conf) );

    if (!exitValue && m_pDecoder)
    {
	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP CS60AMRCodec::Reset()
{
    HX_RESULT res = HXR_OK;

    if (m_pDecoder)
    {
	m_pDecoder->Reset();
    }
    
    m_nConcealSamples = 0;

    return res;
}


STDMETHODIMP CS60AMRCodec::GetMaxSamplesOut(UINT32& nSamples) CONSTMETHOD
{
    nSamples = MaxSamples;
    return HXR_OK;
}

STDMETHODIMP CS60AMRCodec::GetNChannels(UINT32& nChannels) CONSTMETHOD
{
    nChannels = 1;
    return HXR_OK;
}

STDMETHODIMP CS60AMRCodec::GetSampleRate(UINT32& sampleRate) CONSTMETHOD
{
    sampleRate = 8000;
    return HXR_OK;
}

STDMETHODIMP CS60AMRCodec::GetDelay(UINT32& nSamples) CONSTMETHOD
{
    nSamples = 0;
    return HXR_OK;
}

STDMETHODIMP CS60AMRCodec::Decode(const UCHAR* data, UINT32 nBytes, 
			       UINT32 &nBytesConsumed, INT16 *samplesOut, 
			       UINT32& nSamplesOut, HXBOOL eof)
{
    HX_RESULT res = HXR_UNEXPECTED;

    nSamplesOut = 0;
    nBytesConsumed = 0;

    if (m_pDecoder)
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

	    for (UINT32 i = 0; i < nSamplesOut; i++)
		*samplesOut++ = 0;

	    m_nConcealSamples -= nSamplesOut;

	    res = HXR_OK;
	}
	else if (nBytes)
	{
	    TInt consumed = 0;
	    TInt outSize = MaxSamples * 2;

	    if (m_pDecoder->Decode((UCHAR*)data, consumed,
				   (UCHAR*)samplesOut, outSize, FALSE) == 0)
	    {
		nSamplesOut = outSize / 2;
		nBytesConsumed = consumed;
		res = HXR_OK;
	    }
	}
	else
	{
	    res = HXR_OK;
	}
    }
    
    return res;
}

STDMETHODIMP CS60AMRCodec::Conceal(UINT32 nSamples)
{
    // We want to conceal only in MaxSamples size blocks.
    UINT32 nFrameCount = (nSamples + MaxSamples - 1) / MaxSamples;
    UINT32 nConcealSamples = nFrameCount * MaxSamples;

    m_nConcealSamples += nConcealSamples;

    return HXR_OK;
}
