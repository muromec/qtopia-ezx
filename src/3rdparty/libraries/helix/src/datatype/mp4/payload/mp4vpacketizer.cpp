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

#include "hxcom.h"
#include "hxformt.h"
#include "hxccf.h"
#include "ihxpckts.h"
#include "safestring.h"
#include "hxslist.h"
#include "hxwintyp.h"
#include "hxformt.h"
#include "sdptools.h"
#include "mp4desc.h"

#include "mp4vpacketizer.h"

#include "cklicense.h"
#include "defslice.h"


const UINT32 FMTP_SIZE = 128;
const char* MP4V_LICENSE_ERROR_STR = 
    "Packetizer: This Server is not licenced to use video/MP4V-ES Packetizer.";


CMP4VPacketizer::CMP4VPacketizer()
    : m_ulRefCount(0)
    , m_pContext(NULL)
    , m_pCCF(NULL)
    , m_pStreamHeader(NULL)
{
}

CMP4VPacketizer::~CMP4VPacketizer()
{    
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pStreamHeader);
}

STDMETHODIMP 
CMP4VPacketizer::Init(IUnknown* pContext, BOOL bPacketize)
{
    if (!pContext)
    {
	return HXR_INVALID_PARAMETER;
    }
    else if (!bPacketize)
    {
	HX_ASSERT(!"CMP4VPacketizer::Init() only supports packetization");
	return HXR_UNEXPECTED;
    }

    HX_RESULT ret = CheckLicense(pContext, 
	REGISTRY_3GPPPACKETIZER_ENABLED, 
	LICENSE_3GPPPACKETIZER_ENABLED,
	MP4V_LICENSE_ERROR_STR);
    if (HXR_OK != ret)
    {
	return ret;
    }
    
    ret = pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);

    m_pContext = pContext;
    m_pContext->AddRef();
    
    if (HXR_OK == ret)
    {
	return MP4VPayloadFormat::Init(pContext, bPacketize);
    }
    else
    {
	return ret;
    }
}

STDMETHODIMP 
CMP4VPacketizer::SetStreamHeader(IHXValues* pHeader)
{
    HX_ASSERT(!m_pStreamHeader);

    if (!pHeader)
    {
	return HXR_INVALID_PARAMETER;
    }
    else
    {
	m_pStreamHeader = pHeader;
	m_pStreamHeader->AddRef();	
    }	

    HX_RESULT theErr;
    IHXBuffer* pOpaque = NULL;
    
    theErr = HandleMimeType();

    if (HXR_OK == theErr)
    {
	theErr = pHeader->GetPropertyBuffer("OpaqueData", pOpaque);
    }	
    if (HXR_OK == theErr)
    {    
	theErr = HandleOpaqueData(pOpaque);
	HX_RELEASE(pOpaque);
    }	

    if (HXR_OK == theErr)
    {
	HandleMaxPacketSize();
    }

    if (HXR_OK == theErr)
    {
	return MP4VPayloadFormat::SetStreamHeader(pHeader);
    }
    else
    {
	return theErr;
    }
}

HX_RESULT 
CMP4VPacketizer::HandleMimeType(void)
{
    HX_ASSERT(m_pStreamHeader);

    HX_RESULT theErr;	
    IHXBuffer* pMT = NULL;

    theErr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**) (&pMT));
    if (HXR_OK == theErr)
    {
	const char* pc = "video/MP4V-ES";
	theErr = pMT->Set((BYTE*)pc, strlen(pc) + 1);
    }
    if (HXR_OK == theErr)
    {
	theErr = m_pStreamHeader->SetPropertyCString("MimeType", pMT);
    }
    HX_RELEASE(pMT);
    return theErr;
}

void
CMP4VPacketizer::HandleMaxPacketSize(void)
{
    HX_RESULT theErr;
    UINT32 ulMaxPktSize = 0;
    
    theErr = GetConfigDefaultMaxPacketSize(m_pContext, 
	"config.Datatypes.video/mp4v-es.MaxPacketSize",
	ulMaxPktSize);
    if (HXR_OK == theErr)
    {
	SetMaxPacketSize(ulMaxPktSize);
    }
    else if (m_pStreamHeader->GetPropertyULONG32("MaxPacketSize", ulMaxPktSize) == HXR_OK)
    {
	SetMaxPacketSize(ulMaxPktSize);
    }
    else
    {
	SetMaxPacketSize(1400);
    }
}

