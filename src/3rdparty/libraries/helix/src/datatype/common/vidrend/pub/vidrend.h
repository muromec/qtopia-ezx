/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vidrend.h,v 1.49 2009/02/13 15:34:27 ehyche Exp $
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

#ifndef _VIDREND_H_
#define _VIDREND_H_

/****************************************************************************
 *  Includes
 */
#include "vidrendf.h"
#include "hxplugn.h"
#include "hxrendr.h"
#include "hxwin.h"
#include "hxsite2.h"
#include "hxasm.h"
#include "hxmon.h"
#include "hxpcmkr.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxtick.h"
#include "smoothtime.h"
#include "cringbuf.h"
#include "hxplayvelocity.h"
#include "hxtlogutil.h"

#include "vidstats.h"

/****************************************************************************
 *  CVideoRenderer
 */
class CVideoRenderer : public IHXPlugin, 
		       public IHXRenderer, 
		       public IHXSiteUser,
		       public IHXFrameInfo,
		       public IHXInterruptSafe,			
		       public IHXCallback,
		       public IHXStatistics,
		       public IHXRenderTimeLine,
		       public IHXPaceMakerResponse,
		       public IHXUntimedRenderer,
		       public IHXUpdateProperties,
		       public IHXMediaPushdown,
                       public IHXPlaybackVelocity
{
private:
    class CSetSizeCB : public IHXCallback
    {
    public:
	CSetSizeCB(IHXSite* pSite)
	    : m_lRefCount(0)
	    , m_pSite(pSite)
	{
	    HX_ASSERT(m_pSite);
	    m_pSite->AddRef();

	    m_szViewFrame.cx = 0;
	    m_szViewFrame.cy = 0;
	}

	~CSetSizeCB()
	{
	    HX_RELEASE(m_pSite);
	}

	STDMETHOD(QueryInterface)	(THIS_
					 REFIID riid,
					 void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

	STDMETHOD(Func)			(THIS);

	void SetSize(HXxSize szViewFrame)
	{
	    m_szViewFrame = szViewFrame;
	}

    private:
	LONG32	    m_lRefCount;
	IHXSite*   m_pSite;
	HXxSize	    m_szViewFrame;
    };

    typedef enum 
    { 
	Stopped, 
	Buffering, 
	PlayStarting,
	Playing, 
	Paused, 
	Seeking
    } PlayState;

    static const char* const 	zm_pDescription;
    static const char* const	zm_pCopyright;
    static const char* const 	zm_pMoreInfoURL;

    static const char* const	zm_pStreamMimeTypes[];

    LONG32				m_lRefCount;

    IHXMutex*				m_pMutex;
    IHXMutex*				m_pBltMutex;
    IHXMutex*				m_pVSMutex;
    HXBOOL				m_bVSMutexDisrupted;
    IHXScheduler*			m_pScheduler;
    IHXOptimizedScheduler*		m_pOptimizedScheduler;
    IHXPaceMaker*			m_pDecoderPump;
    IHXPaceMaker*			m_pBltrPump;
    ULONG32				m_ulDecoderPacemakerId;
    ULONG32				m_ulBltrPacemakerId;
    LONG32				m_lDecodePriority;
    CVideoFormat*			m_pDecoderVideoFormat;
    CVideoFormat*			m_pBltrVideoFormat;

#if defined(HELIX_FEATURE_STATS)
    CVideoStatistics*			m_pVideoStats;
#else
    void*                               m_pVideoStats;
#endif /* HELIX_FEATURE_STATS */

    HXBOOL				m_bIsScheduledCB;
    ULONG32				m_ulLateFrameTol;
    ULONG32				m_ulEarlyFrameTol;
    ULONG32				m_ulMaxOptimizedVideoLead;
    ULONG32				m_ulNoFramesPollingInterval;
    ULONG32				m_ulMaxSleepTime;
    ULONG32				m_ulBltPacketQueueSize;
    HXBOOL				m_bSchedulerStartRequested;
    HXBOOL				m_bPendingCallback;
    CallbackHandle			m_hPendingHandle;
    ULONG32				m_ulCallbackCounter;

    HXBOOL				m_bSiteAttached;
    HXBOOL				m_bDecoderRunning;

    PlayState				m_PlayState; 

    ULONG32				m_ulBytesToBuffer;
    ULONG32				m_ulAvgBitRate;
    ULONG32				m_ulPreroll;
    ULONG32				m_ulVideoBufferingPreference;
    ULONG32				m_ulBufferingStartTime;
    ULONG32				m_ulBufferingStopTime;
    ULONG32				m_ulBufferingTimeOut;
    HXBOOL				m_bBufferingOccured;
    HXBOOL				m_bBufferingNeeded;
    HXBOOL				m_bFirstFrame;
    HXBOOL				m_bPreSeekPointFrames;
    HXBOOL				m_bFirstSurfaceUpdate;
    HXBOOL				m_bPendingRedraw;
    HXBOOL				m_bVS1UpdateInProgress;
    HXBOOL				m_bServicingFillbackDecode;

    HXBitmapInfoHeader			m_PreTransformBitmapInfoHeader;
    HXBitmapInfoHeader			m_BitmapInfoHeader;
    HXBitmapInfoHeader*			m_pVSurf2InputBIH;

    CRingBuffer*			m_pBltPacketQueue;
protected:
    HXBOOL				m_bBitmapSet;
    HXBOOL                              m_bViewSizeSetInStreamHeader;  // "Width"/"Height" in stream header
    HXBOOL                              m_bFrameSizeSetInStreamHeader; // "FrameWidth"/"FrameHeight" in stream header
    HXxSize                             m_StreamHeaderViewSize;
    HXxSize                             m_StreamHeaderFrameSize;
private:
    HXBOOL				m_bFrameSizeInitialized;
    HXBOOL				m_bWinSizeFixed;
    HXBOOL				m_bOptimizedBlt;
    HXBOOL				m_bOSGranuleBoost;
    HXBOOL				m_bOSGranuleBoostVS2;
    HXBOOL				m_bTryVideoSurface2;
    HXBOOL				m_bUseVideoSurface2;
    HXBOOL				m_bVideoSurface2Transition;
    HXBOOL				m_bVideoSurface1Requested;
    HXBOOL				m_bVideoSurfaceInitialized;
    HXBOOL				m_bVideoSurfaceReinitRequested;
    HXBOOL				m_bVS2BufferUnavailableOnLastBlt;
    HXBOOL				m_bVSBufferUndisplayed;
    HXBOOL				m_bPresentInProgress;
    HXBOOL				m_bVS2Flushed;
    ULONG32				m_ulHWBufCount;
    ULONG32				m_ulConfigHWBufCount;
    ULONG32				m_ulSyncInterval;
    UINT32				m_ulMaxVidArea;

    CSetSizeCB*				m_pResizeCB;

    void Render(HXBOOL bMarker, BYTE* pData, ULONG32 ulSize, HXBOOL bLate);
    
    HX_RESULT CheckStreamVersions (IHXValues* pHeader);

    HXBOOL _ResizeViewFrame(HXxSize szViewFrame,
			  HXBOOL bMutex,
			  HXBOOL bForceSyncResize,
			  HXBOOL bDefaultResize);

    HX_RESULT UpdateDisplay(HXxEvent* pEvent, 
			    HXBOOL bSystemEvent = FALSE,
			    HXBOOL bIsVisible = TRUE);
protected:    
    HX_RESULT UpdateVideoSurface(IHXVideoSurface* pVideoSurface,
				 CMediaPacket* pVideoPacket,
				 HXxRect &destRect,
				 HXxRect &sorcRect,
				 HXBOOL bOptimizedBlt);
    HX_RESULT UpdateVideoSurface2(IHXVideoSurface* pVideoSurface,
				  HXxRect &destRect,
				  HXxRect &sorcRect,
				  HXBOOL bSystemEvent,
				  HXBOOL& bRefresh);
private:    
    HX_RESULT FlushVideoSurface2(IHXSite* pSite);

    inline HX_RESULT SwitchToVideoSurface1(void);
    inline HX_RESULT SwitchToVideoSurface2(void);

    HXBOOL ForceRefresh(void);
    void ForceDisplayUpdate(HXBOOL bInternalSurfaceUpdateOnly = FALSE,
			    HXBOOL bHasVisibleSurface = TRUE);
    inline HX_RESULT ForceDisplayUpdateOnSite(IHXSite* pSite, 
					      HXBOOL bInternalSurfaceUpdateOnly,
					      HXBOOL bHasVisibleSurface);

    void PresentFrame(void);

    HX_RESULT InitVideoSurface1(HXBOOL bUsedVideoSurface2,
				IHXVideoSurface* pVideoSurface = NULL);
    HX_RESULT InitVideoSurface2(ULONG32 ulWidth, ULONG32 ulHeight);

    HXBOOL CheckVideoAreaSize(HXBOOL bReportErrorMsg);

    HX_RESULT StartSchedulers(void);
    inline HXBOOL ShouldKickStartScheduler();
    HX_RESULT ScheduleCallback(UINT32 ulRelativeTime, 
			       HXBOOL bIsScheduled = FALSE,
			       UINT32 ulBaseTime = 0);
    void SchedulerCallback(HXBOOL bIsScheduled,
			   HXBOOL bResched = TRUE,
			   HXBOOL bIsVS2Call = FALSE,
			   HXBOOL bProcessUndisplayableFramesOnly = FALSE);
    
    inline CallbackHandle ScheduleRelativeCallback(UINT32 ulRelativeTime,
						   IHXCallback* pCallback);
    inline CallbackHandle ScheduleAbsoluteCallback(HXTimeval &hxTime,
						   IHXCallback* pCallback);
    void RemoveCallback(CallbackHandle &hCallback);

    HX_RESULT BeginOptimizedBlt(HXBitmapInfoHeader* pBitmapInfo);
    void EndOptimizedBlt(void);

    inline void RequestBuffering(void);
    inline void RequestBufferingEnd(void);
    HX_RESULT BeginBuffering(void);
    HX_RESULT EndBuffering(void);
    inline HXBOOL IsBufferingComplete(IHXPacket* pPacket = NULL);

    void ReleasePacket(CMediaPacket* pPacket,
		       HXBOOL bForceKill = FALSE);

    void ClearBltPacketQueue(void);

    HXBOOL IsDecoderRunning(void)	    { return m_bDecoderRunning; }

    HX_RESULT OnDecoderStart(void);
    HX_RESULT OnDecoderEnd(void);
    HXBOOL OnDecode(void);

    void DisplayMutex_Lock(void)
    {
	m_pBltMutex->Lock();
	m_pVSMutex->Lock();
    }

    void DisplayMutex_Unlock(void)
    {
	m_pVSMutex->Unlock();
	m_pBltMutex->Unlock();
    }

    void VSMutex_Disrupt(void)
    {
	m_bVSMutexDisrupted = TRUE;
	m_pVSMutex->Unlock();
    }

    void VSMutex_Continue(void)
    {
	m_pVSMutex->Lock();
	m_bVSMutexDisrupted = FALSE;
    }

    HXBOOL VSMutex_IsDisrupted(void)
    {
	return m_bVSMutexDisrupted;
    }

protected:
    CVideoFormat*			m_pVideoFormat;

    HXxSize				m_SetWinSize;
    HXxSize				m_LastSetSize;
    HXxRect*				m_pClipRect;
    HXxRect				m_rViewRect;

public:
    /*
     *	Costructor
     */
    CVideoRenderer(void);

    /*
     *	Destructor
     */
    virtual ~CVideoRenderer();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXPlugin methods
     */
    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the basic information about this plugin. Including:
     *
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)	 /*OUT*/ bLoadMultiple,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber
				);

    /************************************************************************
     *	Method:
     *	    IHXPlugin::InitPlugin
     *	Purpose:
     *	    Initializes the plugin for use. This interface must always be
     *	    called before any other method is called. This is primarily needed 
     *	    so that the plugin can have access to the context for creation of
     *	    IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext);

    
    /*
     *	IHXRenderer methods
     */
    /************************************************************************
     *	Method:
     *	    IHXRenderer::GetRendererInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of rendering 
     *	    plugins.
     */
    STDMETHOD(GetRendererInfo)	(THIS_
				REF(const char**) /*OUT*/ pStreamMimeTypes,
				REF(UINT32)      /*OUT*/ unInitialGranularity
				);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::StartStream
    //	Purpose:
    //	    Called by client engine to inform the renderer of the stream it
    //	    will be rendering. The stream interface can provide access to
    //	    its source or player. This method also provides access to the 
    //	    primary client controller interface.
    //
    STDMETHOD (StartStream)	(THIS_			
				IHXStream*	    pStream,
				IHXPlayer*	    pPlayer);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::EndStream
    //	Purpose:
    //	    Called by client engine to inform the renderer that the stream
    //	    is was rendering is closed.
    //
    STDMETHOD (EndStream)	(THIS);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::OnHeader
    //	Purpose:
    //		Called by client engine when a header for this renderer is 
    //		available. The header will arrive before any packets.
    //
    STDMETHOD (OnHeader)	(THIS_
				IHXValues*	    pHeader);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnPacket
    //	Purpose:
    //	    Called by client engine when a packet for this renderer is 
    //	    due.
    //
    STDMETHOD (OnPacket)	(THIS_
				IHXPacket*	    pPacket,
				LONG32		    lTimeOffset);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnTimeSync
    //	Purpose:
    //	    Called by client engine to inform the renderer of the current
    //	    time relative to the streams synchronized time-line. The 
    //	    renderer should use this time value to update its display or
    //	    render it's stream data accordingly.
    //
    STDMETHOD (OnTimeSync)	(THIS_
				ULONG32		    ulTime);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //	    IHXRenderer::OnPreSeek
    //  Purpose:
    //	    Called by client engine to inform the renderer that a seek is
    //	    about to occur. The render is informed the last time for the 
    //	    stream's time line before the seek, as well as the first new
    //	    time for the stream's time line after the seek will be completed.
    //
    STDMETHOD (OnPreSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnPostSeek
    //	Purpose:
    //	    Called by client engine to inform the renderer that a seek has
    //	    just occured. The render is informed the last time for the 
    //	    stream's time line before the seek, as well as the first new
    //	    time for the stream's time line after the seek.
    //
    STDMETHOD (OnPostSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnPause
    //	Purpose:
    //	    Called by client engine to inform the renderer that a pause has
    //	    just occured. The render is informed the last time for the 
    //	    stream's time line before the pause.
    //
    STDMETHOD (OnPause)		(THIS_
				ULONG32		    ulTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::OnBegin
    //	Purpose:
    //		Called by client engine to inform the renderer that a begin or
    //		resume has just occured. The render is informed the first time 
    //		for the stream's time line after the resume.
    //
    STDMETHOD (OnBegin)		(THIS_
				ULONG32		    ulTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::OnBuffering
    //	Purpose:
    //		Called by client engine to inform the renderer that buffering
    //		of data is occuring. The render is informed of the reason for
    //		the buffering (start-up of stream, seek has occured, network
    //		congestion, etc.), as well as percentage complete of the 
    //		buffering process.
    //
    STDMETHOD (OnBuffering)	(THIS_
				ULONG32		    ulFlags,
				UINT16		    unPercentComplete);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::GetDisplayType
    //	Purpose:
    //		Called by client engine to ask the renderer for it's preferred
    //		display type. When layout information is not present, the 
    //		renderer will be asked for it's prefered display type. Depending
    //		on the display type a buffer of additional information may be 
    //		needed. This buffer could contain information about preferred
    //		window size.
    //
    STDMETHOD (GetDisplayType)	(THIS_
				REF(HX_DISPLAY_TYPE)	ulFlags,
				REF(IHXBuffer*)	pBuffer);

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnEndofPackets
     *	Purpose:
     *	    Called by client engine to inform the renderer that all the
     *	    packets have been delivered. However, if the user seeks before
     *	    EndStream() is called, renderer may start getting packets again
     *	    and the client engine will eventually call this function again.
     */
    STDMETHOD(OnEndofPackets)	(THIS);

    /*
     * IHXSiteUser methods called by the "context" to 
     * associate users with the site.
     */
    STDMETHOD(AttachSite)	(THIS_
				IHXSite*	/*IN*/ pSite);

    STDMETHOD(DetachSite)	(THIS);

    STDMETHOD_(HXBOOL,NeedsWindowedSites)	(THIS);

    STDMETHOD_(HXBOOL, IsInterruptSafe)	(THIS) { return TRUE; }

    /*
     * IHXSiteUser methods called to inform user of an event.
     */
    STDMETHOD(HandleEvent)	(THIS_
				HXxEvent*	/*IN*/ pEvent);

    /*
     * IHXCallback method - backbone of renderer scheduling
     */
    /************************************************************************
     *	Method:
     *	    IHXCallback::Func
     *	Purpose:
     *	    This is the function that will be called when a callback is
     *	    to be executed.
     */
    STDMETHOD(Func)		(THIS);


    /*
     * IHXStatistics methods
     */
    STDMETHOD(InitializeStatistics) (THIS_ UINT32 ulRegistryID);
    STDMETHOD(UpdateStatistics)     (THIS);

    //IHXFrameInfo method.
    STDMETHOD_(HXBOOL,FirstFrameDisplayed)(THIS);

    HXBOOL IsVideoOnlyPlayback();
    void CheckForBufferingStart();
    /*
     * IHXUntimedRenderer methods
     */
    STDMETHOD_(HXBOOL,IsUntimedRendering)(THIS);
    STDMETHOD_(HX_RESULT,SetUntimedRendering)(THIS_ HXBOOL);

    virtual HXBOOL IsUntimedMode() { return m_bUntimedRendering; }
    virtual HX_RESULT UntimedModeNotice(HXBOOL bUntimedRendering) { return HXR_OK; }

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePacketTimeOffset
     *	Purpose:
     *	    Call this method to update the timestamp offset of cached packets
     */
    STDMETHOD(UpdatePacketTimeOffset) (THIS_ INT32 lTimeOffset);

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePlayTimes
     *	Purpose:
     *	    Call this method to update the playtime attributes
     */
    STDMETHOD(UpdatePlayTimes)	      (THIS_
				       IHXValues* pProps);

    /************************************************************************
     *  Method:
     *      IHXRenderTimeLine::GetTimeLineValue
     *  Purpose:
     *      Get the current presentation time
     *
     *  Notes:
     *      returns HXR_TIMELINE_SUSPENDED when the time line is suspended
     */
    STDMETHOD (GetTimeLineValue) (THIS_ /*OUT*/ REF(UINT32) ulTime);


    /************************************************************************
     *  Method:
     *      IHXMediaPushdown::GetCurrentPushdown
     *  Purpose:
     *      Retrieves the current queue depth ("pushdown depth") in milliseconds
     *
     *  Notes:
     *      This is the *decoded* pushdown, not the undecoded pushdown.  Returns
     *      HXR_TIMELINE_SUSPENDED if the stream is paused; HXR_FAIL
     *      if the stream is finished, HXR_OK otherwise.
     */
    STDMETHOD (GetCurrentPushdown) (THIS_ /*OUT*/ 
				    REF(UINT32) ulPushdownMS,
                                    REF(UINT32) ulNumFrames);

    /************************************************************************
     *  Method:
     *      IHXMediaPushdown::IsG2Video
     *  Purpose:
     */
    STDMETHOD_(HXBOOL, IsG2Video) (THIS);

    /*
     *	IHXPaceMakerResponse
     */
    STDMETHOD(OnPaceStart)	(THIS_
				ULONG32 ulId);

    STDMETHOD(OnPaceEnd)	(THIS_
				ULONG32 ulId);

    STDMETHOD(OnPace)		(THIS_
				ULONG32 ulId);

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse);
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps);
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    STDMETHOD_(INT32,GetVelocity)          (THIS);
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode);
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS);
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS) { return 0; }
    STDMETHOD(CloseVelocityControl)        (THIS);

    /************************************************************************
     *	Public CVideoRenderer functions
     */
    virtual void Close(void);

    virtual HXBOOL ResizeViewFrame(HXxSize szViewFrame, HXBOOL bMutex = TRUE);
    
    void BltIfNeeded(void) 
    {
	HXLOGL4(HXLOG_BVID, "CVideoRenderer::BltIfNeeded() Start");
	if (m_bUseVideoSurface2)
	{
	    if (m_PlayState == Playing)
	    {
		if (m_bOSGranuleBoostVS2)
		{
		    ForceDisplayUpdate(TRUE);
		}
	    }
	    else
	    {
		SchedulerCallback(FALSE,    // not scheduled 
		    FALSE,    // do not resched    
		    TRUE,	    // is VS2 call
		    TRUE);    // process non-displayable frames only
		if (m_pBltrPump)
		{
		    m_pBltrPump->Signal();
		}
	    }
	}
	else if (m_hPendingHandle != ((CallbackHandle) NULL))
	{
	    if ((!m_bVS1UpdateInProgress) &&
		(m_bOSGranuleBoost || (m_PlayState != Playing)))
	    {
		SchedulerCallback(m_bIsScheduledCB, FALSE);
	    }
	}
	HXLOGL4(HXLOG_BVID, "CVideoRenderer::BltIfNeeded() End");
    }

    LONG32 ComputeTimeAhead(ULONG32 ulTime, 
                            ULONG32 ulTolerance, 
                            ULONG32* p_ulBaseTime = NULL)
    {
        LONG32 lTimeAhead = ulTime;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
        if (m_lPlaybackVelocity < 0)
        {
            if (m_PlayState == Playing)
            {
                lTimeAhead = (LONG32) (m_pTimeSyncSmoother->GetTimeNow(p_ulBaseTime) + ulTolerance - ulTime);
            }
            else
            {
                lTimeAhead = (LONG32) (m_pTimeSyncSmoother->GetBaseTime() +
                                       m_pTimeSyncSmoother->GetTimelineOffset() - ulTime);
            }
        }
        else
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
        {
            if (m_PlayState == Playing)
            {
                lTimeAhead = (LONG32) (ulTime + ulTolerance -
                                       m_pTimeSyncSmoother->GetTimeNow(p_ulBaseTime));
            }
            else
            {
                lTimeAhead = (LONG32) (ulTime - m_pTimeSyncSmoother->GetBaseTime() -
                                       m_pTimeSyncSmoother->GetTimelineOffset());
            }
        }

        // In untimed mode, we have to present a false timeline so anybody calling this thinks
        //  we are running just a bit ahead of real time. The way we do this is relating the
        //  reference packet time (ulTime) to the current packet time, rather than against real
        //  time.
        if( m_bUntimedRendering )
        {
            return ulTime - (INT32) m_ulActiveVideoTime;
        }
        return lTimeAhead;
    }

    LONG32 DiffTime(ULONG32 t1, ULONG32 t2)  { return ((LONG32) (t1 - t2)); }

    IUnknown* GetContext(void)
    {
	return m_pContext;
    }

    IHXPreferences* GetPreferences(void)
    {
	return m_pPreferences;
    }

    IHXValues* GetHeader(void)
    {
    return m_pHeader;
    }

    const HXBitmapInfoHeader* GetCurrentBitmapInfo(void)
    {
	return &m_BitmapInfoHeader;
    }

    IHXSite* GetAttachedSite(void)
    {
	return m_pMISUSSite;
    }

    HXBOOL IsVideoSurface2Possible(void)
    {
	return m_bTryVideoSurface2;
    }

    void ReleaseFramePacket(CMediaPacket *pPacket);

    HX_RESULT LocalizeActiveVideoPacket(void);

    inline HXBOOL IsActive(void)    { return (m_PlayState == Playing); }
    inline HXBOOL IsSeeking(void)    { return (m_PlayState == Seeking); }

    HX_RESULT SetDecodePriority(LONG32 lPriority);

