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

#ifndef _HXNET_H
#define _HXNET_H

#include "hxcom.h"
#include "ihxpckts.h"
#include "ihxlist.h"
/*
 * This file defines the new Helix socket addressing API and synchronous
 * nonblocking networking API.  They are designed with the following goals:
 *
 *  - Align as closely as possible with the standard BSD socket API so that
 *    implementation on most platforms is clean and simple.
 *
 *  - Avoid allocating objects in one space and deleting them in another
 *    (eg. creation in the core and destruction in a plugin).  This is not
 *    strictly possible with COM interfaces in theory, but it can be done
 *    if some simple conventions are established and followed.
 *
 *  - Completely abstract socket addresses into a generic interface, and
 *    provide specific interfaces for each supported address family.
 *
 *  - Never under any circumstances expose or accept protocol specific
 *    binary data (eg. socket addresses) or platform specific values.
 *
 *  - Allow plugin implementation, if desired, by avoiding globals.
 */

/******************************************************************************
 *
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 * This file contains an EXPERIMENTAL API.  It is a work in progress and the
 * interfaces defined here ARE SUBJECT TO CHANGE.
 *
 ******************************************************************************/

/* XXXTDM: Don't know if this is undefined on any supported platforms. */
#if !defined(INET_ADDRSTRLEN)
#define INET_ADDRSTRLEN     16      /* "xxx.xxx.xxx.xxx\0" */
#endif

/* Some forward decls */
typedef _INTERFACE IHXBuffer IHXBuffer;
struct sockaddr;

// Max IPv4/IPv6 packet size (excluding IPv6 jumbograms)
#define MAX_IP_PACKET 0x10000

// Default read size for TCP sockets when MSS is unavailable
#define DEFAULT_TCP_READ_SIZE 1500


// Default read size for all other sockets when MSS is unavailable
#if defined SYMBIAN
#define DEFAULT_UDP_READ_SIZE 0xf000
#else
#define DEFAULT_UDP_READ_SIZE 0xffff
#endif


/******************************************************************************
 *
 * Socket Addressing
 *
 * A socket address may be created in one of three ways:
 *
 *  - Use IHXNetServices.  This will return a socket address of the specified
 *    type, initialized to a suitable empty value.
 *
 *  - Use the resolver.  The callback will return a vector of socket addresses
 *    that correspond to the given name (assuming the lookup succeeds).
 *
 *  - Use a socket.  This will return a socket address of the same type as
 *    the socket.
 *
 * It is highly recommended that applications use the resolver.  It is not
 * convenient for users to remember IP addresses, especially not for IPv6.
 *
 * The socket address interfaces attempt to provide all commonly used address
 * manipulations without exposing any binary data.  Addresses may be compared,
 * retrieved in string form, queried for their address class, and masked.
 *
 * IPv6 socket addressing is an extension of IPv4 socket addressing.  The IPv6
 * API (RFC 3493) specifies that "IPv4 mapped" IPv6 addresses are equivalent
 * to IPv4 addresses.  The IPv6 address "::ffff:192.168.0.1" is treated the
 * same as the IPv4 address "192.168.0.1", and will result in actual IPv4 data
 * when used to bind and connect sockets.  Therefore, no (easy) method is
 * provided to distinguish between the two representations.  Users should check
 * the IPv6 address class to determine whether it is IPv4 mapped.  Users may
 * determine if a string represents a valid IPv4 or IPv6 address by creating a
 * socket address of the desired type and testing whether SetAddr() succeeds.
 *
 * Note that the SetAddr() methods are implemented using native functions such
 * as inet_pton().  Further, RFC 3493 doesn't specify whether a NUL terminator
 * is required when parsing the address.  It is therefore recommended that the
 * strings passed to these functions are always NUL terminated.  Also note
 * that most or all native IPv6 compatible methods require all octets in IPv4
 * addresses be present (unlike BSD derived inet_aton() implementations).
 *
 ******************************************************************************/

/*
 * For IHXSockAddr and IHXNetServices
 *
 * HX_SOCK_FAMILY_INANY detects address family support at runtime.  It uses
 * IPv6 if available and IPv4 if not.  It may ONLY be used in these methods:
 *   IHXSocket::Init
 *   IHXNetServices::CreateSockAddr
 *
 * NB: these are distinct from the values used by the resolver.
 */
