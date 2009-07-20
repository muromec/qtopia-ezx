/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: config.cpp,v 1.11 2004/07/20 23:51:58 dcollins Exp $
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _UNIX
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif /* _UNIX */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "debug.h"
#include "hxstrutl.h"
#include "dict.h"
#include "chxpckts.h"
#include "fio.h"
#include "cfgerror.h"
#include "proc.h"
#include "base_errmsg.h"
#include "_main.h"
#include "servreg.h"
#include "hxmon.h"
#include "config.h"
#include "xmlregconfig.h"
#include "server_version.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

void
Config::SetDefaults(Process* proc)
{
    /*
     * Store the defaults in the registry.
     */
    IHXBuffer* pValue = NULL;

    m_registry->AddInt("configdefaults.Capacity",              10000,         proc);

    m_registry->AddInt("configdefaults.ClientConnections",      0,              proc);
    m_registry->AddInt("configdefaults.CloakingHint",           1,              proc);

    pValue = new CHXBuffer();
    pValue->AddRef();
    pValue->Set((const UCHAR*)"%-1", strlen("%-1")+1);
    m_registry->AddStr("configdefaults.Group",                  pValue,         proc);
    pValue->Release();

    m_registry->AddInt("configdefaults.HTTPPort",               8080,           proc);

    m_registry->AddInt("configdefaults.KeepAliveInterval",              80,             proc);

    m_registry->AddInt("configdefaults.LoggingStyle",           0,              proc);
    m_registry->AddInt("configdefaults.MaxBandwidth",           0,              proc);
    m_registry->AddInt("configdefaults.MinPlayerProtocol",      0,              proc);
    m_registry->AddInt("configdefaults.MonitorConnections",     4,              proc);

    m_registry->AddInt("configdefaults.Multicast.DeliveryOnly", 0,              proc);
    m_registry->AddInt("configdefaults.Multicast.PNAPort",      7070,           proc);
    m_registry->AddInt("configdefaults.Multicast.RTSPPort",     3554,           proc);
    m_registry->AddInt("configdefaults.Multicast.TTL",          16,             proc);
    m_registry->AddInt("configdefaults.Multicast.Resend",       1,              proc);

    pValue = new CHXBuffer();
    pValue->AddRef();
    char szTmpName[64];
    sprintf(szTmpName, "%s.pid", ServerVersion::ExecutableName());
    pValue->Set((const UCHAR*)szTmpName, strlen(szTmpName)+1);
    m_registry->AddStr("configdefaults.PidPath",                pValue,         proc);
    pValue->Release();

    m_registry->AddInt("configdefaults.PlusOnly",               0,              proc);
    m_registry->AddInt("configdefaults.RTSPPort",               554,            proc);
    m_registry->AddInt("configdefaults.MMSPort",                1755,           proc);
    m_registry->AddInt("configdefaults.RTSPMessageDebug",       0,              proc);
    m_registry->AddInt("configdefaults.StatsMask",              0,              proc);

    pValue = new CHXBuffer();
    pValue->AddRef();
    pValue->Set((const UCHAR*)"%-1", strlen("%-1")+1);
    m_registry->AddStr("configdefaults.User",                   pValue,         proc);
    pValue->Release();

    m_registry->AddInt("configdefaults.ValidPlayersOnly",       0,              proc);

#ifdef DEBUG
    m_registry->AddInt("configdefaults.Debug",                 0,               proc);
    m_registry->AddInt("configdefaults.DebugFunc",             0,               proc);
#endif

    /*
     * Now copy the defaults over the the config area.
     */
    m_registry->Copy("configdefaults", "config", proc);
}

Config::Config(Process* proc, char* file, char* key_name,
               ServerRegistry* registry)
{
    char* pRegistryKey = NULL;
    Config_error error_result;
    m_registry = registry;

    if (key_name && key_name[0])
    {
        pRegistryKey = key_name;
    }

    error_result = load(proc, file, pRegistryKey);
    if (error_result.code() != Config_error::NO_ERRORS)
    {
        ERRMSG(proc->pc->error_handler,
               "Could not load config file: %s\n", error_result.errorMsg());
        m_valid = FALSE;
    }
    else
    {
        SetDefaults(proc);
        m_valid = TRUE;
    }
}

Config::~Config()
{
}

Config_error
Config::load(Process* proc, char* file, char* key_name)
{
    Config_error    retval;
    HX_RESULT       hResult = HXR_OK;

    retval = Config_error(Config_error::NO_ERRORS);

    XMLServerConfig *xmlconf = new XMLServerConfig(proc, m_registry);
    xmlconf->AddRef();

    hResult = xmlconf->Read(file, key_name, (char*)"config");
    if (HXR_OK != hResult)
    {
        retval = Config_error(Config_error::INTERNAL_ERROR);
    }

    if (key_name && key_name[0])
    {
        if (hResult == HXR_OK)
        {
            fprintf(stderr, "\nSuccessfully imported \"%s\" contents to the\n"
                "\"HKEY_CLASSES_ROOT\\Software\\RealNetworks\\%s\\%d.%d\\%s\"\n"
                "key of the Windows Registry.\n\n"
                "You may now use the registry:%s parameter instead of a config\n"
                "file to run %s with this new Registry-based "
                "configuration.\n",
                file, key_name, ServerVersion::ProductName(), ServerVersion::MajorVersion(),
                ServerVersion::MinorVersion(), key_name, ServerVersion::ProductName());
        }
        else
        {
            fprintf(stderr, "\nERROR: Unable to import "
                "the configuration.  Please ensure that\n"
                "the configuration file \"%s\" is "
                "valid, and that the Windows\n"
                "Registry is writable.\n\n", file);
        }
    }

    proc->pc->config_file = xmlconf;

    return retval;
}

INT32
Config::GetInt(Process* proc, const char* name) const
{
    HX_RESULT   result;
    INT32       value = 0;

    result = m_registry->GetInt(name, &value, proc);
#if 0
    if (result != HXR_OK)
    {
        /*
         * Everyone should be asking for existing variables
         */
        ERRMSG(proc->pc->error_handler,
               "tried to lookup non-existant config variable %s\n", name);
        ASSERT(FALSE);
    }
#endif

    return value;
}

const char*
Config::GetString(Process* proc, const char* name) const
{
    IHXBuffer* pValue = NULL;
    HX_RESULT   result;
    char*       value;

    result = m_registry->GetStr(name, pValue, proc);
#if 0
    if (result != HXR_OK)
    {
        /*
         * Everyone should be asking for existing variables
         */
        ERRMSG(proc->pc->error_handler,
               "tried to lookup non-existant config variable %s\n", name);
        ASSERT(FALSE);
    }
#endif

    if (!pValue || result != HXR_OK)
    {
        return 0;
    }

    value = (char*)pValue->GetBuffer();
    pValue->Release();

    return value;
}

HX_RESULT
Config::GetComposite(Process* proc, const char* name,
                     IHXValues*& pValues) const
{
    return m_registry->GetPropList(name, pValues, proc);
}


