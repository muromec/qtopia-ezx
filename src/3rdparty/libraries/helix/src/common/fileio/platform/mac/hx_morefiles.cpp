/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hx_morefiles.cpp,v 1.5 2004/07/09 18:19:54 hubbe Exp $
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
 * developer and/or licensor of the Original Code and owns the
 * copyrights in the portions it created.
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

/*
	This code was descended from Apple Sample Code, but the
	code may have been modified.
*/


#include "hx_morefiles.h"
#include <string.h>
#ifndef _MAC_MACHO
#include "datetimeutils.h"
#endif
#ifndef _CARBON
#include "MoreFilesExtras.h"
#else
#include "MoreFilesX.h"
#include "hxassert.h"
#endif
#include "hxstrutl.h"

//	FSpGetItemSpec
//
//	Returns an FSSpec which contains everything but the name of an item contained in the
// specified directory. This merely sets up the returned FSSpec so that the name of the item
// can be copied into the FSSpec, or can easily be used in the FSMakeFSSpec call.
//
//  GR 8/8/01 This was only used by FSpIterateDirectory, and was really inefficient, so it's gone now.



//
// FSpIterateDirectory
//
// Allows for easy iteration of a SINGLE directory. It does not look any deeper than
// the directory with which it starts.  This is to make it quick and easy. 
//
// returns false when the file specified by the "index" is not valid, or any other error
// 		  happens. Returns true if a file was found at that index.  The index starts at 1.
//
// Note: this is really inefficient if called repeatedly since it has to make an
//       extra call to PBGetCatInfo to figure out the directory's dirID.  It is much
//       better to call FSpGetNthDirectoryItem, below, instead if you will be making
//       repeated calls.

Boolean	  FSpIterateDirectory(FSSpec*	dirSpec,short index,FSSpec* fileSpec) 
{
	OSErr err;
	long dirID;
	Boolean isDir;
	
	err = FSpGetDirectoryID(dirSpec, &dirID, &isDir);
	if ((err == noErr) && isDir)
	{
		err = FSpGetNthDirectoryItem(dirSpec->vRefNum, dirID, index, fileSpec);
	}
	return (err == noErr);
	
}

//
// GetVRefnumFromIndex
//
// Returns the n'th item from the directory specified by vRefNum and dirID.  Can return
// a spec to a directory or to a file.
//

OSErr	FSpGetNthDirectoryItem(short vRefNum, long parID, short index, FSSpec *theSpec)
{
	OSErr		err;
	CInfoPBRec	cInfo;
	
	theSpec->vRefNum = vRefNum;
	theSpec->parID = parID;
	
	cInfo.hFileInfo.ioVRefNum = vRefNum;
	cInfo.hFileInfo.ioDirID = parID;
	cInfo.hFileInfo.ioNamePtr = theSpec->name;
	cInfo.hFileInfo.ioFDirIndex = index;	// use vRefNum, dirID and index
	err = PBGetCatInfoSync(&cInfo);
		
	return err;
}



//
// GetVRefnumFromIndex
//
// If the return value of this function is true, then the *realrefnum parameter contains
// the RealRefNum of the volume at that index.
//
// the index starts at 1 and continues until you receive an error.
//
Boolean		GetVRefnumFromIndex(short	index,short	*realRefNum) {

	Boolean			theResult=false;
	OSErr			err=noErr;
	HParamBlockRec	pb;
	
	if (realRefNum==0L) goto CleanUp;

	memset(&pb,0,sizeof(ParamBlockRec));
	
	pb.volumeParam.ioVolIndex=index;
	
	err=PBHGetVInfoSync(&pb);
	
	*realRefNum=pb.volumeParam.ioVRefNum;
	
	theResult=(err==noErr);

CleanUp:
	return(theResult);
}

