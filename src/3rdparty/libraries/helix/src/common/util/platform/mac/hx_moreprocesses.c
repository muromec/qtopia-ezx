/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hx_moreprocesses.c,v 1.7 2007/07/06 20:39:19 jfinnecy Exp $
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

#include "platform/mac/HX_MoreProcesses.h"

#if defined(_CARBON) || defined(_MAC_UNIX)
#include "platform/mac/MoreFilesX.h"
#endif

#ifndef _MAC_UNIX
#include "hxmm.h"
#endif

#ifndef _MAC_UNIX /* this is defined always true for _MAC_UNIX builds */
Boolean IsRunningNativeOnMacOSX()
{
#ifdef _CARBON

	long sysVersion;
	OSErr err;

	static int zIsNative = 0;
	
	if (zIsNative == 1) return true;
	if (zIsNative == -1) return false;
	
	
	err = Gestalt(gestaltSystemVersion, &sysVersion);
	check_noerr(err);
	
	if (err == noErr && sysVersion >= 0x1000)
	{
		zIsNative = 1;
		return true;
	}
	else
	{
		zIsNative = -1;
		return false;
	}

#else
	return false;
#endif
}
#endif


Boolean	IsMacInCooperativeThread()
{
	// stuff that's ok to do at system time is OK to do
	// if we're in a cooperative MP task. This includes
	// both the main app MP task ("system time") as well
	// as thread manager threads.
	
	// This is basically a more OS X friendly way to do
	// a HXMM_ATINTERRUPT() check, only it also checks
	// against all preemptive threads and not just the
	// faux interrupt time for which we've called the
	// pnmm interrupt-setting routines.
	
	// MPTaskIsPreemptive(kMPInvalidIDErr) used to work
	// for this but fails to compile now that the task
	// parameter is opaque. That's why it pulls out the
	// actual current task ID.
	
	if ( MPTaskIsPreemptive( MPCurrentTaskID() ) )
	{
		return false;
	}
	else
	{
		return true;
	}
}



//
//	SameFSSpec
//
//	Compares two FSSpec structures and returns true if they point to the same file.
//

Boolean  SameFSSpec(FSSpecPtr   sa,  FSSpecPtr  sb)
{

	if (sa->vRefNum==sb->vRefNum)
	{
		if (sa->parID==sb->parID)
		{
			if (EqualString(sb->name,sa->name,false,false))
			{
				return true;
			}
		}
	}
	return false;
}



//
//	CheckForAppRunningAtPath
//
//	Looks through all processes and looks for one with the same path name.
//
/*
Boolean	CheckForAppRunningAtPath(Str255		path)
{
		OSErr						e 		 	= noErr;		
		ProcessSerialNumber	psn 	 	= {kNoProcess, kNoProcess};
		Boolean					theResult=false;
		FSSpec					serverSpec;
		FSSpec					appSpec;					

		// Get the fsspec of the path.		
		// Return if there isn't a server at the path.
		if (FSMakeFSSpec(0,0,path,&serverSpec)) return false;
		
		
		while (noErr == (e = GetNextProcess (&psn))) 
		{
			ProcessInfoRec	info;
			
			info.processInfoLength = sizeof (info);
			info.processName = nil;
			info.processAppSpec = &appSpec;
			
			if (noErr != (e = GetProcessInformation (&psn, &info))) break;
			
			// Check to see if this is the correct 
			if (SameFSSpec(&appSpec,&serverSpec)) 
			{
				theResult=true;
				break;
			}
			
			continue;
		} 
			
		return (theResult);
}

*/


//
// CheckForApplicationRunning
//
// Looks through all the processes for one matching the signature passed in the "AppSignature"
// 

Boolean	CheckForApplicationRunning(OSType	AppSignature) {
	
	OSErr	err;
	
	err = GetPSNFromSignature(AppSignature, NULL);
	
	return (err == noErr);
	
}


Boolean	CheckForApplicationRunningFSSpec(OSType	AppSignature,FSSpec *spec) {

	return GetApplicationSpec(AppSignature, spec);
	
}

