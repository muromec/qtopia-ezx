/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h264packetizer.cpp,v 1.2 2006/01/11 07:39:45 pankajgupta Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ******************** END LICENSE BLOCK *************************/ 

/***********************************************************************
- To invoke this packetizer "QTCONFIG_H264_PACKETIZER" must be defined
- This packetizer will generte by default Single NAL RTP packets
- For non-interleave mode "QTCONFIG_NON_INTERLEAVE_MODE" must be defined
- For Interleave mode "QTCONFIG_INTERLEAVE_MODE" must be defined
    - Define "QTCONFIG_STAP_B_PACKET" to generate STAP-B type packets
    - Define "QTCONFIG_MTAP_16_PACKET" to generate MTAP-16 type packets
    - Define "QTCONFIG_MTAP_24_PACKET" to generate MTAP-24 type packets
************************************************************************/

#include "h264packetizer.h"
#include "hxassert.h"
#include "avcconfig.h"
#include "safestring.h"
#include "sdptools.h"
#include "rtsputil.h"

#define STAP_A			24
#define STAP_B			25
#define MTAP_16			26
#define MTAP_24			27
#define FU_A			28
#define FU_B			29
#define ASM_RULE_0      0
#define ASM_RULE_1      1

#define INTERLEAVE_MODE         2
#define NON_INTERLEAVE_MODE     1
#define RTP_PAYLOAD_INTERLEAVE  100

// These values are temporary and can be changed as per requirement
#define MAX_DON_DIFF            20
#define INTERLEAVE_DEPTH        20
#define DEINTERLEAVE_BUF_REQUEST 4 
#define INIT_BUF_TIME           20 
#define MAX_PACKET_SIZE         1400
#define MAX_SDP_DATA_SIZE       1024

#define NALU_SIZE_FIELD_LENGTH    4
#define NAL_SIZE_ATOM_LENGTH      2 
#define DOND_ATOM_LENGTH          1
#define TS_OFFSET_16_ATOM_LENGTH  2
#define TS_OFFSET_24_ATOM_LENGTH  3
#define MP4V_H264_PAYLOAD_MIME_TYPE	    "video/H264"

H264Packetizer::H264Packetizer()
    : m_lRefCount(0)
    , m_pCCF(NULL)
    , m_pHeader(NULL)
    , m_ulByteCount(0)
    , m_ulDON(0)
	, m_interleaveCount(0)
    , m_bFragmented(FALSE)
	, m_bPendingFlush(FALSE)
	, m_bInterleavingDone(FALSE)
{}

H264Packetizer::~H264Packetizer()
{
    Close();
}

HX_RESULT 
H264Packetizer::CreateInstance(REF(IHXPayloadFormatObject*) pPyld)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    pPyld = new H264Packetizer();

    if (pPyld)
    {
        HX_ADDREF(pPyld);
        res = HXR_OK;
    }

    return res;
}

/*
 *      IUnknown methods
 */
STDMETHODIMP H264Packetizer::QueryInterface(THIS_
                                            REFIID riid,
                                            void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), this },
        { GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
    };

    if (ppvObj)
    {
        *ppvObj = NULL;
        return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    }
    return retVal;
}

STDMETHODIMP_(ULONG32) H264Packetizer::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) H264Packetizer::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/*
 *      IHXPayloadFormatObject methods
 */
STDMETHODIMP H264Packetizer::Init(THIS_
                                  IUnknown* pContext,
                                  HXBOOL bPacketize)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (bPacketize && pContext)
    {
        res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                       (void**)&m_pCCF);
    }

    return res;
}

STDMETHODIMP H264Packetizer::Close(THIS)
{
    Reset();
	
    HX_RELEASE(m_pCCF);

    return HXR_OK;
}

STDMETHODIMP H264Packetizer::Reset(THIS)
{

    // Release all input packets we have stored
    FlushQueues();
    m_bFragmented = FALSE;
	m_bPendingFlush = FALSE; 
	m_bInterleavingDone = FALSE;

    return HXR_OK;
}

