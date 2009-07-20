/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audlinux_esound.cpp,v 1.3 2005/03/14 19:43:22 bobclark Exp $
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

//===================================================================================
// 
//   	audESound.cpp
// 
//   	CLASS: CAudioOutESound
// 	
//   	Implements the sound subsystem for the Enlightenment Sound
//      Deamon.
//
//===================================================================================

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h> 
#include <dlfcn.h>


#include "audlinux_esound.h"
#include "ihxpckts.h"
#include "hxtick.h"

//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
CAudioOutESound::CAudioOutESound() :
    CAudioOutUNIX(),
    m_nDevID(NO_FILE_DESCRIPTOR),
    m_nESoundServerID(NO_FILE_DESCRIPTOR),
    m_nESoundPlayerID(NO_FILE_DESCRIPTOR),
    m_ulTickCount(0),
    m_ulPausePosition(0),
    m_strRealplayerName(""),
    m_pESDLib(NULL),
    m_fpESDPlayStream(NULL),
    m_fpESDGetAllInfo(NULL),
    m_fpESDFreeAllInfo(NULL),
    m_fpESDClose(NULL),
    m_fpESDSetStreamPan(NULL),
    m_fpESDAudioFlush(NULL),
    m_fpESDOpenSound(NULL)
{
    //Construct a proccess specific name to register with ESD.
    m_strRealplayerName.Format( "%s-%d", "realplayer", getpid() );
};

CAudioOutESound::~CAudioOutESound()
{
    HX_DELETE( m_pESDLib );
};

//-------------------------------------------------------
// These Device Specific methods must be implemented 
// by the platform specific sub-classes.
//-------------------------------------------------------
INT16 CAudioOutESound::_Imp_GetAudioFd(void)
{
    return m_nDevID;
}

//Devic specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT CAudioOutESound::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    if ( m_nESoundServerID < 0 )
        return RA_AOE_DEVNOTOPEN;

    //We can only do this once after opening the audio device.
    if( m_nDevID != NO_FILE_DESCRIPTOR )
        return RA_AOE_DEVBUSY;
    
    // Open the audio device if it isn't already open
    esd_format_t format = ESD_STREAM | ESD_PLAY;

    //Set steareo or mono
    if( 2 == pFormat->uChannels )
    {
        format |= ESD_STEREO;
    }
    else
    {
        format |= ESD_MONO;
    }
    
    //Now set the format. Either 8-bit or 16-bit audio is supported.
    if( pFormat->uBitsPerSample == 16)
    {
        format |= ESD_BITS16;
    }
    else
    {
        format |= ESD_BITS8;
    }

    //Now open our connection with ESD on the local host.
    m_nDevID = m_fpESDPlayStream( format, pFormat->ulSamplesPerSec, NULL, (const char *)m_strRealplayerName);
    
    if ( m_nDevID < 0 )
    {
#ifdef _DEBUG        
        fprintf( stderr, "Failed to open audio!!!!!!! Code is: %d  errno: %d\n",
                 m_nDevID, errno );
#endif        
        
        //Error opening device.
        return RA_AOE_DEVNOTOPEN;
    }

    m_wBlockSize         = m_ulBytesPerGran;
    m_unNumChannels      = pFormat->uChannels;
    m_unSampleRate       = pFormat->ulSamplesPerSec;
    m_ulDeviceBufferSize = ESD_BUF_SIZE*4;
    m_uSampFrameSize     = pFormat->uBitsPerSample/8;

    //Now, here is the tricky part. We must get a list of players from
    //the esd deamon and interate through them until we find our self.
    //Was we find us we need to store the ID for later use in setting
    //the volume (panning).
    //n
    // From esd.h:
    //
    //      typedef struct esd_info {
    //        
    //          esd_server_info_t *server;
    //          esd_player_info_t *player_list;
    //          esd_sample_info_t *sample_list;
    //
    //      } esd_info_t;
    //
    //  typedef struct esd_player_info {
    //      struct esd_player_info *next; /* point to next entry in list */
    //      esd_server_info_t *server;	/* the server that contains this stream */
    //      int source_id;		/* either a stream fd or sample id */
    //      char name[ ESD_NAME_MAX ];	/* name of stream for remote control */
    //      int rate;			/* sample rate */
    //      int left_vol_scale;		/* volume scaling */
    //      int right_vol_scale;
    //      esd_format_t format;	/* magic int with the format info */
    //  } esd_player_info_t;

    esd_player_info_t *pPlayerInfo = _GetPlayerInfo();
    if( NULL == pPlayerInfo )
    {
        return RA_AOE_GENERAL;
    }

    m_nESoundPlayerID = pPlayerInfo->source_id;
    HX_DELETE( pPlayerInfo );

    if( m_nESoundPlayerID == NO_FILE_DESCRIPTOR )
    {
#ifdef _DEBUG        
        fprintf( stderr, "Can't find the realaudio stream in the ESD server list.\n");
#endif
        return RA_AOE_GENERAL;
    }

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

