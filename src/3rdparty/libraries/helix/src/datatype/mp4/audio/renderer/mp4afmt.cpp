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
// #define _BYPASS_DECODER
// #define _SILENT_PLAY
// #define ENABLE_FRAME_TRACE

#define MIN_ACCEPTBALE_DATA_REMAINDER	1	// in bytes
#define MP4_DEFAULT_AUDIO_PREROLL	2000	// in milliseconds
#define MAX_TOLERABLE_TIME_GAP		5	// in milliseconds


/****************************************************************************
 *  Includes
 */
#include "mp4afmt.h"
#include "mp4adec.h"

#include "hxtick.h"
#include "hxassert.h"
#include "hxstrutl.h"

#include "mp4audio.h"
#include "mp4apyld.h"
#include "mp4gpyld.h"
#include "amrpyld.h"
#include "qclppyld.h"
#include "hxamrpyld.h"
#include "pckunpck.h"

#if defined(HELIX_FEATURE_AUDIO_CODEC_MP3)
#include "mp3draft.h"
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_MP3) */


/****************************************************************************
 *  Debug
 */
#ifdef ENABLE_FRAME_TRACE
#define MAX_FRAME_TRACE_ENTRIES	100000
ULONG32 ulFrameTraceIdx = 0;
LONG32 frameTraceArray[MAX_FRAME_TRACE_ENTRIES][4];

void DumpFrameEntries(void)
{
    FILE* pFile = NULL;
    ULONG32 ulIdx;

    if (ulFrameTraceIdx > 0)
    {
	pFile = fopen("c:\\mp4a.txt", "wb");
    }

    if (pFile)
    {
	for (ulIdx = 0; ulIdx < ulFrameTraceIdx; ulIdx++)
	{
	    fprintf(pFile, "%c(%d=%d) = %d\n", (char) frameTraceArray[ulIdx][1], 
					   frameTraceArray[ulIdx][2],
					   frameTraceArray[ulIdx][3],
					   frameTraceArray[ulIdx][0]);
	}

	fclose(pFile);
    }

    ulFrameTraceIdx = 0;
}
#endif	// ENABLE_FRAME_TRACE


/****************************************************************************
 *  Locals
 */


/****************************************************************************
 *  Method:
 *    CMP4AudioFormat::CMP4AudioFormat
 *
 */
CMP4AudioFormat::CMP4AudioFormat(IHXCommonClassFactory* pCommonClassFactory,
				 CMP4AudioRenderer* pMP4AudioRenderer)
    : CAudioFormat(pCommonClassFactory, pMP4AudioRenderer)
    , m_pRssm(NULL)
    , m_bNewAssembledFrame(TRUE)
    , m_ulAUDuration(0)
    , m_ulLastDecodedEndTime(0)
    , m_ulLastFrameTime(0)
    , m_ulCodecDelayMs(0)
    , m_bCanChangeAudioStream(FALSE)
    , m_ulMaxDecoderOutputSamples(0)
    , m_ulMaxDecoderOutputBytes(0)
    , m_pDecoderBuffer(NULL)
    , m_ulDecoderBufferSize(0)
    , m_pDecoderInstance(NULL)
    , m_pDecoderRenderer(NULL)
    , m_pMP4AudioRenderer(pMP4AudioRenderer)
    , m_pDecoderModule(NULL)
{
    // Register the payload format builders with m_fmtFactory
    RegisterPayloadFormats();

    HX_ASSERT(m_pCommonClassFactory);
}


/****************************************************************************
 *  Method:
 *    CMP4AudioFormat::~CMP4AudioFormat
 *
 */
CMP4AudioFormat::~CMP4AudioFormat()
{
    _Reset();

    HX_RELEASE(m_pDecoderInstance);
    HX_RELEASE(m_pDecoderRenderer);
    HX_DELETE(m_pDecoderModule);
    HX_VECTOR_DELETE(m_pDecoderBuffer);
    HX_RELEASE(m_pRssm);

#ifdef ENABLE_FRAME_TRACE
    DumpFrameEntries();
#endif	// ENABLE_FRAME_TRACE
}


