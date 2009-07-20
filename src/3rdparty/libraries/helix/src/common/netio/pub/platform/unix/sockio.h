/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sockio.h,v 1.13 2005/03/14 19:36:37 bobclark Exp $
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

#ifndef _SOCKIO_H_
#define _SOCKIO_H_

class TCPIO;

#ifdef _SOLARIS
#include <sys/systeminfo.h>
#include <sys/filio.h>
#endif /* _SOLARIS */

#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef _BEOS
#include <arpa/inet.h>
#endif

#ifndef _VXWORKS
#include <netdb.h>
#else
#include <ioLib.h>
#ifndef fcntl
#define fcntl ioctl
#endif
#include <sockLib.h>
#endif
#include <string.h>

#include "hxassert.h"
#include "hxstrutl.h"

#include "asyncio.h"
#include "hxsocket.h"

#if defined PAULM_SOCKTIMING
#include "sockettimer.h"

extern SocketTimer g_SocketTimer;
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

#include <sys/uio.h>
#include <unistd.h>

#ifndef HX_IOV_MAX
typedef struct iovec HX_IOVEC;
#define HX_IOV_MAX IOV_MAX
#endif

class SocketIO: public AsyncIO
{
public:
    friend              class TCPIO;
    friend              class UDPIO;

                        SocketIO();
    virtual             ~SocketIO();

