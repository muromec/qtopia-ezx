/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: winaudio.cpp,v 1.9 2006/09/10 00:02:10 milko Exp $
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

#include "hxtypes.h"

#include "hlxclib/windows.h"
#include <mmsystem.h>
#include <tchar.h> 
#include <stdio.h>

#ifdef _TESTING
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined (_WINDOWS) || defined (_WIN32)

#include <io.h>

#endif

#endif

#include "hxresult.h"

#include "hxcom.h"
#include "hxausvc.h"
#include "auderrs.h"
#include "ihxpckts.h"
#include "hxengin.h"

#include "timeval.h"

#include "hxaudev.h"
#include "hxslist.h"
#include "hxtick.h"
#include "pckunpck.h"
#include "cbqueue.h"
#include "cpqueue.h"
#include "hxthread.h"
#include "hxtlogutil.h"

#include "winaudio.h"

#ifdef WIN32_PLATFORM_PSPC
#define WM_NCCREATE WM_CREATE
#define WM_NCDESTROY WM_DESTROY
#endif

struct IHXCallback;

extern HINSTANCE g_hInstance;
#define	OFFSET_THIS		0

#ifdef _TESTING
int m_audfile = -1;
#endif

HXBOOL	CAudioOutWindows::zm_bVolSupport = FALSE;
HXBOOL	CAudioOutWindows::zm_bLRVolSupport = FALSE;
WORD	CAudioOutWindows::zm_uMaxVolume = 100;
HXBOOL	CAudioOutWindows::zm_bMixerVolSupport = FALSE;
HXBOOL	CAudioOutWindows::zm_bMixerVolSupportChecked = FALSE;
UINT	CAudioOutWindows::zm_uDestroyMessage = 0;
HXBOOL	CAudioOutWindows::zm_bClosed = TRUE;
CAudioOutWindows* zm_pCurrentAudioDevice = NULL;

// BAD drivers which need to call waveOutSetVolume directly
const UINT16	g_nBadDrivers = 1;
const TCHAR*	g_badDrivers[] = { _T("Crystal Audio System") };
audioDevice	CAudioOutWindows::zm_audioDevice = HXAUDIO_UNKNOWN;

//CRITICAL_SECTION CAudioOutWindows::zm_AudioCritSection;

#define MAX_REASONABLE_BUFFS 40

#define PUSH_DOWN_TIME		400    /* push down 400 ms  */
#define LIKELY_PUSH_COUNT	10

CAudioOutWindows::CAudioOutWindows()
    : m_hWave(NULL)
    , m_unAllocedBufferCnt(0)
    , m_unAllocedBuffSize(0)
    , m_ppAllocedBuffers(NULL)
    , m_pWaveHdrs(NULL)
    , m_rAvailBuffers(MAX_REASONABLE_BUFFS)
    , m_bInitialized(FALSE)
    , m_bResetting(FALSE)
    , m_bIsFirstPacket(TRUE)
    , m_hWnd(NULL)
    , m_bClassRegistered(FALSE)
    , m_ulDevPosRollOver(0)
    , m_ulLastDeviceBytesPlayed(0)
    , m_ulLastDeviceSamplesPlayed(0)
    , m_llDeviceBytesPlayed(0)
    , m_llDeviceSamplesPlayed(0)
    , m_pMutex(NULL)
#if defined(_WIN32)
    , m_ulOriginalThreadId(0)
#endif /*_WIN32*/
{
    zm_bClosed = TRUE;    
    zm_pCurrentAudioDevice = this;

#if defined(_WIN32) && !defined(_WINCE)
    m_hMixer = NULL;  
    memset(&m_VolumeControlDetails, 0 , sizeof(MIXERCONTROLDETAILS));
#endif // _WIN32

    // Create a unique message for destroying the audio window
    if (!zm_uDestroyMessage)
    {
	zm_uDestroyMessage = RegisterWindowMessage(_T("HX_DestroyAudioServicesInternal"));
    }   
}		   


CAudioOutWindows::~CAudioOutWindows()
{
//    OutputDebugString("BEFORE CALL TO:CAudioOutWindows::~CAudioOutWindows\r\n");
    // this gives us one last chance to recover packets that are still in the device
    Reset();

    // We might as well consider the device closed!
    zm_bClosed = TRUE;

#if defined(_WIN32) && !defined(_WINCE)

    /* This sleep is added to fix a hang bug that ONLY happens on
     * Darren's machine if you adjust audio volume. His machine
     * has a really old audio driver
     * 
     * I have no clue what this bug is and how this Sleep(0)
     * fixes it. Obviously, there was some race condition.
     *
     * Sound Driver Info:
     *
     * Version 2.03.0 Build 1
     * Creative Sound Blaster 16 Driver (Windows NT)
     * 
     */
    Sleep(0);

    if(m_hMixer)
    {
        mixerClose(m_hMixer);
    }
#endif // _WIN32

    _Imp_Close();

    zm_pCurrentAudioDevice = NULL;
    HX_RELEASE(m_pMutex);
}

HX_RESULT
CAudioOutWindows::_Imp_Init(IUnknown* pContext)
{
    return CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
}