//
// GetVRefnumFromIndex
//
// If the return value of this function is true, then the *realrefnum parameter contains
// the RealRefNum of the volume at that index.
//
// the index starts at 1 and continues until you receive an error.
//
//
// This version only returns LOCAL UNLOCKED volumes, not remote or locked ones.
//
Boolean		GetLocalVRefnumFromIndex(short *index, short *realRefNum)
{
	Boolean			theResult=false;
	OSErr			err=noErr;
	ParamBlockRec	pb;
	HParamBlockRec	hpb;
	Boolean			local;
	short			theSize;
	OSType			*volTypePtr;
	
	if (realRefNum==0L) goto CleanUp;

	do {
		(*index)++;
	
		// Get the next volume
		memset(&hpb,0,sizeof(HParamBlockRec));
		hpb.volumeParam.ioVolIndex = *index;
		err=PBHGetVInfoSync(&hpb);
		*realRefNum=hpb.volumeParam.ioVRefNum;
		theResult=(err==noErr);
		local = TRUE;
	
		if (theResult)
		{
			// Make sure it's unlocked
			if ((((unsigned short)hpb.volumeParam.ioVAtrb) & ((unsigned short)32896)) != 0)
				local = FALSE;
			else
			{
				// Make sure it's local
				memset(&pb,0,sizeof(ParamBlockRec));
				pb.ioParam.ioBuffer = (Ptr)&theSize;
				pb.ioParam.ioCompletion = NULL;
				pb.ioParam.ioVRefNum = *realRefNum;
				err = PBGetVolMountInfoSize(&pb);
				if (!err)
				{
					pb.ioParam.ioBuffer = NewPtr((long)theSize);
					err = PBGetVolMountInfo(&pb);
					if (!err)
					{
						volTypePtr = (OSType *) &((char *)pb.ioParam.ioBuffer)[2];
						if (*volTypePtr=='afpm')	// AppleShare volume - we don't want these.
							local = FALSE;
					}
					DisposePtr((Ptr)pb.ioParam.ioBuffer);
				}
			}
		}
	} while (theResult && !local);

CleanUp:
	return(theResult);
}

//
//	FSpGetVersion
//
//	Retrieves the version number from a file specified by an FSSpec.
//
Boolean		FSpGetVersion(FSSpec	*theFile, Byte *majorversion, 
						  Byte *minorversion, Byte *revision) {


	Handle		r=0L;
	short		saveResFile;
	short		theResFile=-1;
	short		vers;
	Boolean		theResult=false;

	saveResFile=CurResFile();

	if (theFile == 0L) goto CleanUp;
	
	theResFile=FSpOpenResFile(theFile,fsRdPerm);
	if (theResFile != -1) {
		r = GetResource('vers',1);
		if (r==0L) goto CleanUp;
		
		vers=(short)**r;
		
		if (majorversion != 0L) {
			*majorversion = (Byte)vers;
			}//if
		
		vers << sizeof(Byte);
		
		if (minorversion != 0L) {
			*minorversion = vers | 0xF0;
			}//if
		
		if (revision != 0L) {
			*revision = vers | 0x0F;
			}//if
		
		}//if
CleanUp:
	//if (theResFile >=0) CloseResFile(theResFile);
	UseResFile(saveResFile);
	return(theResult);
}


//
//	FSpGetVersionLong
//
//	Retrieves the version number from a file specified by an FSSpec. Returns it as a long.
//
Boolean		FSpGetVersionLong(FSSpec	*theFile, short	*theversion) {


	Handle		r=0L;
	short		saveResFile;
	short		theResFile=-1;
	Boolean		theResult=false;

	saveResFile=CurResFile();

	if (theFile == 0L) goto CleanUp;
	if (theversion == 0L) goto CleanUp;
	
	theResFile=FSpOpenResFile(theFile,fsRdPerm);
	if (theResFile != -1) {
		r = GetResource('vers',1);
		if (r==0L) goto CleanUp;
		
		BlockMove(*r,theversion,sizeof(long));
				
		}//if
CleanUp:
	//if (theResFile >=0) CloseResFile(theResFile);
	UseResFile(saveResFile);
	return(theResult);
}

// 
// FSpGetVersionMessage
//
// Retrieves the version message from a file specified by an FSSpec.
//

Boolean		FSpGetVersionMessage(FSSpec	*theFile, char	*theversion, UINT32 ulBufLen) {


	Handle		r=0L;
	short		saveResFile;
	short		theResFile=-1;
	Boolean		theResult=false;
	Str255		message;

	saveResFile=CurResFile();

	if (theFile == 0L) goto CleanUp;
	if (theversion == 0L) goto CleanUp;
	
	theResFile=FSpOpenResFile(theFile,fsRdPerm);
	if (theResFile != -1) {
		r = GetResource('vers',1);
		if (r==0L) goto CleanUp;
		
		HLock(r);
		HNoPurge(r);
		
		*r+=(sizeof(Byte)*4)+sizeof(short);
		
		*r+=**r;
		
		BlockMove(*r,message,sizeof(Str255));
		
#ifdef _CARBON
		p2cstrcpy((char*)message, message);
#else
		p2cstr(message);
#endif
		
		SafeStrCpy(theversion,(char *)message, ulBufLen);
				
		}//if
CleanUp:
	//if (theResFile >=0) CloseResFile(theResFile);
	UseResFile(saveResFile);
	return(theResult);
}

