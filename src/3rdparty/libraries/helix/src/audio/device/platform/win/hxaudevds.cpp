/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudevds.cpp,v 1.33 2007/07/06 20:21:17 jfinnecy Exp $
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
#include <tchar.h>
#include <mmsystem.h>
#include "mmreg.h"
#endif /*defined( _WINDOWS ) || defined( _WIN32 )*/

#include <stdio.h>
#include <string.h>

#include "hxresult.h"
#include "cbqueue.h"
#include "cpqueue.h"
#include "hxslist.h"


#include "hxcom.h"
#include "hxengin.h"
#include "ihxpckts.h"	
#include "hxausvc.h"
#include "auderrs.h"
#include "math.h"   
#include "dllacces.h"
#include "dllpath.h"
#include "hlxosstr.h"

#include "hxaudev.h"
#include "tsconvrt.h"

#include "hxaudevds.h"
#include "set_debugger_thread_name.h"

#include <initguid.h>
#include "HXAudioDeviceHook/HXAudioDeviceHook.h"

extern HINSTANCE g_hInstance;

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "hlxosstr.h"

#ifndef TEXT
#define TEXT(w)  OS_STRING(w)
#endif

#ifndef _WINDOWS
#define TCHAR  char
#endif

typedef HRESULT (WINAPI* FPDIRECTSOUNDCREATE)(LPGUID lpGuid, LPDIRECTSOUND8 * ppDS, IUnknown FAR * pUnkOuter);

static LRESULT CALLBACK HXDSWndProc(HWND, UINT, WPARAM, LPARAM);
UINT	CHXAudioDeviceDS::zm_uDestroyMessage = 0;

const UINT32 kExitThreadWaitTime = 3000; // ms
#define HXMSG_TIMESYNC	WM_USER+501

const TCHAR* szTitle = TEXT("Helix DSWnd");
const TCHAR* szWindowClass = TEXT("Helix DSWndClass");
const TCHAR* kDSWaitEvent = TEXT("HelixDirectSoundNotifyWait");
const TCHAR* kDSDestroyMessage = TEXT("HX_DestroyDSWindowInternal");
const int BUFFER_TIME = 8;

CHXAudioDeviceDS::CHXAudioDeviceDS():
	m_bOpaqueFormat(FALSE)
    ,	m_pWaveFormat(NULL)
    ,	m_ulLastPlayCursor(0)
    ,	m_ulLastWriteCursor(0)
    ,	m_ulCurrPlayTime(0)
    ,	m_ulCurrLoopTime(0)
    ,	m_pDSDev(NULL)
    ,	m_pPrimaryBuffer(NULL)
    ,	m_pSecondaryBuffer(NULL)
    ,	m_hwnd(NULL)
    ,	m_pAudioPtrStart(NULL)
    ,	m_hSoundDll(NULL)
    ,	m_ulLoops(0)
    ,	m_ulLoopTime(0)
    ,	m_hDSNotifyEvent(NULL)
    ,	m_hWaitThread(NULL)
    ,	m_nBlocksPerBuffer(0)
    ,	m_bExitThread(FALSE)
    ,	m_ulOriginalThreadId(0)
    ,	m_pAudioHookDMO(NULL)
{
    // Create a unique message for destroying the audio window
    if (!zm_uDestroyMessage)
    {
	zm_uDestroyMessage = RegisterWindowMessage(kDSDestroyMessage);
    }

#ifdef _WINCE
	WNDCLASS wcex;
#else
	WNDCLASSEX wcex;
	wcex.cbSize 	= sizeof(WNDCLASSEX); 
	wcex.hIconSm	= NULL;
#endif

    wcex.style		= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= (WNDPROC)HXDSWndProc;
    wcex.cbClsExtra	= 0;
    wcex.cbWndExtra	= 0;
    wcex.hInstance	= g_hInstance;
    wcex.hIcon		= NULL;
    wcex.hCursor	= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= szWindowClass;

#ifdef _WINCE
	RegisterClass(&wcex);
#else
	RegisterClassEx(&wcex);
#endif

#ifdef _WINCE
	m_hwnd = ::CreateWindow(szWindowClass, szTitle, 
				WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_SYSMENU,
				-5000, -5000, 1, 1, NULL, NULL, g_hInstance, NULL );
#else
	m_hwnd = ::CreateWindow(szWindowClass, szTitle, 
				WS_OVERLAPPEDWINDOW,
				-5000, -5000, 1, 1, NULL, NULL, g_hInstance, NULL );
#endif
    m_ulOriginalThreadId = GetCurrentThreadId();
    
    m_hSoundDll = ::LoadLibrary( TEXT("dsound.dll") );
}

