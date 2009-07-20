/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audaix.cpp,v 1.5 2006/02/07 19:33:51 ping Exp $
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

/*******************************************************************
 *	audaix.cpp
 *
 *	CLASS: CAudioOutAIX
 *	
 *	DESCRIPTION: AIX & UMS specific audio class implementation
 *   
 *******************************************************************/

#include <signal.h>		// for  getenv()

#include <stdio.h>
#include <stdlib.h>		// for  getenv()
#include <math.h>
#include <sys/types.h>
#include <stropts.h>		// for I_FLUSH ioctl
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/audio.h>

#include "hxcom.h"

#include "hxresult.h"
#include "hxengin.h"
#include "ihxpckts.h"   // for IHXBuffer 
#include "pckunpck.h"
#include "hxslist.h"

#include "timeval.h"

#include "audaix.h"

#include "hxaudses.h"
#include "hxtick.h"
#include "chxpckts.h"

#include "debug.h"
#include "hxstrutl.h"

#include <UMSBAUDDevice.xh>
#include <UMSAudioDevice.xh>
#include <UMSAudioTypes.xh>

struct IHXCallback;  // forward declaration needed for callback.

// One and ONLY one of the following must be uncommented.  They determine the
// strategy used to determine the elaped time. The first is the preferred.
#define AIX_TIME_BYTES_WRITTEN         // function of bytes written vrs bytes in buffer.


//**********************************************************************
//  constants.
//

// in UMS, balance is really an initial pan setting. 
// -100 = hard left, 0 is centered, 100 = hard right

static const LONG32   lDefaultBalance = 0;   
static const LONG32   lDefaultVolume = 50;   // UMS range is 0..100
static LONG32   zlCurrentVolume = 50;   // UMS range is 0..100
// this is sufficient for PCI devices, for microchannel devices, set the
// envar AUDIODEV to "/dev/maud0".
static const char *   szDefaultPortFilename = "/dev/paud0";
static const char *   moduleName = "CAudioOutAIX"; 

// this will point to the SOM environment needed by the UMS subsystem
static Environment*   gpSomEnvironment                  = NULL;


// static local routines, used to translate UMS message code to RMA types.
static const char *   getUMSAudioDeviceError( UMSAudioDevice_ReturnCode );
static audio_error    UMSErrorCodeToRACode( UMSAudioDevice_ReturnCode ); 

CAudioOutAIX::CAudioOutAIX() 
  : mixm_wID( -1 ),
    m_wState( RA_AOS_CLOSED ),
    m_wLastError( RA_AOE_NOERR ),
    m_bMixerPresent(FALSE),
    m_wBlockSize(0),
    m_ulLastNumBytes (0),
    m_ulBytesRemaining(0),
    m_ulTotalWritten(0),
    m_bFirstWrite (TRUE),
    m_pPlaybackCountCBTime(NULL),
    m_PendingCallbackID (0),
    m_bCallbackPending(FALSE),
    m_paused(FALSE),
    m_pWriteList(NULL),
    m_lLeftGain(100),
    m_lRightGain(100)
{
  
    // set up UMS environment.
    gpSomEnvironment = somGetGlobalEnvironment();
    HX_ASSERT( gpSomEnvironment );
    m_pAudioDevice   = new UMSBAUDDevice();
    
    // Get AUDIODEV environment var to find audio device of choice
    char *adev = (char*)getenv("AUDIODEV"); /* Flawfinder: ignore */
    char *mdev = (char*)getenv("MIXERDEV"); /* Flawfinder: ignore */
    // Use defaults if no environment variable is set.
    m_DevName[26]    = NULL;
    m_DevCtlName[26] = NULL;

    if (adev && strlen(adev) > 0)
        SafeStrCpy(m_DevName, adev, 26);
    else
        SafeStrCpy(m_DevName, szDefaultPortFilename, 26);

    m_pPlaybackCountCBTime = new Timeval;

    if (mdev && strlen(mdev) > 0)
        SafeStrCpy(m_DevCtlName, mdev, 26);
    else
        SafeStrCpy(m_DevCtlName, szDefaultPortFilename, 26);
    
    m_pWriteList = new CHXSimpleList();

    // now configure our device.
    m_pAudioDevice->set_audio_format_type  ( gpSomEnvironment, "PCM" );
    m_pAudioDevice->set_number_format      ( gpSomEnvironment, 
					     "TWOS_COMPLEMENT" );
    m_pAudioDevice->set_byte_order         ( gpSomEnvironment, "MSB" ); 
    m_pAudioDevice->set_time_format        ( gpSomEnvironment, 
					     UMSAudioTypes_Msecs ); 
    
    m_pAudioDevice->set_balance   ( gpSomEnvironment, lDefaultBalance );  
    m_pAudioDevice->set_volume    ( gpSomEnvironment, zlCurrentVolume );
    m_pAudioDevice->enable_output ( gpSomEnvironment, "LINE_OUT", 
				    &m_lLeftGain, &m_lRightGain  );
}


