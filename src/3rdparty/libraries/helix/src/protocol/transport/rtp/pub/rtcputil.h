/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtcputil.h,v 1.15 2007/07/06 20:51:42 jfinnecy Exp $
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


/* 
*   Need info about itself (sender or receiver)
*   Need info about everyone else 
*     - One sender and one or more receivers
*     - No sender and one or more receivers
*/

#ifndef _RTCPUTIL_H_
#define _RTCPUTIL_H_

#include "hxmap.h"
#include "hxstring.h"
#include "ihxpckts.h"

class RTCPPacket;
class ReceptionReport;
class NTPTime;
class CHXTimestampConverter;

/* 
 *  Info about myself
 */
class MyselfAsReceiver
{
public:
    MyselfAsReceiver()
	: m_ulSsrc(0)
    {
//	printf("Myself()\n");fflush(stdout);
    }	
    ~MyselfAsReceiver() 
    {
//	printf("~Myself()\n");fflush(stdout);
    }
        
    UINT32			    m_ulSsrc;    
};

/* 
 *  A Sender have a little more var's to keep track of
 */
class MyselfAsSender
{
public:
    MyselfAsSender()
	: m_ulSsrc(0)
	, m_unLastSeqNo(0)
	, m_ulLastRTPTimestamp(0)
	, m_ulNumPktSentSoFar(0)
	, m_ulNumByteSentSoFar(0)
	, m_bWeSent(FALSE)
    {
//	printf("MyselfAsSender()\n");fflush(stdout);
    }

    ~MyselfAsSender()
    {
//	printf("~MyselfAsSender()\n");fflush(stdout);
    }

    UINT32		    m_ulSsrc;
    UINT16		    m_unLastSeqNo;
    UINT32		    m_ulLastRTPTimestamp;
    UINT32		    m_ulNumPktSentSoFar;
    UINT32		    m_ulNumByteSentSoFar;

    /* will be needed for interval calculation */
    HXBOOL			m_bWeSent;
};

/*
 *  We don't have any specific info for this...
 */
typedef HXBOOL ReceiverInfo;

/*
 *  Info about a reception from a sender (Myself is a receiver)
 *  Keep track of stats on incoming RTP!
 */

class ReceptionInfo
{
    enum 
    {
	INIT,
	UPDATE
    } m_state;

public:
    ReceptionInfo()
	: m_state(INIT)
	, m_unMaxSeqNo(0)
	, m_ulCycles(0)
	, m_ulBaseSeqNo(0)
	, m_ulBadSeqNo(0)
	, m_ulNumPktReceived(0)
	, m_ulExpectedPrior(0)
	, m_ulReceivedPrior(0)
	, m_ulLSR(0)
	, m_ulLastSRReceived(0)
	, m_ulTransit(0)
	, m_ulJitter(0)	
//	, m_ulProbation(0)
	, m_bHeardSinceLastTime(FALSE)
    {
	//printf("ReceptionInfo()\n");fflush(stdout);
    }	
    ~ReceptionInfo() 
    {
	//printf("~ReceptionInfo()\n");fflush(stdout);
    }

    /* deal with sequence number - Only on a receiver */
    void InitSeqNo		(UINT16 unSeqNo);
    HXBOOL UpdateSeqNo		(UINT16 unSeqNo);    

    void    MakeReceptionReport	(UINT32 ulSsrc, REF(ReceptionReport) rr, UINT32 ulNow);

    UINT16  m_unMaxSeqNo;	// Higheset SeqNo seen    
    UINT32  m_ulCycles;		// Shifted number of SeqNo cycle 
    UINT32  m_ulBaseSeqNo;	// base SeqNo
    UINT32  m_ulBadSeqNo;	// last bad SeqNo + 1
    UINT32  m_ulNumPktReceived;	// Packets received
    UINT32  m_ulExpectedPrior;	// pkt expected at last interval
    UINT32  m_ulReceivedPrior;	// pkt received at last interval

