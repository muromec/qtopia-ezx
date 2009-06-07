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

#include "mmf/server/mmfcodec.h"
#include "mmf/plugin/mmfCodecImplementationUIDs.hrh"
#include "hxtlogutil.h"
#include "bitstream.h"
#include "gaConfig.h" // CAudioSpecificConfig
#include "aacconstants.h"
#include "audioconfigurator.h"
#include "audioconfigs.h"

extern "C" int newBitstream(struct BITSTREAM **ppBitstream, int nBits);
extern "C" void deleteBitstream(struct BITSTREAM *pBitstream);
extern "C" int feedBitstream(struct BITSTREAM *pBitstream, const unsigned char *input, int nbits);
extern "C" int setAtBitstream(struct BITSTREAM *pBitstream, int position, int direction);

//--- FACTORY METHOD -----------------------------------------------------------

HXAudioConfigurator* CreateAudioConfigFromFourCC(const char* pFourCC)
{
    HXAudioConfigurator* pAudConfig;

    HXLOGL2(HXLOG_MDFA, "CreateAudioConfigFromFourCC() FourCC=%s", pFourCC);

    // check if codec is supported
    if(!strcmp(pFourCC, "raac") || !strcmp(pFourCC, "aac4"))
    {
	pAudConfig = new HXAudioConfiguratorAAC;
    }
    else
    {
	if(!strcmp(pFourCC, "amrn"))
	{
	    pAudConfig = new HXAudioConfiguratorAMRNB;
	}
	else
	{
	    if(!strcmp(pFourCC, "amrw"))
	    {
		pAudConfig = new HXAudioConfiguratorAMRWB;
	    }
	    else
	    {
		HXLOGL2(HXLOG_MDFA,
			"CreateAudioConfigFromFourCC() FourCC not supported, fourCC=%s",
			pFourCC);

		pAudConfig = NULL;
	    }
	}
    }

    return pAudConfig;
}

//--- AMR-NB -------------------------------------------------------------------

// local constants
#define AMRNB_INPUT_FRAME_LENGTH	160
#define AMRNB_SAMPLES_PER_FRAME		160
#define AMRNB_NUMBER_OF_CHANNELS	1
#define AMRNB_BITS_PER_SAMPLE		16
#define AMRNB_SAMPLES_PER_SECOND	8000
#define AMRNB_CODEC_DELAY_IN_SAMPLES	0


HXAudioConfiguratorAMRNB::HXAudioConfiguratorAMRNB():
	HXAudioConfigurator(KMMFFourCCCodeAMR,
			    AMRNB_INPUT_FRAME_LENGTH,
			    AMRNB_SAMPLES_PER_FRAME,
			    AMRNB_NUMBER_OF_CHANNELS,
			    AMRNB_BITS_PER_SAMPLE,
			    AMRNB_SAMPLES_PER_SECOND,
			    AMRNB_CODEC_DELAY_IN_SAMPLES)
{
}

//--- AMR-WB -------------------------------------------------------------------

// local constants
#define AMRWB_INPUT_FRAME_LENGTH	320
#define AMRWB_SAMPLES_PER_FRAME		320
#define AMRWB_NUMBER_OF_CHANNELS	1
#define AMRWB_BITS_PER_SAMPLE		16
#define AMRWB_SAMPLES_PER_SECOND	16000
#define AMRWB_CODEC_DELAY_IN_SAMPLES	0


HXAudioConfiguratorAMRWB::HXAudioConfiguratorAMRWB():
	HXAudioConfigurator(KMMFFourCCCodeAWB,
			    AMRWB_INPUT_FRAME_LENGTH,
			    AMRWB_SAMPLES_PER_FRAME,
			    AMRWB_NUMBER_OF_CHANNELS,
			    AMRWB_BITS_PER_SAMPLE,
			    AMRWB_SAMPLES_PER_SECOND,
			    AMRWB_CODEC_DELAY_IN_SAMPLES)
{
}

//--- AAC ----------------------------------------------------------------------

#define KMMFFourCCCodeEAACP		0x43414520  // ' ' 'E' 'A' 'C'

// local constants
#define AAC_BITS_PER_SAMPLE		16

// From ISO/IEC 14496-3:2001/Amd.1:2003(E), Table 1.8A, p. 8
#define SBR_DOWNSAMPLE_CUTOFF_RATE	24000
#define MAX_SBR_OUT_SAMPLE_RATE		48000

