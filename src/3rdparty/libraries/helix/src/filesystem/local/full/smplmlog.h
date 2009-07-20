/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: smplmlog.h,v 1.3 2004/07/09 18:40:36 hubbe Exp $
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

#ifndef SMPLMLOG_H
#define SMPLMLOG_H

// Include files
// Usually we don't include header files from
// other header files. However, the idea here is
// to produce a single header file on a per-module
// basis that can be used to include all the logging
// definitions necessary for that module.
#include "multilog.h"

//
// For debugging leaks in smplfsys
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
                                 "c:\\smplfsys.log");
#else
#define MLOG_LEAK(x,y) /**/
#endif

//
// For debugging general issues in smplfsys
//
#define MLOG_GEN_COMPILED_OUT
#if defined(_DEBUG)
#define GEN_TARGET_FILE      0
#define GEN_TARGET_DEBUGGER  1
#define GEN_TARGET_COREDEBUG 0
#else
#define GEN_TARGET_FILE      0
#define GEN_TARGET_DEBUGGER  0
#define GEN_TARGET_COREDEBUG 0
#endif

// Create an inline MLOG_GEN() function
#ifndef MLOG_GEN_COMPILED_OUT
INIT_MULTILOG_GROUP(GEN,
                    GEN_TARGET_FILE,
                    GEN_TARGET_DEBUGGER,
                    GEN_TARGET_COREDEBUG,
                    "c:\\smplfsys.log",
                    DOL_GENERIC,
                    NULL);
#else
#define MLOG_GEN if(0)
#endif

//
// For debugging progressive download issues
//
#define MLOG_PD_COMPILED_OUT
#if defined(_DEBUG)
#define PD_TARGET_FILE      0
#define PD_TARGET_DEBUGGER  1
#define PD_TARGET_COREDEBUG 0
#else
#define PD_TARGET_FILE      0
#define PD_TARGET_DEBUGGER  0
#define PD_TARGET_COREDEBUG 0
#endif

// Create an inline MLOG_PD() function
#ifndef MLOG_PD_COMPILED_OUT
INIT_MULTILOG_GROUP(PD,
                    PD_TARGET_FILE,
                    PD_TARGET_DEBUGGER,
                    PD_TARGET_COREDEBUG,
                    "c:\\smplfsys.log",
                    DOL_GENERIC,
                    NULL);
#else
#define MLOG_PD if(0)
#endif

#endif // #ifndef SMPLMLOG_H
