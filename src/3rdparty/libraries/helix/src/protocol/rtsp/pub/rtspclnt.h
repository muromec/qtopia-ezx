/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspclnt.h,v 1.102 2008/05/06 15:41:00 anshuman Exp $
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _RTSPCLNT_H_
#define _RTSPCLNT_H_

#include "rtspif.h"
#include "rtspbase.h"
#include "rtsptran.h"
#include "rtptran.h"
#include "rtspmdsc.h"
#include "sdptypes.h"   // SdpFileType
#include "hxpreftr.h"
#include "hxbufctl.h" // IHXTransportBufferLimit
#include "hxrtsp2.h"
#include "hxrsdbf.h" // IHXResendBufferControl
#include "hxprefs.h" // IHXPreferences
#include "hxerror.h" // IHXErrorMessages
#include "hxfwctlmgr.h"
#include "baseobj.h"
#include "hxcomponent.h"
#include "unkimp.h"

class RTSPClientState;
class RTSPOptionsMessage;
class RTSPTeardownMessage;
class RTSPGetParamMessage;
class RTSPSetParamMessage;
class RTSPSetupMessage;
class RTSPAnnounceMessage;
class RTSPPlayMessage;
class RTSPRedirectMessage;
class RTSPParser;
class RTSPClientProtocol;
class CBigByteGrowingQueue;
class HXMutex;
class MIMEHeader;
class PipelinedDescribeLogic;
class CHXRateAdaptationInfo;
class HXNetSourceBufStats;

struct IHXKeyValueList;
struct IHXValues;
struct IHXStreamDescription;
struct IHXPacket;
struct IHXConnectionlessControl;
struct IHXInterruptState;
struct IHXErrorMessages;


#if defined(HELIX_CONFIG_NOSTATICS)
#include "globals/hxglobalptr.h"
#define RTSPClientSessionManagerType const RTSPClientSessionManager* const
#else
#define RTSPClientSessionManagerType RTSPClientSessionManager*
#endif /* defined(HELIX_CONFIG_NOSTATICS) */

typedef enum
{
    ALTERNATE_SERVER,
    ALTERNATE_PROXY
} ReconnectType;

typedef enum
{
    RTSPCLIENT_TIMEOUT_CONNECTION,
    RTSPCLIENT_TIMEOUT_KEEPALIVE
} RTSPClientTimeoutType;

typedef enum
{
    RTSPCLIENTSESSION_TIMEOUT_AUTOBWDETECTION
} RTSPClientSessionTimeoutType;

///////////////////////////////////////////////////////////////////////////////
// AutoBWDetectionState RTSPClientSession::m_autoBWDetectionState
//
// F - False/Failure
// S - Success
// Y - Yes
// N - No
//                    ABD_STATE_INIT
//                         |
//                         | check "AutBWDetection" preference
//                     0  / \  1
// ABD_STATE_DISABLED<----   ----->ABD_STATE_INQUERY
//                                        |
//                                        | check "Supported:ABD-1.0" in OPTIONS response
//                                   F   / \  S
//                                -------   ----------------->ABD_STATE_REQUEST
//                                |                                  |
//                ABD Calibration |                                  | send&recv GET_PARAM request for ABD packets
//                             N / \ Y                           F  / \ S
//             ABD_STATE_DONE<---   --->ABD_STATE_WAITING       ----   ----> ABD_STATE_DONE
//                                             |                |
//                                             |                | ABD Calibration
//                                             V             N / \ Y
//                                        ABD_STATE_DONE<------   ---->ABD_STATE_WAITING
//                                                                           |
//                                                                           |
//                                                                           V
//                                                                      ABD_STATE_DONE
//
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
    ABD_STATE_DISABLED,         // ABD is disabled from the preference
    ABD_STATE_NOTSUPPORTED,     // ABD is not supported by the server
    ABD_STATE_INIT,             // default start state
    ABD_STATE_INQUERY,          // inquery whether server supports ABD via RTSP OPTIONS request
    ABD_STATE_REQUEST,          // ask server to send ABD prob packets
    ABD_STATE_WAITING,          // waiting for ABD calibration result
    ABD_STATE_DONE              // done with ABD
} AutoBWDetectionState;

typedef enum
{
    ENTER_PREFETCH = 0,
    LEAVE_PREFETCH,
    ENTER_FASTSTART,
    LEAVE_FASTSTART,
    PAUSE_BUFFER,
    RESUME_BUFFER
} TRANSPORT_MSG;

typedef struct _ReconnectInfo
{
    CHXString   m_server;
    UINT32      m_ulPort;
} ReconnectInfo;

typedef struct _RTSPClientProtocolInfo
{
    RTSPClientProtocol* m_pProt;
    CHXSimpleList       m_seqNoList;
    CHXMapLongToObj     m_interleaveMap;
} RTSPClientProtocolInfo;

typedef enum _RTSPIndex
{
    SETUP           = 0,
    REDIRECT        = 1,
    PLAY            = 2,
    PAUSE           = 3,
    SET_PARAM       = 4,
    GET_PARAM       = 5,
    OPTIONS         = 6,
    DESCRIBE        = 7,
    TEARDOWN        = 8,
    RECORD          = 9,
    ANNOUNCE        = 10
} RTSPIndex;
const UINT32 RTSP_TABLE_SIZE = 11;

typedef enum _ConnectionState
{
    CONN_INIT,
    CONN_PENDING,
    CONN_READY,
    CONN_CLOSED
} ConnectionState;

struct RTSPTableEntry
{
    const char* pMethod;
    RTSPIndex   index;
};

class RTSPTransportInfo
{
public:
    RTSPTransportInfo           ();
    ~RTSPTransportInfo          ();
    HXBOOL containsStreamNumber   (UINT16 streamNumber);
    void addStreamNumber        (UINT16 streamNumber);
    RTSPTransport*              m_pTransport;
    RTCPBaseTransport*          m_pRTCPTransport;
    UINT16                      m_sPort;
    UINT16                      m_sResendPort;

private:
    CHXSimpleList               m_streamNumberList;
};