STDMETHODIMP
H264Packetizer::SetStreamHeader(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);

    HX_RELEASE(m_pHeader);

    m_pHeader = pHeader;
    HX_ADDREF(m_pHeader);

    HX_RESULT res;
    res = AddHeaderMimeType();
    if(SUCCEEDED(res))
    {
        res = AddHeaderSDPData();
    }

    return res;
}

HX_RESULT 
H264Packetizer::AddHeaderMimeType()
{
    HX_RESULT res;
    IHXBuffer* pMimeType = NULL;

    res = m_pCCF->CreateInstance(IID_IHXBuffer, (void**)(&pMimeType));
    if(SUCCEEDED(res))
    {
        res = pMimeType->Set((const UINT8*)MP4V_H264_PAYLOAD_MIME_TYPE, 
            sizeof(MP4V_H264_PAYLOAD_MIME_TYPE));

    }
    if(SUCCEEDED(res))
    {
        res = m_pHeader->SetPropertyCString("MimeType", pMimeType);
    }

    HX_RELEASE(pMimeType);
    return res;
}


HX_RESULT
H264Packetizer::AddHeaderSDPData()
{
    HX_RESULT res = HXR_FAIL;
    IHXBuffer* pOpaque = NULL;
    IHXBuffer* pSDPData = NULL;
    AVCConfigurationBox avcConfigBox;
    IHXBuffer* pAVCConfigurationBoxBuffer = NULL;
    UCHAR* pConfig = NULL;
    UINT32 ulConfigSize = 0;
    char* pSDPBuf = NULL;
    UINT16 usDataCount = 0;
    UINT32 nWritten = 0;
    UINT32 ulIndex;
    UINT32 ulSDPBufLength = MAX_SDP_DATA_SIZE;
    UINT16 usParameterSetLength = 0;
    UINT8 nParameterSets = 0;
    UINT32 ulPacketizationMode = 0;

    // Get the decoder config info
    res = m_pHeader->GetPropertyBuffer("OpaqueData", pAVCConfigurationBoxBuffer);

    if (SUCCEEDED(res))
    {
        res = HXR_INVALID_PARAMETER;
        if (pAVCConfigurationBoxBuffer)
        {
            res = HXR_OK;
        }
    }

    if (SUCCEEDED(res))
    {
        UINT8* pData = pAVCConfigurationBoxBuffer->GetBuffer();
        UINT32 ulSize = pAVCConfigurationBoxBuffer->GetSize();
        res = avcConfigBox.Unpack(pData, ulSize);
        HX_RELEASE(pAVCConfigurationBoxBuffer);
    }

    if (SUCCEEDED(res))
    {
        pConfig = avcConfigBox.m_pAVCDecoderConfigurationRecord->m_pData;
	    ulConfigSize = avcConfigBox.m_pAVCDecoderConfigurationRecord->m_ulLength;
        if(!pConfig)
        {
            res = HXR_INVALID_PARAMETER;
        }
    }

    // Create SDP data Buffer
    if(SUCCEEDED(res))
    {
        res = m_pCCF->CreateInstance(IID_IHXBuffer, 
                (void**)(&pSDPData));
        if(SUCCEEDED(res))
        {
            res = pSDPData->SetSize(ulSDPBufLength);
            pSDPBuf = (char *)pSDPData->GetBuffer();
        }
    }

    // Set the RTP Payload type
    m_pHeader->SetPropertyULONG32("RTPPayloadType", RTP_PAYLOAD_INTERLEAVE);

    nWritten = SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength, 
                            "a=fmtp:%u ", RTP_PAYLOAD_INTERLEAVE);
    ulSDPBufLength -= nWritten;


    // Write the profile-level-id 

    nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,
                            " profile-level-id=");

    ulSDPBufLength -= nWritten;
    usDataCount++;
    BinaryToHexString((pConfig + usDataCount), 3, (pSDPBuf + nWritten));
    nWritten +=6;
    ulSDPBufLength -= nWritten;
    usDataCount +=4;

    // Write packetization mode