//
// GetPSNSpec
//
// Returns the FSSpec for the related PSN.
// 
Boolean	GetPSNSpec(ProcessSerialNumber*	psn,FSSpec*	theSpec)
{
	Boolean					theResult= false;
	ProcessInfoRec			info;
	OSErr						e=noErr;
	
			
	info.processInfoLength = sizeof (info);
	info.processName = nil;
	info.processAppSpec = theSpec;
	
	e=GetProcessInformation (psn, &info);

	theResult = e ? false : true;

	return theResult;
}


//
// GetApplicationSpec
//
// Looks through all the processes for one matching the signature passed in the "AppSignature"
// Then returns the Application's FSSpec.
// 
Boolean	GetApplicationSpec(OSType	AppSignature, FSSpec	*theSpec) 
{

		OSErr					e 		 = noErr;		
		ProcessSerialNumber		psn 	 = {kNoProcess, kNoProcess};
		Boolean					theResult=false;
		
		while (noErr == (e = GetNextProcess (&psn))) {
			ProcessInfoRec	info;
			
			info.processInfoLength = sizeof (info);
			info.processName = nil;
			info.processAppSpec = theSpec;
			if (noErr != (e = GetProcessInformation (&psn, &info))) break;
			
			if (AppSignature != info.processSignature) continue;
			theResult=true;
			break;
			} /* while */
			
		return (theResult);
}


//
// GetCurrentAppSpec
//
// Returns the FSSpec for the currently running application.
// 

Boolean GetCurrentAppSpec(FSSpec *theSpec)
{

	OSErr				err;	
	ProcessSerialNumber	psn;
	ProcessInfoRec		pir;

	if (theSpec == NULL) return false;
		
	psn.highLongOfPSN = 0;
	psn.lowLongOfPSN = kCurrentProcess;
	
	pir.processInfoLength = sizeof(ProcessInfoRec);
	pir.processName = nil;
	pir.processAppSpec = theSpec;
	err = GetProcessInformation (&psn, &pir);
	
	return (err == noErr);
}


//
// GetCurrentAppSignature
//
// Returns the signature for the currently running application.
// 

OSType GetCurrentAppSignature(void)
{

	OSErr				err;	
	ProcessSerialNumber	psn;
	ProcessInfoRec		pir;
		
	psn.highLongOfPSN = 0;
	psn.lowLongOfPSN = kCurrentProcess;
	
	pir.processInfoLength = sizeof(ProcessInfoRec);
	pir.processName = nil;
	pir.processAppSpec = nil;
	err = GetProcessInformation (&psn, &pir);
	
	if (err == noErr)	return pir.processSignature;
	else				return '????';
}


//
// GetPSNFromSignature returns the ProcessSerialNumber for the
// app with the given signature if the app is running
//
// psn can be nil if the caller only wants to know if the
// app is running
//

OSErr GetPSNFromSignature(OSType signature, ProcessSerialNumber *psn)
{
	ProcessSerialNumber	currPSN;
	ProcessInfoRec		currPIR;
	OSErr				err;
	
	currPSN.lowLongOfPSN = kNoProcess;
	currPSN.highLongOfPSN = 0;
	
	do {
		err = GetNextProcess(&currPSN);
		if (err == noErr)
		{
			currPIR.processName = NULL;
			currPIR.processAppSpec = NULL;
			currPIR.processInfoLength = sizeof(ProcessInfoRec);
			err = GetProcessInformation(&currPSN, &currPIR);
			if (err == noErr && currPIR.processSignature == signature)
			{
				if (psn) *psn = currPSN;
				break;
			}
		}
	} while (err == noErr);
	
	return err;
}

//
// GetSignatureFromPSN returns the signature for the
// app with the given PSN
//
// signature can be nil if the caller only wants to know if the
// app is running
//

OSErr GetSignatureFromPSN(const ProcessSerialNumber *psn, OSType *signature)
{
	ProcessSerialNumber	currPSN;
	ProcessInfoRec		currPIR;
	OSErr			err;
	Boolean 		bSamePSN;
	
	currPSN.lowLongOfPSN = kNoProcess;
	currPSN.highLongOfPSN = 0;
	
	do {
		err = GetNextProcess(&currPSN);
		if (err == noErr)
		{
			if (SameProcess(&currPSN, psn, &bSamePSN) == noErr
			  && bSamePSN)
			{
				currPIR.processName = NULL;
				currPIR.processAppSpec = NULL;
				currPIR.processInfoLength = sizeof(ProcessInfoRec);
				err = GetProcessInformation(&currPSN, &currPIR);
				if (err == noErr)
				{
					if (signature) 
					{
						*signature = currPIR.processSignature;
					}
				}
				break;
			}
			
			
		}
	} while (err == noErr);
	
	return err;
}



