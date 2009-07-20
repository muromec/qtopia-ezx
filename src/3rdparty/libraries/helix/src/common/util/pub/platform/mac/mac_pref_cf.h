/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mac_pref_cf.h,v 1.8 2006/02/07 19:21:33 ping Exp $
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

#ifndef _CARBON
#ifndef _MAC_UNIX
#error use mac_pref.h instead for non-carbon builds
#endif
#endif

#include "pref.h"
#include "cfwrappers.h"
#include "hxstring.h"
#include "mac_pref_file.h"
#include "carray.h"

class CMacPref : public CPref 
{
public:
	/* 	call open_pref() to automatically create the correct Mac specific preference object. */
	/*	This will put the product prefs into RealPlayer's prefs file; to create a separate prefs file,
	        precede the product name with !, like "!RealProducer". */
	static	CMacPref	*open_pref (const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer, HXBOOL bCommon, IUnknown* pContext);
	
	/* 	class destructor - writes prefs to disk. */	
	virtual ~CMacPref 			(void);

	/*  read_pref reads the preference specified by Key to the Buffer. */	
    	HX_RESULT	read_pref   (const char* pPrefKey, IHXBuffer*& pBuffer);   
                                                                        
	/*  write_pref writes (to memory) the preference specified by Key from the Buffer. */	                                                                            
    	HX_RESULT    write_pref      (const char* pPrefKey, IHXBuffer* pBuffer); 

	/*  delete_pref deletes the preference specified by Key from the Buffer. */	                                                                            
    	HX_RESULT    delete_pref      (const char* pPrefKey); 

	HX_RESULT last_error (void)	{ return mLastError; } // GR 3/12/02 do we need this here?

   	virtual HX_RESULT   remove_pref(const char* pPrefKey);
   	virtual HX_RESULT   remove_indexed_pref(const char* pPrefKey);

   	HX_RESULT	BeginSubPref(const char* szSubPref);
   	HX_RESULT	EndSubPref();

   	HX_RESULT	GetPrefKey(UINT32 nIndex,IHXBuffer*& pBuffer);
   	
	HX_RESULT commit_prefs();
	
private:	
	// Constructor NOTE: use open_pref() to create an instance of this class
	CMacPref(const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer, HXBOOL bCommon, IUnknown* pContext);		// ctor

	/*
	CFStringRef	m_UserNameRef;	// is kCFPreferencesAnyUser or kCFPreferencesCurrentUser depending on bCommon
	CFStringRef	m_HostNameRef;	// is kCFPreferencesCurrentHost or kCFPreferencesAnyHost depending on bCommon

	CHXCFString 	m_cfsPrefsBundleName;
	*/
	CMacPrefFile*	m_pPrefFile;
	
	CHXString	m_strProductNamePrefix; // product name becomes the beginning of each pref prefix, like RealMediaSDK\ or similar
	CHXString 	m_strSubPrefPrefix;     // empty or "SubPref\" or "Sub1\Sub2\", etc
	
	CHXPtrArray	m_arrSubKeyStack; // when we're in nested BeginSubPrefs, stores previous CHXStrings with the prefixes
	
   	HX_RESULT GetPrefKeyInternal(UINT32 nIndex,HXBOOL bReturnDeeperNestings,CHXString& outStrKey);
   	void RemoveSinglePrefInternal(const char* pPrefKey);
   	
	void MakePrefKeyCF(const char *pPrefKey, CHXCFString& outCfsRealKey);
	void MakePrefKey(const char *pPrefKey, CHXString& outMadeKey);
	
	HX_RESULT MakeIHXBuffer(const char *pData, IHXBuffer*& pBuffer);
	
	void EndAllSubPrefs();

 };
