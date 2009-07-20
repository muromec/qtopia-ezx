/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: 
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

///////////////////////////////////////////////////////////////////////////////
//
// server_stats.h - Classes for clientstats based logging.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef SERVER_STATS_H
#define SERVER_STATS_H


///////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <limits.h>
#include "mutex.h"
#include "simple_callback.h"
#include "hxstats.h"
#include "hxprivstats.h"


///////////////////////////////////////////////////////////////////////////////
// CONSTANTS
///////////////////////////////////////////////////////////////////////////////

#define BRACKETED_UNKNOWN                "[UNKNOWN]"
#define BRACKETED_UNKNOWN_LENGTH         9


///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARES
///////////////////////////////////////////////////////////////////////////////

class ClientStatsTimer;
class SimpleCallback;
_INTERFACE IHXClientProfileInfo;
_INTERFACE IHXQOSTransportAdaptationInfo;
_INTERFACE IHXQOSSessionAdaptationInfo;
_INTERFACE IHXQOSApplicationAdaptationInfo;
_INTERFACE IHXScheduler;


_INTERFACE IUnknown;
_INTERFACE IHXClientStatsTimerControl; 

_INTERFACE IHXClientStats2;
_INTERFACE IHXPrivateClientStats;

// {793AB631-B654-4aae-99A3-0DF87258C363}
DEFINE_GUID(IID_IHXClientStatsTimerControl, 0x793ab631, 0xb654, 0x4aae, 0x99, 
            0xa3, 0xd, 0xf8, 0x72, 0x58, 0xc3, 0x63);

#define CLSID_IHXClientStatsTimerControl   IID_IHXClientStatsTimerControl

#undef  INTERFACE
#define INTERFACE IHXClientStatsTimerControl

DECLARE_INTERFACE_(IHXClientStatsTimerControl, IUnknown)
{

    // IUnknown methods

    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;

    STDMETHOD(SetTimer)                     (THIS_
                                            ClientStatsTimer* pTimer) PURE;
    STDMETHOD(ClearTimers)                  (THIS) PURE;

};




///////////////////////////////////////////////////////////////////////////////
// SessionStats class
///////////////////////////////////////////////////////////////////////////////

