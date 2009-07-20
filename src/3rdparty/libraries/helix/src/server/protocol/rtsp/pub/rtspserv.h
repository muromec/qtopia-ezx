/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspserv.h,v 1.78 2007/02/14 18:50:45 tknox Exp $
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

#ifndef _RTSPSERV_H_
#define _RTSPSERV_H_

#include "rtspif.h"
#include "rtspbase.h"
#include "rtsptran.h"
#include "transport.h"
#include "rtspmdsc.h"
#include "rtspmsg.h"
#include "sdptypes.h"        // SdpFileType

#include "basepkt.h"
#include "hxauthn.h"
#include "hxathsp.h"
#include "hxfilsp.h"
#include "altserv.h"

#include "hxrtsp2.h"
#include "hxcomm.h"    // IHXFastAlloc
#include "hxformt.h"   // IHXFileFormatHeaderAdviseResponse

#include "servprotdef.h" // ClientType

#include "hxnet.h"
#include "hxrtspservprot2.h" // IHXRTSPServerProtocol2

class CHXRTCPTransMapSocket;

class RTSPServerProtocol;
class RTSPServerSessionManager;
class RTSPOptionsMessage;
class RTSPDescribeMessage;
class RTSPAnnounceMessage;
class RTSPSetupMessage;
class RTSPPlayMessage;
class RTSPPauseMessage;
class RTSPTeardownMessage;
class RTSPGetParamMessage;
class RTSPSetParamMessage;
class RTSPRecordMessage;
_INTERFACE IHXSDPAggregateStats;
_INTERFACE IHXValues;
_INTERFACE IHXStreamDescription;
_INTERFACE IHXUDPSocket;
_INTERFACE IHXChallengeResponse;
_INTERFACE IHXErrorMessages;
_INTERFACE IHXClientStatsManager;
_INTERFACE IHXClientStats;
_INTERFACE IHXList2;
_INTERFACE IHXRTSPServerProtocolResponse2;
class RTSPParser;
class CByteGrowingQueue;
class RTSPResendBuffer;
class RTSPServerProtocol;
struct RealChallenge;
struct MidBoxChallenge;
class RTSPStats;
class RTSPSessionEventsList;

class RTSPTransportInstantiator;
class RTSPTransportParams;
#define NOTFIY_QUEUE_SZ 256

struct RTSPSessionItem
{
    CHXString m_sessionID;
    CHXString m_URL;
    UINT32 m_seqNo;
    BOOL m_bSetup;
    RTSPServerProtocol* m_pProt;
};

