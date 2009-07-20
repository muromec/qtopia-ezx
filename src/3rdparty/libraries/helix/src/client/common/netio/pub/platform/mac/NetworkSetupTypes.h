/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: NetworkSetupTypes.h,v 1.5 2007/07/06 21:58:01 jfinnecy Exp $
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

#ifndef __NETWORKSETUPTYPES__
#define __NETWORKSETUPTYPES__

#ifndef __MACTYPES__
#include <MacTypes.h>
#endif
#ifndef __FILES__
#include <Files.h>
#endif
#ifndef _MAC_MACHO
#ifndef __TYPES__
#include <Types.h>
#endif
#endif



#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

#ifndef _MAC_MACHO
#include <OpenTransport.h>
#include <OpenTptInternet.h>
#endif
typedef UInt16 							IRPortSetting;

enum {
	kPrefsTypeStruct			= FOUR_CHAR_CODE('stru'),
	kPrefsTypeElement			= FOUR_CHAR_CODE('elem'),
	kPrefsTypeVector			= FOUR_CHAR_CODE('vect')
};

/*	-------------------------------------------------------------------------
	Error codes
	------------------------------------------------------------------------- */

enum {
	kCfgDatabaseChangedErr		= -3290,						/* database has changed since last call - close and reopen DB*/
	kCfgAreaNotFoundErr			= -3291,						/* Area doesn't exist*/
	kCfgAreaAlreadyExistsErr	= -3292,						/* Area already exists*/
	kCfgAreaNotOpenErr			= -3293,						/* Area needs to open first*/
	kCfgConfigLockedErr			= -3294,						/* Access conflict - retry later*/
	kCfgEntityNotFoundErr		= -3295,						/* An entity with this name doesn't exist*/
	kCfgEntityAlreadyExistsErr	= -3296,						/* An entity with this name already exists*/
	kCfgPrefsTypeNotFoundErr	= -3297,						/* An record with this PrefsType doesn't exist*/
	kCfgDataTruncatedErr		= -3298,						/* Data truncated when read buffer too small*/
	kCfgFileCorruptedErr		= -3299							/* The database format appears to be corrupted.*/
};

typedef struct OpaqueCfgDatabaseRef* 	CfgDatabaseRef;
typedef UInt32 							CfgAreaID;
typedef void *							CfgEntityAccessID;

struct CfgPrefsHeader {
	UInt16 							fSize;						/* size includes this header*/
	UInt16 							fVersion;
	OSType 							fType;
};
typedef struct CfgPrefsHeader			CfgPrefsHeader;
/*	-------------------------------------------------------------------------
	CfgEntityRef
	
	Identifies an object within the database
	------------------------------------------------------------------------- */

struct CfgEntityRef {
	CfgAreaID 						fLoc;
	UInt32 							fReserved;
	Str255 							fID;
};
typedef struct CfgEntityRef				CfgEntityRef;
/*	-------------------------------------------------------------------------
	CfgEntityClass / CfgEntityType

	The database can distinguish between several classes of objects and 
	several types withing each class
	Use of different classes allow to store type of information in the same database

	Other entity classes and types can be defined by developers.
	they should be unique and registered with Developer Tech Support (DTS)
	------------------------------------------------------------------------- */

enum {
	kCfgAnyEntityClass			= FOUR_CHAR_CODE('****'),
	kCfgUnknownEntityClass		= FOUR_CHAR_CODE('????'),
	kCfgAnyEntityType			= FOUR_CHAR_CODE('****'),
	kCfgUnknownEntityType		= FOUR_CHAR_CODE('????'),
	kOTNetworkConnectionClass	= FOUR_CHAR_CODE('otnc'),
	kOTGlobalProtocolSettingsClass = FOUR_CHAR_CODE('otgl'),
	kOTAppleTalkNetworkConnection = FOUR_CHAR_CODE('atlk'),
	kOTTCPv4NetworkConnection	= FOUR_CHAR_CODE('tcp4'),
	kOTTCPv6NetworkConnection	= FOUR_CHAR_CODE('tcp6'),
	kOTRemoteNetworkConnection	= FOUR_CHAR_CODE('ara '),
	kOTDialNetworkConnection	= FOUR_CHAR_CODE('dial'),
	kOTModemNetworkConnection	= FOUR_CHAR_CODE('modm'),
	kOTInfraredNetworkConnection = FOUR_CHAR_CODE('infr'),
	kOTSetOfSettingsClass		= FOUR_CHAR_CODE('otsc'),
	kOTSetOfSettingsType		= FOUR_CHAR_CODE('otst'),
	kOTGenericNetworkConnection	= FOUR_CHAR_CODE('otan')
};

