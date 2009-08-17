/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtcputil.cpp,v 1.21 2006/05/19 06:06:59 pankajgupta Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hlxclib/string.h"
#include "bufnum.h"
#include "rtpwrap.h"
#include "hxtick.h"
#include "hxengin.h"
#include "tconverter.h"
#include "netbyte.h"

#include "interval.h"
#include "ntptime.h"
#include "rtcputil.h"

#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

//#define DUMP_REPORTS
//#define DUMP_RECEIVE_REPORTS
//#define DUMP_MEMBER_COUNT

/* Used in UpdateSeqNo */
const UINT32 MAX_DROPOUT = 3000;
const UINT32 MAX_MISORDER = 100;
const UINT32 MIN_SEQUENTIAL = 2;
const UINT32 RTP_SEQ_MOD = (1 << 16);


//ReportHandler::ReportHandler(HXBOOL bIsSender, HXBOOL bIsReceiver, UINT32 ulSsrc, UINT32 ulDefaultProbation)
ReportHandler::ReportHandler(HXBOOL bIsSender, HXBOOL bIsReceiver, UINT32 ulSsrc)
    : m_pReceiverMe(NULL)
    , m_pSenderMe(NULL)
    , m_ulMySsrc(ulSsrc)
//    , m_ulDefaultProbation(ulDefaultProbation)
    , m_ulAvgRTCPSize(128)
    , m_bInitialIntervalCalc(TRUE)
    , m_ulRSByteRate (8)
    , m_ulRRByteRate (32)
    , m_minRTCPInterval(0.0)
	, m_pNTPBase(NULL)
	, m_nRTPTSBase(0)
	, m_pTSConverter(NULL)	
{
    /* it is "either or" in RealSystem */
    HX_ASSERT((bIsSender || bIsReceiver) && !(bIsSender && bIsReceiver));

    if (bIsSender)
    {
	m_pSenderMe = new MyselfAsSender();
	m_pSenderMe->m_ulSsrc = ulSsrc;
    }
    else
    {
	m_pReceiverMe = new MyselfAsReceiver();
	m_pReceiverMe->m_ulSsrc = ulSsrc;;
    }
}

ReportHandler::~ReportHandler()
{	      
    CHXMapLongToObj::Iterator i;
    for (i = m_mapReceivers.Begin(); i != m_mapReceivers.End(); ++i)
    {
	delete (ReceiverInfo*)(*i);
    }	    
    for (i = m_mapSenders.Begin(); i != m_mapSenders.End(); ++i)
    {
	delete (ReceptionInfo*)(*i);
    }
    HX_DELETE(m_pSenderMe);
    HX_DELETE(m_pReceiverMe);
    HX_DELETE(m_pNTPBase);
    HX_DELETE(m_pTSConverter);
}

void    
ReportHandler::Init(REF(Timeval) tvInitial, 
		    INT64 nInitialRTP,
		    CHXTimestampConverter* pConverter)
{
    HX_ASSERT(!m_pNTPBase && "Are you sure to reset this?");

    HX_DELETE(m_pNTPBase);
    HX_DELETE(m_pTSConverter);

    m_pNTPBase = new NTPTime(tvInitial);    
    m_nRTPTSBase = nInitialRTP;

    if (pConverter)
    {
	m_pTSConverter = new CHXTimestampConverter();
	*m_pTSConverter = *pConverter;
    }
}

void
ReportHandler::OnRTPReceive(UINT32 ulSsrc, UINT16 unSeqNo, 
			    UINT32 ulHXTimestamp, UINT32 ulNow)
{
    HX_ASSERT(m_pReceiverMe && !m_pSenderMe);
    
    ReceptionInfo* pRInfo = GetOrCreateReceptionInfo(ulSsrc); 

    pRInfo->m_bHeardSinceLastTime = TRUE;

    // in the same unit
    INT32 lTransit = (INT32)(ulNow - ulHXTimestamp);

    if (0 == pRInfo->m_ulNumPktReceived)
    {
	pRInfo->InitSeqNo(unSeqNo);
	/* so it won't be some crazy number */
	pRInfo->m_ulTransit = (UINT32)lTransit;
    }

    // all updates will be done here.
    pRInfo->UpdateSeqNo(unSeqNo);

    /************************
    * calculate jitter (rfc 1889 Appendix A.8)
    * this doesn't belong to UpdateSeqNo() so just do it here...
    */    
    INT32 lDiff = (INT32)(lTransit - pRInfo->m_ulTransit);
    pRInfo->m_ulTransit = (UINT32)lTransit;
    if (lDiff < 0)
    {
	lDiff = -lDiff;
    }
    pRInfo->m_ulJitter += lDiff - ((pRInfo->m_ulJitter + 8) >> 4);
}

