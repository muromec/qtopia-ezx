/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tcpio.h,v 1.2 2006/02/14 21:35:27 bobclark Exp $ 
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

#ifdef _WINSOCK2API_
#define	IP_TOS			3 /* IP type of service and preced*/
#endif


class TCPIO: public SocketIO
{
public:
			TCPIO();
			TCPIO(int conn);
			~TCPIO();
    INT32               init(HXBOOL do_block=TRUE, HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    INT32		init(UINT32 local_addr, INT16 port, 
		             HXBOOL do_block=TRUE, 
			         HXBOOL reuse_addr=TRUE, HXBOOL reuse_port=FALSE);
    INT32		reset_on_kill();
    INT32		set_send_size(UINT32 send_size);
    INT32               set_ip_tos(UINT32 ip_tos);
    INT32               get_mss(UINT32& ulMSS);
    INT32               get_err(UINT32& ulERR);
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
TCPIO::init(UINT32 local_addr, INT16 port, HXBOOL do_block, 
			HXBOOL reuse_addr, HXBOOL reuse_port)
{
    return SocketIO::init(SOCK_STREAM, local_addr, port, do_block, 
		                  reuse_addr, reuse_port);
}

inline INT32
TCPIO::reset_on_kill()
{
    struct linger l;
    l.l_onoff = 1;
    l.l_linger = 0;
    int ret = ::setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&l, 
			   sizeof l);
    if (ret < 0)
        err = WSAGetLastError();
    return ret;
}

inline INT32
TCPIO::set_send_size(UINT32 send_size)
{
    INT32 ret = ::setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
			     (char*)&send_size, sizeof(INT32));
    if (ret < 0)
	err = WSAGetLastError();
    return ret;
}

inline INT32
TCPIO::set_ip_tos(UINT32 ip_tos)
{
    INT32 ret = ::setsockopt(sock, IPPROTO_IP, IP_TOS,
                             (char*)&ip_tos, sizeof(UINT32));
    if (ret < 0)
	err = WSAGetLastError();
    return ret;
}

inline INT32               
TCPIO::get_mss(UINT32& ulMSS)
{
    //win32 socket api does not support TCP_MAXSEG from getsockopt
    ulMSS = 0;
    return -1;
}

inline INT32               
TCPIO::get_err(UINT32& ulERR)
{
    INT32 rc;
    INT32 len;

    /* query for ERR */
    len = sizeof(ulERR );
    rc = ::getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&ulERR, (int*)&len );

    if (rc == 0) err = ulERR;

    return rc;
}

#endif /* _TCPIO_H_ */
