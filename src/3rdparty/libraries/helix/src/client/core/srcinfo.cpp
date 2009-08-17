/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: srcinfo.cpp,v 1.83 2007/02/27 06:21:16 gbajaj Exp $
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
#include "hxresult.h"
#include "smiltype.h"
#include "hxcomm.h"		// IHXRegistryID
#include "hxengin.h"
#include "hxcore.h"
#include "hxupgrd.h"
#include "hxrendr.h"
#include "hxasm.h"
#include "hxsmbw.h"
#include "hxgroup.h"
#include "hxausvc.h"
#include "hxslist.h"
#include "hxmap.h"
#include "chxpckts.h"
#include "chxeven.h"
#include "chxelst.h"
#include "strminfo.h"
#include "hxmutex.h"
#include "timeval.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxstrm.h"
#include "hxsmstr.h"
#include "hxaudply.h"
#include "basgroup.h"
#include "advgroup.h"
#include "hxthread.h"
#include "hxtick.h"
#include "hxstrutl.h"
#include "hxtlogutil.h"
#include "pckunpck.h"

#include "srcinfo.h"
#include "hxplay.h"
#include "hxcleng.h"
#include "client_preroll_hlpr.h"

#ifdef HELIX_FEATURE_PARTIALPLAYBACK
// Headers
#include "hxprefutil.h"
#include "nullrend.h"

// Defines used for partial playback
#define DISABLE_PARTIALPLAYBACK "DisablePartialPlayback"
#define MIN_VALID_RENDERER_REQD "MinValidRendererReqd"
#define DEFAULT_MIN_VALID_RENDERER_REQD 1

#endif // End of #ifdef HELIX_FEATURE_PARTIALPLAYBACK

#if defined(HELIX_FEATURE_DRM)
#include "hxdrmcore.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define TIME_SYNC_FUDGE_FACTOR      10

SourceInfo::SourceInfo(HXPlayer* pPlayer)
{
    m_pRendererMap = new CHXMapLongToObj;
    
    m_pPlayer = pPlayer;
    m_pPlayer->AddRef();
    m_pSource		    = NULL;
    m_bDone		    = FALSE;
    m_bStopped		    = FALSE;
    m_bInitialized	    = FALSE;
    m_pStatus		    = NULL;
    m_bAllPacketsReceived   = FALSE;
    m_bActive		    = TRUE;
    m_bIsPersistentSource   = FALSE;
    m_bIsRegisterSourceDone = FALSE;
    m_uTrackID		    = 0;
    m_uGroupID		    = 0;
    m_bToBeResumed	    = TRUE;
    m_bAreStreamsSetup	    = FALSE;
    m_bTrackStartedToBeSent = TRUE;
    m_bTrackStoppedToBeSent = TRUE;
    m_bPrefetch		    = FALSE;
    m_bLoadPluginAttempted  = FALSE;
    m_pCurrentScheduleList  = NULL;
    m_ulSourceDuration	    = 0;
    m_ulMaxDuration	    = 0;
    m_ulTrackDuration	    = 0;
    m_ulTotalTrackDuration  = 0;
    m_pDependNode	    = NULL;
    m_uNumDependencies	    = 0;
    m_bTobeInitializedBeforeBegin = FALSE;
    m_bAltURL		    = FALSE;
    m_lastErrorFromMainURL  = HXR_OK;
    m_lastError             = HXR_OK;
    m_bLocked		    = FALSE;
    m_bIsTrackDurationSet   = FALSE;
    m_bDurationTimeSyncScheduled = FALSE;
    m_bAudioDeviceReflushHint = FALSE;
    m_pMutex = NULL;

    m_pProcessCallback = new CHXGenericCallback((void*)this, (fGenericCBFunc)ProcessCallback);
    m_pProcessCallback->AddRef();

    m_prefetchType = PrefetchUnknown;
    m_ulPrefetchValue = 0;
    m_uSoundLevel = 100;
    m_fillType = FillRemove;
    m_pAltURLList = NULL;
    
#ifdef HELIX_FEATURE_PARTIALPLAYBACK
    m_bIsPartialPlayback = FALSE;
#endif
    
    m_pPeerSourceInfo = NULL;   
    m_bSeekPending = FALSE;
    m_bIndefiniteDuration = FALSE;
    m_bRepeatIndefinite = FALSE;
    m_bSeekToLastFrame = FALSE;
    m_bNetSourceFailed = FALSE;
    m_bFileSourceFailed = FALSE;
    m_ulRepeatInterval = 0;
    m_ulRepeatDelayTimeOffset = 0;
    m_ulSeekTime = 0;
    m_ulPausedStartTime = 0;

    m_bLeadingSource = TRUE;
    m_bRepeatPending = FALSE;
    m_pRepeatList = NULL;
    m_curPosition = 0;
    
    m_ulPersistentComponentID = MAX_UINT32;
    m_ulPersistentComponentSelfID = MAX_UINT32;
    m_pRendererAdviseSink = NULL;

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, (IUnknown*)(IHXPlayer*)m_pPlayer);  

    /*
     * The following members are needed for live sync support.
     */
    m_pWallClock = NULL;
    m_ulStreamStartTime = 0;
    m_llLatestPacketTime   = 0;
    m_llEarliestPacketTime = MAX_UINT32;

    m_ulSeekOnLateBeginTolerance = MAX_ADD_TRACK_LATENCY_MSEC;
    m_bSeekOnLateBegin = FALSE;
}

SourceInfo::~SourceInfo()
{
    if (m_pProcessCallback && m_pPlayer->m_pScheduler)
    {
	m_pPlayer->m_pScheduler->Remove(m_pProcessCallback->GetPendingCallback());
        m_pProcessCallback->CallbackCanceled();
    }

    if (m_pRepeatList)
    {
	while (m_pRepeatList->GetCount())
	{
	    RepeatInfo* pRepeatInfo = (RepeatInfo*)m_pRepeatList->RemoveHead();
	    HX_DELETE(pRepeatInfo);
	}

	HX_DELETE(m_pRepeatList);
    }

    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pStatus);
    HX_RELEASE(m_pProcessCallback);
    HX_RELEASE(m_pPlayer);
    HX_DELETE(m_pCurrentScheduleList);
    HX_VECTOR_DELETE(m_pDependNode);
    HX_DELETE(m_pRendererMap);
    /*
     * For live sync support, we may have used a shared wall
     * clock, if so then we're done with it now, so we may need 
     * to do some cleanup work here.     
     */
    DoneWithWallClock();
    // Clear the AltURL list
    ClearAltURLList();
};



HX_RESULT
SourceInfo::Begin()
{
    HX_RESULT theErr = HXR_OK;

    if(!m_pSource || !m_pSource->IsInitialized())
    {
	return HXR_OK;
    }

    if (m_pPlayer->m_bInitialized)
    {
	// handle the seek so that the server will be notified 
	// via the play request upon the first resume
	if (m_bSeekPending)
	{
	    m_bSeekPending = FALSE;
	
	    Pause();
	    Seek(m_ulSeekTime);
	    m_pSource->DoSeek(m_ulSeekTime);
	}

	theErr = m_pSource->DoResume();
    }

    if (!m_bInitialized)
    {
	return HXR_OK;
    }

    /* Only send this OnBegin()'s if not the first begin. In the case
     * of the first begin, these are actually sent after the header's
     * arrive based on the indicated start delay timing.
     */
    if (SUCCEEDED(theErr) &&
	!m_pPlayer->m_bIsFirstBegin && 
	!m_pPlayer->m_bInternalPauseResume)
    {
	CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
	for (; !theErr &&
		ndxRend != m_pRendererMap->End(); ++ndxRend)
	{
	    RendererInfo* pRendInfo = (RendererInfo*) (*ndxRend);

	    if (!pRendInfo->m_bInitialBeginToBeSent)
	    {
		UINT32 ulOnBeginTime = m_pPlayer->m_ulCurrentPlayTime;

		IHXRenderer* pRend = (IHXRenderer*) pRendInfo->m_pRenderer;
                m_pPlayer->CheckForPacketTimeOffsetUpdate(pRendInfo);
		theErr = m_pPlayer->CalculateOnBeginTime(pRendInfo, ulOnBeginTime);
		HX_ASSERT(theErr == HXR_OK);
		if (pRend)
		{
		    pRend->OnBegin(ulOnBeginTime);
		}
	    }
	}
    }

    return theErr;
}


HX_RESULT
SourceInfo::Pause()
{
    HX_RESULT	theErr	= HXR_OK;

    if(!m_pSource)
    {
	return HXR_OK;
    }

    theErr = m_pSource->DoPause();

    /* Do not send OnPause to renderers if it is an internal Pause */
    if (m_pPlayer->m_bInternalPauseResume || !m_bInitialized)
    {
	return theErr;
    }
#if defined(HELIX_FEATURE_DRM)
    //flush the DRM pending packets
    if (m_pSource && m_pSource->IsHelixDRMProtected() && m_pSource->GetDRM())
    {
        m_pSource->GetDRM()->FlushPackets(TRUE);
    }
#endif /* HELIX_FEATURE_DRM */

    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (; !theErr &&
	    ndxRend != m_pRendererMap->End(); ++ndxRend)
    {
	RendererInfo* pRendInfo = (RendererInfo*) (*ndxRend);
	IHXRenderer* pRend     = (IHXRenderer*) pRendInfo->m_pRenderer;

	m_pPlayer->m_pScheduler->Remove(
         pRendInfo->m_pTimeSyncCallback->GetPendingCallback());
        
        pRendInfo->m_pTimeSyncCallback->CallbackCanceled();
	pRendInfo->m_bIsFirstCallback   = TRUE;

	if (pRend)
	{
	    pRend->OnPause(m_pPlayer->m_ulCurrentPlayTime);
	}
    }

    return theErr;
}


HX_RESULT
SourceInfo::Seek(UINT32 ulSeekTo)
{
    HX_RESULT			    rc = HXR_OK;
    INT64			    llLastExpectedPacketTime = 0;
    HXBOOL			    bSeekToLastFrame = FALSE;
    HXBOOL			    bDurationTimeSyncSent = TRUE;
    UINT32			    ulValue = 0;
    RendererInfo*		    pRendInfo = NULL;
    IHXValues*			    pStatus = NULL;
    IHXPersistentRenderer*	    pPersRender = NULL;
    IHXPersistentComponent*	    pPersComp = NULL;
    HXPersistentComponentManager*  pPersCompMgr = NULL;
    CHXMapLongToObj::Iterator ndx;

    /* m_pSource should never be NULL */
    HX_ASSERT(m_pSource);
    if(!m_pSource || !m_bInitialized)
    {
	return HXR_OK;
    }

    if(m_pSource->IsDelayed() && m_pSource->GetDelay() > ulSeekTo && 
       m_pSource->GetStartTime() == 0)
    {
        /* Seeking before start position of a delayed source. 
           The source will ignore this seek, so here we have to ignore it too.  
        */
        return HXR_OK;
    }

    m_bDone = FALSE;
    m_bActive = TRUE;
    m_bAllPacketsReceived = FALSE;
    llLastExpectedPacketTime = m_pSource->GetLastExpectedPacketTime();

    /* Are we seeking past the last expected packet time?
     * If so, verify the "show" attribute to see whether we need to
     * get the last video frame
     */    
    if (!m_pSource->IsLive() && ulSeekTo >= INT64_TO_UINT32(llLastExpectedPacketTime + m_pSource->GetDelay()))
    {
#if defined(HELIX_FEATURE_NESTEDMETA)
	pPersCompMgr = m_pPlayer->m_pPersistentComponentManager;
	if (FillFreeze == m_fillType && pPersCompMgr) 
	{
	    rc = pPersCompMgr->GetPersistentComponent(m_ulPersistentComponentID,
						      pPersComp);
	    HX_ASSERT(HXR_OK == rc);

	    if (pPersComp)
	    {
		rc = pPersComp->GetPersistentRenderer(pPersRender);
		HX_ASSERT(HXR_OK == rc);

		rc = pPersRender->GetElementStatus(m_uGroupID,
						   m_uTrackID,
						   ulSeekTo,
						   pStatus);
		HX_ASSERT(HXR_OK == rc);

		if (pStatus && HXR_OK == pStatus->GetPropertyULONG32("Show", ulValue))
		{
		    bSeekToLastFrame = (HXBOOL)ulValue;
		}

		HX_RELEASE(pStatus);
		HX_RELEASE(pPersRender);
		HX_RELEASE(pPersComp);
	    }
	}
	else if (FillHold == m_fillType)
	{
	    bSeekToLastFrame = TRUE;
	}
#else
	if (FillHold == m_fillType)
	{
	    bSeekToLastFrame = TRUE;
	}
#endif /* HELIX_FEATURE_NESTEDMETA */
    }

    // also check if we already sent OnTimeSync() upon its duration ends
    // which means the renderer got the last frame so we don't need to issue
    // last frame seek
    if (bSeekToLastFrame)
    {
	ndx = m_pRendererMap->Begin();
	for (; ndx != m_pRendererMap->End(); ++ndx)
	{
	    pRendInfo = (RendererInfo*)(*ndx);
	    if (!pRendInfo->m_bDurationTimeSyncSent)
	    {
		bDurationTimeSyncSent = FALSE;
	    }
	}

	if (bDurationTimeSyncSent)
	{
	    bSeekToLastFrame = FALSE;
	}
    }

    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (; ndxRend != m_pRendererMap->End(); ++ndxRend)
    {
	RendererInfo* pRendInfo     = (RendererInfo*)(*ndxRend);
	IHXRenderer* pRend         = (IHXRenderer*)pRendInfo->m_pRenderer;
	STREAM_INFO* pStreamInfo   = pRendInfo->m_pStreamInfo;
	
	pStreamInfo->m_bSrcInfoStreamDone  = FALSE;

	pRendInfo->m_pStream->ResetASMRuleState();
	pStreamInfo->ResetPostEndTimeEventList();

	if (m_pSource->IsLive())
	{
#if defined(HELIX_FEATURE_RECORDCONTROL)
	    if(m_pSource->IsPlayingFromRecordControl())
	    {
		pStreamInfo->m_ulTimeBeforeSeek = m_pPlayer->m_ulTimeBeforeSeek;
		pStreamInfo->m_ulTimeAfterSeek = m_pPlayer->m_ulTimeAfterSeek;

		pStreamInfo->m_ulTimeBeforeSeek += pRendInfo->m_ulTimeDiff;
		pStreamInfo->m_ulTimeAfterSeek += pRendInfo->m_ulTimeDiff;
	    }
	    else
#endif /* HELIX_FEATURE_RECORDCONTROL */
	    {
		UINT32 ulLastTimeSync = (LONG32) m_pPlayer->m_ulLiveSeekTime;

		ulLastTimeSync += pRendInfo->m_ulTimeDiff;

		pStreamInfo->m_ulTimeBeforeSeek = ulLastTimeSync;
		pStreamInfo->m_ulTimeAfterSeek = ulLastTimeSync + m_pPlayer->m_ulElapsedPauseTime;
	    }
	}
	else
	{
	    pStreamInfo->m_ulTimeBeforeSeek = m_pPlayer->m_ulTimeBeforeSeek;
	    if (bSeekToLastFrame)
	    {		
		HX_ASSERT(llLastExpectedPacketTime + m_pSource->GetDelay() >= 1);
		pStreamInfo->m_ulTimeAfterSeek = INT64_TO_UINT32(llLastExpectedPacketTime + m_pSource->GetDelay() - 1);
	    }
	    else
	    {
		pStreamInfo->m_ulTimeAfterSeek = m_pPlayer->m_ulTimeAfterSeek;
	    }
	}
	
	if (pRend)
	{
	    pRend->OnPreSeek(pStreamInfo->m_ulTimeBeforeSeek,
			     pStreamInfo->m_ulTimeAfterSeek);
	}

	pStreamInfo->m_pStream->m_bPostSeekToBeSent = TRUE;

	// reset renderer info attributes 
	pRendInfo->m_ulLatestEventTime	    = UNLIKELY_EVENT_TIME;
        pRendInfo->m_ulEarliestEventTime    = UNLIKELY_EVENT_TIME;
	pRendInfo->m_bIsFirstCallback	    = TRUE;
	pRendInfo->m_ulLastSyncTime         = 0;
	pRendInfo->m_ulNextDueSyncTime      = m_pPlayer->m_ulCurrentPlayTime;

	/* Do we need to send a time sync for renderer duration? */
	if (bSeekToLastFrame || pRendInfo->m_ulNextDueSyncTime <= pRendInfo->m_ulDuration)
	{
	    pRendInfo->m_bDurationTimeSyncSent  = FALSE;
	    pRendInfo->m_bOnEndOfPacketSent = FALSE;
	}
	else
	{
	    pRendInfo->m_bDurationTimeSyncSent  = TRUE;
	}

	m_pPlayer->m_pScheduler->Remove(
         pRendInfo->m_pTimeSyncCallback->GetPendingCallback());
        
        pRendInfo->m_pTimeSyncCallback->CallbackCanceled();

	if (!pRendInfo->m_bInitialBeginToBeSent)
	{
	    pRendInfo->m_bInitialBeginToBeSent  = TRUE;
	    m_pPlayer->EnterToBeginList(pRendInfo);
	}

	if (m_pSource->IsLive())
	{
	    pRendInfo->m_BufferingReason    = BUFFERING_LIVE_PAUSE;
	}
	else
	{
	    pRendInfo->m_BufferingReason    = BUFFERING_SEEK;
	}
    }

    m_bSeekToLastFrame = bSeekToLastFrame;
    m_llLatestPacketTime = 0;
    m_llEarliestPacketTime = MAX_UINT32;

    return HXR_OK;
}

