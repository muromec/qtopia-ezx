/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespec.h,v 1.10 2005/03/14 19:36:30 bobclark Exp $
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

#ifndef FILESPEC_H
#define FILESPEC_H

// include the platform-specific private data types
#if defined(_MAC_UNIX)
	#include "platform/mac/filespecmac_carbon.h"
#elif defined(_MACINTOSH)
	#ifdef _CARBON	
		#include "platform/mac/filespecmac_carbon.h"
	#else
		#include "platform/mac/filespecmac.h"
	#endif
#elif defined(_WINDOWS) || defined(_SYMBIAN)
	#include "platform/win/filespecwin.h"
#elif defined _UNIX
	#include "platform/unix/filespecunix.h"
#elif defined(_OPENWAVE)
	#include "platform/openwave/filespecopwave.h"
#else
	#error implement for this platform
#endif

#include "hxstring.h"

class CHXDirSpecifier;	// forward declaration

class CHXFileSpecifier // file specifier
{
public:
	// constructors
	CHXFileSpecifier();
	CHXFileSpecifier(const char* psz);				// path in C-string
	CHXFileSpecifier(const CHXString &str);	    // to fix a compile-time ambiguity coercing CHXStrings				
	CHXFileSpecifier(const CHXFileSpecifier &other);		
	~CHXFileSpecifier();	

	CHXFileSpecifier &operator=(const CHXFileSpecifier &other);
	CHXFileSpecifier &operator=(const char *other);
	CHXFileSpecifier &operator=(const CHXString& str);
	HXBOOL operator==(const CHXFileSpecifier &other);
	HXBOOL operator!=(const CHXFileSpecifier &other);
	
	void Unset();

	// accessors
	HXBOOL IsSet() const;	// object has been set to a file (which may or may not exist)
	
	// moving up or down the tree
	CHXDirSpecifier	GetParentDirectory() const;
	CHXDirSpecifier	GetVolume() const; // get a specifier for the disk

	// display & paths
	CHXString	GetName() const;   // get the file's name and extension (not path) like 'Foo.txt'
	CHXString	GetTitle() const;  // gets the file's name (w/o extension)
	CHXString	GetPathName() const;
	CHXString	GetURL() const;
	
	CHXString	GetExtension() const;	// returns whatever is after the last dot, not including the dot
	
	// persistent string can be saved to preferences or
	// handed to another library
	CHXString GetPersistentString() const;
	HX_RESULT SetFromPersistentString(const char *pBuffer);
	
	HX_RESULT SetFromURL(const char *pBuffer);

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
	CHXFileSpecifier(const FSSpec &file);	// Mac-specific
	CHXFileSpecifier(AliasHandle alias);	
	CHXString GetRelativePersistentString(const CHXFileSpecifier& fromFileSpec) const;
	
	operator FSSpec() const;
	operator FSSpec*() const;

	#if defined(_CARBON) || defined(_MAC_UNIX)
		CHXFileSpecifier(const FSRef &ref);	
		operator FSRef() const;
		operator FSRef*() const;
		CHXString GetPOSIXPath() const;
		HX_RESULT SetFromPOSIXPath(const char *pPosixPath);
		CHXString GetHFSPath() const;
		HX_RESULT SetFromHFSPath(const char *pHFSPath);
		
		HFSUniStr255 GetNameHFSUniStr255() const;
		FSVolumeRefNum GetVRefNum() const;
		
		friend class CHXDirSpecifier;
	#endif
#endif

private:

	// private data is platform-specific
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
	#if defined(_CARBON) || defined(_MAC_UNIX)
		CHXMacInternalSpec	m_MacInternalSpec;
		
		mutable FSSpec*	m_pTempSpec; // for (FSSpec*) only
		mutable FSRef*	m_pTempRef; // for (FSRef*) only
	#else
		CAliasHandler	m_macentity;
		
		mutable FSSpec*	m_pTempSpec; // for (FSSpec*) only
		mutable void*	m_pTempRef; 
	#endif
#elif defined( _WINDOWS ) || defined( _UNIX ) || defined(_SYMBIAN)

