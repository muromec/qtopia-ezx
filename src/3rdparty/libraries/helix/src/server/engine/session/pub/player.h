/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: player.h,v 1.54 2007/04/25 00:56:39 darrick Exp $
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

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "hxdtcvt.h"

#include "hxstring.h"
#include "hxslist.h"
#include "srcerrs.h"
#include "sockio.h"
#include "sink.h"
#include "source.h"
#include "hxauth.h"
#include "hxengin.h"
#include "hxallow.h"
#include "servlist.h"
#include "hxformt.h"
#include "clbwcont.h"
#include "pcktflowwrap.h"
#include "hxclientprofile.h"
#include "hxasm.h"

#include "hxstreamadapt.h"

class        URL;
class        TCPIO;
class        UDPIO;
struct        BW_encoding;
class        Client;
class        Process;
class        HXProtocol;
class        Player;
class        CHXSimpleList;
class        CHXMapLongToObj;
class        ServerRequest;
class        ServerRequestWrapper;
class        AllowanceMgr;
class        MulticastSession;
class        PacketFlowWrapper;
class        MulticastSession;
class        ASMRuleBook;
struct       IHXSessionStats;
struct       IHXSessionStats2;
class        Transport;
class        QoSSignalBusController;
class        RateSelectionInfo;

// Forward decls from srcfinder.h
class        BasicSourceFinder;
class        CachingSourceFinder;

_INTERFACE   IHXFileFormatHeaderAdvise;
_INTERFACE IHXBlockFormatObject;
_INTERFACE IHXPacketFlowControl;
_INTERFACE IHXQoSSignal;
_INTERFACE IHXQoSSignalBus;
_INTERFACE IHXQoSSignalBusController;
_INTERFACE IHXQoSProfileConfigurator;
_INTERFACE IHXUberStreamManager;
_INTERFACE IHXUberStreamManagerConfig;


#define ALLOWANCE_DEBUG_MSG(debug_flag, allow_manager, msg) \
            if ( (debug_flag) )                             \
            {                                               \
                if ((allow_manager))                        \
                {                                           \
                    (allow_manager)->PrintDebugInfo();      \
                }                                           \
                printf  msg;                                \
                fflush(stdout);                             \
            }                                               \
            else

class AllowanceSerializer
{
public:
    AllowanceSerializer(BOOL bPrintDebugMessage);
    ~AllowanceSerializer();
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     * You may have several allowance plugins.  We want to call their OnURLs
     * serially, because one allowance plugin may do things that have
     * precedence over another plugin, such as a URL redirect.  This
     * version doesn't actually order the plugins by precedence yet,
     * but it sets the stage.
     *
     * Call our OnURL to start the series of calls to each plugin's OnURL.
     * They will report back to Player::Session::OnURLDone.
     * Player::Session::OnURLDone then calls into our OnURLDone, and
     * we report back one of the following statuses:
     *
     *    more_plugins -- we have more plugins to check
     *    reject_playback -- reject this clip
     *    allow_playback - allow this clip to play
     *
     * If you get a return code of more_plugins, you should call CallNextOne()
     * to check the next plugin.
     *
     * Player::Session needs to make sure it doesn't go away while any of the
     * allowance plugins may still potentially call OnUrlDone.  Player::Session
     * should AddRef itself before calling Allowance::Serializer::OnUrl.  It
     * should release itself after Allowance::Serializer::OnUrlDone returns a
     * status other than more_plugins.
     */

    enum AllowanceStatus
    {
        more_plugins,
        reject_playback,
        allow_playback,
        more_plugins_withoutcurrent,
        allow_playback_withoutcurrent
    };
    void OnURL (ServerRequest* pRequest, CHXSimpleList* pAllowanceMgrs);
    AllowanceMgr* OnURLDone (HX_RESULT status, REF(enum AllowanceStatus) OverallStatus);
    void CallNextOne();

private:
    struct MgrInfo {
        AllowanceMgr* pMgr;
    };
    struct MgrInfo* m_pArray;
    int m_NumPlugins;
    int m_Curr;
    ServerRequest* m_pRequest;
    ULONG32 m_ulRefCount;

    BOOL m_bPrintDebugMessage;
};

