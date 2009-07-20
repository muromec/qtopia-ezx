/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtsptran.h,v 1.44 2007/01/11 21:19:42 milko Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef _RTSPTRAN_H_
#define _RTSPTRAN_H_

//#define RDT_MESSAGE_DEBUG

#include "hxengin.h"
#include "hxmon.h"
#include "transbuf.h"
#include "statinfo.h"
#include "hxsmbw.h"
#include "protdefs.h"
#include "hxabdutil.h"

// GCC won't let me forward declare CHXMapLongToObj::Iterator,
// so I have to include this. -JR
#include "hxmap.h"

#include "packfilt.h"

struct IHXScheduler;
struct IHXUDPSocket;
struct IHXPacket;
struct IHXBuffer;
struct IHXCommonClassFactory;
struct IHXInternalReset;
struct IHXPlayerState;
struct IHXAccurateClock;
struct IHXSourceBufferingStats3;
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

const UINT32 TRANSPORT_BUF_DURATION           = 2000; // 2 seconds
const UINT32 TRANSPORT_BUF_DURATION_UNDEF     = 0xffffffff;
const UINT32 MAX_TRANSPORT_BUF_DURATION       = 15000;

/* Note: Both port numbers MUST be even */
const UINT16 MIN_UDP_PORT = 6970;
const UINT16 MAX_UDP_PORT = 32000;

const UINT32 MAX_UDP_PACKET = 4096;     // XXXBAB - check this
const UINT32 MAX_PACKET_SIZE = 1000;    // XXXBAB - check this
const UINT32 DEFAULT_WRAP_SEQ_NO = 0x10000;
const UINT32 TNG_WRAP_SEQ_NO = 0xff00;
const UINT32 TNG_MAX_SEQ_GAP = 0x00C8;
const UINT16 MAX_DEQUE_SIZE = 0x8000;
const UINT16 INITIAL_DEQUE_SIZE = 0x400;
const UINT16 PREFETCH_START_SIZE = 0x200;
const UINT16 PREFETCH_CHUNK_SIZE = 0x100;

static const UINT32 RESEND_BUF_DURATION     = 15000;   // 15 seconds
static const UINT32 MAX_RESEND_BUF_DURATION = 90000;
static const UINT32 RESEND_BUF_GROWTH_RATE  = 2000;

//XXXGLENN...needs to be a range of ports for firewalls
//const UINT16 SPLITTER_RESEND_PORT = 10002;


class RTSPStreamData
{
public:
    RTSPStreamData(HXBOOL needReliable);
    virtual ~RTSPStreamData(void);

    UINT16                      m_seqNo;
    UINT16                      m_firstSeqNo;
    UINT16                      m_reliableSeqNo;
    UINT16                      m_lastSeqNo;
    HXBOOL                        m_bNeedReliable;
    HXBOOL                        m_packetSent;
    UINT16                      m_streamNumber;
    UINT16                      m_streamGroupNumber;
    UINT32                      m_lastTimestamp;
    UINT32                      m_firstTimestamp;
    RTSPTransportBuffer*        m_pTransportBuffer;
    RTSPResendBuffer*           m_pResendBuffer;
    STREAM_STATS*               m_pStreamStats;
    HXBOOL                        m_bReceivedAllPackets;
    HXBOOL                        m_bNeedToACK;
    HXBOOL                        m_bFirstPacket;
    HXBOOL                        m_bUsesRTPPackets;
    RTSPMediaType               m_eMediaType;
    CHXTimestampConverter*      m_pTSConverter;

    /* The core must receive packet ordered in RMA time stamp.
     * This structure is initialized and used when the stream is
     * is instructed to do so.  In such case RMA time stamps
     * are forced to be ordered (out-of-order time stamp is forced
     * to the value of the last in-order time stamp).
     * The use of this structue must be accompanied with the use
     * of RTP packets which preserve the original packet time stamp
     * in RTPTime packet field.
     */
    struct TSOrderHackInfo
    {
        TSOrderHackInfo(void)
            : m_ulLastSentTS(0)
            , m_ulLastRecvTS(0)
        {
            ;
        }
        UINT32  m_ulLastSentTS;
        UINT32  m_ulLastRecvTS;
    };
    TSOrderHackInfo* m_pTSOrderHack;
};

class RTSPStreamHandler
{
public:
    RTSPStreamHandler(RTSPTransport* pOwner);
    ~RTSPStreamHandler(void);