	CHXPathParser m_parser;

#endif
	
};

class CHXDirSpecifier	// directory specifier
{
public:
	// constructors
	CHXDirSpecifier();	
	CHXDirSpecifier(const char* psz);				// path in C-string
	CHXDirSpecifier(const CHXString& str);	    // to fix a compile-time ambiguity coercing CHXStrings
	CHXDirSpecifier(const CHXDirSpecifier &other);
	~CHXDirSpecifier();	

	CHXDirSpecifier &operator=(const CHXDirSpecifier& other);
	CHXDirSpecifier &operator=(const char *other);
	CHXDirSpecifier &operator=(const CHXString& str);
	HXBOOL operator==(const CHXDirSpecifier &other);
	HXBOOL operator!=(const CHXDirSpecifier &other);

	void Unset();

	// accessors
	HXBOOL	IsVolume() const;	// specifies a disk (or root directory)
	HXBOOL 	IsSet() const;	// object has been set to a directory (which may or may not exist)

	CHXString	GetName() const;   // get the directory name
	
	// moving up or down the tree
	CHXDirSpecifier	GetParentDirectory() const;
	CHXDirSpecifier	GetVolume() const; // get a specifier for the disk

	// get specifiers for file or directory inside this one
	CHXFileSpecifier SpecifyChildFile(const char *child) const;
	CHXDirSpecifier SpecifyChildDirectory(const char *child) const;

	// display & paths
	CHXString	GetPathName() const;

	CHXString	GetURL() const;
	HX_RESULT SetFromURL(const char *pBuffer);

	// persistent string can be saved to preferences or
	// handed to another library
	CHXString GetPersistentString() const;
	HX_RESULT SetFromPersistentString(const char *pBuffer);

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
	// Mac-specific
	CHXDirSpecifier(const FSSpec &folder); // Mac-specific	
	CHXDirSpecifier(AliasHandle dirAlias); // Mac-specific
	
	CHXFileSpecifier SpecifyChildFileStr255(ConstStr255Param child) const;
	CHXDirSpecifier SpecifyChildDirectoryStr255(ConstStr255Param child) const;

	long GetDirID() const;
	operator FSSpec() const;	// note the parID is the folder's
	operator FSSpec*() const;	//   parent's dirID, not the folder's dirID

	#if defined(_CARBON) || defined(_MAC_UNIX)
		CHXFileSpecifier SpecifyChildFileHFSUniStr255(HFSUniStr255 hfsUni) const;
		CHXDirSpecifier SpecifyChildDirectoryHFSUniStr255(HFSUniStr255 hfsUni) const;
		
		HFSUniStr255 GetNameHFSUniStr255() const;
		FSVolumeRefNum GetVRefNum() const;

		CHXDirSpecifier(const FSRef &ref);	// Mac-specific
		operator FSRef() const;
		operator FSRef*() const;
		CHXString GetPOSIXPath() const;
		HX_RESULT SetFromPOSIXPath(const char *pPosixPath);
		CHXString GetHFSPath() const;
		HX_RESULT SetFromHFSPath(const char *pHFSPath);
	#endif
#endif



private:

	// private data is platform-specific
#if defined(_MACINTOSH) || defined(_MAC_UNIX)

	#if defined(_CARBON) || defined(_MAC_UNIX)
		CHXMacInternalSpec	m_MacInternalSpec;
		
		mutable FSSpec*	m_pTempSpec; // for (FSSpec*) only
		mutable FSRef*	m_pTempRef; // for (FSRef*) only
	#else
		CAliasHandler	m_macentity;
		
		mutable FSSpec*	m_pTempSpec; // for (FSSpec*) only
		mutable void*	m_pTempRef; 
	#endif
#ifndef USE_FSREFS
        static OSErr FSpGetDirectoryID(const FSSpec *spec, long *theDirID, Boolean *isDirectory);
#endif

#elif defined( _WINDOWS ) || defined( _UNIX ) || defined(_SYMBIAN)

	CHXPathParser m_parser;

#endif
	
};


#endif // FILESPEC_H
