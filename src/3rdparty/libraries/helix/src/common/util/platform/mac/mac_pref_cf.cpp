/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mac_pref_cf.cpp,v 1.16 2007/07/19 00:07:15 milko Exp $
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

#include "platform/mac/mac_pref_cf.h"
#include "hxbuffer.h"
#include "filespecutils.h"

const char * kPrefPrefixSeparator = "\\"; // backslash since no Windows programmer will use it inside of a key

// 	call open_pref() to automatically create the correct Macintosh specific preference object.
CMacPref * CMacPref::open_pref(const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer, HXBOOL bCommon, IUnknown* pContext)
{
    return new CMacPref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, bCommon, pContext);
}

// 	Constructor NOTE: use open_pref() to create an instance of this class
CMacPref::CMacPref(const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer, HXBOOL bCommon, IUnknown* pContext)
         :CPref(pContext)
{  
	CHXString strBundleID;
	CFStringRef userNameRef, hostNameRef;
	CHXCFString cfsPrefsBundleName;
	
	check(pCompanyName && *pCompanyName);
	check(pProductName && *pProductName);
	
	check(strcspn(pCompanyName, " .,;:/\\!@#$%^&*()\"") == strlen(pCompanyName)); // ensure there are no punctuation
	
	// prefs common among all users are "any user, current host"
	// prefs for just the current user are "current user, any host"
	
	userNameRef = bCommon ? kCFPreferencesAnyUser : kCFPreferencesCurrentUser;
	hostNameRef = bCommon ? kCFPreferencesCurrentHost : kCFPreferencesAnyHost;
	
	m_pPrefFile = NULL;
	m_strProductNamePrefix = pProductName;

#if defined(HELIX_FEATURE_PREF_OVERLOADING)
	// Added support for overloading the preference file name.
	// If the HelixPreferenceFileID preferences exists in the main bundle's pref's, then use its value as the pref. filename.
	CFStringRef helixPrefFileID = ( CFStringRef ) CFPreferencesCopyAppValue( CFSTR( "HelixPreferenceFileID" ), kCFPreferencesCurrentApplication );
	if ( helixPrefFileID )
	{
		// Support mapping of helix product names to application specific values.
		m_pPrefFile = CMacPrefFile::CreatePrefSystemObject( helixPrefFileID, userNameRef, hostNameRef );
		CFDataRef helixPrefMapRef = ( CFDataRef ) CFPreferencesCopyAppValue( CFSTR( "HelixPrefMap" ), helixPrefFileID );
		if ( helixPrefMapRef )
		{
			CFStringRef errorString;
			CFDictionaryRef helixPrefMap = ( CFDictionaryRef ) CFPropertyListCreateFromXMLData( kCFAllocatorDefault, helixPrefMapRef, kCFPropertyListImmutable, &errorString );
			if ( helixPrefMap )
			{
				CFStringRef key = CFStringCreateWithCString( kCFAllocatorDefault, m_strProductNamePrefix, kCFStringEncodingUTF8 );
				if ( key )
				{
					CFStringRef mappedPrefixName = ( CFStringRef ) CFDictionaryGetValue( helixPrefMap, key );
					if ( mappedPrefixName )
					{
						m_strProductNamePrefix = mappedPrefixName;
					}
					CFRelease( key );
				}
				CFRelease( helixPrefMap );
			}
			if ( errorString )
			{
				CFRelease( errorString );
			}
			CFRelease( helixPrefMapRef );
		}
		CFRelease( helixPrefFileID );
	}
#endif	// HELIX_FEATURE_PREF_OVERLOADING
	{
		strBundleID.Format("com.%s.%s", pCompanyName, pProductName);
		cfsPrefsBundleName = strBundleID;
		m_pPrefFile = CMacPrefFile::CreatePrefSystemObject( cfsPrefsBundleName, userNameRef, hostNameRef );
	}
	// save the product name which becomes the prefix for each pref entry, like "RealMediaSDK\"
	m_strProductNamePrefix += kPrefPrefixSeparator;
	
	check_nonnull(m_pPrefFile);
	
	if (bCommon)
	{
		HX_ASSERT(!"Using machine-wide prefs, which fails for non-admin users. Absolutely sure?");
	}
}

