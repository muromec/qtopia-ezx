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

#include "hxtypes.h"

#include "hxcom.h"
#include "hxassert.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxslist.h"
#include "hxformt.h"
#include "safestring.h"

#include "sdptools.h"
#include "mp4desc.h"
#include "rtptypes.h"

#include "latmpacketizer.h"

#include "defslice.h"
#include "cklicense.h"

const char* MP4A_LICENSE_ERROR_STR = 
    "Packetizer: This Server is not licenced to use audio/MP4A-LATM Packetizer.";

LATMPacketizer::LATMPacketizer(UINT32 ulChannels, UINT32 ulSampleRate) :
    m_lRefCount(0),
    m_pClassFactory(NULL),
    m_pStreamHeader(NULL),
    m_bUsesRTPPackets(FALSE),
    m_bRTPPacketTested(FALSE),
    m_bFlushed(FALSE),
    m_ulChannels(ulChannels),
    m_ulSampleRate(ulSampleRate)
{
}

LATMPacketizer::~LATMPacketizer()
{
    while (!m_InputPackets.IsEmpty())
    {
        IHXPacket* pPacket = (IHXPacket*)m_InputPackets.RemoveHead();
        HX_RELEASE(pPacket);
    }

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);
}

STDMETHODIMP 
LATMPacketizer::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPayloadFormatObject))
    {
        AddRef();
        *ppvObj = (IHXPayloadFormatObject*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
LATMPacketizer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
LATMPacketizer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
LATMPacketizer::Init(IUnknown* pContext, BOOL bPacketize)
{
    if(!bPacketize || m_pClassFactory)
    {
        return HXR_UNEXPECTED;
    }

    HX_RESULT theErr = CheckLicense(pContext, 
	REGISTRY_3GPPPACKETIZER_ENABLED, 
	LICENSE_3GPPPACKETIZER_ENABLED,
	MP4A_LICENSE_ERROR_STR);

    if (HXR_OK != theErr)
    {
	return theErr;
    }

    if(SUCCEEDED(pContext->QueryInterface(IID_IHXCommonClassFactory, 
            (void**)&m_pClassFactory)))
    {
        return HXR_OK;
    }    
    return HXR_INVALID_PARAMETER;
}

STDMETHODIMP
LATMPacketizer::Close()
{
    while (!m_InputPackets.IsEmpty())
    {
        IHXPacket* pPacket = (IHXPacket*)m_InputPackets.RemoveHead();
        HX_RELEASE(pPacket);
    }

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);

    m_bFlushed = FALSE;
    m_bUsesRTPPackets = FALSE;
    m_bRTPPacketTested = FALSE;    

    return HXR_OK;
}

STDMETHODIMP
LATMPacketizer::Reset()
{
    while (!m_InputPackets.IsEmpty())
    {
        IHXPacket* pPacket = (IHXPacket*)m_InputPackets.RemoveHead();
        HX_RELEASE(pPacket);
    }

    m_bFlushed = FALSE;
    return HXR_OK;
}

STDMETHODIMP
LATMPacketizer::SetStreamHeader(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);

    HX_RELEASE(m_pStreamHeader);

    m_pStreamHeader = pHeader;
    m_pStreamHeader->AddRef();

    HX_RESULT res;
    res = AddHeaderMimeType();
    if(SUCCEEDED(res))
    {
        res = AddHeaderSDPData();
    }

    return res;
}

STDMETHODIMP
LATMPacketizer::GetStreamHeader(REF(IHXValues*) pHeader)
{
    if(m_pStreamHeader)
    {        
        pHeader = m_pStreamHeader;
        pHeader->AddRef();

        return HXR_OK;
    }

    return HXR_UNEXPECTED;
}

STDMETHODIMP
LATMPacketizer::SetPacket(IHXPacket* pPacket)
{
    HX_ASSERT(pPacket);

    pPacket->AddRef();
    m_InputPackets.AddTail((void*)pPacket);

    if (!m_bRTPPacketTested)
    {
        IHXRTPPacket* pRTPPacket = NULL;
        if(SUCCEEDED(pPacket->QueryInterface(IID_IHXRTPPacket, 
            (void**) &pRTPPacket)))
        {
            m_bUsesRTPPackets = TRUE;
            pRTPPacket->Release();
        }

        m_bRTPPacketTested = TRUE;
    }

    return HXR_OK;
}

