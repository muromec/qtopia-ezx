/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audlinux_oss.cpp,v 1.10 2005/01/24 08:00:50 rggammon Exp $
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

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <stdio.h> 
#include <math.h> 

#include "ihxpckts.h"
#include "hxtick.h"
#include "hxprefs.h"
#include "timeval.h"
#include "hxthread.h"
#include "audlinux_oss.h"
#include "hxstrutl.h"
#include "hxprefutil.h"

//we can't set the PCM volume on the PowerPC. The sound driver
//only lets up set the master volume.
#ifdef __powerpc__
#   define HX_VOLUME  SOUND_MIXER_VOLUME
#else
#   define HX_VOLUME  SOUND_MIXER_PCM
#endif


//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
CAudioOutLinux::CAudioOutLinux() :
    CAudioOutUNIX(),
    m_ulTickCount(0),
    m_ulLastBytesPlayed(0),
    m_ulLastTimeStamp(0),
    m_ulPausePosition(0),
    m_nDevID(NO_FILE_DESCRIPTOR),
    m_nMixerID(NO_FILE_DESCRIPTOR),
    m_bGetODelayFailed(TRUE),
    m_bGetOSpaceFailed(FALSE),
    m_bTestGetODelay(TRUE)
{
};

CAudioOutLinux::~CAudioOutLinux()
{
    //The mixer is opened independently of the audio device. Make sure 
    //it is closed.
    _CloseMixer();
};

//-------------------------------------------------------
// These Device Specific methods must be implemented 
// by the platform specific sub-classes.
//-------------------------------------------------------
INT16 CAudioOutLinux::_Imp_GetAudioFd(void)
{
    return m_nDevID;
}

//Devic specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT CAudioOutLinux::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    if ( m_nDevID < 0 )
        return RA_AOE_DEVNOTOPEN;

    m_wBlockSize = m_ulBytesPerGran;

    //First, we want to determine the fragment size of the device buffer.
    //We will let the device determine the number of fragments after we
    //make sure that each fragment is no bigger than our block size.
    //The minimum block size for OSS is 16 bytes and the max is 2^0xf.
    //Basically, Log-base-two(m_wBlockSize)+1.
    int nPower = 0x4;
    while( (1<<nPower)<m_wBlockSize && nPower<0xf )
    { 
        nPower++;
    }

    if( nPower > 4 )
        nPower--;
    
    //Frag info is 0xMMMMSSSS. Where
    //
    // MMMM is the total number of fragments. Set this to 7fff if you want
    //      OSS to figure it out.
    // SSSS is the size of each fragment. The size is 2^0xSSSS.
    int nFragInfo = 0x7fff0000 | nPower;

    //Now set the fragment size.
    if (ioctl(m_nDevID, SNDCTL_DSP_SETFRAGMENT, &nFragInfo) == -1)
    {
        return (m_wLastError = RA_AOE_NOTENABLED);
    }

    //Now set the format. Either 8-bit or 16-bit audio is supported.
    int      nSampleWidth  = pFormat->uBitsPerSample;
    ULONG32  nSampleRate   = pFormat->ulSamplesPerSec;
    int      numChannels   = pFormat->uChannels;
    int      nFormat1      = 0;
    int      nFormat2      = 0;
    
    if( nSampleWidth == 16)
    {
        nFormat1 = nFormat2 = AFMT_S16_NE;
    }
    else
    {
        nFormat1 = nFormat2 = AFMT_U8;
    }
    
    if(ioctl(m_nDevID, SNDCTL_DSP_SETFMT, &nFormat1) == -1)
    {
        return (  m_wLastError = RA_AOE_NOTENABLED );
    }

    //Check and see if the device supports the format we tried to set.
    //If it didn't take our only other option is unsigned 8 bit. So, if
    //that is what the device returned just use it.
    if(nFormat1!=nFormat2 && nFormat1 != AFMT_U8 )
    {
        //Just try to set AFMT_U8.
        nFormat1 = AFMT_U8;
        if(ioctl(m_nDevID, SNDCTL_DSP_SETFMT, &nFormat1) == -1)
        {
            return (  m_wLastError = RA_AOE_NOTENABLED );
        }
        if( nFormat1 != AFMT_U8 )
        {
            //No know format is supported.
            return (  m_wLastError = RA_AOE_NOTENABLED ); 
        }
    }
    //If we went to 8-bit then 
    if( nFormat1 == AFMT_U8 )
    {
        //AFMT_U8 is set.
        nSampleWidth = 8;
    }
    m_uSampFrameSize = nSampleWidth/8;
    if ( nSampleWidth != pFormat->uBitsPerSample )
    {
        ((HXAudioFormat*)pFormat)->uBitsPerSample = nSampleWidth;
    }
    
    
    // Set sampling rate, sample width, #channels.
    //
    // Make sure you set num channels before rate. If you don't it will
    // screw up SBPro  devices. Your rate will actually be 1/2 of what
    // you think it is.

    //Set number of channels. Stereo or mono.
    if (ioctl(m_nDevID, SOUND_PCM_WRITE_CHANNELS, &numChannels) == -1)
    {
        return ( m_wLastError = RA_AOE_NOTENABLED );
    }
    m_unNumChannels = numChannels;
    if ( numChannels != pFormat->uChannels )
    {
        ((HXAudioFormat*)pFormat)->uChannels = numChannels;
    }

    //Set the sample rate.
    if (ioctl(m_nDevID, SOUND_PCM_WRITE_RATE, &nSampleRate) == -1)
    {
        return ( m_wLastError = RA_AOE_NOTENABLED );
    }

    if (nSampleRate == 0)
    {
        /* 
         * Some drivers actually set the sample rate on the device, but
         * return 0 for the sample rate. On these platforms we just ignore
         * the return value and assume the sample rate is set to what was
         * requested.
         */

        nSampleRate = pFormat->ulSamplesPerSec;
    }

    m_unSampleRate = nSampleRate;
    if ( nSampleRate != pFormat->ulSamplesPerSec )
    {
        ((HXAudioFormat*)pFormat)->ulSamplesPerSec = nSampleRate;
    }



    //Find out if the user wants to support old OSS drivers that don't have,
    //or support well, the iocls needed for good syncing.  SNDCTL_DSP_GETOSPACE and
    //SNDCTL_DSP_GETODELAY.
    IHXPreferences* pPreferences = NULL;
    if( m_pContext && HXR_OK == m_pContext->QueryInterface( IID_IHXPreferences, (void **) &pPreferences))
    {
        UINT32 nOldOSS = 0;
        if (HXR_OK == ReadPrefUINT32(pPreferences, "SoundDriver", nOldOSS) &&
            (nOldOSS == kSupportForOldOSS))
        {
            m_bGetODelayFailed = TRUE;
            m_bGetOSpaceFailed = TRUE;
            m_bTestGetODelay = FALSE;
        }
        HX_RELEASE( pPreferences );
    }