void 
ReportHandler::OnRTCPReceive(RTCPPacket* pPkt, UINT32 ulNow)
{
    HX_ASSERT(m_pSenderMe || m_pReceiverMe);
#ifdef DUMP_RECEIVE_REPORTS    
    printf("\n%u", pPkt->packet_type);
#endif
    if (RTCP_SR == pPkt->packet_type)
    {
	HX_ASSERT(m_pReceiverMe);
	// it IS possible to get RTCP before RTP if this is multicast.
	ReceptionInfo* pRInfo = GetOrCreateReceptionInfo(pPkt->sr_ssrc);

	// the middle 32 bits out of 64 in the NTP timestamp
	pRInfo->m_ulLSR = pPkt->ntp_sec  << 16;
	pRInfo->m_ulLSR |= (pPkt->ntp_frac >> 16);	
	pRInfo->m_ulLastSRReceived = ulNow;
	pRInfo->m_bHeardSinceLastTime = TRUE;
#ifdef DUMP_RECEIVE_REPORTS
    	printf("\tSR %u:\n", pPkt->sr_ssrc);
    	printf("\t\trtp_ts:  %u\n", pPkt->rtp_ts);
    	printf("\t\tntp: %u: %u\n", pPkt->ntp_sec, pPkt->ntp_frac);
    	printf("\t\tpsent: %u osent: %u\n", pPkt->psent, pPkt->osent);
    	fflush(stdout);
#endif    	
    } 
    else if (RTCP_RR == pPkt->packet_type)
    {
	// just need to keep track of them.  Ignore the returned value...
	GetOrCreateReceiverInfo(pPkt->rr_ssrc);
#ifdef DUMP_RECEIVE_REPORTS
	printf("\tRR %u\n", pPkt->rr_ssrc);
	for (UINT32 i = 0; i < pPkt->count; i++)
	{
	    ReceptionReport rr = pPkt->rr_data[i];
    	    printf("\t\tssrc: %u\n", rr.ssrc);
    	    printf("\t\tlast_seq: %u\n", rr.last_seq);
    	    printf("\t\tlost: %u\n", rr.lost);
    	    printf("\t\tfraction: %u\n", rr.fraction);
    	    printf("\t\tjitter: %u\n", rr.jitter);
    	    printf("\t\tlsr: %u\n", rr.lsr);
    	    printf("\t\tdlsr: %u\n", rr.dlsr);
	}    	    
    	fflush(stdout);
#endif    	
    }
    else if (RTCP_BYE == pPkt->packet_type)
    {
	UINT32 ulSsrc;
	for (UINT32 i = 0; i < pPkt->count; i++)
	{
	    ulSsrc = *(pPkt->bye_src + i);

	    // remove this entry
	    DeleteReceiverInfo(ulSsrc);
	    DeleteReceptionInfo(ulSsrc);
	}	    
    }
#ifdef DUMP_RECEIVE_REPORTS
    else if (RTCP_SDES == pPkt->packet_type)
    {
	printf("\tSDES\n");
	SDESItem* pItem = NULL;
	CHXSimpleList* pSdesList = NULL;
	CHXSimpleList::Iterator j;
    	CHXMapLongToObj::Iterator i;
    	for (i = pPkt->m_mapSDESSources.Begin(); i != pPkt->m_mapSDESSources.End(); ++i)
    	{
    	    pSdesList = (CHXSimpleList*)(*i);
    	    for (j = pSdesList->Begin(); j != pSdesList->End(); ++j)
    	    {
    		pItem = (SDESItem*)(*j);
    		printf("\t\ttype %u: %s\n", pItem->sdes_type, pItem->data);
    	    }
    	}	    
    }
    printf("\n");    
    fflush(stdout);
#endif    
}

HX_RESULT
ReportHandler::MakeSR(RTCPPacket* pPkt, UINT32 ulNow)
{
    Timeval tvNow((INT32)(ulNow / 1000), (INT32)(ulNow % 1000 * 1000));
    return MakeSR(pPkt, tvNow);    
}


HX_RESULT
ReportHandler::MakeSR(RTCPPacket* pPkt, REF(Timeval) tvNow)
{
    HX_ASSERT(m_pSenderMe);
    HX_ASSERT(pPkt);    

    if (!m_pSenderMe->m_bWeSent)
    {
	// no pkt has been sent, wait!
	return HXR_UNEXPECTED;
    }

    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_SR;
    pPkt->sr_ssrc = m_pSenderMe->m_ulSsrc;
    pPkt->psent = m_pSenderMe->m_ulNumPktSentSoFar;
    pPkt->osent = m_pSenderMe->m_ulNumByteSentSoFar;
    /* since a sender is never a receiver */
    pPkt->count = 0;
    pPkt->sr_data = NULL;
    pPkt->length = 6;


    /* NTP time when this report is generated */
    NTPTime ntpNow(tvNow);

    /*
     * RTP TS that corresponds to NTP time above  
     * m_pNTPBase and m_nRTPTSBase are the same time in diff unit
     */
    INT64 nRTPNow = m_nRTPTSBase;

    if (m_pTSConverter)
    {
	nRTPNow += m_pTSConverter->hxa2rtp((UINT32)(ntpNow - *m_pNTPBase));
    }
    else
    {
	nRTPNow += (UINT32)(ntpNow - *m_pNTPBase);
    }


    HX_ASSERT(nRTPNow >= 0);

    // NTP
    pPkt->ntp_sec  = ntpNow.m_ulSecond;
    pPkt->ntp_frac = ntpNow.m_ulFraction;

    // RTP
    pPkt->rtp_ts   = INT64_TO_UINT32(nRTPNow);

#ifdef DUMP_REPORTS
    printf("SR %u:\n", pPkt->sr_ssrc);
    printf("m_nRTPTSBase: %u\n", m_nRTPTSBase);
    printf("m_pNTPBase: %u\n", (UINT32)m_pNTPBase);
    printf("\trtp_ts:  %u\n", pPkt->rtp_ts);
    printf("\tntp: %u: %u\n", pPkt->ntp_sec, pPkt->ntp_frac);
    printf("\tpsent: %u osent: %u\n", pPkt->psent, pPkt->osent);
    fflush(stdout);
#endif    

    return HXR_OK;
}

