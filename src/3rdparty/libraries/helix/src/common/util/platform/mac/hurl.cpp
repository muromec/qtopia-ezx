/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hurl.cpp,v 1.8 2005/03/14 19:36:40 bobclark Exp $
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

#include <string.h>
#include <stdio.h>
#ifndef _MAC_MACHO
#include <InternetConfig.h>
#endif

#include "hxtypes.h"
#include "hxcom.h"     /* IUnknown */
#include "ihxpckts.h"  /* IHXBuffer */
#include "platform/mac/hurl.h"
#include "hxwintyp.h"

#if defined(_CARBON) || defined(_MAC_UNIX)
	#include "platform/mac/mac_pref_cf.h"
#ifndef _MAC_UNIX
	#include "filespecutils.h"
	#include "MoreFilesX.h"
#endif

	static HXBOOL LaunchMacBrowserCarbon(const char* pURL, IHXPreferences* pPrefs, HXBOOL bBringToFront, HXBOOL bNewWindow);
	static HXBOOL GetPreferredBrowser(IHXPreferences* pPrefs, FSRef *outBrowserFSRef, OSType *outBrowserSignature);
	static void MakeHurlJavascriptURL(const char * pszURL, const char * pszWindowName, 
		HXBOOL bSetPosition, const HXxPoint& ptWindowPos, const HXxSize& sizeWindow, CHXString& outStrURL, HXBOOL bUseNewJavascriptWindow);
#else
	#include "mac_pref.h"
#endif // _CARBON

// Stuff for using ODOCs instead of OpenURL for Unicode UTF-8 URLs because IE 5.2 and Netscape can't
// deal with UTF8 in URLs; we'll send ODOCs instead of OpenURL events in those cases
Boolean IsLocalFileURLAPotentialProblemBecauseOfPercentEncodedUTF8Chars(const char *pURL);
HXBOOL	MakeOpenDocumentsEventFromURL(OSType appSignature, const char* pURL, AEDesc *theAppleEventPtr, Boolean bNewWindow);


#include "platform/mac/hx_moreprocesses.h"
#ifndef _MAC_UNIX
#include "hx_morefiles.h"
#include "fullpathname.h"
#include "hxinternetconfigutils.h"
#endif

#include "hxresult.h"
#include "hxprefs.h"

const	DescType		kWebBrowserSuite			=	'WWW!';

const	DescType		kOpenUrlEventID				=	'OURL';
const	DescType		kOpenUrlWindowID			=	'WIND';
const	DescType		kCloseAllWindowsEventID			=	'CLSA';

const 	OSType			MSIE_CREATOR				=  'MSIE';
const 	OSType			NETSCAPE_CREATOR			=  'MOSS';
const 	OSType			MOZILLA_CREATOR				=  'MOZZ';
const 	OSType			AOL_CREATOR				=  'AOp3';
const 	OSType			OMNIWEB_CREATOR				=  'OWEB';
const	OSType			ICAB_CREATOR				=  'iCAB';
const	OSType			CHIMERA_CREATOR				=  'CHIM';
const	OSType			OPERA_CREATOR				=  'OPRA';
const	OSType			SAFARI_CREATOR				=  'sfri';
 	
const	OSType			MOSAIC_CREATOR				=  'MOS!';
const   OSType			MOSAIC_BrowserSuite			=  'mos!';
const	OSType			MOSAIC_OpenUrlEventID			=  'ourl';

// if there's no default browser signature that we can determine from HX prefs or from InternetConfig,
// we'll look for one in this list, in this order

static OSType BrowserCreators[] 
	= { SAFARI_CREATOR, MSIE_CREATOR, NETSCAPE_CREATOR, MOZILLA_CREATOR, AOL_CREATOR,
		OMNIWEB_CREATOR, ICAB_CREATOR, CHIMERA_CREATOR, OPERA_CREATOR, 0 }; // what's Opera?  Any others?

/********************************************************************

	LaunchMacBrowserWithURL
	
********************************************************************/