    HX_RESULT         initStreamData(UINT16 streamNumber,
                                     UINT16 streamGroupNumber,
                                     HXBOOL needReliable,
                                     HXBOOL bIsSource,
                                     INT16 rtpPayloadType,
                                     HXBOOL bPushData,
                                     UINT32 wrapSequenceNumber,
                                     UINT32 ulBufferDepth,
                                     HXBOOL bHasOutOfOrderTS = FALSE,
                                     CHXTimestampConverter* pTSConverter = NULL,
                                     RTSPMediaType eMediaType = RTSPMEDIA_TYPE_UNKNOWN);
    HX_RESULT         initStreamData(UINT16 streamNumber,
                                    HXBOOL needReliable,
                                    HXBOOL bIsSource,
                                    INT16 rtpPayloadType,
                                    HXBOOL bPushData,
                                    UINT32 wrapSequenceNumber,
                                    UINT32 ulBufferDepth,
                                    HXBOOL bHasOutOfOrderTS = FALSE,
                                    CHXTimestampConverter* pTSConverter = NULL,
                                    RTSPMediaType eMediaType = RTSPMEDIA_TYPE_UNKNOWN);
    RTSPStreamData*   getStreamData(UINT16 streamNumber);
    RTSPStreamData*   getStreamDataForStreamGroup(UINT16 streamGroupNumber);

    //used to get StreamNumber corresponding to passed 
    //   StreamGroup
    UINT16            getStreamNumForStreamGroup(UINT16 streamGroupNumber)
		      {
			    HX_ASSERT(streamGroupNumber <= m_unMaxStreamGroup);
			    return m_pStreamGroupToStreamNumMap[streamGroupNumber];
		      }

    HX_RESULT         createResendBuffer(UINT16 streamNumber,
                                         UINT32 wrapSequenceNumber);
    RTSPResendBuffer* getResendBuffer(UINT16 streamNumber);
    RTSPStreamData*   firstStreamData(void);
    RTSPStreamData*   nextStreamData(void);
    HXBOOL              endStreamData(void);
    UINT16            streamCount(void) { return (UINT16)(m_pStreamDataMap ? m_pStreamDataMap->GetCount() : 0);};
    void              AddRef(void)  {   m_lRefCount++; }
    void              Release(void) {
                                    m_lRefCount--;
                                    if (m_lRefCount == 0)
                                        delete this;
                                }

private:
    INT32               m_lRefCount;
    RTSPTransport*      m_pOwner;
    CHXMapLongToObj*    m_pStreamDataMap;
    CHXMapLongToObj::Iterator streamIterator;

    UINT16		m_unMaxStreamGroup;
    UINT16*		m_pStreamGroupToStreamNumMap;
};

class RTSPTransport : public RawPacketFilter
{
public:
    RTSPTransport                       (HXBOOL bIsSource);
    virtual ~RTSPTransport              (void);

    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    virtual void Done                   (void) = 0;
    virtual void Reset                  (void) = 0;
    virtual void Restart                (void) = 0;
    virtual RTSPTransportTypeEnum tag   (void) = 0;
    virtual void addStreamInfo          (RTSPStreamInfo* pStreamInfo,
                                         UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);
    virtual void setSessionID           (const char* pSessionID);
    virtual HX_RESULT sendPacket        (BasePacket* pPacket) = 0;
    virtual HX_RESULT sendToResendBuffer(BasePacket* pPacket) { return HXR_OK; }
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer) = 0; 
    
    virtual HX_RESULT handleMasterSync  (ULONG32 ulHXTime, LONG32 lHXOffsetToMaster)
                                        { return HXR_OK; }
    virtual HX_RESULT anchorSync        (ULONG32 ulHXTime, ULONG32 ulNTPTime)
                                        { return HXR_OK; }
    virtual HX_RESULT releasePackets    (void) { return HXR_OK; }
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL) = 0;
    virtual HX_RESULT sendNAKPacket     (UINT16 uStreamNumber,
                                         UINT16 uBeingSeqNo,
                                         UINT16 uEndSeqNo)
                                        { return HXR_OK; }
    virtual HXBOOL isNullSetup(void)          { return FALSE; }
    virtual HXBOOL isRTP(void)                { return FALSE; }
    virtual HXBOOL isReflector(void)          { return FALSE; }

    // BCM
    virtual RTSPStreamHandler* GetStreamHandler(void) { HX_ASSERT(FALSE); return NULL; }
    virtual HX_RESULT getRTCPRule(REF(UINT16) unRTCPRule) { return HXR_FAIL; }
    virtual void SetStreamHandler       (RTSPStreamHandler* pHandler) {}
    virtual void MulticastSetup         (RTSPStreamHandler* pHandler) {}
    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSocket = 0) {}
    virtual HXBOOL isBCM(void)            { return FALSE; }
    // end BCM

    UINT16  getSeqNum                   (UINT16 streamNumber);
    UINT16  getFirstSeqNum            (UINT16 streamNumber);
    UINT32  getTimestamp                (UINT16 streamNumber);
