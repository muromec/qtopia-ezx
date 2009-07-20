/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audqnx.cpp,v 1.7 2007/07/06 20:21:16 jfinnecy Exp $
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
 *
 *	audqnx.cpp
 *
 *	CLASS: CAudioOutQNX
 *	
 *	DESCRIPTION: Class implementation for QNX-specific audio devices 
 *	
 *******************************************************************/

#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/asound.h>

#include "hxcom.h"

#include "hxresult.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxslist.h"
#include "hxstrutl.h"

#include "timeval.h"
#include "audqnx.h"

#include "hxaudses.h"
#include "hxtick.h"
#include "chxpckts.h"
#include "pckunpck.h"

#include "debug.h"

struct IHXCallback;

CAudioOutQNX::CAudioOutQNX() :
	m_wID( -1 ),
	mixm_wID( -1 ),
	m_wPCMChannel( -1 ),
	m_wState( RA_AOS_CLOSED ),
	m_wLastError( RA_AOE_NOERR ),
	m_bMixerPresent(FALSE),
	m_wBlockSize(0),
	m_ulLastNumBytes (0),
	m_ulBytesRemaining(0),
	m_ulTotalWritten(0),
	m_bFirstWrite (TRUE),
	m_pPlaybackCountCBTime(0),
	m_PendingCallbackID (0),
	m_bCallbackPending(FALSE),
	m_paused(FALSE),
	m_pWriteList(NULL),
	m_last_audio_time(0),
	m_ulPauseBytes(0),
    m_ulDeviceBufferSize(0),
    m_pRollbackBuffer(NULL)
{

	// Use Photon registry later 
	// Get AUDIODEV environment var to find audio device of choice
	char *adev = (char*)getenv( "AUDIODEV" ); /* Flawfinder: ignore */
	char *mdev = (char*)getenv( "MIXERDEV" ); /* Flawfinder: ignore */

	// Use defaults if no environment variable is set.
	if ( adev )
	{
	    SafeStrCpy( m_DevName, adev, DEVICE_NAME_SIZE );
	}
	else
	{
	    SafeStrCpy( m_DevName, "/dev/pcm00", DEVICE_NAME_SIZE );	// default
	}

	if ( mdev )
	{
	    SafeStrCpy( m_DevCtlName, mdev, DEVICE_NAME_SIZE );
	}
	else
	{
	    SafeStrCpy( m_DevCtlName, "/dev/mixer00", DEVICE_NAME_SIZE );   // default for volume
	}

	m_pPlaybackCountCBTime = new Timeval;

	m_pWriteList = new CHXSimpleList();
}

CAudioOutQNX::~CAudioOutQNX()
{

    // Check to make sure device is closed
    if ( m_wState != RA_AOS_CLOSED ) 
    {
        HX_ASSERT( "Device not closed in dtor." == NULL );
		_Imp_Close();
    }
 
	HX_RELEASE( m_pScheduler );

    //Just in case it isn't empty at this point.
    while( m_pWriteList && !m_pWriteList->IsEmpty() )
    {
        IHXBuffer* pBuffer = (IHXBuffer *)(m_pWriteList->RemoveHead());
        HX_RELEASE( pBuffer );
    }

	HX_DELETE( m_pWriteList );

	HX_VECTOR_DELETE( m_pRollbackBuffer );
}

UINT16 CAudioOutQNX::_Imp_GetVolume()
{
	struct snd_mixer_channel_direction_t cdata;

    if (!m_bMixerPresent)
		OpenMixer();

    if ( !m_bMixerPresent ) 
		return m_uCurVolume;

	cdata.channel = m_wPCMChannel;
	if ( ioctl( mixm_wID, SND_MIXER_IOCTL_CHANNEL_OREAD, &cdata ) == -1 )
		return (0);

	m_uCurVolume = cdata.volume ; 

	return m_uCurVolume;
}

HX_RESULT CAudioOutQNX::_Imp_SetVolume( UINT16 uVolume )
{
	struct snd_mixer_channel_direction_t cdata;

    if (!m_bMixerPresent)
		OpenMixer();
    
	if ( !m_bMixerPresent ) 
		return RA_AOE_NOERR;

	cdata.channel = m_wPCMChannel;
	if ( ioctl( mixm_wID, SND_MIXER_IOCTL_CHANNEL_OREAD, &cdata ) == -1 )
		return ( m_wLastError = RA_AOE_NOTSUPPORTED );

	cdata.volume  = uVolume;
	if ( ioctl( mixm_wID, SND_MIXER_IOCTL_CHANNEL_OWRITE, &cdata ) == -1 )
		return ( m_wLastError = RA_AOE_NOTSUPPORTED );

	return RA_AOE_NOERR;
}

