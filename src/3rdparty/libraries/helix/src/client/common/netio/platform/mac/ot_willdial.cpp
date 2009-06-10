/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ot_willdial.cpp,v 1.6 2007/07/06 21:57:58 jfinnecy Exp $
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
 * developer and/or licensor of the Original Code and owns the
 * copyrights in the portions it created.
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

/*
	This code was descended from Apple Sample Code, but the
	code may have been modified.
*/

/*
	File:		OTTCPWillDial.c

	Contains:	Library to determine whether open a TCP endpoint will
				dial the modem.

	Written by:	Quinn "The Eskimo!"

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

	You may incorporate this sample code into your applications without
	restriction, though the sample code has been provided "AS IS" and the
	responsibility for its operation is 100% yours.  However, what you are
	not permitted to do is to redistribute the source as "DSC Sample Code"
	after having made changes. If you're going to re-distribute the source,
	we require that you make it clear in the source that the code was
	descended from Apple Sample Code, but that you've made changes.
*/

/////////////////////////////////////////////////////////////////

//#define qDebug 1
#include "hxtypes.h"

#ifndef _MAC_MACHO
#include <MacTypes.h>

/////////////////////////////////////////////////////////////////
// Pick up lots of OT interfaces.

#import <OpenTransport.h>
#import <OpenTptInternet.h>
#import <OpenTptLinks.h>
#import <OTDebug.h>

/////////////////////////////////////////////////////////////////
// Pick up standard system interfaces.

#import <CodeFragments.h>
#import <Resources.h>
#import <Errors.h>
#import <Folders.h>

#endif // ifndef _MAC_MACHO

/////////////////////////////////////////////////////////////////
// Pick up OT configuration database stuff.

#import "NetworkSetupTypes.h"

/////////////////////////////////////////////////////////////////
// Pick up our own header file.

#import "OTTCPWillDial.h"

#include "HX_MoreProcesses.h"

/////////////////////////////////////////////////////////////////
// Default OTDebugStr prototype, because it's not in the interfaces.

extern void OTDebugStr(const char *message);

#ifdef _CARBON
// define empty macros which are otherwise undefined in Carbon.
#define OTAssert(name, cond)
#define OTDebugBreak(str)
#endif

// NetworkSetup loading
//
// PR 50384
//
// The following ugliness is because, if we link to the NetworkSetup library, we'll fail to load if
// extensions are disabled with the shift key, since the link to NetworkSetup will succeed by NetworkSetup
// will fail to prepare successfully.  Instead, we'll manually load all the NetworkSetup symbols that we need.
//
// This is called from OTTCPWillDial because that is the only external entry point to this file.
//
// GR 7/25/01

typedef OSStatus OTCfgOpenPrefsType(CfgDatabaseRef dbRef, const CfgEntityRef *	entityRef, Boolean writer, CfgEntityAccessID *	accessID);
typedef OSStatus OTCfgGetPrefsSizeType(CfgEntityAccessID accessID, OSType prefsType, ByteCount * length);
typedef OSStatus OTCfgGetPrefsType(CfgEntityAccessID accessID, OSType prefsType, void * data, ByteCount length);
typedef OSStatus OTCfgClosePrefsType(CfgEntityAccessID accessID);
typedef OSStatus OTCfgOpenDatabaseType(CfgDatabaseRef *	dbRef);
typedef OSStatus OTCfgGetEntitiesCountType(CfgDatabaseRef dbRef, CfgAreaID areaID, CfgEntityClass entityClass, CfgEntityType entityType, ItemCount * itemCount);
typedef OSStatus OTCfgGetEntitiesListType(CfgDatabaseRef dbRef, CfgAreaID areaID, CfgEntityClass entityClass, CfgEntityType entityType, ItemCount * itemCount, CfgEntityRef entityRef[], CfgEntityInfo entityInfo[]);
typedef OSStatus OTCfgGetCurrentAreaType(CfgDatabaseRef dbRef, CfgAreaID * areaID);
typedef OSStatus OTCfgOpenAreaType(CfgDatabaseRef dbRef, CfgAreaID areaID);
typedef OSStatus OTCfgCloseAreaType(CfgDatabaseRef dbRef, CfgAreaID areaID);
typedef OSStatus OTCfgCloseDatabaseType(CfgDatabaseRef * dbRef);

