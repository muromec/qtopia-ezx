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
 * directly from RealNetworks.	You may not use this file except in
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

// the following will enable ::QIFind() and HX_RELEASE()
#define INITGUID	    1

#include "mmf/server/mmfcodec.h"
#include "mmf/server/mmfdatabuffer.h"
#include "mmf/plugin/mmfcodecimplementationuids.hrh"
#include "hxtlogutil.h"
#include "gaConfig.h" // CAudioSpecificConfig
#include "audioconfigurator.h"
#include "audioconfigs.h"
#include "symbianaudiodecoder.h"
#include "symbianswdecoder.h"

HXSymbianSwAudioDecoder::HXSymbianSwAudioDecoder():
	m_pSoftwareCodec(NULL),	// set by OpenDecoder()
	m_pSourceBuffer(NULL),	// set by OpenDecoder()
	m_pDestBuffer(NULL)	 // set by OpenDecoder()
{
}

HXSymbianSwAudioDecoder::~HXSymbianSwAudioDecoder()
{
    HX_DELETE(m_pSoftwareCodec);
    HX_DELETE(m_pSourceBuffer);
    HX_DELETE(m_pDestBuffer);
}

//----------------------------------------------------------------------------
// IHXAudioDecoder
//----------------------------------------------------------------------------

// configure the decoder. Needs to be called before starting to decode.
//
// cfgType: The interpretation depends on the decoder. In the case of an AAC
//	  decoder, for example, it tells the decoder whether it is
//	  configured with an ADIF header, ADTS header, or
//	  MPEG-4 AudioSpecificConfig
// hdr: a pointer to the config data.
// nBytes: the number of bytes in the config data.
//
HX_RESULT HXSymbianSwAudioDecoder::OpenDecoder(UINT32 cfgType,
					       const void* config,
					       UINT32 nBytes)
{
    HX_RESULT result = HXR_OK;
    HXAudioConfigurator* pAudConfig = NULL;

    // make sure fourCC has been set
    if(m_pFourCC[0] == '\0')
    {
        HXLOGL2(HXLOG_MDFA,
                "HXSymbianSwAudioDecoder::OpenDecoder() HXR_FAIL: FourCC not set");

	result = HXR_FAIL;
    }

    if(SUCCEEDED(result))
    {
	// call factory method to create appropriate configurator
	pAudConfig = CreateAudioConfigFromFourCC(m_pFourCC);

	if(!pAudConfig)
	{
            HXLOGL2(HXLOG_MDFA,
                    "HXSymbianSwAudioDecoder::OpenDecoder() HXR_NOT_SUPPORTED: FourCC=%s",
		    m_pFourCC);

	    result = HXR_NOT_SUPPORTED;
	}
    }

    if(SUCCEEDED(result))
    {
	// validate configuration parameters
	result = pAudConfig->ValidateDecoderConfig(cfgType, config, nBytes);
    }

    if(SUCCEEDED(result))
    {
	HXSymbianAudioDecoderBase::SetAudioConfig(pAudConfig);

	TRAPD(exitV1, m_pSourceBuffer = CMMFDataBuffer::NewL());
	TRAPD(exitV2, m_pDestBuffer = CMMFDataBuffer::NewL());
	TRAPD(exitV0, m_pSoftwareCodec = CMMFCodec::NewL(
					m_pAudConfig->CodecFourCC(),
					KMMFFourCCCodePCM16));

	if(KErrNone != exitV0 || KErrNone != exitV1 || KErrNone != exitV2 ||
	   !m_pSourceBuffer || !m_pDestBuffer || !m_pSoftwareCodec)
	{
	    HX_DELETE(m_pSourceBuffer);
	    HX_DELETE(m_pDestBuffer);
	    HX_DELETE(m_pSoftwareCodec);

	    result = HXR_FAIL;
	}
    }

    if(SUCCEEDED(result))
    {
	// perform codec specific configuration
	result = m_pAudConfig->ConfigureDecoder(m_pSoftwareCodec);
    }

    return result;
}

