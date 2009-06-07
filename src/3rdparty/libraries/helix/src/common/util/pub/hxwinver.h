/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxwinver.h,v 1.9 2006/02/23 23:59:32 bobclark Exp $
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

typedef struct _HXVERSIONINFO
{
	UINT16	wMajorVersion;		// Operating System information
	UINT16	wMinorVersion;		// Operating System information
	UINT16	wReleaseVersion;	// Operating System information release value.
	ULONG32	dwPlatformId;		// Operating System information
    ULONG32	dwMachineType;		// Hardware information
    HXBOOL	bFPUAvailable;		// Hardware information
} HXVERSIONINFO;

//	Add new platform IDs here (as they occur) these are defined as a bitfield
//	In case we need to determine subplatforms later (win16 & WfW for instance
//	though that's not done in this incantation).
#define HX_PLATFORM_WIN16		0x00000001
#define HX_PLATFORM_WIN32S		0x00000002
#define HX_PLATFORM_WIN95		0x00000004
#define	HX_PLATFORM_WINNT		0x00000008
#define HX_PLATFORM_MACOT		0x00000010
#define HX_PLATFORM_MACTCP		0x00000020
#define HX_PLATFORM_LINUX		0x00000040
#define HX_PLATFORM_SOLARIS		0x00000080
#define HX_PLATFORM_IRIX		0x00000100
#define HX_PLATFORM_SUNOS		0x00000200
#define HX_PLATFORM_WIN98		0x00000400
#define HX_PLATFORM_QNXNTO		0x00000800
#define HX_PLATFORM_SYMBIAN		0x00001000
#define HX_PLATFORM_OPENWAVE		0x00002000
#define HX_PLATFORM_MACOSX              0x00004000
#define HX_PLATFORM_UNKNOWN		0xFFFFFFFF

// Old defines used by some windows specific code.
#define HX_WINVER_16BIT			HX_PLATFORM_WIN16
#define HX_WINVER_32BIT_S		HX_PLATFORM_WIN32S
#define	HX_WINVER_32BIT_NT		HX_PLATFORM_WINNT
#define HX_WINVER_32BIT_95		HX_PLATFORM_WIN95

//	Add new machine IDs here (as they occur) these too are defined as a bitfield
#define HX_MACHINE_486			0x00000001	// 486 or better
#define HX_MACHINE_586			0x00000002	// Pentium or better
#define HX_MACHINE_686			0x00000004	// PentiumPro or better
#define HX_MACHINE_PPC			0x00000008	// PowerPC
#define HX_MACHINE_68K			0x00000010
#define HX_MACHINE_ALPHA		0x00000020
#define HX_MACHINE_MIPS			0x00000040
#define HX_MACHINE_SPARC		0x00000080
#define HX_MACHINE_ARM                  0x00000100
#define HX_MACHINE_SYMEMULATOR          0x00000200
#define HX_MACHINE_OWEMULATOR           0x00000400
#define HX_MACHINE_TOOSLOW		0xFFFFFFFE	// Anything that sucks. 286, 386, etc.
#define HX_MACHINE_UNKNOWN		0xFFFFFFFF

/*
 ** ULONG32	HXGetWinVer( HXVERSIONINFO *pVersionInfo )
 *
 *  PARAMETERS:
 *		pVersionInfo : A pointer to the version info struct to receive the
 *						results.  (Can be NULL, in which case our only side
 *						effect is our return value).
 *
 *  DESCRIPTION:
 *		Gets information on the platform, version, and hardware we are running on.
 *
 *  RETURNS:
 *		A flag indicating the platform we are running on.
 */
ULONG32 HXGetWinVer( HXVERSIONINFO* pVersionInfo );

/*
 *	const char* HXGetVerEncodedName( HXVERSIONINFO* pVersionInfo,
 *					const char* pProductName, const char* pProductVer,
 *					const char* pLanguage, const char* pDistCode)
 *
 *  PARAMETERS:
 *		pVersionInfo :	A pointer to the version info struct to receive the
 *						results.
 *
 *		pProductName :	A pointer to the name of the product like play16 for
 *						the 16bit compiled player or plug32 for the 32bit
 *						compiled plugin.
 *
 *		pProductVer :	A pointer to the string form of the version of the
 *						product like 2.0b3.
 *
 *		pLanguage	:	A pointer to the string form of the language code
 *						of the product like EN.
 *
 *		pDistCode 	:	A pointer to the string form of the version of the
 *						distribution code like PN01 for the player.
 *
 *  DESCRIPTION:
 *		Returns a standard formatted encoded platform string.
 *
 *  RETURNS:
 *		pointer to temporary buffer containing the encoded string.
 */
const char* HXGetVerEncodedName
(
	HXVERSIONINFO* pVersionInfo,
	const char* pProductName,
	const char* pProductVer,
	const char* pLanguage,
	const char* pDistCode
);

// convert the OS id into a string. "Linux", "WinNT", etc...
const char* HXGetOSName(ULONG32 nPlatformID);

// convert the Machine id into a string. "586", etc...
const char* HXGetMachName(ULONG32 nMachineType);


///////////////////////////////////////////////////////////////////////
//
//	FUNCTION:
//
//		HXExtractDistributionCode()
//
//	Description:
//
//		Extracts the distribution code resource from the version
//		information of the module.
//
//
HXBOOL HXExtractDistributionCode(char* pDistBuffer, UINT32 ulDistBufferLen, void* hMod);

#define MAX_ENCODED_NAME 128