class SessionStats : public IHXSessionStats2,
                     public IHXCheckRetainEntityForSetup
{

protected:

    UINT32 m_RefCount;

    IHXBuffer* m_pHost;
    IHXBuffer* m_pSessionStartTime;
    IHXBuffer* m_pURL;
    IHXBuffer* m_pLogURL;
    IHXBuffer* m_pLogStats;
    IHXBuffer* m_pPlayerRequestedURL;
    IHXBuffer* m_pSalt;
    IHXBuffer* m_pAuth;
    IHXBuffer* m_pProxyConnectionType;
    IHXBuffer* m_pInterfaceAddr;
    IHXBuffer* m_pClientProfileURIs;

    IHXClientStats* m_pClient;  // points back to the parent client

    UINT32 m_ulID;

    UINT64 m_ulFileSize;
    UINT32 m_ulStatus;
    SessionStatsEndStatus m_ulEndStatus;
    UINT32 m_ulDuration;
    UINT32 m_ulAvgBitrate;
    UINT32 m_ulSendingTime;
    UINT32 m_ulPlayTime;
    UINT32 m_ulPacketLoss;
    UINT16 m_uiXWapProfileStatus;

    BOOL m_bIsMulticastUsed;
    BOOL m_bIsUDP;
    BOOL m_bIsRVStreamFound;
    BOOL m_bIsRAStreamFound;
    BOOL m_bIsREStreamFound;
    BOOL m_bIsRIStreamFound;
    BOOL m_bUpdateRegistryForLive;

    IHXClientProfileInfo* m_pClientProfileInfo;
    IHXQoSTransportAdaptationInfo* m_pQoSTransportAdaptationInfo;
    IHXQoSSessionAdaptationInfo* m_pQoSSessionAdaptationInfo;
    IHXQoSApplicationAdaptationInfo* m_pQoSApplicationAdaptationInfo;

    UINT32 m_ulConnectTime;
    UINT32 m_ulSessionEstablishmentTime;
    UINT32 m_ulSessionSetupTime;
    UINT32 m_ulFirstPacketTime;
    UINT32 m_ulPreDataTime;
    UINT32 m_ulPreDataBytes;
    UINT32 m_ulPrerollInMsec;
public:

    SessionStats();
    ~SessionStats();
    
// IUnknown methods.

    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

// IHXSessionStats methods.

    STDMETHODIMP_(IHXClientStats*)GetClient();
    STDMETHODIMP SetClient(IHXClientStats* pClient);

    STDMETHODIMP_(UINT32) GetID();
    STDMETHODIMP SetID(UINT32 ulID);
    
    STDMETHODIMP_(IHXBuffer*) GetHost();
    STDMETHODIMP SetHost(IHXBuffer* pHost);

    STDMETHODIMP_(IHXBuffer*) GetSessionStartTime();
    STDMETHODIMP SetSessionStartTime(IHXBuffer* pSessionStartTime);

    STDMETHODIMP_(IHXBuffer*) GetURL();
    STDMETHODIMP SetURL(IHXBuffer* pURL);

    STDMETHODIMP_(IHXBuffer*) GetLogURL();
    STDMETHODIMP SetLogURL(IHXBuffer* pLogURL);

    STDMETHODIMP_(IHXBuffer*) GetLogStats();
    STDMETHODIMP SetLogStats(IHXBuffer* pLogStats);

    STDMETHODIMP_(IHXBuffer*) GetPlayerRequestedURL();
    STDMETHODIMP SetPlayerRequestedURL(IHXBuffer* pPlayerRequestedURL);

    STDMETHODIMP_(IHXBuffer*) GetSalt();
    STDMETHODIMP SetSalt(IHXBuffer* pSalt);

    STDMETHODIMP_(IHXBuffer*) GetAuth();
    STDMETHODIMP SetAuth(IHXBuffer* pAuth);

    STDMETHODIMP_(IHXBuffer*) GetProxyConnectionType();
    STDMETHODIMP SetProxyConnectionType(IHXBuffer* pProxyConnectionType);

    STDMETHODIMP_(IHXBuffer*) GetInterfaceAddr();
    STDMETHODIMP SetInterfaceAddr(IHXBuffer* pInterfaceAddr);

    
    STDMETHODIMP_(UINT64) GetFileSize();
    STDMETHODIMP SetFileSize(UINT64 ulFileSize);

    STDMETHODIMP_(UINT32) GetStatus();
    STDMETHODIMP SetStatus(UINT32 ulHTTPStatus);

    STDMETHODIMP_(SessionStatsEndStatus) GetEndStatus();
    STDMETHODIMP SetEndStatus(SessionStatsEndStatus ulEndStatus);

    STDMETHODIMP_(UINT32) GetDuration();
    STDMETHODIMP SetDuration(UINT32 ulDuration);

    STDMETHODIMP_(UINT32) GetAvgBitrate();
    STDMETHODIMP SetAvgBitrate(UINT32 ulAvgBitrate);

    STDMETHODIMP_(UINT32) GetSendingTime();
    STDMETHODIMP SetSendingTime(UINT32 ulSendingTime);

    STDMETHODIMP_(UINT32) GetPlayTime();
    STDMETHODIMP SetPlayTime(UINT32 ulPlayTime);

    STDMETHODIMP_(BOOL) IsMulticastUsed();
    STDMETHODIMP SetMulticastUsed(BOOL bIsMulticastUsed);

    STDMETHODIMP_(BOOL) IsUDP();
    STDMETHODIMP SetUDP(BOOL bIsUDP);

    STDMETHODIMP_(BOOL) IsRVStreamFound();
    STDMETHODIMP SetRVStreamFound(BOOL bIsRVStreamFound);

    STDMETHODIMP_(BOOL) IsRAStreamFound();
    STDMETHODIMP SetRAStreamFound(BOOL bIsRAStreamFound);

    STDMETHODIMP_(BOOL) IsREStreamFound();
    STDMETHODIMP SetREStreamFound(BOOL bIsREStreamFound);

    STDMETHODIMP_(BOOL) IsRIStreamFound();
    STDMETHODIMP SetRIStreamFound(BOOL bIsRIStreamFound);


    STDMETHODIMP_(UINT16) GetXWapProfileStatus();
    STDMETHODIMP SetXWapProfileStatus(UINT16 unXWapProfileStatus);

    STDMETHODIMP GetClientProfileInfo(REF(IHXClientProfileInfo*) pInfo);
    STDMETHODIMP SetClientProfileInfo(IHXClientProfileInfo* pInfo);

    STDMETHODIMP_(IHXBuffer*) GetClientProfileURIs();
    STDMETHODIMP SetClientProfileURIs(IHXBuffer* pURIs);

    STDMETHODIMP_(IHXQoSTransportAdaptationInfo*) GetQoSTransportAdaptationInfo();
    STDMETHODIMP SetQoSTransportAdaptationInfo(IHXQoSTransportAdaptationInfo* pInfo);

    STDMETHODIMP_(IHXQoSSessionAdaptationInfo*) GetQoSSessionAdaptationInfo();
    STDMETHODIMP SetQoSSessionAdaptationInfo(IHXQoSSessionAdaptationInfo* pInfo);

    STDMETHODIMP_(IHXQoSApplicationAdaptationInfo*) GetQoSApplicationAdaptationInfo();
    STDMETHODIMP SetQoSApplicationAdaptationInfo(IHXQoSApplicationAdaptationInfo* pInfo);

// IHXSessionStats2 methods.

    STDMETHODIMP_(UINT32) GetConnectTime();
    STDMETHODIMP SetConnectTime(UINT32 ulConnectTime);

    STDMETHODIMP_(UINT32) GetSessionEstablishmentTime();
    STDMETHODIMP SetSessionEstablishmentTime(UINT32 ulSessionEstablishmentTime);

    STDMETHODIMP_(UINT32) GetSessionSetupTime();
    STDMETHODIMP SetSessionSetupTime(UINT32 ulSessionSetupTime);

    STDMETHODIMP_(UINT32) GetFirstPacketTime();
    STDMETHODIMP SetFirstPacketTime(UINT32 ulFirstPacketTime);

    STDMETHODIMP_(UINT32) GetPreDataTime();
    STDMETHODIMP SetPreDataTime(UINT32 ulPreDataTime);

    STDMETHODIMP_(UINT32) GetPrerollInMsec();
    STDMETHODIMP SetPrerollInMsec(UINT32 ulPrerollInMsec);

    STDMETHODIMP_(UINT32) GetPreDataBytes();
    STDMETHODIMP SetPreDataBytes(UINT32 ulPreDataBytes);

    STDMETHODIMP DumpStartupInfo();
// IHXCheckRetainEntityForSetup methods.

    STDMETHODIMP_(BOOL) GetUpdateRegistryForLive();
    STDMETHODIMP SetUpdateRegistryForLive();
};