class RTSPServerProtocol
    : public RTSPBaseProtocol /* XXXTDM for various member vars */
    , public IHXRTSPServerProtocol2
    , public IHXTCPResponse
    , public IHXSocketResponse
    , public IHXRTSPServerTransportResponse
    , public IHXChallenge
    , public IHXTimeStampSync
    , public IHXFileFormatHeaderAdviseResponse
    , public IHXAccurateClock
{
public:
#ifdef PAULM_RTSPPROTTIMING
    UINT32 GetSocket(){return m_pSocket;}
#endif
    RTSPServerProtocol(BOOL bDisableResend = FALSE,
                        BOOL bSendLostPackets = FALSE);
    ~RTSPServerProtocol();

    class KillRTSPServerProtocolCallback : public IHXCallback
    {
    public:
        KillRTSPServerProtocolCallback(RTSPServerProtocol* pProt);
        ~KillRTSPServerProtocolCallback();
        /*
         * IUnknown methods
         */

        STDMETHOD(QueryInterface)           (THIS_
                                            REFIID riid,
                                            void** ppvObj);

        STDMETHOD_(UINT32,AddRef)          (THIS);

        STDMETHOD_(UINT32,Release)         (THIS);


        /*
         * IHXCallback methods.
         */
        STDMETHOD(Func) (THIS);

        RTSPServerProtocol* m_pProtocol;
    private:
        ULONG32  m_ulRefCount;
    };

    class KeepAliveCallback : public IHXCallback
    {
    public:
        KeepAliveCallback(RTSPServerProtocol* pOwner);
        ~KeepAliveCallback();

    /* IUnknown Interfaces */

        STDMETHOD(QueryInterface)        (THIS_
                                          REFIID riid,
                                          void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)         (THIS);

        STDMETHOD_(ULONG32,Release)        (THIS);

        STDMETHOD(Func)                    (THIS);

    private:
        ULONG32                        m_ulRefCount;
        RTSPServerProtocol*            m_pOwner;
    };

    class CPropWatchResponse : public IHXPropWatchResponse
    {
    public:
        static CPropWatchResponse* Instance(IUnknown* pIContext);        //        This is a singleton class

        // *** IUnknown Methods ***
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)(THIS);
        STDMETHOD_(ULONG32,Release)(THIS);

        // *** IHXPropWatchResponse methods ***
        STDMETHOD(AddedProp)(THIS_ const UINT32 ulHash, const HXPropType type, const UINT32 ulParentHash);
        STDMETHOD(ModifiedProp)(THIS_ const UINT32 ulHash, const HXPropType type, const UINT32 ulParentHash);
        STDMETHOD(DeletedProp)(THIS_ const UINT32 ulHash, const UINT32 ulParentHash);

        UINT32 GetStatsInterval(void) { return m_ulStatsInterval; }

    private:
        // *** private methods ***
        CPropWatchResponse(IUnknown* pIContext);
        ~CPropWatchResponse();        // !!! warning, not virtual, so don't override this class
                                // and then try to delete from this base

        //        Copy and Assignment are NOT valid operations for this class
        CPropWatchResponse(const CPropWatchResponse& PropWatchResponse);
        CPropWatchResponse& operator=(const CPropWatchResponse& PropWatchRespnse);

        // *** private, static data members ***
        static CPropWatchResponse** zm_ppInstance;        //        For Singleton
        static IHXThreadLocal*     zm_pThreadLocal;
        static INT32                zm_nStartupSemaphore;

        // *** private data members ***
        INT32 m_lRefCount;

        IUnknown* m_pIContext;

        IHXRegistry* m_pIRegistry;
        IHXPropWatch* m_pIPropWatch;
        UINT32 m_uidStatsIntervalKey;

        UINT32 m_uidConfigRootKey;        // Handles when StatsInterval key is deleted, or doesn't
                                        // exist on start up, but is added later

        UINT32 m_ulStatsInterval;
    };

    CPropWatchResponse* m_pPropWatchResponse;        //  Each class can have their own instance

    BOOL                            m_bIWillDieSoon;
    HX_RESULT                            TrueDone();

    enum State
    {
        INIT,
        READY,
        PLAYING,
        RECORDING,
        KILLED,     // Session killed (eg. by alert)
        NONE        // Special state value meaning "no change"
    };

    enum ABDState
    {
        ABD_READY,      // When Session is initialized
        ABD_GET_PARAM_RECEIVED, // We restrict only one GET_PARAM request per session  
        ABD_DENY
    };

    /*
     * IUnknown methods
     */

    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(UINT32,AddRef)          (THIS);

    STDMETHOD_(UINT32,Release)         (THIS);

    /*
     * IHXRTSPServerProtocol methods
     */

    STDMETHOD(Init)                        (THIS_
                                            IUnknown* pContext);
    STDMETHOD(SetBuildVersion)                (THIS_
                                            const char* pVersionString);
    STDMETHOD(SetOptionsRespHeaders)        (THIS_
                                            IHXValues* pHeaders);
    STDMETHOD(Done)                        (THIS);
    STDMETHOD(SetControl)                (THIS_
                                        IHXSocket* pSocket,
                                        IHXRTSPServerProtocolResponse2* pResp,
                                        IHXBuffer* pBuffer);
    STDMETHOD(AddSession)                (THIS_
                                            const char* pSessionID,
                                        const char* pURL,
                                        UINT32 ulSeqNo);
    STDMETHOD(SetResponse)                (THIS_
                                        IHXSocket* pSocket,
                                            IHXRTSPServerProtocolResponse2* pResp);
    STDMETHOD(Disconnect)                      (THIS_
                                        const char* pSessionID);
    STDMETHOD(SendAlertRequest)                (THIS_
                                        const char* pSessionID,
                                        INT32 lAlertNumber,
                                        const char* pAlertText);
    STDMETHOD(SendKeepAlive)                (THIS);
    STDMETHOD(SendTeardownRequest)        (THIS_
                                            const char* pSessionID);
    STDMETHOD(SendRedirectRequest)        (THIS_
                                            const char* pSessionID,
                                        const char* pURL,
                                        UINT32 mSecsFromNow);
    STDMETHOD(SendProxyRedirectRequest)        (THIS_
                                            const char* pSessionID,
                                        const char* pURL);
    STDMETHOD(SetupSequenceNumberResponse) (THIS_
                                        const char* pSessionID);

    STDMETHOD(SendStreamResponse)         (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        IHXValues* pFileHeader,
                                        CHXSimpleList* pHeaders,
                                        IHXValues* pOptionalValues,
                                        IHXValues* pResponseHeaders,
                                        BOOL bMulticastOK,
                                        BOOL bRequireMulticast,
                                        BOOL bIsRealDataType);

    STDMETHOD(SendStreamRecordDescriptionResponse) (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        IHXValues* pAuthValues,
                                        IHXValues* pResponseHeaders);

    STDMETHOD(SendPacket)                (THIS_
                                        BasePacket* pPacket,
                                        const char* pSessionID);
    STDMETHOD(StartPackets)                (THIS_
                                        UINT16 uStreamNumber,
                                        const char* pSessionID);
    STDMETHOD(StopPackets)                (THIS_
                                        UINT16 uStreamNumber,
                                        const char* pSessionID);
    STDMETHOD(SetPacketResend)                (THIS_
                                        IHXPacketResend* pPacketResend,
                                        const char* pSessionID);
    STDMETHOD(SendSetParameterRequest)        (THIS_
                                        const char* pSessionID,
                                        const char* pURL,
                                        const char* pParamName,
                                        IHXBuffer* pParamValue);
    STDMETHOD(SendSetParameterRequest)  (THIS_
                                            const char* pSessionID,
                                        const char* pURL,
                                        const char* pParamName,
                                        const char* pParamValue,
                                        const char* pMimeType,
                                        const char* pContent);
    STDMETHOD(SendGetParameterRequest)        (THIS_
                                            UINT32 lParamType,
                                        const char* pParamName);

    /* player redirect/reconnect */
    HX_RESULT SendSetParameterRequest(const char* pSessionID,
                                      const char* pURL,
                                      IHXValues* pNameVal);


    STDMETHOD(SendRTTResponse)                (THIS_
                                            UINT32 secs,
                                        UINT32 uSecs,
                                        const char* pSessionID);

    STDMETHOD(SendCongestionInfo)        (THIS_
                                        INT32 xmitMultiplier,
                                        INT32 recvMultiplier,
                                            const char* pSessionID);

    STDMETHOD(SendStreamDone)                (THIS_
                                            UINT16 streamID,
                                        const char* pSessionID);

    STDMETHOD(SetConnectionTimeout)        (THIS_
                                        UINT32 uSeconds
                                        );

    STDMETHOD(SetFFHeaderAdvise)        (THIS_
                                        IHXFileFormatHeaderAdvise* pAdvise,
                                        const char* pSessionID
                                        );
    /*
     *  IHXTCPResponse methods
     */

    STDMETHOD(ConnectDone)              (THIS_
                                        HX_RESULT status);

    STDMETHOD(ReadDone)                 (THIS_
                                        HX_RESULT status,
                                        IHXBuffer* pBuffer);

    STDMETHOD(WriteReady)               (THIS_
                                        HX_RESULT status);

    STDMETHOD(Closed)                   (THIS_
                                        HX_RESULT status);

    // IHXSocketResponse
    STDMETHOD(EventPending)             (THIS_ UINT32 uEvent, HX_RESULT status);

    /*
     * RTSPServerTransportResponse methods
     */

    STDMETHOD(PacketReady)              (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        IHXPacket* pPacket);

    STDMETHOD(OnRTTRequest)             (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID
                                        );

    STDMETHOD(OnRTTResponse)                (THIS_
                                            HX_RESULT status,
                                        const char* pSessionID,
                                        UINT32 ulSecs,
                                        UINT32 ulUSecs);

    STDMETHOD(OnBWReport)                     (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        INT32 aveBandwidth,
                                        INT32 packetLoss,
                                        INT32 bandwidthWanted
                                        );

    STDMETHOD(OnCongestion)                (THIS_
                                            HX_RESULT status,
                                        const char* pSessionID,
                                        INT32 xmitMultiplier,
                                        INT32 recvMultiplier);

    STDMETHOD(OnStreamDone)                (THIS_
                                        HX_RESULT status,
                                        UINT16 uStreamNumber);

    STDMETHOD(OnSourceDone)                (THIS);

    STDMETHOD(OnACK)                        (THIS_
                                        HX_RESULT status,
                                        RTSPResendBuffer* pResendBuffer,
                                        UINT16 uStreamNumber,
                                        const char* pSessionID,
                                        UINT16* pAckList,
                                        UINT32 uAckListCount,
                                        UINT16* pNakList,
                                        UINT32 uNakListCount);

    STDMETHOD(OnProtocolError)                (THIS_
                                               HX_RESULT status);

    //
    // IHXChallenge
    //
    STDMETHOD(SendChallenge)                (THIS_
        IHXChallengeResponse* pIHXChallengeResponseSender,
        IHXRequest* pIHXRequestChallenge);

    /*
     * IHXFileFormatHeaderAdvise methods
     */
    STDMETHOD(OnHeadersDone)        (THIS_
                                 HX_RESULT status,
                                 UINT32 ulErrNo);

    /*
     * IHXAccurateClock methods
     */
    STDMETHOD_(HXTimeval,GetTimeOfDay)      (THIS);


    /*
     * this method is called by the RMServer immediately after
     * creating an instance of this class. it allows the server
     * to pass on data which has already been read in, but needs
     * to be parsed.
     */
    void SetAlreadyReadData                (Byte* pAlreadyReadData,
                                        UINT32 alreadReadDataLen);

    UINT32 ControlBytesSent(void) { return m_uControlBytesSent; }

    void PlayDone(const char* pSessionID);

    void SessionDone                        (const char* sessionID);

    inline UINT32 GetThreadSafeFlags() { return m_ulThreadSafeFlags; };

