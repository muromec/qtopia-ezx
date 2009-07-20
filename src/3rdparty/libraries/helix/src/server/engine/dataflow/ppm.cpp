/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ppm.cpp,v 1.89 2007/02/14 19:21:54 jrmoore Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <new.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "proc.h"
#include "server_engine.h"
#include "ihxpckts.h"
#include "hxdtcvt.h"
#include "hxformt.h"
#include "hxasm.h"
#include "source.h"
#include "hxassert.h"
#include "hxslist.h"
#include "hxdeque.h"
#include "hxmap.h"
#include "servpckts.h"
#include "transport.h"
#include "base_errmsg.h"
#include "asmrulep.h"
#include "server_info.h"
#include "servreg.h"
#include "bwcalc.h"
#include "bandcalc.h"
#include "mutex.h"
#include "loadinfo.h"
#include "ihxlist.h"
#include "hxlistp.h"
#include "server_stats.h"
#include "server_context.h"
#include "hxservpause.h"
#include "sdptools.h"
#include "hxmime.h"
#include "seqno.h"
#include "servertbf.h"
#include "brdetect.h"

#include "dtcvtcon.h"
#include "strmdata.h"
#include "mem_cache.h"
#include "hxpiids.h"
#include "globals.h"
#include "tsconvrt.h"
#include "rtpinfosync.h"
#include "player.h"
#include "ppm.h"
#include "rsdpacketq.h"

#include "servbuffer.h"

#include "livekeyframe.h"

#include "bcastmgr.h"
#include "hxtick.h"

#include "hxlimit.h"

#define PACKET_QUEUE_SIZE 12
#define FILTER_CONSTANT   0.01
#define EXTRA_MEDIARATE_AMOUNT "config.LiveReducedStartupDelay.ExtraMediaRateAmount"
#define DEFAULT_EXTRA_MEDIARATE_AMOUNT 20 
#define EXTRA_PREROLL_IN_PERCENTAGE "config.LiveReducedStartupDelay.ExtraPrerollInPercentage"
#define DEFAULT_EXTRA_PREROLL_IN_PERCENTAGE 20 
#define MIN_TOKEN_BUCKET_CEILING "config.LiveReducedStartupDelay.MinTokenBucketCeiling"
#define DEFAULT_MIN_TOKEN_BUCKET_CEILING 240000
#define CPU_THRESHOLD_TO_DISABLE_RSD "config.LiveReducedStartupDelay.CPUThresholdToDisableRSD"
#define ENABLE_LIVE_RSD_LOG "config.LiveReducedStartupDelay.EnableLiveRSDClientLog"
#define ENABLE_QUEUE_DEBUG_LOG "config.LiveReducedStartupDelay.EnableServerQueueDebugLog"
#define ENABLE_RSD_PER_PACKET_LOG "config.LiveReducedStartupDelay.EnableLiveRSDPerPacketLog"
#define DEFAULT_CPU_THRESHOLD_TO_DISABLE_RSD 65 
#define USER_AGENT_STRING "config.LiveReducedStartupDelay.UserAgentString"
#define SERVER_LIVERSD_CPUTHRESHOLD_REACHED "Server.LiveRSDCPUThresholdReached"
#define MAX_ITER 5
#define NORMAL_ITER 2
#define RSD_QUEUE_STATUS_LOG_INTERVAL 5000
#define RSD_MAX_QUEUE_LENGTH_IN_SECOND 60
//#define RSD_LIVE_DEBUG
//#define FORCE_MOBILE_LIVE_RSD_LOGIC

char* g_userAgentOS[] = 
{
    "win32",
    "mac",
    "linux",
    "freebsd",
    "sunos",
    NULL
};

char** g_userAgentWildCard = 0;

#ifdef PAULM_STREAMDATATIMING
#include "classtimer.h"
void
_ExpiredStreamDataCallback(void* p)
{
    PPMStreamData* pSD = (StreamData*)p;
    printf("\tm_pSession: 0x%x\n", pSD->m_pSession);
    printf("\tm_pTransport: 0x%x\n", pSD->m_pTransport);
}
ClassTimer g_StreamDataTimer("StreamData",
    _ExpiredStreamDataCallback, 3600);
#endif

#ifdef PAULM_PPMTIMING
#include "classtimer.h"
ClassTimer g_PPMTimer("PPM", 0, 3600);
#endif

PPM::PPM(Process* pProc, BOOL bIsRTP)
    : m_pProc(pProc)
    , m_bInitialPlayReceived(FALSE)
    , m_bIsRTP(bIsRTP)
{
#ifdef PAULM_PPMTIMING
    g_PPMTimer.Add(this);
#endif
#ifndef _SOLARIS27
    m_pTimeCallback                     = new (m_pProc->pc->mem_cache) TimeCallback();
#else
    m_pTimeCallback                     = new TimeCallback();
#endif
    m_pTimeCallback->m_pPPM             = this;
    m_pTimeCallback->AddRef();
    m_uScheduledSendID                  = 0;

    /*
     * All delivery rates subscribed to before the initial play request are
     * stored here until the play is received. This allows us to not show
     * bandwidth for content that is being served from a downstream cache,
     * because in that case, the RealProxy never sends a play request.
     */
    m_ulPendingBitRate                  = 0;

    /* This value is the sum of the delivery rates of all sessions / streams */
    m_ulDeliveryBitRate                 = 0;

    /*
     * This is the rate that the player told us to send (higher than
     * m_ulDeliveryBitRate for oversend, lower for buffered play)
     */
    m_ulActualDeliveryRate              = 0;
    /*
     * This is the rate that the player REALLY told us to send and includes
     * VBR bitrates not included in m_ulActualDeliveryRate
     */
    m_ulAdjustedDeliveryRate            = 0;

    /* The last time we ran SendNextPacket() */
    m_tLastSendTime                     = 0.0;

    /* The next time we call SendNextPacket(), used for timing the next pkt */
    m_tNextSendTime                     = 0.0;

    /*
     * Set initial state to Idle.  Idle state implies:
     *   - SendNextPacket() will not find any packets suitable for delivery.
     *   - There is no outstanding TimeCallback on the scheduler.
     *   - We are waiting for PacketReady() some (if any) non-paused,
     *     non-seeking Streams.
     */
    m_bIdle                             = TRUE;
    HXAtomicIncUINT32(g_pIdlePPMs);

    m_ulBackToBackCounter               = 9;
    m_ulBackToBackFreq                  = 10;
    m_bInformationalAggregatable        = FALSE;
    m_bDidLock                          = FALSE;
}

PPM::~PPM()
{
    if (m_bIdle) HXAtomicDecUINT32(g_pIdlePPMs);
#ifdef PAULM_PPMTIMING
    g_PPMTimer.Remove(this);
#endif
    HXList_iterator     i(&m_Sessions);

    while (m_Sessions.peek_head()) {
        SessionDone((Session*) m_Sessions.peek_head());
    }

    /* Zero the bandwidth so the ServerInfo class gets an update */
    //JMEV  SessionDone should have already  subtracted each session's bandwidth

    if (m_ulDeliveryBitRate)
    {
        ChangeDeliveryBandwidth(-1 * (INT32)m_ulDeliveryBitRate, TRUE);
    }

    if (m_uScheduledSendID)
    {
        m_pProc->pc->engine->ischedule.remove(m_uScheduledSendID);
    }
    m_pTimeCallback->Release();

    if (m_ulActualDeliveryRate)
    {
        HXAtomicSubUINT32(g_pAggregateRequestedBitRate, m_ulActualDeliveryRate);
    }

    if (m_bInformationalAggregatable)
    {
        *g_pAggregatablePlayers -= 1;
    }
}

void
PPM::RegisterSource(IHXPSourceControl* pSourceCtrl,
                    IHXPacketFlowControl** ppSessionControl,
                    UINT16 unStreamCount,
                    BOOL bIsLive, BOOL bIsMulticast,
                    DataConvertShim* pDataConv)
{
    if (bIsLive)
    {
        IHXPSourceLivePackets* pSourcePackets;

        if (HXR_OK == pSourceCtrl->QueryInterface(IID_IHXPSourceLivePackets,
                                                  (void **)&pSourcePackets))
        {
            /* For Static Content */
            Session* pSession = new Session(m_pProc, unStreamCount, this, NULL,
                                            pSourcePackets, bIsLive, bIsMulticast);
            pSession->AddRef();
            if (pDataConv)
            {
                pSession->SetConverter(pDataConv);
            }

            m_Sessions.insert(pSession);

            *ppSessionControl = pSession;
            pSession->AddRef();
            pSourcePackets->Init(pSession);
        }
        else
        {
            PANIC(("No Source for Live Packets\n"));
        }
    }
    else
    {
        IHXPSourcePackets* pSourcePackets;

        if (HXR_OK == pSourceCtrl->QueryInterface(IID_IHXPSourcePackets,
                                                  (void **)&pSourcePackets))
        {
            /* For Static Content */
            Session* pSession = new Session(m_pProc, unStreamCount, this,
                                            pSourcePackets, NULL, bIsLive, bIsMulticast);
            pSession->AddRef();
            if (pDataConv)
            {
                pSession->SetConverter(pDataConv);
            }

            m_Sessions.insert(pSession);

            *ppSessionControl = pSession;
            pSession->AddRef();
            pSourcePackets->Init(pSession);
            pSession->m_bThreadSafeGetPacket = (pSourcePackets->IsThreadSafe() &
                                                HX_THREADSAFE_METHOD_FF_GETPACKET) ? TRUE : FALSE;
        }
        else
        {
            PANIC(("No Source for Packets\n"));
        }
    }
}

void
PPM::RegisterSource(IHXPSourceControl* pSourceCtrl,
                    IHXPacketFlowControl** ppSessionControl,
                    UINT16 unStreamCount,
                    BOOL bIsLive, BOOL bIsMulticast,
                    DataConvertShim* pDataConv,
                    Player* pPlayerCtrl,
                    const char* szPlayerSessionId,
                    IHXSessionStats* pSessionStats)
{

    HX_ASSERT(pPlayerCtrl);
    RegisterSource(pSourceCtrl, ppSessionControl, unStreamCount, bIsLive,
                   bIsMulticast, pDataConv);

    if (*ppSessionControl)
    {
        Session *pSession = (Session*)(*ppSessionControl);
        pSession->SetPlayerInfo(pPlayerCtrl, szPlayerSessionId, pSessionStats);
    }
}

void
PPM::SessionDone(PPM::Session* pSession)
{
    ASSERT(pSession);

    m_Sessions.remove(pSession);
    pSession->Done();
    pSession->Release();
}

void
PPM::ChangeDeliveryBandwidth(INT32 lChange,
                             BOOL bReportChange,
                             PPMStreamData* pSD,
                             BOOL bOverrideAverage)
{
    // Don't record this bandwidth if we are not supposed to report it
    // or we've received a NULL setup (meaning the content itself is
    // being served from a downstream cache).
    if (bReportChange &&
        (!pSD || !pSD->m_bNullSetup))
    {
        if (m_bInitialPlayReceived)
        {
            m_ulDeliveryBitRate += lChange;
        }
        else
        {
            m_ulPendingBitRate += lChange;
        }
    }

    //printf ("New Delivery TOTAL %d\n", m_ulDeliveryBitRate);

    if (pSD)
    {
        if (bOverrideAverage)
        {
            pSD->m_ulAvgBitRate = lChange;

#if 0
            INT32 temp = pSD->m_ulAvgBitRate;

            // XXXtbradley we don't need to do this because the initial
            // average bit rate set for the stream in RegisterStream was
            // never reported to server_info
            lChange -= temp;
#endif // 0
        }
        else
        {
            pSD->m_ulAvgBitRate += lChange;
        }
    }

    RecalcActualDeliveryRate();

    // Don't record this bandwidth if we are not supposed to report it
    // or we've received a NULL setup (meaning the content itself is
    // being served from a downstream cache).
    if (bReportChange &&
        (!pSD || !pSD->m_bNullSetup))
    {
        if (m_bInitialPlayReceived)
        {
            m_pProc->pc->server_info->ChangeBandwidthUsage(lChange, m_pProc);
        }
    }
}

void
PPM::CommitPendingBandwidth()
{
    // The bandwidth gets set during the default subscription for some
    // players.
    if (m_ulPendingBitRate && !m_bInitialPlayReceived)
    {
        m_bInitialPlayReceived = TRUE;
        m_ulDeliveryBitRate += m_ulPendingBitRate;
        m_pProc->pc->server_info->ChangeBandwidthUsage(m_ulPendingBitRate, m_pProc);
        m_ulPendingBitRate = 0;
    }
}

void
PPM::GetAggregationLimits(_ServerState State,
                          UINT32       ulActualDeliveryRate,
                          UINT32*      pAggregateTo,
                          UINT32*      pAggregateHighest)
{
    int i = (int)State;
    if (State == ExtremeLoad || g_pAggregateMaxBPS[i] == 0 ||
        ulActualDeliveryRate < g_pAggregateMaxBPS[i])
    {
        *pAggregateTo = g_pAggregateTo[i];
        *pAggregateHighest = g_pAggregateHighest[i];
    }
    else
    {
        *pAggregateTo = g_pAggregateTo[i+1];
        *pAggregateHighest = g_pAggregateHighest[i+1];
    }
}



