/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h263vidfmt.cpp,v 1.15 2005/11/02 15:34:54 ehyche Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * ***** END LICENSE BLOCK ***** */

/****************************************************************************
 *  Defines
 */
#define _OVERALLOC_CODEC_DATA	3

#define MP4V_RN_3GPP_H263_PAYLOAD_MIME_TYPE "video/X-RN-3GPP-H263"

#if defined(HELIX_FEATURE_VIDEO_H263_FAVOR_CORRUPTEDFRAMES_OVER_JITTER)
#define NON_KEYFRM_DCDE_FALLBEHIND_THRSHLD  -33	// in milliseconds
#else
#define NON_KEYFRM_DCDE_FALLBEHIND_THRSHLD  -800	// in milliseconds
#endif // HELIX_FEATURE_VIDEO_H263_FAVOR_CORRUPTEDFRAMES_OVER_JITTER

#define DFLT_MAX_IMG_WIDTH	176
#define DFLT_MAX_IMG_HEIGHT	144

#define CCCC_ENCODE(b1, b2, b3, b4)	((b1 << 24) |	\
					 (b2 << 16) |	\
					 (b3 << 8)  |	\
					 b4)


/****************************************************************************
 *  Includes
 */

#include "h263vidfmt.h"

#include "hxasm.h"
#include "hxwin.h"
#include "hxvsurf.h"
// #include "hxvctrl.h"
#include "hxsite2.h"
#include "hxthread.h"

#include "hxtick.h"
#include "hxassert.h"
#include "hxstrutl.h"
#include "hxmarsh.h"
#include "unkimp.h"
#include "timeval.h"
#include "cttime.h"

// #include "qtbatom.h"

#include "h263video.h"


/****************************************************************************
 *  Locals
 */


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::CQTVideoFormat
 *
 */
CH263VideoFormat::CH263VideoFormat(IHXCommonClassFactory* pCommonClassFactory,
				   CH263VideoRenderer* pH263VideoRenderer)
    : CVideoFormat(pCommonClassFactory, pH263VideoRenderer)
    , m_pDecoder(NULL)
    , m_pH263VideoRenderer(pH263VideoRenderer)
    , m_pRssm(NULL)
    , m_pMaxDims(NULL)
    , m_pAssmDims(NULL)
    , m_ulDecoderBufSize(0)
{
    HX_ASSERT(m_pCommonClassFactory);
    HX_ASSERT(pH263VideoRenderer);

    m_pH263VideoRenderer->AddRef();
    _Reset();
}


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::~CQTVideoFormat
 *
 */
CH263VideoFormat::~CH263VideoFormat()
{
    HX_RELEASE(m_pH263VideoRenderer);

    if (m_pRssm)
    {
	m_pRssm->Close();
	m_pRssm->Release();
	m_pRssm = NULL;
    }

    HX_DELETE(m_pDecoder);
    HX_DELETE(m_pMaxDims);

    _Reset();
}


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::Init
 *
 */
HX_RESULT CH263VideoFormat::Init(IHXValues* pHeader)
{
    IHXBuffer* pMimeType = NULL;
    const char* pMimeTypeData = NULL;
    HX_RESULT retVal = CVideoFormat::Init(pHeader);

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
	if (strcasecmp(pMimeTypeData, 
		       MP4V_RN_3GPP_H263_PAYLOAD_MIME_TYPE) == 0)
	{
	    IHXBuffer* pConfigData = NULL;

	    retVal = pHeader->GetPropertyBuffer("OpaqueData", pConfigData);

	    if (SUCCEEDED(retVal))
	    {
		retVal = ConfigFrom3GPPHeader(pConfigData);
	    }
	    else
	    {
            	// OpaqueData is optional - can init from stream
            	retVal = HXR_OK;
	    }

	    HX_RELEASE(pConfigData);

	    // This payload is raw H263 frames and needs no assembly
	    HX_RELEASE(m_pRssm);
	}
	else
	{
	    // Assume H263 payload - Create Packet Assembler
	    retVal = HXR_OUTOFMEMORY;
	    m_pRssm = new CH263PayloadFormat();
	    if (m_pRssm)
	    {
		m_pRssm->AddRef();
		retVal = HXR_OK;
	    }
	}
    }

    HX_RELEASE(pMimeType);

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

    if (SUCCEEDED(retVal) && m_pRssm)
    {
	retVal = m_pRssm->Init(m_pCommonClassFactory, FALSE);
    }
    
    if (SUCCEEDED(retVal) && m_pRssm)
    {
	retVal = m_pRssm->SetStreamHeader(pHeader);
    }

    // The depacketizer may have set "FrameWidth" and "FrameHeight" in
    // SetStreamHeader(), so we wait until now to check whether
    // these are set in the stream header
    UINT32 ulTmp1 = 0;
    UINT32 ulTmp2 = 0;
    if (SUCCEEDED(pHeader->GetPropertyULONG32("FrameWidth",  ulTmp1)) &&
        SUCCEEDED(pHeader->GetPropertyULONG32("FrameHeight", ulTmp2)))
    {
        m_DesiredFrameSize.cx = (INT32) ulTmp1;
        m_DesiredFrameSize.cy = (INT32) ulTmp2;
    }

    m_DecoderDims.cx = 0;
    m_DecoderDims.cy = 0;

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::CreateAssembledPacket
 *
 */
