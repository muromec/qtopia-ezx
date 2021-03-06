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

#include "hlxclib/string.h"

#include "mp4a-mux-cfg.h"

#include "bitstream.h"

MP4AAudioSpec::MP4AAudioSpec() :
    m_pConfig(0),
    m_ulConfigSize(0),
    m_ulConfigBits(0)
{}

MP4AAudioSpec::MP4AAudioSpec(const MP4AAudioSpec& rhs) :
    m_pConfig(0),
    m_ulConfigSize(rhs.m_ulConfigSize)
{
    if (m_ulConfigSize)
    {
	m_pConfig = new UINT8[m_ulConfigSize];
	::memcpy(m_pConfig, rhs.m_pConfig, m_ulConfigSize);
    }
}

MP4AAudioSpec::~MP4AAudioSpec()
{
    delete [] m_pConfig;
    m_pConfig = 0;
}

MP4AAudioSpec& MP4AAudioSpec::operator=(const MP4AAudioSpec& rhs)
{
    if (&rhs != this)
    {
	delete [] m_pConfig;
	m_pConfig = 0;

	m_ulConfigSize = rhs.m_ulConfigSize;

	if (m_ulConfigSize)
	{
	    m_pConfig = new UINT8[m_ulConfigSize];
	    ::memcpy(m_pConfig, rhs.m_pConfig, m_ulConfigSize); /* Flawfinder: ignore */
	}
    }

    return *this;
}

HXBOOL MP4AAudioSpec::Unpack(Bitstream& bs, ULONG32 nBits)
{
    HXBOOL ret = FALSE;
    Bitstream bsTmp;
    bsTmp.SetBuffer(bs.GetBuffer());
    bsTmp.SetBufSize(bs.GetBufSize());

    bsTmp.GetBits(nBits);
    ret = AudioSpecificConfigRead(bsTmp,nBits);
    if(ret)
    {
        // Since AudioSpecificCofig size should be in 
        // bytes so add number of bits neded to make 
        // m_ulConfigBits exact multiple of 8
        if(m_ulConfigBits%8 > 0)
        {
            m_ulConfigBits += (8 - m_ulConfigBits%8);
        }

        m_ulConfigSize = m_ulConfigBits/8;
        delete [] m_pConfig;
        m_pConfig = new UINT8 [m_ulConfigSize];
        if(m_pConfig)
        {
            bs.GetBits(m_ulConfigBits, m_pConfig);
        }
        else
        {
            ret = FALSE;        
        }
    }
   
    return ret;
}

ULONG32 MP4AAudioSpec::GetAudioObjectType(Bitstream& bs)
{
    ULONG32 AudioObjectType = bs.GetBits(5);
    m_ulConfigBits += 5;

    if(AudioObjectType == 31)
    {
        AudioObjectType = 32 + bs.GetBits(6);
        m_ulConfigBits += 6;    
    }
    return AudioObjectType;
}

