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

#include "tsconvrt.h"
#include "sdptools.h"
#include "rtptypes.h"

#include "amr_flavor.h"
#include "amr_frame_info.h"
#include "amr_frame_hdr.h"
#include "amrpacketizer.h"

#include "cklicense.h"
#include "defslice.h"

const char* LICENSE_ERROR_STR_NB = 
    "Packetizer: This Server is not licenced to use audio/AMR Packetizer.";
const char* LICENSE_ERROR_STR_WB = 
    "Packetizer: This Server is not licenced to use audio/AMR-WB Packetizer.";

AMRPacketizer::AMRPacketizer(AMRFlavor flavor) :
    m_lRefCount(0),
    m_pContext(NULL),
    m_pClassFactory(NULL),
    m_pStreamHeader(NULL),
    m_bUsesRTPPackets(FALSE),
    m_bRTPPacketTested(FALSE),
    m_bFlushed(FALSE),
    m_ulMaxPayloadSize(AMR_DFLT_MAX_PAYLOAD),
    m_ulSamplesPerSecond(1000),
    m_ulChannels(1),
    m_AMRFlavor(flavor),
    m_ulFrameBlocksPerPacket(0),
    m_bInterleave(FALSE),
    m_uInterleaveLength(0)
{
}

AMRPacketizer::~AMRPacketizer()
{
    while (!m_InputPackets.IsEmpty())
    {
        SampleInfo* pInfo = (SampleInfo*)m_InputPackets.RemoveHead();
        delete pInfo;
    }

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);
}

STDMETHODIMP 
AMRPacketizer::QueryInterface (THIS_ REFIID riid, void** ppvObj)
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
AMRPacketizer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
AMRPacketizer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
AMRPacketizer::Init(IUnknown* pContext, HXBOOL bPacketize)
{
    if(!bPacketize || m_pClassFactory)
    {
        return HXR_UNEXPECTED;
    }

    if(!pContext)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RESULT ret = CheckLicense(pContext, 
	REGISTRY_3GPPPACKETIZER_ENABLED, 
	LICENSE_3GPPPACKETIZER_ENABLED,
	(m_AMRFlavor == WideBand) ? LICENSE_ERROR_STR_WB : LICENSE_ERROR_STR_NB);
    if (HXR_OK != ret)
    {
	return ret;
    }

    m_pContext = pContext;
    pContext->AddRef();
    if(SUCCEEDED(pContext->QueryInterface(IID_IHXCommonClassFactory, 
            (void**)&m_pClassFactory)))
    {
        return HXR_OK;
    }

    return HXR_INVALID_PARAMETER;
}


STDMETHODIMP
AMRPacketizer::Close()
{
    while (!m_InputPackets.IsEmpty())
    {
        SampleInfo* pInfo = (SampleInfo*)m_InputPackets.RemoveHead();
        delete pInfo;
    }

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);

    m_bFlushed = FALSE;
    m_bUsesRTPPackets = FALSE;
    m_bRTPPacketTested = FALSE;    
    m_ulSamplesPerSecond = 1000;
    m_ulChannels = 1;
    m_ulFrameBlocksPerPacket = 0;
    m_bInterleave = FALSE;
    m_uInterleaveLength = 0;

    return HXR_OK;
}

STDMETHODIMP
AMRPacketizer::Reset()
{
    while (!m_InputPackets.IsEmpty())
    {
        SampleInfo* pInfo = (SampleInfo*)m_InputPackets.RemoveHead();
        delete pInfo;
    }

    m_bFlushed = FALSE;
    return HXR_OK;
}

STDMETHODIMP
AMRPacketizer::SetStreamHeader(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);

    HX_RELEASE(m_pStreamHeader);

    if(FAILED(pHeader->GetPropertyULONG32(
        "Channels", m_ulChannels)))
    {
        m_ulChannels = 1;
    }
    if(FAILED(pHeader->GetPropertyULONG32(
        "SamplesPerSecond", m_ulSamplesPerSecond)))
    {
        m_ulSamplesPerSecond = 1000;
    }
    m_TSConverter.SetBase(1000, m_ulSamplesPerSecond);

    m_pStreamHeader = pHeader;
    m_pStreamHeader->AddRef();

    HandleMaxPacketSize();

    HX_RESULT res = ParseSampleEntry();
    if(SUCCEEDED(res))
    {
        res = AddHeaderMimeType();
    }
    if(SUCCEEDED(res))
    {
        res = AddHeaderSDPData();
    }

    return res;
}

