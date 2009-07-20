/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unix_misc.cpp,v 1.5 2007/07/06 20:39:21 jfinnecy Exp $
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

#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "debug.h"

// srobinson : What UNIX was this included for? - perhaps
// we can replace this with an #ifndef for that platform
//
// dbrumley  : I think sys/errono.h is needed on multiple platforms
// (Linux 1&2, FreeBSD, SCO UnixWare, others?).
//          
#if defined _SOLARIS || defined _IRIX || defined _OSF1 || defined _HPUX || defined _AIX || defined _BEOS || defined __QNXNTO__
#include <errno.h>
#include <string.h>                     // for strerror() prototype
#else
#include <sys/errno.h>
#endif

#include "unix_misc.h"
#include "debug.h"

#ifdef _BEOS
#define MAX_BEOS_FDLIMIT 2000
int setfdlimit(int limit)
{
    return limit > MAX_BEOS_FDLIMIT ? MAX_BEOS_FDLIMIT : limit;
}
#else
#if     defined __aix__

int setfdlimit(int limit)
{
        return limit > MAX_AIX_FDLIMIT ? MAX_AIX_FDLIMIT : limit;
}
#else

#if     defined _FREEBSD || defined _OPENBSD || defined _NETBSD
#define RLIM_FMT "%qd"
#else
#define RLIM_FMT "%d"
#endif  /* FreeBSD, *BSD, etc. */

int
setfdlimit(int limit)
{
        struct rlimit rl;
       int reallimit = limit;

        if (getrlimit(FDLIMIT, &rl) < 0) {
                DPRINTF(D_INFO, ("Can not get resource limit: %s\n", 
			strerror(errno)));
                return limit;
        }
        
        DPRINTF(D_INFO, ("cur: " RLIM_FMT ", max: " RLIM_FMT "\n",
                rl.rlim_cur, rl.rlim_max));

        /*
         * Can't get more than the system max limit
         * so adjust the requested limit down.
         */
        if (limit > rl.rlim_max)
                limit = rl.rlim_max;

        /*
         * Set the new limit if it is higher than the current limit
         */
        if (rl.rlim_cur < limit) {
                rl.rlim_cur = limit;

                if (setrlimit(FDLIMIT, &rl) < 0) {
                        DPRINTF(D_INFO, ("Can not set resource limit: %s\n", 
                                strerror(errno)));
                }

                rl.rlim_max = limit;

                if (setrlimit(FDLIMIT, &rl) < 0) {
                        DPRINTF(D_INFO, ("Can not set resource limit: %s\n", 
                                strerror(errno)));
                }

                getrlimit(FDLIMIT, &rl);
        }

#if defined _SUN || defined _SOLARIS
       /* Solaris has a broken getrlimit.  We'll try it the hard way */
       while (rl.rlim_cur < reallimit)
       {
           rl.rlim_cur += 512;
           rl.rlim_max = rl.rlim_cur;
           setrlimit(FDLIMIT, &rl);
       }
#endif

        DPRINTF(D_INFO, ("cur: " RLIM_FMT ", max: " RLIM_FMT "\n",
                rl.rlim_cur, rl.rlim_max));

        return rl.rlim_cur;
}

#endif /* __aix__ */
#endif /* _BEOS */


