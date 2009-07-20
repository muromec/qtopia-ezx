/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdir.cpp,v 1.7 2005/03/14 19:36:28 bobclark Exp $
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

#include "hxdir.h"
#include "HX_MoreProcesses.h"
#include "pn_morefiles.h"
#include "fullpathname.h"
#include "hxstrutl.h"

#ifndef _CARBON
#include "morefilesextras.h"
#include "filecopy.h"
#else
#include "MoreFilesX.h"
#include "filespecutils.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


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

CHXDirectory::CHXDirectory() : m_pCurrentFileName(NULL), m_pNextFileName(NULL), m_pPattern(NULL),
			       m_nDefaultVRefNum(0), m_nDirItems(0)
{
	// call SetPath to initialize both the path string and the path FSSpec
	// to the "current directory"
	SetPath("");
}

CHXDirectory::~CHXDirectory()
{
    if (m_pCurrentFileName)
    {
	delete [] m_pCurrentFileName;
	m_pCurrentFileName = 0;
    }

    if (m_pNextFileName)
    {
	delete [] m_pNextFileName;
	m_pNextFileName = 0;
    }
    if (m_pPattern)
    {
	delete [] m_pPattern;
	m_pPattern = 0;
    }
}

void
CHXDirectory::SetPath(const char* szPath)
{
	if (szPath)
	{
		
		// coerce path to a CHXString, then to a FSSpec,
		// and store in our object
		
		CHXString	pathStr(szPath);
		// make sure path ends with a ':' 
		if ((pathStr.GetLength() > 0) && (pathStr[pathStr.GetLength()-1] != ':'))
			pathStr += ':';

		// it is ok for path not to be valid yet

		m_dirSpec = pathStr;
	}
	
	XHXDirectory::SetPath(szPath);
}

/* folderType is actually a Mac FindFolder type - 
   if != kExtensionFolderType, kChewableItemsFolderType (or kTemporaryFolderType) is used 
   kExtensionFolderType is used when we need a temp folder to load DLLs from.
 */
HXBOOL
CHXDirectory::SetTempPath(HXXHANDLE folderType, const char* szRelPath)
{
    CHXString tempStr;
    
	short	tempVRefNum;
	long	tempDirID;
	FSSpec	tempSpec;
	OSErr	err;
	FSSpec	appSpec;
	
#ifndef USE_APP_DIR
	if (kExtensionFolderType == folderType)
	{
	    err = FindFolder(kOnSystemDisk, kExtensionFolderType, kCreateFolder, &tempVRefNum, &tempDirID);
	}
	else if (kInstallerLogsFolderType == folderType)
	{
	    // if >= Sys 8.5, use Installer Logs folder
	    long	sysVersion;
	    char*	bytes=(char*)&sysVersion;
	    ::Gestalt(gestaltSystemVersion,&sysVersion);
	    if (bytes[2]>8 || ((bytes[2]==8) && (bytes[3] >= 0x50)))
	    	err = FindFolder(kOnSystemDisk, kInstallerLogsFolderType, kCreateFolder, &tempVRefNum, &tempDirID);
	    else
	    {
		GetCurrentAppSpec(&appSpec);
		tempVRefNum = appSpec.vRefNum;
		tempDirID = appSpec.parID;
		err = noErr;
	    }
	}
	else
	{
 	    err = FindFolder(kOnSystemDisk, kChewableItemsFolderType, kCreateFolder, &tempVRefNum, &tempDirID);
	}
	if (err != noErr)
	{
		err = FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder, &tempVRefNum, &tempDirID);
	}

#else

	// use the app's folder rather than the temporary items folder
	
	if (GetCurrentAppSpec(&appSpec))
	{
		tempVRefNum = appSpec.vRefNum;
		tempDirID = appSpec.parID;
		err = noErr;
	}
	else
	{
		err = fnfErr;
	}
