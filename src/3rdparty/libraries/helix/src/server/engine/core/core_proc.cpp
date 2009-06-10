/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: core_proc.cpp,v 1.79 2009/02/25 21:16:50 dcollins Exp $
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

#include "hlxclib/time.h"
#include "hlxclib/signal.h"
#include "hlxclib/limits.h"
#include "hlxclib/sys/types.h"
#include "hlxclib/pwd.h"
#include "hlxclib/grp.h"
#include "hlxclib/errno.h"

#ifdef _UNIX
#include <sys/socket.h>
#endif

#include "hxtypes.h"
#include "platform_config.h"
#include "hxcom.h"
#include "hxmon.h"
#include "hxauth.h"
#include "hxengin.h"
#include "dllpath.h"
#include "platform.h"

#ifdef _UNIX
#include "unix_misc.h"
#include "hxcomm.h"
#include "hxerror.h"
#include "resolver_mux.h"
#endif

#include "hxstrutl.h"
#include "hxstring.h"
#include "hxslist.h"
#include "plgnhand.h"

#include "proc.h"
#include "shmem.h"
#include "mutex.h"
#include "simple_callback.h"
#include "dispatchq.h"
#include "server_engine.h"
#include "server_context.h"
#include "servsockimp.h"
#include "server_inetwork.h"
#include "base_errmsg.h"
#include "acceptor.h"
#include "inetwork.h"
#include "inetwork_acceptor.h"
#include "hxprot.h"
#include "listenresp.h"
#include "rtsp_listenresp.h"
#include "http_listenresp.h"
#include "core_container.h"
#include "core_controller_cont.h"
#include "streamer_info.h"
#include "resolver_info.h"
#include "resolver_dispatch.h"
#include "servsked.h"
#include "conn_dispatch.h"
#include "_main.h"
#include "fdpass_socket.h"
#include "config.h"
#include "uasconfig.h"
#include "servreg.h"
#include "server_prefs.h"
#include "server_info.h"
#include "loadinfo.h"
#include "error_sink_handler.h"
#include "server_fork.h"
#include "iasyncio.h"
#include "hxreg.h"
#include "netbyte.h"
#include "microsleep.h"
#include "multicast_mgr.h"
#include "load_balanced_listener.h"
#include "lbl_cdispatch.h"
#include "servbuffer.h"
#include "server_version.h"

#include "core_proc.h"
#include "misc_plugin.h"

#include "mcast_ctrl.h"
#include "tsdict.h"
#include "bcastmgr.h"
#include "hdprefctrl.h"
#include "mimetpref.h"
#include "tsmap.h"

#include "hxqossig.h"
#include "hxqos.h"
#include "qos_prof_mgr.h"
#include "qos_sig_bus_ctl.h"

#include "hxprofilecache.h"
#include "profile_cache.h"

#include "addrpool.h"       // IHXMulticastAddressPool
#include "mcastaddr_mgr.h"  // MulticastAddressPool
#include "sapmgr.h"         // IHXSapManager
#include "sapclass.h"       // CSapManager
#include "server_control.h" // GlobalServerControl
#include "hxdtcvt.h"        // IHXDataConvert
#include "dtcvtcon.h"       // DataConvertController
#include "altserverproxycfg.h"      // AltServerProxyConfigHandler

#include "hxstats.h"
#include "server_stats.h"
#include "logoutputs.h"
#include "rssmgr.h"

#include "rtspstats.h"
#include "sdpstats.h"
#include "tsid.h"

#include "hxprot.h"
#include "namedlock.h"

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
#include "mms_listenresp.h"
#include "mmsrsend.h"       // CMMSResendManager
#endif

#ifdef PAULM_LEAKCHECK
extern UINT32 g_ulLeakCheckFirst;
extern UINT32 g_ulLeakCheckAlloc;
extern UINT32 g_ulLeakCheckFree;
extern BOOL g_bLeakCheck;
#endif

//These are set by the command-line parser:
extern UINT32 g_ulDebugFlags;
extern UINT32 g_ulDebugFuncFlags;
extern char* VOLATILE g_pszConfigFile;
extern char* VOLATILE g_pszImportKey;
extern UINT32 g_ulSizemmap;
extern UINT32* g_pMemoryMappedDataSize;
extern char* g_pHeartBeatIP;


void os_init();
void os_start();
#ifdef _WIN32
void MaybeNotifyServiceUp();
#endif
extern BOOL g_bSkipCPUTest;
extern int CPUDetect();
extern int _main(int argc, char** argv);

extern char g_szStartupLog[];

#ifdef _UNIX
#include <sys/utsname.h>
#endif

#ifdef _UNIX
#ifdef _LINUX

#include <net/if.h>


/*
 * this code a slightly modified version of the one in
 * W. Richard Stevens book -- UNIX network programming
 *
 * it is used to fix a bug in the server where if it is
 * installed on a red hat 6.1 box which has the default
 * /etc/hosts file (which does not contain the ip addr
 * of the box) then the server only binds with localhost
 *
 * this func returns the ip addrs of all the network
 * interface addrs that are configured and UP.
 */