HX_RESULT
SourceInfo::BeginTrack(void)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT	    rc = HXR_OK;
    UINT32	    ulDelay = 0;
    UINT32	    ulPausedTime = 0;

    if (!m_pSource)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    if (m_pSource->IsPaused())
    {
	ulPausedTime = m_pPlayer->m_ulCurrentPlayTime - m_ulPausedStartTime;	
	ulDelay = m_pSource->GetDelay() + ulPausedTime;
	
	// update source's delay time which will call sourceinfo's
	// UpdateDelay()
	m_pSource->UpdateDelay(ulDelay);

	// resume
	m_pSource->ResumeAudioStreams();

	rc = Begin();
    }
    // track is stopped ahead of its duration
    else if (m_bStopped)
    {
	m_bStopped = FALSE;
	m_bSeekPending = TRUE;
	m_ulSeekTime = m_pPlayer->m_ulCurrentPlayTime;
	
	m_pSource->ReSetup();
    }
    // either track has not been played yet
    // or track is still playing
    else
    {
    	rc = HXR_FAILED;
	goto cleanup;
    }
   
cleanup:
    
    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
SourceInfo::PauseTrack(void)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT	rc = HXR_OK;

    // stop the source
    if (m_pSource)
    {
	m_pSource->PauseAudioStreams();
    }

    m_ulPausedStartTime = m_pPlayer->m_ulCurrentPlayTime;
    rc = Pause();

    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
SourceInfo::SeekTrack(UINT32 ulSeekTime)
{
    // low priority since it is rarely used by the SMIL
    return HXR_NOTIMPL;
}

HX_RESULT
SourceInfo::StopTrack(void)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT	rc = HXR_OK;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::StopTrack() Start",
	    m_pPlayer,
	    this);

    m_bStopped = TRUE;

    // stop the source
    if (m_pSource)
    {
	m_pSource->RemoveAudioStreams();
    }

    Reset();

    m_bDone = TRUE;
    m_pPlayer->m_uNumSourcesActive--;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::StopTrack() End",
	    m_pPlayer,
	    this);

    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
SourceInfo::SetSoundLevel(UINT16 uSoundLevel, HXBOOL bReflushAudioDevice)
{
    m_uSoundLevel = uSoundLevel;
    m_pSource->SetSoundLevel(uSoundLevel, bReflushAudioDevice);

    return HXR_OK;
}

void
SourceInfo::Reset()
{
    /* m_pSource should never be NULL */
    HX_ASSERT(m_pSource);

    if (!m_pSource)
    {
	return;
    }

#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    if(m_bIsPersistentSource)
    {
	m_bDone = FALSE;
	m_bAllPacketsReceived = FALSE;

	CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
	for (; ndxRend != m_pRendererMap->End(); ++ndxRend)
	{
	    RendererInfo* pRendInfo     = (RendererInfo*)(*ndxRend);
	    STREAM_INFO*  pStreamInfo   = pRendInfo->m_pStreamInfo;
	    pStreamInfo->m_bSrcInfoStreamDone  = FALSE;

	    /* Reset the latest packet time */
	    pRendInfo->m_ulLatestEventTime   = UNLIKELY_EVENT_TIME;
            pRendInfo->m_ulEarliestEventTime = UNLIKELY_EVENT_TIME;

	    m_pPlayer->m_pScheduler->Remove(
             pRendInfo->m_pTimeSyncCallback->GetPendingCallback());
        
            pRendInfo->m_pTimeSyncCallback->CallbackCanceled();

	    pRendInfo->m_bIsFirstCallback	    = TRUE;
	    pRendInfo->m_ulLastSyncTime         = 0;
	    pRendInfo->m_ulNumberOfPacketsQueued= 0;
	    pRendInfo->m_ulNextDueSyncTime      = 0;
	    if (!pRendInfo->m_bInitialBeginToBeSent && pRendInfo->m_pRenderer)
	    {
		pRendInfo->m_bInitialBeginToBeSent  = TRUE;
		m_pPlayer->EnterToBeginList(pRendInfo);
	    }
	}
    }
    else
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
    {	
	DoCleanup();
	RenderersCleanup();
    }

    return;
}


void
SourceInfo::DoCleanup(EndCode endCode)
{
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::DoCleanup(endCode=%d) Start",
	    m_pPlayer,
	    this,
	    endCode);

    if (m_pProcessCallback && m_pPlayer->m_pScheduler)
    {
	m_pPlayer->m_pScheduler->Remove(m_pProcessCallback->GetPendingCallback());
        m_pProcessCallback->CallbackCanceled();
    }

    // Adding END_REMOVE check helps fix PR 123782; in cases where
    // we've ended a persistent component early, we do want to clean
    // it up with the code that follows, below.  It's only in cases
    // where the track stops normally that we don't want to do the
    // cleanup; in those cases we want to leave it open in case user
    // navigates back to it:
    if (m_bIsPersistentSource  &&  END_REMOVE != endCode)
    {
	HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::DoCleanup(endCode=%d) End",
		m_pPlayer,
		this,
		endCode);
	return;
    }

    m_bLocked = TRUE;
    m_pMutex->Lock();

    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (; ndxRend != m_pRendererMap->End(); ++ndxRend)
    {
	RendererInfo* pRendInfo = (RendererInfo*)(*ndxRend);

	/* Tell each renderer that we are done with the stream */
	if(pRendInfo)
	{
            if (pRendInfo->m_pStream &&
                pRendInfo->m_pStreamInfo && pRendInfo->m_pStreamInfo->m_pHeader)
            {
                IHXBuffer* pMimeTypeBuffer = NULL;
	        pRendInfo->m_pStreamInfo->m_pHeader->GetPropertyCString("MimeType", pMimeTypeBuffer);

	        if (pMimeTypeBuffer && pMimeTypeBuffer->GetBuffer() &&
                    ::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "application/vnd.rn-objectsstream") == 0 ||
                    ::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "application/x-rn-objects") == 0 ||
		    ::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "application/vnd.rn-objects") == 0)
	        {
	            m_pPlayer->m_pEngine->m_lROBActive--;
                    HX_ASSERT(m_pPlayer->m_pEngine->m_lROBActive >=0 );
	        }

                HX_RELEASE(pMimeTypeBuffer);
            }

	    if(pRendInfo->m_pRenderer)
	    {
		pRendInfo->m_pRenderer->EndStream();
	    }

	    if (pRendInfo->m_bInitialBeginToBeSent)
	    {
		m_pPlayer->RemoveFromPendingList(pRendInfo);
	    }

	    if (pRendInfo->m_pTimeSyncCallback)
	    {
//{FILE* f1 = ::fopen("c:\\temp\\ts.txt", "a+"); ::fprintf(f1, "%p RELEASING TimeSyncCallback: Pending: %d Handle: %lu\n", pRendInfo->m_pTimeSyncCallback, (int)pRendInfo->m_bIsCallbackPending, (UINT32) pRendInfo->m_PendingHandle);::fclose(f1);}

	        m_pPlayer->m_pScheduler->Remove(
                 pRendInfo->m_pTimeSyncCallback->GetPendingCallback());
        
                pRendInfo->m_pTimeSyncCallback->CallbackCanceled();
                HX_RELEASE(pRendInfo->m_pTimeSyncCallback);
	    }

	    HX_RELEASE(pRendInfo->m_pStream);
	}
    }

    // cleanup (i.e. registry)
    m_pSource->DoCleanup(endCode);

    UnRegister();

    m_lastError = HXR_OK;
    m_bAreStreamsSetup = FALSE;
    m_bDone = FALSE;
    m_bInitialized = FALSE;
    m_bSeekPending = FALSE;
    m_bSeekToLastFrame = FALSE;

#ifdef HELIX_FEATURE_PARTIALPLAYBACK
    m_bIsPartialPlayback = FALSE;
#endif
    
    m_pMutex->Unlock();
    m_bLocked = FALSE;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::DoCleanup(endCode=%d) End",
	    m_pPlayer,
	    this,
	    endCode);
}

void
SourceInfo::Stop(EndCode endCode)
{
    /* We have already been here once */
    if (m_pSource == NULL)
    {
	return;
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::Stop(endCode=%d) Start",
	    m_pPlayer,
	    this,
	    endCode);

    DoCleanup(endCode);

    // Check for !=END_REMOVE helps fix PR 123782 where persistent component
    // ended early has to be handled differently:
    if (m_bIsPersistentSource  &&  END_REMOVE != endCode)
    {
	return;
    }

    m_bLocked = TRUE;
    m_pMutex->Lock();

    m_pSource->Stop();

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    if (m_bTrackStoppedToBeSent)
    {
	m_pPlayer->m_pGroupManager->TrackStopped(m_uGroupID, m_uTrackID);  
	m_bTrackStoppedToBeSent = FALSE;
	if (m_pPeerSourceInfo)
	{
	    m_pPeerSourceInfo->m_bTrackStoppedToBeSent = FALSE;
	}
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */

    m_pMutex->Unlock();
    m_bLocked = FALSE;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::Stop End",
	    m_pPlayer,
	    this);
}

void
SourceInfo::Remove()
{
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::Remove Start",
	    m_pPlayer,
	    this);

    // stop the source
    if (m_pSource)
    {
	m_pSource->RemoveAudioStreams();
    }

    m_bDone = TRUE;

    Stop(END_REMOVE);
    CloseRenderers();

#if defined(HELIX_FEATURE_NESTEDMETA)
    if (m_bIsPersistentSource)
    {
        m_pPlayer->m_pPersistentComponentManager->RemovePersistentComponent(
            m_ulPersistentComponentSelfID);
    }
#endif   

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::Remove End",
	    m_pPlayer,
	    this);
}

/*
 * -- LIVE SYNC SUPPORT --
 *
 * SharedWallClock class is used for live sync support.
 * There is one of these objects per shared wall clock.
 *
 * This object will also be associated with an HXPlayer
 * since we won't ever share wall clocks between two players.
 * This is an intentional design choice, since it doesn't
 * reall make sense to do any sync-ing across multiple players.
 * 
 * This object will keep a list of source's that are sharing
 * it. This is mainly needed so that the wall clock can
 * reset the start time for all the sources in the event that
 * it gets a new start time that is earlier than it had
 * previously thought. Basically, the start time of the wall
 * clock is what does the syncing.      
 */
SharedWallClock::SharedWallClock
(
    const char* pName, 
    UINT32 ulStartTime,
    HXPlayer* pPlayer
)
    : m_strName(pName)
    , m_ulStartTime(ulStartTime)
    , m_pPlayer(pPlayer)
{
    HX_ASSERT(m_pPlayer);
    (*m_pPlayer->m_pSharedWallClocks)[pName] = this;
};

/*
 * -- LIVE SYNC SUPPORT --
 *
 * Obviously this is the method that handles resyncing the
 * start times for all the sources that share the wall clock.
 *
 * This method also returns the start time. The intended useage
 * is for the caller to pass the start time that it expects, and
 * then respect the start time that is returned. In the event that
 * the input start time is less than the previously known start
 * time, then all sources are reset.
 */
UINT32
SharedWallClock::ResetStartTime(UINT32 ulStartTime)
{
    if (((LONG32) (ulStartTime - m_ulStartTime)) < 0)
    {
	m_ulStartTime = ulStartTime;
	CHXSimpleList::Iterator indx = m_UserList.Begin();

	for (; indx != m_UserList.End(); ++indx)
	{
	    SourceInfo* pSourceInfo = (SourceInfo*)(*indx);
	    pSourceInfo->ResetStartTime(ulStartTime);
	}
    }
    return m_ulStartTime;
};

/*
 * -- LIVE SYNC SUPPORT --
 *
 * Nothing tricky here, we just track the incoming user on our
 * list of known users.
 */
void
SharedWallClock::AddUser(SourceInfo* pSourceInfo)
{
    m_UserList.AddTail(pSourceInfo);
};

/*
 * -- LIVE SYNC SUPPORT --
 *
 * This method removes users from the shared clock's user list
 * and also cleans up the shared clock when the last user is
 * removed. So, any caller of this method should take note to
 * NOT use the shared clock after calling this method.
 */
void
SharedWallClock::RemoveUser(SourceInfo* pSourceInfo)
{
    LISTPOSITION pos = m_UserList.Find(pSourceInfo);
    HX_ASSERT(pos);

    m_UserList.RemoveAt(pos);
    
    if (m_UserList.IsEmpty())
    {
	m_pPlayer->m_pSharedWallClocks->RemoveKey(m_strName);
	delete this;
    }
};

/*
 * -- LIVE SYNC SUPPORT --
 *
 * If the start time is changed for a source then all the
 * renderer's need to have their start time's reset. This
 * method just loops through the renderer info to do the work.
 */
void
SourceInfo::ResetStartTime(UINT32 ulStartTime)
{
    INT32 lStartTimeAdjustment = ((LONG32) (ulStartTime - m_ulStreamStartTime));

    // We adjust only to the earlier start time
    if (lStartTimeAdjustment < 0)
    {
	m_ulStreamStartTime = ulStartTime;

	// If we already started rendering this source, it is too late for
	// start time adjustment but we can delay the the source and have it
	// effectively pause until in sync based on desired start-time alignment.
	if (HasSourceRenderingBegun())
	{
	    m_pSource->UpdateDelay(m_pSource->GetDelay() - lStartTimeAdjustment);
	}
	else
	{
	    CHXMapLongToObj::Iterator indx = m_pRendererMap->Begin();

	    for (; indx != m_pRendererMap->End(); ++indx)
	    {
		RendererInfo* pRendererInfo = (RendererInfo*)(*indx);
		pRendererInfo->m_ulStreamStartTime = ulStartTime;
	    }
	}	
    }
};

/*
 * -- LIVE SYNC SUPPORT --
 *
 * This is the real work horse for live sync support. This
 * method actually determines if the URL for the source is
 * targeted at a shared wall clock by looking for the wallclock
 * option in the URL.
 *
 * If the wallclock name is found then we try to find the wall
 * clock object for this name and player. If we don't find it
 * then we create a new clock. If we do find it then we want
 * to determine if the start time should be reset to the lower
 * value. We also return the start time to be stored in the
 * renderers.
 */
UINT32
SourceInfo::CalculateLiveStartTime(IHXPacket* pFirstPacket)
{
    UINT32 ulStreamStartTime = pFirstPacket->GetTime();;

    /*
     * Find out if this live feed has a "wallclock" property.
     */
    CHXURL*	pURL = m_pSource->GetCHXURL();
    IHXValues*  pValues = pURL->GetOptions();
    IHXBuffer* pWallClockName = NULL;

    pValues->GetPropertyBuffer("wallclock", pWallClockName);

    if (pWallClockName)
    {
	m_strWallClockName = (const char*)pWallClockName->GetBuffer();

	/* We shouldn't already have a wall clock! */
	HX_ASSERT(m_pWallClock == NULL);

	/*
	 * Lookup a wall clock with this name, if it's not found then
	 * we are the first one, and we should create a shared wallclock
	 * object, add it to the list, and do the standard setup.
	 */
	void* pLookupResult = NULL;
	if (!m_pPlayer->FindSharedWallClocks(m_strWallClockName,pLookupResult))
	{
	    m_pWallClock = new SharedWallClock(m_strWallClockName,
					       ulStreamStartTime,
					       m_pPlayer);
	}
	else
	{
	    m_pWallClock = (SharedWallClock*)pLookupResult;
	    ulStreamStartTime = m_pWallClock->ResetStartTime(ulStreamStartTime);
	}
	m_pWallClock->AddUser(this);
    }

    HX_RELEASE(pValues);
    HX_RELEASE(pWallClockName);

    return ulStreamStartTime;
}

/*
 * -- LIVE SYNC SUPPORT --
 *
 * Basic clean up support for wall clock object. Notice that after
 * calling RemoveUser, you can use the wall clock object since it
 * may get deleted... so we always reset our wall clock object
 * pointer to NULL.
 */
void
SourceInfo::DoneWithWallClock()
{
    if (m_pWallClock)
    {
	m_pWallClock->RemoveUser(this);
	m_pWallClock = NULL;
    }
}