HXBOOL LaunchMacBrowserWithURL(const char* pURL, IHXPreferences* pPrefs)
{	
	const HXBOOL bBringToFront = TRUE;
	
	return LaunchMacBrowserWithURLOrdered(pURL, pPrefs, bBringToFront);
}

HXBOOL LaunchMacBrowserWithURLOrdered(const char* pURL, IHXPreferences* pPrefs, HXBOOL bBringToFront)
{	
	const HXBOOL bExistingWindow = FALSE;
	
	return LaunchMacBrowserWithURLWindowParam(pURL, pPrefs, bBringToFront, bExistingWindow);
}

HXBOOL LaunchMacBrowserTargetingWindow(const char* pURL, IHXPreferences* pPrefs, HXBOOL bBringToFront, 
	const char* pTargetWindowName, HXBOOL bUseNewJavascriptWindow)
{
	// if bUseNewJavascriptWindow is false, then we won't make a new window for the javascript to execute in;
	// on some browsers, this will make the javascript fail to execute if no windows are open.
	
	const HXBOOL kDontSetPosition = FALSE;
	HXxPoint ptUnset = { 0, 0 };
	HXxSize szUnset = { 0, 0 };
	CHXString strNewURL;
	
	MakeHurlJavascriptURL(pURL, pTargetWindowName, kDontSetPosition, ptUnset, szUnset, strNewURL, bUseNewJavascriptWindow);
	if (strNewURL.IsEmpty())
	{
		check(!"LaunchMacBrowserTargetingWindow could not make Javascript URL");
		strNewURL = pURL;
	}
	
	// we make a new window since the Javascript will blow away the existing window	
	return LaunchMacBrowserWithURLWindowParam(strNewURL, pPrefs, bBringToFront, bUseNewJavascriptWindow);
}