/****************************************************************************
 *  Method:
 *    CMP4AudioFormat::Init
 *
 */
HX_RESULT CMP4AudioFormat::Init(IHXValues* pHeader)
{
    HX_RESULT retVal = CAudioFormat::Init(pHeader);

    // Create Packet Assembler
    retVal = m_fmtFactory.BuildFormat(m_pCommonClassFactory, FALSE, 
                                      pHeader, m_pRssm);
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    if (FAILED(retVal) && retVal == HXR_REQUEST_UPGRADE)
    {
        // If BuildFormat returns HXR_REQUEST_UPGRADE, then
        // it means that no depacketizer had successful calls
        // to Init() and SetStreamHeader(), but at least one
        // of the depacketizers returned HXR_REQUEST_UPGRADE.
        //
        // Copy our own mime type into the AU string
        IHXBuffer* pMimeTypeStr = NULL;
        pHeader->GetPropertyCString("MimeType", pMimeTypeStr);
        if (pMimeTypeStr)
        {
            SafeStrCpy(m_szAUStr, // m_szAUStr is from our parent class
                       (const char*) pMimeTypeStr->GetBuffer(),
                       MAX_AUSTR_SIZE);
        }
        HX_RELEASE(pMimeTypeStr);
    }
#endif /* #if defined(HELIX_FEATURE_AUTOUPGRADE) */

    // Open the Decoder Module
    if (SUCCEEDED(retVal))
    {
	m_pDecoderModule = CreateDecoder();

	retVal = HXR_OUTOFMEMORY;
	if (m_pDecoderModule)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pDecoderModule->Open(pHeader, 
                                        m_pRssm,
					&m_pDecoderInstance,
					m_pContext);
#if defined(HELIX_FEATURE_AUTOUPGRADE)
        if (FAILED(retVal) && retVal == HXR_REQUEST_UPGRADE)
        {
            // If we returned a HXR_REQUEST_UPGRADE, then we 
            // failed to load the decoder binary. Therefore, we
            // will copy the decoder's AU string into our
            // own AU String
            SafeStrCpy(m_szAUStr, // m_szAUStr is from our parent class
                       m_pDecoderModule->GetAutoUpgradeString(),
                       MAX_AUSTR_SIZE);
        }
#endif /* #if defined(HELIX_FEATURE_AUTOUPGRADE) */
        if (FAILED(retVal) && retVal != HXR_REQUEST_UPGRADE)
        {
            HX_RELEASE(m_pDecoderInstance);
            retVal = HXR_OK;
        }
    }

    // Open Codec Instance
    if (SUCCEEDED(retVal))
    {
	ULONG32 ulBitstreamHeaderSize;
	const UINT8* pBitstreamHeader;
        UINT8 unBitstreamType;

	HX_ASSERT(m_pDecoderModule);
        ulBitstreamHeaderSize = m_pRssm->GetBitstreamHeaderSize();
	pBitstreamHeader = m_pRssm->GetBitstreamHeader();
	unBitstreamType = m_pRssm->GetBitstreamType();

        do
        {
            if (m_pDecoderInstance)
            {
	        retVal = m_pDecoderInstance->OpenDecoder(unBitstreamType,	// eCfgDesc
						         pBitstreamHeader,
						         ulBitstreamHeaderSize);
            }
            else
            {
                retVal = HXR_FAIL;
            }

            if (SUCCEEDED(retVal))
            {
                break;
            }
            HX_RELEASE(m_pDecoderInstance);
	    retVal = m_pDecoderModule->OpenNext(pHeader, 
                                                m_pRssm,
			                        &m_pDecoderInstance,
					        m_pContext);
#ifdef _IGNORE_UNSUPPORTED
            if (FAILED(retVal) && retVal != HXR_REQUEST_UPGRADE)
            {
	        HX_RELEASE(m_pDecoderInstance);
	        retVal = HXR_OK;
                break;
            }
#endif	// _IGNORE_UNSUPPORTED

        } while (SUCCEEDED(retVal));
    }
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    if (FAILED(retVal) && retVal != HXR_REQUEST_UPGRADE)
    {
        // If we failed in our OpenDecoder() call, then the
        // decoder must be a version which does not support the
        // particular profile which we passed in. Therefore,
        // we will AU with the "qtplayer" string to instruct
        // the TLC to attempt to play this file other means.
        //
        // Copy into our parent class's AU string buffer
        SafeStrCpy(m_szAUStr, // m_szAUStr is from our parent class
                   "qtplayer",
                   MAX_AUSTR_SIZE);
        // When CAudioFormat::Init() returns to where it was called
        // from CAudioRenderer::OnHeader(), then if the return code
        // is HXR_REQUEST_UPGRADE, then it will assume it needs
        // to AU, and will look for an AU string from CAudioFormat.
        // So we need to return HXR_REQUEST_UPGRADE.
        retVal = HXR_REQUEST_UPGRADE;
    }
#endif /* #if defined(HELIX_FEATURE_AUTOUPGRADE) */

#if defined(HELIX_FEATURE_STATS)
    if (SUCCEEDED(retVal) && m_pDecoderInstance)
    {
	char* pVal = (char*) m_pDecoderModule->GetCodecName();

	if (pVal)
	{
	    m_pMP4AudioRenderer->ReportStat(AS_CODEC_NAME, pVal);
	}

	pVal = (char*) m_pDecoderModule->GetCodecFourCC();

	if (pVal)
	{
	    m_pMP4AudioRenderer->ReportStat(AS_CODEC_4CC, pVal);
	}
    }
#endif /* #if defined(HELIX_FEATURE_STATS) */

    if (SUCCEEDED(retVal) && m_pDecoderInstance)
    {
        IHXValues* pSrcProps = NULL;

        if (HXR_OK == m_pMP4AudioRenderer->QueryInterface(IID_IHXValues,
                                                          (void**)&pSrcProps))
        {
            UINT32 ulBitrate = 0;

            if ((HXR_OK == pHeader->GetPropertyULONG32("AvgBitRate", 
                                                       ulBitrate)) ||
                (HXR_OK == pHeader->GetPropertyULONG32("MaxBitRate", 
                                                       ulBitrate)))
            {
                pSrcProps->SetPropertyULONG32("SrcBitRate", 
                                              ulBitrate);
            }

            const char* pFourCC = m_pDecoderModule->GetCodecFourCC();

            if (pFourCC && m_pCommonClassFactory)
            {
                SetCStringPropertyCCF(pSrcProps, "SrcCodec", pFourCC,
                                   m_pCommonClassFactory, FALSE);
            }
        }

        HX_RELEASE(pSrcProps);
    }

    // Inquire and set Audio data parameters
    if (SUCCEEDED(retVal) && m_pDecoderInstance)
    {
        retVal = UpdateAudioFormat(m_ulLastDecodedEndTime, TRUE);
    }

    if (SUCCEEDED(retVal))
    {
	m_bCanChangeAudioStream = CanChangeAudioStream();
    }

    if (SUCCEEDED(retVal) && m_pDecoderInstance) 
    {
        m_pDecoderInstance->QueryInterface(IID_IHXAudioDecoderRenderer, (void**)(&m_pDecoderRenderer));
        
        if (m_pDecoderRenderer)
        {
            m_pDecoderRenderer->SetStartTime(GetStartTime());
        }

    }

#ifdef _BYPASS_DECODER
    HX_RELEASE(m_pDecoderInstance);
#endif	// _BYPASS_DECODER

    return retVal;
}


