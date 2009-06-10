/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: inetwork_acceptor.cpp,v 1.3 2004/07/24 23:10:19 tmarshall Exp $
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
#include "debug.h"
#include "netbyte.h"
#include "inetwork_acceptor.h"
#include "inetwork.h"

void
INetworkAcceptor::Accepted(TCPIO* tcp_io, sockaddr_in peer, int peerlen, int port)
{
    DPRINTF(D_INFO, ("%ld: Accept conn on port %d from %s:%d\n",
            (UINT32)0/*newclient->id()*/, port, inet_ntoa(peer.sin_addr),
            htons(peer.sin_port)));

    /*
     * No IHXResolver is passed to the INetworkTCPSocketContext
     * constructor because a socket created by Accept cannot be
     * used to Connect
     */

    IHXTCPSocketContext* pConn = new INetworkTCPSocketContext(engine, 0,
        tcp_io, DwToHost(peer.sin_addr.s_addr), WToHost(peer.sin_port));

#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(pConn, 1);
#endif
    pConn->AddRef();

#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(pConn, 14);
#endif

    response->NewConnection(HXR_OK, pConn);

#ifdef PAULM_IHXTCPSCAR
    /*
     * Nullify the last one if NewConnection did not addref.
     */
    ADDR_NOTIFY(pConn, 0);
#endif

#ifdef PAULM_IHXTCPSCAR
    REL_NOTIFY(pConn, 3);
#endif
    pConn->Release();
}
