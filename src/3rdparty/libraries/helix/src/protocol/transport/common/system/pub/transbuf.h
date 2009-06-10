/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: transbuf.h,v 1.17 2007/07/06 20:51:40 jfinnecy Exp $
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

#ifndef _TRANSPORT_BUFFER_H_
#define _TRANSPORT_BUFFER_H_

#include "hxpends.h"
#include "hxbitset.h"
#include "timeval.h"
#include "hxtbuf.h"
#include "cachobj.h"

#include "hxthread.h"
#include "hxdeque.h"

class ClientPacket;
class RTSPTransport;

class PendingPacket
{
  public:
    PendingPacket(UINT32 ulSeqNo, UINT32 arrivalTime);
    ~PendingPacket();

    UINT32 m_ulSequenceNumber;
    UINT32 m_ulNumPktsBehind;
    UINT32 m_ulArrivalTime;
};

typedef enum
{
    TRANSBUF_INITIALIZING,
    TRANSBUF_FILLING,
    TRANSBUF_READY
} TRANSBUF_STATUS;

class BufferTimer
{
public:
    BufferTimer() {};
    ~BufferTimer() {};

    Timeval		m_StartTime;
    Timeval		m_PreviousTime;
    Timeval		m_LastTime;
    Timeval		m_PauseTime;
};

class RTSPTransportBuffer
{
public:
    
    RTSPTransportBuffer(RTSPTransport* pOwner, UINT16 uStreamNumber,
                        UINT32 bufferDuration, UINT32 maxBufferDuration,
                        UINT32 growthRate, UINT32 wrapSequenceNumber);
    ~RTSPTransportBuffer();

    void		Reset();
    void		Grow();
    void		SetEndPacket(UINT16 uSeqNo,
			             UINT16 uReliableSeqNo,
			             HXBOOL   bPacketSent,
			             UINT32 uTimestamp,
                                     UINT32 ulReasonCode);
    void		InformSourceStopped(void);
    void		InformTimestampRange(UINT32 ulStartTimestamp,
					     UINT32 ulEndTimestamp,
					     UINT32 ulEndDelayTolerance = 0);

    HX_RESULT		Init(UINT16 uSeqNo, HXBOOL bOnPauseResume = FALSE);
    HX_RESULT		Add(ClientPacket* pPacket);
    HX_RESULT		Insert(ClientPacket* pPacket);
    HX_RESULT		Flush();
    HX_RESULT		GetPacket(ClientPacket*& pPacket);
    HX_RESULT		StartPackets();
    HX_RESULT		StopPackets();
    void		ReleasePackets();
    HX_RESULT		GetStatus(UINT16& uStatusCode, 
			          UINT16& ulPercentDone);
    HX_RESULT		GetCurrentBuffering(UINT32& ulLowestTimestamp, 
					    UINT32& ulHighestTimestamp,
					    UINT32& ulNumBytes,
					    HXBOOL& bDone);
    
    HXBOOL		IsSourceDone(void) {return m_bIsEnded;};

