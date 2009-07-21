/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: transprt.h,v 1.13 2006/12/21 05:06:58 tknox Exp $ 
 *   
 * Portions Copyright (c) 1995-2007 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _TRANSPRT_H_
#define _TRANSPRT_H_

#include "hxcom.h"
#include "hxcomm.h"
#include "hxstats.h"

//#define RDT_MESSAGE_DEBUG

#include "hxengin.h"
#include "hxmon.h"
#include "statinfo.h"
#include "hxsmbw.h"
#include "protdefs.h"
#include "hxabdutil.h"

// GCC won't let me forward declare CHXMapLongToObj::Iterator,
// so I have to include this. -JR
#include "hxmap.h"

#include "packfilt.h"

#include "rtspif.h"
#include "rtspmsg.h"
#include "rtsptran.h"
#include "ihxtrans.h"

#define RULE_TABLE_WIDTH  256
#define RULE_TABLE_HEIGHT 64

class BasePacket;
class TransportStreamHandler;
class RTSPStreamData;

struct IHXScheduler;
struct IHXSocket;
struct IHXPacket;
struct IHXBuffer;
struct IHXCommonClassFactory;
struct IHXInternalReset;
struct IHXPlayerState;
struct IHXAccurateClock;
struct IHXSourceBufferingStats2;
struct IHXSourceLatencyStats;
class  RTSPResendBuffer;
class  CHXBitset;
class  HX_deque;
class  Timeval;
class  CHXTimestampConverter;
class  TNGLatencyReportPacket;
struct IHXSessionStats;

class TNGDataPacket;
class TNGReportPacket;
class TNGACKPacket;
class TNGRTTRequestPacket;
class TNGRTTResponsePacket;
class TNGCongestionPacket;
class TNGStreamEndPacket;

class RTCPBaseTransport;
class RTCPUDPTransport;
class RTCPTCPTransport;
class RTCPPacket;
class ReportHandler;

class RTSPResendBuffer;
class STREAM_STATS;
class Transport;
class IHXRTSPTransportResponse;

class Transport : public IHXTransport
{
public:

    Transport();
    ~Transport();
    /*
     *	IUnknown methods
     */

    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXStatistics methods
     */

    STDMETHOD (InitializeStatistics)	(THIS_
					UINT32	/*IN*/ ulRegistryID) PURE;

    STDMETHOD (UpdateStatistics)	(THIS) PURE;

    STDMETHOD_(BOOL,isNullSetup)        (THIS) { return FALSE; }

    STDMETHOD_(BOOL,isRTSPMulticast)    (THIS) { return FALSE; }

    STDMETHOD_(BOOL,isRTP)		(THIS) { return FALSE; }

    STDMETHOD_(BOOL,isReflector)	(THIS) { return FALSE; }

    STDMETHOD (getRTCPRule)	        (THIS_ REF(UINT16) unRTCPRule) { return HXR_FAIL; }

    void SetSessionStatsObj(IHXSessionStats* pSessionStats) { HX_RELEASE(m_pSessionStats); m_pSessionStats = pSessionStats; m_pSessionStats->AddRef(); }

    // XXXGo
    virtual HX_RESULT              SubscriptionDone(BYTE* bRuleOn,
						    REF(UINT32)ulSourcePort, 
						    REF(UINT32)ulPort,
						    REF(UINT32)ulAddr,
						    REF(TransportStreamHandler*)pHandler)
					{ return 0; }

    virtual void		sendPacket(BasePacket* packet) = 0;
    virtual HX_RESULT           sendPackets(BasePacket** pPacket)
                                    { return HXR_NOTIMPL; }

    virtual void		streamDone(UINT16 uStreamNumber,
                                   UINT32 uReasonCode = 0,
                                   const char* pReasonText = NULL) = 0;
    virtual void		setSequenceNumber(UINT16 uStreamID,
				                  UINT16 uSequenceNumber) = 0;
    virtual HXBOOL		IsInitialized();
    virtual HXBOOL		IsUpdated();
    virtual UINT32		wrapSequenceNumber();
    virtual void		setLatencyPeriod(UINT32 ulMs) {};
    virtual void                SetMediaAttributes(UINT32 ulRate, UINT32 ulPktSz) {};
    virtual HXBOOL		SupportsPacketAggregation() { return FALSE; }
    virtual void		setTimeStamp(UINT16 uStreamNumber, UINT32 ulTS,
                                             BOOL bIsRaw = FALSE) {}

    STDMETHOD(SetDeliveryBandwidth)(UINT32 ulBackOff, UINT32 ulBandwidth) { return HXR_OK; };

    virtual const char*         getMimeType() {return NULL;}

    IHXSessionStats*            m_pSessionStats;