/*	-------------------------------------------------------------------------
	CfgEntityClass & CfgEntityType
	------------------------------------------------------------------------- */

typedef OSType 							CfgEntityClass;
typedef OSType 							CfgEntityType;

struct CfgResourceLocator {
	FSSpec 							fFile;
	UInt16 							fResID;
};
typedef struct CfgResourceLocator		CfgResourceLocator;

struct CfgEntityInfo {
	CfgEntityClass 					fClass;
	CfgEntityType 					fType;
	Str255 							fName;
	CfgResourceLocator 				fIcon;
};
typedef struct CfgEntityInfo			CfgEntityInfo;
/*	reserve a 'free' tag for free blocks*/

enum {
	kFREEPrefsType				= FOUR_CHAR_CODE('free')
};

/*******************************************************************************
** Preferences Structures
********************************************************************************/

enum {
	kSetsIndexActive			= 0,
	kSetsIndexEdit				= 1,
	kSetsIndexLimit				= 2								/*	last value, no comma*/
};


struct CfgSetsStruct {
	UInt32 							fFlags;
	UInt32 							fTimes[2];
};
typedef struct CfgSetsStruct			CfgSetsStruct;

struct CfgSetsElement {
	CfgEntityRef 					fEntityRef;
	CfgEntityInfo 					fEntityInfo;
};
typedef struct CfgSetsElement			CfgSetsElement;

struct CfgSetsVector {
	UInt32 							fCount;
	CfgSetsElement 					fElements[1];
};
typedef struct CfgSetsVector			CfgSetsVector;
/*	AppleTalk	*/

enum {
																/*	connection	*/
	kATResourceCount			= FOUR_CHAR_CODE('cnam'),
	kATPrefsAttr				= FOUR_CHAR_CODE('atpf'),
	kPrefsVersionAttr			= FOUR_CHAR_CODE('cvrs'),
	kLocksAttr					= FOUR_CHAR_CODE('lcks'),
	kPortAttr					= FOUR_CHAR_CODE('port'),
	kProtocolAttr				= FOUR_CHAR_CODE('prot'),
	kPasswordAttr				= FOUR_CHAR_CODE('pwrd'),
	kPortFamilyAttr				= FOUR_CHAR_CODE('ptfm'),		/*	transport options	*/
	kUserLevelAttr				= FOUR_CHAR_CODE('ulvl'),
	kWindowPositionAttr			= FOUR_CHAR_CODE('wpos')
};


enum {
	k11AARPPrefsIndex			= 0,
	k11DDPPrefsIndex			= 1,
	k11NBPPrefsIndex			= 2,
	k11ZIPPrefsIndex			= 3,
	k11ATPPrefsIndex			= 4,
	k11ADSPPrefsIndex			= 5,
	k11PAPPrefsIndex			= 6,
	k11ASPPrefsIndex			= 7,
	k11ATLastPrefsIndex			= 7
};


struct AppleTalk11Preferences {
	UInt16 							fVersion;
	UInt16 							fNumPrefs;
	OTPortRef 						fPort;
	OTLink 							fLink;
	void *							fPrefs[8];
};
typedef struct AppleTalk11Preferences	AppleTalk11Preferences;

struct AARP11Preferences {
	UInt16 							fVersion;
	UInt16 							fSize;
	UInt32 							fAgingCount;
	UInt32 							fAgingInterval;
	size_t 							fProtAddrLen;
	size_t 							fHWAddrLen;
	UInt32 							fMaxEntries;
	size_t 							fProbeInterval;
	size_t 							fProbeRetryCount;
	size_t 							fRequestInterval;
	size_t 							fRequestRetryCount;
};
typedef struct AARP11Preferences		AARP11Preferences;

