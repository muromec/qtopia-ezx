/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vidrend.cpp,v 1.100 2009/02/13 15:34:26 ehyche Exp $
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
 *  Debug Settings
 */
// #define ENABLE_TRACE
// #define ENABLE_SYNC_TRACE
// #define ENABLE_SCHED_TRACE
// #define ENABLE_FETCH_TRACE
// #define ENABLE_INPUT_TRACE


/****************************************************************************
 *  Operational Compile-time Settings
 */
// #define SYNC_RESIZE_OK
// #define SYNC_VS_SWITCHING
// #define REBUFFER_ON_VIDEO
#define SET_NONZERO_VIEWFRAME_ONLY
#define RESIZE_AFTER_SITE_ATTACHED
#define DO_ABSOLUTE_TIMING
#define DEFAULT_VS2_TARGETRECT

#define UNBOUND_VIDEO_AREA  0xFFFFFFFF

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
#define OSGRANULE_BOOSTING_ENABLED  FALSE
#else   // _MACINTOSH
#define OSGRANULE_BOOSTING_ENABLED  TRUE
#endif  // _MACINTOSH

/****************************************************************************
 *  Debug Macros
 */
#ifdef ENABLE_TRACE
#define HX_TRACE_THINGY(x, m, l)                                        \
    {                                                   \
        FILE* f1;                                       \
        f1 = ::fopen(x, "a+");                          \
        (f1)?(::fprintf(f1, "%ld - %s = %ld \n", HX_GET_BETTERTICKCOUNT(), m, l), ::fclose(f1)):(0);\
    }
#else   // ENABLE_TRACE
#define HX_TRACE_THINGY(x, m, l)
#endif  // ENABLE_TRACE


/****************************************************************************
 *  Defines
 */
#define BASE_VIDEO_RENDERER_NAME    "Basic Video"
#define NAME_STATS_EXT              ".name"
#define C4CC_STATS_EXT              ".CodecFourCC"

#if defined(HELIX_FEATURE_VIDREND_NO_DEFAULT_WINDOW_SIZE)
#define DEFAULT_WIN_SIZE_X          0
#define DEFAULT_WIN_SIZE_Y          0
#else
#define DEFAULT_WIN_SIZE_X          160
#define DEFAULT_WIN_SIZE_Y          120
#endif

#define SYNC_INTERVAL               20      // in milliseconds
#define EARLY_FRAME_TOL             3       // in milliseconds
#define LATE_FRAME_TOL              30      // in milliseconds
#define NO_FRAMES_POLLING_INTERVAL  20      // in milliseconds
#define MAX_SLEEP_TIME              132     // in milliseconds
#define BLT_PACKET_QUEUE_SIZE       3       // in packets
#define MAX_BAD_SAMPLE_INTERVAL     1000    // in milliseconds

#define VIDEO_STAT_INTERVAL         1000    // in milliseconds
#define VIDEO_STAT_INTERVAL_COUNT   2       // number of intervals to average over

#define MAX_OPTIMIZED_VIDEO_LEAD        200 // in milliseconds
#define DEFAULT_HARDWARE_BUFFER_COUNT   4

#define MAX_BLT_LOOPS               3
#define BLT_RELIEF_DELAY            1       // in milliseconds

// Absolute timing settings
#define N_STABILIZATION_ITERATIONS  5
#define MAX_ALLOWED_TIMING_ERROR    2
#define SMALLEST_TIMABLE_PERIOD     2

#define DEFAULT_LATE_FRAME_REBUFFER_TOLERANCE 500

// Default Decoding Priority
#ifdef _WIN32
    #ifdef HELIX_FEATURE_VIDREND_BOOSTDECODE_ON_STARTUP
    // on ce we need to get this started quickly, otherwise the initial packets expire
    // we set the priority so we can check in onpace to reset only once
    #define DFLT_DECODE_PRIORITY            THREAD_PRIORITY_HIGHEST
    #define DFLT_PRESENT_PRIORITY           THREAD_PRIORITY_ABOVE_NORMAL
    #else
    #define DFLT_DECODE_PRIORITY            THREAD_PRIORITY_NORMAL
    #define DFLT_PRESENT_PRIORITY           THREAD_PRIORITY_ABOVE_NORMAL
    #endif // HELIX_FEATURE_VIDREND_BOOSTDECODE_ON_STARTUP
#else   // _WIN32
#define DFLT_DECODE_PRIORITY        0
#define DFLT_PRESENT_PRIORITY       0
#endif  // _WIN32

// Lowest acceptable version numbers
#define STREAM_MAJOR_VERSION  0
#define STREAM_MINOR_VERSION  0

#define CONTENT_MAJOR_VERSION 0
#define CONTENT_MINOR_VERSION 0

#define DECODER_INTERVAL        5
#define BLTR_INTERVAL           5

#ifdef _WIN32
#define GETBITMAPCOLOR(x) GetBitmapColor( (LPBITMAPINFO)(x) )
#else   // _WIN32
#define GETBITMAPCOLOR(x) GetBitmapColor( (HXBitmapInfo*)(x))
#endif  // _WIN32


/****************************************************************************
 *  Includes
 */
#include "hlxclib/stdio.h"
#include "vidrend.ver"
#include "hxtypes.h"
#include "hlxclib/windows.h"

#if defined(_UNIX) && !defined(_MAC_UNIX)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(USE_XWINDOWS)
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#endif /* USE_XWINDOWS */

#endif

#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxthread.h"
#include "hxausvc.h"

// for sync
#include "hxengin.h"
#include "hxprefs.h"
#include "hxtick.h"
#include "timeval.h"
#include "hxevent.h"
#include "hxvsurf.h"
#include "smoothtime.h"

#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxerror.h"
#include "hxrendr.h"
#include "addupcol.h"
#include "pckunpck.h"
// #include "hxhyper.h"
#include "hxplugn.h"
#include "hxwin.h"
// #include "hxasm.h"
#include "hxmon.h"
#include "hxbuffer.h"   // for CHXBuffer

#include "hxassert.h"   // for HX_ASSERT()
#include "hxheap.h"     // for heap checking
#include "hxslist.h"    // CHXSimpleList
// Use this to affect the HXLOGLx statements
// in this file only
//#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"

#include "chxbufpl.h"

#include "sdpchunk.h"

#include "vidrend.h"
#include "cpacemkr.h"
#include "hxver.h"

#if defined(_WINDOWS) && !defined(WIN32_PLATFORM_PSPC)
#include "ddraw.h"
#endif  /* defined(_WINDOWS) && !defined(WIN32_PLATFORM_PSPC) */

// #include "coloracc.h"
#include "colormap.h"

#include "chxxtype.h"   // For CHXxSize

#include "hxprefs.h"
#include "hxprefutil.h"

/****************************************************************************
 *  Debug
 */
#ifdef ENABLE_SCHED_TRACE
#define MAX_SCHED_TRACE_ENTRIES 10000
ULONG32 ulSchedTraceIdx = 0;
LONG32 schedTraceArray[MAX_SCHED_TRACE_ENTRIES];

void DumpSchedEntries(void)
{
    FILE* pFile = NULL;
    ULONG32 ulIdx;

    if (ulSchedTraceIdx > 0)
    {
        pFile = fopen("\\helix\\sched.txt", "wb");
    }

    if (pFile)
    {
        for (ulIdx = 0; ulIdx < ulSchedTraceIdx; ulIdx++)
        {
            fprintf(pFile, "%d\n", schedTraceArray[ulIdx]);
        }

        fclose(pFile);
    }

    ulSchedTraceIdx = 0;
}
#endif  // ENABLE_SCHED_TRACE


#ifdef ENABLE_FETCH_TRACE
#define MAX_FETCH_TRACE_ENTRIES 10000
ULONG32 ulFetchTraceIdx = 0;
LONG32 fetchTraceArray[MAX_FETCH_TRACE_ENTRIES];

void DumpFetchEntries(void)
{
    FILE* pFile = NULL;
    ULONG32 ulIdx;

    if (ulFetchTraceIdx > 0)
    {
        pFile = fopen("\\helix\\fetch.txt", "wb");
    }

    if (pFile)
    {
        for (ulIdx = 0; ulIdx < ulFetchTraceIdx; ulIdx++)
        {
            fprintf(pFile, "%d\n", fetchTraceArray[ulIdx]);
        }

        fclose(pFile);
    }

    ulFetchTraceIdx = 0;
}
#endif  // ENABLE_FETCH_TRACE


#ifdef ENABLE_INPUT_TRACE
#define MAX_INPUT_TRACE_ENTRIES 10000
ULONG32 ulInputTraceIdx = 0;
LONG32 inputTraceArray[MAX_INPUT_TRACE_ENTRIES];

void DumpInputEntries(void)
{
    FILE* pFile = NULL;
    ULONG32 ulIdx;

    if (ulInputTraceIdx > 0)
    {
        pFile = fopen("\\helix\\input.txt", "wb");
    }

    if (pFile)
    {
        for (ulIdx = 0; ulIdx < ulInputTraceIdx; ulIdx++)
        {
            fprintf(pFile, "%d\n", inputTraceArray[ulIdx]);
        }

        fclose(pFile);
    }

    ulInputTraceIdx = 0;
}
#endif  // ENABLE_INPUT_TRACE


/****************************************************************************
 *  Constants
 */
const char* const CVideoRenderer::zm_pDescription    = "RealNetworks Video Renderer Plugin";
const char* const CVideoRenderer::zm_pCopyright      = HXVER_COPYRIGHT;
const char* const CVideoRenderer::zm_pMoreInfoURL    = HXVER_MOREINFO;

const char* const CVideoRenderer::zm_pStreamMimeTypes[] =
{
    NULL
};


/************************************************************************
 *  CVideoRenderer
 */
/************************************************************************
 *  Constructor/Destructor
 */
CVideoRenderer::CVideoRenderer(void)
        : m_lRefCount(0)
        , m_pMutex(NULL)
        , m_pBltMutex(NULL)
        , m_pVSMutex(NULL)
	, m_bVSMutexDisrupted(FALSE)
        , m_pScheduler(NULL)
        , m_pOptimizedScheduler(NULL)
        , m_pDecoderPump(NULL)
        , m_pBltrPump(NULL)
        , m_ulDecoderPacemakerId(0)
        , m_ulBltrPacemakerId(0)
        , m_lDecodePriority(DFLT_DECODE_PRIORITY)
        , m_pDecoderVideoFormat(NULL)
        , m_pBltrVideoFormat(NULL)
        , m_pVideoStats(NULL)
        , m_bIsScheduledCB(0)
        , m_ulLateFrameTol(LATE_FRAME_TOL)
        , m_ulEarlyFrameTol(EARLY_FRAME_TOL)
        , m_ulNoFramesPollingInterval(NO_FRAMES_POLLING_INTERVAL)
        , m_ulMaxSleepTime(MAX_SLEEP_TIME)
        , m_ulBltPacketQueueSize(BLT_PACKET_QUEUE_SIZE)
        , m_bSchedulerStartRequested(FALSE)
        , m_bPendingCallback(FALSE)
        , m_hPendingHandle(0)
        , m_ulCallbackCounter(0)
        , m_bSiteAttached(FALSE)
        , m_bDecoderRunning(FALSE)
        , m_PlayState(Stopped)
        , m_ulBytesToBuffer(0)
        , m_ulAvgBitRate(0)
        , m_ulPreroll(0)
        , m_ulVideoBufferingPreference(0)
        , m_ulBufferingStartTime(0)
        , m_ulBufferingStopTime(0)
        , m_ulBufferingTimeOut(0)
        , m_bBufferingOccured(FALSE)
        , m_bBufferingNeeded(FALSE)
        , m_bFirstFrame(TRUE)
	, m_bPreSeekPointFrames(TRUE)
        , m_bFirstSurfaceUpdate(TRUE)
        , m_bPendingRedraw(FALSE)
        , m_bVS1UpdateInProgress(FALSE)
	, m_bServicingFillbackDecode(FALSE)
        , m_pVSurf2InputBIH(NULL)
        , m_pBltPacketQueue(NULL)
        , m_bBitmapSet(FALSE)
        , m_bViewSizeSetInStreamHeader(FALSE)
        , m_bFrameSizeSetInStreamHeader(FALSE)
        , m_bFrameSizeInitialized(FALSE)
        , m_bWinSizeFixed(FALSE)
        , m_bOptimizedBlt(FALSE)
        , m_bOSGranuleBoost(OSGRANULE_BOOSTING_ENABLED)
        , m_bOSGranuleBoostVS2(OSGRANULE_BOOSTING_ENABLED)
#if defined(_WINDOWS) && defined(HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO)
        , m_bTryVideoSurface2(TRUE)
#else   // _WINDOWS
        , m_bTryVideoSurface2(FALSE)
#endif  // _WINDOWS
        , m_bUseVideoSurface2(FALSE)
        , m_bVideoSurface2Transition(FALSE)
        , m_bVideoSurface1Requested(FALSE)
        , m_bVideoSurfaceInitialized(FALSE)
        , m_bVideoSurfaceReinitRequested(FALSE)
        , m_bVS2BufferUnavailableOnLastBlt(FALSE)
        , m_bVSBufferUndisplayed(FALSE)
        , m_bPresentInProgress(FALSE)
	, m_bVS2Flushed(FALSE)
        , m_ulHWBufCount(DEFAULT_HARDWARE_BUFFER_COUNT)
        , m_ulConfigHWBufCount(DEFAULT_HARDWARE_BUFFER_COUNT)
        , m_ulSyncInterval(SYNC_INTERVAL)
        , m_ulMaxVidArea(0)
        , m_pResizeCB(NULL)
        , m_pVideoFormat(NULL)
        , m_pClipRect(NULL)
        , m_pContext(NULL)
        , m_pStream(NULL)
        , m_pHeader(NULL)
        , m_pBackChannel(0)
        , m_pMISUS(NULL)
        , m_pMISUSSite(NULL)
        , m_pCommonClassFactory(0)
        , m_pPreferences(NULL)
        , m_pRegistry(NULL)
        , m_ulRegistryID(0)
        , m_pActiveVideoPacket(NULL)
        , m_ulActiveVideoTime(0)
        , m_bUntimedRendering(FALSE)
        , m_bActiveVideoPacketLocalized(FALSE)
        , m_pTimeSyncSmoother(NULL)
        , m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
	, m_ulAbsPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
        , m_bKeyFrameMode(FALSE)
        , m_bAutoSwitch(TRUE)
        , m_bSentKeyFrameModeRequest(FALSE)
{
    HXLOGL4(HXLOG_BVID, "CON CVideoRenderer this=%p", this);
    m_SetWinSize.cx            = 0;
    m_SetWinSize.cy            = 0;
    m_LastSetSize.cx           = 0;
    m_LastSetSize.cy           = 0;
    m_StreamHeaderViewSize.cx  = 0;
    m_StreamHeaderViewSize.cy  = 0;
    m_StreamHeaderFrameSize.cx = 0;
    m_StreamHeaderFrameSize.cy = 0;

    m_rViewRect.left   = 0;
    m_rViewRect.top    = 0;
    m_rViewRect.right  = 0;
    m_rViewRect.bottom = 0;

    memset(&m_PreTransformBitmapInfoHeader, 0, sizeof(HXBitmapInfoHeader));
    memset(&m_BitmapInfoHeader, 0, sizeof(HXBitmapInfoHeader));
}

CVideoRenderer::~CVideoRenderer()
{
    HXLOGL4(HXLOG_BVID, "DES CVideoRenderer this=%p", this);
    Close();

    HX_DELETE(m_pVSurf2InputBIH);
    HX_DELETE(m_pClipRect);
}


void CVideoRenderer::Close(void)
{
    // NOTE: You should do your renderer cleanup here, instead of
    // in EndStream(), because your renderer is still left around
    // after the stream is ended in case it is a display renderer
    // and it needs to "paint" it's display area.
    EndOptimizedBlt();

    if (m_pActiveVideoPacket)
    {
        m_pActiveVideoPacket->Clear();
        delete m_pActiveVideoPacket;
        m_pActiveVideoPacket = NULL;
        m_ulActiveVideoTime = 0;
    }

    HX_RELEASE(m_pHeader);

    if (m_pVideoFormat)
    {
	m_pVideoFormat->Close();
        m_pVideoFormat->Release();
        m_pVideoFormat = NULL;
    }

    RemoveCallback(m_hPendingHandle);
    m_bPendingCallback = FALSE;

    HX_RELEASE(m_pOptimizedScheduler);
    HX_RELEASE(m_pScheduler);

    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pRegistry);

    ClearBltPacketQueue();
    HX_DELETE(m_pBltPacketQueue);

    HX_RELEASE(m_pResizeCB);

#if defined(HELIX_FEATURE_STATS)
    HX_DELETE(m_pVideoStats);
#endif /* HELIX_FEATURE_STATS */

    HX_DELETE(m_pTimeSyncSmoother);

    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pBltMutex);
    HX_RELEASE(m_pVSMutex);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);
}


/************************************************************************
 *  IHXPlugin Methods
 */
/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CVideoRenderer::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_ENABLE_LOGGING(pContext);
    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::InitPlugin()", this);
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pContext);

    m_pContext = pContext;
    m_pContext->AddRef();

    retVal = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                                        (void**) &m_pCommonClassFactory);

    if (SUCCEEDED(retVal))
    {
        m_pContext->QueryInterface(IID_IHXPreferences,
                                   (void**) &m_pPreferences);
    }

    m_pContext->QueryInterface(IID_IHXRegistry, (void**) &m_pRegistry);

    if (SUCCEEDED(retVal))
    {
        retVal = m_pContext->QueryInterface(IID_IHXScheduler,
                                            (void **) &m_pScheduler);
    }

    if (SUCCEEDED(retVal) && !m_pMutex)
    {
	retVal = CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    }

    if (SUCCEEDED(retVal) && !m_pBltMutex)
    {
	retVal = CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pBltMutex, m_pContext);
    }

    if (SUCCEEDED(retVal) && !m_pVSMutex)
    {
	retVal = CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pVSMutex, m_pContext);
    }

#if defined(HELIX_FEATURE_STATS)
    if (SUCCEEDED(retVal))
    {
        m_pVideoStats = new CVideoStatistics(m_pContext,
                                             VIDEO_STAT_INTERVAL_COUNT);

        retVal = HXR_OUTOFMEMORY;
        if (m_pVideoStats)
        {
            retVal = HXR_OK;
        }
    }
#endif /* HELIX_FEATURE_STATS */

    if (FAILED(retVal))
    {
        HX_RELEASE(m_pCommonClassFactory);
        HX_RELEASE(m_pPreferences);
        HX_RELEASE(m_pRegistry);
        HX_RELEASE(m_pScheduler);
    }

    return retVal;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple     whether or not this plugin DLL can be loaded
 *                      multiple times. All File Formats must set
 *                      this value to TRUE.
 *    pDescription      which is used in about UIs (can be NULL)
 *    pCopyright        which is used in about UIs (can be NULL)
 *    pMoreInfoURL      which is used in about UIs (can be NULL)
 */
