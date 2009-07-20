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

#include "hxtypes.h"
#include "nettypes.h"
#include "hxnet.h" /* XXXTDM: This file should not use HX_stuff */
#include "netdrv.h"

#include "netbyte.h"
#include "hxassert.h"
#include "hxbuffer.h"
#include "hlxclib/sys/ioctl.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_UNIX)
#include <netdb.h>
#if defined(_SOLARIS)
#include <fcntl.h>
// FIONBIO is apparently a BSD-ism and Solaris has quarantined it
#define BSD_COMP
#endif
#include <sys/ioctl.h>
#define SOCK_LAST_ERROR() errno
#define closesocket close
#define ioctlsocket ioctl
typedef int sioctl_t;
#endif

#if defined(_WIN32)
#include "hxwinsocklib.h"
#define SOCK_LAST_ERROR() ::WSAGetLastError()
typedef unsigned long sioctl_t;
#endif

const HX_SOCK HX_SOCK_NONE = { INVALID_SOCKET, 0 };

struct hxoptentry
{
    int level;
    int name;
};

#if defined(_LINUX) && !defined(IP_MTU)
/* Someone forgot to put IP_MTU in <bits/in.h> */
#define IP_MTU 14
#endif

/* Some systems (Solaris, OpenBSD) don't support V6ONLY */
#if defined(IPV6_V6ONLY)
#define HAVE_IPV6_V6ONLY 1
#else
#define IPV6_V6ONLY 0   /* Dummy */
#endif

/* Some systems (Win32) don't support TCP_MAXSEG */
#if defined(TCP_MAXSEG)
#define HAVE_TCP_MAXSEG 1
#else
#define TCP_MAXSEG 0    /* Dummy */
#endif

static const int g_hxfamtbl[] =
{
    0, AF_LOCAL, AF_INET, AF_INET6, 0
};

static const int g_hxtyptbl[] =
{
    0, SOCK_RAW, SOCK_DGRAM, SOCK_STREAM, SOCK_DGRAM, 0, 0, 0
};

static const struct { int level; int name; } g_hxopttbl[] =
{
    { 0, 0 },                           /* HX_SOCKOPT_NONE */

    /* SOL_SOCKET */
    { SOL_SOCKET, SO_ERROR },           /* HX_SOCKOPT_SOCKERR */
    { SOL_SOCKET, SO_KEEPALIVE },       /* HX_SOCKOPT_KEEPALIVE */
    { SOL_SOCKET, SO_OOBINLINE },       /* HX_SOCKOPT_OOBINLINE */
    { SOL_SOCKET, SO_RCVLOWAT },        /* HX_SOCKOPT_RCVLOWAT */
    { SOL_SOCKET, SO_SNDLOWAT },        /* HX_SOCKOPT_SNDLOWAT */
    { SOL_SOCKET, SO_RCVTIMEO },        /* HX_SOCKOPT_RCVTIMEO */
    { SOL_SOCKET, SO_SNDTIMEO },        /* HX_SOCKOPT_SNDTIMEO */
    { SOL_SOCKET, SO_REUSEADDR },       /* HX_SOCKOPT_REUSEADDR */
    { SOL_SOCKET, SO_DONTROUTE },       /* HX_SOCKOPT_DONTROUTE */
    { SOL_SOCKET, SO_BROADCAST },       /* HX_SOCKOPT_BROADCAST */
    { SOL_SOCKET, SO_SNDBUF },          /* HX_SOCKOPT_SNDBUF */
    { SOL_SOCKET, SO_RCVBUF },          /* HX_SOCKOPT_RCVBUF */
    { SOL_SOCKET, SO_LINGER },          /* HX_SOCKOPT_LINGER */

    /* SOL_UNIX (none) */

    /* SOL_IP */
    { SOL_IP, IP_OPTIONS },             /* HX_SOCKOPT_IN4_OPTIONS */
    { SOL_IP, IP_TOS },                 /* HX_SOCKOPT_IN4_TOS */
    { SOL_IP, IP_TTL },                 /* HX_SOCKOPT_IN4_TTL */
    { SOL_IP, IP_MULTICAST_TTL },       /* HX_SOCKOPT_IN4_MULTICAST_TTL */
    { SOL_IP, IP_MULTICAST_LOOP },      /* HX_SOCKOPT_IN4_MULTICAST_LOOP */
    { SOL_IP, IP_ADD_MEMBERSHIP },      /* HX_SOCKOPT_IN4_ADD_MEMBERSHIP */
    { SOL_IP, IP_DROP_MEMBERSHIP },     /* HX_SOCKOPT_IN4_DROP_MEMBERSHIP */
    { SOL_IP, IP_MULTICAST_IF },        /* HX_SOCKOPT_IN4_MULTICAST_IF */

    /* SOL_UDP */
    { SOL_UDP, 0 },                     /* DUMMY OPTION - Added to match 
                                           the 'HX_SOCKOPT_UDP_RCVBUF' option
                                           in 'HXSockOpt' present in 'hxnet.h' */ 

    /* SOL_TCP */
    { SOL_TCP, TCP_MAXSEG },            /* HX_SOCKOPT_TCP_MAXSEG */
    { SOL_TCP, TCP_NODELAY },           /* HX_SOCKOPT_TCP_NODELAY */

    /* SOL_IPV6 */
    { SOL_IPV6, IPV6_V6ONLY },          /* HX_SOCKOPT_IN6_V6ONLY */
    { SOL_IPV6, IPV6_UNICAST_HOPS },    /* HX_SOCKOPT_IN6_UNICAST_HOPS */
    { SOL_IPV6, IPV6_MULTICAST_IF },    /* HX_SOCKOPT_IN6_MULTICAST_IF */
    { SOL_IPV6, IPV6_MULTICAST_HOPS },  /* HX_SOCKOPT_IN6_MULTICAST_HOPS */
    { SOL_IPV6, IPV6_MULTICAST_LOOP },  /* HX_SOCKOPT_IN6_MULTICAST_LOOP */
    { SOL_IPV6, IPV6_JOIN_GROUP },      /* HX_SOCKOPT_IN6_JOIN_GROUP */
    { SOL_IPV6, IPV6_LEAVE_GROUP },     /* HX_SOCKOPT_IN6_LEAVE_GROUP */

    /* SOL_UDP6? */

    /* SOL_TCP6? */

    /* Other...? */

    { 0, 0 }        /* HX_SOCKOPT_LAST */
};

