/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: misc_proc.cpp,v 1.13 2006/05/22 19:22:20 dcollins Exp $
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
#include "hxplugn.h"
#include "hxallow.h"

#ifdef _UNIX
#include "unix_misc.h"
#endif

#include "hxslist.h"
#include "hxstring.h"
#include "plgnhand.h"

#include "core_proc.h"
#include "proc.h"
#include "hxformt.h"
#include "misc_proc.h"
#include "regdb_misc.h"
#include "hxmon.h"
#include "hxreg.h"
#include "hxerror.h"
#include "error_sink_handler.h"
#include "server_context.h"
#include "servsockimp.h"
#include "server_inetwork.h"
#include "server_engine.h"
#include "server_inetwork.h"
#include "base_errmsg.h"
#include "hxmap.h"
#include "dispatchq.h"
#include "misc_plugin.h"
#include "bcastmgr.h"
#include "misc_container.h"
#include "core_proc.h"
#include "hxclientprofile.h"
#include "hxpssprofile.h"
#include "client_profile_mgr.h"

extern char g_szStartupLog[];
extern BOOL g_bUseStartupLog;
extern int* VOLATILE pMiscProcInitMutex;
extern BOOL g_bPrintProcessPIDs;

void
MiscProcessInitCallback::func(Process* proc)
{
    PluginHandler::Errors               plugin_result;
    IUnknown*                           instance;
    MiscProcessInitCallback::Errors     result = NO_ERRORS;
    HX_RESULT                           h_result = 0;
    IHXPlugin*                          plugin_interface;
    char                                temp[512];

    MiscContainer* pMCont;

    proc->pc = pMCont = new MiscContainer(proc, m_proc->pc);
    proc->pc->dispatchq->init(proc);
    proc->pc->process_type = PTMisc;

    pMCont->m_plugin = m_plugin;

    /*
     * Now that the proc->pc is established, it is safe to initialize the
     * IHXNetworkServicesContext
     */

    proc->pc->network_services->Init(proc->pc->server_context,
                                     proc->pc->engine, NULL);

    proc->pc->net_services->Init(proc->pc->server_context);

    if (g_bPrintProcessPIDs)
    {
        sprintf(temp,
                "Loading %s %d: %-.256s",
                m_plugin->m_load_multiple ? "MultiLoad" : "Non-MultiLoad",
                proc->procid(proc->procnum()),
                m_plugin->m_pszDescription);
        proc->pc->error_handler->Report(HXLOG_DEBUG, 0, 0, temp, 0);

        // Output to startup log...
        if (g_bUseStartupLog)
        {
            FILE *fp = fopen(g_szStartupLog, "a");
            if (fp)
            {
            fprintf(fp, "D: %s\n", temp);
            fclose(fp);
            }
        }
    }

    plugin_result = m_plugin->GetInstance(&instance);
    if (PluginHandler::NO_ERRORS != plugin_result)
    {
        result = BAD_PLUGIN;
        ERRMSG(proc->pc->error_handler, "Bad Misc plugin\n");
        m_plugin->ReleaseInstance();
        // XXXSMP Need Proper Exit Code!!
        *pMiscProcInitMutex = 0;
        exit(0);
    }

    if (NO_ERRORS == result)
    {
        h_result = instance->QueryInterface(IID_IHXPlugin,
                                            (void **)&plugin_interface);

        if (HXR_OK == h_result)
        {
            MiscProcInitPlug* pMisc = new MiscProcInitPlug(proc);

            plugin_interface->InitPlugin(proc->pc->server_context);

            pMisc->instance = instance;
            pMisc->instance->AddRef();
            pMisc->plugin_interface = plugin_interface;
            pMisc->pMPI = this;
            pMisc->m_pAcceptor = m_pAcceptor;

            pMCont->m_pInstance = (IUnknown*)plugin_interface;

            proc->pc->engine->schedule.enter(Timeval(0.0), pMisc);
        }
        else
        {
            printf ("Misc Plugin Failed to Init correctly\n");
        }
    }

    m_plugin->m_process = proc;

    m_plugin->ReleaseInstance();
    instance->Release();
    *pMiscProcInitMutex = 0;

    if (NO_ERRORS != result)
        return;

    proc->pc->engine->mainloop();
}