CMediaPacket* CH263VideoFormat::CreateAssembledPacket(IHXPacket* pCodecData)
{
    CMediaPacket* pFramePacket = NULL;
    IHXPacket* pPacket = NULL;

    m_pH263VideoRenderer->BltIfNeeded();

    if (m_pRssm)
    {
	m_LastError = m_pRssm->SetPacket(pCodecData);
        if( m_LastError == HXR_OUTOFMEMORY )
        {
	    return NULL;
        }
	m_pRssm->GetPacket(pPacket);
    }
    else if (!pCodecData->IsLost())
    {
	// Since there is no packet assembler - assume no assembly is needed
	pPacket = pCodecData;
	pPacket->AddRef();
    }

    if (pPacket)
    {
	HXxSize FrameDims;
	HXxSize* pFrameDims = NULL;
	IHXBuffer* pBuffer = pPacket->GetBuffer();

	if (pBuffer)
	{
#ifdef _TRUST_MAX_DIMS_WHEN_AVAILABLE
	    if (!m_pMaxDims)
#endif	// _TRUST_MAX_DIMS_WHEN_AVAILABLE
	    {
		HX_RESULT status = 
		    CH263PayloadFormat::GetFrameDimensions(pBuffer, FrameDims);

		if (status == HXR_NO_DATA)
		{
		    // This is a custom size H263+ and we do not parse the
		    // bitstream deep enough to get to the custom size
		    if (m_pMaxDims &&
			(m_pMaxDims->cx != 0) &&
			(m_pMaxDims->cy != 0))
		    {
			FrameDims = *m_pMaxDims;
		    }
		    else
		    {
			FrameDims.cx = DFLT_MAX_IMG_WIDTH;
			FrameDims.cy = DFLT_MAX_IMG_HEIGHT;
		    }
		}

		// Assembler can extract the frame dimensions
		if ((status == HXR_OK) || (status == HXR_NO_DATA))
		{
		    if (!m_pAssmDims)
		    {
			m_pAssmDims = new HXxSize;
			pFrameDims = new HXxSize;
			
			if (pFrameDims && m_pAssmDims)
			{
			    *m_pAssmDims = *pFrameDims = FrameDims;
			    if (status == HXR_OK)
			    {
				// we resize only on actual frame dimensions
				m_pH263VideoRenderer->ResizeViewFrame(FrameDims);
			    }
			}
			else
			{
			    HX_DELETE(m_pAssmDims);
			    HX_DELETE(pFrameDims);
			}
		    }
		    else
		    {
			if (*m_pAssmDims != FrameDims)
			{
			    pFrameDims = new HXxSize;
			    
			    if (pFrameDims)
			    {
				*pFrameDims = FrameDims;
			    }
			    else
			    {
				HX_DELETE(m_pAssmDims);
			    }
			}
		    }
		}
	    }
#ifdef _TRUST_MAX_DIMS_WHEN_AVAILABLE
	    else if ((!m_pAssmDims) && m_pMaxDims)
	    {
		// If assembler is absent, we should have max. dimensions
		// from out-of and config. data
		m_pAssmDims = new HXxSize;
		pFrameDims = new HXxSize;

		if (pFrameDims && m_pAssmDims)
		{
		    *m_pAssmDims = *pFrameDims = *m_pMaxDims;
		}
		else
		{
		    HX_DELETE(m_pAssmDims);
		    HX_DELETE(pFrameDims);
		}
	    }
#endif	// _TRUST_MAX_DIMS_WHEN_AVAILABLE
	    
	    if (m_pAssmDims)
	    {
		ULONG32 ulDataSize = pBuffer->GetSize();
		ULONG32 ulNewBufferSize = ulDataSize;
		UINT8* pData = pBuffer->GetBuffer();
		UINT8* pNewBuffer = (UINT8*) pBuffer;
		ULONG32 ulFlags = MDPCKT_USES_IHXBUFFER_FLAG;

#ifdef _OVERALLOC_CODEC_DATA
		// If Reassembler is present, it is responsible for overallocating
		// data if needed
		if (!m_pRssm)
		{
		    ulFlags = 0;
		    ulNewBufferSize += _OVERALLOC_CODEC_DATA;

		    pNewBuffer = new UINT8 [ulNewBufferSize];
		    if (pNewBuffer)
		    {
			memcpy(pNewBuffer, pData, ulDataSize); /* Flawfinder: ignore */
		    }
		    pData = pNewBuffer;
		}
#endif	// _OVERALLOC_CODEC_DATA

		if (pNewBuffer)
		{
		    pFramePacket = new CMediaPacket(
					    pNewBuffer,
					    pData, 
					    ulNewBufferSize,
					    ulDataSize,
					    pPacket->GetTime(),
					    ulFlags,
					    pFrameDims);
		}

		if (pFramePacket)
		{
		    pFramePacket->SetSampleDescKiller(KillH263ampleDesc);
		}	
		else
		{
                    m_LastError = HXR_OUTOFMEMORY;
		}	
	    }

	    pBuffer->Release();
	}
	    
        if( m_LastError != HXR_OUTOFMEMORY )
        {
	    m_pH263VideoRenderer->BltIfNeeded();
        }
	    
	pPacket->Release();
    }
    else
    {
	if (pCodecData->IsLost())
	{
#if defined(HELIX_FEATURE_STATS)
	    m_pH263VideoRenderer->ReportLostFrame();
#endif /* #if defined(HELIX_FEATURE_STATS) */
	}
    }

    return pFramePacket;
}


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::Reset
 *
 */