CHXAudioDeviceDS::~CHXAudioDeviceDS()
{
    HX_DELETE(m_pWaveFormat);

    if (m_hSoundDll)
    {
	FreeLibrary(m_hSoundDll);
	m_hSoundDll = NULL;
    }

    if (m_hwnd) 
    {
#if defined(_WIN32)
	if (m_ulOriginalThreadId == GetCurrentThreadId())
	{
	    SendMessage(m_hwnd, zm_uDestroyMessage, 0, 0);
	}
	else
	{
	    PostMessage(m_hwnd, zm_uDestroyMessage, 0, 0);
	    Sleep(0);
	}
#else
	SendMessage(m_hwnd, zm_uDestroyMessage, 0, 0);
#endif

	m_hwnd = NULL;
    }
}

/*
 * Set the format of the primary buffer, if possible. On WDM drivers, this has
 * no effect -- the kernel mixer determines that format.
 */

HX_RESULT CHXAudioDeviceDS::SetPrimaryBufferFormat()
{
    HX_RESULT res = HXR_OK ;

    DSBUFFERDESC bufferDesc;
    ::memset(&bufferDesc, 0, sizeof(DSBUFFERDESC));

    bufferDesc.dwSize	   = sizeof(DSBUFFERDESC);
    bufferDesc.lpwfxFormat = 0 ;

    bufferDesc.dwBufferBytes = 0 ;
    bufferDesc.dwFlags	=  DSBCAPS_PRIMARYBUFFER ;

    /* close the primary buffer if we had one open before. */
    HX_RELEASE(m_pPrimaryBuffer) ;

    /* try to open with WAVE_FORMAT_EXTENSIBLE */
    res = m_pDSDev->CreateSoundBuffer(&bufferDesc, &m_pPrimaryBuffer, NULL);

    if (res == DS_OK)
    {
        res = !DS_OK ;
        if (m_pWaveFormat->nChannels > 2 || m_bOpaqueFormat)
        {
            res = m_pPrimaryBuffer->SetFormat(m_pWaveFormat) ;
        }
        if (res != DS_OK && !m_bOpaqueFormat)
        {
	    /* if that fails, try to open with WAVE_FORMAT_PCM */
	    m_pWaveFormat->wFormatTag = WAVE_FORMAT_PCM ;
	    res = m_pPrimaryBuffer->SetFormat(m_pWaveFormat) ;
        }
    }
    return res ;
}

/*
 *  IHXOpaqueAudioDevice override methods
 */
HX_RESULT CHXAudioDeviceDS::_Imp_OpaqueOpen(
    const HXAudioFormat*    /*IN*/ pAudioFormat, 
    const char*		    /*IN*/ pszOpaqueType, 
    IHXBuffer*		    /*IN*/ pOpaqueData)
{
    if( !pszOpaqueType || !pOpaqueData || strcmp( pszOpaqueType, "audio/x-wma-sap" ) != 0  )
	return HXR_FAIL;

    HX_DELETE(m_pWaveFormat);

    ULONG32 ulBufSize = pOpaqueData->GetSize();
    m_pWaveFormat = (WAVEFORMATEX *) new BYTE[ ulBufSize ];
    memcpy( m_pWaveFormat, pOpaqueData->GetBuffer(), ulBufSize );

    m_bOpaqueFormat = TRUE;
    return _DoOpen(pAudioFormat);
}

/*
 *  IHXAudioDevice override methods
 */