HXBOOL MP4AAudioSpec::AudioSpecificConfigRead(Bitstream& bs, ULONG32 nBits)
{
    ULONG32 samplingFrequency = 0;
    HXBOOL bSBR = FALSE;
    ULONG32 AudioObjectType = 0;
    ULONG32 ExtnAudioObjectType = 0;
    m_ulConfigBits = 0;

    AudioObjectType = GetAudioObjectType(bs);

    ULONG32 samplingFrequencyIndex = bs.GetBits(4);
    m_ulConfigBits += 4;

    if(samplingFrequencyIndex == 0xf)
    {
        // ESC, read sr from bitstream
        ULONG32 samplingFrequency = bs.GetBits(24);
        m_ulConfigBits += 24;
    }

    ULONG32 channelConfiguration = bs.GetBits(4);
    m_ulConfigBits += 4;

    bSBR = (AudioObjectType == 5) ;
    if (bSBR)
    {
        ExtnAudioObjectType = AudioObjectType;
        ULONG32 extensionSamplingFrequencyIndex = bs.GetBits(4);
        m_ulConfigBits += 4;

        if(extensionSamplingFrequencyIndex == 0xf)
        {
            // ESC, read sr from bitstream
            bs.GetBits(24);
            m_ulConfigBits += 24;
        }
        AudioObjectType = GetAudioObjectType(bs);
    }

    switch (AudioObjectType)
    {
        case 1: case 2: case 4:
            GASpecificConfigRead(bs,
            samplingFrequency,channelConfiguration,AudioObjectType) ;
            break ;

        default:
            // no other audio types supported 
            return FALSE ;
    }

    ULONG32 ulBitsLeft = bs.GetBufSize()*8 - m_ulConfigBits - nBits;
    
    if (!bSBR && ulBitsLeft >= 16)
    // if SBR info has not been read before, but there are more bytes to follow...
    {
        if (bs.GetBits(11) == 0x2b7)
        {
            m_ulConfigBits += 11;
            ExtnAudioObjectType = GetAudioObjectType(bs);
            if (ExtnAudioObjectType == 5)
            {
                bSBR = bs.GetBits(1);
                m_ulConfigBits += 1;
                if (bSBR)
                {
                    ULONG32 extensionSamplingFrequencyIndex = bs.GetBits(4);
                    m_ulConfigBits += 4;
                    if(extensionSamplingFrequencyIndex == 0xf)
                    {
                        // ESC, read sr from bitstream
                        bs.GetBits(24);		
                        m_ulConfigBits += 24;
                    }
                }
            }
        }
    }

    return TRUE ;
}

void MP4AAudioSpec::GASpecificConfigRead(Bitstream& bs,
                                UINT32 samplingFrequency,
                                UINT32 ChannelConfiguration,
                                UINT32 AudioObjectType)
{
    //Read FrameLengthFlag
    bs.GetBits(1);
    m_ulConfigBits += 1;
    // Read DependsOnCoreCoder
    if (bs.GetBits(1))
    {
        bs.GetBits(14);
        m_ulConfigBits += 15;
    }
    else
        m_ulConfigBits += 1;

    // Read ExtensionFlag
    ULONG32 extensionFlag = bs.GetBits(1);
    m_ulConfigBits += 1;

    if (ChannelConfiguration == 0)
    {
        PCERead(bs) ;
    }

    if((AudioObjectType == 6) || (AudioObjectType == 20))
    {
        // Read layerNr;
        bs.GetBits(3);
        m_ulConfigBits += 3;
    }
    if(extensionFlag)
    {
        if (AudioObjectType == 22)
        {
            // Read numOfSubFrame, layer_length
            bs.GetBits(16);
            m_ulConfigBits += 16;
        }
        if (AudioObjectType == 17 || AudioObjectType == 19 ||
            AudioObjectType == 20 || AudioObjectType == 23)
        {
            // TRead aacSectionDataResilienceFlag, aacScalefactorDataResilienceFlag
            // and aacSpectralDataResilienceFlag;
            bs.GetBits(3);
            m_ulConfigBits += 3;
        }
        // Read extensionFlag3
        bs.GetBits(1);
        m_ulConfigBits += 1;    
    }
}

