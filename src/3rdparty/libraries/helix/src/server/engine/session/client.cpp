/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: client.cpp,v 1.52 2009/01/06 19:02:07 atin Exp $
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

#ifdef _UNIX
#include "ctype.h"
#endif // _UNIX

#include "hxslist.h"
#include "id.h"
#include "hxprot.h"
#include "chxpckts.h"
#include "rtspmsg.h"
#include "server_stats.h"
#include "client.h"
#include "server_engine.h"
#include "server_info.h"
#include "streamer_container.h"
#include "defslice.h"
#include "clientguid.h"
#include "tsid.h"
#include "shutdown.h"
#include "hxtick.h"
#include "servbuffer.h"
#include "pcktflowwrap.h"
#include "clientsession.h"

static const char* GUID_ENTRY_PERSISTENCE_STRING = "config.ClientGUIDPersistence";
static const INT32 GUID_ENTRY_PERSISTENCE_DEFAULT = 30;

// static initializations
INT32 Client::ClientDeleteCallback::zm_lRegDestructDelay = -1;
UINT32 Client::m_ulNextSessionID = 0;

Client::Client(Process* p)
        : m_ulRefCount(0)
        , m_pProc(p)
        , m_pProtocol(NULL)
        , m_state(ALIVE)
        , m_ulConnId(0)
        , m_ulRegistryConnId(0)
        , m_uBytesSent(0)
        , m_protType(HXPROT_UNKNOWN)
        , m_clientType(UNKNOWN_CLIENT)
        , m_bIsAProxy(FALSE)
        , m_pStats(NULL)
        , m_pRegTree(NULL)
        , m_pClientGUIDEntry(NULL)
        , m_bNeedCountDecrement(FALSE)
        , m_pPlayerGUID(NULL)
        , m_bUseRegistryForStats(TRUE)
        , m_ulClientStatsObjId(0)
        , m_bIsCloak(FALSE)
#if ENABLE_LATENCY_STATS
        , m_ulStartTime(0)
        , m_ulCorePassCBTime(0)
        , m_ulDispatchTime(0)
        , m_ulStreamerTime(0)
        , m_ulFirstReadTime(0)
        , m_ulCloseStatus(HXR_OK)
#endif
        , m_pPacketFlowWrap(NULL)
        , m_pSessions(NULL)
{
    m_pSessions = new CHXSimpleList;
    if(m_ulNextSessionID == 0)
    {
        m_ulNextSessionID = rand();
    }
 
    m_ulCreateTime = HX_GET_BETTERTICKCOUNT();

    AddRef();

    UINT32 ul = 0;
    if (HXR_OK == m_pProc->pc->registry->GetInt(REGISTRY_RTSPPROXY_ENABLED,
    (INT32*)&ul, m_pProc))
    {
        m_bIsAProxy = TRUE;
    }

    m_bUseRegistryForStats = m_pProc->pc->client_stats_manager->UseRegistryForStats();

#if ENABLE_LATENCY_STATS
    m_ulStartTime = m_pProc->pc->engine->now.tv_sec;
#endif
}

