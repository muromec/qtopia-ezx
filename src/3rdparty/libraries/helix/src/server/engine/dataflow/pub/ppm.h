/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ppm.h,v 1.41 2007/02/02 07:09:54 jzeng Exp $
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
/*
 *  Interfaces for the Player Packet Manager
 */

#ifndef _PPM_H_
#define _PPM_H_

#include "hxdtcvt.h"

#include "hxformt.h"
#include "hxengin.h"
#include "hxasm.h"
#include "source.h" // Forward Declare Interface XXXSMP
#include "timeval.h"
#include "base_callback.h"
#include "servpckts.h"
#include "hxmap.h"
#include "servlist.h"
#include "loadinfo.h"
#include "ihxlist.h"
#include "rtspif.h"
#include "hxpcktflwctrl.h"
#include "pcktflowwrap.h"
#include "isifs.h"
//#define RSD_LIVE_DEBUG

class Process;
class Transport;
class ASMRuleBook;
class BWCalculator;
class StreamDoneCallback;
class PPMStreamData;
class DataConvertShim;
class Player;
class CServerTBF;
class CRSDPacketQueue;
class BRDetector;

_INTERFACE IHXPacketFlowControl;
_INTERFACE IHXRTPInfoSynch;
_INTERFACE IHXSessionStats2;

#define MAX_RESENDS_PER_SECOND 256


class PPM {
public:
    class Session;

    PPM(Process* pProc, BOOL bIsRTP);
    ~PPM();

    void            RegisterSource(IHXPSourceControl* pSourceCtrl,
                                   IHXPacketFlowControl** ppSessionControl,
                                   UINT16 unStreamCount,
                                   BOOL bIsLive, BOOL bIsMulticast,
                                   DataConvertShim* pDataConv);

    void            RegisterSource(IHXPSourceControl* pSourceCtrl,
                                   IHXPacketFlowControl** ppSessionControl,
                                   UINT16 unStreamCount,
                                   BOOL bIsLive, BOOL bIsMulticast,
                                   DataConvertShim* pDataConv,
                                   Player* pPlayerCtrl,
                                   const char* szPlayerSessionId,
                                   IHXSessionStats* pSessionStats);

    void            SessionDone(Session* pSession);

private:
    static void     GetAggregationLimits(_ServerState State,
                                         UINT32  ulActualDeliveryRate,
                                         UINT32* pAggregateTo,
                                         UINT32* pAggregateHighest);

public:
    class TimeCallback: public BaseCallback
    {
    public:
#ifndef _SOLARIS27
        MEM_CACHE_MEM
#endif
        STDMETHOD(Func) (THIS);
        PPM*            m_pPPM;
    };
    friend class TimeCallback;