HX_RESULT
SourceInfo::ProcessIdle(ULONG32&    ulNumStreamsToBeFilled,
		        HXBOOL&	    bIsRebuffering,
		        UINT16&     uLowestBuffering,
			UINT32	    ulLoopEntryTime,
			UINT32	    ulProcessingTimeAllowance)
{
    HX_RESULT theErr = HXR_OK;
    CHXEvent* pEvent = NULL;
    HXSource* pSource = NULL;
    IHXPendingStatus* pStatus = NULL;
    RendererInfo* pRendInfo = NULL;
    STREAM_INFO* pStreamInfo = NULL;
    IHXPacket* pPacket = NULL;
    UINT16 unStatusCode = 0;
    UINT16 unPercentDone = 0;
    UINT32 ulPacketTime = 0;
    INT64 llActualPacketTime = 0;
    INT64 llLastExpectedPacketTime = 0;
    IHXBuffer* pStatusDesc = NULL;
    HXBOOL bAtInterrupt = FALSE;
    HXBOOL bHasUnfilledStreamsThusFar = FALSE;

    bAtInterrupt = m_pPlayer->m_pEngine->AtInterruptTime();

    if (m_bStopped)
    {
	return HXR_OK;
    }

    /* Check if a source has not been initialized. This will happen
     * ONLY when we start a new track in the mid of a presentation
     */
    if (!m_bInitialized)
    {
	/* Do not initialize source at interrupt time */
	if (bAtInterrupt)
	{
	    // If we are not initialized yet and we cannot initialize at this
	    // moment, indicate we are buffering at 0% (buffering just starting).
	    // This will prevent time-line from resuming without waiting for
	    // this source to complete buffering.
	    bIsRebuffering = TRUE;
	    uLowestBuffering = 0;
	    ScheduleProcessCallback();
	    return HXR_OK;
	}

	theErr = InitializeAndSetupRendererSites();

	if (theErr || !m_bInitialized)
	{
	    return theErr;
	}
    }

    if (m_bSeekPending)
    {
	m_bSeekPending = FALSE;
	
	Pause();
	Seek(m_ulSeekTime);
	m_pSource->DoSeek(m_ulSeekTime);
	Begin();
    }

    pSource = m_pSource;
    pStatus = m_pStatus;

    // don't start getting events till the player setup has been
    // done   
    if (!m_pPlayer->m_bInitialized  || 
	m_pPlayer->m_bSetupToBeDone || 
	m_pPlayer->m_bPostSetupToBeDone)
    {
	return HXR_OK;
    }
    
    UINT16 uIndex	= 0;
    HXBOOL bHandled	= TRUE;
    llLastExpectedPacketTime = m_pSource->GetLastExpectedPacketTime();

    LISTPOSITION posRend = m_pCurrentScheduleList->GetHeadPosition();
    for (; uIndex < m_pCurrentScheduleList->GetCount(); uIndex++)
    {
	HXBOOL bSentMe = TRUE;
	HXBOOL bEndMe = FALSE;	
        HXBOOL bSkipDeliveryTimeCheck = FALSE;
        UINT32 ulDeliveryTime = 0;
        HXBOOL bDecrementNumStreamsToBeFilled = FALSE;
	HXBOOL bDeliver = FALSE;

	llActualPacketTime = 0;
	
	theErr = HXR_OK;

	if (uIndex > 0)
	{
	    m_pCurrentScheduleList->GetNext(posRend);
	}

	bHandled = TRUE;
	pRendInfo = (RendererInfo*) m_pCurrentScheduleList->GetAt(posRend);

	pStreamInfo = pRendInfo->m_pStreamInfo;

	if (pStreamInfo->m_bSrcInfoStreamDone)
	{
	    // SPECIAL CASE:
	    // the player received all the packets(m_bSrcStreamDone is TRUE) and 
	    // EndOfPacket() hasn't been sent to the renderer yet, 
	    // BUT the renderer still calls ReportRebufferStatus()	
	    if (!IsRebufferDone())
	    {
		bIsRebuffering = TRUE;
	    }
	    else
	    {
		CheckIfDone();
	    }
            // If this is a packetless source, then we still
            // need to get our buffering information from the 
            // source's pending status interface
            if (pSource->IsPacketlessSource())
            {
                pStatus->GetStatus(unStatusCode, pStatusDesc, unPercentDone);
                HX_RELEASE(pStatusDesc);
	        if ((HX_STATUS_BUFFERING == unStatusCode && unPercentDone < 100) ||
	            HX_STATUS_CONTACTING == unStatusCode)
	        {
	            bIsRebuffering = TRUE;
	            if (uLowestBuffering > unPercentDone)
	            {
		        uLowestBuffering = unPercentDone;
	            }
	        }
            }
	    continue;
	}

	pStreamInfo->m_bSrcInfoStreamFillingDone = FALSE;
	ulNumStreamsToBeFilled++;

	// every HXSource has to implement IID_IHXPendingStatus
	HX_ASSERT(pStatus);

	HX_VERIFY(HXR_OK == pStatus->GetStatus(unStatusCode, pStatusDesc,
					       unPercentDone));
	HX_RELEASE(pStatusDesc);

	if ((HX_STATUS_BUFFERING == unStatusCode && unPercentDone < 100) ||
	    HX_STATUS_CONTACTING == unStatusCode)
	{
	    // We must indicate buffering status to the caller prior to obtaining the
	    // event.  Obtaining the event may change this buffering the status but the
	    // caller needs to know the status prior to the event or otherwise caller
	    // may miss buffering to non-buffering state transition.
	    // The transition is importnat to the caller since events that satisfied the
	    // buffering must be dispatched to the renderers before starting the time-line
	    // or we may not have enough data to start without hesitation.
	    bIsRebuffering = TRUE;
            bSkipDeliveryTimeCheck = TRUE;
	    bDeliver = TRUE;

	    if (uLowestBuffering > unPercentDone)
	    {
		uLowestBuffering = unPercentDone;
	    }

	    // If this stream is fully buffered and there are other streams in
	    // this source that are yet to become fully buffered, do not retreive 
	    // events for this stream in order to give CPU cycles to the streams 
	    // that still need to fulfill the buffering.
	    // We skip this optimization when rendering is disabled to any degree
	    // since in such case the engine may be used as the data source provider
	    // and holding off the data may be produce performance issues in
	    // the controlling object.
	    if (pSource->GetRenderingDisabledMask() == HXRNDR_DISABLED_NONE)
	    {
		if (pStreamInfo->BufferingState().GetPercentDone() == 100)
		{
		    bDeliver = FALSE;

		    if (!bHasUnfilledStreamsThusFar)
		    {
			// See if the remaining streams are also unfilled
			UINT16 uRemainingIdx;
			RendererInfo* pRemainingInfo;
			LISTPOSITION posRemaining = posRend;

			bDeliver = TRUE;

			for (uRemainingIdx = uIndex + 1; 
			     uRemainingIdx < m_pCurrentScheduleList->GetCount(); 
			     uRemainingIdx++)
			{
			    m_pCurrentScheduleList->GetNext(posRemaining);
			    pRemainingInfo = (RendererInfo*) m_pCurrentScheduleList->GetAt(posRemaining);
			    if (pRemainingInfo->m_pStreamInfo->BufferingState().GetPercentDone() < 100)
			    {
				bHasUnfilledStreamsThusFar = TRUE;
				bDeliver = FALSE;
				break;
			    }
			}
		    }
		}
		else
		{
		    bHasUnfilledStreamsThusFar = TRUE;
		}
	    }
	}
        else
        {
	    /* the event times are actual ts - preroll.. so we do not
	     * need to add preroll in calculations here...
	     */	  
	    if (!m_pPlayer->IsPlaying())
	    {
		// If we are no longer buffering and not playing yet, do not
		// spend CPU on retrieveing events until we start in order
		// to facilitate faster resumption.
		bSkipDeliveryTimeCheck = TRUE;
		bDeliver = FALSE;
	    }
	    else
	    {
		ulDeliveryTime = m_pPlayer->ComputeFillEndTime(m_pPlayer->m_ulCurrentPlayTime,
							       m_pPlayer->m_ulLowestGranularity,
							       0);
	    }
        }

	if (pSource)
	{
	    pSource->FillRecordControl(ulLoopEntryTime);
	}

	// We deliver if event time reference is not set yet or if based on the reference time
	// the next event is should be delivered.
	if (!bSkipDeliveryTimeCheck)
	{
	    if (m_pSource->IsAcceleratingEventDelivery())
	    {
		ulDeliveryTime = m_pSource->CalcAccelDeliveryTime(ulDeliveryTime);            
	    }

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
	    if (m_pPlayer->GetVelocity() < 0)
	    {
		bDeliver = (((((INT32) (ulDeliveryTime - pRendInfo->m_ulEarliestEventTime)) <= 0) || 
			    (pRendInfo->m_ulEarliestEventTime == UNLIKELY_EVENT_TIME)));
	    }
	    else
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
	    {
		bDeliver = (((((INT32) (ulDeliveryTime - pRendInfo->m_ulLatestEventTime)) >= 0) || 
			    (pRendInfo->m_ulLatestEventTime == UNLIKELY_EVENT_TIME)));
	    }
	}
                                                            
	if (bDeliver)
        {
    	    theErr = pSource->GetEvent(pStreamInfo->m_uStreamNumber, 
				       pEvent, 
				       ulLoopEntryTime,
				       ulProcessingTimeAllowance);

	    if (!theErr)
	    {
                if (m_pSource->IsAcceleratingEventDelivery())
                {
                    // Signal that this event must be delivered 
                    // immediately
                    pEvent->SetImmediateEvent();
                }

		pPacket = pEvent->GetPacket();

		if (pEvent->IsPreSeekEvent())
		{
		     // associate the packet with its renderer..
		     pEvent->m_pRendererInfo   = pRendInfo;		    
		     if (!bAtInterrupt || pRendInfo->m_bInterruptSafe)
		     {
			 theErr = m_pPlayer->SendPacket(pEvent);
			 delete pEvent;
		     }
		     else
		     {
			// insert event in the common packet/event list...
			theErr = m_pPlayer->m_EventList.InsertEvent(pEvent);
                        if( theErr == HXR_OUTOFMEMORY )
                        {
                            return HXR_OUTOFMEMORY;
                        }
			pRendInfo->m_ulNumberOfPacketsQueued++;
		     }

		     continue;
		}

		if (!pPacket->IsLost())
		{
		    ulPacketTime = pPacket->GetTime();

		    llActualPacketTime = 
			pRendInfo->m_pStreamInfo->BufferingState().CreateINT64Timestamp(ulPacketTime);

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
                    if (m_pPlayer->GetVelocity() < 0)
                    {
		        if (llActualPacketTime < m_llEarliestPacketTime)
		        {
			    m_llEarliestPacketTime = llActualPacketTime;
		        }

			if ((((INT32) (pRendInfo->m_ulEarliestEventTime - pEvent->GetTimeStartPos())) >= 0) || 
			    (pRendInfo->m_ulEarliestEventTime == UNLIKELY_EVENT_TIME))
                        {
                            pRendInfo->m_ulEarliestEventTime = pEvent->GetTimeStartPos();
                        }
                        else
                        {
                            // Out-of-order reverse packets
                            pEvent->SetTimeStartPos(pRendInfo->m_ulEarliestEventTime);
                        }
                    }
                    else
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
                    {
		        if (m_llLatestPacketTime < llActualPacketTime)
		        {
			    m_llLatestPacketTime = llActualPacketTime;
		        }

			if ((((INT32) (pEvent->GetTimeStartPos() - pRendInfo->m_ulLatestEventTime)) >= 0) || 
			    (pRendInfo->m_ulLatestEventTime == UNLIKELY_EVENT_TIME))
			{
                            pRendInfo->m_ulLatestEventTime  = pEvent->GetTimeStartPos();
                        }
                        else
                        {
                            // Make sure that the event times are monotonically increasing.
                            // This will prevent the packets from getting reordered.
                            // This branch should only execute in 1 of 3 cases.
                            // Case 1:
                            //   During a SureStream switch we may receive packets that 
                            //   have a timestamp that is lower than the previous packet.
                            //   These packets are usually from different rules so technically
                            //   it is ok that they can be reordered. We are going to prevent
                            //   reordering here to avoid confusion.
                            //
                            // Case 2:
                            //   You are playing a datatype that doesn't have monotonically 
                            //   increasing timestamps. MPEG2 video with B frames could be 
                            //   an example of this. You don't want packets to get reordered 
                            //   because the packets need to stay in decode order eventhough 
                            //   the timestamps represent presentation order.
                            //
                            // Case 3: 
                            //   Something has gone wrong with the event time computations.
                            //   If you hit this branch and you are not dealing with Case 1 
                            //   or Case 2 you need to investigate what is going wrong. 
                            //   Setting the event time to the last event time prevents the 
                            //   packets from getting reordered.
                            pEvent->SetTimeStartPos(pRendInfo->m_ulLatestEventTime);
                        }
                    }

		    if (pSource->IsLive() && pRendInfo->m_bIsFirstPacket)
		    {
			pRendInfo->m_bIsFirstPacket     = FALSE;

			/*
			 * -- LIVE SYNC SUPPORT --
			 *
			 * We used to just set the stream start time to the 
			 * timestamp of this first packet. Now we use this
			 * helper function to implement support for sharing
			 * start times for sources off of the same wall clock.
			 * See other LIVE SYNC SUPPORT comments for more 
			 * details.
			 */
			UINT32 ulLiveStart = CalculateLiveStartTime(pPacket);
			HXBOOL bSourceRenderingStarted = HasSourceRenderingBegun();

			// If rendering of this source hasn't started yet, we can adjust
			// the start time for the streams within the source to the lowest
			// stream start time.  Once the rendering start, no further
			// start time adjustments can be made.
			if (!bSourceRenderingStarted)
			{
			    UINT32 ulLowestTime = ulLiveStart;
			    RendererInfo* pTmpRendInfo;
			    CHXMapLongToObj::Iterator tmpndxRend;

			    // Find the lowest valid stream start time
			    for (tmpndxRend = m_pRendererMap->Begin();
				 tmpndxRend != m_pRendererMap->End();
				 ++tmpndxRend)
			    {
				pTmpRendInfo = (RendererInfo*) (*tmpndxRend);
				if (pTmpRendInfo->m_bStartTimeValid)
				{
				    if (((LONG32) (pTmpRendInfo->m_ulStreamStartTime - ulLowestTime)) < 0)
				    {
					ulLowestTime = pTmpRendInfo->m_ulStreamStartTime;
				    }
				}
			    }

			    // Set the lowest stream start time same for all streams in the source.
			    // Having all streams have the same stream start time is neeed for the
			    // streams in the source to remain synchronized.
			    for (tmpndxRend = m_pRendererMap->Begin();
				 tmpndxRend != m_pRendererMap->End();
				 ++tmpndxRend)
			    {
				pTmpRendInfo = (RendererInfo*) (*tmpndxRend);
				pTmpRendInfo->m_ulStreamStartTime = ulLowestTime;
				pTmpRendInfo->m_bStartTimeValid = TRUE;
				SetLiveSyncStartTime(pSource, pTmpRendInfo, ulLowestTime);
			    }
			    // This the start time for the entire source.
			    m_ulStreamStartTime = ulLiveStart;
			}
		    }

		    if ((!pSource->IsLive() || pSource->isRestrictedLiveStream()) &&
			llActualPacketTime > llLastExpectedPacketTime)
		    {
			// XXX HP
			// work around since the rule flag of all RTSP packets are set to
			// HX_ASM_SWITCH_ON only and this could cause the stream doesn't end 
			// properly if its endtime was changed after the first resume has been 
			// sent
			// THIS WILL BE FIXED IN THE NEXT SERVER RELEASE("dial-tone")  
			if (pSource->m_bRTSPRuleFlagWorkAround)
			{
			    bSentMe = FALSE;
			    bEndMe = TRUE;
			}
			else 
			{
			    pRendInfo->m_pStream->PostEndTimePacket(pPacket, bSentMe, bEndMe);
			}
		    }
		}		    

		if (bSentMe)
		{		    
		    // clear up the post events which were not sent
		    if (pStreamInfo->m_pPostEndTimeEventList)
		    {
			while (pStreamInfo->m_pPostEndTimeEventList->GetNumEvents())
			{
			    CHXEvent* pPostEndTimeEvent = pStreamInfo->m_pPostEndTimeEventList->RemoveHead();

			    // associate the packet with its renderer..
			    pPostEndTimeEvent->m_pRendererInfo   = pRendInfo;
			    // insert event in the common packet/event list...
			    theErr = m_pPlayer->m_EventList.InsertEvent(pPostEndTimeEvent);
                            if( theErr == HXR_OUTOFMEMORY )
                            {
                                return HXR_OUTOFMEMORY;
			    }
			    pRendInfo->m_ulNumberOfPacketsQueued++;
			}
		    }

		    // associate the packet with its renderer..
		    pEvent->m_pRendererInfo   = pRendInfo;
		    // insert event in the common packet/event list...
		    theErr = m_pPlayer->m_EventList.InsertEvent(pEvent);
		    if( theErr == HXR_OUTOFMEMORY )
                    {
                        return HXR_OUTOFMEMORY;
		    }
		    pRendInfo->m_ulNumberOfPacketsQueued++;
		}
		else
		{
		    if (!pStreamInfo->m_pPostEndTimeEventList)
		    {
			pStreamInfo->m_pPostEndTimeEventList = new CHXEventList();
		    }

		    if (pStreamInfo->m_pPostEndTimeEventList)
		    {
//{FILE* f1 = ::fopen("c:\\temp\\out.txt", "a+"); ::fprintf(f1, "bSentMe is FALSE PacketTime: %lu PacketRule: %u\n", pEvent->GetPacket()->GetTime(), pEvent->GetPacket()->GetASMRuleNumber());::fclose(f1);}			
			theErr = pStreamInfo->m_pPostEndTimeEventList->InsertEvent(pEvent);
			if( theErr == HXR_OUTOFMEMORY )
			{
			    return theErr;
			}
		    }
		}
	    }
	    else
	    {
		bHandled = FALSE;
	    }

	    if (theErr)
	    {
                bDecrementNumStreamsToBeFilled = TRUE;
	    }
	}
	else
	{
            bDecrementNumStreamsToBeFilled = TRUE;
        }

        if (bDecrementNumStreamsToBeFilled &&
	    !pStreamInfo->m_bSrcInfoStreamFillingDone &&
            ulNumStreamsToBeFilled > 0)
        {
            pStreamInfo->m_bSrcInfoStreamFillingDone = TRUE;
            ulNumStreamsToBeFilled--;
	}

        if (theErr == HXR_AT_END)
	{
	    if (!pStreamInfo->m_bSrcInfoStreamDone)
	    {
		if (!bAtInterrupt || pRendInfo->m_bInterruptSafe)
		{
		    pStreamInfo->m_bSrcInfoStreamDone = TRUE;

		    pStreamInfo->ResetPostEndTimeEventList();

		    if (pRendInfo->m_ulNumberOfPacketsQueued == 0)
		    {			
			m_pPlayer->SendPostSeekIfNecessary(pRendInfo);
			if (pRendInfo->m_pRenderer)
			{

#if defined(HELIX_FEATURE_DRM)
                            //flush the DRM pending packets
                            if (m_pSource && m_pSource->IsHelixDRMProtected() && m_pSource->GetDRM())
                            {
                                m_pSource->GetDRM()->FlushPackets(TRUE);
                            }
#endif /* HELIX_FEATURE_DRM */

			    pRendInfo->m_pRenderer->OnEndofPackets();
			}
			pRendInfo->m_bOnEndOfPacketSent = TRUE;	
		    }

		    CheckIfDone();
		}
		else
		{
		    ScheduleProcessCallback();
		    continue;
		}
	    }
	}
	else if (theErr == HXR_NO_DATA)
	{
	    /* mask this error */
	    theErr  = HXR_OK;
	}

	if (!llActualPacketTime)
	{
	    STREAM_INFO* pStrInfo = pRendInfo->m_pStreamInfo;
	    UINT32 ulPktTime = 
		pStrInfo->BufferingState().LastPacketTimestamp();
	    llActualPacketTime = 
		pStrInfo->BufferingState().CreateINT64Timestamp(ulPktTime);
	    
	    /* if this is a stream where we do not have to wait to receive 
	     * a packet >= stream's last expected packet time, 
	     * make the highest timestamp to be the HIGHEST timestamp
	     * across ALL streams for this source.
	     *
	     * This is to fix end tag support on sources with sparse streams
	     * e.g. audio/video with an event stream where we may not really 
	     * have a packet till way down in the future.  
	     */ 
	    if (pRendInfo->m_pStreamInfo->m_bCanBeStoppedAnyTime &&
		!pStreamInfo->m_bSrcInfoStreamDone &&
		m_llLatestPacketTime > llLastExpectedPacketTime)
	    {
		/* check if ALL other streams have ended */
		if (AllOtherStreamsHaveEnded(pStreamInfo))
		{
                    /*
                     * The logic above was designed to handle endTime's
                     * on a/v streams, so that once we received a/v packets
                     * past the end time (even though more packets are 
                     * still coming in), we can go ahead and end the
                     * event stream. However, this was causing a bug
                     * where syncmm event streams were getting terminated
                     * even though there was a packet waiting for it
                     * at the transport. This additional test imposes
                     * that we don't terminate the stream unless we
                     * have no packets waiting at the transport AND
                     * the stream has said it's done.
                     */
                    if (pStreamInfo->BufferingState().DoneAtTransport())
                    {
    		        bEndMe		= TRUE;
		        llActualPacketTime  = m_llLatestPacketTime;
                    }
		}
	    }
	}

	if ((!pSource->IsLive() || pSource->isRestrictedLiveStream()) && 
	    !pStreamInfo->m_bSrcInfoStreamDone &&
	    llActualPacketTime > llLastExpectedPacketTime &&
	    bEndMe)
	{
	    if (!bAtInterrupt || pRendInfo->m_bInterruptSafe)
	    {
		pStreamInfo->m_bSrcInfoStreamDone = TRUE;

		pStreamInfo->ResetPostEndTimeEventList();

		if (!pStreamInfo->m_bSrcInfoStreamFillingDone &&
		    ulNumStreamsToBeFilled > 0)
		{
		    pStreamInfo->m_bSrcInfoStreamFillingDone = TRUE;
		    ulNumStreamsToBeFilled--;
		}

		if (pRendInfo->m_ulNumberOfPacketsQueued == 0)
		{
		    m_pPlayer->SendPostSeekIfNecessary(pRendInfo);
		    if (pRendInfo->m_pRenderer)
		    {

#if defined(HELIX_FEATURE_DRM)
                        //flush the DRM pending packets
                        if (m_pSource && m_pSource->IsHelixDRMProtected() && m_pSource->GetDRM())
                        {
                            m_pSource->GetDRM()->FlushPackets(TRUE);
                        }
#endif /* HELIX_FEATURE_DRM */

			pRendInfo->m_pRenderer->OnEndofPackets();
		    }
		    pRendInfo->m_bOnEndOfPacketSent = TRUE;	
		}

		if (!pSource->isRestrictedLiveStream())
		{
		    CheckIfDone();
		}
	    }
	    else
	    {
		ScheduleProcessCallback();
		continue;
	    }
	}
    }

    if (m_bInitialized &&
	m_ulSourceDuration < m_pPlayer->m_ulCurrentPlayTime && 
	m_pSource && 
	!m_pSource->IsLive() &&
	m_pSource->IsActive())
    {
	/* Remove bandwidth usage for this source */
	m_pSource->AdjustClipBandwidthStats(FALSE);
    }
    else if (m_bInitialized &&
	    m_pSource &&
	    m_pPlayer->m_ulCurrentPlayTime > m_pSource->GetDelay() &&
	    m_pPlayer->m_ulCurrentPlayTime <= m_ulSourceDuration && 
	    !m_pSource->IsActive())
    {
	/* Add bandwidth usage for this source */
	m_pSource->AdjustClipBandwidthStats(TRUE);
    }

    return HXR_OK;
}