UINT16 CAudioOutWindows::_Imp_GetVolume()
{
    DWORD dwVol	    = 0;                          
    HXBOOL bSuccess   = FALSE;

#if defined(_WIN32) && !defined(_WINCE)
    if (!zm_bMixerVolSupportChecked)
    {
	CheckForVolumeSupport();
    }

    if(zm_bMixerVolSupport)
    {
	if (!m_hMixer)
	{
	    CheckForVolumeSupport();
	}

        PMIXERCONTROLDETAILS_UNSIGNED pmxVolume;
        UINT16 nItems = 1;
        if(m_VolumeControlDetails.cMultipleItems)
            nItems = (UINT16)m_VolumeControlDetails.cMultipleItems;
        pmxVolume = new MIXERCONTROLDETAILS_UNSIGNED[nItems];
        m_VolumeControlDetails.cbDetails = nItems * sizeof(MIXERCONTROLDETAILS_UNSIGNED);
        m_VolumeControlDetails.paDetails = pmxVolume;
        if(mixerGetControlDetails((HMIXEROBJ)m_hMixer, &m_VolumeControlDetails, 
           MIXER_GETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER) == MMSYSERR_NOERROR)
        {
            dwVol = pmxVolume[0].dwValue;
            bSuccess = TRUE;
        }
        delete[] pmxVolume;
    }
#endif

    if (!bSuccess)
    {
	if (!zm_bVolSupport)
	{
	    return 0;
	}

	MMRESULT hResult = waveOutGetVolume( m_hWave, &dwVol );

	if (hResult != MMSYSERR_NOERROR && m_hWave != NULL)
	{
	    hResult = waveOutGetVolume( NULL, &dwVol );
	}

	if (hResult != MMSYSERR_NOERROR)
	{
	    return 0;
	}
    }

    return ( (UINT16)((DWORD)LOWORD( dwVol ) * zm_uMaxVolume / 0xFFFF ) );
}

HX_RESULT CAudioOutWindows::_Imp_SetVolume
( 
    const UINT16 uVolume
)
{
    DWORD dwVol = 0;
    HXBOOL bSuccess = FALSE;

    if (uVolume > zm_uMaxVolume)
    {
        return HXR_OK;
    }

#if defined(_WINCE) && (_WIN32_WCE >= 400) && defined(_X86_)
	//Windows CE 4.2 Emulator (testing platform) does not seem to support waveOut
	//set volume functionality (on speakers). In some sense it does but only the
	//high volume value work, other values just mute the speakers
	dwVol = (DWORD) 0xFFFF;
#else
    dwVol = (DWORD)uVolume * 0xFFFF / zm_uMaxVolume;
#endif

    // Here we are avoiding rounding error
    if(dwVol * zm_uMaxVolume / 0xFFFF < uVolume)
	dwVol += 0xFFFF / zm_uMaxVolume;

    if (zm_audioDevice == HXAUDIO_BADDEVICE)
    {
	goto noMixer;
    }

#if defined(_WIN32) && !defined(_WINCE)
    if (!zm_bMixerVolSupportChecked)
    {
	CheckForVolumeSupport();
    }

    if(zm_bMixerVolSupport)
    {
	if (!m_hMixer)
	{
	    CheckForVolumeSupport();
	}
        PMIXERCONTROLDETAILS_UNSIGNED pmxVolume;
        UINT16 nItems = 1;
        if(m_VolumeControlDetails.cMultipleItems)
            nItems = (UINT16)m_VolumeControlDetails.cMultipleItems;
        pmxVolume = new MIXERCONTROLDETAILS_UNSIGNED[nItems];
        for(UINT16 nIndex = 0; nIndex < nItems; nIndex++)
            pmxVolume[nIndex].dwValue = dwVol;
        m_VolumeControlDetails.cbDetails = nItems * sizeof(MIXERCONTROLDETAILS_UNSIGNED);
        m_VolumeControlDetails.paDetails = pmxVolume;
        if(mixerSetControlDetails((HMIXEROBJ)m_hMixer, &m_VolumeControlDetails, 
           MIXER_GETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER) == MMSYSERR_NOERROR)
		{
			bSuccess = TRUE;
        }
        delete[] pmxVolume;
    }
#endif

noMixer:
    if ( !bSuccess )
    {
	DWORD dwLRVol = MAKELONG(dwVol, dwVol) ;
	// fix of bug 4965, in which the balance is thrown to one side.  My speculation is that the
	// driver incorrectly reports MONO here when stereo is in use.  To fix the bug, set both
	// channels equally.  john dempsey

	/* related information: 
	"If a devicedoes not support both left and right volume control, the low-order
	 word of the dwVolume argument specifies the volume level and the
	 high-order word is ignored." -- some DEC document on waveOutSetVolume.
	*/

	HX_ASSERT(LOWORD(dwLRVol) == HIWORD(dwLRVol)) ;
	if (!zm_bVolSupport)
	{
	    return HXR_FAILED;
	}

	MMRESULT hResult    = waveOutSetVolume(m_hWave, dwLRVol);

	
	MMRESULT hResult2 = hResult;

	/* 
	 * always set the volume on NULL device. 
	 * Needed to attach volume control on win98 SE, Creative SB Live! Value sound card.
	 */
	if (m_hWave != NULL)
	{
	     hResult2 = waveOutSetVolume(NULL, dwLRVol);
	}

	if (hResult != MMSYSERR_NOERROR && hResult2 != MMSYSERR_NOERROR)
	{
	    return HXR_FAILED;
	}
    }

    return HXR_OK;
}

HXBOOL CAudioOutWindows::_Imp_SupportsVolume()
{
    MMRESULT wSuccess;
    WAVEOUTCAPS auxcap; 

#if defined(_WIN32) && !defined(_WINCE)
    if (!zm_bMixerVolSupportChecked)
    {
	CheckForVolumeSupport();
    }
#endif

#if defined(_WIN32) || defined(_WINCE)

	// Gonna have to make VolSupport static  
    if ( zm_bVolSupport )
    {
        return (zm_bVolSupport);
    }

    wSuccess = waveOutGetDevCaps( 0, &auxcap, sizeof(WAVEOUTCAPS) );

    if ( MMSYSERR_NOERROR == wSuccess )
    { 
        if (auxcap.dwSupport & WAVECAPS_VOLUME)
		{
		    zm_bVolSupport=TRUE;
		}

		if (auxcap.dwSupport & WAVECAPS_LRVOLUME)
		{
			zm_bLRVolSupport = TRUE;
		}
	
		zm_uMaxVolume = 100;
	}                   
    else
    {
        zm_bVolSupport = FALSE;
        zm_bLRVolSupport = FALSE;
    }

    return zm_bVolSupport;
#else
    return FALSE;
#endif /*_WIN32*/
}

