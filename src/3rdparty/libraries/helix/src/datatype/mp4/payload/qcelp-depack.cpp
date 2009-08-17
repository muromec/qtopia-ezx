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
#include "qcelp-depack.h"
#include "hxccf.h"

#define QCELP_SAMPLE_RATE               8000
#define QCELP_SAMPLES_PER_FRAME         160
#define MAX_PACKETS_IN_INTERLEAVE_GROUP 60

QcelpDepack::QcelpDepack() :
    m_unPacketCount(0),
    m_ulFrameSize(0),
    m_ulFrameDuration(0),
    m_ulSampleRate(QCELP_SAMPLE_RATE),
    m_pUserData(0),
    m_pCallback(0),
    m_bHadLoss(FALSE),
    m_pDeinterleaveQueue(NULL),
    m_pClassFactory(NULL)
{}

QcelpDepack::~QcelpDepack()
{
    HX_RELEASE(m_pClassFactory);
    FlushDeinterleaveQueue();
    HX_VECTOR_DELETE(m_pDeinterleaveQueue);
}

HX_RESULT QcelpDepack::Init(OnFrameCB pFrameCB,
			    void* pUserData,
                            IHXCommonClassFactory* pClassFactory)
{
    HX_RESULT retVal = HXR_OK;

    if (!pFrameCB || !pUserData || !pClassFactory)
    {
        return HXR_FAIL;
    }
    m_pClassFactory = pClassFactory;
    m_pClassFactory->AddRef();
    m_pCallback     = pFrameCB;
    m_pUserData     = pUserData;
    m_pDeinterleaveQueue = new IUnknown*[MAX_PACKETS_IN_INTERLEAVE_GROUP];
    if (!m_pDeinterleaveQueue)
    {
        retVal = HXR_OUTOFMEMORY;
        HX_RELEASE(m_pClassFactory);
    }
    if (SUCCEEDED(retVal))
    {
        for (UINT8 i = 0; i < MAX_PACKETS_IN_INTERLEAVE_GROUP; i++)
        {
            m_pDeinterleaveQueue[i] = NULL;
        }
    }
    return retVal;
}

HX_RESULT QcelpDepack::GetCodecConfig(const UINT8*& pConfig, 
                                      ULONG32& ulConfigSize)
{
    HX_RESULT retVal = HXR_OK;
    ulConfigSize = 0;
    return retVal;
}

HX_RESULT QcelpDepack::SetSampleRate(ULONG32 ulSampleRate)
{
    m_ulSampleRate = ulSampleRate;
    return HXR_OK;
}

HX_RESULT QcelpDepack::SetFrameDuration(ULONG32 ulFrameDuration)
{
    m_ulFrameDuration = ulFrameDuration; 
    return HXR_OK;
}

HX_RESULT QcelpDepack::Reset() // Completely reset the depacketizer state
{
    HX_RESULT retVal = HXR_OK;
    m_bHadLoss = FALSE;
    retVal = FlushDeinterleaveQueue();
    return retVal;
}

HX_RESULT QcelpDepack::Flush() // Indicates end of stream
{
    IHXPacket* pPacket = NULL;
    IHXBuffer* pBuffer = NULL;
    ULONG32    ulCount = MAX_PACKETS_IN_INTERLEAVE_GROUP;
    ULONG32    i       = 0;

    if (m_pDeinterleaveQueue)
    {
        for (i = 0; i < ulCount; i++)
        {
            pPacket = (IHXPacket*)m_pDeinterleaveQueue[i];
            m_pDeinterleaveQueue[i] = NULL;
            if (pPacket)
            {
                pBuffer = pPacket->GetBuffer();
            }
            else
            {
                m_bHadLoss = TRUE;
            }
            if (pBuffer && m_pCallback)
            {
                m_pCallback(m_pUserData, pPacket->GetTime(), pBuffer->GetBuffer(), pBuffer->GetSize(), m_bHadLoss);
                m_bHadLoss = FALSE;
            }
            HX_RELEASE(pBuffer);
            HX_RELEASE(pPacket);
        }
    }
    m_bHadLoss = FALSE;
    m_unPacketCount = 0;
    return HXR_OK;
}