STDMETHODIMP
PPM::TimeCallback::Func()
{
    BOOL bReturn;
    UINT32 ulRepeatCount = 0;

    m_pPPM->m_uScheduledSendID = 0;

    /*
     * This code allows us to run SendNextPacket up to 5 times in a row, if the
     * timer was going to be 0.  This provides us with a ~20% performance boost
     * because everything in SNP, the file format, and smplfsys is hot in the
     * processor cache.  Turning the number "6" between 0 - 30 will trade off
     * between improved top end capacity, and smoothness of the bandwidth
     * delivery.
     */
    do {
        bReturn = m_pPPM->SendNextPacket();
    } while (bReturn && ulRepeatCount++ < 6);

    if (bReturn == TRUE)
    {
        m_pPPM->m_uScheduledSendID = m_pPPM->m_pProc->pc->engine->ischedule.enter(
            Timeval(0,0), this);
    }

#ifdef TEST_CRASH_AVOID
    static int count = 0;
    volatile int x;

    if (count++ % 500 == 0) {
        printf ("Destroy Stack\n");
        fflush(0);
        memset((char *)&x - 16384, 0, 32768);
    }
    else if (count % 500 == 250) {
        printf ("Crash\n");
        volatile int* x = 0; *x = 100;
    }
#endif

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::TimeStampCallback::Func()
{
    HX_ASSERT(m_pSession->m_pStreamData[m_unStreamNumber].m_bStreamRegistered);
    m_pSession->
        m_pStreamData[m_unStreamNumber].m_uTimeStampScheduledSendID = 0;
    m_pSession->SendTimeStampedPacket(m_unStreamNumber);

    return HXR_OK;
}

STDMETHODIMP
StreamDoneCallback::Func()
{
    m_pSD->m_uScheduledStreamDoneID = 0;

    m_pTransport->streamDone(m_unStreamNumber);

    return HXR_OK;
}

PPMStreamData::PPMStreamData()
    : m_pPauseAdvise(NULL)
    , m_pExpectedRuleSeqNoArray(NULL)
{
#ifdef PAULM_STREAMDATATIMING
    g_StreamDataTimer.Add(this);
#endif
    m_ulAvgBitRate                  = 0;
    m_ulVBRAvgBitRate               = 0;
    m_ulMaxBitRate                  = 0;
    m_ulMinBitRate                  = 0;
    m_pRules                        = 0;
    m_lNumRules                     = 0;
    m_unSequenceNumber              = 0;
    m_ulExpectedInSeqNo             = 0;
    m_bFirstPacket                  = TRUE;
    m_unReliableSeqNo               = 0;
    m_lBytesDueTimes10              = 0;
    m_pTransport                    = 0;
    m_pSession                      = NULL;
    m_pRVDrop                       = 0;
    m_ulPacketsOutstanding          = 0;

    m_pHistoryQueue                 = new HX_deque;
    m_unOutSeqNo                    = 0;

    m_pTimeStampCallback                        = new PPM::Session::TimeStampCallback();
    m_pTimeStampCallback->AddRef();
    m_pTimeStampCallback->m_pSession            = NULL;
    m_uTimeStampScheduledSendID                 = 0;

    m_pStreamDoneCallback                       = NULL;
    m_uScheduledStreamDoneID                    = 0;

    m_bPacketRequested              = FALSE;
    m_bStreamDone                   = FALSE;
    m_bSentStreamDone               = FALSE;
    m_bStreamDonePending            = FALSE;
    m_lStreamRatioTemp              = 0;
    m_bWouldBlocking                = FALSE;
    m_bSetEncoderOffset             = TRUE;
    m_bGotSubscribe = FALSE;
    m_bStreamRegistered             = FALSE;

    m_ulEncoderTimeMinusPlayerTimeOffset = 0L;
    m_ulFirstPacketTS = 0;
    m_bFirstPacketTSSet = FALSE;
    m_pTSConverter = NULL;

    m_pLinkCharParams = NULL;
    m_pStreamAdaptParams = NULL;
}

PPMStreamData::~PPMStreamData()
{
#ifdef PAULM_STREAMDATATIMING
    g_StreamDataTimer.Remove(this);
#endif
    delete[] m_pRules;

    if (m_uTimeStampScheduledSendID)
    {
        m_pSession->m_pProc->pc->engine->
            ischedule.remove(m_uTimeStampScheduledSendID);
        m_uTimeStampScheduledSendID=0;
    }
    m_pTimeStampCallback->Release();

    if (m_uScheduledStreamDoneID)
    {
        m_pSession->m_pProc->pc->engine->
            schedule.remove(m_uScheduledStreamDoneID);
    }

    ClearSequenceHistory();
    HX_VECTOR_DELETE(m_pExpectedRuleSeqNoArray);

    HX_RELEASE(m_pStreamDoneCallback);
    HX_RELEASE(m_pTransport);
    HX_RELEASE(m_pPauseAdvise);

    HX_DELETE(m_pTSConverter);
    HX_DELETE(m_pRVDrop);
    HX_DELETE(m_pHistoryQueue);
    HX_DELETE(m_pLinkCharParams);
    HX_DELETE(m_pStreamAdaptParams);
}

void
PPMStreamData::SetSession(PPM::Session* pSession)
{
    // Should only be done during initialization
    ASSERT(pSession != NULL && m_pTimeStampCallback->m_pSession == NULL);
    m_pTimeStampCallback->m_pSession = pSession;
    m_pSession = pSession;
}

inline BOOL
PPMStreamData::IsStreamDone()
{
    return (m_pPackets.IsEmpty() && m_bStreamDone && (!m_bSentStreamDone));
}

BOOL
PPMStreamData::IsDependOk(BOOL bOn, UINT16 unRule)
{
    UINT16* pDeps               = bOn ? m_pRules[unRule].m_pOnDepends
                                      : m_pRules[unRule].m_pOffDepends;

    if (!pDeps)
        return TRUE;

    for (; *pDeps != 0xffff; pDeps++)
    {
        if (m_pRules[*pDeps].m_PendingAction != NONE)
        {
            return FALSE;
        }
        if (m_pRules[*pDeps].m_bRuleOn != bOn)
        {
            return FALSE;
        }
    }

    return TRUE;
}

void
PPMStreamData::Reset()
{
    m_pPackets.Clear();
    m_ulPacketsOutstanding = 0;
    m_bPacketRequested = FALSE;
    m_bSetEncoderOffset = TRUE;
    m_ulEncoderTimeMinusPlayerTimeOffset = 0;
    m_tLastScheduledTime = m_pSession->m_tTimeLineStart;
    if (m_uTimeStampScheduledSendID)
    {
        m_pSession->m_pProc->pc->engine->ischedule.remove(
            m_uTimeStampScheduledSendID);
        m_uTimeStampScheduledSendID = 0;
    }
}

void
PPMStreamData::SetFirstPacketTS(UINT32 ulTimeStamp)
{
    m_ulFirstPacketTS = ulTimeStamp;
    m_bFirstPacketTSSet = TRUE;
}

BOOL
PPMStreamData::IsFirstPacketTSSet()
{
    return (m_bFirstPacketTSSet);
}

/* 
 * When we remove the guarantee of in-order packet delivery to support
 * Low Latency Live we cannot keep the trivially simple sequence number 
 * management we had in the past.
 * 
 * In the good old days, LiveSessionPacketReady discarded packets whose
 * rules were not subscribed and incremented a per-stream sequence counter
 * for each packet sent. This worked because the live broadcast ng architecture
 * is built around using a large buffer in the broadcast receiver to guarantee
 * that packets will be delivered in order. Gaps are filled with dummy "lost"
 * packets. There are no late packets.
 * 
 * When you remove that buffer and the in-order guarantee you end up with an 
 * interesting situation. Each packet has a rule number associated with it. 
 * If the given rule is not subscribed the packet is simply discarded and will
 * not affect the sequence space that the client sees.
 *
 * The problem arises with lost packets. If the PPM is expecting packet 10 and
 * instead receives 15, it has to send that packet out but it has no way of 
 * knowing how many of the missing 5 packets are for rules that are subscribed. 
 * We cannot re-order packets within a stream. We can't just pass the max gap
 * on to the client. We have to have a way to know what sequence number to 
 * assign to this packet and also a way to correctly assign a seqeunce number
 * to late packets.
 *
 * This is not a new problem, it existed in legacy servers but was negligible 
 * because we buffered enough to hide it effectively. 
 * 
 * The solution is to use an array to store per-rule sequence number in each 
 * data packet so that we can figure out immediately how many of the missing
 * packets are needed by the client. We also use an array in PPMStreamData 
 * to track the historical data needed to assign the correct sequence number
 * to a late packet.
 *
 * if bSubscribed is FALSE that means we are throwing this packet away
 * (usually because its for a rule we aren't subscribed to).
 *
 * The packets the client sees should be unchanged by this code although
 * they can be sent out of order. This code will not be run unless the rule
 * sequence number information is available.
 */
HX_RESULT
PPMStreamData::HandleSequence(UINT32  ulInSeqNo, 
                              UINT16  unStreamNo, 
                              UINT16  unRuleNo, 
                              UINT16* pRuleSequenceArray,
                              UINT16& unOutSeqNo, 
                              BOOL    bSubscribed)
{
    int i;

    /*
    if (unStreamNo == 1)
    {
        fprintf(stderr, "PPM::HandleSeq %u, %u: in %u exp %u out %u\n",
                unStreamNo,
                unRuleNo,
                ulInSeqNo,
                m_ulExpectedInSeqNo,
                m_unOutSeqNo + 1);
        fflush(stderr);
    }
    */

    if (m_bFirstPacket)
    {
        m_bFirstPacket = FALSE;
        m_unLastLateOutSeqNo = m_unOutSeqNo;
        m_uOutofRange = 0;
        m_unMaxHistoryQSize = 64;
        m_ulExpectedInSeqNo = ulInSeqNo;

        for (i=0; i < m_lNumRules; i++)
        {
            m_pExpectedRuleSeqNoArray[i] = pRuleSequenceArray[i];
        }
    }

    if (ulInSeqNo == m_ulExpectedInSeqNo)
    {
        // got the packet we expected (this code path is performance critical)      
        m_ulExpectedInSeqNo++;

        // If the seqno q is not empty we need to push a NULL onto the end,
        // the q is for bookkeeping for late packets and this one's not late.
        if (!m_pHistoryQueue->empty())
        {
          m_pHistoryQueue->push_back(0);
        }

        if (bSubscribed == TRUE)
        {
            unOutSeqNo = m_unOutSeqNo++;

            // handle wraparound
            if (m_unOutSeqNo >= m_pTransport->wrapSequenceNumber())
            {
                m_unOutSeqNo = 0;
            }
        }

        m_pExpectedRuleSeqNoArray[unRuleNo]++;
    }
    else
    {
        if (Seq32GT(ulInSeqNo, m_ulExpectedInSeqNo))
        {
            // this packet is early : we didn't get some packets we expected

            UINT32 ulDistance = ulInSeqNo - m_ulExpectedInSeqNo;

            if (ulDistance > 1024)
            {
                /* this packet "looks" early but the gap is too big... something
                 * is wrong, either this is a very late packet or somehow a large
                 * disconnect in the sequence numbers happened. We clear the 
                 * decks and start fresh if this happens 10 times in a row. */
                m_uOutofRange++;
                if (m_uOutofRange >= 10)
                {
                    // start fresh ... or maybe it would be better to terminate?
                    ClearSequenceHistory();
                    m_bFirstPacket = TRUE;
                }

                //printf("HandleSeq: early pkt out of range, cntr %d\n", 
                //         m_uOutofRange);

                // returning HXR_FAIL should toss the packet
                return HXR_FAIL;
            }

            /* we see a new max and a seq gap, we use the new rule sequence
             * numbers to figure out how many of the missing packets are 
             * subscribed so we can send this packet with the right sequence
             * number.
             */
            UINT16 uSubscribedPkts = 0;

            for (i=0; i < m_lNumRules; i++)
            {
                if (m_pRules[i].m_bRuleOn)
                {
                    if (pRuleSequenceArray[i] > m_pExpectedRuleSeqNoArray[i])
                    {
                        uSubscribedPkts += pRuleSequenceArray[i] - 
                                                    m_pExpectedRuleSeqNoArray[i];
                    }
                    else if (pRuleSequenceArray[i] < m_pExpectedRuleSeqNoArray[i])
                        // seqno for this rule must have rolled over 
                    {
                        uSubscribedPkts += 0xffff - m_pExpectedRuleSeqNoArray[i] +
                                              pRuleSequenceArray[i];
                    }
                }
            }

            SequenceHistory* pHistory = NULL;

            if (uSubscribedPkts)
            {
                if (m_pHistoryQueue->empty())
                {
                    m_ulFirstQSeqNo = m_ulExpectedInSeqNo;
                }

                pHistory = new SequenceHistory;
                pHistory->m_ulRefCount = ulDistance;
                pHistory->m_unOutSeqNo = m_unOutSeqNo;
                pHistory->m_pRuleSeqNoArray = new UINT16[m_lNumRules];
            }

            for (i=0; i < m_lNumRules; i++)
            {
                if (pHistory)
                {
                    pHistory->m_pRuleSeqNoArray[i] = m_pExpectedRuleSeqNoArray[i];
                }

                m_pExpectedRuleSeqNoArray[i] = pRuleSequenceArray[i];
            }

            if (!m_pHistoryQueue->empty() || uSubscribedPkts)
            {
                for (i=0; i < (int)ulDistance; i++)
                {
                    m_pHistoryQueue->push_back((void *)(pHistory));
                }
            }

            for (i=0; i < uSubscribedPkts; i++)
            {
                m_unOutSeqNo++;

                // handle wraparound
                if (m_unOutSeqNo >= m_pTransport->wrapSequenceNumber())
                {
                    m_unOutSeqNo = 0;
                }
            }

            // push a "0" on the back for the packet that uncovered the gap
            if (!m_pHistoryQueue->empty())
            {
                m_pHistoryQueue->push_back(0);
            }

            //printf("HandleSeq, strm %d, seq gap of %d, %d subscribed. Qsize %d\n",
            //      unStreamNo, ulDistance, uSubscribedPkts, m_pHistoryQueue->size());

            if (bSubscribed == TRUE)
            {
                unOutSeqNo = m_unOutSeqNo++;

                // handle wraparound
                if (m_unOutSeqNo >= m_pTransport->wrapSequenceNumber())
                {
                    m_unOutSeqNo = 0;
                }
            }

            m_ulExpectedInSeqNo = ulInSeqNo + 1;
            m_pExpectedRuleSeqNoArray[unRuleNo]++;
        }
        else
        {
            /* this packet is late. We look in our history queue, if its there,
             * great, otherwise its too late, toss it (and consider growing the 
             * history queue */

            // as long as there are no reserved seqnos wraparound is simple and
            // we don't need to provide a Seq32Subtract function
            UINT32 ulDistance = m_ulExpectedInSeqNo - ulInSeqNo;

            if (ulDistance > 1024)
            {
                /* this packet "looks" late but the gap is too big... something
                 * is wrong. Toss the packet but we clear the decks and start 
                 * fresh if this happens 10 times in a row. */

                m_uOutofRange++;
                if (m_uOutofRange >= 10)
                {
                    // start fresh
                    ClearSequenceHistory();
                    m_bFirstPacket = TRUE;
                }

                //printf("HandleSeq: late pkt out of range, cntr %d\n", m_uOutofRange);

                // returning HXR_FAIL should toss the packet
                return HXR_FAIL;
            }

            UINT32 ulQIndex;

            // as long as there are no reserved seqnos wraparound is simple and
            // we don't need to provide a Seq32Subtract function
            ulQIndex = ulInSeqNo - m_ulFirstQSeqNo;

            if (m_pHistoryQueue->empty() || (ulQIndex > m_pHistoryQueue->size()))
            {
                /* If its subscribed, this packet arrived after its history data
                 * aged out of the queue. Lets see what the seqno was the last
                 * time this happened and consider bumping up the history queue 
                 * size if its happening "too often". This Q size is not a critical 
                 * parameter but a smaller queue will empty faster and this code will
                 * be slightly more efficient. If its too small we give up on late 
                 * packets too soon. */
                if (bSubscribed == TRUE)
                {
                    if (m_unOutSeqNo < m_unLastLateOutSeqNo + 4096)
                    {
                        if ((m_unOutSeqNo > m_unLastLateOutSeqNo + 32) && 
                            (m_unMaxHistoryQSize < 1024))
                        {
                            m_unMaxHistoryQSize += 64;
                            m_unLastLateOutSeqNo = m_unOutSeqNo;
                        }
                    }
                    else
                    {
                        m_unLastLateOutSeqNo = m_unOutSeqNo;
                    }
#if 0
                    printf("HandleSeq strm %d, InSeq %u is late, index OOB! "
                           " Qfirst %u Qsize %d Qmax %d\n", 
                           unStreamNo, ulInSeqNo, m_ulFirstQSeqNo, 
                           m_pHistoryQueue->size(), m_unMaxHistoryQSize);
#endif
                }

                // returning HXR_FAIL should toss the packet
                return HXR_FAIL;
            }

            SequenceHistory* pHistory = (SequenceHistory*)(*m_pHistoryQueue)[ulQIndex];

            HX_ASSERT(pHistory);

            if (!pHistory)
            {
                //printf("HandleSeq strm %d, No history data found, returning\n", 
                //        unStreamNo);

                return HXR_FAIL;
            }

            unOutSeqNo = pHistory->m_unOutSeqNo;

#if 0
            printf("HandleSeq strm %d InSeq %d is late! Qindex %d first on Q %d "
                   " Qsize %d history %p, last out %d\n",
                   unStreamNo, ulInSeqNo, ulQIndex, m_ulFirstQSeqNo, 
                   m_pHistoryQueue->size(), 
                   pHistory, pHistory->m_unOutSeqNo);
#endif

            for (i=0; i < m_lNumRules; i++)
            {
                if (m_pRules[i].m_bRuleOn)
                {
                    if (pHistory->m_pRuleSeqNoArray[i] < pRuleSequenceArray[i])
                    {
                        unOutSeqNo += pRuleSequenceArray[i] - 
                                                    pHistory->m_pRuleSeqNoArray[i];
                    }
                    else if (pRuleSequenceArray[i] < pHistory->m_pRuleSeqNoArray[i])
                        // seqno for this rule must have rolled over 
                    {
                        unOutSeqNo += 0xffff - pHistory->m_pRuleSeqNoArray[i] +
                                              pRuleSequenceArray[i];
                    }
                }
            }

            pHistory->m_ulRefCount--;

            if (!pHistory->m_ulRefCount)
            {
                HX_VECTOR_DELETE(pHistory->m_pRuleSeqNoArray);
                HX_VECTOR_DELETE(pHistory);
            }

            (*m_pHistoryQueue)[ulQIndex] = 0;
        }
    }

    /* We want to drain the history queue because we run a tad more efficiently
     * when its empty but we don't want to go into a long loop here. */
    i=0; 
    while (m_pHistoryQueue->size() && ((*m_pHistoryQueue)[0] == NULL) && (i < 22))
    {
        m_ulFirstQSeqNo++;

        m_pHistoryQueue->pop_front();

        i++;
    }

    /* Prune the history queue to its maximum allowed length */
    while (m_pHistoryQueue->size() > m_unMaxHistoryQSize)
    {
        SequenceHistory* pHistory = (SequenceHistory*)(*m_pHistoryQueue)[0];

        // printf("HandleSeq strm %d, Prune (size %d)! freeing q[0] seq %d value %p\n", 
        //         unStreamNo, m_pHistoryQueue->size(), m_ulFirstQSeqNo, pHistory);
        m_ulFirstQSeqNo++;

        if (pHistory)
        {
            pHistory->m_ulRefCount--;

            if (!pHistory->m_ulRefCount)
            {
                HX_VECTOR_DELETE(pHistory->m_pRuleSeqNoArray);
                HX_VECTOR_DELETE(pHistory);
            }
        }

        m_pHistoryQueue->pop_front();
    }

#if 0
    char pszRuleArray[256];
    char* pTmp = pszRuleArray;
    pszRuleArray[0] = 0;
    for (i=0; i < m_lNumRules; i++)
    {
      sprintf(pTmp, " %d->%d", i, m_pExpectedRuleSeqNoArray[i]);
      pTmp += strlen(pTmp);
    }

    printf("HandleSeq, strm %d, out %d, next out %d expectedIn %d  "
           "Qsize %d\nRules %s\n", 
            unStreamNo, unOutSeqNo, m_unOutSeqNo, m_ulExpectedInSeqNo, 
            m_pHistoryQueue->size(), pszRuleArray);
#endif

    return HXR_OK;
}

HX_RESULT
PPMStreamData::ClearSequenceHistory()
{
    m_bFirstPacket = TRUE;

    while (m_pHistoryQueue->size())
    {
        SequenceHistory* pHistory = (SequenceHistory*)(*m_pHistoryQueue)[0];

        if (pHistory)
        {
            pHistory->m_ulRefCount--;

            if (!pHistory->m_ulRefCount)
            {
                HX_VECTOR_DELETE(pHistory->m_pRuleSeqNoArray);
                HX_VECTOR_DELETE(pHistory);
            }
        }

        m_pHistoryQueue->pop_front();
    }

    return HXR_OK;
}


void
PPM::Session::GetNextPacket(UINT16 unStreamNumber)
{
    HX_ASSERT(!m_bIsLive);
    if (m_bIsLive)
    {
        return;
    }

    PPMStreamData* pSD = m_pStreamData + unStreamNumber;
    HX_ASSERT(pSD->m_bStreamRegistered);

    if (m_pSourcePackets)
    {
        if ((m_bAttemptingBackToBack == FALSE) &&
            (pSD->m_ulPacketsOutstanding != 0))
        {
            return;
        }

        /* Errors trapped in FileFormatSource */
        if (!pSD->m_bPacketRequested)
        {
            pSD->m_bPacketRequested = TRUE;
            m_pSourcePackets->GetPacket(unStreamNumber);
        }
    }
}

void
PPM::Session::ScheduleStreamDone(Session* pSession,
                                 Transport* pTransport,
                                 PPMStreamData* pSD,
                                 UINT16 unStreamNumber)
{
    /*
     * Notify the protocol if we've reached the end of the stream following a
     * seek and have no packets to send.
     */
    if (!pSD->IsFirstPacketTSSet())
    {
        SetStreamStartTime(unStreamNumber, (UINT32)~0);
    }

    HandleStreamDone(pSD);
    pSD->m_bSentStreamDone = TRUE;
    HX_ASSERT(!pSD->m_uScheduledStreamDoneID);
    if (pSD->m_uScheduledStreamDoneID)
    {
        return;
    }
    else if (!pSD->m_pStreamDoneCallback)
    {
#ifndef _SOLARIS27
        pSD->m_pStreamDoneCallback                   = new (m_pProc->pc->mem_cache) StreamDoneCallback();
#else
        pSD->m_pStreamDoneCallback                   = new StreamDoneCallback();
#endif
        pSD->m_pStreamDoneCallback->m_pSession       = pSession;
        pSD->m_pStreamDoneCallback->m_pTransport     = pTransport;
        pSD->m_pStreamDoneCallback->m_pSD            = pSD;
        pSD->m_pStreamDoneCallback->m_unStreamNumber = unStreamNumber;
        pSD->m_pStreamDoneCallback->AddRef();
    }

    Timeval tNow = m_pProc->pc->engine->now;
    Timeval tCallbackTime;

    tCallbackTime = tNow;

    pSD->m_uScheduledStreamDoneID =
        m_pProc->pc->engine->schedule.enter(tCallbackTime,
        pSD->m_pStreamDoneCallback);

    if (!pSD->m_uScheduledStreamDoneID)
    {
        HX_RELEASE(pSD->m_pStreamDoneCallback);
    } 
}

void
PPM::Session::HandleStreamDone(PPMStreamData* pSD)
{
    m_ulNumStreamDones++;
    ASSERT(m_ulNumStreamDones <= m_uNumStreamsRegistered);

    // enumerate the rules, subtract delivery rates that have been reported.

    for (INT32 lRule = 0; lRule < pSD->m_lNumRules; lRule++)
    {
        if (pSD->m_pRules[lRule].m_bBitRateReported)
        {
            // Update delivery rate
            m_ulDeliveryRate -= pSD->m_pRules[lRule].m_ulAvgBitRate;
            m_pPPM->ChangeDeliveryBandwidth((-1) *
                    (INT32)pSD->m_pRules[lRule].m_ulAvgBitRate,
                    !m_bIsMulticast, pSD);
            if (pSD->m_pRules[lRule].m_bTimeStampDelivery)
            {
                pSD->m_ulVBRAvgBitRate -= pSD->m_pRules[lRule].m_ulAvgBitRate;
            }

            pSD->m_pRules[lRule].m_bBitRateReported = FALSE;
        }
    }

    if (m_ulNumStreamDones == m_uNumStreamsRegistered)
    {
        m_bSourceIsDone = TRUE;
        m_pPPM->RecalcActualDeliveryRate();
        /*
         * Stop accumulating PlayTime.  The Protocol won't send us a Pause()
         * even if it gets an RTSP PAUSE, because it thinks we're already
         * paused.
         */
        Pause(FALSE, 0);
    }
}

void
PPM::Session::HandleUnStreamDone(PPMStreamData* pSD)
{
    ASSERT(m_ulNumStreamDones);
    m_ulNumStreamDones--;

    // enumerate the rules, report delivery rates for active ones.

    for (INT32 lRule = 0; lRule < pSD->m_lNumRules; lRule++)
    {
        if (pSD->m_pRules[lRule].m_bRuleOn &&
            !pSD->m_pRules[lRule].m_bBitRateReported)
        {
            // Update delivery rate
            m_ulDeliveryRate += pSD->m_pRules[lRule].m_ulAvgBitRate;
            m_pPPM->ChangeDeliveryBandwidth(
                    (INT32)pSD->m_pRules[lRule].m_ulAvgBitRate,
                    !m_bIsMulticast, pSD);
            if (pSD->m_pRules[lRule].m_bTimeStampDelivery)
            {
                pSD->m_ulVBRAvgBitRate += pSD->m_pRules[lRule].m_ulAvgBitRate;
            }

            pSD->m_pRules[lRule].m_bBitRateReported = TRUE;
        }
    }

    m_bSourceIsDone = FALSE;
    m_pPPM->RecalcActualDeliveryRate();
    /* Unschedule any outstanding stream dones on this stream */
    if (pSD->m_uScheduledStreamDoneID)
    {
        m_pProc->pc->engine->schedule.remove(pSD->m_uScheduledStreamDoneID);
        pSD->m_uScheduledStreamDoneID = 0;
    }
}

STDMETHODIMP
PPM::Session::StreamDone(UINT16 unStreamNumber, BOOL bForce /* = FALSE */)
{
    if (m_bIsDone)
    {
        return HXR_OK;
    }

    HX_ASSERT(unStreamNumber < m_unStreamCount);
    HX_ASSERT(m_pStreamData);

    if (m_pStreamData == NULL)
    {
        return HXR_OK;
    }

    if (unStreamNumber >= m_unStreamCount)
    {
        return HXR_OK;
    }

    PPMStreamData* pSD = m_pStreamData + unStreamNumber;

    HX_ASSERT(pSD->m_bStreamRegistered);
    if (!pSD->m_bStreamRegistered || pSD->m_bStreamDone)
    {
        return HXR_OK;
    }

    pSD->m_bStreamDone = TRUE;

    ServerPacket* pPacket = pSD->m_pPackets.PeekPacket();

    /*
     * Don't issue a stream done here if there are any outstanding packets.
     * We will issue it after the packet is sent if we fail to send it here.
     */
    if (bForce || ((!pPacket) && (!pSD->m_uTimeStampScheduledSendID) &&
        pSD->IsStreamDone()))
    {
        ScheduleStreamDone(this, pSD->m_pTransport, pSD, unStreamNumber);
    }

    if (m_bSourceIsDone)
    {
        /*
         * Make sure our packet queue is empty.
         */
        SendPacketQueue(pSD->m_pTransport);
    }

    return HXR_OK;
}

void
PPM::Session::SetStreamStartTime(UINT32 ulStreamNum, UINT32 ulTS)
{
    if (m_pPlayerControl && m_pPlayerSessionId)
    {
        const char* szSessionID = (const char*)m_pPlayerSessionId->GetBuffer();
        m_pPlayerControl->SetStreamStartTime(szSessionID, ulStreamNum, ulTS);
    }
}

PPMStreamData::RuleInfo::RuleInfo()
    : m_ulPriority(5)
    , m_ulAvgBitRate(0)
    , m_ulMaxBitRate(0)
    , m_bBitRateReported(FALSE)
    , m_bTimeStampDelivery(FALSE)
    , m_pBWCalculator(NULL)
    , m_bWaitForSwitchOffFlag(TRUE)
    , m_bRuleOn(FALSE)
    , m_bSyncOk(TRUE)
    , m_PendingAction(NONE)
    , m_pOnDepends(0)
    , m_pOffDepends(0)
    , m_bActivateOnSeek(FALSE)
{
}

PPMStreamData::RuleInfo::~RuleInfo()
{
    HX_RELEASE(m_pBWCalculator);
    delete[] m_pOnDepends;
    delete[] m_pOffDepends;
}

PPMStreamData::Packets::Packets()
{
    m_ulPacketRingReaderPos = 0;
    m_ulPacketRingWriterPos = 0;
}

PPMStreamData::Packets::~Packets()
{
    Clear();
}

void
PPMStreamData::Packets::Clear()
{
    while (m_ulPacketRingReaderPos != m_ulPacketRingWriterPos)
    {
        HX_RELEASE(m_pPacketRing[m_ulPacketRingReaderPos]);
        Inc(m_ulPacketRingReaderPos);
    }
    m_ulPacketRingReaderPos = 0;
    m_ulPacketRingWriterPos = 0;
}

ServerPacket*
PPMStreamData::Packets::GetPacket()
{
    if (m_ulPacketRingReaderPos == m_ulPacketRingWriterPos)
    {
        return 0;
    }

    ServerPacket* pRet = m_pPacketRing[m_ulPacketRingReaderPos];
    // One AddRef for the outgoing packet.
    // One Release because we don't care about this packet anymore.

    Inc(m_ulPacketRingReaderPos);

    ASSERT(pRet);

    return pRet;
}

ServerPacket*
PPMStreamData::Packets::PeekPacket()
{
    /* This function violates COM Reference Rules */

    if (m_ulPacketRingReaderPos == m_ulPacketRingWriterPos)
    {
        return 0;
    }

    ServerPacket* pRet = m_pPacketRing[m_ulPacketRingReaderPos];

    ASSERT(pRet);

    return pRet;
}

HX_RESULT
PPMStreamData::Packets::PutPacket(ServerPacket* pPacket)
{
    UINT32 ulTemp = m_ulPacketRingWriterPos;

    Inc(ulTemp);

    if (ulTemp == m_ulPacketRingReaderPos)
    {
        ASSERT(0);
        return HXR_BUFFERTOOSMALL;
    }

    ASSERT(pPacket);
    m_pPacketRing[m_ulPacketRingWriterPos] = pPacket;
    pPacket->AddRef();

    m_ulPacketRingWriterPos = ulTemp;

    return HXR_OK;
}

BOOL
PPMStreamData::Packets::IsEmpty()
{
    return (m_ulPacketRingReaderPos == m_ulPacketRingWriterPos);
}

PPM::Session::Session(Process* p, UINT16 unStreamCount, PPM* pPPM,
                      IHXPSourcePackets* pSourcePackets,
                      IHXPSourceLivePackets* pSourceLivePackets,
                      BOOL bIsLive,
                      BOOL bIsMulticast)
    : m_bStallPackets(FALSE)
      , m_bNeedStreamStartTime(FALSE)
      , m_tvBankedPlayTime(0,0)
      , m_pSessionStats(NULL)
      , m_pSessionStats2(NULL)
      , m_uFirstStreamRegistered(0xFFFF)
      , m_uNumStreamsRegistered(0)
      , m_bEnableRSDPerPacketLog(FALSE)
{
    m_pProc                     = p;
    m_pPPM                      = pPPM;
    m_unStreamCount             = unStreamCount;
    m_ulRefCount                = 0;
    m_bPaused                   = FALSE;
    m_bInSeek                   = FALSE;
    m_bInitialSubscriptionDone  = FALSE;
    m_bInitialPlayReceived      = FALSE;
    m_bGetPacketsOutstanding    = FALSE;
    m_ulTSDMark                 = 0;
    m_tTimeLineStart.tv_sec     = 0;
    m_tTimeLineStart.tv_usec    = 0;
    m_bTimeLineSuspended        = TRUE;
    m_pSourcePackets            = pSourcePackets;
    m_pSourceLivePackets        = pSourceLivePackets;
    m_bIsMulticast              = bIsMulticast;
    m_ulPacketsOutstanding      = 0;
    m_bAttemptingBackToBack     = 0;
    m_ucPacketQueuePos          = 0;
    m_ulPacketQueueSize         = 0;
    m_pPacketQueue              = new BasePacket*[PACKET_QUEUE_SIZE];
    m_ulActualDeliveryRate      = 0;
    m_ulDeliveryRate            = 0;
    m_fDeliveryRatio            = 1.0;
    m_bSourceIsDone             = FALSE;
    m_ulNumStreamDones          = 0;
    m_pLiveResync               = 0;

    m_bIsLive                   = bIsLive;
    m_uEndPoint                 = 0;
    m_bIsPausePointSet   = FALSE;
    m_bIsDone                   = FALSE;
    m_ulResendIDPosition        = 0;
    m_bSubscribed               = FALSE;
    m_ulWouldBlocking           = 0;
    m_bWouldBlockAvailable      = FALSE;
    m_pConvertShim              = 0;
    m_pConvertingPacket         = NULL;
    m_bThreadSafeGetPacket      = FALSE;
    m_bPlayPendingOnSeek        = FALSE;
    m_pASMSource          = NULL;
    m_pPlayerControl      = NULL;
    m_pPlayerSessionId    = NULL;
    m_bSeekPacketPending  = FALSE;
    m_bSessionPlaying     = FALSE;
    m_pRTPInfoSynch       = NULL;
    m_unRTPSynchStream    = 0;

    m_pPacketBufferProvider = NULL;
    m_pServerTBF = NULL;
    m_ulClientBandwidth = 0;
    m_ulPacketSent = 0;
    m_bPktBufQProcessed = FALSE;
    m_pRSDPacketQueue = NULL;

    m_ulPreData = 0;
    m_ulTotalBytesSent = 0;
    m_bSentPredata = FALSE;
    m_ulIteration = MAX_ITER;
    m_bCheckLiveRSD = FALSE;
    m_bEnableLiveRSDLog = FALSE;
    m_bQueueDebugLog = FALSE;
    m_ulQueueDebugTS = 0;

    m_bIsReflector = FALSE;
    m_unRTCPRule = 0xffff;
    m_bFirstPacket = TRUE;
    m_bAdjustingQLength = FALSE;
    m_pTSDCB = NULL;
    m_RSDCBhandle = 0;
    m_pCBRCB = NULL;
    m_pBRDetector = NULL;

#ifdef RSD_LIVE_DEBUG
    m_ulPrevTotalBytesSent = 0;
    m_ulLastTS = 0;
    m_uPacketListStart = 0;
#endif
    memset(m_pResendIDs, 0, sizeof(UINT32) * MAX_RESENDS_PER_SECOND);
    m_pBlockedQ = new CHXRingBuffer(NULL);
    m_pBlockedQ->AddRef();
    m_uBlockedBytes = 0;
    m_ulHeadTSDTime = 0;
    m_ulLastTSDTime = 0;
    m_uMaxBlockedBytes = 100000;
    m_uMaxBlockedQInMsecs = 4000;

    m_pBlockedTSDQ = new CHXRingBuffer(NULL);
    m_pBlockedTSDQ->AddRef();

    m_pSessionAggrLinkCharParams = NULL;
    m_bStreamLinkCharSet = FALSE;

    m_enumStreamAdaptScheme = ADAPTATION_NONE;
    m_pAggRateAdaptParams = NULL;

    ((CHXRingBuffer *)m_pBlockedQ)->SetSize(256);
    ((CHXRingBuffer *)m_pBlockedQ)->SetGrow(256);
    ((CHXRingBuffer *)m_pBlockedTSDQ)->SetSize(256);
    ((CHXRingBuffer *)m_pBlockedTSDQ)->SetGrow(256);

    INT32 nMax;

    if (HXR_OK == m_pProc->pc->registry->GetInt("config.MaxBlockedLiveInSecs",
                                                             &nMax, m_pProc))
    {
        m_uMaxBlockedQInMsecs = nMax * 1000;
    }

    if (m_bIsLive && !m_pSourceLivePackets)
    {
        HX_ASSERT(0);
    }

    if (!m_bIsLive && !m_pSourcePackets)
    {
        HX_ASSERT(0);
    }

    if (bIsLive && m_pSourceLivePackets)
    {
        if (HXR_OK != m_pSourceLivePackets->QueryInterface(IID_IHXASMSource,
                                                       (void**)&m_pASMSource))
        {
            m_pASMSource = NULL;
        }

        m_pSourceLivePackets->QueryInterface(IID_IHXPSourceLiveResync,
                                       (void**)&m_pLiveResync);
    }

    // Contorted initialization due to NT not able to intialize array
    // elements on creation even though it is standard C++
    m_pStreamData               = new PPMStreamData[unStreamCount];

    // then the intialization
    for (UINT16 iStream = 0; iStream < unStreamCount; iStream++)
        m_pStreamData[iStream].SetSession(this);

    // get clock for PlayTime tracking
    if (HXR_OK != m_pProc->pc->server_context->
        QueryInterface(IID_IHXAccurateClock, (void**) &m_pAccurateClock))
    {
        // system doesn't have accurate clock, use our own.
        AddRef();
        m_pAccurateClock = (IHXAccurateClock*) this;
    }

    // for safety, initialize this to current time, although we really
    // set it in Play().

    HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
    m_tvRTSPPlayTime.tv_sec = hxt.tv_sec;
    m_tvRTSPPlayTime.tv_usec = hxt.tv_usec;
}

PPM::Session::~Session()
{
    if (!m_bIsDone) Done();
}

STDMETHODIMP
PPM::Session::ControlDone()
{
    if (m_pPPM) m_pPPM->SessionDone(this);

    return HXR_OK;
}

void
PPM::Session::Done()
{
    if (m_bIsDone) return;

    m_bIsDone = TRUE;

    // Update the PlayTime stat
    // we come here on both TEARDOWN and client socket reset

    HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
    Timeval t((long)hxt.tv_sec, (long)hxt.tv_usec);

    if (!m_bPaused) m_tvBankedPlayTime += (t - m_tvRTSPPlayTime);
    UpdatePlayTime();

    if(m_RSDCBhandle)
    {
        m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
        m_RSDCBhandle = 0;
    }

    if (m_ulDeliveryRate)
    {
        // XXXJMEV  the stream/rule data still indicate 'bitrate reported';
        // is it worth enumerating them to set m_bBitRateReported to FALSE?
#if 0
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            PPMStreamData* pSD = m_pStreamData + i;
            for (INT32 lRule = 0; lRule < pSD->m_lNumRules; lRule++)
            {
                pSD->m_pRules[lRule].m_bBitRateReported = FALSE;

            }
        }
#endif
        m_pPPM->ChangeDeliveryBandwidth((-1) * (INT32)m_ulDeliveryRate,
                    !m_bIsMulticast, NULL);
        m_ulDeliveryRate = 0;
    }
    m_pPPM = 0;

    HX_RELEASE(m_pSourcePackets);
    HX_RELEASE(m_pLiveResync);

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        PPMStreamData* pSD = m_pStreamData + i;

        if (!pSD->m_bStreamRegistered)
        {
            continue;
        }

        if (m_pASMSource)
        {
            for (INT32 lRule = 0; lRule < pSD->m_lNumRules; lRule++)
            {
                if (pSD->m_pRules[lRule].m_bRuleOn)
                {
                    m_pASMSource->Unsubscribe(i, lRule);
                }
            }
        }

        if (m_bIsLive && m_pSourceLivePackets)
        {
            m_pSourceLivePackets->StopPackets(i);
        }
    }
    HX_RELEASE(m_pASMSource);
    HX_RELEASE(m_pSourceLivePackets);
    HX_VECTOR_DELETE(m_pStreamData);

    if (m_pRTPInfoSynch)
    {
        m_pRTPInfoSynch->Done();
        HX_RELEASE(m_pRTPInfoSynch);
    }


    HX_RELEASE(m_pBlockedQ);
    HX_RELEASE(m_pBlockedTSDQ);
    m_ulHeadTSDTime = 0;
    m_ulLastTSDTime = 0;

    for (int j = 0; j < MAX_RESENDS_PER_SECOND; j++)
    {
        if (m_pResendIDs[j])
        {
                m_pProc->pc->engine->schedule.remove(m_pResendIDs[j]);
            m_pResendIDs[j] = 0;
        }
    }

    while (--m_ucPacketQueuePos != (UINT8)-1)
    {
        m_pPacketQueue[m_ucPacketQueuePos]->Release();
    }

    HX_VECTOR_DELETE(m_pPacketQueue);
    if (m_pConvertShim)
    {
        m_pConvertShim->Done();
        HX_RELEASE(m_pConvertShim);
    }
    HX_RELEASE(m_pConvertingPacket);

    HX_RELEASE(m_pPlayerControl);
    HX_RELEASE(m_pPlayerSessionId);
    HX_RELEASE(m_pSessionStats);
    HX_RELEASE(m_pSessionStats2);
    HX_RELEASE(m_pAccurateClock);

    HX_RELEASE(m_pPacketBufferProvider);

    HX_DELETE(m_pServerTBF);
    HX_DELETE(m_pRSDPacketQueue);
    HX_RELEASE(m_pTSDCB);
    HX_RELEASE(m_pCBRCB);
    HX_DELETE(m_pBRDetector);

    HX_DELETE(m_pSessionAggrLinkCharParams);
    HX_DELETE(m_pAggRateAdaptParams);
}