//
// CountAppSignatures
//
// Looks through all the processes and returns a count of the processes with AppSignature.
//
short	CountAppSignatures(OSType	AppSignature) {

		OSErr					e 		 = noErr;		
		ProcessSerialNumber		psn 	 = {kNoProcess, kNoProcess};
		short					theResult=0;
		
		while (noErr == (e = GetNextProcess (&psn))) {
			ProcessInfoRec	info;
			
			info.processInfoLength = sizeof (info);
			info.processName = nil;
			info.processAppSpec = nil;
			if (noErr != (e = GetProcessInformation (&psn, &info))) break;
			
			if (AppSignature != info.processSignature) continue;
			theResult++;

			} /* while */
			
		return (theResult);
		
}

//
//	ActivateApplication
//
void	ActivateApplication(OSType	AppSignature) {

		OSErr					e 		 = noErr;		
		ProcessSerialNumber		psn 	 = {kNoProcess, kNoProcess};
		
		while (noErr == (e = GetNextProcess (&psn))) {
			ProcessInfoRec	info;
			
			info.processInfoLength = sizeof (info);
			info.processName = nil;
			info.processAppSpec = nil;
			if (noErr != (e = GetProcessInformation (&psn, &info))) break;
			
			if (AppSignature != info.processSignature) continue;
			
			ActivateApplicationPSN(&psn);
			
			break;
			} /* while */
}

//
//	ActivateApplicationPSN
//
void	ActivateApplicationPSN(ProcessSerialNumber	*psn) {

	if (psn == 0L) return;
	
	SetFrontProcess(psn);

}


//
//	FSpLaunchApplication launches a Mac application given a file spec
//

OSErr	FSpLaunchApplication (FSSpec	*appSpec) {

	return FSpLaunchApplicationWithParams(appSpec, NULL, NULL);
}

//
//	FSpLaunchApplication launches a Mac application given a file spec
//  and an option Apple Event to include as a launch parameter
//
//  If psn is non-nil, the launched processes' PSN is returned
//

OSErr FSpLaunchApplicationWithParams(FSSpec *appSpec, const AppleEvent *theEvent, ProcessSerialNumber *psn)
{
	return FSpLaunchApplicationWithParamsAndFlags(appSpec, theEvent, psn, (launchContinue | launchNoFileFlags));	
}

OSErr FSpLaunchApplicationWithParamsAndFlags(FSSpec *appSpec, const AppleEvent *theEvent, ProcessSerialNumber *psn, short launchFlags)
{
	OSErr	err = noErr;
	LaunchParamBlockRec		launchPB;
	AEDesc					paramsDesc;
	
	paramsDesc.dataHandle = NULL;

	launchPB.launchBlockID 			= extendedBlock;
	launchPB.launchEPBLength 		= extendedBlockLen;
	launchPB.launchFileFlags 		= 0;
	launchPB.launchControlFlags		= launchFlags;
	launchPB.launchAppSpec 			= appSpec;
	launchPB.launchAppParameters 	= NULL;
	
	// if we were passed an event, convert and attach that to the launch param block
	if (theEvent)
	{
		err = AECoerceDesc(theEvent, typeAppParameters, &paramsDesc);
		if (err == noErr)
		{
#if !defined(_CARBON) && !defined(_MAC_UNIX)
			HLock(paramsDesc.dataHandle);
#endif
			launchPB.launchAppParameters = (AppParametersPtr) *paramsDesc.dataHandle;
		}
	}
	
	// do the launch
	if (err == noErr)
	{
		err = LaunchApplication(&launchPB);
		if (err == noErr && psn != NULL)
		{
			*psn = launchPB.launchProcessSN;
		}
	}

	// if we had an event to convert and attach, dispose our converted version
	if (paramsDesc.dataHandle) (void) AEDisposeDesc(&paramsDesc);
	
	return err;
}


OSErr	FSpLaunchApplicationPSN (FSSpec	*appSpec,ProcessSerialNumber*	psn) {

	return FSpLaunchApplicationWithParams(appSpec, NULL, psn);
	
}