#if defined(HELIX_FEATURE_STATS)
    /************************************************************************
     *	Public Statistics Methods
     */
    void ReportLostFrame(ULONG32 ulCount = 1)
    {
	m_pVideoStats->ReportLostFrame(ulCount);
    }

    void ReportDroppedFrame(ULONG32 ulCount = 1)  
    {
	m_pVideoStats->ReportDroppedFrame(ulCount);
    }

    void ReportDecodedFrame(ULONG32 ulCount = 1)  
    { 
	m_pVideoStats->ReportDecodedFrame(ulCount);
    }

    void ReportSampledFrame(ULONG32 ulCount = 1)  
    { 
	m_pVideoStats->ReportSampledFrame(ulCount);
    }

    void ReportFailedBlt(ULONG32 ulCount = 1)  
    {
	m_pVideoStats->ReportFailedBlt(ulCount);
    }

    HX_RESULT ReportStat(VideoStatEntryID eEntryID, const char* pVal)
    {
	return m_pVideoStats->ReportStat(eEntryID, pVal);
    }
    
    HX_RESULT ReportStat(VideoStatEntryID eEntryID, INT32 lVal)
    {
	return m_pVideoStats->ReportStat(eEntryID, lVal);
    }
#else	// HELIX_FEATURE_STATS
    /************************************************************************
     *	Public Statistics Methods
     */
    void ReportLostFrame(ULONG32 ulCount = 1)
    {
	return;
    }

    void ReportDroppedFrame(ULONG32 ulCount = 1)  
    {
	return;
    }

    void ReportDecodedFrame(ULONG32 ulCount = 1)  
    { 
        return;
    }

    void ReportUpsampledFrame(ULONG32 ulCount = 1)  
    {
	return;
    }

    void ReportSampledFrame(ULONG32 ulCount = 1)  
    {
	return;
    }

    void ReportFailedBlt(ULONG32 ulCount = 1)  
    {
	return;
    }

    HX_RESULT ReportStat(VideoStatEntryID eEntryID, const char* pVal)
    {
	return HXR_OK;
    }
    
    HX_RESULT ReportStat(VideoStatEntryID eEntryID, INT32 lVal)
    {
	return HXR_OK;
    }