// Symbian based profile object types used by the decoder
//   Audio Profile Object Type
//   From ISO/IEC 14496-3:2001(E) Subpart 4, Table 4.56, p. 40
#define AAC_PROFILE_UNKNOWN	    -1
#define AAC_PROFILE_MP			0  // not supported
#define AAC_PROFILE_LC			1
#define AAC_PROFILE_SSR			2  // not supported
#define AAC_PROFILE_LTP			3

// Helix AudioObjectType returned from CAudioSpecificConfig::getObjectType()
//   Audio Object Type
//   From ISO/IEC 14496-3:2001/Amd.1:2003(E), Table 1.1, p. 2
#define AAC_OBJECT_TYPE_MAIN		1  // not supported
#define AAC_OBJECT_TYPE_LC		2
#define AAC_OBJECT_TYPE_SSR		3  // not supported
#define AAC_OBJECT_TYPE_LTP		4
#define AAC_OBJECT_TYPE_SBR		5
#define AAC_OBJECT_TYPE_SCALABLE	6  // not supported
#define OBJECT_TYPE_TWINVQ		7  // not supported

HXAudioConfiguratorAAC::HXAudioConfiguratorAAC():
	HXAudioConfigurator(0,	// fourCC, set by StoreDecoderInfo()
			    0,	// uInputFrameLength, set by StoreDecoderInfo()
			    0,	// uSamplesPerFrame, set by StoreDecoderInfo()
			    0,	// uNumChannels, set by StoreDecoderInfo()
			    0,	// uBitsPerSample, set by StoreDecoderInfo()
			    0,	// ulSamplesPerSecond, set by StoreDecoderInfo()
			    0), // ulCodecDelayInSamples
	m_nProfileType(0),
	m_uDownSampleMode(0),
	m_ulOutSamplesPerSecond(0L),
	m_uExtObjectType(0)
{
}

HX_RESULT HXAudioConfiguratorAAC::ValidateDecoderConfig(UINT32 cfgType,
							const void* config,
							UINT32 nBytes)
{
    HX_RESULT result = HXR_OK;

    // decode the configuration bytes into the CAudioSpecificConfig structure
    CAudioSpecificConfig* pASConfig = new CAudioSpecificConfig();
    result = ExtractDecoderInfo(cfgType, config, nBytes, pASConfig);

    if(SUCCEEDED(result))
    {
	// store Symbian-based decoder info from CAudioSpecificConfig
	result = StoreDecoderInfo(pASConfig);
    }

    HX_DELETE(pASConfig);

    if(SUCCEEDED(result))
    {
	result = ValidateDecoderInfo();
    }

    return result;
}

HX_RESULT HXAudioConfiguratorAAC::ExtractDecoderInfo(UINT32 cfgType,
						     const void* config,
						     UINT32 nBytes,
						     CAudioSpecificConfig* pASConfig)
{
    HX_RESULT result = HXR_OK;
    struct BITSTREAM *pBitStream = NULL;

    if(cfgType != eAACConfigAudioSpecificCfg)
    {
	result = HXR_INVALID_PARAMETER;
    }

    if(SUCCEEDED(result))
    {
	// allocate bit stream
	if(newBitstream(&pBitStream, 8*nBytes))
	{
	    result = HXR_OUTOFMEMORY;
	}
    }

    if(SUCCEEDED(result))
    {
	// decode bit stream into configuration structure
	feedBitstream(pBitStream, (unsigned char*)config, 8*nBytes) ;
	setAtBitstream(pBitStream, 0, 1);
	result = pASConfig->Read(*pBitStream);
    }

    if(pBitStream)
    {
	// deallocate bit stream
	deleteBitstream(pBitStream);
    }

    return result;
}

