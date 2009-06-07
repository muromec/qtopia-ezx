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
#include "h263pkt.h"
#include "debug.h"
#include "ihxpckts.h"
#include "safestring.h"
#include "sdptools.h"

#include "defslice.h"
#include "cklicense.h"

#include "qtatmmgs.h"

#define D_H263 D_INFO
//#define H263PKT_RANDOM_PKT_SIZE 0;
//#define H263DUMP 0

const UINT32 SDP_SIZE = 128;

const char* LICENSE_ERROR_STR = 
    "Packetizer: This Server is not licenced to use video/H263-2000 Packetizer.";


// Genaral H.263+ Payload Header (RFC 2429)
typedef struct tagRTP_H263PLUS_PAYLOAD_HEADER 
{
    UINT16   RR:5;	// reserved bits, set to 0
    UINT16   P:1;	// picture segment start (1 - start of picture, GOB or Slice, 0 - otherwise)
    UINT16   V:1;	// VRC bit (0 - VRC not present)
    UINT16   PLEN:6;	// length in bytes of extra picture header (0 for now)
    UINT16   PEBIT:3;	// bits that shall be ignored in the last byte of picture header (0 if plen = 0)
} RTP_H263PLUS_PAYLOAD_HEADER;

const UINT32 CH263Packetizer::zm_ulRTP263PlusHdrSize = sizeof(RTP_H263PLUS_PAYLOAD_HEADER);

CH263Packetizer::CH263Packetizer(CQT_TrackInfo_Manager* pTrackInfo)
    : m_lRefCount(0)
    , m_pContext(NULL)
    , m_pCCF(0)
    , m_pStreamHeader(NULL)
    , m_ulMaxPktSize(1460)
    , m_ulCurMaxPktSize(m_ulMaxPktSize)
    , m_state(H263PKT_STATE_CLOSE)
    , m_pOutPktQueue(NULL)
    , m_pTrackInfo(pTrackInfo)
{
}

CH263Packetizer::~CH263Packetizer()
{
    Close();
}

/*
 *	IHXPayloadFormatObject methods
 */
STDMETHODIMP CH263Packetizer::Init(IUnknown* pContext, BOOL bPacketize)
{
    DPRINTF(D_H263, ("CH263Packetizer::Init(%p, %d)\n", pContext, bPacketize));

    if (H263PKT_STATE_CLOSE != m_state)
    {
	// already initizalized
	HX_ASSERT(H263PKT_STATE_CLOSE != m_state);
	return HXR_UNEXPECTED;
    }
    else if (!pContext)
    {
	return HXR_INVALID_PARAMETER;
    }
    else if (!bPacketize)
    {
	HX_ASSERT(!"CH263Packetizer:Init() only supports packetization");
	return HXR_UNEXPECTED;
    }

    
    HX_RESULT theErr = CheckLicense(pContext, 
	REGISTRY_3GPPPACKETIZER_ENABLED, 
	LICENSE_3GPPPACKETIZER_ENABLED,
	LICENSE_ERROR_STR);
    if (HXR_OK != theErr)
    {
	return theErr;
    }

    theErr = pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    if (HXR_OK != theErr)
    {
	return theErr;
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    m_state = H263PKT_STATE_READY;
    return Reset();
}

STDMETHODIMP CH263Packetizer::Close()
{
    DPRINTF(D_H263, ("CH263Packetizer::Close()\n"));
    m_state = H263PKT_STATE_CLOSE;

    FlushOutput();
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pStreamHeader);

    return HXR_OK;
}

STDMETHODIMP CH263Packetizer::Reset()
{
    // Called on Seeks
    DPRINTF(D_H263, ("CH263Packetizer::Reset()\n"));

    if (H263PKT_STATE_READY != m_state)
    {
	return HXR_UNEXPECTED;
    }    

    FlushOutput();

    return HXR_OK;
}

inline static ULONG32 GetUL32(UINT8 *pData)
{
    return (*pData << 24) |
	   (*(pData + 1) << 16) |
	   (*(pData + 2) << 8 ) |
	    *(pData + 3);
}