    class Session : public IHXPSinkPackets, public IHXPSinkInfo,
                    public IHXPacketFlowControl, public IHXPacketResend,
                    public IHXWouldBlockResponse,
                    public IHXDataConvertResponse, public HXListElem,
                    public IHXAccurateClock, public IHXQoSLinkCharSetup,
                    public IHXStreamAdaptationSetup
        {
        public:
            class TimeStampCallback: public BaseCallback
            {
            public:
                STDMETHOD(Func)     (THIS);
                Session*            m_pSession;
                UINT16              m_unStreamNumber;
            };
        friend class TimeStampCallback;

        class ResendCallback: public BaseCallback
        {
        public:
#ifndef _SOLARIS27
            MEM_CACHE_MEM
#endif
            ~ResendCallback();
            STDMETHOD(Func) (THIS);

            Transport*      m_pTransport;
            BasePacket*     m_pPacket;
            UINT32*         m_pZeroMe;
        };

        class TSDCallback: public BaseCallback
        {
        public:
            ~TSDCallback() {};
            STDMETHOD(Func) (THIS);
            PPM::Session* m_pSession;
        };
        friend class TSDCallback;

        class CBRCallback: public BaseCallback
        {
        public:
            ~CBRCallback() {};
            STDMETHOD(Func) (THIS);
            PPM::Session* m_pSession;
        };
        friend class CBRCallback;

        Session(Process* p, UINT16 unStreamCount,
                PPM* m_pPPM,
                IHXPSourcePackets*,
                IHXPSourceLivePackets*,
                BOOL bIsLive,
                BOOL bIsMulticast);

        void    SendTimeStampedPacket(UINT16 m_unStreamNumber);

        void    Done();

        STDMETHOD(QueryInterface)   (THIS_
                                     REFIID riid,
                                     void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)  (THIS);

        STDMETHOD_(ULONG32,Release) (THIS);

        /* Sink Packets Method */
        STDMETHOD(PacketReady)      (THIS_
                                     HX_RESULT               ulStatus,
                                     IHXPacket*             pPacket);

        /* Sink Packets Info */
        STDMETHOD_(ULONG32,GetBandwidth) (THIS);

        /* Packet Resend Methods */
        STDMETHOD(OnPacket)         (THIS_
                                     UINT16 uStreamNumber,
                                     BasePacket** ppPacket);

        /* IHXWouldBlockResponse methods */
        STDMETHOD(WouldBlock)       (THIS_ UINT32 id);
        STDMETHOD(WouldBlockCleared)(THIS_ UINT32 id);

        /***********************************************************************
         *  IHXDataConvertResponse
         */
        STDMETHOD(DataConvertInitDone) (THIS_ HX_RESULT status);

        STDMETHOD(ConvertedFileHeaderReady) (THIS_
                                HX_RESULT status, IHXValues* pFileHeader);

        STDMETHOD(ConvertedStreamHeaderReady) (THIS_
                                HX_RESULT status, IHXValues* pStreamHeader);

        STDMETHOD(ConvertedDataReady) (THIS_ HX_RESULT status,
                                            IHXPacket* pPacket);

        STDMETHOD(SendControlBuffer) (THIS_ IHXBuffer* pBuffer);

        //IHXQoSLinkCharSetup
        STDMETHOD (SetLinkCharParams) (THIS_
                                 LinkCharParams* /* IN */ pLinkCharParams);
        STDMETHOD (GetLinkCharParams) (THIS_
				 UINT16 unStreamNum,
                                 REF(LinkCharParams) /* OUT */ linkCharParams);

        /* IHXStreamAdaptationSetup */
        STDMETHOD (GetStreamAdaptationScheme)   (THIS_
                                    REF(StreamAdaptationSchemeEnum) /* OUT */ enumAdaptScheme );
        STDMETHOD (SetStreamAdaptationScheme)   (THIS_
                                    StreamAdaptationSchemeEnum enumAdaptScheme );
        STDMETHOD (SetStreamAdaptationParams) (THIS_
                                    StreamAdaptationParams* /* IN */ pStreamAdaptParams);
        STDMETHOD (GetStreamAdaptationParams) (THIS_
				    UINT16 unStreamNum,
                                    REF(StreamAdaptationParams) /* OUT */ streamAdaptParams);
        STDMETHOD (UpdateStreamAdaptationParams) (THIS_
                                    StreamAdaptationParams* /* IN */ pStreamAdaptParams);
        STDMETHOD (UpdateTargetProtectionTime) (THIS_
                                    StreamAdaptationParams* /* IN */ pStreamAdaptParams);

        /* IHXPacketFlowControl Methods */
        STDMETHOD(Play)();
        STDMETHOD(StartSeek)(UINT32 ulTime);
        STDMETHOD(Activate)();
        STDMETHOD(WantWouldBlock)();
        STDMETHOD(SeekDone)();
        STDMETHOD(SetEndPoint)(UINT32 ulEndPoint, BOOL bPause);
        STDMETHOD(SetStartingTimestamp)(UINT32 ulStartingTimestamp);
        STDMETHOD(RegisterStream)(Transport* pTransport,
                                  UINT16 uStreamNumber,
                                  ASMRuleBook* pRuleBook,
                                  IHXValues* pHeader);
        STDMETHOD(RegisterStream)(Transport* pTransport,
                                  UINT16 uStreamGroupNumber,
                                  UINT16 uStreamNumber,
                                  ASMRuleBook* pRuleBook,
                                  IHXValues* pHeader);
        STDMETHOD(GetSequenceNumber)(UINT16 uStreamNumber,
                                     UINT16& uSequenceNumber);
        STDMETHOD(Pause)(UINT32 ulPausePoint);
        STDMETHOD(StreamDone)(UINT16 uStreamNumber, BOOL bForce = FALSE);
        STDMETHOD(SetDropRate)(UINT16 uStreamNumber, UINT32 uDropRate);
        STDMETHOD(SetDropToBandwidthLimit)(UINT16 uStreamNumber,
                                           UINT32 ulBandwidthLimit);
        STDMETHOD(SetDeliveryBandwidth)(UINT32 ulBackOff, UINT32 ulBandwidth);
        STDMETHOD(HandleSubscribe)(INT32 lRuleNumber, UINT16 unStreamNumber);
        STDMETHOD(HandleUnSubscribe)(INT32 lRuleNumber, UINT16 unStreamNumber);
        STDMETHOD_(ULONG32, GetDeliveryRate)() { return m_ulDeliveryRate;};
        STDMETHOD(ControlDone)();
        STDMETHOD_(float, SetSpeed)(float fSpeed);
        STDMETHOD(SetBandwidth)(UINT32 ulBandwidth);

        // IHXAccurateClock methods

        STDMETHOD_(HXTimeval,GetTimeOfDay)      (THIS);

    private:
                                    ~Session();
        void                        HandleTimeLineStateChange(BOOL bState);
        void                        ResetSessionTimeline(ServerPacket* pNextPkt,
                                             UINT16 usStreamNumber,
                                             BOOL bIsPostSeekReset);
        void                        Play(BOOL);
        void                        Pause(BOOL bWouldBlock, UINT32 ulPausePoint = 0);
        void                        StartPackets();
        void                        SetPlayerInfo(Player* pPlayerControl,
                                                  const char* szPlayerSessionId,
                                                  IHXSessionStats* pSessionStats);
        void                        GetNextPacket(UINT16 unStreamNumber);
        void                        SetStreamStartTime(UINT32 ulStreamNum,
                                                       UINT32 ulTimestamp);

        void                        JumpStart(UINT16 uStreamNumber);
        void                        ScheduleStreamDone(Session* pSession,
                                                Transport* pTransport,
                                                PPMStreamData* pSD,
                                                UINT16 unStreamNumber);
        void                        HandleStreamDone(PPMStreamData* pSD);
        void                        HandleUnStreamDone(PPMStreamData* pSD);
        UINT32                      SendPacketQueue(Transport* pTransport);
        void                        QueuePacket(ServerPacket* pPacket,
                                        UINT32 ulPacketSize,
                                        Transport* pTransport);
        void                        SetConverter(DataConvertShim*);
        HX_RESULT                   SessionPacketReady(HX_RESULT status,
                                        IHXPacket* pPacket);
        HX_RESULT                   LiveSessionPacketReady(HX_RESULT status,
                                        IHXPacket* pPacket);
        void                        TransmitPacket(ServerPacket* pPacket);

        HX_RESULT                   HandleDefaultSubscription(void);
        HX_RESULT                   UpdatePlayTime(void);

        HX_RESULT ProcessLivePacket(HX_RESULT ulStatus,
                                    IHXPacket* pPacket);
        void CalculateInitialSendingRate();
        void CalculatePreDataAmount();
        UINT32 GetRTPHeaderSize(IHXBuffer* pBuffer);
        BOOL DoesUseWirelineLogic();
        void GetAllRSDConfigure();
        HX_RESULT MobileLiveCBRPacketReady();
        HX_RESULT MobileLiveTSDPacketReady();
        HX_RESULT WirelineLivePacketReady();
        BOOL IsMobileCBR();
        HX_RESULT SendAndScheduleTSDPacket();
        HX_RESULT SendLiveCBRPacket();
        void ResetAdjustingState();

        Process*                    m_pProc;
        IHXPSourcePackets*          m_pSourcePackets;
        IHXPSourceLivePackets*      m_pSourceLivePackets;
        UINT32                      m_bThreadSafeGetPacket;
        BOOL                        m_bIsLive;
        PPMStreamData*              m_pStreamData;
        ULONG32                     m_ulRefCount;
        UINT16                      m_unStreamCount;
        PPM*                        m_pPPM;
        UINT32                      m_uEndPoint;
        UINT32                      m_bIsPausePointSet;
        BOOL                        m_bPaused;
        BOOL                        m_bPlayPendingOnSeek;
        UINT32                      m_ulTSDMark;
        Timeval                     m_tTimeLineStart;
        BOOL                        m_bTimeLineSuspended;
        BOOL                        m_bInSeek;
        BOOL                        m_bInitialSubscriptionDone;
        BOOL                        m_bInitialPlayReceived;
        BOOL                        m_bGetPacketsOutstanding;
        BOOL                        m_bIsMulticast;
        UINT32                      m_ulPacketsOutstanding;
        BOOL                        m_bAttemptingBackToBack;
        BasePacket**                m_pPacketQueue;
        UINT8                       m_ucPacketQueuePos;
        UINT16                      m_ulPacketQueueSize;
        UINT32                      m_ulActualDeliveryRate;
        UINT32                      m_ulDeliveryRate;
        BOOL                        m_bSourceIsDone;
        UINT32                      m_ulNumStreamDones;
        IHXPSourceLiveResync*       m_pLiveResync;

        BOOL                        m_bIsDone;
        UINT32                      m_pResendIDs[MAX_RESENDS_PER_SECOND];
        UINT32                      m_ulResendIDPosition;
        BOOL                        m_bSubscribed;
        UINT32                      m_ulWouldBlocking;
        BOOL                        m_bWouldBlockAvailable;
        DataConvertShim*            m_pConvertShim;

        void                        Reschedule(UINT16 unStreamNumber,
                                        float fOldRatio);
        void                        HandleSpeedChange(float fOldSpeed);
        float                       m_fDeliveryRatio;

        BOOL                        IsPaused()
                                    {return m_bPaused || m_ulWouldBlocking;}
        IHXPacket*                  m_pConvertingPacket;

    IHXLivePacketBufferProvider* m_pPacketBufferProvider;
    BOOL                         m_bIsLLL;
    INT32                        m_lExtraMediaRateAmount;
    INT32                        m_lMinTokenBucketCeiling;
    CServerTBF*                  m_pServerTBF;
    UINT32                       m_ulClientBandwidth;
    UINT32                       m_ulPacketSent;
    INT32                        m_lCPUThresholdForRSD;
    BOOL                         m_bCheckLiveRSD;
    INT32                        m_ulExtraPrerollInPercentage;
    CRSDPacketQueue*             m_pRSDPacketQueue;
    BOOL                         m_bIsWireline;
    BOOL                         m_bIsMobileCBR;
    UINT32                       m_ulInitialQDuration;
    BRDetector*                  m_pBRDetector;

    BOOL                         m_bIsReflector;
    UINT16                       m_unRTCPRule;

    IHXASMSource*       m_pASMSource;
    Player*             m_pPlayerControl;
    IHXBuffer*          m_pPlayerSessionId;
    IHXSessionStats*    m_pSessionStats;
    IHXSessionStats2*   m_pSessionStats2;
    BOOL                m_bSeekPacketPending;
    BOOL                m_bSessionPlaying;
    BOOL                m_bStallPackets;
    UINT16              m_usStreamsRestarted;
    BOOL                m_bNeedStreamStartTime;

    IHXList*            m_pBlockedQ;
    IHXList*            m_pBlockedTSDQ;
    UINT32              m_uBlockedBytes;
    UINT32              m_uMaxBlockedBytes;
    UINT32              m_uMaxBlockedQInMsecs;
    ULONG32             m_ulHeadTSDTime;
    ULONG32             m_ulLastTSDTime;
    BOOL                m_bRTPInfoRequired;

    BOOL                m_bSubscribeCountersInRSS;

    // PlayTime tracking.  Declare as Timeval so we can do math with them.

                        // time of most recent PLAY request
    Timeval             m_tvRTSPPlayTime;
                        // all elapsed time from prior play/pause cycles
    Timeval             m_tvBankedPlayTime;
    IHXAccurateClock*   m_pAccurateClock;
    IHXRTPInfoSynch*    m_pRTPInfoSynch;
    UINT16              m_unRTPSynchStream;
    UINT32              m_ulBrecvDebugLevel;
    UINT32              m_unKeyframeStream;
    UINT32              m_unKeyframeRule;
    INT32               m_lCPUUsage;
    BOOL                m_bEnableLiveRSDLog;
    BOOL                m_bEnableRSDPerPacketLog;
    BOOL                m_ulTBFBandwidth;
    UINT32              m_ulRSDDebugTS;
    UINT32              m_ulFirstPacketTS;
    UINT32              m_ulPreData;
    UINT32              m_ulTotalBytesSent;
#ifdef RSD_LIVE_DEBUG
    UINT32              m_ulPrevTotalBytesSent;
    UINT32              m_ulLastTS;
    UINT32              m_uPacketListStart;
#endif
    BOOL                m_bSentPredata;
    UINT32              m_ulIteration;
    BOOL                m_bPktBufQProcessed;

    BOOL                m_bFirstPacket;
    UINT32              m_RSDCBhandle;
    TSDCallback*        m_pTSDCB;
    CBRCallback*        m_pCBRCB;
    BOOL                m_bAdjustingQLength;
    BOOL                m_bOversend;
    Timeval             m_tCBRCheckQStartingTime;

    UINT32              m_bQueueDebugLog;
    UINT32              m_ulQueueDebugTS;

    UINT16              m_uFirstStreamRegistered;
    UINT16              m_uNumStreamsRegistered;

    LinkCharParams*     m_pSessionAggrLinkCharParams;
    BOOL                m_bStreamLinkCharSet;

    StreamAdaptationParams     *m_pAggRateAdaptParams;
    StreamAdaptationSchemeEnum  m_enumStreamAdaptScheme;

        friend class PPM;
        friend class TimeCallback;
        friend class StreamDoneCallback;
        friend class PPMStreamData;
    };

private:
    BOOL SendNextPacket(Session* pSession = 0);
    void ChangeDeliveryBandwidth(INT32 lChange,
                                 BOOL bReportChange,
                                 PPMStreamData* pSD = 0,
                                 BOOL bOverrideAverage = FALSE);
    void CommitPendingBandwidth();
    void RecalcActualDeliveryRate();