#endif

	if (err != noErr)
	{
		tempVRefNum = -1;	// boot disk
		tempDirID = fsRtDirID;	// root directory
	}

	// we have the temporary folder's dirID and vRefNum
	//
	// make a FSSpec for the folder
	
	err = FSMakeFSSpec(tempVRefNum, tempDirID, "\p", &tempSpec);
	check_noerr(err);
	
	CHXString	tempPath;
	
	tempPath = tempSpec;	// coerce to a full pathname
	tempPath += szRelPath;
	
	SetPath(tempPath);
	
	return true;
	
}

/* Creates directory. */
HXBOOL 
CHXDirectory::Create()
{
	OSErr	err;
	long	newDirID;
	
	if (EnsureValidPathSpec())
	{
		err = FSpDirCreate(&m_dirSpec, smSystemScript, &newDirID);
	}
	else
	{
		err = fnfErr;
	}
	return (err == noErr);
}

/* Checks if directory exists. */    
HXBOOL 
CHXDirectory::IsValid()
{
    FSSpec		dirSpec;
    CInfoPBRec	cinfo;
    OSErr		err;
    Boolean		dirFlag;
    
    dirFlag = false;
    err = noErr;
    
	if (EnsureValidPathSpec())
	{
		// make a copy of the filespec so that PBGetCatInfo doesn't munge 
		// the name field (paranoid, since that shouldn't happen to a 
		// canonical FSSpec like this)
		
		dirSpec = m_dirSpec;
	
		// check that it's a valid folder or volume
		
		cinfo.dirInfo.ioNamePtr = dirSpec.name;
		cinfo.dirInfo.ioVRefNum = dirSpec.vRefNum;
		cinfo.dirInfo.ioDrDirID = dirSpec.parID;
		cinfo.dirInfo.ioFDirIndex = 0;	// use name and dirID
		
		err = PBGetCatInfoSync(&cinfo);
		
		if (noErr == err)
		{
			// check the folder/volume bit
		    dirFlag = ((cinfo.dirInfo.ioFlAttrib & ioDirMask) != 0);
		}
    }
    
    return ((err == noErr) && dirFlag);
}


/* Deletes empty directory */
HXBOOL 
CHXDirectory::DeleteDirectory()
{
	OSErr		err;

	if (EnsureValidPathSpec())
	{
		err = FSpDelete(&m_dirSpec);
	}
	else
	{
		err = fnfErr;
	}
	return (err == noErr);
}	

/* EnsureValidPathSpec

   If the path was so speculative when it was set that
   we couldn't make a FSSpec then, make one now.
   
   Note that this *doesn't* guarantee that the object
   exists, just that the m_dirSpec matches the path. 
   
   Returns true if the FSSpec is valid, false if no
   FSSpec can be made for the current path. 
   
   Call this at the start of any routine that expects
   a valid FSSpec to have been already set. */
   
HXBOOL 
CHXDirectory::EnsureValidPathSpec()
{
	// to avoid the cost of conversion again,
	// generate the FSSpec only if it's not 
	// currently valid
	
	if (m_dirSpec.vRefNum == 0) 
	{
		m_dirSpec = m_strPath;
	}
	return (m_dirSpec.vRefNum != 0);
}	

/* Destroys directory */
HXBOOL 
CHXDirectory::Destroy(HXBOOL bRemoveContents)
{
    OSErr err;

	if (EnsureValidPathSpec())
	{
		if (bRemoveContents)
	    {
		    	// use MoreFilesExtras' routine for this
				err = ::DeleteDirectory(m_dirSpec.vRefNum, m_dirSpec.parID, m_dirSpec.name);
				return (err == noErr);
	    }
	    else
	    {
	    	return DeleteDirectory();
	    }
	}
	else
	{
		// invalid path
		return false;
	}
}	

