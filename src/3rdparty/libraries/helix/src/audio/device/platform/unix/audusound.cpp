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
 * Contributor(s): Matt Campbell <mattcampbell@pobox.com>
 *  
 * ***** END LICENSE BLOCK ***** */ 

//===================================================================================
// 
//   	audusound.cpp
// 
//   	CLASS: CAudioOutUSound
// 	
//   	Implements the sound subsystem for the Useful Sound Daemon.
//
//===================================================================================

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h> 


#include "audusound.h"
#include "ihxpckts.h"

//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
CAudioOutUSound::CAudioOutUSound() :
    CAudioOutUNIX(),
    m_pStream(NULL)
{
#ifdef _DEBUG
    printf("ran CAudioOutUSound constructor\n");
#endif
};

CAudioOutUSound::~CAudioOutUSound()
{
};

//-------------------------------------------------------
// These Device Specific methods must be implemented 
// by the platform specific sub-classes.
//-------------------------------------------------------

// The client library should have a usound_get_output_fd() function, but
// it doesn't, so I'll do this the ugly way and fix it whenever I release
// the next version of USound. --Matt Campbell
struct _usound_output
{
    int control_fd;
    int data_fd;
};

INT16 CAudioOutUSound::_Imp_GetAudioFd(void)
{
    int fd;
    if (m_pStream == NULL)
        fd = NO_FILE_DESCRIPTOR;
    else
        fd = m_pStream->data_fd;
#ifdef _DEBUG
    printf("usound: _GetAudioFd()\n");
    printf("returning %d\n", fd);
#endif
    return fd;
}

//Devic specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT CAudioOutUSound::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    if ( !usound_is_available() )
        return RA_AOE_DEVNOTOPEN;

#ifdef _DEBUG
    printf("entering CAudioOutUSound::_SetDeviceConfig\n");
#endif

    //We can only do this once after opening the audio device.
    if( m_pStream != NULL )
        return RA_AOE_DEVBUSY;
    
    // Convert from bits to bytes.
    int nSampleBytes = pFormat->uBitsPerSample / 8;
    //Now open our USound output stream.
    m_pStream = usound_open_output(nSampleBytes, pFormat->uChannels,
                                   pFormat->ulSamplesPerSec);
    
    if ( m_pStream == NULL )
    {
        //Error opening device.
#ifdef _DEBUG
        printf("usound: stream open failed\n");
#endif
        return RA_AOE_DEVNOTOPEN;
    }
#ifdef _DEBUG
    printf("usound: stream opened\n");
#endif

    m_wBlockSize         = m_ulBytesPerGran;
    m_unNumChannels      = pFormat->uChannels;
    m_unSampleRate       = pFormat->ulSamplesPerSec;
    m_ulDeviceBufferSize = usound_get_output_buffer_size(m_pStream);
    m_uSampFrameSize     = pFormat->uBitsPerSample/8;

#ifdef _DEBUG
    fprintf( stderr, "Device Configured:\n");
    fprintf( stderr, "        Sample Rate: %d\n", m_unSampleRate);
    fprintf( stderr, "       Sample Width: %d\n", m_uSampFrameSize);
    fprintf( stderr, "       Num channels: %d\n", m_unNumChannels);
    fprintf( stderr, "         Block size: %d\n", m_wBlockSize);
    fprintf( stderr, "  Device buffer size: %lu\n", m_ulDeviceBufferSize);
#endif

    return RA_AOE_NOERR;
}

//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written. 
HX_RESULT CAudioOutUSound::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
{
    
    HX_RESULT retCode = RA_AOE_NOERR;
    if( m_pStream == NULL )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        lCount = usound_write( m_pStream, buffer, ulBuffLength);
        
        if( lCount < 0 )
        {
            //Error occurred.
            if( errno == EAGAIN )
                retCode = RA_AOE_NOERR;
            if( errno == EINTR )
                retCode = RA_AOE_DEVBUSY;
        }
        
    }
    return retCode;
}

//The following comment from audlinux_esound.cpp also applies to USound:
//XXXgfw, since we have to open the audio device in ESD with
//all the format information (no way to change it once open)
//we will just return OK here and do the actual open in 
//SetDeviceConfig() call.
HX_RESULT CAudioOutUSound::_OpenAudio()
{
    
    HX_RESULT retCode = RA_AOE_NOERR;
    m_wLastError = retCode;
    return m_wLastError;
}