/*
*   Make RR.  We need this even if there is no reception report
*/
HX_RESULT 
ReportHandler::MakeRR(RTCPPacket* pPkt, UINT32 ulNow)
{
//    Timeval tvNow((INT32)(ulNow / 1000), (INT32)(ulNow % 1000 * 1000));
//    return MakeRR(pPkt, tvNow);        
    HX_ASSERT(m_pReceiverMe); 
    HX_ASSERT(pPkt);    

    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_RR;
    pPkt->rr_ssrc = m_pReceiverMe->m_ulSsrc;

    // can't be more than this
    ReceptionReport* pRr = new ReceptionReport[m_mapSenders.GetCount()];
    if (!pRr)
    {
	return HXR_OUTOFMEMORY;
    }

#ifdef DUMP_REPORTS
    printf("RR %u:\n", pPkt->rr_ssrc);
#endif
    UINT8   chRRCount = 0;
    ReceptionInfo* pRInfo = NULL;
    CHXMapLongToObj::Iterator i;
    for (i = m_mapSenders.Begin(); i != m_mapSenders.End(); ++i)
    {
	pRInfo = (ReceptionInfo*)(*i);
	if (pRInfo->m_bHeardSinceLastTime)
	{
	    // need to make a report for this sender
	    pRInfo->MakeReceptionReport((UINT32)i.get_key(), pRr[chRRCount++], ulNow);
	    pRInfo->m_bHeardSinceLastTime = FALSE;
	}
    }	        

    pPkt->count = chRRCount;
    pPkt->length = (UINT16)(1 + 6 * (UINT16)pPkt->count);
    pPkt->SetReceiverReport(pRr, pPkt->count);

    // SetReceiverReport is making a copy.
    HX_VECTOR_DELETE(pRr);
    
    return HXR_OK;

}

/*
*   CNAME is the only one required
*/
HX_RESULT
ReportHandler::MakeSDES(RTCPPacket* pPkt, const BYTE* pcCNAME)
{
    HX_ASSERT(m_pSenderMe || m_pReceiverMe);
    HX_ASSERT(pPkt);
    HX_ASSERT(pcCNAME);
    
    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_SDES;
    pPkt->count = 1;
    
    UINT16 unByteCount = 0;
    
    SDESItem item;
    item.sdes_type = SDES_CNAME;
    item.length = (UINT8)strlen((const char*)pcCNAME);
    item.data = (BYTE*)pcCNAME;
    if (m_pSenderMe)
    {
	pPkt->AddSDESItem(m_pSenderMe->m_ulSsrc, item);
    }
    else
    {
	pPkt->AddSDESItem(m_pReceiverMe->m_ulSsrc, item);	
    }

    // 2 for sdes_type and length
    unByteCount += (UINT16)(item.length + 2);

    /*
     * Increment item byte count for null termination
     */
    unByteCount++;
    
    // Align on word boundary
    // RTCP pkt length is in 32-bits word!
    unByteCount += (UINT16)((unByteCount % 4) ? 4 - (unByteCount % 4) : 0);

    HX_ASSERT(unByteCount % 4 == 0);
    // I am counting 32-bits word
    pPkt->length = (UINT16)(unByteCount / 4);	    // count of words - 1

    //one more 32-bits for SSRC
    pPkt->length++;
        
    return HXR_OK;
}

HX_RESULT
ReportHandler::MakeBye(RTCPPacket* pPkt)
{
    HX_ASSERT(m_pSenderMe || m_pReceiverMe);
    HX_ASSERT(pPkt);
    
    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_BYE;    
    pPkt->length = 1;   // len in 32-bits words minus one
    pPkt->count = 1;
    // use access function
    if (m_pSenderMe)
    {
	pPkt->SetByeSrc(&m_pSenderMe->m_ulSsrc, pPkt->count);        
#ifdef DUMP_REPORTS
	printf("BYE %u:\n", m_pSenderMe->m_ulSsrc);
#endif	
    }
    else
    {
    	pPkt->SetByeSrc(&m_pReceiverMe->m_ulSsrc, pPkt->count);        
#ifdef DUMP_REPORTS
	printf("BYE %u\n", m_pReceiverMe->m_ulSsrc);
#endif	
    }

#ifdef DUMP_REPORTS
    fflush(stdout);
#endif    
    return HXR_OK;
}

