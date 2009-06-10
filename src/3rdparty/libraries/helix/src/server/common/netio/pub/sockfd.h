/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sockfd.h,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#ifndef	_SOCK2FD_H_
#define	_SOCK2FD_H_

#if defined _UNIX || defined __MWERKS__
typedef int SOCKET;
#endif

#define	FD_INCR		256
#define	HASH_TAB_SIZE	256

struct Hash_bucket {
	int		fd;
	SOCKET		sock;
	Hash_bucket*	next;

	Hash_bucket(int _fd, SOCKET _sock, Hash_bucket* _next) {
		fd = _fd; sock = _sock; next = _next;
	}
};

class SockFd {
public:
	SockFd();

#if defined WIN32 || defined _WINDOWS
	SOCKET *	fd2socktab;
	int		fd2socktabsize;
	Hash_bucket*	hashtable[HASH_TAB_SIZE];
#endif

	SOCKET	fd2sock(int fd);
	int	sock2fd(SOCKET sock);
	void	sock2fd_delete(int fd);

private:
	int	_sock2fd(SOCKET sock);
	void	_sock2fd_delete(int fd);
};

inline SockFd::SockFd() {
#if defined WIN32
	fd2socktab = 0;
	fd2socktabsize = 0;
	memset(hashtable, 0, HASH_TAB_SIZE * sizeof(Hash_bucket*));
#endif
}

inline int
SockFd::sock2fd(SOCKET sock) {
#if defined _UNIX || defined __MWERKS__
	return sock;
#elif defined WIN32 || defined _WINDOWS
	return _sock2fd(sock);
#endif
}

inline SOCKET
SockFd::fd2sock(int fd) {
#if defined _UNIX || defined __MWERKS__
	return fd;
#elif defined WIN32 || defined _WINDOWS
	return fd >= fd2socktabsize || fd < 0 ? INVALID_SOCKET : fd2socktab[fd];
#endif
}

inline void
SockFd::sock2fd_delete(int fd) {
#if defined WIN32
	_sock2fd_delete(fd);
#endif
}

#endif/*_SOCK2FD_H_*/