OTCfgOpenPrefsType *zpOTCfgOpenPrefs = 0;
OTCfgGetPrefsSizeType *zpOTCfgGetPrefsSize = 0;
OTCfgGetPrefsType *zpOTCfgGetPrefs = 0;
OTCfgClosePrefsType *zpOTCfgClosePrefs = 0;
OTCfgOpenDatabaseType *zpOTCfgOpenDatabase = 0;
OTCfgGetEntitiesCountType *zpOTCfgGetEntitiesCount = 0;
OTCfgGetEntitiesListType *zpOTCfgGetEntitiesList = 0;
OTCfgGetCurrentAreaType *zpOTCfgGetCurrentArea = 0;
OTCfgOpenAreaType *zpOTCfgOpenArea = 0;
OTCfgCloseAreaType *zpOTCfgCloseArea = 0;
OTCfgCloseDatabaseType *zpOTCfgCloseDatabase = 0;

OSErr LoadNetworkSetupSymbols()
{
	CFragConnectionID connID;
	Ptr mainAddr;
	Str255 errStr;
	OSErr err;
	CFragSymbolClass symClass;
	
	if (zpOTCfgOpenPrefs != 0) return noErr;
	
	err = GetSharedLibrary("\pCfgOpenTpt", kPowerPCCFragArch, kLoadCFrag + kFindCFrag, &connID, &mainAddr, errStr);
	if (err == noErr)
	{
		err = FindSymbol(connID, "\pOTCfgOpenPrefs", (Ptr *) &zpOTCfgOpenPrefs, &symClass);
		err = FindSymbol(connID, "\pOTCfgGetPrefsSize", (Ptr *) &zpOTCfgGetPrefsSize, &symClass);
		err = FindSymbol(connID, "\pOTCfgGetPrefs", (Ptr *) &zpOTCfgGetPrefs, &symClass);
		err = FindSymbol(connID, "\pOTCfgClosePrefs", (Ptr *) &zpOTCfgClosePrefs, &symClass);
		err = FindSymbol(connID, "\pOTCfgOpenDatabase", (Ptr *) &zpOTCfgOpenDatabase, &symClass);
		err = FindSymbol(connID, "\pOTCfgGetEntitiesCount", (Ptr *) &zpOTCfgGetEntitiesCount, &symClass);
		err = FindSymbol(connID, "\pOTCfgGetEntitiesList", (Ptr *) &zpOTCfgGetEntitiesList, &symClass);
		err = FindSymbol(connID, "\pOTCfgGetCurrentArea", (Ptr *) &zpOTCfgGetCurrentArea, &symClass);
		err = FindSymbol(connID, "\pOTCfgOpenArea", (Ptr *) &zpOTCfgOpenArea, &symClass);
		err = FindSymbol(connID, "\pOTCfgCloseArea", (Ptr *) &zpOTCfgCloseArea, &symClass);
		err = FindSymbol(connID, "\pOTCfgCloseDatabase", (Ptr *) &zpOTCfgCloseDatabase, &symClass);
	}
	return err;
}

/////////////////////////////////////////////////////////////////
// Common code to parse 'iitf' preferences.

// The structure of an 'iitf' is the same regardless of whether it
// comes from a resource or from the configuration database.

// An 'iitf' preference consists of a UInt16 count followed
// by 0 or more interface specifications.  Each interface
// specification is a variable length data structure, with
// some fixed length and some variable length fields.
// This structure is used to represent an interface as 
// a fixed size data structure, much more suitable for
// C programming.

// In current versions of OT, only one interface is allowed.

struct TCPiitfPref {
	UInt8    fActive;
	InetHost fIPAddress;
	InetHost fSubnetMask;
	Str255   fAppleTalkZone;
	UInt8    fPath[36];			// Pascal string
	UInt8    fModuleName[31];	// Pascal string
	UInt32   fFramingFlags;
};
typedef struct TCPiitfPref TCPiitfPref;