STDMETHODIMP CVideoRenderer::GetPluginInfo
(
   REF(HXBOOL)        /*OUT*/ bLoadMultiple,
   REF(const char*) /*OUT*/ pDescription,
   REF(const char*) /*OUT*/ pCopyright,
   REF(const char*) /*OUT*/ pMoreInfoURL,
   REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright      = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetRendererInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CVideoRenderer::GetRendererInfo
(
 REF(const char**) /*OUT*/ pStreamMimeTypes,
 REF(UINT32)      /*OUT*/ unInitialGranularity
)
{
    pStreamMimeTypes = (const char**) zm_pStreamMimeTypes;
    unInitialGranularity = SYNC_INTERVAL;
    return HXR_OK;
}




/************************************************************************
 *  IHXRenderer Methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//              IHXRenderer::StartStream
//  Purpose:
//              Called by client engine to inform the renderer of the stream it
//              will be rendering. The stream interface can provide access to
//              its source or player. This method also provides access to the
//              primary client controller interface.
//
STDMETHODIMP CVideoRenderer::StartStream(IHXStream* pStream,
                                           IHXPlayer* pPlayer)
{
    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::StartStream(pStream=%p,pPlayer=%p)", this, pStream, pPlayer);
    m_pStream  = pStream;

    if (m_pStream)
    {
        m_pStream->AddRef();
    }

#if defined (HELIX_FEATURE_MISU)
    m_pCommonClassFactory->CreateInstance(
        CLSID_IHXMultiInstanceSiteUserSupplier,
        (void**) &m_pMISUS);

    if (m_pMISUS)
    {
        m_pMISUS->SetSingleSiteUser((IUnknown*)(IHXSiteUser*) this);
    }
#endif //HELIX_FEATURE_MISU

    if (m_pStream)
    {
        IHXStreamSource* pSource = 0;
        if (m_pStream->GetSource(pSource) == HXR_OK)
        {
            /* It is OK if the source does not support backchannel. Reasons:
             *
             * 1. This stream may not be coming from h261 fileformat.
             *    It may instead be merged into a container fileformat which
             *    may be does not support BackChannel.
             *
             * 2. The protocol used to serve this stream may not support
             *    BackChannel.
             */
            pSource->QueryInterface(IID_IHXBackChannel, (void**) &m_pBackChannel);

            pSource->Release();
        }
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//              IHXRenderer::EndStream
//  Purpose:
//              Called by client engine to inform the renderer that the stream
//              is was rendering is closed.
//
STDMETHODIMP CVideoRenderer::EndStream()
{
    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::EndStream()", this);
    // Stop Blts by changing state
    m_pMutex->Lock();
    ChangePlayState(Stopped);
    m_pMutex->Unlock();

    DisplayMutex_Lock();
    // Scheduler will continue to cycle if processing pre-seek frames even
    // if in stopped state.  Thus, reset pre-seek frame processing state
    // modifier.
    m_bPreSeekPointFrames = FALSE;
    // Remove decoder from availability
    IHXPaceMaker* pDecoderPump = m_pDecoderPump;
    m_pDecoderPump = NULL;
    DisplayMutex_Unlock();

    // Stop Decoder
    if (pDecoderPump)
    {
        pDecoderPump->Stop();
        pDecoderPump->Signal();
        pDecoderPump->WaitForStop();
        pDecoderPump->Release();
        pDecoderPump = NULL;
    }

    // Wait for Blt if in progress and then flush packet queues
    DisplayMutex_Lock();
    if (m_pVideoFormat)
    {
        m_pVideoFormat->Reset();
    }
    DisplayMutex_Unlock();

    // IHXStream, IHXSourceStream or IHXBackChannel
    // cannot be used after EndStream has been called.
    HX_RELEASE(m_pStream);
    HX_RELEASE(m_pBackChannel);

    // Stop Bltr pump
    if (m_pBltrPump)
    {
        m_pBltrPump->Stop();
        m_pBltrPump->Signal();
    }

    // Flush Optimized Video Surface if used
    if (m_bUseVideoSurface2 && m_pMISUSSite)
    {
        FlushVideoSurface2(m_pMISUSSite);
    }

    if (m_pBltrPump)
    {
        m_pBltrPump->WaitForStop();
        m_pBltrPump->Release();
        m_pBltrPump = NULL;
    }

    DisplayMutex_Lock();
    if (m_pVideoFormat)
    {
	m_pVideoFormat->Close();
        m_pVideoFormat->Release();
        m_pVideoFormat = NULL;
    }
    DisplayMutex_Unlock();

#ifdef ENABLE_SYNC_TRACE
    DumpSyncEntries();
#endif  // ENABLE_SYNC_TRACE

#ifdef ENABLE_SCHED_TRACE
    DumpSchedEntries();
#endif  // ENABLE_SCHED_TRACE

#ifdef ENABLE_FETCH_TRACE
    DumpFetchEntries();
#endif  // ENABLE_FETCH_TRACE

#ifdef ENABLE_INPUT_TRACE
    DumpInputEntries();
#endif  // ENABLE_INPUT_TRACE

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//              IHXRenderer::OnHeader
//  Purpose:
//              Called by client engine when a header for this renderer is
//              available. The header will arrive before any packets.
//
STDMETHODIMP CVideoRenderer::OnHeader(IHXValues* pHeader)
{
    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::OnHeader()", this);
    HX_RESULT retVal = HXR_OK;

    HX_DELETE(m_pClipRect);

    // Keep this for later use...
    HX_RELEASE(m_pHeader);
    m_pHeader = pHeader;
    m_pHeader->AddRef();

    // Create the timesync smoother
    HX_DELETE(m_pTimeSyncSmoother);
    m_pTimeSyncSmoother = new CHXTimeSyncSmoother();
    if (!m_pTimeSyncSmoother)
    {
        retVal = HXR_OUTOFMEMORY;
    }

    // check the stream versions
    if (SUCCEEDED(retVal))
    {
        m_pHeader->AddRef();
        retVal = CheckStreamVersions(m_pHeader);
        m_pHeader->Release();
    }

#ifdef HELIX_FEATURE_QUICKTIME
    IHXBuffer* pSDPData = NULL;

    if (SUCCEEDED(retVal) &&
        SUCCEEDED(pHeader->GetPropertyCString("SDPData", pSDPData)))
    {
        char *pData = (char*) pSDPData->GetBuffer();
        IHXValues *pValues = NULL;

        if (pData &&
            SUCCEEDED(SDPParseChunk(pData,
                                    strlen(pData),
                                    pValues,
                                    m_pCommonClassFactory,
                                    SDPCTX_Renderer)))
        {
            ULONG32 ulLeft;
            ULONG32 ulRight;
            ULONG32 ulTop;
            ULONG32 ulBottom;

            if (SUCCEEDED(pValues->GetPropertyULONG32(
                    "ClipFrameLeft", ulLeft)) &&
                SUCCEEDED(pValues->GetPropertyULONG32(
                    "ClipFrameRight", ulRight)) &&
                SUCCEEDED(pValues->GetPropertyULONG32(
                    "ClipFrameTop", ulTop)) &&
                SUCCEEDED(pValues->GetPropertyULONG32(
                    "ClipFrameBottom", ulBottom)))
            {
                retVal = SetClipRect((INT32) ulLeft,  (INT32) ulTop,
                                     (INT32) ulRight, (INT32) ulBottom);
            }
        }

        HX_RELEASE(pValues);
    }

    HX_RELEASE(pSDPData);
#endif  // HELIX_FEATURE_QUICKTIME

    if (SUCCEEDED(retVal))
    {
        m_pVideoFormat = CreateFormatObject(m_pHeader);

        retVal = HXR_OUTOFMEMORY;
        if (m_pVideoFormat)
        {
            m_pVideoFormat->AddRef();
            retVal = HXR_OK;
        }
    }

    if (SUCCEEDED(retVal))
    {
        retVal = m_pVideoFormat->Init(pHeader);
    }

#ifdef HELIX_FEATURE_VIDEO_ENFORCE_MAX_FRAMESIZE
    UINT32 ulFrameWidth = 0;
    UINT32 ulFrameHeight = 0;
    if (SUCCEEDED(pHeader->GetPropertyULONG32("FrameWidth",  ulFrameWidth)) &&
        SUCCEEDED(pHeader->GetPropertyULONG32("FrameHeight",  ulFrameHeight))
       )
    {

      UINT32 ulMaxVidWidth = 0;
      UINT32 ulMaxVidHeight = 0;

      ReadPrefUINT32(m_pPreferences, "MaxVideoWidth", ulMaxVidWidth);
      ReadPrefUINT32(m_pPreferences, "MaxVideoHeight", ulMaxVidHeight);

      m_ulMaxVidArea = ulMaxVidWidth * ulMaxVidHeight;


        if (m_ulMaxVidArea > 0 && ((ULONG32) (ulFrameWidth*ulFrameHeight)) > m_ulMaxVidArea)
        {
            retVal = HXR_SLOW_MACHINE;
        }
    }
#endif 

    if (SUCCEEDED(retVal))
    {
        m_ulEarlyFrameTol = GetEarlyFrameTolerance();
        m_ulLateFrameTol = GetLateFrameTolerance();
        m_ulMaxOptimizedVideoLead = GetMaxOptimizedVideoLead();

        m_ulMaxSleepTime = GetMaxSleepTime();
        m_ulNoFramesPollingInterval = GetNoFramesPollingInterval();
        m_ulBltPacketQueueSize = GetBltPacketQueueSize();
        m_pTimeSyncSmoother->SetSyncGoalSmoothingDepth(GetSyncGoalSmoothingDepth());
        m_pTimeSyncSmoother->SetSpeedupGoalSmoothingDepth(GetSpeedupGoalSmoothingDepth());
        m_pTimeSyncSmoother->SetMaxBadSeqSamples(GetMaxBadSeqSamples());
    }

    // Setup preroll
    if (SUCCEEDED(retVal))
    {
        ULONG32 ulMinPreroll = m_pVideoFormat->GetMinimumPreroll(pHeader);
        ULONG32 ulMaxPreroll = m_pVideoFormat->GetMaximumPreroll(pHeader);

        // Check that the stream header has a preroll value. If not...
        if (HXR_OK != pHeader->GetPropertyULONG32("Preroll", m_ulPreroll))
        {
            // ... let's use default value.
            m_ulPreroll = m_pVideoFormat->GetDefaultPreroll(pHeader);
        }
        
        // Apply min/max constraints to media preroll
        if ( m_ulPreroll > ulMaxPreroll )
        {
            m_ulPreroll = ulMaxPreroll;
        }
        else if (m_ulPreroll < ulMinPreroll)
        {
            m_ulPreroll = ulMinPreroll;
        }

        // Add the output queue duration to the post decode delay
        ULONG32 ulPostDecodeDelay = 0;
        pHeader->GetPropertyULONG32("PostDecodeDelay", ulPostDecodeDelay);
        ulPostDecodeDelay += m_pVideoFormat->GetOutputQueueDuration();
        pHeader->SetPropertyULONG32("PostDecodeDelay", ulPostDecodeDelay);

        // Update the preroll value in the header
        pHeader->SetPropertyULONG32("Preroll", m_ulPreroll);
    }

    // Determine Average Bitrate
    if (SUCCEEDED(retVal))
    {
        if (FAILED(pHeader->GetPropertyULONG32("AvgBitRate", m_ulAvgBitRate)))
        {
            m_ulAvgBitRate = 0;
        }
    }

    // Create Blt Queue
    if (SUCCEEDED(retVal))
    {
        m_pBltPacketQueue = new CRingBuffer(m_ulBltPacketQueueSize);

        retVal = HXR_OUTOFMEMORY;
        if (m_pBltPacketQueue)
        {
            retVal = HXR_OK;
        }
    }

    // Read the preference to drop late accelerated keyframes
    if (m_pPreferences)
    {
        HXBOOL bTmp = FALSE;
        if (SUCCEEDED(ReadPrefBOOL(m_pPreferences, "PlaybackVelocity\\DropLateAccelKeyFrames", bTmp)))
        {
            m_pVideoFormat->SetDropLateAccelKeyFramesFlag(bTmp);
        }

        // retrieve the preference value 
        // Values:
        // 0 = do not rebuffer on video
        // 1 = rebuffer on video
        // 2 = rebuffer if video only playback
        // we will be using m_ulVideoBufferingPreference to control rebuffering behavior 
        ReadPrefUINT32(m_pPreferences, "RebufferOnVideo", m_ulVideoBufferingPreference);
    }

    return retVal;
}


/////////////////////////////////////////////////////////////////////////////
//  Method:
//      CVideoRenderer::CheckStreamVersions
//  copied from CRealAudioRenderer
HX_RESULT CVideoRenderer::CheckStreamVersions(IHXValues* pHeader)
{
    // check stream and content versions so an upgrade can
    // be called if necessary...
    HX_RESULT pnr = HXR_OK;

    HXBOOL bVersionOK = TRUE;

    UINT32 ulStreamVersion = 0;
    UINT32 ulContentVersion = 0;

    if(HXR_OK == pHeader->GetPropertyULONG32("StreamVersion",
        ulStreamVersion))
    {
        UINT32 ulMajorVersion = HX_GET_MAJOR_VERSION(ulStreamVersion);
        UINT32 ulMinorVersion = HX_GET_MINOR_VERSION(ulStreamVersion);
        ULONG32 ulThisMajorVersion = 0;
        ULONG32 ulThisMinorVersion = 0;

        GetStreamVersion(ulThisMajorVersion, ulThisMinorVersion);

        if((ulMajorVersion > ulThisMajorVersion) ||
           ((ulMinorVersion > ulThisMinorVersion) &&
            (ulMajorVersion == ulThisMajorVersion)))
        {
            bVersionOK = FALSE;
        }
    }

    if(bVersionOK &&
       (HXR_OK == pHeader->GetPropertyULONG32("ContentVersion",
                                              ulContentVersion)))
    {
        UINT32 ulMajorVersion = HX_GET_MAJOR_VERSION(ulContentVersion);
        UINT32 ulMinorVersion = HX_GET_MINOR_VERSION(ulContentVersion);
        ULONG32 ulThisMajorVersion = 0;
        ULONG32 ulThisMinorVersion = 0;

        GetContentVersion(ulThisMajorVersion, ulThisMinorVersion);

        if((ulMajorVersion > ulThisMajorVersion) ||
           ((ulMinorVersion > ulThisMinorVersion) &&
            (ulMajorVersion == ulMajorVersion)))
        {
            bVersionOK = FALSE;
        }
    }

    if(!bVersionOK)
    {
        AddToAutoUpgradeCollection(GetUpgradeMimeType(), m_pContext);

        pnr = HXR_FAIL;
    }

    return pnr;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//              IHXRenderer::OnPacket
//  Purpose:
//              Called by client engine when a packet for this renderer is
//              due.
//
STDMETHODIMP CVideoRenderer::OnPacket(IHXPacket* pPacket, LONG32 lTimeOffset)
{
    // Ignore any packet delivered during the seek state since they are
    // pre-seek packets and thus out of context for video playback.
    if (m_PlayState == Seeking)
    {
	return HXR_OK;
    }

    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::OnPacket() pts=%lu offset=%ld rule=%u flags=0x%02x timenow=%lu decqdepth=%lu",
	    this,
            (pPacket ? pPacket->GetTime() : 0),
	    lTimeOffset,
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0),
            m_pTimeSyncSmoother->GetTimeNow(),
	    m_pVideoFormat ? m_pVideoFormat->GetDecodedFrameQueueDepth() : 0);

    m_pTimeSyncSmoother->SetTimelineOffset(lTimeOffset);

#ifdef ENABLE_INPUT_TRACE
    if (ulInputTraceIdx < MAX_INPUT_TRACE_ENTRIES)
    {
        inputTraceArray[ulInputTraceIdx++] =
            ComputeTimeAhead(pPacket->GetTime(), 0);
    }
#endif  // ENABLE_INPUT_TRACE

    if (m_bSchedulerStartRequested)
    {
        StartSchedulers();
    }

    HXBOOL bQueueRet = m_pVideoFormat->Enqueue(pPacket);
    if (bQueueRet == FALSE && m_pVideoFormat->GetLastError() == HXR_OUTOFMEMORY)
    {
	return HXR_OUTOFMEMORY;
    }

    // try to decode a frame
    if (m_PlayState == Playing)
    {
        if (!IsDecoderRunning() || (m_pVideoFormat->GetDecodedFrameQueueDepth() == 0))
        {
	    HXBOOL bDecRet = m_pVideoFormat->DecodeFrame(IsDecoderRunning() ? 0 : MAX_VIDREND_DECODE_SPIN);
            if( bDecRet == FALSE && m_pVideoFormat->GetLastError() == HXR_OUTOFMEMORY )
            {
                return HXR_OUTOFMEMORY;
            }
        }
    }
    else
    {
        if (!m_pTimeSyncSmoother->IsBaseTimeSet())
        {
            m_pMutex->Lock();
            if (m_PlayState != Playing)
            {
                m_pTimeSyncSmoother->SetBaseTime(pPacket->GetTime() - lTimeOffset);
		m_ulActiveVideoTime = pPacket->GetTime() - lTimeOffset;
            }
            m_pMutex->Unlock();
            m_pTimeSyncSmoother->SetBaseTimeFlag();
        }

#ifdef HELIX_FEATURE_VIDREND_THREADEDDECODE_ON_STARTUP
	if (IsDecoderRunning() && m_pDecoderPump)
	{
	    m_pDecoderPump->Signal();
	}
	else
#endif  // HELIX_FEATURE_VIDREND_THREADEDDECODE_ON_STARTUP
	{
	    m_pVideoFormat->DecodeFrame();
	}

	if (m_PlayState == Buffering)
	{
	    if (IsBufferingComplete(pPacket))
	    {
		RequestBufferingEnd();
	    }
	}
    }

    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::OnPacket() Exiting timenow=%lu decqdepth=%lu", this,
            m_pTimeSyncSmoother->GetTimeNow(),
	    m_pVideoFormat ? m_pVideoFormat->GetDecodedFrameQueueDepth() : 0);

    return HXR_OK;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//              IHXRenderer::OnTimeSync
//  Purpose:
//              Called by client engine to inform the renderer of the current
//              time relative to the streams synchronized time-line. The
//              renderer should use this time value to update its display or
//              render it's stream data accordingly.
//
STDMETHODIMP CVideoRenderer::OnTimeSync(ULONG32 ulTime)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::OnTimeSync(%lu)", this, ulTime);
    m_pTimeSyncSmoother->OnTimeSync(ulTime);

    if (m_bSchedulerStartRequested)
    {
        StartSchedulers();
    }

#ifdef HELIX_FEATURE_STATS
    if (((ULONG32) (m_pTimeSyncSmoother->GetLastTickCount() -
                    m_pVideoStats->GetLastSyncTime())) >=
        VIDEO_STAT_INTERVAL)
    {
        m_pVideoStats->SyncStats(m_pTimeSyncSmoother->GetLastTickCount());
    }
#endif /* HELIX_FEATURE_STATS */

    if (m_PlayState == Playing)
    {
        if (!IsDecoderRunning())
        {
            m_pVideoFormat->DecodeFrame();
        }

        if (m_bBufferingNeeded)
        {
            m_pMutex->Lock();

            if (m_PlayState == Playing)
            {
                BeginBuffering();
            }

            m_pMutex->Unlock();
        }
    }
    else if ((m_PlayState == PlayStarting) ||
             (m_PlayState == Buffering))
    {
        m_pMutex->Lock();

        m_pTimeSyncSmoother->SetBaseTimeFlag();

        // Reset the offset to avoid race condition
        // with BaseTime setting in OnPacket()
        if (m_pTimeSyncSmoother->WasLastSampleGood())
        {
            m_pTimeSyncSmoother->ResetOffset();
        }

        if (m_PlayState == Buffering)
        {
            EndBuffering();
            ChangePlayState(PlayStarting);
        }

        if (m_PlayState == PlayStarting)
        {
            ChangePlayState(Playing);

            BltIfNeeded();

            StartSchedulers();

            if (m_pBltrPump)
            {
                m_pBltrPump->Signal();
            }
        }

        m_pMutex->Unlock();
    }

    return HXR_OK;
}