HXBOOL LaunchMacBrowserWithURLWindowParam(const char* pURL, IHXPreferences* pPrefs, HXBOOL bBringToFront, HXBOOL bNewWindow)
{	
#if defined(_CARBON) || defined(_MAC_UNIX)
	return LaunchMacBrowserCarbon(pURL, pPrefs, bBringToFront, bNewWindow);
#else
	
    OSErr theErr = noErr;
    CPBusyCursor busyCursor;
    
    Size urlLen = ::strlen(pURL);
    Boolean launched = FALSE;
	
    // Check to see if the default browser is running, otherwise launch it.
    // Then send it an openURL event.

    OSType    defaultBrowser = 'PNst';
    IHXBuffer* pBuffer = NULL;

    Boolean bICStarted = HXInternetConfigMac::StartUsingIC();

	if (pPrefs)
	{
	    if (HXR_OK == pPrefs->ReadPref("PreferredBrowser", pBuffer))
	    {
	    	const char* theBrowserFilePath = (const char*) pBuffer->GetBuffer();
	    	if (theBrowserFilePath)
	    	{
				FInfo 	applFinderInfo;
				FSSpec 	spec;
				if (noErr == FSSpecFromPathName(theBrowserFilePath, &spec))
				{
					if (noErr == FSpGetFInfo(&spec, &applFinderInfo))
					{
						defaultBrowser = applFinderInfo.fdCreator;
					}
				}
			}
	    	pBuffer->Release();
	    }
	}    	
    /* get Preferred Browser from InternetConfig unless either
       InternetConfig isn't linked in, System 8.1, OR
       Preferences dictate a specific browser. */
    if (defaultBrowser == 'PNst')
    {
	    if (ICStart) // not linked with IC for some reason - pre Sys 8.1?
	    {
	    	unsigned char *kDontCareBrowserName = NULL;
	    	
	    	OSStatus err = HXInternetConfigMac::GetHTTPHelper(&defaultBrowser, kDontCareBrowserName);
	    }
		else
		{
			defaultBrowser = NETSCAPE_CREATOR;
		}
	}
    
    OSType    openBrowser = 0;
    FSSpec cachedDefaultBrowserSpec;
    {
		
		// check if any browser is already running
		if (CheckForApplicationRunning(NETSCAPE_CREATOR))
		    openBrowser = NETSCAPE_CREATOR;
		else if (CheckForApplicationRunning(MSIE_CREATOR))
		    openBrowser = MSIE_CREATOR;
		else if (CheckForApplicationRunning(AOL_CREATOR))
		    openBrowser = AOL_CREATOR;

		if (!openBrowser)
		{
		    short	volindex=1;
		    short	vRefNum=0;
		    
		    // Look through all volumes for the default browser application.
		    while (GetVRefnumFromIndex(volindex,&vRefNum)) 
		    {
				short	fileindex=0;
				while (noErr==HX_FSpGetAppSpecFromDesktop(defaultBrowser,fileindex,
						vRefNum,&cachedDefaultBrowserSpec))
				{
				    FInfo	finfo;
				    FSpGetFInfo(&cachedDefaultBrowserSpec,&finfo);
				    if (finfo.fdType=='APPL') 
				    {
						AppleEvent theEvent;
						
						//do this at idle time! don't want to launch app at interrupt time
						if (MakeOpenURLEvent(defaultBrowser, pURL, &theEvent, bNewWindow))
						{
							ProcessSerialNumber* kDontCareAboutPSN = NULL;
							short launchFlags;
							
							if (bBringToFront) 
							{
								launchFlags = (launchContinue | launchNoFileFlags);
							}
							else
							{
								launchFlags = (launchContinue | launchNoFileFlags | launchDontSwitch);
							}
							theErr = FSpLaunchApplicationWithParamsAndFlags(&cachedDefaultBrowserSpec, &theEvent, 
								kDontCareAboutPSN, launchFlags);
							if (theErr == noErr)
							{
								launched = TRUE;
							}
							
							(void) AEDisposeDesc(&theEvent);
						}
						
				    }//if
				    else 
				    {
						fileindex++;
						continue;
				    }//else
					break;
				}//while
				
				if (launched) break;
				volindex++;
		    }//while
		}//if
		
		if( !launched && openBrowser)
		{
		 	const HXBOOL bAsync = TRUE;
		    launched = SendOpenURLOrdered(openBrowser,pURL, bAsync, bBringToFront, bNewWindow);
		}
		
		
		if ( bICStarted && !launched )
		{
			theErr = HXInternetConfigMac::LaunchURL(pURL);
			
			launched = (theErr == noErr);

		}
		goto CleanUp;
    }//if

CleanUp:
	if (bICStarted)
	{
		HXInternetConfigMac::StopUsingIC();
	}
	
	return (launched);
	
#endif // !defined _CARBON

}

HXBOOL	SendOpenURL(OSType	appSignature, const char* pURL, HXBOOL async) 
{
	const HXBOOL bBringToFront = TRUE;
	
	return SendOpenURLOrdered(appSignature, pURL, async, bBringToFront);
}

HXBOOL	SendOpenURLOrdered(OSType	appSignature, const char* pURL, HXBOOL async, HXBOOL bBringToFront, HXBOOL bNewWindow) 
{

	AEDesc				theAppleEvent;
	AEDesc				outAEReply;
	OSErr				err;

	
	if (MakeOpenURLEvent(appSignature, pURL, &theAppleEvent, bNewWindow))
	{
		if (async)
		{
			err = AESend (&theAppleEvent, &outAEReply, 
					 kAENoReply + kAEDontRecord, kAEHighPriority, kNoTimeOut, nil, nil);
		}
		else
		{
			err = AESend (&theAppleEvent, &outAEReply,
					kAEWaitReply+kAEDontRecord, kAEHighPriority, kNoTimeOut,nil,nil);
			AEDisposeDesc(&outAEReply);
		}
		
		if (bBringToFront)
		{
			ActivateApplication(appSignature);
		}
		
		(void) AEDisposeDesc(&theAppleEvent);
	}
	else
	{
		err = -1;
	}

	return (err == noErr);
}

HXBOOL	SendOpenURLSync(OSType	appSignature, const char* pURL) 
{
	return SendOpenURL(appSignature,pURL,false);

}//SendOpenURLSync