UINT32*
GetNetworkInterfaceAddrs()
{
    static const int MAX_ADDRS = 1025; // terminate with 0x00000000
    static UINT32 ulIPAddrs[MAX_ADDRS];
    memset(ulIPAddrs, 0, sizeof(UINT32) * MAX_ADDRS);

    struct ifconf ifc;
    struct ifreq *ifr;
    struct ifreq ifrcopy;
    int nAddrCount = 0;
    const char* localHost = "127.0.0.1";
    ULONG32 ulLocalHost = ::inet_addr(localHost);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int len, lastlen;
    char* ptr;
    char* buf;
    if (!sockfd)
    {
        perror("getting network interface info");
        return 0;
    }

    lastlen = 0;
    len = 100 * sizeof(struct ifreq); // initial buf size guess
    for (;;)
    {
        buf = new char[len];
        ifc.ifc_len = len;
        ifc.ifc_buf = buf;
        if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
        {
            if (errno != EINVAL || lastlen != 0)
                perror("ioctl error");
                return 0;
        }
        else
        {
            if (ifc.ifc_len == lastlen)
                break; // success len has not changed
            lastlen = ifc.ifc_len;
        }
        len += 10;
        HX_DELETE(buf);
    }

    for (ptr = buf; ptr < buf + ifc.ifc_len;)
    {
        ifr = (struct ifreq *)ptr;
#ifdef HAVE_SOCKADDR_SA_LEN
        len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len)
#else
        switch (ifr->ifr_addr.sa_family)
        {
#ifdef IPV6
            case AF_INET6:
                len = sizeof(struct sockaddr_in6);
                break;
#endif
            case AF_INET:
            default:
                len = sizeof(struct sockaddr);
                break;
        }
#endif /* HAVE_SOCKADDR_SA_LEN */

        ptr += sizeof(ifr->ifr_name) + len; // for next one in buf

        ifrcopy = *ifr;

        if (ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy) < 0)
        {
            perror("ioctl error while getting if flags");
            continue;
        }
        if ((ifrcopy.ifr_flags & IFF_UP) == 0)
        {
            continue;
        }

        struct sockaddr_in* sinptr = (struct sockaddr_in *)&ifr->ifr_addr;
        UINT32 ulNetLongAddr = DwToNet(sinptr->sin_addr.s_addr);
        if (DwToNet(ulLocalHost) != ulNetLongAddr)
        {
            // save space for the terminating 0x00000000
            if (nAddrCount < MAX_ADDRS-1)
            {
                ulIPAddrs[nAddrCount++] = ulNetLongAddr;
            }
        }
    }
    return ulIPAddrs;
}

#endif /* _LINUX */

int
GetGIDFromConfig(Process* proc, Config* config)
{
    const char* pGroupName = 0;
    INT32 ulGID = -1;
    if (pGroupName = config->GetString(proc, "config.Group"))
    {
        /* "%n" allows you to use the GID */
        if (pGroupName[0] == '%')
        {
            ulGID = strtol(pGroupName + 1, 0, 0);
        }
        else
        {
            int isNumeric = 1;
            const char* ptr = pGroupName;

            while (ptr)
            {
                if (!isdigit(*ptr))
                {
                    isNumeric = 0;
                    break;
                }
                ptr++;
            }

            if (isNumeric)
            {
                ulGID = strtol(pGroupName + 1, 0, 0);
            }
            else
            {
                struct group* pGroupInfo = getgrnam(pGroupName);

                if (pGroupInfo)
                {
                    ulGID = pGroupInfo->gr_gid;
                }
                else
                {
                    ERRMSG(proc->pc->error_handler,
                           "Couldn't find group %s in the system database",
                            pGroupName);
                }
            }
        }
    }
    else
        ulGID = config->GetInt(proc, "config.Group");

    return ulGID;
}

int
GetUIDFromConfig(Process* proc, Config* config)
{
    const char* pUserName = 0;
    INT32 ulUID = -1;
    if (pUserName = config->GetString(proc, "config.User"))
    {
        /* "%n" allows you to use the UID */
        if (pUserName[0] == '%')
        {
            ulUID = strtol(pUserName + 1, 0, 0);
        }
        else
        {
            int isNumeric = 1;
            const char* ptr = pUserName;

            while (ptr)
            {
                if (!isdigit(*ptr))
                {
                    isNumeric = 0;
                    break;
                }
                ptr++;
            }

            if (isNumeric)
            {
                ulUID = strtol(pUserName, 0, 0);
            }
            else
            {
                struct passwd* pUserInfo = getpwnam(pUserName);

                if (pUserInfo)
                {
                    ulUID = pUserInfo->pw_uid;
                }
                else
                {
                    ERRMSG(proc->pc->error_handler,
                           "Couldn't find user %s in the system database",
                            pUserName);
                }
            }
        }

    }
    else
    {
        ulUID = config->GetInt(proc, "config.User");
    }

    return ulUID;
}
#endif /* _UNIX */

void
GetHTTPPaths(HXRegistry* hxreg, char* pCharListName, char**& pHTTPablePaths)
{
    IHXValues* pHTTPPath = 0;
    HX_RESULT res = HXR_OK;

    if (HXR_OK == hxreg->GetPropListByName(pCharListName, pHTTPPath))
    {
        const char* name;
        UINT32      id;
        UINT32      ulCount = 0;

        res = pHTTPPath->GetFirstPropertyULONG32(name, id);
        while(res == HXR_OK)
        {
            ulCount++;
            res = pHTTPPath->GetNextPropertyULONG32(name, id);
        }

        pHTTPablePaths = new char*[ulCount + 1];
        UINT32 i = 0;

        res = pHTTPPath->GetFirstPropertyULONG32(name, id);
        while(res == HXR_OK)
        {
            IHXBuffer* pBuf = 0;
            if(HXR_OK == hxreg->GetStrById(id, pBuf))
            {
                pHTTPablePaths[i] = new_string((const char *)pBuf->GetBuffer());
                i++;

                HX_RELEASE(pBuf);
            }
            res = pHTTPPath->GetNextPropertyULONG32(name, id);
        }
        pHTTPablePaths[i] = 0;
        pHTTPPath->Release();
    }
}

