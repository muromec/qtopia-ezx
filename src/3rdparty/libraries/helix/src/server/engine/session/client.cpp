/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: client.cpp,v 1.47 2006/12/21 05:13:42 tknox Exp $
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

#ifdef _HPUX
#include <sys/time.h>
#endif
#include <time.h>
#ifdef _UNIX
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <errno.h>
#  include <stdio.h>
#  include <stdlib.h>
#endif // _UNIX

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "dict.h"
#include "debug.h"
#include "netbyte.h"
#include "sio.h"
#include "tcpio.h"
#include "callback_container.h"
#include "hxstring.h"
#include "hxslist.h"
#include "mimehead.h"
#include "cbqueue.h"
#include "rttp2.h"
#include "id.h"
#include "config.h"
#include "hxprot.h"
#include "chxpckts.h"
#include "rtspmsg.h"
#include "hxstats.h"
#include "server_stats.h"
#include "client.h"
#include "server_engine.h"
#include "server_info.h"
#include "shmem.h"
#include "streamer_container.h"
#include "defslice.h"
#include "loadinfo.h"
#include "errhand.h"
#include "dispatchq.h"
#include "clientguid.h"
#include "clientregtree.h"
#include "mutex.h"
#include "tsid.h"
#include "shutdown.h"

#include "hxtick.h"

#include "servbuffer.h"


#ifdef PAULM_CLIENTTIMING
#include "classtimer.h"
#endif

// max delay 1hr. hoping that the bad clients will disconnect by then
static const int MAX_DELAY_MS = 3600000;

INT32 Client::ClientDeleteCallback::zm_lRegDestructDelay = -1;
static const char* GUID_ENTRY_PERSISTENCE_STRING = "config.ClientGUIDPersistence";
static const INT32 GUID_ENTRY_PERSISTENCE_DEFAULT = 30;

#ifdef PAULM_CLIENTTIMING

void
_ExpiredClientReport(void* p)
{
    Client* c = (Client*)p;
    printf("\tstamp: %lu\n", c->stamp);
    if (c->proc->pc->engine)
        printf("\tnow: %lu\n", c->proc->pc->engine->now.tv_sec);
    else
        printf("\tnow: 0\n");
    printf("\tis_cloak: %d\n", c->is_cloak);
    if (c->is_cloak)
    {
        if (c->cloak_id)
        {
            printf("\tcloak_id: %s\n", c->cloak_id);
        }
    }
    printf("\tis_cloaked_get: %d\n", c->is_cloaked_get);
    printf("\tis_multiple_put: %d\n", c->is_multiple_put);
    printf("\tis_dummy: %d\n", c->is_dummy);
    printf("\tgot_read: %d\n", c->got_read);
    printf("\tdied_from_Alive: %d\n", c->died_from_Alive);
    printf("\tin_streamer: %d\n", c->in_streamer);
    printf("\tdied_from_timeout: %d\n", c->died_from_timeout);
    printf("\tm_pSock: 0x%x\n", c->m_pSock);
    printf("\tm_pProtocol: 0x%x\n", c->m_pProtocol);
    printf("\tprotocol: ");
    switch (c->m_protType)
    {
    case HXPROT_RTSP:
        printf("RTSP_PROT\n");
        break;
    case HXPROT_HTTP:
        printf("HTTP_PROT\n");
        break;
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    case HXPROT_MMS:
        printf("MMS_PROT\n");
        break;
#endif
    case HXPROT_UNKNOWN:
    default:
        printf("UNKNOWN_PROT\n");
        break;
    }
    printf("\tstate: ");
    switch (c->m_state)
    {
    case Client::ALIVE:
        printf("ALIVE\n");
        break;

    case Client::CLOSE_CB_QED:
        printf("CLOSE_CB_QED\n");
        break;

    case Client::DELETE_CB_QED:
        printf("DELETE_CB_QED\n");
        break;

    case Client::DEAD:
        printf("DEAD\n");
        break;

    default:
        printf("WHAT!\n");
        break;
    }
#ifdef PAULM_CLIENTAR
    c->DumpState();
#endif
}

