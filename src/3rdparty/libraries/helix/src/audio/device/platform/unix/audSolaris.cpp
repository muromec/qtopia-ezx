/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audSolaris.cpp,v 1.7 2007/07/06 20:21:16 jfinnecy Exp $
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

#include "audSolaris.h"
#include "ihxpckts.h"
#include "hxstrutl.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stropts.h> //for I_FLUSH

#include <stdio.h>


//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
CAudioOutSolaris::CAudioOutSolaris() :
    CAudioOutUNIX(),
    m_nDevID(NO_FILE_DESCRIPTOR),
    m_nMixerID(NO_FILE_DESCRIPTOR),
    m_nBlockDivisions(2)
{
};

CAudioOutSolaris::~CAudioOutSolaris()
{
    //The mixer may be used without the audio device. Make sure
    //it gets closed here.
    _CloseMixer();
};

//-------------------------------------------------------
// These Device Specific methods must be implemented 
// by the platform specific sub-classes.
//-------------------------------------------------------
INT16 CAudioOutSolaris::_Imp_GetAudioFd(void)
{
    return m_nDevID;
}

//Devic specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT CAudioOutSolaris::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    HX_RESULT  retCode = RA_AOE_NOERR;    
    audio_info stAudioinfo;
        
    if( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
        AUDIO_INITINFO( &stAudioinfo );
        stAudioinfo.play.sample_rate      = pFormat->ulSamplesPerSec;
        stAudioinfo.record.sample_rate    = pFormat->ulSamplesPerSec;
        stAudioinfo.play.channels         = pFormat->uChannels;
        stAudioinfo.record.channels       = pFormat->uChannels;
        stAudioinfo.play.precision        = pFormat->uBitsPerSample;
        stAudioinfo.record.precision      = pFormat->uBitsPerSample;
        stAudioinfo.play.encoding         = AUDIO_ENCODING_LINEAR;
        stAudioinfo.record.encoding       = AUDIO_ENCODING_LINEAR;
//            stAudioinfo.play.buffer_size      = m_ulBytesPerGran;

            //Now set the new config.
        if( ::ioctl(m_nDevID, AUDIO_SETINFO, &stAudioinfo) < 0 ) 
        {
#ifdef _DEBUG                
            fprintf( stderr, "Error setting audio config\n");
#endif                
            retCode = RA_AOE_GENERAL;
        }
        else
        {
            //Set member vars from CAudioOutUNIX.
            m_wBlockSize         = m_ulBytesPerGran;
            m_unNumChannels      = pFormat->uChannels;
            m_uSampFrameSize     = pFormat->uBitsPerSample / 8;		
            m_unSampleRate       = pFormat->ulSamplesPerSec;
            //XXXgfw Is there a size? Doesn't look like it.
            //XXXgfw Lets just pick one that looks good.
            m_ulDeviceBufferSize = 1<<16;
        }

            
        AUDIO_INITINFO(&stAudioinfo);
        if( ::ioctl(m_nDevID, AUDIO_GETINFO, &stAudioinfo) < 0 ) 
        {
#ifdef _DEBUG            
            fprintf( stderr, "CAudioOutSolaris: Can't get audio configuration.\n");
#endif            
            retCode = RA_AOE_GENERAL;
        }
        else
        {
            HX_ASSERT( stAudioinfo.play.sample_rate == m_unSampleRate ); 
            HX_ASSERT( stAudioinfo.play.channels    == m_unNumChannels );
            HX_ASSERT( stAudioinfo.play.precision   == m_uSampFrameSize*8 );
            HX_ASSERT( stAudioinfo.play.encoding    == AUDIO_ENCODING_LINEAR );
//                HX_ASSERT( stAudioinfo.play.buffer_size == m_ulBytesPerGran );
        }
    }

    //XXXgfw 
//      fprintf( stderr, "channels: %d   FrameSize: %d   SampleRate: %d   BuffSize: %d BlockSize: %d\n",
//               m_unNumChannels, m_uSampFrameSize, m_unSampleRate, m_ulDeviceBufferSize, m_wBlockSize);


    m_wLastError = retCode;
    return m_wLastError;
}