HX_RESULT
CH263Packetizer::HandleSDP(IHXBuffer* pOpaque)
{
    HX_ASSERT(m_pStreamHeader);
    HX_ASSERT(pOpaque);
    
    HX_RESULT	theErr = HXR_FAIL;
    IHXBuffer*	pSDPData = NULL;
    IHXBuffer*	pOldSDPData = NULL;
    UINT32	ulPT = 0;
    char*	pSDPStart = NULL;
    char*	pSDPBuf = NULL;
    UINT32	ulSDPBufSize = 0;

    theErr = m_pStreamHeader->GetPropertyULONG32("RTPPayloadType", ulPT);
    if (HXR_OK != theErr)
    {
	HX_ASSERT(!"Need to set RTPPayloadType");
	return HXR_INVALID_PARAMETER;
    }

    theErr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**) (&pSDPData));
    if (HXR_OK != theErr)
    {
	// no hope...
	return theErr;
    }

    theErr = m_pStreamHeader->GetPropertyCString("SDPData", pOldSDPData);
    if (HXR_OK == theErr)
    {
	theErr = pSDPData->SetSize(pOldSDPData->GetSize() + SDP_SIZE);
	if (HXR_OK == theErr)
	{
            const char* pTokens[] = {"a=fmtp", "a=framesize", NULL};
            const UINT32 pulTokenLens[] = {6, 11, 0};
	    const char* pOldSDPBuf = (const char*)pOldSDPData->GetBuffer();
	    pSDPStart = pSDPBuf = (char*)pSDPData->GetBuffer();
	    ulSDPBufSize = pSDPData->GetSize();
	    
            UINT32 ulUsed = RemoveSDPTokens(pTokens, pulTokenLens, pOldSDPBuf, 
                pOldSDPData->GetSize(), pSDPBuf, ulSDPBufSize);

       	    HX_ASSERT(ulSDPBufSize >= ulUsed);	    
	    ulSDPBufSize -= ulUsed;		
	    pSDPBuf += ulUsed;
	}
	HX_RELEASE(pOldSDPData);
    }
    else
    {
	theErr = pSDPData->SetSize(SDP_SIZE);
	if (HXR_OK == theErr)
	{
	    pSDPStart = pSDPBuf = (char*)pSDPData->GetBuffer();
	    ulSDPBufSize = pSDPData->GetSize();
	}	    
    }

    HX_ASSERT(pOpaque && pSDPBuf && pSDPStart && 
	ulSDPBufSize && ulPT && !pOldSDPData);
                
    if (HXR_OK == theErr && pOpaque->GetSize() >= 15)
    {
	// H263SpecificBox
	UINT8* pc = pOpaque->GetBuffer();
	HX_ASSERT(GetUL32(pc) >= 15);
	HX_ASSERT(*(pc+4)=='d'&&*(pc+5)=='2'&&*(pc+6)=='6'&&*(pc+7)=='3');

	if (HXR_OK == theErr)
        {
            int nSize = SafeSprintf(pSDPBuf, ulSDPBufSize, 
		"a=fmtp:%u profile=%u; level=%u\r\n",
		ulPT, pc[14], pc[13]);
            if (nSize > 0)
            {
                pSDPBuf += nSize;
                ulSDPBufSize -= nSize;
            }

            if (m_pTrackInfo)
            {
                UINT32 ulWidth = m_pTrackInfo->GetFrameWidth();
                UINT32 ulHeight = m_pTrackInfo->GetFrameHeight();
                if (ulWidth && ulHeight)
                {
                    nSize = SafeSprintf(pSDPBuf, ulSDPBufSize,
                        "a=framesize:%u %u-%u\r\n", ulPT, ulWidth, ulHeight);
                    if (nSize > 0)
                    {
                        pSDPBuf += nSize;
                    }
                }
            }
	    theErr = pSDPData->SetSize(pSDPBuf - pSDPStart + 1);
	}
	if (HXR_OK == theErr)
	{
	    m_pStreamHeader->SetPropertyCString("SDPData", pSDPData);  
	}
    }
    
    HX_RELEASE(pSDPData);
    return theErr;
}

void
CH263Packetizer::HandleBitRates(IHXBuffer* pOpaque)
{
    HX_ASSERT(m_pStreamHeader);
    HX_ASSERT(pOpaque);
    
    if (pOpaque->GetSize() >= 31)
    {
    	// BitrateBox in H263SpecificBox
	UINT8* pc = pOpaque->GetBuffer();
    	HX_ASSERT(GetUL32(pc+15) >= 16);
    	HX_ASSERT(*(pc+19)=='b'&&*(pc+20)=='i'&&*(pc+21)=='t'&&*(pc+22)=='r');
    	m_pStreamHeader->SetPropertyULONG32("AvgBitRate", GetUL32(pc+23));
    	m_pStreamHeader->SetPropertyULONG32("MaxBitRate", GetUL32(pc+27));
    }		
}

