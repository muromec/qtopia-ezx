/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 and Exhibits. 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM 
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. 
 * All Rights Reserved. 
 * 
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Community Source 
 * License Version 1.0 (the "RCSL"), including Attachments A though H, 
 * all available at http://www.helixcommunity.org/content/rcsl. 
 * You may also obtain the license terms directly from RealNetworks. 
 * You may not use this file except in compliance with the RCSL and 
 * its Attachments. There are no redistribution rights for the source 
 * code of this file. Please see the applicable RCSL for the rights, 
 * obligations and limitations governing use of the contents of the file. 
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
 * https://rarvcode-tck.helixcommunity.org 
 * 
 * Contributor(s): 
 * 
 * ***** END LICENSE BLOCK ***** */ 

// #define _DRY_RUN

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxmtypes.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxplugn.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxformt.h"
#include "hxprefs.h"
#include "mp4vdec.h"
#include "mp4vdfmt.h"
#include "mp4video.h"
#include "hlxclib/string.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

#define MP4V_HX_AVC1_PAYLOAD_MIME_TYPE	    "video/X-HX-AVC1"

/****************************************************************************
 *  Statics
 */


/****************************************************************************
 *  Method:
 *    CMP4VDecoder::CMP4VDecoder
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CMP4VDecoder::CMP4VDecoder()
    : m_pContext(NULL)
    , m_pVideoFormat(NULL)
    , m_pInputAllocator(NULL)
    , m_pOutputAllocator(NULL)
    , m_pCodecLib(NULL)
    , m_pCodec(NULL)
    , m_pStream(NULL)
    , m_pCodecId(NULL)
    , m_moftagOut(0)
    , m_pImageInfoBuffer(NULL)
    , m_ulLastTimeStamp(0)
{
    ;
}

CMP4VDecoder::~CMP4VDecoder()
{
    Close();
}


/****************************************************************************
 *  Method:
 *    CMP4VDecoder::Close
 */
HX_RESULT CMP4VDecoder::Close(void)
{
    if (m_pStream != NULL)
    {
	m_pCodecLib->PNStream_Close(m_pStream);
	m_pStream = NULL;
    }

    if (m_pCodec != NULL)
    {
	m_pCodecLib->PNCodec_Close(m_pCodec);
	m_pCodec = NULL;
    }

    HX_DELETE(m_pCodecLib);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pInputAllocator);
    HX_RELEASE(m_pOutputAllocator);
    HX_VECTOR_DELETE(m_pImageInfoBuffer);

    return HXR_OK;
}


/****************************************************************************
 *  Method:
 *    CMP4VDecoder::CMP4VDecoder
 */
HX_RESULT CMP4VDecoder::Init(IUnknown* pContext,
			     CMP4VideoFormat* pVideoFormat,
			     HXxSize* pSize,
			     IHX20MemoryAllocator* pInputAllocator,
			     IHX20MemoryAllocator* pOutputAllocator)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    m_pCodecId = "MP4V";

    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    HX_ASSERT(m_pContext);
    if (m_pContext)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	m_pVideoFormat = pVideoFormat;

	retVal = HXR_INVALID_PARAMETER;
	if (m_pVideoFormat)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	m_pCodecId = m_pVideoFormat->GetCodecId();
    }

    if (SUCCEEDED(retVal))
    {
        HX_RELEASE(m_pInputAllocator);
	m_pInputAllocator = pInputAllocator;
	if (m_pInputAllocator)
	{
	    m_pInputAllocator->AddRef();
	}

        HX_RELEASE(m_pOutputAllocator);
	m_pOutputAllocator = pOutputAllocator;
	if (m_pOutputAllocator)
	{
	    m_pOutputAllocator->AddRef();
	}

	retVal = HXR_INVALID_PARAMETER;
	if (m_pInputAllocator && m_pOutputAllocator)
	{
	    retVal = HXR_OK;
	}
    }

#ifndef _DRY_RUN
    // Open the codec
    if (SUCCEEDED(retVal))
    {
	retVal = OpenCodec(STRINGTOMOFTAG(m_pCodecId));
    }

    if (SUCCEEDED(retVal))
    {
	retVal = OpenStream(STRINGTOMOFTAG(m_pCodecId));
    }
#endif	// _DRY_RUN

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CMP4VDecoder::Decode()
 */
