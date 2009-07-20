/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtpclsnc.cpp,v 1.10 2007/07/06 20:51:41 jfinnecy Exp $
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

/****************************************************************************
 *  Defines
 */

/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "rtpclsnc.h"

#include "ihxpckts.h"
#include "rtspif.h"
#include "ntptime.h"



/****************************************************************************
 *  CRTPClientStreamSync             
 */
/****************************************************************************
 *  Constructor/Destructor          
 */
CRTPClientStreamSync::CRTPClientStreamSync(void)
    : m_pTSConverter(NULL)
    , m_bIsImportedTSConverter(FALSE)
    , m_bIsSyncMaster(FALSE)
    , m_bStartSyncSet(FALSE)
    , m_bIsStronglySynced(FALSE)
    , m_bNTPtoHXOffsetSet(FALSE)
    , m_lNTPtoHXOffset(0)
    , m_lTimeOffsetRTP(0)
    , m_lTimeOffsetHX(0)
    , m_lSyncOffsetRTP(0)
    , m_lSyncOffsetHX(0)
    , m_lOffsetToMasterRTP(0)
    , m_lOffsetToMasterHX(0)
    , m_bStartTimeSet(FALSE)
    , m_ulStartTime(0)
    , m_ulAcceptableSyncNoise(RTPCL_ACCEPTABLE_SYNC_NOISE)
    , m_pSyncServer(NULL)
{
    ;
}

CRTPClientStreamSync::~CRTPClientStreamSync(void)
{
    Close();
}


/****************************************************************************
 *  Main Interface               
 */
/****************************************************************************
 *  CRTPClientStreamSync::Init              
 */
HX_RESULT CRTPClientStreamSync::Init(ULONG32 ulHXAFactor,
				     ULONG32 ulRTPFactor,
				     IHXTransportSyncServer* pSyncServer,
				     HXBOOL bIsSyncMaster,
				     ULONG32 ulAcceptableSyncNoise)
{
    HX_RESULT retVal = HXR_OK;

    ReleaseTSConverter();
    HX_RELEASE(m_pSyncServer);
    Reset();

    m_ulAcceptableSyncNoise = ulAcceptableSyncNoise;
    m_bIsSyncMaster = bIsSyncMaster;

    m_pSyncServer = pSyncServer;
    if (m_pSyncServer)
    {
	m_pSyncServer->AddRef();
    }

    retVal = HXR_OUTOFMEMORY;
    m_pTSConverter = new CHXTimestampConverter(CHXTimestampConverter::FACTORS,
					       ulHXAFactor, 
					       ulRTPFactor);
    if (m_pTSConverter)
    {
	retVal = HXR_OK;
    }

    return retVal;
}
    
HX_RESULT CRTPClientStreamSync::Init(CHXTimestampConverter* pTSConverter,
				     IHXTransportSyncServer* pSyncServer,
				     HXBOOL bIsSyncMaster,
				     ULONG32 ulAcceptableSyncNoise)
{
    HX_RESULT retVal = HXR_OK;

    ReleaseTSConverter();
    HX_RELEASE(m_pSyncServer);
    Reset();

    m_ulAcceptableSyncNoise = ulAcceptableSyncNoise;
    m_bIsSyncMaster = bIsSyncMaster;

    m_pSyncServer = pSyncServer;
    if (m_pSyncServer)
    {
	m_pSyncServer->AddRef();
    }

    m_pTSConverter = pTSConverter;
    if (m_pTSConverter)
    {
	m_bIsImportedTSConverter = TRUE;
    }

    return retVal;
}


/****************************************************************************
 *  CRTPClientStreamSync::Close               
 */
void CRTPClientStreamSync::Close(void)
{
    ReleaseTSConverter();
    HX_RELEASE(m_pSyncServer);
}


/****************************************************************************
 *  CRTPClientStreamSync::Reset               
 */
void CRTPClientStreamSync::Reset(void)
{
    m_bStartSyncSet = FALSE;
    m_bIsStronglySynced = FALSE;
    m_bNTPtoHXOffsetSet = FALSE;
    m_lNTPtoHXOffset = 0;
    m_lTimeOffsetRTP = 0;
    m_lTimeOffsetHX = 0;
    m_lSyncOffsetHX = 0;
    m_lSyncOffsetRTP = 0;
    m_lOffsetToMasterHX = 0;
    m_lOffsetToMasterRTP = 0;
    m_bStartTimeSet = FALSE;
    m_ulStartTime = 0;
    if (m_pTSConverter)
    {
	m_pTSConverter->ReInit(m_pTSConverter->GetConversionFactors());
    }
}


