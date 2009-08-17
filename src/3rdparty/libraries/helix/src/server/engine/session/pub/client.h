/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: client.h,v 1.17 2006/06/26 17:20:57 darrick Exp $
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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#define REPLAY_BUFFER_SIZE 4096

class State;
class ConnList;
class SIO;
class TCPIO;
class Client;
class HXProtocol;
class LCClientList;
class HTTP;
class Engine;
class ClientGUIDEntry;
class ClientRegTree;

struct IHXClientStats;

#include "base_callback.h"
#include "callback_container.h"
#include "proc.h"
#include "sockio.h"
#include "inetwork.h"
#include "hxnet.h"
#include "servsockimp.h"
#include "server_engine.h"
#include "timeval.h"
#include "mutex.h"
#include "hxshutdown.h"

#include "servlist.h"
#include "clientregtree.h"  // allows inlining of GetRegId

#ifdef PAULM_CLIENTAR
#include "objdbg.h"
#include "odbg.h"
#endif

#include "servprotdef.h" // ClientType

#include "hxprotmgr.h" /* HXProtocolType */

class Client: public IHXServerShutdownResponse,
#ifdef PAULM_CLIENTAR
              public ObjDebugger,
#endif
              public IHXThreadSafeMethods
{
public:
    Client(Process* p);
    virtual ~Client(void);

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)           (THIS);
    STDMETHOD_(UINT32,Release)          (THIS);

    /*
     * IHXThreadSafeMethods methods
     */
    STDMETHOD_(UINT32,IsThreadSafe)     (THIS);

    /*
     * IHXServerShutdownResponse
     */
    STDMETHOD(OnShutDownStart)           (THIS_
                                          BOOL bLogPlayerTermination);

    STDMETHOD(OnShutDownEnd)             (THIS);


    /*
     * Client methods
     */

    void            SessionDone(const char* sessionID);

    UINT32          id() { return conn_id; }
    HXProtocol*     m_pProtocol;

    void            init(HXProtocolType type, HXProtocol* pProtocol);
    void            OnClosed(HX_RESULT status);
    void            update_stats();

    Process*        proc;
    UINT32          conn_id;

    BOOL            is_cloak;

    BOOL            m_bUseRegistryForStats;
    BOOL            use_registry_for_stats();

    HXProtocol*     protocol();

    ULONG32         m_ulRefCount;

    BOOL            m_bIsAProxy;
    BOOL            m_bNeedCountDecrement;

#if ENABLE_LATENCY_STATS
    void            TCorePassCB();
    void            TDispatch();
    void            TStreamer();
    void            TFirstRead();
    INT32           m_ulStartTime;
    INT32           m_ulCorePassCBTime;
    INT32           m_ulDispatchTime;
    INT32           m_ulStreamerTime;
    INT32           m_ulFirstReadTime;
#endif
    UINT32          m_ulCreateTime;

    class ClientDeleteCallback : public SimpleCallback, public BaseCallback
    {
    public:
        void func(Process*) { (void) Func(); delete this; }
        STDMETHOD(Func) (THIS);
        Client*         client;
        static INT32    zm_lRegDestructDelay;
    };

    enum ClientDeleteState
    {
        ALIVE,
        CLOSE_CB_DISPATCHED,
        DELETE_CB_DISPATCHED,
        CLOSE_CB_QED,
        DELETE_CB_QED,
        DEAD
    } m_state;
    BOOL isAlive();
    /*
     * for every server session the following number indicates where
     * in the list of total players connected does this client lie.
     */
    UINT32          m_ulRegistryConnId;
    UINT32          m_ulClientStatsObjId;

    HXProtocolType  m_protType;
    ClientType      m_clientType;
    char*           m_pPlayerGUID;

    ClientGUIDEntry* m_pClientGUIDEntry;
    ClientRegTree*  m_pRegTree;
    IHXClientStats* m_pStats;

    UINT32 GetRegId(ClientRegTree::Field nField);
    void init_stats(IHXSockAddr* pLocalAddr, IHXSockAddr* pPeerAddr,
                    BOOL bIsCloak);
    void update_protocol_statistics_info(ClientType nType = PLAYER_CLIENT);
    void cleanup_stats();
    IHXClientStats* get_client_stats();

    UINT32 get_registry_conn_id();
    void set_registry_conn_id(UINT32 ulConnId);

    UINT32 get_client_stats_obj_id();
    void set_client_stats_obj_id(UINT32 ulObjId);

    HX_RESULT   SetPlayerGUID(char* pGUID, int pLen);
    const char* GetPlayerGUID() { return m_pPlayerGUID; };

    UINT32          m_uBytesSent;

#ifdef PAULM_CLIENTTIMING
    int             in_streamer;
    int             died_from_timeout;
#endif

    INT32           m_ulThreadSafeFlags;
    HX_RESULT       m_ulCloseStatus;
};

inline HXProtocol*
Client::protocol()
{
    return m_pProtocol;
}

inline BOOL
Client::isAlive()
{
    return m_state == ALIVE ? TRUE : FALSE;
}


inline UINT32
Client::GetRegId(ClientRegTree::Field nField)
{
    return m_pRegTree ? m_pRegTree->GetRegId(nField) : 0;
}

inline BOOL
Client::use_registry_for_stats()
{
    return m_bUseRegistryForStats;
}

inline IHXClientStats*
Client::get_client_stats()
{
    return m_pStats;
}

inline UINT32
Client::get_client_stats_obj_id()
{
    return m_ulClientStatsObjId;
}

inline void
Client::set_client_stats_obj_id(UINT32 ulObjId)
{
    m_ulClientStatsObjId = ulObjId;
}

inline UINT32
Client::get_registry_conn_id()
{
    return m_ulRegistryConnId;
}

inline void
Client::set_registry_conn_id(UINT32 ulConnId)
{
    m_ulRegistryConnId = ulConnId;
}

#endif/*_CLIENT_H_*/