HX_RESULT QcelpDepack::OnPacket(ULONG32 ulTime, const UINT8* pData, 
			        ULONG32 ulSize, HXBOOL bMarker)
{
    HX_RESULT  retVal            = HXR_OK;
    UINT8      unInterleave      = 0;
    UINT8      unInterleaveIndex = 0;
    UINT32     ulPacketPosition  = 0;
    UINT32     i                 = 0;
    UINT32     ulPacketTime      = 0;
    UINT32     nPackets          = 0;
    IHXPacket* pPacket           = NULL;
    IHXBuffer* pBuffer           = NULL;
    UCHAR*     pSrc              = NULL;
    
    if (!pData || !ulSize)
    {
        return HXR_FAIL;
    }
    unInterleave      = (pData[0] >> 3) & 0x7;
    unInterleaveIndex = (pData[0] & 0x7);
    if (unInterleaveIndex > unInterleave)
    {
        retVal = HXR_FAIL;
    }

    if (!m_ulFrameSize && SUCCEEDED(retVal))
    {
        switch (pData[1])
        {
            case 0:
                m_ulFrameSize = 1;
                break;
            case 1:
                m_ulFrameSize = 4;
                break;
            case 2:
                m_ulFrameSize = 8;
                break;
            case 3:
                m_ulFrameSize = 17;
                break;
            case 4:
                m_ulFrameSize = 35;
                break;
            default:
                retVal = HXR_INVALID_INTERLEAVER;
                break;
        }
    }
    if (SUCCEEDED(retVal))
    {
        if (m_ulFrameSize * 10 < (ulSize - 1))
        {
            retVal = HXR_FAIL;
        }
    }
    if (SUCCEEDED(retVal) && unInterleave && m_unPacketCount == unInterleave + 1)
    {
        m_unPacketCount = 0;
        Flush();
    }
    if (SUCCEEDED(retVal))
    {
        m_unPacketCount++;
        nPackets = (ulSize - 1) / m_ulFrameSize;
        ulPacketTime = ulTime;
        for (i = 0, pSrc = (UCHAR *)pData + 1; i < nPackets; i++, pSrc += m_ulFrameSize)
        {
            retVal = m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
            if (SUCCEEDED(retVal))
            {
                retVal = pBuffer->Set(pSrc, m_ulFrameSize);
            }
            if (SUCCEEDED(retVal))
            {
                retVal = m_pClassFactory->CreateInstance(CLSID_IHXPacket, (void**)&pPacket);
            }
            if (SUCCEEDED(retVal))
            {
                retVal = pPacket->Set(pBuffer, ulPacketTime, 0, 0, 0);
                ulPacketTime += m_ulFrameDuration;
            }
            HX_RELEASE(pBuffer);
            if (SUCCEEDED(retVal))
            {
                ulPacketPosition = unInterleaveIndex + i * (unInterleave + 1);
                HX_RELEASE(m_pDeinterleaveQueue[ulPacketPosition]);
                m_pDeinterleaveQueue[ulPacketPosition] = (IUnknown*)pPacket;
            }
        }
    }
    if (!unInterleave)
    {
        Flush();
    }
    return retVal;
}

HX_RESULT QcelpDepack::OnLoss(ULONG32 ulNumPackets) // called to indicate lost 
                                                    // packets
{
    m_bHadLoss = TRUE;
    return HXR_OK;
}

HX_RESULT QcelpDepack::FlushDeinterleaveQueue()
{
    ULONG32    ulCount = MAX_PACKETS_IN_INTERLEAVE_GROUP;
    ULONG32    i       = 0;
    IHXPacket* pPacket = NULL;

    if (m_pDeinterleaveQueue)
    {
        for( i = 0; i < ulCount; i++)
        {
            pPacket = (IHXPacket*)m_pDeinterleaveQueue[i];
            m_pDeinterleaveQueue[i] = NULL;
            HX_RELEASE(pPacket);
        }
    }
    m_unPacketCount = 0;
    return HXR_OK;
}
