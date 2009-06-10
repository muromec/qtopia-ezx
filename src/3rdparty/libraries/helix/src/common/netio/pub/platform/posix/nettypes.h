/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: nettypes.h,v 1.33 2007/12/11 23:58:27 ping Exp $ 
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

#ifndef _NETTYPES_H
#define _NETTYPES_H

/*
 * This is the Posix implementation of the Helix network driver.  It is used
 * for all Unix platforms and Win32 platforms that use the select model (the
 * server).  It follows the standard BSD API pretty much as one would expect,
 * with the following exceptions:
 *
 *  - All socket addresses are sockaddr_storage.  The ss_family member implies
 *    the underlying type (sockaddr_un, sockaddr_in, sockaddr_in6) and length.
 *    If there is no IPv6 support on a particular platform, sockaddr_storage
 *    will most likely not be defined.  In this case, a generic definition is
 *    provided.
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
 *    0 for pending, and 1 for immediate success.
 *
 *  - Name resoluton... TDB
 *
 *  - hx_inet_pton() must accept address strings that are not terminated (maybe?)
 */

/*
 * Definitions for HX_SOCK, socklen_t, etc.
 */

#define HX_SOCK_ERR int

// Some definitions to unify the minor differences in various platforms.
//XXXTDM: move some of these includes into netdrv.cpp?
#if defined(_UNIX)

#include <sys/types.h>
#include <limits.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <errno.h>
#include <net/if.h>
typedef int sockobj_t;
typedef int sockerr_t;
#ifndef HX_IOV_MAX
typedef struct iovec HX_IOVEC;
#define HX_IOV_MAX IOV_MAX
#endif
#define INVALID_SOCKET          -1
#define SOCKET_ERROR            -1
#define SOCKERR_NONE            0

#define SOCKERR_INTR            EINTR
#define SOCKERR_BADF            EBADF
#define SOCKERR_ACCES           EACCES
#define SOCKERR_FAULT           EFAULT
#define SOCKERR_INVAL           EINVAL
#define SOCKERR_MFILE           EMFILE
#define SOCKERR_WOULDBLOCK      EWOULDBLOCK
#define SOCKERR_INPROGRESS      EINPROGRESS
#define SOCKERR_ALREADY         EALREADY
#define SOCKERR_NOTSOCK         ENOTSOCK
#define SOCKERR_DESTADDRREQ     EDESTADDRREQ
#define SOCKERR_MSGSIZE         EMSGSIZE
#define SOCKERR_PROTOTYPE       EPROTOTYPE
#define SOCKERR_NOPROTOOPT      ENOPROTOOPT
#define SOCKERR_PROTONOSUPPORT  EPROTONOSUPPORT
#define SOCKERR_SOCKTNOSUPPORT  ESOCKTNOSUPPORT
#define SOCKERR_OPNOTSUPP       EOPNOTSUPP
#define SOCKERR_PFNOSUPPORT     EPFNOSUPPORT
#define SOCKERR_AFNOSUPPORT     EAFNOSUPPORT
#define SOCKERR_ADDRINUSE       EADDRINUSE
#define SOCKERR_ADDRNOTAVAIL    EADDRNOTAVAIL
#define SOCKERR_NETDOWN         ENETDOWN
#define SOCKERR_NETUNREACH      ENETUNREACH
#define SOCKERR_NETRESET        ENETRESET
#define SOCKERR_CONNABORTED     ECONNABORTED
#define SOCKERR_CONNRESET       ECONNRESET
#define SOCKERR_NOBUFS          ENOBUFS
#define SOCKERR_ISCONN          EISCONN
#define SOCKERR_NOTCONN         ENOTCONN
#define SOCKERR_SHUTDOWN        ESHUTDOWN
#define SOCKERR_TOOMANYREFS     ETOOMANYREFS
#define SOCKERR_TIMEDOUT        ETIMEDOUT
#define SOCKERR_CONNREFUSED     ECONNREFUSED
#define SOCKERR_LOOP            ELOOP
#define SOCKERR_NAMETOOLONG     ENAMETOOLONG
#define SOCKERR_HOSTDOWN        EHOSTDOWN
#define SOCKERR_HOSTUNREACH     EHOSTUNREACH
#define SOCKERR_PIPE            EPIPE
#define SOCKERR_EOF             -1

#define RSLVERR_NODATA          EAI_NODATA
#define RSLVERR_NONAME          EAI_NONAME

struct hx_iov : public iovec
{
    size_t get_len(void)     { return iov_len; }
    void   set_len(size_t n) { iov_len = n; }
    void*  get_buf(void)     { return iov_base; }
    void   set_buf(void* p)  { iov_base = (char*)p; }
};

/*
 * Ensure that IOV_MAX is defined appropriately.
 * NB: We cap IOV_MAX at 16 because it is used for local arrays and
 *     some systems define it to be (INT_MAX-1).
 */
