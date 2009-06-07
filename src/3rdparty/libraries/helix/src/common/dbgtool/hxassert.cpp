/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxassert.cpp,v 1.21 2005/03/14 19:36:27 bobclark Exp $
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

/////////////////////////////////////////////////////////////////////////////
// HXASSERT.CPP
//
// Debugging support implementation.
//
// HX_ASSERT()	- asserts an expression is TRUE. Compiles to no-ops in
//				retail builds. Provides message box or other UI when 
//				expression fails.
//
// HX_ASSERT_VALID_PTR() - asserts that a pointer is valid. Performs more
//				rigid verification specifically appropriate for pointers.
//
// HX_VERIFY()	- verifies an expression is TRUE. Expression or code DOES NOT 
//				compile away in retail builds, but UI of failure is removed.
//				In debug builds provides message box or other UI when 
//				expression fails.
//
// HX_TRACE()	- Similar to DEBUGPRINTF() but no buffer is required. 
//				Compiles to no-ops in retail builds.
//

#include "hxtypes.h"
#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"

#if defined(_WIN32) || defined(_WINDOWS)
#include "hlxclib/windows.h"
#if !defined(_WIN32) && !defined(WIN32)
#include <shellapi.h>
#include "string.h"
#endif
#endif

#include "hxassert.h"
#include "hxstrutl.h"

#include "debugout.h"

#include "hxtypes.h"
#include "hxresult.h"

#if defined (DEBUG) || defined (_DEBUG)

#ifdef _MACINTOSH
#include <stdarg.h>
#include <string.h>
#include "platform/mac/hxcrt_assert.h"
#define IDIGNORE	2
#define IDRETRY		1
#endif

#ifdef _UNIX
#include <stdarg.h>
#define IDIGNORE	0
#define IDRETRY		1
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#if defined(_SYMBIAN)
#include "avkon.rsg"
#include <aknglobalmsgquery.h> 
#endif

#if defined(_SYMBIAN) || defined(_OPENWAVE)
#include "hlxclib/stdarg.h"
#define IDIGNORE	0
#define IDRETRY		1
#endif

#include "hxheap.h"
#ifdef DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	HXOutputDebugString: 
//		Helper function used by DEBUGOUTSTR(). This is better than 
//		OutputDebugString, because it will check to see if output
//		tracing is turned off in the registry. This prevents the massive
//		slew of output messages.
//
void STDMETHODCALLTYPE HXOutputDebugString(const char* pString)
{
	if (HXWantTraceMessages())
	{

#if _MACINTOSH
		Str255	pstr = {0x00};
		char	*p = (char*)pString;
		while (*p && pstr[0] < 253)	// interrupt safe c2pstr, and we don't mess with pString
			pstr[++pstr[0]] = *p++;
		pstr[++pstr[0]] = ';';		// add go command so we don't stay in MacsBug
		pstr[++pstr[0]] = 'g';
		DebugStr(pstr);

#elif defined( DEBUG ) && (defined( _WIN32 ) || defined( _WINDOWS ))

		// This is the Windows/Win 95 version
		OutputDebugString(OS_STRING(pString));

#elif defined( DEBUG) && defined(_LINUX)
                fprintf( stderr, "%s", pString);
#else
		//	Any other platforms....  Undefine it to be safe.
#endif

	} // end if
};

/////////////////////////////////////////////////////////////////////////////
//
//	HXDebugOptionEnabled: 
//		Determine if the given debug option is enabled.
//		A lookup is done to the registry key, and if it's present
//		and set to '1', TRUE is returned otherwise FALSE
//
#ifdef _WIN16
HXBOOL far _cdecl HXDebugOptionEnabled(const char* szOption)
#else
HXBOOL STDMETHODCALLTYPE HXDebugOptionEnabled(const char* szOption)
#endif
{

#if defined( DEBUG ) && (defined( _WIN32 ) || defined( _WINDOWS ))

	HKEY 	RootKey = HKEY_CLASSES_ROOT;
	char	szBuffer[10] = ""; /* Flawfinder: ignore */
	HKEY 	hKey;
	HXBOOL 	bEnabled = FALSE;
	HX_RESULT hRes;

	DWORD bufSize = sizeof(szBuffer) - 1;

	if( RegOpenKey(RootKey, szOption, &hKey) == ERROR_SUCCESS )
	{
		hRes = RegQueryValue(hKey, "", szBuffer, (long *)&bufSize);
		if (hRes == ERROR_SUCCESS && bufSize != 0)
		{
			if (strcmp(szBuffer,"1") == 0)
			{
				bEnabled = TRUE; 
			}
		}
		RegCloseKey(hKey);  
	}
	
	return bEnabled;

#else
	return FALSE;
	//	Any other platforms....  Undefine it to be safe.
#endif
}

