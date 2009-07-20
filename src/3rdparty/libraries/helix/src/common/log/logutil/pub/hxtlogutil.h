/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef HXTLOGUTIL_H
#define HXTLOGUTIL_H

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxtlogsystem.h"
#include "ihxtlogsystemcontext.h"

#if defined(HELIX_CONFIG_LOGGING_USE_FPRINTF)

// HELIX_CONFIG_LOGGING_USE_FPRINTF is used when we
// want all of the HXLOGLx() macros to simply 
// map to fprintf(<filename>,...). When this config
// is defined, we lose the ability to filter on
// functional area, although we retain the 
// ability to compile out by level. Optionally,
// HELIX_CONFIG_LOGGING_USE_FPRINTF_FILENAME can
// be defined which will provide the filename
// to write to. If HELIX_CONFIG_LOGGING_USE_FPRINTF_FILENAME
// is not defined, then we will write to stdout.

// Define null functions for when we compile out by logging level
inline void HXLogFprintfNull(EHXTLogFuncArea nFuncArea, const char* szMsg, ...) {}
inline void HXLogFprintfNull(const char* szMsg, ...) {}

// This is the function in hxtlogutil.cpp which wraps fprintf()
void HXLogFprintf(EHXTLogFuncArea nFuncArea, const char* szMsg, ...);
void HXLogFprintf(const char* szMsg, ...);

// In this configuration, we never need a 
// HX_ENABLE_LOGGING(), so it compiles out
#define HX_ENABLE_LOGGING(a)

// Set the defaults for HXLOGLx if no
// HELIX_FEATURE_LOGLEVEL_xxx is defined.
#if !defined(HELIX_FEATURE_LOGLEVEL_NONE) && !defined(HELIX_FEATURE_LOGLEVEL_1) && !defined(HELIX_FEATURE_LOGLEVEL_2) && !defined(HELIX_FEATURE_LOGLEVEL_3) && !defined(HELIX_FEATURE_LOGLEVEL_4) && !defined(HELIX_FEATURE_LOGLEVEL_ALL)
#if defined(_DEBUG)
#define HELIX_FEATURE_LOGLEVEL_ALL
#define HELIX_FEATURE_LOGLEVEL_4
#else
#define HELIX_FEATURE_LOGLEVEL_2
#endif

#endif /* #if !defined(HELIX_FEATURE_LOGLEVEL_NONE) && !defined(HELIX_FEATURE_LOGLEVEL_1) ... */

// For each HELIX_FEATURE_LOGLEVEL_xxx, compile in and out
// the appropriate logging level.
#if defined(HELIX_FEATURE_LOGLEVEL_NONE)
// All levels compiled out
#define HXLOGL1            HXLogFprintfNull
#define HXLOGL2            HXLogFprintfNull
#define HXLOGL3            HXLogFprintfNull
#define HXLOGL4            HXLogFprintfNull
#elif defined(HELIX_FEATURE_LOGLEVEL_1)
// Levels 2-4 compiled out
#define HXLOGL1            HXLogFprintf
#define HXLOGL2            HXLogFprintfNull
#define HXLOGL3            HXLogFprintfNull
#define HXLOGL4            HXLogFprintfNull
#elif defined(HELIX_FEATURE_LOGLEVEL_2)
// Levels 3-4 compiled out
#define HXLOGL1            HXLogFprintf
#define HXLOGL2            HXLogFprintf
#define HXLOGL3            HXLogFprintfNull
#define HXLOGL4            HXLogFprintfNull
#elif defined(HELIX_FEATURE_LOGLEVEL_3)
// Only level 4 compiled out
#define HXLOGL1            HXLogFprintf
#define HXLOGL2            HXLogFprintf
#define HXLOGL3            HXLogFprintf
#define HXLOGL4            HXLogFprintfNull
#elif defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
// No levels compiled out
#define HXLOGL1            HXLogFprintf
#define HXLOGL2            HXLogFprintf
#define HXLOGL3            HXLogFprintf
#define HXLOGL4            HXLogFprintf
#endif

#else /* #if defined(HELIX_CONFIG_LOGGING_USE_FPRINTF) */

// Support for 64-bit arguments varies.
//
#if defined(_SYMBIAN)
#if defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY) || defined (_SYMBIAN_81_)
#define HX_I64d_ARG(x) INT64_TO_UINT32(x >>32), INT64_TO_UINT32(x)
#define HX_I64d_SUBST "%p%X"
#else
#define HX_I64d_ARG(x) x.High(), x.Low()
#define HX_I64d_SUBST "%lu:%lu"
#endif
#else
#define HX_I64d_ARG(x) (x)
#define HX_I64d_SUBST "%I64d"
#endif

