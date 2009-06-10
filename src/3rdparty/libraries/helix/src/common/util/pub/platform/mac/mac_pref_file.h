/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mac_pref_file.h,v 1.5 2007/07/06 20:39:27 jfinnecy Exp $
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

#include "filespec.h"
#include "cfwrappers.h"

class CMacPrefFile
{
    public:
    	static CMacPrefFile* CreatePrefXMLObject( CHXFileSpecifier& fileSpec, CFStringRef appIDStringRef, const char* pProductName );
    	static CMacPrefFile* CreatePrefSystemObject( CFStringRef appIDStringRef, CFStringRef userNameRef, CFStringRef hostNameRef );
    	
    	CMacPrefFile() {};
	virtual ~CMacPrefFile() {};
	
	// The following public APIs are meant to mirror the CFPreferences APIs
	
	virtual CFPropertyListRef CopyValue( CFStringRef key ) = 0;
	virtual void SetValue( CFStringRef key, CFPropertyListRef value ) = 0;
	virtual void Synchronize( void ) = 0;
	
	virtual CFArrayRef CopyKeyList() = 0;
};

//
// CMacPrefFileCFPref is the version that uses CFPreferences directly
//

class CMacPrefFileCFPref : public CMacPrefFile
{
    public:
    	CMacPrefFileCFPref();
	virtual ~CMacPrefFileCFPref();
		
	void SetPrefsID( CFStringRef appIDStringRef, CFStringRef userNameRef, CFStringRef hostNameRef );
	
	virtual CFPropertyListRef CopyValue( CFStringRef key );
	virtual void SetValue( CFStringRef key, CFPropertyListRef value );
	
	virtual CFArrayRef CopyKeyList();
	virtual void Synchronize( void );
	

    private:
    	void Destroy();
    		
	// these are set by SetPrefsID...
	CFStringRef 		m_PrefsAppIDNameRef;
	CFStringRef 		m_UserNameRef;
	CFStringRef 		m_HostNameRef;
	
	Boolean			m_bInitialized;
};

#ifndef _MAC_UNIX

//
// CMacPrefFileXML uses a CFDictionary and writes to an XML file
//

class CMacPrefFileXML : public CMacPrefFile
{
    public:
    	CMacPrefFileXML();
	virtual ~CMacPrefFileXML();
		
	void SetPrefsFileSpec( const CHXFileSpecifier& fileSpec, CFStringRef appIDStringRef, const char* pProductName );
	
	virtual CFPropertyListRef CopyValue( CFStringRef key );
	virtual void SetValue( CFStringRef key, CFPropertyListRef value );
	
	virtual CFArrayRef CopyKeyList();
	virtual void Synchronize( void );
	

    private:
    	static void CopyKeyListProc( const void *key, const void *value, void *context ); // used by CopyKeyList
    	
    	void WriteToFile();
    	void ReadFromFile();
    	void UpdateDictFromFile( Boolean forceReadFromFile );
    	void WriteToFileIfNecessary();
    	SInt32 GetGlobalCoherenceSeed();
    	
	// these are set by SetPrefsFileSpec...
	CHXFileSpecifier	m_FileSpec;
	CFMutableDictionaryRef	m_DictRef;
	
	SInt32			m_CoherenceSeed;    // matches the sync seed value in user prefs if our dictionary is up-to-date
	CHXCFString		m_cfsCoherenceKey;   // key for sync seed value in the user prefs
	CHXCFString 		m_cfsPrefsAppIDNameRef;
	
};

#endif
