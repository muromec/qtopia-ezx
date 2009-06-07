/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdidde.cpp,v 1.8 2007/04/15 04:11:14 ping Exp $
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

#pragma data_seg("_DATA")

#include "hxtypes.h"
#include "hlxclib/windows.h"
#include <ddeml.h>

#include "sdidde.h"
#include "hxwinver.h"
#include "hxassert.h"	// needed for HX_TRACE and HX_ASSERT stuff...

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// These SPI_ flags are defined MSDEV 6.0 only when WINVER >= 0x0500
// see windows.h for details on when WINVER is set to >= 0x0500
#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#endif /* SPI_GETFOREGROUNDLOCKTIMEOUT */

#ifndef SPI_SETFOREGROUNDLOCKTIMEOUT
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
#endif /* SPI_SETFOREGROUNDLOCKTIMEOUT */

DWORD		    g_dwIdInst = 0; // DDE Instance information
DWORD		    g_dwNumInstances = 0;
HSZ		    g_hszWWWService = NULL;

typedef struct 
{
    const char* szDDEName;
    const char* szFileName;
} DDE_Browsers;

DDE_Browsers g_BrowsersSupportingDDE[] = 
{
    { "IEXPLORE", "iexplore.exe" },
    { "NETSCAPE", "netscape.exe" },
    { "OPERA",    "opera.exe"	 },
    { "MOSAIC",	  "mosaic.exe"	 }  
};

HDDEDATA HXEXPORT DdeCallback
(
    UINT	uType,			// transaction type
    UINT	uFmt,			// clipboard data format
    HCONV	hconv,			// handle of the conversation
    HSZ		hsz1,			// handle of a string
    HSZ		hsz2,			// handle of a string
    HDDEDATA	hdata,			// handle of a global memory object
    DWORD	dwData1,		// transaction-specific data
    DWORD	dwData2 		// transaction-specific data
)
{
    return 0;
}

void DDEInit(IHXPreferences* pPreferences)
{
    return;
}

HXBOOL DDEStartup()
{
    HX_TRACE("DDEStartup()\r\n");

    HX_ASSERT(g_dwNumInstances == 0 || g_dwIdInst);

    if (g_dwIdInst > 0)
    {
	goto exit;
    }

    if (DMLERR_NO_ERROR != DdeInitialize(&g_dwIdInst, DdeCallback, CBF_SKIP_ALLNOTIFICATIONS, 0))
    {
	return FALSE;
    }

exit:

    g_dwNumInstances++;    
    return TRUE;
}

void DDEShutdown()
{
    HX_TRACE("DDEShutdown()\r\n");

    HX_ASSERT(g_dwNumInstances > 0 && g_dwIdInst);

    if (g_dwNumInstances > 0)
    {
	g_dwNumInstances--;
    }

    if (g_dwNumInstances == 0 && g_dwIdInst)
    {
	if (g_hszWWWService)
	{
	    DdeFreeStringHandle(g_dwIdInst, g_hszWWWService);
	    g_hszWWWService = NULL;
	}

	DdeUninitialize(g_dwIdInst);
	g_dwIdInst = NULL;
    }
}

