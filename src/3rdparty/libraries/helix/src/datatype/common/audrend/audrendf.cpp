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

//////////////////////////////////////////////////////////////////////////////
// 
//  audio format for QuickTime renderer for RealMedia Architecture.
//
// Notes:
// This class puts together the audio packets and decodes the audio data.
// There are still unresolved problems with this code - audio data gets
// dropped. This only occurs when there is more than one frame of audio
// data per packet - like MP3. Also, audio services doesn't seem to be 
// releasing the audio buffers. This is a huge problem when trying to 
// listen to CD quality audio for any amount of time. Another problem - 
// the current implementation will not work when one audio frame spans 
// multiple rtp packets. The code from the video renderer will need to be 
// adapted to work with audio. The class CQTPacket does all the QuickTime
// header parsing. See the notes in that file for more info on the QT header
// structure.
// 

/****************************************************************************
 *  Defines
 */
#define MAX_DECODE_GRANULARITY	    200	    // in milliseconds
#define DEFAULT_AUDIO_PREROLL	    2000    // in milliseconds
#if defined(HELIX_FEATURE_MIN_HEAP)
#define MAXIMUM_AUDIO_PREROLL	    1500    // in milliseconds
#else
#define MAXIMUM_AUDIO_PREROLL	    7500    // in milliseconds
#endif


/****************************************************************************
 *  Includes
 */
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

// #include "netbyte.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxausvc.h"
#include "ihxpckts.h"
#include "hxerror.h"

#include "hxcore.h"
#include "hxupgrd.h"
#include "hxslist.h"
#include "hxassert.h"

#include "hxstrutl.h"
#include "hxtlogutil.h"

#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "hxtick.h"

#include "audrend.h"
#include "audrendf.h"


/****************************************************************************
 *  CAudioFormat
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CAudioFormat::CAudioFormat(IHXCommonClassFactory* pCommonClassFactory,
			   CAudioRenderer* pAudioRenderer)
    : m_pContext(NULL)
    , m_pCommonClassFactory(pCommonClassFactory)
    , m_pAudioFmt(NULL)
    , m_pPendingPacketQueue(NULL)
    , m_bPostStartTime(FALSE)
    , m_ulTrackStartTime(NO_TIME_SET)
    , m_ulTrackEndTime(NO_TIME_SET)
    , m_ulForceDiscardUntilTime(NO_TIME_SET)
    , m_ulStartTime(0)
    , m_fCompressionRatio(1.0)
    , m_lRefCount(0)
{
    if (m_pCommonClassFactory)
    {
	m_pCommonClassFactory->AddRef();
    }

    m_pContext = pAudioRenderer->GetContext();
    if (m_pContext)
    {
	m_pContext->AddRef();
    }

    // NULL out the AU String
    memset(&m_szAUStr[0], 0, MAX_AUSTR_SIZE);
}

CAudioFormat::~CAudioFormat()
{
    HX_DELETE(m_pAudioFmt);

    FlushQueues();    
    HX_DELETE(m_pPendingPacketQueue);

    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);
}


/****************************************************************************
 *  CAudioFormat::Init
 */