STDMETHODIMP
LATMPacketizer::GetPacket(REF(IHXPacket*) pPacket)
{
    if(m_InputPackets.IsEmpty())
    {
        return m_bFlushed ? HXR_STREAM_DONE : HXR_INCOMPLETE;
    }

    HX_RESULT res = HXR_OK;

    // Get the next input packet
    IHXPacket* pInPacket = (IHXPacket*)m_InputPackets.RemoveHead();

    IHXBuffer* pSampleBuf = NULL;
    UCHAR* pSample;
    UINT32 ulSampleSize;
    UINT32 ulTime;
    UINT16 unStreamNumber;
    UINT8 unASMFlags;
    UINT16 unASMRuleNumber;

    IHXBuffer* pPayloadBuf = NULL;
    UCHAR* pPayload;
    UINT32 ulHeaderSize;

    pPacket = NULL;
    res = pInPacket->Get(pSampleBuf, ulTime, unStreamNumber, unASMFlags, 
            unASMRuleNumber);

    // Allocate a new buffer for the packet payload
    if(SUCCEEDED(res))
    {
        pSampleBuf->Get(pSample, ulSampleSize);
        res = m_pClassFactory->CreateInstance(IID_IHXBuffer, 
            (void**)&pPayloadBuf);
    }
    if(SUCCEEDED(res))
    {
        ulHeaderSize = SampleHeaderSize(ulSampleSize);
        res = pPayloadBuf->SetSize(ulHeaderSize + ulSampleSize);
    }

    // Add the header and audio frame to the buffer
    if(SUCCEEDED(res))
    {
        pPayload = pPayloadBuf->GetBuffer();
        CreateSampleHeader(pPayload, ulSampleSize);
        memcpy(pPayload + ulHeaderSize, pSample, ulSampleSize);
    }

    // Create a new packet object to use the new buffer
    if(m_bUsesRTPPackets)
    {
        res = m_pClassFactory->CreateInstance(IID_IHXRTPPacket, 
                (void**)&pPacket);
        if(SUCCEEDED(res))
        {
            res = ((IHXRTPPacket*)pPacket)->SetRTP(pPayloadBuf,
                    ulTime, ((IHXRTPPacket*)pInPacket)->GetRTPTime(), 
                    unStreamNumber, unASMFlags, unASMRuleNumber);
        }
    }
    else
    {
        res = m_pClassFactory->CreateInstance(IID_IHXPacket, 
                (void**)&pPacket);
        if(SUCCEEDED(res))
        {
            res = pPacket->Set(pPayloadBuf, ulTime, unStreamNumber, 
                    unASMFlags, unASMRuleNumber);
        }
    }

    HX_RELEASE(pInPacket);
    HX_RELEASE(pSampleBuf);
    HX_RELEASE(pPayloadBuf);

    if(FAILED(res))
    {
        HX_RELEASE(pPacket);
    }

    return res;
}

STDMETHODIMP
LATMPacketizer::Flush()
{
    m_bFlushed = TRUE;
    return HXR_OK;
}

void
LATMPacketizer::CreateSampleHeader(UINT8 *pHeader, UINT32 ulSampleSize)
{
    while (ulSampleSize > 0xFE)
    {
        *pHeader = 0xFF;
        pHeader++;
        ulSampleSize -= 0xFF;
    }

    *pHeader = (UINT8) ulSampleSize;
}

HX_RESULT 
LATMPacketizer::AddHeaderMimeType()
{
    HX_RESULT res;
    IHXBuffer* pMimeType = NULL;

    res = m_pClassFactory->CreateInstance(IID_IHXBuffer, (void**)(&pMimeType));
    if(SUCCEEDED(res))
    {
        res = pMimeType->Set((const UINT8*)LATM_MIME_TYPE, 
            sizeof(LATM_MIME_TYPE));

    }
    if(SUCCEEDED(res))
    {
        res = m_pStreamHeader->SetPropertyCString("MimeType", pMimeType);
    }

    HX_RELEASE(pMimeType);
    return res;
}