void CH263VideoFormat::Reset()
{
    _Reset();
    CVideoFormat::Reset();
}

void CH263VideoFormat::_Reset(void)
{
    HX_DELETE(m_pAssmDims);

    if (m_pRssm)
    {
	m_pRssm->Reset();
    }

    m_DecoderDims.cx      = 0;
    m_DecoderDims.cy      = 0;
    m_LastFrameDims.cx    = 0;
    m_LastFrameDims.cy    = 0;
    m_DesiredFrameSize.cx = 0;
    m_DesiredFrameSize.cy = 0;
    m_ulDecoderBufSize    = 0;
}


/****************************************************************************
 *  Method:
 *    CQTVideoFormat::CreateDecodedPacket
 *
 */
CMediaPacket* CH263VideoFormat::CreateDecodedPacket(CMediaPacket* pFrameToDecode)
{
    HXxSize currentFrameDims;
    CMediaPacket* pDecodedFrame = NULL;
    UINT8* pData = NULL;

    HX_ASSERT(pFrameToDecode);

    // Compute Decode Buffer Storage if info. available
    if (pFrameToDecode->m_pSampleDesc)
    {
	if (memcmp(pFrameToDecode->m_pSampleDesc, 
		   &m_DecoderDims, 
		   sizeof(HXxSize)) != 0)
	{
	    m_DecoderDims = *((HXxSize*) pFrameToDecode->m_pSampleDesc);
	    m_ulDecoderBufSize = m_DecoderDims.cx * 
				 m_DecoderDims.cy * 
				 H263_PIXEL_SIZE / 
				 8;

	    if (m_pDecoder->InitDecoder(&m_DecoderDims) != HXR_OK)
	    {
		m_DecoderDims.cx = 0;
		m_DecoderDims.cy = 0;
		m_ulDecoderBufSize = 0;
	    }
	}
    }

    // Get the storage for the Decode Buffer
    if (m_ulDecoderBufSize > 0)
    {
	CMediaPacket* pVideoPacket = NULL;

	pVideoPacket = (CMediaPacket*) m_pFramePool->Get(m_ulDecoderBufSize);

	if (pVideoPacket)
	{
	    pData = pVideoPacket->m_pData;
	    
	    HX_ASSERT(pData);
	    
	    pVideoPacket->Init(pData,
			       m_ulDecoderBufSize,
			       pFrameToDecode->m_ulTime,
			       0,
			       pFrameToDecode->m_pSampleDesc);
	    
	    pFrameToDecode->m_pSampleDesc = NULL;
	    
	    pDecodedFrame = pVideoPacket;
	}
	else
	{
	    pData = new UINT8 [m_ulDecoderBufSize];
	}
    }
	
    if (pData && m_pH263VideoRenderer->IsActive())
    {
	LONG32 lTimeAhead;

	lTimeAhead = m_pH263VideoRenderer->ComputeTimeAhead(
				pFrameToDecode->m_ulTime, 
				0);

	if (lTimeAhead < NON_KEYFRM_DCDE_FALLBEHIND_THRSHLD)
	{
	    // Throw away this frame
	    if (pDecodedFrame)
	    {
		m_pFramePool->Put(pDecodedFrame);
		pDecodedFrame = NULL;
		pData = NULL;
	    }
	    else
	    {
		delete [] pData;
		pData = NULL;
	    }

#if defined(HELIX_FEATURE_STATS)
	    m_pH263VideoRenderer->ReportDroppedFrame();
#endif /* #if defined(HELIX_FEATURE_STATS) */
	}
    }

    // Decode
    if (pData)
    {
	m_pH263VideoRenderer->BltIfNeeded();

	if (m_pDecoder->DecodeFrame(pFrameToDecode,
				    pData,
				    &currentFrameDims) != HXR_OK)
	{
	    // Throw away this frame
	    if (pDecodedFrame)
	    {
		m_pFramePool->Put(pDecodedFrame);
		pDecodedFrame = NULL;
		pData = NULL;
	    }
	    else
	    {
		delete [] pData;
		pData = NULL;
	    }

#if defined(HELIX_FEATURE_STATS)
	    m_pH263VideoRenderer->ReportDroppedFrame();
#endif /* #if defined(HELIX_FEATURE_STATS) */
	}
    }

    m_pH263VideoRenderer->BltIfNeeded();

    // If the frame is not formed yet, form it
    if (!pDecodedFrame)
    {
	if (pData)
	{
	    // Data exists to form the docoded packet with
	    pFrameToDecode->SetBuffer(pData,
				      pData,
				      m_ulDecoderBufSize, 
				      m_ulDecoderBufSize);
	    
	    pDecodedFrame = pFrameToDecode;
	    pFrameToDecode = NULL;

	    // If there is a change in frame size, forward the change
	    if (m_LastFrameDims != currentFrameDims)
	    {
		HXxSize* pNewFrameDims = new HXxSize;

		HX_ASSERT(currentFrameDims.cx <= m_DecoderDims.cx);
		HX_ASSERT(currentFrameDims.cy <= m_DecoderDims.cy);

		if (pNewFrameDims)
		{
		    *pNewFrameDims = currentFrameDims;
		    pDecodedFrame->SetSampleDesc(pNewFrameDims);
		    m_LastFrameDims = currentFrameDims;
		}
	    }
	}
	else if (pFrameToDecode->m_pSampleDesc)
	{
	    // No data exists to form the frame with, but description
	    // exists and must be passed down the pipe - form empty 
	    // frame packet
	    pDecodedFrame = pFrameToDecode;
	    pDecodedFrame->m_pData = NULL;
	    pDecodedFrame->m_ulDataSize = 0;
	    pFrameToDecode = NULL; 
	}
    }
  
    if (pFrameToDecode != NULL)
    {
	pFrameToDecode->Clear();
	delete pFrameToDecode;
    }
    
    return pDecodedFrame;
}


