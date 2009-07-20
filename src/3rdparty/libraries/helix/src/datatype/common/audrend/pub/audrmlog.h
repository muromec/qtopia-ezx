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

#ifndef AUDRMLOG_H
#define AUDRMLOG_H

// Include files
// Usually we don't include header files from
// other header files. However, the idea here is
// to produce a single header file on a per-module
// basis that can be used to include all the logging
// definitions necessary for that module.
#include "multilog.h"

//
// For debugging issues in the base audio renderer
//
#if defined(HELIX_CONFIG_MINIMIZE_SIZE)
#define MLOG_MISC_COMPILED_OUT
#endif /* #if defined(HELIX_CONFIG_MINIMIZE_SIZE) */
#if defined(_DEBUG)
#define MISC_TARGET_FILE      0
#define MISC_TARGET_DEBUGGER  1
#define MISC_TARGET_COREDEBUG 1
#else
#define MISC_TARGET_FILE      0
#define MISC_TARGET_DEBUGGER  0
#define MISC_TARGET_COREDEBUG 1
#endif

// Create an inline MLOG_TIMING() function
#ifndef MLOG_MISC_COMPILED_OUT
INIT_MULTILOG_GROUP(MISC,
                    MISC_TARGET_FILE,
                    MISC_TARGET_DEBUGGER,
                    MISC_TARGET_COREDEBUG,
                    "audrend_misc.log",
                    DOL_REALAUDIO,
                    NULL);
#else
#define MLOG_MISC MLOG_DoNothing
#endif

//
// For debugging issues in the base audio renderer
//
#if defined(HELIX_CONFIG_MINIMIZE_SIZE)
#define MLOG_MISCEX_COMPILED_OUT
#endif /* #if defined(HELIX_CONFIG_MINIMIZE_SIZE) */
#if defined(_DEBUG)
#define MISCEX_TARGET_FILE      0
#define MISCEX_TARGET_DEBUGGER  1
#define MISCEX_TARGET_COREDEBUG 1
#else
#define MISCEX_TARGET_FILE      0
#define MISCEX_TARGET_DEBUGGER  0
#define MISCEX_TARGET_COREDEBUG 1
#endif

// Create an inline MLOG_TIMING() function
#ifndef MLOG_MISCEX_COMPILED_OUT
INIT_MULTILOG_GROUP(MISCEX,
                    MISCEX_TARGET_FILE,
                    MISCEX_TARGET_DEBUGGER,
                    MISCEX_TARGET_COREDEBUG,
                    "audrend_misc.log",
                    DOL_REALAUDIO_EXTENDED,
                    NULL);
#else
#define MLOG_MISCEX MLOG_DoNothing
#endif

#endif // #ifndef AUDRMLOG_H