HX_RESULT CAudioOutWindows:: _Imp_Open
( 
    const HXAudioFormat* pFormat
)
{

    HX_RESULT theErr = HXR_OK;
#if defined(_WIN32)
    LPWAVEFORMATEX	wavePtr;
#elif defined( _WINDOWS )
    LPWAVEFORMAT	wavePtr;
#endif

    theErr = Register();
    if (theErr)
    {
	return theErr;
    }
	
#ifdef _TESTING
    m_audfile =  open("\\auddev.raw", O_WRONLY | O_CREAT);
#endif

    m_WaveFormat.SetFormat(pFormat->ulSamplesPerSec, pFormat->uChannels, pFormat->uBitsPerSample);
    // Get the Windows style wave format!
    wavePtr = m_WaveFormat.GetWaveFormat();

    // We ain't closed now
    zm_bClosed = FALSE;

    // Open the wave driver.
#ifdef _WIN16
    MMRESULT wRet = waveOutOpen( &m_hWave, WAVE_MAPPER, wavePtr,
			(UINT16)m_hWnd, (DWORD)this, CALLBACK_WINDOW);
#else
    MMRESULT wRet = waveOutOpen( &m_hWave, WAVE_MAPPER, wavePtr,
			(DWORD)m_hWnd, (DWORD)this, CALLBACK_WINDOW);
#endif


    //	Okay, translate any error returns and get out if there was an error
    switch (wRet)
    {
	case MMSYSERR_NOERROR:	    theErr = HXR_OK;		break;
	case MMSYSERR_ALLOCATED:    theErr = HXR_AUDIO_DRIVER;	break;
	case MMSYSERR_NOMEM:	    theErr = HXR_OUTOFMEMORY;	break;
	case MMSYSERR_BADDEVICEID:  theErr = HXR_AUDIO_DRIVER;	break;
	case WAVERR_BADFORMAT:	    theErr = HXR_AUDIO_DRIVER;	break;
	case WAVERR_SYNC:	    theErr = HXR_AUDIO_DRIVER;	break;
	default:		    theErr = HXR_OUTOFMEMORY;	break;
    }
#if defined(_WIN32) && !defined(_WINCE)
	if ( !theErr )
	{
	    if (!zm_bMixerVolSupportChecked)
	    {
		CheckForVolumeSupport();
	    }
	}
#endif


    return theErr;
}

HX_RESULT CAudioOutWindows::_Imp_Close()
{
#ifdef _TESTING
	if ( m_audfile > 0 ) 
		close(m_audfile);
	m_audfile = -1;
#endif

 zm_bClosed = TRUE;
    if (m_hWave) 
    {
	// NOTE: Before we can close, we need to reset!
	Reset();

	HX_LOCK(m_pMutex);

	if (m_pWaveHdrs)
	{
	    UINT16 unWHIndex;

	    // Unprepare the individual headers
	    for (unWHIndex = 0; unWHIndex < m_unAllocedBufferCnt; ++unWHIndex)
	    {
    		CWaveHeader* pWaveHeader = &m_pWaveHdrs[unWHIndex];
		if (pWaveHeader->m_pPrepared)
		{
		    LPWAVEHDR lpwHeader = LPWAVEHDR(&pWaveHeader->m_WAVEHDR);
		    
		    // By now all headers should be marked as done because they
		    // are back from the audio device or prepared because we
		    // prepared them but never used them
		    HX_ASSERT(lpwHeader->dwFlags & WHDR_DONE ||
			lpwHeader->dwFlags & WHDR_PREPARED);

		    //OutputDebugString("BEFORE CALL TO:waveOutUnprepareHeader\r\n");
		    HX_VERIFY(MMSYSERR_NOERROR == waveOutUnprepareHeader(m_hWave, 
					LPWAVEHDR(&pWaveHeader->m_WAVEHDR), sizeof(WAVEHDR)));
		    //OutputDebugString("AFTER CALL TO:waveOutUnprepareHeader\r\n");
		    pWaveHeader->m_pPrepared = FALSE;
		}
	    }
	}
	
	if (m_ppAllocedBuffers)
	{
	    UINT16 unIndex;

	    for (unIndex = 0; unIndex < m_unAllocedBufferCnt; unIndex++)
	    {
		delete[] m_ppAllocedBuffers[unIndex];
	    }
	    delete[] m_ppAllocedBuffers;
	    m_ppAllocedBuffers = NULL;
	}

	m_rAvailBuffers.FlushQueue();
	m_UsedBuffersList.RemoveAll();

	m_unAllocedBufferCnt = 0;

	HX_UNLOCK(m_pMutex);

	for (int i = 0; 
		i < 5 && (WAVERR_STILLPLAYING == waveOutClose( m_hWave )); ++i)
	{
	    waveOutReset( m_hWave );
	}
	m_hWave = NULL;

	if (m_pWaveHdrs)
	{
	    delete[] m_pWaveHdrs;
	    m_pWaveHdrs = NULL;
	}
    }

    UnRegister();
    m_bIsFirstPacket	= TRUE;
    m_bInitialized	= FALSE;

    return HXR_OK;
}

