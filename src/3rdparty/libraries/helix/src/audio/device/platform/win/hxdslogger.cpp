/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdslogger.cpp,v 1.9 2007/07/06 20:21:17 jfinnecy Exp $
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

#include "hlxosstr.h"

#include <stdio.h>
#include "hlxclib/windows.h"
#include <winreg.h>
#include <stdlib.h>
#include <tchar.h>

#include "auddev.ver"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_WIN32) && !defined(_WINCE)
#include <tchar.h>
#endif

#ifndef TEXT
#define TEXT(w)  OS_STRING(w)
#endif

#ifndef _WINDOWS
#define TCHAR  char
#endif

const char* kNotificationLogFileName = "c:\\DSLog.txt";
bool bEnableDSLoggingStateSet = false;
bool bEnableDSLogging = false;

HXBOOL RMEnableLogging();
void RMDSLog(const char* pFormatString, ...);


void RMDSLog(const char* pFormatString, ...)
{
    if(RMEnableLogging())
    {
	va_list argptr;
	va_start(argptr, pFormatString);

	FILE* pFile = ::fopen(kNotificationLogFileName, "a+"); /* Flawfinder: ignore */
	if (pFile)
	{
	    ::vfprintf(pFile, pFormatString, argptr); /* Flawfinder: ignore */
	    ::fclose(pFile);
	}

	va_end(argptr);
    }
}

HXBOOL RMEnableLogging()
{
    if(bEnableDSLoggingStateSet)
	return bEnableDSLogging;

    //By default, assume the home page is going to be where we start, then see if the registry says otherwise.
    HXBOOL bEnableLogging = FALSE;

    TCHAR szPrefKeyPath[256]; /* Flawfinder: ignore */
    HKEY hKeyPlayerPrefsDSLogging;
    const TCHAR *kEnableDSLogging = TEXT("EnableDSLogging");
    const TCHAR* kPlayerPrefKey = TEXT("Software\\RealNetworks\\RealPlayer");
    
    wsprintf(szPrefKeyPath,TEXT("%s\\%d.%d\\Preferences\\%s"), kPlayerPrefKey, TARVER_MAJOR_VERSION, TARVER_MINOR_VERSION, kEnableDSLogging); /* Flawfinder: ignore */

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szPrefKeyPath, 0, KEY_ALL_ACCESS ,&hKeyPlayerPrefsDSLogging))
    {
	TCHAR szDSLogging[10]; /* Flawfinder: ignore */
	ULONG cb = sizeof(szDSLogging);
	unsigned long type=REG_SZ;
	
	//get key value
	if (ERROR_SUCCESS == RegQueryValueEx( hKeyPlayerPrefsDSLogging, TEXT(""), NULL, &type , (unsigned char*)szDSLogging, &cb))
	    bEnableLogging = (1 == _ttoi(szDSLogging));

	RegCloseKey(hKeyPlayerPrefsDSLogging);

    }
    if(bEnableLogging)
    {
	FILE* pFile = ::fopen(kNotificationLogFileName, "w+");
	if (pFile)
	{
	    ::fclose(pFile);
	}
    }
    bEnableDSLogging = bEnableLogging;
    bEnableDSLoggingStateSet = true;
    return bEnableLogging;
}

