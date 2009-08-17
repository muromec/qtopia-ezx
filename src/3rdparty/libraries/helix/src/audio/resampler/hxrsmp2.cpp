/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrsmp2.cpp,v 1.10 2004/07/09 18:37:30 hubbe Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"

#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif

#include "hlxclib/string.h"
#include "timeval.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxtick.h"
#ifdef _MACINTOSH
#include "hxmm.h"
#endif

#include "hxthread.h"

//#include "resampl2.h"
#include "hxausvc.h"
#include "hxrsmp2.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// HXScheduler...
HXCDQualityResampler::HXCDQualityResampler() 
    :   m_lRefCount (0)
    ,	m_pResampler(NULL)
    ,	m_audioChannelConversion(AUDIO_CHANNEL_NONE)
    ,	m_audioSampleConversion(AUDIO_SAMPLE_NONE)
    ,	m_ulSamplesSaved(0)
    ,	m_ulSamplesFixed(0)
    ,	m_ulBytesFixed(0)
    ,   m_pBPS8To16Out(NULL)
{
}

HXCDQualityResampler::~HXCDQualityResampler()
{
    Close();
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP HXCDQualityResampler::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXAudioResampler), (IHXAudioResampler*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXAudioResampler*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXCDQualityResampler::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXCDQualityResampler::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 * IHXResampler methods
 */
STDMETHODIMP_(UINT32) 
HXCDQualityResampler::Resample(UINT16*	pInput, 
				UINT32	ulInputBytes, 
                                UINT16*	pOutput) 
{
    UINT32  ulOutputBytes = 0;
    UINT32  ulSamplesIn = 0;
    UINT32  ulFramesIn = 0;
    UINT32  ulSamplesOut = 0;
    UINT32  ulBytesSaved = 0;   
    UINT32  ulOut = 0;

    ulSamplesIn = (ulInputBytes * 8) / m_inAudioFormat.uBitsPerSample;
    
    ulSamplesOut = ulSamplesIn;
    ulOutputBytes = ulInputBytes;

    // channel conversion first(STEREO -> MONO)
    // MONO -> STEREO is handled by the caller(AudioStream)
    if (AUDIO_CHANNEL_DOWN == m_audioChannelConversion)
    {
	Downmix16((short*)pInput, ulSamplesIn);
	// half the output samples after channel conversion
	ulSamplesIn /= 2;
	ulSamplesOut = ulSamplesIn;
    }

    // resampler's output always be 16bits
    if (m_inAudioFormat.uBitsPerSample == 8 && m_outAudioFormat.uBitsPerSample == 16)
    {
	ulFramesIn = ulSamplesIn / m_inAudioFormat.uChannels;
	ulOut = m_inAudioFormat.uChannels * 2 * ulFramesIn;

	if (!m_pBPS8To16Out)
	{
	    m_pBPS8To16Out = (short*)new char[ulOut];
	}

	BPS8To16((short*)pInput, ulInputBytes, (short*)m_pBPS8To16Out, ulOut);
	pInput = (UINT16*)m_pBPS8To16Out;
    }

    if (m_pResampler)
    {
	if (m_ulSamplesSaved)
	{
	    ulBytesSaved = m_ulSamplesSaved * m_outAudioFormat.uBitsPerSample / 8;
	    HX_ASSERT(ulBytesSaved < m_ulBytesFixed);
            if (ulBytesSaved >= m_ulBytesFixed) ulBytesSaved = m_ulBytesFixed;
	    ::memcpy(pOutput, (UCHAR*)pOutput+m_ulBytesFixed, ulBytesSaved); /* Flawfinder: ignore */
	}

	ulSamplesOut = m_pResampler->Resample((short*)pInput,
					     ulSamplesIn,
					     (short*)((UCHAR*)pOutput + ulBytesSaved));

	m_ulSamplesSaved += ulSamplesOut - m_ulSamplesFixed;	
	ulOutputBytes = m_ulBytesFixed;
    }
    else
    {
	ulOutputBytes = (ulSamplesOut * m_outAudioFormat.uBitsPerSample) / 8;
        HX_ASSERT(ulOutputBytes < m_ulBytesFixed);
        if (ulOutputBytes >= m_ulBytesFixed) ulOutputBytes = m_ulBytesFixed;
	::memcpy(pOutput, pInput, ulOutputBytes); /* Flawfinder: ignore */
    }

    return ulOutputBytes;
}

STDMETHODIMP_(UINT32) 
HXCDQualityResampler::Requires(UINT32 outputFrames)
{
    UINT32 inputFrames = 0;
    UINT32 ulSamplesRequired = 0;

    // we only need to resample half of the output frames
    // since the caller will do the MONO -> STEREO channel conversion
    if (AUDIO_CHANNEL_UP == m_audioChannelConversion)
    {
	outputFrames /= 2;
    }

    // take into account of the extra samples from the last Resample()
    ulSamplesRequired = outputFrames * m_outAudioFormat.uChannels - m_ulSamplesSaved;
    m_ulSamplesFixed = outputFrames * m_outAudioFormat.uChannels;
    m_ulBytesFixed = (outputFrames * m_outAudioFormat.uChannels * m_outAudioFormat.uBitsPerSample) / 8;;

    if (m_pResampler)
    {
	inputFrames = m_pResampler->GetMinInput(ulSamplesRequired) / m_inAudioFormat.uChannels;
    }
    else
    {
	inputFrames = ulSamplesRequired / m_inAudioFormat.uChannels;
    }

    // we need to double the input frames for STEREO -> MONO channel
    // conversion
    if (AUDIO_CHANNEL_DOWN == m_audioChannelConversion)
    {
	inputFrames *= 2;
    }

    return inputFrames;
}

HX_RESULT
HXCDQualityResampler::Init(HXAudioFormat	inAudioFormat,
			    REF(HXAudioFormat)	outAudioFormat)
{
    HX_RESULT rc = HXR_OK;

    CopyAudioFormat(inAudioFormat, m_inAudioFormat);
    CopyAudioFormat(outAudioFormat, m_outAudioFormat);

    INT32   actualMaxSamplesIn = (m_inAudioFormat.uMaxBlockSize * 8) / m_inAudioFormat.uBitsPerSample;
    INT32   actualMaxSamplesOut = 0;
    INT32   actualMaxInputBytes = m_inAudioFormat.uMaxBlockSize;
    INT32   actualResamplerChannel = m_inAudioFormat.uChannels;

    // cleanup the mess
    Close();

    // this wrapper class will do the STEREO -> MONO channel conversion
    // MONO -> STEREO channel conversion will be done by the caller
    if (m_outAudioFormat.uChannels > m_inAudioFormat.uChannels)
    {
	m_audioChannelConversion = AUDIO_CHANNEL_UP;
    }
    else if (m_outAudioFormat.uChannels < m_inAudioFormat.uChannels)
    {
	m_audioChannelConversion = AUDIO_CHANNEL_DOWN;
	actualMaxInputBytes /= 2;
	actualResamplerChannel = m_outAudioFormat.uChannels;
    }
    else
    {
	m_audioChannelConversion = AUDIO_CHANNEL_NONE;
    }

    if (m_outAudioFormat.ulSamplesPerSec == m_inAudioFormat.ulSamplesPerSec)
    {
	m_audioSampleConversion = AUDIO_SAMPLE_NONE;
    }
    else
    {
	m_audioSampleConversion = AUDIO_SAMPLE_NEEDED;
    }

    if (AUDIO_SAMPLE_NONE != m_audioSampleConversion)
    {
	if (HXR_OK == RAExactResampler::Create(&m_pResampler,
					       m_inAudioFormat.ulSamplesPerSec,
					       m_outAudioFormat.ulSamplesPerSec,
					       actualResamplerChannel,
   					       RAExactResampler::_INT16)) // 16bit per sample output
	{
	    actualMaxSamplesOut = m_pResampler->GetMaxOutput(actualMaxSamplesIn);
	}
	else
	{
	    HX_DELETE(m_pResampler);
	    rc = HXR_FAILED;
	}
    }
    else
    {
	actualMaxSamplesOut = actualMaxSamplesIn;
    }

    outAudioFormat.uMaxBlockSize = m_outAudioFormat.uMaxBlockSize = (actualMaxSamplesOut * m_outAudioFormat.uBitsPerSample) / 8;
    
    return rc;
}

void
HXCDQualityResampler::Downmix16(INT16* pIn, UINT32 nSamplesIn)
{
    UINT32 i ;
    for (i = 0 ; i < nSamplesIn / 2 ; i++)
    {
	INT32 t = (INT32)pIn[2*i] + (INT32)pIn[2*i+1] ;
	pIn[i] = (INT16)(t>>1);
    }
}

void
HXCDQualityResampler::BPS8To16(INT16* pInput, UINT32 ulBytesIn, INT16* pOutput, UINT32& ulBytesOut)
{
    long inputFrames = ulBytesIn / (m_inAudioFormat.uBitsPerSample/8) / m_inAudioFormat.uChannels;
    long n = 0;

    if (1 == m_outAudioFormat.uChannels) 
    {	
	unsigned char* pIn = (unsigned char*) pInput;
	for (n = 0; n < inputFrames; n++) 
	{
	    pOutput[n] = (short) (((short)*pIn++ - 128) << 8);
	}
    }
    else if (2 == m_outAudioFormat.uChannels) 
    { 
        unsigned char* pIn = (unsigned char*) pInput;
	long m = 0;
	for (n = 0; n < inputFrames; n++ ) 
	{
	    pOutput[m] = (short) (((short)*pIn++ - 128) << 8);
	    m++;
	    pOutput[m] = (short) (((short)*pIn++ - 128) << 8);
	    m++;
	}
    } 

    ulBytesOut = m_inAudioFormat.uChannels * 2 * inputFrames;

    return;
}

void
HXCDQualityResampler::CopyAudioFormat(HXAudioFormat from, REF(HXAudioFormat) to)
{
    to.uChannels = from.uChannels;
    to.uBitsPerSample = from.uBitsPerSample;
    to.ulSamplesPerSec = from.ulSamplesPerSec;
    to.uMaxBlockSize = from.uMaxBlockSize;
}

void
HXCDQualityResampler::Close()
{
    HX_VECTOR_DELETE(m_pBPS8To16Out);
    HX_DELETE(m_pResampler);
}
