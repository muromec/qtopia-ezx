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


/****************************************************************************
 *  Defines
 */
// #define _IGNORE_AUTOUPGRADE 

#if !defined(HELIX_FEATURE_MIN_HEAP)
#define MAX_BUFFERED_DECODED_MP4_FRAMES		14
#define MAX_DECODED_MP4_FRAMES_IN_STEP		3
#define DECODE_CALLBACK_RNGBUF_SIZE		10
#else	// HELIX_FEATURE_MIN_HEAP
#ifdef HELIX_CONFIG_MOBLIN
#define MAX_BUFFERED_DECODED_MP4_FRAMES		6
#else
#define MAX_BUFFERED_DECODED_MP4_FRAMES		4
#endif
#define MAX_DECODED_MP4_FRAMES_IN_STEP		3
#define DECODE_CALLBACK_RNGBUF_SIZE		10
#endif	// HELIX_FEATURE_MIN_HEAP

#define NON_KEYFRM_DCDE_FALLBEHIND_THRSHLD  33		// in milliseconds
#if defined(HELIX_FEATURE_MIN_HEAP)
#define MP4V_DEFAULT_PREROLL		    2000	// in milliseconds
#else
#define MP4V_DEFAULT_PREROLL		    3500	// in milliseconds
#endif

#define MAX_NONKEY_CODED_FRAME_FALLBEHIND -800

/****************************************************************************
 *  Includes
 */
#include "hlxclib/memory.h"

#include "mp4vdfmt.h"

#include "hxasm.h"
#include "hxwin.h"
#include "hxvsurf.h"
#include "hxvctrl.h"
#include "hxsite2.h"
#include "hxthread.h"
#include "hxmtypes.h"
#include "hxtick.h"
#include "hxassert.h"
#include "hxstrutl.h"
#include "unkimp.h"
#include "timeval.h"
#include "cttime.h"
#include "hxcodec.h"
#include "addupcol.h"

#include "mp4video.h"
#include "mp4vpyld.h"
#ifdef HELIX_FEATURE_VIDEO_CODEC_AVC1
#include "h264pyld.h"
#endif //HELIX_FEATURE_VIDEO_CODEC_AVC1
#if defined(HELIX_FEATURE_VIDEO_CODEC_VP6)
#include "flvpyld.h"
#endif //HELIX_FEATURE_VIDEO_CODEC_VP6
/****************************************************************************
 *  Locals
 */


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::CQTVideoFormat
 *
 */