HX_RESULT CHXAudioDeviceDS::_Imp_Open(const HXAudioFormat* pFormat)
{
    /* fill out the wave format structure */
    WAVEFORMATEXTENSIBLE *pwfe = new WAVEFORMATEXTENSIBLE;

    HX_DELETE(m_pWaveFormat);
    m_pWaveFormat = (WAVEFORMATEX *) pwfe;

    pwfe->Format.wFormatTag	= WAVE_FORMAT_EXTENSIBLE;
    pwfe->Format.nChannels	= pFormat->uChannels;
    pwfe->Format.nSamplesPerSec = pFormat->ulSamplesPerSec;	
    pwfe->Format.wBitsPerSample = pFormat->uBitsPerSample;
    pwfe->Format.nBlockAlign	= pFormat->uBitsPerSample/8 * pFormat->uChannels;
    pwfe->Format.nAvgBytesPerSec= pwfe->Format.nBlockAlign * pFormat->ulSamplesPerSec;
    pwfe->Format.cbSize		= 22;

    pwfe->Samples.wValidBitsPerSample = pFormat->uBitsPerSample;
    pwfe->SubFormat = KSDATAFORMAT_SUBTYPE_PCM ;

    pwfe->dwChannelMask = defaultChannelMapping(pFormat->uChannels) ;

    m_bOpaqueFormat = FALSE;
    return _DoOpen(pFormat);
}

