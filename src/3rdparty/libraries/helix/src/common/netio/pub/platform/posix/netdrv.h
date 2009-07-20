/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: netdrv.h,v 1.22 2006/02/16 23:03:05 ping Exp $ 
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

#ifndef _NETDRV_H
#define _NETDRV_H

/*
 * This is the Posix implementation of the Helix network driver.  It is used
 * for all Unix platforms and Win32 platforms that use the select model (the
 * server).  It follows the standard BSD API pretty much as one would expect,
 * with the following exceptions:
 *
 *  - Sockets are always nonblocking.  hx_socket() and hx_accept() should set
 *    O_NONBLOCK, FIONBIO or equivalent if necessary.
 *
 *  - Sockets always reuse addresses and ports (SO_REUSEADDR or equivalent)
 *    if available.
 *
 *  - All functions should retry in case of interruption (EINTR).
 *
 *  - hx_connect() is always nonblocking.  The return code is -1 for failure,
 *    0 for immediate success, and 1 for pending.
 *
 *  - Name resoluton... TDB
 *
 *  - hx_inet_pton() must accept address strings that are not terminated (maybe?)
 */

#include "hxcom.h"
#include "nettypes.h"
#include "hxresult.h"

#define AF_INETAUTO 0   /* == HX_SOCK_FAMILY_INAUTO */

/* Socket states are used to generate proper event notifications */
#define HX_SOCK_STATE_CLOSED        0
#define HX_SOCK_STATE_NORMAL        1
#define HX_SOCK_STATE_CONNECTING    2
#define HX_SOCK_STATE_LISTENING     3
#define HX_SOCK_STATE_LINGERING     4

extern const HX_SOCK HX_SOCK_NONE;


HX_RESULT   hx_netdrv_open(IUnknown* pContext);
void        hx_netdrv_close(void);
HXBOOL        hx_netdrv_familyavail(int af);
HXBOOL        hx_netdrv_has_ipv6_name_resolution();

void        hx_map4to6(const sockaddr* psa, sockaddr_in6* psa6);
int         hx_map6to4(const sockaddr* psa, sockaddr_in*  psa4);
HXBOOL        hx_maskaddr4(struct in_addr* paddr, UINT32 bits);
HXBOOL        hx_maskaddr6(struct in6_addr* paddr, UINT32 bits);

int         hx_inet_aton(const char* cp, struct in_addr* inp);
char*       hx_inet_ntoa(struct in_addr in);

UINT32      hx_lastsockerr(void);

int         hx_getaddrinfo(const char* node, const char* serv,
                           const struct addrinfo* hints,
                           struct addrinfo** res /*out*/);
int         hx_getnameinfo(const sockaddr* sa, socklen_t salen,
                           char* node /*out*/, size_t nodelen,
                           char* serv /*out*/, size_t servlen,
                           int flags);
void        hx_freeaddrinfo(struct addrinfo* ai);
struct hostent* hx_gethostbyname(const char* host);

int         hx_socket(HX_SOCK* s, int af, int type, int proto);
int         hx_accept(HX_SOCK* s, HX_SOCK* snew,
                    sockaddr* psa, socklen_t salen);

int         hx_close(HX_SOCK* s);

int         hx_bind(HX_SOCK* s, const sockaddr* psa, socklen_t salen);
int         hx_connect(HX_SOCK* s, const sockaddr* psa, socklen_t salen);
int         hx_listen(HX_SOCK* s, UINT32 backlog);
int         hx_getsockaddr(HX_SOCK* s, sockaddr* psa, socklen_t salen);
int         hx_getpeeraddr(HX_SOCK* s, sockaddr* psa, socklen_t salen);

/* NB: no ioctls are made public */

int         hx_getsockopt(HX_SOCK* s, UINT32 name, UINT32* valp);
int         hx_setsockopt(HX_SOCK* s, UINT32 name, UINT32 val);

int         hx_setsockopt(HX_SOCK* s, UINT32 name, const void* data, UINT32 len);
//int         hx_getsockopt(HX_SOCK* s, UINT32 name, const void* data)


ssize_t     hx_readfrom(HX_SOCK* s, void* buf, size_t len,
                    sockaddr* psa, socklen_t salen, HXBOOL peek);
ssize_t     hx_writeto(HX_SOCK* s, const void* buf, size_t len,
                    sockaddr* psa, socklen_t salen);
ssize_t     hx_readv(HX_SOCK* s, UINT32 vlen, hx_iov* iov);
ssize_t     hx_readfromv(HX_SOCK* s, UINT32 vlen, hx_iov* iov,
                    sockaddr* psalen, socklen_t salen);
ssize_t     hx_writev(HX_SOCK* s, UINT32 vlen, hx_iov* iov);
ssize_t     hx_writetov(HX_SOCK* s, UINT32 vlen, hx_iov* iov,
                    const sockaddr* psa, socklen_t salen);

#endif /* ndef _NETDRV_H */
