/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: osdetect.cpp,v 1.2 2005/08/01 13:09:38 dcollins Exp $
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

#ifdef _UNIX
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include "hxtypes.h"
#include "hxcom.h"

BOOL
OSDetect(BOOL bUnsupportedOSFlag)
{
    BOOL bUnsupportedOSDetected = FALSE;

#ifdef _LINUX
    struct utsname pinfo;
    uname(&pinfo);

    int nMajor=0, nMinor=0, nRev=0;
    sscanf(pinfo.release, "%d.%d.%d", &nMajor, &nMinor, &nRev);

#if defined _CS_GNU_LIBPTHREAD_VERSION && defined _CS_GNU_LIBC_VERSION
    char szPThreadVer[32];
    confstr (_CS_GNU_LIBPTHREAD_VERSION, szPThreadVer, sizeof(szPThreadVer));
    char szLibCVer[32];
    confstr (_CS_GNU_LIBC_VERSION, szLibCVer, sizeof(szLibCVer));
    printf("Linux kernel version %s detected [%s/%s]\n",
           pinfo.release, szLibCVer, szPThreadVer);
    fflush(0);

    if (strncmp(szPThreadVer, "LinuxThreads ", 13) == 0)
    {
        printf("Attention: The LinuxThreads threading library is not supported.\n");
        fflush(0);
        bUnsupportedOSDetected = TRUE;
    }

#else
    printf("Linux kernel version %s detected\n", pinfo.release);
#endif

    if (nMajor < 2 || (nMajor == 2 && nMinor < 6))
    {
        printf("Attention: This kernel version is not supported.  See the release notes.\n");
        fflush(0);
        bUnsupportedOSDetected = TRUE;
    }

    // odd minor rev == development kernel!
    //This is disabled since the future versioning scheme of the Linux kernel is unclear.
    //if (nMinor % 2 != 0)
    //{
    //    printf("Attention: This kernel is a development/beta/experimental kernel.  See the release notes.\n");
    //    fflush(0);
    //    bUnsupportedOSDetected = TRUE;
    //}

    const char* szLDAssumeKernel = getenv("LD_ASSUME_KERNEL");
    if (szLDAssumeKernel)
    {
        UINT32 ulAssKernMaj=0;
        UINT32 ulAssKernMinor=0;
        sscanf(szLDAssumeKernel, "%ld.%ld", &ulAssKernMaj, &ulAssKernMinor);
        if (ulAssKernMaj == 1 || (ulAssKernMaj == 2 && ulAssKernMinor < 6))
        {
            printf("Attention: The LD_ASSUME_KERNEL environment variable is set to '%s' "
                   "which is not supported.  See the release notes.\n",
                   szLDAssumeKernel);
            fflush(0);
            bUnsupportedOSDetected = TRUE;
        }
    }

    if (bUnsupportedOSDetected)
    {
        if (bUnsupportedOSFlag)
        {
            printf("Warning: Unsupported OS configuration detected.\n");
            fflush(0);
            sleep(2); //allow a moment for messages to flush
        }
    }

#elif defined(_UNIX)
    // on other Unixes just dump the uname() date for informational purposes
    struct utsname pinfo;
    uname(&pinfo);
    printf("Detected %s %s %s %s %s\n", pinfo.sysname,
        pinfo.nodename, pinfo.release, pinfo.version, pinfo.machine);
#endif

    return !bUnsupportedOSDetected;
}
