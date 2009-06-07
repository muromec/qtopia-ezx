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
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/****************************************************************************
 * 
 *  Test Client:
 *  
 *
 *  This is an test client running on Windows, Mac, and Unix without a GUI.
 *
 */ 

#ifndef _SPLAY_GLOBALS_H
#define _SPLAY_GLOBALS_H

#include "dllacces.h"
#include "dllpath.h"
#include "hxengin.h"
#include "hxplayvelocity.h"
#include "hxcore.h"
#include "ihxmedpltfm.h"

#define HX_COMMAND_BUFFER_SIZE 256
#define HX_MAX_NUM_MULTI_SEEKS 10

struct _stGlobals
{
    _stGlobals()
        : g_Players(NULL),
	  g_nPlayers(0),
	  m_fpHXMediaPlatformOpen(NULL),
	  m_fpHXCreateMediaPlatform(NULL),
	  m_fpHXMediaPlatformClose(NULL),
	  m_fpCreateEngine(NULL),
	  m_fpCloseEngine(NULL),
	  m_fpSetDLLAccessPath(NULL),
	  bEnableAdviceSink(FALSE),
          bEnableVerboseMode(FALSE),
          g_bEnableSlowStart(TRUE),
          g_bOnBeginOccurred(FALSE),
          g_pszUsername( NULL),
          g_pszPassword(NULL),
          g_pszGUIDFile(NULL),
          g_pszGUIDList(NULL),
          g_Error(HXR_OK),
	  g_ulNumSecondsPlayed(0),
	  pMediaPlatform(NULL),
          g_pIHXKicker(NULL),
          bUseLegacyAPIs(FALSE),
	  pEngine(NULL),
          g_pQuickSeek(NULL),
          g_bNullRender(FALSE),
          g_bUserStop(FALSE),
          g_bInitVelocity(FALSE),
          g_bABD(FALSE),
          g_pVelocityCaps(NULL),
          g_ulNumMultiSeeks(0),
          g_ulMultiSeekIndex(0),
          g_bMultiSeek(FALSE)
    {
        memset(g_szCommandBuffer, 0, HX_COMMAND_BUFFER_SIZE);
        memset(g_szPriorCommandBuffer, 0, HX_COMMAND_BUFFER_SIZE);
    }

    IHXPlayer**              g_Players;
    int                      g_nPlayers;
    DLLAccessPath            g_statclnt;

    FPHXMEDIAPLATFORMOPEN    m_fpHXMediaPlatformOpen;
    FPHXCREATEMEDIAPLATFORM  m_fpHXCreateMediaPlatform;
    FPHXMEDIAPLATFORMCLOSE   m_fpHXMediaPlatformClose;

    FPRMCREATEENGINE         m_fpCreateEngine;
    FPRMCLOSEENGINE          m_fpCloseEngine;
    FPRMSETDLLACCESSPATH     m_fpSetDLLAccessPath;

    HXBOOL                   bEnableAdviceSink;
    HXBOOL                   bEnableVerboseMode;
    HXBOOL                   g_bEnableSlowStart;
    HXBOOL                   g_bOnBeginOccurred;
    char*                    g_pszUsername;
    char*                    g_pszPassword;
    char*                    g_pszGUIDFile;
    char*                    g_pszGUIDList;
    HX_RESULT                g_Error;
    UINT32                   g_ulNumSecondsPlayed;
    IHXMediaPlatform*	     pMediaPlatform;
    IHXMediaPlatformKicker*  g_pIHXKicker;
    HXBOOL                   bUseLegacyAPIs;
    IHXClientEngine*         pEngine;
    IHXQuickSeek*            g_pQuickSeek;
    HXBOOL                   g_bNullRender;
    HXBOOL                   g_bUserStop;
    HXBOOL                   g_bInitVelocity;
    HXBOOL                   g_bABD;
    IHXPlaybackVelocityCaps* g_pVelocityCaps;
    HXBOOL                   g_bMultiSeek;
    UINT32                   g_ulMultiSeekTime[HX_MAX_NUM_MULTI_SEEKS];
    UINT32                   g_ulNumMultiSeeks;
    UINT32                   g_ulMultiSeekIndex;
    char                     g_szCommandBuffer[HX_COMMAND_BUFFER_SIZE];
    char                     g_szPriorCommandBuffer[HX_COMMAND_BUFFER_SIZE];
};


#endif // _SPLAY_GLOBALS_H