HX_RESULT
SourceInfo::Register()
{
    HX_RESULT theErr = HXR_OK;
    if (!m_bIsRegisterSourceDone)
    {
	m_pSource->CanBeFastStarted();

        if (m_pSource->m_bSureStreamClip)
        {
            m_pPlayer->SureStreamSourceRegistered(this);
        }

	m_bIsRegisterSourceDone = TRUE;

        if (!m_pSource->IsRateAdaptationUsed())
        {      
#if defined(HELIX_FEATURE_ASM)
            /* Register Source with ASM Bandwidth Manager */
            IHXBandwidthManager* pMgr = 0;

            HX_VERIFY(HXR_OK == m_pPlayer->QueryInterface(
                IID_IHXBandwidthManager, (void **)&pMgr));

            theErr = pMgr->RegisterSource(m_pSource, (IUnknown*) (IHXPlayer*) m_pPlayer);
            pMgr->Release();
#endif /* HELIX_FEATURE_ASM */
        }
    }
    return theErr;
}

HX_RESULT
SourceInfo::UnRegister()
{
    HX_RESULT theErr = HXR_OK;
    if (m_bIsRegisterSourceDone)
    {
	m_bIsRegisterSourceDone = FALSE;

	if (m_pSource->m_bSureStreamClip)
	{
	    m_pPlayer->SureStreamSourceUnRegistered(this);
	}

#if defined(HELIX_FEATURE_ASM)
	/* Register Source with ASM Bandwidth Manager */
	IHXBandwidthManager* pMgr = 0;

	HX_VERIFY(HXR_OK == m_pPlayer->QueryInterface(
	    IID_IHXBandwidthManager, (void **)&pMgr));

	HX_ASSERT(pMgr) ;
	if (pMgr)
	{
		theErr = pMgr->UnRegisterSource(m_pSource);
		pMgr->Release();
	}

        CheckIfDone();
#endif /* HELIX_FEATURE_ASM */
    }

    return theErr;
}

void		
SourceInfo::ChangeAccelerationStatus(HXBOOL bMayBeAccelerated,
				     HXBOOL bUseAccelerationFactor,
				     UINT32 ulAccelerationFactor)
{
    IHXBandwidthManager* pMgr = 0;

    HX_VERIFY(HXR_OK == m_pPlayer->QueryInterface(
	IID_IHXBandwidthManager, (void **)&pMgr));

    if (pMgr)
    {
	pMgr->ChangeAccelerationStatus(m_pSource, bMayBeAccelerated,
				       bUseAccelerationFactor, ulAccelerationFactor);
	pMgr->Release();
    }
}

void SourceInfo::CheckIfDone()
{
    HXBOOL bIsDone = TRUE;
    CHXMapLongToObj::Iterator   ndxRend;
    RendererInfo*   pRendInfo   = NULL;
    STREAM_INFO*    pStreamInfo = NULL;

    // keep the source active throughout the duration
    // of source
    if (m_bDone					&&
	m_bActive				&& 
	m_pPlayer->m_uNumSourcesActive > 0	&&
	!KeepSourceActive())
    {	  
        m_pSource->m_ulPlaybackStoppedTix = HX_GET_TICKCOUNT();
	m_bActive = FALSE;
	m_pPlayer->m_uNumSourcesActive--;
    }

    if (!m_bDone)
    {
	ndxRend = m_pRendererMap->Begin();

	for (;ndxRend != m_pRendererMap->End(); ++ndxRend)
	{
	    pRendInfo       = (RendererInfo*)(*ndxRend);
	    pStreamInfo     = pRendInfo->m_pStreamInfo;

	    if (!pStreamInfo->m_bSrcInfoStreamDone)
	    {
		bIsDone = FALSE;
		break;
	    }
	}

        // Adding the check for !KeepSourceActive() fixes PR 126671 where
        // a container-file multi-stream source is ended early base on some
        // external timing (like "[URL]?end=5") and contains a sparse stream
        // whose last packet is prior to that externally-applied end time.
        // In that case, the sparse (events) stream was never being allowed
        // to end at the specified; this allows it to:
	if (bIsDone || m_pSource->IsSourceDone()  ||
                !KeepSourceActive())
	{
	    m_bDone = TRUE;	

	    if (!m_bAllPacketsReceived)
	    {
		m_bAllPacketsReceived = TRUE;
		// If we are done receiving all the packets, release bandwidth
		UnRegister();
	    }

	    if (!m_pSource->IsSourceDone())
	    {
		m_pSource->SetEndOfClip(TRUE);
	    }

	    if (m_pPlayer->m_uNumCurrentSourceNotDone > 0)
	    {
		m_pPlayer->m_uNumCurrentSourceNotDone--;
	    }
	}
    }

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    if (!m_bActive && 
	m_bTrackStoppedToBeSent &&
	!m_pPlayer->m_pEngine->AtInterruptTime())
    {
	// send TrackStopped at the end of its active duration
	m_bTrackStoppedToBeSent = FALSE;
	m_pPlayer->m_pGroupManager->TrackStopped(m_uGroupID, m_uTrackID);  
	if (m_pPeerSourceInfo)
	{
	    m_pPeerSourceInfo->m_bTrackStoppedToBeSent = FALSE;
	}
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

void
SourceInfo::SetupRendererSites(HXBOOL bIsPersistent)
{
    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (; ndxRend != m_pRendererMap->End(); ++ndxRend)
    {
	RendererInfo* pRendInfo   = (RendererInfo*) (*ndxRend);
	IHXRenderer* pRenderer   = (IHXRenderer*) pRendInfo->m_pRenderer;

	HX_DISPLAY_TYPE    ulFlags;
	IHXBuffer*         pInfoBuffer = NULL;

	/*
	 * Find out if the renderer is a display type renderer.
	 */
	if (pRenderer &&
	    (HXR_OK == pRenderer->GetDisplayType(ulFlags,pInfoBuffer)))
	{
	    HX_RELEASE(pInfoBuffer);
	    if (HX_DISPLAY_WINDOW == (HX_DISPLAY_WINDOW & ulFlags))
	    {
		STREAM_INFO*  pStreamInfo = pRendInfo->m_pStreamInfo;
		IHXValues*   pProps      = pStreamInfo->m_pStreamProps;
		/*
		 * Call the helper function that handles setup of a
		 * renderer site. This function will do the
		 * whole process of finding out if a site exists by
		 * PlayToFrom; if not, it calls the site supplier; then it
		 * it actually hooks up the SiteUserSupplier with all
		 * the appropriate sites, etc.
		 */
		m_pPlayer->SetupRendererSite(pRenderer,pProps,bIsPersistent);
	    }
	}
    }
}


HX_RESULT
SourceInfo::InitializeRenderers(HXBOOL& bSourceInitialized)
{
    HX_RESULT    theErr              = HXR_OK;
    HX_RESULT    theFinalErr         = HXR_OK;
    IHXValues*   pHeader             = NULL;
    HXStream*    pStream             = NULL;
    IHXRenderer* pRenderer           = NULL;
    IHXBuffer*   pMimeTypeBuffer     = NULL;
    IUnknown*    pUnkRenderer        = NULL;
    STREAM_INFO* pStreamInfo         = NULL;
    HXBOOL       bAddDefaultUpgrade  = FALSE;
    IHXBuffer*   pStreamPlayTo       = NULL;
    
#ifdef HELIX_FEATURE_PARTIALPLAYBACK
    // By default enable partial playback. Pref shall override if reqd
    HXBOOL bPartialPlaybackEnabled = TRUE;
    UINT32 ulMinValidRenderersReqd = DEFAULT_MIN_VALID_RENDERER_REQD;
    UINT32 ulNumNULLRenderer = 0;
    
    // Read the preference for enabling partial playback
    if(ReadPrefBOOL(m_pPlayer->m_pPreferences, DISABLE_PARTIALPLAYBACK,
        bPartialPlaybackEnabled) == HXR_OK)
    {
        bPartialPlaybackEnabled = !bPartialPlaybackEnabled;
    }
    
    if(bPartialPlaybackEnabled)
    {
        // Read the min number of valid renderer reqd for playback
        ReadPrefUINT32(m_pPlayer->m_pPreferences, MIN_VALID_RENDERER_REQD,
            ulMinValidRenderersReqd);
    }
    
#endif // End of #ifdef HELIX_FEATURE_PARTIALPLAYBACK
    bSourceInitialized = FALSE;

    if (m_pPlayer->m_pEngine->AtInterruptTime()) //for Mac
    {
	return HXR_OK;
    }

    if (m_pSource->GetLastError() != HXR_OK)
    {
	/* Since the source has an error, mark plugin load attempt
	 * to TRUE so that the player object can request upgrade
	 * if it needs to
	 */
	m_bLoadPluginAttempted = TRUE;
	return HXR_OK;
    }

    if (!m_pSource->IsInitialized())
    {
	// atleast one source is not yet initialized
	return HXR_OK;
    }

    bSourceInitialized = TRUE;

#if defined(HELIX_FEATURE_NESTEDMETA)
    IHXPersistentComponent* pPersistentComponent = NULL;

    if (HXR_OK == m_pPlayer->m_pPersistentComponentManager->GetPersistentComponent(m_ulPersistentComponentID,
										   pPersistentComponent))
    {
	// no need to AddRef() since it's maintained by PersistentManager
	m_pRendererAdviseSink = ((HXPersistentComponent*)pPersistentComponent)->m_pRendererAdviseSink;
	m_pRendererAdviseSink->AddRef();
    }

    HX_RELEASE(pPersistentComponent);
#endif /* HELIX_FEATURE_NESTEDMETA */

    theErr = SetupStreams();

    if (m_bIndefiniteDuration	||
	m_pSource->IsLive()	|| 
	m_pPlayer->IsLive())
    {
	m_pPlayer->m_bIsLive = TRUE;
	
	m_pPlayer->m_pAudioPlayer->SetLive(m_pPlayer->m_bIsLive);

	if (m_pSource->isRestrictedLiveStream())
	{
	    m_pPlayer->SetPresentationTime(HX_MAX(m_pPlayer->m_ulPresentationDuration, GetActiveDuration()));
	}
	else
	{
	    m_pPlayer->SetPresentationTime(0);
	}
    }
    else
    {
	m_pPlayer->SetPresentationTime(HX_MAX(m_pPlayer->m_ulPresentationDuration, GetActiveDuration()));
    }

    // attemp to load all the plugins
    m_bLoadPluginAttempted = TRUE;

    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (UINT16 i = 0; ndxRend != m_pRendererMap->End(); ++ndxRend, i++)
    {
	RendererInfo*	pRendInfo = (RendererInfo*)(*ndxRend);

	pStreamInfo	= pRendInfo->m_pStreamInfo;
	pHeader         = pStreamInfo->m_pHeader;
	pStream         = pRendInfo->m_pStream;
	
	HX_ASSERT(pHeader);
	HX_ASSERT(pStream);

	pRenderer       = NULL;
	pUnkRenderer    = NULL;
	bAddDefaultUpgrade  = FALSE;
	HX_RELEASE(pMimeTypeBuffer);

	pHeader->GetPropertyCString("MimeType", pMimeTypeBuffer);

	HX_ASSERT(pMimeTypeBuffer && pMimeTypeBuffer->GetBuffer());
	if (!pMimeTypeBuffer || !pMimeTypeBuffer->GetBuffer())
	{
	    GOTOEXITONERROR(theErr = HXR_NOT_INITIALIZED, exit);
	}

	/* 
	 * Check for "CanBeStoppedAnyTime" property. For streams with this 
	 * property, we do not have to wait to receive a 
	 * packet with timestamp >= lastPacketTime. 
	 * We can instead use the highest timestamp received across
	 * ALL the streams in a given source 
	 */
	UINT32 ulCanBeStoppedAnyTime = 0;
	if (pHeader->GetPropertyULONG32("CanBeStoppedAnyTime", ulCanBeStoppedAnyTime) == HXR_OK)	    
	{   
	    pStreamInfo->m_bCanBeStoppedAnyTime = (ulCanBeStoppedAnyTime == 1);
	}
	/* temporary hack till we fix rmff fileformat to correctly set this
	 * property AND the servers areupdated to have this fixed fileformat.
	 */
	else if (::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "application/x-pn-realevent") == 0 ||
		 ::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "syncMM/x-pn-realvideo") == 0)
	{
	    pStreamInfo->m_bCanBeStoppedAnyTime = TRUE;
	}
	else if (::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "application/vnd.rn-objectsstream") == 0 ||
                ::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "application/x-rn-objects") == 0 ||
		 ::strcasecmp((const char*) pMimeTypeBuffer->GetBuffer(), "application/vnd.rn-objects") == 0)
	{
	    m_pPlayer->m_pEngine->m_lROBActive++;
	}

	IHXPluginHandler3* pPlugin2Handler3 = NULL;

	if (m_pSource->GetRenderingDisabledMask() == HXRNDR_DISABLED_NONE)
	{
	    HX_VERIFY(HXR_OK ==
		m_pPlayer->m_pPlugin2Handler->QueryInterface(IID_IHXPluginHandler3, (void**) &pPlugin2Handler3));

	    if (!(HXR_OK == pPlugin2Handler3->FindGroupOfPluginsUsingStrings(PLUGIN_CLASS, PLUGIN_RENDERER_TYPE, 
		PLUGIN_RENDERER_MIME, (char*)pMimeTypeBuffer->GetBuffer(), NULL, NULL, pRendInfo->m_pRendererEnumerator) &&
		pRendInfo->m_pRendererEnumerator &&
		(HXR_OK == pRendInfo->m_pRendererEnumerator->GetNextPlugin(pUnkRenderer, NULL)) &&
		pUnkRenderer))
	    {
#if defined(HELIX_FEATURE_AUTOUPGRADE)
		if(m_pPlayer->m_pUpgradeCollection)
		    m_pPlayer->m_pUpgradeCollection->Add(eUT_Required, pMimeTypeBuffer, 0, 0);
#endif /* HELIX_FEATURE_AUTOUPGRADE */

		theErr = HXR_NO_RENDERER;
	    }

	    HX_RELEASE(pPlugin2Handler3);

	    GOTOEXITONERROR(theErr, nextrend);

tryNextRendererForSameMimeType:
	    HX_ASSERT(pUnkRenderer);
	    pUnkRenderer->QueryInterface(IID_IHXRenderer, (void**) &pRenderer);
	    theErr = pStream->SetRenderer(pUnkRenderer);

	    GOTOEXITONERROR(theErr, nextrend);
	    HX_RELEASE(pUnkRenderer);
	}

        // now get the TAC info for this track/source
        UINT32 sourceID;
	sourceID	    = 0;
        if (HXR_OK == m_pSource->GetID(sourceID))
        {
            m_pPlayer->CheckTrackAndSourceOnTrackStarted(m_uGroupID, m_uTrackID, sourceID);
        }

	if (!pStreamInfo->m_pStreamProps)
	{
	    // Create an IHXValues for storing stream properties related
	    // to metafile initilization and layout hookup.
	    CreateValuesCCF(pStreamInfo->m_pStreamProps, (IUnknown*)(IHXPlayer*)m_pPlayer);
	}

	// If the stream doesn't have a playto property, than
	// check if the parent source has a playto property
	if ((HXR_OK != pStreamInfo->m_pStreamProps->GetPropertyCString("playto",pStreamPlayTo)) ||
	    !pStreamPlayTo)
	{
	    /* Set region name to playto property */
	    pStreamInfo->m_pStreamProps->GetPropertyCString("region",pStreamPlayTo);
	    
	    /* create a unique value */
	    if (!pStreamPlayTo)
	    {
		char szBuffer[14]; /* Flawfinder: ignore */
		ULONG32 length = SafeSprintf(szBuffer,14, "%#010lx",(ULONG32)(void*)pStreamInfo); /* Flawfinder: ignore */

		CreateAndSetBufferCCF(pStreamPlayTo, (UCHAR*)szBuffer,
				      length+1, (IUnknown*)(IHXPlayer*)m_pPlayer);
	    }

	    pStreamInfo->m_pStreamProps->SetPropertyCString("playto",pStreamPlayTo);
	}

	HX_RELEASE(pStreamPlayTo);

	theErr = SetupRenderer(pRendInfo, pRenderer, pStreamInfo, pStream);

