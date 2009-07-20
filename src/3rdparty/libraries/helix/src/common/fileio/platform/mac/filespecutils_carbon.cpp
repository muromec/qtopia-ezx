/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecutils_carbon.cpp,v 1.11 2007/07/06 20:35:13 jfinnecy Exp $
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

#include "filespec.h"
#include "filespecutils.h"

#include "hxstring.h"
#include "hxtick.h"
#include "hxrand.h"
#include "carray.h"

#include "platform/mac/MoreFilesX.h"

// ------------------------------------------------------------------------------------
//
// GetFreeSpaceOnDisk
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::GetFreeSpaceOnDisk(const CHXDirSpecifier& volSpec, INT64& freeSpace)
{

	HX_RESULT		err;
	FSRef			volRef;
	FSVolumeRefNum		vRefNum;
	UInt64			freeBytes;
	UInt64			totalBytes;
	
	require_return(volSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	volRef = (FSRef) volSpec;
	
	err = FSGetVRefNum(&volRef, &vRefNum);
	if (err == noErr)
	{
		HFSUniStr255* kDontCareVolName = NULL;
		
		err = FSGetVInfo(vRefNum, kDontCareVolName, &freeBytes, &totalBytes);
		if (err == noErr)
		{
			freeSpace = *(long long *) &freeBytes;
		}
	}
	return err;
}


// ------------------------------------------------------------------------------------
//
// GetTotalSpaceOnDisk
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::GetTotalSpaceOnDisk(const CHXDirSpecifier& volSpec, INT64& totalSpace)
{
	HX_RESULT		err;
	FSRef			volRef;
	FSVolumeRefNum		vRefNum;
	UInt64			freeBytes;
	UInt64			totalBytes;
	
	require_return(volSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	volRef = (FSRef) volSpec;
	
	err = FSGetVRefNum(&volRef, &vRefNum);
	if (err == noErr)
	{
		HFSUniStr255* kDontCareVolName = NULL;
		
		err = FSGetVInfo(vRefNum, kDontCareVolName, &freeBytes, &totalBytes);
		if (err == noErr)
		{
			totalSpace = *(long long *) &totalBytes;
		}
	}
	return err;

}


// ------------------------------------------------------------------------------------
//
// IsFileLocal
// IsDirectoryLocal
//
// ------------------------------------------------------------------------------------

HXBOOL CHXFileSpecUtils::IsDiskLocal(const CHXDirSpecifier& volSpec)
{
	HParamBlockRec		pb;
	HX_RESULT		err;
	GetVolParmsInfoBuffer	buff;
	HXBOOL			bLocalVol;
	short			vRefNum;
	
	require_return(volSpec.IsSet(), FALSE);

	FSRef volRef;
	
	volRef = (FSRef) volSpec;
	
	err = FSGetVRefNum(&volRef, &vRefNum);
	if (err != noErr)
	{
		return TRUE;
	}
	
	ZeroInit(&pb);
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioBuffer = (Ptr) &buff;
	pb.ioParam.ioReqCount = sizeof(buff);
	
	err = PBHGetVolParmsSync(&pb);
	
	if (err == noErr)
	{
		bLocalVol = (buff.vMServerAdr == 0);
	}
	else
	{
		// error occurred, default to true
		bLocalVol = TRUE;
	}
	
	return bLocalVol;
}



// ------------------------------------------------------------------------------------
//
// IsDiskEjectable
//
// ------------------------------------------------------------------------------------

HXBOOL CHXFileSpecUtils::IsDiskEjectable(const CHXDirSpecifier& volSpec)
{
	OSErr			err;
	short			vRefNum;
	GetVolParmsInfoBuffer 	volParmsInfo;
	HParamBlockRec		pb;
	FSRef 			volRef;

	require_return(volSpec.IsSet(), FALSE);
	
	volRef = (FSRef) volSpec;
	
	err = FSGetVRefNum(&volRef, &vRefNum);
	require_noerr_return(err, FALSE);
	
	ZeroInit(&volParmsInfo);
	ZeroInit(&pb);
	
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioBuffer = (Ptr) &volParmsInfo;
	pb.ioParam.ioReqCount = sizeof(volParmsInfo);
	err = PBHGetVolParmsSync(&pb);
	if (err == noErr)
	{
		// we require version 3 of the info buffer to rely on the vMExtendedAttributes
		if (volParmsInfo.vMVersion >= 3)
		{
			if (volParmsInfo.vMExtendedAttributes & (1L << bIsEjectable))
			{
				return TRUE;
			}
		}
	}
	
	return FALSE;

}

// ------------------------------------------------------------------------------------
//
// IsDiskWritable
//
// ------------------------------------------------------------------------------------

HXBOOL CHXFileSpecUtils::IsDiskWritable(const CHXDirSpecifier& volSpec)
{
	FSVolumeInfo	volumeInfo;
	HXBOOL		bWritable;
	OSErr err;
	FSRef volRef;
	FSVolumeRefNum vRefNum;
	
	require_return(volSpec.IsSet(), FALSE);
	
	bWritable = TRUE;

	volRef = (FSRef) volSpec;
	
	err = FSGetVRefNum(&volRef, &vRefNum);
	check_noerr(err);
	
	if (err == noErr)
	{
		err = FSGetVolumeInfo(vRefNum, 0, NULL, kFSVolInfoFlags, &volumeInfo, NULL, NULL);
		if (err == noErr)
		{
			if ( 0 != (volumeInfo.flags & kFSVolFlagHardwareLockedMask) )
			{
				bWritable = FALSE;
			}
			else if ( 0 != (volumeInfo.flags & kFSVolFlagSoftwareLockedMask) )
			{
				bWritable = FALSE;
			}
		}
	}
	
	return bWritable;
	
}


// ------------------------------------------------------------------------------------
//
// GetFileSize
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::GetFileSize(const CHXFileSpecifier& fileSpec, INT64& fSize, IUnknown* pContext)
{
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);

	HX_RESULT	err;
	FSRef		targetRef;
	UInt64		logicalSize, physicalSize;
	ItemCount	numForks;
	
	targetRef = fileSpec;

	err = FSGetTotalForkSizes(&targetRef, &logicalSize, &physicalSize, &numForks);
	if (err == noErr)
	{
		fSize = (INT64) logicalSize;
	}
	
	return err;
}

// ------------------------------------------------------------------------------------
//
// GetDirectorySize
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::GetDirectorySize(const CHXDirSpecifier& dirSpec, HXBOOL shouldDescend, INT64& fSize)
{
	// TODO: revise for Carbon
	
	// rather than literally recurse through the directory tree, this routine
	// will keep an array of directories yet to be added to the total size
	
	require_return(dirSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	CHXPtrArray specArray;
	CHXDirSpecifier *pCurrDirSpec;
	INT64 totalSize;
	
	// push a copy of the initial directory spec into the array; 
	// it will be deleted when it is popped off
	
	pCurrDirSpec = new CHXDirSpecifier(dirSpec);
	require_nonnull_return(pCurrDirSpec, HXR_OUTOFMEMORY);
	
	specArray.Add(pCurrDirSpec);
	
	totalSize = 0;
	
	while (!specArray.IsEmpty())
	{
		FSSpec targetFSSpec;
		OSErr err;
		CInfoPBRec pb;
		short vRefNum;
		long currentDirID, index;
		Str63 fileName;
		Boolean bDoneWithThisDirectory;
		
		// grab a dirSpec from the array, delete the object, 
		// and step through all items in the directory
		
		pCurrDirSpec = (CHXDirSpecifier *) specArray.ElementAt(0);
		check_nonnull(pCurrDirSpec);
		
		if (pCurrDirSpec)
		{
			targetFSSpec = *pCurrDirSpec;
			currentDirID = pCurrDirSpec->GetDirID();
			vRefNum = targetFSSpec.vRefNum;
			
			// remove this dirSpec from our array and delete the object
			specArray.RemoveAt(0);
			HX_DELETE(pCurrDirSpec);
			check(vRefNum != 0 && currentDirID != 0);
			
			if (vRefNum != 0 && currentDirID != 0)
			{
				// step through all items in this directory
				index = 0;
				
				bDoneWithThisDirectory = false;
				while (!bDoneWithThisDirectory)
				{
					index++;
					
					ZeroInit(&pb);
					fileName[0] = 0;
					pb.hFileInfo.ioVRefNum = vRefNum;
					pb.hFileInfo.ioDirID = currentDirID;
					pb.hFileInfo.ioNamePtr = fileName;
					pb.hFileInfo.ioFDirIndex = index;

					err = PBGetCatInfoSync(&pb);					
					if (err != noErr)
					{
						// no more items in this directory
						bDoneWithThisDirectory = true;
					}
					else
					{
						if ((pb.hFileInfo.ioFlAttrib & ioDirMask) == 0)
						{
							// it's a file; add its size
							totalSize += (INT64) pb.hFileInfo.ioFlLgLen;	// data fork
							totalSize += (INT64) pb.hFileInfo.ioFlRLgLen;	// resource fork
						}
						else
						{
							// it's a directory; add a dirSpec for it to the array
							if (shouldDescend)
							{
								CHXDirSpecifier *pNewDirSpec;

								err = FSMakeFSSpec(vRefNum, currentDirID, fileName, &targetFSSpec);
								check_noerr(err);
								
								pNewDirSpec = new CHXDirSpecifier(targetFSSpec);
								check_nonnull(pNewDirSpec);
								
								if (pNewDirSpec)
								{
									specArray.Add(pNewDirSpec);
								}
								
							}
						}
					}
				} // while (!bDoneWithThisDirectory)
			}
		}
	} // while(!specArray.IsEmpty())
	
	fSize = totalSize;
	return HXR_OK;
}

// ------------------------------------------------------------------------------------
//
// GetCurrentApplication
// GetCurrentApplicationDir
//
// ------------------------------------------------------------------------------------

CHXFileSpecifier CHXFileSpecUtils::GetCurrentApplication()
{
	// TODO revise for Carbon, perhaps with GetProcessBundleLocation
	// if we're a bundle
	
	OSErr					err;
	FSSpec 					appFSSpec;
	ProcessSerialNumber 	appPSN = { 0, kCurrentProcess };
	ProcessInfoRec 			appPIR;
	CHXFileSpecifier 		appSpec;
	
	appPIR.processInfoLength = sizeof(ProcessInfoRec);
	appPIR.processAppSpec = &appFSSpec;
	appPIR.processName = NULL;
	
	err = GetProcessInformation(&appPSN, &appPIR);
	check_noerr(err);
	
	if (err == noErr)
	{
		appSpec = appFSSpec;
	}
	return appSpec;
}

CHXDirSpecifier CHXFileSpecUtils::GetCurrentApplicationDir()
{
	CHXFileSpecifier 	appSpec;
	CHXDirSpecifier 	dirSpec;
	
	appSpec = GetCurrentApplication();
	if (appSpec.IsSet())
	{
		dirSpec = appSpec.GetParentDirectory();
	}
	return dirSpec;
}

// ------------------------------------------------------------------------------------
//
// FileExists
// DirectoryExists
//
// ------------------------------------------------------------------------------------

HXBOOL CHXFileSpecUtils::FileExists(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
	FSRef 	fileRef;
	Boolean	isDirectory;
	long *	kDontWantNodeID = NULL;
	
	if (!fileSpec.IsSet())
	{
		return FALSE;
	}
	fileRef = (FSRef) fileSpec;
	
 	return (FSGetNodeID(&fileRef, kDontWantNodeID, &isDirectory) == noErr) && !isDirectory;
}

HXBOOL CHXFileSpecUtils::DirectoryExists(const CHXDirSpecifier& dirSpec)
{
	FSRef 	dirRef;
	Boolean	isDirectory;
	long *	kDontWantNodeID = NULL;
	
	if (!dirSpec.IsSet())
	{
		return FALSE;
	}
	
	dirRef = (FSRef) dirSpec;
	
	return (FSGetNodeID(&dirRef, kDontWantNodeID, &isDirectory) == noErr) && isDirectory;
}

// ------------------------------------------------------------------------------------
//
// CreateDirectory
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::CreateDir(const CHXDirSpecifier& dirSpec)
{
	require_return(dirSpec.IsSet(), HXR_INVALID_PARAMETER);

	OSErr 		err;
	FSRef 		parentRef;
	CHXDirSpecifier parentSpec;
	HFSUniStr255 	hfsUni;
	UInt32		dirID;
	
	FSCatalogInfo*	kNotSettingCatInfo = NULL;
	FSRef*		kDontWantNewRef = NULL;
	FSSpec*		kDontWantFSSpec = NULL;
	
	err = HXR_INVALID_PARAMETER;
	
	// we need an FSRef for the parent where we will be creating the directory,
	// and an HFSUniStr255 of the name of the directory to be created
	
	parentSpec = dirSpec.GetParentDirectory();
	require(parentSpec.IsSet(), CantGetParent);
	
	parentRef = (FSRef) parentSpec;
	
	hfsUni = dirSpec.GetNameHFSUniStr255();
	require(hfsUni.length != 0, CantGetUnicodeName);
	
	err = FSCreateDirectoryUnicode(&parentRef, hfsUni.length, hfsUni.unicode, 
		kFSCatInfoNone, kNotSettingCatInfo, kDontWantNewRef, kDontWantFSSpec, &dirID);
	require_noerr(err, CantMakeDir);
	
	return HXR_OK;

CantMakeDir:
CantGetUnicodeName:
CantGetParent:
	return err;	  

}

// ------------------------------------------------------------------------------------
//
// RemoveDir - deletes an empty directory
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::RemoveDir(const CHXDirSpecifier& dirSpec)
{
	FSRef 	dirRef;
	OSErr	err;
	
	require_return(dirSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	dirRef = (FSRef) dirSpec;
	
	err = FSDeleteObject(&dirRef);
	
	return err;

}

// ------------------------------------------------------------------------------------
//
// RemoveFile
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::RemoveFile(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
	FSRef 	fileRef;
	OSErr	err;
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	fileRef = (FSRef) fileSpec;
	
	err = FSDeleteObject(&fileRef);
	
	return err;
}

// ------------------------------------------------------------------------------------
//
// MakeFileReadOnly/MakeFileNotReadOnly
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::MakeFileReadOnly(const CHXFileSpecifier& fileSpec)
{
	FSSpec 	fileFSSpec;
	OSErr	err;
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	fileFSSpec = (FSSpec) fileSpec;
	
	err = FSpSetFLock(&fileFSSpec); // is there an FSRef variant for this?
	
	if (err)
	{
		err = HXR_FAIL; // poor-man's OSErr-to-HX_RESULT conversion
	}
	
	return err;
}


HX_RESULT CHXFileSpecUtils::MakeFileNotReadOnly(const CHXFileSpecifier& fileSpec)
{
	FSSpec 	fileFSSpec;
	OSErr	err;
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	fileFSSpec = (FSSpec) fileSpec;
	
	err = FSpRstFLock(&fileFSSpec); // is there an FSRef variant for this?
	
	if (err)
	{
		err = HXR_FAIL; // poor-man's OSErr-to-HX_RESULT conversion
	}
	
	return err;
}

// ------------------------------------------------------------------------------------
//
// RenameMoveFile
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::RenameMoveFile(CHXFileSpecifier& fileSpec, const char* pNewNameIfAny, 
	const CHXDirSpecifier* pNewDirectoryIfAny)
{
	HX_RESULT		res;
	CHXDirSpecifier		fileDir, destDir;
	CHXFileSpecifier	interimFileSpec;
	CHXString		strNewName;
	FSRef			fileRef, interimRef, targetRef;
	HFSUniStr255 		newUniName;
	HXBOOL			bRenaming, bMoving, bRenamed, bMoved;
	
	res = HXR_FAIL;
	bRenamed = bMoved = FALSE;
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	fileDir = fileSpec.GetParentDirectory();
	
	if (pNewDirectoryIfAny)
	{
		destDir = *pNewDirectoryIfAny;
		bMoving = (destDir != fileDir) ? TRUE : FALSE;
	}
	else
	{
		destDir = fileDir;
		bMoving = FALSE;
	}
	
	if (pNewNameIfAny)
	{
		strNewName = pNewNameIfAny;
		bRenaming = (strNewName != fileSpec.GetName()) ? TRUE : FALSE;
	}
	else
	{
		strNewName = fileSpec.GetName();
		bRenaming = FALSE;
	}
	
	// ensure there's not already something there with this name
	CHXFileSpecifier specTestDest = destDir.SpecifyChildFile(strNewName);
	require( (!CHXFileSpecUtils::FileExists(specTestDest)), Bail);
	
	// try renaming, then move, then rename if we couldn't originally
	fileRef = (FSRef) fileSpec;
	
	// rename
	interimFileSpec = fileDir.SpecifyChildFile(strNewName); // new name in old directory
	if (bRenaming && interimFileSpec.IsSet() && !CHXFileSpecUtils::FileExists(interimFileSpec))
	{
		newUniName = interimFileSpec.GetNameHFSUniStr255();
		res = FSRenameUnicode(&fileRef, newUniName.length, newUniName.unicode, 
			kTextEncodingUnknown, &interimRef);
		if (res == noErr)
		{
			bRenamed = TRUE;
			fileRef = interimRef;
		}
	}
	
	// move
	if (bMoving)
	{
		targetRef = (FSRef) destDir;
		res = FSMoveObject(&fileRef, &targetRef, &interimRef);
		if (res == noErr)
		{
			bMoved = TRUE;
			fileRef = interimRef;
		}
	}
	
	// rename -- don't try to rename if we were supposed to move & didn't
	if (bRenaming && !bRenamed && (bMoved || !bMoving))
	{
		interimFileSpec = destDir.SpecifyChildFile(strNewName); // new name in new directory
		
		newUniName = interimFileSpec.GetNameHFSUniStr255();
		res = FSRenameUnicode(&fileRef, newUniName.length, newUniName.unicode, 
			kTextEncodingUnknown, &interimRef);
		if (res == noErr)
		{
			bRenamed = TRUE;
			fileRef = interimRef;
		}
	}
	
	if ((bRenamed || !bRenaming) && (bMoved || !bMoving))
	{
		fileSpec = fileRef;
		res = HXR_OK;
	}
	else
	{
		res = HXR_FAIL;
	}
Bail:
	return res;
}

// ------------------------------------------------------------------------------------
//
// IsDiskAudioCD
//
// ------------------------------------------------------------------------------------

HXBOOL CHXFileSpecUtils::IsDiskAudioCD(const CHXDirSpecifier& volSpec)
{
    // I referred to Apple's AudioCDSample code to help determine whether
    // we're looking at an audio CD.
    // http://developer.apple.com/samplecode/AudioCDSample/listing1.html
    
    FSVolumeInfo        volumeInfo;
    HXBOOL                bIsAudioCD;
    OSErr err;
    FSRef volRef;
    FSVolumeRefNum vRefNum;

    require_return(volSpec.IsSet(), FALSE);

    bIsAudioCD = FALSE;

    volRef = (FSRef) volSpec;

    err = FSGetVRefNum(&volRef, &vRefNum);
    check_noerr(err);

    if (err == noErr)
    {
        err = FSGetVolumeInfo(vRefNum, 0, NULL, kFSVolInfoFSInfo, &volumeInfo, NULL, NULL);
        if (err == noErr)
        {
            if (volumeInfo.filesystemID == 0x4a48) // 'JH' means audio CD
            {
                bIsAudioCD = TRUE;
            }
        }
    }
    return bIsAudioCD;
}

// ------------------------------------------------------------------------------------
//
// read/write files
//
// ------------------------------------------------------------------------------------

static HX_RESULT WriteFileInternal(CHXFileSpecifier& fileSpec, const void *pBuff, UInt32 length, HXBOOL bReplaceExistingFile);
static HX_RESULT ReadFileInternal(const CHXFileSpecifier& fileSpec, IHXBuffer*& pOutBuffer);

HX_RESULT CHXFileSpecUtils::WriteBinaryFile(CHXFileSpecifier& fileSpec, IHXBuffer* inBuffer, HXBOOL bReplaceExistingFile)
{
	return WriteFileInternal(fileSpec, (void *) inBuffer->GetBuffer(), inBuffer->GetSize(), bReplaceExistingFile);
}

HX_RESULT CHXFileSpecUtils::WriteTextFile(CHXFileSpecifier& fileSpec, const CHXString& inStr, HXBOOL bReplaceExistingFile)
{
	return WriteFileInternal(fileSpec, (const char *) inStr, inStr.GetLength(), bReplaceExistingFile);
}

HX_RESULT WriteFileInternal(CHXFileSpecifier& fileSpec, const void *pBuff, UInt32 length, HXBOOL bReplaceExistingFile)
{
	check_nonnull(pBuff);
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	long * kDontWantNodeID = NULL;
	FSCatalogInfo * kDontSetCatInfo = NULL;
	FSSpec * kDontWantFileFSSpec = NULL;
	
	CHXDirSpecifier parentDirSpec;
	
	Boolean targetIsDir;
	Boolean targetAlreadyExists;
	HFSUniStr255 uniName, dataForkName;
	FSRef fileFSRef;
	SInt16 forkRefNum;
	OSErr err;
	
	// if an item is there already, bail if we're in dont replace mode
	// or if the item is a directory
	fileFSRef = (FSRef) fileSpec;
	
	targetAlreadyExists = (FSGetNodeID(&fileFSRef, kDontWantNodeID, &targetIsDir) == noErr);
	
	// before we delete an old file, save the parent and filename info so we can create the new one
	// (since the FSRef for the file in the old file spec becomes invalid once we delete the file)
	parentDirSpec = fileSpec.GetParentDirectory();
	uniName = fileSpec.GetNameHFSUniStr255();


	if (targetAlreadyExists)
	{
		if (targetIsDir) return HXR_INVALID_PATH;
		
		if (!bReplaceExistingFile) return HXR_FILE_EXISTS;
		
		if (bReplaceExistingFile) 
		{
			err = FSDeleteObject(&fileFSRef);
		}
	} 

	// create, open, write into the file, and close it
	err = FSCreateFileUnicode((FSRef *) parentDirSpec, uniName.length, uniName.unicode,
		kFSCatInfoNone, kDontSetCatInfo, &fileFSRef, kDontWantFileFSSpec);
	if (err == noErr)
	{
		dataForkName.length = 0;
		err = FSGetDataForkName(&dataForkName);
		check_noerr(err);
		
		err = FSOpenFork(&fileFSRef, dataForkName.length, dataForkName.unicode,
			fsWrPerm, &forkRefNum);
		if (err == noErr)
		{
			long ioBytes = length;
			
			err = FSWrite(forkRefNum, &ioBytes, pBuff);
			check_noerr(err);
			
			FSCloseFork(forkRefNum);
		}
		
		if (err == noErr)
		{
			// update the file spec to point to the new file
			fileSpec = fileFSRef;
		}
		else
		{
			// we failed to write it; delete it
			(void) FSDeleteObject(&fileFSRef);
		}
	}
	
	return (err == noErr ? HXR_OK : HXR_FAIL);
}


HX_RESULT CHXFileSpecUtils::ReadBinaryFile(const CHXFileSpecifier& fileSpec, IHXBuffer*& pOutBuffer)
{
	return ReadFileInternal(fileSpec, pOutBuffer);
}

HX_RESULT CHXFileSpecUtils::ReadTextFile(const CHXFileSpecifier& fileSpec, CHXString& outStr)
{
	HX_RESULT res;
	IHXBuffer *pBuff = NULL;
	
	res = ReadFileInternal(fileSpec, pBuff);
	
	if (SUCCEEDED(res))
	{
		outStr = CHXString((const char *) pBuff->GetBuffer(), (int) pBuff->GetSize());
		
		HX_RELEASE(pBuff);
	}
	
	return res;
}

HX_RESULT ReadFileInternal(const CHXFileSpecifier& fileSpec, IHXBuffer*& pOutBuffer)
{
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);

	HFSUniStr255 dataForkName;
	FSRef fileFSRef;
	SInt16 forkRefNum = -1;
	SInt64 forkSize;
	OSStatus err;
	
	pOutBuffer = new CHXBuffer();
	require_nonnull_action(pOutBuffer, Bail, err = HXR_OUTOFMEMORY);
	pOutBuffer->AddRef();
	
	dataForkName.length = 0;
	err = FSGetDataForkName(&dataForkName);
	require_noerr(err, Bail);
	
	fileFSRef = (FSRef) fileSpec;
	
	err = FSOpenFork(&fileFSRef, dataForkName.length, dataForkName.unicode,
		fsRdPerm, &forkRefNum);
	require_noerr_quiet(err, Bail);
		
	err = FSGetForkSize(forkRefNum, &forkSize);
	require_noerr(err, Bail);
	
	require_action(forkSize <= 0x07FFFFFFF, Bail, err = HXR_INVALID_PATH); // file is too big for a 32-bit signed length
	
	err = pOutBuffer->SetSize((UInt32) forkSize);
	require_noerr(err, Bail);
	
	{
            long ioBytes = forkSize;
            
            err = FSRead(forkRefNum, &ioBytes, pOutBuffer->GetBuffer());
            require_noerr(err, Bail);
                    
            (void) FSCloseFork(forkRefNum);
            
            return HXR_OK;
        }
	
Bail:
	HX_RELEASE(pOutBuffer);
	if (forkRefNum != -1) (void) FSCloseFork(forkRefNum);
	
	return HXR_FAIL;
}

// ------------------------------------------------------------------------------------
//
// GetFileType
//
// ------------------------------------------------------------------------------------

FOURCC CHXFileSpecUtils::GetFileType(const CHXFileSpecifier& fileSpec)
{
	FOURCC fileType;
	OSErr err;
	
	require_return(fileSpec.IsSet(), 0);
	
	fileType = 0;
	
	FSRef		fileRef;
	FinderInfo	fndrInfo;
	Boolean		isDir;
	
	ExtendedFinderInfo * kDontWantExtendedInfo = NULL;
	
	fileRef = (FSRef) fileSpec;
	
	err = FSGetFinderInfo(&fileRef, &fndrInfo, kDontWantExtendedInfo, &isDir);
	
	if ((err == noErr) && !isDir)
	{
		fileType = fndrInfo.file.fileType;
	}
	
	return fileType;
}

// ------------------------------------------------------------------------------------
//
// MakeNameLegal
//
// returns TRUE if the name was changed
//
// ------------------------------------------------------------------------------------

HXBOOL CHXFileSpecUtils::MakeNameLegal(char *pszName)
{
	const char *badChars = ":";
	const char replacementChar = '-';
	const long maxNameLength = 255;

	long len, idx;
	HXBOOL bChanged;
	
	require_nonnull_return(pszName, FALSE);
	
	bChanged = FALSE;
	
	len = strlen(pszName);
	
	// replace any illegal characters
	for (idx = 0; idx < len; idx++)
	{
		if (strchr(badChars, pszName[idx]))
		{
			pszName[idx] = replacementChar;
			bChanged = TRUE;
		}
	}
	
	// be sure the name isn't too long
	if (len > maxNameLength)
	{
		pszName[maxNameLength] = 0;
		bChanged = TRUE;
	}
	
	return bChanged;
}


// ------------------------------------------------------------------------------------
//
// FindFolder
//
// ------------------------------------------------------------------------------------

CHXDirSpecifier CHXFileSpecUtils::MacFindFolder(short vRefNum, FolderType foldType)
{
	CHXDirSpecifier			foundDirSpec;
	OSErr				err;
	FSRef				folderRef;
	
	err = ::FSFindFolder(vRefNum, foldType, kCreateFolder, &folderRef);
	if (err == noErr)
	{
		foundDirSpec = folderRef;
	}
	
	return foundDirSpec;
}

CHXFileSpecifier CHXFileSpecUtils::SpecifyFileWithMacFindFolder(short vRefNum, FolderType foldType, const char *pszChildFile)
{
	CHXDirSpecifier parentDir;
	CHXFileSpecifier targetFile;
	
	parentDir = CHXFileSpecUtils::MacFindFolder(vRefNum, foldType);
	check(parentDir.IsSet());
	
	if (CHXFileSpecUtils::DirectoryExists(parentDir))
	{
		targetFile = parentDir.SpecifyChildFile(pszChildFile);
	}
	
	return targetFile;
}

CHXDirSpecifier CHXFileSpecUtils::SpecifyFolderWithMacFindFolder(short vRefNum, FolderType foldType, const char *pszChildFolder)
{
	CHXDirSpecifier parentDir;
	CHXDirSpecifier targetDir;
	
	parentDir = CHXFileSpecUtils::MacFindFolder(vRefNum, foldType);
	check(parentDir.IsSet());
	
	if (CHXFileSpecUtils::DirectoryExists(parentDir))
	{
		targetDir = parentDir.SpecifyChildDirectory(pszChildFolder);
	}
	
	return targetDir;
}

// ------------------------------------------------------------------------------------
//
// ResolveFileSpecifierAlias
// ResolveDirSpecifierAlias
//
// These resolve a file spec to an alias file in place
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::ResolveFileSpecifierAlias(CHXFileSpecifier& fileSpec)
{
	HX_RESULT res;
	
	res = HXR_FAIL;
	
	// GR 7/8/02
	// check that the file exists before resolving it in hopes of avoiding
	// a mysterious crash on 10.1.5
	if (CHXFileSpecUtils::FileExists(fileSpec))
	{
		FSRef	fileRef;
		OSErr	err;
		Boolean bIsFolder;
		Boolean bWasAliased;
		
		const Boolean kShouldResolveChains = true;
		
		fileRef = (FSRef) fileSpec;
		
		err = FSResolveAliasFileWithMountFlags(&fileRef, kShouldResolveChains,
			&bIsFolder, &bWasAliased, kResolveAliasFileNoUI);
		
		if ((err == noErr) && !bIsFolder)
		{
			res = HXR_OK;
			
			if (bWasAliased)
			{
				fileSpec = fileRef;
			}
		}
		else
		{
			// error occurred
		}
	}
	
	return res;

}

HX_RESULT CHXFileSpecUtils::ResolveDirSpecifierAlias(CHXDirSpecifier& dirSpec)
{
	HX_RESULT res;
	
	res = HXR_FAIL;
	
	if (dirSpec.IsSet())
	{
		FSRef	dirRef;
		OSErr	err;
		Boolean bIsFolder;
		Boolean bWasAliased;
		
		const Boolean kShouldResolveChains = true;
		
		dirRef = dirSpec;
		
		err = FSResolveAliasFileWithMountFlags(&dirRef, kShouldResolveChains,
			&bIsFolder, &bWasAliased, kResolveAliasFileNoUI);
		
		if ((err == noErr) && bIsFolder)
		{
			res = HXR_OK;
			
			if (bWasAliased)
			{
				dirSpec = dirRef;
			}
		}
		else
		{
			// error occurred
		}
	}
	
	return res;

}

// ------------------------------------------------------------------------------------
//
// MoveFileToTrash
// MoveFolderToTrash
// MoveFileToFolderWithRenaming
// MoveFolderToFolderWithRenaming
//
// ------------------------------------------------------------------------------------

static HX_RESULT MoveToFolderWithRenamingInternal(const FSRef* itemRef, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove, FSRef& outNewItemRef);

HX_RESULT CHXFileSpecUtils::MoveFileToTrash(const CHXFileSpecifier& fileSpec) 
{
	if (!CHXFileSpecUtils::FileExists(fileSpec)) return HXR_INVALID_PARAMETER;

	CHXDirSpecifier trashSpec;
	FSRef itemRef, newItemRef;
	
	const HXBOOL bDeleteIfCantMove = TRUE;

	itemRef = (FSRef) fileSpec;
	
	trashSpec = CHXFileSpecUtils::MacFindFolder(fileSpec.GetVRefNum(), kTrashFolderType);
	
	return MoveToFolderWithRenamingInternal(&itemRef, trashSpec, bDeleteIfCantMove, newItemRef);
}

HX_RESULT CHXFileSpecUtils::MoveFolderToTrash(const CHXDirSpecifier& dirSpec) 
{
	if (!CHXFileSpecUtils::DirectoryExists(dirSpec)) return HXR_INVALID_PARAMETER;

	CHXDirSpecifier trashSpec;
	FSRef itemRef, newItemRef;
	
	const HXBOOL bDeleteIfCantMove = TRUE;
	
	itemRef = (FSRef) dirSpec;
	
	trashSpec = CHXFileSpecUtils::MacFindFolder(dirSpec.GetVRefNum(), kTrashFolderType);
	
	return MoveToFolderWithRenamingInternal(&itemRef, trashSpec, bDeleteIfCantMove, newItemRef);
}

HX_RESULT CHXFileSpecUtils::MoveFileToCleanupAtStartup(const CHXFileSpecifier& fileSpec, HXBOOL bDeleteIfCantMove) 
{
	CHXFileSpecifier nonConstFileSpec(fileSpec);
	
	return MoveFileToCleanupAtStartup(nonConstFileSpec, bDeleteIfCantMove);
}

HX_RESULT CHXFileSpecUtils::MoveFileToCleanupAtStartup(CHXFileSpecifier& fileSpec, HXBOOL bDeleteIfCantMove) 
{
	if (!CHXFileSpecUtils::FileExists(fileSpec)) return HXR_INVALID_PARAMETER;

	CHXDirSpecifier cleanupSpec;
	FSRef itemRef, newItemRef;
	HX_RESULT res;
	
	itemRef = (FSRef) fileSpec;
	
	cleanupSpec = CHXFileSpecUtils::MacFindFolder(fileSpec.GetVRefNum(), kChewableItemsFolderType);
	
	res = MoveToFolderWithRenamingInternal(&itemRef, cleanupSpec, bDeleteIfCantMove, newItemRef);
	if (SUCCEEDED(res))
	{
		fileSpec = newItemRef;
	}
	
	return res;
}

HX_RESULT CHXFileSpecUtils::MoveFolderToCleanupAtStartup(const CHXDirSpecifier& dirSpec, HXBOOL bDeleteIfCantMove) 
{
	CHXDirSpecifier nonConstDirSpec(dirSpec);
	
	return MoveFolderToCleanupAtStartup(nonConstDirSpec, bDeleteIfCantMove);
}

HX_RESULT CHXFileSpecUtils::MoveFolderToCleanupAtStartup(CHXDirSpecifier& dirSpec, HXBOOL bDeleteIfCantMove) 
{
	if (!CHXFileSpecUtils::DirectoryExists(dirSpec)) return HXR_INVALID_PARAMETER;

	CHXDirSpecifier cleanupSpec;
	FSRef itemRef, newItemRef;
	HX_RESULT res;
	
	itemRef = (FSRef) dirSpec;
	
	cleanupSpec = CHXFileSpecUtils::MacFindFolder(dirSpec.GetVRefNum(), kChewableItemsFolderType);
	
	res = MoveToFolderWithRenamingInternal(&itemRef, cleanupSpec, bDeleteIfCantMove, newItemRef);
	
	if (SUCCEEDED(res))
	{
		dirSpec = newItemRef;
	}
	return res;
}


HX_RESULT CHXFileSpecUtils::MoveFileToFolderWithRenaming(CHXFileSpecifier& fileSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove) 
{
	if (!CHXFileSpecUtils::FileExists(fileSpec)) return HXR_INVALID_PARAMETER;
	if (!CHXFileSpecUtils::DirectoryExists(targetSpec)) return HXR_INVALID_PARAMETER;

	FSRef itemRef, newItemRef;
	HX_RESULT res;
	
	itemRef = (FSRef) fileSpec;
	
	res = MoveToFolderWithRenamingInternal(&itemRef, targetSpec, bDeleteIfCantMove, newItemRef);

	if (SUCCEEDED(res))
	{
		fileSpec = newItemRef;
	}
	return res;
}

HX_RESULT CHXFileSpecUtils::MoveFolderToFolderWithRenaming(CHXDirSpecifier& dirSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove) 
{
	if (!CHXFileSpecUtils::DirectoryExists(dirSpec)) return HXR_INVALID_PARAMETER;
	if (!CHXFileSpecUtils::DirectoryExists(targetSpec)) return HXR_INVALID_PARAMETER;

	FSRef itemRef, newItemRef;
	HX_RESULT res;
	
	itemRef = (FSRef) dirSpec;
	
	res = MoveToFolderWithRenamingInternal(&itemRef, targetSpec, bDeleteIfCantMove, newItemRef);
	if (SUCCEEDED(res))
	{
		dirSpec = newItemRef;
	}
	return res;
}

static HX_RESULT MoveToFolderWithRenamingInternal(const FSRef* itemRef, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove, FSRef& outNewItemRef) 
{	
	check_nonnull(itemRef);
	
	HX_RESULT	pnres;

	OSErr err;
	
	pnres = HXR_FAILED;
	
	ZeroInit(&outNewItemRef);
	
	if (!targetSpec.IsSet())
	{
		// targetSpec wasn't set, nowhere to move to
		
		if (bDeleteIfCantMove)
		{
			err = FSDeleteObject(itemRef);
			if (err == noErr)
			{
				pnres = HXR_OK;
			}
		}
	}
	else
	{
		// targetSpec is set
		//
		// try to move the file to the target
		FSRef newItemRef, targetRef, newNameRef;
		
		targetRef = (FSRef) targetSpec;
		
		err = FSMoveObject(itemRef, &targetRef, &newItemRef);
		if (err == noErr)
		{
			outNewItemRef = newItemRef;
			
			pnres = HXR_OK;
		}
		else if (err != dupFNErr)
		{
			// catmove failed for some unknown reason, not a name conflict
			if (bDeleteIfCantMove)
			{
				err = FSDeleteObject(itemRef);
				if (err == noErr)
				{
					pnres = HXR_OK;
				}
			}
		}
		else
		{
			// there's a name conflict; find a unique name for the file we're moving in the
			// target folder, rename it, and then move it again
			
			CHXString strName, strTemplate;
			CHXFileSpecifier specNewNamedFile;
			HFSUniStr255 uniName, newUniName;
			INT32 periodOffset;
			const char* const kWildcard = "%-%-%";
			
			err = FSGetFileDirName(itemRef, &uniName);
			
			strName.SetFromHFSUniStr255(uniName, CFStringGetSystemEncoding()); // make a template like "filename_%-%-%"
			
			periodOffset = strName.ReverseFind('.');
			if (periodOffset != -1)
			{
				// put the _2 or whatever just before the extension
				strTemplate = strName.Left(periodOffset);
				strTemplate += "_";
				strTemplate += kWildcard;
				strTemplate += strName.Mid(periodOffset);
			}
			else
			{
				// put the _2 or whatever after the filename
				strTemplate = strName + "_";
				strTemplate += kWildcard;
			}
			
			specNewNamedFile = CHXFileSpecUtils::GetUniqueFileSpec(targetSpec, strName, strTemplate, kWildcard);
			
			newUniName = specNewNamedFile.GetNameHFSUniStr255();
			
			err = FSRenameUnicode(itemRef, newUniName.length, newUniName.unicode, kTextEncodingUnknown, &newNameRef);
			if (err == noErr)
			{
				err = FSMoveObject(&newNameRef, &targetRef, &newItemRef);
				
				if (err == noErr)
				{
					outNewItemRef = newItemRef;
					
					pnres = HXR_OK;
				}
				else if (err != noErr && bDeleteIfCantMove)
				{
					// couldn't move it; delete the renamed file
					err = FSDeleteObject(&newNameRef);
					if (err == noErr)
					{
						pnres = HXR_OK;
					}
				}
				else
				{
					// couldn't move it; change the name back
					
					err = FSRenameUnicode(&newNameRef, uniName.length, uniName.unicode, 
						kTextEncodingUnknown, &newNameRef);
				}
			}
			else
			{
				// rename failed for some reason
				if (bDeleteIfCantMove)
				{
					// couldn't move it; delete the renamed file
					err = FSDeleteObject(itemRef);
					if (err == noErr)
					{
						pnres = HXR_OK;
					}
				}
			}			
		} 
	}

	return pnres;
}

// ------------------------------------------------------------------------------------
//
// GetSystemTempDirectory
//
// ------------------------------------------------------------------------------------

CHXDirSpecifier CHXFileSpecUtils::GetSystemTempDirectory()
{
	return CHXFileSpecUtils::MacFindFolder(kOnAppropriateDisk, kChewableItemsFolderType);
}

// ------------------------------------------------------------------------------------
//
// GetUniqueFileSpec
// GetUniqueTempFileSpec
//
// ------------------------------------------------------------------------------------

static CHXFileSpecifier GetUniqueFileSpecInternal(const CHXDirSpecifier& locationSpec, 
									const char *pszNameFirst, const char *pszTemplate, 
									const char *pszWildcard, UINT32 nStartNum);
									
const UINT32 kNumWrapValue = 9999+1;	// limit insertions to 4-digit numbers
	
CHXFileSpecifier CHXFileSpecUtils::GetUniqueFileSpec(const CHXDirSpecifier& locationSpec, 
									const char *pszNameFirst, const char *pszTemplate, 
									const char *pszWildcard)
{
	return GetUniqueFileSpecInternal(locationSpec, pszNameFirst, pszTemplate, pszWildcard, 0);
}

CHXFileSpecifier CHXFileSpecUtils::GetUniqueTempFileSpec(const CHXDirSpecifier& locationSpec, 
									const char *pszTemplate, const char *pszWildcard)
{
	CMultiplePrimeRandom 	rand(HX_GET_TICKCOUNT());
	
	UINT32 num;
	
	num = rand.GetRandomNumber();
	
	num %= kNumWrapValue;
	
	// skip 0, which means "don't substitute for the wildcard", and 1
	if (num == 0 || num == 1) num = 2;

	return GetUniqueFileSpecInternal(locationSpec, NULL, pszTemplate, pszWildcard, num);
}


static CHXFileSpecifier GetUniqueFileSpecInternal(const CHXDirSpecifier& locationSpec, 
									const char *pszNameFirst, const char *pszTemplate, 
									const char *pszWildcard, UINT32 nStartNum)
{
	CHXFileSpecifier 		resultFileSpec;
	
	require_return(locationSpec.IsSet(), resultFileSpec);
	require_return(pszTemplate != NULL && pszWildcard != NULL, resultFileSpec);
	require_return(pszNameFirst != NULL || nStartNum != 0, resultFileSpec);
	
	CHXFileSpecifier 	testFileSpec;
	CHXDirSpecifier 	testDirSpec;
	CHXString			strNumber;
	CHXString			strName;
	UINT32				nCurrentNum;
	
	nCurrentNum = nStartNum;

	while (1) 
	{
		// if the number is non-zero, make a string from the template;
		// if the number is zero, user the initial name string
		if (nCurrentNum == 0)
		{
			// replace the wildcard in the template with the number string
			strName = pszNameFirst;
		}
		else
		{
			// replace the wildcard in the template with the number string
			strNumber.Empty();
			strNumber.AppendULONG(nCurrentNum);

			strName = pszTemplate;
			strName.FindAndReplace(pszWildcard, strNumber);	// replace first wildcard with number string
		}
		
		
		// test if a file or directory exists with that name
		testFileSpec = locationSpec.SpecifyChildFile(strName);
		testDirSpec = locationSpec.SpecifyChildDirectory(strName);
		if (CHXFileSpecUtils::FileExists(testFileSpec)
			|| CHXFileSpecUtils::DirectoryExists(testDirSpec))
		{
			// an item already has that name, so increment & wrap the number
			nCurrentNum++;
			nCurrentNum %= kNumWrapValue;
			
			// don't use 0 again, and skip 1 since "MyFile2.txt" logically follows "MyFile.txt"
			if (nCurrentNum == 0 || nCurrentNum == 1) 
			{
				nCurrentNum = 2; 
			}
			
			// a quick sanity check
			if (nCurrentNum == nStartNum)
			{
				check(!"GetUniqueFileSpecInternal number wrapped");
				break;
			}
		}
		else
		{
			// the name is unique
			resultFileSpec = testFileSpec;
			break;
		}
		
	} // while

	return resultFileSpec;
}

CHXDirSpecifier 
CHXFileSpecUtils::GetAppDataDir(const char* szAppName)
{	
	// we'll use a com.RealNetworks.data folder in user's prefs
	CHXDirSpecifier targetDir;
	CHXDirSpecifier prefsDir = CHXFileSpecUtils::MacFindFolder(kUserDomain, kPreferencesFolderType);
	CHXDirSpecifier dataDir = prefsDir.SpecifyChildDirectory( "com.RealNetworks.data" );
	if (!CHXFileSpecUtils::DirectoryExists(dataDir))
	{
	    CHXFileSpecUtils::CreateDir(dataDir);
	}

	if (szAppName == NULL || *szAppName == 0)
	{
	    targetDir = dataDir;
	}
	else
	{
	    targetDir = dataDir.SpecifyChildDirectory(szAppName);

	    if (!CHXFileSpecUtils::DirectoryExists(targetDir))
	    {
		CHXFileSpecUtils::CreateDir(targetDir);
	    }
	}
	HX_ASSERT(CHXFileSpecUtils::DirectoryExists(targetDir));
	
	return targetDir;
}

#ifdef _DEBUG
void CHXFileSpecUtils::TestMacFileSpecUtils()
{
	CHXString drive1Name = "Tenspot:";
	CHXString drive2Name = "Sandy:";
	CHXString driveEjectableLockedName = "Dev.CD Feb 01 TC Disk 1:";
	CHXString driveServerName = "grobbins:";
	
	CHXDirSpecifier dirServerSpec = drive2Name;

	dirServerSpec = "Dev.CD Feb 01 TC Disk 1:Tool Chest:fake";
	dirServerSpec = "Sandy:fake:";
	//dirServerSpec = "grobbins:Sites:";
	//dirServerSpec = "grobbins:Sites:fake";
	CHXDirSpecifier dirServer2Spec = driveServerName;


	CHXFileSpecifier file1Spec = "Tenspot:123456789a123456789b123456789c123456789d.png";
	CHXFileSpecifier file2Spec = "Tenspot:123456789a123456789b123456789c123456789d:LVd";
	CHXFileSpecifier fileFake1Spec = "Tenspot:123456789a123456789b123456789c123456789d:BOO";
	CHXDirSpecifier dirFake1Spec = "Tenspot:123456789a123456789b123456789c123456789d:BOO Folder";

	CHXFileSpecifier fromNameSpec = "RealPlayer.xSYM";
	CHXFileSpecifier fromPartialPath = ":RealPlayer.xSYM";
	CHXFileSpecifier fromBadName = "BOO";
	
	HX_RESULT res;
	INT64 spaceFree, spaceTotal, fileSize;
	HXBOOL bFlag;
	
	res = CHXFileSpecUtils::GetFreeSpaceOnDisk(drive1Name, spaceFree);
	res = CHXFileSpecUtils::GetTotalSpaceOnDisk(drive1Name, spaceTotal);
	check(spaceTotal > spaceFree);

	bFlag = CHXFileSpecUtils::IsDiskEjectable(drive1Name);
	check(!bFlag);
	bFlag = CHXFileSpecUtils::IsDiskEjectable(driveEjectableLockedName);
	check(bFlag);
	
	bFlag = CHXFileSpecUtils::IsDiskWritable(drive1Name);
	check(bFlag);
	bFlag = CHXFileSpecUtils::IsDiskWritable(driveEjectableLockedName);
	check(!bFlag);
	
	bFlag = CHXFileSpecUtils::IsDiskLocal(drive1Name);
	check(bFlag);
	//bFlag = CHXFileSpecUtils::IsDiskLocal(driveServerName);  this fails because HFS<-->FSRef fails on mounted vols
	//check(!bFlag);
	
	res = CHXFileSpecUtils::GetFileSize(file1Spec, fileSize);
	res = CHXFileSpecUtils::GetFileSize(file2Spec, fileSize);
	res = CHXFileSpecUtils::GetFileSize(fileFake1Spec, fileSize);
	
	res = CHXFileSpecUtils::FileExists(file1Spec);
	res = CHXFileSpecUtils::FileExists(file2Spec);
	res = CHXFileSpecUtils::FileExists(fileFake1Spec);
	
	bFlag = CHXFileSpecUtils::DirectoryExists(dirFake1Spec);
	check(!bFlag);
	res = CHXFileSpecUtils::CreateDir(dirFake1Spec);
	bFlag = CHXFileSpecUtils::DirectoryExists(dirFake1Spec);
	check(bFlag);
	res = CHXFileSpecUtils::RemoveDir(dirFake1Spec);
	bFlag = CHXFileSpecUtils::DirectoryExists(dirFake1Spec);
	check(!bFlag);
	
	CHXDirSpecifier ffDir = CHXFileSpecUtils::SpecifyFolderWithMacFindFolder(file1Spec.GetVRefNum(), kPreferencesFolderType, "Real Folder");
	res = CHXFileSpecUtils::CreateDir(ffDir);
	bFlag = CHXFileSpecUtils::DirectoryExists(ffDir);
	check(bFlag);
	res = CHXFileSpecUtils::MoveFolderToCleanupAtStartup(ffDir);
	bFlag = CHXFileSpecUtils::DirectoryExists(ffDir);
	check(!bFlag);
	
}
#endif
