/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdir_carbon.cpp,v 1.8 2007/07/06 20:35:13 jfinnecy Exp $
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

#include "hxdir.h"
#include "hx_morefiles.h"
#include "fullpathname.h"
#include "hxstrutl.h"

#include "MoreFilesX.h"
#include "filespecutils.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "macff.h"	// for CMacFindFile::pmatch


// Mac paths:   full pathnames:  "Hard Disk:"  "Hard Disk:dev folder:"
//               partial paths:  ":dev folder:"  "myfile"
//
// The ending colon for paths to folders is strongly recommended.
// A word without a colon is a file name; a word followed by a colon
// is a drive name (and the full path of the root directory of the drive.)

static Boolean IsFullMacPath(const CHXString& strPath)
{
	// a full Mac pathname has a colon, but not as the first character
	// (it's also non-empty)
	
	return (strPath.Find(OS_SEPARATOR_CHAR) > 0
		&& strPath.GetAt(0) != OS_SEPARATOR_CHAR);
}

static Boolean IsPartialMacPath(const CHXString& strPath)
{
	// a partial Mac pathname is either a name without colons
	// or else starts with a colon
	return (strPath.Find(OS_SEPARATOR_CHAR) == -1
		|| strPath.GetAt(0) == OS_SEPARATOR_CHAR);
}

CHXDirectory::CHXDirectory()
{
	// call SetPath to initialize both the path string and the path FSSpec
	// to the "current directory"
	SetPath("");
	
	m_FSIterator = 0;
}

CHXDirectory::~CHXDirectory()
{
	if (m_FSIterator)
	{
		(void) FSCloseIterator(m_FSIterator);
		m_FSIterator = 0;
	}
}

void
CHXDirectory::SetPath(const char* szPath)
{
	// parent class saves the path in m_strPath

	XHXDirectory::SetPath(szPath);
}

/* folderType is actually a Mac FindFolder type - 
   if != kExtensionFolderType, kChewableItemsFolderType (or kTemporaryFolderType) is used 
   kExtensionFolderType is used when we need a temp folder to load DLLs from.
 */
HXBOOL
CHXDirectory::SetTempPath(HXXHANDLE folderType, const char* szRelPath)
{
	CHXDirSpecifier dirSpec;
	CHXString tempPath;
	
	if (folderType != kExtensionFolderType && folderType != kInstallerLogsFolderType)
	{
		folderType = kChewableItemsFolderType;
	}
	
	dirSpec = CHXFileSpecUtils::MacFindFolder(kOnAppropriateDisk, folderType);
	check(dirSpec.IsSet());

	tempPath = dirSpec.GetPathName();
	
	tempPath += szRelPath;
	
	SetPath(tempPath);
	
	return TRUE;	
}

/* Creates directory. */
HXBOOL 
CHXDirectory::Create()
{
	OSErr err = fnfErr;
	
	CHXDirSpecifier dirSpec(m_strPath);
	
	if (dirSpec.IsSet())
	{
		// create the file if it doesn't already exist
		FSRef parentRef;
		HFSUniStr255 hfsName;
		FSRef newRef;
		UInt32 newDirID;
		
		parentRef = (FSRef) dirSpec.GetParentDirectory();
		hfsName = dirSpec.GetNameHFSUniStr255();
		
		FSCatalogInfo * kDontSetCatInfo = NULL;
		FSSpec *kDontWantSpec = NULL;
		
		err = FSCreateDirectoryUnicode(&parentRef, hfsName.length,
			hfsName.unicode, kFSCatInfoNone, kDontSetCatInfo,
			&newRef, kDontWantSpec, &newDirID);
		
	}
	
	
	return (err == noErr);
}

/* Checks if directory exists. */    
HXBOOL 
CHXDirectory::IsValid()
{
	OSErr err = fnfErr;
	
	CHXDirSpecifier dirSpec(m_strPath);
	
	return dirSpec.IsSet() && CHXFileSpecUtils::DirectoryExists(dirSpec);
}


/* Deletes empty directory */
HXBOOL 
CHXDirectory::DeleteDirectory()
{
	OSErr err = fnfErr;
	
	CHXDirSpecifier dirSpec(m_strPath);
	
	return (dirSpec.IsSet() && CHXFileSpecUtils::RemoveDir(dirSpec));
}	


/* Destroys directory */
HXBOOL 
CHXDirectory::Destroy(HXBOOL bRemoveContents)
{
	OSErr err;

	if (bRemoveContents)
	{
		CHXDirSpecifier dirSpec(m_strPath);

		if (dirSpec.IsSet())
		{
			FSRef dirRef = (FSRef) dirSpec;
			
			// use MoreFilesX's routine for this
			err = FSDeleteContainer(&dirRef);
			return (err == noErr);
		}
	}
	else
	{
		return DeleteDirectory();
	}
	
	return FALSE;
}	