void
PPM::Session::JumpStart(UINT16 uStreamNumber)
{
    HX_ASSERT(!m_bIsLive);
    if (m_bIsLive)
    {
        return;
    }

    PPMStreamData* pStreamData = m_pStreamData + uStreamNumber;
    HX_ASSERT(pStreamData->m_bStreamRegistered);

    /* Jump start the PPM if it's idle or the next packet is TS delivered */
    ServerPacket* pNextPacket = pStreamData->m_pPackets.PeekPacket();
    if (pNextPacket)
    {
        if (!m_bSessionPlaying && !m_bTimeLineSuspended)
        {
            BOOL bIsFirstSeekPckt = m_bSeekPacketPending ? TRUE : FALSE;
            ResetSessionTimeline(pNextPacket, uStreamNumber, bIsFirstSeekPckt);

            m_bSeekPacketPending = FALSE;
            m_bSessionPlaying = TRUE;
        }

        if ((pStreamData->m_pRules[pNextPacket->m_uASMRuleNumber].
            m_bTimeStampDelivery) &&
            (!pStreamData->m_uTimeStampScheduledSendID) && (!IsPaused()))
        {
        Timeval tTimeStamp;
            ULONG32 ulPacketTimeStamp = pNextPacket->GetTime();

            if (pStreamData->m_pSession->m_bIsLive)
            {
                //If this is the first live packet since the player requested
                // the stream, keep track of the time difference between when
                // the remote encoder started sending packets (on which the
                // packet time stamps are based) and the player start time:
                if (pStreamData->m_bSetEncoderOffset)
                {
                    pStreamData->m_ulEncoderTimeMinusPlayerTimeOffset =
                        ulPacketTimeStamp;
                    pStreamData->m_bSetEncoderOffset = FALSE;
                }

                //Adjust the TSDelivery time to the timeline of the player
                // receiving these packets:
                if(ulPacketTimeStamp >=
                   pStreamData->m_ulEncoderTimeMinusPlayerTimeOffset)
                {
                    ulPacketTimeStamp -=
                        pStreamData->m_ulEncoderTimeMinusPlayerTimeOffset;
                }
            }

            ulPacketTimeStamp = (ulPacketTimeStamp > m_ulTSDMark) ?
                ulPacketTimeStamp - m_ulTSDMark : 0;

            if (m_fDeliveryRatio != 0.0)
            {
                ulPacketTimeStamp = ulPacketTimeStamp / m_fDeliveryRatio;
            }

            tTimeStamp.tv_sec  = ulPacketTimeStamp / 1000;
            tTimeStamp.tv_usec = (ulPacketTimeStamp % 1000) * 1000;

            /*
             * Schedule the next packet based on it's
             * delivery time with respect to the current
             * timeline.
             */

            if ( (m_tTimeLineStart + tTimeStamp) >
                 m_pProc->pc->engine->now)
            {
            /* early: schedule for some time in the future */
                    pStreamData->m_uTimeStampScheduledSendID =
                        m_pProc->pc->engine->ischedule.enter(
                            m_tTimeLineStart + tTimeStamp,
                            pStreamData->m_pTimeStampCallback);
                    pStreamData->m_tLastScheduledTime = m_tTimeLineStart +
                        tTimeStamp;
            }
            else if ( ((m_tTimeLineStart + tTimeStamp) <
                       m_pProc->pc->engine->now) &&
                      (pStreamData->m_ulMaxBitRate > 500) )
/* The MaxBitRate > 500 constraint exists to account for event streams
   wherein the current bitrate may exceed in the case of seek, etc. the
   max bit rate. */
            {
                /* late: send now! (unless we excede max birate) */
                UINT32 ulCurrentBitRate = pStreamData->m_pRules[pNextPacket->m_uASMRuleNumber].
                    m_BitRate.GetAverageBandwidth();

                if ( (ulCurrentBitRate != 0) &&
                     (ulCurrentBitRate <= pStreamData->m_ulMaxBitRate) )
                {
                    pStreamData->m_uTimeStampScheduledSendID =
                        m_pProc->pc->engine->ischedule.enter(
                            m_pProc->pc->engine->now,
                            pStreamData->m_pTimeStampCallback);
                    pStreamData->m_tLastScheduledTime = m_pProc->pc->engine->now;
                }
                else
                {
                    pStreamData->m_tLastScheduledTime =
                         pStreamData->m_tLastScheduledTime + (1000 *
                        (INT32)((1000 * pNextPacket->GetSize() * 8) /
                        pStreamData->m_ulMaxBitRate));
                    pStreamData->m_uTimeStampScheduledSendID =
                        m_pProc->pc->engine->ischedule.enter(
                            pStreamData->m_tLastScheduledTime,
                            pStreamData->m_pTimeStampCallback);
                }
            }
            else
            {
                /* on-time or no max bitrate:  schedule for "now" */
                pStreamData->m_uTimeStampScheduledSendID =
                    m_pProc->pc->engine->ischedule.enter(
                        m_pProc->pc->engine->now,
                        pStreamData->m_pTimeStampCallback);
                pStreamData->m_tLastScheduledTime = m_pProc->pc->engine->now;
            }
        }
        else
        {
            if (m_pPPM->m_bIdle)
            {
                BOOL bReturn;

                bReturn = m_pPPM->SendNextPacket();

                if (bReturn == TRUE)
                {
                    m_pPPM->m_uScheduledSendID =
                        m_pPPM->m_pProc->pc->engine->ischedule.enter(
                        Timeval(0,0), m_pPPM->m_pTimeCallback);
                }
            }
        }
    }
}


void
PPM::Session::SendTimeStampedPacket(UINT16 unStreamNumber)
{
    PPMStreamData* pSD = m_pStreamData + unStreamNumber;
    HX_ASSERT(pSD->m_bStreamRegistered);

    ServerPacket* pPacket = pSD->m_pPackets.GetPacket();
    m_ulPacketsOutstanding--;
    pSD->m_ulPacketsOutstanding--;
    ASSERT(pPacket);

    /*
     * Notify the player if this is the first packet for the most recent
     * playback request.
     */
    if (!m_bTimeLineSuspended && !pSD->IsFirstPacketTSSet())
    {
        pSD->SetFirstPacketTS(pPacket->GetTime());
    }

    pSD->m_uTimeStampScheduledSendID = 0;

    pPacket->m_uSequenceNumber = pSD->m_unSequenceNumber++;

    if (pSD->m_unSequenceNumber >= pSD->m_pTransport->wrapSequenceNumber())
    {
        pSD->m_unSequenceNumber = 0;
    }

    if (pPacket->m_uPriority == 10)
        pPacket->m_uReliableSeqNo  = ++pSD->m_unReliableSeqNo;
    else
        pPacket->m_uReliableSeqNo  = pSD->m_unReliableSeqNo;

    if ((m_bRTPInfoRequired) &&
        (!m_bIsMulticast))
    {
        IHXRTPPacket* pRTPPacket = NULL;
        UINT32 ulBaseTime = 0;
        UINT16 i = 0;

        if(pPacket->QueryInterface(IID_IHXRTPPacket, 
                                   (void**) &pRTPPacket) == HXR_OK)
        {
            UINT32 ulRTPTime = pRTPPacket->GetRTPTime();
            
            ulBaseTime =  (pSD->m_pTSConverter) ?
                pSD->m_pTSConverter->rtp2hxa_raw(ulRTPTime)
                : ulRTPTime;

            pSD->m_pTransport->setTimeStamp(unStreamNumber, 
                                            ulRTPTime, 
                                            TRUE);
            HX_RELEASE(pRTPPacket);
        }
        else
        {
            ulBaseTime = pPacket->GetTime();
            pSD->m_pTransport->setTimeStamp(unStreamNumber, ulBaseTime);
        }
        
        pSD->m_pTransport->setSequenceNumber(unStreamNumber, pPacket->m_uSequenceNumber);
        SetStreamStartTime(unStreamNumber, pPacket->GetTime());

        for (i = 0; i < m_unStreamCount; i++)
        {
            if (m_pStreamData[i].m_bStreamRegistered && (i != unStreamNumber))
            { 
                UINT32 ulRTPInfoTime =         (m_pStreamData [i].m_pTSConverter) ?
                    m_pStreamData [i].m_pTSConverter->hxa2rtp_raw(ulBaseTime) :
                    ulBaseTime;

                m_pStreamData [i].m_pTransport->setTimeStamp(i, ulRTPInfoTime, TRUE);
                m_pStreamData [i].m_pTransport->setSequenceNumber(i, 
                                                                  m_pStreamData [i].m_unSequenceNumber);

                SetStreamStartTime(i, pPacket->GetTime());
            }
        }

        m_bRTPInfoRequired = FALSE;
    }

    /*
     * XXXSMP
     * We should support BackToBack packets & aggregation here.  But we never
     * did before, so...
     */

    pSD->m_pTransport->sendPacket(pPacket);
    *g_pBytesServed += pPacket->GetSize();
    (*g_pPPS)++;

    if (pSD->IsStreamDone())
    {
        ScheduleStreamDone(this, pSD->m_pTransport, pSD, unStreamNumber);
    }

    // Notify the appropriate Bandwidth Calculator
    UINT16  uStreamNumber   = pPacket->GetStreamNumber();
    ULONG32 unRule          = pPacket->m_uASMRuleNumber;
    PPMStreamData* pStreamData = m_pStreamData + uStreamNumber;
    pStreamData->m_pRules[unRule].m_pBWCalculator->PacketSent(pPacket);
    HXTimeval tHXNow;
    tHXNow.tv_sec =  m_pProc->pc->engine->now.tv_sec;
    tHXNow.tv_usec = m_pProc->pc->engine->now.tv_usec;
    pStreamData->m_pRules[unRule].m_BitRate.SentPacket(pPacket->GetSize(), tHXNow);

    pPacket->Release();

    JumpStart(uStreamNumber);

    GetNextPacket(uStreamNumber);
}

/* True means start timeline, False means top it */
void
PPM::Session::HandleTimeLineStateChange(BOOL bState)
{
    m_tTimeLineStart = m_pProc->pc->engine->now;
    if (m_bTimeLineSuspended && (bState == TRUE))
    {
        m_bTimeLineSuspended = FALSE;
        m_usStreamsRestarted = 0;
        m_bRTPInfoRequired = TRUE;
    }
    else if ((!m_bTimeLineSuspended) && (bState == FALSE))
    {
        m_bTimeLineSuspended = TRUE;
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            if (!m_pStreamData[i].m_bStreamRegistered)
            {
                continue;
            }

            m_pStreamData[i].m_tLastScheduledTime = m_tTimeLineStart;
                m_pStreamData[i].m_ulFirstPacketTS = 0 ;
                m_pStreamData[i].m_bFirstPacketTSSet = FALSE;
        }
        
        if (m_pRTPInfoSynch)
        {
            m_pRTPInfoSynch->RTPSynch(0);
        }
    }
}

void
PPM::Session::ResetSessionTimeline(ServerPacket* pNextPkt,
                                   UINT16 usStreamNumber,
                                   BOOL bIsPostSeekReset)
{
/* Update session timeline after a stream is paused and resumed */

    if (!m_bSessionPlaying)
    {
        m_tTimeLineStart = m_pProc->pc->engine->now;
        // clip TS corresponding to m_tTimeLineStart:
        m_ulTSDMark = pNextPkt->GetTime();
    }
}

STDMETHODIMP
PPM::Session::Play()
{
    Play(FALSE);

    return HXR_OK;
}

void
PPM::Session::Play(BOOL bWouldBlock)
{
    /*
     * If this play is not really going to unpause us, becuase the other
     * pause is still in effect, then do nothing.
     */
    if ((bWouldBlock && m_bPaused) ||
        (!bWouldBlock && m_ulWouldBlocking))
    {
        return;
    }

    // Subscribe to all rules. This condition is hit when the
    // client does not issue SET_PARAM with subscription before
    // issuing PLAY.

    // "Range" header in PLAY request has a blank "start" time.
    // i.e. "Range: -50.0000" or "Range: - " 
    if (!m_bSubscribed)
    {
        HandleDefaultSubscription();
    }

    BOOL bInitialPlayReceived = m_bInitialPlayReceived;
    if (!m_bInitialPlayReceived)
    {
        m_bInitialPlayReceived = TRUE;

        // Until now, we have not added the bandwidth for this session to
        // the overall total, because we didn't know if the content was
        // being served from a downstream cache. Now, we know that's not
        // the case because the RealProxy wouldn't send a play request.
        m_pPPM->CommitPendingBandwidth();

        // Tell the bandwidth calculators to commit their bandwidth as well
        for (UINT16 n = 0; n < m_unStreamCount; n++)
        {
            PPMStreamData* pStreamData = m_pStreamData + n;

            if (!pStreamData->m_bStreamRegistered)
            {
                continue;
            }

            for (INT32 m = 0; m < pStreamData->m_lNumRules; m++)
            {
                if (pStreamData->m_pRules[m].m_pBWCalculator)
                {
                    pStreamData->m_pRules[m].m_pBWCalculator->
                        CommitPendingBandwidth();
                }
            }
        }
    }

    if (m_bIsMulticast)
        return;

    if (m_bInSeek)
    {
        m_bPlayPendingOnSeek = TRUE;
        return;
    }

    HX_ASSERT(m_uFirstStreamRegistered != 0xFFFF);
    if (m_pStreamData[m_uFirstStreamRegistered].m_pTransport->isNullSetup())
    {
        return;
    }

    if (IsPaused() && m_pLiveResync)
    {
        m_pLiveResync->Resync();

        for (UINT16 j = 0; j < m_unStreamCount; j++)
        {
            PPMStreamData* pSD = m_pStreamData + j;

            if (!pSD->m_bStreamRegistered)
            {
                continue;
            }

            /*
             * A live pause/resume requires a flush
             */

            pSD->Reset();

            for (INT32 i = 0; i < pSD->m_lNumRules; i++)
            {
                pSD->m_pRules[i].m_bSyncOk = FALSE;
                switch (pSD->m_pRules[i].m_PendingAction)
                {
                case NONE:
                case ACTION_ON:
                case SYNC:
                    pSD->m_pRules[i].m_PendingAction = SYNC;
                    break;
                default:
                    pSD->m_pRules[i].m_PendingAction = NONE;
                    break;
                }
            }
        }
    }

    HandleTimeLineStateChange(TRUE);
    m_bInitialSubscriptionDone = TRUE; // It better be done by now!
    if (!bWouldBlock)
    {
        // can we get multiple PLAYs ?
        if (m_bPaused || !bInitialPlayReceived)
        {
            HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
            m_tvRTSPPlayTime.tv_sec = hxt.tv_sec;
            m_tvRTSPPlayTime.tv_usec = hxt.tv_usec;
        }
        m_bPaused = FALSE;
    }
    m_pPPM->RecalcActualDeliveryRate();

    if (m_bIsLive)
    {
        IHXListIterator* pIter;
        IUnknown* punkItem;
        ServerPacket* pCurPkt;
        BOOL bNeedNewHeadTime = FALSE;

        pIter = m_pBlockedQ->Begin();
        while (!m_ulWouldBlocking && pIter->HasItem())
        {
            punkItem = pIter->GetItem();
            HX_ASSERT(punkItem != NULL);
            punkItem->QueryInterface(IID_IHXPacket, (void**)&pCurPkt);
            HX_ASSERT(pCurPkt != NULL);

            // rule and stream are legal, they were checked before queueing
            UINT16 unRule = pCurPkt->GetASMRuleNumber();
            PPMStreamData* pSD = m_pStreamData + pCurPkt->GetStreamNumber();

            if (!pSD->m_pRules[unRule].m_bTimeStampDelivery)
            {
                m_uBlockedBytes -= pCurPkt->GetSize();
            }
            else
            {
                IUnknown* punk2 = m_pBlockedTSDQ->RemoveHead();

                bNeedNewHeadTime = TRUE;
                HX_ASSERT(punkItem == punk2);

                punk2->Release();
            }
            TransmitPacket(pCurPkt);

            pCurPkt->Release();
            punkItem->Release();

            punkItem = m_pBlockedQ->Remove(pIter);
            punkItem->Release();

            pIter->MoveNext();
        }

        pIter->Release();

        if (bNeedNewHeadTime)
        {
            if (m_pBlockedTSDQ->IsEmpty())
            {
                m_ulHeadTSDTime = 0;
                m_ulLastTSDTime = 0;
            }
            else
            {
                punkItem = m_pBlockedTSDQ->GetHead();
                HX_ASSERT(punkItem != NULL);
                punkItem->QueryInterface(IID_IHXPacket, (void**)&pCurPkt);
                HX_ASSERT(pCurPkt != NULL);

                m_ulHeadTSDTime = pCurPkt->GetTime();

                pCurPkt->Release();
                punkItem->Release();
             }
         }

        if(!m_pPacketBufferProvider)
        {
            m_pSourceLivePackets->QueryInterface(IID_IHXLivePacketBufferProvider, 
                                        (void**)&m_pPacketBufferProvider);
        }
    }  /* end if m_bIsLive */

    // advise transports (to resume RTT packets, if desired)

    if (bInitialPlayReceived)
    {
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            if (!m_pStreamData[i].m_bStreamRegistered)
            {
                IHXServerPauseAdvise* pPauseAdvise = m_pStreamData[i].m_pPauseAdvise;
                if (pPauseAdvise)
                {
                    pPauseAdvise->OnPauseEvent(FALSE);
                }
            }
        }
    }

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        if (!m_pStreamData[i].m_bStreamRegistered)
        {
            continue;
        }

        if (m_bIsLive)
        {
            HX_ASSERT(m_pSourceLivePackets);

            // We don't stop incoming packets with wouldblock anymore so
            // its OK to start here without  checking whether we blocked
            // again in loop above

            m_pSourceLivePackets->StartPackets(i);
        }
        else
        {
            JumpStart(i);
        GetNextPacket(i);
        }
    }

    return;
}

STDMETHODIMP
PPM::Session::StartSeek(UINT32 ulTime)
{
    int i;

    if (m_bIsLive)
        return HXR_OK;

    if (m_tTimeLineStart != Timeval(0,0))
    {
        if (m_fDeliveryRatio)
        {
            m_ulTSDMark = ulTime;
        }
    }

    m_tTimeLineStart = m_pProc->pc->engine->now; //- (int)ulTime;

    m_bTimeLineSuspended = TRUE;
    m_bSeekPacketPending = TRUE;

    for (i = 0; i < m_unStreamCount; i++)
    {
        if (m_pStreamData[i].m_bStreamRegistered)
        {
            m_pStreamData[i].Reset();
        }
    }

    // Fix 155342. mp4fformat will not seek properly into 'non-active'
    // streams, so make sure we're subscribed prior to seek.
    if (!m_bSubscribed)
    {
        HandleDefaultSubscription();
    }

    m_bInSeek = TRUE;
    m_bGetPacketsOutstanding = TRUE;
    return HXR_OK;
}

STDMETHODIMP
PPM::Session::SetStartingTimestamp(UINT32 ulStartingTimestamp)
{
    if (m_fDeliveryRatio)
    {
        m_ulTSDMark = ulStartingTimestamp;
    }

    m_tTimeLineStart = m_pProc->pc->engine->now; //- (int)ulTime;
    return HXR_OK;
}

STDMETHODIMP
PPM::Session::SetEndPoint(UINT32 uEndPoint, BOOL bPause)
{
    m_uEndPoint = uEndPoint;
    m_bIsPausePointSet = bPause;

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::Pause(UINT32 ulPausePoint)
{
    Pause(FALSE, ulPausePoint);

    return HXR_OK;
}

void
PPM::Session::Pause(BOOL bWouldBlock, UINT32 ulPausePoint)
{
    if (ulPausePoint > 0)
    {
        SetEndPoint(ulPausePoint, TRUE);
    }
    else
    {
        if(m_bIsPausePointSet)
        {
           m_bIsPausePointSet = FALSE;
           m_uEndPoint = 0;
        }
        m_bPlayPendingOnSeek = FALSE;
        m_bSessionPlaying = FALSE;
        HandleTimeLineStateChange(FALSE);

        // pause live streams

        if (m_bIsLive)
        {
            for (UINT16 i = 0; i < m_unStreamCount; i++)
            {
                if (m_pStreamData[i].m_bStreamRegistered)
                {
                    m_pSourceLivePackets->StopPackets(i);
                }
            }

            // A Pause will cause a skip in the timeline, and we need
            // to reset rsd packet queue.
            // the following play after pause will get a new queue, and
            // start all over again, 
            m_bCheckLiveRSD = FALSE;
            HX_DELETE(m_pServerTBF);
            HX_DELETE(m_pRSDPacketQueue);
            m_ulPacketSent = 0;
            m_ulTBFBandwidth = 0;
            m_ulFirstPacketTS = 0;
            m_ulTotalBytesSent = 0;
            m_bSentPredata = FALSE;
            //if the user want to Pause, then we want to do RSD oversend when the user
            //resume playing.  If Pause is caused by wouldblock, which indicate the bandwidth
            //is not enough for oversend, we should disable RSD by setting m_bPktBufQProcessed
            //to TRUE. So when wouldblock cleared, packets will be delivered the legacy way, 
            //instead of RSD oversending.
            m_bPktBufQProcessed = bWouldBlock;
            m_bFirstPacket = TRUE;
            if(m_RSDCBhandle)
            {
                m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
                m_RSDCBhandle = 0;
            }
            HX_RELEASE(m_pTSDCB);
            HX_RELEASE(m_pCBRCB);
            m_bAdjustingQLength = FALSE;    
        }

        // advise transports (to suspend RTT packets, if desired)

        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            if (!m_pStreamData[i].m_bStreamRegistered)
            {
                continue;
            }

            IHXServerPauseAdvise* pPauseAdvise = m_pStreamData[i].m_pPauseAdvise;
            if (pPauseAdvise)
            {
                pPauseAdvise->OnPauseEvent(TRUE);
            }
        }

        // update PlayTime

        if (!bWouldBlock)
        {
            // can we get multiple pauses?
            if (!m_bPaused)
            {
                m_bPaused = TRUE;  // must be set before UpdatePlayTime().

                HXTimeval hxt = m_pAccurateClock->GetTimeOfDay();
                Timeval t((long)hxt.tv_sec, (long)hxt.tv_usec);

                m_tvBankedPlayTime += (t - m_tvRTSPPlayTime);
                UpdatePlayTime();
            }
        }
    }
}

STDMETHODIMP
PPM::Session::SeekDone()
{
    m_bInSeek = FALSE;

    if (m_bIsLive)
        return HXR_OK;

    HandleTimeLineStateChange(TRUE);
    m_pPPM->RecalcActualDeliveryRate();

    if (!IsPaused())
    {
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            PPMStreamData* pStreamData = m_pStreamData + i;

            if (!pStreamData->m_bStreamRegistered)
            {
                continue;
            }

            if (pStreamData->m_bSentStreamDone)
                HandleUnStreamDone(pStreamData);

            pStreamData->m_bStreamDonePending = FALSE;
            pStreamData->m_bStreamDone = FALSE;
            pStreamData->m_bSentStreamDone = FALSE;

            // unsubscribe any active rules of this stream
            for (int j = 0; j < pStreamData->m_lNumRules; j++)
            {
                if (pStreamData->m_pRules[j].m_bActivateOnSeek)
                {
                    if (pStreamData->m_pRules[j].m_PendingAction ==
                        ACTION_OFF)
                    {
                        if (pStreamData->m_pRules[j].m_bBitRateReported)
                        {
                            m_ulDeliveryRate -= pStreamData->m_pRules[j].m_ulAvgBitRate;
                            m_pPPM->ChangeDeliveryBandwidth((-1) *
                                (INT32)pStreamData->m_pRules[j].m_ulAvgBitRate,
                                !m_bIsMulticast, pStreamData);
                            if (pStreamData->m_pRules[j].m_bTimeStampDelivery)
                            {
                                pStreamData->m_ulVBRAvgBitRate -= pStreamData->m_pRules[j].m_ulAvgBitRate;
                            }

                            pStreamData->m_pRules[j].m_bBitRateReported = FALSE;
                        }

                        pStreamData->m_pRules[j].m_PendingAction = NONE;
                        pStreamData->m_pRules[j].m_bActivateOnSeek = FALSE;
                    }
                    pStreamData->m_pRules[j].m_bActivateOnSeek = FALSE;
                    HandleSubscribe(j, i);
                }

            }

            ASSERT (m_pSourcePackets);
            GetNextPacket(i);
        }
    }
    else
    {
        for (UINT16 i = 0; i < m_unStreamCount; i++)
        {
            PPMStreamData* pStreamData = m_pStreamData + i;

            if (!pStreamData->m_bStreamRegistered)
            {
                continue;
            }

            if (pStreamData->m_bSentStreamDone)
                HandleUnStreamDone(pStreamData);

            pStreamData->m_bStreamDonePending = FALSE;
            pStreamData->m_bStreamDone = FALSE;
            pStreamData->m_bSentStreamDone = FALSE;

            // unsubscribe any active rules of this stream
            for (int j = 0; j < pStreamData->m_lNumRules; j++)
            {
                if (pStreamData->m_pRules[j].m_bActivateOnSeek)
                {
                    if (pStreamData->m_pRules[j].m_PendingAction ==
                        ACTION_OFF)
                    {
                        if (pStreamData->m_pRules[j].m_bBitRateReported)
                        {
                            m_ulDeliveryRate -= pStreamData->m_pRules[j].m_ulAvgBitRate;
                            m_pPPM->ChangeDeliveryBandwidth((-1) *
                                (INT32)pStreamData->m_pRules[j].m_ulAvgBitRate,
                                !m_bIsMulticast, pStreamData);
                            if (pStreamData->m_pRules[j].m_bTimeStampDelivery)
                            {
                                pStreamData->m_ulVBRAvgBitRate -= pStreamData->m_pRules[j].m_ulAvgBitRate;
                            }

                            pStreamData->m_pRules[j].m_bBitRateReported = FALSE;
                        }

                        pStreamData->m_pRules[j].m_PendingAction = NONE;
                        pStreamData->m_pRules[j].m_bActivateOnSeek = FALSE;
                    }
                    pStreamData->m_pRules[j].m_bActivateOnSeek = FALSE;
                    HandleSubscribe(j, i);
                }
            }
        }
    }
    if (m_bPlayPendingOnSeek)
    {
        m_bPlayPendingOnSeek = FALSE;
        Play();
    }

    return HXR_OK;
}


