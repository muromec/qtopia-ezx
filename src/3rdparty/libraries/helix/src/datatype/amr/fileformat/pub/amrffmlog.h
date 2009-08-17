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

#ifndef AMRFFMLOG_H
#define AMRFFMLOG_H

// Include files
// Usually we don't include header files from
// other header files. However, the idea here is
// to produce a single header file on a per-module
// basis that can be used to include all the logging
// definitions necessary for that module.
#include "multilog.h"

//
// For debugging leaks in the AMR file format
//
#define MLOG_LEAK_COMPILED_OUT
#if defined(_DEBUG)
#define LEAK_TARGET_FILE      0
#define LEAK_TARGET_DEBUGGER  1
#else
#define LEAK_TARGET_FILE      0
#define LEAK_TARGET_DEBUGGER  0
#endif

// Create an inline MLOG_LEAK() function
#ifndef MLOG_LEAK_COMPILED_OUT
INIT_MULTILOG_GROUP_NO_COREDEBUG(LEAK,
                                 LEAK_TARGET_FILE,
                                 LEAK_TARGET_DEBUGGER,
                                 "amrff_leak.log");
#else
#define MLOG_LEAK MLOG_DoNothing
#endif

//
// For debugging miscellaneous issues in the AMR file format
//
#define MLOG_MISC_COMPILED_OUT
#if defined(_DEBUG)
#define MISC_TARGET_FILE      0
#define MISC_TARGET_DEBUGGER  1
#define MISC_TARGET_COREDEBUG 0
#else
#define MISC_TARGET_FILE      0
#define MISC_TARGET_DEBUGGER  0
#define MISC_TARGET_COREDEBUG 0
#endif

// Create an inline MLOG_MISC() function
#ifndef MLOG_MISC_COMPILED_OUT
INIT_MULTILOG_GROUP(MISC,
                    MISC_TARGET_FILE,
                    MISC_TARGET_DEBUGGER,
                    MISC_TARGET_COREDEBUG,
                    "amrff_misc.log",
                    DOL_GENERIC,
                    NULL);
#else
#define MLOG_MISC MLOG_DoNothing
#endif

#endif // #ifndef AMRFFMLOG_H
