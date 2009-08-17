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

#ifndef _STRMINFO_H_
#define _STRMINFO_H_

enum PLAYER_REQUIRED {
    REAL_PLAY_UNKNOWN
  , REAL_PLAY_1_0
  , REAL_PLAY_2_0
  , REAL_PLAY_3_0
  , REAL_PLAY_4_0
  , REAL_PLAY_5_0
  , REAL_PLAY_G2
  , REAL_PLAY_7_0
  , REAL_PLAY_8_0
};
// the length of the buffer used in QueueInfoInHTML
#define MAXLINELEN 128

class CStreamInfo
{
public:
    enum StreamType {
	UNKNOWN,
	VIDEO,
	VIDEO_SURESTREAM,
	AUDIO,
	AUDIO_SURESTREAM,
	REALTEXT,
	REALPIX
    };

    StreamType m_type;
    char* m_pStreamName;
private:
    float m_fMaxBitRate;
    float m_fAvgBitRate;
    float m_fBufferTime;
    UINT32 m_ulDuration;
    UINT32 m_ulDur_min;
    UINT32 m_ulDur_sec;
    UINT32 m_ulDur_rem;
    HXBOOL m_bLive;
    CHXPtrArray m_typeSpecInfo;
    IHXValues* m_pFileHeader;
    IUnknown* m_pContext;
public:
    CStreamInfo(IUnknown* pContext);
    ~CStreamInfo();

    STDMETHOD(AttachStreams)(THIS_ CHXSimpleList* pStreamHeaders, 
			HXBOOL bLive = FALSE);
    STDMETHOD(AttachFileHeader)(IHXValues* pFileHeader);
    STDMETHOD_(UINT32, GetWidth)(THIS);
    STDMETHOD_(UINT32, GetHeight)(THIS);
    STDMETHOD(QueueTypeSpecificInfo)(THIS_ CBigByteGrowingQueue* pQ);
};


#endif // _STRMINFO_H_