TimeZoneCheckCallback::TimeZoneCheckCallback(Process* pProc)
    : m_pProc(pProc)
    , m_RefCount(0)
{

}

STDMETHODIMP_(UINT32)
TimeZoneCheckCallback::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


STDMETHODIMP_(UINT32)
TimeZoneCheckCallback::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
    return m_RefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
TimeZoneCheckCallback::QueryInterface(REFIID riid,
                                      void** ppInterfaceObj)
{

    if (IsEqualIID(riid, IID_IUnknown))
    {
    AddRef();
    *ppInterfaceObj = (IUnknown*)(IHXCallback*)this;
    return HXR_OK;
    }

    else if (IsEqualIID(riid, IID_IHXCallback))
    {
    AddRef();
    *ppInterfaceObj = (IUnknown*)(IHXCallback*)this;
    return HXR_OK;
    }

// No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;

}


STDMETHODIMP
TimeZoneCheckCallback::Func()
{
    SetTimeZone();

    HXTimeval tNow = m_pProc->pc->scheduler->GetCurrentSchedulerTime();

    // Compute time of next on-the-hour by rounding up
    HXTimeval tNext = tNow;
    tNext.tv_sec = ((tNext.tv_sec+(60*60)-1)/(60*60))*(60*60);
    tNext.tv_usec = 0;

    m_pProc->pc->scheduler->AbsoluteEnter(this, tNext);

    return HXR_OK;
}


HX_RESULT
TimeZoneCheckCallback::ScheduleFirstCheckCallback()
{
    HXTimeval tNow = m_pProc->pc->scheduler->GetCurrentSchedulerTime();

    // Compute time of next on-the-hour by rounding up
    HXTimeval tNext = tNow;
    tNext.tv_sec = ((tNext.tv_sec+(60*60)-1)/(60*60))*(60*60);
    tNext.tv_usec = 0;

    return m_pProc->pc->scheduler->AbsoluteEnter(this, tNext);
}


HX_RESULT
TimeZoneCheckCallback::SetTimeZone()
{
    char* szValTemp = new char[255];

    INT32 nTimeZone        = 0;
    char cSign             = 0;
    time_t tTime           = 0;

    struct tm localTime;

    time(&tTime);

    hx_localtime_r(&tTime, &localTime);

// Timezone stuff is nearly a straight copy from logplin.
#if defined(__linux) || defined(WIN32) || defined __sgi || defined __sun || defined(_HPUX) || defined(_AIX)
//
// From what I can see, timezone is the number of seconds it will take to
// get to GMT which is backwards from FreeBSD
//
    nTimeZone = -timezone;
    nTimeZone += (3600 * localTime.tm_isdst);
#elif !defined __hpux && !defined _AIX
    nTimeZone = localTime.tm_gmtoff;
#endif // __linux

    if (nTimeZone < 0)
    {
    nTimeZone = -nTimeZone;
    cSign = '-';
    }

    else
    {
    cSign = '+';
    }

    snprintf(szValTemp, 254, "%c%02ld%02ld", cSign, nTimeZone / 3600, nTimeZone % 3600);
    szValTemp[254] = '\0';
    ServerBuffer* pTZDiff = new ServerBuffer((UCHAR*)szValTemp, strlen(szValTemp));

    if (FAILED(m_pProc->pc->registry->SetStr("server.TZDiff", pTZDiff, m_pProc)))
    {
        if (m_pProc->pc->registry->AddStr("server.TZDiff", pTZDiff, m_pProc) == 0)
        {
        // Couldn't add, couldn't set!
            HX_ASSERT(0);
            return HXR_UNEXPECTED;
        }
    }

    return HXR_OK;
}