/* Starts enumeration process. */
CHXDirectory::FSOBJ 
CHXDirectory::FindFirst(const char* szPattern, char* szPath, UINT16 nSize)
{
    FSOBJ RetVal = FSOBJ_NOTVALID;
    int len;

    OSErr err = noErr;
    CHXString tempStr;
    	
    if (err == noErr)
    {
	m_nIndex = 0;
	HXBOOL bContinue;
	FSSpec nextFSpec;
	char* patternStart;

	if (!m_pPattern)
	{
	    m_pPattern	    = new char[31];
	    *m_pPattern = 0;
	}
	SafeStrCpy(m_pPattern, szPattern, 31);
	
    	tempStr = m_strPath;
    	if (m_strPath.GetLength())
    	{
    	    if (tempStr[tempStr.GetLength()-1] != ':')
    	    	tempStr += ":";
    	}
    	tempStr += szPattern;
    	(void) FSSpecFromPathName(tempStr,&nextFSpec);
	if (IsFolder(&nextFSpec))
	{
	    SafeStrCpy(szPath, (const char*)tempStr, nSize);
	    ++m_nIndex;
	    return FSOBJ_DIRECTORY;
	}

    	(void) FSSpecFromPathName(m_strPath,&m_fileSpec);

	CInfoPBRec		pb;
	OSErr			err=noErr;

	memset(&pb,0,sizeof(pb));
				
	pb.hFileInfo.ioVRefNum=m_fileSpec.vRefNum;
	pb.hFileInfo.ioFDirIndex=0;
	pb.hFileInfo.ioNamePtr=m_fileSpec.name;
	pb.hFileInfo.ioDirID=m_fileSpec.parID;

	err = PBGetCatInfoSync(&pb);
	
	// if m_fileSpec doesn't point to a directory
	// check if szPath + szPattern (nextFSpec) points to a valid file
	if (noErr != err)
	{
	    if (noErr == ::FSMakeFSSpec(nextFSpec.vRefNum, nextFSpec.parID, nextFSpec.name, &nextFSpec))
	    {
	    	tempStr = nextFSpec;
	    	SafeStrCpy(szPath, (const char*)tempStr, nSize);
	    	return FSOBJ_FILE;
	    }
	}
	
	m_nDirItems = pb.dirInfo.ioDrNmFls;
    	
	while (::FSpIterateDirectory(&m_fileSpec, ++m_nIndex, &nextFSpec))
	{
	    bContinue = TRUE;
	    if (!m_pNextFileName)
	    {
		m_pNextFileName	    = new char[_MAX_PATH];
		*m_pNextFileName = 0;
	    }
	    
	    if (!m_pCurrentFileName)
	    {
		m_pCurrentFileName	    = new char[_MAX_PATH];
		*m_pCurrentFileName = 0;
	    }
	    
	    // matches pattern(extension)? (eg. DLL)
	    len = nextFSpec.name[0];
	    if (szPattern)
	    {
	    	if ( 0 == strcmp("*.*", szPattern) ||
	    	    	((szPattern[0] == '*') && (strlen(szPattern) == 1)) )
	    	    bContinue = TRUE;
	    	else
	    	{
	    	    INT16 patlen = strlen(szPattern);
	    	    if (szPattern[0] == '*')
	    	    {
	    	    	if (0 != strncasecmp((char*)&nextFSpec.name[len-2], &szPattern[strlen(szPattern)-3], 3))
	    	    	    bContinue = FALSE;
	    	    }
	    	    else if ((patternStart = strchr(m_pPattern, '*')) != 0) //eg. pncrt*.dll
	    	    {
	    	    	INT16 patternOffset = patternStart-m_pPattern;
	    	    	if (0 != strncasecmp((char*)&nextFSpec.name[1], m_pPattern, patternOffset))
	    	    	    bContinue = FALSE;
	    	    	if (bContinue && (patternOffset < patlen))
	    	    	{
	    	    	    int nCompareLen = (patlen > nextFSpec.name[0]) ? nextFSpec.name[0] - patternOffset
	    	    	    						   : patlen - patternOffset - 1;
	    	    	    int nSpecPos = nextFSpec.name[0] - nCompareLen + 1;
	    	    	    if (0 != strncasecmp((char*)&nextFSpec.name[nSpecPos], &m_pPattern[patternOffset+1], nCompareLen))
	    	    	    	bContinue = FALSE;
	    	    	}
	    	    }
	    	    else
	    	    {
	    	    	if ((patlen != nextFSpec.name[0]) ||
	    	    		(0 != strncasecmp((char*)&nextFSpec.name[1], szPattern, patlen)))
	    	    	    bContinue = FALSE;
	    	    }
	    	}
	    }
	    if (bContinue)
	    {
	    	strncpy(m_pNextFileName, (char*)&nextFSpec.name[1], len); /* Flawfinder: ignore */
	    	m_pNextFileName[len] = 0;
	    	tempStr = m_strPath;
	    	if (m_strPath.GetLength())
	    	{
	    	    if (tempStr[tempStr.GetLength()-1] != ':')
	    	    	tempStr += ":";
	    	}
	    	tempStr += m_pNextFileName;
	    	SafeStrCpy(szPath, (const char*)tempStr, nSize);

	    	if (IsFolder(&nextFSpec))
	    	    return FSOBJ_DIRECTORY;
	    	else
	    	    return FSOBJ_FILE;
	    }
	}
        RetVal = FindNext(szPath, nSize);
    }
    return RetVal;
}

