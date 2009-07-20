/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fullpathname.cp,v 1.9 2007/07/06 20:39:19 jfinnecy Exp $
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

#include "platform/mac/FullPathName.h"

#if defined(_MAC_UNIX) || defined(_CARBON)
#include "platform/mac/MoreFilesX.h"
#include "platform/mac/cfwrappers.h"
#endif

/*
 * Pascal string utilities
 */

#ifdef __cplusplus
extern "C" {
#endif 

#if defined(_MAC_UNIX) || defined(_CARBON)

// prototypes for internal routines
static OSErr PathFromFSRefInternal(CFURLPathStyle pathStyle, const FSRef *ref, CHXString& fullPathName);
static OSErr FSRefFromPathInternal(CFURLPathStyle pathStyle, const char *path, FSRef *outRef);
static OSErr AlternateFSRefFromHFSPathInternal(const char* hfspath, FSRef* outRef);

static OSStatus URLFromPathInternal(CFURLPathStyle pathStyle, const char *pszPath, CHXString& strURL);
static OSStatus PathFromURLInternal(CFURLPathStyle pathStyle, const char *pszURL, CHXString& fullPathName);

OSStatus PathFromCFURLInternal(CFURLPathStyle pathStyle, CFURLRef urlRef, CHXString& fullPathName);

OSErr PathFromFSRefInternal(CFURLPathStyle pathStyle, const FSRef *ref, CHXString& fullPathName)
{
	CHXCFString cfs;
	CHXCFURL cfurl;
	
	require_nonnull(ref, BadParam);
	
	// make a CFURL from the FSRef
	cfurl = ref;
	require(cfurl.IsSet(), CantSetURLToRef);
	
	return PathFromCFURLInternal(pathStyle, cfurl, fullPathName);
	
CantSetURLToRef:
BadParam:
	return errFSBadFSRef;
}

OSErr FSRefFromPathInternal(CFURLPathStyle pathStyle, const char *path, FSRef *outRef) 
{
	CHXCFURL cfurl;
	CHXCFString cfstr;
	Boolean bSuccess;
	CFStringEncoding encoding;
	
	const Boolean kDirectoryDoesntMatter = false; // we could look at the last char of the string if it mattered
	
	require_nonnull(outRef, BadOutputParam);

	// copy the path to a CFString
	encoding = (pathStyle == kCFURLHFSPathStyle ? CFStringGetSystemEncoding() : (CFStringEncoding) kCFStringEncodingUTF8);
	cfstr = CHXCFString(path, encoding);
	if (!cfstr.IsSet() && (kCFStringEncodingUTF8 == encoding))
	{
		// it's not a proper path; it might be something from inside a text file, probably created on Windows
		encoding = CFStringGetSystemEncoding();
		if (encoding == kCFStringEncodingMacRoman)
		{
			encoding = kCFStringEncodingWindowsLatin1;
		}
		cfstr = CHXCFString(path, encoding);
	}
	require(cfstr.IsSet(), CantMakeCFString);
	
	// make a CFURL from the CFString
	cfurl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfstr, pathStyle, kDirectoryDoesntMatter);
	require(cfurl.IsSet(), CantSetURL);
	
	// get an FSRef from the URL; this will fail if the item doesn't really exist
	bSuccess = CFURLGetFSRef(cfurl, outRef);
	//if (!bSuccess) CFShow(cfurl);
	
	if ((!bSuccess) && (pathStyle == kCFURLHFSPathStyle))
	{
		OSErr err = AlternateFSRefFromHFSPathInternal(path, outRef);
		bSuccess = (err == noErr);
	}
	require_quiet(bSuccess, CantGetRefFromURL);

	// now make an FSRef
	return noErr;
	
CantGetRefFromURL:
CantSetURL:
CantMakeCFString:
BadOutputParam:
	return paramErr;
}


// GR 7/23/02 blech...
// AlternateFSRefFromHFSPathInternal exists only because Mac OS X 10.1.5 and maybe later
// can't make CFURLs and then FSRefs from HFS paths that have volume names with high-8 bit
// characters in them.