ClassTimer g_ClientTimer("Client", _ExpiredClientReport, 3600);
#endif


void
Client::init(HXProtocolType type, HXProtocol* pProtocol)
{
    m_protType = type;
    m_pProtocol = pProtocol;
    m_pProtocol->AddRef();

    conn_id = proc->pc->conn_id_table->create((void*)1);
}

Client::Client(Process* p)
        : m_ulRefCount(0)
        , proc(p)
        , m_pProtocol(NULL)
        , m_state(ALIVE)
        , conn_id(0)
        , m_ulRegistryConnId(0)
        , m_uBytesSent(0)
        , m_protType(HXPROT_UNKNOWN)
        , m_clientType(UNKNOWN_CLIENT)
        , m_bIsAProxy(FALSE)
        , m_ulThreadSafeFlags(HX_THREADSAFE_METHOD_SOCKET_READDONE)
        , m_pStats(NULL)
        , m_pRegTree(NULL)
        , m_pClientGUIDEntry(NULL)
        , m_bNeedCountDecrement(FALSE)
        , m_pPlayerGUID(NULL)
        , m_bUseRegistryForStats(TRUE)
        , m_ulClientStatsObjId(0)
        , is_cloak(FALSE)
#if ENABLE_LATENCY_STATS
        , m_ulStartTime(0)
        , m_ulCorePassCBTime(0)
        , m_ulDispatchTime(0)
        , m_ulStreamerTime(0)
        , m_ulFirstReadTime(0)
        , m_pProtocol(0)
        , m_ulCloseStatus(HXR_OK)
#endif
{
    m_ulCreateTime = HX_GET_BETTERTICKCOUNT();

#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(m_pCtrl, 3);
#endif

#ifdef PAULM_CLIENTAR
    ADDR_NOTIFY(this, 1);
#endif
    AddRef();

    UINT32 ul = 0;
    if (HXR_OK == proc->pc->registry->GetInt(REGISTRY_RTSPPROXY_ENABLED,
    (INT32*)&ul, proc))
    {
        m_bIsAProxy = TRUE;
    }

    m_bUseRegistryForStats = proc->pc->client_stats_manager->UseRegistryForStats();

    //XXXTDM: is this useful?
    INT32 nVal=0;
    if (SUCCEEDED(proc->pc->registry->GetInt("config.ThreadSafeSockets",
        &nVal, proc)))
    {
        m_ulThreadSafeFlags = (UINT32)nVal;
    }
    else
    {
        m_ulThreadSafeFlags = HX_THREADSAFE_METHOD_SOCKET_READDONE;
    }
#ifdef XXXDC_DEBUG
    printf ("Client::Client: m_ulThreadSafeFlags = %08x\n", m_ulThreadSafeFlags);
#endif

#if ENABLE_LATENCY_STATS
    m_ulStartTime = proc->pc->engine->now.tv_sec;
#endif
}

Client::~Client(void)
{
#ifdef PAULM_CLIENTTIMING
    g_ClientTimer.Remove(this);
#endif

    if (conn_id)
    {
        proc->pc->conn_id_table->destroy(conn_id);
    }

    // DPRINTF(0x48000000, ("Client(%lu) getting deleted\n", conn_id));
    proc->pc->streamer_info->PlayerDisconnect(proc);

    cleanup_stats();

    HX_RELEASE(m_pProtocol);
    HX_RELEASE(m_pClientGUIDEntry);
    HX_VECTOR_DELETE(m_pPlayerGUID);

    DPRINTF(D_INFO|0x48000000, ("Client(%u) got deleted\n", conn_id));
}

/*
 * IUnknown methods
 */

STDMETHODIMP
Client::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocketResponse*)this;
        return HXR_OK;
    }
