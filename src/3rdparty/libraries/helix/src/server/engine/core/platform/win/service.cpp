/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: service.cpp,v 1.3 2004/05/13 18:57:48 tmarshall Exp $ 
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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "hlxclib/windows.h"

#include "service.h"
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif									 


BOOL 
InstallService(const char* pFullExePath,
	       const char* pServiceName,
	       const char* pServiceParams,
	       const char* pLogonName,
	       const char* pLogonPassword)
{
    BOOL        fReturnCode     = FALSE;
    LONG        lRet            = 0;
    HKEY        hkEvent         = NULL;
    HKEY        hkService       = NULL;
    HKEY        hkObject	= NULL;
    DWORD       dwDisposition	= 0;
    DWORD       dwData		= 0;
    SC_HANDLE	schSCManager	= NULL;
    SC_HANDLE   schService	= NULL;
    CHAR        pExePath[MAX_DISPLAY_NAME] = {0};
    CHAR	pServiceLogon[MAX_DISPLAY_NAME] = {0};
    CHAR	pServiceKey[MAX_DISPLAY_NAME] = {0};

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sdPermissions;

    // If an exe path was passed in, use it
    if (pFullExePath)
    {
	strcpy(pExePath, pFullExePath);
    }
    // Otherwise use the path for the current module
    else
    {
	GetModuleFileName(NULL, (LPTSTR)pExePath, MAX_DISPLAY_NAME); 
    }

    sprintf(pServiceLogon, ".\\%s", (pLogonName ? pLogonName : ""));

    // Connect to the local SCM
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL)
    {
	fReturnCode = FALSE;
        goto cleanup;
    }

    // Create the service
    if (!pLogonName || !*pLogonName || !pLogonPassword)
    {
	schService = CreateService(
	    schSCManager,
            pServiceName,
            pServiceName,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            pExePath,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);
    }
    else
    {
	schService = CreateService(
	    schSCManager,
            pServiceName,
            pServiceName,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            pExePath,
            NULL,
            NULL,
            NULL,
            (LPCTSTR)(pServiceLogon),
            (LPCTSTR)(pLogonPassword));
    }

    if (schService == NULL)
    {
	LPVOID lpMsgBuf;

	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	    NULL,
	    GetLastError(),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);

	// Display the string
	fprintf(stderr, "%s\n", lpMsgBuf);

	// Free the buffer.
	LocalFree( lpMsgBuf );

	fReturnCode = FALSE;
        goto cleanup;
    }

    // Generate security attribute/descriptor for the specified user
    if (pLogonName && *pLogonName)
    {
	// SD
        if (!CreateSecurityDescriptor(pLogonName, &sdPermissions))
        {
	    fReturnCode = FALSE;
            goto cleanup;
        }
                
        // SA
        sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle       = FALSE;
        sa.lpSecurityDescriptor = &sdPermissions;       
    }

    // Create the event log entry
    lRet = RegOpenKeyEx(
	HKEY_LOCAL_MACHINE,
        REGISTRY_KEY_EVENTLOG,
        0,
        KEY_ALL_ACCESS,
        &hkEvent);

    if (lRet != ERROR_SUCCESS)
    {
	fReturnCode = FALSE;
        goto cleanup;
    }

    // Create event key
    lRet = CREATEKEY(hkEvent, pServiceName, hkObject, dwDisposition);

    if (lRet != ERROR_SUCCESS)
    {
        fReturnCode = FALSE;
        goto cleanup;
    }

    // Set the value
    lRet = SETSZVALUE(hkObject, "EventMessageFile", pExePath);

    if (lRet != ERROR_SUCCESS)
    {
        fReturnCode = FALSE;
        goto cleanup;
    }

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 

    lRet = SETDWVALUE(hkObject, "TypesSupported", &dwData);

    if (lRet != ERROR_SUCCESS)
    {
	fReturnCode = FALSE;
	goto cleanup;
    }

    // Open the service registry key
    sprintf(pServiceKey, "%s\\%s", REGISTRY_KEY_SERVICE, pServiceName);
    lRet = RegOpenKeyEx(
	HKEY_LOCAL_MACHINE,
        pServiceKey,
        0,
        KEY_ALL_ACCESS,
        &hkService);

    if (lRet != ERROR_SUCCESS)
    {
        fReturnCode = FALSE;
        goto cleanup;
    }

    if (pLogonName && *pLogonName)
    {
        // Set the security
        lRet = RegSetKeySecurity(
	    hkService, 
            (SECURITY_INFORMATION)(DACL_SECURITY_INFORMATION),
            &sdPermissions);

	if (lRet != ERROR_SUCCESS)
        {
	    fReturnCode = FALSE;
	    goto cleanup;
        }
    }

    // Create StartupParams value
    if (pServiceParams)
    {
	lRet = SETSZVALUE(hkService, "StartupParams", pServiceParams);
	if (lRet != ERROR_SUCCESS)
	{
	    fReturnCode = FALSE;
	    goto cleanup;
	}
    }

    fReturnCode = TRUE;

