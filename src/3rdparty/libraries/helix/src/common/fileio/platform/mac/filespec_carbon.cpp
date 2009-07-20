/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespec_carbon.cpp,v 1.8 2005/03/14 19:36:28 bobclark Exp $
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

#include "hxstring.h"
#include "platform/mac/fullpathname.h"

#ifndef _MAC_UNIX
#include "platform/mac/maclibrary.h"		// for ResolveIndependentPath
#endif

#include "platform/mac/hx_moreprocesses.h"	// for GetCurrentAppSpec

#include "platform/mac/cfwrappers.h"
#include "platform/mac/MoreFilesX.h"

#define kExtensionSeparator '.'

inline CFStringEncoding GetStandardPathEncodingForOS()
{
	// Mach-O path elements are always UTF 8; CFM path elements we
	// system double-byte encoded
#ifdef _MAC_CFM
	return CFStringGetSystemEncoding();
#else
	return kCFStringEncodingUTF8;
#endif

}


// ------------------------------------------------------------------------------------
//
// CHXMacInternalSpec
//
// ------------------------------------------------------------------------------------
CHXMacInternalSpec::CHXMacInternalSpec()
{
	InitSpec();
	ClearSpec();
}
// ------------------------------------------------------------------------------------
CHXMacInternalSpec::CHXMacInternalSpec(const FSRef& ref)
{
	InitSpec();
	
	(void) SetFromFSRef(ref);
}
// ------------------------------------------------------------------------------------
CHXMacInternalSpec::CHXMacInternalSpec(const FSSpec& spec)
{
	InitSpec();

	(void) SetFromFSSpec(spec);
}
// ------------------------------------------------------------------------------------
CHXMacInternalSpec::CHXMacInternalSpec(AliasHandle alias)
{
	InitSpec();

	(void) SetFromAlias(alias);
}
// ------------------------------------------------------------------------------------
CHXMacInternalSpec::CHXMacInternalSpec(const char* psz)
{
	InitSpec();
	
	(void) SetFromPath(psz);
	
}
// ------------------------------------------------------------------------------------
CHXMacInternalSpec::~CHXMacInternalSpec()
{
	ClearSpec();
}
// ------------------------------------------------------------------------------------
void CHXMacInternalSpec::InitSpec()
{
	mParentAndLeaf.mpLeafName = NULL;
	
}
// ------------------------------------------------------------------------------------
void CHXMacInternalSpec::ClearSpec()
{
	ZeroInit(&mItemRef);
	HX_DELETE(mParentAndLeaf.mpLeafName);
	mbRefSet = false;
	mbParentAndLeafSet = false;
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXMacInternalSpec::AllocateLeafName()
{
	if (mParentAndLeaf.mpLeafName == NULL)
	{
		mParentAndLeaf.mpLeafName = new HFSUniStr255;
		check_nonnull(mParentAndLeaf.mpLeafName);
	}
	
	if (mParentAndLeaf.mpLeafName)
	{
		ZeroInit(mParentAndLeaf.mpLeafName);
		return HXR_OK;
	}
	
	return HXR_FAIL;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::SetFromFSRef(const FSRef& ref)
{
	OSErr err;
	
	ClearSpec();

	err = FSRefValid(&ref) ? noErr : paramErr;
	check_noerr(err);
	
	if (err == noErr)
	{
		mItemRef = ref;
		mbRefSet = true;
	}
	
	UpdateDebugOnlyPath();
	
	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::SetFromPath(const char *pszPath)
{
#ifdef _MAC_CFM
	return SetFromHFSPath(pszPath);
#else
	return SetFromPOSIXPath(pszPath);
#endif
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXMacInternalSpec::SetFromHFSPath(const char *pszPath)
{
	OSErr err;
	
	ClearSpec();
	
	if (pszPath && strlen(pszPath)) 
	{		
		FSRef ref;
		
		err = FSRefFromHFSPath(pszPath, &ref);
		if (err == noErr)
		{
			err = SetFromFSRef(ref);
		}
		
		if (err != noErr)
		{
			// we couldn't set it that way; try to make
			// an FSRef for the portion excluding the leaf name
			CHXString strPath, strParent, strLeaf;
			
			(void) FullFromPartialHFSPath(pszPath, strPath);

			SplitPath((const char *) strPath, ':', strParent, strLeaf);
			
			if (strParent.IsEmpty())
			{
				// there's only a leaf; we can't deal with it since we can't
				// make an FSRef for it
				err = paramErr;
			}
			else
			{
				err = FSRefFromHFSPath((const char *) strParent, &ref);
				if (err == noErr)
				{
					err = SetFromParentAndLeaf(ref, (const char *) strLeaf,
								CFStringGetSystemEncoding());
				}
			}
			
			
		}

	}
	else
	{
		err = paramErr;
	}

	UpdateDebugOnlyPath();	// for debugging

	return err;

}

// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::SetFromParentAndLeaf(const FSRef& parentRef, const char * pszName,
											 CFStringEncoding encoding)
{
	HFSUniStr255 leafName;
	OSErr err;
	CHXCFString cfs(pszName, encoding);

	ClearSpec();
	
	if (cfs.IsSet())
	{
		leafName = cfs;
		
		err = SetFromParentAndLeafHFSUni(parentRef, leafName);
	}
	else
	{
		err = paramErr;
	}
	
	return err;

}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::SetFromParentAndLeafHFSUni(const FSRef& parentRef, const HFSUniStr255& leafName)
{
	OSErr err;
	
	ClearSpec();
	
	// try to make a valid ref directly
	
	err = FSMakeFSRefUnicode(&parentRef, leafName.length, leafName.unicode, kTextEncodingUnknown,
		&mItemRef);
	if (err == noErr)
	{
		// we have a good ref
		mbRefSet = true;
	}
	else
	{
		// we can't make a good direct ref, so be sure the parent ref is valid
		// and store the leaf name
		
		if (FSRefValid(&parentRef) && SUCCEEDED(AllocateLeafName()))
		{
			mParentAndLeaf.mItemParentRef = parentRef;
			*mParentAndLeaf.mpLeafName = leafName;
			
			mbParentAndLeafSet = true;
			
			err = noErr;
		}
		else
		{
			check(!"Cannot set from parent and leaf");
		}
	}
	
	UpdateDebugOnlyPath();

	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::SetFromFSSpec(const FSSpec& spec)
{
	OSErr err;
	
	ClearSpec();
	
	if (spec.vRefNum != 0)
	{
		err = FSpMakeFSRef(&spec, &mItemRef);
		
		if (err == noErr)
		{
			// the item exists and we have a good ref
			mbRefSet = true;
		}
		else
		{
			// if the item doesn't exist, we need to make a spec for the parent
			// and use the leaf name
			FSRef parentRef;
			
			// make a ref for the parent directory
			err = FSMakeFSRef(spec.vRefNum, spec.parID, "\p", &parentRef);
			check_noerr(err);
			
			if (err == noErr)
			{
				CHXString strName;
				
				strName.SetFromStr255(spec.name);
				
				err = SetFromParentAndLeaf(parentRef, (const char *) strName, CFStringGetSystemEncoding());
				
			}
		}
	}
	else
	{
		err = paramErr;
	}
	
	UpdateDebugOnlyPath();

	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::SetFromAlias(AliasHandle alias)
{
	OSErr err;
	
	ClearSpec();

	if (alias) 
	{
		Boolean changed;
		
		err = FSResolveAliasWithMountFlags(NULL, alias,
				&mItemRef, &changed, kResolveAliasFileNoUI);
		if (err == noErr)
		{
			mbRefSet = true;
		}
		else
		{
			// the item may not exist; try to make an FSSpec
			// from the alias and set from that
			FSSpec fsspec;
			
			err = ResolveAliasWithMountFlags(NULL, alias,
				&fsspec, &changed, kResolveAliasFileNoUI);
			if (err == noErr || err == fnfErr)
			{
				SetFromFSSpec(fsspec);
			}
		}
	} 

	UpdateDebugOnlyPath();	// for debugging
	
	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::SetFromURL(const char *pBuffer)
{
	Boolean bSuccess;
	CHXCFURL cfurl;
	FSRef ref;
	OSErr err;

	// unset our specifier in case we fail
	ClearSpec();

	require_quiet(pBuffer != NULL && *pBuffer != '\0', CantMakeURL);
	require_quiet(0 == ::strnicmp("file:", pBuffer, 5), CantMakeURL);

	// make a CFURL from which we can get an FSRef
	cfurl = pBuffer;
	require(cfurl.IsSet(), CantMakeURL);

	bSuccess = ::CFURLGetFSRef(cfurl, &ref);
	if (bSuccess)
	{
		err = SetFromFSRef(ref);
	}
	else
	{
		// in case item doesn't exist, try using a pathname
		CHXString strPath;

		if (noErr == PathFromURL(pBuffer, strPath))
		{
			err = SetFromPath((const char *) strPath);
		}
	}

CantMakeURL:
		return IsSet() ? HXR_OK : HXR_FAILED;

}
// ------------------------------------------------------------------------------------
void CHXMacInternalSpec::SplitPath(const char *pPath, char separator, CHXString& outParent, CHXString& outLeaf)
{
	CHXString strFull(pPath);
	INT32 offset;
		
	// remove any trailing separator, in case this is a directory
	if (strFull.Right(1) == separator)
	{
		strFull = strFull.Left(strFull.GetLength() - 1);
	}
	
	offset = strFull.ReverseFind(separator);
	if (offset != -1)
	{
		outParent = strFull.Left(offset + 1); // +1 includes the separator
		outLeaf = strFull.Mid(offset + 1);
	}
	else
	{
		outLeaf = strFull;
	}
}
// ------------------------------------------------------------------------------------
CHXMacInternalSpec &CHXMacInternalSpec::operator=(CHXMacInternalSpec& other)
{
	CopyInternalSpec(other);
	return *this;
}
// ------------------------------------------------------------------------------------
HXBOOL CHXMacInternalSpec::operator==(const CHXMacInternalSpec &other)
{
	OSErr err;
	HXBOOL bSame = FALSE;
	
	if (mbRefSet && other.mbRefSet)
	{
		err = FSCompareFSRefs(&mItemRef, &other.mItemRef);
		if (err == noErr)
		{
			bSame = TRUE;
		}
	}
	else if (mbParentAndLeafSet && other.mbParentAndLeafSet)
	{
		err = FSCompareFSRefs(&mParentAndLeaf.mItemParentRef, &other.mParentAndLeaf.mItemParentRef);
		if (err == noErr)
		{
			if (mParentAndLeaf.mpLeafName->length == other.mParentAndLeaf.mpLeafName->length)
			{
				if (0 == memcmp(&mParentAndLeaf.mpLeafName->unicode, &other.mParentAndLeaf.mpLeafName->unicode,
					mParentAndLeaf.mpLeafName->length * sizeof(UniChar)))
				{
					bSame = TRUE;
				}
			}
		}
	}
	
	return bSame;
}
// ------------------------------------------------------------------------------------
HXBOOL CHXMacInternalSpec::operator!=(const CHXMacInternalSpec &other)
{
	return !(*this == other);
}
// ------------------------------------------------------------------------------------
HXBOOL CHXMacInternalSpec::IsSet() const	// object has been set to a file (which may or may not exist)
{
	return (mbRefSet || mbParentAndLeafSet);
}
// ------------------------------------------------------------------------------------
void CHXMacInternalSpec::Unset()
{
	ClearSpec();
}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::GetPersistentString() const
{
	FSRef ref;
	OSErr err;
	CHXString strPersistent;
	
	err = GetFSRef(ref);
	if (err == noErr)
	{
		strPersistent = MakePersistentStringForRef(NULL, ref);
	}
	return strPersistent;
	
}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::GetRelativePersistentString(const CHXMacInternalSpec& fromFile) const
{
	CHXString strResult;
	FSRef fromFileRef, targetRef;
	OSErr err;
	
	err = GetFSRef(targetRef);
	require_noerr(err, CantGetTargetRef);
	
	err = fromFile.GetFSRef(fromFileRef);
	require_noerr(err, CantGetFromFileRef);
	
	strResult = MakePersistentStringForRef(&fromFileRef, targetRef);
	
	return strResult;

CantGetFromFileRef:
CantGetTargetRef:
	return "";
	
}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::MakePersistentStringForRef(const FSRef *pAnchorRef, const FSRef& targetRef)
{
	HX_RESULT	err;
	CHXString	str;
	char*		pBuff;
	Handle		hHexBuffer;
	Size		sHexBuffSize;
	AliasHandle	hAlias;
	
	// be sure we have a target to specify
	
	err = FSNewAlias(pAnchorRef, &targetRef, &hAlias);
	require_noerr(err, CantMakeAlias);
	
	// fill a temporary buffer with hex of the alias
	hHexBuffer = NewHandle(0);

	err = StuffHexHandle((Handle) hAlias, hHexBuffer);
	require_noerr(err, CantStuff);
	
	// copy our hex buffer to the string
	sHexBuffSize = GetHandleSize(hHexBuffer);

	pBuff = str.GetBuffer(sHexBuffSize + 1); // leave space for terminating null
	BlockMoveData(*hHexBuffer, pBuff, sHexBuffSize);
	pBuff[sHexBuffSize] = 0;	// make it a C-string
	str.ReleaseBuffer();
	
	DisposeHandle(hHexBuffer);
	DisposeHandle((Handle) hAlias);
	
#ifdef _DEBUG
	{
		// during debugging, we'll immediately check that the persistent specifier
		// points to the same place as the actual target

		CHXMacInternalSpec tempAliasTarget;
		
		tempAliasTarget.SetFromPersistentString(str);
		
		if (noErr != FSCompareFSRefs(&targetRef, &tempAliasTarget.mItemRef))
		{
			check(!"Persistent alias doesn't self-resolve");
		}
	}
#endif
	
	return str;

	// error handling

CantStuff:
	DisposeHandle(hHexBuffer);
	DisposeHandle((Handle) hAlias);
CantMakeAlias:
	str = "";
	return str;
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXMacInternalSpec::SetFromPersistentString(const char *pBuffer)
{
	HX_RESULT	err;
	Handle		hHexBuffer;
	Size		sBufferSize;
	AliasHandle	hAlias;
	
	check_nonnull(pBuffer);
	
	sBufferSize = strlen(pBuffer);
	
	// handle empty buffers
	if (sBufferSize == 0)
	{
		ClearSpec();
		return HXR_OK;
	}
	
	//  allocate an empty handle to fill
	hAlias = (AliasHandle) NewHandle(0);
	
	// make a temporary buffer with the hex passed in
	err = PtrToHand(pBuffer, &hHexBuffer, sBufferSize);
	require_noerr(err, CantAllocateHexBuffer);
	
	// turn the hex into our binary alias
	err = UnstuffHexHandle(hHexBuffer, (Handle) hAlias);
	DisposeHandle(hHexBuffer);
	
	if (err == noErr)
	{
		SetFromAlias(hAlias);
	}
	else
	{
		// we couldn't interpret it as hex; try it as a full path
		CHXString 	path(pBuffer);
		
		// resolve ÇfoldÈ specifiers... archaic
		//ResolveIndependentPath(path);
		
		// make a reference from the path
		err = SetFromPath((const char *) path);
		require_noerr(err, CantParse);
	}
	
	
	
	// discard the temp buffer
	
	DisposeHandle((Handle) hAlias);

	UpdateDebugOnlyPath();	// for debugging
	
	return HXR_OK;
	
	
	// error handling -- reverse-order cleanup
CantParse:
CantAllocateHexBuffer:
	DisposeHandle((Handle) hAlias);
	
	return err;

}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::GetPathName() const
{
#ifdef _MAC_CFM
	return GetHFSPath();
#else
	return GetPOSIXPath();
#endif
}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::GetHFSPath() const
{
	CHXString	str;
	OSErr		err;

	if (mbRefSet)
	{
		err = HFSPathFromFSRef(&mItemRef, str);
	}
	else if (mbParentAndLeafSet)
	{
		err = HFSPathFromFSRef(&mParentAndLeaf.mItemParentRef, str);
		if (err == noErr)
		{
			CHXString strLeaf;

			strLeaf.SetFromHFSUniStr255(*mParentAndLeaf.mpLeafName, CFStringGetSystemEncoding());
			str += strLeaf;
		}
	}

	return str;
}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::GetPOSIXPath() const
{
	CHXString	str;
	OSErr		err;

	if (mbRefSet) 
	{
		err = POSIXPathFromFSRef(&mItemRef, str);
	}
	else if (mbParentAndLeafSet)
	{
		err = POSIXPathFromFSRef(&mParentAndLeaf.mItemParentRef, str);
		if (err == noErr)
		{
			CHXString strLeaf;
			
			strLeaf.SetFromHFSUniStr255(*mParentAndLeaf.mpLeafName, kCFStringEncodingUTF8);

			if (str.Right(1) != "/")
			{
				str += "/";	
			}
			str += strLeaf;
		}
	}

	return str;
}

HX_RESULT CHXMacInternalSpec::SetFromPOSIXPath(const char *pPosixPath)
{
	FSRef ref;
	OSStatus err;

	ClearSpec();

	err = FSRefFromPosixPath(pPosixPath, &ref);
	if (err == noErr)
	{
		err = SetFromFSRef(ref);
	}
	else
	{
		// we couldn't set it that way; try to make
		// an FSRef for the portion excluding the leaf name
		CHXString strFullPath, strParent, strLeaf;

		(void) FullFromPartialPOSIXPath(pPosixPath, strFullPath);

		SplitPath(strFullPath, '/', strParent, strLeaf);

		if (strParent.IsEmpty())
		{
			// there's only a leaf; we can't deal with it since we can't
			// make an FSRef for it
			err = paramErr;
		}
		else
		{
			err = FSRefFromPosixPath((const char *) strParent, &ref);
			if (err == noErr)
			{
				err = SetFromParentAndLeaf(ref, (const char *) strLeaf, kCFStringEncodingUTF8);
			}
		}
	}
	
	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::GetParentDirectoryRef(FSRef& outParentRef) const
{
	OSErr	err;

	if (mbRefSet) 
	{
		err = FSGetParentRef(&mItemRef, &outParentRef);
	}
	else if (mbParentAndLeafSet)
	{
		outParentRef = mParentAndLeaf.mItemParentRef;
		err = noErr;
	}
	else
	{
		err = paramErr;
	}
	
	if (err != noErr)
	{
		ZeroInit(&outParentRef);
	}
	
	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::GetVolumeRef(FSRef& outVolumeRef) const
{
	OSErr		err;
	FSVolumeRefNum 	vRefNum;

	ZeroInit(&outVolumeRef);
	
	err = GetVRefNum(vRefNum);
	
	if (err == noErr)
	{
		err = FSGetVolumeInfo(vRefNum, 0, NULL, kFSVolInfoNone, NULL, NULL, &outVolumeRef);
	}
	
	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::GetHFSUniLeafName(HFSUniStr255& uniName) const
{
	OSErr err;
	
	if (mbRefSet) 
	{
		err = FSGetFileDirName(&mItemRef, &uniName);
	}
	else if (mbParentAndLeafSet)
	{
		check_nonnull(mParentAndLeaf.mpLeafName);
		
		uniName = *mParentAndLeaf.mpLeafName;
		err = noErr;
	}
	else
	{
		err = paramErr;
	}
	return err;
}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::GetName() const
{
	CHXString strName;
	HFSUniStr255 uniName;
	OSErr err;
	
	err = GetHFSUniLeafName(uniName);
	if (err == noErr)
	{
		CHXCFString cfsName(uniName);
	
		strName = cfsName;
	}

	return strName;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::GetFSSpec(FSSpec& outSpec) const
{
	OSErr err;
	
	ZeroInit(&outSpec);
	
	if (mbRefSet) 
	{
		// make an FSSpec directly
		err = FSRefMakeFSSpec(&mItemRef, &outSpec);
	}
	else if (mbParentAndLeafSet)
	{
		// make an FSSpec from the parID, vRefNum, and name
		long nodeID;
		Boolean isDir;
		FSVolumeRefNum 	vRefNum;
		
		err = FSGetNodeID(&mParentAndLeaf.mItemParentRef, &nodeID, &isDir);
		check(isDir);
		
		if (err == noErr)
		{
			err = FSGetVRefNum(&mParentAndLeaf.mItemParentRef, &vRefNum);
		}
		if (err == noErr)
		{
			CHXString strLeaf;
			Str255 pascLeaf;
			
			strLeaf.SetFromHFSUniStr255(*mParentAndLeaf.mpLeafName, CFStringGetSystemEncoding());
			strLeaf.MakeStr255(pascLeaf);
			
			err = FSMakeFSSpec(vRefNum, nodeID, pascLeaf, &outSpec);
		}
		
	}
	
	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::GetFSRef(FSRef& outRef) const
{
	OSErr err;
	
	ZeroInit(&outRef);
	
	err = errFSBadFSRef;
	
	if (mbRefSet) 
	{
		outRef = mItemRef;
		err = noErr;
	}
	else if (mbParentAndLeafSet)
	{
		// try to make a real FSRef from the parent and leaf name
		
		check_nonnull(mParentAndLeaf.mpLeafName);
		
		err = FSMakeFSRefUnicode(&mParentAndLeaf.mItemParentRef, 
			mParentAndLeaf.mpLeafName->length, mParentAndLeaf.mpLeafName->unicode,
			kTextEncodingUnknown, &outRef);
	}
	
	return err;
}
// ------------------------------------------------------------------------------------
OSErr CHXMacInternalSpec::GetVRefNum(FSVolumeRefNum& outVRefNum) const
{
	OSErr err;
	
	err = paramErr;
	
	if (mbRefSet) 
	{
		err = FSGetVRefNum(&mItemRef, &outVRefNum);
	}
	else if (mbParentAndLeafSet)
	{
		err = FSGetVRefNum(&mParentAndLeaf.mItemParentRef, &outVRefNum);
	}
	return err;
}
// ------------------------------------------------------------------------------------
CHXString CHXMacInternalSpec::GetURL(HXBOOL isDirectorySpec) const
{
	CHXString strURL;
	CHXCFURL cfurl;
	CHXCFURL cfurlPlusLeaf;
	CFStringRef cfstr;

	if (mbRefSet)
	{
		cfurl = mItemRef;
		require(cfurl.IsSet(), CantMakeURL);
		
		cfstr = ::CFURLGetString(cfurl); // we are NOT supposed to release the CFString obtained here
		require_nonnull(cfstr, CantGetURLString);
	}
	else if (mbParentAndLeafSet)
	{
		cfurl = mParentAndLeaf.mItemParentRef;
		require(cfurl.IsSet(), CantMakeURL);

		// we have the FSRef of the parent, so we need to append the leaf name
		cfurlPlusLeaf = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, cfurl,
							CHXCFString(*mParentAndLeaf.mpLeafName), isDirectorySpec);
		cfstr = ::CFURLGetString(cfurlPlusLeaf); // we are NOT supposed to release the CFString obtained here
		require_nonnull(cfstr, CantGetURLString);
	}
	
	strURL = cfstr;

CantGetURLString:
CantMakeURL:
	return strURL;
}
// ------------------------------------------------------------------------------------
void CHXMacInternalSpec::CopyInternalSpec(const CHXMacInternalSpec& otherToBeCopied)
{
	ClearSpec();
	
	if (otherToBeCopied.mbRefSet)
	{
		mItemRef = otherToBeCopied.mItemRef;
		mbRefSet = otherToBeCopied.mbRefSet;
	}
	
	if (otherToBeCopied.mbParentAndLeafSet)
	{
		if (SUCCEEDED(AllocateLeafName()))
		{
			mParentAndLeaf.mItemParentRef = otherToBeCopied.mParentAndLeaf.mItemParentRef;
			
			*mParentAndLeaf.mpLeafName = *otherToBeCopied.mParentAndLeaf.mpLeafName;
			
			mbParentAndLeafSet = otherToBeCopied.mbParentAndLeafSet;
		}
	}
	
	check((!mbRefSet) || (!mbParentAndLeafSet));
	
	UpdateDebugOnlyPath();
}

// ------------------------------------------------------------------------------------
// StuffHexHandle stuffs the data in the supplied handle into the other handle as hex text

HX_RESULT CHXMacInternalSpec::StuffHexHandle(Handle hData, Handle hHex)
{
	OSErr	err;
	UInt32	i;
	UInt8	*pHex;
	UInt8	*pBuff;
	Size	sDataSize;
	
	static char hex[] = "0123456789ABCDEF";
	
	require_action(hData != NULL, fail, err = HXR_NOT_INITIALIZED);
	require_action(hHex != NULL, fail, err = HXR_NOT_INITIALIZED);
	
	sDataSize = GetHandleSize(hData);
	
	SetHandleSize(hHex, sDataSize * 2);
	err = MemError();
	require_noerr(err, fail);
	
	pHex = (UInt8 *) *hHex;
	pBuff = (UInt8 *) *hData;

	for (i = 0; i < sDataSize; i++)
	{
		*pHex++ = hex[(*pBuff) >> 4];
		*pHex++ = hex[(*pBuff) & 0x0F];
		pBuff++;
	}
	
fail:
	return err;
	
}

// ------------------------------------------------------------------------------------
// UnstuffHexHandle moves the ascii hex in the handle into the buffer as binary

HX_RESULT CHXMacInternalSpec::UnstuffHexHandle(Handle hHex, Handle hData)
{
	HX_RESULT	err;
	UInt32		i;
	UInt8		*pHex;
	UInt8		*pBuff;
	Size		sHexSize;
	Size		sDataSize;
	
	require_action(hData != NULL, fail, err = HXR_NOT_INITIALIZED);
	require_action(hHex != NULL, fail, err = HXR_NOT_INITIALIZED);
	
	sHexSize = GetHandleSize(hHex);
	
	// be sure there's some hex, and it's an even number of characters
	require_action_quiet(sHexSize > 0 && (sHexSize & 1) == 0, fail, err = HXR_NOT_INITIALIZED);
	
	sDataSize = sHexSize / 2;
	SetHandleSize(hData, sDataSize);
	err = MemError();
	require_noerr(err, fail);
	
	pHex = (UInt8 *) *hHex;
	pBuff = (UInt8 *) *hData;

	for (i = 0; i < sDataSize; i++)
	{
		require_action_quiet(((*pHex >= '0' && *pHex <= '9') || (*pHex >= 'A' && *pHex <= 'F')),
			fail, err = HXR_UNEXPECTED);

		*pBuff = (((*pHex <= '9') ? (*pHex - '0') : (10 + *pHex - 'A'))) << 4;
		pHex++;
		
		require_action_quiet(((*pHex >= '0' && *pHex <= '9') || (*pHex >= 'A' && *pHex <= 'F')),
			fail, err = HXR_UNEXPECTED);

		*pBuff |= ((*pHex <= '9') ? (*pHex - '0') : (10 + *pHex - 'A')) ;
		pHex++;
		
		pBuff++;
		
	}
	
	err = HXR_OK;
	
fail:
	return err;
	
}


// ------------------------------------------------------------------------------------
// CHXMacInternalSpec::UpdateDebugOnlyPath
//
// UpdateDebugnlyPath does nothing in release builds.
// In debug builds, it updates m_strDebugOnlyPath to reflect
// the current reference.


#ifdef _DEBUG
void CHXMacInternalSpec::UpdateDebugOnlyPath()
{

	OSStatus	err;

	mStrDebugOnlyPath = "<unset>";

	// sanity check that at most one FSRef kind is being used
	check((!mbRefSet) || (!mbParentAndLeafSet));
	
	// make a path from the FSRef for the item or its parent
	if (mbRefSet || mbParentAndLeafSet)
	{
		err = HFSPathFromFSRef((mbRefSet ? &mItemRef : &mParentAndLeaf.mItemParentRef), mStrDebugOnlyPath);
		if (err) 
		{
			mStrDebugOnlyPath.Format("HFSPathFromFSRef OSErr = %d", err);
		}
		
		if (mbParentAndLeafSet)
		{
			// concatenate the leaf name, too
			CHXString strLeaf;
			
			check_nonnull(mParentAndLeaf.mpLeafName);
			
			strLeaf.SetFromHFSUniStr255(*mParentAndLeaf.mpLeafName, CFStringGetSystemEncoding());
			mStrDebugOnlyPath += strLeaf;
		}
	}
	
	// the raw char* pointer is more convenient to view in a debugger
	mPeekDebugOnlyPath = (char*) (const char*) mStrDebugOnlyPath;
	
}
#endif




// ------------------------------------------------------------------------------------
#pragma mark -
// ------------------------------------------------------------------------------------
//
// CHXFileSpecifier
//
// ------------------------------------------------------------------------------------
CHXFileSpecifier::CHXFileSpecifier() : m_pTempSpec(nil), m_pTempRef(nil)
{
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::CHXFileSpecifier(const FSSpec &file) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromFSSpec(file);
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::CHXFileSpecifier(AliasHandle alias) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromAlias(alias);
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::CHXFileSpecifier(const CHXString &str) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromPath((const char *) str);
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::CHXFileSpecifier(const char* psz) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromPath(psz);
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::CHXFileSpecifier(const FSRef &ref) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromFSRef(ref);
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::CHXFileSpecifier(const CHXFileSpecifier &other) : m_pTempSpec(nil), m_pTempRef(nil)
{
	*this = other;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::~CHXFileSpecifier()
{
	HX_DELETE(m_pTempSpec);
	HX_DELETE(m_pTempRef);
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::operator FSSpec() const
{
	FSSpec spec;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSSpec(spec);
	
	return spec;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::operator FSSpec*() const
{
	if (m_pTempSpec == NULL)
	{
		m_pTempSpec = new FSSpec;
		check_nonnull(m_pTempSpec);
	}
	
	if (m_pTempSpec)
	{
		OSErr err;
		
		err = m_MacInternalSpec.GetFSSpec(*m_pTempSpec);
	}
	
	return m_pTempSpec;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::operator FSRef() const
{
	FSRef ref;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSRef(ref);
	
	return ref;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier::operator FSRef*() const
{
	if (m_pTempRef == NULL)
	{
		m_pTempRef = new FSRef;
		check_nonnull(m_pTempRef);
	}
	
	if (m_pTempRef)
	{
		OSErr err;
		
		err = m_MacInternalSpec.GetFSRef(*m_pTempRef);
	}
	
	return m_pTempRef;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier &CHXFileSpecifier::operator=(const CHXFileSpecifier &other)
{
	//m_MacInternalSpec.CopyInternalSpec(other.m_MacInternalSpec);
	m_MacInternalSpec.CopyInternalSpec(other.m_MacInternalSpec);
	return *this;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier &CHXFileSpecifier::operator=(const char * other)
{
	OSErr err;
	
	err = m_MacInternalSpec.SetFromPath(other);
	
	return *this;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier &CHXFileSpecifier::operator=(const CHXString& str)
{
	OSErr err;
	
	err = m_MacInternalSpec.SetFromPath((const char *) str);

	return *this;
}
// ------------------------------------------------------------------------------------
HXBOOL CHXFileSpecifier::operator==(const CHXFileSpecifier &other)
{
	return (m_MacInternalSpec == other.m_MacInternalSpec);
}
// ------------------------------------------------------------------------------------
HXBOOL CHXFileSpecifier::operator!=(const CHXFileSpecifier &other)
{
	return (m_MacInternalSpec != other.m_MacInternalSpec);
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetPathName() const
{
	return m_MacInternalSpec.GetPathName();
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetPOSIXPath() const
{
	return m_MacInternalSpec.GetPOSIXPath();
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXFileSpecifier::SetFromPOSIXPath(const char *pPosixPath)
{
	return m_MacInternalSpec.SetFromPOSIXPath(pPosixPath);
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetHFSPath() const
{
	return m_MacInternalSpec.GetHFSPath();
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXFileSpecifier::SetFromHFSPath(const char *pHFSPath)
{
	return m_MacInternalSpec.SetFromHFSPath(pHFSPath);
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetURL() const
{
	const HXBOOL kIsNotDirectory = FALSE;
	
	return m_MacInternalSpec.GetURL(kIsNotDirectory);
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetExtension() const
{
	CHXString strExt;

	if (m_MacInternalSpec.IsSet())
	{
		CHXString strName;
		
		strName = GetName();
		
		int idx;

		idx = strName.ReverseFind(kExtensionSeparator);
		if (idx != -1 && (idx + 1) < strName.GetLength())
		{
			strExt = strName.Mid(idx + 1);
			
			// anything over 5 characters isn't an extension
			//
			// Under OS X, extensions can be any length
			/*
			if (strExt.GetLength() > 5)
			{
				strExt = "";
			}
			*/
		}
	}
	
	// TODO... utility to look up extension based on file type?
	
	return strExt;
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetTitle() const
{
	CHXString strTitle;
	
	if (m_MacInternalSpec.IsSet())
	{
		CHXString strExt;
		CHXString strName;
		
		long	nExtChars;
		
		// make an extension including the '.', and if it matches
		// the right side of the name, make a title being everything to
		// the left of that
		
		strName = GetName();
		strExt = kExtensionSeparator + GetExtension();
		nExtChars = strExt.GetLength();

		if (strName.Right(nExtChars) == strExt)
		{
			strTitle = strName.Left(strName.GetLength() - nExtChars);
		}
	}

	return strTitle;
}
// ------------------------------------------------------------------------------------
void CHXFileSpecifier::Unset()
{
	m_MacInternalSpec.Unset();
}
// ------------------------------------------------------------------------------------
HXBOOL CHXFileSpecifier::IsSet() const
{
	return m_MacInternalSpec.IsSet();
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier CHXFileSpecifier::GetParentDirectory() const
{
	CHXDirSpecifier resultSpec;
	
	if (IsSet()) 
	{
		FSRef parentRef;
		OSErr	err;
		
		err = m_MacInternalSpec.GetParentDirectoryRef(parentRef);
		if (err == noErr)
		{
			resultSpec = parentRef;
		}
	} 
	
	return resultSpec;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier CHXFileSpecifier::GetVolume() const
{
	CHXDirSpecifier resultSpec;
	
	if (IsSet()) 
	{
		FSRef volRef;
		OSErr	err;
		
		err = m_MacInternalSpec.GetVolumeRef(volRef);
		if (err == noErr)
		{
			resultSpec = volRef;
		}
	} 
	return resultSpec;
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetName() const
{
	return m_MacInternalSpec.GetName();
}
// ------------------------------------------------------------------------------------
HFSUniStr255 CHXFileSpecifier::GetNameHFSUniStr255() const
{
	OSErr err;
	HFSUniStr255 hfsUni;
	
	err = m_MacInternalSpec.GetHFSUniLeafName(hfsUni);
	if (err != noErr)
	{
		hfsUni.length = 0;
	}
	
	return hfsUni;
}
// ------------------------------------------------------------------------------------
FSVolumeRefNum CHXFileSpecifier::GetVRefNum() const
{
	FSVolumeRefNum vRefNum;
	OSErr err;
	
	err = m_MacInternalSpec.GetVRefNum(vRefNum);
	if (err != noErr)
	{
		vRefNum = 0;
	}
	
	return vRefNum;
}
// ------------------------------------------------------------------------------------
/*
CHXString CHXFileSpecifier::GetDisplayString() const
{
	CHXString strDisplay;
	
	if (IsSet()) {
		FSSpec 			spec;
		
		spec = m_macentity;
		strDisplay.SetFromStr255(spec.name);
	} 
	return strDisplay;
}
*/
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetPersistentString() const
{
	return m_MacInternalSpec.GetPersistentString();
}
// ------------------------------------------------------------------------------------
CHXString CHXFileSpecifier::GetRelativePersistentString(const CHXFileSpecifier& fromFileSpec) const
{
	return m_MacInternalSpec.GetRelativePersistentString(fromFileSpec.m_MacInternalSpec);
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXFileSpecifier::SetFromPersistentString(const char *pBuffer)
{
	return m_MacInternalSpec.SetFromPersistentString(pBuffer);
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXFileSpecifier::SetFromURL(const char *pBuffer)
{
	return m_MacInternalSpec.SetFromURL(pBuffer);
}

// ------------------------------------------------------------------------------------
#pragma mark -
// ------------------------------------------------------------------------------------
//
// CHXDirSpecifier
//
// ------------------------------------------------------------------------------------

CHXDirSpecifier::CHXDirSpecifier() : m_pTempSpec(nil), m_pTempRef(nil)
{
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::CHXDirSpecifier(const FSRef &ref) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromFSRef(ref);
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::CHXDirSpecifier(const char* psz) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromPath(psz);
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::CHXDirSpecifier(const FSSpec &folder) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromFSSpec(folder);
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::CHXDirSpecifier(AliasHandle dirAlias) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromAlias(dirAlias);
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::CHXDirSpecifier(const CHXString& str) : m_pTempSpec(nil), m_pTempRef(nil)
{
	m_MacInternalSpec.SetFromPath((const char *) str);
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::CHXDirSpecifier(const CHXDirSpecifier &other) : m_pTempSpec(nil), m_pTempRef(nil)
{
	*this = other;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::~CHXDirSpecifier()
{
	HX_DELETE(m_pTempSpec);
	HX_DELETE(m_pTempRef);
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::operator FSSpec() const
{
	FSSpec spec;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSSpec(spec);
	
	return spec;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::operator FSSpec*() const
{
	if (m_pTempSpec == NULL)
	{
		m_pTempSpec = new FSSpec;
		check_nonnull(m_pTempSpec);
	}
	
	if (m_pTempSpec)
	{
		OSErr err;
		
		err = m_MacInternalSpec.GetFSSpec(*m_pTempSpec);
	}
	
	return m_pTempSpec;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::operator FSRef() const
{
	FSRef ref;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSRef(ref);
	
	return ref;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier::operator FSRef*() const
{
	if (m_pTempRef == NULL)
	{
		m_pTempRef = new FSRef;
		check_nonnull(m_pTempRef);
	}
	
	if (m_pTempRef)
	{
		OSErr err;
		
		err = m_MacInternalSpec.GetFSRef(*m_pTempRef);
	}
	
	return m_pTempRef;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier &CHXDirSpecifier::operator=(const CHXDirSpecifier &other)
{
	m_MacInternalSpec.CopyInternalSpec(other.m_MacInternalSpec);
	return *this;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier &CHXDirSpecifier::operator=(const char *other)
{
	OSErr err;
	
	err = m_MacInternalSpec.SetFromPath(other);

	return *this;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier &CHXDirSpecifier::operator=(const CHXString& str)
{
	OSErr err;
	
	err = m_MacInternalSpec.SetFromPath((const char *) str);

	return *this;
}
// ------------------------------------------------------------------------------------
HXBOOL CHXDirSpecifier::operator==(const CHXDirSpecifier &other)
{
	return (m_MacInternalSpec == other.m_MacInternalSpec);
}
// ------------------------------------------------------------------------------------
HXBOOL CHXDirSpecifier::operator!=(const CHXDirSpecifier &other)
{
	return (m_MacInternalSpec != other.m_MacInternalSpec);
}
// ------------------------------------------------------------------------------------
CHXString CHXDirSpecifier::GetPathName() const
{
	return m_MacInternalSpec.GetPathName();
}
// ------------------------------------------------------------------------------------
CHXString CHXDirSpecifier::GetHFSPath() const
{
	return m_MacInternalSpec.GetHFSPath();
}
// ------------------------------------------------------------------------------------
CHXString CHXDirSpecifier::GetURL() const
{
	const HXBOOL kIsDirectory = TRUE;
	
	return m_MacInternalSpec.GetURL(kIsDirectory);
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXDirSpecifier::SetFromHFSPath(const char *pHFSPath)
{
	return m_MacInternalSpec.SetFromHFSPath(pHFSPath);
}
// ------------------------------------------------------------------------------------
CHXString CHXDirSpecifier::GetPOSIXPath() const
{
	return m_MacInternalSpec.GetPOSIXPath();
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXDirSpecifier::SetFromPOSIXPath(const char *pPosixPath)
{
	return m_MacInternalSpec.SetFromPOSIXPath(pPosixPath);
}
// ------------------------------------------------------------------------------------
void CHXDirSpecifier::Unset()
{
	m_MacInternalSpec.Unset();
}
// ------------------------------------------------------------------------------------
HXBOOL CHXDirSpecifier::IsSet() const
{
	return m_MacInternalSpec.IsSet();
}
// ------------------------------------------------------------------------------------
HXBOOL CHXDirSpecifier::IsVolume() const
{
	if (IsSet()) 
	{
		// Is there a better test?
		
		return (GetDirID() == fsRtDirID);
		
	}
	return FALSE;
}
// ------------------------------------------------------------------------------------
CHXString CHXDirSpecifier::GetName() const
{
	return m_MacInternalSpec.GetName();
}
// ------------------------------------------------------------------------------------
HFSUniStr255 CHXDirSpecifier::GetNameHFSUniStr255() const
{
	OSErr err;
	HFSUniStr255 hfsUni;
	
	err = m_MacInternalSpec.GetHFSUniLeafName(hfsUni);
	if (err != noErr)
	{
		hfsUni.length = 0;
	}
	
	return hfsUni;
}
// ------------------------------------------------------------------------------------
FSVolumeRefNum CHXDirSpecifier::GetVRefNum() const
{
	FSVolumeRefNum vRefNum;
	OSErr err;
	
	err = m_MacInternalSpec.GetVRefNum(vRefNum);
	if (err != noErr)
	{
		vRefNum = 0;
	}
	
	return vRefNum;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier CHXDirSpecifier::GetParentDirectory() const
{
	CHXDirSpecifier resultSpec;
	
	if (IsSet()) 
	{
		FSRef parentRef;
		OSErr	err;
		
		err = m_MacInternalSpec.GetParentDirectoryRef(parentRef);
		if (err == noErr)
		{
			resultSpec = parentRef;
		}
	} 
	
	return resultSpec;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier CHXDirSpecifier::GetVolume() const
{
	CHXDirSpecifier resultSpec;
	
	if (IsSet()) 
	{
		FSRef volRef;
		OSErr	err;
		
		err = m_MacInternalSpec.GetVolumeRef(volRef);
		if (err == noErr)
		{
			resultSpec = volRef;
		}
	} 
	return resultSpec;
	
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier CHXDirSpecifier::SpecifyChildFile(const char *child) const
{
	CHXFileSpecifier resultSpec;
	FSRef currRef;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSRef(currRef);
	require_noerr(err, CantGetFSRef);

	err = resultSpec.m_MacInternalSpec.SetFromParentAndLeaf(currRef, child,
														 GetStandardPathEncodingForOS());
	require_noerr(err, CantSetSpec);
	
CantSetSpec:
CantGetFSRef:
	return resultSpec;
	
}

// ------------------------------------------------------------------------------------
CHXFileSpecifier CHXDirSpecifier::SpecifyChildFileHFSUniStr255(HFSUniStr255 hfsUni) const
{
	CHXFileSpecifier resultSpec;
	FSRef currRef;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSRef(currRef);
	require_noerr(err, CantGetFSRef);
	
	err = resultSpec.m_MacInternalSpec.SetFromParentAndLeafHFSUni(currRef, hfsUni);
	require_noerr(err, CantSetSpec);
	
CantSetSpec:
CantGetFSRef:
	return resultSpec;
}
// ------------------------------------------------------------------------------------
CHXFileSpecifier CHXDirSpecifier::SpecifyChildFileStr255(ConstStr255Param child) const
{
	CHXString str;
	
	str.SetFromStr255(child);
	
	return SpecifyChildFile((const char *) str);
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier CHXDirSpecifier::SpecifyChildDirectory(const char *child) const
{
	CHXDirSpecifier resultSpec;
	FSRef currRef;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSRef(currRef);
	require_noerr(err, CantGetFSRef);
	
	err = resultSpec.m_MacInternalSpec.SetFromParentAndLeaf(currRef, child,
														 GetStandardPathEncodingForOS());
	require_noerr(err, CantSetSpec);
	
CantSetSpec:
CantGetFSRef:
	return resultSpec;

}
// ------------------------------------------------------------------------------------
CHXDirSpecifier CHXDirSpecifier::SpecifyChildDirectoryHFSUniStr255(HFSUniStr255 hfsUni) const
{
	CHXDirSpecifier resultSpec;
	FSRef currRef;
	OSErr err;
	
	err = m_MacInternalSpec.GetFSRef(currRef);
	require_noerr(err, CantGetFSRef);
	
	err = resultSpec.m_MacInternalSpec.SetFromParentAndLeafHFSUni(currRef, hfsUni);
	require_noerr(err, CantSetSpec);
	
	
CantSetSpec:
CantGetFSRef:
	return resultSpec;
}
// ------------------------------------------------------------------------------------
CHXDirSpecifier CHXDirSpecifier::SpecifyChildDirectoryStr255(ConstStr255Param child) const
{
	CHXString str;
	
	str.SetFromStr255(child);
	return SpecifyChildDirectory((const char *) str);
}
// ------------------------------------------------------------------------------------
/*
CHXString CHXDirSpecifier::GetDisplayString() const
{
	CHXString strDisplay;
	
	if (IsSet()) 
	{
		FSSpec 			spec;
		
		spec = m_macentity;	// coerce to FSSpec
		
		strDisplay.SetFromStr255(spec.name);
	}
	return strDisplay;
	
}
*/
// ------------------------------------------------------------------------------------
CHXString CHXDirSpecifier::GetPersistentString() const
{
	return m_MacInternalSpec.GetPersistentString();
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXDirSpecifier::SetFromPersistentString(const char *pBuffer)
{
	return m_MacInternalSpec.SetFromPersistentString(pBuffer);
}
// ------------------------------------------------------------------------------------
HX_RESULT CHXDirSpecifier::SetFromURL(const char *pBuffer)
{
	return m_MacInternalSpec.SetFromURL(pBuffer);
}

// ------------------------------------------------------------------------------------
long CHXDirSpecifier::GetDirID() const
{
	OSErr	err;
	FSRef	fsRef;
	long	parID;
	
	FSCatalogInfo catInfo;
	
	const FSCatalogInfoBitmap whichInfo = kFSCatInfoNodeID | kFSCatInfoNodeFlags;
	
	HFSUniStr255 * kDontWantName = NULL;
	FSSpec * kDontWantFSSpec = NULL;
	FSRef * kDontWantParentRef = NULL;
		
	require(IsSet(), NotSet);

	err = m_MacInternalSpec.GetFSRef(fsRef);
	require_noerr(err, CantGetFSRef);
	
	err = ::FSGetCatalogInfo(&fsRef, whichInfo, &catInfo,
		kDontWantName, kDontWantFSSpec, kDontWantParentRef);
 	
	require_noerr(err, CantGetDirID);
	
	parID = catInfo.nodeID;
	check((catInfo.nodeFlags & kFSNodeIsDirectoryMask) != 0);
	
	return parID;
	
CantGetDirID:
CantGetFSRef:
NotSet:
	return 0;
	
}
// ------------------------------------------------------------------------------------