#ifdef QTCONFIG_INTERLEAVE_MODE
    ulPacketizationMode = INTERLEAVE_MODE;
#endif //QTCONFIG_INTERLEAVE_MODE

#ifdef QTCONFIG_NON_INTERLEAVE_MODE
    ulPacketizationMode = NON_INTERLEAVE_MODE;
#endif //QTCONFIG_INTERLEAVE_MODE

    nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,
                "; packetization-mode=%u; sprop-parameter-sets=",ulPacketizationMode);
    ulSDPBufLength -= nWritten;


    // Write Sequence Parameter set
    nParameterSets = pConfig[usDataCount++] & 0x1F;

    for (ulIndex=0; ulIndex < nParameterSets;  ulIndex++) 
    {
         usParameterSetLength = (pConfig[usDataCount]<<8) + pConfig[usDataCount+1];
         usDataCount += 2;

         nWritten += BinTo64((pConfig + usDataCount), 
                                        usParameterSetLength, 
                                        pSDPBuf + nWritten);
         nWritten--;
         ulSDPBufLength -= nWritten;
         if(ulIndex != nParameterSets-1)
         {
            nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,",");
            ulSDPBufLength -= nWritten;
         }

         usDataCount += usParameterSetLength;
    }

    // Write Picture Parameter set
    nParameterSets = pConfig[usDataCount++];

    for (ulIndex=0; ulIndex < nParameterSets;  ulIndex++)
    {
         usParameterSetLength = (pConfig[usDataCount]<<8) + pConfig[usDataCount+1];
         usDataCount += 2;

         nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,",");
         ulSDPBufLength -= nWritten;

         nWritten += BinTo64((pConfig + usDataCount), 
                                        usParameterSetLength, 
                                        pSDPBuf + nWritten);
         nWritten--;
         ulSDPBufLength -= nWritten;
         if(ulIndex != nParameterSets-1)
         {
            nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,",");
            ulSDPBufLength -= nWritten;
         }

         usDataCount += usParameterSetLength;
    }

    if(ulPacketizationMode == INTERLEAVE_MODE)
    {
        nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,
                                "; sprop-interleaving-depth=%u", INTERLEAVE_DEPTH); 
        ulSDPBufLength -= nWritten;
        nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,
                                "; sprop-dein-buf-req=%u", DEINTERLEAVE_BUF_REQUEST );
        ulSDPBufLength -= nWritten;
        nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,
                                "; sprop-init-buf-time=%u", INIT_BUF_TIME );
        ulSDPBufLength -= nWritten;
        nWritten += SafeSprintf(pSDPBuf + nWritten, ulSDPBufLength,
                                "; sprop-max-don-diff=%u", MAX_DON_DIFF);
        ulSDPBufLength -= nWritten;
    }

    // terminate the line
    *(pSDPBuf + nWritten++) = '\r'; 
    *(pSDPBuf+ nWritten++) = '\n'; 
    *(pSDPBuf + nWritten++) = '\0';


    // Set the buffer size to the actual size
    if (nWritten > 0)
    {
        res = pSDPData->SetSize((UINT32)nWritten + 1);
    }
    else
    {
        res = HXR_FAIL;
    }

    // Add the SDPData to the stream header
    if(SUCCEEDED(res))
    {
        res = m_pHeader->SetPropertyCString("SDPData", pSDPData);
    }

    HX_RELEASE(pSDPData);

    return res;
}


STDMETHODIMP H264Packetizer::GetStreamHeader(THIS_ REF(IHXValues*) pHeader)
{
    pHeader = m_pHeader;

    return (m_pHeader) ? HXR_OK : HXR_FAILED;
}



H264Packetizer::CNALPacket* H264Packetizer::AllocateNALPacket(IHXPacket* pPacket)
{
    CNALPacket* pNALPacket = NULL;

    pNALPacket = new CNALPacket;
    if (pNALPacket)
    {
        pNALPacket->packetValid = TRUE;
        pNALPacket->pBuffer = pPacket->GetBuffer();
        HX_ADDREF(pNALPacket->pBuffer);
        pNALPacket->ulTimeStamp = pPacket->GetTime();
        pNALPacket->usStream = pPacket->GetStreamNumber();
        pNALPacket->uASMFlags = pPacket->GetASMFlags();
    }
    return pNALPacket;
}