MiscProcessInitCallback::Errors
MiscProcessInitCallback::AttemptFileSystemStart(Process* proc,
                                                IUnknown* instance)
{
    HX_RESULT                   h_result;
    IHXFileSystemObject*        file_system = 0;
    FSPlugin*                   pFSPlugin = new FSPlugin;

    h_result = instance->QueryInterface(IID_IHXFileSystemObject,
                                        (void **)&file_system);
    if (HXR_OK != h_result)
    {
        return WRONG_INTERFACE;
    }

    pFSPlugin->m_pProc = proc;
    pFSPlugin->m_pFS   = file_system;

    proc->pc->misc_plugins->SetAt(m_plugin, (void *&)pFSPlugin);

    //XXXSMPm_plugin->m_process = proc;

    return NO_ERRORS;
}

MiscProcessInitCallback::Errors
MiscProcessInitCallback::AttemptBroadcastStart(Process* proc,
                                               IUnknown* instance)
{
    HX_RESULT                   ulResult;
    IHXFileSystemObject*        pFileSystem = 0;
    IHXBroadcastFormatObject*  pBroadcastObject = 0;
    BroadcastPlugin*            pBCPlugin = new BroadcastPlugin;

    ulResult = instance->QueryInterface(IID_IHXFileSystemObject,
                                        (void **)&pFileSystem);
    if (HXR_OK != ulResult)
    {
        return WRONG_INTERFACE;
    }

    ulResult = instance->QueryInterface(IID_IHXBroadcastFormatObject,
                                        (void**)&pBroadcastObject);
    if(HXR_OK != ulResult)
    {
        pFileSystem->Release();
        return WRONG_INTERFACE;
    }

    pBCPlugin->m_pProc      = proc;
    pBCPlugin->m_pFS        = pFileSystem;

    proc->pc->misc_plugins->SetAt(m_plugin, (void *&)pBCPlugin);

    proc->pc->plugin_handler->m_broadcast_handler->Add(m_plugin);

    m_plugin->m_process = proc;

    proc->pc->broadcast_manager->Register(instance, proc->procnum(), proc);

    pFileSystem->Release();
    pBroadcastObject->Release();

    return NO_ERRORS;
}

MiscProcessInitCallback::Errors
MiscProcessInitCallback::AttemptAllowanceStart(Process* proc,
                                               IUnknown* instance)
{
    HX_RESULT                           h_result;
    IHXPlayerConnectionAdviseSinkManager*
                                        allowance_plugin = 0;
    AllowancePlugin*                    pAllowancePlugin = new AllowancePlugin;

    h_result =
        instance->QueryInterface(IID_IHXPlayerConnectionAdviseSinkManager,
                                 (void **)&allowance_plugin);
    if (HXR_OK != h_result)
    {
        return WRONG_INTERFACE;
    }

    pAllowancePlugin->m_pProc = proc;
    pAllowancePlugin->m_pAdviseSinkManager = allowance_plugin;

    proc->pc->allowance_plugins->SetAt(m_plugin, (void *&)pAllowancePlugin);

    return NO_ERRORS;
}

MiscProcessInitCallback::Errors
MiscProcessInitCallback::AttemptPTAgentStart(Process* proc,
                                             IUnknown* instance)
{
    HX_RESULT h_status;
    IHXPSSPTAgent*  pPTAgent = NULL;
    PTAgentPlugin* pPTAgentPlgn = new PTAgentPlugin;

    h_status = instance->QueryInterface(IID_IHXPSSPTAgent, (void **)&pPTAgent);
    if (HXR_OK != h_status)
    {
        return WRONG_INTERFACE;
    }

    pPTAgentPlgn->m_pProc = proc;
    pPTAgentPlgn->m_pPTAgent = pPTAgent;
    proc->pc->misc_plugins->SetAt(m_plugin, (void *&)pPTAgentPlgn);
    proc->pc->client_profile_manager->InitGlobalCache(pPTAgent, proc);

    return NO_ERRORS;
}

MiscProcInitPlug::MiscProcInitPlug(Process* pProc)
{
    m_pProc = pProc;
}

STDMETHODIMP
MiscProcInitPlug::Func()
{
    /*
     * The call to InitPlugin() is now done before this callback is set.
     */
    // plugin_interface->InitPlugin(m_pProc->pc->server_context);

    // plugin_interface->Release();

    // XXX Do not release this plugin because if it only implements
    // IHXPlugin it will get destroyed.  Need to keep a list
    // of non-multiinstance plugins to be released on shutdown
    // When shutdown code gets added, uncomment this release

    pMPI->AttemptFileSystemStart(m_pProc, instance);
    pMPI->AttemptBroadcastStart(m_pProc, instance);
    pMPI->AttemptAllowanceStart(m_pProc, instance);
    pMPI->AttemptPTAgentStart(m_pProc, instance);

    instance->Release();
    return HXR_OK;
}