// 	class destructor 	
CMacPref::~CMacPref (void)
{
	commit_prefs();		// writes prefs to disk
	
	EndAllSubPrefs();	// frees up the stack of CHXStrings
	
	if (m_pPrefFile)
	{
		HX_DELETE(m_pPrefFile);
	}
}

HX_RESULT CMacPref::commit_prefs(void)
{
	require_nonnull_return(m_pPrefFile, HXR_NOT_INITIALIZED);
	
	m_pPrefFile->Synchronize();

	// Boolean bSuccess = CFPreferencesAppSynchronize(m_cfsPrefsBundleName);
	
	//return bSuccess ? HXR_OK : HXR_FAIL;
	
	return HXR_OK;
}

void CMacPref::MakePrefKeyCF(const char *pPrefKey, CHXCFString& outCfsRealKey)
{
	CHXString strKey;
	
	MakePrefKey(pPrefKey, strKey);
	
	// CHXCFString takes ownership of the retained CFStringRef.
	outCfsRealKey = CFStringCreateWithCString(kCFAllocatorDefault, (const char*) strKey, kCFStringEncodingUTF8);
}

void CMacPref::MakePrefKey(const char *pPrefKey, CHXString& outMadeKey)
{
	CHXString strInKey;
	
	//check(strstr(pPrefKey, kPrefPrefixSeparator) == 0); // check that the separator isn't in a key
	// alas, the Core sometimes builds subprefs by lazily including backslashes
	
	strInKey = pPrefKey;
	
	// delete any leading separator at the beginning of the string
	if (strInKey.Find(kPrefPrefixSeparator) == 0)
	{
		strInKey = strInKey.Right(strInKey.GetLength() - strlen(kPrefPrefixSeparator));
	}
	
	// the full key is constructed as "Product/Subpref/Key"
	
	outMadeKey = m_strProductNamePrefix;
	
	if (m_strSubPrefPrefix.IsEmpty())
	{
		// no subpref prefix
		outMadeKey += strInKey;
	}
	else
	{
		// prepend with subpref prefix
		outMadeKey += m_strSubPrefPrefix;
		outMadeKey += strInKey;
	}
}

HX_RESULT CMacPref::MakeIHXBuffer(const char *pData, IHXBuffer*& pBuffer)
{
	check_nonnull(pData);
	
	return CHXBuffer::FromCharArray(pData, &pBuffer);
}

//  read_pref reads the preference specified by Key to the Buffer.
HX_RESULT CMacPref::read_pref(const char* pPrefKey, IHXBuffer*& pBuffer)
{
	require_nonnull_return(m_pPrefFile, HXR_NOT_INITIALIZED);

	CHXCFString cfsKey;
	CHXCFString cfsPref;
	
	MakePrefKeyCF(pPrefKey, cfsKey);
	
	cfsPref = (CFStringRef) m_pPrefFile->CopyValue(cfsKey);

	if (cfsPref.IsSet())
	{
		CHXString str;
		
		str = cfsPref;
		
		return MakeIHXBuffer((const char *) str, pBuffer);
	}
	
	return HXR_FAIL;
}
 
//  write_pref writes (saves) the preference specified by pPrefKey from the Buffer. 
HX_RESULT CMacPref::write_pref(const char* pPrefKey, IHXBuffer* pBuffer)
{   
	require_nonnull_return(m_pPrefFile, HXR_NOT_INITIALIZED);
	
	CHXCFString cfsKey;
	CHXCFString cfsValue((const char *) pBuffer->GetBuffer());

	MakePrefKeyCF(pPrefKey, cfsKey);

	// CFPreferences doesn't return an error for this...
	m_pPrefFile->SetValue(cfsKey, cfsValue);
	//::CFPreferencesSetValue(cfsKey, cfsValue, m_cfsPrefsBundleName, m_UserNameRef, m_HostNameRef);
	
	return HXR_OK;
}