class RTSPTransportRequest
{
public:
    RTSPTransportRequest        (RTSPTransportTypeEnum tType, UINT16 sPort);
    ~RTSPTransportRequest       ();
    HX_RESULT addTransportInfo  (RTSPTransport* pTransport,
                                RTCPBaseTransport* pRTCPTransport,
                                UINT16 streamNumber);
    HX_RESULT addTransportInfo  (RTSPTransport* pTransport,
                                RTCPBaseTransport* pRTCPTransport,
                                UINT16 streamNumber,
                                IHXSockAddr* pAddr);
    RTSPTransportInfo*          getTransportInfo(UINT16 streamNumber);
    void                        ResetTransports();
    RTSPTransportInfo*          getFirstTransportInfo();
    RTSPTransportInfo*          getNextTransportInfo();

    RTSPTransportTypeEnum       m_lTransportType;
    UINT16                      m_sPort;
    UINT16                      m_sResendPort;
    INT8                        m_tcpInterleave;
    HXBOOL                        m_bDelete;

private:
    CHXSimpleList               m_transportInfoList;
    LISTPOSITION                m_lListPos;
};



class RTSPClientSession : public IHXSocketResponse,
                          public IHXInterruptSafe,
                          public IHXAutoBWDetection,
                          public IHXAutoBWDetectionAdviseSink,
                          public IHXConnectionBWAdviseSink
{
public:
    RTSPClientSession(void);
    virtual ~RTSPClientSession(void);

    HX_RESULT Done                      ();
    HX_RESULT Init                          (IUnknown* pContext,
                                        RTSPClientProtocol* pProt,
                                        IHXSockAddr* pConnectAddr,
                                        const char* pHostName,
                                        UINT16 hostPort,
                                        HXBOOL bUseProxy,
                                        HXBOOL bHTTPCloak);

    /* IUnknown methods */
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    /* IHXSocketResponse methods */
    STDMETHOD(EventPending)             (THIS_ UINT32 uEvent, HX_RESULT status);

    /* Old IHXTCPResponse methods */
    STDMETHOD(ConnectDone)              (THIS_ HX_RESULT status);
    STDMETHOD(ReadDone)                 (THIS_ HX_RESULT status, IHXBuffer* pBuffer);

    /* IHXInterruptSafe methods */
    STDMETHOD_(HXBOOL,IsInterruptSafe)    (THIS) {return TRUE;};

    /*
     *  IHXAutoBWDetection methods
     */
    STDMETHOD(InitAutoBWDetection)              (THIS_
                                                 HXBOOL bEnabled);
    STDMETHOD(AddAutoBWDetectionSink)           (THIS_
                                                 IHXAutoBWDetectionAdviseSink* pSink);
    STDMETHOD(RemoveAutoBWDetectionSink)        (THIS_
                                                 IHXAutoBWDetectionAdviseSink* pSink);
    /*
     *  IHXAutoBWDetectionAdviseSink methods
     */
    STDMETHOD(AutoBWDetectionDone)      (THIS_
                                         HX_RESULT  status,
                                         UINT32     ulBW);

    /*
     * IHXConnectionBWAdviseSink methods
     */
    STDMETHOD(NewConnectionBW)(THIS_ UINT32 uConnectionBW);

    class TimeoutCallback : public IHXCallback
    {
    public:
        TimeoutCallback(RTSPClientSession* pOwner,
                        RTSPClientSessionTimeoutType type);
        ~TimeoutCallback();

        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        STDMETHOD(Func)                 (THIS);

    private:
        LONG32                          m_lRefCount;
        RTSPClientSession*              m_pOwner;
        RTSPClientSessionTimeoutType    m_type;
    };

    HX_RESULT addProtocol                   (RTSPClientProtocol* pProt);
    HX_RESULT removeProtocol        (RTSPClientProtocol* pProt);
    int getProtocolCount                    ();

    HX_RESULT setProtocolInterleave         (RTSPClientProtocol* pProt,
                                            INT8 interleave);
    HX_RESULT setProtocolSeqNo      (RTSPClientProtocol* pProt,
                                            UINT32 seqNo);
    HX_RESULT removeProtocolSeqNo           (RTSPClientProtocol* pProt,
                                            UINT32 seqNo);
    HXBOOL isEmpty                            ();
    IHXSocket* getSocket         ();
    HX_RESULT closeSocket                   ();
    HX_RESULT reopenSocket                  (RTSPClientProtocol* pProt);
    UINT32 getNextSeqNo             (RTSPClientProtocol* pProt);
    HXBOOL HttpOnly                           ();

    HXBOOL                  amIDoingABD(RTSPClientProtocol* pProtocol) { return (pProtocol == m_pConnectingProt)?TRUE:FALSE; };
    AutoBWDetectionState    getABDState(void) { return m_autoBWDetectionState; };
    void                    setABDState(AutoBWDetectionState state);

    void updateABDState(HX_RESULT status);
    UINT32 getConnectionBW(void);

    HXBOOL                            m_bIgnoreSession;

    // host:port from URL
    CHXString                       m_hostName;
    UINT16                          m_hostPort;

    // resolved address for host we connect to (based on proxy and cloak mode)
    IHXSockAddr*                    m_pConnectAddr;

    HXBOOL                          m_bUseProxy;
    HXBOOL                          m_bHTTPCloak;
    IUnknown*                       m_pContext;
    HXBOOL                          m_bReopenSocket;


    HXBOOL                          m_bChallengeDone;
    HXBOOL                          m_bChallengeMet;
    VOLATILE HXBOOL                 m_bIsValidChallengeT;

    ConnectionState                 m_connectionState;

    void ReportError( HX_RESULT theErr, 
                      const UINT8 unSeverity = HXLOG_ERR,
                      const char* pUserString = NULL );

    UINT32 GetEmptySessionLingerTimeout() const;

protected:

    HX_RESULT StartCloakingSession(IUnknown* pContext);

private:
    HX_RESULT handleInput                   (IHXBuffer* pBuffer);
    RTSPClientProtocol*
        findProtocolFromInterleave          (INT8 interleave);
    RTSPClientProtocol*
        findProtocolFromSeqNo               (UINT32 seqNo);
    RTSPClientProtocol*
        findProtocolFromSessionID           (CHXString* pszSessionID);

    void getSessionID               (RTSPMessage* pMsg, CHXString* pszSessionID);
    HX_RESULT CreateAndConnectSessionSocket(RTSPClientProtocol* pProt);


    INT32                           m_lRefCount;
    IHXNetServices*                 m_pNetServices;
    IHXSocket*                      m_pSocket;
    UINT32                          m_ulLastSeqNo;
    CBigByteGrowingQueue*           m_pInQueue;
    RTSPParser*                     m_pParser;
    HXBOOL                          m_bSessionDone;
    HXBOOL                          m_bSetSessionCalled;

    IHXScheduler*                   m_pScheduler;
    IHXConnectionBWInfo*            m_pConnBWInfo;
    IHXPreferences*                 m_pPreferences;
    LISTPOSITION                    m_abdDispatchItr;
    TimeoutCallback*                m_pAutoBWDetectionCallback;
    UINT32                          m_ulAutoBWDetectionCallbackHandle;
    CHXSimpleList*                  m_pAutoBWDetectionSinkList;
    CHXSimpleList                   m_protList;
    RTSPClientProtocol*             m_pConnectingProt;
    AutoBWDetectionState            m_autoBWDetectionState;

    void dispatchABDDoneCalls(HX_RESULT status, UINT32 ulBW);

    // used in Connect/ConnectDone
    IHXMutex*                       m_pMutex;

    int m_nCloakVerMajor;
    int m_nCloakVerMinor;
    UINT32                          m_emptySessionLingerTimeout;
};