HX_RESULT 
CH263Packetizer::HandleMimeType(void)
{
    HX_ASSERT(m_pStreamHeader);

    HX_RESULT theErr;	
    IHXBuffer* pMT = NULL;

    theErr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**) (&pMT));
    if (HXR_OK == theErr)
    {
	const char* pc = "video/H263-2000";
	theErr = pMT->Set((BYTE*)pc, strlen(pc) + 1);
    }
    if (HXR_OK == theErr)
    {
	theErr = m_pStreamHeader->SetPropertyCString("MimeType", pMT);
    }
    HX_RELEASE(pMT);
    return theErr;
}

HX_RESULT
CH263Packetizer::HandleMaxPacketSize(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);
    /*
     * there are 4 possible max pkt size, in the order of preference
     * 1) in packetizer specific config section
     * 2) in general packetizer config section
     * 3) "MaxPacketSize" in stream header
     * 4) hard coded value
     */
    HX_RESULT theErr;
    UINT32 ulMaxPktSize = 0;
    
    theErr = GetConfigDefaultMaxPacketSize(m_pContext, 
	"config.Datatypes.video/h263-2000.MaxPacketSize",
	ulMaxPktSize);
	
    if (HXR_OK == theErr)
    {
	if (ulMaxPktSize > zm_ulRTP263PlusHdrSize)
	{
	    m_ulCurMaxPktSize = m_ulMaxPktSize = ulMaxPktSize;
	    theErr = pHeader->SetPropertyULONG32("MaxPacketSize", ulMaxPktSize);
	}
	else
	{
	    theErr = HXR_FAIL;
	}
    }	
    else
    {    
	if (pHeader->GetPropertyULONG32("MaxPacketSize", ulMaxPktSize) == HXR_OK)
	{
	    if (ulMaxPktSize > zm_ulRTP263PlusHdrSize)
	    {
		m_ulCurMaxPktSize = m_ulMaxPktSize = ulMaxPktSize;
	    }
	}
	else
	{
	    theErr = pHeader->SetPropertyULONG32("MaxPacketSize", m_ulCurMaxPktSize);
	}
    }    
    return theErr;
}

STDMETHODIMP CH263Packetizer::SetStreamHeader(IHXValues* pHeader)
{
    DPRINTF(D_H263, ("CH263Packetizer::SetStreamHeader()\n"));
    HX_ASSERT(!m_pStreamHeader);

    if (H263PKT_STATE_READY != m_state)
    {
	return HXR_UNEXPECTED;
    }
    else if (!pHeader)
    {
	return HXR_INVALID_PARAMETER;
    }
    else
    {
	m_pStreamHeader = pHeader;
	m_pStreamHeader->AddRef();	
    }	

    HX_RESULT theErr = HXR_OK;
    IHXBuffer* pOpaque = NULL;
    UINT32 ulMaxPktSize = 0;

    theErr = HandleMimeType();

    if (HXR_OK == theErr)
    {
	theErr = pHeader->GetPropertyBuffer("OpaqueData", pOpaque);
    }	
    if (HXR_OK == theErr)
    {    
	theErr = HandleSDP(pOpaque);
	HandleBitRates(pOpaque);
	HX_RELEASE(pOpaque);
    }	

    theErr = HandleMaxPacketSize(pHeader);

#ifdef H263DUMP
    IHXBuffer* pb = NULL;
    char*   pc = NULL;
    UINT32 ul = 0;

    if (pHeader->GetFirstPropertyULONG32(pc, ul) == HXR_OK)
    {
	printf("ULONG32:\n");
	printf("\t%s: %u\n", pc, ul);
	while (pHeader->GetNextPropertyULONG32(pc, ul) == HXR_OK)
	{
	    printf("\t%s: %u\n", pc, ul);	    
	}
	fflush(0);
    }
    if (pHeader->GetFirstPropertyBuffer(pc, pb) == HXR_OK)
    {
	printf("Buffer:\n");
	printf("\t%s: %s\n", pc, pb->GetBuffer());
	pb->Release();
	while (pHeader->GetNextPropertyBuffer(pc, pb) == HXR_OK)
	{
	    printf("\t%s: %s\n", pc, pb->GetBuffer());
	    pb->Release();
	}
	fflush(0);
    }    
    if (pHeader->GetFirstPropertyCString(pc, pb) == HXR_OK)
    {
	printf("CString:\n");
	printf("\t%s: %s\n", pc, pb->GetBuffer());
	pb->Release();
	while (pHeader->GetNextPropertyCString(pc, pb) == HXR_OK)
	{
	    printf("\t%s: %s\n", pc, pb->GetBuffer());
	    pb->Release();
	}
	fflush(0);
    }

    IHXBuffer* pBuf = NULL;
    if (pHeader->GetPropertyBuffer("OpaqueData", pBuf) == HXR_OK)
    {
	// H263SpecificBox field 		    	
	if (pBuf->GetSize() >= 15)
        {
	    UINT8* pc = pBuf->GetBuffer();
	    if (pc)
	    {
		HX_ASSERT(GetUL32(pc) >= 15); // BoxHader.Size(32)
		HX_ASSERT(*(pc+4)=='d' && *(pc+5)=='2' && *(pc+6)=='6' && *(pc+7)=='3');// BoxHeader.Type(32)
				
		printf("size: %u type: %c%c%c%c ven: %u ver: %u level: %u profile: %u\n",
		    GetUL32(pc),*(pc+4), *(pc+5), *(pc+6), *(pc+7), GetUL32(pc+8),
		    *(pc+12), *(pc+13), *(pc+14));

		if (pBuf->GetSize() >= 31)
		{
		    printf("size: %u type: %c%c%c%c AvgBit: %u MaxBit: %u\n",
			GetUL32(pc+15), *(pc+19), *(pc+20), *(pc+21), *(pc+22),
			GetUL32(pc+23), GetUL32(pc+27));
		}
		fflush(0);
            }
	}
	HX_RELEASE(pBuf);
    }
#endif

    return theErr;
}

