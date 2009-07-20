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
// #define _APPEND_BITSTREAM_HEADER
// #define _DONOT_SEGMENT
// #define _ASSERT_ON_LOSS
// #define _DUMP_FIRST_NFRAMES 5
#define _OVERALLOC_CODEC_DATA	3

#define MP4V_RN_PAYLOAD_MIME_TYPE	    "video/X-RN-MP4"
#define MP4V_3016_PAYLOAD_MIME_TYPE	    "video/MP4V-ES"
#define MP4V_RN_3GPP_H263_PAYLOAD_MIME_TYPE "video/X-RN-3GPP-H263"
#define MP4V_HX_AVC1_PAYLOAD_MIME_TYPE	    "video/X-HX-AVC1"
#define MP4V_HX_DIVX_PAYLOAD_MIME_TYPE      "video/X-HX-DIVX"

#define FLUSH_ALL_PACKETS   0xFFFFFFFF

#define MAX_FRAME_SEGMENTS	1024
#define NUM_OVERLAP_SEGMENTS	256

#define DLFT_MAX_PACKET_DATA_SIZE   0x7FFFFFFF

#define CHAR_LF	0x0a
#define CHAR_CR	0x0d

#define MAX_INT_TEXT_LENGTH	10


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
#include "hxpacketflags.h"

#include "sdptools.h"
#include "mp4desc.h"
#include "mp4vpyld.h"
#include "mp4pyldutil.h"
#include "avcconfig.h"
#include "pckunpck.h"
#include "hxver.h"
#include "mp4payload.ver"

const char* const MP4VPayloadFormat::m_ppszMPEG4VideoCodecID[] = {
#ifdef HELIX_FEATURE_OMX_VIDEO_DECODER_MP4
                                                                    "OMXV",
#endif
                                                                    "MP4V", NULL};

const char* const MP4VPayloadFormat::m_ppszAVCCodecID[]        = {
#ifdef HELIX_FEATURE_OMX_VIDEO_DECODER_AVC1
                                                                    "OMXV",
#endif
                                                                    "AVC1", "AVCQ", NULL};

const char* const MP4VPayloadFormat::zm_pDescription    = "RealNetworks MPEG4 Video Packetizer Plugin";
const char* const MP4VPayloadFormat::zm_pCopyright      = HXVER_COPYRIGHT;
const char* const MP4VPayloadFormat::zm_pMoreInfoURL    = HXVER_MOREINFO;
const char* const zm_pMimeType    = "video/MP4V-ES";

MP4VPayloadFormat::MP4VPayloadFormat(CHXBufferMemoryAllocator* pAllocator)
    : m_lRefCount	(0)
    , m_pClassFactory	(NULL)
    , m_pAllocator	(pAllocator)
    , m_pStreamHeader	(NULL)
    , m_pBitstreamHeader	(NULL)
    , m_ulBitstreamHeaderSize	(0)
    , m_ulSamplesPerSecond(1000)
    , m_ulFrameCount	(0)
    , m_uSeqNumber	(0)
    , m_bPictureStarted	(FALSE)
    , m_ulMaxPacketDataSize(DLFT_MAX_PACKET_DATA_SIZE)
    , m_bFlushed	(FALSE)
    , m_bFirstPacket	(TRUE)
    , m_bFirstFrame	(TRUE)
    , m_bUsesRTPPackets (FALSE)
    , m_bRTPPacketTested(FALSE)
    , m_bPacketize	(FALSE)
    , m_ulCodecIDIndex(0)
    , m_ppszCodecID(NULL)
    , m_PayloadID	(PYID_UNKNOWN)
    , m_pContext(NULL)
{
    if (m_pAllocator)
    {
	m_pAllocator->AddRef();
    }
}

MP4VPayloadFormat::~MP4VPayloadFormat()
{
    FlushPackets(FLUSH_ALL_PACKETS);
    FlushOutputPackets(FLUSH_ALL_PACKETS);

    HX_VECTOR_DELETE(m_pBitstreamHeader);
    if (m_pAllocator)
    {
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);
    HX_RELEASE(m_pContext);
}

HX_RESULT MP4VPayloadFormat::Build(REF(IMP4VPayloadFormat*) pFmt)
{
    pFmt = new MP4VPayloadFormat();

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
MP4VPayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
	{ GET_IIDHANDLE(IID_IHXPluginProperties), (IHXPluginProperties*) this },
	{ GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*) this },
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
MP4VPayloadFormat::AddRef()
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
MP4VPayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT STDAPICALLTYPE 
MP4VPayloadFormat::HXCreateInstance(IUnknown** ppIUnknown)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*) new MP4VPayloadFormat();
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}

HX_RESULT STDAPICALLTYPE 
MP4VPayloadFormat::CanUnload(void)
{
    return CanUnload2();
}

HX_RESULT STDAPICALLTYPE 
MP4VPayloadFormat::CanUnload2(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

/************************************************************************
 *  IHXPlugin methods
 */
/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP 
MP4VPayloadFormat::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pContext);

    if(!pContext)
    {
        return HXR_INVALID_PARAMETER;
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    HX_RELEASE(m_pClassFactory);
    retVal = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
					 (void**) &m_pClassFactory);

    return retVal;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP MP4VPayloadFormat::GetPluginInfo