STDMETHODIMP
PPM::Session::Activate()
{
    HandleTimeLineStateChange(TRUE);
    m_pPPM->RecalcActualDeliveryRate();

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        PPMStreamData* pStreamData = m_pStreamData + i;

        if (!pStreamData->m_bStreamRegistered)
        {
            continue;
        }

        if (pStreamData->m_bSentStreamDone)
            HandleUnStreamDone(pStreamData);

        pStreamData->m_bStreamDonePending = FALSE;
        pStreamData->m_bStreamDone = FALSE;
        pStreamData->m_bSentStreamDone = FALSE;

        pStreamData->Reset();

        // unsubscribe any active rules of this stream
        for (int j = 0; j < pStreamData->m_lNumRules; j++)
        {
            if (pStreamData->m_pRules[j].m_bActivateOnSeek)
            {
                if (pStreamData->m_pRules[j].m_PendingAction ==
                    ACTION_OFF)
                {
                    if (pStreamData->m_pRules[j].m_bBitRateReported)
                    {
                        m_ulDeliveryRate -= pStreamData->m_pRules[j].m_ulAvgBitRate;
                        m_pPPM->ChangeDeliveryBandwidth((-1) *
                            (INT32)pStreamData->m_pRules[j].m_ulAvgBitRate,
                            !m_bIsMulticast, pStreamData);
                        if (pStreamData->m_pRules[j].m_bTimeStampDelivery)
                        {
                            pStreamData->m_ulVBRAvgBitRate -= pStreamData->m_pRules[j].m_ulAvgBitRate;
                        }

                        pStreamData->m_pRules[j].m_bBitRateReported = FALSE;
                    }

                    pStreamData->m_pRules[j].m_PendingAction = NONE;
                    pStreamData->m_pRules[j].m_bActivateOnSeek = FALSE;
                }
                pStreamData->m_pRules[j].m_bActivateOnSeek = FALSE;
                HandleSubscribe(j, i);
            }
        }

        ASSERT (m_pSourcePackets);
        GetNextPacket(i);
    }

    return HXR_OK;
}



/*
 * we have not received a subscription request from a client.  Ask Player
 * object to figure out default subscriptions, which will call back to
 * HandleSubscribe()
 */
HX_RESULT
PPM::Session::HandleDefaultSubscription()
{
    HX_ASSERT(!m_bSubscribed);
    HX_RESULT theErr = HXR_OUTOFMEMORY;

    if (m_pPlayerControl && m_pPlayerSessionId)
    {
        return m_pPlayerControl->HandleDefaultSubscription((const char*)m_pPlayerSessionId->GetBuffer());
    }

    return HXR_UNEXPECTED;
}