nextrend:
	if (theErr)
	{
	    if (theErr == HXR_OUTOFMEMORY)
	    {
		m_pPlayer->Report( HXLOG_ERR, theErr, 0, NULL, NULL );
	    goto exit;
	    }
#if defined(HELIX_FEATURE_AUTOUPGRADE)
        // Enable handling of auto upgrade only if feature is enabled.
        if (theErr == HXR_REQUEST_UPGRADE)
        {
            bAddDefaultUpgrade = TRUE;
        }
#endif
	    HX_RELEASE(pUnkRenderer);
	    if (pRendInfo->m_pRendererEnumerator &&
		HXR_OK == pRendInfo->m_pRendererEnumerator->GetNextPlugin(pUnkRenderer, NULL) &&
		pUnkRenderer)
	    {
		theErr = HXR_OK;
		HX_RELEASE(pRendInfo->m_pRenderer);
		goto tryNextRendererForSameMimeType;
	    }
	    else
	    {
		if (!theFinalErr)
		{
		    bSourceInitialized = FALSE;
		    theFinalErr = theErr;
		}	    

		// merge any upgrade requests for this source to the player
		m_pSource->MergeUpgradeRequest(bAddDefaultUpgrade, pMimeTypeBuffer ? (char*) pMimeTypeBuffer->GetBuffer() : (char*)NULL);
		theErr = HXR_OK;
#ifdef HELIX_FEATURE_PARTIALPLAYBACK
                if( (bPartialPlaybackEnabled) && (!bAddDefaultUpgrade) )
                {
                    // create the NULL renderer.
                    NullRenderer *pNULLRenderer = new NullRenderer();
                    if(pNULLRenderer)
                    {
                        ulNumNULLRenderer++;
                        theErr = HXR_OK;
                        HX_RELEASE(pRendInfo->m_pRenderer);
                        HX_RELEASE(pUnkRenderer);
                        pNULLRenderer->QueryInterface(IID_IHXRenderer, (void**) &pUnkRenderer);
                        goto tryNextRendererForSameMimeType;
                    }
                    else
                    {
                        theFinalErr = HXR_OUTOFMEMORY;
                    }
                } // End of if(bPartialPlaybackEnabled)
#endif
            }
        } // End of if (theErr)
    } // End of for (UINT16 i = 0; ndxRend != m_pRendererMap->End(); ++ndxRend, i++)
    
exit:
#ifdef HELIX_FEATURE_PARTIALPLAYBACK 
    if(bPartialPlaybackEnabled)
    {
        // Partial playback is enabled. 
        // check valid renderer count against min value required.
        if((theFinalErr || theErr) && (ulNumNULLRenderer > 0))
        {
            UINT32 ulValidRenderers = m_pSource->GetNumStreams() - ulNumNULLRenderer;
            if(ulValidRenderers >= ulMinValidRenderersReqd && 
                theErr != HXR_OUTOFMEMORY && 
                theFinalErr != HXR_OUTOFMEMORY)
            {
                // reset the error and mark that partial playback is happening.
                bSourceInitialized          = TRUE;
                theErr                      = HXR_OK;
                theFinalErr                 = HXR_OK;
                m_bIsPartialPlayback = TRUE;
            }
        } // End of if(theFinalErr || theErr)
    } // if(bPartialPlaybackEnabled)
    
#endif
    if (theErr && !theFinalErr)
    {
	bSourceInitialized = FALSE;
	theFinalErr = theErr;
    }

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    if(!theFinalErr && 
       m_bTrackStartedToBeSent)
    {    
	m_pPlayer->m_pGroupManager->TrackStarted(m_uGroupID, m_uTrackID);  
	m_bTrackStartedToBeSent = FALSE;
	if (m_pPeerSourceInfo)
	{
	    m_pPeerSourceInfo->m_bTrackStartedToBeSent = FALSE;
	}
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */

    HX_RELEASE(pMimeTypeBuffer);
    HX_RELEASE(pUnkRenderer);

    if (!theFinalErr)
    {
	theFinalErr = InitializeRenderersExt(bSourceInitialized);
    }

    if (!theFinalErr && bSourceInitialized)
    {
	m_bInitialized = TRUE;
	  
	// renderers initialized...clear any pending upgrade requests for this source!
	m_pSource->ClearUpgradeRequest();

	if (m_bAudioDeviceReflushHint)
	{
	    m_pSource->SetAudioDeviceReflushHint();
	}

	// set the soundLevel != 100 by default
	if (m_uSoundLevel != 100 || m_pSource->GetSoundLevelOffset() != 0)
	{
	    m_pSource->SetSoundLevel(m_uSoundLevel, FALSE);
	}

	/* Enter the order of the stream numbers in which they will be 
	 * scheduled for GetEvent() calls
	 */
	if (NULL == m_pCurrentScheduleList)
	{
	    m_pCurrentScheduleList = new CHXSimpleList;
	}

	CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
	for (; ndxRend != m_pRendererMap->End(); ++ndxRend)
	{
	    RendererInfo* pRendInfo = (RendererInfo*)(*ndxRend);
	    m_pCurrentScheduleList->AddTail((void*) pRendInfo);
	}
    }

    return theFinalErr;
}

HX_RESULT
SourceInfo::InitializeRenderersExt(HXBOOL& bSourceInitialized)
{
    return HXR_OK;
}

HX_RESULT
SourceInfo::SetupStreams()
{
    if (m_bAreStreamsSetup)
    {
	return HXR_OK;
    }

    HX_RESULT	    theErr          = HXR_OK;
    IHXValues*	    pHeader         = NULL;
    HXStream*	    pStream         = NULL;
    STREAM_INFO*    pStreamInfo     = NULL;
    RendererInfo*   pRendInfo       = NULL;
    HXSource*	    pSource	    = m_pSource;
    IHXBuffer*      pMimeType       = NULL;

    if (!pSource->IsInitialized())
    {
	HX_ASSERT(FALSE);
	return HXR_UNEXPECTED;
    }

    UINT16 uNumStreams = pSource->GetNumStreams();

    HX_ASSERT(m_pRendererMap->IsEmpty() == TRUE);
    if (m_pRendererMap->IsEmpty() && uNumStreams > 0 &&
	(UINT32) uNumStreams < m_pRendererMap->GetHashTableSize())
    {
	m_pRendererMap->InitHashTable((UINT32) uNumStreams);
    }
    
    for (UINT16 i=0; i < uNumStreams; i++)
    {
	UINT32	ulStreamNumber = 0;
	pHeader         = NULL;
	pStream         = NULL;

	pSource->GetStreamHeaderInfo(i, pHeader);

	if (!pHeader)
	{
	    GOTOEXITONERROR(theErr = HXR_FAILED, exit);	    
	}

	pStream = new HXStream;
	if (!pStream)
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto exit;
	}

	pStream->AddRef();
	theErr = pStream->Init(m_pPlayer, pSource, pHeader);
	
	GOTOEXITONERROR(theErr, exit);

	pHeader->GetPropertyULONG32("StreamNumber", ulStreamNumber);

	if (HXR_OK != pSource->GetStreamInfo(ulStreamNumber, pStreamInfo) || !pStreamInfo)
	{
	    GOTOEXITONERROR(theErr = HXR_FAILED, exit);
	}

	HX_RELEASE(pHeader);

	pRendInfo = NewRendererInfo();
	if (!pRendInfo)
	{
	    theErr = HXR_OUTOFMEMORY;
	    GOTOEXITONERROR(theErr, exit);
	}
	// get properties of the header
	pRendInfo->m_pRenderer          = NULL;
	pRendInfo->m_pStreamInfo        = pStreamInfo;

        //Find out if this is an audio renderer or not.
        pRendInfo->m_bIsAudioRenderer = FALSE;
        theErr = pStreamInfo->m_pHeader->GetPropertyCString("MimeType", pMimeType);
        if( SUCCEEDED(theErr) )
        {
            if( !strncasecmp("audio/", (char*)pMimeType->GetBuffer(), 6))
            {
                pRendInfo->m_bIsAudioRenderer = TRUE;
            }
            HX_RELEASE(pMimeType);
        }

	pRendInfo->m_ulGranularity      = 0;
	pRendInfo->m_ulDuration         = pStreamInfo->m_ulDuration;
	pRendInfo->m_BufferingReason    = BUFFERING_START_UP;
        
        struct timeSyncParamStruct obj = {this, pRendInfo};
        pRendInfo->m_pTimeSyncCallback  = new CTimeSyncCallback((void*)&obj, (fGenericCBFunc)TimeSyncCallback);
	pRendInfo->m_pStream		= pStream;
	pRendInfo->m_pStream->AddRef();

	if (m_ulSourceDuration < pStreamInfo->m_ulDuration)
	{
	    m_ulSourceDuration = pStreamInfo->m_ulDuration;
	}
    
	if (pRendInfo->m_pTimeSyncCallback)
	{
	    pRendInfo->m_pTimeSyncCallback->AddRef();
	}
	else
	{
	    theErr = HXR_OUTOFMEMORY;
	    GOTOEXITONERROR(theErr, exit);
	}

	m_pRendererMap->SetAt(pStreamInfo->m_uStreamNumber, (void*) pRendInfo);

	if (pStream)
	{
	    if (pStream->IsSureStream())
	    {
		pSource->m_bSureStreamClip = TRUE;
	    }
	    pSource->AddHXStream(pStream);
	    HX_RELEASE(pStream);
	}
    }

exit:
    HX_RELEASE(pStream);
    HX_RELEASE(pHeader);

    if (!theErr)
    {
	m_bAreStreamsSetup = TRUE;
    }

    return theErr;
}

RendererInfo* 
SourceInfo::NewRendererInfo()
{
    return (new RendererInfo());
}

HX_RESULT
SourceInfo::SetupRenderer(RendererInfo*& pRendInfo, IHXRenderer*& pRenderer, 
			 STREAM_INFO*& pStreamInfo, HXStream*& pStream)
{
    HX_RESULT       theErr              = HXR_OK;
    ULONG32         ulSyncGranularity   = DEFAULT_TIMESYNC_GRANULARITY;
    HXBOOL	    bLiveSource		= FALSE;
    HXSource*      pSource             = m_pSource;
    IHXPlugin*     pPlugin             = NULL;
    const char**    ppTmpMimeType = 0;
    IHXValues* pStrHdr = NULL;
    IHXBuffer* pMimeType = NULL;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    STREAM_INFO*    pSrcStreamInfo	= NULL;
    IHXStatistics*  pStatistics		= NULL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    pRendInfo->m_pRenderer          = pRenderer;

    if (pRenderer)
    {
	if (HXR_OK != pRenderer->QueryInterface(IID_IHXPlugin,(void**)&pPlugin))
	{
	    theErr = HXR_NOT_INITIALIZED;
	}
	else
	{
	    /* Initialize the plugin for use */
	    if (HXR_OK != pPlugin->InitPlugin((IUnknown*) (IHXStreamSource*) m_pSource))
	    {
		theErr = HXR_NOT_INITIALIZED;
	    }
	    pPlugin->Release();
	}

	GOTOEXITONERROR(theErr, exit);

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
	if (HXR_OK == pRenderer->QueryInterface(IID_IHXStatistics, (void**) &pStatistics))
	{
	    if (HXR_OK == pSource->GetStreamInfo(pRendInfo->m_pStreamInfo->m_uStreamNumber, pSrcStreamInfo) &&
		pSrcStreamInfo && pSrcStreamInfo->m_pStats)
	    {
		pStatistics->InitializeStatistics(pSrcStreamInfo->m_pStats->m_pRenderer->m_ulRegistryID);
	    }

	    HX_RELEASE (pStatistics);
	}
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

	theErr = pRenderer->StartStream(pStream, m_pPlayer);
	GOTOEXITONERROR(theErr, exit);

	theErr = pRenderer->OnHeader(pStreamInfo->m_pHeader);

	if ( theErr != HXR_OK )
	{
	    //  Since OnHeader() failed, call EndStream() so the renderer 
	    //  will release its internal members.  This will prevent 
	    //  memory leaks during partial playback cases.
	    pRenderer->EndStream();
	    goto exit;
	}

	// start the stream
	/* get the minimum granularity */
	pRenderer->GetRendererInfo(ppTmpMimeType, ulSyncGranularity);
    }

    /* sanity check */
    if (ulSyncGranularity < MINIMUM_TIMESYNC_GRANULARITY)
    {
	ulSyncGranularity = MINIMUM_TIMESYNC_GRANULARITY;
    }

    if (ulSyncGranularity < m_pPlayer->m_ulLowestGranularity)
    {
	m_pPlayer->m_ulLowestGranularity = ulSyncGranularity;
    }

    pRendInfo->m_ulGranularity = ulSyncGranularity;

    // get properties of the header
    pStreamInfo->m_ulDelay  	    = m_pSource->GetDelay();
    pRendInfo->m_ulDuration         = pStreamInfo->m_ulDuration;
    // OnHeader may have overriddden the m_ulPreroll from the stream header
    // if it was too big or too small or not set.

    pStrHdr = pStreamInfo->m_pHeader;

    // Check to see if this is an audio stream
    if (HXR_OK == pStrHdr->GetPropertyCString("MimeType", pMimeType) &&
        !strncasecmp("audio/", (char*)pMimeType->GetBuffer(), 6))
    {
        IHXAudioPushdown2* pPushdown = NULL;
        UINT32 ulPushdown = 0;

        // Get the audio pushdown
        if (HXR_OK == m_pPlayer->QueryInterface(IID_IHXAudioPushdown2,
                                                (void **)&pPushdown) &&
            HXR_OK == pPushdown->GetAudioPushdown(ulPushdown))
        {
            // Add the pushdown to the post decode delay
            ULONG32 ulPostDecodeDelay = 0;
            pStrHdr->GetPropertyULONG32("PostDecodeDelay", 
                                        ulPostDecodeDelay);
            ulPostDecodeDelay += ulPushdown;
            pStrHdr->SetPropertyULONG32("PostDecodeDelay", 
                                        ulPostDecodeDelay);
        }
        HX_RELEASE(pPushdown);
    }
    HX_RELEASE(pMimeType);

    // Update the buffer state with the new preroll
    pStreamInfo->UpdatePreroll(ClientPrerollHelper::GetPreroll(pStrHdr));

    if (pRenderer)
    {
	// check if renderer is interrupt safe
	IHXInterruptSafe* pInterruptSafe;
	if (HXR_OK == pRenderer->QueryInterface(IID_IHXInterruptSafe,(void**)&pInterruptSafe))
	{
	    HX_ASSERT(pInterruptSafe) ;
	    if (pInterruptSafe)
	    {
		pRendInfo->m_bInterruptSafe = pInterruptSafe->IsInterruptSafe();
		pInterruptSafe->Release();
	    }
	}
    
	m_pPlayer->m_bResumeOnlyAtSystemTime |= (!pRendInfo->m_bInterruptSafe);
    }
    
    /* Enter to the begin list so that we can call Begin at the right time */
    m_pPlayer->EnterToBeginList(pRendInfo);

    // notify the persistent renderer(source) who implements
    // IHXRendererAdviseSink to monitor the status of its tracks
    if (m_pRendererAdviseSink && pStream && !m_bIsPersistentSource)
    {
	bLiveSource = pSource->IsLive();

	if (m_bIndefiniteDuration)
	{
	    m_ulTrackDuration = MAX_UINT32;
	}
	else	
	{	
	    m_ulTrackDuration = m_pSource->GetDuration();
	}

	if (!m_bIsTrackDurationSet)
	{	    
	    m_bIsTrackDurationSet = TRUE;

	    m_pRendererAdviseSink->TrackDurationSet(m_uGroupID,
						    m_uTrackID,
						    m_ulTrackDuration,
						    pStreamInfo->m_ulDelay,
						    bLiveSource);
	}
    }	

    if (m_pRendererAdviseSink && pStream)
    {
	IUnknown* pUnk = 0;
	if(HXR_OK == pStream->QueryInterface(IID_IUnknown, (void**)&pUnk))
	{
	    /* 
	     * construct the IHXValues info for the RendererInitialized call
	     */
	    IHXValues* pValues = NULL;
	    if (HXR_OK == CreateValuesCCF(pValues, (IUnknown*)(IHXPlayer*)m_pPlayer))
	    {
		pValues->SetPropertyULONG32("GroupIndex", m_uGroupID);
		pValues->SetPropertyULONG32("TrackIndex", m_uTrackID);
		pValues->SetPropertyULONG32("Delay", pStreamInfo->m_ulDelay);
	    
		/* We should really fix SMIL renderer to look at MAX duration
		* For now, we will pass max duration for all the renderers.
		* The only case where the duration is different is with image maps 
		* in video/audio stream.
		*/
		pValues->SetPropertyULONG32("Duration", m_ulTrackDuration);	    
		pValues->SetPropertyULONG32("LiveSource", bLiveSource);

		if (!m_id.IsEmpty())
		{
    		    IHXBuffer* pBuffer = NULL;
		    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)(const char*)m_id, 
						        m_id.GetLength()+1, (IUnknown*)(IHXPlayer*)m_pPlayer))
		    {
			pValues->SetPropertyCString("id", pBuffer);
			HX_RELEASE(pBuffer);
		    }
		}

		if (pRenderer)
		{
		    m_pRendererAdviseSink->RendererInitialized(pRenderer, pUnk, pValues);
		}

		pValues->Release();
	    }
	    pUnk->Release();
	}
    }

    // XXX HP we need to re-examine how the SMIL renderer's layout site 
    // setup work!!
    if (m_bIsPersistentSource)
    {
	m_pPlayer->m_bSetupLayoutSiteGroup = FALSE;
    }