typedef HX_RESULT (HXEXPORT_PTR FPCREATELOGSYSTEMINTERFACE)(IHXTLogSystem**);

// These functions will attempt to create a DLLAccess
// class and therefore you must link against DLLAccess
// in common_system before they will work
#if defined(HELIX_FEATURE_LOG_STATICDLLACCESS)

void RSLog(const char* szMsg, ...);
void RSLog(UINT32 nMsg, const char* szMsg, ...);
void RSLog(EHXTLogCode nLogCode, UINT32 nMsg, const char* szMsg, ...);
void RSLog(EHXTLogCode nLogCode, EHXTLogFuncArea nFuncArea, UINT32 nMsg, const char* szMsg, ...);
void RSLog(EHXTLogCode nLogCode, const char* szMsg, ...);
void RSLog(EHXTLogCode nLogCode, EHXTLogFuncArea nFuncArea, const char* szMsg, ...);

void HXTLog_Push_Context(const char* szContext);
void HXTLog_Pop_Context();
void HXTLog_Set_FileAndLine(const char * szFilename, int nLineNum);
void HXTLog_SetThreadJobName(const char* szJobName, UINT32 nThreadId);
void HXTLog_CreateChildThread(UINT32 nParentThreadId, UINT32 nChildThreadId);
void HXTLog_EndThread(UINT32 nThreadId);

// Define producer-related macros
#if defined(COMPILE_OUT_RSLOG)

#define HXTLOG             if(0)
#define HXTLOG_APPROVED    if(0)
#define RSLOG_PUSH_CONTEXT if(0)
#define RSLOG_POP_CONTEXT  if(0)

#else /* #if defined(COMPILE_OUT_RSLOG) */

#define HXTLOG                    HXTLog_Set_FileAndLine(__FILE__, __LINE__), RSLog
#define HXTLOG_APPROVED           HXTLog_Set_FileAndLine(__FILE__, __LINE__), RSLog
#define RSLOG_PUSH_CONTEXT        HXTLog_Push_Context
#define RSLOG_POP_CONTEXT         HXTLog_Pop_Context
#define RSLOG_SET_THREAD_JOB_NAME HXTLog_SetThreadJobName
#define RSLOG_CREATE_CHILD_THREAD HXTLog_CreateChildThread
#define RSLOG_END_THREAD          HXTLog_EndThread

#endif /* #if defined(COMPILE_OUT_RSLOG) */

// logging for debug builds only
#if defined(_DEBUG)
#define HXTLOG_DEBUG HXTLOG  // set to whatever HXTLOG is currently defined as
#else /* #if defined(_DEBUG) */
#define HXTLOG_DEBUG if(0)   // compile out
#endif /* #if defined(_DEBUG) */

#endif /* #if defined(HELIX_FEATURE_LOG_STATICDLLACCESS) */

#include "hxdllaccess.h"
#include "hxccf.h"

HXBOOL HXLoggingEnabled();
// These functions require HXEnableLogging() to be called
// first before they will work
void HXEnableLogging(IUnknown* pContext);
void HXEnableLogging_New(IUnknown* pContext);
void HXDisableLogging(HXBOOL bShutdown = TRUE);
void HXLog_Push_Context(const char* szContext);
void HXLog_Pop_Context();
void HXLog_Set_FileAndLine(const char * szFilename, int nLineNum);
void HXLog1(const char* szMsg, ...);
void HXLog2(const char* szMsg, ...);
void HXLog3(const char* szMsg, ...);
void HXLog4(const char* szMsg, ...);
void HXLog1(EHXTLogFuncArea nFuncArea, const char* szMsg, ...);
void HXLog2(EHXTLogFuncArea nFuncArea, const char* szMsg, ...);
void HXLog3(EHXTLogFuncArea nFuncArea, const char* szMsg, ...);
void HXLog4(EHXTLogFuncArea nFuncArea, const char* szMsg, ...);


// Define versions that compile out
#if defined(__GNUC__)
#define HXEnableLoggingNull(a)
#define HXDisableLoggingNull(a)
#define HXLogNull(a, args...)
#define HXLog_Push_ContextNull(a)
#define HXLog_Pop_ContextNull()
#else
inline void HXEnableLoggingNull(IUnknown* pContext) {}
inline void HXDisableLoggingNull(HXBOOL bShutdown = TRUE) {}
inline void HXLogNull(const char* szMsg, ...) {}
inline void HXLogNull(EHXTLogFuncArea nFuncArea, const char* szMsg, ...) {}
inline void HXLog_Push_ContextNull(const char* szContext) {}
inline void HXLog_Pop_ContextNull() {}
#endif