    static INT32        create_address(struct sockaddr_in& addr, char* host,
                                       INT16 port);
    INT32               init(INT32 type, HXBOOL do_block=TRUE,
                               HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    INT32               init(INT32 type, UINT32 local_addr, INT16 port,
                             HXBOOL do_block=TRUE,
                             HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    virtual INT32       listen(INT32 backlog);
    virtual INT32       connect(char* host, INT16 port);
    virtual INT32       connect(sockaddr_in *addr);
    virtual TCPIO*      accept(sockaddr_in *addr, INT32* addrlen);
    virtual INT32       bind(sockaddr_in*);
    virtual INT32       blocking();
    virtual INT32       nonblocking();
    virtual INT32       getsockname(sockaddr_in* addr, INT32* addr_len);
    virtual INT32       close();
    virtual INT32       read(void* buf, INT32 size);
    virtual INT32       write(const void* buf, INT32 size);
    virtual off_t       seek(off_t off, INT32 whence);
    virtual INT16       port();
    virtual INT32       disable();
    virtual INT32       reuse_port(HXBOOL enable);
    virtual INT32       reuse_addr(HXBOOL enable);
    virtual INT32       error();
    virtual INT32       flags();
    virtual int         fd();
    virtual void        fd(int s);
#if defined PAULM_SOCKTIMING
    void                fd(int s, int c);
#endif
    virtual off_t       file_size();

    static const UINT32 MAX_HOSTNAME_LEN;
    static int          gethostname(char* name, int count);

protected:
                        SocketIO(int conn);

    INT32               err;
    int                 sock;
    INT32               _flags;
};

inline
SocketIO::SocketIO()
{
    _flags = O_RDWR;
    sock = -1;
    err = 0;
}

inline
SocketIO::SocketIO(int conn)
{
    _flags = O_RDWR;
    sock = conn;
    err = 0;
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

inline int
SocketIO::fd()
{
    return sock;
}

inline void
SocketIO::fd(int s)
{
#if defined PAULM_SOCKTIMING
    sockaddr_in a;
    int size = sizeof(a);
    if (!::getsockname(s, (sockaddr*)&a, &size))
    {
        g_SocketTimer.Add(s, ntohs(a.sin_port));
    }
#endif
    sock = s;
}

#if defined PAULM_SOCKTIMING
inline void
SocketIO::fd(int s, int c)
{
    sockaddr_in a;
    int size = sizeof(a);
    if (!::getsockname(s, (sockaddr*)&a, &size))
    {
        g_SocketTimer.Add(s, ntohs(a.sin_port), c);
    }
    sock  = s;
}
#endif

inline INT32
SocketIO::disable()
{
    sock = -2;
    return 0;
}

inline INT32
SocketIO::close()
{
    if (sock < 0)
    {
        return 0;
    }
#if defined PAULM_SOCKTIMING
    g_SocketTimer.Remove(sock);
#endif
    INT32 ret = ::close(sock);
    sock = -1;
    if (ret < 0)
        err = errno;
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
#ifdef FIONBIO
    INT32 dont_block = 0;
    INT32 ret = ::ioctl(sock, FIONBIO, (char*)&dont_block);
    if (ret < 0)
        err = errno;
#elif SO_NONBLOCK
    char dont_block=0;
    INT32 ret = ::setsockopt(sock,SOL_SOCKET,SO_NONBLOCK,&dont_block,1);
#else
    INT32 ret = ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) & ~O_NONBLOCK);
#endif
    return ret;
}

inline INT32
SocketIO::nonblocking()
{
#ifdef FIONBIO
    INT32 dont_block = 1;
    INT32 ret = ::ioctl(sock, FIONBIO, (char*)&dont_block);
    if (ret < 0)
        err = errno;
#elif SO_NONBLOCK
    char dont_block=1;
    INT32 ret = ::setsockopt(sock,SOL_SOCKET,SO_NONBLOCK,&dont_block,1);
#else
    INT32 ret = ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
#endif
    return ret;
}

inline INT32
SocketIO::getsockname(sockaddr_in* addr, INT32* addr_len)
{
    if (sock < 0)
    {
        err = EBADF;
        return -1;
    }

    HX_SOCKLEN_T temp_addr = *addr_len;
    int ret = ::getsockname(sock, (HX_SOCKADDR_T*)addr, (HX_SOCKLEN_T *)&temp_addr);
    *addr_len = temp_addr;
#ifdef _LINUX
    if (!ret && ((sockaddr_in*)addr)->sin_family == AF_UNIX)
    {
        ((sockaddr_in*)addr)->sin_addr.s_addr = inet_addr("127.0.0.1");
    }
#elif _SUN
    if (!ret && !((sockaddr_in*)addr)->sin_addr.s_addr)
    {
        ((sockaddr_in*)addr)->sin_addr.s_addr = inet_addr("127.0.0.1");
    }
#endif /* _LINUX */
    if (ret < 0)
        err = errno;

    return ret;
}

inline INT32
SocketIO::listen(INT32 backlog)
{
    if (sock < 0)
    {
        err = EBADF;
        return -1;
    }
    INT32 ret = ::listen(sock, backlog);
    if (ret < 0)
        err = errno;
    return ret;
}

inline INT32
SocketIO::bind(sockaddr_in* addr)
{
    if (sock < 0)
    {
        err = EBADF;
        return -1;
    }
    INT32 ret = ::bind(sock, (HX_SOCKADDR_T*)addr, sizeof *addr);
    if (ret < 0)
        err = errno;
    return ret;
}

inline INT32
SocketIO::connect(sockaddr_in* addr)
{
    if (sock < 0)
    {
        err = EBADF;
        return -1;
    }
    INT32 ret = ::connect(sock, (HX_SOCKADDR_T*)addr, sizeof *addr);
    if (ret < 0)
        err = errno;
    return ret;
}

inline INT32
SocketIO::read(void * buf, INT32 len)
{
    if (sock < 0)
    {
        err = EBADF;
        return -1;
    }
    INT32 ret = ::recv(sock, (char*)buf, len, 0);
    err = errno;
    while (ret < 0 && err == EINTR)
    {
        ret = ::recv(sock, (char*)buf, len, 0);
        err = errno;
    }
    return ret;
}

inline INT32
SocketIO::write(const void * buf, INT32 len)
{
    if (sock < 0)
    {
        err = EBADF;
        return -1;
    }
    INT32 ret = ::send(sock, (const char *)buf, len, 0);
    if (ret < 0)
        err = errno;

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
    memset(&addr, 0, addr_len);
    INT32 ret = getsockname(&addr, &addr_len);
    return (ret < 0) ? ret : ntohs(addr.sin_port);
}

inline INT32
SocketIO::reuse_port(HXBOOL enable)
{
#if defined(SO_REUSEPORT) && !defined(__QNXNTO__)
    if (sock < 0)
    {
        err = EBADF;
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
        err = errno;
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
        err = EBADF;
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
        err = errno;
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
    LONG32 result;
#ifdef _SOLARIS
    result = sysinfo(SI_HOSTNAME, name, count);
#elif defined _VXWORKS
    SafeStrCpy(name,  "VXWORKS", count);
#else
    result = ::gethostname(name, count);
#endif /* _SOLARIS */
    if (result < 0)
    {
        return errno;
    }
    return 0;
}

inline off_t
SocketIO::file_size()
{
    ASSERT(0);
    return -1;
}

#endif /* _SOCKIO_H_ */
