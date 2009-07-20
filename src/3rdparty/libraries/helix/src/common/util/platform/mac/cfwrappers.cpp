/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cfwrappers.cpp,v 1.6 2004/09/13 19:29:44 sehancher Exp $
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

#pragma once

#if defined(_CARBON) || defined(_MAC_UNIX)

#include "platform/mac/cfwrappers.h"
#include "hxassert.h"

//------------------------------------------------------------
//
// CHXCFString
//
//------------------------------------------------------------

// Constructors

CHXCFString::CHXCFString(void) : mCFStringRef(NULL)
{
	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(CFStringRef cfs)
{
	mCFStringRef = cfs;

	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(const CHXCFString& othercfs)
{
	mCFStringRef = othercfs.mCFStringRef; 
	
	if (mCFStringRef)
	{
		CFRetain(mCFStringRef); // since he'll release it and we'll release it
	}

	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(const char *pCString, CFStringEncoding encoding)
{
	mCFStringRef = CFStringCreateWithCString(kCFAllocatorDefault, pCString, encoding);

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(const char *pCString)
{
	mCFStringRef = CFStringCreateWithCString(kCFAllocatorDefault, pCString, CFStringGetSystemEncoding());

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(const CHXString &str)
{
	mCFStringRef = CFStringCreateWithCString(kCFAllocatorDefault, (const char *) str, CFStringGetSystemEncoding());

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(ConstStr255Param pPString)
{
	mCFStringRef = CFStringCreateWithPascalString(kCFAllocatorDefault, pPString, CFStringGetSystemEncoding());

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(const UInt16* pUniChars, INT32 numChars)
{
	mCFStringRef = CFStringCreateWithCharacters(kCFAllocatorDefault, pUniChars, numChars);

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
}

CHXCFString::CHXCFString(CFURLRef urlRef)
{
	mCFStringRef = NULL;
	
	SetToCFURL(urlRef);
}

CHXCFString::CHXCFString(const HFSUniStr255& hfsUni)
{
	mCFStringRef = CFStringCreateWithCharacters(kCFAllocatorDefault, 
		hfsUni.unicode, hfsUni.length);

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
}

// Type cast operators

CHXCFString::operator HFSUniStr255(void) const
{
	HFSUniStr255 hfsUni;
	CFIndex cfLength;
	
	cfLength = CFStringGetLength(mCFStringRef);
	if (cfLength <= 255)
	{
		hfsUni.length = cfLength;
		
		CFStringGetCharacters(mCFStringRef, CFRangeMake(0, cfLength), hfsUni.unicode);
	}
	else
	{
		hfsUni.length = 0;
	}
	
	return hfsUni;
}

// Assignment operators

const CHXCFString& CHXCFString::operator =(const char *pCString)
{
	ReleaseCFString();
	
	mCFStringRef = CFStringCreateWithCString(kCFAllocatorDefault, pCString, CFStringGetSystemEncoding());

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
	
	return *this;
}

const CHXCFString& CHXCFString::operator =(const CHXString& str)
{
	ReleaseCFString();
	
	mCFStringRef = CFStringCreateWithCString(kCFAllocatorDefault, (const char *) str, CFStringGetSystemEncoding());

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
	
	return *this;
}

const CHXCFString& CHXCFString::operator =(CFStringRef cfs)
{
	ReleaseCFString();
	
	mCFStringRef = cfs; // note we're NOT doing an additional retain, since these usually are returned by toolbox calls

	UpdateDebugOnlyString();
	
	return *this;
}

const CHXCFString& CHXCFString::operator =(const CHXCFString& othercfs)
{
	ReleaseCFString();
	
	mCFStringRef = othercfs.mCFStringRef; 

	if (mCFStringRef)
	{
		CFRetain(mCFStringRef); // since he'll release it and we'll release it
	}

	UpdateDebugOnlyString();
	
	return *this;
}

const CHXCFString& CHXCFString::operator =(CFURLRef urlRef)
{
	SetToCFURL(urlRef);
	
	return *this;
}

const CHXCFString& CHXCFString::operator =(const HFSUniStr255& hfsUni)
{
	ReleaseCFString();
	
	mCFStringRef = CFStringCreateWithCharacters(kCFAllocatorDefault, 
		hfsUni.unicode, hfsUni.length);

	check_nonnull(mCFStringRef);
	
	UpdateDebugOnlyString();
	
	return *this;
}

// Destructor

CHXCFString::~CHXCFString()
{
	ReleaseCFString();
}

// Internal utilities

void CHXCFString::ReleaseCFString()
{
	if (mCFStringRef)
	{
		::CFRelease(mCFStringRef);
		mCFStringRef = NULL;
	}
}

void CHXCFString::SetToCFURL(CFURLRef cfURL)
{
	ReleaseCFString();
	
	CFStringRef tempStringRef;
	
	tempStringRef = ::CFURLGetString(cfURL); // we do NOT release the CFString from CFURLGetString
	if (tempStringRef)
	{
		mCFStringRef = ::CFStringCreateCopy(kCFAllocatorDefault, tempStringRef);
	}
	
	UpdateDebugOnlyString();
}

#ifdef _DEBUG
void CHXCFString::UpdateDebugOnlyString()
{
	if (mCFStringRef)
	{
		mStrDebugOnly.SetFromCFString(mCFStringRef, CFStringGetSystemEncoding());
	}
	else
	{
		mStrDebugOnly.Empty();
	}
	pStrDebugOnlyPeek = (const char *) mStrDebugOnly;
}
#endif // _DEBUG

#pragma mark -

//------------------------------------------------------------
//
// CHXCFURL
//
//------------------------------------------------------------

// Constructors

CHXCFURL::CHXCFURL(void) : mCFURLRef(NULL)
{
	UpdateDebugOnlyString();
}

CHXCFURL::CHXCFURL(const char *pCString)
{
	mCFURLRef = NULL;
	
	SetToString(pCString);
}

CHXCFURL::CHXCFURL(const CHXString& str)
{
	mCFURLRef = NULL;
	
	SetToString((const char *) str);
}

CHXCFURL::CHXCFURL(const FSRef& fsRef)
{
	mCFURLRef = NULL;
	
	SetToFSRef(fsRef);
}

CHXCFURL::CHXCFURL(const FSRef* fsRef)
{
	mCFURLRef = NULL;
	
	SetToFSRef(*fsRef);
}

CHXCFURL::CHXCFURL(CFURLRef urlRef)
{
	mCFURLRef = urlRef; // note we're NOT doing an additional retain

	UpdateDebugOnlyString();
}

// Assignment operators

const CHXCFURL& CHXCFURL::operator =(const char *pCString)
{
	SetToString(pCString);
	
	return *this;
}

const CHXCFURL& CHXCFURL::operator =(const CHXString& str)
{
	SetToString((const char *) str);
	
	return *this;
}

const CHXCFURL& CHXCFURL::operator =(const FSRef& fsRef)
{
	SetToFSRef(fsRef);
	
	return *this;
}

const CHXCFURL& CHXCFURL::operator =(const FSRef* fsRef)
{
	SetToFSRef(*fsRef);
	
	return *this;
}

const CHXCFURL& CHXCFURL::operator =(CFURLRef urlRef)
{
	ReleaseCFURL();
	
	mCFURLRef = urlRef; // note we're NOT doing an additional retain

	UpdateDebugOnlyString();
	
	return *this;
}

const CHXCFURL& CHXCFURL::operator =(const CHXCFURL& othercfurl)
{
	ReleaseCFURL();
	
	mCFURLRef = othercfurl.mCFURLRef; // note we're retaining since he'll release it and we'll release it
	CFRetain(mCFURLRef);

	UpdateDebugOnlyString();
	
	return *this;
}


// Destructor

CHXCFURL::~CHXCFURL()
{
	ReleaseCFURL();
}

// Internal utilities

void CHXCFURL::SetToString(const char *pCString)
{
	const CFURLRef kNoBaseURL = NULL;
	
	ReleaseCFURL();
	
	// make a CFString then a CFURL from which we can get an FSRef
	CHXCFString cfstr(pCString, kCFStringEncodingUTF8);
	
	require(cfstr.IsSet(), CantMakeCFString);
	
	mCFURLRef = ::CFURLCreateWithString(kCFAllocatorDefault, cfstr, kNoBaseURL);
	if (mCFURLRef == NULL)
	{
		// failed; CFURL seems unable to deal with parameters, so try
		// stripping those and trying again
		CHXString strURL = pCString;
		int paramOffset = strURL.Find('?');
		if (paramOffset != -1)
		{
			CHXCFString cfstr2((const char *) strURL.Left(paramOffset), kCFStringEncodingUTF8);
			mCFURLRef = ::CFURLCreateWithString(kCFAllocatorDefault, cfstr2, kNoBaseURL);
		}
	}
	
	UpdateDebugOnlyString();

CantMakeCFString:
	return;
}

void CHXCFURL::SetToFSRef(const FSRef& fsRef)
{
	ReleaseCFURL();
	
	mCFURLRef = ::CFURLCreateFromFSRef(kCFAllocatorDefault, &fsRef);
	
	UpdateDebugOnlyString();

	return;
}

void CHXCFURL::ReleaseCFURL()
{
	if (mCFURLRef)
	{
		::CFRelease(mCFURLRef);
		mCFURLRef = NULL;
	}
	
	UpdateDebugOnlyString();
}


#ifdef _DEBUG
void CHXCFURL::UpdateDebugOnlyString()
{
	if (mCFURLRef)
	{
		mStrDebugOnly = ::CFURLGetString(mCFURLRef); // we do NOT release the CFString from CFURLGetString
	}
	else
	{
		mStrDebugOnly.Empty();
	}
	pStrDebugOnlyPeek = (const char *) mStrDebugOnly;
}
#endif // _DEBUG

#endif // _CARBON