HX_RESULT CVideoRenderer::StartSchedulers(void)
{
    HX_RESULT retVal = HXR_OK;

    /* 
     * When running the decoder as fast as possible, we dont want to start the
     * scheduler, so short circuit this code. Additionally, calls such as 
     * BltIfNeeded() and ScheduleCallback() (which may still be called) make 
     * assumptions based on the state of our interactions with the scheduler, 
     * so we satisfy them by setting a pending handle representing a 
     * nonexistant callback.
     */
    if( m_bUntimedRendering )
    {
        m_hPendingHandle = 1;
        return HXR_OK;
    }

    m_bSchedulerStartRequested = FALSE;

    DisplayMutex_Lock();

    if (ShouldKickStartScheduler())
    {
        m_bBufferingNeeded = FALSE;
        ScheduleCallback(0);
    }

    if (SUCCEEDED(retVal) && (m_pDecoderPump == NULL) && ShouldDecodeOnSeparateThread())
    {
	retVal = CreateInstanceCCF(CLSID_IHXPaceMaker, (void**)&m_pDecoderPump, m_pContext);
        if (SUCCEEDED(retVal))
        {
            // We will use the decoder pace maker in suspended mode
            m_pDecoderPump->Suspend(TRUE);
            m_pDecoderPump->Start(this,
                GetDecodePriority(),
                DECODER_INTERVAL,
                m_ulDecoderPacemakerId);
        }
    }

    if (SUCCEEDED(retVal) && (m_pBltrPump == NULL) && m_bTryVideoSurface2)
    {
	retVal = CreateInstanceCCF(CLSID_IHXPaceMaker, (void**)&m_pBltrPump, m_pContext);
        if (SUCCEEDED(retVal))
        {
            m_pBltrPump->Start(this,
                DFLT_PRESENT_PRIORITY,
                BLTR_INTERVAL,
                m_ulBltrPacemakerId);
        }
    }

    DisplayMutex_Unlock();

    return retVal;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXRenderer::OnPreSeek
//  Purpose:
//      Called by client engine to inform the renderer that a seek is
//      about to occur. The render is informed the last time for the
//      stream's time line before the seek, as well as the first new
//      time for the stream's time line after the seek will be completed.
//
STDMETHODIMP CVideoRenderer::OnPreSeek(ULONG32 ulOldTime, ULONG32 ulNewTime)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::OnPreSeek(OldTime=%lu, NewTime=%lu)", this, ulOldTime, ulNewTime);

    // Change state to stop Blts
    m_pMutex->Lock();
    EndBuffering();
    ChangePlayState(Seeking);
    m_pMutex->Unlock();

    // Suspend the decoder thread
    m_pVideoFormat->SuspendDecode(TRUE);
    if (m_pDecoderPump)
    {
        m_pDecoderPump->Signal();
        m_pDecoderPump->WaitForSuspend();
    }

    // Wait for Blt in progress to complete and reset
    // packet queues
    DisplayMutex_Lock();

    m_pVideoFormat->SetStartTime(ulNewTime);
    m_pVideoFormat->Reset();
#if defined(HELIX_FEATURE_STATS)
    m_pVideoStats->ResetSequence();
#endif /* HELIX_FEATURE_STATS */

    m_bFirstSurfaceUpdate = TRUE;
    m_bFirstFrame = TRUE;
    m_bPreSeekPointFrames = TRUE;
    m_bVSBufferUndisplayed = FALSE;

    m_pTimeSyncSmoother->ClearBaseTimeFlag();

    DisplayMutex_Unlock();

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXRenderer::OnPostSeek
//  Purpose:
//      Called by client engine to inform the renderer that a seek has
//      just occured. The render is informed the last time for the
//      stream's time line before the seek, as well as the first new
//      time for the stream's time line after the seek.
//
STDMETHODIMP CVideoRenderer::OnPostSeek(ULONG32 ulOldTime, ULONG32 ulNewTime)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::OnPostSeek(OldTime=%lu, NewTime=%lu)", this, ulOldTime, ulNewTime);

    DisplayMutex_Lock();

    // Resume the decoder thread
    m_pVideoFormat->SuspendDecode(FALSE);
    if (m_pDecoderPump)
    {
        m_pDecoderPump->Signal();
    }

    if (m_bUseVideoSurface2 && m_pMISUSSite)
    {
        FlushVideoSurface2(m_pMISUSSite);
    }

    // clean up the packet lists
    m_pVideoFormat->SetStartTime(ulNewTime);
    m_pVideoFormat->Reset();

#if defined(HELIX_FEATURE_STATS)
    m_pVideoStats->ResetSequence();
#endif /* HELIX_FEATURE_STATS */

    m_bFirstSurfaceUpdate = TRUE;
    m_bFirstFrame = TRUE;
    m_bPreSeekPointFrames = TRUE;
    m_bVSBufferUndisplayed = FALSE;

    m_pTimeSyncSmoother->SetBaseTime(ulNewTime);
    m_pTimeSyncSmoother->SetBaseTimeFlag();
    m_ulActiveVideoTime = ulNewTime;
    m_bVS2BufferUnavailableOnLastBlt = FALSE;

    DisplayMutex_Unlock();

    // PostSeek signals the proper packets are to start arriving
    m_pMutex->Lock();
    ChangePlayState(PlayStarting);
    m_pMutex->Unlock();

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXRenderer::OnPause
//  Purpose:
//      Called by client engine to inform the renderer that a pause has
//      just occured. The render is informed the last time for the
//      stream's time line before the pause.
//
STDMETHODIMP CVideoRenderer::OnPause(ULONG32 ulTime)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::OnPause(Time=%lu)", this, ulTime);

    m_pMutex->Lock();

    ChangePlayState(Paused);

    m_pMutex->Unlock();

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXRenderer::OnBegin
//  Purpose:
//      Called by client engine to inform the renderer that a begin or
//      resume has just occured. The render is informed the first time
//      for the stream's time line after the resume.
//
STDMETHODIMP CVideoRenderer::OnBegin(ULONG32 ulTime)
{   
    HX_RESULT retVal = HXR_OK;

    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::OnBegin(Time=%lu)", this, ulTime);

    m_pMutex->Lock();

    m_bBufferingOccured = FALSE;
    // If we are seeking, PostSeek will notify us when the play
    // will be starting
    if (m_PlayState != Seeking)
    {
        ChangePlayState(PlayStarting);
    }

    // No need to clear the Blt Packet queue here since
    // The Refresh event will always clear it and clearing
    // it here would create a race condition on Blt Packet
    // Queue (ring buffer) read.
    // ClearBltPacketQueue();
    m_pTimeSyncSmoother->ResetSampleCounts();

    // Following statements used t0 be commented out as OnBegin used to report 
    // incorrect starting time for live streams.
    m_pTimeSyncSmoother->SetBaseTime(ulTime);
    m_pTimeSyncSmoother->SetBaseTimeFlag();

    m_ulActiveVideoTime = ulTime;

    m_bIsScheduledCB = 0;
    m_bVS2BufferUnavailableOnLastBlt = FALSE;

    retVal = StartSchedulers();

    m_pMutex->Unlock();

#ifdef HELIX_FEATURE_VIDREND_BOOSTDECODE_ON_STARTUP
    if (m_pDecoderPump)
    {
       //m_pDecoderPump->Suspend(TRUE);
       //m_pDecoderPump->WaitForSuspend();  // Wait for decoder thread to start and suspends itself
       //m_pDecoderPump->Suspend(FALSE);
       m_pDecoderPump->Signal();   // Kick-out decoder pump from suspension
    }
#endif  // HELIX_FEATURE_VIDREND_BOOSTDECODE_ON_STARTUP

    return retVal;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXRenderer::OnBuffering
//  Purpose:
//      Called by client engine to inform the renderer that buffering
//      of data is occuring. The render is informed of the reason for
//      the buffering (start-up of stream, seek has occured, network
//      congestion, etc.), as well as percentage complete of the
//      buffering process.
//
STDMETHODIMP CVideoRenderer::OnBuffering(ULONG32 ulFlags, UINT16 unPercentComplete)
{
    HX_RESULT retVal = HXR_OK;

    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::OnBuffering(Flags=%lu, Percent=%hu)", this, ulFlags, unPercentComplete);

    m_pMutex->Lock();

    if (m_PlayState == Buffering)
    {
        if (IsBufferingComplete())
        {
            EndBuffering();
        }
    }
    else if (m_PlayState == Playing)
    {
	// Time-line halted - clear the time-line smoothing history as the
	// on resumption, we'll have a play-time to wall-clock time shift and
	// will need to restart learning the relation between the two.
	m_pTimeSyncSmoother->ResetSampleCounts();
        ChangePlayState(PlayStarting);
    }

    m_pMutex->Unlock();

    return retVal;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXRenderer::GetDisplayType
//  Purpose:
//      Called by client engine to ask the renderer for it's preferred
//      display type. When layout information is not present, the
//      renderer will be asked for it's prefered display type. Depending
//      on the display type a buffer of additional information may be
//      needed. This buffer could contain information about preferred
//      window size.
//
STDMETHODIMP CVideoRenderer::GetDisplayType(REF(HX_DISPLAY_TYPE) ulFlags,
                                           REF(IHXBuffer*) pBuffer)
{
    ulFlags = HX_DISPLAY_WINDOW |
              HX_DISPLAY_SUPPORTS_RESIZE |
              HX_DISPLAY_SUPPORTS_FULLSCREEN |
              HX_DISPLAY_SUPPORTS_VIDCONTROLS;

    return HXR_OK;
}

/************************************************************************
*       Method:
*           IHXRenderer::OnEndofPackets
*       Purpose:
*           Called by client engine to inform the renderer that all the
*           packets have been delivered. However, if the user seeks before
*           EndStream() is called, renderer may start getting packets again
*           and the client engine will eventually call this function again.
*/
STDMETHODIMP CVideoRenderer::OnEndofPackets(void)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::OnEndofPackets()", this);

    if (m_pVideoFormat)
    {
        m_pVideoFormat->OnRawPacketsEnded();
    }

    m_pMutex->Lock();
    EndBuffering();
    m_pMutex->Unlock();	
    return HXR_OK;
}


/************************************************************************
 *  IHXStatistics Methods
 */
/************************************************************************
 *  InitializeStatistics
 */
#define MAX_STAT_ENTRY_PAIRS    32
STDMETHODIMP CVideoRenderer::InitializeStatistics(UINT32 ulRegistryID)
{
    m_ulRegistryID = ulRegistryID;

#if defined(HELIX_FEATURE_STATS)
    HXBOOL bCodecNameKnown = FALSE;
    char* pValue = NULL;
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::InitializeStatistics(RegistryID=%lu)", this, ulRegistryID);

    if (m_pVideoStats)
    {
        retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
        pValue = (char*) GetCodecName();
        if (pValue != NULL)
        {
            ReportStat(VS_CODEC_NAME, pValue);
            bCodecNameKnown = TRUE;
        }
    }

    if (SUCCEEDED(retVal))
    {
        pValue = (char*) GetRendererName();
        if (pValue != NULL)
        {
            ReportStat(VS_REND_NAME, pValue);
            // If Codec name is unknown, use a more generic renderer name
            if (!bCodecNameKnown)
            {
                ReportStat(VS_CODEC_NAME, pValue);
            }
        }
    }

    if (SUCCEEDED(retVal))
    {
        pValue = (char*) GetCodecFourCC();
        if (pValue != NULL)
        {
            ReportStat(VS_CODEC_4CC, pValue);
        }
    }

    if (SUCCEEDED(retVal))
    {
        ReportStat(VS_SURFACE_MODE, (INT32) (m_bUseVideoSurface2 ? 2 : 1));
        ReportStat(VS_CURRENT_FRAMERATE, "0.0");
        ReportStat(VS_FRAMES_DISPLAYED, "100.0");
        ReportStat(VS_FRAMES_DROPPED, (INT32) 0);
        ReportStat(VS_FRAMES_UPSAMPLED, "0.0");
        ReportStat(VS_FAILED_BLTS, (INT32) 0);
        ReportStat(VS_FRAMES_LOST, (INT32) 0);
        ReportStat(VS_SURESTREAM, "FALSE");
        ReportStat(VS_IMAGE_WIDTH, (INT32) 0);
        ReportStat(VS_IMAGE_HEIGHT, (INT32) 0);
	ReportStat(VS_CODEC_FRAME_WIDTH, (INT32) 0);
        ReportStat(VS_CODEC_FRAME_HEIGHT, (INT32) 0);
    }

    if (SUCCEEDED(retVal))
    {
        InitExtraStats();
    }

    if (SUCCEEDED(retVal))
    {
        retVal = m_pVideoStats->DisplayStats(m_ulRegistryID);
    }

    return retVal;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS */
}

/************************************************************************
 *  UpdateStatistics
 */
STDMETHODIMP CVideoRenderer::UpdateStatistics()
{
#if defined(HELIX_FEATURE_STATS)
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::UpdateStatistics()", this);

    if (m_pVideoStats)
    {
        retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
        retVal = m_pVideoStats->DisplayStats(m_ulRegistryID);
    }

    return retVal;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS */
}


/************************************************************************
 *  IHXUntimedDecoder Methods
 */
STDMETHODIMP_(HXBOOL) CVideoRenderer::IsUntimedRendering()
{
    return IsUntimedMode();
}

STDMETHODIMP_(HX_RESULT) CVideoRenderer::SetUntimedRendering(HXBOOL bUntimedRendering)
{
    HX_RESULT hr = HXR_OK;

#if defined(HELIX_FEATURE_VIDREND_UNTIMED_DECODE)
    if( m_PlayState != Stopped && m_PlayState != PlayStarting )
    {
        hr = HXR_UNEXPECTED;
    }
    else
    {
	if( m_bUntimedRendering && !bUntimedRendering )
	{
	    m_hPendingHandle = NULL;
	}
	m_bUntimedRendering = bUntimedRendering;
        hr = UntimedModeNotice(bUntimedRendering);
    }
#endif // defined(HELIX_FEATURE_VIDREND_UNTIMED_DECODE)

    return hr;
}


/************************************************************************
 *  IHXUpdateProperties Methods
 */
/************************************************************************
 *  UpdatePacketTimeOffset
 *      Call this method to update the timestamp offset of cached packets
 */
STDMETHODIMP CVideoRenderer::UpdatePacketTimeOffset(INT32 lTimeOffset)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::UpdatePacketTimeOffset(Offset=%ld)", this, lTimeOffset);

    m_pTimeSyncSmoother->UpdateTimelineOffset(lTimeOffset);

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXUpdateProperties::UpdatePlayTimes
 *      Purpose:
 *          Call this method to update the playtime attributes
 */
STDMETHODIMP
CVideoRenderer::UpdatePlayTimes(IHXValues* pProps)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::UpdatePlayTimes()", this);

    return HXR_OK;
}


/************************************************************************
 *  IHXRenderTimeLine Methods
 */
/************************************************************************
 *  GetTimeLineValue
 */
STDMETHODIMP CVideoRenderer::GetTimeLineValue(REF(UINT32) ulTime)
{
    HX_RESULT retVal = HXR_OK;

    ulTime = ((ULONG32) -ComputeTimeAhead(0, 0));

    switch (m_PlayState)
    {
        case Stopped:
            retVal = HXR_TIMELINE_ENDED;
            break;
        case Buffering:
        case PlayStarting:
        case Paused:
        case Seeking:
            retVal = HXR_TIMELINE_SUSPENDED;
            break;
        case Playing:
            retVal = HXR_OK;
            break;
    }

    return retVal;
}


/************************************************************************
 *  IHXMediaPushdown Methods
 */
/************************************************************************
 *  Method:
 *      IHXMediaPushdown::GetCurrentPushdown
 *  Purpose:
 *      Retrieves the current queue depth ("pushdown depth") in milliseconds
 *      and the number of decoded frames in the output queue.
 *
 *  Notes:
 *      This is the *decoded* pushdown, not the undecoded pushdown.  Returns
 *      HXR_TIMELINE_SUSPENDED if the stream is paused; HXR_FAIL
 *      if the stream is finished, HXR_OK otherwise.
 */
STDMETHODIMP
CVideoRenderer::GetCurrentPushdown(REF(UINT32) ulPushdownMS,
                                   REF(UINT32) ulNumFrames)
{
    ulPushdownMS = 0;
    ulNumFrames = 0;
    
    if (!m_pVideoFormat)
    {
        return HXR_FAIL;    
    }
    
    // If the first frame is still pending, we cannot count any frames
    // in post decode buffer towards the pushdown as they may be
    // pre-start frames used to warm up the decoder from the key-frame
    // that are to be flushed.
    if (!m_bPreSeekPointFrames)
    {
	INT32 lPushdownDelta;

	if (!m_bFirstFrame)
	{
	    ulNumFrames = 1;
	}

	ulNumFrames += (m_pBltPacketQueue->Count() + m_pVideoFormat->GetDecodedFrameQueueDepth());

	lPushdownDelta = 
	    ComputeTimeAhead(
		    m_pVideoFormat->GetLastDecodedFrameTime(),
		    0);

	ulPushdownMS = ((lPushdownDelta >= 0) ? ((UINT32) lPushdownDelta) : 0);
    }

    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::GetCurrentPushdown() PastPreSeek=%c ulPushdownMS=%lu ulNumFrames=%lu", this,
	    m_bPreSeekPointFrames ? 'F' : 'T',
	    ulPushdownMS, 
	    ulNumFrames);

    return (m_PlayState == Playing) ? HXR_OK : HXR_TIMELINE_SUSPENDED;
}

/************************************************************************
 *  Method:
 *      IHXMediaPushdown::IsG2Video
 *  Purpose:
 */
STDMETHODIMP_(HXBOOL)
CVideoRenderer::IsG2Video()
{
    return FALSE;
}


/************************************************************************
 *  IHXSiteUser Methods
 */
/************************************************************************
 *  AttachSite
 */
STDMETHODIMP CVideoRenderer::AttachSite(IHXSite* /*IN*/ pSite)
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::AttachSite(Site=%p)", this, pSite);

    if (m_pMISUSSite)
    {
        return HXR_UNEXPECTED;
    }

    m_bSiteAttached = TRUE;

    m_pMISUSSite = pSite;
    m_pMISUSSite->AddRef();

    // for sync
    IHXInterruptSafe* pIHXInterruptSafe = NULL;

    if (HXR_OK == m_pMISUSSite->QueryInterface(IID_IHXInterruptSafe,
                                               (void**)&pIHXInterruptSafe))
    {
        // First check if the derived renderer allows us to
        // use the optimized scheduler or not. If it does,
        // then check if the use of the optimized scheduler
        // is disabled by a preference.
        HXBOOL bUseOptimized = CanUseOptimizedScheduler();
        if (bUseOptimized)
        {
            // Check the "UseOptimizedScheduler" pref
            ReadPrefBOOL(m_pPreferences, "UseOptimizedScheduler", bUseOptimized);
            // Now if we are able to use the optimized scheduler, then get it
            if (pIHXInterruptSafe->IsInterruptSafe() && bUseOptimized)
            {
                HX_RELEASE(m_pOptimizedScheduler);
                if (HXR_OK !=
                        m_pContext->QueryInterface(IID_IHXOptimizedScheduler,
                        (void **) &m_pOptimizedScheduler))
                {
                    // just for good luck
                    m_pOptimizedScheduler = NULL;
                }
            }
        }
    }
    HX_RELEASE(pIHXInterruptSafe);

#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    // Get Run Configuration
    // Use of VideoSurface2
    if (m_pPreferences)
    {
	if (SUCCEEDED(ReadPrefBOOL(m_pPreferences, "VideoBoost\\NoFlip", m_bTryVideoSurface2)))
	{
	    m_bTryVideoSurface2 = !m_bTryVideoSurface2;
	}
    }

    // Use of OS Granule Boosting
    if (m_pPreferences)
    {
        ReadPrefBOOL(m_pPreferences, "VideoBoost\\NoOSGranuleBoost", m_bOSGranuleBoost);
        m_bOSGranuleBoostVS2 = m_bOSGranuleBoost;
    }

    // Hardware buffer count to request from VideoSurface2
    if (m_bTryVideoSurface2 && m_pPreferences)
    {
        ReadPrefUINT32(m_pPreferences, "VideoBoost\\InitialHSCount", m_ulConfigHWBufCount);
    }

#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    /*
     * This is the best time to set the size of the site, we
     * know for example the the header has already been received
     * at this point.
     *
     * In most display renderers, you will store size information
     * in your stream header. In this example, we assume a specific
     * size, but we will initialize that size here as if it had
     * come from our header.
     *
     */
    SetupBitmapDefaults(m_pHeader, m_BitmapInfoHeader);
    // If the view size was set in the stream header, then
    // the renderer should initially set the site size to
    // this width and height and then not change it. Calling
    // FormatAndSetViewFrame() with bAsDefault=FALSE will
    // cause the site size to be fixed. It can still be changed
    // by user interaction, but not be the renderer.
    HXBOOL bAsDefault = TRUE;
    if (m_bViewSizeSetInStreamHeader)
    {
        bAsDefault = FALSE;
    }
    FormatAndSetViewFrame(m_pClipRect,
                          m_BitmapInfoHeader,
                          m_rViewRect,
                          TRUE, TRUE, bAsDefault);

    m_PreTransformBitmapInfoHeader = m_BitmapInfoHeader;

    m_bBitmapSet = ((m_BitmapInfoHeader.biWidth > 0) &&
                    (m_BitmapInfoHeader.biHeight > 0));

    return HXR_OK;
}


