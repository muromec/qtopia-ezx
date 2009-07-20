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
#include "hxtypes.h"

#include <stdlib.h>

#include "hlxclib/time.h"
#include "ihxmedpltfm.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxclsnk.h"
#include "hxgroup.h"
#include "hxerror.h"
#include "hxauth.h"
#include "hxwin.h"
#include "hxprefs.h"
#include "hxtbuf.h"
#include "fivemmap.h"
#include "dllacces.h"
#include "hxausvc.h"
#include "hxstrutl.h"
#include "hxgroup.h"
#include "hxwin.h"
#include "hxtick.h"
#include "hxbuffer.h"
#include "hxplayvelocity.h"
#include "pckunpck.h"  //  For CreateAndSetBufferCCF()
#include "hxvctrl.h"

#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
#include "hxpfs.h"
#endif //  HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.

#if defined(HELIX_FEATURE_PREFERENCES)
#include "preflist.h"
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */
#include "exadvsnk.h"
#include "exerror.h"
#include "exsitsup.h"
#include "exaumgr.h"
#if defined(_WIN32)
#include "exabd.h"
#endif
#include "excontxt.h"
#include "print.h"
#if defined(USE_XWINDOWS)
#include <X11/Xlib.h>
#endif
#if defined (_UNIX)
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#endif
#if defined(_WINDOWS)
#include <conio.h>
#endif

#ifdef __TCS__
#include <unistd.h>
#if defined(__cplusplus)
extern      "C"     {
#endif      /* defined(__cplusplus) */
    extern      unsigned long  tmosTimSleep(unsigned long ticks);
    unsigned long       gForcePlayerToStop = 0;
#if defined(__cplusplus)
}
#endif      /* defined(__cplusplus) */
#endif

#if defined (_MACINTOSH) || defined (_MAC_UNIX)
bool gMacPlayerIsDone = false;
#endif


#if defined _VXWORKS
#include "sys/times.h"
#include "string.h"
#include "hxtime.h"
#endif

#if defined(_MACINTOSH)
#ifndef _MAC_MACHO
#include <sioux.h>
#include <console.h>
#endif
#endif

#include "dllpath.h"

#ifdef _SYMBIAN
#include <e32svr.h>
#include "platform/symbian/symbian_event_hndlr.h"
#endif

ENABLE_DLLACCESS_PATHS(g_SPlayAccessPath);

// typedef for SetDLLAccessPath


#include "thrdutil.h"

#if defined(_AIX)
#include <strings.h>
#endif

#if defined _DEBUG || defined DEBUG
#include "debug.h"
#endif

#ifndef MAX_PATH
#define MAX_PATH    256
#endif //!MAX_PATH

#if defined(HELIX_CONFIG_NOSTATICS)
# include "globals/hxglobals.h"
#endif

#define MAX_NUM_URLS 10

#include "globals.h" //for global struct.

#if defined(_WINDOWS) && defined(_STATICALLY_LINKED)
HINSTANCE g_hInstance = NULL;
#endif

struct _stGlobals*& GetGlobal()
{
#if defined(HELIX_CONFIG_NOSTATICS)
    static const struct _stGlobals* const _g_pstGlobals = NULL;
    struct _stGlobals*& g_pstGlobals = (struct _stGlobals*&)HXGlobalPtr::Get(&_g_pstGlobals);
#else
    static struct _stGlobals* g_pstGlobals = NULL;
#endif
    if( g_pstGlobals == NULL )
    {
        g_pstGlobals = new struct _stGlobals();
    }
    return g_pstGlobals;

}


// Constants
const int DEFAULT_TIME_DELTA = 2000;
const int DEFAULT_STOP_TIME  = -1;
const int SLEEP_TIME         = 5;
const int GUID_LEN           = 64;

// Function prototypes
void  PrintUsage(const char* pszAppName);
HXBOOL  AllPlayersDone(int nNumPlayers, IHXPlayer** ppPlayers);
void  StopAllPlayers(int nNumPlayers, IHXPlayer** ppPlayers);
HXBOOL  ReadGUIDFile();
char* GetAppName(char* pszArgv0);

#ifdef __TCS__
#if defined(__cplusplus)
extern      "C"     {
#endif      /* defined(__cplusplus) */

    extern      void    _alloc_stats(int verbose);
#if defined(__cplusplus)
}
#endif      /* defined(__cplusplus) */

static  IHXPlayer** g_Players;
static  int         g_nPlayers = 0;
static  long        evtCount = 0;
static  long        evtFullCount = 0;
#endif

#ifdef _WIN32
#if !defined(WIN32_PLATFORM_PSPC)
void
ProperShutdown()
{
    int i;
    for (i = 0; i < GetGlobal()->g_nPlayers; i++)
    {
        GetGlobal()->g_Players[i]->Stop();
    }
}

HXBOOL
HandlerRoutine(DWORD dwType)
{
    switch (dwType)
    {
       case CTRL_C_EVENT:
       case CTRL_BREAK_EVENT:
       case CTRL_CLOSE_EVENT:
       case CTRL_LOGOFF_EVENT:
       case CTRL_SHUTDOWN_EVENT:
           ProperShutdown();
    }
    return TRUE;
}
#endif /* !defined(WIN32_PLATFORM_PSPC) */

#endif

#if defined(HELIX_FEATURE_VIDEO)

HX_RESULT GetFullScreenSite(IHXPlayer* pPlayer, REF(IHXSiteFullScreen*) rpSite)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pPlayer)
    {
        // Get the IHXSiteManager2 interface
        IHXSiteManager2* pSiteMgr2 = NULL;
        retVal = pPlayer->QueryInterface(IID_IHXSiteManager2, (void**) &pSiteMgr2);
        if (SUCCEEDED(retVal))
        {
            // Get the number of sites
            UINT32 ulNumSites = 0;
            retVal = pSiteMgr2->GetNumberOfSites(ulNumSites);
            if (SUCCEEDED(retVal))
            {
                // XXXMEH - for now, only allow this to 
                // work when there's only 1 site
                if (ulNumSites == 1)
                {
                    IHXSite* pSite = NULL;
                    retVal = pSiteMgr2->GetSiteAt(0, pSite);
                    if (SUCCEEDED(retVal))
                    {
                        IHXSiteFullScreen* pFSSite = NULL;
                        retVal = pSite->QueryInterface(IID_IHXSiteFullScreen, (void**) &pFSSite);
                        if (SUCCEEDED(retVal))
                        {
                            HX_RELEASE(rpSite);
                            rpSite = pFSSite;
                            rpSite->AddRef();
                        }
                        HX_RELEASE(pFSSite);
                    }
                    HX_RELEASE(pSite);
                }
                else
                {
                    retVal = HXR_FAIL;
                }
            }
        }
        HX_RELEASE(pSiteMgr2);
    }

    return retVal;
}

HX_RESULT GetAnySite(IHXPlayer* pPlayer, REF(IHXSite*) rpSite)
{
    HX_RESULT retVal = HXR_FAIL;

    if( pPlayer )
    {
        // Get the IHXSiteManager2 interface
        IHXSiteManager2* pSiteMgr2 = NULL;
        retVal = pPlayer->QueryInterface(IID_IHXSiteManager2, (void**) &pSiteMgr2);
        if (SUCCEEDED(retVal))
        {
            // Get the number of sites
            UINT32 ulNumSites = 0;
            retVal = pSiteMgr2->GetNumberOfSites(ulNumSites);
            if (SUCCEEDED(retVal))
            {
                if (ulNumSites != 0)
                {
                    IHXSite* pSite = NULL;
                    retVal = pSiteMgr2->GetSiteAt(0, pSite);
                    if (SUCCEEDED(retVal))
                    {
                        rpSite = pSite;
                        rpSite->AddRef();
                        pSite = NULL;
                    }
                    else
                    {
                        HX_ASSERT( "can't get a site"==NULL);
                    }
                }
                else
                {
                    retVal = HXR_FAIL;
                }
            }
        }
        HX_RELEASE(pSiteMgr2);
    }
    return retVal;
}

enum COLOR_CONTROL
{
    BRIGHTNESS,
    CONTRAST,
    HUE,
    SATURATTION,
};

float LimitIt(float fValue )
{
    float fTmp = fValue;
    if( fTmp < -1 )
    {
        fTmp = -1;
    }
    if( fTmp > 1 )
    {
        fTmp = 1;
    }
    return fTmp;
}

void SetColorControl( COLOR_CONTROL cc, float fAdjustment )
{
    IHXSite* pSite = NULL;
    HX_RESULT rv = GetAnySite(GetGlobal()->g_Players[0], pSite);
    if (SUCCEEDED(rv))
    {
        IHXVideoControl* pVC = NULL;
        rv = pSite->QueryInterface( IID_IHXVideoControl, (void**)&pVC);
        if( SUCCEEDED(rv) )
        {
            float fTmp = 0.0;
            switch( cc )
            {
               case BRIGHTNESS:
                   fTmp = pVC->GetBrightness();
                   fTmp += fAdjustment;
                   pVC->SetBrightness(LimitIt(fTmp));
                   break;
               case CONTRAST:
                   fTmp = pVC->GetContrast();
                   fTmp += fAdjustment;
                   pVC->SetContrast(LimitIt(fTmp));
                   break;
               case HUE:
                   fTmp = pVC->GetHue();
                   fTmp += fAdjustment;
                   pVC->SetHue(LimitIt(fTmp));
                   break;
               case SATURATTION:
                   fTmp = pVC->GetSaturation();
                   fTmp += fAdjustment;
                   pVC->SetSaturation(LimitIt(fTmp));
                   break;
               default:
                   HX_ASSERT("bad color command"==NULL);
                   break;
            };
            fprintf( stderr, "Brightness:%f  Contrast:%f  Hue:%f  Saturation:%f\n",
                     pVC->GetBrightness(),
                     pVC->GetContrast(),
                     pVC->GetHue(),
                     pVC->GetSaturation()
                     ); 
        }
        HX_RELEASE(pVC);
    }
    HX_RELEASE(pSite);
}


#endif /* #if defined(HELIX_FEATURE_VIDEO) */

