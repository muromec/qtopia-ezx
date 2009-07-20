/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: basicpcktflow.h,v 1.22 2007/01/30 00:48:42 jzeng Exp $
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
#ifndef _BASICPCKTFLOW_H_
#define _BASICPCKTFLOW_H_

#include "hxpcktflwctrl.h"
#include "hxdtcvt.h"

class PacketStream;
class PacketFlowManager;
_INTERFACE IHXPSourcePackets;

#define MAX_RESENDS_PER_SECOND 256

class BasicPacketFlow : public IHXPSinkPackets,
                        public IHXPSinkInfo,
                        public IHXPacketFlowControl,
                        public IHXPacketResend,
                        public IHXWouldBlockResponse, 
                        public IHXDataConvertResponse,
                        public HXListElem,
                        public IHXServerPacketSource,
                        public IHXAccurateClock
{
public:
    static BasicPacketFlow* Create(Process* p,
                                   IHXSessionStats* pSessionStats,
                                   UINT16 unStreamCount,
                                   PacketFlowManager* pFlwObj,
                                   IHXPSourcePackets* pSourcePackets,
                                   BOOL bIsMulticast);

    static BasicPacketFlow* Create(Process* p,
                                   IHXSessionStats* pSessionStats,
                                   UINT16 unStreamCount,
                                   PacketFlowManager* pFlwObj,
                                   IHXPSourceLivePackets* pSourcePackets,
                                   BOOL bIsMulticast);

    static BasicPacketFlow* Create(Process* p,
                                   IHXSessionStats* pSessionStats,
				   UINT16 unStreamCount,
				   PacketFlowManager* pFlowMgr,
				   IHXServerPacketSource* pSource,
				   BOOL bIsMulticast,
				   BOOL bIsLive);
    
    virtual ~BasicPacketFlow();

    virtual UINT32 GetConstantBitRate();

    virtual void Done();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)();
    STDMETHOD_(ULONG32,Release)();

    // Sink Packets
    STDMETHOD(PacketReady)(HX_RESULT ulStatus, IHXPacket* pPacket);
	
    // Sink Packets Info
    STDMETHOD_(ULONG32,GetBandwidth) (THIS);

    // Packet Resend Methods
    STDMETHOD(OnPacket)(UINT16 uStreamNumber, BasePacket** ppPacket);
	
    // IHXWouldBlockResponse methods
    STDMETHOD(WouldBlock)(UINT32 id);
    STDMETHOD(WouldBlockCleared)(UINT32 id);

    // IHXDataConvertResponse
    STDMETHOD(DataConvertInitDone)(HX_RESULT status);
    STDMETHOD(ConvertedFileHeaderReady)(HX_RESULT status, IHXValues* pFileHeader);
    STDMETHOD(ConvertedStreamHeaderReady)(HX_RESULT status,
                                          IHXValues* pStreamHeader);
    STDMETHOD(ConvertedDataReady)(HX_RESULT status, IHXPacket* pPacket);
    STDMETHOD(SendControlBuffer)(IHXBuffer* pBuffer);
	
    // IHXPacketFlowControl Methods
    STDMETHOD(Play)();
    STDMETHOD(StartSeek)(UINT32 ulTime) = 0;
    STDMETHOD(Activate)() = 0;
    STDMETHOD(WantWouldBlock)();
    STDMETHOD(SeekDone)() = 0;
    STDMETHOD(SetEndPoint)(UINT32 ulEndPoint, BOOL bPause);
    STDMETHOD(SetStartingTimestamp)(UINT32 ulStartingTimestamp);
    STDMETHOD(RegisterStream)(Transport* pTransport,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader) PURE;
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
    STDMETHOD_(ULONG32, GetDeliveryRate)();
    STDMETHOD(ControlDone)();
    STDMETHOD_(float, SetSpeed)(float fSpeed);
    STDMETHOD(SetBandwidth)(UINT32 ulBandwidth);

    // IHXServerPacketSource
    STDMETHOD(SetSink)(THIS_ IHXServerPacketSink* pSink);
    STDMETHOD(StartPackets)(THIS);
    STDMETHOD(GetPacket)(THIS);
    STDMETHOD(SinkBlockCleared)(UINT32 ulStream);
    STDMETHOD(EnableTCPMode) (THIS);

    // IHXAccurateClock methods
    STDMETHOD_(HXTimeval,GetTimeOfDay)      (THIS);