STDMETHODIMP
CVideoRenderer::DetachSite()
{
    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::DetachSite() Start", this);

    m_bSiteAttached = FALSE;

    HX_RELEASE(m_pMISUSSite);

    // We're done with these...
    if (m_pMISUS)
    {
        m_pMISUS->ReleaseSingleSiteUser();
    }

    HX_RELEASE(m_pMISUS);

    // Remove any pending callbacks, we don't need it anymore
    RemoveCallback(m_hPendingHandle);
    m_bPendingCallback = FALSE;

    HXLOGL3(HXLOG_BVID, "CVideoRenderer[%p]::DetachSite() End", this);

    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CVideoRenderer::NeedsWindowedSites()
{
    return FALSE;
};

STDMETHODIMP
CVideoRenderer::HandleEvent(HXxEvent* /*IN*/ pEvent)
{
    HX_RESULT retVal = HXR_OK;

    pEvent->handled = FALSE;
    pEvent->result  = 0;

    switch (pEvent->event)
    {
    case HX_SURFACE_UPDATE:
        m_pVSMutex->Lock();
	// If VS Mutex has been disrupted, this means it was done for
	// benefit of processing events.  However, UpdateDisplay
	// must not be called during this time to enforce atomic execution
	// of UpdateDisplay.
	// Since VSMutex disruption occurs only during VS2 initialization,
	// the update can be safely ignored as frame display will promptly
	// follow the initialization.
	if (!VSMutex_IsDisrupted())
	{
	    retVal = UpdateDisplay(pEvent, TRUE);
	}
        m_pVSMutex->Unlock();
        break;

#if defined(HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO)
    case HX_SURFACE_MODE_CHANGE:
        switch ((int) pEvent->param2)
        {
        case HX_VIDEOSURFACE1_RECOMMENDED:
            pEvent->result = SwitchToVideoSurface1();

            if (SUCCEEDED(pEvent->result))
            {
                pEvent->handled = TRUE;
            }
            break;

        case HX_VIDEOSURFACE1_NOT_RECOMMENDED:
            pEvent->result = SwitchToVideoSurface2();

            if (SUCCEEDED(pEvent->result))
            {
                pEvent->handled = TRUE;
            }
            break;

        default:
            HX_ASSERT(FALSE);
            break;
        }
        break;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    default:
	HXBOOL bUpdateDisplay = FALSE;

	retVal = HandleUserEvent(pEvent,
				 bUpdateDisplay);

	if ((retVal == HXR_OK) && bUpdateDisplay)
	{
	    m_pVSMutex->Lock();

	    m_bVSBufferUndisplayed = TRUE;

	    // If VS Mutex has been disrupted, this means it was done for
	    // benefit of processing events.  However, UpdateDisplay
	    // must not be called during this time to enforce atomic execution
	    // of UpdateDisplay.
	    // Since VSMutex disruption occurs only deing VS2 initialization,
	    // the update can be safely ignored as frame display will promptly
	    // follow the initialization.
	    if (!VSMutex_IsDisrupted())
	    {
		retVal = UpdateDisplay(pEvent, TRUE);
	    }

	    m_pVSMutex->Unlock();
	}

	break;
    }

    return retVal;
}

HX_RESULT CVideoRenderer::HandleUserEvent(HXxEvent* pEvent,
					  HXBOOL& bUpdateDisplay)
{
    // nothing to do
    return HXR_OK;
}

// *** IUnknown methods ***

/****************************************************************************
*  IUnknown::AddRef                                            ref:  hxcom.h
*
*  This routine increases the object reference count in a thread safe
*  manner. The reference count is used to manage the lifetime of an object.
*  This method must be explicitly called by the user whenever a new
*  reference to an object is used.
*/
STDMETHODIMP_(ULONG32) CVideoRenderer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/****************************************************************************
*  IUnknown::Release                                           ref:  hxcom.h
*
*  This routine decreases the object reference count in a thread safe
*  manner, and deletes the object if no more references to it exist. It must
*  be called explicitly by the user whenever an object is no longer needed.
*/
STDMETHODIMP_(ULONG32) CVideoRenderer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/****************************************************************************
*  IUnknown::QueryInterface                                    ref:  hxcom.h
*
*  This routine indicates which interfaces this object supports. If a given
*  interface is supported, the object's reference count is incremented, and
*  a reference to that interface is returned. Otherwise a NULL object and
*  error code are returned. This method is called by other objects to
*  discover the functionality of this object.
*/
STDMETHODIMP CVideoRenderer::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList  qiList[] =
    {
	{ GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this},
	{ GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*)this},
	{ GET_IIDHANDLE(IID_IHXUpdateProperties), (IHXUpdateProperties*)this},
	{ GET_IIDHANDLE(IID_IHXRenderTimeLine), (IHXRenderTimeLine*)this},
#if defined(HELIX_FEATURE_VIDREND_UNTIMED_DECODE)
        { GET_IIDHANDLE(IID_IHXUntimedRenderer), (IHXUntimedRenderer*)this},
#endif // defined(HELIX_FEATURE_VIDREND_UNTIMED_DECODE)
	{ GET_IIDHANDLE(IID_IHXPaceMakerResponse), (IHXPaceMakerResponse*)this},
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlugin*) this},
	{ GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*)this},
	{ GET_IIDHANDLE(IID_IHXRenderer), (IHXRenderer*)this},
	{ GET_IIDHANDLE(IID_IHXSiteUser), (IHXSiteUser*)this},
	{ GET_IIDHANDLE(IID_IHXStatistics), (IHXStatistics*)this},
        { GET_IIDHANDLE(IID_IHXMediaPushdown), (IHXMediaPushdown*)this}, 
        { GET_IIDHANDLE(IID_IHXFrameInfo), (IHXFrameInfo*)this}
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
        , { GET_IIDHANDLE(IID_IHXPlaybackVelocity), (IHXPlaybackVelocity*)this}
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    };

    if (QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj) == HXR_OK)
    {
        return HXR_OK ;
    }
    else if (m_pMISUS && IsEqualIID(riid, IID_IHXSiteUserSupplier))
    {
        return m_pMISUS->QueryInterface(IID_IHXSiteUserSupplier,ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        return HXR_NOINTERFACE;
    }
}


/****************************************************************************
 *  Renderer's customizable fuctions - can be called any time
 */
/****************************************************************************
 *  GetStreamVersion
 */
void CVideoRenderer::GetStreamVersion(ULONG32 &ulThisMajorVersion,
                                              ULONG32 &ulThisMinorVersion)
{
    ulThisMajorVersion = STREAM_MAJOR_VERSION;
    ulThisMinorVersion = STREAM_MINOR_VERSION;
}

/****************************************************************************
 *  GetContentVersion
 */
void CVideoRenderer::GetContentVersion(ULONG32 &ulThisMajorVersion,
                                               ULONG32 &ulThisMinorVersion)
{
    ulThisMajorVersion = CONTENT_MAJOR_VERSION;
    ulThisMinorVersion = CONTENT_MINOR_VERSION;
}

/****************************************************************************
 *  GetUpgradeMimeType
 */
const char* CVideoRenderer::GetUpgradeMimeType(void)
{
    const char** pStreamMimeTypes = NULL;
    UINT32 ulInitialGranularity;

    GetRendererInfo(pStreamMimeTypes, ulInitialGranularity);

    if (pStreamMimeTypes)
    {
        return pStreamMimeTypes[0];
    }

    return NULL;
}

/****************************************************************************
 *  GetRendererName
 */
const char* CVideoRenderer::GetRendererName(void)
{
    return BASE_VIDEO_RENDERER_NAME;
}

/****************************************************************************
 *  GetCodecName
 */
const char* CVideoRenderer::GetCodecName(void)
{
    return NULL;
}

/****************************************************************************
 *  GetCodecFourCC
 */
const char* CVideoRenderer::GetCodecFourCC(void)
{
    return NULL;
}

/****************************************************************************
 *  GetLateFrameTolerance
 */
ULONG32 CVideoRenderer::GetLateFrameTolerance(void)
{
    return LATE_FRAME_TOL;
}

/****************************************************************************
 *  GetEarlyFrameTolerance
 */
ULONG32 CVideoRenderer::GetEarlyFrameTolerance(void)
{
    return EARLY_FRAME_TOL;
}

/****************************************************************************
 *  GetMaxOptimizedVideoLead
 */
ULONG32 CVideoRenderer::GetMaxOptimizedVideoLead(void)
{
    return MAX_OPTIMIZED_VIDEO_LEAD;
}

/****************************************************************************
 *  GetBltPacketQueueSize
 */
ULONG32 CVideoRenderer::GetBltPacketQueueSize(void)
{
    ULONG32 ulSize = BLT_PACKET_QUEUE_SIZE;

    if (m_pVideoFormat)
    {
        m_pVideoFormat->GetMaxDecodedFrames();
    }

    return ulSize;
}

/****************************************************************************
 *  GetSyncGoalSmoothingDepth
 */
ULONG32 CVideoRenderer::GetSyncGoalSmoothingDepth(void)
{
    return SYNC_GOAL_SMOOTHING_DEPTH;
}

/****************************************************************************
 *  GetSpeedupGoalSmoothingDepth
 */
ULONG32 CVideoRenderer::GetSpeedupGoalSmoothingDepth(void)
{
    return SPEEDUP_GOAL_SMOOTHING_DEPTH;
}

/****************************************************************************
 *  GetNoFramesPollingInterval
 */
ULONG32 CVideoRenderer::GetNoFramesPollingInterval(void)
{
    return NO_FRAMES_POLLING_INTERVAL;
}

/****************************************************************************
 *  GetMaxSleepTime
 */
ULONG32 CVideoRenderer::GetMaxSleepTime(void)
{
    return m_ulMaxSleepTime;
}


/****************************************************************************
 *  GetMaxSleepTime
 */
ULONG32 CVideoRenderer::GetMaxBadSeqSamples(void)
{
    return MAX_BAD_SAMPLE_INTERVAL / m_ulSyncInterval;
}


/****************************************************************************
 *  GetDecodePriority
 */
LONG32 CVideoRenderer::GetDecodePriority(void)
{
    return m_lDecodePriority;
}

ULONG32 CVideoRenderer::GetLateFrameRebufferingTolerance()
{
    return DEFAULT_LATE_FRAME_REBUFFER_TOLERANCE;
}

HX_RESULT CVideoRenderer::SetDecodePriority(LONG32 lPriority)
{
    HX_RESULT retVal = HXR_OK;

    if (m_pDecoderPump)
    {
        retVal = m_pDecoderPump->SetPriority(lPriority);
    }

    if (SUCCEEDED(retVal))
    {
        m_lDecodePriority = lPriority;
    }

    return retVal;
}


/****************************************************************************
 *  CreateFormatObject
 */
CVideoFormat* CVideoRenderer::CreateFormatObject(IHXValues* pHeader)
{
    return new CVideoFormat(m_pCommonClassFactory,
                            this);
}

/****************************************************************************
 *  SetupBitmapDefaults
 */
void CVideoRenderer::SetupBitmapDefaults(IHXValues* pHeader,
                                         HXBitmapInfoHeader &bitmapInfoHeader)
{
    INT32 lWidth  = 0;
    INT32 lHeight = 0;
    
    if (pHeader)
    {
        // See if the view size is set from the
        // stream header. The view size is the
        // "Width" and "Height" properties.
        UINT32 ulTmp1 = 0;
        UINT32 ulTmp2 = 0;
	if (SUCCEEDED(pHeader->GetPropertyULONG32("Width",  ulTmp1)) &&
            SUCCEEDED(pHeader->GetPropertyULONG32("Height", ulTmp2)))
	{
            m_bViewSizeSetInStreamHeader = TRUE;
            m_StreamHeaderViewSize.cx = (INT32) ulTmp1;
            m_StreamHeaderViewSize.cy = (INT32) ulTmp2;
	}
        // Now check if the frame size is set from
        // the stream header. This is the "FrameWidth"
        // and "FrameHeight" properties.
        ulTmp1 = 0;
        ulTmp2 = 0;
	if (SUCCEEDED(pHeader->GetPropertyULONG32("FrameWidth",  ulTmp1)) &&
            SUCCEEDED(pHeader->GetPropertyULONG32("FrameHeight", ulTmp2)))
	{
            m_bFrameSizeSetInStreamHeader = TRUE;
            m_StreamHeaderFrameSize.cx = (INT32) ulTmp1;
            m_StreamHeaderFrameSize.cy = (INT32) ulTmp2;
	}
        // If the view size was set in the stream header, use that.
        // If the view size was not set in the stream header, but
        // the frame size was, then use the frame size.
        if (m_bViewSizeSetInStreamHeader)
        {
            lWidth  = m_StreamHeaderViewSize.cx;
            lHeight = m_StreamHeaderViewSize.cy;
        }
        else if (m_bFrameSizeSetInStreamHeader)
        {
            lWidth  = m_StreamHeaderFrameSize.cx;
            lHeight = m_StreamHeaderFrameSize.cy;
        }
    }

    // size calculation is taken from crvvideo
    bitmapInfoHeader.biSize          = sizeof (HXBitmapInfoHeader);
    bitmapInfoHeader.biWidth         = lWidth;
    bitmapInfoHeader.biHeight        = lHeight;
    bitmapInfoHeader.biPlanes        = 1;
    bitmapInfoHeader.biBitCount      = 24;
    bitmapInfoHeader.biCompression   = HX_I420;
    bitmapInfoHeader.biSizeImage     = (bitmapInfoHeader.biWidth *
                                        bitmapInfoHeader.biHeight *
                                        bitmapInfoHeader.biBitCount + 7) / 8;
    bitmapInfoHeader.biXPelsPerMeter = 0;
    bitmapInfoHeader.biYPelsPerMeter = 0;
    bitmapInfoHeader.biClrUsed       = 0;
    bitmapInfoHeader.biClrImportant  = 0;
    bitmapInfoHeader.rcolor          = 0;
    bitmapInfoHeader.gcolor          = 0;
    bitmapInfoHeader.bcolor          = 0;
}

/****************************************************************************
 *  FormatAndSetViewFrame
 */
void CVideoRenderer::FormatAndSetViewFrame(HXxRect* pClipRect,
                                           HXBitmapInfoHeader &bitmapInfoHeader,
                                           HXxRect &rViewRect,
                                           HXBOOL bMutex,
                                           HXBOOL bForceSyncResize,
                                           HXBOOL bAsDefault)
{
    HXxSize szViewFrame;

    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::FormatAndSetViewFrame() Start", this);

    if (bMutex)
    {
        DisplayMutex_Lock();
    }

    if (pClipRect)
    {
        rViewRect = *pClipRect;

        // Insure the ViewRect is inside the bitmap Rect
        // Clip x
        rViewRect.left = (rViewRect.left > 0) ?
            rViewRect.left : 0;
        rViewRect.right = (rViewRect.right > 0) ?
            rViewRect.right : 0;
        rViewRect.left = (rViewRect.left < bitmapInfoHeader.biWidth) ?
            rViewRect.left : bitmapInfoHeader.biWidth;
        rViewRect.right = (rViewRect.right < bitmapInfoHeader.biWidth) ?
            rViewRect.right : bitmapInfoHeader.biWidth;

        // Clip y
        rViewRect.top = (rViewRect.top > 0) ?
            rViewRect.top : 0;
        rViewRect.bottom = (rViewRect.bottom > 0) ?
            rViewRect.bottom : 0;
        rViewRect.top = (rViewRect.top < bitmapInfoHeader.biHeight) ?
            rViewRect.top : bitmapInfoHeader.biHeight;
        rViewRect.bottom = (rViewRect.bottom < bitmapInfoHeader.biHeight) ?
            rViewRect.bottom : bitmapInfoHeader.biHeight;
    }
    else
    {
        rViewRect.left = 0;
        rViewRect.top = 0;
        rViewRect.right = bitmapInfoHeader.biWidth;
        rViewRect.bottom = bitmapInfoHeader.biHeight;
    }

    // Compute Size
    szViewFrame.cx = rViewRect.right - rViewRect.left;
    szViewFrame.cy = rViewRect.bottom - rViewRect.top;

    if ((szViewFrame.cx <= 0) || (szViewFrame.cy <= 0))
    {
        if (m_pClipRect)
        {
            szViewFrame.cx = m_pClipRect->right - m_pClipRect->left;
            szViewFrame.cy = m_pClipRect->bottom - m_pClipRect->top;
        }

        if ((szViewFrame.cx <= 0) || (szViewFrame.cy <= 0))
        {
            szViewFrame.cx = DEFAULT_WIN_SIZE_X;
            szViewFrame.cy = DEFAULT_WIN_SIZE_Y;
        }
    }

    if (m_pClipRect)
    {
        bAsDefault = FALSE;
    }

    _ResizeViewFrame(szViewFrame, FALSE, bForceSyncResize, bAsDefault);

    if (bMutex)
    {
        DisplayMutex_Unlock();
    }

    HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::FormatAndSetViewFrame() End", this);
}


/****************************************************************************
 *  ResizeViewFrame
 */
HXBOOL CVideoRenderer::ResizeViewFrame(HXxSize szViewFrame,
                                     HXBOOL bMutex)
{
    HX_RESULT retVal;

    retVal = _ResizeViewFrame(szViewFrame,
                              bMutex,
                              FALSE,
                              FALSE);

    return retVal;
}

HXBOOL CVideoRenderer::_ResizeViewFrame(HXxSize szViewFrame,
                                      HXBOOL bMutex,
                                      HXBOOL bForceSyncResize,
                                      HXBOOL bAsDefault)
{
    HXBOOL bRetVal = FALSE;

    if (m_bFrameSizeInitialized)
    {
        return bRetVal;
    }

    if (bMutex)
    {
        DisplayMutex_Lock();
    }

    if (!m_bFrameSizeInitialized)
    {
        // If window size is already set, ignore further attempts to
        // resize
        if (m_bWinSizeFixed)
        {
            szViewFrame.cx = m_SetWinSize.cx;
            szViewFrame.cy = m_SetWinSize.cy;
        }

        // If resulting size invalid, default to cliprect or bitmap size
        if ((szViewFrame.cx <= 0) || (szViewFrame.cy <= 0))
        {
            if (((szViewFrame.cx <= 0) || (szViewFrame.cy <= 0)) &&
                m_pClipRect)
            {
                szViewFrame.cx = m_pClipRect->right - m_pClipRect->left;
                szViewFrame.cy = m_pClipRect->bottom - m_pClipRect->top;
            }

            if ((szViewFrame.cx <= 0) || (szViewFrame.cy <= 0))
            {
                szViewFrame.cx = m_BitmapInfoHeader.biWidth;
                szViewFrame.cy = m_BitmapInfoHeader.biHeight;
            }
        }

        if (!m_bWinSizeFixed ||
            (m_SetWinSize.cx != szViewFrame.cx) ||
            (m_SetWinSize.cy != szViewFrame.cy))
        {
            if(m_pHeader)
            {
                m_pHeader->SetPropertyULONG32("Width", (ULONG32) szViewFrame.cx);
                m_pHeader->SetPropertyULONG32("Height", (ULONG32) szViewFrame.cy);
            }
#if defined(HELIX_FEATURE_STATS)
            ReportStat(VS_IMAGE_WIDTH, (INT32) szViewFrame.cx);
            ReportStat(VS_IMAGE_HEIGHT, (INT32) szViewFrame.cy);
#endif /* #if defined(HELIX_FEATURE_STATS) */
        }

        m_SetWinSize.cx = szViewFrame.cx;
        m_SetWinSize.cy = szViewFrame.cy;

#if !defined(HELIX_FEATURE_VIDREND_DYNAMIC_RESIZE)
        m_bWinSizeFixed = (m_bWinSizeFixed || (!bAsDefault));
#else
        HX_ASSERT(!m_bWinSizeFixed);
#endif

#ifdef RESIZE_AFTER_SITE_ATTACHED
        if (m_bSiteAttached)
#endif  // RESIZE_AFTER_SITE_ATTACHED
        {
#ifdef SET_NONZERO_VIEWFRAME_ONLY
            if ((szViewFrame.cx > 0) && (szViewFrame.cy > 0))
#endif  // SET_NONZERO_VIEWFRAME_ONLY
            {
#ifdef SYNC_RESIZE_OK
                bForceSyncResize = TRUE;
#endif  // SYNC_RESIZE_OK

                if ((m_LastSetSize.cx != szViewFrame.cx) ||
                    (m_LastSetSize.cy != szViewFrame.cy))
                {
                    m_LastSetSize = szViewFrame;

                    if (bForceSyncResize)
                    {
                        m_pMISUSSite->SetSize(szViewFrame);
                    }
                    else
                    {
                        if (m_pResizeCB == NULL)
                        {
                            m_pResizeCB = new CSetSizeCB(m_pMISUSSite);

                            HX_ASSERT(m_pResizeCB);

                            if (m_pResizeCB)
                            {
                                m_pResizeCB->AddRef();
                            }
                        }

                        if (m_pResizeCB)
                        {
                            m_pResizeCB->SetSize(szViewFrame);

                            HX_ASSERT(m_pScheduler);

                            if (m_pScheduler)
                            {
                                m_pScheduler->RelativeEnter(m_pResizeCB, 0);
                            }
                        }
                    }
                }
            }

            // Once the the frame size is initialized, it is no longer
            // changable by the renderer.
            // The frame size can become initialzied only of the window
            // size is fixed. It can become fixed only if explicitly set
            // by non-default mode call to ResizeViewFrame.  ResizeViewFrame
            // can be called in non-default mode by either the video format
            // or the call can be made internally based on meta-header
            // specified information (e.g. clip rect.)
            if (m_bWinSizeFixed)
            {
                m_bFrameSizeInitialized = TRUE;
                bRetVal = TRUE;
            }
        }
    }

    if (bMutex)
    {
        DisplayMutex_Unlock();
    }

    return bRetVal;
}


