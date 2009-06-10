/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hx_moreprocesses.h,v 1.6 2007/07/06 20:39:27 jfinnecy Exp $
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

#pragma once

#ifdef __cplusplus
	extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////
// OS X-savvy calls
////////////////////////////////////////////////////////////////////////////////////

#ifdef _MAC_UNIX
#define IsRunningNativeOnMacOSX() (1)
#else
Boolean IsRunningNativeOnMacOSX(void); // returns false for non-Carbon code and Carbon on OS 9
#endif
		
Boolean IsMacInCooperativeThread(void); // use on OS X instead of HXMM_ATINTERRUPT

// Calls for currently-running processes
OSType GetCurrentAppSignature(void);
Boolean	CheckForApplicationRunning(OSType AppSignature);
OSErr GetPSNFromSignature(OSType signature, ProcessSerialNumber *psn);
OSErr GetSignatureFromPSN(const ProcessSerialNumber *psn, OSType *signature);
short CountAppSignatures(OSType AppSignature);

void ActivateApplication(OSType	AppSignature);
void ActivateApplicationPSN(ProcessSerialNumber	*psn);

// Calls for launching processes
#if defined(_CARBON) || defined(_MAC_UNIX)
OSStatus GetApplicationFSRefFromSignature(OSType signature, FSRef *outAppFSRef);
#endif

OSErr LaunchApplicationBySignature(OSType signature, AppleEvent *theEvent);
OSErr SendQuitEventToApplication(ProcessSerialNumber *psn);



// pre-OS X calls...
Boolean	CheckForApplicationRunningFSSpec(OSType	AppSignature, FSSpec *spec);

Boolean	GetApplicationSpec(OSType	AppSignature, FSSpec	*theSpec);
Boolean	GetPSNSpec(ProcessSerialNumber*	psn,FSSpec*	theSpec);
Boolean GetCurrentAppSpec(FSSpec *theSpec);
Boolean SameFSSpec(FSSpecPtr   sa,  FSSpecPtr  sb);

OSErr	FSpLaunchApplication (FSSpec	*appSpec);
OSErr	FSpLaunchApplicationPSN (FSSpec	*appSpec,ProcessSerialNumber*	psn);	// Returns a ProcessSerialNumber
OSErr 	FSpLaunchApplicationWithParams(FSSpec *appSpec, const AppleEvent *theEvent, ProcessSerialNumber *psn);
OSErr 	FSpLaunchApplicationWithParamsAndFlags(FSSpec *appSpec, const AppleEvent *theEvent, ProcessSerialNumber *psn, short launchFlags);

OSErr FindApplicationBySignature(OSType signature, FSSpec *appSpec);	// searches Desktop Databases, use GetApplicationFSRefFromSignature instead
OSErr OpenDocumentWithApplication(OSType signature, FSSpec *file); // either launch or send event directly, as appropriate

#ifdef __cplusplus
 }
#endif
