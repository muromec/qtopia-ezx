/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprofile.h,v 1.5 2007/07/06 20:35:09 jfinnecy Exp $
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

#ifndef _HXPROFILE_H_
#define _HXPROFILE_H_

#ifdef ENABLE_ACTIVE_PROFILING

#ifdef _DEBUG

#ifdef _MACINTOSH

#include "Profiler.h"

#define HX_CODE_PROFILE_INIT( inNumFunctions, inStackSize ) \
static short hx_code_profile_numFunctions = inNumFunctions; \
static short hx_code_profile_stackSize = inStackSize;

#define HX_CODE_PROFILE_START \
OSErr hx_code_profile_start_error = ::ProfilerInit( collectDetailed, bestTimeBase, hx_code_profile_numFunctions, hx_code_profile_stackSize ); \
if ( noErr != hx_code_profile_start_error ) \
{ \
    Str255 hx_code_profile_errorString; \
    ::sprintf( ( char* ) hx_code_profile_errorString, "ProfilerInit() error: %d", hx_code_profile_start_error ); /* Flawfinder: ignore */ \
    ::c2pstr( ( char* ) hx_code_profile_errorString ); \
    ::DebugStr( hx_code_profile_errorString ); \
} \
::ProfilerSetStatus( true );

#define HX_CODE_PROFILE_STOP( inFilename ) \
long hx_code_profile_checkNrFunctions; \
long hx_code_profile_checkStackSize; \
::ProfilerSetStatus( false ); \
::ProfilerGetDataSizes( &hx_code_profile_checkNrFunctions, &hx_code_profile_checkStackSize ); \
if ( hx_code_profile_checkNrFunctions >= hx_code_profile_numFunctions ) \
{ \
    Str255 hx_code_profile_errorString; \
    ::sprintf( ( char* ) hx_code_profile_errorString, "Num Functions: %d >= Allocated Functions: %d", hx_code_profile_checkNrFunctions, hx_code_profile_numFunctions ); /* Flawfinder: ignore */ \
    ::c2pstr( ( char* ) hx_code_profile_errorString ); \
    ::DebugStr( hx_code_profile_errorString ); \
} \
if ( hx_code_profile_checkStackSize >= hx_code_profile_stackSize ) \
{ \
    Str255 hx_code_profile_errorString; \
    ::sprintf( ( char* ) hx_code_profile_errorString, "Stack size: %d >= Allocated Stack size: %d", hx_code_profile_checkStackSize, hx_code_profile_stackSize ); /* Flawfinder: ignore */ \
    ::c2pstr( ( char* ) hx_code_profile_errorString ); \
    ::DebugStr( hx_code_profile_errorString ); \
} \
OSErr hx_code_profile_stop_error = ::ProfilerDump( "\p"inFilename ); \
if ( noErr != hx_code_profile_stop_error ) \
{ \
    Str255 hx_code_profile_errorString; \
    ::sprintf( ( char* ) hx_code_profile_errorString, "ProfilerInit() error: %d", hx_code_profile_stop_error ); /* Flawfinder: ignore */ \
    ::c2pstr( ( char* ) hx_code_profile_errorString ); \
    ::DebugStr( hx_code_profile_errorString ); \
} \
::ProfilerTerm();

#else

#define HX_CODE_PROFILE_INIT( inNumFunctions, inStackSize )
#define HX_CODE_PROFILE_START
#define HX_CODE_PROFILE_STOP( inFilename )

#endif

#else

#define HX_CODE_PROFILE_INIT( inNumFunctions, inStackSize )
#define HX_CODE_PROFILE_START
#define HX_CODE_PROFILE_STOP( inFilename )

#endif // _DEBUG

#else

#define HX_CODE_PROFILE_INIT( inNumFunctions, inStackSize )
#define HX_CODE_PROFILE_START
#define HX_CODE_PROFILE_STOP( inFilename )

#endif // ENABLE_ACTIVE_PROFILING

#endif // _HXPROFILE_H_