/****************************************************************************
 *  SetSyncInterval
 */
void CVideoRenderer::SetSyncInterval(ULONG32 ulSyncInterval)
{
    if (ulSyncInterval != 0)
    {
        m_ulSyncInterval = ulSyncInterval;
    }
}

void CVideoRenderer::ChangePlayState(PlayState eState)
{
    m_PlayState = eState;
}

void CVideoRenderer::SetPlayStateStopped()
{
    ChangePlayState(Stopped);
}

/****************************************************************************
 *  InitExtraStats
 */
HX_RESULT CVideoRenderer::InitExtraStats(void)
{
    return HXR_OK;
}


/****************************************************************************
 *  Method:
 *    CVideoRenderer::InitVideoTransform
 *
 */
HX_RESULT CVideoRenderer::InitVideoTransform(HXBitmapInfoHeader& postTransformBIH,
					     const HXBitmapInfoHeader& preTransformBIH)
{
    postTransformBIH = preTransformBIH;

    return HXR_OK;
}

/****************************************************************************
 *  Method:
 *    CVideoRenderer::TransformVideo
 *
 */
HX_RESULT CVideoRenderer::TransformVideo(CMediaPacket* &pActiveVideoPacket,
					 HXxRect &destRect, 
					 HXxRect &sorcRect,
					 HXBOOL &bNewFrameBlt,
					 const HXBitmapInfoHeader& preTransformBIH,
					 const HXBitmapInfoHeader& postTransformBIH,
					 HXBOOL bOptimizedVideo)
{
    return HXR_OK;
}

/****************************************************************************
 *  Renderer's private fuctions
 */
HXBOOL CVideoRenderer::ForceRefresh(void)
{
    HXBOOL bIsVisible;

    // Ask what the size was really set to!
    HXxSize finalSize;
    m_pMISUSSite->GetSize(finalSize);

    /* We now damage the entire rect all the time             */
    HXxRect damageRect = {0, 0, finalSize.cx, finalSize.cy};

    // Mark associated screen area as damaged as well...
    m_pMISUSSite->DamageRect(damageRect);

    HX_ASSERT(!m_bPendingRedraw);
    HX_ASSERT(!m_bVS1UpdateInProgress);

    m_bPendingRedraw = TRUE;
    m_bVS1UpdateInProgress = TRUE;

    m_pVSMutex->Unlock();
    m_pMISUSSite->ForceRedraw();
    m_pVSMutex->Lock();
    m_bVS1UpdateInProgress = FALSE;

    // If the redraw is still pending, it did not occur so assume the
    // surface is invisible
    bIsVisible = !m_bPendingRedraw;
    m_bPendingRedraw = FALSE;

    return bIsVisible;
}

HX_RESULT CVideoRenderer::UpdateDisplay(HXxEvent* pEvent,
                                        HXBOOL bSystemEvent,
                                        HXBOOL bIsVisible)
{
    IHXVideoSurface* pVideoSurface;
    CMediaPacket* pTmpPacket;
    CMediaPacket* pVideoPacket = NULL;
    HXBOOL bVideoPacketLocalized = FALSE;
    HXBOOL bBitmapFormatChanged = FALSE;
    // If current buffer is not displayed, treat it like a new frame blt
    HXBOOL bNewFrameBlt = (m_bVSBufferUndisplayed && (!m_bFirstSurfaceUpdate));    
    HXxSize windowSize = {0, 0};
    LONG32 lFramesInBuffer;
    HX_RESULT retVal = HXR_OK;

    if (m_bVS1UpdateInProgress && m_bUseVideoSurface2)
    {
        // Ignore update for incorrect surface
        return HXR_OK;
    }

    lFramesInBuffer = m_pBltPacketQueue->Count();

    m_bPendingRedraw = FALSE;

    HX_TRACE_THINGY("C:/trace.txt", "Handle", m_hPendingHandle);

    if (!m_bUseVideoSurface2 || !bSystemEvent)
    {
        do
        {
            pVideoPacket = (CMediaPacket*) m_pBltPacketQueue->Get();
            lFramesInBuffer--;

            if ((lFramesInBuffer <= 0) ||
                (!m_pVideoFormat->IsFrameSkippable(pVideoPacket)))
            {
                break;
            }

#if defined(HELIX_FEATURE_STATS)
            m_pVideoStats->ReportDroppedFrame();
#endif /* HELIX_FEATURE_STATS */

            ReleasePacket(pVideoPacket);
        } while (TRUE);
    }

    pVideoSurface = (IHXVideoSurface*) (pEvent->param1);

    if (pVideoPacket)
    {
        bNewFrameBlt = TRUE;

	HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::UpdateDisplay GotNewFrame: PTS=%lu", this,
	        pVideoPacket->m_ulTime);

        if (m_bVSBufferUndisplayed && m_pActiveVideoPacket)
        {
            m_bVSBufferUndisplayed = FALSE;

#if defined(HELIX_FEATURE_STATS)
            m_pVideoStats->ReportDroppedFrame();
            m_pVideoStats->ReportFailedBlt();
#endif /* HELIX_FEATURE_STATS */
        }

        m_bVS2BufferUnavailableOnLastBlt = FALSE;

	pTmpPacket = pVideoPacket;
	pVideoPacket = m_pActiveVideoPacket;
	bVideoPacketLocalized = m_bActiveVideoPacketLocalized;
	m_pActiveVideoPacket = pTmpPacket;
	m_bActiveVideoPacketLocalized = FALSE;

	bBitmapFormatChanged = (m_pActiveVideoPacket->m_pSampleDesc &&
				m_pVideoFormat->IsBitmapFormatChanged(
					m_PreTransformBitmapInfoHeader,
					m_pActiveVideoPacket));
    }

    if (((!m_bVideoSurfaceInitialized) && m_pActiveVideoPacket) ||
        m_bVideoSurfaceReinitRequested ||
        bBitmapFormatChanged)
    {
        HXBOOL bUsedVideoSurface2 = m_bUseVideoSurface2;

	if (bBitmapFormatChanged)
	{
	    m_bBitmapSet = FALSE;

	    if (m_pVideoFormat->InitBitmapInfoHeader(m_PreTransformBitmapInfoHeader,
						     m_pActiveVideoPacket) == HXR_OK)
	    {
		if (InitVideoTransform(m_BitmapInfoHeader,
				       m_PreTransformBitmapInfoHeader) == HXR_OK)
		{
		    m_bBitmapSet = TRUE;
		}
	    }

	    m_bVideoSurfaceInitialized = FALSE;	// Force full reinit
	}

	FormatAndSetViewFrame(m_pClipRect,
			      m_BitmapInfoHeader,
			      m_rViewRect,
			      FALSE,	    // We already have display mutex
			      bSystemEvent);// Can do sync resize on system events

#if defined(HELIX_FEATURE_STATS)
        ReportStat(VS_CODEC_FRAME_WIDTH, (INT32) (m_rViewRect.right - m_rViewRect.left));
        ReportStat(VS_CODEC_FRAME_HEIGHT, (INT32) (m_rViewRect.bottom - m_rViewRect.top));
#endif /* #if defined(HELIX_FEATURE_STATS) */

        if (!CheckVideoAreaSize(TRUE))
        {
            retVal = HXR_ABORT;
        }

        HX_ASSERT(m_bBitmapSet);
        
        // If we do not have information on the bitmap configuration or if
        // we do not have surface to cofigure - do not intialize
        if ((retVal == HXR_OK) && m_bBitmapSet && pVideoSurface)
        {
            // Try VideoSurface2 first unless not desired
            if (m_bTryVideoSurface2 && (!m_bVideoSurface1Requested))
            {
		if (!bSystemEvent)
                {
		    // Disruption of VSMutex allows Handle Event to acquire
		    // VSMutex and thus handle events and yet enforce
		    // UpdateDisplay to be called atomically.
		    VSMutex_Disrupt();
		}
		
		InitVideoSurface2(m_SetWinSize.cx, m_SetWinSize.cy);

		if (!bSystemEvent)
		{
		    VSMutex_Continue();

		    // During VSMutex disruption, it is possible
		    // that notion of display status of current VSBuffer
		    // has changed.  This may occur if video transform
		    // upon reception of keyboard, mouse or other event
		    // changes the image during VSMutex disruption period.
		    // Whenever such change of underlaying image occurs,
		    // we need to treat the current frame as a new one.
		    if (m_bVSBufferUndisplayed)
		    {
			bNewFrameBlt = TRUE;
		    }
		}
            }

            // Try VideoSurface1 if prefered or if VideoSurface2 would
            // not initialize
            if ((!m_bUseVideoSurface2) || m_bVideoSurface1Requested)
            {
                // Video Surface1 must be used
                if (bSystemEvent)
                {
                    InitVideoSurface1(bUsedVideoSurface2, pVideoSurface);
                }
                else
                {
                    // This is not a system event and this call is not under
                    // the protection of the top level site (TLS) mutex and
                    // it needs to be.
                    // Bring it under the TLS mutex protection by envoking
                    // forced refresh.

		    // The buffer may have not been displayed in this update
		    m_bVSBufferUndisplayed = bNewFrameBlt; 

                    ForceRefresh();

		    // Since we have handed off the update by setting 
		    // m_bVSBufferUndisplayed (and envoking ForceRefresh()),
		    // proceed outside of new frame handling mode since subsequent 
		    // update will take into account the undisplayed new frame.
		    bNewFrameBlt = FALSE;

                    pEvent->result = HXR_OK;
                    retVal = HXR_ABORT;
                }
            }

            if (retVal == HXR_OK)
            {
                m_bVideoSurfaceInitialized = TRUE;
                m_bVideoSurfaceReinitRequested = FALSE;

                m_bSchedulerStartRequested = TRUE;


                // Force the cycle in the decoder pump to expedite the completion
                // of the scheduler start request.
                if (IsDecoderRunning())
                {
                    if (m_pDecoderPump)
                    {
                        m_pDecoderPump->Signal();
                    }
                }
            }
        }  
        
        if (retVal != HXR_OK)
        {
            // Since init was abnormal, proceed as if the surface is invisible
            bIsVisible = FALSE;
        }
    }
   
    /*
     * If there is a media packet to display, the media packet is meant 
     * to be visible, the display is visible and video surface exists,
     * proceed to display the media packet (which contains a video frame).
    */
    if (m_pActiveVideoPacket &&
        !(m_pActiveVideoPacket->m_ulFlags & MDPCKT_IS_INVISIBLE))
    {
	m_ulActiveVideoTime = m_pActiveVideoPacket->m_ulTime;

	if (pVideoSurface && bIsVisible)    // display is visible
	{
	    HX_ASSERT(m_bVideoSurfaceInitialized);
	    
	    if (m_bVideoSurfaceInitialized && m_pActiveVideoPacket->m_pData)
	    {
#ifdef DEFAULT_VS2_TARGETRECT
		if (!m_bUseVideoSurface2)
#endif	// DEFAULT_VS2_TARGETRECT
		{
		    m_pMISUSSite->GetSize(windowSize);
		}
		
		HXxRect rDestRect = { 0, 0, windowSize.cx, windowSize.cy };
		HXxRect rSrcRect = m_rViewRect;
		
		pEvent->result = TransformVideo(m_pActiveVideoPacket,
					        rDestRect, 
						rSrcRect,
						bNewFrameBlt,
						m_PreTransformBitmapInfoHeader,
						m_BitmapInfoHeader,
						m_bUseVideoSurface2);
		
		if ((pEvent->result == HXR_OK) &&
		    m_pActiveVideoPacket->m_pData)
		{
		    if (m_bUseVideoSurface2)
		    {
			pEvent->result = UpdateVideoSurface2(pVideoSurface,
							     rDestRect,
							     rSrcRect,
							     bSystemEvent,
							     bNewFrameBlt);
			
			if (pEvent->result == (UINT32)HXR_ABORT)
			{
			    pEvent->result = HXR_OK;
			    retVal = HXR_ABORT;
			}
		    }
		    else if (bSystemEvent)
		    {
			pEvent->result = UpdateVideoSurface(
					    pVideoSurface,
					    m_pActiveVideoPacket,
					    rDestRect,
					    rSrcRect,
					    m_bOptimizedBlt);
			
			if (FAILED(pEvent->result))
			{
			    if (m_bVideoSurfaceReinitRequested)
			    {
				HX_ASSERT(!m_bVideoSurface1Requested);
				
				InitVideoSurface2(m_SetWinSize.cx, 
						  m_SetWinSize.cy);
				
				if (m_bUseVideoSurface2)
				{
				    // If this is just a refresh frame, we cannot treat it as 
				    // a new frame if it is a first surface update since
				    // first surface update implies a new frame sequence
				    // and we should start a new sequence only with a new
				    // frame.
				    HXBOOL bNewFrameBltForVS2 = 
					(bNewFrameBlt || (!m_bFirstSurfaceUpdate));
				    
				    m_bVideoSurfaceInitialized = TRUE;
				    m_bVideoSurfaceReinitRequested = FALSE;
				    
				    pEvent->result = UpdateVideoSurface2(
							pVideoSurface,
							rDestRect,
							rSrcRect,
							bSystemEvent,
							bNewFrameBltForVS2);
				    
				    if (pEvent->result == (UINT32)HXR_ABORT)
				    {
					pEvent->result = HXR_OK;
				    }
				}
			    }
			    else
			    {
				InitVideoSurface1(FALSE, pVideoSurface);
				
				pEvent->result = UpdateVideoSurface(
						    pVideoSurface,
						    m_pActiveVideoPacket,
						    rDestRect,
						    rSrcRect,
						    m_bOptimizedBlt);
			    }
			}
		    }
		    else
		    {
			// This is not a system event and this call is not under
			// the protection of the top level site (TLS) mutex and
			// it needs to be.
			// Bring it under the TLS mutex protection by envoking
			// forced refresh.
			
			// The buffer was not displayed in this update
			m_bVSBufferUndisplayed = TRUE; 
			
			ForceRefresh();
			
			// Since we have handed off the update by setting 
			// m_bVSBufferUndisplayedForceRefresh (and envoking ForceRefresh()),
			// proceed outside of new frame handling mode since subsequent 
			// update will take into account the undisplayed new frame.
			bNewFrameBlt = FALSE;	
			
			pEvent->result = HXR_OK;
			retVal = HXR_ABORT;
		    }
		}
		
		if (bNewFrameBlt)
		{
		    if (pEvent->result == HXR_OK)
		    {
			m_bVSBufferUndisplayed = FALSE;
			
#if defined(HELIX_FEATURE_STATS)
			m_pVideoStats->ReportBlt(m_pActiveVideoPacket->m_ulTime);
#endif /* HELIX_FEATURE_STATS */
		    }
		    else
		    {
			// We might still display the frame with some 
			// later refresh event
			m_bVSBufferUndisplayed = TRUE;
		    }
		}
	    }
	}
	else
	{
	    if (bNewFrameBlt)
	    {
		m_bVSBufferUndisplayed = TRUE;
	    }
	}
    }

    if (pVideoPacket)
    {
        ReleasePacket(pVideoPacket, bVideoPacketLocalized);
    }

    pEvent->handled = TRUE;

    return retVal;
}


HXBOOL CVideoRenderer::CheckVideoAreaSize(HXBOOL bReportErrorMsg)
{
    HXBOOL bAreaOK = TRUE;

	INT32 nWidth = (INT32) (m_rViewRect.right - m_rViewRect.left);
    INT32 nHeight = (INT32) (m_rViewRect.bottom - m_rViewRect.top);
	if (nWidth < 0)
    {
       nWidth = -nWidth;
    }
    if (nHeight < 0)
    {
       nHeight = -nHeight;
    }
    
	if(m_pHeader)
	{
		m_pHeader->SetPropertyULONG32("FrameWidth",(ULONG32) nWidth);
		m_pHeader->SetPropertyULONG32("FrameHeight",(ULONG32) nHeight);
	}

    // Now that we have the video dimensions, check to see that this clip
    // is not going to be too taxing on the poor CPU.
    if (m_pPreferences)
    {
        if (m_ulMaxVidArea == 0)
        {        
            UINT32 ulMaxVidWidth = 0;
            UINT32 ulMaxVidHeight = 0;
            
            ReadPrefUINT32(m_pPreferences, "MaxVideoWidth", ulMaxVidWidth);
            ReadPrefUINT32(m_pPreferences, "MaxVideoHeight", ulMaxVidHeight);
            
            m_ulMaxVidArea = ulMaxVidWidth * ulMaxVidHeight;
            
            if (m_ulMaxVidArea == 0)
            {
                m_ulMaxVidArea = UNBOUND_VIDEO_AREA;
            }
        }
        
        if (((ULONG32) (nWidth*nHeight)) > m_ulMaxVidArea)
        {
            bAreaOK = FALSE;

            if (bReportErrorMsg)
            {
                IHXErrorMessages * pErrMsg = NULL;
                m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&pErrMsg);
                if (pErrMsg)
                {
                    pErrMsg->Report(HXLOG_ERR, HXR_SLOW_MACHINE, 0, NULL, NULL);
                    HX_RELEASE(pErrMsg);
                }
            }
        }
    }

    return bAreaOK;
}


HX_RESULT CVideoRenderer::UpdateVideoSurface(IHXVideoSurface* pVideoSurface,
                                             CMediaPacket* pVideoPacket,
                                             HXxRect &destRect,
                                             HXxRect &sorcRect,
                                             HXBOOL bOptimizedBlt)
{
    HXLOGL4(HXLOG_BVID, "VS1 Blt Frame ts=%lu", pVideoPacket->m_ulTime);
    
    HX_RESULT retVal;

    if (bOptimizedBlt)
    {
        retVal = pVideoSurface->OptimizedBlt(
            pVideoPacket->m_pData,
            destRect,
            sorcRect);
    }
    else
    {
        retVal = pVideoSurface->Blt(
            pVideoPacket->m_pData,
            &m_BitmapInfoHeader,
            destRect,
            sorcRect);
    }
    if (SUCCEEDED(retVal))
    {
        m_bFirstSurfaceUpdate = FALSE;
    }


    return retVal;
}