protected:
    virtual void      HandleResume() = 0;
    virtual void      InitialStartup() = 0;

    BasicPacketFlow(Process* p,
                    IHXSessionStats* pSessionStats,
                    UINT16 unStreamCount, 
                    PacketFlowManager* pFlwObj,
                    BOOL bIsMulticast);

    virtual void      HandleTimeLineStateChange(BOOL bState);

    void              ResetSessionTimeline(ServerPacket* pNextPkt,
                                   UINT16 usStreamNumber,
                                   BOOL bIsPostSeekReset);

    virtual void      Play(BOOL);

    virtual void      Pause(BOOL bWouldBlock, UINT32 ulPausePoint = 0);

    virtual void      SetPlayerInfo(Player* pPlayerControl, 
                                    const char* szPlayerSessionId,
                                    IHXSessionStats* pSessionStats);

    virtual void      SetStreamStartTime(UINT32 ulStreamNum,
                                         UINT32 ulTimestamp);

    virtual void      ScheduleStreamDone(BasicPacketFlow* pFlow,
                                         Transport* pTransport,
                                         PacketStream* pStream,
                                         UINT16 unStreamNumber);

    virtual void      HandleStreamDone(PacketStream* pStream);

    virtual void      HandleUnStreamDone(PacketStream* pStream);

    virtual void      SetConverter(DataConvertShim*);

    virtual HX_RESULT SessionPacketReady(HX_RESULT status,
                                         IHXPacket* pPacket) = 0;

    virtual void      TransmitPacket(ServerPacket* pPacket) = 0;

    virtual void      HandleSpeedChange(float fOldSpeed);
    virtual void      RescheduleTSD(UINT16 unStream, float fOldRatio) = 0;

    ServerPacket*     HXPacketToServerPacket(IHXPacket* pPacket);
    UINT16	      TranslateStreamNumToStreamGroupNum(UINT16 uStreamNum);

    HX_RESULT         UpdatePlayTime(void);
    
    BOOL      IsPaused() {return m_bPaused || m_ulWouldBlocking;}
    BOOL      IsDone() { return m_bIsDone; }

    float                   m_fDeliveryRatio;
    Process*		    m_pProc;
    UINT32		    m_bThreadSafeGetPacket;
    PacketStream*           m_pStreams;
    IHXQoSRateManager*      m_pRateManager;
    ULONG32		    m_ulRefCount;
    UINT16		    m_unStreamCount;
    UINT16		    m_uNumStreamsRegistered;
    PacketFlowManager*      m_pFlowMgr;
    UINT32		    m_uEndPoint;
    UINT32		    m_bIsPausePointSet;
    BOOL                    m_bPaused;
    BOOL                    m_bPlayPendingOnSeek;
    Timeval		    m_tTimeLineStart;
    BOOL		    m_bTimeLineSuspended;
    BOOL                    m_bInSeek;
    BOOL		    m_bInitialSubscriptionDone;
    BOOL		    m_bInitialPlayReceived;
    BOOL		    m_bGetPacketsOutstanding;
    BOOL		    m_bIsMulticast;
    UINT32		    m_ulPacketsOutstanding;
    BOOL		    m_bSourceIsDone;
    UINT32		    m_ulNumStreamDones;
    BOOL		    m_bIsDone;
    UINT32		    m_pResendIDs[MAX_RESENDS_PER_SECOND];
    UINT32		    m_ulResendIDPosition;

    BOOL		    m_bSubscribed;
    BOOL		    m_bAutoSubscription;

    UINT32		    m_ulWouldBlocking;
    BOOL                    m_bWouldBlockAvailable;
    DataConvertShim*	    m_pConvertShim;
    IHXPacket*		    m_pConvertingPacket;
    Player*                 m_pPlayerControl;
    IHXBuffer*              m_pPlayerSessionId;
    BOOL                    m_bSeekPacketPending;
    BOOL                    m_bSessionPlaying;
    BOOL                    m_bDeliveryBandwidthSet;
    IHXSessionStats*        m_pStats;

    // PlayTime tracking.  Declare as Timeval so we can do math with them.

    			// time of most recent PLAY request
    Timeval             m_tvRTSPPlayTime;
    			// all elapsed time from prior play/pause cycles
    Timeval		m_tvBankedPlayTime;
    IHXAccurateClock*	m_pAccurateClock;

    UINT16		m_uFirstStreamRegistered;

    /* RTSP RTP-Info header support: */
    BOOL                       m_bRTPInfoRequired;

    // bandwidth info
    UINT32 m_ulSetDeliveryBandwidth;
    UINT32 m_ulBandwidth;

    friend class PacketFlowTimeStampCallback;
    friend class PacketFlowManager;
    friend class PacketFlowTimeCallback;
    friend class PacketStreamDoneCallback;
    friend class PacketStream;
};

class PacketFlowResendCallback: public BaseCallback                          
{
public:
    MEM_CACHE_MEM

    ~PacketFlowResendCallback();                                          
    STDMETHOD(Func) (THIS);
        
    Transport*  m_pTransport;
    BasePacket* m_pPacket;
    UINT32*     m_pZeroMe;
};

#endif //_BASICPCKTFLOW_H_