/*
*   for now this is the only APP.  
*/
HX_RESULT
ReportHandler::MakeEOSApp(RTCPPacket* pPkt)
{
    HX_ASSERT(m_pSenderMe);
    HX_ASSERT(pPkt);

    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_APP;
    pPkt->app_ssrc = m_pSenderMe->m_ulSsrc;
    pPkt->count = 1;
    pPkt->length = 4;   // 2 + the length of APPItem...fortunately, we don't
                        // have to align them
    memcpy(pPkt->app_name, "RNWK", 4); /* Flawfinder: ignore */

    // this is application dependent...
    APPItem item;
    item.app_type = APP_EOS;
    item.seq_no = m_pSenderMe->m_unLastSeqNo;
    item.packet_sent = (UINT8)(m_pSenderMe->m_ulNumPktSentSoFar ? 1 : 0);
    item.timestamp = m_pSenderMe->m_ulLastRTPTimestamp;

    pPkt->SetAPPItem(&item, pPkt->count);

    return HXR_OK;
}

HX_RESULT
ReportHandler::MakeBufInfoApp(RTCPPacket* pPkt, 
			      UINT32 ulLowTS, UINT32 ulHighTS,
			      UINT32 ulBytesBuffered)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (pPkt)
    {
	pPkt->version_flag = 0x02;
	pPkt->padding_flag = 0;    
	pPkt->packet_type = RTCP_APP;
	pPkt->count = 1;
	pPkt->app_ssrc = m_pReceiverMe->m_ulSsrc;
	memcpy(pPkt->app_name, "HELX", 4);
	pPkt->length = 2;

	APPItem item;
	item.app_type = APP_BUFINFO;
	item.lowest_timestamp = ulLowTS;
	item.highest_timestamp = ulHighTS;
	item.bytes_buffered = ulBytesBuffered;
	item.padding0 = 0;
	pPkt->SetAPPItem(&item, 1);
	pPkt->length += 4;
	res = HXR_OK;
    }

    return res;
}

HX_RESULT 
ReportHandler::MakeNADU(RTCPPacket* pPkt,
                        HXBOOL bNextSeqValid,
                        UINT16 uPlayoutDelay,
                        UINT16 uNextSeqNumber,
                        UINT16 uNextUnitNumber,
                        UINT16 uFreeBufferSpace)
{
    HX_RESULT res = HXR_UNEXPECTED;

    // We only handle the 1 sender case for
    // right now.
    if (pPkt && (m_mapSenders.GetCount() == 1))
    {
        CHXMapLongToObj::Iterator i = m_mapSenders.Begin();
        UINT32 ulSSRC = (UINT32)i.get_key();

        if (!bNextSeqValid)
        {
            ReceptionInfo* pRInfo = (ReceptionInfo*)(*i);
            
            // We don't have any valid sequence number info so
            // we are going to report the next expected sequence
            // number.
            uNextSeqNumber = (UINT16)((pRInfo->m_ulCycles + 
                              pRInfo->m_unMaxSeqNo + 1) & 0xffff);

            // We don't know the playout delay so just set it to
            // the reserved value
            uPlayoutDelay = 0xffff;

            // We don't have a unit number so just set it to 0
            uNextUnitNumber = 0;

            // uFreeBufferSpace is expected to have a valid value in
            // this situation
        }
        
        HXLOGL3(HXLOG_TRAN, "NADU %08x %5u %5u %5u %5u (%5u)\n", 
                ulSSRC, uPlayoutDelay, uNextSeqNumber, uNextUnitNumber,
                uFreeBufferSpace,
                uFreeBufferSpace * 64);

	pPkt->version_flag = 0x02;
	pPkt->padding_flag = 0;    
	pPkt->packet_type = RTCP_APP;
	pPkt->count = 0;
	pPkt->app_ssrc = m_pReceiverMe->m_ulSsrc;
	memcpy(pPkt->app_name, "PSS0", 4);
        pPkt->length = 2;

        UINT16 naduBuf[6];

        UINT32* pSSRC = (UINT32*)&naduBuf[0];
        *pSSRC = DwToNet(ulSSRC);
        naduBuf[2] = WToNet(uPlayoutDelay);
        naduBuf[3] = WToNet(uNextSeqNumber);
        naduBuf[4] = WToNet((UINT16)(uNextUnitNumber & 0x1f));
        naduBuf[5] = WToNet(uFreeBufferSpace);

        pPkt->SetAPPData((const UINT8*)naduBuf, sizeof(naduBuf));
	res = HXR_OK;
    }

    return res;
}