HX_RESULT CVideoRenderer::InitVideoSurface1(HXBOOL bUsedVideoSurface2,
                                            IHXVideoSurface* pVideoSurface)
{
    HX_RESULT retVal = HXR_OK;

    HXLOGL4(HXLOG_BVID, "CVideoRenderer::InitVideoSurface1 Start");

    if (bUsedVideoSurface2)
    {
        OffOptimizedVideo();
    }

    /***
    if (pVideoSurface)
    {
        retVal = pVideoSurface->EndOptimizedBlt();
    }
    else
    {
        EndOptimizedBlt();
    }
    ***/

    // Video Surface1 is needed - try optimized Blt setup
    if (!m_bVideoSurfaceInitialized)
    {
        if (pVideoSurface)
        {
            retVal = pVideoSurface->BeginOptimizedBlt(&m_BitmapInfoHeader);
        }
        else
        {
            retVal = BeginOptimizedBlt(&m_BitmapInfoHeader);
        }

        if (SUCCEEDED(retVal))
        {
            m_bOptimizedBlt = TRUE;
        }
        else
        {
            m_bOptimizedBlt = FALSE;
        }

        retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
        m_ulEarlyFrameTol = GetEarlyFrameTolerance();
        HX_DELETE(m_pVSurf2InputBIH);
        m_bUseVideoSurface2 = FALSE;
        m_bVS2BufferUnavailableOnLastBlt = FALSE;
    }

    ReportStat(VS_SURFACE_MODE, (INT32) 1);

    HXLOGL4(HXLOG_BVID, "CVideoRenderer::InitVideoSurface1 End");

    return retVal;
}


inline HXBOOL CVideoRenderer::ShouldKickStartScheduler()
{
    HXBOOL bShouldStart;

    bShouldStart = (!m_bPendingCallback && !m_bUseVideoSurface2);

    return bShouldStart;
}


HX_RESULT CVideoRenderer::ScheduleCallback(UINT32 ulRelativeTime,
                                           HXBOOL bIsScheduled,
                                           UINT32 ulBaseTime)
{
    IHXCallback* pCallback = (IHXCallback*) this;
    CallbackHandle hPendingHandle;
    ULONG32 ulThisCallbackCount;

    if (m_hPendingHandle == (CallbackHandle)NULL)
    {
        m_bPendingCallback = TRUE;
        m_ulCallbackCounter++;
        if (m_ulCallbackCounter == 0)
        {
            m_ulCallbackCounter++;
        }
        ulThisCallbackCount = m_ulCallbackCounter;
        m_hPendingHandle = ulThisCallbackCount;
        m_bIsScheduledCB = bIsScheduled;

        if (ulRelativeTime > m_ulMaxSleepTime)
        {
            ulRelativeTime = m_ulMaxSleepTime;
            m_bIsScheduledCB = FALSE;
        }

        HX_TRACE_THINGY("C:/trace.txt", "SchedCBTime", ulRelativeTime);

#ifdef DO_ABSOLUTE_TIMING
        LONG32 lTimeOffset;
        HXTimeval hxTime;
        ULONG32 ulNTries = N_STABILIZATION_ITERATIONS;
        ULONG32 ulCurrentTime1;
        ULONG32 ulCurrentTime2;

        if (bIsScheduled)
        {
            // Obtain atomic time reading (avoid context switch)
            do
            {
                ulCurrentTime1 = HX_GET_BETTERTICKCOUNT();

                // Always use the optimized scheduler if we have one
                if (m_pOptimizedScheduler != NULL)
                {
                    hxTime = m_pOptimizedScheduler->GetCurrentSchedulerTime();
                }
                else
                {
                    hxTime = m_pScheduler->GetCurrentSchedulerTime();
                }

                ulCurrentTime2 = HX_GET_BETTERTICKCOUNT();

                lTimeOffset = (LONG32) (ulCurrentTime2 - ulBaseTime + ulRelativeTime);

                ulNTries--;
            } while (((ulCurrentTime2 - ulCurrentTime1) > MAX_ALLOWED_TIMING_ERROR) &&
                (ulNTries != 0) &&
                (lTimeOffset > 0));

            if (lTimeOffset >= SMALLEST_TIMABLE_PERIOD)
            {
                hxTime.tv_usec += (lTimeOffset * MILLISECOND);
                if (hxTime.tv_usec >= SECOND)
                {
                    hxTime.tv_sec += (hxTime.tv_usec / SECOND);
                    hxTime.tv_usec %= SECOND;
                }
            }

            hPendingHandle = ScheduleAbsoluteCallback(hxTime, pCallback);
        }
        else
#endif  // DO_ABSOLUTE_TIMING
        {

            hPendingHandle = ScheduleRelativeCallback(ulRelativeTime, pCallback);

        }

        HX_ASSERT(hPendingHandle);

        // Remember the handle if callback did not already fire synchronously
        if (m_hPendingHandle == ulThisCallbackCount)
        {
            HX_TRACE_THINGY("C:/trace.txt", "SchedCBHandle", hPendingHandle);

            m_hPendingHandle = hPendingHandle;
            m_bPendingCallback = (m_hPendingHandle != ((CallbackHandle) NULL));
        }
    }

    return HXR_OK;
}


inline CallbackHandle CVideoRenderer::ScheduleRelativeCallback
(
    UINT32 ulRelativeTime,
    IHXCallback* pCallback
)
{
    CallbackHandle hCallback;

    // Always use the optimized scheduler if we have one
    if (m_pOptimizedScheduler != NULL)
    {
        hCallback = m_pOptimizedScheduler->RelativeEnter(
            pCallback, ulRelativeTime);
    }
    else
    {
        hCallback = m_pScheduler->RelativeEnter(
            pCallback, ulRelativeTime);
    }

    return hCallback;
}


inline CallbackHandle CVideoRenderer::ScheduleAbsoluteCallback
(
    HXTimeval &hxTime,
    IHXCallback* pCallback
)
{
    CallbackHandle hCallback;

    // Always use the optimized scheduler if we have one
    if (m_pOptimizedScheduler != NULL)
    {
        hCallback = m_pOptimizedScheduler->AbsoluteEnter(
            pCallback, hxTime);
    }
    else
    {
        hCallback = m_pScheduler->AbsoluteEnter(
            pCallback, hxTime);
    }

    return hCallback;
}


void CVideoRenderer::RemoveCallback(CallbackHandle &hCallback)
{
    if ((!m_bUntimedRendering) && (hCallback != (CallbackHandle)NULL))
    {
        if (m_pOptimizedScheduler != NULL)
        {
            m_pOptimizedScheduler->Remove(hCallback);
        }
        else if (m_pScheduler)
        {
            m_pScheduler->Remove(hCallback);
        }

        hCallback = NULL;
    }
}


/*
 *   Draw any frame that's due, and schedule a new callback for the next
 *   frame
 */
void CVideoRenderer::SchedulerCallback(HXBOOL bIsScheduled,
                                       HXBOOL bResched,
                                       HXBOOL bIsVS2Call,
                                       HXBOOL bProcessUndisplayableFramesOnly)
{
    CMediaPacket* pPkt;
    LONG32 lTimeDelta = 0;
    ULONG32 ulNextFrameTime;
    HXBOOL bFrameIsSkippable;
    ULONG32 ulBaseTime;
    HXBOOL bHaveNextFrame;
    HXBOOL bDisplayFrame;
    ULONG32 ulLoopCounter = 0;

    DisplayMutex_Lock();

    while ((m_PlayState == Playing) ||
           ((m_PlayState == PlayStarting) && m_bFirstFrame) ||
	   ((m_PlayState == Stopped) && m_bPreSeekPointFrames))
    {
        if (m_bUseVideoSurface2)
        {
            if (bIsVS2Call)
            {
                m_bVideoSurface2Transition = FALSE;
            }
            else
            {
                m_bPendingCallback = FALSE;
                DisplayMutex_Unlock();
                return;
            }
        }
        else
        {
            if (bIsVS2Call)
            {
                DisplayMutex_Unlock();
                return;
            }

            m_bVS2BufferUnavailableOnLastBlt = FALSE;
        }

#ifdef ENABLE_FETCH_TRACE
        ULONG32 ulFetchTraceStart = HX_GET_BETTERTICKCOUNT();
#endif  // ENABLE_FETCH_TRACE

	bDisplayFrame = FALSE;

        while ((bHaveNextFrame = m_pVideoFormat->GetNextFrameTime(ulNextFrameTime)) != 0)
        {

#ifdef ENABLE_FETCH_TRACE
            if (ulFetchTraceIdx < MAX_FETCH_TRACE_ENTRIES)
            {
                fetchTraceArray[ulFetchTraceIdx++] =
                    HX_GET_BETTERTICKCOUNT() - ulFetchTraceStart;
            }
#endif  // ENABLE_FETCH_TRACE

            lTimeDelta = ComputeTimeAhead(ulNextFrameTime,
                                          0,
                                          &ulBaseTime);

	    INT32 lLateFrameTolerance = ((m_PlayState == Playing) ? m_ulLateFrameTol : 0);

	    if ((m_ulAbsPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL) && 
		(m_ulAbsPlaybackVelocity != 0))
	    {
		lTimeDelta = ((lTimeDelta * HX_PLAYBACK_VELOCITY_NORMAL) / 
			      ((INT32) m_ulAbsPlaybackVelocity));
	    }

	    bDisplayFrame = ((lTimeDelta + lLateFrameTolerance) >= 0);

	    HXLOGL4(HXLOG_BVID, "CVideoRenderer::SchedulerCallback() Found frame: PTS=%lu TimeAhead=%ld bIsScheduled=%c bResched=%c bIsVS2Call=%c bDisplayFrame=%c m_bFirstFrame=%c bProcessUndisplayableFramesOnly=%c VS2UnavailOnLastBlt=%c decqdepth=%lu",
		    ulNextFrameTime,
		    lTimeDelta,
		    bIsScheduled ? 'T' : 'F',
		    bResched ? 'T' : 'F',
		    bIsVS2Call ? 'T' : 'F',
		    bDisplayFrame ? 'T' : 'F',
		    m_bFirstFrame ? 'T' : 'F',
		    bProcessUndisplayableFramesOnly ? 'T' : 'F',
		    m_bVS2BufferUnavailableOnLastBlt ? 'T' : 'F',
		    m_pVideoFormat ? m_pVideoFormat->GetDecodedFrameQueueDepth() : 0);

	    // We do not service m_bVS2BufferUnavailableOnLastBlt in first surface update as
	    // entirely new frame is needed to start the first surface update.
	    // Doing otherwise would mean possibly re-blting the frame from a prior
	    // sequence (before seek).
            if (bDisplayFrame ||
                bIsScheduled ||
                (m_bVS2BufferUnavailableOnLastBlt && (!m_bFirstSurfaceUpdate)))
            {
		if (bDisplayFrame)
		{
		    // If we are in stopped state and are just processing pre-seek frames
		    // kick out of the loop once we reach the frame that is to be
		    // displayed.
		    if (m_bPreSeekPointFrames && (m_PlayState == Stopped))
		    {
			m_bPreSeekPointFrames = FALSE;
			DisplayMutex_Unlock();
			return;
		    }
		    m_bPreSeekPointFrames = FALSE;
		}
		bDisplayFrame = TRUE;
                break;
            }
            else
            {
#ifdef ENABLE_SCHED_TRACE
                if (ulSchedTraceIdx < MAX_SCHED_TRACE_ENTRIES)
                {
                    schedTraceArray[ulSchedTraceIdx++] = lTimeDelta;
                }
#endif  // ENABLE_SCHED_TRACE

                bFrameIsSkippable = TRUE;

                m_pVideoFormat->IsNextFrameSkippable(bFrameIsSkippable);

                if (!bFrameIsSkippable)
                {
                    if (m_bFirstFrame)
                    {
                        bDisplayFrame = FALSE;
                    }
                    break;
                }
            }

#ifdef ENABLE_FETCH_TRACE
            ulFetchTraceStart = HX_GET_BETTERTICKCOUNT();
#endif  // ENABLE_FETCH_TRACE

            pPkt = m_pVideoFormat->Dequeue();

#ifdef ENABLE_FETCH_TRACE
            if (ulFetchTraceIdx < MAX_FETCH_TRACE_ENTRIES)
            {
                fetchTraceArray[ulFetchTraceIdx++] =
                    HX_GET_BETTERTICKCOUNT() - ulFetchTraceStart;
            }
#endif  // ENABLE_FETCH_TRACE

            if (!m_bFirstFrame)
            {
#if defined(HELIX_FEATURE_STATS)
                m_pVideoStats->ReportDroppedFrame();
#endif /* HELIX_FEATURE_STATS */
            }

	    HXLOGL3(HXLOG_BVID, "CVideoRenderer::SchedulerCallback() Dropping Frame: ulNextFrameTime=%lu lTimeDelta=%ld lLateFrameTolerance=%ld DROPPING DECODED FRAME",
                    ulNextFrameTime, lTimeDelta, lLateFrameTolerance);

            ReleasePacket(pPkt);
        }

        if (bProcessUndisplayableFramesOnly && bDisplayFrame)
        {
	    HX_ASSERT(bIsVS2Call);
            DisplayMutex_Unlock();
            return;
        }

        if (!m_bVS2BufferUnavailableOnLastBlt)
        {
            if (bHaveNextFrame)
            {
                if ((lTimeDelta > ((LONG32) m_ulEarlyFrameTol)) &&
                    (!m_bFirstFrame) &&
		    (!m_bUntimedRendering))
                {
                    if (bResched)
                    {
                        ScheduleCallback(lTimeDelta, TRUE, ulBaseTime);
                    }

                    DisplayMutex_Unlock();

		    if (m_pVideoFormat && m_pVideoFormat->IsFillbackDecodeNeeded())
		    {
			m_pVideoFormat->DecodeFrame();
		    }

                    return;
                }
            }
            else
            {
                CheckForBufferingStart();            
                if (bResched)
                {
                    ScheduleCallback(m_ulNoFramesPollingInterval);
                }

                DisplayMutex_Unlock();

		if (m_pVideoFormat && m_pVideoFormat->IsFillbackDecodeNeeded())
		{
		    m_pVideoFormat->DecodeFrame();
		}

                return;
            }

            // Render
            pPkt = m_pVideoFormat->Dequeue();
        }
        else
        {
            // We need to reblt - but if there is a frame that's not 
	    // too early to display or non-displayable frame, use it 
	    // instead of old failed frame.
            pPkt = NULL;
            if (bHaveNextFrame && 
		((lTimeDelta > ((LONG32) m_ulEarlyFrameTol)) || (!bDisplayFrame)))
            {
                pPkt = m_pVideoFormat->Dequeue();
            }
        }

        if (pPkt != NULL)
        {
#ifdef ENABLE_SCHED_TRACE
            if (ulSchedTraceIdx < MAX_SCHED_TRACE_ENTRIES)
            {
                schedTraceArray[ulSchedTraceIdx++] = lTimeDelta;
            }
#endif  // ENABLE_SCHED_TRACE

            if (!bDisplayFrame)
            {
                pPkt->m_ulFlags |= MDPCKT_IS_INVISIBLE;
            }

            if (m_pMISUSSite &&
                m_pBltPacketQueue->Put(pPkt))
            {
                if (pPkt->m_pData && bDisplayFrame && m_bFirstFrame)
                {
                    HXLOGL2(HXLOG_BVID, "Displaying first frame");
                    m_bFirstFrame = FALSE;
                }

                if (m_bUseVideoSurface2)
                {
                    ForceDisplayUpdate(FALSE, bDisplayFrame);

                    if (m_bVS2BufferUnavailableOnLastBlt)
                    {
                        // We couldn't blt frame because video buffer
                        // wasn't available, try again a bit later
                        DisplayMutex_Unlock();

                        return;
                    }
                }
                else
                {
                    if ((!bDisplayFrame) || (!ForceRefresh()))
                    {
                        // Site redraw did not occur - treat this as if the surface
                        // is invisible
                        ForceDisplayUpdate(FALSE, FALSE);
                        HX_ASSERT(!m_bPendingRedraw);
                    }
                }
            }
            else
            {
#if defined(HELIX_FEATURE_STATS)
                m_pVideoStats->ReportDroppedFrame();
#endif /* HELIX_FEATURE_STATS */
                ReleasePacket(pPkt);
            }
        }
        else
        {
            // This is a reblt servicing
            if (m_bUseVideoSurface2)
            {
                ForceDisplayUpdate(FALSE, TRUE);

                if (m_bVS2BufferUnavailableOnLastBlt)
                {
                    // We couldn't blt frame because video buffer
                    // wasn't available, try again a bit later
                    DisplayMutex_Unlock();
                    return;
                }
            }
        }

        if (bResched)
        {
            ulLoopCounter++;

            if (ulLoopCounter > MAX_BLT_LOOPS)
            {
                ScheduleCallback(BLT_RELIEF_DELAY);

                DisplayMutex_Unlock();

                return;
            }
        }
        else
        {
            if (!bIsVS2Call)
            {
                DisplayMutex_Unlock();

                return;
            }
        }

        bIsScheduled = FALSE;
    }

    if (((m_PlayState != Stopped) || m_bPreSeekPointFrames) &&
        bResched)
    {
        CheckForBufferingStart();
        ScheduleCallback(NO_FRAMES_POLLING_INTERVAL);

        DisplayMutex_Unlock();

	if (m_pVideoFormat && 
	    m_pVideoFormat->IsFillbackDecodeNeeded() && 
	    (!m_bServicingFillbackDecode))
	{
	    m_bServicingFillbackDecode = TRUE;
	    m_pVideoFormat->DecodeFrame();
	    m_bServicingFillbackDecode = FALSE;
	}

        return;
    }

    m_bPendingCallback = FALSE;

    DisplayMutex_Unlock();
}

void CVideoRenderer::ForceDisplayUpdate(HXBOOL bInternalSurfaceUpdateOnly,
                                        HXBOOL bHasVisibleSurface)
{
    // Create fake events for HandleSurfaceUpdate:
    HX_ASSERT(m_pMISUSSite);

#if defined (HELIX_FEATURE_MISU)
    IHXSiteEnumerator* pSiteEnumerator;
    if (SUCCEEDED(m_pMISUSSite->QueryInterface(IID_IHXSiteEnumerator,
                                               (void**) &pSiteEnumerator)))
    {
        IHXSite* pSite;
        IHXSiteEnumerator::SitePosition nextPosition;

        if (FAILED(pSiteEnumerator->GetFirstSite(pSite, nextPosition)))
        {
            HX_ASSERT(FALSE);
        }
        else
        {
            HXBOOL bKeepUpdating = TRUE;

            do
            {
                bKeepUpdating = (ForceDisplayUpdateOnSite(pSite, 
                                                          bInternalSurfaceUpdateOnly, 
                                                          bHasVisibleSurface) != HXR_ABORT);
                pSite->Release();
            }
            while (bKeepUpdating &&
                   SUCCEEDED(pSiteEnumerator->GetNextSite(pSite, nextPosition)));
        }

        pSiteEnumerator->Release();
    }
    else
    {
        ForceDisplayUpdateOnSite(m_pMISUSSite,
                                 bInternalSurfaceUpdateOnly,
                                 bHasVisibleSurface);
    }
#else   //HELIX_FEATURE_MISU

    ForceDisplayUpdateOnSite(m_pMISUSSite,
                             bInternalSurfaceUpdateOnly,
                             bHasVisibleSurface);

#endif  //HELIX_FEATURE_MISU

}

HX_RESULT CVideoRenderer::ForceDisplayUpdateOnSite(IHXSite* pSite, 
                                                   HXBOOL bInternalSurfaceUpdateOnly,
                                                   HXBOOL bHasVisibleSurface)
{
    HX_RESULT retVal = HXR_OK;

    IHXVideoSurface2* pVideoSurface2;
    IHXSite2* pSite2 = NULL;
    IHXVideoSurface* pVideoSurface = NULL;
    
    if (SUCCEEDED(pSite->QueryInterface(IID_IHXSite2, (void**) &pSite2)))
    {
        pSite2->GetVideoSurface(pVideoSurface);
    }

    if (bInternalSurfaceUpdateOnly)
    {
        if (pVideoSurface)
        {
            if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2, 
                                                        (void**) &pVideoSurface2)))
            {
                pVideoSurface2->PresentIfReady();
                pVideoSurface2->Release();
            }
        }
    }
    else
    {
        HXxEvent fakeEvent;

        fakeEvent.param1 = (void*) pVideoSurface;
        retVal = UpdateDisplay(&fakeEvent, FALSE, bHasVisibleSurface);
    }
    
    HX_RELEASE(pVideoSurface);
    HX_RELEASE(pSite2);

    return retVal;
}


STDMETHODIMP CVideoRenderer::Func(void)
{
    HX_TRACE_THINGY("C:/trace.txt", "Func", 0);

    m_hPendingHandle = NULL;

    SchedulerCallback(m_bIsScheduledCB);

    return HXR_OK;
}