//for now, PowerPC linux doesn't support the following ioctl call
//So, I will just make up some buffer size and live with it for
//now. You won't be able to tell the difference until you turn
//off threaded audio, then it will be block city!
#ifndef __powerpc__  
    audio_buf_info getYourInfoHere;
    if( !m_bGetOSpaceFailed )
    {
        //This call is to get how the device is set up after we have
        //set the sample rate etc. We use this to set up the buffers
        //we use for the fake pause/resume features.
        if (ioctl(m_nDevID, SNDCTL_DSP_GETOSPACE, &getYourInfoHere) == -1) 
        {
            m_wLastError = RA_AOE_NOTENABLED;
            return m_wLastError;
        }
        m_ulDeviceBufferSize = getYourInfoHere.fragsize*getYourInfoHere.fragstotal;
    }
    else
    {
        //We don't have anyway to determine how big the buffer is.
        //just guess I guess.
        m_ulDeviceBufferSize = 8192*4;
    }
#else
    m_ulDeviceBufferSize = 8192*4;
#endif

#ifdef _DEBUG
    fprintf( stderr, "Device Configured:\n");
    fprintf( stderr, "         Sample Rate: %d\n",  m_unSampleRate);
    fprintf( stderr, "        Sample Width: %d\n",  nSampleWidth);
    fprintf( stderr, "        Num channels: %d\n",  m_unNumChannels);
    fprintf( stderr, "          Block size: %d\n",  m_wBlockSize);
    fprintf( stderr, "  Device buffer size: %lu\n", m_ulDeviceBufferSize);
    fprintf( stderr, "  Supports GETOSPACE: %d\n",  !m_bGetOSpaceFailed);
    fprintf( stderr, "  Supports GETODELAY: %d\n",  !m_bGetODelayFailed);
#endif
    return RA_AOE_NOERR;
}

void CAudioOutLinux::_SyncUpTimeStamps(ULONG32 lCount)
{
    int bytes2  = 0;
    int theErr = -1;
    if( m_bTestGetODelay || !m_bGetODelayFailed )
    {
        HX_ASSERT(m_nDevID );
        theErr = ::ioctl(m_nDevID, SNDCTL_DSP_GETODELAY, &bytes2);
    }
    if( theErr != -1)
    {
        if (m_bTestGetODelay && (bytes2 != 0))
        {
            // We've now seen SNDCTL_DSP_GETODELAY return
            // a non-zero value so we know it is working
            m_bTestGetODelay = FALSE;
            m_bGetODelayFailed = FALSE;
        }

        if (!m_bTestGetODelay)
        {
            m_ulLastBytesPlayed = (UINT64)(m_ulTotalWritten+lCount-bytes2);
            m_ulLastTimeStamp   = GetTickCount();
        }
    }
    else
    {
        //so we don't try it again.
        m_bGetODelayFailed = TRUE;
        m_bTestGetODelay = FALSE;
    }
}


