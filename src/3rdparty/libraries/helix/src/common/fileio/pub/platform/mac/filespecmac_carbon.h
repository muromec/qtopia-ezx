/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecmac_carbon.h,v 1.7 2007/07/06 20:35:19 jfinnecy Exp $
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

#ifndef FILESPECMAC_CARBON_H
#define FILESPECMAC_CARBON_H

#include "hxstring.h"

// CHXMacInternalSpec is the internal component of CHXFileSpecifier and CHXDirSpecifier
// on the Mac. 


class CHXMacInternalSpec
{
private:
	Boolean			mbRefSet;
	Boolean 		mbParentAndLeafSet;
	union
	{
		// if the mItemRef is valid, mbRefSet is true;
		// if the mItemParentRef is valid and mpLeafName is specified, mbParentRefSet is true
		//
		// mpLeafName is a null pointer when it's not needed since it's >512 bytes otherwise
		
		FSRef mItemRef;
		struct
		{
			FSRef 		mItemParentRef;
			HFSUniStr255 * 	mpLeafName;
		} mParentAndLeaf;
	};
	
#ifdef _DEBUG
	CHXString		mStrDebugOnlyPath;	// shows path during debugging
	const char *		mPeekDebugOnlyPath;	// easier to see path during debugging
	void 			UpdateDebugOnlyPath();
#else
	inline void 		UpdateDebugOnlyPath() {};
#endif
	

	static HX_RESULT StuffHexHandle(Handle hData, Handle hHexHandle);
	static HX_RESULT UnstuffHexHandle(Handle hHex, Handle hData);
	

	void InitSpec();
	void ClearSpec();
	HX_RESULT AllocateLeafName();

	static void SplitPath(const char *pPath, char separator, CHXString& outParent, CHXString& outLeaf);
	
	static CHXString MakePersistentStringForRef(const FSRef *pAnchorRef, const FSRef& targetRef);
		
public:
	CHXMacInternalSpec();
	CHXMacInternalSpec(const FSSpec& spec);
	CHXMacInternalSpec(AliasHandle alis);
	CHXMacInternalSpec(const char* psz);
	CHXMacInternalSpec(const FSRef& ref);

	virtual ~CHXMacInternalSpec();
	
	CHXMacInternalSpec& operator=(CHXMacInternalSpec &other);
	HXBOOL operator==(const CHXMacInternalSpec &other);
	HXBOOL operator!=(const CHXMacInternalSpec &other);

	CHXString GetPOSIXPath() const;
	HX_RESULT SetFromPOSIXPath(const char *pPosixPath);

	CHXString GetHFSPath() const;
	HX_RESULT SetFromHFSPath(const char *pHFSPath);

	HXBOOL IsSet() const;	// object has been set to a file (which may or may not exist)

	void CopyInternalSpec(const CHXMacInternalSpec& otherToBeCopied);

	OSErr SetFromPath(const char *pszPath);
	OSErr SetFromFSRef(const FSRef& ref);
	OSErr SetFromFSSpec(const FSSpec& spec);
	OSErr SetFromAlias(AliasHandle alias);
	OSErr SetFromParentAndLeaf(const FSRef& parentRef, const char *pszName, CFStringEncoding encoding);
	OSErr SetFromParentAndLeafHFSUni(const FSRef& parentRef, const HFSUniStr255& leafName);
	OSErr SetFromURL(const char *pBuffer);

	void Unset();

	CHXString GetPathName() const;
	CHXString GetName() const; // get the leaf item name
	OSErr GetHFSUniLeafName(HFSUniStr255& uniName) const;
	OSErr GetParentDirectoryRef(FSRef &outParentRef) const;
	OSErr GetVolumeRef(FSRef& outVolumeRef) const; // get a specifier for the disk
	OSErr GetFSRef(FSRef& outRef) const;
	OSErr GetFSSpec(FSSpec& outSpec) const;
	OSErr GetVRefNum(FSVolumeRefNum& outVRefNum) const;


	CHXString GetURL(HXBOOL isDirectorySpec) const;
	CHXString GetPersistentString() const;
	CHXString GetRelativePersistentString(const CHXMacInternalSpec& fromFile) const;
	HX_RESULT SetFromPersistentString(const char *pBuffer);

};


#endif // FILESPECMAC_CARBON_H
