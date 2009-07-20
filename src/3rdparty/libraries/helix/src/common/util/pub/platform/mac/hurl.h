/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hurl.h,v 1.5 2005/03/14 19:36:41 bobclark Exp $
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

#pragma once

/* Functions to interact with Web Browsers */

#ifdef __cplusplus
extern "C" {
#endif

struct IHXPreferences;

HXBOOL LaunchMacBrowserWithURL(const char* pURL, IHXPreferences* pPrefs);
HXBOOL LaunchMacBrowserWithURLOrdered(const char* pURL, IHXPreferences* pPrefs, HXBOOL bBringToFront);
HXBOOL LaunchMacBrowserWithURLWindowParam(const char* pURL, IHXPreferences* pPrefs, 
	HXBOOL bBringToFront, HXBOOL bNewWindow);
HXBOOL LaunchMacBrowserTargetingWindow(const char* pURL, IHXPreferences* pPrefs, 
	HXBOOL bBringToFront, const char* pTargetWindowName, HXBOOL bUseNewJavascriptWindow);

HXBOOL	SendOpenURL(OSType	appSignature, const char* pURL, HXBOOL async);
HXBOOL	SendOpenURLOrdered(OSType	appSignature, const char* pURL, HXBOOL async, 
		HXBOOL bBringToFront, HXBOOL bNewWindow = FALSE);
HXBOOL	SendOpenURLAsync(OSType	appSignature, const char* pURL);
HXBOOL	SendOpenURLSync(OSType	appSignature, const char* pURL);

HXBOOL	MakeOpenURLEvent(OSType	appSignature, const char* pURL, AEDesc *theAppleEventPtr, Boolean bNewWindow = false);

#ifdef _CARBON
HXBOOL GetPreferredOrSystemBrowser(IHXPreferences* pPrefs, FSRef *outBrowserFSRef, OSType *outBrowserSignature);
#endif

#ifdef __cplusplus
}
#endif

