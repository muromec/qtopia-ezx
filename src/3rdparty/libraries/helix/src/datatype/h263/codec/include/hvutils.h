/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvutils.h,v 1.5 2007/07/06 22:00:32 jfinnecy Exp $
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

#ifndef HVUTILS_H
#define HVUTILS_H

#include "machine.h"
#include <stdio.h>

#ifdef FOR_MAC
// #define MEM_LEAK_TEST 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define kNumberOfTempBlocks 100
#define kSizeOfEachBlock 20000

void GetDebugSettings(void);
S32 GetUserPlaybackSampleRate(void);
char LoggingEnabled(void);
S32 GetMinimumRequiredMemory(HXBOOL isStandalone);
S32 GetMinRingBlocks(void);
S32 GetMaxRingBlocks(void);
S32 GetRingBlockSize(void);
HXBOOL VMIsEnabled(void);
S32 GetLimitDataRate(void);


// Comment this out to suppress debugging logfile generation
#if defined(FOR_MAC)
#define FOR_LOGFILES 1

// For compatibility with older include files
// #define USE_OLDER_NAMES 1
#endif

/* this brings dprintf() to life  */
#ifndef _LINUX
extern void dprintf(char *format, ...);
#endif
extern void dflush(void);

extern void vvlogend();
extern void vvlog(U8 eventowner, U32 event, char *format, ...);


#ifdef __cplusplus
}
#endif

// Include platform-specific utilities as well

#ifdef FOR_MAC
#include <ctype.h> // for isspace
#endif
#ifdef FOR_WINDOWS
#include "HvUtilsW.h"
#endif
#ifdef FOR_UNIX
#if 0
#include "hvutilsu.h"
#endif

#define STRCMP      strcmp
#define STRICMP     strcasecmp
#define STRNICMP    strncasecmp
#define STRCOLL     strcoll
#define STRSTR      strstr
#define STRCHR      strchr
#define STRPBRK     strpbrk
#define STRPBRK     strpbrk
#define STRSPN      strspn
#define STRCSPN     strcspn

#endif

#endif // HVUTILS_H
