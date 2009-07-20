/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: proc_container.cpp,v 1.37 2009/02/25 21:16:50 dcollins Exp $
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
#include "hxcomm.h"
#include "hxnet.h"
#include "hxclfact.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "qos_clfact.h"
#include "hxstring.h"
#include "hxstats.h"
#include "server_context.h"
#include "server_control.h"
#include "mem_cache.h"
#include "multicast_mgr.h"
#include "server_engine.h"
#include "resolvcache.h"
#include "servsockimp.h"
#include "server_inetwork.h"
#include "server_network_services.h"
#include "sockimp.h"
#include "servsked.h"
#include "errhand.h"
#include "error_sink_ctrl.h"
#include "error_sink_handler.h"
#include "iasyncio.h"
#include "xmlconfig.h"
#include "uasconfig.h"
#include "mcast_ctrl.h"
#include "hxmap.h"
#include "bcastmgr.h"
#include "altserverproxycfg.h"
#include "altserverproxy.h"
#include "server_prefs.h"
#include "mutexlist.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "qos_prof_mgr.h"
#include "qos_sig_bus_ctl.h"
#include "qos_diffserv_cfg.h"
#include "hxclientprofile.h"
#include "hxprofilecache.h"
#include "client_profile_mgr.h"
#include "profile_cache.h"
#include "logoutputs.h"
#include "hxrssmgr.h"
#include "rssmgr.h"
#include "namedlock.h"
#include "resolve_proc.h"
#include "tsmap.h"
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
#include "mmsrsend.h"
#endif

#include "proc_container.h"

#include "rtspstats.h"
#include "sdpstats.h"
#include "server_stats.h"

extern BOOL g_bFastMalloc;
extern BOOL g_bFastMallocAll;

ProcessContainer::ProcessContainer(Process* proc)
{
    engine                  = new ServerEngine(proc);
    license                 = 0;
    config                  = 0;
    streamer_info           = 0;
    registry                = 0;
    conn_id_table           = 0;
    rdispatch               = 0;
    resolver_info           = 0;
    plugin_handler          = 0;
    server_info             = 0;
    loadinfo                = 0;
    server_prefs            = 0;
    error_sink_handler      = 0;
    misc_plugins            = 0;
    allowance_plugins       = 0;
    dispatchq               = 0;
    server_fork             = 0;
    config_file             = 0;
    multicast_mgr           = 0;
    mcast_ctrl              = 0;
    HTTP_deliver_paths      = 0;
    HTTP_postable_paths     = 0;
    mime_type_dict          = 0;
    cloaked_guid_dict       = 0;
    fcs_session_map         = 0;
    sspl_session_map        = 0;
    broadcast_manager       = 0;
    load_listen_mgr         = 0;
    mcst_addr_pool          = 0;
    sap_mgr                 = 0;
    global_server_control   = 0;
    data_convert_con        = 0;
    alt_server_proxy_cfg_mgr = 0;
    client_stats_manager    = NULL;
    qos_prof_select         = 0;
    qos_bus_ctl             = 0;
    qos_sig_src             = 0;
    qos_diffserv_cfg        = 0;
    capex_static_cache      = 0;
    capex_profile_cache     = 0;
    client_profile_manager  = 0;
    rss_manager             = NULL;
    resolver_pool           = NULL;

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    mmsResendManager        = 0;
#endif
#if defined _UNIX
    m_pResMUX               = 0;
#endif
    process_type            = PTUnknown;

    if (!g_bFastMallocAll && !g_bFastMalloc)
        mem_cache           = 0;
    else
        mem_cache           = new MemCache(proc->procnum());
    server_context          = new ServerContext(proc, mem_cache);
    server_control          = new ServerControl(proc);
    common_class_factory    = new HXCommonClassFactory(proc, mem_cache);
    qos_class_factory       = new HXQoSClassFactory(proc);
    network_services        = new CServerNetworkServicesContext(proc);
    net_services            = new CServNetServices(proc);
    scheduler               = new ServerScheduler(proc);
    ischeduler              = new ServerIScheduler(proc);
    error_handler           = new ErrorHandler(proc);
    error_sink_proc         = new ErrorSinkProc(proc);
    async_io                = new CAsyncIOSelection(proc);
    cached_fs_map           = new CHXMapLongToObj;
    cached_dc_map           = new CHXMapLongToObj;
    managed_mutex_list      = new ManagedMutexList();
    cached_cdist_advise_list= 0;
    alt_server_proxy        = 0;
    rtspstats_manager       = 0;
    sdpstats_manager        = 0;
    named_lock_manager      = new CNamedLock();
    resolver_pool           = new CResolveProcPool();

    /*
     * AddRef() all objects handed out by ServerContext
     */

    server_context->AddRef();
    common_class_factory->AddRef();
    qos_class_factory->AddRef();
    network_services->AddRef();
    net_services->AddRef();
    scheduler->AddRef();
    ischeduler->AddRef();
    error_handler->AddRef();
    error_sink_proc->AddRef();
    async_io->AddRef();
    server_control->AddRef();
    named_lock_manager->AddRef();
}