HX_RESULT CVideoRenderer::BeginOptimizedBlt(HXBitmapInfoHeader* pBitmapInfo)
{
    HX_RESULT retVal = HXR_UNEXPECTED;
    IHXSite2* pMISUSSite2   = NULL;
    IHXVideoSurface* pVideoSurface = NULL;

    if (m_pMISUSSite)
    {
        m_pMISUSSite->QueryInterface(IID_IHXSite2, (void**)&pMISUSSite2);
        if (pBitmapInfo && pMISUSSite2)
        {
            if (SUCCEEDED(pMISUSSite2->GetVideoSurface(pVideoSurface)))
            {
                retVal = pVideoSurface->BeginOptimizedBlt(pBitmapInfo);
                if (retVal == HXR_OK)
                {
                    m_bOptimizedBlt = TRUE;
                }
                HX_RELEASE(pVideoSurface);
            }
        }
        HX_RELEASE(pMISUSSite2);
    }

    return retVal;
}


void CVideoRenderer::EndOptimizedBlt(void)
{
    IHXSite2* pMISUSSite2 = NULL;
    IHXVideoSurface* pVideoSurface = NULL;

    if (m_bOptimizedBlt && m_pMISUSSite)
    {
        m_pMISUSSite->QueryInterface(IID_IHXSite2, (void**)&pMISUSSite2);

        if (pMISUSSite2)
        {
            pMISUSSite2->GetVideoSurface(pVideoSurface);
            if (pVideoSurface)
            {
                pVideoSurface->EndOptimizedBlt();
                m_bOptimizedBlt = FALSE;
                HX_RELEASE(pVideoSurface);
            }
        }
    }

    HX_RELEASE(pMISUSSite2);
}


inline void CVideoRenderer::RequestBuffering(void)
{
    m_bBufferingNeeded = TRUE;
}


inline void CVideoRenderer::RequestBufferingEnd(void)
{
    m_bBufferingNeeded = FALSE;
}


HX_RESULT CVideoRenderer::BeginBuffering(void)
{
    HX_RESULT retVal = HXR_FAIL;
    if (m_ulVideoBufferingPreference == 0)
    {
        // Video Buffering disabled
        return HXR_OK;
    }

    m_ulBufferingStartTime = HX_GET_BETTERTICKCOUNT();

    // If this is a reccuring rebuffer, start buffering only if
    // the preroll period passed since the last buffering completion.
    // This is done to prevent buffering all the time.
    if (!m_bBufferingOccured ||
        ((m_ulBufferingStartTime - m_ulBufferingStopTime) > m_ulPreroll))
    {
        // Bytes to Buffer could be used for Predata
        // - Predata was not found beneficial yet
        m_ulBytesToBuffer = (ULONG32) (((double) m_ulPreroll) *
                                       ((double) m_ulAvgBitRate) /
                                       8000.0);

        m_ulBufferingTimeOut = m_ulPreroll * 2;

        if (m_ulBufferingTimeOut > 0)
        {
            ChangePlayState(Buffering);

            retVal = m_pStream->ReportRebufferStatus(1, 0);
        }
    }

    return retVal;
}


HX_RESULT CVideoRenderer::EndBuffering(void)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_ulVideoBufferingPreference == 0)
    {
        // Video Buffering disabled
        return HXR_OK;
    }
	
    if (m_PlayState == Buffering)
    {
        ChangePlayState(PlayStarting);
        m_bBufferingOccured = TRUE;
        m_ulBufferingStopTime = HX_GET_BETTERTICKCOUNT();

        HX_ASSERT(m_pStream);
        retVal = m_pStream->ReportRebufferStatus(1,1);
    }

    return retVal;
}


inline HXBOOL CVideoRenderer::IsBufferingComplete(IHXPacket *pPacket)
{
    if (m_ulVideoBufferingPreference == 0)
    {
        // Video Buffering disabled
        return TRUE;
    }
	
    ULONG32 ulTimeNow = HX_GET_BETTERTICKCOUNT();

    if ((m_PlayState == Buffering) && (m_bBufferingNeeded))
    {
        if (pPacket &&
            ((ComputeTimeAhead(pPacket->GetTime(), 0)) > ((INT32) m_ulPreroll)))
        {
            return TRUE;
        }

        return ((ulTimeNow - m_ulBufferingStartTime) > m_ulBufferingTimeOut);
    }

    return TRUE;
}


HX_RESULT CVideoRenderer::LocalizeActiveVideoPacket(void)
{
    HX_RESULT retVal = HXR_IGNORE;

    DisplayMutex_Lock();

    if (m_pActiveVideoPacket && (!m_bActiveVideoPacketLocalized))
    {
        CMediaPacket* pLocalPacket = NULL;
        ULONG32 ulDataSize = m_pActiveVideoPacket->m_ulDataSize;
        UINT8* pData = new UINT8 [ulDataSize];

        retVal = HXR_OUTOFMEMORY;
        if (pData)
        {
            retVal = HXR_OK;
        }

        if (retVal == HXR_OK)
        {
            pLocalPacket = new CMediaPacket(pData,
                                            pData,
                                            ulDataSize,
                                            ulDataSize,
                                            m_pActiveVideoPacket->m_ulTime,
                                            m_pActiveVideoPacket->m_ulFlags,
                                            NULL);

            retVal = HXR_OUTOFMEMORY;
            if (pLocalPacket)
            {
                retVal = HXR_OK;
            }
        }

        if (retVal == HXR_OK)
        {
            memcpy(pData, m_pActiveVideoPacket->m_pData, ulDataSize); /* Flawfinder: ignore */
            ReleasePacket(m_pActiveVideoPacket);
            m_pActiveVideoPacket = pLocalPacket;
            m_bActiveVideoPacketLocalized = TRUE;
        }
        else
        {
            HX_VECTOR_DELETE(pData);
            HX_DELETE(pLocalPacket);
        }
    }

    DisplayMutex_Unlock();

    return retVal;
}


void CVideoRenderer::ReleaseFramePacket(CMediaPacket* pPacket)
{
    ReleasePacket(pPacket);
}


void CVideoRenderer::ReleasePacket(CMediaPacket* pPacket,
                                          HXBOOL bForceKill)
{
    CHXBufferPool* pFramePool = NULL;

    if (m_pVideoFormat)
    {
        m_pVideoFormat->OnDecodedPacketRelease(pPacket);
        pFramePool = m_pVideoFormat->GetFramePool();
    }

    if (pPacket)
    {
        if (pFramePool && (!bForceKill))
        {
            pFramePool->Put(pPacket);
        }
        else
        {
            pPacket->Clear();
            delete pPacket;
        }
    }

    if (!IsDecoderRunning())
    {
	if (m_pVideoFormat)
	{
	    m_pVideoFormat->SetFillbackDecodeNeeded();
	}
    }
}


void CVideoRenderer::ClearBltPacketQueue(void)
{
    CMediaPacket* pVideoPacket;

    if (m_pBltPacketQueue)
    {
        while ((pVideoPacket = (CMediaPacket*) m_pBltPacketQueue->Get()) != 0)
        {
            ReleasePacket(pVideoPacket);
        }
    }
}


/****************************************************************************
 *  CVideoRenderer::CSetSizeCB
 *  This routine increases the object reference count in a thread safe
 *  manner. The reference count is used to manage the lifetime of an object.
 *  This method must be explicitly called by the user whenever a new
 *  reference to an object is used.
 */
/****************************************************************************
 *  CVideoRenderer::CSetSizeCB::QueryInterface
 */
STDMETHODIMP CVideoRenderer::CSetSizeCB::QueryInterface(REFIID riid,
                                                          void** ppvObj)
{
    QInterfaceList  qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlugin*) this},
        { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this},
    };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/****************************************************************************
 *  CVideoRenderer::CSetSizeCB::AddRef                             \
 */
STDMETHODIMP_(ULONG32) CVideoRenderer::CSetSizeCB::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/****************************************************************************
 *  CVideoRenderer::CSetSizeCB::Release
 */
STDMETHODIMP_(ULONG32) CVideoRenderer::CSetSizeCB::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/****************************************************************************
 *  CVideoRenderer::CSetSizeCB::Func
 */
STDMETHODIMP CVideoRenderer::CSetSizeCB::Func()
{
    return m_pSite->SetSize(m_szViewFrame);
}


/****************************************************************************
 *  CVideoRenderer::Pacemaker Responses
 */
STDMETHODIMP CVideoRenderer::OnPaceStart(ULONG32 ulId)
{
    if (ulId == m_ulDecoderPacemakerId)
    {
        if (m_pVideoFormat)
        {
            m_pDecoderVideoFormat = m_pVideoFormat;
            m_pDecoderVideoFormat->AddRef();
        }
        m_bDecoderRunning = TRUE;
	HXLOGL4(HXLOG_BVID, "CVideoRenderer::OnPaceStart() Decoder Started");
    }
    else if (ulId == m_ulBltrPacemakerId)
    {
        if (m_pVideoFormat)
        {
            m_pBltrVideoFormat = m_pVideoFormat;
            m_pBltrVideoFormat->AddRef();
        }
	HXLOGL4(HXLOG_BVID, "CVideoRenderer::OnPaceStart() Bltr Started");
    }

    return HXR_OK;
}

STDMETHODIMP CVideoRenderer::OnPaceEnd(ULONG32 ulId)
{
    if (ulId == m_ulDecoderPacemakerId)
    {
        m_bDecoderRunning = FALSE;
        if (m_pDecoderVideoFormat)
        {
            m_pDecoderVideoFormat->Release();
            m_pDecoderVideoFormat = NULL;
        }
	HXLOGL3(HXLOG_BVID, "CVideoRenderer::OnPaceEnd() Decoder Stopped");
    }
    else if (ulId == m_ulBltrPacemakerId)
    {
        if (m_pBltrVideoFormat)
        {
            m_pBltrVideoFormat->Release();
            m_pBltrVideoFormat = NULL;
        }
	HXLOGL3(HXLOG_BVID, "CVideoRenderer::OnPaceEnd() Bltr Stopped");
    }

    return HXR_OK;
}

STDMETHODIMP CVideoRenderer::OnPace(ULONG32 ulId)
{
    if (ulId == m_ulBltrPacemakerId)
    {
	HXLOGL4(HXLOG_BVID, "CVideoRenderer::OnPace() Cycling Bltr");
        PresentFrame();
    }
    else if (ulId == m_ulDecoderPacemakerId)
    {
        if (m_bSchedulerStartRequested)
        {
            StartSchedulers();
        }

	HXLOGL4(HXLOG_BVID, "CVideoRenderer::OnPace() Cycling Decoder");
        while ((m_pDecoderVideoFormat->DecodeFrame()) != 0) ;

#ifdef HELIX_FEATURE_VIDREND_BOOSTDECODE_ON_STARTUP
        // on ce we need to get this started quickly, otherwise the initial packets expire
        if (THREAD_PRIORITY_BELOW_NORMAL != GetDecodePriority())
        {
            SetDecodePriority(THREAD_PRIORITY_BELOW_NORMAL);
            return HXR_OK;
        }
#endif
    }

    return HXR_OK;
}


/****************************************************************************
 *  CVideoRenderer::IHXPlaybackVelocity methods
 */
STDMETHODIMP CVideoRenderer::InitVelocityControl(IHXPlaybackVelocityResponse* pResponse)
{
    // No need to save response interface
    return HXR_OK;
}

STDMETHODIMP CVideoRenderer::QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    // No need to implement this
    return HXR_NOTIMPL;
}

STDMETHODIMP CVideoRenderer::SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    HXLOGL1(HXLOG_BVID, "CVideoRenderer::SetVelocity(%ld)", lVelocity);
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (m_lPlaybackVelocity != lVelocity ||
        m_bKeyFrameMode     != bKeyFrameMode)
    {
        m_lPlaybackVelocity = lVelocity;
	if (m_lPlaybackVelocity >= 0)
	{
	    m_ulAbsPlaybackVelocity = (UINT32) m_lPlaybackVelocity;
	}
	else
	{
	    m_ulAbsPlaybackVelocity = (UINT32) (-m_lPlaybackVelocity);
	}
        m_bKeyFrameMode     = bKeyFrameMode;
        m_bAutoSwitch       = bAutoSwitch;
        if (m_pTimeSyncSmoother)
        {
            m_pTimeSyncSmoother->SetVelocity(lVelocity);
        }
        if (m_pVideoFormat)
        {
            m_pVideoFormat->SetVelocity(lVelocity);
            m_pVideoFormat->SetKeyFrameMode(bKeyFrameMode);
        }
        // Anytime we change velocities we reset the flag which
        // says we sent a keyframe mode request.
        m_bSentKeyFrameModeRequest = FALSE;
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return HXR_OK;
}

STDMETHODIMP_(INT32) CVideoRenderer::GetVelocity()
{
    return m_lPlaybackVelocity;
}

STDMETHODIMP CVideoRenderer::SetKeyFrameMode(HXBOOL bKeyFrameMode)
{
    m_bKeyFrameMode = bKeyFrameMode;
    if (m_pVideoFormat)
    {
        m_pVideoFormat->SetKeyFrameMode(bKeyFrameMode);
    }
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) CVideoRenderer::GetKeyFrameMode()
{
    return m_bKeyFrameMode;
}

STDMETHODIMP CVideoRenderer::CloseVelocityControl()
{
    return HXR_OK;
}


/****************************************************************************
 *  Video Surface 2 Support
 */
HX_RESULT CVideoRenderer::InitVideoSurface2(ULONG32 ulWidth, ULONG32 ulHeight)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    HX_RESULT retVal = HXR_FAIL;

    HXLOGL4(HXLOG_BVID, "CVideoRenderer::InitVideoSurface2(Width=%lu Height=%lu) Start",
	    ulWidth,
	    ulHeight);

    HXBOOL bTryVideoSurface2 = m_bTryVideoSurface2;

    if (bTryVideoSurface2)
    {
        HX_ASSERT(m_pMISUSSite);
        IHXVideoSurface* pVideoSurface = NULL;
        IHXVideoSurface2* pVideoSurface2 = NULL;
        IHXSite2* pSite2 = NULL;

        if (m_bUseVideoSurface2 &&
            m_pVSurf2InputBIH &&
            (m_pVSurf2InputBIH->biWidth == ((LONG32) ulWidth)) &&
            (m_pVSurf2InputBIH->biHeight == ((LONG32) ulHeight)))
        {
	    HXLOGL4(HXLOG_BVID, "CVideoRenderer::InitVideoSurface2 Bailing out: Already initialized");
            return HXR_OK;
        }

        // EndOptimizedBlt();

        bTryVideoSurface2 = FALSE;

        m_pMISUSSite->QueryInterface(IID_IHXSite2, (void**) &pSite2);
        if (pSite2)
        {
            pSite2->GetVideoSurface(pVideoSurface);
        }
        HX_RELEASE(pSite2);

        HX_ASSERT(pVideoSurface);
        if (pVideoSurface)
        {
            pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                          (void**) &pVideoSurface2);
        }
        HX_RELEASE(pVideoSurface);

        if (pVideoSurface2)
        {
            HXBitmapInfoHeader userBIH;
            ULONG32 ulUserBufCount;
            IHXBuffer* pInitialCountBuffer = NULL;
            HX_RESULT status;

            m_ulHWBufCount = m_ulConfigHWBufCount;

            HX_RELEASE(pInitialCountBuffer);

            if (m_pVSurf2InputBIH == NULL)
            {
                m_pVSurf2InputBIH = new HXBitmapInfoHeader;
            }

            retVal = HXR_OUTOFMEMORY;
            if (m_pVSurf2InputBIH)
            {
                *m_pVSurf2InputBIH = m_BitmapInfoHeader;
                m_pVSurf2InputBIH->biWidth = ulWidth;
                m_pVSurf2InputBIH->biHeight = ulHeight;
                retVal = HXR_OK;
            }

            if (SUCCEEDED(retVal))
            {
                do
                {
                    // SetProperties should modify m_pVSurf2InputBIH
                    status = pVideoSurface2->SetProperties(m_pVSurf2InputBIH,
                                                           m_ulHWBufCount,
                                                           (IHXRenderTimeLine*) this);

                    /*** This code creates a leak - the renderer is not released
                     *** Needs investigation.
                    if (SUCCEEDED(status) && (m_ulHWBufCount > 0))
                    {
                        UINT8* pVideoMem;
                        INT32 iPitch;

                        status = pVideoSurface2->GetVideoMem(pVideoMem,
                                                             iPitch,
                                                             HX_WAIT_FOREVER,
                                                             m_pVSurf2InputBIH);

                        if (SUCCEEDED(status))
                        {
                            pVideoSurface2->ReleaseVideoMem(pVideoMem);
                        }
                    }
                     ***/

                    userBIH = *m_pVSurf2InputBIH;
                    ulUserBufCount = m_ulHWBufCount;

                    retVal = OnOptimizedVideo(status,
                                              m_BitmapInfoHeader,
                                              userBIH,
                                              ulUserBufCount);

                    if (retVal == HXR_RETRY)
                    {
                        *m_pVSurf2InputBIH = userBIH;
                        m_ulHWBufCount = ulUserBufCount;
                    }
                } while (retVal == HXR_RETRY);
            }

            if (SUCCEEDED(retVal) &&
                SUCCEEDED(status) &&
                (m_ulHWBufCount != 0))
            {
                bTryVideoSurface2 = TRUE;
            }
        }

        if (bTryVideoSurface2)
        {
            // Turning on video surface 2
            m_ulEarlyFrameTol = m_ulMaxOptimizedVideoLead;
            if (!m_bUseVideoSurface2)
            {
                m_bVideoSurface2Transition = TRUE;
                m_bUseVideoSurface2 = TRUE;
                ReportStat(VS_SURFACE_MODE, (INT32) 2);
                if (m_pBltrPump)
                {
                    // Wake up Bltr thread
                    m_pBltrPump->Suspend(FALSE);
                    m_pBltrPump->Signal();
                }
            }
        }

        HX_RELEASE(pVideoSurface2);
    }

    if (FAILED(retVal))
    {
        HX_DELETE(m_pVSurf2InputBIH);
    }

    m_bUseVideoSurface2 = bTryVideoSurface2;

    HXLOGL4(HXLOG_BVID, "CVideoRenderer::InitVideoSurface2 End");

    return retVal;
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    return HXR_NOTIMPL;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}


