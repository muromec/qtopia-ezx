/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: client.h,v 1.20 2007/08/18 00:21:14 dcollins Exp $
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

#include "base_callback.h"
#include "proc.h"
#include "hxnet.h"
#include "servsockimp.h"
#include "server_engine.h"
#include "hxshutdown.h"
#include "servlist.h"
#include "clientregtree.h"  // allows inlining of GetRegId
#include "servprotdef.h" // ClientType
#include "hxprotmgr.h" /* HXProtocolType */
#include "hxclientprofile.h"

class       PacketFlowWrapper;
class       ClientSession;
class       HXProtocol;
class       ClientGUIDEntry;
class       ClientRegTree;
struct      IHXClientStats;

class Client : public IHXServerShutdownResponse,
               public HXListElem
{
public:
    Client(Process* proc);
    virtual ~Client(void);

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)           (THIS);
    STDMETHOD_(UINT32,Release)          (THIS);

    /*
     * IHXServerShutdownResponse methods
     */
    STDMETHOD(OnShutDownStart)           (THIS_ BOOL bLogPlayerTermination);
    STDMETHOD(OnShutDownEnd)             (THIS);

    /*
     * Client methods
     */
    HXProtocol*     GetProtocol();
    void            SessionDone(const char* sessionID);
    UINT32          GetConnId() { return m_ulConnId; }

    void            Init(HXProtocolType type, HXProtocol* pProtocol);
    void            OnClosed(HX_RESULT status);

    void            InitStats(IHXSockAddr* pLocalAddr,
                              IHXSockAddr* pPeerAddr,
                              BOOL bIsCloak);
    void            UpdateStats();
    HXBOOL          UseRegistryForStats();
    void            UpdateProtocolStatsInfo(ClientType nType = PLAYER_CLIENT);
    void            CleanupStats();

    IHXClientStats* GetClientStats();
    
    UINT32          GetRegId(ClientRegTree::Field nField);

    UINT32          GetRegistryConnId();
    void            SetRegistryConnId(UINT32 ulConnId);

    UINT32          GetClientStatsObjId();
    void            SetClientStatsObjId(UINT32 ulObjId);

    const char*     GetPlayerGUID() { return m_pPlayerGUID; };
    HX_RESULT       SetPlayerGUID(char* pGUID, int pLen);
    
    HXBOOL          IsAlive();

    void            Done(HX_RESULT status);
    HX_RESULT       GenerateNewSessionID(CHXString& sessionID,
                                         UINT32 ulSeqNo);
    HX_RESULT       NewSession(ClientSession** ppSession,
                               UINT32 ulSeqNo,
                               BOOL bRetainEntityForSetup=TRUE);
    HX_RESULT       NewSessionWithID(ClientSession** ppSession,
                                     UINT32 ulSeqNo,
                                     const char* pSessionID,
                                     BOOL bRetainEntityForSetup=TRUE);
    ClientSession*  FindSession(const char* pSessionID);
    HX_RESULT       RemoveSession(const char* pSessionID,
                                  HX_RESULT status);
    void            ClearSessionList(HX_RESULT status);
    INT32           NumSessions() { return m_pSessions ? m_pSessions->GetCount() : 0; }

    void            SetStreamStartTime(const char* pSessionID,
                                       UINT32 ulStreamNum,
                                       UINT32 ulTimestamp);

    HX_RESULT       HandleDefaultSubscription(const char* szSessionID);

    class ClientDeleteCallback : public SimpleCallback,
                                 public BaseCallback
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

    ULONG32             m_ulRefCount;

    /*
     * for every server session the following number indicates where
     * in the list of total players connected this client lies.
     */
    UINT32              m_ulRegistryConnId;
    UINT32              m_ulClientStatsObjId;

    Process*            m_pProc;
    UINT32              m_ulConnId;

    HXBOOL              m_bIsAProxy;
    HXBOOL              m_bNeedCountDecrement;
    HXBOOL              m_bIsCloak;
    HXBOOL              m_bUseRegistryForStats;

    HXProtocol*         m_pProtocol;
    HXProtocolType      m_protType;
    ClientType          m_clientType;
    char*               m_pPlayerGUID;

    ClientGUIDEntry*    m_pClientGUIDEntry;
    ClientRegTree*      m_pRegTree;
    IHXClientStats*     m_pStats;

    UINT32              m_uBytesSent;
    HX_RESULT           m_ulCloseStatus;
    
    UINT32              m_ulCreateTime;

#if ENABLE_LATENCY_STATS
    void                TCorePassCB();
    void                TDispatch();
    void                TStreamer();
    void                TFirstRead();
    INT32               m_ulStartTime;
    INT32               m_ulCorePassCBTime;
    INT32               m_ulDispatchTime;
    INT32               m_ulStreamerTime;
    INT32               m_ulFirstReadTime;
#endif
    
    PacketFlowWrapper*  m_pPacketFlowWrap;
    CHXSimpleList*      m_pSessions;
    static UINT32       m_ulNextSessionID;
};

inline HXProtocol*
Client::GetProtocol()
{
    return m_pProtocol;
}

inline HXBOOL
Client::IsAlive()
{
    return m_state == ALIVE ? TRUE : FALSE;
}

inline UINT32
Client::GetRegId(ClientRegTree::Field nField)
{
    return m_pRegTree ? m_pRegTree->GetRegId(nField) : 0;
}

inline HXBOOL
Client::UseRegistryForStats()
{
    return m_bUseRegistryForStats;
}

inline IHXClientStats*
Client::GetClientStats()
{
    return m_pStats;
}

inline UINT32
Client::GetClientStatsObjId()
{
    return m_ulClientStatsObjId;
}

inline void
Client::SetClientStatsObjId(UINT32 ulObjId)
{
    m_ulClientStatsObjId = ulObjId;
}

inline UINT32
Client::GetRegistryConnId()
{
    return m_ulRegistryConnId;
}

inline void
Client::SetRegistryConnId(UINT32 ulConnId)
{
    m_ulRegistryConnId = ulConnId;
}

#endif/*_CLIENT_H_*/