#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
HX_RESULT
DumpPFdata(IHXPresentationFeatureManager* pPFMgr,
           const char* pszPFName)
{
    HX_RESULT hxr = HXR_OK;

    IHXBuffer* pFeatureCurrentSetting = NULL;
    IHXValues* pFeatureOptions = NULL;

    if (!pPFMgr  ||  !pszPFName)
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
        //  List all the options for this PF:
        HX_RESULT hxr = pPFMgr->GetPresentationFeature(
                pszPFName,
                pFeatureCurrentSetting,
                pFeatureOptions);

        if (FAILED(hxr))
        {
            //  PF doesn't exist!
            STDOUT("\t%s - No such presentation feature\n\n", pszPFName);
        }
        else
        {
            //  Now, go through pFeatureOptions and display
            // them and their "is-selectable"-flag values:
            const char* pszPFOptionName = NULL;
            const char* pszPFCurSetting = pFeatureCurrentSetting?
                    (const char*)pFeatureCurrentSetting->GetBuffer() : NULL;
            UINT32 bPFOptionIsSelectableFlag = FALSE;
            IHXBuffer* pPFOptionIsSelectableFlag = NULL;

            HX_ASSERT(pszPFCurSetting);
            STDOUT("\t%s\t(Current setting == \"%s\")\n", pszPFName, pszPFCurSetting?
                    pszPFCurSetting : "<ERROR: THIS PF HAS NO CURRENT SETTING!>");
            
            if (!pFeatureOptions) //  NULL is OK; that means freeform (open-ended):
            {
                STDOUT("\t\t(%s's options are freeform, i.e., open-ended)\n",
                        pszPFName);
            }
            else //  List all the options and their flags:
            {
                if (HXR_OK == pFeatureOptions->GetFirstPropertyCString(pszPFOptionName,
                        pPFOptionIsSelectableFlag)  &&  *pszPFOptionName)
                {
                    do
                    {
                        const UCHAR* pIsSelFlag = pPFOptionIsSelectableFlag->GetBuffer();
                        if (pIsSelFlag  &&  *pIsSelFlag)
                        {
                            //  Anything but '0' (zero) is treated as '1' (is selectable):
                            bPFOptionIsSelectableFlag = (HXBOOL)('0' != *pIsSelFlag  &&
                                    '\0' == *(pIsSelFlag+1));
                        }
                        else
                        {
                            //  Set the error, but we'll go ahead and treat it as '0':
                            hxr = HXR_INVALID_PARAMETER;
                            bPFOptionIsSelectableFlag = FALSE;
                        }

                        STDOUT("\t\t%s\t\tIs selectable: %s\n",
                                pszPFOptionName, bPFOptionIsSelectableFlag? "yes":"no");
                        //  Release it to get the next PF:
                        HX_RELEASE(pPFOptionIsSelectableFlag);
                    } while (HXR_OK == pFeatureOptions->GetNextPropertyCString(
                                    pszPFOptionName, pPFOptionIsSelectableFlag));
                }
            }

            HX_RELEASE(pPFOptionIsSelectableFlag);
        }
    }

    HX_RELEASE(pFeatureCurrentSetting);
    HX_RELEASE(pFeatureOptions);

    return hxr;
}
#endif //  HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.


/*
 *  handle one event
 */