exit:
    return theErr;
}


HX_RESULT
SourceInfo::InitializeAndSetupRendererSites()
{
    HX_RESULT theErr = HXR_OK;
    HXBOOL bInitialized = FALSE;

    if (HXR_OK != m_lastError)
    {
        return m_lastError;
    }

    if (m_bLocked)
    {
	return HXR_OK;
    }

    m_bLocked = TRUE;
    m_pMutex->Lock();

    theErr = InitializeRenderers(bInitialized);
    if (!theErr && m_bInitialized)
    {
	/* This source may have audio streams */
	m_pSource->SetMinimumPreroll();

#if defined(HELIX_FEATURE_VIDEO)
	/* Set all the renderer sites */
	SetupRendererSites(!m_pPlayer->m_bSetupLayoutSiteGroup);

	/* Did we have to call BeginChangeLayout? */
	if (m_pPlayer->m_pSiteSupplier			&& 
	    !m_pPlayer->m_bBeginChangeLayoutTobeCalled	&&
	    // XXX HP we will not call DoneChangeLayout on persistent source until
	    //	      the actual source has been initialized. this fixed the resizing
	    //	      problem during stop/play on SMIL in RAM
	    //	      the other DoneChangeLayout in HXPlayer won't be called either
	    //	      since we set m_bSetupLayoutSiteGroup=FALSE in SetupRenderer() of
	    //	      the persistent source
	    !m_bIsPersistentSource)
	{
	    /*
	     * At this point all renderers are layed out!
	     */
	    m_pPlayer->m_bBeginChangeLayoutTobeCalled	= TRUE;
	    m_pPlayer->m_pSiteSupplier->DoneChangeLayout();
	}
#endif /* HELIX_FEATURE_VIDEO */

	if (m_pSource->TryResume())
	{
	    m_pPlayer->RegisterSourcesDone();
	}

	Begin();
    }

    if (HXR_OK != theErr && HXR_OK == m_lastError)
    {
        m_lastError = theErr;
    }

    m_pMutex->Unlock();
    m_bLocked = FALSE;

    return theErr;
}

void
SourceInfo::RenderersCleanup()
{
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::RenderersCleanup() Start",
	    m_pPlayer,
	    this);

    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (; ndxRend != m_pRendererMap->End(); ++ndxRend)
    {
	RendererInfo* pRendInfo = (RendererInfo*)(*ndxRend);
	LONG32	      lStreamNumber = ndxRend.get_key();

	if(m_pRendererAdviseSink && pRendInfo->m_pRenderer)
	{
	    IHXValues* pValues = NULL;
	    
	    if (HXR_OK == CreateValuesCCF(pValues, (IUnknown*)(IHXPlayer*)m_pPlayer))
	    {
		pValues->SetPropertyULONG32("GroupIndex", m_uGroupID);
		pValues->SetPropertyULONG32("TrackIndex", m_uTrackID);
		pValues->SetPropertyULONG32("StreamNumber", lStreamNumber);

		if (!m_id.IsEmpty())
		{
    		    IHXBuffer* pBuffer = NULL;
		    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)(const char*)m_id, 
							m_id.GetLength()+1, (IUnknown*)(IHXPlayer*)m_pPlayer))
		    {
			pValues->SetPropertyCString("id", pBuffer);
			HX_RELEASE(pBuffer);
		    }
		}

		m_pRendererAdviseSink->RendererClosed(pRendInfo->m_pRenderer, pValues);
		pValues->Release();
	    }
	}

	// remove all the packets associated with this renderer
	LISTPOSITION lPos = m_pPlayer->m_EventList.GetHeadPosition();
	while(lPos && m_pPlayer->m_EventList.GetCount())
	{	    
	    CHXEvent* pEvent = (CHXEvent*)m_pPlayer->m_EventList.GetAt(lPos);
	    
	    if (pEvent->m_pRendererInfo == pRendInfo)
	    {
		HX_DELETE(pEvent);
		lPos = m_pPlayer->m_EventList.RemoveAt(lPos);
	    }
	    else
	    {
		m_pPlayer->m_EventList.GetNext(lPos);
	    }
	}

	RenderersCleanupExt(pRendInfo);

	HX_RELEASE(pRendInfo->m_pRenderer);
	HX_RELEASE(pRendInfo->m_pRendererEnumerator);
	HX_DELETE(pRendInfo);
    }

    if (m_pCurrentScheduleList)
    {
	m_pCurrentScheduleList->RemoveAll();
    }

    m_pRendererMap->RemoveAll();

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::RenderersCleanup() End",
	    m_pPlayer,
	    this);

    return;
}

void
SourceInfo::RenderersCleanupExt(RendererInfo* pRendInfo)
{
    return;
}

HX_RESULT
SourceInfo::CloseRenderers()
{
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::CloseRenderers() Start",
	    m_pPlayer,
	    this);

    if (m_bActive)
    {
	m_bActive = FALSE;
        if (m_pPlayer->m_uNumSourcesActive > 0)
        {
            m_pPlayer->m_uNumSourcesActive--;
        }
    }

    // persistent source will be closed by the persistent manager
    if (m_bIsPersistentSource)
    {
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        //Move this reg tree to a different place.
        m_pPlayer->UpdatePersistentSrcInfo(this,
                                           m_pPlayer->m_pStats->m_ulRegistryID,
                                           m_uTrackID);
#endif
	HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::CloseRenderers() End",
		m_pPlayer,
		this);

	return HXR_FAIL;
    }

    // This way the source will still be around until the
    // renderers are closed.
    HX_RELEASE(m_pSource);

    RenderersCleanup();

    HX_RELEASE(m_pRendererAdviseSink);

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::CloseRenderers() End",
	    m_pPlayer,
	    this);

    return HXR_OK;
}

HX_RESULT 
SourceInfo::OnTimeSync(ULONG32 ulCurrentTime)
{
    HX_RESULT theErr = HXR_OK;

//{FILE* f1 = ::fopen("d:\\temp\\audio.txt", "a+"); ::fprintf(f1, "SourceInfo::OnTimeSync: %lu\n", ulCurrentTime);::fclose(f1);}

    CHXMapLongToObj::Iterator ndxRend =	m_pRendererMap->Begin();

    for (; !theErr && ndxRend != m_pRendererMap->End(); ++ndxRend)
    {
	RendererInfo* pRendInfo = (RendererInfo*)(*ndxRend);

	/* It is OK to send time sync within +TIME_SYNC_FUDGE_FACTOR */
        HXBOOL bWithinFudge = TRUE;
        if (m_pPlayer->GetVelocity() == HX_PLAYBACK_VELOCITY_NORMAL)
        {
            UINT32 ulFudgeTime  = ulCurrentTime + TIME_SYNC_FUDGE_FACTOR;
            bWithinFudge        = (pRendInfo->m_ulNextDueSyncTime <= ulFudgeTime ? TRUE : FALSE);
        }
	if (!pRendInfo->m_bInitialBeginToBeSent &&
            bWithinFudge &&
	    (m_pSource->IsLive() || ulCurrentTime <= pRendInfo->m_ulDuration ||
	     m_pSource->HasSubordinateLifetime() ||
	    !pRendInfo->m_bDurationTimeSyncSent ||
	     (m_bIsPersistentSource && !m_bActive)))
	{
	    m_pPlayer->m_pScheduler->Remove(
             pRendInfo->m_pTimeSyncCallback->GetPendingCallback());
        
            pRendInfo->m_pTimeSyncCallback->CallbackCanceled();
	    
            theErr = OnTimeSync(pRendInfo, TRUE);

	    if(m_pPlayer->m_bCurrentPresentationClosed)
	    {
		goto exit;
	    }
	}
    }

exit:
    return theErr;
}

HX_RESULT    
SourceInfo::OnTimeSync(RendererInfo* pRendInfo, HXBOOL bUseCurrentTime)
{
    HX_RESULT theErr = HXR_OK;

    HXBOOL bLegalTimeSync = FALSE;
    if (m_bLocked)
    {
	return HXR_OK;
    }

    m_bLocked = TRUE;
    m_pMutex->Lock();

    // no OnTimeSync() when the source(track) is paused
    if (!m_pSource || m_pSource->IsPaused())
    {
	m_pMutex->Unlock();
	m_bLocked = FALSE;
	return HXR_OK;
    }

    if (!m_pPlayer->m_bInitialized)
    {
	m_pMutex->Unlock();
	m_bLocked = FALSE;
	return HXR_NOT_INITIALIZED;
    }

    // We should never generate OnTimeSync to the renderer until we
    // haven't issued an OnBegin previously.
    HX_ASSERT(!pRendInfo->m_bInitialBeginToBeSent);

    m_pPlayer->m_bCurrentPresentationClosed = FALSE;

    /* Do we query for the current time?
     * This routine may be called as a callback (where we want to find
     * out the current time) OR can be called for all DUE renderers
     * whenevr we get a timesync (here we do not want to call getcurrenttime()
     * again)
     *
     * Changed to: (on 09/05/1998 --RA) 
     *
     * Always ask the audio services what the current time is. This is 
     * because we do not make system call in this function anymore to get the 
     * audio device time. Instead, we use realtime to adjust the
     * last audio time returned by the device.
     */

    m_pPlayer->m_ulCurrentPlayTime = 
	m_pPlayer->m_pAudioPlayer->GetCurrentPlayBackTime();

    if (m_pPlayer->m_bIsFirstTimeSync)
    {
	m_pPlayer->m_bIsFirstTimeSync  = FALSE;
	m_pPlayer->m_ulFirstTimeSync   = m_pPlayer->m_ulCurrentPlayTime;
    }

    /*
     * Call ProcessCurrentEvents ONLY if we are here form a scheduled
     * timesync callback. If we are here from HXPlayer::OnTimeSync, we
     * should have already called ProcessCurrentEvents in HXPlayer.
     */
    if (!bUseCurrentTime)
    {
	// Ths process current events is only for processing at non-interrupt time
	// to make sure tha non-interrupt safe renderers get serviced.
        theErr  = m_pPlayer->ProcessCurrentEvents(HX_GET_BETTERTICKCOUNT(), FALSE, FALSE);
    }

    /* Duration check */
    ULONG32 ulCurrentPlayTime = m_pPlayer->m_ulCurrentPlayTime;

//{FILE* f1 = ::fopen("c:\\temp\\sync.txt", "a+"); ::fprintf(f1, "SourceInfo::OnTimeSync: Renderer: %p %lu\n", pRendInfo->m_pRenderer, ulCurrentPlayTime);::fclose(f1);}

    IHXRenderer* lRend         = (IHXRenderer*) pRendInfo->m_pRenderer;
    STREAM_INFO*  pStreamInfo   = pRendInfo->m_pStreamInfo;
    
    if (m_pSource->isRestrictedLiveStream() && 
	ulCurrentPlayTime > pRendInfo->m_ulDuration)
    {
	// mark all the streams done if one of the streams is done
	if (!pStreamInfo->m_bSrcInfoStreamDone)
	{
	    pStreamInfo->m_bSrcInfoStreamDone = TRUE;

	    pStreamInfo->ResetPostEndTimeEventList();

	    if (!pStreamInfo->m_bSrcInfoStreamFillingDone)
	    {
		pStreamInfo->m_bSrcInfoStreamFillingDone = TRUE;
	    }
	}
	
	CheckIfDone();

	goto exit;
    }

    if (m_pSource)
    {
	theErr = m_pSource->OnTimeSync(ulCurrentPlayTime);
    }

    /* if we were in a seek state before, we need to send a postseekmessage
     * before sending timesync...
     */

    /* We should probably not do this since there may be Pre-Seek Packets for
     * some renderers that have not been sent yet since they are not
     * interrup safe.
     */
    /*
    if (pRendInfo->m_BufferingReason == BUFFERING_SEEK || 
	pRendInfo->m_BufferingReason == BUFFERING_LIVE_PAUSE)
    {
        //FXD: need to add DRM flushPackets if re-enable this
	pRendInfo->m_BufferingReason   = BUFFERING_CONGESTION;
	lRend->OnPostSeek(pStreamInfo->m_ulTimeBeforeSeek,
			  pStreamInfo->m_ulTimeAfterSeek);
    }
    */

    /* We only want to send time sync if it is greater than the last time
     * sync sent to the renderer, after properly adjusting for the
     * playback velocity.
     */
//{FILE* f1 = ::fopen("c:\\temp\\sync.txt", "a+"); ::fprintf(f1, "SourceInfo::OnTimeSync: lastsynctime: %lu currentplaytime: %lu\n", pRendInfo->m_ulLastSyncTime, ulCurrentPlayTime );::fclose(f1);}

    bLegalTimeSync = HX_EARLIER_USING_VELOCITY(pRendInfo->m_ulLastSyncTime,
                                               ulCurrentPlayTime,
                                               m_pPlayer->GetVelocity());
    if (pRendInfo->m_bIsFirstCallback || bLegalTimeSync)
    {
        if (m_pPlayer->GetVelocity() == HX_PLAYBACK_VELOCITY_NORMAL && pRendInfo->m_bIsFirstCallback)
        {
            if (m_pSource->m_ulPlaybackStartedTix == 0)
            {
                m_pSource->m_ulPlaybackStartedTix = HX_GET_TICKCOUNT();
            }
        }
        
	/* tell the renderer what their time should be (not the player's time)
	 * taking into account the source's start time, delay, loop count.
	 */
	UINT32 ulRealtime = ulCurrentPlayTime;

	if (m_pSource->IsLive())
	{
	    /* First TimeSync and received */
	    /* First time sync can only be received if OnBegin has already been
	       issued to the renderer and thus rendering started.  This in turn
	       can only happen in case of live stream, if begin time could
	       be determined.  If begin time could be determined, this means that
	       means that the offset between player and renderer time coordinate
	       system (m_ulTimeDiff) is set */
	    if (pRendInfo->m_bIsFirstTimeSync)
	    {
		pRendInfo->m_bIsFirstTimeSync = FALSE;
	    }

	    ulRealtime += pRendInfo->m_ulTimeDiff;
	}

	HXBOOL bAtInterrupt = m_pPlayer->m_pEngine->AtInterruptTime();
	if (!bAtInterrupt || pRendInfo->m_bInterruptSafe)
	{
	    if (!m_bIsPersistentSource	&& 
		!m_pSource->IsLive()	&&
		ulRealtime >= pRendInfo->m_ulDuration)
	    {
		ulRealtime = pRendInfo->m_ulDuration;
	    }

	    if (lRend)
	    {
		theErr = lRend->OnTimeSync(ulRealtime);	    
		if( theErr == HXR_OUTOFMEMORY )
		{
		    m_pPlayer->Report( HXLOG_ERR, theErr, 0, NULL, NULL );
		}
		if( theErr != HXR_OK )
		{
		    goto exit;
		}
	    }

#if defined(HELIX_FEATURE_SMIL_REPEAT)
	    // repeat support:
	    // make sure we called OnTimeSync() on the active source
	    // before we tearing down its peer source
	    if (!bAtInterrupt	    && 
		m_pPeerSourceInfo   &&
		m_pPeerSourceInfo->m_bRepeatPending)
	    {
		CHXSimpleList*  pRepeatList = m_bLeadingSource?m_pRepeatList:m_pPeerSourceInfo->m_pRepeatList;

		HX_ASSERT(pRepeatList);

		RepeatInfo*	    pRepeatInfo = NULL;
		HXSource*	    pPeerSource = m_pPeerSourceInfo->m_pSource;
		LISTPOSITION	    nextPos = 0;

		// take care repeat="indefinite" when max. duration is set
		if (m_bRepeatIndefinite &&
		    m_ulMaxDuration	&&
		    m_pSource->GetDuration() >= m_pSource->m_ulOriginalDelay + m_ulMaxDuration)
		{
		    m_pPeerSourceInfo->Reset();
		}
		// otherwise, prepare for the new repeat
		else
		{
		    if (m_curPosition != pRepeatList->GetTailPosition())
		    {
			nextPos = m_curPosition;
			pRepeatInfo = (RepeatInfo*)pRepeatList->GetAtNext(nextPos);
		    }		
		    else if (m_bRepeatIndefinite)
		    {
			nextPos = (RepeatInfo*)pRepeatList->GetHeadPosition();
			pRepeatInfo = (RepeatInfo*)pRepeatList->GetAt(nextPos);

			UINT32 ulDelayTimeOffset = m_pSource->GetDelay() + m_ulRepeatInterval - pRepeatInfo->ulDelay;
			m_pPeerSourceInfo->m_ulRepeatDelayTimeOffset = ulDelayTimeOffset;
			m_ulRepeatDelayTimeOffset = ulDelayTimeOffset;
		    }

		    if (nextPos && pRepeatInfo)
		    {
			m_pPeerSourceInfo->m_curPosition = nextPos;

			// initialize the new repeat
			m_pPeerSourceInfo->m_uTrackID = pRepeatInfo->uTrackID;
			m_pPeerSourceInfo->Reset();
			pPeerSource->ReSetup();
		    }
		    // done with the repeat
		    else
		    {
			m_pPeerSourceInfo->Reset();
		    }		
		}

                // /Call TrackStarted to let groupSinks know that this
                // track is starting again (repeating).  Set 3rd parameter
                // to TRUE (== is repeating):
                m_pPlayer->m_pGroupManager->TrackStarted(m_uGroupID, m_uTrackID, TRUE);

		m_pPeerSourceInfo->m_bRepeatPending = FALSE;
	    }
#endif /* HELIX_FEATURE_SMIL_REPEAT */

	    if ((UINT32) ulRealtime >= pRendInfo->m_ulDuration && pRendInfo->m_bOnEndOfPacketSent)
	    {
		pRendInfo->m_bDurationTimeSyncSent = TRUE;
	    }
	}
	else 
	{
	    if (!pRendInfo->m_pTimeSyncCallback->GetPendingCallback())
	    {
                pRendInfo->m_pTimeSyncCallback->CallbackScheduled(
                 m_pPlayer->m_pScheduler->RelativeEnter(pRendInfo->m_pTimeSyncCallback, 0));
//{FILE* f1 = ::fopen("c:\\temp\\ts.txt", "a+"); ::fprintf(f1, "%p ADDING IMMEDIATE TimeSyncCallback: Handle: %lu Thread: %lu\n", pRendInfo->m_pTimeSyncCallback, (UINT32) pRendInfo->m_PendingHandle, GetCurrentThreadId());::fclose(f1);}
	    }

	    goto exit;
	}

	if(m_pPlayer->m_bCurrentPresentationClosed)
	{
	    goto exit;
	}

	pRendInfo->m_ulLastSyncTime = ulCurrentPlayTime;
    }

    /* Do we need to send more timesyncs */
    if (m_pSource->IsLive() || !pRendInfo->m_bDurationTimeSyncSent ||
	(m_bIsPersistentSource && !m_bActive))
    {
        // XXXMEH - The following code guarantees time syncs by a certain time,
        // even if the audio device is late. However, when we are
        // operating at non-1X playback, the time syncs are coming
        // from a fake audio timeline, so this shouldn't be needed.
        if (m_pPlayer->GetVelocity() == HX_PLAYBACK_VELOCITY_NORMAL)
        {
	    ULONG32 ulNextCBTime = pRendInfo->m_ulGranularity;

	    /* Do we need to increment our next due time sync value?
	     * This check is needed since a lot of WOM_DONEs may get accumulated
	     * and by the time we end up getting the time sync,
	     * we have already played a bunch of data thereby resulting in a
	     * jump in current audio time.
	     * So we only increment next due time sync if the audio clock is
	     * also moving
	     */
            if (pRendInfo->m_ulNextDueSyncTime  <
                ulCurrentPlayTime + pRendInfo->m_ulGranularity)
            {
                pRendInfo->m_ulNextDueSyncTime  += pRendInfo->m_ulGranularity;
            }

	    /* We wanna reset ourself if its either the first callback OR
	     * somehow we are lagging behind in the timesyncs
	     */
	    if (pRendInfo->m_bIsFirstCallback ||
	        pRendInfo->m_ulNextDueSyncTime < pRendInfo->m_ulLastSyncTime)
	    {
	        pRendInfo->m_bIsFirstCallback = FALSE;
	        HXTimeval lTime = m_pPlayer->m_pScheduler->GetCurrentSchedulerTime();
	        pRendInfo->m_pTimeSyncCBTime->tv_sec = lTime.tv_sec;
	        pRendInfo->m_pTimeSyncCBTime->tv_usec = lTime.tv_usec;
	        pRendInfo->m_ulNextDueSyncTime  = pRendInfo->m_ulLastSyncTime +
					          pRendInfo->m_ulGranularity;

	        /* Next due time sync at integral multiple of granularity */
	        ULONG32 ulTimeDiff = 0;
		if (pRendInfo->m_ulGranularity)
		{
		    ulTimeDiff = pRendInfo->m_ulNextDueSyncTime % pRendInfo->m_ulGranularity;
		}
	        pRendInfo->m_ulNextDueSyncTime -= ulTimeDiff;
	        ulNextCBTime -= ulTimeDiff;
	    }

	    if (pRendInfo->m_ulNextDueSyncTime > pRendInfo->m_ulDuration &&
	        (!m_bIsPersistentSource && m_bActive) && !m_pSource->IsLive())
	    {
	        ULONG32 ulDiff = pRendInfo->m_ulNextDueSyncTime - pRendInfo->m_ulDuration;
	        if (ulNextCBTime > ulDiff)
	        {
		    ulNextCBTime -= ulDiff;
	        }
	        pRendInfo->m_ulNextDueSyncTime = pRendInfo->m_ulDuration;
	    }

	    *(pRendInfo->m_pTimeSyncCBTime) += (int) (ulNextCBTime*1000);

	    if (!pRendInfo->m_pTimeSyncCallback->GetPendingCallback())
	    {
	        UINT32 ulSchedTime = 0;
	        if (pRendInfo->m_ulNextDueSyncTime > ulCurrentPlayTime)
	        {
		    ulSchedTime = pRendInfo->m_ulNextDueSyncTime - ulCurrentPlayTime;
	        }
	        pRendInfo->m_pTimeSyncCallback->CallbackScheduled(
                 m_pPlayer->m_pScheduler->RelativeEnter(pRendInfo->m_pTimeSyncCallback, ulSchedTime));
	    }
        }
        else
        {
            // At non-1x playback, we just need to clear this flag
            pRendInfo->m_bIsFirstCallback = FALSE;
        }
    }

exit:
    m_pMutex->Unlock();
    m_bLocked	= FALSE;
    return theErr;
}