// reset the decoder to the state just after OpenDecoder().
// Use this when seeking or when decoding has returned with an error.
//
HX_RESULT HXSymbianSwAudioDecoder::Reset()
{
    HX_RESULT result = HXR_OK;

    if(m_pSoftwareCodec)
    {
	// ResetL() is optional; Trap and ignore KErrNotUnsupported
	TRAPD(err, m_pSoftwareCodec->ResetL());
	switch(err)
	{
	    case KErrNone:
	    case KErrNotSupported:
		// ok
		break;
	    default:
		// not ok
		result = HXR_FAIL;
	}
    }

    return result;
}

// Decode up to nbytes bytes of bitstream data. nBytesConsumed will be updated to
// reflect how many bytes have been read by the decoder (and can thus be discarded
// in the caller). The decoder does not necessarily buffer the bitstream; that is
// up to the caller.
// samplesOut should point to an array of INT16s, large enough to hold MaxSamplesOut()
// samples.
//
// nSamplesOut is updated to reflect the number of samples written by the decoder.
//
// set eof when no more data is available and keep calling the decoder until it does
// not produce any more output.
//
HX_RESULT HXSymbianSwAudioDecoder::Decode(const UCHAR* data,
					  UINT32 nBytes,
					  UINT32 &nBytesConsumed,
					  INT16 *samplesOut,
					  UINT32& nSamplesOut,
					  HXBOOL eof)
{
    HX_RESULT status = HXR_OK;

    // check for concealment
    UINT32 ulConcealed = m_pAudConfig->GetCurrentSamplesToConceal();
    if(ulConcealed > 0)
    {
        HXLOGL4(HXLOG_MDFA,
                "HXSymbianSwAudioDecoder::Decode() Concealing %lu samples",
                ulConcealed);

	// output silence for concealed samples
	memset(samplesOut, 0, ulConcealed * sizeof(INT16));

	// update the counters
	nBytesConsumed = 0L;
	nSamplesOut = ulConcealed;
    }
    else
    {
	// set up the source data
	TDes8& pSrcDes = m_pSourceBuffer->Data();
	TPtr8& pSrc = (TPtr8&)pSrcDes;
	pSrc.Set((TUint8*)data, nBytes, nBytes);

	// set up the dest data
	UINT32 ulOutSize = m_pAudConfig->MaxBytesPerFrame();
	TDes8& pDstDes = m_pDestBuffer->Data();
	TPtr8& pDst = (TPtr8&) pDstDes;
	pDst.Set((TUint8*)samplesOut, ulOutSize, ulOutSize);

	// perform the decoding
	if(m_pSoftwareCodec)
	{
	    TCodecProcessResult result;

	    TRAPD(err, result = m_pSoftwareCodec->ProcessL(*m_pSourceBuffer, *m_pDestBuffer));

	    // on error, consume the corrupted frame and return 0's for encoded data
	    if(err != KErrNone ||
	       (result.iStatus != TCodecProcessResult::EProcessComplete && // normal success
		result.iStatus != TCodecProcessResult::EProcessIncomplete && // input not empty, output full
		result.iStatus != TCodecProcessResult::EDstNotFilled && // input empty, output not full
		result.iStatus != TCodecProcessResult::EEndOfData)) // end of data
	    {
                HXLOGL4(HXLOG_MDFA,
                        "HXSymbianSwAudioDecoder::Decode() Warning: Consuming corrupted frame");

		// update the counters
		nBytesConsumed = nBytes;
		nSamplesOut = m_pAudConfig->SamplesPerFrame();

		// conceal the problem for this one frame
		memset(samplesOut, 0, m_pAudConfig->MaxBytesPerFrame());
	    }
	    else
	    {
		// update the counters
		nBytesConsumed = (UINT32) result.iSrcBytesProcessed;
		nSamplesOut = ConvertBytesToSamples(result.iDstBytesAdded);
	    }
	}
    }

    return status;
}