#if !defined(IOV_MAX)
#if defined(UIO_MAXIOV)
#define IOV_MAX UIO_MAXIOV
#elif defined(MAXIOV)
#define IOV_MAX MAXIOV
#endif
#endif
#if !defined(IOV_MAX)
#define IOV_MAX 16
#endif
#if (IOV_MAX > 16)
#undef IOV_MAX
#define IOV_MAX 16
#endif

#if defined(_LINUX) && defined(__GLIBC__)
#if (__GLIBC__ < 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 2)
/* GNU libc prior to 2.2.x needs a few workarounds */
#define ss_family __ss_family
#define MISSING_SOCKADDR_IN6_SCOPE_ID
typedef UINT32 in_addr_t;
typedef UINT16 in_port_t;
#endif
#endif
#ifdef _AIX
#define ss_family __ss_family
#endif

/* Define socklen_t for platforms that don't use it */
#if defined(_FREEBSD2) || defined(_FREEBSD3) || \
    (defined(_MAC_UNIX) && (MAC_OS_X_VERSION_MAX_ALLOWED < 1030)) || \
    defined(_SOLARIS26) || defined(_SOLARIS27)
typedef int socklen_t;
#endif

/* Define argument type for functions taking sockaddrs */
#if defined(_HPUX)
typedef int salen_t;
typedef void* saptr_t;
#else
typedef socklen_t salen_t;
typedef sockaddr* saptr_t;
#endif

#endif /* defined(_UNIX) */

#if defined(_WIN32)

#if defined(USE_WINSOCK1)
#error "New net API requires winsock2"
#endif

// This includes all the winsock2 stuff
#include "hlxclib/windows.h"

// Winsock 2.x does not support IPv4 compatibility (RFC 3493 3.7)
#define MISSING_DUALSOCKET

typedef SOCKET sockobj_t;
typedef int    sockerr_t;
typedef int    socklen_t;
typedef int    sa_family_t;
typedef int    ssize_t;
typedef UINT32 in_addr_t;
typedef UINT16 in_port_t;

//XXXTDM: how to implement local sockets in Win32?
struct sockaddr_un { sa_family_t sun_family; char sun_path[128]; };

#if !defined(IN6_ARE_ADDR_EQUAL)
#define IN6_ARE_ADDR_EQUAL(a,b) IN6_ADDR_EQUAL(a,b)
#endif

// The ifndef below is for compatibility with the old net implementation.
// When we remove the old files and reimplement the old net api in terms
// of this one, it can be removed if desired. -- TDM
#ifndef HX_IOV_MAX
struct iovec { void* iov_base; size_t iov_len; };
typedef struct iovec HX_IOVEC;
#if !defined(MSG_MAXIOVLEN)
#error "MSG_MAXIOVLEN not defined, this should be in <winsock2.h>"
#endif
#if (MSG_MAXIOVLEN > 16)
#undef MSG_MAXIOVLEN
#define MSG_MAXIOVLEN 16
#endif
#define HX_IOV_MAX MSG_MAXIOVLEN
#endif

struct hx_iov : public WSABUF
{
    size_t get_len(void)     { return len; }
    void   set_len(size_t n) { len = n; }
    void*  get_buf(void)     { return buf; }
    void   set_buf(void* p)  { buf = (char*)p; }
};

// INVALID_SOCKET defined in winsock.h
// SOCKET_ERROR defined in winsock.h
#define SOCKERR_NONE            0

#define SOCKERR_INTR            WSAEINTR
#define SOCKERR_BADF            WSAEBADF
#define SOCKERR_ACCES           WSAEACCES
#define SOCKERR_FAULT           WSAEFAULT
#define SOCKERR_INVAL           WSAEINVAL
#define SOCKERR_MFILE           WSAEMFILE
#define SOCKERR_WOULDBLOCK      WSAEWOULDBLOCK
#define SOCKERR_INPROGRESS      WSAEINPROGRESS
#define SOCKERR_ALREADY         WSAEALREADY
#define SOCKERR_NOTSOCK         WSAENOTSOCK
#define SOCKERR_DESTADDRREQ     WSAEDESTADDRREQ
#define SOCKERR_MSGSIZE         WSAEMSGSIZE
#define SOCKERR_PROTOTYPE       WSAEPROTOTYPE
#define SOCKERR_NOPROTOOPT      WSAENOPROTOOPT
#define SOCKERR_PROTONOSUPPORT  WSAEPROTONOSUPPORT
#define SOCKERR_SOCKTNOSUPPORT  WSAESOCKTNOSUPPORT
#define SOCKERR_OPNOTSUPP       WSAEOPNOTSUPP
#define SOCKERR_PFNOSUPPORT     WSAEPFNOSUPPORT
#define SOCKERR_AFNOSUPPORT     WSAEAFNOSUPPORT
#define SOCKERR_ADDRINUSE       WSAEADDRINUSE
#define SOCKERR_ADDRNOTAVAIL    WSAEADDRNOTAVAIL
#define SOCKERR_NETDOWN         WSAENETDOWN
#define SOCKERR_NETUNREACH      WSAENETUNREACH
#define SOCKERR_NETRESET        WSAENETRESET
#define SOCKERR_CONNABORTED     WSAECONNABORTED
#define SOCKERR_CONNRESET       WSAECONNRESET
#define SOCKERR_NOBUFS          WSAENOBUFS
#define SOCKERR_ISCONN          WSAEISCONN
#define SOCKERR_NOTCONN         WSAENOTCONN
#define SOCKERR_SHUTDOWN        WSAESHUTDOWN
#define SOCKERR_TOOMANYREFS     WSAETOOMANYREFS
#define SOCKERR_TIMEDOUT        WSAETIMEDOUT
#define SOCKERR_CONNREFUSED     WSAECONNREFUSED
#define SOCKERR_LOOP            WSAELOOP
#define SOCKERR_NAMETOOLONG     WSAENAMETOOLONG
#define SOCKERR_HOSTDOWN        WSAEHOSTDOWN
#define SOCKERR_HOSTUNREACH     WSAEHOSTUNREACH

