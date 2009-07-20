/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audirix.cpp,v 1.2 2004/07/09 18:38:01 hubbe Exp $
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

#include "audirix.h"
#include "ihxpckts.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h> 
#include <math.h> 

//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
CAudioOutIrix::CAudioOutIrix() :
    CAudioOutUNIX(),
    m_pALPort(NULL),
    m_nDevID(NO_FILE_DESCRIPTOR),
    m_nMixerID(NO_FILE_DESCRIPTOR)
{
};

CAudioOutIrix::~CAudioOutIrix()
{
};

//-------------------------------------------------------
// These Device Specific methods must be implemented 
// by the platform specific sub-classes.
//-------------------------------------------------------
INT16 CAudioOutIrix::_Imp_GetAudioFd(void)
{
    return m_nDevID;
}

//Devic specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT CAudioOutIrix::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    if ( m_nDevID < 0 )
        return RA_AOE_DEVNOTOPEN;


    m_wBlockSize = m_ulBytesPerGran;

    //Now set the format. Either 8-bit or 16-bit audio is supported.
    int      nSampleWidth  = pFormat->uBitsPerSample;
    ULONG32  nSampleRate   = pFormat->ulSamplesPerSec;
    int      numChannels   = pFormat->uChannels;
    m_unNumChannels        = numChannels;
    m_uSampFrameSize       = nSampleWidth / 8;		
    m_unSampleRate         = nSampleRate;

    //Get a new alConfig so we can set the port.
    ALconfig pALConfig = ALnewconfig();
    if( !pALConfig )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't Alloc new Config.\n");
#endif
       return (  m_wLastError = RA_AOE_NOTENABLED );
    }
 
    //Set the format to PCM two compliment integer data.
    ALsetsampfmt( pALConfig, AL_SAMPFMT_TWOSCOMP );
 
    if( nSampleWidth == 16)
    {
        ALsetwidth( pALConfig, AL_SAMPLE_16 );
    }
    else
    {
        ALsetwidth( pALConfig, AL_SAMPLE_8 );
    }
    
    if(ALsetchannels( pALConfig, numChannels) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't set number of channels.\n");
#endif
       return (  m_wLastError = RA_AOE_NOTENABLED );
    }

    //Set sample rate. For now we are assuming that they
    //are using the default analog out device.
    //XXXgfw we should query for the active output device
    //       and use that one instead.

    //Now let do the rate.
    long params[4];
    params[0] = AL_OUTPUT_RATE;
    params[1] = nSampleRate;
#ifdef _DEBUG
    fprintf( stderr, "Trying to set a rate of: %d\n", nSampleRate );
#endif
    if( ALsetparams( AL_DEFAULT_DEVICE, params, 2 ) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "alSetParams has failed: %d n", errno);
#endif
       return (  m_wLastError = RA_AOE_NOTENABLED );
    }
    if( params[1] != nSampleRate )
    {
#ifdef _DEBUG
        fprintf( stderr, "Invalid rate specified.\n");
#endif
       return (  m_wLastError = RA_AOE_NOTENABLED );
    }

    //In Irix we can pick our own buffer size. Yeah!
    //For now, however, lets just leave the defualt which
    //is 50,000 samples big.
    int nTmp = ALgetqueuesize( pALConfig );
    ALsetqueuesize( pALConfig, nTmp );
#ifdef _DEBUG
    fprintf( stderr, "Irix is using: %d sample frames to buffer.\n", nTmp );
#endif
    m_ulDeviceBufferSize = nTmp*m_uSampFrameSize*m_unNumChannels;

    //Now set the ports configuration.
    ALcloseport( m_pALPort );
    m_pALPort = ALopenport( "RealPlayer", "w", pALConfig );
//    if ( ALsetconfig( m_pALPort, pALConfig ) == -1 )
//    {
//#ifdef _DEBUG
//        fprintf( stderr, "Can not set the ports configuration.\n" );
//        fprintf( stderr, "errno: %d.\n", oserror() );
//#endif
//        return ( m_wLastError = RA_AOE_NOTENABLED );
//    } 
    //XXXgfw
    ALfreeconfig( pALConfig );
    
#ifdef _DEBUG
    fprintf( stderr, "Device Configured:\n");
    fprintf( stderr, "      Sample Rate: %d\n", m_unSampleRate);
    fprintf( stderr, "     Sample Width: %d\n", nSampleWidth);
    fprintf( stderr, "     Num channels: %d\n", m_unNumChannels);
    fprintf( stderr, " Device buff size: %lu\n", m_ulDeviceBufferSize);
#endif
    return RA_AOE_NOERR;
}

//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written. 
HX_RESULT CAudioOutIrix::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
{
    HX_RESULT retCode = RA_AOE_NOERR;

    if( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        lCount = ulBuffLength;
        int nFrameCount = ulBuffLength/m_uSampFrameSize;
        ALwritesamps( m_pALPort, buffer, nFrameCount );
    }
    return retCode;
}