HXBOOL	SendOpenURLAsync(OSType	appSignature, const char* pURL) 
{

	return SendOpenURL(appSignature,pURL,true);

}//SendOpenURLASync

// MakeOpenURLEvent
//
// The caller is responsible for disposing of theAppleEventPtr allocated by this routine

HXBOOL	MakeOpenURLEvent(OSType	appSignature, const char* pURL, AEDesc *theAppleEventPtr, Boolean bNewWindow) 
{
	if (IsLocalFileURLAPotentialProblemBecauseOfPercentEncodedUTF8Chars(pURL))
	{
		// for local file URLs which have UTF8 characters in them encoded as % hex,
		// we use an ODOC event instead of the OpenURL event since IE 5.2 and Netscape 6.?
		// can't handle the UTF8 URLs
		return MakeOpenDocumentsEventFromURL(appSignature, pURL, theAppleEventPtr, bNewWindow);
	}
	
	AEDesc				target;
	OSErr				err;
	AEEventClass		eventClass;
	AEEventID			eventID;
	
	if (MOSAIC_CREATOR == appSignature)
	{
		eventClass = MOSAIC_BrowserSuite;
		eventID = MOSAIC_OpenUrlEventID;
	} 
	else
	{
		eventClass = kWebBrowserSuite;
		eventID = kOpenUrlEventID;
	} 

	// Create an application signature address.
	err = AECreateDesc(typeApplSignature, &appSignature, sizeof(OSType), &target);
	require_noerr(err, CreateDescFailed);
	
	// Create the Apple event
	err = AECreateAppleEvent(eventClass, eventID, &target,
					kAutoGenerateReturnID, kAnyTransactionID, theAppleEventPtr);			
	require_noerr(err, AECreateAppleEventFailed);
	
	// Add the URL as the direct object
	err = AEPutParamPtr(theAppleEventPtr, keyDirectObject, typeChar, pURL, strlen(pURL));		
	require_noerr(err, CouldntPutDirectObject);
	
	{
        // Add the targetted window parameter
	const long kSameWindow = -1, kNewWindow = 0;
	
	long window = bNewWindow ? kNewWindow : kSameWindow;
	
	err = AEPutParamPtr(theAppleEventPtr, kOpenUrlWindowID, typeLongInteger, &window, sizeof(window));		
	require_noerr(err, CouldntPutWindow);
		
	(void) AEDisposeDesc(&target);

	return TRUE;
        }

	// error handling: reverse-order cleanup
CouldntPutWindow:
CouldntPutDirectObject:
	(void) AEDisposeDesc(theAppleEventPtr);
AECreateAppleEventFailed:
	(void) AEDisposeDesc(&target);
CreateDescFailed:
	
	return FALSE;
}

HXBOOL	MakeOpenDocumentsEventFromURL(OSType appSignature, const char* pURL, AEDesc *theAppleEventPtr, Boolean bNewWindow) 
{
	
#ifdef _MAC_UNIX
    HX_ASSERT("unimplemented" == NULL);
#else
	AEDesc				target;
	OSErr				err;
	AliasHandle 		alias = NULL;
	CHXFileSpecifier 	fileSpec;
	FSRef			fileRef;
	
	theAppleEventPtr->dataHandle = NULL;
	target.dataHandle = NULL;
	
	fileSpec.SetFromURL(pURL);
	require(CHXFileSpecUtils::FileExists(fileSpec), bail);
	
	fileRef = (FSRef) fileSpec;
	
	// Create an application signature address.
	err = AECreateDesc(typeApplSignature, &appSignature, sizeof(OSType), &target);
	require_noerr(err, bail);
	
	// Create the Apple event
	err = AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments, &target,
					kAutoGenerateReturnID, kAnyTransactionID, theAppleEventPtr);			
	require_noerr(err, bail);
	
	// Add an alias as the direct object
	err = FSNewAlias(NULL, &fileRef, &alias);
	require_noerr(err, bail);
	
	err = AEPutParamPtr(theAppleEventPtr, keyDirectObject, typeAlias, *alias, ::GetHandleSize((Handle) alias));		
	require_noerr(err, bail);
			
	(void) AEDisposeDesc(&target);
	DisposeHandle((Handle) alias);

	return TRUE;

	// error handling: reverse-order cleanup