STDMETHODIMP H264Packetizer::SetPacket(THIS_ IHXPacket* pPacket)
{
	HX_RESULT res = HXR_FAIL;
    CNALPacket* pNALPacket = NULL;
    UINT32 ulNALULen = 0;
    UINT8* pData = NULL;
	IHXBuffer* pBuf = pPacket->GetBuffer();
	UINT32 ulPacketLen = pBuf->GetSize();
    HXBOOL bPacketAdded = FALSE;

    if(!pBuf)
    {
        return HXR_FAIL;
    }

    pData = pBuf->GetBuffer();

    if(!pData)
    {
        HX_RELEASE(pBuf);
        return HXR_FAIL;
    }

    // Count number of bytes available to make packet
    m_ulByteCount += ulPacketLen;

    // Extract all NALUs from packet
    while(ulPacketLen > NALU_SIZE_FIELD_LENGTH)
    {
        // Determine NAL size
        ulNALULen = (pData[0] << 24) + (pData[1] << 16) + (pData[2] << 8) + pData[3];
        pData += NALU_SIZE_FIELD_LENGTH;
        ulPacketLen -= NALU_SIZE_FIELD_LENGTH;
        HX_ASSERT(ulPacketLen >= ulNALULen);

        if(ulPacketLen >= ulNALULen)
        {
            // Create a NAL Packet and add in to NALPacket List
            pNALPacket = AllocateNALPacket(pPacket);
            if(pNALPacket)
            {
                res = HXR_OK;
                pNALPacket->ulSize = ulNALULen;
                pNALPacket->usDON = m_ulDON++;
                pNALPacket->pData = pData;
                pData = pData + ulNALULen;
                if( ulPacketLen > ulNALULen)
                {
                    ulPacketLen -= ulNALULen;
                }
                else
                {
                    ulPacketLen = 0;
                }
                m_NALPacketList.AddTail(pNALPacket);
                bPacketAdded = TRUE;
            }
        }    
    }
    if(bPacketAdded)
    {
        pNALPacket = (CNALPacket *)m_NALPacketList.GetTail();
        pNALPacket->usASMRule = ASM_RULE_1;
    }
    HX_RELEASE(pBuf);

#if defined QTCONFIG_NON_INTERLEAVE_MODE || defined QTCONFIG_INTERLEAVE_MODE
    // Create packet once we have sufficient data
    if(m_ulByteCount > MAX_PACKET_SIZE )
        ConstructAggregationPacket();
#else
        CreateSingleNALPacket();
#endif

    return res;   
}


STDMETHODIMP H264Packetizer::GetPacket(THIS_ REF(IHXPacket*) pPacket)
{
     HX_RESULT res = HXR_OK;

    if (m_bPendingFlush && m_FinishedPktList.IsEmpty())
    {
        /* Handle the pending flush*/
#if defined QTCONFIG_NON_INTERLEAVE_MODE || defined QTCONFIG_INTERLEAVE_MODE
        res = ConstructAggregationPacket();
#else
        res = CreateSingleNALPacket();
#endif
        
    }

    if (!m_FinishedPktList.IsEmpty() && m_bInterleavingDone)
    {
        /* Transfering ownership here */
        pPacket = (IHXPacket*)m_FinishedPktList.RemoveHead();

        // TMP to be removed
        IHXBuffer* pBuffer = pPacket->GetBuffer();
        UINT8* pData = pBuffer->GetBuffer();
        UINT32 ulBuffSize = pBuffer->GetSize();
    }
    else if (HXR_OK == res)
    {
        /* We don't have a finished
         * packet here. We need more
         * packets.
         */
        res = HXR_INCOMPLETE;
    }

    return res;
}

