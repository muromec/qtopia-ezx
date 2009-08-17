/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_cc.h,v 1.39 2007/04/25 00:56:48 darrick Exp $ 
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

#ifndef _QOS_TRAN_RTCP_CC_H_
#define _QOS_TRAN_RTCP_CC_H_

#include "hxstreamadapt.h"

typedef _INTERFACE IHXCallback		      IHXCallback;
typedef _INTERFACE IHXQoSCongestionControl    IHXQoSCongestionControl;
typedef _INTERFACE IHXQoSSignalSink           IHXQoSSignalSink;
typedef _INTERFACE IHXQoSSignalSourceResponse IHXQoSSignalSourceResponse;
typedef _INTERFACE IHXServerPacketSource      IHXServerPacketSource;
typedef _INTERFACE IHXServerPacketSink        IHXServerPacketSink;
typedef _INTERFACE IHXQoSCongestionEquation   IHXQoSCongestionEquation;
typedef _INTERFACE IHXQoSTransportAdaptationInfo IHXQoSTransportAdaptationInfo;

/* Rate Shaping Defaults: */
#define QOS_CC_DEFAULT_SCALAR           5       /* decimal percentage */
#define QOS_CC_DEFAULT_MAX        10000000       /* bits per second */
#define QOS_CC_DEFAULT_START_RATE    4096       /* bytes per second */
#define QOS_CC_DEFAULT_PKT_SIZE       300       /* bytes */
#define QOS_CC_DEFAULT_BURST_LIMT       3       /* pkts */
#define QOS_CC_MAX_PKT_SZ            1400       /* bytes */
#define AVG_PKT_SZ_FILTER               3
#define QOS_CC_OUT_RATE_FILTER          0.4

/* Timeout Defaults: */
#define QOS_CC_DEFAULT_TIMEOUT      15000       /* msec */
#define QOS_CC_DEFAULT_INTVL_CNT        8       /* # of intervals */
#define QOS_CC_DEFAULT_MAX_TIMEO        2       /* # of timeouts */
#define QOS_CC_DEFAULT_MIN_TIMEO_INTVL 10       /* msec */

class CongestionFeedbackTimeout;