HX_RESULT CAudioOutWindows::_Imp_Write
( 
    const HXAudioData* pAudioOutHdr 
)
{
    HXBOOL	    bTemp;
    CWaveHeader*    pWaveHeader;
    MMRESULT	    res;
    ULONG32	    ulBufLen = 0;
    UCHAR*	    pBuffer = 0;
    if (pAudioOutHdr->pData)
    {
	pBuffer	 = pAudioOutHdr->pData->GetBuffer();
	ulBufLen = pAudioOutHdr->pData->GetSize();
    }

    if (!m_bInitialized)
    {
	m_bInitialized = AllocateBuffers(MAX_REASONABLE_BUFFS,(UINT16)ulBufLen);
    }
    if (!m_bInitialized) return HXR_FAILED;

    HX_LOCK(m_pMutex);

    pWaveHeader = (CWaveHeader*)m_rAvailBuffers.DeQueuePtr(bTemp);

    HX_UNLOCK(m_pMutex);

    if (!bTemp) 
    {
	return HXR_WOULD_BLOCK;
    }

#ifdef _TESTING
    if ( m_audfile > 0 )
        write(m_audfile, pBuffer,ulBufLen); 
#endif

    UINT32 ulBytesToCopy = ulBufLen;
    HX_ASSERT(ulBytesToCopy <= pWaveHeader->m_WAVEHDR.dwBufferLength);
    if (ulBytesToCopy > pWaveHeader->m_WAVEHDR.dwBufferLength)
        ulBytesToCopy = pWaveHeader->m_WAVEHDR.dwBufferLength;

    pWaveHeader->m_bAvail = FALSE;
        memcpy(pWaveHeader->m_WAVEHDR.lpData, pBuffer, HX_SAFESIZE_T(ulBytesToCopy)); /* Flawfinder: ignore */
    pWaveHeader->m_ulTimeEnd = pAudioOutHdr->ulAudioTime;

    /* This is needed for LIVE where audio packets may not start
     * from timestamp 0
     */
    if (m_bIsFirstPacket)
    {
	m_bIsFirstPacket = FALSE;
    }

    //OutputDebugString("BEFORE CALL TO:waveOutWrite\r\n");
    res = waveOutWrite(m_hWave, LPWAVEHDR(&pWaveHeader->m_WAVEHDR), sizeof(WAVEHDR));

    HX_LOCK(m_pMutex);
    m_UsedBuffersList.AddTail(pWaveHeader);
    HX_UNLOCK(m_pMutex);
    //OutputDebugString("AFTER CALL TO:waveOutWrite\r\n");

    return HXR_OK;
}

HX_RESULT CAudioOutWindows::_Imp_Seek(ULONG32 ulSeekTime)
{
    return HXR_OK;
}

HX_RESULT CAudioOutWindows::_Imp_Pause()
{
    MMRESULT res;
    if (m_hWave) 
    {
//	OutputDebugString("BEFORE CALL TO:waveOutPause\r\n");
        res = waveOutPause(m_hWave);
	//OutputDebugString("AFTER CALL TO:waveOutPause\r\n");
    }
    return HXR_OK;
}

HX_RESULT CAudioOutWindows::_Imp_Resume()
{
    MMRESULT res;
    if (m_hWave) 
    {
//	OutputDebugString("BEFORE CALL TO:waveOutRestart\r\n");
        res = waveOutRestart(m_hWave);
	//OutputDebugString("AFTER CALL TO:waveOutRestart\r\n");
    }

    OnTimeSync();

    return HXR_OK;
}

HX_RESULT CAudioOutWindows::_Imp_Reset()
{
//{FILE* f1 = ::fopen("c:\\audio.txt", "a+"); ::fprintf(f1, "Reset: \n");::fclose(f1);}
    MMRESULT res;
    m_bResetting = TRUE;
    if (m_hWave) 
    {
//	OutputDebugString("BEFORE CALL TO:waveOutReset\r\n");
	res = waveOutReset(m_hWave);
	// Need to be turned on after RC2000 XXXRA
	// commenting it out for safety reasons.
	//_Imp_Pause();
	//OutputDebugString("AFTER CALL TO:waveOutReset\r\n");
	m_bIsFirstPacket	= TRUE;

	m_llDeviceBytesPlayed = 0;
	m_llDeviceSamplesPlayed = 0;

	m_ulLastDeviceBytesPlayed = 0;
	m_ulLastDeviceSamplesPlayed = 0;
	m_ulDevPosRollOver = 0;
    }

    // First return the unused buffers to the available queue
    // Then pump out the wom_dones.
    //
    _NumberOfBlocksRemainingToPlay();

    // Make sure we handle all the wave-done messages,
    // but establish a time limit to cope with the 
    // unpredictable nature of Windows. Wait for up
    // to 1 second.

    DWORD start = HX_GET_TICKCOUNT();
    if (m_hWnd)
    {
	do
	{
	    MSG msg;

	    // Yield, and try to pump WOM_DONE's to the Async Window!
	    while (PeekMessage(&msg, m_hWnd, MM_WOM_DONE, MM_WOM_DONE, PM_REMOVE))
	    {
		if(msg.message == WM_QUIT) 
		{   
		    // When peeking WM_QUIT message in the main thread of an application 
		    // we have to put it back into message queue to allow the application 
		    // to exit correctly. SB
		    PostQuitMessage(0);
		    break;
		}
		else
		{
		    DispatchMessage(&msg);
		}
	    }

	    if (CALCULATE_ELAPSED_TICKS(start, HX_GET_TICKCOUNT()) >= 1000)
	    {

		// Brute force, mark them as free... We probably 
		// could do something better here, like close and
		// reopen the wave device... but this is what the
		// old code did, so lets try it as well!

		ULONG32 lTmp;
		HX_LOCK(m_pMutex);
		for (lTmp = 0; m_pWaveHdrs && (lTmp < m_unAllocedBufferCnt); ++lTmp)
		{
		    CWaveHeader *pwhDone = &m_pWaveHdrs[lTmp];
		    if (pwhDone && pwhDone->m_bAvail == FALSE)
		    {
			pwhDone->m_bAvail = TRUE;
			m_rAvailBuffers.EnQueuePtr(pwhDone);
		    }
		}
		HX_UNLOCK(m_pMutex);

		//OutputDebugString("Bail WOM_DONE vacuum, passed timeout!\r\n");
		break;
	    }

	}
	while (_NumberOfBlocksRemainingToPlay());
    }
    //OutputDebugString("END: WOM_DONE vacuum!\r\n");

    m_bResetting = FALSE;
    return HXR_OK;
}