HX_RESULT
LATMPacketizer::AddHeaderSDPData()
{
    HX_RESULT res;
    IHXBuffer* pOpaque = NULL;
    IHXBuffer* pSDPData = NULL;
    IHXBuffer* pOldSDP = NULL;
    ES_Descriptor ESDesc;
    UCHAR* pConfig = NULL;
    UINT32 ulConfigSize = 0;
    char* pSDPBuf = NULL;
    UINT32 ulSDPBufSize = LATM_SDP_SIZE;
    UINT32 ulRTPPayloadType;
    const char* pOldSDPBuf = NULL;
    UINT32 ulOldSDPLen;

    // If we weren't passed the number of channels and sample rate,
    // see if they are in the header
    if (m_ulChannels == 0 && 
        FAILED(m_pStreamHeader->GetPropertyULONG32("Channels", 
        m_ulChannels)))
    {
        m_ulChannels = 0;
    }
    if (m_ulSampleRate == 0 && 
        FAILED(m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond", 
        m_ulSampleRate)))
    {
        m_ulSampleRate = 0;
    }

    // Get the decoder config info
    res = m_pStreamHeader->GetPropertyBuffer("OpaqueData", pOpaque);
    if (SUCCEEDED(res))
    {
        UCHAR* pESDescData;
        UINT32 ulESDescSize;
        pOpaque->Get(pESDescData, ulESDescSize);
        res = ESDesc.Unpack(pESDescData, ulESDescSize);
        pOpaque->Release();
    }
    if(SUCCEEDED(res))
    {
        DecoderConfigDescriptor* pDCDesc = ESDesc.m_pDecConfigDescr;
        if (pDCDesc && pDCDesc->m_pDecSpecificInfo)
        {
            pConfig = pDCDesc->m_pDecSpecificInfo->m_pData;
            ulConfigSize = pDCDesc->m_pDecSpecificInfo->m_ulLength;
        }
        if(!pConfig)
        {
            res = HXR_INVALID_PARAMETER;
        }
    }

    if(SUCCEEDED(res))
    {
        // Look for any existing SDP data
        // I don't think this should ever really happen
        if(SUCCEEDED(m_pStreamHeader->GetPropertyCString("SDPData", pOldSDP)))
        {
            pOldSDPBuf = (const char*)pOldSDP->GetBuffer();
            ulOldSDPLen = pOldSDP->GetSize();
            ulSDPBufSize += ulOldSDPLen;
        }
    }

    // Create SDP data Buffer
    if(SUCCEEDED(res))
    {
        res = m_pClassFactory->CreateInstance(IID_IHXBuffer, 
                (void**)(&pSDPData));
        if(SUCCEEDED(res))
        {
            res = pSDPData->SetSize(ulSDPBufSize);
            pSDPBuf = (char*)pSDPData->GetBuffer();
        }
    }

    if(SUCCEEDED(res))
    {
        // Copy the old SDP if there is one, removing any existing a=fmtp
        if(pOldSDPBuf)
        {
            UINT32 ulCopySize = RemoveSDPToken("a=fmtp", 6, pOldSDPBuf,
                ulOldSDPLen, pSDPBuf, ulSDPBufSize);
            pSDPBuf += ulCopySize;
            ulSDPBufSize -= ulCopySize;
        }

        // Get the RTP Payload type
        if(FAILED(m_pStreamHeader->GetPropertyULONG32(
            "RTPPayloadType", ulRTPPayloadType)))
        {
            ulRTPPayloadType = RTP_PAYLOAD_RTSP;
        }

        // Write the a=fmtp data to the sdp
        int nSize = WriteFMTP(pSDPBuf, ulSDPBufSize, pConfig, 
                              ulConfigSize, ulRTPPayloadType, 
                              m_ulChannels, m_ulSampleRate);

        // Set the buffer size to the actual size
        if (nSize > 0)
        {
            res = pSDPData->SetSize((UINT32)nSize + 1);
        }
        else
        {
            res = HXR_FAIL;
        }
    }

    // Add the SDPData to the stream header
    if(SUCCEEDED(res))
    {
        res = m_pStreamHeader->SetPropertyCString("SDPData", pSDPData);
    }

    HX_RELEASE(pOldSDP);
    HX_RELEASE(pSDPData);

    return res;
}