inline
UINT32 RTSPClientSession::GetEmptySessionLingerTimeout() const
{
    return m_emptySessionLingerTimeout;
}

class SessionLingerTimeout;

class RTSPClientSessionManager : public IUnknown
{
public:
    virtual ~RTSPClientSessionManager(void);

    static RTSPClientSessionManager*    instance(IUnknown* pContext);
    HXBOOL isValid                        () { return SessionManGlobal() != NULL; }

    /*
     * IUnknown methods
     */

    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    /*
     * RTSPClientSessionManager methods
     */
    virtual HX_RESULT CreateSessionInstance(RTSPClientSession*& pSession);

    HX_RESULT NewSession       (IUnknown* pContext,
                                        RTSPClientProtocol* pProt,
                                        IHXSockAddr* pConnectAddr,  // actual connect addr:port
                                        const char* pHostName,      // host from request URL
                                        UINT16 hostPort,            // port from request URL
                                        HXBOOL bUseProxy,
                                        HXBOOL bHTTPCloak);


    HX_RESULT RemoveFromSession         (RTSPClientProtocol* pProt,
                                        RTSPClientSession* pSessionRemoved);

    RTSPClientSession* FindSession      (IHXSockAddr* pConnectAddr, // actual connect addr:port
                                        const char* pHostName,      // host from request URL
                                        UINT16 hostPort,            // port from request URL
                                        HXBOOL bUseProxy,
                                        IUnknown* pContext = NULL);

    int     GetSessionCount             ();

    HXBOOL  MatchPlayerContext          (IUnknown* pNewContext, IUnknown* pKnownContext);

    void    CheckLingerTimeout          ();
    void    OnLingerTimeout             (RTSPClientSession* pSession);

protected:
    RTSPClientSessionManager(IUnknown* pContext);
    void   Close();
    void   DoRemoveEmptySession(RTSPClientSession* pSession, LISTPOSITION pos = NULL);

    static RTSPClientSessionManagerType zm_pSessionManager;

    static RTSPClientSessionManager*& SessionManGlobal();

    IHXMutex*                           m_pMutex;
    SessionLingerTimeout*               m_pLingerTimeout;
    IUnknown*                           m_pContext;
    CHXSimpleList                       m_sessionList;

private:
    INT32                               m_lRefCount;
};

class RTSPClientProtocol: public CUnknownIMP, 
			  public RTSPBaseProtocol,
			  public IHXRTSPClientProtocol,
			  public IHXRTSPClientProtocol2,
                          public IHXRTSPClientTransportResponse,
                          public IHXPendingStatus,
                          public IHXStatistics,
                          public IHXThinnableSource,
                          public IHXPacketResend,
                          public IHXInterruptSafe,
                          public IHXResendBufferControl,
                          public IHXResendBufferControl2,
                          public IHXTransportSyncServer,
                          public IHXTransportBufferLimit,
                          public IHXResolveResponse,
			  public CHXBaseCountingObject
{
public:
    DECLARE_MANAGED_COMPONENT(RTSPClientProtocol);

    RTSPClientProtocol                  ();
    ~RTSPClientProtocol                 ();

    enum State
    {
        INIT,
        READY,
        PLAYING,
        RECORDING
    };

    class UDPResponseHelper: public IHXSocketResponse,
                             public IHXInterruptSafe
    {
    private:
        LONG32                  m_lRefCount;
        IHXSocket*              m_pSock;
        IHXSockAddr*            m_pLocalAddr;
        RTSPClientProtocol*     m_pOwner;

    public:
        UDPResponseHelper(RTSPClientProtocol* pParent);
        ~UDPResponseHelper(void);

        void SetSock(IHXSocket* pSock);

        /* IUnknown */
        STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)      (THIS);
        STDMETHOD_(ULONG32,Release)     (THIS);

        /* IHXInterruptSafe */
        STDMETHOD_(HXBOOL,IsInterruptSafe)(THIS) { return TRUE; }

        /* IHXSocketResponse methods */
        STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);
    };

    class TimeoutCallback : public IHXCallback
    {
    public:
        TimeoutCallback(RTSPClientProtocol* pOwner,
                        RTSPClientTimeoutType type);
        ~TimeoutCallback();

        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        STDMETHOD(Func)                 (THIS);

    private:
        LONG32                          m_lRefCount;
        RTSPClientProtocol*             m_pOwner;
        RTSPClientTimeoutType           m_type;
    };

#ifdef _MACINTOSH
    class RTSPClientProtocolCallback : public IHXCallback
    {
    public:
        RTSPClientProtocolCallback(RTSPClientProtocol* pOwner);
        ~RTSPClientProtocolCallback();

    /* IUnknown Interfaces */

        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        STDMETHOD(Func)                 (THIS);

        LONG32                  m_lRefCount;
        RTSPClientProtocol*     m_pOwner;
        HXBOOL                    m_bIsCallbackPending;
        CallbackHandle          m_Handle;
        IHXValues*              m_pPendingRequestHeaders;
        CHXString               m_PendingDescURL;
    };

    friend class RTSPClientProtocolCallback;
