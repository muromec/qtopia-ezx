/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: unixio.cpp,v 1.4 2003/06/27 02:41:30 dcollins Exp $ 
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

#include <string.h>
#include <sys/types.h>
#include "hxtypes.h"
#include "hxtime.h"
#ifndef _VXWORKS
#include <sys/resource.h>
#endif

#include "debug.h"

#include "bio.h"

#ifdef RLIMIT_OFILE
#undef FDLIMIT
#define FDLIMIT RLIMIT_OFILE
#endif /* RLIMIT_OFILE */

#ifdef RLIMIT_NOFILE
#undef FDLIMIT
#define FDLIMIT RLIMIT_NOFILE
#endif /* RLIMIT_NOFILE */

#if defined _FREEBSD || defined _OPENBSD || defined _NETBSD
#	define RLIM_FMT "%qd"
#else
#   define RLIM_FMT "%d"
#endif /* defined _FREEBSD || defined _OPENBSD || defined _NETBSD */

#ifdef _BEOS
#define MAX_BEOS_FDLIMIT 2000
int
setfdlimit(INT32 limit)
{
    return limit > MAX_BEOS_FDLIMIT ? MAX_BEOS_FDLIMIT : limit;
}
#else
#if defined _AIX
#define MAX_AIX_FDLIMIT 2000
int
setfdlimit(INT32 limit)
{
    return limit > MAX_AIX_FDLIMIT ? MAX_AIX_FDLIMIT : limit;
}

#else

int
setfdlimit(INT32 limit)
{
    extern int errno;
    struct rlimit rl;

    if (getrlimit(FDLIMIT, &rl) < 0) 
    {
	DPRINTF(D_INFO, ("Can not get resource limit: %s\n", strerror(errno)));
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
    if (rl.rlim_cur < limit) 
    {
	rl.rlim_cur = limit;

	if (setrlimit(FDLIMIT, &rl) < 0) 
	{
	    DPRINTF(D_INFO, ("Can not set resource limit: %s\n",
	                     strerror(errno)));
	}

	getrlimit(FDLIMIT, &rl);
    }

    DPRINTF(D_INFO, ("cur: " RLIM_FMT ", max: " RLIM_FMT "\n",
		     rl.rlim_cur, rl.rlim_max));

    return limit;
}

#endif /* _AIX */
#endif /* _BEOS */