OSErr AlternateFSRefFromHFSPathInternal(const char* hfspath, FSRef* outRef)
{
	require_nonnull_return(outRef, paramErr);
	require_nonnull_return(hfspath, paramErr);
	
	OSErr err = -1;
	
	// use FSMakeFSSpec for short paths, NewAliasMinimalFromFullPath for long paths
	
	if (strlen(hfspath) < 256)
	{
		Str255 pascPath;
		FSSpec spec;
	
		c2pstrcpy(pascPath, hfspath);
		err = FSMakeFSSpec(0, 0, pascPath, &spec);
		if (err == noErr || err == fnfErr)
		{
			err = FSpMakeFSRef(&spec, outRef);
		}
	}
	else
	{
		AliasHandle aliasH;
		Boolean wasChanged;
		
		err = NewAliasMinimalFromFullPath(strlen(hfspath), hfspath, "\p", "\p", &aliasH);
		if (err == noErr)
		{
			err = FSResolveAliasWithMountFlags(nil, aliasH, outRef, &wasChanged, kResolveAliasFileNoUI);
			
			DisposeHandle((Handle) aliasH);
		}
	}
	return err;
}


OSStatus URLFromPathInternal(CFURLPathStyle pathStyle, const char *pszPath, CHXString& strURL)
{
	CHXCFURL cfurl;
	CHXCFString cfstr;
	CFStringRef cfsNoRelease;
	CFStringEncoding encoding;
	
	const Boolean kDirectoryDoesntMatter = false; // we could look at the last char of the string if it mattered
	
	// copy the path into a CFString
	encoding = (pathStyle == kCFURLHFSPathStyle ? CFStringGetSystemEncoding() : (CFStringEncoding) kCFStringEncodingUTF8);
	cfstr = CHXCFString(pszPath, encoding);
	
	// make a CFURL from the CFString of the path
	cfurl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfstr, pathStyle, kDirectoryDoesntMatter);
	require(cfurl.IsSet(), CantSetURL);

	// get the URL as a CFString
	cfsNoRelease = ::CFURLGetString(cfurl); // we are NOT supposed to release the CFString obtained here
	require_nonnull(cfsNoRelease, CantGetURLString);
	
	// copy the URL from the CFString into the output CHXString
	strURL.SetFromCFString(cfsNoRelease, kCFStringEncodingUTF8);
	
	return noErr;
	
CantGetURLString:
CantSetURL:
	return paramErr;
}

OSStatus PathFromURLInternal(CFURLPathStyle pathStyle, const char *pszURL, CHXString& fullPathName)
{
	CHXCFURL cfurl;
	CHXCFString cfs;
	
	// make a CFURL from the input url
	cfurl = pszURL;
	require(cfurl.IsSet(), CantMakeURL);

	return PathFromCFURLInternal(pathStyle, cfurl, fullPathName);

CantMakeURL:
	return kNSLBadURLSyntax;
}

OSStatus PathFromCFURLInternal(CFURLPathStyle pathStyle, CFURLRef urlRef, CHXString& fullPathName)
{
	if (pathStyle == kCFURLHFSPathStyle)
	{
		CHXCFString cfs;

		// get the HFS path as a CFString from the CFURL
		cfs = CFURLCopyFileSystemPath (urlRef, pathStyle);
		require(cfs.IsSet(), CantSetStringToURL);

		// copy the path to the CHXString
		fullPathName.SetFromCFString(cfs, CFStringGetSystemEncoding());
	}
	else
	{
#if defined(_MAC_MACHO)

		// POSIX paths are canonical UTF-8, as returned by CFURLGetFileSystemRepresentation

		const Boolean kAbsolutePath = true;

		UInt8* buff = (UInt8*) fullPathName.GetBuffer(1 + MAXPATHLEN);
		
		Boolean succeeded = ::CFURLGetFileSystemRepresentation(urlRef, kAbsolutePath, buff, MAXPATHLEN);

		fullPathName.ReleaseBuffer();

		require(succeeded, CantSetStringToURL);
#else
		HX_ASSERT(!"CFM builds shouldn't be getting posix paths!");
#endif
	}

	require_quiet(!fullPathName.IsEmpty(), URLCreationFailed);

	return noErr;

URLCreationFailed:
CantSetStringToURL:
		return kNSLBadURLSyntax;
}

