/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pcktstrm.h,v 1.20 2007/04/17 03:13:22 darrick Exp $
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


#ifndef _PCKTSTRM_H_
#define _PCKTSTRM_H_

#include "rvdrop.h"
#include "pcktflowwrap.h"
#include "bandcalc.h"
#include "tsconvrt.h"
#include "hxstreamadapt.h"

#define MAX_METER_QUEUE 1024
#define INVALID_RULE_NUM 0xFFFF

class PacketStreamDoneCallback;
class BasicPacketFlow;
class PacketFlowTimeStampCallback;
class PacketFlowManager;
class PullPacketFlow;
class Transport;
class ASMRuleBook;
class BWCalculator;
class ServerPacket;
_INTERFACE IHXServerPacketSink;
_INTERFACE IHXServerPauseAdvise;

typedef enum 
{
    NONE,
    SYNC,
    ACTION_OFF,
    ACTION_ON
} ActionState;


class PacketStream
{
public:

    void CommitPendingBandwidth();
    void CreateMeterCallback();
    void CreateTSCallback(PullPacketFlow* pFlow, UINT16 unStream);
    void HandleLiveResume();
    BOOL Activate(BOOL bReset,
                  BOOL bInitialSubscriptionDone,
                  BOOL bIsMulticast,
                  REF(UINT32) ulDeliveryRate,
                  PacketFlowManager* pFlowMgr);

    void ChangeDeliveryBandwidth(INT32 lChange);
    HX_RESULT Register(Transport* pTransport,
                       ASMRuleBook* pRuleBook,
                       IHXValues* pHeader,
                       UINT16 uStreamNumber,
                       BOOL bIsLive,
                       UINT16 uStreamGroupNumber = 0xFFFF);

    BOOL HandleSeekSubscribes(BOOL bInitialSubscriptionDone,
                              BOOL bIsMulticast,
                              REF(UINT32) ulDeliveryRate,
                              PacketFlowManager* pFlowMgr);

    INT32 SubscribeRule(INT32 lRule,
                        BOOL bInitialSubscriptionDone,
                        BOOL bIsMulticast,
                        PacketFlowManager* pFlowMgr);

	HX_RESULT SetStreamAdaptation (StreamAdaptationSchemeEnum enumAdaptScheme,
					StreamAdaptationParams* pStreamAdaptParams);
    class RuleInfo
    {
    public:
	RuleInfo();
	~RuleInfo();

	UINT32		    m_ulPriority;
	UINT32		    m_ulAvgBitRate;
	BOOL		    m_bBitRateReported;
	BOOL		    m_bTimeStampDelivery;
	BWCalculator*	    m_pBWCalculator;
	AvgBandwidthCalc    m_BitRate;
	BOOL		    m_bWaitForSwitchOffFlag;
	ActionState	    m_PendingAction;
	BOOL		    m_bRuleOn;
	BOOL		    m_bActivateOnSeek;
	BOOL		    m_bSyncOk;
	UINT16*		    m_pOnDepends;
	UINT16*		    m_pOffDepends;
    };

    class Packets
    {
    public:
	Packets();
	~Packets();
	void	    	Init(UINT32 ulSize);
	ServerPacket*	GetPacket();
	ServerPacket*	PeekPacket(); //This function violates COM Reference Rules
	HX_RESULT	PutPacket(ServerPacket*);
	void	    	Clear();
	BOOL	    	IsEmpty();
    private:
	void		Inc(UINT32& ulNumber)
			{
			    if (++ulNumber == 8)
				ulNumber = 0;
			};
	ServerPacket*	    m_pPacketRing[8];
	UINT32		    m_ulPacketRingWriterPos;
	UINT32		    m_ulPacketRingReaderPos;
    };

    class MeterCallback : public BaseCallback
    {
    public:
      STDMETHOD(Func)	(THIS);
      PacketStream*    m_pSD;
    };
    friend class MeterCallback;

    PacketStream();
    ~PacketStream();

    void SetFlow(BasicPacketFlow* pFlow);
    BOOL IsDependOk(BOOL bOn, UINT16 unRule);
    BOOL IsStreamDone();
    void Reset();
    void MeterCallbackFunc();

    void SetFirstPacketTS(UINT32 ulTimeStamp);
    BOOL IsFirstPacketTSSet();

    UINT16		m_unSequenceNumber;
    UINT16		m_unReliableSeqNo;
    Packets		m_pPackets;
    UINT32		m_ulAvgBitRate;
    UINT32		m_ulVBRAvgBitRate;
    UINT32              m_ulMaxBitRate;
    UINT32              m_ulMinBitRate;
    UINT32              m_ulAvgPktSz;
    UINT32              m_ulPreroll;
    IHXBuffer*          m_bufMimeType;
    RuleInfo*		m_pRules;
    INT32		m_lNumRules;
    INT32		m_lBytesDueTimes10;
    Transport*  	m_pTransport;
    IHXServerPauseAdvise* m_pPauseAdvise;
    BOOL		m_bSupportsPacketAggregation;
    BOOL		m_bNullSetup;
    PacketFlowTimeStampCallback* m_pTimeStampCallback;
    PacketStreamDoneCallback* m_pStreamDoneCallback;
    Timeval		m_tLastScheduledTime;
    UINT32		m_uTimeStampScheduledSendID;
    UINT32		m_ulLastTSDTS;
    UINT32		m_ulTSDMark;
    UINT32		m_ulLastScaledPacketTime;
    UINT32		m_uScheduledStreamDoneID;
    BasicPacketFlow* 	m_pFlow;
    RVDrop*		m_pRVDrop;
    UINT32		m_ulPacketsOutstanding;
    BOOL		m_bPacketRequested;
    BOOL		m_bStreamDonePending;
    BOOL		m_bStreamDone;
    BOOL		m_bSentStreamDone;
    INT32		m_ulRatio;
    BOOL		m_bWouldBlocking;
    BOOL		m_bGotSubscribe;

    HX_QOS_SIGNAL	m_mediaRateSignal;

    //This is used for live timestampdelivery packets only:
    UINT32		m_ulEncoderTimeMinusPlayerTimeOffset;
    BOOL                m_bSetEncoderOffset;

    //Packet metering queue management (for live sources)
    ServerPacket*       m_pMeterQueue[MAX_METER_QUEUE];
    UINT16		m_unMeterQueueHead;
    UINT16              m_unMeterQueueTail;
    float               m_fPktSizeEst;
    MeterCallback*      m_pMeterCallback;
    UINT32              m_ulMeterCallbackID;

    //Used for range headers in play responses
    BOOL                m_bFirstPacketTSSet;
    UINT32              m_ulFirstPacketTS;
    CHXTimestampConverter*  m_pTSConverter;

    UINT16              m_unStreamNumber;
    UINT16              m_uStreamGroupNumber;
    UINT16              m_unDefaultRuleNum;

    IHXServerPacketSink* m_pSink;

    //Stream Adaptation Parameters
    StreamAdaptationSchemeEnum	m_enumStreamAdaptScheme;
    StreamAdaptationParams*	m_pStreamAdaptParams;

    //Link Characteristics
    LinkCharParams*	m_pLinkCharParams;
    UINT32              m_ulInitialRate;
    
    //XXXVS: Used to identify Registered Streams
    BOOL                m_bStreamRegistered;
};

#endif // _PCKTSTRM_H_

