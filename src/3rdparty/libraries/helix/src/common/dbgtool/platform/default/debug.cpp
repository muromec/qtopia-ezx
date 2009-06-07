/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: debug.cpp,v 1.4 2005/07/20 21:37:03 dcollins Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/stdarg.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/time.h"

#include "hxtypes.h"

#include "debug.h"

#include "hxtime.h"
#include "hxproc.h"
#include "hxassert.h"
#include "hxstrutl.h"
#include "hlxclib/time.h"

#ifdef _MACINTOSH
#include <stdlib.h> /* for exit */
#endif

#if (defined WIN32 && defined _SERVER)
#include "messages.h"
#endif 

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if !defined(HELIX_CONFIG_NOSTATICS)
const char* progname = "unknown";
#else
const char* const progname = "unknown";
#endif

#if defined(_SYMBIAN)
#error not for symbian
#endif

#if (defined WIN32 && defined _SERVER)
void
logprintf(int code, char* buf)
{
	HANDLE EventSource;
	WORD EventType;

	switch(code) {

		case 1:
			EventType = EVENTLOG_ERROR_TYPE;
			break;

		default:
			EventType = EVENTLOG_INFORMATION_TYPE;
	}

	//
	// Use event logging to log the error.
	//

	EventSource = RegisterEventSource(NULL, TEXT("HXServer"));

	if (EventSource != NULL) {
	    ReportEvent(EventSource,
	                EventType,
	                0,
	                MSG_PNSERVEREVENT,
	                NULL,
	                1,
	                0,
	                (const char **)&buf,
	                NULL);

	    (VOID) DeregisterEventSource(EventSource);
	}
}

#endif

#define BUF_SIZE 4096

void
vdprintf(int print_line_header, int err, const char* fmt, va_list args)
{ 
	char *buf = new char[BUF_SIZE];

	if (!buf)
	{
	    return;
	}
	
#if (defined _MACINTOSH)
// The implementation of time on the mac is quite different from UNIX.
// For now, I hacked the debug time to not include the milliseconds.
// I call time() to get the actual date.

	struct tm tm;
	time_t now;
	::time(&now);
	hx_localtime_r(&now, &tm);
	strftime(buf, BUF_SIZE, "%d-%b-%y %H:%M:%S", &tm);
	
	ProcessSerialNumber		process_id;
	GetCurrentProcess(&process_id);
	
	int len = SafeSprintf(buf, BUF_SIZE, "%s %s(%ld): ", buf, progname, process_id.lowLongOfPSN);
	
        if (len < 0) len = 0;
	vsprintf(&buf[len], fmt, args);

#elif defined(_WINDOWS) && (!defined(WIN32) || defined(WIN32_PLATFORM_PSPC))
// for the win 16 implementation we just skip line headers to avoid linking
// in a library that messes up the client core for win 16.
	vsprintf(buf, fmt, args);
#else
	if (print_line_header) {
		struct tm tm;
		HXTime now;
		UINT32 ulProcId = 0;
		gettimeofday(&now, 0);
		hx_localtime_r((const time_t *)&now.tv_sec, &tm);
		size_t len = strftime(buf, BUF_SIZE, "%d-%b-%y %H:%M:%S", &tm);

#if !defined(_OPENWAVE)    // temporary work-around
		ulProcId = process_id();
#endif	// _OPENWAVE

                int nWrite = SafeSprintf(&buf[len], BUF_SIZE - len, 
                    ".%03ld %s(%ld): ", now.tv_usec/1000, progname, ulProcId);
                if (nWrite > 0)
                {
                    len += nWrite;
                }
                vsnprintf(&buf[len], BUF_SIZE-len, fmt, args);
	} else
		vsnprintf(buf, BUF_SIZE, fmt, args);
#endif

#if (defined WIN32 && defined _SERVER)

	if (IsService) {
		logprintf(err, buf);
		delete [] buf;
		return;
	}

#endif

#if !defined( _MACINTOSH)
	if (err) {
		fprintf(stderr, "***");
	}

	fprintf(stderr, "%s", buf); 
#endif

// For the macintosh using printf with the debug.console.c, will display the 
// debug messages to a window as well, when using the dummy console for linking
// this function does nothing.
#if (defined _MACINTOSH && defined  _DEBUG)
	if (err) 
	    fprintf(stderr, "***");
	fprintf(stderr, "%s", buf);
	printf("%s",buf);
#endif

	fflush(stderr);

	delete [] buf;
}


void
dprintf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vdprintf(1, 0, fmt, args);
	va_end(args);
}

void
dprintfx(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vdprintf(0, 0, fmt, args);
	va_end(args);
}

void
rm_panic(const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vdprintf(1, 1, fmt, args);
	va_end(args);
	int In_PANIC_Macro = 0;
	HX_ASSERT(In_PANIC_Macro);
}