STDMETHODIMP
AMRPacketizer::GetStreamHeader(REF(IHXValues*) pHeader)
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
AMRPacketizer::SetPacket(IHXPacket* pPacket)
{
    HX_ASSERT(pPacket);

    pPacket->AddRef();
    SampleInfo* pSample = new SampleInfo(pPacket, m_AMRFlavor);
    m_InputPackets.AddTail((void*)pSample);

    if(!m_bRTPPacketTested)
    {
        IHXRTPPacket* pRTPPacket = NULL;
        if(SUCCEEDED(pPacket->QueryInterface(IID_IHXRTPPacket, 
            (void**) &pRTPPacket)))
        {
            m_bUsesRTPPackets = TRUE;
            pRTPPacket->Release();
        }

        m_TSConverter.SetOffset(pRTPPacket->GetRTPTime());
        m_bRTPPacketTested = TRUE;
    }

    return HXR_OK;
}

STDMETHODIMP
AMRPacketizer::GetPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT res = HXR_OK;

    PacketInfo pktInfo;

    IHXBuffer* pPayloadBuf = NULL;
    UCHAR* pPayload; 
    UINT32 ulTotalFrames = 0;
    UINT32 ulPayloadSize = 0;

    pPacket = NULL;

    // Check if we have enough data for a full packet
    res = FindFrameBlocks(ulTotalFrames, ulPayloadSize);
    if(res != HXR_OK)
    {
        // Need more data or stream done
        return res;
    }

    // Allocate a new buffer for the packet payload.
    if(SUCCEEDED(res))
    {
        res = m_pClassFactory->CreateInstance(IID_IHXBuffer, 
            (void**)&pPayloadBuf);
    }
    if(SUCCEEDED(res))
    {
        res = pPayloadBuf->SetSize(ulPayloadSize + PayloadHeaderSize());
    }

    if(SUCCEEDED(res))
    {
        // Write the payload
        pPayload = pPayloadBuf->GetBuffer();
        UINT32 ulBufSize = WritePayload(ulTotalFrames, pPayload, &pktInfo);
        pPayloadBuf->SetSize(ulBufSize);
        
        // Create a new packet object to use the new buffer
        if(m_bUsesRTPPackets)
        {
            res = m_pClassFactory->CreateInstance(IID_IHXRTPPacket, 
                    (void**)&pPacket);
            if(SUCCEEDED(res))
            {
                res = ((IHXRTPPacket*)pPacket)->SetRTP(pPayloadBuf,
                        pktInfo.ulTime, pktInfo.ulRTPTime, pktInfo.unStreamNumber,
                        pktInfo.unASMFlags, pktInfo.unASMRuleNumber);
            }
        }
        else
        {
            res = m_pClassFactory->CreateInstance(IID_IHXPacket, 
                    (void**)&pPacket);
            if(SUCCEEDED(res))
            {
                res = pPacket->Set(pPayloadBuf, pktInfo.ulTime, 
                        pktInfo.unStreamNumber, pktInfo.unASMFlags, 
                        pktInfo.unASMRuleNumber);
            }
        }
    }

    HX_RELEASE(pPayloadBuf);

    if(FAILED(res))
    {
        HX_RELEASE(pPacket);
    }

    return res;
}

STDMETHODIMP
AMRPacketizer::Flush()
{
    m_bFlushed = TRUE;
    return HXR_OK;
}

void
AMRPacketizer::HandleMaxPacketSize(void)
{
    UINT32 ulMaxPktSize = 0;
    HX_RESULT res = GetConfigDefaultMaxPacketSize(m_pContext,
	m_AMRFlavor == WideBand ? AMR_WB_CFG_MAX_PYLD : AMR_NB_CFG_MAX_PYLD,
	ulMaxPktSize);

    if (SUCCEEDED(res))
    {
	m_ulMaxPayloadSize = ulMaxPktSize;
    }
    else if (SUCCEEDED(
        m_pStreamHeader->GetPropertyULONG32("MaxPacketSize", ulMaxPktSize)))
    {
	m_ulMaxPayloadSize = ulMaxPktSize;
    }
    else
    {
	m_ulMaxPayloadSize = AMR_DFLT_MAX_PAYLOAD;
    }
}