struct DDP11Preferences {
	UInt16 							fVersion;
	UInt16 							fSize;
	UInt32 							fTSDUSize;
	UInt8 							fLoadType;
	UInt8 							fNode;
	UInt16 							fNetwork;
	UInt16 							fRTMPRequestLimit;
	UInt16 							fRTMPRequestInterval;
	UInt32 							fAddressGenLimit;
	UInt32 							fBRCAgingInterval;
	UInt32 							fRTMPAgingInterval;
	UInt32 							fMaxAddrTries;
	Boolean 						fDefaultChecksum;
	Boolean 						fIsFixedNode;
	UInt8 							fMyZone[33];
};
typedef struct DDP11Preferences			DDP11Preferences;

struct atpfPreferences {
	AppleTalk11Preferences 			fAT;
	AARP11Preferences 				fAARP;
	DDP11Preferences 				fDDP;
	char 							fFill[122]; /* Flawfinder: ignore */
};
typedef struct atpfPreferences			atpfPreferences;
/*	Infrared	*/

enum {
	kInfraredPrefsAttr			= FOUR_CHAR_CODE('atpf'),
	kInfraredGlobalPrefsType	= FOUR_CHAR_CODE('irgo'),
	kInfraredWindowPosAttr		= FOUR_CHAR_CODE('wpos')
};


struct IrPreferences {
	CfgPrefsHeader 					fHdr;
	OTPortRef 						fPort;						/*	OT port id*/
	IRPortSetting 					fPortSetting;				/*	Ir protocol,  irda or irtalk*/
	Boolean 						fNotifyOnDisconnect;		/*	notify user on irda disconnect?*/
	Boolean 						fDisplayIRControlStrip;		/*	show ir control strip?*/
	Point 							fWindowPosition;			/*	The position of the editor window*/
};
typedef struct IrPreferences			IrPreferences;

struct InfraredGlobalPreferences {
	CfgPrefsHeader 					fHdr;						/* standard prefererences header*/
	UInt32 							fOptions;					/* options bitmask*/
	UInt32 							fNotifyMask;				/* Notification options.*/
	UInt32 							fUnloadTimeout;				/* Unload timeout (in milliseconds)*/
};
typedef struct InfraredGlobalPreferences InfraredGlobalPreferences;
/*	TCP/IP v4	*/

enum {
																/*	connection	*/
	kALISResType				= FOUR_CHAR_CODE('alis'),
	kCVRSResType				= FOUR_CHAR_CODE('cvrs'),
	kDCIDResType				= FOUR_CHAR_CODE('dcid'),
	kDTYPResType				= FOUR_CHAR_CODE('dtyp'),
	kIDNSResType				= FOUR_CHAR_CODE('idns'),
	kIHSTResType				= FOUR_CHAR_CODE('ihst'),
	kIITFResType				= FOUR_CHAR_CODE('iitf'),
	kARAResType					= FOUR_CHAR_CODE('ipcp'),
	kIRTEResType				= FOUR_CHAR_CODE('irte'),
	kISDMResType				= FOUR_CHAR_CODE('isdm'),
	kSTNGResType				= FOUR_CHAR_CODE('stng'),
	kUNLDResType				= FOUR_CHAR_CODE('unld'),
	kPORTResType				= FOUR_CHAR_CODE('port'),
	kTCPResourceCount			= FOUR_CHAR_CODE('cnam'),		/*	No. of resources */
	kTCPSelectedConfigType		= FOUR_CHAR_CODE('ccfg'),		/*	id of current selected CCL configuration */
	kTCPVersionAttr				= FOUR_CHAR_CODE('cvrs'),		/*	Version */
	kTCPDevTypeAtrr				= FOUR_CHAR_CODE('dvty'),
	kTCPPrefsAttr				= FOUR_CHAR_CODE('iitf'),
	kTCPServersListAttr			= FOUR_CHAR_CODE('idns'),
	kTCPSearchListAttr			= FOUR_CHAR_CODE('ihst'),
	kTCPRoutersListAttr			= FOUR_CHAR_CODE('irte'),
	kTCPDomainsListAttr			= FOUR_CHAR_CODE('isdm'),
	kTCPPortAttr				= FOUR_CHAR_CODE('port'),		/*	Ports */
	kTCPProtocolAttr			= FOUR_CHAR_CODE('prot'),
	kTCPPasswordAttr			= FOUR_CHAR_CODE('pwrd'),		/*	Password */
	kTCPLocksAttr				= FOUR_CHAR_CODE('stng'),		/*	locks */
	kTCPUnloadTypeAtrr			= FOUR_CHAR_CODE('unld'),		/*	transport options	*/
	kUserModeResType			= FOUR_CHAR_CODE('ulvl'),
	kTCPUserLevelAttr			= FOUR_CHAR_CODE('ulvl'),		/*	UserLevel */
	kTCPWindowPositionAttr		= FOUR_CHAR_CODE('wpos')		/*	Window Position */
};