class QoSCongestionCtl : public IHXQoSCongestionControl,
			 public IHXQoSSignalSink,
			 public IHXQoSSignalSourceResponse,
			 public IHXQoSRateShaper,
			 public IHXServerPacketSink,
			 public IHXServerPacketSource,
			 public IHXCallback,
			 public IHXQoSLinkCharSetup
{
 public:
    QoSCongestionCtl (IUnknown* pContext);
    ~QoSCongestionCtl ();
    
    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    
    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXServerPacketSink */
    STDMETHOD(SetSource) (THIS_ IHXServerPacketSource* pSource);
    STDMETHOD(PacketReady) (THIS_ ServerPacket* pPacket);
    STDMETHOD(Flush) (THIS);
    STDMETHOD(SourceDone) (THIS);

    /* IHXServerPacketSource */
    STDMETHOD(SetSink)(THIS_ IHXServerPacketSink* pSink);
    STDMETHOD(StartPackets) (THIS);
    STDMETHOD(GetPacket) (THIS);
    STDMETHOD(SinkBlockCleared)(THIS_ UINT32 ulStream);
    STDMETHOD(EnableTCPMode) (THIS);

    /* IHXQoSCongestionControl */
    STDMETHOD (Init)   (THIS_ IHXBuffer* pSessionId, 
			UINT16             unStrmNumber,
			UINT32 /* bytes */ ulMediaRate,  
			UINT32 /* bytes */ ulPacketSize);

    /* IHXQoSRateShaper */
    STDMETHOD (Init)             (THIS_ IHXQoSRateShapeAggregator* pAggregator);
    STDMETHOD (StreamCleared)    (THIS_ UINT16 nStreamNumber);
    STDMETHOD (GetStreamNumber)  (THIS_ REF(UINT16) /*OUT*/ nStreamNumber);
    STDMETHOD (GetMaxTokens)     (THIS_ REF(UINT32) /*OUT*/ ulMaxTokens);

    /* IHXQoSSignalSink */
    STDMETHOD (Signal)   (THIS_ IHXQoSSignal* pSignal, IHXBuffer* pSessionId);
    STDMETHOD (ChannelClosed) (THIS_ IHXBuffer* pSessionId);

    /* IHXQoSSignalSourceResponse */
    STDMETHOD (SignalBusReady)(THIS_ HX_RESULT hResult, 
			       IHXQoSSignalBus* pBus, 
			       IHXBuffer* pSessionId);

    /* IHXCallback method */
    STDMETHOD(Func)			(THIS);

    /* IHXQoSLinkCharSetup method */
    STDMETHOD (SetLinkCharParams) (THIS_
                                 LinkCharParams* /* IN */ pLinkCharParams);
    STDMETHOD (GetLinkCharParams) (THIS_
                                 UINT16 unStreamNum,
                                 REF(LinkCharParams) /* OUT */ linkCharParams);

 private:
    /* QoSCongestionCtl Methods: */
    HX_RESULT                     HandleRTCPFeedback(IHXBuffer* pRTCPReceiverReport);
    void                          FeedbackTimeout(BOOL bDisableCongestionCtl);
    HX_RESULT                     ConfigureTCPMode ();
    void                          UpdateMaxRate();
    UINT32                        AdjustRateForOverhead(UINT32 ulRate);
    UINT32                        TransmitInterval(UINT32 ulTokens, UINT32 ulRate,
						   UINT32 ulPacketSize);

    void                          PrintMaxSendRateDebugInfo();
    void                          PrintLinkCharDebugInfo(LinkCharParams* pParams);

    IUnknown*                     m_pContext;
    IHXQoSSignalBus*              m_pSignalBus;
    IHXQoSSignal*                 m_pThroughputSignal;
    IHXQoSSignal*                 m_pRTTSignal;
    IHXThreadSafeScheduler*       m_pTSScheduler;
    IHXScheduler*                 m_pScheduler;
    IHXQoSCongestionEquation*     m_pCongestionEqn;
    IHXServerPacketSource*        m_pSource;
    IHXServerPacketSink*          m_pSink;
    IHXBuffer*                    m_pSessionId;
    UINT16                        m_unStreamNumber;
    IHXQoSTransportAdaptationInfo* m_pTransportInfo;
    BOOL                          m_bShowMaxSendRateDebugInfo;

    CongestionFeedbackTimeout*    m_pTimeout;

    /* IUnknown */
    LONG32                        m_lRefCount;

    /* Rate Shaping */
    CallbackHandle                m_ulCallbackHandle;
    BOOL                          m_bCallbacksStarted;
    UINT32                        m_ulTokens;           /* bytes   */
    UINT32                        m_ulMaxBurst;         /* packets */
    UINT32                        m_ulFeedbackTimeout;  /* msec */
    BOOL                          m_bPassThrough;       /* boolean */
    UINT32                        m_ulStartRate;        /* bytes per second */
    UINT32                        m_ulAvgPacketSize;     /* bytes   */
    UINT32                        m_ulMediaRate;        /* bytes   */
    UINT32                        m_ulTotalRate;    /* percentage */
    UINT32                        m_ulChannelRate;      /* bps   */
    UINT32                        m_ulMaxRate;          /* bps */
    UINT32                        m_ulSDBRate;          /* bps */
    UINT32                        m_ulTotalBandwidth;   // bps - "Bandwidth" header value 
                                                        //(aggregate across all streams).
    BOOL                          m_bUseSDB;             /* boolean */
    double                        m_fMaxOversendRate;     /* % */
    BOOL                          m_bBlocked;
    double                        m_fRRIntvl;
    UINT16                        m_nProtocolOverhead;
    IHXQoSRateShapeAggregator*    m_pAggregator;

    /* TCP Mode */
    BOOL                          m_bConfigureTCPMode;
    BOOL                          m_bTCP;
    
    /* RTCP Metrics Analysis */
    BOOL                          m_bFirstRR;
    UINT16                        m_unLastSeq;
    UINT16                        m_unCycleCnt;
    UINT32                        m_ulLastLoss;
    UINT32                        m_ulNumLost;
    UINT32                        m_ulLastRecvd;
    UINT32                        m_ulTotalAckd;

    float                         m_fRTT;
    float                         m_fLastLoss;
    double                        m_fRecvdRate;

    Timeval                       m_tLastRR;

    /* Outbound Rate */
    double                        m_fAvgOutboundRate;
    UINT32                        m_ulBitAccumulator;
    Timeval                       m_tRateTimer;

    LinkCharParams*               m_pLinkCharParams;

    /* Debug: */
    UINT32                        m_ulSessionStartTime;

    friend class CongestionFeedbackTimeout;
};

class CongestionFeedbackTimeout : public IHXCallback
{
 public:
    CongestionFeedbackTimeout(QoSCongestionCtl* pOwner, IHXQoSProfileConfigurator* pConfig);
    ~CongestionFeedbackTimeout();

    void OnFeedback();
    void Stop();

    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    /* IHXCallback method */
    STDMETHOD(Func)			(THIS);

 private:
    LONG32             m_lRefCount;
    CallbackHandle     m_ulCallbackHandle;

    IHXThreadSafeScheduler* m_pSched;
    QoSCongestionCtl*       m_pOwner;
    
    UINT32             m_ulInitialFeedback;
    
    /* Feedback Interval Timing: */
    UINT32             m_ulLastFeedback;
    UINT32             m_ulTimeoutEventCount;
    UINT32             m_ulInitialTimeout;
    UINT32             m_ulLastTimeout;
    UINT32             m_ulFeedbackInterval;
    UINT32             m_ulMinTimeoutInterval;
    
    /* Configurable Paramters: */
    UINT32             m_ulTimeoutIntervalCount;
    UINT32             m_ulMaxTimeouts;
};

#endif /*_QOS_TRAN_RTCP_CC_H_ */
