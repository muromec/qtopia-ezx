/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllmain.cpp,v 1.5 2006/05/12 22:29:51 ping Exp $
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

#include "hxwinver.h"	// for HXWinVer()

#include "hlxclib/windows.h"
#include "hxmullan.h"	// for Multipl-Language Support API
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "hxperf.h"
HX_ENABLE_CHECKPOINTS_FOR_MODULE( "hxmedplyeng", "hxmedplyeng.log" );

#ifdef _WIN16
struct RTSPError
{
    const char* pErrNo;
    const char* pErrMsg;
};

#include "resource.h"
HGLOBAL	hFilter1 = NULL;
HGLOBAL hFilter2 = NULL;
HGLOBAL	hFilter4 = NULL;
HGLOBAL hFilter8 = NULL;
HGLOBAL hFilter11 = NULL;
HGLOBAL hFilter16 = NULL;
HGLOBAL hFilter32 = NULL;
HGLOBAL hAlpha = NULL;
HGLOBAL hADelta = NULL;
HGLOBAL hRTSPErrors = NULL;
HGLOBAL hUpTable = NULL;
HGLOBAL hDownTable = NULL;
HGLOBAL hFilterSizeTable = NULL;
HGLOBAL hPerPlex = NULL;
HGLOBAL hMimeBase = NULL;
HGLOBAL hMessedUp = NULL;
float*	filter1 = NULL;
float*	filter2 = NULL;
float*	filter4 = NULL;
float*	filter8 = NULL;
float*	filter11 = NULL;
float*	filter16 = NULL;
float*	filter32 = NULL;
INT32*	alpha = NULL;
INT32*  adelta = NULL;
RTSPError* RTSPErrorTable = NULL;
long*	upTable = NULL;
long* 	downTable = NULL;
long*	filterSizeTable = NULL;
char*	zPerPlexChars = NULL;
char*   zMIMEBase64Chars = NULL;
char*	zMessedUpBase64Chars = NULL;
#endif /* _WIN16 */

#ifndef _WINCE
HINSTANCE g_hInstance = NULL;
#else
HMODULE g_hInstance = NULL;
#endif
/////////////////////////////////////////////////////////////////////////////
//
//  Function:
//
//	InitInstance()
//
//  Purpose:
//
//	Performs any per-DLL initialization. Called in 16bit or 32bit case.
//
#ifndef _WINCE
BOOL InitInstance(HINSTANCE hDLL)
#else
BOOL InitInstance(HMODULE hDLL)
#endif
{
    g_hInstance = hDLL;

#ifdef _WIN16
    HRSRC   hResource = NULL;

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTER1), "Binary");
    hFilter1 = LoadResource(g_hInstance, hResource);
    filter1 = (float*)LockResource(hFilter1);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTER2), "Binary");
    hFilter2 = LoadResource(g_hInstance, hResource);
    filter2 = (float*)LockResource(hFilter2);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTER4), "Binary");
    hFilter4 = LoadResource(g_hInstance, hResource);
    filter4 = (float*)LockResource(hFilter4);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTER8), "Binary");
    hFilter8 = LoadResource(g_hInstance, hResource);
    filter8 = (float*)LockResource(hFilter8);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTER11), "Binary");
    hFilter11 = LoadResource(g_hInstance, hResource);
    filter11 = (float*)LockResource(hFilter11);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTER16), "Binary");
    hFilter16 = LoadResource(g_hInstance, hResource);
    filter16 = (float*)LockResource(hFilter16);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTER32), "Binary");
    hFilter32 = LoadResource(g_hInstance, hResource);
    filter32 = (float*)LockResource(hFilter32);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYALPHA), "Binary");
    hAlpha = LoadResource(g_hInstance, hResource);
    alpha = (INT32*)LockResource(hAlpha);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYADELTA), "Binary");
    hADelta = LoadResource(g_hInstance, hResource);
    adelta = (INT32*)LockResource(hADelta);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYRTSPERRORS), "Binary");
    hRTSPErrors = LoadResource(g_hInstance, hResource);
    RTSPErrorTable = (RTSPError*)LockResource(hRTSPErrors);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYUPTABLE), "Binary");
    hUpTable = LoadResource(g_hInstance, hResource);
    upTable = (LONG*)LockResource(hUpTable);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYDOWNTABLE), "Binary");
    hDownTable = LoadResource(g_hInstance, hResource);
    downTable = (LONG*)LockResource(hDownTable);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYFILTERSIZETABLE), "Binary");
    hFilterSizeTable = LoadResource(g_hInstance, hResource);
    filterSizeTable = (LONG*)LockResource(hFilterSizeTable);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYPERPLEX), "Binary");
    hPerPlex = LoadResource(g_hInstance, hResource);
    zPerPlexChars = (char*)LockResource(hPerPlex);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYMIMEBASE), "Binary");
    hMimeBase = LoadResource(g_hInstance, hResource);
    zMIMEBase64Chars = (char*)LockResource(hMimeBase);

    hResource = FindResource(g_hInstance, MAKEINTRESOURCE(MYMESSEDUP), "Binary");
    hMessedUp = LoadResource(g_hInstance, hResource);
    zMessedUpBase64Chars = (char*)LockResource(hMessedUp);
