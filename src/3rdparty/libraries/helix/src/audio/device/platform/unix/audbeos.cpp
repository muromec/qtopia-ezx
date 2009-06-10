/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audbeos.cpp,v 1.6 2007/07/06 20:21:16 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

/*******************************************************************
 *	audbeos.cpp
 *
 *	CLASS: CAudioOutBeOS
 *	
 *	DESCRIPTION: Class implementation for BeOS-specific audio devices 
 *	
 *******************************************************************/

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxausvc.h"
#include "auderrs.h"
#include "hxaudev.h"
#include "hxtick.h"
#include "hxthread.h"
#include "hxbuffer.h"
#include "hxslist.h"
#include "hxengin.h"
#include "timeval.h"
#include "pckunpck.h"
#include "hxaudses.h"
#include "audbeos.h"

#include <PushGameSound.h>
#include <MediaDefs.h>

struct IHXCallback;  // forward declaration needed for callback.

CAudioOutBeOS::CAudioOutBeOS()
    : m_player(NULL)
    , m_bFirstWrite(TRUE)
    , m_pPendingBufferList(NULL)
    , m_ulLastPlayCursor(0)
    , m_ulCurrentPlayTime(0)
    , m_ulLastTimeSync(0)
    , m_bPlaying(FALSE)
	, m_ulBufferSize(0)
	, m_ulFrameSize(0)
	, m_ulNextWriteOffset(0)
	, m_ulOldBytesLeft(0)
	, m_ulBlockSize(0)
	, m_bGotAWrite(FALSE)
#if _BEOS_AUDIODEV_CALLBACK
	, m_pPlaybackCountCBTime(NULL)
	, m_bCallbackPending(FALSE)
	, m_PendingCallbackID(0)
#endif
{
#if _BEOS_AUDIODEV_CALLBACK
	m_pPlaybackCountCBTime = new Timeval;
#endif
}		   

CAudioOutBeOS::~CAudioOutBeOS()
{
	_Imp_Close();
#if _BEOS_AUDIODEV_CALLBACK
	delete m_pPlaybackCountCBTime;
#endif
}

