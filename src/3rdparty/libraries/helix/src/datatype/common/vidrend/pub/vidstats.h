/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vidstats.h,v 1.11 2007/07/06 22:00:25 jfinnecy Exp $
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

#ifndef __VIDSTATS_H__
#define __VIDSTATS_H__

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxbuffer.h"
#include "hxmon.h"
#include "statinfo.h"
#include "rendstats.h"

typedef enum
{
    VS_REND_NAME = 0,
    VS_CODEC_4CC,
    VS_CODEC_NAME,
    VS_CODEC_VERSION,
    VS_CODEC_FRAMERATE,
    VS_CURRENT_FRAMERATE,
    VS_FRAMES_DISPLAYED,
    VS_FRAMES_DROPPED,
    VS_FRAMES_DECODED,
    VS_FRAMES_UPSAMPLED,
    VS_FAILED_BLTS,
    VS_FRAMES_LOST,
    VS_SURESTREAM,
    VS_POSTFILTER,
    VS_HARDWARE_POSTFILTER,
    VS_SURFACE_MODE,
    VS_DECODER_MODE,
    VS_CODECS,
    VS_CODECS_FRAMERATES,
    VS_IMAGE_WIDTH,
    VS_IMAGE_HEIGHT,
    VS_CODEC_FRAME_WIDTH,
    VS_CODEC_FRAME_HEIGHT,
    VS_NUM_ENTRIES
} VideoStatEntryID;

class CVideoStatistics
{
public:
    /*
     *  Costructor/Destructor
     */
    CVideoStatistics(IUnknown* pContext, ULONG32 ulNumIntervals = 2);
    
    ~CVideoStatistics();
    
    /*
     *  Main Interface
     */
    HX_RESULT DisplayStats(UINT32 ulRegistryID);
    HX_RESULT SyncStats(ULONG32 ulTime);

    void ReportLostFrame(ULONG32 ulCount = 1)  
    { 
	m_Master.m_ulLostFrameCount += ulCount; 
    }
    
    void ReportDroppedFrame(ULONG32 ulCount = 1)  
    { 
	m_Master.m_ulDroppedFrameCount += ulCount;
    }

    void ReportDecodedFrame(ULONG32 ulCount = 1)  
    { 
	m_Master.m_ulDecodedFrameCount += ulCount;
    }

    void ReportSampledFrame(ULONG32 ulCount = 1)  
    { 
	m_Master.m_ulSampledFrameCount += ulCount;
    }

    void ReportFailedBlt(ULONG32 ulCount = 1)  
    { 
	m_Master.m_ulFailedBltCount += ulCount;
    }
    
    void ReportBlt(ULONG32 ulTime)  
    { 
	m_Master.m_ulBlitedFrameCount++;
	m_Master.m_ulLastFrameTimeStamp = ulTime;
    }
    
    HX_RESULT ReportStat(VideoStatEntryID eEntryID, const char* pVal)
    {
	return m_pDisplay->UpdateEntry((UINT32) eEntryID, pVal);
    }
    
    HX_RESULT ReportStat(VideoStatEntryID eEntryID, INT32 lVal)
    {
	return m_pDisplay->UpdateEntry((UINT32) eEntryID, lVal);
    }

    ULONG32 GetLastSyncTime(void)
    {
	return m_ulLastSyncTime;
    }

    void ResetSequence(void)
    {
	m_bStatsComputed = FALSE;
	m_ulStartIntervalIndex = 0;
	m_ulEndIntervalIndex = 0;
    }

private:
    class VStatCounters
    {
    public:
	VStatCounters(void)
	: m_ulLostFrameCount(0)
	, m_ulDroppedFrameCount(0)
	, m_ulDecodedFrameCount(0)
	, m_ulSampledFrameCount(0)
	, m_ulFailedBltCount(0)
	, m_ulBlitedFrameCount(0)
	, m_ulLastFrameTimeStamp(0)
	{
	    ;
	}

	~VStatCounters()
	{
	    ;
	}

	ULONG32 m_ulLostFrameCount;
	ULONG32 m_ulDroppedFrameCount;
	ULONG32 m_ulDecodedFrameCount;
	ULONG32 m_ulSampledFrameCount;
	ULONG32 m_ulFailedBltCount;
	ULONG32 m_ulBlitedFrameCount;
	ULONG32 m_ulLastFrameTimeStamp;
    };

    HX_RESULT PrimeEntries(void);

    void ComputeStatistics(void);

    void AdvanceIndex(void)
    {
	m_ulEndIntervalIndex = (m_ulEndIntervalIndex + 1) % m_ulNumIntervalPoints;
	if (m_ulEndIntervalIndex == m_ulStartIntervalIndex)
	{
	    m_ulStartIntervalIndex = (m_ulStartIntervalIndex + 1) % m_ulNumIntervalPoints;
	}
    }

    IHXRegistry* m_pRegistry;
    ULONG32 m_ulRegistryID;
    CRendererStatisticsDisplay* m_pDisplay;

    ULONG32 m_ulNumIntervalPoints;

    ULONG32 m_ulLastSyncTime;
    
    VStatCounters m_Master;

    VStatCounters* m_pPast;
    ULONG32 m_ulStartIntervalIndex;
    ULONG32 m_ulEndIntervalIndex;
    HXBOOL m_bStatsComputed;

    double m_fPercentFramesDisplayed;	// % of arrived frames displayed
    double m_fPercentFramesUpsampled;	// % of upsampled frames
    double m_fFrameRate;		// number of frames per second
};

#endif /* __VIDSTATS_H__ */