#define SOCKERR_HOSTDOWN        WSAEHOSTDOWN
#define SOCKERR_NOTEMPTY        WSAENOTEMPTY
#define SOCKERR_PROCLIM		WSAEPROCLIM
#define SOCKERR_USERS           WSAEUSERS
#define SOCKERR_DQUOT           WSAEDQUOT
#define SOCKERR_STALE           WSAESTALE
#define SOCKERR_REMOTE          WSAEREMOTE
#define SOCKERR_SYSNOTREADY     WSASYSNOTREADY
#define SOCKERR_VERNOTSUPPORTED WSAVERNOTSUPPORTED
#define SOCKERR_NOTINITIALISED  WSANOTINITIALISED
#define SOCKERR_DISCON          WSAEDISCON
#define SOCKERR_NOMORE          WSAENOMORE
#define SOCKERR_CANCELLED       WSAECANCELLED
#define SOCKERR_INVALIDPROCTABLE    WSAEINVALIDPROCTABLE
#define SOCKERR_INVALIDPROVIDER     WSAEINVALIDPROVIDER
#define SOCKERR_PROVIDERFAILEDINIT  WSAEPROVIDERFAILEDINIT
#define SOCKERR_SYSCALLFAILURE      WSASYSCALLFAILURE
#define SOCKERR_SERVICE_NOT_FOUND   WSASERVICE_NOT_FOUND
#define SOCKERR_TYPE_NOT_FOUND      WSATYPE_NOT_FOUND
#define SOCKERR_E_NO_MORE           WSA_E_NO_MORE
#define SOCKERR_E_CANCELLED         WSA_E_CANCELLED
#define SOCKERR_REFUSED             WSAEREFUSED
#define SOCKERR_HOST_NOT_FOUND      WSAHOST_NOT_FOUND
#define SOCKERR_TRY_AGAIN           WSATRY_AGAIN
#define SOCKERR_NO_RECOVERY         WSANO_RECOVERY
#define SOCKERR_NO_DATA             WSANO_DATA
#define SOCKERR_EOF             0x7FFFFFFF

#define RSLVERR_NODATA          WSANO_DATA
#define RSLVERR_NONAME          WSAHOST_NOT_FOUND


int         inet_aton(const char* cp, struct in_addr* inp);
// inet_ntop and inet_pton are defined in MS Windows SDK for Vista
// Microsoft SDKs\Windows\v6.0\Include\ws2tcpip.h
#if (NTDDI_VERSION < NTDDI_LONGHORN)
const char* inet_ntop(int af, const void* src, char* dst, socklen_t len);
int         inet_pton(int af, const char* src, void* dst);
#endif 
#endif /* defined(_WIN32) */

// Some platforms have strange and/or outdated headers.
#ifndef INADDR_NONE
#define INADDR_NONE ((UINT32)0xFFFFFFFF)
#endif
#ifndef INADDR_ANY
#define INADDR_ANY  ((UINT32)0x00000000)
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif
#ifndef SOL_IP
#define SOL_IP IPPROTO_IP
#endif
#ifndef SOL_UDP
#define SOL_UDP IPPROTO_UDP
#endif
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
#ifndef SOL_IPV6
#define SOL_IPV6 IPPROTO_IPV6
#endif

// Handle RFC 2133 style sockopts for older systems
#if !defined(IPV6_JOIN_GROUP) && defined(IPV6_ADD_MEMBERSHIP)
#define IPV6_JOIN_GROUP IPV6_ADD_MEMBERSHIP
#endif
#if !defined(IPV6_LEAVE_GROUP) && defined(IPV6_DROP_MEMBERSHIP)
#define IPV6_LEAVE_GROUP IPV6_DROP_MEMBERSHIP
#endif

typedef struct
{
    sockobj_t       sock;       // Underlying kernel handle
    unsigned short  state;      // Used to synthesize events in Posix
} HX_SOCK;

#define HX_SOCK_VALID(s) (s.sock != INVALID_SOCKET)

#endif /* ndef _NETTYPES_H */