(
    REF(HXBOOL)         bLoadMultiple,
    REF(const char*)    pDescription,
    REF(const char*)    pCopyright,
    REF(const char*)    pMoreInfoURL,
    REF(ULONG32)        ulVersionNumber
)
{
    bLoadMultiple   = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright      = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXPluginProperties::GetProperties
 */

STDMETHODIMP
MP4VPayloadFormat::GetProperties(REF(IHXValues*) pIHXValuesProperties)
{
    HX_RESULT res = HXR_FAIL;
    HX_ASSERT(m_pClassFactory);
    if(!m_pClassFactory)
    {
        return res;
    }
    m_pClassFactory->CreateInstance(IID_IHXValues, (void**)&pIHXValuesProperties);

    if (pIHXValuesProperties)
    {
        //plugin class
        res = SetCStringPropertyCCF(pIHXValuesProperties, PLUGIN_CLASS,
                                    PLUGIN_PACKETIZER_TYPE, m_pContext);
        if (HXR_OK != res)
        {
            return res;
        }

        //mime type
        res = SetCStringPropertyCCF(pIHXValuesProperties, PLUGIN_PACKETIZER_MIME,
                                    zm_pMimeType, m_pContext); 
        if (HXR_OK != res)
        {
            return res;
        }
    }
    else
    {
        return HXR_OUTOFMEMORY;
    }
    return HXR_OK;
}


STDMETHODIMP
MP4VPayloadFormat::Init(IUnknown* pContext,
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
MP4VPayloadFormat::Reset()
{
    // Release all input packets we have stored
    FlushPackets(FLUSH_ALL_PACKETS);
    FlushOutputPackets(FLUSH_ALL_PACKETS);

    m_bFlushed = FALSE;
    m_bFirstPacket = TRUE;
    m_bFirstFrame = TRUE;
    m_bPictureStarted = FALSE;
    m_bUsesRTPPackets = FALSE;
    m_bRTPPacketTested = FALSE;
    m_ulFrameCount = 0;
    m_uSeqNumber = 0;

    m_TSConverter.Reset();

    return HXR_OK;
}

STDMETHODIMP
MP4VPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pHeader);

    HX_RELEASE(m_pStreamHeader);
    m_pStreamHeader = pHeader;
    if (m_pStreamHeader)
    {
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

void MP4VPayloadFormat::SetAllocator(CHXBufferMemoryAllocator*	pAllocator)
{
    if (pAllocator)
    {
        m_pAllocator = pAllocator; 
        m_pAllocator->AddRef();
    }
}

HX_RESULT MP4VPayloadFormat::SetPacketizerHeader(IHXValues* pHeader)
{
    IHXBuffer* pMimeType = NULL;
    const char* pMimeTypeData = NULL;
    HX_RESULT retVal = HXR_OK;

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

    if (SUCCEEDED(retVal))
    {
	if (strcasecmp(pMimeTypeData, MP4V_3016_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_MP4V_ES;
	    if (DLFT_MAX_PACKET_DATA_SIZE != m_ulMaxPacketDataSize)
	    {
		retVal = pHeader->SetPropertyULONG32
		    ("MaxPacketSize", m_ulMaxPacketDataSize);
	    }
	}
	else if (strcasecmp(pMimeTypeData, MP4V_RN_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_X_HX_MP4;
	}
	else if (strcasecmp(pMimeTypeData, MP4V_RN_3GPP_H263_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_X_HX_3GPP_H263;
	}
	else if (strcasecmp(pMimeTypeData, MP4V_HX_AVC1_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_X_HX_AVC1;
	}
	else
	{
	    retVal = HXR_FAIL;
	}
    }

    HX_RELEASE(pMimeType);

    return retVal;
}

HX_RESULT MP4VPayloadFormat::SetAssemblerHeader(IHXValues* pHeader)
{
    IHXBuffer* pMimeType = NULL;
    const char* pMimeTypeData = NULL;
    HX_RESULT retVal = HXR_OK;

    HX_VECTOR_DELETE(m_pBitstreamHeader);
    m_ulBitstreamHeaderSize = 0;

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

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_FAIL;

        m_ppszCodecID = (const char**) m_ppszMPEG4VideoCodecID;

	if (strcasecmp(pMimeTypeData, MP4V_3016_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_MP4V_ES;
	    retVal = SetAssembler3016Header(pHeader);
	}
	else if (strcasecmp(pMimeTypeData, MP4V_RN_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_X_HX_MP4;
	    retVal = SetAssemblerHXHeader(pHeader);
	}
	else if (strcasecmp(pMimeTypeData, MP4V_RN_3GPP_H263_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_X_HX_3GPP_H263;
	    retVal = SetAssemblerHX3GPPH263Header(pHeader);
	}
	else if (strcasecmp(pMimeTypeData, MP4V_HX_AVC1_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_X_HX_AVC1;
            m_ppszCodecID = (const char**) m_ppszAVCCodecID;
	    retVal = SetAssemblerHX3GPPH264Header(pHeader);
	}
	else if (strcasecmp(pMimeTypeData, MP4V_HX_DIVX_PAYLOAD_MIME_TYPE) == 0)
	{
	    m_PayloadID = PYID_X_HX_DIVX;
	    retVal = SetAssemblerHXAVIHeader(pHeader);
	}
    }

    if (SUCCEEDED(retVal))
    {
	m_ulSamplesPerSecond = 0;
	m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond",
					    m_ulSamplesPerSecond);
    }

    HX_RELEASE(pMimeType);

    return retVal;
}

HX_RESULT MP4VPayloadFormat::SetAssembler3016Header(IHXValues* pHeader)
{
    IHXBuffer* pConfigBuffer = NULL;
    HX_RESULT retVal = HXR_NO_DATA;

    retVal = CHXMP4PayloadUtil::GetFMTPConfig(pHeader, m_pClassFactory,
					      pConfigBuffer);

    if (retVal == HXR_OK)
    {
	m_ulBitstreamHeaderSize = pConfigBuffer->GetSize();

	if (m_ulBitstreamHeaderSize > 0)
	{
	    m_pBitstreamHeader = new UINT8 [m_ulBitstreamHeaderSize];

	    if (m_pBitstreamHeader)
	    {
		memcpy(m_pBitstreamHeader, pConfigBuffer->GetBuffer(),
		       m_ulBitstreamHeaderSize);
	    }
	    else
	    {
		m_ulBitstreamHeaderSize = 0;
		retVal = HXR_OUTOFMEMORY;
	    }
	}
    }

    HX_RELEASE(pConfigBuffer);

    return retVal;
}

HX_RESULT MP4VPayloadFormat::SetAssemblerHXHeader(IHXValues* pHeader)
{
    ES_Descriptor ESDesc;
    DecoderConfigDescriptor* pDCDesc = NULL;
    DecoderSpecifcInfo* pDSIDesc = NULL;
    IHXBuffer* pESDescriptor = NULL;

    UINT8* pESDescData;
    ULONG32 ulESDescSize;

    HX_RESULT retVal = HXR_OK;

    HX_VECTOR_DELETE(m_pBitstreamHeader);
    m_ulBitstreamHeaderSize = 0;

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
	m_ulBitstreamHeaderSize = pDSIDesc->m_ulLength;

	if (m_ulBitstreamHeaderSize > 0)
	{
	    m_pBitstreamHeader = new UINT8 [m_ulBitstreamHeaderSize];

	    if (m_pBitstreamHeader == NULL)
	    {
		m_ulBitstreamHeaderSize = 0;
		retVal = HXR_OUTOFMEMORY;
	    }
	}
    }

    if (SUCCEEDED(retVal))
    {
	if (m_ulBitstreamHeaderSize > 0)
	{
	    memcpy(m_pBitstreamHeader, pDSIDesc->m_pData, m_ulBitstreamHeaderSize); /* Flawfinder: ignore */
	}
    }

    return retVal;
}

HX_RESULT MP4VPayloadFormat::SetAssemblerHX3GPPH263Header(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    // We'll let H263 decoder initialize with in-band bitstream
    HX_VECTOR_DELETE(m_pBitstreamHeader);
    m_ulBitstreamHeaderSize = 0;

    return retVal;
}

HX_RESULT MP4VPayloadFormat::SetAssemblerHX3GPPH264Header(IHXValues* pHeader)
{
    AVCConfigurationBox avcConfigBox;
    HX_RESULT retVal = HXR_OK;
    IHXBuffer* pAVCConfigurationBoxBuffer = NULL;

    retVal = m_pStreamHeader->GetPropertyBuffer("OpaqueData",
						pAVCConfigurationBoxBuffer);

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_INVALID_PARAMETER;
	if (pAVCConfigurationBoxBuffer)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	UINT8* pData = pAVCConfigurationBoxBuffer->GetBuffer();
	UINT32 ulSize = pAVCConfigurationBoxBuffer->GetSize();
	retVal = avcConfigBox.Unpack(pData, ulSize);
    }

    if (SUCCEEDED(retVal))
    {
	m_pBitstreamHeader = avcConfigBox.m_pAVCDecoderConfigurationRecord->m_pData;
	m_ulBitstreamHeaderSize = avcConfigBox.m_pAVCDecoderConfigurationRecord->m_ulLength;
	avcConfigBox.m_pAVCDecoderConfigurationRecord->m_pData = NULL;
	avcConfigBox.m_pAVCDecoderConfigurationRecord->m_ulLength = 0;
    }

    HX_RELEASE(pAVCConfigurationBoxBuffer);

    retVal = HXR_OK;
    return retVal;
}

HX_RESULT MP4VPayloadFormat::SetAssemblerHXAVIHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    // We'll let decoder initialize with in-band bitstream
    HX_VECTOR_DELETE(m_pBitstreamHeader);
    m_ulBitstreamHeaderSize = 0;

    return retVal;
}

STDMETHODIMP
MP4VPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_ASSERT(m_pStreamHeader);
    pHeader = m_pStreamHeader;
    pHeader->AddRef();

    return HXR_OK;
}


STDMETHODIMP
MP4VPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal;

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
	    HX_ASSERT(m_ulSamplesPerSecond != 0);

	    m_TSConverter.SetBase(m_ulSamplesPerSecond,
				  1000);
	}
	else
	{
	    m_TSConverter.SetBase(1000, 1000);
	}
    }

    // Add this packet to our list of input packets
    pPacket->AddRef();
    m_InputPackets.AddTail(pPacket);

    if (m_bPacketize)
    {
	retVal = SetPacketizerPacket(pPacket);
    }
    else
    {
	retVal = SetAssemblerPacket(pPacket);
    }

    return retVal;
}

HX_RESULT MP4VPayloadFormat::SetPacketizerPacket(IHXPacket* pPacket)
{
    if (m_bFirstFrame)
    {
	m_bFirstFrame = FALSE;
    }

    return HXR_OK;
}

HX_RESULT MP4VPayloadFormat::SetAssemblerPacket(IHXPacket* pPacket)
{
    HXBOOL bNewPictureStart;

    if (!pPacket->IsLost())
    {
	bNewPictureStart = IsPictureStart(pPacket);

	if (m_bFirstPacket)
	{
	    m_bFirstPacket = FALSE;
	    m_ulFrameTime = GetPacketTime(pPacket);
	}

	if ((GetPacketTime(pPacket) != m_ulFrameTime) ||
	    (bNewPictureStart && m_bPictureStarted))
	{
	    m_ulFrameCount++;
	    m_ulFrameTime = GetPacketTime(pPacket);
	}

	if (pPacket->GetASMRuleNumber() == 1)
	{
	    m_ulFrameCount++;
	    m_bFirstPacket = TRUE;
	    m_bPictureStarted = FALSE;
	}
	else if (!m_bPictureStarted)
	{
	    m_bPictureStarted = bNewPictureStart;
	}
    }

    return HXR_OK;
}


STDMETHODIMP
MP4VPayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
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
	if (m_bPacketize)
	{
	    retVal = GetPacketizerPacket(pOutPacket);
	}
	else
	{
	    retVal = GetAssemblerPacket(pOutPacket);
	}
    }

    return retVal;
}