ULONG32 CAudioOutSolaris::_WriteWithEOF(UCHAR* buffer, ULONG32 ulBuffLength )
{
    HX_ASSERT( ulBuffLength != 0 );
    
    ULONG32 lCount = ::write( m_nDevID, buffer, ulBuffLength);
    if( lCount > 0 )
    {
        //We wrote at least one byte. Write an EOF marker.
        char  szEOF[1]; /* Flawfinder: ignore */
        if( ::write( m_nDevID, szEOF, 0) < 0 )
        {
            //Some sort of error. This is going to throw off our
            //ability to report the number of bytes played. We could
            //keep track of how many times this happens and then just
            //force a restart of the playback or something.
            HX_ASSERT( "_WriteWithEOF: Failed to write EOF" == NULL );
        }
    }
    else
    {
        HX_ASSERT( "_WriteWithEOF: Can't write to device." == NULL );
    }
    return lCount;
}


//Device specific method to write bytes out to the audiodevice and return a 
//count of bytes written. 
HX_RESULT CAudioOutSolaris::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
{
    HX_ASSERT( ulBuffLength==m_wBlockSize);
    
    HX_RESULT retCode = RA_AOE_NOERR;

    if( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else if( ulBuffLength > 0 ) //Ignore all zero length writes. They screw up eof count.
    {
        //Try to increase resolution by splitting the write. ulBuffLength is 
        //always even.
        HX_ASSERT( (m_wBlockSize%m_nBlockDivisions) == 0 );
        
        int i=0;
        int nChunkSize = m_wBlockSize/m_nBlockDivisions;
        
        for( i=0 ; i< m_nBlockDivisions ; i++ )
        {
            lCount = _WriteWithEOF( buffer+i*nChunkSize, nChunkSize );
            if( lCount < 0 )
                break;
        }
        
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
            lCount = m_wBlockSize/m_nBlockDivisions*i;
        }
    }
    
    
    return retCode;
}

//Device specific methods to open/close the mixer and audio devices.
HX_RESULT CAudioOutSolaris::_OpenAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    //Check the environmental variable to let user overide default device.
    char *pszOverrideName = getenv( "AUDIO" ); /* Flawfinder: ignore */
    char szDevName[MAX_DEV_NAME+1]; /* Flawfinder: ignore */
    
    // Use defaults if no environment variable is set.
    if ( pszOverrideName && strlen(pszOverrideName)>0 )
    {
        SafeStrCpy( szDevName, pszOverrideName, MAX_DEV_NAME );
    }
    else
    {
        pszOverrideName = getenv( "AUDIODEV" ); /* Flawfinder: ignore */
        if ( pszOverrideName && strlen(pszOverrideName)>0 )
        {
            SafeStrCpy( szDevName, pszOverrideName, MAX_DEV_NAME );
        }
        else
        {
            SafeStrCpy( szDevName, "/dev/audio", MAX_DEV_NAME);
        }
    }
    szDevName[MAX_DEV_NAME]='\0';
    
    // Open the audio device if it isn't already open
    if ( m_nDevID < 0 )
    {
        //We open in non block mode first, just to test wether or not
        //someone has the audio device. After that we must reopen it
        //so that our writes always block.
        m_nDevID = ::open( szDevName, O_WRONLY | O_EXCL | O_NONBLOCK );
        if( m_nDevID >= 0 )
        {
            //XXXgfw Yeah, I know, why don't I just use ioctl(....,FIONBIO, ...);
            //or fcntl? because it doesn't work!
            close(m_nDevID);
            m_nDevID = ::open( szDevName, O_WRONLY | O_EXCL);
        }
        
    }
    
    if ( m_nDevID < 0 )
    {
        //Error opening device.
        retCode = RA_AOE_BADOPEN;
    }

    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutSolaris::_CloseAudio()
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