STDMETHODIMP H264Packetizer::Flush(THIS)
{
    HX_RESULT res = HXR_OK;

    if (!m_NALPacketList.IsEmpty())
    {
        m_bPendingFlush = TRUE;
    }

    return res;
}

HX_RESULT H264Packetizer::CreateSingleNALPacket()
{
    HX_RESULT res = HXR_FAIL;
    CNALPacket* pNALPacket = NULL;
    IHXPacket* pNewPkt = NULL;
    IHXBuffer* pBuf = NULL;
    UINT32 ulPacketLength = 0;
    UINT8* pCurPos = NULL;

    while (!m_NALPacketList.IsEmpty())
    {
        pNALPacket = (CNALPacket*) m_NALPacketList.RemoveHead();
        ulPacketLength = pNALPacket->ulSize;
        if( ulPacketLength <= MAX_PACKET_SIZE)
        {

            if (m_pCCF)
            {
                if ((HXR_OK == m_pCCF->CreateInstance(CLSID_IHXPacket,
                                                  (void**)&pNewPkt)) &&
                    (HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, 
                                                      (void**)&pBuf)) &&
                    (HXR_OK == pBuf->SetSize(ulPacketLength)))
                {
                    memcpy(pBuf->GetBuffer(), pNALPacket->pData, pNALPacket->ulSize);
                    res = pNewPkt->Set(pBuf, pNALPacket->ulTimeStamp, 
                                        pNALPacket->usStream, 
                                        pNALPacket->uASMFlags,
                                        pNALPacket->usASMRule);
                    HX_DELETE(pNALPacket);

                    if (HXR_OK == res)
                    {
                        m_bPendingFlush = FALSE;
                        m_FinishedPktList.AddTail(pNewPkt);
                    } 
                }
                else
                {
                    if(pNewPkt)
                    {
                        HX_DELETE(pNewPkt);
                    }
                    if(pBuf)
                    {
                        HX_DELETE(pBuf);
                    }
                }
            }
        }
        else
        {
            res = ConstructFragmentedPacket(pNALPacket);
            m_bPendingFlush = FALSE;        
        }
    }

    m_bInterleavingDone = TRUE;
    return res;
}

HX_RESULT H264Packetizer::ConstructAggregationPacket()
{
	HX_RESULT res = HXR_FAIL;
    CHXSimpleList tmpList;
    CNALPacket* pNALPacket = NULL;
    CNALPacket* pNextNALPacket = NULL;
    LISTPOSITION lPos;
	UINT32 ulPacketLength = 0 ;
    IHXPacket* pPacket = NULL;
    UINT32 ulMinRtpTime = 0;
    UINT32 ulNALOverheadSize = NAL_SIZE_ATOM_LENGTH;
    UINT32 ulPacketCount = 0;

#ifdef QTCONFIG_MTAP_16_PACKET
    ulNALOverheadSize += DOND_ATOM_LENGTH + TS_OFFSET_16_ATOM_LENGTH;
#endif //QTCONFIG_MTAP_16_PACKET

#ifdef QTCONFIG_MTAP_24_PACKET
    ulNALOverheadSize += DOND_ATOM_LENGTH + TS_OFFSET_24_ATOM_LENGTH;
#endif //QTCONFIG_MTAP_24_PACKET
    
    if(!m_NALPacketList.IsEmpty())
    {
        pNALPacket = (CNALPacket*) m_NALPacketList.RemoveHead();
        ulPacketLength = pNALPacket->ulSize + ulNALOverheadSize;
        ulMinRtpTime = pNALPacket->ulTimeStamp;
        if( ulPacketLength <= MAX_PACKET_SIZE)
        {
            tmpList.AddTail(pNALPacket);

            while(!m_NALPacketList.IsEmpty())
	        {
                lPos = m_NALPacketList.GetHeadPosition();;
                pNextNALPacket = (CNALPacket*)m_NALPacketList.GetNext(lPos);
                if((!pNextNALPacket) || 
#if !defined QTCONFIG_MTAP_16_PACKET  && !defined QTCONFIG_MTAP_24_PACKET
                    // check for STAP packets
                   (pNALPacket->ulTimeStamp != pNextNALPacket->ulTimeStamp) ||
#endif
                   (ulPacketLength + pNextNALPacket->ulSize > MAX_PACKET_SIZE) ||
                   (ulPacketCount >= MAX_DON_DIFF - 1))
                {
                    break;
                }
                
                ulPacketLength += pNextNALPacket->ulSize + ulNALOverheadSize;
                if( ulMinRtpTime > pNextNALPacket->ulTimeStamp)
                {
                    ulMinRtpTime = pNextNALPacket->ulTimeStamp;
                
                }
                tmpList.AddTail(pNextNALPacket);
                ulPacketCount++;
                m_NALPacketList.RemoveHead();
            }
            pPacket = CreateFinishedPkt(&tmpList, ulPacketLength, ulMinRtpTime);
            m_ulByteCount -= ulPacketLength;
            if(pPacket)
            {
                res = HXR_OK;
#ifdef QTCONFIG_INTERLEAVE_MODE
                InterleaveRTPPacket(pPacket);
#else
                m_bInterleavingDone = TRUE;
                m_FinishedPktList.AddTail(pPacket);
                m_bPendingFlush = FALSE;
#endif
            }
        }
        else
        {
            res = ConstructFragmentedPacket(pNALPacket);
            m_bFragmented = TRUE;
            m_bPendingFlush = FALSE;
        }
    }
	return res;
}