HX_RESULT CHXAudioDeviceDS::_DoOpen(const HXAudioFormat* pFormat)
{
    HX_RESULT theErr = HXR_FAIL;

    if(!m_hwnd || !m_hSoundDll)
	return theErr;

    // close open resources
    InternalClose() ;

    /* Get the IDirectSound interface */
    HXBOOL bUsingDS8 = TRUE;
    FPDIRECTSOUNDCREATE fpCreateDS = (FPDIRECTSOUNDCREATE) ::GetProcAddress(m_hSoundDll, TEXT("DirectSoundCreate8") );
    if(!fpCreateDS)
    {
	bUsingDS8 = FALSE;
	fpCreateDS = (FPDIRECTSOUNDCREATE) ::GetProcAddress(m_hSoundDll, TEXT("DirectSoundCreate"));
	if(!fpCreateDS)
	    return theErr;
    }

    theErr = fpCreateDS(NULL, &m_pDSDev, NULL);
    if (FAILED(theErr))
	return theErr;

    /* set the cooperative level. Because we want control over the format of the
       primary buffer (16 bit, multichannel!), we need DSSCL_PRIORITY. */
    m_pDSDev->SetCooperativeLevel(m_hwnd, DSSCL_PRIORITY );

    /* set the format of the primary buffer. This will fail on WDM systems (because
       the kernel mixer termines the primary buffer format), but is important on
       non-WDM systems.
    
       This might change the m_WaveFormat structure from a WAVE_FORMAT_EXTENSIBLE
       to a WAVEFORMATEX.

       Ignore the result.
    */
    SetPrimaryBufferFormat() ;

    /* Now open a secondary buffer. */

    DSBUFFERDESC bufferDesc;
    ::memset(&bufferDesc, 0, sizeof(DSBUFFERDESC));

    bufferDesc.dwSize	   = sizeof(DSBUFFERDESC);
    bufferDesc.lpwfxFormat = m_pWaveFormat;

    // Manipulate the buffer size so that is is an exact multiple of the block size.
    // This will ensure that our write positions on the buffer are the same in every loop.
    // We need to do this so that we have fixed playback notification positions marking the end each write block.
    m_nBlocksPerBuffer = (m_pWaveFormat->nAvgBytesPerSec*BUFFER_TIME)/pFormat->uMaxBlockSize;
    m_ulTotalBuffer = pFormat->uMaxBlockSize*m_nBlocksPerBuffer;
    m_ulLoopTime = (double)m_ulTotalBuffer / (double)m_pWaveFormat->nAvgBytesPerSec;

    bufferDesc.dwBufferBytes = m_ulTotalBuffer ;
    bufferDesc.dwFlags	=
		      DSBCAPS_CTRLVOLUME | // so we can control the volume
		      DSBCAPS_GETCURRENTPOSITION2 | // finer position reports
		      DSBCAPS_CTRLPOSITIONNOTIFY | // have them reported here
		      DSBCAPS_GLOBALFOCUS | // take control!
		      DSBCAPS_STICKYFOCUS |
#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
		      (bUsingDS8 && m_pWaveFormat->nChannels <= 2 && !m_bOpaqueFormat ? DSBCAPS_CTRLFX : 0);
#else
		      0;
#endif

    /* Again, try with WAVE_FORMAT_EXTENSIBLE first, but only if multichannel. */

    theErr = !DS_OK ;
    if (m_pWaveFormat->nChannels > 2 || m_bOpaqueFormat)
    {
	if(!m_bOpaqueFormat)
	    m_pWaveFormat->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        theErr = m_pDSDev->CreateSoundBuffer(&bufferDesc, &m_pSecondaryBuffer, NULL);
    }

    if (theErr != DS_OK && !m_bOpaqueFormat)
    {
	/* and if that fails, try WAVEFORMATEX */
	m_pWaveFormat->wFormatTag = WAVE_FORMAT_PCM;
	theErr = m_pDSDev->CreateSoundBuffer(&bufferDesc, &m_pSecondaryBuffer, NULL);
    }

    /* call it a day and count our blessings. */
    switch (theErr)
    {
	case DS_OK: 
	    theErr = HXR_OK;
	    break;
	case DSERR_OUTOFMEMORY:
	    theErr = HXR_OUTOFMEMORY;
	    break;
	default:
	    theErr = HXR_FAIL;
	    break;
    }

    if (SUCCEEDED(theErr) && m_pSecondaryBuffer)
    {
	m_eState = E_DEV_OPENED;

	KillThreadAndEvent();

	SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)this);

	// Create the event to be signalled on playback position notifications and the thread to wait for those events to be signalled.
	m_hDSNotifyEvent = CreateEvent(NULL, TRUE, FALSE, kDSWaitEvent);

	// now set the notification positions for direct sound playback.
	IDirectSoundNotify* pNotify = NULL;
	m_pSecondaryBuffer->QueryInterface(IID_IDirectSoundNotify, (void**)&pNotify);
	if(pNotify && m_hDSNotifyEvent)
	{
	    DSBPOSITIONNOTIFY* aPositionNotify = new DSBPOSITIONNOTIFY[m_nBlocksPerBuffer];
	    if(aPositionNotify)
	    {
		for(int i = 0; i < m_nBlocksPerBuffer; i++)
		{
		    aPositionNotify[i].dwOffset = i * pFormat->uMaxBlockSize;
		    aPositionNotify[i].hEventNotify = m_hDSNotifyEvent;
		}
	    }
	    pNotify->SetNotificationPositions(m_nBlocksPerBuffer, aPositionNotify);
	    delete[] aPositionNotify;
	    DWORD dwWaitThreadID(0);
	    m_hWaitThread = CreateThread(NULL, 0, EventThreadProc, (LPVOID)this, 0, &dwWaitThreadID);
            SetDebuggerThreadName(dwWaitThreadID, "DirectSound Audio Device Thread");
	    SetThreadPriority( m_hWaitThread, THREAD_PRIORITY_HIGHEST );
	}
	HX_RELEASE(pNotify);

	m_pSecondaryBuffer->SetVolume(DSBVOLUME_MAX);
	m_pSecondaryBuffer->SetCurrentPosition(0);

        // detect whether we are playing remotely(via Remote Desktop)
        // disable audio device hook if it is to avoid poor audio quality
        int bRemoteSession = GetSystemMetrics(SM_REMOTESESSION);
	if(!bRemoteSession && bUsingDS8 && m_pWaveFormat->nChannels <= 2 && !m_bOpaqueFormat)
	{
	    LoadDirectSoundFilter();
	}
    }

    m_ulCurrPlayTime = 0;
	m_ulCurrLoopTime = 0;
    m_ulLoops = 0;

    // Setup converter to convert from samples/sec to milliseconds
    m_TSConverter.SetBase(m_pWaveFormat->nSamplesPerSec, 1000);

    return theErr;
}