HX_RESULT CAudioOutSolaris::_OpenMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    if(!m_bMixerPresent)
    {
        //Let user override default device with environ variable.
        char *pszOverrideName = getenv( "MIXER" ); /* Flawfinder: ignore */
        char szDevCtlName[MAX_DEV_NAME]; /* Flawfinder: ignore */
        
        if (pszOverrideName && strlen(pszOverrideName)>0 )
        {
            SafeStrCpy( szDevCtlName , pszOverrideName, MAX_DEV_NAME );
        }
        else
        {
            SafeStrCpy( szDevCtlName, "/dev/audioctl", MAX_DEV_NAME);	// default for volume
        }
        
        m_nMixerID = ::open( szDevCtlName, O_RDWR );
        
        if (m_nMixerID > 0)
        {
            m_bMixerPresent = 1;
            _Imp_GetVolume();
        }
        else
        {
            m_nMixerID = NO_FILE_DESCRIPTOR;
            m_bMixerPresent = 0;
        }
    }

    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutSolaris::_CloseMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    if( m_nMixerID >= 0 )
    {
        ::close( m_nMixerID );
        m_nMixerID = NO_FILE_DESCRIPTOR;
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to reset device and return it to a state that it 
//can accept new sample rates, num channels, etc.
HX_RESULT CAudioOutSolaris::_Reset()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else if( ::ioctl(m_nDevID, I_FLUSH, FLUSHW) == -1 )
    {
        retCode = RA_AOE_GENERAL;
    }

    //Now, m_ulTotalWritten is going to go to Zero in CAudioOutUNIX.
    //So, we must reset the EOF marker.
    audio_info stAudioInfo;
    AUDIO_INITINFO( &stAudioInfo );
    
    //Fill in the structure with current settings.
    if (::ioctl(m_nDevID, AUDIO_GETINFO, &stAudioInfo) < 0) 
    {
#ifdef _DEBUG        
	fprintf( stderr, "CAudioOutSolaris: Just trying to reset EOF in _Reset.\n");
#endif        
    }

    //Just change the gain setting. Must map incoming [0,100]-->[0,255]
    stAudioInfo.play.eof     = 0;
    stAudioInfo.play.samples = 0;

    //Now store the new settings.
    if( ::ioctl(m_nMixerID, AUDIO_SETINFO, &stAudioInfo)<0 ) 
    {
#ifdef _DEBUG        
	fprintf( stderr, "CAudioOutSolaris: just trying to reset EOF\n");
#endif        
    }

    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to get/set the devices current volume.
UINT16 CAudioOutSolaris::_GetVolume() const
{
    UINT16     unVolume = 0;
    audio_info stAudioInfo;
    
    if (::ioctl( m_nMixerID, AUDIO_GETINFO, &stAudioInfo) < 0)
    {
        //What to do here? Should we close the mixer and pretend
        //we don't have one anymore?
        unVolume = m_uCurVolume;
    }
    else
    {
        //Map device specific volume levels to [0,100]. 
        unVolume = (UINT16) (((float)(stAudioInfo.play.gain-AUDIO_MIN_GAIN))*100.0 / (float)(AUDIO_MAX_GAIN-AUDIO_MIN_GAIN) + 0.5);
        
    }

    return unVolume;
}

HX_RESULT CAudioOutSolaris::_SetVolume(UINT16 unVolume)
{
    HX_RESULT  retCode = RA_AOE_NOERR;
    audio_info stAudioInfo;

    //Fill in the structure with current settings.
    if (::ioctl(m_nMixerID, AUDIO_GETINFO, &stAudioInfo) < 0) 
    {
#ifdef _DEBUG        
	fprintf( stderr, "CAudioOutSolaris: Can't get the audio configuration\n");
#endif        
	retCode = RA_AOE_NOTSUPPORTED;
    }

    //Just change the gain setting. Must map incoming [0,100]-->[0,255]
    stAudioInfo.play.gain = (UINT16)((float)unVolume/100.0 *(float)(AUDIO_MAX_GAIN-AUDIO_MIN_GAIN) + AUDIO_MIN_GAIN + 0.5);

    //Now store the new settings.
    if( ::ioctl(m_nMixerID, AUDIO_SETINFO, &stAudioInfo)<0 ) 
    {
#ifdef _DEBUG        
	fprintf( stderr, "CAudioOutSolaris: Can't set the audio configuration.\n");
#endif        
        retCode = RA_AOE_NOTSUPPORTED;
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to drain a device. This should play the remaining
//bytes in the devices buffer and then return.
HX_RESULT CAudioOutSolaris::_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }

    //XXXgfw We don't use _Drain anywhere right now. The problem
    //XXXgfw is that drain will block the proc until the buffers
    //XXXgfw are all drained. We don't want that. Just let it
    //XXXgfw play.
//      else if( ::ioctl (m_nDevID, AUDIO_DRAIN, 0) == -1 )
//      {
//          retCode = RA_AOE_GENERAL;
//      }

    m_wLastError = retCode;
    return m_wLastError;
}


UINT64 CAudioOutSolaris::_GetBytesActualyPlayed(void) const
{
    UINT64    ulTheAnswer = 0;
    audio_info stAudioInfo;


    //Fill in the structure with current settings.
    AUDIO_INITINFO( &stAudioInfo );
    if (::ioctl(m_nDevID, AUDIO_GETINFO, &stAudioInfo) < 0) 
    {
#ifdef _DEBUG        
	fprintf( stderr, "CAudioOutSolaris::_GetBytesActualyPlayed: Failure.\n");
#endif
        HX_ASSERT( "_GetBytesActualyPlayed: Error reading audio device"==NULL );
        ulTheAnswer = 0;
    }

    if( stAudioInfo.play.error )
    {
        //Underflow has occured.
        HX_ASSERT( "_GetBytesActualyPlayed: underflow in audio device"==NULL);
    }
    
    //We *always* right full blocks to the device. Well, in the
    //case of solaris, we don't use the rewind mechinism of CAudioOutUNIX 
    //so this is true.
    ulTheAnswer = stAudioInfo.play.eof*m_wBlockSize/m_nBlockDivisions;

    //XXXgfw It would be so cool of we could count on the samples field.
//    ulTheAnswer = (ULONG32)stAudioInfo.play.samples*m_unNumChannels*m_uSampFrameSize;

//        fprintf( stderr, "m_ulTotalWritten: %lu  Bytesplayed:%lu   eof: %d  samples: %d \n",
//                 m_ulTotalWritten, ulTheAnswer, stAudioInfo.play.eof, stAudioInfo.play.samples );
    
    return  ulTheAnswer; 
}


HX_RESULT CAudioOutSolaris::_GetRoomOnDevice(ULONG32& ulBytes) const 
{
    HX_RESULT      retCode = RA_AOE_NOERR;
    
    ulBytes = m_ulDeviceBufferSize-(m_ulTotalWritten-_GetBytesActualyPlayed());
    m_wLastError = retCode;
    return m_wLastError;
}
HX_RESULT CAudioOutSolaris::_CheckFormat( const HXAudioFormat* pFormat )
{
    m_wLastError = RA_AOE_NOERR;
    return m_wLastError;
}


HX_RESULT CAudioOutSolaris::_CheckSampleRate( ULONG32 ulSampleRate ) 
{

    m_wLastError = RA_AOE_NOERR;

    //XXXgfw For now, just assume that it is supported.
    
    return m_wLastError;
}

HX_RESULT CAudioOutSolaris::_Pause() 
{
    audio_info_t stAudioInfo;
    m_wLastError = HXR_OK;

    AUDIO_INITINFO(&stAudioInfo);
    stAudioInfo.play.pause = 1;

    if( ::ioctl(m_nDevID, AUDIO_SETINFO, &stAudioInfo) < 0 )
    {
        m_wLastError = RA_AOE_NOTSUPPORTED;
    }
    return m_wLastError;
}

HX_RESULT CAudioOutSolaris::_Resume()
{
    audio_info_t stAudioInfo;
    m_wLastError = HXR_OK;

    AUDIO_INITINFO(&stAudioInfo);
    stAudioInfo.play.pause = 0;

    if( ::ioctl(m_nDevID, AUDIO_SETINFO, &stAudioInfo) < 0 ) 
    {
        m_wLastError = RA_AOE_NOTSUPPORTED;
    }
    return m_wLastError;
}

HXBOOL CAudioOutSolaris::_IsSelectable() const
{
    return FALSE;
}

HXBOOL CAudioOutSolaris::_HardwarePauseSupported() const
{
    return TRUE;
}

 