void
CoreProcessInitCallback::func(Process* proc)
{
    Config*         config   = NULL;
    ServerRegistry* registry = NULL;
    int backlog = 0;
    int setegid_err = 0;
    int seteuid_err = 0;
    INT32 nUseRegistryForStats = 0;

    printf ("Starting %s %d.%d Core...\n", ServerVersion::ProductName(),
            ServerVersion::MajorVersion(), ServerVersion::MinorVersion());

    proc->pc = new CoreContainer(proc);
    proc->pc->process_type = PTCore;

    proc->pc->dispatchq = dispatch_queue;
    proc->pc->dispatchq->init(proc);

    MulticastAccessControl* mcast_ctrl = new MulticastAccessControl();

    /*
     * Must create the ErrorSinkHandler before accessing Config
     */

    proc->pc->error_sink_handler = new ErrorSinkHandler();

    progname = GetProgName(argv[0]);
    os_init();
    registry = new ServerRegistry(proc);
    proc->pc->registry = registry;
    registry->AddComp("Client", proc);
    registry->AddComp("Server", proc);

    // Add system composite to the registry
    registry->AddComp("system", proc);

    // Add startuplog info to the registry
    ServerBuffer* pBuf = new ServerBuffer((UCHAR*)g_szStartupLog, strlen(g_szStartupLog));
    if (g_szStartupLog)
        registry->AddStr("Server.StartupLogPath", pBuf, proc);

    config = new Config(proc, g_pszConfigFile, g_pszImportKey, registry);
    if (!config->valid())
    {
        ERRMSG(proc->pc->error_handler, "Invalid configuration.\n");
        terminate(1);
    }

    if (g_pszImportKey && g_pszImportKey[0])
    {
        terminate(1);
    }

    //Load user agent settings from .uas files into the registry
    UASConfig* pUasConfig = new UASConfig(proc);
    pUasConfig->AddRef();
    pUasConfig->Load();
    proc->pc->uasconfig = pUasConfig;

    proc->pc->client_stats_manager = new ClientStatsManager;
    //proc->pc->client_stats_manager->AddRef();

    if (SUCCEEDED(registry->GetInt("config.ClientStats.UseRegistry",
                                            &nUseRegistryForStats,
                                            proc)))
    {
        proc->pc->client_stats_manager->SetUseRegistryForStats(nUseRegistryForStats ? TRUE : FALSE);
    }



    /*
     * Now that the proc->pc and the registry are established, it is safe to
     * initialize the IHXNetworkServicesContext
     */

    proc->pc->network_services->Init(proc->pc->server_context,
                                     proc->pc->engine,
                                     NULL);

    proc->pc->net_services->Init(proc->pc->server_context);
    // MulticastManager needs MulticastAddressPool!
    proc->pc->mcst_addr_pool    = new MulticastAddressPool();
    proc->pc->mcst_addr_pool->AddRef();

    /*
    *   CSapManager and MulticastManager both needs
    *   plugin hander which will be avilable in
    *   _main CoreTransferCallback::func
    */
    // will be Init'ed in _main CoreTransferCallback::func
    proc->pc->sap_mgr = new CSapManager();
    proc->pc->sap_mgr->AddRef();

    // addtional Init will be done in _main CoreTransferCallback::func
    proc->pc->multicast_mgr = new MulticastManager(proc->pc->server_context);

    /*
     */
    proc->pc->alt_server_proxy_cfg_mgr =
        new AltServerProxyConfigHandler(proc, registry);
    proc->pc->alt_server_proxy_cfg_mgr->AddRef();
    if (HXR_FAIL == proc->pc->alt_server_proxy_cfg_mgr->Init())
    {
        HX_RELEASE(proc->pc->alt_server_proxy_cfg_mgr);
    }

    /* setup QoS core */
    proc->pc->qos_prof_select = new QoSProfileSelector(proc);
    proc->pc->qos_prof_select->AddRef();
    proc->pc->qos_bus_ctl     = new QoSSignalBusController();
    proc->pc->qos_bus_ctl->AddRef();
    proc->pc->qos_sig_src     = new QoSSignalSource(proc, proc->pc->qos_bus_ctl);
    proc->pc->qos_sig_src->AddRef();

    /* setup capex profile cache */
    proc->pc->capex_static_cache = new StaticProfileCache();
    proc->pc->capex_static_cache->AddRef();
    proc->pc->capex_profile_cache = new HTTPProfileCache();
    proc->pc->capex_profile_cache->AddRef();

    Config_error* error_result = 0;

    backlog = config->GetInt(proc, "config.ListenBacklog");
    if (backlog < CONFIG_BACKLOG)
        backlog = CONFIG_BACKLOG;

#ifdef  DEBUG
    debug_level() = g_ulDebugFlags ? g_ulDebugFlags :
                  config->GetInt(proc, "config.Debug");
    debug_func_level() = g_ulDebugFuncFlags ? g_ulDebugFuncFlags :
                       config->GetInt(proc, "config.DebugFunc");
#endif

    ((CoreContainer*)proc->pc)->m_pListenResponseList = new CHXSimpleList;

    HXRegistry* hxreg = new HXRegistry(registry, proc);
    HX_RESULT res = HXR_OK;

    hxreg->AddRef();

    char**      pHTTPablePaths = NULL;
    char**      pHTTPpostablePaths = NULL;

    ::GetHTTPPaths(hxreg, (char*)"config.HTTPDeliverable", pHTTPablePaths);
    ::GetHTTPPaths(hxreg, (char*)"config.HTTPPostable", pHTTPpostablePaths);

    /*
     * Add key for broadcast plugins to register their connections
     */
    hxreg->AddComp("LiveConnections");
    hxreg->AddInt("LiveConnections.Index", 0);
    hxreg->AddInt("LiveConnections.Count", 0);

    proc->pc->named_lock_manager->CreateNamedLock("LiveConnectionsLock");

    INT32* pEncoders = new INT32;
    IHXBuffer* pIHXBuf = new ServerBuffer(TRUE);
    HX_ASSERT(pIHXBuf);

    *pEncoders = 0;
    pIHXBuf->SetSize(sizeof(void*));
    *((void**)pIHXBuf->GetBuffer()) = (void*)pEncoders;
    hxreg->AddBuf("EncoderConnections.Index", pIHXBuf);
    HX_RELEASE(pIHXBuf);

    /*
     * Add key for monitor plugins to register their connections
     */
    hxreg->AddComp("Monitors");
    hxreg->AddInt("Monitors.Count", 0);

    /*
     * Add key for splitters to register their connections
     */
    hxreg->AddComp("Splitters");
    hxreg->AddInt("Splitters.Index", 0);
    hxreg->AddInt("Splitters.Count", 0);

    mcast_ctrl->Init(proc);
    proc->pc->mcast_ctrl        = mcast_ctrl;

    IHXValues* pAddrList = NULL;
    IHXBuffer* pAddrBuf = NULL;
    const char* pAddrStr = NULL;
    BOOL bBindToLocalHost = FALSE;

    TimeZoneCheckCallback* tzcb = new TimeZoneCheckCallback(proc);
    tzcb->SetTimeZone();
    tzcb->ScheduleFirstCheckCallback();

    HX_RESULT pn_res = hxreg->GetPropListByName("config.IPBindings", pAddrList);
    if (pn_res != HXR_OK)
    {
        pn_res = hxreg->GetPropListByName("config.IPBinding", pAddrList);
    }

    INT32 sbind_id = hxreg->AddComp("server.ipbinding");
    HX_ASSERT(sbind_id);
    int num_props = 0;

    switch (pn_res)
    {
        case HXR_OK:
        {
            const char* addr_prop_name;
            UINT32      addr_prop_id;

            res = pAddrList->GetFirstPropertyULONG32(addr_prop_name,
                addr_prop_id);
            while(res == HXR_OK)
            {
                char str[64];

                if (HXR_OK == hxreg->GetStrById(addr_prop_id, pAddrBuf))
                {
                    const char* strAddr = (const char*)pAddrBuf->GetBuffer();
                    if (!strcmp(strAddr, "*"))
                    {
                        //XXXJJ "*" means any interfaces(including ipv4 and ipv6)
                        // will be handled in "default:".

                        //if we have "*", we shouldn't have other entries in the list.
                        HX_ASSERT(num_props == 0);
                        break;
                    }
                    else if (!strcmp(strAddr, "127.0.0.1") || !strcmp(strAddr, "0.0.0.0"))
                    {
                        bBindToLocalHost = TRUE;
                    }

                    ++num_props;
                    sprintf(str, "server.ipbinding.addr_%.2d", num_props);
                    hxreg->AddStr(str, pAddrBuf);

                    HX_RELEASE(pAddrBuf);
                }
                res = pAddrList->GetNextPropertyULONG32(addr_prop_name,
                                                        addr_prop_id);
            }
            pAddrList->Release();

            if (num_props)
                break;
        }

        // Three cases fall into here: no ipbinding list, empty list, or only "*"
        // in the list
        default:
        {
            ServerBuffer::FromCharArray("*", &pAddrBuf);
            hxreg->AddStr("server.ipbinding.addr_01", pAddrBuf);
            pAddrBuf->Release();

            bBindToLocalHost = TRUE;
            break;
        }
    };

    if(!g_pHeartBeatIP) 
    {
        // heartbeat ip not specified
        if(!bBindToLocalHost)
        {
            //localhost is not in the binding list, we need to manually add it
            // for heartbeat connection to succeed.
            char str[64];
            ++num_props;
            sprintf(str, "server.ipbinding.addr_%.2d", num_props);
            ServerBuffer::FromCharArray("127.0.0.1", &pAddrBuf);
            hxreg->AddStr(str, pAddrBuf);
            pAddrBuf->Release();
        }

    }
    
    _initializeListenRespObjects(proc, config, registry, backlog);

    hxreg->Release();

    // This used to set g_pCPUCount based on config.ProcessorCount but
    // setting StreamerCount is the way to do this now.
    *g_pCPUCount = g_bSkipCPUTest ? 1 : CPUDetect();


#ifdef _UNIX
    const char* pPIDPath;
    if ((pPIDPath = config->GetString(proc, "config.PidPath")))
    {
        FILE* f = fopen(pPIDPath, "w");
        if (f > 0)
        {
#if defined PTHREADS_SUPPORTED
            fprintf(f, "%d\n", getpid());
#else
            fprintf(f, "%d\n", proc->procid(PROC_RM_CONTROLLER));
#endif
            fclose(f);
        }
        else
        {
            ERRMSG(proc->pc->error_handler,
                   "Couldn't open PID File %s", pPIDPath);
        }
    }

    int gid = GetGIDFromConfig(proc, config);
    int uid = GetUIDFromConfig(proc, config);

    if (pPIDPath && gid >= 0 && uid >= 0)
    {
        if (chown(pPIDPath, uid, gid) < 0)
            perror("could not set the PIDPath's ownership\n");
    }
    if (gid >= 0)
    {
#ifdef _AIX
        if (setregid(-1, gid) < 0)
#elif defined _HPUX
         if (setresgid(-1, gid, gid) < 0)
#else
        if (setegid(gid) < 0)
#endif
        {
            setegid_err = errno;
            perror("setegid() failed(1)");
            *return_gid = (UINT32)-1;
        }
        else
            *return_gid = gid;
    }
    if (uid >= 0)
    {
#if defined _AIX || defined _HPUX
        if (setreuid(-1, uid) < 0)
#else
        if (seteuid(uid) < 0)
#endif
        {
            seteuid_err = errno;
            perror("seteuid() failed(1)");
            *return_uid = (UINT32)-1;
        }
        else
        {
            *return_uid = uid;
        }
    }
    fflush(0);
#endif

    proc->pc->config            = config;

    /*
     * Handle streamer_info creation and overriding of capacity defaults
     * from the config file.
     */
    UINT32 ul;
    proc->pc->streamer_info     = new StreamerInfo;
    if (HXR_OK == proc->pc->registry->GetInt("config.StreamerSessionCapacity",
        (INT32*)&ul, proc))
    {
        proc->pc->streamer_info->SetSessionCapacity(ul);
    }

    if (HXR_OK == proc->pc->registry->GetInt("config.MaxSockCapacity",
        (INT32*)&ul, proc))
    {
        SOCK_CAPACITY_VALUE = ul;
    }

    if ((HXR_OK == proc->pc->registry->GetInt("config.MaxDescCapacity",
        (INT32*)&ul, proc)) ||
        (HXR_OK == proc->pc->registry->GetInt("config.MaxDescriptorCapacity",
        (INT32*)&ul, proc)))
    {
        DESCRIPTOR_CAPACITY_VALUE = ul;
    }

    proc->pc->conn_id_table     = new CHXTSID(config->GetInt(proc, "config.Capacity"));
    proc->pc->resolver_info     = new ResolverInfo;
    proc->pc->rdispatch         = new ResolverDispatch(proc);
    proc->pc->scheduler         = new ServerScheduler(proc);
    proc->pc->server_prefs      = new ServerPreferences(proc);
    proc->pc->server_info       = new ServerInfo(proc);
    proc->pc->loadinfo          = new LoadInfo(proc);

    //XXXTDM: Where's all the AddRef() calls???
    proc->pc->net_services->AddRef();
    proc->pc->scheduler->AddRef();

    // Tell mem routines where to find a regularly-updated timestamp in shared memory

    SharedMemory::SetTimePtr(&proc->pc->engine->now);

    // Set the SingleMaxMemoryAllocation from config.
    // Default to 64 MB, if not configured or if the
    // configured value is less than 64 MB.
    INT32 nTmp;

    if (FAILED(registry->GetInt("config.SingleMaxMemoryAllocation", &nTmp, proc)) ||
        nTmp < SINGLE_MAX_MEMORY_ALLOCATION)
    {
        nTmp = SINGLE_MAX_MEMORY_ALLOCATION;
    }

    SharedMemory::SetSingleMaxAllocation(nTmp);

#ifdef PAULM_LEAKCHECK
    if (g_bLeakCheck)
    {
        proc->pc->registry->AddInt("server.LeakCheckEnabled", 1, proc);
        new MemChecker(proc);
    }
#endif /* PAULM_LEAKCHECK */

    proc->pc->misc_plugins      = new CHXMapPtrToPtr();
    proc->pc->allowance_plugins = new CHXMapPtrToPtr();
    proc->pc->server_fork       = new ServerFork(proc);
    proc->pc->global_server_control = new GlobalServerControl(proc);
    proc->pc->data_convert_con  = new DataConvertController;

#if defined _UNIX
    proc->pc->m_pResMUX         = new ResolverMUX(RESOLVER_CAPACITY_VALUE,
                                        MAX_RESOLVERS, proc->pc->server_fork,
                                        proc->pc->async_io, proc->pc->error_handler);
#endif

    proc->pc->HTTP_deliver_paths = new char**;
    if (pHTTPablePaths)
    {
        *proc->pc->HTTP_deliver_paths= pHTTPablePaths;
    }
    else
    {
        *proc->pc->HTTP_deliver_paths = NULL;
    }
    new HTTPDeliverablePrefController(proc, (char*)"config.HTTPDeliverable",
                                      proc->pc->HTTP_deliver_paths);

    proc->pc->HTTP_postable_paths = new char**;
    if (pHTTPpostablePaths)
    {
        *proc->pc->HTTP_postable_paths= pHTTPpostablePaths;
    }
    else
    {
        *proc->pc->HTTP_postable_paths = NULL;
    }
    new HTTPDeliverablePrefController(proc, (char*)"config.HTTPPostable",
                                      proc->pc->HTTP_postable_paths);

    proc->pc->cloaked_guid_dict = new CHXThreadSafeMap();
    proc->pc->broadcast_manager = new BroadcastManager();
    proc->pc->load_listen_mgr   = new LoadBalancedListenerManager(proc);
    proc->pc->fcs_session_map   = new CHXThreadSafeMap();
    proc->pc->sspl_session_map  = new CHXThreadSafeMap();

    /*
     * Setup the global mimetype dictionary.
     */
    proc->pc->mime_type_dict = new Dict();
    hxreg = new HXRegistry(proc->pc->registry, proc);
    hxreg->AddRef();
    IHXValues* pTypeList;

    if(HXR_OK == hxreg->GetPropListByName("config.MimeTypes", pTypeList))
    {
        HX_RESULT res;
        const char* mimeTypeRegname;
        UINT32 mime_id;
        res = pTypeList->GetFirstPropertyULONG32(mimeTypeRegname, mime_id);
        while(HXR_OK == res)
        {
            HXPropType mimetypetype = hxreg->GetTypeById(mime_id);
            if(mimetypetype != PT_COMPOSITE)
                res = HXR_FAIL;
            else
            {
                const char* mimeType = strrchr(mimeTypeRegname, '.');
                if(!mimeType)
                    mimeType = mimeTypeRegname;
                else
                    mimeType++;

                IHXValues* pExtList;
                if(HXR_OK == hxreg->GetPropListById(mime_id, pExtList))
                {
                    const char* ext;
                    UINT32 ext_id;
                    res = pExtList->GetFirstPropertyULONG32(ext, ext_id);
                    while(res == HXR_OK)
                    {
                        if(PT_STRING == hxreg->GetTypeById(ext_id) ||
                           PT_BUFFER == hxreg->GetTypeById(ext_id))
                        {
                            IHXBuffer* extBuffer;
                            if(HXR_OK == hxreg->GetStrById(ext_id,
                                                           extBuffer))
                            {
                                proc->pc->mime_type_dict->enter(
                                    (const char*)extBuffer->GetBuffer(),
                                    new_string(mimeType));
                                HX_RELEASE(extBuffer);
                            }
                        }
                        res = pExtList->GetNextPropertyULONG32(ext, ext_id);
                    }
                    HX_RELEASE(pExtList);
                }
                res = pTypeList->GetNextPropertyULONG32(mimeTypeRegname,
                    mime_id);
            }
        }
        HX_RELEASE(pTypeList);
    }
    HX_RELEASE(hxreg);

    proc->pc->rtspstats_manager = new RTSPEventsManager();
    proc->pc->sdpstats_manager = new SDPAggregateStats();
    proc->pc->sdpstats_manager->AddRef();

    // If a default mime type doesn't yet exist, add one so that
    // we will always send some mime type
    Dict_entry* ent = proc->pc->mime_type_dict->find("*");
    if(!ent)
    {
        proc->pc->mime_type_dict->enter("*", new_string("application/octet-stream"));
    }
    new MimeTypesPrefController(proc);

    ((CoreContainer *)proc->pc)->cdispatch = new ConnDispatch(proc);

    /*
     * AddRef() all objects handed out by ServerContext
     */

    proc->pc->server_prefs->AddRef();
    proc->pc->server_fork->AddRef();

    *core_proc = proc;
    *controller_waiting_on_core = 0; // Signal Controller to continue
    while (!(*controller_ready))
        microsleep(1); // Busy Wait for the Controller
    delete controller_ready;

    CoreTransferCallback* cb = new CoreTransferCallback;
    cb->core_proc           = proc;
#ifdef DEBUG
    cb->m_debug_level       = debug_level();
    cb->m_debug_func_level  = debug_func_level();
#endif

    int* volatile core_waiting_for_refresh;
    cb->core_waiting_for_refresh = new int;
    *cb->core_waiting_for_refresh = 1;
    core_waiting_for_refresh = cb->core_waiting_for_refresh;

#ifdef _WIN32
    MaybeNotifyServiceUp();
#endif

    proc->pc->dispatchq->send(proc, cb, PROC_RM_CONTROLLER);

    /*
     * Wait here for the controller to get the plugins loaded.
     */
    while (*core_waiting_for_refresh)
        microsleep(1);

    delete core_waiting_for_refresh;

    /*
     * NOTE: Both NT and mac have servers that run to completion in
     *       os_start()
     */
    os_start();

    fflush(0);

    // add the global var ptr into the registry, so that the mmap managers in
    // each process can then access and modify the var using atomic
    // operations. g_pMemoryMappedDataSize is used to report the amount of
    // mmapped data that is read via the local file system (smplfsys).
    registry->AddIntRef("server.MMappedDataSize",
    (INT32 *)g_pMemoryMappedDataSize, proc);

    proc->pc->engine->mainloop();

    // NEVER REACHED!
#if 0
    delete this;
#endif
    PANIC(("Internal Error cp/737\n"));
}