///////////////////////////////////////////////////////////////////////////////
// ClientStats class
///////////////////////////////////////////////////////////////////////////////

class ClientStats : public IHXClientStats2,
                    public IHXPrivateClientStats,
                    public IHXClientStatsTimerControl,
                    public HXListElem
{

protected:
    
    IHXClientStatsManager* m_pStatsMgr;
    
    CHXID* m_pSessionTable;

    UINT32 m_RefCount;

    UINT32 m_ulID; 
    INT32 m_lSessionCount;
    INT32 m_lSessionIndex;

    IHXBuffer* m_pIPAddress;
    IHXBuffer* m_pCBID;
    IHXBuffer* m_pGUID;
    IHXBuffer* m_pClientID;
    IHXBuffer* m_pPNAClientID;
    IHXBuffer* m_pCompanyID;
    IHXBuffer* m_pClientChallenge;
    IHXBuffer* m_pLanguage;
    IHXBuffer* m_pPlayerStartTime;
    IHXBuffer* m_pProtocol;
    IHXBuffer* m_pStartTime;
    IHXBuffer* m_pRequestMethod;
    IHXBuffer* m_pUserAgent;
    IHXBuffer* m_pVersion;
    IHXBuffer* m_pLoadTestPassword;
    IHXBuffer* m_pRTSPEvents;
    IHXBuffer* m_pStreamSelectionInfo;    

    UINT64 m_ulControlBytesSent;
    UINT32 m_ulPort;
    UINT32 m_ulTotalMediaAdaptations;
    UINT32 m_ulReasonForTermination;

    BOOL m_bIsCloaked;
    BOOL m_bIsRDT;
    BOOL m_bIsPrevAuth;

    BOOL m_bSupportsMaximumASMBandwidth;
    BOOL m_bSupportsMulticast;

    HX_MUTEX m_pMutex;

    HXList* m_pTimerList;

    Process* m_pProc;

public:

    ClientStats(Process* pProc);
    ~ClientStats();


// IUnknown methods.

    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

// IHXClientStats methods.

    STDMETHODIMP_(IHXClientStatsManager*)GetStatsManager();
    STDMETHODIMP SetStatsManager(IHXClientStatsManager* pStatsMgr);

    STDMETHODIMP_(UINT32) GetID();
    STDMETHODIMP SetID(UINT32 ulID);

    STDMETHODIMP_(IHXBuffer*) GetIPAddress();
    STDMETHODIMP SetIPAddress(IHXBuffer* pIPAddress);

    STDMETHODIMP_(IHXBuffer*) GetCBID();
    STDMETHODIMP SetCBID(IHXBuffer* pCBID);

    STDMETHODIMP_(IHXBuffer*) GetGUID();
    STDMETHODIMP SetGUID(IHXBuffer* pGUID);

    STDMETHODIMP_(IHXBuffer*) GetClientID();
    STDMETHODIMP SetClientID(IHXBuffer* pClientID);

    STDMETHODIMP_(IHXBuffer*) GetPNAClientID();
    STDMETHODIMP SetPNAClientID(IHXBuffer* pPNAClientID);

    STDMETHODIMP_(IHXBuffer*) GetCompanyID();
    STDMETHODIMP SetCompanyID(IHXBuffer* pCompanyID);

    STDMETHODIMP_(IHXBuffer*) GetClientChallenge();
    STDMETHODIMP SetClientChallenge(IHXBuffer* pClientChallenge);

    STDMETHODIMP_(IHXBuffer*) GetLanguage();
    STDMETHODIMP SetLanguage(IHXBuffer* pLanguage);

    STDMETHODIMP_(IHXBuffer*) GetPlayerStartTime();
    STDMETHODIMP SetPlayerStartTime(IHXBuffer* pPlayerStartTime);

    STDMETHODIMP_(IHXBuffer*) GetProtocol();
    STDMETHODIMP SetProtocol(IHXBuffer* pProtocol);

    STDMETHODIMP_(IHXBuffer*) GetStartTime();
    STDMETHODIMP SetStartTime(IHXBuffer* pStartTime);

    STDMETHODIMP_(IHXBuffer*) GetRequestMethod();
    STDMETHODIMP SetRequestMethod(IHXBuffer* pRequestMethod);

    STDMETHODIMP_(IHXBuffer*) GetUserAgent();
    STDMETHODIMP SetUserAgent(IHXBuffer* pUserAgent);

    STDMETHODIMP_(IHXBuffer*) GetVersion();
    STDMETHODIMP SetVersion(IHXBuffer* pVersion);

    STDMETHODIMP_(IHXBuffer*) GetLoadTestPassword();
    STDMETHODIMP SetLoadTestPassword(IHXBuffer* pLoadTestPassword);

    STDMETHODIMP_(IHXBuffer*) GetRTSPEvents();
    STDMETHODIMP SetRTSPEvents(IHXBuffer* pRTSPEvents);

    STDMETHODIMP_(IHXBuffer*) GetStreamSelectionInfo();
    STDMETHODIMP SetStreamSelectionInfo(IHXBuffer* pStreamSelectionInfo);


    STDMETHODIMP_(UINT64) GetControlBytesSent();
    STDMETHODIMP SetControlBytesSent(UINT64 ulControlBytesSent);

    STDMETHODIMP_(UINT32) GetPort();
    STDMETHODIMP SetPort(UINT32 ulPort);

    STDMETHODIMP_(UINT32) GetTotalMediaAdaptations();
    STDMETHODIMP SetTotalMediaAdaptations(UINT32 ulTotalMediaAdaptations);

    STDMETHODIMP_(UINT32) GetSessionCount();
    STDMETHODIMP SetSessionCount(UINT32 ulSessionCount);

    STDMETHODIMP_(UINT32) GetSessionIndex();
    STDMETHODIMP SetSessionIndex(UINT32 ulSessionIndex);

    STDMETHODIMP_(BOOL) IsCloaked();
    STDMETHODIMP SetCloaked(BOOL bIsCloaked);

    STDMETHODIMP_(BOOL) IsRDT();
    STDMETHODIMP SetRDT(BOOL bIsRDT);


    STDMETHODIMP_(BOOL) SupportsMaximumASMBandwidth();
    STDMETHODIMP SetSupportsMaximumASMBandwidth(BOOL bSupportsMaximumASMBandwidth);

    STDMETHODIMP_(BOOL) SupportsMulticast();
    STDMETHODIMP SetSupportsMulticast(BOOL bSupportsMulticast);

    STDMETHODIMP_(IHXSessionStats*) GetSession(UINT32 ulID);
    STDMETHODIMP RemoveSession(UINT32 ulID);
    STDMETHODIMP AddSession(IHXSessionStats* pSession);
    //STDMETHODIMP_(UINT32) AddSession(IHXSessionStats* pSession, UINT32 ulId);

    STDMETHODIMP SetTimer(ClientStatsTimer* pTimer);
    //HX_RESULT RemoveTimer(IHXClientStatsSink* pSink);

    STDMETHODIMP ClearTimers();

// IHXClientStats2 methods.

    STDMETHODIMP_(UINT32) GetReasonForTermination();
    STDMETHODIMP SetReasonForTermination(UINT32 ulReasonForTermination); 

// IHXPrivateClientStats methods.

    STDMETHODIMP_(BOOL) IsPrevAuth();
    STDMETHODIMP SetPrevAuth(BOOL bIsPrevAuth);
};