HX_RESULT MP4VPayloadFormat::GetPacketizerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_INCOMPLETE;

    if (m_OutputPackets.GetCount() > 0)
    {
	pOutPacket = (IHXPacket*) m_OutputPackets.RemoveHead();
	retVal = HXR_OK;
    }

    if (retVal == HXR_INCOMPLETE)
    {
	IHXPacket* pRawPacket = NULL;

	do
	{
	    HX_RELEASE(pRawPacket);
	    retVal = GetRawPacketizerPacket(pRawPacket);
	} while ((retVal == HXR_OK) && (!IsValidPacket(pRawPacket)));

	if (retVal == HXR_OK)
	{
	    retVal = FragmentPacket(pRawPacket);
	    pRawPacket->Release();
	}

	if (retVal == HXR_OK)
	{
	    HX_ASSERT(m_OutputPackets.GetCount() > 0);

	    retVal = HXR_INCOMPLETE;
	    if (m_OutputPackets.GetCount() > 0)
	    {
		pOutPacket = (IHXPacket*) m_OutputPackets.RemoveHead();
		retVal = HXR_OK;
	    }
	}
    }

    return retVal;
}


HX_RESULT MP4VPayloadFormat::FragmentPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_OK;
    IHXBuffer* pPacketBuffer = pPacket->GetBuffer();
    ULONG32 ulRemainingSize = pPacketBuffer->GetSize();

    if (ulRemainingSize > m_ulMaxPacketDataSize)
    {
	UINT8* pRemainingData = pPacketBuffer->GetBuffer();
	ULONG32 ulFragmentSize = m_ulMaxPacketDataSize;
	IHXPacket* pNewPacket = NULL;
	IHXBuffer* pNewBuffer = NULL;
	IHXRTPPacket* pRTPPacket = NULL;

	if (m_bUsesRTPPackets)
	{
	    if (SUCCEEDED(retVal))
	    {
		retVal = pPacket->QueryInterface(IID_IHXRTPPacket,
		    (void**) &pRTPPacket);
	    }
	}

	do
	{
	    // Create new packet
	    if (SUCCEEDED(retVal))
	    {
		if (m_bUsesRTPPackets)
		{
		    retVal = m_pClassFactory->CreateInstance(CLSID_IHXRTPPacket,
							     (void**) &pNewPacket);
		}
		else
		{
		    retVal = m_pClassFactory->CreateInstance(CLSID_IHXPacket,
							     (void**) &pNewPacket);
		}
	    }

	    // Create new buffer
	    if (SUCCEEDED(retVal))
	    {
		retVal = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
							 (void**) &pNewBuffer);
	    }

	    // Set Buffer
	    if (SUCCEEDED(retVal))
	    {
		retVal = pNewBuffer->Set(pRemainingData, ulFragmentSize);
	    }

	    // Set the new packet
	    if (SUCCEEDED(retVal))
	    {
		if (SUCCEEDED(retVal))
		{
		    if (m_bUsesRTPPackets)
		    {
			retVal = ((IHXRTPPacket*) pNewPacket)->SetRTP(
			    pNewBuffer,
			    pRTPPacket->GetTime(),
			    pRTPPacket->GetRTPTime(),
			    pRTPPacket->GetStreamNumber(),
			    pRTPPacket->GetASMFlags(),
			    (ulRemainingSize > m_ulMaxPacketDataSize) ?
				0 : pRTPPacket->GetASMRuleNumber());
		    }
		    else
		    {
			retVal = ((IHXRTPPacket*) pNewPacket)->Set(
			    pNewBuffer,
			    pPacket->GetTime(),
			    pPacket->GetStreamNumber(),
			    pPacket->GetASMFlags(),
			    (ulRemainingSize > m_ulMaxPacketDataSize) ?
				0 : pPacket->GetASMRuleNumber());
		    }
		}
	    }

	    pRemainingData += ulFragmentSize;
	    ulRemainingSize -= ulFragmentSize;
	    if (ulRemainingSize < ulFragmentSize)
	    {
		ulFragmentSize = ulRemainingSize;
	    }

	    if (SUCCEEDED(retVal))
	    {
		pNewPacket->AddRef();
		m_OutputPackets.AddTail(pNewPacket);
	    }

	    HX_RELEASE(pNewBuffer);
	    HX_RELEASE(pNewPacket);
	} while (SUCCEEDED(retVal) && (ulRemainingSize > 0));

	HX_RELEASE(pRTPPacket);
    }
    else
    {
	pPacket->AddRef();
	m_OutputPackets.AddTail(pPacket);
    }

    HX_RELEASE(pPacketBuffer);

    return retVal;
}