HX_RESULT HXAudioConfiguratorAAC::StoreDecoderInfo(CAudioSpecificConfig* pASConfig)
{
    HX_RESULT result = HXR_OK;

    m_uNumChannels = (UINT16) pASConfig->GetNChannels();
    m_uInputFrameLength = pASConfig->GetFrameLength();
    m_uBitsPerSample = AAC_BITS_PER_SAMPLE; // (16)
    m_ulSamplesPerSecond = (UINT32) pASConfig->GetCoreSampleRate();

    m_uDownSampleMode = 0;
    m_uSamplesPerFrame = (UINT16) (m_uNumChannels * m_uInputFrameLength);
    m_ulOutSamplesPerSecond = m_ulSamplesPerSecond;
    m_uExtObjectType = 0;
    m_bIsSBR = (pASConfig->GetIsSBR()) ? TRUE: FALSE;

    //
    // The following block of code is derived from
    // ISO/IEC 14496-3:2001/Amd.1:2003(E), section 1.6.5.x.
    //

    if(!m_bIsSBR)
    {
	// assume implicit SBR signalling only if object type is AAC_LC
	if(pASConfig->GetObjectType() == AAC_OBJECT_TYPE_LC)
	{
	    // From ISO/IEC 14496-3:2001/Amd.1:2003(E), Table 1.8A, Note 1
	    if(m_ulSamplesPerSecond > SBR_DOWNSAMPLE_CUTOFF_RATE)
	    {
		// Table 1.8A, Note 1
		//	(m_ulSamplesPerSecond > 24 Khz)
		//	=> downsample = TRUE
		// Table 1.15A, Note 1
		//	((2 * m_ulSamplesPerSecond) > 48 Khz)
		//	This test is equivalent to the above (m_ulSamplesPerSecond > 24Khz).

		//
		// The SBR Tool operates at double the input rate which,
		// in this case, would exceed its maximum allowed rate.
		// Therefore, we must operate the SBR Tool in down sampled mode.
		m_uDownSampleMode = 1;
	    }
	    else
	    {
		// Table 1.15A, Note 1
		//	Extension Audio Object Type is not present or is
		//	not SBR. Therefore, we assume implicit SBR signalling.
		//	If no SBR data is found during decoding, the SBR Tool will
		//	be used for the sole purpose of upsampling.
		m_uExtObjectType = AAC_OBJECT_TYPE_SBR;

		m_ulOutSamplesPerSecond = 2 * m_ulSamplesPerSecond;

		// Non-down-sampled SBR Tool yields twice the output rate.
		// Double the output rate => double the samples per frame.
		m_uSamplesPerFrame *= 2;
	    }
	}
    }
    else
    {
	m_uExtObjectType = AAC_OBJECT_TYPE_SBR;

	// Table 1.15A, Note 3
	//	Extension Audio Object has explicitly signalled SBR.
	//	Therefore, the output sample rate is set to the given
	//	extension sampling rate signalled at the end of the
	//	Audio Specific Config.
	m_ulOutSamplesPerSecond = pASConfig->GetSampleRate(); // Extension Sample Rate

	if(m_ulOutSamplesPerSecond == m_ulSamplesPerSecond ||
	   m_ulOutSamplesPerSecond > MAX_SBR_OUT_SAMPLE_RATE)
	{
	    // Section 1.6.5.4
	    //	The down sampled SBR Tool shall be operated if the
	    //	output sample rate would otherwise exceed the max
	    //	allowed output sample rate, or if the extension
	    //	sample rate equals the input sample rate
	    m_uDownSampleMode = 1;
	}
	else
	{
	    // The extension sample rate should be twice in the input
	    // sample rate.
	    if(m_ulOutSamplesPerSecond != (2 * m_ulSamplesPerSecond))
	    {
		result = HXR_INVALID_PARAMETER;
	    }
	    else
	    {
		// Non-down-sampled SBR Tool yields twice the output rate.
		// Double the output rate => double the samples per frame.
		m_uSamplesPerFrame *= 2;;
	    }
	}
    }

    // update the codec delay
    m_ulCodecDelayInSamples = 2 * m_uSamplesPerFrame;

    if(SUCCEEDED(result))
    {
	result = SetFourCC();
    }

    if(SUCCEEDED(result))
    {
	SetProfile(pASConfig->GetObjectType());
    }

    return result;
}

HX_RESULT HXAudioConfiguratorAAC::SetFourCC()
{
    HX_RESULT result = HXR_OK;

    // first check for EAACP codec
    CMMFCodec* pCodec = NULL;
    m_FourCC = KMMFFourCCCodeEAACP;
    TRAPD(exit, pCodec = CMMFCodec::NewL(m_FourCC, KMMFFourCCCodePCM16));

    if(KErrNone != exit)
    {
	// fall back to AAC codec
	m_FourCC = KMMFFourCCCodeAAC;

	// undo eAAC+ settings
	if(m_ulOutSamplesPerSecond == (2 * m_ulSamplesPerSecond))
	{
	    m_ulOutSamplesPerSecond = m_ulSamplesPerSecond;
	    m_uSamplesPerFrame /= 2;
	    m_ulCodecDelayInSamples = m_uSamplesPerFrame;
	}
    }

    HX_DELETE(pCodec);

    return result;
}

