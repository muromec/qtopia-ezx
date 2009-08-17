/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: udpio.h,v 1.5 2004/05/13 20:42:17 tmarshall Exp $ 
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

#ifndef _UDPIO_H_
#define _UDIIO_H_

#include <errno.h>
#include "sockio.h"

class UDPIO: public SocketIO
{
public:
			UDPIO();
			~UDPIO();

    INT32		init(UINT32 local_addr, INT16 port, BOOL do_block=1, 
			     BOOL reuse_addr=1, BOOL reuse_port=0);
    INT32		recvfrom(void *buf, size_t len, INT32 flags, 
				 sockaddr *from,INT32 *fromlen);
    INT32		recvfrom(void *buf, size_t len, INT32 flags, 
				 sockaddr_in *from,INT32 *fromlen);
    INT32		sendto(const BYTE* msg, size_t len, INT32 flags,
			       const struct sockaddr_in *to, INT32 tolen);
    INT32		join_group(struct ip_mreq& multicast_group);
    INT32		leave_group(struct ip_mreq& multicast_group);
    INT32		set_multicast();
    INT32		set_multicast_ttl(unsigned char ttl);
    INT32		reuse_port();
    INT32		reuse_port(BOOL enable) {return SocketIO::reuse_port(enable);}
    INT32		set_recv_size(UINT32 recv_size);
    INT32		set_send_size(UINT32 send_size);
    INT32               set_ip_tos(UINT32 ip_tos);
    virtual INT32       disconnect(void);
};

inline
UDPIO::UDPIO() 
{
}

inline
UDPIO::~UDPIO()
{
}

inline INT32
UDPIO::recvfrom(void * addr, size_t len, INT32 flags, 
		    sockaddr* from, INT32 *fromlen)
{
    int temp_from = HX_SAFEINT(*fromlen);

    INT32 ret = ::recvfrom(sock, (char*)addr, len, HX_SAFEINT(flags), from, &temp_from);
    *fromlen = temp_from;
    if (ret < 0)
	err = WSAGetLastError();
    return ret;
}

inline INT32
UDPIO::init(UINT32 local_addr, INT16 port, BOOL do_block, 
			BOOL reuse_addr, BOOL reuse_port)
{
    return SocketIO::init(SOCK_DGRAM, local_addr, port, do_block, 
		                  reuse_addr, reuse_port);
}

inline INT32
UDPIO::recvfrom(void * addr, size_t len, INT32 flags,
		 sockaddr_in* from, INT32 *fromlen)
{
    int	temp_from = HX_SAFEINT(*fromlen);
    INT32 ret = ::recvfrom(sock, (char*)addr, len, HX_SAFEINT(flags), (sockaddr*)from, &temp_from);
    *fromlen = temp_from;
    if (ret < 0)
	err = WSAGetLastError();
    return ret;
}

inline INT32
UDPIO::sendto(const BYTE* msg, size_t len, INT32 flags,
	       const struct sockaddr_in* to, INT32 tolen)
{
    INT32 ret = ::sendto(sock, (char*) msg, len, HX_SAFEINT(flags), (sockaddr*) to, HX_SAFEINT(tolen));
    if (ret < 0)
	err = WSAGetLastError();
    return ret;
}

inline INT32
UDPIO::join_group(struct ip_mreq& multicast_group)
{

#ifdef NO_MULTICAST
    socket_io->err = EPERM;
    return -1;
#endif /* NO_MULTICAST */

    INT32 ret;
    ret = ::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
		       (char*) &multicast_group, sizeof (multicast_group));
    if (ret < 0)
    {
	err = WSAGetLastError();
    }
    return ret;
}

inline INT32
UDPIO::leave_group(struct ip_mreq& multicast_group)
{
#ifdef NO_MULTICAST
    err = EPERM;
    return -1;
#endif /* NO_MULTICAST */
    INT32 ret;
    ret = ::setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
		       (char*) &multicast_group, sizeof (multicast_group));
    if (ret < 0)
    {
	err = errno;
    }
    return ret;
}

inline INT32
UDPIO::set_recv_size(UINT32 recv_size)
{
    INT32 ret = ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
                             (char*)&recv_size, sizeof(INT32));
    if (ret < 0)
    {
	err = errno;
    }
    return ret;
}

/*****************************************************************************
 * UDPIO::disconnect
 *
 * Description: disconnects a UDP socket
 *
 * Implementation: specify null address in connect call
 */
inline INT32
UDPIO::disconnect(void)
{
    // init all members to 0 which covers specifying null address as s_addr
    struct sockaddr_in disConAddr = {0};
    disConAddr.sin_family = AF_UNSPEC;
    int ret = ::connect(sock, (sockaddr*)&disConAddr, sizeof disConAddr);

    return ret;
}

inline INT32
UDPIO::set_send_size(UINT32 send_size)
{
    INT32 ret = ::setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
			     (char*)&send_size, sizeof(INT32));
    if (ret < 0)
    {
	err = errno;
    }
    return ret;
}

inline INT32
UDPIO::set_ip_tos(UINT32 ip_tos)
{
    INT32 ret = ::setsockopt(sock, IPPROTO_IP, IP_TOS,
                             (char*)&ip_tos, sizeof(UINT32));
    if (ret < 0)
    {
	err = errno;
    }
    return ret;
}

#endif /* _UUDPIO_H_ */
