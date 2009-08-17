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
// #define _LOG_DATA_ACCESS
#define QTTRACKCACHE_PAGE_SIZE	0x0000FFFF

#define QT_MAX_SEEK_SKIPBACK_TIME_CLIENT	300000   // milliseconds
#define QT_MAX_SEEK_SKIPBACKAHEAD_RATIO_CLIENT	20
#define QT_MAX_SEEK_SKIPBACK_TIME_SERVER	10000	// milliseconds
#define QT_MAX_SEEK_SKIPBACKAHEAD_RATIO_SERVER	5	
#define QT_NO_KEY_FRAME_SKIPBACK_TIME		5000


/****************************************************************************
 *  Includes
 */
#include "qtpktasm.h"

#include "qtffplin.h"
#include "qtpacketizerfct.h"
#include "qttrack.h"
#include "qtffrefcounter.h"

#include "hxstrutl.h"

#include "sdpchunk.h"
#include "sdppyldinfo.h"


/****************************************************************************
 *  Class CQTTrack
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQTTrack::CQTTrack(CQTAtom* pTrackAtom)
    : m_pTrackAtom((CQT_trak_Atom*) pTrackAtom)
    , m_pResponse(NULL)
    , m_pFileFormat(NULL)
    , m_pPacketAssembler(NULL)
    , m_pClassFactory(NULL)
    , m_ulTrackID(0)
    , m_ulReadSize(0)
    , m_ulReadPageSize(0)
    , m_ulReadPageOffset(0)
    , m_pReadFileNameBuffer(NULL)
    , m_pReadFileName(NULL)
    , m_PendingState(QTT_Offline)
    , m_bTrackDone(FALSE)
    , m_uBytesPerCBlock(0)
    , m_uSamplesPerCBlock(0)
    , m_uStreamNumber(0)
    , m_uBaseRuleNumber(0)
    , m_pPacketizer(NULL)
    , m_pFileSwitcher(NULL)
    , m_bIsSubscribed(FALSE)
    , m_lRefCount(0)
    , m_ulLastSampleDescIdx(0)
{
    CQT_tkhd_Atom* pTrackHeaderAtom = NULL;

    HX_ASSERT(pTrackAtom);
    pTrackAtom->AddRef();
    
    if (pTrackAtom)
    {
	pTrackHeaderAtom = (CQT_tkhd_Atom*) 
	    pTrackAtom->FindPresentChild(QT_tkhd);
    }
    
    if (pTrackHeaderAtom)
    {
	m_ulTrackID = pTrackHeaderAtom->Get_TrackID();
    }

    g_nRefCount_qtff++;
}

CQTTrack::~CQTTrack()
{
    Close();

    g_nRefCount_qtff--;
}

/****************************************************************************
 *  Main (IQTTrack) Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CQTTrack::Init(IQTTrackResponse* pResponse,
			 CQTFileFormat *pFileFormat,
			 CQTPacketAssembler *pPacketAssembler,
			 CQTPacketizerFactory* pPacketizerFactory,
			 const char* pProtocol)
{
    CQTAtom* pAtom = NULL;
    HXBOOL bIsStreamTrack = FALSE;
    HX_RESULT retVal = HXR_OK;

    if (!m_pTrackAtom)
    {
	retVal = HXR_UNEXPECTED;
    }

    pAtom = m_pTrackAtom;

    m_uBytesPerCBlock = 0;
    m_uSamplesPerCBlock = 0;

    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(pResponse);
	HX_ASSERT(pFileFormat);

	m_pResponse = pResponse;
	pResponse->AddRef();

	bIsStreamTrack = pFileFormat->m_TrackManager.IsStreamTrack(this);
	
	m_pFileFormat = pFileFormat;
	pFileFormat->AddRef();

	HX_ASSERT(pPacketAssembler);

	m_pPacketAssembler = pPacketAssembler;
	pPacketAssembler->AddRef();

	retVal = pFileFormat->QueryInterface(
	    IID_IHXFileSwitcher,
	    (void**) &m_pFileSwitcher);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pFileFormat->QueryInterface(IID_IHXCommonClassFactory,
					     (void**) &m_pClassFactory);
    }

    if (SUCCEEDED(retVal))
    {
	CQT_tkhd_Atom* pTrackHeaderAtom = NULL;

	HX_ASSERT(m_pFileSwitcher);

	if (pAtom)
	{
	    pTrackHeaderAtom = (CQT_tkhd_Atom*) 
			       pAtom->FindPresentChild(QT_tkhd);
	}

	if (pTrackHeaderAtom)
	{
	    m_ulTrackID = pTrackHeaderAtom->Get_TrackID();
	}
	else
	{
	    retVal = HXR_FAIL;
	}
    }

    if (SUCCEEDED(retVal))
    {
	IHXCommonClassFactory* pClassFactory = NULL;
	CQTAtom* pChildAtom = NULL;

	if (pAtom)
	{
	    pAtom = pAtom->FindPresentChild(QT_mdia);
	}
	
	if (pAtom)
	{
	    pAtom = pAtom->FindPresentChild(QT_minf);
	}
	
	if (pAtom)
	{
	    pChildAtom = pAtom->FindPresentChild(QT_dinf);
	}
	
	retVal = pFileFormat->QueryInterface(
	    IID_IHXCommonClassFactory,
	    (void**) &pClassFactory);

	if (SUCCEEDED(retVal))
	{
	    retVal = m_DataRef.Init(pChildAtom, pClassFactory);
	}

	HX_RELEASE(pClassFactory);
    }

    if (retVal == HXR_OK)
    {
	if (pAtom)
	{
	    pAtom = pAtom->FindPresentChild(QT_stbl);
	}

	retVal = m_SampleSize.Init(pAtom);
    }

    if (retVal == HXR_OK)
    {
	retVal = m_SampleToChunk.Init(pAtom);
    }

    if (retVal == HXR_OK)
    {
	retVal = m_TimeToSample.Init(pAtom);
    }

    if (retVal == HXR_OK)
    {
	retVal = m_ChunkToOffset.Init(pAtom);
    }

    if (retVal == HXR_OK)
    {
	retVal = m_SampleDesc.Init(pAtom);
    }

    if ((retVal == HXR_OK) && bIsStreamTrack)
    {
	retVal = m_TrackInfo.Init(pFileFormat->GetContext(),
				  m_pTrackAtom,
				  &m_SampleDesc,
				  &pFileFormat->m_TrackManager,
				  &pFileFormat->m_MovieInfo);
    }

    if ((retVal == HXR_OK) && bIsStreamTrack)
    {
	retVal = InitPacketizer(m_pPacketizer,
				pPacketizerFactory,
				pProtocol,
				&m_TrackInfo,
				&pFileFormat->m_MovieInfo,
				&pFileFormat->m_TrackManager,
				this,
				pFileFormat->GetContext());
    }

    if ((retVal == HXR_OK) && bIsStreamTrack)
    {
	CQTAtom* pChildAtom = NULL;

	pChildAtom = m_pTrackAtom->FindPresentChild(QT_edts);

	retVal = m_TrackEdit.Init(
	    pChildAtom,
	    pFileFormat->m_MovieInfo.GetMovieTimeScale(),
	    m_TrackInfo.GetMediaTimeScale());
    }

    if ((retVal == HXR_OK) && bIsStreamTrack)
    {
	retVal = Seek(0);

	if (retVal == HXR_STREAM_DONE)
	{
	    retVal = HXR_OK;
	}
    }

    HX_RELEASE(m_pTrackAtom);

    return retVal;
}

/****************************************************************************
 *  SetResponse
 */
 HX_RESULT CQTTrack::SetResponse(IQTTrackResponse* pResponse)
 {
     HX_RESULT retVal = HXR_OK;

     HX_ASSERT(pResponse);
     
     HX_RELEASE(m_pResponse);
     m_pResponse = pResponse;
     m_pResponse->AddRef();

     return retVal;
 }

