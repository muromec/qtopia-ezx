/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fcntl.h,v 1.12 2008/01/18 09:17:27 vkathuria Exp $
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

#ifndef HLXSYS_FCNTL_H
#define HLXSYS_FCNTL_H

#if !defined(WIN32_PLATFORM_PSPC) && !defined(_OPENWAVE) && !defined(_BREW)
#include <fcntl.h>

#else
#if defined(_OPENWAVE)
#include "platform/openwave/hx_op_fs.h"

#define _O_RDONLY kOpFsFlagRdOnly
#define _O_WRONLY kOpFsFlagWrOnly
#define _O_RDWR   kOpFsFlagRdwr

#define _O_CREAT  kOpFsFlagCreate
#define _O_TRUNC  kOpFsFlagTrunc
#define _O_EXCL   kOpFsFlagExcl

// Make sure this doesn't interfere with any of above flags...and make
// sure the Openwave-specific code masks it out before calling the OpFs
// functions.
#define _O_APPEND 0x0008

#else
#define _O_RDONLY       0x0000
#define _O_WRONLY       0x0001
#define _O_RDWR         0x0002
#define _O_APPEND       0x0008

#define _O_CREAT        0x0100
#define _O_TRUNC        0x0200
#define _O_EXCL         0x0400

#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif

#ifndef O_RDWR
#define O_RDWR _O_RDWR
#endif

#ifndef O_WRONLY
#define O_WRONLY _O_WRONLY
#endif

#endif


#ifndef _O_BINARY
#define _O_BINARY 0
#endif /* _O_BINARY */

#ifndef O_CREAT
#define O_CREAT _O_CREAT
#endif /* O_CREAT */

#ifndef O_APPEND
#define O_APPEND _O_APPEND
#endif /* O_APPEND */

#ifndef O_TRUNC
#define O_TRUNC _O_TRUNC
#endif /* O_TRUNC */

#ifndef O_EXCL
#define O_EXCL _O_EXCL
#endif /* O_EXCL */

#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif /* O_BINARY */

#ifndef O_RDONLY
#define O_RDONLY (_O_RDONLY | _O_BINARY)
#endif /* O_RDONLY */

#ifndef O_WRONLY
#define O_WRONLY (_O_WRONLY | _O_BINARY)
#endif /* O_WRONLY */

#ifndef O_RDWR
#define O_RDWR (_O_RDWR | _O_BINARY)
#endif /* O_RDWR */
#endif /* defined(WIN32_PLATFORM_PSPC) */


#if defined(_UNIX) || (defined(_MACINTOSH) && defined(_MAC_MACHO))
#if !defined(O_BINARY)
#define O_BINARY    0
#endif        // O_BINARY
#endif        // _UNIX

// XXX HP O_ACCMODE has been defined(0x0003) in macfd.h on MacOSX
#if !defined(_MACINTOSH)
#ifndef O_ACCMODE
#define O_ACCMODE (_O_RDONLY | _O_WRONLY | _O_RDWR | _O_BINARY)
#endif /* O_ACCMODE */
#endif /* _MACINTOSH */

#endif /* HLXSYS_FCNTL_H */