STDMETHODIMP CH263Packetizer::GetStreamHeader(REF(IHXValues*) pHeader)
{
    DPRINTF(D_H263, ("CH263Packetizer::GetStreamHeader()\n"));
    HX_ASSERT(m_pStreamHeader);

    if (H263PKT_STATE_READY != m_state)
    {
	return HXR_UNEXPECTED;
    }
    else if (!m_pStreamHeader)
    {
	return HXR_UNEXPECTED;
    }
        
    pHeader = m_pStreamHeader;
    pHeader->AddRef();
    return HXR_OK;
}

STDMETHODIMP CH263Packetizer::SetPacket(IHXPacket* pPacket)
{    
    DPRINTF(D_H263, ("CH263Packetizer::SetPacket()\n"));    
    HX_ASSERT(pPacket);

#ifdef H263PKT_RANDOM_PKT_SIZE
    m_ulMaxPktSize = 4000;
    m_ulCurMaxPktSize = 0;
    while (m_ulCurMaxPktSize <= zm_ulRTP263PlusHdrSize)
    {
	m_ulCurMaxPktSize = rand() % m_ulMaxPktSize;
    }	
#endif

    if (H263PKT_STATE_READY != m_state)
    {
	return HXR_UNEXPECTED;
    }
   
    HX_RESULT theErr = HXR_UNEXPECTED;


    if (NULL == m_pOutPktQueue)
    {
	IHXRTPPacket* pRTPPkt = NULL;	
	if (pPacket->QueryInterface(IID_IHXRTPPacket, (void**) &pRTPPkt) == HXR_OK)
	{
	    m_pOutPktQueue = &CH263Packetizer::AddOutHXRTPPkt;
	    pRTPPkt->Release();
	}
	else
	{
	    m_pOutPktQueue = &CH263Packetizer::AddOutHXPkt;
	}		
    }

    theErr = PacketizeRFC2429(pPacket);
    
    return theErr;
}