    UINT32  m_ulLSR;		// last SR time      
    UINT32  m_ulLastSRReceived; // last time we receive SR
    UINT32  m_ulTransit;	// relative transit time for prev pkt
    UINT32  m_ulJitter;		// estimated jitter

//    UINT32  m_ulProbation;	// Sequ.pkts till source is valid
    HXBOOL    m_bHeardSinceLastTime;
};


// this is a utility class that takes care of RTCP stuff
class ReportHandler
{
public:
    ReportHandler() { HX_ASSERT(!"don't use default constractor"); }
    /* for now it is "either or" in RealSystem */
//    ReportHandler(HXBOOL bIsSender, HXBOOL bIsReceiver, UINT32 ulSsrc, UINT32 ulDefaultProbation = 0);    
    ReportHandler(HXBOOL bIsSender, HXBOOL bIsReceiver, UINT32 ulSsrc);    
    ~ReportHandler();

    UINT32  GetSSRC() 
    { 
	return m_pSenderMe ? m_pSenderMe->m_ulSsrc : m_pReceiverMe->m_ulSsrc; 
    }
 
    void SetSSRC(UINT32 ulSSRC) 
    { 
	if (m_pSenderMe) 
	{
	    m_pSenderMe->m_ulSsrc = ulSSRC; 
	}
	else
	{
	    m_pReceiverMe->m_ulSsrc = ulSSRC;
	}
    }

    /* for each RTP pkt sent */
    void OnRTPSend		    (UINT16 unSeqNo, 
				    UINT32 ulPktIncrement, 
				    UINT32 ulByteIncrement,
				    UINT32 ulRTPTimestamp)
    {				    
    	HX_ASSERT(m_pSenderMe && !m_pReceiverMe);
    
    	m_pSenderMe->m_unLastSeqNo		= unSeqNo;
    	m_pSenderMe->m_ulLastRTPTimestamp	= ulRTPTimestamp;
    	m_pSenderMe->m_ulNumPktSentSoFar	+= ulPktIncrement;
    	m_pSenderMe->m_ulNumByteSentSoFar	+= ulByteIncrement;
    	m_pSenderMe->m_bWeSent			= TRUE;
    }
				    
    /* for each RTP pkt received */				    
    void OnRTPReceive		    (UINT32 ulSsrc, 
				     UINT16 unSeqNo, 
				     UINT32 ulHXTimestamp,
				     UINT32 ulNow);

    /* for each RTCP pkt received */
    void OnRTCPReceive		    (RTCPPacket* pPkt, UINT32 ulNow);
    
    /* Set this once before making any report */
    void    Init		    (REF(Timeval) tvInitial, 
				     INT64 nInitialRTP,
				     CHXTimestampConverter* pConverter);

    /* Acquire NTP base offset. */
    NTPTime GetNTPBase() { return *m_pNTPBase; }    

    /* For resetting RTP ts base on seek. */
    void SetRTPBase(INT64 nNewBase) { m_nRTPTSBase = nNewBase; }

    /* make a report */
    HX_RESULT MakeSR                (RTCPPacket* pPkt, UINT32 ulNow); 
    HX_RESULT MakeSR                (RTCPPacket* pPkt, REF(Timeval) tvNow); 
    HX_RESULT MakeRR                (RTCPPacket* pPkt, UINT32 ulNow); 
    HX_RESULT MakeSDES              (RTCPPacket* pPkt, const BYTE* pcCNAME); 
    HX_RESULT MakeBye               (RTCPPacket* pPkt); 
    HX_RESULT MakeEOSApp            (RTCPPacket* pPkt);
    HX_RESULT MakeBufInfoApp	    (RTCPPacket* pPkt, 
				     UINT32 ulLowTS, UINT32 ulHighTS,
				     UINT32 ulBytesBuffered);
    HX_RESULT MakeNADU	             (RTCPPacket* pPkt, 
                                      HXBOOL bNextSeqValid,
                                      UINT16 uPlayoutDelay,
                                      UINT16 uNextSeqNumber,
                                      UINT16 uNextUnitNumber,
                                      UINT16 uFreeBufferSpace);