class Player : public HXListElem
{
public:
    class Session: public IHXPSinkControl,
                   public IHXClientBandwidthController,
                   public IHXPlayerController,
                   public IHXDataConvertResponse,
                   public IHXPlayerControllerProxyRedirect,
                   public IHXSetPlayParamResponse,
                   public IHXSeekByPacketResponse,
                   public IHXSourceFinderFileResponse,
                   public IHXFileStatResponse,
                   public IHXASMSource
    {
    public:
                                    Session(Process* _proc,
                                            Client* c, Player* p,
                                            const char* pSessionID,
                                            BOOL bRetainEntityForSetup);
        void                            init_stats(IHXBuffer* pLocalAddr);
        void                            init_registry();
        void                            clear_stats();
        void                            clear_registry();
        void                            log_start_time();

        void                            SetupQoSAdaptationInfo();

        void                            setup_transport(BOOL bIsMulticast,
                                        BOOL bIsActuallyRTP,
                                        MulticastSession* pSession,
                                        ASMRuleBook** ppASMRuleBooks = NULL);

        int                             got_url(URL* url, BOOL bUseRTP);
        void                            Done(HX_RESULT status);
        void                            IfDoneCleanup();
        void                            DoDone();

        void                            clear_allowance_list();
        void                            clear_stream_list();
        void                            update_statistics();
        void                            SetStreamSequenceNumbers();
        void                            SetStreamSequenceNumbers(UINT32 ulFromTS);
        IHXSessionStats*                GetSessionStats() { return m_pStats; }

#ifdef XXXAAK_PERF_NO_ALLOWANCE
        HX_RESULT InsertGenericResponseHeaders(IHXRequest* pRequest);
#endif /* XXXAAK_PERF_NO_ALLOWANCE */
        HX_RESULT                    SessionInitDone(HX_RESULT status);
        HX_RESULT                    SessionFileHeaderReady(HX_RESULT status,
                                                        IHXValues* pHeader);
        HX_RESULT                    SessionStreamHeaderReady(HX_RESULT status,
                                                        IHXValues* pHeader);
        inline IHXFileFormatHeaderAdvise*
                                     GetFFAdviseObj() { return m_pFFAdvise; };

        /* IUnknown methods */
        STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)  (THIS);
        STDMETHOD_(ULONG32,Release) (THIS);


        /* IHXPSinkControl methods */
        STDMETHOD(InitDone)            (THIS_ HX_RESULT ulStatus);
        STDMETHOD(FileHeaderReady)  (THIS_ HX_RESULT ulStatus,
                                     IHXValues* pHeader);
        STDMETHOD(StreamHeaderReady)(THIS_ HX_RESULT ulStatus,
                                     IHXValues* pHeader);
        STDMETHOD(StreamDone)            (THIS_ UINT16 unStreamNumber);
        STDMETHOD(SeekDone)            (THIS_ HX_RESULT ulStatus);


        /* IHXPlayerController Methods */
        STDMETHOD(Pause)                (THIS);
        STDMETHOD(Resume)               (THIS);
        STDMETHOD(Disconnect)           (THIS);
        STDMETHOD(AlertAndDisconnect)   (THIS_ IHXBuffer* pAlert);
        STDMETHOD(HostRedirect)                (THIS_ IHXBuffer* pHost,
                                         UINT16 nPort);
        STDMETHOD(NetworkRedirect)        (THIS_ IHXBuffer* pURL,
                                         UINT32 ulSecsFromNow);
        STDMETHOD(Redirect)                (THIS_ IHXBuffer* pPartialURL);

        /* IHXPlayerControllerProxyRedirect Methods */
        STDMETHOD(NetworkProxyRedirect)        (THIS_ IHXBuffer* pURL);

        /* IHXClientBandwidthController Methods */

        //XXXDPL Candidate for conversion into non-interface method if no plugin
        // uses this method.
        STDMETHOD(GetCurrentBandwidth) (THIS_ REF(ULONG32) ulBandwidth);

        //XXXDPL Candidates for removal if no plugins use these methods.
        STDMETHOD(GetBandwidthStep) (THIS_ ULONG32 ulUpperBound,
                                           REF(ULONG32) ulBandwidth);
        STDMETHOD(SetBandwidthLimit) (THIS_ ULONG32 ulBandwidth);


        /***********************************************************************
         *  IHXDataConvertResponse
         */
        STDMETHOD(DataConvertInitDone)  (THIS_ HX_RESULT status);

        STDMETHOD(ConvertedFileHeaderReady) (THIS_
                                HX_RESULT status, IHXValues* pFileHeader);

        STDMETHOD(ConvertedStreamHeaderReady) (THIS_
                                HX_RESULT status, IHXValues* pStreamHeader);