HX_RESULT CAudioFormat::Init(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pHeader)
    {
        m_pAudioFmt = new HXAudioFormat;
        if (m_pAudioFmt)
        {
            m_pPendingPacketQueue = new CHXSimpleList;
            if (m_pPendingPacketQueue)
            {
                m_pAudioFmt->uChannels       = (UINT16) GetULONG32Property(pHeader, "Channels", 1);
                m_pAudioFmt->uBitsPerSample  = (UINT16) GetULONG32Property(pHeader, "BitsPerSample", 16);
                m_pAudioFmt->ulSamplesPerSec = GetULONG32Property(pHeader, "SamplesPerSecond", 8000);
                // MaxBlockSize - default to 200ms of data
#ifdef HELIX_FEATURE_FLP_AUDREND
		if( ((((double) (m_pAudioFmt->ulSamplesPerSec)) * 
		    ((double) (m_pAudioFmt->uBitsPerSample)) * 
		    ((double) (m_pAudioFmt->uChannels))) < 
		    (((double) 0xFFFFFFFF) / MAX_DECODE_GRANULARITY)) )
		{
		    m_pAudioFmt->uMaxBlockSize = (UINT16)
						 (((double) (m_pAudioFmt->ulSamplesPerSec)) * 
						     ((double) (m_pAudioFmt->uBitsPerSample)) * 
						     ((double) (m_pAudioFmt->uChannels)) *
						     MAX_DECODE_GRANULARITY /
						     8000.0 +
						     ((double) (m_pAudioFmt->uBitsPerSample)) * // Compensate for possible round off
						     ((double) (m_pAudioFmt->uChannels)) /
						     8.0);
		    retVal = HXR_OK;
		}
#else   // HELIX_FEATURE_FLP_AUDREND
		if( ((((UINT32) (m_pAudioFmt->ulSamplesPerSec)) * 
		    ((UINT32) (m_pAudioFmt->uBitsPerSample)) * 
		    ((UINT32) (m_pAudioFmt->uChannels))) < 
		    (((ULONG32) 0xFFFFFFFF) / MAX_DECODE_GRANULARITY)) )
		{
		    m_pAudioFmt->uMaxBlockSize = (UINT16)
						 (((UINT32) (m_pAudioFmt->ulSamplesPerSec)) * 
						     ((UINT32) (m_pAudioFmt->uBitsPerSample)) * 
						     ((UINT32) (m_pAudioFmt->uChannels)) *
						     MAX_DECODE_GRANULARITY /
						     8000 +
						     ((UINT32) (m_pAudioFmt->uBitsPerSample)) * // Compensate for possible round off
						     ((UINT32) (m_pAudioFmt->uChannels)) /
						     8);
		    retVal = HXR_OK;
		}
#endif  // HELIX_FEATURE_FLP_AUDREND
            }
        }
    }

    return retVal;
}


/****************************************************************************
 *    CAudioFormat::GetDefaultPreroll
 */
ULONG32 CAudioFormat::GetDefaultPreroll(IHXValues* pValues)
{
    return DEFAULT_AUDIO_PREROLL;
}

ULONG32 CAudioFormat::GetMaximumPreroll(IHXValues* pValues)
{
    return MAXIMUM_AUDIO_PREROLL;
}

void CAudioFormat::FlushQueues(void)
{
    if (m_pPendingPacketQueue != NULL)
    {
	CMediaPacket* pQTPkt;
	while (!m_pPendingPacketQueue->IsEmpty())
	{
	    // empty all of the pending packets
	    // and delete them
	    pQTPkt = (CMediaPacket*) m_pPendingPacketQueue->RemoveHead();
	    CMediaPacket::DeletePacket(pQTPkt);
	}
    }
}


HX_RESULT CAudioFormat::GetAudioFormat(HXAudioFormat& audioFmt)
{
    audioFmt.uChannels = m_pAudioFmt->uChannels;
    audioFmt.uBitsPerSample = m_pAudioFmt->uBitsPerSample;
    audioFmt.ulSamplesPerSec = m_pAudioFmt->ulSamplesPerSec;
    audioFmt.uMaxBlockSize = m_pAudioFmt->uMaxBlockSize;

    return HXR_OK;
}


/****************************************************************************
 *  CAudioFormat::Enqueue
 */
HXBOOL CAudioFormat::Enqueue(IHXPacket* pPacket)
{
    CMediaPacket* pFramePacket = NULL;
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(m_pPendingPacketQueue);

    pFramePacket = CreateAssembledPacket(pPacket);

    if (pFramePacket)
    {
	m_pPendingPacketQueue->AddTail(pFramePacket);
    }
    
    return retVal;
}


HX_RESULT CAudioFormat::CreateAudioFrame(HXAudioData& audioData, 
					 AUDIO_STATE audioState)
{
    HX_RESULT retVal = HXR_NO_DATA;
    HXBOOL bTryAgain;

    do
    {
	bTryAgain = FALSE;

	HX_RELEASE(audioData.pData);
	audioData.ulAudioTime = 0;

	retVal = DecodeAudioData(audioData, audioState == AUDIO_END_OF_PACKETS);

	if (retVal == HXR_OK)
	{
	    if (audioData.pData && AdjustAudioData(audioData))
	    {
		// once we are passed the start time, disable it as a
		// a requirement to prevent clipping on wrap-around
		m_bPostStartTime = TRUE;
		
		// if we've fulfilled the discard, reset state
		if ((m_ulForceDiscardUntilTime != NO_TIME_SET) && 
		    IsTimeGreater(audioData.ulAudioTime + 
		    ConvertBytesToMs(audioData.pData->GetSize()),
		    m_ulForceDiscardUntilTime))
		{
		    m_ulForceDiscardUntilTime = NO_TIME_SET;
		}
	    }
	    else
	    {
		retVal = HXR_NO_DATA;
		HX_RELEASE(audioData.pData);
		bTryAgain = TRUE;
	    }
	}
    } while (bTryAgain);

    return retVal;
}