void
ProcessContainer::Copy(ProcessContainer* pc)
{
    license                 = pc->license;
    config                  = pc->config;
    uasconfig               = pc->uasconfig;
    streamer_info           = pc->streamer_info;
    mime_type_dict          = pc->mime_type_dict;
    cloaked_guid_dict       = pc->cloaked_guid_dict;
    fcs_session_map         = pc->fcs_session_map;
    sspl_session_map        = pc->sspl_session_map;
    broadcast_manager       = pc->broadcast_manager;
    load_listen_mgr         = pc->load_listen_mgr;
    registry                = pc->registry;
    conn_id_table           = pc->conn_id_table;
    rdispatch               = pc->rdispatch;
    resolver_info           = pc->resolver_info;
    plugin_handler          = pc->plugin_handler;
    server_info             = pc->server_info;
    loadinfo                = pc->loadinfo;
    server_prefs            = pc->server_prefs;
    error_sink_handler      = pc->error_sink_handler;
    misc_plugins            = pc->misc_plugins;
    allowance_plugins       = pc->allowance_plugins;
    dispatchq               = pc->dispatchq;
    server_fork             = pc->server_fork;
    config_file             = pc->config_file;
    multicast_mgr           = pc->multicast_mgr;
    mcast_ctrl              = pc->mcast_ctrl;
    HTTP_deliver_paths      = pc->HTTP_deliver_paths;
    HTTP_postable_paths     = pc->HTTP_postable_paths;
    mcst_addr_pool          = pc->mcst_addr_pool;
    sap_mgr                 = pc->sap_mgr;
    global_server_control   = pc->global_server_control;
    data_convert_con        = pc->data_convert_con;
    alt_server_proxy_cfg_mgr = pc->alt_server_proxy_cfg_mgr;
    client_stats_manager    = pc->client_stats_manager;
    rtspstats_manager       = pc->rtspstats_manager;
    sdpstats_manager        = pc->sdpstats_manager;
    qos_prof_select         = pc->qos_prof_select;
    qos_bus_ctl             = pc->qos_bus_ctl;
    capex_static_cache      = pc->capex_static_cache;
    capex_profile_cache     = pc->capex_profile_cache;
    rss_manager             = pc->rss_manager;
    named_lock_manager      = pc->named_lock_manager;
    resolver_pool           = pc->resolver_pool;

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    mmsResendManager        = pc->mmsResendManager;
#endif

#if defined _UNIX
    m_pResMUX               = pc->m_pResMUX;
#endif

    config_file->AddRef();
    uasconfig->AddRef();
    qos_prof_select->AddRef();
    qos_bus_ctl->AddRef();
    capex_static_cache->AddRef();
    capex_profile_cache->AddRef();
}

ProcessContainer::ProcessContainer(Process* proc, ProcessContainer* pc)
{
    engine                  = new ServerEngine(proc);
    if (!g_bFastMallocAll && !g_bFastMalloc)
        mem_cache           = 0;
    else
        mem_cache           = new MemCache(proc->procnum(), pc->mem_cache);
    server_context          = new ServerContext(proc, mem_cache);
    common_class_factory    = new HXCommonClassFactory(proc, mem_cache);

    qos_class_factory       = new HXQoSClassFactory(proc);
    network_services        = new CServerNetworkServicesContext(proc);
    net_services            = new CServNetServices(proc);
    scheduler               = new ServerScheduler(proc);
    ischeduler              = new ServerIScheduler(proc);
    error_handler           = new ErrorHandler(proc);
    error_sink_proc         = new ErrorSinkProc(proc);
    async_io                = new CAsyncIOSelection(proc);
    server_control          = new ServerControl(proc);
    cached_fs_map           = new CHXMapLongToObj;
    cached_dc_map           = new CHXMapLongToObj;
    managed_mutex_list      = new ManagedMutexList();
    cached_cdist_advise_list= 0;
    alt_server_proxy        = 0;
    process_type            = PTUnknown;
    client_stats_manager    = NULL;
    rtspstats_manager       = NULL;
    sdpstats_manager        = 0;
    qos_sig_src             = new QoSSignalSource(proc, pc->qos_bus_ctl);
    qos_diffserv_cfg        = new QoSDiffServConfigurator(proc);
    client_profile_manager  = new ClientProfileManager(proc);

    /*
     * AddRef() all objects handed out by ServerContext
     */

    server_context->AddRef();
    common_class_factory->AddRef();
    qos_class_factory->AddRef();
    network_services->AddRef();
    net_services->AddRef();
    scheduler->AddRef();
    ischeduler->AddRef();
    error_handler->AddRef();
    error_sink_proc->AddRef();
    async_io->AddRef();
    qos_sig_src->AddRef();
    qos_diffserv_cfg->AddRef();
    client_profile_manager->AddRef();

    Copy(pc);
}