void
DoEvent()
{
    char  ch = 0;
#if defined(_WINDOWS)
    MSG msg;

    GetMessage(&msg, NULL, 0, 0);
    DispatchMessage(&msg);

    // Check for key hit
    if (_kbhit())
    {
        ch = _getch();
    }
#elif defined (_UNIX) && !defined(_MAC_UNIX)

    struct _HXxEvent* pNothing = 0;
    struct timeval    mtime;
    fd_set fds;
    int retval;
    
    //To pass in events in Atlas you have two choices now. The first
    //is for backwards compatibility with Cayenne; calling
    //EventOccurred(). The second way is to ask the platform for the
    //event handler and to use that. Neither of these are strictly
    //needed because the site provides its own event gathering
    //code. You only need to do this if you have a special site or
    //want to do some custom event work.

    UINT32 sleepTime = 0;
    
    if( GetGlobal()->bUseLegacyAPIs )
    {
        //Compatibility with Cayenne based systems.
        GetGlobal()->pEngine->EventOccurred(pNothing);
    }
    else
    {
        //The new Atlas platform method.

        //Kick'ing the platforms gives time to all of its
        //schedulers. This is usefull for systems that do
        //not have threads enabled or do not have timers.
        IHXMediaPlatformKicker* pKicker = GetGlobal()->g_pIHXKicker;
        if( pKicker )
        {
            pKicker->Kick(HXGetCurrentThreadID(), &sleepTime);
        }

//        //This is how to pass events into the site if needed.
//         IHXMediaPlatform* pMediaPlatform = GetGlobal()->pMediaPlatform;
//         IHXSiteEventHandler* pHandler = NULL;
//         pMediaPlatform->QueryInterface(IID_IHXSiteEventHandler,
//                                        (void**)&pHandler);
//         pHandler->EventOccurred(pNothing);
//         HX_RELEASE(pHandler);
    }


    mtime.tv_sec  = 0;
    mtime.tv_usec = sleepTime;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    retval = select(1, &fds, NULL, NULL, &mtime);

    if(retval)
    {
        read(0, &ch, 1);
    }
    
#elif defined(__TCS__)
    _HXxEvent* pNothing = 0;

    // This is in units of 10ms.
    tmosTimSleep( 10 );
    IHXMediaPlatformKicker* pKicker = GetGlobal()->g_pIHXKicker;
    if( pKicker )
        pKicker->Kick(HXGetCurrentThreadID());
    evtCount++;

    if (evtCount > 80)
    {
        evtFullCount++;
        if (evtFullCount > 10)
        {
            _alloc_stats(1);
            evtFullCount = 0;
        }
        else
            _alloc_stats(0);
        evtCount = 0;
    }

#elif defined (_MACINTOSH) || defined(_MAC_UNIX)

    EventRecord event;

    if (WaitNextEvent(everyEvent, &event, 1, nil))
    {
	switch (event.what)
	{
		case keyDown:
			gMacPlayerIsDone = true;
			//ExitToShell();
			break;
		default:
			HXxEvent _HXxEvent;
			_HXxEvent.event  = ((EventRecord*)&event)->what;
			_HXxEvent.param1 = &event;
			_HXxEvent.param2 = NULL; // IMPORTANT! Must be NULL or a RgnHandle!
			GetGlobal()->pEngine->EventOccurred(&_HXxEvent);
	}
    }
    else
    {
	GetGlobal()->pEngine->EventOccurred(NULL);
    }
	
#elif defined(_SYMBIAN)
    // We have to allocate the object on the heap because
    // CActive does not appear to initialize iActive to 0
    // like it should
    SplayEventHandler* pEventHandler = new SplayEventHandler();
    pEventHandler->DoEvents(20); // Process events for 20ms
    delete pEventHandler;
#endif
    // Do we have a key hit?
    if (ch)
    {
        STDOUT("%c", ch);
        //  XXXEH- NOTE: I moved the {ch=tolower(ch)} conversion out of here
        // because the "pf" command is case-sensitive.  I moved the tolower()
        // conversion to the code, below, that looks at the contents of
        // g_szCommandBuffer for deciding what command to process.

        //Do forward and backward frame scrubbing at constant
        //rate
        if( '<' == ch )
        {
            ULONG32 ulNow = GetGlobal()->g_Players[0]->GetCurrentPlayTime();
	    GetGlobal()->g_pQuickSeek->QuickSeek((ulNow > 133) ? (ulNow - 133) : 0);
        }
        if( '>' == ch )
        {
            ULONG32 ulNow = GetGlobal()->g_Players[0]->GetCurrentPlayTime();
            GetGlobal()->g_pQuickSeek->QuickSeek(ulNow + 133);
        }

#if defined(HELIX_FEATURE_VIDEO)
        //Support for controlling the Brightness, Contrast and Hue. The keys
        //were chosen by ease of en-us keyboard layout and availability.
        //
        // Brightness:  . and /
        // Contrast..:  ; and '
        // Hue.......:  [ and ]
        // Saturation:  - and =
        //
        if( '.' == ch )
        {
            SetColorControl(BRIGHTNESS, -0.05 );
        }
        if( '/' == ch )
        {
            SetColorControl(BRIGHTNESS, 0.05 );
        }
        if( ';' == ch )
        {
            SetColorControl(CONTRAST, -0.05 );
        }
        if( '\'' == ch )
        {
            SetColorControl(CONTRAST, 0.05 );
        }
        if( '[' == ch )
        {
            SetColorControl(HUE, -0.05 );
        }
        if( ']' == ch )
        {
            SetColorControl(HUE, 0.05 );
        }
        if( '-' == ch )
        {
            SetColorControl(SATURATTION,  -0.05 );
        }
        if( '=' == ch )
        {
            SetColorControl(SATURATTION, 0.05 );
        }
#endif
        
        // Is this a carriage return or linefeed?
        if (ch == '\r' || ch == '\n')
        {
            // Process the command buffer
            //
            // Get the command buffer
            const char*  pszCmdBuffer = (char*) GetGlobal()->g_szCommandBuffer;
            UINT32       ulLen        = strlen(pszCmdBuffer);

            char ch0 = tolower(pszCmdBuffer[0]);
            char ch1 = ulLen>1? tolower(pszCmdBuffer[1]):'\0';

            // Switch on command
            if (ulLen == 1 && 'q' == ch0)
            {
                GetGlobal()->g_bUserStop = TRUE;
            }
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
            else if (ulLen > 1 && 'v' == ch0)
            {
                // This is a command to set the velocity
                //
                // Get the velocity
                const char* pszVelocity = pszCmdBuffer + 1;
                INT32 lVelocity         = 0;
                INT32 lKeyFrameMode     = 0;
                INT32 lAutoSwitch       = 0;
                INT32 lNumArg           = (INT32) sscanf(pszVelocity, "%ld,%ld,%ld",
                                                         &lVelocity, &lKeyFrameMode, &lAutoSwitch);
                if (lNumArg == 3)
                {
                    // Set the velocity in the FIRST player only
                    if (GetGlobal()->g_nPlayers >= 1)
                    {
                        IHXPlaybackVelocity* pPlaybackVelocity = NULL;
                        GetGlobal()->g_Players[0]->QueryInterface(IID_IHXPlaybackVelocity,
                                                                  (void**) &pPlaybackVelocity);
                        if (pPlaybackVelocity)
                        {
                            if (!GetGlobal()->g_bInitVelocity)
                            {
                                STDOUT("Initializing playback velocity in first player\n");
                                pPlaybackVelocity->InitVelocityControl(NULL); // don't need response for now
                                GetGlobal()->g_bInitVelocity = TRUE;
                            }
                            if (pPlaybackVelocity->GetVelocity() != lVelocity)
                            {
                                // Check if we can set this velocity
                                HXBOOL bCanSet = FALSE;
                                if (GetGlobal()->g_pVelocityCaps)
                                {
                                    bCanSet = GetGlobal()->g_pVelocityCaps->IsCapable(lVelocity);
                                }
                                if (bCanSet)
                                {
                                    STDOUT("Calling SetVelocity(%ld,%s,%s) in first player\n",
                                           lVelocity,
                                           (lKeyFrameMode ? "TRUE" : "FALSE"),
                                           (lAutoSwitch ? "TRUE" : "FALSE"));
                                    pPlaybackVelocity->SetVelocity(lVelocity, lKeyFrameMode, lAutoSwitch);
                                }
                                else
                                {
                                    STDOUT("This presentation not capable of setting velocity to %ld\n", lVelocity);
                                }
                            }
                        }
                        HX_RELEASE(pPlaybackVelocity);
                    }
                }
                else
                {
                    fprintf(stderr, "Velocity run-time command is \"vX,Y,Z\" where:\n"
                            "      X   is the velocity in integer percent of normal playback speed,\n"
                            "      Y   is 0 for keyframe mode = FALSE or 1 for keyframe mode = TRUE,\n"
                            "      Z   is 0 for autoswitch = FALSE or 1 for autoswitch = TRUE\n");
                }
            }
            else if (ulLen == 1 && 'k' == ch0)
            {
                // This is a command to go to toggle keyframe mode
                //
                // Resume the FIRST player only
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    IHXPlaybackVelocity* pPlaybackVelocity = NULL;
                    GetGlobal()->g_Players[0]->QueryInterface(IID_IHXPlaybackVelocity,
                                                              (void**) &pPlaybackVelocity);
                    if (pPlaybackVelocity &&
                        pPlaybackVelocity->GetVelocity() != HX_PLAYBACK_VELOCITY_NORMAL)
                    {
                        if (!pPlaybackVelocity->GetKeyFrameMode())
                        {
                            STDOUT("Setting keyframe mode to TRUE in first player\n");
                            pPlaybackVelocity->SetKeyFrameMode(TRUE);
                        }
                    }
                    HX_RELEASE(pPlaybackVelocity);
                }
            }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
            else if (ulLen == 1 && 'p'== ch0)
            {
                // This is a command to pause the player
                //
                // Pause the FIRST player only
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    STDOUT("Pausing the first player\n");
                    GetGlobal()->g_Players[0]->Pause();
                }
            }
            else if (ulLen == 1 && 'r' == ch0)
            {
                // This is a command to resume the player
                //
                // Resume the FIRST player only
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    STDOUT("Resuming the first player\n");
                    GetGlobal()->g_Players[0]->Begin();
                }
            }
            else if (ulLen == 1 && 't'== ch0)
            {
                // This is a command to get the current playback time
                if (GetGlobal()->g_nPlayers >= 1)
		{
                    STDOUT("Playback time for player 0 : %lu ms\n", 
				    GetGlobal()->g_Players[0]->GetCurrentPlayTime());
		}
            }
            else if (ulLen == 1 && 'm' == ch0)
            {
                // This is a command to mute/unmute the player
                //
                // Mute/Unmute the FIRST player only
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    IHXAudioPlayer* pAudioPlayer = NULL;
                    GetGlobal()->g_Players[0]->QueryInterface(IID_IHXAudioPlayer, (void**) &pAudioPlayer);
                    if (pAudioPlayer)
                    {
                        // Get the IHXVolume
                        IHXVolume* pVolume = pAudioPlayer->GetAudioVolume();
                        if (pVolume)
                        {
                            // Get the current value of mute
                            HXBOOL bMute = pVolume->GetMute();
                            // Flip it
                            pVolume->SetMute(!bMute);
                        }
                        HX_RELEASE(pVolume);
                    }
                    HX_RELEASE(pAudioPlayer);
                }
            }
            else if (ulLen == 1 && 'f' == ch0)
            {
                // This is a command to go to the next group
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    IHXGroupManager* pManager = NULL;
                    GetGlobal()->g_Players[0]->QueryInterface(IID_IHXGroupManager, (void**) &pManager);
                    if (pManager)
                    {
                        // Get the number of groups
                        UINT16 usNumGroups = pManager->GetGroupCount();
                        UINT16 usCurGroup  = 0;
                        if (usNumGroups > 1)
                        {
                            // Get the current group
                            pManager->GetCurrentGroup(usCurGroup);
                            // Make sure there is a next group to go to
                            if (usCurGroup + 1 < usNumGroups)
                            {
                                // Increment the group
                                usCurGroup++;
                                // Set that group as the current group
                                STDOUT("Switching to group %u of %u\n", usCurGroup, usNumGroups);
                                pManager->SetCurrentGroup(usCurGroup);
                            }
                            else
                            {
                                STDOUT("Cannot go to next group (numGroups=%u, curGroup=%u)\n", usNumGroups, usCurGroup);
                            }
                        }
                        else
                        {
                            STDOUT("Cannot go to next group (numGroups=%u, curGroup=%u)\n", usNumGroups, usCurGroup);
                        }
                    }
                    HX_RELEASE(pManager);
                }
            }
            else if (ulLen == 1 && 'b' == ch0)
            {
                // This is a command to go to the previous group
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    IHXGroupManager* pManager = NULL;
                    GetGlobal()->g_Players[0]->QueryInterface(IID_IHXGroupManager, (void**) &pManager);
                    if (pManager)
                    {
                        // Get the number of groups
                        UINT16 usNumGroups = pManager->GetGroupCount();
                        UINT16 usCurGroup  = 0;
                        if (usNumGroups > 1)
                        {
                            // Get the current group
                            pManager->GetCurrentGroup(usCurGroup);
                            // Make sure there is a previous group to go to
                            if (usCurGroup)
                            {
                                // Decrement the group
                                usCurGroup--;
                                // Set that group as the current group
                                STDOUT("Switching to group %u of %u\n", usCurGroup, usNumGroups);
                                pManager->SetCurrentGroup(usCurGroup);
                            }
                            else
                            {
                                STDOUT("Cannot go to previous group (numGroups=%u, curGroup=%u)\n", usNumGroups, usCurGroup);
                            }
                        }
                        else
                        {
                            STDOUT("Cannot go to previous group (numGroups=%u, curGroup=%u)\n", usNumGroups, usCurGroup);
                        }
                    }
                    HX_RELEASE(pManager);
                }
            }
            else if (ulLen > 1 && ch0 == 'l')
            {
                // This is a command to change the volume.
                const char* pszVolume = pszCmdBuffer + 1;
                UINT16      usVolume  = (UINT16) atol(pszVolume);
                // This value must be <= 100
                if (usVolume <= 100)
                {
                    // Change the volume of the FIRST player only
                    if (GetGlobal()->g_nPlayers >= 1)
                    {
                        IHXAudioPlayer* pAudioPlayer = NULL;
                        GetGlobal()->g_Players[0]->QueryInterface(IID_IHXAudioPlayer, (void**) &pAudioPlayer);
                        if (pAudioPlayer)
                        {
                            // Get the IHXVolume
                            IHXVolume* pVolume = pAudioPlayer->GetAudioVolume();
                            if (pVolume)
                            {
                                // Get the current volume
                                UINT16 usCurVolume = pVolume->GetVolume();
                                // Change the volume
                                STDOUT("Changing the volume of the first player from %u to %u\n", usCurVolume, usVolume);
                                pVolume->SetVolume(usVolume);
                            }
                            HX_RELEASE(pVolume);
                        }
                        HX_RELEASE(pAudioPlayer);
                    }
                }
                else
                {
                    STDOUT("Volume value must be less than or equal to 100.\n");
                }
            }
            else if (ulLen > 1 && 's' == ch0)
            {
                // Seek commands are in the form sXXXX where XXXX is the 
                // time in ms to seek to
                const char* pszSeekTime = pszCmdBuffer + 1;
                UINT32      ulSeekTime  = (UINT32) atol(pszSeekTime);
                // Seek the FIRST player only
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    STDOUT("Seeking the first player to %lu\n", ulSeekTime);
                    // First pause the player
                    GetGlobal()->g_Players[0]->Pause();
                    GetGlobal()->g_Players[0]->Seek(ulSeekTime);
                    GetGlobal()->g_Players[0]->Begin();
                }
            }
            else if (ulLen > 2 && 'q' == ch0 && 's' == ch1)
            {
                // Quick-Seek commands are in the form qsXXXX where XXXX is the 
                // time in ms to seek to
                const char* pszSeekTime = pszCmdBuffer + 2;
                UINT32      ulSeekTime  = (UINT32) atol(pszSeekTime);
                // Seek the FIRST player only
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    STDOUT("Quick-seeking the first player to %lu\n", ulSeekTime);
                    GetGlobal()->g_Players[0]->Seek(ulSeekTime);
                }
            }
            else if (ulLen > 2 && 'm' == ch0 && 's' == ch1)
            {
                // Multi-Seek commands are in the form msXXX,YYY,...,ZZZ where XXX,
                // YYY, and ZZZ are times to seek to.
                UINT32      ulNumSeekTimes = 0;
                const char* pszSeekTime    = pszCmdBuffer + 2;
                char*       pszToken       = strtok((char*) pszSeekTime, ",");
                while (pszToken && ulNumSeekTimes < HX_MAX_NUM_MULTI_SEEKS)
                {
                    GetGlobal()->g_ulMultiSeekTime[ulNumSeekTimes++] = (UINT32) atol(pszToken);
                    pszToken = strtok(NULL, ",");
                }
                // Seek the FIRST player only
                if (GetGlobal()->g_nPlayers >= 1)
                {
                    GetGlobal()->g_ulNumMultiSeeks  = ulNumSeekTimes;
                    GetGlobal()->g_ulMultiSeekIndex = 0;
                    GetGlobal()->g_bMultiSeek       = (ulNumSeekTimes > 1 ? TRUE : FALSE);
                    STDOUT("Multi-seeking the first player to %lu\n",
                           GetGlobal()->g_ulMultiSeekTime[GetGlobal()->g_ulMultiSeekIndex]);
                    GetGlobal()->g_Players[0]->Seek(GetGlobal()->g_ulMultiSeekTime[GetGlobal()->g_ulMultiSeekIndex++]);
                }
            }