void H264Packetizer::InterleaveRTPPacket(IHXPacket* pPacket)
{
    LISTPOSITION lPos;

    if(m_interleaveCount == 2 && !m_bFragmented)
    {
        lPos = m_FinishedPktList.GetTailPosition();
        m_FinishedPktList.InsertBefore(lPos, pPacket);
    }
    else
    {
        m_bFragmented = FALSE;
        m_FinishedPktList.AddTail(pPacket);                
    }
    
    if(m_FinishedPktList.GetCount() == INTERLEAVE_DEPTH)
        m_bInterleavingDone = TRUE;   
    if(m_interleaveCount == INTERLEAVE_DEPTH)
        m_interleaveCount = 0;
    else
        m_interleaveCount++;

}


IHXPacket* H264Packetizer::CreateFinishedPkt(CHXSimpleList* tmpList, 
                                             UINT32 ulPacketLength, 
                                             UINT32 ulMinRtpTime)
{
	HX_RESULT res = HXR_UNEXPECTED;
    CNALPacket* pNALPacket = NULL;
    UINT16 usSize = 0;
    HXBOOL bFirstPacket = TRUE;
    UINT32 ulTimeStamp;
    UINT16 usStream;
    UINT8 uASMFlags;
    UINT16 usASMRule;
    UINT16 usDON;
    UINT8 tmp1[2];
    IHXPacket* pNewPkt = NULL;
    IHXBuffer* pBuf = NULL;
    UINT32 ulBufSize = 0;
    UINT8 type = 0;
    UINT8 uDOND = 0;
    UINT32 usTimeDiff;


#ifdef QTCONFIG_INTERLEAVE_MODE

#ifdef QTCONFIG_STAP_B_PACKET
    ulBufSize = ulPacketLength + 3;
    type = STAP_B;
#endif //QTCONFIG_STAP_B_PACKET

#ifdef QTCONFIG_MTAP_16_PACKET
    ulBufSize = ulPacketLength + 3;
    type = MTAP_16;
#endif //QTCONFIG_MTAP_16_PACKET

#ifdef QTCONFIG_MTAP_24_PACKET
    UINT8 tmp2[3];
    ulBufSize = ulPacketLength + 3;
    type = MTAP_24;
#endif //QTCONFIG_MTAP_24_PACKET

#else 
    //QTCONFIG_STAP_A_PACKET
    ulBufSize = ulPacketLength + 1;
    type = STAP_A;
#endif //QTCONFIG_INTERLEAVE_MODE

    if (m_pCCF)
    {
        if ((HXR_OK == m_pCCF->CreateInstance(CLSID_IHXPacket,
                                          (void**)&pNewPkt)) &&
            (HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, 
                                              (void**)&pBuf)) &&
            (HXR_OK == pBuf->SetSize(ulBufSize)))
        {
            UINT8* pCurPos = pBuf->GetBuffer();
            
           while(!tmpList->IsEmpty())
            {
                pNALPacket = (CNALPacket*)tmpList->RemoveHead();
                if(bFirstPacket)
                {
                    bFirstPacket = FALSE;
                    ulTimeStamp = pNALPacket->ulTimeStamp;
                    usStream     = pNALPacket->usStream;
                    uASMFlags   = pNALPacket->uASMFlags;
                    usDON       = pNALPacket->usDON;
                    memcpy(pCurPos, &type, 1);
                    pCurPos++;
#ifdef QTCONFIG_INTERLEAVE_MODE
                    tmp1[0] = UINT8 (usDON >> 8);
                    tmp1[1] = (UINT8)usDON;
                    memcpy(pCurPos, tmp1, 2);
                    pCurPos +=2;
#endif
                }

                // ASMRULE should be set according to last packet
                usASMRule   = pNALPacket->usASMRule;
                usSize = (UINT16)pNALPacket->ulSize;
                tmp1[0] = UINT8 (usSize >> 8);
                tmp1[1] = (UINT8)usSize;
                memcpy(pCurPos, tmp1, 2);
                pCurPos += 2;
#ifdef QTCONFIG_MTAP_16_PACKET
                uDOND = pNALPacket->usDON - usDON;
                memcpy(pCurPos, &uDOND, 1);
                pCurPos++;
                usTimeDiff = pNALPacket->ulTimeStamp - ulMinRtpTime;
                tmp1[0] = UINT8 (usTimeDiff >> 8);
                tmp1[1] = (UINT8)usTimeDiff;
                memcpy(pCurPos, tmp1, 2);
                pCurPos +=2;
#endif
#ifdef QTCONFIG_MTAP_24_PACKET 
                uDOND = pNALPacket->usDON - usDON;
                memcpy(pCurPos, &uDOND, 1);
                pCurPos++;
                usTimeDiff = pNALPacket->ulTimeStamp - ulMinRtpTime;
                tmp2[0] = UINT8 (usTimeDiff >> 16);
                tmp2[1] = UINT8 (usTimeDiff >> 8);
                tmp2[2] = (UINT8)usTimeDiff;
                memcpy(pCurPos, tmp2, 3);
                pCurPos +=3;

#endif
                memcpy(pCurPos, pNALPacket->pData, pNALPacket->ulSize);
                pCurPos += pNALPacket->ulSize;
                HX_DELETE(pNALPacket);
            }

            res = pNewPkt->Set(pBuf, ulTimeStamp, usStream, uASMFlags,
                               usASMRule);

            if (HXR_OK == res)
            {
                m_bPendingFlush = FALSE;
            } 
        }
        else
        {
            if(pNewPkt)
            {
                HX_DELETE(pNewPkt);
            }
            if(pBuf)
            {
                HX_DELETE(pBuf);
            }
        }
    }

    return pNewPkt;
}


