/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: playpref.h,v 1.5 2007/07/06 21:57:57 jfinnecy Exp $
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

#ifndef _PLAYPREF
#define  _PLAYPREF

#include "pref.h"
#include "plprefk.h"

#if 0
// list of player's preferences
static CPrefTableEntry PlayPrefTable[] =
{
#ifdef _MACINTOSH
	{ Pref_CapiBrowserName, PrefType_Text, 0, "MOSS",          ""     },			// topic names for DDE to Web browsers
#else
	{ Pref_CapiBrowserName, PrefType_Text, 0, "Browser",          ""     },			// topic names for DDE to Web browsers
#endif
   	{ Pref_LastUsedURL,     PrefType_Text, 0, "LastURL",          ""     },			// Last URL played :  str
   	{ Pref_Volume,          PrefType_Text, 0, "Volume",           "49"   },			// Volume setting  :  float 0 - 100
	{ Pref_DefPort,         PrefType_Text, 0, "DefPort",          "7070" },			// Default port	 :  int   7070
   	{ Pref_AlwaysOnTop,     PrefType_Text, 0, "OnTop",            "0"    },	        // Make Player a top most window :  bool true or false 
   	{ Pref_WindowPositionX, PrefType_Text, 0, "x",                "0"    },			// x pos on root window: platform dep
   	{ Pref_WindowPositionY, PrefType_Text, 0, "y",                "0"    },			// y pos on root window: platform dep
   	{ Pref_ShowStatusBar,   PrefType_Text, 0, "StatusBar",        "1"    },			// show status bar :  bool true or false
   	{ Pref_ShowInfoVolume,  PrefType_Text, 0, "InfoandVolume",	  "1"    },			// show info and vol: bool true or false
   	{ Pref_SamplingRate,    PrefType_Text, 0, "SamplingRate",     "8000" },			// sampling rate : ??
   	{ Pref_BitsPerSample,   PrefType_Text, 0, "BitsPerSample",    "16"   },			// bits per sample: int ( 8 or 16 )
   	{ Pref_LossCorrection,  PrefType_Text, 0, "LossCorrection",   "1"    },			// do deinterleaving : bool ( T or F )
   	{ Pref_SendStatistics,  PrefType_Text, 0, "SendStatistics",   "1"    },			// send stats to server: bool ( T or F )
   	{ Pref_ServerTimeOut,   PrefType_Text, 0, "ServerTimeOut",    "90"   },			// ??? in secs : int ( 0 to ? )
	{ Pref_AcceptableLoss,  PrefType_Text, 0, "AcceptableLoss",	  ""     },			// ???
	{ Pref_RampBlocks,      PrefType_Text, 0, "RampBlocks", 	  ""     },			// ???
	{ Pref_SeekPage,        PrefType_Text, 0, "SeekPage",    	  ""     },			// ???
	{ Pref_SeekLine,        PrefType_Text, 0, "SeekLine",    	  "20"   },			// ???
	{ Pref_NetPeriod,       PrefType_Text, 0, "NetPeriod", 		  ""     },			// ???
	{ Pref_FilePeriod,      PrefType_Text, 0, "FilePeriod", 	  ""     },			// ???
	{ Pref_CyclesPerPeriod, PrefType_Text, 0, "CyclesPerPeriod",  ""     },			// ???
   	{ Pref_UseProxy,        PrefType_Text, 0, "ProxySupport",     "0"    },			// ??? : bool ( T or F )
   	{ Pref_ProxyHost,       PrefType_Text, 0, "HttpHost",         ""     },			// proxy hostname: char *
   	{ Pref_ProxyPort,       PrefType_Text, 0, "HttpPort",         "1090" },		    // proxy port: int
   	{ Pref_NotProxyHost,    PrefType_Text, 0, "NotProxy",         ""     },			// ???
   	{ Pref_AudioQuality,    PrefType_Text, 0, "AudioQuality",     "0"    },			// decoder quality : int ( 0 to 100 )
   	{ Pref_ReceiveTCP,      PrefType_Text, 0, "ReceiveTCP",       "0"    },			// get audio via TCP : bool ( T or F )
   	{ Pref_UseUDPPort,      PrefType_Text, 0, "UseUDPPort",       "0"    },			// ???
   	{ Pref_UDPPort,         PrefType_Text, 0, "UDPPort",          "7070" },		    // UDP port: int ( ?? to ?? )
   	{ Pref_UseSplitter,     PrefType_Text, 0, "UseSplitter",      "0"    },			// use splitter: bool ( T or F )
	{ Pref_SplitterHost,    PrefType_Text, 0, "SplitterHost",	  " "    },			// splitter hostname: char *
	{ Pref_SplitterPort,    PrefType_Text, 0, "SplitterPort",	  " "    },			// splitter port: int ( ?? to ?? )
 	{ Pref_SyncMultimedia,  PrefType_Text, 0, "SyncMultimedia",	  "1"    },			// do URL events: bool ( T or F )
   	{ Pref_License,         PrefType_Text, 0, "License",          "0"    },			// show PN license: bool ( T or F )
   	{ Pref_MultiplePlayer,  PrefType_Text, 0, "MultiplePlayer",   "0"    },			// allow multiple players: bool ( T or F )
   	{ Pref_Bandwidth,       PrefType_Text, 0, "Bandwidth",        "28800"},		    // default bandwidth for BW
   	{ Pref_Sites,           PrefType_Text, 0, "Sites",			  ""     },			// defaults for sites menu
   	{ Pref_Clips,           PrefType_Text, 0, "Clips",        	  ""     },			// iterator for clips menu
   	{ Pref_MaxClipCount,    PrefType_Text, 0, "MaxClipCount",     "4"    },			// number of items to remember for clips menu
   	{ Pref_ShowPresets,     PrefType_Text, 0, "ShowPresets",	  "1"    },			// show presets: bool true or false
	{ Pref_Presets,         PrefType_Text, 0, "Presets", 		  ""     },			// iterator for Presets
	{ Pref_Scan,            PrefType_Text, 0, "Scan", 		  	  ""     },			// scan iterator
	{ Pref_ScanTime,        PrefType_Text, 0, "EndScan",		  "10000"},  		// endtime for scan ram file items
	{ Pref_HTTPProxyHost,   PrefType_Text, 0, "HTTPProxyHost", 	  ""     },			// http proxy for http get
	{ Pref_HTTPProxyPort,   PrefType_Text, 0, "HTTPProxyPort", 	  "80"   },			// http proxy port
	{ Pref_ClientLicense,   PrefType_Text, 0, "ClientLicenseKey", "0"    },			// encoded license key
	{ Pref_URLtoHurl,       PrefType_Text, 0, "URLtoHurl",		  ""     },			// The user's timecast URL: http://www.timecast.com/config.cgi?uid=fff3456...
	{ Pref_HurledURL,       PrefType_Text, 0, "HurledURL",		  "0"    },			// 0 or 1 indicates that we Hurled it...
	{ Pref_PerfectPlayTime, PrefType_Text, 0, "PerfectPlayTime",  "60"   },			// PerfectPlay default time - 30 secs.
	{ Pref_PerfectPlayMode, PrefType_Text, 0, "PerfectPlayMode",  "0"    },			// 0 or 1 indicates that we are in perfect play mode in player
    { Pref_NetworkLatency,  PrefType_Text, 0, "NetworkLatency",	  "3000" },			// default network latency is 3000 ms. used for resend buffer depth
    { Pref_AutoTransport,   PrefType_Text, 0, "AutoTransport",	  "1"    },			// 0 or 1 indicates that we will auto find the best transport
    { Pref_AttemptMulticast,PrefType_Text, 0, "AttemptMulticast", "1"    },			// 0 or 1 indicates that in user transport mode we will attempt Multicast
    { Pref_AttemptUDP,      PrefType_Text, 0, "AttemptUDP",		  "1"    },			// 0 or 1 indicates that in user transport mode we will attempt UDP
    { Pref_AttemptTCP,      PrefType_Text, 0, "AttemptTCP",		  "1"    },			// 0 or 1 indicates that in user transport mode we will attempt TCP
    { Pref_MulticastTimeout,PrefType_Text, 0, "MulticastTimeout", "2000" },			// time in ms to give up on Multicast.
    { Pref_UDPTimeout,      PrefType_Text, 0, "UDPTimeout",		  "2000" },			// time in ms to give up on UDP.
    { Pref_ClipID,          PrefType_Text, 0, "ClipID",			  "0"    },			// Clip number played - acts as a unique ID to write log files...
	{ Pref_MusicChoice,     PrefType_Text, 0, "MusicChoice",      "0"    },			// ???
    { Pref_HomeURL,         PrefType_Text, 0, "HomeURL",		  "http://www.realaudio.com/"},// URL to hurl when user clicks on Home button
    { Pref_CustomCaption,   PrefType_Text, 0, "CustomCaption",	  NULL   },	// This caption overwrites the caption of the main Player's window
    { Pref_UserName,        PrefType_Text, 0, "UserName",		  "0"    },			// ???
    { Pref_UserCompany,     PrefType_Text, 0, "UserCompany",	  "0"    },			// ???
    { Pref_UserEmail,       PrefType_Text, 0, "UserEmail",		  "0"    },			// ???
    { Pref_PerfPlayEntireClip, PrefType_Text, 0, "PerfPlayEntireClip",  "0"  }		// 1 indicates that in perfect play mode Player will buffer entire clip
};
#endif // 0

class CPlayerPref : public CPref
{

public:
/*  call open_pref() to automatically create the correct platform specific Player's 
    preference object. If open_pref() returns NULL, an error occurred and the 
    CPlayerPref object was not created. Call last_error to get the error */

#ifndef _CARBON

static CPref * open_pref(const char* pCompanyName, const char* pProductName, UINT32 nProdMajorVer,  UINT32 nProdMinorVer, HXBOOL bCommon, IUnknown* pContext);

#else

// for the Mac, we default to bCommon=FALSE since we want non-admin users to be able to run a new install, so by default
// prefs get written per-user
static CPref * open_pref(const char* pCompanyName, const char* pProductName, UINT32 nProdMajorVer,  UINT32 nProdMinorVer, HXBOOL bCommon, IUnknown* pContext);

#endif // _CARBON

/*    class destructor */  
   virtual              ~CPlayerPref         (void);

};              

#endif // _PLAYPREF      