CMediaPacket* CAudioFormat::PeekAudioPacket(void)
{
    CMediaPacket* pAudioPacket = NULL;

    if (m_pPendingPacketQueue && (!m_pPendingPacketQueue->IsEmpty()))
    {
	pAudioPacket = (CMediaPacket*) m_pPendingPacketQueue->GetHead();
    }

    return pAudioPacket;
}


CMediaPacket* CAudioFormat::GetAudioPacket(void)
{
    CMediaPacket* pAudioPacket = NULL;

    if (m_pPendingPacketQueue && (!m_pPendingPacketQueue->IsEmpty()))
    {
	pAudioPacket = (CMediaPacket*) m_pPendingPacketQueue->RemoveHead();
    }

    return pAudioPacket;
}

UINT32 CAudioFormat::GetNumQueuedAudioPackets()
{
    UINT32 ulRet = 0;

    if (m_pPendingPacketQueue)
    {
        ulRet = m_pPendingPacketQueue->GetCount();
    }

    return ulRet;
}

HX_RESULT CAudioFormat::PutAudioPacket(CMediaPacket* pMediaPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(m_pPendingPacketQueue);

    if (m_pPendingPacketQueue && pMediaPacket)
    {
	m_pPendingPacketQueue->AddTail(pMediaPacket);
	retVal = HXR_OK;
    }

    return retVal;
}

CMediaPacket* CAudioFormat::CreateAssembledPacket(IHXPacket* pPayloadPacket)
{
    return NULL;
}


HX_RESULT CAudioFormat::DecodeAudioData(HXAudioData& audioData,
					HXBOOL bFlushCodec)
{
    return HXR_NOTIMPL;
}

    
void CAudioFormat::DiscardAudioUntil(UINT32 ulTimestamp)
{
    // cannot equal NO_TIME_SET so we bump it up one
    if (ulTimestamp == NO_TIME_SET)
    {
	ulTimestamp += 1;
    }

    m_ulForceDiscardUntilTime = ulTimestamp;

    // look through the re-assembled packet queue and delete data
    // from there that's earlier than the discard until time
    
    CMediaPacket* pQTPktCurrent = NULL;
    CMediaPacket* pQTPktNext = NULL;
    HXBOOL        bPacketDeleted = FALSE;
	ULONG32       ulLastestDeletedTimestamp;
    
    while(!m_pPendingPacketQueue->IsEmpty())
		{
    	pQTPktCurrent = (CMediaPacket*) m_pPendingPacketQueue->GetHead();
    	m_pPendingPacketQueue->RemoveHead();
    	if(!m_pPendingPacketQueue->IsEmpty())
    		{
    		pQTPktNext = (CMediaPacket*) m_pPendingPacketQueue->GetHead();
    		}
    	else
    		{
    	    m_pPendingPacketQueue->AddHead(pQTPktCurrent);
    		break;
    		}
   	
    	if (pQTPktCurrent->m_ulTime > ulTimestamp || (pQTPktCurrent->m_ulTime <= ulTimestamp && pQTPktNext->m_ulTime > ulTimestamp))
    		{
    	    m_pPendingPacketQueue->AddHead(pQTPktCurrent);
    		break;
    		}
    	else
    		{
			ulLastestDeletedTimestamp = pQTPktCurrent->m_ulTime;
    		CMediaPacket::DeletePacket(pQTPktCurrent);
    		bPacketDeleted = TRUE;
    		}
		}
    if(bPacketDeleted)
    	{
		InputAudioDiscardedNotification(ulTimestamp, ulLastestDeletedTimestamp);
    	}
}

void CAudioFormat::InputAudioDiscardedNotification(UINT32 /*ulTimestamp*/, ULONG32 /*ulLastDeletedTimestamp*/)
{
	//Derived classes have to clean up their variables properly.
}



void CAudioFormat::Reset(void)
{
    m_bPostStartTime = FALSE;
    m_ulForceDiscardUntilTime = NO_TIME_SET;

    FlushQueues();
}

void CAudioFormat::OverrideFactory(IHXCommonClassFactory* pCommonClassFactory)
{
    HX_ASSERT(pCommonClassFactory);

    if (pCommonClassFactory)
    {
	HX_RELEASE(m_pCommonClassFactory);
	m_pCommonClassFactory = pCommonClassFactory;
	m_pCommonClassFactory->AddRef();
    }
}