    // From RTSPTransport
public:
    //virtual void Done                   (void) = 0;
    //virtual void Reset                  (void) = 0;
    //virtual void Restart                (void) = 0;
    //virtual RTSPTransportTypeEnum tag   (void) = 0;
    virtual void Done                   (void) {}       // Need for now to compile
    virtual void Reset                  (void) {}       // Need for now to compile
    virtual void Restart                (void) {}       // Need for now to compile
    virtual RTSPTransportTypeEnum tag   (void) { return RTSP_TR_NONE; }       // Need for now to compile
    virtual void addStreamInfo          (RTSPStreamInfo* pStreamInfo,
                                         UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);
    virtual void setSessionID           (const char* pSessionID);
    virtual HX_RESULT sendToResendBuffer(BasePacket* pPacket) { return HXR_OK; }
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer) { return HXR_NOTIMPL; } // Need for now to compile
    
    virtual HX_RESULT handleMasterSync  (ULONG32 ulHXTime, LONG32 lHXOffsetToMaster)
                                        { return HXR_OK; }
    virtual HX_RESULT anchorSync        (ULONG32 ulHXTime, ULONG32 ulNTPTime)
                                        { return HXR_OK; }
    virtual HX_RESULT releasePackets    (void) { return HXR_OK; }
    virtual HX_RESULT sendNAKPacket     (UINT16 uStreamNumber,
                                         UINT16 uBeingSeqNo,
                                         UINT16 uEndSeqNo)
                                        { return HXR_OK; }

    // BCM
    virtual TransportStreamHandler* GetStreamHandler(void) { HX_ASSERT(FALSE); return NULL; }
    virtual void SetStreamHandler       (TransportStreamHandler* pHandler) {}
    virtual void MulticastSetup         (TransportStreamHandler* pHandler) {}
    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSocket = 0) {}
    virtual HXBOOL isBCM(void)            { return FALSE; }
    // end BCM

    UINT16  getSeqNum                   (UINT16 streamNumber);
    UINT16  getFirstSeqNum            (UINT16 streamNumber);
    UINT32  getTimestamp                (UINT16 streamNumber);
//    HX_RESULT setMarkerRule           (UINT16 ruleNumber);
    virtual HX_RESULT setFirstSeqNum    (UINT16 streamNumber,
                                         UINT16 seqNum);
    UINT32  getFirstTimestamp         (UINT16 streamNumber);
    virtual void      setFirstTimeStamp (UINT16 uStreamNumber, UINT32 ulTS,
                                         HXBOOL bIsRaw = FALSE);

    // only in RTPTransport...
    virtual void notifyRTPInfoProcessed (void) {}
    virtual void  setPlayRange          (UINT32 ulFrom, UINT32 ulTo);
    virtual HX_RESULT setFirstPlayTime  (Timeval* pTv) {return HXR_OK;};
    virtual void      OnPause  (Timeval* pTv) { };
    virtual void setLegacyRTPLive(void){};

    HX_RESULT resetFlags                (UINT16 streamNumber);
    HX_RESULT getPacket                 (UINT16 uStreamNumber,
                                         IHXPacket*& pPacket);
    HX_RESULT getPacket                 (UINT16 uStreamNumber,
                                         IHXPacket*& pPacket,
                                         REF(UINT32) uSeqNum);
    virtual HX_RESULT startPackets      (UINT16 uStreamNumber);
    virtual HX_RESULT stopPackets       (UINT16 uStreamNumber);

    HX_RESULT storePacket               (IHXPacket* pPacket,
                                         UINT16 uStreamNumber,
                                         UINT16 uSeqNo,
                                         UINT16 uReliableSeqNo,
                                         HXBOOL isReliable);
    HX_RESULT packetReady               (HX_RESULT status,
                                         RTSPStreamData* pStreamData,
                                         IHXPacket* pPacket);
    HX_RESULT packetReady               (HX_RESULT status,
                                         UINT16 uStreamNumber,
                                         IHXPacket* pPacket);
    RTSPResendBuffer* getResendBuffer(UINT16 streamNumber);
    HX_RESULT getStatus                 (UINT16& uStatusCode,
                                         IHXBuffer*& pStatusDesc,
                                         UINT16& ulPercentDone);
    HX_RESULT   GetCurrentBuffering(UINT16  uStreamNumber,
                                    INT64&  llLowestTimestamp,
                                    INT64&  llHighestTimestamp,
                                    UINT32& ulNumBytes,
                                    HXBOOL&   bDone);

    HX_RESULT   SeekFlush               (UINT16 uStreamNumber);

    HXBOOL        IsDataReceived(void)        {return m_bIsReceivedData;};

    HXBOOL        IsSourceDone(void);
    HX_RESULT initializeStatistics      (UINT32 ulRegistryID);
    HX_RESULT SetStatistics             (UINT16 uStreamNumber, STREAM_STATS* pStats);
    HX_RESULT updateStatistics          (HXBOOL bUseRegistry = TRUE);
    HX_RESULT UpdateRegistry            (UINT32 ulStreamNumber,
                                         UINT32 ulRegistryID);
    INT64  getBytesSent                 (void) {return m_lBytesSent;}
    UINT32 getPacketsSent               (void) {return m_ulPacketsSent;}
    HX_RESULT playReset                 (void);
    virtual HX_RESULT pauseBuffers      (void);
    virtual HX_RESULT resumeBuffers     (void);
    HX_RESULT Init                      (IUnknown* pContext);
    HX_RESULT SetResendBufferDepth      (UINT32 uMilliseconds);

    void CheckForSourceDone             (UINT16 uStreamNumber);
    void HandleBufferError              (void);

    IHXScheduler*  GetScheduler(void) {return m_pScheduler;};

     /*XXXMC
      *Special-case handling for PV clients
      */
     virtual HX_RESULT sendPVHandshakeResponse(UINT8* pPktPayload)
                                              { return HXR_NOTIMPL; }

    virtual HX_RESULT sendBWProbingPackets     (UINT32 ulCount, UINT32 ulSize, REF(INT32) lSeqNo) {return HXR_NOTIMPL;}

    //XXXGH...Fix this stuff left over from Ogre
    HXBOOL                                m_bHackedRecordFlag;

    IHXPlayerState*                     m_pPlayerState;

    HXBOOL isSparseStream(UINT16 uStreamNumber);

    void GetContext(IUnknown*& pContext);
    void EnterPrefetch(void) { m_bPrefetch = TRUE; };
    void LeavePrefetch(void);

    void EnterFastStart(void);
    void LeaveFastStart(void);

    inline HXBOOL HasPlayRequestBeenSent(void) { return m_bPlayRequestSent; }

    inline void SetPlayRequestSent(HXBOOL bValue) { m_bPlayRequestSent = bValue; }

    UINT32 GetPacketsSent(void) { return m_ulPacketsSent; }
    INT64 GetBytesSent(void) { return m_lBytesSent; }
    UINT32 GetTotalSuccessfulResends(void) { return m_ulTotalSuccessfulResends; }
    UINT32 GetTotalFailedResends(void) { return m_ulTotalFailedResends; }
    UINT32 GetSendingTime(void) { return m_ulSendingTime; }

    HX_RESULT SetProbingPacketsRequested(UINT8 nPacketsRequested);