HX_RESULT MP4VPayloadFormat::GetRawPacketizerPacket(IHXPacket* &pOutPacket)
{
    IHXPacket* pPacket;
    IHXPacket* pAfterPacket = NULL;
    IHXRTPPacket* pRTPPacket = NULL;
    UINT16 uASMRuleNumber;
    HXBOOL bMarker = TRUE;
    HXBOOL bNewPictureStart = TRUE;
    HX_RESULT retVal = HXR_INCOMPLETE;

    if ((m_InputPackets.GetCount() > 1) ||
	m_bFlushed)
    {
	retVal = HXR_OK;
	pPacket = (IHXPacket*) m_InputPackets.RemoveHead();

	if (!m_bPictureStarted)
	{
	    m_bPictureStarted = IsPictureStart(pPacket);
	}

	if (m_bFirstPacket)
	{
	    m_bFirstPacket = FALSE;
	}

	// Determine marker usage
	if (m_InputPackets.GetCount() > 0)
	{
	    pAfterPacket = (IHXPacket*) m_InputPackets.GetHead();
	}

	if (pAfterPacket)
	{
	    bNewPictureStart = IsPictureStart(pAfterPacket);

	    bMarker = ((GetPacketTime(pPacket) != GetPacketTime(pAfterPacket)) ||
		       (bNewPictureStart && m_bPictureStarted));

	    if (bMarker)
	    {
		m_bPictureStarted = bNewPictureStart;
	    }
	}
	else
	{
	    bMarker = m_bFlushed;
	}


	uASMRuleNumber = bMarker ? 1 : 0;

	// If rule number must change, create new packet
	if (uASMRuleNumber != pPacket->GetASMRuleNumber())
	{
	    IHXPacket* pNewPacket = NULL;

	    if (m_bUsesRTPPackets)
	    {
		retVal = m_pClassFactory->CreateInstance(CLSID_IHXRTPPacket,
							 (void**) &pNewPacket);

		if (SUCCEEDED(retVal))
		{
		    retVal = pPacket->QueryInterface(IID_IHXRTPPacket,
						     (void**) &pRTPPacket);
		}
	    }
	    else
	    {
		retVal = m_pClassFactory->CreateInstance(CLSID_IHXPacket,
							 (void**) &pNewPacket);
	    }

	    if (SUCCEEDED(retVal))
	    {
		IHXBuffer* pBuffer = pPacket->GetBuffer();

		if (m_bUsesRTPPackets)
		{
		    retVal = ((IHXRTPPacket*) pNewPacket)->SetRTP(
			pBuffer,
			pRTPPacket->GetTime(),
			pRTPPacket->GetRTPTime(),
			pRTPPacket->GetStreamNumber(),
			pRTPPacket->GetASMFlags(),
			uASMRuleNumber);
		}
		else
		{
		    retVal = ((IHXRTPPacket*) pNewPacket)->Set(
			pBuffer,
			pPacket->GetTime(),
			pPacket->GetStreamNumber(),
			pPacket->GetASMFlags(),
			uASMRuleNumber);
		}

		HX_RELEASE(pBuffer);
	    }

	    pPacket->Release();
	    pPacket = NULL;

	    if (SUCCEEDED(retVal))
	    {
		pPacket = pNewPacket;
		pNewPacket = NULL;
	    }

	    HX_RELEASE(pNewPacket);
	    HX_RELEASE(pRTPPacket);
	}

	if (SUCCEEDED(retVal))
	{
	    pOutPacket = pPacket;
	}
    }

    return retVal;
}