    /* for RTCP interval calc */
    void    UpdateAvgRTCPSize	    (UINT32 ulCompoundRTCPSize)
    {
	/* par RFC1889 */
	m_ulAvgRTCPSize = (UINT32)((1.0/16.0) * ulCompoundRTCPSize + 
				   (15.0/16.0) * m_ulAvgRTCPSize);

    }
    double  GetRTCPInterval	    ();

    void SetRTCPIntervalParams(UINT32 ulRSBitRate, UINT32 ulRRBitRate,
			       UINT32 ulMinRTCPIntervalMs);
private:
    // create an entry it's not there.
    ReceiverInfo*   GetOrCreateReceiverInfo (UINT32 ulSsrc);
    ReceptionInfo*  GetOrCreateReceptionInfo(UINT32 ulSsrc);
    
    // find and delete an entry.
    void	DeleteReceiverInfo  (UINT32 ulSsrc);
    void	DeleteReceptionInfo (UINT32 ulSsrc);    
private:
    // they are exclusive
    MyselfAsReceiver*	    m_pReceiverMe;
    MyselfAsSender*	    m_pSenderMe;

    UINT32		    m_ulMySsrc;
    
//    UINT32		    m_ulDefaultProbation;    
    /* sender report */
    // m_pMe will be pointing MyselfAsSender if sender
    
    /* receiver report */
    // map of ReceptionInfo's
    CHXMapLongToObj	    m_mapSenders;
    // map of ReceiverInfo's
    CHXMapLongToObj	    m_mapReceivers;

    /* for RTCP interval calc */
    UINT32		    m_ulAvgRTCPSize;
    HXBOOL		    m_bInitialIntervalCalc;
    UINT32                  m_ulRSByteRate;    // Sender RTCP bandwidth
    UINT32                  m_ulRRByteRate;    // Receiver RTCP bandwidth
    double                  m_minRTCPInterval; // seconds

    NTPTime*		    m_pNTPBase;
    INT64		    m_nRTPTSBase;    
    CHXTimestampConverter*  m_pTSConverter;
};

class CReflectionHandler
{
public:
    CReflectionHandler(CHXTimestampConverter* pConverter);

    ~CReflectionHandler();

    void OnRTCPPacket(IHXBuffer* pBuffer, Timeval timeval);

    void OnRTPPacket(UINT32 ulOctetCount, UINT32 ulRTPTime, UINT32 ulSeqNo);

    inline HXBOOL IsByeSent() 
    { 
	    return m_bByeSent;
    }

    HX_RESULT MakeSR(RTCPPacket* pPkt, Timeval timeval);

    HX_RESULT MakeSDES(RTCPPacket* pPkt);

    HX_RESULT MakeBye(RTCPPacket* pPkt);

    HX_RESULT MakeEOSApp(RTCPPacket* pPkt);

    inline UINT32 GetSSRC()
    {
        return m_SR_SSRC;
    }

private:
    void SetRTCP_SDES(BYTE* pRTCPData, UINT16 uLength);

    void SetRTCP_SR(BYTE* pSRReport, UINT16 uLength, Timeval timeval, BYTE* pRTCPDataEnd);

private:
   //Information stored from a RTCP Packet.
    UINT32		    m_SR_SSRC;
    UINT32		    m_SR_NTP_secs;
    UINT32		    m_SR_NTP_fraction_secs;
    UINT32		    m_SR_RTP_Timestamp;
    UINT32		    m_RTP_Timestamp;
    UINT32		    m_RTP_SeqNo;
    UINT32		    m_PacketCount;
    UINT32		    m_OctetCount;
    UINT32		    m_SDES_SSRC;
    CHAR*		    m_pcCNAME;
    HXBOOL		    m_bByeSent;
    Timeval		    m_LastRTCPUpdateSchedulerTime;
    CHXTimestampConverter* m_pTSConverter;
};

#endif // _RTCPUTIL_H_