HX_RESULT
CH263Packetizer::PacketizeRFC2429(IHXPacket* pInHXPkt)
{
    IHXBuffer* pData	    = pInHXPkt->GetBuffer();
    UINT8* pBuffer	    = pData->GetBuffer();	
    UINT32 ulBufferSize     = pData->GetSize();

    // Makes no sense to packetize less than one byte.
    const UINT32 ulMinPacketSize = 3;

    if (ulBufferSize < ulMinPacketSize)
    {
        HX_RELEASE(pData);
        return HXR_FAILED;
    }

    UINT32 ulDataSize	    = ulBufferSize - 2;
    UINT32 ulNewDataSize    = 0;
#ifdef H263DUMP
    printf("%p: Packtize: size: %u Max: %u\n", this, ulBufferSize, m_ulCurMaxPktSize);fflush(0);
#endif    

#ifdef _DEBUG
    BOOL    bGenerated = FALSE;
#endif
    if ((ulDataSize + zm_ulRTP263PlusHdrSize) > m_ulCurMaxPktSize) 
    {	
	UINT32 ulSearchSize = ulBufferSize;	
	UINT8* pPacket	    = NULL;
	UINT8* pNextPacket  = pBuffer;
	
        // Check for valid H.263+ GOB/Slice header at the start
        if (!(pBuffer[0] == 0x00 && pBuffer[1] == 0x00 && pBuffer[2] & 0x80))
        {
            HX_RELEASE(pData);
            return HXR_FAILED;
        }

	// Split large frame into smaller packets at GOB/Slice boundaries 
	while (pPacket = (UINT8*)memchr(pNextPacket, 0x00, ulSearchSize)) 
	{
	    ulSearchSize = ulBufferSize - (pPacket - pBuffer);
	    
	    // Check for valid H.263+ GOB/Slice header and at least one byte
	    if (ulSearchSize >= ulMinPacketSize && (pPacket[1] == 0x00) && (pPacket[2] & 0x80)) 
	    {		
		// Find next GOB or Slice header
		while (pNextPacket = (UINT8*)memchr(pNextPacket + 1, 0x00, ulSearchSize - 1)) 
		{
		    ulSearchSize = ulBufferSize - (pNextPacket - pBuffer);
		    
		    // Check for valid H.263+ GOB/Slice header and at least one byte
		    if (ulSearchSize >= ulMinPacketSize && pNextPacket[1] == 0x00 && (pNextPacket[2] & 0x80)) 
		    {
			ulDataSize = pNextPacket - pPacket - 2;
			if (ulDataSize > ulBufferSize)
			{
			    HX_ASSERT(FALSE); // should not happen
			    HX_RELEASE(pData);
			    return HXR_FAILED;
			}

			GeneratePacket(pPacket+2, ulDataSize, pInHXPkt, MARKER_FALSE);
			HX_ASSERT(bGenerated = 1);
			break;
		    }
		}
		
		// Search completed
		if (!pNextPacket)
		{
		    break;
		}
	    }
	}
	
	// Formatting the last packet (or original packet without GOB/Slice headers)
	if (pPacket) 
	{
            // Protect subtraction from going negative since we are using unsigned
	    if ((ulBufferSize - 2U) > (UINT32)(pPacket - pBuffer) &&
                (pPacket[0] == 0x00) && (pPacket[1] == 0x00) && (pPacket[2] & 0x80)) 
	    {
		ulDataSize = pBuffer + ulBufferSize - pPacket - 2;
		if (ulDataSize > ulBufferSize)
		{
		    HX_ASSERT(FALSE); // should not happen
		    HX_RELEASE(pData);
		    return HXR_FAILED;
		}

		if ((ulDataSize + zm_ulRTP263PlusHdrSize) > m_ulCurMaxPktSize) 
		{
		    GeneratePacket(pPacket+2, ulDataSize, pInHXPkt, MARKER_UNKNOWN);
		}		    
		else
		{
		    GeneratePacket(pPacket+2, ulDataSize, pInHXPkt, MARKER_TRUE);   
		}
		HX_ASSERT(bGenerated = 1);
	    }
	}
    }
    else 
    {	// ulBufferSize <= m_ulCurMaxPacketSize, format frame into 1 packet
	// Check for valid H.263+ PSC
	if ((pBuffer[0] == 0x00) && (pBuffer[1] == 0x00) && (pBuffer[2] & 0x80) && 
	    ((pBuffer[2] & 0x7C) == 0)) 
	{
	    GeneratePacket(pBuffer+2, ulDataSize, pInHXPkt, MARKER_TRUE);
	    HX_ASSERT(bGenerated = 1);
	}
    }

    HX_RELEASE(pData);
    HX_ASSERT(bGenerated);
    return HXR_OK;
}

/*
 * Takes care of fragmentation.
 * Create a hx buffer and a hx packet. 
 * Write H263+ payload header.
 * Write H263+ data
 * Add a hx packet to a outpkt list
 */