STDMETHODIMP
PPM::Session::HandleSubscribe(INT32 lRuleNumber,
                              UINT16 unStreamNumber)
{
    PPMStreamData* pStreamData = m_pStreamData + unStreamNumber;
    HX_ASSERT(pStreamData->m_bStreamRegistered);

    if (!pStreamData->m_bStreamRegistered 
            || lRuleNumber >= pStreamData->m_lNumRules)
        return HXR_OK;
    m_bSubscribed = TRUE;

    pStreamData->m_pRules[lRuleNumber].m_bRuleOn = TRUE;

    if (m_pASMSource)
    {
        m_pASMSource->Subscribe(unStreamNumber, lRuleNumber);
    }

    if (m_bInitialSubscriptionDone)
    {
        pStreamData->m_pRules[lRuleNumber].m_PendingAction = ACTION_ON;

#if WHAT_THE_HELL_IS_THIS_DO_XXXSMP
        ServerPacket* pPacket = 0;
        if ((pPacket = pStreamData->m_pPackets.PeekPacket()))
        {
            /* Kill any packet that isn't valid in the queue */
            UINT16 unRule = pPacket->m_uASMRuleNumber;
            if (!pStreamData->m_pRules[unRule].m_bRuleOn)
            {
                ServerPacket* pKillPacket = pStreamData->m_pPackets.GetPacket();
                m_ulPacketsOutstanding--;
                pStreamData->m_ulPacketsOutstanding--;

                pKillPacket->Release();
                GetNextPacket(unStreamNumber);
            }
        }
#endif
    }
    else
    {
        // Update delivery rate

        if (!pStreamData->m_pRules[lRuleNumber].m_bBitRateReported)
        {
            m_ulDeliveryRate += pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate;

            m_pPPM->ChangeDeliveryBandwidth(
                pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate,
                !m_bIsMulticast, pStreamData, !pStreamData->m_bGotSubscribe);
            if (pStreamData->m_pRules[lRuleNumber].m_bTimeStampDelivery)
            {
                pStreamData->m_ulVBRAvgBitRate += pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate;
            }

            if (!pStreamData->m_bGotSubscribe)
                pStreamData->m_bGotSubscribe = TRUE;

            pStreamData->m_pRules[lRuleNumber].m_bBitRateReported = TRUE;
        }
    }

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::HandleUnSubscribe(INT32 lRuleNumber,
                              UINT16 unStreamNumber)
{
    PPMStreamData* pStreamData = m_pStreamData + unStreamNumber;
    HX_ASSERT(pStreamData->m_bStreamRegistered);

    if (!pStreamData->m_bStreamRegistered 
            || lRuleNumber >= pStreamData->m_lNumRules)
        return HXR_OK;

    if (m_pASMSource)
    {
        m_pASMSource->Unsubscribe(unStreamNumber, lRuleNumber);
    }

    pStreamData->m_pRules[lRuleNumber].m_bRuleOn = FALSE;

    if (m_bInitialSubscriptionDone)
    {
        if (pStreamData->m_pRules[lRuleNumber].m_bWaitForSwitchOffFlag)
        {
            pStreamData->m_pRules[lRuleNumber].m_PendingAction = ACTION_OFF;
        }
        else
        {
            if (pStreamData->m_pRules[lRuleNumber].m_bBitRateReported)
            {
                // Update delivery rate
                m_ulDeliveryRate -= pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate;
                m_pPPM->ChangeDeliveryBandwidth((-1) *
                    (INT32)pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate,
                    !m_bIsMulticast, pStreamData);
                if (pStreamData->m_pRules[lRuleNumber].m_bTimeStampDelivery)
                {
                    pStreamData->m_ulVBRAvgBitRate -= pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate;
                }

                pStreamData->m_pRules[lRuleNumber].m_bBitRateReported = FALSE;
            }
        }
    }

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::RegisterStream(Transport* pTransport, UINT16 uStreamGroupNumber,
                             UINT16 uStreamNumber, ASMRuleBook* pRuleBook, 
                             IHXValues* pHeader)
{
    return RegisterStream( pTransport, uStreamNumber, pRuleBook, pHeader);
}

STDMETHODIMP
PPM::Session::RegisterStream(Transport* pTransport, UINT16 uStreamNumber,
                             ASMRuleBook* pRuleBook, IHXValues* pHeader)
{
    m_uNumStreamsRegistered++;

    if (m_uFirstStreamRegistered == 0xFFFF)
    {
        m_uFirstStreamRegistered = uStreamNumber;

        if(m_bIsLive)
        {
            HX_ASSERT(m_pBRDetector == NULL);
            HX_DELETE(m_pBRDetector);
            m_pBRDetector = new BRDetector();
            m_pBRDetector->SetStreamCount(m_unStreamCount);
        }
    }

    PPMStreamData* pStreamData = m_pStreamData + uStreamNumber;
    pStreamData->m_pTransport = pTransport;
    pStreamData->m_bSupportsPacketAggregation = pTransport->SupportsPacketAggregation();
    pStreamData->m_bNullSetup = pTransport->isNullSetup();
    pStreamData->m_pTransport->AddRef();
    pStreamData->m_pTimeStampCallback->m_unStreamNumber = uStreamNumber;
    pStreamData->m_bStreamRegistered = TRUE;

    // See if transport has a pause advise

    pStreamData->m_pTransport->QueryInterface(IID_IHXServerPauseAdvise,
                                              (void**) &pStreamData->m_pPauseAdvise);

    if (pHeader)
    {
        UINT32 sampleRate = 0;
        UINT32 RTPFactor = 0;
        UINT32 HXFactor = 0;
  
        pHeader->GetPropertyULONG32("SamplesPerSecond", sampleRate);
        pHeader->GetPropertyULONG32("RTPTimestampConversionFactor",
                                    RTPFactor);
        pHeader->GetPropertyULONG32("HXTimestampConversionFactor",
                                    HXFactor);

        HX_DELETE(pStreamData->m_pTSConverter);
        
        if (HXFactor && RTPFactor)
        {
            pStreamData->m_pTSConverter =
            new CHXTimestampConverter(CHXTimestampConverter::FACTORS,
                                                     HXFactor,
                                                     RTPFactor);
        }
        else if (sampleRate)
        {
            pStreamData->m_pTSConverter =
            new CHXTimestampConverter(CHXTimestampConverter::SAMPLES,
                                                      sampleRate);
        }
    }

    if (m_bIsLive)
    {
        m_pBRDetector->OnStreamHeader(pHeader);

        if (pStreamData->m_pTransport->isReflector())
        {
            m_bIsReflector = TRUE; 
            HX_VERIFY(SUCCEEDED(pStreamData->m_pTransport->getRTCPRule(m_unRTCPRule)));
            if (!m_pRTPInfoSynch)
            {
                RTPInfoSynch* pInfoSynch = new RTPInfoSynch;
                pInfoSynch->QueryInterface(IID_IHXRTPInfoSynch, (void**)& m_pRTPInfoSynch);
                m_pRTPInfoSynch->InitSynch(m_unStreamCount);
            }

            HX_ASSERT(m_pRTPInfoSynch);

            if (m_pRTPInfoSynch)
            {
                IHXBuffer* pMimeType = NULL;

                if (pStreamData->m_pTSConverter)
                {
                    m_pRTPInfoSynch->SetTSConverter(pStreamData->m_pTSConverter->
                                                    GetConversionFactors(),
                                                    uStreamNumber);
                }

                if (pHeader && (SUCCEEDED(pHeader->
                                          GetPropertyCString ("MimeType", pMimeType)) &&
                                pMimeType && pMimeType->GetBuffer()))
                {
                    if (RTSPMEDIA_TYPE_AUDIO ==
                        SDPMapMimeToMediaType ((const char*)pMimeType->GetBuffer()))
                    {
                        m_unRTPSynchStream = uStreamNumber;
                    }
                }

                HX_RELEASE(pMimeType);

                m_pRTPInfoSynch->RTPSynch(m_unRTPSynchStream);
            }
        }
    }

    UINT32 ulTemp = 0;

    if (pHeader)
    {
        pHeader->GetPropertyULONG32("AvgBitRate", ulTemp);
        pStreamData->m_ulAvgBitRate = ulTemp;
        ulTemp = 0;

        pHeader->GetPropertyULONG32("MaxBitRate", ulTemp);
        pStreamData->m_ulMaxBitRate = ulTemp;
        ulTemp = 0;

        pHeader->GetPropertyULONG32("MinBitRate", ulTemp);
        pStreamData->m_ulMinBitRate = ulTemp;
        ulTemp = 0;

        if (SUCCEEDED(pHeader->GetPropertyULONG32("ServerPreroll", ulTemp)))
        {
            pStreamData->m_ulPreroll = ulTemp;
            ulTemp = 0;
        }
        else
        {
            pHeader->GetPropertyULONG32("Preroll", ulTemp);
            pStreamData->m_ulPreroll = ulTemp;
            ulTemp = 0;
        }
    }

    if (!pRuleBook)
    {
        pHeader->GetPropertyULONG32("AvgBitRate", ulTemp);

        pStreamData->m_pRules =
            new PPMStreamData::RuleInfo[1];

        pStreamData->m_lNumRules = 1;

        pStreamData->m_pExpectedRuleSeqNoArray = 
                                   new UINT16[pStreamData->m_lNumRules];

        pStreamData->m_pRules[0].m_ulPriority = 5;
        pStreamData->m_pRules[0].m_ulAvgBitRate = ulTemp;
    }
    else
    {
        UINT16 i;
        pStreamData->m_pRules =
            new PPMStreamData::RuleInfo[pRuleBook->GetNumRules()];

        pStreamData->m_lNumRules = pRuleBook->GetNumRules();

        pStreamData->m_pExpectedRuleSeqNoArray = 
                                   new UINT16[pStreamData->m_lNumRules];


        for (i = 0; i < pRuleBook->GetNumRules(); i++)
        {
            /*
             * If this is a live stream, set the pending action to SYNC so
             * that the stream will start at the beginning of a keyframe
             */

            if (m_bIsLive)
            {
                pStreamData->m_pRules[i].m_bSyncOk = FALSE;
                pStreamData->m_pRules[i].m_PendingAction = SYNC;
            }

            IHXValues* pRuleProps;
            pRuleBook->GetProperties(i, pRuleProps);

            IHXBuffer* pBuffer = 0;
            pRuleProps->GetPropertyCString("Priority", pBuffer);
            if (pBuffer)
            {
                pStreamData->m_pRules[i].m_ulPriority =
                    atoi((const char *)pBuffer->GetBuffer());
                pBuffer->Release();
            }

            pBuffer = 0;
            pRuleProps->GetPropertyCString("TimeStampDelivery", pBuffer);
            if (pBuffer)
            {
                pStreamData->m_pRules[i].m_bTimeStampDelivery =
                    (pBuffer->GetBuffer()[0] == 'T') ||
                    (pBuffer->GetBuffer()[0] == 't');
                pBuffer->Release();
            }

            pBuffer = 0;
            pRuleProps->GetPropertyCString("WaitForSwitchOff", pBuffer);
            if (pBuffer)
            {
                pStreamData->m_pRules[i].m_bWaitForSwitchOffFlag = TRUE;
                    (pBuffer->GetBuffer()[0] == 'F') ||
                    (pBuffer->GetBuffer()[0] == 'f');
                pBuffer->Release();
            }

            pBuffer = 0;
            pRuleProps->GetPropertyCString("OnDepend", pBuffer);
            if (pBuffer)
            {
                UINT16 pOnDeps[4096] = { 0 };
                UINT16 ulNum = 0;
                const UINT8*  pTemp = pBuffer->GetBuffer();

                while (*pTemp)
                {
                    if (*pTemp == ',')
                    {
                        ulNum++;
                        pOnDeps[ulNum] = 0;
                    }
                    else
                    {
                        if ((*pTemp >= '0') && (*pTemp <= '9'))
                        {
                            pOnDeps[ulNum] *= 10;
                            pOnDeps[ulNum] += *pTemp - '0';
                        }
                    }

                    pTemp++;
                }
                ulNum++;

                pStreamData->m_pRules[i].m_pOnDepends = new UINT16[ulNum + 1];
                memcpy(pStreamData->m_pRules[i].m_pOnDepends, pOnDeps,
                    (ulNum + 1) * sizeof(UINT16));

                pStreamData->m_pRules[i].m_pOnDepends[ulNum] = 0xffff;

                pBuffer->Release();
            }

            pBuffer = 0;
            pRuleProps->GetPropertyCString("OffDepend", pBuffer);
            if (pBuffer)
            {
                UINT16 pOffDeps[4096] = { 0 };
                UINT16 ulNum = 0;
                const UINT8*  pTemp = pBuffer->GetBuffer();

                while (*pTemp)
                {
                    if (*pTemp == ',')
                    {
                        ulNum++;
                        pOffDeps[ulNum] = 0;
                    }
                    else
                    {
                        if ((*pTemp >= '0') && (*pTemp <= '9'))
                        {
                            pOffDeps[ulNum] *= 10;
                            pOffDeps[ulNum] += *pTemp - '0';
                        }
                    }

                    pTemp++;
                }
                ulNum++;

                pStreamData->m_pRules[i].m_pOffDepends = new UINT16[ulNum + 1];
                memcpy(pStreamData->m_pRules[i].m_pOffDepends, pOffDeps,
                    (ulNum + 1) * sizeof(UINT16));

                pStreamData->m_pRules[i].m_pOffDepends[ulNum] = 0xffff;

                pBuffer->Release();
            }

            pBuffer = 0;
            pRuleProps->GetPropertyCString("AverageBandwidth", pBuffer);

            if (pBuffer)
            {
                pStreamData->m_pRules[i].m_ulAvgBitRate =
                atoi((const char *)pBuffer->GetBuffer());
                pBuffer->Release();
            }

            pBuffer = 0;
            pRuleProps->GetPropertyCString("MaximumBandwidth", pBuffer);

            if (pBuffer)
            {
                pStreamData->m_pRules[i].m_ulMaxBitRate =
                atoi((const char *)pBuffer->GetBuffer());
                pBuffer->Release();
            }

            if (pStreamData->m_pRules[i].m_bTimeStampDelivery)
            {
                pStreamData->m_pRules[i].m_pBWCalculator =
                    new BWCalculator(m_pProc, pStreamData->m_bNullSetup);
                pStreamData->m_pRules[i].m_pBWCalculator->AddRef();
            }

            pBuffer = 0;
            pRuleProps->GetPropertyCString("DropByN", pBuffer);
            if (pBuffer)
            {
                if ((pBuffer->GetBuffer()[0] == 'T') ||
                    (pBuffer->GetBuffer()[0] == 't'))
                {
                        // XXXSMP What to do?
                }
                pBuffer->Release();
            }

            pRuleProps->Release();
        }
    }

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::GetSequenceNumber(UINT16 uStreamNumber,
    UINT16& uSequenceNumber)
{
    PPMStreamData* pStreamData = m_pStreamData + uStreamNumber;
    HX_ASSERT(pStreamData && pStreamData->m_bStreamRegistered);
    if (pStreamData)
    {
        uSequenceNumber = pStreamData->m_unSequenceNumber;
        return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
PPM::Session::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPSinkPackets*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkPackets))
    {
        AddRef();
        *ppvObj = (IHXPSinkPackets*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkInfo))
    {
        AddRef();
        *ppvObj = (IHXPSinkInfo*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPacketResend))
    {
        AddRef();
        *ppvObj = (IHXPacketResend*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXWouldBlockResponse))
    {
        AddRef();
        *ppvObj = (IHXWouldBlockResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAccurateClock))
    {
        AddRef();
        *ppvObj = (IHXAccurateClock*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSLinkCharSetup))
    {
        AddRef();
        *ppvObj = (IHXQoSLinkCharSetup*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXStreamAdaptationSetup))
    {
        AddRef();
        *ppvObj = (IHXStreamAdaptationSetup*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
PPM::Session::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
PPM::Session::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
PPM::Session::PacketReady(HX_RESULT              ulStatus,
                         IHXPacket*             pPacket)
{
    // update the PlayTime stat if not paused
    // (for some reason, we keep streaming when live pauses.)

    if (!m_bPaused) UpdatePlayTime();

    if (ulStatus == HXR_OK && m_pConvertShim)
    {
        HX_RELEASE(m_pConvertingPacket);
        m_pConvertingPacket = pPacket;
        m_pConvertingPacket->AddRef();
        m_pConvertShim->ConvertData(pPacket);
    }
    else
    {
        if (m_bIsLive)
        {
            if(!m_bPktBufQProcessed)
            {
                if(m_bQueueDebugLog == TRUE)
                {
                    UINT32 tCur = HX_GET_BETTERTICKCOUNT();

                    // Print the debug message first time (m_ulQueueDebugTS is zero)
                    // and every 5 seconds (RSD_QUEUE_STATUS_LOG_INTERVAL) thereafter...
                    if(tCur - m_ulQueueDebugTS > RSD_QUEUE_STATUS_LOG_INTERVAL && m_pRSDPacketQueue)
                    {
                        char szTime[128];
                        Timeval tNow = m_pProc->pc->engine->now;
                        struct tm localTime;
                        time_t tTime = 0;
                        hx_localtime_r(&tNow.tv_sec, &localTime);
                        strftime(szTime, 128, "%d-%b-%y %H:%M:%S", &localTime);
                        
                        fprintf(stderr, "%s.%3d S=%s QT=%f QPN=%d\n",
                                    szTime, tNow.tv_usec/1000,
                                    m_pPlayerSessionId->GetBuffer(),
                                    (float)(m_pRSDPacketQueue->GetQueueDuration())/1000.0,
                                    m_pRSDPacketQueue->GetQueuePacketNumber());
                        fflush(0);
                        m_ulQueueDebugTS = tCur;
                    }
                }
                ProcessLivePacket(ulStatus, pPacket);
            }
            else
            {
                return LiveSessionPacketReady(ulStatus, pPacket);
            }
        }
        else
        {
            return SessionPacketReady(ulStatus, pPacket);
        }
    }
    return HXR_OK;
}

/* we have two queues here. the queue from bcastgateway and our own queue.
   In the beginning we process the queue from bcastgateway, the packets arrive from
   packetReady at the period are put into our own queue.

   After we finish the queue from bcastgateway, i.e the seqno of the packet meets that
   of the first item of our queue, we turn to process our own queue.

   After we finish our own queue, we will skip this func.

   The ratio we send out packets in this period is tricky, we need more tests to decide
   this number.
*/

HX_RESULT PPM::Session::ProcessLivePacket(HX_RESULT ulStatus, IHXPacket* pPacket)
{
    UINT32 ulQSize = 0;
    UINT32 ulLoopIterations = 0;
    IHXBroadcastDistPktExt* pPacketExt = NULL;
    HX_RESULT rc = HXR_OK;
    IHXLivePacketBufferQueue* pQueue = NULL;

    if(!m_bCheckLiveRSD)
    {
        pPacket->QueryInterface(IID_IHXBroadcastDistPktExt,
                                (void **)&pPacketExt);

        if (pPacketExt)
        {
            m_bIsLLL = pPacketExt->SupportsLowLatency();
        }

        if(m_bIsLLL || ulStatus != HXR_OK ||  pPacketExt == NULL || pPacket->IsLost()) 
        {
           // rsd is disabled for LLL
           HX_RELEASE(pPacketExt);
           m_bPktBufQProcessed = TRUE;
           return LiveSessionPacketReady(ulStatus, pPacket);
        }
        HX_RELEASE(pPacketExt);

        for(UINT16 i = 0; i < m_unStreamCount; i++)
        {
            PPMStreamData* pSD = m_pStreamData + i;
            if (!pSD->m_bStreamRegistered)
            {
                continue;
            }

            for(UINT16 j = 0; j < pSD->m_lNumRules; j++)
            {
                if(pSD->m_pRules[j].m_bRuleOn)
                {
                     m_pPacketBufferProvider->GetPacketBufferQueue(i, j, pQueue);
                }

                if(pQueue)
                {
                    m_unKeyframeStream = i;
                    m_unKeyframeRule = j;
                    break; // break out of rule loop
                }
            }

            if(pQueue)
            {
                 break; // break out of stream loop
            }
        }
       
        if (pQueue == NULL)
        {
            //Something is wrong here, we better not process the buffer queue
            m_bPktBufQProcessed = TRUE;
            return LiveSessionPacketReady(ulStatus, pPacket);
        }
        m_bCheckLiveRSD = TRUE;

        // okay here we are sure that we have a Queue to process, it is time to get all the 
        // the config variables.  
        // this is moved from the ctor to here, so the performance of non-rsd live is not
        // impacted by parsing all these variables.
        GetAllRSDConfigure();

        // rsd is disabled when cpu usage > the threshold(default 50%) 
        if(m_lCPUUsage > m_lCPUThresholdForRSD || m_lCPUUsage < 0)
        {
            if(HXR_OK != m_pProc->pc->registry->SetInt(
                SERVER_LIVERSD_CPUTHRESHOLD_REACHED, 1, m_pProc))
            {
                m_pProc->pc->registry->AddInt(SERVER_LIVERSD_CPUTHRESHOLD_REACHED, 1, m_pProc);
            }

            if(m_bEnableLiveRSDLog)
            {
                fprintf(stderr, "RSDLive: Live RSD is disabled because CPU Usage(%d) passes "
                        "the threshold(%d)\n", m_lCPUUsage, m_lCPUThresholdForRSD);
                fflush(0);
            }
            HX_RELEASE(pQueue);
            m_bPktBufQProcessed = TRUE;
            return LiveSessionPacketReady(ulStatus, pPacket);
        }

        if(HXR_OK != m_pProc->pc->registry->SetInt(
            SERVER_LIVERSD_CPUTHRESHOLD_REACHED, 0, m_pProc))
        {
            m_pProc->pc->registry->AddInt(SERVER_LIVERSD_CPUTHRESHOLD_REACHED, 0, m_pProc);
        }

        m_bIsWireline = DoesUseWirelineLogic();

        HX_ASSERT(m_pRSDPacketQueue == NULL);
        HX_DELETE(m_pRSDPacketQueue);
        m_pRSDPacketQueue = new CRSDPacketQueue(32);
        m_pRSDPacketQueue->AddPacketBufferQueue(pQueue);
        HX_RELEASE(pQueue);

        CalculatePreDataAmount();

        if(m_bIsWireline)
        {
            if(m_bEnableLiveRSDLog)
            {
                printf("wire line logic\n");
                fflush(0);
            }
        }
        else
        {
            m_bIsMobileCBR = IsMobileCBR();

            if(m_bEnableLiveRSDLog)
            {
                printf("mobile %s logic\n", m_bIsMobileCBR ? "CBR" : "TSD");
                fflush(0);
            }
        }

#ifdef RSD_LIVE_DEBUG
        m_uPacketListStart = HX_GET_BETTERTICKCOUNT();
#endif

        if(m_pSessionStats2)
        {
            m_pSessionStats2->SetFirstPacketTime(HX_GET_BETTERTICKCOUNT());
        }
    }
   
    m_pRSDPacketQueue->AddPacket(pPacket);
    if(m_pRSDPacketQueue->GetQueueDuration() > RSD_MAX_QUEUE_LENGTH_IN_SECOND*1000)
    {
        //if the queue length accumulates over 60 seconds, which mean the bandwidth or
        //network condition are not good for RSD oversending, we should clear up RSD(by
        //calling Pause) and do it the legacy way.
        Pause(TRUE);
        return HXR_OK;
    }
    pPacket = NULL;

    if(m_bIsWireline)
    {
        return WirelineLivePacketReady();
    }
    else if(m_bIsMobileCBR)
    {
        return MobileLiveCBRPacketReady();
    }
    else
    {
        return MobileLiveTSDPacketReady();
    }

    return HXR_OK;
}

BOOL PPM::Session::IsMobileCBR()
{
    StreamType type = STRM_TSD;

    for(UINT16 i = 0; i < m_unStreamCount; i++)
    {
        PPMStreamData* pSD = m_pStreamData + i;
        if (!pSD->m_bStreamRegistered)
        {
            continue;
        }

        for(UINT16 j = 0; j < pSD->m_lNumRules; j++)
        {
            if(pSD->m_pRules[j].m_bRuleOn)
            {
                if(m_pBRDetector->GetType(i, j, type) == HXR_OK)
                {
                    //if one of the rules subscribed is TSD, then TSD is picked.
                    if(type == STRM_TSD)
                    {
                        return FALSE;
                    }
                }
            }
        }
    }

    return  TRUE;
}

HX_RESULT PPM::Session::MobileLiveCBRPacketReady()
{
    if(m_bFirstPacket)
    {
        m_bFirstPacket = FALSE;
        m_ulInitialQDuration = m_pRSDPacketQueue->GetQueueDuration();

        HX_ASSERT(m_pServerTBF == NULL);
        HX_DELETE(m_pServerTBF);
        m_pServerTBF = new CServerTBF;
        m_pServerTBF->Init(m_pProc->pc->server_context);
        m_pServerTBF->SetMinTokenCeiling(m_lMinTokenBucketCeiling);

        ResetAdjustingState();

        HX_ASSERT(m_pCBRCB == NULL);
	if (m_RSDCBhandle)
	{
	    m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
	    m_RSDCBhandle = 0;
	}
        HX_RELEASE(m_pCBRCB);
        m_pCBRCB = new CBRCallback();
        m_pCBRCB->m_pSession = this;
        m_pCBRCB->AddRef();
        m_RSDCBhandle = m_pPPM->m_pProc->pc->engine->ischedule.enter(
                m_pPPM->m_pProc->pc->engine->now + Timeval(0, 500*1000), m_pCBRCB);
    }

    m_ulPacketSent++;
    if (m_pServerTBF && m_ulPacketSent%2 == 0)
    {
        m_pServerTBF->UpdateTokens();
    }
    
    return SendLiveCBRPacket();
}


HX_RESULT PPM::Session::SendLiveCBRPacket()
{
    HX_RESULT rc = HXR_OK;
    IHXPacket* pPacket = NULL;
    UINT32 ulLoopIterations = 0;

    if(m_bAdjustingQLength)
    {
        if(m_bOversend)
        {
            //queue grow enough
            if(m_pRSDPacketQueue->GetQueueDuration() > m_ulInitialQDuration)
            {
#ifdef RSD_LIVE_DEBUG
                printf("oversending adjusting goal reached\n");fflush(0);
#endif
                ResetAdjustingState();
            }
        }
        else //undersend
        {
            //queue shrink enough
            if(m_pRSDPacketQueue->GetQueueDuration() <  m_ulInitialQDuration)
            {
#ifdef RSD_LIVE_DEBUG
                printf("undersending adjusting goal reached\n");fflush(0);
#endif
                ResetAdjustingState();
            }
        }
    }
    else
    {
        Timeval tDiff = m_pPPM->m_pProc->pc->engine->now - m_tCBRCheckQStartingTime;
        //check every second
        if(tDiff.tv_sec > 1)
        {
            UINT32 ulQueueDuration = m_pRSDPacketQueue->GetQueueDuration();

            // if the queue grows by 10%, then we are undersending, and the player will rebuffer 
            // unless we increase the sending rate.
            if(ulQueueDuration > m_ulInitialQDuration*11/10)
            {
#ifdef RSD_LIVE_DEBUG
                printf("undersending, int q duration: %d, cur q duration: %d\n", m_ulInitialQDuration, ulQueueDuration);fflush(0);
#endif
                //queue growing, we are undersending
                m_pServerTBF->SetBandwidth(m_ulDeliveryRate*11/10);
                m_bAdjustingQLength = TRUE;
                m_bOversend = FALSE;

            }
            // if the queue shrinks by 10%, then we are oversending, and the player risks buffer overflowing 
            // unless we decrease the sending rate.
            else if(ulQueueDuration < m_ulInitialQDuration*9/10)
            {
#ifdef RSD_LIVE_DEBUG
                printf("oversending, int q duration: %d, cur q duration: %d\n", m_ulInitialQDuration, ulQueueDuration);fflush(0);
#endif
                //queue shrinking, we are oversending
                m_pServerTBF->SetBandwidth(m_ulDeliveryRate*9/10);
                m_bAdjustingQLength = TRUE;
                m_bOversend = TRUE;
            }
            else
            {
                //reset the timer
                m_tCBRCheckQStartingTime = m_pPPM->m_pProc->pc->engine->now;
            }
        }
    }

    while(1)
    {
        if (m_ulWouldBlocking || !m_pServerTBF || m_pServerTBF->GetTokenCount() < 0)
        {
            return HXR_OK;
        }

        rc = m_pRSDPacketQueue->GetPacket(pPacket);
        if(FAILED(rc))
        {
            goto cbrfailed;
        }
    
        rc = LiveSessionPacketReady(HXR_OK, pPacket); 
        HX_RELEASE(pPacket);

        ulLoopIterations++;
        if (ulLoopIterations >= m_ulIteration || rc == HXR_BLOCKED)
        {
            return HXR_OK;
        }
    }
    HX_ASSERT(0);
    return rc;

cbrfailed:
    HX_RELEASE(pPacket);
    m_bPktBufQProcessed = TRUE;

    if(m_RSDCBhandle)
    {
        m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
        m_RSDCBhandle = 0;
    }
    HX_DELETE(m_pRSDPacketQueue);
    return HXR_FAIL;
}

void PPM::Session::ResetAdjustingState()
{
    m_pServerTBF->SetBandwidth(m_ulDeliveryRate);
    m_bAdjustingQLength = FALSE;
    m_tCBRCheckQStartingTime = m_pPPM->m_pProc->pc->engine->now;
}

HX_RESULT PPM::Session::MobileLiveTSDPacketReady()
{
    HX_RESULT rc = HXR_OK;

    if(m_bFirstPacket)
    {
        m_bFirstPacket = FALSE;

        if(m_bIsReflector) 
        {
            //streams from rtp live, we need to send out rtcp packets and 
            // master sync packet asap regardless of timestamp
            BOOL bDone = FALSE;
            UINT32 unRule = 0xffff;
            UINT32 unStream = 0xffff;
            IHXPacket* pPacket = NULL;
            
            while(!bDone)
            {
                rc = m_pRSDPacketQueue->PeekPacket(pPacket);
                if(FAILED(rc))
                {
                    m_bPktBufQProcessed = TRUE;
                    if(m_RSDCBhandle)
                    {
                        m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
                        m_RSDCBhandle = 0;
                    }
                    HX_DELETE(m_pRSDPacketQueue);
                    return HXR_FAIL;
                }
                unRule = pPacket->GetASMRuleNumber();
                unStream = pPacket->GetStreamNumber();
                HX_RELEASE(pPacket);

                // if m_bIsReflector is true, there should be one rtcp packet from
                // each stream, then an audio(master sync) packet before the video keyframe
                // packet.
                if(unRule == m_unRTCPRule)
                {
                    m_pRSDPacketQueue->GetPacket(pPacket);
                    LiveSessionPacketReady(HXR_OK, pPacket);
                    HX_RELEASE(pPacket);
                    continue;
                }

                if(unStream != m_unKeyframeStream)
                {
                    m_pRSDPacketQueue->GetPacket(pPacket);
                    LiveSessionPacketReady(HXR_OK, pPacket);
                    HX_RELEASE(pPacket);
                    continue;
                }
                else
                {
                   bDone = TRUE; 
                }
            }
            HX_RELEASE(pPacket);
        }

        PPMStreamData* pSD = m_pStreamData + m_unKeyframeStream ;
        BOOL bHasMaxBitrate = (pSD->m_ulMaxBitRate != pSD->m_ulAvgBitRate 
                                        && pSD->m_ulMaxBitRate > 0);

        if(bHasMaxBitrate)
        {
            UINT32 ulMaxBR = 0;
            HX_ASSERT(m_pServerTBF == NULL);
            HX_DELETE(m_pServerTBF);
            m_pServerTBF = new CServerTBF;
            m_pServerTBF->Init(m_pProc->pc->server_context);
            m_pServerTBF->SetMinTokenCeiling(m_lMinTokenBucketCeiling);
            
            for(int i = 0; i < m_unStreamCount; i++)
            {
                PPMStreamData* psd = m_pStreamData + i;
                ulMaxBR += psd->m_ulMaxBitRate ?
                                psd->m_ulMaxBitRate : pSD->m_ulAvgBitRate;
            }
            m_pServerTBF->SetBandwidth(ulMaxBR);
        }

        rc = m_pRSDPacketQueue->GetHeadPacketTimeStamp(m_ulFirstPacketTS);
        if(FAILED(rc))
        {
            m_bPktBufQProcessed = TRUE;
            if(m_RSDCBhandle)
            {
                m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
                m_RSDCBhandle = 0;
            }
            HX_DELETE(m_pRSDPacketQueue);
            return HXR_FAIL;
        }

        m_tTimeLineStart = m_pPPM->m_pProc->pc->engine->now;

        HX_ASSERT(m_pTSDCB == NULL);
        HX_RELEASE(m_pTSDCB);
        m_pTSDCB = new TSDCallback();
        m_pTSDCB->m_pSession = this;
        m_pTSDCB->AddRef();

        rc = SendAndScheduleTSDPacket();
        return rc;
    }
    else
    {
        if(m_pServerTBF)
        {
            m_ulPacketSent++;
            if(m_ulPacketSent%10 == 0)
            {
                m_pServerTBF->UpdateTokens();
            }
        }
        // debug output shows that calling SendAndScheduleTSDPacket here
        // seldom sends out packets, hence it it commented out here, 
        // constrasted to the sod.  This will be investigated deeper 
        // in the following performance tuning.
        //rc = SendAndScheduleTSDPacket();
    }

    return rc;
}

HX_RESULT PPM::Session::SendAndScheduleTSDPacket()
{
    IHXPacket* pPacket = NULL;
    HX_RESULT rc = HXR_OK;
    UINT32 ulHeadPacketTS = 0;
    Timeval tTimeStamp;
    ULONG32 ulPacketTimeStamp = 0;
    BOOL bPacketSent = FALSE;
    BOOL bContinue = TRUE; 
    if(m_pServerTBF && m_pServerTBF->GetTokenCount() <= 0)
    {   
#ifdef RSD_LIVE_DEBUG
        printf("SendAndScheduleTSDPacket doesn't send packets because no tokens available\n");
#endif
        bContinue = FALSE;

        // no tokens available, check 10 millisecond later.
        tTimeStamp.tv_sec  = 0;
        tTimeStamp.tv_usec = 10 * 1000;
        if(m_RSDCBhandle == 0)
        {
            m_RSDCBhandle = m_pPPM->m_pProc->pc->engine->ischedule.enter(
                            m_pPPM->m_pProc->pc->engine->now + tTimeStamp, m_pTSDCB);
        }
        return HXR_BLOCKED;

    } 

    while(bContinue)
    {
        //if tcp wouldblock, we don't reschedule callbacks. WoulblockCleared
        //will cause a restart.
        if(m_ulWouldBlocking != 0)
        {
            return HXR_OK;
        }
        rc = m_pRSDPacketQueue->GetHeadPacketTimeStamp(ulHeadPacketTS);
        if(FAILED(rc))
        {
            goto tsdfailed;
        }
    
        if(ulHeadPacketTS >= m_ulFirstPacketTS)
        {
            ulPacketTimeStamp = ulHeadPacketTS - m_ulFirstPacketTS;
        }
        else
        {
            ulPacketTimeStamp = 0;
#ifdef RSD_LIVE_DEBUG
            m_pRSDPacketQueue->PeekPacket(pPacket);
            IHXBroadcastDistPktExt* pPacketExt = NULL;
            pPacket->QueryInterface(IID_IHXBroadcastDistPktExt,
                        (void **)&pPacketExt);

            UINT32 ulStrmSeqNo= 0;
            if(pPacketExt)
            {
                ulStrmSeqNo = pPacketExt->GetStreamSeqNo();
                pPacketExt->Release();
            } 
            printf("backward growth timeline kf: %d, ts: %u, strm#: %d, rule: %d, strmseq: %d\n", 
            pPacket->GetASMFlags() & HX_ASM_SWITCH_ON, pPacket->GetTime(),
            pPacket->GetStreamNumber(), pPacket->GetASMRuleNumber(), ulStrmSeqNo);
            HX_RELEASE(pPacket);
#endif
        }
    
        tTimeStamp.tv_sec  = ulPacketTimeStamp / 1000;
        tTimeStamp.tv_usec = (ulPacketTimeStamp % 1000) * 1000;

        if((m_tTimeLineStart + tTimeStamp) <= m_pPPM->m_pProc->pc->engine->now)
        {
            rc = m_pRSDPacketQueue->GetPacket(pPacket);
            if(FAILED(rc))
            {
                goto tsdfailed;
            }
    
            rc = LiveSessionPacketReady(HXR_OK, pPacket); 
            HX_RELEASE(pPacket);
            bPacketSent = TRUE;
            if(rc == HXR_BLOCKED)
            {
                bContinue = FALSE;
            }
            else if(FAILED(rc))
            {
                goto tsdfailed;
            }
        }
        else
        {
            bContinue = FALSE;
        }
    }

    if(bPacketSent || m_RSDCBhandle == 0)
    {
        //head packet modified, we need to reschedule the callback
        if(m_RSDCBhandle)
        {
            m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
        }
        // schedule the head packet for 0.01 second late, because of the resolution 
        // of the timer, if we schedule the packet at the exact time, sometimes the 
        // callback is evoked a litter bit early, and no packets are sent in the callback.
        tTimeStamp.tv_usec += 10*1000;
        m_RSDCBhandle = m_pPPM->m_pProc->pc->engine->ischedule.enter(
                m_tTimeLineStart + tTimeStamp, m_pTSDCB);
    }

    return HXR_OK;

tsdfailed:
    HX_RELEASE(pPacket);
    m_bPktBufQProcessed = TRUE;
    if(m_RSDCBhandle)
    {
        m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
        m_RSDCBhandle = 0;
    }
    HX_DELETE(m_pRSDPacketQueue);
    return HXR_FAIL;
}

STDMETHODIMP PPM::Session::TSDCallback::Func()
{
    m_pSession->m_RSDCBhandle = 0;
    return m_pSession->SendAndScheduleTSDPacket();
}

STDMETHODIMP PPM::Session::CBRCallback::Func()
{
    HX_RESULT hxr;
    m_pSession->m_RSDCBhandle = 0;
    hxr = m_pSession->SendLiveCBRPacket();
    if(SUCCEEDED(hxr))
    {
	/* XXX -- aak
	 * SendLiveCBRPacket() results in a call to PPM::Session::Pause()
	 * which is called when the write to the socket blocks (EAGAIN).
	 * with Pause() the m_pCBRCB is released and set to NULL, resulting in
	 * a CA without the following conditional statement. so for now this
	 * is the defensive fix for pr# 159570.
	 */
	if (m_pSession->m_pCBRCB)
        m_pSession->m_RSDCBhandle = m_pSession->m_pProc->pc->engine->ischedule.enter(
            m_pSession->m_pProc->pc->engine->now + Timeval(0, 500*1000), m_pSession->m_pCBRCB);
    }
    return HXR_OK;
}

HX_RESULT PPM::Session::WirelineLivePacketReady()
{
    UINT32 ulQSize = 0;
    UINT32 ulLoopIterations = 0;
    HX_RESULT rc = HXR_OK;

    m_ulPacketSent++;
    // m_pServerTBF will only be used when processing the Qed packets
    // when done, we will transmit the packets as they comes from the 
    // encoder and no flow control is provided.
    if(!m_pServerTBF)
    {
        m_pServerTBF = new CServerTBF;
        m_pServerTBF->Init(m_pProc->pc->server_context);
        m_pServerTBF->SetMinTokenCeiling(m_lMinTokenBucketCeiling);
        CalculateInitialSendingRate();
    }
    if(m_pServerTBF)
    {
        //time to update tokens 
        // choosing 10 here is arbitary, ti should be based on the bitrate of the
        // content. 
        // XXXJJ do more research on it.
        if(m_ulPacketSent%10 == 0)
        {
            m_pServerTBF->UpdateTokens();
        }

        if(m_pServerTBF->GetTokenCount() <= 0)
        {
#ifdef RSD_LIVE_DEBUG
            printf("PPM::ProcessLivePacket no tokens, can't send any packets.\n"); 
#endif
            return HXR_OK;
        }
    }

    while(m_ulWouldBlocking == 0 && m_pRSDPacketQueue)
    {
        IHXPacket* pQPacket = NULL;
        rc = m_pRSDPacketQueue->GetPacket(pQPacket); ;

        if(!SUCCEEDED(rc))
        {
            //no more packet in the queue and our own list
#ifdef RSD_LIVE_DEBUG
             printf("PPM::Sess(%p) Done processing PBQ, timespan : %d ms\n", this, 
                                    HX_GET_BETTERTICKCOUNT() - m_uPacketListStart);
             //((CPacketBufferQueue*)m_pPacketBufferQueue)->Dump();        
#endif

            HX_DELETE(m_pRSDPacketQueue);
            if(m_RSDCBhandle)
            {
                m_pPPM->m_pProc->pc->engine->ischedule.remove(m_RSDCBhandle);
                m_RSDCBhandle = 0;
            }
            m_bPktBufQProcessed = TRUE;
            return HXR_OK;
        }

        rc = LiveSessionPacketReady(HXR_OK, pQPacket);
        pQPacket->Release();

        ulLoopIterations++;
        if (ulLoopIterations >= m_ulIteration || rc == HXR_BLOCKED)
        {
#ifdef RSD_LIVE_DEBUG
            printf("PPM::ProcessLivePacket iter = %d, token = %d\n", 
                ulLoopIterations, m_pServerTBF ? m_pServerTBF->GetTokenCount() : 0);
            fflush(0);
#endif
            return HXR_OK;
        }
    }

    return HXR_OK;
}

HX_RESULT
PPM::Session::LiveSessionPacketReady(HX_RESULT ulStatus,
                                     IHXPacket* pPacket)
{
    HX_ASSERT(m_bIsLive);
    HX_ASSERT(pPacket);

    if ((ulStatus != HXR_OK) || (pPacket == NULL))
    {
        return HXR_OK;
    }

    if (m_bIsDone)
    {
        return HXR_UNEXPECTED;
    }

    PPMStreamData*      pSD;
    ServerPacket*       pServerPacket = NULL;
    UINT16              uStreamNumber;
    UINT16              unRule;
    UINT8               ucFlags;
    UINT8               i = 0;
    BOOL                bStreamDone = FALSE;
    BOOL                bIsSynched = FALSE;
    HX_RESULT           rc = HXR_OK;
    IHXBroadcastDistPktExt* pPacketExt = NULL;

    UINT16              unNewSeq;

    if (pPacket->IsLost())
    {
        uStreamNumber = pPacket->GetStreamNumber();
        unRule = 0;
        ucFlags = 0;
        pSD = m_pStreamData + uStreamNumber;
        pSD->m_bPacketRequested = FALSE;
    }
    else
    {
        uStreamNumber = pPacket->GetStreamNumber();
        unRule = pPacket->GetASMRuleNumber();
        ucFlags = pPacket->GetASMFlags();
        pSD = m_pStreamData + uStreamNumber;
        pSD->m_bPacketRequested = FALSE;
    }

    DPRINTF(0x02000000, ("Live Session %p: sent packet. stream: %d time %ld "
                         "asm rule: %d\n", this, uStreamNumber,
                         pPacket->GetTime(), unRule));

    if (pSD->m_bStreamDone)
    {
        /*
         * We may get PacketReady calls for pending GetPacket request on
         * other streams. This should ONLY happen when there is an end time
         * associated with this source (refer to the next if condition).
         */
        HX_ASSERT(!m_bIsLive && m_uEndPoint > 0);
        return HXR_FAIL;
    }

    //
    // the rule is outside of our rulebook so the packet is bad
    //
    if (unRule >= pSD->m_lNumRules)
    {
        DPRINTF(0x02000000, ("Live Session %p: dropped packet with bad rule. stream:"
                             " %d time %ld asm rule: %d\n", this, uStreamNumber,
                             pPacket->GetTime(), unRule));
        return HXR_FAIL;
    }

    if (m_bIsLLL)
    {
        pPacket->QueryInterface(IID_IHXBroadcastDistPktExt, (void **)&pPacketExt);
        HX_ASSERT(pPacketExt);
    }

    if (pPacket->IsLost())
    {
        goto SkipAllASMProcessing;
    }

    /*
     * Synch RTPInfo data:
     * see: protocol/transport/rtp/include/rtpinfosync.h
     */
    if (m_pRTPInfoSynch)
    {
        HX_VERIFY(SUCCEEDED(m_pRTPInfoSynch->IsStreamSynched(uStreamNumber,
                                                             bIsSynched)));
        if (!bIsSynched)
        {
            Transport* pTransport = pSD->m_pTransport;
            HX_ASSERT(pTransport->isReflector());
            
            HX_VERIFY(SUCCEEDED(pTransport->getRTCPRule(m_unRTCPRule)));
            
            IHXBuffer* pPktBuf = pPacket->GetBuffer();
            if (unRule == m_unRTCPRule)
            {
                HX_VERIFY(SUCCEEDED(m_pRTPInfoSynch->OnRTCPPacket(pPktBuf,
                                                                  uStreamNumber)));
            }
            else
            {
                UINT32 ulSequenceNumber = 0;
                UINT32 ulTimestamp = 0;
                BOOL bSynched = FALSE;
                
                HX_VERIFY(SUCCEEDED(m_pRTPInfoSynch->OnRTPPacket(pPktBuf, 
                                                                 uStreamNumber,
                                                                 bSynched,
                                                                 ulSequenceNumber,
                                                                 ulTimestamp)));
                
                if (!bSynched)
                {
                    //not enough info to synch yet. drop this packet

                    if (m_bIsLLL)
                    {
                        pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), 
                                            uStreamNumber,
                                            unRule, 
                                            pPacketExt->GetRuleSeqNoArray(),
                                            unNewSeq, 
                                            FALSE);

                        pPacketExt->Release();
                    }

                    HX_RELEASE(pPktBuf);
                    return HXR_OK;
                }
                else 
                {
                    //tell RTSP to send the play response.
                    //the play response won't be constructed until start times
                    //for all streams have been set:
                    pTransport->setSequenceNumber(uStreamNumber, ulSequenceNumber);
                    pTransport->setTimeStamp(uStreamNumber, ulTimestamp, TRUE);
                    
                    SetStreamStartTime(uStreamNumber, pPacket->GetTime());
                    pSD->SetFirstPacketTS(pPacket->GetTime());
                }
            }
            HX_RELEASE(pPktBuf);
        }
    }

    /*
     * Sync live streams to keyframe
     */

    if (!pSD->m_pRules[unRule].m_bSyncOk)
    {
        if (!pSD->m_pRules[unRule].m_bRuleOn)
        {
            pSD->m_pRules[unRule].m_bSyncOk = TRUE;
            pSD->m_pRules[unRule].m_PendingAction = NONE;


            if (m_bIsLLL)
            {
                pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), 
                         uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(),unNewSeq, FALSE);

                pPacketExt->Release();
            }

            return HXR_OK;
        }
        else if (pSD->IsDependOk(TRUE, unRule) &&
                 (ucFlags & HX_ASM_SWITCH_ON))
        {
            //fprintf(stderr, "got keyframe (%u, %u)\n", uStreamNumber, unRule);
            //fflush(stderr);
            pSD->m_pRules[unRule].m_bSyncOk = TRUE;
            pSD->m_pRules[unRule].m_PendingAction = NONE;
            if(m_bEnableLiveRSDLog && !m_bPktBufQProcessed && !m_bIsWireline 
                && uStreamNumber == m_unKeyframeStream && unRule == m_unKeyframeRule)
            {
                char szTime[128];
                Timeval tNow = m_pProc->pc->engine->now;
                struct tm localTime;
                time_t tTime = 0;
                hx_localtime_r(&tNow.tv_sec, &localTime);
                strftime(szTime, 128, "%d-%b-%y %H:%M:%S", &localTime);
                
                fprintf(stderr, "%s.%3d RSDLive(mobile) S=%s DM=%s FKT=%f\n",
                            szTime, tNow.tv_usec/1000,
                            m_pPlayerSessionId->GetBuffer(),
                            m_bIsMobileCBR ? "CBR" : "TSD",
                            (float)(pPacket->GetTime())/1000.0);
                fflush(0);
            }
        }
        else
        {
#ifdef XXXTDM_GENERATE_EMPTY_PACKETS
            IHXBuffer*     ppktBuf;
            UINT32          npktTime;
            UINT16          npktStream;
            UINT8           npktFlags;
            UINT16          npktRule;
            ServerPacket*   pNewPacket;
            IHXBuffer*     pEmptyBuf;

            pPacket->Get(ppktBuf, npktTime, npktStream, npktFlags, npktRule);
            ppktBuf->Release();
            pPacket->Release();
            pNewPacket = new ServerPacket;
            pEmptyBuf = new ServerBuffer;
            // Force a memory alloc so GetBuffer does not return NULL
            pEmptyBuf->SetSize(1);
            pEmptyBuf->SetSize(0);
            pNewPacket->Set(pEmptyBuf, npktTime, npktStream, npktFlags, npktRule);
            pNewPacket->QueryInterface(IID_IHXPacket, (void**)&pPacket);

            goto SkipAllASMProcessing;
#else
            if (m_bIsLLL)
            {
                pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), 
                                 uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(), unNewSeq, FALSE);
                pPacketExt->Release();
            }
 
            return HXR_OK;
#endif
        }
    }

    if ((pSD->m_pRVDrop) &&
        (!pSD->m_pRVDrop->PacketApproved(pPacket)))
    {
        if (m_bIsLLL)
        {
            pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), 
                             uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(),unNewSeq, FALSE);
            pPacketExt->Release();
        }

        return HXR_OK;
    }

    if (pSD->m_pRules[unRule].m_PendingAction == NONE)
    {
        /*
         * No rule transition is in progress.  Attempt normal filtering
         * of packets whose rules are not on.
         */
        if (!pSD->m_pRules[unRule].m_bRuleOn)
        {
            if (m_bIsLLL)
            {
                pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), 
                                    uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(), unNewSeq, FALSE);
                pPacketExt->Release();
            }

            return HXR_OK;
        }
    }
    else
    {
        /*
         * Rule transition is in progress.  Handle SwitchOn and SwitchOff
         * flags correctly here.
         */
        if (pSD->m_pRules[unRule].m_PendingAction == ACTION_ON)
        {
            if ((!(ucFlags & HX_ASM_SWITCH_ON)) ||
                (!pSD->IsDependOk(TRUE, unRule)))
            {
                if (m_bIsLLL)
                {
                    pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), 
                                     uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(), unNewSeq, FALSE);
                    pPacketExt->Release();
                }

                return HXR_OK;
            }
            else
            {
                // Update Delivery rate
                if (!pSD->m_pRules[unRule].m_bBitRateReported)
                {
                    m_ulDeliveryRate += pSD->m_pRules[unRule].m_ulAvgBitRate;
                    m_pPPM->ChangeDeliveryBandwidth(
                        pSD->m_pRules[unRule].m_ulAvgBitRate,
                        !m_bIsMulticast, pSD);
                    if (pSD->m_pRules[unRule].m_bTimeStampDelivery)
                    {
                        pSD->m_ulVBRAvgBitRate += pSD->m_pRules[unRule].m_ulAvgBitRate;
                    }

                    pSD->m_pRules[unRule].m_bBitRateReported = TRUE;
                }
                pSD->m_pRules[unRule].m_PendingAction = NONE;
            }
        }
        else if (pSD->m_pRules[unRule].m_PendingAction == ACTION_OFF)
        {
            if ((ucFlags & HX_ASM_SWITCH_OFF) &&
                (pSD->IsDependOk(FALSE, unRule)))
            {
                if (pSD->m_pRules[unRule].m_bBitRateReported)
                {
                    m_ulDeliveryRate -= pSD->m_pRules[unRule].m_ulAvgBitRate;
                    m_pPPM->ChangeDeliveryBandwidth((-1) *
                        (INT32)pSD->m_pRules[unRule].m_ulAvgBitRate,
                        !m_bIsMulticast, pSD);
                    if (pSD->m_pRules[unRule].m_bTimeStampDelivery)
                    {
                        pSD->m_ulVBRAvgBitRate -= pSD->m_pRules[unRule].m_ulAvgBitRate;
                    }

                    pSD->m_pRules[unRule].m_bBitRateReported = FALSE;
                }

                pSD->m_pRules[unRule].m_PendingAction = NONE;

                if (pSD->m_bStreamDonePending)
                {
                    bStreamDone = TRUE;

                    for (i = 0; i < pSD->m_lNumRules; i++)
                    {
                        if (pSD->m_pRules[unRule].m_PendingAction != NONE)
                        {
                            bStreamDone = FALSE;
                        }
                    }

                    if (bStreamDone)
                    {
                        // all the rules are unsubscribed and
                        // we are done with this stream
                        StreamDone(uStreamNumber);
                    }
                }

                if (m_bIsLLL)
                {
                    pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), 
                                     uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(), unNewSeq, FALSE);
                    pPacketExt->Release();
                }

                return HXR_OK;
            }
        }
    }

    /*
     * If the packet is NOT IsLost() but we don't have a buffer, then we
     * must should have only needed this packet for updating the ASM state
     * machine, but the state machine must be messed up somehow.
     * In this case, don't queue the packet!
     */
    IHXBuffer* pTmpBuffer;
    pTmpBuffer = pPacket->GetBuffer();
    if (pTmpBuffer != NULL)
    {
        pTmpBuffer->Release();
    }
    else
    {
        if (m_bIsLLL)
        {
            pSD->HandleSequence(pPacketExt->GetStreamSeqNo(),
                              uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(), unNewSeq, FALSE);
            pPacketExt->Release();
        }

        return HXR_OK;
    }

