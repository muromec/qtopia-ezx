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

#include "hxtick.h"
#include "rtppkt.h"
#include "rtpinfosync.h"

#if defined (_SYMBIAN)
#include <netinet/in.h>
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
    m_bHasSR (FALSE),
    m_lRTPtoNTPOffset (0),
    m_ulRTPInfoTime (0),
    m_bSynched (FALSE),
    m_pTSConverter (NULL)
{
}

RTPInfoSynchData::~RTPInfoSynchData()
{
    HX_DELETE(m_pTSConverter);
}

void
RTPInfoSynchData::Reset()
{
    m_bHasSR = FALSE;
    m_bSynched = FALSE;
    m_lRTPtoNTPOffset = 0;
    m_ulRTPInfoTime = 0;
}

RTPInfoSynch::RTPInfoSynch() :
    m_lRefCount (0),
    m_pSynchData (NULL),
    m_unStreamCount (0),
    m_unSRCount (0),
    m_unSynchStream (0),
    m_bHaveAllSRs (FALSE),
    m_bRTPTimesGenerated (FALSE)
{
}

RTPInfoSynch::~RTPInfoSynch()
{
    Done();
}

STDMETHODIMP
RTPInfoSynch::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTPInfoSynch))
    {
	AddRef();
	*ppvObj = (IHXRTPInfoSynch*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
RTPInfoSynch::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
RTPInfoSynch::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return (UINT32)m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
RTPInfoSynch::InitSynch (UINT16 nStreamCount)
{
    m_unStreamCount = nStreamCount;

    HX_VECTOR_DELETE(m_pSynchData);
    m_pSynchData = new RTPInfoSynchData [nStreamCount];

    return HXR_OK;
}

HX_RESULT
RTPInfoSynch::SetTSConverter(CHXTimestampConverter::
			     ConversionFactors conversionFactors,
			     UINT16 unStream)
{
    if (!m_pSynchData)
    {
	return HXR_NOT_INITIALIZED;
    }

    if (unStream >= m_unStreamCount)
    {
	return HXR_INVALID_PARAMETER;
    }

    HX_DELETE(m_pSynchData [unStream].m_pTSConverter);

    m_pSynchData [unStream].m_pTSConverter =
	new CHXTimestampConverter(conversionFactors);

    return HXR_OK;
}


STDMETHODIMP
RTPInfoSynch::RTPSynch (UINT16 unMaster)
{
    m_bHaveAllSRs = FALSE;
    m_unSRCount = 0;
    m_unSynchStream = unMaster;
    m_bRTPTimesGenerated = FALSE;

    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
	m_pSynchData [i].Reset();
    }

    return HXR_OK;
}

STDMETHODIMP
RTPInfoSynch::IsStreamSynched (UINT16 unStream, REF(HXBOOL) bIsSynched)
{
    if (!m_pSynchData)
    {
	return HXR_NOT_INITIALIZED;
    }

    if (unStream >= m_unStreamCount)
    {
	return HXR_INVALID_PARAMETER;
    }

    bIsSynched = (m_pSynchData [unStream].m_bSynched);
    return HXR_OK;
}

STDMETHODIMP
RTPInfoSynch::OnRTPPacket (IHXBuffer* pRTPPacket,
			   UINT16 unStream,
			   REF(HXBOOL) bSynched,
			   REF(UINT32) ulSequenceNumber,
			   REF(UINT32) ulTimestamp)
{
    if (!m_bHaveAllSRs)
    {
	bSynched = FALSE;
	ulSequenceNumber = 0;
	ulTimestamp = 0;
	return HXR_OK;
    }

    if (!m_pSynchData)
    {
	return HXR_NOT_INITIALIZED;
    }

    if (!pRTPPacket)
    {
	return HXR_INVALID_PARAMETER;
    }

    //Header offset plus RTP time field must at least be present:
    if (pRTPPacket->GetSize() < 8)
    {
	HX_ASSERT(0);
	return HXR_INVALID_PARAMETER;
    }

    BYTE* pByte = pRTPPacket->GetBuffer();

    if (!pByte)
    {
	HX_ASSERT(0);
	return HXR_INVALID_PARAMETER;
    }

    if (unStream >= m_unStreamCount)
    {
	return HXR_INVALID_PARAMETER;
    }

    if (m_pSynchData [unStream].m_bSynched)
    {
	HX_ASSERT(0); //shouldn't happen
	bSynched = TRUE;
	ulTimestamp = m_pSynchData [unStream].m_ulRTPInfoTime;
	ulSequenceNumber = ntohs(*(UINT16*)((pByte+2)));
    }

    UINT32 ulRTPTime = ntohl(*(unsigned int*)(pByte+4));

    if (!m_bRTPTimesGenerated)
    {
	if (unStream != m_unSynchStream)
	{
	    bSynched = FALSE;
	    ulSequenceNumber = 0;
	    ulTimestamp = 0;
	    return HXR_OK;
	}

        //First packet of of the synch stream is the reference:
	m_pSynchData [unStream].m_ulRTPInfoTime = ulRTPTime;

	//Convert RTP time from RTP units to milliseconds:
	UINT32 ulRTPTimeHX =
	  (m_pSynchData [unStream].m_pTSConverter) ?
	  m_pSynchData [unStream].m_pTSConverter->
	  rtp2hxa_raw(ulRTPTime)
	  : ulRTPTime;

	//Transform the RTP time from this packet to the corresponding NTP time in milliseconds:
	UINT32 NTPMasterMSec = ulRTPTimeHX + m_pSynchData [unStream].m_lRTPtoNTPOffset;

	//For each stream map the master NTP time to the stream's RTP time:
	for (UINT16 i = 0; i < m_unStreamCount; i++)
	{
	    if (i != unStream)
	    {
		//convert NTP to RTP using the arithmetic negation of the RTP to NTP mapping:
		UINT32 RTPMSec = NTPMasterMSec + -(m_pSynchData [i].m_lRTPtoNTPOffset);

		//Convert mapped RTP from milliseconds to RTP time:
		m_pSynchData [i].m_ulRTPInfoTime =
		    (m_pSynchData [i].m_pTSConverter) ?
		    m_pSynchData [i].m_pTSConverter->
		    hxa2rtp_raw(RTPMSec)
		    : RTPMSec;
	    }
	}

	m_bRTPTimesGenerated = TRUE;
    }
    else if (m_pSynchData [unStream].m_ulRTPInfoTime > ulRTPTime)
    {
        // make sure the ts of the first packet is >= to rtptime in RTP-Info
        // because some clients don't choke
        bSynched = FALSE;
        ulSequenceNumber = 0;
        ulTimestamp = 0;
        return HXR_OK;
     }


    bSynched = TRUE;
    ulSequenceNumber = ntohs(*(UINT16*)(pByte+2));
    ulTimestamp      = m_pSynchData [unStream].m_ulRTPInfoTime;
    m_pSynchData [unStream].m_bSynched = TRUE;

    return HXR_OK;
}

STDMETHODIMP
RTPInfoSynch::OnRTCPPacket (IHXBuffer* pRTCPPacket,
			    UINT16 unStream)
{
    if (m_bHaveAllSRs)
    {
	return HXR_OK;
    }
    if (!m_pSynchData)
    {
	return HXR_NOT_INITIALIZED;
    }

    if (unStream >= m_unStreamCount)
    {
	return HXR_INVALID_PARAMETER;
    }

    if (m_pSynchData [unStream].m_bHasSR)
    {
	return HXR_OK;
    }

    if (pRTCPPacket->GetSize() < 20)
    {
	return HXR_INVALID_PARAMETER;
    }

    BYTE* pcRTCP = pRTCPPacket->GetBuffer();

    if (!pcRTCP)
    {
	return HXR_INVALID_PARAMETER;
    }

    // make sure it's SR
    if (RTCP_SR != *(pcRTCP+1))
    {
	return HXR_IGNORE;
    }

    //Compute the RTP to NTP mapping from the RTCP SR for this stream::
    pcRTCP += 8;

    NTPTime rtcpNTPTime;
    UINT32 rtcpRTPTimeMSec = 0;

    /* Truncate NTP time to 32 bits from 64 bits to make room for expansion to milliseconds*/
    rtcpNTPTime.m_ulSecond = (ntohl(*(unsigned int*)(pcRTCP))) & 0x0000ffff;
    rtcpNTPTime.m_ulFraction = (ntohl(*(unsigned int*)(pcRTCP+4))) & 0xffff0000;

    rtcpRTPTimeMSec =
	(m_pSynchData [unStream].m_pTSConverter) ?

	m_pSynchData [unStream].m_pTSConverter->
	rtp2hxa_raw(ntohl(*(unsigned int*)(pcRTCP+8))) :

	ntohl(*(unsigned int*)(pcRTCP+8));

    m_pSynchData [unStream].m_lRTPtoNTPOffset =
      (INT32)(rtcpNTPTime.toMSec() - rtcpRTPTimeMSec);

    m_pSynchData [unStream].m_bHasSR = TRUE;
    m_bHaveAllSRs = (++m_unSRCount >= m_unStreamCount);
    return HXR_OK;
}

STDMETHODIMP
RTPInfoSynch::Done ()
{
    HX_VECTOR_DELETE(m_pSynchData);

    return HXR_OK;
}