#if defined(_SYMBIAN)
int QueryAssertActionL(const char* pAssertText)
{
    // Allocate assertion text
    TPtrC8 ptr8((const TUint8*)pAssertText);
    HBufC* pText = HBufC::NewLC(ptr8.Length());
    pText->Des().Copy(ptr8);

    // Show message
    TRequestStatus status = KRequestPending;
    CAknGlobalMsgQuery * pDlg = CAknGlobalMsgQuery::NewL();
    CleanupStack::PushL(pDlg);
    pDlg->ShowMsgQueryL(status, *pText, R_AVKON_SOFTKEYS_OK_DETAILS,
        _L("Assert failed!"), KNullDesC, 0, -1, CAknQueryDialog::EErrorTone );
    User::WaitForRequest(status);
   
    CleanupStack::PopAndDestroy(2); //pText, pDlg
           
    // Go to debugger if user clicked 'details', otherwise ignore
    int nCode = (EAknSoftkeyDetails == status.Int() ? IDRETRY : IDIGNORE);

    return nCode;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	HXWantTraceMessages: 
//		Helper function used to determine if the system has asked for trace 
//		messages.
//
HXBOOL STDMETHODCALLTYPE HXWantTraceMessages()
{
#if __MWERKS__

	return TRUE;

#elif defined( DEBUG ) && (defined( _WIN32 ) || defined( _WINDOWS ))

	HKEY 	RootKey = HKEY_CLASSES_ROOT;
	char	szBuffer[10] = ""; /* Flawfinder: ignore */
	HKEY 	hKey;
	HXBOOL 	bWantTrace = FALSE;
	HX_RESULT hRes;

	DWORD bufSize = sizeof(szBuffer) - 1;

	if( RegOpenKey(RootKey, "HXDebug", &hKey) == ERROR_SUCCESS )
	{
		hRes = RegQueryValue(hKey, "", szBuffer, (long *)&bufSize);
		if (hRes == ERROR_SUCCESS && bufSize != 0)
		{
			if (strcmp(szBuffer,"1") == 0)
			{
				bWantTrace = TRUE; 
			}
		}
		RegCloseKey(hKey);  
	}
	
	return bWantTrace;

#elif defined(_UNIX) && defined( DEBUG)
        const char* debugOutputOpts = getenv("HX_DEBUG");
        if( debugOutputOpts && strlen(debugOutputOpts) != 0)
        {
            return TRUE;
        }
        return FALSE;
#else
	return FALSE;
	//	Any other platforms....  Undefine it to be safe.
#endif
}

#if !defined(HELIX_CONFIG_NOSTATICS)
int g_trace_log_enabled = 0;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// HXTrace: Helper function used by HX_TRACE()
//
void STDMETHODVCALLTYPE HXTrace(const char* pszFormat, ...)
{
#if !defined(HELIX_CONFIG_NOSTATICS)    
    if(!g_trace_log_enabled)
	return;

    static char z_szDebugBuffer[MAX_TRACE_OUTPUT]; /* Flawfinder: ignore */

    va_list		vaMarker;

    va_start( vaMarker, pszFormat );
    vsnprintf( z_szDebugBuffer, sizeof(z_szDebugBuffer), pszFormat, vaMarker );
    va_end( vaMarker );
    DEBUGOUTSTR( z_szDebugBuffer );
#endif
};

/////////////////////////////////////////////////////////////////////////////
//
// HXAssertFailedLine: Helper function used by HX_ASSERT()
//
#ifdef _WIN16
// see comment in hxassert.h
HXBOOL far _cdecl HXAssertFailedLine(const char* pszExpression, const char* pszFileName, int nLine)
#else
HXBOOL STDMETHODCALLTYPE HXAssertFailedLine(const char* pszExpression, const char* pszFileName, int nLine)
#endif
{
#if !defined(HELIX_CONFIG_NOSTATICS)    
    static char z_szAssertMessage[MAX_TRACE_OUTPUT]; /* Flawfinder: ignore */
    static HXBOOL z_nMultiAssertCount = 0;
#else
    char z_szAssertMessage[MAX_TRACE_OUTPUT]; /* Flawfinder: ignore */
    HXBOOL z_nMultiAssertCount = 0;
#endif        

    // format message into buffer
    SafeSprintf(z_szAssertMessage, MAX_TRACE_OUTPUT, "(%s)... File %s, Line %d", pszExpression, pszFileName, nLine);

    // assume the debugger or auxiliary port
    // output into MacsBug looks better if it's done in one string,
    // since MacsBug always breaks the line after each output
    HX_TRACE("Assertion Failed: %s\n", z_szAssertMessage);

    if (z_nMultiAssertCount > 0)
    {
        // assert within assert (examine call stack to determine first one)
        HXDebugBreak();
        return FALSE;
    }
    z_nMultiAssertCount++;

/////////////////////////////////////////////////////////////////
//
// BEGIN: Platform specific portion of HXAssert(), namely, we 
// need to show some UI to tell us that an assertion has failed. 
// The rest of this function is cross-platform.
//
#if defined(_WIN32) || defined(_WINDOWS)

	// active popup window for the current thread
	HWND hWndParent = GetActiveWindow();
#if !defined(WIN32_PLATFORM_PSPC)
	if (hWndParent != NULL)
	{
            hWndParent = GetLastActivePopup(hWndParent);
	}
#endif /* !defined(WIN32_PLATFORM_PSPC) */
		
	// display the assert
#if !defined(WIN32_PLATFORM_PSPC)
	// we remove WM_QUIT because if it is in the queue then the message box
	// won't display
	MSG msg;
	HXBOOL bQuit = ::PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);

	int nCode = ::MessageBox
            (
                hWndParent, 
                z_szAssertMessage,
                "Assertion Failed!",
                MB_TASKMODAL|
                MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_DEFBUTTON3
#if defined(_WIN32)
                |MB_SETFOREGROUND
#endif
                );
	if (bQuit)
	    PostQuitMessage(msg.wParam);
#else /* !defined(WIN32_PLATFORM_PSPC) */
	int nCode = ::MessageBox
            (
                hWndParent, 
                OS_STRING(z_szAssertMessage),
                OS_STRING("Assertion Failed!"),
                MB_ICONHAND|MB_ABORTRETRYIGNORE
                );
#endif /* !defined(WIN32_PLATFORM_PSPC) */

#elif defined (_MACINTOSH)
        int nCode = HXCRT_ASSERT(z_szAssertMessage);

#elif defined (_UNIX)
        const char *debugopts;
        int debuglevel = 0;
        int nCode = IDIGNORE;
#if defined(_MAC_UNIX)
        debuglevel = 2; // default to something less extreme on Mac
#endif
        debugopts = getenv("HX_DEBUGLEVEL");
        if (debugopts != NULL)
        {
            debuglevel = atoi(debugopts);
        }
        switch(debuglevel)
        {
           case 1: /* debugger */
           {
               fprintf(stderr, "HX_ASSERT failed: %s\n", z_szAssertMessage );
               nCode = IDRETRY;
               break;
           }
           case 2: /* terminate */
           {
               fprintf(stderr, "HX_ASSERT failed: %s\n", z_szAssertMessage );
               nCode = IDIGNORE;
               break;

           }
           case 3: /* silent */
           {
               nCode = IDIGNORE;
               break;
           }
           case 4: /* interactive */
           {
               setvbuf(stdin, NULL, _IONBF, 0);
               fprintf(stderr, "HX_ASSERT failed: %s\n((d)ebug (i)gnore (a)bort)?  ", z_szAssertMessage );
               char input = '\n';
               while (input == '\n')
                   read(0, &input, 1);
               switch (input)
               {
                  case 'i':	            /* ignore */
                      nCode = IDIGNORE;
                      break;
                  case 'a':               /* abort */
                      abort();
                      break;
                  case 'd':
                  default:                /* debug */
                      nCode = IDRETRY;
                      break;
               }
               break;
           }
           case 0: /* terminate */
           default:
           {
               fprintf(stderr, "HX_ASSERT failed: %s\n", z_szAssertMessage );
               abort();
           }
        }
#endif

#if defined(_SYMBIAN)
    int nCode = IDRETRY;
    TRAPD(err, nCode = QueryAssertActionL(z_szAssertMessage));
#endif //_SYMBIAN        
       
#if defined(_OPENWAVE)
        int nCode = IDIGNORE;

// XXXSAB Fill this in!!!

# ifdef _OPENWAVE_SIMULATOR
        //What to do on the emulator.
# else
        //What to do on the device.
# endif        
#endif        
//
// END: Platform specific portion of HXAssert(). The rest of 
// this function is cross-platform.
//
/////////////////////////////////////////////////////////////////

            // cleanup
            z_nMultiAssertCount--;

            if (nCode == IDIGNORE)
            {
                return FALSE;   // ignore
            }

            if (nCode == IDRETRY)
            {
		return TRUE;
            }

            HXAbort();     // should not return (but otherwise HXDebugBreak)
            return TRUE;

};