#pragma options align=packed

struct IDNS11Preferences {
	short 							fCount;
	InetHost 						fAddressesList;
};
typedef struct IDNS11Preferences		IDNS11Preferences;

struct HSTF11Preferences {
	char 							fPrimaryInterfaceIndex;		/*	always 1 in OT 1.0 / 1.1*/
																/*	this structure IS packed!*/
	Str255 							fLocalDomainName;
																/*	followed by */
	Str255 							admindomain;
};
typedef struct HSTF11Preferences		HSTF11Preferences;
/*	This is your worst case, a fixed size structure, tacked on after a variable length string.*/
//	Use the macro to help access the movable beast.

#define	kIITFPartP( h ) ( (IITF11PreferencesPart*) &( (**( (IITF11Preferences**) h )).fAppleTalkZone[ (**( (IITF11Preferences**) h )).fAppleTalkZone[0] + 1 ] ) )

struct IITF11PreferencesPart {
	char 							path[36]; /* Flawfinder: ignore */
	char 							module[32]; /* Flawfinder: ignore */
	unsigned long 					framing;
};
typedef struct IITF11PreferencesPart	IITF11PreferencesPart;

struct IITF11Preferences {
	short 							fCount;
	UInt8 							fConfigMethod;
																/*	this structure IS packed!*/
																/*	Followed by:*/
	InetHost 						fIPAddress;
	InetHost 						fSubnetMask;
	unsigned char 					fAppleTalkZone[1]; /* Flawfinder: ignore */
																/*	this structure IS packed!*/
	IITF11PreferencesPart 			part;
};
typedef struct IITF11Preferences		IITF11Preferences;

struct IRTE11Entry {
	InetHost 						fToHost;					/*	always 0;*/
	InetHost 						fViaHost;					/*	router address;*/
	short 							fLocal;						/*	always 0*/
	short 							fHost;						/*	always 0*/
};
typedef struct IRTE11Entry				IRTE11Entry;

struct IRTE11Preferences {
	short 							fCount;
	IRTE11Entry 					fList[1];
};
typedef struct IRTE11Preferences		IRTE11Preferences;

struct ISDM11Preferences {
	short 							fCount;
	Str255 							fDomainsList;
};
typedef struct ISDM11Preferences		ISDM11Preferences;
#pragma options align=reset
/*	Modem	*/

enum {
																/*	connection	*/
	kModemResourceCount			= FOUR_CHAR_CODE('cnam'),		/*	No. of resources*/
	kModemSelectedConfigType	= FOUR_CHAR_CODE('ccfg'),		/*	id of current selected CCL configuration*/
	kModemConfigTypeModem		= FOUR_CHAR_CODE('ccl '),		/*	Type for Modem configuration resource*/
	kModemConfigTypeLocks		= FOUR_CHAR_CODE('lkmd'),		/*	Types for lock resources*/
	kModemConfigAdminPswdResID	= FOUR_CHAR_CODE('mdpw'),		/*	Password*/
																/*	transport options	*/
	kModemAppPrefsType			= FOUR_CHAR_CODE('mapt')
};


struct RAConfigModem {
	UInt32 							version;
	Boolean 						useModemScript;
	char 							pad00;						/*	this structure is NOT packed!*/
	FSSpec 							modemScript;
	Boolean 						modemSpeakerOn;
	Boolean 						modemPulseDial;
	UInt32 							modemDialToneMode;
	SInt8 							lowerLayerName[36];
};
typedef struct RAConfigModem			RAConfigModem;

struct LockStates {
	UInt32 							version;
	UInt32 							port;
	UInt32 							script;
	UInt32 							speaker;
	UInt32 							dialing;
};
typedef struct LockStates				LockStates;

struct ModemAppPrefs {
	UInt32 							version;
	Point 							windowPos;
	SInt32 							userMode;
};
typedef struct ModemAppPrefs			ModemAppPrefs;
/*	Remote Access	*/