CAudioOutAIX::~CAudioOutAIX()
{
    if ( m_wState != RA_AOS_CLOSED ) 
      {
	_Imp_Close();
	m_wState = RA_AOS_CLOSED;
      }
    
    mixm_wID = -1;
    m_wState =  RA_AOS_CLOSED;
    m_wLastError = RA_AOE_NOERR;
    m_bMixerPresent = FALSE;
    m_wBlockSize       = 0;
    m_ulLastNumBytes   = 0;
    m_ulBytesRemaining = 0;
    m_ulTotalWritten   = 0;
    
    if (m_pScheduler) 
      m_pScheduler->Release();
    
    m_PendingCallbackID = 0;
    m_bCallbackPending = TRUE; 
    
    while(!m_pWriteList->IsEmpty())
      {
	IHXBuffer* pBuffer = (IHXBuffer*)m_pWriteList->RemoveHead();
	HX_RELEASE(pBuffer);
      }

    HX_DELETE(m_pWriteList);
    
    delete m_pAudioDevice;
}


/*
 *    I do not open /dev/mixer to control volume, the volume is scaled
 *    by the UMS device.
 *    The device's volume has a range of 0..100.
 */
UINT16 CAudioOutAIX::_Imp_GetVolume()
{
    long volume;
    m_pAudioDevice->get_volume( gpSomEnvironment, &volume );
    return zlCurrentVolume;
    return (UINT16)volume;
}


HX_RESULT CAudioOutAIX::_Imp_SetVolume( const UINT16 uVolume )
{
    m_pAudioDevice->set_volume( gpSomEnvironment, (long)uVolume );
    zlCurrentVolume = uVolume;
    return RA_AOE_NOERR;
}


/*
 *  All UMS audio devices support volume.
 */
HXBOOL CAudioOutAIX::_Imp_SupportsVolume()
{
    return TRUE;
}

HX_RESULT CAudioOutAIX:: _Imp_Open( const HXAudioFormat* pFormatSupplied )
{
    if( pFormatSupplied != NULL )
      m_pAudioFormat = pFormatSupplied;
    
    HX_ASSERT( m_pAudioFormat != NULL ); 
    
    // Get the core scheduler interface; Use this to schedule polling
    // the audio device for number of bytes played.
    if ( m_pContext )
    {
	m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler );
    }

    // Check state. Could already be opened.
    if ( m_wState == RA_AOS_OPEN_PAUSED  || 
         m_wState == RA_AOS_OPEN_PLAYING ||
         m_wState == RA_AOS_OPENING )
      return RA_AOE_NOERR;
    
    m_ulBlocksProcessed = 0;
    UMSAudioDevice_ReturnCode retCode =  
      m_pAudioDevice->open( gpSomEnvironment, m_DevName, "PLAY", 0 ); 
    
    if( retCode != UMSAudioDevice_Success ) 
      {
        return HXR_FAILED;
      }
    
    m_wBlockSize = m_ulBytesPerGran;  //m_pAudioFormat->uMaxBlockSize;
    m_uSampFrameSize = m_pAudioFormat->uBitsPerSample / 8;
    
    // Set device state
    m_wState = RA_AOS_OPENING;
    
    // Configure the audio device.
    AUDIOERROR iVal = SetDeviceConfig( m_pAudioFormat );
    
    return RA_AOE_NOERR;
}



