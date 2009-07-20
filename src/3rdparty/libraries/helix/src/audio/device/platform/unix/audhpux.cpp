/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audhpux.cpp,v 1.6 2004/09/23 18:20:29 gwright Exp $
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

#include "audhpux.h"
#include "ihxpckts.h"
#include "hxtick.h"
#include "hxprefs.h"
#include "hxstrutl.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <stdio.h> 
#include <math.h> 

#include <sys/audio.h>

#define MAX_DEV_NAME 256

//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
CAudioOutHPUX::CAudioOutHPUX() :
    CAudioOutUNIX(),
    m_ulTickCount(0),
    m_ulLastBytesPlayed(0),
    m_ulLastTimeStamp(0),
    m_ulPausePosition(0),
    m_nDevID(NO_FILE_DESCRIPTOR),
    m_nMixerID(NO_FILE_DESCRIPTOR),
    m_bGetODelayFailed(FALSE),
    m_bGetOSpaceFailed(FALSE),
    m_nLastVolume(0),
    m_nMinVolume(0),
    m_nMaxVolume(0)
{
};

CAudioOutHPUX::~CAudioOutHPUX()
{
    //The mixer is opened independently of the audio device. Make sure 
    //it is celosed.
  _CloseAudio();
};

//-------------------------------------------------------
// These Device Specific methods must be implemented 
// by the platform specific sub-classes.
//-------------------------------------------------------
INT16 CAudioOutHPUX::_Imp_GetAudioFd(void)
{
    return m_nDevID;
}

//Devic specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.

HX_RESULT CAudioOutHPUX::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    if ( !pFormat )
    {
      pFormat = &m_lastFormat;
    }
    else
      {
	m_lastFormat = *pFormat;
      }
    m_lastFormat = *pFormat;
    
    if ( m_nDevID < 0 )
    {
        return RA_AOE_DEVNOTOPEN;
    }
    

    // set output if it's not set, depending on what sound control you
    // have, some older systems set audio to a default of no sound
    int audio_out;
    ioctl(m_nDevID, AUDIO_GET_OUTPUT, &audio_out);
    if (audio_out == AUDIO_OUT_NONE) 
    {
        audio_out = ioctl(m_nDevID, AUDIO_SET_OUTPUT, AUDIO_OUT_INTERNAL);
    }
        
    int nFragSize = 8192;

    while (-1 != ioctl(m_nDevID, AUDIO_SET_TXBUFSIZE, nFragSize))
    {
	nFragSize <<= 1;
    }

    ioctl(m_nDevID, AUDIO_GET_TXBUFSIZE, &m_ulDeviceBufferSize);


    //Now set the format. Either 8-bit or 16-bit audio is supported.
    int      nSampleWidth  = pFormat->uBitsPerSample;
    ULONG32  nSampleRate   = pFormat->ulSamplesPerSec;
    int      numChannels   = pFormat->uChannels;
    int      nFormat1      = 0;
    int      nFormat2      = 0;
    
    if( nSampleWidth == 16)
    {
        nFormat1 = nFormat2 = AUDIO_FORMAT_LINEAR16BIT;
    }
    else
    {
        nFormat1 = nFormat2 = AUDIO_FORMAT_LINEAR8BIT;
    }
    
    if(ioctl(m_nDevID, AUDIO_SET_DATA_FORMAT, nFormat1) == -1)
    {
        return (  m_wLastError = RA_AOE_NOTENABLED );
    }

    m_uSampFrameSize = nSampleWidth/8;

    if ( nSampleWidth != pFormat->uBitsPerSample )
    {
        ((HXAudioFormat*)pFormat)->uBitsPerSample = nSampleWidth;
    }

    //Set number of channels. Stereo or mono.
    if (ioctl(m_nDevID, AUDIO_SET_CHANNELS, numChannels) == -1)
    {
        return ( m_wLastError = RA_AOE_NOTENABLED );
    }
    m_unNumChannels = numChannels;

    if ( numChannels != pFormat->uChannels )
    {
        ((HXAudioFormat*)pFormat)->uChannels = numChannels;
    }

    //Set the sample rate.
    if (ioctl(m_nDevID, AUDIO_SET_SAMPLE_RATE, nSampleRate) == -1)
    {
        return ( m_wLastError = RA_AOE_NOTENABLED );
    }
    m_unSampleRate = nSampleRate;

    if ( nSampleRate != pFormat->ulSamplesPerSec )
    {
        ((HXAudioFormat*)pFormat)->ulSamplesPerSec = nSampleRate;
    }

#ifdef _DEBUG
    fprintf( stderr, "Device Configured:\n");
    fprintf( stderr, "         Sample Rate: %d\n",  m_unSampleRate);
    fprintf( stderr, "        Sample Width: %d\n",  nSampleWidth);
    fprintf( stderr, "        Num channels: %d\n",  m_unNumChannels);
    fprintf( stderr, "          Block size: %d\n",  m_wBlockSize);
    fprintf( stderr, "  Device buffer size: %lu\n", m_ulDeviceBufferSize);
    fprintf( stderr, " Support for old OSS: %d\n",  m_bGetOSpaceFailed);
