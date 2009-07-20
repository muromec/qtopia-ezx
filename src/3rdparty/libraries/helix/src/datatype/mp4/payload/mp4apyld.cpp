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

 /****************************************************************************
 *  Defines
 */
#define _OVERALLOC_CODEC_DATA	3

#define FLUSH_ALL_PACKETS   0xFFFFFFFF

#define CHAR_LF	0x0a
#define CHAR_CR	0x0d

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"

#include "hxassert.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxrendr.h"
#include "hxformt.h"
#include "hxengin.h"

#include "mp4-latm-depack.h"

#include "mp4desc.h"
#include "mp4apyld.h"
#include "mp4pyldutil.h"

MP4APayloadFormat::MP4APayloadFormat()
    : m_lRefCount	    (0)
    , m_pClassFactory	    (NULL)
    , m_pStreamHeader	    (NULL)
    , m_bFlushed	    (FALSE)
    , m_bUsesRTPPackets	    (FALSE)
    , m_bRTPPacketTested    (FALSE)
    , m_bPacketize	    (FALSE)
    , m_bPriorLoss	    (FALSE)
#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
    , m_pLATMDepack	    (NULL)
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */
    , m_pAudioConfig	    (NULL)
    , m_ulAudioConfigSize   (0)
    , m_unAudioConfigType   (2)
    , m_uObjectProfileIndication(0)
    , m_ulSamplesPerSecond  (1000)
    , m_ulRTPSamplesPerSecond(0)
    , m_PayloadID	    (PYID_X_HX_MP4_RAWAU)
{
    ;
}

MP4APayloadFormat::~MP4APayloadFormat()
{
    FlushPackets(FLUSH_ALL_PACKETS);

    HX_VECTOR_DELETE(m_pAudioConfig);
#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
    HX_DELETE(m_pLATMDepack);
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);
}

HX_RESULT MP4APayloadFormat::Build(REF(IMP4APayloadFormat*) pFmt)
{
    pFmt = new MP4APayloadFormat();

    HX_RESULT res = HXR_OUTOFMEMORY;
    if (pFmt)
    {
	pFmt->AddRef();
	res = HXR_OK;
    }

    return res;
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//	object.
//
STDMETHODIMP
MP4APayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
MP4APayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
MP4APayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
MP4APayloadFormat::Init(IUnknown* pContext,
			HXBOOL bPacketize)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pClassFactory);

    m_bPacketize = bPacketize;

    if (SUCCEEDED(retVal))
    {
	retVal = pContext->QueryInterface(IID_IHXCommonClassFactory,
					  (void**) &m_pClassFactory);
    }

    return retVal;
}

STDMETHODIMP
MP4APayloadFormat::Reset()
{
    // Release all input packets we have stored
    FlushPackets(FLUSH_ALL_PACKETS);

    m_bFlushed = FALSE;
    m_bPriorLoss = FALSE;

#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
    if (m_pLATMDepack)
    {
	m_pLATMDepack->Reset();
    }
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */
    m_TSConverter.Reset();

    return HXR_OK;
}

STDMETHODIMP
MP4APayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal;

    if (pHeader)
    {
	m_pStreamHeader = pHeader;
	m_pStreamHeader->AddRef();
    }

    if (m_bPacketize)
    {
	retVal = SetPacketizerHeader(pHeader);
    }
    else
    {
	retVal = SetAssemblerHeader(pHeader);
    }

    return retVal;
}

HX_RESULT MP4APayloadFormat::SetPacketizerHeader(IHXValues* pHeader)
{
    return HXR_OK;
}