static void UnpackIITF(Ptr *buffer, TCPiitfPref *unpackedIITF)
	// This routine unpacks an interface from an 'iitf' preference
	// into a TCPiitfPref.  *buffer must point to the beginning
	// of the interface, ie two bytes into the pref data if
	// if you're extracting the first interface.  *buffer
	// is updated to point to the byte after the last byte
	// parsed, so you can parse multiple interfaces by
	// repeatedly calling this routine.
{
	UInt8 *cursor;
	
	cursor = (UInt8 *) *buffer;
	
	unpackedIITF->fActive = *cursor;
	cursor += sizeof(UInt8);
	unpackedIITF->fIPAddress = *((InetHost *) cursor);
	cursor += sizeof(InetHost);
	unpackedIITF->fSubnetMask = *((InetHost *) cursor);
	cursor += sizeof(InetHost);
	BlockMoveData(cursor, unpackedIITF->fAppleTalkZone, *cursor + 1);
	cursor += (*cursor + 1);
	BlockMoveData(cursor, unpackedIITF->fPath, 36);
	cursor += 36;
	BlockMoveData(cursor, unpackedIITF->fModuleName, 32);
	cursor += 32;
	unpackedIITF->fFramingFlags = *((UInt32 *) cursor);
	cursor += sizeof(UInt32);

	*buffer = (Ptr) cursor;
}

static OSStatus GetPortNameFromIITF(Ptr buffer, SInt32 prefSize, char *portName)
	// This routine takes the address and size of an 'iitf' preference
	// and extracts the port name from the first interface.
{
	OSStatus err;
	UInt16 interfaceCount;
	Ptr cursor;
	TCPiitfPref firstInterface;
	UInt8 portNameLength;
	
	// Get the count of interfaces, checking for possibly bogus
	// preference data.
	
	err = noErr;
	if (prefSize < sizeof(UInt16)) {
		err = -1;
	}
	if (err == noErr) {
		interfaceCount = *((UInt16 *)buffer);
		if (interfaceCount < 1) {
			err = -1;
		}
	}
	
	// Unpack the first interface out of the 'iitf'.
	
	if (err == noErr) {
		cursor = buffer + sizeof(UInt16);
		UnpackIITF(&cursor, &firstInterface);

		OTAssert("GetPortNameFromIITF: Did not consume correct number of bytes",
					interfaceCount > 1 || (cursor == buffer + prefSize) );
	}
	
	// Copy the port name out of the unpacked interface.
	
	if (err == noErr) {
		portNameLength = firstInterface.fPath[0];
		if ( portNameLength > kMaxProviderNameLength) {
			err = -1;
		} else {

			// Poor Man's C2PString avoids me having to figure
			// out which wacky library CodeWarrior wants me to link with
			// today!
			
			BlockMoveData(firstInterface.fPath + 1, portName, portNameLength);
			portName[ portNameLength ] = 0;
		}
	}

	return err;
}

/////////////////////////////////////////////////////////////////
// OT configuration database implementation.

static OSStatus GetFixedSizePref(CfgDatabaseRef ref, const CfgEntityRef *entityID, OSType prefType,
						void *buffer, ByteCount prefSize)
	// This routine gets a fixed size preference out of
	// the configuration database described by ref.  entityID
	// is the entity containing the preference.  prefType is the
	// type of preference within the entity.  buffer is the address
	// where the preference data should be put.  prefSize is the size
	// of the buffer, and the routine validates that the preference
	// is exactly that size.
{
	OSStatus err;
	OSStatus err2;
	CfgEntityAccessID prefsRefNum;
	ByteCount actualPrefSize;
	
	OTAssert("GetFixedSizePref: paramErr", buffer != nil);

	// Open the entity, read out the preference, and then
	// close it down.
	
	err = (*zpOTCfgOpenPrefs)(ref, entityID, false, &prefsRefNum);
	if (err == noErr) {
		err = (*zpOTCfgGetPrefsSize)(prefsRefNum, prefType, &actualPrefSize);
		if (err == noErr && actualPrefSize != prefSize) {
			err = -1;
		}
		if (err == noErr) {
			err = (*zpOTCfgGetPrefs)(prefsRefNum, prefType, buffer, prefSize);
		}
	
		err2 = (*zpOTCfgClosePrefs)(prefsRefNum);
		if (err == noErr) {
			err = err2;
		}
	}
	
	return err;
}