HX_RESULT 
CH263Packetizer::GeneratePacket(UINT8* pData, UINT32 ulDataSize, IHXPacket* pInHXPkt, RTPMarkerBit mBit)
{
#ifdef H263DUMP 
    printf("\tGeneratePacket: ");
#endif    
    // If the packet size is more than m_ulCurMaxPktSize
    // produce 1 picture segment start packet and follow-on packets (RFC 2429)
    HX_ASSERT(m_ulCurMaxPktSize > zm_ulRTP263PlusHdrSize);
    HX_ASSERT(m_ulCurMaxPktSize <= m_ulMaxPktSize);

#ifdef _DEBUG
    BOOL bTest = (BOOL)((ulDataSize + zm_ulRTP263PlusHdrSize) > m_ulCurMaxPktSize);
    UINT32 ulIteration = 0;
#endif

    HX_RESULT theErr;
    UINT32  ulNewDataSize = ulDataSize;
    UINT8*  pNewData = pData;
    UINT8*  pPktData = NULL;
    BOOL    bPBit = 1;
    IHXBuffer* pHXBuf = NULL;    

    BOOL    bMBit = FALSE;    
    if (MARKER_TRUE == mBit)
    {
	bMBit = TRUE;
    }
    else if (MARKER_UNKNOWN == mBit)
    {
	bMBit = (BOOL)((ulDataSize + zm_ulRTP263PlusHdrSize) <= m_ulCurMaxPktSize);
    }
    
    while (ulDataSize)
    {
	HX_ASSERT(ulIteration += 1);
#ifdef H263DUMP	
	printf(" %u[P:%u ", ulIteration, bPBit);
#endif	

	ulNewDataSize = min(ulDataSize, m_ulCurMaxPktSize - zm_ulRTP263PlusHdrSize);
	HX_ASSERT((!bTest &&  bPBit && (ulNewDataSize == ulDataSize)) || // only once
		  ( bTest && (ulNewDataSize != ulDataSize)) || 
		  ( bTest && !bPBit && (ulNewDataSize == ulDataSize))); // the last one

	pPktData = MakeBuffer(pHXBuf, ulNewDataSize + zm_ulRTP263PlusHdrSize);
	if (!pPktData)
	{   
	    return HXR_OUTOFMEMORY;
	}
									
	pPktData = WriteRTP263PlusPayloadHdr(pPktData, bPBit);

	memcpy(pPktData,	// offset to data
	       pNewData,	// actual data
	       ulNewDataSize);  // data size

	// Next packet is follow-on
	bPBit = 0;
	ulDataSize -= ulNewDataSize;
	pNewData += ulNewDataSize;

	HX_ASSERT(MARKER_UNKNOWN == mBit ||
		  (MARKER_TRUE == mBit && bMBit) ||
		  (MARKER_FALSE == mBit && !bMBit));
	if (MARKER_UNKNOWN == mBit)
	{
	    if (!ulDataSize)
	    {
		bMBit = TRUE;
	    }		
	    else
	    {
		HX_ASSERT(!bMBit);
	    }
	}

#ifdef H263DUMP	
	printf("M:%u]", bMbit);
#endif	
	
	// AddOutPkt
	theErr = (this->*m_pOutPktQueue)(pHXBuf, pInHXPkt, bMBit);
	HX_RELEASE(pHXBuf);

	if (HXR_OK != theErr)
	{
	    return theErr;
	}	
    }    

#ifdef H263DUMP
    printf("\n");
#endif    
#ifdef _DEBUG
    if (!bTest)
    {
	// should be only once.
	HX_ASSERT(1 == ulIteration);	
    }
#endif    

    return HXR_OK;
}

UINT8*
CH263Packetizer::WriteRTP263PlusPayloadHdr(UINT8* pc, BOOL bPBit)
{    
    HX_ASSERT(zm_ulRTP263PlusHdrSize == sizeof(RTP_H263PLUS_PAYLOAD_HEADER));

    pc[0] = (bPBit<<2)&0x04;
    pc[1] = 0x00;    

    return pc + zm_ulRTP263PlusHdrSize;
}

UINT8*
CH263Packetizer::MakeBuffer(REF(IHXBuffer*)pOutBuf, UINT32 ulSize)
{
    HX_RESULT theErr;

    theErr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**) (&pOutBuf));
    
    if (HXR_OK == theErr)
    {
	if (pOutBuf->SetSize(ulSize) == HXR_OK)
	{	
	    return pOutBuf->GetBuffer();
	}	
	pOutBuf->Release();
    }
    
    return NULL;
}