//    HX_RESULT setMarkerRule           (UINT16 ruleNumber);
    virtual HX_RESULT setFirstSeqNum    (UINT16 streamNumber,
                                         UINT16 seqNum,
					 HXBOOL bOnPauseResume = FALSE);
    UINT32  getFirstTimestamp         (UINT16 streamNumber);
    virtual void      setFirstTimeStamp (UINT16 uStreamNumber, UINT32 ulTS,
                                         HXBOOL bIsRaw = FALSE,
					 HXBOOL bOnPauseResume = FALSE);

    // only in RTPTransport...
    virtual void notifyRTPInfoProcessed (HXBOOL bOnPauseResume = FALSE) {}
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
    RTSPTransportBuffer* getTransportBuffer(UINT16 uStreamNumber);
    RTSPResendBuffer* getResendBuffer(UINT16 streamNumber);
    HX_RESULT getStatus                 (UINT16& uStatusCode,
                                         IHXBuffer*& pStatusDesc,
                                         UINT16& ulPercentDone);
    HX_RESULT   GetCurrentBuffering(UINT16 uStreamNumber,
                                    UINT32& ulLowestTimestamp,
                                    UINT32& ulHighestTimestamp,
                                    UINT32& ulNumBytes,
                                    HXBOOL& bDone);

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
    HX_RESULT SetResendBufferParameters(UINT32 uMinimumDelay,             /* ms */
                                        UINT32 uMaximumDelay,             /* ms */
                                        UINT32 uExtraDelayDuringBuffering /* ms */);

    HXBOOL IsInitialized                  (void);
    HXBOOL IsUpdated                      (void);
    UINT32 wrapSequenceNumber           (void);

    void CheckForSourceDone             (UINT16 uStreamNumber);
    void HandleBufferError              (void);

    IHXScheduler*  GetScheduler(void) {return m_pScheduler;};

    virtual HXBOOL SupportsPacketAggregation(void) { return FALSE; }
    virtual HX_RESULT sendPackets       (BasePacket** pPacket) { return HXR_NOTIMPL; }

     /*XXXMC
      *Special-case handling for PV clients
      */
     virtual HX_RESULT sendPVHandshakeResponse(UINT8* pPktPayload)
                                              { return HXR_NOTIMPL; }

    virtual HX_RESULT sendBWProbingPackets     (UINT32 ulCount, UINT32 ulSize, REF(INT32) lSeqNo) {return HXR_NOTIMPL;}

    //XXXGH...Fix this stuff left over from Ogre
    HXBOOL                                m_bHackedRecordFlag;

    IHXPlayerState*                     m_pPlayerState;

    void FilterPacket(IHXPacket* pPacket);
    void SetFilterResponse(RawPacketFilter*);

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

private:
    HX_RESULT handleNoTransBufList(RTSPTransportBuffer* pTransportBuffer);
    HX_RESULT storeClientPacket(RTSPTransportBuffer* pTransportBuffer,
                                ClientPacket* pClientPkt);
    void destroyPktList(REF(CHXSimpleList*) pList);

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
    IHXSourceBufferingStats3*           m_pSrcBufferStats;
    IHXSourceLatencyStats*              m_pSourceLatencyStats;
    CHXString                           m_sessionID;
    RTSPStreamHandler*                  m_pStreamHandler;
    HXBOOL                                m_bIsSource;
    UINT32                              m_ulRegistryID;
    UINT32                              m_ulPacketsSent;
    INT64                               m_lBytesSent;
    UINT32                              m_ulStartTime;
    UINT32                              m_ulPlayRangeFrom;
    UINT32                              m_ulPlayRangeTo;
    HXBOOL                                m_bIsInitialized;
    HXBOOL                                m_bIsUpdated;

    HXBOOL                                m_bPrefetch;
    HXBOOL                                m_bFastStart;
    HXBOOL                                m_bIsReceivedData;
    HXBOOL                                m_bSourceDone;
    UINT32                              m_wrapSequenceNumber;
    // actual multicast sender
    HXBOOL                                m_bMulticast;

    RawPacketFilter*                    m_pPacketFilter;
    CHXSimpleList*                      m_pClientPacketList;
    CHXSimpleList*                      m_pNoTransBufPktList;
    HXBOOL                                m_bPlayRequestSent;

    UINT32                              m_ulTotalSuccessfulResends;
    UINT32                              m_ulTotalFailedResends;
    UINT32                              m_ulSendingTime;

    HXBOOL                                m_drop_packets;
    UINT32                              m_packets_since_last_drop;

    HXBOOL                                m_bSkipTimeAdjustment;

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
RTSPTransport::IsInitialized(void)
{
    return m_bIsInitialized;
}

inline HXBOOL
RTSPTransport::IsUpdated(void)
{
    return m_bIsUpdated;
}

inline UINT32
RTSPTransport::wrapSequenceNumber(void)
{
    return m_wrapSequenceNumber;
}

#endif /* ndef _RTSPTRAN_H_ */