HX_RESULT CAudioOutAIX::_Imp_Close()
{
    m_wState = RA_AOS_CLOSED;
    
    // this will force the player to play all remaining data.  If passed TRUE, 
    // the call will block until finished playing. Do we want to call this?
    // m_pAudioDevice->play_remaining_data( gpSomEnvironment, TRUE );
    _Imp_Write(NULL);
    
    m_pAudioDevice->stop( gpSomEnvironment );
    m_pAudioDevice->close( gpSomEnvironment );
    
    // Remove callback from scheduler
    if (m_bCallbackPending)
      {
	m_pScheduler->Remove(m_PendingCallbackID);
	m_bCallbackPending = FALSE;
    }
    
    return RA_AOE_NOERR;
}




HX_RESULT CAudioOutAIX::_Imp_Write( const HXAudioData* pAudioOutHdr )
{
    IHXBuffer* pBuffer = NULL;
    UCHAR* pData = NULL;
    UMSAudioTypes_Buffer adBuffer;
    UINT32* pTimeStamp = NULL;
    
    if ( m_bFirstWrite && pAudioOutHdr)
    {
        /*  Initialize the playback callback time. */
        HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
        m_pPlaybackCountCBTime->tv_sec = lTime.tv_sec;
        m_pPlaybackCountCBTime->tv_usec = lTime.tv_usec;

	m_bFirstWrite = FALSE;
	ReschedPlaybackCheck();
    }

    if (pAudioOutHdr)
      {
        ++m_ulBlocksProcessed ; 
	
	DPRINTF(D_INFO, ("_Imp_Write: buf len in milli-seconds: %lu\n",
			 ((ULONG32) (( 1000.0
				       / (m_num_channels * m_uSampFrameSize * m_sample_rate))
				     *  pAudioOutHdr->pData->GetSize()))));
      }
    
    // If we are paused, just add the block to the end of the WriteList.
    if (m_paused)
    {
	if (!pAudioOutHdr)
	    return m_wLastError = RA_AOE_NOERR;
	
	IHXBuffer* pNewBuffer = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer, 
					    pAudioOutHdr->pData->GetBuffer(),
					    pAudioOutHdr->pData->GetSize(),
					    m_pContext))
	{
	    m_pWriteList->AddTail(pNewBuffer);
	}
	
	return  RA_AOE_NOERR;
    }
 
    HXBOOL bWroteSomething;
    do {
        bWroteSomething = FALSE;
	if(m_pWriteList->GetCount() <= 0)
	{
	    if(!pAudioOutHdr)
		return  RA_AOE_NOERR;
	    
	    pData = (UCHAR*)pAudioOutHdr->pData->GetBuffer();
	    adBuffer._length = pAudioOutHdr->pData->GetSize();
	}
	else
	{
	    if (pAudioOutHdr)
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
	    HX_ASSERT(pBuffer);
	    pData = pBuffer->GetBuffer();
	    adBuffer._length  = pBuffer->GetSize();
	}
    
        // Write audio data to device.
        long	count = 0;
        int         wrote;
    
        adBuffer._buffer = pData;
        adBuffer._maximum = adBuffer._length;
    
        int     nBytesPerSample = (m_bits_per_sample/8)* m_num_channels;
        long    lSamplesToWrite = adBuffer._length / nBytesPerSample;
        long    lSamplesWritten =0;    
    
        int     transferredCount = adBuffer._length;

        m_wLastError = RA_AOE_NOERR;  // less work this way.

        UMSAudioDevice_ReturnCode retCode = 
            m_pAudioDevice->write( gpSomEnvironment, 
				 &adBuffer,
				 lSamplesToWrite,
				 &lSamplesWritten );

        switch(retCode)
        {
            case UMSAudioDevice_Success :    
	    {
	        int nBytesWritten = lSamplesWritten * nBytesPerSample;
	        m_ulTotalWritten += nBytesWritten;
	      
  	  	HX_ASSERT(lSamplesWritten <= lSamplesToWrite);
	        if( lSamplesWritten == lSamplesToWrite ) 
	        {
  		    bWroteSomething = TRUE;
	        }
	        else 
                {  
	    	    /* requeue the balance. */
		    long lSamplesToRequeue = lSamplesToWrite - lSamplesWritten;
		    IHXBuffer* pNewBuffer = NULL;
		    if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
                                                        adBuffer._buffer + nBytesWritten,
							adBuffer._length - nBytesWritten,
							m_pContext))
		    {
			m_pWriteList->AddHead(pNewBuffer);
		    }
	        }
	    }
	    break;
	    
	    case UMSAudioDevice_DeviceError : // indicates EWOULDBLOCK
	    case UMSAudioDevice_Preempted :
	    case UMSAudioDevice_Interrupted :
            {
	        // just requeue the data
	        IHXBuffer* pNewBuffer = NULL;
		if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
                                                    adBuffer._buffer,
						    adBuffer._length,
						    m_pContext))
		{
		    m_pWriteList->AddHead(pNewBuffer);
		}
	        break;
            }
	    
	    default :  // failure!
	        HX_ASSERT(FALSE);
	        m_wLastError = UMSErrorCodeToRACode( retCode );
        }  
    
       INT32 newRefCount = 0;
       if( pBuffer ) 
           newRefCount = pBuffer->Release();

	HX_ASSERT(newRefCount == 0);
	pBuffer = NULL;
       pAudioOutHdr = NULL; // don't add the same buffer again...
    } while (bWroteSomething);
    
    return m_wLastError;
}