bail:
	if (theAppleEventPtr->dataHandle) (void) AEDisposeDesc(theAppleEventPtr);
	if (target.dataHandle)	(void) AEDisposeDesc(&target);
	if (alias) DisposeHandle((Handle) alias);

	
	return FALSE;
#endif
}

#if defined(_CARBON) || defined(_MAC_UNIX)
HXBOOL GetPreferredOrSystemBrowser(IHXPreferences* pPrefs, FSRef *outBrowserFSRef, OSType *outBrowserSignature)
{
#ifdef _MAC_UNIX
    HX_ASSERT("GetPreferredOrSystemBrowser unimplemented" == NULL);
#else
	FSRef browserFSRef;
	OSType defaultBrowserSignature;
	HXBOOL bSuccess;
	
	ZeroInit(&browserFSRef);
	
	bSuccess = GetPreferredBrowser(pPrefs, &browserFSRef, &defaultBrowserSignature);
	if (!bSuccess)
	{
		// there isn't a preferred browser in prefs; get one from InternetConfig, or else look
		// for any of our known browsers
		
		unsigned char *kDontCareBrowserName = NULL;
		OSErr err;
		
		err = HXInternetConfigMac::GetHTTPHelper(&defaultBrowserSignature, kDontCareBrowserName);
		if (err == noErr)
		{
			err = GetApplicationFSRefFromSignature(defaultBrowserSignature, &browserFSRef);
		}
		
		if (err != noErr)
		{
			defaultBrowserSignature = NETSCAPE_CREATOR;

			for (int idx = 0; BrowserCreators[idx] != 0; idx++)
			{
				if (noErr == GetApplicationFSRefFromSignature(BrowserCreators[idx], &browserFSRef))
				{
					defaultBrowserSignature = BrowserCreators[idx];
					break;
				}
			}
		}
		if (err == noErr)
		{
			bSuccess = TRUE;
		}
	}
	
	if (outBrowserSignature) *outBrowserSignature = defaultBrowserSignature;
	if (outBrowserFSRef) *outBrowserFSRef = browserFSRef;
	
	return bSuccess;
#endif
}

HXBOOL GetPreferredBrowser(IHXPreferences* pPrefs, FSRef *outBrowserFSRef, OSType *outBrowserSignature)
{
#ifdef _MAC_UNIX
    HX_ASSERT("GetPreferredBrowser unimplemented" == NULL);
#else
	HX_RESULT res;
	OSStatus err;
	const char* theBrowserPref;
	FinderInfo fileInfo;
	IHXBuffer* pBuffer = NULL;
	
	check(outBrowserFSRef != NULL && outBrowserSignature != NULL);
	
	require_nonnull_quiet(pPrefs, NoPrefsAvailable);
	
	res = pPrefs->ReadPref("PreferredBrowser", pBuffer);
	require_quiet(SUCCEEDED(res), CantGetPreferredBrowser);
	
	theBrowserPref = (const char*) pBuffer->GetBuffer();
	require_nonnull(theBrowserPref, CantGetPrefFromBuffer);
	
	require_quiet(strlen(theBrowserPref) > 1, NoBrowserSet);
	
	if (strlen(theBrowserPref) == 4)
	{
		// the pref is an OSType
		*outBrowserSignature = *(OSType *) theBrowserPref;
		
		err = GetApplicationFSRefFromSignature(*outBrowserSignature, outBrowserFSRef);
		require_noerr_quiet(err, CantGetBrowserFSRef);
	}
	else
	{
		// the pref is a persistent file spec or a path
		CHXFileSpecifier browserSpec;
		
		(void) browserSpec.SetFromPersistentString(theBrowserPref);
		require(browserSpec.IsSet(), CantMakeBrowserSpec);
		
		*outBrowserFSRef = (FSRef) browserSpec;
		
		err = FSGetFinderInfo(outBrowserFSRef, &fileInfo, NULL, NULL);
		require_noerr_quiet(err, CantGetBrowserType);
		
		*outBrowserSignature = fileInfo.file.fileCreator;
	}
	
	HX_RELEASE(pBuffer);
	return TRUE;

CantGetBrowserType:
CantGetAppRef:
CantMakeBrowserSpec:
CantGetBrowserFSRef:
NoBrowserSet:
CantGetPrefFromBuffer:
	HX_RELEASE(pBuffer);
CantGetPreferredBrowser:
NoPrefsAvailable:
	return FALSE; 		
#endif
}


