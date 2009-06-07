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

#ifndef MP4A_MUX_CFG_H
#define MP4A_MUX_CFG_H

#include "hxtypes.h"
#include "bitstream.h"

class MP4AAudioSpec
{
public:
    MP4AAudioSpec();
    MP4AAudioSpec(const MP4AAudioSpec& rhs);

    ~MP4AAudioSpec();

    MP4AAudioSpec& operator=(const MP4AAudioSpec& rhs);

    HXBOOL Unpack(Bitstream& bs, ULONG32 nBits);
    ULONG32 GetAudioObjectType(Bitstream& bs);
    HXBOOL AudioSpecificConfigRead(Bitstream& bs, ULONG32 nBits);
    void GASpecificConfigRead(Bitstream& bs,
                                UINT32 samplingFrequency,
                                UINT32 ChannelConfiguration,
                                UINT32 AudioObjectType);
    void PCERead(Bitstream& bs);

    const UINT8* Config() const;
    ULONG32 ConfigSize() const;

private:
    UINT8* m_pConfig;
    ULONG32 m_ulConfigSize;
    ULONG32 m_ulConfigBits;
};

class MP4AStreamInfo
{
public:
    MP4AStreamInfo();
    
    void SetProgram(ULONG32 program);
    ULONG32 GetProgram() const;

    void SetLayer(ULONG32 layer);
    ULONG32 GetLayer() const;

    void SetAudioSpec(const MP4AAudioSpec& audioSpec);
    const MP4AAudioSpec& GetAudioSpec() const;
    
    void SetLengthType(ULONG32 type);
    ULONG32 GetLengthType() const;

    void SetBlockDelay(ULONG32 blockType);
    ULONG32 GetBlockDelay() const;

    void SetFracDelayPresent(HXBOOL present);
    HXBOOL GetFracDelayPresent() const;

    void SetFracDelay(ULONG32 fracDelay);
    ULONG32 GetFracDelay() const;

    void SetFrameLength(ULONG32 frameLength);
    ULONG32 GetFrameLength() const;

    void SetCELPIndex(ULONG32 celpIndex);
    ULONG32 GetCELPIndex() const;

    void SetHVXCIndex(ULONG32 hvxcIndex);
    ULONG32 GetHVXCIndex() const;

private:
    ULONG32 m_ulProgram;
    ULONG32 m_ulLayer;
    MP4AAudioSpec m_audioSpec;
    ULONG32 m_ulLengthType;
    ULONG32 m_ulBlockDelay;
    HXBOOL m_bFracDelayPresent;
    ULONG32 m_ulFracDelay;
    ULONG32 m_ulFrameLength;
    ULONG32 m_ulCELPIndex;
    ULONG32 m_ulHVXCIndex;
};

class MP4AMuxConfig
{
public:
    MP4AMuxConfig();
    ~MP4AMuxConfig();

    HXBOOL Unpack(Bitstream& bs);

    HXBOOL AllSameTiming() const;
    ULONG32 NumSubFrames() const;
    ULONG32 NumPrograms() const;
    ULONG32 NumLayers(ULONG32 program) const;
    ULONG32 GetStreamID(ULONG32 program, ULONG32 layer);

    ULONG32 NumStreams() const;
    const MP4AStreamInfo& GetStream(ULONG32 streamID) const;
   
protected :
    void Reset();
    void AddStream(const MP4AStreamInfo& info);   
    ULONG32 LatmGetValue(Bitstream& bs, ULONG32& nBits);

private:
    HXBOOL m_bAllSameTiming;
    ULONG32 m_ulNumSubFrames;

    ULONG32 m_ulNumPrograms;
    
    ULONG32* m_pLayerCounts;
    ULONG32** m_ppStreamLookup;

    ULONG32 m_ulNumStreams;
    MP4AStreamInfo* m_pStreamInfo;
};

inline
const UINT8* MP4AAudioSpec::Config() const
{
    return m_pConfig;
}

inline
ULONG32 MP4AAudioSpec::ConfigSize() const
{
    return m_ulConfigSize;
}

inline
void MP4AStreamInfo::SetProgram(ULONG32 program)
{
    m_ulProgram = program;
}

inline
ULONG32 MP4AStreamInfo::GetProgram() const
{
    return m_ulProgram;
}

inline
void MP4AStreamInfo::SetLayer(ULONG32 layer)
{
    m_ulLayer = layer;
}

inline
ULONG32 MP4AStreamInfo::GetLayer() const
{
    return m_ulLayer;
}

inline
void MP4AStreamInfo::SetAudioSpec(const MP4AAudioSpec& audioSpec)
{
    m_audioSpec = audioSpec;
}

inline
const MP4AAudioSpec& MP4AStreamInfo::GetAudioSpec() const
{
    return m_audioSpec;
}

inline
void MP4AStreamInfo::SetLengthType(ULONG32 type)
{
    m_ulLengthType = type;
}

inline
ULONG32 MP4AStreamInfo::GetLengthType() const
{
    return m_ulLengthType;
}

inline
void MP4AStreamInfo::SetBlockDelay(ULONG32 blockType)
{
    m_ulBlockDelay = blockType;
}

inline
ULONG32 MP4AStreamInfo::GetBlockDelay() const
{
    return m_ulBlockDelay;
}

inline
void MP4AStreamInfo::SetFracDelayPresent(HXBOOL present)
{
    m_bFracDelayPresent = present;
}

inline
HXBOOL MP4AStreamInfo::GetFracDelayPresent() const
{
    return m_bFracDelayPresent;
}

inline
void MP4AStreamInfo::SetFracDelay(ULONG32 fracDelay)
{
    m_ulFracDelay = fracDelay;
}

inline
ULONG32 MP4AStreamInfo::GetFracDelay() const
{
    return m_ulFracDelay;
}

inline
void MP4AStreamInfo::SetFrameLength(ULONG32 frameLength)
{
    m_ulFrameLength = frameLength;
}

inline
ULONG32 MP4AStreamInfo::GetFrameLength() const
{
    return m_ulFrameLength;
}

inline
void MP4AStreamInfo::SetCELPIndex(ULONG32 celpIndex)
{
    m_ulCELPIndex = celpIndex;
}

inline
ULONG32 MP4AStreamInfo::GetCELPIndex() const
{
    return m_ulCELPIndex;
}

inline
void MP4AStreamInfo::SetHVXCIndex(ULONG32 hvxcIndex)
{
    m_ulHVXCIndex = hvxcIndex;
}

inline
ULONG32 MP4AStreamInfo::GetHVXCIndex() const
{
    return m_ulHVXCIndex;
}

inline
HXBOOL MP4AMuxConfig::AllSameTiming() const
{
    return m_bAllSameTiming;
}

inline
ULONG32 MP4AMuxConfig::NumSubFrames() const
{
    return m_ulNumSubFrames;
}

inline
ULONG32 MP4AMuxConfig::NumPrograms() const
{
    return m_ulNumPrograms;
}

inline
ULONG32 MP4AMuxConfig::NumLayers(ULONG32 program) const
{
    return m_pLayerCounts[program];
}

inline
ULONG32 MP4AMuxConfig::GetStreamID(ULONG32 program, ULONG32 layer)
{
    return m_ppStreamLookup[program][layer];
}

inline
ULONG32 MP4AMuxConfig::NumStreams() const
{
    return m_ulNumStreams;
}

inline
const MP4AStreamInfo& MP4AMuxConfig::GetStream(ULONG32 streamID) const
{
    return m_pStreamInfo[streamID];
}

#endif // MP4A_MUX_CFG_H