HX_RESULT CVideoRenderer::UpdateVideoSurface2(IHXVideoSurface* pVideoSurface,
                                              HXxRect &destRect,
                                              HXxRect &sorcRect,
                                              HXBOOL bSystemEvent,
                                              HXBOOL& bNewFrameBlt)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    VideoMemStruct videoMemoryInfo;
    IHXVideoSurface2* pVideoSurface2 = NULL;
    HX_RESULT retVal;

    HX_ASSERT(pVideoSurface);

    videoMemoryInfo.pVidMem = NULL;

    retVal = pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                           (void**) &pVideoSurface2);

    if (bNewFrameBlt)
    {
        // If we are in transition to video surface 2 or if we are in the
        // middle of processing a system event, we are not in
        // natural Bltr thread. Thus we cannot be suspending the thread we
        // are in in order to wait for the video memory to become available
        // as that is valid to do only when in Bltr thread.
        HXBOOL bWaitForVideoMem = (!m_bVideoSurface2Transition && !bSystemEvent);
	HXBOOL bVS2BufferUnavailableOnLastBlt = FALSE;
	// Remove current video packet as active in case we end up disrrupting
	// the display mutex below since if we do, we do not wish current
	// packet to be destroyed/replaced while we are presenting.
	CMediaPacket* pVideoPacket = m_pActiveVideoPacket;
	HXBOOL bVideoPacketLocalized = m_bActiveVideoPacketLocalized;
	m_pActiveVideoPacket = NULL;

	// We uae this to detect if VS2 is flushed while presenting
	m_bVS2Flushed = FALSE;

        m_bPresentInProgress = TRUE;

        // If we'll be waiting for video memory (even potentially), unlock
        // the display mutex to allow refresh events to be processed.
        if (bWaitForVideoMem)
        {
	    // Once we unlock the mutex, we must execute GetVideoMem
	    // followed by Present/ReleaseVideoMem without any interceding 
	    // display mutex acquisitions.  The reason is that GetVideoMem
	    // locks the surface and site mutex inside the site object
	    // and releases it only when Present or ReleaseVideoMem
	    // is called.
            DisplayMutex_Unlock();
        }

        if (SUCCEEDED(retVal))
        {
            HX_ASSERT(m_bVideoSurface2Transition ? bSystemEvent : TRUE);

            retVal = pVideoSurface2->GetVideoMem(&videoMemoryInfo,
                                                 bWaitForVideoMem ?
                                                 HX_WAIT_FOREVER : HX_WAIT_NEVER);

	    bVS2BufferUnavailableOnLastBlt = FAILED(retVal);
        }

        if (SUCCEEDED(retVal))
        {
            retVal = TransferOptimizedVideo(pVideoSurface2,
                                            &videoMemoryInfo,
                                            pVideoPacket,
                                            m_BitmapInfoHeader,
                                            sorcRect,
                                            sorcRect);
        }

        if (SUCCEEDED(retVal))
        {
            if (m_bFirstSurfaceUpdate || m_bVS2Flushed)
            {
		// If this is not a system event processing, than we are not under the
		// protection of TLS mutex.  Present immediate will atemt acquisition of TLS
		// mutex which may cause a deadlock since Site may already be holding a TLS
		// mutex and waiting for VSMutex owned by this renderer code.
		// In oreder to prevent a deadlock, we disrupt VSMutex.  When VSMutex is
		// in dirupted state, it can be acquired but this VS servicing code is
		// bypassed as a no-op if call and thus mutual exclusion of VS
		// servicing section is upheld.
		/**** MBO - The folowing invocation of Present in immediate mode
		            poses an issue in VS2 which occasionaly deadlock shortly after
			    this invocation.  To temporary work around this issues, the
			    use of immediate mode is avoided in this circumstance.
                if (!bSystemEvent)
                {
		    VSMutex_Disrupt();
		}
		
		retVal = pVideoSurface2->Present(&videoMemoryInfo,
                                                 pVideoPacket->m_ulTime,
                                                 HX_MODE_IMMEDIATE,
                                                 &destRect,
                                                 &sorcRect);

		if (!bSystemEvent)
		{
		    VSMutex_Continue();
                }

                // We'll fail if the site is hidden by SMIL:
                if (FAILED(retVal))
		******/
		{
		    HXLOGL4(HXLOG_BVID, "CVideoRenderer::UpdateVideoSurface2() PresentFirstFrame PTS=%lu",
					pVideoPacket->m_ulTime);

		    retVal = pVideoSurface2->Present(&videoMemoryInfo,
                                                     ((ULONG32) -ComputeTimeAhead(0, 0)),   // Current time
                                                     HX_MODE_TIMED,
                                                     &destRect,
                                                     &sorcRect);
                }
            }
            else
            {
		HXLOGL4(HXLOG_BVID, "CVideoRenderer::UpdateVideoSurface2() PresentFrame PTS=%lu",
					pVideoPacket->m_ulTime);

                retVal = pVideoSurface2->Present(&videoMemoryInfo,
                                                 pVideoPacket->m_ulTime,
                                                 HX_MODE_TIMED,
                                                 &destRect,
                                                 &sorcRect);
            }

            if (SUCCEEDED(retVal))
            {
                videoMemoryInfo.pVidMem = NULL;
            }
        }

	if (videoMemoryInfo.pVidMem != NULL)
	{
	    pVideoSurface2->ReleaseVideoMem(&videoMemoryInfo);
	}

	// If we have been waiting for video memory, we have unlocked the 
        // display mutex and we need to relock it to proceed.
        if (bWaitForVideoMem)
        {
	    DisplayMutex_Lock();

            // If the switch to VS1 occured after we successfully obtained video
            // memory, fail this VS2 Blt
            if ((!m_bUseVideoSurface2) && SUCCEEDED(retVal))
            {
                retVal = HXR_FAIL;
            }
        }
	
	// Restore the presented video packet back as the active video packet
	// to be used for refresh purposes unless onother packet was made active
	// in the mean-time.
	if (m_pActiveVideoPacket)
	{
	    // There is another video packet - discard our since it is out of date.
	    ReleasePacket(pVideoPacket, bVideoPacketLocalized);
	}
	else
	{
	    m_pActiveVideoPacket = pVideoPacket;
	}

	// We indicate this not the first surface update or if VS2 buffer was not
	// available but only if VS2 has not been flushed while presenting the 
	// frame above.
	// If VS2 has been flushed, it inidcates a new sequence and a new 
	// UpdateVideoSurface call is needed to clear the first surface update 
	// state.  Any anavailability of the VS2 buffer or other failures must 
	// also be ignored in such case.
	if (m_bVS2Flushed)
	{
	    retVal = HXR_OK;
	}
	else
	{
	    m_bFirstSurfaceUpdate = FALSE;
	    m_bVS2BufferUnavailableOnLastBlt = bVS2BufferUnavailableOnLastBlt;
	}

        m_bPresentInProgress = FALSE;
    }
    else if ((!m_bPresentInProgress) && bSystemEvent)
    {
	// We refresh only in case this was system request and we do not have
	// another present already in progress.
	HXLOGL4(HXLOG_BVID, "CVideoRenderer::UpdateVideoSurface2() Refresh");

        retVal = pVideoSurface2->Present(NULL,
                                         0,
                                         HX_MODE_REFRESH,
                                         &destRect,
                                         &sorcRect);

        // If no frames in video surface - make one and present it
        if (FAILED(retVal) && bSystemEvent)
        {
            retVal = pVideoSurface2->GetVideoMem(&videoMemoryInfo,
                                                 HX_WAIT_NEVER);

            if (SUCCEEDED(retVal))
            {
                retVal = TransferOptimizedVideo(pVideoSurface2,
                                                &videoMemoryInfo,
                                                m_pActiveVideoPacket,
                                                m_BitmapInfoHeader,
                                                sorcRect,
                                                sorcRect);

                if (SUCCEEDED(retVal))
                {
		    /**** In order to avoid deadlock issues HX_MODE_IMMEDIATE may pose, we use
		          timed mode blt at current time
                    retVal = pVideoSurface2->Present(&videoMemoryInfo,
                                                     m_pActiveVideoPacket->m_ulTime,
                                                     HX_MODE_IMMEDIATE,
                                                     &destRect,
                                                     &sorcRect);
		    ***/

		    retVal = pVideoSurface2->Present(&videoMemoryInfo,
                                                     ((ULONG32) -ComputeTimeAhead(0, 0)),   // Current time
                                                     HX_MODE_TIMED,
                                                     &destRect,
                                                     &sorcRect);

                    if (FAILED(retVal))
                    {
                        pVideoSurface2->ReleaseVideoMem(&videoMemoryInfo);
                        retVal = pVideoSurface2->Present(NULL,
                                                         0,
                                                         HX_MODE_REFRESH,
                                                         &destRect,
                                                         &sorcRect);
                    }

                    videoMemoryInfo.pVidMem = NULL;
                }
            }
            else
            {
                retVal = pVideoSurface2->Present(NULL,
                                                 0,
                                                 HX_MODE_REFRESH,
                                                 &destRect,
                                                 &sorcRect);
            }
        }

	if (videoMemoryInfo.pVidMem != NULL)
	{
	    pVideoSurface2->ReleaseVideoMem(&videoMemoryInfo);
	}
    }

    HX_RELEASE(pVideoSurface2);

    return retVal;
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    return HXR_NOTIMPL;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}


HX_RESULT CVideoRenderer::FlushVideoSurface2(IHXSite* pSite)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    HX_RESULT retVal = HXR_FAIL;

    IHXVideoSurface* pVideoSurface;
    IHXVideoSurface2* pVideoSurface2;
    IHXSite2* pSite2;

    HX_ASSERT(pSite);

    if (SUCCEEDED(pSite->QueryInterface(IID_IHXSite2, (void**) &pSite2)))
    {
        if (SUCCEEDED(pSite2->GetVideoSurface(pVideoSurface)))
        {
            if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                        (void**) &pVideoSurface2)))
            {
		m_bVS2Flushed = TRUE;
                pVideoSurface2->Flush();
                pVideoSurface2->Release();
                retVal = HXR_OK;
            }

            pVideoSurface->Release();
        }

        pSite2->Release();
    }

    HX_ASSERT(SUCCEEDED(retVal));

    return retVal;
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    return HXR_NOTIMPL;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}


inline HX_RESULT CVideoRenderer::SwitchToVideoSurface1(void)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    HX_RESULT retVal = HXR_OK;

    m_pVSMutex->Lock();

    m_bVideoSurface1Requested = TRUE;
    if (m_bUseVideoSurface2)
    {
        HX_TRACE_THINGY("C:/trace.txt", "-->Switch to VS1", 0);

#ifdef SYNC_VS_SWITCHING
        InitVideoSurface1(TRUE);
        StartSchedulers();
#else   // SYNC_VS_SWITCHING
        // Force Video Surface reinitialization
        m_bVideoSurfaceReinitRequested = TRUE;
#endif  // SYNC_VS_SWITCHING
    }

    m_pVSMutex->Unlock();

    return retVal;
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    return HXR_NOTIMPL;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}


inline HX_RESULT CVideoRenderer::SwitchToVideoSurface2(void)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    HX_RESULT retVal = HXR_OK;

    m_pVSMutex->Lock();

    m_bVideoSurface1Requested = FALSE;
    if (!m_bUseVideoSurface2)
    {
        if (m_bTryVideoSurface2)
        {
            HX_TRACE_THINGY("C:/trace.txt", "-->Switch to VS2", 0);

#ifdef SYNC_VS_SWITCHING
            InitVideoSurface2(m_SetWinSize.cx, m_SetWinSize.cy);
            StartSchedulers();
#else   // SYNC_VS_SWITCHING
            // Force Video Surface reinitialization
            m_bVideoSurfaceReinitRequested = TRUE;
#endif  // SYNC_VS_SWITCHING
        }
        else
        {
            // VideoSurface2 is not allowed
            HX_TRACE_THINGY("C:/trace.txt", "-->Switch to VS2 Denied!", 0);

            retVal = HXR_FAIL;
        }
    }

    m_pVSMutex->Unlock();

    return retVal;
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    return HXR_NOTIMPL;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::OnOptimizedVideo
 *
 */
HX_RESULT CVideoRenderer::OnOptimizedVideo(HX_RESULT status,
                                           const HXBitmapInfoHeader& sourceBIH,
                                           HXBitmapInfoHeader &targetBIH,
                                           ULONG32 &ulTargetBufCount)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    HX_RESULT retVal = HXR_FAIL;

    UINT32 ulBitmapColor = GETBITMAPCOLOR(&sourceBIH);
    if (SUCCEEDED(status) &&
        (ulTargetBufCount > 0) &&
        (ulBitmapColor == CID_I420 ||
         ulBitmapColor == CID_YV12))
    {
        retVal = HXR_OK;
    }

    return retVal;
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    return HXR_NOTIMPL;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::OffOptimizedVideo
 *
 */
void CVideoRenderer::OffOptimizedVideo(void)
{
    return;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::TransferOptimizedVideo
 *
 */
HX_RESULT CVideoRenderer::TransferOptimizedVideo(IHXVideoSurface2* pVideoSurface2,
                                                 VideoMemStruct* pVideoMemoryInfo,
                                                 CMediaPacket* pVideoPacket,
                                                 const HXBitmapInfoHeader& sorcBIH,
                                                 HXxRect &destRect,
                                                 HXxRect &sorcRect,
						 SourceInputStruct* pVideoLayout)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    SourceInputStruct input;
    UINT8* aSrcInput[3];
    INT32 aSrcPitch[3];
    SourceInputStruct* pInputLayout = &input;
    HX_RESULT retVal = HXR_FAIL;

    UINT32 ulBitmapColor = GETBITMAPCOLOR(&sorcBIH);
    if (ulBitmapColor == CID_I420 || ulBitmapColor == CID_YV12)
    {
	if (pVideoLayout)
	{
	    pInputLayout = pVideoLayout;
	}
	else
	{
	    input.aSrcInput = aSrcInput;
	    input.aSrcPitch = aSrcPitch;
	    input.nNumInputs = 3;

	    aSrcInput[0] = pVideoPacket->m_pData;
	    aSrcInput[1] = aSrcInput[0] + (sorcBIH.biWidth * sorcBIH.biHeight);
	    aSrcInput[2] = aSrcInput[1] +
			   ((sorcBIH.biWidth / 2) * (sorcBIH.biHeight / 2));
            // The above is the layout for I420. For YV12, the
            // U and V planes are swapped. So simply swap
            // aSrcInput[1] and aSrcInput[2].
            if (ulBitmapColor == CID_YV12)
            {
                BYTE* pTmp = aSrcInput[1];
                aSrcInput[1] = aSrcInput[2];
                aSrcInput[2] = pTmp;
            }
	    
	    aSrcPitch[0] = sorcBIH.biWidth;
	    aSrcPitch[1] = sorcBIH.biWidth / 2;
	    aSrcPitch[2] = sorcBIH.biWidth / 2;
	}

        CHXxSize sorcSize(sorcBIH.biWidth, sorcBIH.biHeight);
        CHXxSize destSize(pVideoMemoryInfo->bmi.biWidth, pVideoMemoryInfo->bmi.biHeight);

        INT32 lFourCCIn = HX_I420;
        if (ulBitmapColor == CID_YV12)
        {
            lFourCCIn = HX_YV12;
        }
        retVal = pVideoSurface2->ColorConvert(lFourCCIn,
                                              &sorcSize,
                                              &sorcRect,
                                              pInputLayout,
                                              pVideoMemoryInfo->bmi.biCompression,
                                              pVideoMemoryInfo->pVidMem,
                                              &destSize,
                                              &destRect,
                                              pVideoMemoryInfo->lPitch);

        // Alpha blend subrects:
        for (UINT32 i = 0; i < pVideoMemoryInfo->ulCount; ++i)
        {
                       destSize.SetSize(pVideoMemoryInfo->pAlphaList[i].ulImageWidth, pVideoMemoryInfo->pAlphaList[i].ulImageHeight);
            HX_RESULT localResult = pVideoSurface2->ColorConvert(
                             lFourCCIn,
                             &sorcSize,
                             &pVideoMemoryInfo->pAlphaList[i].rcImageRect,
                             pInputLayout,
                             pVideoMemoryInfo->pAlphaList[i].ulFourCC,
                             pVideoMemoryInfo->pAlphaList[i].pBuffer,
                                                        &destSize,
                             &pVideoMemoryInfo->pAlphaList[i].rcImageRect,
                             pVideoMemoryInfo->pAlphaList[i].lPitch);
            if (SUCCEEDED(retVal))
            {
                retVal = localResult;
            }
        }
    }

    return retVal;
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    return HXR_NOTIMPL;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}


void CVideoRenderer::PresentFrame(void)
{
#ifdef HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
    if (m_bUseVideoSurface2)
    {
        SchedulerCallback(FALSE,    // not scheduled
                          FALSE,    // do not resched
                          TRUE);    // is VS2 call
    }
    else
    {
        // Video Surface2 has been turned off
        IHXPaceMaker* pBltr = m_pBltrPump;

        if (pBltr)
        {
            if (m_bTryVideoSurface2)
            {
                // We might try VideoSurface2 again later - suspend the
                // Bltr thread for now
                pBltr->Suspend(TRUE);
            }
            else
            {
                // We are not to try VideoSurface2 any more - stop the
                // Bltr thread
                pBltr->Stop();
                pBltr->Signal();
            }
        }
    }
#else   // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO

    ;
#endif  // HELIX_FEATURE_VIDREND_OPTIMIZEDVIDEO
}

HX_RESULT CVideoRenderer::SendKeyFrameModeRequest()
{
    HX_RESULT retVal = HXR_OK;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (m_bAutoSwitch && !m_bSentKeyFrameModeRequest)
    {
        HXLOGL1(HXLOG_TRIK, "Sending request to go to keyframe mode");
        retVal = HXR_FAIL;
        if (m_pBackChannel && m_pContext && m_pCommonClassFactory)
        {
            const char* pszMsg  = "SetKeyFrameMode: TRUE";
            IHXBuffer*  pBuffer = NULL;
            retVal = CreateStringBufferCCF(pBuffer, pszMsg, m_pContext);
            if (SUCCEEDED(retVal))
            {
                // Create the packet
                IHXPacket* pPacket = NULL;
                retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXPacket, (void**) &pPacket);
                if (SUCCEEDED(retVal))
                {
                    // Set the packet
                    retVal = pPacket->Set(pBuffer, 0, 0, 0, 0);
                    if (SUCCEEDED(retVal))
                    {
                        // Set the flag
                        m_bSentKeyFrameModeRequest = TRUE;
                        // Send the packet to the back channel
                        retVal = m_pBackChannel->PacketReady(pPacket);
                    }
                }
                HX_RELEASE(pPacket);
            }
            HX_RELEASE(pBuffer);
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

HX_RESULT CVideoRenderer::SetClipRect(INT32 lLeft, INT32 lTop, INT32 lRight, INT32 lBottom)
{
    HX_RESULT retVal = HXR_FAIL;

    if (lLeft >= 0 && lRight  > lLeft &&
        lTop  >= 0 && lBottom > lTop)
    {
        retVal = HXR_OUTOFMEMORY;
        HX_DELETE(m_pClipRect);
        m_pClipRect = (HXxRect*) new HXxRect;
        if (m_pClipRect)
        {
            m_pClipRect->left   = lLeft;
            m_pClipRect->right  = lRight;
            m_pClipRect->top    = lTop;
            m_pClipRect->bottom = lBottom;
            retVal              = HXR_OK;
        }
    }

    return retVal;
}

void CVideoRenderer::ClearClipRect()
{
    HX_DELETE(m_pClipRect);
}

void CVideoRenderer::SignalDecoderThread()
{
    if (m_pDecoderPump && m_bDecoderRunning)
    {
        HXLOGL4(HXLOG_BVID, "CVideoRenderer[%p]::SignalDecoderThread() - signalling decoder pump", this);
        m_pDecoderPump->Signal();
    }
}

STDMETHODIMP_(HXBOOL) CVideoRenderer::FirstFrameDisplayed()
{
    return !m_bFirstFrame;
}

HXBOOL CVideoRenderer::IsVideoOnlyPlayback()
{
    IHXStreamSource* pSource = NULL;
    IHXAudioPlayer* pAudioPlayer = NULL;
    IHXPlayer* pPlayer = NULL;
    HXBOOL bIsVideoOnlyPlayback = FALSE;
    HX_ASSERT(m_pStream);
    m_pStream->GetSource(pSource);
    HX_ASSERT(pSource);
    if (pSource)
    {
        if (pSource->GetPlayer(pPlayer) == HXR_OK)
        {
            HX_ASSERT(pPlayer);
            if (pPlayer && (pPlayer->QueryInterface(IID_IHXAudioPlayer, (void**)&pAudioPlayer) == HXR_OK))
            {
                if (pAudioPlayer->GetAudioStreamCount() == 0)
                {
                    bIsVideoOnlyPlayback = TRUE;
                }
                HX_RELEASE(pAudioPlayer);
            }
            HX_RELEASE(pPlayer);			
        }
        HX_RELEASE(pSource);
    }
    return bIsVideoOnlyPlayback;
}

void CVideoRenderer::CheckForBufferingStart()
{
    LONG32 lTimeDelta = 0;
    if (m_ulVideoBufferingPreference == 0)
    {
        // Video Buffering disabled
        return;
    }
	
    if ((m_PlayState == Playing) && m_pVideoFormat && !m_pVideoFormat->AreRawPacketsDone())
    {
        lTimeDelta = ComputeTimeAhead(m_ulActiveVideoTime, 0);

        if ((m_ulAbsPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL) && 
                (m_ulAbsPlaybackVelocity != 0))
        {
            lTimeDelta = ((lTimeDelta * HX_PLAYBACK_VELOCITY_NORMAL) / 
                                ((INT32) m_ulAbsPlaybackVelocity));
        }

        if (((INT32) (lTimeDelta + GetLateFrameRebufferingTolerance())) < 0)
        {
            if ((m_ulVideoBufferingPreference == 2) && !IsVideoOnlyPlayback())
            {
                // Video buffering disabled for non-video only playback
                return;
            }
            RequestBuffering();
        }
    }
}
