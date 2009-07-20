/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxtime.h,v 1.10 2008/01/18 07:35:20 vkathuria Exp $
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

#ifndef _HXTIME_H_
#define _HXTIME_H_

/*
 * Place for platform independent time functions. Right now, using this
 * wierd include scheme we mimic how we have been handling 'platform 
 * independent' time in the server. 
 * This really should be replaced with some platform indpendent function
 */

#if (defined _UNIX && !defined _VXWORKS) || (defined(_MACINTOSH) && defined(_MAC_MACHO))
#       if defined  _LINUX && !defined _RED_HAT_5_X_
#   	      	define int long
#      	        include <linux/time.h>
#       	undef int
#	elif defined _AIX || defined _VXWORKS || defined __QNXNTO__
#		include <time.h>
#       endif /* _LINUX */
#       include <sys/time.h>

class HXTime : public timeval {};

#elif defined (_WINDOWS) || defined (_WIN32) || defined(_VXWORKS) || defined(_OPENWAVE) || defined(_BREW)

#       include "hlxclib/time.h"

class HXTime {
public:
        LONG32    tv_sec;         /* seconds */
        LONG32    tv_usec;        /* and microseconds */
};

#elif (defined _MACINTOSH)
#   include "hxtypes.h"
#   include <time.h>    
#   include <utime.h>

class HXTime : public timeval 
{
public:
	HXTime() 
	{
		tv_sec=NULL;
		tv_usec=NULL;	
	};
};

	typedef long tv_sec_t;
	typedef long tv_usec_t;
/*
	struct timeval 
	{
		tv_sec_t	tv_sec;
		tv_usec_t	tv_usec;
	};
*/
#elif defined(_SYMBIAN)
# include <sys/time.h>
class HXTime : public timeval {};
#else
#	error need to include time functions (and timeval).
#endif /* _UNIX */

#if defined (_WINDOWS) || defined (_WIN32) || defined (_MACINTOSH) || defined(_VXWORKS) || defined(_OPENWAVE) || defined(_BREW)
	int gettimeofday(HXTime* time, void* unused);
#endif /* WINDOWS OR MAC OR BREW*/

#if (defined _MACINTOSH)
/* Mac time is secs since 1904 */
time_t WinToMacTime(time_t timeT);
time_t MacToWinTime(time_t timeT);
#endif

#endif /* _HXTIME_H_ */