/* Continues enumeration process. */
CHXDirectory::FSOBJ 
CHXDirectory::FindNext(char* szPath, UINT16 nSize)
{
    FSOBJ RetVal = FSOBJ_NOTVALID;
    CHXString tempStr;
    FSSpec nextFSpec;
    HXBOOL bContinue;

    if (m_pNextFileName)
    {
	SafeStrCpy(m_pCurrentFileName, m_pNextFileName, nSize);
    }
    else
    {
	if (m_pCurrentFileName)
	{
	    delete [] m_pCurrentFileName;
	    m_pCurrentFileName = NULL;
	}
    }
/*
    tempStr = szPath;
    ::FSMakeFSSpec(0,0,*((Str255*)tempStr),&nextFSpec);

    if (IsFolder(&nextFSpec, 1))
    {
	strcpy(szPath, (const char*)tempStr);
	return FSOBJ_DIRECTORY;
    }
*/    
    if (m_pNextFileName)
    {
	INT16 len = 0;
	char* patternStart;

	CInfoPBRec		pb;
	OSErr			err=noErr;

	memset(&pb,0,sizeof(pb));
				
	pb.hFileInfo.ioVRefNum=m_fileSpec.vRefNum;
	pb.hFileInfo.ioFDirIndex=0;
	pb.hFileInfo.ioNamePtr=m_fileSpec.name;
	pb.hFileInfo.ioDirID=m_fileSpec.parID;

	err = PBGetCatInfoSync(&pb);
	
	UINT16 dirItems = pb.dirInfo.ioDrNmFls;
	if (dirItems < m_nDirItems) // items have been deleted
	{
	    m_nIndex -= (m_nDirItems - dirItems);
	    m_nDirItems = dirItems;
	}

	while (::FSpIterateDirectory(&m_fileSpec, ++m_nIndex, &nextFSpec))
	{
	    bContinue = TRUE;
	    if (m_pPattern)
	    {
	    	INT16 patlen = strlen(m_pPattern);
	    	if ( 0 == strcmp("*.*", m_pPattern) || // *.*
	    	    	((m_pPattern[0] == '*') && (strlen(m_pPattern) == 1)) ) // *
	    	    bContinue = TRUE;
	    	else
	    	{
	    	    if (m_pPattern[0] == '*') //eg. *.dll
	    	    {
	    	    	if (0 != strncasecmp((char*)&nextFSpec.name[len-2], &m_pPattern[strlen(m_pPattern)-3], 3))
	    	    	    bContinue = FALSE;
	    	    }
	    	    else if ((patternStart = strchr(m_pPattern, '*')) != 0) //eg. pncrt*.dll
	    	    {
	    	    	INT16 patternOffset = patternStart-m_pPattern;
	    	    	if (0 != strncasecmp((char*)&nextFSpec.name[1], m_pPattern, patternOffset))
	    	    	    bContinue = FALSE;
	    	    	if (bContinue && (patternOffset < patlen))
	    	    	{
	    	    	    int nCompareLen = (patlen > nextFSpec.name[0]) ? nextFSpec.name[0] - patternOffset
	    	    	    						   : patlen - patternOffset - 1;
	    	    	    int nSpecPos = nextFSpec.name[0] - nCompareLen + 1;
	    	    	    if (0 != strncasecmp((char*)&nextFSpec.name[nSpecPos], &m_pPattern[patternOffset+1], nCompareLen))
	    	    	    	bContinue = FALSE;
	    	    	}
	    	    }
	    	    else // eg. pncrtd.dll
	    	    {
	    	    	if (0 != strncasecmp((char*)&nextFSpec.name[1], m_pPattern, patlen))
	    	    	    bContinue = FALSE;
	    	    }
	    	}
	    }
	    
	    
	    if (bContinue)
	    {
		len = nextFSpec.name[0];
		strncpy(m_pNextFileName, (char*)&nextFSpec.name[1], len); /* Flawfinder: ignore */
		m_pNextFileName[len] = 0;
	    	tempStr = m_strPath;
	    	if (m_strPath.GetLength())
	    	{
	    	    if (tempStr[tempStr.GetLength()-1] != ':')
	    	    	tempStr += ":";
	    	}
		tempStr += m_pNextFileName;
		SafeStrCpy(szPath, (const char*)tempStr, nSize);
	    	if (IsFolder(&nextFSpec))
	    	    return FSOBJ_DIRECTORY;
	    	else
	    	    return FSOBJ_FILE;
	    }
	}
	if (RetVal != FSOBJ_FILE)
	{
	    delete [] m_pNextFileName;
	    m_pNextFileName = 0;
	}
    }
    return RetVal;
}

