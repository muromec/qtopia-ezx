/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecutils.cpp,v 1.6 2006/05/19 17:19:52 ping Exp $
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

#include "filespec.h"
#include "filespecutils.h"

#include "hxstring.h"
#include "hxtick.h"
#include "hxrand.h"
#include "carray.h"

#ifndef _CARBON
#include "morefilesextras.h"
#else
#include "MoreFilesX.h"
#endif

// ------------------------------------------------------------------------------------
//
// GetFreeSpaceOnDisk
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::GetFreeSpaceOnDisk(const CHXDirSpecifier& volSpec, INT64& freeSpace)
{
	HX_RESULT		err;
	FSSpec			targetSpec;
#ifdef _CARBON
	UInt64	freeBytes;
	UInt64	totalBytes;
#else
	short		foundVRefNum;
	UnsignedWide	freeBytes;
	UnsignedWide	totalBytes;
#endif
	
	require_return(volSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	targetSpec = volSpec;
	
	const StringPtr kDontCareVolName = NULL;
	
#ifndef _CARBON
	err = XGetVInfo(targetSpec.vRefNum, kDontCareVolName, &foundVRefNum, &freeBytes, &totalBytes);
#else
	err = FSGetVInfo(targetSpec.vRefNum, (HFSUniStr255*) kDontCareVolName, &freeBytes, &totalBytes);
#endif
	if (err == noErr)
	{
		freeSpace = *(long long *) &freeBytes;
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
	FSSpec			targetSpec;
#ifdef _CARBON
	UInt64	freeBytes;
	UInt64	totalBytes;
#else
	UnsignedWide	freeBytes;
	UnsignedWide	totalBytes;
	short		foundVRefNum;
#endif
	
	require_return(volSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	targetSpec = volSpec;
	
	const StringPtr kDontCareVolName = NULL;
	
#ifndef _CARBON
	err = XGetVInfo(targetSpec.vRefNum, kDontCareVolName, &foundVRefNum, &freeBytes, &totalBytes);
#else
	err = FSGetVInfo(targetSpec.vRefNum, (HFSUniStr255*) kDontCareVolName, &freeBytes, &totalBytes);
#endif
	if (err == noErr)
	{
		totalSpace = *(long long *) &totalBytes;
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
	HParamBlockRec			pb;
	HX_RESULT				err;
	FSSpec					targetSpec;
	GetVolParmsInfoBuffer	buff;
	HXBOOL					bLocalVol;
	
	require_return(volSpec.IsSet(), FALSE);
	
	targetSpec = volSpec;

	ZeroInit(&pb);
	pb.ioParam.ioVRefNum = targetSpec.vRefNum;
	pb.ioParam.ioBuffer = (Ptr) &buff;
	pb.ioParam.ioReqCount = sizeof(buff);
	
	err = PBHGetVolParmsSync(&pb);
	check_noerr(err);
	
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
#ifdef _CARBON
	
	OSErr			err;
	FSSpec			targetSpec;
	GetVolParmsInfoBuffer 	volParmsInfo;
	HParamBlockRec		pb;

	require_return(volSpec.IsSet(), FALSE);
	
	targetSpec = volSpec;
	
	ZeroInit(&volParmsInfo);
	ZeroInit(&pb);
	
	pb.ioParam.ioVRefNum = targetSpec.vRefNum;
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

#else
	HX_RESULT				err;
	FSSpec					targetSpec;
	Boolean					driverWantsEject, driveEjectable, volumeEjected, volumeOnline;
	
	require_return(volSpec.IsSet(), FALSE);
	
	targetSpec = volSpec;

	const StringPtr kDontCareVolName = NULL;

	err = GetVolState(kDontCareVolName, targetSpec.vRefNum, &volumeOnline, &volumeEjected,
							&driveEjectable, &driverWantsEject);
	check_noerr(err);
	if (err == noErr)
	{
		return driveEjectable;
	}
	else
	{
		return FALSE;
	}
#endif
}

// ------------------------------------------------------------------------------------
//
// IsDiskWritable
//
// ------------------------------------------------------------------------------------

HXBOOL CHXFileSpecUtils::IsDiskWritable(const CHXDirSpecifier& volSpec)
{
	OSErr	err;
	FSSpec	targetSpec;
	
	require_return(volSpec.IsSet(), FALSE);
	
	targetSpec = volSpec;
	
	HParamBlockRec hpb;
	
	ZeroInit(&hpb);
	hpb.volumeParam.ioVolIndex = 0;
	hpb.volumeParam.ioVRefNum = targetSpec.vRefNum;
	
	err = PBHGetVInfoSync(&hpb);
	check_noerr(err);
	
	if (err == noErr)
	{
		if ((hpb.volumeParam.ioVAtrb & 0x8080) != 0) // locked, software or hardware, per tech note FL 530
		{
			return FALSE; // not writable
		}
	}
	
	return TRUE;
}


// ------------------------------------------------------------------------------------
//
// GetFileSize
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::GetFileSize(const CHXFileSpecifier& fileSpec, INT64& fSize, IUnknown* pContext)
{
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);

#ifdef USE_FSREFS

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
#else

	HX_RESULT				err;
	FSSpec					targetSpec;
	
	targetSpec = fileSpec;

	HParamBlockRec pb;

	ZeroInit(&pb);
	pb.fileParam.ioVRefNum = targetSpec.vRefNum;
	pb.fileParam.ioDirID = targetSpec.parID;
	pb.fileParam.ioNamePtr = targetSpec.name;
	pb.fileParam.ioFDirIndex = 0;
	err = PBHGetFInfoSync(&pb);
	check_noerr(err);
	
	if ( err == noErr )
	{
		fSize = (INT64) pb.fileParam.ioFlLgLen;
		fSize += (INT64) pb.fileParam.ioFlRLgLen;
	}
	
	return err;
#endif

}

// ------------------------------------------------------------------------------------
//
// GetDirectorySize
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::GetDirectorySize(const CHXDirSpecifier& dirSpec, HXBOOL shouldDescend, INT64& fSize)
{
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

#ifndef USE_FSREFS

static HXBOOL FSSpecExists(FSSpecPtr itemSpec, HXBOOL *isDirectory);	// Forward declaration

HXBOOL CHXFileSpecUtils::FileExists(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
	FSSpec 	fileFSSpec;
	HXBOOL	isDirectory;
	
	require_return(fileSpec.IsSet(), FALSE);
	
	fileFSSpec = fileSpec;
	
	return FSSpecExists(&fileFSSpec, &isDirectory) && !isDirectory;
}

HXBOOL CHXFileSpecUtils::DirectoryExists(const CHXDirSpecifier& dirSpec)
{
	FSSpec 	dirFSSpec;
	HXBOOL	isDirectory;
	
	require_return(dirSpec.IsSet(), FALSE);
	
	dirFSSpec = dirSpec;
	
	return FSSpecExists(&dirFSSpec, &isDirectory) && isDirectory;
}

static HXBOOL FSSpecExists(FSSpecPtr itemSpec, HXBOOL *isDirectory)
{
	OSErr		err;
	CInfoPBRec	cInfo;
	FSSpec		tempSpec;
	HXBOOL		bExists;
	
	bExists = FALSE;
	
	// copy the provided file spec so PBGetCatInfo doesn't change the name
	tempSpec = *itemSpec;
	
	cInfo.dirInfo.ioVRefNum = tempSpec.vRefNum;
	cInfo.dirInfo.ioDrDirID = tempSpec.parID;
	cInfo.dirInfo.ioNamePtr = tempSpec.name;
	cInfo.dirInfo.ioFDirIndex = 0;	// use name, vRefNum, and dirID
	err = PBGetCatInfoSync(&cInfo);
	
	if (err == noErr)
	{
		bExists = TRUE;
		if (isDirectory)
		{
			if ((cInfo.hFileInfo.ioFlAttrib & ioDirMask) != 0)
			{
				*isDirectory = TRUE;
			}
			else
			{
				*isDirectory = FALSE;
			}
		}
	}
	
	return bExists;
}

#else // defined USE_FSREFS

static HXBOOL FSRefExists(const FSRef * itemSpec, HXBOOL *isDirectory);	// Forward declaration

HXBOOL CHXFileSpecUtils::FileExists(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
	FSRef 	fileRef;
	HXBOOL	isDirectory;
	
	require_return(fileSpec.IsSet(), FALSE);
	
	fileRef = fileSpec;
	
 	return FSRefExists(&fileRef, &isDirectory) && !isDirectory;
}

HXBOOL CHXFileSpecUtils::DirectoryExists(const CHXDirSpecifier& dirSpec)
{
	FSRef 	dirRef;
	HXBOOL	isDirectory;
	
	require_return(dirSpec.IsSet(), FALSE);
	
	dirRef = dirSpec;
	
	return FSRefExists(&dirRef, &isDirectory) && isDirectory;
}

static HXBOOL FSRefExists(const FSRef *itemRef, HXBOOL *isDirectory)
{
	OSErr		err;
	HXBOOL		bExists;
	FSCatalogInfo 	catInfo;
	
	const FSCatalogInfoBitmap whichInfo = kFSCatInfoNodeID | kFSCatInfoNodeFlags;

	HFSUniStr255 * kDontWantName = NULL;
	FSSpec * kDontWantFSSpec = NULL;
	FSRef * kDontWantParentRef = NULL;
	
	bExists = FALSE;
	
	err = ::FSGetCatalogInfo(itemRef, whichInfo, &catInfo,
		kDontWantName, kDontWantFSSpec, kDontWantParentRef);
		
	if (err == noErr)
	{
		bExists = TRUE;
		if (isDirectory)
		{
			if ((catInfo.nodeFlags & kFSNodeIsDirectoryMask) != 0)
			{
				*isDirectory = TRUE;
			}
			else
			{
				*isDirectory = FALSE;
			}
		}
	}
	
	return bExists;
}

#endif // defined USE_FSREFS

// ------------------------------------------------------------------------------------
//
// CreateDirectory
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::CreateDir(const CHXDirSpecifier& dirSpec)
{
	FSSpec 	dirFSSpec;
	long	dirID;
	OSErr	err;
	
	require_return(dirSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	dirFSSpec = dirSpec;
	
	err = FSpDirCreate(&dirFSSpec, smSystemScript, &dirID);
	
	return err;
}

// ------------------------------------------------------------------------------------
//
// RemoveDir - deletes an empty directory
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::RemoveDir(const CHXDirSpecifier& dirSpec)
{
	FSSpec 	dirFSSpec;
	OSErr	err;
	
	require_return(dirSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	dirFSSpec = dirSpec;
	
	err = FSpDelete(&dirFSSpec);
	
	return err;
}

// ------------------------------------------------------------------------------------
//
// RemoveFile
//
// ------------------------------------------------------------------------------------

HX_RESULT CHXFileSpecUtils::RemoveFile(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
	FSSpec 	fileFSSpec;
	OSErr	err;
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	fileFSSpec = fileSpec;
	
	err = FSpDelete(&fileFSSpec);
	
	return err;
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
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);
	
	fileType = 0;
	
	if (fileSpec.IsSet())
	{
#ifdef USE_FSREFS
		FSRef		fileRef;
		FinderInfo	fndrInfo;
		Boolean		isDir;
		
		ExtendedFinderInfo * kDontWantExtendedInfo = NULL;
		
		fileRef = fileSpec;
		
		err = FSGetFinderInfo(&fileRef, &fndrInfo, kDontWantExtendedInfo, &isDir);
		
		if ((err == noErr) && !isDir)
		{
			fileType = fndrInfo.file.fileType;
		}

#else
		FSSpec	fileFSSpec;
		FInfo	fndrInfo;
		
		fileFSSpec = fileSpec;
		
		err = FSpGetFInfo(&fileFSSpec, &fndrInfo);
		check_noerr(err);
		
		if (err == noErr)
		{
			fileType = fndrInfo.fdType;
		}
#endif
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
	const long maxNameLength = 31;

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
	short 				foundVRefNum;
	long 				foundDirID;
	FSSpec				targetSpec;
	CHXDirSpecifier		foundDirSpec;
	OSErr				err;
	
	err = ::FindFolder(vRefNum, foldType, kCreateFolder, &foundVRefNum, &foundDirID);
	if (err == noErr)
	{
		err = FSMakeFSSpec(foundVRefNum, foundDirID, "\p", &targetSpec);
		check_noerr(err);
		if (err == noErr)
		{
			foundDirSpec = targetSpec;
		}
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
	
	if (fileSpec.IsSet())
	{
		FSSpec	fileFSSpec;
		OSErr	err;
		Boolean bIsFolder;
		Boolean bWasAliased;
		
		const Boolean kShouldResolveChains = true;
		
		fileFSSpec = fileSpec;
		
		err = ResolveAliasFileWithMountFlags(&fileFSSpec, kShouldResolveChains,
			&bIsFolder, &bWasAliased, kResolveAliasFileNoUI);
		check(err == noErr);
		
		if ((err == noErr) && !bIsFolder)
		{
			res = HXR_OK;
			
			if (bWasAliased)
			{
				fileSpec = fileFSSpec;
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
		FSSpec	dirFSSpec;
		OSErr	err;
		Boolean bIsFolder;
		Boolean bWasAliased;
		
		const Boolean kShouldResolveChains = true;
		
		dirFSSpec = dirSpec;
		
		err = ResolveAliasFileWithMountFlags(&dirFSSpec, kShouldResolveChains,
			&bIsFolder, &bWasAliased, kResolveAliasFileNoUI);
		
		if ((err == noErr) && bIsFolder)
		{
			res = HXR_OK;
			
			if (bWasAliased)
			{
				dirSpec = dirFSSpec;
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

static HX_RESULT MoveToFolderWithRenamingInternal(const FSSpec* itemFSSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove);

HX_RESULT CHXFileSpecUtils::MoveFileToTrash(const CHXFileSpecifier& fileSpec) 
{
	if (!CHXFileSpecUtils::FileExists(fileSpec)) return HXR_INVALID_PARAMETER;

	CHXDirSpecifier trashSpec;
	FSSpec itemFSSpec;
	
	itemFSSpec = fileSpec;
	
	trashSpec = CHXFileSpecUtils::MacFindFolder(itemFSSpec.vRefNum, kTrashFolderType);
	
	return MoveToFolderWithRenamingInternal(&itemFSSpec, trashSpec, TRUE);
}

HX_RESULT CHXFileSpecUtils::MoveFolderToTrash(const CHXDirSpecifier& dirSpec) 
{
	if (!CHXFileSpecUtils::DirectoryExists(dirSpec)) return HXR_INVALID_PARAMETER;

	CHXDirSpecifier trashSpec;
	FSSpec itemFSSpec;
	
	itemFSSpec = dirSpec;
	
	trashSpec = CHXFileSpecUtils::MacFindFolder(itemFSSpec.vRefNum, kTrashFolderType);
	
	return MoveToFolderWithRenamingInternal(&itemFSSpec, trashSpec, TRUE);
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
	FSSpec itemFSSpec;
	
	itemFSSpec = fileSpec;
	
	cleanupSpec = CHXFileSpecUtils::MacFindFolder(itemFSSpec.vRefNum, kChewableItemsFolderType);
	
	return MoveToFolderWithRenamingInternal(&itemFSSpec, cleanupSpec, TRUE);
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
	FSSpec itemFSSpec;
	
	itemFSSpec = dirSpec;
	
	cleanupSpec = CHXFileSpecUtils::MacFindFolder(itemFSSpec.vRefNum, kChewableItemsFolderType);
	
	return MoveToFolderWithRenamingInternal(&itemFSSpec, cleanupSpec, TRUE);
}


HX_RESULT CHXFileSpecUtils::MoveFileToFolderWithRenaming(CHXFileSpecifier& fileSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove) 
{
	if (!CHXFileSpecUtils::FileExists(fileSpec)) return HXR_INVALID_PARAMETER;
	if (!CHXFileSpecUtils::DirectoryExists(targetSpec)) return HXR_INVALID_PARAMETER;

	FSSpec	fsSpec;
	
	fsSpec = fileSpec;
	return MoveToFolderWithRenamingInternal(&fsSpec, targetSpec, bDeleteIfCantMove);
}

HX_RESULT CHXFileSpecUtils::MoveFolderToFolderWithRenaming(CHXDirSpecifier& dirSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove) 
{
	if (!CHXFileSpecUtils::DirectoryExists(dirSpec)) return HXR_INVALID_PARAMETER;
	if (!CHXFileSpecUtils::DirectoryExists(targetSpec)) return HXR_INVALID_PARAMETER;

	FSSpec	fsSpec;
	
	fsSpec = dirSpec;
	return MoveToFolderWithRenamingInternal(&fsSpec, targetSpec, bDeleteIfCantMove);
}

static HX_RESULT MoveToFolderWithRenamingInternal(const FSSpec* itemFSSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove) 
{	
	check_nonnull(itemFSSpec);
	
	HX_RESULT	pnres;

	OSErr err;
	FSSpec targetFSSpec;
	
	pnres = HXR_FAILED;
	
	// find the trash for the disk containing the file
	if (!targetSpec.IsSet())
	{
		// targetSpec wasn't set, nowhere to move to
		
		if (bDeleteIfCantMove)
		{
			err = FSpDelete(itemFSSpec);
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
		
		targetFSSpec = targetSpec;
		err = CatMove(itemFSSpec->vRefNum, itemFSSpec->parID, itemFSSpec->name, targetFSSpec.parID, targetFSSpec.name);
		if (err == noErr)
		{
			pnres = HXR_OK;	
		}
		else if (err == dupFNErr)
		{
			// there's a name conflict; find a unique name for the file we're moving in the
			// target folder, rename it, and then move it again
			
			CHXString strName, strTemplate;
			CHXFileSpecifier specNewNamedFile;
			
			strName.SetFromStr255(itemFSSpec->name); // make a template like "filename_N"
			strTemplate = strName + "_%-%-%";
			
			specNewNamedFile = CHXFileSpecUtils::GetUniqueFileSpec(targetSpec, strName, strTemplate, "%-%-%");
			
			err = FSpRename(itemFSSpec, (ConstStr255Param) specNewNamedFile.GetName());
			if (err == noErr)
			{
				err = CatMove(itemFSSpec->vRefNum, itemFSSpec->parID, (ConstStr255Param) specNewNamedFile.GetName(), 
					targetFSSpec.parID, targetFSSpec.name);
				if (err == noErr)
				{
					pnres = HXR_OK;
				}
				else if (err != noErr && bDeleteIfCantMove)
				{
					// couldn't move it; delete the renamed file
					err = HDelete(itemFSSpec->vRefNum, itemFSSpec->parID, (ConstStr255Param) specNewNamedFile.GetName());
					if (err == noErr)
					{
						pnres = HXR_OK;
					}
				}
				else
				{
					// couldn't move it; change the name back
					err = FSpRename((FSSpec *) specNewNamedFile, itemFSSpec->name);
				}
			}
			else
			{
				// rename failed for some reason
				if (bDeleteIfCantMove)
				{
					err = FSpDelete(itemFSSpec);
					if (err == noErr)
					{
						pnres = HXR_OK;
					}
				}
			}			
		} 
		else
		{
			// catmove failed for some unknown reason, not a name conflict
			if (bDeleteIfCantMove)
			{
				err = FSpDelete(itemFSSpec);
				if (err == noErr)
				{
					pnres = HXR_OK;
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
	return CHXFileSpecUtils::MacFindFolder(kOnSystemDisk, kChewableItemsFolderType);
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
	// XXXSEH: Placeholder.
	
	check(!"GetAppDataDir doesn't find anyplace useful on the Mac");
	
	// GR 3/19/02  What is supposed to go in the "app data dir"?
	//
	// We could make an appName folder in the users documents directory 
	// with MacFindFolder(kUserDomain, kDocumentsFolderType), 
	// but it's not normal for applications to hardcode that location for anything.
	//
	// The windows implementation finds someplace in the user directories, but this implementation doesn't.
	
	return GetCurrentApplicationDir();
}


#ifdef _DEBUG
void CHXFileSpecUtils::TestMacFileSpecUtils()
{
}
#endif
