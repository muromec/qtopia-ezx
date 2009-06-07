/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: wsockio.cpp,v 1.2 2006/02/14 21:35:27 bobclark Exp $ 
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
#include <stdlib.h>
#include "hxtypes.h"

#include "sockio.h"
#include "tcpio.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

INT32
SocketIO::init(INT32 type, HXBOOL blocking, HXBOOL reuse_addr, HXBOOL reuse_port) 
{
    UINT32 mode = 1;

    /*
     * Multicast gets initialized through set_multicast, same as BSD
     */
#ifdef _WIN32
    sock = ::socket(AF_INET, type, 0);
#elif _WIN16
    sock = SOCKET(0);
#endif

    _flags = O_RDWR;
    if (sock == INVALID_SOCKET) 
    {
	err = WSAGetLastError();
	return -1;
    }

    if(reuse_addr)
    {
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       (const char*)&mode, sizeof mode) < 0)
	{
	    // XXX we will ignore this error for now, so we continue...
	}
    }
    if (reuse_port)
    {
	if (this->reuse_port(1) < 0)
	{
	    return -1;
	}
    }

    if (!blocking && ::ioctlsocket(sock, FIONBIO, &mode) < 0) 
    {
	err = WSAGetLastError();
	goto reterr;
    }

    return 0;

reterr:
    close();
    return -1;
}


INT32
SocketIO::init(INT32 type, UINT32 local_addr, INT16 port, 
			   HXBOOL blocking, 
	           HXBOOL reuse_addr, HXBOOL reuse_port) 
{
    struct sockaddr_in addr;
    UINT32 mode = 1;

    /*
     * Multicast gets initialized through set_multicast, same as BSD
     */
#ifdef _WIN32
    sock = ::socket(AF_INET, type, 0);
#elif _WIN16
    sock = SOCKET(0);
#endif

    _flags = O_RDWR;
    if (sock == INVALID_SOCKET) 
    {
	err = WSAGetLastError();
	return -1;
    }

    if(reuse_addr)
    {
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       (const char*)&mode, sizeof mode) < 0)
	{
	    // XXX we will ignore this error for now, so we continue...
	}
    }
    if (reuse_port)
    {
	if (this->reuse_port(1) < 0)
	{
	    return -1;
	}
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(local_addr);
    addr.sin_port = htons(port);
    if (bind(&addr) < 0) 
    {
	err = WSAGetLastError();
	goto reterr;
    }

    if (!blocking && ::ioctlsocket(sock, FIONBIO, &mode) < 0) 
    {
	err = WSAGetLastError();
	goto reterr;
    }

    return 0;

reterr:
    close();
    return -1;
}

INT32
SocketIO::connect(char* host, INT16 port)
{
    if (sock < 0)
    {
	return -1;
    }
    struct sockaddr_in addr;
    SocketIO::create_address(addr, host, port);
    INT32 ret = ::connect(sock, (sockaddr*)&addr, sizeof addr);
    if (ret < 0)
	err = WSAGetLastError();
    return ret;
}

TCPIO*
SocketIO::accept(sockaddr_in *addr, INT32* addrlen)
{
    if (sock < 0)
    {
	err = WSAEBADF;
	return 0;
    }
    int temp_addr = HX_SAFEINT(*addrlen);
    INT32 ret = ::accept(sock, (sockaddr*)addr, &temp_addr);
    if (ret < 0)
    {
	err = WSAGetLastError();
	return 0;
    }
    *addrlen = temp_addr;
    return new TCPIO(HX_SAFEINT(ret));
}