private:
    class Session
        : public IHXUDPResponse
        , public IHXChallenge
        , public IHXThreadSafeMethods
        , public IHXAlternateServerProxyResponse

    {
    public:
        Session(const char* pSessionID, RTSPServerProtocol* pServProt);
        ~Session(void);

        HX_RESULT Init(IUnknown* pContext);
        HX_RESULT Done(void);
        HX_RESULT handleChallengeResponse(IHXRTSPMessage* pMsg);

        // IUnknown methods
        STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(UINT32,AddRef)       (THIS);
        STDMETHOD_(UINT32,Release)      (THIS);

        // IHXUDPResponse methods
        STDMETHOD(ReadDone)             (THIS_ HX_RESULT status,
                                            IHXBuffer* pBuffer,
                                            UINT32 ulAddr,
                                            UINT16 nPort);

        // IHXChallenge
        STDMETHOD(SendChallenge)        (THIS_ IHXChallengeResponse* pSender,
                                            IHXRequest* pRequest);

        // IHXThreadSafeMethods methods
        STDMETHOD_(UINT32,IsThreadSafe) (THIS);

        // IHXAlternateServerProxyResponse
        STDMETHOD(OnModifiedEntry)      (THIS_ HX_ALTERNATES_MOD_FLAG type);


        // Session methods
        Transport* getTransport     (UINT16 streamNumber);
        Transport* getFirstTransportSetup();

        void mapTransportStream         (Transport* pTransport,
                                            UINT16 streamNumber);
        void mapTransportChannel        (Transport* pTransport,
                                            UINT16 channel);
       
        void mapFportTransResponse        (CHXRTCPTransMapSocket* pTransResponse,
                                            UINT16 port);

        void clearDescribeMimeTypeList  (void);
        void clearStreamInfoList        (void);
        HX_RESULT handleTCPData         (BYTE* pData, UINT16 dataLen, UINT16 channel);
        HX_RESULT handleTCPData         (IHXBuffer* pBuffer, UINT16 channel);
        BOOL RegisterWithSharedUDPPort(IHXSocket* pUDPSocket);
	
	//ABD - Auto Bandwidth Detection 
        HX_RESULT sendBWProbingPackets  (UINT32 ulCount, UINT32 ulSize, REF(INT32) lSeqNo);
	ABDState                        m_ABDState;

        /*
         * This method extracts a session GUID from query parameters in an RTSP
         * request URL and sets the session id in the stream description (SDP).
         */
        HX_RESULT SetSDPSessionGUID(IHXStreamDescription* pSD,
                                    IHXValues** pHeaderValues);

        void SetSessionActiveStamp(time_t tTimeStamp = 0);

        // These functions add the Session header to the passed msg.
        // The session timeout is added as a param if it hasn't been sent yet.
        HX_RESULT AddSessionHeader(RTSPResponseMessage* pMsg);
        HX_RESULT AddSessionHeader(IHXRTSPMessage* pMsg);

        RTSPStreamInfo* GetStreamFromControl(UINT32 ulControl);

        IUnknown*                       m_pContext;
        IHXCommonClassFactory*          m_pCommonClassFactory;
        IHXScheduler*                   m_pScheduler;

        BOOL                            m_bUnused;
        RTSPStreamInfo**                m_ppStreamInfo;
        char*                           m_pStreamUrl;
        CHXString                       m_streamSequenceNumbers;
        CHXString                       m_describeURL;
        UINT16                          m_uStreamCount;
        UINT16                          m_uStreamGroupCount;
        CHXString                       m_sessionID;
        CHXSimpleList                   m_describeMimeTypeList;
        CHXString                       m_describeMimeType;
        CHXSimpleList*                  m_pUDPSocketList;
        BOOL                            m_bIsTNG;
        UINT16                          m_sSetupCount;
        UINT16                          m_uTotalSetupReceived;
        BOOL                            m_bSetupsComplete;
        RTSPMethod                      m_RTSPMessageTagOriginating;
        UINT32                          m_ulControlID;
        BOOL                            m_bRetainEntityForSetup;
        BOOL                            m_bNeedSessionHeader;
        BOOL                            m_bUseOldSdp;
        BOOL                            m_bIsMidBox;
        BOOL                            m_bIsViaRealProxy;
        MidBoxChallenge*                m_pMidBoxChallenge;
        BOOL                            m_bChallengeMet;
        BOOL                            m_bSendClientRealChallenge3;
        State                           m_state;
        IHXSockAddr*                    m_pLocalAddr;
        CHXSimpleList*                  m_pTransportList;
        CHXMapLongToObj*                m_pTransportStreamMap;  // map stream->transport
        CHXMapLongToObj*                m_pTransportPortMap;    // map port->transport
        CHXMapLongToObj*                m_pTransportChannelMap; // map channel -> transport
        
        CHXMapLongToObj*                m_pFportTransResponseMap;  // map Foreinport->Transportresponse

        UINT32                          m_ulMaxUDPSize;
        ULONG32                         m_ulRefCount;
        IHXPacketResend*                m_pPacketResend;
        BOOL                                m_bMulticastOK;
        BOOL                                m_bRequireMulticast;
        BOOL                                m_bIsRealDataType;
        UINT32                                m_ulPacketCount;
        RTSPServerProtocol*                m_pServProt;
        IHXFileFormatHeaderAdvise*     m_pFFAdvise;
        RTSPSessionEventsList*               m_pEventList;
        BOOL                            m_bUseControlID;
        UINT16                          m_uFirstSetupStreamNum;

        /* used by BCNG tran only */
        IHXRTSPRequestMessage*         m_pReqMsg;

        UINT32                          m_ulRDTFeatureLevel;

        BOOL                            m_bSupportsRARTPChallenge;
        BOOL                            m_bSessionSetup;
        UINT32                          m_ulThreadSafeFlags;

        /* player redirect/reconnect */
        IHXAlternateServerProxy*        m_pAltMgr;
        BOOL                            m_bAltEnabled;
        IHXBuffer*                      m_pServAlt;
        IHXBuffer*                      m_pProxAlt;

        /*
         *  RTP only (for now)
         *  Allows a client to SETUP only a subset of streams in SDP file.
         *  i.e.  SETUP only 1 stream in 2 streams presentation.
         */
        BOOL*                           m_pbSETUPRcvdStrm;
        UINT16*                         m_punNumASMRules;
        BOOL                            m_bHandleRules;

        UINT32                          m_ulInitialHXTS;
        BOOL                            m_bNeedInitialHXTS;
        void                            SetInitialHXTS(UINT32 ulInitialTS);
        BOOL                            NeedInitialHXTS() { return m_bNeedInitialHXTS; }
        UINT32                          GetInitialHXTS() { return m_ulInitialHXTS; }
        void                            ClearInitialHXTS();

        BOOL                            m_bBlockTransfer;
        UINT32                          m_ulSessionRegistryNumber;
        UINT32                          m_ulSessionStatsObjId;


        BOOL                            m_bAddStatsInterval;
        UINT32                          m_ulStatsInterval;
        UINT32                          m_ulSessionDuration;
        BOOL                            m_bIsLive;

        // Member vars for delayed play responses
        BOOL                        m_bPlayResponseDone;
        BOOL                        m_bScaleResponsePending;
        BOOL                        m_bRangeResponsePending;
        BOOL                        m_bRTPInfoResponsePending;
        BOOL                        m_bNeedRTPSequenceNo;
        RTSPRange::RangeType        m_ulPlayRangeUnits;
        UINT32                      m_ulPlayReqCSeq;
        UINT32                      m_ulPlayRangeStart;
        UINT32                      m_ulPlayRangeEnd;
        FIXED32                     m_fPlayScale;
        FIXED32                     m_fPlaySpeed;
        UINT32                      m_ulStreamsEnded;

        UINT16                      m_unStreamsRTPInfoReady;

        /* XXXMC
         * Support for PV Emulation.
         */
        BOOL                        m_bEmulatePVSession;

        UINT32                      m_ulInitiationID;

        friend class RTSPServerProtocol;
        friend class RTSPTransportInstantiator;
     
        BOOL                        m_bNeedAggregateTransportHeader;
        RTSPTransportInstantiator*  m_pTransportInstantiator;
        RTSPTransportParams*        m_pAggregateTransportParams;

    private:
        HX_RESULT MakeSessionHeaderValue(MIMEHeaderValue** ppVal);

        time_t                      m_tSessionActiveStamp;
        BOOL                        m_bSessionKeepAliveExpired;
    };
    friend class Session;
    friend class RTSPProtocol;
    friend class RTSPTransportInstantiator;

    HX_RESULT filterRFC822Headers        (IHXValues* pOldHeaders,
                                         REF(IHXValues*) pNewHeaders);
    void AddRFC822Headers(IHXRTSPMessage* pMsg, IHXValues* pRFC822Headers);

    BOOL AddUint32MIMEHeader(const char* header, UINT32 value);
    void Add3GPVideoHeaders(RTSPServerProtocol::Session* pSession);

    HX_RESULT GetSourceAddr(REF(IHXBuffer*) pAddrBuf);

    HX_RESULT addEntityHeaders(RTSPServerProtocol::Session* pSession,
                          const char* mimeType,
                          IHXBuffer* pDescription,
                          IHXValues* pResponseHeaders);

    HX_RESULT sendInitialMessage        ();
    IHXStreamDescription* getStreamDescriptionInstance
                                        (const char* pMimeType);
    HX_RESULT AddStatsHeaders();

    Session* getSession                        (const char* pSessionID, BOOL bCreate = FALSE);
    void putPropsInRegistry                ();
    HX_RESULT handleTCPData                (const char* pSessionID,
                                         BYTE* pData, UINT16 dataLen,
                                         UINT16 channel);
    UINT16 GetSessionOriginatingStreamNumber(const char* pSessionID);
    SdpFileType GetSdpFileTypeWeNeed        (IHXValues* pHeaders);

    HX_RESULT _FinishPlaynow( RTSPServerProtocol::Session* pSession);
    HX_RESULT _FinishSetup(UINT16 usStreamNumber, const char* pcharSessionID);
    HX_RESULT _FinishCommon(RTSPServerProtocol::Session* pSession,
                            RTSPStreamInfo* pStreamInfo,
                            UINT16 usStream,
                            UINT16 usType);
    HX_RESULT FinishAllSetups(RTSPServerProtocol::Session* pSession);

    HX_RESULT GetSalt(RTSPSetupMessage* pMsg,
                      IHXValues* pRequestHeaders,
                      UINT32 ulSessionRegistryNumber,
                      UINT32 ulSessionStatsObjId);
    HX_RESULT GetSalt(IHXRTSPRequestMessage* pReqMsg,
                      IHXValues* pRequestHeaders,
                      UINT32 ulSessionRegistryNumber,
                      UINT32 ulSessionStatsObjId);

    void _FinishDescribe(const char* pcharSessionID, BOOL bUseOldSdp,
                         IHXValues* pFileHeader, CHXSimpleList* pHeaders,
                         IHXValues* pOptionalValues, CHXString& mimeType,
                         IHXBuffer** ppIHXBufferDescription);

    STDMETHOD(_SendStreamSetupResponse)(HX_RESULT status,
                                        const char* pSessionID,
                                        IHXValues* pFileHeader,
                                        CHXSimpleList* pHeaders,
                                        IHXValues* pOptionalValues,
                                        IHXValues* pResponseHeaders);

    STDMETHOD(_SendStreamDescriptionResponse)(HX_RESULT status,
                                              const char* pSessionID,
                                              IHXValues* pFileHeader,
                                              CHXSimpleList* pHeaders,
                                              IHXValues* pOptionalValues,
                                              IHXValues* pResponseHeaders);

    STDMETHOD(_PlaynowSecondStage)(HX_RESULT status,
                                        const char* pSessionID,
                                        IHXValues* pFileHeader,
                                        CHXSimpleList* pHeaders,
                                        IHXValues* pOptionalValues,
                                        IHXValues* pResponseHeaders);


    HX_RESULT HandleStreamAdaptationHeader (REF(const CHXString) sessionID,
                                            IHXRTSPMessage* pRTSPMsg,
                                            BOOL bProcessingSetup);

    HX_RESULT Handle3GPPLinkCharHeader (REF(const CHXString) sessionID,
                                            IHXRTSPMessage* pRTSPMsg,
                                            BOOL bProcessingSetup);

    HX_RESULT HandleBandwidthHeader(const char* pSessionID, IHXRTSPMessage* pRTSPMsg);

    BOOL mimetypeSupportsRARTPChallenge(const char* pMimeType);

    void SetStatus(UINT32 ulSessionRegistryNumber, UINT32 ulSessionStatsObjId, const INT32 nStatus);

    void SetClientStatsObj(IHXClientStats* pClientStats);

    void SetIsProxy(BOOL bIsProxy);

    HX_RESULT SetStreamStartTime(const char* pszSessionID,
                                 UINT32 ulStreamNum, UINT32 ulTimestamp);

    UINT32 GetClientStatsObjId() { return m_ulClientStatsObjId; }
    BOOL IsClientStatsObjIdSet() { return m_bIsClientStatsObjIdSet; }

    BOOL IsViaRealProxy(IHXRTSPMessage* pMsg);

    BOOL IsTCPPrefLicensed();

    HX_RESULT AddRDTFeatureLevelHeader(UINT32 ulRDTFeatureLevel);
    HX_RESULT HandleAltServerProxy(RTSPServerProtocol::Session* pSession);