SkipAllASMProcessing:
    /* Prepare to Send this packet */

    /* Unless we are about to overflow the wouldblock queue */
    if (m_ulHeadTSDTime && pSD->m_pRules[unRule].m_bTimeStampDelivery)
    {
        m_ulLastTSDTime = pPacket->GetTime();
    }

    if ((m_ulHeadTSDTime && ((m_ulLastTSDTime - m_ulHeadTSDTime) >
         m_uMaxBlockedQInMsecs)) || (m_uBlockedBytes > m_uMaxBlockedBytes))
    {
        (*g_pBroadcastPPMOverflows)++;
        if (m_bIsLLL)
        {
            pSD->HandleSequence(pPacketExt->GetStreamSeqNo(),
                               uStreamNumber, unRule, pPacketExt->GetRuleSeqNoArray(), unNewSeq, FALSE);
            pPacketExt->Release();
        }

        return HXR_OK;
    }

    /* This is a travesty of justice, but it's REALLY fast */
    if (HXR_OK ==
        pPacket->QueryInterface(IID_ServerPacket, (void **)0xffffd00d))
    {
        /*
         * This is a live stream so we need to make sure to wrap the packet,
         * else the SAME packet could come up into another session
         * and trash the sequence number while sitting in the queue for this
         * session
         */
        pServerPacket = new (m_pProc->pc->mem_cache) ServerPacket();
        pServerPacket->SetPacket((ServerPacket*)pPacket);
        pServerPacket->AddRef();
    }
    else
    {
        IHXRTPPacket *pRTPPacket;

        PANIC(("Old-Style IHXPacket Wrapping Used\n"));
        if (pPacket->QueryInterface(IID_IHXRTPPacket,
                                    (void**) &pRTPPacket) == HXR_OK)
        {
            pServerPacket = new ServerRTPPacket();
            pRTPPacket->Release();
        }
        else
        {
            pServerPacket = new (m_pProc->pc->mem_cache) ServerPacket();
        }

        pServerPacket->SetPacket(pPacket);
        pServerPacket->AddRef();
    }
    
    //If AverageBandwidth is not available in the ASMRule book calculate BandwidthUsage using 
    //BWCalculator.
    if (pSD->m_pRules[unRule].m_ulAvgBitRate == 0 && pSD->m_pRules[unRule].m_pBWCalculator != NULL)
    {
        pSD->m_pRules[unRule].m_pBWCalculator->BytesSent(pServerPacket->GetSize());
    }
    pServerPacket->m_uPriority =
        pSD->m_pRules[unRule].m_ulPriority;

    pServerPacket->m_uASMRuleNumber = unRule;

    if (m_bIsLLL)
    {
        HX_RESULT hRes;

        hRes = pSD->HandleSequence(pPacketExt->GetStreamSeqNo(), uStreamNumber, 
                        unRule, pPacketExt->GetRuleSeqNoArray(), unNewSeq, TRUE);

        if (hRes != HXR_OK)
        {
            pPacketExt->Release();
            return HXR_OK;
        }

        pServerPacket->m_uSequenceNumber = unNewSeq;

#if 0
        printf("PPM Live, sending pkt %p strm %d strm seqno %d\n", pServerPacket,
                uStreamNumber, pServerPacket->m_uSequenceNumber);
#endif
    }
    else
    {
        // oh so simple sequence number management assumes in-order guaranteed (no gap)
        // delivery of packets.
        pServerPacket->m_uSequenceNumber = pSD->m_unSequenceNumber++;

        if (pSD->m_unSequenceNumber >=
            pSD->m_pTransport->wrapSequenceNumber())
        {
            pSD->m_unSequenceNumber = 0;
        }
    }

    if (pServerPacket->m_uPriority == 10)
    {
        pServerPacket->m_uReliableSeqNo  = ++pSD->m_unReliableSeqNo;
    }
    else
    {
        pServerPacket->m_uReliableSeqNo = pSD->m_unReliableSeqNo;
    }

                    //Require RTPInfoSynch for the relfector case
                    //This should have been setup in ::RegisterStream()
                    //and the RTP-Info data should have been set at the
                    //begining of ::LiveSesisonPacketReady()
    HX_ASSERT(!pSD->m_pTransport->isReflector() ||
              (pSD->m_pTransport->isReflector() && m_pRTPInfoSynch));
    
    if ((m_bRTPInfoRequired) &&
        (!m_bIsMulticast) &&
        (!pSD->m_pTransport->isReflector()))
    {
        IHXRTPPacket* pRTPPacket = NULL;
        UINT32 ulBaseTime = 0;
        UINT16 i = 0;

        if(pPacket->QueryInterface(IID_IHXRTPPacket, 
                                    (void**) &pRTPPacket) == HXR_OK)
        {
            UINT32 ulRTPTime = pRTPPacket->GetRTPTime();
            
            ulBaseTime =  (pSD->m_pTSConverter) ?
                pSD->m_pTSConverter->rtp2hxa_raw(ulRTPTime)
                : ulRTPTime;

            pSD->m_pTransport->setTimeStamp(uStreamNumber, 
                                            ulRTPTime, 
                                            TRUE);
            HX_RELEASE(pRTPPacket);
            }
            else
            {
            ulBaseTime = pPacket->GetTime();
            pSD->m_pTransport->setTimeStamp(uStreamNumber, ulBaseTime);
        }

        pSD->m_pTransport->setSequenceNumber(uStreamNumber, pServerPacket->m_uSequenceNumber);
        SetStreamStartTime(uStreamNumber, pPacket->GetTime());

        for (i = 0; i < m_unStreamCount; i++)
        {
            if (i != uStreamNumber)
            { 
                UINT32 ulRTPInfoTime =         (m_pStreamData [i].m_pTSConverter) ?
                    m_pStreamData [i].m_pTSConverter->hxa2rtp_raw(ulBaseTime) :
                    ulBaseTime;
                
                if (m_pStreamData [i].m_pTransport)
                {
                    m_pStreamData[i].m_pTransport->setTimeStamp(i, ulRTPInfoTime, TRUE);
                    m_pStreamData [i].m_pTransport->setSequenceNumber(i, 
                                                  m_pStreamData [i].m_unSequenceNumber);
                
                    SetStreamStartTime(i, pPacket->GetTime());
                }
        }
        }

        m_bRTPInfoRequired = FALSE;
    }  

    if (m_bWouldBlockAvailable)
    {
        if (!m_ulWouldBlocking)
        {
            HX_ASSERT(m_pBlockedQ->IsEmpty());

            TransmitPacket(pServerPacket);
        }
        else
        {
            IUnknown* punkItem;

            rc = HXR_BLOCKED;

            if (m_pBlockedQ->IsEmpty())
            {
                m_uMaxBlockedBytes = m_ulDeliveryRate * m_uMaxBlockedQInMsecs / 8000;
            }

            if (pSD->m_pRules[unRule].m_bTimeStampDelivery)
            {
                m_ulLastTSDTime = pServerPacket->GetTime();
                if (!m_ulHeadTSDTime)
                {
                    m_ulHeadTSDTime = m_ulLastTSDTime;
                }
            }
            else
            {
                m_uBlockedBytes += pServerPacket->GetSize();
            }

            pServerPacket->QueryInterface(IID_IUnknown, (void**)&punkItem);
            HX_ASSERT(punkItem != NULL);
            m_pBlockedQ->InsertTail(punkItem);
            if (pSD->m_pRules[unRule].m_bTimeStampDelivery)
            {
                m_pBlockedTSDQ->InsertTail(punkItem);
            }
            punkItem->Release();
        }
    }
    else
    {
        // UDP data path
        TransmitPacket(pServerPacket);
    }

    // This code is for bit accounting - both for logging and for when a token 
    // buffer filter is in use. Can't just look at m_bPktBufQProcessed because
    // we ALWAYS want to be able to log when the predata has been fulfilled...
    // and we can get here without a PktBufQ
    if(!m_bPktBufQProcessed || !m_bSentPredata)
    {
#ifdef RSD_LIVE_DEBUG
        if(m_bIsWireline)
        {
            if(m_ulLastTS == 0)
            {
                m_ulLastTS = HX_GET_BETTERTICKCOUNT();
            }

            UINT32 tCur = HX_GET_BETTERTICKCOUNT();

            if(tCur - m_ulLastTS > 1000)
            {
                UINT32 tb = m_ulTotalBytesSent - m_ulPrevTotalBytesSent;
                UINT32 ts = tCur - m_ulLastTS;
                float bw = (float)tb/(float)ts*1000.0*8.0;
                printf("Interval %dmsec, %d bytes sent, bw = %f\n", ts, tb, bw);

                m_ulPrevTotalBytesSent = m_ulTotalBytesSent;
                m_ulLastTS = tCur;
            }
        }
#endif

        if(m_bEnableLiveRSDLog)
        {
            if(!m_ulFirstPacketTS)
            {
                m_ulFirstPacketTS = pServerPacket->GetTime();
            }

            if(!m_ulRSDDebugTS)
            {
                m_ulRSDDebugTS = HX_GET_TICKCOUNT();
            }
        }

        IHXBuffer* pBuffer = NULL;
        UINT32 ulSize = 0;
        pBuffer = pServerPacket->GetBuffer();
        if(pBuffer)
        {
            ulSize = pBuffer->GetSize();
            //rtp live, we need to substract the rtp header and skip rtcp packets
            // for token calculation
            if(m_bIsReflector)
            {
                UINT32 ulRTPHdrSize = 0;
                if(unRule != m_unRTCPRule)
                {   
                    ulRTPHdrSize = GetRTPHeaderSize(pBuffer);
                    if(ulRTPHdrSize < ulSize)
                    {   
                        ulSize -= ulRTPHdrSize;
                    }
                }
                else
                {   
                    //rtcp packets will not be counted into the tokens.
                    ulSize = 0;
                }
            }

            pBuffer->Release();

            m_ulTotalBytesSent += ulSize;

            // If there is a token buffer filter we need to do the accounting for 
            // the bits we just sent
            if(m_pServerTBF)
            {
                rc = m_pServerTBF->RemoveTokens(ulSize);
            }
        }

        // When we have sent enough to satisfy the clients predata amount we 
        // need to log it and relax the sending rate of the token buffer 
        // filter (if present)
        if(m_bSentPredata == FALSE)
        {
            if(m_ulTotalBytesSent > m_ulPreData)
            {
                if(m_bIsWireline && m_pServerTBF)
                {
                    UINT32 ulNewDeliveryRate = m_ulDeliveryRate + 
                                m_ulDeliveryRate/100*m_lExtraMediaRateAmount;
#ifdef RSD_LIVE_DEBUG
                    printf("RSD reset the bandwidth to %d bps\n", ulNewDeliveryRate);
#endif
                    m_pServerTBF->SetBandwidth(ulNewDeliveryRate);

                    //even if we have some tokens left, we want to slow down.
                    //The max sending rate after this stage is 2 X media rate.
                    m_ulIteration = NORMAL_ITER;
                }

                m_bSentPredata = TRUE;

                if(m_pSessionStats2)
                {
                    m_pSessionStats2->SetPreDataTime(HX_GET_BETTERTICKCOUNT());
                }

                if(m_bEnableLiveRSDLog)
                {
                    UINT32 unStrmNum = pPacket->GetStreamNumber();
                    UINT16 unRule = pPacket->GetASMRuleNumber();
                    UINT32 ulDR = m_ulTBFBandwidth ? 
                                  m_ulTBFBandwidth : m_ulDeliveryRate;

                    //we can only send as fast as MediaRate*MAX_ITER, even we have extra
                    //bandwidth left.
                    if(ulDR > m_ulDeliveryRate*MAX_ITER)
                    {
                        ulDR = m_ulDeliveryRate*MAX_ITER;
                    }

                    char* szSignalType = NULL;
                    char  szDR[48];
                    char  szEDT[48];
                    float fEDT = 0.0;

                    if(ulDR)
                    {
                        fEDT = (float)m_ulPreData*8.0/(float)ulDR;
                    }

                    snprintf(szDR, sizeof(szDR), "%d", ulDR);
                    snprintf(szEDT, sizeof(szEDT), "%f", fEDT);

                    if(m_bWouldBlockAvailable)
                    {
                        strcpy(szDR, "N/A");
                        strcpy(szEDT, "N/A");
                        szSignalType = "TCP";
                    }
                    else if(m_ulClientBandwidth)
                    {
                        szSignalType = "BW";
                    }
                    else if(m_ulActualDeliveryRate)
                    {
                        szSignalType = "SDB";
                    }
                    else 
                    {
                        szSignalType = "NONE";
                    }

                    float fADT = float(HX_GET_TICKCOUNT() - m_ulRSDDebugTS)/1000.0;
                    char szTime[128];
                    Timeval tNow = m_pProc->pc->engine->now;
                    struct tm localTime;
                    time_t tTime = 0;
                    hx_localtime_r(&tNow.tv_sec, &localTime);
                    strftime(szTime, sizeof(szTime), "%d-%b-%y %H:%M:%S", &localTime);
                    
                    fprintf(stderr, "%s.%3d RSDLive S=%s St=%u R=%u MR=%u PR=%u PD=%ub DR=<%s>:%s"
                                    " EDT=%s ADT=%f CTS=%d\n",
                                szTime, tNow.tv_usec/1000,
                                m_pPlayerSessionId->GetBuffer(),
                                unStrmNum, unRule, 
                                m_ulDeliveryRate, 
                                m_pStreamData[m_unKeyframeStream].m_ulPreroll, 
                                m_ulPreData*8,
                                szSignalType, szDR, szEDT, fADT, m_ulFirstPacketTS);
                    fflush(0);
                }
            }
        }

        if(m_bEnableRSDPerPacketLog)
        {
            UINT32 unStrmNum = pPacket->GetStreamNumber();
            UINT8  ucAsmFlags = pPacket->GetASMFlags();
            UINT16 unRule = pPacket->GetASMRuleNumber();
            UINT32 ulTS = pPacket->GetTime();

            fprintf(stderr, "RSDLivePacket(%d, %d) session: %s ts: %d keyframe; %d\n",
                unStrmNum, unRule, 
                m_pPlayerSessionId ? (char*)m_pPlayerSessionId->GetBuffer(): "unknown",
                ulTS, ucAsmFlags & HX_ASM_SWITCH_ON);
            fflush(0);
         }

    }

    if (pSD->IsStreamDone())
    {
        ScheduleStreamDone(this, pSD->m_pTransport,
                           pSD, uStreamNumber); 
    }

    HX_RELEASE(pPacketExt);
    HX_RELEASE(pServerPacket);
    return rc;
}

void
PPM::Session::CalculateInitialSendingRate()
{
    UINT32 ulFloorSpeed = m_ulDeliveryRate + m_ulDeliveryRate/100*m_lExtraMediaRateAmount;
    UINT32 ulCeilingSpeed = 0;
    
    if(m_ulClientBandwidth)
    {
        ulCeilingSpeed = m_ulClientBandwidth/100*(100 - m_lCPUUsage);
    }
    else
    {
        ulCeilingSpeed = m_ulActualDeliveryRate/100*(100 - m_lCPUUsage);
    }


    m_ulTBFBandwidth = ulCeilingSpeed > ulFloorSpeed ? ulCeilingSpeed : ulFloorSpeed;
#ifdef RSD_LIVE_DEBUG
    printf("avg: %d, act: %d, floor: %d, ceil:  %d, tbf: %d\n",
    m_ulClientBandwidth, m_ulActualDeliveryRate, ulFloorSpeed, ulCeilingSpeed, m_ulTBFBandwidth);
#endif

    m_pServerTBF->SetBandwidth(m_ulTBFBandwidth);

    return;
}

void
PPM::Session::CalculatePreDataAmount()
{
    //different streams may have different preroll, here we will use the one of keyframe stream.
    float fPrerollInSec = 1.0;
    PPMStreamData* pSD = m_pStreamData + m_unKeyframeStream;
    if(pSD)
    {
        fPrerollInSec = (float)pSD->m_ulPreroll/1000.0;
    }

    for (int i = 0; i < m_unStreamCount; i++)
    {
        PPMStreamData* pSD = m_pStreamData + i;
        BOOL bCBR = (pSD->m_ulMaxBitRate == pSD->m_ulAvgBitRate || pSD->m_ulMaxBitRate == 0);

        if (pSD && pSD->m_bStreamRegistered && !pSD->m_bSentStreamDone)
        {
            for (int j = 0; j < pSD->m_lNumRules; j++)
            {
                PPMStreamData::RuleInfo* pRI = pSD->m_pRules + j;
                //
                if (pRI && pRI->m_bRuleOn)
                {
                    //CBR case
                    if(bCBR)
                    {
                        if(pRI->m_ulAvgBitRate)
                        {
                            //the resaon to use floating computation is to avoid overflow 
                            m_ulPreData += (UINT32)(fPrerollInSec * (float)pRI->m_ulAvgBitRate);
                        }
                    }
                    else //VBR case
                    {
                        if(pRI->m_ulMaxBitRate)
                        {
                            m_ulPreData += (UINT32)(fPrerollInSec * (float)pRI->m_ulMaxBitRate);
                        }
                    }
                }
            }
        }
    }
    //convert to bytes and second
    m_ulPreData = m_ulPreData/8;
    m_ulPreData += m_ulPreData/100*m_ulExtraPrerollInPercentage;

#ifdef RSD_LIVE_DEBUG
    printf("PPM::Session(%p) PreData %d bytes, preroll %f sec\n", this,
                   m_ulPreData, fPrerollInSec);
#endif

    if(m_pSessionStats2)
    {
        m_pSessionStats2->SetPreDataBytes(m_ulPreData);
        pSD = m_pStreamData + m_unKeyframeStream;
        m_pSessionStats2->SetPrerollInMsec(pSD->m_ulPreroll);
    }

    return;
}

//copied from rtptran.cpp
UINT32 
PPM::Session::GetRTPHeaderSize(IHXBuffer* pBuffer)
{
    BYTE* pRawPkt = (BYTE*)pBuffer->GetBuffer();
    UINT32 ulRTPHeaderSize = 0;

    UINT8 uiCSRCCount = (UINT32)(pRawPkt[0] & 0x0F);
    ulRTPHeaderSize += (4 * 3); // RTP fixed header size, not including CSRCs.
    ulRTPHeaderSize += 4 * uiCSRCCount; // CSRCs.

    // Extension header present.
    if (pRawPkt[0] & 0x20)
    {
        ulRTPHeaderSize += 2; // 16-bit profile-defined field

        // Overrun prevention.
        if (pBuffer->GetSize() > ulRTPHeaderSize + 1)
        {
            // Extension length is last 16 bits of first word.
            UINT32 ulExtensionLength = (pRawPkt[ulRTPHeaderSize] << 8) +
                 pRawPkt[ulRTPHeaderSize + 1];
            ulRTPHeaderSize += 2; // 16-bit length field.
            // Rest of extension header.
            ulRTPHeaderSize += (ulExtensionLength * 4);
        }                                     
    }
    return ulRTPHeaderSize;
}

void
PPM::Session::GetAllRSDConfigure()
{
    m_lExtraMediaRateAmount = 0;
    m_pProc->pc->registry->GetInt(EXTRA_MEDIARATE_AMOUNT,
                                &m_lExtraMediaRateAmount, m_pProc);

    LimitRange(m_lExtraMediaRateAmount, 1, 100, DEFAULT_EXTRA_MEDIARATE_AMOUNT, 100);

    m_ulExtraPrerollInPercentage = 0;
    m_pProc->pc->registry->GetInt(EXTRA_PREROLL_IN_PERCENTAGE,
                                &m_ulExtraPrerollInPercentage, m_pProc);

    LimitRange(m_ulExtraPrerollInPercentage, 0, 500, DEFAULT_EXTRA_MEDIARATE_AMOUNT, 500);

    m_lMinTokenBucketCeiling = 0;
    m_pProc->pc->registry->GetInt(MIN_TOKEN_BUCKET_CEILING,
                                &m_lMinTokenBucketCeiling, m_pProc);

    if(m_lMinTokenBucketCeiling<= 0)
    {
        m_lMinTokenBucketCeiling = DEFAULT_MIN_TOKEN_BUCKET_CEILING;
    }

    INT32 nTemp = 0;
    if(HXR_OK ==m_pProc->pc->registry->GetInt(ENABLE_LIVE_RSD_LOG,
                                       &nTemp, m_pProc))
    {
        m_bEnableLiveRSDLog= (nTemp != 0);
    }
    else
    {
        m_bEnableLiveRSDLog = FALSE;
    }

    if(HXR_OK ==m_pProc->pc->registry->GetInt(ENABLE_QUEUE_DEBUG_LOG,
                                       &nTemp, m_pProc))
    {
        m_bQueueDebugLog = (nTemp != 0);
    }
    else
    {
        m_bQueueDebugLog = FALSE;
    }

    if (HXR_OK ==
        m_pProc->pc->registry->GetInt(ENABLE_RSD_PER_PACKET_LOG,
                                &nTemp, m_pProc))
    {
        m_bEnableRSDPerPacketLog = (nTemp != 0);
    }
    else
    {
        m_bEnableRSDPerPacketLog = FALSE;
    }

    m_lCPUThresholdForRSD = 0;
    m_pProc->pc->registry->GetInt(CPU_THRESHOLD_TO_DISABLE_RSD,
                                &m_lCPUThresholdForRSD, m_pProc);

    if(m_lCPUThresholdForRSD <= 0 || m_lCPUThresholdForRSD >= 100)
    {
        m_lCPUThresholdForRSD = DEFAULT_CPU_THRESHOLD_TO_DISABLE_RSD;
    }

    m_pProc->pc->registry->GetInt("Server.PercentAggCPUUsage", &m_lCPUUsage, m_pProc);
    m_ulRSDDebugTS = 0;
    m_ulFirstPacketTS = 0;

}


BOOL
PPM::Session::DoesUseWirelineLogic()
{
#ifdef FORCE_MOBILE_LIVE_RSD_LOGIC
    return FALSE;
#endif

    if(!m_pSessionStats)
    {
        return FALSE;
    }

    IHXClientStats * pClientStat = m_pSessionStats->GetClient();
    IHXBuffer* pUserAgent = NULL;
    char* szUserAgent = NULL;
    UINT32 i = 0;

    if(pClientStat)
    {
        pUserAgent = pClientStat->GetUserAgent();
        pClientStat->Release();
    }

    if(pUserAgent)
    {
        szUserAgent = (char*)pUserAgent->GetBuffer();
        pUserAgent->Release();
    }

    if(!szUserAgent)
    {
        return FALSE;
    }

    if(strncmp(szUserAgent, "RealMedia", 9) == 0 || 
       strncmp(szUserAgent, "RealOnePlayer", 13) == 0)
    {
        //realplayer, check for os
        while(g_userAgentOS[i])
        {
            if(strstr(szUserAgent, g_userAgentOS[i]))
            {
                return TRUE;
            }
            i++;
        }
    }

    // not using else here, so we have a chance to enable rsd for players like
    // "RealMedia....Symbian..."
    if(!g_userAgentWildCard)
    {
        //parse the config and init the wildcard table
        HX_RESULT res = HXR_OK;
        IHXValues* pUAWildCards = 0;

        if (HXR_OK == m_pProc->pc->registry->GetPropList(
            USER_AGENT_STRING, pUAWildCards, m_pProc))
        {
            const char* name;
            UINT32      id;
            UINT32      ulCount = 0;

            res = pUAWildCards->GetFirstPropertyULONG32(name, id);
            while(res == HXR_OK)
            {
                ulCount++;
                res = pUAWildCards->GetNextPropertyULONG32(name, id);
            }

            g_userAgentWildCard = new char*[ulCount + 1];
            UINT32 i = 0;

            res = pUAWildCards->GetFirstPropertyULONG32(name, id);
            while(res == HXR_OK)
            {
                IHXBuffer* pBuf = 0;
                if(HXR_OK == m_pProc->pc->registry->GetStr(id, pBuf, m_pProc))
                {
                    g_userAgentWildCard[i] = new_string((const char *)pBuf->GetBuffer());
                    i++;

                    HX_RELEASE(pBuf);
                }
                res = pUAWildCards->GetNextPropertyULONG32(name, id);
            }
            g_userAgentWildCard[i] = 0;
            pUAWildCards->Release();
        }
        else
        {
            g_userAgentWildCard = new char*[1];
            g_userAgentWildCard[0] = NULL;
        }
    }

    i = 0;
    while(g_userAgentWildCard[i])
    {
        if(strstr(szUserAgent, g_userAgentWildCard[i]))
        {
            return TRUE;
        }
        i++;
    }

    return FALSE;
}
    