HX_RESULT MP4APayloadFormat::SetAssemblerHeader(IHXValues* pHeader)
{
    IHXBuffer* pMimeType = NULL;
    const char* pMimeTypeData = NULL;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pHeader)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pHeader->GetPropertyCString("MimeType", pMimeType);
    }

    if (SUCCEEDED(retVal))
    {
	pMimeTypeData = (char*) pMimeType->GetBuffer();

	retVal = HXR_FAIL;
	if (pMimeTypeData)
	{
	    retVal = HXR_OK;
	}
    }

    // Determine payload type here based on mime type
    if (SUCCEEDED(retVal))
    {
	if (strcasecmp(pMimeTypeData, "audio/X-RN-MP4-RAWAU") == 0)
	{
	    m_PayloadID = PYID_X_HX_MP4_RAWAU;
	}
	else if (strcasecmp(pMimeTypeData, "audio/X-RN-QT-RAWAU") == 0)
	{
	    m_PayloadID = PYID_X_HX_QT_RAWAU;
	}
	else if (strcasecmp(pMimeTypeData, "audio/mpeg4-simple-A2") == 0)
	{
	    m_PayloadID = PYID_MPEG4_SIMPLE_A2;
	}
	else if (strcasecmp(pMimeTypeData, "audio/MP4A-LATM") == 0)
	{
	    m_PayloadID = PYID_MP4A_LATM;
	}
	else if (strcasecmp(pMimeTypeData, "audio/X-HX-AAC-GENERIC") == 0)
	{
	    m_PayloadID = PYID_X_HX_AAC_GENERIC;
	}
	else if (strcasecmp(pMimeTypeData, "audio/X-HX-3GPP-QCELP") == 0)
	{
	    m_PayloadID = PYID_X_HX_3GPP_QCELP;
	}
	else
	{
	    retVal = HXR_FAIL;
	}
    }

    if (SUCCEEDED(retVal))
    {
	switch (m_PayloadID)
	{
	case PYID_X_HX_MP4_RAWAU:
	case PYID_X_HX_QT_RAWAU:	
	    retVal = SetAssemblerHXHeader(pHeader);
	    break;
	case PYID_MP4A_LATM:
	    retVal = SetAssemblerLATMHeader(pHeader);
	    break;
	case PYID_X_HX_AAC_GENERIC:
	    retVal = SetAssemblerAACGenericHeader(pHeader);
	    break;
        case PYID_X_HX_3GPP_QCELP:
            retVal = SetAssemblerQCELPHeader(pHeader);
            break;
	default:
	    retVal = HXR_NOTIMPL;
	    break;
	}
    }

    if (SUCCEEDED(retVal))
    {
	m_ulSamplesPerSecond = 0;
	m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond",
					    m_ulRTPSamplesPerSecond);
    }

    HX_RELEASE(pMimeType);

    return retVal;
}


HX_RESULT MP4APayloadFormat::SetAssemblerQCELPHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;
    // Extract OpaqueData to get decoder specific information in case of 'sqcp' descriptor
    return retVal;
}


HX_RESULT MP4APayloadFormat::SetAssemblerAACGenericHeader(IHXValues* pHeader)
{
    IHXBuffer *pBuffer = NULL;
    UCHAR *pBuf = NULL;
    HX_RESULT retVal = HXR_FAIL;

    retVal = m_pStreamHeader->GetPropertyBuffer("OpaqueData",
						pBuffer);
    if (SUCCEEDED(retVal))
    {
        m_pAudioConfig = NULL;
        m_ulAudioConfigSize = pBuffer->GetSize();
        if (m_ulAudioConfigSize)
        {
            pBuf = pBuffer->GetBuffer();
            m_unAudioConfigType = pBuf[0];
            if (--m_ulAudioConfigSize)
            {
                m_pAudioConfig = new UINT8[m_ulAudioConfigSize];
                if (!m_pAudioConfig)
                {
                    retVal = HXR_OUTOFMEMORY;
                }
                else
                {
                    memcpy(m_pAudioConfig, &pBuf[1], m_ulAudioConfigSize);
                }
            }
        }
        else
        {
            m_unAudioConfigType = 0; 
        }
    }
    HX_RELEASE(pBuffer);
    return retVal;
}

