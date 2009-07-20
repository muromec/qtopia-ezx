/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: strmdata.h,v 1.16 2009/04/21 18:38:52 ckarusala Exp $ 
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
/*  Helper class for use by the Player Packet Manager
 */

#ifndef _STRMDATA_H_
#define _STRMDATA_H_

#include "rvdrop.h"
#include "ppm.h"
#include "bandcalc.h"
#include "tsconvrt.h"

#define MAX_METER_QUEUE 1024
#define HX_INVALID_STREAM 0xFFFF

_INTERFACE IHXServerPauseAdvise;

class StreamDoneCallback;

typedef enum 
{
    NONE,
    SYNC,
    ACTION_OFF,
    ACTION_ON
} ActionState;

class PPMStreamData
{
public:

    class RuleInfo
    {
    public:
	RuleInfo();
	~RuleInfo();

	UINT32		    m_ulPriority;
	UINT32		    m_ulAvgBitRate;
        UINT32              m_ulMaxBitRate;
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
	INT16		    m_lInterDepends;
    };

    class Packets
    {
    public:
	Packets();
	~Packets();
	void	    	Init(UINT32 ulSize);
	ServerPacket*	GetPacket();
	ServerPacket*	PeekPacket(); /* This function violates COM Reference Rules */
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
      PPMStreamData*    m_pSD;
    };
    friend class MeterCallback;

    PPMStreamData();
    ~PPMStreamData();

    void SetSession(PPM::Session* pSession);
    BOOL IsDependOk(BOOL bOn, UINT16 unRule);
    HXBOOL IsInterDependOk(UINT16 unRule);
    BOOL IsStreamDone();
    void Reset();
    void MeterCallbackFunc();

    void SetFirstPacketTS(UINT32 ulTimeStamp);
    BOOL IsFirstPacketTSSet();

    HX_RESULT HandleSequence(UINT32 ulInSeqNo,
                             UINT16 uStreamNo,
                             UINT16 uRuleNo,
                             UINT16* pRuleSequenceArray,
                             UINT16& unOutSeqNo,
                             BOOL bSubscribed);

    HX_RESULT ClearSequenceHistory();

    HX_RESULT SetStreamAdaptation(StreamAdaptationSchemeEnum enumAdaptScheme,
							StreamAdaptationParams* pStreamAdaptParams);

    void SuspendTimeline(Timeval& tvTimelineStart);

    UINT16              m_unSequenceNumber;
    UINT16              m_unStartingSeqNum;

    struct SequenceHistory {
        UINT32  m_ulRefCount;
        UINT16  m_unOutSeqNo;
        UINT16* m_pRuleSeqNoArray;
    };

    HX_deque*           m_pHistoryQueue;
    UINT32              m_ulFirstQSeqNo;
    UINT32              m_ulExpectedInSeqNo;
    UINT16*             m_pExpectedRuleSeqNoArray;
    UINT16              m_unOutSeqNo;
    UINT16              m_unLastLateOutSeqNo;
    UINT16              m_uOutofRange;
    UINT16              m_unMaxHistoryQSize;
    BOOL                m_bFirstPacket;
    UINT16		m_unReliableSeqNo;
    Packets		m_pPackets;
    UINT32		m_ulAvgBitRate;
    UINT32		m_ulVBRAvgBitRate;
    UINT32              m_ulMaxBitRate;
    UINT32              m_ulMinBitRate;
    RuleInfo*		m_pRules;
    INT32		m_lNumRules;
    INT32		m_lBytesDueTimes10;
    Transport*  	m_pTransport;
    IHXServerPauseAdvise* m_pPauseAdvise;
    BOOL		m_bSupportsPacketAggregation;
    BOOL		m_bNullSetup;
    PPM::Session::TimeStampCallback* m_pTimeStampCallback;
    StreamDoneCallback*	m_pStreamDoneCallback;
    Timeval		m_tLastScheduledTime;
    UINT32		m_uTimeStampScheduledSendID;
    UINT32		m_ulLastTSDTS;
    UINT32		m_ulTSDMark;
    UINT32		m_ulLastScaledPacketTime;
    UINT32		m_uScheduledStreamDoneID;
    PPM::Session*	m_pSession;
    RVDrop*		m_pRVDrop;
    UINT32		m_ulPacketsOutstanding;
    BOOL		m_bPacketRequested;
    BOOL		m_bStreamDonePending;
    BOOL		m_bStreamDone;
    BOOL		m_bSentStreamDone;
    INT32		m_lStreamRatioTemp;
    BOOL		m_bWouldBlocking;
    BOOL		m_bGotSubscribe;
    BOOL		m_bStreamRegistered;
    HXBOOL              m_bSwitchGroupRegistered;
    UINT32              m_ulSwitchGroupID;
    UINT16              m_unRegisteredStream;
    HXPacketType        m_nPacketType;

    //This is used for live timestampdelivery packets only:
    UINT32		m_ulEncoderTimeMinusPlayerTimeOffset;
    BOOL                m_bSetEncoderOffset;

    //Used for range headers in play responses
    HXBOOL              m_bFirstPacketTSSet;
    UINT32              m_ulFirstPacketTS;
    
    HXBOOL              m_bSyncReady;
    UINT32              m_ulFirstMediaTS;
    UINT32              m_ulFirstRTPTS;
    UINT32              m_ulFirstDeliveryTime;

    CHXTimestampConverter* m_pTSConverter;
    UINT32              m_ulPreroll;

    //Link Characteristics
    LinkCharParams*	m_pLinkCharParams;

    //Stream Adaptation Parameters
    StreamAdaptationSchemeEnum	m_enumStreamAdaptScheme;
    StreamAdaptationParams*	m_pStreamAdaptParams;

    friend class PPM::Session;
};

#endif