OSErr 
CHXDirectory::GetDirID(long& dirID)
{
	FSSpec		dirSpec;
	CInfoPBRec	cinfo;
	OSErr		err;
	Boolean		dirFlag;
	
	if (EnsureValidPathSpec())
	{
		dirSpec = m_dirSpec;
		
		// call GetCatInfo to get it's dirID
		
		cinfo.dirInfo.ioNamePtr = dirSpec.name;
		cinfo.dirInfo.ioVRefNum = dirSpec.vRefNum;
		cinfo.dirInfo.ioDrDirID = dirSpec.parID;
		cinfo.dirInfo.ioFDirIndex = 0;	// use name and dirID
		
		err = PBGetCatInfoSync(&cinfo);
		
		// be sure it's really a directory
		if (err == noErr)
		{
			dirFlag = ((cinfo.dirInfo.ioFlAttrib & ioDirMask) != 0);
			if (!dirFlag) err = dirNFErr;
		}
		
		if (err == noErr)
		{
			dirID = cinfo.dirInfo.ioDrDirID;
		}
	}	
	else
	{
		// invalid path
		err = fnfErr;
	}
	return err;
}

HXBOOL 
CHXDirectory::DeleteFile(const char* szRelPath)
{
	CHXString	relativeStr(szRelPath);
	CHXString 	fullFileStr;
	FSSpec		fileSpec;
	OSErr		err;
	

	// make a full pathname to the file if we don't have a 
	// directory set (since some callers use a null dir
	// just to delete files)
	
	if (IsPartialMacPath(relativeStr) && (m_strPath.GetLength() > 0))
	{
		// we're deleting for a partial path from the current obj directory

		fullFileStr = m_strPath;
		// add seperator if needed
		if ((fullFileStr.GetLength() > 0) && (fullFileStr[fullFileStr.GetLength()-1] != ':'))
		    fullFileStr += ':';
		fullFileStr += relativeStr;
	}
	else
	{
		HX_ASSERT(IsFullMacPath(relativeStr));  // object's dir not set, so this works only for full paths
		
		fullFileStr = relativeStr;
	}
	
	// delete the file
	fileSpec = fullFileStr;
	err = FSpDelete(&fileSpec);
	// we really should return an error
	// this gets called frequently if the file doesn't exist - not an error 
	// so return error only if file is busy
	return (err != fBsyErr);
}