#pragma mark -

// ref <--> path routines


OSErr PathFromFSRef(const FSRef *ref, CHXString& fullPathName)
{
#ifdef _MAC_CFM
	return HFSPathFromFSRef(ref, fullPathName);
#else
	return POSIXPathFromFSRef(ref, fullPathName);
#endif
}

OSErr FSRefFromPath(const char *path, FSRef *outRef)
{
#ifdef _MAC_CFM
	return FSRefFromHFSPath(path, outRef);
#else
	return FSRefFromPosixPath(path, outRef);
#endif
}

OSErr PathFromURL(const char *pszURL, CHXString& fullPathName)
{
#ifdef _MAC_CFM
	return HFSPathFromURL(pszURL, fullPathName);
#else
	return POSIXPathFromURL(pszURL, fullPathName);
#endif
}

OSErr URLFromPath(const char *pszPath, CHXString& strURL)
{
#ifdef _MAC_CFM
	return URLFromHFSPath(pszPath, strURL);
#else
	return URLFromPOSIXPath(pszPath, strURL);
#endif
}

OSErr HFSPathFromFSRef(const FSRef *pRef, CHXString& fullPathName)
{
	OSErr err;
	
	err = PathFromFSRefInternal(kCFURLHFSPathStyle, pRef, fullPathName);

	if (err == noErr)
	{
		// be sure that directory path names end with ':'
		OSErr err2;
		Boolean isDir;
		long *kDontWantNodeID = NULL;
		
		err2 = FSGetNodeID(pRef, kDontWantNodeID, &isDir);
		if ((err2 == noErr) && isDir)
		{
			if (fullPathName.Right(1) != ":")
			{
				fullPathName += ':';
			}
		}
	}
	
	return err;
}

OSErr FSRefFromHFSPath(const char *path, FSRef *outRef) 
{
	CHXString strPath;
	OSErr err;
	
	err = FullFromPartialHFSPath(path, strPath);
	
	// now we have a full path name to convert
	
	// GR 3/26/02
	// ick, this fails for paths on mounted volumes due to CFURL bugs, and none
	// of the alternatives I can find seem to work, either
	
	err =  FSRefFromPathInternal(kCFURLHFSPathStyle, (const char *) strPath, outRef);
	return err;
}


OSStatus POSIXPathFromFSRef (const FSRef *pRef, CHXString& fullPathName)
{
	return PathFromFSRefInternal(kCFURLPOSIXPathStyle, pRef, fullPathName);
}

OSStatus FSRefFromPosixPath (const char *path, FSRef *pRef)
{
	OSStatus err;
	CHXString strPath;

	err = FullFromPartialPOSIXPath(path, strPath);

	return FSRefFromPathInternal(kCFURLPOSIXPathStyle, path, pRef);
}

// url <--> path routines

OSStatus URLFromPOSIXPath(const char *pszPath, CHXString& strURL)
{
	OSStatus err;
	CHXString strPath;

	err = FullFromPartialPOSIXPath(pszPath, strPath);

	return URLFromPathInternal(kCFURLPOSIXPathStyle, pszPath, strURL);
}

OSStatus POSIXPathFromURL(const char *pszURL, CHXString& fullPathName)
{
	return PathFromURLInternal(kCFURLPOSIXPathStyle, pszURL, fullPathName);
}

OSStatus URLFromHFSPath(const char *pszPath, CHXString& strURL)
{
	OSStatus err;
	CHXString strPath;

	err = FullFromPartialHFSPath(pszPath, strPath);

	return URLFromPathInternal(kCFURLHFSPathStyle, pszPath, strURL);
}

OSStatus HFSPathFromURL(const char *pszURL, CHXString& fullPathName)
{
	return PathFromURLInternal(kCFURLHFSPathStyle, pszURL, fullPathName);
}

