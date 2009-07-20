/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: wcorecom.cpp,v 1.5 2005/03/14 20:31:04 bobclark Exp $
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

#include "hlxclib/windows.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "hxcleng.h"
#include "hxcorcom.h"
#include "platform/win/wcorecom.h"
#include <tchar.h>



#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifndef WIN32_PLATFORM_PSPC
extern HINSTANCE g_hInstance;
#else
extern HMODULE g_hInstance;
#endif

#define CORECOMM_CLASS		"HXEngineCommInternal"
#define CORECOMM_NAME		CORECOMM_CLASS
#define CORECOMM_STOPAUDIOMSG	"HXEngineStopAudioInternalMsg"
#define CORECOMM_RELOADPLUGINS	"HXEngineReloadPlugins"
#define CORECOMM_UNLOADPLUGINS	"HXEngineUnloadPlugins"
#define OLDWND_CLASS		"RealAudioInternal"

UINT32 WinCoreComm::m_uStopAudioMsg	= 0;
UINT32 WinCoreComm::m_uUnloadPluginsMsg = 0;
UINT32 WinCoreComm::m_uReloadPluginsMsg = 0;

// --------------------------------------------------------------------------
//  constructor
// --------------------------------------------------------------------------

WinCoreComm::WinCoreComm(HXClientEngine* pEngine)
    : HXCoreComm(pEngine)
    , m_hWnd(NULL)
    , m_bReady(FALSE)
#ifdef _WIN32
    , m_ulOriginalThreadId(0)
#endif /* _WIN32 */
{
    Init();
}


// --------------------------------------------------------------------------
//  destructor
// --------------------------------------------------------------------------

WinCoreComm::~WinCoreComm()
{
    Shutdown();
}


// --------------------------------------------------------------------------
//  StopAllOtherAudioPlayers
//
//  Stop the other players in all other processes.
// --------------------------------------------------------------------------

STDMETHODIMP
WinCoreComm::StopAllOtherAudioPlayers()
{
    EnumAction ea;
    ea.pComm = this;
    ea.uAction = WCCACTION_STOPAUDIO;

    EnumWindows((WNDENUMPROC)CoreCommEnumProc, (LPARAM)&ea);

    return HXR_OK;
}

// --------------------------------------------------------------------------
//  AskAllOtherPlayersToUnload
//
//  Asks players in all other processes to stop and then unload all of the
//  DLLs not in use.
// --------------------------------------------------------------------------

STDMETHODIMP
WinCoreComm::AskAllOtherPlayersToUnload()
{
    EnumAction ea;
    ea.pComm = this;
    ea.uAction = WCCACTION_UNLOADPLUGINS;

    EnumWindows((WNDENUMPROC)CoreCommEnumProc, (LPARAM)&ea);

    return HXR_OK;
}

// --------------------------------------------------------------------------
//  AskAllOtherPlayersToReload
//
//  Asks players in all other processes to reload all DLLs
// --------------------------------------------------------------------------

STDMETHODIMP
WinCoreComm::AskAllOtherPlayersToReload()
{
    EnumAction ea;
    ea.pComm = this;
    ea.uAction = WCCACTION_RELOADPLUGINS;

    EnumWindows((WNDENUMPROC)CoreCommEnumProc, (LPARAM)&ea);

    return HXR_OK;
}


// --------------------------------------------------------------------------
//  Init
//
//  Register the window class, create a hidden window for callbacks.
// --------------------------------------------------------------------------

HXBOOL
WinCoreComm::Init()
{
    if (!m_bReady && !m_hWnd)
    {
	if (!m_uStopAudioMsg)
	{
	    m_uStopAudioMsg = RegisterWindowMessage(OS_STRING(CORECOMM_STOPAUDIOMSG));
	    if (!m_uStopAudioMsg)
	    {
		goto exit;
	    }
	}

	if (!m_uUnloadPluginsMsg)
	{
	    m_uUnloadPluginsMsg = RegisterWindowMessage(OS_STRING(CORECOMM_UNLOADPLUGINS));
	    if (!m_uUnloadPluginsMsg)
	    {
		goto exit;
	    }
	}

	if (!m_uReloadPluginsMsg)
	{
	    m_uReloadPluginsMsg = RegisterWindowMessage(OS_STRING(CORECOMM_RELOADPLUGINS));
	    if (!m_uReloadPluginsMsg)
	    {
		goto exit;
	    }
	}

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc	    = (WNDPROC)WinCoreComm::CoreCommWndProc;
	wc.hInstance	    = g_hInstance;
	wc.lpszClassName    = TEXT(CORECOMM_CLASS);

	if (RegisterClass(&wc))
	{
	    m_hWnd = CreateWindow(OS_STRING(CORECOMM_CLASS),  // class name
				  OS_STRING(CORECOMM_NAME),   // window name
				  WS_POPUP,	    // style
				  CW_USEDEFAULT,    // x
				  CW_USEDEFAULT,    // y
				  CW_USEDEFAULT,    // cx
				  CW_USEDEFAULT,    // cy
				  NULL,		    // parent
				  NULL,		    // menu
				  g_hInstance,	    // instance
				  NULL);    // create params
#ifdef _WIN32
	    m_ulOriginalThreadId = GetCurrentThreadId();
#endif
	}

	if (m_hWnd)
	{
	    // Store a pointer to ourself within the Window
#ifdef _WIN32	    
	    SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);
#else
	    SetWindowLong(m_hWnd, DWL_USER, (LONG)this);
#endif /* _WIN32 */
	    m_bReady = TRUE;
	}
    }

exit:
    return m_bReady;
}