// Seek() is never called.
HX_RESULT CAudioOutAIX::_Imp_Seek(ULONG32 ulSeekTime)
{
    return HXR_OK;
}


HX_RESULT CAudioOutAIX::_Imp_Pause()
{
    m_paused = TRUE;
    
    return RA_AOE_NOERR;
}

HX_RESULT CAudioOutAIX::_Imp_Resume()
{
    m_paused = FALSE;
    return RA_AOE_NOERR;
}

/////////////////////////////////////////////////////////////////////////
// Imp_Reset()
//    Reset the device.  It is important to note that this call results 
//    from a seek, so we must act accordingly.
//
HX_RESULT CAudioOutAIX::_Imp_Reset()
{
    _Imp_Close();
    _Imp_Open(NULL);
    while(!m_pWriteList->IsEmpty())
      {
	IHXBuffer* pBuffer = (IHXBuffer*)m_pWriteList->RemoveHead();
	HX_RELEASE(pBuffer);
      }
    HX_ASSERT( m_pWriteList->IsEmpty() );
    // these are vital! If they are not set this way, playback after a seek 
    // will just stop after a few frames.
    m_bPaused       = FALSE;
    m_bFirstWrite   = TRUE;
    // and these are fairly obvious, but necessary after a seek. It should be
    // noted here that the audio timeline after a seek is relative to the 
    // internal timeline of the core scheduler; all our playback is based from 
    // our start.
    m_ulTotalWritten =0;
    m_ulCurrentTime = 0;
    
    return RA_AOE_NOERR;
}


// this will force the player to play all remaining data.  The call
// will block until finished playing - if this is not what is desired,
// change the second parameter of the call to FALSE. 
HX_RESULT CAudioOutAIX::_Imp_Drain()
{
    // -pjg need to drain the WriteList also.
    // this will force the player to play all remaining data.  If passed TRUE, 
    // the call will block until finished playing. 

    while(!m_pWriteList->IsEmpty())
      {
	IHXBuffer* pBuffer = (IHXBuffer*)m_pWriteList->RemoveHead();
	HX_RELEASE(pBuffer);
      }

    m_pAudioDevice->play_remaining_data( gpSomEnvironment, TRUE );
    
    return RA_AOE_NOERR;
}




