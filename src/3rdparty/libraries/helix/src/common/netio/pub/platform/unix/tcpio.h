/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tcpio.h,v 1.3 2006/02/14 21:35:27 bobclark Exp $ 
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

#ifndef _TCPIO_H_
#define _TCPIO_H_

#include "sockio.h"
#include <netinet/tcp.h>

class TCPIO: public SocketIO
{
public:
			TCPIO();
			TCPIO(int conn);
			~TCPIO();
    INT32               init(HXBOOL do_block=TRUE, HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    INT32		init(UINT32 local_addr, INT16 port, HXBOOL do_block=TRUE, 
			     HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    INT32		reset_on_kill();
    INT32		set_send_size(UINT32 send_size);
    INT32               get_mss(UINT32& ulMSS);
    INT32               get_err(UINT32& ulERR);
    INT32               get_sndbuf_sz(UINT32& ulSndBufSz);
    INT32               set_ip_tos(UINT32 tos);
};

inline
TCPIO::TCPIO()
{
}

inline
TCPIO::TCPIO(int conn)
    : 
    SocketIO(conn)
{
#if defined PAULM_SOCKTIMING 
    sockaddr_in a;
    int size = sizeof(a);
    if (!::getsockname(conn, (sockaddr*)&a, &size))
    {
	int port = ntohs(a.sin_port);
	g_SocketTimer.Add(conn, port);
    }
    else
    {
	printf("Unbound socket will not get timed!\n");
    }
#endif
}

inline
TCPIO::~TCPIO()
{
}

inline INT32
TCPIO::init(HXBOOL do_block, HXBOOL reuse_addr, HXBOOL reuse_port)
{
    return SocketIO::init(SOCK_STREAM, do_block, reuse_addr, reuse_port);
}


inline INT32
TCPIO::init(UINT32 local_addr, INT16 port, HXBOOL do_block, HXBOOL reuse_addr, 
            HXBOOL reuse_port)
{
    INT32 ret = 
    SocketIO::init(SOCK_STREAM, local_addr, port, do_block, 
			  reuse_addr, reuse_port);

#if defined PAULM_SOCKTIMING 
    g_SocketTimer.Add(sock, port);
#endif
    return ret;
}

inline INT32               
TCPIO::get_mss(UINT32& ulMSS)
{
    INT32 rc;
    HX_SOCKLEN_T len;

    /* query for MSS */
    len = sizeof( ulMSS );
    rc = ::getsockopt( sock, IPPROTO_TCP, TCP_MAXSEG, (char*) &ulMSS, &len );

    return rc;
}

inline INT32               
TCPIO::get_err(UINT32& ulERR)
{
    INT32 rc;
    HX_SOCKLEN_T len;

    /* query for ERR */
    len = sizeof( ulERR );
    rc = ::getsockopt( sock, SOL_SOCKET, SO_ERROR, (char*) &ulERR, &len );

    if (rc == 0) err = ulERR;

    return rc;
}

inline INT32               
TCPIO::get_sndbuf_sz(UINT32& ulSndBufSz)
{
    INT32 rc;
    HX_SOCKLEN_T len;

    /* query for MSS */
    len = sizeof( ulSndBufSz );
    rc = ::getsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char*) &ulSndBufSz, &len );

    return rc;
}

inline INT32
TCPIO::reset_on_kill()
{
#ifdef _BEOS
// linger?  FIXME
    return -1;
#else

    struct linger l;
    l.l_onoff = 1;
    l.l_linger = 0;
    int ret = ::setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&l, 
			   sizeof l);
    if (ret < 0)
        err = errno;
    return ret;
#endif
}

inline INT32
TCPIO::set_send_size(UINT32 send_size)
{
#ifdef _BEOS
    // Be is messed up
    return -1;
#else
again:
    INT32 ret = ::setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
                             (char*)&send_size, sizeof(INT32));
    if (ret < 0 && send_size <= 8192)
    {
	err = errno;
    }
    else if (ret < 0)
    {
	send_size >>= 1;
        goto again;
    }
    return ret;
#endif
}

inline INT32
TCPIO::set_ip_tos(UINT32 tos)
{
#ifdef _BEOS
    // Be is messed up
    return -1;
#else
    return ::setsockopt(sock, IPPROTO_IP, IP_TOS,
			(char*)&tos, sizeof(UINT32));
#endif
}


#endif /* _TCPIO_H_ */