// --------------------------------------------------------------------------
//  Shutdown
//
//  Close the window and unregister the window class.
// --------------------------------------------------------------------------

HXBOOL
WinCoreComm::Shutdown()
{
    if (m_bReady)
    {
	if (m_hWnd)
	{
#ifdef _WIN32
	    if (m_ulOriginalThreadId == GetCurrentThreadId())
	    {
		DestroyWindow(m_hWnd);
	    }
	    else
	    {
		SendMessage(m_hWnd, WM_CLOSE, 0, 0);
	    }
#else
	    DestroyWindow(m_hWnd);
#endif
	    m_hWnd = NULL;
	}

	UnregisterClass(OS_STRING(CORECOMM_CLASS), g_hInstance);

	m_bReady = FALSE;
    }

    return TRUE;
}


// --------------------------------------------------------------------------
//  CoreCommWndProc
//
//  Handle messages sent to the Core comm object from other Core comm 
//  objects.
// --------------------------------------------------------------------------

LRESULT CALLBACK
WinCoreComm::CoreCommWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == m_uStopAudioMsg || uMsg == m_uUnloadPluginsMsg || 
	uMsg == m_uReloadPluginsMsg)
    {
	// Retrieve a pointer to the WinCoreComm from the GWL_USER area
#ifdef _WIN32
	WinCoreComm* pComm = (WinCoreComm*)GetWindowLong(hWnd, GWL_USERDATA);
#else
	WinCoreComm* pComm = (WinCoreComm*)GetWindowLong(hWnd, DWL_USER);
#endif /* _WIN32 */
	if (pComm)
	{
	    if (uMsg == m_uStopAudioMsg)
	    {
		return pComm->StopAudioPlayback();
	    }
	    if (uMsg == m_uUnloadPluginsMsg)
	    {
		return pComm->UnloadPlugins();
	    }
	    if (uMsg == m_uReloadPluginsMsg)
	    {
		return pComm->ReloadPlugins();
	    }
	}
    }
    else if (uMsg == WM_CLOSE)
    {
	DestroyWindow(hWnd);
	return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


// --------------------------------------------------------------------------
//  CoreCommEnumProc
//
//  Enumerate windows and look for PN internal Core and Core comm windows
//  to communicate with.
// --------------------------------------------------------------------------

HXBOOL CALLBACK
WinCoreComm::CoreCommEnumProc(HWND hWnd, LPARAM lParam)
{
    EnumAction* pEa = (EnumAction*)lParam;

    // Enumerate until ProcessWindow tells us to stop
    return pEa->pComm->ProcessWindow(hWnd, pEa->uAction);
}


// --------------------------------------------------------------------------
//  ProcessWindow
//
//  Called by the CoreCommEnumProc to handle an individual window.
// --------------------------------------------------------------------------

HXBOOL
WinCoreComm::ProcessWindow(HWND hWnd, UINT32 uAction)
{
    if (hWnd != m_hWnd)
    {
	// Only tell windows other than ours to perform the action
	switch (uAction)
	{
	    case WCCACTION_STOPAUDIO:
	    {
		StopAudioWindow(hWnd);
	    }
	    break;
	    case WCCACTION_UNLOADPLUGINS:
	    {
		UnloadWindowPlugins(hWnd);
	    }
	    break;
	    case WCCACTION_RELOADPLUGINS:
	    {
		ReloadWindowPlugins(hWnd);
	    }
	    break;
	default:
	    // Unknown action- stop enumerating.
	    return FALSE;
	}
    }

    // keep enumerating
    return TRUE;
}


// --------------------------------------------------------------------------
//  StopAudioWindow
//
//  Called by the ProcessWindow handler to direct a window to stop using the
//  audio.
// --------------------------------------------------------------------------

void
WinCoreComm::StopAudioWindow(HWND hWnd)
{
    char pClassName[256]; /* Flawfinder: ignore */

    if (GetClassName(hWnd, OS_STRING2(pClassName, 255), 255))
    {
	if (!strcmp(pClassName, CORECOMM_CLASS))
	{
	    // A 6.0 engine
	    SendMessage(hWnd, m_uStopAudioMsg, 0, 0);
	}
	else if (!strcmp(pClassName, OLDWND_CLASS))
	{
	    // a 4.0 or 5.0 audio window
	    SendMessage(hWnd, PWM_RELEASE_AUDIO_DEVICE, 0, 0);
	}
    }
}


// --------------------------------------------------------------------------
//  UnloadPlugins
//
//  Called by the ProcessWindow handler to direct a window to stop and then 
//  unload its plugins.
// --------------------------------------------------------------------------

void
WinCoreComm::UnloadWindowPlugins(HWND hWnd)
{
    char pClassName[256]; /* Flawfinder: ignore */

    if (GetClassName(hWnd, OS_STRING2(pClassName, 255), 255))
    {
	if (!strcmp(pClassName, CORECOMM_CLASS))
	{
	    // A 6.0 engine
	    SendMessage(hWnd, m_uUnloadPluginsMsg, 0, 0);
	}
    }
}

// --------------------------------------------------------------------------
//  ReloadPlugins
//
//  Called by the ProcessWindow handler to direct a window to reload plugins.
// --------------------------------------------------------------------------

void
WinCoreComm::ReloadWindowPlugins(HWND hWnd)
{
    char pClassName[256]; /* Flawfinder: ignore */

    if (GetClassName(hWnd, OS_STRING2(pClassName, 255), 255))
    {
	if (!strcmp(pClassName, CORECOMM_CLASS))
	{
	    // A 6.0 engine
	    SendMessage(hWnd, m_uReloadPluginsMsg, 0, 0);
	}
    }
}