/* Sets itself to current directory. */
HXBOOL 
CHXDirectory::SetCurrentDir()
{
	// This was never implemented, doesn't work reliably on
	// Macs, and since it's a bad idea, it's not worth implementing.
	
    long 	dirID;
    short	vRefNum;
    OSErr err = ::HGetVol(NULL, &vRefNum, &dirID);
    err = ::FSMakeFSSpec(vRefNum, dirID, "\p", &m_dirSpec);
    if (noErr == err)
    {
    	m_strPath = m_dirSpec;
    	m_strPath = m_strPath.Left(m_strPath.GetLength()-1);
    	return TRUE;
    }
    return FALSE;
}

/* Makes itself a current directory. */
HXBOOL 
CHXDirectory::MakeCurrentDir()
{
	// Relying on a global current default directory is a bad
	// idea, so never call this.
	//
	// Also beware that calling GetVol() won't work properly
	// after this, anyway.  See ch. 2 of Inside Mac: Files
	// (under Manipulating the Default Volume and Directory)
	// for an explanation of why.
	//
	// SetVol, GetVol, and everything using
	// WDRefNums has been obsolete since 1987.
	// Use FSSpecs (or full pathnames, if necessary) instead.
	
	OSErr	err;
	long 	dirID;
	
	err = GetDirID(dirID);
	if (err == noErr)
	{
		err = HSetVol(NULL, m_dirSpec.vRefNum, dirID);
	}
	
	return (err == noErr);
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
	StringPtr	newNamePtr;
	
	FSSpec		oldFileSpec;
	FSSpec		newFileSpec;
	FSSpec		destDirSpec;
	
	(void) EnsureValidPathSpec();
	
	oldFileSpec.vRefNum = 0;
	if (oldFileStr.Find(':') >= 0)
	{
		// the old name has a colon; convert it to a file spec
		oldFileSpec = oldFileStr;
	}
	
	if (oldFileSpec.vRefNum == 0)
	{
		// we couldn't get a valid FSSpec for the old name,
		// so assume it's relative to the current directory,
		// and make a file spec for it
		
		oldFileStr = m_dirSpec;
		oldFileStr += ":";
		oldFileStr += szOldName;
		oldFileSpec = oldFileStr;
	}
	require_action(oldFileSpec.vRefNum != 0, CantGetSourceForRename, err = fnfErr);
	
	newFileSpec.vRefNum = 0;
	if (newFileStr.Find(':') >= 0)
	{
		// the new name has a colon; try to convert it to a file spec
		newFileSpec = newFileStr;
	}
	
	// make a filespec for the destination folder
	//
	// use the directory of the new file if it was specified, otherwise use
	//   the directory of the old file
	
	err = FSMakeFSSpec(newFileSpec.vRefNum, 
			(newFileSpec.vRefNum == 0 ? oldFileSpec.parID : newFileSpec.parID), 
			"\p", &destDirSpec);
	check_noerr(err);
	
	// make sure we're not trying to move to another volume
	require_action(destDirSpec.vRefNum == oldFileSpec.vRefNum, CantChangeVolumes, err = HXR_FAILED);

	// they're on the same drive; possibly in different folders

	// use the name from the new file spec, if we have one, or else from the parameter
	newNamePtr = (newFileSpec.vRefNum ? newFileSpec.name : newFileStr);
	
	// use morefiles' Move/Rename call
	err = FSpMoveRenameCompat(&oldFileSpec, &destDirSpec, newNamePtr);
	if (err == dupFNErr)
	{
		err = FSpDelete(&newFileSpec);
		if (err == noErr)
		{
			err = FSpMoveRenameCompat(&oldFileSpec, &destDirSpec, newNamePtr);
		}
	}
		
CantChangeVolumes:
CantGetSourceForRename:

	if (err == noErr) 	err = HXR_OK;
	else				err = HXR_FAILED;
	
	
	return err;
}