/****************************************************************************
 *  CRTPClientStreamSync::SetStartTime 
 *
 *  Establishes the reference starting time (in ms) of the session to be used
 *  for computation of individual initial stream syncronization offsets in
 *  case the stream RTP-NPT time is not explictly provided (e.g. via rtp-info
 *  in PLAY response).
 *  This function can be called multiple times by various streams in the
 *  the session.  However, only the first setting will take effect and will
 *  be reflected to all other streams.
 *	ulRefHXStartTime     - reference starting time (in ms) of a session
 *			        (normally this is the reference arrival time
 *				 of the first packet of the stream)
 *	bAsSecondaryRecepient - TRUE =  called as reaction of some other stream
 *				        establishing the sessions reference
 *				        start time and thus reflection to other 
 *				        streams in the session should not be
 *				        attempted
 *				FALSE = stream initiated establishing of the 
 *					sessions reference start time and thus
 *					the reflection of the start time to
 *					all streams in the session is to be
 *					attempted	       
 */
HX_RESULT CRTPClientStreamSync::SetStartTime(ULONG32 ulRefHXStartTime,
					     HXBOOL bAsSecondaryRecipient)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (!m_bStartSyncSet)
    {
	retVal = HXR_IGNORE;
	if (!m_bStartTimeSet)
	{
	    retVal = HXR_OK;
	    if (bAsSecondaryRecipient)
	    {
		m_bStartTimeSet = TRUE;
		m_ulStartTime = ulRefHXStartTime;
	    }
	    else
	    {
		if (m_pSyncServer)
		{
		    retVal = m_pSyncServer->DistributeStartTime(
						ulRefHXStartTime);
		}
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  CRTPClientStreamSync::SetStartSync    
 *
 *  Establishes intial synchronization of the stream relative to other
 *  streams.  This method must be called to enable the syncronization
 *  functionality of this class.
 *	ulRTPTime         - stream RTP time in vicinity of the first RTP time
 *		            stamp received for the stream, corresponding to
 *			    the passed in RMA time (in RTP units)
 *	ulHXTime         - stream NPT time corresponding to the passed in
 *			    RTP time (in ms)
 *      bIsNominalHXTime - FALSE = binding between RTP and NPT is exact
 *				    (This normally means that RTP-NPT mapping
 *				     was explictly provided through out of
 *				     band means such as rtp-info in PLAY
 *				     response)
 *			    TRUE  = binding between RTP and NPT is loose
 *				    (This normally means that RTP-NPT mapping
 *				     was NOT explictly provided through out of
 *				     band means such as rtp-info in PLAY
 *				     response. In such case offset between
 *				     streams will be established using the
 *				     difference in arrival time of the first 
 *				     stream packets - if supplied to this
 *				     class.)
 *	ulRefHXStartTime - reference time value in ms representing the arrival
 *			    time of the packet corresponding to the passed in
 *			    RTP time.  This value must be properly set if
 *			    bIsNominalHXTime == TRUE and SetStartTime() method
 *			    was invoked on any of the streams in the session.
 *			    This value is used to compute the initial
 *			    synchronization offset of the stream based on the
 *			    packet arrival time when RTP-NPT mapping is not
 *			    explicitly communicated 
 *			    (bIsNominalHXTime == TRUE).
 */
HX_RESULT CRTPClientStreamSync::SetStartSync(ULONG32 ulRTPTime,
					     ULONG32 ulHXTime,
					     HXBOOL bIsNominalHXTime,
					     ULONG32 ulRefHXStartTime)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (!m_bIsStronglySynced)
    {
	LONG32 lHXTimeAdjustment = 0;

	retVal = HXR_OK;

	m_bStartSyncSet = TRUE;
	m_bIsStronglySynced = (!bIsNominalHXTime);

	if (bIsNominalHXTime && m_bStartTimeSet)
	{
	    lHXTimeAdjustment = ((LONG32) 
		(ulRefHXStartTime - m_ulStartTime));
	}

	if (m_pTSConverter)
	{
	    m_lTimeOffsetRTP = (LONG32)(ulRTPTime - 
			       m_pTSConverter->hxa2rtp_raw(ulHXTime + lHXTimeAdjustment));
	    m_pTSConverter->setAnchor(ulHXTime + lHXTimeAdjustment, 
				      ulRTPTime);
	    m_lTimeOffsetHX = 0;
	}
	else
	{
	    m_lTimeOffsetHX = m_lTimeOffsetRTP = 
		(LONG32)((ulRTPTime - ulHXTime - lHXTimeAdjustment));		
	}
    }

    return retVal;
}


/****************************************************************************
 *  CRTPClientStreamSync::HandleMasterSync     
 *
 *  Incorporates the synchronization adjustment as requested by the master
 *  syncronization stream.  Master synchronization stream (usally audio stream)
 *  does not adjust its time-line but rather reflects its adjustment to all
 *  other streams.  This is normally done to avoid gaps or clipping in audio.
 *	ulHXTime   - NPT (Normal Play time) of the master sync. stream when
 *		      the synchronization adjustemnt of slave streams was
 *		      requested (in ms)
 *	lHXOffsetToMaster - time offset from master stream to apply (in ms)
 */
HX_RESULT CRTPClientStreamSync::HandleMasterSync(ULONG32 ulHXTime, 
						 LONG32 lHXOffsetToMaster)
{
    HX_RESULT retVal = HXR_IGNORE;

    if (!m_bIsSyncMaster)
    {
	retVal = HXR_OK;
	
	m_lOffsetToMasterHX = lHXOffsetToMaster;
	if (lHXOffsetToMaster >= 0)
	{
	    m_lOffsetToMasterRTP = (LONG32) 
		(m_pTSConverter->hxa2rtp_raw((ULONG32) lHXOffsetToMaster));
	}
	else
	{
	    m_lOffsetToMasterRTP = (LONG32) 
		(-m_pTSConverter->hxa2rtp_raw((ULONG32) (-lHXOffsetToMaster)));
	}
    }

    return retVal;
}


/****************************************************************************
 *  CRTPClientStreamSync::AnchorSync
 *
 *  Establishes the mapping between NPT (Normal play time) and NTP time.
 *  This relationship is needed before RTCP reported (NTP,RTP) time stamp
 *  pairs can be applied to synchronization.  This mapping is common for
 *  all streams in the session and is normally established by reception
 *  of the first RTCP packet after the starting stream synchronization is
 *  known (or is estimated).
 *	ulHXTime    - NPT time corresponding to passed in NTP time
 *	ulNTPHXTime - NTP time in ms corresponding to passed in NPT time
 */
HX_RESULT CRTPClientStreamSync::AnchorSync(ULONG32 ulHXTime, 
					   ULONG32 ulNTPHXTime)
{
    HX_RESULT retVal = HXR_OK;

    m_lNTPtoHXOffset = (LONG32)(ulHXTime - ulNTPHXTime);
    m_bNTPtoHXOffsetSet = TRUE;

    return retVal;
}


/****************************************************************************
 *  CRTPClientStreamSync::HandleRTCPSync    
 *
 *  Incorporates the NTP and RTP time stamp relationships reported by RTCP
 *  into synchronization equation.
 *	ntpTime   - RTCP reported NTP time
 *	ulRTPTime - RTCP reported RTP time
 */
HX_RESULT CRTPClientStreamSync::HandleRTCPSync(NTPTime ntpTime, 
					       ULONG32 ulRTPTime)
{
    HX_RESULT retVal = HXR_IGNORE;

    ULONG32 ulNtpHX = ntpTime.toMSec();
    
    // We ignore the RTCP sync until we can compute npt (m_bStartSyncSet) or
    // if the RTCP packet contains no synchronization information 
    // (ulNtpHX == 0)
    if ((ulNtpHX != 0) && m_bStartSyncSet)
    {
	// Npt time can be computed (ulHXTime)
	ULONG32 ulHXTime = m_pTSConverter->rtp2hxa(ulRTPTime);
	
	retVal = HXR_OK;
		
	if (m_bNTPtoHXOffsetSet)
	{
	    // We can sync - NTP to NPT offset is known
	    ULONG32 ulExpectedHXTime = ulNtpHX + m_lNTPtoHXOffset;
	    LONG32 lSyncOffsetHX = (LONG32)(ulExpectedHXTime - ulHXTime);
	    LONG32 lSyncOffsetChange = lSyncOffsetHX - m_lSyncOffsetHX;
	    
	    if (((ULONG32)lSyncOffsetChange > m_ulAcceptableSyncNoise) ||
		(lSyncOffsetChange < (LONG32)(-m_ulAcceptableSyncNoise)))
	    {
		if (m_bIsSyncMaster && m_pSyncServer)
		{
		    m_pSyncServer->DistributeSync(ulHXTime, -lSyncOffsetHX);
		}
		else
		{		    
		    m_lSyncOffsetHX = lSyncOffsetHX;
		    if (lSyncOffsetHX >= 0)
		    {
			m_lSyncOffsetRTP = (LONG32) 
			    (m_pTSConverter->hxa2rtp_raw((ULONG32) lSyncOffsetHX));
		    }
		    else
		    {
			m_lSyncOffsetRTP = (LONG32) 
			    (-m_pTSConverter->hxa2rtp_raw((ULONG32) (-lSyncOffsetHX)));
		    }
		}
	    }

	    m_bIsStronglySynced = TRUE;
	}
	else
	{
	    // This the first RTCP sync accross all streams, anchor sync
	    if (m_pSyncServer)
	    {
		m_pSyncServer->DistributeSyncAnchor(ulHXTime, ulNtpHX);
	    }
	}
    }

    return retVal;
}

/****************************************************************************
 *  Private Methods             
 */
/****************************************************************************
 *  CRTPClientStreamSync::ReleaseTSConverter               
 */
void CRTPClientStreamSync::ReleaseTSConverter(void)
{
    if (m_bIsImportedTSConverter)
    {
	m_pTSConverter = NULL;
    }
    else
    {
	HX_DELETE(m_pTSConverter);
    }
    m_bIsImportedTSConverter = FALSE;
}