HX_RESULT MP4APayloadFormat::SetAssemblerHXHeader(IHXValues* pHeader)
{
    ES_Descriptor ESDesc;
    DecoderConfigDescriptor* pDCDesc = NULL;
    DecoderSpecifcInfo* pDSIDesc = NULL;
    IHXBuffer* pESDescriptor = NULL;

    UINT8* pESDescData;
    ULONG32 ulESDescSize;

    HX_RESULT retVal = HXR_OK;

    HX_VECTOR_DELETE(m_pAudioConfig);
    m_ulAudioConfigSize = 0;

    retVal = m_pStreamHeader->GetPropertyBuffer("OpaqueData",
						pESDescriptor);

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_INVALID_PARAMETER;
	if (pESDescriptor)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	pESDescData = pESDescriptor->GetBuffer();
	ulESDescSize = pESDescriptor->GetSize();

	retVal = ESDesc.Unpack(pESDescData, ulESDescSize);
    }

    HX_RELEASE(pESDescriptor);

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_FAIL;
	pDCDesc = ESDesc.m_pDecConfigDescr;

	if (pDCDesc)
	{
	    pDSIDesc = pDCDesc->m_pDecSpecificInfo;
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal) && pDSIDesc)
    {
	m_ulAudioConfigSize = pDSIDesc->m_ulLength;

	if (m_ulAudioConfigSize > 0)
	{
	    m_pAudioConfig = new UINT8 [m_ulAudioConfigSize];

	    if (m_pAudioConfig == NULL)
	    {
		m_ulAudioConfigSize = 0;
		retVal = HXR_OUTOFMEMORY;
	    }
	}
    }

    if (SUCCEEDED(retVal))
    {
        m_uObjectProfileIndication = ESDesc.m_pDecConfigDescr->m_uObjectProfileIndication;
	if (m_ulAudioConfigSize > 0)
	{
	    memcpy(m_pAudioConfig, pDSIDesc->m_pData, m_ulAudioConfigSize); /* Flawfinder: ignore */
	}
    }

    return retVal;
}

HX_RESULT MP4APayloadFormat::SetAssemblerLATMHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = SetAssemblerLATMConfig(pHeader);

    if (SUCCEEDED(retVal))
    {
	m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond",
					    m_ulRTPSamplesPerSecond);
    }

    return retVal;
}

HX_RESULT MP4APayloadFormat::SetAssemblerLATMConfig(IHXValues* pFMTParams)
{
    HX_RESULT retVal = HXR_FAIL;

#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
    retVal = HXR_OK;
    IHXBuffer* pConfigBuffer = NULL;
    ULONG32 ulObject = 0;
    ULONG32 ulProfileID = 0;
    ULONG32 ulBitrate = 0;
    ULONG32 bConfigPresent = TRUE;

    HX_ASSERT(pFMTParams);

    // Obtain Required parameters
    retVal = CHXMP4PayloadUtil::GetFMTPConfig(pFMTParams, m_pClassFactory,
					      pConfigBuffer);

    // Obtain optional parameters
    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	pFMTParams->GetPropertyULONG32("FMTPobject", ulObject);
	pFMTParams->GetPropertyULONG32("FMTPprofile-level-id", ulProfileID);
	if (SUCCEEDED(pFMTParams->GetPropertyULONG32("FMTPcpresent", ulVal)))
	{
	    bConfigPresent = (ulVal != 0);
	}
	pFMTParams->GetPropertyULONG32("FMTPbitrate", ulBitrate);
    }

    // Create and initialize the MP4A-LATM depacketizer
    if (SUCCEEDED(retVal))
    {
	HX_DELETE(m_pLATMDepack);
	m_pLATMDepack = new MP4LATMDepack;

	retVal = HXR_OUTOFMEMORY;
	if (m_pLATMDepack)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	UINT8* pStreamMuxConfig = NULL;
	ULONG32 ulMuxConfigSize = 0;
	const UINT8* pCodecConfig = NULL;
	ULONG32 ulConfigSize = 0;


	if (pConfigBuffer)
	{
	    pStreamMuxConfig = pConfigBuffer->GetBuffer();
	    ulMuxConfigSize = pConfigBuffer->GetSize();
	}

	retVal = HXR_FAIL;
	if (m_pLATMDepack->Init(ulProfileID,
				ulObject,
				ulBitrate,
				bConfigPresent,
				pStreamMuxConfig,
				ulMuxConfigSize,
				OnFrameCallback,
				this))
	{
	    retVal = HXR_OK;
	}

	if (SUCCEEDED(retVal))
	{
	    if (!m_pLATMDepack->GetCodecConfig(pCodecConfig,
					       ulConfigSize))
	    {
		pCodecConfig = NULL;
		ulConfigSize = 0;
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    m_ulAudioConfigSize = ulConfigSize;

	    if (m_ulAudioConfigSize > 0)
	    {
		m_pAudioConfig = new UINT8 [m_ulAudioConfigSize];

		if (m_pAudioConfig == NULL)
		{
		    m_ulAudioConfigSize = 0;
		    retVal = HXR_OUTOFMEMORY;
		}
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    if (m_ulAudioConfigSize > 0)
	    {
		memcpy(m_pAudioConfig, pCodecConfig, m_ulAudioConfigSize); /* Flawfinder: ignore */
	    }
	}
    }

    HX_RELEASE(pConfigBuffer);
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */

    return retVal;
}

STDMETHODIMP
MP4APayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pStreamHeader)
    {
	retVal = HXR_OK;
	pHeader = m_pStreamHeader;
	pHeader->AddRef();
    }

    return retVal;
}