HX_RESULT CHXAudioDeviceDS::LoadDirectSoundFilter()
{
    HX_RESULT res = HXR_OK;

#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    res = RegisterDirectSoundFilter();
    if(FAILED(res))
	return res;

    IDirectSoundBuffer8* pDSBuf8 = NULL;
    res = m_pSecondaryBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&pDSBuf8);
    if(pDSBuf8)
    {
	DSEFFECTDESC dsfx;
	memset( &dsfx, 0, sizeof( dsfx ));
	dsfx.dwSize = sizeof( dsfx );
	dsfx.guidDSFXClass = CLSID_HXAUDIODEVICEHOOK;

	DWORD dwResult = 0;
	HRESULT hr = pDSBuf8->SetFX(1, &dsfx, &dwResult);
	if(SUCCEEDED(hr))
	{
	    pDSBuf8->GetObjectInPath(CLSID_HXAUDIODEVICEHOOK, 0,
		IID_IHXAudioDeviceHookDMO, (void **)&m_pAudioHookDMO );
	    if(m_pAudioHookDMO)
	    {
		res = m_pAudioHookDMO->Init(m_pContext);
	    }
	}
	else
	    res = HXR_FAIL;

	HX_RELEASE(pDSBuf8);
    }
#endif

    return res;
}

typedef HRESULT (STDAPICALLTYPE *FPDLLREGSVR)(void);

HX_RESULT CHXAudioDeviceDS::RegisterDirectSoundFilter()
{
    HX_RESULT res = HXR_OK;

#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    DLLAccess dllAccess;
    CHXString strPath;

    if(SUCCEEDED(GetFilterPathFromRegistry(strPath)))
    {
	if(dllAccess.open(strPath) == DLLAccess::DLL_OK &&
	    dllAccess.getSymbol("DllGetClassObject") != NULL)
	    return HXR_OK;
    }

    strPath = GetDLLAccessPath()->GetPath(DLLTYPE_COMMON);
    if(strPath.Right(1) != "\\")
	strPath += '\\';
    strPath += "HXAudioDeviceHook.dll";

    if(dllAccess.open(strPath) != DLLAccess::DLL_OK)
	return HXR_FAIL;

    FPDLLREGSVR fpDllRegisterServer = (FPDLLREGSVR) dllAccess.getSymbol("DllRegisterServer");
    if(fpDllRegisterServer == NULL)
	return HXR_FAIL;
    
    res = fpDllRegisterServer();
#endif
    return res;

}

HX_RESULT CHXAudioDeviceDS::GetFilterPathFromRegistry(CHXString & rstrPath)
{
    rstrPath = "";
#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    LPOLESTR pszClsId = NULL;
    if(FAILED(StringFromCLSID(CLSID_HXAUDIODEVICEHOOK, &pszClsId)))
	return HXR_OUTOFMEMORY;

    char szKey[256];
    sprintf(szKey, "CLSID\\%s\\InprocServer32", (const char *)HLXOsStrW(pszClsId));
    CoTaskMemFree(pszClsId);

    HKEY hKey = NULL;
    DWORD dwType = 0;
    BYTE szValue[_MAX_PATH];
    DWORD cbValue = _MAX_PATH;
    LONG lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey);
    if(lRet != ERROR_SUCCESS)
	return HXR_FAIL;
    lRet = RegQueryValueEx(hKey, "", NULL, &dwType, szValue, &cbValue);
    RegCloseKey(hKey);
    if(lRet != ERROR_SUCCESS || dwType != REG_SZ)
    {
	return HXR_FAIL;
    }

    rstrPath = (LPCSTR)szValue;
#endif
    return HXR_OK;
}

HX_RESULT CHXAudioDeviceDS::_Imp_Close()
{
    return InternalClose();
}