HX_RESULT CAudioOutWindows::_Imp_Drain()
{
    return HXR_OK;
}

HX_RESULT CAudioOutWindows::_Imp_CheckFormat
( 
    const HXAudioFormat* pFormat 
)
{
#if defined(_WIN32)
    LPWAVEFORMATEX	wavePtr;
#elif defined( _WINDOWS )
    LPWAVEFORMAT	wavePtr;
#endif

    m_WaveFormat.SetFormat(pFormat->ulSamplesPerSec, pFormat->uChannels, pFormat->uBitsPerSample);
   
	// Get the Windows style wave format!
    wavePtr = m_WaveFormat.GetWaveFormat();


    MMRESULT wRet = waveOutOpen(&m_hWave,WAVE_MAPPER,wavePtr,		
        0, (DWORD)this,WAVE_FORMAT_QUERY);

    switch (wRet)
    {
	case MMSYSERR_NOERROR:	    return( HXR_OK );
	case MMSYSERR_ALLOCATED:    return( HXR_AUDIO_DRIVER );
	case MMSYSERR_NOMEM:	    return( HXR_OUTOFMEMORY );
	case MMSYSERR_BADDEVICEID:  return( HXR_AUDIO_DRIVER );
	case WAVERR_BADFORMAT:	    return( HXR_AUDIO_DRIVER );
	case WAVERR_SYNC:	    return( HXR_AUDIO_DRIVER );
	case MMSYSERR_NODRIVER:	    return( HXR_AUDIO_DRIVER );
	default:		    return( HXR_AUDIO_DRIVER );
    }

}

HXBOOL CAudioOutWindows::AllocateBuffers(UINT16 unNumBuffers, UINT16 unBufSize)
{
    UCHAR**	    ppWaveBuffers;
    CWaveHeader*    pWaveHdrs;
    MMRESULT	    tRes;
    UINT16	    unWHIndex;


    ppWaveBuffers = NULL;
    pWaveHdrs = NULL;

    // Allocate memory for wave block headers and for wave data.
    if (!(pWaveHdrs = new CWaveHeader [unNumBuffers]))
    {
        goto OnError;
    }

    //	This array holds the buffer pointers so we can free them later
    //	There NO other purpose for this array.	
    if (!(ppWaveBuffers = new UCHAR*[unNumBuffers]))
    {
	goto OnError;
    }
    memset( ppWaveBuffers, 0, unNumBuffers * sizeof( UCHAR* ) );
				    
    // Mark all wave block headers as available, and point them
    // at their respective wave data blocks.
    // AND enqueue them in the AvailableBuffers Queue.
    // AND allocate their buffers!!!

    for (unWHIndex = 0; unWHIndex < unNumBuffers; unWHIndex++) 
    {
        UCHAR*	pBuff;

	// Try to allocate the individual audio buffers
    	pBuff = new UCHAR [unBufSize];
	if (!pBuff)
	{
	    goto OnError;
    	}
	ppWaveBuffers[unWHIndex] = pBuff;

	CWaveHeader* pWaveHeader = &pWaveHdrs[unWHIndex];

	// Init some header data before Preparing the header
	pWaveHeader->m_WAVEHDR.lpData = (char *)pBuff;
	pWaveHeader->m_WAVEHDR.dwUser = DWORD(pWaveHeader);
	pWaveHeader->m_WAVEHDR.dwFlags = 0L;
	pWaveHeader->m_WAVEHDR.dwBufferLength = unBufSize;
	pWaveHeader->m_WAVEHDR.dwLoops = 0L;
	pWaveHeader->m_bAvail = TRUE;
	pWaveHeader->m_pUser = this;

	HX_LOCK(m_pMutex);
	m_rAvailBuffers.EnQueuePtr(pWaveHeader);
	HX_UNLOCK(m_pMutex);

	//OutputDebugString("BEFORE CALL TO:waveOutPrepareHeader\r\n");
        tRes = waveOutPrepareHeader(m_hWave, LPWAVEHDR(&pWaveHeader->m_WAVEHDR), sizeof(WAVEHDR));
	//OutputDebugString("AFTER CALL TO:waveOutPrepareHeader\r\n");
	if (tRes != MMSYSERR_NOERROR)
	{
	    goto OnError;
	}
	else
	{
	    pWaveHeader->m_pPrepared = TRUE;
    	}
    }
    m_unAllocedBufferCnt = unNumBuffers;
    m_unAllocedBuffSize = unBufSize;
    m_pWaveHdrs = pWaveHdrs;
    m_ppAllocedBuffers = ppWaveBuffers;

    return TRUE;

OnError:

    if (pWaveHdrs)
    {
	// Unprepare the individual headers
	for (unWHIndex = 0; unWHIndex < unNumBuffers; ++unWHIndex)
	{
    	    CWaveHeader* pWaveHeader = &pWaveHdrs[unWHIndex];
	    if (pWaveHeader->m_pPrepared)
	    {
		//OutputDebugString("BEFORE CALL TO:waveOutUnprepareHeader\r\n");
		tRes = waveOutUnprepareHeader(m_hWave, 
				    LPWAVEHDR(&pWaveHeader->m_WAVEHDR), sizeof(WAVEHDR));
		//OutputDebugString("AFTER CALL TO:waveOutUnprepareHeader\r\n");
		pWaveHeader->m_pPrepared = FALSE;
	    }
	}

	delete[] pWaveHdrs;
	pWaveHdrs = NULL;
    }
    //	ppWaveBuffers actually points at an array of pointers that need to be free'd
    if (ppWaveBuffers)
    {
	//	Free the individual buffers
	for (unWHIndex = 0; unWHIndex < unNumBuffers; ++unWHIndex)
	{
	    UCHAR* pBuff;

	    pBuff = ppWaveBuffers[unWHIndex];
	    if (pBuff)
	    {
    		delete[] pBuff;
	    }
	}
	//	Now free ppWaveBuffers itself
	delete[] ppWaveBuffers;
	ppWaveBuffers = NULL;
    }
    return FALSE;
}

