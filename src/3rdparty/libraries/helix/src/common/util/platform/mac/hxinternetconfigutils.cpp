/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxinternetconfigutils.cpp,v 1.6 2005/03/14 19:36:40 bobclark Exp $
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

#include "hxtypes.h"
#include "hxinternetconfigutils.h"
#include "hx_moreprocesses.h" // for GetCurrentAppSignature
#include "string.h"
#ifndef _MAC_MACHO
#include <Processes.h>
#endif
#include "hxstrutl.h"

#ifndef _CARBON
#define ICAttr long
#endif

ICInstance 	HXInternetConfigMac::m_icInstance = NULL;
UInt32 		HXInternetConfigMac::m_refcount = 0;

#ifdef _CARBON
    const ICAttr kNoAttrChange = kICAttrNoChange;
#else
    const ICAttr kNoAttrChange = ICattr_no_change;
#endif	    

static void ConcatPString(StringPtr mainStr, ConstStr255Param suffix)
{
	short len1 = mainStr[0];
	short len2 = suffix[0];
	
	if (len1 + len2 > 255)
	{
		len2 = 255 - len1;
	}
	BlockMoveData(&suffix[1], &mainStr[len1 + 1], len2);
	mainStr[0] += len2;
}

static void CopyPString(ConstStr255Param source, StringPtr dest)
{
	BlockMoveData(source, dest, 1 + source[0]);
}

static Boolean PStringInString(ConstStr255Param searchStr, ConstStr255Param biggerStr)
{
	char csearch[256]; /* Flawfinder: ignore */
	char cbigger[256]; /* Flawfinder: ignore */
	Boolean bFound;
	
	CopyP2CString(searchStr, csearch, 256);
	CopyP2CString(biggerStr, cbigger, 256);
	
	bFound = (NULL != strstr(cbigger, csearch));
	return bFound;
}

Boolean HXInternetConfigMac::StartUsingIC(void)
{
	Boolean		bResult;
	
	bResult = false;
	
	if (!ICStart)
	{
		// we didn't link to InternetConfig
		return false;
	}
	
	// are there other instances already?
	if (m_refcount > 0)
	{
		m_refcount++;
		bResult = true;
	}
	else
	{
		// make a new instance
		OSStatus			status;
		
		if (m_icInstance) return true;
				
		// start internet config
		status = ICStart(&m_icInstance, GetCurrentAppSignature());
		if (status == noErr)
		{
#ifndef _CARBON
			status = ICFindConfigFile(m_icInstance, 0, NULL);
#endif
		}
		else
		{
			(void) ICStop(m_icInstance);
		}
	
		if (status == noErr)
		{
			// IC is started now, so increase the refcount
			m_refcount++;
			bResult = true;
		}

	}
	
	return bResult;
}

void HXInternetConfigMac::StopUsingIC(void)
{
	// are there outstanding instances besides us?
	if (m_refcount > 1)
	{
		m_refcount--;
	}
	else if (m_refcount == 1)
	{
		// we're the last instance, so close our connection
		// to the component
		
		(void) ICStop(m_icInstance);
		
		m_icInstance = NULL;
		m_refcount--;
	}
}

Boolean HXInternetConfigMac::StartReadingICPrefs(void)
{
	OSStatus 	status;
	Boolean		bResult;

	bResult = false;
	if (StartUsingIC())
	{
		status = ICBegin(m_icInstance, icReadOnlyPerm);
		if (status == noErr)
		{
			bResult = true;
		}
	}
	return bResult;
}

Boolean HXInternetConfigMac::StartWritingICPrefs(void)
{
	OSStatus 	status;
	Boolean		bResult;

	bResult = false;
	if (StartUsingIC())
	{
		status = ICBegin(m_icInstance, icReadWritePerm);
		if (status == noErr)
		{
			bResult = true;
		}
	}
	return bResult;
}

void HXInternetConfigMac::StopUsingICPrefs(void)
{
	(void) ICEnd(m_icInstance);
	StopUsingIC();
}

// IsInternetConfigAvailable initializes internet config if necessary
// and returns our connection

Boolean HXInternetConfigMac::IsInternetConfigAvailable(void)
{
	if (StartUsingIC())
	{
		StopUsingIC();
		return true;
	}
	return false;
}

// GetInstance initializes InternetConfig, if necessary, and returns
// our instance of the IC component

