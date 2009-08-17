/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: debug.h,v 1.4 2004/07/09 18:21:47 hubbe Exp $
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

#ifndef	_DEBUG_H_
#define	_DEBUG_H_

#include "hlxclib/stdio.h"
#include <stdarg.h>

extern void vdprintf(int, int, const char*, va_list);

#ifdef HELIX_CONFIG_NOSTATICS
# include "globals/hxglobals.h"
#endif


#if !defined(HELIX_CONFIG_NOSTATICS)
extern const char* progname;
#else
extern const char* const progname;
#endif

extern FILE* error_file;

// Profiling
#if defined (DEBUG) && (defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD))
#define PRE_PROFILE							\
    	struct rusage before;						\
    	getrusage(RUSAGE_SELF, &before);
#define AFT_PROFILE							\
    	struct rusage after;						\
    	int x[13];							\
    	getrusage(RUSAGE_SELF, &after);					\
    	x[0]  = (after.ru_utime.tv_sec   * 1000000 +			\
    	    	 after.ru_utime.tv_usec) -				\
    	        (before.ru_utime.tv_sec  * 1000000 +			\
    	    	 before.ru_utime.tv_usec);				\
    	x[1]  = (after.ru_stime.tv_sec   * 1000000 +			\
    	    	 after.ru_stime.tv_usec) -				\
    	        (before.ru_stime.tv_sec  * 1000000 +			\
    	    	 before.ru_stime.tv_usec);				\
    	x[2]  = after.ru_minflt   - before.ru_minflt;			\
    	x[3]  = after.ru_majflt   - before.ru_majflt;			\
    	x[4]  = after.ru_nswap    - before.ru_nswap;			\
    	x[5]  = after.ru_inblock  - before.ru_inblock;			\
    	x[6]  = after.ru_oublock  - before.ru_oublock;			\
    	x[7]  = after.ru_nsignals - before.ru_nsignals;			\
    	x[8]  = after.ru_msgsnd   - before.ru_msgsnd;			\
    	x[9]  = after.ru_msgrcv   - before.ru_msgrcv;			\
    	x[10] = after.ru_nvcsw	  - before.ru_nvcsw;			\
    	x[11] = after.ru_nivcsw   - before.ru_nivcsw;			\
    	x[12] = after.ru_maxrss   - before.ru_maxrss;			\
	DPRINTF(D_PROF,	("\n"						\
	"       user     %3dus  system %3dus pg-reclaim %3d\n"		\
    	"   	pg-fault %3d    swaps  %3d   blkin %3d\n"		\
    	"   	blkout   %3d    nsig   %3d   msg-sent %3d\n"		\
    	"       msg-rcv  %3d\n"						\
	"       voluntary / involuntary context switches %3d / %3d\n"	\
	"       Max RSS setsize %3d\n",					\
	    x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],		\
    	    x[8], x[9], x[10], x[11], x[12]));
#else
#define PRE_PROFILE
#define AFT_PROFILE
#endif

#ifdef DEBUG
extern "C" void rm_panic(const char*, ...)
#ifdef  __GNUC__
__attribute__((format(printf,1,2)))
#endif
;

#define PANIC(x) rm_panic x
#else
#define PANIC(x)
#endif

#ifdef DEBUG

inline int& debug_level()
{
#if defined(HELIX_CONFIG_NOSTATICS)
    static const INT32 nDebugLevel = 0;
    return (int&)HXGlobalInt32::Get(&nDebugLevel, 0 );
#else
    static int nDebugLevel = 0;
    return nDebugLevel;
#endif    
}

inline int& debug_func_level()
{
#if defined(HELIX_CONFIG_NOSTATICS)
    static const INT32 nDebugFuncLevel = 0;
    return (int&)HXGlobalInt32::Get(&nDebugFuncLevel, 0 );
#else
    static int nDebugFuncLevel = 0;
    return nDebugFuncLevel;
#endif    
}



    extern void dprintf( const char *, ... )
#ifdef __GNUC__
	__attribute__((format(printf,1,2)))
#endif /* __GNUC__ */
    ;
    extern void dprintfx( const char *, ... )
#ifdef __GNUC__
	__attribute__((format(printf,1,2)))
#endif /* __GNUC__ */
    ;

#ifdef DEVEL_DEBUG
#define	DPRINTF(mask,x)	if (debug_level() & (mask))	{	\
	dprintf("%s:%d ", __FILE__, __LINE__); dprintfx x; } else
#else
#define	DPRINTF(mask,x)	if (debug_level() & (mask)) dprintf x; else
#endif /* DEVEL_DEBUG */

#else /* No Debuggging */
#ifdef _CARBON
#undef DPRINTF
#endif
#define	DPRINTF(mask,x)
#endif

#if 0
inline void
DPRINTF(int mask, const char* fmt, ...)
{
#ifdef	DEBUG
    extern void dprintf(const char*, ...);
    extern void dprintfx(const char*, ...);
    extern int debug_level;
    if ((debug_level & mask) == 0)
	return;
    va_list args;
    va_start(args, fmt);
    vdprintf(1, 0, fmt, args);
    va_end(args);
#endif
}
#endif

#endif/*_DEBUG_H_*/