#if defined(HELIX_FEATURE_VIDEO)
            else if (ulLen >= 3  &&  'f' == ch0  &&  's' == ch1)
            {
                // Get the site
                IHXSiteFullScreen* pSite = NULL;
                HX_RESULT rv = GetFullScreenSite(GetGlobal()->g_Players[0], pSite);
                if (SUCCEEDED(rv))
                {
                    // Are we coming or going from full-screen?
                    if (pszCmdBuffer[2] == '+')
                    {
                        pSite->EnterFullScreen();
                    }
                    else if (pszCmdBuffer[2] == '-')
                    {
                        pSite->ExitFullScreen();
                    }
                }
                HX_RELEASE(pSite);
            }
#if defined(HELIX_FEATURE_PNG)
            else if (ulLen >= 1 && 'c' == ch0)
            {
                IHXPlayer* pPlayer0 = GetGlobal()->g_Players[0];
                ExampleClientContext*  pExContext=NULL;
                ExampleSiteSupplier*   pExSiteSupplier=NULL;

                if (pPlayer0 && HXR_OK == pPlayer0->GetClientContext((IUnknown*&)pExContext) && 
                    pExContext && HXR_OK == pExContext->QueryInterface(IID_IHXSiteSupplier, (void**)&pExSiteSupplier) && 
                    pExSiteSupplier)
                {
                        CHXString pszFileName;
                        INT32 dWidth=0, dHeight=0;

                        // Image size 
                        char* strXY;
                        if (strXY=strchr((char*) pszCmdBuffer, ','))
                        {
                            *(strXY++) = '\0';
                        }

                        // File name
                        char* strC=(char*)pszCmdBuffer+1;
                        while (strC && isspace(*strC))
                        {
                            strC++;
                        }

                        if (*strC == '\0')
                        {
                            pszFileName = "image.png";
                        }
                        else
                        {
                            pszFileName = strC;
                        }

                        if (!strXY || *strXY == '\0' || sscanf(strXY, "%ld,%ld", &dWidth, &dHeight) != 2)
                        {
                            // Default to the size of the image source
                            dWidth = 0;
                            dHeight = 0;
                        }

                        // Call capture on the site
                        HX_RESULT rv = pExSiteSupplier->CaptureImage(pszFileName, dWidth, dHeight);
                        if (rv == HXR_RETRY)
                        {
                                STDOUT("Capture failed with HXR_RETRY.\n");
                        }
                        else if (rv != HXR_OK)
                        {
                                STDOUT("ERROR: Capture failed.\n");
                        }
                }
                HX_RELEASE(pExContext);
                HX_RELEASE(pExSiteSupplier);
            }
#endif /* #if defined(HELIX_FEATURE_PNG) */
#endif /* #if defined(HELIX_FEATURE_VIDEO) */
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
            else if (2 <= ulLen  &&  'p' == ch0  &&  'f' == ch1)
            {
                // Get the P.F.Manager from the FIRST player:
                IHXPresentationFeatureManager* pPFMgr = NULL;
                IHXPlayer* pPlayer0 = GetGlobal()->g_Players[0];
                if (pPlayer0)
                {
                    //  Go past whitespace after the "pf" and see if there's anything else:
                    const char* pszPFName = pszCmdBuffer+2;
                    while (*pszPFName  &&  isspace(*pszPFName))
                    {
                        pszPFName++;
                    }

                    HX_RESULT hxr = pPlayer0->QueryInterface(IID_IHXPresentationFeatureManager,
                                                (void**)&pPFMgr);

                    if ('\0' == *pszPFName) //  Then it's just "pf", so show all PFs' data:
                    {
                        STDOUT("Player[0]'s presentation features:\n");
                        HXBOOL bHasAtLeastOnePF = FALSE;
                        if (SUCCEEDED(hxr))
                        {
                            //  Get all the presentation features:
                            IHXValues* pFeatures = NULL;
                            hxr = pPFMgr->GetPresentationFeatures(pFeatures);
                            if (SUCCEEDED(hxr))
                            {
                                //  Look at all PF options from PFS.[PF] registry:
                                const char* pszPFName = NULL;
                                UINT32 ulRegID = 0;

                                HX_RESULT hxrGetProp = pFeatures->GetFirstPropertyULONG32(pszPFName,
                                        ulRegID);
                                while (SUCCEEDED(hxrGetProp))
                                {
                                    HX_ASSERT(*pszPFName  &&  strlen(pszPFName) >
                                            strlen(PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX)+1);
                                    if (!(*pszPFName)  ||  strlen(pszPFName) <=
                                            strlen(PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX)+1)
                                    {
                                        //  Shouldn't be empty string and should
                                        // have more bytes than just "PFS." :
                                        hxr = HXR_UNEXPECTED;
                                        STDOUT("\t(PF name \"%s\" as stored in the player registry is not valid)\n", pszPFName);
                                        break;
                                    }
                                    else
                                    {
                                        bHasAtLeastOnePF = TRUE;

                                        const char* pszPFNameNoPrefix =
                                                pszPFName + 1 + strlen(
                                                PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX);

                                        DumpPFdata(pPFMgr, pszPFNameNoPrefix);
                                    }

                                    hxrGetProp = pFeatures->GetNextPropertyULONG32(pszPFName,
                                        ulRegID);

                                } //  end "while (SUCCEEDED(hxrGetProp))".

                            }

                            if (!bHasAtLeastOnePF)
                            {
                                STDOUT("\t<none>\n");
                            }

                            STDOUT("\n");

                            HX_RELEASE(pFeatures);
                        }
                    } //  End ...it's just "pf"...
                    //  else it's either just "pf {PFname}" or it's
                    // "pf {PFname} {PFvalue}" where both name and value must
                    // each be an already-known and selectable quantity. 
                    // PFvalue can be anything if the PF's options are
                    // "freeform" (i.e., open-ended):
                    else
                    {
                        CHXString strPFName = pszPFName;
                        //  Now, find the {PFvalue}:
                        const char* pszPFValue = pszPFName+1;
                        while (*pszPFValue  &&  !isspace(*pszPFValue))
                        {
                            pszPFValue++;
                        }
                        //  we've found the end of the PFName:
                        strPFName.SetAt(pszPFValue - pszPFName,'\0');

                        //  Jump past the whitespace between:
                        while (*pszPFValue  &&  isspace(*pszPFValue))
                        {
                            pszPFValue++;
                        }

                        //  If there was no {PFValue} specified, then just
                        // show the PF's info:
                        if ('\0' == *pszPFValue)
                        {
                            hxr = DumpPFdata(pPFMgr, pszPFName);
                        }
                        //  Else set the PF's current value as specified:
                        else
                        {
                            IHXMediaPlatform* pMediaPlatform = GetGlobal()->pMediaPlatform;
                            IHXCommonClassFactory* pCCF = NULL;
                            hxr = pMediaPlatform? HXR_OK : HXR_UNEXPECTED;
                            if (pMediaPlatform)
                            {
                                hxr = pMediaPlatform->QueryInterface(IID_IHXCommonClassFactory,
                                                                     (void**)&pCCF);
                                if (SUCCEEDED(hxr))
                                {                                    
                                    IHXBuffer*  /*IN*/ pPFNewCurrentSetting = NULL;
                                    if (SUCCEEDED(CreateAndSetBufferCCF(
                                            pPFNewCurrentSetting,
                                            (BYTE*)pszPFValue,
                                            strlen(pszPFValue)+1, pCCF)) )
                                    {
                                        pszPFName = (const char*)strPFName;
                                        hxr = pPFMgr->SetPresentationFeatureValue(
                                                pszPFName, pPFNewCurrentSetting);
                                        STDOUT("\n");

                                        if (HXR_PROP_INACTIVE == hxr)
                                        {
                                            STDOUT("Can not set PF \"%s\" to \"%s\"; that option is currently disabled.\n",
                                                    pszPFName, pszPFValue);
                                            hxr = DumpPFdata(pPFMgr, (const char*)strPFName);
                                        }
                                        else if (HXR_PROP_NOT_FOUND == hxr)
                                        {
                                            STDOUT("\"%s\" PF not found.\n",
                                                    pszPFName, pszPFValue);
                                        }
                                        else if (HXR_INVALID_PARAMETER == hxr)
                                        {
                                            STDOUT("\"%s\" PF's option: \"%s\" not found.\n",
                                                    pszPFName, pszPFValue);
                                            hxr = DumpPFdata(pPFMgr, pszPFName);
                                        }
                                    }

                                    HX_RELEASE(pPFNewCurrentSetting);
                                }
                            }

                            HX_RELEASE(pCCF);
                        }
                    }
                }

                HX_RELEASE(pPFMgr);
            }
#endif //  HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.

            STDOUT("\n");

            //  Copy the command buffer into the prior-command buffer:
            char* pszPriorCmdBuffer = (char*) GetGlobal()->g_szPriorCommandBuffer;
            memcpy((void*) pszPriorCmdBuffer, (void*) pszCmdBuffer, HX_COMMAND_BUFFER_SIZE);

            // Clear the command buffer
            memset((void*) pszCmdBuffer, 0, HX_COMMAND_BUFFER_SIZE);
        }
        else
        {
            // Add this character to the command buffer (if there's room)
            char*  pszCmdBuffer = (char*) GetGlobal()->g_szCommandBuffer;
            UINT32 ulLen        = strlen(pszCmdBuffer);
            if (ulLen < HX_COMMAND_BUFFER_SIZE - 1)
            {
                //  If this is a 'tab' and nothing is in the buffer yet, then
                // display the prior command as already-written text:
                if ('\t' == ch  &&  0 == ulLen)
                {
                    const char* pszPriorCmdBuffer = (char*) GetGlobal()->g_szPriorCommandBuffer;
                    memcpy((void*) pszCmdBuffer, (void*) pszPriorCmdBuffer, HX_COMMAND_BUFFER_SIZE);
                    STDOUT("\n%s", pszCmdBuffer);
                }
                else if ('\b' == ch) //  Backspace, so back up buffer
                {
                    if (ulLen > 0) // (else ignore it)
                    {
                        pszCmdBuffer[ulLen-1] = '\0';
                        UINT32 ulNumBkspaces = ulLen;
                        while (--ulNumBkspaces)
                        {
                            STDOUT("\b");
                        }
                        // Space to clear the end char & then back up the cursor:
                        STDOUT("%s \b", pszCmdBuffer);
                    }
                }
                else
                {
                    // Add the character
                    pszCmdBuffer[ulLen] = ch;
                }
            }
        }
    }
}