void CAudioFormat::SetCompressionRation(double fCompressionRatio)
{
    HX_ASSERT(fCompressionRatio > 0.0);
    
    if (fCompressionRatio > 0.0)
    {
	m_fCompressionRatio = fCompressionRatio;
    }
}


UINT32 CAudioFormat::ConvertMsToBytes(UINT32 ulMs)
{
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulMs) * 
		    ((double) m_pAudioFmt->uChannels) * 
		    ((double) m_pAudioFmt->uBitsPerSample) *
		    ((double) m_pAudioFmt->ulSamplesPerSec) / 
		    8000.0 + 
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND
    UINT32 ulBitsPerSecond = ((UINT32) m_pAudioFmt->uChannels) * 
			     ((UINT32) m_pAudioFmt->uBitsPerSample) *
			     ((UINT32) m_pAudioFmt->ulSamplesPerSec);

    HX_ASSERT(ulMs < (((ULONG32) 0xFFFFF05F) / ulBitsPerSecond));

    return (ulMs * ulBitsPerSecond + 4000) / 8000;
#endif	// HELIX_FEATURE_FLP_AUDREND
}

UINT32 CAudioFormat::ConvertBytesToMs(UINT32 ulBytes)
{    	
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulBytes) * 
		    8000.0 /
		    (((double) m_pAudioFmt->uChannels) * 
		     ((double) m_pAudioFmt->uBitsPerSample) *
		     ((double) m_pAudioFmt->ulSamplesPerSec)) + 
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND
    UINT32 ulBitsPerSecond = ((UINT32) m_pAudioFmt->uChannels) * 
			     ((UINT32) m_pAudioFmt->uBitsPerSample) *
			     ((UINT32) m_pAudioFmt->ulSamplesPerSec);

    HX_ASSERT(ulBytes < 536871);

    return ((ulBytes * 8000) / ulBitsPerSecond + 
	    ((ulBytes * 8000) % ulBitsPerSecond + (ulBitsPerSecond >> 1)) / ulBitsPerSecond);
#endif	// HELIX_FEATURE_FLP_AUDREND
}

UINT32 CAudioFormat::ConvertMsToSamples(UINT32 ulMs)
{
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulMs) /
		    1000.0 *
		    ((double) m_pAudioFmt->uChannels) *
		    ((double) m_pAudioFmt->ulSamplesPerSec) +
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND

    UINT32 ulCombinedSamplesPerSecond = ((UINT32) m_pAudioFmt->uChannels) *
				        ((UINT32) m_pAudioFmt->ulSamplesPerSec);

    return ((ulMs / 1000) * ulCombinedSamplesPerSecond +
	    ((ulMs % 1000) * ulCombinedSamplesPerSecond + 500) / 1000);
#endif	// HELIX_FEATURE_FLP_AUDREND
}

UINT32 CAudioFormat::ConvertSamplesToMs(UINT32 ulSamples)
{
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulSamples) /
		    (((double) m_pAudioFmt->uChannels) *
		     ((double) m_pAudioFmt->ulSamplesPerSec)) *
		    1000.0 + 
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND

    UINT32 ulCombinedSamplesPerSecond = ((UINT32) m_pAudioFmt->uChannels) *
				        ((UINT32) m_pAudioFmt->ulSamplesPerSec);

    return ((ulSamples / ulCombinedSamplesPerSecond) * 1000 +
	    ((ulSamples % ulCombinedSamplesPerSecond) * 1000 + 500) / ulCombinedSamplesPerSecond);
#endif	// HELIX_FEATURE_FLP_AUDREND
}

UINT32 CAudioFormat::ConvertBytesToSamples(UINT32 ulBytes)
{
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulBytes) *
		    8.0 /
		    ((double) m_pAudioFmt->uBitsPerSample) +
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND

    UINT32 ulBitsPerSample = ((UINT32) m_pAudioFmt->uBitsPerSample);

    return (((ulBytes / ulBitsPerSample) << 3) +
	    (((ulBytes % ulBitsPerSample) << 3) + (ulBitsPerSample >> 1)) / ulBitsPerSample);
#endif	// HELIX_FEATURE_FLP_AUDREND
}