void		
SourceInfo::Resumed()
{
    if (m_bTobeInitializedBeforeBegin)
    {
	m_bTobeInitializedBeforeBegin = FALSE;

	if (m_pPlayer->m_uNumSourceToBeInitializedBeforeBegin > 0)
	{
	    m_pPlayer->m_uNumSourceToBeInitializedBeforeBegin--;
	}
    }
}

void
SourceInfo::UpdateDuration(UINT32 ulDuration)
{
    m_ulTotalTrackDuration = m_ulTrackDuration = m_ulSourceDuration = ulDuration;
    if (m_pPeerSourceInfo)
    {
	m_pPeerSourceInfo->m_ulTotalTrackDuration = m_ulTotalTrackDuration;
    }

    CHXMapLongToObj::Iterator i = m_pRendererMap->Begin();
    for (; i != m_pRendererMap->End(); ++i)
    {
	RendererInfo* pRendInfo = (RendererInfo*)(*i);

	IHXUpdateProperties* pUpdateProperties = NULL;
	if (pRendInfo->m_pRenderer &&
	    HXR_OK == pRendInfo->m_pRenderer->QueryInterface(IID_IHXUpdateProperties,
							     (void**)&pUpdateProperties))
	{
	    IHXValues* pValues = NULL;	    
	    if (HXR_OK == CreateValuesCCF(pValues, (IUnknown*)(IHXPlayer*)m_pPlayer))
	    {
		pValues->SetPropertyULONG32("End", m_pSource->GetEndTime());
		pValues->SetPropertyULONG32("Duration", ulDuration);

		pUpdateProperties->UpdatePlayTimes(pValues);
		HX_RELEASE(pValues);
	    }
	}
	HX_RELEASE(pUpdateProperties);

	pRendInfo->m_ulDuration = ulDuration;
	pRendInfo->m_pStreamInfo->m_ulDuration = ulDuration;
    }

    m_pPlayer->AdjustPresentationTime();
}

void
SourceInfo::UpdateDelay(UINT32 ulDelay)
{
    INT32   lOffset = 0;
    UINT32  ulStartTime = 0;
    UINT32  ulDuration = 0;

    ulStartTime = m_pSource->GetStartTime();
    ulDuration = GetActiveDuration();

    CHXMapLongToObj::Iterator i = m_pRendererMap->Begin();
    for (; i != m_pRendererMap->End(); ++i)
    {
	RendererInfo* pRendInfo = (RendererInfo*)(*i);
	lOffset = ulDelay - pRendInfo->m_pStreamInfo->m_ulDelay;

	// adjust the packet time offset within the core
	LISTPOSITION lPos = m_pPlayer->m_EventList.GetHeadPosition();
	while(lPos)
	{	    
	    CHXEvent* pEvent = (CHXEvent*)m_pPlayer->m_EventList.GetNext(lPos);
	    
	    if (pEvent->m_pRendererInfo == pRendInfo)
	    {
		pEvent->SetTimeOffset(ulStartTime - ulDelay); 
	    }
	}

	// adjust the packet time offset within the renderer
	IHXUpdateProperties* pUpdateProperties = NULL;
	if (pRendInfo->m_pRenderer &&
	    HXR_OK == pRendInfo->m_pRenderer->QueryInterface(IID_IHXUpdateProperties,
							     (void**)&pUpdateProperties))
	{
	    pUpdateProperties->UpdatePacketTimeOffset(lOffset);
	}
	HX_RELEASE(pUpdateProperties);

	pRendInfo->m_pStreamInfo->m_ulDelay = ulDelay;
	pRendInfo->m_ulDuration = ulDuration;
	pRendInfo->m_pStreamInfo->m_ulDuration = ulDuration;
    }

    m_pPlayer->AdjustPresentationTime();
}

#ifdef SEQ_DEPENDENCY
void
SourceInfo::SetDependency(IHXBuffer* pDependency)
{
    char* pTemp = = new char[pDependency->GetSize()+1];
    strcpy(pTemp, pDependency->GetBuffer()); /* Flawfinder: ignore */

    UINT16 i = 0;
    
    m_uNumDependencies = 1;

    /* Find the number of dependencies */
    while(pTemp[i])
    {
	if (pTemp[i] == '.')
	{
	    m_uNumDependencies++;
	}
	i++;
    }

    m_pDependNode = new UINT16[m_uNumDependencies];

    char* pNode = strtok(pTemp, ".\0");

    /* At least one node should be present */
    HX_ASSERT(pNode);

    i = 0;
    while(pNode)
    {
	m_pDependNode[i++] = atoi(pNode);
	pNode = strtok(NULL, ".\0");
    }

    HX_ASSERT(i == m_uNumDependencies);

    HX_VECTOR_DELETE(pTemp);
}
#endif /*SEQ_DEPENDENCY*/

HXBOOL		
SourceInfo::IsRebufferDone(void)
{
    HXBOOL	    bResult = TRUE;
    RendererInfo*   pRendInfo = NULL;

    if (m_pSource && REBUFFER_REQUIRED == m_pSource->m_rebufferStatus)
    {
	CHXMapLongToObj::Iterator i = m_pRendererMap->Begin();
	for (; i != m_pRendererMap->End(); ++i)
	{
	    pRendInfo = (RendererInfo*)(*i);
	    
	    if (!pRendInfo->m_bOnEndOfPacketSent)
	    {
		bResult = FALSE;
		break;
	    }
	}
    }

    return bResult;
}