/* Starts enumeration process. */
CHXDirectory::FSOBJ 
CHXDirectory::FindFirst(const char* szPattern, char* szPath, UINT16 nSize)
{
	OSErr err;
	CHXDirSpecifier dirSpec(m_strPath);
	
	require(dirSpec.IsSet() && CHXFileSpecUtils::DirectoryExists(dirSpec), bail);

	// if there is already an iterator, dispose it
	if (m_FSIterator)
	{

		err = FSCloseIterator(m_FSIterator);
		check_noerr(err);

		m_FSIterator = 0;
	}

	err = FSOpenIterator((FSRef *) dirSpec, kFSIterateFlat, &m_FSIterator);
	require_noerr(err, bail);
	
	m_strFindPattern = szPattern;

	return FindNext(szPath, nSize);
	
bail:
	return FSOBJ_NOTVALID;
}

CHXDirectory::FSOBJ 
CHXDirectory::FindNext(char* szPath, UINT16 nSize)
{
	FSOBJ resultObjType;

	OSErr err;
	Boolean bIsDir;
	CHXString strTemp;
	HXBOOL bNameMatchesPattern;
	
	const ItemCount kWantOneItem = 1;
	Boolean * kDontCareIfContainerChanged = NULL;
	FSSpec * kDontWantFSSpecs = NULL;
	FSRef itemFSRef;
	HFSUniStr255 uniName;
	ItemCount actualCount;
	FSCatalogInfo catInfo;

	require_nonnull(m_FSIterator, bail); 

	// get an item, looping if it doesn't match the pattern
	do
	{
		err = FSGetCatalogInfoBulk(m_FSIterator, 
			kWantOneItem, &actualCount,
			kDontCareIfContainerChanged,
			kFSCatInfoNodeFlags, &catInfo,
			&itemFSRef, kDontWantFSSpecs,
			&uniName);
		
		if (err == noErr)
		{
			strTemp.SetFromHFSUniStr255(uniName, CFStringGetSystemEncoding());

			bNameMatchesPattern = CMacFindFile::pmatch((const char *) m_strFindPattern, (const char *) strTemp);
		}
		
	} while (err == noErr && !bNameMatchesPattern);
	
	if (err == noErr)
	{
		// got a file or directory that matches
		err = HFSPathFromFSRef(&itemFSRef, strTemp);
		require_noerr(err, bail);
		
		if (nSize >= (1 + strTemp.GetLength()))
		{
		    SafeStrCpy(szPath, (const char *) strTemp, nSize);
		}
		
		bIsDir = ((catInfo.nodeFlags & kFSNodeIsDirectoryMask) != 0);

		resultObjType = (bIsDir ? FSOBJ_DIRECTORY : FSOBJ_FILE);
	}
	else
	{
		// no more found
		resultObjType = FSOBJ_NOTVALID;
	}

	return resultObjType;

bail:
	return FSOBJ_NOTVALID;

}
	

OSErr 
CHXDirectory::GetDirID(long& dirID)
{
	CHXDirSpecifier dirSpec(m_strPath);
	
	if (dirSpec.IsSet())
	{
		dirID = dirSpec.GetDirID();
		return (dirID != 0 ? noErr : fnfErr);
	}
	return fnfErr;
}

HXBOOL 
CHXDirectory::DeleteFile(const char* szRelPath)
{
	CHXString	relativeStr(szRelPath);
	CHXString 	fullFileStr;
	OSErr		err;
	

	// make a full pathname to the file if we don't have a 
	// directory set (since some callers use a null dir
	// just to delete files)
	
	if (IsPartialMacPath(relativeStr) && (m_strPath.GetLength() > 0))
	{
		// we're deleting for a partial path from the current obj directory

		fullFileStr = m_strPath;
		// add seperator if needed
		if ((fullFileStr.GetLength() > 0) && (fullFileStr.Right(1) != ':'))
		{
		    fullFileStr += ':';
		}
		fullFileStr += relativeStr;
	}
	else
	{
		HX_ASSERT(IsFullMacPath(relativeStr));  // object's dir not set, so this works only for full paths
		
		fullFileStr = relativeStr;
	}
	
	// delete the file
	err = CHXFileSpecUtils::RemoveFile(fullFileStr);
	
	// we really should return an error
	// this gets called frequently if the file doesn't exist - not an error 
	// so return error only if file is busy
	return (err != fBsyErr);
}