HX_RESULT 
CAudioOutBeOS::_Imp_Open(const HXAudioFormat* pFormat)
{
	size_t	bufSize;
	void	*basePtr;

    SetFormat(pFormat);

#if _BEOS_AUDIODEV_CALLBACK
	if (m_pOwner)
	{
		m_pOwner->GetScheduler(&m_pScheduler);
		m_pScheduler->AddRef();
	}
#endif

	// set up the player with enough buffer space for about 4 seconds of audio
//	m_player = new BPushGameSound(m_gameSoundFormat.frame_rate, &m_gameSoundFormat, 4);

	// BPushGameSound->CurrentPosition is accurate to within a buffer
	// Therefore, to keep good synchronization between audio and video, a single
	// buffer should not have a longer duration than a frame of video. We don't
	// know here what the video framerate (if any) may be, but we can assume that it
	// isn't over 30fps. Thus, each audio buffer should be no longer than about 30 ms.
	// To store about 3 seconds of audio, we'll need 100 buffers.
	bufSize = (size_t)(m_gameSoundFormat.frame_rate * 0.03f);
	m_player = new BPushGameSound(bufSize, &m_gameSoundFormat, 100);

	if (m_player->InitCheck() != B_OK)
	{
		delete m_player;
		m_player = NULL;
		return RA_AOE_BADOPEN;
	}
	if (m_player->LockForCyclic(&basePtr, &bufSize) != BPushGameSound::lock_ok)
		m_player->UnlockCyclic();
	m_ulBufferSize = bufSize;
	m_ulBlockSize = m_ulBytesPerGran;

	return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_Close()
{
#if _BEOS_AUDIODEV_CALLBACK
	if (m_pScheduler)
	{
		// Remove callback from scheduler
		if (m_bCallbackPending)
		{
			m_pScheduler->Remove(m_PendingCallbackID);
			m_bCallbackPending = FALSE;
		}
		m_pScheduler->Release();
		m_pScheduler = NULL;
	}
#endif

	if (m_pPendingBufferList)
	{
		while (!m_pPendingBufferList->IsEmpty())
		{
			IHXBuffer	*pBuffer = (IHXBuffer *)m_pPendingBufferList->RemoveHead();
			pBuffer->Release();
			pBuffer = NULL;
		}

		delete m_pPendingBufferList;
		m_pPendingBufferList = NULL;
	}

	if (m_player)
	{
		m_player->StopPlaying();
		delete m_player;
		m_player = NULL;
	}
	m_bPlaying = FALSE;
    return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_Seek(ULONG32 ulSeekTime)
{
    return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_Pause()
{
	if (m_player)
	{
		size_t	curPos = m_player->CurrentPosition() * m_ulFrameSize;
		uint8	*pAudioPtr;
		size_t	i, ulAudioBytes;

		m_player->StopPlaying();
		if (m_player->LockForCyclic((void **)&pAudioPtr, &ulAudioBytes) == BPushGameSound::lock_ok && pAudioPtr)
		{
			// shift the audio data all back to the beginning of the buffer
			if (m_ulNextWriteOffset > curPos)
			{
				for (i = curPos; i < m_ulNextWriteOffset; i++)
				{
					pAudioPtr[i - curPos] = pAudioPtr[i];
				}
				m_ulNextWriteOffset = m_ulNextWriteOffset - curPos;
			}
			else
			{ // wrap-around
				uint32	remaining = ulAudioBytes - curPos;
				uint8	*tempBuf = new uint8[m_ulNextWriteOffset];

				for (i = 0; i < m_ulNextWriteOffset; i++)
				{
					tempBuf[i] = pAudioPtr[i];
				}
				for (i = curPos; i < ulAudioBytes; i++)
				{
					pAudioPtr[i - curPos] = pAudioPtr[i];
				}
				for (i = 0; i < m_ulNextWriteOffset; i++)
				{
					pAudioPtr[i + remaining] = tempBuf[i];
				}

				delete [] tempBuf;
				m_ulNextWriteOffset = remaining + m_ulNextWriteOffset;
			}
			m_player->UnlockCyclic();
			m_bGotAWrite = TRUE;
			m_ulLastPlayCursor = 0;
		}
	}

	m_bPlaying = FALSE;
	return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_Resume()
{
	if (m_player)
		m_player->StartPlaying();

	m_ulLastPlayCursor = 0;
	m_bPlaying = TRUE;
	m_ulLastTimeSync = HX_GET_TICKCOUNT();
	OnTimeSync();

	return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_Write(const HXAudioData* pAudioOutData)
{
	HX_RESULT	theErr = HXR_OK;
	IHXBuffer	*pBuffer = NULL;

#if _BEOS_AUDIODEV_CALLBACK
	if (m_bFirstWrite && pAudioOutData)
	{
		/*  Initialize the playback callback time. */
		HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
		m_pPlaybackCountCBTime->tv_sec = lTime.tv_sec;
		m_pPlaybackCountCBTime->tv_usec = lTime.tv_usec;

		m_bFirstWrite = FALSE;
		ReschedPlaybackCheck();
	}
#endif

	if (pAudioOutData && pAudioOutData->pData)
	{
		pAudioOutData->pData->AddRef();
	}

	if (m_pPendingBufferList && m_pPendingBufferList->GetCount() > 0)
	{
		if (pAudioOutData && pAudioOutData->pData)
			m_pPendingBufferList->AddTail(pAudioOutData->pData);
		pBuffer = (IHXBuffer *)m_pPendingBufferList->RemoveHead();
	}
	else
	{
		if (pAudioOutData && pAudioOutData->pData)
			pBuffer = pAudioOutData->pData;
	}

    HXBOOL    bCanContinue = TRUE;

	while (bCanContinue && pBuffer)
	{
		UINT32 ulBufferSize = pBuffer->GetSize();

		uint8* pAudioPtr1	= NULL;
		uint8* pAudioPtr2	= NULL;
		size_t curPos;
		size_t ulAudioBytes1 = 0;
		size_t ulAudioBytes2 = 0;
		size_t ulAudioBytesWritten1 = 0;
		size_t ulAudioBytesWritten2 = 0;

		if (m_player->LockForCyclic((void **)&pAudioPtr1, &ulAudioBytes1) == BPushGameSound::lock_ok && pAudioPtr1)
		{
			size_t	ulSizeToWrite, curPos;

			if (m_player->IsPlaying())
				curPos = m_player->CurrentPosition() * m_ulFrameSize;
			else
				curPos = 0;

			if (m_ulNextWriteOffset >= curPos)
			{
				pAudioPtr2 = pAudioPtr1;
				ulAudioBytes1 -= m_ulNextWriteOffset;
			}
			else
			{
				pAudioPtr2 = NULL;
				ulAudioBytes1 = curPos - m_ulNextWriteOffset - 1;
			}
			pAudioPtr1 += m_ulNextWriteOffset;

			if (ulAudioBytes1 < 0)
				ulAudioBytes1 = 0;

			ulSizeToWrite = ulBufferSize;
			if (ulSizeToWrite > ulAudioBytes1)
			{
				ulSizeToWrite = ulAudioBytes1;
			}

			if (ulSizeToWrite > 0)
			{
				::memcpy(pAudioPtr1, (void*) pBuffer->GetBuffer(), ulSizeToWrite); /* Flawfinder: ignore */
			}
			ulAudioBytesWritten1 = ulSizeToWrite;

			size_t	ulRemainingToWrite = ulBufferSize - ulSizeToWrite;

			ulAudioBytes2 = curPos - 1;
			if (ulRemainingToWrite > 0 && pAudioPtr2 && ulAudioBytes2 > 0)
			{
				ulSizeToWrite = ulRemainingToWrite;
				if (ulSizeToWrite > ulAudioBytes2)
				{
					ulSizeToWrite = ulAudioBytes2;
				}
				::memcpy(pAudioPtr2, (void*) (pBuffer->GetBuffer() + ulAudioBytesWritten1), ulSizeToWrite);
				ulAudioBytesWritten2 = ulSizeToWrite;
				ulRemainingToWrite -= ulSizeToWrite;
			}

			if (ulRemainingToWrite > 0)
			{
				IHXBuffer* pNewBuffer = NULL;
				if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
								    pBuffer->GetBuffer() + (ulBufferSize - ulRemainingToWrite),
								    ulRemainingToWrite,
								    m_pContext))
				{
				    if (!m_pPendingBufferList)
				    {
					m_pPendingBufferList = new CHXSimpleList;
				    }

				    m_pPendingBufferList->AddHead((void*) pNewBuffer);

				    // no more space in the secondary buffer
				    bCanContinue = FALSE;
				}
			}

			m_player->UnlockCyclic();

			m_ulNextWriteOffset	+= ulAudioBytesWritten1 + ulAudioBytesWritten2;
			if (pAudioOutData)
				m_bGotAWrite = TRUE;

			if (m_ulNextWriteOffset >= m_ulBufferSize)
			{
				m_ulNextWriteOffset -= m_ulBufferSize;
			}

			if (m_bFirstWrite)
			{
				m_bFirstWrite = FALSE;
			}
		} // lock
		else
		{
			bCanContinue = FALSE;
			if (!m_pPendingBufferList)
			{
				m_pPendingBufferList = new CHXSimpleList;
			}
			
			pBuffer->AddRef();
			m_pPendingBufferList->AddHead((void*) pBuffer);
		}

		pBuffer->Release();
		pBuffer = NULL;

		if (bCanContinue && m_pPendingBufferList && m_pPendingBufferList->GetCount() > 0)
		{
			pBuffer = (IHXBuffer*) m_pPendingBufferList->RemoveHead();
		}
	} // while

	if (m_bPlaying)
	{
		UINT32	ulCurrentTime = HX_GET_TICKCOUNT();
		if (CALCULATE_ELAPSED_TICKS(m_ulLastTimeSync, ulCurrentTime) > 100)
		{
			m_ulLastTimeSync = ulCurrentTime;
			OnTimeSync();
		}
	}

//@ This is an ugly hack, but it seems to work very well.
// Hopefully I can figure it out and fix it properly at some point.
// (mclifton 10/5/99)
/*if (pAudioOutData && m_bPlaying && m_pPendingBufferList && m_pPendingBufferList->GetCount() > 0)
{
snooze(50000);
_Imp_Write(NULL);
}
if (pAudioOutData && m_bPlaying && m_pPendingBufferList && m_pPendingBufferList->GetCount() > 0)
{
snooze(50000);
_Imp_Write(NULL);
}*/

	return theErr;
}

HX_RESULT 
CAudioOutBeOS::_Imp_Reset()
{
    m_ulCurrentPlayTime = 0;
    m_bFirstWrite = TRUE;

	m_ulNextWriteOffset = 0;

	if (m_player)
		m_player->StopPlaying();
	m_bPlaying = FALSE;

    return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_Drain()
{
    return HXR_OK;
}

HXBOOL 
CAudioOutBeOS::_Imp_SupportsVolume()
{
    return TRUE;
}

UINT16 
CAudioOutBeOS::_Imp_GetVolume()
{
	float	vol = 0.0f;

	if (m_player)
		vol = m_player->Gain();
	return (UINT16)(vol * 100.0f);
}

HX_RESULT 
CAudioOutBeOS::_Imp_SetVolume(const UINT16 uVolume)
{
	float	vol = (float)uVolume / 100.0f;

	if (m_player)
		m_player->SetGain(vol);
    return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_CheckFormat(const HXAudioFormat* pFormat)
{
    return HXR_OK;
}

HX_RESULT 
CAudioOutBeOS::_Imp_GetCurrentTime(ULONG32& ulCurrentTime)
{
	size_t	curPos = 0;

	if (m_player)
	{
		if (m_player->IsPlaying())
			curPos = m_player->CurrentPosition() * m_ulFrameSize;

		// This method of calculating elapsed time was basically copied over from
		// the DirectSound code. The bad news is that it is error-prone. Converting
		// from bytes to milliseconds is prone to roundoff, leading to an accumulation
		// of error, causing a drift of synchronization between audio and video.
		// (That DirectSound code has caused me nothing but headaches...)
		//m_ulCurrentPlayTime +=
		//	CalcMs(CalculateElapsedBytes(m_ulLastPlayCursor, curPos));

		// The more accurate way to do it is to accumulate elapsed bytes, then
		// convert the total to milliseconds. The elapsed bytes is accurate to
		// within a video frame or so. Even though it can't be absolutely accurate,
		// at least it won't drift over time.
		m_ulCurrentPlayTime +=
			CalculateElapsedBytes(m_ulLastPlayCursor, curPos);

		m_ulLastPlayCursor = curPos;
	}
	// old method
	//ulCurrentTime = m_ulCurrentPlayTime;
	// new method
	ulCurrentTime = CalcMs(m_ulCurrentPlayTime);

	return HXR_OK;
}


UINT16
CAudioOutBeOS::_NumberOfBlocksRemainingToPlay(void)
{
	UINT32	res = 0;
	size_t	curPos = 0;

	// add up the number of audio bytes queued up
	if (m_pPendingBufferList)
	{
		LISTPOSITION i = m_pPendingBufferList->GetHeadPosition();

		while (i)
		{
			res += ((IHXBuffer *)m_pPendingBufferList->GetAt(i))->GetSize();
			m_pPendingBufferList->GetNext(i);
		}
	}

	// add in the bytes that are currently in the playback buffer
	if (m_player)
	{
		UINT32	playingBytes = 0;

		if (m_player->IsPlaying())
			curPos = m_player->CurrentPosition() * m_ulFrameSize;

		if (curPos < m_ulNextWriteOffset)
			playingBytes += m_ulNextWriteOffset - curPos;
		else
			playingBytes += (m_ulBufferSize - curPos) + m_ulNextWriteOffset;

		res += playingBytes;
	}

	if (m_bGotAWrite)
		m_ulOldBytesLeft = res;
	else if (res > m_ulOldBytesLeft)
	{
		fprintf(stderr, "Buffer overflow!\n");
		// This is a bad situation - I wish this never happened.
		// But what should I do when it does happen?
		// I used to return 0, since I thought that would force more
		// audio buffers my way, but it also seems to introduce more glitches.
		//res = 0;
	}
	m_bGotAWrite = FALSE;

	res /= m_ulBlockSize;

	return res;
}

void
CAudioOutBeOS::SetFormat(const HXAudioFormat* pFormat)
{
    ::memset(&m_gameSoundFormat, 0, sizeof(gs_audio_format));

	m_gameSoundFormat.frame_rate = pFormat->ulSamplesPerSec;
	m_gameSoundFormat.channel_count = pFormat->uChannels;
	switch (pFormat->uBitsPerSample)
	{
		case 8:
			m_gameSoundFormat.format = gs_audio_format::B_GS_U8;
			break;
		case 16:
			m_gameSoundFormat.format = gs_audio_format::B_GS_S16;
			break;
	}
	m_gameSoundFormat.byte_order = B_MEDIA_LITTLE_ENDIAN;
	m_gameSoundFormat.buffer_size = 2048;

	m_ulFrameSize = m_gameSoundFormat.channel_count * ((m_gameSoundFormat.format==gs_audio_format::B_GS_U8)?1:2);
}

inline UINT32 
CAudioOutBeOS::CalcMs(ULONG32 ulNumBytes)
{
    return ( (ULONG32) (( 1000.0
		/ (m_ulFrameSize
		*  m_gameSoundFormat.frame_rate) )
		*  ulNumBytes) );
}


inline UINT32
CAudioOutBeOS::CalculateElapsedBytes(UINT32 ulLastBytePos, UINT32 ulCurrentBytePos)
{
	 return ((ulCurrentBytePos >= ulLastBytePos) ? (ulCurrentBytePos - ulLastBytePos) : (ulCurrentBytePos + (m_ulBufferSize - ulLastBytePos)));
}

#if _BEOS_AUDIODEV_CALLBACK
void
CAudioOutBeOS::DoTimeSyncs(void)
{
	ReschedPlaybackCheck();
	OnTimeSync();

	return;
}

HX_RESULT
CAudioOutBeOS::ReschedPlaybackCheck()
{
	HX_RESULT theErr = HXR_OK;

	if (m_bCallbackPending)
		return theErr;

	*m_pPlaybackCountCBTime += (int)(m_ulGranularity * 1000) / 2;
	// Put this back in the scheduler.
	HXPlaybackCountCb	*pCallback = 0;
	pCallback = new HXPlaybackCountCb(TRUE);
	if (pCallback)
	{
		pCallback->m_pAudioDeviceObject = this;
		m_bCallbackPending = TRUE;
		m_PendingCallbackID = m_pScheduler->AbsoluteEnter(pCallback, *((HXTimeval *)m_pPlaybackCountCBTime));
	}
	else
		theErr = HXR_OUTOFMEMORY;  // but ignored, why?

	return theErr;
}

CAudioOutBeOS::HXPlaybackCountCb::HXPlaybackCountCb(HXBOOL timed) 
	: m_lRefCount(0)
	, m_pAudioDeviceObject(0)
	, m_timed(timed)
{
}

CAudioOutBeOS::HXPlaybackCountCb::~HXPlaybackCountCb()
{
}

/*
 * IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your 
//              object.
//
STDMETHODIMP CAudioOutBeOS::HXPlaybackCountCb::QueryInterface(REFIID riid, void** ppvObj)
{
	if (IsEqualIID(riid, IID_IHXCallback))
	{
		AddRef();
		*ppvObj = (IHXCallback*)this;
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
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) CAudioOutBeOS::HXPlaybackCountCb::AddRef()
{
	return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) CAudioOutBeOS::HXPlaybackCountCb::Release()
{
	if (InterlockedDecrement(&m_lRefCount) > 0)
	{
		return m_lRefCount;
	}

	delete this;
	return 0;
}

/*
 *      IHXPlaybackCountCb methods
 */
STDMETHODIMP CAudioOutBeOS::HXPlaybackCountCb::Func(void)
{
	if (m_pAudioDeviceObject)
	{
		if (!m_timed)
		{
			m_pAudioDeviceObject->_Imp_Write(NULL);
		}
		else
		{
			m_pAudioDeviceObject->_Imp_Write(NULL);
			m_pAudioDeviceObject->m_bCallbackPending = FALSE;
			m_pAudioDeviceObject->DoTimeSyncs();
		}
	}

	return HXR_OK;
}
#endif
