/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: server_network_services.cpp,v 1.4 2004/06/02 17:30:30 tmarshall Exp $
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
#include "debug.h"
#include "base_errmsg.h"
#include "server_inetwork.h"
#include "server_engine.h"
#include "server_network_services.h"

#include "proc.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_RESERVE_DESCRIPTORS             4
#define DEFAULT_RESERVE_SOCKETS                 4

CServerNetworkServicesContext::CServerNetworkServicesContext(Process* proc) :
    m_proc(proc)
{
}

STDMETHODIMP
CServerNetworkServicesContext::CreateTCPSocket(IHXTCPSocket** ppTCPSocket)
{
    IHXResolver* pResolver;

    pResolver = new CServerResolverContext(m_proc);
    *ppTCPSocket = new INetworkTCPSocketContext(m_proc->pc->engine, pResolver);
    if (*ppTCPSocket == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppTCPSocket)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CServerNetworkServicesContext::CreateResolver(IHXResolver** ppResolver)
{
    *ppResolver = new CServerResolverContext(m_proc);
    if(*ppResolver == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppResolver)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CServerNetworkServicesContext::CreateListenSocket(IHXListenSocket**
    ppListenSocket)
{
    *ppListenSocket = new CServerListenSocketContext(m_proc, m_pEngine,
                                                     m_pMessages,
                                                     m_pAccessCtrl);
    if(*ppListenSocket == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    (*ppListenSocket)->AddRef();
    return HXR_OK;
}