static OSStatus GetPref(CfgDatabaseRef ref, const CfgEntityRef *entityID, OSType prefType,
						void **buffer, ByteCount *prefSize)
	// This routine gets a variable size preference out of
	// the configuration database described by ref.  entityID
	// is the entity containing the preference.  prefType is the
	// type of preference within the entity.  buffer is the address
	// a pointer where the address of the newly allocated preference
	// buffer should be put.  prefSize is the address of a variable
	// where the size of the newly allocated preference should be.
	// 
	// The caller is responsible for disposing of the preference buffer
	// using OTFreeMem.  If the routine fails, no preference buffer is 
	// returned.
{
	OSStatus err;
	OSStatus err2;
	CfgEntityAccessID prefsRefNum;
	
	OTAssert("GetPref: paramErr", buffer != nil);

	// Open the entity, read out the preference, and then
	// close it down.
	
	*buffer = nil;	
	err = (*zpOTCfgOpenPrefs)(ref, entityID, false, &prefsRefNum);
	if (err == noErr) {
		err = (*zpOTCfgGetPrefsSize)(prefsRefNum, prefType, prefSize);

		if (err == noErr) {
			//*buffer = OTAllocMem(*prefSize);
			*buffer = new unsigned char[*prefSize];
			if (*buffer == nil) {
				err = kOTOutOfMemoryErr;
			}
		}
		if (err == noErr) {
			err = (*zpOTCfgGetPrefs)(prefsRefNum, prefType, *buffer, *prefSize);
		}
	
		err2 = (*zpOTCfgClosePrefs)(prefsRefNum);
		if (err == noErr) {
			err = err2;
		}
	}
	
	// Clean up.
	
	if (err != noErr && *buffer != nil) {
		//OTFreeMem(*buffer);
		delete [] (*buffer);
		*buffer = nil;
	}
	return err;
}

static OSStatus GetEntityList(CfgDatabaseRef ref, CfgAreaID area,
								OSType otclass, OSType ottype,
								CfgEntityRef **entityIDs, ItemCount *entityCount)
	// This routine gets a list of all the entities that match
	// class and type in the specified area of the specified database.
	// It allocates a buffer (using OTAllocMem) to hold the CfgEntityRef's
	// for the result and sets *entityIDs to point to the buffer.  It sets
	// entityCount to the number of CfgEntityRef's in the buffer.
{
	OSStatus err;
	CfgEntityInfo *junkEntityInfos;

	OTAssert("GetEntityList: paramErr", entityIDs != nil);
	OTAssert("GetEntityList: paramErr", entityCount != nil);
	
	*entityIDs = nil;
	junkEntityInfos = nil;
	
	err = (*zpOTCfgGetEntitiesCount)(ref, area, otclass, ottype, entityCount);
	if (err == noErr) {
//		*entityIDs = (CfgEntityRef *) OTAllocMem(*entityCount * sizeof(CfgEntityRef));
		*entityIDs = (CfgEntityRef *) new unsigned char[*entityCount * sizeof(CfgEntityRef)];

//		junkEntityInfos = (CfgEntityInfo *) OTAllocMem(*entityCount * sizeof(CfgEntityInfo));
		junkEntityInfos = (CfgEntityInfo *) new unsigned char[*entityCount * sizeof(CfgEntityInfo)];
		if (*entityIDs == nil || junkEntityInfos == nil) {
			err = kOTOutOfMemoryErr;
		}
	}
	if (err == noErr) {
	
		// I'm not sure whether you can pass nil to the entityInfos parameter
		// of OTCfgGetEntitiesList, so for the moment I'm passing in a valid
		// buffer.  I'll fix this up pending confirmation from engineering
		// that nil is OK.
		
		err = (*zpOTCfgGetEntitiesList)(ref, area, 
					otclass, ottype, 
					entityCount, *entityIDs, junkEntityInfos);
	}
	
	// Clean up.
	
	if (junkEntityInfos != nil) {
		//OTFreeMem(junkEntityInfos);
		delete [] (junkEntityInfos);
	}
	
	if (err != noErr) {
		if (*entityIDs != nil) {
			//OTFreeMem(*entityIDs);
			delete [] (*entityIDs);
			*entityIDs = nil;
		}
	}

	return err;
}

