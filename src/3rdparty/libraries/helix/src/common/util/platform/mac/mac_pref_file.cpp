/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mac_pref_file.cpp,v 1.4 2004/07/09 18:23:03 hubbe Exp $
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

#include "platform/mac/mac_pref_file.h"
#ifndef _MAC_UNIX
#include "filespecutils.h"
#endif

CMacPrefFile*
CMacPrefFile::CreatePrefSystemObject( CFStringRef appIDStringRef, 
	CFStringRef userNameRef, CFStringRef hostNameRef ) // static
{
	HX_RESULT res = HXR_FAIL;
	
	CMacPrefFileCFPref* pObj = new CMacPrefFileCFPref;
	check_nonnull( pObj );
	
	if ( pObj )
	{
		pObj->SetPrefsID( appIDStringRef, userNameRef, hostNameRef );
		res = HXR_OK;
	}
	
	return (CMacPrefFile*) pObj;
}

#ifndef _MAC_UNIX

CMacPrefFile*
CMacPrefFile::CreatePrefXMLObject( CHXFileSpecifier& fileSpec, CFStringRef appIDStringRef, const char* pProductName ) // static
{
	HX_RESULT res = HXR_FAIL;
	
	CMacPrefFileXML* pObj = new CMacPrefFileXML;
	check_nonnull( pObj );
	
	if ( pObj )
	{
		pObj->SetPrefsFileSpec( fileSpec, appIDStringRef, pProductName );
		res = HXR_OK;
	}
	
	return (CMacPrefFile*) pObj;
}

#endif // _MAC_UNIX

#pragma mark -

//////////////////////////////////////////////////////////////////////////////////
//
// CMacPrefFileCFPref methods are just thin wrappers on CFPreferences functions
//

CMacPrefFileCFPref::CMacPrefFileCFPref()
	: m_PrefsAppIDNameRef( NULL ),
	  m_UserNameRef( NULL ),
	  m_HostNameRef( NULL ),
	  m_bInitialized( false )
{

}

CMacPrefFileCFPref::~CMacPrefFileCFPref()
{
	Destroy();
}

void 
CMacPrefFileCFPref::SetPrefsID( CFStringRef appIDStringRef, CFStringRef userNameRef, CFStringRef hostNameRef )
{
	require_nonnull_void_return( appIDStringRef );
	require_nonnull_void_return( userNameRef );
	require_nonnull_void_return( hostNameRef );
	
	Destroy();
	
	m_PrefsAppIDNameRef = appIDStringRef;
	m_UserNameRef = userNameRef;
	m_HostNameRef = hostNameRef;
	
	::CFRetain( (CFStringRef) m_PrefsAppIDNameRef );
	::CFRetain( (CFStringRef) m_UserNameRef );
	::CFRetain( (CFStringRef) m_HostNameRef );
	
	m_bInitialized = true;
}

void
CMacPrefFileCFPref::Destroy()
{
	if ( m_PrefsAppIDNameRef )	::CFRelease( (CFStringRef) m_PrefsAppIDNameRef );
	if ( m_UserNameRef ) 		::CFRelease( (CFStringRef) m_UserNameRef );
	if ( m_HostNameRef ) 		::CFRelease( (CFStringRef) m_HostNameRef );
	
	m_bInitialized = false;
}

CFPropertyListRef
CMacPrefFileCFPref::CopyValue( CFStringRef key )
{
	require_return( m_bInitialized, NULL );
	
	return ::CFPreferencesCopyValue(key, m_PrefsAppIDNameRef, m_UserNameRef, m_HostNameRef );
}

void
CMacPrefFileCFPref::SetValue( CFStringRef key, CFPropertyListRef value )
{
	require_void_return( m_bInitialized );

	::CFPreferencesSetValue( key, value, m_PrefsAppIDNameRef, m_UserNameRef, m_HostNameRef );
}

CFArrayRef
CMacPrefFileCFPref::CopyKeyList( void )
{
	require_return( m_bInitialized, NULL );

	return ::CFPreferencesCopyKeyList( m_PrefsAppIDNameRef, m_UserNameRef, m_HostNameRef );
}

void
CMacPrefFileCFPref::Synchronize( void )
{
	require_void_return( m_bInitialized );

	::CFPreferencesSynchronize( m_PrefsAppIDNameRef, m_UserNameRef, m_HostNameRef );
}

#pragma mark -

#ifndef _MAC_UNIX

//////////////////////////////////////////////////////////////////////////////////
//
// CMacPrefFileXML methods keep the prefs in a dictionary stores as XML in a file
//

/*  Coherence strategy
    
    Because this object is implemented in a static library and included by multiple
    shared libraries, we have to take care that our data (m_DictRef) doesn't
    become stale compared to other instances data.
    
    The strategy we'll use to ensure coherence is:
    - A "coherence value" is a 32-bit non-zero seed that we store via CFPreferences in user prefs
      every time we write out the preferences.  We don't really care if the seed
      is remembered across runs; it just needs to be the same for all running instances
      of this object.  CFPreferences acts as a global cache for the app, so the 
      seed is independent of the object instance and library.
      
    - For every get, if the object's seed doesn't match the seed stored in the user preferences,
      we'll re-load the dictionary from the file prior to reading the caller's key's value.
      
    - For every set, we'll read the pref first to ensure that our dictionary is up-to-date
      and to avoid saving the new set value if it's the same as the old value. If necessary, we'll
      save the new value in our dictionary and write out the XML file, and increment and save
      in CFPreferences our new seed. 
      
 */
    
