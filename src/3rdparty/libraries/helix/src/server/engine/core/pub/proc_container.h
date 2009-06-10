/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: proc_container.h,v 1.30 2009/02/25 21:16:50 dcollins Exp $
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

#ifndef _PROCESS_CONTAINER_H_
#define _PROCESS_CONTAINER_H_

#define PROC_RM_CONTROLLER              0
#define PROC_RM_TIMER                   1
#define PROC_RM_CORE                    2
#define PROC_RM_HTTP                    3

class Process;
class ProcessContainer;
class ServerEngine;
class ServerContext;
class IHXNetworkServicesContext;
class CServNetServices;
struct IHXErrorMessages;
struct IHXCommonClassFactory;
struct IHXQoSClassFactory;
class ServerScheduler;
class ServerIScheduler;
class ErrorSinkProc;
class ServerLicense;
class Config;
class UASConfig;
class StreamerInfo;
class ServerRegistry;
class CHXTSID;
class ResolverDispatch;
class ResolverInfo;
class PluginHandler;
class ServerPreferences;
class ServerInfo;
class LoadInfo;
class ErrorSinkHandler;
class CHXMapPtrToPtr;
class DispatchQueue;
class ServerFork;
class CAsyncIOSelection;
class XMLConfig;
class ServerControl;
class MulticastManager;
class ServerAccessControl;
class MulticastAccessControl;
class CHXThreadSafeMap;
class Dict;
class CHXMapLongToObj;
class BroadcastManager;
class LoadBalancedListenerManager;
class MulticastAddressPool;
class CSapManager;
class GlobalServerControl;
class DataConvertController;
class HXList;
class AltServerProxy;
class AltServerProxyConfigHandler;
class ManagedMutexList;
class DistributedLicenseRequester;
class ClientStatsManager;
class QoSProfileSelector;
class QoSSignalBusController;
class QoSSignalSource;
class QoSDiffServConfigurator;
class StaticProfileCache;
class HTTPProfileCache;
class RSSManager;
class CNamedLock;
class CResolveProcPool;

class  SDPAggregateStats;
class  RTSPEventsManager;


#ifdef HELIX_FEATURE_SERVER_WMT_MMS
class CMMSResendManager;
#endif
#if defined _UNIX
class ResolverMUX;
#endif
class MemCache;
class ClientProfileManager;

enum _ProcessType { PTUnknown, PTController, PTCore, PTStreamer, PTMisc,
                    PTResolver, PTMemory, PTWorker };

class ProcessContainer {
public:
                                ProcessContainer(Process* proc);
                                ProcessContainer(Process* proc,
                                                 ProcessContainer* pc);
private:
    // mem_cache won't get inited correctly if you use Copy() directly.
    void                        Copy(ProcessContainer* pc);
public:
    const char*                 ProcessType()
    {
        switch (process_type)
        {
        case PTController:
            return "Controller";
        case PTCore:
            return "Core";
        case PTStreamer:
            return "Streamer";
        case PTMisc:
            return "Misc";
        case PTResolver:
            return "Resolver";
        case PTMemory:
            return "Memory";
        case PTWorker:
            return "Worker";
        default:
            return "Unknown";
        };
    };

    // One instance per process
    ServerEngine*               engine;
    ServerContext*              server_context;
    IHXNetworkServicesContext*  network_services;
    CServNetServices*           net_services;
    IHXCommonClassFactory*      common_class_factory;
    IHXQoSClassFactory*         qos_class_factory;
    ServerScheduler*            scheduler;
    ServerIScheduler*           ischeduler;
    IHXErrorMessages*           error_handler;
    ErrorSinkProc*              error_sink_proc;
    CAsyncIOSelection*          async_io;
    CHXMapLongToObj*            cached_fs_map;
    CHXMapLongToObj*            cached_dc_map;
    HXList*                     cached_cdist_advise_list;
    ServerControl*              server_control;
    enum _ProcessType           process_type;
    MemCache*                   mem_cache;
    AltServerProxy*             alt_server_proxy;
    ManagedMutexList*           managed_mutex_list;
    RTSPEventsManager*          rtspstats_manager;
    SDPAggregateStats*          sdpstats_manager;
    QoSSignalSource*            qos_sig_src;
    QoSDiffServConfigurator*    qos_diffserv_cfg;
    ClientProfileManager*       client_profile_manager;

    // One instance per server
    ServerLicense*              license;
    Config*                     config;
    UASConfig*                  uasconfig;
    StreamerInfo*               streamer_info;
    ServerRegistry*             registry;
    CHXTSID*                    conn_id_table;
    ResolverDispatch*           rdispatch;
    ResolverInfo*               resolver_info;
    PluginHandler*              plugin_handler;
    ServerPreferences*          server_prefs;
    ServerInfo*                 server_info;
    LoadInfo*                   loadinfo;
    ErrorSinkHandler*           error_sink_handler;
    CHXMapPtrToPtr*             misc_plugins;
    CHXMapPtrToPtr*             allowance_plugins;
    DispatchQueue*              dispatchq;
    ServerFork*                 server_fork;
    XMLConfig*                  config_file;
    MulticastManager*           multicast_mgr;
    ServerAccessControl*        access_ctrl;
    MulticastAccessControl*     mcast_ctrl;
    char***                     HTTP_deliver_paths;
    char***                     HTTP_postable_paths;
    CHXThreadSafeMap*           cloaked_guid_dict;
    CHXThreadSafeMap*           fcs_session_map;
    CHXThreadSafeMap*           sspl_session_map;
    Dict*                       mime_type_dict;
    BroadcastManager*           broadcast_manager;
    LoadBalancedListenerManager*load_listen_mgr;
    MulticastAddressPool*       mcst_addr_pool;
    CSapManager*                sap_mgr;
    GlobalServerControl*        global_server_control;
    DataConvertController*      data_convert_con;
    AltServerProxyConfigHandler* alt_server_proxy_cfg_mgr;
    ClientStatsManager*         client_stats_manager;
    QoSProfileSelector*          qos_prof_select;
    QoSSignalBusController*      qos_bus_ctl;
    StaticProfileCache*         capex_static_cache;
    HTTPProfileCache*           capex_profile_cache;
    RSSManager*                 rss_manager;
    CNamedLock*                 named_lock_manager;
    CResolveProcPool*           resolver_pool;

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    CMMSResendManager*          mmsResendManager;
#endif
#if defined _UNIX
    ResolverMUX*                m_pResMUX;
#endif
};

#endif /* _PROCESS_CONTAINER_H_ */