enum {
																/*	connection	*/
	kRAConfigNameType			= FOUR_CHAR_CODE('cnam'),
	kRAConfigTypeARAP			= FOUR_CHAR_CODE('arap'),
	kRAConfigTypeAddress		= FOUR_CHAR_CODE('cadr'),
	kRAConfigTypeChat			= FOUR_CHAR_CODE('ccha'),
	kRAConfigTypeDialing		= FOUR_CHAR_CODE('cdia'),
	kRAConfigTypeExtAddress		= FOUR_CHAR_CODE('cead'),
	kRAConfigTypeClientLocks	= FOUR_CHAR_CODE('clks'),
	kRAConfigTypeClientMisc		= FOUR_CHAR_CODE('cmsc'),
	kRAConfigTypeConnect		= FOUR_CHAR_CODE('conn'),
	kRAConfigTypeUser			= FOUR_CHAR_CODE('cusr'),
	kRAConfigTypeDialAssist		= FOUR_CHAR_CODE('dass'),
	kRAConfigTypeIPCP			= FOUR_CHAR_CODE('ipcp'),
	kRAConfigTypeLCP			= FOUR_CHAR_CODE('lcp '),		/* trailing space is important! */
	kRAConfigTypeLogOptions		= FOUR_CHAR_CODE('logo'),
	kRAConfigTypePassword		= FOUR_CHAR_CODE('pass'),
	kRAConfigTypePort			= FOUR_CHAR_CODE('port'),
	kRAConfigTypeServerLocks	= FOUR_CHAR_CODE('slks'),
	kRAConfigTypeServer			= FOUR_CHAR_CODE('srvr'),
	kRAConfigTypeUserMode		= FOUR_CHAR_CODE('usmd'),
	kRAConfigTypeX25			= FOUR_CHAR_CODE('x25 '),		/* trailing space is important! */
																/*	transport options	*/
	kRemoteAppPrefsType			= FOUR_CHAR_CODE('capt'),
	kRemoteSelectedConfigType	= FOUR_CHAR_CODE('ccfg')
};

/*******************************************************************************
*	RALogOptions
*
*	This structure is appended to RAConnect records in the 
*	RAConnect::additional list.
*
*	NOTE
*
*	All RAConnect::additional structures MUST have the same fields up to
*	the "additional" field.  See RAX25Info.
********************************************************************************/

struct RALogOptions {
	UInt32 							version;
	UInt32 							fType;						/*	kRAConnectAdditionalLogOptions*/
	void *							additional;
	UInt32 							logLevel;					/*	values defined above.*/
	UInt32 							reserved[4];				/*	for later use.*/
};
typedef struct RALogOptions				RALogOptions;
/*******************************************************************************
*	New structures for dialing mode, phone numbers, and configuration stats.
*	
*	
********************************************************************************/

enum {
	kRAMaxAddressSize			= (255 + 1)
};


struct RAAddress {
	struct RAAddress *				next;
	UInt8 							address[256];
};
typedef struct RAAddress				RAAddress;
/*******************************************************************************
*	RADialing
*
*	This structure is appended to RAConnect records in the 
*	RAConnect::additional list.
*
*	NOTE
*
*	All RAConnect::additional structures MUST have the same fields up to
*	the "additional" field.  See RAX25Info.
********************************************************************************/

struct RADialing {
	UInt32 							version;
	UInt32 							fType;						/*	kRAConnectAdditionalDialing*/
	void *							additional;
	UInt32 							dialMode;					/*	values defined above.*/
	SInt32 							redialTries;
	UInt32 							redialDelay;				/*	in seconds.*/
	RAAddress *						addresses;
};
typedef struct RADialing				RADialing;
/*******************************************************************************
*	RAScript
*
*	This is appended to RAConnect records in the "additional" list.
*	It is currently only used for passing in a modem script to override
*	the default script.  Connect scripts have their own field in RAConnect.
*
*	NOTE
*
*	All RAConnect::additional structures MUST have the same fields up to
*	the "additional" field.  See RAX25Info and RADialing.
********************************************************************************/

struct RAScript {
	UInt32 							version;
	UInt32 							fType;						/*	kRAConnectAdditionalScript*/
	void *							additional;
	UInt32 							scriptType;
	UInt32 							scriptLength;
	UInt8 *							scriptData;
};
typedef struct RAScript					RAScript;
/*******************************************************************************
*	Miscellaneous limits
*	The size limits for strings include a 1 byte for the string length or
*	a terminating NULL character.
********************************************************************************/

