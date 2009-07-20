/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: usockio.cpp,v 1.4 2008/07/03 21:53:48 dcollins Exp $ 
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

#include <errno.h>

#include "hxtypes.h"

#include "sockio.h"
#include "tcpio.h"

#if defined(_LSB)
#include "hlxclib/sys/ioctl.h"
#endif


const UINT32 SocketIO::MAX_HOSTNAME_LEN = 1024;



INT32
SocketIO::init(INT32 type, HXBOOL blocking,
                 HXBOOL reuse_addr, HXBOOL reuse_port)
{
#ifdef _BEOS
    char mode = 1;
    sock = ::socket(AF_INET, type, 0);
#else    
    INT32 mode = 1;
    sock = ::socket(PF_INET, type, 0);
#endif
    _flags = O_RDWR;
    if (sock < 0) {
	err = errno;
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

#ifdef _BEOS
    if (!blocking && ::setsockopt(sock, SOL_SOCKET,SO_NONBLOCK, &mode,1) < 0) 
#else
    if (!blocking && ::ioctl(sock, FIONBIO, (char*)&mode) < 0) 
#endif
    {
	err = errno;
	goto reterr;
    }

    return 0;

reterr:
    close();
    return -1;
}


INT32
SocketIO::init(INT32 type, UINT32 local_addr, INT16 port, HXBOOL blocking, 
	       HXBOOL reuse_addr, HXBOOL reuse_port) 
{
    struct sockaddr_in addr;
    
#ifdef _BEOS
    char mode = 1;
    sock = ::socket(AF_INET, type, 0);
#else    
    INT32 mode = 1;
    sock = ::socket(PF_INET, type, 0);
#endif
    _flags = O_RDWR;
    if (sock < 0) {
	err = errno;
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
	err = errno;
	goto reterr;
    }

#ifdef _BEOS
    if (!blocking && ::setsockopt(sock, SOL_SOCKET,SO_NONBLOCK, &mode,1) < 0) 
#else
    if (!blocking && ::ioctl(sock, FIONBIO, (char*)&mode) < 0) 
#endif
    {
	err = errno;
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
	err = errno;
    return ret;
}

TCPIO*
SocketIO::accept(sockaddr_in *addr, INT32* addrlen)
{
    if (sock < 0)
    {
	err = EBADF;
	return 0;
    }
    HX_SOCKLEN_T  temp_addr = *addrlen;

    INT32 ret = ::accept(sock, (sockaddr*)addr, &temp_addr);
    if (ret < 0)
    {
	err = errno;
	return 0;
    }
    *addrlen = temp_addr;
    return new TCPIO(ret);
}