ICInstance HXInternetConfigMac::GetInstance(void)
{
	return m_icInstance;
}


// GetFileTypeFromName returns the appropriate file type for
// a given file name, or else zero if none is found

OSType HXInternetConfigMac::GetFileTypeFromName(ConstStr255Param fileName)
{
	ICMapEntry	mapEntry;
	OSStatus	status;
	OSType		result;

	if (!StartUsingIC()) return 0;

	result = (OSType) 0;
	status = ICMapFilename(m_icInstance, fileName, &mapEntry);
	if (status == noErr)
	{
#ifdef _CARBON
		result = mapEntry.fileType;
#else
		result = mapEntry.file_type;
#endif
	}

	StopUsingIC();
	
	return result;
}

// GetFileCreatorFromName returns the appropriate file creator for
// a given file name, or else zero if none is found

OSType HXInternetConfigMac::GetFileCreatorFromName(ConstStr255Param fileName)
{
	ICMapEntry	mapEntry;
	OSStatus	status;
	OSType		result;
	
	if (!StartUsingIC()) return 0;

	result = (OSType) 0;
	status = ICMapFilename(m_icInstance, fileName, &mapEntry);
	if (status == noErr)
	{
#ifdef _CARBON
		result = mapEntry.fileCreator;
#else
		result = mapEntry.file_creator;
#endif
	}
	
	StopUsingIC();
	
	return result;
}

// GetFileCreatorFromName returns the appropriate file creator for
// a given file name, or else zero if none is found

OSErr HXInternetConfigMac::GetFileExtensionFromType(OSType fType, OSType fCreator, ConstStr255Param fileName, StringPtr extension)
{
	ICMapEntry	mapEntry;
	OSStatus	status;
	
	if (!StartUsingIC()) return icInternalErr;

	status = ICMapTypeCreator(m_icInstance, fType, fCreator, fileName, &mapEntry);
	if (status == noErr)
	{
		// copy the extension (pascal string)
		CopyPString(mapEntry.extension, extension);
	}
	
	StopUsingIC();
	
	return (OSErr) status;
}

// LaunchURL launches the URL pointed to by the c-string

OSErr HXInternetConfigMac::LaunchURL(const char *url)
{
	OSStatus status;

	if (!StartUsingIC()) return icInternalErr;
				
	long startSel;
	long endSel;

	startSel = 0;
	endSel = strlen(url);
	status = ICLaunchURL(m_icInstance, "\p", (char *) url, 
						strlen(url), &startSel, &endSel);

	StopUsingIC();
	
	return (OSErr) status;
}


// GetFTPHelper gets the signature and name of the ftp helper app.
// Either parameter can be nil if the result isn't desired.

OSErr HXInternetConfigMac::GetFTPHelper(OSType *signature, StringPtr helperName)
{
	OSStatus status;

	if (!StartUsingIC()) return icInternalErr;
				
	long 		dataSize;
	ICAppSpec	targetSpec;
	ICAttr		icAttr;

	// construct our key string for getting the ftp helper by concatenating
	// ftp to the standard pascal helper prefix
	#define kICHelperFTP kICHelper "ftp"

	dataSize = sizeof(ICAppSpec);
	
	// get the preference information for the ftp helper
	status = ICGetPref(m_icInstance,  kICHelperFTP, &icAttr, (Ptr) &targetSpec, &dataSize);
	if (status == noErr)
	{
		if (signature)	*signature = targetSpec.fCreator;
		if (helperName)	CopyPString(targetSpec.name, helperName);
	}

	StopUsingIC();
	
	return (OSErr) status;
}


// GetHTTPHelper gets the signature and name of the http helper app.
// Either parameter can be nil if the result isn't desired.

OSErr HXInternetConfigMac::GetHTTPHelper(OSType *signature, StringPtr helperName)
{
	OSStatus status;

	if (!StartUsingIC()) return icInternalErr;
				
	long 		dataSize;
	ICAppSpec	targetSpec;
	ICAttr		icAttr;

	// construct our key string for getting the ftp helper by concatenating
	// ftp to the standard pascal helper prefix
	#define kICHelperHTTP kICHelper "http"

	dataSize = sizeof(ICAppSpec);
	
	// get the preference information for the ftp helper
	status = ICGetPref(m_icInstance,  kICHelperHTTP, &icAttr, (Ptr) &targetSpec, &dataSize);
	if (status == noErr)
	{
		if (signature)	*signature = targetSpec.fCreator;
		if (helperName)	CopyPString(targetSpec.name, helperName);
	}

	StopUsingIC();
	
	return (OSErr) status;
}