#endif	// HELIX_FEATURE_STATS

    HX_RESULT SendKeyFrameModeRequest();
    HXBOOL    HasSentKeyFrameModeRequest() { return m_bSentKeyFrameModeRequest; }
    HX_RESULT SetClipRect(INT32 lLeft, INT32 lTop, INT32 lRight, INT32 lBottom);
    void      ClearClipRect();
    HXBOOL    IsDisplaySizeFixed() { return m_bWinSizeFixed; }
    HXxSize   GetViewSize() { return m_SetWinSize; }
    void      SignalDecoderThread();
protected:

    /*
     *	Renderer's customizable fuctions - can be called any time
     */
    /*
     *	Fixed Renderer Configuration - must be callable at any time
     */
    virtual const char* GetUpgradeMimeType(void);

    virtual const char* GetRendererName(void);

    virtual const char* GetCodecName(void);

    virtual const char* GetCodecFourCC(void);

    virtual HXBOOL      ShouldDecodeOnSeparateThread() { return TRUE; }
    virtual HXBOOL      CanUseOptimizedScheduler()     { return TRUE; }


    /*
     *	Renderer Configuration - must be callable any time
     *                           after reception of the header
     */
    virtual void GetStreamVersion(ULONG32 &ulThisMajorVersion, 
				  ULONG32 &ulThisMinorVersion);

    virtual void GetContentVersion(ULONG32 &ulThisMajorVersion, 
				   ULONG32 &ulThisMinorVersion);

    virtual ULONG32 GetLateFrameTolerance(void);

    virtual ULONG32 GetEarlyFrameTolerance(void);

    virtual ULONG32 GetMaxOptimizedVideoLead(void);

    virtual ULONG32 GetBltPacketQueueSize(void);

    virtual ULONG32 GetSyncGoalSmoothingDepth(void);

    virtual ULONG32 GetSpeedupGoalSmoothingDepth(void);

    virtual ULONG32 GetNoFramesPollingInterval(void);

    virtual ULONG32 GetMaxSleepTime(void);

    virtual ULONG32 GetMaxBadSeqSamples(void);

    virtual LONG32 GetDecodePriority(void);

    virtual ULONG32 GetLateFrameRebufferingTolerance();

    virtual CVideoFormat* CreateFormatObject(IHXValues* pHeader);

    
    /*
     *	Renderer Setup - must be callable after the renderer attaches the site
     */
    virtual void SetupBitmapDefaults(IHXValues* pHeader,
				     HXBitmapInfoHeader &bitmapInfoHeader);

    virtual void FormatAndSetViewFrame(HXxRect* pClipRect,
				       HXBitmapInfoHeader &bitmapInfoHeader,
				       HXxRect &rViewRect,
				       HXBOOL bMutex = TRUE,
				       HXBOOL bForceSyncResize = TRUE,
                                       HXBOOL bAsDefault = TRUE);

    /*
     *	Renderer Execution - must be callable anytime
     */
    virtual HX_RESULT HandleUserEvent(HXxEvent* pEvent,
				      HXBOOL& bUpdateDisplay);

    virtual HX_RESULT OnOptimizedVideo(HX_RESULT status,
				       const HXBitmapInfoHeader& sourceBIH,
				       HXBitmapInfoHeader &targetBIH, 
				       ULONG32 &ulTargetBufCount);

    virtual void OffOptimizedVideo(void);

    virtual HX_RESULT TransferOptimizedVideo(IHXVideoSurface2* pVideoSurface2,
                                             VideoMemStruct* pVideoMemoryInfo,
                                             CMediaPacket* pVideoPacket,
                                             const HXBitmapInfoHeader& sorcBIH,
                                             HXxRect &destRect,
                                             HXxRect &sorcRect,
					     SourceInputStruct* pVideoLayout = NULL);

    /*
     *	InitVideoTransform:
     *	    postTransformBIH	- bitmap image header indicating format after
     *				  transform (out)
     *	    preTransformBIH	- bitmap image header indicating format prior
     *				  to transform (in)
     */	
    virtual HX_RESULT InitVideoTransform(HXBitmapInfoHeader& postTransformBIH,
					 const HXBitmapInfoHeader& preTransformBIH);

    /*
     *	TransformVideo:
     *	    pActiveVideoPacket	- Videopacket to transform (in/out/replacable)
     *	    destRect		- region where to post tranform image
     *				  will be mapped onto video surface (in/out)
     *	    sorcRect		- region where from post tranform image
     *				  will be mapped onto video surface (in/out)
     *	    bNewFrameBlt	- indicates of post transform image is to be 
     *				  treated as a new frame (in/out)
     *	    preTransformBIH	- pre-transform bitmap image header (in)
     *	    postTransformBIH	- post-transform bitmap image header (in)
     *	    bOptimizedVideo	- indicates if optimized video is in use
     *				  for passed in video packet (in)
     */	
    virtual HX_RESULT TransformVideo(CMediaPacket* &pActiveVideoPacket,
				     HXxRect &destRect, 
				     HXxRect &sorcRect,
				     HXBOOL &bNewFrameBlt,
				     const HXBitmapInfoHeader& preTransformBIH,
				     const HXBitmapInfoHeader& postTransformBIH,	     
				     HXBOOL bOptimizedVideo);

    virtual HX_RESULT InitExtraStats(void);

    /*
     *	Base Renderer Setup - must be called if the base renderer overrides
     &			      the report of the related values to the core
     */
    void SetSyncInterval(ULONG32 ulSyncInterval);

    // Central method where we change the player state
    void ChangePlayState(PlayState eState);
    void SetPlayStateStopped();

    /*
     *	Renderer's member variables sharable with the derived renderer
     */
    IUnknown*				m_pContext;
    IHXStream*				m_pStream;
    IHXValues*				m_pHeader;
    
    IHXBackChannel*			m_pBackChannel;
    IHXMultiInstanceSiteUserSupplier*	m_pMISUS;
    IHXSite*				m_pMISUSSite;
    IHXCommonClassFactory*		m_pCommonClassFactory;
    IHXPreferences*			m_pPreferences;
    IHXRegistry*			m_pRegistry;
    ULONG32				m_ulRegistryID;

    CMediaPacket*			m_pActiveVideoPacket;
    ULONG32				m_ulActiveVideoTime;
    HXBOOL				m_bUntimedRendering;
    HXBOOL				m_bActiveVideoPacketLocalized;
    CHXTimeSyncSmoother*                m_pTimeSyncSmoother;
    INT32                               m_lPlaybackVelocity;
    UINT32				m_ulAbsPlaybackVelocity;
    HXBOOL                              m_bKeyFrameMode;
    HXBOOL                              m_bAutoSwitch;
    HXBOOL                              m_bSentKeyFrameModeRequest;
};

#endif // _VIDREND_H_

