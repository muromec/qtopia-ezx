/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: codverify.cpp,v 1.6 2005/03/14 19:35:26 bobclark Exp $
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

#if defined( _WIN32 ) || defined( _WINDOWS )
#include <hxtypes.h>
#include "hlxclib/windows.h"
#include <windowsx.h>
#ifndef _WIN32	
#include <ver.h>
#endif
#endif

#include <stdlib.h>
#include <string.h>

#if defined( _WIN32 ) || defined( _WINDOWS )
#ifndef _WINCE
#include <dos.h>
#include "hxloader.h"
#include <io.h>
#endif
#endif

#include "hxassert.h"
#include "safestring.h"

#include "codverify.h"
#include "hlxosstr.h"

HXBOOL DoesCodecFileExist(const char * DecDllPath)
{
#if defined(_STATICALLY_LINKED)
    return TRUE;
#else
    HXBOOL Exist = FALSE;

#ifndef _WINCE
    INT32				findhandle;
    struct _finddata_t	fileinfo;

    if (-1 != (findhandle = _findfirst( (char*)DecDllPath, &fileinfo )))
	Exist = TRUE;

    _findclose( findhandle );
#else
    
	if (GetFileAttributes(OS_STRING(DecDllPath)) != 0xffffffff)
        Exist=TRUE;

#endif

    return Exist;
#endif /* #if defined(_STATICALLY_LINKED) */
}

HXBOOL VerifyCodecFile(const char * DecDllPath)
{
#if defined(_STATICALLY_LINKED)
    return TRUE;
#else
#if !defined(_WINCE)
//	
//	Can return:
//		Success-	TRUE
//		Fail-		FALSE 	The file is NOT a codec, blow it off but no error occurs.
//
    LPCSTR		lpstrVffInfo = NULL;	// Pointer to block to hold resource

    ULONG32		dwVerHnd;
    ULONG32		dwVerInfoSize;
    char*		pLibType = NULL;
    HXBOOL		bIsCodec = FALSE;

    dwVerInfoSize = GetFileVersionInfoSize( (char*) DecDllPath, &dwVerHnd );
    if(dwVerInfoSize)
    {
	lpstrVffInfo  = (LPCSTR)GlobalAllocPtr( GMEM_MOVEABLE, dwVerInfoSize );
	if (lpstrVffInfo)
	{
	    if(!GetFileVersionInfo( (char*) DecDllPath, dwVerHnd, dwVerInfoSize, (LPVOID)lpstrVffInfo ))
	        lpstrVffInfo = NULL;
	}
    }

    if (lpstrVffInfo)
    {
	char    szGetName[_MAX_PATH]; /* Flawfinder: ignore */
	HXBOOL    bRetCode;
	LPSTR   lpVersion;
	ULONG32	dwVersionLen;

	SafeStrCpy( szGetName, "\\VarFileInfo\\Translation", _MAX_PATH);
	bRetCode = RAVerQueryValue( lpstrVffInfo, (LPSTR)szGetName,
 		                    (void FAR* FAR*)&lpVersion, &dwVersionLen );

	char TransNumber[10];  /* Flawfinder: ignore */
	SafeSprintf(TransNumber, 10, "%8lx", *(INT32 *)lpVersion);
	char * pSpace = strchr(TransNumber, ' ');
	while(pSpace)
	{ 
	    *pSpace = '0';
	    pSpace = strchr(TransNumber, ' ');
	}

	SafeStrCpy(szGetName, "\\StringFileInfo\\", _MAX_PATH);
	SafeStrCat(szGetName, TransNumber + 4, _MAX_PATH);
	TransNumber[4] = 0;               
	SafeStrCat(szGetName, TransNumber, _MAX_PATH);         
	SafeStrCat(szGetName, "\\LibraryType", _MAX_PATH);

	//lstrcpy(szGetName, "\\StringFileInfo\\040904b0\\LibraryType");
	bRetCode = RAVerQueryValue( lpstrVffInfo, (LPSTR)szGetName,
		                    (void FAR* FAR*)&lpVersion, &dwVersionLen );

	if (bRetCode && dwVersionLen && lpVersion)
	{
	    pLibType = new char[dwVersionLen + 1];
	    memcpy(pLibType, lpVersion, HX_SAFESIZE_T(dwVersionLen)); /* Flawfinder: ignore */
	    pLibType[ dwVersionLen ] = 0;
	}

	{
	    //	The win32 case
	    GlobalFreePtr( lpstrVffInfo );
	}    
    }

    if(pLibType)
    {
	if (strcmp(pLibType, "RACodecHelper") == 0)
	    bIsCodec = TRUE;
	delete [] pLibType;
    }
    return bIsCodec;
#else /* #if !defined(_WINCE) */
    return TRUE;
#endif
#endif /* #if defined(_STATICALLY_LINKED) */
}