CMacPrefFileXML::CMacPrefFileXML()
{
	CFIndex noItems = 0;
	
	m_DictRef = ::CFDictionaryCreateMutable( kCFAllocatorDefault, noItems,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks );
	check_nonnull( m_DictRef );
	
	m_CoherenceSeed = 0;
}

CMacPrefFileXML::~CMacPrefFileXML()
{
	if ( m_DictRef )
	{
		::CFRelease( m_DictRef );
	}
	
}

void 
CMacPrefFileXML::SetPrefsFileSpec( const CHXFileSpecifier& fileSpec, CFStringRef appIDStringRef, const char* pProductName )
{
	check( fileSpec.IsSet() );
	require_nonnull_void_return( appIDStringRef );
	
	m_FileSpec = fileSpec;
	
	m_cfsPrefsAppIDNameRef = appIDStringRef;
	::CFRetain( m_cfsPrefsAppIDNameRef );
		
	// the coherence key is the pref key where we save the new coherence seed value after each
	// write; it's something like "UpdateSync"
	
	m_cfsCoherenceKey = CHXString( pProductName ) + "Sync";

	UpdateDictFromFile( true ); // true -> always read from file
}

CFPropertyListRef
CMacPrefFileXML::CopyValue( CFStringRef key )
{
	require_nonnull_return( m_DictRef, NULL );
	
	UpdateDictFromFile( false ); // reads m_DictRect from the file if necessary, false -> only if necessary
	
	CFPropertyListRef value = ::CFDictionaryGetValue( m_DictRef, key );
	
	if ( value )
	{
		::CFRetain( value );
	}
	
	return value;
}

void
CMacPrefFileXML::SetValue( CFStringRef key, CFPropertyListRef value )
{
	require_nonnull_void_return( m_DictRef );
	
	// Because of the expense of saving the dict to disk with each write to maintain
	// coherence, we want to set and save the value only if it differs from what's
	// currently in the dict.
	
	// We have to use CopyValue to get the value out of the dict because
	// CopyValue will ensure the dict is synced with what's been saved to the
	// file.
	
	CFPropertyListRef oldValue = CopyValue( key );
	if ( oldValue == NULL && value == NULL )
	{
		// both unset, so do nothing
	}
	else if ( oldValue && value 
		&& ( kCFCompareEqualTo == ::CFStringCompare( (CFStringRef) oldValue, (CFStringRef) value, (CFStringCompareFlags) 0 ) ) )
	{
		// both set to the same thing, so do nothing
	}
	else
	{
		// change or remove the value in the dictionary
		if (value)
		{
			::CFDictionarySetValue( m_DictRef, key, value );
		}
		else
		{
			::CFDictionaryRemoveValue( m_DictRef, key );
		}
		
		// write immediately to maintain coherence
		WriteToFile();
	}
	
	if ( oldValue )
	{
		::CFRelease( oldValue );
	}
}

CFArrayRef
CMacPrefFileXML::CopyKeyList( void )
{
	require_return( m_DictRef, NULL );
	
	UpdateDictFromFile( false ); // reads m_DictRect from the file if necessary, false -> only if necessary

	CFIndex noItems = 0;

	// we'll build an array of keys by calling CopyKeyListProc repeatedly
	
	CFMutableArrayRef arrayRef = ::CFArrayCreateMutable( kCFAllocatorDefault,
		noItems, &kCFTypeArrayCallBacks );
	if ( arrayRef )
	{
		::CFDictionaryApplyFunction( m_DictRef, CMacPrefFileXML::CopyKeyListProc, (void*) arrayRef );
	}

	return arrayRef;
}

void
CMacPrefFileXML::CopyKeyListProc( const void *key, const void *value, void *context )
{
	check_nonnull( context );
	
	::CFArrayAppendValue( (CFMutableArrayRef) context, key );
}

void
CMacPrefFileXML::Synchronize( void )
{
	// we always write out from SetPref, so all we need to do to sync is read in if necessary
	
	UpdateDictFromFile( false ); // false -> read file only if necessary
}