        STDMETHOD(ConvertedDataReady)   (THIS_ HX_RESULT status,
                                            IHXPacket* pPacket);

        STDMETHOD(SendControlBuffer)    (THIS_ IHXBuffer* pBuffer);

        // IHXSourceFinderFileResponse
        STDMETHOD(FindDone)             (THIS_ HX_RESULT status,
                                            IUnknown* pSourceContainer,
                                            IUnknown* pFileObject);

        void ControlBufferReady(const char* pBufferData);

        /*
         * PLAY parameters support
         */
        float       SetSpeed(FIXED32 fSpeed); // syncronous call...
        HX_RESULT   SetScale(FIXED32 fScale); // asyncronous call...
        HX_RESULT   SeekByPacket(UINT32 ulPacketNumber); // asyncronous call...

        /* IHXSetPlayParamResponse */
        STDMETHOD(SetParamDone)             (THIS_ HX_RESULT status,
                                           HX_PLAY_PARAM param,
                                           UINT32 ulValue);

        /* IRMASeekByPacketResponse methods */
        STDMETHOD(SeekToPacketDone)     (THIS_
                                       HX_RESULT status,
                                       UINT32 ulStartingTimestamp);

        void        FindDone(HX_RESULT status, IHXPSourceControl* source);

        /* IHXASMSource */
        STDMETHOD(Subscribe)    (THIS_ UINT16 uStreamNumber, UINT16 uRuleNumber);
        STDMETHOD(Unsubscribe)  (THIS_ UINT16 uStreamNumber, UINT16 uRuleNumber);

        /* Protocol responses */
        void                        handle_subscribe(UINT16 ruleNumber,
                                        UINT16 streamNumber);
        void                        handle_unsubscribe(UINT16 ruleNumber,
                                        UINT16 streamNumber);
        void                        set_drop_rate(UINT16 streamNumber,
                                              UINT32 dropRate);
        void                        set_dropto_bandwidth_limit(
                                            UINT16 streamNumnber,
                                            UINT32 ulBandwidthLimit);
        void                        set_delivery_bandwidth(
                                            UINT32 ulBackOff,
                                            UINT32 ulBandwidth);
        void                        handle_backchannel_packet(IHXPacket* pPacket);
        void                        begin();
        void                        pause(UINT32 ulPausePoint = 0);
        void                        seek(UINT32 offset);
        void                        set_endpoint(UINT32 endpoint);
        void                        set_byterange(UINT64 ulFrom, UINT64 ulTo);
        void                        set_midbox(BOOL bMidBox);

        void                        SendAlert(IHXBuffer* pAlert);
        void                        SendAlert(StreamError err);

        void                        InitAllowancePlugins(void);
        void                        SetRegistryID(UINT32 ulPlayerRegID);
        void                        OnURL(ServerRequest* pRequest);
        HX_RESULT                   OnURLDone(HX_RESULT status);
        void                        OnBegin(void);
        void                        OnPause(void);
        void                        OnStop(void);
        HX_RESULT                   HandleStreamAdaptation(REF(const StreamAdaptationParams) streamAdapt,
                                                                BOOL bHlxStreamAdaptScheme);
        LinkCharParams              Adjust3GPPLinkCharUnits(REF(const LinkCharParams) RawLinkCharParams);
        HX_RESULT                   Handle3GPPLinkChar(REF(const LinkCharParams) RawLinkCharParams);
        HX_RESULT                   HandleStreamSetup(UINT16 uStreamNumber,
                                                      UINT16 uStreamGroupNumber);

        IHXValues*                  FlattenConvertedHeader(IHXValues* pFrom,
                                                        const char* pKeyKey);

        HX_RESULT                   FindSource(URL* pURL);
        HX_RESULT                   FindSourceDone(HX_RESULT status,
                                        IUnknown* pSource,
                                        IUnknown* pFileObject);

        STDMETHOD(StatDone)                    (THIS_
                                                HX_RESULT status,
                                                UINT32 ulSize,
                                                UINT32 ulCreationTime,
                                                UINT32 ulAccessTime,
                                                UINT32 ulModificationTime,
                                                UINT32 ulMode);

        HX_RESULT InitSourceWithBWE();
        BOOL SeeIfThisIsRealDataType(IHXValues* pHeader);

        void SetTransportConverter(DataConvertShim*);

        void SetStreamStartTime(UINT32 ulStreamNum, UINT32 ulTimestamp);

        void ResetSessionTimeline(UINT16 usStreamNumber, UINT32 ulTimelineStart,
                                  Transport* pTransport, BOOL bIsMcastTransport);

