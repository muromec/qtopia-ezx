/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: windows.h,v 1.11 2005/03/14 19:36:38 bobclark Exp $
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

#ifndef HLXSYS_WINDOWS_H
#define HLXSYS_WINDOWS_H

#include "hxtypes.h"

#if defined(_WINDOWS) && !defined(_OPENWAVE)

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#include "hlxclib/sys/socket.h"
#include <windows.h>
#include <mmsystem.h>

#include "hlxosstr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

LONG __helix_RegOpenKey(HKEY hKey, const char* lpSubKey, PHKEY phkResult);
LONG __helix_RegQueryValue(HKEY hKey, const char* lpSubKey,
                           char* lpValue, PLONG lpcbValue);
LONG __helix_RegCreateKey(HKEY hKey, const char* lpSubKey, PHKEY phkResult);
LONG __helix_RegEnumKey(HKEY hKey, DWORD dwIndex, char* lpName, DWORD cbName);
LONG __helix_RegSetValue(HKEY hKey, const char* lpSubKey, DWORD dwType,
                         const char* lpData, DWORD cbData);

HXBOOL __helix_GetDiskFreeSpace(LPCTSTR lpRootPathName,
                              LPDWORD lpSectorsPerCluster,
                              LPDWORD lpBytesPerSector,
                              LPDWORD lpNumberOfFreeClusters,
                              LPDWORD lpTotalNumberOfClusters);

#ifdef __cplusplus
};
#endif /* __cplusplus */


#if defined(WIN32_PLATFORM_PSPC)

#define COLOR_BTNHILIGHT COLOR_BTNHIGHLIGHT

_inline
LONG RegOpenKey(HKEY hKey, const char* lpSubKey, PHKEY phkResult)
{
    return __helix_RegOpenKey(hKey, lpSubKey, phkResult);
}

_inline
LONG RegQueryValue(HKEY hKey, const char* lpSubKey, char* lpValue,
                   PLONG lpcbValue)
{
    return __helix_RegQueryValue(hKey, lpSubKey, lpValue, lpcbValue);
}

_inline
LONG RegCreateKey(HKEY hKey, const char* lpSubKey, PHKEY phkResult)
{
    return __helix_RegCreateKey(hKey, lpSubKey, phkResult);
}

_inline
LONG RegEnumKey(HKEY hKey, DWORD dwIndex, char* lpName, DWORD cbName)
{
    return __helix_RegEnumKey(hKey, dwIndex, lpName, cbName);
}

_inline
LONG RegSetValue(HKEY hKey, const char* lpSubKey, DWORD dwType,
                 const char* lpData, DWORD cbData)
{
    return __helix_RegSetValue(hKey, lpSubKey, dwType, lpData, cbData);
}

#if 0
// this no longer seems to be a problem on ce
/*
 * The WinCE header files to not properly declare GetDiskFreeSpaceEx
 * as a WINAPI call so we run into linking problems. We are
 * redefining the GetDiskFreeSpaceEx macro so that it calls Helix
 * versions of the functions. These functions will use the proper
 * declaration to access the GetDiskFreeSpaceEx symbol.
 */
#include "platform/wince/get_disk_free.h"

#ifdef GetDiskFreeSpaceEx
#undef GetDiskFreeSpaceEx
#endif

#ifdef UNICODE
#define GetDiskFreeSpaceEx __helix_GetDiskFreeSpaceExW
#else
#define GetDiskFreeSpaceEx __helix_GetDiskFreeSpaceExA
#endif /* UNICODE */

_inline
HXBOOL GetDiskFreeSpace(LPCTSTR lpRootPathName,
                      LPDWORD lpSectorsPerCluster,
                      LPDWORD lpBytesPerSector,
                      LPDWORD lpNumberOfFreeClusters,
                      LPDWORD lpTotalNumberOfClusters)
{
    return __helix_GetDiskFreeSpace(lpRootPathName,
                                    lpSectorsPerCluster,
                                    lpBytesPerSector,
                                    lpNumberOfFreeClusters,
                                    lpTotalNumberOfClusters);
}

#endif // 0

#endif /* defined(WIN32_PLATFORM_PSPC) */

#endif /* _WINDOWS */

#endif /* HLXSYS_WINDOWS_H */
