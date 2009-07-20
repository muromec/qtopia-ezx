/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudev.cpp,v 1.39 2009/01/22 21:27:26 sfu Exp $
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

#include "hxtypes.h"
#if defined( _WINDOWS ) || defined( _WIN32 )
#include "hlxclib/windows.h"
#include <mmsystem.h>
#endif /*defined( _WINDOWS ) || defined( _WIN32 )*/

#ifdef _UNIX
#include <signal.h>
#endif

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "hxresult.h"
#include "cbqueue.h"
#include "cpqueue.h"
#include "hxslist.h"


#include "hxcom.h"
#include "hxausvc.h"
#include "hxengin.h"
#include "ihxpckts.h"   

#include "timeval.h"

#include "hxaudev.h"
#include "hxprefutil.h"

#ifdef _UNIX
#include "hxprefs.h"
extern IHXPreferences* z_pIHXPrefs;
#endif

#if defined( _WINDOWS ) || defined( _WIN32 )
#  include "winaudio.h"
#  if defined (HELIX_FEATURE_DIRECT_SOUND)
#    include "hxaudevds.h"
#  endif
#endif


#if defined( _LINUX ) || defined ( _FREEBSD )
#  include "audlinux_oss.h"
#  if defined(HELIX_FEATURE_ESOUND)
#    include "audlinux_esound.h"
#  endif
#  if defined(HELIX_FEATURE_ALSA)
#    include "audlinux_alsa.h"
#  endif
#  if defined(ANDROID)
#    include "audAndroid.h"
#  endif
#endif

#if defined(HELIX_FEATURE_USOUND)
#include <usound.h>
#include "audusound.h"
#endif


#if defined( __QNXNTO__ )
#  include "audqnx.h"
#endif 
#if defined( _IRIX )
#include "audirix.h"
#endif
#if defined( _SUN ) && !defined( _SOLARIS )
#include "audsunos.h"
#endif
#if defined( _AIX ) 
#include "audaix.h"
#endif
#if defined( _SOLARIS )
#include "audSolaris.h"
#endif
#if defined( _MACINTOSH ) || defined( _MAC_UNIX )
#include "osxaudio.h"
#include "macaudio.h"
#endif
#if defined( _BEOS )
#include "audbeos.h"
#endif
#if defined( _HPUX )
#include "audhpux.h"
#endif

#if defined(_SYMBIAN)
#include "audsymbian.h"
#endif

#if defined(_OPENWAVE)
#include "audopwave.h"
#endif

#if defined(_BREW)
#include "audbrew.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif



/************************************************************************
 *  Method:
 *		IHXAudioDevice::~CHXAudioDevice()
 *	Purpose:
 *		Destructor. Clean up and set free.
 */