//  delete_pref deletes the preference specified by Key from the Buffer.
HX_RESULT CMacPref::delete_pref(const char* pPrefKey)
{   
    return remove_pref(pPrefKey);
}

HX_RESULT CMacPref::remove_pref(const char* pPrefKey)
{
	// pPrefKey is either a key (in the current subpref) or a subpref branch name
	// (also in the current subpref)
	
	// try deleting that key directly
	RemoveSinglePrefInternal(pPrefKey);
	
	// now try deleting that key as a subpref subtree: we'll set to the subpref and
	// remove all keys inside of it
	if (pPrefKey && *pPrefKey)
	{
		const UINT32 kFirstKeyIndex = 0;
		CHXString strKey;
		const HXBOOL kReturnDeeperNestings = TRUE;
		
		BeginSubPref(pPrefKey);
		while (1)
		{
			HX_RESULT res = GetPrefKeyInternal(kFirstKeyIndex, kReturnDeeperNestings, strKey); // we want "sub/x" and "sub/x/y"
			if (FAILED(res)) break;
			
			RemoveSinglePrefInternal(strKey);
		}
		EndSubPref();
	}
	
	return HXR_OK;
}

void CMacPref::RemoveSinglePrefInternal(const char* pPrefKey)
{
	require_nonnull_void_return(m_pPrefFile);

	const CFPropertyListRef kRemovePreference = NULL;
	
	CHXCFString cfsKey;
	
	MakePrefKeyCF(pPrefKey, cfsKey);
	m_pPrefFile->SetValue(cfsKey, kRemovePreference); // CFPrefs doesn't return an error for this
}

HX_RESULT CMacPref::remove_indexed_pref(const char* pPrefKey)
{
	CHXString strKey;
	UINT16 index;
	IHXBuffer* pBuffer;
	HX_RESULT err;
	
	index = 1;
	while (1)
	{
		strKey.Format("%s%d", pPrefKey, index);
		
		// if there's one with this index, remove it
		pBuffer = nil;
		
		err = read_pref(strKey, pBuffer);
		HX_RELEASE(pBuffer);

		if (SUCCEEDED(err))
		{
			err = remove_pref(strKey);
			check_success(err);
			
			index++;
		}
		else
		{	
			// no more keyN prefs here; we're done
			break;
		}
	}

	return HXR_OK;
}

HX_RESULT CMacPref::BeginSubPref(const char* szSubPref)
{
	check(szSubPref && strlen(szSubPref));
	
	// if there was already a prefix, push it onto the stack to save it for later
	if (!m_strSubPrefPrefix.IsEmpty())
	{
		CHXString *pStrNew = new CHXString;
		check_nonnull(pStrNew);
		
		*pStrNew = (const char *) m_strSubPrefPrefix;
		
		m_arrSubKeyStack.Add(&pStrNew);
	}
	
	// now add the new subpref prefix part and a separator to the previous subpref prefix
	m_strSubPrefPrefix += szSubPref;
	m_strSubPrefPrefix += kPrefPrefixSeparator;

	return HXR_OK;
}

HX_RESULT CMacPref::EndSubPref()
{
	require_return(!m_strSubPrefPrefix.IsEmpty(), HXR_FAIL);
	
	m_strSubPrefPrefix.Empty();
	if (!m_arrSubKeyStack.IsEmpty())
	{
		// we're nested in; pop an old prefix off the stack
		CHXString *pStrOld;
		HX_RESULT res = HXR_OK;
		
		pStrOld = (CHXString*)m_arrSubKeyStack.GetAt(m_arrSubKeyStack.GetSize()-1);
		m_arrSubKeyStack.RemoveAt(m_arrSubKeyStack.GetSize()-1);
		check_success(res);
		
		if (SUCCEEDED(res))
		{
			m_strSubPrefPrefix = (const char *) *pStrOld;
			delete pStrOld;
		}
	}

	return HXR_OK;
}