/*** Platform Features ***/

/*
 * Set options common to all sockets:
 *   - Non-blocking I/O (FIONBIO)
 *   - Inline OOB/urgent data (SO_OOBINLINE)
 */
static void
setup_socket(sockobj_t s)
{
    sioctl_t tmp;

    tmp = 1;
#ifdef FIONBIO
    ::ioctlsocket(s, FIONBIO, &tmp);
#else
    ::fcntl(s, F_SETFL, O_NONBLOCK);
#endif
    tmp = 1;
    // NB: Win32 arg 3 is "const char*", everyone else uses "const void*"
    ::setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (const char*)&tmp, sizeof(tmp));
}

static HXBOOL
is_ipv6_avail(void)
{
    HXBOOL rc = TRUE;
    sockobj_t s = ::socket(AF_INET6, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
    {
        if (SOCK_LAST_ERROR() == SOCKERR_AFNOSUPPORT)
        {
            rc = FALSE;
        }
        // else we don't know for sure, so assume available
    }
    else
    {
        ::closesocket(s);
    }
    return rc;
}

HX_RESULT
hx_netdrv_open(IUnknown* pContext)
{
#if defined(_WIN32)
    return HXLoadWinSock(pContext);
#else
    return 0;
#endif
}

void
hx_netdrv_close(void)
{
#if defined(_WIN32)
    HXReleaseWinSock();
#endif
}

HXBOOL
hx_netdrv_has_ipv6_name_resolution(void)
{
#if defined(HELIX_CONFIG_SOCKLIB_EXPLICIT_LOAD)
    return HXWinSockLib::Lib().HasIPv6NameResolution();
#else
    return TRUE;
#endif
}

HXBOOL
hx_netdrv_familyavail(int af)
{
    HXBOOL rc = FALSE;
    switch (g_hxfamtbl[af])
    {
    case AF_INET:
        rc = TRUE;
        break;
    case AF_INET6:
        rc = is_ipv6_avail();
        break;
    default:
        HX_ASSERT(FALSE);
    }
    return rc;
}

int
hx_inet_aton(const char* cp, struct in_addr* inp)
{
    UINT32 addr = HXinet_addr(cp);
    if (addr == INADDR_NONE)
    {
        return 0;
    }
    inp->s_addr = addr;
    return 1;
}

char*
hx_inet_ntoa(struct in_addr in)
{
    return inet_ntoa(in);
}


UINT32
hx_lastsockerr(void)
{
    return (UINT32)SOCK_LAST_ERROR();
}

int
hx_getaddrinfo(const char* node, const char* serv,
               const struct addrinfo* hints,
               struct addrinfo** res /*out*/)
{
    return getaddrinfo(node, serv, hints, res);
}

int hx_getnameinfo(const sockaddr* sa, socklen_t salen,
                   char* node /*out*/, size_t nodelen,
                   char* serv /*out*/, size_t servlen,
                   int flags)
{
    return getnameinfo(sa, salen, node, nodelen, serv, servlen, flags);
}

void
hx_freeaddrinfo(struct addrinfo* ai)
{
    freeaddrinfo(ai);
}

hostent*
hx_gethostbyname(const char* name)
{
    return gethostbyname(name);
}

int
hx_socket(HX_SOCK* s, int af, int type, int proto)
{
    HX_ASSERT(s->sock == INVALID_SOCKET);
    // XXX need proto?
    s->sock = ::socket(g_hxfamtbl[af], g_hxtyptbl[type], 0);
    if (s->sock == INVALID_SOCKET)
    {
        return -1;
    }

#if defined(HELIX_FEATURE_SERVER) && defined (_SOLARIS)
    //XXXDPL - Sockets open on FDs less than 256 count against FOPEN_MAX on Solaris.
    //HACK: Reassign to a higher FD so that we can still log.
    if (s->sock < 256)
    {
        int temp_fd = s->sock;
        int dup_fd = fcntl(s->sock, F_DUPFD, 256);
        if (dup_fd != INVALID_SOCKET)
        {
            s->sock = dup_fd;
            close(temp_fd);
        }
    }
#endif

    s->state = HX_SOCK_STATE_CLOSED;
    setup_socket(s->sock);
    return 0;
}

int
hx_accept(HX_SOCK* s, HX_SOCK* snew, sockaddr* psa, socklen_t salen)
{
    int rc;
    do
    {
        rc = ::accept(s->sock, psa, &salen);
    }
    while (rc == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    if (rc == INVALID_SOCKET)
    {
        return -1;
    }

#if defined(HELIX_FEATURE_SERVER) && defined (_SOLARIS)
    //XXXDPL - Sockets open on FDs less than 256 count against FOPEN_MAX on Solaris.
    //HACK: Reassign to a higher FD so that we can still log.
    if (rc < 256)
    {
        int rc_temp = rc;
        int rc_dup = fcntl(rc, F_DUPFD, 256);
        if (rc_dup != INVALID_SOCKET)
        {
            rc = rc_dup;
            close(rc_temp);
        }
    }
#endif //defined _SOLARIS

    snew->sock = rc;
    snew->state = HX_SOCK_STATE_NORMAL;
    // Some implementations inherit socket attributes, some do not.
    // It doesn't hurt to reset them, so do it for all platforms.
    setup_socket(snew->sock);
    return 0;
}

int
hx_close(HX_SOCK* s)
{
    int rc;
    ::shutdown( s->sock, 0 );
    s->state = HX_SOCK_STATE_CLOSED;
    do
    {
        rc = ::closesocket(s->sock);
    }
    while (rc == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    s->sock = INVALID_SOCKET;
    return rc;
}

int
hx_bind(HX_SOCK* s, const sockaddr* psa, socklen_t salen)
{
    int rc;

    rc = ::bind(s->sock, psa, salen);
    if (rc == 0)
    {
        s->state = HX_SOCK_STATE_NORMAL;
    }
    return rc;
}

int
hx_connect(HX_SOCK* s, const sockaddr* psa, socklen_t salen)
{
    int rc;
    do
    {
        rc = ::connect(s->sock, psa, salen);
#if defined(_LINUX)
        /* Work around a bug in the Linux 2.6 kernel:
         * If an IPv6 connect fails due to a routing error the socket is left
         * in a bad state.  The next connect attempt immediately returns
         * ECONNABORTED and then the socket is fine.
         */
        if (rc == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_CONNABORTED)
        {
            rc = ::connect(s->sock, psa, salen);
        }
#endif
    }
    while (rc == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    if (rc == 0)
    {
        s->state = HX_SOCK_STATE_NORMAL;
        return 0;
    }
    if (SOCK_LAST_ERROR() != SOCKERR_INPROGRESS &&
        SOCK_LAST_ERROR() != SOCKERR_WOULDBLOCK)
    {
        return -1;
    }
    s->state = HX_SOCK_STATE_CONNECTING;
    return 1;
}

int
hx_listen(HX_SOCK* s, UINT32 backlog)
{
    int rc = ::listen(s->sock, backlog);
    if (rc == 0)
    {
        s->state = HX_SOCK_STATE_LISTENING;
    }
    return rc;
}

int
hx_getsockaddr(HX_SOCK* s, sockaddr* psa, socklen_t salen)
{
    return ::getsockname(s->sock, psa, &salen);
}

int
hx_getpeeraddr(HX_SOCK* s, sockaddr* psa, socklen_t salen)
{
    return ::getpeername(s->sock, psa, &salen);
}

int
hx_getsockopt(HX_SOCK* s, UINT32 name, UINT32* valp)
{
    int rc;
    int optlevel;
    int optname;
    char* vp;
    socklen_t vlen;
    linger ling;
    if (name >= HX_SOCKOPT_LAST)
    {
        return -1;
    }
 #if !defined(HAVE_IPV6_V6ONLY)
    if (name == (UINT32)HX_SOCKOPT_IN6_V6ONLY)
    {
        return -1;
    }
 #endif
#if !defined(HAVE_TCP_MAXSEG)
    if (name == (UINT32)HX_SOCKOPT_TCP_MAXSEG)
    {
        return -1;
    }
#endif
    optlevel = g_hxopttbl[name].level;
    optname = g_hxopttbl[name].name;
    vp = (char*)valp;
    vlen = sizeof(*valp);
    if (name == (UINT32)HX_SOCKOPT_LINGER)
    {
        vp = (char*)&ling;
        vlen = sizeof(ling);
    }
    rc = ::getsockopt(s->sock, optlevel, optname, vp, &vlen);
    if (rc == 0 && name == (UINT32)HX_SOCKOPT_LINGER)
    {
        // Linger should be disabled or zero in nonblocking sockets
        HX_ASSERT(ling.l_onoff == 0 || ling.l_linger == 0);
        *valp = (ling.l_onoff ? (UINT32)ling.l_linger : LINGER_OFF);
    }
    return rc;
}

int
hx_setsockopt(HX_SOCK* s, UINT32 name, const void* data, UINT32 len)
{
    HX_ASSERT(s);
    HX_ASSERT(data);

    if (name >= HX_SOCKOPT_LAST)
    {
        return -1;
    }
 #if !defined(HAVE_IPV6_V6ONLY)
    if (name == (UINT32)HX_SOCKOPT_IN6_V6ONLY)
    {
        return -1;
    }
 #endif
#if !defined(HAVE_TCP_MAXSEG)
    if (name == (UINT32)HX_SOCKOPT_TCP_MAXSEG)
    {
        return -1;
    }
#endif

    int optlevel = g_hxopttbl[name].level;
    int optname = g_hxopttbl[name].name;

    linger ling;
    if (name == (UINT32)HX_SOCKOPT_LINGER)
    {
        memset(&ling, 0, sizeof(ling));

        UINT32 val = *((int*)data);
        if (val != LINGER_OFF)
        {
            ling.l_onoff = 1;
            ling.l_linger = (int)val;
        }
        // Linger should be disabled or zero in nonblocking sockets
        HX_ASSERT(ling.l_onoff == 0 || ling.l_linger == 0);

        data = &ling;
        len = sizeof(ling);
    }

    // multicast ttl has to have a UCHAR data and len param for SOLARIS
    if (optname == IP_MULTICAST_TTL)
    {
#ifdef _SOLARIS
	unsigned char ttl = (unsigned char)*(int *)data;
#else
	UINT32 ttl = (UINT32)*(int *)data;
#endif
	return ::setsockopt(s->sock, optlevel, optname, (const char *)&ttl, sizeof(ttl));
    }

    return ::setsockopt(s->sock, optlevel, optname, (const char *)data, len);
}

int
hx_setsockopt(HX_SOCK* s, UINT32 name, UINT32 val)
{
    return hx_setsockopt(s, name, &val, sizeof(val));
}

ssize_t
hx_readfrom(HX_SOCK* s, void* buf, size_t len,
                sockaddr* psa, socklen_t salen, HXBOOL peek)
{
    ssize_t n;
    int f = peek ? MSG_PEEK : 0;
    socklen_t* psalen = psa ? &salen : (socklen_t*)NULL;
    do
    {
        n = ::recvfrom(s->sock, (char*)buf, len, f, psa, psalen);
    }
    while (n == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);

    /* WM50 socket library does not seem to set the remote socket address in recvfrom calls'
     * get that address explicitly.
     */
#ifdef WINCE
    if (n != INVALID_SOCKET && psa)
    {
	memset( psa, 0, salen );
	getpeername( s->sock, psa, psalen );
    }
#endif

    return n;
}

ssize_t
hx_writeto(HX_SOCK* s, const void* buf, size_t len,
                const sockaddr* psa, socklen_t salen)
{
    ssize_t n;
    do
    {
        n = ::sendto(s->sock, (char*)buf, len, 0, psa, salen);
    }
    while (n == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    return n;
}

ssize_t
hx_readv(HX_SOCK* s, UINT32 vlen, hx_iov* iov)
{
#ifdef _WIN32
    DWORD n = 0;
    if (WSARecv(s->sock, iov, vlen, &n, 0, NULL, NULL) != 0)
    {
        return -1;
    }

    return (ssize_t)n;
#else
    ssize_t n;
    do
    {
        /* XXX: use recvmsg here? */
        n = ::readv(s->sock, iov, vlen);
    }
    while (n == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    return n;
#endif
}

ssize_t
hx_readfromv(HX_SOCK* s, UINT32 vlen, hx_iov* iov,
                sockaddr* psa, socklen_t salen)
{
#ifdef _WIN32
    DWORD n = 0;
    socklen_t* psalen = psa ? &salen : (socklen_t*)NULL;
    if (WSARecvFrom(s->sock, iov, vlen, &n, 0, psa, psalen, NULL, NULL) != 0)
    {
        return -1;
    }
    return (ssize_t)n;
#else
    msghdr mh;
    ssize_t n;
    memset(&mh, 0, sizeof(mh));
    mh.msg_iov = iov;
    mh.msg_iovlen = vlen;
    do
    {
        n = ::recvmsg(s->sock, &mh, 0);
    }
    while (n == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    /* XXXTDM: verify recvmsg() fills in mh.msg_name */
    if (psa != NULL)
    {
        if (mh.msg_namelen != salen)
        {
            HX_ASSERT(FALSE);
            memset(psa, 0, salen);
            return n;
        }
        memcpy(psa, mh.msg_name, salen);
    }
    return n;
#endif
}

ssize_t
hx_writev(HX_SOCK* s, UINT32 vlen, hx_iov* iov)
{
#ifdef _WIN32
    DWORD n = 0;
    if (WSASend(s->sock, iov, vlen, &n, 0, NULL, NULL) != 0)
    {
        return -1;
    }

    return (ssize_t)n;
#else
    ssize_t n;
    do
    {
        n = ::writev(s->sock, iov, vlen);
    }
    while (n == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    return n;
#endif
}

ssize_t
hx_writetov(HX_SOCK* s, UINT32 vlen, hx_iov* iov,
                const sockaddr* psa, socklen_t salen)
{
#ifdef _WIN32
    DWORD n = 0;
    if (WSASendTo(s->sock, iov, vlen, &n, 0, psa, salen, NULL, NULL) != 0)
    {
        return -1;
    }

    return (ssize_t)n;
#else
    msghdr mh;
    ssize_t n;
    memset(&mh, 0, sizeof(mh));
    mh.msg_name = (char*)psa;
    mh.msg_namelen = salen;
    mh.msg_iov = iov;
    mh.msg_iovlen = vlen;
    do
    {
        n = ::sendmsg(s->sock, &mh, 0);
    }
    while (n == INVALID_SOCKET && SOCK_LAST_ERROR() == SOCKERR_INTR);
    return n;
#endif
}