void HXAudioConfiguratorAAC::SetProfile(UINT16 uObjectType)
{
    // map Object Type to a supported Profile Type
    switch(uObjectType)
    {
    case AAC_OBJECT_TYPE_LC:
	m_nProfileType = AAC_PROFILE_LC;
	break;
    case AAC_OBJECT_TYPE_LTP:
	m_nProfileType = AAC_PROFILE_LTP;
	break;
    default:
	m_nProfileType = AAC_PROFILE_UNKNOWN;
    }
}

HX_RESULT HXAudioConfiguratorAAC::ValidateDecoderInfo()
{
    HX_RESULT result = HXR_OK;

    // we only support up to 2 channels
    if(m_uNumChannels != 1 && m_uNumChannels != 2)
    {
	HXLOGL2(HXLOG_MDFA,
		"HXAudioConfiguratorAAC::ValidateDecoderInfo() HXR_NOT_SUPPORTED: number of channels=%d",
		 m_uNumChannels);

	result = HXR_NOT_SUPPORTED;
    }
    else
    {
	// our CMMFCodec only supports LC or LTP AAC profiles
	if(m_nProfileType != AAC_PROFILE_LC &&
	   m_nProfileType != AAC_PROFILE_LTP)
	{
	    HXLOGL2(HXLOG_MDFA,
		    "HXAudioConfiguratorAAC::ValidateDecoderInfo() HXR_NOT_SUPPORTED: profile type=%d",
		     m_nProfileType);
	    HXLOGL2(HXLOG_MDFA, "Profile Type: Unknown(-1), MP(0), LC(1), SSR(2), LTP(3)");

	    result = HXR_NOT_SUPPORTED;
	}
	else
	{
	    // SBR should only occur with LC profile
	    if(m_uExtObjectType == AAC_OBJECT_TYPE_SBR &&
	       m_nProfileType != AAC_PROFILE_LC)
	    {
		HXLOGL2(HXLOG_MDFA,
		    "HXAudioConfiguratorAAC::ValidateDecoderInfo() INVALID_PARAMETER: SBR with non-LC profile type=%d",
		     m_nProfileType);
		HXLOGL2(HXLOG_MDFA, "Profile Type: Unknown(-1), MP(0), LC(1), SSR(2), LTP(3)");

		result = HXR_INVALID_PARAMETER;
	    }
	}
    }

    return result;
}

HX_RESULT HXAudioConfiguratorAAC::ConfigureDecoder(CMMFCodec* pDecoder)
{
    HX_RESULT result = HXR_OK;

    if(m_FourCC == KMMFFourCCCodeEAACP)
    {
	result = ConfigureEAACPDecoder(pDecoder);
    }
    else
    {
	result = ConfigureAACDecoder(pDecoder);
    }

    return result;
}

