/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */
#include "hxtypes.h"
#include "hxassert.h"
#include "debug.h"
#include "hxcom.h"
#include "hxmarsh.h"
#include "hxstrutl.h"
#include "netbyte.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxsbuffer.h"
#include "hxcomm.h"
#include "netbyte.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxbitset.h"
#include "timebuff.h"
#include "timeval.h"
#include "rtptypes.h"
#include "bufnum.h"

#include "ntptime.h"
#include "tscalc.h"
#include "hxtick.h"
#include "rtppkt.h"
#include "rtpwrap.h"
#include "rtpinfosync.h"

#if defined (_SYMBIAN)
#include <netinet/in.h>
#endif
#if defined (_LSB)
#include <arpa/inet.h>
#endif

/*
 * This algorithm determines the RTP timestamp to present in the RTP-Info RTSP field.
 * The RTP-Info field presents to the client the RTP time that maps to the NPT time for the session.
 * The client uses this mapping to determine a reference point for the RTP timestamps in each stream.
 * For RTP reflection streams (live streams) the starting point for each client within the total timeline of the stream
 * is arbitrary.
 * In order to select a reference point for inter and intra stream synchronization the client must be able to map NPT to RTP to NTP
 * for the time at which the client enters the live session timeline.
 * To accomplish this the following algorithm is used:
 * 1) Get the NTP to RTP mapping for each stream from the RTCP SR for each of the streams.
 * 2) On the first packet after RTCP SRs for each stream have arrived:
 *   a) Map the RTP time of that packet to NTP time; this is the reference NTP time that maps NPT to NTP.
 *   b) Compute the corresponding RTP time to that NTP from 2a) for each stream.
 * 3) Set the resulting values from each stream in 2b) to the RTP-Info field.
 * The result is a mapping in terms of each stream's time domain to a common NTP time that represents the NPT of the session.
 */

RTPInfoSynchData::RTPInfoSynchData() :
    m_bHasSR(FALSE),
    m_bHasPacket(FALSE),
    m_ntpTimeFromSR(0, 0),
    m_ulRTPTimeFromSR(0),
    m_ulRTPPacketTime(0),
    m_ulRTPStartTime(0),
    m_ulRTPFrequency(1000)
{
}

RTPInfoSynchData::~RTPInfoSynchData()
{
}

void
RTPInfoSynchData::Reset()
{
    m_bHasSR = FALSE;
    m_bHasPacket = FALSE;
    m_ntpTimeFromSR.m_ulSecond = 0;
    m_ntpTimeFromSR.m_ulFraction = 0;
    m_ulRTPTimeFromSR = 0;
    m_ulRTPPacketTime = 0;
    m_ulRTPStartTime = 0;
    m_ulRTPFrequency = 1000;
}

RTPInfoSynch::RTPInfoSynch() :
    m_lRefCount (0),
    m_pSynchData (NULL),
    m_unStreamCount (0),
    m_unSRCount (0),
    m_unSynchStream (HX_INVALID_STREAM),
    m_bNeedSyncStream (TRUE),
    m_bHaveAllSRs (FALSE),
    m_bHaveAllPackets (FALSE),
    m_unSyncPacketsReceived (0),
    m_bRTPTimesGenerated (FALSE),
    m_pResponse (NULL)
{
}

RTPInfoSynch::~RTPInfoSynch()
{
    Done();
}

UINT32
RTPInfoSynch::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

UINT32
RTPInfoSynch::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return (UINT32)m_lRefCount;
    }
    delete this;
    return 0;
}

HX_RESULT
RTPInfoSynch::InitSynch(UINT16 unStreamCount, 
                        IRTPSyncResponse* pResponse, 
                        UINT16 unMasterStream)
{
    Done();

    if (unStreamCount == 0)
    {
        return HXR_INVALID_PARAMETER;
    }

    m_unStreamCount = unStreamCount;
    
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);

    m_pSynchData = new RTPInfoSynchData [unStreamCount];
    if (unMasterStream != HX_INVALID_STREAM)
    {
        if (unMasterStream < m_unStreamCount)
        {
            m_unSynchStream = unMasterStream;
            m_bNeedSyncStream = FALSE;
        }
        else
        {
            HX_VECTOR_DELETE(m_pSynchData);
            return HXR_INVALID_PARAMETER;
        }
    }

    return HXR_OK;
}