ReceiverInfo*
ReportHandler::GetOrCreateReceiverInfo(UINT32 ulSsrc)
{
    ReceiverInfo* pReceiver = NULL;
    if (!m_mapReceivers.Lookup((LONG32)ulSsrc, (void*&)pReceiver))
    {
#ifdef DUMP_MEMBER_COUNT    
	printf("New Receiver (#%u): %u\n", m_mapReceivers.GetCount()+1, ulSsrc);
	fflush(stdout);
#endif	
	// doesn't exist, create one.
	pReceiver = new ReceiverInfo;
	if (!pReceiver)
	{
	    return NULL;
	}
	m_mapReceivers.SetAt((LONG32)ulSsrc, (void*)pReceiver);
#if _DEBUG
	ReceiverInfo* pTmp = NULL;
	HX_ASSERT(m_mapReceivers.Lookup((LONG32)ulSsrc, (void*&)pTmp));	
	HX_ASSERT(pTmp);
#endif		
    }

    HX_ASSERT(pReceiver);
    return pReceiver;
}

void
ReportHandler::DeleteReceiverInfo(UINT32 ulSsrc)
{
    /*	since we need to do timeout, we might as well do the right thing for
    *	this as well....mark as delete, and delete after a fixed timeout...so, 
    *	we know we don't receive any late pkt after we delete this entry...    
    */

    ReceiverInfo* pReceiver = NULL;
    if (m_mapReceivers.Lookup((LONG32)ulSsrc, (void*&)pReceiver))
    {
#ifdef DUMP_MEMBER_COUNT    
	printf("Deleteing Receiver: %u\n", ulSsrc);
	fflush(stdout);
#endif	
	HX_ASSERT(pReceiver);
	m_mapReceivers.RemoveKey((LONG32)ulSsrc);
	delete pReceiver;
    }    
}

ReceptionInfo*
ReportHandler::GetOrCreateReceptionInfo(UINT32 ulSsrc)
{
    ReceptionInfo* pRInfo = NULL;
    if (!m_mapSenders.Lookup((LONG32)ulSsrc, (void*&)pRInfo))
    {
#ifdef DUMP_MEMBER_COUNT    
	printf("New Sender (#%u): %u\n", m_mapSenders.GetCount()+1, ulSsrc);
	fflush(stdout);
#endif	
	// doesn't exist, create one.
	pRInfo = new ReceptionInfo();
	if (!pRInfo)
	{
	    return NULL;
	}
//	pRInfo->m_ulProbation = m_ulDefaultProbation;
	
	m_mapSenders.SetAt((LONG32)ulSsrc, (void*)pRInfo);
#if _DEBUG
	ReceptionInfo* pTmp = NULL;
	HX_ASSERT(m_mapSenders.Lookup(ulSsrc, (void*&)pTmp));	
	HX_ASSERT(pTmp);
#endif		
    }

    HX_ASSERT(pRInfo);
    return pRInfo;
}

void
ReportHandler::DeleteReceptionInfo(UINT32 ulSsrc)
{
    /*	since we need to do timeout, we might as well do the right thing for
    *	this as well....mark as delete, and delete after a fixed timeout...so, 
    *	we know we don't receive any late pkt after we delete this entry...    
    */

    ReceptionInfo* pRInfo= NULL;
    if (m_mapSenders.Lookup((LONG32)ulSsrc, (void*&)pRInfo))
    {
#ifdef DUMP_MEMBER_COUNT    
	printf("Deleteing Sender: %u\n", ulSsrc);
	fflush(stdout);
#endif	    
	HX_ASSERT(pRInfo);
	m_mapSenders.RemoveKey((LONG32)ulSsrc);
	delete pRInfo;
    }    
}

/*
*/
double
ReportHandler::GetRTCPInterval()
{
    // include myself
    double interval = 
    rtcp_interval(m_mapReceivers.GetCount() + 1,
		  m_pSenderMe ? m_mapSenders.GetCount()+1 : m_mapSenders.GetCount(),
		  (double)m_ulRSByteRate,
		  (double)m_ulRRByteRate,
		  m_pSenderMe ? m_pSenderMe->m_bWeSent : 0,
		  m_ulAvgRTCPSize,
		  m_bInitialIntervalCalc,
		  m_minRTCPInterval);

    m_bInitialIntervalCalc = FALSE;	
    return interval;
}

void ReportHandler::SetRTCPIntervalParams(UINT32 ulRSBitRate, 
					  UINT32 ulRRBitRate,
					  UINT32 ulMinRTCPIntervalMs)
{
    m_ulRSByteRate = ulRSBitRate >> 3; // bitrate -> byterate
    m_ulRRByteRate = ulRRBitRate >> 3; // bitrate -> byterate
    m_minRTCPInterval = ((double)ulMinRTCPIntervalMs) / 1000.0; // ms -> sec
}

/*
*   ReceptionInfo Func's
*/

void
ReceptionInfo::InitSeqNo(UINT16 unSeqNo)
{
#ifdef _DEBUG
    HX_ASSERT(INIT == m_state);
    m_state = UPDATE;
#endif    
//    m_ulBaseSeqNo = unSeqNo - 1;
    m_ulBaseSeqNo = unSeqNo;
    m_unMaxSeqNo =  unSeqNo;
    m_ulBadSeqNo =  RTP_SEQ_MOD + 1;
    m_ulCycles =    0;
    m_ulNumPktReceived = 0;    
    m_ulExpectedPrior  = 0;
    m_ulReceivedPrior  = 0;        
}