/****************************************************************************
 *    CMP4AudioFormat::GetDefaultPreroll
 */
ULONG32 CMP4AudioFormat::GetDefaultPreroll(IHXValues* pValues)
{
    ULONG32 ulRet = MP4_DEFAULT_AUDIO_PREROLL;

    if (m_ulAUDuration && m_pAudioFmt && m_pAudioFmt->ulSamplesPerSec)
    {
        // If m_ulAUDuration is set and we know the sample rate,
        // generate a preroll value that only requires ulAUCount
        // of AUs to be buffered.
        ULONG32 ulAUCount = 5;
        ulRet = (ulAUCount * 1000 * m_ulAUDuration) / m_pAudioFmt->ulSamplesPerSec;
    }

    return ulRet;
}

HX_RESULT CMP4AudioFormat::UpdateAudioFormat(ULONG32& ulAnchorTime, 
					     HXBOOL bForceUpdate)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pDecoderInstance && m_pAudioFmt && m_pRssm)
    {
	UINT32 ulChannels = 0;
	HXBOOL bFormatChanged = FALSE;

        // Set the bits per sample (always 16 bits per sample)
        m_pAudioFmt->uBitsPerSample = 16;                 
        // Set number of channels
        retVal = m_pDecoderInstance->GetNChannels(ulChannels);
        if (SUCCEEDED(retVal))
        {
            retVal = HXR_FAIL;
	    if (ulChannels)
	    {
		retVal = HXR_OK;

		if ((ulChannels != m_pAudioFmt->uChannels) || bForceUpdate)
		{
		    bFormatChanged = TRUE;
		}
	    }
        }
        // Set the sample rate
        if (SUCCEEDED(retVal))
        {
            UINT32 ulSampleRate = 0;
            retVal = m_pDecoderInstance->GetSampleRate(ulSampleRate);
            if (SUCCEEDED(retVal))
            {
                retVal = HXR_FAIL;
                if (ulSampleRate)
                {
		    retVal = HXR_OK;

		    if ((m_pAudioFmt->ulSamplesPerSec != ulSampleRate) || 
			bForceUpdate)
		    {
			ULONG32 ulAnchorInMs = m_TSConverter.Convert(ulAnchorTime);

			m_pAudioFmt->uChannels = (UINT16) ulChannels;
			m_pAudioFmt->ulSamplesPerSec = ulSampleRate;

			ulAnchorTime = 	ConvertMsToTime(ulAnchorInMs);

			ConfigureRssm(ulAnchorInMs);

			m_TSConverter.SetBase(ulSampleRate, 1000);
			m_TSConverter.SetOffset(ulAnchorInMs);

			bFormatChanged = TRUE;
		    }
		    else
		    {
			m_pAudioFmt->uChannels = (UINT16) ulChannels;
		    }  
                }
            }
        }
        // Set the max samples out
        if (SUCCEEDED(retVal))
        {
            UINT32 ulMaxSamplesOut = 0;
            retVal = m_pDecoderInstance->GetMaxSamplesOut(ulMaxSamplesOut);
            if (SUCCEEDED(retVal))
            {
                retVal = HXR_FAIL;
                if (ulMaxSamplesOut)
                {
                    retVal = HXR_OK;

		    if ((ulMaxSamplesOut != m_ulMaxDecoderOutputSamples) || 
			bForceUpdate)
		    {
			// Save the max samples out
			m_ulMaxDecoderOutputSamples = ulMaxSamplesOut;
			// Compute and set the AU duration
			m_ulAUDuration = m_ulMaxDecoderOutputSamples / m_pAudioFmt->uChannels;
			m_pRssm->SetAUDuration(m_ulAUDuration);
			// Compute the max output in bytes
			m_ulMaxDecoderOutputBytes = CAudioFormat::ConvertSamplesToBytes(m_ulMaxDecoderOutputSamples);
			// Save the max block size
			m_pAudioFmt->uMaxBlockSize = (UINT16) m_ulMaxDecoderOutputBytes;

			bFormatChanged = TRUE;
		    }
                }
            }
        }
        // Set the codec delay in samples
        if (bFormatChanged && SUCCEEDED(retVal))
        {
            UINT32 ulCodecDelaySamples = 0;
            retVal = m_pDecoderInstance->GetDelay(ulCodecDelaySamples);
            if (SUCCEEDED(retVal))
            {
                // Compute and save the codec delay in ms
                m_ulCodecDelayMs = CAudioFormat::ConvertSamplesToMs(ulCodecDelaySamples);
            }
        }
        // Allocate Decoder Buffer
        if (SUCCEEDED(retVal))
        {
            // Did the size change?
            if (m_ulDecoderBufferSize != m_ulMaxDecoderOutputBytes)
            {
                retVal = HXR_OUTOFMEMORY;
                HX_VECTOR_DELETE(m_pDecoderBuffer);
                m_pDecoderBuffer = new UINT8 [m_ulMaxDecoderOutputBytes];
                if (m_pDecoderBuffer)
                {
		    retVal = HXR_OK;
                    m_ulDecoderBufferSize = m_ulMaxDecoderOutputBytes;
                }
            }
        }
    }

    return retVal;
}