static OSStatus GetInfoForTCPEntity(CfgDatabaseRef ref, const CfgEntityRef *entityID,
									Boolean *enabled, char *portName)
	// This routine returns the enabled status and port name
	// for the TCP/IP preferences entity described by entityID
	// in the ref database.
{	
	OSStatus err;
	SInt16 enabledInt;
	Ptr buffer;
	ByteCount prefSize;

	buffer = nil;

	// First return enabled using the simple API.
	
	err = GetFixedSizePref(ref, entityID, 'unld', &enabledInt, sizeof(SInt16));
	if (err == noErr) {
		*enabled = (enabledInt != 3);
	}
	
	// Now return the port name.  Now call the variable sized
	// API to get the 'iitf' resource and then extract the port name 
	// from the preference buffer.
	
	if (err == noErr) {
#ifdef _CARBON
		err = GetPref(ref, entityID, 'iitf', &(void*)buffer, &prefSize);
#else
		err = GetPref(ref, entityID, 'iitf', &buffer, &prefSize);
#endif
	}
	if (err == noErr) {
		err = GetPortNameFromIITF(buffer, prefSize, portName);
	}
	
	// Clean up.
	
	if (buffer != nil) {
		//OTFreeMem(buffer);
		delete [] (buffer);
	}
	return err;
}

static OSStatus FindActiveSet(CfgDatabaseRef ref, CfgAreaID area, CfgEntityRef *activeSet)
	// This routine finds the entity ref of the active set entity
	// in the database.  It works by finding all the set entities
	// (there is generally only one in the current OT implementation)
	// and checks each one for the active bit set in its flags.
	// It returns the first set that claims to be active.
{
	OSStatus err;
	ItemCount setCount;
	CfgEntityRef *setEntities;
	Boolean found;
	ItemCount thisSetIndex;
	CfgSetsStruct thisStruct;

	setEntities = nil;

	err = GetEntityList(ref, area, kOTSetOfSettingsClass, kOTSetOfSettingsType, &setEntities, &setCount);
	if (err == noErr) {
		thisSetIndex = 0;
		found = false;
		while (err == noErr && thisSetIndex < setCount && ! found) {
			err = GetFixedSizePref(ref, &setEntities[thisSetIndex], kPrefsTypeStruct,
							&thisStruct, sizeof(thisStruct));
			if (err == noErr) {
				found = ((thisStruct.fFlags & (1 << kSetsIndexActive)) != 0);
				if ( ! found ) {
					thisSetIndex += 1;
				}
			}
		}
		if (err == noErr && ! found) {
			err = -1;
		}
	}
	if (err == noErr) {
		*activeSet = setEntities[thisSetIndex];
	}

	// Clean up.
	
	if (setEntities != nil) {
		//OTFreeMem(setEntities);
		delete [] (setEntities);
	}
	
	return err;
}

static OSStatus FindCurrentTCPEntity(CfgDatabaseRef ref, CfgAreaID area, CfgEntityRef *currentTCPEntity)
	// This routine finds the current active TCP/IP connection entity.
	// It does this by first looking up the active set, then getting
	// the list of entities out of the active set, then searching
	// through that list of entities for the first TCP/IP connection
	// entity.
{
	OSStatus err;
	CfgEntityRef activeSet;
	CfgSetsVector *vectorPrefData;
	ByteCount vectorPrefSize;
	Boolean found;
	ItemCount thisElementIndex;
	CfgEntityInfo thisEntityInfo;
	
	vectorPrefData = nil;

	err = FindActiveSet(ref, area, &activeSet);
	if (err == noErr) {
#ifdef _CARBON
		err = GetPref(ref, &activeSet, kPrefsTypeVector,
						&(void*)vectorPrefData, &vectorPrefSize);
#else
		err = GetPref(ref, &activeSet, kPrefsTypeVector,
						&vectorPrefData, &vectorPrefSize);
#endif
	}
	if (err == noErr) {

		// The kOTSetOfSettingsClass/kOTSetOfSettingsType preference
		// data is a count of elements followed by an array of that
		// many elements.  We walk index through the array looking
		// for the first TCP/IP connection entity.
		
		thisElementIndex = 0;
		found = false;
		while ( thisElementIndex < vectorPrefData->fCount && ! found ) {
			thisEntityInfo = vectorPrefData->fElements[thisElementIndex].fEntityInfo;
			found = (thisEntityInfo.fClass == kOTNetworkConnectionClass) 
						&& (thisEntityInfo.fType == kOTTCPv4NetworkConnection);
			if (found) {
				*currentTCPEntity = vectorPrefData->fElements[thisElementIndex].fEntityRef;
				
				// A weird misfeature of kOTSetOfSettingsClass/kOTSetOfSettingsType 
				// preference is that the CfgEntityRef's it holds have their area
				// (ie fLoc) set to a bogus area ID.  [It's actually the area ID
				// of the temporary area generated when the person who wrote the
				// set called OTCfgBeginAreaModifications.]  So we have to reset
				// this to the current area before returning it to our caller.
				
				currentTCPEntity->fLoc = area;
			} else {
				thisElementIndex += 1;
			}
		}
		if ( err == noErr && ! found ) {
			err = -3;
		}
	}
	
	// Clean up.
	
	if (vectorPrefData != nil) {
		//OTFreeMem(vectorPrefData);
		delete [] (vectorPrefData);
	}
	return err;
}