HX_RESULT CMP4VDecoder::Decode(CMediaPacket* pFrameToDecode,
			       ULONG32 ulQuality) 
{
    ULONG32 ulFirstTimeStamp = 0;
    HX_OQS2 theOutputQueueStatus;
    HX_RESULT retVal = HXR_OK;

    // tell the stream about renderer status so it can scale its processing
    theOutputQueueStatus.queueDepth = m_pVideoFormat->GetDecodedFrameQueueDepth();

    theOutputQueueStatus.newestQueueElementTimeStamp = m_ulLastTimeStamp;

    m_pVideoFormat->GetNextFrameTime(ulFirstTimeStamp);
    theOutputQueueStatus.oldestQueueElementTimeStamp = ulFirstTimeStamp;

    theOutputQueueStatus.maxQueueDepth = m_pVideoFormat->m_ulMaxDecodedFrames; 	
    theOutputQueueStatus.currentTimeStamp = -m_pVideoFormat->m_pMP4VideoRenderer->ComputeTimeAhead(0, 0);
    theOutputQueueStatus.nonFRUDroppedFrames = 0;
    theOutputQueueStatus.TotalDroppedFrames = 0; 

#ifdef _DRY_RUN
    m_pInputAllocator->ReleasePacketPtr(((HXCODEC_DATA*) pFrameToDecode->m_pData)->data);
    ((HXCODEC_DATA*) pFrameToDecode->m_pData)->data = NULL;

    retVal = DecodeDone(NULL);
#else	// _DRY_RUN
    // this is for Beta 2 codecs
    m_pCodecLib->PNStream_SetProperty(m_pStream, 
				      SP_OUPUT_QUEUE_STATUS, 
				      &theOutputQueueStatus);

    // this is for > beta 2 codecs
    m_pCodecLib->PNStream_SetProperty(m_pStream, 
				      SP_OUPUT_QUEUE_STATUS2, 
				      &theOutputQueueStatus);

    m_pCodecLib->PNStream_Input(m_pStream, 
				NULL, 
				(HXCODEC_DATA*) pFrameToDecode->m_pData);

    ((HXCODEC_DATA*) pFrameToDecode->m_pData)->data = NULL;
#endif	// _DRY_RUN
	
    return retVal;
}

void 
CMP4VDecoder::ReleaseBuffer(UINT8* pBuff)
{
    HXCODEC_DATA sData;
    sData.data = pBuff;
    m_pCodecLib->PNStream_ReleaseFrame(m_pStream,&sData);
}

/****************************************************************************
 *  Method:
 *    CRVDecoder::DecodeDone
 */
HX_RESULT CMP4VDecoder::DecodeDone(HXCODEC_DATA* pData)
{
    if (pData)
    {
	m_ulLastTimeStamp = pData->timestamp;
    }

    return m_pVideoFormat->DecodeDone(pData);
}


/****************************************************************************
 *  Method:
 *    CRVDecoder::GetImageInfo
 */