HX_RESULT CHXAudioDeviceDS::InternalClose()
{
    HX_RESULT	theErr = HXR_OK;

    KillThreadAndEvent();

    HX_RELEASE(m_pAudioHookDMO);
    HX_RELEASE(m_pPrimaryBuffer);
    HX_RELEASE(m_pSecondaryBuffer);
    HX_RELEASE(m_pDSDev);
    m_pAudioPtrStart = NULL;
    m_ulLastPlayCursor = 0;
    m_ulLastWriteCursor =0;
    m_eState = E_DEV_CLOSED;
    m_ulLoops = 0;
    m_ulLoopTime = 0;
    m_nBlocksPerBuffer = 0;
    m_ulCurrPlayTime = 0;
	m_ulCurrLoopTime = 0;

    if (m_hwnd)
    {
	SetWindowLong(m_hwnd, GWL_USERDATA, NULL);
    }

    return HXR_OK;
}


HX_RESULT CHXAudioDeviceDS::_Imp_Pause()
{
    HX_RESULT	theErr = HXR_OK;

    m_bPaused = TRUE;
    if (m_pSecondaryBuffer)
    {
	m_pSecondaryBuffer->Stop();
    }

    m_eState = E_DEV_PAUSED;
    return HXR_OK;
}


HX_RESULT CHXAudioDeviceDS::_Imp_Resume()
{
    if (m_pSecondaryBuffer && m_pAudioPtrStart)
    {
	m_pSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

	// XXXKCR: This is a hack for DSound 8.1 (and probably 8.0) when using DMOs.
	// Without checking the status after beginning playback, there ends up
	// being a glitch in the audio every 8 seconds (the size of our buffer).
	DWORD dwStatus = 0;
	m_pSecondaryBuffer->GetStatus(&dwStatus);

	if(m_bPaused)
	{
	    m_bPaused = FALSE;
	}

	m_eState = E_DEV_RESUMED;

	// Call to OnTimeSync can kill audio device.
	// We need to either self-addref or make sure member variables
	// are not accessed after OnTimeSync call.
	OnTimeSync();
    }
    else
    {
	m_eState = E_DEV_RESUMED;
    }

    return HXR_OK;
}

HX_RESULT CHXAudioDeviceDS::_Imp_Write(const HXAudioData* pAudioHdr)
{
    HRESULT res ;

    HXBOOL bFireOnTimeSync = FALSE;
    IHXBuffer* pBuffer = pAudioHdr->pData;

    UINT32 ulBufSize = pBuffer->GetSize();

    void* pAudioPtr1	= NULL;
    void* pAudioPtr2	= NULL;
    DWORD ulAudioBytes1 = 0;
    DWORD ulAudioBytes2 = 0;

    res = m_pSecondaryBuffer->Lock(m_ulLastWriteCursor, ulBufSize, &pAudioPtr1, &ulAudioBytes1, 
	&pAudioPtr2, &ulAudioBytes2, 0);

    if(res != DS_OK)
    {
	return HXR_FAIL ;
    }

    HX_ASSERT(ulBufSize = ulAudioBytes1+ulAudioBytes2);

    m_ulLastWriteCursor += ulBufSize ;

    if (m_ulLastWriteCursor >= m_ulTotalBuffer)
	m_ulLastWriteCursor -= m_ulTotalBuffer;

    if(pAudioPtr1)
    {
	::memcpy(pAudioPtr1, (void*) pBuffer->GetBuffer(), ulAudioBytes1); /* Flawfinder: ignore */

	if(!m_pAudioPtrStart)
	{
	    m_pAudioPtrStart = pAudioPtr1;
	    m_ulLoops = 0;
	    m_pSecondaryBuffer->SetCurrentPosition(0);

            // resume has been called
            if (E_DEV_RESUMED == m_eState && m_pSecondaryBuffer)
            {
	        m_pSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
	        if(m_bPaused)
	        {
	            m_bPaused = FALSE;
	        }
		bFireOnTimeSync = TRUE;
            }
	}
    }

    if (pAudioPtr2)
	::memcpy(pAudioPtr2, ((char*)pBuffer->GetBuffer()) + ulAudioBytes1 , ulAudioBytes2); /* Flawfinder: ignore */

    res = m_pSecondaryBuffer->Unlock(pAudioPtr1, ulAudioBytes1, pAudioPtr2, ulAudioBytes2);

    if (bFireOnTimeSync)
    {
	// Call to OnTimeSync can kill audio device.
	// We need to either self-addref or make sure member variables
	// are not accessed after OnTimeSync call.
	OnTimeSync();
    }

    if(res != DS_OK)
    {
	return HXR_FAIL ;
    }

    return HXR_OK;
}