/* Sets itself to current directory. */
HXBOOL 
CHXDirectory::SetCurrentDir()
{
	OSErr err;
	long dirID;
	short vRefNum;
	FSRef currDir;
	
	err = HGetVol(NULL, &vRefNum, &dirID);
	if (err == noErr)
	{
		err = FSMakeFSRef(vRefNum, dirID, NULL, &currDir);
		if (err == noErr)
		{
			CHXDirSpecifier dirSpec(currDir);
			
			SetPath(dirSpec.GetPathName());
		}
	}
	return (err == noErr);

}

/* Makes itself a current directory. */
HXBOOL 
CHXDirectory::MakeCurrentDir()
{
	CHXDirSpecifier dirSpec(m_strPath);
	
	
	if (dirSpec.IsSet())
	{
		FSRef newDir, oldDir;
		OSErr err;
		
		newDir = (FSRef) dirSpec;
		
		err = FSSetDefault(&newDir, &oldDir);
		return (err == noErr);
		
	}
	return FALSE;
}

UINT32 
CHXDirectory::Rename(const char* szOldName, const char* szNewName)
{
	UINT32		err;
	
	// Unfortunately, the semantics of the parameters for this call aren't clear
	//
	// presumably, szOldName is a full path, or a partial path in the current directory
	// presumably, szNewName is a full path, or just a name
	
	CHXString	oldFileStr(szOldName);
	CHXString	newFileStr(szNewName);
	
	CHXFileSpecifier		oldFileSpec;
	CHXFileSpecifier		newFileSpec;
	CHXDirSpecifier			destDirSpec;
	
	if (oldFileStr.Find(':') >= 0)
	{
		// the old name has a colon; convert it to a file spec
		oldFileSpec = oldFileStr;
	}
	
	if (!oldFileSpec.IsSet())
	{
		// we couldn't get a valid FSSpec for the old name,
		// so assume it's relative to the current directory,
		// and make a file spec for it
		
		CHXDirSpecifier currPathSpec(m_strPath);
		
		oldFileSpec = currPathSpec.SpecifyChildFile((const char *) oldFileStr);
		
	}
	require_action(oldFileSpec.IsSet(), CantGetSourceForRename, err = fnfErr);
	
	if (newFileStr.Find(':') >= 0)
	{
		// the new name has a colon; try to convert it to a file spec
		newFileSpec = newFileStr;
	}
	
	// make a filespec for the destination folder
	//
	// use the directory of the new file if it was specified, otherwise use
	//   the directory of the old file
	FSRef destFSRef;
	
	if (newFileSpec.IsSet())
	{
		CHXDirSpecifier newParentDir = newFileSpec.GetParentDirectory();
		
		err = FSMakeFSRef(newFileSpec.GetVRefNum(), newParentDir.GetDirID(),
			NULL, &destFSRef);
	}
	
	else
	{
		CHXDirSpecifier oldParentDir = oldFileSpec.GetParentDirectory();

		err = FSMakeFSRef(oldFileSpec.GetVRefNum(), oldParentDir.GetDirID(),
			NULL, &destFSRef);
	}
	
	check_noerr(err);
	
	destDirSpec = destFSRef;
	
	// make sure we're not trying to move to another volume
	require_action(destDirSpec.GetVRefNum() == oldFileSpec.GetVRefNum(), CantChangeVolumes, err = HXR_FAILED);

	// they're on the same drive; possibly in different folders

	// use the name from the new file spec, if we have one, or else from the parameter
	HFSUniStr255 uniName;
	
	if (newFileSpec.IsSet())
	{
		uniName = newFileSpec.GetNameHFSUniStr255();
	}
	else
	{
		newFileStr.MakeHFSUniStr255(uniName, CFStringGetSystemEncoding());
	}
	
	FSRef newFSRef;
	
	err = FSMoveRenameObjectUnicode(oldFileSpec, destDirSpec, uniName.length, uniName.unicode,
		kTextEncodingUnknown, &newFSRef);
	if (err == dupFNErr)
	{
		err = FSDeleteObject(newFileSpec);
		if (err == noErr)
		{
			err = FSMoveRenameObjectUnicode(oldFileSpec, destDirSpec, uniName.length, uniName.unicode,
				kTextEncodingUnknown, &newFSRef);
		}
	}
		
		
CantChangeVolumes:
CantGetSourceForRename:

	if (err == noErr) 	err = HXR_OK;
	else			err = HXR_FAILED;
	
	
	return err;
}

