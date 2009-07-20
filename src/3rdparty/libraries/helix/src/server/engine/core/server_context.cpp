/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: server_context.cpp,v 1.22 2007/09/28 22:28:05 atin Exp $
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
#include "hxcomm.h"
#include "hxerror.h"
#include "hxstring.h"
#include "hxplugn.h"
#include "hxcfg.h"
#include "hxengin.h"
#include "hxronly.h"
#include "intrpm.h"
#include "hxdataf.h"
#include "datffact.h"
#include "hxqossig.h"
#include "hxqos.h"

#include "servreg.h"
#include "servsked.h"
#include "server_prefs.h"
#include "server_fork.h"
#include "iasyncio.h"

#include "proc.h"

#include "server_engine.h"
#include "server_process.h"
#include "server_context.h"
#include "server_inetwork.h"
#include "hxservinfo.h"
#include "server_info.h"
#include "hxnet.h"
#include "servsockimp.h"
#include "server_control.h"
#include "server_version_wrapper.h"
#include "errhand.h"
#include "error_sink_ctrl.h"
#include "error_sink_handler.h"
#include "xmlconfig.h"

#include "servreg.h"
#include "hxreg.h"
#include "hxpropw.h"
#include "hxspriv.h"
#include "streamer_info.h"
#include "imalloc.h"
#include "server_intrpm.h"
#include "sapclass.h"       // IHXSapManager
#include "mcastaddr_mgr.h"  // MulticastAddressPool
#include "hxrdonly.h"
#include "cdist_wrappers.h"
#include "altserverproxy.h"
#include "_main.h"
#include "dispatchq.h"
#include "stktrc.h"
#include "mem_services.h"
#include "rtspstats.h" //IHXRTSPAggregateEventStats
#include "sdpstats.h" //IHXSDPAggregateStats
#include "hxstats.h" //IHXClientStatsManager
#include "server_stats.h"
#include "logoutputs.h"
#include "rssmgr.h"
#include "qos_prof_mgr.h"
#include "qos_prof_conf.h"
#include "qos_sig_bus_ctl.h"
#include "qos_diffserv_cfg.h"
#include "namedlock.h"
#include "pkt_loss_discrim_alg.h"

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
#include "mmsrsend.h"
#endif

ServerContext::ServerContext(Process* pProc, MemCache* pCache)
{
    m_proc = pProc;
    m_ulRefCount = 1;
    m_pRegistry = NULL;
    m_pMalloc = new IMallocContext(pCache);
    m_pMalloc->AddRef(); // Don't ever delete me!
    m_pIPM = new ServerInterPluginMessenger(m_proc, pCache);
}

ServerContext::~ServerContext()
{
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pMalloc);
    HX_RELEASE(m_pIPM);
}