CMP4VideoFormat::CMP4VideoFormat(IHXCommonClassFactory* pCommonClassFactory,
				 CMP4VideoRenderer* pMP4VideoRenderer,
				 HXBOOL bSecure)
    : CVideoFormat(pCommonClassFactory, pMP4VideoRenderer)
    , m_ulMaxDecodedFrames(0)
    , m_pMP4VideoRenderer(pMP4VideoRenderer)
    , m_pRssm(NULL)
    , m_pInputAllocator(NULL)
    , m_pDecodedRngBuf(NULL)
    , m_pDecoder(NULL)
    , m_ulWidthContainedInSegment(0)
    , m_ulHeightContainedInSegment(0)
    , m_pCodecOutputBIH(NULL)
    , m_bFirstDecode(TRUE)
    , m_bDecoderMemMgt(FALSE)
{
    HX_ASSERT(m_pCommonClassFactory);
    HX_ASSERT(pMP4VideoRenderer);

    m_sMediaSize.cx = 0;
    m_sMediaSize.cy = 0;
    // Register the payload format builders with m_fmtFactory
    RegisterPayloadFormats();
    m_pMP4VideoRenderer->AddRef();
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::~CMP4VideoFormat
 *
 */
CMP4VideoFormat::~CMP4VideoFormat()
{
    Close();
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::Close
 *
 */
void CMP4VideoFormat::Close(void)
{
    CVideoFormat::Close();

    HX_RELEASE(m_pMP4VideoRenderer);

    if (m_pRssm)
    {
	m_pRssm->Close();
	m_pRssm->Release();
	m_pRssm = NULL;
    }

    FlushDecodedRngBuf();
    HX_DELETE(m_pDecodedRngBuf);
    HX_DELETE(m_pDecoder);

    _Reset();

    HX_DELETE(m_pInputAllocator);
    HX_DELETE(m_pCodecOutputBIH);
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::Init
 */
HX_RESULT CMP4VideoFormat::Init(IHXValues* pHeader)
{
    HX_RESULT retVal = CVideoFormat::Init(pHeader);

    // MP4 video renderer requires frame pool to be created
    if (SUCCEEDED(retVal))
    {
	retVal = HXR_OUTOFMEMORY;
	if (m_pFramePool)
	{
	    retVal = HXR_OK;
	}
    }

    // Create Decode callback to caller ring buffer
    if (SUCCEEDED(retVal))
    {
	m_pDecodedRngBuf = new CRingBuffer(DECODE_CALLBACK_RNGBUF_SIZE);
	retVal = HXR_OUTOFMEMORY;
	if (m_pDecodedRngBuf)
	{
	    retVal = HXR_OK;
	}
    }

    // Create memory allocators
    if (SUCCEEDED(retVal))
    {
	retVal = CreateAllocators();
    }

    // Create Packet Assembler
    if (SUCCEEDED(retVal))
    {
	retVal = HXR_OUTOFMEMORY;
	retVal = m_fmtFactory.BuildFormat(m_pCommonClassFactory, FALSE, 
                                      pHeader, m_pRssm);
    }
    
    if (SUCCEEDED(retVal))
    {
	m_pRssm->SetAllocator(m_pInputAllocator);
    }

    // Create Decoder
    if (SUCCEEDED(retVal))
    {
	retVal = HXR_OUTOFMEMORY;
	m_pDecoder = CreateDecoder();
	if (m_pDecoder)
	{
	    retVal = HXR_OK;
	}
    }

    // Initialize Assembler
    if (SUCCEEDED(retVal) && m_pRssm)
    {

#ifdef _IGNORE_AUTOUPGRADE
	if (retVal == HXR_REQUEST_UPGRADE)
	{
	    retVal = HXR_FAIL;
	}
#endif	// _IGNORE_AUTOUPGRADE
	
#ifdef _IGNORE_UNSUPPORTED
	if (FAILED(retVal) && (retVal != HXR_REQUEST_UPGRADE))
	{
	    HXxSize nullViewFrame = {1, 1};
	    m_pMP4VideoRenderer->ResizeViewFrame(nullViewFrame);
	    HX_RELEASE(m_pRssm);
	    retVal = HXR_OK;
	}
#endif	// _IGNORE_UNSUPPORTED

    }

    // Initialize Decoder
    if (SUCCEEDED(retVal) && m_pRssm)
    {
        // Set the return value
        retVal = HXR_REQUEST_UPGRADE;
        // Loop through all possible codec IDs tha tthe depacketizer knows about
        while (retVal == HXR_REQUEST_UPGRADE)
        {
            retVal = m_pDecoder->Init(m_pMP4VideoRenderer->GetContext(),
				      this,
				      NULL,
				      m_pInputAllocator,
				      m_pMP4VideoRenderer->m_pOutputAllocator);
            if (retVal == HXR_REQUEST_UPGRADE)
            {
                // The depacketizer may know about more than one codec ID,
                // so we ask the depacketizer to advance to the next codec ID.
                HX_RESULT rv = m_pRssm->SetNextCodecId();
                // If the depacketizer was successful, then it has
                // another codec ID for us to try. If it returned failure,
                // then it doesn't know about any more codec IDs.
                if (FAILED(rv))
                {
                    // The depacketizer doesn't konw about any more codec ID's,
                    // so we will reset the depacketizer back to the first
                    // codec ID, so that it will be the one to show up in
                    // AU request.
                    m_pRssm->ResetCodecId();
                    // Now we need to break out of the loop
                    break;
                }
            }
        }
        // If we failed with HXR_REQUEST_UPGRADE, then we need to
        // populate the upgrade collection
        if (retVal == HXR_REQUEST_UPGRADE)
        {
            // Get the length of the codec ID string
            UINT32 ulIDLen = (UINT32) strlen(m_pRssm->GetCodecId());
            // Allocate a string
            char* pszAUStr = new char [ulIDLen + 1];
            if (pszAUStr)
            {
                // Copy the string into the buffer
                strcpy(pszAUStr, m_pRssm->GetCodecId());
                // Make sure it is lower-case
                strlwr(pszAUStr);
                // Add this to the upgrade collection
                AddToAutoUpgradeCollection(pszAUStr, m_pMP4VideoRenderer->GetContext());
            }
            HX_VECTOR_DELETE(pszAUStr);
        }
	if (SUCCEEDED(retVal))
	{
	    HX_MOF* pImageInfo;
	    m_pDecoder->GetProperty(SP_HWMEM_MGT, &m_bDecoderMemMgt);
	    if (m_pDecoder->GetImageInfo(pImageInfo) == HXR_OK)
	    {
		retVal = SetupOutputFormat(pImageInfo);

		if (SUCCEEDED(retVal))
		{
		    if ((m_sMediaSize.cx != 0) && (m_sMediaSize.cy != 0))
		    {
			m_pMP4VideoRenderer->ResizeViewFrame(m_sMediaSize);
		    }
		}
	    }
	}

#ifdef _IGNORE_AUTOUPGRADE
	if (retVal == HXR_REQUEST_UPGRADE)
	{
	    retVal = HXR_FAIL;
	}
#endif	// _IGNORE_AUTOUPGRADE
	
#ifdef _IGNORE_UNSUPPORTED
	if (FAILED(retVal) && (retVal != HXR_REQUEST_UPGRADE))
	{
	    HXxSize nullViewFrame = {1, 1};
	    m_pMP4VideoRenderer->ResizeViewFrame(nullViewFrame);
	    HX_RELEASE(m_pRssm);
	    retVal = HXR_OK;
	}
#endif	// _IGNORE_UNSUPPORTED
    }

    if (SUCCEEDED(retVal))
    {
        m_ulMaxDecodedFrames = GetDecodedFrameQueueCapacity();
    }

    m_bFirstDecode = TRUE;

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::SetupOutputFormat
 *
*/
HX_RESULT
CMP4VideoFormat::SetupOutputFormat(HX_MOF* pMof)
{
    HX_RESULT retVal = HXR_OK;
    HX_FORMAT_IMAGE* pMofOutI = (HX_FORMAT_IMAGE*) pMof;

    if (!m_pCodecOutputBIH)
    {
        m_pCodecOutputBIH = new HXBitmapInfoHeader;
        if (!m_pCodecOutputBIH)
        {
            return HXR_OUTOFMEMORY;
        }
        memset(m_pCodecOutputBIH, 0, sizeof(HXBitmapInfoHeader));
    }

    m_sMediaSize.cy = pMofOutI->uiHeight;
    m_sMediaSize.cx = pMofOutI->uiWidth;

    UINT32 ulColorType;
    switch (pMofOutI->submoftag)
    {
        case HX_RGB3_ID:    ulColorType = HX_RGB; break;
        case HX_YUY2_ID:    ulColorType = HX_YUY2; break;
        case HX_NV21_ID:    ulColorType = HX_NV21; break;
        case HX_OMXV_ID:    ulColorType = HX_OMXV; break;
        default:            ulColorType = HX_I420; break;
    }

    m_pCodecOutputBIH->biBitCount = pMofOutI->uiBitCount;
    m_pCodecOutputBIH->biCompression = ulColorType;
    m_pCodecOutputBIH->biWidth = pMofOutI->uiWidth;
    m_pCodecOutputBIH->biHeight = pMofOutI->uiHeight;
    m_pCodecOutputBIH->biPlanes = 1;

    m_pCodecOutputBIH->biSizeImage =
        (m_pCodecOutputBIH->biWidth *
         m_pCodecOutputBIH->biBitCount *
         m_pCodecOutputBIH->biHeight +
         7) / 8;

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::GetDefaultPreroll
 *
 */
ULONG32 CMP4VideoFormat::GetDefaultPreroll(IHXValues* pValues)
{
    return MP4V_DEFAULT_PREROLL;
}


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::GetCodecId
 */
const char* CMP4VideoFormat::GetCodecId(void)
{
    const char* pId = NULL;

    if (m_pRssm)
    {
	pId = m_pRssm->GetCodecId();
    }

    return pId;
}


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::GetBitstreamHeaderSize
 */
ULONG32 CMP4VideoFormat::GetBitstreamHeaderSize(void)
{
    ULONG32 ulSize = 0;

    if (m_pRssm)
    {
	ulSize = m_pRssm->GetBitstreamHeaderSize();
    }

    return ulSize;
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::GetBitstreamHeader
 */
const UINT8* CMP4VideoFormat::GetBitstreamHeader(void)
{
    const UINT8* pHeader = NULL;

    if (m_pRssm)
    {
	pHeader = m_pRssm->GetBitstreamHeader();
    }

    return pHeader;
}

HX_RESULT CMP4VideoFormat::GetStreamHeader(REF(IHXValues*) rpValues)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pHeader)
    {
        // Make sure our out parameter is initially NULL
        HX_ASSERT(rpValues == NULL);
        // Assign the out parameter
        rpValues = m_pHeader;
        // AddRef before going out
        rpValues->AddRef();
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::SetCPUScalability
 */
HX_RESULT CMP4VideoFormat::SetCPUScalability(HXBOOL bVal)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pDecoder)
    {
       retVal = m_pDecoder->SetCPUScalability(bVal);
    }

    return retVal;
}

/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::GetMaxDecodedFrames
 *
 */
ULONG32 CMP4VideoFormat::GetMaxDecodedFrames(void)
{
    return MAX_BUFFERED_DECODED_MP4_FRAMES;
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::GetMaxDecodedFramesInStep
 *
 */
ULONG32 CMP4VideoFormat::GetMaxDecodedFramesInStep(void)
{
    return MAX_DECODED_MP4_FRAMES_IN_STEP;
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::CreateAssembledPacket
 */
CMediaPacket* CMP4VideoFormat::CreateAssembledPacket(IHXPacket* pCodecData)
{
    ULONG32* pCodecPacketRaw = NULL;
    HXCODEC_DATA* pCodecPacket;
    CMediaPacket* pFramePacket = NULL;
    HX_RESULT status = HXR_OK;

#ifdef _IGNORE_UNSUPPORTED
    if (!m_pRssm)
    {
	return NULL;
    }
#endif // _IGNORE_UNSUPPORTED

    m_pMP4VideoRenderer->BltIfNeeded();

    if (pCodecData)
    {
    status = m_pRssm->SetPacket(pCodecData);
    if( status == HXR_OUTOFMEMORY )
    {
	m_LastError = status;
        return NULL;
    }
    }
    else
    {
        // CreateAssembledPacket called with NULL, so
        // call Flush on the depacketizer
        status = m_pRssm->Flush();
    }
    m_pRssm->CreateHXCodecPacket(pCodecPacketRaw);
    pCodecPacket = (HXCODEC_DATA*) pCodecPacketRaw;
    
    while (pCodecPacket)
    {
	pFramePacket = new CMediaPacket(
				pCodecPacket,
				(UINT8*) pCodecPacket, 
				pCodecPacket->dataLength,
				pCodecPacket->dataLength,
				pCodecPacket->timestamp,
				pCodecPacket->flags, //0,
				NULL);
	
	if (pFramePacket == NULL)
	{
	    HX_ASSERT(HXR_OUTOFMEMORY == HXR_OK);
	    KillInputBuffer(pCodecPacket, this);
            m_LastError = HXR_OUTOFMEMORY;
            return NULL;
	}
	else
	{
	    pFramePacket->SetBufferKiller(KillInputBuffer);
	    pFramePacket->m_pUserData = this;
	}
	
	m_pMP4VideoRenderer->BltIfNeeded();

	pCodecPacketRaw = NULL;
	m_pRssm->CreateHXCodecPacket(pCodecPacketRaw);
	pCodecPacket = (HXCODEC_DATA*) pCodecPacketRaw;

	if (pCodecPacket)
	{
	    ReturnAssembledPacket(pFramePacket);
	    pFramePacket = NULL;
	}
    }
    
    // Are we flushing?
    if (!pCodecData)
    {
        // Create a dummy HXCODEC_DATA to inform the 
        // decoder that no more packets are coming
        CMediaPacket* pDummy = CreateFinalDummyFrame();
        if (pDummy)
        {
            // Did we already have an outgoing frame packet?
            if (pFramePacket)
            {
                // We already had an outgoing packet, so return it
                ReturnAssembledPacket(pFramePacket);
                pFramePacket = NULL;
            }
            // Return the dummy frame
            ReturnAssembledPacket(pDummy);
        }
    }

    return pFramePacket;
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::Reset
 */
void CMP4VideoFormat::Reset()
{
    _Reset();
    CVideoFormat::Reset();
}

void CMP4VideoFormat::_Reset(void)
{
    if (m_pRssm)
    {
	m_pRssm->Reset();
	m_pRssm->SetTimeAnchor(GetStartTime());
    }

    m_bFirstDecode = TRUE;
    m_ulWidthContainedInSegment = 0;
    m_ulHeightContainedInSegment = 0;
    m_sMediaSize.cx = 0;
    m_sMediaSize.cy = 0;
}

void CMP4VideoFormat::FlushDecodedRngBuf(void)
{
    if (m_pDecodedRngBuf)
    {
	HXCODEC_DATA* pFrame;
	
	while ((pFrame = ((HXCODEC_DATA*) m_pDecodedRngBuf->Get())))
	{
	    ReleaseDecodedPacket(pFrame);
	}
    }
}

void CMP4VideoFormat::ReleaseDecodedPacket(HXCODEC_DATA* &pDecodedPacket)
{
    HX_ASSERT(pDecodedPacket);

    if (pDecodedPacket)
    {
        KillMP4VSampleDesc(pDecodedPacket, this);
        pDecodedPacket = NULL;
    }
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::CreateDecodedPacket
 *
 */
CMediaPacket* CMP4VideoFormat::CreateDecodedPacket(CMediaPacket* pFrameToDecode)
{
    HXCODEC_DATA* pDecodedPacket = NULL;
    CMediaPacket* pDecodedFrame = NULL;
    HXBOOL bInputFrameProcessed = FALSE;
    HX_RESULT status = HXR_OK;
 
    HX_ASSERT(pFrameToDecode);

#if defined(HELIX_FEATURE_VIDEO_MPEG4_DISCARD_LATE_ENCODED_FRAMES)
// discard late frames before they are decoded

    if (m_pMP4VideoRenderer->IsActive())
    {
	LONG32 lTimeAhead;

	lTimeAhead = m_pMP4VideoRenderer->ComputeTimeAhead(
				pFrameToDecode->m_ulTime, 
				0);

	if (lTimeAhead < MAX_NONKEY_CODED_FRAME_FALLBEHIND)
	{
	    // Throw away this frame
	    pFrameToDecode->Clear();
	    delete pFrameToDecode;
	    pFrameToDecode = NULL;
        //this will break out of loop once ringbuffer is empty
        bInputFrameProcessed = TRUE;

#if defined(HELIX_FEATURE_STATS)
	    m_pMP4VideoRenderer->ReportDroppedFrame();
#endif /* #if defined(HELIX_FEATURE_STATS) */
	}
    }
#endif // HELIX_FEATURE_VIDEO_MPEG4_DISCARD_LATE_ENCODED_FRAMES

    // Handle restart
    if (m_bFirstDecode)
    {
	m_bFirstDecode = FALSE;
	FlushDecodedRngBuf();
    }

    // Check for decoded frames already available
    pDecodedPacket = (HXCODEC_DATA*) m_pDecodedRngBuf->Get();

    // Loop processing frames in the following order:
    // 1. Return (queue) currently outstanding decoded frames (mormally is 0)
    // 2. Decode input frame (must do unless frame dropped pre-decode)
    // 3. Return (queue) frames produced synchronously with this decode
    // The proces wil stop short if the frames can no longer be returned
    // (queued).  This may occur due to output queue becomming full when
    // MAX_DECODED_RV_FRAMES_IN_STEP is set too low and decoder is performing
    // frame rate upsampling.
    do
    {
	m_pMP4VideoRenderer->BltIfNeeded();

	// If frame is decoded, return it to be queued
	if (pDecodedFrame)
	{
	    if (!ReturnDecodedPacket(pDecodedFrame))
	    {
		// Unable to return decoded frame - throw it away
		m_pFramePool->Put(pDecodedFrame);
		m_pMP4VideoRenderer->ReportDroppedFrame();
	    }
	    pDecodedFrame = NULL;
	}

	// If decoded packet isn't available, create one by decoding the
	// frame to decode
	if (!pDecodedPacket && pFrameToDecode)
	{
	    ProcessAssembledFrame(pFrameToDecode);

	    status = m_pDecoder->Decode(pFrameToDecode, 
					MAX_DECODE_QUALITY);

	    bInputFrameProcessed = TRUE;

	    if (status == HXR_OUTOFMEMORY)
	    {
		m_LastError = HXR_OUTOFMEMORY;
	    }

	    pDecodedPacket = (HXCODEC_DATA*) m_pDecodedRngBuf->Get();
	}

#if !defined(HELIX_FEATURE_VIDEO_MPEG4_DISCARD_LATE_ENCODED_FRAMES)
	if (pDecodedPacket && !m_pMP4VideoRenderer->IsUntimedMode())
	{
	    if (m_pMP4VideoRenderer->ComputeTimeAhead(
		    pDecodedPacket->timestamp, 
		    0) 
		< NON_KEYFRM_DCDE_FALLBEHIND_THRSHLD)
	    {
		ReleaseDecodedPacket(pDecodedPacket);
	    }
	}
#endif // HELIX_FEATURE_VIDEO_MPEG4_DISCARD_LATE_ENCODED_FRAMES

	// Package the decoded frame into a media packet
	if (pDecodedPacket)
	{
	    CMediaPacket* pVideoPacket = NULL;

	    // Obtain frame dimensions if not set
	    if (m_sMediaSize.cx == 0) 
	    {
		HX_MOF* pImageInfo;
		
		if (m_pDecoder->GetImageInfo(pImageInfo) == HXR_OK)
		{
		    status = SetupOutputFormat(pImageInfo);
		    
		    if (SUCCEEDED(status))
		    {
			if ((m_sMediaSize.cx != 0) && (m_sMediaSize.cy != 0))
			{
			    m_pMP4VideoRenderer->ResizeViewFrame(m_sMediaSize);
			}
		    }
		}
	    }

	    // Form decoded media packets if dimensions set
	    if (m_sMediaSize.cx != 0)
	    {
		pVideoPacket = (CMediaPacket*) m_pFramePool->Get(0);

		if (pVideoPacket == NULL)
		{
		    if (bInputFrameProcessed && pFrameToDecode)
		    {
			// Reuse media packet from coded frame for decoded frame
			pVideoPacket = pFrameToDecode;
			pFrameToDecode = NULL;
		    }
		    else
		    {
			// Create new media packet to store decoded frame
			pVideoPacket = new CMediaPacket();
			
			if (!pVideoPacket)
			{
			    status = HXR_OUTOFMEMORY;
			    m_LastError = status;
			}
		    }
		}
	    }

	    if (pVideoPacket)
	    {
		pVideoPacket->SetBuffer(pDecodedPacket->data,
					pDecodedPacket->data,
					pDecodedPacket->dataLength,
					pDecodedPacket->dataLength,
					FALSE);
		
		pVideoPacket->Init(pDecodedPacket->data,
				   pDecodedPacket->dataLength,
				   pDecodedPacket->timestamp,
				   0,
				   pDecodedPacket); // sample description
		
		// Data is now linked through VideoPacket
		pDecodedPacket->data = NULL;
		pDecodedPacket->dataLength = 0;
		
		pVideoPacket->SetBufferKiller(KillOutputBuffer);
		pVideoPacket->SetSampleDescKiller(KillMP4VSampleDesc);
		pVideoPacket->m_pUserData = this;
		
		pDecodedPacket = NULL;
		
		pDecodedFrame = pVideoPacket;
		pVideoPacket = NULL;
	    }
	    
	    if (pVideoPacket)
	    {
		delete pVideoPacket;
	    }
	}

	// Decoded packet should be packaged inside of a media packet.
	// If still present here, release it as something has gone wrong.
	if (pDecodedPacket)
	{
	    ReleaseDecodedPacket(pDecodedPacket);
	    m_pMP4VideoRenderer->ReportDroppedFrame();
	}
	
	// See if more decoded frames are avaialable
	if (CanReturnDecodedPacket())
	{
	    pDecodedPacket = (HXCODEC_DATA*) m_pDecodedRngBuf->Get();
	}
    } while ((pDecodedPacket || (!bInputFrameProcessed)) && 
	     (status == HXR_OK));

    if (pFrameToDecode != NULL)
    {
	pFrameToDecode->Clear(); 
	delete pFrameToDecode;
    }

    if (pDecodedPacket && !CanReturnDecodedPacket())
    {
	// Unable to return decoded frame - throw it away
	m_pFramePool->Put(pDecodedFrame);
	m_pMP4VideoRenderer->ReportDroppedFrame();
	pDecodedFrame = NULL;
    }
		
    return pDecodedFrame;
}

void CMP4VideoFormat::OnPacketsEnded()
{
    CreateAssembledPacket(NULL);
}

CMP4VDecoder* CMP4VideoFormat::CreateDecoder()
{
    return new CMP4VDecoder();
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::OnDecodedPacketRelease
 *
 */
void CMP4VideoFormat::OnDecodedPacketRelease(CMediaPacket* &pPacket)
{
    if (pPacket)
    {
	pPacket->Clear();
    }

    CVideoFormat::OnDecodedPacketRelease(pPacket);
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::DecodeDone
 */
HX_RESULT CMP4VideoFormat::DecodeDone(HXCODEC_DATA* pData)
{
    HX_RESULT retVal = HXR_OK;

    m_pMP4VideoRenderer->BltIfNeeded();

    if (pData)
    {
	ULONG32 ulFrameSize = HXCODEC_PTR_POPULATED_SIZE(pData);
	HXCODEC_DATA* pFrame = (HXCODEC_DATA*) new ULONG32[ulFrameSize / 4 + 1];

	m_pMP4VideoRenderer->ReportDecodedFrame();

	if (pFrame)
	{
	    memcpy(pFrame, pData, ulFrameSize); /* Flawfinder: ignore */

	    if (!m_pDecodedRngBuf->Put(pFrame))
	    {
		    ReleaseDecodedPacket(pFrame);
		    m_pMP4VideoRenderer->ReportDroppedFrame();
	    }
	}
	else
	{
	    if (pData->data)
	    {
            _KillOutputBuffer(pData->data);
            m_pMP4VideoRenderer->ReportDroppedFrame();
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::IsBitmapFormatChanged
 *
 */
HXBOOL CMP4VideoFormat::IsBitmapFormatChanged(
			    HXBitmapInfoHeader &BitmapInfoHeader,
			    CMediaPacket* pVideoPacket)
{
    HXBOOL bRetVal = (m_ulWidthContainedInSegment == 0);
    HXCODEC_DATA* pData = (HXCODEC_DATA*) pVideoPacket->m_pSampleDesc;
	
    if (pData &&
	(pData->flags & HX_SEGMENT_CONTAINS_OUTPUT_SIZE_FLAG))
    {
	bRetVal = (bRetVal ||
	    ((m_ulWidthContainedInSegment != pData->Segments[1].ulSegmentOffset) ||
	     (m_ulHeightContainedInSegment != ((ULONG32) pData->Segments[1].bIsValid))));
    }

    return bRetVal;
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::InitBitmapInfoHeader
 */
HX_RESULT CMP4VideoFormat::InitBitmapInfoHeader(
    HXBitmapInfoHeader &bitmapInfoHeader,
    CMediaPacket* pVideoPacket)
{
    if (m_pCodecOutputBIH)
    {
	HXCODEC_DATA* pData = (HXCODEC_DATA*) pVideoPacket->m_pSampleDesc;

	if (pData && (pData->flags & HX_SEGMENT_CONTAINS_OUTPUT_SIZE_FLAG))
	{
	    if ((m_ulWidthContainedInSegment != pData->Segments[1].ulSegmentOffset) ||
	        (m_ulHeightContainedInSegment != ((ULONG32) pData->Segments[1].bIsValid)))
	    {
		m_pCodecOutputBIH->biWidth =  m_ulWidthContainedInSegment = 
		    pData->Segments[1].ulSegmentOffset;
		m_pCodecOutputBIH->biHeight = m_ulHeightContainedInSegment = 
		    ((ULONG32) pData->Segments[1].bIsValid);

		m_pCodecOutputBIH->biSizeImage =
		    (m_pCodecOutputBIH->biWidth * 
		     m_pCodecOutputBIH->biBitCount * 
		     m_pCodecOutputBIH->biHeight +
		     7) / 8;
	    }
	}
	else
	{
	    m_ulWidthContainedInSegment = m_pCodecOutputBIH->biWidth;
	    m_ulHeightContainedInSegment = m_pCodecOutputBIH->biHeight;
	}
         
	bitmapInfoHeader = *m_pCodecOutputBIH;
    }

    return HXR_OK;
}

void 
CMP4VideoFormat:: DecoderReleaseBuffer(UINT8* pBuff)
{
    m_pDecoder->ReleaseBuffer(pBuff);
}

/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::CreateAllocators()
 */
HX_RESULT CMP4VideoFormat::CreateAllocators(void)
{
    HX_RESULT retVal = HXR_OK;
    HX20ALLOCPROPS allocRequest, allocActual;

    HX_RELEASE(m_pInputAllocator);
    HX_DELETE(m_pMP4VideoRenderer->m_pOutputAllocator);
    
    // setup the input allocator for the codec
    if (retVal == HXR_OK)
    {
	m_pInputAllocator = new CHXBufferMemoryAllocator(m_pMP4VideoRenderer->GetContext(), TRUE);

	retVal = HXR_OUTOFMEMORY;
	if (m_pInputAllocator)
	{
	    m_pInputAllocator->AddRef();
	    m_pInputAllocator->GetProperties(&allocRequest);
	    allocRequest.nNumBuffers = 0;   // No retention
	    m_pInputAllocator->SetProperties(&allocRequest, 
					     &allocActual);
	    retVal = HXR_OK;
	}
    }

    // setup the output allocator for the codec
    if (retVal == HXR_OK)
    {
	m_pMP4VideoRenderer->m_pOutputAllocator = new CHXMemoryAllocator(m_pMP4VideoRenderer->GetContext(),TRUE);

	retVal = HXR_OUTOFMEMORY;
	if (m_pMP4VideoRenderer->m_pOutputAllocator)
	{
	    m_pMP4VideoRenderer->m_pOutputAllocator->AddRef();
	    m_pMP4VideoRenderer->m_pOutputAllocator->GetProperties(&allocRequest);
	    // Allocate sufficient number of buffers to retain 
	    // (does not hur to overallocateas memory is not spent - this is a
	    //  only a container):
	    // 1 for active frame (displayed)
	    // 1 for frame in transit
	    // 3 for frames retained by codec
	    // 5 for frames upsampled by codec
	    // post decode queue depth * 2 (x2 to be on safe side)
	    // Note: it does not really hurt the memory consumption to set the
	    // allocator pooling high since the system is designed not to
	    // build the pool beyond what it needs.
	    allocRequest.nNumBuffers = 10 + GetMaxDecodedFrames() * 2;
	    m_pMP4VideoRenderer->m_pOutputAllocator->SetProperties(&allocRequest, 
								&allocActual);
	    retVal = HXR_OK;
	}
    }

#if _MACINTOSH
    // TODO: we want to pre-allocate the output buffers here so they come
    // from the standard memory pools instead of the interrupt memory
    // pools.
#endif	// _MACINTOSH

    return retVal;
}

CMediaPacket* CMP4VideoFormat::CreateFinalDummyFrame()
{
    CMediaPacket* pRet = NULL;

    // For some codecs we need to tell the decoder we have passed
    // it the last frame. Otherwise, we will not get the last frame
    // out of the decoder, due to the frame delay caused by B-frames.
    // Therefore, we need to create a dummy HXCODEC_DATA with special
    // parameters and pass this into the codec.
    //
    // Produce a dummy HXCODEC_DATA
    HXCODEC_DATA* pCodecData = new HXCODEC_DATA;
    if (pCodecData)
    {
        // Set the values
        pCodecData->dataLength  = 0;
        pCodecData->data        = NULL;
//        pCodecData->timestamp   = m_ulLastTimestamp;
//        pCodecData->sequenceNum = m_ulLastSequenceNumber + 1;
        pCodecData->timestamp   = 0;
        pCodecData->sequenceNum = 0;
        pCodecData->flags       = 0; // don't care
        pCodecData->lastPacket  = TRUE;
        pCodecData->numSegments = 1;
        pCodecData->Segments[0].bIsValid        = FALSE;
        pCodecData->Segments[0].ulSegmentOffset = 0;
        // Now create the media packet
        pRet = new CMediaPacket(pCodecData,
                                (UINT8*) pCodecData, 
                                pCodecData->dataLength,
                                pCodecData->dataLength,
                                pCodecData->timestamp,
                                pCodecData->flags,
                                NULL);
        if (pRet)
        {
            pRet->SetBufferKiller(KillInputBuffer);
            pRet->m_pUserData = this;
        }
    }

    return pRet;
}

/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::KillMP4VSampleDesc
 *
 */
void CMP4VideoFormat::KillMP4VSampleDesc(void* pSampleDesc, void* pUserData)
{
    if (pSampleDesc)
    {
	HXCODEC_DATA* pFrame = (HXCODEC_DATA*) pSampleDesc;
	ULONG32* pFrameData = (ULONG32*) pFrame;

	if (pFrame->data)
	{
	    KillOutputBuffer(pFrame->data, pUserData);
	}

	delete [] pFrameData;
    }
}


/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::KillInputBuffer
 *
 */
void CMP4VideoFormat::KillInputBuffer(void* pBuffer, void* pUserData)
{
    if (pBuffer)
    {
	HXCODEC_DATA* pDeadData = (HXCODEC_DATA*) pBuffer;
	CMP4VideoFormat* pVidFmt = (CMP4VideoFormat*) pUserData;

	HX_ASSERT(pUserData);

	if (pDeadData->data)
	{
	    pVidFmt->m_pInputAllocator->ReleasePacketPtr(pDeadData->data);
	}

	delete[] ((ULONG32*) pDeadData);
    }
}

void CMP4VideoFormat::_KillOutputBuffer(void* pBuffer)
{
    if(m_bDecoderMemMgt)
    {
        DecoderReleaseBuffer((UINT8*)pBuffer);
        
    }
    else
    {
         if (m_pMP4VideoRenderer)
         {
             m_pMP4VideoRenderer->m_pOutputAllocator->ReleasePacketPtr((UINT8*) pBuffer);
         }
    }
}

/****************************************************************************
 *  Method:
 *    CMP4VideoFormat::KillOutputBuffer
 *
 */
void CMP4VideoFormat::KillOutputBuffer(void* pBuffer, void* pUserData)
{
    if (pBuffer)
    {
        CMP4VideoFormat* pVidFmt = (CMP4VideoFormat*)pUserData;
        HX_ASSERT(pVidFmt);
        if (pVidFmt)
        {
            pVidFmt->_KillOutputBuffer(pBuffer);
        }
    }
}

void CMP4VideoFormat::RegisterPayloadFormats()
{
    // Register the various payload format builder functions
    // with the payload format factory

    // MPEG4 video format
    m_fmtFactory.RegisterBuilder(&MP4VPayloadFormat::Build);

#if defined(HELIX_FEATURE_VIDEO_CODEC_AVC1 ) 
	// H264 video format
    m_fmtFactory.RegisterBuilder(&H264PayloadFormat::Build);
#endif	// defined(HELIX_FEATURE_VIDEO_CODEC_AVC1)
#if defined(HELIX_FEATURE_VIDEO_CODEC_VP6)
    m_fmtFactory.RegisterBuilder(&CHXFLVPayloadFormat::Build);
#endif
}