#endif /* _WIN16 */

    // Call Multi-Language support functions to allow 
    // them to be init'd as well!
#ifndef _WINCE
    HXSetupMulLang(g_hInstance, (HXGetWinVer(NULL) == HX_PLATFORM_WIN16));
#endif

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Function:
//
//	ExitInstance()
//
//  Purpose:
//
//	Performs any per-DLL cleanup. Called in 16bit or 32bit case.
//
int ExitInstance() 
{
#ifdef _WIN16
    UnlockResource(hFilter1);
    FreeResource(hFilter1);

    UnlockResource(hFilter2);
    FreeResource(hFilter2);

    UnlockResource(hFilter4);
    FreeResource(hFilter4);

    UnlockResource(hFilter8);
    FreeResource(hFilter8);

    UnlockResource(hFilter11);
    FreeResource(hFilter11);

    UnlockResource(hFilter16);
    FreeResource(hFilter16);

    UnlockResource(hFilter32);
    FreeResource(hFilter32);

    UnlockResource(hAlpha);
    FreeResource(hAlpha);

    UnlockResource(hADelta);
    FreeResource(hADelta);

    UnlockResource(hRTSPErrors);
    FreeResource(hRTSPErrors);

    UnlockResource(hUpTable);
    FreeResource(hUpTable);

    UnlockResource(hDownTable);
    FreeResource(hDownTable);

    UnlockResource(hFilterSizeTable);
    FreeResource(hFilterSizeTable);

    UnlockResource(hPerPlex);
    FreeResource(hPerPlex);

    UnlockResource(hMimeBase);
    FreeResource(hMimeBase);

    UnlockResource(hMessedUp);
    FreeResource(hMessedUp);
#endif /* _WIN16 */

    // Cleanup Multi-Language Support
#ifndef _WINCE
    HXCleanupMulLang();
#endif

    return 0;
}

#ifdef _WIN32
#if !defined(_STATICALLY_LINKED)
/////////////////////////////////////////////////////////////////////////////
//
//  Function:
//
//	DllMain()
//
//  Purpose:
//
//	Standard 32bit DLL entry point. Handles initialization and cleanup
//	of DLL instances. Only called in 32bit case.
//
#ifndef _WINCE
extern "C" BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
#else
BOOL WINAPI DllMain (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
#endif
{
    switch (dwReason)
    {
	case DLL_PROCESS_ATTACH:
	{
	    //
	    // DLL is attaching to the address space of the current process.
	    //
#ifndef _WINCE
	    InitInstance(hDLL);
#else
	    InitInstance((HMODULE)hDLL);
#endif
	}
	break;

	case DLL_THREAD_ATTACH:
	{
	    //
	    // A new thread is being created in the current process.
	    //
	}
	break;

	case DLL_THREAD_DETACH:
	{	
	    //
	    // A thread is exiting cleanly.
	    //
	}
	break;

	case DLL_PROCESS_DETACH:
	{
	    //
	    // The calling process is detaching the DLL from its address space.
	    //
	    ExitInstance();
	}
	break;
    }

    return TRUE;
}
#endif /* #if !defined(_STATICALLY_LINKED) */
#else
			       
/////////////////////////////////////////////////////////////////////////////
//
//  Function:
//
//  	LibMain()
//
//  Purpose:
//
//	Standard 16bit DLL entry point. Handles initialization of DLL 
//	instances. Only called in 16bit case.
//
extern "C" HANDLE WINAPI LibMain(HANDLE hInstance, WORD wDataSeg, WORD cbHeapSize, LPSTR lpCmdLine)
{
    if (InitInstance((HINSTANCE)hInstance))
    {
	if (0!=cbHeapSize)
	{
	    UnlockData(0);
	}
    }
    return hInstance;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Function:
//
//  	_WEP()
//
//  Purpose:
//
//	Standard 16bit DLL entry point. Handles clean up of DLL 
//	instances. Only called in 16bit case.
//
extern "C" int WINAPI _WEP (int bSystemExit);
#pragma alloc_text(FIXEDSEG, _WEP)
extern "C" int WINAPI _WEP (int bSystemExit)
{
    ExitInstance();
    return(1);
}

#endif