HX_RESULT MP4VPayloadFormat::GetAssemblerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

void MP4VPayloadFormat::FlushPackets(ULONG32 ulCount)
{
    IHXPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_InputPackets.IsEmpty()))
    {
	pDeadPacket = (IHXPacket*) m_InputPackets.RemoveHead();
	HX_RELEASE(pDeadPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }
}

void MP4VPayloadFormat::FlushOutputPackets(ULONG32 ulCount)
{
    IHXPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_OutputPackets.IsEmpty()))
    {
	pDeadPacket = (IHXPacket*) m_OutputPackets.RemoveHead();
	HX_RELEASE(pDeadPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }
}

ULONG32 MP4VPayloadFormat::CountValidPackets(ULONG32 ulCount)
{
    IHXPacket* pPacket;
    LISTPOSITION listPos;
    ULONG32 ulValidCount = 0;

    listPos = m_InputPackets.GetHeadPosition();

    while ((ulCount > 0) && (listPos != NULL))
    {
	pPacket = (IHXPacket*) m_InputPackets.GetNext(listPos);
	HX_ASSERT(pPacket);
	if (!pPacket->IsLost())
	{
	    ulValidCount++;
	}

	ulCount--;
    }

    return ulValidCount;
}

ULONG32 MP4VPayloadFormat::SumPacketSizes(ULONG32 ulCount)
{
    IHXPacket* pPacket;
    IHXBuffer* pBuffer;
    LISTPOSITION listPos;
    ULONG32 ulSize = 0;

    listPos = m_InputPackets.GetHeadPosition();

    while ((ulCount > 0) && (listPos != NULL))
    {
	pPacket = (IHXPacket*) m_InputPackets.GetNext(listPos);
	HX_ASSERT(pPacket);
	if (!pPacket->IsLost())
	{
	    pBuffer = pPacket->GetBuffer();

	    if (pBuffer)
	    {
		ulSize += pBuffer->GetSize();
		pBuffer->Release();
	    }
	}

	ulCount--;
    }

    return ulSize;
}