// GetEmailAddress returns user's email address
// returns FALSE if not found.

HXBOOL HXInternetConfigMac::GetEmailAddress(CHXString& strEmail)
{
	return GetICPreferenceString(kICEmail, strEmail);
}

// GetICPreferenceBoolean gets an IC preference that is a simple string
// returns FALSE if not found
// keys are in InternetConfig.h

HXBOOL HXInternetConfigMac::GetICPreferenceBoolean( ConstStr255Param keyPascalString, HXBOOL& outBool )
{
	OSStatus status;
	HXBOOL bSuccess = FALSE;
	
	outBool = FALSE;
	
	if (StartUsingIC())
	{
				
	    	long 		dataSize = sizeof(Boolean);
	    	ICAttr		icAttr;

		Boolean icBool;	
		
		status = ICGetPref(m_icInstance, keyPascalString, &icAttr, (char*) &icBool, &dataSize);
		
		bSuccess = (status == noErr && dataSize == sizeof(Boolean));
		if (bSuccess)
		{
			outBool = (icBool ? TRUE : FALSE);
		}

		StopUsingIC();
	}
	
	return bSuccess;
}

// GetICPreference gets an IC preference that is a simple string
// returns FALSE if not found
// keys are in InternetConfig.h

HXBOOL HXInternetConfigMac::GetICPreferenceString(ConstStr255Param keyPascalString, CHXString& strResult)
{
	OSStatus status;
	HXBOOL bSuccess = FALSE;
	
	strResult.Empty();

	if (StartUsingIC())
	{
				
	    	long 		textSize = 255;
		ICAttr		icAttr;

		Str255	bufferStr;	
		
		status = ICGetPref(m_icInstance, keyPascalString, &icAttr, (char*) bufferStr, &textSize);
		
		bSuccess = (status == noErr && textSize <= 255);
		if (bSuccess)
		{
			strResult.SetFromStr255(bufferStr);
		}

		StopUsingIC();
	}
	
	return bSuccess;
}

// GetICPreferenceStringList returns a STR#-style IC preference
// returns FALSE if not found
// keys are in InternetConfig.h
// the list is returns with items separated by the outputSeparator character

HXBOOL HXInternetConfigMac::GetICPreferenceStringList(ConstStr255Param keyPascalString, char outputSeparator, CHXString& strResult)
{
	OSStatus status;
	HXBOOL bSuccess = FALSE;
	
	strResult.Empty();

	if (StartUsingIC())
	{
		const int kBuffSize = 1024;
				
	    	long 		dataSize = kBuffSize;
		ICAttr		icAttr;

		char buffer[kBuffSize]; /* Flawfinder: ignore */
		
		status = ICGetPref(m_icInstance, keyPascalString, &icAttr, (char*) buffer, &dataSize);
		
		bSuccess = (status == noErr && dataSize <= kBuffSize && dataSize >= sizeof(short));
		if (bSuccess)
		{
			// buffer is a short number of strings, followed by Pascal strings
			//
			// combine them into the output CHXString, separated by outputSeparator characters
			
			short numItems = *(short *) buffer;
			StringPtr pCurrentStr = (StringPtr) &buffer[2];
			CHXString strTemp;
			
			for (int idx = 0; idx < numItems; idx++)
			{
				strTemp.SetFromStr255(pCurrentStr);
				
				if ((!strTemp.IsEmpty()) && (!strResult.IsEmpty()))
				{
					strResult += outputSeparator;
				}
				strResult += strTemp;
				
				pCurrentStr += (1 + pCurrentStr[0]);
			}
		}

		StopUsingIC();
	}
	
	return bSuccess;
}

// SetProtocol 

const unsigned char kHelper[] = kICHelper; // "\pHelper¥"

const unsigned char kHelperDesc[] = "\pHelperExtra¥"; 
  // in OS 9, this is kICHelperDesc == "\pHelperDesc¥" but for some reason OS X uses "\pHelperExtra¥"