HXBOOL
ReceptionInfo::UpdateSeqNo(UINT16 unSeqNo)
{   
#ifdef _DEBUG
    HX_ASSERT(UPDATE == m_state);
#endif

    UINT16 unDelta = (UINT16)(unSeqNo - m_unMaxSeqNo);

#if 0 /* don't ever throu pkt away! */
    if (m_ulProbation)
    {
	if (unSeqNo == m_unMaxSeqNo + 1)
	{
	    // pkt is in sequence
	    m_ulProbation--;
	    m_unMaxSeqNo = unSeqNo;
	    if (0 == m_ulProbation)
	    {	
		InitSeqNo(unSeqNo);
		m_ulNumPktReceived++;
		return TRUE;
	    }
	}
	else
	{ 
	    // pkt is NOT in sequence
	    m_ulProbation = MIN_SEQUENTIAL - 1;
	    m_unMaxSeqNo = unSeqNo;
	}
	return FALSE;
    }
    else 
#endif    
    if (unDelta < MAX_DROPOUT)
    {
	// in order, with permissible gap
	if (unSeqNo < m_unMaxSeqNo)
	{
	    // seq# wrapped - count another 64k cycle
	    m_ulCycles += RTP_SEQ_MOD;	    
	}
	
	m_unMaxSeqNo = unSeqNo;
    }
    else if (unDelta <= RTP_SEQ_MOD - MAX_MISORDER)
    {
	// seq# made a very large jump
	if (unSeqNo == m_ulBadSeqNo)
	{
	    // two sequential pkts -- assume that the other side
	    // restarted w/o telling us, so just re-sync
	    // (i.e., pretned this was the first pkt)
	    InitSeqNo(unSeqNo);
	}
	else
	{
	    
	    m_ulBadSeqNo = (unSeqNo + 1) & (RTP_SEQ_MOD - 1);
	    // (i.e. m_ulBadSeq = unSeq + 1;)
	    return FALSE;
	}
    }
    else 
    {
	/* duplicate or reordered packet */
    }

    m_ulNumPktReceived++;    

    return TRUE;
}


void
ReceptionInfo::MakeReceptionReport(UINT32 ulSsrc, REF(ReceptionReport) rr, UINT32 ulNow)
{
#ifdef DUMP_REPROTS
    printf("making rr for %u\n", ulSsrc);
    fflush(stdout);
#endif    

    // ssrc of a sender that we are reporting
    rr.ssrc = ulSsrc;
    
    /* extended last seqno received */
    rr.last_seq = m_ulCycles + m_unMaxSeqNo; 

    /* simply #pkt expected - received */
    UINT32 ulExpected = rr.last_seq - m_ulBaseSeqNo + 1;
    rr.lost =  ulExpected - m_ulNumPktReceived; 

    UINT32 ulExpectedInterval = ulExpected - m_ulExpectedPrior;
    UINT32 ulReceivedInterval = m_ulNumPktReceived - m_ulReceivedPrior;
    INT32  lLostInterval = (INT32)(ulExpectedInterval - ulReceivedInterval);

    // save them for the next time
    m_ulExpectedPrior = ulExpected;
    m_ulReceivedPrior = m_ulNumPktReceived;

    /*
     * The resulting fraction is an 8-bit fixed point number with the binary
     * point at the left edge.
     */  
    if (0 == ulExpectedInterval || lLostInterval <= 0)
    {
	rr.fraction = 0;
    }
    else
    {
	rr.fraction = (UINT8)((lLostInterval << 8) / ulExpectedInterval);
    }
    
    rr.jitter = m_ulJitter >> 4;
    rr.lsr = m_ulLSR ? m_ulLSR : 0;
    /* expressed in 1/65536 sec...ahhh...make it 66 */
    rr.dlsr = (m_ulLastSRReceived ? 
		CALCULATE_ELAPSED_TICKS(m_ulLastSRReceived, ulNow) : 
		0 /* SR not yet received */) * 66;
		

#ifdef DUMP_REPROTS
    printf("\tssrc: %u\n", rr.ssrc);
    printf("\tlast_seq: %u\n", rr.last_seq);
    printf("\tlost: %u\n", rr.lost);
    printf("\tfraction: %u\n", rr.fraction);
    printf("\tjitter: %u\n", rr.jitter);
    printf("\tlsr: %u\n", rr.lsr);
    printf("\tdlsr: %u\n", rr.dlsr);
    fflush(stdout);
#endif    
}

CReflectionHandler::CReflectionHandler(CHXTimestampConverter* pConverter)
    :m_SR_SSRC(0),
    m_SR_NTP_secs(0),
    m_SR_NTP_fraction_secs(0),
    m_SR_RTP_Timestamp(0),
    m_RTP_Timestamp(0),
    m_RTP_SeqNo(0),
    m_PacketCount(0),
    m_OctetCount(0),
    m_SDES_SSRC(0),
    m_pcCNAME(NULL),
    m_bByeSent(FALSE),
    m_pTSConverter(NULL),
    m_LastRTCPUpdateSchedulerTime(0,0)
{
    if (pConverter)
    {
        m_pTSConverter = new CHXTimestampConverter();
        *m_pTSConverter = *pConverter;
    }
}