///////////////////////////////////////////////////////////////////////////////
// ClientStatsManager class
///////////////////////////////////////////////////////////////////////////////

class ClientStatsManager 
{

protected:

    class SinkListElem : public HXListElem
    {
    public:

        SinkListElem() : m_pSink(NULL), m_pProc(NULL) {}

        IHXClientStatsSink* m_pSink;
        Process* m_pProc;
    };

    CHXID* m_pClientTable; 
    HXList* m_pClientList;
    HXList* m_pSinkList;
    UINT32 m_ulCurrentId;
    UINT32 m_RefCount;
    HX_MUTEX m_pMutex;

    BOOL m_bUseRegistryForStats;

public:

    ClientStatsManager();
    ~ClientStatsManager();

// IHXClientStatsManager methods.

    HX_RESULT RegisterSink(IHXClientStatsSink* pSink, Process* pProc);
    HX_RESULT RemoveSink(IHXClientStatsSink* pSink);

    HX_RESULT AddClient(IHXClientStats* pClient, Process* pProc);
    IHXClientStats* GetClient(UINT32 ulID);   
    HX_RESULT RemoveClient(UINT32 ulID, Process* pProc);
    HX_RESULT SessionDone(IHXClientStats* pClient, 
                          IHXSessionStats* pSession, 
                          Process* pProc);