void
CMacPrefFileXML::WriteToFile( void )
{
	require_nonnull_void_return( m_DictRef );
	require_void_return( m_FileSpec.IsSet() );
	
	// increment and save our coherence seed value; don't let it be zero since
	// that's the value of the member variable when the object is first initialized
	
	m_CoherenceSeed++;
	if ( !m_CoherenceSeed )
	{
		m_CoherenceSeed++;
	}

	CFNumberRef cfnValue = ::CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &m_CoherenceSeed );
	check_nonnull( cfnValue );
	
	if ( cfnValue )
	{
		::CFPreferencesSetValue(m_cfsCoherenceKey, cfnValue, m_cfsPrefsAppIDNameRef, kCFPreferencesCurrentUser, kCFPreferencesAnyHost );
		::CFRelease( cfnValue );
		cfnValue = NULL;
	}
	
	
	// write the dictionary out to the file
	CFDataRef 	xmlData = NULL;
	CFURLRef 	fileURL = NULL;
	CHXString	strPosixPath;
	CFStringRef	posixPathString = NULL;
	OSStatus 	err;
	Boolean		bSuccess;
	const Boolean 	kIsntADir = false;
	
	// we have to use a path in case the file doesn't exist yet, since
	// we can't get an FSRef for a file that doesn't exist
	strPosixPath = m_FileSpec.GetPOSIXPath();
	
	posixPathString = ::CFStringCreateWithCString( kCFAllocatorDefault, (const char *) strPosixPath,
		::CFStringGetSystemEncoding() );
	require_nonnull( posixPathString, Bail );
	
	fileURL = ::CFURLCreateWithFileSystemPath( kCFAllocatorDefault, posixPathString, kCFURLPOSIXPathStyle, kIsntADir );
	require_nonnull( fileURL, Bail );
	
	xmlData = ::CFPropertyListCreateXMLData( kCFAllocatorDefault, m_DictRef );
	require_nonnull( xmlData, Bail );

	bSuccess = ::CFURLWriteDataAndPropertiesToResource( fileURL, xmlData, NULL, &err );
	check( bSuccess );
	
Bail:
	if ( fileURL ) 		::CFRelease( fileURL );
	if ( xmlData ) 		::CFRelease( xmlData );
	if ( posixPathString ) 	::CFRelease( posixPathString );
}

void
CMacPrefFileXML::UpdateDictFromFile( Boolean forceReadFromFile )
{
	require_nonnull_void_return( m_DictRef );
	require_void_return( m_FileSpec.IsSet() );
	
	// check if the coherence seed value matches the one saved in prefs; if not,
	// re-fill the dict from the file, and set our coherence seed to the saved
	// one
	
	SInt32 coherenceValue = GetGlobalCoherenceSeed();
	
	if ( ( coherenceValue != m_CoherenceSeed ) || forceReadFromFile )
	{
		ReadFromFile();
		m_CoherenceSeed = coherenceValue;
	}
	
    	return;
}

SInt32
CMacPrefFileXML::GetGlobalCoherenceSeed()
{
	SInt32 coherenceValue = 0;
	
	CFNumberRef cfnValue = (CFNumberRef) ::CFPreferencesCopyValue(m_cfsCoherenceKey, m_cfsPrefsAppIDNameRef, kCFPreferencesCurrentUser, kCFPreferencesAnyHost );
	if ( cfnValue )
	{
		check( ::CFGetTypeID( cfnValue ) == ::CFNumberGetTypeID() );
		
		Boolean bSuccess = ::CFNumberGetValue( cfnValue, kCFNumberSInt32Type, &coherenceValue );
		check( bSuccess );
	}
	return coherenceValue;
}

void
CMacPrefFileXML::ReadFromFile( void )
{
	require_nonnull_void_return( m_DictRef );
	require_void_return( m_FileSpec.IsSet() );
	
	CFDataRef 		xmlData = NULL;
	CFPropertyListRef 	props = NULL;
	CFURLRef 		fileURL = NULL;
	CFStringRef		errString = NULL;
	FSRef			fileRef;
	OSStatus 		err;
	Boolean			bSuccess;
	
	::CFDictionaryRemoveAllValues( m_DictRef );
	
	require_quiet( CHXFileSpecUtils::FileExists( m_FileSpec ), Bail );
	
	fileRef = ( FSRef ) m_FileSpec;
	
	fileURL = ::CFURLCreateFromFSRef( kCFAllocatorDefault, &fileRef );
	require_nonnull( fileURL, Bail );
	
	// read the XML file
	//
	// the nils are for retrieving property info; could we just use those
	// and skip the call to CFPropertyListCreateFromXMLData?
	
	bSuccess = ::CFURLCreateDataAndPropertiesFromResource( kCFAllocatorDefault,
		fileURL, &xmlData, nil, nil, &err );
		
	require( bSuccess, Bail );

	// make a dictionary from the XML
	props = ::CFPropertyListCreateFromXMLData( kCFAllocatorDefault,
			xmlData, kCFPropertyListMutableContainersAndLeaves, &errString );
	require( ::CFGetTypeID( props ) == ::CFDictionaryGetTypeID(), Bail );
	
	::CFRelease( m_DictRef );
	m_DictRef = (CFMutableDictionaryRef) props;
	::CFRetain( m_DictRef ); // the release below is kept for error-handlign cleanup
	
Bail:
	if ( fileURL ) 		::CFRelease( fileURL );
	if ( xmlData ) 		::CFRelease( xmlData );
	if ( props ) 		::CFRelease( props );
	if ( errString )	::CFRelease( errString );
}

#endif // _MAC_UNIX
