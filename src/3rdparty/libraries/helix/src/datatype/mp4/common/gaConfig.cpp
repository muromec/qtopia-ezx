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

/* parse an MPEG-4 general audio specific config structure from a bitstream */

#include "gaConfig.h"
#include "bitstream.h"

CGASpecificConfig::CGASpecificConfig()
: mFrameLengthFlag   (1)
, mDependsOnCoreCoder(1)
, mCoreCoderDelay    (14,0)
, mExtensionFlag     (1)
{
}

CGASpecificConfig::~CGASpecificConfig() {}

HX_RESULT CGASpecificConfig::Read(struct BITSTREAM &bs,
								UINT32 samplingFrequency,
								UINT32 ChannelConfiguration,
								UINT32 AudioObjectType)
{
	mFrameLengthFlag.Read(bs) ;
	mFrameLength = mFrameLengthFlag ? 960 : 1024 ;

	if (mDependsOnCoreCoder.Read(bs))
		mCoreCoderDelay.Read(bs) ;

	mExtensionFlag.Read(bs) ;

	if (ChannelConfiguration == 0)
	{
		mPCE.Read(bs) ;
	}
	else
	{
		HX_RESULT res = mPCE.SetChannelConfiguration(ChannelConfiguration) ;
		if (FAILED(res))
			return res ;
	}

	if (mExtensionFlag)
	{
		// to be defined in mpeg4 phase 2
		// should we return HXR_FAIL?
	}
	return HXR_OK ;
}

UINT32 CGASpecificConfig::GetFrameLength() const
{
	return mFrameLength ;
}

CPCE::CPCE()
: element_instance_tag         (4)
, object_type                  (2)
, sampling_frequency_index     (4)
, num_front_channel_elements   (4)
, num_side_channel_elements    (4)
, num_back_channel_elements    (4)
, num_lfe_channel_elements     (2)
, num_assoc_data_elements      (3)
, num_valid_cc_elements        (4)
, mono_mixdown_present         (1)
, mono_mixdown_element_number  (4,0)
, stereo_mixdown_present       (1)
, stereo_mixdown_element_number(4,0)
, matrix_mixdown_idx_present   (1)
, matrix_mixdown_idx           (2)
, pseudo_surround_enable       (1)
, comment_field_bytes          (8)
, comment_field_data           (0)
{
}

CPCE::~CPCE()
{
	if (comment_field_data) delete[] comment_field_data ;
}

UINT32 CPCE::GetNChannels() const
{
	return
		num_front_channels +
		num_side_channels +
		num_back_channels +
		num_lfe_channel_elements ;
}

const int CPCE::channelMapping[][4] =
{
	{0,0,0,0},
	{1,0,0,0}, // center front speaker
	{2,0,0,0}, // left, right front speakers
	{3,0,0,0}, // center front speaker, left, right front speakers
	{3,0,1,0}, // center front speaker, left, right center front speakers, rear surround speakers
	{3,0,2,0}, // center front speaker, left, right front speakers, left surround, right surround rear speakers
	{3,0,2,1}, //  center front speaker, left, right front speakers, left surround, right surround rear speakers, front low frequency effects speaker
	{5,0,2,1}, // center front speaker left, right center front speakers, left, right outside front speakers, left surround, right surround rear speakers, front low frequency effects speaker
} ;

/* Utility function for implizit channel config signalling.
   Initialize the channel pair information and return the number of channel elements. */

static int initChannelPairs(int n, int isCpe[])
{
	int nElem = (n+1)>>1 ;
	int i ;

	isCpe[0] = 0 ;
	for (i = (n & 1); i < nElem; i++)
		isCpe[i] = 1 ;

	return nElem ;
}

HX_RESULT CPCE::SetChannelConfiguration(UINT32 channelConfig)
{
	if (channelConfig >= 8)
		return HXR_FAIL ;

	num_front_channels = channelMapping[channelConfig][0] ;
	num_side_channels  = channelMapping[channelConfig][1] ;
	num_back_channels  = channelMapping[channelConfig][2] ;
	num_lfe_channel_elements = channelMapping[channelConfig][3] ;

	num_front_channel_elements = initChannelPairs(num_front_channels, front_element_is_cpe) ;
	num_side_channel_elements  = initChannelPairs(num_side_channels, side_element_is_cpe) ;
	num_back_channel_elements  = initChannelPairs(num_back_channels, back_element_is_cpe) ;

	return HXR_OK ;
}