static void
ReportListenErrorAndExit()
{
    printf("Fatal error: cannot bind to requested interface(s).\n"  
    "Please consult the error log for more detailed information.\n");
    fflush(0);
    terminate(1);
    /* NOTREACHED */
    HX_ASSERT(FALSE);
}

/**
 * The bShareSamePort variable, if TRUE,  specifies that the process can share the same 
 * the same port value as another process. If this variable is FALSE, the function would 
 * report an error. Mainly introduced for SSPL, FCS and FileSys Port Sharing.
 */
void CoreProcessInitCallback::InitHTTPControlPort(char* szConfigString,
                                                  Process* proc,
                                                  Config* config,
                                                  ServerRegistry* registry,
                                                  HXBOOL bShareSamePort)
{
    IHXActivePropUser* pActiveUser = NULL;
    UINT32 uPort = config->GetInt(proc, szConfigString);

    if(uPort == 0)
    {
        //not found
        return;
    }

    HTTPListenResponse* pResponse = new HTTPListenResponse(proc);
    if (pResponse->Init(uPort) != HXR_OK)
    {
        if(bShareSamePort && (hx_lastsockerr() == SOCKERR_ADDRINUSE))
            return;

        ReportListenErrorAndExit();
    }

    if (HXR_OK == pResponse->QueryInterface(IID_IHXActivePropUser,
        (void**)&pActiveUser) && pActiveUser)
    {
        registry->SetAsActive(szConfigString, pActiveUser, proc);
        HX_RELEASE(pActiveUser);
    }
    pResponse->AddRef();
    ((CoreContainer*)proc->pc)->m_pListenResponseList->AddTail(pResponse);
}

