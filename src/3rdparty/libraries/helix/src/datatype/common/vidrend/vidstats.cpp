/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vidstats.cpp,v 1.11 2007/07/06 22:00:25 jfinnecy Exp $
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
#define MAX_FLOAT_STRING_LENGTH	    35

/****************************************************************************
 *  Includes
 */
#include <stdio.h>
#include "rendstats.h"
#include "vidstats.h"
#include "hxstrutl.h"


/****************************************************************************
 *  CVideoStatistics
 */	
/****************************************************************************
 *  Costructor/Destructor
 */
CVideoStatistics::CVideoStatistics(IUnknown* pContext,
				   ULONG32 ulNumIntervals)
    : m_pRegistry(NULL)
    , m_pDisplay(NULL)
    , m_ulLastSyncTime(0)
    , m_ulNumIntervalPoints(ulNumIntervals + 1)
    , m_pPast(NULL)
    , m_ulStartIntervalIndex(0)
    , m_ulEndIntervalIndex(0)
    , m_bStatsComputed(FALSE)
    , m_fPercentFramesDisplayed(0.0)
    , m_fPercentFramesUpsampled(0.0)
    , m_fFrameRate(0.0)
{
    pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);
    HX_ASSERT(m_ulNumIntervalPoints > 0);

    if (m_ulNumIntervalPoints > 0)
    {
	m_pPast = new VStatCounters [m_ulNumIntervalPoints];

	HX_ASSERT(m_pPast);
    }

    m_pDisplay = new CRendererStatisticsDisplay(pContext, 
						(UINT32) VS_NUM_ENTRIES);

    HX_ASSERT(m_pDisplay);

    PrimeEntries();
}


CVideoStatistics::~CVideoStatistics()
{
    HX_DELETE(m_pDisplay);
    HX_VECTOR_DELETE(m_pPast);
    HX_RELEASE(m_pRegistry);
}


/****************************************************************************
 *  PrimeEntries
 */
HX_RESULT CVideoStatistics::PrimeEntries(void)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pDisplay)
    {
	m_pDisplay->PrimeEntry((UINT32) VS_REND_NAME, "Name", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_CODEC_NAME, "CodecName", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_CODEC_4CC, "CodecFourCC", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_CODEC_VERSION, "CodecVersion", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_CODEC_FRAMERATE, "CodecRate", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_CURRENT_FRAMERATE, "CurrentFrameRate", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_FRAMES_DISPLAYED, "FramesDisplayed", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_FRAMES_DROPPED, "FramesDropped", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_FRAMES_DECODED, "FramesDecoded", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_FRAMES_UPSAMPLED, "FramesUpsampled", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_FAILED_BLTS, "FailedBlts", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_FRAMES_LOST, "FramesLost", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_SURESTREAM, "SureStream", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_POSTFILTER, "PostFilter", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_HARDWARE_POSTFILTER, "HWPostFilter", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_SURFACE_MODE, "VideoSurfaceMode", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_DECODER_MODE, "DecoderMode", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_CODECS, "CodecsSuite", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_CODECS_FRAMERATES, "CodecsFrameRates", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) VS_IMAGE_WIDTH, "ImageWidth", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_IMAGE_HEIGHT, "ImageHeight", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_CODEC_FRAME_WIDTH, "CodecFrameWidth", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) VS_CODEC_FRAME_HEIGHT, "CodecFrameHeight", REG_TYPE_NUMBER);

	retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  UpdateStatistics
 */
HX_RESULT CVideoStatistics::DisplayStats(UINT32 ulRegistryID)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pDisplay && (ulRegistryID != 0))
    {
	retVal = HXR_OK;
    }

    // Perform any outstanding updates
    if (SUCCEEDED(retVal))
    {
	if (m_bStatsComputed)
	{
	    char pValBuffer[MAX_FLOAT_STRING_LENGTH];

	    m_bStatsComputed = FALSE;

	    SafeSprintf(pValBuffer, MAX_FLOAT_STRING_LENGTH, "%.1f", m_fFrameRate);
	    ReportStat(VS_CURRENT_FRAMERATE, pValBuffer);

	    SafeSprintf(pValBuffer, MAX_FLOAT_STRING_LENGTH, "%.1f", m_fPercentFramesDisplayed);
	    ReportStat(VS_FRAMES_DISPLAYED, pValBuffer);

	    SafeSprintf(pValBuffer, MAX_FLOAT_STRING_LENGTH, "%.1f", m_fPercentFramesUpsampled);
	    ReportStat(VS_FRAMES_UPSAMPLED, pValBuffer);
	    
	}

	ReportStat(VS_FRAMES_DROPPED, m_Master.m_ulDroppedFrameCount);
	ReportStat(VS_FRAMES_DECODED, m_Master.m_ulDecodedFrameCount);
	ReportStat(VS_FAILED_BLTS, m_Master.m_ulFailedBltCount);
	ReportStat(VS_FRAMES_LOST, m_Master.m_ulLostFrameCount);
    }

    // Refresh Display
    if (SUCCEEDED(retVal))
    {
	m_pDisplay->RefreshEntries(ulRegistryID);
    }

    return HXR_OK;
}


