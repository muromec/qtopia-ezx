/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: load_balanced_listener.cpp,v 1.6 2008/07/03 21:54:18 dcollins Exp $
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

#include <stdio.h>
#include "hlxclib/signal.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "proc.h"
#include "hxengin.h"
#include "dispatchq.h"
#include "base_errmsg.h"
#include "server_engine.h"
#include "server_context.h"
#include "load_balanced_listener.h"
#include "acceptor.h"
#include "hxslist.h"
#include "lbl_cdispatch.h"
#include "netbyte.h"

LoadBalancedListenerManager::LoadBalancedListenerManager(Process* pCoreProc)
{
    m_pCoreProc = pCoreProc;
    m_pListenEntries = new CHXSimpleList;
}

LoadBalancedListenerManager::~LoadBalancedListenerManager()
{
    CHXSimpleList::Iterator     i, j;

    for (i = m_pListenEntries->Begin(); i != m_pListenEntries->End(); ++i)
    {
        ListenerEntry* pEntry = (ListenerEntry*) (*i);
        for (j  = pEntry->m_pListeners->Begin();
             j != pEntry->m_pListeners->End(); ++j)
        {
            ListeningProcessEntry* pPEntry = (ListeningProcessEntry*) (*j);
            HX_RELEASE(pPEntry->m_pListenResponse);
            delete pPEntry;
        }
        delete pEntry->m_pListeners;
        delete pEntry;
    }

    delete m_pListenEntries;

    PANIC(("Internal Error lb/30\n"));
}

void*
LoadBalancedListenerManager::AddListener(Process* pPluginProc,
                                         REFIID ID, UINT32 ulLocalAddr,
                                         UINT16 port,
                                         IHXListenResponse* pListenResponse,
                                         UINT32 ulReserveDescriptors,
                                         UINT32 ulReserveSockets)

{
    CHXSimpleList::Iterator i;
    ListenerEntry* pEntry = 0;
    ListenerEntry::CreateAcceptorCallback* cb = 0;

    for (i = m_pListenEntries->Begin(); i != m_pListenEntries->End(); ++i)
    {
        pEntry = (ListenerEntry*) (*i);

        if (IsEqualIID(ID, pEntry->m_ID))
        {
            goto GotAnEntry;
        }
    }

    /* Create a new entry and setup the listen socket. */
    pEntry                          = new ListenerEntry;
    pEntry->m_pListeners            = new CHXSimpleList;
    pEntry->m_ulLocalAddr           = ulLocalAddr;
    pEntry->m_unPort                = port;
#ifdef _UNIX
    /* On Unix, sockets are actually descriptors */
    pEntry->m_ulReserveDescriptors  = ulReserveDescriptors + ulReserveSockets;
    pEntry->m_ulReserveSockets      = 0;
#else
    pEntry->m_ulReserveDescriptors  = ulReserveDescriptors;
    pEntry->m_ulReserveSockets      = ulReserveSockets;
#endif

    SetGUID(pEntry->m_ID, ID);

    cb = new ListenerEntry::CreateAcceptorCallback;
    cb->m_pEntry = pEntry;
    pPluginProc->pc->dispatchq->send(pPluginProc, cb, PROC_RM_CORE);
    m_pListenEntries->AddTail((void *)pEntry);

GotAnEntry:

    if (pEntry->m_ulReserveDescriptors < ulReserveDescriptors)
    {
        pEntry->m_ulReserveDescriptors = ulReserveDescriptors;
    }

    if (pEntry->m_ulReserveSockets < ulReserveSockets)
    {
        pEntry->m_ulReserveSockets = ulReserveSockets;
    }

    ListeningProcessEntry* pPEntry = new ListeningProcessEntry;
    pPEntry->m_pPluginProc = pPluginProc;
    pPEntry->m_pListenResponse = pListenResponse;
    pPEntry->m_pListenResponse->AddRef();
    pEntry->m_pListeners->AddTail(pPEntry);

    /* Return a pointer to pPEntry as a Handle */
    return pPEntry;
}