HX_RESULT MP4VPayloadFormat::CreateHXCodecPacket(UINT32* &pHXCodecDataOut)
{
    HX_RESULT retVal = HXR_INCOMPLETE;

    if (m_ulFrameCount > 0)
    {
	// Compute frame size and count segments
	ULONG32 ulFrameSize = 0;
	ULONG32 ulSegmentCount = 0;
	ULONG32 ulSegmentSize;
	ULONG32 ulFrameTime = 0;
	ULONG32 ulValidSegmentCount = 0;
	IHXPacket* pPacket;
	IHXBuffer* pBuffer = NULL;
	LISTPOSITION tentativeListPos;
	LISTPOSITION listPos;
	ULONG32 ulIdx;
	HXBOOL bIsLost;
	HXBOOL bPictureStarted = FALSE;
	HXBOOL bNewPictureStart;
	HXCODEC_DATA* pHXCodecData = NULL;
	HXCODEC_SEGMENTINFO* pHXCodecSegmentInfo;
	ULONG32* pData = NULL;

	retVal = HXR_OK;

	// Gather Frame Information - until valid frame detected
	do
	{
	    listPos = m_InputPackets.GetHeadPosition();
            if (listPos == NULL)
            {
                return HXR_OUTOFMEMORY;
            }

	    do
	    {
		if (ulSegmentCount >= MAX_FRAME_SEGMENTS)
		{
		    HX_ASSERT(pPacket);

		    FlushPackets(ulSegmentCount - NUM_OVERLAP_SEGMENTS);

		    ulSegmentCount = NUM_OVERLAP_SEGMENTS;
		    ulFrameSize = SumPacketSizes(NUM_OVERLAP_SEGMENTS);
		    ulValidSegmentCount = CountValidPackets(NUM_OVERLAP_SEGMENTS);
		}

		// Get Next below will advance the list position.
		// Consider this list position tentative until we decided
		// to include that list element into the current frame.
		tentativeListPos = listPos;
		pPacket = (IHXPacket*) m_InputPackets.GetNext(tentativeListPos);

		if (!pPacket->IsLost())
		{
		    bNewPictureStart = IsPictureStart(pPacket);

		    if (ulValidSegmentCount > 0)
		    {
			if ((GetPacketTime(pPacket) != ulFrameTime) ||
			    (bPictureStarted && bNewPictureStart))
			{
			    break;
			}
		    }
		    else
		    {
			ulFrameTime = GetPacketTime(pPacket);
		    }

		    if (!bPictureStarted)
		    {
			bPictureStarted = bNewPictureStart;
		    }

		    ulValidSegmentCount++;

		    pBuffer = pPacket->GetBuffer();
		}

		if (pBuffer)
		{
		    ulFrameSize += pBuffer->GetSize();
		    pBuffer->Release();
		    pBuffer = NULL;
		}

		ulSegmentCount++;
		// Since we are including the current packet into the frame,
		// make the tentative list position permanent.
		listPos = tentativeListPos;
	    } while ((pPacket->GetASMRuleNumber() != 1) && (listPos != NULL));

	    m_ulFrameCount--;

	    // If the entire list has been exhausted, this should mean that
	    // last packet terminated the last frame.
	    // This translates into last frame being not lost
	    // and assembler having no more frames left to process.
	    // Reset frame count to restore sane state if this is
	    // not the case.
	    if (listPos == NULL)
	    {
		HX_ASSERT(!pPacket->IsLost());
		HX_ASSERT(m_ulFrameCount == 0);
		m_ulFrameCount = 0;
	    }
	} while ((ulValidSegmentCount == 0) &&
		 (m_ulFrameCount > 0));

	if (ulValidSegmentCount == 0)
	{
	    retVal = HXR_INCOMPLETE;
	}

	// Allocate codec data header
	if (retVal == HXR_OK)
	{
	    HX_ASSERT(ulSegmentCount != 0);
	    HX_ASSERT(ulValidSegmentCount != 0);

#ifdef _APPEND_BITSTREAM_HEADER
	    if (m_bFirstFrame)
	    {
		ulFrameSize += m_ulBitstreamHeaderSize;
	    }
#endif	// _APPEND_BITSTREAM_HEADER

	    pData = new ULONG32[(sizeof(HXCODEC_DATA) +
			         sizeof(HXCODEC_SEGMENTINFO) *
			         (ulSegmentCount - 1)) / 4 + 1];

	    retVal = HXR_OUTOFMEMORY;
	    if (pData)
	    {
		retVal = HXR_OK;
	    }
	}

	// Init. codec data header
	if (retVal == HXR_OK)
	{
	    pHXCodecData = (HXCODEC_DATA*) pData;

	    pHXCodecData->dataLength = ulFrameSize;
	    pHXCodecData->timestamp = ulFrameTime;
	    pHXCodecData->sequenceNum = m_uSeqNumber;
	    pHXCodecData->flags = 0;
	    pHXCodecData->lastPacket = FALSE;
	    pHXCodecData->numSegments = ulSegmentCount;
	    pHXCodecData->data = NULL;

	    if ((ulFrameSize > 0)
#ifndef _OVERALLOC_CODEC_DATA
		&&
		(
		 (ulValidSegmentCount > 1) ||
		 (m_pAllocator == NULL)
#ifdef _APPEND_BITSTREAM_HEADER
		 || m_bFirstFrame
#endif	// _APPEND_BITSTREAM_HEADER
	        )
#endif	// _OVERALLOC_CODEC_DATA
	       )
	    {
#ifdef _OVERALLOC_CODEC_DATA
		ulFrameSize += _OVERALLOC_CODEC_DATA;   // over allocate since codec reads 24bits at a time
#endif	// _OVERALLOC_CODEC_DATA

		if (m_pAllocator)
		{
		    IHXUnknown* pIUnkn = NULL;
		    HX20ALLOCPROPS allocRequest;
		    HX20ALLOCPROPS allocActual;

		    allocRequest.uBufferSize = ulFrameSize;
		    allocRequest.nNumBuffers = 0;
		    m_pAllocator->SetProperties(&allocRequest, &allocActual);
		    pHXCodecData->data = m_pAllocator->GetPacketBuffer(&pIUnkn);
		}
		else
		{
		    pHXCodecData->data = (UINT8*) new ULONG32 [ulFrameSize / 4 + 1];
		}

		if (pHXCodecData->data == NULL)
		{
		    retVal = HXR_OUTOFMEMORY;
		}
	    }
	}

	// Build Codec Data
	if (retVal == HXR_OK)
	{
	    HXBOOL bNoneLost = TRUE;
	    pHXCodecSegmentInfo = (HXCODEC_SEGMENTINFO*) &(pHXCodecData->Segments[0]);

	    ulFrameSize = 0;

	    for (ulIdx = 0; ulIdx < ulSegmentCount; ulIdx++)
	    {
		HX_ASSERT(!m_InputPackets.IsEmpty());

		pPacket = (IHXPacket*) m_InputPackets.RemoveHead();

		HX_ASSERT(pPacket);

                // Determine if this is a keyframe. If there
                // are multiple packets with the same timestamp,
                // then only look at the first one.
                if (ulIdx == 0)
                {
                    // Get the ASM flags
                    BYTE ucFlags = pPacket->GetASMFlags();
                    // Is HX_ASM_SWITCH_ON set in the ASM flags?
                    if (ucFlags & HX_ASM_SWITCH_ON)
                    {
                        // This is a keyframe packet, so set the keyframe
                        // flag in the HXCODEC_DATA flags field.
                        pHXCodecData->flags |= HX_KEYFRAME_FLAG;
                    }
                }

		ulSegmentSize = 0;

		pHXCodecSegmentInfo[ulIdx].ulSegmentOffset = ulFrameSize;

		bIsLost = pPacket->IsLost();
		if (!bIsLost)
		{
		    pBuffer = pPacket->GetBuffer();

		    if (pHXCodecData->data)
		    {
			if (m_bFirstFrame)
			{
#ifdef _APPEND_BITSTREAM_HEADER
			    memcpy(pHXCodecData->data, m_pBitstreamHeader, m_ulBitstreamHeaderSize); /* Flawfinder: ignore */
			    ulFrameSize += m_ulBitstreamHeaderSize;
#endif	// _APPEND_BITSTREAM_HEADER
			    m_bFirstFrame = FALSE;
			}

			if (pBuffer)
			{
			    ulSegmentSize = pBuffer->GetSize();

			    HX_ASSERT(pHXCodecData->dataLength >= (ulFrameSize + ulSegmentSize));

			    memcpy(pHXCodecData->data + ulFrameSize, /* Flawfinder: ignore */
				   pBuffer->GetBuffer(),
				   ulSegmentSize);

			    pBuffer->Release();
			}
		    }
		    else
		    {
			HX_ASSERT(m_pAllocator);
			HX_ASSERT(ulValidSegmentCount == 1);
#ifdef _APPEND_BITSTREAM_HEADER
			HX_ASSERT(m_bFirstFrame == FALSE);
#endif	// _APPEND_BITSTREAM_HEADER

			if (pBuffer)
			{
			    ulSegmentSize = pBuffer->GetSize();
			    pHXCodecData->data = m_pAllocator->AddBuffer(pBuffer);
			    pBuffer->Release();
			}
		    }

		    ulFrameSize += ulSegmentSize;
		}

		pHXCodecSegmentInfo[ulIdx].bIsValid = !bIsLost;
		bNoneLost = (bNoneLost && (!bIsLost));

		pPacket->Release();
	    }

	    if (bNoneLost)
	    {
		pHXCodecData->numSegments = 1;
	    }
#ifdef _ASSERT_ON_LOSS
	    else
	    {
		HX_ASSERT(FALSE);
	    }
#endif	// _ASSERT_ON_LOSS

#ifdef _DONOT_SEGMENT
	    pHXCodecData->numSegments = 1;
	    pHXCodecSegmentInfo[0].bIsValid = bNoneLost;
#endif	// _DONOT_SEGMENT
	}

	// Finalize Results
	if (retVal == HXR_OK)
	{
	    pHXCodecDataOut = pData;
	}
	else
	{
	    if (pHXCodecData && pHXCodecData->data)
	    {
		m_pAllocator->ReleasePacketPtr(pHXCodecData->data);
	    }
	    HX_VECTOR_DELETE(pData);
	}

#ifdef _DUMP_FIRST_NFRAMES
	if (m_uSeqNumber < _DUMP_FIRST_NFRAMES)
	{
	    char pFileName[100]; /* Flawfinder: ignore */

	    SafeSprintf(pFileName, 100, "%s%d", "c:\\fframe.bin", m_uSeqNumber);

	    FILE* pFile = fopen(pFileName, "wb");

	    if (pFile)
	    {
		fprintf(pFile, "T=%ld;B=%ld:", pHXCodecData->timestamp, pHXCodecData->dataLength);
		fwrite(pHXCodecData->data, sizeof(UINT8), pHXCodecData->dataLength, pFile);
		fclose(pFile);
	    }
	}
#endif	// _DUMP_FIRST_NFRAMES

	m_uSeqNumber++;
    }

    return retVal;
}