cleanup:

    FREEHSCM(schService);
    FREEHSCM(schSCManager);

    FREEHKEY(hkEvent);
    FREEHKEY(hkService);
    FREEHKEY(hkObject);

    RegFlushKey(HKEY_LOCAL_MACHINE);

    return fReturnCode;
}

BOOL 
StartService(const char* pServiceName)
{
    BOOL	    bStarted	 = FALSE;
    DWORD	    dwBufferSize = MAX_PATH - 1;
    SC_HANDLE	    schSCManager = NULL;
    SC_HANDLE	    schService   = NULL;
    SERVICE_STATUS  ssStatus; 
    DWORD	    dwOldCheckPoint;

    // Get a handle to the service manager
    schSCManager = OpenSCManager(NULL, NULL, GENERIC_EXECUTE);
    if (schSCManager)
    {
	// Get a handle to the service
	schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS); 
 	if (schService)
	{
	    // Start the service
	    if (StartService(schService, 0, NULL))
	    {
		bStarted = TRUE;
	    }
// For now, we won't hang around to find out if it started
#ifdef XXXDPS
	    if (! QueryServiceStatus(schService, &ssStatus) )
	    {
		return FALSE;	
	    }
			
	    // Wait until the service starts (possibly animate this...)
	    while (ssStatus.dwCurrentState != SERVICE_RUNNING && 
		   ssStatus.dwCurrentState != SERVICE_STOPPED) 
	    {
		// Save the current checkpoint 
		dwOldCheckPoint = ssStatus.dwCheckPoint; 

		// Wait for the specified interval 
		Sleep(ssStatus.dwWaitHint); 

		// Check the status again 
		if (! QueryServiceStatus(schService, &ssStatus) )
		{
		    ServiceErrorMsg();
		    break;
		}
		// Break if the checkpoint has not been incremented 
		// (Currently the RMServer does not fill this in)
 		//if (dwOldCheckPoint >= ssStatus.dwCheckPoint)
		//{
		//	break;
		//}
	    }
#endif
 	}
    }
 
    // Close handles
    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);

    //return (ssStatus.dwCurrentState == SERVICE_RUNNING);
    return bStarted;
}

BOOL 
StopService(const char* pServiceName)
{
    BOOL	    bStopped	 = FALSE;
    DWORD	    dwBufferSize = MAX_PATH - 1;
    SC_HANDLE	    schSCManager = NULL;
    SC_HANDLE	    schService   = NULL;
    SERVICE_STATUS  ssStatus; 
    DWORD	    dwOldCheckPoint;

    // Get a handle to the service manager
    schSCManager = OpenSCManager(NULL, NULL, GENERIC_EXECUTE);
    if (schSCManager)
    {
	// Get a handle to the service
	schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS); 
 	if (schService)
	{
	    // Start the service
	    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
	    {
		bStopped = TRUE;
	    }

	    if (! QueryServiceStatus(schService, &ssStatus) )
	    {
		return FALSE;	
	    }
			
	    // Wait until the service stops (possibly animate this...)
	    while (ssStatus.dwCurrentState != SERVICE_STOPPED) 
	    {
		// Save the current checkpoint 
		dwOldCheckPoint = ssStatus.dwCheckPoint; 

		// Wait for the specified interval 
		Sleep(ssStatus.dwWaitHint); 

		// Check the status again 
		if (! QueryServiceStatus(schService, &ssStatus) )
		{
		    //ServiceErrorMsg();
		    break;
		}
		// Break if the checkpoint has not been incremented 
		// (Currently the RMServer does not fill this in)
 		//if (dwOldCheckPoint >= ssStatus.dwCheckPoint)
		//{
		//	break;
		//}
	    }
 	}
    }
 
    // Close handles
    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);

    return bStopped;
}