OSErr HXInternetConfigMac::SetHelper(ConstStr255Param protocol, ConstStr255Param protocolDescString, 
	ConstStr255Param appName, OSType appSignature, HXBOOL bReplaceExisting)
{
	OSStatus status;

	if (!StartUsingIC()) return icInternalErr;
	
	Str255 helperStr, helperDescStr, helperDescFullString;
				
	ICAppSpec thisICApp, currICApp;
	ICAttr icAttr;
	long prefSize;

	// first, fill in thisICApp with our app's name and creator
	check(appName[0] < sizeof(thisICApp.name));
	CopyPString(appName, thisICApp.name); // like "RealPlayer"
	thisICApp.fCreator = appSignature;
	
	// next, make the two helper key strings and the protocol string
	check(protocolDescString[0] < 32);
	
	CopyPString(kHelper, helperStr);
	ConcatPString(helperStr, protocol);
	
	CopyPString(kHelperDesc, helperDescStr);
	ConcatPString(helperDescStr, protocol);
	
	CopyPString(protocolDescString, helperDescFullString);
	
	// construct our key string for getting the ftp helper by concatenating
	// ftp to the standard pascal helper prefix
	
	prefSize = sizeof(ICAppSpec);
	status = ICGetPref(m_icInstance, helperStr, &icAttr, (Ptr) &currICApp, &prefSize);
	
	if (status == icPrefNotFoundErr || bReplaceExisting)
	{
	    if (status == noErr)
	    {
	    	// delete the old entry
	    	status = ICDeletePref(m_icInstance, helperStr);

		if (status == noErr)
		{
			status = ICDeletePref(m_icInstance, helperDescStr);
		}
	    }
	    
	    if ( appName[0] != 0 && appSignature != 0 )
	    {
		    // put in our new entry and description
		    status = ICSetPref(m_icInstance, helperStr, kNoAttrChange, (Ptr) &thisICApp, sizeof(ICAppSpec));
		    check_noerr(status);
		    
		    if (status == noErr)
		    {
		    	status = ICSetPref(m_icInstance, helperDescStr, kNoAttrChange, (Ptr) helperDescFullString, 32);
		    }
	    }
	}
	else if (status == 0)
	{
	    status = -1;	// already set and we didn't replace it
	}

	StopUsingIC();
	
	return (OSErr) status;
}

OSErr HXInternetConfigMac::GetHelper(ConstStr255Param protocol, StringPtr outAppName,
	OSType *outAppSignature, StringPtr outProtocolDesc)
{
	OSStatus status;

	if (!StartUsingIC()) return icInternalErr;
	
	Str255 helperStr, helperDescStr, helperDescFullString;
				
	ICAppSpec currICApp;
	ICAttr icAttr;
	long prefSize;

	// next, make the two helper key strings and the protocol string
	
	CopyPString(kHelper, helperStr);
	ConcatPString(helperStr, protocol);
	
	CopyPString(kHelperDesc, helperDescStr);
	ConcatPString(helperDescStr, protocol);
	
	// construct our key string for getting the ftp helper by concatenating
	// ftp to the standard pascal helper prefix
	
	prefSize = sizeof(ICAppSpec);
	status = ICGetPref(m_icInstance, helperStr, &icAttr, (Ptr) &currICApp, &prefSize);
	if (status == noErr)
	{
		prefSize = sizeof(helperDescFullString);
		status = ICGetPref(m_icInstance, helperDescStr, &icAttr, (Ptr) helperDescFullString, &prefSize);
	}
	if (status == noErr)
	{
		if (outAppName) CopyPString(currICApp.name, outAppName);
		if (outAppSignature) *outAppSignature = currICApp.fCreator;
		if (outProtocolDesc) CopyPString(helperDescFullString, outProtocolDesc);
	}

	StopUsingIC();
	
	return (OSErr) status;
}