        HX_RESULT HandleDefaultSubscription(void);

        HX_RESULT HandleClientAvgBandwidth(UINT32 ulClientAvgBandwidth);

        UINT16 GetSetupStream(UINT16 uStreamGroup);

        UINT32 GetMaxPreRoll() { return m_ulMaxPreRoll; }

        class StreamInfo
        {
            friend class                Session;

        private:
            StreamInfo(UINT16 uStreamGroupNumber, UINT16 stream_number);
            ~StreamInfo(void);

            IHXValues*              m_pActualHeader;
            IHXValues*              m_pConvertedHeader;
            CHXSimpleList*          m_packet_buf;
            BOOL                    m_waiting_for_packet;
            BOOL                    m_is_done;
            UINT16                  m_stream_number;
            UINT16                  m_uStreamGroupNumber;
            IHXBuffer*              m_pRuleBookBuffer;
            ASMRuleBook*            m_pRuleBook;
            BOOL                    m_bSetupReceived;

            //These are used only for Stream Rate Adaptation
            LinkCharParams              *m_pLinkCharParams;
            StreamAdaptationParams      *m_pStreamAdaptParams;
        };

        void SetInactive();
        void ClearInactive();

        class DoneCallback : public IHXCallback
        {
        public:
            DoneCallback(Session* pOwner);
            ~DoneCallback();

            /* IUnknown Interfaces */
            STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
            STDMETHOD_(ULONG32,AddRef)      (THIS);
            STDMETHOD_(ULONG32,Release)     (THIS);

            STDMETHOD(Func)                 (THIS);

        private:
            ULONG32                 m_ulRefCount;
            Session*                m_pOwner;
        };

        class InactivityCallback : public IHXCallback
        {
        public:
            InactivityCallback(Session* pOwner);
            ~InactivityCallback();

            /* IUnknown Interfaces */
            STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
            STDMETHOD_(ULONG32,AddRef)      (THIS);
            STDMETHOD_(ULONG32,Release)     (THIS);

            STDMETHOD(Func)                 (THIS);

        private:
            ULONG32                 m_ulRefCount;
            Session*                m_pOwner;
        };
        friend class InactivityCallback;

        /* Protocol data */
        URL*                            m_url;
        CHXString                       m_sessionID;    //XXXTDM: redundant
        IHXBuffer*                      m_pSessionId;   //XXXTDM: redundant
        UINT32                          m_ulSessionRegistryNumber;
        UINT32                          m_ulSessionStatsObjId;
        IHXSessionStats*                m_pStats;
        IHXSessionStats2*               m_pStats2;
        UINT32                          m_ulRegistryID;
        char*                           m_pRegistryKey;
        BOOL                            m_bIsMidBox;
        IHXValues*                      m_pSessionHeader;

        /* Player state */
        Process*                        m_pProc;
        IHXPacketFlowControl*           m_pSessionControl;

        /* Allowance plugins */
        CHXSimpleList*                  m_pAllowanceMgrs;
        BOOL                            m_bPaused;
        BOOL                            m_bAllowanceDebug;
        AllowanceSerializer*            m_pAllowanceSerializer;

        /* Safe Cleanup */
        BOOL                            m_bDone;
        BOOL                            m_bCleanedUp;
        UINT32                          m_ulFunctionsInUse;

        /* QoS */
        IHXQoSSignal*                   m_pSignal;
        IHXQoSSignalBus*                m_pSignalBus;
        IHXQoSProfileConfigurator*      m_pQoSConfig;
        QoSSignalBusController*         m_pSignalBusCtl;
        RateSelectionInfo*              m_pRateSelInfo;
        IHXBuffer*                      m_pMediaMimeType;
        BOOL                            m_bUseMediaDeliveryPipeline;

        // File system manager stuff
        BasicSourceFinder*              m_pSrcFinder;

        IHXBroadcastMapper*             m_pBroadcastMapper;
        IHXFileMimeMapper*              m_pMimeMapper;
        IHXFileFormatObject*            m_pFileFormat;
        IHXBlockFormatObject*           m_pBlockTransferFileFormat;
        ServerRequest*                  m_pRequest;
        ServerRequestWrapper*           m_pFileRequest;
        ServerRequestWrapper*           m_pFileFormatRequest;

        UINT32                          m_ulLastModifiedTime;