#endif
    return RA_AOE_NOERR;
}

void CAudioOutHPUX::_SyncUpTimeStamps(ULONG32 lCount)
{
    int bytes2  = 0;
    int theErr = -1;
    audio_status astatus;
    if( !m_bGetODelayFailed )
    {
	theErr = ::ioctl(m_nDevID, AUDIO_GET_STATUS, &astatus);
    }
    if( theErr != -1)
    {
        m_ulLastBytesPlayed = (ULONG32)(m_ulTotalWritten+lCount-astatus.transmit_buffer_count);
        m_ulLastTimeStamp   = GetTickCount();
    }
    else
    {
        //so we don't try it again.
        m_bGetODelayFailed = TRUE;
    }
}


//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written. 
HX_RESULT CAudioOutHPUX::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
{
    HX_RESULT retCode = RA_AOE_NOERR;

    if( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        if( m_ulTickCount == 0 )
            m_ulTickCount = GetTickCount();

        lCount = ::write( m_nDevID, buffer, ulBuffLength);
        if( lCount < 0 )
        {
            //Error occurred.
            if( errno == EAGAIN )
                retCode = RA_AOE_NOERR;
            if( errno == EINTR )
                retCode = RA_AOE_DEVBUSY;
        }
        else
        {
            _SyncUpTimeStamps(lCount);
        }
    }
    return retCode;
}

//Device specific methods to open/close the mixer and audio devices.
HX_RESULT CAudioOutHPUX::_OpenAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    // Open the audio device if it isn't already open
    if ( -1 == m_nDevID )
    {
	
	//Check the environmental variable to let user overide default device.
	char *pszOverrideName = getenv( "AUDIO" ); /* Flawfinder: ignore */
	char szDevName[MAX_DEV_NAME]; /* Flawfinder: ignore */
	
	// Use defaults if no environment variable is set.
	if ( pszOverrideName && strlen(pszOverrideName)>0 )
	{
	    SafeStrCpy( szDevName, pszOverrideName, MAX_DEV_NAME );
	}
	else
	{
	    SafeStrCpy( szDevName, "/dev/audio", MAX_DEV_NAME );
	}
	//Set the tick count to zero
	m_ulTickCount       = 0;
	m_ulLastTimeStamp   = 0;
	m_ulLastBytesPlayed = 0;
	m_ulPausePosition   = 0;
        m_nDevID = ::open( szDevName, O_WRONLY );
    }
    
    if ( m_nDevID < 0 )
    {
#ifdef _DEBUG        
        fprintf( stderr, "Failed to open audio!!!!!!! Code is: %d  errno: %d\n",
                 m_nDevID, errno );
#endif        
        
        //Error opening device.
        retCode = RA_AOE_BADOPEN;
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutHPUX::_CloseAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if( m_nDevID >= 0 )
    {
        ::close( m_nDevID );
        m_nDevID = NO_FILE_DESCRIPTOR;
    }
    else
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutHPUX::_OpenMixer()
{
    if (-1 == m_nDevID)
	_OpenAudio(); // control the level through the audio device
    // get and store the current volume

    struct audio_describe adescribe;
    if (-1 != ioctl(m_nDevID, AUDIO_DESCRIBE, &adescribe)) {
	m_nMinVolume = adescribe.min_transmit_gain;
	m_nMaxVolume = adescribe.max_transmit_gain;
    }
    
    struct audio_gain gainsettings;

    if (-1 != ioctl(m_nDevID, AUDIO_GET_GAINS, &gainsettings))
    {
	m_nLastVolume = (gainsettings.cgain[0].transmit_gain - 
			 m_nMinVolume) * 100 / 
	    (m_nMaxVolume - m_nMinVolume);
    }
    
    m_bMixerPresent=TRUE;
    
    return RA_AOE_NOERR;
}

HX_RESULT CAudioOutHPUX::_CloseMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    m_wLastError = retCode;
    m_bMixerPresent = FALSE;
    return m_wLastError;
}

//Device specific method to reset device and return it to a state that it 
//can accept new sample rates, num channels, etc.
HX_RESULT CAudioOutHPUX::_Reset()
{
    ioctl(m_nDevID, AUDIO_RESET, RESET_TX_BUF);
  
    _CloseAudio();
    UINT32 ulTickCount = m_ulTickCount;
    UINT32 ulLastTimeStamp = m_ulLastTimeStamp;
    UINT32 ulLastBytesPlayed = m_ulLastBytesPlayed;
    UINT32 ulPausePosition = m_ulPausePosition;

    _OpenAudio();

    m_ulTickCount = ulTickCount;
    m_ulLastTimeStamp = ulLastTimeStamp;
    m_ulLastBytesPlayed = ulLastBytesPlayed;
    m_ulPausePosition = ulPausePosition;

    _SetDeviceConfig(NULL);

    m_ulPausePosition = 0;
    HX_RESULT retCode = RA_AOE_NOERR;
    m_wLastError = retCode;
    
    // verify that this worked
    audio_status astatus;
    ioctl(m_nDevID, AUDIO_GET_STATUS, &astatus);
    
    return m_wLastError;
}