const char* MP4VPayloadFormat::GetCodecId()
{
    const char* pszRet = NULL;

    if (m_ppszCodecID)
    {
        pszRet = m_ppszCodecID[m_ulCodecIDIndex];
    }

    return pszRet;
}

HX_RESULT MP4VPayloadFormat::SetNextCodecId()
{
    HX_RESULT retVal = HXR_FAIL;

    // Is our current codec ID non-NULL?
    if (m_ppszCodecID && m_ppszCodecID[m_ulCodecIDIndex])
    {
        // Advance the codec ID index
        m_ulCodecIDIndex++;
        // Now is our codec index non-NULL?
        if (m_ppszCodecID[m_ulCodecIDIndex])
        {
            // If we have a non-NULL codec ID, then clear the return value
            retVal = HXR_OK;
        }
    }

    return retVal;
}

void MP4VPayloadFormat::ResetCodecId()
{
    m_ulCodecIDIndex = 0;
}


STDMETHODIMP
MP4VPayloadFormat::Flush()
{
    m_bFlushed = TRUE;

    return HXR_OK;
}


HXBOOL MP4VPayloadFormat::IsPictureStart(IHXPacket* pPacket)
{
    if (m_PayloadID == PYID_X_HX_MP4)
    {
	ULONG32 ulIdx;
	ULONG32 ulSize = 0;
	UINT8* pData = NULL;
	IHXBuffer* pBuffer;

	pBuffer = pPacket->GetBuffer();

	if (pBuffer)
	{
	    ulSize = pBuffer->GetSize();
	    if (ulSize >= 4)
	    {
		ulSize -= 4;
		pData = pBuffer->GetBuffer();
	    }

	    pBuffer->Release();
	}

	if (pData)
	{
	    ulIdx = 0;

	    do
	    {
		if (pData[ulIdx++] == 0x00)
		{
		    if (pData[ulIdx++] == 0x00)
		    {
			if (pData[ulIdx] == 0x01)
			{
			    ulIdx++;
			    if (pData[ulIdx] == 0xb6)
			    {
				return TRUE;
			    }
			}
			else
			{
			    ulIdx--;
			}
		    }
		}
	    } while (ulIdx < ulSize);
	}
    }

    return FALSE;
}