typedef enum
{
    HX_SOCK_FAMILY_NONE,        /* Invalid, should never be used */
    HX_SOCK_FAMILY_LOCAL,       /* Local/IPC/UNIX */
    HX_SOCK_FAMILY_IN4,         /* IPv4 */
    HX_SOCK_FAMILY_IN6,         /* IPv6 */
    HX_SOCK_FAMILY_INANY,       /* IPv6 with IPv4 fallback */
    HX_SOCK_FAMILY_LBOUND,      /* Hack for proxy, don't use in new code */
    HX_SOCK_FAMILY_CLOAK,
    HX_SOCK_FAMILY_LAST
} HXSockFamily;

typedef enum
{
    HX_SOCK_TYPE_NONE,          /* Invalid, should never be used */
    HX_SOCK_TYPE_RAW,           /* NB: May require admin privileges. */
    HX_SOCK_TYPE_UDP,           /* UDP */
    HX_SOCK_TYPE_TCP,           /* TCP */
    HX_SOCK_TYPE_MCAST,         /* Multicast (special case of UDP) */
    HX_SOCK_TYPE_SCTP,          /* SCTP (not yet implemented) */
    HX_SOCK_TYPE_DCCP,          /* DCCP (not yet implemented) */
    HX_SOCK_TYPE_LAST
} HXSockType;

#define IS_STREAM_TYPE(t) ((t) == HX_SOCK_TYPE_TCP) // SCTP also?

typedef enum
{
    HX_SOCK_PROTO_NONE,         /* Invalid, should never be used */
    HX_SOCK_PROTO_ANY,
    /* ... */
    HX_SOCK_PROTO_LAST
} HXSockProtocol;

/*
 * Maximum size of a numeric address when converted to a string
 * (including the terminating NUL) across non-local socket domains.
 */
#define HX_ADDRSTRLEN       HX_ADDRSTRLEN_IN6
#define HX_PORTSTRLEN       (5+1)

/*
 * This provides access to the implementation's socket address.
 * DO NOT use this in application code.
 */