STDMETHODIMP
MP4APayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pPacket);

    if (!m_bRTPPacketTested)
    {
	IHXRTPPacket* pRTPPacket = NULL;

	m_bUsesRTPPackets = (pPacket->QueryInterface(
				IID_IHXRTPPacket,
				(void**) &pRTPPacket)
			    == HXR_OK);

	m_bRTPPacketTested = TRUE;

	HX_RELEASE(pRTPPacket);

	if (m_bUsesRTPPackets)
	{
	    if (m_ulRTPSamplesPerSecond == 0)
	    {
		m_ulRTPSamplesPerSecond = m_ulSamplesPerSecond;
	    }
	}
	else
	{
	    m_ulRTPSamplesPerSecond = 1000; // RDT time stamp
	}

	m_TSConverter.SetBase(m_ulRTPSamplesPerSecond,
			      m_ulSamplesPerSecond);
    }

    // Add this packet to our list of input packets
    switch (m_PayloadID)
    {
    case PYID_X_HX_3GPP_QCELP:
    case PYID_X_HX_MP4_RAWAU:
    case PYID_X_HX_QT_RAWAU:		
    case PYID_X_HX_AAC_GENERIC:
	pPacket->AddRef();
	m_InputPackets.AddTail(pPacket);
	break;

    case PYID_MP4A_LATM:
	retVal = HXR_FAIL;
#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
	if (m_pLATMDepack)
	{
	    if (pPacket->IsLost())
	    {
		if (m_pLATMDepack->OnLoss(1))
		{
		    retVal = HXR_OK;
		}
	    }
	    else
	    {
		IHXBuffer* pBuffer = pPacket->GetBuffer();

		if (pBuffer)
		{
        if (m_pLATMDepack->OnPacket(GetPacketTime(pPacket),
					  pBuffer->GetBuffer(),
					  pBuffer->GetSize(),
            (pPacket->GetASMRuleNumber() % 2 == 1)))
        {		    
		        retVal = HXR_OK;
		    }

		    pBuffer->Release();
		}
	    }
	}
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */
	break;

    default:
	retVal = HXR_NOTIMPL;
	break;
    }

    return retVal;
}

STDMETHODIMP
MP4APayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    HX_RESULT retVal = HXR_OK;

    if (m_bPacketize)
    {
	retVal = GetPacketizerPacket(pOutPacket);
    }
    else
    {
	retVal = GetAssemblerPacket(pOutPacket);
    }

    return retVal;
}

HX_RESULT MP4APayloadFormat::GetPacketizerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_INCOMPLETE;

    if (!m_InputPackets.IsEmpty())
    {
	pOutPacket = (IHXPacket*) m_InputPackets.RemoveHead();
	retVal = HXR_OK;
    }
    else if (m_bFlushed)
    {
	retVal = HXR_STREAM_DONE;
    }

    return retVal;
}

HX_RESULT MP4APayloadFormat::GetAssemblerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

HX_RESULT MP4APayloadFormat::SetSamplingRate(ULONG32 ulSamplesPerSecond)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (!m_bRTPPacketTested)
    {
	m_ulSamplesPerSecond = ulSamplesPerSecond;
	retVal = HXR_OK;
    }

    return HXR_OK;
}

ULONG32 MP4APayloadFormat::GetTimeBase(void)
{
    return m_ulSamplesPerSecond;
}