CHXAudioDevice::~CHXAudioDevice()
{
    HX_RELEASE(m_pAudioDevHook);
    HX_RELEASE(m_pDeviceResponse);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pContext);

    m_uMinVolume = 0;
    m_uMaxVolume = 0;
    m_uCurVolume = 0;
    m_uSampFrameSize = 0;
    m_ulCurrentTime = 0;
    m_ulLastSysTime = 0;
    m_ulGranularity = 0;
    m_pdevName = 0;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP CHXAudioDevice::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXAudioDevice), (IHXAudioDevice*)this },
            { GET_IIDHANDLE(IID_IHXOpaqueAudioDevice), (IHXOpaqueAudioDevice*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXAudioDevice*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioDevice::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioDevice::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXAudioDevice methods
 */

/************************************************************************
 *  Method:
 *		IHXAudioDevice::Create
 *	Purpose:
 *		Create the audio device.
 */
CHXAudioDevice* CHXAudioDevice::Create(IHXPreferences* pPrefs)
{
    CHXAudioDevice*	pAudioDevice = NULL;

   /*
    *   The Mac compiler has trouble w/ overly complicated expressions to the
    *   preprocessor (that's why the #if... #endif...  #if... #endif blocks).
    */
#if defined( _SUN ) && !defined( _SOLARIS )
    pAudioDevice = new CAudioOutSun;
#endif
#if defined( _AIX)
    pAudioDevice = new CAudioOutAIX;
#endif
#if defined( _SOLARIS )
    pAudioDevice = new CAudioOutSolaris();
#endif
    
#if defined(_LINUX) ||  defined(_FREEBSD) || defined(_NETBSD)
# if defined(HELIX_FEATURE_OLPC) || defined(HELIX_CONFIG_MOBLIN)
    UINT16 nSoundDriver = kALSA;
#elif defined(ANDROID)
    UINT16 nSoundDriver = kAndroidAudio;
# else
    UINT16 nSoundDriver = kOSS;
# endif

    ReadPrefUINT16(z_pIHXPrefs, "SoundDriver", nSoundDriver );
    
#if defined(HELIX_FEATURE_USOUND)
    //Only use USound if it is available. If it isn't, drop to
    //OSS for now.
    //XXXgfw we need a better way to let the user deciede what
    //to fall back to, ALSA for example.
    if( kUSound == nSoundDriver && !usound_is_available() )
    {
        nSoundDriver = kOSS;
    }
#endif        

    switch( nSoundDriver )
    {
#if defined(HELIX_FEATURE_ESOUND)
       case kESound:
           pAudioDevice = new CAudioOutESound();
           break;
#endif        
#if defined(HELIX_FEATURE_USOUND)
       case kUSound:
               pAudioDevice = new CAudioOutUSound();
           break;
#endif        
#if defined(HELIX_FEATURE_ALSA)
       case kALSA:
           pAudioDevice = new CAudioOutLinuxAlsa();
           break;
#endif
#if defined(ANDROID)
       case kAndroidAudio:
           pAudioDevice = new CAudioOutAndroid();
           break;
#endif
       case kOSS:              //fall through
       case kSupportForOldOSS: //fall through
       default:
           //If SoundDriver is set incorrectly or the support for that
           //setting is not compiled we default to OSS drivers which
           //is currently always compiled in.
           pAudioDevice = new CAudioOutLinux();
           break;
    }
    
#endif
#if defined( __QNXNTO__ )
    pAudioDevice = new CAudioOutQNX;
#endif 
#if defined( _IRIX )
    pAudioDevice = new CAudioOutIrix;
#endif 
#if defined( _BEOS )
    pAudioDevice = new CAudioOutBeOS;
#endif
#if defined(_SYMBIAN)
    pAudioDevice = CAudioOutSymbian::NewL();
#endif    
#if defined(_OPENWAVE)
    // currently, audio unsupported for arm
#ifndef _OPENWAVE_ARMULATOR
    pAudioDevice = new CAudioOutOpenwave;
#endif
#endif
#if defined(_BREW)
    pAudioDevice = new CAudioOutBrew;
#endif

#if defined( _WINDOWS )
    
#if defined (HELIX_FEATURE_DIRECT_SOUND)
#if !defined(HELIX_CONFIG_MINIMIZE_SIZE)    
    HXBOOL bUseDS = TRUE;
    if(pPrefs)
    {
        IHXBuffer* pBuff = NULL;
        pPrefs->ReadPref("UseDirectSound", pBuff);
        if (pBuff)
        {
            bUseDS = (::atoi((const char*)pBuff->GetBuffer()) == 1);
            HX_RELEASE(pBuff);
        }
    }

    if(bUseDS)
        pAudioDevice = new CHXAudioDeviceDS;
    else
        pAudioDevice = new CAudioOutWindows;
#  else
    pAudioDevice = new CHXAudioDeviceDS;    
#  endif    
#else
    pAudioDevice = new CAudioOutWindows;
#endif //HELIX_FEATURE_DIRECT_SOUND

#endif


#if defined( _MACINTOSH ) || defined( HELIX_FEATURE_AUDIO_OUT_LEGACY_MAC )
    pAudioDevice = new CAudioOutMac;
#elif defined( _MAC_UNIX )
    pAudioDevice = new CAudioOutOSX;
#endif
#if defined _HPUX
    pAudioDevice = new CAudioOutHPUX;
#endif
    return( pAudioDevice );
}

void	
CHXAudioDevice::Init(IUnknown* pContext)
{
    HX_ASSERT(m_pContext == NULL && pContext != NULL);
    HX_RELEASE(m_pContext);
    
    m_pContext = pContext;
    
    if (m_pContext)
    {
        m_pContext->AddRef();
        _initAfterContext();

	m_pContext->QueryInterface(IID_IHXAudioDeviceHookManager, (void **)&m_pAudioDevHook);
    }

    _Imp_Init(pContext);
}

void CHXAudioDevice::_initAfterContext()
{}


/************************************************************************
 *  Method:
 *		IHXAudioDevice::Open
 *	Purpose:
 *		Open the audio device using the given format.
 */
STDMETHODIMP CHXAudioDevice::Open
(
    const HXAudioFormat*	pAudioFormat,
    IHXAudioDeviceResponse*	pDeviceResponse
)
{
    HX_RESULT theErr = HXR_OK;

    m_pDeviceResponse = pDeviceResponse;

    if (m_pDeviceResponse)
    {
        m_pDeviceResponse->AddRef();
    }

    memcpy( &m_AudioFmt, pAudioFormat, sizeof(HXAudioFormat) ); /* Flawfinder: ignore */
    theErr = _Imp_Open( pAudioFormat );
    
    if (!theErr)
    {
        m_eState = E_DEV_OPENED;
    }

    return theErr;
}

/************************************************************************
 *  Method:
 *		IHXOpaqueAudioDevice::Open
 *	Purpose:
 *		Open the audio device using the given format in opaque mode
 */
HX_RESULT CHXAudioDevice::Open(
    const HXAudioFormat*    /*IN*/ pAudioFormat, 
    IHXAudioDeviceResponse* /*IN*/ pDeviceResponse, 
    const char*		    /*IN*/ pszOpaqueType, 
    IHXBuffer*		    /*IN*/ pOpaqueData)
{
    HX_RESULT theErr = HXR_OK;

    m_pDeviceResponse = pDeviceResponse;

    if (m_pDeviceResponse)
    {
        m_pDeviceResponse->AddRef();
    }

    memcpy( &m_AudioFmt, pAudioFormat, sizeof(HXAudioFormat) ); /* Flawfinder: ignore */
    theErr = _Imp_OpaqueOpen( pAudioFormat, pszOpaqueType, pOpaqueData );
    
    if (!theErr)
    {
        m_eState = E_DEV_OPENED;
    }

    return theErr;
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::Close
 *	Purpose:
 *  	Close the audio device. 
 */
STDMETHODIMP CHXAudioDevice::Close
( 
    const 	HXBOOL	bFlush
)
{
    HX_RESULT 	theErr = HXR_OK;

    AddRef();

    if ( bFlush )
    {
        theErr = Reset();
    }
    else
    {
        theErr = Drain();
    }
    if ( !theErr )
        theErr = _Imp_Close();
    
    m_eState = E_DEV_CLOSED;

    Release();
    
    return HXR_OK;
}

HX_RESULT CHXAudioDevice::Seek( UINT32  ulSeekTime)
{
    _Imp_Seek(ulSeekTime);
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *		IHXAudioDevice::Pause
 *	Purpose:
 *		Pause the audio device.
 */
STDMETHODIMP CHXAudioDevice::Pause
(
)
{
    HX_RESULT 	theErr = HXR_OK;

    AddRef();

    m_bPaused = TRUE;

    theErr = _Imp_Pause();

    m_eState = E_DEV_PAUSED;

    Release();

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::Resume
 *	Purpose:
 *		Resume the audio device.
 */
STDMETHODIMP CHXAudioDevice::Resume
(
)
{
    HX_RESULT 	theErr = HXR_OK;

    AddRef();

    m_bPaused = FALSE;

    theErr = _Imp_Resume();

    m_eState = E_DEV_RESUMED;

    Release();

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::Write
 *	Purpose:
 *		Write data to the audio device.
 */
STDMETHODIMP CHXAudioDevice::Write
(
    const HXAudioData*	pAudioHdr
)
{
    HX_RESULT theErr = _Imp_Write( pAudioHdr );
    return theErr;
}


/************************************************************************
 *  Method:
 *		IHXAudioDevice::SetCallback
 *	Purpose:
 */
/*
STDMETHODIMP CHXAudioDevice::SetCallback
( 
)
{
    return HXR_OK;

}
*/

/************************************************************************
 *  Method:
 *		IHXAudioDevice::SetBuffering
 *	Purpose:
 *		Let the Audio Device manage the audio buffers.
 */
STDMETHODIMP CHXAudioDevice::SetBuffering
(
    const HXBOOL	bSetBuffering
)
{
    m_bBuffer = bSetBuffering;
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::InitVolume
 *	Purpose:
 *		Initialize the volume. Return TRUE if device supports volume.
 */
STDMETHODIMP_(HXBOOL) CHXAudioDevice::InitVolume
(
    const UINT16    uMinVolume,
    const UINT16    uMaxVolume
)
{
    m_uMinVolume = uMinVolume;
    m_uMaxVolume = uMaxVolume;

    return (_Imp_SupportsVolume());
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::SetVolume
 *	Purpose:
 *		Set the volume.
 */
STDMETHODIMP CHXAudioDevice::SetVolume 
(
    const UINT16    uVolume
)
{
    m_uCurVolume = uVolume;
    return _Imp_SetVolume( uVolume );
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::GetVolume
 *	Purpose:
 *		Get the volume.
 */
STDMETHODIMP_(UINT16) CHXAudioDevice::GetVolume()
{
    return _Imp_GetVolume();
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::Reset
 *	Purpose:
 *		Reset audio. Stop playback and flush all buffers.
 */
STDMETHODIMP CHXAudioDevice::Reset ()
{
    return( _Imp_Reset() );
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::Drain
 *	Purpose:
 *		Drain remaining audio with playback. 
 */
STDMETHODIMP CHXAudioDevice::Drain()
{
    return( _Imp_Drain() );
}

/************************************************************************
 *  Method:
 *              IHXAudioDevice::CheckFormat
 *      Purpose:
 *		Check to see if the audio device will accept this audio
 *		format.
 */
STDMETHODIMP CHXAudioDevice::CheckFormat( const HXAudioFormat* pFormat )
{
    return( _Imp_CheckFormat( pFormat ) );
}

/************************************************************************
 *  Method:
 *		IHXAudioDevice::GetCurrentAudioTime
 *	Purpose:
 *		Get the current time from the audio device.
 *              We added this to support the clock available in the
 *              Window's audio driver.
 */
STDMETHODIMP CHXAudioDevice::GetCurrentAudioTime 
(
    ULONG32&	ulCurrentTime
)
{
    return _Imp_GetCurrentTime( ulCurrentTime );
}

/************************************************************************
 *  Method:
 *		CHXAudioDevice::GetAudioFd
 *	Purpose:
 */
STDMETHODIMP CHXAudioDevice::GetAudioFd()
{
	return _Imp_GetAudioFd();
}

/************************************************************************
 *  Method:
 *		CHXAudioDevice::OnTimeSync
 *	Purpose:
 *		Get the audio time from the platform specific function
 *		GetCurrentAudioTime() and pass this up to the audio
 *		session.
 */
HX_RESULT    CHXAudioDevice::OnTimeSync()
{
    HX_RESULT theErr = HXR_OK;
    // Ignore this if we're paused!
    if (!m_bPaused)
    {
        ULONG32 ulAudioTime = 0;
        theErr = _Imp_GetCurrentTime( ulAudioTime );

        if (m_pDeviceResponse)
        {
	    // Call to OnTimeSync can kill audio device.
	    // We need to either self-addref or make sure member variables
	    // are not accessed after OnTimeSync call.
            theErr = m_pDeviceResponse->OnTimeSync(ulAudioTime);
        }
    }
    return theErr;
}


UINT16 CHXAudioDevice::NumberOfBlocksRemainingToPlay()
{
    return _NumberOfBlocksRemainingToPlay();
}

HXBOOL CHXAudioDevice::IsWaveOutDevice()
{
    return _IsWaveOutDevice();
}

HXBOOL CHXAudioDevice::_IsWaveOutDevice()
{
    return TRUE;
}

/*************************************************************************
 * Test code..
 *
 */ 
#ifdef _TESTING_STR

#include <stdio.h>
#ifndef _WINDOWS
#include <unistd.h>             // for getopt()
#else
#include <io.h>			// for open()
#define O_NONBLOCK  0
#endif
#include <stdlib.h>             // for atoi()
#include <sys/types.h>          // for open()
#include <sys/stat.h>           // for open()
#include <fcntl.h>                      // for open()

#define INITGUID 1

#include "hxcom.h"
#include "hxausvc.h"
#include "hxaudev.h"

int opt_debug = 0xff;

int process_id(void)
{
return 10; 
}

#define BLOCK_SIZE  3176

void main( int argc, char **argv )
{
	CHXAudioDevice* mydev = 0;
	HX_RESULT theErr = HXR_OK;

	mydev = CHXAudioDevice::Create();

	printf("main: created: %x\n", mydev);

	HXAudioFormat audioFmt;
	audioFmt.uChannels = 1;
	audioFmt.uBitsPerSample= 16;
	audioFmt.ulSamplesPerSec= 11025; //44100; // 22050;  //11025;
	audioFmt.uMaxBlockSize = BLOCK_SIZE;
	
	printf("main: Format:\n");
	printf("main: Channels:         %d\n", audioFmt.uChannels );
	printf("main: Bits Per Sample   %d\n", audioFmt.uBitsPerSample);
	printf("main: SamplesPerSec     %d\n", audioFmt.ulSamplesPerSec);
	printf("main: MaxBlockSize      %d\n", audioFmt.uMaxBlockSize);
	printf("main: Check Format: err: %d\n", theErr);

        theErr = mydev->CheckFormat( &audioFmt );

	if ( !theErr )
	{
		IHXAudioDeviceResponse*        playerResponse = 0;
		theErr = mydev->Open( &audioFmt, playerResponse );
	}

	if ( !theErr )
	{
		theErr = mydev->Pause();
		theErr = mydev->Resume();
	}

	HXAudioData audioHdr;
	if ( !theErr )
	{
		audioHdr.ulAudioTime = 66;

		// Initialize the volume to 0,100
		if ( mydev->InitVolume( HX_MIN_VOLUME, HX_MAX_VOLUME ) )
			printf("main: SupportsVolume is TRUE\n");
		else
			printf("main: SupportsVolume is FALSE\n");

		UINT16	curVol = 0;
		theErr = mydev->GetVolume( curVol );
		printf("main: GetVolume: %d\n", curVol);

		UINT16	newVol = HX_INIT_VOLUME;
		theErr = mydev->SetVolume( newVol );
		printf("main: SetVolume: %d\n", newVol);

		curVol = 0;
		theErr = mydev->GetVolume( curVol );
		printf("main: GetVolume: %d\n", curVol);
	}

	// Write some PCM to the device...
	if ( !theErr )
	{
int cnt = 0;
		unsigned char buf[BLOCK_SIZE]; /* Flawfinder: ignore */
		if ( argc > 1 )
		{
			int fd;
			*argv++;
			printf("main: Playing: %s \n", *argv);
			fd = open( *argv,O_RDONLY | O_NONBLOCK ); /* Flawfinder: ignore */
			if ( fd > 0 )
			{
				int nbytes = 0;
				audioHdr.pData = (UCHAR*) new char [BLOCK_SIZE ]; 
				while( (nbytes = read( fd, buf, BLOCK_SIZE)) > 0 ) /* Flawfinder: ignore */
				{
					memcpy(audioHdr.pData,buf, nbytes); /* Flawfinder: ignore */
					// play silence memset(audioHdr.pData, 0, nbytes);
					audioHdr.ulBufLen = nbytes;
        				theErr = mydev->Write( &audioHdr );
cnt++;
				}
			}
		}
printf("Played %d blocks of %d bytes = %d\n", cnt, BLOCK_SIZE, cnt * BLOCK_SIZE);
		for (int i = 0; i<6; i++)
		{
			memset(buf,0,BLOCK_SIZE);
			memcpy(audioHdr.pData,buf, BLOCK_SIZE); /* Flawfinder: ignore */
			audioHdr.ulBufLen = BLOCK_SIZE;
        		theErr = mydev->Write( &audioHdr );
		}
	}

	if ( !theErr )
		theErr = mydev->Close(TRUE);
}

#endif // _TESTING