    UINT32 GetClientCount();

    BOOL UseRegistryForStats();
    void SetUseRegistryForStats(BOOL bUseRegistryForStats);

    HX_RESULT ScheduleSinkNotifications(IHXClientStats* pClient, 
                                     IHXSessionStats* pSession,
                                     ClientStatsEvent nEvent,
                                     Process* pProc);
};


///////////////////////////////////////////////////////////////////////////////
// ClientStatsManagerPerProcessWrapper class
///////////////////////////////////////////////////////////////////////////////

class ClientStatsManagerPerProcessWrapper 
: public IHXClientStatsManager
{

protected:

    ClientStatsManager* m_pMgr;
    UINT32 m_RefCount;
    Process* m_pProc;

public:

    ClientStatsManagerPerProcessWrapper(ClientStatsManager* pClientStatsMgr,
                                        Process* pProc);
    ~ClientStatsManagerPerProcessWrapper();

// IUnknown methods.

    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

// IHXClientStatsManager methods.

    STDMETHODIMP RegisterSink(IHXClientStatsSink* pSink);
    STDMETHODIMP RemoveSink(IHXClientStatsSink* pSink);

    STDMETHODIMP AddClient(IHXClientStats* pClient);
    STDMETHODIMP_(IHXClientStats*) GetClient(UINT32 ulID);   
    STDMETHODIMP RemoveClient(UINT32 ulID);

    STDMETHODIMP_(UINT32) GetClientCount();

    STDMETHODIMP_(BOOL) UseRegistryForStats();
    void SetUseRegistryForStats(BOOL bUseRegistryForStats);

    STDMETHODIMP ScheduleSinkNotifications(IHXClientStats* pClient, 
                                     IHXSessionStats* pSession,
                                     ClientStatsEvent nEvent);
};


