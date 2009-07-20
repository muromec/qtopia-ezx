/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxhypnv.cpp,v 1.5 2008/08/15 17:53:54 ping Exp $
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

#include "hlxclib/stdio.h"

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxprefs.h"
#include "hxhyper.h"
#include "hxtick.h"
#include "hxstrutl.h"
#include "ihxpckts.h"

#include "hxhypnv.h"
#include "hxescapeutil.h"
#include "hxurlwrp.h"
#include "hxurl.h"

#if defined (_WIN16)
#include <stdlib.h>
#include "hlxclib/windows.h"
#include <shellapi.h>
#endif

#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
#include <ddeml.h>
#include "platform/win/sdidde.h"

HXBOOL CALLBACK FindAOLWindowProc(HWND hwnd, LPARAM lParam);

#endif /*(defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)*/

#include "dbcs.h" // needed for DBCS-relative string-processing functions

#if defined(_UNIX) && !defined(_VXWORKS) && !defined(_MAC_UNIX)
#include "unix_hurl.h"
#endif

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
#include "platform/mac/hurl.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


// amount of time to wait before trying to launch a browser with the same URL as
// the previous launch. Used in GoToURL to prevent the case where double-clicking
// calls GoToURL() twice, with the 2nd call launching a browser to the same URL while
// the first is still launching
#define MILLISEC_BETWEEN_BROWSER_LAUNCH 2000

#define _MAX_AOL_HURL_URL_SIZE	124

/****************************************************************************
 *
 *  Interface:
 *
 *	HXHyperNavigate
 *
 *  Purpose:
 *
 *	TBD
 *
 *  IID_IHXHyperNavigate:
 *
 *	{00000900-61DF-11d0-9CEE-080017035B43}
 *
 */


HXHyperNavigate::HXHyperNavigate() :
      m_lRefCount(0)
    , m_pContext(NULL)
    , m_pPreferences(0)
    , m_bInitialized(FALSE)
    , m_pLastURL(NULL)
    , m_nLastLaunchTime(0)
    , m_bKeepTargetBehind(FALSE)
{
#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
    DDEStartup();
#endif /*(defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)*/
}

HXHyperNavigate::~HXHyperNavigate()
{
#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
    DDEShutdown();
#endif /*(defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)*/
#if defined(_UNIX) && !defined(_MAC_UNIX)
    ShutdownHurlListener();
#endif // _UNIX && !_MAC_UNIX
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pContext);
    HX_VECTOR_DELETE(m_pLastURL);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your