void MP4AAudioSpec::PCERead(Bitstream& bs)
{
    UINT32 i ;

    // Read element_instance_tag, object_type, sampling_frequency_index
    bs.GetBits(10);
    m_ulConfigBits += 10;

    UINT32 num_front_channel_elements = bs.GetBits(4) ;
    UINT32 num_side_channel_elements = bs.GetBits(4) ;
    UINT32 num_back_channel_elements = bs.GetBits(4) ;
    UINT32 num_lfe_channel_elements = bs.GetBits(2) ;
    UINT32 num_assoc_data_elements = bs.GetBits(3) ;
    UINT32 num_valid_cc_elements = bs.GetBits(4) ;
    m_ulConfigBits += 21;

    // Read mono_mixdown_present
    m_ulConfigBits += 1;
    if (bs.GetBits(1))
    {
        // Read mono_mixdown_element_number
        bs.GetBits(4);
        m_ulConfigBits += 4;
    }

    // Read stereo_mixdown_present
    if (bs.GetBits(1))
    {
        bs.GetBits(4);
        m_ulConfigBits += 4;
    }
    m_ulConfigBits += 1;

    // Read matrix_mixdown_idx_present
    if (bs.GetBits(1))
    {
        // Read matrix_mixdown_idx, pseudo_surround_enable
        bs.GetBits(3);
        m_ulConfigBits += 3;
    }
    m_ulConfigBits += 1;

    for ( i = 0; i < num_front_channel_elements; i++) {
        bs.GetBits(5);
        m_ulConfigBits += 5;
    }

    for ( i = 0; i < num_side_channel_elements; i++) {
        bs.GetBits(5);
        m_ulConfigBits += 5;
    }

    for ( i = 0; i < num_back_channel_elements; i++) {
        bs.GetBits(5);
        m_ulConfigBits += 5;
    }


    for ( i = 0; i < num_lfe_channel_elements; i++)  {
        bs.GetBits(4);
        m_ulConfigBits += 4;
    }
    for ( i = 0; i < num_assoc_data_elements; i++)  {
        bs.GetBits(4);
        m_ulConfigBits += 4;
    }
    for ( i = 0; i < num_valid_cc_elements; i++)  {
       bs.GetBits(5);
       m_ulConfigBits += 5;
    }

    UINT32 comment_field_bytes = bs.GetBits(8);

    if (comment_field_bytes)
    {
        for (i = 0 ; i < comment_field_bytes ; i++)
        {
            bs.GetBits(8);
            m_ulConfigBits += 8;
        }
    }
}

MP4AStreamInfo::MP4AStreamInfo() :
    m_ulProgram(0),
    m_ulLayer(0),
    m_ulLengthType(0),
    m_ulBlockDelay(0),
    m_bFracDelayPresent(0),
    m_ulFracDelay(0),
    m_ulFrameLength(0),
    m_ulCELPIndex(0),
    m_ulHVXCIndex(0)
{}

MP4AMuxConfig::MP4AMuxConfig() :
    m_bAllSameTiming(FALSE),
    m_ulNumSubFrames(0),
    m_ulNumPrograms(0),
    m_pLayerCounts(0),
    m_ppStreamLookup(0),
    m_ulNumStreams(0),
    m_pStreamInfo(0)
{}

MP4AMuxConfig::~MP4AMuxConfig()
{
    Reset();
}