HX_RESULT CPCE::Read(struct BITSTREAM &bs)
{
	UINT32 i ;

	num_front_channels = 0 ;
	num_side_channels  = 0 ;
	num_back_channels  = 0 ;

	/* should check here whether we have enough bits in the bitstream */

	element_instance_tag.Read(bs) ;
	object_type.Read(bs) ;
	sampling_frequency_index.Read(bs) ;
	num_front_channel_elements.Read(bs) ;
	num_side_channel_elements.Read(bs) ;
	num_back_channel_elements.Read(bs) ;
	num_lfe_channel_elements.Read(bs) ;
	num_assoc_data_elements.Read(bs) ;
	num_valid_cc_elements.Read(bs) ;

	if (mono_mixdown_present.Read(bs))
		mono_mixdown_element_number.Read(bs) ;

	if (stereo_mixdown_present.Read(bs))
		stereo_mixdown_element_number.Read(bs) ;

	if (matrix_mixdown_idx_present.Read(bs))
	{
		matrix_mixdown_idx.Read(bs) ;
		pseudo_surround_enable.Read(bs) ;
	}

	for ( i = 0; i < num_front_channel_elements; i++) {
		front_element_is_cpe[i] = readBits(&bs,1) ;
		num_front_channels += (1+front_element_is_cpe[i]) ;
		front_element_tag_select[i] = readBits(&bs,4) ;
	}
	for ( i = 0; i < num_side_channel_elements; i++) {
		side_element_is_cpe[i] = readBits(&bs,1) ;
		num_side_channels += (1+side_element_is_cpe[i]) ;
		side_element_tag_select[i] = readBits(&bs,4) ;
	}
	for ( i = 0; i < num_back_channel_elements; i++) {
		back_element_is_cpe[i] = readBits(&bs,1) ;
		num_back_channels += (1+back_element_is_cpe[i]) ;
		back_element_tag_select[i] = readBits(&bs,4) ;
	}
	for ( i = 0; i < num_lfe_channel_elements; i++)
		lfe_element_tag_select[i] = readBits(&bs,4) ;
	for ( i = 0; i < num_assoc_data_elements; i++)
		assoc_data_element_tag_select[i] = readBits(&bs,4) ;
	for ( i = 0; i < num_valid_cc_elements; i++) {
		cc_element_is_ind_sw[i] = readBits(&bs,1) ;
		valid_cc_element_tag_select[i] = readBits(&bs,4) ;
	}

	byteAlign(&bs) ;

	comment_field_bytes.Read(bs) ;

	if (comment_field_bytes)
	{
		// this is a slow way to read characters, but since we don't
		// parse many GAconfigs, I think that's OK
		CBitstreamInt t(8) ;

		comment_field_data = new char[(UINT32)comment_field_bytes] ;
		for (i = 0 ; i < comment_field_bytes ; i++)
		{
			comment_field_data[i] = (char)t.Read(bs) ;
		}
	}

	return HXR_OK ;
}

CAudioSpecificConfig::CAudioSpecificConfig()
: AudioObjectType        (5)
, samplingFrequencyIndex (4)
, samplingFrequency     (24)
, extensionSamplingFrequencyIndex (4)
, extensionSamplingFrequency     (24)
, channelConfiguration   (4)
{
}

CAudioSpecificConfig::~CAudioSpecificConfig() {}

const UINT32 CAudioSpecificConfig::aSampleRate[13] =
{
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
	11025, 8000, 7350
} ;

HX_RESULT CAudioSpecificConfig::Read(struct BITSTREAM &bs)
{
	HX_RESULT res = HXR_OK;

	AudioObjectType.Read(bs) ;
	samplingFrequencyIndex.Read(bs) ;
	extensionSamplingFrequencyIndex = samplingFrequencyIndex ;

	switch (samplingFrequencyIndex)
	{
	case 0xf: // ESC, read sr from bitstream
		samplingFrequency.Read(bs) ;
		break ;
	case 0xd: case 0xe: // these are reserved.
		return HXR_FAIL ;
		break ;
	default: // the normal case
		samplingFrequency = aSampleRate[samplingFrequencyIndex] ;
	}
	extensionSamplingFrequency = samplingFrequency ;

	channelConfiguration.Read(bs) ;

	m_bSBR = (AudioObjectType == AACSBR) ;
	if (m_bSBR)
	{
		extensionSamplingFrequencyIndex.Read(bs) ;
		switch (extensionSamplingFrequencyIndex)
		{
		case 0xf: // ESC, read sr from bitstream
			extensionSamplingFrequency.Read(bs) ;
			break ;
		case 0xd: case 0xe: // these are reserved.
			return HXR_FAIL ;
			break ;
		default: // the normal case
			extensionSamplingFrequency = aSampleRate[extensionSamplingFrequencyIndex] ;
		}
		AudioObjectType.Read(bs) ;
	}

	switch (AudioObjectType)
	{
	case AACMAIN: case AACLC: case AACLTP:
		res = GASpecificConfig.Read(bs,
			samplingFrequency,channelConfiguration,AudioObjectType) ;
		break ;

	default:
		// no other audio types supported
		return HXR_FAIL ;
	}

	if (!m_bSBR && bitsLeftInBitstream(&bs) >= 16)
	// if SBR info has not been read before, but there are more bytes to follow...
	{
		if (readBits(&bs,11) == 0x2b7)
		{
			if (readBits(&bs,5) == 5)
			{
				m_bSBR = readBits(&bs,1) ;
				if (m_bSBR)
				{
					extensionSamplingFrequencyIndex.Read(bs) ;
					switch (extensionSamplingFrequencyIndex)
					{
					case 0xf: // ESC, read sr from bitstream
						extensionSamplingFrequency.Read(bs) ;
						break ;
					case 0xd: case 0xe: // these are reserved.
						return HXR_FAIL ;
						break ;
					default: // the normal case
						extensionSamplingFrequency = aSampleRate[extensionSamplingFrequencyIndex] ;
					}
				}
			}
		}
	}

	return HXR_OK ;
}

UINT32 CAudioSpecificConfig::GetSampleRate() const
{
	return extensionSamplingFrequency ;
}

UINT32 CAudioSpecificConfig::GetCoreSampleRate() const
{
	return samplingFrequency ;
}

UINT32 CAudioSpecificConfig::GetCoreSamplingFrequencyIndex() const
{
    return samplingFrequencyIndex;
}

UINT32 CAudioSpecificConfig::GetObjectType() const
{
	return AudioObjectType ;
}