STDMETHODIMP
ServerContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXCommonClassFactory))
    {
        m_proc->pc->common_class_factory->AddRef();
        *ppvObj = (IHXCommonClassFactory*) m_proc->pc->common_class_factory;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSClassFactory))
    {
        m_proc->pc->qos_class_factory->AddRef();
        *ppvObj = (IHXQoSClassFactory*) m_proc->pc->qos_class_factory;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRegistry) ||
             IsEqualIID(riid, IID_IHXRegistry2) ||
             IsEqualIID(riid, IID_IHXActiveRegistry) ||
             IsEqualIID(riid, IID_IHXRegistryAltStringHandling))
    {
        // XXXSMP Why do we create this here?
        if (!m_pRegistry)
        {
            m_pRegistry = new HXRegistry(m_proc->pc->registry, m_proc);
            m_pRegistry->AddRef();
        }

        return m_pRegistry->QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(riid, IID_IHXInternalSetPropReadOnly))
    {
        *ppvObj = (IHXInternalSetPropReadOnly*) new HXISetPropReadOnly(
            m_proc->pc->registry, m_proc);
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXScheduler))
    {
        m_proc->pc->scheduler->AddRef();
        *ppvObj = (IHXScheduler*) m_proc->pc->scheduler;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeScheduler))
    {
       m_proc->pc->ischeduler->AddRef();
       *ppvObj = (IHXThreadSafeScheduler*) m_proc->pc->ischeduler;
       return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAccurateClock))
    {
        return m_proc->pc->scheduler->QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(riid, IID_IHXDescriptorRegistration))
    {
        return m_proc->pc->engine->QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(riid, IID_IHXErrorMessages))
    {
        m_proc->pc->error_handler->AddRef(); // XXXSMP - WTF?
        *ppvObj = (IHXErrorMessages*) m_proc->pc->error_handler;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPreferences))
    {
        *ppvObj = (IHXPreferences*) new ServerPreferences(m_proc);
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRSSManager))
    {
        // *ppvObj = (IHXRSSManager*) new HXRSSManager(m_proc->pc->rss_manager, m_proc);
        // ((IUnknown*)*ppvObj)->AddRef();
        m_proc->pc->rss_manager->AddRef();
        *ppvObj = (IHXRSSManager*) m_proc->pc->rss_manager;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetworkServices))
    {
        m_proc->pc->network_services->AddRef();
        *ppvObj = (IHXNetworkServices*) m_proc->pc->network_services;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetServices))
    {
        m_proc->pc->net_services->AddRef();
        *ppvObj = (IHXNetServices*) m_proc->pc->net_services;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerInfo))
    {
        IHXServerInfo* pServerInfo = new ServerInfoFacade (m_proc, m_proc->pc->server_info);
        pServerInfo->AddRef();
        *ppvObj = pServerInfo;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPluginHandler))
    {
        //XXXBAB bad way to get to plugin handler
        m_proc->pc->plugin_handler->AddRef();
        *ppvObj = (void*)m_proc->pc->plugin_handler;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXGetRecursionLevel))
    {
        AddRef();
        *ppvObj = (IHXGetRecursionLevel*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IMalloc))
    {
        m_pMalloc->AddRef();
        *ppvObj = (IMalloc*) m_pMalloc;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFastAlloc))
    {
        m_pMalloc->AddRef();
        *ppvObj = (IHXFastAlloc*) m_pMalloc;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXErrorSinkControl))
    {
        m_proc->pc->error_sink_proc->AddRef();
        *ppvObj = (IHXErrorSinkControl*) m_proc->pc->error_sink_proc;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXInterPluginMessenger) ||
             IsEqualIID(riid, IID_IHXInterPluginMessenger2))
    {
        m_pIPM->AddRef();
        *ppvObj = m_pIPM;
        return HXR_OK;
    }
    /*
     * This will respond to IID_IHXPluginEnumerator
     */
    else if (m_proc->pc->plugin_handler &&
             HXR_OK == m_proc->pc->plugin_handler->QueryInterface(riid, ppvObj))
    {
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXConfigFile))
    {
        m_proc->pc->config_file->AddRef();
        *ppvObj = (IHXConfigFile*)m_proc->pc->config_file;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRegConfig))
    {
        ServerRegConfig* pRegCfg = new ServerRegConfig(m_proc);
        pRegCfg->AddRef();
        *ppvObj = (IHXRegConfig*)pRegCfg;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerControl))
    {
        m_proc->pc->server_control->AddRef();
        *ppvObj = (IHXServerControl*)m_proc->pc->server_control;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerControl2) ||
             IsEqualIID(riid, IID_IHXServerReconfigNotification))
    {
        return m_proc->pc->server_control->
            QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(riid, IID_IHXMulticastAddressPool))
    {
        m_proc->pc->mcst_addr_pool->AddRef();
        *ppvObj = (IHXMulticastAddressPool*)m_proc->pc->mcst_addr_pool;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSapManager))
    {
        m_proc->pc->sap_mgr->AddRef();
        *ppvObj = (IHXSapManager*)m_proc->pc->sap_mgr;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXDataFileFactory))
    {
        *ppvObj = new HXDataFileFactory();
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXProcess))
    {
        *ppvObj = new ServerHXProcess(m_proc);
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXContentDistributionAdvise))
    {
        *ppvObj = new CDistAdviseWrapper(this,
                                         m_proc);
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAlternateServerProxy) &&
             m_proc->pc->alt_server_proxy)
    {
        m_proc->pc->alt_server_proxy->AddRef();
        *ppvObj = (IHXAlternateServerProxy*)m_proc->pc->alt_server_proxy;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadLocal))
    {
        *ppvObj = new ThreadLocalInfo();
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }

#ifdef _UNIX
    else if (IsEqualIID(riid, IID_IHXServerFork))
    {
        m_proc->pc->server_fork->AddRef();
        *ppvObj = (IHXServerFork*)m_proc->pc->server_fork;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAsyncIOSelection))
    {
        m_proc->pc->async_io->AddRef();
        *ppvObj = (IHXAsyncIOSelection*)m_proc->pc->async_io;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXStackTrace))
    {
        *ppvObj = new StackTrace();
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }
#endif

    else if (IsEqualIID(riid, IID_IHXMemoryServices))
    {
        *ppvObj = new ServerMemoryServices();
        ((IUnknown*)*ppvObj)->AddRef();

        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXProductVersion))
    {
        *ppvObj = new ServerVersionWrapper();
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPEventsManager))
    {
        *ppvObj = new RTSPEventsManagerProcWrapper(m_proc->pc->rtspstats_manager,
                                                   m_proc);
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSDPAggregateStats))
    {
        m_proc->pc->sdpstats_manager->AddRef();
        *ppvObj = (IHXSDPAggregateStats*)m_proc->pc->sdpstats_manager;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXClientStatsManager))
    {
        ClientStatsManagerPerProcessWrapper* pWrapper = new ClientStatsManagerPerProcessWrapper(m_proc->pc->client_stats_manager,
                                                                                                m_proc);
        pWrapper->AddRef();
        *ppvObj = (IHXClientStatsManager*)pWrapper;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSProfileSelector))
    {
        m_proc->pc->qos_prof_select->AddRef();
        *ppvObj = (IHXQoSProfileSelector*)m_proc->pc->qos_prof_select;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSSignalSource))
    {
        m_proc->pc->qos_sig_src->AddRef();
        *ppvObj = (IHXQoSSignalSource*)m_proc->pc->qos_sig_src;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSDiffServConfigurator))
    {
        m_proc->pc->qos_diffserv_cfg->AddRef();
        *ppvObj = (IHXQoSDiffServConfigurator*)m_proc->pc->qos_diffserv_cfg;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNamedLock))
    {
        m_proc->pc->named_lock_manager->AddRef();
        *ppvObj = (IHXNamedLock*)m_proc->pc->named_lock_manager;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSProfileConfigurator))
    {
        QoSProfileConfigurator* pQoSProfCfg = new QoSProfileConfigurator((IUnknown*)this);
        HX_ADDREF(pQoSProfCfg);
        *ppvObj = pQoSProfCfg;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPacketLossDiscriminationAlgorithm))
    {
        *ppvObj = new CPacketLossDiscriminationAlgorithm((IUnknown *)this);
        ((IUnknown*)*ppvObj)->AddRef();
        return HXR_OK;
    }

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    else if (IsEqualIID(riid, IID_IHXMMSResendManager))
    {
        IHXMMSResendManager* pMMSResendManager =
            new MMSResendManagerFacade (m_proc, m_proc->pc->mmsResendManager);
        pMMSResendManager->AddRef();
        *ppvObj = pMMSResendManager;
        return HXR_OK;
    }
#endif

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerContext::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
ServerContext::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    ASSERT(FALSE);
    delete this;
    return 0;
}



ServerRegConfig::ServerRegConfig(Process* pProc)
    : m_ulRefCount(0)
    , m_pProc(pProc)
{
}

ServerRegConfig::~ServerRegConfig()
{
}

STDMETHODIMP
ServerRegConfig::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXRegConfig))
    {
        AddRef();
        *ppvObj = (IHXRegConfig*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerRegConfig::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
ServerRegConfig::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
ServerRegConfig::WriteKey(const char* pKeyName)
{
    WriteConfigCallback* cb = new WriteConfigCallback(pKeyName);
    cb->calling_procnum = m_pProc->procnum();
    m_pProc->pc->dispatchq->send(m_pProc, cb, PROC_RM_CORE);
    return HXR_OK;
}