    Process*                        m_pProc;
    BOOL                            m_bIsRTP;

    BOOL                            m_bInitialPlayReceived;
    TimeCallback*                   m_pTimeCallback;
    HXList                          m_Sessions;
    BOOL                            m_bIdle;
    UINT32                          m_uScheduledSendID;
private:
    UINT32                          m_ulPendingBitRate;
    UINT32                          m_ulDeliveryBitRate;
    UINT32                          m_ulActualDeliveryRate;
    UINT32                          m_ulAdjustedDeliveryRate;
    Timeval                         m_tNextSendTime;
    Timeval                         m_tLastSendTime;
    UINT32                          m_ulBackToBackCounter;
    UINT32                          m_ulBackToBackFreq;
    BOOL                            m_bInformationalAggregatable;
    BOOL                            m_bDidLock;

    friend class Session;
    friend class StreamDoneCallback;
    friend class PPMStreamData;
};

class StreamDoneCallback: public BaseCallback
{
public:
#ifndef _SOLARIS27
    MEM_CACHE_MEM
#endif
    STDMETHOD(Func) (THIS);

    PPM::Session*               m_pSession;
    Transport*        m_pTransport;
    PPMStreamData*              m_pSD;
    UINT16                      m_unStreamNumber;
};

#endif /* _PPM_H_ */