/****************************************************************************
 *  SyncStats
 */
HX_RESULT CVideoStatistics::SyncStats(ULONG32 ulTime)
{
    ULONG32 ulLostFrameCount = m_Master.m_ulLostFrameCount;
    ULONG32 ulDroppedFrameCount = m_Master.m_ulDroppedFrameCount;
    ULONG32 ulDecodedFrameCount = m_Master.m_ulDecodedFrameCount;
    ULONG32 ulSampledFrameCount = m_Master.m_ulSampledFrameCount;
    ULONG32 ulFailedBltCount = m_Master.m_ulFailedBltCount;
    ULONG32 ulBlitedFrameCount = m_Master.m_ulBlitedFrameCount;
    ULONG32 ulLastFrameTimeStamp = m_Master.m_ulLastFrameTimeStamp;

    VStatCounters* pNewStats = &(m_pPast[m_ulEndIntervalIndex]);

    pNewStats->m_ulLostFrameCount = ulLostFrameCount;
    pNewStats->m_ulDroppedFrameCount = ulDroppedFrameCount;
    pNewStats->m_ulDecodedFrameCount = ulDecodedFrameCount;
    pNewStats->m_ulSampledFrameCount = ulSampledFrameCount;
    pNewStats->m_ulFailedBltCount = ulFailedBltCount;
    pNewStats->m_ulBlitedFrameCount = ulBlitedFrameCount;
    pNewStats->m_ulLastFrameTimeStamp = ulLastFrameTimeStamp;

    m_ulLastSyncTime = ulTime;

    ComputeStatistics();

    AdvanceIndex();

    return HXR_OK;
}

/****************************************************************************
 *  UpdateStatistics
 */
void CVideoStatistics::ComputeStatistics(void)
{
    if (m_ulStartIntervalIndex != m_ulEndIntervalIndex)
    {
	ULONG32 ulIntervalLostFrameCount = 
	    m_pPast[m_ulEndIntervalIndex].m_ulLostFrameCount -
	    m_pPast[m_ulStartIntervalIndex].m_ulLostFrameCount;
	ULONG32 ulIntervalDroppedFrameCount = 
	    m_pPast[m_ulEndIntervalIndex].m_ulDroppedFrameCount -
	    m_pPast[m_ulStartIntervalIndex].m_ulDroppedFrameCount;
	ULONG32 ulIntervalDecodedFrameCount = 
	    m_pPast[m_ulEndIntervalIndex].m_ulDecodedFrameCount -
	    m_pPast[m_ulStartIntervalIndex].m_ulDecodedFrameCount;
	ULONG32 ulIntervalSampledFrameCount = 
	    m_pPast[m_ulEndIntervalIndex].m_ulSampledFrameCount -
	    m_pPast[m_ulStartIntervalIndex].m_ulSampledFrameCount;
	ULONG32 ulIntervalBlitedFrameCount = 
	    m_pPast[m_ulEndIntervalIndex].m_ulBlitedFrameCount -
	    m_pPast[m_ulStartIntervalIndex].m_ulBlitedFrameCount;
	ULONG32 ulIntervalTime = 
	    m_pPast[m_ulEndIntervalIndex].m_ulLastFrameTimeStamp -
	    m_pPast[m_ulStartIntervalIndex].m_ulLastFrameTimeStamp;

	ULONG32 ulIntervalTotalFrames = ulIntervalLostFrameCount +
					ulIntervalDroppedFrameCount +
					ulIntervalBlitedFrameCount;

	if (ulIntervalTime != 0)
	{
	    m_fFrameRate = ((double) ulIntervalBlitedFrameCount) *
			   1000.0 /
			   ((double) ulIntervalTime);
	}

	// Compute percent of frames displayed this interval
	if (ulIntervalTotalFrames != 0)
	{
	    m_fPercentFramesDisplayed = ((double) ulIntervalBlitedFrameCount) *
					100.0 /
					((double) ulIntervalTotalFrames);

	    
	}
	else
	{
	    m_fPercentFramesDisplayed = 100.0;
	}

	// Compute percent of upsampled frames decoded this interval
	if ((ulIntervalDecodedFrameCount != 0) && 
	    (m_Master.m_ulSampledFrameCount != 0) &&
	    (ulIntervalDecodedFrameCount > ulIntervalSampledFrameCount))
	{
	    // We compute % of upsampled frames only if there is evidence that
	    // sampled frames are beeing indentified and frames have been 
	    // decoded this interval and we have decoded more than sampled frames.
	    ULONG32 ulIntervalUpsampledFrameCount = ulIntervalDecodedFrameCount - 
						    ulIntervalSampledFrameCount;

	    m_fPercentFramesUpsampled = ((double) ulIntervalUpsampledFrameCount) *
					100.0 /
					((double) ulIntervalDecodedFrameCount);
	}
	else
	{
	    m_fPercentFramesUpsampled = 0.0;
	}

	m_bStatsComputed = TRUE;
    }
}