void
LoadBalancedListenerManager::RemoveListener(Process* pProc, REFIID ID,
                                            void* pLoadBalancedListenerHandle)
{
    CHXSimpleList::Iterator i;
    ListenerEntry* pEntry = 0;

    for (i = m_pListenEntries->Begin(); i != m_pListenEntries->End(); ++i)
    {
        pEntry = (ListenerEntry*) (*i);

        if (IsEqualIID(ID, pEntry->m_ID))
        {
            goto GotAnEntry;
        }
    }

    PANIC(("Internal Error lb/124"));
    return;

GotAnEntry:

    ListeningProcessEntry* pPEntry = (ListeningProcessEntry*)
        pLoadBalancedListenerHandle;

    HX_RELEASE(pPEntry->m_pListenResponse);
    pEntry->m_pListeners->RemoveAt(
        pEntry->m_pListeners->Find(pLoadBalancedListenerHandle));
    delete pPEntry;

    /*
     * XXXSMP There is a possible race condition here where a process is
     * currently being created and we decide to destroy the LBLConnDispatch
     * object from under it.  This case is not handled, it's a 1 in billion
     * shot anyway :)
     */

    if (pEntry->m_pListeners->IsEmpty())
    {
        m_pListenEntries->RemoveAt(m_pListenEntries->Find((void *)pEntry));
        UINT32 ul;
        for (ul = 0; ul < pEntry->m_ulNumAcceptors; ul++)
        {
            pEntry->m_ppAcceptors[ul]->m_bDefunct = TRUE;
        }
        delete pEntry->m_pListeners;

        ListenerEntry::DestroyAcceptorCallback* cb =
                new ListenerEntry::DestroyAcceptorCallback;
        cb->m_pEntry = pEntry;
        pProc->pc->dispatchq->send(pProc, cb, PROC_RM_CORE);
    }
}


/* Runs in Core Proc */
void
LoadBalancedListenerManager::ListenerEntry::CreateAcceptorCallback::func(
    Process* pProc)
{
    /*
     * If the addr is HX_INADDR_IPBINDINGS then create
     * an acceptor for each ip in the list.
     */

    IHXRegistry* preg = 0;
    IHXValues* pValues = 0;
    UINT32 ulNumAcceptors = 0;
    const char* pName;
    UINT32 ul;
    HX_RESULT res;
    INT32 bBindToAll = FALSE;

    if (m_pEntry->m_ulLocalAddr == HX_INADDR_IPBINDINGS)
    {
        if (!pProc)
        {
            /*
             * Everyone has a proc!
             */
            return;
        }
        ((IUnknown*)pProc->pc->server_context)->QueryInterface(
            IID_IHXRegistry, (void**)&preg);
        if (preg)
        {
            preg->GetIntByName("server.BindToAll", bBindToAll);
            preg->GetPropListByName("server.IPBinding", pValues);
            if (pValues)
            {
                res = pValues->GetFirstPropertyULONG32(pName, ul);
                while (res == HXR_OK)
                {
                    ulNumAcceptors++;
                    res = pValues->GetNextPropertyULONG32(pName, ul);
                }
            }
        }
        if (bBindToAll || !ulNumAcceptors)
        {
            m_pEntry->m_ulLocalAddr = HXR_INADDR_ANY;
        }
    }

    if (!ulNumAcceptors || bBindToAll)
    {
        HX_RELEASE(preg);
        HX_RELEASE(pValues);
        m_pEntry->m_ulNumAcceptors = 1;
        m_pEntry->m_ppAcceptors = new LBLAcceptor*;
        m_pEntry->m_ppAcceptors[0] = new LBLAcceptor(pProc, m_pEntry);

        if (m_pEntry->m_ppAcceptors[0]->init(m_pEntry->m_ulLocalAddr,
            m_pEntry->m_unPort, 64) < 0) // XXXSMP BackLog?
        {
            ERRMSG(pProc->pc->error_handler,
                    "Could not open address(%s) and port(%d)\n",
                    HXInetNtoa(m_pEntry->m_ulLocalAddr), m_pEntry->m_unPort);
            delete m_pEntry->m_ppAcceptors[0];
            delete[] m_pEntry->m_ppAcceptors;
            m_pEntry->m_ppAcceptors = 0;
            m_pEntry->m_ulNumAcceptors = 0;
            return;
        }

        m_pEntry->m_ppAcceptors[0]->enable();

    }
    else
    {
        UINT32 addr;
        int gotone = 0;
        m_pEntry->m_ppAcceptors = new LBLAcceptor*[ulNumAcceptors];
        res = pValues->GetFirstPropertyULONG32(pName, ul);
        while (res == HXR_OK)
        {
            if (HXR_OK == preg->GetIntByName(pName, (INT32 &)addr))
            {
                m_pEntry->m_ulLocalAddr = ntohl(addr);
                m_pEntry->m_ppAcceptors[m_pEntry->m_ulNumAcceptors] = new
                    LBLAcceptor(pProc, m_pEntry);
                if (m_pEntry->m_ppAcceptors[m_pEntry->m_ulNumAcceptors]->init(
                    m_pEntry->m_ulLocalAddr,
                    m_pEntry->m_unPort, 64) < 0)
                {
                    in_addr addr;
                    addr.s_addr = m_pEntry->m_ulLocalAddr;
                    ERRMSG(pProc->pc->error_handler,
                            "Could not open address(%s) and port(%d)\n",
                            HXInetNtoa(m_pEntry->m_ulLocalAddr),
                            m_pEntry->m_unPort);
                    return;
                    delete m_pEntry->m_ppAcceptors[m_pEntry->m_ulNumAcceptors];
                }
                else
                {
                    m_pEntry->m_ppAcceptors[m_pEntry->m_ulNumAcceptors]->enable();
                    m_pEntry->m_ulNumAcceptors++;
                    gotone = 1;
                }
            }
            res = pValues->GetNextPropertyULONG32(pName, ul);
        }
        HX_RELEASE(preg);
        HX_RELEASE(pValues);
        if (!gotone)
        {
            delete[] m_pEntry->m_ppAcceptors;
            m_pEntry->m_ppAcceptors = 0;
            return;
        }

    }

    delete this;
}