void
CoreProcessInitCallback::_initializeListenRespObjects(
    Process* proc,
    Config* config,
    ServerRegistry* registry,
    int backlog)
{
    IHXActivePropUser* pActiveUser = NULL;
    UINT32 uPort;

    // We could make the config entries strings and use /etc/services -- TDM

    uPort = config->GetInt(proc, "config.RTSPPort");

    RTSPListenResponse* pRTSPResponse = new RTSPListenResponse(proc);
    if (pRTSPResponse->Init(uPort) != HXR_OK)
    {
        ReportListenErrorAndExit();
    }

    /* Set the rtsp port as active. */
    if (HXR_OK == pRTSPResponse->QueryInterface(IID_IHXActivePropUser,
        (void**)&pActiveUser) && pActiveUser)
    {
        registry->SetAsActive("config.RTSPPort", pActiveUser, proc);
        HX_RELEASE(pActiveUser);
    }
    pRTSPResponse->AddRef();
    ((CoreContainer*)proc->pc)->m_pListenResponseList->AddTail(pRTSPResponse);

    InitHTTPControlPort("config.HTTPPort", proc, config, registry, FALSE);

#if defined(HELIX_FEATURE_SERVER_FCS)
    //ChannelControlPort for FCS.
    InitHTTPControlPort("config.ChannelControlPort", proc, config, registry, TRUE);
#endif //HELIX_FEATURE_SERVER_FCS

#if defined(HELIX_FEATURE_SERVER_SSPL)
    //sspl control port
    InitHTTPControlPort("config.PlaylistControlPort", proc, config, registry, TRUE);
#endif //HELIX_FEATURE_SERVER_SSPL

#if defined(HELIX_FEATURE_SERVER_CONTENT_MGMT)
    //content management port
    InitHTTPControlPort("config.FileSystemControlPort", proc, config, registry, TRUE);
#endif //HELIX_FEATURE_SERVER_CONTENT_MGMT

    /*
     *  If they specified an AdminPort, then we need to open that too.
     */
    InitHTTPControlPort("config.AdminPort", proc, config, registry, FALSE);

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    uPort = config->GetInt(proc, "config.MMSPort");

    MMSListenResponse* pMMSResponse = new MMSListenResponse(proc);
    if (pMMSResponse->Init(uPort) != HXR_OK)
    {
        ReportListenErrorAndExit();
    }

    proc->pc->mmsResendManager = new CMMSResendManager();
    proc->pc->mmsResendManager->AddRef();
    proc->pc->mmsResendManager->Init(proc, uPort);

    /* Set the MMS port as active. */
    if (HXR_OK == pMMSResponse->QueryInterface(IID_IHXActivePropUser,
        (void**)&pActiveUser) && pActiveUser)
    {
        registry->SetAsActive("config.MMSPort", pActiveUser, proc);
        HX_RELEASE(pActiveUser);
    }
    pMMSResponse->AddRef();
    ((CoreContainer*)proc->pc)->m_pListenResponseList->AddTail(pMMSResponse);
#endif
}