HXBOOL CAudioOutQNX::_Imp_SupportsVolume()
{
	return TRUE;
}

HX_RESULT CAudioOutQNX:: _Imp_Open ( const HXAudioFormat* pFormat )
{

printf( "_imp_open\n" );
	m_ulLastTimeChecked = (UINT32) -1;
	m_ulLastTimeReturned = 0;

	// Get the core scheduler interface; Use this to schedule polling
	// the audio device for number of bytes played.
#if 0
	if ( m_pOwner )
	{
		m_pOwner->GetScheduler( &m_pScheduler );
		m_pScheduler->AddRef();
	}
#else
    // Get the core scheduler interface; Use this to schedule polling
    // the audio device for number of bytes played.
    if ( m_pContext )
    {
		m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler );
    }
#endif

	// Check state. Could already be opened.
	if ( m_wState == RA_AOS_OPEN_PAUSED || m_wState == RA_AOS_OPEN_PLAYING ||
		m_wState == RA_AOS_OPENING )
		return RA_AOE_NOERR;

	// Open audio device.
	if ( m_wID < 0 )
		m_wID = open ( m_DevName, O_WRONLY | O_NONBLOCK );

	if ( m_wID < 0 )
		return ( m_wLastError = RA_AOE_BADOPEN );

	m_wBlockSize = m_ulBytesPerGran;  //pFormat->uMaxBlockSize;
	m_uSampFrameSize = pFormat->uBitsPerSample / 8;

	// Set device state
	m_wState = RA_AOS_OPENING;

	// Configure the audio device.
	AUDIOERROR iVal = SetDeviceConfig( pFormat );
	if (iVal != RA_AOE_NOERR) 
	{
		close ( m_wID );
		m_wID = -1;
		return iVal;
	}

	// Find out if mixer is there.. the mixer controls volume.
	// If there is no mixer device, then we handle volume manually by
	// multiplying the samples by the volume level in the Write() method.

	if (!m_bMixerPresent)
		OpenMixer();
	
	IHXAsyncIOSelection* pAsyncIO = NULL;
	if( HXR_OK == m_pContext->QueryInterface(IID_IHXAsyncIOSelection, (void**)&pAsyncIO))
	{
	    pAsyncIO->Add(new HXPlaybackCountCb(FALSE), m_wID, PNAIO_WRITE);
		HX_RELEASE( pAsyncIO );
	}
     
	HX_ASSERT( m_ulDeviceBufferSize != 0 );
	if( NULL == m_pRollbackBuffer)
	{
		m_pRollbackBuffer = new UCHAR[m_ulDeviceBufferSize];
		memset( m_pRollbackBuffer, '0', m_ulDeviceBufferSize );
	}

	return RA_AOE_NOERR;
}

HX_RESULT CAudioOutQNX::_Imp_Close()
{

printf( "_imp_close\n" );
	m_wState = RA_AOS_CLOSING;

	/* reset pause offset */
	m_ulPauseBytes = 0;

	_Imp_Reset( );

	// Close the audio device.
	if ( m_wID >= 0 ) 
	{
		close ( m_wID );
		IHXAsyncIOSelection* pAsyncIO;
		if( HXR_OK == m_pContext->QueryInterface(IID_IHXAsyncIOSelection, (void**)&pAsyncIO))
        {
	    	pAsyncIO->Remove(m_wID, PNAIO_WRITE);
	    	pAsyncIO->Release();
		}
		m_wID = -1;
	}

    CloseMixer();

	m_wState = RA_AOS_CLOSED;

	// Remove callback from scheduler
	if (m_bCallbackPending)
	{
	    m_pScheduler->Remove(m_PendingCallbackID);
	    m_bCallbackPending = FALSE;
	}

	HX_VECTOR_DELETE( m_pRollbackBuffer );

	return RA_AOE_NOERR;
}