/****************************************************************************
 *  Close
 */
void CQTTrack::Close(void)
{
    m_PendingState = QTT_Offline;

    HX_RELEASE(m_pTrackAtom);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pFileFormat);
    HX_RELEASE(m_pPacketAssembler);
    HX_RELEASE(m_pReadFileNameBuffer);
    HX_RELEASE(m_pFileSwitcher);
    HX_RELEASE(m_pPacketizer);
    HX_RELEASE(m_pClassFactory);
}

/****************************************************************************
 *  BuildStreamHeader
 */
HX_RESULT CQTTrack::BuildStreamHeader(IHXValues* &pHeader,
				      CQT_MovieInfo_Manager* pMovieInfo,
				      CQTTrackManager* pTrackManager)
{
    IHXBuffer* pName = NULL;
    IHXBuffer* pASMRuleBook = NULL;
    IHXBuffer* pMimeType = NULL;
    ULONG32 ulPayloadType = QT_BAD_PAYLOAD;
    ULONG32 ulAvgBitRate = 0;
    ULONG32 ulMaxBitRate = 0;
    ULONG32 ulAvgPacketSize = 0;

    ULONG32 ulPreroll = 0;
    ULONG32 ulPredata = 0;
    ULONG32 ulWidth = 0;
    ULONG32 ulHeight = 0;
    ULONG32 ulFrameWidth = 0;
    ULONG32 ulFrameHeight = 0;
    HX_RESULT retVal = HXR_OK;

    if (m_TrackInfo.GetHeader(pHeader) == HXR_OK)
    {
	return retVal;
    }

    // Create needed buffers
    if (!pHeader)
    {
	retVal = m_pClassFactory->CreateInstance(CLSID_IHXValues,
						 (void**) &pHeader);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
						 (void**) &pName);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
						 (void**) &pASMRuleBook);
    }

    // Set TrackID
    if (SUCCEEDED(retVal))
    {
	retVal = pHeader->SetPropertyULONG32("TrackID", m_ulTrackID);
    }

    // Set Switch Group Id if present
    if (SUCCEEDED(retVal))
    {
	if (m_TrackInfo.GetSwitchGroupId() != 0)
	{
	    retVal = pHeader->SetPropertyULONG32("SwitchGroupID", 
						 m_TrackInfo.GetSwitchGroupId());
	}
    }

    // Set Stream Name
    if (SUCCEEDED(retVal) && (m_TrackInfo.GetNameLength() > 0))
    {
	UCHAR* pNameMem;
	ULONG32 ulNameMemLength = 0;
	
	retVal = pName->SetSize(m_TrackInfo.GetNameLength() + 1);
	if (SUCCEEDED(retVal))
	{
	    pName->Get(pNameMem, ulNameMemLength);
	    memcpy(pNameMem, m_TrackInfo.GetName(), /* Flawfinder: ignore */
		   ulNameMemLength - 1);
	    pNameMem[ulNameMemLength - 1] = '\0';

	    pHeader->SetPropertyCString("StreamName", pName);
	}
    }

    HX_RELEASE(pName);

    // Set Opaque Data - if any
    if (SUCCEEDED(retVal) && (m_TrackInfo.GetOpaqueDataLength() > 0))
    {
	IHXBuffer* pOpaqueData = NULL;

	retVal = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
						 (void**) &pOpaqueData);

	if (SUCCEEDED(retVal))
	{
	    UCHAR* pOpaqueDataMem;
	    ULONG32 ulOpaqueDataLength = 0;
	
	    retVal = pOpaqueData->SetSize(m_TrackInfo.GetOpaqueDataLength());
	    if (SUCCEEDED(retVal))
	    {
		pOpaqueData->Get(pOpaqueDataMem, ulOpaqueDataLength);
		memcpy(pOpaqueDataMem, /* Flawfinder: ignore */
		       m_TrackInfo.GetOpaqueData(),
		       ulOpaqueDataLength);
		pHeader->SetPropertyBuffer("OpaqueData", pOpaqueData);
	    }
	}

	HX_RELEASE(pOpaqueData);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = GetPayloadIdentity(pHeader);
    }
    
    // Set Duration, AvgBitRate and Preroll
    if (SUCCEEDED(retVal))
    {
	ULONG32 ulDuration = (ULONG32) ((((double) m_TrackInfo.GetTrackDuration())
					* 1000.0)
					/ pMovieInfo->GetMovieTimeScale());

	ObtainTrackBitrate(ulAvgBitRate);
#if defined(QTCONFIG_GET_BITRATE_FROM_SDP_PAYLOAD)
	if (ulAvgBitRate == 0)
	{
	    ulAvgBitRate = SDPMapPayloadToBitrate(ulPayloadType);
	}
#endif /* #if defined(QTCONFIG_GET_BITRATE_FROM_SDP_PAYLOAD) */

        if (ulAvgBitRate == 0)
        {
             pHeader->GetPropertyULONG32("AvgBitRate", ulAvgBitRate);
        }
        ulAvgPacketSize = m_TrackInfo.GetAvgPacketSize();


	ulMaxBitRate = m_TrackInfo.GetMaxBitrate();

	ulPreroll = m_TrackInfo.GetPreroll();
	ulPredata = m_TrackInfo.GetPredata();

	ulWidth       = m_TrackInfo.GetTrackWidth();
	ulHeight      = m_TrackInfo.GetTrackHeight();
        ulFrameWidth  = m_TrackInfo.GetFrameWidth();
        ulFrameHeight = m_TrackInfo.GetFrameHeight();

	pHeader->SetPropertyULONG32("Duration", ulDuration);

	if (ulAvgBitRate != 0)
	{
	    pHeader->SetPropertyULONG32("AvgBitRate", ulAvgBitRate);
	}

	if (ulMaxBitRate != 0)
	{
	    pHeader->SetPropertyULONG32("MaxBitRate", ulMaxBitRate);
	}

	if (ulPreroll != 0)
	{
	    pHeader->SetPropertyULONG32("Preroll", ulPreroll);
	}

	if (ulPredata != 0)
	{
	    pHeader->SetPropertyULONG32("Predata", ulPredata);
        if (ulAvgPacketSize != 0)
        {
            pHeader->SetPropertyULONG32("AvgPacketSize", ulAvgPacketSize);
        }
	}

	if (ulWidth != 0)
	{
	    pHeader->SetPropertyULONG32(QT_WIDTH_METANAME, ulWidth);
	}

	if (ulHeight != 0)
	{
	    pHeader->SetPropertyULONG32(QT_HEIGHT_METANAME, ulHeight);
	}

	if (ulFrameWidth != 0)
	{
	    pHeader->SetPropertyULONG32(QT_FRAMEWIDTH_METANAME, ulFrameWidth);
	}

	if (ulFrameHeight != 0)
	{
	    pHeader->SetPropertyULONG32(QT_FRAMEHEIGHT_METANAME, ulFrameHeight);
	}

    if (m_TrackInfo.GetTrackType() == QT_vide)
    {
        UINT32 framesPerMSecond;
        if ( (framesPerMSecond=GetFramesPerMSecond(pMovieInfo)) )
        {
	            pHeader->SetPropertyULONG32("FramesPerMSecond", framesPerMSecond);
        }
    }
    }

    if (SUCCEEDED(retVal))
    {
	// .mp4 and .mov files may produce streams with out-of-order time stamps
	// Client core does not normally expect this in RDT protocol so set this
	// property to alert client core of this fact.
	pHeader->SetPropertyULONG32("HasOutOfOrderTS", 1);
    }

    // Set ASM Rule Book
    if (SUCCEEDED(retVal))
    {
	char pRuleBook[128]; /* Flawfinder: ignore */

#ifdef QTCONFIG_SERVER
	pHeader->GetPropertyULONG32("RTPPayloadType", ulPayloadType);

	if (pTrackManager->GetEType() != QT_ETYPE_CLIENT)
	{
	    // Server Side
	    retVal = HXR_FAIL;
	    if (ulPayloadType != QT_BAD_PAYLOAD)
	    {
		retVal = HXR_OK;
	    }

	    if (SUCCEEDED(retVal))
	    {
		HXBOOL bSupportedPayload = SDPIsKnownPayload(ulPayloadType);

		pHeader->GetPropertyCString("MimeType", pMimeType);

		// This is not a standard payload - 
		// but maybe the encoding is standard
		if (!bSupportedPayload && pMimeType)
		{
		    bSupportedPayload = (SDPMapMimeToPayload(pMimeType, 
							     ulPayloadType)
					 == HXR_OK);
		}

		// Set the ASM rule book
		if ((ulAvgBitRate > 0)
#ifndef QTFORCE_AVG_BITRATE_DELIVERY
		    && bSupportedPayload
		    && SDPIsFixedRatePayload(ulPayloadType)
#endif	// QTFORCE_AVG_BITRATE_DELIVERY
		   )
		{
		    // divide avg bitrate
		    UINT32 ulAvgBandwidth = ulAvgBitRate / 2;

		    SafeSprintf(pRuleBook, 128, 
			    "Marker=%d,AverageBandwidth=%d;"
			    "Marker=%d,AverageBandwidth=%d;",
			    (QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
			    ulAvgBitRate - ulAvgBandwidth,
			    (QTASM_MARKER_ON_RULE == 0) ? 0 : 1,
			    ulAvgBandwidth);
		}
		else
		{
		    if (ulAvgBitRate > 0)
		    {
			SafeSprintf(pRuleBook, 128, "Marker=%d,AverageBandwidth=%d,TimeStampDelivery=TRUE;"
					   "Marker=%d,AverageBandwidth=0,TimeStampDelivery=TRUE;",
			        (QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
				ulAvgBitRate,
				(QTASM_MARKER_ON_RULE == 0) ? 0 : 1);
		    }
		    else
		    {
			SafeSprintf(pRuleBook, 128, "Marker=%d,TimeStampDelivery=TRUE;"
					   "Marker=%d,TimeStampDelivery=TRUE;",
			        (QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
				(QTASM_MARKER_ON_RULE == 0) ? 0 : 1);
		    }
		}
	    }
	}
	else
#endif	// QTCONFIG_SERVER
	{
	    // Player Side
	    SafeSprintf(pRuleBook, 128, "Marker=%d;Marker=%d;",
		(QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
		(QTASM_MARKER_ON_RULE == 0) ? 0 : 1);
	}


	if (SUCCEEDED(retVal))
	{
	    retVal = pASMRuleBook->Set((UCHAR*) pRuleBook, strlen(pRuleBook) + 1);
	}

	if (SUCCEEDED(retVal))
	{
	    pHeader->SetPropertyCString("ASMRuleBook", pASMRuleBook);
	}
    }

    HX_RELEASE(pASMRuleBook);
    HX_RELEASE(pMimeType);

    return retVal;
}

/****************************************************************************
 *  GetPayloadIdentity
 */
HX_RESULT CQTTrack::GetPayloadIdentity(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    if (SUCCEEDED(retVal))
    {
	const char* pMimeType = m_TrackInfo.GetMimeType();
	retVal = HXR_FAIL;
	if (pMimeType)
	{
	    IHXBuffer* pMimeTypeBuffer = NULL;
	    UINT32 ulMimeTypeLength = strlen(pMimeType);
	    retVal = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
						     (void**) &pMimeTypeBuffer);

	    if (SUCCEEDED(retVal))
	    {	
		retVal = pMimeTypeBuffer->SetSize(ulMimeTypeLength + 1);
	    }

	    if (SUCCEEDED(retVal))
	    {
		memcpy(pMimeTypeBuffer->GetBuffer(), /* Flawfinder: ignore */
		       pMimeType,
		       ulMimeTypeLength + 1);
		retVal = pHeader->SetPropertyCString("MimeType", pMimeTypeBuffer);
	    }

	    HX_RELEASE(pMimeTypeBuffer);
	}
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulTimeScale = m_TrackInfo.GetMediaTimeScale();

	retVal = HXR_FAIL;
	if (ulTimeScale != 0)
	{
	    retVal = pHeader->SetPropertyULONG32("SamplesPerSecond", ulTimeScale);
	}
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulPayloadType = m_TrackInfo.GetPayloadType();

	retVal = HXR_FAIL;
	if (ulPayloadType != QT_BAD_PAYLOAD)
	{
	    retVal = pHeader->SetPropertyULONG32("RTPPayloadType", ulPayloadType);
	}
    }

    return retVal;
}

/****************************************************************************
 *  Seek
 */
HX_RESULT CQTTrack::Seek(ULONG32 ulTime, HXBOOL bUseSyncPoints)
{
    if (m_pPacketizer)
    {
	m_pPacketizer->Reset();
    }

    m_bTrackDone = !m_TrackEdit.EstablishByTime(ulTime);

    if (!m_bTrackDone)
    {
	m_bTrackDone = !SequenceToTime(m_TrackEdit,
				       m_TimeToSample,
				       m_SampleToChunk,
				       bUseSyncPoints ? QTTRK_USE_KEY_FRAMES : QTTRK_USE_ANY_FRAME,
				       bUseSyncPoints); // If using sync points, use best pick heuristic
    }

    return m_bTrackDone ? HXR_STREAM_DONE : HXR_OK;
}

/****************************************************************************
 *  GetPacket
 */
HX_RESULT CQTTrack::GetPacket(UINT16 uStreamNum)
{
    HX_RESULT retVal = HXR_STREAM_DONE;

    m_uStreamNumber = uStreamNum;

    if (m_pPacketizer)
    {
	IHXPacket* pPacket = NULL;
	
	retVal = m_pPacketizer->GetPacket(pPacket);
	
	if (retVal != HXR_INCOMPLETE)
	{
	    m_pResponse->PacketReady(m_uStreamNumber, 
				     retVal, 
				     pPacket);
	    
	    HX_RELEASE(pPacket);
	    
	    return HXR_OK;
	}

	retVal = HXR_STREAM_DONE;
    }

    if (!m_bTrackDone)
    {
	m_PendingState = QTT_SampleRead;

	// Locate Sample
	if (m_ChunkToOffset.EstablishByChunk(
		m_SampleToChunk.GetChunkNum()) &&
	    m_SampleSize.EstablishBySample(
		m_TimeToSample.GetSampleNumber(),
		m_SampleToChunk.GetChunkSampleNum()) &&
	    m_SampleDesc.EstablishByIdx(
		m_SampleToChunk.GetSampleDescIdx()) &&
	    m_DataRef.EstablishByIdx(
		m_SampleDesc.GetDataRefIdx()))
	{
	    ULONG32 ulFileOffset = m_ChunkToOffset.GetChunkOffset() +
				   m_SampleSize.GetChunkSampleOffset();

#ifdef QTCONFIG_TRACK_CACHE
	    if (m_DataRef.GetDataRefName() == NULL)
	    {
		if (m_TrackCache.IsInPage(ulFileOffset, m_SampleSize.GetSampleSize()))
		{
		    IHXBuffer* pBuffer;
		    UINT32 ulPageOffset;
		    m_TrackCache.Get(ulFileOffset,
				     pBuffer,
				     ulPageOffset);

		    return ReturnPacket(HXR_OK,
					pBuffer,
					ulPageOffset, 
					m_SampleSize.GetSampleSize());
		}
	    }
#endif	// QTCONFIG_TRACK_CACHE

	    retVal = LoadData(
			m_DataRef.GetDataRefName(),
			ulFileOffset,
			m_SampleSize.GetSampleSize());
	}
	else
	{
	    retVal = ReturnPacket(retVal, NULL);
	}
    }
    else
    {
	retVal = ReturnPacket(retVal, NULL);
    }

    return retVal;
}

/****************************************************************************
 *  Subscribe/Unsubscribe
 *	Any subscription notification on non-switchable track are unexpected.
 */
HX_RESULT CQTTrack::Subscribe(UINT16 uRuleNumber)
{
    m_bIsSubscribed = TRUE;

    return HXR_OK;
}

HX_RESULT CQTTrack::Unsubscribe(UINT16 uRuleNumber)
{
    m_bIsSubscribed = FALSE;

    return HXR_OK;
}

HX_RESULT CQTTrack::SubscribeDefault(void)
{
    m_bIsSubscribed = TRUE;

    return HXR_OK;
}


HXBOOL CQTTrack::IsSubscribed(void)
{
    return m_bIsSubscribed;
}

/****************************************************************************
 *  ComputeTrackSize
 */
HX_RESULT CQTTrack::ComputeTrackSize(ULONG32& ulTrackSizeOut)
{
    ULONG32 ulTrackSize = 0;
    ULONG32 ulCurrentMediaTime = m_TrackEdit.GetMediaTime();
    HX_RESULT retVal = Seek(0);

    if (SUCCEEDED(retVal))
    {
	do
	{
	    if (!m_SampleSize.EstablishBySample(
		    m_TimeToSample.GetSampleNumber(),
		    m_SampleToChunk.GetChunkSampleNum()))
	    {
		break;
	    }

	    ulTrackSize += m_SampleSize.GetSampleSize();
	} while (AdvanceSample(m_TrackEdit,
			       m_TimeToSample,
			       m_SampleToChunk));

	ulTrackSizeOut = ulTrackSize;
    }

    Seek(ulCurrentMediaTime);

    return retVal;
}

/****************************************************************************
 *  ObtainBandwidth
 */
#if defined(QTCONFIG_ALTERNATE_STREAMS) && defined(QTCONFIG_SERVER)
HX_RESULT CQTTrack::ObtainTrackBandwidth(ULONG32& ulBandwidthOut)
{
    IHXValues* pProperties = NULL;
    UINT32 ulBandwidth = m_TrackInfo.GetNeededBandwidth();
    HX_RESULT retVal = HXR_OK;

    if (SUCCEEDED(retVal) && (ulBandwidth == 0))
    {
	if (m_TrackInfo.GetSDPLength() > 0)
	{
	    // Try to obtain the bandwidth from SDP information
	    SDPParseChunk(
		(char*) m_TrackInfo.GetSDP(),
		m_TrackInfo.GetSDPLength(),
		pProperties,
		m_pClassFactory,
		SDPCTX_Bandwidth,
		FALSE);	// do not remove bandwidth info from SDP
	}
	
	if (SUCCEEDED(retVal) && pProperties)
	{
	    pProperties->GetPropertyULONG32("AvgBitRate", ulBandwidth);
	}

	if (SUCCEEDED(retVal) && (ulBandwidth != 0))
	{
	    m_TrackInfo.SetNeededBandwidth(ulBandwidth);
	}
	
	HX_RELEASE(pProperties);
    }

    // If bandwidth is still unknown, attempt to use clip bitrate as bandwidth
    if (SUCCEEDED(retVal) && (ulBandwidth == 0))
    {
	retVal = ObtainTrackBitrate(ulBandwidth);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_FAIL;

	if (ulBandwidth != 0)
	{
	    ulBandwidthOut = ulBandwidth;
	    retVal = HXR_OK;
	}
    }

    return retVal;
}
#else	// defined(QTCONFIG_ALTERNATE_STREAMS) && defined(QTCONFIG_SERVER)
HX_RESULT CQTTrack::ObtainTrackBandwidth(ULONG32& ulBandwidthOut)
{
    return ObtainTrackBitrate(ulBandwidthOut);
}
#endif	// defined(QTCONFIG_ALTERNATE_STREAMS) && defined(QTCONFIG_SERVER)


HX_RESULT CQTTrack::ObtainTrackBitrate(ULONG32& ulAvgBitrateOut)
{
    ULONG32 ulAvgBitRate;
    ULONG32 ulDuration;
    HX_RESULT retVal = HXR_FAIL;

    ulAvgBitRate = m_TrackInfo.GetAvgBitrate();

    if (ulAvgBitRate == 0)
    {
	ulDuration = (ULONG32) ((((double) m_TrackInfo.GetTrackDuration())
				 * 1000.0)
				/ m_pFileFormat->m_MovieInfo.GetMovieTimeScale());
    
	if (ulDuration != 0)
	{
	    ULONG32 ulTrackSize = m_TrackInfo.GetTrackSize();
	
	    if (ulTrackSize == 0)
	    {
		ComputeTrackSize(ulTrackSize);
		m_TrackInfo.SetTrackSize(ulTrackSize);
	    }
	
	    ulAvgBitRate = (ULONG32) ((((double) ulTrackSize) * 8000.0) 
				      / ulDuration + 0.5);
	}
    }
    
    if (ulAvgBitRate != 0)
    {
	ulAvgBitrateOut = ulAvgBitRate;
	retVal = HXR_OK;
    }

    return retVal;
}


/****************************************************************************
 *  IHXThreadSafeMethods method
 */
/****************************************************************************
 *  IsThreadSafe
 */
STDMETHODIMP_(UINT32)
CQTTrack::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FF_GETPACKET | HX_THREADSAFE_METHOD_FSR_READDONE;
}


/****************************************************************************
 *  Protected Functions
 */
/****************************************************************************
 *  InitPacketizer
 */
HX_RESULT CQTTrack::InitPacketizer(IHXPayloadFormatObject* &pPacketizer,
				   CQTPacketizerFactory* pPacketizerFactory,
				   const char* pProtocol,
				   CQT_TrackInfo_Manager* pTrackInfo,
				   CQT_MovieInfo_Manager* pMovieInfo,
				   CQTTrackManager* pTrackManager,
				   CQTTrack* pTrack,
				   IUnknown* pContext)
{
    HX_RESULT retVal = HXR_OK;

#ifdef QTCONFIG_PACKETIZER_FACTORY
    if (pPacketizerFactory)
    {
	retVal = pPacketizerFactory->Construct(pPacketizer,
					       pProtocol,
					       pTrackInfo,
					       pMovieInfo,
					       pTrackManager,
					       pTrack,
					       pContext);

	if (retVal == HXR_NO_DATA)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	if (pPacketizer)
	{
	    IHXValues* pHeader = NULL;

	    retVal = pPacketizer->Init(pContext, TRUE);

	    if (SUCCEEDED(retVal))
	    {
		retVal = BuildStreamHeader(pHeader,
					   pMovieInfo,
					   pTrackManager);
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = pPacketizer->SetStreamHeader(pHeader);
	    }

	    HX_RELEASE(pHeader);

	    if (SUCCEEDED(retVal))
	    {
		retVal = pPacketizer->GetStreamHeader(pHeader);
	    }

	    if (SUCCEEDED(retVal))
	    {
		pTrackInfo->SetHeader(pHeader);
	    }

	    HX_RELEASE(pHeader);
	}
    }
#endif	// QTCONFIG_PACKETIZER_FACTORY

    return retVal;
}

/****************************************************************************
 *  LoadData
 */
HX_RESULT CQTTrack::LoadData(IHXBuffer* pFileName,
			    ULONG32 ulOffset,
			    ULONG32 ulSize)
{
    HX_RELEASE(m_pReadFileNameBuffer);
    m_pReadFileName = NULL;

#ifdef _LOG_DATA_ACCESS
    FILE* pf = fopen("C:\\qt_access.txt", "a");
    fprintf(pf, "%d %d %d\n", m_ulTrackID, ulOffset, ulSize);
    fclose(pf);
#endif // _LOG_DATA_ACCESS

#ifdef QTCONFIG_TRACK_CACHE
    m_TrackCache.ReleasePage();
#endif	// QTCONFIG_TRACK_CACHE

    m_ulReadSize = ulSize;
    m_ulReadPageSize = ulSize;
    m_ulReadPageOffset = ulOffset;
    
    if (pFileName)
    {
	m_pReadFileNameBuffer = pFileName;
	pFileName->AddRef();
	m_pReadFileName = (char *) pFileName->GetBuffer();
    }
#ifdef QTCONFIG_TRACK_CACHE
    else if (m_TrackCache.IsEnabled() && (ulSize < QTTRACKCACHE_PAGE_SIZE))
    {
	m_ulReadPageSize = QTTRACKCACHE_PAGE_SIZE;
    }
#endif	// QTCONFIG_TRACK_CACHE

    return m_pFileSwitcher->Seek(   ulOffset,
				    FALSE,
				    (IHXFileResponse*) this,
				    m_pReadFileName);
}

/****************************************************************************
 *  DataReady
 */
HX_RESULT CQTTrack::DataReady(HX_RESULT status, IHXBuffer *pBuffer)
{
#ifdef QTCONFIG_TRACK_CACHE
    if (status == HXR_OK)
    {
	if (m_ulReadPageSize > m_ulReadSize)
	{
	    m_TrackCache.SetPage(m_ulReadPageOffset, pBuffer);
	}
    }
    else
    {
	if (m_TrackCache.IsEnabled())
	{
	    m_TrackCache.Enable(FALSE);
	    return LoadData(m_pReadFileNameBuffer,
			    m_ulReadPageOffset,
			    m_ulReadSize);
	}
    }
#endif	// QTCONFIG_TRACK_CACHE

    switch (m_PendingState)
    {
        case QTT_SegmentRead:
            m_PendingState = QTT_Offline;

            if (m_pPacketAssembler)
            {
                return m_pPacketAssembler->SegmentReady(status, pBuffer,
                                                        0, m_ulReadSize);
            }
            break;

        case QTT_SampleRead:
            m_PendingState = QTT_Offline;
            return ReturnPacket(status, pBuffer, 0, m_ulReadSize);
            break;
        case QTT_Offline:
            break;
    }

    return HXR_UNEXPECTED;
}

/****************************************************************************
 *  ReturnPacket
 */
HX_RESULT CQTTrack::ReturnPacket(HX_RESULT status, 
				 IHXBuffer *pBuffer)
{
    return ReturnPacket(status,
			pBuffer,
			0,
			pBuffer ? pBuffer->GetSize() : 0);
}

/****************************************************************************
 *  ReturnPacket
 */
HX_RESULT CQTTrack::ReturnPacket(HX_RESULT status, 
				IHXBuffer *pBuffer,
				ULONG32 ulOffset,
				ULONG32 ulSize)
{
    ULONG32 ulTime = (ULONG32) (m_TrackEdit.GetRealTime() + 0.5);
    ULONG32 ulRTPTime = m_TrackEdit.GetMediaTime() + 
			m_TimeToSample.GetCompositionOffset();
    IHXPacket* pPacket = NULL;
    HX_RESULT retVal = HXR_OK;

    if (pBuffer)
    {
#ifdef QTCONFIG_BFRAG
	if ((ulOffset != 0) || (ulSize != pBuffer->GetSize()))
	{
#ifdef QTCONFIG_BFRAG_FACTORY
	    pBuffer = 
		m_pFileFormat->m_pBufferFragmentFactory->WrapFragment(
		    pBuffer, ulOffset, ulSize);
#else	// QTCONFIG_BFRAG_FACTORY
	    pBuffer = new CBufferFragment(pBuffer, ulOffset, ulSize);
	    
	    if (pBuffer)
	    {
		pBuffer->AddRef();
	    }
#endif	// QTCONFIG_BFRAG_FACTORY
	}
	else
#else	// QTCONFIG_BFRAG
	HX_ASSERT((ulOffset == 0) && (ulSize == pBuffer->GetSize()));
#endif	// QTCONFIG_BFRAG
	{
	    pBuffer->AddRef();
	}
    }

    // Form packet
    if (status == HXR_OK)
    {
	status = m_pClassFactory->CreateInstance(CLSID_IHXRTPPacket,
						 (void**) &pPacket);
    }

    if (pBuffer)
    {
	if (status == HXR_OK)
	{
	    status = ((IHXRTPPacket*) pPacket)->SetRTP(
		pBuffer,
		ulTime,
		ulRTPTime,
		m_uStreamNumber,
		m_TimeToSample.IsOnSyncSample() ? 
		    (HX_ASM_SWITCH_ON | HX_ASM_SWITCH_OFF) :
		    HX_ASM_SWITCH_OFF,			    // ASM Flags
		m_uBaseRuleNumber + QTASM_MARKER_ON_RULE);
	}

	pBuffer->Release();
	pBuffer = NULL;
    }
    
    if (status == HXR_OK)
    {
	m_ulLastSampleDescIdx = m_SampleToChunk.GetSampleDescIdx();
	m_bTrackDone = !AdvanceSample(m_TrackEdit,
				      m_TimeToSample,
				      m_SampleToChunk);
    }
    else
    {
	m_bTrackDone = TRUE;
    }

    if (m_pPacketizer)
    {
	if (status == HXR_OK)
	{
	    m_pPacketizer->SetPacket(pPacket);
	}

	HX_RELEASE(pPacket);

	if (m_bTrackDone)
	{
	    m_pPacketizer->Flush();
	}

	status = m_pPacketizer->GetPacket(pPacket);

	if (status == HXR_INCOMPLETE)
	{
	    if (!m_bTrackDone)
	    {
		status = GetPacket(m_uStreamNumber);
		if (status == HXR_OK)
		{
		    return status;
		}
	    }
	    else
	    {
		status = HXR_STREAM_DONE;
	    }
	}
    }

    retVal = m_pResponse->PacketReady(m_uStreamNumber, status, pPacket);

    HX_RELEASE(pPacket);

    return retVal;
}

/****************************************************************************
 *  SequenceToTime
 */
HXBOOL CQTTrack::SequenceToTime(CQT_TrackEdit_Manager &TrackEdit,
			      CQT_TimeToSample_Manager &TimeToSample,
			      CQT_SampleToChunk_Manager &SampleToChunk,
			      HXBOOL bUseNonKeyFrames,
			      HXBOOL bUseBestPickHeuristic)
{
    HXBOOL bOverSeek;

    do
    {
	bOverSeek = !(	bUseNonKeyFrames ?
			TimeToSample.EstablishByMediaTime(
			    TrackEdit.GetMediaTime()) :
			TimeToSample.EstablishAtKeyByMediaTime(
			    TrackEdit.GetMediaTime()));

	if (bUseBestPickHeuristic && !bUseNonKeyFrames)
	{
	    ULONG32 ulPrevKeyMediaTime = 0;

	    /* Evaluate the forward and backward time interval to the 
	       key-frames surrounding the target seek time and utilize 
	       a heuristic to choose either the forward or backward key-frame
	       as the starting point for the stream.

	       The heuristic:
	          The key-frame prior to the target seek point is selected
	       only if the following is true:
	          - there is no key-frame at target seek point
		  AND
		  - time_interval_to_key-frame_before_seek_point <= MaxSeekSkipBackTime
		 AND
		  - time_interval_to_key-frame_before_seek_point/
		    time_interval_to_key-frame_after_seek_point <= MaxSeekSkipBackAheadRatio
	    */
	    
	    if (TimeToSample.GetLastPreTargetKeyMediaTime(ulPrevKeyMediaTime))
	    {
		HXBOOL bTryPrevKeyframe;
		ULONG32 MaxSeekSkipBackTime = QT_MAX_SEEK_SKIPBACK_TIME_CLIENT;
	        ULONG32 MaxSeekSkipBackAheadRatio = QT_MAX_SEEK_SKIPBACKAHEAD_RATIO_CLIENT;
		LONG32 lMediaTimeToPrevKeyframe = 
		    TrackEdit.GetMediaTime() - ulPrevKeyMediaTime;
		ULONG32 ulMediaTimeToKeyframe = TimeToSample.GetLastMediaSyncTime();

		if (m_pFileFormat->m_TrackManager.GetEType() != QT_ETYPE_CLIENT)
		{
		    MaxSeekSkipBackTime = QT_MAX_SEEK_SKIPBACK_TIME_SERVER;
		    MaxSeekSkipBackAheadRatio = QT_MAX_SEEK_SKIPBACKAHEAD_RATIO_SERVER;
		}

		bTryPrevKeyframe = 
		    ((lMediaTimeToPrevKeyframe > 0) &&
		     (bOverSeek || (ulMediaTimeToKeyframe != 0)) &&
		     (TrackEdit.ConvertMediaToRealTime((ULONG32) lMediaTimeToPrevKeyframe) <= MaxSeekSkipBackTime) &&
		     (bOverSeek || ((lMediaTimeToPrevKeyframe / ulMediaTimeToKeyframe) <= MaxSeekSkipBackAheadRatio)));

		if (bTryPrevKeyframe)
		{
		    bOverSeek = !TrackEdit.EstablishByMediaTime(ulPrevKeyMediaTime);

		    if (!bOverSeek)
		    {
			bOverSeek = !SequenceToTime(TrackEdit,
			                            TimeToSample,
			                            SampleToChunk,
			                            bUseNonKeyFrames,
			                            FALSE);
		    }

		    return !bOverSeek;
		}
	    }
	    // If we were unable to locate a key-frame (sync frame), just use any frame
	    // fixed amount of time prior to the seek point hoping that the frames prior
	    // to the seek point will contain enough information to sufficiently
	    // reconstruct the frame at the seek point.
	    if (bOverSeek)
	    {
		UINT32 ulTargetTime = (UINT32) TrackEdit.GetRealTime();

		ulTargetTime = (ulTargetTime > QT_NO_KEY_FRAME_SKIPBACK_TIME) ? 
		    (ulTargetTime - QT_NO_KEY_FRAME_SKIPBACK_TIME) :
		    0;

		bOverSeek = !TrackEdit.EstablishByTime(ulTargetTime);

		if (!bOverSeek)
		{
		    bOverSeek = !SequenceToTime(TrackEdit,
						TimeToSample,
						SampleToChunk,
						TRUE,	// Use non-key frames
						FALSE);	// Do not use best-pick heuristic
		}

		return !bOverSeek;
	    }		
	}

	bUseBestPickHeuristic = FALSE;
    } while ((!bOverSeek) &&
	     TrackEdit.AdvanceByMediaTime(
		TimeToSample.GetLastMediaSyncTime(), bOverSeek) &&
	     (!bOverSeek));

    if (!bOverSeek)
    {
	bOverSeek = !SampleToChunk.EstablishBySample(
	    TimeToSample.GetSampleNumber());
    }

    return !bOverSeek;
}

/****************************************************************************
 *  AdvanceSample
 */
HXBOOL CQTTrack::AdvanceSample(CQT_TrackEdit_Manager &TrackEdit,
			    CQT_TimeToSample_Manager &TimeToSample,
			    CQT_SampleToChunk_Manager &SampleToChunk)
{
    HXBOOL bStreamDone;

    if (TrackEdit.AdvanceByMediaTime(
	    TimeToSample.GetSampleDuration(),
	    bStreamDone))
    {
	// Advancement in time caused change in edit segment
	if (!bStreamDone)
	{
	    bStreamDone = !SequenceToTime(TrackEdit,
					  TimeToSample,
					  SampleToChunk,
					  QTTRK_USE_ANY_FRAME);
	}
    }
    else
    {
	bStreamDone = ((!TimeToSample.AdvanceBySample()) ||
		       (!SampleToChunk.AdvanceBySample()));
    }

    return !bStreamDone;
}

/****************************************************************************
 *  IHXFileResponse methods 
 */
/****************************************************************************
 *  ReadDone
 */
STDMETHODIMP CQTTrack::ReadDone(HX_RESULT status,
				IHXBuffer* pBuffer)
{
    return DataReady(status, pBuffer);
}

// returns 0 on failure.

UINT32 CQTTrack::GetFramesPerMSecond(CQT_MovieInfo_Manager* pMovieInfo)
{
    UINT64 frameCount = m_SampleSize.Get_NumEntries();
    UINT32 ulTimeScale  = pMovieInfo->GetMovieTimeScale();
    UINT32 ulRetVal = 0;
    if (frameCount != 0 && ulTimeScale != 0)
    {
        UINT64 trackDuration = m_TrackInfo.GetTrackDuration();
        // 1 million seconds converted to movie time scale.
        UINT64 timeUnits     = ((UINT64) (1000*1000) * ulTimeScale);
        //calculate uframesPerMSecond = timeUnits * frameCount / trackDuration;
        UINT64 uframesPerMSecond = timeUnits/trackDuration * frameCount +
                           timeUnits%trackDuration * frameCount / trackDuration;
        if (uframesPerMSecond > MAX_UINT32)
        {
            ulRetVal = 0;
        }
        else
        {
            ulRetVal = INT64_TO_UINT32(uframesPerMSecond);
        }
    } 

    return ulRetVal;
}

/****************************************************************************
 *  SeekDone
 */
STDMETHODIMP CQTTrack::SeekDone(HX_RESULT status)
{
    if (SUCCEEDED(status))
    {
	return m_pFileSwitcher->Read(	m_ulReadPageSize,
					(IHXFileResponse*) this,
					m_pReadFileName);
    }

    return DataReady(status, NULL);
}

/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CQTTrack::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
	AddRef();
	*ppvObj = (IHXThreadSafeMethods*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CQTTrack::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CQTTrack::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