AUDIOERROR CAudioOutAIX::SetDeviceConfig( const HXAudioFormat* pFormat )
{
    m_pAudioDevice->set_sample_rate        ( gpSomEnvironment, 
					     pFormat->ulSamplesPerSec, 
					     &m_oSamples );
    m_pAudioDevice->set_number_of_channels ( gpSomEnvironment, 
					     pFormat->uChannels );
    m_pAudioDevice->set_bits_per_sample    ( gpSomEnvironment, 
					     pFormat->uBitsPerSample );
    m_pAudioDevice->set_audio_format_type  ( gpSomEnvironment, "PCM" );
    m_pAudioDevice->set_number_format      ( gpSomEnvironment, 
					     "TWOS_COMPLEMENT" );
    m_pAudioDevice->set_byte_order         ( gpSomEnvironment, "MSB" ); 
    m_pAudioDevice->set_time_format        ( gpSomEnvironment, 
					     UMSAudioTypes_Msecs ); 
    
    m_pAudioDevice->set_balance   ( gpSomEnvironment, lDefaultBalance );  
    m_pAudioDevice->set_volume    ( gpSomEnvironment, zlCurrentVolume );
    m_pAudioDevice->enable_output ( gpSomEnvironment, "LINE_OUT", 
				    &m_lLeftGain, &m_lRightGain  );
    
    m_pAudioDevice->initialize    ( gpSomEnvironment );
    
    m_pAudioDevice->start         ( gpSomEnvironment );
    
    m_num_channels = pFormat->uChannels;
    m_sample_rate  = pFormat->ulSamplesPerSec; 
    m_bits_per_sample = pFormat->uBitsPerSample;
    
    return RA_AOE_NOERR;
}


HX_RESULT CAudioOutAIX::_Imp_CheckFormat( const HXAudioFormat* pFormat )
{
    // Check for valid format inputs.
    if ( pFormat->uChannels != 1 && pFormat->uChannels != 2 ) 
      {
	return HXR_FAIL;
      }
    if ( pFormat->uBitsPerSample != 8 && pFormat->uBitsPerSample != 16 ) 
      {
	return HXR_FAIL;
      }
    
    return HXR_OK;
}



/************************************************************************
 *  Method:
 *              CAudioOutAIX::_Imp_GetCurrentTime
 *      Purpose:
 *              Get the current time from the audio device.
 *		We added this to support the clock available in the
 *		Window's audio driver.
 *  
 *              Current time is simply a function of the total number of 
 *              bytes written to the device minus the number still in
 *              the device queue.  This is expressed as playbackbytes.
 *
 *              Ported to Aix 4.3.1 by fhutchinson, Jan 1999.
 *
 *   The minitimer below is a simple timer employed to avoid excessive 
 *   calls to the UMS audio driver to get the current time, the rationale 
 *   being that a gettimeofday call and a little arithmetic is less expensive 
 *   than a call to the driver. The threshold value is the number of 
 *   microseconds (1/1,000,000 sec) that must pass before the time goes off.
 */    

#include <string.h>  // for strerror()