OSErr FullFromPartialHFSPath(const char *pszPartialOrFullPath, CHXString& fullPathName)
{
	// icky; hopefully no one will ever pass in a file name or partial path
	//
	// If there's no colon in the HFS path, or it begins with a colon,
	// we assume it's a local file name or partial path and use the
	// current directory to make a full path
	
	CHXString strTemp;
	OSErr err;
	
	err = noErr;
	strTemp = pszPartialOrFullPath;
	
	INT32 colonOffset = strTemp.Find(':');
	
	if (colonOffset == -1 || colonOffset == 0)
	{
		FSVolumeRefNum vRefNum;
		long dirID;
		
		err = HGetVol(nil, &vRefNum, &dirID);
		if (err == noErr)
		{
			err = PathNameFromDirID(dirID, vRefNum, strTemp);
			if (err == noErr)
			{
				if (colonOffset == 0)
				{
					// concatenate a partial path starting with :
					// (skip the initial colon, though)
					strTemp += &pszPartialOrFullPath[1];
				}
				else
				{
					// concatenate just a file name
					strTemp += pszPartialOrFullPath;
				}
			}
		}
	}
	
	fullPathName = strTemp;
	
	return err;
}

OSErr FullFromPartialPOSIXPath(const char *pszPartialOrFullPath, CHXString& fullPathName)
{
	// icky; hopefully no one will ever pass in a file name or partial path
	//
	// If the path does not start with a slash,
	// we assume it's a local file name or partial path and use the
	// current directory to make a full path

	CHXString strTemp;
	OSErr err;

	err = noErr;

#if defined(_MAC_MACHO)
	if (pszPartialOrFullPath[0] != '/')
	{
		char buff[MAXPATHLEN];
		getcwd(buff, MAXPATHLEN);

		strTemp = buff;
		strTemp += (strTemp.Right(1) == "/" ? "" : "/");
		strTemp += pszPartialOrFullPath;
	}
	else
	{
		strTemp = pszPartialOrFullPath;
	}
#else
	strTemp = pszPartialOrFullPath;
#endif

	fullPathName = strTemp;

	return err;
}

#endif // defined _CARBON


#pragma mark -
#pragma mark [Pre-Carbon routines]
#pragma mark -

OSErr PathNameFromDirID(long dirID, short vRefNum, CHXString &fullPathName)
{
#if defined(_MAC_UNIX) || defined(_CARBON)
	FSRef fsref;
	OSErr err;
	
	// get the FSRef for this directory
	
	err = FSMakeFSRef(vRefNum, dirID, NULL, &fsref); // MoreFilesX passes null for the name, so we can, too
	if (err == noErr)
	{
		// get the path name from the FSRef
		err = HFSPathFromFSRef(&fsref, fullPathName);
	}
	
	return err;
	
#else // !defined _CARBON

	DirInfo	block;
	Str63	directoryName;
	OSErr	err = noErr;

	fullPathName.Empty();

	block.ioDrParID = dirID;
	block.ioNamePtr = directoryName;
	do {
		block.ioVRefNum = vRefNum;
		block.ioFDirIndex = -1;
		block.ioDrDirID = block.ioDrParID;
		
		err = PBGetCatInfoSync ((CInfoPBPtr) &block);
		if (noErr != err) return (err);
		
		directoryName[++directoryName[0]] = ':'; // append a colon
		
		if (noErr != err) return (err);

		fullPathName.InsertFromStr255(directoryName);
		
	} while (block.ioDrDirID != fsRtDirID);
	
	return (err);
	
#endif // !defined _CARBON
}

/*
PathNameFromWD

*/
OSErr PathNameFromWD(long vRefNum, CHXString &pathName)
{
	// Working Directories are obsolete under System 7 and later
	//
	// We shouldn't be calling this routine
	
	HX_ASSERT(FALSE);
	return paramErr;
}

/*
PathNameFromFSSpec
*/
OSErr PathNameFromFSSpec (const FSSpec* fsSpec, CHXString &fullPathName)
	