//Device specific method to get/set the devices current volume.
UINT16 CAudioOutHPUX::_GetVolume() const
{
    _OpenMixer();
    
    return m_nLastVolume; 
}

HX_RESULT CAudioOutHPUX::_SetVolume(UINT16 unVolume)
{
    m_nLastVolume = unVolume;

    HX_RESULT retCode = RA_AOE_NOERR;

    struct audio_gain gainsettings;
    memset(&gainsettings, 0, sizeof(audio_gain));
    gainsettings.channel_mask = AUDIO_CHANNEL_LEFT | AUDIO_CHANNEL_RIGHT;

    gainsettings.cgain[0].transmit_gain = (INT32)unVolume * (m_nMaxVolume - m_nMinVolume) / 100 + m_nMinVolume;
    gainsettings.cgain[1].transmit_gain = (INT32)unVolume * (m_nMaxVolume - m_nMinVolume) / 100 + m_nMinVolume;
    
    if (::ioctl( m_nDevID, AUDIO_SET_GAINS, &gainsettings) < 0)
      {
  	retCode = RA_AOE_NOTENABLED;
      }

    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to drain a device. This should play the remaining
//bytes in the devices buffer and then return.
HX_RESULT CAudioOutHPUX::_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    

    m_wLastError = retCode;
    return m_wLastError;
}



UINT64 CAudioOutHPUX::_GetBytesActualyPlayed(void) const
{
    /* Get current playback position in device DMA. */
    int     bytes2 = 0;
    ULONG32 ulTheAnswer = 0;

    //What versions of the linux kernel do we want to support?
    if( !m_bGetODelayFailed )
    {
        if( m_ulTotalWritten > 0 )
        {
            HX_ASSERT( m_unSampleRate!=0 && m_uSampFrameSize!=0 );
            ULONG32 ulTick = GetTickCount();

            //We need to update the timestamps every so often.
            //This make sure that if the XServer was blocked, and
            //we ran dry, that we re-sync up.
            if( (ulTick-m_ulLastTimeStamp)>200 )
            {
                ((CAudioOutHPUX*)this)->_SyncUpTimeStamps(); // ug... constness of method prohibits this call somtimes
                ulTick = GetTickCount();
            }
            
            ulTheAnswer = (ULONG32)(m_ulLastBytesPlayed+
            ((float)(ulTick-m_ulLastTimeStamp)*
             (float)m_unNumChannels/1000.0*
             m_unSampleRate*m_uSampFrameSize) +0.5 );
        }
    }
    else
    {
        //We will assume that the error is because of an incomplete 
        //implementation of the oss compatible driver. So, just 
        //fake it with time stamps.
        if( m_ulTotalWritten > 0 )
        {
            ulTheAnswer = (ULONG32)((float)(GetTickCount()-m_ulTickCount)*(float)m_unNumChannels/1000.0*m_unSampleRate*m_uSampFrameSize);
            ulTheAnswer += m_ulPausePosition;
        }
    }
    return  ulTheAnswer;
}


//this must return the number of bytes that can be written without blocking.
//Don't use SNDCTL_DSP_GETODELAY here as it can't compute that amount
//correctly.
HX_RESULT CAudioOutHPUX::_GetRoomOnDevice(ULONG32& ulBytes) const
{
    audio_status astatus;
    int theErr;
    
    theErr = ioctl(m_nDevID, AUDIO_GET_STATUS, &astatus);
    
    if( theErr != -1)
    {
	ulBytes = m_ulDeviceBufferSize - astatus.transmit_buffer_count;
    }

    return RA_AOE_NOERR;
}

HX_RESULT CAudioOutHPUX::_CheckFormat( const HXAudioFormat* pFormat )
{
    return RA_AOE_NOERR;
}


HX_RESULT CAudioOutHPUX::_CheckSampleRate( ULONG32 ulSampleRate )
{
    return m_wLastError = RA_AOE_NOERR;
}


HX_RESULT CAudioOutHPUX::_Pause() 
{
  //  ioctl(m_nDevID, AUDIO_PAUSE, AUDIO_TRANSMIT);
  
    m_wLastError = HXR_OK;
      m_ulPausePosition = m_ulTotalWritten;
      m_ulTickCount = 0;
      m_ulLastTimeStamp   = 0;
    return m_wLastError;
}

HX_RESULT CAudioOutHPUX::_Resume()
{
  //  ioctl(m_nDevID, AUDIO_RESUME, AUDIO_TRANSMIT);
    m_wLastError = HXR_OK;
    
    if( m_ulTotalWritten > 0 )
    {
	m_ulTickCount = GetTickCount();
	m_ulLastTimeStamp = m_ulTickCount;
    }
    return m_wLastError;
}