public:
    RTSPMethod GetSessionOriginatingMessage(const char* pSessionID);

    /*
     *        only used by RTP
     */
    //XXXTDM: how did these get here?  this should be a syntax error!
    STDMETHOD(SetInitialTS(const char* pSessionID, UINT32 ulInitialTS));
    STDMETHOD_(BOOL, NeedInitialTS(const char* pSessionID));
    STDMETHOD_(UINT32, GetInitialTS(const char* pSessionID));
    STDMETHOD(ClearInitialTS(const char* pSessionID));


private:
    ULONG32                             m_ulRefCount;
    IHXRTSPServerProtocolResponse2*     m_pResp;
    IHXScheduler*                       m_pScheduler;
    IHXThreadSafeScheduler*             m_pIScheduler;
    IHXAccurateClock*                   m_pAccurateClock;
    IHXRegistry*                        m_pRegistry;
    RTSPServerSessionManager*           m_pSessionManager;
    CHXMapStringToOb*                   m_pSessions;
    CHXMapStringToOb*                   m_pSessionStatsObjIDs;
    UINT32                              m_nUnusedSessions;
    CHXMapLongToObj*                    m_pSessionIDList;
    CHXString                           m_challenge;
    CHXString                           m_pVersionString;
    IHXValues*                          m_pSessionHeaders;
    UINT32                              m_ulLastSeqNo;
    BOOL                                m_bResendRequired;
    BOOL                                m_bSetupRecord;
    INT32                               m_iStatsMask;
    INT32                               m_iDebugLevel;
    FILE*                               m_pDebugFile;
    INT32                               m_iTimingLevel;
    FILE*                               m_pTimingFile;
    BOOL                                m_bAuthenticationFailed;
    INT8                                m_tcpInterleave;
    RealChallenge*                      m_pRealChallenge;
    BOOL                                m_bChallengeDone;
    BOOL                                m_bChallengeMet;
    UINT32                              m_ulChallengeInitMethod;
    BOOL                                m_bRARTPChallengeMet;
    BOOL                                m_bRARTPChallengePending;
    BOOL                                m_bSendClientRealChallenge3;
    VOLATILE BOOL                       m_bIsValidChallengeT;
    IHXValues*                          m_pOptionsResponseHeaders;
    IHXErrorMessages*                   m_pErrorMessages;

    IHXClientStatsManager*              m_pClientStatsMgr;
    UINT32                              m_ulClientStatsObjId;
    BOOL                                m_bIsClientStatsObjIdSet;

    BOOL                                m_bUseRegistryForStats;
    BOOL                                m_bDisableResend;
    BOOL                                m_bProxyResponseSent;
    BOOL                                m_bIsLocalBoundSocket;
    UINT32                              m_ulThreadSafeFlags;
    IHXFastAlloc*                       m_pFastAlloc;
    UINT16                              m_usStreamNumberTmp;
    char*                               m_pSessionIDTmp;
    char*                               m_pRTCPInterval;
    time_t                              m_tActiveStamp;
    time_t                              m_tKeepAliveInterval;
    UINT32                              m_ulKeepAliveCallbackID;
    CHXMapLongToObj*                    m_pKeepAlivePendingMessages;
    BOOL                                m_bRTSPPingResponsePending;
    BOOL                                m_bIsStreamAttemptCounted;

    CHXMapPtrToPtr*                     m_pWriteNotifyMap;
    IUnknown*                           m_pWriteNotifyLastKey;

    ClientType                          m_clientType;
    SdpFileType                         m_sdpFileType;
    BOOL                                m_bSendBWPackets;
    BOOL                                m_bOriginUsesTrackID;

    BOOL                                m_bPN3Enabled;

    DECLARE_SMART_POINTER(IHXChallengeResponse) m_spChallengeResponseSender;
    DECLARE_SMART_POINTER(IHXRequest) m_spRequestChallenge;

    enum Field
    {
        NUM_RESEND_REQS,
        NUM_BYTES_RESENT,
        MAX_FIELDS
    };

    UINT32                      reg_id[MAX_FIELDS];
    IHXSockAddr*                m_pProxyPeerAddr;
    IHXSockAddr*                m_pProxyLocalAddr;
    BOOL                        m_bSendLostPackets;

    UINT32                      m_ulRegistryConnId;

    // XXX: aak -- for the new rtsp parser
    IHXRTSPConsumer*            m_pConsumer;
    IHXBuffer*                  m_pBufFrag;
    IHXRTSPProtocolResponse*    m_pResponse2;

