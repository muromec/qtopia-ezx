/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: plprefk.h,v 1.4 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _PLPREFK
#define  _PLPREFK

typedef enum player_preference_key
{  
	Pref_CapiBrowserName = 0, 
	Pref_LastUsedURL, 
	Pref_Volume, 
	Pref_DefPort, 
	Pref_AlwaysOnTop, 
	Pref_WindowPositionX, 
	Pref_WindowPositionY, 
	Pref_ShowStatusBar, 
	Pref_ShowInfoVolume,
	Pref_SamplingRate, 
	Pref_BitsPerSample, 
	Pref_LossCorrection, 
	Pref_SendStatistics,
	Pref_ServerTimeOut, 	
	Pref_AcceptableLoss, 
	Pref_RampBlocks, 
	Pref_SeekPage, 
	Pref_SeekLine,  
	Pref_NetPeriod,
	Pref_FilePeriod,
	Pref_CyclesPerPeriod,
	Pref_UseProxy,
	Pref_ProxyHost, 
	Pref_ProxyPort, 	
	Pref_NotProxyHost,
	Pref_AudioQuality,
	Pref_ReceiveTCP,
	Pref_UseUDPPort, 		
	Pref_UDPPort,
	Pref_UseSplitter,
	Pref_SplitterHost, 
	Pref_SplitterPort,
	Pref_SyncMultimedia,		
	Pref_License,
	Pref_MultiplePlayer,
	Pref_Bandwidth,
	Pref_Sites,
	Pref_Clips,
	Pref_MaxClipCount,
	Pref_ShowPresets,
	Pref_Presets,
	Pref_Scan,
	Pref_ScanTime,
	Pref_HTTPProxyHost,
	Pref_HTTPProxyPort,
	Pref_ClientLicense,
	Pref_URLtoHurl,
	Pref_HurledURL,
	Pref_PerfectPlayTime,
	Pref_PerfectPlayMode,
	Pref_NetworkLatency,
	Pref_AutoTransport,
	Pref_AttemptMulticast,
	Pref_AttemptUDP,
	Pref_AttemptTCP,
	Pref_MulticastTimeout,
	Pref_UDPTimeout,
	Pref_ClipID,
	Pref_MusicChoice,
	Pref_HomeURL,
	Pref_CustomCaption,
	Pref_UserName,
	Pref_UserCompany,
	Pref_UserEmail,
	Pref_PerfPlayEntireClip,
	Pref_NumberOfPreferences 
} player_preference_key;     

#endif // _PLPREFK