#ifdef PAULM_CLIENTAR
    else if (IsEqualIID(riid, IID_IHXObjDebugger))
    {
        *ppvObj = (IHXObjDebugger*)this;
        return HXR_OK;
    }
#endif
    else if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
        AddRef();
        *ppvObj = (IHXThreadSafeMethods*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerShutdownResponse))
    {
        AddRef();
        *ppvObj = (IHXServerShutdownResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
Client::AddRef(void)
{
    DPRINTF(0x5d000000, ("%u:Client::AddRef() == %u\n", conn_id,
            m_ulRefCount+1));
#ifdef PAULM_CLIENTAR
    ((ObjDebugger*)this)->NotifyAddRef();
#endif
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
Client::Release(void)
{
    DPRINTF(0x5d000000, ("%u:Client::Release() == %u\n", conn_id,
            m_ulRefCount-1));
#ifdef PAULM_CLIENTAR
    ((ObjDebugger*)this)->NotifyRelease();
#endif
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    // HX_ASSERT(Process::get_procnum() == proc->procnum());
    delete this;

    return 0;
}

/*
 * IHXServerShutdownResponse methods
 */

STDMETHODIMP
Client::OnShutDownStart(BOOL bLogPlayerTermination)
{
    if(bLogPlayerTermination)
    {
        m_pProtocol->SetStatus(500);
    }
    m_pProtocol->sendAlert(NULL, SE_SERVER_SHUTTING_DOWN);
    return HXR_OK;
}

STDMETHODIMP
Client::OnShutDownEnd()
{
    OnClosed(HXR_FAIL);
    return HXR_OK;
}


/*
 * IUnknown methods
 */

STDMETHODIMP_(UINT32)
Client::IsThreadSafe()
{
    return m_ulThreadSafeFlags;
}

void
Client::OnClosed(HX_RESULT status)
{
    DPRINTF(D_INFO, ("%lu: Client(refcount(%lu)) Closed() called\n", conn_id,
            m_ulRefCount));

    if(m_ulClientStatsObjId != 0)
    {
        HX_ASSERT(proc->pc->process_type == PTStreamer);
        StreamerContainer* pStreamer = ((StreamerContainer*)(proc->pc));
        pStreamer->m_pServerShutdown->Unregister(m_ulClientStatsObjId);
    }

    if (isAlive() && m_state != DELETE_CB_QED)
    {
        m_state = DELETE_CB_QED;
        m_ulCloseStatus = status;

        /*
         * Cannot delete the client here because the client may be part of
         * the call stack. So, schedule the deletion for immediate callback
         * as part of the "dependencies".
         */
        ClientDeleteCallback* cb = new ClientDeleteCallback;
        cb->client = this;
        proc->pc->engine->schedule.enter(0.0, cb);
    }
}

void
Client::init_stats(IHXSockAddr* pLocalAddr, IHXSockAddr* pPeerAddr,
                BOOL bIsCloak)
{
    IHXBuffer* pValue = NULL;

    if (!isAlive() || m_protType == HXPROT_UNKNOWN)
        return;

#ifdef PERF_NOCLIENTREG
    m_ulRegistryConnId = proc->pc->server_info->IncrementTotalClientCount(proc);
#endif
    // Get a regtree

    HX_ASSERT(!m_pRegTree);
    /*
     * Only clients with a playerGUID have a ClientGUIDEntry in the table.
     * However, we still need a "private" entry to manage destruction of
     * the regtree at the end (for example, to allow the 1s delay for the
     * http case; see Client::ClientDeleteCallback::Func()).
     *
     * N.B. The m_pRegTree pointer is only valid as long as we hold a ref
     * on the m_pClientGUIDEntry COM object.
     */

    if (!m_pClientGUIDEntry)
    {
        m_pClientGUIDEntry = g_pClientGUIDTable->GetCreateEntry(NULL, this);
    }

#ifndef PERF_NOCLIENTREG
    m_pRegTree = m_pClientGUIDEntry->GetRegTree();
    m_ulRegistryConnId = m_pRegTree->GetSeqNum();
#endif //ndef PERF_NOCLIENTREG

    if(!m_bIsAProxy)
    {
        if (pLocalAddr == NULL || pPeerAddr == NULL)
        {
            return;
        }

        IHXBuffer* pLocalAddrBuf = NULL;
        UINT16 uLocalPort;
        IHXBuffer* pPeerAddrBuf = NULL;
        UINT16 uPeerPort;

        pLocalAddr->GetAddr(&pLocalAddrBuf);
        uLocalPort = pLocalAddr->GetPort();
        pPeerAddr->GetAddr(&pPeerAddrBuf);
        uPeerPort = pPeerAddr->GetPort();

        if (m_pStats)
        {
            m_pStats->SetIPAddress(pPeerAddrBuf);
            m_pStats->SetPort(uPeerPort);
            m_pStats->SetCloaked(bIsCloak);
        }
#ifndef PERF_NOCLIENTREG
        if (m_bUseRegistryForStats)
        {
            m_pRegTree->SetAddrs(pLocalAddrBuf, pPeerAddrBuf, uPeerPort, bIsCloak);
        }
#endif //ndef PERF_NOCLIENTREG

        HX_RELEASE(pPeerAddrBuf);
        HX_RELEASE(pLocalAddrBuf);
    }

    char version[64]; //ok
    pValue = new ServerBuffer(TRUE);

    if(m_ulClientStatsObjId != 0)
    {
        HX_ASSERT(proc->pc->process_type == PTStreamer);
        StreamerContainer* pStreamer = ((StreamerContainer*)(proc->pc));
        pStreamer->m_pServerShutdown->Register(m_ulClientStatsObjId, this);
    }

    if (m_protType == HXPROT_RTSP)
    {
        sprintf(version, "%d.%d", RTSPMessage::MAJ_VERSION,
                              RTSPMessage::MIN_VERSION);
        pValue->Set((const BYTE*)"RTSP", strlen("RTSP")+1);
    }
    else if (m_protType == HXPROT_HTTP)
    {
        //XXXGH...version?
        sprintf(version, "1.0");
        pValue->Set((const BYTE*)"HTTP", strlen("HTTP")+1);
    }
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    else if (m_protType == HXPROT_MMS)
    {
        //XXXGH...version?
        sprintf(version, "1.0");
        pValue->Set((const BYTE*)"MMS", strlen("MMS")+1);
    }
#endif
    else if (m_protType == HXPROT_CLOAK)
    {
        sprintf(version, "1.0");
        pValue->Set((const BYTE*)"HTTP", strlen("HTTP")+1);
    }
    else
    {
        sprintf(version, "0.0");
        pValue->Set((const BYTE*)"UNKNOWN", strlen("UNKNOWN")+1);
    }

#ifndef PERF_NOCLIENTREG
    // Cloaking calls init_registry with HXPROT_RTSP, so m_pRegTree won't be
    // set here.

    if (m_bUseRegistryForStats && m_pRegTree)
    {
        m_pRegTree->SetProtocol(pValue);
    }
#endif // ndef PERF_NOCLIENTREG

    if (m_pStats)
    {
        m_pStats->SetProtocol(pValue);
    }

    HX_RELEASE(pValue);

    // Reusing pValue for version string.

    pValue = new ServerBuffer(TRUE);
    pValue->Set((Byte*)version, strlen(version)+1);

#ifndef PERF_NOCLIENTREG
    if (m_bUseRegistryForStats && m_pRegTree)
    {
        m_pRegTree->SetVersion(pValue);
    }
#endif // ndef PERF_NOCLIENTREG

    if (m_pStats)
    {
        m_pStats->SetVersion(pValue);
    }

    HX_RELEASE(pValue);
}

void
Client::update_protocol_statistics_info(ClientType nType)
{
    if (!isAlive() || m_bNeedCountDecrement)
        return;

    m_clientType = nType;

    if (m_clientType == MIDBOX_CLIENT)
    {
        proc->pc->server_info->IncrementMidBoxCount(proc);
    }

    if (m_protType == HXPROT_RTSP)
    {
        /**
         * \note When this class is updated to use the shiny new
         * IHXServerInfo interface, instead of the raw proc->pc->server_info
         * the following call will need to become two: One to increment the
         * RTSP client count, and one to increment the cloaked client count.
         */
        if (m_clientType == PLAYER_CLIENT)
            proc->pc->server_info->IncrementRTSPClientCount(is_cloak, proc);
    }
    else if (m_protType == HXPROT_HTTP)
    {
        if (m_clientType == PLAYER_CLIENT)
            proc->pc->server_info->IncrementHTTPClientCount(proc);
    }
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    else if (m_protType == HXPROT_MMS)
    {
        if (m_clientType == PLAYER_CLIENT)
            proc->pc->server_info->IncrementMMSClientCount(proc);
    }
#endif

    m_bNeedCountDecrement = TRUE;

}

void
Client::cleanup_stats()
{
#ifndef PERF_NOCLIENTREG
    if (m_pClientGUIDEntry)
    {
        m_pClientGUIDEntry->Release();
        m_pClientGUIDEntry = NULL;
    }

    if (m_pRegTree)
    {
        m_pRegTree = NULL;
    }
#endif /* ndef PERF_NOCLIENTREG */

    HX_RELEASE(m_pStats);

    if (m_bNeedCountDecrement)
    {
        if(m_clientType == PLAYER_CLIENT)
        {
            /**
             * \note When this class is updated to use the shiny new
             * IHXServerInfo interface, instead of the raw proc->pc->server_info
             * the following call will need to become two: One to decrement the
             * RTSP client count, and one to decrement the cloaked client count.
             */
            if (m_protType == HXPROT_RTSP)
                proc->pc->server_info->DecrementRTSPClientCount(is_cloak, proc);
            else if (m_protType == HXPROT_HTTP)
                proc->pc->server_info->DecrementHTTPClientCount(proc);
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
            else if (m_protType == HXPROT_MMS)
                proc->pc->server_info->DecrementMMSClientCount(proc);
#endif
        }
        else if (m_clientType == MIDBOX_CLIENT)
        {
            proc->pc->server_info->DecrementMidBoxCount(proc);
        }
        m_bNeedCountDecrement = FALSE;
    }
}


////////////////////////////////////////////////////////////////////////
//      Client::SessionDone
////////////////////////////////////////////////////////////////////////
//
// Pass a SessionDone down to the protocol member.
// Used by a player object (which knows its client) to inform the
// protocol that the corresponding player session is being destroyed.
//
// jmevissen, 12/2000

void
Client::SessionDone(const char* sessionID)
{
    if (m_pProtocol) m_pProtocol->SessionDone(sessionID);
}

STDMETHODIMP
Client::ClientDeleteCallback::Func()
{
    DPRINTF(D_INFO, ("%u: CDC::Func() start -- Client->RefCount(%u)\n",
        client->conn_id, client->m_ulRefCount));

    if (client->m_state == DEAD)
    {
        client->Release();
        client = 0;
        return HXR_OK;
    }

    client->m_state = DEAD;

    /*
     * since the protocol does not release the client, we must
     * release ourself
     */
    DPRINTF(0x10000000, ("%u: CDC::Func() before Protocol done "
        "Client->RC(%u)\n",
        client->conn_id, client->m_ulRefCount));

    if (client->m_pProtocol)
    {
        client->m_pProtocol->Done(client->m_ulCloseStatus);
    }

    DPRINTF(0x10000000, ("%u: CDC::Func() before HTTP done Client->RC(%u)\n",
        client->conn_id, client->m_ulRefCount));

    if (client->m_pClientGUIDEntry)
    {
        // Set a destruct delay if the player had a GUID.  (maybe this
        // should be set by the protocol instead?  Right now, only mms-http
        // sets a playerGUID.)

        if (client->m_pPlayerGUID)
        {
            if (zm_lRegDestructDelay < 0)
            {
                INT32 delay = GUID_ENTRY_PERSISTENCE_DEFAULT;
                if (HXR_OK ==
                    client->proc->pc->registry->GetInt(GUID_ENTRY_PERSISTENCE_STRING,
                                                       &delay, client->proc))
                {
                    zm_lRegDestructDelay = delay;
                }
                else
                {
                    zm_lRegDestructDelay = GUID_ENTRY_PERSISTENCE_DEFAULT;
                }
            }
            client->m_pClientGUIDEntry->SetDestructDelay(zm_lRegDestructDelay);
        }

        // Clean up entry

        client->m_pClientGUIDEntry->Done();

        // At this point, the client ref in ClientGUIDEntry has been released.
        // We need to wait until cleanup_stats is executed to release the
        // ClientGUIDEntry-- we don't want to loose the client registry entry
        // before then
    }

    DPRINTF(0x10000000, ("%u: CDC::Func() before release Client->RC(%u)\n",
        client->conn_id, client->m_ulRefCount));

    ULONG32 conn_id = client->conn_id;
    LONG32 ref_count = (client->m_ulRefCount ? client->m_ulRefCount-1 : 0);

#ifdef PAULM_CLIENTAR
    REL_NOTIFY(client, 5);
#endif
    HX_RELEASE(client);

    DPRINTF(0x04000000, ("%6.1u: 4. Client->RefCount(%u)\n",
        conn_id, ref_count));

    return HXR_OK;
}

HX_RESULT
Client::SetPlayerGUID(char* pGUID, int pLen)
{
    // This should only be called once.

    HX_ASSERT(!m_pPlayerGUID || !strncasecmp(m_pPlayerGUID, pGUID, pLen));
    if (m_pPlayerGUID) return HXR_FAIL;

    // set the member string

    m_pPlayerGUID = new char [ pLen+1 ];
    memcpy(m_pPlayerGUID, pGUID, pLen);
    m_pPlayerGUID[pLen] = '\0';

    // do we have to make it case insensitive?

    for (int i=0; i<pLen; ++i)
    {
        m_pPlayerGUID[i] = (char) toupper((int)m_pPlayerGUID[i]);
    }

    // Create / Get an entry in the guid table
    // Is returned with 1 addref.

    m_pClientGUIDEntry = g_pClientGUIDTable->GetCreateEntry(m_pPlayerGUID, this);

    return HXR_OK;
}

void
Client::update_stats()
{
    if (!isAlive())
        return;

    if (m_pStats)
    {
        m_pStats->SetControlBytesSent(m_pProtocol->controlBytesSent());
    }

#ifndef PERF_NOCLIENTREG
    if (m_pRegTree && m_bUseRegistryForStats)
    {
        m_pRegTree->SetControlBytesSent(m_pProtocol->controlBytesSent());
    }
#endif
}

#if ENABLE_LATENCY_STATS
void
Client::TCorePassCB()
{
    m_ulCorePassCBTime = proc->pc->engine->now.tv_sec - m_ulStartTime;
    proc->pc->server_info->UpdateCorePassCBLatency(m_ulCorePassCBTime);
}

void
Client::TDispatch()
{
    m_ulDispatchTime = proc->pc->engine->now.tv_sec - m_ulStartTime;
    proc->pc->server_info->UpdateDispatchLatency(m_ulDispatchTime);
}

void
Client::TStreamer()
{
    m_ulStreamerTime = proc->pc->engine->now.tv_sec - m_ulStartTime;
    proc->pc->server_info->UpdateStreamerLatency(m_ulStreamerTime);
}

void
Client::TFirstRead()
{
    m_ulFirstReadTime = proc->pc->engine->now.tv_sec - m_ulStartTime;
    proc->pc->server_info->UpdateFirstReadLatency(m_ulFirstReadTime);
}
#endif