        IHXPSourceControl*              m_pSourceControl;
        IHXUberStreamManager*           m_pUberStreamMgr;
        IHXUberStreamManagerConfig*     m_pUberStreamConfig;
        int                             m_ProtocolSetupAfterHeadersDone;

        //Stream Adaptation
        IHXStreamAdaptationSetup*       m_pStreamAdaptationSetup;
        StreamAdaptationSchemeEnum      m_StreamAdaptationScheme;
        UINT16                          m_unStreamAdaptSetupCount;
        BOOL                            m_bHlxStreamAdaptScheme;

        //3GPP-Link-Char
        IHXQoSLinkCharSetup*            m_pLinkCharSetup;
        LinkCharParams*                 m_pSessionAggrLinkCharParams;
        UINT16                          m_unLinkCharSetupCount;

    private:
        ~Session();
        HX_RESULT SetSessionStreamAdaptScheme();
        void HandleStreamRegistrationInfo(StreamInfo* pStreamInfo);

        BOOL                            m_bBegun;
        ULONG32                         m_ulRefCount;
        StreamInfo**                    m_ppStreams;
        UINT16                          m_uStreamGroupCount;
        UINT16                          m_num_streams;
        UINT16                          m_num_streamheadersdone;
        UINT16                          m_num_streamsdone;

        BOOL                            m_is_ready;
        BOOL                            m_bUseRTP;
        CHXString                       m_BroadcastType;
    public:
        Player*                         m_pPlayer;
        Client*                         m_pClient;
        BOOL                            m_bIsRealDataType;
        BOOL                            m_bBlockTransfer;
        time_t                          m_tIfModifiedSince;
    private:
        BOOL                            m_bNoLatency;
        BOOL                            m_bInitialized;
        BOOL                            m_bRedirected;
        UINT32                          m_uDoneCallbackHandle;
        IHXBuffer*                      m_pMasterRuleBookBuffer;
        ASMRuleBook*                    m_pMasterRuleBook;
        DataConvertShim*                m_pDataConvert;
        DataConvertShim*                m_pMulticastTransportDataConvert;
        IHXBuffer*                      m_pURLBuf;
        IHXValues*                      m_pActualFileHeader;
        IHXValues*                      m_pConvertedFileHeader;
        IHXBuffer*                      m_pHeaderControlBuffer;
        IHXFileFormatHeaderAdvise*      m_pFFAdvise;
        BOOL                            m_bRetainEntityForSetup;
        UINT32                          m_ulInactivityTimeout;
        UINT32                          m_uInactivityCallbackHandle;

        IHXCommonClassFactory*          m_pCommonClassFactory;

        UINT32                          m_ulClientAvgBandwidth; // Avg bandwidth -- reported by the client
        UINT16                          m_uStreamSetupCount;

        UINT32                          m_ulMaxPreRoll;

	//* represents the adaptation parameters 
	//*   for aggregate rate adaptation
	//* Stream specific adaptation params are stored in
	//*   m_ppStreams
	StreamAdaptationParams          *m_pAggRateAdaptParams;
        IHXBuffer*                      m_pBandwidthSignal;
    };
    friend class Session;

    void                Done(HX_RESULT status);
                        Player(Process* _proc, Client* c, int option);
    HX_RESULT           GenerateNewSessionID(CHXString& sessionID, UINT32 ulSeqNo);
    HX_RESULT           NewSession(Session** ppSession,
                                   UINT32 ulSeqNo,
                                   BOOL bRetainEntityForSetup=TRUE);
    HX_RESULT           NewSessionWithID(Session** ppSession,
                                UINT32 ulSeqNo, const char* pSessionID,
                                BOOL bRetainEntityForSetup=TRUE);
    Session*            FindSession(const char* pSessionID);
    HX_RESULT           RemoveSession(const char* pSessionID, HX_RESULT status);
    INT32               NumSessions()
        { return m_pSessions ? m_pSessions->GetCount() : 0; }

    void                        clear_session_list(HX_RESULT status);
    void                        SetStreamStartTime(const char* pSessionID,
                                                   UINT32 ulStreamNum,
                                                   UINT32 ulTimestamp);

    HX_RESULT HandleDefaultSubscription(const char* szSessionID);

    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

private:
                                ~Player();
    ULONG32                     m_ulRefCount;
    Process*                    m_pProc;
public:
    Client*                     m_pClient;
private:
    PacketFlowWrapper*          m_pPacketFlowWrap;
    CHXSimpleList*              m_pSessions;

    static UINT32               m_ulNextSessionID;

};

#endif /*_PLAYER_H_*/