HX_RESULT CAudioOutAIX::_Imp_GetCurrentTime( ULONG32& ulCurrentTime )
{
    long lBytesPerSample = m_bits_per_sample/8;
    long lBytesStillInDeviceBuffer;
    ulCurrentTime = m_ulCurrentTime;

    UMSAudioDevice_ReturnCode retCode;
    
    // set the device to return the amount of data still in the device buffer as a
    // number of bytes
    retCode = m_pAudioDevice->set_time_format( gpSomEnvironment, UMSAudioTypes_Bytes );
    if( retCode != UMSAudioDevice_Success ) 
    {
	m_wLastError = UMSErrorCodeToRACode( retCode );
	return (m_wLastError = UMSErrorCodeToRACode( retCode ) );
    }
    
    retCode = m_pAudioDevice->write_buff_used( gpSomEnvironment, &lBytesStillInDeviceBuffer );
    if( retCode != UMSAudioDevice_Success ) 
    {
	// almost certainly because the device is not opened.
	return (m_wLastError = UMSErrorCodeToRACode( retCode ) );
    }
    
    long lBytesPlayed =  m_ulTotalWritten - lBytesStillInDeviceBuffer;
    
    m_ulCurrentTime = ulCurrentTime = 
      ( (lBytesPlayed /lBytesPerSample) / 
	(float)m_sample_rate * 1000 / m_num_channels );
   
    return HXR_OK;
}




/************************************************************************
 *  Method:
 *              CAudioOutAIX::_Imp_GetAudioFd
 *      Purpose: Intended to return the file descriptor of the device. 
 *               However, UMS is not file descriptor-based, so we 
 *               return a -1, generally accepted as a closed device fd.
 */
INT16 CAudioOutAIX::_Imp_GetAudioFd()
{
    return -1;
}


/************************************************************************
 *  Method:
 *              CAudioOutAIX::DoTimeSyncs
 *      Purpose:
 *		Manual time syncs! Fork!
 */
void CAudioOutAIX::DoTimeSyncs()
{
    ReschedPlaybackCheck();
    OnTimeSync(); // XXXDMB      // hxaudev.cpp::CHXAudioDevice::OnTimeSync()
    
    return;
}

/************************************************************************
 *  Method:
 *              CAudioOutAIX::ReschedPlaybackCheck()
 *      Purpose:
 *		Reschedule playback callback.
 */
HX_RESULT CAudioOutAIX::ReschedPlaybackCheck()
{
    HX_RESULT theErr = HXR_OK;
    
    if (m_bCallbackPending)
      return theErr;

    *m_pPlaybackCountCBTime += (int) (m_ulGranularity*1000) / 2;
    // Put this back in the scheduler.
    HXPlaybackCountCb* pCallback = 0;
    pCallback = new HXPlaybackCountCb(TRUE);
    if (pCallback)
      {
	pCallback->m_pAudioDeviceObject = this;
	m_bCallbackPending = TRUE;
	m_PendingCallbackID = 
	  m_pScheduler->AbsoluteEnter(pCallback,*((HXTimeval*)m_pPlaybackCountCBTime)) ;
      }
    else
      theErr = HXR_OUTOFMEMORY;  // but ignored, why?
    
    return theErr;
}



UINT16	CAudioOutAIX::_NumberOfBlocksRemainingToPlay(void)
{
    ULONG32 ulCurTime = 0;
    GetCurrentAudioTime(ulCurTime);
    
    UINT32 bytesBuffered = 0;
    LISTPOSITION i = m_pWriteList->GetHeadPosition();
    
    while (i)
    {
	bytesBuffered += ((IHXBuffer *)m_pWriteList->GetAt(i)) -> GetSize();
	m_pWriteList->GetNext(i);
    }


    m_pAudioDevice->set_time_format( gpSomEnvironment, UMSAudioTypes_Bytes );

    long lBytesStillInDeviceBuffer;
    UINT32 tries = 100;
    UMSAudioDevice_ReturnCode retCode;

    while (--tries &&
           (m_pAudioDevice->write_buff_used( gpSomEnvironment, &lBytesStillInDeviceBuffer )) != UMSAudioDevice_Success);

    if (!tries)
    {
	HX_ASSERT(FALSE);
	// don't know what to do!
    }

    UINT16 blocks = 
	(bytesBuffered + lBytesStillInDeviceBuffer) / m_wBlockSize + 1;
    //UINT16 blocks = (int) (((double)bytesBuffered + lBytesStillInDeviceBuffer) /
//			     m_ulBytesPerGran) + 1;

    return blocks;
}