HX_RESULT MP4APayloadFormat::SetAUDuration(ULONG32 ulAUDuration)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (!m_bRTPPacketTested)
    {
	switch (m_PayloadID)
	{
	case PYID_MP4A_LATM:
	    retVal = HXR_FAIL;
	    
#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
	    if (m_pLATMDepack &&
		m_pLATMDepack->SetFrameDuration(ulAUDuration))
	    {
		retVal = HXR_OK;
	    }
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */
	    break;
	    
	default:
	    retVal = HXR_NOTIMPL;
	    break;
	}
    }

    return retVal;
}

HX_RESULT MP4APayloadFormat::SetTimeAnchor(ULONG32 ulTimeMs)
{
    CTSConverter tempConverter;
    ULONG32 ulSamplesAnchor;
    ULONG32 ulRTPSamplesAnchor;

    tempConverter.SetBase(1000, m_ulSamplesPerSecond);
    ulSamplesAnchor = tempConverter.ConvertVector(ulTimeMs);

    tempConverter.SetBase(1000, m_ulRTPSamplesPerSecond);
    ulRTPSamplesAnchor = tempConverter.ConvertVector(ulTimeMs);

    m_TSConverter.SetAnchor(ulRTPSamplesAnchor, ulSamplesAnchor);

    return HXR_OK;
}

HX_RESULT MP4APayloadFormat::CreateMediaPacket(CMediaPacket* &pOutMediaPacket)
{
    HX_RESULT retVal = HXR_OK;

    switch (m_PayloadID)
    {
    case PYID_X_HX_AAC_GENERIC:
    case PYID_X_HX_MP4_RAWAU:
    case PYID_X_HX_QT_RAWAU:		
	retVal = CreateRawAUMediaPacket(pOutMediaPacket);
	break;

    case PYID_MP4A_LATM:
	if (m_OutMediaPacketQueue.IsEmpty())
	{
	    if (m_bFlushed)
	    {
		// We have used up all available input
		retVal = HXR_STREAM_DONE;
	    }
	    else
	    {
		// We don't have enough input
		// data to produce a packet
		retVal = HXR_INCOMPLETE;
	    }
	}
	else
	{
	    pOutMediaPacket = (CMediaPacket*) m_OutMediaPacketQueue.RemoveHead();
	}
	break;

    default:
	retVal = HXR_NOTIMPL;
	break;
    }

    return retVal;
}

HX_RESULT MP4APayloadFormat::OnFrame(ULONG32 ulTime,
				     const UINT8* pData,
				     ULONG32 ulSize,
				     HXBOOL bPreviousLoss)
{
    ULONG32 ulFlags = 0;
    CMediaPacket* pMediaPacket = NULL;
    HX_RESULT retVal = HXR_OK;

    if (pData && (ulSize != 0))
    {
	ULONG32 ulNewSize;

	if (bPreviousLoss)
	{
	    ulFlags |= MDPCKT_FOLLOWS_LOSS_FLAG;
	}

#ifdef _OVERALLOC_CODEC_DATA
	ulNewSize = ulSize + _OVERALLOC_CODEC_DATA;
#else	// _OVERALLOC_CODEC_DATA
	ulNewSize = ulSize;
#endif	// _OVERALLOC_CODEC_DATA

	UINT8* pNewData = new UINT8 [ulNewSize];

	if (pData)
	{
	    memcpy(pNewData, pData, ulSize); /* Flawfinder: ignore */

	    pMediaPacket = new CMediaPacket(pNewData,
					    pNewData,
					    ulNewSize,
					    ulSize,
					    ulTime,
					    ulFlags,
					    NULL);
	}
    }

    if (pMediaPacket)
    {
	m_OutMediaPacketQueue.AddTail(pMediaPacket);
    }

    return retVal;
}

void MP4APayloadFormat::OnFrameCallback(void* pUserData,
					ULONG32 ulTime,
					const UINT8* pData,
					ULONG32 ulSize,
					HXBOOL bPreviousLoss)
{
    MP4APayloadFormat* pObject = (MP4APayloadFormat*) pUserData;

    pObject->OnFrame(ulTime, pData, ulSize, bPreviousLoss);
}