Client::~Client(void)
{
    if (m_ulConnId)
    {
        m_pProc->pc->conn_id_table->destroy(m_ulConnId);
    }

    // DPRINTF(0x48000000, ("Client(%lu) getting deleted\n", m_ulConnId));
    m_pProc->pc->streamer_info->PlayerDisconnect(m_pProc);

    CleanupStats();

    HX_RELEASE(m_pProtocol);
    HX_RELEASE(m_pClientGUIDEntry);
    HX_VECTOR_DELETE(m_pPlayerGUID);
    Done(HXR_OK);
    DPRINTF(D_INFO|0x48000000, ("Client(%u) got deleted\n", m_ulConnId));
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
    DPRINTF(0x5d000000, ("%u:Client::AddRef() == %u\n", m_ulConnId,
            m_ulRefCount+1));
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
Client::Release(void)
{
    DPRINTF(0x5d000000, ("%u:Client::Release() == %u\n", m_ulConnId,
            m_ulRefCount-1));
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    // HX_ASSERT(Process::get_procnum() == m_pProc->procnum());
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

void
Client::Init(HXProtocolType type, HXProtocol* pProtocol)
{
    m_protType = type;
    m_pProtocol = pProtocol;
    m_pProtocol->AddRef();

    m_ulConnId = m_pProc->pc->conn_id_table->create((void*)1);
}

void
Client::OnClosed(HX_RESULT status)
{
    DPRINTF(D_INFO, ("%lu: Client(refcount(%lu)) Closed() called\n", m_ulConnId,
            m_ulRefCount));

    if(m_ulClientStatsObjId != 0)
    {
        HX_ASSERT(m_pProc->pc->process_type == PTStreamer);
        StreamerContainer* pStreamer = ((StreamerContainer*)(m_pProc->pc));
        pStreamer->m_pServerShutdown->Unregister(m_ulClientStatsObjId);
    }

    if (IsAlive() && m_state != DELETE_CB_QED)
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
        m_pProc->pc->engine->schedule.enter(0.0, cb);
    }
}

void
Client::InitStats(IHXSockAddr* pLocalAddr, IHXSockAddr* pPeerAddr,
                BOOL bIsCloak)
{
    IHXBuffer* pValue = NULL;

    if (!IsAlive() || m_protType == HXPROT_UNKNOWN)
        return;

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

    m_pRegTree = m_pClientGUIDEntry->GetRegTree();
    m_ulRegistryConnId = m_pRegTree->GetSeqNum();

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
        if (m_bUseRegistryForStats)
        {
            m_pRegTree->SetAddrs(pLocalAddrBuf, pPeerAddrBuf, uPeerPort, bIsCloak);
        }

        HX_RELEASE(pPeerAddrBuf);
        HX_RELEASE(pLocalAddrBuf);
    }

    char version[64]; //ok
    pValue = new ServerBuffer(TRUE);

    if(m_ulClientStatsObjId != 0)
    {
        HX_ASSERT(m_pProc->pc->process_type == PTStreamer);
        StreamerContainer* pStreamer = ((StreamerContainer*)(m_pProc->pc));
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

    // Cloaking calls init_registry with HXPROT_RTSP, so m_pRegTree won't be
    // set here.

    if (m_bUseRegistryForStats && m_pRegTree)
    {
        m_pRegTree->SetProtocol(pValue);
    }

    if (m_pStats)
    {
        m_pStats->SetProtocol(pValue);
    }

    HX_RELEASE(pValue);

    // Reusing pValue for version string.

    pValue = new ServerBuffer(TRUE);
    pValue->Set((Byte*)version, strlen(version)+1);

    if (m_bUseRegistryForStats && m_pRegTree)
    {
        m_pRegTree->SetVersion(pValue);
    }

    if (m_pStats)
    {
        m_pStats->SetVersion(pValue);
    }

    HX_RELEASE(pValue);
}

void
Client::UpdateProtocolStatsInfo(ClientType nType)
{
    if (!IsAlive() || m_bNeedCountDecrement)
        return;

    m_clientType = nType;

    if (m_clientType == MIDBOX_CLIENT)
    {
        m_pProc->pc->server_info->IncrementMidBoxCount(m_pProc);
    }

    if (m_protType == HXPROT_RTSP)
    {
        /**
         * \note When this class is updated to use the shiny new
         * IHXServerInfo interface, instead of the raw m_pProc->pc->server_info
         * the following call will need to become two: One to increment the
         * RTSP client count, and one to increment the cloaked client count.
         */
        if (m_clientType == PLAYER_CLIENT)
            m_pProc->pc->server_info->IncrementRTSPClientCount(m_bIsCloak, m_pProc);
    }
    else if (m_protType == HXPROT_HTTP)
    {
        if (m_clientType == PLAYER_CLIENT)
            m_pProc->pc->server_info->IncrementHTTPClientCount(m_pProc);
    }
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    else if (m_protType == HXPROT_MMS)
    {
        if (m_clientType == PLAYER_CLIENT)
            m_pProc->pc->server_info->IncrementMMSClientCount(m_pProc);
    }
#endif

    m_bNeedCountDecrement = TRUE;

}

void
Client::CleanupStats()
{
    if (m_pClientGUIDEntry)
    {
        m_pClientGUIDEntry->Release();
        m_pClientGUIDEntry = NULL;
    }

    if (m_pRegTree)
    {
        m_pRegTree = NULL;
    }

    HX_RELEASE(m_pStats);

    if (m_bNeedCountDecrement)
    {
        if(m_clientType == PLAYER_CLIENT)
        {
            /**
             * \note When this class is updated to use the shiny new
             * IHXServerInfo interface, instead of the raw m_pProc->pc->server_info
             * the following call will need to become two: One to decrement the
             * RTSP client count, and one to decrement the cloaked client count.
             */
            if (m_protType == HXPROT_RTSP)
                m_pProc->pc->server_info->DecrementRTSPClientCount(m_bIsCloak, m_pProc);
            else if (m_protType == HXPROT_HTTP)
                m_pProc->pc->server_info->DecrementHTTPClientCount(m_pProc);
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
            else if (m_protType == HXPROT_MMS)
                m_pProc->pc->server_info->DecrementMMSClientCount(m_pProc);
#endif
        }
        else if (m_clientType == MIDBOX_CLIENT)
        {
            m_pProc->pc->server_info->DecrementMidBoxCount(m_pProc);
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
        client->m_ulConnId, client->m_ulRefCount));

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
        client->m_ulConnId, client->m_ulRefCount));

    if (client->m_pProtocol)
    {
        client->m_pProtocol->Done(client->m_ulCloseStatus);
    }

    DPRINTF(0x10000000, ("%u: CDC::Func() before HTTP done Client->RC(%u)\n",
        client->m_ulConnId, client->m_ulRefCount));

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
                    client->m_pProc->pc->registry->GetInt(GUID_ENTRY_PERSISTENCE_STRING,
                                                       &delay, client->m_pProc))
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
        client->m_ulConnId, client->m_ulRefCount));

    ULONG32 ulConnId = client->m_ulConnId;
    LONG32 ref_count = (client->m_ulRefCount ? client->m_ulRefCount-1 : 0);

    HX_RELEASE(client);

    DPRINTF(0x04000000, ("%6.1u: 4. Client->RefCount(%u)\n",
        ulConnId, ref_count));

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
Client::UpdateStats()
{
    if (!IsAlive())
        return;

    if (m_pStats)
    {
        m_pStats->SetControlBytesSent(m_pProtocol->controlBytesSent());
    }

    if (m_pRegTree && m_bUseRegistryForStats)
    {
        m_pRegTree->SetControlBytesSent(m_pProtocol->controlBytesSent());
    }
}