void
SourceInfo::ReInitializeStats()
{
    if (!m_bInitialized)
    {
	return;
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (UINT16 i = 0; ndxRend != m_pRendererMap->End(); ++ndxRend, i++)
    {
	RendererInfo*	pRendInfo = (RendererInfo*)(*ndxRend);

	if (pRendInfo->m_pRenderer)
	{
	    IHXStatistics* pStatistics	= NULL;
	    STREAM_INFO*    pSrcStreamInfo  = NULL;
	    if (HXR_OK == pRendInfo->m_pRenderer->QueryInterface(IID_IHXStatistics, (void**) &pStatistics))
	    {
		if (HXR_OK == m_pSource->GetStreamInfo(i, pSrcStreamInfo) &&
	    	    pSrcStreamInfo && pSrcStreamInfo->m_pStats &&
		    pSrcStreamInfo->m_pStats->m_pRenderer)
		{
		    pStatistics->InitializeStatistics(pSrcStreamInfo->m_pStats->m_pRenderer->m_ulRegistryID);
		}

		HX_RELEASE (pStatistics);
	    }
	}
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

HX_RESULT		
SourceInfo::HandleRedirectRequest()
{
    if (m_pPlayer)
    {
	return m_pPlayer->HandleRedirectRequest(this);
    }

    return HXR_FAILED;
}

HX_RESULT		
SourceInfo::HandleSDPRequest()
{
    if (m_pPlayer)
    {
	return m_pPlayer->HandleSDPRequest(this);
    }

    return HXR_FAILED;
}

void
SourceInfo::ScheduleProcessCallback()
{
    if (!m_pProcessCallback->GetPendingCallback())
    {
        m_pProcessCallback->CallbackScheduled(
         m_pPlayer->m_pScheduler->RelativeEnter(m_pProcessCallback, 0));
    }
}

void 
SourceInfo::SetLiveSyncStartTime(HXSource* pSource, RendererInfo* pTmpRendInfo, 
				 UINT32 ulLowestTime)
{
    UINT32 ulLiveSyncStartTime = ulLowestTime;
    INT32 lTimeOffset = pSource->GetStartTime() - pTmpRendInfo->m_pStreamInfo->m_ulDelay;

    if (pTmpRendInfo->m_pStreamInfo->m_pHeader)
    {
	// It is OK for ulLiveSyncStartTime to wrap-around.
	// All components must handle 32bit time-stamp wrap-around and starting time-stamp
	// for live stream can be anything.  
	// Only the relative relationships between time-stamps matter which the 
	// wrap around preserves without issues.
	ulLiveSyncStartTime -= lTimeOffset;

	pTmpRendInfo->m_pStreamInfo->m_pHeader->SetPropertyULONG32("LiveSyncStartTime", ulLiveSyncStartTime);
    }

    m_pSource->SetLiveSyncOffset(ulLiveSyncStartTime);
    return;
}

HXBOOL
SourceInfo::AllOtherStreamsHaveEnded(STREAM_INFO* pThisStreamInfo)
{
    HXBOOL bResult = TRUE;
    CHXMapLongToObj::Iterator ndxRend = m_pRendererMap->Begin();
    for (; ndxRend != m_pRendererMap->End(); ++ndxRend)
    {
	RendererInfo* pRendInfo     = (RendererInfo*)(*ndxRend);
	STREAM_INFO*  pStreamInfo   = pRendInfo->m_pStreamInfo;

	/* 
	 * check if any other stream is still active.
	 * do not consider other "CanBeStoppedAnyTime" streams
	 * as active streams
	 */
	if (pStreamInfo != pThisStreamInfo &&
	    !pStreamInfo->m_bCanBeStoppedAnyTime &&
	    !pStreamInfo->m_bSrcInfoStreamDone)
	{
	    bResult = FALSE;
	    break;
	}
    }

    return bResult;
}

HX_RESULT
SourceInfo::AppendRepeatRequest(UINT16 uTrackID, IHXValues* pTrack)
{
#if defined(HELIX_FEATURE_SMIL_REPEAT)
    HX_RESULT	    theErr = HXR_OK;
    HXBOOL	    bLive = FALSE;
    HXBOOL	    bDurationSet = FALSE;
    HXBOOL	    bMaxDurationSet = FALSE;
    UINT32	    ulStart = 0;
    UINT32	    ulEnd = 0;
    UINT32	    ulDelay = 0;
    UINT32	    ulDuration = 0;   
    UINT32	    ulMaxDuration = 0;
    UINT32	    ulRepeatTag = RepeatUnknown;
    UINT32	    ulRepeatInterval = 0;
    UINT32	    ulLastDelay = 0;
    char	    szId[] = "id";
    char	    szStart[] = "Start";
    char	    szEnd[] = "End";
    char	    szDelay[] = "Delay";
    char	    szDuration[] = "Duration";
    char	    szMaxDuration[] = "maxduration";
    char	    szRepeatTag[] = "repeatTag";
    IHXBuffer*	    pBuffer = NULL;
    RepeatInfo*	    pNewRepeatInfo = NULL;

    HX_ASSERT(m_bLeadingSource);    

    if (HXR_OK != pTrack->GetPropertyCString(szId, pBuffer))
    {
	HX_ASSERT(FALSE);
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }

    // repeated sources should share the same start/end timing attributes
    if (HXR_OK == pTrack->GetPropertyULONG32(szStart,ulStart))
    {
	HX_ASSERT(ulStart == m_pSource->GetStartTime());
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szEnd,ulEnd))
    {
	HX_ASSERT(ulEnd == m_pSource->GetEndTime());
    }

    if (HXR_OK != pTrack->GetPropertyULONG32(szDelay,ulDelay))
    {
	HX_ASSERT(FALSE);
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }
    
    ulLastDelay = m_pRepeatList?((RepeatInfo*)m_pRepeatList->GetTail())->ulDelay:m_pSource->GetDelay();
    if (ulDelay <= ulLastDelay)
    {
	goto cleanup;
    }
    
    if (HXR_OK == pTrack->GetPropertyULONG32(szDuration,ulDuration))
    {
	bDurationSet = TRUE;
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szMaxDuration, ulMaxDuration))
    {
	bMaxDurationSet = TRUE;
    }
    
    // skip this repeat if duration == 0
    if ((bDurationSet && 0 == ulDuration) ||
	(bMaxDurationSet && 0 == ulMaxDuration))
    {
	goto durationset;
    }

    if (HXR_OK != pTrack->GetPropertyULONG32(szRepeatTag, ulRepeatTag))
    {
	ulRepeatTag = RepeatUnknown;
    }

    if (!m_pRepeatList)
    {
	m_pRepeatList = new CHXSimpleList();

	pNewRepeatInfo = new RepeatInfo;
	pNewRepeatInfo->ulStart = m_pSource->GetStartTime();
	pNewRepeatInfo->ulEnd = m_pSource->GetEndTime();
	pNewRepeatInfo->ulDelay = m_pSource->GetDelay();
	pNewRepeatInfo->ulDuration = m_pSource->m_ulOriginalDuration;
	pNewRepeatInfo->uTrackID = m_uTrackID;	
	m_curPosition = m_pRepeatList->AddHead(pNewRepeatInfo);
    }

    pNewRepeatInfo = new RepeatInfo;
    pNewRepeatInfo->ulStart = ulStart?ulStart:m_pSource->GetStartTime();
    pNewRepeatInfo->ulEnd = ulEnd?ulEnd:m_pSource->GetEndTime();
    pNewRepeatInfo->ulDelay = ulDelay;
    pNewRepeatInfo->uTrackID = uTrackID;
    // if the duration is set
    if (bDurationSet)
    {
	pNewRepeatInfo->ulDuration = ulDuration;
    }
    // if max. duration is set, make sure its duration <= ulMaxDuration
    else if (bMaxDurationSet && ulMaxDuration < m_pSource->m_ulOriginalDuration)
    {
	pNewRepeatInfo->ulDuration = ulMaxDuration;
    }
    // set to its leading source's original duration by default
    else
    {
	pNewRepeatInfo->ulDuration = m_pSource->m_ulOriginalDuration;
    }

    m_pRepeatList->AddTail(pNewRepeatInfo);

    if (ulRepeatTag == RepeatIndefiniteOnGroup ||
	ulRepeatTag == RepeatIndefiniteOnMe)
    {
	m_bRepeatIndefinite = TRUE;	    
	ulRepeatInterval = ulDelay - ulLastDelay;

	if (ulRepeatInterval > m_ulRepeatInterval)
	{
	    m_ulRepeatInterval = ulRepeatInterval;
	}
	
	// recaculate the max. duration for repeat="indefinite"
	if (bMaxDurationSet)
	{	    
	    m_ulMaxDuration = ulMaxDuration + m_pSource->m_ulOriginalDuration;
	    ulDuration = ulMaxDuration;
	    if (pNewRepeatInfo->ulDuration > ulMaxDuration)
	    {
		pNewRepeatInfo->ulDuration = ulMaxDuration;
	    }
	}
	else
	{
	    if (ulRepeatTag == RepeatIndefiniteOnGroup)
	    {
		ulDuration = m_pSource->m_ulOriginalDuration;
	    }
	    else
	    {
    		ulDuration = MAX_UINT32;
		bLive = TRUE;
	    }
	}

	if (m_pPeerSourceInfo)
	{
	    m_pPeerSourceInfo->m_bRepeatIndefinite = m_bRepeatIndefinite;
	    m_pPeerSourceInfo->m_ulRepeatInterval = m_ulRepeatInterval;
	    m_pPeerSourceInfo->m_ulMaxDuration = m_ulMaxDuration;
	}
    }
    else
    {
	ulDuration = pNewRepeatInfo->ulDuration;
    }

durationset:

    if (m_pRendererAdviseSink)
    {
	m_pRendererAdviseSink->RepeatedTrackDurationSet((const char*)pBuffer->GetBuffer(),
							ulDuration,
							bLive);
    }

cleanup:

    HX_RELEASE(pBuffer);

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_SMIL_REPEAT */
}

HXBOOL		
SourceInfo::IsAdjustSeekNeeded(UINT32 ulSeekTime)
{
    HXBOOL bResult = TRUE;

    // after the repeated sources
    if (ulSeekTime > GetActiveDuration())
    {
	m_pPeerSourceInfo->Reset();
	bResult = FALSE;
    }
    // within the active source
    else if (ulSeekTime >= m_pSource->GetDelay() &&
	     ulSeekTime < m_pSource->GetDuration())
    {
	bResult = FALSE;
    }

    return bResult;
}

SourceInfo*		
SourceInfo::DoAdjustSeek(UINT32 ulSeekTime)
{    
#if defined(HELIX_FEATURE_SMIL_REPEAT)
    HXBOOL	    bRepeatDelayTimeOffsetAdjusted = FALSE;
    UINT32	    ulOriginalDuration = 0;
    UINT32	    ulOriginalDelay = 0;
    UINT32	    ulActiveDuration = 0;
    UINT32	    ulRepeatDelayTimeOffset = 0;
    SourceInfo*	    pResult = NULL;
    RepeatInfo*	    pRepeatInfo = NULL;
    RepeatInfo*	    pRepeatInfoHead = NULL;
    RepeatInfo*	    pRepeatInfoTail = NULL;
    CHXSimpleList*  pRepeatList = NULL;
    LISTPOSITION    pRepeatPos = 0;
   
    pRepeatPos = m_curPosition;
    ulOriginalDuration = m_pSource->m_ulOriginalDuration;
    ulOriginalDelay = m_pSource->m_ulOriginalDelay;
    ulActiveDuration = GetActiveDuration();
    pRepeatList = m_bLeadingSource?m_pRepeatList:m_pPeerSourceInfo->m_pRepeatList;

    // within the active source
    if (ulSeekTime >= m_pSource->GetDelay() &&
	ulSeekTime < m_pSource->GetDuration())
    {
	pResult = this;
	goto cleanup;
    }

    // after the repeated sources(exclude repeat="indefinite")
    if (!m_bRepeatIndefinite && ulSeekTime > ulActiveDuration)
    {
	pResult = this;
	goto cleanup;
    }

    // end the current source
    if (!m_pSource->IsSourceDone())
    {
	m_pSource->SetEndOfClip(TRUE);
    }
    m_bDone = TRUE;

    // seek to the next spawned source
    if (ulSeekTime >= m_pSource->GetDuration() &&
        ulSeekTime < m_pPeerSourceInfo->m_pSource->GetDuration())
    {
	pResult = m_pPeerSourceInfo;
	goto cleanup;
    }

    // seek before the repeated source
    if (ulSeekTime < ulOriginalDelay)
    {
	pResult = this;
	pResult->m_curPosition = pRepeatList->GetHeadPosition();
	
	if (pResult->m_ulRepeatDelayTimeOffset != 0)
	{
	    pResult->m_ulRepeatDelayTimeOffset = 0;
	    pResult->m_pPeerSourceInfo->m_ulRepeatDelayTimeOffset = 0;
	    bRepeatDelayTimeOffsetAdjusted = TRUE;
	}
    }
    else
    {
	// need to figure out the exact iteration on repeat indefinite
	if (m_bRepeatIndefinite)
	{
	    pRepeatInfoHead = (RepeatInfo*)pRepeatList->GetHead();
	    pRepeatInfoTail = (RepeatInfo*)pRepeatList->GetTail();
	    
	    ulRepeatDelayTimeOffset = pRepeatInfoTail->ulDelay + m_ulRepeatInterval - pRepeatInfoHead->ulDelay;
	}

	// ahead of the current source
	if (ulSeekTime < m_pSource->GetDelay())
	{
	    if (m_bRepeatIndefinite)
	    {
		// need to figure out the exact iteration on repeat indefinite
		while (m_ulRepeatDelayTimeOffset + pRepeatInfoHead->ulDelay > ulSeekTime)
		{
		    m_ulRepeatDelayTimeOffset -= ulRepeatDelayTimeOffset;
		    bRepeatDelayTimeOffsetAdjusted = TRUE;
		}

		// start from its tail
		if (bRepeatDelayTimeOffsetAdjusted)
		{
		    pRepeatPos = pRepeatList->GetTailPosition();
		}
	    }

	    pRepeatInfo = (RepeatInfo*)pRepeatList->GetAt(pRepeatPos);
	    while (pRepeatPos != pRepeatList->GetHeadPosition())
	    {
		if (ulSeekTime >= (pRepeatInfo->ulDelay + m_ulRepeatDelayTimeOffset))
		{
		    break;
		}
		pRepeatInfo = (RepeatInfo*)pRepeatList->GetAtPrev(pRepeatPos);
	    }
	}
	else
	{	    	    	    
	    if (m_bRepeatIndefinite)
	    {
		// need to figure out the exact iteration on repeat indefinite
		while (m_ulRepeatDelayTimeOffset + pRepeatInfoTail->ulDelay + ulOriginalDuration <
		       ulSeekTime)
		{
		    m_ulRepeatDelayTimeOffset += ulRepeatDelayTimeOffset;
		    bRepeatDelayTimeOffsetAdjusted = TRUE;
		}

		// start from its head
		if (bRepeatDelayTimeOffsetAdjusted)
		{
		    pRepeatPos = pRepeatList->GetHeadPosition();
		}
	    }

	    pRepeatInfo = (RepeatInfo*)pRepeatList->GetAt(pRepeatPos);
	    while (pRepeatPos != pRepeatList->GetTailPosition())
	    {
		if (ulSeekTime <= (m_ulRepeatDelayTimeOffset + pRepeatInfo->ulDelay + ulOriginalDuration))
		{
		    break;
		}
		pRepeatInfo = (RepeatInfo*)pRepeatList->GetAtNext(pRepeatPos);
	    }
	}

	pResult = m_pPeerSourceInfo;
	pResult->m_curPosition = pRepeatPos;
	pResult->m_bTobeInitializedBeforeBegin = TRUE;
    }

    if (bRepeatDelayTimeOffsetAdjusted)
    {
	pResult->m_ulRepeatDelayTimeOffset = m_ulRepeatDelayTimeOffset;
	pResult->m_pPeerSourceInfo->m_ulRepeatDelayTimeOffset = m_ulRepeatDelayTimeOffset;	
	m_pPlayer->AdjustPresentationTime();
    }

    pResult->Reset();
    pResult->m_pSource->ReSetup();

cleanup:

    if (pRepeatPos == pRepeatList->GetTailPosition() ||
	ulSeekTime > ulActiveDuration)
    {
	pResult->m_pPeerSourceInfo->Reset();
    }

    pResult->m_bRepeatPending = FALSE;
    pResult->m_pPeerSourceInfo->m_bRepeatPending = TRUE;

    if (pResult != this)
    {
	pResult->m_bSeekPending = TRUE;
	pResult->m_ulSeekTime = ulSeekTime;
    }

    return pResult;
#else
    return NULL;
#endif /* HELIX_FEATURE_SMIL_REPEAT */
}

UINT32
SourceInfo::GetActiveDuration()
{
    UINT32	    ulDuration = 0;
    RepeatInfo*	    pRepeatInfo = NULL;
    CHXSimpleList*  pRepeatList = m_bLeadingSource?m_pRepeatList:m_pPeerSourceInfo->m_pRepeatList;

    if (m_ulTotalTrackDuration)
    {
	return m_ulTotalTrackDuration;
    }

    if (pRepeatList)
    {
	pRepeatInfo = (RepeatInfo*)pRepeatList->GetTail();	
	ulDuration = pRepeatInfo->ulDuration + pRepeatInfo->ulDelay + m_ulRepeatDelayTimeOffset;

	if (m_bRepeatIndefinite &&
	    m_ulMaxDuration)
	{
	    ulDuration = m_pSource->m_ulOriginalDelay + m_ulMaxDuration;
	}
    }
    else
    {	
	ulDuration = m_pSource->GetDuration();
    }
    
    return ulDuration;
}

HXBOOL		
SourceInfo::KeepSourceActive(void)
{
    HXBOOL bResult = TRUE;
    CHXSimpleList* pRepeatList = m_bLeadingSource?m_pRepeatList:m_pPeerSourceInfo->m_pRepeatList;
    
    if (pRepeatList)
    {
	// XXX HP re-exam
	// we should keep the repeated source active till the end of its duration
	if (m_bRepeatIndefinite)
	{
	    if (m_ulMaxDuration &&
		m_pSource->GetDuration() >= m_pSource->m_ulOriginalDelay + m_ulMaxDuration)
	    {
		bResult = FALSE;
		goto cleanup;
	    }
	}
	else if (m_curPosition == pRepeatList->GetTailPosition())
	{
	    bResult = FALSE;
	    goto cleanup;
	}
    }
		
    if (m_pPlayer->IsAtSourceMap(this)	&& 
	!m_pSource->IsPaused()		&&
	GetActiveDuration() < m_pPlayer->m_ulCurrentPlayTime)
    {
	bResult = FALSE;
	goto cleanup;
    }

cleanup:

    return bResult;
}

HX_RESULT SourceInfo::AddToAltURLList(IHXBuffer* pURLStr, HXBOOL bDefault)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pURLStr)
    {
        // Set the return value
        retVal = HXR_OUTOFMEMORY;
        // Do we have an AltURL list?
        if (!m_pAltURLList)
        {
            m_pAltURLList = new CHXSimpleList();
        }
        if (m_pAltURLList)
        {
            // Create a AltURLInfo struct
            AltURLInfo* pInfo = new AltURLInfo;
            if (pInfo)
            {
                // Fill in the struct
                pInfo->m_pAltURLStr = pURLStr;
                pInfo->m_bDefault   = bDefault;
                // AddRef the buffer
                pInfo->m_pAltURLStr->AddRef();
                // Add to the list
                m_pAltURLList->AddTail((void*) pInfo);
                // Clear the return value
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

void SourceInfo::ClearAltURLList()
{
    if (m_pAltURLList)
    {
        while (m_pAltURLList->GetCount() > 0)
        {
            AltURLInfo* pInfo = (AltURLInfo*) m_pAltURLList->RemoveHead();
            if (pInfo)
            {
                HX_RELEASE(pInfo->m_pAltURLStr);
            }
            HX_DELETE(pInfo);
        }
        HX_DELETE(m_pAltURLList);
    }
}

HXBOOL SourceInfo::HasAltURL()
{
    return ((m_pAltURLList && m_pAltURLList->GetCount() > 0) ? TRUE : FALSE);
}

HX_RESULT SourceInfo::GetAltURL(IHXBuffer** ppBuf, HXBOOL* pbDefault)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pAltURLList && m_pAltURLList->GetCount() > 0 && ppBuf && pbDefault)
    {
        // Get the head AltURLInfo struct off the list
        AltURLInfo* pInfo = (AltURLInfo*) m_pAltURLList->RemoveHead();
        if (pInfo)
        {
            // Make sure the out parameter is initially NULL
            HX_ASSERT(*ppBuf == NULL);
            // Get the buffer
            *ppBuf = (IHXBuffer*) pInfo->m_pAltURLStr;
            // Get the default flag
            *pbDefault = pInfo->m_bDefault;
            // We don't need to AddRef(), since we are removing
            // from the list and the list had a ref on the buffer
            //
            // Clear the return value
            retVal = HXR_OK;
        }
        // Delete the AltURLInfo struct we just pulled off the list
        HX_DELETE(pInfo);
    }

    return retVal;
}

HX_RESULT SourceInfo::CopyAltURLListToNewSourceInfo(SourceInfo* pSourceInfo)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSourceInfo)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Loop through our alt url list, copying each one
        if (m_pAltURLList)
        {
            LISTPOSITION pos = m_pAltURLList->GetHeadPosition();
            while (pos)
            {
                AltURLInfo* pInfo = (AltURLInfo*) m_pAltURLList->GetNext(pos);
                if (pInfo)
                {
                    pSourceInfo->AddToAltURLList(pInfo->m_pAltURLStr, pInfo->m_bDefault);
                }
            }
        }
    }

    return retVal;
}

void SourceInfo::TimeSyncCallback(void* pParam)
{
    struct timeSyncParamStruct* pObj = (struct timeSyncParamStruct*)pParam;

    if (pObj)
    {
	HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::TimeSyncCallback Start",
		pObj->pSourceInfo ? pObj->pSourceInfo->m_pPlayer : 0,
		pObj->pSourceInfo);
        HX_RESULT theErr = pObj->pSourceInfo->OnTimeSync(pObj->pRenderInfo);
        //{FILE* f1 = ::fopen("c:\\temp\\ts.txt", "a+"); ::fprintf(f1, "%p TimeSyncCallback::Func: Before False\n", this);::fclose(f1);}
        //{FILE* f1 = ::fopen("c:\\temp\\ts.txt", "a+"); ::fprintf(f1, "%p TimeSyncCallback::Func: Before OnTimeSync\n", this);::fclose(f1);}
        //{FILE* f1 = ::fopen("c:\\temp\\ts.txt", "a+"); ::fprintf(f1, "%p TimeSyncCallback::Func: After OnTimeSync\n", this);::fclose(f1);}
        if (theErr)
        {
            pObj->pSourceInfo->m_pPlayer->Report( HXLOG_ERR, theErr, 0, NULL, NULL );
        }
	HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::TimeSyncCallback End",
		pObj->pSourceInfo ? pObj->pSourceInfo->m_pPlayer : 0,
	        pObj->pSourceInfo);
    }
}

void SourceInfo::ProcessCallback(void* pParam)
{
    HX_RESULT theErr = HXR_OK;
    SourceInfo* pObj = (SourceInfo*)pParam;

    if (pObj)
    {
	HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[Source=%p]::ProcessCallback Start",
		pObj->m_pPlayer,
		pObj->m_pSource);

	if (pObj->m_pSource)
	{
	    theErr = pObj->m_pSource->ProcessIdle();
	}

	if (!theErr && 
            pObj->m_pPlayer && 
            pObj->m_pPlayer->IsInitialized() && 
            !pObj->IsInitialized())
	{
	    theErr = pObj->InitializeAndSetupRendererSites();
	}

	HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SourceInfo[%p]::ProcessCallback End",
		pObj->m_pPlayer,
		pObj->m_pSource);
    }
}


HXBOOL SourceInfo::FirstFrameDisplayed()
{
    HXBOOL        bRetVal    = TRUE;
    IHXFrameInfo* pFrameInfo = NULL;
    HX_RESULT     res        = HXR_OK;
    
    if(!m_pSource || !m_pSource->IsInitialized() || !m_bInitialized)
    {
	return FALSE;
    }

    CHXMapLongToObj::Iterator iter = m_pRendererMap->Begin();
    while(iter != m_pRendererMap->End() && bRetVal)
    {
        RendererInfo* pRendInfo = (RendererInfo*) (*iter);
        IHXRenderer*  pRend     = (IHXRenderer*) pRendInfo->m_pRenderer;
        res = pRend->QueryInterface(IID_IHXFrameInfo, (void**)&pFrameInfo );
        if( SUCCEEDED(res) && pFrameInfo )
        {
            bRetVal = (bRetVal && pFrameInfo->FirstFrameDisplayed());
        }
        HX_RELEASE(pFrameInfo);
        ++iter;
    }

    return bRetVal;
}

HXBOOL SourceInfo::HasSourceRenderingBegun(void)
{
    RendererInfo* pRendInfo;
    CHXMapLongToObj::Iterator ndxRend;

    for (ndxRend = m_pRendererMap->Begin();
	 ndxRend != m_pRendererMap->End(); 
	 ++ndxRend)
    {
	pRendInfo = (RendererInfo*)(*ndxRend);
	if (!pRendInfo->m_bInitialBeginToBeSent)
	{
	    return TRUE;
	}
    }

    return FALSE;
}