/************************************************************************
 *  Method:
 *              CAudioOutWindows::_Imp_GetCurrentTime
 *      Purpose:
 *              Get the current time from the audio device.
 *              We added this to support the clock available in the
 *              Window's audio driver.
 */
HX_RESULT CAudioOutWindows::_Imp_GetCurrentTime
( 
        ULONG32& ulCurrentTime
)
{
	ULONG32 ulDeviceTime = 0;
	ULONG32 ulDeviceBytesPlayed = 0;
	UINT32	ulDeviceSamplesPlayed = 0;

	WAVEFMTPTR pWFmt = m_WaveFormat.GetWaveFormat();

	// Set the time to the playback time of the
	// wave device since it's the most accurate.
	MMTIME mmTime;

	// we want MS if we can get them
	mmTime.wType = TIME_MS;

	MMRESULT res = waveOutGetPosition(m_hWave, &mmTime, sizeof(MMTIME));

	if (MMSYSERR_NOERROR == res)
	{
		// We must check the output format and convert to MS if possible
		switch (mmTime.wType)
		{
			case TIME_MS:
			{
				ulDeviceTime = mmTime.u.ms;

#if defined(_WIN32)
				ULONG32 BitsPerSample = (pWFmt->nChannels * pWFmt->wBitsPerSample);
#elif _WIN16
				ULONG32 BitsPerSample = (pWFmt->nAvgBytesPerSec * 8 / pWFmt->nSamplesPerSec);
#endif
				ULONG32 BitsPerSecond = (BitsPerSample * pWFmt->nSamplesPerSec);
				ulDeviceBytesPlayed = (mmTime.u.ms * BitsPerSecond) / 8000;
			}
			break;
			case TIME_SAMPLES:
			{
				// Convert samples to MS
				ulDeviceSamplesPlayed = mmTime.u.sample;

				if (m_ulLastDeviceSamplesPlayed > ulDeviceSamplesPlayed &&
				    ((m_ulLastDeviceSamplesPlayed - ulDeviceSamplesPlayed) > MAX_TIMESTAMP_GAP))
				{
				    m_ulDevPosRollOver++;
				}
 
				m_ulLastDeviceSamplesPlayed = ulDeviceSamplesPlayed;
				m_llDeviceSamplesPlayed = (INT64)ulDeviceSamplesPlayed + (INT64)m_ulDevPosRollOver * (INT64)MAX_UINT32;

				ulDeviceTime = (ULONG32)(((float)m_llDeviceSamplesPlayed/(float)pWFmt->nSamplesPerSec)*1000.f);

#if defined(_WIN32)
				ULONG32 BitsPerSample = (pWFmt->nChannels * pWFmt->wBitsPerSample);
#elif _WIN16
				ULONG32 BitsPerSample = (pWFmt->nAvgBytesPerSec * 8 / pWFmt->nSamplesPerSec);
#endif
				ulDeviceBytesPlayed = (ULONG32)(mmTime.u.sample * 8.0f) / BitsPerSample;
			}
			break;
			case TIME_BYTES:
			{
				// Convert Bytes to MS
				ulDeviceBytesPlayed = mmTime.u.cb;

				if (m_ulLastDeviceBytesPlayed > ulDeviceBytesPlayed &&
				    ((m_ulLastDeviceBytesPlayed - ulDeviceBytesPlayed) > MAX_TIMESTAMP_GAP))
				{
				    m_ulDevPosRollOver++;
				}

				m_ulLastDeviceBytesPlayed = ulDeviceBytesPlayed;
				m_llDeviceBytesPlayed = (INT64)ulDeviceBytesPlayed + (INT64)m_ulDevPosRollOver * (INT64)MAX_UINT32;

//{FILE* f1 = ::fopen("c:\\audio.txt", "a+"); ::fprintf(f1, "ulDeviceBytesPlayed: %lu\n", ulDeviceBytesPlayed);::fclose(f1);}

#if defined(_WIN32)
				ULONG32 BitsPerSample = (pWFmt->nChannels * pWFmt->wBitsPerSample);
#elif _WIN16
				ULONG32 BitsPerSample = (pWFmt->nAvgBytesPerSec * 8 / pWFmt->nSamplesPerSec);
#endif
				ULONG32 BitsPerSecond = (BitsPerSample * pWFmt->nSamplesPerSec);
				ulDeviceTime = (ULONG32)((m_llDeviceBytesPlayed * 8000)/(INT64)BitsPerSecond);			}
			break;
		}
		// ?? ulCurrentTime = ulDeviceTime + m_StreamTimeOffset + m_CorrectionOffset;
		ulCurrentTime = m_ulCurrentTime = ulDeviceTime;
	}
	else
	{
		ulCurrentTime = m_ulCurrentTime = 0;
	}	

//{FILE* f1 = ::fopen("c:\\audio.txt", "a+"); ::fprintf(f1, "%lu\t%lu\n", HX_GET_TICKCOUNT(), m_ulCurrentTime);::fclose(f1);}

	return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CAudioOutWindows::WaveOutProc
 *      Purpose:
 *              This static member function is called when the wave 
 *              device is finished playing its audio buffer.  We use 
 *              it to tell audio services that we're ready for more.
 */
#if defined(_WIN32)
	LRESULT __declspec(dllexport) CALLBACK 
#else
	LRESULT CALLBACK __export 
#endif
CAudioOutWindows::WaveOutWndProc(HWND hWnd,	// handle of window
				UINT uMsg,	// message identifier
				WPARAM wParam,	// first message parameter
				LPARAM lParam) 	// second message parameter)
{
    DWORD dwParam1 = lParam;

    CAudioOutWindows* pThis = 0;

    if (uMsg == WM_NCCREATE)
    {
	CREATESTRUCT* lpCreate = 0;

	// Set our this pointer, so our WndProc can find us again
	lpCreate = (CREATESTRUCT FAR*) lParam;
	pThis = (CAudioOutWindows*) lpCreate->lpCreateParams;
	SetWindowLong(hWnd, OFFSET_THIS, (long) pThis);
    }
    else if (uMsg == WM_NCDESTROY)
    {
	// remove our this pointer so if somebody calls this function
	// again after the window is gone (and the object is gone
	// too) we don't try to call a method from the pointer
	SetWindowLong(hWnd, OFFSET_THIS, 0L);
    }
    else
    {
	pThis = (CAudioOutWindows*) (LPHANDLE)GetWindowLong(hWnd, OFFSET_THIS);
    }

    if (!pThis)
    {
	goto exit;
    }

    switch (uMsg)
    {
	    case MM_WOM_DONE:
		    HX_ASSERT(zm_bClosed || zm_pCurrentAudioDevice == pThis);
		    if (!zm_bClosed && zm_pCurrentAudioDevice == pThis)
		    {
			    HXLOGL4(HXLOG_ADEV, "CAudioOutWindows::WaveOutWndProc: block done");

			    // Only do this on WOM_DONE, note there are other messages sent to this
			    // callback function, like WOM_OPEN and WOM_CLOSE
			    //DWORD dwTime1 = timeGetTime();

			    //OutputDebugString("START:(uMsg == MM_WOM_DONE) in WaveOutThreadProc\r\n");

			    if (pThis->m_pMutex)
			    {
				pThis->m_pMutex->Lock();
			    }

			    if (zm_bClosed)
			    {
				if (pThis->m_pMutex)
				{
				    pThis->m_pMutex->Unlock();
				}
				goto exit;
			    }

			    WAVEHDR*		pWAVEHDR;
			    CWaveHeader*	pWaveHeader;
			    
			    pWAVEHDR = (WAVEHDR*)dwParam1;
			    pWaveHeader = (CWaveHeader*)pWAVEHDR->dwUser;

			    /* We may have already removed it from the queue
			     * while checking for _NumberOfBlocksRemainingToPlay
			     */
			    if ((pWAVEHDR->dwFlags & WHDR_DONE) &&
                                !pThis->m_UsedBuffersList.IsEmpty() &&
				pThis->m_UsedBuffersList.GetHead() == (void*) pWaveHeader)
			    {
				// put it back in the queue
				pWaveHeader->m_bAvail	= TRUE;
				pThis->m_rAvailBuffers.EnQueuePtr(pWaveHeader);

				/* This is assuming that we get WOM_DONEs sequentially
				 * There seems to be a bug in WIN16 where it may return
				 * WOM_DONEs out of order. Need to be fixed for that
				 * XXX Rahul
				 */
				pThis->m_UsedBuffersList.RemoveHead();
			    }
			    if (pThis->m_pMutex)
			    {
				pThis->m_pMutex->Unlock();
			    }

			    if (pThis &&
				!pThis->m_bResetting)
			    {
				//  Call the base class's  OnTimeSync to let it know that we're done playing
				//  the current chunk of audio
				//OutputDebugString("BEFORE:pThis->OnTimeSync()\r\n");
				pThis->OnTimeSync();
			    }
		    }
		    break;

	    case MM_WOM_OPEN:
		    break;
			       	    
	    case MM_WOM_CLOSE:   	
		    break;
		    
	    default:
		    break;	
    }

    if (uMsg == zm_uDestroyMessage)
    {
	LRESULT result = (LRESULT)DestroyWindow(hWnd);

	// free the memory used by this class now that our window is destroyed
	UnregisterClass(WND_CLASS, g_hInstance);
	return result;
    }

exit:
    return ( DefWindowProc(hWnd, uMsg, wParam, lParam) );    
}




UINT16	CAudioOutWindows::_NumberOfBlocksRemainingToPlay(void)
{
    HX_LOCK(m_pMutex);

    UINT16 unRemaining = m_UsedBuffersList.GetCount();

    /* There may be some buffers for which we have not receoved WOM_DONEs
     * but they have already been played
     */
    if (unRemaining > 0)
    {
	LISTPOSITION ndxLastUsed = NULL;
	LISTPOSITION ndxUsed = m_UsedBuffersList.GetHeadPosition();
	while (ndxUsed != NULL)
	{
	    ndxLastUsed	= ndxUsed;
	    CWaveHeader* pWaveHeader = 
		    (CWaveHeader*) m_UsedBuffersList.GetNext(ndxUsed);
	    if (pWaveHeader->m_WAVEHDR.dwFlags & WHDR_DONE)
	    {
		if (unRemaining > 0)
		    unRemaining--;

		// put it back in the queue
		pWaveHeader->m_bAvail	= TRUE;
		m_rAvailBuffers.EnQueuePtr(pWaveHeader);
		m_UsedBuffersList.RemoveAt(ndxLastUsed);
	    }
	    else
	    {
		/* We assume that subsequent blocks are also not done playing */
		break;
	    }
	}
    }

    HX_UNLOCK(m_pMutex);

    return unRemaining;
}

#if defined(_WIN32) && !defined(_WINCE)
void
CAudioOutWindows::CheckForVolumeSupport()
{
    zm_bMixerVolSupportChecked = TRUE;

    if (!m_hWnd)
    {
	Register();
    }

    // if there is more than 1 mixer, we need to find out which is the current one
    // we do this by determining the mixer being used by the current wave device
    int startIndex = 0;
    if (m_hWave != NULL && mixerGetNumDevs() > 1)
    {
	UINT nId = -1;   
	if ((mixerGetID((HMIXEROBJ)m_hWave, &nId, MIXER_OBJECTF_HWAVEOUT) == MMSYSERR_NOERROR) && nId != -1)
	{
	    startIndex = nId;
	}
    }

    for(UINT index = startIndex; index < mixerGetNumDevs(); index++)
    {
	if(mixerOpen(&m_hMixer, index, (DWORD)m_hWnd, 0, CALLBACK_WINDOW) 
							== MMSYSERR_NOERROR)
	{
	    MIXERLINE mxLine;
	    mxLine.cbStruct = sizeof(MIXERLINE);
	    mxLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
	    if(mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mxLine, 
	       MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER) 
						    == MMSYSERR_NOERROR)
	    {
		// use waveOutSetVolume for bad device(i.e. crystal audio)
		if (zm_audioDevice == HXAUDIO_UNKNOWN && mxLine.Target.szPname)
		{
		    for (int i = 0; i < g_nBadDrivers; i++)
		    {
			if (_tcsstr(mxLine.Target.szPname, g_badDrivers[i]))
			{
			    zm_audioDevice = HXAUDIO_BADDEVICE;
			    break;
			}
		    }

		    if (zm_audioDevice == HXAUDIO_UNKNOWN)
		    {
			zm_audioDevice = HXAUDIO_GOODDEVICE;
		    }
		}

		if (zm_audioDevice != HXAUDIO_BADDEVICE)
		{
		    MIXERCONTROL mxControl;
		    mxControl.cbStruct = sizeof(MIXERCONTROL);
		    MIXERLINECONTROLS mxLineControls;
		    mxLineControls.cbStruct = sizeof(MIXERLINECONTROLS);
		    mxLineControls.dwLineID = mxLine.dwLineID;
		    mxLineControls.cControls = 1;
		    mxLineControls.cbmxctrl = mxControl.cbStruct;
		    mxLineControls.pamxctrl = &mxControl;
		    mxLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME | 
						   MIXERCONTROL_CONTROLF_UNIFORM;

		    if(mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxLineControls, 
		       MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER) 
							    == MMSYSERR_NOERROR)
		    {
			if(mxLineControls.cControls)
			{
			    zm_bVolSupport = TRUE;
			    zm_bMixerVolSupport = TRUE;
			    m_VolumeControlDetails.cbStruct = 
						sizeof(MIXERCONTROLDETAILS); 
			    m_VolumeControlDetails.dwControlID = 
						mxControl.dwControlID; 
			    m_VolumeControlDetails.cChannels = 1; 
			    m_VolumeControlDetails.cMultipleItems = 
						mxControl.cMultipleItems; 
			}
		    }   
		}
	    }
	    break;
	}
    }
}
#endif /*_WIN32*/