BOOL 
RemoveService(const char* pServiceName)
{
    long	lRet		= 0;
    HKEY	hkParameters	= NULL;
    SC_HANDLE	schService	= NULL;
    SC_HANDLE   schSCManager	= NULL;
    BOOL	bStatus		= TRUE;

    SERVICE_STATUS  ssStatus; 
    
    // Open the service control manager
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if(schSCManager == NULL)
    {                           
	bStatus = FALSE;
	goto cleanup;
    }

    // Open the service entry
    schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS);

    if(schService == NULL) 
    {   
	if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
	{
	    bStatus = TRUE;
	    goto cleanup;
	}
	else 
	{
	    LPVOID lpMsgBuf;

	    FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	    );

	    // Display the string
	    fprintf(stderr, "%s\n", lpMsgBuf);

	    // Free the buffer.
	    LocalFree( lpMsgBuf );

	    bStatus = FALSE;
	    goto cleanup;
	}
    }

    // Determine the status of the service (started/stopped)
    if (!QueryServiceStatus(schService, &ssStatus))
    {
	bStatus = FALSE;
	goto cleanup;
    }

    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
    {
	// Service is still running!
	bStatus = FALSE;
	goto cleanup;
    }

    // Delete the service
    if (!DeleteService(schService))
    {
	bStatus = FALSE;
	goto cleanup;
    }

    // Clean up the registry entry associated with the service
    lRet = RegOpenKeyEx(
	HKEY_LOCAL_MACHINE,
	REGISTRY_KEY_EVENTLOG,
	0,
	KEY_ALL_ACCESS,
	&hkParameters);

    if (lRet != ERROR_SUCCESS)
    {
	bStatus = FALSE;
	goto cleanup;
    }

    // Delete the service specific keys
    lRet = RegDeleteKey(hkParameters, pServiceName);
    if (lRet != ERROR_SUCCESS)
    {
	bStatus = FALSE;
	goto cleanup;
    }

    bStatus = TRUE;

cleanup:

    FREEHSCM(schService);
    FREEHSCM(schSCManager);

    FREEHKEY(hkParameters);
    RegFlushKey(HKEY_LOCAL_MACHINE);
  
    return bStatus;
}

BOOL 
CreateSecurityDescriptor(const char* pUserName, IN OUT SECURITY_DESCRIPTOR * psd)
{
    BOOL  fReturnCode	     = FALSE;
    PSID  psidAdmins	     = NULL;
    PACL  paclKey	     = NULL;
    DWORD cbReferencedDomain = 16;
    DWORD cbSid		     = 128;
    LPSTR lpReferencedDomain = NULL;
    PSID  psidUser	     = NULL;
    SID_NAME_USE sidNameUse;

    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    // Here we're creating a System Identifier (SID) to represent
    // the Admin group.
    if (!AllocateAndInitializeSid(&SystemSidAuthority, 2, 
	SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 
	0, 0, 0, 0, 0, 0, &psidAdmins))
    {
	goto cleanup;
    }

    // Now we'll find the System Identifier which represents
    // the specified user
    if((psidUser = HeapAlloc(GetProcessHeap(), 0, cbSid)) == NULL)
    {
	goto cleanup;
    }
 
    if((lpReferencedDomain = (LPSTR) HeapAlloc(GetProcessHeap(), 0, 
	cbReferencedDomain)) == NULL)
    {
	goto cleanup;
    }

    if (!LookupAccountName(NULL,		// local system
			   pUserName,		// account name
			   psidUser,		// receive SID of the account
			   &cbSid,		// size of the SID
			   lpReferencedDomain,	// buffer to receive user's domain
			   &cbReferencedDomain,	// size of UserDomain buffer
			   &sidNameUse))	// type of the user account
    {
	fReturnCode = FALSE;
	goto cleanup;
    }

    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
    {
       goto cleanup;
    }

    // We want the admin group to own this key.
    if (!SetSecurityDescriptorOwner(psd, psidAdmins, 0))
    {
       goto cleanup;
    }

    // Finally we must allocate and construct the discretionary
    // access control list (DACL) for the key.

    // Note that _alloca allocates memory on the stack frame
    // which will automatically be deallocated when this routine
    // exits.
    //if (!(paclKey = (PACL) _alloca(ACL_BUFFER_SIZE)))
    //{
    //   goto cleanup;
    //}

    if (!(paclKey = (PACL) malloc(ACL_BUFFER_SIZE)))
    {
       goto cleanup;
    }

    if (!InitializeAcl(paclKey, ACL_BUFFER_SIZE, ACL_REVISION))
    {
	goto cleanup;
    }

    // Our DACL will contain two access control entries (ACEs). One which allows
    // members of the Admin group complete access to the key, and one which gives
    // read-only access to everyone.
    if (!AddAccessAllowedAce(paclKey, ACL_REVISION, KEY_ALL_ACCESS, psidAdmins))
    {
       goto cleanup;
    }

    if (!AddAccessAllowedAce(paclKey, ACL_REVISION, KEY_ALL_ACCESS, psidUser))
    {
	goto cleanup;
    }

    if (!IsValidAcl(paclKey))
    {
       goto cleanup;
    }

    // We must bind this DACL to the security descriptor...
    if (!SetSecurityDescriptorDacl(psd, TRUE, paclKey, FALSE))
    {
       goto cleanup;
    }

    if (!IsValidSecurityDescriptor(psd))
    {
       goto cleanup;
    }

    fReturnCode = TRUE;
	
cleanup:

    if (paclKey)
    {
	free(paclKey);
	paclKey = NULL;
    }

    return fReturnCode;
}