OSErr HXInternetConfigMac::GetICMapEntryMatchingTemplate(UInt32 searchFromIndex, const ICMapEntry& templateICMapEntry, 
	ICMapEntry& outICMapEntry, UInt32& outICMapEntryIndex, SInt32& outEntryPos)
{
#ifndef _CARBON
	HX_ASSERT(!"HXInternetConfigMac::GetIndexedICMapMatchingTemplate not implemented pre-Carbon");
	return 0;
#else
	OSStatus status = icInternalErr;
	ICAttr mapAttr;
	Handle mapHandle = NULL;
	long numEntries;
	UInt32 idx;
	long entryPos;
	ICMapEntry currEntry;
	OSErr returnResult = -1;
	
	check(templateICMapEntry.fileType || templateICMapEntry.fileCreator || templateICMapEntry.postCreator 
		|| templateICMapEntry.MIMEType[0] || templateICMapEntry.extension[0]);
	
	if (!StartUsingIC()) goto Bail;
	
	mapHandle = ::NewHandle(0);
	if (mapHandle == NULL) goto Bail;
	
	status = ICFindPrefHandle(m_icInstance, kICMapping, &mapAttr, mapHandle);
	if (status != noErr) goto Bail;
	
	numEntries = 0;
	status = ICCountMapEntries(m_icInstance, mapHandle, &numEntries);
	
	// check(searchFromIndex < numEntries); this commonly happens when searching for all matching entries
	
	// retrieve each entry, see if the file_creator or post_creator field points to out app, and if so
	// be sure the bit is set to open the app after downloading
	
	if (status == noErr)
	{
		status = icPrefNotFoundErr;
		for (idx = 1 + searchFromIndex; idx <= numEntries; ++idx)
		{
			status = ICGetIndMapEntry(m_icInstance, mapHandle, idx, &entryPos, &currEntry);
			
			if (status == noErr)
			{
				const Boolean kCaseSensitive = true;
				const Boolean kDiacriticSensitive = true;
				
				Boolean bMatches = true;
				
				// check if it matches the template entry in any non-empty, important fields
				if  ((templateICMapEntry.fileType    != 0 && templateICMapEntry.fileType    != currEntry.fileType)
				  || (templateICMapEntry.fileCreator != 0 && templateICMapEntry.fileCreator != currEntry.fileCreator)
				  || (templateICMapEntry.postCreator != 0 && templateICMapEntry.postCreator != currEntry.postCreator) 
				  || (templateICMapEntry.MIMEType[0] != 0 
					&& !EqualString(templateICMapEntry.MIMEType, currEntry.MIMEType, kCaseSensitive, kDiacriticSensitive))
				  || (templateICMapEntry.extension[0] != 0
					&& !PStringInString(templateICMapEntry.extension, currEntry.extension)))
				{
					bMatches = false;
				}
				
				if (bMatches)
				{
					outICMapEntry = currEntry;
					outICMapEntryIndex = idx;
					outEntryPos = entryPos;
					returnResult = noErr;
					break;
				}
			}
		}	
	}
	

Bail:
	if (mapHandle) DisposeHandle(mapHandle);
	
	StopUsingIC();
	
	return returnResult;
#endif
}

OSErr HXInternetConfigMac::SetICMapEntry(const ICMapEntry * theICMapEntry, SInt32 entryPos, const void* pAppData, Size appDataSize)
{
#ifndef _CARBON
	HX_ASSERT(!"HXInternetConfigMac::SetExistingICMap for Mac OS X only");
	return 0;
#else
	// entryPos of -1 means add new entry rather than replace one at a given position
	// ICMapEntry of NULL means just delete the old entry
	
	OSStatus status = icInternalErr;
	ICAttr mapAttr;
	Handle mapHandle = NULL;
	Boolean icStarted = FALSE;
	
	if (theICMapEntry)
	{
		check(theICMapEntry->fixedLength != 0);
		check(theICMapEntry->fixedLength <= theICMapEntry->totalLength);
		check(theICMapEntry->totalLength == sizeof(ICMapEntry));
	}
	
	icStarted = StartUsingIC();
	require(icStarted, Bail);
	
	mapHandle = ::NewHandle(0);
	require_nonnull(mapHandle, Bail);
	
	status = ICFindPrefHandle(m_icInstance, kICMapping, &mapAttr, mapHandle);
	require_noerr(status, Bail);
	
	if (pAppData && entryPos != -1)
	{
		// since we want to replace old app data as well as the entry, we need
		// to completely delete the old entry (which also deletes the data), 
		// add the new one, find the position of the new one, and add the user data to that
		
		status = ICDeleteMapEntry(m_icInstance, mapHandle, entryPos);
		require_noerr(status, Bail);
		
		entryPos = -1;
	}

	// if entryPos is -1, add a new entry; if theICMapEntry is NULL, delete
	// the entry at entryPos; otherwise, replace the entry at entryPos
	

	if (entryPos != -1 && theICMapEntry != NULL)
	{
		status = ICSetMapEntry(m_icInstance, mapHandle, entryPos, theICMapEntry);
		require_noerr(status, Bail);
	}
	else if (theICMapEntry != NULL)
	{
		status = ICAddMapEntry(m_icInstance, mapHandle, theICMapEntry);
		require_noerr(status, Bail);
	}
	else
	{
		status = ICDeleteMapEntry(m_icInstance, mapHandle, entryPos);
		require_noerr(status, Bail);
	}
	
	if (pAppData)
	{
		check_nonnull(theICMapEntry);
		
		// add is documented to put the new ICMapEntry at the end of the handle
		//
		// first, find the entryPos of the last entry in the handle
		
		long numEntries;
		ICMapEntry tempEntry;
		SInt32 mungeRes;
		
		status = ICCountMapEntries(m_icInstance, mapHandle, &numEntries);
		require_noerr(status, Bail);
		
		status = ICGetIndMapEntry(m_icInstance, mapHandle, numEntries, &entryPos, &tempEntry);
		check_noerr(status);
		check(EqualString(tempEntry.MIMEType, theICMapEntry->MIMEType, true, true)); // be sure it's what we inserted
		
		if (status == noErr)
		{
			// insert the app data after the new ICMapEntry and increase the entry's total length
			
			mungeRes = ::Munger(mapHandle, entryPos + tempEntry.totalLength, NULL, 0, pAppData, appDataSize);
			check(mungeRes >= 0);
			
			if (mungeRes >= 0)
			{
				ICMapEntry* pEntry = (ICMapEntry *) (entryPos + *mapHandle);
				pEntry->totalLength += appDataSize;
			}
		}
	}
	
	status = ICSetPrefHandle(m_icInstance, kICMapping, mapAttr, mapHandle);
	
	
Bail:
	if (mapHandle) DisposeHandle(mapHandle);
	
	if (icStarted) StopUsingIC();
	
	return (OSErr) status;
#endif

}