HX_RESULT CHXAudioDeviceDS::_Imp_SetVolume(const UINT16 uVolume)
{
    LONG lVol = 0;
    m_uCurVolume = uVolume;
    if( m_uCurVolume == 0)
	lVol = DSBVOLUME_MIN;
    else
    {
	double dVolFromMin = (double)m_uCurVolume - m_uMinVolume;
	double dVolFrac = (dVolFromMin/(m_uMaxVolume - m_uMinVolume));
	lVol = (LONG)(1055.0 * log(dVolFrac));
    }
    if(m_pSecondaryBuffer)
	m_pSecondaryBuffer->SetVolume(lVol);
    return HXR_OK;
}

UINT16 CHXAudioDeviceDS::_Imp_GetVolume()
{
    LONG lVolume;

    if (!m_pSecondaryBuffer)
	return m_uMaxVolume ;

    m_pSecondaryBuffer->GetVolume(&lVolume);

    return (UINT16)(exp(lVolume / 1055.0) * (m_uMaxVolume - m_uMinVolume) + m_uMinVolume) ;
}

HX_RESULT CHXAudioDeviceDS::_Imp_Reset()
{
    if ( NULL == m_pSecondaryBuffer )
    {
	return HXR_OK;
    }

    void* pAudioPtr1	= NULL;
    void* pAudioPtr2	= NULL;
    DWORD ulAudioBytes1 = 0;
    DWORD ulAudioBytes2 = 0;

    HRESULT result = m_pSecondaryBuffer->Lock(0, 0, &pAudioPtr1, &ulAudioBytes1,&pAudioPtr2, &ulAudioBytes2, DSBLOCK_ENTIREBUFFER);
    if(result == DS_OK)
    {
	::ZeroMemory(pAudioPtr1, ulAudioBytes1);
	::ZeroMemory(pAudioPtr2, ulAudioBytes2);

        m_ulLastWriteCursor = 0;
	m_ulCurrPlayTime = 0;
        m_ulCurrLoopTime = 0;
	m_ulLoops = 0;
	m_ulLastPlayCursor = 0;
	m_pAudioPtrStart = pAudioPtr1;

        m_pSecondaryBuffer->Unlock(pAudioPtr1, ulAudioBytes1, pAudioPtr2, ulAudioBytes2);
	m_pSecondaryBuffer->SetCurrentPosition(0);

    }

    return HXR_OK;
}

HX_RESULT CHXAudioDeviceDS::_Imp_Drain()
{
    return HXR_OK;
}

HX_RESULT CHXAudioDeviceDS::_Imp_CheckFormat( const HXAudioFormat* pFormat )
{
    return HXR_OK;
}

HX_RESULT CHXAudioDeviceDS::_Imp_GetCurrentTime(ULONG32& ulCurrentTime)
{
    DWORD dwCurrentPlayCursor = 0;
    DWORD dwCurrentWriteCursor = 0;
    HRESULT result;

    ulCurrentTime = m_ulCurrPlayTime;
    if (m_pSecondaryBuffer)
    {
	result = m_pSecondaryBuffer->GetCurrentPosition(&dwCurrentPlayCursor, 
							&dwCurrentWriteCursor);
	if (result == DS_OK)
	{
	    UINT32 uLast = m_ulCurrPlayTime;
	    if(dwCurrentPlayCursor != m_ulLastPlayCursor)
	    {
		if( (dwCurrentPlayCursor < m_ulLastPlayCursor) && ((m_ulLastPlayCursor-dwCurrentPlayCursor) > (m_ulTotalBuffer/2))  )
		{
		    m_ulLoops++;
		    m_ulCurrPlayTime = m_ulCurrLoopTime = (UINT32) (m_ulLoopTime * 1000.0 * m_ulLoops);
		    m_ulLastPlayCursor = 0;
		}

		// Time can only move forward
		if (dwCurrentPlayCursor > m_ulLastPlayCursor)
		{
		    ULONG32 ulSamplesPlayedThisLoop = 
			dwCurrentPlayCursor / (m_pWaveFormat->wBitsPerSample/8 * m_pWaveFormat->nChannels);

		    m_ulCurrPlayTime = m_ulCurrLoopTime + 
			m_TSConverter.ConvertVector(ulSamplesPlayedThisLoop);

		    m_ulLastPlayCursor = dwCurrentPlayCursor;
		}

		ulCurrentTime = m_ulCurrPlayTime;
	    }
	}
    }

    return HXR_OK;
}