HX_RESULT
RTPInfoSynch::SetTSFrequency(UINT32 ulRTPFrequency, UINT16 unStream)
{
    if (!m_pSynchData)
    {
	return HXR_NOT_INITIALIZED;
    }

    if (unStream >= m_unStreamCount)
    {
	return HXR_INVALID_PARAMETER;
    }

    m_pSynchData[unStream].m_ulRTPFrequency = ulRTPFrequency;

    return HXR_OK;
}

HX_RESULT 
RTPInfoSynch::OnRTPPacket(IHXRTPPacket* pPacket, UINT16 unStream)
{
    if (!pPacket || unStream >= m_unStreamCount)
    {
	return HXR_INVALID_PARAMETER;
    }
    if (!m_pSynchData)
    {
	return HXR_NOT_INITIALIZED;
    }
    if (m_bRTPTimesGenerated)
    {
        // We are already synced!! Why are you sending packets?
        HX_ASSERT(FALSE);
        return HXR_OK;
    }

    if (!m_pSynchData[unStream].m_bHasPacket && 
        (m_bNeedSyncStream || unStream == m_unSynchStream))
    {
        m_pSynchData[unStream].m_bHasPacket = TRUE;
        m_pSynchData[unStream].m_ulRTPPacketTime = pPacket->GetRTPTime();
        ++m_unSyncPacketsReceived;

        if (m_unSynchStream == HX_INVALID_STREAM)
        {
            // If this is the first packet received (and no user-specified 
            // master stream) then we will start by syncing to this stream
            m_unSynchStream = unStream;
        }

        // Wait for a packet from each stream, unless master stream is 
        // user-specified
        if (m_unSyncPacketsReceived >= m_unStreamCount || !m_bNeedSyncStream)
        {
            m_bHaveAllPackets = TRUE;
        }
    }

    if (m_bHaveAllPackets && (m_bHaveAllSRs || m_unStreamCount < 2))
    {
         // We have everything we need to calculate the sync offsets
        CalculateSyncTimes();
    }
    // else we are still waiting for SRs and/or packets

    return HXR_OK;
}

HX_RESULT 
RTPInfoSynch::OnRTCPPacket(RTCPPacketBase* pPacket, UINT16 unStream)
{
    if (!m_pSynchData)
    {
	return HXR_NOT_INITIALIZED;
    }
    if (!pPacket || unStream >= m_unStreamCount)
    {
	return HXR_INVALID_PARAMETER;
    }

    if (!(pPacket->packet_type == RTCP_SR) || m_pSynchData[unStream].m_bHasSR)
    {
        // If it's not an SR or is on a stream we already got an SR for
        // then we don't need to do anything with it
        return HXR_OK;
    }

    m_pSynchData[unStream].m_ntpTimeFromSR.m_ulSecond = pPacket->ntp_sec;
    m_pSynchData[unStream].m_ntpTimeFromSR.m_ulFraction = pPacket->ntp_frac;
    m_pSynchData[unStream].m_ulRTPTimeFromSR = pPacket->rtp_ts;
    m_pSynchData[unStream].m_bHasSR = TRUE;

    m_bHaveAllSRs = (++m_unSRCount >= m_unStreamCount);

    if (m_bHaveAllSRs && m_bHaveAllPackets)
    {
        // This was the last thing we needed, calculate sync times now!
        CalculateSyncTimes();
    }
    // else we are still waiting for SRs and/or packets

    return HXR_OK;
}