enum {
	kRAMaxPasswordLength		= 255,
	kRAMaxPasswordSize			= (255 + 1),
	kRAMaxUserNameLength		= 255,
	kRAMaxUserNameSize			= (255 + 1),
	kRAMaxAddressLength			= 255,							/*	kRAMaxAddressSize					= (255 + 1),*/
	kRAMaxServerNameLength		= 32,
	kRAMaxServerNameSize		= (32 + 1),
	kRAMaxMessageLength			= 255,
	kRAMaxMessageSize			= (255 + 1),
	kRAMaxX25ClosedUserGroupLength = 4,
	kRAInfiniteSeconds			= (long)0xFFFFFFFF,
	kRAMinReminderMinutes		= 1,
	kRAChatScriptFileCreator	= FOUR_CHAR_CODE('ttxt'),
	kRAChatScriptFileType		= FOUR_CHAR_CODE('TEXT'),
	kRAMaxChatScriptLength		= 0x8000
};

/*******************************************************************************
*	X.25 connection information, added to RAConnect's additional info list.
*
*	NOTE
*
*	All RAConnect::additional structures MUST have the same fields up to
*	the "additional" field.  See RAScript & RADialing.
********************************************************************************/

struct RAX25Info {
	UInt32 							version;
	UInt32 							fType;						/*	kRAConnectAdditionalX25*/
	void *							additional;					/*	Ptr to additional connect info*/
	FSSpec 							script;						/*	PAD's CCL script*/
	UInt8 							address[256];				/*	address of server*/
	UInt8 							userName[256];
																/*	network user ID*/
	UInt8 							closedUserGroup[5];
																/*	closed user group*/
	Boolean 						reverseCharge;				/*	request reverse charging*/
};
typedef struct RAX25Info				RAX25Info;
/*******************************************************************************
*	RADisconnect
*
*	Use this structure to terminate Remote Access connections.
********************************************************************************/

struct RADisconnect {
	UInt32 							whenSeconds;				/*	Number of seconds until disconnect*/
	UInt32 							showStatus;					/*	Show disconnect status window*/
};
typedef struct RADisconnect				RADisconnect;
/*******************************************************************************
*	RAIsRemote
*
*	Use this structure to find out if an AppleTalk address is on the 
*	remote side of the current ARA link. The "isRemote" field is set to
*	"true" if the address is remote. 
********************************************************************************/

struct RAIsRemote {
	UInt32 							net;						/*	AppleTalk network number*/
	UInt32 							node;						/*	AppleTalk node number*/
	UInt32 							isRemote;					/*	returned.*/
};
typedef struct RAIsRemote				RAIsRemote;
/*******************************************************************************
*	RAConnect
*
*	Use this structure to initiate Remote Access connections.
********************************************************************************/

struct RAConnect {
	UInt32 							version;
	UInt32 							fType;						/*	RAConnectType defined above.*/
	UInt32 							isGuest;					/*	(boolean) True for guest login*/
	UInt32 							canInteract;				/*	(boolean) True if dialogs can be displayed*/
	UInt32 							showStatus;					/*	(boolean) Display (dis)connection status dialogs?*/
	UInt32 							passwordSaved;				/*	(boolean) "Save Password" checked in doc.*/
	UInt32 							flashConnectedIcon;			/*	(boolean) Flash icon in menu bar*/
	UInt32 							issueConnectedReminders;	/*	(boolean) Use Notification Manager reminders*/
	SInt32 							reminderMinutes;			/*	How long between each reminder?*/
	UInt32 							connectManually;			/*	(boolean) True if we are connecting manually*/
	UInt32 							allowModemDataCompression;	/*	(boolean) currently, only for kSerialProtoPPP*/
	UInt32 							chatMode;					/*	Flags defined above*/
	UInt32 							serialProtocolMode;			/*	Flags defined above*/

	UInt8 *							password;
	UInt8 *							userName;
	UInt32 							addressLength;				/*	Length of phone number or other address*/
	UInt8 *							address;					/*	Phone number or address data*/
	Str63 							chatScriptName;				/*	Name of imported chat script (informational only)*/
	UInt32 							chatScriptLength;			/*	Length of Chat script*/
	UInt8 *							chatScript;					/*	Chat script data*/