HX_RESULT
CAudioOutWindows::Register()
{
    WNDCLASS internalClass;

//    OutputDebugString("BEFORE CALL TO:Register\r\n");
    if (m_hWnd)
    {
	return HXR_OK;
    }

	//m_hInst = hInst;
	m_hInst = g_hInstance;

	if (!m_hInst)
	{
#ifdef _DEBUG
	MessageBox(NULL, _T("Don't have a valid handle"), NULL, MB_OK);
#endif
 	return HXR_OUTOFMEMORY;
	}


	// XXXKM - let's see if we can get the class info first; added this additional
	// check due to a strange problem when registering a class that seems to already
	// be registered.  For some reason, under some circumstance, RegisterClass returns
	// NULL and GetLastError returns 0x57 (invalid parameter), however the class
	// is already registered
	if (!::GetClassInfo(m_hInst, WND_CLASS, &internalClass))
	{
		//	First register our window class                                  
		internalClass.style 	= 0;

		internalClass.lpfnWndProc 	= CAudioOutWindows::WaveOutWndProc;

		internalClass.cbClsExtra    = 0;
		internalClass.cbWndExtra    = sizeof( this );
		internalClass.hInstance     = m_hInst; // Use app's instance
		internalClass.hIcon         = 0;
		internalClass.hCursor       = 0;
		internalClass.hbrBackground = 0;
		internalClass.lpszMenuName  = NULL;
		internalClass.lpszClassName = WND_CLASS;

	#ifdef _WIN16
		if (!RegisterClass( &internalClass ))
	#else
		if (!RegisterClass( &internalClass ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	#endif /* _WIN16 */    
		{
	#ifdef _DEBUG
		MessageBox(NULL, _T("Could Not register class"), NULL, MB_OK);
	#endif
		return(HXR_OUTOFMEMORY);
		}
		m_bClassRegistered = TRUE;
	}

//    OutputDebugString("BEFORE CALL TO:CreateWindow\r\n");
    //	Now create an instance of the window	
    m_hWnd = CreateWindow( WND_CLASS /*"AudioServicesInternal"*/, _T("Audio Services Internal Messages"), 
	WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, m_hInst, this);

#if defined(_WIN32)
    m_ulOriginalThreadId = GetCurrentThreadId();
#endif

    if (!m_hWnd)
    {
#ifdef _DEBUG
	MessageBox(NULL, _T("Could Not create messageWindow"), NULL, MB_OK);
#endif
	return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

void
CAudioOutWindows::UnRegister()
{
//    OutputDebugString("BEFORE CALL TO:UnRegister\r\n");
    // Ask the window to destroy itself
    if (m_hWnd) 
    {
#if defined(_WIN32)
	if (m_ulOriginalThreadId == GetCurrentThreadId())
	{
	    SendMessage(m_hWnd, zm_uDestroyMessage, 0, 0);
	}
	else
	{
	    PostMessage(m_hWnd, zm_uDestroyMessage, 0, 0);
	    Sleep(0);
	}
#else
	SendMessage(m_hWnd, zm_uDestroyMessage, 0, 0);
#endif

	m_hWnd = NULL;
    }

/*
    // Ask the window to destroy itself
    if (m_hWnd && SendMessage(m_hWnd, zm_uDestroyMessage, 0, 0)) 
    {
	m_hWnd = NULL;
    }

    // free the memory used by this class now that our window is destroyed
    UnregisterClass(WND_CLASS, g_hInstance);

*/
}