HXBOOL MP4AMuxConfig::Unpack(Bitstream& bs)
{
    HXBOOL failed = FALSE;
    ULONG32 nBits= 0;

    Reset();
#if 1
    // This updates StreamMuxConfig to ISO/IEC draft 14496-3:2001
    ULONG32 ulAudioMuxVersion = bs.GetBits(1); // audioMuxVersion
    nBits++;

    // This updates StreamMuxConfig to ISO/IEC 14496-3:2001/Cor.2:2004(E), Table 1.21
    ULONG32 ulAudioMuxVersionA = 0;
    if (ulAudioMuxVersion == 1)
    {
        ulAudioMuxVersionA = bs.GetBits(1); // audioMuxVersionA
        nBits++;
    }
    
    if (ulAudioMuxVersionA == 0)
    {
        if (ulAudioMuxVersion == 1)
        {
            // this increments nBits
            LatmGetValue(bs, nBits); // TaraBufferFullness
        }

        m_bAllSameTiming = (bs.GetBits(1) ? TRUE : FALSE);
        m_ulNumSubFrames = bs.GetBits(6) + 1; // 0-based (old version was not)
        m_ulNumPrograms  = bs.GetBits(4) + 1; // 0-based (old version was not)
        nBits += 11;
        // Allocate arrays
        m_pLayerCounts   = new ULONG32[m_ulNumPrograms];
        m_ppStreamLookup = new ULONG32*[m_ulNumPrograms];
        if (m_pLayerCounts && m_ppStreamLookup)
        {
            // Null out arrays
            ULONG32 ulProg = 0;
            for (ulProg = 0; ulProg < m_ulNumPrograms; ulProg++)
            {
	        m_pLayerCounts[ulProg]   = 0;
	        m_ppStreamLookup[ulProg] = 0;
            }
            // Parse programs
            for (ulProg = 0; !failed && ulProg < m_ulNumPrograms; ulProg++)
            {
                // Get the number of layers
                ULONG32 ulNumLayers = bs.GetBits(3) + 1; // 0-based (old version was not)
                nBits += 3;
                // Allocate stream lookup array
                MP4AAudioSpec as;
                m_pLayerCounts[ulProg]   = ulNumLayers;
                m_ppStreamLookup[ulProg] = new ULONG32[ulNumLayers];
                if (m_ppStreamLookup[ulProg])
                {
                    for (ULONG32 ulLay = 0; ulLay < ulNumLayers; ulLay++)
                    {
                        ULONG32 ulUseSameConfig = 0;
	                MP4AStreamInfo streamInfo;

	                m_ppStreamLookup[ulProg][ulLay] = m_ulNumStreams;

	                streamInfo.SetProgram(ulProg);
	                streamInfo.SetLayer(ulLay);

                        if ((ulProg != 0) || (ulLay != 0))
                        {
                            ulUseSameConfig = bs.GetBits(1);
                            nBits++;
                        }

	                if (!ulUseSameConfig)
	                {
                            ULONG32 ulAscLen = 0;
                            if (ulAudioMuxVersion == 1)
                            {
                                // this increments nBits
                                ulAscLen = LatmGetValue(bs, nBits);
                            }

                            if (!as.Unpack(bs, nBits)) //nBits bytes already read
                            {
                                failed = TRUE;
                                break;
                            }

                            ULONG32 ulAscBits = as.ConfigSize() * 8;
                            if (ulAscLen > ulAscBits)
                            {
                                // get fill bits
                                ulAscLen -= ulAscBits;
                                bs.GetBits(ulAscLen);
                                nBits += ulAscLen;
                            }
	                }

	                streamInfo.SetAudioSpec(as);

	                streamInfo.SetLengthType(bs.GetBits(3));

	                switch (streamInfo.GetLengthType())
                        {
                            case 0:
                                bs.GetBits(8); // Buffer fullness
                                if (!m_bAllSameTiming)
                                {
                                    // Get Core Frame offset
                                    // XXXMEH - We always hint such that m_bAllSameTiming == TRUE,
                                    // so for now we won't worry about this
                                }
		                break;
                            case 1:
                                streamInfo.SetFrameLength(bs.GetBits(9));
                                break;
                            case 3:
                            case 4:
                            case 5:
                                streamInfo.SetCELPIndex(bs.GetBits(6));
                                break;
                            case 6 :
                            case 7 :
                                streamInfo.SetHVXCIndex(bs.GetBits(1));
                                break;
                            default:
                                failed = TRUE;
                                break;
	                };
                        if(!failed)
                        {
                            AddStream(streamInfo);
                        }
                    }
                }
                else
                {
                    failed = TRUE;
                }
            }
            // Other data present
            if (bs.GetBits(1))
            {
                UINT8 otherDataLenTemp = 0;
                UINT32 otherDataLenBits = 0;
                UINT8 otherDataLenEsc  = 0;
                do {
                    otherDataLenBits *= 256;
                    otherDataLenEsc = (UINT8)bs.GetBits(1);
                    otherDataLenTemp = (UINT8)bs.GetBits(8);
                    otherDataLenBits += otherDataLenTemp;
                } while (otherDataLenEsc);
            }
            // Crc present
            if (bs.GetBits(1))
            {
                bs.GetBits(8);
            }

        }
        else
        {
            failed = TRUE;
        }
    }
#else
    if (bs.GetBits(1))
	m_bAllSameTiming = TRUE;

    m_ulNumSubFrames = bs.GetBits(3);
    
    m_ulNumPrograms = bs.GetBits(4);

    m_pLayerCounts = new ULONG32[m_ulNumPrograms];
    m_ppStreamLookup = new ULONG32*[m_ulNumPrograms];

    for (ULONG32 i = 0; i < m_ulNumPrograms ; i++)
    {
	m_pLayerCounts[i] = 0;
	m_ppStreamLookup[i] = 0;
    }

    for (ULONG32 prog = 0; !failed && (prog < m_ulNumPrograms) ; prog++)
    {
	ULONG32 ulNumLayers = bs.GetBits(3);
	
	MP4AAudioSpec as;

	m_pLayerCounts[prog] = ulNumLayers;
	m_ppStreamLookup[prog] = new ULONG32[ulNumLayers];

	for (ULONG32 lay = 0; lay < ulNumLayers; lay++)
	{
	    HXBOOL readAudioSpec = TRUE;

	    MP4AStreamInfo streamInfo;

	    m_ppStreamLookup[prog][lay] = m_ulNumStreams;

	    streamInfo.SetProgram(prog);
	    streamInfo.SetLayer(lay);

	    if (((prog != 0) || (lay != 0)) &&
		(bs.GetBits(1) != 0))
		readAudioSpec = FALSE;

	    if (readAudioSpec)
	    {
		if (!as.Unpack(bs))
		{
		    failed = TRUE;
		    break;
		}
	    }

	    streamInfo.SetAudioSpec(as);

	    streamInfo.SetLengthType(bs.GetBits(3));

	    switch (streamInfo.GetLengthType()) {
	    case 0 :
		streamInfo.SetBlockDelay(bs.GetBits(5));

		if (bs.GetBits(1))
		{
		    streamInfo.SetFracDelayPresent(TRUE);
		    streamInfo.SetFracDelay(bs.GetBits(8));
		}
		break;

	    case 1 :
		streamInfo.SetFrameLength(bs.GetBits(9));
		break;

	    case 3 :
	    case 4 :
	    case 5 :
		streamInfo.SetCELPIndex(bs.GetBits(6));
		break;

	    case 6 :
	    case 7 :
		streamInfo.SetHVXCIndex(bs.GetBits(1));
		break;
	    };

	    AddStream(streamInfo);
	}
    }
#endif

    return !failed;
}