HXBOOL BrowserOpenURL(const char* pszUrl, const char* pszTarget, const char* pszDefBrowser) 
{
    HX_TRACE("BrowserOpenURL()\r\n");

    HXBOOL	result = TRUE;
    DWORD	dwResult = 0;
    HDDEDATA	hOpenRetVal = NULL;
    HSZ		hszOpenTopic = NULL;
    HSZ		hszOpenItem = NULL;
    HCONV	hOpenConv = NULL;
    HDDEDATA	hActivateRetVal = NULL;
    HSZ		hszActivateTopic = NULL;
    HSZ		hszActivateItem = NULL;
    HCONV	hActivateConv = NULL;
    UINT16	i = 0;
    UINT16	nNumberOfBrowsers = 0;
    char*	pMessage = NULL;
    DWORD	dwWindowID = 0xFFFFFFFF;	    // -1 = last active window
    DWORD	dwLockTimeout = 0;
    HXBOOL	bForceBroswserToForeground = FALSE;
    HXVERSIONINFO versionInfo;

    if (!pszUrl)
    {
	result = FALSE;
	goto cleanup;
    }

    pMessage = new char[strlen(pszUrl)+48];
    if (!pMessage)
    {
	result = FALSE;
	goto cleanup;
    }

    // handle pszTarget parameter according to:
    // https://common.helixcommunity.org/nonav/2003/HCS_SDK_r5/htmfiles/HyperNavigate.htm
    if (pszTarget && (0 == strcmp(pszTarget, "_new") || 0 == strcmp(pszTarget, "_blank")))
    {
        dwWindowID = 0;
    }

    ZeroInit(&versionInfo);
    HXGetWinVer(&versionInfo);
    bForceBroswserToForeground = ((versionInfo.dwPlatformId == HX_PLATFORM_WIN98) || (versionInfo.dwPlatformId == HX_PLATFORM_WINNT && versionInfo.wMajorVersion > 4));
    
    DDEStartup();

    // establish browser
    if (NULL == g_hszWWWService)
    {
	hszOpenTopic = DdeCreateStringHandle(g_dwIdInst, "WWW_OpenURL", CP_WINANSI);
	if (hszOpenTopic)
	{
	    nNumberOfBrowsers = sizeof(g_BrowsersSupportingDDE)/sizeof(DDE_Browsers);

	    // Look for a browser that supports DDE???
	    while (i < nNumberOfBrowsers)
	    {
		g_hszWWWService = DdeCreateStringHandle(g_dwIdInst,
						        g_BrowsersSupportingDDE[i].szDDEName,
							CP_WINANSI);

		hOpenConv = DdeConnect(g_dwIdInst, g_hszWWWService, hszOpenTopic, NULL);
		if (hOpenConv)
		{
		    break;
		}

		if(g_hszWWWService)
		{
		    DdeFreeStringHandle(g_dwIdInst, g_hszWWWService);
		    g_hszWWWService = NULL;  
		}

		i++;
	    }
	}

	if (NULL == g_hszWWWService)
	{
	    result = FALSE;
	    goto cleanup;
	}
    }
    else
    {
	hszOpenTopic = DdeCreateStringHandle(g_dwIdInst,"WWW_OpenURL", CP_WINANSI);
	hOpenConv = DdeConnect(g_dwIdInst, g_hszWWWService, hszOpenTopic, NULL);
    }

    if (!hOpenConv)
    {
	HX_TRACE("Conversation failed to start...\r\n");
	
	DdeFreeStringHandle(g_dwIdInst, g_hszWWWService);
	g_hszWWWService = NULL;

	result = FALSE;
	goto cleanup;
    }

    wsprintf(pMessage,"\"%s\",,%lu,0,,,,", pszUrl, dwWindowID); 
    hszOpenItem = DdeCreateStringHandle(g_dwIdInst, pMessage, CP_WINANSI);

    HX_TRACE("Conversation started, sending URL command...\r\n");

    // Request
    hOpenRetVal = DdeClientTransaction(NULL, 0, hOpenConv, hszOpenItem, CF_TEXT, XTYP_REQUEST, 60000, NULL);
    if (DDE_FNOTPROCESSED != hOpenRetVal)
    {
	DdeGetData(hOpenRetVal, (LPBYTE)&dwResult, sizeof(dwResult), 0);
	if (!dwResult)
	{
	    result = FALSE;
	    goto cleanup;
	}
    }

    // force the browser to the foreground then do it here.  This does not actually put the browser
    // in the foreground instead it enables the browser's call to SetForegroundWindow to bring the window to the
    // front instead of flashing it on the taskbar
    if (bForceBroswserToForeground)
    {
	// These are new flags that work on Win98 and WinNT5 ONLY.  First get the current foreground lock timeout and save
	// it off and then set it to 0.
	::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT,0,&dwLockTimeout,0);
	::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,0,0,0);
    }

    // activate the browser
    wsprintf(pMessage, "%lu,0", dwWindowID); 
    hszActivateItem = DdeCreateStringHandle(g_dwIdInst, pMessage, CP_WINANSI);
    hszActivateTopic = DdeCreateStringHandle(g_dwIdInst,"WWW_Activate", CP_WINANSI);

    // Connect to server
    if (hszActivateTopic && hszActivateItem)
    {
	hActivateConv = DdeConnect(g_dwIdInst, g_hszWWWService, hszActivateTopic, NULL);
	if (hActivateConv)
	{
	    hActivateRetVal = DdeClientTransaction(NULL, 0, hActivateConv, hszActivateItem, CF_TEXT, XTYP_REQUEST, 10000, NULL);
	}
    }

    if (bForceBroswserToForeground)
    {
	// Restore the old foreground lock timeout
	::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,0,(PVOID)dwLockTimeout,0);
    }

cleanup:

    if (hOpenRetVal)
	DdeFreeDataHandle(hOpenRetVal);
    if (hActivateRetVal)
	DdeFreeDataHandle(hActivateRetVal);

    if (hszOpenTopic)
	DdeFreeStringHandle(g_dwIdInst, hszOpenTopic);    
    if (hszActivateTopic)
	DdeFreeStringHandle(g_dwIdInst, hszActivateTopic);    

    if (hszOpenItem)    
	DdeFreeStringHandle(g_dwIdInst, hszOpenItem);
    if (hszActivateItem)    
	DdeFreeStringHandle(g_dwIdInst, hszActivateItem);

    if (hOpenConv)
	DdeDisconnect(hOpenConv);
    if (hActivateConv)
	DdeDisconnect(hActivateConv);

    HX_VECTOR_DELETE(pMessage);
    
    DDEShutdown();

    return result;
}