HX_RESULT CMP4AudioFormat::ConfigureRssm(ULONG32 ulAnchorInMs)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pRssm)
    {
	CTSConverter tempConverter;
	ULONG32 ulRssmTimeBase;
	ULONG32 ulRssmAnchor;
	ULONG32 ulDecoderAnchor;
	ULONG32 ulSamplingRate = m_pAudioFmt->ulSamplesPerSec;

	retVal = HXR_OK;

	if (ulSamplingRate == 0)
	{
	    ulSamplingRate = 1000;
	}

	m_pRssm->SetSamplingRate(ulSamplingRate);
	m_pRssm->SetTimeAnchor(ulAnchorInMs);
	
	ulRssmTimeBase = m_pRssm->GetTimeBase();
	if (ulRssmTimeBase == 0)
	{
	    ulRssmTimeBase = m_pAudioFmt->ulSamplesPerSec;
	}
	
	m_InputTSConverter.SetBase(ulRssmTimeBase, 
				   m_pAudioFmt->ulSamplesPerSec);
	
	tempConverter.SetBase(1000, ulRssmTimeBase);
	ulRssmAnchor = tempConverter.ConvertVector(ulAnchorInMs);
	tempConverter.SetBase(1000, m_pAudioFmt->ulSamplesPerSec);
	ulDecoderAnchor = tempConverter.ConvertVector(ulAnchorInMs);
	
	m_InputTSConverter.SetAnchor(ulRssmAnchor, ulDecoderAnchor);
    }

    return retVal;
}