/*
 *  handle events for at most nTimeDelta milliseconds
 */
void DoEvents(int nTimeDelta)
{
#if defined(_WINDOWS)
    MSG msg;
    DWORD starttime, endtime, i;
    HXBOOL sleep = TRUE;
    static const int checkinterval = 10;

    starttime = GetTickCount();
    endtime = starttime + (nTimeDelta);
    i = 0;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        DispatchMessage(&msg);
        if ((i % checkinterval) == 0)
        {
            if (GetTickCount() > endtime)
                break;
            ++i;
        }
        sleep = FALSE;
    }
    if (sleep)
        Sleep(SLEEP_TIME);

#else
    DoEvent();
#endif
}

/*
 *  return the number of milliseconds since the epoch
 */
UINT32 GetTime()
{
#if defined (_WINDOWS) || defined(__TCS__)
    return (UINT32)GetTickCount();

#elif defined (_VXWORKS)
    HXTime t;
    gettimeofday(&t, NULL);
    return (UINT32)((t.tv_sec * 1000) + (t.tv_usec / 1000));

#elif defined (_UNIX) || defined(_SYMBIAN) || defined(_MAC_UNIX)
    timeval t;
    gettimeofday(&t, NULL);
    return (UINT32)((t.tv_sec * 1000) + (t.tv_usec / 1000));

#elif defined (_MACINTOSH)

    unsigned long long     micro_now;

    Microseconds((UnsignedWide*)&micro_now);
    return micro_now / 1000L;
#endif
}

#ifdef _UNIX

struct termios g_orig_termios;
bool g_orig_termios_set = false;

void SetupTerminal(void)
{
    int result;
    
    result = tcgetattr(0, &g_orig_termios);
    if(result == 0)
    {
        // Sets terminal to cbreak mode
        struct termios cbreak_termios;

        g_orig_termios_set = true;
        cbreak_termios = g_orig_termios;

        cbreak_termios.c_lflag &= ~(ICANON);
        cbreak_termios.c_cc[VMIN] = 1;
        cbreak_termios.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &cbreak_termios);
    }
}

void RestoreTerminal(void)
{
    // Restores terminal to normal mode
    if(g_orig_termios_set)
    {
        tcsetattr(0, TCSANOW, &g_orig_termios);        
    }    
}

#endif



#if defined(_SYMBIAN) && defined(_ARM)
//XXXGfw get rid of this...
extern "C"
{
    void __gccmain()
    {
    }
};
#endif

char* RemoveWrappingQuotes(char* str)
{
    int len = strlen(str);
    if (len > 0)
    {
        if (str[len-1] == '"') str[--len] = 0;
        if (str[0] == '"') { int i = 0; do { str[i++] = str[i+1]; } while(--len); }
    }
    return str;
}

#ifdef __TCS__
extern  "C"     int helixmain( int argc, char *argv[] )
#else
    int main( int argc, char *argv[] )