/////////////////////////////////////////////////////////////////////////////
//
// HXAssertValidPointer: Helper function used by HX_ASSERT_VALID_PTR()
//
void STDMETHODCALLTYPE HXAssertValidPointer(const void* pVoid, const char* pszFileName, int nLine)
{
	if (pVoid == NULL)
	{
		if (HXAssertFailedLine("HX_ASSERT_VALID_PTR fails on NULL pointer",pszFileName,nLine))
		{
			HXDebugBreak();
		}
		return;     // quick escape
	}

	if (!HXIsValidAddress(pVoid))
	{
		if (HXAssertFailedLine("HX_ASSERT_VALID_PTR fails with illegal pointer.",pszFileName,nLine))
		{
			HXDebugBreak();
		}
		return;     // quick escape
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// HXIsValidAddress: Helper function used by HXAssertValidPointer()
//
#ifdef _WIN16
// see comment in hxassert.h for problem with STDMETHODCALLTYPE in win16
HXBOOL far _cdecl        HXIsValidAddress(const void* lp, ULONG32 nBytes, HXBOOL bReadWrite)
#else
HXBOOL STDMETHODCALLTYPE HXIsValidAddress(const void* lp, ULONG32 nBytes, HXBOOL bReadWrite)
#endif
{

/////////////////////////////////////////////////////////////////
//
// BEGIN: Platform specific portion of HXIsValidAddress(), namely,
// we need to to check if a pointer is a valid pointer.
//
#if defined(_WIN32) || defined(_WINDOWS)
	// simple version using Win APIs for pointer validation.
	return	(
				(lp != NULL)
				 &&
				 !IsBadReadPtr(lp, (UINT)nBytes)
				 &&
				(!bReadWrite || !IsBadWritePtr((LPVOID)lp, (UINT)nBytes))
			);
#else
#  ifdef __MWERKS__
		return TRUE;
#  endif
#  ifdef _UNIX
		return lp != NULL;
#  endif
#  ifdef _SYMBIAN
		return lp != NULL;
#  endif
#  ifdef _OPENWAVE
		return lp != NULL;
#  endif
#endif
//
// END: Platform specific portion of HXIsValidAddress(). The rest 
// of this function is cross-platform.
//
/////////////////////////////////////////////////////////////////

}

#ifdef _WIN16
// see comment in hxassert.h for problem with STDMETHODCALLTYPE in win16
HXBOOL far _cdecl        HXIsValidString(const char* psz, int nLength)
#else
HXBOOL STDMETHODCALLTYPE HXIsValidString(const char* psz, int nLength)
#endif
{
	if (psz == NULL)
	{
		return FALSE;
	}

/////////////////////////////////////////////////////////////////
//
// BEGIN: Platform specific portion of HXIsValidString(), namely,
// we need to to check if a pointer is a valid string pointer.
//
#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC)
	// simple version using Win APIs for pointer validation.
        return !IsBadStringPtr(psz, nLength);
#else
	return TRUE;
#endif
}

#if defined(_SYMBIAN) && !defined (__WINS__)
#include <e32std.h>
void HXDebugBreak()
{
    User::Invariant();
}
#elif defined(_OPENWAVE) && !defined(_OPENWAVE_SIMULATOR)
void HXDebugBreak()
{
    #error Figure out if HXDebugBreak() makes sense on target device...
}
#elif defined(_UNIX)
void HXDebugBreak() 
{
    static int debuggerpid = 0;
    pid_t pid = getpid();
    const char *pname = getenv("PROCESS_NAME");
    const char *pDebuggerProcess = getenv("HX_DEBUGGER");
    
    if (debuggerpid) 
    {
	kill(pid, SIGSTOP);
	return;
    }

    // This allows the user to override the debug command.  The command 
    // called will be given process name and pid arguments

    if (pDebuggerProcess)
    {
	char pCmdTemplate[1024], pCmd[1024]; /* Flawfinder: ignore */
	SafeSprintf(pCmdTemplate, 1024, "%s %%s %%d", pDebuggerProcess);
	SafeSprintf(pCmd, 1024, pCmdTemplate, pname? pname : "/dev/null", pid);
	system(pCmd);
	sleep(3);
        return;
    }

    debuggerpid = fork();
    if (debuggerpid)
    {
	sleep(3);
    }
    else
    {
        setsid();
        int kid;
        if (!(kid = fork()))
        {
            char buf[1024]; /* Flawfinder: ignore */
            sprintf(buf, "%d", pid); /* Flawfinder: ignore */

	    if (!pname)
	    {
		fprintf(stderr, "Need to set PROCESS_NAME to enable jit debugging\n");
                fflush(0);
		_exit(0);
	    }

            if (-1 == (execlp("xterm", "xterm", "-e", "gdb", "-nw", "-nx", pname, buf, NULL)))
            {
                fprintf(stderr, "failed to start debugger (%s)\n", strerror(errno));
                abort();
            }
        }
        else
        {
            int dummy;
            waitpid(kid, &dummy, 0);
            fflush(0);
            _exit(0);
        }
    }
}
#endif

#endif