//
// OpenDocumentWithApplication sends an event to the target application
// to open the document, launching the application if necessary.
//
// Unlike sending an Apple event to the finder, this gives us an
// error if the open doesn't happen
//

OSErr OpenDocumentWithApplication(OSType signature, FSSpec *file)
{
	OSErr				err;
	ProcessSerialNumber	appPSN;
	FSSpec				appSpec;
	AEDesc				targetAddrDesc;
	AppleEvent			theAppleEvent, theReplyEvent;
	AliasHandle			fileAlias;
	
	theAppleEvent.dataHandle = NULL;	// to facilitate cleanup
	targetAddrDesc.dataHandle = NULL;
	appPSN.lowLongOfPSN = 0;
	fileAlias = NULL;
	
	err = GetPSNFromSignature(signature, &appPSN);
	if (err != noErr)
	{

		// app isn't running, we'll try to launch it
		//
		// address the event target by the process's signature

		err = AECreateDesc(typeApplSignature, &signature, sizeof(DescType), &targetAddrDesc);
	}
	else
	{
		// address target by PSN
		err = AECreateDesc(typeProcessSerialNumber, &appPSN, sizeof(ProcessSerialNumber), &targetAddrDesc);
	}
	
	if (err == noErr)
	{
		// make the Apple event, stuff in an alias to the file as the direct object
		err = AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments, &targetAddrDesc, 
				kAutoGenerateReturnID, kAnyTransactionID, &theAppleEvent);
		if (err == noErr)
		{
			err = NewAliasMinimal(file, &fileAlias);
			if (err == noErr)
			{
				HLock((Handle) fileAlias);
				err = AEPutParamPtr(&theAppleEvent, keyDirectObject, typeAlias, 
						*fileAlias, GetHandleSize((Handle) fileAlias));
			}
		}
	}

	if (err == noErr)
	{
		if (appPSN.lowLongOfPSN)
		{
			// app is running and we know its PSN, so
			// send the event directly to it
			err = AESend(&theAppleEvent, &theReplyEvent, kAENoReply, 
				kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
				
			(void) SetFrontProcess(&appPSN);
		}
		else
		{
			// the app isn't running, so launch it with the
			// event attached
			err = FindApplicationBySignature(signature, &appSpec);
			if (err == noErr) 
			{
				err = FSpLaunchApplicationWithParams(&appSpec, &theAppleEvent, NULL);
			}
		}
	}

	if (fileAlias) 					DisposeHandle((Handle) fileAlias);
	if (theAppleEvent.dataHandle)	(void) AEDisposeDesc(&theAppleEvent);
	if (targetAddrDesc.dataHandle)	(void) AEDisposeDesc(&targetAddrDesc);
	
	return err;
}

//
// LaunchApplicationBySignature finds an application on a local 
// volume and launches it.  theEvent can be null if no Apple
// event should be attached.
//

OSErr LaunchApplicationBySignature(OSType signature, AppleEvent *theEvent)
{
	OSErr	err;
	FSSpec	appSpec;
	
	err = FindApplicationBySignature(signature, &appSpec);
	if (err == noErr) 
	{
		err = FSpLaunchApplicationWithParams(&appSpec, theEvent, NULL);
	}

	return err;
}

//
// FindApplicationBySignature finds an application on a local (not server)
// volume by checking the Desktop Databases of the mounted drives
//
// appSpec can be NULL if the caller is just interested in if the app
// exists or not
//