HX_RESULT CAudioOutQNX::_Imp_Write ( const HXAudioData* pAudioOutHdr )
{
    IHXBuffer*	pBuffer		= NULL;
	UCHAR*		pData		= 0;
	ULONG32		ulBufLen	= 0;

	// Schedule callbacks
	if ( m_bFirstWrite && pAudioOutHdr)
	{
  	    m_bFirstWrite = FALSE;

	    /*  Initialize the playback callback time. */
	    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
	    m_pPlaybackCountCBTime->tv_sec = lTime.tv_sec;
	    m_pPlaybackCountCBTime->tv_usec = lTime.tv_usec;

	    /*  Scheduler playback callback. */
    	ReschedPlaybackCheck();
	}

	if ( m_paused )
	{
		if ( !pAudioOutHdr )
			return RA_AOE_NOERR;

		IHXBuffer* pNewBuffer = NULL;
		if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
						    pAudioOutHdr->pData->GetBuffer(),
						    pAudioOutHdr->pData->GetSize(),
						    m_pContext))
		{
		    m_pWriteList->AddTail(pNewBuffer);
		}

	    return RA_AOE_NOERR;
	}

	HXBOOL bWroteSomething = TRUE;
	do 
	{
		bWroteSomething = FALSE;

	    if(m_pWriteList->GetCount() <= 0)
	    {
			if(!pAudioOutHdr)
			    return RA_AOE_NOERR;

			pData = (UCHAR*)pAudioOutHdr->pData->GetBuffer();
			ulBufLen = pAudioOutHdr->pData->GetSize();
	    }
	    else
	    {
			if(pAudioOutHdr)
			{
			    IHXBuffer* pNewBuffer = NULL;
			    if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
								pAudioOutHdr->pData->GetBuffer(),
								pAudioOutHdr->pData->GetSize(),
								m_pContext))
			    {
				m_pWriteList->AddTail(pNewBuffer);
			    }
			}

		pBuffer = (IHXBuffer*)m_pWriteList->RemoveHead();
		pData = pBuffer->GetBuffer();
		ulBufLen = pBuffer->GetSize();
	    }

		// Write audio data to device.
		int		count = 0;
		count = write(m_wID, pData, ulBufLen);

		if ( count == -1 )
		{
			// Rebuffer the data
			IHXBuffer* pNewBuffer = NULL;
			if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
							    pData,
							    ulBufLen,
							    m_pContext))
			{
			    m_pWriteList->AddHead( pNewBuffer );
			}
		}

		// anything that is left over must be added to the write list at 
		// the beginning
		if (count != -1 && count != ulBufLen) 
		{ 
			// replace the extra data in the writelist
			IHXBuffer* pNewBuffer = NULL;
			if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
							    pData + count,
							    ulBufLen - count,
							    m_pContext))
			{
			    m_pWriteList->AddHead(pNewBuffer);
			}
		}

		if (count != -1)
		{
			bWroteSomething = TRUE;
			m_ulTotalWritten += count;

            // If we wrote to the device we need to keep a copy of the 
            // data our device buffer. We use this to 'rewind' the data
            // in case we get paused.
            // If we could write ulCount without blocking then there was at 
            // least that much room in the device and since m_pRollbackBuffer
            // is as large as the devices buffer, we can safely shift and copy.
            // Add the new stuff to the end pushing the rest of the data forward.
                        
                        // Throw an assert here
                        HX_ASSERT(count <= m_ulDeviceBufferSize);
                        // Now protect against a crash
                        if (count > m_ulDeviceBufferSize) count = m_ulDeviceBufferSize;

            memmove( m_pRollbackBuffer, m_pRollbackBuffer+count, m_ulDeviceBufferSize-count);
            memcpy( m_pRollbackBuffer+m_ulDeviceBufferSize-count, pData, count ); /* Flawfinder: ignore */

		}

		HX_RELEASE( pBuffer );

		pBuffer = NULL;
		pAudioOutHdr = NULL; // Don't add the same buffer again

	} while( bWroteSomething );

	return RA_AOE_NOERR;
}

HX_RESULT CAudioOutQNX::_Imp_Seek(ULONG32 ulSeekTime)
{
    return HXR_OK;
}