//Device specific methods to open/close the mixer and audio devices.
HX_RESULT CAudioOutIrix::_OpenAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    // Open the audio device if it isn't already open
    if ( m_nDevID < 0 )
    {
        //Open an audio port and set the config later.
        m_pALPort = ALopenport( "RealPlayer", "w", 0 );
        if( m_pALPort == 0 )
        {
#ifdef _DEBUG
            fprintf( stderr, "Can't open port. oserror is: %d\n",
                     oserror() );
#endif
        }
#ifdef _DEBUG
        fprintf( stderr, "Got a port: %p\n", m_pALPort );
#endif
            
        //Grab a file descriptor so we can select on it.
        if( NULL != m_pALPort )
            m_nDevID = ALgetfd( m_pALPort );
    }
    
    if ( m_nDevID < 0 || NULL == m_pALPort )
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

HX_RESULT CAudioOutIrix::_CloseAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if( m_nDevID >= 0 )
    {
        ALcloseport( m_pALPort );
        m_pALPort = NULL;
        m_nDevID  = NO_FILE_DESCRIPTOR;
    }
    else
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutIrix::_OpenMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    //We always have a mixer on Irix.
    m_bMixerPresent = 1;
    _Imp_GetVolume();

    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutIrix::_CloseMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to reset device and return it to a state that it 
//can accept new sample rates, num channels, etc.
HX_RESULT CAudioOutIrix::_Reset()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        //Throw away all frames in the audio buffer.
        //XXXgfw this isn't available on Irix 6.2 
        //alDiscardFrames(m_pALPort, m_ulDeviceBufferSize/m_uSampFrameSize/m_unNumChannels );
    }

    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to get/set the devices current volume.
UINT16 CAudioOutIrix::_GetVolume() const
{  
    float newVolume = 0;
    long buf[4] = {
                    AL_LEFT_SPEAKER_GAIN, 128,
                    AL_RIGHT_SPEAKER_GAIN, 128
                  };

    ALgetparams(AL_DEFAULT_DEVICE, buf, 4);
    //Map volume to [0..100] which is what the core needs.
    //We are doing a logrythmic conversion.
    if( buf[1] != 0 )
      newVolume = 18.1*logf((float)buf[1]);

    //Round up or down.
    newVolume = trunc( newVolume+0.5 );

    if( newVolume<0 )  newVolume=0;
    if( newVolume>255) newVolume=255;

#ifdef _DEBUG
    //fprintf( stderr, "Got a volume of: %f calculated from %lu\n", newVolume, buf[1] );
#endif
    return (UINT16)newVolume; 
}

HX_RESULT CAudioOutIrix::_SetVolume(UINT16 unVolume)
{ 
    float     newVolume = 0;
    HX_RESULT retCode   = RA_AOE_NOERR;

    long buf[4] = {
                    AL_LEFT_SPEAKER_GAIN, 128,
                    AL_RIGHT_SPEAKER_GAIN, 128
                  };


   //Map incoming volume from [0..100] to [0..255]
   //Volume appears to be logrythmic.
   if( unVolume != 0 )
      newVolume = expf((float)unVolume/18.1);

   //Round up or down.
   newVolume = trunc( newVolume+0.5 );

   if( newVolume>255 ) 
      newVolume=255;

   if( newVolume<0 )
      newVolume=0;

#ifdef _DEBUG
   //fprintf( stderr, "Setting volume to %f calculated from %lu\n", newVolume, unVolume );
#endif

   buf[1] = newVolume;
   buf[3] = newVolume;
   ALsetparams(AL_DEFAULT_DEVICE, buf, 4);

   m_wLastError = retCode;
   return m_wLastError;
}

//Device specific method to drain a device. This should play the remaining
//bytes in the devices buffer and then return.
HX_RESULT CAudioOutIrix::_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }

    m_wLastError = retCode;
    return m_wLastError;
}



ULONG32 CAudioOutIrix::_GetBytesActualyPlayed(void) const
{
    /* Get current playback position in device DMA. */
    int     bytes2 = 0;
    ULONG32 ulTheAnswer = 0;

    //Ask for the number of frames yet to be played in the
    //audio buffer and convert to bytes.
    bytes2 = ALgetfilled( m_pALPort );
    if( bytes2 >= 0)
    {
        //Convert frames to bytes.
        bytes2 = bytes2*m_uSampFrameSize*m_unNumChannels;
        ulTheAnswer = (ULONG32)(m_ulTotalWritten - bytes2 );
    }

    return  ulTheAnswer;
}


//this must return the number of bytes that can be written without blocking.
//Don't use SNDCTL_DSP_GETODELAY here as it can't compute that amount
//correctly.
HX_RESULT CAudioOutIrix::_GetRoomOnDevice(ULONG32& ulBytes) const
{
    HX_RESULT retCode     = RA_AOE_NOERR;

    //Get the number of frames we can write without blocking 
    //and convert it to bytes.
    int nFrames = ALgetfillable( m_pALPort );
    ulBytes = nFrames*m_uSampFrameSize*m_unNumChannels;
    
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutIrix::_CheckSampleRate( ULONG32 ulSampleRate )
{
    //Still need to do this one.
#ifdef _DEBUG
    fprintf( stderr, "_CheckSampleRate not support yet.\n" );
#endif
    m_wLastError = RA_AOE_NOERR;
    return m_wLastError;
}