void
PPM::Session::TransmitPacket(ServerPacket* pPacket)
{
    if (!pPacket)
    {
    return;
    }

    PPMStreamData* pStreamData = m_pStreamData + pPacket->GetStreamNumber();
    UINT16     unRule             = 0;
    UINT32         ulAggregateTo      = 0;
    UINT32         ulAggregateHighest = 0;
    UINT32         ulPacketSize       = 0;

    if (m_bNeedStreamStartTime)
    {
        m_bNeedStreamStartTime = FALSE;

        // This forces the pending PLAY response (if any) to be sent
        SetStreamStartTime(pPacket->GetStreamNumber(), pPacket->GetTime());
    }

    if (pPacket->IsLost())
    {
        unRule = 0;
        pStreamData->m_bPacketRequested = FALSE;
    }
    else
    {
        unRule = pPacket->GetASMRuleNumber();
        ulPacketSize = pPacket->GetSize();
        pStreamData->m_bPacketRequested = FALSE;
    }

    /* Do Back-to-Back and Packet Aggregation */

    /* TimeStamp delivered sources don't currently support BackToBack pkts */
    if (!m_pPPM->m_bIsRTP &&
    pStreamData->m_pRules[unRule].m_bTimeStampDelivery == FALSE)
    {
        m_pPPM->m_ulBackToBackCounter++;
        if (m_pPPM->m_ulBackToBackCounter >= m_pPPM->m_ulBackToBackFreq)
        {
            m_pPPM->m_ulBackToBackCounter = 0;
            m_bAttemptingBackToBack = TRUE;
        }
    }

    /*
     * Depending on the server's load state and the delivery rate of this
     * PPM (LBR vs HBR), we decide the lowest and highest boundaries for
     * aggregated packets (and turn off aggregation if it's not supported).
     */
    if (pStreamData->m_bSupportsPacketAggregation &&
    g_bDisablePacketAggregation == FALSE)
    {
        _ServerState State = m_pProc->pc->loadinfo->GetLoadState();
    
        if (m_pPPM->m_bInformationalAggregatable == FALSE)
        {
            m_pPPM->m_bInformationalAggregatable = TRUE;
            *g_pAggregatablePlayers += 1;
        }
    
        PPM::GetAggregationLimits(State, m_ulActualDeliveryRate,
                                  &ulAggregateTo, &ulAggregateHighest);
    }
    else
    {
        ulAggregateTo = 0;
    }


    /*

     * In addition to packet aggregation, we use the PacketQueue for handling
     * back-to-back packets.  The second of any back-to-back packet pair MUST
     * not be aggregated (the client's RTSP layer can't handle that).
     *
     * This allows us to send the 2 back-to-back packets if they are
     * available, but tells us to keep aggregating, if we can still fit
     * more data into the first of the back to back pair.
     */
    if (m_bAttemptingBackToBack &&
       m_ucPacketQueuePos > 0 &&
       (m_ulPacketQueueSize > ulAggregateTo ||
        m_ulPacketQueueSize + ulPacketSize > ulAggregateHighest))
    {
        m_pPacketQueue[m_ucPacketQueuePos - 1]->m_bBackToBack = TRUE;

        UINT32 ulQueueSize = SendPacketQueue(pStreamData->m_pTransport);

        pStreamData->m_pTransport->sendPacket(pPacket);
        *g_pBytesServed += ulPacketSize;

        (*g_pPPS)++;

        ulPacketSize = ulQueueSize + ulPacketSize;

        m_bAttemptingBackToBack = FALSE;
    }
    else
    {
        /*
         * If queueing this packet will cause us to go over our maximum
         * allowed aggregation size, then we should flush the aggregation
         * queue and then queue this packet.
         *
         * Don't do this if there is nothing in the packet queue.  This
         * case means that a single packet from the file format is larger
         * then ulAggregateHighest.  In this case, we allow the else code
         * to correctly handle this.
         */
        if (m_bAttemptingBackToBack == FALSE && m_ulPacketQueueSize &&
            m_ulPacketQueueSize + ulPacketSize > ulAggregateHighest)
        {
            UINT32 ulQueueSize = SendPacketQueue(pStreamData->m_pTransport);
            ASSERT(ulQueueSize);

            /* QueuePacket will take control of pPacket (Screw COM) */
            pPacket->AddRef();
            QueuePacket(pPacket, ulPacketSize, pStreamData->m_pTransport);
            ulPacketSize = ulQueueSize;
        }
        else
        {
            /* QueuePacket will take control of pPacket (Screw COM) */
            pPacket->AddRef();
            QueuePacket(pPacket, ulPacketSize, pStreamData->m_pTransport);

            /*
             * Flush the aggregation queue, if we have reached the target
             * size and we aren't trying to do any back-to-back-packets.
             */
            if (m_ulPacketQueueSize > ulAggregateTo &&
            m_bAttemptingBackToBack == FALSE)
            {
            ulPacketSize = SendPacketQueue(pStreamData->m_pTransport);
            ASSERT(ulPacketSize);
            }
        }
    }
}


HX_RESULT
PPM::Session::SessionPacketReady(HX_RESULT ulStatus,
                                IHXPacket* pPacket)
{
    HX_ASSERT(!m_bIsLive);
/*
    XXXSMPNOW Disabled for safety.  Because this function will exit
    without a mutex.

    if (m_pPPM->m_bDidLock)
    {
        m_pPPM->m_pProc->pc->engine->m_bMutexProtection = FALSE;
        HXMutexUnlock(g_pServerMainLock);
        m_pPPM->m_bDidLock = FALSE;
    }
*/

    PPMStreamData*      pStreamData;
    ServerPacket*       pServerPacket = NULL;
    UINT16              uStreamNumber;
    UINT16              unRule;
    UINT8               ucFlags;
    UINT8               i = 0;
    BOOL                bStreamDone = FALSE;

    //XXXGH...how about checking the status Sujal?

    if (m_bIsDone)
    {
        return HXR_UNEXPECTED;
    }

    if (pPacket->IsLost())
    {
        uStreamNumber = pPacket->GetStreamNumber();
        unRule = 0;
        ucFlags = 0;
        pStreamData = m_pStreamData + uStreamNumber;
        pStreamData->m_bPacketRequested = FALSE;
    }
    else
    {
        uStreamNumber = pPacket->GetStreamNumber();
        unRule = pPacket->GetASMRuleNumber();
        ucFlags = pPacket->GetASMFlags();
        pStreamData = m_pStreamData + uStreamNumber;
        pStreamData->m_bPacketRequested = FALSE;
    }

    UINT32 ulActualdiff = 0;
#ifdef DEBUG
    Timeval tActual = m_pProc->pc->engine->now - m_tTimeLineStart;
    ulActualdiff = tActual.tv_sec*1000+tActual.tv_usec/1000;
#endif


    if (pStreamData->m_bStreamDone)
    {
        /*
         * We may get PacketReady calls for pending GetPacket request on
         * other streams. This should ONLY happen when there is an end time
         * associated with this source (refer to the next if condition).
         */
        HX_ASSERT(!m_bIsLive && m_uEndPoint > 0);

        // keep calling GetNextPacket() so that the other stream
        // will not be blocked by the done stream in RMFFPLIN
        goto SkipPacketAndGetNextOne;
    }

    //
    // the rule is outside of our rulebook so the packet is bad
    //
    if (unRule >= pStreamData->m_lNumRules)
    {
        DPRINTF(0x02000000, ("Session %p: dropped packet with bad rule. stream:"
                             " %d time %ld asm rule: %d\n", this, uStreamNumber,
                             pPacket->GetTime(), unRule));
        goto SkipPacketAndGetNextOne;
    }

    /*
     * If we have received a packet past the end time,
     * mark the source as done. This assumes that the fileformat
     * always returns packets in increasing timestamp order across
     * streams.
     */
    if (m_uEndPoint > 0 &&
            pPacket->GetTime() >= m_uEndPoint)
    {
    if (m_bIsPausePointSet)
    {
        //Use pause-stream semantics... don't unsubscribe rules
        Pause(FALSE);
    }
    else if (!pStreamData->m_bStreamDonePending)
    {
            pStreamData->m_bStreamDonePending = TRUE;
        pStreamData->m_bFirstPacketTSSet = FALSE;
        pStreamData->m_ulFirstPacketTS = 0;

            // unsubscribe any active rules of this stream
            for (i = 0; i < pStreamData->m_lNumRules; i++)
            {
                if (pStreamData->m_pRules[i].m_bRuleOn)
                {
                    pStreamData->m_pRules[i].m_bActivateOnSeek = TRUE;
                    HandleUnSubscribe(i, uStreamNumber);
                }
            }
        }
    }

    if (pPacket->IsLost())
    {
        goto SkipAllASMProcessing;
    }

    if ((pStreamData->m_pRVDrop) &&
            (!pStreamData->m_pRVDrop->PacketApproved(pPacket)))
    {
        goto SkipPacketAndGetNextOne;
    }

    if (pStreamData->m_pRules[unRule].m_PendingAction == NONE)
    {
        /*
         * No rule transition is in progress.  Attempt normal filtering
         * of packets whose rules are not on.
         */
        if (!pStreamData->m_pRules[unRule].m_bRuleOn)
        {
            goto SkipPacketAndGetNextOne;
        }
    }
    else
    {
        /*
         * Rule transition is in progress.  Handle SwitchOn and SwitchOff
         * flags correctly here.
         */
        if (pStreamData->m_pRules[unRule].m_PendingAction == ACTION_ON)
        {
            if ((!(ucFlags & HX_ASM_SWITCH_ON)) ||
                (!pStreamData->IsDependOk(TRUE, unRule)))
            {
                goto SkipPacketAndGetNextOne;
            }
            else
            {
                // Update Delivery rate
                if (!pStreamData->m_pRules[unRule].m_bBitRateReported)
                {
                    m_ulDeliveryRate += pStreamData->m_pRules[unRule].m_ulAvgBitRate;
                    m_pPPM->ChangeDeliveryBandwidth(
                        pStreamData->m_pRules[unRule].m_ulAvgBitRate,
                        !m_bIsMulticast, pStreamData);
                    if (pStreamData->m_pRules[unRule].m_bTimeStampDelivery)
                    {
                        pStreamData->m_ulVBRAvgBitRate += pStreamData->m_pRules[unRule].m_ulAvgBitRate;
                    }

                    pStreamData->m_pRules[unRule].m_bBitRateReported = TRUE;
                }
                pStreamData->m_pRules[unRule].m_PendingAction = NONE;
            }
        }
        else if (pStreamData->m_pRules[unRule].m_PendingAction == ACTION_OFF)
        {
            if ((ucFlags & HX_ASM_SWITCH_OFF) &&
                (pStreamData->IsDependOk(FALSE, unRule)))
            {
                if (pStreamData->m_pRules[unRule].m_bBitRateReported)
                {
                    m_ulDeliveryRate -= pStreamData->m_pRules[unRule].m_ulAvgBitRate;
                    m_pPPM->ChangeDeliveryBandwidth((-1) *
                        (INT32)pStreamData->m_pRules[unRule].m_ulAvgBitRate,
                        !m_bIsMulticast, pStreamData);
                    if (pStreamData->m_pRules[unRule].m_bTimeStampDelivery)
                    {
                        pStreamData->m_ulVBRAvgBitRate -= pStreamData->m_pRules[unRule].m_ulAvgBitRate;
                    }

                    pStreamData->m_pRules[unRule].m_bBitRateReported = FALSE;
                }

                pStreamData->m_pRules[unRule].m_PendingAction = NONE;

                if (pStreamData->m_bStreamDonePending)
                {
                    bStreamDone = TRUE;

                    for (i = 0; i < pStreamData->m_lNumRules; i++)
                    {
                        if (pStreamData->m_pRules[unRule].m_PendingAction != NONE)
                        {
                            bStreamDone = FALSE;
                        }
                    }

                    if (bStreamDone)
                    {
                        // all the rules are unsubscribed and
                        // we are done with this stream
                        StreamDone(uStreamNumber);
                    }
                }

                goto SkipPacketAndGetNextOne;
            }
        }
    }

    /*
     * If the packet is NOT IsLost() but we don't have a buffer, then we
     * must should have only needed this packet for updating the ASM state
     * machine, but the state machine must be messed up somehow.
     * In this case, don't queue the packet!
     */
    IHXBuffer* pTmpBuffer;
    pTmpBuffer = pPacket->GetBuffer();
    if (pTmpBuffer != NULL)
    {
        pTmpBuffer->Release();
    }
    else
    {
        goto SkipPacketAndGetNextOne;
    }

SkipAllASMProcessing:

    /* This is a travesty of justice, but it's REALLY fast */
    if (HXR_OK ==
        pPacket->QueryInterface(IID_ServerPacket, (void **)0xffffd00d))
    {
        pServerPacket = (ServerPacket *)pPacket;
    }
    else
    {
        IHXRTPPacket *pRTPPacket;

        PANIC(("Old-Style IHXPacket Wrapping Used\n"));
        if (pPacket->QueryInterface(IID_IHXRTPPacket,
                                    (void**) &pRTPPacket) == HXR_OK)
        {
            pServerPacket = new ServerRTPPacket();
            pRTPPacket->Release();
        }
        else
        {
            pServerPacket = new (m_pProc->pc->mem_cache) ServerPacket();
        }

        pServerPacket->SetPacket(pPacket);
    }

    pServerPacket->m_uPriority =
        pStreamData->m_pRules[unRule].m_ulPriority;

    pServerPacket->m_uASMRuleNumber = unRule;

    pStreamData->m_pPackets.PutPacket(pServerPacket);
    m_ulPacketsOutstanding++;
    pStreamData->m_ulPacketsOutstanding++;

    /* TimeStamp delivered sources don't currently support BackToBack pkts */
    if (!m_pPPM->m_bIsRTP &&
        pStreamData->m_pRules[unRule].m_bTimeStampDelivery == FALSE)
    {
        m_pPPM->m_ulBackToBackCounter++;
        if (m_pPPM->m_ulBackToBackCounter >= m_pPPM->m_ulBackToBackFreq)
        {
            m_pPPM->m_ulBackToBackCounter = 0;
            m_bAttemptingBackToBack = TRUE;
        }
    }

    JumpStart(uStreamNumber);

    return HXR_OK;

    /* This code is for skipping this packet and getting the next one */
SkipPacketAndGetNextOne:
    GetNextPacket(uStreamNumber);

    return HXR_OK;
}

STDMETHODIMP_(ULONG32)
PPM::Session::GetBandwidth()
{
    UINT32 ulActualDeliveryRate = 0;

    if (m_ulActualDeliveryRate)
    {
        if (!m_bSourceIsDone)
            ulActualDeliveryRate += m_ulActualDeliveryRate;
    }
    else if (m_pStreamData)
    {
        for (int i = 0; i < m_unStreamCount; i++)
        {
            PPMStreamData* pSD = m_pStreamData + i;
            if (pSD && pSD->m_bStreamRegistered && !pSD->m_bSentStreamDone)
                ulActualDeliveryRate += pSD->m_ulAvgBitRate;
        }
    }

    return ulActualDeliveryRate;
}

void
PPM::RecalcActualDeliveryRate()
{
    HXList_iterator     i(&m_Sessions);
    Session*                    pSession;

    UINT32 ulOld = m_ulAdjustedDeliveryRate;
    UINT32 ulVBRRate = 0;
    UINT32 ulTotalVBRRate = 0;
    m_ulActualDeliveryRate = 0;
    m_ulAdjustedDeliveryRate = 0;

    // A PPM may contain CBR and VBR streams.  The m_ulActualDeliveryRate
    // computed here is consumed only by the CBR streams.  New VBR streams
    // will include AvgBitrate in the ASM Rulebook, so they appear to
    // players as CBR streams, but in fact are TimeStampDelivery.  TSD
    // streams are adjusted through the ratio of AvgBitrate and actual
    // bit rate.  This routine converts the delivery rate into the delivery
    // ratio.
    //
    // There is a second method to set delivery rate.  This is through the
    // RTSP speed parameter.  In this case, the procedure is reversed.
    // For CBR streams, the acceleration is converted from the ratio into
    // an actual delivery rate

    for (; *i != 0; ++i)
    {
        pSession = (Session*) (*i);

        if (pSession->m_bTimeLineSuspended)
            continue;

        if (pSession->m_ulActualDeliveryRate)
        {
            if (pSession->m_ulDeliveryRate != 0.0)
            {
                pSession->m_fDeliveryRatio =
                    (float) pSession->m_ulActualDeliveryRate /
                    (float)pSession->m_ulDeliveryRate;
            }
            else
            {
                // This shouldn't really happen
                pSession->m_fDeliveryRatio = 1.0;
            }



            for (int j = 0; j < pSession->m_unStreamCount; j++)
            {
                // Need to go through and eliminate VBR TSD streams
                PPMStreamData* pSD = pSession->m_pStreamData + j;

                if (pSD->m_bStreamRegistered)
                {
                    ulVBRRate += (UINT32)
                        (pSD->m_ulVBRAvgBitRate*pSession->m_fDeliveryRatio);
                }
            }

            if (!pSession->m_bSourceIsDone)
                m_ulActualDeliveryRate += pSession->m_ulActualDeliveryRate
                    - ulVBRRate;
        }
        else
        {
            for (int j = 0; j < pSession->m_unStreamCount; j++)
            {
                PPMStreamData* pSD = pSession->m_pStreamData + j;

                if (!pSD->m_bStreamRegistered)
                {
                    continue;
                }

                UINT32 ulAdjustedRate = pSD->m_ulAvgBitRate -
                    pSD->m_ulVBRAvgBitRate;
                ulVBRRate += (UINT32)(pSD->m_ulVBRAvgBitRate*
                        pSession->m_fDeliveryRatio);


                // If a delivery ratio has been set, adjust delivery rate
                ulAdjustedRate = (UINT32)
                    ((float)ulAdjustedRate*pSession->m_fDeliveryRatio);

                // VBR streams are timestamp delivered and do not
                // figure into this value

                if (!pSD->m_bSentStreamDone)
                    m_ulActualDeliveryRate += ulAdjustedRate;
            }
        }
        ulTotalVBRRate += ulVBRRate;
        ulVBRRate = 0;
    }
    m_ulAdjustedDeliveryRate = m_ulActualDeliveryRate + ulTotalVBRRate;

//    printf ("---> Actual Delivery TOTAL %d\n", m_ulActualDeliveryRate);fflush(0);
    HXAtomicAddUINT32(g_pAggregateRequestedBitRate, (m_ulActualDeliveryRate - ulOld));
}

BOOL
PPM::SendNextPacket(Session* pFixedSession)
{

    // XXXSMP Handle roundoff errors upwards (or maybe it doesn't matter with
    //      adaptive bandwidth adjustments).
    HXList_iterator     i(&m_Sessions);
    Session*                    pSession;

    Timeval tNow = m_pProc->pc->engine->now;
    Timeval tTimeSinceLastRecalc = tNow - m_tLastSendTime;
    UINT32  ulMsecSinceLastRecalc = tTimeSinceLastRecalc.tv_sec * 1000 +
        tTimeSinceLastRecalc.tv_usec / 1000;
    UINT16 j;
    INT32  lHighestDue     = -0x0fffffff;
    PPMStreamData* pBestStream = 0;
    UINT16 unBestStreamNumber = 0;
    Session* pBestSession = 0;
    BOOL bLeftIdle = FALSE;
    UINT32 ulActualDeliveryRate = (UINT32)(
        (g_bSlightAcceleration) ? m_ulActualDeliveryRate * 1.05
                                : m_ulActualDeliveryRate);
    BOOL bAgain = FALSE;
    Timeval tNextTime;

    if (m_bIdle)
    {
       if (m_tNextSendTime.tv_sec && (tNow < m_tNextSendTime +
           Timeval(1,0)))
       {
           /*
            * We're not even a second behind.  We'll just continue
            * sending packets from the timings where we left off.
            */
           m_bIdle = FALSE;
           HXAtomicDecUINT32(g_pIdlePPMs);
       }
       else
       {
           // XXXSMP This is basically a hack that should be reconsidered.

           ulMsecSinceLastRecalc = 250;
           m_bIdle = FALSE;
           HXAtomicDecUINT32(g_pIdlePPMs);
           bLeftIdle = TRUE;
       }
        //printf ("%p: PPM Leaving Idle State\n", this);
    }

    for (; *i != 0; ++i)
    {
        pSession = (Session*) (*i);

        if ((pSession->IsPaused()) || (pSession->m_bInSeek))
        {
            continue;
        }

        if (pFixedSession && (pFixedSession != pSession))
        {
            continue;
        }

        if (pSession->m_bIsLive)
        {
            continue;
        }

        if ((pSession->m_bInitialPlayReceived) &&
            (pSession->m_bRTPInfoRequired) &&
            (!pSession->m_bIsMulticast))
        {
            pBestSession = pSession;
            pBestStream = NULL;

            for (j = 0; j < pSession->m_unStreamCount; j++)
            {
                PPMStreamData* pSD = pSession->m_pStreamData + j;
                if (!pSD->m_bStreamRegistered)
                {
                    continue;
                }

                ServerPacket* pPacket = pSD->m_pPackets.PeekPacket();
                if (pPacket)
                {
                    pBestStream = pSD;
                    unBestStreamNumber = j;
                    break;
                }
            }
            
            if (pBestSession && pBestStream)
            {
                break;
            }
        }

        UINT32  ulLowestTimeStampSeen     = 0xffffffff;
        BOOL bLowStampInvalid = TRUE;
        PPMStreamData* pTSBasedBestStream = 0;
        UINT16 unTSBasedBestStreamNumber = 0;
        Session* pTSBasedBestSession = 0;
        INT32 lSourceLowRatio = -0x0fffffff;
        UINT32 ulSessionBitRate = 0;
        INT32 ulTotalBytesDueTimes10 = 0;
        BOOL bAnyPacketsForThisSession = FALSE;
        UINT32 ulScaledAvgBitRate   = 0;

        for (j = 0; j < pSession->m_unStreamCount; j++)
        {
            PPMStreamData* pSD = pSession->m_pStreamData + j;
            if (!pSD->m_bStreamRegistered)
            {
                continue;
            }

            ServerPacket* pPacket = pSD->m_pPackets.PeekPacket();
            ulScaledAvgBitRate = (UINT32)
                (pSD->m_ulAvgBitRate * pSession->m_fDeliveryRatio);

            if (pPacket)
            {
                bAnyPacketsForThisSession = TRUE;
            }

            // Distribute each streams' share of bytes.
            pSD->m_lBytesDueTimes10 += (ulMsecSinceLastRecalc * ulScaledAvgBitRate)
                / 800;

            if (pPacket && !pSD->m_pRules[pPacket->m_uASMRuleNumber].
                    m_bTimeStampDelivery)
            {
                pSD->m_lStreamRatioTemp = (INT32)(((double)pSD->m_lBytesDueTimes10
                    * 10000 / (double)ulScaledAvgBitRate));

                ulSessionBitRate += ulScaledAvgBitRate;
                ulTotalBytesDueTimes10 += pSD->m_lBytesDueTimes10;
            }
        }

        if (pSession->m_bAttemptingBackToBack &&
            bAnyPacketsForThisSession == FALSE)
        {
            /*
             * Just use any old transport from this session.  Left over
             * packets in the queue implies that this is RDT.  These packets
             * are either left over BackToBack packets or left over
             * aggregation packets.
             */
            HX_ASSERT(pSession->m_uFirstStreamRegistered != 0xFFFF);
            pSession->SendPacketQueue(
                pSession->m_pStreamData[pSession->m_uFirstStreamRegistered].m_pTransport);
        }

        if (ulSessionBitRate)
        {
            lSourceLowRatio = (INT32)(((double)ulTotalBytesDueTimes10
                * 10000 / (double)ulSessionBitRate));
        }

        for (j = 0; j < pSession->m_unStreamCount; j++)
        {
            PPMStreamData* pSD = pSession->m_pStreamData + j;
            if (!pSD->m_bStreamRegistered)
            {
                continue;
            }

            ServerPacket* pPacket = pSD->m_pPackets.PeekPacket();

            DPRINTF(D_PROT,
                    ("Bytes Due: %ld (Stream: %d) [Highest %ld] Pkt %p\n",
                     pSD->m_lBytesDueTimes10, j, lHighestDue, pPacket));

            if (pPacket && !pSD->m_pRules[pPacket->m_uASMRuleNumber].
                    m_bTimeStampDelivery)
            {
                UINT32 ulTime = pPacket->GetTime();
                INT32 lRatio = pSD->m_lStreamRatioTemp;

                if ((ulTime < ulLowestTimeStampSeen) &&
                        (lSourceLowRatio > lHighestDue))
                {
                    bLowStampInvalid = FALSE;

                    lHighestDue    = lRatio;
                    ulLowestTimeStampSeen = ulTime;
                    pTSBasedBestStream    = pSD;
                    unTSBasedBestStreamNumber = j;
                    pTSBasedBestSession = pSession;
                }
                else if (ulTime == ulLowestTimeStampSeen)
                {
                    /* Low timestamp delivery is invalid if two streams have
                     * the same low timestamp.
                     */
                    bLowStampInvalid = TRUE;
                }

                if (bLowStampInvalid)
                {
                    if (lRatio > lHighestDue)
                    {
                        lHighestDue    = lRatio;
                        pBestStream    = pSD;
                        unBestStreamNumber = j;
                        pBestSession = pSession;
                    }
                }
                else
                {
                    pBestStream    = pTSBasedBestStream;
                    unBestStreamNumber = unTSBasedBestStreamNumber;
                    pBestSession = pTSBasedBestSession;
                }
            }
        }
    }

    if ((!pBestStream) || (!ulActualDeliveryRate))
    {
        /*
         * If we are trying to do a back to back packet then don't idle the
         * PPM, just return.
         */
        if (pFixedSession)
        {
            return FALSE;
        }

        m_tLastSendTime = tNow;
        if (!m_bIdle) HXAtomicIncUINT32(g_pIdlePPMs);
        m_bIdle = TRUE;

        /*
         * Don't go idle, just remove the packet on the queue because there
         * is no delivery bit rate.
         */
        if (pBestStream)
        {
            ServerPacket* pPacket= pBestStream->m_pPackets.GetPacket();
            pBestSession->m_ulPacketsOutstanding--;
            pBestStream->m_ulPacketsOutstanding--;

            if(pBestStream->m_uTimeStampScheduledSendID)
            {
                m_pProc->pc->engine->ischedule.remove(pBestStream->m_uTimeStampScheduledSendID);
                pBestStream->m_uTimeStampScheduledSendID = 0;    
            }

            HX_RELEASE(pPacket);

            pBestSession->GetNextPacket(unBestStreamNumber);
        }

        return FALSE;
    }

    ServerPacket* pPacket= pBestStream->m_pPackets.GetPacket();
    pBestSession->m_ulPacketsOutstanding--;
    pBestStream->m_ulPacketsOutstanding--;

    /*
     * Notify the player if this is the first packet for the most recent
     * playback request.
     */
    if (!pBestSession->m_bTimeLineSuspended &&
        !pBestStream->IsFirstPacketTSSet())
    {
        pBestStream->SetFirstPacketTS(pPacket->GetTime());
    }

    UINT32 ulPacketSize = pPacket->GetSize();

    pPacket->m_uSequenceNumber = pBestStream->m_unSequenceNumber++;

    if (pBestStream->m_unSequenceNumber >=
        pBestStream->m_pTransport->wrapSequenceNumber())
    {
        pBestStream->m_unSequenceNumber = 0;
    }

    if (pPacket->m_uPriority == 10)
    {
        pPacket->m_uReliableSeqNo  = ++pBestStream->m_unReliableSeqNo;
    }
    else
    {
        pPacket->m_uReliableSeqNo = pBestStream->m_unReliableSeqNo;
    }

    pBestStream->m_lBytesDueTimes10 -= ulPacketSize * 10;

    Transport* pTransport = pBestStream->m_pTransport;
    UINT32 ulAggregateTo;
    UINT32 ulAggregateHighest;
    _ServerState State = m_pProc->pc->loadinfo->GetLoadState();

    if ((pSession->m_bInitialPlayReceived) &&
        (pSession->m_bRTPInfoRequired) &&
         (!pSession->m_bIsMulticast))
    {
         IHXRTPPacket* pRTPPacket = NULL;
         UINT32 ulBaseTime = 0;
         UINT16 unStreamNumber = pPacket->GetStreamNumber();
         UINT16 i = 0;
 
         if(pPacket->QueryInterface(IID_IHXRTPPacket, 
                                    (void**) &pRTPPacket) == HXR_OK)
         {
             UINT32 ulRTPTime = pRTPPacket->GetRTPTime();
 
             ulBaseTime =  (pBestStream->m_pTSConverter) ?
                 pBestStream->m_pTSConverter->rtp2hxa_raw(ulRTPTime)
                 : ulRTPTime;
 
             pTransport->setTimeStamp(unStreamNumber, 
                                      ulRTPTime, 
                                      TRUE);
             HX_RELEASE(pRTPPacket);
         }
         else
         {
             ulBaseTime = pPacket->GetTime();
             pTransport->setTimeStamp(unStreamNumber, ulBaseTime);
         }
         
         pTransport->setSequenceNumber(unStreamNumber,
                                       pPacket->m_uSequenceNumber);
         pSession->SetStreamStartTime(unStreamNumber,
                                      pPacket->GetTime());
 
         for (i = 0; i < pSession->m_unStreamCount; i++)
         {
             if (pSession->m_pStreamData[i].m_bStreamRegistered
                    && (i != unStreamNumber))
             { 
                 UINT32 ulRTPInfoTime =         (pSession->m_pStreamData [i].m_pTSConverter) ?
                     pSession->m_pStreamData [i].m_pTSConverter->hxa2rtp_raw(ulBaseTime) :
                     ulBaseTime;
 
                 pSession->m_pStreamData [i].m_pTransport->setTimeStamp(i, ulRTPInfoTime, TRUE);
                 pSession->m_pStreamData [i].m_pTransport->
                     setSequenceNumber(i, pSession->m_pStreamData [i].m_unSequenceNumber);
 
                 pSession->SetStreamStartTime(i, pPacket->GetTime());
             }
         }
 
         pSession->m_bRTPInfoRequired = FALSE;
    }

    /*
     * Depending on the server's load state and the delivery rate of this
     * PPM (LBR vs HBR), we decide the lowest and highest boundaries for
     * aggregated packets (and turn off aggregation if it's not supported).
     */
    if (pBestStream->m_bSupportsPacketAggregation &&
        g_bDisablePacketAggregation == FALSE)
    {
        if (m_bInformationalAggregatable == FALSE)
        {
            m_bInformationalAggregatable = TRUE;
            *g_pAggregatablePlayers += 1;
        }

        GetAggregationLimits(State, ulActualDeliveryRate,
                             &ulAggregateTo, &ulAggregateHighest);
    }
    else
    {
        ulAggregateTo = 0;
    }

    BOOL bDontRelease = FALSE;
    /*
     * In addition to packet aggregation, we use the PacketQueue for handling
     * back-to-back packets.  The second of any back-to-back packet pair MUST
     * not be aggregated (the client's RTSP layer can't handle that).
     *
     * This allows us to send the 2 back-to-back packets if they are
     * available, but tells us to keep aggregating, if we can still fit
     * more data into the first of the back to back pair.
     */

    if (pBestSession->m_bAttemptingBackToBack &&
        pBestSession->m_ucPacketQueuePos > 0 &&
        (pBestSession->m_ulPacketQueueSize > ulAggregateTo ||
         pBestSession->m_ulPacketQueueSize + ulPacketSize > ulAggregateHighest))
    {
        pBestSession->m_pPacketQueue[pBestSession->m_ucPacketQueuePos - 1]->
            m_bBackToBack = TRUE;
        UINT32 ulQueueSize = pBestSession->SendPacketQueue(pTransport);

        pTransport->sendPacket(pPacket);
        *g_pBytesServed += ulPacketSize;
        (*g_pPPS)++;

        ulPacketSize = ulQueueSize + ulPacketSize;

        pBestSession->m_bAttemptingBackToBack = FALSE;
    }
    else
    {
        /*
         * If queueing this packet will cause us to go over our maximum
         * allowed aggregation size, then we should flush the aggregation
         * queue and then queue this packet.
         *
         * Don't do this if there is nothing in the packet queue.  This
         * case means that a single packet from the file format is larger
         * then ulAggregateHighest.  In this case, we allow the else code
         * to correctly handle this.
         */
        if (pBestSession->m_bAttemptingBackToBack == FALSE &&
            pBestSession->m_ulPacketQueueSize &&
            pBestSession->m_ulPacketQueueSize + ulPacketSize >
                ulAggregateHighest)
        {
            UINT32 ulQueueSize = pBestSession->SendPacketQueue(pTransport);
            ASSERT(ulQueueSize);

            /* QueuePacket will take control of pPacket (Screw COM) */
            bDontRelease = TRUE;
            pBestSession->QueuePacket(pPacket, ulPacketSize, pTransport);
            ulPacketSize = ulQueueSize;
        }
        else
        {
            /* QueuePacket will take control of pPacket (Screw COM) */
            bDontRelease = TRUE;
            pBestSession->QueuePacket(pPacket, ulPacketSize, pTransport);

            /*
             * Flush the aggregation queue, if we have reached the target
             * size and we aren't trying to do any back-to-back-packets.
             */
            if (pBestSession->m_ulPacketQueueSize > ulAggregateTo &&
                pBestSession->m_bAttemptingBackToBack == FALSE)
            {
                ulPacketSize = pBestSession->SendPacketQueue(pTransport);
                ASSERT(ulPacketSize);
            }
            else
            {
                m_tLastSendTime = tNow;
                bAgain = TRUE;
                goto DoItFromHere;
            }
        }
    }

    if (pBestStream->IsStreamDone())
    {
        pBestSession->ScheduleStreamDone(pBestSession, pTransport,
            pBestStream, unBestStreamNumber);
    }

    // Schedule Next Packet
    if (bLeftIdle || m_tNextSendTime.tv_sec == 0)
    {
        tNextTime =  tNow + (1000 * (INT32)((1000 *
            ulPacketSize * 8) / ulActualDeliveryRate));
    }
    else
    {
#if 0
        // XXXSMP Faster but inaccurate integer calculation
        tNextTime =  m_tNextSendTime + 100 * ((INT32)((10000 *
            ulPacketSize * 8) / ulActualDeliveryRate));
#endif
        tNextTime =  m_tNextSendTime + (INT32)(1000000.0 *
            (double)ulPacketSize * 8.0 / (double)ulActualDeliveryRate);
    }
    m_tLastSendTime = tNow;
    if (tNow > tNextTime)
    {
        if (tNow > tNextTime + Timeval(1,0))
        {
            UINT32 ulBehindBy = (tNow - tNextTime).tv_sec;
            tNextTime += Timeval(0.5 * ulBehindBy);
            (*g_pOverloads)++;
            m_pProc->pc->loadinfo->Overload();
        }
        else
        {
            (*g_pBehind)++;
        }
        bAgain = TRUE;
    }
    else
    {
        m_uScheduledSendID = m_pProc->pc->engine->ischedule.enter(
            tNextTime, m_pTimeCallback);
    }

    m_tNextSendTime = tNextTime;

DoItFromHere:
    if (bDontRelease == FALSE)
    {
        pPacket->Release();
    }

    if (pBestSession->m_bThreadSafeGetPacket == FALSE &&
        m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
        /* XXXSMP Can't get a lock? Schedule GNP */
        HXMutexLock(g_pServerMainLock);
        m_pProc->pc->engine->m_bMutexProtection = TRUE;
        m_bDidLock = TRUE;
        pBestSession->GetNextPacket(unBestStreamNumber);
        if (m_bDidLock)
        {
            m_pProc->pc->engine->m_bMutexProtection = FALSE;
            HXMutexUnlock(g_pServerMainLock);
            m_bDidLock = FALSE;
        }
    }
    else
    {
        pBestSession->GetNextPacket(unBestStreamNumber);
    }

    return bAgain;
}