// CAudioOutAIX::HXPlaybackCountCb
CAudioOutAIX::HXPlaybackCountCb::HXPlaybackCountCb(HXBOOL timed) 
  : m_lRefCount (0),
    m_pAudioDeviceObject (0),
    m_timed(timed)
{
}

CAudioOutAIX::HXPlaybackCountCb::~HXPlaybackCountCb()
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
STDMETHODIMP CAudioOutAIX::HXPlaybackCountCb::QueryInterface(REFIID riid, void** ppvObj)
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
STDMETHODIMP_(ULONG32) CAudioOutAIX::HXPlaybackCountCb::AddRef()
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
STDMETHODIMP_(ULONG32) CAudioOutAIX::HXPlaybackCountCb::Release()
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
STDMETHODIMP CAudioOutAIX::HXPlaybackCountCb::Func(void)
{
    if (m_pAudioDeviceObject)
    {
	if(!m_timed)
	{
	    m_pAudioDeviceObject->_Imp_Write(NULL);
	}
	else
	{
	    m_pAudioDeviceObject->_Imp_Write(NULL);
	    m_pAudioDeviceObject->m_bCallbackPending  = FALSE;
	    m_pAudioDeviceObject->DoTimeSyncs();
	}
    }

    return HXR_OK;
}

//
//   getUMSAudioDeviceError()
//  
//  returns the string description of the given UMS error code
//

static const char * getUMSAudioDeviceError( UMSAudioDevice_ReturnCode code ) 
{
  switch( code )
    {
    case UMSAudioDevice_Success        : return "Success";
    case UMSAudioDevice_InvalidParam   : return "Invalid parameter";
    case UMSAudioDevice_MemoryError    : return "Success";
    case UMSAudioDevice_DeviceNotAvail : return "Device is not available";
    case UMSAudioDevice_Preempted      : return "Preempted";
    case UMSAudioDevice_Interrupted    : return "Inerrupted";
    case UMSAudioDevice_DeviceError    : return "Device error";
      
      // make sure this is last before default case.
    case UMSAudioDevice_Failure : return "undescribed error";
    default : break;
      
    }
  return "Unknown error";
}


//
//  UMSErrorCodeToRACode()
// 
//  Converts the UMS return code to a best-guess RMA return code equivalent.
//
audio_error UMSErrorCodeToRACode( UMSAudioDevice_ReturnCode retCode ) 
{
  switch( retCode )
    {
    case UMSAudioDevice_Success               : return RA_AOE_NOERR;
    case UMSAudioDevice_InvalidParam          : return RA_AOE_INVALPARAM;
    case UMSAudioDevice_DeviceNotAvail        : return RA_AOE_BADDEVICEID;
    case UMSAudioDevice_NoDevice              : return RA_AOE_BADDEVICEID; 
    case UMSAudioDevice_IncompatibleSettings  : return RA_AOE_BADFORMAT  ; 
    case UMSAudioDevice_NotOpen               : return RA_AOE_DEVNOTOPEN  ; 
    case UMSAudioDevice_NotReady              : return RA_AOE_DEVBUSY  ; 
    case UMSAudioDevice_SettingsChanged       : return RA_AOE_BADFORMAT  ; 
      // this is a stretch...
    case UMSAudioDevice_DeviceError           : return RA_AOE_BADWRITE  ; 
    case UMSAudioDevice_NotSupported          : return RA_AOE_NOTSUPPORTED  ; 

      // and these we will classify under general for now.
    case UMSAudioDevice_Interrupted           : ;
    case UMSAudioDevice_Preempted             : ;
    case UMSAudioDevice_MemoryError           : ;
    case UMSAudioDevice_Failure               : ;
    case UMSAudioDevice_UnderRun              : ;
    case UMSAudioDevice_OverRun               : ;
    default : break;
    }
  return RA_AOE_GENERAL;
}



