/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: microsleep.h,v 1.11 2008/01/18 07:35:20 vkathuria Exp $
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

#ifndef _MICROSLEEP_H_
#define _MICROSLEEP_H_

#if defined _HPUX || defined _AIX
#include <sys/time.h>
#endif
#include "hlxclib/time.h"

#ifdef _UNIX
#include <unistd.h>
#endif

#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif

#ifdef _BEOS	// for snooze()
#include <OS.h>
#endif

#ifdef _SYMBIAN
#include <e32std.h>
#endif

#if defined (_OSF1) && defined (_NATIVE_COMPILER) 
#include <sys/types.h>
int usleep(useconds_t useconds);
#endif

__inline void
microsleep(unsigned int uSecs)
{
#if defined _HPUX || defined _SOLARIS
    struct timespec interval, remainder;

    // sleep for 1 micro-second or 1000 nano-seconds
    interval.tv_sec = 0;
    interval.tv_nsec = uSecs * 1000;
#endif

#ifdef _AIX
    struct timestruc_t interval, remainder;
    interval.tv_sec = 0;
    interval.tv_nsec = uSecs * 1000;
    nsleep(&interval, &remainder);
#elif defined _SOLARIS
    nanosleep(&interval, &remainder);
#elif defined _WIN32 
    Sleep(uSecs / 1000);
#elif defined _OSF1 || defined _HPUX
    usleep((useconds_t)uSecs);
#elif defined _MACINTOSH
    AbsoluteTime timeToWakeUp = AddDurationToAbsolute( uSecs * kDurationMicrosecond, UpTime() );
    MPDelayUntil(&timeToWakeUp);
#elif defined _BEOS
    snooze(uSecs);
#elif defined _SYMBIAN
    User::After(uSecs);
#elif defined _BREW
    MSLEEP(uSecs/1000);
#else
    usleep(uSecs);
#endif
}

#endif
