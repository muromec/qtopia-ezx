/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sockio.h,v 1.8 2005/04/28 23:40:16 ehyche Exp $
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

#ifndef _MAC_MACHO
#include "platform/mac/oterrs.h"
#endif
//#include "OpenTransport.h" 
#include "bio.h"
#include "platform/mac/macsockets.h"
#ifndef _PN_MACHDEP_H_	// because of the mish-mash of precompiler hell, we do this this way
#include <errno.h>
#define sock_error() (errno)
#endif
class SockFd;

#ifndef INADDR_NONE
#define INADDR_NONE (0xFFFFFFFF)
#endif /* ndef INADDR_NONE */

class mIO {
public:
	mIO() {fd = -1; err = 0;}
	~mIO() {}
protected:
    INT16			fd;
    INT16			err;
    INT16			_flags;
};
#ifdef DEFINED_ELSEWHERE
class IO {
public:
			IO();
    virtual		~IO();
    virtual INT16		read(void* buf, INT32 size) = 0;
    virtual INT16		write(const void* buf, INT32 size) = 0;
    virtual off_t		seek(off_t off, INT32 whence) = 0;
    virtual INT32		close() = 0;
    virtual INT16		ioctl(UINT32 request, char* arg) = 0;
    virtual INT16		status(struct stat *st) = 0;
    INT16			filenum();
    INT16			error();
    INT16			flags();

protected:
    INT16			fd;
    INT16			err;
    INT16			_flags;
};

inline
IO::IO() {
    fd = -1;
    err = 0;
}

inline INT16
IO::filenum() {
    return fd;
}

inline INT16
IO::error() {
    return err;
}

inline INT16
IO::flags() {
    return _flags;
}
#endif //DEFINED_ELSEWHERE

#ifdef DEFINED_ELSEWHERE
inline
IO::~IO() {
}
#endif

class File_IO : public IO, mIO {
public:
			File_IO(const char* file, INT16 flags, mode_t mode = 0666);
			File_IO(INT16 fd, INT16 flags);
			~File_IO();
    INT16			ioctl(UINT32 request, char* arg);
    INT32			close();
    INT32			read(void* buf, INT32 size);
    INT32			write(const void* buf, INT32 size);
    off_t			seek(off_t off, INT32 whence);
    INT16			status(struct stat *st);
    off_t			file_size();

#ifdef _MACINTOSH
private:
    void		*mFile; // pointer to the actual Mac file object
#endif
};

struct SocketIO: public IO,mIO {
			SocketIO();
			~SocketIO();
    static INT32	create_address(struct sockaddr_in& addr, char* host,
				       INT16 port);
    INT32		init(INT32 type, INT16 port, HXBOOL do_block=TRUE, 
			     HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
//    INT32			init(INT16 type, INT16 port, INT16 do_block=1);
    INT32			listen(INT32 backlog);
    INT32			connect(char* host, INT16 port);
    INT32			connect(sockaddr_in *addr);
    INT16			accept(sockaddr_in *addr, INT32* addrlen);
    INT32			bind(sockaddr_in*);
    INT32			blocking();
    INT32			nonblocking();
    INT32			getsockname(sockaddr_in* addr, INT32* addr_len);
//    INT32			ioctl(UINT32 request, char* arg);
    INT32		close();
    INT32		read(void* buf, INT32 size);
    INT32		write(const void* buf, INT32 size);
    off_t		seek(off_t off, INT32 whence);
    INT16		port();
    INT32		disable();
    INT32		reuse_port(HXBOOL enable);
    INT32		reuse_addr(HXBOOL enable);
    INT32		error();
    INT32		flags();
//    virtual SOCKET	socket();
    off_t		file_size();

    static const UINT32 MAX_HOSTNAME_LEN;
    static INT16	gethostname(char* name, INT16 count);

    void		start_write();
    void		stop_write();
    INT16			status(struct stat *st);
    INT16 		socknum();
    
    INT16			sock;
    INT32		do_write;
    SockFd*		sock_fd;
    
#ifdef _MACINTOSH
protected:

    void		*mSocket;	// ptr to the actual Mac network socket
    INT16			mBackLog;	// accepting socket backlog
#endif

};

// XXX does not belong in the SocketIO class
inline void
SocketIO::stop_write() {
    do_write = 0;
}

// XXX does not belong in the SocketIO class
inline void
SocketIO::start_write() {
    do_write = 1;
}

inline INT16
SocketIO::socknum()
{
    return sock;
}

inline INT32
SocketIO::disable()
{
    sock = -2;
    return 0;
}


struct UDP_IO: public SocketIO {
			UDP_IO(SockFd* s);
    INT16			init(INT16 port, INT16 do_block=1);
    INT16			connect(char* host, INT16 port, INT16 do_block=1);
    INT16			connect(sockaddr_in *addr, INT16 do_block=1);
    INT16			recvfrom(void *buf, size_t len, INT16 flags,
				 sockaddr *from,INT16 *fromlen);
    INT16			recvfrom(void *buf, size_t len, INT16 flags,
				 sockaddr_in *from,INT16 *fromlen);
    INT16			sendto(const char* msg, size_t len, INT16 flags,
			       const struct sockaddr_in *to, INT16 tolen);
};

struct TCP_IO: public SocketIO {
			TCP_IO(SockFd* s);
			TCP_IO(INT16 conn, SockFd* s);
    INT16			init(INT16 port, INT16 do_block=1);
    INT16			connect(char* host, INT16 port, INT16 do_block=1);
    INT16			connect(sockaddr_in *addr, INT16 do_block=1);
    INT16			listen(INT16 port, INT16 backlog, INT16 do_block=1);
    INT16			reset_on_kill();
};

// XXX abstract out the interface so that MWERKS def is not needed
inline INT16
SocketIO::gethostname(char* name, INT16 count)
{
INT16 result = ::gethostname(name, count);
    if (result < 0) return sock_error();
    return 0;
}

#undef sock_error
#endif/*_SOCKIO_H_*/