OSErr FindApplicationBySignature(OSType signature, FSSpec *appSpec)
{
#ifndef _MAC_UNIX
	Str63					name;
	short					volIndex;
	short					vRefNum;
	HParamBlockRec			hpb;
	DTPBRec					dtpb;
	GetVolParmsInfoBuffer	gvpb;
	Boolean					foundAppFlag;
	Boolean					giveUpFlag;
#endif
	OSErr					err;
	
#if defined(_CARBON) || defined(_MAC_UNIX)
	if (IsRunningNativeOnMacOSX())
	{
		// Mac OS X; we can't rely on the desktop database, so use launch services
		FSRef appRef;
		
		err = GetApplicationFSRefFromSignature(signature, &appRef);
		if (err == noErr)
		{
			err = FSRefMakeFSSpec(&appRef, appSpec);
		}
		return err;
	}
#endif
	
#if defined(_MAC_UNIX)
	return -1; // should never get here
#else
	foundAppFlag = false;
	giveUpFlag = false;


	// step through each volume available
	volIndex = 1;
	
	do {
		name[0] = 0;
		hpb.volumeParam.ioNamePtr = name;
		hpb.volumeParam.ioVolIndex = volIndex;
		hpb.volumeParam.ioVRefNum = 0;
		hpb.volumeParam.ioCompletion = NULL; // shouldn't matter since we're calling sync
		
		err = PBHGetVInfoSync(&hpb);
		if (err == noErr)
		{
			
			vRefNum = hpb.volumeParam.ioVRefNum;
		
			// check only local volumes for the application
			//
			// PBHGetVolParms returns vMServerAdr of zero for local volumes
			//
			// name and vRefNum are already set in param block hpb from the PBHGetVInfo call
			
			hpb.ioParam.ioBuffer = (char *) &gvpb;
			hpb.ioParam.ioReqCount = sizeof(GetVolParmsInfoBuffer);
			
			err = PBHGetVolParmsSync(&hpb);
			if (err == noErr && gvpb.vMServerAdr == 0)
			{
				// now we know this is a valid, local volume; 
				// see if it has a desktop database
				dtpb.ioNamePtr = NULL;
				dtpb.ioVRefNum = vRefNum;
				
				err = PBDTGetPath(&dtpb);
				if (err == noErr)
				{
					// this volume has a desktop database; search it
					name[0] = 0;
					dtpb.ioNamePtr = name;
					dtpb.ioIndex = 0; // get newest application
					dtpb.ioFileCreator = signature;
					dtpb.ioCompletion = NULL; // shouldn't matter since we're calling sync
					// dtpb.ioDTRefNum set by GetPath call
					
					err = PBDTGetAPPLSync(&dtpb);
					if (err == noErr)
					{
						FSSpec	foundSpec;
						
						err = FSMakeFSSpec(vRefNum, dtpb.ioAPPLParID, name, &foundSpec);
						
						if (err == noErr)
						{
							if (appSpec) *appSpec = foundSpec;
							
							// we got the app; bail
							foundAppFlag = true;
						}
					}
				}
			}
			
			
		}
		else
		{
			// PBHGetVInfoSync returned an error
			giveUpFlag = true;
		}
		volIndex++;
		
	} while (!giveUpFlag &&!foundAppFlag);
	
	// at this point, we should have noErr indicating that we found
	// an app and the FSSpec is valid
	
	return err;
#endif // !defined _MAC_UNIX
}


OSErr SendQuitEventToApplication(ProcessSerialNumber *psn)
{
	OSErr			err;
	AEAddressDesc	targetAddrDesc;
	AppleEvent		theAppleEvent;
	AppleEvent		replyAppleEvent;
	
	// address target by PSN
	err = AECreateDesc(typeProcessSerialNumber, psn, sizeof(ProcessSerialNumber), &targetAddrDesc);
	if (err == noErr)
	{
		// make the quit event
		err = AECreateAppleEvent(kCoreEventClass, kAEQuitApplication, &targetAddrDesc, 
				kAutoGenerateReturnID, kAnyTransactionID, &theAppleEvent);
	
		(void) AEDisposeDesc(&targetAddrDesc);
	}
	
	if (err == noErr)
	{
		// send it
		err = AESend(&theAppleEvent, &replyAppleEvent, kAENoReply, 
							kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
		(void) AEDisposeDesc(&theAppleEvent);
	}
	return err;
}

#if defined(_CARBON) || defined(_MAC_UNIX)
OSStatus GetApplicationFSRefFromSignature(OSType signature, FSRef *outAppFSRef)
{
	OSStatus err;
	
	const OSType kAnyFileType = kLSUnknownType;
	const CFStringRef kAnyExtension = NULL;
	CFURLRef* kDontWantURL = NULL;
	
	if (IsRunningNativeOnMacOSX())
	{
		err = LSGetApplicationForInfo(kAnyFileType, signature, kAnyExtension, kLSRolesAll, outAppFSRef, kDontWantURL);
	}
	else
	{
		FSSpec appSpec;
		err = FindApplicationBySignature(signature, &appSpec);
		if (err == noErr)
		{
			err = FSpMakeFSRef(&appSpec, outAppFSRef);
		}
	}
	
	return err;
}

#endif