//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written. 
HX_RESULT CAudioOutLinux::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
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
HX_RESULT CAudioOutLinux::_OpenAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    //Set the tick count to zero
    m_ulTickCount       = 0;
    m_ulLastTimeStamp   = 0;
    m_ulLastBytesPlayed = 0;
    m_ulPausePosition   = 0;

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
        SafeStrCpy( szDevName, "/dev/dsp", MAX_DEV_NAME );
    }
    
    // Open the audio device if it isn't already open
    if ( m_nDevID < 0 )
    {
        m_nDevID = ::open( szDevName, O_WRONLY );
    }
    
    if ( m_nDevID < 0 )
    {
#ifdef _DEBUG        
        fprintf( stderr, "Failed to open audio(%s)!!!!!!! Code is: %d  errno: %d\n",
                 szDevName, m_nDevID, errno );
#endif        
        
        //Error opening device.
        retCode = RA_AOE_BADOPEN;
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutLinux::_CloseAudio()
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

HX_RESULT CAudioOutLinux::_OpenMixer()
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
            SafeStrCpy( szDevCtlName , "/dev/mixer", MAX_DEV_NAME );    // default for volume
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

HX_RESULT CAudioOutLinux::_CloseMixer()
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
HX_RESULT CAudioOutLinux::_Reset()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    m_ulPausePosition = 0;
    
    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else if( ::ioctl (m_nDevID, SOUND_PCM_RESET, 0) == -1 )
    {
        retCode = RA_AOE_GENERAL;
    }

    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to get/set the devices current volume.
UINT16 CAudioOutLinux::_GetVolume() const
{
    int nVolume      = 0;
    int nRetVolume   = 0;
    int nLeftVolume  = 0;
    int nRightVolume = 0;
    
    if (::ioctl( m_nMixerID, MIXER_READ(HX_VOLUME), &nVolume) < 0)
    {
        nRetVolume = 0;
    }
    nLeftVolume  = (nVolume & 0x000000ff); 
    nRightVolume = (nVolume & 0x0000ff00) >> 8;

    //Which one to use? Average them?
    nRetVolume = nLeftVolume ;

    return nRetVolume; 
}

HX_RESULT CAudioOutLinux::_SetVolume(UINT16 unVolume)
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    int nNewVolume=0;

    //Set both left and right volumes.
    nNewVolume = (unVolume & 0xff) | ((unVolume &0xff) << 8);
    
    if (::ioctl( m_nMixerID, MIXER_WRITE(HX_VOLUME), &nNewVolume) < 0)
    {
        retCode = RA_AOE_NOTSUPPORTED;
    }

    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to drain a device. This should play the remaining
//bytes in the devices buffer and then return.
HX_RESULT CAudioOutLinux::_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }
    else if( ::ioctl (m_nDevID, SNDCTL_DSP_SYNC, 0) == -1 )
    {
        retCode = RA_AOE_GENERAL;
    }

    m_wLastError = retCode;
    return m_wLastError;
}



UINT64 CAudioOutLinux::_GetBytesActualyPlayed(void) const
{
    /* Get current playback position in device DMA. */
    int     bytes2 = 0;
    UINT64  ulTheAnswer = 0;

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
                ((CAudioOutLinux*)this)->_SyncUpTimeStamps();
                ulTick = GetTickCount();
            }
            
            ulTheAnswer = (UINT64)(m_ulLastBytesPlayed+
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
            ulTheAnswer = (UINT64)((float)(GetTickCount()-m_ulTickCount)*(float)m_unNumChannels/1000.0*m_unSampleRate*m_uSampFrameSize);
            ulTheAnswer += m_ulPausePosition;
        }
    }
    return  ulTheAnswer;
}