void
Client::Done(HX_RESULT status)
{
   if (m_pSessions)
   {
		ClearSessionList(status);
	    HX_DELETE(m_pSessions);
   }
   if (m_pPacketFlowWrap)
   {
	    HX_DELETE(m_pPacketFlowWrap);
   }
}

void
Client::ClearSessionList(HX_RESULT status)
{
    CHXSimpleList::Iterator i;

    for (i = m_pSessions->Begin(); i != m_pSessions->End(); ++i)
    {
        ClientSession* pSession = (ClientSession*)(*i);
        pSession->Done(status);
        pSession->Release();
    }

    m_pSessions->RemoveAll();
}


ClientSession*
Client::FindSession(const char* pSessionID)
{
    CHXSimpleList::Iterator i;

    if (pSessionID)
    {
        for (i = m_pSessions->Begin(); i != m_pSessions->End(); ++i)
        {
            ClientSession* pSession = (ClientSession*)(*i);
            if (pSession->m_sessionID == pSessionID)
            {
                return pSession;
            }
        }
    }
    return 0;
}

HX_RESULT
Client::RemoveSession(const char* pSessionID, HX_RESULT status)
{
    HX_ASSERT(m_pSessions != NULL);
    if (m_pSessions != NULL)
    {
        LISTPOSITION pos = m_pSessions->GetHeadPosition();
        while (pos)
        {
            ClientSession* pSession = (ClientSession*)m_pSessions->GetAt(pos);
            if (pSession->m_sessionID == pSessionID)
            {
                pos = m_pSessions->RemoveAt(pos);
                pSession->Done(status);
                m_pProc->pc->server_info->DecrementStreamCount(m_pProc);
                pSession->Release();
            }
            else
            {
                m_pSessions->GetNext(pos);
            }
        }
    }
    return HXR_OK;
}