public:
    STDMETHOD(OnOptionsRequest)        (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnAnnounceRequest)(THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnDescribeRequest)(THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnSetupRequest)        (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnRecordRequest)  (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnGetParameterRequest)(THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnSetParameterRequest)(THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnPlayRequest)        (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnPauseRequest)        (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnTeardownRequest)(THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnPlaynowRequest)(THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnRedirectRequest)(THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnExtensionRequest)(THIS_ IHXRTSPRequestMessage* pmsg);

    HX_RESULT handleInput        (IHXBuffer* pBuf);

    HX_RESULT handleStateError      (RTSPServerProtocol::State state);
    HX_RESULT handleBadVersion      (void);

    HX_RESULT handleCommonRequestHeaders(IHXRTSPRequestMessage* pReqMsg);
    HX_RESULT handleRequireHeaders(IHXMIMEHeader* pMIMEHeaderRequire,
                       IHXBuffer* pVerb, char* szUnSupported);
    HX_RESULT handleAcceptEncodingHeader(IHXMIMEHeader* pAcceptEncodingHeader,
                       IHXBuffer* pVerb);
    HX_RESULT handleChallengeResponse        (IHXRTSPMessage* pMsg);

    HX_RESULT ValidateChallengeResp(RTSPServerProtocol::Session* pSession,
                                    CHXString& sessionID,
                                    BOOL bMidBoxChallengeHdr,
                                    IHXRTSPMessage* pIRMAReqMsg);

    HX_RESULT SetChallengeHeader(RTSPServerProtocol::Session* pSession,
                                 UINT32 ulLastReqMethod,
                                 BOOL bMidBoxChallengeHdr,
                                 IHXRTSPMessage* pRespMsg);

    HX_RESULT SetChallengeHeader(RTSPServerProtocol::Session* pSession,
                                 UINT32 ulLastReqMethod,
                                 BOOL bMidBoxChallengeHdr,
                                 RTSPResponseMessage* pRespMsg);

    HX_RESULT PrepareChallengeHeader(RTSPServerProtocol::Session* pSession,
                                     UINT32 ulLastReqMethod,
                                     char*& pszChallenge1,
                                     char*& pszChallenge3,
                                     BOOL bMidBoxChallengeHdr,
                                     UINT32 ulMaxBuffSize);

    /*
     * RTSP Event logging and timing handlers (virtual methods in RTSPBase
     * overriden by RTSPServerProtocol).
     */
    void handleDebug(IHXBuffer* pMsgBuf, BOOL bInbound);
    void handleTiming(IHXBuffer* pMsgBuf, BOOL bInbound);

    HX_RESULT OnClientRequestEvent(IHXRTSPRequestMessage* pReq,
                                   IHXRTSPMessage* pMsg,
                                   RTSPServerProtocol::Session* pSession);

    HX_RESULT OnServerResponseEvent(UINT32 ulStatusCode);

    HX_RESULT OnServerRequestEvent(RTSPRequestMessage* pMsg,
                                   const char* pSessionID,
                                   RTSPServerProtocol::Session* pSession);

    HX_RESULT OnClientResponseEvent(IHXRTSPResponseMessage* pResp,
                                    IHXRTSPMessage* pMsg,
                                    RTSPMethod RequestMethod,
                                    RTSPServerProtocol::Session* pSession);

    HX_RESULT FailAndTeardown(UINT32 uStatusCode,
                                RTSPServerProtocol::Session* pSession);

    HX_RESULT setupPlay(RTSPServerProtocol::Session* pSession,
                        CHXString& sessionID, CHXSimpleList& ruleList,
                        UINT64 tBegin, UINT64 tEnd);

    HX_RESULT SendPlayResponse(UINT32 ulStatusCode, UINT32 ulCSeq,
                               RTSPServerProtocol::Session* pSession);

    HX_RESULT SetScaleDone(HX_RESULT pnStatus, CHXString& sSessionID,
                           FIXED32 fScale);

    HX_RESULT parseRange(IHXBuffer* pAttr,
                         IHXBuffer* pVal,
                         REF(UINT64) tBegin,
                         REF(UINT64) tEnd,
                         REF(RTSPRange::RangeType) rangeUnits);
    HX_RESULT setState              (IHXRTSPRequestMessage* pMsg,
                                     RTSPServerProtocol::State state);
    HX_RESULT setState              (const char* pSessionID,
                                     RTSPServerProtocol::State state);
    HX_RESULT getState              (IHXRTSPRequestMessage* pReqMsg,
                                     RTSPServerProtocol::State& state);
    HX_RESULT getState              (const char* pSessionID,
                                     RTSPServerProtocol::State& state);
    HX_RESULT getSessionID(IHXRTSPMessage* pMsg, CHXString& sessionID);
    HX_RESULT getSetupSession(IHXRTSPMessage* pMsg,
                              RTSPServerProtocol::Session*& pSession,
                              BOOL& rbHasSessionHeader);
    HX_RESULT SendResponse(UINT32 ulCode, BOOL bSkipOnServRespEv = FALSE);
    HX_RESULT GetAndConvertRFC822Headers(IHXRTSPMessage* pMsg,
        REF(IHXValues*) pRFC822Headers);
    HX_RESULT ConvertAndAddRFC822Headers(IHXRTSPMessage* pMsg,
        IHXValues* pRFC822Headers);
    HX_RESULT _BeginSetup(IHXRTSPMessage* pRTSPMessageStart,
        const char* pcharUrl, CHXString& CHXStringSessionID,
        BOOL& bRTPAvailable);
    BOOL IsInHeaderValues(IHXRTSPMessage* pMessageToSearch,
        const char* pSearchInHeader, const char* pSearchForValue);
    RTPInfoEnum ParseRTPInfoField(IHXMIMEField* pSeqValue,
        UINT16& streamID, UINT16& seqNum, UINT32& ulTimestamp,
        const char*& pControl);

    BOOL CanCreateSession(void);
    void SetActiveStamp(time_t tv_sec=0);
    void KeepAliveCheck(KeepAliveCallback* pCB);
    HX_RESULT SendKeepAlive(Session* pSession);