{
	OSErr	err;
	short	leafNameLength;
	
	if (fsSpec->parID == fsRtParID)
	{
		// parID of 1 means the name is a volume name
		//
		// just add a colon to the end to make the path (like "Hard Disk:")
		
		fullPathName.SetFromStr255(fsSpec->name);
		if (fullPathName.GetAt(fullPathName.GetLength() - 1) != ':')
		{
			fullPathName += ":";
		}
		return noErr;
	}

	// generate the path up to the parent directory
	err = PathNameFromDirID (fsSpec->parID, fsSpec->vRefNum, fullPathName);
	if (err) return err;

	// add the leaf item's name
	leafNameLength = fsSpec->name[0];
	if (leafNameLength)
	{
		fullPathName.AppendFromStr255(fsSpec->name);
		
		// if the leaf item doesn't end in a colon, it might
		// or might not be a directory.  Let's check, and add a colon if
		// necessary
		
		if (fsSpec->name[leafNameLength] != ':')
		{
			CInfoPBRec	catInfo;
			Str63		name;
			
			// copy the name to avoid changing the input parameters
			BlockMoveData(fsSpec->name, name, 1 + leafNameLength);
			
			catInfo.hFileInfo.ioVRefNum = fsSpec->vRefNum;
			catInfo.hFileInfo.ioDirID = fsSpec->parID;
			catInfo.hFileInfo.ioNamePtr = name;
			catInfo.hFileInfo.ioFDirIndex = 0;	// use name and parID
			
			// we might get an error from PBGetCatInfo if the
			// leaf item doesn't actually exist
			err = PBGetCatInfoSync(&catInfo);
			if (err == noErr
				&& (catInfo.hFileInfo.ioFlAttrib & ioDirMask) != 0)
			{
				// the leaf item is a directory
				fullPathName += ":";
			}
		}
	}	
	
	return noErr;
} 

// FSSpecFromPathName converts a full pathname (in a null-terminated
// string) to a Mac file spec.
// 
// The fields of spec are set to zero if the conversion fails.
// This routine returns fnfErr if the conversion succeeded but
// the file doesn't (yet) exist.

OSErr FSSpecFromPathName(const char *path, FSSpecPtr spec) 
{
	// revised to remove 255 character limit length path names  
	
	UInt32 len;
	
	OSErr		err;
	FSSpec		fileSpec;
	
	fileSpec.vRefNum = 0;
	fileSpec.parID = 0;
	fileSpec.name[0] = 0;
	
	len = strlen(path);
	
	if (len == 0)
	{
		// empty path... leave the fileSpec invalid
		err = nsvErr;
	}
	else if (len <= 255)
	{
		// path is under 256 characters, so let FSMakeFSSpec make the path for us
		//
		// Warning: passing zeros as vRefNum and dirID and a *partial* pathname
		// is interpreted as being relative to the poorly-defined "current" directory.
		// This conversion should really only be used for full paths, starting
		// from the drive name.
		
		Str255	name;
		
		name[0] = len;
		BlockMoveData(path, &name[1], len);
		err = FSMakeFSSpec(0, 0, name, &fileSpec);
	}
	else {
	
		Boolean		wasChanged;
		Str32		nullString;
		AliasHandle	alias;

		// longer paths, let the alias manager make the file spec for us
		//
		// The alias manager expects a full pathname, so this won't
		// work on partial paths longer than 255 characters.
		
		HX_ASSERT(path[0] != ':');	// be sure it's not a partial path
		
		nullString[0] = 0;	// null string to indicate no zone or server name 
		err  = NewAliasMinimalFromFullPath(len, path, nullString, nullString, &alias);
		if (err == noErr)
		{
			// Let the Alias Manager resolve the alias.
			err = ResolveAlias(NULL, alias, &fileSpec, &wasChanged);
			
			// ResolveAlias returns fnfErr if the target doesn't exist but
			// the fileSpec is valid, just like FSMakeFSSpec does
			
			DisposeHandle((Handle) alias);	// we don't really need the alias 
		}
	}
	
	*spec = fileSpec;
	
	return err;
}

#ifdef __cplusplus
}
#endif 
