/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllmain.cpp,v 1.2 2007/07/06 21:58:20 jfinnecy Exp $
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

#include "hxwinver.h"	// for HXWinVer()

#include "hlxclib/windows.h"
#include "hxmullan.h"	// for Multipl-Language Support API
#include "hxchkpt.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

HX_ENABLE_CHECKPOINTS_FOR_MODULE( "HXMediaPlatform", "HXMediaPlatform.log" );

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
    // Cleanup Multi-Language Support
#ifndef _WINCE
    HXCleanupMulLang();
#endif

    return 0;
}

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