// SetMappingsToLaunchDownloadedFiles runs through the file helper mappings looking for
// the specified app, and if it finds mappings to the app, modifies the mappings to
// launch the app when the file type is downloaded (and changes the app name in the mapping
// to appName)
OSErr HXInternetConfigMac::SetMappingsToOpenDownloadedFiles(OSType appSignature, ConstStr255Param appName)
{
#ifndef _CARBON
	HX_ASSERT(!"HXInternetConfigMac::SetMappingsToOpenDownloadedFiles for Mac OS X only");
	return 0;
#else
	OSStatus status = icInternalErr;
	ICAttr mapAttr;
	Handle mapHandle = NULL;
	long numEntries;
	long idx, entryPos;
	ICMapEntry currEntry;
	Boolean bChangedMap;
	
	if (!StartUsingIC()) goto Bail;
	
	mapHandle = ::NewHandle(0);
	if (mapHandle == NULL) goto Bail;
	
	status = ICFindPrefHandle(m_icInstance, kICMapping, &mapAttr, mapHandle);
	if (status != noErr) goto Bail;
	
	numEntries = 0;
	status = ICCountMapEntries(m_icInstance, mapHandle, &numEntries);
	
	bChangedMap = false;
	
	// retrieve each entry, see if the file_creator or post_creator field points to out app, and if so
	// be sure the bit is set to open the app after downloading
	
	if (status == noErr)
	{
		for (idx = 1; idx <= numEntries; ++idx)
		{
			status = ICGetIndMapEntry(m_icInstance, mapHandle, idx, &entryPos, &currEntry);
			
			if (status == noErr)
			{
				if (currEntry.fileCreator == appSignature || currEntry.postCreator == appSignature)
				{
					if ((currEntry.flags & kICMapPostMask) != 0
						&& currEntry.postCreator == appSignature)
					{
						// it launches the player, it's good
					}
					else
					{
						currEntry.flags |= kICMapPostMask;
						currEntry.postCreator = appSignature;
						
						CopyPString(appName, currEntry.postAppName);
						
						status = ICSetMapEntry(m_icInstance, mapHandle, entryPos, &currEntry);
						
						bChangedMap = true;
					}
				}
			}
		}	
	}
	
	
	if (bChangedMap)
	{
		status = ICSetPrefHandle(m_icInstance, kICMapping, mapAttr, mapHandle);
	}
	

Bail:
	if (mapHandle) DisposeHandle(mapHandle);
	
	StopUsingIC();
	
	return (OSErr) status;
#endif
}