void
RTPInfoSynch::CalculateSyncTimes()
{
    m_bRTPTimesGenerated = TRUE;

    if (m_unStreamCount == 1)
    {
        // For single-stream case we just need the first packet timestamp!
        m_pSynchData[0].m_ulRTPStartTime = m_pSynchData[0].m_ulRTPPacketTime;

        if (m_pResponse)
        {
            m_pResponse->SyncDone();
        }
        return;
    }

    UINT16 i = 0;
    UINT16 unInitStream = m_unSynchStream;
    Timeval tvStrmOffset;
    Timeval tvReSyncDiff = Timeval(0, 0);
    INT32 lPktOffset = 0;

    // Get the offset from the sync stream's SR to the first
    // packet start time (packet_time - SR_time) (can be negative)
    // Mind the units - calculate with sync stream's RTP timestamp
    // rollover, then convert to Timeval for proper handling with
    // other streams
    INT32 lRTPSROffset = DiffTimeStamp(
        m_pSynchData[unInitStream].m_ulRTPPacketTime, 
        m_pSynchData[unInitStream].m_ulRTPTimeFromSR);
    
    Timeval tvSyncSROffset ((double)lRTPSROffset / 
        (double)(m_pSynchData[unInitStream].m_ulRTPFrequency));

    NTPTime ntpSyncTime = m_pSynchData[unInitStream].m_ntpTimeFromSR;

    m_pSynchData[unInitStream].m_ulRTPStartTime = m_pSynchData[unInitStream].m_ulRTPPacketTime;

    for (i = 0; i < m_unStreamCount; i++)
    {
        if (i != unInitStream)
        {
            // Get the offset from the stream SR to the packet start time. 
            // Can be negative! But watch the NTP handling, as NTP
            // is unsigned
            // This is SyncSROffset - (StreamSR - SyncSR)
            tvStrmOffset = tvSyncSROffset;
            if (m_pSynchData[i].m_ntpTimeFromSR >= ntpSyncTime)
            {
                tvStrmOffset -= 
                    (m_pSynchData[i].m_ntpTimeFromSR - ntpSyncTime).toTimeval();
            }
            else
            {
                tvStrmOffset += 
                    (ntpSyncTime - m_pSynchData[i].m_ntpTimeFromSR).toTimeval();
            }

            // and now add this to the SR's RTP time (converted) to 
            // get the (raw) stream time at the sync packet time
            tvStrmOffset += (Timeval)(m_pSynchData[i].m_ulRTPTimeFromSR /
                (double)(m_pSynchData[i].m_ulRTPFrequency));

            // and convert back to RTP TS units
            m_pSynchData[i].m_ulRTPStartTime = ConvertToTimestamp(tvStrmOffset,
                (INT32)(m_pSynchData[i].m_ulRTPFrequency));

            // Check if the first packet's time is earlier than start time;
            // If so, then we'll need to re-adjust offsets
            if (m_bNeedSyncStream)
            {
                lPktOffset = DiffTimeStamp(m_pSynchData[i].m_ulRTPPacketTime, 
                                m_pSynchData[i].m_ulRTPStartTime);

                if (lPktOffset < 0)
                {
                    // and check if this is the earliest one
                    tvStrmOffset = (Timeval)((double)lPktOffset /
                        (double)(m_pSynchData[i].m_ulRTPFrequency));

                    if (tvStrmOffset > tvReSyncDiff)
                    {
                        tvReSyncDiff = tvStrmOffset;
                        m_unSynchStream = i;
                    }
                }
            }
        }
    }
    
    // Do we need to re-adjust our sync start point?
    if (m_bNeedSyncStream && m_unSynchStream != unInitStream)
    {
        UINT32 ulRTPDiff;
        for (i = 0; i < m_unStreamCount; i++)
        {
            if (i == m_unSynchStream)
            {
                m_pSynchData[i].m_ulRTPStartTime = 
                    m_pSynchData[i].m_ulRTPPacketTime;
            }
            else
            {
                ulRTPDiff = ConvertToTimestamp(tvReSyncDiff, 
                    (INT32)(m_pSynchData[i].m_ulRTPFrequency));

                m_pSynchData[i].m_ulRTPStartTime += ulRTPDiff;
            }
        }
    }

    if (m_pResponse)
    {
        m_pResponse->SyncDone();
    }
}

HX_RESULT
RTPInfoSynch::GetRTPStartTime(UINT16 unStream, REF(UINT32) ulStartTime)
{
    if (unStream < m_unStreamCount)
    {
        if (m_bRTPTimesGenerated)
        {
            ulStartTime = m_pSynchData[unStream].m_ulRTPStartTime;
            return HXR_OK;
        }

        return HXR_NOT_INITIALIZED;
    }
    
    return HXR_INVALID_PARAMETER;
}

HX_RESULT
RTPInfoSynch::Done ()
{
    m_bHaveAllSRs = FALSE;
    m_bHaveAllPackets = FALSE;
    m_unSRCount = 0;
    m_unSyncPacketsReceived = 0;
    m_bRTPTimesGenerated = FALSE;

    HX_VECTOR_DELETE(m_pSynchData);
    HX_RELEASE(m_pResponse);

    return HXR_OK;
}
