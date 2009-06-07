/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: debugout.h,v 1.5 2004/07/09 18:21:50 hubbe Exp $
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

/*
 *	DebugOut.h	A platform independent way of defining instrumentation macros.
 *				Currently we're only implementing a simple one.
 *				And so far nobody else has signed on except for the Windows & Win32
 *				platforms.
 *
 *	DEBUGOUTSTR( DEBUG_STR )	: Sends DEBUG_STR to a debug window of a platform
 *									dependent sort.
 *
 *	DEBUGPRINTF( char *DEBUG_BUFF, char *DEBUG_FORMAT, ... )
 *								: Sends a printf() style formated string to the
 *									instrumentation device.
 *
 */
#ifndef __DEBUGOUT_H__
#define __DEBUGOUT_H__
#ifdef __cplusplus
extern "C" {
#endif

#if _MACINTOSH
	//	This is the macintosh version, I'm not sure what it should look like
	//	exactly.
	#if defined( Debug_Signal )
		#define  DEBUGOUTSTR( DEBUG_STR )	{DebugStr(c2pstr(DEBUG_STR));\
											p2cstr((unsigned char *)DEBUG_STR); }
	#else
		#define	DEBUGOUTSTR( DEBUG_STR )
	#endif
#elif defined( _DEBUG ) && (defined( _WIN32 ) || defined( _WINDOWS ))
	//	This is the Windows/Win 95 version
	#if defined( _DEBUG )
		#include "hxassert.h" // Needed for HXOutputDebugString()
		#define	DEBUGOUTSTR( DEBUG_STR )	HXOutputDebugString( DEBUG_STR )
	#else
		#define DEBUGOUTSTR( DEBUG_STR )
	#endif
#elif defined(_DEBUG) && defined(_VXWORKS)
	#define DEBUGOUTSTR(DEBUG_STR) {printf("%s\n", DEBUG_STR);}
#elif defined(_DEBUG) && defined(_UNIX)
#        define DEBUGOUTSTR(DEBUG_STR) {FILE*fff = fopen("trace.log", "w"); fprintf(fff, "%s\n", DEBUG_STR); fclose(fff);}
#else
	//	Any other platforms....  Undefine it to be safe.
	#define DEBUGOUTSTR( DEBUG_STR )
#endif


#if defined( _DEBUG ) && (defined( _WIN32 ) || defined( _WINDOWS ))
#include <stdio.h>
#include <stdarg.h>
void __inline DEBUGPRINTF( char *DEBUG_BUFF, char *DEBUG_FORMAT, ... )
{
	va_list		vaMarker;

	va_start( vaMarker, DEBUG_FORMAT );
	vsprintf( DEBUG_BUFF, DEBUG_FORMAT, vaMarker );
	va_end( vaMarker );
	DEBUGOUTSTR( DEBUG_BUFF );
}
#else
#define DEBUGPRINTF( DEBUG_BUFF, DEBUG_FORMAT )
#endif


/*
 *	Use this macro to break into the debugger at execution time.
 *	If your platform doesn't define such functionality, then you
 *	are cool to leave the macro undefined, in which case it's a nop.
 */
#if defined( _DEBUG )
	//	This case is for Win16 & Win32
	#if defined( _WINDOWS )
		#if defined( _M_ALPHA )
			#include "windows.h"
			#define DEBUGGER()	DebugBreak()
		#else
			#define DEBUGGER()	_asm { int 3 }
		#endif	// _M_ALPHA
	#elif defined( _MACINTOSH )
		#define DEBUGGER()		Debugger();
	#elif defined( _UNIX )
		//	NEED a definition for UNIX
		#define DEBUGGER()
	#elif defined( _SYMBIAN )
                #define DEBUGGER() 
	#endif
#else
	#define DEBUGGER()
#endif	// else case for defined( _DEBUG )

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // __DEBUGOUT_H__