#endif
{
#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC)
    setvbuf(stdout, NULL, _IONBF, 0);
#endif /* defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC) */

    FPHXMEDIAPLATFORMOPEN   fpHXMediaPlatformOpen   = NULL;
    FPHXCREATEMEDIAPLATFORM fpHXCreateMediaPlatform = NULL;
    FPHXMEDIAPLATFORMCLOSE  fpHXMediaPlatformClose  = NULL;

    FPRMCREATEENGINE        fpCreateEngine          = NULL;
    FPRMCLOSEENGINE         fpCloseEngine           = NULL;
    FPRMSETDLLACCESSPATH    fpSetDll                = NULL;

    HX_RESULT               theErr                  = HXR_OK;
    ExampleClientContext**  ppExContexts            = NULL;
#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) && defined(_WIN32)
    ExampleABD*             pABD                    = NULL;
#endif
    IHXPlayer**             ppPlayers               = NULL;
    IHXQuickSeek*           ppQuickSeek             = NULL;
    IHXErrorSink*           pErrorSink              = NULL;
    IHXErrorSinkControl*    pErrorSinkControl       = NULL;
    UINT32                  ulABDResult             = 0;
    UINT32                  ulProbPktSize           = 0;
    UINT32                  ulProbPktNum            = 0;
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    INT32                   lVelocity               = 100;
    INT32                   lKeyFrameMode           = 0;
    INT32                   lAutoSwitch             = 0;
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    CHXString*              pABDServer              = NULL;
    char*                   pszURL[MAX_NUM_URLS];
    char*                   pszURLOrig[MAX_NUM_URLS];
    UINT32                  ulNumURLsFound          = 0;
    int                     nNumPlayers             = 1;
    int                     nNumPlayRepeats         = 1;
    int                     nTimeDelta              = DEFAULT_TIME_DELTA;
    int                     nStopTime               = DEFAULT_STOP_TIME;
    bool                    bStopTime               = true;
    int                     i                       = 0;
    char*                   pszGUIDList             = NULL;
#ifdef _MACINTOSH
    char                    dllhome[MAX_PATH]       = {'\0'}; /* Flawfinder: ignore */
#elif defined(_SYMBIAN)
    char                    dllhome[MAX_PATH]       = "c:"; /* Flawfinder: ignore */
#else
    char                    dllhome[MAX_PATH]       = {'.','\0'}; /* Flawfinder: ignore */
#endif
    DLLAccess*              pDLLAccess              = NULL;
    char                    staticLibPath[MAX_PATH] = {0}; /* Flawfinder: ignore */
#ifdef _WINDOWS
    HINSTANCE               hDll                    = NULL;
#endif
    bool                    bStopping = false;
    int                     nPlay = 0;
#if defined(HELIX_FEATURE_PREFERENCES)
    CHXPrefList             prefList;
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */

    // NULL out the URL arrays
    memset(pszURL, 0, MAX_NUM_URLS * sizeof(char*));
    memset(pszURLOrig, 0, MAX_NUM_URLS * sizeof(char*));


    //See if the user has set their HELIX_LIBS env var. This is overridden by the
    //-l option.
    const char* pszHelixLibs = getenv("HELIX_LIBS");
    if( pszHelixLibs )
        SafeStrCpy( dllhome,  pszHelixLibs, MAX_PATH);


#ifdef _MACINTOSH
    // on the Mac, look beside the application for its DLLs
    // for now. If we wanted to look elsewhere we'd need to set dllhome
    // appropriately, with colon-delimited paths.

    // strcpy(dllhome, "Macintosh HD:helix:clientapps:simpleplayer:debug");
#endif

#if defined(_MAC_UNIX)
	if (!pszHelixLibs)
	{
		CFBundleRef mainBundle;
		CFURLRef mainBundleURL;
		CFURLRef updirURL;
		
		// get the main bundle for the app
		mainBundle = ::CFBundleGetMainBundle();
		
		// look for a resource in the main bundle by name
		mainBundleURL = ::CFBundleCopyBundleURL( mainBundle );
		updirURL = ::CFURLCreateCopyDeletingLastPathComponent(NULL, mainBundleURL);
		
		CFStringRef urlString = CFURLCopyPath(updirURL);
		CFStringGetCString(urlString, dllhome, _MAX_PATH, kCFStringEncodingMacRoman);
		
		::CFRelease(updirURL);
		::CFRelease(mainBundleURL);
		::CFRelease(urlString);
	}
#endif	// _MAC_UNIX

#if defined(_WINDOWS) && defined(_STATICALLY_LINKED)
    g_hInstance = GetModuleHandle(NULL);
#endif

#ifdef _MACINTOSH
#ifndef _MAC_MACHO
    argc = ccommand( &argv );
    SIOUXSettings.standalone     = FALSE;
    SIOUXSettings.setupmenus     = FALSE;
    SIOUXSettings.showstatusline = TRUE;
#endif
#endif

#ifdef __TCS__
    bEnableAdviceSink = TRUE;
#endif

#ifdef _UNIX
    // Set the terminal to cbreak mode so that splay terminal commands work
    SetupTerminal();
#endif
    
    for (i = 1; i < argc; i++)
    {
        if (0 == stricmp(argv[i], "-legacy"))
        {
            GetGlobal()->bUseLegacyAPIs = TRUE;
        }
        else if (0 == stricmp(argv[i], "-a"))
        {
            GetGlobal()->bEnableAdviceSink = TRUE;
        }
        else if (0 == stricmp(argv[i], "-s"))
        {
            GetGlobal()->bEnableVerboseMode = TRUE;
        }
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
        else if (0 == strncmp(argv[i], "-v", 2))
        {
            // This is a command to set the velocity
            if (sscanf(argv[i]+2, "%ld,%ld,%ld", &lVelocity, &lKeyFrameMode, &lAutoSwitch) != 3)
            {
                fprintf(stderr, "Set velocity command is \"vX,Y,Z\" where:\n"
                            "      X   is the velocity in integer percent of normal playback speed,\n"
                            "      Y   is 0 for keyframe mode = FALSE or 1 for keyframe mode = TRUE,\n"
                            "      Z   is 0 for autoswitch = FALSE or 1 for autoswitch = TRUE\n");
                return -1;
            }
        }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
        else if (0 == stricmp(argv[i], "-iss"))
        {
            GetGlobal()->g_bEnableSlowStart = FALSE;
        }
        else if (0 == stricmp(argv[i], "-0"))
        {
            GetGlobal()->g_bNullRender = TRUE;
        }
        else if (0 == stricmp(argv[i], "-abd"))
        {
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -abd option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }

            int n = 0;
            char* pszToken = strtok(argv[i], ",");
            while (pszToken)
            {
                switch (n)
                {
                   case 0:
                       pABDServer = new CHXString(pszToken);
                       pABDServer->TrimLeft();
                       pABDServer->TrimRight();
                       break;
                   case 1:
                       ulProbPktSize = atoi(pszToken);
                       break;
                   case 2:
                       ulProbPktNum = atoi(pszToken);
                       break;
                   default:
                       break;
                }

                n++;
                pszToken = strtok(NULL, ",");
            }
            GetGlobal()->g_bABD = TRUE;
        }
        else if (0 == stricmp(argv[i], "-n"))
        {
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -n option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nNumPlayers = atoi(argv[i]);
            if (nNumPlayers < 1)
            {
                STDOUT("\nError: Invalid value for -n option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == stricmp(argv[i], "-rn"))
        {
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -rn option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nNumPlayRepeats = atoi(argv[i]);
            if (nNumPlayRepeats < 1)
            {
                STDOUT("\nError: Invalid value for -rn option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == stricmp(argv[i], "-t"))
        {
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -t option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nTimeDelta = atoi(argv[i]);
            if (nTimeDelta < 0)
            {
                STDOUT("\nError: Invalid value for -t option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == stricmp(argv[i], "-st"))
        {
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -st option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nStopTime = atoi(argv[i]);
            if (nStopTime < 0)
            {
                STDOUT("\nError: Invalid value for -st option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
#if defined _DEBUG || defined DEBUG
        else if (0 == stricmp(argv[i], "-d"))
        {
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -d option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            debug_level() = (int)strtoul(argv[i], 0, 0);
        }
#endif
        else if (0 == stricmp(argv[i], "-u"))
        {
            GetGlobal()->g_pszUsername = new char[1024];
            strcpy(GetGlobal()->g_pszUsername, ""); /* Flawfinder: ignore */
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -u option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            SafeStrCpy(GetGlobal()->g_pszUsername,  argv[i], 1024);
        }
        else if (0 == stricmp(argv[i], "-p"))
        {
            GetGlobal()->g_pszPassword = new char[1024];
            strcpy(GetGlobal()->g_pszPassword, ""); /* Flawfinder: ignore */
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -p option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            SafeStrCpy(GetGlobal()->g_pszPassword,  argv[i], 1024);
        }
        else if (0 == stricmp(argv[i], "-g"))
        {
            GetGlobal()->g_pszGUIDFile = new char[1024];
            strcpy(GetGlobal()->g_pszGUIDFile, ""); /* Flawfinder: ignore */
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -g option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            SafeStrCpy(GetGlobal()->g_pszGUIDFile, RemoveWrappingQuotes(argv[i]), 1024);
            if (!ReadGUIDFile())
            {
                STDOUT("\nError: Unable to read file specified by -g option.\n\n");
                return -1;
            }
        }
        else if (0 == stricmp(argv[i], "-l"))
        {
            if (++i == argc)
            {
                STDOUT("\nError: Invalid value for -l option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            SafeStrCpy(dllhome, RemoveWrappingQuotes(argv[i]), MAX_PATH);
        }
#if defined(HELIX_FEATURE_PREFERENCES)
        else if (0 == stricmp(argv[i], "-pref"))
        {
            if (i + 2 >= argc)
            {
                STDOUT("\nError: Invalid value for -pref option.\n\n");
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }

            prefList.Add(argv[i + 1], argv[i + 2]);

            i += 2;
        }
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */
        else if (ulNumURLsFound < MAX_NUM_URLS)
        {
            pszURLOrig[ulNumURLsFound] = RemoveWrappingQuotes(argv[i]);

            //if no "://" was found lets add file:// by default so that you
            //can refer to local content as just ./splay ~/Content/startrek.rm,
            //for example, and not ./splay file:///home/gregory/Content/startrek.rm
            char* pszAddOn = NULL;
            if( strstr( pszURLOrig[ulNumURLsFound], "://") )
                pszAddOn = "";
            else
                pszAddOn = "file://";

            pszURL[ulNumURLsFound] = new char[strlen(pszURLOrig[ulNumURLsFound])+strlen(pszAddOn)+1];
            sprintf( pszURL[ulNumURLsFound], "%s%s", pszAddOn, pszURLOrig[ulNumURLsFound] ); /* Flawfinder: ignore */
            // Increment the number of URLs we have found
            ulNumURLsFound++;
        }
        else
        {
            PrintUsage(GetAppName(argv[0]));
            return -1;
        }
    }

    if (!ulNumURLsFound && !GetGlobal()->g_bABD)
    {
        if (argc > 1)
        {
            STDOUT("\nError: No media file or URL was specified.\n\n");
        }
        PrintUsage(GetAppName(argv[0]));
        return -1;
    }
    else if (ulNumURLsFound > 1 && ((int) ulNumURLsFound) != nNumPlayers)
    {
        STDOUT("\nError: Number of URLs found doesn't match number of players specified.\n\n");
        PrintUsage(GetAppName(argv[0]));
        return -1;
    }
    else if (ulNumURLsFound == 1 && nNumPlayers > 1)
    {
        // Just copy the URLS - play all the same
        // thing for each player
        UINT32 ulLen = strlen(pszURL[0]) + 1;
        for (i = 1; i < nNumPlayers; i++)
        {
            pszURL[i] = new char [ulLen];
            if (pszURL[i])
            {
                strcpy(pszURL[i], pszURL[0]);
            }
        }
    }


#if defined (_STATICALLY_LINKED)
    // Load "clntcore" by default
    SafeSprintf(staticLibPath, MAX_PATH, "%s", "clntcore");
#else
#  if defined(_MAC_UNIX)
    SafeSprintf(staticLibPath, MAX_PATH, "%s/%s", dllhome, "hxmedpltfm.bundle");
#  elif defined(_UNIX)
    SafeSprintf(staticLibPath, MAX_PATH, "%s/%s", dllhome, "hxmedpltfm.so");
#  elif defined(_WINDOWS) || defined(_SYMBIAN)
    SafeSprintf(staticLibPath, MAX_PATH, "%s\\%s", dllhome, "hxmedpltfm.dll");
#  elif defined(_MACINTOSH)
#ifdef _MAC_MACHO
    SafeSprintf(staticLibPath, MAX_PATH, "clntcore.bundle");
#else
    if (strlen(dllhome) > 0 )
    {
        SafeSprintf(staticLibPath, MAX_PATH, "%s:%s", dllhome, "clntcore.shlb");
    }
    else
    {
        SafeSprintf(staticLibPath, MAX_PATH, "%s", "clntcore.shlb");
    }
#endif
#  endif
#endif

	if (dllhome)
    {
        GetDLLAccessPath()->SetPath(DLLTYPE_COMMON, dllhome);
        GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN, dllhome);
        GetDLLAccessPath()->SetPath(DLLTYPE_CODEC, dllhome);
    }
	
    // Allocate arrays to keep track of players and client
    // context pointers
    ppExContexts = new ExampleClientContext*[nNumPlayers];
    ppPlayers    = new IHXPlayer*[nNumPlayers];

    if (!ppExContexts || !ppPlayers)
    {
        if (nNumPlayers > 10)
        {
            STDOUT("Error: Out of Memory. Perhaps you are trying to launch too many players at once.\n");
        }
        else
        {
            STDOUT("Error: Out of Memory.\n");
        }
        theErr = HXR_UNEXPECTED;
        goto cleanup;
    }

    for (i = 0; i < nNumPlayers; i++)
    {
        ppPlayers[i] = NULL;

        ppExContexts[i] = new ExampleClientContext(i);
        if (!ppExContexts[i])
        {
            if (nNumPlayers > 10)
            {
                STDOUT("Error: Out of Memory. Perhaps you are trying to launch too many players at once.\n");
            }
            else
            {
                STDOUT("Error: Out of Memory.\n");
            }
            theErr = HXR_UNEXPECTED;
            goto cleanup;
        }
        ppExContexts[i]->AddRef();
    }

    // initialize the globals
    GetGlobal()->m_fpCreateEngine    = NULL;
    GetGlobal()->m_fpCloseEngine = NULL;

    // prepare/load the HXCore module
    pDLLAccess = new DLLAccess();

    STDOUT("Simpleplayer is looking for the client core at %s\n", staticLibPath );

    if (DLLAccess::DLL_OK != pDLLAccess->open(staticLibPath))
    {
        const char* pErrorString = NULL;
        pErrorString = pDLLAccess->getErrorString();
        STDERR("splayer: %s\n\n", pErrorString);
#ifndef _STATICALLY_LINKED
#ifndef _MACINTOSH
        //Make sure the user has told us where to find the DLLs at. Either
        //with the -l option or with the HELIX_LIBS env var.
        STDERR("You must tell the player where to find the client core and\n");
        STDERR("all of its supporting DLLs and codecs. Please use the -l\n");
        STDERR("option or set your HELIX_LIBS env variable to point the player.\n");
        STDERR("to where you have all of the DLLs installed.\n\n" );
        PrintUsage(argv[0]);
#endif
#endif
        goto cleanup;
    }

    GetGlobal()->m_fpHXMediaPlatformOpen = (FPHXMEDIAPLATFORMOPEN) pDLLAccess->getSymbol("HXMediaPlatformOpen");
    GetGlobal()->m_fpHXCreateMediaPlatform = (FPHXCREATEMEDIAPLATFORM) pDLLAccess->getSymbol("HXCreateMediaPlatform");
    GetGlobal()->m_fpHXMediaPlatformClose = (FPHXMEDIAPLATFORMCLOSE) pDLLAccess->getSymbol("HXMediaPlatformClose");

    GetGlobal()->m_fpCreateEngine = (FPRMCREATEENGINE) pDLLAccess->getSymbol("CreateEngine");
    GetGlobal()->m_fpCloseEngine  = (FPRMCLOSEENGINE)  pDLLAccess->getSymbol("CloseEngine");
    GetGlobal()->m_fpSetDLLAccessPath = (FPRMSETDLLACCESSPATH) pDLLAccess->getSymbol("SetDLLAccessPath");

    if (GetGlobal()->m_fpHXMediaPlatformOpen == NULL    ||
        GetGlobal()->m_fpHXCreateMediaPlatform == NULL  ||
        GetGlobal()->m_fpHXMediaPlatformClose == NULL   ||
        GetGlobal()->m_fpCreateEngine == NULL           ||
        GetGlobal()->m_fpCloseEngine  == NULL           ||
        GetGlobal()->m_fpSetDLLAccessPath == NULL )
    {
        theErr = HXR_FAILED;
        goto cleanup;
    }

#if defined(USE_XWINDOWS)
    XInitThreads();
#endif
    
    if( GetGlobal()->bUseLegacyAPIs )
    {
        // create client engine
        fpCreateEngine = GetGlobal()->m_fpCreateEngine;
        if (HXR_OK != fpCreateEngine((IHXClientEngine**) &GetGlobal()->pEngine))
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }
    }
    else
    {
        fpHXMediaPlatformOpen = GetGlobal()->m_fpHXMediaPlatformOpen;
        fpHXCreateMediaPlatform = GetGlobal()->m_fpHXCreateMediaPlatform;
        fpHXMediaPlatformClose = GetGlobal()->m_fpHXMediaPlatformClose;

        if (HXR_OK != fpHXMediaPlatformOpen())
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }

        if (HXR_OK != fpHXCreateMediaPlatform((IHXMediaPlatform**)&GetGlobal()->pMediaPlatform))
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }

        IHXMediaPlatform* pMediaPlatform = GetGlobal()->pMediaPlatform;
        

        if (HXR_OK != pMediaPlatform->AddPluginPath("HelixSimplePlayer", dllhome))
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }

        if (HXR_OK != pMediaPlatform->Init(NULL))
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }
    
        IHXCommonClassFactory* pCCF = NULL;
        if (HXR_OK != pMediaPlatform->QueryInterface(IID_IHXCommonClassFactory,
                                                     (void**)&pCCF))
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }

        if (HXR_OK != pCCF->CreateInstance(CLSID_IHXClientEngine, (void**)&GetGlobal()->pEngine))
        {
            HX_RELEASE(pCCF);
            theErr = HXR_FAILED;
            goto cleanup;
        }

        HX_RELEASE(pCCF);

        IHXMediaPlatformKicker* pKicker = NULL;
        pMediaPlatform->QueryInterface(IID_IHXMediaPlatformKicker, (void**)&pKicker);
        HX_ASSERT(pKicker);
        GetGlobal()->g_pIHXKicker = pKicker;

    }

    // create players
    for (i = 0; i < nNumPlayers; i++)
    {
        IHXClientEngine* pEngine = GetGlobal()->pEngine;
        if (HXR_OK != pEngine->CreatePlayer(ppPlayers[i]))
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }
    }


#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) && defined(_WIN32)
    // perform ABD if requested
    if (GetGlobal()->g_bABD)
    {
        IHXClientEngine* pEngine = GetGlobal()->pEngine;

        pABD = new ExampleABD((IUnknown*)(IHXClientEngine*)GetGlobal()->pEngine);
        pABD->AddRef();

        if (HXR_OK == pABD->DoABD((const char*)*pABDServer, 1, ulProbPktSize, ulProbPktNum))
        {
            ulABDResult = pABD->GetABD();
        }
        
        HX_RELEASE(pABD);
    }

    if (!ulNumURLsFound)
    {
        goto cleanup;
    }
#endif

    //initialize the example context
    pszGUIDList = GetGlobal()->g_pszGUIDList;
    for (i = 0; i < nNumPlayers; i++)
    {
        char pszGUID[GUID_LEN + 1]; /* Flawfinder: ignore */ // add 1 for terminator
        char* token = NULL;
        IHXPreferences* pPreferences = NULL;

        pszGUID[0] = '\0';

        if (pszGUIDList)
        {
            // Get next GUID from the GUID list
            if (i == 0)
            {
                token = strtok(pszGUIDList, "\n\0");
            }
            else
            {
                token = strtok(NULL, "\n\0");
            }
            if (token)
            {
                strncpy(pszGUID, token, GUID_LEN); /* Flawfinder: ignore */
                pszGUID[GUID_LEN] = '\0';
            }
        }

        ppPlayers[i]->QueryInterface(IID_IHXPreferences,
                                     (void**) &pPreferences);

        if (pPreferences && ulABDResult)
        {
            IHXBuffer* pValue = new CHXBuffer();
            pValue->AddRef();
            
            pValue->SetSize(16);    
            sprintf((char*)pValue->GetBuffer(), "%lu", ulABDResult * 1024); /* Flawfinder: ignore */
            pPreferences->WritePref("Bandwidth", pValue);

            HX_RELEASE(pValue);
        }

        
        ppExContexts[i]->Init(ppPlayers[i], pPreferences, pszGUID);

        ppPlayers[i]->SetClientContext(ppExContexts[i]);

        HX_RELEASE(pPreferences);

        ppPlayers[i]->QueryInterface(IID_IHXErrorSinkControl,
                                     (void**) &pErrorSinkControl);
        if (pErrorSinkControl)
        {
            ppExContexts[i]->QueryInterface(IID_IHXErrorSink,
                                            (void**) &pErrorSink);

            if (pErrorSink)
            {
                pErrorSinkControl->AddErrorSink(pErrorSink, HXLOG_EMERG, HXLOG_INFO);
            }

            HX_RELEASE(pErrorSink);
        }

        HX_RELEASE(pErrorSinkControl);

#if defined(HELIX_FEATURE_PREFERENCES)
        prefList.SetPreferences(ppPlayers[i]);
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */
    }

#if defined(HELIX_FEATURE_PREFERENCES)
    // Clear the prefList since we don't need its values anymore
    prefList.Clear();
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */

    nPlay = 0;
    while(nPlay < nNumPlayRepeats) 
    {

        nPlay++;
        if (GetGlobal()->bEnableVerboseMode)
        {
            STDOUT("Starting play #%d...\n", nPlay);
        }

        GetGlobal()->g_Players  = ppPlayers;
        GetGlobal()->g_nPlayers = nNumPlayers;

#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC)
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine,
                              TRUE);
#endif

        IHXQuickSeek* pTmp = NULL;
        IHXPlayer* pPlayer = GetGlobal()->g_Players[0];
        pPlayer->QueryInterface( IID_IHXQuickSeek,
                                 (void**)&pTmp
                                 );
        GetGlobal()->g_pQuickSeek = pTmp;

        // There is only one URL for this presentation, so open the URL
        // for each player in the array
        UINT32 starttime, endtime, now;
        for (i = 0; i < nNumPlayers; i++)
        {
            STDERR("opening %s on player %d\n", pszURL[i], i );

            if (HXR_OK == ppPlayers[i]->OpenURL(pszURL[i]))
            {
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
                // Set velocity, if not 100%
                if (lVelocity != 100)
                {
                    STDOUT("Setting playback velocity to: %ld%%\n", lVelocity);
                    IHXPlaybackVelocity* pPlaybackVelocity = NULL;
                    ppPlayers[i]->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pPlaybackVelocity);
                    if (pPlaybackVelocity)
                    { 
                              STDOUT("Calling SetVelocity(%ld,%s,%s) in player %d\n",
                                           lVelocity,
                                           (lKeyFrameMode ? "TRUE" : "FALSE"),
                                           (lAutoSwitch ? "TRUE" : "FALSE"),
                                           i);
                              pPlaybackVelocity->SetVelocity(lVelocity, lKeyFrameMode, lAutoSwitch);
                    }
                    HX_RELEASE(pPlaybackVelocity);
                }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

                if (GetGlobal()->bEnableVerboseMode)
                {
                    STDOUT("Starting player %d...\n", i);
                }
                ppPlayers[i]->Begin();
            }
            starttime = GetTime();
            endtime = starttime + nTimeDelta;
            while (1)
            {
                DoEvents(nTimeDelta);
                now = GetTime();
                if (now >= endtime)
                    break;
#ifdef __TCS__
                if (gForcePlayerToStop != 0)
                    break;
#endif
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
                if (gMacPlayerIsDone)
                {
                    break;
                }
#endif
            }
        }

#ifdef __TCS__
        g_Players = ppPlayers;
        g_nPlayers = nNumPlayers;
#endif

        starttime = GetTime();
        if (nStopTime == -1)
        {
            bStopTime = false;
        }
        else
        {
            endtime = starttime + nStopTime;
        }
        bStopping = false;
        // Handle events coming from all of the players
        while (!AllPlayersDone(nNumPlayers, ppPlayers))
        {
            now = GetTime();
            if (!bStopping && bStopTime && now >= endtime)
            {
                // Stop all of the players, as they should all be done now
                if (GetGlobal()->bEnableVerboseMode)
                {
                    STDOUT("\nEnd (Stop) time reached. Stopping all players...\n");
                }
                StopAllPlayers(nNumPlayers, ppPlayers);
                bStopping = true;
            }
            DoEvent();
            if (GetGlobal()->g_bUserStop)
            {
                break;
            }
#ifdef __TCS__
            if (gForcePlayerToStop != 0)
                break;
#endif
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
            if (gMacPlayerIsDone)
            {
                break;
            }
#endif
        }

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
        // Close the velocity control if we have opened it
        // in the first player
        if (GetGlobal()->g_bInitVelocity &&
            GetGlobal()->g_nPlayers >= 1)
        {
            IHXPlaybackVelocity* pPlaybackVelocity = NULL;
            GetGlobal()->g_Players[0]->QueryInterface(IID_IHXPlaybackVelocity,
                                                      (void**) &pPlaybackVelocity);
            if (pPlaybackVelocity)
            {
                STDOUT("Closing playback velocity in first player\n");
                pPlaybackVelocity->CloseVelocityControl();
                GetGlobal()->g_bInitVelocity = FALSE;
            }
            HX_RELEASE(pPlaybackVelocity);
        }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

        // Stop all of the players, as they should all be done now
        if (GetGlobal()->bEnableVerboseMode)
        {
            if (GetGlobal()->g_bUserStop)
            {
                STDOUT("\nUser stopped playback. Stopping all players...\n");
            }
            else
            {
                STDOUT("\nPlayback complete. Stopping all players...\n");
            }
        }
        StopAllPlayers(nNumPlayers, ppPlayers);

        // repeat until nNumRepeats
    }

  cleanup:

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (GetGlobal()->g_pVelocityCaps)
    {
        IHXPlaybackVelocityCaps* pCaps = GetGlobal()->g_pVelocityCaps;
        pCaps->Release();
        GetGlobal()->g_pVelocityCaps = NULL;
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    // Clean up the URL array
    for (i = 0; i < ((int) ulNumURLsFound); i++)
    {
        HX_VECTOR_DELETE(pszURL[i]);
    }

    if (ppExContexts)
    {
        for (i = 0; i < nNumPlayers; i++)
        {
            if (ppExContexts[i])
            {
                ppExContexts[i]->Release();
                ppExContexts[i] = NULL;
            }
        }
        delete []ppExContexts;
        ppExContexts = NULL;
    }

    HX_RELEASE(GetGlobal()->g_pQuickSeek);
    
    if (ppPlayers)
    {
        for (i = 0; i < nNumPlayers; i++)
        {
            if (ppPlayers[i])
            {
                if (GetGlobal()->pEngine)
                {
                    GetGlobal()->pEngine->ClosePlayer(ppPlayers[i]);
                }
                ppPlayers[i]->Release();
                ppPlayers[i] = NULL;
            }
        }
        delete []ppPlayers;
        ppPlayers = NULL;
    }

#ifdef __TCS__
    g_nPlayers = 0;
#endif

#ifdef _UNIX
    RestoreTerminal();
#endif
    
    if( GetGlobal()->bUseLegacyAPIs )
    {
        if (GetGlobal()->pEngine)
        {
            fpCloseEngine = GetGlobal()->m_fpCloseEngine;
            fpCloseEngine(GetGlobal()->pEngine);
            GetGlobal()->pEngine = NULL;
        }
    }
    else
    {
        IHXClientEngine* pEngine = GetGlobal()->pEngine;

        if (pEngine)
        {
            IHXClientEngine2* pEngine2 = NULL;
            if (HXR_OK == pEngine->QueryInterface(IID_IHXClientEngine2, 
                                                  (void**)&pEngine2))
            {
                pEngine2->Close();
            }
            HX_RELEASE(pEngine2);
        }
        HX_RELEASE(GetGlobal()->pEngine);
        HX_RELEASE(GetGlobal()->g_pIHXKicker);
        
        if (GetGlobal()->pMediaPlatform)
        {
            // Reset() not only close the platform but also remove all
            // persistent information(i.e. preferences) maintained by the
            // platform
            // GetGlobal()->pMediaPlatform->Reset(NULL);
            GetGlobal()->pMediaPlatform->Close();
            HX_RELEASE(GetGlobal()->pMediaPlatform);
        }

        if (fpHXMediaPlatformClose)
        {
            fpHXMediaPlatformClose();
        }
    }

    pDLLAccess->close();
    delete pDLLAccess;
    pDLLAccess = NULL;

    HX_DELETE(pABDServer);

    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("\nDone.\n");
    }

    if (GetGlobal()->g_pszUsername)
    {
        delete [] GetGlobal()->g_pszUsername;
        GetGlobal()->g_pszUsername = NULL;
    }
    if (GetGlobal()->g_pszPassword)
    {
        delete [] GetGlobal()->g_pszPassword;
        GetGlobal()->g_pszPassword = NULL;
    }
    if (GetGlobal()->g_pszGUIDFile)
    {
        delete [] GetGlobal()->g_pszGUIDFile;
        GetGlobal()->g_pszGUIDFile = NULL;
    }
    if (GetGlobal()->g_pszGUIDList)
    {
        delete [] GetGlobal()->g_pszGUIDList;
        GetGlobal()->g_pszGUIDList = NULL;
    }

    // If an an error occurred in this function return it
    if (theErr != HXR_OK)
    {
        return theErr;
    }
    // If an error occurred during playback, return that
    else if (GetGlobal()->g_Error != HXR_OK)
    {
        return GetGlobal()->g_Error;
    }
    // If all went well, return the number of seconds played (if there
    // was only one player)...
    else if (nNumPlayers == 1)
    {
        return GetGlobal()->g_ulNumSecondsPlayed;
    }
    // or HXR_OK (if there was more than one player)
    else
    {
        return HXR_OK;
    }
}
char* GetAppName(char* pszArgv0)
{
    char* pszAppName;

    pszAppName = strrchr(pszArgv0, '\\');

    if (NULL == pszAppName)
    {
        return pszArgv0;
    }
    else
    {
        return pszAppName + 1;
    }
}

void PrintUsage(const char* pszAppName)
{
    STDOUT("\n");

#if defined _DEBUG || defined DEBUG
    STDOUT("USAGE:\n%s [-legacy] [-as0] [-abd server_ip:port,probpkt_size,probpkt_num] [-d D] [-n N] [-t T] [-st ST] [-g file] [-u username] [-p password] [-pref prefname prefvalue] [-vX,Y,Z] <URL0> [<URL1> [<URL2>]]\n", pszAppName);
#else
    STDOUT("USAGE:\n%s [-legacy] [-as0] [-abd server_ip:port] [-n N] [-t T] [-g file] [-u username] [-p password] <URL>\n", pszAppName);
#endif
    STDOUT("       -legacy : optional flag to load media engine via legacy APIs(i.e. CreateEngine/CloseEngine)\n");
    STDOUT("       -a : optional flag to show advise sink output\n");
    STDOUT("       -s : optional flag to output useful status messages\n");
    STDOUT("       -iss: optional flag to ignore slow-start and begin\n"
           "            playback immediately\n");
    STDOUT("       -0 : optional flag to disable all output windows\n");
#ifndef _STATICALLY_LINKED
    STDOUT("       -l : optional flag to tell the player where to find its DLLs\n");
#endif

    STDOUT("      -abd: Auto Bandwidth Detection\n");
    STDOUT("            server_ip:port\n");
#if defined _DEBUG || defined DEBUG
    STDOUT("       -d : HEX flag to print out DEBUG info\n");
    STDOUT("            0x8000 -- for audio methods calling sequence\n"
           "            0x0002 -- for variable values\n");
#endif
    STDOUT("       -n : optional flag to spawn N players\n");
    STDOUT("       -rn: optional flag to repeat playback N times\n");
    STDOUT("       -t : wait T milliseconds between starting players (default: %d)\n", DEFAULT_TIME_DELTA);
    STDOUT("       -st: wait ST milliseconds until stopping players (default: %d)\n", DEFAULT_STOP_TIME);
    STDOUT("       -g : use the list of GUIDS in the specified newline-delimited file\n");
    STDOUT("            to give each of the N players a different GUID\n");
    STDOUT("       -u : username to use in authentication response\n");
    STDOUT("       -p : password to use in authentication response\n");
    STDOUT("       -pref : preference name and value to set. You can specify more than 1 -pref option on a command-line\n");
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    STDOUT("       -vX,Y,Z : Set playback velocity to X percent (normal speed = 100), Y=keyframe, Z=autoswitch\n");
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    STDOUT("   <URL0> : URLs for multiple players\n");
    STDOUT("            (If multiple URLs are specified, then the number of players (-n)\n"
           "             must match the number of URLs. If multiple players are specified\n"
           "             but only 1 URL, then the same URL will be played on all players.)\n");
    STDOUT("\n\n");
    STDOUT("Run-time commands (enter the command below followed by <Enter>:\n\n");
    STDOUT("  p or P  : Pause player 0\n");
    STDOUT("  r or R  : Resume player 0\n");
    STDOUT("  t or T  : Show current playback time for player 0\n");
    STDOUT("  f or F  : Set player 0 to next group\n");
    STDOUT("  b or B  : Set player 0 to previous group\n");
    STDOUT("  m       : Toggle mute/un-mute for player 0\n");
    STDOUT("  q or Q  : Stop all players and end splay\n");
    STDOUT("  sXXXXX  : Seek player 0 to XXXXX ms\n");
    STDOUT("  lV      : Set volume of player 0 to V (where V is >= 0 and <= 100)\n");
    STDOUT("  vX,Y,Z  : Set playback velocity of player 0 to X percent (normal speed = 100),");
    STDOUT("            Y=keyframe, Z=autoswitch\n");
#if defined(HELIX_FEATURE_VIDEO) && defined(HELIX_FEATURE_PNG)
    STDOUT("  c       : Capture a video frame and save it to './image.png'.\n");
    STDOUT("  c F     : Capture a video frame and save it to file F.\n");
    STDOUT("  c F,X,Y : Capture a video frame and save it to file F, with the X,Y dimensions.\n");
#endif //  HELIX_FEATURE_VIDEO && HELIX_FEATURE_PNG
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
    STDOUT("  pf or PF: Show all presentation features, their current settings,\n");
    STDOUT("            and each PF's list of options\n");
    STDOUT("  pf X    : Show presentation feature X's current & optional settings\n");
    STDOUT("  pf X Y  : Set presentation feature X's current setting to Y\n");
#endif //  HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.
    STDOUT("\n Editing of commands:\n");
    STDOUT("  tab key : load the prior command in the command buffer\n");
    STDOUT("  bksp key: undo the last-typed character\n");
    STDOUT("\n\n");
}

HXBOOL AllPlayersDone(int nNumPlayers, IHXPlayer** ppPlayers)
{
    HXBOOL bAllDone = TRUE;

    // Start checking at the end of the array since those players
    // were started last and are therefore more likely to not be
    // finished yet.
    for (int i = nNumPlayers - 1; i >= 0 && bAllDone; i--)
    {
        if (!ppPlayers[i]->IsDone())
        {
            bAllDone = FALSE;
        }
    }

    return bAllDone;
}

void StopAllPlayers(int nNumPlayers, IHXPlayer** ppPlayers)
{
    for (int i = 0; i < nNumPlayers; i++)
    {
        ppPlayers[i]->Stop();
    }
}

HXBOOL ReadGUIDFile()
{
    HXBOOL  bSuccess = FALSE;
    FILE* pFile    = NULL;
    int   nNumRead = 0;
    int   readSize = 10000;
    char*  pszBuffer = new char[readSize];

    if (GetGlobal()->g_pszGUIDFile)
    {
        if((pFile = fopen(GetGlobal()->g_pszGUIDFile, "r")) != NULL)
        {
            // Read in the entire file
            nNumRead = fread(pszBuffer, sizeof(char), readSize, pFile);
            pszBuffer[nNumRead] = '\0';

            // Store it for later parsing
            GetGlobal()->g_pszGUIDList = new char[nNumRead + 1];
            strcpy(GetGlobal()->g_pszGUIDList, pszBuffer); /* Flawfinder: ignore */

            fclose(pFile);
            pFile = NULL;

            if (nNumRead > 0)
            {
                bSuccess = TRUE;
            }
        }
    }

    delete [] pszBuffer;

    return bSuccess;
}

#ifdef __TCS__

extern  "C"     void Helix_SeekAudio(unsigned long uTimeMS)
{
    int i;
    for (i = 0; i < g_nPlayers; i++)
    {
        g_Players[i]->Seek(uTimeMS);
    }
}


extern  "C"     void Helix_StopAudio(void)
{
    int i;
    for (i = 0; i < g_nPlayers; i++)
    {
        g_Players[i]->Stop();
    }
}


extern  "C"     void Helix_PauseAudio(void)
{
    int i;
    for (i = 0; i < g_nPlayers; i++)
    {
        g_Players[i]->Pause();
    }
}


extern  "C"     void Helix_ResumeAudio(void)
{
    int i;
    for (i = 0; i < g_nPlayers; i++)
    {
        g_Players[i]->Begin();
    }
}

#endif