static HXBOOL LaunchMacBrowserCarbon(const char* pURL, IHXPreferences* pPrefs, HXBOOL bBringToFront, HXBOOL bNewWindow)
{	
#ifdef _MAC_UNIX
    HX_ASSERT("LaunchMacBrowserCarbon unimplemented" == NULL);
#else
	FSRef browserFSRef;
	//HXBOOL bSuccess;
	OSType defaultBrowserSignature;
	OSType openBrowserSignature;
	OSStatus err;
	HXBOOL bLaunched;
	int idx;

	bLaunched = FALSE;
	
	// get the preferred browser from prefs, if it's available; otherwise, find some other browser
	// to default to
	
	defaultBrowserSignature = 0;
	ZeroInit(&browserFSRef);
	
	GetPreferredOrSystemBrowser(pPrefs, &browserFSRef, &defaultBrowserSignature);
	
	// see if the preferred browser, or any other, is running
	openBrowserSignature = 0;
	if (CheckForApplicationRunning(defaultBrowserSignature))
	{
		openBrowserSignature = defaultBrowserSignature;
	}
	else
	{
		for (idx = 0; BrowserCreators[idx] != 0; idx++)
		{
			if (CheckForApplicationRunning(BrowserCreators[idx]))
			{
				openBrowserSignature = BrowserCreators[idx];
				break;
			}
		}
	}
	
	// CZ 10/29/02 - if a browser is running and the current app is that browser
	// then we must be running the embedded player 
	// in this case don't force the browser to come to the front
	// ie. a hurl from the embedded player shouldn't bring the browser to the front
	
    	if (GetCurrentAppSignature() == openBrowserSignature)
    	{
		bBringToFront = FALSE; 
    	}

	
	// GR 10/1/02 Nasty hack: Netscape 6 and 7 can't deal with URLs in standard OS X format, so
	// for that browser and for local URLs only, we'll make a bogus old-style file:/// URL
	// by taking a full path and replacing colons with slashes
	
	CHXString strNetscapeLocalURL;
	if (openBrowserSignature == 'MOSS' || (openBrowserSignature == 0 && defaultBrowserSignature == 'MOSS'))
	{
		CHXFileSpecifier fileSpec;
		
		fileSpec.SetFromURL(pURL);
		if (fileSpec.IsSet())
		{
			CHXString strPathMunge = fileSpec.GetPathName();
			for (int idx = 0; idx < strPathMunge.GetLength(); idx++)
			{
				if      (strPathMunge[idx] == ':') strPathMunge.SetAt(idx, '/');
				else if (strPathMunge[idx] == '/') strPathMunge.SetAt(idx, ':');
			}
			strPathMunge.FindAndReplace(" ", "%20", TRUE);
			
			strNetscapeLocalURL = "file:///";
			strNetscapeLocalURL += strPathMunge;
			pURL = (const char *) strNetscapeLocalURL;
		}
	}
	
	// try to send the URL via an Apple event to an open browser
	if (openBrowserSignature)
	{
		const HXBOOL bAsync = TRUE;
		
		bLaunched = SendOpenURLOrdered(openBrowserSignature, pURL, bAsync, bBringToFront, bNewWindow);
	}
	
	// if necessary, launch a browser
	if (!bLaunched)
	{
		if (IsRunningNativeOnMacOSX())
		{
			CHXFileSpecifier tempSpec;

			if (IsLocalFileURLAPotentialProblemBecauseOfPercentEncodedUTF8Chars(pURL))
			{
				// we handle the UTF8 problem only with local files; avoid the problem
				// by opening file document directly instead of using a temp file
				tempSpec.SetFromURL(pURL);
			}
			else
			{
				// Is OS X we can't pass an Apple event with the launch (can we?), so we'll make a document to
				// open which redirects to the real page
				
				// make a temp file with the redirect to the real URL; we do this for network URLs
				// or for local URLs that don't have UTF-8
				
				CHXDirSpecifier tempDirSpec;
				CHXString strRedirect;
				const HXBOOL kReplaceExistingTempFile = TRUE;
				
				strRedirect = "<HEAD>\n<META HTTP-EQUIV=\"refresh\" CONTENT=\"0;URL=";
				strRedirect += pURL;
				strRedirect += "\">\n</HEAD>\n";
				
				// make a file spec for a temp file, create the file and open it,
				// and write the redirect into the file
				
				tempDirSpec = CHXFileSpecUtils::MacFindFolder(kOnAppropriateDisk, kChewableItemsFolderType);
				tempSpec = CHXFileSpecUtils::GetUniqueTempFileSpec(tempDirSpec, "HX_hurl_%.html", "%");
				check(tempSpec.IsSet());
				
				err = CHXFileSpecUtils::WriteTextFile(tempSpec, strRedirect, kReplaceExistingTempFile);
				check_noerr(err);
			}
			
			// launch the browser with that temp file
			
			if (err == noErr)
			{
				LSLaunchFSRefSpec launchSpec;
				
				ZeroInit(&launchSpec);
				launchSpec.appRef = &browserFSRef;
				launchSpec.numDocs = 1;
				launchSpec.itemRefs = (FSRef *) tempSpec;
				launchSpec.launchFlags = kLSLaunchDefaults | (bBringToFront ? 0 : kLSLaunchDontSwitch);
				
				err = LSOpenFromRefSpec(&launchSpec, NULL); // NULL -> don't care about FSRef of launched app
			}
			if (err == noErr)
			{
				bLaunched = TRUE;
			}
			
		}
		else
		{
			// OS 9 launch
			AppleEvent theEvent;
			if (MakeOpenURLEvent(defaultBrowserSignature, pURL, &theEvent, bNewWindow))
			{
				ProcessSerialNumber* kDontCareAboutPSN = NULL;
				short launchFlags;
				FSSpec appSpec;
				
				launchFlags = (launchContinue | launchNoFileFlags);
				if (!bBringToFront) 
				{
					launchFlags |= launchDontSwitch;
				}
				
				err = FindApplicationBySignature(defaultBrowserSignature, &appSpec);
				
				if (err == noErr)
				{
					err = FSpLaunchApplicationWithParamsAndFlags(&appSpec, &theEvent, 
						kDontCareAboutPSN, launchFlags);
				}
				if (err == noErr)
				{
					bLaunched = TRUE;
				}
				
				(void) AEDisposeDesc(&theEvent);
			}
		}
	}
	
	// last ditch effort is to rely on InternetConfig
	if (!bLaunched)
	{
		err = HXInternetConfigMac::LaunchURL(pURL);
		if (err == noErr)
		{
			bLaunched = TRUE;
		}
	}
	
	return bLaunched;
#endif
}    