HX_RESULT MP4APayloadFormat::CreateRawAUMediaPacket(CMediaPacket* &pOutMediaPacket)
{
    CMediaPacket* pMediaPacket;
    IHXPacket* pPacket = NULL;
    HX_RESULT retVal = HXR_OK;

    if (m_InputPackets.IsEmpty())
    {
	if (m_bFlushed)
	{
	    // We have used up all available input
	    retVal = HXR_STREAM_DONE;
	}
	else
	{
	    // We don't have enough input
	    // data to produce a packet
	    retVal = HXR_INCOMPLETE;
	}
    }
    else
    {
	ULONG32 ulFlags = MDPCKT_USES_IHXBUFFER_FLAG;
	IHXBuffer* pBuffer = NULL;
	pPacket = (IHXPacket*) m_InputPackets.RemoveHead();

	if (!pPacket->IsLost())
	{
	    pBuffer = pPacket->GetBuffer();

	    if (pBuffer)
	    {
		if (m_bPriorLoss)
		{
		    ulFlags |= MDPCKT_FOLLOWS_LOSS_FLAG;
		    m_bPriorLoss = FALSE;
		}

		pMediaPacket = BuildMediaPacket(pBuffer,
						GetPacketTime(pPacket),
						ulFlags);

		retVal = HXR_OUTOFMEMORY;
		if (pMediaPacket)
		{
		    pOutMediaPacket = pMediaPacket;
		    retVal = HXR_OK;
		}

		pBuffer->Release();
	    }
	    else
	    {
		m_bPriorLoss = TRUE;
		retVal = HXR_INCOMPLETE;
	    }

	    pPacket->Release();
	}
	else
	{
	    m_bPriorLoss = TRUE;
	    retVal = HXR_INCOMPLETE;
	}
    }

    return retVal;
}


CMediaPacket* MP4APayloadFormat::BuildMediaPacket(IHXBuffer* pBuffer,
						  ULONG32 ulTime,
						  ULONG32 ulFlags)
{
    CMediaPacket* pMediaPacket = NULL;
#ifdef _OVERALLOC_CODEC_DATA
    UINT8* pData = new UINT8 [pBuffer->GetSize() + _OVERALLOC_CODEC_DATA];

    if (pData)
    {
	memcpy(pData, pBuffer->GetBuffer(), pBuffer->GetSize()); /* Flawfinder: ignore */

	ulFlags &= (~MDPCKT_USES_IHXBUFFER_FLAG);
	pMediaPacket = new CMediaPacket(pData,
					pData,
					pBuffer->GetSize() + 3,
					pBuffer->GetSize(),
					ulTime,
					ulFlags,
					NULL);
    }
#else	// _OVERALLOC_CODEC_DATA
    pMediaPacket = new CMediaPacket(pBuffer,
				    pBuffer->GetBuffer(),
				    pBuffer->GetSize(),
				    pBuffer->GetSize(),
				    ulTime,
				    ulFlags,
				    NULL);

#endif	// _OVERALLOC_CODEC_DATA

    return pMediaPacket;
}

STDMETHODIMP
MP4APayloadFormat::Flush()
{
#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
    if (m_pLATMDepack)
    {
	m_pLATMDepack->Flush();
    }
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */
    m_bFlushed = TRUE;

    return HXR_OK;
}

void MP4APayloadFormat::FlushPackets(ULONG32 ulCount)
{
    IHXPacket* pDeadPacket;
    CMediaPacket* pDeadMediaPacket;

    while ((ulCount > 0) && (!m_InputPackets.IsEmpty()))
    {
	pDeadPacket = (IHXPacket*) m_InputPackets.RemoveHead();
	HX_RELEASE(pDeadPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }

    while ((ulCount > 0) && (!m_OutMediaPacketQueue.IsEmpty()))
    {
	pDeadMediaPacket = (CMediaPacket*) m_OutMediaPacketQueue.RemoveHead();
	HX_DELETE(pDeadMediaPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }
}

ULONG32 MP4APayloadFormat::GetPacketTime(IHXPacket* pPacket)
{
    ULONG32 ulTime;

    HX_ASSERT(pPacket);

    if (m_bUsesRTPPackets)
    {
	ulTime = ((IHXRTPPacket*) pPacket)->GetRTPTime();
    }
    else
    {
	ulTime = pPacket->GetTime();
    }

    ulTime = m_TSConverter.Convert(ulTime);

    return ulTime;
}