HX_RESULT HXAudioConfiguratorAAC::ConfigureEAACPDecoder(CMMFCodec* pDecoder)
{
    HX_RESULT result = HXR_OK;

    HXLOGL2(HXLOG_MDFA, "HXAudioConfiguratorAAC::ConfigureEAACPDecoder()");
    HXLOGL2(HXLOG_MDFA, "Samples Per Second     : %lu", m_ulSamplesPerSecond);
    HXLOGL2(HXLOG_MDFA, "Number of Channels     : %u", m_uNumChannels);
    HXLOGL2(HXLOG_MDFA, "Profile Type           : %d", m_nProfileType);
    HXLOGL2(HXLOG_MDFA, "Profile Type: Unknown(-1), MP(0), LC(1), SSR(2), LTP(3)");
    HXLOGL2(HXLOG_MDFA, "Samples Per Frame      : %u", m_uSamplesPerFrame);
    HXLOGL2(HXLOG_MDFA, "Frame Length           : %u", m_uInputFrameLength);
    HXLOGL2(HXLOG_MDFA, "Samples Per Second     : %lu", m_ulSamplesPerSecond);
    HXLOGL2(HXLOG_MDFA, "Down Sample Mode       : %u", m_uDownSampleMode);
    HXLOGL2(HXLOG_MDFA, "Bits Per Sample        : %u", m_uBitsPerSample);
    HXLOGL2(HXLOG_MDFA, "Output SamplesPerSecond: %lu", m_ulOutSamplesPerSecond);
    HXLOGL2(HXLOG_MDFA, "Extension Object Type  : %u", m_uExtObjectType);
    HXLOGL2(HXLOG_MDFA, "Object Type            : Main(1), LC(2), SSR(3), LTP(4), SBR(5), SCALABLE(6), TWINVQ(7)");

    RArray<TInt> configParams;
    configParams.Append(m_ulSamplesPerSecond);	//  0: Input Sample Frequency
    configParams.Append(m_uNumChannels);	  //  1: Num Channels [1|2]
    configParams.Append(m_nProfileType);	  //  2: Input Profile Object type [1 - LC, 3 - LTP]
    configParams.Append(m_uSamplesPerFrame);	//  3: Output Frame Size
    configParams.Append(m_uInputFrameLength);	//  4: Input Frame Len [1024, 960]
    configParams.Append(m_ulSamplesPerSecond);	//  5: Input Sample Rate
    configParams.Append(0);			//  6: 0
    configParams.Append(m_uDownSampleMode);	//  7: Down Sample Mode [0|1]
    configParams.Append(m_uBitsPerSample);	//  8: Sample resolution, 8Khz (8-bit PCM) or 16Khz (16-bit)
    configParams.Append(m_ulOutSamplesPerSecond); //  9: Output Sample Frequency
    configParams.Append(m_uExtObjectType);	// 10: Extension Object Type

    TUid codecId = TUid::Uid(KUidMmfCodecAudioSettings);

    TRAPD(err, pDecoder->ConfigureL(codecId, (TDesC8&) configParams));
    if(KErrNone != err)
    {
	result = HXR_FAIL;
    }

    configParams.Close();

    // don't change the sample rate if we operate the SBR tool in
    // downsampled mode
    if(m_uDownSampleMode == 0)
    {
	// now that we're done configuring, update the true sample rate
	m_ulSamplesPerSecond = m_ulOutSamplesPerSecond;
    }

    return result;
}

HX_RESULT HXAudioConfiguratorAAC::ConfigureAACDecoder(CMMFCodec* pDecoder)
{
    HXLOGL2(HXLOG_MDFA, "HXAudioConfiguratorAAC::ConfigureAACDecoder()");
    HXLOGL2(HXLOG_MDFA, "Samples Per Second     : %lu", m_ulSamplesPerSecond);
    HXLOGL2(HXLOG_MDFA, "Number of Channels     : %u", m_uNumChannels);
    HXLOGL2(HXLOG_MDFA, "Profile Type           : %d", m_nProfileType);
    HXLOGL2(HXLOG_MDFA, "Profile Type           : Unknown(-1), MP(0), LC(1), SSR(2), LTP(3)");
    HXLOGL2(HXLOG_MDFA, "Frame Length           : %u", m_uInputFrameLength);
    HXLOGL2(HXLOG_MDFA, "Bits Per Sample        : %u", m_uBitsPerSample);

    HX_RESULT result = HXR_OK;

    RArray<TInt> configParams;
    configParams.Append(m_ulSamplesPerSecond); // Input Sample Rate
    configParams.Append(m_uNumChannels);       // Num Channels [1|2]
    configParams.Append(m_nProfileType);       // AAC Input Profile [1 - LC, 3 - LTP]
    configParams.Append(m_uInputFrameLength);  // Input Frame Len [1024, 960]
    configParams.Append(0);		    // AAC Down Mixing [0-none | 1 mono | 2 stereo]
    configParams.Append(0);		    // Aac output channels selection {0 - none, 1 - 1, 2 - 2}
    configParams.Append(0);		    // Aac decimation factor {0 - none, 2 - decimation by 2, 4 - decimation by 4}
    configParams.Append(0);		    // Aac concealment - It can be {0 - none, 1 - basic}
    configParams.Append(m_uBitsPerSample);     // Sample resolution - It can be {16 - 16-bit resolution}
    configParams.Append(0);		    // Sample Rate Conversion 0 : none

    TUid codecId = TUid::Uid(KUidMmfCodecAudioSettings);

    TRAPD(err, pDecoder->ConfigureL(codecId, (TDesC8&) configParams));
    if(KErrNone != err)
    {
	result = HXR_FAIL;
    }

    configParams.Close();

    return result;
}