HX_RESULT CAudioOutQNX::_Imp_Pause()
{

    m_paused = TRUE;
	
	// Find out how much we have left in the device's buffer.
	int pause_bytes = GetPlaybackBytes( );
	ULONG32 ulNumBytesToRewind = m_ulTotalWritten - pause_bytes;

	// Reset player and discard all the data in the device's buffer
	if( _Imp_Reset() != RA_AOE_NOERR )
	{
	    //We will just ignore it. That means the buffer will just drain
	    //and we will hear it again when they unpause.
	}

	// Add it to the front of the write buffer.

	// Make sure we only deal with full samples. Bytes-per-sample*num-channels.
	int nRem = ulNumBytesToRewind % (m_uSampFrameSize * m_num_channels);
	ulNumBytesToRewind -= nRem;

	IHXBuffer* pNewBuffer = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
					    m_pRollbackBuffer+m_ulDeviceBufferSize-ulNumBytesToRewind,
					    ulNumBytesToRewind,
					    m_pContext))
	{
	    m_pWriteList->AddHead(pNewBuffer);
	}

	// Update total pause bytes offset for time/video sync
	m_ulPauseBytes += pause_bytes;

	return RA_AOE_NOERR;
}

HX_RESULT CAudioOutQNX::_Imp_Resume()
{
	m_paused = FALSE;
	_Imp_Write( NULL );
	return RA_AOE_NOERR;
}

HX_RESULT CAudioOutQNX::_Imp_Reset()
{

	m_ulLastTimeChecked = (UINT32) -1;
	m_ulLastTimeReturned = 0;

	if ( m_wState == RA_AOS_CLOSED )
		return RA_AOE_NOERR;

	if ( m_wID < 0 )
		return RA_AOE_DEVNOTOPEN;

	// Temporary FLUSH <--> DRAIN
	if ( ioctl (m_wID, SND_PCM_IOCTL_DRAIN_PLAYBACK, 0) == -1 )
		return RA_AOE_GENERAL;

	while (m_pWriteList && m_pWriteList->GetCount() > 0)
	{
	    IHXBuffer* pBuffer = (IHXBuffer *)(m_pWriteList->RemoveHead());
	    pBuffer->Release();
	}

	m_ulTotalWritten    = 0;
	m_bFirstWrite	    = TRUE;
	m_ulLastNumBytes    = 0;

	return RA_AOE_NOERR;
}

HX_RESULT CAudioOutQNX::_Imp_Drain()
{
	if ( m_wID < 0 )
		return RA_AOE_DEVNOTOPEN;

	// Temporary FLUSH <--> DRAIN
	if ( ioctl (m_wID, SND_PCM_IOCTL_FLUSH_PLAYBACK, 0) == -1 )
		return RA_AOE_GENERAL;

	return RA_AOE_NOERR;
}

AUDIOERROR CAudioOutQNX::SetDeviceConfig
( 
	const HXAudioFormat* pFormat 
)
{
	if ( m_wID < 0 ) 
		return RA_AOE_NOTENABLED;

	int sampleWidth      = pFormat->uBitsPerSample;
	ULONG32 sampleRate   = pFormat->ulSamplesPerSec;
	int numChannels      = pFormat->uChannels;
	m_wBlockSize         = m_ulBytesPerGran;  //pFormat->uMaxBlockSize;

	ULONG32 bufSize = 128;
	ULONG32 bytesPerBlock = m_wBlockSize; 
	while ( bufSize < 4096 )
	{
		bufSize *= 2;
	}

	m_ulFragSize = bufSize;

	snd_pcm_playback_params_t playback_params;
	memset( &playback_params, 0, sizeof(playback_params) );
	playback_params.fragment_size  = m_ulFragSize;
	playback_params.fragments_max  = -1;
	playback_params.fragments_room =  1;

	/* it's okay to fail, card may not handle fragment size */
	ioctl(m_wID, SND_PCM_IOCTL_PLAYBACK_PARAMS, &playback_params );

	snd_pcm_format_t format;
	memset( &format, 0, sizeof(format));

	if( sampleWidth == 16 )
	{
	    format.format = SND_PCM_SFMT_S16_LE;
	}
	else 
	{
	    format.format = SND_PCM_SFMT_U8;
	    m_uSampFrameSize /= 2;
	}

	format.channels		= numChannels;
	format.rate			= sampleRate;

	m_sample_rate		= sampleRate;
	m_num_channels		= numChannels;
	m_uSampFrameSize	= sampleWidth / 8;

	if(ioctl(m_wID, SND_PCM_IOCTL_PLAYBACK_FORMAT, &format) == -1)
	{
	    return (  m_wLastError = RA_AOE_NOTENABLED );
	}

	numChannels = format.channels;
	sampleRate  = format.rate;

	//
	// Verify that requested format was accepted by the audio device.
	//
	if ( numChannels != pFormat->uChannels )
		((HXAudioFormat*)pFormat)->uChannels = numChannels;
		
	if ( sampleRate != pFormat->ulSamplesPerSec )
		((HXAudioFormat*)pFormat)->ulSamplesPerSec = sampleRate;

	// Get the audio driver's buffer size for our rollback buffer
    snd_pcm_playback_info_t pinfo;
	memset( &pinfo, 0, sizeof( pinfo ) );

	if(ioctl(m_wID, SND_PCM_IOCTL_PLAYBACK_INFO, &pinfo) != -1)
		m_ulDeviceBufferSize = pinfo.buffer_size;	

	return RA_AOE_NOERR;
}

