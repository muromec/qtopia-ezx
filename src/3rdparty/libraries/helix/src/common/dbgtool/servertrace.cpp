/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servertrace.cpp,v 1.1 2006/09/19 20:24:41 dcollins Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#ifdef HELIX_FEATURE_SERVER

#include "hxtypes.h"
#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/time.h"
#include "hlxclib/stdarg.h"
#include "hxtime.h"
#include "servertrace.h"

#ifdef _UNIX
#include <pthread.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#ifdef _LINUX
#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
_syscall0(pid_t,gettid)
pid_t gettid(void);
#endif

extern "C" char* get_trace();


void
ServerTrace::Print(const char* pszHeader, const char* pszFmt, ...)
{
    printf("\n-------------------------------------------------------------------------------\n");
    printf("*** %s\n", pszHeader ? pszHeader : "");

    PrintTime();
    PrintThreadId();

    va_list args;
    va_start(args, pszFmt);
    if (pszFmt)
    {
        vfprintf(stdout, pszFmt, args);
    }
    va_end(args);

    PrintTrace();

    printf("\n-------------------------------------------------------------------------------\n");
    fflush(stdout);
}


void
ServerTrace::PrintTrace(void)
{
    printf("\nTrace:\n%s", get_trace());
}

void
ServerTrace::PrintTime(void)
{
    HXTime now;
    struct tm tm;
    char szTime[64];

    gettimeofday(&now, NULL);
    hx_localtime_r((const time_t *)&now.tv_sec, &tm);
    strftime(szTime, sizeof(szTime), "%d-%b-%y %H:%M:%S", &tm);

    printf("When: %s\n", szTime);
}


void
ServerTrace::PrintThreadId(void)
{
#ifdef _WIN32
    printf("TID %lu\n", GetCurrentThreadId());
#elif defined(_LINUX)
    printf("TID %lu/%lu\n", pthread_self(), (UINT32)gettid());
#else
    printf("TID %lu\n", pthread_self());
#endif
}


#endif /* HELIX_FEATURE_SERVER */