static OSStatus GetTCPInfoUsingAPI(Boolean *enabled, char *portName)
	// The high-level entry point into the configuration database
	// implementation.  We open the database, find the current
	// TCP entity and read the info we need out of that entity.
{
	OSStatus err;
	OSStatus err2;
	CfgDatabaseRef ref;
	CfgAreaID currentArea;
	CfgEntityRef currentTCPEntity;
	
	err = (*zpOTCfgOpenDatabase)(&ref);
	if (err == noErr) {
		err = (*zpOTCfgGetCurrentArea)(ref, &currentArea);
		if (err == noErr) {
			err = (*zpOTCfgOpenArea)(ref, currentArea);
			if (err == noErr) {
				err = FindCurrentTCPEntity(ref, currentArea, &currentTCPEntity);
				if (err == noErr) {
					err = GetInfoForTCPEntity(ref, &currentTCPEntity, enabled, portName);
				}
				
				err2 = (*zpOTCfgCloseArea)(ref, currentArea);
				if (err == noErr) {
					err = err2;
				}
			}
		}
	
		err2 = (*zpOTCfgCloseDatabase)(&ref);
		if (err == noErr) {
			err = err2;
		}
	}
	return err;
}

/////////////////////////////////////////////////////////////////
// Implementation that reads the TCP/IP Preferences file directly.

// You have to search for the preferences file by type and creator
// because the name will be different on localised systems.

enum {
	kOTTCPPrefFileType = 'pref',
	kOTTCPPrefFileCreator = 'ztcp'
};

static OSStatus FindTCPPrefFile(FSSpec *fss)
	// This routine scans the Preferences folder looking
	// for the "TCP/IP Preferences" file by type and creator.
{
	OSStatus err;
	Boolean found;
	CInfoPBRec cpb;
	SInt16 index;
	
	err = FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, &fss->vRefNum, &fss->parID);
	if (err == noErr) {
		found = false;
		index = 1;
		do {
			cpb.hFileInfo.ioVRefNum = fss->vRefNum;
			cpb.hFileInfo.ioDirID = fss->parID;
			cpb.hFileInfo.ioNamePtr = fss->name;
			cpb.hFileInfo.ioFDirIndex = index;
			err = PBGetCatInfoSync(&cpb);
			if (err == noErr) {
				found = (	cpb.hFileInfo.ioFlFndrInfo.fdType == kOTTCPPrefFileType &&
							cpb.hFileInfo.ioFlFndrInfo.fdCreator == kOTTCPPrefFileCreator );
			}
			index += 1;
		} while (err == noErr & ! found);
	}
	return err;
}

static OSStatus CheckResError(void *testH)
	// A trivial wrapper routine for ResError,
	// which is too lame to report an error code
	// in all cases when GetResource fails.
{
	OSStatus err;

	err = ResError();
	if (err == noErr && testH == nil) {
		err = resNotFound;
	}
	return err;
}