protected:
    void destroyABDPktInfo();
    HX_RESULT handleABDPktInfo(TransportMode mode, UINT32 uSeq, 
                               UINT32 uSendTime, UINT32 uRecvTime, 
                               UINT32 uPktSize);

    IUnknown*                           m_pContext;
    IHXCommonClassFactory*              m_pCommonClassFactory;
    IHXScheduler*                       m_pScheduler;
    IHXRTSPTransportResponse*           m_pResp;
    IHXRegistry*                        m_pRegistry;
    IHXInternalReset*                   m_pInternalReset;
    IHXSourceBufferingStats2*           m_pSrcBufferStats;
    IHXSourceLatencyStats*              m_pSourceLatencyStats;
    CHXString                           m_sessionID;
    TransportStreamHandler*                  m_pStreamHandler;
    HXBOOL                              m_bIsSource;
    UINT32                              m_ulRegistryID;
    UINT32                              m_ulPacketsSent;
    INT64                               m_lBytesSent;
    UINT32                              m_ulStartTime;
    UINT32                              m_ulPlayRangeFrom;
    UINT32                              m_ulPlayRangeTo;
    HXBOOL                              m_bIsInitialized;
    HXBOOL                              m_bIsUpdated;

    HXBOOL                              m_bPrefetch;
    HXBOOL                              m_bFastStart;
    HXBOOL                              m_bIsReceivedData;
    HXBOOL                              m_bSourceDone;
    UINT32                              m_wrapSequenceNumber;
    // actual multicast sender
    HXBOOL                              m_bMulticast;

    RawPacketFilter*                    m_pPacketFilter;
    CHXSimpleList*                      m_pClientPacketList;
    CHXSimpleList*                      m_pNoTransBufPktList;
    HXBOOL                              m_bPlayRequestSent;

    UINT32                              m_ulTotalSuccessfulResends;
    UINT32                              m_ulTotalFailedResends;
    UINT32                              m_ulSendingTime;

    HXBOOL                              m_drop_packets;
    UINT32                              m_packets_since_last_drop;

    HXBOOL                              m_bSkipTimeAdjustment;

    UINT8                               m_nProbingPacketsRequested;
    UINT8                               m_nProbingPacketsReceived;
    ABD_PROBPKT_INFO**                  m_pABDProbPktInfo;

#ifdef RDT_MESSAGE_DEBUG
    HXBOOL      m_bRDTMessageDebug;
    CHXString m_RDTmessageDebugFileName;
    void RDTmessageFormatDebugFileOut(const char* fmt, ...);
#endif // RDT_MESSAGE_DEBUG
};

inline HXBOOL
Transport::IsInitialized(void)
{
    return m_bIsInitialized;
}

inline HXBOOL
Transport::IsUpdated(void)
{
    return m_bIsUpdated;
}

inline UINT32
Transport::wrapSequenceNumber(void)
{
    return m_wrapSequenceNumber;
}


#endif /* _TRANSPRT_H_ */