void CMacPref::EndAllSubPrefs()
{
	while (!m_arrSubKeyStack.IsEmpty())
	{
		// get rid of those old CHxStrings
		EndSubPref();
	}
	m_strSubPrefPrefix.Empty();
}		



HX_RESULT CMacPref::GetPrefKey(UINT32 nIndex,IHXBuffer*& pBuffer)
{
	const HXBOOL kDontReturnDeeperNestings = FALSE;
	
	CHXString strKey;
	
	HX_RESULT res = GetPrefKeyInternal(nIndex, kDontReturnDeeperNestings, strKey);
	if ( SUCCEEDED( res ) )
	{
		res = MakeIHXBuffer((const char *) strKey, pBuffer);
	}
	return res;
}

HX_RESULT CMacPref::GetPrefKeyInternal(UINT32 nIndex, HXBOOL bReturnDeeperNestings, CHXString& outStrKey)
{
	require_nonnull_return(m_pPrefFile, HXR_NOT_INITIALIZED);

	CFArrayRef cfArray;
	HX_RESULT res;
	
	res = HXR_FAIL;
	
	// Get the array of keys from our preferences file, step through the keys, and look for
	// any beginning with the current SubPref prefix.
	//
	// We only want to count and return keys with the matching SubPref prefix that are not 
	// nested more deeply.  For example, for the SubPref  'foo', we want to return 'foo\a' 
	// and 'foo\b' but not 'foo\x\c'
	
	cfArray = m_pPrefFile->CopyKeyList();
	//cfArray = CFPreferencesCopyKeyList(m_cfsPrefsBundleName, m_UserNameRef, m_HostNameRef);
	if (cfArray)
	{
		CFIndex numArrayElements, nCountDown, idx;
		CHXString strCurrKey, strCurrKeyAfterPrefix, strActivePrefix;
		
		// make the current prefix including product name and any subpref
		MakePrefKey("", strActivePrefix);
		
		nCountDown = nIndex; // when nCountDown hits zero, we return the next appropriate key
		
		numArrayElements = CFArrayGetCount(cfArray);
		
		for (idx = 0; idx < numArrayElements; idx++)
		{
			CFStringRef cfsRef; // we're not using CHXCFString since we won't release these; they belong to the array
			
			cfsRef = (CFStringRef) CFArrayGetValueAtIndex(cfArray, idx);
			check_nonnull(cfsRef);
			
			if (cfsRef)
			{
				UINT32 prefixLen, keyLen;
				
				// convert this key to a CHXString
				strCurrKey = cfsRef;
				
				// if the left side of the key matches our current prefix,
				// then this counts
				
				prefixLen = strActivePrefix.GetLength();
				keyLen = strCurrKey.GetLength();

				// check that the prefix matches
				if (prefixLen == 0 || strActivePrefix == strCurrKey.Left( prefixLen ))
				{
					// check that this is not a key for a deeper subpref (if it is,
					// there will be another separator in the key string)
					
					strCurrKeyAfterPrefix = strCurrKey.Right(keyLen - prefixLen);
					
					if (bReturnDeeperNestings ||
						-1 == strCurrKeyAfterPrefix.Find(kPrefPrefixSeparator) )
					{
						if (nCountDown > 0)
						{
							nCountDown--;
						}
						else
						{
							// this is the one to return
							outStrKey = (const char*) strCurrKeyAfterPrefix;
							res = HXR_OK;
							
							// no need to look at others
							break;
						}
					}
				}
			}
		}
		
		CFRelease(cfArray);
	}
	return res;
}