static OSStatus GetTCPInfoFromFile(Boolean *enabled, char *portName)
	// This is the high-level entry point into the direct file
	// access implementation.  It simply finds the preferences
	// file and reads the preferences out directly.
{
	OSStatus err;
	FSSpec fss;
	SInt16 oldResFile;
	SInt16 prefResFile;
	Handle currentConfigResourceH;
	Handle unldResource;
	Handle iitfResource;
	SInt8  s;
	
	oldResFile = CurResFile();
	
	err = FindTCPPrefFile(&fss);
	if (err == noErr) {
		prefResFile = FSpOpenResFile(&fss, fsRdPerm);
		err = ResError();
	}
	if (err == noErr) {

		currentConfigResourceH = Get1Resource('ccfg', 1);
		err = CheckResError(currentConfigResourceH);

		if (err == noErr && GetHandleSize(currentConfigResourceH) != sizeof(SInt16) ) {
			OTDebugBreak("GetTCPInfoFromFile: 'ccfg' is of the wrong size");
			err = -1;
		}

		if (err == noErr) {
			unldResource = Get1Resource('unld', **( (SInt16 **) currentConfigResourceH));
			err = CheckResError(unldResource);
		}
		if (err == noErr) {
			*enabled = ( **((SInt16 **) unldResource) != 3);
		}

		if (err == noErr) {
			iitfResource = Get1Resource('iitf', **( (SInt16 **) currentConfigResourceH));
			err = CheckResError(iitfResource);
		}

		if (err == noErr) {
			s = HGetState(iitfResource);
			HLock(iitfResource);
			err = GetPortNameFromIITF(*iitfResource, GetHandleSize(iitfResource), portName);
			HSetState(iitfResource, s);
		}
		
		CloseResFile(prefResFile);
		OTAssert("GetTCPInfoFromFile: Failed to close prefResFile", ResError() == noErr);
	}
	
	UseResFile(oldResFile);
	OTAssert("GetTCPInfoFromFile: Could not re-establish CurResFile", ResError() == noErr);
	
	return err;
}

/////////////////////////////////////////////////////////////////
// Code that's common to both implementations.

static OSStatus GetTCPInfo(Boolean *enabled, char *portName)
	// A dispatcher.  If the config database is available,
	// we call it, otherwise we fall back to reading the
	// preferences file directly.
{
	OSStatus err;
	
	if ( (void *) zpOTCfgOpenDatabase == (void *) kUnresolvedCFragSymbolAddress) {
		err = GetTCPInfoFromFile(enabled, portName);
	} else {
		err = GetTCPInfoUsingAPI(enabled, portName);
	}
	return err;
}

// If you set kUseInetInterfaceInfo to false, OTTCPWillDial will not
// use the heuristic of "if the TCP/IP stack is loaded, it's safe
// to open an endpoint".  This is especially useful when debugging.

const Boolean kUseInetInterfaceInfo = true;


