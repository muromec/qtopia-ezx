/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: wudpio.cpp,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
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

#include "hxtypes.h"
#include "udpio.h"

INT32
UDPIO::set_multicast()
{
#ifdef NO_MULTICAST
    err = EPERM;
    return -1;
#else
    INT32         	ret;
    sockaddr_in 	addr;
    INT32         	len = sizeof (addr);

    ret = getsockname(&addr, &len);
    if (ret < 0)
    {
	err = WSAGetLastError();
        return ret;
    }

    ret = ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
                       (char*) &addr.sin_addr.s_addr,
                       sizeof (addr.sin_addr.s_addr));
    if (ret < 0)
    {
        err = WSAGetLastError();
    }
    return ret;

#endif /* NO_MULTICAST */
}

INT32
UDPIO::set_multicast_ttl(unsigned char ttl)
{
#ifdef NO_MULTICAST
    err = EPERM;
    return -1;
#else

    INT32         ret;
    INT32         ttl_proxy = ttl;

    ret = ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
                       (char*) &ttl_proxy, sizeof (ttl_proxy));

    if (ret < 0)
    {
        err = WSAGetLastError();
    }
    return ret;

#endif /* ! NO_MULTICAST */
}