Boolean IsLocalFileURLAPotentialProblemBecauseOfPercentEncodedUTF8Chars(const char *pURL)
{
	// In Mac OS X, 8-bit characters become %-encoded Unicode in the URL, like
	// this:
	//  folderª:file -> file://localhost/folder%E2%84%A2/file.txt
	//
	// IE 5.2 and Netscape 6.? can't handle these URLs (OmniWeb can)
	
	CHXString strURL = pURL;
	Boolean bProblemFound = false;
	
	// this only applies to local files; we don't have a workaround for non-local
	// URLs that have UTF8 in them
	
	if (strURL.Left(5) == "file:") 
	{
		// if there's a '%' in the URL, then look for a %8, %9, %a, %b, etc
		if (strURL.Find('%') != -1)
		{
			const char *kHighBitHex = "89AaBbCcDdEeFf";
			char hexchars[3]; /* Flawfinder: ignore */
			
			hexchars[0] = '%';
			hexchars[2] = 0;
			
			for (int idx = strlen(kHighBitHex) - 1; idx >= 0; idx--)
			{
				hexchars[1] = kHighBitHex[ idx ];
				
				if (strURL.Find(hexchars) != -1)
				{
					bProblemFound = true;
					break;
				}
			}
		}
	}
	return bProblemFound;	
}