//		object.
//
STDMETHODIMP HXHyperNavigate::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXHyperNavigate), (IHXHyperNavigate*)this },
            { GET_IIDHANDLE(IID_IHXHyperNavigate2), (IHXHyperNavigate2*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXHyperNavigate*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXHyperNavigate::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXHyperNavigate::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    
    delete this;
    return 0;
}



STDMETHODIMP HXHyperNavigate::Init(IUnknown* pContext)
{
    if (!pContext)
    {
	return HXR_UNEXPECTED;
    }

    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    m_pContext->AddRef();

    IHXPreferences* pPreferences = 0;
    if (pContext->QueryInterface(IID_IHXPreferences, (void**) &pPreferences) != HXR_OK)
    {
	return HXR_UNEXPECTED;
    }

    if (m_pPreferences)
    {
	m_pPreferences->Release();
	m_pPreferences = 0;
    }

    m_pPreferences = pPreferences;

#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
    DDEInit(m_pPreferences);
#endif /*(defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)*/


#if defined(_UNIX) && !defined(_VXWORKS) && !defined(_MAC_UNIX)
    StartHurlListener();
#endif /* _UNIX */

    m_bInitialized = TRUE;
    return HXR_OK;
}


/*
 *	IHXHyperNavigate methods
 */


/************************************************************************
 *	Method:
 *	    IHXHyperNavigate::GoToURL
 *	Purpose:
 *	    Performs a simple Go To URL operation.
 */
STDMETHODIMP HXHyperNavigate::GoToURL( const char* pURL,
					const char* pTarget)
{    
    if (!m_bInitialized)
    {
	return HXR_NOT_INITIALIZED;
    }

    if ((pURL == NULL) ||
	(pTarget && (stricmp(pTarget, "_player") == 0)) ||
	(strnicmp(pURL, URL_COMMAND, sizeof(URL_COMMAND) - 1) == 0))
    {
	return HXR_NOTIMPL;
    }

    UINT32 nCurrentTime = HX_GET_TICKCOUNT();

    // if user recently launched browser with same URL, then don't launch browser again..
    // Prevents case of double-clicking causing 2 browser launches, when second
    // click happens before first browser is up.
    if  ( m_pLastURL!=NULL && strcmp(pURL,m_pLastURL)==0 &&
    	  CALCULATE_ELAPSED_TICKS(m_nLastLaunchTime,nCurrentTime)<MILLISEC_BETWEEN_BROWSER_LAUNCH)
	return HXR_OK;

    CHXString encodedURL;
    if (strncasecmp(pURL, "http:", 5) == 0)
    {
        encodedURL = pURL;
        HXEscapeUtil::EnsureEscapedURL(encodedURL);

	pURL = encodedURL;
    }

    // note when we last attempted a browser launch (no puns please)
    m_nLastLaunchTime = nCurrentTime;

    HX_VECTOR_DELETE(m_pLastURL);

    // save the URL we are going to launch..
    m_pLastURL = new char[strlen(pURL)+1];
    strcpy(m_pLastURL,pURL); /* Flawfinder: ignore */

    CHXURL url(pURL, m_pContext);
    pURL = url.GetURL();   // will do compression of ../../, if so required

#ifndef _MACINTOSH
    // GR 7/13/01 -  We don't need to wrap on the Mac since the URL length
    // limition for hurling doesn't apply, and we don't want to wrap since 
    // local file URLs under Mac OS X do not begin file:///Hard Drive/...
    
    CHXString strHtmlFile;

    if (strlen(pURL) > _MAX_AOL_HURL_URL_SIZE)
    {
	if (SUCCEEDED(CHXUrlWrapper::Wrap(pURL, &strHtmlFile, m_pContext)))
	{
#ifdef _MACINTOSH
	    strHtmlFile = "file:///" + strHtmlFile;
#else
	    strHtmlFile = "file://" + strHtmlFile;
#endif	    
	    pURL = (const char*)strHtmlFile;
	}
    }
#endif // !_MACINTOSH
    
#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
    CHXString strPreferredBrowser;

    if(m_pPreferences)
    {
	IHXBuffer* pPreferredBrowser = NULL;

	if(m_pPreferences->ReadPref("PreferredBrowser", pPreferredBrowser) == HXR_OK)
	    strPreferredBrowser = (const char*)pPreferredBrowser->GetBuffer();

	HX_RELEASE(pPreferredBrowser);
    }

    if(!strPreferredBrowser.IsEmpty())
    {
	if (BrowserOpenURL(pURL, pTarget, strPreferredBrowser))
	{
	    return HXR_OK;
	}
	else if (LaunchBrowserWithURL(pURL, strPreferredBrowser))
	{
	    return HXR_OK;
	}
    }

    if (BrowserOpenURL(pURL, pTarget))
    {
	return HXR_OK;
    }
    else if (LaunchBrowserWithURL(pURL))
    {
        return HXR_OK;
    }
#endif /*(defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)*/
#if defined (_MACINTOSH) || defined(_MAC_UNIX)
    HXBOOL bBringToFront = TRUE;
    
    if (GetKeepTargetBehind())
    {
    	bBringToFront = FALSE;
    }
    
    if(LaunchMacBrowserWithURLOrdered(pURL, m_pPreferences, bBringToFront)) //in pnmisc/mac/hurl.cpp
    {
	return HXR_OK;
    }
#endif /*defined (_MACINTOSH)*/
    
#if defined(_UNIX) && !defined(_VXWORKS) && !defined(_MAC_UNIX)
    SendHurlRequest(pURL);
    return HXR_OK;
#endif
    return HXR_FAILED;
}

#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)

HXBOOL GetAOLBrowserCmdLine(char* lpLaunchCmd)
{
    if(GetProfileString("WAOL", "AppPath", "", lpLaunchCmd, _MAX_PATH) > 0)
    {
	if(lpLaunchCmd[strlen(lpLaunchCmd) - 1] != '\\')
	{
	    SafeStrCat(lpLaunchCmd, "\\", _MAX_PATH);
	}
	SafeStrCat(lpLaunchCmd, "waol.exe -u%1", _MAX_PATH);

	return TRUE;
    }

    return FALSE;
}

// This function is called by LaunchBrowserWithURL below.  I removed
// this chunk of code from that function, so that this function could
// also be called rpmisc (via rpclsvc) to get the browser to send 
// to techsupport via rphurlerror in error messages.
HXBOOL GetBrowserFromRegistry(char* lpLaunchCmd)  // lpLaunchCmd should point to a buffer of _MAX_PATH+1 size
{
    HXBOOL bBrowserFound = FALSE;
    HXBOOL bFoundAOLBrowser = FALSE;

    // Can't use FindWindow because it waits if a window is doing activity inside of its WndProc and if the window is waiting
    // on some event deadlock occurs.  Enumerate windows instead.
    ::EnumWindows(FindAOLWindowProc,(LPARAM)&bFoundAOLBrowser);
    if(bFoundAOLBrowser)
    {
	bBrowserFound = GetAOLBrowserCmdLine(lpLaunchCmd);
    }

    if(!bBrowserFound)
    {
	char*	lpSubKey	= new char[_MAX_PATH+1];
	HKEY	hkProtocol;
	LONG	lBuffLen;
	HXBOOL	bTypeNameFound = FALSE;

	SafeStrCpy(lpSubKey, ".HTM", _MAX_PATH+1);
	if (RegOpenKey(HKEY_CLASSES_ROOT, lpSubKey, &hkProtocol) == ERROR_SUCCESS)
	{
	    lBuffLen = _MAX_PATH;
	    if (RegQueryValue(hkProtocol, NULL, lpLaunchCmd, &lBuffLen) == ERROR_SUCCESS)
	    {
		bTypeNameFound = TRUE;
	    }
	    RegCloseKey(hkProtocol);
	}

	if (!bTypeNameFound)
	{
	    SafeStrCpy(lpSubKey, ".HTML", _MAX_PATH+1);
	    if (RegOpenKey(HKEY_CLASSES_ROOT, lpSubKey, &hkProtocol) == ERROR_SUCCESS)
	    {
		lBuffLen = _MAX_PATH;
		if (RegQueryValue(hkProtocol, NULL, lpLaunchCmd, &lBuffLen) == ERROR_SUCCESS)
		{
		    bTypeNameFound = TRUE;
		}
		RegCloseKey(hkProtocol);
	    }
	}

	// Ok, we figured out the "name" of HTML documents, this will help us find the
	// application that thinks it will launch them...
	if (bTypeNameFound)
	{
	    SafeStrCpy(lpSubKey,lpLaunchCmd, _MAX_PATH+1);
	    if (RegOpenKey(HKEY_CLASSES_ROOT, lpSubKey, &hkProtocol) == ERROR_SUCCESS)
	    {
		lBuffLen = _MAX_PATH;
		if (RegQueryValue(hkProtocol, "shell\\open\\command", lpLaunchCmd, &lBuffLen) == ERROR_SUCCESS)
		{
		    bBrowserFound = TRUE;
		}
		RegCloseKey(hkProtocol);
	    }
	}
    }

    if(!bBrowserFound)
    {
	bBrowserFound = GetAOLBrowserCmdLine(lpLaunchCmd);
    }

    return bBrowserFound;
}

HXBOOL CALLBACK FindAOLWindowProc(HWND hwnd, LPARAM lParam)
{
    char pWindowText[32] = ""; /* Flawfinder: ignore */
    DWORD dwResult = 0;
    ::SendMessageTimeout(hwnd, WM_GETTEXT, (WPARAM)sizeof(pWindowText), (LPARAM)pWindowText, SMTO_BLOCK, 500, &dwResult);
    
    if (!strcmp("America  Online", pWindowText))
    {
	HXBOOL* pBool = (HXBOOL*)lParam;
	*pBool = TRUE;
	return FALSE;
    }

    return TRUE;
}

#endif /*(defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)*/


HXBOOL HXHyperNavigate::LaunchBrowserWithURL(const char* pURL, const char* pDefBrowser)
{
#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
    HXBOOL	bHandled = FALSE;
    char*	lpLaunchCmd	= new char[_MAX_PATH+1];
    char*	lpActualLaunch	= new char[_MAX_PATH+1];
    HXBOOL	bBrowserFound = FALSE;

    if(pDefBrowser)
    {
	const char* pName = strrchr(pDefBrowser, '\\');
	if(pName)
	{
	    pName++;
	    if(!stricmp(pName, "waol.exe"))
	    {
		SafeSprintf(lpLaunchCmd,_MAX_PATH+1,"%s -u%%1", pDefBrowser);
	    }
	    else
	    {
		SafeSprintf(lpLaunchCmd,_MAX_PATH+1,"%s %%1", pDefBrowser);
	    }
	    bBrowserFound = TRUE;
	}
    }

    if(!bBrowserFound)
	bBrowserFound = GetBrowserFromRegistry(lpLaunchCmd);

    // Ok, we got a browser "launch" entry...
    if (bBrowserFound)
    {
	// Now we want to find out if it's a reasonable format...
	char* pParam = (char*)HXFindString(lpLaunchCmd, "%1");

	if (pParam)
	{
	    if ( (strlen(lpLaunchCmd)+strlen(pURL)) >= _MAX_PATH)
	    {
		bHandled = FALSE;
	    }
	    else
	    {
	    	// Replace %1 with %s to use format!
	    	pParam++;
	    	*pParam = 's';
    
	    	SafeSprintf(lpActualLaunch,_MAX_PATH+1,lpLaunchCmd,pURL);
    
	    	if(WinExec(lpActualLaunch,
			    #ifdef _WIN32
			    SW_SHOWDEFAULT
			    #else
			    SW_SHOWNORMAL
			    #endif
		    	) > 31)
	    	{
		    bHandled = TRUE;
	    	}
	    	// If we failed, we should try without the URL....
	    	else
	    	{
		    SafeSprintf(lpActualLaunch,_MAX_PATH+1,lpLaunchCmd,"");
    
		    if(WinExec(lpActualLaunch,
			    	#ifdef _WIN32
			    	SW_SHOWDEFAULT
			    	#else
			    	SW_SHOWNORMAL
			    	#endif
			    	) > 31)
		    {
		    	if (BrowserOpenURL(pURL, NULL, pDefBrowser))
		    	{
			    bHandled = TRUE;
		    	}
		    }
	    	} // end if Launch with URL failed.
	    } // launchCmd+Url too long
	}
	// If we didn't find a paramter, try to launch the browser
	// then do DDE to it...
	else
	{
	    if(WinExec(lpLaunchCmd,
			#ifdef _WIN32
			SW_SHOWDEFAULT
			#else
			SW_SHOWNORMAL
			#endif
		) > 31)
	    {
		if (BrowserOpenURL(pURL, NULL, pDefBrowser))
		{
		    bHandled = TRUE;
		}
	    }
	} // end if we found a parameter...
    }

    if (lpLaunchCmd)
    {
	delete [] lpLaunchCmd;
	lpLaunchCmd = 0;
    }

    if (lpActualLaunch)
    {
	delete [] lpActualLaunch;
	lpActualLaunch = 0;
    }

    return bHandled;
#else
    return FALSE;
#endif /*(defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)*/
}


/************************************************************************
 *	Method:
 *	    IHXHyperNavigate2::Execute
 *	Purpose:
 *	    
 *	Parameters:
 *      pURL:	    URL (absolute or relative)
 *	    pTargetInstance:	
 *	    pTargetApplication: 
 *	    pTargetRegion:
 *	    pParams:
 */
STDMETHODIMP 
HXHyperNavigate::Execute(const char* pURL,
			  const char* pTargetInstance,
			  const char* pTargetApplication,
			  const char* pTargetRegion,
			  IHXValues* pParams)
{
    return GoToURL(pURL, pTargetInstance);
}