int
LATMPacketizer::WriteFMTP(char* pBuf, UINT32 ulSize, UINT8* pConfig, 
                          UINT32 ulConfigSize, UINT32 ulRTPPayloadType,
                          UINT32 ulChannels, UINT32 ulSampleRate)
{
    if(ulConfigSize == 0)
    {
        return -1;
    }

    UINT8 uAudioObjectType = ((pConfig[0] >> 3) & 0x1F);

    // 3GPP requires profile-level-id 15 for AAC-LC (object 2)
    // and AAC-LTP (object 4) streams fitting level 15 criteria
    // so check if it fits and set the level
    UINT32 ulProfileLevelId = 
            (uAudioObjectType == 2 || uAudioObjectType == 4) &&
            (ulChannels == 1 || ulChannels == 2) && 
            (ulSampleRate > 0 && ulSampleRate <= 48000) 
            ? 15 : 0;

    char* pWriter = pBuf;
    int nWritten = SafeSprintf(pWriter, ulSize, 
        "a=fmtp:%u object=%u; cpresent=0;",
        ulRTPPayloadType, uAudioObjectType);

    // Write the profile-level-id if known and start the config
    if (nWritten >= 0)
    {
        pWriter += nWritten;
        ulSize -= nWritten;

        if (ulProfileLevelId)
        {
            nWritten = SafeSprintf(pWriter, ulSize,
                " profile-level-id=%u; config=",
                ulProfileLevelId);
        }
        else
        {
            nWritten = SafeSprintf(pWriter, ulSize, " config=");
        }
    }

    // calculate and write the config string
    if (nWritten >= 0)
    {
        pWriter += nWritten;
        ulSize -= nWritten;

        nWritten = FormatStreamMuxConfig(pWriter, ulSize, pConfig,
            ulConfigSize, 1);
    }

    // terminate the line
    if (nWritten >= 0)
    {
        pWriter += nWritten;
        ulSize -= nWritten;

        if (ulSize > 2)
        {
            *(pWriter++) = '\r'; *(pWriter++) = '\n'; *pWriter = '\0';
            return pWriter - pBuf;
        }
    }

    return -1;
}

int 
LATMPacketizer::FormatStreamMuxConfig(char* pConfigString, 
                                      UINT32 ulSize,
                                      UINT8* pConfig, 
                                      UINT32 ulConfigSize,
                                      UINT32 ulFramesPerPacket)
{
    int nSize = (ulConfigSize + 4)*2;
    if(ulSize < (UINT32)nSize + 1)
    {
        return -1;
    }

    UINT8 pStreamMuxConfig[1024];
    UINT32 idx = 0;

    // StreamMuxConfig per ISO/IEC draft 14496-3:2001
    pStreamMuxConfig[idx] = 0;
    // audioMuxVersion(1)
    pStreamMuxConfig[idx] |= (1 << 6); // allStreamsSametimeFraming(1)
    pStreamMuxConfig[idx] |= ((UINT8) ((ulFramesPerPacket-1) & 0x0000003F)); // numSubFrames(6)
    idx++;
    pStreamMuxConfig[idx] = 0;
    // numProgram(4) 0-based (i.e. value is numProgram + 1)
    // numLayer(3) 0-based (i.e. value is numLayer + 1)
    // BitBufferImportBytes() copies an integral number
    // of bytes which may be bit-shifted over the destination
    // buffer. The current shift is 7 bits (numProgram(4) + numLayer(3))
    BitBufferImportBytes(&(pStreamMuxConfig[idx]),    // AudioSpecificConfig
                         7,
                         pConfig,
                         ulConfigSize);
    idx += ulConfigSize;
    // frameLengthType (1 of 3)
    idx++;
    // frameLengthType (2 of 3)
    // bufferFullness (value = 0xFF, 8 bits, 6 of 8)
    pStreamMuxConfig[idx] = 0x3F;
    idx++;
    // bufferFullness (value = 0xFF, 8 bits, 2 of 8)
    pStreamMuxConfig[idx] = (3 << 6);
    // otherDataPresent(1)
    // crcCheckPresent(1)
    // 4 bits left over in this last byte

    BinaryToHexString(pStreamMuxConfig, idx + 1, pConfigString);

    return nSize;
}

void 
LATMPacketizer::BitBufferImportBytes(UINT8* pTarget,
                                     UINT32 ulTargetBitOffset,
                                     UINT8* pSource,
                                     UINT32 ulSourceSize)
{
    UINT32 ulByteOffset = ulTargetBitOffset / 8;
    UINT8 ulBitOffset = (UINT8) (ulTargetBitOffset % 8);
    UINT32 idx;

    pTarget += ulByteOffset;

    if (ulBitOffset == 0)
    {
        memcpy(pTarget, pSource, ulSourceSize);
    }
    else
    {
        for (idx = 0; idx < ulSourceSize; idx++)
        {
            pTarget[idx] |= (UINT8) ((pSource[idx] >> ulBitOffset) & 
                                        ((1 << (8 - ulBitOffset)) - 1));
            pTarget[idx + 1] = (UINT8) (pSource[idx] << (8 - ulBitOffset));
        }
    }
}