void MakeHurlJavascriptURL(const char * pszURL, const char * pszWindowName, 
	HXBOOL bSetPosition, const HXxPoint& ptWindowPos, const HXxSize& sizeWindow, CHXString& outStrURL,
	HXBOOL bCloseJavaScriptWindow)
{
	CHXString strScript, strOptions, strWindowLoc, strWindowSize;
	
	const char * pSizePosScript = 
	
		// open a pop-up window with our name and position settings...
		"javascript:windowVar=window.open(\"%URL%\",\"%windowname%\",\"%options%\")"
		
		// close the _new window our hurl opened to ensure we had a JavaScript context...
		";window.close();"
		
		// force our pop-up window to the front and to the proper position and size
		"windowVar.focus();windowVar.moveTo(%windowloc%);windowVar.resizeTo(%windowsize%);";
	
	const char *pSizeScript =	
		// this one leaves out the moveTo
		
		// open a pop-up window with our name and position settings...
		"javascript:windowVar=window.open(\"%URL%\",\"%windowname%\",\"%options%\")"
		
		// close the _new window our hurl opened to ensure we had a JavaScript context...
		";window.close();"
		
		// force our pop-up window to the front and to the proper  size
		"windowVar.focus();windowVar.resizeTo(%windowsize%);";
		
	const char *pSimpleScriptWithClosingJavascriptWindow =	
		// this one leaves out the moveTo
		
		// open a pop-up window with our name settings...
		"javascript:windowVar=window.open(\"%URL%\",\"%windowname%\",\"resizable=yes\")"
		
		// close the _new window our hurl opened to ensure we had a JavaScript context...
		";window.close();"
		
		// force our pop-up window to the front 
		"windowVar.focus();";
		
	const char *pSimpleScript =	
		// this one leaves out the moveTo & doesn't close the javascript window
		
		// open a pop-up window with our name settings...
		"javascript:windowVar=window.open(\"%URL%\",\"%windowname%\",\"\");"
		
		// force our pop-up window to the front 
		"windowVar.focus();";

	if (bSetPosition)
	{
		strScript = pSizePosScript;
		
		strOptions.Format("height=%ld,width=%ld,top=%ld,left=%ld,resizable=yes",
			sizeWindow.cy, sizeWindow.cx, ptWindowPos.y, ptWindowPos.x);
		strWindowLoc.Format("%ld,%ld", ptWindowPos.x, ptWindowPos.y);
		strWindowSize.Format("%ld,%ld", sizeWindow.cx, sizeWindow.cy);
	}
	else if (sizeWindow.cx > 0 && sizeWindow.cy > 0)
	{
		strScript = pSizeScript;
		
		strOptions.Format("height=%ld,width=%ld,resizable=yes", sizeWindow.cy, sizeWindow.cx);
		strWindowSize.Format("%ld,%ld", sizeWindow.cx, sizeWindow.cy);
	}
	else if (bCloseJavaScriptWindow)
	{
		strScript = pSimpleScriptWithClosingJavascriptWindow;
	}
	else
	{
		strScript = pSimpleScript;
	}
	
	strScript.FindAndReplace("%URL%", pszURL);
	strScript.FindAndReplace("%windowname%", pszWindowName);
	strScript.FindAndReplace("%options%", strOptions);
	strScript.FindAndReplace("%windowloc%", strWindowLoc);
	strScript.FindAndReplace("%windowsize%", strWindowSize);
	
	outStrURL = strScript;
}

#endif // _CARBON