/****************************************************************************
 *  Method:
 *    CH263VideoFormat::CSecureH263VideoFormat
 *
 */
CH263Decoder* CH263VideoFormat::CreateDecoder()
{
    return new CH263Decoder(m_pH263VideoRenderer->GetContext());
}

/****************************************************************************
 *  Method:
 *    CH263VideoFormat::ConfigFrom3GPPHeader
 *
 */
HX_RESULT CH263VideoFormat::ConfigFrom3GPPHeader(IHXBuffer* pConfigData)
{
    UINT8*    pData  = NULL;
    ULONG32   ulSize = 0;
    HX_RESULT retVal = HXR_FAIL;

    HX_DELETE(m_pMaxDims);

    if (pConfigData)
    {
	pData = pConfigData->GetBuffer();
	ulSize = pConfigData->GetSize();

	m_pMaxDims = new HXxSize;

	retVal = HXR_OUTOFMEMORY;
    }

    if (m_pMaxDims && pData)
    {
	retVal = HXR_OK;

	// We'll assume unknown size
	m_pMaxDims->cx = 0;
	m_pMaxDims->cy = 0;

	if (ulSize >= sizeof(DecoderSpecificInfoV20))
	{
	    // Try 3GPP-v2.0 DecoderSpecificInfo
	    if (getlong(((DecoderSpecificInfoV20*) pData)->pType)
		== CCCC_ENCODE('d', '2', '6', '3'))
	    {
		// Looks good as 3GPP-v2.0 DecoderSpecificInfo
		;
	    }
	    else if ((ulSize >= sizeof(DecoderSpecificInfoV10))&&
		     (((DecoderSpecificInfoV10*) pData)->pTag[0] == 0x05))
	    {
		// Looks good as 3GPP-v1.0 DecoderSpecificInfo
		m_pMaxDims->cx = getshort(
		    ((DecoderSpecificInfoV10*) pData)->pMaxWidth);
		m_pMaxDims->cy = getshort(
		    ((DecoderSpecificInfoV10*) pData)->pMaxHeight);
	    }
	}
    }
	    
    return retVal;
}