void
CoreSystemReadyCallback::func(Process* proc)
{
    DPRINTF(D_INFO, ("%s Ready - Accepting Connections\n",
                     ServerVersion::ProductName()));

#if !defined WIN32
#ifdef SHARED_FD_SUPPORT
    if (!g_bSharedDescriptors)
#endif /* SHARED_FD_SUPPORT */
    {
        ((CoreContainer *)proc->pc)->fdp_socket = fdp;
    }
#endif

    delete this;
}

void
CoreInvokeAcceptorCallback::func(Process* proc)
{
    acceptor->m_pCDispatch->process();

    delete this;
}

void
CoreFDPassSocketCallback::func(Process* proc)
{
#if !defined WIN32
#ifdef SHARED_FD_SUPPORT
    if (!g_bSharedDescriptors)
#endif
    {
        int fd = ((CoreContainer *)proc->pc)->fdp_socket->Recv(PROC_RM_CORE);
        ASSERT (fd > 0);

        fdp->SetSocket0(fd);
        ((CoreContainer *)proc->pc)->m_fdps[streamer_num] = fdp;
    }
#endif

    if (m_pAllowSendMutex)
        *m_pAllowSendMutex = FALSE;

    if (m_bNeedEnableStreamer)
    {
        proc->pc->streamer_info->EnableStreamer(streamer_num);
    }

        /* For Streamers */
    if (m_pLBLConnDispatch)
    {
        m_pLBLConnDispatch->process();
    }
    else
    {
        ((CoreContainer *)proc->pc)->cdispatch->process();
    }

    delete this;
}