UINT32 
CHXAudioDeviceDS::CalcMs(UINT32 ulNumBytes)
{
    return (ulNumBytes * 1000UL) / m_pWaveFormat->nAvgBytesPerSec ;
}

DWORD
CHXAudioDeviceDS::defaultChannelMapping(UINT32 ulChannels) const
{
    switch (ulChannels)
    {
    case 1:
	return SPEAKER_FRONT_CENTER ;
    case 2:
	return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT ;
    case 5:
	return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
	    SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_CENTER ;
    case 6:
	return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
	    SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_CENTER |
	    SPEAKER_LOW_FREQUENCY;
    }
    return 0 ;
}

void 
CHXAudioDeviceDS::KillThreadAndEvent()
{
    DWORD dwThreadWaitResult = WAIT_FAILED;

    if(m_hDSNotifyEvent)
    {
	m_bExitThread = TRUE;
	::SetEvent(m_hDSNotifyEvent);	
	// Wait for thread to exit
	if ( m_hWaitThread )
	{
	    dwThreadWaitResult = WaitForSingleObject(m_hWaitThread, kExitThreadWaitTime);
	}
	CloseHandle(m_hDSNotifyEvent);
	m_hDSNotifyEvent = NULL;
    }

    if(m_hWaitThread)
    {
	if ( dwThreadWaitResult != WAIT_OBJECT_0 )
	{
	    ::TerminateThread(m_hWaitThread, -1 );
	}
	CloseHandle(m_hWaitThread);
	m_hWaitThread = NULL;
    }
    m_bExitThread = FALSE;
}

void 
CHXAudioDeviceDS::PostTimeSyncMessage()
{
    ::PostMessage(m_hwnd, HXMSG_TIMESYNC, 0, 0);
}

DWORD WINAPI CHXAudioDeviceDS::EventThreadProc(LPVOID pVoid)
{
    CHXAudioDeviceDS* pThis = (CHXAudioDeviceDS*)pVoid;
    if(!pThis)
	return 0;


    HANDLE hWaitEvent = pThis->GetEventHandle();
    if(!hWaitEvent)
	return 0;

    while(1)
    {
	DWORD dwReturn = WaitForMultipleObjects(1, &hWaitEvent, FALSE, INFINITE);
   
	if(pThis->GetExitCode())
	{
	    return 0;
	}
	if (dwReturn != (WAIT_OBJECT_0 + 1))
	{
	    // Post message to the window so that it can call OnTimeSync on the audio thread( on which the window was created )
	    // and then reset the event
	    pThis->PostTimeSyncMessage();
	    ResetEvent(hWaitEvent);
	}

    }
    return 0;
}

static LRESULT CALLBACK HXDSWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == HXMSG_TIMESYNC)
    {
	CHXAudioDeviceDS* pThis = (CHXAudioDeviceDS*)GetWindowLong(hWnd, GWL_USERDATA);
	if(pThis)
	    pThis->OnTimeSync();

	return 0;
    }
    else if (message == WM_CREATE)
    {
        CoInitialize(NULL);
        return 0;
    }
    else if (message == CHXAudioDeviceDS::zm_uDestroyMessage)
    {
	LRESULT result = (LRESULT)DestroyWindow(hWnd);

	// free the memory used by this class now that our window is destroyed
	UnregisterClass(szWindowClass, g_hInstance);

        CoUninitialize();

	return result;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