DEFINE_GUID(IID_IHXSockAddrNative, 0x9a419062, 0xdb35, 0x11d8, 0xb8, 0x60, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef  INTERFACE
#define INTERFACE IHXSockAddrNative
DECLARE_INTERFACE_(IHXSockAddrNative, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD_(void,Get)                    (THIS_ sockaddr** ppsa,
                                                   size_t* psalen) PURE;
};

/*
 * Generic socket address.  Retrieve type and QI for specific information.
 */
DEFINE_GUID(IID_IHXSockAddr, 0xa954f190, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef  INTERFACE
#define INTERFACE IHXSockAddr
DECLARE_INTERFACE_(IHXSockAddr, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD_(HXSockFamily,GetFamily)      (void) PURE;
    STDMETHOD_(HXBOOL,IsEqual)                (IHXSockAddr* pOther) PURE;

    // Copy and Clone this address to another.  Note that Copy requires a
    // compatible address (eg. same family).
    STDMETHOD(Copy)                         (IHXSockAddr* pOther) PURE;
    STDMETHOD(Clone)                        (IHXSockAddr** ppNew) PURE;

    // The Addr methods are always available
    STDMETHOD(GetAddr)                      (THIS_ IHXBuffer** ppBuf) PURE;
    STDMETHOD(SetAddr)                      (THIS_ IHXBuffer* pBuf) PURE;
    STDMETHOD_(HXBOOL,IsEqualAddr)            (THIS_ IHXSockAddr* pOther) PURE;

    // All of the following methods are are only available for internet class
    // addresses (IPv4, IPv6).  Other address classes will assert and return
    // either HXR_NOTIMPL or a default value.

    STDMETHOD_(UINT16, GetPort)             (THIS) PURE;
    STDMETHOD(SetPort)                      (THIS_ UINT16 port) PURE;
    STDMETHOD_(HXBOOL,IsEqualPort)            (THIS_ IHXSockAddr* pOther) PURE;

    STDMETHOD(MaskAddr)                     (THIS_ UINT32 nBits) PURE;
    STDMETHOD_(HXBOOL,IsEqualNet)             (THIS_ IHXSockAddr* pOther,
                                                   UINT32 nBits) PURE;
};

/*
 * Local address classes.
 */
#define HX_ADDRSTRLEN_LOCAL sizeof(sockaddr_un.sun_path)

DEFINE_GUID(IID_IHXSockAddrLocal, 0xa9552beb, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXSockAddrLocal
DECLARE_INTERFACE_(IHXSockAddrLocal, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // Nothing yet...
};

/*
 * IPv4 address classes.
 *
 * XXXTDM: list applicable RFCs here.
 */
typedef enum
{
    HX_IN4_CLASS_NONE,          /* None of the below */
    HX_IN4_CLASS_ANY,           /* 0.0.0.0 */
    HX_IN4_CLASS_LOOPBACK,      /* 127.0.0.0/8 */
    HX_IN4_CLASS_MULTICAST,     /* 224.0.0.0/4 */
    HX_IN4_CLASS_BROADCAST,     /* 255.255.255.255 (or LAN broadcast?) */
    HX_IN4_CLASS_PRIVATE,       /* RFC 1918 spaces */
    HX_IN4_CLASS_LAST
} HXIN4AddrClass;

#define HX_ADDRSTRLEN_IN4   INET_ADDRSTRLEN
#define HX_PORTSTRLEN_IN4   (5+1)

DEFINE_GUID(IID_IHXSockAddrIN4, 0xa9552bec, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXSockAddrIN4
DECLARE_INTERFACE_(IHXSockAddrIN4, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD_(HXIN4AddrClass,GetAddrClass) (THIS) PURE;
};

/*
 * IPv6 address classes.
 *
 * RFC 2373 defines IPv6 address space.  Notable items:
 *
 *   There are no broadcast addresses in IPv6, their function being
 *   superseded by multicast addresses. {2.2}
 *
 * Example: ff02::1 is "link-scope all-hosts multicast", similar to the
 * IPv4 broadcast address 255.255.255.255.
 *
 *   IPv6 nodes that utilize [dynamic tunneling] are assigned special IPv6
 *   unicast addresses that carry an IPv4 address in the low-order 32-bits.
 *   This type of address is termed an "IPv4-compatible IPv6 address" {2.5.4}
 *
 *   A second type of IPv6 address which holds an embedded IPv4 address is
 *   also defined.  This address is used to represent the addresses of IPv4-
 *   only nodes (those that *do not* support IPv6) as IPv6 addresses.  This
 *   type of address is termed an "IPv4-mapped IPv6 address" {2.5.4}
 *
 * The latter, v4mapped, is typically used by the kernel when an IPv4
 * connection is made to an IPv6 listening socket on a dual-stack host.
 * In both cases, the embedded IPv4 address may be extracted with the
 * IHXSockAddrIN6::ExtractIN4Addr() method.
 */
typedef enum
{
    HX_IN6_CLASS_NONE,          /* None of the below */
    HX_IN6_CLASS_ANY,           /* :: (in6addr_any) */
    HX_IN6_CLASS_LOOPBACK,      /* ::1 (in6addr_loopback) */
    HX_IN6_CLASS_MULTICAST,     /* ff00::/8 */
    HX_IN6_CLASS_LINKLOCAL,     /* fe80::/10 */
    HX_IN6_CLASS_SITELOCAL,     /* fec0::/10 */
    HX_IN6_CLASS_UNICAST,       /* 2000::/3 */
    HX_IN6_CLASS_V4MAPPED,      /* ::ffff:xxxx:xxxx */
    HX_IN6_CLASS_V4COMPAT,      /* ::xxxx:xxxx */
    HX_IN6_CLASS_LAST
} HXIN6AddrClass;

/* There are a couple EUI-64 properties that may be interesting */

/* Do we want or need IN6_IS_ADDR_MC_xxx? */

#define HX_ADDRSTRLEN_IN6   INET6_ADDRSTRLEN
#define HX_PORTSTRLEN_IN6   (5+1)

DEFINE_GUID(IID_IHXSockAddrIN6, 0xa95566de, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXSockAddrIN6
DECLARE_INTERFACE_(IHXSockAddrIN6, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(GetFullAddr)                  (THIS_ IHXBuffer** ppBuf) PURE;
    STDMETHOD(GetFullAddrZ)                 (THIS_ IHXBuffer** ppBuf) PURE;
    STDMETHOD(ExtractIN4Addr)               (THIS_ IHXSockAddrIN4** ppAddr) PURE;
    STDMETHOD_(HXIN6AddrClass,GetAddrClass) (THIS) PURE;
    STDMETHOD_(UINT32,GetFlowInfo)          (THIS) PURE;
    STDMETHOD(SetFlowInfo)                  (THIS_ UINT32 uFlowInfo) PURE;
    STDMETHOD_(UINT32,GetScopeId)           (THIS) PURE;
    STDMETHOD(SetScopeId)                   (THIS_ UINT32 uScopeId) PURE;
};

/******************************************************************************
 *
 * Sockets
 *
 * Connection oriented sockets (eg. TCP) are connected with ConnectToOne() or
 * ConnectToAny().  The former is useful if you have a single address, perhaps
 * obtained from a config file or over the network.  The latter may be passed
 * the vector from the resolver directly.  It will attempt to connect to any of
 * the returned addresses, in turn, until one succeeds or all have been tried.
 *
 * It is most efficient to avoid fragmentation when using sockets.  There are
 * different methods of doing this, depending on the socket type and whether
 * reading or writing.  The best way to handle reads is to always use zero for
 * the read size.  The underlying implementation will take care of allocating
 * and reading one full packet from the kernel.  When writing, avoid writing
 * many small packets and use the vectored methods where possible.  Ideally,
 * writes should be no larger than the MTU to avoid fragmentation.  Note that
 * it is possible for the path MTU to change at any time, but this is a very
 * rare occurrence in practice.
 *
 * SelectEvents() tells the network code which events you are interested in
 * receiving.  A typical client socket will be interested in connect, close
 * and read events.  When an event is pending, the network code will call
 * EventPending() with an event mask indicating the event.  If more than
 * one event is pending, EventPending() will be called multiple times.  That
 * is, the event parameter will not indicate more than one event at a time.
 *
 * The Peek() and Read() family of functions allocate and return a buffer if
 * data is available.  On success, the buffer(s) will have a valid size.  The
 * caller is guaranteed to never receive a zero sized buffer.  On failure, the
 * buffer will be unset.  Failure does NOT necessarily indicate an underlying
 * socket error.  It may mean that no data can be read at the moment.  If the
 * caller wishes to proactively disconnect the socket intead of letting the
 * implementation decide which errors are fatal, it must examine the return
 * code to determine the severity of the error.  This is not recommended for
 * normal use.
 *
 * The Write() family of functions implement outbound data buffering.  The
 * implementation chooses an appropriate default outbound buffer size for a
 * typical client socket.  If the user expects to write large amounts of data
 * to the socket, it is highly recommended to select writes events (eg. if
 * sending a file of arbitrary but fixed size) or adjust the outbound buffer
 * size (eg. if sending a continuous stream of data).  A good rule of thumb
 * is to select a size that represents 30 to 60 seconds of data, depending on
 * memory constraints.  If the write succeeds, the entire data buffer has been
 * accepted for transmission. There is no concept of a partial write.
 * [XXX describe async write errors]

 * The Peek, Read and Write methods are equivalent to the corresponding
 * methods with default parameters.  That is,
 *   Peek(&pBuf) is equivalent to PeekFrom(&pBuf, &pAddr)
 *   Read(&pBuf) is equivalent to ReadFrom(&pBuf, &pAddr)
 *   Write(pBuf) is equivalent to WriteTo(pBuf, &pAddr)
 *   [XXX should also be true for s/g methods but that needs work]
 *
 * For all methods, the caller who passes in an array owns the array and is
 * responsible for its deletion.  The callee who receives the array may only
 * use it for the duration of the method execution.  If the array data needs
 * to be saved, it must be copied into an array that the callee owns.
 *
 * XXXTDM: If there is a possibility that we might have string or buffer
 *         socket options, we should use an IHXBuffer instead of a UINT32
 *         in the GetOption and SetOption methods.
 *
 ******************************************************************************/

/* For IHXSocket::SelectEvents() and IHXSocketResponse::EventPending() */
#define HX_SOCK_EVENT_NONE      0
#define HX_SOCK_EVENT_READ      (1<<0)
#define HX_SOCK_EVENT_WRITE     (1<<1)
#define HX_SOCK_EVENT_EXCEPT    (1<<2)
#define HX_SOCK_EVENT_ACCEPT    (1<<3)
#define HX_SOCK_EVENT_CONNECT   (1<<4)
#define HX_SOCK_EVENT_CLOSE     (1<<5)
#define HX_SOCK_EVENT_ERROR     (1<<6)
/* If another HX_SOCK_EVENT_xxx event is added,
   then make sure and update HX_SOCK_EVENT_LAST */
#define HX_SOCK_EVENT_LAST      HX_SOCK_EVENT_ERROR

#define HX_SOCK_EVENT_ALL       (~0U)

#define HX_SOCK_RCVBUF_MAX      (~0U)

typedef enum {
    HX_SOCKBUF_DEFAULT,         /* IHXBuffer */
    HX_SOCKBUF_TIMESTAMPED      /* IHXTimeStampedBuffer */
} HXSockBufType;

/* private/experimental; subject to change */
typedef enum {
    HX_SOCK_READBUF_SIZE_DEFAULT,    /* read once into default sized read buffer */
    HX_SOCK_READBUF_SIZE_COPY,       /* same as default but reallocates once size is known */
    HX_SOCK_READBUF_SIZE_PEEK        /* use facility such as FIONBIO to determine size of pending data in network buffers */
} HXSockReadBufAlloc;

/* This is the value for "don't linger" */
#define LINGER_OFF (~0U)

/* Socket options for IHXSocket::GetOption() and IHXSocket::SetOption() 
 * XXXTDM: Separate these like the real getsockopt/setsockopt? 
 *
 * NOTE: If any option is added to this enum, then it needs to be added to struct g_hxopttbl[]
 * in common/netio/platform/posix/netdrv.cpp as well to maintain the offsets 
 */
typedef enum {
    HX_SOCKOPT_NONE,

    /* SOL_SOCKET */
    HX_SOCKOPT_SOCKERR,
    HX_SOCKOPT_KEEPALIVE,
    HX_SOCKOPT_OOBINLINE,
    HX_SOCKOPT_RCVLOWAT,
    HX_SOCKOPT_SNDLOWAT,
    HX_SOCKOPT_RCVTIMEO,
    HX_SOCKOPT_SNDTIMEO,
    HX_SOCKOPT_REUSEADDR,
    HX_SOCKOPT_DONTROUTE,
    HX_SOCKOPT_BCAST,
    HX_SOCKOPT_SNDBUF,
    HX_SOCKOPT_RCVBUF,
    HX_SOCKOPT_LINGER,

    /* SOL_UNIX (none) */

    /* SOL_IP */
    HX_SOCKOPT_IN4_OPTIONS,
    HX_SOCKOPT_IN4_TOS,
    HX_SOCKOPT_IN4_TTL,
    HX_SOCKOPT_IN4_MULTICAST_TTL,
    HX_SOCKOPT_IN4_MULTICAST_LOOP,
    HX_SOCKOPT_IN4_ADD_MEMBERSHIP,
    HX_SOCKOPT_IN4_DROP_MEMBERSHIP,
    HX_SOCKOPT_IN4_MULTICAST_IF,

    /* SOL_UDP */
    HX_SOCKOPT_UDP_RCVBUF,

    /* SOL_TCP */
    HX_SOCKOPT_TCP_MAXSEG,
    HX_SOCKOPT_TCP_NODELAY,

    /* SOL_IPV6 */
    HX_SOCKOPT_IN6_V6ONLY,
    HX_SOCKOPT_IN6_UNICAST_HOPS,
    HX_SOCKOPT_IN6_MULTICAST_IF,
    HX_SOCKOPT_IN6_MULTICAST_HOPS,
    HX_SOCKOPT_IN6_MULTICAST_LOOP,
    HX_SOCKOPT_IN6_JOIN_GROUP,
    HX_SOCKOPT_IN6_LEAVE_GROUP,

    /* SOL_UDP6? */

    /* SOL_TCP6? */

    /* Other */
    HX_SOCKOPT_APP_IDLETIMEOUT,     /* read timeout in seconds */

    /* Helix only */
    HX_SOCKOPT_APP_BUFFER_TYPE,     /* IHXBuffer type returned from read functions */
    HX_SOCKOPT_APP_SNDBUF,          /* outbound app buffer size in bytes */
    HX_SOCKOPT_APP_RCVBUF,          /* inbound app buffer size in bytes */
    HX_SOCKOPT_APP_READBUF_ALLOC,   /* controls read buffer allocation and sizing behavior */
    HX_SOCKOPT_APP_READBUF_MAX,     /* max read buffer size in bytes (datagram and stream) */
    HX_SOCKOPT_APP_AGGLIMIT,        /* controls write aggregation for stream sockets */

    HX_SOCKOPT_LAST
} HXSockOpt;

// 831bc2f2-d236-11d8-9745-0002b3658720
DEFINE_GUID(IID_IHXSocketAccessControl, 0x831bc2f2, 0xd236, 0x11d8, 0x97, 0x45, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXSocketAccessControl
DECLARE_INTERFACE_(IHXSocketAccessControl, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(AddressCheck)         (THIS_ IHXSockAddr* pSource,
                                           IHXSockAddr* pDest) PURE;
};


DEFINE_GUID(IID_IHXSocketResponse, 0xa955da88, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXSocketResponse
DECLARE_INTERFACE_(IHXSocketResponse, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status) PURE;
};

DEFINE_GUID(IID_IHXSocket, 0xa955a090, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXSocket
DECLARE_INTERFACE_(IHXSocket, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /* Basic methods */

    STDMETHOD_(HXSockFamily,GetFamily)      (THIS) PURE;
    STDMETHOD_(HXSockType,GetType)          (THIS) PURE;
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS) PURE;

    STDMETHOD(SetResponse)          (THIS_ IHXSocketResponse* pResponse) PURE;
    STDMETHOD(SetAccessControl)     (THIS_ IHXSocketAccessControl* pControl) PURE;
    STDMETHOD(Init)                 (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p) PURE;
    STDMETHOD(CreateSockAddr)       (THIS_ IHXSockAddr** ppAddr) PURE;

    STDMETHOD(Bind)                 (THIS_ IHXSockAddr* pAddr) PURE;
    STDMETHOD(ConnectToOne)         (THIS_ IHXSockAddr* pAddr) PURE;
    STDMETHOD(ConnectToAny)         (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec) PURE;
    STDMETHOD(GetLocalAddr)         (THIS_ IHXSockAddr** ppAddr) PURE;
    STDMETHOD(GetPeerAddr)          (THIS_ IHXSockAddr** ppAddr) PURE;

    STDMETHOD(SelectEvents)         (THIS_ UINT32 uEventMask) PURE;
    STDMETHOD(Peek)                 (THIS_ IHXBuffer** ppBuf) PURE;
    STDMETHOD(Read)                 (THIS_ IHXBuffer** ppBuf) PURE;
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf) PURE;
    STDMETHOD(Close)                (THIS) PURE;

    STDMETHOD(Listen)               (THIS_ UINT32 uBackLog) PURE;
    STDMETHOD(Accept)               (THIS_ IHXSocket** ppNewSock,
                                           IHXSockAddr** ppSource) PURE;

    /* Advanced methods */

    STDMETHOD(GetOption)            (THIS_ HXSockOpt name, UINT32* pval) PURE;
    STDMETHOD(SetOption)            (THIS_ HXSockOpt name, UINT32 val) PURE;

    // Basic methods that support source and destination addresses
    STDMETHOD(PeekFrom)             (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr) PURE;
    STDMETHOD(ReadFrom)             (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr) PURE;
    STDMETHOD(WriteTo)              (THIS_ IHXBuffer* pBuf,
                                           IHXSockAddr* pAddr) PURE;

    // Methods that use scatter/gather (aka vectored methods)
    STDMETHOD(ReadV)                (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec) PURE;
    STDMETHOD(ReadFromV)            (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr** ppAddr) PURE;

    STDMETHOD(WriteV)               (THIS_ UINT32 nVecLen,
                                           IHXBuffer** ppBufVec) PURE;
    STDMETHOD(WriteToV)             (THIS_ UINT32 nVecLen,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr* pAddr) PURE;
};

// IGMPv3 options
typedef enum
{
    HX_MCAST_SOURCE_ADD,
    HX_MCAST_SOURCE_DROP,
    HX_MCAST_SOURCE_BLOCK,
    HX_MCAST_SOURCE_UNBLOCK
} HXMulticastSourceOption;

DEFINE_GUID(IID_IHXMulticastSocket, 0xa9561368, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXMulticastSocket
DECLARE_INTERFACE_(IHXMulticastSocket, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // NULL may be passed for pInterface to indicate "any"
    STDMETHOD(JoinGroup)            (THIS_ IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface) PURE;
    STDMETHOD(LeaveGroup)           (THIS_ IHXSockAddr* pGroupAddr,

                                           IHXSockAddr* pInterface) PURE;

    STDMETHOD(SetSourceOption)      (THIS_ HXMulticastSourceOption flag,
                                           IHXSockAddr* pSourceAddr,
                                           IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface) PURE;
};

DEFINE_GUID(IID_IHXListeningSocketResponse, 0xa95686b8, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXListeningSocketResponse
DECLARE_INTERFACE_(IHXListeningSocketResponse, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(OnConnection)         (THIS_ IHXSocket* pNewSock,
                                           IHXSockAddr* pSource) PURE;
    STDMETHOD(OnError)              (THIS_ HX_RESULT err) PURE;
};

DEFINE_GUID(IID_IHXListeningSocket, 0xa9564d38, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXListeningSocket
DECLARE_INTERFACE_(IHXListeningSocket, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD_(HXSockFamily,GetFamily)      (THIS) PURE;
    STDMETHOD_(HXSockType,GetType)          (THIS) PURE;
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS) PURE;

    STDMETHOD(Init)                 (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p,
                                           IHXListeningSocketResponse* pResponse) PURE;
    STDMETHOD(Listen)               (THIS_ IHXSockAddr* pAddr) PURE;
    STDMETHOD(Close)                (THIS) PURE;

    STDMETHOD(GetOption)            (THIS_ HXSockOpt name, UINT32* pval) PURE;
    STDMETHOD(SetOption)            (THIS_ HXSockOpt name, UINT32 val) PURE;
};

/******************************************************************************
 *
 * Resolver
 *
 * This is mostly a COM wrapper for getnameinfo() and getaddrinfo().
 *
 ******************************************************************************/

// Various definitions for IHXAddrInfo below.
// NB: these are distinct from the values used by the socket addressing.
#define HX_ADDRFLAGS_NUMERICHOST    AI_NUMERICHOST      /* Suppress DNS query */
#define HX_ADDRFLAGS_CANONNAME      AI_CANONNAME        /* Useful? */
#define HX_ADDRFLAGS_PASSIVE        AI_PASSIVE          /* Useful? */

#define HX_ADDRFAMILY_LOCAL         PF_LOCAL            /* aka PF_UNIX */
#define HX_ADDRFAMILY_IN4           PF_INET
#define HX_ADDRFAMILY_IN6           PF_INET6

#define HX_SOCKTYPE_RAW             SOCK_RAW
#define HX_SOCKTYPE_UDP             SOCK_DGRAM
#define HX_SOCKTYPE_TCP             SOCK_STREAM
#define HX_SOCKTYPE_SCTP            FIXME               /* XXX ??? */
#define HX_SOCKTYPE_DCCP            FIXME               /* XXX ??? */

// Flags for IHXResolver::GetNameInfo()
#define HX_NI_NOFQDN        NI_NOFQDN       /* Only return nodename portion for local hosts */
#define HX_NI_NUMERICHOST   NI_NUMERICHOST  /* Return numeric form of the host's address */
#define HX_NI_NAMEREQD      NI_NAMEREQD     /* Error if the host's name not in DNS */
#define HX_NI_NUMERICSERV   NI_NUMERICSERV  /* Return numeric form of the service (port #) */
#define HX_NI_DGRAM         NI_DGRAM        /* Service is a datagram service */


/*
 * The IHXAddrInfo interface is used by the resolver for hints.
 *
 * XXXTDM: do we need to define protocols here?
 */
DEFINE_GUID(IID_IHXAddrInfo, 0xa956c0ba, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXAddrInfo
DECLARE_INTERFACE_(IHXAddrInfo, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD_(UINT32,GetFlags)     (THIS) PURE;
    STDMETHOD(SetFlags)             (THIS_ UINT32 uFlags) PURE;
    STDMETHOD_(UINT32,GetFamily)    (THIS) PURE;
    STDMETHOD(SetFamily)            (THIS_ UINT32 uFamily) PURE;
    STDMETHOD_(UINT32,GetType)      (THIS) PURE;
    STDMETHOD(SetType)              (THIS_ UINT32 uType) PURE;
    STDMETHOD_(UINT32,GetProtocol)  (THIS) PURE;                    /* Useful? */
    STDMETHOD(SetProtocol)          (THIS_ UINT32 uProtocol) PURE;  /* Useful? */
};

/*
 * The resolver converts node (address) and service (port) names to objects
 * and IHXSockAddr objects back to node and service names.
 */
DEFINE_GUID(IID_IHXResolveResponse, 0xa9574904, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXResolveResponse
DECLARE_INTERFACE_(IHXResolveResponse, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(GetAddrInfoDone)      (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec) PURE;
    STDMETHOD(GetNameInfoDone)      (THIS_ HX_RESULT status, const char* pNode, const char* pService) PURE;
};


DEFINE_GUID(IID_IHXResolve, 0xa956fa80, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);
#undef INTERFACE
#define INTERFACE IHXResolve
DECLARE_INTERFACE_(IHXResolve, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(Init)                 (THIS_ IHXResolveResponse* pResponse) PURE;

    STDMETHOD(Close)                (THIS) PURE;

    STDMETHOD(GetAddrInfo)          (THIS_ const char* pNode, const char* pService,
                                           IHXAddrInfo* pHints) PURE;

    STDMETHOD(GetNameInfo)          (THIS_ IHXSockAddr* pAddr, UINT32 uFlags) PURE;

};

/******************************************************************************
 *
 * Net Services
 *
 ******************************************************************************/

DEFINE_GUID(IID_IHXNetServices, 0xa9578e5a, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x20);

#define CLSID_IHXNetServices IID_IHXNetServices

#undef INTERFACE
#define INTERFACE IHXNetServices
DECLARE_INTERFACE_(IHXNetServices, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(CreateResolver)       (THIS_ IHXResolve** ppResolver) PURE;
    STDMETHOD(CreateSockAddr)       (THIS_ HXSockFamily f,
                                           IHXSockAddr** ppAddr) PURE;
    STDMETHOD(CreateListeningSocket)(THIS_ IHXListeningSocket** ppSock) PURE;
    STDMETHOD(CreateSocket)         (THIS_ IHXSocket** ppSock) PURE;
};

DEFINE_GUID(IID_IHXNetServices2, 0xa9578e5a, 0x7c47, 0x11d8, 0x8b, 0xcb, 0x00,
            0x02, 0xb3, 0x65, 0x87, 0x21);

#undef INTERFACE
#define INTERFACE IHXNetServices2
DECLARE_INTERFACE_(IHXNetServices2, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     * IHXNetServices2 methods
     */
    STDMETHOD(Close)		    (THIS) PURE;
};

#ifdef HELIX_FEATURE_SECURE_SOCKET

// {FC6621C4-441F-4404-83AD-07E479B0BBCB}
DEFINE_GUID(IID_IHXCertificateManager, 
0xfc6621c4, 0x441f, 0x4404, 0x83, 0xad, 0x7, 0xe4, 0x79, 0xb0, 0xbb, 0xcb);

#undef INTERFACE
#define INTERFACE IHXCertificateManager
DECLARE_INTERFACE_(IHXCertificateManager, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /* Basic methods */
	STDMETHOD(Initialize)(THIS) PURE;
	STDMETHOD(VerifyCertificateChain)(THIS_ IHXList* pCerts, IHXBuffer*& pPublicKey) PURE;
	STDMETHOD(AddCARoot)(THIS_ IHXBuffer* pCert) PURE;
	STDMETHOD(AddCACRLChain)(THIS_ IHXList*) PURE;
};
#define CLSID_IHXCertificateManager IID_IHXCertificateManager

// {B71C9A70-2731-44e4-B225-2A7A41234361}
DEFINE_GUID(IID_IHXCertificateUser, 
0xb71c9a70, 0x2731, 0x44e4, 0xb2, 0x25, 0x2a, 0x7a, 0x41, 0x23, 0x43, 0x61);

#undef INTERFACE
#define INTERFACE IHXCertificateUser
DECLARE_INTERFACE_(IHXCertificateUser, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;
    STDMETHOD(AddClientKey)(THIS_ IHXBuffer* pKey) PURE;
    STDMETHOD(AddCertificate)(THIS_ IHXBuffer* pCert) PURE;
};

/******************************************************************************
 *
 * Secure Net Services
 *
 ******************************************************************************/
DEFINE_GUID(IID_IHXSecureSocket, 0x4711296b, 0x94fd, 0x4a28, 0xb4, 0x4d, 0x67, 0xe7, 0xb8, 0xfd, 0x3f, 0x7f);
#undef INTERFACE
#define INTERFACE IHXSecureSocket
DECLARE_INTERFACE_(IHXSecureSocket, IUnknown)
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /* Basic methods */
    
    STDMETHOD_(HX_RESULT,SetClientCertificate)(THIS_ IHXList* pCertChain, IHXBuffer* pPrvKey) PURE;
    STDMETHOD_(HX_RESULT,InitSSL)(THIS_ IHXCertificateManager* pContext) PURE;

    STDMETHOD_(HX_RESULT, SetSessionID)(THIS_ IHXBuffer* ) PURE;
    STDMETHOD_(HX_RESULT, GetSessionID)(THIS_ IHXBuffer**) PURE;
};

DEFINE_GUID(IID_IHXSecureNetServices, 0xed1a738c, 0x1a76, 0x4ab7, 0x8f, 0x40, 0xdd, 0xbd, 0xa8, 0xa, 0xf1, 0xbc);

#define CLSID_IHXSecureNetServices IID_IHXSecureNetServices

#undef INTERFACE
#define INTERFACE IHXSecureNetServices
DECLARE_INTERFACE_(IHXSecureNetServices, IUnknown)
{
    STDMETHOD(CreateSecureSocket)         (THIS_ IHXSecureSocket** ppSock) PURE;
};
#endif
// Define smart pointers
#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXSockAddrNative)
DEFINE_SMART_PTR(IHXSockAddr)
DEFINE_SMART_PTR(IHXSockAddrLocal)
DEFINE_SMART_PTR(IHXSockAddrIN4)
DEFINE_SMART_PTR(IHXSockAddrIN6)
DEFINE_SMART_PTR(IHXSocketAccessControl)
DEFINE_SMART_PTR(IHXSocketResponse)
DEFINE_SMART_PTR(IHXSocket)
DEFINE_SMART_PTR(IHXMulticastSocket)
DEFINE_SMART_PTR(IHXListeningSocketResponse)
DEFINE_SMART_PTR(IHXListeningSocket)
DEFINE_SMART_PTR(IHXAddrInfo)
DEFINE_SMART_PTR(IHXResolveResponse)
DEFINE_SMART_PTR(IHXResolve)
DEFINE_SMART_PTR(IHXNetServices)
DEFINE_SMART_PTR(IHXNetServices2)

#endif /* ndef _HXNET_H */