///////////////////////////////////////////////////////////////////////////////
// ClientStatsTimer class
///////////////////////////////////////////////////////////////////////////////

class ClientStatsTimer
: public IHXCallback
, public HXListElem
{

private:

    UINT32 m_RefCount;
    IHXClientStatsSink* m_pSink;
    IHXClientStats* m_pStats;
    Process* m_pSinkProc;
    Process* m_pTimerProc;
    CallbackHandle m_CBH;
    

public:

    ClientStatsTimer();
    ~ClientStatsTimer();

    HX_RESULT Init(IHXClientStats* pClientStats,
                   IHXClientStatsSink* pSink, 
                   Process* pTimerProc,
                   Process* pSinkProc);

    void Cleanup();

// IUnknown COM Interface Methods                          

    STDMETHODIMP QueryInterface(REFIID ID, 
                                void** ppInterfaceObj);
    
    STDMETHODIMP_(UINT32) AddRef();    
    STDMETHODIMP_(UINT32) Release();


// IHXCallback COM Interface Methods
    
    STDMETHODIMP Func();
};


///////////////////////////////////////////////////////////////////////////////
// ClientStatsSinkNotifyCallback class
///////////////////////////////////////////////////////////////////////////////

class ClientStatsSinkNotifyCallback 
: public SimpleCallback
{
protected:

    IHXClientStats* m_pClientStats;
    IHXSessionStats* m_pSessionStats;
    IHXClientStatsSink* m_pSink;
    Process* m_pProc;
    ClientStatsEvent m_Event;


public:

    ClientStatsSinkNotifyCallback();
    ~ClientStatsSinkNotifyCallback();

    
    HX_RESULT Init(IHXClientStats* pClientStats, 
                   IHXSessionStats* pSessionStats,
                   IHXClientStatsSink* pSink,
                   Process* pProc,
                   ClientStatsEvent Event);

    void func(Process* pProc);

    void SetClientStatsObject(IHXClientStats* pStats);

};
#endif
