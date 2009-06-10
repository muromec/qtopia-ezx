/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: service.h,v 1.2 2003/01/23 23:42:56 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#define MAX_DISPLAY_NAME	256
#define ACL_BUFFER_SIZE		1024
#define SZSERVICENAME		"RMServer"
#define REGISTRY_KEY_EVENTLOG	"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"
#define REGISTRY_KEY_SERVICE	"SYSTEM\\CurrentControlSet\\Services"
#define REGISTRY_KEY_RMSERVICE 	"SYSTEM\\CurrentControlSet\\Services\\RMServer"

#define CREATEKEY(hkParent, pszName, hkOut, dwDisposition) \
    RegCreateKeyEx(hkParent, pszName, 0, "", REG_OPTION_NON_VOLATILE, \
	KEY_ALL_ACCESS, NULL, &hkOut, &dwDisposition)

#define SETKEYSECURITY(hk, psecurity_descriptor) \
    RegSetKeySecurity(hk, (SECURITY_INFORMATION)( DACL_SECURITY_INFORMATION), \
	psecurity_descriptor)

#define SETSZVALUE(hk, pszName, pszValue) \
    RegSetValueEx(hk, pszName, 0, REG_SZ, (BYTE*)pszValue, lstrlen(pszValue)+1)

#define SETDWVALUE(hk, pszName, pdwValue) \
    RegSetValueEx(hk, pszName, 0, REG_DWORD, (LPBYTE)pdwValue, sizeof(DWORD))

#define GETKEYVALUE(hk, pszName, pszValue, pdwSize) \
	RegQueryValueEx(hk, pszName, 0, NULL, (LPBYTE)pszValue, pdwSize)

#define FREEHKEY(hk) \
    if(hk != INVALID_HANDLE_VALUE) \
	RegCloseKey(hk);

#define FREEHSCM(h)		\
{                               \
    if ((h) != NULL)            \
	{                       \
        CloseServiceHandle(h);	\
		(h) = NULL;     \
	}                       \
}


BOOL InstallService (const char* pFullExePath,
		     const char* pServiceName, 
		     const char* pServiceParams,
		     const char* pLogonName, 
		     const char* pLogonPassword);
BOOL StartService   (const char* pServiceName);
BOOL StopService    (const char* pServiceName);
BOOL RemoveService  (const char* pServiceName);

BOOL CreateSecurityDescriptor (const char* pUserName, 
			       IN OUT SECURITY_DESCRIPTOR * psd);

#endif // _REGISTRY_H_