HX_RESULT H264Packetizer::ConstructFragmentedPacket(CNALPacket* pNALPacket)
{
    HX_RESULT res = HXR_FAIL;
    UINT8 uFUIndicator;
    UINT8 uFUHeader;
    UINT8 uFUHeaderType;
    UINT8 tmp1[4];
    UINT8 tmp2[2];
    UINT16 usDON;
    UINT8* pData = pNALPacket->pData;
    UINT32 ulPacketLength = pNALPacket->ulSize;
    HXBOOL bFirstFragment = TRUE;
    IHXPacket* pNewPkt = NULL;
    IHXBuffer* pBuf = NULL;
    UINT32 offset = 0;
    UINT32 ulBufSize = 0;
    UINT32 ulHeaderSize = 2;
    UINT16 usASMRule = ASM_RULE_0;

    // Devide NAL octet in to FUIndicator and FUHeader.
    uFUIndicator = (pData[0] & 0xE0);
    uFUHeaderType = (pData[0] & 0x1F);
    pData++;
    ulPacketLength--;

    m_ulByteCount -=ulPacketLength;
    while(ulPacketLength > 0)
    {
#ifdef QTCONFIG_INTERLEAVE_MODE
            if(bFirstFragment)
                ulHeaderSize = 4; //For DON
            else
                ulHeaderSize = 2;
#else
            ulHeaderSize = 2;
#endif
        if(ulPacketLength > MAX_PACKET_SIZE)
        {
            ulBufSize = MAX_PACKET_SIZE;            
        }
        else 
        {
            ulBufSize = ulPacketLength + ulHeaderSize;
        }

            
        if (m_pCCF)
        {
            if ((HXR_OK == m_pCCF->CreateInstance(CLSID_IHXPacket,
                                              (void**)&pNewPkt)) &&
                (HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, 
                                                  (void**)&pBuf)) &&
                (HXR_OK == pBuf->SetSize(ulBufSize)))
            {
                UCHAR* pCurPos = pBuf->GetBuffer();


                if(bFirstFragment)
                {
                    bFirstFragment = FALSE;
#ifdef QTCONFIG_INTERLEAVE_MODE
                    usDON   = pNALPacket->usDON; 
                    tmp1[0] = uFUIndicator + FU_B;
                    tmp1[1] = 0x80 + uFUHeaderType;
                    tmp1[2] = UINT8 (usDON >> 8);
                    tmp1[3] = (UINT8)usDON;
                    memcpy(pCurPos, tmp1, 4);
                    pCurPos +=4;
#else
                    tmp2[0] = uFUIndicator + FU_A;
                    tmp2[1] = 0x80 + uFUHeaderType;
                    memcpy(pCurPos, tmp2, 2);
                    pCurPos +=2;
#endif
                }
                else
                {
                    tmp2[0] = uFUIndicator + FU_A;
                    if(ulPacketLength < MAX_PACKET_SIZE)
                        tmp2[1] = 0x40 + uFUHeaderType;
                    else
                        tmp2[1] = uFUHeaderType;

                    memcpy(pCurPos, tmp2, 2);
                    pCurPos +=2;
                }
                if(ulPacketLength > MAX_PACKET_SIZE)
                {
                    memcpy(pCurPos, pData, MAX_PACKET_SIZE - ulHeaderSize);
                    pData += (MAX_PACKET_SIZE - ulHeaderSize);
                    ulPacketLength -= (MAX_PACKET_SIZE - ulHeaderSize);
                }
                else
                {
                    memcpy(pCurPos, pData, ulPacketLength);
                    ulPacketLength = 0;
                    // ASMRULE should be set according to last packet
                    usASMRule   = ASM_RULE_1;
                }

                res = pNewPkt->Set(pBuf, pNALPacket->ulTimeStamp,
                                   pNALPacket->usStream, 
                                   pNALPacket->uASMFlags,
                                   usASMRule);

                if (HXR_OK == res)
                {
                    m_FinishedPktList.AddTail(pNewPkt);
                } 
            }
            else
            {
                if(pNewPkt)
                {
                    HX_DELETE(pNewPkt);
                }
                if(pBuf)
                {
                    HX_DELETE(pBuf);
                }
            }
        }
    }
    HX_DELETE(pNALPacket);
    return res;
}

void H264Packetizer::FlushQueues(void)
{

    IHXPacket* pDeadPacket;
    CNALPacket* pDeadNALPacket = NULL;

    while (!m_NALPacketList.IsEmpty())
    {
        pDeadNALPacket = (CNALPacket*) m_NALPacketList.RemoveHead();
        HX_DELETE(pDeadNALPacket);
    }

    while (!m_FinishedPktList.IsEmpty())
    {
        pDeadPacket = (IHXPacket*) m_FinishedPktList.RemoveHead();
        HX_RELEASE(pDeadPacket);
    }
}
