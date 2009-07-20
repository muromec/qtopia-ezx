/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: maclibrary.h,v 1.8 2005/03/14 19:36:41 bobclark Exp $
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

#include "hxtypes.h"

// the following is defined (for some reason) in <racodec:pub:codec.h>
// did this here, so as not to cause undo worry about modifying a heavy
// traffic area header
// rlovejoy 9/11/98

#ifndef _WIN_CODEC
const INT16 	kCodecIDStrResID = 9000;
const INT16 	kCodecIDStrItem = 1;
#endif

const INT16 	kCodecBandwidthItem = 2;
const OSType 	kDecoderCreator = 'PNdc';
const OSType 	kEncoderCreator = 'PNen';
const OSType	kVideoDecoderCreator = 'PNrv';

#ifdef __cplusplus
class CHXString;
#endif



// begin from old macintosh.h file...


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 'interface' is in MSVC++ but not in codewarrior
// the following would let it compile, but would not really be kosher
//#define interface struct
// this is what seanh said to do..
#define interface \
#error "Using the keyword interface is an error, it should be changed to _INTERFACE."

// this is temporary until the win folks yank OutputDebugString
// but for now, mac people can do 'dx off' in MacsBugs

#define OutputDebugString debugstr

inline void VERIFY(Boolean ok){}

inline void TRACE(char * msg){}
inline void TRACE0(char * msg){}

#ifndef _BUILDOLDSTUFF
typedef void* HINSTANCE;
typedef HINSTANCE HMODULE;
typedef int LRESULT;
typedef Handle LPHANDLE;
typedef short WPARAM;
typedef void* HWND;
typedef Handle HANDLE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef signed long LONG;

typedef void* FARPROC ;

#define wsprintf sprintf /* Flawfinder: ignore */
#define lstrcpy strcpy /* Flawfinder: ignore */
#define lstrlen strlen /* Flawfinder: ignore */
#define lstrcat strcat /* Flawfinder: ignore */
#define lstrcpyn strncpy /* Flawfinder: ignore */
#else
typedef unsigned long HINSTANCE;
typedef HINSTANCE HMODULE;
typedef int LRESULT;
typedef Handle LPHANDLE;
typedef short WPARAM;
typedef void* POSITION;
typedef void* HWND;
typedef Handle HANDLE;
typedef unsigned int UINT;
typedef unsigned short WORD;
typedef unsigned char BYTE;

typedef void* FARPROC ;

#define wsprintf sprintf /* Flawfinder: ignore */
#define lstrcpy strcpy /* Flawfinder: ignore */
#define lstrlen strlen
#define lstrcat strcat /* Flawfinder: ignore */
#define lstrcpyn strncpy /* Flawfinder: ignore */
#endif

/////////////////

struct	LPARAM	{ 
	long	DWORD1;
	long	DWORD2;
};
typedef struct LPARAM LPARAM;

#define afx_msg

#ifndef max
#define max(x, y)	((x) > (y) ? (x) : (y))
#endif

#ifndef min
#define min(x, y) 	((x) < (y) ? (x) : (y))
#endif

inline int GetSafeInt(int x) { return x; }
inline int MulDiv(int m1, int m2, int d1) { return (m1*m2)/d1; }

///////////////////////
// video related

typedef  void* HDRAWDIB;

typedef void** HPALETTE;


///////////////////////


#define	CALLBACK	


// end of old macintosh.h

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif



/* Functions to load & release Shared Libraries */

#ifdef __cplusplus                                                              
extern "C"                                                                      
{
#endif /* __cplusplus */


/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		FindLibrary()
//
//	Purpose:
//
//		Called to get the fragment/file name of a Mac Shared Library given
//		the library's creator and codec ID. The Codec ID is stored in STR#9000;
//		item #1 of the library.
//
//	Parameters:
//
//		HX_MOFTAG moftCodecTag
//		The four character codec identifier (RV10, dnet, ...)
//
//		OSType codecCreator
//		The Creator type of the codec; currently kDecoderCreator or kEncoderCreator
//		
//		char* pLibName
//		Allocate this before calling FindLibrary. Library file name is
//		returned in it. Library's fragment name should be same as file name!!
//	
//		FSSpec *fsLibSpec
//		FSSpec of library that was found.
//
//	Return:
//
//		HX_RESULT
//		Error code indicating the success or failure of the function.
//
//HX_RESULT FindLibrary(HX_MOFTAG moftCodecTag, OSType codecCreator, char* pLibName, FSSpec &fsLibSpec);

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		LoadLibrary()
//
//	Purpose:
//
//		Called to load a Mac Shared Library given the library's fragment name.
//
//	Parameters:
//
//		char* dllname
//		The fragment name of the library.
//
//	Return:
//
//		ULONG32
//		Returns the ConnectionID
//
HINSTANCE LoadLibrary(const char* dllname);

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		FreeLibrary()
//
//	Purpose:
//
//		Called to free a Mac Shared Library. If this is not called the library
//		will be freed when the application quits.
//
//	Parameters:
//
//		HINSTANCE lib
//		This is actually the ConnectionID.
//
//	Return:
//
//		none
//
void FreeLibrary(HINSTANCE lib);

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		GetProcAddress()
//
//	Purpose:
//
//		Called to get a function pointer in a Mac Shared Library.
//
//	Parameters:
//
//		HMODULE lib
//		This is the ConnectionID that is returned from LoadLibrary
//
//		char* function
//		The function name
//
//	Return:
//
//		void*
//		The address of the function.
//
void* GetProcAddress(HMODULE lib, char* function);

//Opens the Library's resource fork if it's creator == codecCreator
//Also resolves aliases
short OpenLibraryRes(FSSpec *aliasSpec, OSType codecCreator);

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		ResolveIndependentPath()
//
//	Purpose:
//
// 		ResolveIndependentPath checks the supplied full path to see
// 		if it's prefixed by a location indicator (like 'ÇAPPLÈ:'
// 		or 'ÇasupÈ:'.  If so, it changes the prefix to the appropriate
// 		path beginning.
//
// 		If the supplied path does not contain a prefix, or the folder type
// 		isn't recognized by FindFolder, the string is not changed.
//
//  Return:
//
// 		TRUE if the path string was changed, FALSE otherwise.

#ifdef __cplusplus
HXBOOL ResolveIndependentPath(CHXString& strPath);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */


