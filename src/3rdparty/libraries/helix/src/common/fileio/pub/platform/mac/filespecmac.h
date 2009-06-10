/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecmac.h,v 1.5 2007/07/06 20:35:19 jfinnecy Exp $
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

#ifndef FILESPECMAC_H
#define FILESPECMAC_H

#ifdef _CARBON
#define USE_FSREFS 1
#endif

#include "hxstring.h"

// CAliasHandler are the internal component of CHXFileSpecifier and CHXDirSpecifier
// on the Mac. 

class CAliasHandler
{
private:
	AliasHandle	m_alias;
	
#ifdef _DEBUG
	CHXString		m_strReadOnlyPath;	// shows path during debugging
	const char *		m_pszReadOnlyPath;	// easier to see path during debugging
	void 			UpdateReadOnlyPath();
#else
	inline void 	UpdateReadOnlyPath() {};
#endif

	static HX_RESULT StuffHexHandle(Handle hData, Handle hHexHandle);
	static HX_RESULT UnstuffHexHandle(Handle hHex, Handle hData);
	static OSErr ResolveRelativeAliasQuiet(AliasHandle alias,
		FSSpec *target, Boolean *changed);
	static OSErr ResolveAliasQuiet(const FSSpec *fromFile, AliasHandle alias,
		FSSpec *target, Boolean *changed);
#ifdef USE_FSREFS
	static OSErr ResolveRelativeAliasQuietToRef(AliasHandle alias,
		FSRef *target, Boolean *changed);
#endif
		
	
		
public:
	CAliasHandler();
	CAliasHandler(const FSSpec& spec);
	CAliasHandler(AliasHandle alis);
	CAliasHandler(const char* psz);
#ifdef USE_FSREFS
	CAliasHandler(const FSRef& ref);
#endif
	virtual ~CAliasHandler();
	
	CAliasHandler& operator=(CAliasHandler &other);
	HXBOOL operator==(const CAliasHandler &other);
	HXBOOL operator!=(const CAliasHandler &other);
	
	operator FSSpec() const;
#ifdef USE_FSREFS
	operator FSRef() const;
	CHXString GetPOSIXPath() const;
	HX_RESULT SetFromPOSIXPath(const char *pPosixPath);
#endif

	HXBOOL IsSet() const;	// object has been set to a file (which may or may not exist)

	void Unset();

	void DisposeAlias();

	CHXString GetPathName() const;
	CHXString	GetName() const; // get the leaf item name
	OSErr GetParentDirectorySpec(FSSpec &parent) const;
	OSErr GetVolumeSpec(FSSpec &parent) const; // get a specifier for the disk

	void CopyAlias(const CAliasHandler& other);
	
	CHXString GetPersistentString() const;
	CHXString GetRelativePersistentString(const CAliasHandler& anchor) const;
	HX_RESULT SetFromPersistentString(const char *pBuffer);

	CHXString MakePersistentStringForAlias(AliasHandle theAlias) const;
};

#endif // FILESPECMAC_H