// 
// FSpGetFileAge
//
// This function returns the age of a file relative to the current time.
//
// It is legal to pass in NULL if you don't want a particular age granularity.

Boolean
FSpGetFileAge(FSSpec*	file,long* minutes,long* seconds,long* ticks)
{

	HParmBlkPtr		fileinfo=(HParmBlkPtr)NewPtrClear(sizeof(HParamBlockRec));
	Boolean			theResult=false;
	unsigned long		curseconds;
	
	GetDateTime(&curseconds); // seconds since 1904

	if (file == 0L) goto CleanUp;
	if (fileinfo == 0L) goto CleanUp;

	fileinfo->fileParam.ioNamePtr=file->name;
	fileinfo->fileParam.ioDirID=file->parID;
	fileinfo->fileParam.ioVRefNum=file->vRefNum;
	
	if (noErr != PBHGetFInfoSync(fileinfo)) goto CleanUp;
	
	if (ticks != NULL) 
	{
	    unsigned long createTicks = (fileinfo->fileParam.ioFlCrDat * 60);
	    unsigned long curticks = curseconds * 60;
	    *ticks=curticks - createTicks;
	    if (*ticks < 0)
	    	*ticks = 0;
	}
	if (seconds != NULL) 
	{
	    unsigned long createSecs = fileinfo->fileParam.ioFlCrDat;
	    *seconds=curseconds - createSecs;
	    if (*seconds < 0)
	    	*seconds = 0;
	}
	if (minutes != NULL) 
	{
	    unsigned long createMinutes = fileinfo->fileParam.ioFlCrDat / 60;
	    unsigned long curMinutes = curseconds / 60;
	    *minutes = (curMinutes - createMinutes);
	    if (*minutes < 0)
	    	*minutes = 0;
	}
	
	theResult=true;
	
CleanUp:
	if (fileinfo != 0L) DisposePtr((Ptr)fileinfo);	

	return (theResult);

}

Boolean		
IsFolder(FSSpec* spec)
{
    long dirID;
    Boolean isDir;
    OSErr err;
    
    err = FSpGetDirectoryID(spec, &dirID, &isDir);
    return ((err == noErr) && isDir);    
}

OSErr	GetMoreVInfo(short volref, StringPtr name, unsigned long *volcreated, Boolean *islocked)
{
HVolumeParam	block;
OSErr			err;

	memset(&block, 0x00, sizeof(block));
	block.ioVolIndex = 0;
	block.ioVRefNum = volref;
	block.ioNamePtr = name;
	err = PBHGetVInfoSync((HParmBlkPtr)&block);
	if (!err) {
		if (islocked) *islocked = (block.ioVAtrb & (1<<7)) != 0;
		if (volcreated) *volcreated = block.ioVCrDate;
	}
	return err;
}

#ifdef _CARBON
// Stuff that used to be in MoreFilesExtras but isn't in MoreFilesX in FSSpec form

OSErr FSpGetDirectoryID(const FSSpec* pDirSpec, long *pDirID, Boolean *pIsDir)
{
	FSRef dirRef;
	Boolean isDir = false;
	long dirID = 0;
	OSErr err;
	
	err = FSpMakeFSRef(pDirSpec, &dirRef);
	if (err == noErr)
	{
		err = FSGetNodeID(&dirRef, &dirID, &isDir);
	}
	
	if (pDirID) *pDirID = dirID;
	if (pIsDir) *pIsDir = isDir;
	return err;
}

OSErr FSpMoveRenameCompat(const FSSpec * srcSpec, const FSSpec * dstSpec, ConstStr255Param  copyName)
{
	FSRef	srcRef, destDirRef, newRef;
	HFSUniStr255 hfsUniName;
	OSErr err;
	
	err = FSpMakeFSRef(srcSpec, &srcRef);
	require_noerr(err, CantMakeSrcRef);
	
	err = FSpMakeFSRef(dstSpec, &destDirRef);
	require_noerr(err, CantMakeDestRef);
	
	if (copyName)
	{
		err = HFSNameGetUnicodeName(copyName, kTextEncodingUnknown, &hfsUniName);
		require_noerr(err, CantMakeUnicodeName);
	}
	
	err = FSMoveRename(&srcRef, &destDirRef, hfsUniName.length, copyName ? hfsUniName.unicode : NULL,
		kTextEncodingUnknown, &newRef);
	require_noerr(err, CantMoveRename);
	
	return noErr;
	
// reverse-order cleanup

CantMoveRename:
CantMakeUnicodeName:
CantMakeDestRef:
CantMakeSrcRef:
	
	return err;
}