void
LoadBalancedListenerManager::ListenerEntry::DestroyAcceptorCallback::func(
    Process* pProc)
{
    UINT32 ul;
    for (ul = 0; ul < m_pEntry->m_ulNumAcceptors; ul++)
    {
        delete m_pEntry->m_ppAcceptors[ul];
    }
    delete[] m_pEntry->m_ppAcceptors;
    delete m_pEntry;

    delete this;
}

/* Runs in Core Proc */
LBLAcceptor::LBLAcceptor(Process* pProc,
                         LoadBalancedListenerManager::ListenerEntry* _entry)
    : Acceptor(pProc)
{
    m_pEntry = _entry;
    m_pCDispatch = new LBLConnDispatch(pProc, this);
    m_bDefunct = FALSE;
}

LBLAcceptor::~LBLAcceptor()
{
    delete m_pCDispatch;
}


int
LBLAcceptor::GetBestProcess(REF(IHXListenResponse*) pResp,
                            REF(PluginHandler::Plugin*) pPlugin)
{
    CHXSimpleList::Iterator     i;
    INT32 ulBestScore = -1;
    LoadBalancedListenerManager::ListeningProcessEntry* pBestPEntry = 0;

    pPlugin = 0;

    for (i = m_pEntry->m_pListeners->Begin();
         i != m_pEntry->m_pListeners->End(); ++i)
    {
        LoadBalancedListenerManager::ListeningProcessEntry* pPEntry;
        pPEntry = (LoadBalancedListenerManager::ListeningProcessEntry*) (*i);
        if (!pPlugin)
        {
            pPlugin = ((MiscContainer*)
                (pPEntry->m_pPluginProc->pc))->m_plugin;
        }

        UINT32 ulDesc = pPEntry->m_pPluginProc->pc->engine->GetDescCapacity();
        UINT32 ulSock = pPEntry->m_pPluginProc->pc->engine->GetSockCapacity();
        INT32 ulScore  =  ulDesc + ulSock;

        if ((ulDesc < m_pEntry->m_ulReserveDescriptors) ||
            (ulSock < m_pEntry->m_ulReserveSockets))
        {
            ulScore = -1;
        }

        if (ulScore > ulBestScore)
        {
            ulBestScore = ulScore;
            pBestPEntry = pPEntry;
        }
    }

    if (pBestPEntry)
    {
        pResp = pBestPEntry->m_pListenResponse;
    }

    return pBestPEntry ? pBestPEntry->m_pPluginProc->procnum() : -1;
}


void
LBLAcceptor::Accepted(TCPIO* tcp_io, sockaddr_in peer, int peerlen, int port)
{
    if (m_bDefunct)
    {
        delete tcp_io;
        return;
    }

    IHXTCPSocketContext* pConn = new INetworkTCPSocketContext(engine, 0,
        tcp_io, DwToHost(peer.sin_addr.s_addr), WToHost(peer.sin_port));
    pConn->AddRef();

    m_pCDispatch->Send(pConn);
}