    HX_RESULT		UpdateStatistics(ULONG32& ulNormal,
			                 ULONG32& ulLost,
			                 ULONG32& ulLate,
			                 ULONG32& ulResendRequested,
			                 ULONG32& ulResendReceived,
			                 ULONG32& ulAvgBandwidth,
			                 ULONG32& ulCurBandwidth,
					 ULONG32& ulTotal30,
					 ULONG32& ulLost30,
					 ULONG32& ulDuplicate,
					 ULONG32& ulOutOfOrder);
    HX_RESULT		SetupForACKPacket(UINT16& uSeqNo,
			                  CHXBitset& pBitset,
			                  UINT16& uBitCount,
			                  HXBOOL& didACK,
			                  HXBOOL& bLostHigh,
			                  HXBOOL& bNeedAnotherACK);
    UINT32		GetIndex(UINT32 uBaseSequenceNumber, UINT16 uSeqNo);
    UINT32		GetPacketIndex(UINT16 uSeqNo);
    UINT32		GetSeekIndex(UINT16 uSeqNo);
    UINT32		GetACKIndex(UINT16 uSeqNo);
    void		InitTimer();
    void		InitializeTime(BufferTimer* Timer);
    void		UpdateTime(BufferTimer* Timer);
    void		SetMulticast();
    void		SetLive() { m_bIsLive = TRUE; }
    Timeval		GetTime(BufferTimer* Timer);
    Timeval		GetTime();
    Timeval		AdjustedStartTime(BufferTimer* Timer);
    Timeval		AdjustedLastTime(BufferTimer* Timer);
    void		Pause();
    void		Resume();
    void		SanitizePacketQueue();
    void		CheckForSourceDone();
    void		UpdateStatsFromPacket(ClientPacket* pPacket);
    void		SeekFlush();
    void		SetBufferDepth(UINT32 uMilliseconds);
    void                SetBufferParameters(UINT32 uMinimumDelay,             /* ms */
                                            UINT32 uMaximumDelay,             /* ms */
                                            UINT32 uExtraDelayDuringBuffering /* ms */);

    void		EnterPrefetch(void);
    void		LeavePrefetch(void);
    void		DoPrefetch(void);

    void		EnterFastStart(void) { m_bFastStart = TRUE; };
    void		LeaveFastStart(void) { m_bFastStart = FALSE; };

    HX_RESULT		GetPacketFromCache(ClientPacket*& pPacket);
    HX_RESULT		GetPacketFromQueue(ClientPacket*& pPacket);

    HX_RESULT           GetTransportBufferInfo(UINT32& ulLowestTimestamp,
					       UINT32& ulHighestTimestamp,
					       UINT32& ulBytesBuffered);

    HXBOOL                OverByteLimit() const;
    void                SetByteLimit(UINT32 ulByteLimit);
    UINT32              GetByteLimit() const;
    void                ConvertToDroppedPkt(ClientPacket*& pPacket);

    void Func();

private:

    inline HXBOOL IsQueueEmpty();

    class RTSPTransportBufferCallback : public IHXCallback
    {
    public:
        RTSPTransportBuffer*  m_pTransBuff;
   
        RTSPTransportBufferCallback(RTSPTransportBuffer* pTransBuff);
        //IUnknown methods
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef) (THIS);
        STDMETHOD_(ULONG32,Release) (THIS);

        //IRMACallback methods
        STDMETHOD(Func) (THIS); 

        void Clear();

    protected:
        ~RTSPTransportBufferCallback();

        LONG32 m_lRefCount;
    };
    CallbackHandle               m_CallbackHandle;
    RTSPTransportBufferCallback* m_pCallBack;
    IHXMutex*                    m_pPendingLock;
    
    //To keep track of packets we think might be lost.
    CHXSimpleList		 m_PendingPackets;

    IHXScheduler*	m_pScheduler;
#if defined(HELIX_FEATURE_FIFOCACHE)
    IHXFIFOCache*	m_pFIFOCache;
#else
    void*		m_pFIFOCache;