UINT32
PPM::Session::SendPacketQueue(Transport* pTransport)
{
    /*
     * It should be noted that we aggregate packets across streams in
     * a single session.  Because of this, we make the assumption that
     * the underlying pTransport is irrelevant (which is true for RDT but NOT
     * true for RTP).  I.e., all aggregated transports must support this
     * concept.
     *
     * When the queue only has 1 packet, the pTransport is correct.
     */
    if (m_ucPacketQueuePos == 0)
    {
        return 0;
    }

    m_pPacketQueue[m_ucPacketQueuePos] = NULL;
    if (m_ucPacketQueuePos == 1)
    {
        pTransport->sendPacket(*m_pPacketQueue);
    }
    else
    {
        pTransport->sendPackets(m_pPacketQueue);
    }
    *g_pBytesServed += m_ulPacketQueueSize;

    (*g_pPPS) += m_ucPacketQueuePos;
    if (m_ucPacketQueuePos > 1)
    {
        (*g_pAggreg) += m_ucPacketQueuePos;
    }

    while (--m_ucPacketQueuePos != (UINT8)-1)
    {
        m_pPacketQueue[m_ucPacketQueuePos]->Release();
    }
    UINT32 ulPacketSize = m_ulPacketQueueSize;
    m_ulPacketQueueSize = 0;
    m_ucPacketQueuePos = 0;

    return ulPacketSize;
}

/* This function violates COM Reference Rules */
void
PPM::Session::QueuePacket(ServerPacket* pPacket, UINT32 ulPacketSize,
    Transport* pTransport)
{
    /*
     * Reserve one for the NULL, one for zero indexing,
     * and one for good luck.
     */
    if (m_ucPacketQueuePos >= (PACKET_QUEUE_SIZE - 3))
    {
        SendPacketQueue(pTransport);
    }
    m_ulPacketQueueSize += ulPacketSize;
    m_pPacketQueue[m_ucPacketQueuePos++] = pPacket;
}

STDMETHODIMP
PPM::Session::ResendCallback::Func()
{
    m_pTransport->sendPacket(m_pPacket);
    *g_pBytesServed += m_pPacket->GetSize();
    (*g_pPPS)++;
    (*g_pResends)++;
    HX_RELEASE(m_pPacket);
    *m_pZeroMe = 0;

    return HXR_OK;
}

PPM::Session::ResendCallback::~ResendCallback()
{
    HX_RELEASE(m_pPacket);
}

STDMETHODIMP
PPM::Session::OnPacket(UINT16 uStreamNumber, BasePacket** ppPacket)
{
    if (m_bIsDone)
    {
        return HXR_UNEXPECTED;
    }

    PPMStreamData* pSD = m_pStreamData + uStreamNumber;
    HX_ASSERT(pSD->m_bStreamRegistered);
    BasePacket* pPacket;

    for (; (pPacket = *ppPacket); ppPacket++)
    {
        /*
         * Do not check for IsStreamDone() because this is part of the
         * resend mechanism
         */

        if (!pPacket->IsResendRequested() || pPacket->m_uPriority == 10)
        {
            if (!pPacket->IsResendRequested())
            {
                pPacket->SetResendRequested();
            }

            UINT32 ulPos = m_ulResendIDPosition + 1;
            if (ulPos == MAX_RESENDS_PER_SECOND)
            {
                ulPos = 0;
            }

            while (m_pResendIDs[ulPos] && ulPos != m_ulResendIDPosition)
            {
                ulPos++;
                if (ulPos == MAX_RESENDS_PER_SECOND)
                {
                    ulPos = 0;
                }
            }

            if (!m_pResendIDs[m_ulResendIDPosition = ulPos])
            {
#ifndef _SOLARIS27
                ResendCallback* pResendCB = new (m_pProc->pc->mem_cache) ResendCallback();
#else
                ResendCallback* pResendCB = new ResendCallback();
#endif
                pResendCB->AddRef();
                pResendCB->m_pTransport   = pSD->m_pTransport;
                pResendCB->m_pPacket      = pPacket;
                pResendCB->m_pPacket->AddRef();
                pResendCB->m_pZeroMe      = m_pResendIDs + m_ulResendIDPosition;

                UINT32 ulMinWaitTime;
                if (m_ulActualDeliveryRate > 60000)
                {
                    ulMinWaitTime = 500;
                }
                else
                {
                    ulMinWaitTime = 10;
                }

                UINT32 ulTime = rand() % 300 + ulMinWaitTime;

                m_pResendIDs[m_ulResendIDPosition] =
                    m_pProc->pc->engine->schedule.enter(
                    m_pProc->pc->engine->now + Timeval(0, ulTime * 1000),
                    pResendCB);
                pResendCB->Release();
            }
        }
    }
    return HXR_OK;
}

STDMETHODIMP
PPM::Session::SetDropRate(UINT16 uStreamNumber, UINT32 uDropRate)
{
    PPMStreamData* pSD = m_pStreamData + uStreamNumber;

    ASSERT(pSD->m_pRVDrop);

    /*
     * Note: the Fiji preview client sends a drop rate of 0 and we
     * don't want to wrap around or we'll never send a packet
     * JEFFA 4/24/98
     */
    if (pSD && pSD->m_pRVDrop)
        pSD->m_pRVDrop->m_drop_rate = (uDropRate > 0)?(uDropRate - 1):(0);

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::SetDropToBandwidthLimit(UINT16 uStreamNumber,
                                      UINT32 ulBandwidthLimit)
{
    //XXXSMP Sujal please implement.

    return HXR_OK;
}

// Reschedule TS streams when delivery rate changes
void
PPM::Session::Reschedule(UINT16 unStreamNumber, float fOldRatio)
{
    HX_ASSERT(m_pStreamData[unStreamNumber].m_bStreamRegistered 
            && m_pStreamData[unStreamNumber].m_uTimeStampScheduledSendID);
    m_pProc->pc->engine->ischedule.remove(
            m_pStreamData[unStreamNumber].m_uTimeStampScheduledSendID);
    m_pStreamData[unStreamNumber].m_uTimeStampScheduledSendID=0;

    Timeval timeDiff;

    if (m_pProc->pc->engine->now <
        m_pStreamData[unStreamNumber].m_tLastScheduledTime)
    {
            timeDiff = m_pStreamData[unStreamNumber].m_tLastScheduledTime
                - m_pProc->pc->engine->now;
    }
    else
    {
        timeDiff.tv_sec = timeDiff.tv_usec = 0;
    }

    INT64 ulTimeDiffms = timeDiff.tv_sec * 1000 + timeDiff.tv_usec / 1000;

    ulTimeDiffms = (INT64) (ulTimeDiffms * fOldRatio);
    if (m_fDeliveryRatio != 0.0)
        ulTimeDiffms = (INT64) (ulTimeDiffms / m_fDeliveryRatio);

    ServerPacket* pNextPacket =
        m_pStreamData[unStreamNumber].m_pPackets.PeekPacket();

    if (pNextPacket)
    {
        m_pStreamData[unStreamNumber].m_uTimeStampScheduledSendID =
            m_pProc->pc->engine->ischedule.enter(m_tTimeLineStart +
                (int)ulTimeDiffms, m_pStreamData[unStreamNumber].
                m_pTimeStampCallback);

        m_pStreamData[unStreamNumber].m_tLastScheduledTime =
            m_tTimeLineStart + (int)ulTimeDiffms;
    }
}

void
PPM::Session::HandleSpeedChange(float fOldRatio)
{
    if (m_tTimeLineStart != Timeval(0,0))
    {
        Timeval t = m_pProc->pc->engine->now - m_tTimeLineStart;

        if (m_fDeliveryRatio)
        {
            m_ulTSDMark += (UINT32)((t.tv_sec*1000+t.tv_usec/1000) * fOldRatio);
        }
    }

    m_tTimeLineStart = m_pProc->pc->engine->now;

    for (int i=0; i<m_unStreamCount; ++i)
    {
        if (m_pStreamData[i].m_bStreamRegistered
                && m_pStreamData[i].m_uTimeStampScheduledSendID)
        {
            Reschedule(i, fOldRatio);
        }
    }
}

STDMETHODIMP
PPM::Session::SetBandwidth(UINT32 ulBandwidth)
{
    m_ulClientBandwidth = ulBandwidth;
    return HXR_OK;
}

STDMETHODIMP
PPM::Session::SetDeliveryBandwidth(UINT32 ulBackOff,
                                   UINT32 ulBandwidth)
{
    BOOL bReschedule = FALSE;
    float fOldRatio = m_fDeliveryRatio;

    /* XXXSMP, Sanity Gaurd */
    if (ulBandwidth < 100)
        ulBandwidth = 100;

    if (m_ulActualDeliveryRate != ulBandwidth)
    {
        bReschedule = TRUE;
    }

    m_ulActualDeliveryRate = ulBandwidth;
    m_pPPM->RecalcActualDeliveryRate();

    UINT32 ulMs;

         if (ulBandwidth < 5000 )    m_pPPM->m_ulBackToBackFreq = 3;
    else if (ulBandwidth < 20000)    m_pPPM->m_ulBackToBackFreq = 5;
    else if (ulBandwidth < 40000)    m_pPPM->m_ulBackToBackFreq = 10;
    else if (ulBandwidth < 60000)    m_pPPM->m_ulBackToBackFreq = 15;
    else                             m_pPPM->m_ulBackToBackFreq = 20;

    if (ulBandwidth < 20000)
        ulMs = 1000;
    else
        ulMs = (UINT32)(1000 - ((ulBandwidth - 20000) / 100));

    if ((ulMs > 1000) || (ulMs < 100))
        ulMs = 100;

    HX_ASSERT(m_uFirstStreamRegistered != 0xFFFF);
    m_pStreamData[m_uFirstStreamRegistered].m_pTransport->setLatencyPeriod(ulMs);

    if (bReschedule)
    {
        HandleSpeedChange(fOldRatio);
    }

    //printf ("%p: %ld %ld\n", this, ulBandwidth, ulBackOff);

    return HXR_OK;
}

STDMETHODIMP_(float)
PPM::Session::SetSpeed(float fSpeed)
{
    float fOldSpeed = m_fDeliveryRatio;
    m_ulActualDeliveryRate = 0;
    m_fDeliveryRatio = fSpeed;

    // If speed is zero, pause and suspend the timeline
    if (m_fDeliveryRatio == 0.0)
    {
        Pause(0);
    }
    else if (fOldSpeed == 0.0 && m_bPaused)
    {
        Play();
    }

    m_pPPM->RecalcActualDeliveryRate();
    if (m_fDeliveryRatio != fOldSpeed)
    {
        HandleSpeedChange(fOldSpeed);
    }
    return fSpeed;
}

STDMETHODIMP
PPM::Session::WantWouldBlock()
{
    if (m_pStreamData)
    {
        IHXWouldBlock* pWouldBlock = 0;
        HX_ASSERT(m_uFirstStreamRegistered != 0xFFFF);
        if (HXR_OK == m_pStreamData[m_uFirstStreamRegistered].m_pTransport->QueryInterface(
                    IID_IHXWouldBlock, (void**)&pWouldBlock))
        {
            m_bWouldBlockAvailable = TRUE;
            pWouldBlock->WantWouldBlock(this, 0 /* bogus ID */);
            pWouldBlock->Release();
        }
    }

    return HXR_OK;
}

/* IHXWouldBlockResponse methods */
STDMETHODIMP
PPM::Session::WouldBlock(UINT32 id)
{
    PPMStreamData* pSD = m_pStreamData + id;

    HX_ASSERT(pSD->m_bStreamRegistered);
    if (m_pStreamData && pSD->m_bStreamRegistered && !pSD->m_bWouldBlocking)
    {
        pSD->m_bWouldBlocking = TRUE;
        m_ulWouldBlocking++;
        if (m_ulWouldBlocking == 1 && m_bPktBufQProcessed)
        {
            Pause(TRUE);
        }
    }
    return HXR_OK;
}

STDMETHODIMP
PPM::Session::WouldBlockCleared(UINT32 id)
{
    PPMStreamData* pSD = m_pStreamData + id;

    HX_ASSERT(m_bIsDone || pSD->m_bStreamRegistered);
    if (!m_bIsDone && pSD && pSD->m_bStreamRegistered 
           && pSD->m_bWouldBlocking)
    {
        pSD->m_bWouldBlocking = FALSE;
        m_ulWouldBlocking--;
        if (m_ulWouldBlocking == 0 && m_bPktBufQProcessed)
        {
            Play(TRUE);
        }
    }
    return HXR_OK;
}

void
PPM::Session::SetConverter(DataConvertShim* pShim)
{
    HX_RELEASE(m_pConvertShim);
    m_pConvertShim = pShim;
    m_pConvertShim->AddRef();
    m_pConvertShim->SetDataResponse(this);
}

/***********************************************************************
 *  IHXDataConvertResponse
 */
STDMETHODIMP
PPM::Session::DataConvertInitDone(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PPM::Session::ConvertedFileHeaderReady(HX_RESULT status,
                                        IHXValues* pFileHeader)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PPM::Session::ConvertedStreamHeaderReady(HX_RESULT status,
                                            IHXValues* pStreamHeader)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PPM::Session::ConvertedDataReady(HX_RESULT status, IHXPacket* pPacket)
{
    if (status == HXR_OK && !pPacket)
    {
        pPacket = m_pConvertingPacket;
        pPacket->AddRef();
    }
    else
    {
        pPacket->AddRef();
    }

    (m_bIsLive) ? LiveSessionPacketReady(status, pPacket) :
      SessionPacketReady(status, pPacket);
    pPacket->Release();
    return HXR_OK;
}

STDMETHODIMP
PPM::Session::SendControlBuffer(IHXBuffer* pBuffer)
{
    return HXR_NOTIMPL;
}

void
PPM::Session::SetPlayerInfo(Player* pPlayerControl,
                            const char* szSessionId,
                            IHXSessionStats* pSessionStats)
{
    HX_RELEASE(m_pPlayerControl);
    HX_RELEASE(m_pPlayerSessionId);
    HX_RELEASE(m_pSessionStats);
    HX_RELEASE(m_pSessionStats2);

    m_pPlayerControl = pPlayerControl;
    m_pPlayerControl->AddRef();

    m_pSessionStats = pSessionStats;
    if (m_pSessionStats)
    {
        m_pSessionStats->AddRef();
        m_pSessionStats->QueryInterface(IID_IHXSessionStats2,
                                       (void **)&m_pSessionStats2);
    }

    UINT32 ulBuffLen = strlen(szSessionId) + 1;
    m_pPlayerSessionId = new ServerBuffer(TRUE);
    m_pPlayerSessionId->Set((UCHAR*)szSessionId, ulBuffLen);
}

HX_RESULT
PPM::Session::UpdatePlayTime()
{
    /*
     * put the number of elapsed ticks into the session stats.
     *
     * I don't want to call a system call (gettimeofday) for every packet
     * sent; engine->now, updated every mainloop iteration, should be good
     * enough for this.
     *
     * Note that Pause() or Done() effectively wipes out the running
     * calculation done on every packet get, which was a rough calc anyway.
     */

    UINT32 ticks;
    Timeval t(m_tvBankedPlayTime);

    if (!m_bPaused && !m_bIsDone)
    {
        t += (m_pProc->pc->engine->now - m_tvRTSPPlayTime);
    }

    // store elapsed time, t, as number of ticks in session stats

    ticks = t.tv_sec * 1000 + t.tv_usec / 1000;

    if (m_pSessionStats) m_pSessionStats->SetPlayTime(ticks);

    return HXR_OK;
}

// IHXAccurateClock method

STDMETHODIMP_(HXTimeval)
PPM::Session::GetTimeOfDay()
{
    HXTime now;
    HXTimeval hxnow;

    gettimeofday(&now, NULL);
    hxnow.tv_sec = now.tv_sec;
    hxnow.tv_usec = now.tv_usec;

    return hxnow;
}

/* IHXQoSLinkCharSetup methods */
STDMETHODIMP
PPM::Session::SetLinkCharParams (THIS_
                                 LinkCharParams* /* IN */ pLinkCharParams)
{
    UINT16 unStreamNum = 0;

printf("set linkchar\n");
    if (pLinkCharParams->m_bSessionAggregate)
    {
	    if (!m_pSessionAggrLinkCharParams)
	    {
	        m_pSessionAggrLinkCharParams = new LinkCharParams;
	    }
	    *m_pSessionAggrLinkCharParams = *pLinkCharParams;
    }
    else if ( (unStreamNum = pLinkCharParams->m_unStreamNum) < m_unStreamCount )
    {
        m_bStreamLinkCharSet = TRUE;
	    if (!m_pStreamData[unStreamNum].m_pLinkCharParams)
	    {
	        m_pStreamData[unStreamNum].m_pLinkCharParams = new LinkCharParams;
	    }

	    *m_pStreamData[unStreamNum].m_pLinkCharParams = *pLinkCharParams;
    }
    else
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::GetLinkCharParams (THIS_
                                 UINT16 unStreamNum,
                                 REF(LinkCharParams) /* OUT */ linkCharParams)
{
    return HXR_NOTIMPL;
}

/* IHXStreamAdaptationSetup */
STDMETHODIMP
PPM::Session::GetStreamAdaptationScheme (THIS_
                                 REF(StreamAdaptationSchemeEnum) /* OUT */ enumAdaptScheme )
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PPM::Session::SetStreamAdaptationScheme (THIS_
                                         StreamAdaptationSchemeEnum enumAdaptScheme)
{
    m_enumStreamAdaptScheme = enumAdaptScheme;
    return HXR_OK;
}

STDMETHODIMP
PPM::Session::SetStreamAdaptationParams (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams)
{
    if (m_enumStreamAdaptScheme == ADAPTATION_HLX_AGGR)
    {
        if (!m_pAggRateAdaptParams)
	    {
	        m_pAggRateAdaptParams = new StreamAdaptationParams;
	    }
        *m_pAggRateAdaptParams = *pStreamAdaptParams;
    }
    else
    {
        if (pStreamAdaptParams->m_unStreamNum < m_unStreamCount)
        {
            return m_pStreamData[pStreamAdaptParams->m_unStreamNum].SetStreamAdaptation(
                                                                m_enumStreamAdaptScheme,
                                                                pStreamAdaptParams);
        }
	    else
	    {
	        return HXR_FAIL;
	    }
    }

    return HXR_OK;
}

STDMETHODIMP
PPM::Session::GetStreamAdaptationParams (THIS_
                                 UINT16 unStreamNum,
                                 REF(StreamAdaptationParams) /* OUT */ streamAdaptParams)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PPM::Session::UpdateStreamAdaptationParams (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PPM::Session::UpdateTargetProtectionTime (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams)
{
    return HXR_NOTIMPL;
}

HX_RESULT
PPMStreamData::SetStreamAdaptation (StreamAdaptationSchemeEnum enumAdaptScheme,
							StreamAdaptationParams* pStreamAdaptParams)
{
	m_enumStreamAdaptScheme = enumAdaptScheme;

	if (!m_pStreamAdaptParams)
	{
		m_pStreamAdaptParams = new StreamAdaptationParams;
		if (!m_pStreamAdaptParams)
		{
			return HXR_OUTOFMEMORY;
		}
	}

	*m_pStreamAdaptParams = *pStreamAdaptParams;

	return HXR_OK;
}
