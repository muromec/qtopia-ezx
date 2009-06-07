/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: wcorecom.h,v 1.3 2005/03/14 20:31:05 bobclark Exp $
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

#ifndef __WCORECOM_H
#define __WCORECOM_H


// --------------------------------------------------------------------------
//  WinCoreComm
//
//  Windows specific class for communicating with other audio sessions.
// --------------------------------------------------------------------------


class WinCoreComm : public HXCoreComm
{
friend class HXCoreComm;

public:

    STDMETHOD(StopAllOtherAudioPlayers)	    (THIS_);
    STDMETHOD(AskAllOtherPlayersToUnload)   (THIS_);
    STDMETHOD(AskAllOtherPlayersToReload)   (THIS_);

protected:
    WinCoreComm(HXClientEngine* pEngine);
    ~WinCoreComm();

    HXBOOL    Init();
    HXBOOL    Shutdown();

    HXBOOL    ProcessWindow(HWND hWnd, UINT32 uAction);
    void    StopAudioWindow(HWND hWnd);
    void    UnloadWindowPlugins(HWND hWnd);
    void    ReloadWindowPlugins(HWND hWnd);

    HWND    m_hWnd;
    HXBOOL    m_bReady;

#ifdef _WIN32
    ULONG32 m_ulOriginalThreadId;
#endif

    static LRESULT CALLBACK CoreCommWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HXBOOL CALLBACK    CoreCommEnumProc(HWND hWnd, LPARAM lParam);
    static UINT32	    m_uStopAudioMsg;
    static UINT32	    m_uUnloadPluginsMsg;
    static UINT32	    m_uReloadPluginsMsg;
};



//      PWMs (private window messages)
//  These were taken from the 4.0 player
enum
{
        PWM_POSTED_IDLE = WM_USER + 0xCD,
        PWM_POSTED_ERROR,
        PWM_ASYNC_SELECT,
        PWM_RELEASE_AUDIO_DEVICE,
	PWM_UNLOAD_PLUGINS,
	PWM_RELOAD_PLUGINS
};



#define WCCACTION_STOPAUDIO	0   // tell the enum proc to send a stop message
#define WCCACTION_UNLOADPLUGINS 1   // tell the enum proc to stop and then unload all plugins
#define WCCACTION_RELOADPLUGINS 2   // tell the enum proc to reload all plugins

// state info for the EnumWindows proc
typedef struct
{
    WinCoreComm*    pComm;
    UINT32	    uAction;
} EnumAction;

#endif