#endif /* HELIX_FEATURE_FIFOCACHE */
    RTSPTransport*	m_pOwner;
    UINT16		m_uStreamNumber;
    HX_deque*		m_pPacketDeque;
    CHXSimpleList	m_pHoldList;
    TRANSBUF_STATUS	m_status;
    HXBOOL		m_bPrefetch;
    HXBOOL		m_bFastStart;
    HXBOOL		m_bCacheIsEmpty;
    HXBOOL		m_bIsInitialized;
    HXBOOL		m_bWaitingForSeekFlush;
    HXBOOL		m_bWaitingForLiveSeekFlush;
    HXBOOL		m_bFlushHolding;
    HXBOOL		m_bIsEnded;
    HXBOOL		m_bStreamBegin;
    HXBOOL		m_bStreamDone;
    HXBOOL		m_bSourceStopped;
    HXBOOL		m_bACKDone;
    HXBOOL		m_bStreamDoneSent;
    HXBOOL		m_bPaused;
    HXBOOL		m_bPausedHack;
    UINT32		m_bufferDuration;
    UINT32              m_extraDelayDuringBuffering;
    UINT32		m_maxBufferDuration;
    UINT32		m_growthRate;
    UINT32		m_wrapSequenceNumber;
    UINT32		m_ulFrontTimeStampCached;
    UINT32		m_ulRearTimeStampCached;
    UINT16		m_uReliableSeqNo;
    UINT16		m_uEndReliableSeqNo;
    UINT16		m_uFirstSequenceNumber;
    UINT16		m_uLastSequenceNumber;
    UINT16		m_uACKSequenceNumber;
    UINT16		m_uEndSequenceNumber;
    UINT16		m_uSeekSequenceNumber;
    UINT16		m_uSeekCount;
    UINT32		m_uNormal;
    UINT32		m_ulDuplicate;
    UINT32		m_ulOutOfOrder;
    UINT32		m_uLost;
    UINT32		m_uLate;
    UINT32		m_ulIndex30;
    UINT32		m_ulTotal30[30];
    UINT32		m_ulLost30[30];
    UINT32		m_uResendRequested;
    UINT32		m_uResendReceived;
    INT64		m_uByteCount;
    INT64		m_uLastByteCount;
    UINT32		m_uAvgBandwidth;
    UINT32		m_uCurBandwidth;
    UINT32		m_ulLastLost30;
    UINT32		m_ulLastTotal30;
    UINT32		m_uLastTimestamp;
    UINT32		m_uStartTimestamp;
    UINT32		m_uEndTimestamp;
    UINT32		m_ulEndDelayTolerance;
    HXBOOL		m_bExpectedTSRangeSet;
    BufferTimer		m_StatisticsTime;
    BufferTimer		m_PacketTime;
    BufferTimer		m_LastPacketTime;
    HXBOOL		m_bPacketsStarted;
    UINT32		m_ulCurrentQueueByteCount;
    UINT32		m_ulCurrentCacheByteCount;
    HXBOOL		m_bAtLeastOnePacketReceived;
    HXBOOL		m_bAtLeastOneResetHandled;
    UINT32		m_ulFirstTimestampReceived;
    UINT32		m_ulLastTimestampReceived;
    UINT32		m_ulBufferingStartTime;
    UINT32		m_ulLastGrowTime;
    UINT32              m_ulEndReasonCode;
    HXBOOL		m_bMulticast;
    HXBOOL		m_bMulticastReset;
    HXBOOL		m_bMulticastReliableSeqNoSet;
    HXBOOL		m_bIsLive;
    HXBOOL		m_bSparseStream;
    HXBOOL              m_bFirstPacketAdd;
    HXBOOL              m_bFirstPacketGet;
    UINT32              m_ulByteLimit; /* Max number of bytes allowed in
					* in queue. 0 means unlimited
					*/
};

inline UINT32
RTSPTransportBuffer::GetPacketIndex(UINT16 uSeqNo)
{
    return GetIndex(m_uFirstSequenceNumber, uSeqNo);
}

inline UINT32
RTSPTransportBuffer::GetSeekIndex(UINT16 uSeqNo)
{
    return GetIndex(m_uSeekSequenceNumber, uSeqNo);
}

inline UINT32
RTSPTransportBuffer::GetACKIndex(UINT16 uSeqNo)
{
    return GetIndex(m_uACKSequenceNumber, uSeqNo);
}

inline void                
RTSPTransportBuffer::SetByteLimit(UINT32 ulByteLimit)
{
    m_ulByteLimit = ulByteLimit;
}

inline UINT32
RTSPTransportBuffer::GetByteLimit() const
{
    return m_ulByteLimit;
}

inline HXBOOL 
RTSPTransportBuffer::IsQueueEmpty()
{
    HXBOOL bRet = TRUE;
    if(m_pPacketDeque != NULL)
    {
        bRet = m_pPacketDeque->empty();
    }
    return bRet;
}

#endif /* _TRANSPORT_BUFFER_H_ */