STDMETHODIMP
CorePassSockCallback::Func()
{
    ((CoreContainer*)m_pProc->pc)->cdispatch->Send(m_pSock, m_iNewProc);

    return HXR_OK;
}

#ifdef PAULM_LEAKCHECK

MemChecker::MemChecker(Process* pProc)
{
    m_pProc = pProc;
    m_state = ALLOC_OFF;
    m_bOnce = FALSE;

    /* 1/2 hour */
    m_pProc->pc->engine->schedule.enter(m_pProc->pc->engine->now
        + Timeval((double)(g_ulLeakCheckFirst * 60)), this);
}

STDMETHODIMP
MemChecker::Func()
{
    switch (m_state)
    {
        case ALLOC_OFF:
            if (m_bOnce)
            {
                SharedMemory::checkleaks();
                SharedMemory::resetleaks();
            }
            else
            {
                SharedMemory::setogredebug(TRUE);
                SharedMemory::resetleaks();
            }
            m_bOnce = TRUE;
            m_state = ALLOC_ON;
            /* 20 mins */
            m_pProc->pc->engine->schedule.enter(m_pProc->pc->engine->now
                + Timeval((double)(g_ulLeakCheckAlloc * 60)), this);
            break;

        case ALLOC_ON:
            m_state = ALLOC_OFF;
            /* 40 mins */
            m_pProc->pc->engine->schedule.enter(m_pProc->pc->engine->now
                + Timeval((double)(g_ulLeakCheckFree * 60)), this);
            SharedMemory::suspendleaks();
            break;
    }

    return HXR_OK;
}
#endif /* PAULM_LEAKCHECK */