HX_RESULT
CH263Packetizer::AddOutHXPkt(IHXBuffer* pBuf, IHXPacket* pInPkt, BOOL bMBit)
{
//    HX_ASSERT((H263PK_IN_PKT_TYPE_HX == m_inPktType));
    HX_RESULT theErr;
    IHXPacket* pNewPkt;

    theErr = m_pCCF->CreateInstance(CLSID_IHXPacket, 
					     (void**) &pNewPkt);
    if (HXR_OK != theErr)
    {
	return theErr;
    }

    theErr = pNewPkt->Set(pBuf, 
			  pInPkt->GetTime(), 
			  pInPkt->GetStreamNumber(),
			  bMBit ? pInPkt->GetASMFlags() : HX_ASM_SWITCH_OFF,
			  bMBit ? pInPkt->GetASMRuleNumber() : 0);
    if (HXR_OK == theErr)
    {
	m_outputQueue.AddTail(pNewPkt);    
    }
    else
    {
	pNewPkt->Release();
    }

    return theErr;
}


HX_RESULT
CH263Packetizer::AddOutHXRTPPkt(IHXBuffer* pBuf, IHXPacket* pInPkt, BOOL bMBit)
{
//    HX_ASSERT((H263PK_IN_PKT_TYPE_RTP == m_inPktType));
    HX_RESULT theErr;
    IHXRTPPacket* pNewPkt;

    theErr = m_pCCF->CreateInstance(CLSID_IHXRTPPacket, (void**) &pNewPkt);
    if (HXR_OK != theErr)
    {
	return theErr;
    }

    theErr = pNewPkt->SetRTP(pBuf, 
			     ((IHXRTPPacket*)pInPkt)->GetTime(), 
			     ((IHXRTPPacket*)pInPkt)->GetRTPTime(),
			     ((IHXRTPPacket*)pInPkt)->GetStreamNumber(),
			     bMBit ? ((IHXRTPPacket*)pInPkt)->GetASMFlags() : HX_ASM_SWITCH_OFF,
			     bMBit ? ((IHXRTPPacket*)pInPkt)->GetASMRuleNumber() : 0);
    if (HXR_OK == theErr)
    {
	m_outputQueue.AddTail(pNewPkt);    
    }
    else
    {
	pNewPkt->Release();
    }

    return theErr;
}


STDMETHODIMP CH263Packetizer::GetPacket(REF(IHXPacket*) pPacket)
{
    DPRINTF(D_H263, ("CH263Packetizer::GetPacket()\n"));
    HX_RESULT res = HXR_FAILED;

    if (H263PKT_STATE_READY != m_state)
    {
	res =  HXR_UNEXPECTED;
    }
    else if (m_outputQueue.IsEmpty())
    {
	res = HXR_INCOMPLETE;
    }
    else
    {
	pPacket = (IHXPacket*)m_outputQueue.RemoveHead();
	res = HXR_OK;
    }
    
    DPRINTF(D_H263, ("CH263Packetizer::GetPacket() : %d\n", res));

    return res;
}

STDMETHODIMP CH263Packetizer::Flush()
{
    // Called at the end of the clip
    DPRINTF(D_H263, ("CH263Packetizer::Flush()\n"));

    if (H263PKT_STATE_READY != m_state)
    {
	return HXR_UNEXPECTED;
    }
    else
    {
	return HXR_OK;
    }	
}

void CH263Packetizer::FlushOutput()
{
    LISTPOSITION pos = m_outputQueue.GetHeadPosition();
    
    while(pos != 0)
    {
	IHXPacket* pPacket = (IHXPacket*)m_outputQueue.GetNext(pos);
	
	HX_RELEASE(pPacket);
    }

    m_outputQueue.RemoveAll();
}

/*
 *	IUnknown methods
 */
STDMETHODIMP CH263Packetizer::QueryInterface(REFIID riid,
						   void** ppvObj)
{
    HX_RESULT res = HXR_NOINTERFACE;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	res = HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPayloadFormatObject))
    {
	AddRef();
	*ppvObj = (IHXPayloadFormatObject*)this;
	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP_(ULONG32) CH263Packetizer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CH263Packetizer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