HX_RESULT
CMP4VPacketizer::HandleOpaqueData(IHXBuffer* pOpaque)
{
    HX_RESULT theErr;
    ES_Descriptor ESDesc;
    DecoderConfigDescriptor* pDCDesc = NULL;
    UINT8* pOpaqueData = pOpaque->GetBuffer();
    UINT32 ulOpaqueDataSize = pOpaque->GetSize();
    
    theErr = ESDesc.Unpack(pOpaqueData, ulOpaqueDataSize);
    
    if (HXR_OK == theErr)
    {
	pDCDesc = ESDesc.m_pDecConfigDescr;
    
    	if (pDCDesc)
    	{
    	    m_pStreamHeader->SetPropertyULONG32("AvgBitRate", pDCDesc->m_ulAvgBitrate);
    	    m_pStreamHeader->SetPropertyULONG32("MaxBitRate", pDCDesc->m_ulMaxBitrate);
    	        
	    if (pDCDesc->m_pDecSpecificInfo)
	    {
		theErr = HandleFMTP(pDCDesc->m_pDecSpecificInfo->m_pData, 
				    pDCDesc->m_pDecSpecificInfo->m_ulLength);
	    }    
    	}
    }

    return theErr;
}

HX_RESULT 
CMP4VPacketizer::HandleFMTP(BYTE* pOpaque, UINT32 ulOpaqueSize)
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
	theErr = pSDPData->SetSize(pOldSDPData->GetSize() + FMTP_SIZE + ulOpaqueSize*2);
	if (HXR_OK == theErr)
	{
	    const char* pToken = "a=fmtp";
	    const char* pOldSDPBuf = (const char*)pOldSDPData->GetBuffer();
	    pSDPStart = pSDPBuf = (char*)pSDPData->GetBuffer();
	    ulSDPBufSize = pSDPData->GetSize();
	    
	    UINT32 ulUsed = RemoveSDPToken(pToken, sizeof(pToken), 
		pOldSDPBuf, pOldSDPData->GetSize(), pSDPBuf, ulSDPBufSize);

	    HX_ASSERT(ulSDPBufSize >= ulUsed);	    
	    ulSDPBufSize -= ulUsed;		
	    pSDPBuf += ulUsed;
	}
	HX_RELEASE(pOldSDPData);
    }
    else
    {
	theErr = pSDPData->SetSize(FMTP_SIZE + ulOpaqueSize*2);
	if (HXR_OK == theErr)
	{
	    pSDPStart = pSDPBuf = (char*)pSDPData->GetBuffer();
	    ulSDPBufSize = pSDPData->GetSize();
	}	    
    }

    HX_ASSERT(pOpaque && pSDPBuf && pSDPStart && 
	ulSDPBufSize && ulPT && !pOldSDPData);


    if (HXR_OK == theErr)
    {
	char* pStart = pSDPBuf;
	UINT32 ulSize = ulSDPBufSize;
    
    	// visual_object_sequence_start_code
    	static UINT8 pSS[] = {0x00, 0x00, 0x01, 0xb0};
    	if (ulOpaqueSize >= 5 && !memcmp(pOpaque, pSS, 4))
    	{	
	    int lUsed = SafeSprintf(pSDPBuf, ulSize, "a=fmtp:%u profile-level-id=%u; config=",
		    	ulPT, pOpaque[4]);
    
	    pSDPBuf += lUsed;
	    ulSize -= lUsed;
    	}	    
    
    	// write out config
    	if ((ulOpaqueSize * 2 + 3) <= ulSize)
    	{
    	    BinaryToHexString(pOpaque, ulOpaqueSize, pSDPBuf);
	    pSDPBuf += ulOpaqueSize*2;
	    ulSize -= ulOpaqueSize*2;
	    pSDPBuf += SafeSprintf(pSDPBuf, ulSize, "\r\n\0");
    	}   
	else
	{
	    HX_ASSERT(!"this should never happen");
	}

    	
	HX_ASSERT(pSDPBuf - pStart == (int)strlen(pStart));
	theErr = pSDPData->SetSize(pSDPBuf - pSDPStart);
    }

    if (HXR_OK == theErr)
    {
	m_pStreamHeader->SetPropertyCString("SDPData", pSDPData);	    
    }
    
    HX_RELEASE(pSDPData);
    return theErr;
}


STDMETHODIMP_(ULONG32) CMP4VPacketizer::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) CMP4VPacketizer::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