	void *							additional;					/*	Ptr to additional connect info,*/
																/*	such as RAX25Info*/

	UInt32 							useSecurityModule;			/*	(boolean) use line-level security module ?*/
	OSType 							securitySignature;			/*	signature of security module file*/
	UInt32 							securityDataLength;			/*	0..kSecurityMaxConfigData*/
	UInt8 *							securityData;				/*	Ptr to data of size securityDataLength*/
};
typedef struct RAConnect				RAConnect;
/*******************************************************************************
*	RAConnectInfo
*
*	If requestCode = kRAGetConnectInfo, "connectInfo" returns a pointer to a
*	new RAConnect block that describes the current connection.
*
*	If requestCode = kRADisposeConnectInfo, the memory pointed to by
*	"connectInfo" is released for reuse.  "connectInfo" must point to a valid
*	RAConnect structure previously returned by kRAGetConnectInfo.
********************************************************************************/

struct RAConnectInfo {
	RAConnect *						connectInfo;				/*	Returned or disposed, depending on requestCode*/
};
typedef struct RAConnectInfo			RAConnectInfo;
/*******************************************************************************
*	RAStatus
*
*	Use this structure to get the status of Remote Access connections.
********************************************************************************/

enum {
	kRAStatusIdle				= 1,
	kRAStatusConnecting			= 2,
	kRAStatusConnected			= 3,
	kRAStatusDisconnecting		= 4
};



struct RAStatus {
	UInt32 							status;						/*	values defined above*/
	Boolean 						answerEnabled;
	char 							pad00;						/*	This structure is NOT packed*/
	UInt32 							secondsConnected;
	UInt32 							secondsRemaining;
	UInt8 							userName[256];				/*	Pascal format*/
	UInt8 							serverName[33];				/*	Pascal format*/
	char 							pad01;						/*	This structure is NOT packed*/
	UInt32 							messageIndex;
	UInt8 							message[256];				/*	Pascal format*/
	UInt32 							serialProtocolMode;			/*	Flags defined above.*/
	UInt8 							baudMessage[256];			/*	Pascal format*/
	Boolean 						isServer;
	char 							pad02;						/*	This structure is NOT packed*/
	UInt32 							bytesIn;
	UInt32 							bytesOut;
	UInt32 							linkSpeed;
	UInt32 							localIPAddress;
	UInt32 							remoteIPAddress;
};
typedef struct RAStatus					RAStatus;
/*******************************************************************************
*	RAUserMessage
*
*	Use this structure when converting result codes into user messages.
********************************************************************************/

struct RAUserMessage {
	UInt32 							version;
	SInt32 							messageID;
	UInt8 							userMessage[256];
	UInt8 							userDiagnostic[256];
};
typedef struct RAUserMessage			RAUserMessage;
/*******************************************************************************
*	RANotifier
*
*	Use this structure to install a procedure to receive asynchronous 
*	Remote Access notifications.
********************************************************************************/

typedef UInt32 							RAEventCode;
typedef CALLBACK_API_C( void , RANotifyProcPtr )(void *contextPtr, RAEventCode code, OSStatus result, void *cookie);

struct RANotifier {
	RANotifyProcPtr 				procPtr;
	void *							contextPtr;
};
typedef struct RANotifier				RANotifier;
/*******************************************************************************
*	RARequest
*
*	All Remote Access API calls must pass a pointer to an RARequest structure.
********************************************************************************/

struct RARequest {
	SInt8 							reserved1[16];				/*	Do not use. */
	OSErr 							result;						/*	<--*/
	SInt8 							reserved2[8];				/*	Do not use.*/
	SInt16 							requestCode;				/*	 -->*/
	SInt16 							portId;						/*	<-->*/
	union {
		RANotifier 						Notifier;
		RAConnect 						Connect;
		RADisconnect 					Disconnect;
		RAStatus 						Status;
		RAIsRemote 						IsRemote;
		RAConnectInfo 					ConnectInfo;
	} 								fType;
};
typedef struct RARequest				RARequest;

struct RAConfigCAPT {
	UInt32 							fWord1;
	Point 							fWindowPosition;
	UInt32 							fWord3;
	UInt32 							fUserLevel;
	UInt32 							fSetupVisible;
};
typedef struct RAConfigCAPT				RAConfigCAPT;

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NETWORKSETUPTYPES__ */