HX_RESULT CAudioOutQNX::_Imp_CheckFormat
( 
	const HXAudioFormat* pFormat 
)
{

	// QNX audio driver can do all formats that we our
	// currently interested in. However, we should check
	// for valid inputs.
	if ( pFormat->uChannels != 1 && pFormat->uChannels != 2 )
		return HXR_FAILED;
	if ( pFormat->uBitsPerSample != 8 && pFormat->uBitsPerSample != 16 )
		return HXR_FAILED;

	// Ask driver later
#if 0
  /*No reason why the driver won't accept other sampling rates...*/
	if ( pFormat->ulSamplesPerSec != 8000 && pFormat->ulSamplesPerSec != 9600  &&
	     pFormat->ulSamplesPerSec != 11025 && pFormat->ulSamplesPerSec != 16000  &&
	     pFormat->ulSamplesPerSec != 18900 && pFormat->ulSamplesPerSec != 22050  &&
	     pFormat->ulSamplesPerSec != 32000 && pFormat->ulSamplesPerSec != 44100  &&
	     pFormat->ulSamplesPerSec != 48000 )
		return HXR_FAILED;
#endif

	return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CAudioOutQNX::_Imp_GetCurrentTime
 *      Purpose:
 *              Get the current time from the audio device.
 *		We added this to support the clock available in the
 *		Window's audio driver.
 */
HX_RESULT CAudioOutQNX::_Imp_GetCurrentTime ( ULONG32& ulCurrentTime )
{

    ULONG32 ulTime   = 0;
    ULONG32 ulBytes  = GetPlaybackBytes();

	ulBytes += m_ulPauseBytes;
    
    ulTime = (UINT32)((  (double)(ulBytes/m_uSampFrameSize)/(double)m_sample_rate) * 1000 / m_num_channels);
    
    //Not used anywhere but belongs to CHXAudioDevice so we must set it.
    m_ulCurrentTime  = ulTime;

    //Set the answer.
    ulCurrentTime = ulTime;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CAudioOutQNX::_Imp_GetAudioFd
 *      Purpose:
 */
INT16 CAudioOutQNX::_Imp_GetAudioFd()
{
    return m_wID;
}

/************************************************************************
 *  Method:
 *              CAudioOutQNX::DoTimeSyncs
 *      Purpose:
 *		Manual time syncs! Fork!
 */
void CAudioOutQNX::DoTimeSyncs()
{
    ReschedPlaybackCheck();
	OnTimeSync();

    return;
}

/************************************************************************
 *  Method:
 *              CAudioOutQNX::GetPlaybackBytes
 *      Purpose:
 *		Get the number of bytes played since last open() was
 *		called. This ioctl() returns funky values sometimes?!@%
 */
ULONG32 CAudioOutQNX::GetPlaybackBytes()
{

	snd_pcm_playback_status_t info;
	memset( &info,0, sizeof(info));

	if ( ioctl (m_wID, SND_PCM_IOCTL_PLAYBACK_STATUS, &info) != -1 )
		return info.scount;

	// If ioctl failed, just guess the value
    int bytes = m_ulTotalWritten - m_ulGranularity / 2;
    if (bytes < 0) 
		bytes = 0;

	return (ULONG32) bytes; 
}

ULONG32 CAudioOutQNX::_GetPlaybackBuffer( )
{
	return( m_ulTotalWritten - GetPlaybackBytes( ) );
}

/************************************************************************
 *  Method:
 *              CAudioOutQNX::ReschedPlaybackCheck()
 *      Purpose:
 *		Reschedule playback callback.
 */
HX_RESULT CAudioOutQNX::ReschedPlaybackCheck()
{
    HX_RESULT theErr = HXR_OK;
    if (m_bCallbackPending)
		return theErr;
    /* Put this back in the scheduler.
     */
    HXPlaybackCountCb* pCallback = 0;
    pCallback = new HXPlaybackCountCb;
    if (pCallback)
    {
        *m_pPlaybackCountCBTime += (int) (1000 * m_ulGranularity);
        pCallback->m_pAudioDeviceObject = this;
        m_bCallbackPending = TRUE;
        m_PendingCallbackID = m_pScheduler->AbsoluteEnter(pCallback,
                        *((HXTimeval*) m_pPlaybackCountCBTime));
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

UINT16	CAudioOutQNX::_NumberOfBlocksRemainingToPlay(void)
{
    UINT32 			bytesBuffered = 0;
    LISTPOSITION	i = m_pWriteList->GetHeadPosition();

    while (i)
    {
		bytesBuffered += ((IHXBuffer *)m_pWriteList->GetAt(i)) -> GetSize();
		m_pWriteList->GetNext(i);
    }
    
    bytesBuffered += (m_ulTotalWritten - GetPlaybackBytes());

    return bytesBuffered / m_wBlockSize + 1;
}


// CAudioOutQNX::HXPlaybackCountCb

CAudioOutQNX::HXPlaybackCountCb::HXPlaybackCountCb(HXBOOL timed) :
     m_lRefCount (0)
    ,m_pAudioDeviceObject (0)
    ,m_timed(timed)
{
}

CAudioOutQNX::HXPlaybackCountCb::~HXPlaybackCountCb()
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
STDMETHODIMP CAudioOutQNX::HXPlaybackCountCb::QueryInterface
(	REFIID riid, void** ppvObj )
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
STDMETHODIMP_(ULONG32) CAudioOutQNX::HXPlaybackCountCb::AddRef()
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
STDMETHODIMP_(ULONG32) CAudioOutQNX::HXPlaybackCountCb::Release()
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
STDMETHODIMP CAudioOutQNX::HXPlaybackCountCb::Func(void)
{
    if (m_pAudioDeviceObject)
    {
	if(!m_timed)
	{
	    m_pAudioDeviceObject->_Imp_Write(NULL);
	}
	else
	{
	    m_pAudioDeviceObject->m_bCallbackPending  = FALSE;
	    m_pAudioDeviceObject->_Imp_Write(NULL);
	    m_pAudioDeviceObject->DoTimeSyncs();
	}
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CAudioOutQNX::BuffersEmpty
 *      Purpose:
 */
HXBOOL CAudioOutQNX::BuffersEmpty()
{
    snd_pcm_playback_status_t info;

    if ( -1 == ioctl(m_wID, SND_PCM_IOCTL_PLAYBACK_STATUS, &info) )
		return FALSE;

    if ( info.queue )
		return FALSE;

    return TRUE;
}



void CAudioOutQNX::OpenMixer()
{
	int								i;
	struct snd_mixer_info_t 		info;
	struct snd_mixer_channel_info_t cinfo;

    //
    // return if the mixer is already opened
    //
    if (m_bMixerPresent)
		return;

    mixm_wID = open ( m_DevCtlName, O_RDWR );

	if ( -1 == ioctl( mixm_wID, SND_MIXER_IOCTL_INFO, &info ) )
	{
		CloseMixer( );
		return;
	}
    
    if (mixm_wID > 0)
    {
		/* find pcm channel */
		memset( &cinfo, 0, sizeof( cinfo ) );
		for ( i = 0; i < info.channels; i++ )
		{
			cinfo.channel = i;
			if ( -1 == ioctl( mixm_wID, SND_MIXER_IOCTL_CHANNEL_INFO, &cinfo ) )
			{
				continue;
			}
			if ( 0 == strcmp( (char *)cinfo.name, SND_MIXER_ID_PCM ) ) 
			{
				m_wPCMChannel = i;
				break;
			}
		}

	}

	if ( m_wPCMChannel > 0 )
	{
		m_bMixerPresent = 1;
    }
    else
    {
		CloseMixer( );
		m_bMixerPresent = 0;
    }
}


void CAudioOutQNX::CloseMixer()
{
    // Close the mixer device.
    if ( mixm_wID >= 0 ) 
    {
		close ( mixm_wID );
		mixm_wID = -1;
    }
}
