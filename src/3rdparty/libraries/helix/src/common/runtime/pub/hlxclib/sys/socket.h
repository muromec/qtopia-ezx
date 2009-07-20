/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: socket.h,v 1.13 2008/07/03 21:53:49 dcollins Exp $
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

#ifndef HLXSYS_SYS_SOCKET_H
#define HLXSYS_SYS_SOCKET_H

#if defined(_UNIX)

#if defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD)
#include <sys/types.h>
#elif defined(_LSB)
#include <arpa/inet.h>
#define IN6_ARE_ADDR_EQUAL(a,b) \
        ((((__const uint32_t *) (a))[0] == ((__const uint32_t *) (b))[0])     \
         && (((__const uint32_t *) (a))[1] == ((__const uint32_t *) (b))[1])  \
         && (((__const uint32_t *) (a))[2] == ((__const uint32_t *) (b))[2])  \
         && (((__const uint32_t *) (a))[3] == ((__const uint32_t *) (b))[3]))
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#elif defined(_SYMBIAN)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#elif defined(_MACINTOSH)
#include "platform/mac/macsockets.h" //for sockaddr_in
#elif defined(_OPENWAVE)
#include "platform/openwave/socketdefs.h"
#elif _BREW
#include "platform/brew/socketdefs.h"
#elif defined(_WIN32)

#include "hxtypes.h"
#if defined(_WINSOCKAPI_)
#error "winsock cannot be included prior to hlxclib/sys/socket.h"
#endif
#if defined(USE_WINSOCK1)
#include <winsock.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#endif /* defined(_UNIX) */

#if !defined(INADDR_NONE)
#define INADDR_NONE (UINT32)(0xffffffff)
#endif

#endif /* HLXSYS_SYS_SOCKET_H */
