/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cfwrappers.h,v 1.5 2004/07/09 18:23:21 hubbe Exp $
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

//
// Core Foundation wrappers
//
// These wrappers primarily exist to ensure that the types get disposed
//
// They can in most instances be dropped directly into Core Foundation
// toolbox calls.  Because of that, they have few methods themselves.
//
// BIG WARNING: These will RELEASE the native CF object of the same type
// assigned to them. So don't assign a CFString to a CHXCFString or
// a CFURL to a CHXCFURL unless it should be released
// when the wrapper object is disposed.
//
// This means that a CFObject returned by a "..Copy.." CF routine can
// be assigned to these wrappers, but a CFObject returned by a "..Get.."
// CF routine should NOT be assigned to these (unleass an extra CFRetain
// is called on the object.)
//
// A additional retain is done when assigning CHXCFString to a CHXCFString
// or a CHXCFURL to a CHXCFURL since both objects will release the
// underlying ref on destruction.

#pragma once

#ifndef _CARBON
#ifndef _MAC_UNIX
#error Carbon-only header
#endif
#endif

#ifndef _MAC_MACHO
#include <CFString.h>
#endif

#include "hxstring.h"	
#include "hxtypes.h"

class	CHXCFString 
{
public:
	CHXCFString(void);
	CHXCFString(CFStringRef cfs); // doesn't do an additional retain
	CHXCFString(const CHXCFString& cfs); // does do an additional retain
	CHXCFString(const CHXString &str); // makes an immutable string
	CHXCFString(const char *pCString); // makes an immutable string
	CHXCFString(const char *pCString, CFStringEncoding encoding); // makes an immutable string
	CHXCFString(ConstStr255Param pPString);
        CHXCFString(const UInt16* pUniChars, INT32 numChars);
	CHXCFString(CFURLRef urlRef);
	CHXCFString(const HFSUniStr255& hfsUni);
	~CHXCFString();

	operator CFStringRef() const { return mCFStringRef; }
	operator HFSUniStr255() const;
	Boolean IsSet() const { return (mCFStringRef != NULL); }

	const CHXCFString& operator =(CFStringRef cfs);  // NB: just frees the old one and copies the ref, doesn't do an additional retain
	const CHXCFString& operator =(const CHXCFString& cfs); // this one does add a retain to the string

	const CHXCFString& operator =(const char *pString);
	const CHXCFString& operator =(const CHXString& str);
	const CHXCFString& operator =(CFURLRef urlRef);
	const CHXCFString& operator =(const HFSUniStr255& hfsUni);

private:
	// There is no (const char *) operator defined for return strings since
	// we don't want to manage memory with this little wrapper class; assign 
	// to a CHXString instead
	
	CFStringRef		mCFStringRef;
	
	inline void ReleaseCFString();
	void SetToCFURL(CFURLRef cfURL);
	
	#ifdef _DEBUG
		// these enable us to see the internals of the CFString during debugging
		void UpdateDebugOnlyString(void);
		const char *pStrDebugOnlyPeek;
		CHXString mStrDebugOnly;
	#else
		inline void UpdateDebugOnlyString(void) {};
	#endif

};


class	CHXCFURL 
{
public:
	CHXCFURL(void);
	CHXCFURL(const char *pCString);
	CHXCFURL(const CHXString& str);
	CHXCFURL(const FSRef& fsRef);
	CHXCFURL(const FSRef* fsRef);
	CHXCFURL(CFURLRef urlRef);
	~CHXCFURL();

	operator CFURLRef() const { return mCFURLRef; }
	Boolean IsSet() const { return (mCFURLRef != NULL); }

	const CHXCFURL& operator =(CFURLRef cfs);  // NB: just copies the ref, doesn't do an additional retain
	const CHXCFURL& operator =(const CHXCFURL& cfs); // does do an additional retain
	
	const CHXCFURL& operator =(const CHXString& str);
	const CHXCFURL& operator =(const char *pString);
	const CHXCFURL& operator =(const FSRef& fsRef);
	const CHXCFURL& operator =(const FSRef* fsRef);
	
private:
	CFURLRef		mCFURLRef;
	
	void SetToString(const char *pCString);
	void SetToFSRef(const FSRef& fsRef);
	inline void ReleaseCFURL();

	#ifdef _DEBUG
		// these enable us to see the internals of the CFURL during debugging
		void UpdateDebugOnlyString();
		const char *pStrDebugOnlyPeek;
		CHXString mStrDebugOnly;
	#else
		inline void UpdateDebugOnlyString() {};
	#endif
};
