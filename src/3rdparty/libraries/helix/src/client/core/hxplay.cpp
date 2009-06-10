/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxplay.cpp,v 1.200 2009/05/06 20:17:30 sfu Exp $
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

#include "hxtypes.h"
#ifndef _BREW 
#include "hlxclib/sys/socket.h"
#endif
#include "hxcom.h"
#include "hxmime.h"
#include "dbcs.h"
#include "chxxtype.h"
#include "hxresult.h"
#include "hxthreadyield.h"
#ifndef _WINCE
#include "hlxclib/signal.h"
#endif

#include "conn.h"
#if defined( _WIN32 ) || defined( _WINDOWS )
#include "platform/win/win_net.h"
#elif defined (_MACINTOSH)
#include "mac_net.h"
#elif defined (_UNIX)
#include "unix_net.h"
#elif defined(__TCS__)
#include "platform/tm1/tm1_net.h"
#endif // defined( _WIN32 ) || defined( _WINDOWS )

#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxclsnk.h"
#include "hxpends.h"
#include "hxhyper.h"
#include "playhpnv.h"
#include "hxmon.h"
#if defined(HELIX_FEATURE_ASM)
#include "hxasm.h"
#include "hxsmbw.h"
#include "createbwman.h"
#endif /* HELIX_FEATURE_ASM */
#include "hxplugn.h"
#include "chxeven.h"
#include "chxelst.h"
#include "strminfo.h"
#if defined(HELIX_FEATURE_MEDIAMARKER)
#include "hxmmrkr.h"
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
#if defined(HELIX_FEATURE_EVENTMANAGER)
#include "hxinter.h"
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */

#include "pckunpck.h"
#include "hxmutex.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxstrutl.h"
//#include "plgnhand.h"
#include "hxrquest.h"
#include "chxelst.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxflsrc.h"
#include "hxntsrc.h"
#include "hxrendr.h"
#include "hxwin.h"
#include "hxstrm.h"
#include "hxcleng.h"
#include "timeline.h"
#include "hxstring.h"
#include "timeval.h"
#include "hxerror.h"
#include "sinkctl.h"
#include "upgrdcol.h"
#include "chxphook.h"
#include "hxgroup.h"
#include "basgroup.h"
#include "advgroup.h"
#include "hxplayvelocitycaps.h"
#include "hxtlogutil.h"

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

#include "auderrs.h"
#include "hxausvc.h"
#include "hxaudses.h"
#include "hxaudply.h"
#include "hxplay.h"
#include "hxplugn.h"
#include "hxtick.h"

#include "srcinfo.h"
#include "nxtgrmgr.h"
#include "prefmgr.h"
#include "srcinfo.h"
#include "hxthread.h"
#include "hxxrsmg.h"
#include "hxtick.h"
#include "hxplayvelocity.h"
#include "hxresmgr.h"
#include "portaddr.h"
#if defined(HELIX_FEATURE_SMARTERNETWORK)
#include "preftran.h"
#endif /* HELIX_FEATURE_SMARTERNETWORK */
#include "cookies.h"
#include "viewport.h"
#if defined(HELIX_FEATURE_MEDIAMARKER)
#include "mediamrk.h"
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
#if defined(HELIX_FEATURE_EVENTMANAGER)
#include "eventmgr.h"
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
#include "pfeventproxy.h" //  CPresentationFeatureEventProxy class.
#endif //  End else of #if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION).
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */
#include "hxtac.h"
#if defined(_STATICALLY_LINKED) || !defined(HELIX_FEATURE_PLUGINHANDLER2)
#if defined(HELIX_CONFIG_CONSOLIDATED_CORE)
#include "basehand.h"
#else /* HELIX_CONFIG_CONSOLIDATED_CORE */
#include "hxpluginmanager.h"
#endif /* HELIX_CONFIG_CONSOLIDATED_CORE */
#else
#include "plghand2.h"
#endif /* _STATICALLY_LINKED */
//#include "../dcondev/dcon.h"
//HXBOOL g_bRahulLog = FALSE;

#if defined(HELIX_FEATURE_DRM)
#include "hxdrmcore.h"
#endif /* HELIX_FEATURE_DRM */

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
#include "platform/mac/hx_moreprocesses.h"
#endif

#ifdef _MACINTOSH
#include "hxmm.h"
#endif

#ifdef _UNIX
#include "unix_net.h"
#endif

#include "sitemgr.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _WIN16
extern HINSTANCE g_hInstance;       // initialized in DLLMAIN.CPP(rmacore.dll)
#endif

#if defined(__TCS__)
#if defined(__cplusplus)
extern "C" {
#endif

void hookRealAudio_ReportError(int err, long errVal);

#ifdef __cplusplus
}
#endif
#endif

#define MIN_DELAYBEFORE_START       2000
#define BEGIN_SYNC_FUDGE_FACTOR     50
#define ALLFS                       0xFFFFFFFF
#define DEFAULT_NUM_FORWARD_KEYFRAMES_AHEAD   2
#define DEFAULT_NUM_REVERSE_KEYFRAMES_AHEAD   4
#define NUM_FORWARD_KEYFRAMES_AHEAD_STR       "PlaybackVelocity\\NumForwardKeyFramesAhead"
#define NUM_REVERSE_KEYFRAMES_AHEAD_STR       "PlaybackVelocity\\NumReverseKeyFramesAhead"
#define STOP_WHEN_HIT_START_IN_REVERSE        "PlaybackVelocity\\StopWhenHitStartInReverse"

#ifdef HELIX_SYMBIAN81_WINSCW_EMULATOR
/* Symbian emulator has 1 timer tick == 100ms */
#define PLAYER_UPDATE_INTERVAL					90
#elif defined(ANDROID)
#define PLAYER_UPDATE_INTERVAL					90
#else
#define PLAYER_UPDATE_INTERVAL                  30
#endif

#ifdef THREADS_SUPPORTED
#define SYSTEM_PERCENT  65
#define INTERRUPT_PERCENT   30
#else
#define SYSTEM_PERCENT  95
#define INTERRUPT_PERCENT   0
#endif

#define PLAYER_SYSTEM_PROCESSING_INTERVAL       (PLAYER_UPDATE_INTERVAL*SYSTEM_PERCENT/100)
#define PLAYER_INTERRUPT_PROCESSING_INTERVAL    (PLAYER_UPDATE_INTERVAL*INTERRUPT_PERCENT/100)

#define MAX_QUICK_SEEK_POST_BUFFERING_WAIT  500 // in milliseconds

/* Please add any variables to be re-initialized in the ResetPlayer()
 * function also.
 */
HXPlayer::HXPlayer() :
     m_lRefCount (0)
    ,m_ulRepeatedRegistryID(0)
    ,m_ulNextGroupRegistryID(0)
    ,m_LastError(HXR_OK)
    ,m_pLastUserString(NULL)
    ,m_LastSeverity(HXLOG_ERR)
    ,m_ulLastUserCode(HXR_OK)
    ,m_pLastMoreInfoURL(NULL)
    ,m_pRegistry(0)
    ,m_pStats(0)
    ,m_pUpdateStatsCallback(0)
    ,m_pHXPlayerCallback(0)
    ,m_pHXPlayerInterruptCallback(NULL)
    ,m_pHXPlayerInterruptOnlyCallback(NULL)
    ,m_pAuthenticationCallback(NULL)
    ,m_pAutheticationValues(NULL)
    ,m_ulStatsGranularity(1000)
    ,m_pEngine (0)
    ,m_pPlugin2Handler(0)
    ,m_pClient(0)
    ,m_pAudioPlayer (0)
    ,m_pAdviseSink(NULL)
    ,m_pClientStateAdviseSink(NULL)
    ,m_pErrorSinkControl(NULL)
#if defined(HELIX_FEATURE_SINKCONTROL) && defined(HELIX_FEATURE_LOGGING_TRANSLATOR)
    ,m_pErrorSinkTranslator(NULL)
#endif /* #if defined(HELIX_FEATURE_SINKCONTROL) && defined(HELIX_FEATURE_LOGGING_TRANSLATOR) */
    ,m_pClientRequestSink(0)
    ,m_pPreferences(0)
    ,m_pHyperNavigate(0)
    ,m_pPacketHookManager(NULL)
    ,m_pGroupManager(NULL)
    ,m_pMasterTAC(NULL)
    ,m_pRedirectList(NULL)
    ,m_pSDPURLList(NULL)
    ,m_pClientViewSource(NULL)
    ,m_pClientViewRights(NULL)
    ,m_pViewPortManager(NULL)
    ,m_pMediaMarkerManager(NULL)
    ,m_pEventManager(NULL)
    ,m_pCookies3(NULL)
    ,m_pPersistentComponentManager(NULL)
    ,m_pPreferredTransportManager(NULL)
    ,m_pNetInterfaces(NULL)
    ,m_bForceStatsUpdate(FALSE)
    ,m_bActiveRequest(FALSE)
    ,m_bAddLayoutSiteGroupCalled(FALSE)
    ,m_bDoRedirect(FALSE)
    ,m_bFastStartCheckDone(FALSE)
    ,m_bFastStart(FALSE)
    ,m_turboPlayOffReason(TP_OFF_BY_UNKNOWN)
    ,m_ulActiveSureStreamSource(0)
    ,m_nCurrentGroup(0)
    ,m_nGroupCount(0)
    ,m_pAltURLs(NULL)
    ,m_pOppositeHXSourceTypeRetryList(NULL)
    ,m_pURL (0)
    ,m_pRequest(NULL)
    ,m_pMetaSrcStatus(NULL)
    ,m_pSourceMap(NULL)
    ,m_pPendingTrackList(NULL)
    ,m_ulCurrentPlayTime (0)
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
    ,m_ulCurrentSystemPlayTime(0)
#endif
    ,m_ulPresentationDuration(0)
    ,m_ulTimeBeforeSeek (0)
    ,m_ulTimeAfterSeek (0)
    ,m_BufferingReason(BUFFERING_START_UP)
    ,m_pScheduler(0)
    ,m_uNumSourcesActive(0)
    ,m_uNumCurrentSourceNotDone(0)
    ,m_bSourceMapUpdated(FALSE)
    ,m_bInitialized (FALSE)
    ,m_bIsDone(TRUE)
    ,m_bIsPresentationDone(TRUE)
    ,m_bInStop(FALSE)
    ,m_bIsPresentationClosedToBeSent(FALSE)
    ,m_bCloseAllRenderersPending(FALSE)
    ,m_bUseCoreThread(FALSE)
    ,m_ulCoreLockCount(0)
    ,m_bPaused(FALSE)
    ,m_bBeginPending(FALSE)
    ,m_bIsFirstBeginPending(FALSE)
    ,m_bIsFirstBegin(TRUE)
    ,m_bUserHasCalledBegin(FALSE)
    ,m_bTimelineToBeResumed (FALSE)
    ,m_bIsPlaying(FALSE)
    ,m_bIsBuffering(FALSE)
    ,m_eClientState(HX_CLIENT_STATE_READY)
    ,m_eClientStateStatus(HX_CLIENT_STATE_STATUS_ACTIVE)
    ,m_bHaltInConnected(FALSE)
    ,m_bRestartToPrefetched(FALSE)
    ,m_bNetInitialized(FALSE)
    ,m_bPrefTransportInitialized(FALSE)
    ,m_bSetupLayoutSiteGroup(TRUE)
    ,m_bIsSmilRenderer(TRUE)
    ,m_bTimeSyncLocked(FALSE)
    ,m_bIsLive(FALSE)
    ,m_bLiveSeekToBeDone(FALSE)
    ,m_bHasSubordinateLifetime(FALSE)
    ,m_bProcessEventsLocked(FALSE)
    ,m_bDidWeDeleteAllEvents(FALSE)
    ,m_ulLowestGranularity(DEFAULT_TIMESYNC_GRANULARITY)
    ,m_ulElapsedPauseTime(0)
    ,m_ulLiveSeekTime(0)
    ,m_bSeekCached(FALSE)
    ,m_ulSeekTime(0)
    ,m_ulTimeOfPause(0)
    ,m_ulMinimumTotalPreroll(0)
    ,m_ulFirstTimeSync(0)
    ,m_ulFSBufferingEndTime(0)
    ,m_uNumSourceToBeInitializedBeforeBegin(0)
    ,m_bFastStartInProgress(FALSE)
    ,m_bIsFirstTimeSync(TRUE)
    ,m_bPlayerWithoutSources(FALSE)
    ,m_bInternalPauseResume(FALSE)
    ,m_bInternalReset(FALSE)
    ,m_bSetVelocityInProgress(FALSE)
    ,m_bCurrentPresentationClosed(FALSE)
    ,m_bContactingDone(FALSE)
    ,m_bFSBufferingEnd(FALSE)
    ,m_b100BufferingToBeSent(TRUE)
    ,m_bSetupToBeDone(FALSE)
    ,m_bPostSetupToBeDone(FALSE)
    ,m_bInternalReportError(FALSE)
    ,m_bPartOfNextGroup(FALSE)
    ,m_bLastGroup(FALSE)
    ,m_bNextGroupStarted(FALSE)
    ,m_bBeginChangeLayoutTobeCalled(TRUE)
    ,m_bPendingAudioPause(FALSE) // currently used ONLY on Mac
    ,m_bPlayStateNotified(FALSE)
    ,m_bResumeOnlyAtSystemTime(FALSE)

    ,m_bSetModal(FALSE)
    ,m_bEventAcceleration(FALSE)
    ,m_pPrefetchManager(NULL)
#ifdef _WIN32
    ,m_bScreenSaverActive(0)
#endif
    ,m_pNextGroupManager(NULL)
    ,m_pCurrentGroup(NULL)
    ,m_pParentPlayer(NULL)
    ,m_pChildPlayerList(NULL)
#if defined(HELIX_FEATURE_ASM)
    ,m_pBandwidthMgr(NULL)
    ,m_pASM(NULL)
#endif
    ,m_ulPlayerUpdateInterval(PLAYER_UPDATE_INTERVAL)
    ,m_ulPlayerSystemTimeProcessingInterval(PLAYER_SYSTEM_PROCESSING_INTERVAL)
    ,m_ulPlayerInterruptTimeProcessingInterval(PLAYER_INTERRUPT_PROCESSING_INTERVAL)
    ,m_bYieldLessToOthers(FALSE)
    ,m_pSiteManager(NULL)
    ,m_pSiteSupplier(NULL)
    ,m_pPlaybackVelocityResponse(NULL)
    ,m_pPlaybackVelocityCaps(NULL)
    ,m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
    ,m_bKeyFrameMode(FALSE)
    ,m_bAutoSwitch(FALSE)
    ,m_bPlaybackVelocityCached(FALSE)
    ,m_lPlaybackVelocityCached(HX_PLAYBACK_VELOCITY_NORMAL)
    ,m_bKeyFrameModeCached(FALSE)
    ,m_bAutoSwitchCached(FALSE)
    ,m_bVelocityControlInitialized(FALSE)
    ,m_ulKeyFrameInterval(0)
    ,m_ulNumForwardKeyFramesAhead(DEFAULT_NUM_FORWARD_KEYFRAMES_AHEAD)
    ,m_ulNumReverseKeyFramesAhead(DEFAULT_NUM_REVERSE_KEYFRAMES_AHEAD)
    ,m_bStopWhenHitStartInReverse(FALSE)
    ,m_pUpgradeCollection(NULL)
    ,m_pCoreMutex(NULL)
    ,m_bQuickSeekMode(FALSE) 
    ,m_ulSeekQueue(kNoValue)
    ,m_ulBufferingCompletionTime(0)
    ,m_pSharedWallClocks(NULL)
    ,m_pRecordService(NULL)
    ,m_bRecordServiceEnabled(FALSE)
    ,m_pMetaInfo(NULL)
    ,m_pEmbeddedUI(NULL)
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    , m_ulTotalDurReported(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulTimeOfOpenURL(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_pPDStatusMgr(NULL)
    , m_pPDSObserverList(NULL) // /XXXEH- TODO: reset in ResetPlayer()?
    , m_pProgDnldStatusReportInfoList(NULL)
    , m_pAuthenticationRequestsPending(NULL)
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
#if defined(HELIX_FEATURE_EVENTMANAGER)
    , m_pPFSEventProxyList(NULL)
#else //  If Events are not supported, then notify the P.F.Sinks directly:
    , m_pPFSSinkList(NULL)
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.
#if defined(HELIX_FEATURE_PLAYBACK_MODIFIER)
    , m_pPlaybackModifiers(NULL)
#endif
/* Please add any variables to be re-initialized in the ResetPlayer()
 * function also.
 */
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::HXPlayer()", this);

    m_pSourceMap = new CHXMapPtrToPtr;
    m_pSharedWallClocks = new CHXMapStringToOb;
    
    ResetError();

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    m_pUpdateStatsCallback              = new UpdateStatsCallback;
    m_pUpdateStatsCallback->m_pPlayer   = this;
    m_pUpdateStatsCallback->AddRef();
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    m_pHXPlayerCallback = new HXPlayerCallback((void*)this, (fGenericCBFunc)PlayerCallback);
    m_pHXPlayerInterruptCallback = new HXPlayerCallback((void*)this, (fGenericCBFunc)PlayerCallbackInterruptSafe);
    m_pHXPlayerInterruptOnlyCallback = new HXPlayerCallback((void*)this, (fGenericCBFunc)PlayerCallbackInterruptOnly);

#if defined (_WIN32) || defined (_MACINTOSH) || defined(THREADS_SUPPORTED)
    ((HXPlayerCallback*)m_pHXPlayerInterruptCallback)->m_bInterruptSafe = TRUE;
    ((HXPlayerCallback*)m_pHXPlayerInterruptOnlyCallback)->m_bInterruptOnly = TRUE;
#endif

    m_pHXPlayerCallback->AddRef();
    m_pHXPlayerInterruptCallback->AddRef();
    m_pHXPlayerInterruptOnlyCallback->AddRef();

#if defined(HELIX_FEATURE_AUTHENTICATION)
    m_pAuthenticationCallback = new CHXGenericCallback((void*)this, (fGenericCBFunc)AuthenticationCallback);
    m_pAuthenticationCallback->AddRef();
#endif /* HELIX_FEATURE_AUTHENTICATION */

#if defined(HELIX_FEATURE_SINKCONTROL)
    m_pErrorSinkControl     = new CHXErrorSinkControl;
    m_pAdviseSink           = new CHXAdviseSinkControl;
    m_pClientStateAdviseSink = new CHXClientStateAdviseSink;
#if defined(HELIX_FEATURE_LOGGING_TRANSLATOR)
    m_pErrorSinkTranslator  = new CHXErrorSinkTranslator;
    HX_ADDREF(m_pErrorSinkTranslator);
#endif /* #if defined(HELIX_FEATURE_LOGGING_TRANSLATOR) */
#endif /* HELIX_FEATURE_SINKCONTROL */
#if defined(HELIX_FEATURE_VIDEO)
    m_pSiteManager          = new CHXSiteManager((IUnknown*)(IHXPlayer*) this);
#endif /* HELIX_FEATURE_VIDEO */
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    m_pGroupManager         = new HXAdvancedGroupManager(this);
#elif defined(HELIX_FEATURE_BASICGROUPMGR)
    m_pGroupManager         = new HXBasicGroupManager(this);
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    m_pNextGroupManager     = new NextGroupManager(this);
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
#if defined(HELIX_FEATURE_VIEWPORT)
    m_pViewPortManager      = new HXViewPortManager(this);
#endif /* HELIX_FEATURE_VIEWPORT */
#if defined(HELIX_FEATURE_MEDIAMARKER)
    m_pMediaMarkerManager   = new CHXMediaMarkerManager(this);
    if (m_pMediaMarkerManager) m_pMediaMarkerManager->AddRef();
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
#if defined(HELIX_FEATURE_NESTEDMETA)
    m_pPersistentComponentManager = new HXPersistentComponentManager(this);
#endif /* HELIX_FEATURE_NESTEDMETA */

    m_pMetaInfo = new CHXMetaInfo((IUnknown*)(IHXPlayer*)this);
    HX_ADDREF(m_pMetaInfo);

#if defined(HELIX_FEATURE_EMBEDDED_UI)
    m_pEmbeddedUI = new CHXEmbeddedUI((IUnknown*)(IHXPlayer*)this);
    HX_ADDREF(m_pEmbeddedUI);
#endif // HELIX_FEATURE_EMBEDDED_UI

    HX_ADDREF(m_pErrorSinkControl);
    HX_ADDREF(m_pAdviseSink);
    HX_ADDREF(m_pClientStateAdviseSink);
#if defined(HELIX_FEATURE_VIDEO)
    m_pSiteManager->AddRef();
#endif /* HELIX_FEATURE_VIDEO */
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    m_pGroupManager->AddRef();
    m_pGroupManager->AddSink(this);
#endif /* HELIX_FEATURE_BASICGROUPMGR */

#if defined(HELIX_FEATURE_VIEWPORT)
    m_pViewPortManager->AddRef();
#endif /* HELIX_FEATURE_VIEWPORT */

#if defined(HELIX_FEATURE_NESTEDMETA)
    m_pPersistentComponentManager->AddRef();
    if (m_pGroupManager)
    {
        m_pGroupManager->AddSink(m_pPersistentComponentManager);
    }
#endif /* HELIX_FEATURE_NESTEDMETA */
}

HXPlayer::~HXPlayer()
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::~HXPlayer()", this);

    CloseAllRenderers(m_nCurrentGroup);
    ResetPlayer();
    Close();
    HX_DELETE(m_pSourceMap);
    HX_DELETE(m_pSharedWallClocks);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP HXPlayer::QueryInterface(REFIID riid, void** ppvObj)
{
#if defined(HELIX_FEATURE_PACKETHOOKMGR)
    // create the following objects only if needed
    if (!m_pPacketHookManager && IsEqualIID(riid, IID_IHXPacketHookManager))
    {
        m_pPacketHookManager = new PacketHookManager(this);

        if (m_pPacketHookManager)
        {
            m_pPacketHookManager->AddRef();
        }
    }
#endif /* HELIX_FEATURE_PACKETHOOKMGR */

    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPlayer), (IHXPlayer*)this },
            { GET_IIDHANDLE(IID_IHXQuickSeek), (IHXQuickSeek*)this },
            { GET_IIDHANDLE(IID_IHXPlayer2), (IHXPlayer2*)this },
            { GET_IIDHANDLE(IID_IHXPendingStatus), (IHXPendingStatus*)this },
#if defined(HELIX_FEATURE_AUTHENTICATION)
            { GET_IIDHANDLE(IID_IHXAuthenticationManager), (IHXAuthenticationManager*)this },
            { GET_IIDHANDLE(IID_IHXAuthenticationManager2), (IHXAuthenticationManager2*)this },
#endif /* HELIX_FEATURE_AUTHENTICATION */
            { GET_IIDHANDLE(IID_IHXGroupSink), (IHXGroupSink*)this },
            { GET_IIDHANDLE(IID_IHXAudioPlayerResponse), (IHXAudioPlayerResponse*)this },
            { GET_IIDHANDLE(IID_IHXRegistryID), (IHXRegistryID*)this },
            { GET_IIDHANDLE(IID_IHXErrorMessages), (IHXErrorMessages*)this },
            { GET_IIDHANDLE(IID_IHXLayoutSiteGroupManager), (IHXLayoutSiteGroupManager*)this },
#if defined(HELIX_FEATURE_NESTEDMETA)
            { GET_IIDHANDLE(IID_IHXPersistenceManager), (IHXPersistenceManager*)this },
#endif /* HELIX_FEATURE_NESTEDMETA */
            { GET_IIDHANDLE(IID_IHXDriverStreamManager), (IHXDriverStreamManager*)this },
            { GET_IIDHANDLE(IID_IHXRendererUpgrade), (IHXRendererUpgrade*)this },
            { GET_IIDHANDLE(IID_IHXInternalReset), (IHXInternalReset*)this },
            { GET_IIDHANDLE(IID_IHXPlayerState), (IHXPlayerState*)this },
            { GET_IIDHANDLE(IID_IHXClientState), (IHXClientState*)this },
            { GET_IIDHANDLE(IID_IHXClientStateAdviseSinkControl), (IHXClientStateAdviseSinkControl*)this },
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
            { GET_IIDHANDLE(IID_IHXPlayerNavigator), (IHXPlayerNavigator*)this },
#endif /* HELIX_FEATURE_PLAYERNAVIGATOR */
            { GET_IIDHANDLE(IID_IHXClientStatisticsGranularity), (IHXClientStatisticsGranularity*)this },
            { GET_IIDHANDLE(IID_IHXPlayerPresentation), (IHXPlayerPresentation*)this },
#if defined(HELIX_FEATURE_RECORDCONTROL)
            { GET_IIDHANDLE(IID_IHXRecordManager), (IHXRecordManager*)this },
#endif /* HELIX_FEATURE_RECORDCONTROL */
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlayer*)this },
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
            { GET_IIDHANDLE(IID_IHXPlaybackVelocity), (IHXPlaybackVelocity*) this },
            { GET_IIDHANDLE(IID_IHXPlaybackVelocityResponse), (IHXPlaybackVelocityResponse*) this },
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
            { GET_IIDHANDLE(IID_IHXOverrideDefaultServices), (IHXOverrideDefaultServices*)this },
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
            { GET_IIDHANDLE(IID_IHXPDStatusMgr), (IHXPDStatusMgr*)this },
            { GET_IIDHANDLE(IID_IHXPDStatusObserver), (IHXPDStatusObserver*)this },
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
            { GET_IIDHANDLE(IID_IHXPresentationFeatureManager), (IHXPresentationFeatureManager*)this },
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.
#if defined(HELIX_FEATURE_PLAYBACK_MODIFIER)
            { GET_IIDHANDLE(IID_IHXPlaybackModifier), (IHXPlaybackModifier*)this },
#endif // HELIX_FEATURE_PLAYBACK_MODIFIER.
#if defined(HELIX_FEATURE_VIDEO_FRAME_STEP)
            { GET_IIDHANDLE(IID_IHXFrameStep), (IHXFrameStep*)this },
#endif 
        };
    HX_RESULT res = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    
    // if it succeeded, return immediately...
    if (res == HXR_OK)
    {
        return res;
    }
    // ...otherwise continue onward
    
    if (m_pClientViewSource &&
             IsEqualIID(riid, IID_IHXViewSourceCommand))
    {
        AddRef();
        *ppvObj = (IHXViewSourceCommand*)this;
        return HXR_OK;
    }
    else if (m_pErrorSinkControl &&
             m_pErrorSinkControl->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pAdviseSink &&
             m_pAdviseSink->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pClientStateAdviseSink &&
             m_pClientStateAdviseSink->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pAudioPlayer &&
             m_pAudioPlayer->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pMetaInfo &&
             m_pMetaInfo->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pEmbeddedUI &&
             m_pEmbeddedUI->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_PREFERENCES)
    else if (m_pPreferences &&
             m_pPreferences->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_PREFERENCES */
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    else if (m_pHyperNavigate &&
             m_pHyperNavigate->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */
#if defined(HELIX_FEATURE_ASM)
    /* m_pASM will be available on a per player basis ONLY under Load testing */
    else if (m_pASM &&
             m_pASM->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_ASM */
#if defined(HELIX_FEATURE_VIDEO)
    else if (m_pSiteManager &&
             m_pSiteManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_VIDEO */
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    else if (m_pUpgradeCollection &&
             m_pUpgradeCollection->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_AUTOUPGRADE */
#if defined(HELIX_FEATURE_PACKETHOOKMGR)
    else if (m_pPacketHookManager &&
             m_pPacketHookManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_PACKETHOOKMGR */
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    else if (m_pGroupManager &&
             m_pGroupManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */
    else if (m_pViewPortManager &&
             m_pViewPortManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_MEDIAMARKER)
    else if (m_pMediaMarkerManager &&
             m_pMediaMarkerManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
#if defined(HELIX_FEATURE_EVENTMANAGER)
    else if (m_pEventManager &&
             m_pEventManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */
#if defined(HELIX_FEATURE_NESTEDMETA)
    else if (m_pPersistentComponentManager &&
             m_pPersistentComponentManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_NESTEDMETA */
    else if (m_pPlugin2Handler &&
             m_pPlugin2Handler->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pClient &&
             m_pClient->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pEngine &&
             m_pEngine->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    /* DO NOT ADD ANY MROE QIs HERE. ADD IT BEFORE m_pClient QI*/

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXPlayer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXPlayer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    if(m_lRefCount == 0)
    {
        delete this;
    }
    return 0;
}

// *** IHXPlayer methods ***

/************************************************************************
 *      Method:
 *              IHXPlayer::GetClientEngine
 *      Purpose:
 *              Get the interface to the client engine object of which the player
 *              is a part of.
 *
 */
STDMETHODIMP HXPlayer::GetClientEngine(IHXClientEngine* &pEngine)
{
    pEngine = m_pEngine;

    if (pEngine)
    {
        pEngine->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXPlayer::Begin
 *      Purpose:
 *              Tell the player to begin playback of all its sources.
 *
 */
STDMETHODIMP HXPlayer::Begin(void)
{
    HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::Begin()", this);
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

    LeaveQuickSeekMode(TRUE);
    theErr = BeginPlayer();

    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

HX_RESULT
HXPlayer::BeginPlayer(void)
{
    HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::BeginPlayer()", this);
    HX_RESULT   theErr = HXR_OK;

    m_bUserHasCalledBegin   = TRUE;
    m_bFastStartInProgress  = FALSE;

    if (!m_bInternalPauseResume && !m_bIsFirstBegin && !m_bPaused)
    {
        return HXR_OK;
    }

    if (m_bIsLive && m_bPaused && m_bLiveSeekToBeDone)
    {
        m_ulElapsedPauseTime = CALCULATE_ELAPSED_TICKS(m_ulTimeOfPause, HX_GET_TICKCOUNT());

        /* This is an internal seek due to live pause */
        theErr = SeekPlayer(m_ulLiveSeekTime + m_ulElapsedPauseTime);
    }
    else if (m_bSeekCached)
    {
    theErr = SeekPlayer(m_ulSeekTime);
    }

    m_bPaused = FALSE;

    if (m_bIsFirstBegin)
    {
        UpdateSourceActive();
    }

    if (!theErr)
    {
        theErr = UpdateStatistics();
    }

    if (!theErr)
    {
        UpdateCurrentPlayTime( m_pAudioPlayer->GetCurrentPlayBackTime() );
    }

    /* Unregister all the sources that are not currently active */
    UnregisterNonActiveSources();

    if (!m_bIsDone)
    {
        CheckSourceRegistration();

        CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
        /* Check if we are done. This may be TRUE for empty files */
        for (;  !theErr && ndxSources != m_pSourceMap->End(); ++ndxSources)
        {
            SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
            theErr = pSourceInfo->Begin();
        }
    }

    /* Only send this OnBegin()'s if not the first begin. In the case
     * of the first begin, these are actually sent after the header's
     * arrive...
     */
    if (!theErr && !m_bIsFirstBegin && !m_bInternalReset && m_pAdviseSink)
    {
        m_pAdviseSink->OnBegin(m_ulCurrentPlayTime);
    }

    m_bIsFirstBegin = FALSE;
    m_bBeginPending = TRUE;

    m_bFastStartInProgress  = FALSE;

    if (!m_ToBeginRendererList.IsEmpty())
    {
        CheckBeginList();
    }

    return (theErr);
}

/************************************************************************
 *      Method:
 *              IHXPlayer::Stop
 *      Purpose:
 *              Tell the player to stop playback of all its sources.
 *
 */
STDMETHODIMP HXPlayer::Stop(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::Stop()", this);
    // we want to protect against the TLC opening another URL
    if (m_bSetModal)
    {
        return HXR_OK;
    }

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

    LeaveQuickSeekMode();
    StopPlayer(END_STOP);
    
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return HXR_OK;
}

void
HXPlayer::StopPlayer(EndCode endCode)
{
    StopAllStreams(endCode);
    /* Reset player condition */
    ResetPlayer();
}


/************************************************************************
 *      Method:
 *              IHXPlayer::Pause
 *      Purpose:
 *              Tell the player to pause playback of all its sources.
 *
 */
STDMETHODIMP HXPlayer::Pause(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::Pause()", this);
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;
    theErr = PausePlayer();
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

HX_RESULT
HXPlayer::PausePlayer(HXBOOL bNotifyTLC /* = TRUE*/)
{
    HX_RESULT theErr = HXR_OK;

    if (m_bIsDone)
    {
        return HXR_UNEXPECTED;
    }

    if (m_bPaused)
    {
        return HXR_OK;
    }

    m_bPaused = TRUE;

    if (m_bIsLive && !(m_bRecordServiceEnabled && m_pRecordService))
    {
        m_bLiveSeekToBeDone = TRUE;
        m_ulLiveSeekTime    = m_pAudioPlayer->GetCurrentPlayBackTime();
        m_ulTimeOfPause     = HX_GET_TICKCOUNT();
    }
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    else
    {
        m_ulTimeOfPause     = HX_GET_TICKCOUNT();
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    m_bIsPlaying            = FALSE;
    m_bTimelineToBeResumed  = TRUE;
    m_pAudioPlayer->Pause();

    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();

    for (;  !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        theErr = pSourceInfo->Pause();
    }

    /* Send OnPause to advice sink ONLY if it is not an internal Pause */
    if (bNotifyTLC && !m_bInternalReset)
    {
    SetClientState(HX_CLIENT_STATE_PAUSED);
    if (m_pAdviseSink)
    {
            m_pAdviseSink->OnPause(m_ulCurrentPlayTime);
    }
    }

    return (theErr);
}

/************************************************************************
 *      Method:
 *              IHXPlayer::Seek
 *      Purpose:
 *              Tell the player to seek in the playback timeline of all its
 *              sources.
 *
 */
STDMETHODIMP HXPlayer::Seek(ULONG32    ulTime)
{
    HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::Seek(%lu)", this, ulTime);
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

    LeaveQuickSeekMode();
    theErr = SeekPlayer(ulTime);
    
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

HX_RESULT HXPlayer::SeekPlayer(ULONG32 ulTime)
{
    HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::SeekPlayer(%lu)", this, ulTime);
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SeekPlayer(%lu)", this, ulTime);
    
    HX_RESULT theErr = HXR_OK;

    if (m_bIsDone)
    {
        return HXR_UNEXPECTED;
    }

    // do not allow seeking till we have been initialized
    if (!m_bInitialized)
    {
        return HXR_NOT_INITIALIZED;
    }

    // Cache seek value, if not in the opened state
    if (m_eClientState < HX_CLIENT_STATE_OPENED)
    {
        HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::SeekPlayer(%lu) Caching seek value", this, ulTime);
    m_bSeekCached = TRUE;
    m_ulSeekTime = ulTime;

    if (m_eClientState < HX_CLIENT_STATE_CONNECTED)
    {
        return HXR_OK;
    }
    }

    /* we do allow internal seek (done during resume after live pause)*/
    if ((m_bIsLive && !m_bLiveSeekToBeDone && !(m_bRecordServiceEnabled && m_pRecordService)) ||
        !AreAllSourcesSeekable())
    {
        /* Add error code for HXR_OPERATION_NOT_ALLOWED*/
        return HXR_FAILED;
    }

    /* Someone called Seek without calling Pause, So we will have to call
     * Pause and Resume internally
     * If seek is cached, no need to pause/resume
     */
    if (!m_bPaused && !m_bSeekCached)
    {
        m_bInternalPauseResume  = TRUE;
        // Disable the OnPause advise sink control call
        m_pAdviseSink->DisableAdviseSink(HX_ADVISE_SINK_FLAG_ONPAUSE);
    // Disable client state sink control, check if it's already disabled from SetVelocity
    HXBOOL bIsEnabled = m_pClientStateAdviseSink->IsEnabled(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
    if (bIsEnabled)
    {
        m_pClientStateAdviseSink->DisableClientStateAdviseSink(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
    }
        // Do the internal pause
        PausePlayer();
        // Re-enable the OnPause advise sink control call
        m_pAdviseSink->EnableAdviseSink(HX_ADVISE_SINK_FLAG_ONPAUSE);
    if (!bIsEnabled)
    {
        m_pClientStateAdviseSink->EnableClientStateAdviseSink(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
    }

    }

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    /* Stop prefetching */
    if (m_pNextGroupManager->GetNumSources() > 0)
    {
        m_pNextGroupManager->StopPreFetch();
        m_bLastGroup        = FALSE;
        m_bNextGroupStarted = FALSE;
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

    m_ulTimeBeforeSeek  = m_pAudioPlayer->GetCurrentPlayBackTime();
    m_ulTimeAfterSeek   = ulTime;

    //In quick seek, never honor seeks to the same time as the
    //current playback time.
    if( m_bQuickSeekMode && m_ulTimeBeforeSeek == m_ulTimeAfterSeek )
    {
        HXLOGL3(HXLOG_CORE,
                "HXPlayer[%p]::SeekPlayer() Seek to current time; Ignored",
                this, ulTime);
        return HXR_OK;
    }

    UpdateCurrentPlayTime(ulTime);

    m_pAudioPlayer->Seek(ulTime);

    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        pSourceInfo->Seek(ulTime);
    }

    if (!m_bSeekCached)
    {
    SetClientState(HX_CLIENT_STATE_SEEKING);

    if (m_pAdviseSink)
    {
        m_pAdviseSink->OnPreSeek(m_ulTimeBeforeSeek, m_ulTimeAfterSeek);
    }

    // change the state of buffering to seek
    if (m_bIsLive)
    {
        m_BufferingReason       = BUFFERING_LIVE_PAUSE;
    }
    else
    {
        m_BufferingReason       = BUFFERING_SEEK;
    }
    }

    /* Send all pre-seek events to the renderers */
    SendPreSeekEvents();

#if defined(HELIX_FEATURE_DRM)
    /* ask DRM to flush packets */
    ndxSource = m_pSourceMap->Begin();
    for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        HXSource*   pSource     = pSourceInfo->m_pSource;
        if (pSource && pSource->IsHelixDRMProtected() && pSource->GetDRM())
        {
            pSource->GetDRM()->FlushPackets(TRUE);
        }
    }
#endif /* HELIX_FEATURE_DRM */

    ndxSource = m_pSourceMap->Begin();
    for (; !theErr && ndxSource != m_pSourceMap->End();)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        HXSource * pSource = pSourceInfo->m_pSource;

        // since the map index could be screwed up by the removing of
        // the current node in AdjustSeekOnRepeatedSource()
        ++ndxSource;

        /* pSource should never be NULL */
        HX_ASSERT(pSource);
        if (pSourceInfo->m_bSeekPending || !pSourceInfo->IsInitialized())
        {
            continue;
        }

        /* This will pump all pre-seek packets to the renderer(s) */
        if (pSourceInfo->m_pPeerSourceInfo)
        {
            theErr = AdjustSeekOnRepeatedSource(pSourceInfo, ulTime);
        }
        else
        {
            theErr = pSource->DoSeek(ulTime);
        }
    }

    m_b100BufferingToBeSent = TRUE;

    // reset the player state
    UpdateSourceActive();
    m_bIsDone           = FALSE;

    if (!theErr)
    {
    // SeekCached means this is Seek that was cached prior to preentation openining.
    // Activation of download will be deferred to whatever mechanism is
    // normally used rather than forcing it here.
        if (!m_bSeekCached)
        {
            /* Start pre-fetch */
            theErr = StartDownload();
        }

        if (SUCCEEDED(theErr) && m_bInternalPauseResume)
        {
            // Disable the OnBegin advise sink call
            m_pAdviseSink->DisableAdviseSink(HX_ADVISE_SINK_FLAG_ONBEGIN);
        HXBOOL bIsEnabled = m_pClientStateAdviseSink->IsEnabled(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
        if (bIsEnabled)
        {
        m_pClientStateAdviseSink->DisableClientStateAdviseSink(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
        }
            // Call the internal begin
            theErr = Begin();
            // Re-enable the OnBegin advise sink call
            m_pAdviseSink->EnableAdviseSink(HX_ADVISE_SINK_FLAG_ONBEGIN);
        if (!bIsEnabled)
        {
        m_pClientStateAdviseSink->EnableClientStateAdviseSink(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
        }

            // Clear the internal pause/resume flag
            m_bInternalPauseResume = FALSE;
        }
    }

    m_bSeekCached = FALSE;

    return (theErr);
}


/************************************************************************
 *  Method:
 *    IHXPlayer::GetSourceCount
 *  Purpose:
 *    Returns the current number of source instances supported by
 *    this player instance.
 */
STDMETHODIMP_(UINT16) HXPlayer::GetSourceCount()
{
    /* We may have stopped the sources but not removed from the SourceMap
     * since we need to keep the renderers active till the next URL is
     * opened. In this case, report the current number of active sources
     * as zero.
     */
    if (m_bCloseAllRenderersPending)
    {
        return 0;
    }
    else
    {
        return (UINT16)m_pSourceMap->GetCount();
    }
}

/************************************************************************
 *  Method:
 *    IHXPlayer::GetSource
 *  Purpose:
 *    Returns the Nth source instance supported by this player.
 */
STDMETHODIMP HXPlayer::GetSource
(
    UINT16              nIndex,
    REF(IUnknown*)      pUnknown
)
{
    pUnknown = NULL;

    if (m_bCloseAllRenderersPending || nIndex >= m_pSourceMap->GetCount())
    {
        return HXR_INVALID_PARAMETER;
    }

    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();

    for (UINT16 i = 0; i < nIndex; i++)
    {
        ++ndxSource;
    }

    SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
    HX_ASSERT(pSourceInfo);

    HXSource* pSource = pSourceInfo->m_pSource;
    if(!pSource)
    {
        pUnknown = NULL;
        return HXR_UNEXPECTED;
    }

    return pSource ? pSource->QueryInterface(IID_IUnknown,(void**)&pUnknown) : HXR_FAIL;
}

/************************************************************************
 *  Method:
 *    IHXPlayer::GetClientContext
 *  Purpose:
 *    Called by the get the client context for this player. This is
 *    traditionally to determine called by top level client application.
 */
STDMETHODIMP HXPlayer::GetClientContext
(
    REF(IUnknown*)      pUnknown
)
{
    pUnknown = m_pClient;
    if (m_pClient)
    {
        m_pClient->AddRef();
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXPlayer::SetClientContext
 *  Purpose:
 *      Called by the client to install itself as the provider of client
 *      services to the core. This is traditionally called by the top
 *      level client application.
 */
STDMETHODIMP HXPlayer::SetClientContext(IUnknown* pUnknown)
{
    if (m_pClient) return HXR_UNEXPECTED;
    if (!pUnknown) return HXR_UNEXPECTED;
    m_pClient = pUnknown;
    m_pClient->AddRef();

    /* Override Default objects */

#if defined(HELIX_FEATURE_PREFERENCES)
    IHXPreferences* pPreferences = 0;
    if (HXR_OK == m_pClient->QueryInterface(IID_IHXPreferences, (void**) &pPreferences) ||
        HXR_OK == m_pEngine->QueryInterface(IID_IHXPreferences, (void**) &pPreferences))
    {
        HX_RELEASE(m_pPreferences);
        m_pPreferences = pPreferences;
    }
#endif /* HELIX_FEATURE_PREFERENCES */

#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    IHXHyperNavigate* pHyperNavigate = NULL;
    IHXHyperNavigateWithContext* pHyperNavigateWithContext = NULL;
    m_pClient->QueryInterface(IID_IHXHyperNavigateWithContext,
                                (void**) &pHyperNavigateWithContext);

    m_pClient->QueryInterface(IID_IHXHyperNavigate, (void**) &pHyperNavigate);
    if (pHyperNavigate == NULL)
    {
        m_pEngine->QueryInterface(IID_IHXHyperNavigate, (void**) &pHyperNavigate);
    }

    HX_ASSERT(pHyperNavigate != NULL);

    if (pHyperNavigate || pHyperNavigateWithContext)
    {
        HX_ASSERT(m_pHyperNavigate == NULL);
        HX_RELEASE(m_pHyperNavigate);

        //
        // Create new hypernaviate interface that knows how to interpret commands.
        //

        PlayerHyperNavigate* pPlayerHyperNavigate = new PlayerHyperNavigate;

        // override the default hypernavigate interface with one that can interpret commands.
        pPlayerHyperNavigate->AddRef();

        pPlayerHyperNavigate->Init((IHXPlayer*)this, pHyperNavigate, pHyperNavigateWithContext);

        m_pHyperNavigate = pPlayerHyperNavigate;

        // free memory
        HX_RELEASE(pHyperNavigate);
        HX_RELEASE(pHyperNavigateWithContext);
    }
#endif /* defined(HELIX_FEATURE_HYPER_NAVIGATE) */

#if defined(HELIX_FEATURE_VIDEO)
    IHXSiteSupplier*   pSiteSupplier = 0;
    if (HXR_OK == m_pClient->QueryInterface(IID_IHXSiteSupplier, (void**) &pSiteSupplier))
    {
        HX_RELEASE(m_pSiteSupplier);
        m_pSiteSupplier = pSiteSupplier;
    }
#endif /* HELIX_FEATURE_VIDEO */

    m_pClient->QueryInterface(IID_IHXClientRequestSink, (void**) &m_pClientRequestSink);

    /* For load testing, we have ASM Manager on a per player basis */
#if defined(HELIX_FEATURE_ASM)
    HXBOOL bLoadTest = FALSE;
    ReadPrefBOOL(m_pPreferences, "LoadTest", bLoadTest);

    if (bLoadTest)
    {
        HX_ASSERT(m_pASM == NULL);
        m_pASM = CreateBandwidthManager();

        if (m_pASM)
        {
            m_pASM->AddRef();

            HX_RELEASE(m_pBandwidthMgr);
            m_pASM->QueryInterface(IID_IHXBandwidthManager,
                                   (void**) &m_pBandwidthMgr);
        }
    }
#endif /* HELIX_FEATURE_ASM */

    UINT32 ulInterval = 0;
    if (SUCCEEDED(ReadPrefUINT32(m_pPreferences, "PlayerUpdateInterval", ulInterval)))
    {
        m_ulPlayerUpdateInterval = ulInterval;
    }
    if (SUCCEEDED(ReadPrefUINT32(m_pPreferences, "PlayerSystemTimeProcessingInterval", ulInterval)))
    {
        m_ulPlayerSystemTimeProcessingInterval = ulInterval;
    }
    if (SUCCEEDED(ReadPrefUINT32(m_pPreferences, "PlayerInterruptTimeProcessingInterval", ulInterval)))
    {
        m_ulPlayerInterruptTimeProcessingInterval = ulInterval;
    }

#if defined(HELIX_FEATURE_PREFERENCES)
    // Read the TrickPlay number of forward keyframes ahead pref
    UINT32 ulTmp = 0;
    if (SUCCEEDED(ReadPrefUINT32(m_pPreferences, NUM_FORWARD_KEYFRAMES_AHEAD_STR, ulTmp)))
    {
        m_ulNumForwardKeyFramesAhead = ulTmp;
    }
    // Read the TrickPlay number of reverse keyframes ahead pref
    ulTmp = 0;
    if (SUCCEEDED(ReadPrefUINT32(m_pPreferences, NUM_REVERSE_KEYFRAMES_AHEAD_STR, ulTmp)))
    {
        m_ulNumReverseKeyFramesAhead = ulTmp;
    }
    // When we are in reverse playback and hit the beginning
    // of the clip, this prefkey says whether we should
    // stop or whether we should start playing forward at 1x.
    HXBOOL bTmp = FALSE;
    if (SUCCEEDED(ReadPrefBOOL(m_pPreferences, STOP_WHEN_HIT_START_IN_REVERSE, bTmp)))
    {
        m_bStopWhenHitStartInReverse = bTmp;
    }

    if (SUCCEEDED(ReadPrefBOOL(m_pPreferences, "YieldLessToOthers", bTmp)))
    {
        m_bYieldLessToOthers = bTmp;
    }
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */
    
    return HXR_OK;
}




/************************************************************************
 *      Method:
 *              HXPlayer::Init
 *      Purpose:
 *              Get the interface to the client engine object of which the player
 *              is a part of.
 *
 */
STDMETHODIMP HXPlayer::Init(IHXClientEngine*  pEngine,
                             UINT32             unRegistryID,
                             CHXAudioPlayer*    pAudioPlayer)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::Init()", this);
    HX_RESULT   theErr = HXR_OK;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    IHXBuffer*  pPlayerRegName = NULL;
#endif

    m_pEngine   = (HXClientEngine *) pEngine;
    if (m_pEngine)
    {
        m_pAudioPlayer  = pAudioPlayer;
        m_pCoreMutex = m_pEngine->GetCoreMutex();

        m_pAuthenticationRequestsPending = new _CHXAuthenticationRequests((IUnknown*)(IHXPlayer*)this);

        m_pEngine->m_pPlugin2Handler->QueryInterface(IID_IHXPlugin2Handler,
                                                    (void**)&m_pPlugin2Handler);

        m_pEngine->AddRef();

#if defined(HELIX_FEATURE_SINKCONTROL)
        if (m_pAdviseSink)
        {
            m_pAdviseSink->Init(m_pEngine);
        }

        if (m_pClientStateAdviseSink)
        {
            m_pClientStateAdviseSink->Init(m_pEngine);
        }

        if (m_pErrorSinkControl)
        {
            m_pErrorSinkControl->Init(m_pEngine);
            // Add the translator as an error sink (TRUE means add)
            AddRemoveErrorSinkTranslator(TRUE);
        }
#endif /* HELIX_FEATURE_SINKCONTROL */

        theErr = m_pEngine->QueryInterface(IID_IHXScheduler,
                                            (void**) &m_pScheduler);

#if defined(HELIX_FEATURE_ASM)

        m_pEngine->QueryInterface(IID_IHXBandwidthManager,
                                            (void**) &m_pBandwidthMgr);
#endif /* HELIX_FEATURE_ASM */

        m_pEngine->QueryInterface(IID_IHXClientViewSource,
                                            (void**)&m_pClientViewSource);

        m_pEngine->QueryInterface(IID_IHXClientViewRights,
                                            (void**)&m_pClientViewRights);

        m_pEngine->QueryInterface(IID_IHXPreferredTransportManager,
                                            (void**)&m_pPreferredTransportManager);

        m_pEngine->QueryInterface(IID_IHXNetInterfaces,
                                            (void**)&m_pNetInterfaces);
#if defined(HELIX_FEATURE_REGISTRY)
        // create registry entries
        if (HXR_OK != m_pEngine->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
        {
            m_pRegistry = NULL;
        }
        else
#endif /* HELIX_FEATURE_REGISTRY */
        {
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
            char        szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

            m_pStats = new PLAYER_STATS((IUnknown*)(IHXPlayer*)this, unRegistryID);

            if (m_pRegistry &&
                HXR_OK == m_pRegistry->GetPropName(unRegistryID, pPlayerRegName))
            {
                SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.StreamSwitchOccured", pPlayerRegName->GetBuffer());
                m_pRegistry->AddInt(szRegName, 0);

                SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.Repeat", pPlayerRegName->GetBuffer());
                m_ulRepeatedRegistryID = m_pRegistry->AddComp(szRegName);

                SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.NextGroup", pPlayerRegName->GetBuffer());
                m_ulNextGroupRegistryID = m_pRegistry->AddComp(szRegName);
            }
            HX_RELEASE(pPlayerRegName);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
        }

        m_pEngine->QueryInterface(IID_IHXCookies3, (void**)&m_pCookies3);

#if defined(HELIX_FEATURE_EVENTMANAGER)
        // Get our own IUnknown interface
        IUnknown* pUnknown = NULL;
        m_pEngine->QueryInterface(IID_IUnknown, (void**) &pUnknown);
        if (pUnknown)
        {
            // Initialize the renderer event manager
            m_pEventManager = new CRendererEventManager(pUnknown);
            if (m_pEventManager) m_pEventManager->AddRef();
        }
        HX_RELEASE(pUnknown);
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */
    }
    else
    {
        theErr = HXR_INVALID_PARAMETER;
    }

    if (m_pAudioPlayer && theErr == HXR_OK)
    {
        m_pAudioPlayer->AddRef();

        theErr = m_pAudioPlayer->Init((IUnknown*) (IHXPlayer*)this);
    }

#if defined(HELIX_FEATURE_MASTERTAC)
    m_pMasterTAC = new HXMasterTAC((IUnknown*)(IHXPlayer*)this, m_pGroupManager);
    HX_ADDREF(m_pMasterTAC);

#if defined(HELIX_FEATURE_REGISTRY)
    // create master TAC object
    UINT32 playerID = 0;
    GetID(playerID);
    m_pMasterTAC->SetRegistry(m_pRegistry,playerID);
#endif /* HELIX_FEATURE_REGISTRY */

#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    if (m_pGroupManager && m_pMasterTAC)
    {
        m_pGroupManager->SetMasterTAC(m_pMasterTAC);
    }
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
#endif /* HELIX_FEATURE_MASTERTAC */

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    if (m_pNextGroupManager)
    {
        m_pNextGroupManager->Init();
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

#if defined(HELIX_FEATURE_AUTOUPGRADE)
    CreateInstanceCCF(CLSID_IHXUpgradeCollection, (void**)&m_pUpgradeCollection, (IUnknown*)(IHXClientEngine*)m_pEngine);  
#endif /* HELIX_FEATURE_AUTOUPGRADE */

    return theErr;
}


/************************************************************************
 *      Method:
 *              IHXPlayer::IsDone
 *      Purpose:
 *              Ask the player if it is done with the current presentation
 *
 */
STDMETHODIMP_ (HXBOOL) HXPlayer::IsDone(void)
{
    return m_bIsPresentationDone;
}

/************************************************************************
 *      Method:
 *              IHXPlayer::IsLive
 *      Purpose:
 *              Ask the player whether it contains the live source
 *
 */
STDMETHODIMP_ (HXBOOL) HXPlayer::IsLive(void)
{
    return m_bIsLive;
}

/************************************************************************
 *      Method:
 *              IHXPlayer::GetCurrentPlayTime
 *      Purpose:
 *              Get the current time on the Player timeline
 *
 */
STDMETHODIMP_ (ULONG32) HXPlayer::GetCurrentPlayTime(void)
{
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;
    theErr = m_pAudioPlayer->GetCurrentPlayBackTime();
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

/************************************************************************
 *      Method:
 *          IHXPlayer::AddAdviseSink
 *      Purpose:
 *          Call this method to add a client advise sink.
 *
 */
STDMETHODIMP HXPlayer::AddAdviseSink   (IHXClientAdviseSink*  pAdviseSink)
{
#if defined(HELIX_FEATURE_SINKCONTROL)
    if (m_pAdviseSink)
    {
        return m_pAdviseSink->AddAdviseSink(pAdviseSink);
    }
    else
#endif /* HELIX_FEATURE_SINKCONTROL */
    {
        return HXR_NOTIMPL;
    }
}

/************************************************************************
 *      Method:
 *          IHXPlayer::RemoveAdviseSink
 *      Purpose:
 *          Call this method to remove a client advise sink.
 */
STDMETHODIMP HXPlayer::RemoveAdviseSink(IHXClientAdviseSink*  pAdviseSink)
{
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;
#if defined(HELIX_FEATURE_SINKCONTROL)
    if (m_pAdviseSink)
    {
        theErr = m_pAdviseSink->RemoveAdviseSink(pAdviseSink);
    }
#endif /* HELIX_FEATURE_SINKCONTROL */
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

/************************************************************************
 *      Method:
 *          IHXClientStateAdviseSinkControl::AddClientStateAdviseSink
 *      Purpose:
 *          Call this method to add a client advise sink.
 *
 */
HX_RESULT HXPlayer::AddClientStateAdviseSink   (IHXClientStateAdviseSink*  pClientStateAdviseSink)
{
#if defined(HELIX_FEATURE_SINKCONTROL)
    if (m_pClientStateAdviseSink)
    {
        return m_pClientStateAdviseSink->AddClientStateAdviseSink(pClientStateAdviseSink);
    }
    else
#endif /* HELIX_FEATURE_SINKCONTROL */
    {
        return HXR_NOTIMPL;
    }
}

/************************************************************************
 *      Method:
 *          IHXClientStateAdviseSinkControl::RemoveClientStateAdviseSink
 *      Purpose:
 *          Call this method to remove a client advise sink.
 */
HX_RESULT HXPlayer::RemoveClientStateAdviseSink(IHXClientStateAdviseSink*  pClientStateAdviseSink)
{
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;
#if defined(HELIX_FEATURE_SINKCONTROL)
    if (m_pClientStateAdviseSink)
    {
        theErr = m_pClientStateAdviseSink->RemoveClientStateAdviseSink(pClientStateAdviseSink);
    }
#endif /* HELIX_FEATURE_SINKCONTROL */
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

ULONG32 HXPlayer::GetInst(void)
{
#if defined(_WIN32)
    return (ULONG32)GetModuleHandle(NULL);
#elif defined(_WIN16)
    return (ULONG32)g_hInstance;
#else
    return 0;
#endif
}

HX_RESULT
HXPlayer::SetStatsGranularity
(
    ULONG32     ulGranularity
)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    m_ulStatsGranularity = ulGranularity;

    if (m_pUpdateStatsCallback->m_bIsCallbackPending && ALLFS == m_ulStatsGranularity)
    {
        m_pUpdateStatsCallback->m_bIsCallbackPending = FALSE;
        m_pScheduler->Remove(m_pUpdateStatsCallback->m_PendingHandle);
        m_pUpdateStatsCallback->m_PendingHandle = 0;
    }
    else if (!m_pUpdateStatsCallback->m_bIsCallbackPending && ALLFS != m_ulStatsGranularity)
    {
        UpdateStatistics();
    }
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

STDMETHODIMP
HXPlayer::ClosePresentation()
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::ClosePresentation()", this);
    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

    // This stops the player if it is playing and cleans up the layout.
    StopPlayer(END_STOP);
    CloseAllRenderers(0);

#if defined(HELIX_FEATURE_VIDEO)
    if (m_pSiteSupplier && !m_bBeginChangeLayoutTobeCalled)
    {
        m_bBeginChangeLayoutTobeCalled  = TRUE;
        m_pSiteSupplier->DoneChangeLayout();
    }
#endif /* HELIX_FEATURE_VIDEO */

    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return HXR_OK;
}

STDMETHODIMP
HXPlayer::SetMinimumPreroll(UINT32 ulMinPreroll)
{
    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::SetMinimumPreroll(): preroll = %lu", this, ulMinPreroll);
    HX_RESULT hr = HXR_OK;

    m_ulMinimumTotalPreroll = ulMinPreroll;

    return hr;
}

STDMETHODIMP
HXPlayer::GetMinimumPreroll(REF(UINT32) ulMinPreroll)
{
    HX_RESULT hr = HXR_OK;

    ulMinPreroll = m_ulMinimumTotalPreroll;

    return hr;
}

HX_RESULT
HXPlayer::UpdateStatistics(void)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    HXBOOL                bUpdate = FALSE;
    ULONG32             ulPlayerTotal = 0;
    ULONG32             ulPlayerReceived = 0;
    ULONG32             ulPlayerNormal = 0;
    ULONG32             ulPlayerRecovered = 0;
    ULONG32             ulPlayerDuplicate = 0;
    ULONG32             ulPlayerOutOfOrder = 0;
    ULONG32             ulPlayerFilledBufferSize = 0;
    ULONG32             ulPlayerLost = 0;
    ULONG32             ulPlayerLate = 0;
    UINT32              ulPlayerTotal30 = 0;
    UINT32              ulPlayerLost30 = 0;
    ULONG32             ulPlayerResendRequested = 0;
    ULONG32             ulPlayerResendReceived = 0;

    ULONG32             ulPlayerBandwidth = 0;
    ULONG32             ulPlayerCurBandwidth = 0;
    ULONG32             ulPlayerAvgBandwidth = 0;

    INT32               lAvgLatency = 0;
    INT32               lHighLatency = 0;
    INT32               lLowLatency = 0xFFFF;

    SourceInfo*         pSourceInfo = NULL;
    RendererInfo*       pRenderInfo = NULL;
    IHXStatistics*     pStatistics = NULL;
    UINT16              uBufferingMode = 0;

    CHXMapPtrToPtr::Iterator    ndxSource;
    CHXMapLongToObj::Iterator   ndxRend;

    if (!m_bInitialized || m_bPaused)
    {
        goto exit;
    }

    // update statistics
    ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        pSourceInfo = (SourceInfo*) (*ndxSource);

        // notify the renderer to update its statistics
        ndxRend = pSourceInfo->m_pRendererMap->Begin();
        for (; ndxRend != pSourceInfo->m_pRendererMap->End(); ++ndxRend)
        {
            pRenderInfo = (RendererInfo*)(*ndxRend);

            if (pRenderInfo->m_pRenderer &&
                HXR_OK == pRenderInfo->m_pRenderer->
                                QueryInterface(IID_IHXStatistics,
                                                (void**)&pStatistics))
            {
                pStatistics->UpdateStatistics();

                pStatistics->Release();
                pStatistics = NULL;
            }
        }

        // update each source
        if (pSourceInfo->m_pSource              &&
            HXR_OK == pSourceInfo->m_pSource->UpdateStatistics())
        {
            ulPlayerTotal           += pSourceInfo->m_pSource->m_pStats->m_pTotal->GetInt();
            ulPlayerReceived        += pSourceInfo->m_pSource->m_pStats->m_pReceived->GetInt();
            ulPlayerNormal          += pSourceInfo->m_pSource->m_pStats->m_pNormal->GetInt();
            ulPlayerRecovered       += pSourceInfo->m_pSource->m_pStats->m_pRecovered->GetInt();
            ulPlayerDuplicate       += pSourceInfo->m_pSource->m_pStats->m_pDuplicate->GetInt();
            ulPlayerOutOfOrder      += pSourceInfo->m_pSource->m_pStats->m_pOutOfOrder->GetInt();
            ulPlayerFilledBufferSize += pSourceInfo->m_pSource->m_pStats->m_pFilledBufferSize->GetInt();
            ulPlayerLost            += pSourceInfo->m_pSource->m_pStats->m_pLost->GetInt();
            ulPlayerLate            += pSourceInfo->m_pSource->m_pStats->m_pLate->GetInt();
            ulPlayerResendRequested += pSourceInfo->m_pSource->m_pStats->m_pResendRequested->GetInt();
            ulPlayerResendReceived  += pSourceInfo->m_pSource->m_pStats->m_pResendReceived->GetInt();

            ulPlayerTotal30         += pSourceInfo->m_pSource->m_pStats->m_pTotal30->GetInt();
            ulPlayerLost30          += pSourceInfo->m_pSource->m_pStats->m_pLost30->GetInt();

            ulPlayerBandwidth       += pSourceInfo->m_pSource->m_pStats->m_pClipBandwidth->GetInt();
            ulPlayerCurBandwidth    += pSourceInfo->m_pSource->m_pStats->m_pCurBandwidth->GetInt();
            ulPlayerAvgBandwidth    += pSourceInfo->m_pSource->m_pStats->m_pAvgBandwidth->GetInt();

            lAvgLatency             += pSourceInfo->m_pSource->m_pStats->m_pAvgLatency->GetInt();

            if (lHighLatency < pSourceInfo->m_pSource->m_pStats->m_pHighLatency->GetInt())
            {
                lHighLatency = pSourceInfo->m_pSource->m_pStats->m_pHighLatency->GetInt();
            }

            if (lLowLatency > pSourceInfo->m_pSource->m_pStats->m_pLowLatency->GetInt())
            {
                lLowLatency = pSourceInfo->m_pSource->m_pStats->m_pLowLatency->GetInt();
            }

            if (uBufferingMode < (UINT16) pSourceInfo->m_pSource->m_pStats->m_pBufferingMode->GetInt())
            {
                uBufferingMode = (UINT16) pSourceInfo->m_pSource->m_pStats->m_pBufferingMode->GetInt();
            }

            if (pSourceInfo->m_pSource->m_pStatsManager)
            {
                pSourceInfo->m_pSource->m_pStatsManager->Copy();
            }
        }
    }

    bUpdate = SetIntIfNecessary(m_pStats->m_pTotal, (INT32)ulPlayerTotal);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pReceived, (INT32)ulPlayerReceived);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pNormal, (INT32)ulPlayerNormal);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pRecovered, (INT32)ulPlayerRecovered);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pDuplicate, (INT32)ulPlayerDuplicate);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pOutOfOrder, (INT32)ulPlayerOutOfOrder);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pFilledBufferSize, (INT32)ulPlayerFilledBufferSize);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pLost, (INT32)ulPlayerLost);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pLate, (INT32)ulPlayerLate);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pTotal30, (INT32)ulPlayerTotal30);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pLost30, (INT32)ulPlayerLost30);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pResendRequested, (INT32)ulPlayerResendRequested);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pResendReceived, (INT32)ulPlayerResendReceived);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pClipBandwidth, (INT32)ulPlayerBandwidth);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pCurBandwidth, (INT32)ulPlayerCurBandwidth);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pAvgBandwidth, (INT32)ulPlayerAvgBandwidth);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pAvgLatency, (INT32)lAvgLatency);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pHighLatency, (INT32)lHighLatency);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pLowLatency, (INT32)lLowLatency);
    bUpdate |= SetIntIfNecessary(m_pStats->m_pBufferingMode, (INT32)uBufferingMode);

    if (bUpdate || m_bForceStatsUpdate)
    {
        if (m_pAdviseSink)
        {
            m_pAdviseSink->OnStatisticsChanged();
        }

        m_bForceStatsUpdate = FALSE;
    }

exit:
    if (!m_pUpdateStatsCallback->m_bIsCallbackPending && ALLFS != m_ulStatsGranularity)
    {
        m_pUpdateStatsCallback->m_bIsCallbackPending  = TRUE;
        m_pUpdateStatsCallback->m_PendingHandle = m_pScheduler->RelativeEnter(m_pUpdateStatsCallback,
                                                                m_ulStatsGranularity);
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return HXR_OK;
}


HX_RESULT
HXPlayer::SetupAudioPlayer(UINT32 &ulSchedulerFlags)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::SetupAudioPlayer()", this);
    HX_RESULT theErr = HXR_OK;

    HX_ASSERT(m_bInitialized);

#if defined(HELIX_CONFIG_AUDIO_ON_CORE_THREAD)
#if defined(THREADS_SUPPORTED) && !defined(_MAC_UNIX)
    if( m_bUseCoreThread && !m_pEngine->AtInterruptTime() )
    {
    // Schedule player again looking for invocation on core thread
    ulSchedulerFlags = (PLAYER_SCHEDULE_INTERRUPT_ONLY | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::SetupAudioPlayer() Wrong Thread: Bailing out", this);
        return HXR_RETRY;
    }
#endif //_WIN32
#endif //HELIX_CONFIG_AUDIO_ON_CORE_THREAD    

    m_bSetupToBeDone = FALSE;

    PrepareAudioPlayer();

#if defined(THREADS_SUPPORTED) && !defined(_MAC_UNIX)
    if (m_bUseCoreThread && m_pEngine->AtInterruptTime())
    {
        m_pAudioPlayer->UseCoreThread();
    }
#endif
    theErr = m_pAudioPlayer->Setup(m_ulLowestGranularity);

    if (theErr)
    {
        SetLastError(theErr);
    }

    if (!theErr)
    {
        m_bPostSetupToBeDone = TRUE;
    }

    return theErr;
}

HX_RESULT
HXPlayer::PrepareAudioPlayer()
{
    return HXR_OK;
}

HX_RESULT HXPlayer::ProcessIdle(HXBOOL bFromInterruptSafeChain)
{
    // When we are called from interrupt safe chain, we could be called at
    // interrupt and system time.
    ULONG32 ulTime;
    StartYield(&ulTime);
    HXBOOL bAtInterrupt = (bFromInterruptSafeChain && m_pEngine->AtInterruptTime());

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Entering: Init=%c PostSetupDue=%c InterrTime=%c",
    this,
    m_bInitialized ? 'T' : 'F',
    m_bPostSetupToBeDone ? 'T' : 'F',
    bAtInterrupt ? 'T' : 'F');

    UINT32 ulSchedulerFlags = PLAYER_SCHEDULE_DEFAULT;

    if ((!m_bInitialized || m_bPostSetupToBeDone ) && bAtInterrupt)
    {
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Non-interr time needed: Bailing out", this);
        return HXR_OK;
    }

    if (m_ulCoreLockCount)
    {
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Core Locked: Bailing out", this);
        SchedulePlayer(ulSchedulerFlags);
        return HXR_OK;
    }

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

#if defined (HELIX_CONFIG_YIELD_LESS_TO_OTHERS)
    UINT32 ulLoopEntryTime = 0;
#else   // HELIX_CONFIG_YIELD_LESS_TO_OTHERS
    UINT32 ulLoopEntryTime = m_bYieldLessToOthers ? 0 : HX_GET_BETTERTICKCOUNT();
#endif  // HELIX_CONFIG_YIELD_LESS_TO_OTHERS

#if 0
#if defined(_WIN32) && defined(_DEBUG)
    GetChangesToWorkingSet();
#endif
#endif

    HX_RESULT           theErr              = HXR_OK;
    UINT16              uLowestBuffering    = 100;

    SourceInfo*         pSourceInfo         = NULL;
    RendererInfo*       pRendInfo           = NULL;
    IHXRenderer*        pRenderer           = NULL;
    HXBOOL      bDone               = FALSE;
    HXBOOL      bIsFirst            = TRUE;

#if defined(HELIX_FEATURE_AUTOUPGRADE)
    HXBOOL      bReadyToUpgrade     = TRUE;
#endif

    HXBOOL      bSuppressErrorReporting = FALSE;
    UINT32              ulNumStreamsToBeFilled = 0;
    UINT16              unStatusCode        = 0;
    UINT16              unPercentDone       = 0;
    IHXBuffer*          pStatusDesc         = NULL;

    HXBOOL      bAttemptedResume    = FALSE;
    HXBOOL      bIsBuffering        = FALSE;
    HXBOOL      bWasBuffering;
    
    CHXMapPtrToPtr::Iterator    ndxSource;
    CHXMapLongToObj::Iterator   ndxRend;

    m_bSourceMapUpdated = FALSE;

    if (m_bIsDone)
    {
        goto exitRoutine;
    }

    //check to see if we have a queued up seek request.
    if (kNoValue != m_ulSeekQueue)
    {
        QuickSeek(m_ulSeekQueue);
    }

#ifdef _MACINTOSH
    /* check InternalPause() for details */
    if (m_bPendingAudioPause && !bAtInterrupt)
    {
    m_pAudioPlayer->Pause();
    }
#endif  // _MACINTOSH

    ndxSource = m_pSourceMap->Begin();

    for (;!theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        pSourceInfo     = (SourceInfo*)(*ndxSource);

        /* Do not call Process source at interrupt time unless
         * it is initialized.
         */
        if (pSourceInfo->m_pSource &&
            (pSourceInfo->m_pSource->IsInitialized() ||
            !bAtInterrupt))
        {
            // XXX HP may need to rework
            if (!pSourceInfo->m_bDone)
            {
                theErr = pSourceInfo->m_pSource->ProcessIdle(ulLoopEntryTime);
                if( theErr == HXR_OUTOFMEMORY )
                {
                    goto exitRoutine;
                }
            }

            theErr = SpawnSourceIfNeeded(pSourceInfo);
            if( theErr == HXR_OUTOFMEMORY )
            {
                goto exitRoutine;
            }
            else
            {
                theErr = HXR_OK;    // filter out HXR_NOTIMPL
            }
        }

        if (pSourceInfo->m_pPeerSourceInfo                      &&
            pSourceInfo->m_pPeerSourceInfo->m_pSource           &&
            !pSourceInfo->m_pPeerSourceInfo->m_bDone            &&
            (pSourceInfo->m_pPeerSourceInfo->m_pSource->IsInitialized() ||
            !bAtInterrupt))
        {
            theErr = pSourceInfo->m_pPeerSourceInfo->m_pSource->ProcessIdle();
            if( theErr == HXR_OUTOFMEMORY )
            {
                goto exitRoutine;
            }
        }
    }

#if defined(HELIX_FEATURE_PREFETCH)
    if (m_pPrefetchManager &&
        m_pPrefetchManager->GetNumSources() > 0)
    {
        m_pPrefetchManager->ProcessIdle();
    }
#endif /* HELIX_FEATURE_PREFETCH */

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    if (m_pNextGroupManager->GetNumSources() > 0)
    {
        m_pNextGroupManager->ProcessIdle();
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

    // check the status first
    if (!m_bContactingDone &&
        HXR_OK == GetStatus(unStatusCode, pStatusDesc, unPercentDone))
    {
        if (HX_STATUS_CONTACTING == unStatusCode && pStatusDesc && m_pAdviseSink)
        {
            m_pAdviseSink->OnContacting((const char*)pStatusDesc->GetBuffer());
        }
        else if (HX_STATUS_INITIALIZING != unStatusCode)
        {
            m_bContactingDone = TRUE;
        }
        HX_RELEASE(pStatusDesc);
    }

    // initialize renderers if not done yet..
    // this involves reading headers from source object
    // creating the right renderers for each stream and pass this
    // header to the renderer.
    if (!m_bInitialized)
    {
        /* A temporary hack till we change SMIL fileformat
         * to send Layout info in Header
         */
        m_bIsSmilRenderer = m_bSetupLayoutSiteGroup;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Initialize Renderers: Start", this);

        theErr = InitializeRenderers();

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Initialize Renderers: End", this);

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SetupLayout: Start", this);

#if defined(HELIX_FEATURE_VIDEO)
        if (!m_bPlayerWithoutSources && !theErr)
        {
            SetupLayout(/*!m_bSetupLayoutSiteGroup*/ FALSE);
            if (m_bIsSmilRenderer == m_bSetupLayoutSiteGroup &&
                m_pSiteSupplier && !m_bBeginChangeLayoutTobeCalled)
            {
                m_bBeginChangeLayoutTobeCalled  = TRUE;
                m_pSiteSupplier->DoneChangeLayout();
            }
        }
#endif /* HELIX_FEATURE_VIDEO */

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SetupLayout: End", this);
    }

    // Perform prefetching if we are in the connected state and there is no halt set or halt has been cleared
    if (!theErr && m_bInitialized &&
    m_eClientState == HX_CLIENT_STATE_CONNECTED &&
    m_eClientStateStatus == HX_CLIENT_STATE_STATUS_ACTIVE)
    {
        HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::ProcessIdle() Prefetching", this);
    m_eClientStateStatus = HX_CLIENT_STATE_STATUS_ACTIVE;
    SetClientState(HX_CLIENT_STATE_OPENING);

    m_bSetupToBeDone = TRUE;

    AdjustPresentationTime();
    }

    if (m_bSetupToBeDone)
    {
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SetupAudioPlayer: Start", this);

    theErr = SetupAudioPlayer(ulSchedulerFlags);

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SetupAudioPlayer: End", this);

    if (theErr == HXR_RETRY)
    {
        theErr = HXR_OK;
        goto exitRoutine;
    }
    }

    if (m_bPostSetupToBeDone)
    {
        m_bPostSetupToBeDone = FALSE;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() PostSetup: Start", this);

        /* Initialize audio services */
        if (!theErr && m_bInitialized)
        {
            m_bIsPresentationClosedToBeSent = TRUE;
        SetClientState(HX_CLIENT_STATE_OPENED);
            if (m_pAdviseSink)
            {
                m_pAdviseSink->OnPresentationOpened();
            }
        // Check if seek and velocity were cached
        if (m_bPlaybackVelocityCached)
        {
        theErr = SetVelocity(m_lPlaybackVelocityCached, m_bKeyFrameModeCached, m_bAutoSwitchCached);
        m_bPlaybackVelocityCached = FALSE;
        }
        if (m_bSeekCached && !theErr)
        {
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SeekPlayer: Start", this);

        theErr = SeekPlayer(m_ulSeekTime);

        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SeekPlayer: End", this);
        }
        }

        if (!theErr && m_bInitialized)
        {
            SetMinimumPushdown();
        }

        if (!theErr && m_bInitialized && !m_bIsFirstBegin)
        {
            CheckSourceRegistration();

            ndxSource = m_pSourceMap->Begin();
            for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
            {
                SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSource);
                HXSource* pSource = pSourceInfo->m_pSource;
                
                if (pSource)
                {
                    theErr = pSource->DoResume(ulLoopEntryTime,
                           GetPlayerProcessingInterval(bFromInterruptSafeChain));
                }
                else
                {
                    /* pSource should never be NULL */
                    HX_ASSERT(FALSE);
                }
            }
        }

        /* Start Downloading even if we have not been issued a
         * Begin() command
         */
        if (!theErr && m_bInitialized && m_bIsFirstBegin && !m_bIsDone)
        {
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() StartDownload: Start", this);

            theErr = StartDownload();

        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() StartDownload: End", this);
        }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() PostSetup: End", this);
    }

    // if it is still not initialized check error code and return
    if (theErr || !m_bInitialized || m_eClientStateStatus == HX_CLIENT_STATE_STATUS_HALTED)
    {
        goto exitRoutine;
    }

    UpdateCurrentPlayTime(m_pAudioPlayer->GetCurrentPlayBackTime());

    if (!m_ToBeginRendererList.IsEmpty())
    {
    CheckBeginList();
    }

    do
    {
    UINT32 ulProcessingTimeAllowance = GetPlayerProcessingInterval(bFromInterruptSafeChain);

    while (!theErr && !bDone && m_uNumSourcesActive > 0)
    {
        bWasBuffering = m_bIsBuffering;
        bIsBuffering = FALSE;
        uLowestBuffering = 100;
        ulNumStreamsToBeFilled = 0;

        ndxSource = m_pSourceMap->Begin();
        for (;!theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
        {
        pSourceInfo = (SourceInfo*)(*ndxSource);

        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SourceInfo ProcessIdle: Start", this);
        theErr = pSourceInfo->ProcessIdle(
            ulNumStreamsToBeFilled,
            bIsBuffering, uLowestBuffering,
            ulLoopEntryTime,
            ulProcessingTimeAllowance);
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SourceInfo ProcessIdle: End", this);

        if (pSourceInfo->m_pPeerSourceInfo)
        {
            HXBOOL tmp_bIsBuffering = FALSE;
            UINT32 tmp_ulNumStreamsToBeFilled = 0;
            UINT16 tmp_uLowestBuffering = 100;

            pSourceInfo->m_pPeerSourceInfo->ProcessIdle(
            tmp_ulNumStreamsToBeFilled,
            tmp_bIsBuffering, tmp_uLowestBuffering,
            ulLoopEntryTime,
            ulProcessingTimeAllowance);
        }

        // From the player perspective, we are buffering only if the
        // we are not playing.  That is, even if one of the source is
        // buffering, as long the source did not pause the player,
        // this background buffering not relevant at player level.
        bIsBuffering = (bIsBuffering && !m_bIsPlaying);

        /* Source was added during ProcessIdle.
         * Start all over again
         */
        if (m_bSourceMapUpdated)
        {
            m_bIsBuffering = bIsBuffering;
            bDone = TRUE;
            break;
        }
        }

        m_bIsBuffering = bIsBuffering;

        if (bIsBuffering && !bWasBuffering)
        {
        ulProcessingTimeAllowance = GetPlayerProcessingInterval(bFromInterruptSafeChain);
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Buffering...", this);
        }

        if (bWasBuffering && !bIsBuffering)
        {
        ulProcessingTimeAllowance = GetPlayerProcessingInterval(bFromInterruptSafeChain);
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Buffering completed", this);
        }

        if (bIsBuffering)
        {
        ndxSource = m_pSourceMap->Begin();
        for (;!theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
        {
            pSourceInfo = (SourceInfo*)(*ndxSource);

            ndxRend = pSourceInfo->m_pRendererMap->Begin();

            for (;!theErr && ndxRend != pSourceInfo->m_pRendererMap->End();
            ++ndxRend)
            {
            pRendInfo   = (RendererInfo*)(*ndxRend);
            pRenderer   = pRendInfo->m_pRenderer;

            if((pRendInfo->m_bInterruptSafe || !bAtInterrupt)
                && pRenderer)
            {
                pRenderer->OnBuffering(m_BufferingReason, uLowestBuffering);
            }
            }
        }

        m_b100BufferingToBeSent = TRUE;
        SetClientState(HX_CLIENT_STATE_PREFETCHING);
        if (m_pAdviseSink)
        {
            m_pAdviseSink->OnBuffering(m_BufferingReason, uLowestBuffering);
        }
        }
        else
        {
        if (bWasBuffering)
        {
            m_ulBufferingCompletionTime = HX_GET_BETTERTICKCOUNT();
        }

        if ((!bAttemptedResume) && (!m_bSourceMapUpdated))
        {
            if (!m_bIsPlaying)
            {
            // The below test if for safety only.  In case we somehow missed to obtain
            // the buffering completion time, obtain it now.
            if (m_ulBufferingCompletionTime == 0)
            {
                m_ulBufferingCompletionTime = HX_GET_BETTERTICKCOUNT();
            }

            // We are not buffering any more, still not playing and we have not attempted to
            // resume playback - try to resume.
            HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Buffering done: Trying to resume...", this);
            if (m_bQuickSeekMode)
            {
                // If we are in quick see mode - there is no need to come back and fill in more
                // data since we met the buffering requirement and we are not going to resume
                // the timeline since quick seek mode implies we are paused.
                // If we are in quick seek mode, we'll not declare "bDone" in order to loop
                // back after resumption and fill in some more data as long as our processing
                // slice alotment allows us to do so.
                bDone = TRUE;
            }
            break;
            }
        }
        }

        if (!theErr)
        {
        // We do not limit event processing below by ulLoopEntryTime since
        // we process current events after every fetching of events (at most one
        // event per stream is feteched).  Thus, we simply dispatch what was
        // retrieved.
        theErr = ProcessCurrentEvents(0, bAtInterrupt, bFromInterruptSafeChain);
        }

        if (theErr || m_bSourceMapUpdated)
        {
        bDone = TRUE;
        ulSchedulerFlags |= PLAYER_SCHEDULE_IMMEDIATE;
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SourceMapUpdated: Skipping to exit...", this);
        goto exitRoutine;
        }

        // If we have no more streams to fill or we have exceeded
        // our processing slice window, break out of the loop.
        // We'll need next slice to continue.
        if ((ulNumStreamsToBeFilled == 0) ||
        ((ulLoopEntryTime != 0) &&
         ((HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime) > GetPlayerProcessingInterval(bFromInterruptSafeChain))))
        {
        bDone = TRUE;
        if (ulNumStreamsToBeFilled)
        {
            HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() SourceInfo servicing CPU use timeout: Time=%lu Allowed=%lu", 
                this, 
                HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime,
                GetPlayerProcessingInterval(bFromInterruptSafeChain));
        }
        break;
        }
        YieldIfRequired(&ulTime);			
    }

    // SPECIAL CASE:
    // the player received all the packets(m_uNumSourcesActive is 0) and
    // EndOfPacket() hasn't been sent to the renderer yet,
    // BUT the renderer still calls ReportRebufferStatus()
    // we need to make sure the rebuffering is done before calling resume.
    if (m_uNumSourcesActive == 0)
    {
        ndxSource = m_pSourceMap->Begin();
        for (;!theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
        {
        pSourceInfo = (SourceInfo*)(*ndxSource);

        if (!pSourceInfo->IsRebufferDone())
        {
            bIsBuffering = TRUE;
            m_bIsBuffering = TRUE;
            break;
        }
        }
    }

    /* Stop Downloading if we have not been issued a
    * Begin() command and we are done with Buffering.
    */
    if (!theErr && !bIsBuffering && m_bFastStartInProgress && !m_bIsDone)
    {
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() PauseDownload: Start", this);

        theErr = PauseDownload();

        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() PauseDownload: End", this);
    }

    // Process Current Events Due...
    if (!theErr && (m_uNumSourcesActive == 0))
    {
        // If we have sources active, event processing will occur above as
        // part of servicing source's process idle.
        // We do not limit event processing below by ulLoopEntryTime since
        // we process current events after every fetching of events (see above)
        // and at most one event per stream is feteched.  
        // Thus, we simply dispatch the events retrieved.
        theErr = ProcessCurrentEvents(0, bAtInterrupt, bFromInterruptSafeChain);
    }

    // repeat source specific
    SwitchSourceIfNeeded();

    if (!theErr && !bIsBuffering)
    {
        theErr = CheckForAudioResume(ulSchedulerFlags);

        if (theErr == HXR_RETRY)
        {
        theErr = HXR_OK;
        goto exitRoutine;
        }
    }

    bAttemptedResume = TRUE;
    } while (!theErr && !bDone && (m_uNumSourcesActive > 0));

    if (!theErr && !m_bLastGroup && !m_bNextGroupStarted && !bAtInterrupt)
    {
        CheckToStartNextGroup();
    }

    /* Check if live stream has ended */
    if (!theErr && m_bInitialized && !m_bPlayerWithoutSources &&
        ((!m_bHasSubordinateLifetime && !m_bIsLive && (!m_ulPresentationDuration ||
        (m_ulPresentationDuration <= m_ulCurrentPlayTime))) ||
        (m_bIsLive && AreAllPacketsSent())) &&
        m_uNumSourcesActive == 0)
    {
        /* If there are any sources that MUST be initialized before playback
         * begins/stops, we should not end the presentation here. Instead, wait for
         * the stream headers to come down...
         */
        if ((m_uNumSourceToBeInitializedBeforeBegin > 0) &&
            m_ulPresentationDuration)
        {
            InternalPause();
        }
        else if (!m_ulPresentationDuration ||
                 (m_ulPresentationDuration <= m_ulCurrentPlayTime))
        {
            if (ScheduleOnTimeSync())
            {
                // schedule a system callback to ensure
                // the renderers receive OnTimeSync() on its duration
                // AND the clip ends ASAP
        SchedulePlayer(PLAYER_SCHEDULE_MAIN | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);
                goto cleanup;
            }
            else
            {
                m_bIsDone = TRUE;

                m_pAudioPlayer->DonePlayback();

                /* This assert is to find bugs in fileformats which place invalid ts
                 * (ts > m_ulPresentationDuration) on the packets.
                 */
                HX_ASSERT(AreAllPacketsSent() == TRUE);
            }
        }
    }

    if (m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL &&
        m_bKeyFrameMode)
    {
        UINT32 ulOrigTime = m_pAdviseSink->GetOriginalTime(m_ulCurrentPlayTime);
        if (m_lPlaybackVelocity > 0)
        {
            // Our warped time is greater than the
            // presentation duration, so stop.
            if (ulOrigTime > m_ulPresentationDuration)
            {
                // Stop the player
                m_bIsDone = TRUE;
                m_pAudioPlayer->DonePlayback();
            }
        }
        else
        {
            if (ulOrigTime == 0)
            {
                // We are playing in reverse and we hit the
                // start of the clip. Is the pref set to
                // stop or reset?
                if (m_bStopWhenHitStartInReverse)
                {
                    // Stop the player
                    m_bIsDone = TRUE;
                    m_pAudioPlayer->DonePlayback();
                }
                else
                {
                    // Start forward playback at 1x
                    SetVelocity(100, FALSE, FALSE);
                }
            }
        }
    }

exitRoutine:

#if defined(HELIX_FEATURE_AUTOUPGRADE)
    if(m_pUpgradeCollection && m_pUpgradeCollection->GetCount() > 0 && !m_pEngine->AtInterruptTime())
    {
        // Request an upgrade
        IHXUpgradeHandler* pUpgradeHandler = NULL;
        if(m_pClient)
            m_pClient->QueryInterface(IID_IHXUpgradeHandler, (void**)&pUpgradeHandler);
        if(!pUpgradeHandler)
        {
            // In case of clients with no IHXUpgradeHandler support
            // just remove all the upgrade collectopn components
            m_pUpgradeCollection->RemoveAll();
        theErr = HXR_MISSING_COMPONENTS;
        }
        else
        {
            // see if we should send an upgrade request only if all the sources
            // have been initialized...
            ndxSource = m_pSourceMap->Begin();
            for (;ndxSource != m_pSourceMap->End(); ++ndxSource)
            {
                pSourceInfo = (SourceInfo*)(*ndxSource);
                if (!pSourceInfo->m_bLoadPluginAttempted &&
                    pSourceInfo->m_pSource &&
                    pSourceInfo->m_pSource->GetLastError() == HXR_OK)
                {
                    bReadyToUpgrade = FALSE;
                    break;
                }
            }

            if (bReadyToUpgrade)
            {
                // Request an upgrade
                IHXUpgradeHandler* pUpgradeHandler = NULL;
                if(m_pClient)
                    m_pClient->QueryInterface(IID_IHXUpgradeHandler, (void**)&pUpgradeHandler);
                if(pUpgradeHandler)
                {
                    HX_RESULT hr = HXR_MISSING_COMPONENTS;
                    m_bIsPresentationClosedToBeSent = TRUE;

                    HX_RESULT nUpgradeResult = pUpgradeHandler->RequestUpgrade(m_pUpgradeCollection, FALSE);
                    if(nUpgradeResult == HXR_OK || nUpgradeResult == HXR_CANCELLED)
                    {
                        // We want to stop playback but we dont want error
                        // to be reported untill upgrade tries to fix situation.
                        hr = HXR_OK;
                        bSuppressErrorReporting = TRUE;
                    }

                    m_bIsDone = TRUE;
                    m_pAudioPlayer->DonePlayback();
                    m_LastError = hr;
                }

                HX_RELEASE(pUpgradeHandler);
                m_pUpgradeCollection->RemoveAll();
            }
            else
            {
                /* We are not done yet... Wait till all the sources have been
                 * attempted to load required plugins
                 */
                m_bIsDone = FALSE;
            }
        }
    }
#endif /* HELIX_FEATURE_AUTOUPGRADE */

    if (!theErr && m_pSDPURLList && m_pSDPURLList->GetCount() && !m_pEngine->AtInterruptTime())
    {
        theErr = DoSDPURL();
    }

    if (!theErr && m_pAltURLs && m_pAltURLs->GetCount() && !m_pEngine->AtInterruptTime())
    {
        theErr = DoAltURL();
    }

    if (!theErr && m_pOppositeHXSourceTypeRetryList && m_pOppositeHXSourceTypeRetryList->GetCount() &&
        !m_pEngine->AtInterruptTime())
    {
        theErr = DoOppositeHXSourceRetry();
    }

    if (!theErr && m_pRedirectList && m_pRedirectList->GetCount() && !m_pEngine->AtInterruptTime())
    {
        theErr = DoRedirect();
    }

    if (!theErr && m_bIsDone && !m_bTimeSyncLocked && !m_pEngine->AtInterruptTime())
    {
        if (m_LastError != HXR_OK)
        {
            m_bIsDone = FALSE;
            PausePlayer(FALSE);
            m_bIsDone = TRUE;
            ActualReport(m_LastSeverity, m_LastError,
                   m_ulLastUserCode, m_pLastUserString, m_pLastMoreInfoURL);
            ResetError();
        }

        PlayNextGroup();
    }

    if (!theErr)
    {
        ProcessIdleExt();
    }

    SchedulePlayer(ulSchedulerFlags);

    /* this error is crucial...need to be reported upto ragui...*/
    if (theErr && !bSuppressErrorReporting)
    {
        SetLastError(theErr);
    }

cleanup:

    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessIdle() Exiting", this);

    return theErr;
}

HX_RESULT
HXPlayer::ProcessIdleExt(void)
{
    return HXR_OK;
}

HX_RESULT
HXPlayer::DoAltURL(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::DoAltURL()", this);
#if defined(HELIX_FEATURE_ALT_URL)
    HX_RESULT           hr                  = HXR_OK;
    SourceInfo*         pMainSourceInfo     = NULL;

    ResetError();

    while (!hr && m_pAltURLs->GetCount())
    {
        pMainSourceInfo = (SourceInfo*) m_pAltURLs->RemoveHead();
        hr = DoURLOpenFromSource(pMainSourceInfo, URL_ALTERNATE);
    }

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ALT_URL */
}

HX_RESULT
HXPlayer::DoOppositeHXSourceRetry(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::DoOppositeHXSourceRetry()", this);
    HX_RESULT   retVal          = HXR_OK;
    SourceInfo* pMainSourceInfo = NULL;

    ResetError();

    while (SUCCEEDED(retVal) && m_pOppositeHXSourceTypeRetryList->GetCount())
    {
        pMainSourceInfo = (SourceInfo*) m_pOppositeHXSourceTypeRetryList->RemoveHead();
        retVal = DoURLOpenFromSource(pMainSourceInfo, URL_OPPOSITE_HXSOURCE);
    }

    return retVal;
}

HX_RESULT
HXPlayer::DoSDPURL(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::DoSDPURL()", this);
    HX_RESULT               hr                          = HXR_OK;
    SourceInfo*             pMainSourceInfo             = NULL;

    pMainSourceInfo = (SourceInfo*) m_pSDPURLList->RemoveHead();
    hr = DoURLOpenFromSource(pMainSourceInfo, URL_SDP);

    return hr;
}

HX_RESULT
HXPlayer::DoRedirect(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::DoRedirect()", this);
    HX_RESULT               hr                          = HXR_OK;
    HXBOOL                    bNoRedirectSupport          = FALSE;
    HXBOOL                    bBegin                      = m_bUserHasCalledBegin;
    HXBOOL                    bRAMOnly                    = TRUE;
    SourceInfo*             pMainSourceInfo             = NULL;
    IHXValues*              pRequestHeaders             = NULL;
    HXPersistentComponent*  pRootPersistentComponent    = NULL;
    HXPersistentComponent*  pPersistentComponent        = NULL;

    m_bDoRedirect = TRUE;

    pMainSourceInfo = (SourceInfo*) m_pRedirectList->RemoveHead();

    if (!pMainSourceInfo->m_pSource->IsInitialized())
    {
        hr = DoURLOpenFromSource(pMainSourceInfo, URL_REDIRECTED);
    }
    else
    {   
#if defined(HELIX_FEATURE_NESTEDMETA)
        pRootPersistentComponent = m_pPersistentComponentManager->m_pRootPersistentComponent;
        if (pRootPersistentComponent)
        {
            // no redirect support by default
            bNoRedirectSupport = TRUE;

            // except the live source within a RAM
            m_pPersistentComponentManager->GetPersistentComponent(pMainSourceInfo->m_ulPersistentComponentID,
                                                                 (IHXPersistentComponent*&)pPersistentComponent);

            // allow redirect on the root persistent component(i.e. SMIL/RAM)
            if (pPersistentComponent == pRootPersistentComponent)
            {
                bNoRedirectSupport = FALSE;
            }
            else
            {
                while (pPersistentComponent != pRootPersistentComponent)
                {
                    if (PersistentSMIL == pPersistentComponent->m_ulPersistentType)
                    {
                        bRAMOnly = FALSE;
                        break;
                    }
                    pPersistentComponent = pPersistentComponent->m_pPersistentParent;
                }

                if (pRootPersistentComponent->m_ulPersistentType == PersistentRAM   &&
                    bRAMOnly                                                        &&
                    pMainSourceInfo->m_pSource)
                {
                    bNoRedirectSupport = FALSE;
                }
            }
            HX_RELEASE(pPersistentComponent);
        }
#endif /* HELIX_FEATURE_NESTEDMETA */

        // no Redirect support for SMIL presentation for now
        // return HXR_SERVER_DISCONNECTED instead
        if (bNoRedirectSupport)
        {
            Report(HXLOG_ERR, HXR_SERVER_DISCONNECTED, HXR_OK, NULL, NULL);
            goto cleanup;
        }

        if (m_pRequest)
            m_pRequest->GetRequestHeaders(pRequestHeaders);

        // Set the redirecting property to true before calling stop so the TLC can tell if a redirect is going to occur
        if (pRequestHeaders)
            pRequestHeaders->SetPropertyULONG32("IsRedirecting", TRUE);

        // SourceInfo object will be destroyed when we stop player
        CHXString redirectURL = pMainSourceInfo->m_pSource->GetRedirectURL();

        StopPlayer(END_REDIRECT);
        CloseAllRenderers(m_nCurrentGroup);
        pMainSourceInfo = NULL; // now invalid

        // Set the redirecting property to false before opening the new URL.
        if (pRequestHeaders)
        {
            pRequestHeaders->SetPropertyULONG32("IsRedirecting", FALSE);
            HX_RELEASE(pRequestHeaders);
        }

        hr = OpenRedirect(redirectURL);

        if (hr == HXR_OK && bBegin)
        {
            Begin();
        }
    }

cleanup:

    m_bDoRedirect = FALSE;

    return hr;
}

HX_RESULT
HXPlayer::OpenRedirect(const char* pszURL)
{
    IHXCommonClassFactory* pCCF = NULL;

    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::OpenRedirect(): %s", this, pszURL);
    HX_RESULT   hr = HXR_OK;

    // we want to protect against the TLC opening another URL
    if (m_bSetModal)
    {
        return HXR_OK;
    }

    HX_RELEASE(m_pRequest);

    // get the compresses URL
    CHXURL url(pszURL, (IHXClientEngine*)m_pEngine);
    
    if (HXR_OK == m_pEngine->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF))
    {
    pCCF->CreateInstance(CLSID_IHXRequest, (void**)&m_pRequest);
    }
    HX_RELEASE(pCCF);

    if (m_pRequest)
    {
        m_pRequest->SetURL(url.GetURL());
        m_bActiveRequest = TRUE;

        hr = DoURLOpen(&url, NULL);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }

    return hr;
}

HX_RESULT
HXPlayer::StartDownload()
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::StartDownload()", this);
    HX_RESULT                   theErr = HXR_OK;
    CHXMapPtrToPtr::Iterator    ndxSource;

    if (!m_bIsDone)
    {
        CheckSourceRegistration();

        ndxSource = m_pSourceMap->Begin();

        /* Check if we are done. This may be TRUE for empty files */
        for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
        {
            SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSource);
            HXSource* pSource = pSourceInfo->m_pSource;
            
            if(pSource && pSource->CanBeResumed())
            {
                theErr = pSource->StartInitialization();
            }
        }
    }

    m_bFastStartInProgress  = TRUE;
    m_bFSBufferingEnd       = FALSE;

    return theErr;
}

HX_RESULT
HXPlayer::DoURLOpenFromSource(SourceInfo* pMainSourceInfo, URL_TYPE urlType)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::DoURLOpenFromSource()", this);
    HX_RESULT               rc = HXR_OK;
    HXBOOL                    bDefault = FALSE;
    SourceInfo*             pSourceInfo = NULL;
    CHXString               newURL;
    HXPersistentComponent*  pRootPersistentComponent = NULL;
    HXPersistentComponent*  pPersistentComponent = NULL;
    PersistentType          SourceType = PersistentUnknown;

    if (pMainSourceInfo && pMainSourceInfo->m_pSource)
    {
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
        if (pMainSourceInfo->m_pSource->IsPartOfNextGroup())
        {
            m_pNextGroupManager->RemoveSource(pMainSourceInfo);
            m_bPartOfNextGroup = TRUE;
        }
        else
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
#if defined(HELIX_FEATURE_PREFETCH)
        if (pMainSourceInfo->m_pSource->IsPartOfPrefetchGroup())
        {
            m_pPrefetchManager->RemoveSource(pMainSourceInfo);
        }
        else
#endif /* HELIX_FEATURE_PREFETCH */
        {
            m_pSourceMap->RemoveKey(pMainSourceInfo->m_pSource);
            m_bSourceMapUpdated = TRUE;
        }

        if (pMainSourceInfo->m_bTobeInitializedBeforeBegin &&
            m_uNumSourceToBeInitializedBeforeBegin > 0)
        {
            m_uNumSourceToBeInitializedBeforeBegin--;
        }

#if defined(HELIX_FEATURE_NESTEDMETA)
        pRootPersistentComponent = m_pPersistentComponentManager->m_pRootPersistentComponent;
        // only cleanup the layout if
        // * it's not a SMIL presentation OR
        // * the AltURL is on the root SMIL presentation itself
        // in other words
        // * don't cleanup the layout if the AltURL is on the tracks
        // * within the SMIL presentation        
        m_pPersistentComponentManager->GetPersistentComponent(pMainSourceInfo->m_ulPersistentComponentID,
                                                              (IHXPersistentComponent*&)pPersistentComponent);
        if( pPersistentComponent )
            SourceType = (PersistentType)pPersistentComponent->m_ulPersistentType;
        
        HX_RELEASE(pPersistentComponent);
        
        if( !pRootPersistentComponent  ||
            SourceType != PersistentSMIL ||
            pRootPersistentComponent->m_pSourceInfo == pMainSourceInfo)
        {
            CleanupLayout();
            if (pRootPersistentComponent)
            {
                pRootPersistentComponent->m_bCleanupLayoutCalled = TRUE;
            }
        }
#endif /* HELIX_FEATURE_NESTEDMETA */

        switch(urlType)
        {
        case URL_ALTERNATE:
            newURL = pMainSourceInfo->m_pSource->GetAltURL(bDefault);
            break;
        case URL_SDP:
            newURL = pMainSourceInfo->m_pSource->GetSDPURL();            
            break;
        case URL_REDIRECTED:
            newURL = pMainSourceInfo->m_pSource->GetRedirectURL();            
            break;
        case URL_OPPOSITE_HXSOURCE:
            {
                const char* pszOldURL = pMainSourceInfo->m_pSource->GetURL();
                if (pszOldURL)
                {
                    newURL = pszOldURL;
                }
            }
            break;
        default:
            HX_ASSERT(FALSE);
            break;
        }

        if (newURL.IsEmpty())
        {
            rc = HXR_UNEXPECTED;
            goto cleanup;
        }

        m_bIsDone   = FALSE;
        ResetActiveRequest();

        HX_DELETE(m_pURL);

        m_pURL = new CHXURL(newURL, (IHXClientEngine*)m_pEngine);
        if( m_pURL )
        {
            rc = m_pURL->GetLastError();
        }
        else
        {
            rc = HXR_OUTOFMEMORY;
        }

        if (rc)
        {
            HX_DELETE(m_pURL);
            goto cleanup;
        }

        pMainSourceInfo->Stop();
        pMainSourceInfo->CloseRenderers();

        pSourceInfo = NewSourceInfo();
        if (pSourceInfo)
        {
            pSourceInfo->m_uGroupID = pMainSourceInfo->m_uGroupID;
            pSourceInfo->m_uTrackID = pMainSourceInfo->m_uTrackID;
            pSourceInfo->m_bPrefetch = pMainSourceInfo->m_bPrefetch;
            pSourceInfo->m_id = pMainSourceInfo->m_id;

            pSourceInfo->m_bAltURL = (urlType == URL_ALTERNATE)?1:0;
            pSourceInfo->m_lastErrorFromMainURL = pMainSourceInfo->m_lastErrorFromMainURL;
            pSourceInfo->m_lastErrorStringFromMainURL = pMainSourceInfo->m_lastErrorStringFromMainURL;
            pSourceInfo->m_ulPersistentComponentID = pMainSourceInfo->m_ulPersistentComponentID;
            pSourceInfo->m_bNetSourceFailed = pMainSourceInfo->m_bNetSourceFailed;
            pSourceInfo->m_bFileSourceFailed = pMainSourceInfo->m_bFileSourceFailed;
            // Copy any remaining alternate URLs to the new source info
            pMainSourceInfo->CopyAltURLListToNewSourceInfo(pSourceInfo);
        }
        else
        {
            rc = HXR_OUTOFMEMORY;
            goto cleanup;
        }

        rc = AddURL(pSourceInfo, TRUE);
        if (HXR_OK == rc && pSourceInfo->m_bAltURL && pSourceInfo->m_pSource)
        {
            pSourceInfo->m_pSource->SetAltURLType(bDefault);
        }

        m_bPartOfNextGroup = FALSE;
        HX_DELETE(pMainSourceInfo);

        SchedulePlayer(PLAYER_SCHEDULE_DEFAULT | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);
    }
    else
    {
        HX_ASSERT(FALSE);
    }

cleanup:

    return rc;
}

HX_RESULT
HXPlayer::PauseDownload(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::PauseDownload()", this);
    HX_RESULT                   theErr = HXR_OK;
    CHXMapPtrToPtr::Iterator    ndxSource;

    if (!m_bFSBufferingEnd)
    {
        m_bFSBufferingEnd       = TRUE;
        m_ulFSBufferingEndTime  = HX_GET_TICKCOUNT();

        /* In network case, we want to buffer extra 2-3 seconds worth
         * of data. We renter this function over and over again in the
         * else clause and ultimately send 100% buffering message.
         * In local playback however, we want to send this message
         * right away. So do not return the function from here, instead fall
         * down below to return 100% buffering message instantly
         */
        ndxSource = m_pSourceMap->Begin();
        for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
        {
            SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSource);

            HXSource* pSource = pSourceInfo->m_pSource;

            if(pSource && pSource->IsNetworkAccess())
            {
                return HXR_OK;
            }
        }
    }
    else
    {
        /* Buffer extra 3 seconds worth of data before pausing */
        if (CALCULATE_ELAPSED_TICKS(m_ulFSBufferingEndTime, HX_GET_TICKCOUNT())
            < 3000)
        {
            return HXR_OK;
        }
    }

    m_bFastStartInProgress  = FALSE;

    ndxSource = m_pSourceMap->Begin();

    /* Check if we are done. This may be TRUE for empty files */
    for (; !theErr && !m_bIsDone && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSource);

        HXSource* pSource = pSourceInfo->m_pSource;

        /* Do not pause a live source */
        if(!pSource || pSource->IsLive())
        {
            continue;
        }

        theErr = pSource->StopInitialization();
    }

    if (m_b100BufferingToBeSent)
    {
        m_b100BufferingToBeSent = FALSE;
    SetClientState(HX_CLIENT_STATE_PREFETCHED);
        if (m_pAdviseSink)
        {
            m_pAdviseSink->OnBuffering(m_BufferingReason, 100);
        }
    }

    return theErr;
}

void
HXPlayer::SchedulePlayer(UINT32 ulFlags)
{
    UINT32 ulNextInterval = ((ulFlags & PLAYER_SCHEDULE_IMMEDIATE) ? 0 : m_ulPlayerUpdateInterval);

    if (((ulFlags & PLAYER_SCHEDULE_MAIN) || !m_bUseCoreThread) &&
    (!m_bIsDone || m_LastError || m_pEngine->AtInterruptTime()))
    {
    if (ulFlags & PLAYER_SCHEDULE_RESET)
    {
        HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SchedulePlayer() Removing System Callback", this);
        m_pHXPlayerCallback->Cancel(m_pScheduler);
    }

    if (!m_pHXPlayerCallback->GetPendingCallback())
    {
        HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SchedulePlayer() Scheduling System Callback in %lu", 
            this,
            ulNextInterval);

        m_pHXPlayerCallback->CallbackScheduled(
        m_pScheduler->RelativeEnter(m_pHXPlayerCallback, 
                        ulNextInterval));
    }
    }

    //only schedule on the interrupt-safe and interrupt-only chains if we are
    //using core thread
    if (m_bUseCoreThread)
    {
    if ((ulFlags & PLAYER_SCHEDULE_INTERRUPT_SAFE) &&
    (!m_bIsDone || m_LastError))
    {
    if (ulFlags & PLAYER_SCHEDULE_RESET)
    {
        HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SchedulePlayer() Removing Interrupt Callback", this);
        m_pHXPlayerInterruptCallback->Cancel(m_pScheduler);
    }

    if (!m_pHXPlayerInterruptCallback->GetPendingCallback())
    {
        HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SchedulePlayer() Scheduling Interupt Callback in %lu", 
            this,
            ulNextInterval);

        m_pHXPlayerInterruptCallback->CallbackScheduled(
        m_pScheduler->RelativeEnter(m_pHXPlayerInterruptCallback, 
                        ulNextInterval));
    }
    }

    if ((ulFlags & PLAYER_SCHEDULE_INTERRUPT_ONLY) &&
    (!m_bIsDone || m_LastError))
    {
    if (ulFlags & PLAYER_SCHEDULE_RESET)
    {
        HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SchedulePlayer() Removing Interrupt Only Callback", this);
        m_pHXPlayerInterruptOnlyCallback->Cancel(m_pScheduler);
    }

    if (!m_pHXPlayerInterruptOnlyCallback->GetPendingCallback())
    {
        HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SchedulePlayer() Scheduling Interupt Only Callback in %lu", 
            this,
            ulNextInterval);

        m_pHXPlayerInterruptOnlyCallback->CallbackScheduled(
        m_pScheduler->RelativeEnter(m_pHXPlayerInterruptOnlyCallback, 
                        ulNextInterval));
    }
    }
    }
}

HX_RESULT
HXPlayer::CheckForAudioResume(UINT32 &ulSchedulerFlags)
{
    HX_RESULT theErr = HXR_OK;

    if (m_bInitialized && !m_bSetupToBeDone && !m_bPaused &&
        (m_bBeginPending || m_bTimelineToBeResumed) &&
        m_uNumSourceToBeInitializedBeforeBegin == 0)
    {
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CheckForAudioResume(): Starting to resume...", this); 

#if defined(HELIX_CONFIG_AUDIO_ON_CORE_THREAD)
    if (m_bResumeOnlyAtSystemTime && m_pEngine->AtInterruptTime())
    {
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CheckForAudioResume(): Wrong Thread: Bailing out:", this);
        ulSchedulerFlags = (PLAYER_SCHEDULE_MAIN | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);
        return HXR_RETRY;
    }
#endif
        if (m_b100BufferingToBeSent)
        {
            m_b100BufferingToBeSent = FALSE;
        SetClientState(HX_CLIENT_STATE_PREFETCHED);
            if (m_pAdviseSink)
            {
                m_pAdviseSink->OnBuffering(m_BufferingReason, 100);
            }
        }

        /*
         * change the state of buffering since we have crossed
         * initial startup state...
         * BUFFERING_SEEK and BUFFERING_LIVE_PAUSE are handled in OnTimeSync
         */
        if (m_BufferingReason == BUFFERING_START_UP)
        {
            m_BufferingReason = BUFFERING_CONGESTION;
        }

        /* did someone pause/stop the player within 100% OnBuffering call?
         * if so, do not resume the timeline yet.
         */
        if (m_bPaused || !m_bInitialized)
        {
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CheckForAudioResume(): Interrupted: Bailing out", this);
            return HXR_OK;
        }

        m_bIsPlaying            = TRUE;
        m_bBeginPending         = FALSE;
        m_bTimelineToBeResumed  = FALSE;
        m_bPendingAudioPause    = FALSE;
    
        HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::CheckForAudioResume(): resuming audio player", this);            
    
    SetClientState(HX_CLIENT_STATE_PLAYING);

#if !defined(HELIX_FEATURE_LOGLEVEL_NONE)
        // The following logic is only valuable when logging is enabled
        // 
        // Source is notified the start of playback so that startup performance figures
        // can be collected and saved to the logs
        if (HXLoggingEnabled())
        {
            CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
            for (;  ndxSource != m_pSourceMap->End(); ++ndxSource)
            { 
                SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
                pSourceInfo->m_pSource->OnResumed();
            }
        }
#endif /* #if !defined(HELIX_FEATURE_LOGLEVEL_NONE) */

        theErr = m_pAudioPlayer->Resume();

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CheckForAudioResume(): Completed resume", this); 
    }

    return theErr;
}

void HXPlayer::SetMinimumPushdown()
{
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (;  ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);

        HXSource* pSource = pSourceInfo->m_pSource;

        pSource->SetMinimumPreroll();
    }
}

void
HXPlayer::ResetError(void)
{
    m_LastSeverity      = HXLOG_ERR;
    m_LastError         = HXR_OK;
    m_ulLastUserCode    = HXR_OK;

    HX_VECTOR_DELETE(m_pLastUserString);
    HX_VECTOR_DELETE(m_pLastMoreInfoURL);
}

void
HXPlayer::SetLastError(HX_RESULT theErr)
{
    if (theErr && !m_LastError)
    {
        m_LastError = theErr;
        m_bIsDone   = TRUE;
        m_pAudioPlayer->DonePlayback();
    }

    // Handle OOM errors as special case: we do not want to continue playback
    // any longer.
    // Add SLOW_MACHINE to the "Abort Now" category.
    if( theErr == HXR_OUTOFMEMORY || theErr == HXR_SLOW_MACHINE )
    {
        // ActualReport does not get called next time in ProcessIdle because
        // we Abort playback here, so have to call it manually.
        ActualReport(m_LastSeverity, m_LastError,
                     m_ulLastUserCode, m_pLastUserString, m_pLastMoreInfoURL);
        AbortPlayer();
    }
}

HX_RESULT
HXPlayer::HandleRedirectRequest(SourceInfo* pSourceInfo)
{
    HX_RESULT       theErr = HXR_OK;

    if (!m_pRedirectList)
    {
        m_pRedirectList = new CHXSimpleList();

        if (!m_pRedirectList)
        {
            theErr = HXR_OUTOFMEMORY;
            goto cleanup;
        }
    }

    m_pRedirectList->AddTail(pSourceInfo);

cleanup:

    return theErr;
}

HX_RESULT
HXPlayer::HandleSDPRequest(SourceInfo* pSourceInfo)
{
    HX_RESULT       theErr = HXR_OK;

    if (!m_pSDPURLList)
    {
        m_pSDPURLList = new CHXSimpleList();

        if (!m_pSDPURLList)
        {
            theErr = HXR_OUTOFMEMORY;
            goto cleanup;
        }
    }

    m_pSDPURLList->AddTail(pSourceInfo);

cleanup:

    return theErr;
}

HXBOOL
HXPlayer::IsAtSourceMap(SourceInfo* pSourceInfo)
{
    SourceInfo* pResult = NULL;

    return m_pSourceMap->Lookup(pSourceInfo->m_pSource, (void*&)pResult);
}

/*
* IHXErrorMessages methods
*/

STDMETHODIMP
HXPlayer::Report
(
    const UINT8 unSeverity,
    HX_RESULT   ulHXCode,
    const ULONG32       ulUserCode,
    const char* pUserString,
    const char* pMoreInfoURL
)
{
    /* Always pass through info and debug messages.
     * We use these severity levels for logging purposes and
     * do not consider them as errors
     */
    if (unSeverity == HXLOG_INFO    ||
        unSeverity == HXLOG_DEBUG   ||
        ulHXCode  == HXR_OK)
    {
        return ActualReport(unSeverity, ulHXCode, ulUserCode,
                            pUserString, pMoreInfoURL);
    }

    /* Do not override an error if it is already set.
     */
    if (m_LastError != HXR_OK)
    {
        m_bIsDone = TRUE;
        return HXR_OK;
    }

    m_LastSeverity      = unSeverity;
    m_ulLastUserCode    = ulUserCode;
    SetLastError(ulHXCode);

    if (pUserString != m_pLastUserString)
    {
        HX_VECTOR_DELETE(m_pLastUserString);

        if (pUserString && *pUserString)
        {
            m_pLastUserString = new char[strlen(pUserString) + 1];
            ::strcpy(m_pLastUserString, pUserString); /* Flawfinder: ignore */
        }
    }


    if (pMoreInfoURL != m_pLastMoreInfoURL)
    {
        HX_VECTOR_DELETE(m_pLastMoreInfoURL);

        if (pMoreInfoURL && *pMoreInfoURL)
        {
            m_pLastMoreInfoURL = new char[strlen(pMoreInfoURL) + 1];
            ::strcpy(m_pLastMoreInfoURL, pMoreInfoURL); /* Flawfinder: ignore */
        }
    }

    m_bIsDone                   = TRUE;
    m_bIsPresentationClosedToBeSent = TRUE;
    m_pAudioPlayer->DonePlayback();
    return HXR_OK;
}

HX_RESULT
HXPlayer::ActualReport
(
    const UINT8 unSeverity,
    HX_RESULT   ulHXCode,
    const ULONG32       ulUserCode,
    const char* pUserString,
    const char* pMoreInfoURL
)
{
#if defined(HELIX_FEATURE_SINKCONTROL)
    if (m_pErrorSinkControl)
    {
        m_pErrorSinkControl->ErrorOccurred(unSeverity, ulHXCode,
                                           ulUserCode, pUserString,
                                           pMoreInfoURL);
    }
#endif /* HELIX_FEATURE_SINKCONTROL */

    return HXR_OK;
}

void HXPlayer::AddRemoveErrorSinkTranslator(HXBOOL bAdd)
{
#if defined(HELIX_FEATURE_SINKCONTROL) && defined(HELIX_FEATURE_LOGGING_TRANSLATOR)
    if (m_pErrorSinkControl && m_pErrorSinkTranslator)
    {
        // Get the error sink control's IHXErrorSinkControl interface
        IHXErrorSinkControl* pControl = NULL;
        m_pErrorSinkControl->QueryInterface(IID_IHXErrorSinkControl, (void**) &pControl);
        if (pControl)
        {
            // Get the translator's IHXErrorSink interface
            IHXErrorSink* pSink = NULL;
            m_pErrorSinkTranslator->QueryInterface(IID_IHXErrorSink, (void**) &pSink);
            if (pSink)
            {
                // Are we adding or removing?
                if (bAdd)
                {
                    // Add the translator as an error sink, only including
                    // the HXLOG_DEBUG severity levels
                    m_pErrorSinkControl->AddErrorSink(pSink, HXLOG_DEBUG, HXLOG_DEBUG);
                }
                else
                {
                    // Remove the translator as an error sink
                    m_pErrorSinkControl->RemoveErrorSink(pSink);
                }
            }
            HX_RELEASE(pSink);
        }
        HX_RELEASE(pControl);
    }
#endif /* #if defined(HELIX_FEATURE_SINKCONTROL) */
}


STDMETHODIMP_ (IHXBuffer*)
HXPlayer::GetErrorText(HX_RESULT ulHXCode)
{
#if defined(HELIX_FEATURE_RESOURCEMGR)
    return m_pEngine->GetResMgr()->GetErrorString(ulHXCode);
#else
    return NULL;
#endif /* HELIX_FEATURE_RESOURCEMGR */
}


/************************************************************************
 *  Method:
 *      IHXAudioPlayerResponse::OnTimeSync
 *  Purpose:
 *      Notification interface provided by users of the IHXAudioPlayer
 *      interface. This method is called by the IHXAudioPlayer when
 *      audio playback occurs.
 */
STDMETHODIMP HXPlayer::OnTimeSync(ULONG32 /*IN*/ ulCurrentTime)
{
    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::OnTimeSync(%lu)", this, ulCurrentTime);
    HX_RESULT theErr = HXR_OK;
    CHXMapPtrToPtr::Iterator ndxSource;

    m_bCurrentPresentationClosed = FALSE;

    if (!m_bInitialized)
        return HXR_NOT_INITIALIZED;

    m_bTimeSyncLocked = TRUE;

    UpdateCurrentPlayTime( ulCurrentTime );

    if (m_bIsFirstTimeSync)
    {
        m_bIsFirstTimeSync  = FALSE;
        m_ulFirstTimeSync   = m_ulCurrentPlayTime;
    }

    m_pCoreMutex->Lock();

    /* ProcessIdle can result in triggering rebuffering (delayed due to accelerated delivery).
     * This will call InternalPause() and will pause the timeline. No further timesyncs 
     * should be sent to the source/renderers
     */
    if (m_bCurrentPresentationClosed || !m_bIsPlaying)
    {
        goto exit;
    }

#if defined(HELIX_FEATURE_NESTEDMETA)
    m_pPersistentComponentManager->OnTimeSync(m_ulCurrentPlayTime);
#endif /* HELIX_FEATURE_NESTEDMETA */

    ndxSource = m_pSourceMap->Begin();

    for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo *)(*ndxSource);
        pSourceInfo->OnTimeSync(m_ulCurrentPlayTime);

        if (pSourceInfo->m_pPeerSourceInfo &&
            pSourceInfo->m_pPeerSourceInfo->IsInitialized())
        {
            pSourceInfo->m_pPeerSourceInfo->OnTimeSync(m_ulCurrentPlayTime);
        }

        if (m_bCurrentPresentationClosed)
        {
            goto exit;
        }
    }

    if (m_BufferingReason == BUFFERING_SEEK || m_BufferingReason == BUFFERING_LIVE_PAUSE)
    {
        m_BufferingReason   = BUFFERING_CONGESTION;
        if (m_pAdviseSink)
        {
            m_pAdviseSink->OnPostSeek(m_ulTimeBeforeSeek, m_ulTimeAfterSeek);
        }
    }

    // Is this the first time sync after a SetVelocity?
    if (m_bSetVelocityInProgress)
    {
        // Clear the flag
        m_bSetVelocityInProgress = FALSE;
        // Re-enable the OnPreSeek, OnPostSeek, OnBuffering,
        // and OnPosLen calls in the advise sink. We don't have
        // to re-enable OnPause and OnBegin since we disabled/enabled
        // them during the SetVelocity call
        m_pAdviseSink->EnableAdviseSink(HX_ADVISE_SINK_FLAG_ONPOSLENGTH |
                                        HX_ADVISE_SINK_FLAG_ONPRESEEK   |
                                        HX_ADVISE_SINK_FLAG_ONPOSTSEEK  |
                                        HX_ADVISE_SINK_FLAG_ONBUFFERING);
        m_pClientStateAdviseSink->EnableClientStateAdviseSink(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
    }

    if (m_pAdviseSink)
    {
        m_pAdviseSink->OnPosLength( (m_bIsLive || m_bHasSubordinateLifetime ||
                                    m_ulCurrentPlayTime <= m_ulPresentationDuration) ?
                                    m_ulCurrentPlayTime : m_ulPresentationDuration,
                                    m_ulPresentationDuration);
    }

exit:
    m_bTimeSyncLocked = FALSE;

    m_pCoreMutex->Unlock();
    return theErr;
}

HX_RESULT   HXPlayer::DoURLOpen(CHXURL* pCHXURL, char* pMimeType)
{
    HX_RESULT       theErr = HXR_OK;

    if (!pCHXURL || !pCHXURL->GetURL() || !*(pCHXURL->GetURL()))
    {
        return HXR_INVALID_PARAMETER;
    }

    ResetError();

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

    /* Validate the URL */
    theErr = pCHXURL->GetLastError();

    if (theErr)
    {
        goto exit;
    }

    /* If we are not in a STOP state, do an internal stop */
    if (!m_bIsPresentationDone)
    {
//      m_bIsPresentationClosedToBeSent = FALSE;
        StopPlayer(END_STOP);
    }

    if (m_pMetaInfo)
    {
        m_pMetaInfo->ResetMetaInfo();
    }

    m_bIsDone   = FALSE;
    m_bIsPresentationDone = FALSE;
    m_bIsPresentationClosedToBeSent = TRUE;

#if defined(HELIX_FEATURE_AUTOUPGRADE)
    HX_RELEASE (m_pUpgradeCollection);
    CreateInstanceCCF(CLSID_IHXUpgradeCollection, (void**)&m_pUpgradeCollection, (IUnknown*)(IHXClientEngine*)m_pEngine);  
#endif /* HELIX_FEATURE_AUTOUPGRADE */

#if defined(HELIX_FEATURE_RECORDCONTROL)
    m_bRecordServiceEnabled = IsRecordServiceEnabled();
#endif

    // all presentation started with one track in a group
    theErr = SetSingleURLPresentation(pCHXURL);

    if (m_LastError && !theErr)
    {
        theErr = m_LastError;
    }

    if (theErr)
    {
        m_bIsPresentationClosedToBeSent = FALSE;
        ResetPlayer();
#if defined(HELIX_FEATURE_VIDEO)
        /*
         * Let the site supplier know that we are done changing the layout.
         */
        if (m_pSiteSupplier && !m_bBeginChangeLayoutTobeCalled)
        {
            m_bBeginChangeLayoutTobeCalled      = TRUE;
            m_pSiteSupplier->DoneChangeLayout();
        }
#endif /* HELIX_FEATURE_VIDEO */
    }

    if (!theErr)
    {
        SchedulePlayer(PLAYER_SCHEDULE_DEFAULT | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);
    }

exit:

    if (theErr)
    {
        /* If no one has set the last error and there is no user string,
         * copy URL to the user string
         */
        const char* pszURL = (pCHXURL ? pCHXURL->GetURL() : 0);
        if (!m_LastError && !m_pLastUserString && pszURL)
        {
            m_pLastUserString = new char[strlen(pszURL) + 1];
            strcpy(m_pLastUserString, pszURL); /* Flawfinder: ignore */
        }

        SetLastError(theErr);
    }

    if (m_LastError)
    {
        /* Schedule a ProcessIdle callback. That is where the error
         * would be reported/ auto-upgrade will be requested.
         */
        m_bIsDone = FALSE;
        SchedulePlayer(PLAYER_SCHEDULE_DEFAULT | PLAYER_SCHEDULE_IMMEDIATE);
        m_bIsDone = TRUE;
        theErr = HXR_OK;
    }
    else
    {
        if (!m_bPlayStateNotified && m_pEngine)
        {
            m_bPlayStateNotified = TRUE;
            m_pEngine->NotifyPlayState(m_bPlayStateNotified);
        }
    }

    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return (theErr);
}
/************************************************************************
 *  Method:
 *      IHXPlayer::OpenURL
 *  Purpose:
 *      Tell the player to open and URL.
 */
STDMETHODIMP HXPlayer::OpenURL(const char* pURL)
{
    IHXCommonClassFactory* pCCF = NULL;

    // we want to protect against the TLC opening another URL
    if (m_bSetModal)
    {
        return HXR_OK;
    }

    HX_RESULT   hr = HXR_OK;

    HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::OpenURL(): %s", this, pURL);
    SetClientState(HX_CLIENT_STATE_CONNECTING);

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    m_ulTimeOfOpenURL = HX_GET_TICKCOUNT();
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    // get the compresses URL
    CHXURL url(pURL, (IHXClientEngine*)m_pEngine);
    //pURL = url.GetURL();

    HX_RELEASE(m_pRequest);
    if (HXR_OK == m_pEngine->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF))
    {
    pCCF->CreateInstance(CLSID_IHXRequest, (void**)&m_pRequest);
    }
    HX_RELEASE(pCCF);

    if (m_pRequest)
    {
        m_pRequest->SetURL(url.GetURL());
        m_bActiveRequest = TRUE;

        if (m_pClientRequestSink)
        {
            m_pClientRequestSink->OnNewRequest(m_pRequest);
        }

        hr = DoURLOpen(&url, NULL);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }

    return hr;
}

/************************************************************************
 *      Method:
 *          IID_IHXPlayer2::OpenRequest
 *      Purpose:
 *          Call this method to open the IHXRequest
 */
STDMETHODIMP HXPlayer::OpenRequest(IHXRequest* pRequest)
{    
    // we want to protect against the TLC opening another URL
    if (m_bSetModal)
    {
        return HXR_OK;
    }

    HX_RESULT   hr = HXR_OK;
    const char* pURL = NULL;

    if (!pRequest)
    {
        return HXR_UNEXPECTED;
    }

    HX_RELEASE(m_pRequest);

    m_pRequest = pRequest;
    m_pRequest->AddRef();

    m_bActiveRequest = TRUE;

    SetClientState(HX_CLIENT_STATE_CONNECTING);

    // retrieve the URL
    if (HXR_OK != m_pRequest->GetURL(pURL))
    {
        return HXR_UNEXPECTED;
    }

    if (m_pClientRequestSink && m_pRequest)
    {
        m_pClientRequestSink->OnNewRequest(m_pRequest);
    }

    HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::OpenRequest(): %s", this, pURL);

    // get the compresses URL
    CHXURL url(pURL, (IHXClientEngine*)m_pEngine);
    pURL = url.GetURL();
    m_pRequest->SetURL(pURL);

    hr = DoURLOpen(&url, NULL);

    return hr;
}

/************************************************************************
 *     Method:
 *         IID_IHXPlayer2::GetRequest
 *     Purpose:
 *         Call this method to get the IHXRequest
 */
STDMETHODIMP HXPlayer::GetRequest(REF(IHXRequest*) pRequest)
{
    HX_RESULT  hr = HXR_OK;

    pRequest = NULL;

    if (!m_pRequest)
    {
        hr = HXR_UNEXPECTED;
        goto cleanup;
    }

    pRequest = m_pRequest;
    pRequest->AddRef();

cleanup:

    return hr;
}

////////////////////////////
//
HX_RESULT
HXPlayer::SetSingleURLPresentation(const CHXURL* pURL)
{
    HX_RESULT   theErr = HXR_OK;
    IHXGroup*   pGroup = NULL;
    IHXValues*  pProperties = NULL;
    IHXBuffer*  pValue = NULL;

    m_nGroupCount = 0;

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    // create group
    m_pGroupManager->CreateGroup(pGroup);

    // create track
    CreateValuesCCF(pProperties, (IUnknown*)(IHXClientEngine*)m_pEngine);
    
    char* url = (char*)pURL->GetURL();
    if (HXR_OK == CreateAndSetBufferCCF(pValue, (BYTE*)url, strlen(url)+1, (IUnknown*)(IHXPlayer*)this))
    {
    pProperties->SetPropertyCString("url", pValue);
    HX_RELEASE(pValue);
    }

    m_pGroupManager->AddGroup(pGroup);
    pGroup->AddTrack(pProperties);
   
    HX_RELEASE(pProperties);
    HX_RELEASE(pGroup);

    m_pGroupManager->SetCurrentGroup(0);
#else
    SourceInfo* pSourceInfo = NULL;
    HX_DELETE(m_pURL);
    CloseAllRenderers(0);

    m_pURL = new CHXURL(*pURL);
    if( m_pURL )
    {
        theErr = m_pURL->GetLastError();
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
    }

    if (HXR_OK == theErr)
    {
        pSourceInfo = NewSourceInfo();
        if (pSourceInfo)
        {
            theErr = AddURL(pSourceInfo, TRUE);
        }
        else
        {
            theErr = HXR_OUTOFMEMORY;
        }
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */

    if (HXR_OK == theErr)
    {
        // At this point, we setup the number of active streams to be the
        // number of sctreams described in the SourceList!
        UpdateSourceActive();

        if (m_bIsFirstBeginPending)
        {
            m_bIsFirstBeginPending = FALSE;
            Begin();
        }
    }
    else
    {
        HX_DELETE(m_pURL);
    }

    return theErr;
};

/************************************************************************
 *  Method:
 *      IHXPlayer::AddURL
 *  Purpose:
 *      Tell the player to add an URL to the current source list.
 */
HX_RESULT HXPlayer::AddURL(SourceInfo*& /*OUT*/ pSourceInfo, HXBOOL bAltURL)
{
    HX_RESULT   theErr = HXR_OK;
    HXSource*   pSource = NULL;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::AddURL(pSourceInfo=%p) Start", 
        this, 
        pSourceInfo);

    theErr = CreateSourceInfo(pSourceInfo, bAltURL);

    if (theErr != HXR_OK)
    {
        goto cleanup;
    }

    pSource = pSourceInfo->m_pSource;

    /* add to the list of sources for this player... */
    if (pSource)
    {
        if (!m_bPartOfNextGroup)
        {
#if defined(HELIX_FEATURE_PREFETCH)
            if (pSourceInfo->m_bPrefetch)
            {
                if (!m_pPrefetchManager)
                {
                    m_pPrefetchManager = new PrefetchManager(this);
                }

                m_pPrefetchManager->AddSource(pSourceInfo);
                pSourceInfo->m_pSource->PartOfPrefetchGroup(TRUE);
            }
            else
#endif /* HELIX_FEATURE_PREFETCH */
            {
                m_pSourceMap->SetAt((void*) pSourceInfo->m_pSource,
                                    (void*) pSourceInfo);

                if (pSource->GetDelay() >= m_ulCurrentPlayTime &&
                    pSource->GetDelay() - m_ulCurrentPlayTime <= MIN_DELAYBEFORE_START)
                {
                    pSourceInfo->m_bTobeInitializedBeforeBegin = TRUE;
                    m_uNumSourceToBeInitializedBeforeBegin++;
                }

                m_bPlayerWithoutSources = FALSE;
                m_bSourceMapUpdated = TRUE;
                m_bForceStatsUpdate = TRUE;
            }
        }
        else
        {
            if (pSource->GetDelay() <= MIN_DELAYBEFORE_START)
            {
                pSourceInfo->m_bTobeInitializedBeforeBegin = TRUE;
                /* We will increment the
                 * m_uNumSourceToBeInitializedBeforeBegin value
                 * once we start this next group
                 */
                //m_uNumSourceToBeInitializedBeforeBegin++;
            }
        }
    }

    /* Are we adding more sources mid presentation ? */
    if (!m_bPartOfNextGroup && !pSourceInfo->m_bPrefetch)
    {
        m_uNumSourcesActive++;
        m_uNumCurrentSourceNotDone++;
    }

cleanup:

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::AddURL(pSourceInfo=%p) End", 
        this, 
        pSourceInfo);

    return (theErr);
}

HX_RESULT
HXPlayer::CreateSourceInfo(SourceInfo*& pSourceInfo, HXBOOL bAltURL)
{
    HX_RESULT   theErr = HXR_OK;
    HXSource*   pSource = NULL;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CreateSourceInfo(pSourceInfo=%p, bAltURL=%c) Start", 
        this, 
        pSourceInfo,
        bAltURL ? 'T' :'F');

    IHXPluginSearchEnumerator* pEnum = NULL;
    HXBOOL bFileFormatClaim = CanFileFormatClaimSchemeExtensionPair(m_pURL, 
                                                                    pEnum,
                                                                    m_pURL->IsNetworkProtocol());

    if (m_pURL->IsNetworkProtocol() && !bFileFormatClaim && !pSourceInfo->m_bNetSourceFailed)
    {
        theErr = InitializeNetworkDrivers();

        if (!theErr)
        {
            theErr = DoNetworkOpen(pSourceInfo, bAltURL);
        }
    }
    else
    {
        // If this is an RTSP URL but we don't have any plugins
        // that claimed both the scheme and extension, then we
        // should query the plugin handler again to find any
        // plugins which can handle just the scheme.
        if (m_pURL->IsNetworkProtocol() && !bFileFormatClaim)
        {
            HX_RELEASE(pEnum);
            CanFileFormatClaimSchemeExtensionPair(m_pURL, pEnum, FALSE);
        }
        // Only send in URL now.
        theErr = DoFileSystemOpen(pSourceInfo, bAltURL, pEnum);
    }
    HX_RELEASE(pEnum);

    pSource = pSourceInfo->m_pSource;
    if (!theErr && pSource)
    {
#ifdef HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION
    IHXValues* pURLOptions = m_pURL->GetOptions();
    if (pURLOptions)
    {
        ULONG32 ulOffset;
        IHXBuffer* pBuffer = NULL;

        // Positive numbers are created as ULONG32 properties, whereas
        // negative numbers are created as buffers.  So we have to check both.
        if (pURLOptions->GetPropertyULONG32("soundLevelOffset", ulOffset) == HXR_OK)
        pSource->SetSoundLevelOffset ((INT16)ulOffset);
        else if (pURLOptions->GetPropertyBuffer("soundLevelOffset", pBuffer) == HXR_OK)
        {
        pSource->SetSoundLevelOffset ((INT16)atoi((const char *)pBuffer->GetBuffer()));
        HX_RELEASE(pBuffer);
        }

        HX_RELEASE(pURLOptions);
    }
#endif

        pSourceInfo->m_bInitialized = FALSE;

        if (HXR_OK != pSource->QueryInterface(IID_IHXPendingStatus,
                                            (void**)&(pSourceInfo->m_pStatus)))
        {
            pSourceInfo->m_pStatus = NULL;
        }
    }

    if (HXR_OK != theErr)
    {
        HX_DELETE(pSourceInfo);
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CreateSourceInfo(pSourceInfo=%p, bAltURL=%c) End", 
        this, 
        pSourceInfo,
        bAltURL ? 'T' :'F');

    return (theErr);
}

HX_RESULT
HXPlayer::PrepareSourceInfo(IHXValues* pTrack, SourceInfo*& pSourceInfo)
{
#if defined(HELIX_FEATURE_PREFETCH)
    char            szPrefetchType[] = "PrefetchType";
#endif

    HX_RESULT       rc = HXR_OK;
    char            szMaxDuration[] = "maxduration";
    char            szIndefiniteDuration[] = "indefiniteduration";
    char            szSoundLevel[] = "soundLevel";
    char            szAudioDeviceReflushHint[] = "audioDeviceReflushHint";
    char            szPersistentComponentID[] = "PersistentComponentID";
    char            szFill[] = "fill";
    char            szSeekOnLateBegin[] = "SeekOnLateBegin";
    char            szSeekOnLateBeginTolerance[] = "SeekOnLateBeginTolerance";
    UINT32          ulValue = 0;
#ifdef SEQ_DEPENDENCY
    char            szTrack_hint[] = "track-hint";
#endif

    pSourceInfo->m_bAudioDeviceReflushHint = FALSE;
    pSourceInfo->m_uSoundLevel = 100;
    pSourceInfo->m_bIndefiniteDuration = FALSE;
    pSourceInfo->m_ulMaxDuration = 0;
    pSourceInfo->m_ulPersistentComponentID = MAX_UINT32;
    pSourceInfo->m_ulPersistentComponentSelfID = MAX_UINT32;

#if defined(HELIX_FEATURE_ALT_URL)
    // Is there a "NumAltURL" ULONG32 property in the track properties?
    UINT32 ulNumAltURLs = 0;
    if (SUCCEEDED(pTrack->GetPropertyULONG32("NumAltURLs", ulNumAltURLs)) && ulNumAltURLs)
    {
        UINT32 i = 0;
        for (i = 0; i < ulNumAltURLs; i++)
        {
            // Construct the property name of the i-th AltURL
            CHXString strAltURLPropName("AltURL");
            strAltURLPropName.AppendULONG(i);
            // Get the i-th AltURL
            IHXBuffer* pAltURL = NULL;
            if (SUCCEEDED(pTrack->GetPropertyCString((const char*) strAltURLPropName, pAltURL)))
            {
                // Add this AltURL to the SourceInfo's AltURL list
                pSourceInfo->AddToAltURLList(pAltURL, FALSE);
            }
            HX_RELEASE(pAltURL);
        }
    }
    else
    {
        // Determine if this URL can be claimed by a file format
        IHXPluginSearchEnumerator* pEnum = NULL;
        HXBOOL bFFClaim = CanFileFormatClaimSchemeExtensionPair(m_pURL, pEnum, m_pURL->IsNetworkProtocol());
        if (!bFFClaim)
        {
            // This URL cannot be claimed by a fileformat.
            // Get the alternate URL from the CHXURL object. If
            // the out parameter bDefaultURL returns FALSE, then
            // there was an "AltURL" URL option.
            HXBOOL bDefaultURL = FALSE;
            char* pszAltURL = m_pURL->GetAltURL(bDefaultURL);
            if (pszAltURL)
            {
                // Create an IHXBuffer to hold this Alt URL
                IHXBuffer* pAltURLBuffer = NULL;
                HX_RESULT rv = CreateStringBufferCCF(pAltURLBuffer, (const char*) pszAltURL, (IUnknown*) (IHXPlayer*) this);
                if (SUCCEEDED(rv))
                {
                    // Add this to the source info's alt url list
                    pSourceInfo->AddToAltURLList(pAltURLBuffer, bDefaultURL);
                }
                HX_RELEASE(pAltURLBuffer);
            }
            HX_VECTOR_DELETE(pszAltURL);
        }
        HX_RELEASE(pEnum);
    }
#endif /* #if defined(HELIX_FEATURE_ALT_URL) */

#if defined(HELIX_FEATURE_PREFETCH)
    char            szPrefetchValue[] = "PrefetchValue";
    // read prefetch info
    if (HXR_OK == pTrack->GetPropertyULONG32(szPrefetchType, ulValue) && ulValue)
    {
        pSourceInfo->m_bPrefetch = TRUE;
        pSourceInfo->m_prefetchType = (PrefetchType)ulValue;

        if (HXR_OK == pTrack->GetPropertyULONG32(szPrefetchValue, ulValue) && ulValue)
        {
            pSourceInfo->m_ulPrefetchValue = ulValue;
        }
    }
#endif /* HELIX_FEATURE_PREFETCH */

    // read audioDeviceReflushHint
    if (HXR_OK == pTrack->GetPropertyULONG32(szAudioDeviceReflushHint, ulValue) && ulValue)
    {
        pSourceInfo->m_bAudioDeviceReflushHint = TRUE;
    }

    // read soundLevel
    if (HXR_OK == pTrack->GetPropertyULONG32(szSoundLevel, ulValue))
    {
        pSourceInfo->m_uSoundLevel = (UINT16)ulValue;
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szIndefiniteDuration, ulValue) && ulValue)
    {
        pSourceInfo->m_bIndefiniteDuration = TRUE;
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szMaxDuration, ulValue))
    {
        pSourceInfo->m_ulMaxDuration = ulValue;
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szPersistentComponentID, ulValue))
    {
        pSourceInfo->m_ulPersistentComponentID = ulValue;
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szFill, ulValue))
    {
        pSourceInfo->m_fillType = (FillType)ulValue;
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szSeekOnLateBegin, ulValue))
    {
        pSourceInfo->m_bSeekOnLateBegin = (0 != ulValue);

        if (HXR_OK == pTrack->GetPropertyULONG32(szSeekOnLateBeginTolerance, ulValue))
        {
            pSourceInfo->m_ulSeekOnLateBeginTolerance = ulValue;
        }
    }

#ifdef SEQ_DEPENDENCY
    IHXBuffer* pDependency = NULL;
    pTrack->GetPropertyCString(szTrack_hint,pDependency);

    if (pDependency && *(pDependency->GetBuffer()))
    {
        pSourceInfo->SetDependency(pDependency);
    }

    HX_RELEASE(pDependency);
#endif /*SEQ_DEPENDENCY*/

    return rc;
}

/************************************************************************
 *      Method:
 *          IHXPendingStatus::GetStatus
 *      Purpose:
 *          Called by the user to get the current pending status from an object
 */
STDMETHODIMP
HXPlayer::GetStatus
(
    REF(UINT16) uStatusCode,
    REF(IHXBuffer*) pStatusDesc,
    REF(UINT16) ulPercentDone
)
{
    HX_RESULT           hResult = HXR_OK;
    UINT16              statusCode = 0;
    UINT16              percentDone = 0;
    UINT16              totalPercentDone = 0;
    HXBOOL                bIsContacting = FALSE;
    HXBOOL                bIsBuffering = FALSE;
    HXBOOL                bIsReady = FALSE;
    HXBOOL                bInitializing = FALSE;
    IHXPendingStatus*   pStatus = NULL;

    // initialize(default)
    uStatusCode = HX_STATUS_READY;
    pStatusDesc = NULL;
    ulPercentDone = 0;

    // collect info from all the sources
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();

    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        pStatus = pSourceInfo->m_pStatus;

        if (pStatus && HXR_OK == pStatus->GetStatus(statusCode, pStatusDesc, percentDone))
        {
            if (HX_STATUS_CONTACTING == statusCode)
            {
                bIsContacting = TRUE;
                break;
            }
            else if (HX_STATUS_BUFFERING == statusCode)
            {
                bIsBuffering = TRUE;
                totalPercentDone += percentDone;
            }
            else if (HX_STATUS_READY == statusCode)
            {
                bIsReady = TRUE;
                totalPercentDone += 100;
            }
            else if (HX_STATUS_INITIALIZING == statusCode)
            {
                bInitializing = TRUE;
            }
        }
    }

    if (bInitializing)
    {
        uStatusCode = HX_STATUS_INITIALIZING;
        ulPercentDone = 0;
    }
    else if (bIsContacting)
    {
        uStatusCode = HX_STATUS_CONTACTING;
        ulPercentDone = 0;
    }
    else if (bIsBuffering)
    {
        uStatusCode = HX_STATUS_BUFFERING;
        pStatusDesc = NULL;
        ulPercentDone = totalPercentDone / m_pSourceMap->GetCount();
    }
    else if (bIsReady)
    {
        uStatusCode = HX_STATUS_READY;
        pStatusDesc = NULL;
        ulPercentDone = 0;
    }

    return hResult;
}

HX_RESULT
HXPlayer::DoOpenGroup(UINT16 nGroupNumber)
{
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    HX_RESULT   theErr = HXR_OK;
    IHXGroup*   pGroup = 0;

    HX_VERIFY((theErr = m_pGroupManager->GetGroup(nGroupNumber, pGroup)) == HXR_OK);
    if (theErr)
    {
        return theErr;
    }

    if (!m_bPartOfNextGroup)
    {
        m_bInitialized = FALSE; //so that we InitializeRenderers for this group

        m_bIsPresentationClosedToBeSent = FALSE;
        StopAllStreams(END_STOP);
        ResetGroup();
        CloseAllRenderers(m_nCurrentGroup); //kill all currently open renderers
        m_bIsDone   = FALSE;
    }

    // add all the tracks in the group to the player's source list
    UINT16          nTrackCount = pGroup->GetTrackCount();
    IHXValues*      pTrack = NULL;
    IHXPrefetch*    pPrefetch = NULL;
    HX_RESULT       theErrToReturn = HXR_OK;

    for (UINT16 nTrackIndex = 0; nTrackIndex < nTrackCount; nTrackIndex++)
    {
        if ((theErr = pGroup->GetTrack(nTrackIndex,pTrack)) == HXR_OK)
        {
            theErr = OpenTrack(pTrack, nGroupNumber, nTrackIndex);

            if (theErr && !theErrToReturn)
            {
                theErrToReturn = theErr;
            }

            HX_RELEASE(pTrack);
        }
    }

#if defined(HELIX_FEATURE_PREFETCH)
    if (HXR_OK == pGroup->QueryInterface(IID_IHXPrefetch, (void**)&pPrefetch))
    {
        nTrackCount = pPrefetch->GetPrefetchTrackCount();

        for (UINT16 nTrackIndex = 0; nTrackIndex < nTrackCount; nTrackIndex++)
        {
            if ((theErr = pPrefetch->GetPrefetchTrack(nTrackIndex,pTrack)) == HXR_OK &&
                pTrack)
            {
                theErr = OpenTrack(pTrack, nGroupNumber, nTrackIndex);

                if (theErr && !theErrToReturn)
                {
                    theErrToReturn = theErr;
                }

                HX_RELEASE(pTrack);
            }
        }
    }
#endif /* HELIX_FEATURE_PREFETCH */

    HX_RELEASE(pPrefetch);
    HX_RELEASE(pGroup);

    return theErrToReturn;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

HX_RESULT
HXPlayer::SpawnSourceIfNeeded(SourceInfo* pSourceInfo)
{
#if defined(HELIX_FEATURE_SMIL_REPEAT)
    HX_RESULT   theErr = HXR_OK;
    RepeatInfo* pRepeatInfo = NULL;

    // spawned the source if it is repeated
    if (pSourceInfo->m_pRepeatList                  &&
        !pSourceInfo->m_pPeerSourceInfo             &&
        pSourceInfo->m_pSource->IsInitialized())
    {
        SourceInfo* pPeerSourceInfo = NewSourceInfo();
        if( !pPeerSourceInfo )
        {
            return HXR_OUTOFMEMORY;
        }

        CHXURL*     pURL = m_pURL;
        const char* pszURL = pSourceInfo->m_pSource->GetURL();
        m_pURL = new CHXURL(pszURL, (IHXClientEngine*)m_pEngine);
        if( !m_pURL )
        {
            HX_DELETE(pSourceInfo);
            return HXR_OUTOFMEMORY;
        }

        pPeerSourceInfo->m_curPosition = pSourceInfo->m_curPosition;
        pRepeatInfo = (RepeatInfo*)pSourceInfo->m_pRepeatList->GetAtNext(pPeerSourceInfo->m_curPosition);

        if (pRepeatInfo->ulStart)
        {
            m_pURL->AddOption("Start", pRepeatInfo->ulStart);
        }

        if (pRepeatInfo->ulEnd)
        {
            m_pURL->AddOption("End", pRepeatInfo->ulEnd);
        }

        m_pURL->AddOption("Delay", pRepeatInfo->ulDelay);
        m_pURL->AddOption("Duration", pRepeatInfo->ulDuration);

        pPeerSourceInfo->m_bLeadingSource = FALSE;
        pPeerSourceInfo->m_bRepeatIndefinite = pSourceInfo->m_bRepeatIndefinite;
        pPeerSourceInfo->m_ulRepeatInterval = pSourceInfo->m_ulRepeatInterval;
        pPeerSourceInfo->m_ulMaxDuration = pSourceInfo->m_ulMaxDuration;
        pPeerSourceInfo->m_bTrackStartedToBeSent = pSourceInfo->m_bTrackStartedToBeSent;
        pPeerSourceInfo->m_uGroupID = pSourceInfo->m_uGroupID;
        pPeerSourceInfo->m_uTrackID = pRepeatInfo->uTrackID;
        pPeerSourceInfo->m_ulPersistentComponentID = pSourceInfo->m_ulPersistentComponentID;
        pPeerSourceInfo->m_ulTotalTrackDuration = pSourceInfo->m_ulTotalTrackDuration;

        theErr = CreateSourceInfo(pPeerSourceInfo, FALSE);

        if(pPeerSourceInfo && pPeerSourceInfo->m_pSource)
        {
            pPeerSourceInfo->m_pSource->m_ulOriginalDelay = pSourceInfo->m_pSource->m_ulOriginalDelay;

            pSourceInfo->m_pPeerSourceInfo = pPeerSourceInfo;
            pPeerSourceInfo->m_pPeerSourceInfo = pSourceInfo;
        }

        HX_DELETE(m_pURL);
        m_pURL = pURL;
    }

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_SMIL_REPEAT */
}

HX_RESULT
HXPlayer::SwitchSourceIfNeeded(void)
{
#if defined(HELIX_FEATURE_SMIL_REPEAT)
    // swapping the repeated sources if it's time
    HX_RESULT   theErr = HXR_OK;
    UINT32      ulTotalTrackDuration = 0;
    UINT32      ulPeerSourceDuration = 0;
    UINT32      ulPeerSourceDelay = 0;
    SourceInfo* pSourceInfo = NULL;
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();

    for (;!theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        pSourceInfo = (SourceInfo*)(*ndxSource);

        if (!pSourceInfo->m_pPeerSourceInfo ||
            !pSourceInfo->m_pPeerSourceInfo->m_pSource)
        {
            continue;
        }

        ulTotalTrackDuration = pSourceInfo->GetActiveDuration();
        ulPeerSourceDuration = pSourceInfo->m_pPeerSourceInfo->m_pSource->GetDuration();
        ulPeerSourceDelay = pSourceInfo->m_pPeerSourceInfo->m_pSource->GetDelay();

        if (ulTotalTrackDuration > m_ulCurrentPlayTime              &&
            ulPeerSourceDelay > pSourceInfo->m_pSource->GetDelay()  &&
            ulPeerSourceDelay <= m_ulCurrentPlayTime)
        {
            if (ulPeerSourceDuration > ulTotalTrackDuration)
            {
                pSourceInfo->m_pPeerSourceInfo->UpdateDuration(ulTotalTrackDuration - ulPeerSourceDelay);
            }

            m_pSourceMap->RemoveKey(pSourceInfo->m_pSource);

            if (!pSourceInfo->m_pSource->IsSourceDone())
            {
                pSourceInfo->m_pSource->SetEndOfClip(TRUE);
            }
            pSourceInfo->m_bDone = TRUE;

            pSourceInfo->m_bRepeatPending = TRUE;
            pSourceInfo->m_pPeerSourceInfo->m_bRepeatPending = FALSE;

            m_pSourceMap->SetAt((void*) pSourceInfo->m_pPeerSourceInfo->m_pSource,
                              (void*) pSourceInfo->m_pPeerSourceInfo);

            m_bSourceMapUpdated = TRUE;
            m_bForceStatsUpdate = TRUE;

            break;
        }
    }

    return theErr;
#else 
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_SMIL_REPEAT */
}

HX_RESULT
HXPlayer::OpenTrack(IHXValues* pTrack, UINT16 uGroupID, UINT16 uTrackID)
{
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    HX_RESULT           theErr = HXR_OK;
    IHXBuffer*          pBuffer = NULL;
    IHXBuffer*          pID = NULL;
    SourceInfo*         pSourceInfo = NULL;
    const char*         pURL = NULL;

    char                szID[] = "id";
    char                szUrl[] = "url";
    char                szSrc[] = "src";
    char                szStart[] = "Start";
    char                szEnd[] = "End";
    char                szDelay[] = "Delay";
    char                szDuration[] = "Duration";
    UINT32              ulValue = 0;

    theErr = pTrack->GetPropertyCString(szUrl,pBuffer);

    /*
     * Make sure everyone is setting url property (and not the
     * src property) for consistency.
     */

    /* temp - for now support both "src" & "url" */
    if (theErr)
    {
        theErr = pTrack->GetPropertyCString(szSrc,pBuffer);
    }

    if (theErr)
    {
        theErr = HXR_INVALID_PATH;
        goto cleanup;
    }

    pURL = (const char*)pBuffer->GetBuffer();

    if (!pURL || !*pURL)
    {
        theErr = HXR_INVALID_PATH;
        goto cleanup;
    }

    // Cleanup any url object!
    HX_DELETE(m_pURL);

    m_pURL = new CHXURL(pURL, (IHXClientEngine*)m_pEngine); //parse the url
    if (!m_pURL)
    {
        theErr = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    theErr = m_pURL->GetLastError();

    if (theErr)
    {
        goto cleanup;
    }

    theErr = OpenTrackExt();

    //temp - DoNetworkOpen/DoFSOpen extract these properties from m_pURL
    if (HXR_OK == pTrack->GetPropertyULONG32(szStart,ulValue))
    {
        m_pURL->AddOption(szStart, ulValue);
    }
    //pProperty = NULL;
    if (HXR_OK == pTrack->GetPropertyULONG32(szEnd,ulValue))
    {
        m_pURL->AddOption(szEnd, ulValue);
    }
    if (HXR_OK == pTrack->GetPropertyULONG32(szDelay,ulValue))
    {
        m_pURL->AddOption(szDelay, ulValue);
    }

    if (HXR_OK == pTrack->GetPropertyULONG32(szDuration,ulValue))
    {
        m_pURL->AddOption(szDuration, ulValue);
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::OpenTrack(): URL=%s GroupID=%hd TrackID=%hd", 
        this, 
        m_pURL ? m_pURL->GetURL() : "NULL",
        uGroupID, 
        uTrackID);

    pSourceInfo = NewSourceInfo();
    if(pSourceInfo)
    {
        pSourceInfo->m_uGroupID = uGroupID;
        pSourceInfo->m_uTrackID = uTrackID;

        if (HXR_OK == pTrack->GetPropertyCString(szID, pID))
        {
            pSourceInfo->m_id = (const char*)pID->GetBuffer();
        }

        PrepareSourceInfo(pTrack, pSourceInfo);
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    theErr = AddURL(pSourceInfo, FALSE);

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    if (HXR_OK == theErr    &&
        m_bPartOfNextGroup  &&
        pSourceInfo->m_pSource)
    {
        m_pNextGroupManager->AddSource(pSourceInfo);
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

    if (HXR_OK == theErr)
    {
    SchedulePlayer(PLAYER_SCHEDULE_DEFAULT | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);
    }

cleanup:

    HX_RELEASE(pBuffer);
    HX_RELEASE(pID);

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

HX_RESULT
HXPlayer::OpenTrackExt()
{
    return HXR_OK;
}

/* called from ProcessIdle when done playing current group */
void HXPlayer::PlayNextGroup()
{
    UINT16 uNextGroup = 0;

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    m_pGroupManager->GetNextGroup(uNextGroup);
#else
    uNextGroup = m_nGroupCount;
#endif /* HELIX_FEATURE_BASICGROUPMGR */

    m_nCurrentGroup = uNextGroup;

    if (m_nCurrentGroup >= m_nGroupCount)
    {
    if (m_bRestartToPrefetched)
    {
        HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::PlayNextGroup() Restarting, transitioning to PREFETCHED...", this);

        // Send callback messages
            m_pAdviseSink->OnPosLength( (m_bIsLive && m_ulPresentationDuration == 0) ?
                                    m_ulCurrentPlayTime : m_ulPresentationDuration,
                                    m_ulPresentationDuration);
            m_pAdviseSink->OnStop();

        // Pause and seek to zero
        m_bIsDone = FALSE;
        m_bInternalReset = TRUE; // Don't send 'OnPause' or set state to PAUSED
        PausePlayer();
        m_bInternalReset = FALSE;
        SeekPlayer(0);
    }
    else
    {
        // Stop completely...
        m_bIsPresentationClosedToBeSent = TRUE;
        m_bIsDone = TRUE;

        StopPlayer(END_DURATION);

#if defined(HELIX_FEATURE_VIDEO)
        /*
         * Let the site supplier know that we are done changing the layout.
         */
        if (m_pSiteSupplier && !m_bBeginChangeLayoutTobeCalled)
        {
            m_bBeginChangeLayoutTobeCalled      = TRUE;
            m_pSiteSupplier->DoneChangeLayout();
        }
#endif /* HELIX_FEATURE_VIDEO */
    }
    }
    else
    {
        // build the group's source list
        m_bIsPresentationClosedToBeSent = FALSE;

        StopAllStreams(END_DURATION);
        m_bIsPresentationClosedToBeSent = TRUE;

        ResetGroup();

#if defined(HELIX_FEATURE_BASICGROUPMGR)
        m_pGroupManager->SetCurrentGroup((UINT16) m_nCurrentGroup);
#endif /* HELIX_FEATURE_BASICGROUPMGR */
    }

    m_bForceStatsUpdate = TRUE;
}

/************************************************************************
 *      Method:
 *          HXPlayer::CheckTrackAndSourceOnTrackStarted
 *      Purpose:
 *           Passthrough to allow SourceInfo to call Master TAC manager
 *
 */
HXBOOL HXPlayer::CheckTrackAndSourceOnTrackStarted(INT32 nGroup,
                                                  INT32 nTrack,
                                                  UINT32 sourceID)
{
#if defined(HELIX_FEATURE_MASTERTAC)
    return (!m_pMasterTAC ?TRUE :m_pMasterTAC->CheckTrackAndSourceOnTrackStarted(nGroup, nTrack, sourceID));
#else
    return TRUE;
#endif /* HELIX_FEATURE_MASTERTAC */
}


/*
 *      IHXRegistryID methods
 */

/************************************************************************
 *      Method:
 *          IHXRegistryID::GetID
 *      Purpose:
 *          Get registry ID(hash_key) of the objects(player, source and stream)
 *
 */
STDMETHODIMP HXPlayer::GetID(REF(UINT32) /*OUT*/ ulRegistryID)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    (m_pStats)?(ulRegistryID = m_pStats->m_ulRegistryID):(ulRegistryID = 0);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

HX_RESULT HXPlayer::DoNetworkOpen(SourceInfo*& pSourceInfo, HXBOOL bAltURL)
{
#if defined(HELIX_FEATURE_PLAYBACK_NET)
    HX_RESULT   theErr = HXR_OK;
    HXSource*   pSource = NULL;
    IHXValues*  pURLProperties = NULL;
    IHXBuffer* pBuffer = NULL;
    UINT32      ulRegistryID = 0;
    char*       pszHost = NULL;
    char*       pszResource = NULL;
    const char* pszURL = NULL;
    ULONG32     ulPort = 0;
    IHXBuffer* pszParentName = NULL;

#if defined(HELIX_FEATURE_SMARTERNETWORK)
    if (!m_bPrefTransportInitialized && m_pPreferredTransportManager)
    {
        // re-load proxy/subnet preferences
        m_bPrefTransportInitialized = TRUE;

        if (m_pNetInterfaces)
        {
            m_pNetInterfaces->UpdateNetInterfaces();
        }
        m_pPreferredTransportManager->Initialize();
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    pSource = pSourceInfo->m_pSource = NewNetSource();
    if (!pSource)
    {
        return( HXR_OUTOFMEMORY );
    }
    pSource->AddRef();

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    // registry setting
    if (m_pRegistry && m_pStats)
    {
        char        szSourceName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

        if (m_bPartOfNextGroup &&
            HXR_OK == m_pRegistry->GetPropName(m_ulNextGroupRegistryID, pszParentName))
        {
            SafeSprintf(szSourceName, MAX_DISPLAY_NAME, "%s.Source%ld", pszParentName->GetBuffer(),
                    pSourceInfo->m_uTrackID);
        }
        else if (HXR_OK == m_pRegistry->GetPropName(m_pStats->m_ulRegistryID, pszParentName))
        {
            SafeSprintf(szSourceName, MAX_DISPLAY_NAME, "%s.Source%ld", pszParentName->GetBuffer(),
                    pSourceInfo->m_uTrackID);
        }
        else
        {
            HX_ASSERT(FALSE);
        }

        /* does this ID already exists ? */
        ulRegistryID = m_pRegistry->GetId(szSourceName);
        if (!ulRegistryID)
        {
            ulRegistryID = m_pRegistry->AddComp(szSourceName);
        }
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    HX_RELEASE(pszParentName);

    pSource->SetSourceInfo(pSourceInfo);
    pSource->Init(this, ulRegistryID);

    UINT32 ulStart = 0, ulEnd = HX_EOF_TIME, ulDelay = 0, ulDuration = 0;    
    GetTimingFromURL(m_pURL, ulStart, ulEnd, ulDelay, ulDuration);

    if (!(pURLProperties = m_pURL->GetProperties()))
    {
        theErr = HXR_FAILED;
        goto cleanup;
    }

    pURLProperties->GetPropertyULONG32(PROPERTY_PORT, ulPort);

    if (HXR_OK == pURLProperties->GetPropertyBuffer(PROPERTY_HOST, pBuffer))
    {
        pszHost = (char*)pBuffer->GetBuffer();
        pBuffer->Release();
    }

    if (HXR_OK == pURLProperties->GetPropertyBuffer(PROPERTY_RESOURCE, pBuffer))
    {
        pszResource = (char*)pBuffer->GetBuffer();
        pBuffer->Release();
    }

    pszURL = m_pURL->GetURL();

    pSource->SetPlayTimes(ulStart, ulEnd, ulDelay, ulDuration);

    pSource->PartOfNextGroup(m_bPartOfNextGroup);

#if defined(HELIX_FEATURE_PREFETCH)
    if (pSourceInfo->m_bPrefetch)
    {
        pSource->EnterPrefetch(pSourceInfo->m_prefetchType, pSourceInfo->m_ulPrefetchValue);
    }
#endif /* HELIX_FEATURE_PREFETCH */

    theErr = ((HXNetSource*)pSource)->Setup(pszHost, pszResource, (UINT16)ulPort, TRUE, m_pURL, bAltURL);

cleanup:

    HX_RELEASE(pURLProperties);

    // cleanup...
    if(theErr && pSource)
    {
        pSource->Stop();
        pSource->DoCleanup();
        HX_RELEASE(pSource);
    }

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_PLAYBACK_NET */
}

HX_RESULT HXPlayer::DoFileSystemOpen(SourceInfo*& pSourceInfo, HXBOOL bAltURL, IHXPluginSearchEnumerator* pFFClaim)
{
#if defined(HELIX_FEATURE_PLAYBACK_LOCAL)
    HX_RESULT   theErr = HXR_OK;
    HXSource*   pSource = NULL;
    IHXValues*  pURLProperties = NULL;
    UINT32      ulRegistryID = 0;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    IHXBuffer* pszParentName = NULL;
#endif

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::DoFileSystemOpen(pSourceInfo=%p, bAltURL=%c) Start", 
        this, 
        pSourceInfo,
        bAltURL ? 'T' :'F');

    pSource = pSourceInfo->m_pSource = NewFileSource();
    if (!pSource)
    {
        return( HXR_OUTOFMEMORY );
    }
    pSource->AddRef();

    UINT32 ulStart = 0, ulEnd = HX_EOF_TIME, ulDelay = 0, ulDuration = 0;    
    GetTimingFromURL(m_pURL, ulStart, ulEnd, ulDelay, ulDuration);

    if (m_pURL)
    {
        if (!(pURLProperties = m_pURL->GetProperties()))
        {
            theErr = HXR_FAILED;
            goto cleanup;
        }
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    // registry setting
    if (m_pRegistry && m_pStats)
    {
        HX_RESULT res    = HXR_OK;
        UINT32    nRegID = 0;

        if(m_bPartOfNextGroup)
            nRegID = m_ulNextGroupRegistryID;
        else
            nRegID = m_pStats->m_ulRegistryID;

        res = m_pRegistry->GetPropName(nRegID, pszParentName);
        if ( HXR_OK == res && pszParentName )
        {
            char        szSourceName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
            SafeSprintf(szSourceName, MAX_DISPLAY_NAME, "%s.Source%u", pszParentName->GetBuffer(),
                    pSourceInfo->m_uTrackID);

            // does this ID already exist?
            ulRegistryID = m_pRegistry->GetId(szSourceName);
            if ( !ulRegistryID )
            {
                ulRegistryID = m_pRegistry->AddComp(szSourceName);
            }
            HX_RELEASE(pszParentName);
        }
        else
        {
            HX_ASSERT(FALSE);
        }

    }
    HX_RELEASE(pszParentName);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    pSource->SetSourceInfo(pSourceInfo);
    pSource->Init(this, ulRegistryID);

    pSource->SetPlayTimes(ulStart, ulEnd, ulDelay, ulDuration);

    pSource->PartOfNextGroup(m_bPartOfNextGroup);

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    // /Do this before calling pSource->Setup(), below, otherwise observer
    // doesn't get added until after some initial status reporting happens:
    {
        HX_RELEASE(m_pPDStatusMgr);
        (HXFileSource*)pSource->QueryInterface(IID_IHXPDStatusMgr, (void**)&m_pPDStatusMgr);
        if (m_pPDStatusMgr)
        {
            m_pPDStatusMgr->AddObserver((IHXPDStatusObserver*)this); 
        }
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    // If we have a plugin enumerator of file formats
    // which have claimed the URL, then pass it
    // into the source
    if (pFFClaim)
    {
        ((HXFileSource*)pSource)->SetSchemeExtensionPairClaimedEnumerator(pFFClaim);
    }

    theErr = ((HXFileSource*)pSource)->Setup(m_pURL, bAltURL);

cleanup:

    HX_RELEASE(pURLProperties);

    // cleanup...
    if(theErr && pSource)
    {
        pSource->Stop();
        pSource->DoCleanup();
        HX_RELEASE(pSource);
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::DoFileSystemOpen(pSourceInfo=%p, bAltURL=%c) End", 
        this, 
        pSourceInfo,
        bAltURL ? 'T' :'F');

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_PLAYBACK_LOCAL */
}

HX_RESULT
HXPlayer::UnRegisterCurrentSources()
{
    HX_RESULT theErr = HXR_OK;
    
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        theErr = pSourceInfo->UnRegister();
    }

    return theErr;
}

HX_RESULT HXPlayer::InitializeRenderers(void)
{
    HX_RESULT theErr        = HXR_OK;
    HX_RESULT thefinalErr   = HXR_OK;

    SourceInfo*     pSourceInfo = NULL;
    HXBOOL            bSourceInitialized = TRUE;

    // assume everything will be initialized in this pass...
    HXBOOL            bAllInitialized = TRUE;

    UINT16 uInitialSourceCount = m_pSourceMap->GetCount();
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        pSourceInfo = (SourceInfo*)(*ndxSource);

        if (pSourceInfo->m_bInitialized)
            continue;

        theErr = pSourceInfo->InitializeRenderers(bSourceInitialized);

        if (theErr && !thefinalErr)
        {
            thefinalErr = theErr;
        }
#ifdef HELIX_FEATURE_PARTIALPLAYBACK
        else
        {
            if(pSourceInfo->m_bIsPartialPlayback)
            {
                // Warn the controller of the partial playback
                // and it is upto it to decide whether to warn 
                // user or to consume the warning
                ActualReport(HXLOG_WARNING, HXR_PARTIALPLAYBACK, HXR_OK, NULL, NULL);
            }
        }
#endif // End of #ifdef HELIX_FEATURE_PARTIALPLAYBACK

        if (!bSourceInitialized)
        {
            bAllInitialized      = FALSE;
        }

        /* Someone added a source during ProcessIdle.
         * Start all over again
         */
        if (uInitialSourceCount != m_pSourceMap->GetCount())
        {
            bAllInitialized = FALSE;
            break;
        }
    }

    if (!thefinalErr && bAllInitialized)
    {
        m_bInitialized = TRUE;

    SetClientState(HX_CLIENT_STATE_CONNECTED);
        if (m_pAdviseSink)
        {
            m_pAdviseSink->OnPosLength( m_ulCurrentPlayTime,
                                        m_ulPresentationDuration);
            m_pAdviseSink->OnBegin(m_ulCurrentPlayTime);
        }

    }

    if (!thefinalErr && m_bInitialized && m_pSourceMap->GetCount() == 0)
    {
        m_bPlayerWithoutSources = TRUE;
    }

#if defined(HELIX_FEATURE_PREFETCH)
    if (!thefinalErr && m_bInitialized)
    {
        while (m_pPendingTrackList && m_pPendingTrackList->GetCount() > 0)
        {
            PendingTrackInfo* pPendingTrackInfo =
                        (PendingTrackInfo*) m_pPendingTrackList->RemoveHead();

            theErr = OpenTrack(pPendingTrackInfo->m_pTrack,
                               pPendingTrackInfo->m_uGroupIndex,
                               pPendingTrackInfo->m_uTrackIndex);

            if (theErr && !thefinalErr)
            {
                thefinalErr = theErr;
            }

            delete pPendingTrackInfo;
        }

        if (thefinalErr)
        {
            ReportError(NULL, thefinalErr, NULL);
        }
    }
#endif /* HELIX_FEATURE_PREFETCH */

    return thefinalErr;
}

// get all packets from event queue and send them to the various renders
HX_RESULT
HXPlayer::ProcessCurrentEvents(UINT32 ulLoopEntryTime, 
                   HXBOOL bAtInterrupt, 
                   HXBOOL bFromInterruptSafeChain)
{
    HX_RESULT theErr = HXR_OK;
    LISTPOSITION listpos;
    CHXEvent* pEvent;
    IHXPacket* pPacket;

    /*
     * check for m_bLiveSeekToBeDone && m_bPaused is so that we do not issue
     * a seek (and start sending packets) before the user has issued a Begin()
     * after Pausing a live stream.
     *
     * Should work for now. Need to be re-visited when Live Pause support
     * is refined in the core.
     *
     * XXXRA
     */
    if (m_bProcessEventsLocked || (m_bLiveSeekToBeDone && m_bPaused))
    {
        return HXR_OK;
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessCurrentEvents() Start", this);

    if (m_EventList.GetNumEvents() == 0)
    {
        HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessCurrentEvents() End m_EventList.GetNumEvents() == 0", this);
        return HXR_OK;
    }

    m_bDidWeDeleteAllEvents = FALSE;
    m_bProcessEventsLocked  = TRUE;
    m_bEventAcceleration = FALSE;

    pEvent = m_EventList.GetHead();
    listpos = m_EventList.GetHeadPosition();

    do
    {
        pPacket = pEvent->GetPacket();

        /* since packet's time may be different from player's time, send
        * renderer the offset */
        HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::ProcessCurrentEvents() Packet Time: %lu", this, pPacket->GetTime());

        if (m_bLiveSeekToBeDone && !pEvent->IsPreSeekEvent())
        {
            /* Audio device was earlier seeked  based on the last timesync
            * + pause duration. The actual first packet time we get after
            * Pause may be different from the seek time. This is because
            * at pause time, packets may have been given in advance to
            * the renderers due to preroll. Also, they may be stored in
            * resend buffer at protocol level. So the only way to kind
            * of know what time we should have actually seeked to is based
            * on the first packet time. Even this may result in initial
            * re-buffering. This is still a hack. We probably need a
            * better live pause solution.
            */
            m_bLiveSeekToBeDone = FALSE;
            UINT32 ulCurTime = m_pAudioPlayer->GetCurrentPlayBackTime();
            /* make sure we have sent ATLEAST one ontimesync to the renderer.
            * otherwise, we cannot rely on the m_ulTimeAfterSeek value since it is based on
            * m_ulTimeDiff which only gets set in the first ontimesync call.
            * Needed to fix pause during inital buffering of live playback
            */
            if (!pEvent->m_pRendererInfo->m_bIsFirstTimeSync &&
                 pPacket->GetTime() > pEvent->m_pRendererInfo->m_pStreamInfo->m_ulTimeAfterSeek)
            {
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
                /* Cannot call audio player Seek() at interrupt time. It results
                * in audio device Reset() which issues DoImmediate commands
                */
                if (bAtInterrupt)
                {
                    SchedulePlayer(PLAYER_SCHEDULE_MAIN | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);
                    break;
                }
#endif
                UINT32 ulTimeDiff = 0;
                ulTimeDiff = pPacket->GetTime() -
                    pEvent->m_pRendererInfo->m_pStreamInfo->m_ulTimeAfterSeek;
                m_pAudioPlayer->Seek(ulCurTime + ulTimeDiff);
            }
        }

        if (!bAtInterrupt || pEvent->m_pRendererInfo->m_bInterruptSafe)
        {
            if (!pEvent->IsPreSeekEvent())
            {
                SendPostSeekIfNecessary(pEvent->m_pRendererInfo);
            }

            // remove this event from the packet list...
            listpos = m_EventList.RemoveAt(listpos);

            theErr = SendPacket(pEvent);

            /* A renderer may call SetCurrentGroup OR Stop() from within
            * OnPacket call. If so, we delete all the pending events
            */
            if (m_bDidWeDeleteAllEvents)
            {
                delete pEvent;
                break;
            }

            HX_ASSERT(pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued > 0);

            pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued--;

            if (pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued == 0 &&
                pEvent->m_pRendererInfo->m_pStreamInfo->m_bSrcInfoStreamDone)
            {
                SendPostSeekIfNecessary(pEvent->m_pRendererInfo);
                pEvent->m_pRendererInfo->m_bOnEndOfPacketSent = TRUE;
                if (pEvent->m_pRendererInfo->m_pRenderer)
                {

#if defined(HELIX_FEATURE_DRM)
                    /* ask DRM to flush packets */
                    HXSource * pSource = pEvent->m_pRendererInfo->m_pStream->GetHXSource();
                    if (pSource && pSource->IsHelixDRMProtected() && pSource->GetDRM())
                    {
                        pSource->GetDRM()->FlushPackets(TRUE);
                    }
#endif /* HELIX_FEATURE_DRM */

                    pEvent->m_pRendererInfo->m_pRenderer->OnEndofPackets();
                }
                if (m_bIsLive && m_ulPresentationDuration == 0 &&
                    pEvent->m_pRendererInfo->m_pStreamInfo->m_ulLastPacketTime > 0)
                {
                    // Set the duration, playout end of clip
                    SetPresentationTime(pEvent->m_pRendererInfo->m_pStreamInfo->m_ulLastPacketTime);
                    HXLOGL3(HXLOG_CORE, "HXPlayer[%p] Received end of packets event, duration set to : %ld",
                    this, m_ulPresentationDuration);
                }
            }

            // cleanup this event...
            delete pEvent;
            pEvent = NULL;

            if(m_bEventAcceleration)
            {
                m_bEventAcceleration = FALSE;
                listpos = m_EventList.GetHeadPosition();
            }

            // and get the next event...
            if (m_EventList.GetNumEvents() > 0)
            {
                pEvent = m_EventList.GetAt(listpos);
            }
        }
        else
        {
            pEvent = m_EventList.GetAtNext(listpos); //skip over event - *++listpos
        }

        // If loop entry time is set, we must limit the processing to the
        // player update interval duration.
        if ((ulLoopEntryTime != 0) &&
            ((HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime) > GetPlayerProcessingInterval(bFromInterruptSafeChain)))
        {
            HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessCurrentEvents() CPU use timeout: Time=%lu Allowed=%lu", 
            this, 
            HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime,
            GetPlayerProcessingInterval(bFromInterruptSafeChain));
            break;
        }
    } while (!theErr && pEvent);

    m_bDidWeDeleteAllEvents = FALSE;
    m_bProcessEventsLocked  = FALSE;
    m_bEventAcceleration = FALSE;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::ProcessCurrentEvents() End", this);

    return theErr;
}

HX_RESULT HXPlayer::SendPreSeekEvents()
{
    HX_RESULT theErr = HXR_OK;
    m_bEventAcceleration = FALSE;
    LISTPOSITION listpos = m_EventList.GetHeadPosition();
    while (listpos && (m_EventList.GetNumEvents() != 0))
    {
        CHXEvent*  pEvent = m_EventList.GetAt(listpos);

        HXBOOL bAtInterrupt = m_pEngine->AtInterruptTime();
        if (!bAtInterrupt || pEvent->m_pRendererInfo->m_bInterruptSafe)
        {
            listpos = m_EventList.RemoveAt(listpos);
            theErr = SendPacket(pEvent);

            HX_ASSERT(pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued > 0);
            pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued--;
            delete pEvent;

        if(m_bEventAcceleration)
        {
        m_bEventAcceleration = FALSE;
        listpos = m_EventList.GetHeadPosition();
        }
        }
        else
        {
            pEvent->SetPreSeekEvent();
            m_EventList.GetNext(listpos);
        }
    }
    m_bEventAcceleration = FALSE;

    SendPreSeekEventsExt();

    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        if (pSourceInfo->m_pSource && pSourceInfo->m_pSource->m_PacketBufferList.GetCount() > 0)
        {
            CHXSimpleList* pPacketList = &pSourceInfo->m_pSource->m_PacketBufferList;

            LISTPOSITION pos = pPacketList->GetHeadPosition();
            while (pos != NULL)
            {
                CHXEvent* pTempEvent  = (CHXEvent*) pPacketList->GetNext(pos);
                pTempEvent->SetPreSeekEvent();
            }
        }
    }


    return theErr;
}

HX_RESULT 
HXPlayer::AccelerateEventsForSource(HXSource* pSource)
{
    m_bEventAcceleration = TRUE;
    return m_EventList.MakeSourceEventsImmediate(pSource);
}

HX_RESULT HXPlayer::DeleteAllEvents()
{
    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::DeleteAllEvents()", this);
    while (m_EventList.GetNumEvents() != 0)
    {
        CHXEvent*  pEvent = m_EventList.RemoveHead();
        /* If these packets belong to a persistent source,
         * pass them over.
         */
        if (pEvent->m_pRendererInfo->m_bIsPersistent)
        {
            HXBOOL bAtInterrupt = m_pEngine->AtInterruptTime();
            if (!bAtInterrupt || pEvent->m_pRendererInfo->m_bInterruptSafe)
            {
                SendPacket(pEvent);
            }
        }
        delete pEvent;
    }

    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        if (pSourceInfo->m_pSource)
        {
            pSourceInfo->m_pSource->DeleteAllEvents();
        }
    }

    m_bDidWeDeleteAllEvents = TRUE;
    return HXR_OK;
}

HX_RESULT
HXPlayer::EventReady(HXSource* pSource, CHXEvent* pEvent)
{
    HX_RESULT theErr = HXR_OK;

    SourceInfo*     pSourceInfo = NULL;
    RendererInfo*   pRendInfo   = NULL;

    if (!m_pSourceMap->Lookup(pSource, (void*&) pSourceInfo))
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    if (!pSourceInfo->m_pRendererMap->Lookup((pEvent->GetPacket())->GetStreamNumber(), (void*&) pRendInfo))
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    pEvent->m_pRendererInfo = pRendInfo;

    HXBOOL bOkToSend = (!m_pEngine->AtInterruptTime() ||
                      pEvent->m_pRendererInfo->m_bInterruptSafe);
    if (pEvent->IsPreSeekEvent() && bOkToSend)
    {
        SendPacket(pEvent);
        delete pEvent;
    }
    else
    {
        m_EventList.InsertEvent(pEvent);
        pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued++;
    }

    return theErr;
}

HX_RESULT HXPlayer::SendPacket(CHXEvent* pEvent)
{
    RendererInfo* pRendInfo;
    IHXRenderer*  pRend;
    HX_RESULT     retVal = HXR_OK;
    
    pRendInfo = pEvent->m_pRendererInfo;
    pRend     = pRendInfo->m_pRenderer;
    
    //If we are in quick seek mode we only deliver packets to
    //video renderers.
    if (pRend && (!m_bQuickSeekMode || !pRendInfo->m_bIsAudioRenderer))
    {
    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::SendPacket Strm=%hu TS=%lu PlayTime=%lu",
            this,
            pEvent->GetPacket()->GetStreamNumber(),
            pEvent->GetPacket()->GetTime(),
                        GetInternalCurrentPlayTime());

#if defined(HELIX_FEATURE_DRM)
        HXSource* pSource = pEvent->m_pRendererInfo->m_pStream->GetHXSource();
        if (pSource && pSource->IsHelixDRMProtected() && pSource->GetDRM())
        {
            pSource->GetDRM()->ProcessEvent(pEvent);
        }
        else
#endif /* HELIX_FEATURE_DRM */
        {
            retVal = pRend->OnPacket( pEvent->GetPacket(), pEvent->GetTimeOffset() );
        }
    }

    if (retVal == HXR_OUTOFMEMORY)
    {
        Report(HXLOG_ERR, retVal, HXR_OK, "Ran out of memory in SendPacket", NULL);
    }
    return retVal;
}

HXBOOL
HXPlayer::AreAllPacketsSent()
{
    HXBOOL bAllSent = TRUE;
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        if (pSourceInfo->m_pSource && 
            pSourceInfo->m_pSource->m_PacketBufferList.GetCount() > 0)
        {
            bAllSent = FALSE;
            break;
        }
    }

    bAllSent = bAllSent & (m_EventList.GetNumEvents() == 0);
    return bAllSent;
}


void
HXPlayer::StopAllStreams(EndCode endCode)
{
    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::StopAllStreams(): code = %ld", this, endCode);

    /* If we do not have access to engine any more, it means it is the final
     * destruction and everything has been deleted by now
     */
    if (!m_pEngine)
        return;

    StopAllStreamsExt(endCode);

    m_bIsDone = TRUE;

    if (m_bInStop)
        return;

    m_bInStop = TRUE;

    m_bIsPlaying = FALSE;
    m_bIsBuffering = FALSE;

    /* stop timeline */
    m_pAudioPlayer->Stop(TRUE);
    m_bPendingAudioPause = FALSE;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_pUpdateStatsCallback->m_bIsCallbackPending)
    {
        m_pUpdateStatsCallback->m_bIsCallbackPending = FALSE;
        m_pScheduler->Remove(m_pUpdateStatsCallback->m_PendingHandle);
        m_pUpdateStatsCallback->m_PendingHandle = 0;
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    RemovePendingCallback(m_pHXPlayerCallback);
    RemovePendingCallback(m_pHXPlayerInterruptCallback);
    RemovePendingCallback(m_pHXPlayerInterruptOnlyCallback);

#if defined(HELIX_FEATURE_AUTHENTICATION)
    ClearPendingAuthenticationRequests();
#endif /* HELIX_FEATURE_AUTHENTICATION */

    m_bCloseAllRenderersPending     = TRUE;

    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::StopAllStreams(): stopping sources", this);

    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo *) (*ndxSource);

//{FILE* f1 = ::fopen("c:\\temp\\ts.txt", "a+"); ::fprintf(f1, "%p pSourceInfo::Stop, ThreadID: %lu\n", pSourceInfo, GetCurrentThreadId());::fclose(f1);}

        pSourceInfo->Stop(endCode);

        if (pSourceInfo->m_pPeerSourceInfo)
        {
            pSourceInfo->m_pPeerSourceInfo->Stop(endCode);
        }
    }

    DeleteAllEvents();

    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::DeleteAllEvents(): notifying sinks (OnStop, OnPresentationClosed)", this);

    if (m_bIsPresentationClosedToBeSent)
    {
        m_bIsPresentationClosedToBeSent = FALSE;
    SetClientState(HX_CLIENT_STATE_READY);
        if (m_pAdviseSink)
        {
        if (endCode == END_DURATION)
        {
            m_pAdviseSink->OnPosLength( (m_bIsLive && m_ulPresentationDuration == 0) ?
                                    m_ulCurrentPlayTime : m_ulPresentationDuration,
                                    m_ulPresentationDuration);
        }
            m_pAdviseSink->OnStop();
            m_pAdviseSink->OnPresentationClosed();
        }
    }
    m_bInStop = FALSE;

    m_bCurrentPresentationClosed    = TRUE;
}

HX_RESULT
HXPlayer::StopAllStreamsExt(EndCode endCode)
{
    return HXR_OK;
}

HX_RESULT
HXPlayer::SendPreSeekEventsExt()
{
    return HXR_OK;
}

SourceInfo*
HXPlayer::NewSourceInfo(void)
{
    return (new SourceInfo(this));
}

HXFileSource*
HXPlayer::NewFileSource(void)
{
#if defined(HELIX_FEATURE_PLAYBACK_LOCAL)
    return (new HXFileSource());
#else
    return NULL;
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_LOCAL) */
}

HXNetSource*
HXPlayer::NewNetSource(void)
{
#if defined(HELIX_FEATURE_PLAYBACK_NET)
    return (new HXNetSource());
#else
    return NULL;
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_NET) */
}

void
HXPlayer::CloseAllRenderers(INT32 nGroupSwitchTo)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CloseAllRenderers()", this);
    // these will become invalid when we delete SourceInfo objects in m_pSourceMap
    HX_DELETE(m_pAltURLs);
    HX_DELETE(m_pOppositeHXSourceTypeRetryList);
    HX_DELETE(m_pRedirectList);
    HX_DELETE(m_pSDPURLList);

    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CloseAllRenderers(): src map [%p]", this, m_pSourceMap);
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);

        HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CloseAllRenderers(): closing source", this);
        
        if (pSourceInfo->m_pPeerSourceInfo)
        {
            pSourceInfo->m_pPeerSourceInfo->CloseRenderers();
            HX_DELETE(pSourceInfo->m_pPeerSourceInfo);
        }

        if (pSourceInfo->CloseRenderers() == HXR_OK)
        {
            HX_DELETE(pSourceInfo);
        }
    }

    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CloseAllRenderers(): clearing map", this);
    m_pSourceMap->RemoveAll();
    m_bSourceMapUpdated = TRUE;

#if defined(HELIX_FEATURE_NESTEDMETA)
    if (m_pPersistentComponentManager)
    {
        m_pPersistentComponentManager->CloseAllRenderers(nGroupSwitchTo);
    }
#else
    CleanupLayout();
#endif /* HELIX_FEATURE_NESTEDMETA */

#ifdef _MACINTOSH
    unsigned long lastFree = ::TempFreeMem();
    long deltaFree = 1;
    long iterationCount = 0;
    while (deltaFree > 0 && iterationCount < 5)
    {
        HXMM_COMPACT();
        unsigned long curFree = ::TempFreeMem();
        deltaFree = curFree - lastFree;
        lastFree = curFree;
        iterationCount++;
    }
#endif

    m_bCloseAllRenderersPending = FALSE;
}

/* Reset the player condition
 * Remove all sources and re-initialize all variables...
 */
void
HXPlayer::ResetPlayer(void)
{
    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::ResetPlayer()", this);

    if (m_pCookies3)
    {
#if defined(HELIX_FEATURE_COOKIES)
        m_pCookies3->SyncRMCookies(TRUE);
#endif /* defined(HELIX_FEATURE_COOKIES) */
    }

    EmptyBeginList();
    ResetGroup();

    if (m_pAltURLs)
    {
        m_pAltURLs->RemoveAll();
    }

    if (m_pOppositeHXSourceTypeRetryList)
    {
        m_pOppositeHXSourceTypeRetryList->RemoveAll();
    }

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    if (m_pGroupManager && m_pGroupManager->GetGroupCount() > 0)
    {
        m_pGroupManager->RemoveAllGroup();
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    if (m_pNextGroupManager)
    {
        m_pNextGroupManager->Cleanup();
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

#if defined(HELIX_FEATURE_PREFETCH)
    if (m_pPrefetchManager)
    {
        m_pPrefetchManager->Cleanup();
    }
#endif /* HELIX_FEATURE_PREFETCH */

#if defined(HELIX_FEATURE_NESTEDMETA)
    if (m_pPersistentComponentManager)
    {
        m_pPersistentComponentManager->Reset();
    }
#endif /* HELIX_FEATURE_NESTEDMETA */

    m_bSetupLayoutSiteGroup = TRUE;

    m_bIsDone               = TRUE;
    m_bIsPresentationDone   = TRUE;
    m_bPlayerWithoutSources = FALSE;

    m_bPartOfNextGroup      = FALSE;
    m_bLastGroup            = FALSE;
    m_bNextGroupStarted     = FALSE;
    m_bUserHasCalledBegin   = FALSE;
    m_bSourceMapUpdated     = FALSE;
    m_bPrefTransportInitialized = FALSE;
    m_bAddLayoutSiteGroupCalled = FALSE;

    if (!m_bDoRedirect && !m_bBeginChangeLayoutTobeCalled)
    {
        m_bBeginChangeLayoutTobeCalled = TRUE;
#if defined(HELIX_FEATURE_VIDEO)
        if (m_pSiteSupplier)
        {
            m_pSiteSupplier->DoneChangeLayout();
        }
#endif /* HELIX_FEATURE_VIDEO */
    }

#if defined(HELIX_FEATURE_ASM)
    if (m_pBandwidthMgr)
    {
        m_pBandwidthMgr->PresentationDone();
    }
#endif /* HELIX_FEATURE_ASM */

    if (m_bPlayStateNotified && m_pEngine)
    {
        m_bPlayStateNotified = FALSE;
        m_pEngine->NotifyPlayState(m_bPlayStateNotified);
    }

#if defined(HELIX_FEATURE_VIDEO)
    if (m_pSiteManager)
    {
        m_pSiteManager->NeedFocus(FALSE);
    }
#endif /* HELIX_FEATURE_VIDEO */

#if defined(HELIX_FEATURE_PLAYBACK_NET) && !defined(_SYMBIAN) && !defined(_OPENWAVE)
    /* Clean DNS cache */
    conn::clear_cache();
#endif /* HELIX_FEATURE_PLAYBACK_NET */

    /* Compact memory pools now that we are done with a presentation */
#ifdef _MACINTOSH
    unsigned long lastFree = ::TempFreeMem();
    long deltaFree = 1;
    long iterationCount = 0;
    while (deltaFree > 0 && iterationCount < 5)
    {
        HXMM_COMPACT();
        unsigned long curFree = ::TempFreeMem();
        deltaFree = curFree - lastFree;
        lastFree = curFree;
        iterationCount++;
    }
#endif
    // Close any velocity control
    CloseVelocityControl();
}

/* Before playing the next group
 * reset everything ResetPlayer does except .
 */
void
HXPlayer::ResetGroup(void)
{
    HXLOGL4(HXLOG_CORE, "HXPlayer[%p]::ResetGroup()", this);
    m_uNumSourcesActive     = 0;
    m_uNumCurrentSourceNotDone = 0;
    m_bInitialized          = FALSE;
    m_bSetupLayoutSiteGroup = TRUE;
    m_ulCurrentPlayTime     = 0;
#if defined(_MACINTOSH)
    m_ulCurrentSystemPlayTime = 0;
#endif
    m_ulPresentationDuration  = 0;
    m_ulTimeBeforeSeek      = 0;
    m_ulTimeAfterSeek       = 0;
    m_BufferingReason       = BUFFERING_START_UP;
    m_bIsDone               = FALSE;
    m_bPaused               = FALSE;
    m_bBeginPending         = FALSE;
    m_bTimelineToBeResumed  = FALSE;
    m_bIsPlaying            = FALSE;
    m_bIsBuffering      = FALSE;
    m_bSetupToBeDone        = FALSE;
    m_bPostSetupToBeDone    = FALSE;

    /* default DEFAULT_TIMESYNC_GRANULARITY ms timesyncs are
     * given to renderers
     */
    m_ulLowestGranularity   = DEFAULT_TIMESYNC_GRANULARITY;

    m_bIsFirstBegin         = TRUE;
    m_bIsLive               = FALSE;
    m_LastError             = HXR_OK;
    m_bInternalPauseResume  = FALSE;
    m_bSetVelocityInProgress = FALSE;
    m_bInternalReset        = FALSE;
    m_bContactingDone       = FALSE;

    m_bIsFirstTimeSync      = TRUE;
    m_ulFirstTimeSync       = 0;

    m_ulElapsedPauseTime    = 0;
    m_ulLiveSeekTime        = 0;
    m_ulTimeOfPause         = 0;
    m_bLiveSeekToBeDone     = FALSE;
    m_bFastStartInProgress  = FALSE;
    m_ulFSBufferingEndTime  = 0;
    m_bFSBufferingEnd       = FALSE;
    m_b100BufferingToBeSent = TRUE;
    m_uNumSourceToBeInitializedBeforeBegin = 0;
    m_bResumeOnlyAtSystemTime = FALSE;

#if defined(HELIX_FEATURE_PREFETCH)
    if (m_pPrefetchManager)
    {
        m_pPrefetchManager->Cleanup();
        HX_DELETE(m_pPrefetchManager);
    }
#endif /* HELIX_FEATURE_PREFETCH */

    HX_RELEASE(m_pCurrentGroup);

    while (m_pPendingTrackList && m_pPendingTrackList->GetCount() > 0)
    {
        PendingTrackInfo* pPendingTrackInfo =
                    (PendingTrackInfo*) m_pPendingTrackList->RemoveHead();

        delete pPendingTrackInfo;
    }
}

/************************************************************************
 *      Method:
 *              HXPlayer::ReportError
 *      Purpose:
 *              The source object reports of any fatal errors.
 *
 */
void
HXPlayer::ReportError
(
    HXSource*  pSource,
    HX_RESULT   theErr,
    const char* pUserString
)
{
    SourceInfo* pSourceInfo = NULL;
    CHXURL*     pURL = NULL;

    if (pSource && theErr != HXR_DNR && theErr != HXR_PROXY_DNR)
    {
        pURL = pSource->GetCHXURL();

        // Alt-URL only in network playback mode
        if (pURL                                                    &&
            (m_pSourceMap->Lookup(pSource, (void*&)pSourceInfo))
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
            || (m_pNextGroupManager                                 &&
            m_pNextGroupManager->Lookup(pSource, pSourceInfo))
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
#if defined(HELIX_FEATURE_PREFETCH)
            || (m_pPrefetchManager                                  &&
            m_pPrefetchManager->Lookup(pSource, pSourceInfo))
#endif /* HELIX_FEATURE_PREFETCH */
            )
        {
            // There can be multiple RTSP client implementations within
            // Helix. If we tried one of the RTSP implementations against
            // an incompatible server, then the HXR_INCOMPATIBLE_RTSP_SERVER
            // error will be returned by that source.
            if (pURL->IsNetworkProtocol() && theErr == HXR_INCOMPATIBLE_RTSP_SERVER)
            {
                // HXSource's won't fail out here until they have tried all
                // of their possibilities. So if a source fails out here,
                // then we should try the other HXSource type. However,
                // if we see that we've already tried that source type, then
                // fail out.
                if ((pSource->IsLocalSource() && !pSourceInfo->m_bNetSourceFailed) ||
                    (!pSource->IsLocalSource() && !pSourceInfo->m_bFileSourceFailed))
                {
                    // We will add this SourceInfo to a list so
                    // that at the next ProcessIdle, we will try
                    // the opposite HXSource.
                    //
                    // Set the flag saying which HXSource type failed
                    if (pSource->IsLocalSource())
                    {
                        pSourceInfo->m_bFileSourceFailed = TRUE;
                    }
                    else
                    {
                        pSourceInfo->m_bNetSourceFailed = TRUE;
                    }
                    // Do we already have such a list?
                    if (!m_pOppositeHXSourceTypeRetryList)
                    {
                        // Create the list
                        m_pOppositeHXSourceTypeRetryList  = new CHXSimpleList();
                    }
                    if (m_pOppositeHXSourceTypeRetryList)
                    {
                        // Is this SourceInfo already in the list?
                        if (!m_pOppositeHXSourceTypeRetryList->Find(pSourceInfo))
                        {
                            // This SourceInfo is not already in the list
                            //
                            // Save the original error
                            pSourceInfo->m_lastErrorFromMainURL       = theErr;
                            pSourceInfo->m_lastErrorStringFromMainURL = pUserString;
                            // Add this SourceInfo to the list
                            m_pOppositeHXSourceTypeRetryList->AddTail(pSourceInfo);
                        }
                        goto cleanup;
                    }
                }
            }
            if (pSourceInfo && pSourceInfo->m_bAltURL && !pSource->HasAltURL())
            {
                ResetError();
                /* If this error is for the next group, we do not want to
                 * display the error in the current group playback. Instead,
                 * pass it to the next group manager (by falling out of this
                 * if condition) so that it can be displayed when the next
                 * group is played
                 */
                if (!pSourceInfo->m_pSource ||
                    (!pSourceInfo->m_pSource->IsPartOfNextGroup() &&
                     !pSourceInfo->m_pSource->IsPartOfPrefetchGroup()))
                {
                    // return the last error from the main URL instead
                    Report(HXLOG_ERR, pSourceInfo->m_lastErrorFromMainURL,
                           HXR_OK, pSourceInfo->m_lastErrorStringFromMainURL, NULL);
                    goto cleanup;
                }
                else
                {
                    /* Use the error from the main URL even for the next group */
                    theErr = pSourceInfo->m_lastErrorFromMainURL;
                    pUserString = pSourceInfo->m_lastErrorStringFromMainURL;
                    /* Fall through to report this error to the
                     * next group manager
                     */
                }
            }
#if defined(HELIX_FEATURE_ALT_URL)
            // switch to Alt-URL when:
            // * the network playback has succeeded in this session or
            //   the network playback is in HTTP Cloaking mode
            //   AND
            // * the pSource is at SourceMap OR
            // * the pSource is at the NextGroupManager
            else if ((theErr != HXR_NOT_AUTHORIZED) &&
                     !IS_SERVER_ALERT(theErr)       &&
                     pSource->HasAltURL())
            {
                if (!m_pAltURLs)
                {
                    m_pAltURLs  = new CHXSimpleList();
                }

                if (!m_pAltURLs->Find(pSourceInfo))
                {
                    pSourceInfo->m_lastErrorFromMainURL = theErr;
                    pSourceInfo->m_lastErrorStringFromMainURL = pUserString;

                    m_pAltURLs->AddTail(pSourceInfo);
                }
                goto cleanup;
            }
#endif /* HELIX_FEATURE_ALT_URL */
        }
    }

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    /* Check if this error is for the next group */
    if (m_pNextGroupManager &&
        m_pNextGroupManager->ReportError(pSource, theErr, pUserString))
    {
        goto cleanup;
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

    Report(HXLOG_ERR, theErr, HXR_OK, pUserString, NULL);

cleanup:

    return;
}

HX_RESULT HXPlayer::InitializeNetworkDrivers(void)
{
#if defined(HELIX_FEATURE_PLAYBACK_NET)
    if (!m_bNetInitialized)
    {

#if defined( _WIN32 ) || defined( _WINDOWS )
        //  Have we been able to load and initialize the winsock stuff yet?
        m_bNetInitialized = win_net::IsWinsockAvailable(this);
#elif defined (_MACINTOSH)
        m_bNetInitialized = (conn::init_drivers(NULL) == HXR_OK);
#elif defined(_UNIX) || defined(_SYMBIAN) || defined(__TCS__) || defined(_OPENWAVE)
        m_bNetInitialized = TRUE;
#endif

    }

    return (m_bNetInitialized) ? HXR_OK : HXR_GENERAL_NONET;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_PLAYBACK_NET */
}

void
HXPlayer::EventOccurred(HXxEvent* pEvent)
{
  // Note: Unix player does not go through here.
  // calls for UNIX go directly from Client Engine to
  // Site Window class
#if defined (_MACINTOSH) || defined(_MAC_UNIX)

//    if (HXMM_RAHUL_CHECK())
//    {
//        DebugStr("\pEventOccurred SYSTEM Task ENTER;g");
//    }

#ifdef _MACINTOSH
    extern ULONG32         gTIMELINE_MUTEX;

    InterlockedIncrement(&gTIMELINE_MUTEX);
#endif

    if (((pEvent == NULL) || (pEvent->event == nullEvent)) && m_bIsPlaying)
    {
        ULONG32 ulAudioTime = m_pAudioPlayer->GetCurrentPlayBackTime();
        if ( m_ulCurrentSystemPlayTime != ulAudioTime )
        {
            OnTimeSync( ulAudioTime );
        }
    }

#ifdef _MACINTOSH
    InterlockedDecrement(&gTIMELINE_MUTEX);
#endif

//    if (HXMM_RAHUL_CHECK())
//    {
//        DebugStr("\pEventOccurred SYSTEM Task LEAVE;g");
//    }
#endif
}

#ifdef _UNIX
void
HXPlayer::CollectSelectInfo(INT32* n,
                             fd_set* readfds,
                             fd_set* writefds,
                             fd_set* exceptfds,
                             struct timeval* tv)
{
}

void
HXPlayer::ProcessSelect(INT32* n,
                         fd_set* readfds,
                         fd_set* writefds,
                         fd_set* exceptfds,
                         struct timeval* tv)
{
}
#endif

void
HXPlayer::SetGranularity(ULONG32 ulGranularity)
{
    if (m_ulLowestGranularity > ulGranularity)
    {
        m_ulLowestGranularity = ulGranularity;
    }
    /* sanity check */
    if (m_ulLowestGranularity < MINIMUM_TIMESYNC_GRANULARITY)
    {
        m_ulLowestGranularity = MINIMUM_TIMESYNC_GRANULARITY;
    }
}

HX_RESULT
HXPlayer::SetGranularity(HXSource* pSource, UINT16 uStreamNumber,
                          UINT32 ulGranularity)
{
    SourceInfo*     pSourceInfo = pSource->m_pSourceInfo;
    RendererInfo*   pRendInfo   = NULL;

    HX_ASSERT(pSourceInfo);

    if (!pSourceInfo->m_pRendererMap->Lookup((LONG32) uStreamNumber, (void*&) pRendInfo))
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    pRendInfo->m_ulGranularity  = ulGranularity;

    /* sanity check */
    if (pRendInfo->m_ulGranularity < MINIMUM_TIMESYNC_GRANULARITY)
    {
        pRendInfo->m_ulGranularity = MINIMUM_TIMESYNC_GRANULARITY;
    }

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *              HXPlayer::ClosePlayer
 *      Purpose:
 *              Just adding a lock around calls to Close()
 *
 */
void HXPlayer::ClosePlayer(void)
{
    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;
    Close();
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();
}

void
HXPlayer::AbortPlayer(void)
{
    // Stop completely...
    m_bIsPresentationClosedToBeSent = TRUE;
    m_bIsDone = TRUE;

    StopPlayer(END_ABORT);

#if defined(HELIX_FEATURE_VIDEO)
    /*
     * Let the site supplier know that we are done changing the layout.
     */
    if (m_pSiteSupplier && !m_bBeginChangeLayoutTobeCalled)
    {
        m_bBeginChangeLayoutTobeCalled  = TRUE;
        m_pSiteSupplier->DoneChangeLayout();
    }
#endif /* HELIX_FEATURE_VIDEO */
}

void
HXPlayer::Close()
{
    StopPlayer(END_STOP);
    CloseAllRenderers(m_nCurrentGroup);

    if (m_pMetaInfo)
    {
        m_pMetaInfo->Close();
        HX_RELEASE(m_pMetaInfo);
    }

#if defined HELIX_FEATURE_EMBEDDED_UI
    HX_RELEASE(m_pEmbeddedUI);
#endif // HELIX_FEATURE_EMBEDDED_UI

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_pRegistry)
    {
        // delete player stats
        if (m_pStats && m_pStats->m_ulRegistryID)
        {
            m_pRegistry->DeleteById(m_pStats->m_ulRegistryID);
        }
        HX_DELETE(m_pStats);

        // delete repeat stats
        if (m_ulRepeatedRegistryID)
        {
            m_pRegistry->DeleteById(m_ulRepeatedRegistryID);
            m_ulRepeatedRegistryID = 0;
        }

        // delete nextgroup stats
        if (m_ulNextGroupRegistryID)
        {
            m_pRegistry->DeleteById(m_ulNextGroupRegistryID);
            m_ulNextGroupRegistryID = 0;
        }
    }

    if (m_pUpdateStatsCallback)
    {
        if (m_pUpdateStatsCallback->m_bIsCallbackPending)
        {
            m_pUpdateStatsCallback->m_bIsCallbackPending = FALSE;
            m_pScheduler->Remove(m_pUpdateStatsCallback->m_PendingHandle);
            m_pUpdateStatsCallback->m_PendingHandle = 0;
        }

        HX_RELEASE(m_pUpdateStatsCallback);
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    if (m_bIsPresentationClosedToBeSent)
    {
        m_bIsPresentationClosedToBeSent = FALSE;
        SetClientState(HX_CLIENT_STATE_READY);
        if (m_pAdviseSink)
        {
            m_pAdviseSink->OnStop();
            m_pAdviseSink->OnPresentationClosed();
        }
    }
    HX_RELEASE (m_pAdviseSink);

    RemovePendingCallback(m_pHXPlayerCallback);
    HX_RELEASE(m_pHXPlayerCallback);

    RemovePendingCallback(m_pHXPlayerInterruptCallback);
    HX_RELEASE(m_pHXPlayerInterruptCallback);

    RemovePendingCallback(m_pHXPlayerInterruptOnlyCallback);
    HX_RELEASE(m_pHXPlayerInterruptOnlyCallback);

#if defined(HELIX_FEATURE_AUTHENTICATION)
    if (m_pAuthenticationCallback)
    {
        ClearPendingAuthenticationRequests();
        HX_RELEASE(m_pAuthenticationCallback);
    }
#endif /* HELIX_FEATURE_AUTHENTICATION */

    HX_DELETE(m_pAuthenticationRequestsPending);
    HX_RELEASE(m_pAutheticationValues);

    HX_RELEASE(m_pRequest);

    HX_DELETE (m_pRedirectList);
    HX_DELETE (m_pSDPURLList);
    HX_DELETE (m_pURL);
    HX_DELETE (m_pAltURLs);
    HX_DELETE(m_pOppositeHXSourceTypeRetryList);

#if defined(HELIX_FEATURE_REGISTRY)
    HX_RELEASE (m_pRegistry);
#endif /* HELIX_FEATURE_REGISTRY */

    HX_RELEASE (m_pEngine);
    HX_RELEASE (m_pClient);
    HX_RELEASE (m_pScheduler);
    HX_RELEASE (m_pClientStateAdviseSink);

    if (m_pErrorSinkControl)
    {
#if defined(HELIX_FEATURE_SINKCONTROL)
        // Remove the translator as an error sink
        // FALSE means remove.
        AddRemoveErrorSinkTranslator(FALSE);
        // Close the error sink control
        m_pErrorSinkControl->Close();
#if defined(HELIX_FEATURE_LOGGING_TRANSLATOR)
        HX_RELEASE(m_pErrorSinkTranslator);
#endif /* #if defined(HELIX_FEATURE_LOGGING_TRANSLATOR) */
#endif /* HELIX_FEATURE_SINKCONTROL */
        HX_RELEASE (m_pErrorSinkControl);
    }

    HX_RELEASE(m_pClientRequestSink);

#if defined(HELIX_FEATURE_PREFERENCES)
    HX_RELEASE (m_pPreferences);
#endif /* HELIX_FEATURE_PREFERENCES */

#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    if (m_pHyperNavigate)
    {
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
        m_pHyperNavigate->Close();
#endif /* defined(HELIX_FEATURE_HYPER_NAVIGATE) */
        HX_RELEASE (m_pHyperNavigate);
    }
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    if (m_pGroupManager)
    {
        m_pGroupManager->RemoveSink(this);
#if defined(HELIX_FEATURE_NESTEDMETA)
        m_pGroupManager->RemoveSink(m_pPersistentComponentManager);
#endif /* HELIX_FEATURE_NESTEDMETA */
        HX_RELEASE(m_pGroupManager);
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */

#if defined(HELIX_FEATURE_VIDEO)
    HX_RELEASE (m_pSiteManager);
    HX_RELEASE (m_pSiteSupplier);
#endif /* HELIX_FEATURE_VIDEO */
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    HX_RELEASE (m_pUpgradeCollection);
#endif /* HELIX_FEATURE_AUTOUPGRADE */
#if defined(HELIX_FEATURE_PACKETHOOKMGR)
    HX_RELEASE (m_pPacketHookManager);
#endif /* HELIX_FEATURE_PACKETHOOKMGR */
#if defined(HELIX_FEATURE_SMARTERNETWORK)
    HX_RELEASE (m_pPreferredTransportManager);
#endif /* HELIX_FEATURE_SMARTERNETWORK */
    HX_RELEASE (m_pNetInterfaces);
    HX_RELEASE (m_pClientViewSource);
    HX_RELEASE (m_pViewPortManager);
    HX_RELEASE (m_pClientViewRights);
    HX_RELEASE (m_pCookies3);

#if defined(HELIX_FEATURE_MEDIAMARKER)
    if (m_pMediaMarkerManager)
    {
        m_pMediaMarkerManager->Close();
        HX_RELEASE(m_pMediaMarkerManager);
    }
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */

#if defined(HELIX_FEATURE_EVENTMANAGER)
    if (m_pEventManager)
    {
        m_pEventManager->Close();
        HX_RELEASE(m_pEventManager);
    }
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */

#if defined(HELIX_FEATURE_NESTEDMETA)
    if (m_pPersistentComponentManager)
    {
        m_pPersistentComponentManager->Close();
        HX_RELEASE (m_pPersistentComponentManager);
    }
#endif /* HELIX_FEATURE_NESTEDMETA */

#if defined(HELIX_FEATURE_AUDIO)
    if (m_pAudioPlayer)
    {
        CHXAudioSession* pAudioSession = m_pAudioPlayer->GetOwner();
        if (pAudioSession)
        {
            pAudioSession->CloseAudioPlayer(m_pAudioPlayer);
        }

        HX_RELEASE (m_pAudioPlayer);
    }
#endif /* HELIX_FEATURE_AUDIO */

    if (m_pMasterTAC)
    {
#if defined(HELIX_FEATURE_MASTERTAC)
        m_pMasterTAC->Close();
#endif /* HELIX_FEATURE_MASTERTAC */
        HX_RELEASE(m_pMasterTAC);
    }

#if defined(HELIX_FEATURE_PREFETCH)
    HX_DELETE(m_pPrefetchManager);
#endif /* HELIX_FEATURE_PREFETCH */

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    HX_DELETE(m_pNextGroupManager);
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

#if defined(HELIX_FEATURE_ASM)
    if (m_pBandwidthMgr)
    {
        m_pBandwidthMgr->PresentationDone();
        HX_RELEASE(m_pBandwidthMgr);
    }
#endif /* HELIX_FEATURE_ASM */

#if defined(HELIX_FEATURE_ASM)
    HX_RELEASE(m_pASM);
#endif /* HELIX_FEATURE_ASM */

    HX_ASSERT(!m_pPendingTrackList || m_pPendingTrackList->IsEmpty());
    HX_DELETE(m_pPendingTrackList);

    if (m_pChildPlayerList)
    {
        CHXSimpleList::Iterator i = m_pChildPlayerList->Begin();
        for (; i != m_pChildPlayerList->End(); ++i)
        {
            IHXPlayer* pChildPlayer = (IHXPlayer*)(*i);
            HX_RELEASE(pChildPlayer);
        }
        HX_DELETE(m_pChildPlayerList);
    }

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    HX_RELEASE(m_pPDStatusMgr);

    if (m_pPDSObserverList)
    {
        while (!m_pPDSObserverList->IsEmpty())
        {
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)m_pPDSObserverList->RemoveHead();
            HX_RELEASE(pObserver);
        }
        HX_DELETE(m_pPDSObserverList);
    }
    if (m_pProgDnldStatusReportInfoList)
    {
        CHXSimpleList::Iterator i = m_pProgDnldStatusReportInfoList->Begin();
        for (; i != m_pProgDnldStatusReportInfoList->End(); ++i)
        {
            CProgDnldStatusRptInfo* pPDSRInfo = (CProgDnldStatusRptInfo*)(*i);
            delete pPDSRInfo;
        }
        HX_DELETE(m_pProgDnldStatusReportInfoList);
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
#if defined(HELIX_FEATURE_EVENTMANAGER)
    if (m_pPFSEventProxyList)
    {
        CHXSimpleList::Iterator i = m_pPFSEventProxyList->Begin();
        for (; i != m_pPFSEventProxyList->End(); ++i)
        {
            CPresentationFeatureEventProxy* pPFEventProxy =
                    (CPresentationFeatureEventProxy*)(*i);
            HX_DELETE(pPFEventProxy);
        }
        HX_DELETE(m_pPFSEventProxyList);
    }
#else //  If Events are not supported, then notify the P.F.Sinks directly:
    if (m_pPFSSinkList)
    {
        CHXSimpleList::Iterator i = m_pPFSSinkList->Begin();
        for (; i != m_pPFSSinkList->End(); ++i)
        {
            IHXPresentationFeatureSink* pPFSink = (IHXPresentationFeatureSink*)(*i);
            HX_RELEASE(pPFSink);
        }
        HX_DELETE(m_pPFSSinkList);
    }
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.

#if defined(HELIX_FEATURE_PLAYBACK_MODIFIER)
    HX_RELEASE(m_pPlaybackModifiers);
#endif

    HX_RELEASE(m_pParentPlayer);

#if defined(HELIX_FEATURE_PLAYBACK_NET)
    if (m_bNetInitialized)
    {
        m_bNetInitialized = FALSE;
#if defined( _WIN32 ) || defined( _WINDOWS )
        win_net::ReleaseWinsockUsage(this);
#endif /* defined( _WIN32 ) || defined( _WINDOWS ) */
    }
#endif /* HELIX_FEATURE_PLAYBACK_NET */

    HX_RELEASE(m_pPlugin2Handler);

#if defined(HELIX_FEATURE_RECORDCONTROL)
    UnloadRecordService();
#endif /* HELIX_FEATURE_RECORDCONTROL */

    ResetError();
}

void
HXPlayer::ProcessPendingAuthentication(void)
{
#if defined(HELIX_FEATURE_AUTHENTICATION)
    IHXAuthenticationManager2* pAuthenticationManagerClient2 = NULL;

    if (HXR_OK == m_pClient->QueryInterface(IID_IHXAuthenticationManager2,
                             (void**)&pAuthenticationManagerClient2))
    {
        HX_ASSERT(pAuthenticationManagerClient2);

        if (pAuthenticationManagerClient2)
        {
            pAuthenticationManagerClient2->HandleAuthenticationRequest2(this, m_pAutheticationValues);
        }

        HX_RELEASE(pAuthenticationManagerClient2);
        return;
    }

    // otherwise continue with the old-fashioned authentication manager

    IHXAuthenticationManager* pAuthenticationManagerClient = NULL;

    m_pClient->QueryInterface(IID_IHXAuthenticationManager,
                             (void**)&pAuthenticationManagerClient);

    HX_ASSERT(pAuthenticationManagerClient);
    if (pAuthenticationManagerClient)
    {
        pAuthenticationManagerClient->HandleAuthenticationRequest(this);
    }

    HX_RELEASE(pAuthenticationManagerClient);
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

void
HXPlayer::ClearPendingAuthenticationRequests(void)
{
#if defined(HELIX_FEATURE_AUTHENTICATION)
    m_pAuthenticationRequestsPending->ClearPendingList();
    RemovePendingCallback(m_pAuthenticationCallback);

    /* Remove all pending authentication requests TBD */
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

STDMETHODIMP
HXPlayer::HandleAuthenticationRequest(
    IHXAuthenticationManagerResponse* pResponse)
{
#if defined(HELIX_FEATURE_AUTHENTICATION)
    return HandleAuthenticationRequest2( pResponse, NULL );
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

STDMETHODIMP
HXPlayer::HandleAuthenticationRequest2(
    IHXAuthenticationManagerResponse* pResponse, IHXValues* pValues)
{
#if defined(HELIX_FEATURE_AUTHENTICATION)
    IHXValues*                  pHeader = NULL;
    IHXBuffer*                  pUserName = NULL;
    IHXBuffer*                  pPassword = NULL;
    ULONG32                     ulAuthenticationAttempts = 0;

    HX_RELEASE(m_pAutheticationValues);
    m_pAutheticationValues = pValues;
    if (m_pAutheticationValues)
    {
        m_pAutheticationValues->AddRef();
    }

    /* Username/Password in the url will not work for tracks within
     * SMIL/RAM files.
     * This is because the m_pURL points to the last opened track
     * in the presentation and may not point to the URL for which
     * the authentication has been requested for.
     * This is tricky to fix --- post B2 - XXXRA
     */
    // Get info about the URL
    if (m_pURL)
    {
        pHeader = m_pURL->GetProperties();
    }
    if (pHeader)
    {
        // Try to get the username and password from the URL
        pHeader->GetPropertyBuffer(PROPERTY_USERNAME, pUserName);
        pHeader->GetPropertyBuffer(PROPERTY_PASSWORD, pPassword);

        // Try to get the number of times we've tried to authenticate
        // already based on this name and password
        HX_RESULT res = pHeader->GetPropertyULONG32("AUTHENTICATION_ATTEMPTS", ulAuthenticationAttempts);
    }

    // First check to see if a username and/or password were supplied in the URL.  If they were there is no need to pass
    // this on to the client to display any UI.
    // (And in case the wrong name/password were supplied, don't let them try too many times.)
    if (pUserName && pPassword && (ulAuthenticationAttempts < 3))
    {
        if (pHeader)
        {
            pHeader->SetPropertyULONG32("AUTHENTICATION_ATTEMPTS", ulAuthenticationAttempts + 1);
        }

        pResponse->AuthenticationRequestDone(HXR_OK, (const char*)pUserName->GetBuffer(), (const char*)pPassword->GetBuffer());
    }
    else
    {
#ifndef _WIN16
        m_pAuthenticationRequestsPending->Add(this, pResponse, pValues);
#else
        IHXAuthenticationManager2* pAuthenticationManagerClient2 = NULL;

        if (HXR_OK == m_pClient->QueryInterface(IID_IHXAuthenticationManager2, (void**)&pAuthenticationManagerClient2))
        {
            pAuthenticationManagerClient2->HandleAuthenticationRequest2
                (
                    pResponse, pValues
                );

            HX_RELEASE(pAuthenticationManagerClient2);
        }

        else

        {
            // if the new authentication manager isn't available, use the old authentication manager.
            IHXAuthenticationManager* pAuthenticationManagerClient = NULL;

            m_pClient->QueryInterface
                (
                    IID_IHXAuthenticationManager,
                    (void**)&pAuthenticationManagerClient
                );

            pAuthenticationManagerClient->HandleAuthenticationRequest
                (
                    pResponse
                );

            HX_RELEASE(pAuthenticationManagerClient);

        }

#endif /* _WIN16 */
    }


    HX_RELEASE(pUserName);
    HX_RELEASE(pPassword);
    HX_RELEASE(pHeader);
#endif /* HELIX_FEATURE_AUTHENTICATION */

    return HXR_OK;
}

// IHXAuthenticationManagerResponse
STDMETHODIMP
HXPlayer::AuthenticationRequestDone
(
    HX_RESULT result,
    const char* user,
    const char* password
)
{
#if !defined(_WIN16) && defined(HELIX_FEATURE_AUTHENTICATION)
    return m_pAuthenticationRequestsPending->SatisfyPending
    (
        result,
        user,
        password
    );
#else
    return HXR_NOTIMPL;
#endif /* _WIN16 */
}

void
HXPlayer::SetClientState(EHXClientState eState)
{
    if (eState == m_eClientState)
    {
    return;
    }

    // It is possible for engine to never enter prefetching state due to
    // buffering being immediately completed.  To keep state machine transitions
    // consistent and simple, we report prefetching state prior to transitioning 
    // to prefetched state.
    if ((eState == HX_CLIENT_STATE_PREFETCHED) && 
    (m_eClientState != HX_CLIENT_STATE_PREFETCHING))
    {
    SetClientState(HX_CLIENT_STATE_PREFETCHING);
    }
        
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::SetClientState() %d -> %d", this, (UINT16)m_eClientState, (UINT16)eState);

    if (m_bHaltInConnected && eState == HX_CLIENT_STATE_CONNECTED)
    {
        HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::SetClientState() HALTING from %d", this, (UINT16) m_eClientState);
    m_eClientStateStatus = HX_CLIENT_STATE_STATUS_HALTED;
    }

    if (m_pClientStateAdviseSink)
    {
    m_pClientStateAdviseSink->OnStateChange(m_eClientState, eState);
    }
    m_eClientState = eState;
}

/************************************************************************
*   Method:
*       IHXClientState::SetConfig
*   Purpose:
*       Specify whether or not to halt in the connected state
*
*/
STDMETHODIMP
HXPlayer::SetConfig(IHXValues* pValues)
{
    if (pValues)
    {
    ULONG32 ulValue;
    if (pValues->GetPropertyULONG32("HaltInConnected", ulValue) == HXR_OK)
    {
        m_bHaltInConnected = (ulValue != 0);
        HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::SetConfig() HaltInConnected: %s", this, m_bHaltInConnected ? "TRUE" : "FALSE");
    }
    else if (pValues->GetPropertyULONG32("RestartToPrefetched", ulValue) == HXR_OK)
    {
        m_bRestartToPrefetched = (ulValue != 0);
        HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::SetConfig() RestartToPrefetched: %s", this, m_bRestartToPrefetched ? "TRUE" : "FALSE");
    }
    }

    return HXR_OK;
}

/************************************************************************
*   Method:
*       IHXClientState::Resume
*   Purpose:
*       Resume player state transition, prefetch
*
*/
STDMETHODIMP
HXPlayer::Resume()
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::Resume() m_eClientStateStatus : %d", this, m_eClientStateStatus);
    m_eClientStateStatus = HX_CLIENT_STATE_STATUS_ACTIVE;
    return HXR_OK;
}

/************************************************************************
*   Method:
*       IHXClientState::GetState
*   Purpose:
*       Get the client state
*
*/
STDMETHODIMP_(UINT16)
HXPlayer::GetState()
{
    return (UINT16) m_eClientState;
}

/*
 *      IHXViewSourceCommand::CanViewSouce
 */
STDMETHODIMP_(HXBOOL)
HXPlayer::CanViewSource(IHXStreamSource* pStream)
{
#if defined(HELIX_FEATURE_VIEWSOURCE)
    HXBOOL bRet = TRUE;
    HX_RESULT ret = HXR_OK;
    if ( m_pClientViewSource == NULL && m_pEngine )
    {
        m_pEngine->QueryInterface(IID_IHXClientViewSource,
            (void**)&m_pClientViewSource);
    }
    if ( m_pClientViewSource )
    {
        if ( pStream )
        {
            bRet = m_pClientViewSource->CanViewSource(pStream);
        }
        else
        {
            IHXStreamSource* pStrmSource = NULL;
            if ( GetViewSourceStream(pStrmSource) )
            {
                bRet = m_pClientViewSource->CanViewSource(pStrmSource);
            }
            else
            {
                bRet = FALSE;
            }
            HX_RELEASE(pStrmSource);
        }
    }
    else
    {
        bRet = FALSE;
    }
    return bRet;
#else
    return FALSE;
#endif
}

/*
 *      IHXViewSourceCommand::DoViewSouce
 */
STDMETHODIMP
HXPlayer::DoViewSource(IHXStreamSource* pStream)
{
#if defined(HELIX_FEATURE_VIEWSOURCE)
    HX_RESULT ret = HXR_OK;
    if ( m_pClientViewSource == NULL && m_pEngine )
    {
        m_pEngine->QueryInterface(IID_IHXClientViewSource,
            (void**)&m_pClientViewSource);
    }
    if ( m_pClientViewSource )
    {
        if ( pStream )
        {
            ret = m_pClientViewSource->DoViewSource((IUnknown*)(IHXPlayer*)this, pStream);
        }
        else
        {
            IHXStreamSource* pStrmSource = NULL;
            if ( GetViewSourceStream(pStrmSource) )
            {
                ret = m_pClientViewSource->DoViewSource((IUnknown*)(IHXPlayer*)this,
                    pStrmSource);
            }
            else
            {
                // pass null for the stream source -
                // will cause a no source error to be shown.
                ret = m_pClientViewSource->DoViewSource(
                    (IUnknown*)(IHXPlayer*)this, NULL);
            }

            HX_RELEASE(pStrmSource);
        }
    }
    else
    {
        ret = HXR_NOT_INITIALIZED;
    }
    return ret;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

/*
 *      IHXViewSourceCommand::GetViewSourceURL
 */
STDMETHODIMP
HXPlayer::GetViewSourceURL(IHXStreamSource* pStream, IHXViewSourceURLResponse* pResp)
{
#if defined(HELIX_FEATURE_VIEWSOURCE)
    HX_RESULT ret = HXR_OK;
    if ( m_pClientViewSource == NULL && m_pEngine )
    {
        m_pEngine->QueryInterface(IID_IHXClientViewSource,
            (void**)&m_pClientViewSource);
    }
    if ( m_pClientViewSource )
    {
        if ( pStream )
        {
            ret = m_pClientViewSource->GetViewSourceURL(
                (IUnknown*)(IHXPlayer*)this, pStream, pResp);
        }
        else
        {
            IHXStreamSource* pStrmSource = NULL;
            if ( GetViewSourceStream(pStrmSource) )
            {
                ret = m_pClientViewSource->GetViewSourceURL(
                    (IUnknown*)(IHXPlayer*)this, pStrmSource, pResp);
            }
            else
            {
                ret = HXR_FAIL;
            }
            HX_RELEASE(pStrmSource);
        }
    }
    else
    {
        ret = HXR_NOT_INITIALIZED;
    }
    return ret;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_VIEWSOURCE */
}

HXBOOL
HXPlayer::GetViewSourceStream(REF(IHXStreamSource*) pStrmSource)
{
    HXBOOL bRet = FALSE;

#if defined(HELIX_FEATURE_VIEWSOURCE)
#if defined(HELIX_FEATURE_NESTEDMETA)
        HXPersistentComponent* pPersistentComponent = m_pPersistentComponentManager->m_pRootPersistentComponent;

        // not a persistent playback
        if (!pPersistentComponent)
        {
            HX_ASSERT(m_pSourceMap->GetCount() <= 1);
            if ( m_pSourceMap->GetCount() > 0 )
            {
                CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
                SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
                HX_ASSERT(pSourceInfo);

                HXSource* pSource = pSourceInfo->m_pSource;
                if( pSource )
                {
                    HX_RELEASE(pStrmSource);
                    if ( SUCCEEDED(pSource->QueryInterface(IID_IHXStreamSource,
                        (void**)&pStrmSource)) )
                    {
                        bRet = TRUE;
                    }
                }
            }
        }
        else
        {
            HXSource* pSource = pPersistentComponent->m_pSourceInfo->m_pSource;
            if ( pSource )
            {
                HX_RELEASE(pStrmSource);
                if ( SUCCEEDED(pSource->QueryInterface(IID_IHXStreamSource,
                    (void**)&pStrmSource)) )
                {
                    bRet = TRUE;
                }
            }
        }
#else
        HX_ASSERT(m_pSourceMap->GetCount() <= 1);
        if ( m_pSourceMap->GetCount() > 0 )
        {
            CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
            SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
            HX_ASSERT(pSourceInfo);

            HXSource* pSource = pSourceInfo->m_pSource;
            if( pSource )
            {
                HX_RELEASE(pStrmSource);
                if ( SUCCEEDED(pSource->QueryInterface(IID_IHXStreamSource,
                    (void**)&pStrmSource)) )
                {
                    bRet = TRUE;
                }
            }
        }
#endif /* HELIX_FEATURE_NESTEDMETA */
#endif /* HELIX_FEATURE_VIEWSOURCE */

    return bRet;
}

HX_RESULT
HXPlayer::GetSourceInfo(UINT16 uGroupIndex, UINT16 uTrackIndex, SourceInfo*& pSourceInfo)
{
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    HX_RESULT   hr = HXR_OK;
    SourceInfo* pTempSourceInfo = NULL;
    CHXMapPtrToPtr::Iterator ndxSource;

    pSourceInfo = NULL;

    if (uGroupIndex != m_nCurrentGroup)
    {
        hr = HXR_UNEXPECTED;
        goto cleanup;
    }

    // find the sourceinfo
    ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        pTempSourceInfo = (SourceInfo*)(*ndxSource);

        if (pTempSourceInfo->m_uTrackID == uTrackIndex)
        {
            pSourceInfo = pTempSourceInfo;
            break;
        }
    }

    if (!pSourceInfo)
    {
        hr = HXR_FAILED;
    }

cleanup:

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

HX_RESULT
HXPlayer::CopyRegInfo(UINT32 ulFromRegID, UINT32 ulToRegID)
{
#if defined(HELIX_FEATURE_REGISTRY)
    HX_RESULT       hr = HXR_OK;
    const char*     pPropName = NULL;
    ULONG32          ulPropId = 0;
    UINT32          ulRegId = 0;
    IHXBuffer*      pFromRegName = NULL;
    IHXBuffer*      pToRegName = NULL;
    IHXValues*      pValues = NULL;
    char            szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

    m_pRegistry->GetPropName(ulFromRegID, pFromRegName);
    m_pRegistry->GetPropName(ulToRegID, pToRegName);

    // Get the IHXValues under this id.
    m_pRegistry->GetPropListById(ulFromRegID, pValues);

    // PT_COMPOSITE without child
    if (!pValues)
    {
        HX_RELEASE(pFromRegName);
        HX_RELEASE(pToRegName);
        return HXR_OK;
    }

    // iterate through the child list
    hr = pValues->GetFirstPropertyULONG32(pPropName, ulPropId);
    while (hr == HXR_OK)
    {
        HXPropType type = m_pRegistry->GetTypeById(ulPropId);

        SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.%s", pToRegName->GetBuffer(),
                pPropName + pFromRegName->GetSize());

        if (type == PT_COMPOSITE)
        {
            ulRegId = m_pRegistry->AddComp(szRegName);
            CopyRegInfo(ulPropId, ulRegId);
        }
        else
        {
            switch(type)
            {
                case PT_INTEGER:
                {
                    INT32 val;
                    if(HXR_OK == m_pRegistry->GetIntById(ulPropId, val))
                    {
                        m_pRegistry->AddInt(szRegName, val);
                    }
                    break;
                }
                case PT_STRING:
                {
                    IHXBuffer* pBuffer = NULL;
                    if(HXR_OK == m_pRegistry->GetStrById(ulPropId,
                                                       pBuffer))
                    {
                        m_pRegistry->AddStr(szRegName, pBuffer);
                    }
                    HX_RELEASE(pBuffer);
                    break;
                }
                case PT_BUFFER:
                {
                    IHXBuffer* pBuffer = NULL;
                    if(HXR_OK == m_pRegistry->GetBufById(ulPropId,
                                                       pBuffer))
                    {
                        m_pRegistry->AddBuf(szRegName, pBuffer);
                    }
                    HX_RELEASE(pBuffer);
                    break;
                }
                default:
                    break;
            }
        }

        hr = pValues->GetNextPropertyULONG32(pPropName, ulPropId);
    }
    HX_RELEASE(pFromRegName);
    HX_RELEASE(pToRegName);
    HX_RELEASE(pValues);
#endif /* HELIX_FEATURE_REGISTRY */

    return HXR_OK;
}

HX_RESULT
HXPlayer::UpdateTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT   hr = HXR_OK;
    UINT16      uNewTrackIndex = 0;
    UINT32      ulTempNewTrackIndex = 0;
    UINT32      ulParentRegId = 0;
    SourceInfo* pSourceInfo = NULL;
#if defined (HELIX_FEATURE_NEXTGROUPMGR)
    UINT16      uCurrentGroup = 0;
    IHXGroup*  pGroup = NULL;
#endif

    if (HXR_OK == pValues->GetPropertyULONG32("TrackIndex", ulTempNewTrackIndex))
    {
        // /Fixes PR 121880 on Mac and other Big-endian machines, upcasting of UINT16
        // to UINT32 in GetProp...() causes the wrong 2 bytes of the 4-byte prop to get
        // used, so we pass in a temp UINT32 and copy from that:
        uNewTrackIndex = (UINT16)ulTempNewTrackIndex; 
        if (uGroupIndex == m_nCurrentGroup &&
            HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
        {
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
            ulParentRegId = m_pStats->m_ulRegistryID;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
        }
#if defined (HELIX_FEATURE_NEXTGROUPMGR)
        else if (m_bNextGroupStarted                                                &&
                 m_pNextGroupManager->GetCurrentGroup(uCurrentGroup, pGroup) == HXR_OK  &&
                 uCurrentGroup == uGroupIndex)
        {
            m_pNextGroupManager->GetSource(uTrackIndex, pSourceInfo);
            ulParentRegId = m_ulNextGroupRegistryID;
        }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

        if (pSourceInfo)
        {
            hr = UpdateSourceInfo(pSourceInfo,
                                  ulParentRegId,
                                  uNewTrackIndex);
        }
    }

#if defined(HELIX_FEATURE_NESTEDMETA)
    m_pPersistentComponentManager->TrackUpdated(uGroupIndex, uTrackIndex, pValues);
#endif /* HELIX_FEATURE_NESTEDMETA */

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
HXPlayer::RemoveTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT   hr = HXR_OK;
    SourceInfo* pSourceInfo = NULL;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::RemoveTrack(GroupID=%hd TrackID=%hd): Start", 
        this, 
        uGroupIndex, 
        uTrackIndex);

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    UINT16      uCurrentGroup = 0;
    IHXGroup*  pGroup = NULL;
#endif
    // track removed from the current group
    if (uGroupIndex == m_nCurrentGroup &&
        HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
    {
        // remove source from the current group
        m_pSourceMap->RemoveKey(pSourceInfo->m_pSource);

        pSourceInfo->Remove();
        HX_DELETE(pSourceInfo);

        AdjustPresentationTime();

        m_bSourceMapUpdated = TRUE;
        m_bForceStatsUpdate = TRUE;
    }
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    // track removed from the next group being prefetched
    else if (m_bNextGroupStarted                                                    &&
             m_pNextGroupManager->GetCurrentGroup(uCurrentGroup, pGroup) == HXR_OK  &&
             uCurrentGroup == uGroupIndex)
    {
        if (HXR_OK == m_pNextGroupManager->GetSource(uTrackIndex, pSourceInfo))
        {
            m_pNextGroupManager->RemoveSource(pSourceInfo);

            pSourceInfo->Remove();
            HX_DELETE(pSourceInfo);
        }
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::RemoveTrack(GroupID=%hd TrackID=%hd): End", 
        this, 
        uGroupIndex, 
        uTrackIndex);

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
HXPlayer::AddPrefetchTrack(UINT16 uGroupIndex,
                            UINT16 uPrefetchTrackIndex,
                            IHXValues* pTrack)
{
#if defined(HELIX_FEATURE_PREFETCH)
    HX_RESULT   theErr = HXR_OK;
    char        szDelay[] = "Delay";
    UINT32      ulDelay = 0;
    IHXGroup*   pGroup = NULL;

    if (m_pGroupManager)
    {
        theErr = m_pGroupManager->GetGroup(uGroupIndex, pGroup);
    }

    /* Check if a track is added to the group currently played */
    if (uGroupIndex == m_nCurrentGroup && pGroup == m_pCurrentGroup)
    {
        /* If we are not yet initialized, add tracks to the current source
         * map only if the track is within the fudge factor.
         *
         * This is to fix a bug in SMIL file with multiple sources in seq
         * with outer par.
         * The expected behavior is to start the first source
         * in seq and then initialize the remaining sources. Since we were
         * adding additional tracks to the source map, the player was not
         * getting initialized until ALL the sources in the seq have been
         * initialized. This resulted in massive startup delays.
         */
        if (!m_bInitialized)
        {
            if ((HXR_OK != pTrack->GetPropertyULONG32(szDelay,ulDelay)) ||
                (ulDelay <= m_ulCurrentPlayTime + MIN_DELAYBEFORE_START))
            {
                theErr = OpenTrack(pTrack, uGroupIndex, uPrefetchTrackIndex);
            }
            else
            {
                if (!m_pPendingTrackList)
                {
                    m_pPendingTrackList = new CHXSimpleList;
                }

                PendingTrackInfo* pPendingTrackInfo =
                    new PendingTrackInfo(uGroupIndex, uPrefetchTrackIndex, pTrack);

                m_pPendingTrackList->AddTail(pPendingTrackInfo);
            }
        }
        else
        {
            theErr = OpenTrack(pTrack, uGroupIndex, uPrefetchTrackIndex);
        }

        if (theErr)
        {
            ReportError(NULL, theErr, NULL);
        }
    }
    // we don't support prefetch for the next group
    else
    {
        HX_ASSERT(FALSE);
    }

    HX_RELEASE(pGroup);

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_PREFETCH */
}

HX_RESULT
HXPlayer::UpdatePrefetchTrack(UINT16 uGroupIndex,
                               UINT16 uPrefetchTrackIndex,
                               IHXValues* pValues)
{
#if defined(HELIX_FEATURE_PREFETCH)
    HX_RESULT   hr = HXR_OK;
    UINT16      uCurrentGroup = 0;
    UINT16      uNewTrackIndex = 0;
    UINT32      ulTempNewTrackIndex = 0;

    SourceInfo* pSourceInfo = NULL;
    IHXGroup*  pGroup = NULL;

    if (!m_pPrefetchManager)
    {
        hr = HXR_FAILED;
        goto cleanup;
    }

    if (HXR_OK == pValues->GetPropertyULONG32("TrackIndex", ulTempNewTrackIndex))
    {
        // /On Mac and other Big-endian machines, upcasting of UINT16 to UINT32& in
        // GetProp...() causes the wrong 2 bytes of the 4-byte prop to get used, so
        // we pass in a temp UINT32 and copy from that:
        uNewTrackIndex = (UINT16)ulTempNewTrackIndex; 
        if (uGroupIndex == m_nCurrentGroup &&
            HXR_OK == m_pPrefetchManager->GetSource(uNewTrackIndex, pSourceInfo))
        {
            HX_ASSERT(pSourceInfo->m_uTrackID == uNewTrackIndex);
        }
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
        else if (m_bNextGroupStarted                                                &&
                 m_pNextGroupManager->GetCurrentGroup(uCurrentGroup, pGroup) == HXR_OK  &&
                 uCurrentGroup == uGroupIndex)
        {
            hr = HXR_NOTIMPL;
        }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
    }

cleanup:

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_PREFETCH */
}

HX_RESULT
HXPlayer::RemovePrefetchTrack(UINT16 uGroupIndex,
                               UINT16 uPrefetchTrackIndex,
                               IHXValues* pValues)
{
#if defined(HELIX_FEATURE_PREFETCH)
    HX_RESULT   theErr = HXR_OK;
    SourceInfo* pSourceInfo = NULL;

    if (m_pPrefetchManager &&
        m_pPrefetchManager->Lookup(pValues, pSourceInfo))
    {
        theErr = m_pPrefetchManager->RemoveSource(pSourceInfo);

        // cleanup if the prefetch track has not been activated
        if (pSourceInfo->m_pSource->IsPartOfPrefetchGroup())
        {
            pSourceInfo->Remove();
            HX_DELETE(pSourceInfo);
        }
    }

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_PREFETCH */
}

HX_RESULT
HXPlayer::PrefetchTrackDone(UINT16 uGroupIndex, UINT16 uPrefetchTrackIndex, HX_RESULT status)
{
#if defined(HELIX_FEATURE_PREFETCH)
    IHXGroup*           pGroup = NULL;
    IHXPrefetchSink*   pPrefetchSink = NULL;

    if (m_pGroupManager && HXR_OK == m_pGroupManager->GetGroup(uGroupIndex, pGroup))
    {
        if (HXR_OK == pGroup->QueryInterface(IID_IHXPrefetchSink, (void**)&pPrefetchSink))
        {
            pPrefetchSink->PrefetchTrackDone(uGroupIndex, uPrefetchTrackIndex, status);
        }
        HX_RELEASE(pPrefetchSink);
    }
    HX_RELEASE(pGroup);
#endif /* HELIX_FEATURE_PREFETCH */

    return HXR_OK;
}

HX_RESULT
HXPlayer::BeginTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT   hr = HXR_OK;
    SourceInfo* pSourceInfo = NULL;

    // begin on the current group only
    if (uGroupIndex == m_nCurrentGroup &&
        HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
    {
        hr = pSourceInfo->BeginTrack();
    }
    else
    {
        hr = HXR_UNEXPECTED;
    }

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
HXPlayer::PauseTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT   hr = HXR_OK;
    SourceInfo* pSourceInfo = NULL;

    // pause on the current group only
    if (uGroupIndex == m_nCurrentGroup &&
        HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
    {
        hr = pSourceInfo->PauseTrack();
    }
    else
    {
        hr = HXR_UNEXPECTED;
    }

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
HXPlayer::SeekTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues, UINT32 ulSeekTime)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT   hr = HXR_OK;
    SourceInfo* pSourceInfo = NULL;

    // seek on the current group only
    if (uGroupIndex == m_nCurrentGroup &&
        HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
    {
        hr = pSourceInfo->SeekTrack(ulSeekTime);
    }
    else
    {
        hr = HXR_UNEXPECTED;
    }

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
HXPlayer::StopTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues)
{
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HX_RESULT   hr = HXR_OK;
    SourceInfo* pSourceInfo = NULL;

    // stop on the current group only
    if (uGroupIndex == m_nCurrentGroup &&
        HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
    {
        hr = pSourceInfo->StopTrack();
    }
    else
    {
        hr = HXR_UNEXPECTED;
    }

    return hr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */
}

HX_RESULT
HXPlayer::SetSoundLevel(UINT16 uGroupIndex, UINT16 uTrackIndex, UINT16 uSoundLevel, HXBOOL bReflushAudioDevice)
{
    HX_RESULT   hr = HXR_OK;
    SourceInfo* pSourceInfo = NULL;

    // set on the current group only
    if (uGroupIndex == m_nCurrentGroup &&
        HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
    {
        hr = pSourceInfo->SetSoundLevel(uSoundLevel, bReflushAudioDevice);
    }
    else
    {
        hr = HXR_UNEXPECTED;
    }

    return hr;
}
    
void
HXPlayer::CheckSourceRegistration(void)
{
    HXBOOL bAtLeastOneRegister = FALSE;

    CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
    /* Check if we are done. This may be TRUE for empty files */
    for (; ndxSources != m_pSourceMap->End(); ++ndxSources)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
        if (pSourceInfo->m_pSource && pSourceInfo->m_pSource->CanBeResumed())
        {
            pSourceInfo->Register();
            bAtLeastOneRegister = TRUE;
        }
    }

    if (bAtLeastOneRegister)
    {
        RegisterSourcesDone();
    }
}

void
HXPlayer::GetTimingFromURL(CHXURL* pURL, UINT32& ulStart, 
                           UINT32& ulEnd, UINT32& ulDelay, UINT32& ulDuration)
{
    IHXValues* pURLOptions = NULL;

    ulStart = 0;
    ulEnd = HX_EOF_TIME;
    ulDelay = 0;
    ulDuration = 0;

    if (pURL)
    {
        pURLOptions = pURL->GetOptions();
        if (pURLOptions)
        {
            pURLOptions->GetPropertyULONG32("Start", ulStart);
            pURLOptions->GetPropertyULONG32("End", ulEnd);
            pURLOptions->GetPropertyULONG32("Delay", ulDelay);
            pURLOptions->GetPropertyULONG32("Duration", ulDuration);
        }
        HX_RELEASE(pURLOptions);
    }
}

HXBOOL HXPlayer::CanFileFormatClaimSchemeExtensionPair(CHXURL* pURL, 
                               REF(IHXPluginSearchEnumerator*) rpEnum,
                               HXBOOL bMustClaimExtension)
{
    HXBOOL bRet = FALSE;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CanFileFormatClaimSchemeExtensionPair(URL=%s, bMustClaimExtension=%c) Start", 
        this, 
        pURL ? pURL->GetURL() : "NULL",
        bMustClaimExtension ? 'T' :'F');

    HX_ASSERT(rpEnum == NULL);

    if (pURL && m_pPlugin2Handler)
    {
        // Get the properties
        IHXValues* pProps = pURL->GetProperties();
        if (pProps)
        {
            // Get the scheme
            IHXBuffer* pSchemeStr = NULL;
            HX_RESULT retVal = pProps->GetPropertyBuffer(PROPERTY_SCHEME, pSchemeStr);
            if (SUCCEEDED(retVal))
            {
                // Initialize the extension string and the search string
                CHXString cExtension;
                CHXString cSearch;
                // Get the full path
                IHXBuffer* pFullPathStr = NULL;
                retVal = pProps->GetPropertyBuffer(PROPERTY_FULLPATH, pFullPathStr);
                if (SUCCEEDED(retVal))
                {
                    // Put this into a CHXString
                    CHXString cFullPath((const char*) pFullPathStr->GetBuffer());
                    CHXString cFileName = cFullPath;
                    // Find the last "/" or "\"
                    INT32 lLastSlash1 = cFullPath.ReverseFind('/');
                    INT32 lLastSlash2 = cFullPath.ReverseFind('\\');
                    INT32 lLastSlash  = HX_MAX(lLastSlash1, lLastSlash2);
                    if (lLastSlash >= 0)
                    {
                        cFileName = cFullPath.Right(cFullPath.GetLength() - lLastSlash - 1);
                    }
                    // Get the last '.'
                    INT32 lPeriod = cFileName.ReverseFind('.');
                    if (lPeriod >= 0)
                    {
                        cExtension = cFileName.Right(cFileName.GetLength() - lPeriod - 1);
                    }
                    // Construct the search string
                    cSearch = (const char*) pSchemeStr->GetBuffer();
                    // Do we have an extension?
                    if (cExtension.GetLength() > 0)
                    {
                        // Append the separator
                        cSearch += ":";
                        // Append the extension
                        cSearch += cExtension;
                    }
                }
                else
                {
                    // Some URLs (like capture://audio) do not have a path 
                    // at all, so they will fail the test where we look 
                    // for the PROPERTY_FULLPATH in the CHXURL properties.
                    // So in this case, the search string is just the scheme
                    cSearch = (const char*) pSchemeStr->GetBuffer();
                    // Clear the return value
                    retVal = HXR_OK;
                }
                // We either need to have an extension or the input flag needs
                // to be set saying we don't have to claim the extension.
            if (!bMustClaimExtension || (cExtension.GetLength() > 0))
            {
            // Get the IHXPluginHandler3 interface
            IHXPluginHandler3* pHandler3 = NULL;
            retVal = m_pPlugin2Handler->QueryInterface(IID_IHXPluginHandler3, (void**) &pHandler3);
            if (SUCCEEDED(retVal))
            {
                // Find out if we have any fileformat plugins which
                // have a scheme extension pair equal to cSearch
                IHXPluginSearchEnumerator* pEnum = NULL;
                retVal = pHandler3->FindGroupOfPluginsUsingStrings(PLUGIN_CLASS,
                                           PLUGIN_FILEFORMAT_TYPE,
                                           PLUGIN_SCHEME_EXTENSION,
                                           (char*) (const char*) cSearch,
                                           NULL, NULL, pEnum);
                if ((FAILED(retVal) || (pEnum->GetNumPlugins() == 0)) &&
                (!bMustClaimExtension) &&
                (cExtension.GetLength() > 0))
                {
                // Try just the scheme as the search string. This is because
                // some plugins may claim a scheme for all file extensions
                cSearch = (const char*) pSchemeStr->GetBuffer();
                // Search for just the scheme
                HX_RELEASE(pEnum);
                retVal = pHandler3->FindGroupOfPluginsUsingStrings(PLUGIN_CLASS,
                                           PLUGIN_FILEFORMAT_TYPE,
                                           PLUGIN_SCHEME_EXTENSION,
                                           (char*) (const char*) cSearch,
                                           NULL, NULL, pEnum);
                }
                if (SUCCEEDED(retVal) && pEnum && pEnum->GetNumPlugins() > 0)
                {
                // We do have a file format that can claim this
                // combination of scheme and file extension
                bRet = TRUE;
                // Pass the enumerator as an out parameter
                rpEnum = pEnum;
                rpEnum->AddRef();
                }
                HX_RELEASE(pEnum);
            }
            HX_RELEASE(pHandler3);
            }
                HX_RELEASE(pFullPathStr);
            }
            HX_RELEASE(pSchemeStr);
        }
        HX_RELEASE(pProps);
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CanFileFormatClaimSchemeExtensionPair(URL=%s, bMustClaimExtension=%c) End=%c", 
        this, 
        pURL ? pURL->GetURL() : "NULL",
        bMustClaimExtension ? 'T' :'F',
        bRet ? 'T' : 'F');

    return bRet;
}

/************************************************************************
 *  Method:
 *      IHXOverrideDefaultServices::OverrideServices
 *  Purpose:
 *      Override default services provided by the G2 system.
 *
 */
STDMETHODIMP
HXPlayer::OverrideServices(IUnknown* pContext)
{
    if (!pContext)
    {
        return HXR_UNEXPECTED;
    }

#if defined(HELIX_FEATURE_PREFERENCES)
    /* override IHXPreferences */
    IHXPreferences* pPreferences = NULL;
    if (pContext->QueryInterface(IID_IHXPreferences, (void**) &pPreferences)
            == HXR_OK)
    {
        HX_RELEASE(m_pPreferences);
        m_pPreferences = pPreferences;
    }
#endif /* HELIX_FEATURE_PREFERENCES */

    /* override IHXPlugin2Handler */
    IHXPlugin2Handler* pPlugin2Handler = NULL;
    if (pContext->QueryInterface(IID_IHXPlugin2Handler, (void**) &pPlugin2Handler)
            == HXR_OK)
    {
        HX_RELEASE(m_pPlugin2Handler);
        m_pPlugin2Handler = pPlugin2Handler;
    }

    return HXR_OK;
}

/*
 * IHXPlayerNavigator methods
 */

/************************************************************************
 *      Method:
 *          IHXPlayerNavigator::AddChildPlayer
 *      Purpose:
 *          Add child player to the current player
 */
STDMETHODIMP
HXPlayer::AddChildPlayer(IHXPlayer* pPlayer)
{
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    if (!m_pChildPlayerList)
    {
        m_pChildPlayerList = new CHXSimpleList();
    }

    if (m_pChildPlayerList &&
        !m_pChildPlayerList->Find(pPlayer))
    {
        pPlayer->AddRef();
        m_pChildPlayerList->AddTail(pPlayer);
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

/************************************************************************
 *      Method:
 *          IHXPlayerNavigator::RemoveChildPlayer
 *      Purpose:
 *          Remove child player from the current player
 */
STDMETHODIMP
HXPlayer::RemoveChildPlayer(IHXPlayer* pPlayer)
{
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    if (m_pChildPlayerList)
    {
        LISTPOSITION lPosition = m_pChildPlayerList->Find(pPlayer);

        if (lPosition)
        {
            m_pChildPlayerList->RemoveAt(lPosition);
            HX_RELEASE(pPlayer);
        }
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

/************************************************************************
 *      Method:
 *          IHXPlayerNavigator::GetNumChildPlayer
 *      Purpose:
 *          Get number of the child players
 */
STDMETHODIMP_(UINT16)
HXPlayer::GetNumChildPlayer()
{
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    return m_pChildPlayerList ? m_pChildPlayerList->GetCount() : 0;
#else
    return 0;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

/************************************************************************
 *      Method:
 *          IHXPlayerNavigator::GetChildPlayer
 *      Purpose:
 *          Get Nth child player
 */
STDMETHODIMP
HXPlayer::GetChildPlayer(UINT16 uPlayerIndex,
                          REF(IHXPlayer*) pPlayer)
{
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    HX_RESULT       rc = HXR_OK;
    LISTPOSITION    lPosition;

    pPlayer = NULL;

    if (!m_pChildPlayerList)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    lPosition = m_pChildPlayerList->FindIndex(uPlayerIndex);

    if (!lPosition)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    pPlayer = (IHXPlayer*)m_pChildPlayerList->GetAt(lPosition);
    pPlayer->AddRef();

cleanup:

    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

/************************************************************************
 *      Method:
 *          IHXPlayerNavigator::SetParentPlayer
 *      Purpose:
 *          Set the parent player
 */
STDMETHODIMP
HXPlayer::SetParentPlayer(IHXPlayer* pPlayer)
{
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    HX_ASSERT(!m_pParentPlayer);
    HX_RELEASE(m_pParentPlayer);

    m_pParentPlayer = pPlayer;
    pPlayer->AddRef();

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

/************************************************************************
 *      Method:
 *          IHXPlayerNavigator::RemoveParentPlayer
 *      Purpose:
 *          Remove the parent player
 */
STDMETHODIMP
HXPlayer::RemoveParentPlayer(IHXPlayer* pPlayer)
{
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    HX_ASSERT(pPlayer == m_pParentPlayer);
    HX_RELEASE(m_pParentPlayer);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

/************************************************************************
 *      Method:
 *          IHXPlayerNavigator::GetParentPlayer
 *      Purpose:
 *          Get the parent player
 */
STDMETHODIMP
HXPlayer::GetParentPlayer(REF(IHXPlayer*) pPlayer)
{
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    pPlayer = NULL;

    if (m_pParentPlayer)
    {
        pPlayer = m_pParentPlayer;
        pPlayer->AddRef();

        return HXR_OK;
    }
    else
    {
        return HXR_FAILED;
    }
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
// UpdateStatsCallback
UpdateStatsCallback::UpdateStatsCallback() :
     m_lRefCount (0)
    ,m_pPlayer (0)
    ,m_PendingHandle (0)
    ,m_bIsCallbackPending(FALSE)
{
}

UpdateStatsCallback::~UpdateStatsCallback()
{
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP UpdateStatsCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) UpdateStatsCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) UpdateStatsCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *      IHXCallback methods
 */
STDMETHODIMP UpdateStatsCallback::Func(void)
{
    m_PendingHandle     = 0;
    m_bIsCallbackPending        = FALSE;

    if (m_pPlayer)
    {
        m_pPlayer->UpdateStatistics();
    }

    return HXR_OK;
}
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */


// HXPlayerCallback
HXPlayerCallback::HXPlayerCallback(void* pParam, fGenericCBFunc pFunc)
 :  CHXGenericCallback(pParam, pFunc)
 ,  m_bInterruptSafe(FALSE) 
 ,  m_bInterruptOnly(FALSE)
{
    ;
}
    
STDMETHODIMP HXPlayerCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXInterruptSafe))
    {
    if (m_bInterruptSafe)
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXInterruptSafe*) this;
        return HXR_OK;
    }
    else
    {
        ppvObj = NULL;
        return HXR_NOINTERFACE;
    }
    }
    else if (IsEqualIID(riid, IID_IHXInterruptOnly))
    {
    if (m_bInterruptOnly)
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXInterruptOnly*) this;
        return HXR_OK;
    }
    else
    {
        ppvObj = NULL;
        return HXR_NOINTERFACE;
    }
    }
    else
    {
        return CHXGenericCallback::QueryInterface(riid, ppvObj);
    }
}

STDMETHODIMP_(HXBOOL) HXPlayerCallback::IsInterruptSafe()
{
    HXPlayer* pPlayer = (HXPlayer*)m_pParam;

    return m_bInterruptSafe && 
           pPlayer->m_bInitialized &&
           !pPlayer->m_bIsDone;
}

STDMETHODIMP_(HXBOOL) HXPlayerCallback::IsInterruptOnly()
{
    return m_bInterruptOnly;
}


void HXPlayer::PlayerCallback(void* pParam)
{
    HXPlayer* pObj = (HXPlayer*)pParam;

    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::PlayerCallback()", pObj);

    if (pObj)
    {
    pObj->ProcessIdle(FALSE);
    }
}

void HXPlayer::PlayerCallbackInterruptSafe(void* pParam)
{
    HXPlayer* pObj = (HXPlayer*)pParam;

    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::PlayerCallbackInterruptSafe()", pObj);

    if (pObj)
    {
        pObj->ProcessIdle(TRUE);
    }
}

void HXPlayer::PlayerCallbackInterruptOnly(void* pParam)
{
    HXPlayer* pObj = (HXPlayer*)pParam;

    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::PlayerCallbackInterruptOnly()", pObj);

    if (pObj)
    {
        pObj->ProcessIdle(TRUE);
    }
}

#if defined(HELIX_FEATURE_AUTHENTICATION)
void HXPlayer::AuthenticationCallback(void* pParam)
{
    HXPlayer* pObj = (HXPlayer*)pParam;

    if (pObj)
    {
        pObj->ProcessPendingAuthentication();
    }
}
#endif //HELIX_FEATURE_AUTHENTICATION

/************************************************************************
 *  Method:
 *      HXPlayer::SetupRendererSite
 *
 *  NOTE: Notice that the props passed in are associated with the stream
 *  that the renderer is rendering, but we don't make renderers implement
 *  their own support IHXValues. That would be mean.
 */
void
HXPlayer::SetupRendererSite(IUnknown* pRenderer, IHXValues* pProps, HXBOOL bIsPersistent)
{
#if defined(HELIX_FEATURE_VIDEO)
    HX_ASSERT(pProps);
    HX_ASSERT(pRenderer);

    IHXSiteUserSupplier*   pSUS = NULL;
    IHXSiteUser*           pSU  = NULL;
    UINT32                  uReqestID;
    /*
     * If the renderer doesn't support IHXSiteUserSupplier then
     * it is not a display oriented renderer and we skip it to
     * go to the next one!
     */
    if (HXR_OK == pRenderer->QueryInterface(IID_IHXSiteUserSupplier,
                                            (void**)&pSUS))
    {
        /*
         * Ask the site manager if any sites are available
         * for the renderer by this playto/from info.
         */
        if (!m_pSiteManager->
                        IsSiteAvailableByPlayToFrom(pProps, /*bIsPersistent*/ FALSE))
        {
            if (m_pSiteSupplier)
            {
                /*
                 * Let the site supplier know that we are changing the layout.
                 */
                if (m_bBeginChangeLayoutTobeCalled)
                {
                    m_bBeginChangeLayoutTobeCalled      = FALSE;
                    m_pSiteSupplier->BeginChangeLayout();
                }


                // XXXRA Change to revert back to old focus behavior for
                // datatypes that rely on this behavior
                CheckIfRendererNeedFocus(pRenderer);

                /*
                 * Inform the TLC/SS of the new sites we need.
                 */
                uReqestID = (ULONG32)pSUS;
                m_pSiteSupplier->SitesNeeded(uReqestID,pProps);
                DisableScreenSaver();
                m_SiteRequestIDList.AddTail((void*)uReqestID);
            }
        }

        /*  JEB: if the hookup failed, do not release the site user
         *  Hookup will add it to a list and hook it up later
         *  when the site has really been added
         */

        if (m_pSiteManager->HookupByPlayToFrom(pSUS,pProps,/*bIsPersistent*/ FALSE))
        {
            pSUS->Release();
        }
    }
    // If the renderer doesn't support SiteUserSupplier, than see if
    // it supports single instance as a SiteUser...
    else if (HXR_OK == pRenderer->QueryInterface(IID_IHXSiteUser,
                            (void**)&pSU))
    {
        /*
         * Ask the site manager if any sites are available
         * for the renderer by this playto/from info.
         */
        if (!m_pSiteManager->
                        IsSiteAvailableByPlayToFrom(pProps, /*bIsPersistent*/ FALSE))
        {
            if (m_pSiteSupplier)
            {
                /*
                 * Let the site supplier know that we are changing the layout.
                 */
                if (m_bBeginChangeLayoutTobeCalled)
                {
                    m_bBeginChangeLayoutTobeCalled      = FALSE;
                    m_pSiteSupplier->BeginChangeLayout();
                }

                // XXXRA Change to revert back to old focus behavior for
                // datatypes that rely on this behavior
                CheckIfRendererNeedFocus(pRenderer);

                /*
                 * Inform the TLC/SS of the new sites we need.
                 */
                uReqestID = (ULONG32)pSU;
                m_pSiteSupplier->SitesNeeded(uReqestID,pProps);
                DisableScreenSaver();
                m_SiteRequestIDList.AddTail((void*)uReqestID);
            }
        }

        /*  JEB: if the hookup failed, do not release the site user
         *  Hookup will add it to a list and hook it up later
         *  when the site has really been added
         */

        if (m_pSiteManager->HookupSingleSiteByPlayToFrom(pSU,pProps, /*bIsPersistent*/ FALSE))
        {
            pSU->Release();
        }
    }
#endif /* HELIX_FEATURE_VIDEO */
}

/************************************************************************
 *  Method:
 *      HXPlayer::SetupLayoutSiteGroup
 */
void
HXPlayer::SetupLayoutSiteGroup(IUnknown* pLSG, HXBOOL bIsPersistent)
{
#if defined(HELIX_FEATURE_VIDEO)
    HX_ASSERT(pLSG);

    IHXSiteUserSupplier*   pSUS = NULL;
    IHXSiteUser*           pSU = NULL;

    if (HXR_OK == pLSG->QueryInterface(IID_IHXSiteUserSupplier,
                            (void**)&pSUS))
    {
        IHXValues* pSUSProps = NULL;

        HXBOOL releaseSUS = TRUE;

        if (HXR_OK == pSUS->QueryInterface(IID_IHXValues,
                                (void**)&pSUSProps))
        {
            /*
             * Ask the site manager if any sites are available
             * for the LSG by this LSGName.
             */
            if (!m_pSiteManager->IsSiteAvailableByLSGName(pSUSProps,
                /*bIsPersistent*/ FALSE))
            {
                if (m_pSiteSupplier)
                {
                    /*
                     * Let the site supplier know that we are changing the layout.
                     */
                    if (m_bBeginChangeLayoutTobeCalled)
                    {
                        m_bBeginChangeLayoutTobeCalled  = FALSE;
                        m_pSiteSupplier->BeginChangeLayout();
                    }

                    /*
                     * Inform the TLC/SS of the new sites we need.
                     */
                    ULONG32 uReqestID = (ULONG32)pSUS;
                    m_pSiteSupplier->SitesNeeded(uReqestID,pSUSProps);
                    DisableScreenSaver();
                    m_SiteRequestIDList.AddTail((void*)uReqestID);
                }
            }

            /*
             * We can now assume that the site supplier has added
             * any sites for this set of properties to the site
             * manager that it is willing to provide.
             *
             * Tell the site manager to hook up the site user to
             * any sites with the same LSGName as the layoutSiteGroup.
             *
             * Hookup will also create child sites for the tuners,
             * But these are added to the site manager with a flag
             * which states how they can be used (for renderers only).
             * Because we don't want to accidentally connect one of these
             * tuner sites as a LSG.
             */

            /*  JEB: if the hookup failed, do not release the site user
             *  Hookup will add it to a list and hook it up later
             *  when the site has really been added
             */

            if (!m_pSiteManager->HookupByLSGName(pSUS,pSUSProps,/*bIsPersistent*/ FALSE))
            {
                releaseSUS = FALSE;
            }

            pSUSProps->Release();
        }

        if (releaseSUS)
        {
            pSUS->Release();
        }
    }
    // If the LSG doesn't support SiteUserSupplier, than see if it supports
    // single instance as a SiteUser...
    else if (HXR_OK == pLSG->QueryInterface(IID_IHXSiteUser,
                            (void**)&pSU))
    {
        IHXValues* pSUProps = NULL;

        HXBOOL releaseSU = TRUE;

        if (HXR_OK == pSU->QueryInterface(IID_IHXValues,
                                (void**)&pSUProps))
        {
            /*
             * Ask the site manager if any sites are available
             * for the LSG by this LSGName.
             */
            if (!m_pSiteManager->IsSiteAvailableByLSGName(pSUProps,
                /*bIsPersistent*/ FALSE))
            {
                if (m_pSiteSupplier)
                {
                    /*
                     * Let the site supplier know that we are changing the layout.
                     */
                    if (m_bBeginChangeLayoutTobeCalled)
                    {
                        m_bBeginChangeLayoutTobeCalled  = FALSE;
                        m_pSiteSupplier->BeginChangeLayout();
                    }

                    /*
                     * Inform the TLC/SS of the new sites we need.
                     */
                    ULONG32 uReqestID = (ULONG32)pSU;
                    m_pSiteSupplier->SitesNeeded(uReqestID,pSUProps);
                    DisableScreenSaver();
                    m_SiteRequestIDList.AddTail((void*)uReqestID);
                }
            }

            /*
             * We can now assume that the site supplier has added
             * any sites for this set of properties to the site
             * manager that it is willing to provide.
             *
             * Tell the site manager to hook up the site user to
             * any sites with the same LSGName as the layoutSiteGroup.
             *
             * Hookup will also create child sites for the tuners,
             * But these are added to the site manager with a flag
             * which states how they can be used (for renderers only).
             * Because we don't want to accidentally connect one of these
             * tuner sites as a LSG.
             */

            /*  JEB: if the hookup failed, do not release the site user
             *  Hookup will add it to a list and hook it up later
             *  when the site has really been added
             */

            if (!m_pSiteManager->HookupSingleSiteByLSGName(pSU,pSUProps,/*bIsPersistent*/ FALSE))
            {
                releaseSU = FALSE;
            }

            pSUProps->Release();
        }

        if (releaseSU)
        {
            pSU->Release();
        }
    }
#endif /* HELIX_FEATURE_VIDEO */
}

/************************************************************************
 *  Method:
 *      HXPlayer::SetupLayout
 *  Purpose:
 *      shuts down old layout and sets up new layout. If bShowNewLayout
 *      is FALSE then new layout is not set up.
 */
STDMETHODIMP
HXPlayer::SetupLayout(HXBOOL bIsPersistent)
{
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();

    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSource);
        pSourceInfo->SetupRendererSites(bIsPersistent);
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      HXPlayer::CleanupLayout
 *  Purpose:
 *      shuts down old layout
 */
STDMETHODIMP
HXPlayer::CleanupLayout()
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CleanupLayout()", this);
    /*
     * For each Site/SiteUser combination unhook them because
     * the layout is changing. You might think that this is
     * only appropriate for associations by LSGName, but we do it
     * for all because we want this implementation to work for
     * empty-layouts as well.
     *
     * Note this also means to unhook any and all site/user
     * combinations for child sites for each of these sites.
     * This means that all layouts are released.
     */
#if defined(HELIX_FEATURE_VIDEO)
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CleanupLayout(): unhook all; mgr [%p]", this, m_pSiteManager);
    if (m_pSiteManager) 
    {
        m_pSiteManager->UnhookAll();
    }

    /*
     * Run through all previous requested sites... Note:
     * this is not the same as the list of layoutGroupSites
     * since we don't request ones that were available when
     * we set the layout.
     */
    CHXSimpleList::Iterator ndxRequest = m_SiteRequestIDList.Begin();

    for (; ndxRequest != m_SiteRequestIDList.End(); ++ndxRequest)
    {
        ULONG32 requestID = (ULONG32)(*ndxRequest);

        HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CleanupLayout(): next site id = %lu; supplier [%p]", this, requestID, m_pSiteSupplier);

        if (m_pSiteSupplier)
        {
            /*
             * Let the site supplier know that we are changing the layout.
             */
            if (m_bBeginChangeLayoutTobeCalled)
            {
                m_bBeginChangeLayoutTobeCalled  = FALSE;
                
                m_pSiteSupplier->BeginChangeLayout();
            }

            /*
             * Inform the TLC/SS that we don't need old sites.
             */
            m_pSiteSupplier->SitesNotNeeded(requestID);
        }
    }
    m_SiteRequestIDList.RemoveAll();
#endif /* HELIX_FEATURE_VIDEO */

    return HXR_OK;
}

void
HXPlayer::InternalPause()
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::InternalPause()", this);
    if (!m_bIsFirstBegin && !m_bBeginPending && !m_bTimelineToBeResumed)
    {
        m_bIsPlaying = FALSE;
        m_bTimelineToBeResumed = TRUE;
        m_pAudioPlayer->Pause();
    }
}

/*
 *  IHXGroupSink methods
 */
/************************************************************************
*  Method:
*      IHXGroupSink::GroupAdded
*  Purpose:
*               Notification of a new group being added to the presentation.
*/
STDMETHODIMP
HXPlayer::GroupAdded(UINT16         /*IN*/ uGroupIndex,
                      IHXGroup*    /*IN*/ pGroup)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::GroupAdded(): idx = %u; group [%p]", this, uGroupIndex, pGroup);
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    m_nGroupCount++;
    if (m_nCurrentGroup == m_pGroupManager->GetGroupCount() - 1)
    {
        m_bLastGroup = TRUE;
    }
    else
    {
        m_bLastGroup = FALSE;
    }
#endif /* HELIX_FEATURE_BASICGROUPMGR */

    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupSink::GroupRemoved
*  Purpose:
*               Notification of a group being removed from the presentation.
*/
STDMETHODIMP
HXPlayer::GroupRemoved(UINT16       /*IN*/ uGroupIndex,
                        IHXGroup*  /*IN*/ pGroup)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::GroupRemoved(): idx = %u; group [%p]", this, uGroupIndex, pGroup);
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    if (m_nGroupCount > 0)
        m_nGroupCount--;

    UINT16 uNumGroups = m_pGroupManager->GetGroupCount();
    if (uNumGroups == 0 ||
        m_nCurrentGroup == uNumGroups - 1)
    {
        m_bLastGroup = TRUE;
    }
    else
    {
        m_bLastGroup = FALSE;
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

/************************************************************************
*  Method:
*      IHXGroupSink::AllGroupsRemoved
*  Purpose:
*               Notification that all groups have been removed from the
*               current presentation.
*/
STDMETHODIMP
HXPlayer::AllGroupsRemoved(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::AllGroupsRemoved()", this);
    HX_RELEASE(m_pCurrentGroup);
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupSink::TrackAdded
*  Purpose:
*               Notification of a new track being added to a group.
*/
STDMETHODIMP
HXPlayer::TrackAdded(UINT16         /*IN*/ uGroupIndex,
                      UINT16        /*IN*/ uTrackIndex,
                      IHXValues*   /*IN*/ pTrack)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::TrackAdded(): idx group = %u;  track [pTrack] idx = %u", 
                    this, uGroupIndex, pTrack, uTrackIndex);

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::TrackAdded(GroupID=%hd TrackID=%hd) Start", 
        this, 
        uGroupIndex, 
        uTrackIndex);

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    HX_RESULT       theErr = HXR_OK;
    char            szDelay[] = "Delay";
    UINT32          ulDelay = 0;
    IHXGroup*       pThisGroup = NULL;

    m_pGroupManager->GetGroup(uGroupIndex, pThisGroup);

    /* Check if a track is added to the group currently played */
    if (uGroupIndex == m_nCurrentGroup &&
        m_pCurrentGroup == pThisGroup)
    {
#if defined(HELIX_FEATURE_PREFETCH)
    SourceInfo*     pSourceInfo = NULL;
    IHXPrefetch*   pPrefetch = NULL;
        // determine whether the track has been prefetched
        if (m_pPrefetchManager &&
            m_pPrefetchManager->Lookup(pTrack, pSourceInfo))
        {
            pSourceInfo->m_pSource->PartOfPrefetchGroup(FALSE);
            if (HXR_OK == m_pCurrentGroup->QueryInterface(IID_IHXPrefetch,
                                                          (void**)&pPrefetch))
            {
                theErr = pPrefetch->RemovePrefetchTrack(pSourceInfo->m_uTrackID);
            }
            HX_RELEASE(pPrefetch);

            pSourceInfo->m_uGroupID = uGroupIndex;
            pSourceInfo->m_uTrackID = uTrackIndex;
            PrepareSourceInfo(pTrack, pSourceInfo);

            pSourceInfo->m_pSource->UpdatePlayTimes(pTrack);

            m_pSourceMap->SetAt((void*)pSourceInfo->m_pSource,
                              (void*)pSourceInfo);

            m_bPlayerWithoutSources = FALSE;
            m_bSourceMapUpdated = TRUE;
            m_bForceStatsUpdate = TRUE;

            m_uNumSourcesActive++;
            m_uNumCurrentSourceNotDone++;

            AdjustPresentationTime();

            InternalPause();
        }
        /* If we are not yet initialized, add tracks to the current source
         * map only if the track is within the fudge factor.
         *
         * This is to fix a bug in SMIL file with multiple sources in seq
         * with outer par.
         * The expected behavior is to start the first source
         * in seq and then initialize the remaining sources. Since we were
         * adding additional tracks to the source map, the player was not
         * getting initialized until ALL the sources in the seq have been
         * initialized. This resulted in massive startup delays.
         */
    else if (!m_bInitialized)
#else
        if (!m_bInitialized)
#endif
        {
            if ((HXR_OK != pTrack->GetPropertyULONG32(szDelay,ulDelay)) ||
                (ulDelay <= m_ulCurrentPlayTime + MIN_DELAYBEFORE_START))
            {
                theErr = OpenTrack(pTrack, uGroupIndex, uTrackIndex);
            }
            else
            {
                if (!m_pPendingTrackList)
                {
                    m_pPendingTrackList = new CHXSimpleList;
                }

                PendingTrackInfo* pPendingTrackInfo =
                    new PendingTrackInfo(uGroupIndex, uTrackIndex, pTrack);

                m_pPendingTrackList->AddTail(pPendingTrackInfo);
            }
        }
        else
        {
            theErr = OpenTrack(pTrack, uGroupIndex, uTrackIndex);
        }

        if (theErr)
        {
            ReportError(NULL, theErr, NULL);
        }
    }
    /* Check if a track is added to the group that is being prefetch */
    else
    {
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
        IHXGroup* pGroup        = NULL;
        UINT16 uCurrentGroup    = 0;
        if (m_bNextGroupStarted &&
            (m_pNextGroupManager->GetCurrentGroup(uCurrentGroup, pGroup)
                                                == HXR_OK) &&
            uCurrentGroup == uGroupIndex &&
            pGroup == pThisGroup)
        {
            m_bPartOfNextGroup = TRUE;
            theErr = OpenTrack(pTrack, uGroupIndex, uTrackIndex);
            if (theErr)
            {
                ReportError(NULL, theErr, NULL);
            }
            m_bPartOfNextGroup = FALSE;
        }

        HX_RELEASE(pGroup);
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
    }

    HX_RELEASE(pThisGroup);

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::TrackAdded(GroupID=%hd TrackID=%hd) End", 
        this, 
        uGroupIndex, 
        uTrackIndex);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

HX_RESULT
HXPlayer::RepeatTrackAdded(UINT16       /*IN*/ uGroupIndex,
                            UINT16      /*IN*/ uTrackIndex,
                            IHXValues*  /*IN*/ pTrack)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::RepeatTrackAdded(): idx group = %u;  track [pTrack] idx = %u", 
                    this, uGroupIndex, pTrack, uTrackIndex);

#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR) && defined(HELIX_FEATURE_SMIL_REPEAT)
    HX_RESULT           theErr = HXR_OK;
    UINT16              uCurrentGroup   = 0;
    SourceInfo*         pSourceInfo = NULL;
    IHXGroup*           pGroup  = NULL;
    IHXGroup*           pThisGroup = NULL;

    m_pGroupManager->GetGroup(uGroupIndex, pThisGroup);

    if (HXR_OK == GetSourceInfo(uGroupIndex, uTrackIndex, pSourceInfo))
    {
        theErr = pSourceInfo->AppendRepeatRequest(uTrackIndex, pTrack);
        AdjustPresentationTime();
    }
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    else if (m_bNextGroupStarted &&
             (m_pNextGroupManager->GetCurrentGroup(uCurrentGroup, pGroup) == HXR_OK) &&
             uCurrentGroup == uGroupIndex &&
             pGroup == pThisGroup)
    {
        m_pNextGroupManager->AddRepeatTrack(uTrackIndex, pTrack);
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
    else
    {
        // XXX HP something is wrong!!
        HX_ASSERT(FALSE);
    }

    HX_RELEASE(pGroup);
    HX_RELEASE(pThisGroup);

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR && HELIX_FEATURE_SMIL_REPEAT */
}


/************************************************************************
*  Method:
*      IHXGroupSink::TrackRemoved
*  Purpose:
*               Notification of a track being removed from a group.
*/
STDMETHODIMP
HXPlayer::TrackRemoved(UINT16           /*IN*/ uGroupIndex,
                        UINT16          /*IN*/ uTrackIndex,
                        IHXValues*      /*IN*/ pTrack)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::TrackRemoved(): idx group = %u;  track [pTrack] idx = %u", 
                    this, uGroupIndex, pTrack, uTrackIndex);
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupSink::TrackStarted
*  Purpose:
*               Notification of a track being started in a group.
*/
STDMETHODIMP
HXPlayer::TrackStarted(UINT16           /*IN*/ uGroupIndex,
                        UINT16          /*IN*/ uTrackIndex,
                        IHXValues*      /*IN*/ pTrack)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::TrackStarted(): idx group = %u;  track [pTrack] idx = %u", 
                    this, uGroupIndex, pTrack, uTrackIndex);
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupSink::TrackStopped
*  Purpose:
*               Notification of a track being stopped in a group.
*/
STDMETHODIMP
HXPlayer::TrackStopped(UINT16           /*IN*/ uGroupIndex,
                        UINT16          /*IN*/ uTrackIndex,
                        IHXValues*      /*IN*/ pTrack)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::TrackStopped(): idx group = %u;  track [pTrack] idx = %u", 
                    this, uGroupIndex, pTrack, uTrackIndex);
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupSink::CurrentGroupSet
*  Purpose:
*               This group is being currently played in the presentation.
*/
STDMETHODIMP
HXPlayer::CurrentGroupSet(UINT16        /*IN*/ uGroupIndex,
                           IHXGroup*   /*IN*/ pGroup)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::CurrentGroupSet(): group [%p] idx = %u", this, pGroup, uGroupIndex);
#if defined(HELIX_FEATURE_BASICGROUPMGR)
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;
    /* If we called SetCurrentGroup, ignore this callback */
    m_bIsPresentationClosedToBeSent = FALSE;
    StopAllStreams(END_STOP);
    m_bIsPresentationClosedToBeSent = TRUE;

    ResetGroup();
    m_bInitialized      = FALSE;
    m_bIsDone           = FALSE;
    m_ulActiveSureStreamSource = 0;
    m_bFastStartCheckDone = FALSE;
    m_turboPlayOffReason = TP_OFF_BY_UNKNOWN;

    // uGroupIndex is needed to determine whether destroy the root layout
    // during group switching under nested meta support
    // uGroupIndex is used by IHXPersistentComponentManager::CloseAllRenderers()
    // which is called in CloseAllRenderers()
    CloseAllRenderers(uGroupIndex);

    /* Open this group here */

    /* Ask the next group manager for this group */

    IHXGroup* pCurGroup = NULL;
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    UINT16 uCurrentGroup = 0;
    if (m_pNextGroupManager->GetCurrentGroup(uCurrentGroup, pCurGroup) == HXR_OK &&
        uCurrentGroup == uGroupIndex && pCurGroup == pGroup)
    {
        SourceInfo*     pSourceInfo = NULL;

        UINT16 uNumSources = m_pNextGroupManager->GetNumSources();
        for (UINT16 i = 0; i < uNumSources; i++)
        {
            m_pNextGroupManager->GetSource(i, pSourceInfo);

            if (pSourceInfo->m_pSource)
            {
                m_pSourceMap->SetAt((void*) pSourceInfo->m_pSource,
                                  (void*) pSourceInfo);

                m_bPlayerWithoutSources = FALSE;
                m_bSourceMapUpdated = TRUE;

                pSourceInfo->m_pSource->PartOfNextGroup(FALSE);

                if (pSourceInfo->m_bTobeInitializedBeforeBegin)
                {
                    m_uNumSourceToBeInitializedBeforeBegin++;
                }

                // update the registries (from NextGroup.* to Player.*)
                UpdateSourceInfo(pSourceInfo,
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
                                 m_pStats->m_ulRegistryID,
#else
                                 NULL,
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
                                 pSourceInfo->m_uTrackID);
            }
        }

        m_pNextGroupManager->RemoveAllSources();

        const char* pErrorString = NULL;
        HXSource* pSource        = NULL;
        theErr = m_pNextGroupManager->GetLastError(pSource, pErrorString);

        if (theErr)
        {
            SetLastError(theErr);
        }

        if (m_LastError != HXR_OK)
        {
            m_bIsDone = TRUE;
            ReportError(pSource, m_LastError, pErrorString);
        }
        else
        {
            m_nCurrentGroup = uGroupIndex;
        }

        m_pNextGroupManager->Cleanup();

        /* Get the duration from group properties */
        UINT32 ulGroupDuration = 0;
        IHXValues* pGroupProps = pGroup->GetGroupProperties();
        if (pGroupProps &&
            pGroupProps->GetPropertyULONG32("Duration", ulGroupDuration)
                                                                == HXR_OK)
        {
            m_ulPresentationDuration = ulGroupDuration;
        }

        HX_RELEASE(pGroupProps);

        m_bIsPresentationClosedToBeSent = FALSE;
    }
    else
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
    {
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
        /* Cleanup any next group that we may have started downloading */
        m_pNextGroupManager->Cleanup();
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

        m_nCurrentGroup = uGroupIndex;
        theErr = DoOpenGroup(uGroupIndex);
    }

    if (uGroupIndex == (m_pGroupManager->GetGroupCount() - 1))
    {
        m_bLastGroup = TRUE;
    }
    else
    {
        m_bLastGroup = FALSE;
    }

    m_pCurrentGroup = pGroup;
    m_pCurrentGroup->AddRef();

    m_bNextGroupStarted = FALSE;
    HX_RELEASE(pCurGroup);

    if (!theErr && !m_LastError && m_bUserHasCalledBegin)
    {
        Begin();
    }

    SchedulePlayer(PLAYER_SCHEDULE_DEFAULT | PLAYER_SCHEDULE_IMMEDIATE | PLAYER_SCHEDULE_RESET);

    if (theErr)
    {
        ReportError(NULL, theErr, NULL);
    }
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

/************************************************************************
*  Method:
*      IHXGroupSink::CurrentGroupSet
*  Purpose:
*               This group is set to be played next in the presentation.
*/
HX_RESULT
HXPlayer::NextGroupSet(UINT16   /*IN*/ uGroupIndex)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::NextGroupSet(): group idx = %u", this, uGroupIndex);
#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    /* Ask the next group manager for this group */
    UINT16 uCurrentGroup = 0;
    IHXGroup* pCurGroup = NULL;
    if (m_pNextGroupManager && m_pNextGroupManager->GetCurrentGroup(uCurrentGroup,
        pCurGroup) == HXR_OK)
    {
        // the next group being set is already the next group, so don't do anything
        if (uCurrentGroup == uGroupIndex)
        {
            return HXR_OK;
        }

        /* Cleanup any next group that we may have started downloading */
        m_pNextGroupManager->Cleanup();
        m_bNextGroupStarted     = FALSE;
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
}

/************************************************************************
*   Method:
*       IHXRendererUpgrade::IsRendererAvailable
*   Purpose:
*       See if a renderer with this mime type has been loaded
*/
STDMETHODIMP_(HXBOOL)
HXPlayer::IsRendererAvailable(const char* pMimeType)
{
    HXBOOL bAvailable = FALSE;
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    // create an upgrade collection
    HXUpgradeCollection* pCheckComponent = new HXUpgradeCollection((IUnknown*)(IHXPlayer*)this);
    if (pCheckComponent)
    {
        pCheckComponent->AddRef();

        IHXBuffer* pBuffer = NULL;
    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (BYTE*)pMimeType, strlen(pMimeType)+1, (IUnknown*)(IHXPlayer*)this))
    {
        // add the component to the list
        pCheckComponent->Add(eUT_Required, pBuffer, 0, 0);
        HX_RELEASE(pBuffer);
    }

        // Request the upgrade handler
        IHXUpgradeHandler* pUpgradeHandler = NULL;
        if(m_pClient &&
            m_pClient->QueryInterface(IID_IHXUpgradeHandler, (void**)&pUpgradeHandler) == HXR_OK)
        {
            // now see if the upgrade handler already has this component
            if(pUpgradeHandler->HasComponents(pCheckComponent) == HXR_OK)
            {
                bAvailable = TRUE;
            }
            HX_RELEASE(pUpgradeHandler);
        }
    }

    HX_RELEASE(pCheckComponent);
#endif /* HELIX_FEATURE_AUTOUPGRADE */

    // return component availability
    return(bAvailable);
}

/************************************************************************
*   Method:
*       IHXRendererUpgrade::ForceUpgrade
*   Purpose:
*       Use the force to upgrade all renderers
*/
STDMETHODIMP
HXPlayer::ForceUpgrade()
{
    return HXR_UNEXPECTED;
}

/************************************************************************
*  Method:
*      IHXLayoutSiteGroupManager::AddLayoutSiteGroup
*  Purpose:
*               Add LSG to the presentation.
*/
STDMETHODIMP
HXPlayer::AddLayoutSiteGroup(IUnknown*  /*IN*/ pLSG)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::AddLayoutSiteGroup()", this);
#if defined(HELIX_FEATURE_VIDEO)
    m_bAddLayoutSiteGroupCalled = TRUE;
    SetupLayoutSiteGroup(pLSG, TRUE);
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_VIDEO */
}

/************************************************************************
*  Method:
*      IHXLayoutSiteGroupManager::RemoveLayoutSiteGroup
*  Purpose:
*               Remove LSG from the presentation.
*/
STDMETHODIMP
HXPlayer::RemoveLayoutSiteGroup(IUnknown* pLSG)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::RemoveLayoutSiteGroup()", this);
#if defined(HELIX_FEATURE_PLAYERNAVIGATOR)
    HX_ASSERT(pLSG);

    IHXSiteUserSupplier*   pSUS = NULL;
    IHXSiteUser*           pSU = NULL;
    HXBOOL                    bIsPersistent = TRUE;   // assume called by a persistent renderer

    if (HXR_OK == pLSG->QueryInterface(IID_IHXSiteUserSupplier,
                            (void**)&pSUS))
    {
        IHXValues* pSUSProps = NULL;

        HXBOOL releaseSUS = TRUE;

        if (HXR_OK == pSUS->QueryInterface(IID_IHXValues,
                                (void**)&pSUSProps))
        {
#if defined(HELIX_FEATURE_VIDEO)
            m_pSiteManager->RemoveSitesByLSGName(pSUSProps, /*bIsPersistent*/ FALSE);
            //m_pSiteManager->UnhookByLSGName(pSUS, pSUSProps, bIsPersistent);
#endif //HELIX_FEATURE_VIDEO

            pSUSProps->Release();
        }
        pSUS->Release();
    }
    // If the LSG doesn't support SiteUserSupplier, than see if it supports
    // single instance as a SiteUser...
    else if (HXR_OK == pLSG->QueryInterface(IID_IHXSiteUser,
                            (void**)&pSU))
    {
        IHXValues* pSUProps = NULL;

        if (HXR_OK == pSU->QueryInterface(IID_IHXValues,
                                (void**)&pSUProps))
        {
#if defined(HELIX_FEATURE_VIDEO)
            m_pSiteManager->RemoveSitesByLSGName(pSUProps, /*bIsPersistent*/ FALSE);
            // m_pSiteManager->UnhookByLSGName(pSU, pSUProps, bIsPersistent);
#endif //HELIX_FEATURE_VIDEO

            pSUProps->Release();
        }
        pSU->Release();
    }

    m_bAddLayoutSiteGroupCalled = FALSE;
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_VIDEO */
}

/*
 * IHXInternalReset method
 */
STDMETHODIMP
HXPlayer::InternalReset(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::InternalReset()", this);
    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

    m_bInternalReset = TRUE;
    PausePlayer();
    BeginPlayer();
    m_bInternalReset = FALSE;

    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return HXR_OK;
}

void
HXPlayer::CheckToStartNextGroup(void)
{
    if (m_bLastGroup)
    {
        return;
    }

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    if (m_pGroupManager->GetGroupCount() <= 1)
    {
        m_bLastGroup = TRUE;
        return;
    }

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    /* If all sources have ended, start downloading the next group */
    if (m_uNumCurrentSourceNotDone == 0)
    {
        /* Resume prefetching if the next group already has sources */
        if (m_pNextGroupManager->GetNumSources() > 0)
        {
            UnRegisterCurrentSources();
            m_pNextGroupManager->ContinuePreFetch();
            m_bNextGroupStarted = TRUE;
            return;
        }

        if (m_nCurrentGroup < (m_pGroupManager->GetGroupCount() - 1))
        {
            IHXGroup* pGroup = NULL;
            UINT16 uNextGroup = 0;
            m_pGroupManager->GetNextGroup(uNextGroup);
            HX_RESULT theErr = m_pGroupManager->GetGroup(uNextGroup, pGroup);
            if (!theErr)
            {
                m_pNextGroupManager->SetCurrentGroup(uNextGroup, pGroup);
                HX_RELEASE(pGroup);

                UnRegisterCurrentSources();

                m_bPartOfNextGroup = TRUE;
                theErr = DoOpenGroup(uNextGroup);
                m_bPartOfNextGroup = FALSE;
                m_bNextGroupStarted     = TRUE;

                HXLOGL2(HXLOG_TRAN, "Next Group is prefetched: %lu", 
                        uNextGroup);

                if (theErr)
                {
                    m_pNextGroupManager->SetLastError(theErr);
                }
            }
        }
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */
#endif /* HELIX_FEATURE_BASICGROUPMGR */
}

void
HXPlayer::AdjustPresentationTime(void)
{
    UINT32 ulSourceDuration = 0;

    // reset presentation duration
    m_ulPresentationDuration = 0;

    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSource);

        // get duration after the source has been initialized
        // to ensure HXSource::AdjustClipTime() was called
        if (pSourceInfo->m_pSource->IsInitialized())
        {
            ulSourceDuration = pSourceInfo->GetActiveDuration();
            m_ulPresentationDuration = HX_MAX(m_ulPresentationDuration,
                                           ulSourceDuration);
        }
    }

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    // presentation duration is also restrained by its group duration
    // if it exists
    IHXGroup*   pGroup = NULL;
    if (HXR_OK == m_pGroupManager->GetGroup((UINT16)m_nCurrentGroup, pGroup))
    {
        /* Get the duration from group properties */
        UINT32 ulGroupDuration = 0;
        IHXValues* pGroupProps = pGroup->GetGroupProperties();
        if (pGroupProps &&
            HXR_OK == pGroupProps->GetPropertyULONG32("Duration", ulGroupDuration))
        {
            m_ulPresentationDuration = ulGroupDuration;
        }
        HX_RELEASE(pGroupProps);
    }
    HX_RELEASE(pGroup);
#endif /* HELIX_FEATURE_BASICGROUPMGR */

    if (m_pAdviseSink)
    {
        m_pAdviseSink->OnPosLength(m_ulCurrentPlayTime, m_ulPresentationDuration);
    }

    return;
}

void
HXPlayer::SetPresentationTime(UINT32 ulPresentationTime)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::SetPresentationTime(%lu)", this, ulPresentationTime);
    m_ulPresentationDuration = ulPresentationTime;

    if (m_ulPresentationDuration > 0)
    {
    m_bHasSubordinateLifetime = FALSE;
    }

    if (m_pAdviseSink)
    {
        m_pAdviseSink->OnPosLength(m_ulCurrentPlayTime, m_ulPresentationDuration);
    }
}

void
HXPlayer::UpdateSourceActive(void)
{
    m_uNumSourcesActive = 0;

    CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
    for (; ndxSources != m_pSourceMap->End(); ++ndxSources)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
        if (pSourceInfo->m_bActive)
        {
            m_uNumSourcesActive++;
        }
    }

    m_uNumCurrentSourceNotDone = m_uNumSourcesActive;
}

HX_RESULT
HXPlayer::UpdateSourceInfo(SourceInfo* pSourceInfo,
                            UINT32 ulParentRegId,
                            UINT16 ulTrackIndex)
{
    HX_RESULT   rc = HXR_OK;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    UINT32      ulRegId = 0;
    IHXBuffer*  pParentName = NULL;
    // update sourc/stream stats' registry
    if (m_pRegistry && m_pStats &&
        HXR_OK == m_pRegistry->GetPropName(ulParentRegId, pParentName))
    {
        char        szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
        SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.Source%ld", pParentName->GetBuffer(), ulTrackIndex);
        // delete this registry if it exists
        ulRegId = m_pRegistry->GetId(szRegName);
        if (ulRegId)
        {
            m_pRegistry->DeleteById(ulRegId);
        }
        // create/update registry
        ulRegId = m_pRegistry->AddComp(szRegName);
        pSourceInfo->m_pSource->UpdateRegistry(ulRegId);

#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

        pSourceInfo->m_uTrackID = ulTrackIndex;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    }
    HX_RELEASE(pParentName);

    // update renderer stats registry
    pSourceInfo->ReInitializeStats();
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return rc;
}

HX_RESULT
HXPlayer::UpdatePersistentSrcInfo(SourceInfo* pSourceInfo,
                                  UINT32 ulParentRegId,
                                  UINT16 ulTrackIndex)
{
    HX_RESULT   rc = HXR_OK;
    IHXBuffer*  pParentName = NULL;

#if defined(HELIX_FEATURE_REGISTRY)
    UINT32      ulRegId = 0;
    char        szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    // update sourc/stream stats' registry
    if (m_pRegistry && m_pStats &&
        HXR_OK == m_pRegistry->GetPropName(ulParentRegId, pParentName))
    {
        SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.Source%u",
                pParentName->GetBuffer(), ulTrackIndex);

        // delete this registry if it exists
        ulRegId = m_pRegistry->GetId(szRegName);
        if (ulRegId)
        {
            m_pRegistry->DeleteById(ulRegId);
        }

        //Create a new one.
        SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.Persistent%u", pParentName->GetBuffer(),
                pSourceInfo->m_ulPersistentComponentSelfID);

        // create/update registry
        ulRegId = m_pRegistry->GetId(szRegName);
        if( !ulRegId )
        {
            ulRegId = m_pRegistry->AddComp(szRegName);
        }
        pSourceInfo->m_pSource->UpdateRegistry(ulRegId);
        pSourceInfo->m_uTrackID = ulTrackIndex;
    }
#endif
    HX_RELEASE(pParentName);

    // update renderer stats registry
    pSourceInfo->ReInitializeStats();

    return rc;
}

HX_RESULT
HXPlayer::SetupAllStreams(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::SetupAllStreams()", this);
    HX_RESULT theErr = HXR_OK;
    CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
    /* Check if we are done. This may be TRUE for empty files */
    for (; !theErr && ndxSources != m_pSourceMap->End(); ++ndxSources)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
        theErr = pSourceInfo->SetupStreams();
    }

    return theErr;
}

void
HXPlayer::EnterToBeginList(RendererInfo* pRendInfo)
{
    if (m_ToBeginRendererList.IsEmpty())
    {
        m_ToBeginRendererList.AddHead(pRendInfo);
        return;
    }

    RendererInfo*   pTmpRendInfo        = NULL;
    HXBOOL            earlierPacketFound  = FALSE;

    UINT32 startPos = pRendInfo->m_pStreamInfo->m_ulDelay;

    LISTPOSITION    position = m_ToBeginRendererList.GetTailPosition();
    while (position != NULL && !earlierPacketFound)
    {
        pTmpRendInfo =
            (RendererInfo*) m_ToBeginRendererList.GetPrev(position);

        // If this event is less than or equal to the timestamp
        // then this things are looking ok...
        if (pTmpRendInfo->m_pStreamInfo->m_ulDelay <= startPos)
        {
            // Remember that we found an earlier packet...
            earlierPacketFound = TRUE;

            // If the position is null, then event was the first
            // item in the list, and we need to do some fancy footwork...
            if (!position)
            {
                POSITION theHead = m_ToBeginRendererList.GetHeadPosition();
                m_ToBeginRendererList.InsertAfter(theHead,pRendInfo);
            }
            // otherwise, roll ahead one...
            else
            {
                m_ToBeginRendererList.GetNext(position);
                // Now if the position is null, then event was the last
                // item in the list, and we need to do some more fancy footwork...
                if (!position)
                {
                    m_ToBeginRendererList.AddTail(pRendInfo);
                }
                else
                // otherwise, we have a normal case and we want to insert
                // right after the position of event
                {
                    m_ToBeginRendererList.InsertAfter(position,pRendInfo);
                }
            }

            // We don't need to search any more...
            break; // while
        }
    } // end while...

    // If we didn't find an earlier packet, then we should insert at
    // the head of the event list...
    if (!earlierPacketFound)
    {
        m_ToBeginRendererList.AddHead(pRendInfo);
    }
}

HX_RESULT
HXPlayer::CheckBeginList(void)
{
    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CheckBeginList() Start", this);
    CHXSimpleList beginningSourceList;
    HXSource* pBeginningSource;

    /* Send Begins to all the scheduled renderers */
    LISTPOSITION lPos = m_ToBeginRendererList.GetHeadPosition();
    while (lPos)
    {
        RendererInfo* pRendInfo = (RendererInfo*) m_ToBeginRendererList.GetAt(lPos);
        if ((pRendInfo->m_bInterruptSafe || !m_pEngine->AtInterruptTime()) &&
            (((m_ulCurrentPlayTime < pRendInfo->m_pStreamInfo->m_ulDelay) &&
              (pRendInfo->m_pStreamInfo->m_ulDelay - m_ulCurrentPlayTime < m_ulPlayerUpdateInterval)) ||
             (pRendInfo->m_pStreamInfo->m_ulDelay <= m_ulCurrentPlayTime)))
        {
            HX_RESULT   rcTemp = HXR_OK;
            UINT32      ulOnBeginTime = 0;

            rcTemp = CalculateOnBeginTime(pRendInfo, ulOnBeginTime);
            if (HXR_OK == rcTemp)
            {
        pBeginningSource = pRendInfo->m_pStream->GetHXSource();

        HX_ASSERT(pBeginningSource);

                pRendInfo->m_bInitialBeginToBeSent  = FALSE;
        if (pRendInfo->m_pRenderer)
        {
            pRendInfo->m_pRenderer->OnBegin(ulOnBeginTime);
        }
                lPos = m_ToBeginRendererList.RemoveAt(lPos);

        if (pBeginningSource)
        {
            if (!beginningSourceList.Find(pBeginningSource))
            {
            // source we have not seen yet
            beginningSourceList.AddHead(pBeginningSource);
            pBeginningSource = NULL;
            }
            HX_RELEASE(pBeginningSource);
        }
            }
            else
            {
                // no other error except HXR_NO_DATA can be returned
                //
                // this can happen for live stream if there is insufficient 
                // information to calculate the OnBegin Time
                HX_ASSERT(HXR_NO_DATA == rcTemp);
                m_ToBeginRendererList.GetAtNext(lPos);
            }
        }
        else
        {
            break;
        }
    }

    // Resume audio streams of all sources for which the renderers (streams)
    // were just begun.
    while (!beginningSourceList.IsEmpty())
    {
    pBeginningSource = (HXSource*) beginningSourceList.RemoveHead();
    HX_ASSERT(pBeginningSource);
    if (pBeginningSource)
    {
        pBeginningSource->ResumeAudioStreams();
        HX_RELEASE(pBeginningSource);
    }
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::CheckBeginList() End", this);

    return HXR_OK;
}

HX_RESULT              
HXPlayer::CalculateOnBeginTime(RendererInfo* pRendInfo, 
                   UINT32& ulOnBeginTime)
{
    HX_RESULT retVal;
    IHXStreamSource* pSource = NULL;

    retVal = pRendInfo->m_pStream->GetSource(pSource);

    // For live streams, we cannot compute begin time until the stream start time is
    // known.  Start time should become known for the live stream once the first
    // packet of the live stream is received.
    if ((retVal == HXR_OK) && 
    pSource->IsLive() && 
    (!pRendInfo->m_bStartTimeValid))
    {
        retVal = HXR_NO_DATA;
    }

    if (retVal == HXR_OK)
    {
    UpdateCurrentPlayTime(m_pAudioPlayer->GetCurrentPlayBackTime());

    ulOnBeginTime = m_ulCurrentPlayTime;
    if (pRendInfo->m_bInitialBeginToBeSent)
    {
        // If initial begin is yet to be sent, this means we have not
        // started playing this stream and we can and need to compute
        // the time offset to this renderer's time coordinate system
        // that will be used in Onbegin as well as in OnTimeSync call
        // to the renderer.  In case of the live stream, the time-stamps
        // passed to the renderer are not offset to 0 and thus we
        // anchor the renderer time coordinate system to the live stream
        // start time.
        if (pSource->IsLive())
        {
        pRendInfo->m_ulTimeDiff = pRendInfo->m_ulStreamStartTime;
        }
    }
    ulOnBeginTime += pRendInfo->m_ulTimeDiff;
    }

    HX_RELEASE(pSource);

    return retVal;
}

void
HXPlayer::RemoveFromPendingList(RendererInfo* pRendInfo)
{
    LISTPOSITION lPos = NULL;

    lPos = m_ToBeginRendererList.Find((void*) pRendInfo);
    if (lPos)
    {
        m_ToBeginRendererList.RemoveAt(lPos);
    }
}

void
HXPlayer::UnregisterNonActiveSources()
{
    CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
    /* Check if we are done. This may be TRUE for empty files */
    for (; !m_bIsDone && ndxSources != m_pSourceMap->End(); ++ndxSources)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
        if (pSourceInfo->m_pSource &&
            (pSourceInfo->m_pSource->IsSourceDone() ||
            pSourceInfo->m_pSource->IsDelayed()))
        {
            pSourceInfo->UnRegister();
            pSourceInfo->m_pSource->AdjustClipBandwidthStats(FALSE);
        }
    }
}


/*
 *  Purpose: Shutdown all the renderers/fileformats
 */
void
HXPlayer::ShutDown(void)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::ShutDown()", this);
    /* If we are not in a STOP state, do an internal stop */
    if (!m_bIsDone)
    {
        m_bIsPresentationClosedToBeSent = FALSE;
        StopPlayer(END_STOP);
    }

    CloseAllRenderers(m_nCurrentGroup);
}


HXBOOL
HXPlayer::AreAllSourcesSeekable(void)
{
    CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
    for (; ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
        HXSource * pSource = pSourceInfo->m_pSource;

        if(!pSource)
        {
            continue;
        }

        /* If any one source is non-seekable, entire presentation
         * is non-seekable
         */
        if (!pSource->IsSeekable())
        {
            return FALSE;
        }
    }

    return TRUE;
}

void
HXPlayer::RegisterSourcesDone()
{
#if defined(HELIX_FEATURE_ASM)
    /* We are done Registering Sources with ASM Bandwidth Manager */
    HX_ASSERT(m_pBandwidthMgr);

    m_pBandwidthMgr->RegisterSourcesDone();

    if (m_pBandwidthMgr->NotEnoughBandwidth() == TRUE)
    {
        SetLastError(HXR_NOTENOUGH_BANDWIDTH);
    }
#endif /* HELIX_FEATURE_ASM */
}

HXBOOL
HXPlayer::CanBeStarted(HXSource* pSource, SourceInfo* pThisSourceInfo, HXBOOL m_bPartOfNextGroup)
{
    UINT32      ulDelay             = pSource->GetDelay();

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    if (m_bPartOfNextGroup && m_pNextGroupManager)
    {
        return m_pNextGroupManager->CanBeStarted(pSource, pThisSourceInfo);
    }
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

    if ((ulDelay < m_ulCurrentPlayTime)                             ||
        (ulDelay - m_ulCurrentPlayTime <= MIN_DELAYBEFORE_START)    ||
        !pThisSourceInfo)
    {
        return TRUE;
    }

    CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
    /* Check if we are done. This may be TRUE for empty files */
    for (; ndxSources != m_pSourceMap->End(); ++ndxSources)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
        if (!pSourceInfo->m_pSource ||
            pSourceInfo->m_pSource->IsSourceDone() ||
            !pSourceInfo->m_pSource->IsInitialized())
        {
            continue;
        }

#ifdef SEQ_DEPENDENCY
        int iRetVal = 0;
        if (!pSourceInfo->m_pSource->IsLive() &&
            IsDependent(pThisSourceInfo, pSourceInfo) &&
            !pSourceInfo->m_pSource->IsSourceDone())
        {
            return FALSE;
        }
#else
        if (!pSourceInfo->m_pSource->IsLive() &&
             pSourceInfo->m_pSource->GetDuration() <= ulDelay &&
            !pSourceInfo->m_pSource->IsSourceDone())
        {
            return FALSE;
        }
#endif /*SEQ_DEPENDENCY*/

    }

    return TRUE;
}

#ifdef SEQ_DEPENDENCY
HXBOOL
HXPlayer::IsDependent(SourceInfo* pThisSourceInfo, SourceInfo* pSourceInfo)
{
    if (!pThisSourceInfo                            ||
        !pSourceInfo                                ||
        pThisSourceInfo->m_uNumDependencies == 0    ||
        pSourceInfo->m_uNumDependencies     == 0)
    {
        return FALSE;
    }

    UINT16 uDepdencyNum = 0;

    while (uDepdencyNum < pThisSourceInfo->m_uNumDependencies &&
           uDepdencyNum < pSourceInfo->m_uNumDependencies)
    {
        if (pThisSourceInfo->m_pDependNode[uDepdencyNum] ==
            pSourceInfo->m_pDependNode[uDepdencyNum])
        {
            uDepdencyNum++;
        }
        else
        {

        }
    }

    if (uDepdencyNum < pThisSourceInfo->m_uNumDependencies &&
        uDepdencyNum >= pSourceInfo->m_uNumDependencies
    {
        return FALSE;
    }

    return TRUE;
}
#endif /*SEQ_DEPENDENCY*/

void
HXPlayer::EndOfSource(HXSource* pSource)
{
    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::EndOfSource(): source [%p]", this, pSource);

    HXBOOL bAtLeastOneSourceToBeResumed = FALSE;

    CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
    /* Check if we are done. This may be TRUE for empty files */
    for (; ndxSources != m_pSourceMap->End(); ++ndxSources)
    {
        SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
        if (!pSourceInfo->m_pSource)
        {
            continue;
        }

        if (pSourceInfo->m_pSource->TryResume())
        {
            bAtLeastOneSourceToBeResumed = TRUE;
        }
    }

    if (bAtLeastOneSourceToBeResumed)
    {
        RegisterSourcesDone();

        ndxSources = m_pSourceMap->Begin();
        /* Check if we are done. This may be TRUE for empty files */
        for (; ndxSources != m_pSourceMap->End(); ++ndxSources)
        {
            SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
            if (!pSourceInfo->m_pSource)
            {
                continue;
            }

            if (pSourceInfo->m_pSource->IsResumePending())
            {
                pSourceInfo->m_pSource->DoResume();
            }
        }
    }
}

void
HXPlayer::SureStreamSourceRegistered(SourceInfo* pSourceInfo)
{
    if (m_nCurrentGroup == pSourceInfo->m_uGroupID)
    {
        m_ulActiveSureStreamSource++;

    /*** Do not turn off fast start if multiple sure-streams present.
         We used to this via the code below as a conservative measure while
         introducing FAST-START (a.k.a. turbo-play).  However, this is overly
         restrictive as any RealVideo stream appears as sure-stream file due
         to on-the fly generation of the key-frame only stream.  Even if this
         code needed to be re-enable, care should be taken it is not done
         for local content as that would place start-up penalty on multi-video
         stream presentations.
        if (m_ulActiveSureStreamSource > 1 && m_bFastStart)
        {
            HXLOGL2(HXLOG_TRAN, "SureStreams > 1 - TurboPlay Off");

            m_bFastStart = FALSE;

            CHXMapPtrToPtr::Iterator ndxSources = m_pSourceMap->Begin();
            for (;ndxSources != m_pSourceMap->End(); ++ndxSources)
            {
                SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
                if (pSourceInfo->m_bIsRegisterSourceDone &&
                    pSourceInfo->m_pSource)
                {
                    pSourceInfo->m_pSource->LeaveFastStart(TP_OFF_BY_MULTISURESTREAMS);
                }
            }
        }
    ***/
    }

    return;
}

void
HXPlayer::SureStreamSourceUnRegistered(SourceInfo* pSourceInfo)
{
    if (m_nCurrentGroup == pSourceInfo->m_uGroupID)
    {
        m_ulActiveSureStreamSource--;
    }

    return;
}

HXBOOL
HXPlayer::CanBeFastStarted(SourceInfo* pSourceInfo)
{
    HXBOOL                bFastStart = TRUE;
#ifdef WIN32_PLATFORM_PSPC
    // TurboPlay only hurts on WM platforms
    HXBOOL                bTurboPlay = FALSE;
#else //WIN32_PLATFORM_PSPC
    HXBOOL                bTurboPlay = TRUE;
#endif //WIN32_PLATFORM_PSPC
    IHXBuffer*          pBuffer    = NULL;
    IHXUpgradeHandler*  pHandler   = NULL;

#if defined(__TCS__)
    m_turboPlayOffReason = TP_OFF_BY_PREFERENCE;
    m_bFastStart = FALSE;
    goto cleanup;
#endif /* __TCS__ */

    // check if the source is within the current group
    if (m_nCurrentGroup != pSourceInfo->m_uGroupID)
    {
        bFastStart = FALSE;

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    UINT16              uNextGroup = 0;
    IHXGroup*           pNextGroup = NULL;
        // we want faststart the next group if we are done with the current one
        if (m_bNextGroupStarted &&
            m_pNextGroupManager &&
            HXR_OK == m_pNextGroupManager->GetCurrentGroup(uNextGroup, pNextGroup))
        {
            if (uNextGroup == pSourceInfo->m_uGroupID)
            {
                bFastStart = TRUE;
            }
        }
        HX_RELEASE(pNextGroup);
#endif /* HELIX_FEATURE_NEXTGROUPMGR */

        goto cleanup;
    }
    else if (!m_bFastStartCheckDone)
    {
        m_bFastStartCheckDone = TRUE;
        m_bFastStart = TRUE;

        // check the preference
        ReadPrefBOOL(m_pPreferences, "TurboPlay", bTurboPlay);
        if (!bTurboPlay)
        {
            HXLOGL2(HXLOG_TRAN, "Preference check - TurboPlay Off");
            m_turboPlayOffReason = TP_OFF_BY_PREFERENCE;
            m_bFastStart = FALSE;
            goto cleanup;
        }

    }
    else if (m_ulActiveSureStreamSource > 1)
    {
        m_turboPlayOffReason = TP_OFF_BY_MULTISURESTREAMS;
        bFastStart = FALSE;
        goto cleanup;
    }

  cleanup:

    HX_RELEASE(pHandler);
    HX_RELEASE(pBuffer);

    return (m_bFastStart & bFastStart);
}

void
HXPlayer::UpdateCurrentPlayTime( ULONG32 ulCurrentPlayTime )
{
    m_ulCurrentPlayTime = ulCurrentPlayTime;
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
    if ( IsMacInCooperativeThread() )
    {
        m_ulCurrentSystemPlayTime = ulCurrentPlayTime;
    }
#endif
}

void
HXPlayer::SendPostSeekIfNecessary(RendererInfo* pRendererInfo)
{
    if (pRendererInfo->m_BufferingReason == BUFFERING_SEEK ||
        pRendererInfo->m_BufferingReason == BUFFERING_LIVE_PAUSE)
    {
        pRendererInfo->m_BufferingReason = BUFFERING_CONGESTION;
    if (pRendererInfo->m_pRenderer)
    {
#if defined(HELIX_FEATURE_DRM)
            //flush the DRM pending packets
            HXSource* pSource = pRendererInfo->m_pStream->GetHXSource();
            if (pSource && pSource->IsHelixDRMProtected() && pSource->GetDRM())
            {
                pSource->GetDRM()->FlushPackets(TRUE);
            }
#endif /* HELIX_FEATURE_DRM */

        pRendererInfo->m_pRenderer->OnPostSeek(
                pRendererInfo->m_pStreamInfo->m_ulTimeBeforeSeek,
                pRendererInfo->m_pStreamInfo->m_ulTimeAfterSeek);
    }

        pRendererInfo->m_pStreamInfo->m_pStream->m_bPostSeekToBeSent = FALSE;
    }
}

HXBOOL
HXPlayer::ScheduleOnTimeSync()
{
    HXBOOL            bResult = FALSE;
    HXBOOL            bDurationTimeSyncAllSent = FALSE;
    ULONG32         ulAudioTime = 0;
    SourceInfo*     pSourceInfo = NULL;
    CHXMapPtrToPtr::Iterator    ndxSource;

    if (m_pAudioPlayer)
    {
        ulAudioTime = m_pAudioPlayer->GetCurrentPlayBackTime();
    }

#if defined(HELIX_FEATURE_NESTEDMETA)
    m_pPersistentComponentManager->OnTimeSync(ulAudioTime);
#endif /* HELIX_FEATURE_NESTEDMETA */

    ndxSource = m_pSourceMap->Begin();
    for (;ndxSource != m_pSourceMap->End(); ++ndxSource)
    {
        pSourceInfo = (SourceInfo*)(*ndxSource);

        if (!DurationTimeSyncAllSent(pSourceInfo))
        {
            pSourceInfo->OnTimeSync( ulAudioTime );

            bDurationTimeSyncAllSent = DurationTimeSyncAllSent(pSourceInfo);

            if (!pSourceInfo->m_bDurationTimeSyncScheduled &&
                !bDurationTimeSyncAllSent)
            {
                pSourceInfo->m_bDurationTimeSyncScheduled = TRUE;
                bResult = TRUE;
                goto cleanup;
            }
        }
    }

cleanup:

    return bResult;
}

HXBOOL
HXPlayer::DurationTimeSyncAllSent(SourceInfo* pSourceInfo)
{
    HXBOOL    bResult = TRUE;

    RendererInfo* pRendInfo = NULL;
    CHXMapLongToObj::Iterator   ndxRend;

    ndxRend = pSourceInfo->m_pRendererMap->Begin();
    for (;ndxRend != pSourceInfo->m_pRendererMap->End();++ndxRend)
    {
        pRendInfo   = (RendererInfo*)(*ndxRend);

        if (!pRendInfo->m_bDurationTimeSyncSent)
        {
            bResult = FALSE;
            break;
        }
    }

    return bResult;
}

void
HXPlayer::DisableScreenSaver()
{
    return;
}

void HXPlayer::RemovePendingCallback(CHXGenericCallback* pCB)
{
    if (pCB && 
        pCB->GetPendingCallback() &&
        m_pScheduler)
    {
        m_pScheduler->Remove(pCB->GetPendingCallback());
        pCB->CallbackCanceled();
    }
}

HX_RESULT HXPlayer::DoVelocityCommand(IHXPlaybackVelocity* pVel, PlaybackVelocityCommandType eCmd)
{
    HX_RESULT retVal = HXR_FAIL;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (pVel)
    {
        switch (eCmd)
        {
            case VelocityCommandInit:
                {
                    // Get our IHXPlaybackVelocityResponse interface
                    IHXPlaybackVelocityResponse* pResp = NULL;
                    retVal = QueryInterface(IID_IHXPlaybackVelocityResponse, (void**) &pResp);
                    if (SUCCEEDED(retVal))
                    {
                        // Init the interface
                        retVal = pVel->InitVelocityControl(pResp);
                    }
                    HX_RELEASE(pResp);
                }
                break;
            case VelocityCommandSetVelocity:
                {
                    retVal = pVel->SetVelocity(m_lPlaybackVelocity, m_bKeyFrameMode, m_bAutoSwitch);
                }
                break;
            case VelocityCommandSetKeyFrameMode:
                {
                    retVal = pVel->SetKeyFrameMode(m_bKeyFrameMode);
                }
                break;
            case VelocityCommandClose:
                {
                    retVal = pVel->CloseVelocityControl();
                }
                break;
            case VelocityCommandQueryCaps:
                {
                    IHXPlaybackVelocityCaps* pCaps = NULL;
                    retVal = pVel->QueryVelocityCaps(pCaps);
                    if (SUCCEEDED(retVal))
                    {
                        // The component gave us a caps interface. Therefore,
                        // we need to logically AND our current caps interface
                        // with this one.
                        if (m_pPlaybackVelocityCaps)
                        {
                            retVal = m_pPlaybackVelocityCaps->CombineCapsLogicalAnd(pCaps);
                        }
                    }
                    else if (retVal == HXR_NOTIMPL)
                    {
                        // This is a special case which means that this
                        // component doesn't change anything with regards
                        // to the velocity. This would be the same as returning
                        // the full range, so the AND would not change anything.
                        // So we can clear the return value.
                        retVal = HXR_OK;
                    }
                    HX_RELEASE(pCaps);
                }
                break;
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

HX_RESULT HXPlayer::PlaybackVelocityCommand(PlaybackVelocityCommandType eCmd)
{
    HX_RESULT retVal = HXR_FAIL;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Do we have a source map?
    if (m_pSourceMap && m_pAudioPlayer && m_pAdviseSink)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Get our own IHXPlaybackVelocity interface
        IHXPlaybackVelocity* pMyVel = NULL;
        QueryInterface(IID_IHXPlaybackVelocity, (void**) &pMyVel);
        // Send command to sources
        CHXMapPtrToPtr::Iterator srcItr = m_pSourceMap->Begin();
        for (; SUCCEEDED(retVal) && srcItr != m_pSourceMap->End(); ++srcItr)
        {
            SourceInfo* pSourceInfo = (SourceInfo*)(*srcItr);
            if (pSourceInfo)
            {
                // Send command to the HXSource
                if (pSourceInfo->m_pSource)
                {
                    IHXPlaybackVelocity* pVel = NULL;
                    retVal = pSourceInfo->m_pSource->QueryInterface(IID_IHXPlaybackVelocity,
                                                                    (void**) &pVel);
                    if (SUCCEEDED(retVal))
                    {
                        retVal = DoVelocityCommand(pVel, eCmd);
                    }
                    HX_RELEASE(pVel);
                }
                   
                // Send command to all renderers
                if (SUCCEEDED(retVal) && pSourceInfo->m_pRendererMap)
                {
                    CHXMapLongToObj::Iterator rendItr = pSourceInfo->m_pRendererMap->Begin();
                    for (; SUCCEEDED(retVal) && rendItr != pSourceInfo->m_pRendererMap->End(); ++rendItr)
                    {
                        RendererInfo* pRendInfo = (RendererInfo*)(*rendItr);
                        if (pRendInfo && pRendInfo->m_pRenderer)
                        {
                            // QI for IHXPlaybackVelocity
                            IHXPlaybackVelocity* pVel = NULL;
                            retVal = pRendInfo->m_pRenderer->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
                            if (SUCCEEDED(retVal))
                            {
                                // Send the command
                                retVal = DoVelocityCommand(pVel, eCmd);
                            }
                            HX_RELEASE(pVel);
                        }
                    }
                }
            }
        }
        HX_RELEASE(pMyVel);

        // Send the command to the audio player
        if (SUCCEEDED(retVal))
        {
            // QI for IHXPlaybackVelocity
            IHXPlaybackVelocity* pVel = NULL;
            retVal = m_pAudioPlayer->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
            if (SUCCEEDED(retVal))
            {
                // Send the command
                retVal = DoVelocityCommand(pVel, eCmd);
            }
            HX_RELEASE(pVel);
        }
        // Send the command to the sink control
        if (SUCCEEDED(retVal))
        {
            // QI for IHXPlaybackVelocity
            IHXPlaybackVelocity* pVel = NULL;
            retVal = m_pAdviseSink->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
            if (SUCCEEDED(retVal))
            {
                // Send the command
                retVal = DoVelocityCommand(pVel, eCmd);
            }
            HX_RELEASE(pVel);
        }
        // Send only SetVelocity commands to the player's event list
        if (SUCCEEDED(retVal) && eCmd == VelocityCommandSetVelocity)
        {
            m_EventList.SetVelocity(m_lPlaybackVelocity);
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

#ifndef _WIN16
#if defined(HELIX_FEATURE_AUTHENTICATION)

_CListOfWrapped_IUnknown_Node::_CListOfWrapped_IUnknown_Node()
  : m_plocPrev(NULL)
  , m_plocNext(NULL)
{
}

_CListOfWrapped_IUnknown_Node::~_CListOfWrapped_IUnknown_Node()
{
    Remove();
}

void
_CListOfWrapped_IUnknown_Node::Remove()
{
    if(m_plocPrev)
    {
        m_plocPrev->next(m_plocNext);
    }

    if(m_plocNext)
    {
        m_plocNext->prev(m_plocPrev);
    }
}

void
_CListOfWrapped_IUnknown_Node::Insert(_CListOfWrapped_IUnknown_Node& rlocnNew)
{
    rlocnNew.next(this);
    rlocnNew.prev(m_plocPrev);

    if(m_plocPrev)
    {
        m_plocPrev->next(&rlocnNew);
    }

    m_plocPrev = &rlocnNew;
}

Wrapped_IUnknown&
_CListOfWrapped_IUnknown_Node::value()
{
    return m_clsValue;
}

const Wrapped_IUnknown&
_CListOfWrapped_IUnknown_Node::value() const
{
    return m_clsValue;
}

void
_CListOfWrapped_IUnknown_Node::value(const Wrapped_IUnknown& rclsNewValue)
{
    m_clsValue = rclsNewValue;
}

_CListOfWrapped_IUnknown_Node&
_CListOfWrapped_IUnknown_Node::operator=(const Wrapped_IUnknown& rclsNewValue)
{
    m_clsValue = rclsNewValue;
    return *this;
}

_CListOfWrapped_IUnknown_Node*
_CListOfWrapped_IUnknown_Node::next() const
{
    return m_plocNext;
}

void
_CListOfWrapped_IUnknown_Node::next(_CListOfWrapped_IUnknown_Node* plocnNew)
{
    m_plocNext = plocnNew;
}

_CListOfWrapped_IUnknown_Node*
_CListOfWrapped_IUnknown_Node::prev() const
{
    return m_plocPrev;
}

void
_CListOfWrapped_IUnknown_Node::prev(_CListOfWrapped_IUnknown_Node* plocnNew)
{
    m_plocPrev = plocnNew;
}

_CListOfWrapped_IUnknown_::_CListOfWrapped_IUnknown_()
{
    m_locnREnd.next(&m_locnEnd);
    m_locnEnd.prev(&m_locnREnd);
}

_CListOfWrapped_IUnknown_::_CListOfWrapped_IUnknown_(const _CListOfWrapped_IUnknown_& rlocOther)
{
    m_locnREnd.next(&m_locnEnd);
    m_locnEnd.prev(&m_locnREnd);

    _copy(rlocOther);
}

_CListOfWrapped_IUnknown_::~_CListOfWrapped_IUnknown_()
{
    empty();
}

_CListOfWrapped_IUnknown_&
_CListOfWrapped_IUnknown_::operator=(const _CListOfWrapped_IUnknown_& rlocOther)
{
    empty();
    _copy(rlocOther);

    return *this;
}

void
_CListOfWrapped_IUnknown_::_copy(const _CListOfWrapped_IUnknown_& rlocOther)
{
    iterator itOther;

    for
    (
        itOther = rlocOther.begin();
        itOther != rlocOther.end();
        ++itOther
    )
    {
        insert(end(), *itOther);
    }
}

_CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::begin()
{
    return iterator(*(m_locnREnd.next()));
}

const _CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::begin() const
{
    return iterator(*(m_locnREnd.next()));
}

_CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::end()
{
    return iterator(m_locnEnd);
}

const _CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::end() const
{
    return iterator(m_locnEnd);
}

_CListOfWrapped_IUnknown_::reverse_iterator
_CListOfWrapped_IUnknown_::rbegin()
{
    return reverse_iterator(*(m_locnEnd.prev()));
}

const _CListOfWrapped_IUnknown_::reverse_iterator
_CListOfWrapped_IUnknown_::rbegin() const
{
    return const_reverse_iterator(*(m_locnEnd.prev()));
}

_CListOfWrapped_IUnknown_::reverse_iterator
_CListOfWrapped_IUnknown_::rend()
{
    return reverse_iterator(m_locnREnd);
}

const _CListOfWrapped_IUnknown_::reverse_iterator
_CListOfWrapped_IUnknown_::rend() const
{
    return const_reverse_iterator(*((const _CListOfWrapped_IUnknown_Node *)&m_locnREnd));
}

_CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::insert(iterator itBefore, const Wrapped_IUnknown& rclsNew)
{
    _CListOfWrapped_IUnknown_Node* plocnNew = new _CListOfWrapped_IUnknown_Node;

    HX_ASSERT(plocnNew);

    *plocnNew = rclsNew;

    itBefore.m_plocCurrent->Insert(*plocnNew);

    return iterator(*plocnNew);
}

void
_CListOfWrapped_IUnknown_::insert
(
    iterator itBefore,
    const iterator itFirst,
    const iterator itLast
)
{
    iterator itOther;
    _CListOfWrapped_IUnknown_Node* plocnNew;

    for (itOther = itFirst; itOther != itLast; ++itOther)
    {
        plocnNew = new _CListOfWrapped_IUnknown_Node;

        HX_ASSERT(plocnNew);

        *plocnNew = *itOther;

        itBefore.m_plocCurrent->Insert(*plocnNew);
    }
}

void
_CListOfWrapped_IUnknown_::remove(iterator itThis)
{
    if
    (
        itThis.m_plocCurrent == &m_locnEnd ||
        itThis.m_plocCurrent == &m_locnREnd
    )
    {
        return;
    }

    _CListOfWrapped_IUnknown_Node* plocnOld;

    plocnOld = itThis.m_plocCurrent;

    ++itThis;

    plocnOld->Remove();

    delete plocnOld;
}

void
_CListOfWrapped_IUnknown_::remove(iterator itFirst, iterator itLast)
{
    if
    (
        itFirst.m_plocCurrent == &m_locnEnd ||
        itFirst.m_plocCurrent == &m_locnREnd
    )
    {
        return;
    }

    iterator itOther;
    _CListOfWrapped_IUnknown_Node* plocnOld;

    for (itOther = itFirst; itOther != itLast;)
    {
        plocnOld = itOther.m_plocCurrent;

        ++itOther;

        plocnOld->Remove();

        delete plocnOld;
    }
}

void
_CListOfWrapped_IUnknown_::empty()
{
    remove(begin(), end());
}

_CListIteratorWrapped_IUnknown_::_CListIteratorWrapped_IUnknown_()
  : m_plocCurrent(NULL)
{
}

_CListIteratorWrapped_IUnknown_::_CListIteratorWrapped_IUnknown_
(
    const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
)
  : m_plocCurrent((_CListOfWrapped_IUnknown_Node*)&rlocnNewLocation)
{
}

_CListIteratorWrapped_IUnknown_::_CListIteratorWrapped_IUnknown_
(
    const _CListIteratorWrapped_IUnknown_& rliocOther
)
  : m_plocCurrent(rliocOther.m_plocCurrent)
{
}

_CListIteratorWrapped_IUnknown_::~_CListIteratorWrapped_IUnknown_()
{
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator=
(
    const _CListIteratorWrapped_IUnknown_& rliocOther
)
{
    m_plocCurrent = rliocOther.m_plocCurrent;

    return *this;
}

Wrapped_IUnknown&
_CListIteratorWrapped_IUnknown_::operator*()
{
    HX_ASSERT(m_plocCurrent);
    return m_plocCurrent->value();
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator=(const Wrapped_IUnknown& rclsNewValue)
{
    if(!m_plocCurrent)
        return *this;

    m_plocCurrent->value(rclsNewValue);

    return *this;
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator++()
{
    if(!m_plocCurrent)
        return *this;

    m_plocCurrent = m_plocCurrent->next();

    return *this;
}

const _CListIteratorWrapped_IUnknown_
_CListIteratorWrapped_IUnknown_::operator++(int)
{
    _CListIteratorWrapped_IUnknown_ liocRet(*this);

    ++(*this);

    return liocRet;
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator--()
{
    if(!m_plocCurrent)
        return *this;

    m_plocCurrent = m_plocCurrent->prev();

    return *this;
}

const _CListIteratorWrapped_IUnknown_
_CListIteratorWrapped_IUnknown_::operator--(int)
{
    _CListIteratorWrapped_IUnknown_ liocRet(*this);

    --(*this);

    return liocRet;
}

HXBOOL operator==
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
)
{
    return (rliocLeft.m_plocCurrent == rliocRight.m_plocCurrent);
}

HXBOOL operator!=
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
)
{
    return (rliocLeft.m_plocCurrent != rliocRight.m_plocCurrent);
}

_CListReverseIteratorWrapped_IUnknown_::_CListReverseIteratorWrapped_IUnknown_()
  : m_plocCurrent(NULL)
{
}

_CListReverseIteratorWrapped_IUnknown_::_CListReverseIteratorWrapped_IUnknown_
(
    const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
)
  : m_plocCurrent((_CListOfWrapped_IUnknown_Node*)&rlocnNewLocation)
{
}

_CListReverseIteratorWrapped_IUnknown_::_CListReverseIteratorWrapped_IUnknown_
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
)
  : m_plocCurrent(rlriocOther.m_plocCurrent)
{
}

_CListReverseIteratorWrapped_IUnknown_::~_CListReverseIteratorWrapped_IUnknown_()
{
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator=
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
)
{
    m_plocCurrent = rlriocOther.m_plocCurrent;
    return *this;
}

Wrapped_IUnknown&
_CListReverseIteratorWrapped_IUnknown_::operator*()
{
    HX_ASSERT(m_plocCurrent);
    return m_plocCurrent->value();
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator=(const Wrapped_IUnknown& rclsNewValue)
{
    if(!m_plocCurrent)
        return *this;

    m_plocCurrent->value(rclsNewValue);

    return *this;
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator++()
{
    if(!m_plocCurrent)
        return *this;

    m_plocCurrent = m_plocCurrent->prev();

    return *this;
}

const _CListReverseIteratorWrapped_IUnknown_
_CListReverseIteratorWrapped_IUnknown_::operator++(int)
{
    _CListReverseIteratorWrapped_IUnknown_ lriocRet(*this);

    ++(*this);

    return lriocRet;
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator--()
{
    if(!m_plocCurrent)
        return *this;

    m_plocCurrent = m_plocCurrent->next();

    return *this;
}

const _CListReverseIteratorWrapped_IUnknown_
_CListReverseIteratorWrapped_IUnknown_::operator--(int)
{
    _CListReverseIteratorWrapped_IUnknown_ lriocRet(*this);

    --(*this);

    return lriocRet;
}

HXBOOL operator==
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
)
{
    return (rlriocLeft.m_plocCurrent == rlriocRight.m_plocCurrent);
}

HXBOOL operator!=
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
)
{
    return (rlriocLeft.m_plocCurrent != rlriocRight.m_plocCurrent);
}
#endif /* HELIX_FEATURE_AUTHENTICATION */

_CHXAuthenticationRequests::_CHXAuthenticationRequests(IUnknown* pContext)
    : m_pMutexProtectList(NULL)
    , m_bUIShowing(FALSE)
{
#ifdef HELIX_FEATURE_AUTHENTICATION
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutexProtectList, pContext);  
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

_CHXAuthenticationRequests::~_CHXAuthenticationRequests()
{
#ifdef HELIX_FEATURE_AUTHENTICATION
    HX_RELEASE(m_pMutexProtectList);
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

HX_RESULT
_CHXAuthenticationRequests::Add(HXPlayer* pPlayerRequester,
        IHXAuthenticationManagerResponse* pAuthenticationManagerResponseRequester,
        IHXValues* pAuthenticationHeaderValues)
{
#ifdef HELIX_FEATURE_AUTHENTICATION
    HXBOOL bShowUI = FALSE;
    IHXAuthenticationManager* pAuthenticationManagerClient = NULL;
    IHXAuthenticationManager2* pAuthenticationManagerClient2 = NULL;

    pPlayerRequester->m_pClient->QueryInterface(IID_IHXAuthenticationManager2, (void**)&pAuthenticationManagerClient2);

    if (pAuthenticationManagerClient2)
    {
        HX_ASSERT(pPlayerRequester);
        if (pAuthenticationManagerClient2 == (IHXAuthenticationManager2*)pPlayerRequester)
        {
            HX_RELEASE(pAuthenticationManagerClient2);
            pAuthenticationManagerClient2 = NULL;
        }
    }

    if (!pAuthenticationManagerClient2)
    {

        pPlayerRequester->m_pClient->QueryInterface
        (
            IID_IHXAuthenticationManager,
            (void**)&pAuthenticationManagerClient
        );

    }

    if (!pAuthenticationManagerClient && !pAuthenticationManagerClient2)
    {
        return pAuthenticationManagerResponseRequester->AuthenticationRequestDone
        (
            HXR_UNEXPECTED,
            NULL,
            NULL
        );
    }

    HX_ASSERT(pAuthenticationManagerClient || pAuthenticationManagerClient2); // we want one...
    HX_ASSERT(!(pAuthenticationManagerClient && pAuthenticationManagerClient2)); // .. but not both

    // Don't allow iterating while we are adding.
    m_pMutexProtectList->Lock();

    m_ListOfIUnknownRequesters.insert(m_ListOfIUnknownRequesters.end(), (IUnknown*)pAuthenticationManagerResponseRequester);

    if (!m_bUIShowing)
    {
        bShowUI = m_bUIShowing = TRUE;
    }

    m_pMutexProtectList->Unlock();

    if (bShowUI)
    {
        IHXInterruptSafe* pInterruptSafe = NULL;
        if (pAuthenticationManagerClient) pAuthenticationManagerClient->QueryInterface(IID_IHXInterruptSafe,
                                                     (void**) &pInterruptSafe);
        if (pAuthenticationManagerClient2) pAuthenticationManagerClient2->QueryInterface(IID_IHXInterruptSafe,
                                                        (void**) &pInterruptSafe);

        if (!pPlayerRequester->m_pEngine->AtInterruptTime() ||
                      (pInterruptSafe && pInterruptSafe->IsInterruptSafe()))
        {
            /* Remove any pending callback */
            pPlayerRequester->RemovePendingCallback(pPlayerRequester->m_pAuthenticationCallback);

            if (pAuthenticationManagerClient)
            {
                pAuthenticationManagerClient->HandleAuthenticationRequest
                            (
                                pPlayerRequester
                            );
            }

            if (pAuthenticationManagerClient2)
            {
                pAuthenticationManagerClient2->HandleAuthenticationRequest2
                            (
                                pPlayerRequester,
                                pAuthenticationHeaderValues
                            );
            }
        }
        else
        {
            m_bUIShowing = FALSE;

            /*
             * Schedule a callback to request authentication at
             * non-interrupt time
             */
            if (pPlayerRequester->m_pAuthenticationCallback &&
                !pPlayerRequester->m_pAuthenticationCallback->GetPendingCallback())
            {
                pPlayerRequester->m_pAuthenticationCallback->CallbackScheduled(
                    pPlayerRequester->m_pScheduler->RelativeEnter(pPlayerRequester->m_pAuthenticationCallback, 0));
            }
        }

        HX_RELEASE(pInterruptSafe);
    }

    HX_RELEASE(pAuthenticationManagerClient);
    HX_RELEASE(pAuthenticationManagerClient2);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

HX_RESULT
_CHXAuthenticationRequests::SatisfyPending
(
    HX_RESULT ResultStatus,
    const char* pCharUser,
    const char* pCharPassword
)
{
#ifdef HELIX_FEATURE_AUTHENTICATION
    _CListOfWrapped_IUnknown_::iterator ListOfIUnknownIteratorCurrentRequester;
    IUnknown* pUnknownRequester = NULL;
    IHXAuthenticationManagerResponse* pAuthenticationManagerResponseRequester = NULL;

    // Don't allow add's while we are iterating.
    m_pMutexProtectList->Lock();

    m_bUIShowing = FALSE;

    for
    (
        ListOfIUnknownIteratorCurrentRequester = m_ListOfIUnknownRequesters.begin();
        ListOfIUnknownIteratorCurrentRequester != m_ListOfIUnknownRequesters.end();
        ++ListOfIUnknownIteratorCurrentRequester
    )
    {
        pUnknownRequester = (*ListOfIUnknownIteratorCurrentRequester).wrapped_ptr();

        if (pUnknownRequester)
        {
            pUnknownRequester->QueryInterface
            (
                IID_IHXAuthenticationManagerResponse,
                (void**)&pAuthenticationManagerResponseRequester
            );

            if (pAuthenticationManagerResponseRequester)
            {
                pAuthenticationManagerResponseRequester->AuthenticationRequestDone
                (
                    ResultStatus,
                    pCharUser,
                    pCharPassword
                );
            }
        }

        HX_RELEASE(pAuthenticationManagerResponseRequester);
        pUnknownRequester = NULL;
    }
    m_ListOfIUnknownRequesters.empty();

    m_pMutexProtectList->Unlock();

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

void
_CHXAuthenticationRequests::ClearPendingList()
{
#ifdef HELIX_FEATURE_AUTHENTICATION
    m_ListOfIUnknownRequesters.empty();
#endif /* HELIX_FEATURE_AUTHENTICATION */
}
#endif /* _WIN16 */

HX_RESULT
HXPlayer::AdjustSeekOnRepeatedSource(SourceInfo*   pSourceInfo,
                                      UINT32        ulSeekTime)
{
#if defined(HELIX_FEATURE_SMIL_REPEAT)
    HX_RESULT   theErr = HXR_OK;
    SourceInfo* pAdjustedSourceInfo = NULL;

    pAdjustedSourceInfo = pSourceInfo->DoAdjustSeek(ulSeekTime);

    if (pAdjustedSourceInfo == pSourceInfo)
    {
        theErr = pSourceInfo->m_pSource->DoSeek(ulSeekTime);
    }
    else
    {
        m_pSourceMap->RemoveKey(pSourceInfo->m_pSource);
        m_pSourceMap->SetAt((void*)pAdjustedSourceInfo->m_pSource,
                          (void*)pAdjustedSourceInfo);

        m_bSourceMapUpdated = TRUE;
        m_bForceStatsUpdate = TRUE;

        if (pAdjustedSourceInfo->m_bTobeInitializedBeforeBegin)
        {
            m_uNumSourceToBeInitializedBeforeBegin++;
        }
    }

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_SMIL_REPEAT */
}

HXBOOL
HXPlayer::IsSitePresent
(
    IHXSite*                pSite
)
{
#if defined(HELIX_FEATURE_VIDEO)
    return m_pSiteManager->IsSitePresent(pSite);
#else
    return FALSE;
#endif /* HELIX_FEATURE_VIDEO */
}

void
HXPlayer::CheckIfRendererNeedFocus(IUnknown* pRenderer)
{
#if defined(HELIX_FEATURE_VIDEO)
    HXBOOL bNeedFocus = FALSE;
    ReadPrefBOOL(m_pPreferences, "GrabFocus", bNeedFocus);

    IHXRenderer* pRend = NULL;
    pRenderer->QueryInterface(IID_IHXRenderer, (void**) &pRend);
    if (pRend)
    {
        UINT32 ulGran = 0;
        const char**    ppszMimeTypes = NULL;
        pRend->GetRendererInfo(ppszMimeTypes, ulGran);
        while (ppszMimeTypes && *ppszMimeTypes)
        {
            // XXRA Hernry, please remove the GrabFocus preference code from above
            // when you add the right mimetypes
            if ((::strcasecmp(*ppszMimeTypes, "MIMETYPE1") == 0) ||
                (::strcasecmp(*ppszMimeTypes, "MIMETYPE2") == 0))
            {
                bNeedFocus = TRUE;
                goto rendexit;
            }

            ppszMimeTypes++;
        }
    }

rendexit:
    HX_RELEASE(pRend);

    if (m_pSiteManager && bNeedFocus)
    {
        m_pSiteManager->NeedFocus(TRUE);
    }
#endif /* HELIX_FEATURE_VIDEO */
}

UINT32 HXPlayer::ComputeFillEndTime(UINT32 ulCurTime, UINT32 ulOffsetScale, UINT32 ulOffsetFixed)
{
    UINT32 ulRet = 0;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    UINT32 ulOffset = ulOffsetScale + ulOffsetFixed;
    if (HX_IS_ACCELERATED_PLAYBACK(m_lPlaybackVelocity))
    {
        // Get the absolute value of the velocity
        UINT32 ulAbsVelocity = (UINT32) HX_ABSVAL(m_lPlaybackVelocity);
        // Scale the time offset by the velocity
        ulOffset = HX_SCALE_BY_VELOCITY(ulOffsetScale, ulAbsVelocity);
        // Add in the fixed offset (which doesn't scale with velocity)
        ulOffset += ulOffsetFixed;
        // Are we in keyframe mode?
        if (m_bKeyFrameMode)
        {
            // If we are in keyframe mode, we try to stay a
            // certain number of keyframe intervals ahead.
            UINT32 ulNumKeyFramesAhead = (m_lPlaybackVelocity < 0 ?
                                          m_ulNumReverseKeyFramesAhead :
                                          m_ulNumForwardKeyFramesAhead);
            UINT32 ulKeyFrameOffset    = ulNumKeyFramesAhead * m_ulKeyFrameInterval;
            // Take the bigger of the offsets
            if (ulKeyFrameOffset > ulOffset)
            {
                ulOffset = ulKeyFrameOffset;
            }
        }
    }
    ulRet = HX_ADD_USING_VELOCITY(ulCurTime, ulOffset, m_lPlaybackVelocity);
#else /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    ulRet = ulCurTime + ulOffsetScale + ulOffsetFixed;
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

#ifdef _MACINTOSH
#define ADDITIONAL_PREDELIVERY_TIME 4000
    /* on Mac we try to be even farther ahead than the timeline in
     * packet delivery since the callbacks are not that smooth
     * and if we are really close to the wire, it results in
     * unnecccessary re-buffers. 
     */ 
    ulRet += ADDITIONAL_PREDELIVERY_TIME;
#endif  // _MACINTOSH

    return ulRet;
}

void HXPlayer::CheckForPacketTimeOffsetUpdate(RendererInfo* pRendInfo)
{
    if (pRendInfo && pRendInfo->m_pStream && pRendInfo->m_pRenderer)
    {
        // Get the HXSource
        HXSource* pSource = pRendInfo->m_pStream->GetHXSource();
        if (pSource)
        {
            // Compute the timeline offset
            INT32 lOffset = pSource->GetStartTime() - pSource->GetDelay();
            // Is the source a packetless source?
            // We only need to do this for packetless sources
            // since sources with packets will get their offset via
            // the OnPacket() call.
            if (pSource->IsPacketlessSource() && lOffset)
            {
                // Does the IHXRenderer support IHXUpdateProperties
                IHXUpdateProperties* pProps = NULL;
                pRendInfo->m_pRenderer->QueryInterface(IID_IHXUpdateProperties, (void**) &pProps);
                if (pProps)
                {
                    // Update the packet time offset
                    pProps->UpdatePacketTimeOffset(lOffset);
                }
                HX_RELEASE(pProps);
            }
        }
        HX_RELEASE(pSource);
    }
}

#if defined(HELIX_FEATURE_RECORDCONTROL)
HXBOOL HXPlayer::IsRecordServiceEnabled()
{
    HXBOOL bRes = FALSE;

    ReadPrefBOOL(m_pPreferences, "LiveSuperBuffer", bRes);

    return bRes;
}
#endif

STDMETHODIMP
HXPlayer::LoadRecordService(IHXRecordService* pRecordService)
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    HX_RELEASE(m_pRecordService);
    
    m_pRecordService = pRecordService;
    HX_ADDREF(m_pRecordService);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_RECORDCONTROL */
}

STDMETHODIMP
HXPlayer::GetRecordService(REF(IHXRecordService*) pRecordService)
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    pRecordService = m_pRecordService;
    if(pRecordService)
    {
        pRecordService->AddRef();
        return HXR_OK;
    }

    return HXR_FAILED;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_RECORDCONTROL */
}

STDMETHODIMP
HXPlayer::UnloadRecordService()
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    HX_RELEASE(m_pRecordService);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_RECORDCONTROL */
}

STDMETHODIMP HXPlayer::InitVelocityControl(IHXPlaybackVelocityResponse* pResponse)
{
    HX_RESULT retVal = HXR_OK;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // It's OK if no response interface is provided. It just means
    // that the user won't get asynch notification if conditions change.
    if (pResponse)
    {
        HX_RELEASE(m_pPlaybackVelocityResponse);
        m_pPlaybackVelocityResponse = pResponse;
        m_pPlaybackVelocityResponse->AddRef();
    }
    // Now inform all the components involved (fileformat, renderer)
    // of the Init
    retVal = PlaybackVelocityCommand(VelocityCommandInit);
    if (SUCCEEDED(retVal))
    {
        m_bVelocityControlInitialized = TRUE;
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

STDMETHODIMP HXPlayer::QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    HXLOGL3(HXLOG_TRIK, "HXPlayer QueryVelocityCaps()");
    HX_RESULT retVal = HXR_OK;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // If we have not yet been initialized, then
    // initialize ourselves with a NULL response interface
    if (!m_bVelocityControlInitialized)
    {
        retVal = InitVelocityControl(NULL);
    }
    if (SUCCEEDED(retVal))
    {
        // Create an initial full range caps object
        HX_RELEASE(m_pPlaybackVelocityCaps);
        retVal = CHXPlaybackVelocityCaps::CreateFullRangeCaps(m_pPlaybackVelocityCaps);
        if (SUCCEEDED(retVal))
        {
            // Query all the components to find capabilities
            retVal = PlaybackVelocityCommand(VelocityCommandQueryCaps);
            // If this fails, then we can only play at 1x
            if (FAILED(retVal))
            {
                HX_RELEASE(m_pPlaybackVelocityCaps);
                retVal = CHXPlaybackVelocityCaps::Create1xOnlyCaps(m_pPlaybackVelocityCaps);
            }
            if (SUCCEEDED(retVal))
            {
                // Assign the out parameter
                HX_RELEASE(rpCaps);
                rpCaps = m_pPlaybackVelocityCaps;
                rpCaps->AddRef();
            }
        }
    }
#else
    retVal = HXR_FAIL;
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    HXLOGL2(HXLOG_TRIK, "HXPlayer QueryVelocityCaps() returns 0x%08x", retVal);
    if (SUCCEEDED(retVal) && rpCaps)
    {
        UINT32 ulNumRanges = rpCaps->GetNumRanges();
        HXLOGL2(HXLOG_TRIK, "\tNum Ranges: %lu", ulNumRanges);
        for (UINT32 i = 0; i < ulNumRanges; i++)
        {
            INT32 lMin = 0;
            INT32 lMax = 0;
            HX_RESULT rv = rpCaps->GetRange(i, lMin, lMax);
            if (SUCCEEDED(rv))
            {
                HXLOGL3(HXLOG_TRIK, "\t\tRange %lu (min=%ld,max=%ld)", i, lMin, lMax);
            }
        }
    }

    return retVal;
}

STDMETHODIMP HXPlayer::SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    HXLOGL1(HXLOG_TRIK, "HXPlayer SetVelocity(velocity=%ld,keyframemode=%lu,autoswitch=%lu)",
            lVelocity, bKeyFrameMode, bAutoSwitch);
    HX_RESULT retVal = HXR_FAIL;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Enforce mininum velocity
    if (lVelocity >= HX_PLAYBACK_VELOCITY_MIN &&
        lVelocity <= HX_PLAYBACK_VELOCITY_MAX)
    {
    if (m_eClientState < HX_CLIENT_STATE_OPENED)
    {
            HXLOGL1(HXLOG_TRIK, "HXPlayer SetVelocity(velocity=%ld,keyframemode=%lu,autoswitch=%lu) Caching until client state is OPENED",
                lVelocity, bKeyFrameMode, bAutoSwitch);
        m_bPlaybackVelocityCached = TRUE;
        m_lPlaybackVelocityCached = lVelocity;
        m_bKeyFrameModeCached = bKeyFrameMode;
        m_bAutoSwitchCached = bAutoSwitch;
        return HXR_OK;
    }

        // Clear the return value
        retVal = HXR_OK;
        // If we have not yet been initialized, then
        // initialize ourselves with a NULL response interface
        if (!m_bVelocityControlInitialized)
        {
            retVal = InitVelocityControl(NULL);
        }
        // Is this new velocity different than our current velocity?
        if (m_lPlaybackVelocity != lVelocity)
        {
            // Do we have a velocity caps object?
            if (!m_pPlaybackVelocityCaps)
            {
                IHXPlaybackVelocityCaps* pDummy = NULL;
                retVal = QueryVelocityCaps(pDummy);
                HX_RELEASE(pDummy);
            }
            if (SUCCEEDED(retVal))
            {
                // Check the capability object to make sure
                // we can play this velocity
                if (m_pPlaybackVelocityCaps->IsCapable(lVelocity))
                {
                    // Set the flag saying a SetVelocity is in progress.
                    // Since a SetVelocity causes an internal pause, seek,
                    // and resume, then we will reset this flag when we
                    // receive the first OnTimeSync after resuming.
                    m_bSetVelocityInProgress = TRUE;
                    // Disable the OnPreSeek, OnPostSeek, OnBuffering,
                    // and OnPosLen calls in the advise sink. We don't disable
                    // OnPause and OnBegin here since we could get a SetVelocity
                    // while we are paused.
                    m_pAdviseSink->DisableAdviseSink(HX_ADVISE_SINK_FLAG_ONPOSLENGTH |
                                                     HX_ADVISE_SINK_FLAG_ONPRESEEK   |
                                                     HX_ADVISE_SINK_FLAG_ONPOSTSEEK  |
                                                     HX_ADVISE_SINK_FLAG_ONBUFFERING);
            // Do not disable if in the paused state, otherwise we will miss some state transitions waiting for TimeSync
            if (m_bIsPlaying)
            {
                        m_pClientStateAdviseSink->DisableClientStateAdviseSink(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE);
            }
                    // Get the current time
                    UINT32 ulTime = GetCurrentPlayTime();
                    // Get the IID_IHXPlaybackVelocityTimeRegulator interface from
                    // the advise sink
                    UINT32 ulOrigTime = ulTime;
                    if (m_pAdviseSink)
                    {
                        IHXPlaybackVelocityTimeRegulator* pRegulator = NULL;
                        m_pAdviseSink->QueryInterface(IID_IHXPlaybackVelocityTimeRegulator, (void**) &pRegulator);
                        if (pRegulator)
                        {
                            ulOrigTime = pRegulator->GetOriginalTime(ulTime);
                        }
                        HX_RELEASE(pRegulator);
                    }
                    // Save the old velocity, in case this call fails
                    INT32 lOldVelocity = m_lPlaybackVelocity;
                    // Is the player currently playing?
                    HXBOOL bPlaying = m_bIsPlaying;
                    // If we are playing, then we need to pause the player
                    if (bPlaying)
                    {
                        // Get the core mutex
                        m_pCoreMutex->Lock();
            m_ulCoreLockCount++;
                        // Disable the advise sink OnPause call
                        m_pAdviseSink->DisableAdviseSink(HX_ADVISE_SINK_FLAG_ONPAUSE);
                        // Pause the player
                        PausePlayer();
                        // Re-enable the advise sink OnPause call
                        m_pAdviseSink->EnableAdviseSink(HX_ADVISE_SINK_FLAG_ONPAUSE);
                        // Release the core mutex
            m_ulCoreLockCount--;
                        m_pCoreMutex->Unlock();
                    }
                    // Copy the new velocity to the member variable
                    m_lPlaybackVelocity = lVelocity;
                    // Copy the keyframe mode and autoswitch members,
                    // but force keyframe mode to TRUE for reverse playback
                    m_bKeyFrameMode = (lVelocity < 0 ? TRUE : bKeyFrameMode);
                    m_bAutoSwitch   = bAutoSwitch;
                    // Do the SetVelocity command
                    retVal = PlaybackVelocityCommand(VelocityCommandSetVelocity);
                    if (SUCCEEDED(retVal))
                    {
                        // Get the core mutex
                        m_pCoreMutex->Lock();
            m_ulCoreLockCount++;
                        // Seek the player
                        SeekPlayer(ulOrigTime);
                        // If we were playing when we got the SetVelocity,
                        // then we need to restart the player
                        if (bPlaying)
                        {
                            // Disable the advise sink OnBegin call
                            m_pAdviseSink->DisableAdviseSink(HX_ADVISE_SINK_FLAG_ONBEGIN);
                            // Restart the player
                            BeginPlayer();
                            // Re-enable the advise sink OnBegin call
                            m_pAdviseSink->EnableAdviseSink(HX_ADVISE_SINK_FLAG_ONBEGIN);
                        }
                        // Release the core mutex
            m_ulCoreLockCount--;
                        m_pCoreMutex->Unlock();
                    }
                    else
                    {
                        // This failed, so reset the playback velocity
                        m_lPlaybackVelocity = lOldVelocity;
                        // This should reset any components that successfully
                        // had their velocites set
                        PlaybackVelocityCommand(VelocityCommandSetVelocity);
                    }
                }
                else
                {
                    // We can't accelerate to this velocity
                    retVal = HXR_FAIL;
                }
                if (SUCCEEDED(retVal))
                {
                    // Callback to the response interface
                    UpdateVelocity(m_lPlaybackVelocity);
                }
            }
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

STDMETHODIMP_(INT32) HXPlayer::GetVelocity()
{
    return m_lPlaybackVelocity;
}

STDMETHODIMP HXPlayer::SetKeyFrameMode(HXBOOL bKeyFrameMode)
{

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    HX_RESULT retVal = HXR_OK;
    // Is this keyframe mode different than current?
    if (bKeyFrameMode != m_bKeyFrameMode)
    {
        // If we have not yet been initialized, then
        // initialize ourselves with a NULL response interface
        if (!m_bVelocityControlInitialized)
        {
            retVal = InitVelocityControl(NULL);
        }
        // Do we have a velocity caps object?
        if (!m_pPlaybackVelocityCaps)
        {
            IHXPlaybackVelocityCaps* pDummy = NULL;
            retVal = QueryVelocityCaps(pDummy);
            HX_RELEASE(pDummy);
        }
        // Save the old keyframe mode
        HXBOOL bOldKeyFrameMode = m_bKeyFrameMode;
        // Save the keyframe mode
        m_bKeyFrameMode = bKeyFrameMode;
        // Inform all the components
        retVal = PlaybackVelocityCommand(VelocityCommandSetKeyFrameMode);
        if (FAILED(retVal))
        {
            // Changing one of the components failed, so reset
            m_bKeyFrameMode = bOldKeyFrameMode;
            // Now call again to flip back any components that succeeded
            PlaybackVelocityCommand(VelocityCommandSetKeyFrameMode);
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) HXPlayer::GetKeyFrameMode()
{
    return m_bKeyFrameMode;
}

STDMETHODIMP_(UINT32) HXPlayer::GetKeyFrameInterval()
{
    return m_ulKeyFrameInterval;
}

STDMETHODIMP HXPlayer::CloseVelocityControl()
{
    HX_RESULT retVal = HXR_OK;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Reset playback velocity
    m_lPlaybackVelocity = HX_PLAYBACK_VELOCITY_NORMAL;
    // Release the response interface
    HX_RELEASE(m_pPlaybackVelocityResponse);
    // Release the capabilities object
    HX_RELEASE(m_pPlaybackVelocityCaps);
    // Close all the components
    PlaybackVelocityCommand(VelocityCommandClose);
    // Reset the initialized flag
    m_bVelocityControlInitialized = FALSE;
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

STDMETHODIMP HXPlayer::UpdateVelocityCaps(IHXPlaybackVelocityCaps* pCaps)
{
    return HXR_OK;
}

STDMETHODIMP HXPlayer::UpdateVelocity(INT32 lVelocity)
{
    if (m_pAdviseSink)
    {
        m_pAdviseSink->UpdateVelocity(lVelocity);
    }
    return HXR_OK;
}

STDMETHODIMP HXPlayer::UpdateKeyFrameMode(HXBOOL bKeyFrameMode)
{
    HX_RESULT retVal = SetKeyFrameMode(bKeyFrameMode);
    if (SUCCEEDED(retVal) && m_pAdviseSink)
    {
        m_pAdviseSink->UpdateKeyFrameMode(bKeyFrameMode);
    }
    return retVal;
}

	// IHXFrameStep methods
#if defined(HELIX_FEATURE_VIDEO_FRAME_STEP)
STDMETHODIMP HXPlayer::StepFrames(INT32 lSteps)
{
    HX_RESULT retVal = HXR_FAIL;
	HXBOOL bIsRendererFound = FALSE;
	// Do we have a source map?
	if (m_pSourceMap && m_pAdviseSink)
	{
		retVal = HXR_OK;
		// Send command to sources
		CHXMapPtrToPtr::Iterator srcItr = m_pSourceMap->Begin();
		for (; SUCCEEDED(retVal) && srcItr != m_pSourceMap->End() && !bIsRendererFound; ++srcItr)
		{
			SourceInfo* pSourceInfo = (SourceInfo*)(*srcItr);
			if (pSourceInfo)
			{
				// Send command to all renderers
				if (SUCCEEDED(retVal) && pSourceInfo->m_pRendererMap)
				{
					CHXMapLongToObj::Iterator rendItr = pSourceInfo->m_pRendererMap->Begin();
					for (; rendItr != pSourceInfo->m_pRendererMap->End() && !bIsRendererFound; ++rendItr)
					{
						RendererInfo* pRendInfo = (RendererInfo*)(*rendItr);
						if (pRendInfo && pRendInfo->m_pRenderer)
						{
							// QI for IHXFrameStep
							IHXFrameStep* pVel = NULL;
							retVal = pRendInfo->m_pRenderer->QueryInterface(IID_IHXFrameStep, (void**) &pVel);
							if (SUCCEEDED(retVal))
							{
								// Send the command
								retVal = pVel->StepFrames(lSteps);
								bIsRendererFound = TRUE;
							}
							HX_RELEASE(pVel);
						}
					}
				}
			}
		}
	}
	return retVal;
}
#endif  //HELIX_FEATURE_VIDEO_FRAME_STEP

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
/*
 *  Helper methods that are used by IHXPDStatusMgr methods, below
 */        
HX_RESULT
HXPlayer::EstablishPDSObserverList()
{
    HX_RESULT  hxrslt = HXR_OK;

    if (!m_pPDSObserverList)
    {
        m_pPDSObserverList = new CHXSimpleList();

        if (!m_pPDSObserverList)
        {
            hxrslt = HXR_OUTOFMEMORY;
        }
    }

    return hxrslt;
}

/*
 *  IHXPDStatusMgr methods
 */        

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::AddObserver
 *  Purpose:
 *      Lets an observer register so it can be notified of file changes
 */
STDMETHODIMP
HXPlayer::AddObserver(IHXPDStatusObserver* /*IN*/ pObserver)
{
    HX_RESULT hxrslt = HXR_INVALID_PARAMETER;
    if (pObserver)
    {
        if (!m_pPDSObserverList)
        {
            hxrslt = EstablishPDSObserverList();
        }
        if (SUCCEEDED(hxrslt)  &&  m_pPDSObserverList)
        {
            pObserver->AddRef();
            m_pPDSObserverList->AddTail(pObserver);
        }
    }

    return hxrslt;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::RemoveObserver
 *  Purpose:
 *      Lets an observer unregister so it can stop being notified of
 *      file changes
 */
STDMETHODIMP
HXPlayer::RemoveObserver(IHXPDStatusObserver* /*IN*/ pObserver)
{
    HX_RESULT hxrslt = HXR_INVALID_PARAMETER;
    if (pObserver)
    {
        hxrslt = HXR_FAIL;
        if (m_pPDSObserverList)
        {
            LISTPOSITION lPosition = m_pPDSObserverList->Find(pObserver);
            if (lPosition)
            {
                m_pPDSObserverList->RemoveAt(lPosition);
                HX_RELEASE(pObserver);
            }

            hxrslt = HXR_OK;
        }
    }
    return hxrslt;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::SetStatusUpdateGranularityMsec
 *  Purpose:
 *      Lets an observer set the interval that the reporter (fsys) takes
 *      between status updates.  Value is in milliseconds.
 */
STDMETHODIMP
HXPlayer::SetStatusUpdateGranularityMsec(
        UINT32 /*IN*/ ulStatusUpdateGranularityInMsec)
{
    HX_RESULT hxrslt = HXR_FAIL;
    if (m_pPDStatusMgr)
    {
        hxrslt = m_pPDStatusMgr->SetStatusUpdateGranularityMsec(
                ulStatusUpdateGranularityInMsec);
    }

    return hxrslt;
}

/*
 *  IHXPDStatusObserver methods
 */

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadProgress
 *
 *  Purpose:
 *      Notification from IHXPDStatusMgr of download progress when
 *      file size changes.
 *
 *      lTimeSurplus:
 *      - When negative, the absolute value of it is the estimated number
 *      of milliseconds of wall-clock time that need to pass while
 *      downloading continues before reaching the point at which playback
 *      can resume and play the remainder of the stream without having to
 *      buffer, assuming that playback is paused and remains so during
 *      that period.
 *      - When positive, it is the estimated number of milliseconds of
 *      wall-clock time between when the download should complete and when
 *      the natural content play-out duration will be reached, assuming
 *      playback is currently progressing and that no pause will occur.
 *
 *      Note: ulNewDurSoFar can be HX_PROGDOWNLD_UNKNOWN_DURATION if the
 *      IHXMediaBytesToMediaDur was not available to, or was unable to
 *      convert the bytes to a duration for the IHXPDStatusMgr calling this:
 */
STDMETHODIMP
HXPlayer::OnDownloadProgress(
             IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
             UINT32 /*IN*/ ulNewDurSoFar,
             UINT32 /*IN*/ ulNewBytesSoFar,
             INT32  /*IN*/ lTimeSurplus)
{
    HX_RESULT hxrslt = HXR_OK;
    CProgDnldStatusRptInfo* pPDSRInfoCur = NULL;
    // /Keep track of info reported to us here back to
    // HX_PROGDOWNLD_DNLD_RATE_MOVING_WINDOW_SZ in the past so we can
    // calculate the download rate based on most-recent trend:
    if (!m_pProgDnldStatusReportInfoList)
    {
        m_pProgDnldStatusReportInfoList = new CHXSimpleList();
    }
    if (m_pProgDnldStatusReportInfoList)
    {
        pPDSRInfoCur = new CProgDnldStatusRptInfo(ulNewDurSoFar,
                m_ulTimeOfOpenURL);
        if (!pPDSRInfoCur)
        {
            hxrslt = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        hxrslt = HXR_OUTOFMEMORY;
    }
    if (m_pPDSObserverList  &&  HXR_OK == hxrslt)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                UINT32 ulCurPlayTime = GetCurrentPlayTime();
                
                if (HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS == lTimeSurplus  &&
                        HX_PROGDOWNLD_UNKNOWN_DURATION !=
                        m_ulTotalDurReported  &&
                        HX_PROGDOWNLD_UNKNOWN_DURATION !=
                        ulNewDurSoFar  &&
                        ulNewDurSoFar > 0)
                {
                    // /Calculate timeSurplus: will be negative if
                    // data is downloading slower than bitrate, positive
                    // if download rate is greater than bitrate:
                    UINT32 ulTimeSinceOpenURL = CALCULATE_ELAPSED_TICKS(
                            m_ulTimeOfOpenURL, HX_GET_TICKCOUNT());

                    // /Now, go through the download-status-report list to
                    // get all the durs reported in the last n seconds
                    // (moving window).  If there is no reported dur in the
                    // first (oldest) half of the window, keep the one prior
                    // to the start of the window and use it:
                    CProgDnldStatusRptInfo* pPDSRInfo1 = NULL;
                    CProgDnldStatusRptInfo* pPDSRInfo2 = NULL;
                    UINT32 ulTimeSinceEarliestInfo = ulTimeSinceOpenURL;
                    // /Init it to same as current dur so that way we will
                    // calculate 0 download rate and thus report MIN_SURPLUS
                    // if we have not received prior download info:
                    // /To avoid reporting MIN surplus right up front if
                    // download is indeed faster than playback, use the
                    // min of newDurSoFar and curPlayTime.  If CurPlayTime
                    // is less, then we will properly report fast-enough
                    // download rate to avoid an unwarranted TLC pause:
                    UINT32 ulEarliestDurReptd = HX_MIN(ulNewDurSoFar,ulCurPlayTime);
                    if (m_pProgDnldStatusReportInfoList->GetCount())
                    {
                        LISTPOSITION lPos = m_pProgDnldStatusReportInfoList->
                                GetHeadPosition();
                        LISTPOSITION lPosPrior = lPos;

                        if (lPos)
                        {
                            pPDSRInfo1 = (CProgDnldStatusRptInfo*)
                                    m_pProgDnldStatusReportInfoList->GetNext(lPos);
                            ulTimeSinceEarliestInfo =
                                    CALCULATE_ELAPSED_TICKS(
                                    pPDSRInfo1->m_ulRelTimeOfRept,
                                    pPDSRInfoCur->m_ulRelTimeOfRept);
                            ulEarliestDurReptd =
                                    pPDSRInfo1->m_ulDurOfDnldBytesAtRptTime;
                            if (lPos)
                            {
                                pPDSRInfo2 = (CProgDnldStatusRptInfo*)
                                        m_pProgDnldStatusReportInfoList->
                                        GetAt(lPos);
                            }
                        }
                        while (lPos) // /List has 2+ elements:
                        {
                            ulTimeSinceEarliestInfo =
                                    CALCULATE_ELAPSED_TICKS(
                                    pPDSRInfo1->m_ulRelTimeOfRept,
                                    pPDSRInfoCur->m_ulRelTimeOfRept);
                            ulEarliestDurReptd =
                                    pPDSRInfo1->m_ulDurOfDnldBytesAtRptTime;
                            if (ulTimeSinceEarliestInfo >
                                    HX_PROGDOWNLD_DNLD_RATE_MOVING_WINDOW_SZ)
                            {
                                // /1st might be too old; see if there is a
                                // newer entry that is at least 1/2 the moving
                                // time window earlier than now; if so, toss
                                // the prior one:
                                UINT32 ulTimeSince2ndEarliestInfo =
                                        CALCULATE_ELAPSED_TICKS(
                                        pPDSRInfo2->m_ulRelTimeOfRept,
                                        pPDSRInfoCur->m_ulRelTimeOfRept);
                                if (HX_PROGDOWNLD_DNLD_RATE_MOVING_WINDOW_SZ <
                                        (ulTimeSince2ndEarliestInfo*2))
                                {
                                    // /Delete oldest in list:
                                    lPosPrior = m_pProgDnldStatusReportInfoList->
                                            RemoveAt(lPosPrior);

                                    HX_ASSERT(pPDSRInfo2 != pPDSRInfo1);
                                    HX_DELETE(pPDSRInfo1);
                                    ulTimeSinceEarliestInfo =
                                            ulTimeSince2ndEarliestInfo;
                                    ulEarliestDurReptd =
                                            pPDSRInfo2->m_ulDurOfDnldBytesAtRptTime;
                                }
                                else
                                {
                                    // /2nd-oldest is not old enough to approx
                                    // the download rate, so use the oldest
                                    break;
                                }
                            }
                            else
                            {
                                // /We have found the earliest in the window:
                                break;
                            }

                            lPosPrior = lPos;
                            pPDSRInfo1 = pPDSRInfo2;
                            pPDSRInfo2 = (CProgDnldStatusRptInfo*)
                                    m_pProgDnldStatusReportInfoList->
                                    // /Use GetAtNext(), not GetNext(); fixes
                                    // crash, above, when Info1==Info2 and
                                    // deletion of Info1 made Info2 invalid:
                                    // (PR 130520)
                                    GetAtNext(lPos); 
                        } /* while(lPos). */
                    }

                    // /Now it's OK to insert the latest info:
                    if (pPDSRInfoCur)
                    {
                        m_pProgDnldStatusReportInfoList->AddTail(pPDSRInfoCur);
                    }


                    UINT32 ulCurPlayTime = GetCurrentPlayTime();

                    // /Ignore all this if download is complete
                    if (ulNewDurSoFar >= m_ulTotalDurReported  ||
                            m_ulTotalDurReported <= ulCurPlayTime)
                    {
                        // /Download is complete so send remaining
                        // playback duration:
                        lTimeSurplus = HX_PROGDOWNLD_MAX_TIME_SURPLUS;
                    }
                    else if (ulTimeSinceEarliestInfo > 0) // /Avoid div by 0.
                    {
                        if(ulNewDurSoFar < ulEarliestDurReptd)
                            ulEarliestDurReptd = ulNewDurSoFar;
                        float fDownloadRate = 
                                (float)(ulNewDurSoFar - ulEarliestDurReptd) /
                                (float)ulTimeSinceEarliestInfo;
                        if (fDownloadRate >= .001)
                        {
                            if (m_ulTotalDurReported > ulCurPlayTime)
                            {
                                UINT32 lRemainingTimeUntilDownloadCompletes =
                                        (INT32)(float(m_ulTotalDurReported -
                                        ulNewDurSoFar) / fDownloadRate);
                                UINT32 ulRemainingPlaybackTime =
                                        m_ulTotalDurReported - ulCurPlayTime;
                                lTimeSurplus = (INT32)ulRemainingPlaybackTime -
                                        lRemainingTimeUntilDownloadCompletes;
                            }
                            else
                            {
                                // /Error condition where total dur reported is
                                // bogus (too small) so just report 0 since
                                // we're in no-man's land now:
                                lTimeSurplus = 0;
                            }
                        }
                        else
                        {
                            // /Download is _way_ too slow based on current
                            // data, so avoid overflow in calc and just pass
                            // along HX_PROGDOWNLD_MIN_TIME_SURPLUS
                            lTimeSurplus = HX_PROGDOWNLD_MIN_TIME_SURPLUS;
                        }
                    }
                    else
                    {
                        // /Can we ever get here?:
                        HX_ASSERT(ulTimeSinceEarliestInfo > 0);
                        // /Hmm.  Just pass UNKNOWN, I guess:
                        lTimeSurplus = HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS;
                    }
                }
                else
                {
                    // /Not enough info yet so report unknown:
                    lTimeSurplus = HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS;
                }
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadProgress(pStreamSource,
                        ulNewDurSoFar, ulNewBytesSoFar, lTimeSurplus);
            }
        }
    }
    return hxrslt;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnTotalDurChanged
 *  Purpose:
 *      This is a notification if the total file duration becomes known
 *      or becomes better-known during download/playback
 *      
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 */
STDMETHODIMP
HXPlayer::OnTotalDurChanged(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
            UINT32 ulNewDur)
{
    m_ulTotalDurReported = ulNewDur;
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                // /Pass what we observed along to our observers:
                pObserver->OnTotalDurChanged(pStreamSource,
                        ulNewDur);
            }
        }
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadComplete
 *
 *  Purpose:
 *      Notification that the entire file has been downloaded.
 *
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 *      
 */
STDMETHODIMP
HXPlayer::OnDownloadComplete(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadComplete(pStreamSource);
            }
        }
    }
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::SrcClaimsSeekSupport         ref: hxprdnld.h
 *  Purpose:
 *      Passes along notification from file sys that seek support
 *      is or is not claimed to be available (although sometimes HTTP
 *      server claims this when it doesn't actually support it).
 */
STDMETHODIMP
HXPlayer::SrcClaimsSeekSupport(IHXStreamSource* pStreamSource,
                               HXBOOL bClaimsSupport)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                // /Pass what we observed along to our observers:
                pObserver->SrcClaimsSeekSupport(pStreamSource, bClaimsSupport);
            }
        }
    }
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadPause
 *  Purpose:
 *      Notification that the file-download process has purposefully
 *      and temporarily halted downloading of the file
 *      
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 */
STDMETHODIMP
HXPlayer::OnDownloadPause(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (m_pPDSObserverList)
    {
        /* Send Begins to all the scheduled renderers */
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();

        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadPause(pStreamSource);
            }
        }
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadResume
 *  Purpose:
 *      Notification that the file-download process has resumed
 *      the process of downloading the remainder of the file
 *      
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 */
STDMETHODIMP
HXPlayer::OnDownloadResume(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadResume(pStreamSource);
            }
        }
    }
    return HXR_OK;
}
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.


#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
/*
 *  Helper methods that are used by IHXPresentationFeatureManager methods, below
 */        
HX_RESULT
HXPlayer::EstablishPFSSinkList()
{
    HX_RESULT  hxr = HXR_OK;

#if defined(HELIX_FEATURE_EVENTMANAGER)
    if (!m_pPFSEventProxyList)
    {
        m_pPFSEventProxyList = new CHXSimpleList();

        if (!m_pPFSEventProxyList)
        {
            hxr = HXR_OUTOFMEMORY;
        }
    }
#else //  If Events are not supported, then notify the P.F.Sinks directly:
    if (!m_pPFSSinkList)
    {
        m_pPFSSinkList = new CHXSimpleList();

        if (!m_pPFSSinkList)
        {
            hxr = HXR_OUTOFMEMORY;
        }
    }
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).

    return hxr;
}


HX_RESULT
HXPlayer::NotifyAllPFSinks(PfChangeEventType thePfChangeEvent,
                           const char* pszFeatureName)
{
    HX_RESULT  hxr = HXR_OK;

    if (!pszFeatureName  ||  '\0' == *pszFeatureName)
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
#if defined(HELIX_FEATURE_EVENTMANAGER)
        //  Fire the event and let the EventManager notify all EventSinks.
        // Each PresentationFeatureSink will get notified when its associated
        // CPresentationFeatureEventProxy object, an IHXEventSink, gets the
        // notification of the event and passes it on:
        HX_ASSERT(m_pEventManager);
        if (!m_pEventManager)
        {
            hxr = HXR_UNEXPECTED; //  This should not happen in this ifdef.
        }
        else
        {
            IHXBuffer* pEventNameBuf = NULL;

            // QI for IHXCommonClassFactory
            HX_ASSERT(m_pEngine);
            if (m_pEngine)
            {
                CHXString strPFEvent;

                IHXBuffer* pFeatureCurrentSetting = NULL;
                GetPresentationFeatureValue(pszFeatureName,
                        pFeatureCurrentSetting);

                //  PF event format is one of the following, depending on what
                // changed.  If the current setting changed, then the format is:
                // "PFS.[PF name].currentSettingChanged:[current setting]", e.g.,
                //    "PFS.language.currentSettingChanged:ja" or
                //    "PFS.pip.currentSettingChanged:on".
                // If the PF was just added or if it already was added but its
                // list of options just changed, then the event format is:
                // "PFS.[PF name].featureChanged", e.g.,
                //    "PFS.language.featureChanged"
                if (PF_EVENT_PF_CURRENT_VALUE_CHANGED == thePfChangeEvent)
                {
                    strPFEvent.Format("%s.%s.%s:%s",
                            PRESENTATION_FEATURE_SELECTION_EVENT_PREFIX,
                            pszFeatureName,
                            PRESENTATION_FEATURE_SELECTION_EVENT_CURVALCHANGED_STR,
                            (const char*)pFeatureCurrentSetting->GetBuffer());
                }
                else if (PF_EVENT_PF_FEATURE_CHANGED == thePfChangeEvent  ||
                        PF_EVENT_PF_ADDED == thePfChangeEvent  ||
                        PF_EVENT_PF_REMOVED == thePfChangeEvent)
                {
                    strPFEvent.Format("%s.%s.%s",
                            PRESENTATION_FEATURE_SELECTION_EVENT_PREFIX,
                            pszFeatureName,
                            PRESENTATION_FEATURE_SELECTION_EVENT_FEATURECHANGED_STR);
                }
                else
                {
                    hxr = HXR_UNEXPECTED;
                    HX_ASSERT(SUCCEEDED(hxr));
                }
                        
                if (SUCCEEDED(hxr))
                {
                    IUnknown* pContext = NULL;
                    hxr = m_pEngine->QueryInterface(IID_IUnknown, (void**)&pContext);
                    if (SUCCEEDED(hxr))
                    {
                        hxr = CreateStringBufferCCF(pEventNameBuf,
                                (const char*)strPFEvent, pContext);

                        //  The first parameter (an URL) can't be NULL in
                        // FireEvent, so we just pass in a dummy string:
                        IHXBuffer* pDummyURLBuf = NULL;
                        hxr = CreateStringBufferCCF(pDummyURLBuf,
                                PRESENTATION_FEATURE_SELECTION_EVENT_PREFIX,
                                pContext);

                        m_pEventManager->FireEvent(pDummyURLBuf,
                                                   NULL, //pFragmentStr
                                                   pEventNameBuf,
                                                   NULL); //IHXValues* pOtherVals

                        HX_RELEASE(pDummyURLBuf);
                    }

                    HX_RELEASE(pContext);
                }

                HX_RELEASE(pEventNameBuf);
                HX_RELEASE(pFeatureCurrentSetting);
            }
        }

#else //  If Events are not supported, then notify the P.F.Sinks directly:
        if (m_pPFSSinkList)
        {
            LISTPOSITION lPos = m_pPFSSinkList->GetHeadPosition();
            while (lPos)
            {
                HX_ASSERT(m_pPFSSinkList->GetCount() > 0);
                IHXPresentationFeatureSink* pPFSink = (IHXPresentationFeatureSink*)
                        m_pPFSSinkList->GetNext(lPos);
                if (pPFSink)
                {
                    // /Pass what we observed along to our observers:
                    switch (thePfChangeEvent)
                    {
                        case PF_EVENT_PF_CURRENT_VALUE_CHANGED:
                        {
                            IHXBuffer* pFeatureCurrentSetting = NULL;
                            GetPresentationFeatureValue(pszFeatureName,
                                    pFeatureCurrentSetting);
                            pPFSink->PresentationFeatureCurrentSettingChanged(
                                    pszFeatureName, pFeatureCurrentSetting);
                        }
                        break;

                        case PF_EVENT_PF_FEATURE_CHANGED:
                        case PF_EVENT_PF_ADDED:
                        case PF_EVENT_PF_REMOVED:
                        {
                            pPFSink->PresentationFeatureChanged(pszFeatureName);
                        }
                        break;

                        default:
                        {
                            hxr = HXR_INVALID_PARAMETER;
                        }
                    }
                }
            }
        }
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).
    }

    return hxr;
}


/*
 * IHXPresentationFeatureManager methods:
 */

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::SetPresentationFeature
 *
 * Purpose:
 *
 * This method is used for:
 * (1) adding a PF to the master PF list with an initial value and options,
 *  or
 * (2) setting or changing an existing PF's list of optional values.
 *
 * pFeatureOptions, an IHXValues, is a list of name+value pairs where the
 * names are the options that are possible for the PF to be set to during
 * any part of the presentation.  The associated value field for each name
 * should be set to "1" if that PF option is currently selectable, or "0" if
 * it is not selectable.  Anything other than "0" is treated as if it were
 * "1" (selectable).
 *
 * If pFeatureOptions is NULL, that indicates that the PF's possible values
 * are unconstrained (e.g., an URL or free-form text).  If it sees NULL, the
 * GUI may choose to prompt the user to input a free-form text string or
 * assume some bounds on input based purely on GUI design.  NOTE: if you want
 * to change the current setting but not change the current list of optional
 * values of an existing PF, you must call SetPresentationFeatureValue(...)
 * instead.
 *
 * When this method is used for (2), above, i.e., for setting an existing PF's
 * optional values and each of their associated "is-selectable" flags, it
 * overwrites the PF's entire list of options and their associated flags.
 * Thus, callers who wish to add to or change an existing PF's options should
 * first get those options (IHXValues) by calling GetPresentationFeature(),
 * changing them as needed, then passing them back into this method.  The
 * IHXValues are passed by value, not by reference, to avoid race conditions
 * and other problems; it is very unlikely that will result in a strain on
 * resources as these operations are not anticipated to be performed very
 * often.
 *
 * This can be called at any time during a presentation.  Any time it is
 * called, IHXPresentationFeatureManager does one of two things, depending
 * on whether IHXEventManager is implemented or not in HXPlayer:
 *   1. If IHXEventManager is implemented, the PF Manager notifies all IHXEvent
 *      sinks by calling m_pEventManager->FireEvent(...) with the event string
 *      format as one of the following, depending on what PF info changed.
 *      If the current setting changed, then the format is:
 *          "PFS.[PF name].currentSettingChanged:[current setting]"
 *      e.g.,
 *          "PFS.language.currentSettingChanged:ja"
 *      If the PF was just added or if it already was added but its list of
 *      options just changed, then the event format is:
 *          "PFS.[PF name].featureChanged"
 *      e.g.,
 *          "PFS.language.featureChanged"
 *   2. Else if IHXEventManager support is not there, the PF Manager notifies
 *      the PF sinks directly by calling:
 *           PresentationFeatureChanged([PF name]);
 * 
 * Note: the following characters and are not allowed in a PF name or value
 * due to XML and/or SMIL syntax restraints (other than a single optional
 * '!' at the start of a PF value):
 *  '!'  '.'  ','  ':'  ';'  '<'  '>'  (white space)  '''  '"'
 *
 * Also, the following strings are not allowed in a PF value due to SMIL
 * PF-specific syntax restraints:
 *  "_previous", "_next"
 * 
 * Return values for errors:
 *   - If the pszFeatureName is found but the pFeatureInitialValue is not
 *     found in the list of pFeatureOptions, then HXR_INVALID_PARAMETER is
 *     returned.  Note that if that list is NULL, then all values are allowed.
 *   - It returns HXR_PROP_INACTIVE if the PF's optional-values list contains
 *     the pFeatureInitialSetting but it is set to "0" (i.e., is not available
 *     for selection).
 *   - It returns HXR_INVALID_PARAMETER if pFeatureName or pFeatureCurSetting
 *     or any of the pFeatureOptions contain any of the disallowed characters
 *     or strings mentioned above.
 */
STDMETHODIMP
HXPlayer::SetPresentationFeature(const char* /*IN*/ pszFeatureName,
                                 IHXBuffer*  /*IN*/ pFeatureInitialValue,
                                 IHXValues*  /*IN*/ pFeatureOptions)
{
    HX_RESULT hxr = HXR_OK;
    HXBOOL bIsExistingPF = TRUE;

    if (!pszFeatureName  ||  !pFeatureInitialValue)
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
        CHXString strPFName;
        CHXString strPFCurrentSetting;
        CHXString strPFOptions;

        strPFName.Format("%s.%s", PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                pszFeatureName);
        strPFCurrentSetting.Format("%s.%s.%s", PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                pszFeatureName, PRESENTATION_FEATURE_SELECTION_REGISTRY_CURVAL_STR);
        strPFOptions.Format("%s.%s.%s", PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                pszFeatureName, PRESENTATION_FEATURE_SELECTION_REGISTRY_OPTIONS_STR);

        UINT32 ulRegId = m_pRegistry->GetId(PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX);
        if (!ulRegId) //  PF base isn't already in the registry, so add it:
        {
            bIsExistingPF = FALSE;
            //  Add the PF base ("PFS"):
            if (0 == m_pRegistry->AddComp(PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX))
            {
                //  Failed adding it.  Can't do much else, so throw an error:
                hxr = HXR_UNEXPECTED;
            }
        }

        ulRegId = m_pRegistry->GetId((const char*)strPFName);
        if (!ulRegId) //  This PF isn't already in the registry, so add it:
        {
            bIsExistingPF = FALSE;
            //  Add the PF (PFS.[name of PF]):
            m_pRegistry->AddComp((const char*)strPFName);

            //  Add the PF's current setting (PFS.[name of PF].curValue.[cur val]):
            m_pRegistry->AddStr((const char*)strPFCurrentSetting, pFeatureInitialValue);
        }
        else //  PF Already exists:
        {
            //  Set the new current setting:
            m_pRegistry->SetStrByName((const char*)strPFCurrentSetting, pFeatureInitialValue);

            //  Overwrite existing stored feature options; we'll
            // (re)write them, below:
            UINT32 ulPfId = m_pRegistry->DeleteByName(strPFOptions);
            HX_ASSERT(ulPfId);
        }

        //  Add (or add back) the ".options" composite property
        // (PFS.[name of PF].options) before filling it with values:
        m_pRegistry->AddComp((const char*)strPFOptions);

        // Copy all PF options from IHXValues into PFS.[PF].options registry:
        if (pFeatureOptions)
        {
            const char* pPFOptionName = NULL;
            UINT32 bPFOptionIsSelectableFlag = FALSE;
            IHXBuffer* pPFOptionIsSelectableFlag = NULL;
         
            if (HXR_OK == pFeatureOptions->GetFirstPropertyCString(pPFOptionName,
                    pPFOptionIsSelectableFlag)  &&  *pPFOptionName)
            {
                do
                {
                    const UCHAR* pIsSelFlag = pPFOptionIsSelectableFlag->GetBuffer();
                    if (pIsSelFlag  &&  *pIsSelFlag)
                    {
                        //  Anything but '0' (zero) is treated as '1' (is selectable):
                        bPFOptionIsSelectableFlag = (HXBOOL)('0' != *pIsSelFlag  &&
                                '\0' == *(pIsSelFlag+1));
                    }
                    else
                    {
                        //  Set the error, but we'll go ahead and treat it as '0':
                        hxr = HXR_INVALID_PARAMETER;
                        bPFOptionIsSelectableFlag = FALSE;
                    }

                    CHXString strPFOptionRegistryName;
                    strPFOptionRegistryName.Format("%s.%s",
                            (const char*)strPFOptions, pPFOptionName);
                    if (0 == m_pRegistry->AddInt(
                            (const char*)strPFOptionRegistryName,
                            bPFOptionIsSelectableFlag? 1:0))
                    {
                        hxr = HXR_FAIL;
                        break;
                    }

                    //  Release it to get the next PF:
                    HX_RELEASE(pPFOptionIsSelectableFlag);
                } while (HXR_OK == pFeatureOptions->GetNextPropertyCString(
                                pPFOptionName, pPFOptionIsSelectableFlag));

                HX_RELEASE(pPFOptionIsSelectableFlag);
            }
        }
        else //  Else there is no set list of options, so it's "freeform" (open-ended):
        {
            CHXString strPFOptionRegistryName;
            strPFOptionRegistryName.Format("%s.%s", (const char*)strPFOptions,
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_OPTIONS_FREEFORM_STR);
            if (0 == m_pRegistry->AddInt(
                    (const char*)strPFOptionRegistryName, 1))
            {
                hxr = HXR_FAIL;
            }
        }

        //  XXXEH- PFS-TODO: test what works best, here, if there's an error.
        // Do we go ahead and notify anyway, or not?
        HX_ASSERT(HXR_OK == hxr);

        //  Notify all sinks of change in PF.  Sinks receiving this
        // featureChanged event will have to look at the current setting
        // in case it changed along with a change in options:
        if (bIsExistingPF)
        {
            NotifyAllPFSinks(PF_EVENT_PF_FEATURE_CHANGED, pszFeatureName);
        }
        else
        {
            NotifyAllPFSinks(PF_EVENT_PF_ADDED, pszFeatureName);
        }
    }

    return hxr;
}

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::GetPresentationFeature
 *
 * Purpose:
 *
 * This method gets all information stored for a particular presentation
 * feature: (1) the current setting of the PF, and (2) the list of possible
 * optional values the PF is allowed to be set to in the current presentation.
 *
 * pFeatureOptions, an IHXValues, is a list of name+value pairs where the
 * names are the options that are possible for the PF to be set to during
 * any part of the presentation.  The associated value field for each name
 * should be set to "1" if that PF option is currently selectable, or "0" if
 * it is not selectable.  Anything other than "0" is treated as if it were
 * "1" (selectable).
 *
 * Note that pFeatureOptions can be NULL indicating that the choices are
 * not enumerable, e.g., URLs.
 *
 * This can be called at any time during a presentation.
 *
 * Return values for errors:
 *   - Returns HXR_INVALID_PARAMETER if the feature name does not correspond
 *    to a PF that was added already via SetPresentationFeature().
 */
STDMETHODIMP
HXPlayer::GetPresentationFeature(const char* /*IN*/ pszFeatureName,
                                 REF(IHXBuffer*) /*OUT*/ pFeatureCurrentSetting,
                                 REF(IHXValues*) /*OUT*/ pFeatureOptions)
{
    HX_RESULT hxr = HXR_OK;

    //  name must not be NULL and both REFs must be NULL on the way in:
    if (!pszFeatureName  ||  pFeatureCurrentSetting  ||  pFeatureOptions)
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
        CHXString strPFRegistryName;

        strPFRegistryName.Format("%s.%s",
                PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX, pszFeatureName);

        UINT32 ulRegId = m_pRegistry->GetId((const char*)strPFRegistryName);
        if (!ulRegId) //  It doesn't already exist in the registry:
        {
            hxr = HXR_INVALID_PARAMETER;
        }
        else //  Already exists, so get its value:
        {
            CHXString strPFCurrentSetting;
            CHXString strPFOptions;

            strPFCurrentSetting.Format("%s.%s.%s",
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX, pszFeatureName,
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_CURVAL_STR);

            strPFOptions.Format("%s.%s.%s",
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX, pszFeatureName,
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_OPTIONS_STR);

            // Get the current setting:
            m_pRegistry->GetBufByName(strPFCurrentSetting, pFeatureCurrentSetting);


            IHXValues* pOptionsListFromRegistry = NULL;
            //  XXXEH- NOTE: the following will always return HXR_OK as far as
            // I can tell; pFeatureOptions will just get nothing in it if
            // pszFeatureName is not a known PF:
            hxr = m_pRegistry->GetPropListByName(strPFOptions,
                    pOptionsListFromRegistry);

            if (SUCCEEDED(hxr))
            {
                //  Go through the int props that are in the registry-formatted
                // options list and remove the "PFS.{PF_name}.options." that's
                // at the start of each.  Then, take each value field, which is
                // a ulRegistryID value, and use it to get the associated
                // option's string value ("0" or "1"):
                hxr = CreateValuesCCF(pFeatureOptions,
                        (IUnknown*)(IHXClientEngine*)m_pEngine);

                if (SUCCEEDED(hxr))
                {
                    const char* pszPFOptionFromRegistry = NULL;
                    UINT32 ulOptionRegID = 0;

                    hxr = pOptionsListFromRegistry->GetFirstPropertyULONG32(
                            pszPFOptionFromRegistry, ulOptionRegID);
                    if (SUCCEEDED(hxr))
                    {
                        HX_ASSERT(*pszPFOptionFromRegistry);
                        if (!(*pszPFOptionFromRegistry))
                        {
                            hxr = HXR_UNEXPECTED; //  Shouldn't be empty string
                        }
                        else
                        {
                            do
                            {
                                //  Copy to pFeatureOptions:
                INT32 lIsOptionEnabled = 0;
                                hxr = m_pRegistry->GetIntById(ulOptionRegID,
                                        lIsOptionEnabled);
                                if (FAILED(hxr))
                                {
                                    break;
                                }

                                //  The option name we want is past the last
                                // '.' in "PFS.{PF_name}.options.{Option_name}"
                                const char* pLastDot =
                                        strrchr(pszPFOptionFromRegistry, '.');
                                if (pLastDot  &&  '.' == *pLastDot  &&
                                                         *(pLastDot+1))
                                {
                                    hxr = SetCStringPropertyCCF(pFeatureOptions,
                                            pLastDot+1,
                                            lIsOptionEnabled != 0? "1":"0",
                                            (IUnknown*)(IHXClientEngine*)m_pEngine,
                                            FALSE);
                                }
                            } while (HXR_OK ==
                                    pOptionsListFromRegistry->GetNextPropertyULONG32(
                                    pszPFOptionFromRegistry,ulOptionRegID) );
                        }
                    }
                }                
            }

            HX_RELEASE(pOptionsListFromRegistry);
        }

        if (FAILED(hxr))
        {
            HX_RELEASE(pFeatureCurrentSetting);
            HX_RELEASE(pFeatureOptions);
        }
    }

    return hxr;
}

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::SetPresentationFeatureValue
 *
 * Purpose:
 *
 * This method is used for for setting an existing PF's current setting.
 *
 * This can be called at any time during a presentation.  Any time it is
 * called, IHXPresentationFeatureManager notifies all sinks of the change
 * by calling:
 *   PresentationFeatureCurrentSettingChanged([PF name],[PF new current val]);
 *
 * Return values for errors:
 *   - It returns HXR_PROP_INACTIVE if the PF already exists and its optional
 *     values list contains the pRequestedPFCurrentSetting but it is set to "0"
 *     (i.e., is not available for selection).
 *   - If the pszFeatureName is not found, HXR_PROP_NOT_FOUND is returned.
 *   - If the pRequestedPFCurrentSetting is not found in the PF's list of
 *     options and that list is not NULL, HXR_INVALID_PARAMETER is returned.
 *
 * NOTE: even if the PF's current setting was already set to the same as what's
 * in pPFNewCurrentSetting, we still process this as a change so that the
 * caller and sinks get feedback that the information is flowing; they can
 * ignore the non-change event. 
 */
STDMETHODIMP
HXPlayer::SetPresentationFeatureValue(const char* /*IN*/ pszFeatureName,
                                      IHXBuffer*  /*IN*/ pPFNewCurrentSetting)
{
    HX_RESULT hxr = HXR_OK;

    if (!pszFeatureName  ||  !pPFNewCurrentSetting  ||
            !pPFNewCurrentSetting->GetSize())
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
        CHXString strPFRegistryName;
        const char* pszNewPFCurrentSetting = (const char*)
                pPFNewCurrentSetting->GetBuffer();

        strPFRegistryName.Format("%s.%s",
                PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                pszFeatureName);

        UINT32 ulRegId = m_pRegistry->GetId((const char*)strPFRegistryName);
        if (!ulRegId) //  It doesn't already exist in the registry:
        {
            hxr = HXR_PROP_NOT_FOUND;
        }
        else //  PF exists; check if options includes this value:
        {
            CHXString strPFOptions;
            strPFOptions.Format("%s.%s.%s",
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                    pszFeatureName,
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_OPTIONS_STR);

            IHXValues* pFeatureOptions = NULL;
            hxr = m_pRegistry->GetPropListByName((const char*)strPFOptions,
                    pFeatureOptions);

            if (!pFeatureOptions)
            {
                //  This is an open-ended feature so all options are OK:
                hxr = HXR_OK;
            }
            else if (SUCCEEDED(hxr))
            {
                //  PF options are not NULL (i.e., not open-ended), so check if
                // an option matches the requested new value.  If so, make sure
                // it is enabled for selection:
                CHXString strPFOption;
                strPFOption.Format("%s.%s", (const char*)strPFOptions,
                        pszNewPFCurrentSetting);

                INT32 lIsSelectable = 0;
                hxr = m_pRegistry->GetIntByName((const char*)strPFOption,
                        lIsSelectable);

                if (SUCCEEDED(hxr))
                {
                    if (lIsSelectable == 0)
                    {
                        hxr = HXR_PROP_INACTIVE; //  Option not enabled.
                    }
                }
                else
                {
                    hxr = HXR_INVALID_PARAMETER;
                }
            }

            if (SUCCEEDED(hxr))
            {
                CHXString strPFCurrentSetting;
                strPFCurrentSetting.Format("%s.%s.%s",
                        PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                        pszFeatureName,
                        PRESENTATION_FEATURE_SELECTION_REGISTRY_CURVAL_STR);

                //  XXXEH-NOTE: we want to send the notification to the sinks
                // even if the value didn't actually change here.  Let the
                // sinks deal with a non-change if they want to do special
                // processing for that (e.g., SMIL2 renderer might not want
                // to restart an element listening for a pfs:onSettingChanged
                // event).

                //  Set new current setting:
                m_pRegistry->SetBufByName(strPFCurrentSetting,
                        pPFNewCurrentSetting);

                //  Notify all sinks of change in PF:
                NotifyAllPFSinks(PF_EVENT_PF_CURRENT_VALUE_CHANGED,
                        pszFeatureName);
            }

            HX_RELEASE(pFeatureOptions);

        } //  END: else //  PF exists.
    }

    return hxr;
}

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::GetPresentationFeatureValue
 *
 * Purpose:
 *
 * This method gets the current setting of a particular presentation feature.
 *
 * This can be called at any time during a presentation.
 *
 * Return values for errors:
 *  - If pszFeatureName is not found in the PFs registry, this returns
 *    HXR_INVALID_PARAMETER.
 */
STDMETHODIMP
HXPlayer::GetPresentationFeatureValue(const char* /*IN*/ pszFeatureName,
                                      REF(IHXBuffer*) /*OUT*/ pFeatureCurrentSetting)
{
    HX_RESULT hxr = HXR_OK;

    pFeatureCurrentSetting = NULL;

    if (!pszFeatureName)
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
        CHXString strPFRegistryName;

        strPFRegistryName.Format("%s.%s",
                PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                pszFeatureName);

        UINT32 ulRegId = m_pRegistry->GetId((const char*)strPFRegistryName);
        if (!ulRegId) //  It doesn't already exist in the registry:
        {
            hxr = HXR_INVALID_PARAMETER;
        }
        else //  PF exists; get its current setting:
        {
            CHXString strPFCurrentSetting;
            strPFCurrentSetting.Format("%s.%s.%s",
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                    pszFeatureName,
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_CURVAL_STR);

            //  Get current setting:
            hxr = m_pRegistry->GetBufByName(strPFCurrentSetting,
                    pFeatureCurrentSetting);
        } //  END: else //  PF exists.
    }

    return hxr;
}



/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::RemovePresentationFeature
 *
 * Purpose:
 *
 * This method removes completely from the registry the PF, its current
 * value, and its associated optional values.  Note: nothing prevents two
 * components from adding the same PF.  Implementers of this may want to
 * keep a ref count of each PF to prevent one component from removing a PF
 * that another is still using.
 *
 * This can be called at any time during a presentation.
 *
 * Return values for errors:
 *  - If pszFeatureName is not found in the PFs registry, this returns
 *    HXR_INVALID_PARAMETER.
 */
STDMETHODIMP
HXPlayer::RemovePresentationFeature(const char* /*IN*/ pszFeatureName)
{
    HX_RESULT hxr = HXR_OK;

    if (!pszFeatureName)
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
        //  XXXEH- TODO: implement this better so that it keeps a ref count
        // of how many added the PF; only DeleteByName() if the refcount drops
        // to 0:

        CHXString strPFRegistryName;
        strPFRegistryName.Format("%s.%s",
                PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX, pszFeatureName);

        UINT32 ulPfId = m_pRegistry->DeleteByName(strPFRegistryName);
        if (0 == ulPfId)
        {
            hxr = HXR_INVALID_PARAMETER;
        }
        else //  Notify all sinks of change in PF
        {
            NotifyAllPFSinks(PF_EVENT_PF_REMOVED, pszFeatureName);
        }
    }

    return hxr;
}

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::GetPresentationFeatures
 *
 * Purpose:
 *
 * This passes back the entire feature set that has been added to the player
 * registry for the presentation, along with each PF's current setting.  Note
 * that the options for each PF are not included in what is provided by this
 * method; to get that information, call GetPresentationFeatureOptions(...)
 */
STDMETHODIMP
HXPlayer::GetPresentationFeatures(REF(IHXValues*) /*OUT*/ pFeatures)
{
    HX_RESULT hxr = HXR_NOTIMPL;
    
    //Get everything from PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX

    if (pFeatures) //  It's supposed to point to NULL coming in:
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
        UINT32 ulRegId = m_pRegistry->GetId(PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX);
        if (!ulRegId) //  It doesn't already exist in the registry:
        {
            hxr = HXR_INVALID_PARAMETER;
        }
        else //  Already exists, so get all the PFs (features) in it:
        {
            hxr = m_pRegistry->GetPropListByName(
                    PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
                    pFeatures);
        }
    }

    return hxr;
}

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::SetGlobalUserPreference
 *
 * Purpose:
 *
 * If the application wishes to make a current PF value be the
 * global user-preference value for a PF that matches one of the
 * IHXPreferences, it does so using the following method.  Effect
 * on IHXPreferences is immediate.  If the PF does not match an
 * existing IHXPreference, HXR_INVALID_PARAMETER is returned, i.e.,
 * a new IHXPreference is not created in that case.
 *
 * This can be called at any time during a presentation.
 */
STDMETHODIMP
HXPlayer::SetGlobalUserPreference(const char* /*IN*/ pszFeatureName)
{
    //  XXXEH- TODO: implement this:
    HX_RESULT hxr = HXR_NOTIMPL;

    CHXString strPFName;
    strPFName.Format("%s.%s", PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX,
            pszFeatureName);

    return hxr;
}

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::AddPresentationFeatureSink
 *
 * Purpose:
 *
 * This does one of two things:
 * 1) If IHXEventManager is implemented, then this creates a new
 * CPresentationFeatureEventProxy object using the pPFSink passed in, and
 * adds it (as an IHXEventSink) via AddEventSink to the IHXEventManager,
 * and it saves the proxy object in a list.
 * Or,
 * 2) If IHXEventManager is not implemented, this adds the pPFSink to a list
 * of PF sinks that the PF manager maintains.
 */
STDMETHODIMP
HXPlayer::AddPresentationFeatureSink(IHXPresentationFeatureSink* pPFSink)
{
    HX_RESULT hxr = HXR_INVALID_PARAMETER;
    if (pPFSink)
    {
        HX_ADDREF(pPFSink);

#if defined(HELIX_FEATURE_EVENTMANAGER)
        HX_ASSERT(m_pEventManager);
        if (!m_pEventManager)
        {
            hxr = HXR_UNEXPECTED;
        }
        else if (!m_pPFSEventProxyList)
#else //  If Events are not supported, then notify the P.F.Sinks directly:
        if (!m_pPFSSinkList)
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).
        {
            hxr = EstablishPFSSinkList();
        }

        if (SUCCEEDED(hxr))
        {
#if defined(HELIX_FEATURE_EVENTMANAGER)
            IHXPresentationFeatureManager* pPFMgr = NULL;
            hxr = QueryInterface(IID_IHXPresentationFeatureManager, (void**)&pPFMgr);
            if (SUCCEEDED(hxr))
            {
                CPresentationFeatureEventProxy* pPFEventProxy =
                        new CPresentationFeatureEventProxy(pPFSink, pPFMgr);
                HX_ASSERT(pPFEventProxy);
                if (pPFEventProxy)
                {
                    HX_ADDREF(pPFEventProxy);
                    m_pPFSEventProxyList->AddTail(pPFEventProxy);
                    //  Now, add the PFEventProxy as an EventSink:
                    // QI for IHXEventManager
                    if (m_pEventManager)
                    {
                        // Get the proxy's IHXEventSink:
                        IHXEventSink* pEventSink = NULL;
                        pPFEventProxy->QueryInterface(IID_IHXEventSink,
                                (void**) &pEventSink);
                        HX_ASSERT(pEventSink);
                        if (pEventSink)
                        {
                            // Add it as an event sink:
                            m_pEventManager->AddEventSink(pEventSink);
                        }
                    }
                }
                else
                {
                    hxr = HXR_OUTOFMEMORY;
                }
            }

            HX_RELEASE(pPFMgr);

            //  New CP.F.EventProxy AddRef'd this, above, so we can release
            // our ref:
            HX_RELEASE(pPFSink);

#else //  If Events are not supported, then we notify the P.F.Sinks directly:
            //  pPFSink was AddRef'd for this, above:
            m_pPFSSinkList->AddTail(pPFSink);
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).
        }
    }

    return hxr;
}

/***************************************************************************
 * Method:
 *      IHXPresentationFeatureManager::RemovePresentationFeatureSink
 *
 * Purpose:
 *
 * This does one of two things:
 * 1) If IHXEventManager is implemented, then this removes the associated
 * CPresentationFeatureEventProxy object from the proxy object list, then
 * releases it.  It also removes the IHXEventSink from the Event manager.
 * Or,
 * 2) If IHXEventManager is not implemented, this removes the pPFSink from
 * its list of PF sinks, and releases it.
 */
STDMETHODIMP
HXPlayer::RemovePresentationFeatureSink(IHXPresentationFeatureSink* pPFSink)
{
    HX_RESULT hxr = HXR_INVALID_PARAMETER;
    if (pPFSink)
    {
        HXBOOL bFound = FALSE;

#if defined(HELIX_FEATURE_EVENTMANAGER)
        if (m_pPFSEventProxyList)
        {
            LISTPOSITION lPos;

            lPos = m_pPFSEventProxyList->GetHeadPosition();
            // Search for the one with the matching pPFSink:
            while (lPos  && !bFound)
            {
                CPresentationFeatureEventProxy* pPFEventProxy =
                        (CPresentationFeatureEventProxy*)
                        m_pPFSEventProxyList->GetAt(lPos);
                if (pPFEventProxy  &&  pPFEventProxy->GetSink() == pPFSink)
                {
                m_pPFSEventProxyList->RemoveAt(lPos);
                    HX_RELEASE(pPFEventProxy); //  This releases its PFSink.
                    bFound = TRUE;
                    hxr = HXR_OK;
                }
                else
                {
                    m_pPFSEventProxyList->GetNext(lPos);
                }
            }
        }
#else //  If Events are not supported, then we have a list of P.F.Sinks:
        if (m_pPFSSinkList)
        {
            LISTPOSITION lPosition = m_pPFSSinkList->Find(pPFSink);
            if (lPosition)
            {
                m_pPFSSinkList->RemoveAt(lPosition);
                HX_RELEASE(pPFSink);
                hxr = HXR_OK;
            }
        }
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).

        if (!bFound)
        {
            hxr = HXR_FAIL;
        }
    }
    return hxr;
}


/***************************************************************************
* This breaks a PF event string up into its components.
*
* PF event format is one of the following, depending on what information
* changed.  If the PF's current setting changed, then the format is:
*
*   "PFS.[PF name].currentSettingChanged:[current setting]"
*
* e.g., "PFS.language.currentSettingChanged:ja" or
*       "PFS.pip.currentSettingChanged:on".
*
* If the PF was just added, or if it was added earlier but its list of
* options just changed, then the event format is:
*
*   "PFS.[PF name].featureChanged"
*
* e.g., "PFS.language.featureChanged"
*/

STDMETHODIMP
HXPlayer::ParsePFEvent(const char* pszPFEvent,
                       REF(PfChangeEventType) /*OUT*/ thePfChangeEvent,
                       REF(IHXBuffer*) /*OUT*/ pFeatureNameBuf,
                       REF(IHXBuffer*) /*OUT*/ pFeatureSettingBuf)
{
    HX_RESULT  hxr = HXR_OK;

    thePfChangeEvent = PF_EVENT_INVALID_EVENT;

    if (!pszPFEvent  ||  0 != strncmp(pszPFEvent,
            PRESENTATION_FEATURE_SELECTION_EVENT_PREFIX_WITH_SEPARATOR,
            PRESENTATION_FEATURE_SELECTION_EVENT_LEN_OF_PREFIX_WITH_SEPARATOR))
    {
        //  This is not a valid PF event:
        hxr = HXR_INVALID_PARAMETER;
    }
    else
    {
#if defined(HELIX_FEATURE_EVENTMANAGER)
        HX_ASSERT(m_pEventManager); //  It should not be NULL within this ifdef
        if (!m_pEventManager)
        {
            hxr = HXR_UNEXPECTED;
        }
        else
        {
            //  Parse the event string and figure out what kind it is.  First go
            // past the prefix+separator ("PFS."):
            INT32 lSizeInclTerminator = strlen(pszPFEvent) + 2 -
                    PRESENTATION_FEATURE_SELECTION_EVENT_LEN_OF_PREFIX_WITH_SEPARATOR;
            char* pszPFNameLoc = new char[lSizeInclTerminator];

            if (!pszPFNameLoc)
            {
                hxr = HXR_OUTOFMEMORY;
            }
            else
            {
                SafeStrCpy(pszPFNameLoc, pszPFEvent +
                        PRESENTATION_FEATURE_SELECTION_EVENT_LEN_OF_PREFIX_WITH_SEPARATOR,
                        lSizeInclTerminator-1);

                //  Find first of '.' or ':'
                char* psz2ndSeparator = !pszPFNameLoc? NULL :
                        strpbrk(pszPFNameLoc,
                        PRESENTATION_FEATURE_SELECTION_EVENT_SEPARATORS_SET);

                if (!psz2ndSeparator)
                {
                    hxr = HXR_OUTOFMEMORY;
                }
                else if ('.' != *psz2ndSeparator  ||
                        '\0' == *(psz2ndSeparator+1))
                {
                    hxr = HXR_INVALID_PARAMETER;
                }
                else
                {
                    const char* pPFEventType = psz2ndSeparator+1;
                    const char* pszPFValueLoc = NULL;

                    *psz2ndSeparator = '\0'; //  Terminate PF name string.

                    char* psz3rdSeparator = strpbrk(psz2ndSeparator+1,
                        PRESENTATION_FEATURE_SELECTION_EVENT_SEPARATORS_SET);

                    if (!psz3rdSeparator)
                    {
                        pszPFValueLoc = NULL; //  Must be a featureChanged event.
                    }
                    //  If it's not NULL then it has to be a ':' followed by
                    // non-null character(s):
                    else if (':' == *psz3rdSeparator  &&  '\0' != *(psz3rdSeparator+1) )
                    {
                        //  It's a currentSettingChanged event:
                        *psz3rdSeparator = '\0';
                        pszPFValueLoc = (psz3rdSeparator+1);
                        HX_ASSERT(0 == strcmp(pPFEventType, 
                                PRESENTATION_FEATURE_SELECTION_EVENT_CURVALCHANGED_STR) );
                    }
                    else
                    {
                        //  Event string has syntax error
                        hxr = HXR_INVALID_PARAMETER;
                    }

                    //  Is it a currentSettingChanged or featureChanged event?:
                    if (0 == strcmp(pPFEventType, 
                            PRESENTATION_FEATURE_SELECTION_EVENT_CURVALCHANGED_STR))
                    {
                        //  It's a currentSettingChanged event:
                        thePfChangeEvent = PF_EVENT_PF_CURRENT_VALUE_CHANGED;
                    }
                    else if (0 == strcmp(pPFEventType,
                            PRESENTATION_FEATURE_SELECTION_EVENT_FEATURECHANGED_STR))
                    {
                        //  It's a featureChanged event:
                        thePfChangeEvent = PF_EVENT_PF_FEATURE_CHANGED;
                        HX_ASSERT(!psz3rdSeparator  ||  !(*psz3rdSeparator));
                    }
                    else
                    {
                        hxr = HXR_INVALID_PARAMETER;
                        thePfChangeEvent = PF_EVENT_INVALID_EVENT;
                    }

                    if (SUCCEEDED(hxr))
                    {
                        //  Now create the buffers to pass back.  Release them in case
                        // they're not NULL:
                        HX_RELEASE(pFeatureNameBuf);
                        HX_RELEASE(pFeatureSettingBuf);

                        IUnknown* pContext = NULL;
                        hxr = m_pEngine->QueryInterface(IID_IUnknown, (void**)&pContext);
                        if (SUCCEEDED(hxr))
                        {
                            hxr = CreateStringBufferCCF(pFeatureNameBuf,
                                    (const char*)pszPFNameLoc, pContext);
                            if (HXR_OK == hxr  &&  pszPFValueLoc)
                            {
                                hxr = CreateStringBufferCCF(pFeatureSettingBuf,
                                        (const char*)pszPFValueLoc, pContext);
                            }

                            HX_RELEASE(pContext);
                        }
                        else
                        {
                            hxr = HXR_UNEXPECTED;
                        }
                    }
                }

                delete [] pszPFNameLoc;
            }
        }

#else   //  If Events are not supported, then P.F.Sinks were notified directly
        //  so there is no reason for this method to be called:
        hxr = HXR_UNEXPECTED;
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).
    }

    return hxr;
}

#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.__^^^^^^^^^^^^^^^^^^^^


#if defined(HELIX_FEATURE_PLAYBACK_MODIFIER)

HX_RESULT
HXPlayer::SetPlayRangeEndTime(UINT32 ulEndTime)
{
    HX_RESULT theErr = HXR_OK;

    SetPresentationTime(ulEndTime);

    IHXValues*  pUrlOptions = NULL;

    if (m_pURL)
    {
        pUrlOptions = m_pURL->GetOptions();

        if ( pUrlOptions )
        {
            pUrlOptions->SetPropertyULONG32("End", ulEndTime);            
        
            CHXMapPtrToPtr::Iterator ndxSource = m_pSourceMap->Begin();
            for (; !theErr && ndxSource != m_pSourceMap->End(); ++ndxSource)
            {
                SourceInfo* pSourceInfo = (SourceInfo*)(*ndxSource);
                HXSource * pSource = pSourceInfo->m_pSource;

                // pSource should never be NULL
                HX_ASSERT(pSource);

                theErr = pSource->UpdatePlayTimes(pUrlOptions);
            }
            
            HX_RELEASE(pUrlOptions);
        }
    }
    return (theErr);
}

STDMETHODIMP
HXPlayer::ParsePlaybackModifiers( IHXValues* pOptions )
{
    if (!pOptions || m_bIsDone) 
    {
        return HXR_UNEXPECTED;
    }

    if (!m_bInitialized)
    {
        return HXR_NOT_INITIALIZED;
    }

    HX_RELEASE(m_pPlaybackModifiers);
    m_pPlaybackModifiers = pOptions;
    HX_ADDREF(m_pPlaybackModifiers);

    HX_RESULT theErr = HXR_OK;

    UINT32 ulTemp = 0;

    if ( SUCCEEDED(pOptions->GetPropertyULONG32("PlayRangeEndTime", ulTemp)) )
    { 
        theErr = SetPlayRangeEndTime(ulTemp);
    }

    return theErr;
}

/*
 *  IHXPlaybackModifier methods
 */

/************************************************************************
 *  Method:
 *      IHXPlaybackModifier::SetPlaybackModifiers
 *
 *  Purpose:
 *      This method is used to adjust playback values without restarting
 *      the presentation.
 */
STDMETHODIMP HXPlayer::SetPlaybackModifiers( IHXValues* pModifiers )
{
    HXLOGL1(HXLOG_CORE, "HXPlayer[%p]::SetPlaybackModifiers()", this);
    HX_RESULT theErr = HXR_OK;

    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;
    theErr = ParsePlaybackModifiers(pModifiers);
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

/************************************************************************
 *  Method:
 *      IHXPlaybackModifier::GetPlaybackModifiers
 *
 *  Purpose:
 *      This method gets the adjustments made to the current presentation
 */
STDMETHODIMP HXPlayer::GetPlaybackModifiers( IHXValues* &pModifiers )
{
    pModifiers = m_pPlaybackModifiers;

    HX_ADDREF(m_pPlaybackModifiers);

    return HXR_OK;
}

#endif // HELIX_FEATURE_PLAYBACK_MODIFIER


// working set monitoring tools on Windows
#if 0
#ifdef WIN32

typedef struct _PSAPI_WS_WATCH_INFORMATION
{
    LPVOID FaultingPc;
    LPVOID FaultingVa;
} PSAPI_WS_WATCH_INFORMATION;

#define WORKING_SET_BUFFER_ENTRYS 4096
PSAPI_WS_WATCH_INFORMATION WorkingSetBuffer[WORKING_SET_BUFFER_ENTRYS];

typedef HXBOOL (WINAPI * InitializeProcessForWsWatchType) (
    HANDLE hProcess
    );

typedef HXBOOL (WINAPI * GetWsChangesType) (
    HANDLE hProcess,
    PSAPI_WS_WATCH_INFORMATION* lpWatchInfo,
    DWORD cb
    );

InitializeProcessForWsWatchType g_fpInitializeProcessForWsWatch = NULL;
GetWsChangesType                g_fpGetWsChanges = NULL;
HINSTANCE                       g_PSAPIHandle = NULL;
HANDLE                          g_hCurrentProcess = NULL;
HXBOOL                            g_bPrintChanges = FALSE;
HXBOOL                            g_bSetWsChangeHook = FALSE;
HXBOOL                            g_bLockLikeACrazyMan = TRUE;
CHXMapPtrToPtr                  g_mWorkingSetMap;
int                             g_LockedPages = 0;
int                             g_totalFaults = 0;
int                             g_AttemptedLockedPages = 0;
int                             g_FailedLockedPages = 0;


void GetChangesToWorkingSet()
{
    int   count = 0;

    if (!g_hCurrentProcess)
    {
        g_hCurrentProcess = GetCurrentProcess();

        // check to see if we want to print data
        HKEY    hKey;
        char    szKeyName[256]; /* Flawfinder: ignore */
        long    szValueSize = 1024;
        char    szValue[1024]; /* Flawfinder: ignore */

        g_bLockLikeACrazyMan = FALSE;

        SafeSprintf(szKeyName, 256, "Software\\%s\\Debug\\LockLikeCrazy", HXVER_COMMUNITY);
        if( RegOpenKey(HKEY_CLASSES_ROOT, szKeyName, &hKey) == ERROR_SUCCESS )
        {
            RegQueryValue(hKey, "", szValue, (long *)&szValueSize);
            g_bLockLikeACrazyMan = !strcmp(szValue,"1");
            RegCloseKey(hKey);
        }
        else
        {
            RegCreateKey(HKEY_CLASSES_ROOT, szKeyName, &hKey);
            RegCloseKey(hKey);
        }

        g_bSetWsChangeHook = FALSE;

        SafeSprintf(szKeyName, 256, "Software\\%s\\Debug\\SetWsHook", HXVER_COMMUNITY);
        if( RegOpenKey(HKEY_CLASSES_ROOT, szKeyName, &hKey) == ERROR_SUCCESS )
        {
            RegQueryValue(hKey, "", szValue, (long *)&szValueSize);
            g_bSetWsChangeHook = !strcmp(szValue,"1");
            RegCloseKey(hKey);
        }
        else
        {
            RegCreateKey(HKEY_CLASSES_ROOT, szKeyName, &hKey);
            RegCloseKey(hKey);
        }

        if (g_bSetWsChangeHook)
        {
            if (g_bLockLikeACrazyMan)
            {
                HXBOOL setProcessWorkingSize = SetProcessWorkingSetSize(g_hCurrentProcess, (1<<23), (1<<24) );
            }

            g_PSAPIHandle = LoadLibrary("psapi.dll");

            if (g_PSAPIHandle)
            {
                g_fpInitializeProcessForWsWatch = (InitializeProcessForWsWatchType) GetProcAddress(g_PSAPIHandle, "InitializeProcessForWsWatch");
                g_fpGetWsChanges = (GetWsChangesType) GetProcAddress(g_PSAPIHandle, "GetWsChanges");
            }

            HXBOOL initRetVal = g_fpInitializeProcessForWsWatch(g_hCurrentProcess);
        }

        // check to see if we want to print data
        SafeSprintf(szKeyName, 256, "Software\\%s\\Debug\\PrintFaults", HXVER_COMMUNITY);
        if( RegOpenKey(HKEY_CLASSES_ROOT, szKeyName, &hKey) == ERROR_SUCCESS )
        {
            RegQueryValue(hKey, "", szValue, (long *)&szValueSize);
            g_bPrintChanges = !strcmp(szValue,"1");
            RegCloseKey(hKey);
        }
        else
        {
            RegCreateKey(HKEY_CLASSES_ROOT, szKeyName, &hKey);
            RegCloseKey(hKey);
        }
    }

    HXBOOL b = FALSE;

    if (g_fpGetWsChanges)
    {
        b = g_fpGetWsChanges(g_hCurrentProcess,&WorkingSetBuffer[0],sizeof(WorkingSetBuffer));
    }

    if ( b )
    {
        int i = 0;
        while (WorkingSetBuffer[i].FaultingPc)
        {
            void* Pc = WorkingSetBuffer[i].FaultingPc;
            void* Va = WorkingSetBuffer[i].FaultingVa;
            void* VaPage = (void*) ((ULONG)Va & 0xfffff000);

            g_totalFaults++;

            Va = (LPVOID)( (ULONG)Va & 0xfffffffe);
            count = 0;

            g_mWorkingSetMap.Lookup(VaPage, (void*&)count);
            count++;
            g_mWorkingSetMap.SetAt(VaPage, (void*)count);
            if (count==50 && g_bLockLikeACrazyMan)
            {
                g_AttemptedLockedPages++;
#if 0 /* do not attempt to lock - RA */
                HXBOOL bRetVal = VirtualLock(VaPage, 4096);
                if (!bRetVal)
                {
                    DWORD lastError = GetLastError();
                    void* lpMsgBuf;
                    FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                    NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
                    LocalFree( lpMsgBuf );
                    g_FailedLockedPages++;
                }
                else
                {
                    g_LockedPages++;
                }
#endif
            }
            i++;
        }
    }
}
#endif
#endif


STDMETHODIMP HXPlayer::QuickSeek(ULONG32 ulSeekTime)
{
    HX_RESULT theErr      = HXR_OK;
    HXBOOL    bFirstFrame;

    HXLOGL3(HXLOG_CORE, "HXPlayer[%p]::QuickSeek(%lu)",
            this, ulSeekTime);

    //If we are not paused, just change this into a normal seek.
    if( !m_bPaused )
    {
        return Seek(ulSeekTime);
    }
    
    m_pCoreMutex->Lock();
    m_ulCoreLockCount++;

    m_bQuickSeekMode = TRUE;

    //Find out if we are still waiting on the first frame
    //from the last seek. If so, don't honor this seek yet.
    bFirstFrame = IsFirstFrameDisplayed();
    
    if (!bFirstFrame && 
    (m_bIsBuffering || 
     (m_ulBufferingCompletionTime == 0) ||
     (((LONG32) (HX_GET_BETTERTICKCOUNT() - m_ulBufferingCompletionTime)) < MAX_QUICK_SEEK_POST_BUFFERING_WAIT)))
    {
        // We have a seek going already. Add this one to the queue.
        m_ulSeekQueue = ulSeekTime;
        theErr = HXR_OK;
    }
    else
    {
    m_ulBufferingCompletionTime = 0;
        theErr = SeekPlayer(ulSeekTime);
        m_ulSeekQueue = kNoValue;
    }
    
    m_ulCoreLockCount--;
    m_pCoreMutex->Unlock();

    return theErr;
}

HXBOOL HXPlayer::IsFirstFrameDisplayed(void)
{
    HXBOOL bFirstFrame = TRUE;

    CHXMapPtrToPtr::Iterator iter = m_pSourceMap->Begin();
    while(iter != m_pSourceMap->End() && bFirstFrame )
    {
        SourceInfo* pSourceInfo = (SourceInfo*)(*iter);
        bFirstFrame = (bFirstFrame && pSourceInfo->FirstFrameDisplayed());
        ++iter;
    }

    return bFirstFrame;
}

void HXPlayer::LeaveQuickSeekMode(HXBOOL bDoSeek)
{
    UINT32 ulSeekPoint = 0;

    if( m_bQuickSeekMode )
    {
        //We need to do a real Seek() at this point after leaving the quickseek
        //mode so that we buffer and deliver packets to the audio renderer.

        //Leave quickseek mode.
        m_bQuickSeekMode = FALSE;
        
        //Next, If there is an outstanding seek queued up we can just use that
        //time as the final seek, otherwise we need to seek to the current
        //player time.
        if( bDoSeek )
        {
            ulSeekPoint =  kNoValue != m_ulSeekQueue ?
                m_ulSeekQueue : m_pAudioPlayer->GetCurrentPlayBackTime();
            
            SeekPlayer(ulSeekPoint);
        }
        
        m_ulSeekQueue = kNoValue;
    }
}