/****************************************************************************
 *  Method:
 *    CH263VideoFormat::IsBitmapFormatChanged
 *
 */
HXBOOL CH263VideoFormat::IsBitmapFormatChanged(
			    HXBitmapInfoHeader &BitmapInfoHeader,
			    CMediaPacket* pVideoPacket)
{
    if (pVideoPacket->m_pSampleDesc != NULL)
    {
	HXxSize* pDims = (HXxSize*) pVideoPacket->m_pSampleDesc;

	if ((BitmapInfoHeader.biWidth != pDims->cx) ||
	    (BitmapInfoHeader.biHeight != pDims->cy))
	{
	    return TRUE;
	}
    }

    return FALSE;
}


/****************************************************************************
 *  Method:
 *    CH263VideoFormat::InitBitmapInfoHeader
 *
 */
HX_RESULT CH263VideoFormat::InitBitmapInfoHeader(
    HXBitmapInfoHeader &bitmapInfoHeader,
    CMediaPacket* pVideoPacket)
{
    HXxSize* pDims = (HXxSize*) pVideoPacket->m_pSampleDesc;

    if (pDims)
    {
	bitmapInfoHeader.biWidth = pDims->cx;
	bitmapInfoHeader.biHeight = pDims->cy;
	bitmapInfoHeader.biSizeImage = bitmapInfoHeader.biWidth * 
				       bitmapInfoHeader.biHeight * 
				       bitmapInfoHeader.biBitCount / 
				       8;

        // m_DesiredFrameSize holds the specified video frame
        // width and height set in the stream header via the
        // "FrameWidth" and "FrameHeight" properties. If these
        // were not set in the strema header, then m_DesiredFrameWidth
        // will still be (0,0). We only check against it if
        // it's not zero.
        if (m_DesiredFrameSize.cx && m_DesiredFrameSize.cy &&
            (pDims->cx > m_DesiredFrameSize.cx ||
             pDims->cy > m_DesiredFrameSize.cy))
        {
            m_pH263VideoRenderer->SetClipRect(0, 0, m_DesiredFrameSize.cx, m_DesiredFrameSize.cy);
        }
    }

    return HXR_OK;
}


/****************************************************************************
 *  Method:
 *    CH263VideoFormat::KillH263ampleDesc
 *
 */
void CH263VideoFormat::KillH263ampleDesc(void* pSampleDesc, void* pUserData)
{
    if (pSampleDesc)
    {
	HXxSize* pFrameDims = (HXxSize*) pSampleDesc;

	delete pFrameDims;
    }
}