HXBOOL MP4VPayloadFormat::IsValidPacket(IHXPacket* pPacket)
{
    if (m_PayloadID == PYID_X_HX_MP4)
    {
	ULONG32 ulSize;
	UINT8* pData = NULL;
	IHXBuffer* pBuffer = pPacket->GetBuffer();

	if (pBuffer)
	{
	    ulSize = pBuffer->GetSize();
	    if (ulSize >= 4)
	    {
		ulSize -= 4;
		pData = pBuffer->GetBuffer();
	    }

	    pBuffer->Release();
	}

	if (pData)
	{
	    // Should be more sophisiticated here and fully parse the
	    // headeres and make sure there is no data of substance
	    // following the headers - for this prototype code it
	    // is good enough
	    if ((pData[0] == 0x00) &&
		(pData[1] == 0x00) &&
		(pData[2] == 0x01) &&
		((pData[3] == 0x00) ||
		(pData[3] == 0x20)))
	    {
		return FALSE;
	    }
	}
    }

    return TRUE;
}


ULONG32 MP4VPayloadFormat::GetPacketTime(IHXPacket* pPacket)
{
    ULONG32 ulTime;

    HX_ASSERT(pPacket);

    if (m_bUsesRTPPackets && (m_ulSamplesPerSecond != 0))
    {
	ulTime = ((IHXRTPPacket*) pPacket)->GetRTPTime();

	ulTime = m_TSConverter.Convert(ulTime);
    }
    else
    {
	ulTime = pPacket->GetTime();
    }

    return ulTime;
}