//FREE THE RETURNED POINTER!!!!!!!!!!!!!!!
esd_player_info_t* CAudioOutESound::_GetPlayerInfo() const
{
    
    esd_info_t       *pServerInfo  = NULL;
    esd_player_info_t *pPlayerInfo = NULL;
    esd_player_info_t *pRetVal     = NULL;

    pServerInfo = m_fpESDGetAllInfo(m_nESoundServerID);
    if( pServerInfo == NULL )
    {
#ifdef _DEBUG        
        fprintf( stderr, "Can't get server info from ESD.\n");
#endif
        return NULL;
    }
    if( pServerInfo->player_list == NULL )
    {
#ifdef _DEBUG        
        fprintf( stderr, "There seem to be no players connected to esd server.\n");
#endif
        m_fpESDFreeAllInfo( pServerInfo );
        return NULL;
    }
    pPlayerInfo = pServerInfo->player_list;
    while( pPlayerInfo )
    {
        if( strcmp( pPlayerInfo->name, (const char *)m_strRealplayerName ) == 0 )
        {
            //found it.
            break;
        }
        pPlayerInfo = pPlayerInfo->next;
    }

    //We found it. Make a new one and copy.
    pRetVal = new esd_player_info_t(*pPlayerInfo);

    if( NULL == pRetVal )
    {
        //OOps.
        return NULL;
    }
    
    pRetVal->next    = NULL; //Don't even think about it.
    pRetVal->server  = NULL;
//      pRetVal->source_id = pPlayerInfo->source_id;
//      strcpy(pRetVal->name, pPlayerInfo->name );
//      pRetVal->rate = pPlayerInfo->rate;
//      pRetVal->left_vol_scale = pPlayerInfo->left_vol_scale;
//      pRetVal->right_vol_scale = pPlayerInfo->right_vol_scale
//      pRetVal->format = pPlayerInfo->format;
    
    //Free the info struct.
    m_fpESDFreeAllInfo( pServerInfo );

    return pRetVal;
}

//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written. 
HX_RESULT CAudioOutESound::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
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
        
    }
    return retCode;
}

//XXXgfw, since we have to open the audio device in ESD with
//all the format information (no way to change it once open)
//we will just return OK here and do the actual open in 
//SetDeviceConfig() call. In this call we will just open
//our connectin to the ESD server that we use for changing
//the volume of our stream (panning).
HX_RESULT CAudioOutESound::_OpenAudio()
{
    
    HX_RESULT retCode = RA_AOE_NOERR;
    
    //Set the tick count to zero
    m_ulTickCount = 0;
    m_ulPausePosition = 0;

    // create DLLAccess object
    m_pESDLib = new DLLAccess();

    if((DLLAccess::DLL_OK==m_pESDLib->open("libesd.so", DLLTYPE_NOT_DEFINED))||
       (DLLAccess::DLL_OK==m_pESDLib->open("libesd.so.0", DLLTYPE_NOT_DEFINED)))
    {
        m_fpESDPlayStream = (ESDPlayStreamType)m_pESDLib->getSymbol("esd_play_stream");
        if( dlerror() != NULL )
            retCode = RA_AOE_DEVNOTOPEN;
        m_fpESDGetAllInfo = (ESDGetAllInfoType)m_pESDLib->getSymbol("esd_get_all_info");
        if( dlerror() != NULL )
            retCode = RA_AOE_DEVNOTOPEN;
        m_fpESDFreeAllInfo = (ESDFreeAllInfoType)m_pESDLib->getSymbol("esd_free_all_info");
        if( dlerror() != NULL )
            retCode = RA_AOE_DEVNOTOPEN;
        m_fpESDClose = (ESDCloseType)m_pESDLib->getSymbol("esd_close");
        if( dlerror() != NULL )
            retCode = RA_AOE_DEVNOTOPEN;
        m_fpESDSetStreamPan = (ESDSetStreamPanType)m_pESDLib->getSymbol("esd_set_stream_pan");
        if( dlerror() != NULL )
            retCode = RA_AOE_DEVNOTOPEN;
        m_fpESDAudioFlush = (ESDAudioFlushType)m_pESDLib->getSymbol("esd_audio_flush");
        if( dlerror() != NULL )
            retCode = RA_AOE_DEVNOTOPEN;
        m_fpESDOpenSound = (ESDOpenSoundType)m_pESDLib->getSymbol("esd_open_sound");
        if( dlerror() != NULL )
            retCode = RA_AOE_DEVNOTOPEN;
    }
    else
    {
#ifdef _DEBUG        
        //Can't load the ESD shared lib. Tell the user.
        fprintf( stderr, "The ESD library, libesd.so, could not be loaded.\n");
        fprintf( stderr, "Please install ESD, disable ESD support or locate\n" );
        fprintf( stderr, "the missing library.\n");
#endif
        retCode = RA_AOE_DEVNOTOPEN;
    }
    
    //Open the ESD server on the localhost.
    if( RA_AOE_NOERR == retCode )
    {
        m_nESoundServerID = m_fpESDOpenSound( NULL );
        if( m_nESoundServerID == -1 )
        {
#ifdef _DEBUG        
            fprintf( stderr, "The ESD server could not be located on localhost.\n");
            fprintf( stderr, "Either disable ESD support or start the esd daemon.\n");
#endif        
            retCode = RA_AOE_DEVNOTOPEN;
        }
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}


HX_RESULT CAudioOutESound::_CloseAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if( m_nDevID >= 0 )
    {
        //close the esd server
        m_fpESDClose( m_nESoundServerID );
        m_nESoundServerID = NO_FILE_DESCRIPTOR;

        //Close the esd player FD.
        ::close( m_nDevID );
        m_nDevID = NO_FILE_DESCRIPTOR;
    }
    else
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }

    m_ulPausePosition = 0;
    m_ulTickCount     = 0;
    
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutESound::_OpenMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    //ESD always has volume support.
    m_bMixerPresent = 1;
    _Imp_GetVolume();
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Do nothing under ESD.
HX_RESULT CAudioOutESound::_CloseMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to reset device and return it to a state that it 
//can accept new sample rates, num channels, etc.
//Can't really do anything here under ESD.
HX_RESULT CAudioOutESound::_Reset()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }

    m_ulPausePosition = 0;
    
    m_wLastError = retCode;
    return m_wLastError;
}