// Is any HELIX_FEATURE_LOGLEVEL_xxx defined?
#if !defined(HELIX_FEATURE_LOGLEVEL_NONE) && !defined(HELIX_FEATURE_LOGLEVEL_1) && !defined(HELIX_FEATURE_LOGLEVEL_2) && !defined(HELIX_FEATURE_LOGLEVEL_3) && !defined(HELIX_FEATURE_LOGLEVEL_4) && !defined(HELIX_FEATURE_LOGLEVEL_ALL)
// No HELIX_FEATURE_LOGLEVEL_xxx defined, so
// if we are release we will define level 2
// and if we are debug we will define all levels
#  if defined(_DEBUG)
#    define HELIX_FEATURE_LOGLEVEL_ALL
#    define HELIX_FEATURE_LOGLEVEL_4
#  else
#    define HELIX_FEATURE_LOGLEVEL_2
#  endif
#endif

// Define client-related macros
#if defined(HELIX_FEATURE_LOGLEVEL_NONE)
// All levels compiled out
#define HXLOGL1            HXLogNull
#define HXLOGL2            HXLogNull
#define HXLOGL3            HXLogNull
#define HXLOGL4            HXLogNull
#define HX_ENABLE_LOGGING  HXEnableLoggingNull
#define HX_DISABLE_LOGGING HXDisableLoggingNull
#define HXLOG_PUSH_CONTEXT HXLog_Push_ContextNull
#define HXLOG_POP_CONTEXT  HXLog_Pop_ContextNull
#else
// These compile in for all levels
#define HX_ENABLE_LOGGING  HXEnableLogging
#define HX_DISABLE_LOGGING HXDisableLogging
#define HXLOG_PUSH_CONTEXT HXLog_Push_Context
#define HXLOG_POP_CONTEXT  HXLog_Pop_Context
#if defined(HELIX_FEATURE_LOGLEVEL_1)
// Levels 2-4 compiled out
#define HXLOGL1            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog1
#define HXLOGL2            HXLogNull
#define HXLOGL3            HXLogNull
#define HXLOGL4            HXLogNull
#elif defined(HELIX_FEATURE_LOGLEVEL_2)
// Levels 3-4 compiled out
#define HXLOGL1            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog1
#define HXLOGL2            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog2
#define HXLOGL3            HXLogNull
#define HXLOGL4            HXLogNull
#elif defined(HELIX_FEATURE_LOGLEVEL_3)
// Only level 4 compiled out
#define HXLOGL1            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog1
#define HXLOGL2            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog2
#define HXLOGL3            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog3
#define HXLOGL4            HXLogNull
#elif defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
// No levels compiled out
#define HXLOGL1            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog1
#define HXLOGL2            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog2
#define HXLOGL3            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog3
#define HXLOGL4            HXLog_Set_FileAndLine(__FILE__, __LINE__), HXLog4
#endif

#endif // !HELIX_FEATURE_LOGLEVEL_NONE

#if defined(_STATICALLY_LINKED)
#define HXLOG_DLLSUFFIX ""
#elif defined(WIN32) || defined(_WIN32) || defined(_SYMBIAN)
#define HXLOG_DLLSUFFIX  ".dll"
#elif defined(_MAC_UNIX)
#define HXLOG_DLLSUFFIX  ".bundle"
#elif _MACINTOSH
#if defined(_CARBON)
#ifdef _MAC_MACHO
#define HXLOG_DLLSUFFIX  ".bundle"
#else
#define HXLOG_DLLSUFFIX  ".shlb"
#endif
#else	// _CARBON
#define HXLOG_DLLSUFFIX  ".DLL"
#endif	// _CARBON
#elif defined (_UNIX)
#define HXLOG_DLLSUFFIX  ".so"
#else
#define HXLOG_DLLSUFFIX  "\0"
#endif

// This should be kept to the max DLL suffix length
#define HXLOG_MAXDLLSUFFIXLEN 7


static const char HXLOG_ENABLE_DELIVERY_THREAD_PREFKEY[] = "Logging\\DeliveryThread";
#if defined (_SYMBIAN)
// delivery thread is disabled by default on Symbian
const HXBOOL HXLOG_ENABLE_DELIVERY_THREAD_PREFVAL_DEFAULT = FALSE;
#else
const HXBOOL HXLOG_ENABLE_DELIVERY_THREAD_PREFVAL_DEFAULT = TRUE;
#endif

#endif /* #if defined(HELIX_CONFIG_LOGGING_USE_FPRINTF) */

#endif /* #ifndef HXTLOGUTIL_H */
