/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sockfd.cpp,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
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

#include <stdio.h>
#include "machdep.h"
#if defined WIN32 || defined TESTING

#if	defined unix
#define	INVALID_SOCKET -1
#endif

#include <string.h>
#include "debug.h"
#include "sockfd.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


static int HASH(SOCKET sock) {
	return sock % HASH_TAB_SIZE;
}

int
SockFd::_sock2fd(SOCKET sock) {

	int h = HASH(sock);
	int fd;

	// search through the hash table
	for (Hash_bucket* hb = hashtable[h]; hb != 0; hb = hb->next)
		if (hb->sock == sock) {
			return hb->fd;
		}

	// search for an empty slot
	for (fd = 0; fd < fd2socktabsize; fd++)
		if (fd2socktab[fd] == INVALID_SOCKET) {
			break;
		}

	// increase the table size if necessary
	if (fd >= fd2socktabsize) {
		SOCKET * tab = new SOCKET[fd2socktabsize + FD_INCR];
		if (fd2socktab)
			memcpy(tab, fd2socktab, fd2socktabsize*sizeof sock);
		for (int i = fd2socktabsize; i < fd2socktabsize + FD_INCR; i++)
			tab[i] = INVALID_SOCKET;
		fd2socktabsize += FD_INCR;
		if (fd2socktab)
			delete[] fd2socktab;
		fd2socktab = tab;
	}

	fd2socktab[fd] = sock;
	hashtable[h] = new Hash_bucket(fd, sock, hashtable[h]);

	return fd;
}

void
SockFd::_sock2fd_delete(int fd)
{
	if (0 > fd || fd >= fd2socktabsize) {
		return;
	}

	SOCKET sock = fd2socktab[fd];
	if (sock == INVALID_SOCKET) {
		return;
	}

	int h = HASH(sock);
	Hash_bucket* hb, **hbp;
	for (hbp = &hashtable[h]; (hb = *hbp) != 0; hbp = &hb->next) {
		if (hb->fd != fd)
			continue;
		*hbp = hb->next;
		delete hb;
		break;
	}
	fd2socktab[fd] = INVALID_SOCKET;
}
#endif