CReflectionHandler::~CReflectionHandler()
{
    HX_VECTOR_DELETE(m_pcCNAME);
    HX_DELETE(m_pTSConverter);
}

void CReflectionHandler::OnRTCPPacket(IHXBuffer* pBuffer, Timeval timeval)
{
	if (pBuffer == NULL)
	{
	    return;
	}

	//Validate packet.
	BYTE* pRTCPData = pBuffer->GetBuffer();
	if (pRTCPData == NULL)
	{
	    return;
	}

	const UINT32 kRTCPHeaderSize = 4;
	if (pBuffer->GetSize() < kRTCPHeaderSize )
	{
	    return;
	}
	
	//Save Packet End.
	BYTE* pRTCPDataEnd = pRTCPData + pBuffer->GetSize();

	UCHAR ucReportType = 0;
	BYTE* pSRReport = NULL;
	BYTE* pSDESReport = NULL;
	UINT16 uLength = 0;

	while (pRTCPData + kRTCPHeaderSize <= pRTCPDataEnd)
	{
	    //Make pRTCPData point to RTCP packet type
	    ++pRTCPData;
	    ucReportType = *(pRTCPData);

	    //Make pRTCPData point to length of single RTCP packet in a compound RTCP packet.
	    pRTCPData += 1;

	    uLength = GetWordFromBufAndInc(pRTCPData);

	    if (pRTCPData + uLength*sizeof(UINT32) > pRTCPDataEnd)
	    {
		    //Invalid length.
		    break;
	    }

	    if (ucReportType == RTCP_SR)
	    {
		    //Save SR information.
		    SetRTCP_SR(pRTCPData, uLength, timeval, pRTCPDataEnd);
	    }
	    else if (ucReportType == RTCP_SDES && m_pcCNAME == NULL)
	    {
		    SetRTCP_SDES(pRTCPData, uLength);
	    }
	    else if (ucReportType == RTCP_BYE)
	    {
		    m_bByeSent = TRUE;
	    }

	    pRTCPData += uLength * sizeof(UINT32);
	}

	return;
}


void CReflectionHandler::OnRTPPacket(UINT32 ulOctetCount,UINT32 ulRTPTime, UINT32 ulSeqNo)
{
    m_RTP_Timestamp = ulRTPTime;
    m_RTP_SeqNo = ulSeqNo;
    m_PacketCount += 1;
    m_OctetCount += ulOctetCount;
}

HX_RESULT CReflectionHandler::MakeSR(RTCPPacket* pPkt, Timeval timeval)
{
    HX_ASSERT(pPkt);    

    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_SR;
    pPkt->sr_ssrc = m_SR_SSRC;
    pPkt->psent = m_PacketCount;
    pPkt->osent = m_OctetCount;
    pPkt->length = 6;

    /* since a sender is never a receiver */
    pPkt->count = 0;
    pPkt->sr_data = NULL;

    // NTP
    NTPTime ntpTemp1 = NTPTime(timeval);
    NTPTime ntpTemp2 = NTPTime(m_LastRTCPUpdateSchedulerTime);

    NTPTime ntpOffset = ntpTemp1 - ntpTemp2;
    NTPTime ntpLastTime = NTPTime(m_SR_NTP_secs, m_SR_NTP_fraction_secs);

    ntpLastTime += ntpOffset;

    pPkt->ntp_sec  = ntpLastTime.m_ulSecond;
    pPkt->ntp_frac = ntpLastTime.m_ulFraction;

    UINT32 ulRTPOffset = m_pTSConverter->hxa2rtp(ntpOffset);
    
    // RTP
    pPkt->rtp_ts   = m_SR_RTP_Timestamp + ulRTPOffset;

    return HXR_OK;
}

HX_RESULT CReflectionHandler::MakeSDES(RTCPPacket* pPkt)
{
    HX_ASSERT(pPkt);
    
    if (!m_pcCNAME)
    {
    	return HXR_FAIL;
    }
    
    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_SDES;
    pPkt->count = 1;
    
    UINT16 unByteCount = 0;
    
    SDESItem item;
    item.sdes_type = SDES_CNAME;
    item.length = strlen((const char*)m_pcCNAME);
    item.data = (BYTE*)m_pcCNAME;
    pPkt->AddSDESItem(m_SDES_SSRC, item);

    // 2 for sdes_type and length
    unByteCount += item.length + 2;

    /*
     * Increment item byte count for null termination
     */
    unByteCount++;
    
    // Align on word boundary
    // RTCP pkt length is in 32-bits word!
    unByteCount += (unByteCount % 4) ? 4 - (unByteCount % 4) : 0;

    HX_ASSERT(unByteCount % 4 == 0);
    // I am counting 32-bits word
    pPkt->length = (unByteCount / 4);	    // count of words - 1

    //one more 32-bits for SSRC
    pPkt->length++;
        
    return HXR_OK;
}