#endif /* _MACINTOSH */

    /*
     * IUnknown methods
     */

    STDMETHOD(_QueryInterface)          (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    /*
     * IHXRTSPClientProtocol methods
     */

    STDMETHOD(Init)                     (THIS_
                                        IUnknown*   pContext,
                                        const char* pHostName,
                                        UINT16      hostPort,
                                        IHXRTSPClientProtocolResponse* pClient,
                                        UINT32      initializationType,
                                        IHXValues* pSessionHeaders,
                                        IHXValues* pInfo,
                                        HXBOOL        bHTTPCloak,
                                        UINT16      cloakPort,
                                        HXBOOL        bNoReuseConnection);
    STDMETHOD(SetBuildVersion)          (THIS_
                                        const char* pVersionString);
    STDMETHOD(Done)                     (THIS);
    STDMETHOD_(HXBOOL, IsRateAdaptationUsed)(THIS_);
    STDMETHOD(SendStreamDescriptionRequest)
                                        (THIS_
                                        const char* pURL,
                                        IHXValues* pRequestHeaders);
    STDMETHOD(SendStreamRecordDescriptionRequest)
                                        (THIS_
                                        const char* pURL,
                                        IHXValues* pFileHeader,
                                        CHXSimpleList* pStreams,
                                        IHXValues* pRequestHeaders);
    STDMETHOD(SendSetupRequest)
    (
        THIS_
        RTSPTransportType* pTransType,
        UINT16 nTransTypes,
        IHXValues* pIHXValuesRequestHeaders
    );

    STDMETHOD(SendPlayRequest)          (THIS_
                                        UINT32 lFrom,
                                        UINT32 lTo,
                                        CHXSimpleList* pSubscriptions);
    STDMETHOD(SendRecordRequest)        (THIS);
    STDMETHOD(SendPauseRequest)         (THIS);
    STDMETHOD(SendResumeRequest)        (THIS);
    STDMETHOD(SendTeardownRequest)      (THIS);
    STDMETHOD(SendSetParameterRequest)  (THIS_
                                        UINT32 lParamType,
                                        const char* pParamName,
                                        IHXBuffer* pParamValue);
    STDMETHOD(SendSetParameterRequest)  (THIS_
                                        const char* pParamName,
                                        const char* pParamValue,
                                        const char* pMimeType,
                                        const char* pContent);
    STDMETHOD(SendGetParameterRequest)  (THIS_
                                        UINT32 lParamType,
                                        const char* pParamName);
    STDMETHOD(SendPacket)               (THIS_
                                        BasePacket* pPacket);
    STDMETHOD(SendStreamDone)           (THIS_
                                        UINT16 streamNumber);
    STDMETHOD(SendPlayerStats)          (THIS_
                                        const char* pStats);
    STDMETHOD(SendKeepAlive)            (THIS);
    STDMETHOD(GetPacket)                (THIS_
                                        UINT16 uStreamNumber,
                                        REF(IHXPacket*) pPacket);
    STDMETHOD(StartPackets)             (THIS_
                                        UINT16 uStreamNumber);
    STDMETHOD(StopPackets)              (THIS_
                                        UINT16 uStreamNumber);
    STDMETHOD(SetProxy)                 (THIS_
                                        const char* pProxyHost,
                                        UINT16 proxyPort);
    STDMETHOD(SetResponse)              (THIS_
                                        IHXRTSPClientProtocolResponse* pResp);

    // IHXRTSPClientProtocol2 
    STDMETHOD_(UINT16, GetRDTFeatureLevel)  (THIS) { return 0; };
    STDMETHOD(EnterPrefetch)		    (THIS) { m_bPrefetch = TRUE; return HXR_OK; };
    STDMETHOD(LeavePrefetch)		    (THIS);
    STDMETHOD(EnterFastStart)		    (THIS);
    STDMETHOD(LeaveFastStart)		    (THIS);
    STDMETHOD(InitCloak)		    (THIS_
					    UINT16* pCloakPorts, 
					    UINT8 nCloakPorts, 
					    IHXValues* pValues);
    STDMETHOD(SetStatistics)		    (THIS_
					    UINT16 uStreamNumber, 
					    STREAM_STATS* pStats);

    /* IHXResolveResponse methods */
    STDMETHOD(GetAddrInfoDone)      (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone)      (THIS_ HX_RESULT status, const char* pNode, const char* pService);


    /*
     * IHXThinnableSource methods.
     */

    /************************************************************************
     *  Method:
     *      IHXThinnableSource::DropByN
     *  Purpose:
     *
     *  Implemented by protocols that allow infinite thinnability through
     *      LimitBandwidthByDropping
     */

    STDMETHOD(LimitBandwidthByDropping) (THIS_
                                        UINT32 ulStreamNo,
                                        UINT32 ulBandwidthLimit);

    STDMETHOD(SetDeliveryBandwidth)     (THIS_
                                        UINT32 ulBandwidth,
                                        UINT32 ulMsBackoff);

    /*
     * XXX...The following 3 functions had better be removed under
     *       full IRMA
     */

    STDMETHOD_(IHXPendingStatus*, GetPendingStatus)     (THIS);
    STDMETHOD_(IHXStatistics*, GetStatistics)           (THIS);
    STDMETHOD_(HXBOOL, HttpOnly)                          (THIS);

    STDMETHOD(Subscribe)                (THIS_
                                        CHXSimpleList* pSubscriptions);
    STDMETHOD(Unsubscribe)              (THIS_
                                        CHXSimpleList* pUnsubscriptions);
    STDMETHOD(RuleChange)               (THIS_
                                        CHXSimpleList* pRuleChanges);
    STDMETHOD(BackChannelPacketReady)   (THIS_
                                        IHXPacket* pPacket);
    STDMETHOD(SendRTTRequest)           (THIS);

    STDMETHOD(SendBWReport)             (THIS_
                                        INT32 aveBandwidth,
                                        INT32 packetLoss,
                                        INT32 bandwidthWanted);
    STDMETHOD(SetFirstSeqNum)           (THIS_
                                        UINT16 uStreamNumber,
                                        UINT16 uSeqNum);

    STDMETHOD(SetRTPInfo)               (THIS_
                                        UINT16 uStreamNumber,
                                        UINT16 uSeqNum,
                                        UINT32 ulRTPTime,
                                        RTPInfoEnum info
                                        );

    STDMETHOD(InitSockets)              (THIS);

    STDMETHOD(GetCurrentBuffering)      (THIS_
                                        UINT16 uStreamNumber,
                                        REF(UINT32) ulLowestTimestamp,
                                        REF(UINT32) ulHighestTimestamp,
                                        REF(UINT32) ulNumBytes,
                                        REF(HXBOOL) bDone);

    STDMETHOD(SeekFlush)                (THIS);

    STDMETHOD_(HXBOOL, IsDataReceived)    (THIS);

    STDMETHOD_(HXBOOL,IsSourceDone)       (THIS);

    /*
     *  IHXInterruptSafe methods
     */
    STDMETHOD_(HXBOOL,IsInterruptSafe)    (THIS) {return TRUE;};

    /*
     * IHXRTSPClientTransportResponse methods
     */

    STDMETHOD(PacketReady)              (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        IHXPacket* pPacket);

    STDMETHOD(OnRTTRequest)             (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID
                                        );

    STDMETHOD(OnRTTResponse)            (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        UINT32 ulSecs,
                                        UINT32 ulUSecs);

    STDMETHOD(OnBWReport)               (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        INT32 aveBandwidth,
                                        INT32 packetLoss,
                                        INT32 bandwidthWanted
                                        );

    STDMETHOD(OnCongestion)             (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        INT32 xmitMultiplier,
                                        INT32 recvMultiplier);

    STDMETHOD(OnACK)                    (THIS_
                                        HX_RESULT status,
                                        RTSPResendBuffer* pResendBuffer,
                                        UINT16 uStreamNumber,
                                        const char* pSessionID,
                                        UINT16* pAckList,
                                        UINT32 uAckListCount,
                                        UINT16* pNakList,
                                        UINT32 uNakListCount);

    STDMETHOD(OnStreamDone)             (THIS_
                                        HX_RESULT status,
                                        UINT16 uStreamNumber);

    STDMETHOD(OnSourceDone)             (THIS);


    STDMETHOD(OnProtocolError)          (THIS_
                                        HX_RESULT status);

    /*
     * IHXPendingStatus methods
     */

    STDMETHOD(GetStatus)                (THIS_
                                        REF(UINT16) uStatusCode,
                                        REF(IHXBuffer*) pStatusDesc,
                                        REF(UINT16) ulPercentDone);

    /*
     *  IHXStatistics methods
     */

    STDMETHOD (InitializeStatistics)    (THIS_
                                        UINT32  /*IN*/ ulRegistryID);

    STDMETHOD (UpdateStatistics)        (THIS);

    STDMETHOD(OnPacket)                 (THIS_
                                        UINT16 uStreamNumber,
                                        BasePacket** ppPacket);

    STDMETHOD(SetConnectionTimeout)     (THIS_
                                        UINT32 uSeconds);

    STDMETHOD_(UINT16, GetProtocolType) (THIS) {return m_uProtocolType;};

    STDMETHOD(InitPacketFilter)         (THIS_
                                        RawPacketFilter* pFilter);
    STDMETHOD(SetUseRTPFlag)         (THIS_
                                     HXBOOL bUseRTP);

    /*
     *  IHXResendBufferControl methods
     */
    STDMETHOD(SetResendBufferDepth)     (THIS_
                                        UINT32 uSeconds);

    /*
     *  IHXResendBufferControl2 methods
     */

    STDMETHOD(SetResendBufferParameters) (THIS_
                                          UINT32 uMinimumDelay,      /* ms */
                                          UINT32 uMaximumDelay,      /* ms */
                                          UINT32 uExtraBufferingDelay/* ms */);

    /*
     * IHXTransportSyncServer methods
     */
    STDMETHOD(DistributeSyncAnchor) (THIS_
                                    ULONG32 ulHXTime,
                                    ULONG32 ulNTPTime);

    STDMETHOD(DistributeSync)       (ULONG32 ulHXTime,
                                    LONG32 lHXTimeOffset);

    STDMETHOD(DistributeStartTime)  (ULONG32 ulHXRefTime);

    /*
     * IHXTransportBufferLimit methods
     */

    /************************************************************************
     *  Method:
     *      IHXTransportBufferLimit::SetByteLimit
     *  Purpose:
     *      Sets the maximum number of bytes that can be buffered in the
     *      transport buffer. If incomming packets would put us over this
     *      limit, then they are replaced with lost packets. A byte limit
     *      of 0 means unlimited buffering.
     */
    STDMETHOD(SetByteLimit) (THIS_ UINT16 uStreamNumber,
                             UINT32 uByteLimit);

    /************************************************************************
     *  Method:
     *      IHXTransportBufferLimit::GetByteLimit
     *  Purpose:
     *      Returns the current byte limit in effect. A value of 0 means
     *      unlimited buffering is allowed
     */
    STDMETHOD_(UINT32,GetByteLimit) (THIS_ UINT16 uStreamNumber);

    /*
     *  RTSPClientProtocol public methods
     */

    HX_RESULT(ReadFromDone)             (HX_RESULT status,
                                         IHXBuffer* pBuffer,
                                         IHXSockAddr* pSource,
                                         IHXSockAddr* pDest);

    HX_RESULT HandleUnexpected          (RTSPMessage* pMsg);
    HX_RESULT HandleBadVersion          (RTSPMessage* pMsg);
    HX_RESULT HandleOptions             (RTSPOptionsMessage* pMsg);
    HX_RESULT HandleTeardown            (RTSPTeardownMessage* pMsg);
    HX_RESULT HandleGetParam            (RTSPGetParamMessage* pMsg);
    HX_RESULT HandleSetParam            (RTSPSetParamMessage* pMsg);
    HX_RESULT HandleRedirect            (RTSPRedirectMessage* pMsg);
    HX_RESULT HandleUseProxy            (RTSPResponseMessage* pMsg);
    void      SessionCreated            (RTSPClientSession* pSession);
    void      SessionSucceeded          (RTSPClientSession* pSession,
                                        IHXSocket* pSocket);
    void      SessionFailed             (RTSPClientSession* pSession,
                                        IHXSocket* pSocket);
    HX_RESULT InitDone                  (HX_RESULT status);
    void      AutoBWDetectionDone       (HX_RESULT status, UINT32 ulBW);
    void DoConnectionCheck              ();
    void GetOriginHostPort              (CHXString& host, UINT16& port);
    void SetSplitterConsumer            (HXBOOL);
    UINT16 GetCloakPortSucceeded        (void);
    HXBOOL IsSessionSucceeded           (){return m_bSessionSucceeded;};


    HX_RESULT   InitExt(IUnknown*   pContext,
                        const char* pHostName,
                        UINT16      hostPort,
                        IHXRTSPClientProtocolResponse* pClient,
                        UINT32      initializationType,
                        IHXValues* pSessionHeaders,
                        IHXValues* pInfo,
                        HXBOOL        bHTTPCloak,
                        UINT16      cloakPort,
                        HXBOOL        bNoReuseConnection);

    friend class RTSPClientSession;

    void ReportError( HX_RESULT theErr );
protected:

    // InitExt helpers
    HX_RESULT InitExtInitPrefs();
    HX_RESULT InitExtInitSDP(IHXValues* pInfo, CHXString& strHostOut, UINT16& hostPortOut);

    friend class CHXPipelinedDescribeLogic;

    HX_RESULT handleMessage             (RTSPMessage* pMsg);
    HX_RESULT handleTCPData             (BYTE* pData, UINT16 dataLen, UINT16 channel, UINT32 ulTimeStamp = 0);
    HX_RESULT handleOptionsResponse     (RTSPResponseMessage* pMsg,
                                         RTSPOptionsMessage*  pOptionsMsg);
    HX_RESULT handleGetParamResponse    (RTSPResponseMessage* pMsg);
    HX_RESULT handleSetParamResponse    (RTSPResponseMessage* pMsg, 
                                         RTSPSetParamMessage* pReqMsg);
    HX_RESULT handleTeardownResponse    (RTSPResponseMessage* pMsg);
    HX_RESULT handlePlayResponse        (RTSPResponseMessage* pMsg,
                                        RTSPPlayMessage* pPlayMsg);
    HX_RESULT handleRecordResponse      (RTSPResponseMessage* pMsg);
    HX_RESULT handlePauseResponse       (RTSPResponseMessage* pMsg);
    HX_RESULT handleSetupResponse       (RTSPResponseMessage* pMsg,
                                        RTSPSetupMessage* pSetupMsg);
    HX_RESULT handleDescribeResponse    (RTSPResponseMessage* pMsg);
    HX_RESULT handleAnnounceResponse    (RTSPResponseMessage* pMsg);
    HX_RESULT HandleRedirectResponse    (RTSPResponseMessage* pMsg);
    HX_RESULT sendInitialMessage        (RTSPClientSession* pSession,
                                        IHXSocket* pSocket);
    HX_RESULT addTransportMimeType      (MIMEHeaderValue* pValue,
                                        UINT16 streamNumber);

    HX_RESULT _SetRTPInfo		(UINT16 uStreamNumber,
                                         UINT16 uSeqNum,
                                         UINT32 ulRTPTime,
                                         RTPInfoEnum info,
					 HXBOOL bOnPauseResume = FALSE);
    void NotifyStreamsRTPInfoProcessed  (HXBOOL bOnPauseResume = FALSE);

    HXBOOL DetermineIfPreferenceUseRTP(HXBOOL& bUseRTP);

    HX_RESULT sendFirstSetupRequest
    (
        IHXValues* pIHXValuesRequestHeaders
    );
    HX_RESULT sendRemainingSetupRequests();
    HX_RESULT sendSetupRequestMessage
    (
        RTSPStreamInfo* pStreamInfo,
        IHXValues* pIHXValuesRequestHeaders,
        HXBOOL bFirstSetup
    );
    const char* allowedMethods          ();
    RTSPTransportRequest* getTransportRequest(MIMEHeaderValue* pValue);
    void reset();
    void clearTransportRequestList      ();
    void clearStreamInfoList            ();
    void clearUDPResponseHelperList     ();
    void clearRequestPendingReplyList(CHXSimpleList& requestPendingReplyList);
    HXBOOL trimRequestPendingReplyList(CHXSimpleList& requestPendingReplyList, 
				       UINT32 ulReplySeqNum,
				       UINT32* pNumRepliesTrimmed = NULL);
    HX_RESULT augmentRequestPendingReplyList(CHXSimpleList& requestPendingReplyList,
					     UINT32 ulReplySeqNum);
    void clearSocketStreamMap(CHXMapLongToObj*& pSocketStreamMap);
    HX_RESULT getStreamDescriptionMimeType  (char*& pMimeType);
    IHXStreamDescription*
              getStreamDescriptionInstance(const char* pMimeType);

    HX_RESULT closeSocket               ();
    HX_RESULT reopenSocket              ();

    HX_RESULT ReopenSocketDone          (HX_RESULT status);
    HX_RESULT ConnectDone               (HX_RESULT status);

    HX_RESULT sendPendingStreamDescription(const char* pURL,
                                           IHXValues* pRequestHeaders,
                                           HXBOOL bLockMutex = TRUE);

    // only for [RTP|RDT]/TCP
    void mapTransportChannel            (RTSPTransport* pTran, UINT16 nChannel);

    // Interop - control number could be anything...
    void mapControlToStreamNo           (const char* pControl, UINT16 uStreamNo);
    HXBOOL getStreamNoFromControl         (const char* pControl, REF(UINT16) uStreamNo);

    CHXString getSetupRequestURL(RTSPStreamInfo* pStreamInfo) const;
    CHXString getAggControlURL() const;
    RTSPStreamInfo* getStreamInfoFromSetupRequestURL(const char* pUrl);

    /* interop                                                         */
    /* since Allow doesn't deal with encoder stuff, just do it here    */
    /* depending on the result, we will use either old or new sdp file */
    SdpFileType     GetSdpFileTypeWeNeed(IHXValues* pHeaders);

    HX_RESULT       GetStreamDescriptionInfo(IUnknown* pUnknown, CHXString& mimeTypes);
    void            SendMsgToTransport(TRANSPORT_MSG msg);
    void            AddCommonHeaderToMsg(RTSPRequestMessage* pMsg);
    HX_RESULT       SendMsgToServer(RTSPMethod msg, UINT32* pulMsgSeqNum = NULL);
    HX_RESULT       SendOptionsMsgToServer(HXBOOL bKeepAlive);

    HX_RESULT extractRealmInformation(RTSPResponseMessage* pMsg);
    HX_RESULT extractExistingAuthorizationInformation(IHXValues* pIHXValuesRequestHeaders);
    void      appendAuthorizationHeaders(/*RTSPDescribeMessage*/ RTSPMessage* pMsg);

    HX_RESULT   RetrieveReconnectInfo(MIMEHeader*   pHeader,
                                      ReconnectType reconnectType,
                                      IHXValues*&  pReconnectValues);

    HX_RESULT   handleAuthentication(RTSPResponseMessage* pMsg);

    HX_RESULT   ParseSDP(const char* pszContentType, IHXBuffer* pSDPBuffer);
    HX_RESULT   SetMulticastAddrHelper(RTSPStreamInfo* pInfo, const char* pszAddr, UINT16 port);

    void        RemoveSDPHeaders(void);

#if defined(HELIX_FEATURE_RTSP_RESP_SINK)
    void        LogRTSPResponseMessage(RTSPResponseMessage* pMsg);
#endif

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    HX_RESULT   FinishSDPInit();
    HXBOOL        DetermineIfRMPresentation(IHXValues** ppStrmHeaders,
                                          UINT32 ulNumStreams);

    HXBOOL        GetSubscriptionBW(IHXValues*    pFileHeader,
                                  IHXValues**   ppStrmHeaders,
                                  UINT16        unNumStrmHeaders,
                                  REF(UINT32*)  pulSubscriptionBW,
                                  UINT32        ulNumStreams,
                                  REF(UINT32)   ulRuleNumber);

    HXBOOL        GetRightHeaders(REF(IHXValues**)    ppRealHeaders, // out
                                UINT32              ulNumStreams,
                                IHXValues**         ppHeaders,
                                UINT32              cHeaders,
                                UINT32*             pulSubscriptionBW,
                                UINT32              ulRuleNumber);

    HX_RESULT HandleSetParamMulticastTransportHelper(RTSPResponseMessage* pMsg);

    HX_RESULT   preparePOSTStatsMsg(CHXString& host,
                                    UINT32& port,
                                    CHXString& resource,
                                    REF(IHXValues*) pPOSTHeader);
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

    HXBOOL        GetStreamCountNoTrust(IHXValues**   ppHeaders,
                                      UINT16        unNumHeader,
                                      REF(UINT32)   ulNumStream);

    HX_RESULT   CreateUDPSockets(UINT32 ulStream, UINT32 ulBitRate, UINT16 ulPort);
    RTSPTransport* GetTransport(UINT16 idxStream);

    HXBOOL                IsRealDataType(void);
    virtual HXBOOL        IsRealServer(void);
    virtual HX_RESULT   RetrieveChallenge(RTSPResponseMessage* pMessage);
    virtual HX_RESULT   sendSetupRequestMessageExt(RTSPStreamInfo* pStreamInfo,
                                                   IHXValues*& pIHXValuesRequestHeaders,
                                                   HXBOOL bFirstSetup,
                                                   RTSPSetupMessage*& pMsg);
    virtual HX_RESULT   handleSetupResponseExt(RTSPStreamInfo* pStreamInfo,
                                               RTSPResponseMessage* pMsg,
                                               RTSPSetupMessage* pSetupMsg);
    virtual void UpdateChosenTransport(RTSPResponseMessage* pMsg);
    virtual HX_RESULT   DoSendRTTRequest(void);
    virtual HX_RESULT   DoSendBWReport(INT32 aveBandwidth,
                                       INT32 packetLoss,
                                       INT32 bandwidthWanted);
    void addUAProfHeaders(IHXValues *pHeaders);
    void addRateAdaptationHeaders(RTSPMessage* pMsg,
                                  RTSPStreamInfo* pStreamInfo);
    void handleRateAdaptResponse(RTSPRequestMessage* pReq,
                                 RTSPResponseMessage* pResp,
                                 UINT16 streamNumber);

    CHXString createSessionLinkCharHdr() const;
    CHXString createStreamLinkCharHdr(RTSPStreamInfo* pStreamInfo) const;
    CHXString createLinkCharHdrFromPrefs(const CHXString& baseKey,
                                         const CHXString& url) const;
    CHXString createLinkCharHdr(const CHXString& url,
                                UINT32* pGuaranteedBW,
                                UINT32* pMaxBW,
                                UINT32* pMaxTranferDelay) const;

    RTSPTransportBuffer* getTransportBuffer(UINT16 uStreamNumber);
    RTSPStreamInfo* getStreamInfo(UINT16 uStreamNumber);
    void SetBuffByteLimit(RTSPTransport* pTrans, RTSPStreamInfo* pStreamInfo);


    virtual HX_RESULT   sendRequest(RTSPRequestMessage* pMsg, UINT32 seqNo);
    virtual HX_RESULT   sendRequest(RTSPRequestMessage* pMsg,
                                    const char* pContent,
                                    const char* pMimeType, UINT32 seqNo);

    HXBOOL PipelineRTSP();
    HXBOOL ServerProducesWallClockTS(void);	
    HXBOOL m_bPipelineRTSP; //defaults to TRUE.

    HX_RESULT   ReportSuccessfulTransport(void);

    HX_RESULT   canSendRemainingSetupRequests(HX_RESULT status);

    UINT32 getSSRCFromTransportHeader(MIMEHeaderValue* pValue);

    void   AddPacketToPreSetupResponseQueue(UINT16 usPort, IHXBuffer* pBuffer);
    HXBOOL AnyPreSetupResponsePackets(UINT16 usPort);
    void   FlushPreSetupResponsePacketsToTransport(UINT16 usPort);
    void   ClearPreSetupResponseQueueMap();
    UINT32 GetBufferLimit(UINT16 usPort);
    UINT32 GetBufferSize(CHXSimpleList* pQueue);
    void   AddPortToStreamMapping(UINT16 usPort, UINT16 usStreamNumber);

    IHXSockAddr*                        m_pPeerAddr;        // UDP transport source=
    IHXSockAddr*                        m_pConnectAddr;   // UDP peer (redundant?)

    UINT16                              m_setupResponseCount;
    IHXInterruptState*                  m_pInterruptState;
    IHXRTSPClientProtocolResponse*      m_pResp;
    RTSPClientSessionManager*           m_pSessionManager;
    RTSPClientSession*                  m_pSession;
    RTSPClientProtocol::State           m_state;
    IHXScheduler*                       m_pScheduler;
    IHXValues*                          m_pSessionHeaders;
    IHXValues*                          m_pCloakValues;
    IHXKeyValueList*                    m_pResponseHeaders;
    IHXRegistry*                        m_pRegistry;
    IHXErrorMessages*                   m_pErrMsg;

    CHXString                           m_versionString;
    CHXString                           m_url;
    CHXString                           m_contentBase;
    CHXString                           m_headerControl;
    CHXString                           m_challenge;
    CHXString                           m_hostName;
    CHXString                           m_proxyName;
    CHXString                           m_sdpMulticastAddr;

    UINT32				m_ulServerVersion;
    UINT16                              m_proxyPort;
    UINT16                              m_hostPort;
    UINT16                              m_FWPortToBeClosed;

    UINT16*                             m_pCloakPorts;
    UINT8                               m_nCloakPorts;

    CHXString                           m_sessionID;
    IHXValues*                          m_pFileHeader;
    CHXSimpleList                       m_streamInfoList;
    CHXSimpleList                       m_transportRequestList; // transport supported sent in SETUP request
    CHXSimpleList                       m_activeTransportList;  // RTSPTransport* chosen after SETUP response
    CHXSimpleList                       m_UDPResponseHelperList;
    CHXSimpleList                       m_sessionList;
    CHXSimpleList			m_ResumeRequestPendingReplyList;
    CHXSimpleList			m_PlayRequestPendingReplyList;
    CHXMapLongToObj*                    m_pTransportStreamMap;  // map streamID->trans
    CHXMapLongToObj*                    m_pTransportPortMap;    // map port->trans
    CHXMapLongToObj*                    m_pTransportMPortMap;   // map multicast port->trans
    CHXMapLongToObj*                    m_pTransportChannelMap; // map channel->trans (only in TCP)
    CHXMapLongToObj*                    m_pUDPSocketStreamMap;  // map streamID->socket
    CHXMapLongToObj*                    m_pRTCPSocketStreamMap; // map streamID->socket
    CHXMapStringToOb*                   m_pControlToStreamNoMap;// streamID->streamNumber
    CHXMapLongToObj*                    m_pPreSetupResponsePacketQueueMap;
    CHXMapLongToObj*                    m_pUDPPortToStreamNumMap;
    HXBOOL                                m_bSeqValueReceived;
    HXBOOL                                m_bSetupRecord;
    HXBOOL                                m_bClientDone;
    HXBOOL                                m_bUseProxy;
    HXBOOL                                m_bUseHTTPProxy;
    HXBOOL                                m_bHTTPCloak;
    HXBOOL                                m_bNoReuseConnection;
    HXBOOL                                m_bLoadTest;
    HXBOOL                                m_bPrefetch;
    HXBOOL                                m_bFastStart;
    HXBOOL                                m_bPaused;
    HXBOOL                                m_bSessionSucceeded;
    HXBOOL                                m_bMulticastStats;
    HXBOOL                              m_bHandleWMServers;
    PipelinedDescribeLogic*             m_pPipelinedDescLogic;
    IHXConnectionlessControl*           m_pConnectionlessControl;
    HXBOOL                              m_bConnectionlessControl;
    TimeoutCallback*                    m_pConnectionCheckCallback;
    UINT32                              m_uConnectionCheckCallbackHandle;
    HXBOOL                              m_bConnectionAlive;
    UINT32                              m_uConnectionTimeout;
    HXBOOL                              m_bEntityRequired;
    UINT16                              m_cloakPort;
    IHXMutex*                           m_pMutex;
    UINT16                              m_uProtocolType;
    TransportMode                       m_currentTransport;

    UINT32                              m_ulBufferDepth;
    HXBOOL                              m_bSplitterConsumer;
    RawPacketFilter*                    m_pPacketFilter;

    HXBOOL                              m_bHasSyncMasterStream;

    IHXFirewallControlManager*          m_pFWCtlMgr;
    IHXNetServices*                     m_pNetSvc;
    IHXResolve*                         m_pResolver;
    IHXPreferences*                     m_pPreferences;
    IHXBuffer*                          m_pUAProfURI;
    IHXBuffer*                          m_pUAProfDiff;

    /* Interop */

    // TRUE iff a server we are talking is not RS...This implies RTP..
    HXBOOL                              m_bNonRSRTP;
    IHXValues*                          m_pSetupRequestHeader;
    HXBOOL                              m_bPlayJustSent;
    HXBOOL                              m_bIPTV;
    HXBOOL                              m_bColumbia;
    HXBOOL                              m_bNoKeepAlive;
    HXBOOL                              m_bForceUCaseTransportMimeType;
    HXBOOL                              m_bReportedSuccessfulTransport;

    HXBOOL                              m_bSDPInitiated;
    HXBOOL				m_bInitMsgSent;
    HXBOOL                              m_bMulticast;
    HXBOOL                              m_bIsLive;
    HXBOOL                              m_bInitDone;
    IHXValues*                          m_pSDPFileHeader;
    IHXValues*                          m_pSDPRequestHeader;
    CHXSimpleList*                      m_pSDPStreamHeaders;

    HXBOOL                                m_pIsMethodSupported[RTSP_TABLE_SIZE];
    static const RTSPTableEntry         zm_pRTSPTable[RTSP_TABLE_SIZE];


    CHXKeepAlive*                       m_pSessionTimeout;
    TimeoutCallback*                    m_pKeepAliveCallback;
    HXBOOL                              m_bUseLegacyTimeOutMsg;
    UINT32                              m_ulServerTimeOut;
    UINT32                              m_ulCurrentTimeOut;
#ifdef HELIX_FEATURE_FORCE_KEEPALIVE_DURING_PAUSE
    UINT32				m_ulForceKeepAliveDuringPauseTimout;
    HXBOOL				m_bKeepAliveTimeouToBeReset;
#endif
    CHXRateAdaptationInfo*              m_pRateAdaptInfo;
    HXNetSourceBufStats*                m_pSrcBufStats;
    UINT32                              m_ulRegistryID;
    UINT32			        m_ulLastBWSent;	
    HXBOOL                              m_bHaveSentRemainingSetupRequests;
    HXBOOL                              m_bSDBDisabled;

#if defined(_MACINTOSH)
    RTSPClientProtocolCallback*         m_pCallback;
#endif /* _MACINTOSH */
};

REGISTER_MANAGED_COMPONENT(RTSPClientProtocol);

#endif /* _RTSPCLNT_H_ */
