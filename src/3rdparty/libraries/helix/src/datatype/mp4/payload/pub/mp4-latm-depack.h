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

#ifndef MP4_LATM_DEPACK_H
#define MP4_LATM_DEPACK_H

#include "hxtypes.h"
#include "mp4a-mux-cfg.h"

typedef void (*OnFrameCB)(void* pUserData, ULONG32 ulTime, 
			  const UINT8* pData, ULONG32 ulSize,
			  HXBOOL bPreviousLoss);

class MP4LATMDepack
{
public:
    MP4LATMDepack();
    ~MP4LATMDepack();

    HXBOOL Init(ULONG32 ulProfileID,
	      ULONG32 ulObject,
	      ULONG32 ulBitrate,
	      HXBOOL bConfigPresent,
	      const UINT8* pStreamMuxConfig,
	      ULONG32 ulMuxConfigSize,
	      OnFrameCB pFrameCB,
	      void* pUserData);
    
    HXBOOL GetCodecConfig(const UINT8*& pConfig, ULONG32& ulConfigSize);

    HXBOOL SetTimeBase(ULONG32 ulSampleRate);
    
    HXBOOL SetFrameDuration(ULONG32 ulFrameDuration);

    HXBOOL Reset(); // Completely reset the depacketizer state

    HXBOOL Flush(); // Indicates end of stream

    HXBOOL OnPacket(ULONG32 ulTime, const UINT8* pData,
		  ULONG32 ulSize, HXBOOL bMarker);

    HXBOOL OnLoss(ULONG32 ulNumPackets); // called to indicate lost packets

protected:
    HXBOOL ProcessStagingBuffer(const UINT8* pBuffer,ULONG32 ulMuxConfigSize);

    HXBOOL HandleMuxConfig(Bitstream& bs);
    HXBOOL GetPayloadLengths(Bitstream& bs);
    HXBOOL GetPayloads(Bitstream& bs, ULONG32 ulTime);

private:
    ULONG32 m_ulSampleRate;
    ULONG32 m_ulFrameDuration;

    void* m_pUserData;
    OnFrameCB m_pCallback;

    HXBOOL m_bConfigPresent;
    MP4AMuxConfig m_muxConfig;

    HXBOOL m_bNeedTime;
    ULONG32 m_ulCurrentTime;

    UINT8* m_pStagingBuf;
    ULONG32 m_ulStagingBufSize;
    ULONG32 m_ulBytesStaged;
    
    ULONG32* m_pSlotLengths;

    UINT8* m_pPayloadBuf;
    ULONG32 m_ulPayloadBufSize;

    HXBOOL m_bHadLoss;
    HXBOOL m_bNeedMarker;
};

#endif // MP4_LATM_DEPACK_H