HX_RESULT CReflectionHandler::MakeBye(RTCPPacket* pPkt)
{
    HX_ASSERT(pPkt);
    
    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_BYE;    
    pPkt->length = 1;   // len in 32-bits words minus one
    pPkt->count = 1;
    // use access function
    pPkt->SetByeSrc(&m_SR_SSRC, pPkt->count);        

#ifdef DUMP_REPORTS
	printf("BYE %u:\n",m_SR_SSRC);
        fflush(stdout);
#endif	
    return HXR_OK;
}

void CReflectionHandler::SetRTCP_SR(BYTE* pSRReport, UINT16 uLength, Timeval timeval, BYTE* pRTCPDataEnd)
{
    const UINT32 kMinSR_length = ( sizeof(m_SR_SSRC) 
				+ sizeof(m_SR_NTP_secs)
				+ sizeof(m_SR_NTP_fraction_secs)
				+ sizeof(m_SR_RTP_Timestamp)
				+ sizeof(m_PacketCount)
				+ sizeof(m_OctetCount))/ sizeof(UINT32);

    const UINT32 kBytesToRead = kMinSR_length * sizeof(UINT32);

    if (uLength >= kMinSR_length && pSRReport + kBytesToRead <= pRTCPDataEnd)
    {
	    //Save SR information.
	    m_SR_SSRC = GetDwordFromBufAndInc(pSRReport);
	    m_SR_NTP_secs = GetDwordFromBufAndInc(pSRReport);
	    m_SR_NTP_fraction_secs = GetDwordFromBufAndInc(pSRReport);
	    m_SR_RTP_Timestamp = GetDwordFromBufAndInc(pSRReport);
	    m_PacketCount = GetDwordFromBufAndInc(pSRReport);
	    m_OctetCount = GetDwordFromBufAndInc(pSRReport);
	    m_LastRTCPUpdateSchedulerTime = timeval;
    }
}

void CReflectionHandler::SetRTCP_SDES(BYTE* pRTCPData, UINT16 uLength)
{
    const UINT32 kSDESPacketLength = uLength * sizeof(UINT32);
    const UINT32 kMinSR_length = sizeof(m_SR_SSRC)/sizeof(UINT32);

    UCHAR uSDES_ItemType = 0;
    UCHAR ulSDES_ItemLength = 0;

    UINT32 uLengthInBytes = uLength*sizeof(UINT32);
    UINT32 uMinimumSDESData = kMinSR_length*sizeof(UINT32) + 
				sizeof(uSDES_ItemType) + 
				sizeof(ulSDES_ItemLength);

    if (uLengthInBytes <= uMinimumSDESData)
    {
	    return;
    }

    //Read SDES SSRC.
    BYTE* pSDESReport = pRTCPData;
    m_SDES_SSRC = GetDwordFromBufAndInc(pSDESReport);

    uSDES_ItemType = *pSDESReport;
    pSDESReport++;

    ulSDES_ItemLength = *pSDESReport;
    pSDESReport++;

    BYTE* pSDESEnd = pRTCPData + uLength*sizeof(UINT32);
   

    while (pSDESReport + ulSDES_ItemLength <= pSDESEnd)
    {
	    if (uSDES_ItemType == SDES_CNAME)
	    {
	        if (m_pcCNAME)
	        {
		        if (strncmp((const char*)m_pcCNAME,(const char*)pSDESReport,ulSDES_ItemLength) == 0)
		        {
		            //No need to re-copy.
		            break;
		        }
		        HX_VECTOR_DELETE(m_pcCNAME);
	        }
	        m_pcCNAME = new CHAR [ulSDES_ItemLength +1];
	        memcpy(m_pcCNAME,(const char*)pSDESReport,ulSDES_ItemLength);
	        m_pcCNAME[ulSDES_ItemLength] = '\0';
	        break;
	    }

        pSDESReport += ulSDES_ItemLength;


        if (pSDESReport == pSDESEnd)
        {
            break;
        }
        uSDES_ItemType = *pSDESReport;
        pSDESReport++;


        if (pSDESReport == pSDESEnd)
        {
            break;
        }
        ulSDES_ItemLength = *pSDESReport;
        pSDESReport++;

    }
}

HX_RESULT
CReflectionHandler::MakeEOSApp(RTCPPacket* pPkt)
{
    HX_ASSERT(pPkt);
    
    pPkt->version_flag = 0x02;
    pPkt->padding_flag = 0;    
    pPkt->packet_type = RTCP_APP;
    pPkt->app_ssrc = m_SR_SSRC;
    pPkt->count = 1;
    pPkt->length = 4;   

    memcpy(pPkt->app_name, "RNWK", 4); /* Flawfinder: ignore */

    // this is application dependent...
    APPItem item;
    item.app_type = APP_EOS;
    item.seq_no = m_RTP_SeqNo;
    item.packet_sent = m_PacketCount ? 1 : 0;
    item.timestamp = m_RTP_Timestamp;

    pPkt->SetAPPItem(&item, pPkt->count);   

    return HXR_OK;
}

