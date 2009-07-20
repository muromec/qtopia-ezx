/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: buffmgr.h,v 1.19 2007/07/06 21:58:11 jfinnecy Exp $
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

#ifndef _BUFFMGR_H_
#define _BUFFMGR_H_

class HXSource;

typedef enum _buffer_state
{
    BUFFMGR_READY = 0,
    BUFFMGR_SEEK, 
    BUFFMGR_REBUFFER
} BUFFER_STATE;

class CBufferManager
{
private:

    UINT32		m_ulMaxAdditionalBufferingInMs;
    UINT32		m_ulAdditionalBufferingInMs;
    INT64		m_llHighestTimeStamp;
    INT64               m_llLowestTimeStamp;
    UINT32		m_ulTotalPauseTime;
    UINT32		m_ulLastPauseTick;
    UINT32		m_ulBufferingStartTick; // wallclock time when 1st packet received
    UINT32		m_ulBufferingStartTimeTS; // timestamp of 1st packet received
    UINT32		m_ulMinimumSourcePreroll;
    UINT32              m_ulWallclockProofingDelay;

    BUFFER_STATE	m_state;
    HX_BITFIELD		m_bLocalPlayback : 1;
    HX_BITFIELD		m_bPerfectPlay : 1;
    HX_BITFIELD		m_bPaused : 1;
    HX_BITFIELD		m_bBufferStartTimeToBeSet : 1;
    HX_BITFIELD		m_bFirstResumeDone : 1;
    HX_BITFIELD		m_bIsSeekPerformed : 1;
    
    HX_BITFIELD		m_bBufferedPlay : 1;
    HX_BITFIELD		m_bBufferCalcToBeChanged : 1;
    HX_BITFIELD		m_bIsInitialized : 1;
    UINT32		m_ulSeekTime;
    HXSource*		m_pParent;
    CHXMapLongToObj*	m_pStreamInfoTable;
    INT32               m_lPlaybackVelocity;
    HXBOOL              m_bKeyFrameMode;

protected:
    HX_RESULT	Reset(UINT32 ulSeekTime = 0, HXBOOL bSeekInsideRecordBuffer = FALSE);

    UINT32      GetElapsedTime(UINT32 ulCurrentTime);

    void        UpdateHighestTimestamps(INT64 llActualTimeStamp,
					STREAM_INFO* pStreamInfo);
    void        UpdateLowestTimestamps(INT64 llActualTimeStamp,
                                       STREAM_INFO* pStreamInfo);

    void        UpdateMinimumPreroll(HXBOOL bModifyStartTime);
    void        SetPerfectPlay(HXBOOL bPerfectPlay);
    void        ReadPrefs();
    INT32       ComputeWallclockDelay(UINT32 ulFirstPacketTimeStamp, UINT32 ulFirstPacketTick);
public:

    CBufferManager(HXSource* pParent);
    ~CBufferManager();

    // initialization
    HX_RESULT	Init(void);

    // set minimum preroll
    HX_RESULT	SetMinimumPreroll(HXBOOL bPerfectPlay, UINT32 ulSourcePreroll,
				  HXBOOL	 bModifyStartTime = TRUE);

    // stop the buffering(i.e. reach the end of source)
    HX_RESULT	Stop(void);

    // notify the seek
    HX_RESULT	DoSeek(UINT32 ulSeekTime, HXBOOL bSeekInsideRecordBuffer = FALSE);

    // pause
    HX_RESULT	DoPause(void);

    // resume
    HX_RESULT	DoResume(void);

    // rebuffer
    HX_RESULT	ReBuffer(UINT32 uPrerollIncrement);

    // update counters
    HX_RESULT	UpdateCounters(IHXPacket* pPacket, INT32 lPacketTimeOffSet);

    // get status
    HX_RESULT	GetStatus(REF(UINT16)	    uStatusCode,
			  REF(IHXBuffer*)  pStatusDesc,
			  REF(UINT16)	    uPercentDone);

    // get remain to buffer
    HX_RESULT	GetRemainToBuffer(REF(UINT32)	ulRemainToBufferInMs,
				  REF(UINT32)	ulRemainToBuffer);

    /* Get excess buffer info from Transport layer */
    HX_RESULT	GetExcessBufferInfo(REF(UINT32)	ulRemainToBufferInMs,
				    REF(UINT32)	ulRemainToBuffer,
				    REF(UINT32)	ulExcessBufferInMs,
				    REF(UINT32)	ulExcessBuffer,
				    REF(HXBOOL)	bValidInfo,
				    REF(UINT32)	ulActualExcessBufferInMs,
				    REF(UINT32) ulActualExcessBuffer);

    // get maximum preroll
    HX_RESULT	GetMaximumPreroll(REF(UINT32)	ulMaximumPrerollInMs);

    void	EnterBufferedPlay(void);
    void	LeaveBufferedPlay(void);

    void        OnResumed(void);

    void        SetVelocity(INT32 lVel);
    void        SetKeyFrameMode(HXBOOL bMode);
    
    void        SetMaxAdditionalBuffering(UINT32 ulMaxInMs);
    void        UpdateElapsedTimeAllStreams();
};

#endif /* _BUFFERMGR_H_ */