OSErr	DeleteDirectory(short vRefNum, long dirID, ConstStr255Param name)
{
	FSRef dirRef;
	OSErr err;

	err = FSMakeFSRef(vRefNum, dirID, name, &dirRef);
	if (err == noErr)
	{
		err = FSDeleteContainerContents(&dirRef);
	}
	return err;
}


OSErr FSpFileCopy(const FSSpec *srcSpec, const FSSpec *dstSpec, ConstStr255Param copyName,
	void *copyBufferPtr, long copyBufferSize, Boolean preflight)
{
	// Ultimately, this should be replaced with a new one that accompanies MoreFilesX, but that's not
	// yet available from Apple
	void *pBuff = NULL;
	long dirID;
	Str255 newName;
	FSSpec newSpec;
	short srcRefNum = -1;
	short destRefNum = -1;
	FInfo fInfo;
	OSErr err;
	
	// get a buffer to use for copying
	if (copyBufferPtr)
	{
		pBuff = copyBufferPtr;
	}
	else
	{
		copyBufferSize = 16*1024;
		pBuff = NewPtr(copyBufferSize);
	}
	require_nonnull(pBuff, CantGetBuffer);
	

	// copy the name of the new file
	if (copyName)
	{
		BlockMoveData(&copyName[0], &newName[0], 1 + copyName[0]);
	}
	else
	{
		BlockMoveData(&srcSpec->name[0], &newName[0], 1 + srcSpec->name[0]);
	}
	
	// make a new file; bail if it already exists
	err = FSpGetDirectoryID(dstSpec, &dirID, NULL);
	require_noerr(err, CantGetDirID);
	
	err = FSMakeFSSpec(dstSpec->vRefNum, dirID, newName, &newSpec);
	require(err == fnfErr, FileExistsAlready);
	
	// create the file
	
	err = FSpCreate(&newSpec, 0, 0, smSystemScript);
	require_noerr(err, CantMakeNewFile);
	
	// copy the data forks
	
	err = FSpOpenDF(srcSpec, fsRdPerm, &srcRefNum);
	require_noerr(err, CantOpenSourceDF);
	
	err = FSpOpenDF(&newSpec, fsRdWrPerm, &destRefNum);
	require_noerr(err, CantOpenDestDF);
	
	err = FSCopyFork(srcRefNum, destRefNum, pBuff, copyBufferSize);
	require_noerr(err, CantCopyDataFork);
	
	FSClose(srcRefNum);
	FSClose(destRefNum);
	
	srcRefNum = -1;
	destRefNum = -1;

	// copy the resource forks
	
	err = FSpOpenRF(srcSpec, fsRdPerm, &srcRefNum);
	require_noerr(err, CantOpenSourceRF);
	
	err = FSpOpenRF(&newSpec, fsRdWrPerm, &destRefNum);
	require_noerr(err, CantOpenDestRF);
	
	err = FSCopyFork(srcRefNum, destRefNum, pBuff, copyBufferSize);
	require_noerr(err, CantCopyResourceFork);
	
	FSClose(srcRefNum);
	FSClose(destRefNum);
	
	srcRefNum = -1;
	destRefNum = -1;
	
	// copy Finder info
	err = FSpGetFInfo(srcSpec, &fInfo);
	require_noerr(err, CantGetFInfo);
	
	fInfo.fdLocation.h = fInfo.fdLocation.v = 0;
	fInfo.fdFldr = 0;
	fInfo.fdFlags &= ~kHasBeenInited;
	err = FSpSetFInfo(&newSpec, &fInfo);
	require_noerr(err, CantSetFInfo);
	
	// we're done with the buffer; dispose it if we allocated it here	
	if (copyBufferPtr == NULL)
	{
		DisposePtr((Ptr) pBuff);
	}
	return noErr;

// reverse-order cleanup

CantSetFInfo:
CantGetFInfo:
CantCopyResourceFork:
CantOpenDestRF:
CantOpenSourceRF:
CantCopyDataFork:
	if (destRefNum != -1) FSClose(destRefNum);	
CantOpenDestDF:
	if (srcRefNum != -1) FSClose(srcRefNum);
CantOpenSourceDF:
	FSpDelete(&newSpec);
CantMakeNewFile:
FileExistsAlready:
CantGetDirID:
	if (pBuff && !copyBufferPtr)
	{
		DisposePtr((Ptr) pBuff);
	}
CantGetBuffer:
	return err;
	
}

#endif
