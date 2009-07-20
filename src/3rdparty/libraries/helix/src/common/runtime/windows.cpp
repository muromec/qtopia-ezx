/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: windows.cpp,v 1.5 2005/03/14 19:36:37 bobclark Exp $
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

#include "hlxclib/windows.h"

#if defined(WIN32_PLATFORM_PSPC)
LONG __helix_RegOpenKey(HKEY hKey, const char* lpSubKey, PHKEY phkResult)
{
    return RegOpenKeyEx(hKey, OS_STRING(lpSubKey), 0, KEY_ALL_ACCESS,
			phkResult);
}

LONG __helix_RegQueryValue(HKEY hKey, const char* lpSubKey, 
			   char* lpValue, PLONG lpcbValue)
{
    DWORD dwType;
    DWORD cbValue = *lpcbValue;
    LONG ret = RegQueryValueEx(hKey, OS_STRING(lpSubKey),
			       0, 
			       &dwType,
			       (LPBYTE)(OS_TEXT_PTR)OS_STRING2(lpValue, cbValue),
			       &cbValue);
    *lpcbValue = (LONG)cbValue;

    return ret;
}

LONG __helix_RegCreateKey(HKEY hKey, const char* lpSubKey, PHKEY phkResult)
{
    DWORD dwDisposition;
    return RegCreateKeyEx(hKey, OS_STRING(lpSubKey), 0,
			  OS_STRING(""),
			  REG_OPTION_NON_VOLATILE,
			  KEY_ALL_ACCESS,
			  0,
			  phkResult,
			  &dwDisposition);
}

LONG __helix_RegEnumKey(HKEY hKey, DWORD dwIndex, char* lpName, DWORD cbName)
{
    DWORD dwNameSize = cbName / sizeof(TCHAR);
    FILETIME fileTime;
    return RegEnumKeyEx(hKey, dwIndex, OS_STRING2(lpName, cbName),
			&dwNameSize, 0, 0, 0, &fileTime);
}

LONG __helix_RegSetValue(HKEY hKey, const char* lpSubKey, DWORD dwType, 
			 const char* lpData, DWORD cbData)
{
    LONG ret;

    if (dwType == REG_SZ || dwType == REG_MULTI_SZ || dwType == REG_EXPAND_SZ)
    {
	OS_STRING_TYPE lpStr(lpData);
	ret = RegSetValueEx(hKey, OS_STRING(lpSubKey), 0, dwType,
			    (LPBYTE)(OS_TEXT_PTR)(lpStr), (cbData + 1) * sizeof(*(OS_TEXT_PTR)lpStr));
    }
    else
    {
	ret = RegSetValueEx(hKey, OS_STRING(lpSubKey), 0, dwType,
			    (LPBYTE)lpData, cbData);
    }
    return ret;
}

HXBOOL __helix_GetDiskFreeSpace(LPCTSTR lpRootPathName,
			      LPDWORD lpSectorsPerCluster,
			      LPDWORD lpBytesPerSector,
			      LPDWORD lpNumberOfFreeClusters,
			      LPDWORD lpTotalNumberOfClusters)
{
    ULARGE_INTEGER freeBytesAvailableToCaller;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;
    
    HXBOOL ret = GetDiskFreeSpaceEx(lpRootPathName,
				  &freeBytesAvailableToCaller,
				  &totalNumberOfBytes,
				  &totalNumberOfFreeBytes);
    if (ret)
    {
	*lpSectorsPerCluster = 1;
	*lpBytesPerSector = 1;
	*lpNumberOfFreeClusters = freeBytesAvailableToCaller.LowPart;
	*lpTotalNumberOfClusters = totalNumberOfBytes.LowPart;
    }

    return ret;
}

#endif /* defined(WIN32_PLATFORM_PSPC) */
