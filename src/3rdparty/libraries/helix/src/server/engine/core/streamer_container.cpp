/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: streamer_container.cpp,v 1.8 2005/08/15 17:36:53 srobinson Exp $
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
#include "hxcom.h"
#include "hxerror.h"

#include "udpio.h"
#include "proc.h"
#include "server_engine.h"
#include "base_errmsg.h"
#include "hxslist.h"
#include "plgnhand.h"
#include "hxmap.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "shutdown.h"

#include "streamer_container.h"

StreamerContainer::StreamerContainer(Process* proc, ProcessContainer* copy_pc)
    :
    ProcessContainer(proc, copy_pc),
    pProxyContextInfo(NULL)
{
    INT32 result = 0;

    udp_channel = new UDPIO();
    engine->RegisterSock();

    result = udp_channel->init(INADDR_ANY, 0, FALSE);
    if (result != 0)
    {
        ERRMSG(copy_pc->error_handler, "Streamer's UDP Init Failed\n");
        /* XXX PSH - Need to print error and kill server? */
    }

    m_BusMap.InitHashTable(1024);
    m_BusMapLock = HXCreateMutex();
    
    m_pServerShutdown = new CServerShutdown();
}

StreamerContainer::~StreamerContainer()
{
    CHXMapStringToOb::Iterator i;

    HXMutexLock(m_BusMapLock, TRUE);
    for (i = m_BusMap.Begin(); i != m_BusMap.End(); ++i)
    {
        IHXQoSSignalBus* pBus = (IHXQoSSignalBus*)(*i);

        if (pBus)
        {
            pBus->Close();
            pBus->Release();
            pBus = NULL;
        }
    }
    m_BusMap.RemoveAll();


    HXMutexUnlock(m_BusMapLock);

    HXDestroyMutex(m_BusMapLock);

    HX_DELETE(m_pServerShutdown);
}