//this must return the number of bytes that can be written without blocking.
//Don't use SNDCTL_DSP_GETODELAY here as it can't compute that amount
//correctly.
HX_RESULT CAudioOutLinux::_GetRoomOnDevice(ULONG32& ulBytes) const
{
    HX_RESULT      retCode = RA_AOE_NOERR;
    audio_buf_info stBuffInfo;
    int            theErr=0;

    //What versions of the linux kernel do we want to support?
    if( m_bGetOSpaceFailed )
    {
        theErr = -1;
    }
    else
    {
        theErr = ::ioctl(m_nDevID, SNDCTL_DSP_GETOSPACE, &stBuffInfo);
    }

    if ( theErr != -1 )
    {
        ulBytes = stBuffInfo.bytes;
    }
    else
    {
        //So we just try it once.
        m_bGetOSpaceFailed = TRUE;
        ulBytes = m_ulDeviceBufferSize-(m_ulTotalWritten-_GetBytesActualyPlayed() );
    }

    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutLinux::_CheckFormat( const HXAudioFormat* pFormat )
{
    int   nBitsPerSample = pFormat->uBitsPerSample;
    int   ulTmp          = pFormat->ulSamplesPerSec;    
    int   nNumChannels   = pFormat->uChannels;
    float fTmp           = 0.0;

    HX_RESULT retCode = RA_AOE_NOERR;
    
    //Is the device already open?
    if( m_nDevID > 0 || RA_AOE_NOERR != _OpenAudio() )
    {
        retCode = RA_AOE_DEVBUSY;
        return retCode;
    }

    //See if the sample rate is supported.
    if (ioctl(m_nDevID, SOUND_PCM_WRITE_RATE, &ulTmp) == -1)
    {
        //Not quite the real error, but it is what we need to return.
        retCode = RA_AOE_DEVBUSY;
        goto donechecking;
    }

    if (ulTmp == 0)
    {
        /* 
         * Some drivers actually set the sample rate on the device, but
         * return 0 for the sample rate. On these platforms we just ignore
         * the return value and assume the sample rate is set to what was
         * requested.
         */

        ulTmp = pFormat->ulSamplesPerSec;
    }

    //The ESS 1688 Sound card (not the ESS Solo-1) will return sample
    //rates that are close but not quite the ones we asked for (the
    //ESS Solo-1 doesn't play mono). I have see freqs like 44194
    //instead of 44100 (.2%) and 7984 instead of 8000 (.6%).
    //So, if we are close enough just say it is OK. 
    //How about 1%?

    //XXXRGG: i810 gives sample rates like 43512 for 44100 (1.3%), how
    //about 2%?
    fTmp = (float)ulTmp/(float)pFormat->ulSamplesPerSec;
    if( fabs(1.0-fTmp) > .02 )
    {
        //It is NOT supported
        retCode = RA_AOE_BADFORMAT;
        goto donechecking;
    }

    //Check num channels.
    if (ioctl(m_nDevID, SOUND_PCM_WRITE_CHANNELS, &nNumChannels) == -1)
    {
        retCode = RA_AOE_DEVBUSY;
        goto donechecking;
    }
    else if ( nNumChannels != pFormat->uChannels )
    {
        retCode = RA_AOE_BADFORMAT;
        goto donechecking;
    }

    //Check the frame size.
    if (ioctl(m_nDevID, SNDCTL_DSP_SETFMT, &nBitsPerSample) == -1)
    {
        retCode = RA_AOE_DEVBUSY;
        goto donechecking;
    }
    else if ( nBitsPerSample != pFormat->uBitsPerSample )
    {
        retCode = RA_AOE_BADFORMAT;
        goto donechecking;
    }

    //Close the audio device.
  donechecking:
    _CloseAudio();
    m_wLastError = retCode;
    return retCode;
}


HX_RESULT CAudioOutLinux::_CheckSampleRate( ULONG32 ulSampleRate )
{
    ULONG32 ulTmp = ulSampleRate;

    m_wLastError = RA_AOE_NOERR;
    
    //Is the device already open?
    if( m_nDevID > 0 || RA_AOE_NOERR != _OpenAudio() )
    {
        m_wLastError = RA_AOE_DEVBUSY;
    }
    else
    {   
        //See if the sample rate is supported.
        if (ioctl(m_nDevID, SOUND_PCM_WRITE_RATE, &ulTmp) == -1)
        {
            //Not quite the real error, but it is what we need to return.
            m_wLastError = RA_AOE_DEVBUSY;
        }
        else if( ulSampleRate != ulTmp )
        {
            //It is NOT supported
            m_wLastError = RA_AOE_BADFORMAT;
        }
        
        _CloseAudio();
    }
    
    return m_wLastError;
}


HX_RESULT CAudioOutLinux::_Pause() 
{
    m_wLastError = HXR_OK;
    m_ulPausePosition = m_ulTotalWritten;
    m_ulTickCount = 0;
    m_ulLastTimeStamp   = 0;
    return m_wLastError;
}

HX_RESULT CAudioOutLinux::_Resume()
{
    m_wLastError = HXR_OK;

    if( m_ulTotalWritten > 0 )
    {
        m_ulTickCount = GetTickCount();
        m_ulLastTimeStamp = m_ulTickCount;
    }
    return m_wLastError;
}