// this moves or copies and renames a file
HXBOOL
CHXDirectory::MoveRename(const char* szSrcName, const char* szDestName, HXBOOL bMove)
{
    OSErr err;

    FSSpec srcSpec, destSpec, deleteSpec;
    CHXString strSourcePath(szSrcName);
    srcSpec = strSourcePath;
    
    CHXString strDestFilePath(szDestName);
    CHXString strDestFile;
    strDestFile = (strrchr(szDestName, OS_SEPARATOR_CHAR)+1);
    CHXString strDestPath(szDestName);
    strDestPath = strDestPath.Left(strDestPath.ReverseFind(OS_SEPARATOR_CHAR));
    destSpec = strDestPath;
    
    if (noErr == FSSpecFromPathName(strDestFilePath, &deleteSpec))
    {
	err = ::FSpDelete(&deleteSpec); //delete existing file
    }
    // Note, FSpMoveRenameCompat doesn't move across volumes, 
    // so copy, then delete original (below) if a move has to be done??
    
    if (bMove)
    {
    	err = FSpMoveRenameCompat(&srcSpec, &destSpec, *((Str255*)strDestFile));
    }
    if (!bMove || (diffVolErr == err))
    {
    	char* buffer = new char[0x40000];
	err = FSpFileCopy(&srcSpec,&destSpec, (ConstStr255Param)strDestFile, buffer, 0x40000, FALSE);
	delete buffer;
    }


    return (err == noErr);	
}

/* Checks if directory is on CD or removable drive. */    
HXBOOL 
CHXDirectory::IsRemovable()
{
#ifdef _CARBON

	if (EnsureValidPathSpec())
	{
		return CHXFileSpecUtils::IsDiskEjectable(m_dirSpec);
	}
	return FALSE;

#else
	OSErr				err;
	HParamBlockRec		hpb;

	
	DrvQElPtr	queueElPtr;
    QHdrPtr     queueHeader;
    Ptr         p;

	if (EnsureValidPathSpec())
	{	
		hpb.volumeParam.ioVRefNum = m_dirSpec.vRefNum;
		hpb.volumeParam.ioNamePtr = NULL;
		hpb.volumeParam.ioVolIndex = 0;					// use vRefNum only

		err = PBHGetVInfoSync(&hpb);
		check_noerr(err);
		
		if (err != noErr) return false;

		// check the flag bits in the drive queue (from Technote FL 530)
	    queueHeader = GetDrvQHdr();
	    queueElPtr = (DrvQElPtr) queueHeader->qHead;

	    while (queueElPtr != nil)
	    {
	        if (queueElPtr->dQDrive == hpb.volumeParam.ioVDrvInfo) 
	        {
	            p = (Ptr) queueElPtr;
	            p -= 3; /* to get to the byte with eject info */
	            if ((*p) & 8) return false;		// non-ejectable
	            else return true;				// ejectable
	        }
	        queueElPtr = (DrvQElPtr) queueElPtr->qLink;
	    }
	}	
	return false;	// we can't find this drive
#endif
}
