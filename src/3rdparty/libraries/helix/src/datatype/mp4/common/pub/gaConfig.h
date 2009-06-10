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

#ifndef GACONFIG_H
#define GACONFIG_H

#include "hxresult.h"
#include "hxtypes.h"
#include "bitstream.h"
#include "bitstrmint.h"

class CPCE
{
public:
	CPCE() ;
	~CPCE() ;

	HX_RESULT Read(struct BITSTREAM &bs) ;
	HX_RESULT SetChannelConfiguration(UINT32 channelConfig) ;

	UINT32 GetNChannels() const ;
private:
	enum {
		MAX_FRONT_ELEMS = 16,
		MAX_SIDE_ELEMS = 16,
		MAX_BACK_ELEMS = 16,
		MAX_LFE_ELEMS = 4,
		MAX_ASSOC_ELEMS = 8,
		MAX_CC_ELEMS = 16
	} ;

	CBitstreamInt element_instance_tag ;
	CBitstreamInt object_type ;
	CBitstreamInt sampling_frequency_index ;
	CBitstreamInt num_front_channel_elements ;
	CBitstreamInt num_side_channel_elements ;
	CBitstreamInt num_back_channel_elements ;
	CBitstreamInt num_lfe_channel_elements ;
	CBitstreamInt num_assoc_data_elements ;
	CBitstreamInt num_valid_cc_elements ;
	CBitstreamInt mono_mixdown_present ;
	CBitstreamInt mono_mixdown_element_number ;
	CBitstreamInt stereo_mixdown_present ;
	CBitstreamInt stereo_mixdown_element_number ;
	CBitstreamInt matrix_mixdown_idx_present ;
	CBitstreamInt matrix_mixdown_idx ;
	CBitstreamInt pseudo_surround_enable ;
	HXBOOL front_element_is_cpe[MAX_FRONT_ELEMS] ;
	HXBOOL side_element_is_cpe[MAX_SIDE_ELEMS] ;
	HXBOOL back_element_is_cpe[MAX_BACK_ELEMS] ;
	HXBOOL cc_element_is_ind_sw[MAX_CC_ELEMS] ;
	int front_element_tag_select[MAX_FRONT_ELEMS] ;
	int side_element_tag_select[MAX_SIDE_ELEMS] ;
	int back_element_tag_select[MAX_BACK_ELEMS] ;
	int lfe_element_tag_select[MAX_LFE_ELEMS] ;
	int assoc_data_element_tag_select[MAX_ASSOC_ELEMS] ;
	int valid_cc_element_tag_select[MAX_CC_ELEMS] ;
	CBitstreamInt comment_field_bytes ;
	char *comment_field_data ;

	int num_front_channels ;
	int num_side_channels ;
	int num_back_channels ;

	static const int channelMapping[8][4] ;
} ;

class CGASpecificConfig
{
public:
	CGASpecificConfig() ;
	~CGASpecificConfig() ;

    HX_RESULT Read(struct BITSTREAM &bs,
					UINT32 samplingFrequency,
					UINT32 ChannelConfiguration,
					UINT32 AudioObjectType) ;

	UINT32 GetNChannels() const {return mPCE.GetNChannels();}
	UINT32 GetFrameLength() const ;

private:
	CBitstreamInt mFrameLengthFlag ;
	CBitstreamInt mDependsOnCoreCoder ;
	CBitstreamInt mCoreCoderDelay ;
	CBitstreamInt mExtensionFlag ;
	CPCE mPCE ;
	UINT32 mFrameLength ;
} ;

class CAudioSpecificConfig
{
public:
	CAudioSpecificConfig() ;
	~CAudioSpecificConfig() ;
	HX_RESULT Read(struct BITSTREAM &bs) ;

        UINT32 GetCoreConfigSize() const;
	UINT32 GetNChannels() const {return GASpecificConfig.GetNChannels();}
	UINT32 GetSampleRate() const ;
	UINT32 GetCoreSampleRate() const ;
        UINT32 GetCoreSamplingFrequencyIndex() const;
	UINT32 GetObjectType() const ;
	UINT32 GetFrameLength() const {return GASpecificConfig.GetFrameLength();}
	HXBOOL   GetIsSBR() const {return m_bSBR;}
        INT16  GetSBRPresentFlag() const {return m_nSBRPresentFlag;}
        INT16  GetPSPresentFlag() const {return m_nPSPresentFlag;}

private:
	CBitstreamInt AudioObjectType ;
	CBitstreamInt samplingFrequencyIndex ;
	CBitstreamInt samplingFrequency ;
	CBitstreamInt extensionSamplingFrequencyIndex ;
	CBitstreamInt extensionSamplingFrequency ;
	CBitstreamInt channelConfiguration ;
	CGASpecificConfig GASpecificConfig ;
	static const UINT32 aSampleRate[13] ;
	HXBOOL m_bSBR ;
        INT16 m_nSBRPresentFlag;
        INT16 m_nPSPresentFlag;
        UINT32 m_ulCoreConfigSize;
        UINT32 m_ulExtendedSize;

	enum AudioObjectType
	{
		AACMAIN = 1,
		AACLC   = 2,
		AACSSR  = 3,
		AACLTP  = 4,
		AACSBR  = 5,
		AACSCALABLE = 6,
		TWINVQ  = 7,
		AACPS   = 29
	} ;
} ;

#endif /* #ifndef GACONFIG_H */