#ifdef __cplusplus
extern "C" {
#endif

OSStatus OTTCPWillDial(ULONG32 *willDial)
//extern OSStatus OTTCPWillDial(UInt32 *willDial)
	// The main entry point.  We call our core
	// implementation and then generate the result
	// based on the returned information.
{
	OSStatus err;
	InetInterfaceInfo info;
	Boolean enabled;
	char currentPortName[kMaxProviderNameSize]; /* Flawfinder: ignore */
	OTPortRecord portRecord;
	
	OTAssert("OTTCPWillDial: paramErr", willDial != nil);
	
	*willDial = kOTTCPDialUnknown;
	
	err = noErr;
	if (OTInetGetInterfaceInfo == NULL || LoadNetworkSetupSymbols() != noErr) {	// GR 7/24/01
	
		// OT libraries aren't available; calling them would crash
		err = kOTNotFoundErr;
	}
	else if ( kUseInetInterfaceInfo && OTInetGetInterfaceInfo(&info, kDefaultInetInterface) == noErr) {
	
		// The TCP/IP stack is already loaded.  With the current
		// way TCP/IP is organised, the stack being loaded implies
		// that we're already dialled in.
		
		*willDial = kOTTCPDialNo;
		
	} else {
		err = GetTCPInfo(&enabled, currentPortName);
		if (err == noErr) {
			if (enabled) {
				if ( OTStrEqual(currentPortName, "ddp") ) { 

					// A special case for MacIP, because "ddp" does
					// not have an active port if AppleTalk is disabled.
					
					*willDial = kOTTCPDialNo;
					
				} 
				else if ( OTStrEqual(currentPortName, "AOLLink0") ) { 
					// another special case for AOL
					// we can't reliably tell if we will dial so err on safe side
					*willDial = kOTTCPDialYes;
				
				}
				else if ( OTFindPort(&portRecord, currentPortName) ) {
				
					// We know the port.  Look at the device type
					// to decide whether we might dial.
				
					switch ( OTGetDeviceTypeFromPortRef(portRecord.fRef) ) {
						case kOTADEVDevice:
						case kOTIRTalkDevice:
						case kOTSMDSDevice:
							OTDebugBreak("OTTCPWillDial: TCP shouldn't be using this link type");
							*willDial = kOTTCPDialNo;
							break;
							
						case kOTISDNDevice:
						case kOTATMDevice:
						case kOTSerialDevice:
						case kOTModemDevice:
							OTDebugBreak("OTTCPWillDial: TCP shouldn't be using this link type");
							*willDial = kOTTCPDialYes;
							break;

						case kOTLocalTalkDevice:
						case kOTTokenRingDevice:
						case kOTEthernetDevice:
						case kOTFastEthernetDevice:
						case kOTFDDIDevice:
						case kOTIrDADevice:
						case kOTATMSNAPDevice:
						case kOTFibreChannelDevice:
						case kOTFireWireDevice:
							*willDial = kOTTCPDialNo;
							break;

						case kOTMDEVDevice:
						case kOTSLIPDevice:
						case kOTPPPDevice:
							*willDial = kOTTCPDialYes;
							break;

						default:
							OTAssert("OTTCPWillDial", *willDial == kOTTCPDialUnknown);
							break;
					}
				} else {
					err = -1;
				}
			} else {
				*willDial = kOTTCPDialTCPDisabled;
			}
		}
	}
	
	return err;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HXMacNetWillDial
//

// From SCNetwork.h in SystemConfiguration.framework
typedef enum {
	kSCNetworkFlagsTransientConnection	=  1<<0,
	kSCNetworkFlagsReachable			=  1<<1,
	kSCNetworkFlagsConnectionRequired	=  1<<2,
	kSCNetworkFlagsConnectionAutomatic	=  1<<3,
	kSCNetworkFlagsInterventionRequired	=  1<<4
} SCNetworkConnectionFlags;

Boolean SCNetworkCheckReachabilityByName( const char *nodename,
                                          SCNetworkConnectionFlags *flags );

typedef Boolean (*SCNetworkCheckReachabilityByNameProcPtr) ( const char *nodename,
                                                             SCNetworkConnectionFlags *flags );

OSStatus HXMacNetWillDial( char* remoteHost, ULONG32* willDial )
{
	OSStatus outResult = paramErr;
	
#ifdef _CARBON
	if( IsRunningNativeOnMacOSX() )
	{
		Boolean success = FALSE;
		
		if( !willDial )
		{
			return outResult;
		}
		
		*willDial = kOTTCPDialUnknown;
		
		char* nodename = "real.com";
		
		if( remoteHost )
		{
			nodename = remoteHost;
		}
		
		SCNetworkCheckReachabilityByNameProcPtr proc;
		SCNetworkConnectionFlags flags;
		
		CFBundleRef sysConfigBundle = CFBundleGetBundleWithIdentifier( CFSTR("com.apple.SystemConfiguration") );

		if( sysConfigBundle )
		{
			proc = (SCNetworkCheckReachabilityByNameProcPtr) CFBundleGetFunctionPointerForName( sysConfigBundle, 
																								CFSTR("SCNetworkCheckReachabilityByName") );
			if( proc )
			{
				success = (*proc) ( nodename, &flags );
			}
			
			CFRelease( sysConfigBundle );
		}
		
		if( success )
		{
			if( flags & kSCNetworkFlagsConnectionRequired )
			{
				*willDial = kOTTCPDialYes;
			}
			else
			{
				*willDial = kOTTCPDialNo;
			}
			
			outResult = noErr;
		}
	}
	else
#endif
	{
		outResult = OTTCPWillDial( willDial );
	}
	
	return outResult;
}


#ifdef __cplusplus
}
#endif