HX_RESULT 
AMRPacketizer::AddHeaderMimeType()
{
    HX_RESULT res;
    IHXBuffer* pMimeType = NULL;

    const char* szMimeType = 
        m_AMRFlavor == WideBand ? AMR_WB_MIME_TYPE : AMR_NB_MIME_TYPE;

    res = m_pClassFactory->CreateInstance(IID_IHXBuffer, (void**)(&pMimeType));
    if(SUCCEEDED(res))
    {
        res = pMimeType->Set((const UCHAR*)szMimeType, strlen(szMimeType) + 1);
    }
    if(SUCCEEDED(res))
    {
        res = m_pStreamHeader->SetPropertyCString("MimeType", pMimeType);
    }

    HX_RELEASE(pMimeType);
    return res;
}

HX_RESULT
AMRPacketizer::AddHeaderSDPData()
{
    HX_RESULT res;
    IHXBuffer* pSDPData = NULL;
    IHXBuffer* pOldSDP = NULL;
    char* pSDPBuf = NULL;
    UINT32 ulSDPBufSize = AMR_SDP_SIZE;
    UINT32 ulRTPPayloadType;
    const char* pOldSDPBuf = NULL;
    UINT32 ulOldSDPLen;

    // Look for any existing SDP data
    // I don't think this should ever really happen
    if(SUCCEEDED(m_pStreamHeader->GetPropertyCString("SDPData", pOldSDP)))
    {
        pOldSDPBuf = (const char*)pOldSDP->GetBuffer();
        ulOldSDPLen = pOldSDP->GetSize();
        ulSDPBufSize += ulOldSDPLen;
    }

    // Create SDP data Buffer
    res = m_pClassFactory->CreateInstance(IID_IHXBuffer, 
            (void**)(&pSDPData));
    if(SUCCEEDED(res))
    {
        res = pSDPData->SetSize(ulSDPBufSize);
        pSDPBuf = (char*)pSDPData->GetBuffer();
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
        int nSize = WriteFMTP(pSDPBuf, ulSDPBufSize, ulRTPPayloadType);

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

HX_RESULT
AMRPacketizer::FindFrameBlocks(UINT32& ulFrameCount, UINT32& ulPayloadSize)
{
    if(m_InputPackets.IsEmpty())
    {
        return m_bFlushed ? HXR_STREAM_DONE : HXR_INCOMPLETE;
    }

    ulFrameCount = 0;
    ulPayloadSize = 0;
    UINT32 ulNumFrames = m_ulFrameBlocksPerPacket * m_ulChannels;

    LISTPOSITION pos = m_InputPackets.GetHeadPosition();
    while(ulFrameCount < ulNumFrames && pos)
    {
        // Get the next input packet
        SampleInfo* pSampleInfo = (SampleInfo*) m_InputPackets.GetNext(pos);
        HX_ASSERT(pSampleInfo);

        ulFrameCount += pSampleInfo->m_ulFrames;
        ulPayloadSize += pSampleInfo->m_ulSize;
    }

    // Need more data
    if (ulFrameCount < ulNumFrames && !m_bFlushed)
    {
        return HXR_INCOMPLETE;
    }
    else if(ulFrameCount > ulNumFrames)
    {
        ulFrameCount = ulNumFrames;
    }

    return HXR_OK;
}

UINT32
AMRPacketizer::WritePayload(UINT32 ulFrames, UCHAR* pPayload, 
                            PacketInfo* pPktInfo)
{
    UINT32 ulNumFrames = 0;
    SampleInfo* pSampleInfo;
    CAMRFrameHdr frameHeader(m_AMRFlavor);
    UINT32 ulFrameBytes;
    UINT32 ulCopySize;
    UCHAR* pStart = pPayload;

    // Write the payload header
    WritePayloadHeader(pPayload);

    // TOC is one byte per frame. write frames after TOC space
    UCHAR* pFrameBuf = pPayload + ulFrames;

    while(ulNumFrames < ulFrames)
    {
        pSampleInfo = (SampleInfo*)m_InputPackets.GetHead();

        if(ulNumFrames == 0)
        {
            // If this is the first sample in the outgoing packet, get the 
            // packet info
            GetPacketInfo(pSampleInfo, pPktInfo);
        }

        // Write TOC entries and audio frames from this sample
        while(ulNumFrames < ulFrames && pSampleInfo->m_ulFrames && 
            pSampleInfo->m_ulSize)
        {
            // Write the frame TOC entry
            // Set F bit on all except last frame to indicate more follow
            pPayload[ulNumFrames] = (ulNumFrames == ulFrames - 1) ? 
                pSampleInfo->m_pBuffer[0] & 0x7F : 
                pSampleInfo->m_pBuffer[0] | 0x80;

            frameHeader.Unpack(pSampleInfo->m_pBuffer);
            pSampleInfo->m_ulSize--;

            // Copy the frame to the payload
            ulFrameBytes = frameHeader.DataBytes();

            ulCopySize = pSampleInfo->m_ulSize >= ulFrameBytes ? 
                         ulFrameBytes : pSampleInfo->m_ulSize;

            memcpy(pFrameBuf, pSampleInfo->m_pBuffer, ulCopySize);
            pSampleInfo->m_pBuffer += ulCopySize;
            pSampleInfo->m_ulSize -= ulCopySize;
            pSampleInfo->m_ulFrames--;

            pFrameBuf += ulCopySize;
            ulNumFrames++;
        }

        // If we've sent all the frames in this sample, discard it
        if(pSampleInfo->m_ulFrames == 0)
        {
            m_InputPackets.RemoveHead();
            delete pSampleInfo;
        }
    }

    return (UINT32)(pFrameBuf - pStart);
}

HX_RESULT
AMRPacketizer::ParseSampleEntry()
{
    // Get the decoder config info
    IHXBuffer* pOpaque;
    UCHAR* pData;
    UINT32 ulSize;

    HX_RESULT res = m_pStreamHeader->GetPropertyBuffer("OpaqueData", pOpaque);
    if(SUCCEEDED(res))
    {
        pOpaque->Get(pData, ulSize);

        if(ulSize < 17)
        {
            res = HXR_INVALID_PARAMETER;
        }
    }

    if(SUCCEEDED(res))
    {
        // mode set is all we care about
        UINT16 uModeSet = ((pData[13] << 4) & 0xf0) | pData[14];
        UINT32 ulFrameBits;
        UINT32 ulMaxFrameBits = 0;

        if(uModeSet != 0)
        {
            UINT32 ulMode;
            UINT16 mask;
            for(mask = 0x8000, ulMode = 15; mask > 0; mask >>= 1, ulMode--)
            { 
                if(uModeSet & mask)
                {
                    ulFrameBits = CAMRFrameInfo::FrameBits(m_AMRFlavor, 
                                    ulMode);
                    if(ulFrameBits > ulMaxFrameBits)
                    {
                        ulMaxFrameBits = ulFrameBits;
                    }
                }
            }
        }

        if(ulMaxFrameBits == 0)
        {
            ulMaxFrameBits = CAMRFrameInfo::MaxFrameBits(m_AMRFlavor);
        }

        // MaxPayload - HeaderSize bytes available
        // MaxFrameBytes + TOC entry size (1) bytes used per frame
        // NumChannels frames per frame block
        // min value 1, max value AMR_MAX_PAYLOAD_FRAMES
        m_ulFrameBlocksPerPacket = HX_MAX(1, HX_MIN(AMR_MAX_PAYLOAD_FRAMES, 
            (m_ulMaxPayloadSize - PayloadHeaderSize()) / 
            ((((ulMaxFrameBits + 7) >> 3) + 1) * m_ulChannels)));
    }

    HX_RELEASE(pOpaque);
    return res;
}

AMRPacketizer::SampleInfo::SampleInfo(IHXPacket* pPacket, AMRFlavor flavor)
    : m_pPacket(pPacket),
      m_AMRFlavor(flavor)
{
    m_pBufObj = m_pPacket->GetBuffer();
    m_pBuffer = m_pBufObj->GetBuffer();
    m_ulTotalSize = m_pBufObj->GetSize();
    m_ulSize = m_ulTotalSize;
    m_ulTotalFrames = GetNumFrames(m_pBuffer, m_ulSize);
    m_ulFrames = m_ulTotalFrames;
}

AMRPacketizer::SampleInfo::~SampleInfo()
{
    HX_RELEASE(m_pBufObj);
    HX_RELEASE(m_pPacket);
}

UINT32
AMRPacketizer::SampleInfo::GetNumFrames(UINT8* pSample, UINT32 ulSampleSize)
{
    UINT32 ulFrames = 0;
    UINT32 ulFrameSize;
    CAMRFrameHdr frameHeader(m_AMRFlavor);

    while(ulSampleSize)
    {
        frameHeader.Unpack(pSample);
        ulSampleSize--;

        ulFrameSize = frameHeader.DataBytes();
        if(ulSampleSize >= ulFrameSize)
        {
            ulSampleSize -= ulFrameSize;
            pSample += ulFrameSize;
            ulFrames++;
        }
    }

    return ulFrames;
}


