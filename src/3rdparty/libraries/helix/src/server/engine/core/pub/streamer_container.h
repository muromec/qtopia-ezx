/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: streamer_container.h,v 1.7 2004/07/31 15:17:41 dcollins Exp $
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

#ifndef _STREAMER_CONTAINER_H_
#define _STREAMER_CONTAINER_H_

#include "mutex.h"
#include "proc_container.h"
#include "chxmapstringtoob.h"

class UDPIO;
class Process;
class SourceFinder;
class CServerShutdown;

_INTERFACE IHXSocket;

class StreamerContainer: public ProcessContainer
{
public:
    StreamerContainer(Process* proc, ProcessContainer* copy_pc);
    ~StreamerContainer();

    UDPIO*              udp_channel;
    IHXSocket*          udp_sock;
    CHXMapStringToOb    m_BusMap;
    CServerShutdown*    m_pServerShutdown;
    HX_MUTEX            m_BusMapLock;
    UINT32              m_ulStreamerNum;

    //XXX JJ this one will only be used by the proxy
    void *              pProxyContextInfo;
};

#endif /* _STREAMER_CONTAINER_H_ */
