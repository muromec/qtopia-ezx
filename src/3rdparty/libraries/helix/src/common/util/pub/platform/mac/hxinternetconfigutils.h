/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxinternetconfigutils.h,v 1.5 2005/03/14 19:36:41 bobclark Exp $
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

#ifndef _MAC_MACHO
#include <InternetConfig.h>
#endif
#include "hxstring.h"

// All methods are static

// Mac OS X note: InternetConfig settings are per-user, not per-machine

class HXInternetConfigMac
{
	public:
		static Boolean 		IsInternetConfigAvailable(void);	// optional
		static ICInstance	GetInstance(void);					// gets our InternetConfig component instance
				
		static OSType		GetFileTypeFromName(ConstStr255Param fileName);		
		static OSType		GetFileCreatorFromName(ConstStr255Param fileName);		

		static OSErr		GetFileExtensionFromType(OSType fType, OSType fCreator, ConstStr255Param fileName, StringPtr extension);
		static OSErr		LaunchURL(const char *url);

		static OSErr		GetFTPHelper(OSType *signature, StringPtr helperName);
		static OSErr		GetHTTPHelper(OSType *signature, StringPtr helperName);

		static HXBOOL 		GetEmailAddress(CHXString& strEmail);
		
		static HXBOOL 		GetICPreferenceString(ConstStr255Param keyPascalString, CHXString& strResult);
		static HXBOOL 		GetICPreferenceStringList(ConstStr255Param keyPascalString, char outputSeparator,
						CHXString& strResult);
		static HXBOOL 		GetICPreferenceBoolean(ConstStr255Param keyPascalString, HXBOOL& outBool);
		
		// file mapping (downloaded file helpers)
		//
		// GetICMapEntryMatchingTemplate search
		//   for speed, put StartUsingIC and StopUsingIC around searches
		//
		//   pass 0 for searchFromIndex initially, then pass outICMapEntryIndex for subsequent searches
		//
		//   initialize templateICMapEntry to 0 except for fields to be compared (OSType fields, extension, or MIMType)
		//
		//   comparison of the extension field is a substring search (like looking for .rm parameter in .rm,.ram,.rpm map entry)
		//
		//   outEntryPos can be passed to SetICMapEntry to replace the existing map entry
		
		static OSErr GetICMapEntryMatchingTemplate(UInt32 searchFromIndex, const ICMapEntry& templateICMapEntry, 
				ICMapEntry& outICMapEntry, UInt32& outICMapEntryIndex, SInt32& outEntryPos);
		
		// SetICMapEntry
		//
		// SetICMapEntry adds or replaces a file helper entry
		//
		// optional app data is included with the entry if pAppData is non-nil
		//
		// pass -1 for entryPos to add a new entry
		
		static OSErr SetICMapEntry(const ICMapEntry * theICMapEntry, SInt32 entryPos, const void* pAppData, 
			Size appDataSize); // entryPos of -1 means add new entry
	
		//  helpers handle the protocols
		//
		//  example: protocol="rtsp" protocolDescString="Real-Time Streaming Protocol"
		//           appName="RealPlayer" appSignature="PNst"
		//
		//  To remove a protocol helper, use SetHelper with appSignature of 0
		
		static OSErr		SetHelper(ConstStr255Param protocol, ConstStr255Param protocolDescString, 
						ConstStr255Param appName, OSType appSignature, HXBOOL bReplaceExisting);
						
		static OSErr 		GetHelper(ConstStr255Param protocol, StringPtr outAppName,
						OSType *outAppSignature, StringPtr outProtocolDesc);

		// SetMappingsToLaunchDownloadedFiles runs through the file helper mappings looking for
		// the specified app, and if it finds mappings to the app, modifies the mappings to
		// launch the app when the file type is downloaded (and changes the app name in the mapping
		// to appName). Implemented on Carbon only.
		
		static OSErr 		SetMappingsToOpenDownloadedFiles(OSType appSignature, ConstStr255Param appName);

		// put StartUsingIC/StopUsingIC calls around the other IC routines
		// if the others will be made repeatedly
		
		static Boolean		StartUsingIC(void);		// optional; call before batches of IC calls
		static void		StopUsingIC(void);		// balances the Start call

		// put StartReadingICPrefs/StartWritingICPrefs - StopUsingIC calls around calls that read or write IC prefs
		// if the calls will be made repeatedly. This avoid repeated opening and
		// closing of the IC preferences file. 
		//
		// Do not call WaitNextEvent between these calls.
		//
		// There's no need to call StartUsingIC/StopUsingIC if
		// you make these start/stop calls.
		
		static Boolean		StartReadingICPrefs(void); // optional; call before batches of IC prefs calls
		static Boolean 		StartWritingICPrefs(void); // same as previous but for writing prefs
		static void 		StopUsingICPrefs(void);    // balances the preceding two calls

	private:
		static ICInstance	m_icInstance;
		static UInt32		m_refcount;
};
