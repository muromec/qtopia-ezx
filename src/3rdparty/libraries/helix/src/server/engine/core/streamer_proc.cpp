/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: streamer_proc.cpp,v 1.11 2006/05/22 19:22:20 dcollins Exp $
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
#include "hxstring.h"
#include "hxplugn.h"
#include "ihxpckts.h"
#include "hxformt.h"

#ifdef _UNIX
#include "unix_misc.h"
#endif

#include "server_engine.h"
#include "server_context.h"
#include "servsockimp.h"
#include "server_inetwork.h"
#include "dispatchq.h"
#include "core_proc.h"
#include "proc.h"
#include "streamer_container.h"
#include "streamer_info.h"
#include "shared_udp.h"
#include "streamer_proc.h"
#include "shmem.h"
#include "base_errmsg.h"
#include "altserverproxy.h"
#include "hxqos.h"
#include "qos_sig_bus_ctl.h"
#include "shutdown.h"


extern int* VOLATILE pStreamerProcInitMutex;

void
StreamerProcessInitCallback::func(Process* proc)
{
    int                         result;

    StreamerContainer* streamer_container;
    streamer_container = new StreamerContainer(proc, m_proc->pc);
    proc->pc = streamer_container;
    proc->pc->dispatchq->init(proc);
    proc->pc->process_type = PTStreamer;

    /*
     * Now that the proc->pc is established, it is safe to initialize the
     * IHXNetworkServicesContext
     */

    proc->pc->network_services->Init(proc->pc->server_context,
                                     proc->pc->engine, NULL);

    proc->pc->net_services->Init(proc->pc->server_context);

    streamer_container->m_pServerShutdown->Init(proc);

    // open up the incoming UDP port(s) (if enabled)

    SharedUDPPortReader* reader = new SharedUDPPortReader(proc->pc->server_context);
    reader->AddRef();
    streamer_container->engine->SetSharedUDPReader(reader);

    ((StreamerContainer*)(proc->pc))->m_ulStreamerNum =
      proc->pc->streamer_info->NewStreamer(proc);

    HX_VERIFY(HXR_OK == proc->pc->qos_bus_ctl->AddStreamer(proc));

    if (m_proc->pc->alt_server_proxy_cfg_mgr)
    {
        proc->pc->alt_server_proxy = new AltServerProxy(proc);
        proc->pc->alt_server_proxy->AddRef();
        if (FAILED(proc->pc->alt_server_proxy->
                SetCfgHandler(m_proc->pc->alt_server_proxy_cfg_mgr)))
        {
            HX_RELEASE(proc->pc->alt_server_proxy);
        }
    }

    *pStreamerProcInitMutex = 0;

    proc->pc->engine->mainloop();
}