HX_RESULT CMP4VDecoder::GetImageInfo(HX_MOF* &pImageInfo)
{
    ULONG32 ulSize;
    ULONG32* pMofBuf = NULL;
    HX_RESULT retVal = HXR_OK;

    HX_VECTOR_DELETE(m_pImageInfoBuffer);

    retVal = m_pCodecLib->PNStream_GetStreamHeaderSize(m_pStream, 
						       &ulSize);

    if (retVal == HXR_OK)
    {
	HX_ASSERT((ulSize == sizeof(HX_FORMAT_IMAGE)) || 
		  (ulSize == sizeof(HX_FORMAT_IMAGE2)));

	pMofBuf = new ULONG32[ulSize / 4 + 1];

	retVal = HXR_OUTOFMEMORY;
	if (pMofBuf)
	{
	    retVal = HXR_OK;
	}
    }

    if (retVal == HXR_OK)
    {
	retVal = m_pCodecLib->PNStream_GetStreamHeader(m_pStream, 
						       (HX_MOF *) pMofBuf);
    }

    if (retVal == HXR_OK)
    {
	m_pImageInfoBuffer = pMofBuf;
	pMofBuf = NULL;
	pImageInfo = (HX_MOF*) m_pImageInfoBuffer;
    }

    HX_VECTOR_DELETE(pMofBuf);

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CRVDecoder::OnNewImage
 */
HX_RESULT CMP4VDecoder::OnNewImage(HXSTREAM streamRef, 
				   HXSTREAM fromStreamRef,	
				   HXCODEC_DATA* pData)
{
    CMP4VDecoder* pDecoder = (CMP4VDecoder*) streamRef;
    
    return pDecoder->DecodeDone(pData);
}

CRADynamicCodecLibrary* CMP4VDecoder::CreateCodecLibrary()
{
    return new CRADynamicCodecLibrary(m_pContext);
}


/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  Method:
 *    CMP4VDecoder::OpenCodec()
 */
HX_RESULT CMP4VDecoder::OpenCodec(HX_MOFTAG mofTag)
{
    HX_RESULT retVal = HXR_OK;

    HX_DELETE(m_pCodecLib);

    // Create Codec Library
    m_pCodecLib = CreateCodecLibrary();

    retVal = HXR_OUTOFMEMORY;
    if (m_pCodecLib != NULL)
    {
	retVal = HXR_OK;
    }

    // Load Codec into library
    if (SUCCEEDED(retVal))
    {
	retVal = m_pCodecLib->LoadCodecLib(mofTag);
        if (FAILED(retVal))
        {
            // Make sure we return HXR_REQUEST_UPGRADE
            // so that we know we failed due to not being able
            // to load the codec as opposed to some other reason
            retVal = HXR_REQUEST_UPGRADE;
        }
    }

    // Open the codec
    if (SUCCEEDED(retVal))
    {
	retVal = m_pCodecLib->PNCodec_Open(mofTag, &m_pCodec);

	if (FAILED(retVal))
	{
	    retVal = HXR_REQUEST_UPGRADE;
	}
    }

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CMP4VDecoder::OpenStream
 */
HX_RESULT CMP4VDecoder::OpenStream(HX_MOFTAG mofTag)
{
    ULONG32 ulMofInDataSize = 0;
    ULONG32* pMofInData = NULL;
    HX_FORMAT_NATIVE* pMofIn;
    HX_FORMAT_IMAGE2 mofOut;
    HXCODEC_INIT codecInit;
    HX_RESULT retVal = HXR_OK;

    if (SUCCEEDED(retVal))
    {
	ulMofInDataSize = (m_pVideoFormat->GetBitstreamHeaderSize() / 4) + 1;
	ulMofInDataSize += sizeof(HX_FORMAT_NATIVE);
	pMofInData = new ULONG32 [ulMofInDataSize];
 
	retVal = HXR_OUTOFMEMORY;
	if (pMofInData)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	pMofIn = (HX_FORMAT_NATIVE*) pMofInData;

	pMofIn->cbLength = m_pVideoFormat->GetBitstreamHeaderSize() + 
			   (pMofIn->data - ((UINT8*) pMofIn));
	pMofIn->moftag = HX_MEDIA_NATIVE;
	pMofIn->submoftag = STRINGTOMOFTAG(m_pCodecId);
#if defined HELIX_FEATURE_OMX_VIDEO_DECODER_MP4 || defined HELIX_FEATURE_OMX_VIDEO_DECODER_AVC1
//  Set MofTag to "MP4V" or "AVC1", so that omx video decoder wrapper can identify which decoder to load
    if(pMofIn->submoftag == 0x4f4d5856) // "OMXV"
    {
        IHXValues* pHeader = NULL;
        const char* pMimeTypeData = NULL;
        IHXBuffer* pMimeType = NULL;
 
        retVal = m_pVideoFormat->GetStreamHeader(pHeader);
        if (SUCCEEDED(retVal))
        {
	        retVal = pHeader->GetPropertyCString("MimeType", pMimeType);
        }
        if (SUCCEEDED(retVal))
        {
	        pMimeTypeData = (char*) pMimeType->GetBuffer();
            if (strcasecmp(pMimeTypeData, MP4V_HX_AVC1_PAYLOAD_MIME_TYPE) == 0)
	        {
                pMofIn->submoftag = 0x41564331; // AVC1
	        }
            else
            {
                pMofIn->submoftag = 0x4d503456; // MP4v
            }
        }
        HX_RELEASE(pMimeType);
    }
#endif
        if ((m_pVideoFormat->GetBitstreamHeaderSize() > 0)&& 
             (m_pVideoFormat->GetBitstreamHeader()))
        {
	    memcpy(pMofIn->data,
	        m_pVideoFormat->GetBitstreamHeader(),
	        m_pVideoFormat->GetBitstreamHeaderSize());
        }
        else
        {
            pMofIn->cbLength = 0;
        }
    }

    if (SUCCEEDED(retVal))
    {
	mofOut.cbLength = sizeof(HX_FORMAT_IMAGE2);
	mofOut.moftag = HX_MEDIA_IMAGE2;
	mofOut.submoftag = HX_YUV420_ID;
	m_moftagOut = HX_YUV420_ID;
    }

    if (SUCCEEDED(retVal))
    {
	codecInit.pInMof = (HX_MOF*) pMofIn;
	codecInit.pOutMof = (HX_MOF*) &mofOut;
	codecInit.memoryRef = (HXMEMORY) m_pInputAllocator;
	codecInit.pContext = m_pContext;
    }

    if (retVal == HXR_OK)
    {
	retVal = m_pCodecLib->PNCodec_StreamOpen(m_pCodec, 
						 &m_pStream,
						 &codecInit);
    }

    if (SUCCEEDED(retVal))
    {
        // Get the stream header from the video format
        IHXValues* pStreamHdr = NULL;
        retVal = m_pVideoFormat->GetStreamHeader(pStreamHdr);
        if (SUCCEEDED(retVal))
        {
            // Get the "FrameWidth" and "FrameHeight" properties
            UINT32 ulFrameWidth  = 0;
            UINT32 ulFrameHeight = 0;
            pStreamHdr->GetPropertyULONG32("FrameWidth", ulFrameWidth);
            pStreamHdr->GetPropertyULONG32("FrameHeight", ulFrameHeight);
            // If we have framewidth and frameheight, set them into the decoder
            if (ulFrameWidth && ulFrameHeight)
            {
	        m_pCodecLib->PNStream_SetProperty(m_pStream,
                                                  SP_FRAME_WIDTH,
                                                  &ulFrameWidth);
	        m_pCodecLib->PNStream_SetProperty(m_pStream,
                                                  SP_FRAME_HEIGHT,
                                                  &ulFrameHeight);
            }
        }
        HX_RELEASE(pStreamHdr);
    }

    if (retVal == HXR_OK)
    {
	retVal = m_pCodecLib->PNStream_SetDataCallback(
		    m_pStream, 
		    this,
		    (HXMEMORY) m_pOutputAllocator,
		    OnNewImage);
    }

    if (retVal == HXR_OK)
    {
	SetCodecQuality();
    }

    if (retVal == HXR_OK)
    {
	HXBOOL bAllowDifferentOutputSizes = TRUE; 

	//  tell codec we can handle different output sizes
	m_pCodecLib->PNStream_SetProperty(m_pStream, 
					  SP_ALLOW_DIFFERENT_OUPUT_SIZES, 
					  &bAllowDifferentOutputSizes); 
    }

    HX_VECTOR_DELETE(pMofInData);

    if (retVal == HXR_OK)
    {
       if (m_pVideoFormat && m_pVideoFormat->m_pMP4VideoRenderer &&
           m_pVideoFormat->m_pMP4VideoRenderer->IsUntimedRendering())
       {
           SetCPUScalability(FALSE);
       }
    }

    if (FAILED(retVal))
    {
        // Make sure we return HXR_REQUEST_UPGRADE
        // so that we know we failed due to not being able
        // to load the codec as opposed to some other reason
        retVal = HXR_REQUEST_UPGRADE;
    }

    return retVal;
}

/****************************************************************************
 *  Method:
 *    CMP4VDecoder::SetCPUScalability
 */
HX_RESULT CMP4VDecoder::SetCPUScalability(HXBOOL bVal)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pCodecLib)
    {
       retVal = m_pCodecLib->PNStream_SetProperty(m_pStream, SP_CPUSCALABILITY, &bVal);
    }

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CMP4VDecoder::GetQualityPreference
 */
HX_RESULT CMP4VDecoder::GetQualityPreference(UINT16 &usQuality)
{
    HX_RESULT pnr = HXR_OK;

    IHXBuffer* pBuffer = NULL;
    IHXPreferences* pPreferences = 0;

    if (!m_pContext || 
	(m_pContext->QueryInterface(IID_IHXPreferences, (void**) &pPreferences) !=
	 HXR_OK))
    {
	pnr = HXR_INVALID_PARAMETER;
    }
    else
    {
        if (pPreferences->ReadPref("Quality", pBuffer) == HXR_OK)
	{   
	    usQuality = ::atoi((const char*) pBuffer->GetBuffer());
	    HX_RELEASE(pBuffer);
	}
    }

    HX_RELEASE(pPreferences);

    return pnr;
}


/****************************************************************************
 *  Method:
 *    CMP4VDecoder::GetQualityPreference
 */
void CMP4VDecoder::SetCodecQuality(void)
{
    UINT16 usQualityPreference = 4;

    GetQualityPreference(usQualityPreference);
    
    // Turn on or off post filter based on flags in source URL
    // if the URL turns off post filter or the TLC quality
    // preference is < 2 we turn off the post filter
    HXBOOL bPostFilter = (usQualityPreference >= 2);

    m_pCodecLib->PNStream_SetProperty(m_pStream, 
				      SP_POSTFILTER,
				      &bPostFilter);

    // Turn on or off FRU based on flags in source URL
    // HXBOOL bTemporalInterp = (usQualityPreference >= 2);
    HXBOOL bTemporalInterp = FALSE;

    m_pCodecLib->PNStream_SetProperty(m_pStream, 
				      SP_TEMPORALINTERP,
				      &bTemporalInterp);

    // Turn on or off decode of B-frames based on flags in source URL
    // HXBOOL bDecodeBFrames = (usQualityPreference >= 2);
    HXBOOL bDecodeBFrames = FALSE;

    m_pCodecLib->PNStream_SetProperty(m_pStream, 
				      SP_DECODE_B_FRAMES,
				      &bDecodeBFrames);
}

HX_RESULT 
CMP4VDecoder::GetProperty(UINT32 id, void* pPropVal)
{
    return m_pCodecLib->PNStream_GetProperty(m_pStream, 
				      id,
				      pPropVal);
}
