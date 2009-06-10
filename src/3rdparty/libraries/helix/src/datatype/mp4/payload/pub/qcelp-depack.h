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

#ifndef QCELP_DEPACK_H
#define QCELP_DEPACK_H

#include "hxtypes.h"
#include "hxresult.h"
#include "hxslist.h"
#include "ihxpckts.h"
#include "tsconvrt.h"

typedef void (*OnFrameCB)(void* pUserData, ULONG32 ulTime, 
			  const UINT8* pData, ULONG32 ulSize,
			  HXBOOL bPreviousLoss);
typedef _INTERFACE IHXCommonClassFactory IHXCommonClassFactory;

class QcelpDepack
{
public:
    QcelpDepack();
    ~QcelpDepack();

    HX_RESULT Init(OnFrameCB pFrameCB, void* pUserData, IHXCommonClassFactory* pClassFactory);
    
    HX_RESULT GetCodecConfig(const UINT8*& pConfig, ULONG32& ulConfigSize);

    HX_RESULT SetSampleRate(ULONG32 ulSampleRate);
    
    HX_RESULT SetFrameDuration(ULONG32 ulFrameDuration);

    HX_RESULT Reset(); // Completely reset the depacketizer state

    HX_RESULT Flush(); // Indicates end of stream

    HX_RESULT OnPacket(ULONG32 ulTime, const UINT8* pData,
		       ULONG32 ulSize, HXBOOL bMarker);

    HX_RESULT OnLoss(ULONG32 ulNumPackets); // called to indicate lost packets

private:
    HX_RESULT FlushDeinterleaveQueue();

    UINT8                  m_unPacketCount;
    ULONG32                m_ulFrameSize;
    ULONG32                m_ulFrameDuration;
    ULONG32                m_ulSampleRate;
    void*                  m_pUserData;
    OnFrameCB              m_pCallback;
    HXBOOL                   m_bHadLoss;
    IUnknown**             m_pDeinterleaveQueue;
    IHXCommonClassFactory* m_pClassFactory;
};

#endif // QCELP_DEPACK_H