HX_RESULT CAudioOutUSound::_CloseAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if( m_pStream != NULL )
    {
        //close the stream.
        usound_close_output(m_pStream);
        m_pStream = NULL;
    }
    else
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }

    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutUSound::_OpenMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    //USound always has volume support.
    m_bMixerPresent = 1;
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Do nothing under USound.
HX_RESULT CAudioOutUSound::_CloseMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to reset device and return it to a state that it 
//can accept new sample rates, num channels, etc.
HX_RESULT CAudioOutUSound::_Reset()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    if ( m_pStream == NULL )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        usound_reset_output(m_pStream);
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to get/set the devices current volume.
UINT16 CAudioOutUSound::_GetVolume() const
{
    int nRetVolume   = 0;

    if( m_pStream != NULL )
    {
        nRetVolume = usound_get_output_volume(m_pStream);
    }
    else
    {
        //USound always starts out an app at 65536.
        nRetVolume = 65536;
    }

    //Map device specific volume levels to [0,100]. 
    nRetVolume = (int) ((float)nRetVolume/65536.0*100.0+0.5);

    return nRetVolume; 
}

HX_RESULT CAudioOutUSound::_SetVolume(UINT16 unVolume)
{
    HX_RESULT retCode = RA_AOE_NOERR;

    //Map incoming [0..100] volume to 65536.
    ULONG32 ulVolume = (int)((float)unVolume/100.0 * 65536.0 + 0.5 );
#ifdef _DEBUG
    printf("usound: setting volume to %d\n", ulVolume);
#endif

    if( m_pStream != NULL )
    {
        usound_set_output_volume(m_pStream, ulVolume);
    }
    else
    {
        //Just do nothing here for now....
    }

    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to drain a device. This should play the remaining
//bytes in the devices buffer and then return.
HX_RESULT CAudioOutUSound::_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_pStream == NULL )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        usound_reset_output(m_pStream);
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}



UINT64 CAudioOutUSound::_GetBytesActualyPlayed(void) const
{
    if (m_pStream == NULL)
    {
        return 0;
    }
    else
    {
        return usound_get_bytes_played(m_pStream);
    }
}


//this must return the number of bytes that can be written without blocking.
//Don't use SNDCTL_DSP_GETODELAY here as it can't compute that amount
//correctly.
HX_RESULT CAudioOutUSound::_GetRoomOnDevice(ULONG32& ulBytes) const
{
    HX_RESULT retCode     = RA_AOE_NOERR;

    if (m_pStream == NULL)
    {
        ulBytes = 0;
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        ulBytes = usound_get_output_space(m_pStream);
    }

    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutUSound::_CheckFormat( const HXAudioFormat* pFormat )
{
    m_wLastError = RA_AOE_NOERR;
    if ((pFormat->uChannels != 1 && pFormat->uChannels != 2) ||
        (pFormat->uBitsPerSample != 8 && pFormat->uBitsPerSample != 16))
    {
       m_wLastError = RA_AOE_BADFORMAT;
    }
    return m_wLastError;
}


HX_RESULT CAudioOutUSound::_CheckSampleRate( ULONG32 ulSampleRate )
{
    //USound will probably work with any sample rate.
    m_wLastError = RA_AOE_NOERR;
    return m_wLastError;
}
 

HX_RESULT CAudioOutUSound::_Pause() 
{
    m_wLastError = HXR_OK;

    if (m_pStream == NULL)
    {
        m_wLastError = HXR_FAIL;
    }
    else
    {
        usound_stop(m_pStream);
    }

    return m_wLastError;
}

HX_RESULT CAudioOutUSound::_Resume()
{
    m_wLastError = HXR_OK;

    if (m_pStream == NULL)
    {
        m_wLastError = HXR_FAIL;
    }
    else
    {
        usound_play(m_pStream);
    }

    return m_wLastError;
}

HXBOOL CAudioOutUSound::_IsSelectable() const
{
    return TRUE;
}


HXBOOL CAudioOutUSound::_HardwarePauseSupported() const
{
    return TRUE;
}