// this moves or copies and renames a file
//
// This is not related to the current directory
HXBOOL
CHXDirectory::MoveRename(const char* szSrcName, const char* szDestName, HXBOOL bMove)
{
    OSErr err;
    HXBOOL bOnSameVolume;
    Str255 pascDestFileName;
    
    CHXFileSpecifier srcFileSpec = szSrcName;
    CHXFileSpecifier destFileSpec = szDestName;
    
    CHXDirSpecifier destFileDir = destFileSpec.GetParentDirectory();
    CHXString strDestFileName = destFileSpec.GetName();
    
    // delete anything at our target location
    CHXFileSpecUtils::RemoveFile(destFileSpec);
    
    bOnSameVolume = (srcFileSpec.GetVRefNum() == destFileSpec.GetVRefNum());
    if (bMove && bOnSameVolume)
    {
    	strDestFileName.MakeStr255(pascDestFileName);
    	err = FSpMoveRenameCompat((FSSpec *) srcFileSpec, (FSSpec *) destFileDir, 
    		pascDestFileName);
    }
    
    if (!bMove || !bOnSameVolume)
    {
    	CHXString strBuffer;
    	const int kBufferSize = 40000;
    	
    	char *pBuff = strBuffer.GetBuffer(kBufferSize);
    	check_nonnull(pBuff);
    	
    	strDestFileName.MakeStr255(pascDestFileName);
    	err = FSpFileCopy((FSSpec *) srcFileSpec, (FSSpec *) destFileDir, 
    		pascDestFileName, pBuff, kBufferSize, FALSE);
    }
    
    // should we delete the source if bMove and we successfully copied it?
    
    return (err == noErr);
}

/* Checks if directory is on CD or removable drive. */    
HXBOOL 
CHXDirectory::IsRemovable()
{
	return CHXFileSpecUtils::IsDiskEjectable(m_strPath);

}

#ifdef _DEBUG
void CHXDirectory::TestHXDir()
{
	char *pszTestDir = "Tenspot:123456789a123456789b123456789c123456789d:";
	char *pszScanDir = "Tenspot:";
	char *pszFakeDir = "Tenspot:123456789a123456789b123456789c123456789dXXX:";
	
	CHXDirectory dirTest;
	dirTest.SetPath(pszTestDir);
	
	CHXDirectory dirInvalid;
	dirInvalid.SetPath(pszFakeDir);
	
	HXBOOL bIsValid = dirTest.IsValid();
	check(bIsValid);
	
	bIsValid = dirInvalid.IsValid();
	check(!bIsValid);
	
	CHXDirectory dirTemp;
	dirTemp.SetTempPath(0, "");
	bIsValid = dirTemp.IsValid();
	check(bIsValid);
	dirTemp.SetTempPath(0, "NotInTemp");
	bIsValid = dirTemp.IsValid();
	check(!bIsValid);

	CHXDirectory dirNew;
	CHXString strNewPath;
	strNewPath = pszTestDir;
	strNewPath += "New Folder";
	dirNew.SetPath(strNewPath);
	bIsValid = dirNew.IsValid();
	check(!bIsValid);
	
	dirNew.Create();
	bIsValid = dirNew.IsValid();
	check(bIsValid);
	
	dirNew.Destroy(TRUE);
	bIsValid = dirNew.IsValid();
	check(!bIsValid);
	
	CHXDirectory oldCurrentDir, newCurrentDir;
	
	oldCurrentDir.SetCurrentDir();
	dirTest.MakeCurrentDir();
	newCurrentDir.SetCurrentDir();
	check(strcmp(oldCurrentDir.GetPath(), dirTest.GetPath()));
	check(!strcmp(newCurrentDir.GetPath(), dirTest.GetPath()));
	oldCurrentDir.MakeCurrentDir();
	
	HXBOOL bIsRemovable = dirTest.IsRemovable();
	check(!bIsRemovable);
	
	CHXDirectory scanDir;
	scanDir.SetPath(pszScanDir);

	FSOBJ obj;
	char foundPath[4000]; /* Flawfinder: ignore */
	obj = scanDir.FindFirst("*.*", foundPath, 4000);
	while (obj != FSOBJ_NOTVALID)
	{
		obj = scanDir.FindNext(foundPath, 4000);
	}
	
	long dirID;
	OSErr err;
	err = scanDir.GetDirID(dirID);
	FSSpec spec = scanDir.GetFSSpec();

	/*
	CHXString strTemp(pszScanDir);
	CHXString strTempFrog(pszScanDir);
	scanDir.Rename("test", "test frog");
	strTempFrog += "test frog";
	strTemp += "test";
	scanDir.Rename(strTempFrog, strTemp);
	*/
	scanDir.DeleteFile("123456789a123456789b123456789c123456789d copy 1.png");
	

}
#endif _DEBUG
