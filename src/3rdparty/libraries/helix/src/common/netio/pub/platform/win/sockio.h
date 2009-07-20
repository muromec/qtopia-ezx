/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sockio.h,v 1.8 2007/07/06 20:44:00 jfinnecy Exp $
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

#ifndef _SOCKIO_H_
#define _SOCKIO_H_

class TCPIO;

#ifdef _WIN16
#include <memory.h>
#endif
#include "hlxclib/windows.h"

#include "hlxclib/fcntl.h"
#ifndef WIN32_PLATFORM_PSPC
#include "asyncio.h"
#endif

#include "hlxclib/sys/types.h"
#include "hlxclib/errno.h"

#include "hxassert.h"

#ifndef HX_IOV_MAX
struct iovec { void* iov_base; size_t iov_len; };
typedef struct iovec HX_IOVEC;
#define HX_IOV_MAX MSG_MAXIOVLEN
#endif

int inet_aton(register const char *cp, struct in_addr* addr);

class SocketIO
#ifndef WIN32_PLATFORM_PSPC
                : public AsyncIO
#endif
{
public:
    friend              class TCPIO;
    friend              class UDPIO;

                        SocketIO::SocketIO();
    virtual             SocketIO::~SocketIO();

    static INT32        create_address(struct sockaddr_in& addr, char* host,
                                       INT16 port);
    INT32               init(INT32 type, HXBOOL do_block=TRUE,
                                HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    INT32               init(INT32 type, UINT32 local_addr, INT16 port,
                             HXBOOL do_block=TRUE,
                                 HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    INT32               listen(INT32 backlog);
    INT32               connect(char* host, INT16 port);
    INT32               connect(sockaddr_in *addr);
    TCPIO*              accept(sockaddr_in *addr, INT32* addrlen);
    INT32               bind(sockaddr_in*);
    INT32               blocking();
    INT32               nonblocking();
    INT32               getsockname(sockaddr_in* addr, INT32* addr_len);
    INT32               close();
    INT32               read(void* buf, INT32 size);
    INT32               write(const void* buf, INT32 size);
    off_t               seek(off_t off, INT32 whence);
    INT16               port();
    INT32               disable();
    INT32               reuse_port(HXBOOL enable);
    INT32               reuse_addr(HXBOOL enable);
    INT32               error();
    INT32               flags();
    virtual SOCKET      socket();
    off_t               file_size();

    static const UINT32 MAX_HOSTNAME_LEN;
    static int  gethostname(char* name, int count);

protected:
                        SocketIO::SocketIO(SOCKET conn);

    INT32               err;
    SOCKET              sock;
    INT32               _flags;
};

inline
SocketIO::SocketIO()
{
    err = 0;
    _flags = O_RDWR;
    sock = INVALID_SOCKET;
}

inline
SocketIO::SocketIO(SOCKET conn)
{
    err = 0;
    _flags = O_RDWR;
    sock = conn;
}

inline INT32
SocketIO::error()
{
    return err;
}

inline INT32
SocketIO::flags()
{
    return _flags;
}

inline INT32
SocketIO::disable()
{
    sock = INVALID_SOCKET;
    return 0;
}

inline INT32
SocketIO::close()
{
    if (sock < 0)
    {
        return 0;
    }
    INT32 ret = ::closesocket(sock);
    sock = INVALID_SOCKET;
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline
SocketIO::~SocketIO()
{
    close();
}

inline INT32
SocketIO::blocking()
{
    u_long dont_block = 0;
    INT32 ret = ::ioctlsocket(sock, FIONBIO, &dont_block);
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline INT32
SocketIO::nonblocking()
{
    u_long dont_block = 1;
    INT32 ret = ::ioctlsocket(sock, FIONBIO, &dont_block);
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline INT32
SocketIO::getsockname(sockaddr_in* addr, INT32* addr_len)
{
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }

    int temp_addr = HX_SAFEINT(*addr_len);
    int ret = ::getsockname(sock,(sockaddr*) addr, &temp_addr);
    *addr_len = temp_addr;
    if (ret < 0)
        err = WSAGetLastError();

    return ret;
}

inline INT32
SocketIO::listen(INT32 backlog)
{
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }
    INT32 ret = ::listen(sock, HX_SAFEINT(backlog));
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline INT32
SocketIO::bind(sockaddr_in* addr)
{
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }
    INT32 ret = ::bind(sock, (sockaddr*)addr, sizeof *addr);
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline INT32
SocketIO::connect(sockaddr_in* addr)
{
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }
    INT32 ret = ::connect(sock, (sockaddr*)addr, sizeof *addr);
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline INT32
SocketIO::read(void * buf, INT32 len)
{
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }
    INT32 ret = ::recv(sock, (char *)buf, HX_SAFEINT(len), 0);
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline INT32
SocketIO::write(const void * buf, INT32 len)
{
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }
    INT32 ret = ::send(sock, (const char *)buf, HX_SAFEINT(len), 0);
    if (ret < 0)
        err = WSAGetLastError();

#if defined SOLARIS2_4
    if (err == EINVAL)     /* This is a hack to get solaris 2.4 to work */
    {
        err = 0;
        ret = 0;
    }
#endif
    return ret;
}

inline INT16
SocketIO::port()
{
    sockaddr_in addr;
    INT32 addr_len = sizeof addr;
    memset(&addr, 0, HX_SAFESIZE_T(addr_len));
    INT32 ret = getsockname(&addr, &addr_len);
    return (ret < 0) ? (INT16) ret : ntohs(addr.sin_port);
}

inline INT32
SocketIO::reuse_port(HXBOOL enable)
{
#if defined SO_REUSEPORT
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }
    INT32       ret;
    int         opt_val = 0;
    if (enable)
    {
        opt_val = 1;
    }
    ret = ::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char*) &opt_val,
                       sizeof (int));
    if (ret < 0)
    {
        err = WSAGetLastError();
    }
    return ret;
#else
    return 0;
#endif
}

inline INT32
SocketIO::reuse_addr(HXBOOL enable)
{
    if (sock < 0)
    {
        err = WSAEBADF;
        return -1;
    }
    INT32 ret;
    int   opt_val = 0;
    if (enable)
    {
        opt_val = 1;
    }
    ret = ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &opt_val,
                       sizeof (int));
    if (ret < 0)
    {
        err = WSAGetLastError();
    }
    return ret;
}

inline off_t
SocketIO::seek(off_t off, INT32 whence)
{
    ASSERT(FALSE);
    return -1;
}

inline int
SocketIO::gethostname(char* name, int count)
{
    int result;

    result = ::gethostname(name, count);
    if (result < 0)
    {
        return WSAGetLastError();
    }
    return 0;
}

inline off_t
SocketIO::file_size()
{
    ASSERT(0);
    return -1;
}

inline SOCKET
SocketIO::socket()
{
    return sock;
}

#endif /* _SOCKIO_H_ */