UINT32 CAudioFormat::ConvertSamplesToBytes(UINT32 ulSamples)
{    	
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulSamples) *
		    ((double) m_pAudioFmt->uBitsPerSample) /
		    8.0 + 
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND

    UINT32 ulBitsPerSample = ((UINT32) m_pAudioFmt->uBitsPerSample);

    return ((ulSamples / 8) * ulBitsPerSample +
	    ((ulSamples % 8) * ulBitsPerSample + 4) / 8);
#endif	// HELIX_FEATURE_FLP_AUDREND
}

UINT32 CAudioFormat::ConvertCompressedBytesToMs(UINT32 ulBytes)
{
    return (UINT32)(((double) ulBytes) * 
		    8000.0 /
		    ((double) m_pAudioFmt->uChannels) * 
		    ((double) m_pAudioFmt->uBitsPerSample) *
		    ((double) m_pAudioFmt->ulSamplesPerSec) *
		    m_fCompressionRatio +
		    0.5);
}

UINT32 CAudioFormat::ConvertMsToCompressedBytes(UINT32 ulMs)
{
    return (UINT32)(((double) ulMs) * 
		    ((double) m_pAudioFmt->uChannels) * 
		    ((double) m_pAudioFmt->uBitsPerSample) *
		    ((double) m_pAudioFmt->ulSamplesPerSec) / 
		    8000.0 /
		    m_fCompressionRatio + 
		    0.5);
}

UINT32 CAudioFormat::ConvertMsToTime(UINT32 ulMs)
{
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulMs) /
		    1000.0 *
		    ((double) m_pAudioFmt->ulSamplesPerSec) +
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND

    UINT32 ulSamplesPerSec = ((UINT32) m_pAudioFmt->ulSamplesPerSec);

    return ((ulMs / 1000) * ulSamplesPerSec +
	    ((ulMs % 1000) * ulSamplesPerSec + 500) / 1000);
#endif	// HELIX_FEATURE_FLP_AUDREND
}

UINT32 CAudioFormat::ConvertTimeToMs(UINT32 ulTime)
{   
#ifdef HELIX_FEATURE_FLP_AUDREND
    return (UINT32)(((double) ulTime) /
		    ((double) m_pAudioFmt->ulSamplesPerSec) *
		    1000.0 + 
		    0.5);
#else	// HELIX_FEATURE_FLP_AUDREND

    UINT32 ulSamplesPerSec = ((UINT32) m_pAudioFmt->ulSamplesPerSec);

    return ((ulTime / ulSamplesPerSec) * 1000 +
	    ((ulTime % ulSamplesPerSec) * 1000 + (ulSamplesPerSec >> 1)) / ulSamplesPerSec);
#endif	// HELIX_FEATURE_FLP_AUDREND
}


HXBOOL CAudioFormat::ClipAudioBuffer(HXAudioData* pAudioData, 
				     UINT32 ulAudioTime, 
				     HXBOOL bFromStart)
{
    HXBOOL	bResult = FALSE;
    UINT32	ulDuration = 0;
    UINT32	ulSize = 0;
    UINT32	ulExtraInMs = 0;
    UINT32	ulExtraInBytes = 0;
    IHXBuffer* pBuffer = NULL;

    HX_ASSERT(pAudioData);
    HX_ASSERT(pAudioData->pData);

    ulSize = (pAudioData->pData)->GetSize();

    // caculate the worth of this data in ms
    ulDuration = ConvertBytesToMs(ulSize);
    
    // trim off any extra bytes
    if (bFromStart)
    {
	HX_ASSERT(IsTimeLess(pAudioData->ulAudioTime, ulAudioTime));

	if (IsTimeGreater(pAudioData->ulAudioTime + ulDuration, ulAudioTime))
	{	 	    
	    // trim off partial data
	    ulExtraInMs = ulAudioTime - pAudioData->ulAudioTime;

	    // convert to bytes
	    ulExtraInBytes = ConvertMsToBytes(ulExtraInMs);

	    // align in sample boundary
	    ulExtraInBytes -= (ulExtraInBytes % (m_pAudioFmt->uBitsPerSample * 
			       m_pAudioFmt->uChannels / 8));

	    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, 
		(void**) &pBuffer);

	    if (pBuffer)
	    {
		if (pBuffer->Set((pAudioData->pData)->GetBuffer() + ulExtraInBytes, 
				  ulSize - ulExtraInBytes)
		    == HXR_OK)
		{
		    pAudioData->pData->Release();

		    pAudioData->pData = pBuffer;	 	 
		    pAudioData->ulAudioTime = ulAudioTime;

		    bResult = TRUE;
		}
		else
		{
		    pBuffer->Release();
		}
	    }
	}
    }
    else
    {
	HX_ASSERT(IsTimeGreater(pAudioData->ulAudioTime + ulDuration, ulAudioTime));

	if (IsTimeLess(pAudioData->ulAudioTime, ulAudioTime))
	{
	    // trim off the extra ones
	    ulExtraInMs = pAudioData->ulAudioTime + ulDuration - ulAudioTime;

	    // convert to bytes
	    ulExtraInBytes = ConvertMsToBytes(ulExtraInMs);

	    // align in sample boundary
	    ulExtraInBytes -= (ulExtraInBytes % (m_pAudioFmt->uBitsPerSample * 
			       m_pAudioFmt->uChannels / 8));

	    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, 
		(void**) &pBuffer);

	    if (pBuffer)
	    {
		if (pBuffer->Set((pAudioData->pData)->GetBuffer(), 
				 ulSize - ulExtraInBytes)
		    == HXR_OK)
		{
		    pAudioData->pData->Release();

		    pAudioData->pData = pBuffer;

		    bResult = TRUE;
		}
	    }   
	}
    }

    return bResult;
}