/* XXXTDM: get rid of this and NewSessionWithID().
* These are used for the Pragma: initiate-session, to allow an RTSP session
* to be created without a corresponding ClientSession.  Allowing this has
* created problems (see comments in rtspserv.cpp).  The two session
* objects should have a 1:1 correspondence.
*/
HX_RESULT
Client::GenerateNewSessionID(CHXString& sessionID, UINT32 ulSeqNo)
{
    char tmp[64];
    sprintf(tmp, "%ld-%ld", ++m_ulNextSessionID, ulSeqNo);
    sessionID = tmp;
    return HXR_OK;
}

HX_RESULT
Client::NewSession(ClientSession** ppSession,
                   UINT32 uSeqNo,
                   BOOL bRetainEntityForSetup)
{
    CHXString sessionID;
    GenerateNewSessionID(sessionID, uSeqNo);

    *ppSession = new ClientSession(m_pProc, this, sessionID);
    m_pSessions->AddHead(*ppSession);

    // AddRef for both list and out parameter.
    (*ppSession)->AddRef();
    (*ppSession)->AddRef();

    /*
    * this solves the problem of the subscriber counting http connections,
    * which was solved previously by counting only pna and rtsp clients.
    * since only pnaprot and rtspprot call NewSession() and/or
    * NewSessioWithID() the subscriber can rely on the 'server.streamCount'
    * var for the same info.
    */
    m_pProc->pc->server_info->IncrementStreamCount(m_pProc);

    return HXR_OK;
}

HX_RESULT
Client::NewSessionWithID(ClientSession** ppSession,
                         UINT32 uSeqNo, const char* pSessionId,
                         BOOL bRetainEntityForSetup)
{
    *ppSession = new ClientSession(m_pProc, this, pSessionId);
    m_pSessions->AddHead(*ppSession);

    // AddRef for both list and out parameter.
    (*ppSession)->AddRef();
    (*ppSession)->AddRef();

    /*
    * this solves the problem of the subscriber counting http connections,
    * which was solved previously by counting only pna and rtsp clients.
    * since only pnaprot and rtspprot call NewSession() and/or
    * NewSessioWithID() the subscriber can rely on the 'server.streamCount'
    * var for the same info.
    */
    m_pProc->pc->server_info->IncrementStreamCount(m_pProc);

    return HXR_OK;
}

void
Client::SetStreamStartTime(const char* szSessionID, UINT32 ulStreamNum,
                           UINT32 ulTimestamp)
{
    ClientSession* pSession = FindSession(szSessionID);
    if (pSession)
    {
        pSession->SetStreamStartTime(ulStreamNum, ulTimestamp);
    }
}

HX_RESULT
Client::HandleDefaultSubscription(const char* szSessionID)
{
    ClientSession* pSession = FindSession(szSessionID);
    if (pSession)
    {
        return pSession->HandleDefaultSubscription();
    }
    return HXR_UNEXPECTED;
}


#if ENABLE_LATENCY_STATS
void
Client::TCorePassCB()
{
    m_ulCorePassCBTime = m_pProc->pc->engine->now.tv_sec - m_ulStartTime;
    m_pProc->pc->server_info->UpdateCorePassCBLatency(m_ulCorePassCBTime);
}

void
Client::TDispatch()
{
    m_ulDispatchTime = m_pProc->pc->engine->now.tv_sec - m_ulStartTime;
    m_pProc->pc->server_info->UpdateDispatchLatency(m_ulDispatchTime);
}

void
Client::TStreamer()
{
    m_ulStreamerTime = m_pProc->pc->engine->now.tv_sec - m_ulStartTime;
    m_pProc->pc->server_info->UpdateStreamerLatency(m_ulStreamerTime);
}

void
Client::TFirstRead()
{
    m_ulFirstReadTime = m_pProc->pc->engine->now.tv_sec - m_ulStartTime;
    m_pProc->pc->server_info->UpdateFirstReadLatency(m_ulFirstReadTime);
}
#endif