protected:
    BOOL                        m_bPlayReceived;
    BOOL                        m_bNoRtpInfoInResume;
    BOOL                        m_bDisableTurboPlay;

    INT32                        m_lTurboPlayBW;
    //PV player RTCP RR bug work around
    BOOL                        m_bRTCPRRWorkAround;

    // Support for RTSP Event logging
    BOOL                        m_bTrackEvents;
    BOOL                        m_bIsProxy;
    RTSPStats*                  m_pRtspStatsMgr;
    IHXRTSPAggregateEventStats* m_pAggStats;
    IHXSDPAggregateStats*       m_pSDPStatsMgr;

    /* XXXMC
     * Support for PV Emulation.
     */
    BOOL                        m_bPVEmulationEnabled;
    IHXBuffer*                  m_pPVClientUAPrefix;
    IHXBuffer*                  m_pPVServerId;

    BOOL                        m_bRTPLiveLegacyMode;

    void                        DispatchMessage(void);

    IHXRTSPMessage*             m_pRespMsg;
    RTSPMethod                  m_LastRequestMethod;
    CHXMapLongToObj*            m_pPipelineMap;
};

class RTSPServerSessionManager : public IUnknown 
{
public:
    RTSPServerSessionManager(void);
    ~RTSPServerSessionManager(void);

    const char* addSessionInstance      (const char* pSessionID,
                                         const char* pURL,
                                         UINT32 ulSeqNo,
                                         RTSPServerProtocol* pProt);
    HX_RESULT removeSessionInstance     (const char* pSessionID);
    HX_RESULT removeSessionInstances    (RTSPServerProtocol* pProt);
    HX_RESULT getSessionID              (IHXBuffer* pURL,
                                         RTSPServerProtocol* pProt,
                                         BOOL bNotSetup,
                                         CHXString& sessionID);
    CHXMapStringToOb* getSessionMap     (void) { return &m_sessionMap; }
    RTSPSessionItem* findInstance       (const char* pSessionID);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

private:

    CHXMapStringToOb                    m_sessionMap;
    CHXMapStringToOb                    m_sessionIDHelperMap;
    ULONG32                             m_ulRefCount;
};

#endif /* _RTSPSERV_H_ */