ULONG32 MP4AMuxConfig::LatmGetValue(Bitstream&bs, ULONG32& nBits)
{
    ULONG32 ulValue = 0;
    ULONG32 ulBytesForValue = bs.GetBits(2);
    nBits += 2;

    for (ULONG32 i=0; i<=ulBytesForValue; i++)
    {
        ulValue << 8;
        ulValue += bs.GetBits(8);
        nBits += 8;
    }

    return ulValue;
}

void MP4AMuxConfig::Reset()
{
    delete [] m_pLayerCounts;
    m_pLayerCounts = 0;
    m_ulNumStreams = 0;

    if (m_ppStreamLookup)
    {
	for (ULONG32 i = 0; i < m_ulNumPrograms; i++)
	{
	    delete [] m_ppStreamLookup[i];
	    m_ppStreamLookup[i] = 0;
	}

	delete [] m_ppStreamLookup;
	m_ppStreamLookup = 0;
    }

    delete [] m_pStreamInfo;
    m_pStreamInfo = 0;
}

void MP4AMuxConfig::AddStream(const MP4AStreamInfo& info)
{
    MP4AStreamInfo* pTmp = new MP4AStreamInfo[m_ulNumStreams + 1];

    if (m_pStreamInfo)
    {
	for (ULONG32 i = 0; i < m_ulNumStreams; i++)
	{
	    pTmp[i] = m_pStreamInfo[i];
	}

	delete [] m_pStreamInfo;
    }
    
    m_pStreamInfo = pTmp;
    m_pStreamInfo[m_ulNumStreams] = info;
    m_ulNumStreams++;
}