/****************************************************************************
 *  Method:
 *    CMP4AudioFormat::CreateAssembledPacket
 *
 */
CMediaPacket* CMP4AudioFormat::CreateAssembledPacket(IHXPacket* pPacket)
{
    CMediaPacket* pLastFramePacket = NULL;
    CMediaPacket* pFramePacket = NULL;

    if (m_pRssm)
    {
	if (pPacket)
	{
	    m_pRssm->SetPacket(pPacket);
	}
	else
	{
	    m_pRssm->Flush();
	}
	
	if (m_pRssm->CreateMediaPacket(pLastFramePacket) == HXR_OK)
	{
	    HX_ASSERT(pLastFramePacket);
	    
	    do
	    {
		pFramePacket = NULL;
		if (m_pRssm->CreateMediaPacket(pFramePacket) == HXR_OK)
		{
		    HX_ASSERT(pFramePacket);
		    
		    if (pLastFramePacket)
		    {
			CAudioFormat::PutAudioPacket(pLastFramePacket);
		    }
		    
		    pLastFramePacket = pFramePacket;
		}
	    } while (pFramePacket);
	}
    }

    return pLastFramePacket;
}


/****************************************************************************
 *  Method:
 *    CMP4AudioFormat::DecodeAudioData
 *
 */
HX_RESULT CMP4AudioFormat::DecodeAudioData(HXAudioData& audioData,
					   HXBOOL bFlushCodec)
{
    ULONG32 ulBytesDecoded;
    ULONG32 ulSamplesProduced;
    ULONG32 ulBytesProduced;
    ULONG32 ulNextDecodeStartTime;
    ULONG32 ulPacketBasedNextDecodeStartTime;
    HX_RESULT retVal = HXR_NO_DATA;
    HX_RESULT checkStatus;

    CMediaPacket* pAssembledFrame = NULL;

    if (bFlushCodec)
    {
	pAssembledFrame = CreateAssembledPacket(NULL);
        if (pAssembledFrame)
        {
            CAudioFormat::PutAudioPacket(pAssembledFrame);
        }
    }

    while ((pAssembledFrame = CAudioFormat::PeekAudioPacket()) &&
	   (retVal == HXR_NO_DATA))
    {
	ulBytesDecoded = 0;
	ulSamplesProduced = 0;
	ulBytesProduced = 0;
	ulNextDecodeStartTime = m_ulLastDecodedEndTime;

	if (m_bNewAssembledFrame)
	{
	    LONG32 lDecodeStartOffset;

	    m_bNewAssembledFrame = FALSE;

	    // Filter uot fluctuations in time-stamps present in some poorly generated audio streams.
	    ulPacketBasedNextDecodeStartTime = m_InputTSConverter.Convert(pAssembledFrame->m_ulTime);
	    lDecodeStartOffset = ulPacketBasedNextDecodeStartTime - ulNextDecodeStartTime;
	    if (lDecodeStartOffset >= ((LONG32) m_ulLastFrameTime))
	    {
		ulNextDecodeStartTime = ulPacketBasedNextDecodeStartTime;
	    }

	    // Estimate audio frame time if unknown
	    if (pAssembledFrame->m_ulFlags & MDPCKT_HAS_UKNOWN_TIME_FLAG)
	    {
		if (CAudioRenderer::CmpTime(m_ulLastDecodedEndTime, ulNextDecodeStartTime) >= 0)
		{
		    ulNextDecodeStartTime = m_ulLastDecodedEndTime;

		    if (pAssembledFrame->m_ulFlags & MDPCKT_FOLLOWS_LOSS_FLAG)
		    {
			ulNextDecodeStartTime += m_ulLastFrameTime;
		    }
		}
	    }

	    // See if there was loss we need to conceal
	    if (pAssembledFrame->m_ulFlags & MDPCKT_FOLLOWS_LOSS_FLAG)
	    {
		if (CAudioRenderer::CmpTime(ulNextDecodeStartTime, m_ulLastDecodedEndTime) > 0)
		{
		    ULONG32 ulTimeLost = ulNextDecodeStartTime - 
					 m_ulLastDecodedEndTime;

		    if (CAudioFormat::ConvertTimeToMs(ulTimeLost) > MAX_TOLERABLE_TIME_GAP)
		    {
			ULONG32 ulSamplesLost = CAudioFormat::ConvertTimeToSamples(ulTimeLost);

#ifdef ENABLE_FRAME_TRACE
			if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
			{
			    frameTraceArray[ulFrameTraceIdx][2] = m_ulLastDecodedEndTime;
			    frameTraceArray[ulFrameTraceIdx][3] = CAudioFormat::ConvertTimeToMs(frameTraceArray[ulFrameTraceIdx][2]);
			    frameTraceArray[ulFrameTraceIdx][0] = 
				(LONG32) ulTimeLost;
			    frameTraceArray[ulFrameTraceIdx++][1] = 'G';
			}
#endif	// ENABLE_FRAME_TRACE

			// Conceal the lost samples
			if (m_pDecoderInstance)
			{
			    m_pDecoderInstance->Conceal(ulSamplesLost);
			    ulNextDecodeStartTime = m_ulLastDecodedEndTime;
#ifdef ENABLE_FRAME_TRACE
			    if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
			    {
				frameTraceArray[ulFrameTraceIdx][2] = ulNextDecodeStartTime;
				frameTraceArray[ulFrameTraceIdx][3] = CAudioFormat::ConvertTimeToMs(frameTraceArray[ulFrameTraceIdx][2]);
				frameTraceArray[ulFrameTraceIdx][0] = 
				    (LONG32) ulSamplesLost;
				frameTraceArray[ulFrameTraceIdx++][1] = 'C';
			    }
#endif	// ENABLE_FRAME_TRACE
			}
		    }
		}
	    }
	}


	checkStatus = CheckDecoderInstance(m_pDecoderInstance);
	if (FAILED(checkStatus))
	{
	    return checkStatus;
	}

    // Decode some samples

	if (((CMediaPacket::GetBufferSize(pAssembledFrame) != 
	      pAssembledFrame->m_ulDataSize) &&
	     (pAssembledFrame->m_ulDataSize < MIN_ACCEPTBALE_DATA_REMAINDER))
#ifdef _IGNORE_UNSUPPORTED
	    || (!m_pDecoderInstance)
#endif	// _IGNORE_UNSUPPORTED
	   )
	{	
	    ulSamplesProduced = 0;
	    ulBytesDecoded = pAssembledFrame->m_ulDataSize;
	}
	else
	{
	    ulSamplesProduced = m_ulMaxDecoderOutputSamples;

	    ProcessAssembledFrame(pAssembledFrame);

        if (m_pDecoderRenderer)
        {
            UINT32 ulNextFrameTime = m_TSConverter.Convert(ulNextDecodeStartTime);
            m_pDecoderRenderer->SetNextFrameTime(ulNextFrameTime);
        }

	    retVal = m_pDecoderInstance->Decode(pAssembledFrame->m_pData,
						pAssembledFrame->m_ulDataSize,
						ulBytesDecoded,
						(INT16*) m_pDecoderBuffer,
						ulSamplesProduced,
						bFlushCodec);
	    if (retVal != HXR_OK)
	    {
		ulBytesDecoded = 0;
		ulSamplesProduced = 0;
	    }
#ifdef ENABLE_FRAME_TRACE
	    if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
	    {
		frameTraceArray[ulFrameTraceIdx][2] = ulNextDecodeStartTime;
		frameTraceArray[ulFrameTraceIdx][3] = CAudioFormat::ConvertTimeToMs(frameTraceArray[ulFrameTraceIdx][2]);
		frameTraceArray[ulFrameTraceIdx][0] = 
		    (LONG32) ulBytesDecoded;
		frameTraceArray[ulFrameTraceIdx++][1] = 'B';
	    }
	    if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
	    {
		frameTraceArray[ulFrameTraceIdx][2] = ulNextDecodeStartTime;
		frameTraceArray[ulFrameTraceIdx][3] = CAudioFormat::ConvertTimeToMs(frameTraceArray[ulFrameTraceIdx][2]);
		frameTraceArray[ulFrameTraceIdx][0] = 
		    (LONG32) ulSamplesProduced;
		frameTraceArray[ulFrameTraceIdx++][1] = 'D';
	    }
#endif	// ENABLE_FRAME_TRACE
	}

	// Adjust ramaining data pointers based on bytes consumed
	HX_ASSERT(ulBytesDecoded <= pAssembledFrame->m_ulDataSize);
	if (ulBytesDecoded <= pAssembledFrame->m_ulDataSize)
	{
	    pAssembledFrame->m_ulDataSize -= ulBytesDecoded;
	}
	else
	{
	    pAssembledFrame->m_ulDataSize = 0;
	}
	pAssembledFrame->m_pData += ulBytesDecoded;

	// Place decoded data into the audio buffer
	if (ulSamplesProduced > 0)
	{
	    if (m_bCanChangeAudioStream && m_pDecoderInstance)
	    {
		retVal = UpdateAudioFormat(ulNextDecodeStartTime);
		if (FAILED(checkStatus))
		{
		    retVal = checkStatus;
		}
	    }

#ifndef _SILENT_PLAY
	    if (SUCCEEDED(retVal))
	    {
		retVal = HXR_OK;
		
		ulBytesProduced = CAudioFormat::ConvertSamplesToBytes(ulSamplesProduced);
		
		if (retVal == HXR_OK)
		{
		    retVal = m_pCommonClassFactory->CreateInstance(
					    CLSID_IHXBuffer, 
					    (void**) &audioData.pData);
		}
		
		if (retVal == HXR_OK)
		{
		    retVal = audioData.pData->Set(
					    m_pDecoderBuffer,
					    ulBytesProduced);
		}
		
		if (retVal == HXR_OK)
		{
		    audioData.ulAudioTime = m_TSConverter.Convert(ulNextDecodeStartTime) -
					    m_ulCodecDelayMs;
		}
	    }
#endif	// _SILENT_PLAY
	}

	m_ulLastFrameTime = CAudioFormat::ConvertSamplesToTime(ulSamplesProduced);
	m_ulLastDecodedEndTime = ulNextDecodeStartTime + m_ulLastFrameTime;
				 
	if ((pAssembledFrame->m_ulDataSize == 0) ||
	    ((ulSamplesProduced == 0) && (ulBytesDecoded == 0)))
	{
	    CMediaPacket* pDeadAssembledFrame = CAudioFormat::GetAudioPacket();

	    HX_ASSERT(pAssembledFrame == pDeadAssembledFrame);
		
	    CMediaPacket::DeletePacket(pDeadAssembledFrame);
	    m_bNewAssembledFrame = TRUE;
	}

	if (ulSamplesProduced > 0)
	{
	    break;
	}
    }

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CMP4AudioFormat::Reset
 *
 */
void CMP4AudioFormat::Reset()
{
    _Reset();
    CAudioFormat::Reset();
}

HXBOOL CMP4AudioFormat::CanChangeAudioStream()
{
    HXBOOL bRet = FALSE;

    if (m_pDecoderModule)
    {
        bRet = m_pDecoderModule->CanChangeAudioStream();
    }

    return bRet;
}

void CMP4AudioFormat::RegisterPayloadFormats()
{
    // Register the various payload format builder functions
    // with the payload format factory

#if defined(HELIX_FEATURE_AUDIO_CODEC_AAC) || defined(HELIX_FEATURE_AUDIO_CODEC_RAAC)
    // MPEG4 audio formats
    m_fmtFactory.RegisterBuilder(&MP4APayloadFormat::Build);
#endif // defined(HELIX_FEATURE_AUDIO_CODEC_AAC)

#if defined(HELIX_FEATURE_ISMA) || defined(HELIX_FEATURE_AUDIO_RALF)
    m_fmtFactory.RegisterBuilder(&MP4GPayloadFormat::Build);
#endif	// defined(HELIX_FEATURE_ISMA) || defined(HELIX_FEATURE_AUDIO_RALF)

    // AMR formats
#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB) || defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB)
    m_fmtFactory.RegisterBuilder(&CAMRPayloadFormat::Build);
    m_fmtFactory.RegisterBuilder(&CHXAMRPayloadFormat::Build);
#endif	// defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB) || defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB)

#if defined(HELIX_FEATURE_AUDIO_CODEC_MP3)
    // MP3 format
    m_fmtFactory.RegisterBuilder(&CMP3DraftPayloadFormat::Build);
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_MP3) */

#if defined(HELIX_FEATURE_AUDIO_CODEC_QCELP)
    // Qcelp format
    m_fmtFactory.RegisterBuilder(&CQCELPPayloadFormat::Build);
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_QCELP) */

}

void CMP4AudioFormat::_Reset(void)
{
    if (m_pRssm)
    {
	m_pRssm->Reset();
	m_pRssm->SetTimeAnchor(GetStartTime());

	ConfigureRssm(GetStartTime());
    }

    if (m_pDecoderInstance)
    {
        m_pDecoderInstance->Reset();
    }

    if (m_pDecoderRenderer)
    {
        m_pDecoderRenderer->SetStartTime(GetStartTime());
    }

    m_ulLastDecodedEndTime = 0;
    m_ulLastFrameTime = 0;
    m_bNewAssembledFrame = TRUE;

    m_TSConverter.Reset();
    m_TSConverter.SetOffset(GetStartTime());
}