//Device specific method to get/set the devices current volume.
UINT16 CAudioOutESound::_GetVolume() const
{
    int nRetVolume   = 0;

    if( m_nESoundPlayerID > 0 )
    {
        //We have been added to the esd server list and can report
        //volume.
        esd_player_info_t *pPlayer = _GetPlayerInfo();
        if( NULL != pPlayer )
        {
            //Choose either the left or right?
            nRetVolume = pPlayer->left_vol_scale;
            HX_DELETE( pPlayer );
        }
    }
    else
    {
        //ESD always starts out an app at 256. ESD_VOLUME_BASE.
        nRetVolume = ESD_VOLUME_BASE;
    }

    //Map device specific volume levels to [0,100]. 
    nRetVolume = (int) ((float)nRetVolume/256.0*100.0+0.5);

    return nRetVolume; 
}

HX_RESULT CAudioOutESound::_SetVolume(UINT16 unVolume)
{
    HX_RESULT retCode = RA_AOE_NOERR;

    //Map incoming [0..100] volume to ESD_VOLUME_BASE.
    unVolume = (int)((float)unVolume/100.0 * (float)ESD_VOLUME_BASE + 0.5 );

    if( m_nESoundPlayerID > 0 )
    {
        //We have been added to the esd server list and can set
        //volumes.
        m_fpESDSetStreamPan( m_nESoundServerID, m_nESoundPlayerID, unVolume, unVolume);
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
HX_RESULT CAudioOutESound::_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_nDevID < 0 )
    {
        retCode = RA_AOE_DEVNOTOPEN;
    }

    m_fpESDAudioFlush();
    
    m_wLastError = retCode;
    return m_wLastError;
}



UINT64 CAudioOutESound::_GetBytesActualyPlayed(void) const
{
    UINT64 ulBytes2 = 0;

    if( m_ulTotalWritten > 0 )
    {
        ulBytes2 = ((double)((GetTickCount()-m_ulTickCount)*m_unNumChannels)/(double)1000*m_unSampleRate*m_uSampFrameSize);
        ulBytes2 += m_ulPausePosition;
    }

    return  ulBytes2;
}


//this must return the number of bytes that can be written without blocking.
//Don't use SNDCTL_DSP_GETODELAY here as it can't compute that amount
//correctly.
HX_RESULT CAudioOutESound::_GetRoomOnDevice(ULONG32& ulBytes) const
{
    HX_RESULT retCode     = RA_AOE_NOERR;

    //XXXgfw :-( This is going to suck if they don't use threads...
    ulBytes = m_wBlockSize;
//    ulBytes = m_ulDeviceBufferSize-(m_ulTotalWritten-_GetBytesActualyPlayed() );

    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutESound::_CheckFormat( const HXAudioFormat* pFormat )
{
    m_wLastError = RA_AOE_NOERR;
    return m_wLastError;
}


HX_RESULT CAudioOutESound::_CheckSampleRate( ULONG32 ulSampleRate )
{
    //ESD supposidly converts any format to one the matches the 
    //installed hardware the best. ESD does the conversion for
    //us. So, Just return OK.
    m_wLastError = RA_AOE_NOERR;
    return m_wLastError;
}
 

HX_RESULT CAudioOutESound::_Pause() 
{
    m_wLastError = HXR_OK;
    m_ulPausePosition = m_ulTotalWritten;
    m_ulTickCount = 0;
    return m_wLastError;
}

HX_RESULT CAudioOutESound::_Resume()
{
    m_wLastError = HXR_OK;

    if( m_ulTotalWritten > 0 )
        m_ulTickCount = GetTickCount();
    
    return m_wLastError;
}

HXBOOL CAudioOutESound::_IsSelectable() const
{
    return TRUE;
}


HXBOOL CAudioOutESound::_HardwarePauseSupported() const
{
    return TRUE;
}