inline HXBOOL CAudioFormat::AdjustAudioData(REF(HXAudioData) audioData)
{
    HXBOOL	bResult = TRUE;
    UINT32	ulDuration = 0;
    UINT32	ulSize = 0;

    if (((!m_bPostStartTime) && (m_ulTrackStartTime != NO_TIME_SET)) || 
	(m_ulTrackEndTime != NO_TIME_SET) ||
	(m_ulForceDiscardUntilTime != NO_TIME_SET))
    {
	ulSize = (audioData.pData)->GetSize();

	// caculate the worth of this data in ms
	ulDuration = ConvertBytesToMs(ulSize);
	
	// trim off any extra bytes
	if (((!m_bPostStartTime) &&
	     (m_ulTrackStartTime != NO_TIME_SET) && 
	     IsTimeLess(audioData.ulAudioTime, m_ulTrackStartTime)) ||
	    ((m_ulForceDiscardUntilTime != NO_TIME_SET) && 
	     IsTimeLess(audioData.ulAudioTime, m_ulForceDiscardUntilTime)))
	{
	    UINT32 ulStartTimeToUse = m_ulTrackStartTime;
	    
	    if (m_bPostStartTime ||
		(ulStartTimeToUse == NO_TIME_SET) ||
		IsTimeLess(m_ulTrackStartTime, m_ulForceDiscardUntilTime))
	    {
		ulStartTimeToUse = m_ulForceDiscardUntilTime;
	    }

	    bResult = ClipAudioBuffer(&audioData,
				      ulStartTimeToUse,
				      TRUE);
	}

	if ((m_ulTrackEndTime != NO_TIME_SET) &&
	    bResult &&
	    IsTimeGreater((audioData.ulAudioTime + ulDuration), m_ulTrackEndTime))
	{
	    bResult = ClipAudioBuffer(&audioData,
				      m_ulTrackEndTime,
				      FALSE);
	}
    }

    return bResult;
}


// *** IUnknown methods ***
/****************************************************************************
*  IUnknown::QueryInterface                                    ref:  hxcom.h
*
*  This routine indicates which interfaces this object supports. If a given
*  interface is supported, the object's reference count is incremented, and
*  a reference to that interface is returned. Otherwise a NULL object and
*  error code are returned. This method is called by other objects to
*  discover the functionality of this object.
*/
STDMETHODIMP CAudioFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*) this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/****************************************************************************
*  IUnknown::AddRef                                            ref:  hxcom.h
*
*  This routine increases the object reference count in a thread safe
*  manner. The reference count is used to manage the lifetime of an object.
*  This method must be explicitly called by the user whenever a new
*  reference to an object is used.
*/
STDMETHODIMP_(ULONG32) CAudioFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/****************************************************************************
*  IUnknown::Release                                           ref:  hxcom.h
*
*  This routine decreases the object reference count in a thread safe
*  manner, and deletes the object if no more references to it exist. It must
*  be called explicitly by the user whenever an object is no longer needed.
*/
STDMETHODIMP_(ULONG32) CAudioFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

UINT32 CAudioFormat::GetULONG32Property(IHXValues*  pValues,
                                        const char* pszName,
                                        UINT32      ulDefault)
{
    UINT32 ulRet = ulDefault;

    if (pValues && pszName)
    {
        pValues->GetPropertyULONG32(pszName, ulRet);
    }

    return ulRet;
}

