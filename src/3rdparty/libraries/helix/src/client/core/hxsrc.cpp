/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsrc.cpp,v 1.136 2009/05/04 14:24:31 joaquincab Exp $
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
#include "hxcom.h"
#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "prefdefs.h"
#include "plprefk.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxausvc.h"
#include "hxmon.h"
#include "hxclreg.h"
#include "hxgroup.h"
#include "hxsmbw.h"
#include "hxstrm.h"
#include "hxwin.h"


#include "hxcore.h"
#include "hxhyper.h"
#include "playhpnv.h"
#include "hxplugn.h"
#include "hxrendr.h"

#include "chxeven.h"
#include "chxelst.h"
#include "hxmap.h"
#include "hxrquest.h"
#include "hxmangle.h"
#include "hxtick.h"
#include "dbcs.h"

#include "hxstrutl.h"
#include "strminfo.h"
#include "timeval.h"
#include "statsmgr.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "srcinfo.h"
#include "corshare.h"
#include "upgrdcol.h"
#include "hxrasyn.h"
#include "hxaudstr.h"
#include "hxaudses.h"
#include "hxplugn.h"
#include "hxrendr.h"
#include "errdbg.h"
#include "hxtlogutil.h"
#include "hxplayvelocity.h"

#include "client_preroll_hlpr.h"

// will be taken out once flags are defined in a separate file
#include "rmfftype.h"   
#include "hxplay.h"
#include "hxcleng.h"
#include "hxsrc.h"

#include "hxmime.h"

#if defined(HELIX_FEATURE_DRM)
#include "hxdrmcore.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MIN_AUDIO_TURBO_PUSHDOWN MINIMUM_AUDIO_STARTUP_PUSHDOWN    // ms
#define MIN_VIDEO_TURBO_PUSHDOWN 90				   // ms

const UINT32 DefaultMinimumLatencyThreshold =  1500;  // ms
const UINT32 DefaultMinimumLatencyJitter   =  300;   // ms

HXSource::HXSource() :
	  m_lRefCount(0)
	, m_ulStreamIndex(0)
	, m_pPlayer (0)
	, m_pBufferManager(NULL)
	, m_pRegistry (0)
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
	, m_pStatsManager(NULL)
	, m_pStats (0)
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
	, m_pFileHeader(0)
	, m_rebufferStatus(REBUFFER_NONE)
	, m_bRTSPRuleFlagWorkAround(FALSE)
	, m_bReSetup(FALSE)
	, m_bFastStart(FALSE)
	, m_srcEndCode(END_UNKNOWN)
	, m_serverTurboPlay(TURBO_PLAY_UNKNOWN)
	, m_bSureStreamClip(FALSE)
	, m_pSourceInfo(NULL)
	, m_ulMaxBandwidth(4000)	// 4000kbps
	, m_ulDelay (0)
	, m_ulOriginalDuration(0)
	, m_ulOriginalDelay (0)
	, m_ulPrefetchDelay (0)
	, m_ulRegistryID (0)
	, m_ulMinAudioTurboPushDown(MIN_AUDIO_TURBO_PUSHDOWN)
	, m_ulMinVideoTurboPushDown(MIN_VIDEO_TURBO_PUSHDOWN)
	, m_ulRebufferingStartTix(0)
	, m_ulRebufferingCumulativeTime(0)
	, m_ulPlaybackStartedTix(0)
	, m_ulPlaybackStoppedTix(0)
	, m_bContinueWithHeaders(FALSE)
	, m_bDefaultAltURL(FALSE)
	, mLastError (HXR_OK)
	, m_ulPerfectPlayTime (0)
	, m_ulBufferedPlayTime(0)
	, m_ulStreamHeadersExpected(0)
	, m_nSeeking (0)
	, m_bPerfectPlayEntireClip(FALSE)
	, m_bCannotBufferEntireClip(FALSE)
	, m_bPerfectPlay(FALSE)
	, m_bBufferedPlay(FALSE)
	, m_bSeekedOrPaused(FALSE)
	, m_bClipTimeAdjusted(FALSE)
	, m_bInitialized (FALSE)
	, m_bIsPreBufferingDone(FALSE)
	, m_bAltURL(TRUE)
	, m_bReceivedData (FALSE)
	, m_bReceivedHeader(FALSE)
	, m_bInitialBuffering(TRUE)
	, m_bCustomEndTime(FALSE)	
	, m_bCustomDuration(FALSE)
	, m_bPaused (FALSE)  
	, m_bFirstResume(TRUE)
	, m_bResumePending(FALSE)
	, m_bIsActive(FALSE)

	, m_bDelayed(FALSE)
	, m_bLocked (FALSE)
	, m_bPartOfNextGroup(FALSE)
	, m_bPartOfPrefetchGroup(FALSE)
	, m_bPrefetch(FALSE)
	, m_bPerfectPlayAllowed (FALSE)
	, mSaveAsAllowed (FALSE)
	, mLiveStream (FALSE)
	, m_bRestrictedLiveStream (FALSE)
	, m_bNonSeekable(FALSE)
	, m_bHasSubordinateLifetime(FALSE)
        , m_bRenderersControlBuffering(FALSE)
	, m_bSourceEnd (FALSE)
	, m_bForcedSourceEnd(FALSE)
	, m_bIsPreBufferingStarted(FALSE)
	, m_bIsMeta(FALSE)
	, m_bRARVSource(FALSE)
	, m_bSeekInsideRecordControl(FALSE)
	, m_pushDownStatus(PUSHDOWN_NONE)
	, m_ulTurboStartActiveTime(0)
	, mFlags (0)
	, m_uNumStreams (0)
	, m_ulPreRollInMs (0)
	, m_ulPreRoll (0)
	, m_ulMaxPreRoll (0)
	, m_ulMaxLatencyThreshold(0)
	, m_ulAvgBandwidth (0)
	, m_ulDuration (0)
	, m_ulLastBufferingCalcTime(0)
	, m_uActiveStreams (0)
	, m_prefetchType(PrefetchUnknown)
	, m_ulPrefetchValue(0)
	, m_pszURL(NULL)
	, m_pURL(NULL)
	, m_bRebufferAudio(TRUE)
	, m_ulAcceleratedRebufferBaseTime(0)
	, m_ulAcceleratedRebufferFactor(0)
	, m_ulStartTime (0)
	, m_ulEndTime (0)
	, m_llLastExpectedPacketTime(0)
	, m_ulRestrictedDuration(0)
	, m_ulLossHack(0)
	, m_ulNumFakeLostPackets(0)
	, m_ulFirstPacketTime(0)
	, m_maxPossibleAccelRatio(4.0)	// I donno what's a good value?
#if defined(HELIX_FEATURE_STATS)
	, m_ulLivePlaybackIntervalPacketCount(0)
	, m_ullLivePlaybackIntervalCumulativeLatency(0)
	, m_ulLivePlaybackIntervalMinimumLatency(0x7fffffff)
	, m_ulLivePlaybackIntervalMaximumLatency(0)
#endif
	, m_ulLiveSyncOffset(0)
	, m_ulLastReportedPlayTime(0)
	, m_ulLastLatencyCalculation(0)
	, m_ulSourceStartTime(0)
	, m_pPreferences (0)
	, m_pScheduler (0)
	, m_pRequest(NULL)
	, m_pEngine (0)
	, m_pAudioStreamList(NULL)
#if defined(HELIX_FEATURE_AUTOUPGRADE)
        , m_pUpgradeCollection(NULL)
#endif /* HELIX_FEATURE_AUTOUPGRADE */
    , m_pASMSource(NULL)
    , m_pBackChannel(NULL)
    , m_pRecordControl(NULL)
    , m_bPlayFromRecordControl(FALSE)
    , m_ulRenderingDisabledMask(HXRNDR_DISABLED_NONE)
    , m_pRedirectURL(NULL)
    , m_pSDPURL(NULL)
    , m_bRedirectPending(FALSE)
    , m_pTransportTimeSink(NULL)
    , m_pSrcBufStats(NULL)
    , m_lRAStreamNumber(-1)
    , m_uLastBuffering(0)
    , m_uLastStatusCode(HX_STATUS_BUFFERING)
    , m_ulStartDataWait (0)
    , m_ulFirstDataArrives (0)
    , m_nSoundLevelOffset(0)
    , m_ulKeyFrameInterval(0)
    , m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
    , m_bKeyFrameMode(FALSE)
    , m_bAutoSwitch(FALSE)
    , m_pPlaybackVelocityResponse(NULL)
    , m_pPlaybackVelocity(NULL)
#if defined(HELIX_FEATURE_DRM)
    , m_pDRM(NULL)
    , m_bIsProtected(FALSE)
#endif
{
    mStreamInfoTable = new CHXMapLongToObj;
}

HXSource::~HXSource()
{
    // XXX moved this to the distuctor because we want to keep arround
    // the request until the renderers are close at which point we should
    // be released and deleted.
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pPlaybackVelocityResponse);
    HX_RELEASE(m_pPlaybackVelocity);
#if defined(HELIX_FEATURE_DRM)
    HX_RELEASE(m_pDRM);
#endif
    HX_VECTOR_DELETE(m_pszURL);
    HX_DELETE(mStreamInfoTable);
}

void
HXSource::Stop()
{
    m_pSourceInfo = NULL;    

    if (!m_ulPlaybackStoppedTix)
    {
        m_ulPlaybackStoppedTix = HX_GET_TICKCOUNT();
    }
    
    HX_DELETE (m_pURL);
    HX_DELETE (m_pBufferManager);

    if ( FAILED(mLastError) )
    {
        HX_RELEASE(m_pRequest);
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_pStatsManager)
    {
        m_pStatsManager->DoCleanup();
        HX_RELEASE(m_pStatsManager);
    }        
    HX_DELETE(m_pStats);    
    if (m_pRegistry && m_ulRegistryID)
    {
        m_pRegistry->DeleteById(m_ulRegistryID);
    }
    m_ulRegistryID = 0;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    DeleteAllEvents(); 

    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pPlayer);
    HX_RELEASE(m_pEngine);
}    

HX_RESULT HXSource::DoCleanup(EndCode endCode)
{
    m_ulDuration = 0;    
    m_ulPreRollInMs = 0;
    m_ulPreRoll = 0;
    m_ulAvgBandwidth = 0;
    m_ulLastBufferingCalcTime = 0;

    m_ulMaxBandwidth = 0;
    m_serverTurboPlay = TURBO_PLAY_UNKNOWN;

    m_bAltURL = FALSE;    
    m_bPaused = FALSE;
    m_bFirstResume = TRUE;
    m_bIsActive = FALSE;
    m_bResumePending = FALSE;
    m_bIsPreBufferingStarted = FALSE;
    m_bIsPreBufferingDone = FALSE;  
    m_bSeekedOrPaused = FALSE;
    m_bClipTimeAdjusted = FALSE;    
    m_bInitialBuffering = TRUE;
    ChangeRebufferStatus(REBUFFER_NONE);
    m_bReceivedData = FALSE;
    m_bReceivedHeader = FALSE;
    m_bHasSubordinateLifetime = FALSE;

    ReleaseAudioStreams(m_pAudioStreamList);
    HX_DELETE(m_pAudioStreamList);

    DeleteStreamTable();

    CHXSimpleList::Iterator lIter = m_HXStreamList.Begin();
    for (; lIter != m_HXStreamList.End(); ++lIter)
    {
        HXStream* pStream = (HXStream*) (*lIter);
        pStream->Release();
    }

    m_HXStreamList.RemoveAll();    

    HX_RELEASE(m_pFileHeader);    
    HX_RELEASE(m_pASMSource);
    HX_RELEASE(m_pBackChannel);
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    HX_RELEASE(m_pUpgradeCollection);
#endif /* HELIX_FEATURE_AUTOUPGRADE */

    m_bForcedSourceEnd  = FALSE;
    m_bSourceEnd        = FALSE;

    HX_DELETE(m_pRedirectURL);
    HX_DELETE(m_pSDPURL);
    m_bRedirectPending  = FALSE;
    HX_RELEASE(m_pTransportTimeSink);
    HX_RELEASE(m_pSrcBufStats);

#if defined(HELIX_FEATURE_RECORDCONTROL)
    if(m_pRecordControl)
    {
        m_pRecordControl->Cleanup();
    }
#endif /* HELIX_FEATURE_RECORDCONTROL */
    HX_RELEASE(m_pRecordControl);
    HX_RELEASE(m_pPlaybackVelocityResponse);
    HX_RELEASE(m_pPlaybackVelocity);

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your 
//              object.
//
STDMETHODIMP HXSource::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXStreamSource), (IHXStreamSource*)this },
            { GET_IIDHANDLE(IID_IHXStreamSource2), (IHXStreamSource2*) this },
            { GET_IIDHANDLE(IID_IHXPendingStatus), (IHXPendingStatus*)this },
            { GET_IIDHANDLE(IID_IHXInfoLogger), (IHXInfoLogger*)this },
            { GET_IIDHANDLE(IID_IHXPrivateStreamSource), (IHXPrivateStreamSource*)this },
            { GET_IIDHANDLE(IID_IHXRegistryID), (IHXRegistryID*)this },
            { GET_IIDHANDLE(IID_IHXClientRateAdaptControl), (IHXClientRateAdaptControl*)this },
            { GET_IIDHANDLE(IID_IHXSourceLatencyStats), (IHXSourceLatencyStats*)this },
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
            { GET_IIDHANDLE(IID_IHXHyperNavigate), (IHXHyperNavigate*)this },
            { GET_IIDHANDLE(IID_IHXHyperNavigate2), (IHXHyperNavigate2*)this },
#endif /* defined(HELIX_FEATURE_HYPER_NAVIGATE) */
#if defined(HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION)
            { GET_IIDHANDLE(IID_IHXAudioLevelNormalization), (IHXAudioLevelNormalization*)this },
#endif
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXStreamSource*)this },
            { GET_IIDHANDLE(IID_IHXTransportTimeManager), (IHXTransportTimeManager*)this},
            { GET_IIDHANDLE(IID_IHXPlaybackVelocity), (IHXPlaybackVelocity*)this}
        };
    
    HX_RESULT res = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    
    // if successful, return immediately...
    if (SUCCEEDED(res))
    {
        return res;
    }
    // ... otherwise proceed onward
    
    if (IsEqualIID(riid, IID_IHXBackChannel))
    {
        if (m_pBackChannel)
        {
            AddRef();
            *ppvObj = (IHXBackChannel*)this;
            return HXR_OK;
        }
        else
        {
            *ppvObj = NULL;
            return HXR_NOINTERFACE;
        }
    }
    else if (IsEqualIID(riid, IID_IHXASMSource))
    {
        if (m_pASMSource)
        {
            AddRef();
            *ppvObj = (IHXASMSource*)this;
            return HXR_OK;
        }
        else
        {
            *ppvObj = NULL;
            return HXR_NOINTERFACE;
        }
    }
    else if (m_pSrcBufStats &&
             IsEqualIID(riid, IID_IHXSourceBufferingStats3))
    {
        m_pSrcBufStats->AddRef();
        *ppvObj = m_pSrcBufStats;
        return HXR_OK;
    }

#if defined(HELIX_FEATURE_AUTOUPGRADE)
    else if (IsEqualIID(riid, IID_IHXUpgradeCollection))
    {
        if (!m_pUpgradeCollection)
        {
            m_pUpgradeCollection = new HXUpgradeCollection((IUnknown*)(IHXPlayer*)m_pPlayer);
            m_pUpgradeCollection->AddRef();
        }

        return m_pUpgradeCollection->QueryInterface(riid, ppvObj);
    }
#endif /* HELIX_FEATURE_AUTOUPGRADE */
    else if (m_pRequest &&
        m_pRequest->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pFileHeader &&
        m_pFileHeader->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_DRM)
    else if (m_pDRM &&
        m_pDRM->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_DRM */

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
STDMETHODIMP_(ULONG32) HXSource::AddRef()
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
STDMETHODIMP_(ULONG32) HXSource::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

// *** IHXStreamSource methods ***

/************************************************************************
 *      Method:
 *              IHXStreamSource::IsLive
 *      Purpose:
 *              Ask the source whether it is live
 *
 */
STDMETHODIMP_ (HXBOOL) HXSource::IsLive(void)
{
    return mLiveStream;
}

/************************************************************************
 *      Method:
 *          IHXStreamSource::GetPlayer
 *      Purpose:
 *          Get the interface to the player object of which the source is
 *          a part of.
 *
 */
STDMETHODIMP HXSource::GetPlayer(IHXPlayer* &pPlayer)
{
    pPlayer = m_pPlayer;
    if (pPlayer)
    {
        pPlayer->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXStreamSource::GetContext
 *      Purpose:
 *          Get the interface to the context object of which the source is
 *          a part of.
 *
 */
STDMETHODIMP HXSource::GetContext(IUnknown* &pContext)
{
    pContext = ((IUnknown*)(IHXClientEngine*) m_pEngine);
    if (pContext)
    {
        pContext->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXStreamSource::GetURL
 *      Purpose:
 *          Get the URL for this source. NOTE: The returned string is
 *          assumed to be valid for the life of the IHXStreamSource from which it
 *          was returned.
 *
 */
STDMETHODIMP_(const char*) HXSource::GetURL(void)
{
    const char* pURL = NULL;

    /* 
     * Make sure that the we have not been re-directed
     * to a new URL.
     */ 
    if (m_pRequest && 
        m_pRequest->GetURL(pURL) == HXR_OK &&
        pURL)
    {
        if (!m_pszURL ||
            ::strcasecmp(pURL, m_pszURL) != 0)
        {
            HX_VECTOR_DELETE(m_pszURL);
            m_pszURL = new char[strlen(pURL) + 1];
            strcpy(m_pszURL, pURL); /* Flawfinder: ignore */
        }
    }

    // append RAM mimeType as URL option to notify the
    // caller(i.e. ViewSource)
    if (m_bIsMeta)
    {
        char* pszTemp = NULL;

        pszTemp = ::HXFindChar(m_pszURL, '?');
        if (pszTemp)
        {
                int lenTemp = strlen(m_pszURL) + strlen("&mimeType=") + strlen(RAM_MIMETYPE) + 1;
            pszTemp = new char[lenTemp];
            SafeSprintf(pszTemp, lenTemp, "%s%s%s", m_pszURL, "&mimeType=", RAM_MIMETYPE); /* Flawfinder: ignore */
        }
        else
        {
                int lenTemp = strlen(m_pszURL) + strlen("?mimeType=") + strlen(RAM_MIMETYPE) + 1;
            pszTemp = new char[lenTemp];
            SafeSprintf(pszTemp, lenTemp, "%s%s%s", m_pszURL, "?mimeType=", RAM_MIMETYPE); /* Flawfinder: ignore */
        }

        HX_VECTOR_DELETE(m_pszURL);
        m_pszURL = pszTemp;
    }

    return m_pszURL;
}


HXBOOL
HXSource::IsRARVSource(void)
{
    HXBOOL            bResult = TRUE;
    STREAM_INFO*    pStreamInfo = NULL;

    CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
    for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
    {
        pStreamInfo = (STREAM_INFO*) (*lStreamIterator);

        if (!IsRARVStream(pStreamInfo->m_pHeader))
        {
            bResult = FALSE;
            break;
        }
    }

    return bResult;
}

HXBOOL
HXSource::IsRARVStream(IHXValues* pHeader)
{
    HXBOOL        bResult = FALSE;
    IHXBuffer*  pMimeType = NULL;

    if (pHeader &&
        HXR_OK == pHeader->GetPropertyCString("MimeType", pMimeType))
    {
        const char* pszMimeType = (const char*)pMimeType->GetBuffer();

        if (strcmp(pszMimeType, REALAUDIO_MIME_TYPE) == 0             ||
            strcmp(pszMimeType, REALAUDIO_MULTIRATE_MIME_TYPE) == 0   ||
            strcmp(pszMimeType, REALAUDIO_MULTIRATE_LIVE_MIME_TYPE) == 0)
        {
            bResult = TRUE;
            pHeader->GetPropertyULONG32("StreamNumber", (UINT32&)m_lRAStreamNumber);
        }
        // all "-encrypted" mimetypes should have been stripped in FixUpMime()
        else if (strcmp(pszMimeType, REALMEDIA_MIME_TYPE) == 0  ||
                 strcmp(pszMimeType, REALVIDEO_MIME_TYPE) == 0  ||
                 strcmp(pszMimeType, REALVIDEO_MULTIRATE_MIME_TYPE) == 0)
        {
            bResult = TRUE;
        }
    }
    HX_RELEASE(pMimeType);

    return bResult;
}

HX_RESULT HXSource::GetBufferingStatusFromRenderers(REF(UINT16)     rusStatusCode,
                                                    REF(IHXBuffer*) rpStatusDesc,
                                                    REF(UINT16)     rusPercentDone)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pSourceInfo && m_pSourceInfo->m_pRendererMap)
    {
        // Initialize to defaults
        rusStatusCode  = HX_STATUS_READY;
        rpStatusDesc   = NULL;
        rusPercentDone = 0;
        // Set up variables to track status across renderers
        HXBOOL     bAtLeastOneRendererInitializing = FALSE;
        HXBOOL     bAtLeastOneRendererContacting   = FALSE;
        HXBOOL     bAtLeastOneRendererBuffering    = FALSE;
        UINT16     usTotalPercentDone              = 0;
        UINT16     usNumRenderers                  = 0;
        // Now loop through all the renderers
        CHXMapLongToObj::Iterator itrRend; 
        for (itrRend =  m_pSourceInfo->m_pRendererMap->Begin();
             itrRend != m_pSourceInfo->m_pRendererMap->End();
             ++itrRend)
        {
            RendererInfo* pRendInfo = (RendererInfo*)(*itrRend);
            if (pRendInfo &&
                pRendInfo->m_pRendererPendingStatus &&
                pRendInfo->m_bRendererDrivenBuffering)
            {
                // Get the status from this renderer
                UINT16     usTmpStatusCode  = 0;
                IHXBuffer* pTmpStatusDesc   = NULL;
                UINT16     usTmpPercentDone = 0;
                retVal = pRendInfo->m_pRendererPendingStatus->GetStatus(usTmpStatusCode,
                                                                        pTmpStatusDesc,
                                                                        usTmpPercentDone);
                HX_RELEASE(pTmpStatusDesc);
                if (SUCCEEDED(retVal))
                {
                    // Is this renderer initializing?
                    if (usTmpStatusCode == HX_STATUS_INITIALIZING)
                    {
                        // This renderer is initializing, so set the flag
                        bAtLeastOneRendererInitializing = TRUE;
                    }
                    else if (usTmpStatusCode == HX_STATUS_CONTACTING)
                    {
                        // This renderer is contacting, so set the flag
                        bAtLeastOneRendererContacting = TRUE;
                    }
                    else if (usTmpStatusCode == HX_STATUS_BUFFERING)
                    {
                        // This renderer is buffering, so set the flag
                        bAtLeastOneRendererBuffering = TRUE;
                        // Add the percent done to the total percentage
                        usTotalPercentDone += usTmpPercentDone;
                        // Increment the number of renderers
                        usNumRenderers++;
                    }
                    else if (usTmpStatusCode == HX_STATUS_READY)
                    {
                        // Add 100% to the total percent done
                        usTotalPercentDone += 100;
                        // Increment the number of renderers
                        usNumRenderers++;
                    }
                }
                else
                {
                    // A renderer returned failure, so break out
                    break;
                }
            }
            else
            {
                // One of the renderers does not support IHXPendingStatus,
                // so we cannot get a status call across all renderers,
                // so we should return failure
                retVal = HXR_UNEXPECTED;
                break;
            }
        } // for (itrRend = m_pRendererMap->Begin(); itrRend != m_pRendererMap->End(); ++itrRend)
        if (SUCCEEDED(retVal))
        {
            // Do we have at least one renderer initializing?
            if (bAtLeastOneRendererInitializing)
            {
                // We have at least one renderer initializing, so we
                // will return an overall state of initializing.
                rusStatusCode  = HX_STATUS_INITIALIZING;
                rusPercentDone = 0;
            }
            else if (bAtLeastOneRendererContacting)
            {
                // We have at least one renderer contacting, so we
                // will return an overall state of contacting.
                rusStatusCode  = HX_STATUS_CONTACTING;
                rusPercentDone = 0;
            }
            else if (bAtLeastOneRendererBuffering)
            {
                // All the renderer were either buffering or
                // ready, but there was at least one that was
                // still buffering, so we will return an
                // overall state of buffering and calculate
                // the overall percentage between them.
                rusStatusCode  = HX_STATUS_BUFFERING;
                // Divide the total percentage by the number of renderers
                if (usNumRenderers)
                {
                    rusPercentDone = usTotalPercentDone / usNumRenderers;
                }
            }
            else
            {
                // The only thing left is that all the renderers
                // returned ready, so we will return an overall
                // state of ready.
                rusStatusCode  = HX_STATUS_READY;
                rusPercentDone = 0;
            }
        }
    }

    return retVal;
}

HX_RESULT
HXSource::IsFaststartPushdownFullfilled(REF(UINT16) uStatusCode,
                                        REF(UINT16) ulPercentDone)
{
#if defined(HELIX_FEATURE_TURBOPLAY)
    HX_RESULT           rc = HXR_OK;
    UINT32              ulWaitTime = 0;
    UINT32              ulPlayerTime  = 0;
    UINT32              ulPushdownMS = 0;
    UINT32              ulNumFrames =0;
    CHXAudioStream*     pCHXAudioStream = NULL;
    CHXAudioSession*    pAudioSession = NULL;
    HXStream*           pStream = NULL;
    HXAudioData         audioData;
    IUnknown*           pUnknown = NULL;
    IHXMediaPushdown*   pMediaPushdown = NULL;
    UINT32              ulAudioPushDownThreshold;
    UINT32              ulVideoPushDownThreshold;
    CHXSimpleList::Iterator i;

    // These locals hold the information about pushdown completing in this call
    // due to absence of gauge
    HXBOOL bAudioPushdownCompletedOnGaugeAbsence = FALSE;
    HXBOOL bVideoPushdownCompletedOnGaugeAbsence = FALSE;
    // If were able to determine eudio.video pushdown to be completed, this must 
    // mean we had a gauge mechanism available to determine this.
    HXBOOL bAudioGaugeAvailable = ((PUSHDOWN_AUDIO_DONE & m_pushDownStatus) != 0);
    HXBOOL bVideoGaugeAvailable = ((PUSHDOWN_VIDEO_DONE & m_pushDownStatus) != 0);

    uStatusCode = HX_STATUS_READY;
    ulPercentDone = 100;

    if (PUSHDOWN_ALL_DONE == m_pushDownStatus)
    {
        return rc;
    }

    if (!m_pPlayer)
    {
        return HXR_FAILED;
    }

    // both source and sourceinfo have to be initialized
    // since the logic below depends on the successful initialization
    // of renderer and audio streams.
    if (!m_bInitialized || !m_pSourceInfo->m_bInitialized)
    {
        uStatusCode = HX_STATUS_INITIALIZING;
        ulPercentDone = 0;
        return rc;
    }

    ulAudioPushDownThreshold = m_ulMinAudioTurboPushDown;       // in ms
    ulVideoPushDownThreshold = m_ulMinVideoTurboPushDown;       // in ms

    if (!(PUSHDOWN_AUDIO_DONE & m_pushDownStatus))
    {
#if defined(HELIX_FEATURE_AUDIO)
        if (!m_pAudioStreamList && (!m_pPlayer || !m_pPlayer->IsInQuickSeek()))
        {
            CollectAudioStreams(m_pAudioStreamList);
        }

        if (m_pAudioStreamList && m_pAudioStreamList->GetCount() &&
	    (!m_pPlayer || !m_pPlayer->IsInQuickSeek())
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
            && (!m_pPlayer || HX_IS_1X_PLAYBACK(m_pPlayer->GetVelocity()))
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
            )
        {
	    LONG32 lDiff = 0;

	    // We have means of measuring audio pushdown
	    bAudioGaugeAvailable = TRUE;

            memset(&audioData, 0, sizeof(HXAudioData));

            ulPlayerTime = m_pPlayer->GetCurrentPlayTime();

            if (mLiveStream)
            {
                ulPlayerTime += m_ulFirstPacketTime;
            }

            for(i = m_pAudioStreamList->Begin(); i != m_pAudioStreamList->End(); ++i)
            {
                pCHXAudioStream = (CHXAudioStream*)*i;
                pCHXAudioStream->Write(&audioData);

                // For live, ulPlayerTime could be > audioData.ulAudioTime if
                // the renderer hasn't send 1st audio packet to the audio stream
		lDiff = audioData.ulAudioTime - ulPlayerTime;

                if (lDiff >= ((LONG32) ulAudioPushDownThreshold))
                {
                    m_pushDownStatus = (PushDownStatus)(m_pushDownStatus | PUSHDOWN_AUDIO_DONE);
		    HXLOGL2(HXLOG_TRAN, "(%p) AudioPushDown Satisfied: Have=%ld Needed=%lu", 
			    this, 
			    lDiff, 
			    ulAudioPushDownThreshold);
		    HXLOGL3(HXLOG_CORP, "(%p) AudioPushDown Satisfied: Have=%ld Needed=%lu", 
			    this, 
			    lDiff, 
			    ulAudioPushDownThreshold);

                    break;
                }
            }
        }
        // no audio stream
        else
#endif /* HELIX_FEATURE_AUDIO */
        {
	    bAudioPushdownCompletedOnGaugeAbsence = TRUE;
        }
    }

    if (!(PUSHDOWN_VIDEO_DONE & m_pushDownStatus))
    {
	HXBOOL bRendererPresent = FALSE;

        // check video push down
        for (i = m_HXStreamList.Begin(); i != m_HXStreamList.End(); ++i)
        {
            pStream = (HXStream*) (*i);

            // there should be 1 renderer per stream
            if (HXR_OK == pStream->GetRenderer(0, pUnknown))
            {
		bRendererPresent = TRUE;

                if (HXR_OK == pUnknown->QueryInterface(IID_IHXMediaPushdown, (void**)&pMediaPushdown))
                {
		    // We have means of measuring video pushdown
		    bVideoGaugeAvailable = TRUE;

                    if (pStream->m_bPostSeekToBeSent)
                    {
                        goto cleanup;
                    }

                    pMediaPushdown->GetCurrentPushdown(ulPushdownMS, ulNumFrames);

                    if ((ulPushdownMS < ulVideoPushDownThreshold) &&
			((!m_pPlayer->IsInQuickSeek()) || (ulNumFrames == 0)))
                    {
                        goto cleanup;
                    }

		    pStream->SetBufferingFullfilled();
                }
                HX_RELEASE(pMediaPushdown);
            }
            HX_RELEASE(pUnknown);
        }

	// If renderers are not present, we cannot satisfy the pushdown using the
	// fast start logic. Renderers are required to deinterleave and decode
	// data and fill the post-decode buffers which in turn are used to determine
	// the sufficient level of buffering for the time-line to start.
	if (bRendererPresent)
	{
	    if (bVideoGaugeAvailable)
	    {
		m_pushDownStatus = (PushDownStatus)(m_pushDownStatus | PUSHDOWN_VIDEO_DONE);
		HXLOGL2(HXLOG_TRAN, "(%p) VideoPushDown Satisfied: Have=%lu Needed=%lu QueuedFrames=%lu", 
			this, 
			ulPushdownMS, 
			ulVideoPushDownThreshold, 
			ulNumFrames);
		HXLOGL3(HXLOG_CORP, "(%p) VideoPushDown Satisfied: Have=%lu Needed=%lu QueuedFrames=%lu", 
			this, 
			ulPushdownMS, 
			ulVideoPushDownThreshold, 
			ulNumFrames);
	    }
	    else
	    {
		bVideoPushdownCompletedOnGaugeAbsence = TRUE;
	    }
	}
    }

  cleanup:

    HX_RELEASE(pMediaPushdown);
    HX_RELEASE(pUnknown);

    // We do not to have at least one pushdown gauge to measure fast-start 
    // requirements, we cannot satosfy buffering based on fast-start logic.
    if (bAudioPushdownCompletedOnGaugeAbsence && bVideoGaugeAvailable)
    {
	m_pushDownStatus = (PushDownStatus)(m_pushDownStatus | PUSHDOWN_AUDIO_DONE);
        HXLOGL2(HXLOG_TRAN, "(%p) AudioPushDown Satisfied(no audio)", this);
	HXLOGL3(HXLOG_CORP, "(%p) AudioPushDown Satisfied(no audio)", this);
    }
    if (bVideoPushdownCompletedOnGaugeAbsence && bAudioGaugeAvailable)
    {
	m_pushDownStatus = (PushDownStatus)(m_pushDownStatus | PUSHDOWN_VIDEO_DONE);
	HXLOGL2(HXLOG_TRAN, "(%p) VideoPushDown Satisfied(no video pushdown gauge)", this);
	HXLOGL3(HXLOG_CORP, "(%p) VideoPushDown Satisfied(no video pushdown gauge)", this);
    }

    if (PUSHDOWN_ALL_DONE == m_pushDownStatus)
    {
        if (m_bInitialBuffering)
        {
            InitialBufferingDone();
        }
        m_uLastBuffering = 100;
        m_ulTurboStartActiveTime = HX_GET_TICKCOUNT();
        HXLOGL1(HXLOG_TRAN, "(%p) TURBO Started", this);
	HXLOGL3(HXLOG_CORP, "(%p) TURBO Started", this);
    }
    else
    {
        uStatusCode = HX_STATUS_BUFFERING;
        ulPercentDone = 0;

	rc = HXR_FAILED;

	// By failing here, we always use standard buffering as a fallback.
	// That is, the start-up time is triggered by either standard or
	// turbo-start logic whichever fires first.
    }

    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_TURBOPLAY */
}


HXBOOL
HXSource::CanBeFastStarted(void)
{
    // In low heap mode we always want TurboPlay off.
#if defined(HELIX_FEATURE_TURBOPLAY)

    HXStream*   pStream = NULL;
    CHXSimpleList::Iterator i;

    m_bFastStart = TRUE;

    if (FALSE == m_pPlayer->CanBeFastStarted(m_pSourceInfo))
    {
        m_turboPlayStats.tpOffReason = m_pPlayer->m_turboPlayOffReason;
        m_bFastStart = FALSE;
        goto cleanup;
    }

    if (ShouldDisableFastStart())
    {
        m_bFastStart = FALSE;
        goto cleanup;
    }

    // faststart can be disabled by the server
    if (TURBO_PLAY_OFF == m_serverTurboPlay)
    {
        HXLOGL1(HXLOG_TRAN, "(%p) Disabled By Server - TurboPlay Off", this);
        m_turboPlayStats.tpOffReason = TP_OFF_BY_SERVER;
        m_bFastStart = FALSE;
        goto cleanup;
    }

    // no faststart on ROB presentation
    if (m_pPlayer->m_pEngine->m_lROBActive > 0)
    {
        HXLOGL1(HXLOG_TRAN, "(%p) ROB Presentation - TurboPlay Off", this);
        m_turboPlayStats.tpOffReason = TP_OFF_BY_ROB;
        m_bFastStart = FALSE;
        goto cleanup;
    }

    if (m_bFastStart)
    {
        EnterFastStart();
    }

  cleanup:

    return m_bFastStart;
#else
    return FALSE;
#endif /* HELIX_FEATURE_TURBOPLAY */
}

HX_RESULT HXSource::GetRecordControl(REF(HXRecordControl*) rpControl)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pRecordControl)
    {
        HX_RELEASE(rpControl);
        rpControl = m_pRecordControl;
        rpControl->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

/************************************************************************
 *      Method:
 *          IHXStreamSource::GetStreamCount
 *      Purpose:
 *          Get the number of streams in this source
 *
 */
STDMETHODIMP_(UINT16)
    HXSource::GetStreamCount()
{
    return m_HXStreamList.GetCount();
}


/************************************************************************
 *      Method:
 *          IHXStreamSource::GetStream
 *      Purpose:
 *          Get the stream with the given index.
 *      Note: 
 *          Since stream numbers may not be zero based, stream index is not
 *          equal to the stream number. Actual stream number may be determined 
 *          from IHXStream::GetStreamNumber()
 *
 */
STDMETHODIMP
HXSource::GetStream
(
    UINT16 nIndex,
    REF(IUnknown*) pUnknown
    )
{
    LISTPOSITION lPosition = m_HXStreamList.FindIndex((int) nIndex);
    if (!lPosition)
    {
        pUnknown = NULL;
        return HXR_INVALID_PARAMETER;
    }

    HXStream* pStream = (HXStream*) m_HXStreamList.GetAt(lPosition);
    HX_ASSERT(pStream);

    return pStream->QueryInterface(IID_IUnknown,(void**)&pUnknown);
}

STDMETHODIMP HXSource::GetGroupIndex(REF(UINT16) rusGroupIndex)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pSourceInfo)
    {
        rusGroupIndex = m_pSourceInfo->m_uGroupID;
        retVal        = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP HXSource::GetTrackIndex(REF(UINT16) rusTrackIndex)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pSourceInfo)
    {
        rusTrackIndex = m_pSourceInfo->m_uTrackID;
        retVal        = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP HXSource::GetPersistentComponentID(REF(UINT32) rulID)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pSourceInfo && m_pSourceInfo->m_ulPersistentComponentID != MAX_UINT32)
    {
        rulID  = m_pSourceInfo->m_ulPersistentComponentID;
        retVal = HXR_OK;
    }

    return retVal;
}

// IHXPrivateStreamSource methods 
STDMETHODIMP_(HXBOOL) HXSource::IsSaveAllowed()
{
    return mSaveAsAllowed;
}


/*
 * IHXBackChannel method
 */

/************************************************************************
 *      Method:
 *          IHXBackChannel::PacketReady
 *      Purpose:
 *      A back channel packet sent from Renderer to FileFormat plugin.
 */
STDMETHODIMP
HXSource::PacketReady(IHXPacket* pPacket)
{
    HX_ASSERT(m_pBackChannel);

    if (m_pBackChannel)
    {
        return m_pBackChannel->PacketReady(pPacket);
    }

    return HXR_FAIL;
}


/*
 * IHXASMSource methods
 */

/************************************************************************
 *      Method:
 *          IHXASMSource::Subscribe
 *      Purpose:
 *      Called to inform a file format that a subscription has occurred,
 *          to rule number uRuleNumber, for stream uStreamNumber.
 */
STDMETHODIMP
HXSource::Subscribe(UINT16      uStreamNumber,
                     UINT16     uRuleNumber)
{
    HX_ASSERT(m_pASMSource);

    if (m_pASMSource)
    {
        return m_pASMSource->Subscribe(uStreamNumber, uRuleNumber);
    }

    return HXR_FAIL;
}

/************************************************************************
 *      Method:
 *          IHXASMSource::Unsubscribe
 *      Purpose:
 *      Called to inform a file format that a unsubscription has occurred,
 *          to rule number uRuleNumber, for stream uStreamNumber.
 */
STDMETHODIMP
HXSource::Unsubscribe(UINT16    uStreamNumber,
                       UINT16   uRuleNumber)
{
    HX_ASSERT(m_pASMSource);

    if (m_pASMSource)
    {
        return m_pASMSource->Unsubscribe(uStreamNumber, uRuleNumber);
    }

    return HXR_FAIL;
}

void    
HXSource::AddHXStream(HXStream* pStream)
{
    pStream->AddRef();
    m_HXStreamList.AddTail(pStream);

    UINT16 uStreamNumber = pStream->GetStreamNumber();
    STREAM_INFO* pStreamInfo;
    if (mStreamInfoTable->Lookup((LONG32) uStreamNumber, (void*& )pStreamInfo))
    {
        pStreamInfo->OnStream(pStream); 
        
        // Now that we have an HXStream object and
        // set pStreamInfo->m_pStream, we should 
        // use the cached rate adaptation state 
        // to update the rate adaptation state in
        // the HXStream object.
        if (pStreamInfo->m_bClientRateAdapt)
        {
            Enable(pStreamInfo->m_uStreamNumber);
        }
        else
        {
            Disable(pStreamInfo->m_uStreamNumber);
        }
    }
}

HX_RESULT
HXSource::CollectAudioStreams(CHXSimpleList*& pAudioStreamList)
{
    HX_RESULT       rc = HXR_OK;
    UINT16          i = 0;
    UINT16          uNumAudioStreams = 0;
    CHXAudioPlayer* pAudioPlayer = NULL;
    CHXAudioStream* pCHXAudioStream = NULL;
    IHXValues*      pHeader = NULL;

    pAudioStreamList = NULL;

    if (!m_pPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer = m_pPlayer->GetAudioPlayer();
    if (!pAudioPlayer)
    {
        goto cleanup;
    }

    uNumAudioStreams = pAudioPlayer->GetAudioStreamCount();
    for (i = 0; i < uNumAudioStreams; i++)
    {
        pCHXAudioStream = pAudioPlayer->GetCHXAudioStream(i);
        pCHXAudioStream->AddRef();

        pHeader = ((IHXAudioStream*)pCHXAudioStream)->GetStreamInfo();

        if (pHeader &&
            IsAudioStreamFromThisSource(pHeader))
        {
            if (!pAudioStreamList)
            {
                pAudioStreamList = new CHXSimpleList();
            }

            pAudioStreamList->AddTail(pCHXAudioStream);
        }
        else
        {
            HX_RELEASE(pCHXAudioStream);
        }
        HX_RELEASE(pHeader);    
    }

  cleanup:

    if (!pAudioStreamList)
    {    
        rc = HXR_FAILED;
    }

    return rc;
}

void            
HXSource::RemoveAudioStreams(void)
{    
    CHXAudioPlayer* pAudioPlayer = NULL;
    CHXSimpleList*  pAudioStreamList = NULL;

    HXLOGL3(HXLOG_CORP, 
            "HXSource[%p]::RemoveAudioStreams(): Start", 
            this);

    if (!m_pPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer = m_pPlayer->GetAudioPlayer();
    if (!pAudioPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer->AddRef();

    if (HXR_OK == CollectAudioStreams(pAudioStreamList) && pAudioStreamList)
    {
        pAudioPlayer->ManageAudioStreams(pAudioStreamList, AUD_PLYR_STR_REMOVE);

        ReleaseAudioStreams(pAudioStreamList);
        HX_DELETE(pAudioStreamList);
    }

    SetStreamHeadersToResumed(FALSE);

    HX_RELEASE(pAudioPlayer);

  cleanup:

    HXLOGL3(HXLOG_CORP, 
            "HXSource[%p]::RemoveAudioStreams(): End", 
            this);
    return;
}

void
HXSource::PauseAudioStreams(void)
{
    CHXAudioPlayer* pAudioPlayer = NULL;
    CHXSimpleList*  pAudioStreamList = NULL;

    if (!m_pPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer = m_pPlayer->GetAudioPlayer();
    if (!pAudioPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer->AddRef();

    if (HXR_OK == CollectAudioStreams(pAudioStreamList) && pAudioStreamList)
    {
        pAudioPlayer->ManageAudioStreams(pAudioStreamList, AUD_PLYR_STR_PAUSE);

        ReleaseAudioStreams(pAudioStreamList);
        HX_DELETE(pAudioStreamList);
    }

    SetStreamHeadersToResumed(FALSE);

    HX_RELEASE(pAudioPlayer);

cleanup:

    return;
}

void
HXSource::ResumeAudioStreams(void)
{
    CHXAudioPlayer* pAudioPlayer = NULL;
    CHXSimpleList*  pAudioStreamList = NULL;

    if (!m_pPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer = m_pPlayer->GetAudioPlayer();
    if (!pAudioPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer->AddRef();

    if (HXR_OK == CollectAudioStreams(pAudioStreamList) && pAudioStreamList)
    {
        pAudioPlayer->ManageAudioStreams(pAudioStreamList, AUD_PLYR_STR_RESUME);

        ReleaseAudioStreams(pAudioStreamList);
        HX_DELETE(pAudioStreamList);
    }

    SetStreamHeadersToResumed(TRUE);

    HX_RELEASE(pAudioPlayer);

cleanup:

    return;
}

void
HXSource::ReleaseAudioStreams(CHXSimpleList* pAudioStreamList)
{
    if (pAudioStreamList && !pAudioStreamList->IsEmpty())
    {
        CHXSimpleList::Iterator lIter = pAudioStreamList->Begin();
        for (; lIter != pAudioStreamList->End(); ++lIter)
        {
            CHXAudioStream* pAudioStream = (CHXAudioStream*) (*lIter);
            HX_RELEASE(pAudioStream);
        }
    }

    return;
}

HX_RESULT HXSource::Init(HXPlayer * pPlayer, UINT32 unRegistryID)
{
    HX_RESULT   theErr = HXR_OK;
   
    m_pPlayer = pPlayer;
    HX_ADDREF(m_pPlayer);
    if (m_pPlayer->QueryInterface(IID_IHXPreferences, (void **) &m_pPreferences) != HXR_OK)
    {
        theErr = HXR_INVALID_PARAMETER; //HX_INVALID_INTERFACE;
    }

    m_pBufferManager = new CBufferManager(this);
    if(!m_pBufferManager)
    {
        theErr = HXR_OUTOFMEMORY;
    }

    // create registry entries
    if (HXR_OK != m_pPlayer->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
    {
        m_pRegistry = NULL;
    }
    else
    {
        m_ulRegistryID = unRegistryID;
    }

    if (!theErr &&
        m_pPlayer->QueryInterface(IID_IHXScheduler, (void **) &m_pScheduler) != HXR_OK)
    {
        theErr = HXR_INVALID_PARAMETER; //HX_INVALID_INTERFACE;
    }

    IHXPreferences* pPref = NULL;

    m_pPlayer->QueryInterface(IID_IHXPreferences, (void**)&pPref);

    ReadPrefUINT32(pPref, "MinAudioTurboPushdown", m_ulMinAudioTurboPushDown);
    ReadPrefUINT32(pPref, "MinVideoTurboPushdown", m_ulMinVideoTurboPushDown);

    HX_RELEASE(pPref);

    m_pEngine = m_pPlayer->m_pEngine;
    m_pEngine->AddRef();

    return theErr;
}

HX_RESULT HXSource::SetupRegistry()
{
    HX_RESULT   theErr = HXR_OK;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    UINT32      ulRepeatedRegistryID = 0;
    IHXBuffer*  pRepeatRegName = NULL;
    char        szSourceName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

    // Can be NULL in case where persistent component is being removed early:
    if (!m_pSourceInfo)
    {
        return HXR_FAIL;
    }

    if (m_pSourceInfo->m_bLeadingSource &&
        !m_pSourceInfo->m_pRepeatList)
    {
        m_pStats = new SOURCE_STATS((IUnknown*)(IHXPlayer*)m_pPlayer, m_ulRegistryID);
    }
    else if (m_pRegistry && 
            HXR_OK == m_pRegistry->GetPropName(m_pPlayer->m_ulRepeatedRegistryID, pRepeatRegName))
    {
        SafeSprintf(szSourceName, MAX_DISPLAY_NAME, "%s.%ld%ld%ld", 
                pRepeatRegName->GetBuffer(),
                m_pSourceInfo->m_uGroupID,
                m_pSourceInfo->m_uTrackID,
                (int)m_pSourceInfo->m_bLeadingSource);
    
        ulRepeatedRegistryID = m_pRegistry->GetId(szSourceName);
        if (!ulRepeatedRegistryID)
        {
            ulRepeatedRegistryID = m_pRegistry->AddComp(szSourceName);
        }

        m_pStatsManager = new StatsManager(m_pRegistry, m_ulRegistryID, ulRepeatedRegistryID);
        m_pStatsManager->AddRef();

        m_pStats = new SOURCE_STATS((IUnknown*)(IHXPlayer*)m_pPlayer, ulRepeatedRegistryID);
        if( !m_pStats )
        {
            theErr = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        // why stats' creation failed??
        HX_ASSERT(FALSE);
    }

    HX_RELEASE(pRepeatRegName);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return theErr;
}

HX_RESULT HXSource::ReadPreferences(void)
{
    UINT32      un16Temp = 10;

    ReadPrefUINT32(m_pPreferences, "PerfectPlayTime", un16Temp); 
    m_ulPerfectPlayTime = un16Temp;

    ReadPrefUINT32(m_pPreferences, "BufferedPlayTime", un16Temp);
    m_ulBufferedPlayTime = un16Temp;

    ReadPrefBOOL(m_pPreferences, "PerfPlayEntireClip", m_bPerfectPlayEntireClip); 

    return HXR_OK;
}


HX_RESULT HXSource::DeleteStreamTable(void)
{
    HX_RESULT theErr = HXR_OK;

    STREAM_INFO * sInfo = NULL;
        
    for (CHXMapLongToObj::Iterator i = mStreamInfoTable->Begin();
         i != mStreamInfoTable->End(); ++i) 
    {    
        sInfo = (STREAM_INFO*) (*i);
        
        if (sInfo)
        {
            HX_DELETE (sInfo);
        }
    }

    mStreamInfoTable->RemoveAll();
    m_ulStreamIndex = 0;
    m_uNumStreams = 0;
    
    return theErr;
}       


IHXValues* HXSource::GetHeaderInfo(UINT16 stream_number)
{
    STREAM_INFO * pStreamInfo;
    if (mStreamInfoTable->Lookup((LONG32) stream_number, (void*& )pStreamInfo))
    {
        return pStreamInfo->m_pHeader;
    }
    else
    {
        return NULL;
    }
}

HX_RESULT       
HXSource::GetStreamHeaderInfo(UINT16 index, IHXValues*& hdr)
{
    HX_TRACE("HXSource::GetStreamHeaderInfo");
    
    // sanity check
    if (index >= m_uNumStreams)
    {
        return HXR_INVALID_PARAMETER; // HX_INVALID_INDEX;
    }
    
    CHXMapLongToObj::Iterator i = mStreamInfoTable->Begin();
    for (UINT16 j=0; j < index; j++)
    {
        ++i;
    }

    STREAM_INFO* pStreamInfo = (STREAM_INFO*) *i;

    hdr = pStreamInfo->m_pHeader;
    if (hdr)
    {
        hdr->AddRef();
    }

    return HXR_OK;
}


void    HXSource::SetFlags(UINT16 flags)
{
    mFlags = flags;

    if (mFlags & HX_PERFECT_PLAY_ENABLED)
    {
        m_bPerfectPlayAllowed = TRUE;
    }
    else
    {
        m_bPerfectPlayAllowed = FALSE;
    }

    if (mFlags & HX_SAVE_ENABLED)
    {
        mSaveAsAllowed = TRUE;
    }
    else
    {
        mSaveAsAllowed = FALSE;
    }
}

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
HX_RESULT HXSource::CopyMetaDataToRegistry(IHXValues* pHeader)
{
    // get Request Object
    char pszServerMetaData[256] = {0}; /* Flawfinder: ignore */
    char pszMetaData[256] = {0}; /* Flawfinder: ignore */

    IHXValues* pReqHeaders = NULL;
    IHXRequest* pRequest = NULL;
    IHXBuffer* pBuffer = NULL;
    char szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    char buff[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    char szSMDKey[256]; /* Flawfinder: ignore */
    ULONG32 ulValue = 0;
    IHXBuffer* pParentName = NULL;
    UINT32 regid = 0;
    const char szServerMetaData[] = {"ServerMetaData"};


    if (HXR_OK == m_pRegistry->GetPropName(m_pStats->m_ulRegistryID, pParentName))
    {
        SafeStrCpy(buff, (const char*) pParentName->GetBuffer(), MAX_DISPLAY_NAME);
        char* pDot = strrchr(buff, '.');
        if (pDot)
        {
            *pDot = '\0';
        }
        SafeStrCpy(szSMDKey, buff, 256);

        if (HXR_OK == GetRequest(pRequest))
        {
            // get request headers
            if ((HXR_OK == pRequest->GetRequestHeaders(pReqHeaders)) && pReqHeaders)
            {
                // look for the meta data properties
                if (HXR_OK == pReqHeaders->GetPropertyCString("AcceptMetaInfo", pBuffer))
                {
                    SafeStrCpy(pszMetaData, (char*) pBuffer->GetBuffer(), 256);
                    HX_RELEASE(pBuffer);

                    // look for comma delimited entries
                    const char* pCharEntryStart = pszMetaData;
                    const char* pCharEntryEnd = pCharEntryStart;

                    while (pCharEntryEnd && *pCharEntryEnd)
                    {
                        ++pCharEntryEnd;
                        if (*pCharEntryEnd == ',' || !*pCharEntryEnd)
                        {
                            // copy next prop request into buffer 
                            strncpy(buff, pCharEntryStart, (UINT32)pCharEntryEnd - (UINT32)pCharEntryStart); /* Flawfinder: ignore */
                            *(buff+(UINT32)pCharEntryEnd - (UINT32)pCharEntryStart) = '\0';

                            // see if this prop is in file header (it should be!)
                            if (HXR_OK == pHeader->GetPropertyCString(buff, pBuffer))
                            {
                                // create new registry entry
                                SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.%s", pParentName->GetBuffer(), buff);
                                regid = m_pRegistry->GetId(szRegKeyName);
                                if (!regid)
                                {
                                    m_pRegistry->AddStr(szRegKeyName, pBuffer);
                                }
                                else
                                {
                                    // set new value
                                    m_pRegistry->SetStrByName(szRegKeyName, pBuffer);
                                }

                                HX_RELEASE(pBuffer);
                            }
                            else if (HXR_OK == pHeader->GetPropertyULONG32(buff, ulValue))
                            {
                                // create new registry entry
                                SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.%s", pParentName->GetBuffer(), buff);

                                regid = m_pRegistry->GetId(szRegKeyName);
                                if (!regid)
                                {
                                    m_pRegistry->AddInt(szRegKeyName, ulValue);
                                }
                                else
                                {
                                    // set new value
                                    m_pRegistry->SetIntByName(szRegKeyName, ulValue);
                                }
                            }

                            pCharEntryStart = pCharEntryEnd + 1;
                        }
                    }
                }

                // look for the meta data properties
                if (HXR_OK == pReqHeaders->GetPropertyCString("AcceptServerMetaData", pBuffer))
                {
                    SafeStrCpy(pszServerMetaData, (char*) pBuffer->GetBuffer(), 256);
                    HX_RELEASE(pBuffer);

                    // first make sure we have a composit key for "ServerMetaData"
                    SafeSprintf(buff, MAX_DISPLAY_NAME, "%s.%s", szSMDKey, szServerMetaData);
                    regid = m_pRegistry->GetId(buff);
                    if (!regid)
                    {
                        m_pRegistry->AddComp(buff);
                    }
                    SafeStrCpy(szSMDKey, buff, 256);

                    // look for comma delimited entries
                    const char* pCharEntryStart = pszServerMetaData;
                    const char* pCharEntryEnd = pCharEntryStart;

                    while (pCharEntryEnd && *pCharEntryEnd)
                    {
                        ++pCharEntryEnd;
                        if (*pCharEntryEnd == ',' || !*pCharEntryEnd)
                        {
                            // copy next prop request into buffer 
                            strncpy(buff, pCharEntryStart, (UINT32)pCharEntryEnd - (UINT32)pCharEntryStart); /* Flawfinder: ignore */
                            *(buff+(UINT32)pCharEntryEnd - (UINT32)pCharEntryStart) = '\0';

                            // see if this prop is in file header (it should be!)
                            if (HXR_OK == pHeader->GetPropertyCString(buff, pBuffer))
                            {
                                // create new registry entry (if one does not exist)
                                SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.%s", szSMDKey, buff);
                                regid = m_pRegistry->GetId(szRegKeyName);
                                if (!regid)
                                {
                                    m_pRegistry->AddStr(szRegKeyName, pBuffer);
                                }
                                else
                                {
                                    // set new value
                                    m_pRegistry->SetStrByName(szRegKeyName, pBuffer);
                                }

                                HX_RELEASE(pBuffer);
                            }
                            else if (HXR_OK == pHeader->GetPropertyULONG32(buff, ulValue))
                            {
                                // create new registry entry
                                SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.%s", szSMDKey, buff);
                                regid = m_pRegistry->GetId(szRegKeyName);
                                if (!regid)
                                {
                                    m_pRegistry->AddInt(szRegKeyName, ulValue);
                                }
                                else
                                {
                                    // set new value
                                    m_pRegistry->SetIntByName(szRegKeyName, ulValue);
                                }
                            }

                            pCharEntryStart = pCharEntryEnd + 1;
                        }
                    }
                }

                HX_RELEASE(pReqHeaders);
            }

            HX_RELEASE(pRequest);
        }

        HX_RELEASE(pParentName);
    }

    return HXR_OK;
}
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

HX_RESULT HXSource::SetPlayTimes(UINT32 ulStartTime, 
                                  UINT32 ulEndTime, 
                                  UINT32 ulDelay, 
                                  UINT32 ulDuration)
{
    m_ulStartTime           = ulStartTime;
    m_ulEndTime             = ulEndTime;
    /* We do not handle Delay field for now */
    m_ulDelay               = ulDelay;
    m_ulRestrictedDuration  = ulDuration;

    if (m_ulEndTime > 0)
    {
        m_bCustomEndTime = TRUE;
    }
    else
    {
        m_bCustomEndTime = FALSE;
    }

    return HXR_OK;
}

HX_RESULT
HXSource::UpdatePlayTimes(IHXValues* pValues)
{
    HX_RESULT   rc = HXR_OK;
    char        szStart[] = "Start";
    char        szEnd[] = "End";
    char        szDelay[] = "Delay";
    char        szDuration[] = "Duration";
    UINT32      ulStartTime = 0;
    UINT32      ulEndTime = 0;
    UINT32      ulDelay = 0;
    UINT32      ulRestrictedDuration = 0;

    pValues->GetPropertyULONG32(szStart, ulStartTime);
    pValues->GetPropertyULONG32(szEnd, ulEndTime);
    pValues->GetPropertyULONG32(szDelay, ulDelay);
    pValues->GetPropertyULONG32(szDuration, ulRestrictedDuration);

    if (ulStartTime             != m_ulStartTime    ||
        ulEndTime               != m_ulEndTime      ||
        ulDelay                 != m_ulDelay        ||
        ulRestrictedDuration    != m_ulRestrictedDuration)
    {
        SetPlayTimes(ulStartTime, ulEndTime, ulDelay, ulRestrictedDuration);
        rc = AdjustClipTime();
    }

    return rc;
}

ULONG32 
HXSource::GetPerfectPlayTime(void)
{
    ULONG32 ulCurrentPlayTime = m_pPlayer ? m_pPlayer->GetCurrentPlayTime() : 0;
    ULONG32 result            = m_bPerfectPlay ? m_ulPerfectPlayTime : m_ulBufferedPlayTime;

    if (m_ulDuration != 0)
    {
        // mPlaybackLength is in Milliseconds, we want to calculate
        // playback in seconds...
        ULONG32 playbackTimeLeftInClip = 
            ((m_ulDuration > ulCurrentPlayTime ? m_ulDuration - ulCurrentPlayTime : 0) /1000)+1;

        /* Perfect Play entire clip ONLY if user has chosen PerfectPlay specifically
         * If we internally go in buffered play mode, always use perfect play time
         * setting.
         */
        if (m_bPerfectPlay)
        {
            if (m_bPerfectPlayEntireClip)
            {
                result = playbackTimeLeftInClip;
            }
            else
            {
                result = HX_MIN(m_ulPerfectPlayTime, playbackTimeLeftInClip);
            }
        }
        else // if (m_bBufferedPlay)
        {
            result = HX_MIN(m_ulBufferedPlayTime, playbackTimeLeftInClip);
        }
    }
    
    // check if enough memory is available to handle the result
    if(result > 0)
    {
        // get 50% of the available memory
        ULONG32 maxMemAvail = GetAvailableMemory() / 2;
        ULONG32 bytesPerSecond = m_ulAvgBandwidth/8;
        ULONG32 maxTimeAvail = maxMemAvail/(bytesPerSecond > 0 ? bytesPerSecond : 1);

        // if this is true then we can't buffer the entire clip
        if (maxTimeAvail < result)
                m_bCannotBufferEntireClip = (m_bPerfectPlayEntireClip) ? TRUE : FALSE;
        else
                m_bCannotBufferEntireClip = FALSE;

        result = HX_MIN(result,maxTimeAvail);
    }

    /* Value now returned in ms. */
    return 1000*(HX_MAX(result, PERFECTPLAY_MIN_TIME));
}


//Returns TRUE if rebuffer was issued.
HXBOOL HXSource::DoRebufferIfNeeded()
{
    HXBOOL retVal = FALSE;

    //Check to see if it is already too late for any kind of accelerated
    //delivery. We only do this for audio streams.
    if( m_pPlayer && m_pPlayer->m_pAudioPlayer )
    {
        // We only force a rebuffer is a renderer has called ReportRebufferStatus()
        // to put us in a accelerated delivery mode.
        if( m_pPlayer->IsPlaying() && REBUFFER_WARNING == m_rebufferStatus )
        {
            CHXAudioSession* pSession = m_pPlayer->m_pAudioPlayer->GetOwner();
            HX_ASSERT( pSession);
            if( pSession && pSession->GoingToUnderflow() )
            {
                HXLOGL2(HXLOG_TRAN,
                        "(%p) HXSource::DoRebufferIfNeeded Rebuffering to prevent underflow",
                        this );
                DoRebuffer();
                retVal = TRUE;
            }
        }
    }
    return retVal;
}

HX_RESULT
HXSource::ReportRebufferStatus(UINT16   uStreamNumber,
                               HXBOOL     bAudio,
                               UINT8    unNeeded, 
                               UINT8    unAvailable) 
{
    STREAM_INFO*     pStreamInfo = NULL;
    HX_RESULT        res         = HXR_UNEXPECTED;
    CHXAudioSession* pSession    = NULL;
    
    HXLOGL2(HXLOG_TRAN, "(%p) HXSource::ReportRebufferStatus(%u,%lu,%u,%u)", 
            this, uStreamNumber, bAudio, unNeeded, unAvailable);

    if( m_bDelayed )
    {
        res = HXR_OK;
        return res;
    }

    if( mStreamInfoTable->Lookup((LONG32) uStreamNumber, (void*& )pStreamInfo) )
    {
        pStreamInfo->m_unNeeded    = unNeeded;
        pStreamInfo->m_unAvailable = unAvailable;
    }
    else
    {
        //XXXgfw is this an error at all?
        HX_ASSERT("Bad stream number in HXSource::ReportRebufferStatus" == NULL );
    }
    
    if ( unNeeded > unAvailable )
    {
        m_bRebufferAudio = bAudio;

        if( REBUFFER_REQUIRED != m_rebufferStatus )
        {
            if( bAudio )
            {
                res = StartAcceleratingEventDelivery();
            }
            else
            {
                //Video stream's only do the hard rebuffering, no accel delivery.
                HXLOGL2(HXLOG_TRAN,
                        "(%p) HXSource::ReportRebufferStatus: Rebuffering for video stream",
                        this );
                DoRebuffer();
            }
        }
    }
    else
    {
        if( IsRebufferDone() )
        {
            StopAcceleratingEventDelivery(REBUFFER_NONE);
            m_bRebufferAudio = FALSE;
            LogInformation("BUFEND", NULL);
        }
    }

    DoRebufferIfNeeded();

    res = HXR_OK;
    return res;
}

STDMETHODIMP 
HXSource::SetGranularity
(
    UINT16 uStreamNumber,
    ULONG32 ulGranularity
    ) 
{
    STREAM_INFO* pStreamInfo = 0;

    if (mStreamInfoTable->Lookup((LONG32) uStreamNumber, (void*& )pStreamInfo))
    {
        m_pPlayer->SetGranularity(this, uStreamNumber, ulGranularity);
        return HXR_OK;
    }
    else
    {
        return HXR_OK;
    }

    return HXR_UNEXPECTED;
}

HXBOOL
HXSource::IsPassedResumeTime(UINT32 ulAdditionalResumeLeadTime)
{
    HXBOOL bPassedResumeTime;
    UINT32 ulResumeTime = 0;
    UINT32 ulCurrentTime = 0;

    if (m_pPlayer)
    {
	ulCurrentTime = m_pPlayer->GetInternalCurrentPlayTime();
    }
    
    if (m_ulDelay > (m_ulPreRollInMs + ulAdditionalResumeLeadTime))
    {
	ulResumeTime = m_ulDelay - (m_ulPreRollInMs + ulAdditionalResumeLeadTime);
    }

    bPassedResumeTime = (ulCurrentTime >= ulResumeTime);

    return bPassedResumeTime;
}

HXBOOL
HXSource::TryResume(void)
{
    HXBOOL    bResume = FALSE;

    // resume the persistent source ASAP
    if (m_pSourceInfo && m_pSourceInfo->m_bIsPersistentSource)
    {
        m_bDelayed = FALSE;
        bResume = TRUE;
    }
    else if (m_bPaused && m_bDelayed && m_pPlayer &&
             m_pPlayer->CanBeStarted(this, m_pSourceInfo, m_bPartOfNextGroup))
    {
	if (IsPassedResumeTime(NETWORK_FUDGE_FACTOR))
	{
	    m_bDelayed = FALSE;
	    bResume = TRUE;
	}
        
        if (!m_bIsPreBufferingStarted)
        {
            m_bIsPreBufferingStarted = TRUE;
            bResume = TRUE;
        }
    }
    else if (m_bPrefetch)
    {
        bResume = TRUE;
    }

    if (bResume && CanBeResumed())
    {
        if (m_pSourceInfo)
        {
            m_pSourceInfo->Register();
        }

        m_bResumePending = TRUE;
    }

    return bResume;
}

HXBOOL
HXSource::IsAnyAudioStream(void)
{
    IHXAudioPlayer* pAudioPlayer                = NULL;
    HXBOOL                bAtLeastOneAudioStream  = FALSE;

    if (!m_pPlayer)
    {
        return FALSE;
    }

    m_pPlayer->QueryInterface(IID_IHXAudioPlayer, (void **) &pAudioPlayer);
    UINT16 uNumAudioStreams = pAudioPlayer->GetAudioStreamCount();
    for (UINT16 i = 0; i < uNumAudioStreams && !bAtLeastOneAudioStream; i++)
    {
        IHXAudioStream* pAudioStream = pAudioPlayer->GetAudioStream(i);
        IHXValues* pHeader = pAudioStream->GetStreamInfo();
        pAudioStream->Release();
        if (!pHeader)
        {
            continue;
        }
        
        if (IsAudioStreamFromThisSource(pHeader))
        {
            bAtLeastOneAudioStream = TRUE;
            pHeader->Release();
            break;
        }

        pHeader->Release();
    }

    if (!bAtLeastOneAudioStream)
    {
        IHXValues* pHeader = NULL;
        int nStreamCnt = GetNumStreams();
        ULONG32     ulIsAudioStream = FALSE;

        for (int i=0; i<nStreamCnt && !bAtLeastOneAudioStream; i++)
        {
            GetStreamHeaderInfo(i, pHeader);

            pHeader->GetPropertyULONG32("IsAudioStream", ulIsAudioStream);
            if (ulIsAudioStream)
            {
                bAtLeastOneAudioStream = TRUE;
            }
            HX_RELEASE(pHeader);
        }
    }

    pAudioPlayer->Release();

    return bAtLeastOneAudioStream;
}

void
HXSource::EventReady(CHXEvent* pEvent)
{
    if (m_pPlayer)
    {
        m_pPlayer->EventReady(this, pEvent);
    }
}

HXBOOL 
HXSource::IsAudioStreamFromThisSource(IHXValues* pAudioHeader)
{
    HXBOOL bFound = FALSE;

    CHXMapLongToObj::Iterator ndxStreamIterator = mStreamInfoTable->Begin();    
    for (; ndxStreamIterator != mStreamInfoTable->End(); ++ndxStreamIterator) 
    {
        STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*ndxStreamIterator);
        if (pStreamInfo->m_pHeader == pAudioHeader)
        {
            bFound = TRUE;
            break;
        }
    }

    return bFound;
}

/*
 * Settting the stream header to resumed, allows any audio streams
 * created by any of the source renderers to be automatically placed
 * into the proper state (paused or resumed) upon initialization.
 */
void 
HXSource::SetStreamHeadersToResumed(HXBOOL bSetResumed)
{
    CHXMapLongToObj::Iterator ndxStreamIterator = mStreamInfoTable->Begin();    
    for (; ndxStreamIterator != mStreamInfoTable->End(); ++ndxStreamIterator) 
    {
        STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*ndxStreamIterator);
        if (pStreamInfo && pStreamInfo->m_pHeader)
        {
	    pStreamInfo->m_pHeader->SetPropertyULONG32("Resumed", bSetResumed ? 1 : 0);
        }
    }
}

ULONG32
HXSource::GetAvailableMemory()
{
    ULONG32 memAvail = 1000000;

#if   defined (__MWERKS__)
//  XXX Hack! We can't call any of the Memory Manager calls at interrupt time so we will have 
//  to change this to ask our interrupt safe memory allocator how much memory is free
    memAvail = 1000000;
#elif defined(_WINCE) && !defined(_WIN32_WCE_EMULATION)
    STORE_INFORMATION stInfo;
    GetStoreInformation(&stInfo);
    memAvail = stInfo.dwFreeSize;
#elif (_WIN32)
    MEMORYSTATUS status;
    status.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&status);
    memAvail = status.dwAvailPageFile;
#elif defined( _WINDOWS)
    memAvail = GetFreeSpace(0);
#elif defined (_UNIX)
    // XXXX Need to get this to compile.
    memAvail = 1000000;
#endif

    return memAvail;
}

void
HXSource::ReportError(HX_RESULT theErr)
{
    if (m_pPlayer)
    {
        m_pPlayer->ReportError(this, theErr);
    }
}


HX_RESULT               
HXSource::AdjustClipTime(HXBOOL bIsResumingFromPause /* =FALSE */)
{
    HX_RESULT theErr = HXR_OK;
    UINT32  ulOriginalDuration = m_ulDuration;        
    UINT32  ulTrackEndTime = 0;
    CHXMapLongToObj::Iterator i;
    HXBOOL    bCustomEndTimeSet = FALSE;
    UINT32 ulEffectiveRestrictedDuration = 0;

    if (m_bPartOfPrefetchGroup)
    {
        if (m_ulDelay)
        {
            m_ulPrefetchDelay = m_ulDelay;
            m_bDelayed = TRUE;
            AdjustClipBandwidthStats(FALSE);
            DoPause();
        }

        m_ulEndTime = 0;
        m_bCustomEndTime = FALSE;
        goto cleanup;
    }
        
    /* For a live stream, the only qualifiers that make sense
     * are Duration and Delay. All other qualifiers should be made 0
     */
    if (mLiveStream)
    {
        m_ulStartTime = 0;

        if (!m_bRestrictedLiveStream)
        {
            m_ulEndTime = 0;
        }
    }

    for (i = mStreamInfoTable->Begin(); i != mStreamInfoTable->End(); ++i) 
    {    
        STREAM_INFO*    pStreamInfo = (STREAM_INFO*) (*i);

        ulTrackEndTime = 0;

        HX_RESULT hrTemp = pStreamInfo->m_pHeader->GetPropertyULONG32("EndTime", ulTrackEndTime);

        if (HXR_OK == hrTemp && !m_bCustomEndTime)
        {    
	    // If source does not have an EndTime specified, the source's
	    // end time is the maximum of the stream end times.
            if (m_ulEndTime < ulTrackEndTime)
            {
                m_ulEndTime = ulTrackEndTime;
            }
            pStreamInfo->m_bCustomEndTime = TRUE;
        }
        else if (m_bCustomEndTime)
        {
	    // If source has end time specified, it takes precedence over
	    // stream end times.
            ulTrackEndTime = m_ulEndTime;
            pStreamInfo->m_bCustomEndTime = TRUE;
        }

        if (ulTrackEndTime > 0 && !mLiveStream)
        {
            pStreamInfo->m_pHeader->SetPropertyULONG32("TrackEndTime",
                                                        ulTrackEndTime);
            bCustomEndTimeSet = TRUE;
        }
    }
    
    // if max. duration is set on this source
    if (m_pSourceInfo && 
        m_pSourceInfo->m_ulMaxDuration)
    {
        if (m_ulRestrictedDuration)
        {           
            if (m_ulRestrictedDuration > m_pSourceInfo->m_ulMaxDuration)
            {
                m_ulRestrictedDuration = m_pSourceInfo->m_ulMaxDuration;
            }
        }
        else if (m_ulDuration)
        {
            if (m_ulDuration > m_pSourceInfo->m_ulMaxDuration)
            {
                m_ulRestrictedDuration = m_pSourceInfo->m_ulMaxDuration;
            }
        }
    }

    // By default, we always set the end time to be the duration of the clip,
    // but do not set end time if it was manually specified.
    if (!bCustomEndTimeSet && !mLiveStream)
    {
        m_ulEndTime = m_ulDuration;
    }

    /* Check if there is any end time */
    if ((m_ulEndTime < m_ulDuration) || m_bRestrictedLiveStream)
    {
        m_ulDuration = m_ulEndTime;
    }
    
    /* Is "Delay" specified too? */
    if (m_ulDelay > 0) 
    {
        /* Increase duration of this clip */
        m_ulDuration += m_ulDelay;
	
        // no need to pause delayed persistent component since Pause() will be called
        // on actual tracks' AdjustClipTime() if necessary
        if (m_pSourceInfo && !m_pSourceInfo->m_bIsPersistentSource &&
	    (!IsPassedResumeTime(NETWORK_FUDGE_FACTOR)))
        {
            m_bDelayed = TRUE;
            AdjustClipBandwidthStats(FALSE);
            DoPause();
        }
    }

    if (m_ulStartTime > 0) /* reduce duration by start time amount */
    {
        if (m_ulDuration > m_ulStartTime)
        {
            m_ulDuration -= m_ulStartTime;
        }
        else
        {
            /* This is bad case. We consider it invalid */
            m_ulDuration = 0;
        }
    }

    ulEffectiveRestrictedDuration = 0;
    // /Needed for PR 100608 fix:
    if (bIsResumingFromPause)
    {
        m_ulDuration = m_ulDuration - m_ulOriginalDelay + m_ulDelay;
        ulEffectiveRestrictedDuration = m_ulOriginalDuration;
    }

    /* We now allow to increase the default duration of the clip */
    if (m_ulRestrictedDuration > 0)
    {
        m_ulDuration = m_ulRestrictedDuration + m_ulDelay;

        if (!bCustomEndTimeSet && !mLiveStream)
        {
            m_ulEndTime = m_ulRestrictedDuration + m_ulStartTime;
        }
        if (mLiveStream && !m_bRestrictedLiveStream)
        {
            m_bRestrictedLiveStream = TRUE;
            m_ulEndTime = m_ulRestrictedDuration + m_ulStartTime;
        }
        
        if (m_ulEndTime > m_ulRestrictedDuration + m_ulStartTime)
        {
            m_ulEndTime = m_ulRestrictedDuration + m_ulStartTime;
        }
    }
    // Fixes PR 100608 where unpausing a track was overextending the
    // duration by the original delay amount.  We need to see if it is
    // paused and if it was paused for > 0 sec.  If so, use the original
    // duration as the restricted duration (if no m_ulRestrictedDuration
    // was set by an explicit dur or end in the SMIL) because the clip's
    // natural duration is being used and should not be extended:
    else if (ulEffectiveRestrictedDuration > 0)
    {
        m_ulDuration = ulEffectiveRestrictedDuration + m_ulDelay;

        if (mLiveStream && !m_bRestrictedLiveStream)
        {
            m_bRestrictedLiveStream = TRUE;
            m_ulEndTime = ulEffectiveRestrictedDuration + m_ulStartTime;
        }
        
        if (m_ulEndTime > ulEffectiveRestrictedDuration + m_ulStartTime)
        {
            m_ulEndTime = ulEffectiveRestrictedDuration + m_ulStartTime;
        } 
    } 

    // orig duration is active duration for this source -- time for which
    // this source lasts.
    if (m_ulDuration > m_ulDelay)
    {
        m_ulOriginalDuration = m_ulDuration - m_ulDelay;
    }
    else
    {
        m_ulOriginalDuration = 0;
    }

    if (m_pURL                                      &&
        rtspProtocol == m_pURL->GetProtocol()       &&
        m_llLastExpectedPacketTime != m_ulEndTime   &&
        !m_bFirstResume)
    {
        m_bRTSPRuleFlagWorkAround = TRUE;
    }
    /* If we receive a packet after this stream, we consider the stream to be done */
    m_llLastExpectedPacketTime  = CAST_TO_INT64 m_ulEndTime;

    // Seek to the starting position only if the source
    // has not been resumed yet
    //  NOTE: If m_ulStartTime==0, we still want to call DoSeek() if
    // m_bSeekOnLateBegin is TRUE since it will increase the seekTime to >0:
    if (m_bFirstResume  &&  (m_ulStartTime>0  ||
            (m_pSourceInfo  &&  m_pSourceInfo->m_bSeekOnLateBegin)) )
    {
        /* We will add m_ulStartTime in DoSeek() call*/
        theErr = DoSeek(0);
    }

    /* Update stream durations if required */
    for (i = mStreamInfoTable->Begin(); i != mStreamInfoTable->End(); ++i) 
    {    
        STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*i);
        if (m_ulStartTime > 0)
        {
            pStreamInfo->m_pHeader->SetPropertyULONG32("TrackStartTime",
                                            m_ulStartTime);
        }

        
        if (m_ulEndTime > 0 && 
            !mLiveStream    &&
            ((HXR_OK != pStreamInfo->m_pHeader->GetPropertyULONG32("TrackEndTime",
                                                                   ulTrackEndTime)) || 
	     (ulTrackEndTime > m_ulEndTime)))
        {
            pStreamInfo->m_pHeader->SetPropertyULONG32("TrackEndTime",
                                                        m_ulEndTime);
        }

        if (ulOriginalDuration != m_ulDuration)
        {
            pStreamInfo->m_ulDuration = m_ulDuration;
            pStreamInfo->m_pHeader->SetPropertyULONG32("Duration", 
                                        pStreamInfo->m_ulDuration);
        }

        if (m_ulDelay > 0)
        {
            pStreamInfo->m_pHeader->SetPropertyULONG32("Delay", m_ulDelay);
        }
    }

//{FILE* f1 = ::fopen("d:\\temp\\url.txt", "a+"); ::fprintf(f1, "%p %s %lu %lu\n", this, m_pszURL, m_ulDelay, m_ulDuration);::fclose(f1);}

  cleanup:

    m_bClipTimeAdjusted = TRUE;

    return theErr;
}

void
HXSource::GenerateFakeLostPacket(CHXEvent*& theEvent)
{
    IHXPacket* pPacket = theEvent->GetPacket();
    CHXPacket* pLostPacket = new CHXPacket;
    pLostPacket->AddRef();
    pLostPacket->Set(0, 0, pPacket->GetStreamNumber(), 0, 0);
    pLostPacket->SetAsLost();

    /* Create a new event with lost packet */
    CHXEvent* pEvent = new CHXEvent(pLostPacket);
    pEvent->SetTimeStartPos(theEvent->GetTimeStartPos());
    pEvent->SetTimeOffset(theEvent->GetTimeOffset());
    pEvent->SetPreSeekEvent(theEvent->IsPreSeekEvent());
    pLostPacket->Release();

    delete theEvent;
    theEvent = pEvent;
}

char*
HXSource::GetAltURL(HXBOOL& bDefault)
{
    char* pAltURL = NULL;

#if defined(HELIX_FEATURE_ALT_URL)
    if (m_pURL && !m_bInitialized && m_pSourceInfo)
    {
        // Get an alternate URL from the source info
        IHXBuffer* pTmp = NULL;
        HX_RESULT rv = m_pSourceInfo->GetAltURL(&pTmp, &bDefault);
        if (SUCCEEDED(rv))
        {
            // Get the URL string
            const char* pszAltURL = (const char*) pTmp->GetBuffer();
            if (pszAltURL)
            {
                // Get the URL strlen
                INT32 lAltURLLen = (INT32) strlen(pszAltURL);
                // Copy the alt URL string
                char* pszTmp = new char [lAltURLLen + 1];
                if (pszTmp)
                {
                    // Copy the string
                    strcpy(pszTmp, pszAltURL); /* Flawfinder: ignore */
                    // Assign the copied string
                    pAltURL = pszTmp;
                }
            }
        }
        HX_RELEASE(pTmp);
    }
#endif /* HELIX_FEATURE_ALT_URL */

    return pAltURL;
}

HXBOOL HXSource::HasAltURL()
{
    HXBOOL bRet = FALSE;

#if defined(HELIX_FEATURE_ALT_URL)
    if (m_pSourceInfo && !m_bInitialized)
    {
        bRet = m_pSourceInfo->HasAltURL();
    }
#endif /* #if defined(HELIX_FEATURE_ALT_URL) */

    return bRet;
}

HX_RESULT
HXSource::SetRequest(const CHXURL* pURL, HXBOOL bAltURL)
{
    HX_RESULT           hr = HXR_OK;
    IHXValues*          pValues = NULL;
    IHXValues*          pValuesInRequest = NULL;
    IHXGroup*           pGroup = NULL;
    IHXGroup2*          pGroup2 = NULL;
    IHXGroupManager*    pGroupManager = NULL;
    IHXCommonClassFactory* pCCF = NULL;

    if (m_pEngine)
    {
        m_pEngine->QueryInterface(IID_IHXCommonClassFactory, (void**) &pCCF);
        if (pCCF)
        {
            HX_RELEASE(m_pRequest);

            if (m_pPlayer)
            {
                m_pPlayer->GetActiveRequest(m_pRequest);

                if (m_pRequest)
                {
                    m_pPlayer->ResetActiveRequest();
                }
            }

#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
            if (m_pPlayer && m_pSourceInfo)
            {
                if (HXR_OK == m_pPlayer->QueryInterface(IID_IHXGroupManager, (void**)&pGroupManager))
                {
                    if (HXR_OK == pGroupManager->GetGroup(m_pSourceInfo->m_uGroupID, pGroup))
                    {
                        if (HXR_OK == pGroup->QueryInterface(IID_IHXGroup2, (void**)&pGroup2))
                        {
                            pGroup2->GetTrack2(m_pSourceInfo->m_uTrackID, pValues, pValuesInRequest);
                            UINT32 ulValue = 0;
                            char   szDuration[128] = {0}; /* Flawfinder: ignore */
                            IHXBuffer* pBuffer = NULL;
                            if (pValues && HXR_OK == pValues->GetPropertyULONG32("Duration", ulValue))
                            {
                                if (!pValuesInRequest)
                                {
                                    pCCF->CreateInstance(CLSID_IHXValues, (void**) &pValuesInRequest);
                                }
                                if (pValuesInRequest)
                                {
                                    SafeSprintf (szDuration, 128, "%lu", ulValue); /* Flawfinder: ignore */
                                    pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pBuffer);
                                    if (pBuffer)
                                    {
                                        pBuffer->Set((UCHAR*)szDuration, strlen(szDuration) + 1);
                                        pValuesInRequest->SetPropertyCString("Duration", pBuffer);
                                    }
                                    HX_RELEASE(pBuffer);
                                }
                                else
                                {
                                    hr = HXR_OUTOFMEMORY;
                                }
                            }
                        }
                        HX_RELEASE(pGroup2);
                    }
                    HX_RELEASE(pGroup);
                }
                HX_RELEASE(pGroupManager);
            }
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */

#if defined(HELIX_FEATURE_PLAYBACK_NET)
            IHXRegistry* pRegistry = NULL;   
            m_pEngine->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);

            hr = ::SetRequest(pURL->GetURL(), bAltURL, m_pPreferences, pRegistry, pValuesInRequest, pCCF, m_pRequest);
            HX_RELEASE(pRegistry);
#endif /* HELIX_FEATURE_PLAYBACK_NET */

            HX_RELEASE(pValues);
            HX_RELEASE(pValuesInRequest);

#if defined(HELIX_FEATURE_RECORDCONTROL)
            if(hr == HXR_OK && pURL->GetProtocol() != fileProtocol && pURL->GetProtocol() != unknownProtocol)
            {
                if(!m_pRecordControl)
                {
                    m_pRecordControl = new HXRecordControl((IHXPlayer*)m_pPlayer, (IHXStreamSource*)this);
                    if(m_pRecordControl)
                    {
                        m_pRecordControl->AddRef();
                        if(m_pRecordControl->IsValid())
                            m_bPlayFromRecordControl = m_pRecordControl->CanGetPackets();
                        else
                            HX_RELEASE(m_pRecordControl);
                    }
                }

                if(m_pRecordControl)
                {
                    m_pRecordControl->SetSource((IHXStreamSource*)this);
                }
                // If we are playing back via RTSP, then get
                // the IHXPlaybackVelocity interface from the
                // record control
                if (pURL->GetProtocol() == rtspProtocol &&
                    m_pRecordControl                    &&
                    m_bPlayFromRecordControl)
                {
                    // Get the HXRecordControl's IHXRecordSource
                    IHXRecordSource* pRecordSource = NULL;
                    HX_RESULT rv = m_pRecordControl->GetRecordSource(pRecordSource);
                    if (SUCCEEDED(rv))
                    {
                        // QI this member for IHXPlaybackVelocity
                        HX_RELEASE(m_pPlaybackVelocity);
                        rv = pRecordSource->QueryInterface(IID_IHXPlaybackVelocity,
                                                           (void**) &m_pPlaybackVelocity);
                        if (SUCCEEDED(rv))
                        {
                            // If InitVelocityControl was called before the
                            // source was created, then we saved m_pPlaybackVelocityResponse.
                            // Therefore, we need to now initialize m_pPlaybackVelocity.
                            if (m_pPlaybackVelocityResponse)
                            {
                                rv = m_pPlaybackVelocity->InitVelocityControl(m_pPlaybackVelocityResponse);
                            }
                            // Now we can release the reponse interface, 
                            // since m_pPlaybackVelocity now owns it
                            HX_RELEASE(m_pPlaybackVelocityResponse);
                        }
                    }
                    HX_RELEASE(pRecordSource);
                }
            }
#endif /* HELIX_FEATURE_RECORDCONTROL */
        }
        HX_RELEASE(pCCF);
    }
    else
    {
        hr = HXR_FAIL;
    }

    return hr;
}

void
HXSource::UpdateDuration(UINT32 ulDuration)
{
    CHXSimpleList* pRepeatList = m_pSourceInfo->GetRepeatList();

    // ulDuration excludes the delay time
    if (pRepeatList && 
        ulDuration >= (m_ulDuration - m_ulDelay))
    {
        m_pSourceInfo->m_ulTotalTrackDuration = ulDuration + m_ulDelay;
        if (m_pSourceInfo->m_pPeerSourceInfo)
        {
            m_pSourceInfo->m_pPeerSourceInfo->m_ulTotalTrackDuration = m_pSourceInfo->m_ulTotalTrackDuration;
        }
    }
    else
    {
        m_ulOriginalDuration = m_ulRestrictedDuration = ulDuration;
        AdjustClipTime();
        m_pSourceInfo->UpdateDuration(m_ulDuration);
    }
}

void
HXSource::UpdateDelay(UINT32 ulDelay)
{
    m_ulDelay = ulDelay;
    AdjustClipTime(IsPaused() /* is resuming from pause */);
    m_pSourceInfo->UpdateDelay(m_ulDelay);
}

void
HXSource::InitialBufferingDone(void)
{
    HXLOGL3(HXLOG_CORE, "HXSource[%p]::InitialBufferingDone()", this);
    m_bInitialBuffering = FALSE;

    // resume if we satisfy the initial preroll AND we have issued
    // rebuffer
    if (REBUFFER_REQUIRED == m_rebufferStatus)
    {
        if (IsRebufferDone())
        {
            ChangeRebufferStatus(REBUFFER_NONE);
        }
    }

    return;
}

void
HXSource::DoRebuffer(UINT32 ulLoopEntryTime,
		     UINT32 ulProcessingTimeAllowance)
{
    HXLOGL3(HXLOG_CORE, "HXSource[%p]::DoRebuffer()", this);

    if (REBUFFER_REQUIRED != m_rebufferStatus)
    {
        // log rebuffer action
        LogInformation("BUFBEG", NULL);     

        m_ulRebufferingStartTix = HX_GET_TICKCOUNT();

        StopAcceleratingEventDelivery(REBUFFER_REQUIRED);

        if (m_bFastStart)
        {
            LeaveFastStart(TP_OFF_BY_REBUFFER);
            HXLOGL1(HXLOG_TRAN, "(%p) Turbo OFF ReportRebufferStatus", this);
        }

        if (m_pPlayer)
        {
            m_pPlayer->InternalPause();     
            ReBuffer(ulLoopEntryTime, ulProcessingTimeAllowance);
        }
    }

    return;
}

HXBOOL
HXSource::IsRebufferDone(void)
{
    HXBOOL            bResult = TRUE;
    STREAM_INFO*    pStreamInfo = NULL;

    if( REBUFFER_REQUIRED != m_rebufferStatus )
    {
        // we are not even in rebuffer mode
        return bResult;
    }

    // check if all streams are done with rebuffering
    for (CHXMapLongToObj::Iterator ndxStrm = mStreamInfoTable->Begin();
         ndxStrm != mStreamInfoTable->End(); ++ndxStrm) 
    {    
        pStreamInfo = (STREAM_INFO*) (*ndxStrm);
        
        if (pStreamInfo->m_unNeeded > pStreamInfo->m_unAvailable)
        {
            bResult = FALSE;
            break;
        }
    }

    // if not, then check whether we have received all the packets
    // for all the streams
    if (!bResult && m_pSourceInfo)
    {
        bResult = m_pSourceInfo->IsRebufferDone();
    }

    if (bResult)
    {
        m_ulRebufferingCumulativeTime += CALCULATE_ELAPSED_TICKS(m_ulRebufferingStartTix, HX_GET_TICKCOUNT());
        m_ulRebufferingStartTix = 0;
    }

    return bResult;
}

HXBOOL
HXSource::FastStartPrerequisitesFullfilled(REF(UINT16) uStatusCode, REF(UINT16) ulPercentDone)
{
    HXBOOL bFastStartPrerequisitesFullfilled = FALSE;

#if defined(HELIX_FEATURE_TURBOPLAY)

    if ((m_bFastStart || m_bSeekInsideRecordControl)                &&
        (!m_pSourceInfo || !m_pSourceInfo->m_bIsPersistentSource))
    {
        if (HXR_OK == IsFaststartPushdownFullfilled(uStatusCode, ulPercentDone))
        {
            bFastStartPrerequisitesFullfilled = TRUE;
        }
    }
#endif /* HELIX_FEATURE_TURBOPLAY */

    return bFastStartPrerequisitesFullfilled;
}

void
HXSource::FastStartUpdateInfo(void)
{
    if (m_bFastStart && !m_turboPlayStats.bBufferDone && HX_STATUS_READY == m_uLastStatusCode)
    {
        m_turboPlayStats.bBufferDone = TRUE;
        m_turboPlayStats.ulBufferedTime = CALCULATE_ELAPSED_TICKS(m_ulStartDataWait, HX_GET_TICKCOUNT());
    }
}

void            
HXSource::ScheduleProcessCallback()
{
    if (m_pSourceInfo)
    {
        m_pSourceInfo->ScheduleProcessCallback();
    }
}

#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
/************************************************************************
 *      Method:
 *          IHXHyperNavigate::GoToURL
 *      Purpose:
 *          Acts as a proxy for actual hypernavigate interface.
 *          Is used to convert any relative URLs to absolute URLs
 */
STDMETHODIMP 
HXSource::GoToURL(const char* pURL, const char* pTarget)
{
    return Execute(pURL, pTarget, NULL, NULL, NULL);
}

/************************************************************************
 *      Method:
 *          IHXHyperNavigate2::Execute
 *      Purpose:
 *          
 *      Parameters:
 *      pURL:       URL (absolute or relative)
 *          pTargetInstance:    
 *          pTargetApplication: 
 *          pTargetRegion:
 *          pParams:
 */
STDMETHODIMP 
HXSource::Execute(const char* pURL,
                   const char* pTargetInstance,
                   const char* pTargetApplication,
                   const char* pTargetRegion,
                   IHXValues* pParams)
{
    HX_RESULT theErr = HXR_OK;
    CHXString newURL = pURL;

    if (ShouldConvert(pTargetInstance) && 
        pURL  &&
        strnicmp(pURL, URL_COMMAND, sizeof(URL_COMMAND) - 1) != 0 )
    {
        CHXURL urlObj(pURL, (IHXClientEngine*)m_pEngine);
        IHXValues* pHeader = urlObj.GetProperties();
        IHXBuffer* pBuffer = NULL;

        if(pHeader && 
           m_pszURL &&
           HXR_OK != pHeader->GetPropertyBuffer(PROPERTY_SCHEME, pBuffer))
        {
            // relative URL
            // if it starts with '/', make it relative to the root of 
            // the URL prefix

            CHXString urlPrefix, urlRoot;
            char* pURLFragment = NULL;
            theErr = CHXURL::GeneratePrefixRootFragment(m_pszURL, 
                                                        urlPrefix, urlRoot, pURLFragment, 
							(IUnknown*)(IHXClientEngine*)m_pEngine);
            HX_VECTOR_DELETE(pURLFragment);

            if (!theErr)
            {
                if(*pURL == '/')
                {
                    newURL = urlRoot + pURL;
                }
                else
                {
                    newURL = urlPrefix + pURL;
                }       
            }
        }
        HX_RELEASE(pBuffer);
        HX_RELEASE(pHeader);
    }
    
    AddRef();

    HX_ASSERT(m_pPlayer && m_pPlayer->m_pHyperNavigate);
    if (m_pPlayer && m_pPlayer->m_pHyperNavigate)
    {
        theErr = m_pPlayer->m_pHyperNavigate->ExecuteWithContext(newURL, 
                        pTargetInstance, pTargetApplication, pTargetRegion, 
                        pParams, (IUnknown*) (IHXStreamSource*) this);
    }

    Release();

    return theErr;
}
#endif /* defined(HELIX_FEATURE_HYPER_NAVIGATE) */

/*
 * IHXClientRateAdaptControl Methods
 */

/************************************************************************
 *      Method:
 *          IHXClientRateAdaptControl::Enable
 *      Purpose:
 *          Enable client rate adaptation for the specified stream
 *
 */
STDMETHODIMP
HXSource::Enable(THIS_ UINT16 uStreamNum)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    STREAM_INFO* pStreamInfo = NULL;
    if (mStreamInfoTable->Lookup((LONG32) uStreamNum, (void*& )pStreamInfo) &&
        pStreamInfo)
    {
        if (pStreamInfo->m_pStream)
        {
            IHXASMStream2* pASMStr = NULL;
            res = pStreamInfo->m_pStream->QueryInterface(IID_IHXASMStream2,
                                                         (void**)&pASMStr);

            if (HXR_OK == res)
            {
                res = pASMStr->UnlockSubscriptions();

                if (HXR_OK == res)
                {
                    pStreamInfo->m_bClientRateAdapt = TRUE;
                }
            }

            HX_RELEASE(pASMStr);
        }
        else
        {
            // We don't have an HXStream object for
            // this stream yet so cache the state
            // in the STREAM_INFO object
            pStreamInfo->m_bClientRateAdapt = TRUE;
            res = HXR_OK;
        }
    }

    return res;
}

/************************************************************************
 *      Method:
 *          IHXClientRateAdaptControl::Disable
 *      Purpose:
 *          Disable client rate adaptation for the specified stream
 *
 */
STDMETHODIMP
HXSource::Disable(THIS_ UINT16 uStreamNum)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    STREAM_INFO* pStreamInfo = NULL;
    if (mStreamInfoTable->Lookup((LONG32) uStreamNum, (void*& )pStreamInfo) &&
        pStreamInfo)
    {
        if (pStreamInfo->m_pStream)
        {
            IHXASMStream2* pASMStr = NULL;
            res = pStreamInfo->m_pStream->QueryInterface(IID_IHXASMStream2,
                                                         (void**)&pASMStr);

            if (HXR_OK == res)
            {
                res = pASMStr->LockSubscriptions();

                if (HXR_OK == res)
                {
                    pStreamInfo->m_bClientRateAdapt = FALSE;
                }
            }

            HX_RELEASE(pASMStr);
        }
        else
        {
            // We don't have an HXStream object for
            // this stream yet so cache the state
            // in the STREAM_INFO object
            pStreamInfo->m_bClientRateAdapt = FALSE;
            res = HXR_OK;
        }
    }

    return res;
}

/************************************************************************
 *      Method:
 *          IHXClientRateAdaptControl::IsEnabled
 *      Purpose:
 *          Is client rate adaptation enabled for the specified stream
 *
 */
STDMETHODIMP
HXSource::IsEnabled(THIS_ UINT16 uStreamNum,
                    REF(HXBOOL) bEnabled)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    STREAM_INFO* pStreamInfo = NULL;
    if (mStreamInfoTable->Lookup((LONG32) uStreamNum, (void*& )pStreamInfo) &&
        pStreamInfo)
    {
        if (pStreamInfo->m_pStream)
        {
            IHXASMStream2* pASMStr = NULL;
            res = pStreamInfo->m_pStream->QueryInterface(IID_IHXASMStream2,
                                                         (void**)&pASMStr);

            if (HXR_OK == res)
            {
                bEnabled = !pASMStr->AreSubscriptionsLocked();
            }

            HX_RELEASE(pASMStr);
        }
        else
        {
            // We don't have an HXStream object for
            // this stream yet so used the cached
            // value in the STREAM_INFO object
            bEnabled = pStreamInfo->m_bClientRateAdapt;
            res = HXR_OK;
        }
    }
    
    return res;
}

/*
 * IHXTransportTimeManager methods
 */
STDMETHODIMP
HXSource::AddSink(THIS_ IHXTransportTimeSink* pSink)
{
    HX_RESULT res = HXR_FAILED;

    // Currently we only support one transport time sink
    if (pSink && !m_pTransportTimeSink)
    {
        m_pTransportTimeSink = pSink;
        m_pTransportTimeSink->AddRef();

        res = HXR_OK;
    }

    return res;
}

STDMETHODIMP
HXSource::RemoveSink(THIS_ IHXTransportTimeSink* pSink)
{
    HX_RESULT res = HXR_FAILED;

    if (pSink && (pSink == m_pTransportTimeSink))
    {
        HX_RELEASE(m_pTransportTimeSink);
        res = HXR_OK;
    }

    return res;
}

STDMETHODIMP
HXSource::SetLiveSyncOffset(UINT32  ulLiveSyncStartTime)
{
    m_ulLiveSyncOffset = ulLiveSyncStartTime;
    return HXR_OK;
}

STDMETHODIMP
HXSource::NewPacketTimeStamp(UINT32  ulDueTimeStamp)
{
    if (EnforceLiveLowLatency())
    {
        UINT32 ulAdjustedPacketTimestamp = CALCULATE_ELAPSED_TICKS(m_ulLiveSyncOffset, ulDueTimeStamp) + m_ulDelay;
        // XXXLCM member var works around mutex deadlock when called from rtsp protocol on app thread
        UINT32 ulAdjustedCurrentPlayTime = /*m_pPlayer->GetCurrentPlayTime()*/ m_ulLastReportedPlayTime - m_ulDelay;
        UINT32 ulLatency = CALCULATE_ELAPSED_TICKS(ulAdjustedCurrentPlayTime, ulAdjustedPacketTimestamp);

        if (ulLatency < SOME_VERY_LARGE_VALUE)
        {
            // if ulLatency is wacky-big we may not have set the live sync start time
            // for the offset. We'll only track latency after the live sync start time
            // has been set. (This usually happens by the fourth or fifth packet at the
            // latest.)

            m_ulLastLatencyCalculation = ulLatency;

#if defined(HELIX_FEATURE_STATS)
            m_ulLivePlaybackIntervalPacketCount++;
            m_ullLivePlaybackIntervalCumulativeLatency += (INT64)ulLatency;
            if (ulLatency < m_ulLivePlaybackIntervalMinimumLatency)
            {
                m_ulLivePlaybackIntervalMinimumLatency = ulLatency;
            }
            if (ulLatency > m_ulLivePlaybackIntervalMaximumLatency)
            {
                m_ulLivePlaybackIntervalMaximumLatency = ulLatency;
            }
#endif /* defined(HELIX_FEATURE_STATS) */
        }
    }
    
    return HXR_OK;
}

STDMETHODIMP
HXSource::GetLatencyStats( REF(UINT32) ulAverageLatency,
                           REF(UINT32) ulMinimumLatency,
                           REF(UINT32) ulMaximumLatency )
{
#if defined(HELIX_FEATURE_STATS)

    if (IsLive() && m_ulLivePlaybackIntervalPacketCount > 0)
    {
        ulAverageLatency = (UINT32)((m_ullLivePlaybackIntervalCumulativeLatency
                                     + (UINT64)(m_ulLivePlaybackIntervalPacketCount / 2))
                                    / (UINT64)m_ulLivePlaybackIntervalPacketCount);
        ulMinimumLatency = m_ulLivePlaybackIntervalMinimumLatency;
        ulMaximumLatency = m_ulLivePlaybackIntervalMaximumLatency;
    }
    else
    {
        ulAverageLatency = 0;
        ulMinimumLatency = 0;
        ulMaximumLatency = 0;
    }
    return HXR_OK;
#else /* defined(HELIX_FEATURE_STATS) */
    return HXR_NOTIMPL;
#endif /* defined(HELIX_FEATURE_STATS) */
}
    
STDMETHODIMP
HXSource::ResetLatencyStats()
{
#if defined(HELIX_FEATURE_STATS)
    m_ulLivePlaybackIntervalPacketCount = 0;
    m_ullLivePlaybackIntervalCumulativeLatency = 0;
    m_ulLivePlaybackIntervalMinimumLatency = 0x7fffffff;
    m_ulLivePlaybackIntervalMaximumLatency = 0;
#endif /* defined(HELIX_FEATURE_STATS) */

    return HXR_OK;
}
    
    
/* 
 * All relative URLs are converted to absolute URLs unless the 
 * original request (ram/smil) passed in OpenRequest/OpenURL()
 * is a mem: URL AND the target is not _player.
 *
 * This fixes relative URLs being hurled to the browser using events
 * come from the same location as the .ram file. (Broadcase usage case)
 * PR 31352
 *
 * This also fixes content on CD-ROMs where relative URLs being hurled 
 * to the browser using events come from the same location as 
 * the .rm file in which they are merged. 
 * PR 23489
 */
HXBOOL HXSource::ShouldConvert(const char* pTargetInstance)
{
    if (pTargetInstance && 
        stricmp(pTargetInstance, "_player") == 0)
    {
        return TRUE;
    }
    
    const char* pPlayerURL = NULL;
    IHXRequest* pPlayerRequest = NULL;
    if (m_pPlayer)
    {
        m_pPlayer->GetRequest(pPlayerRequest);
        if (pPlayerRequest)
        {
            pPlayerRequest->GetURL(pPlayerURL);
        }
    }
    HX_RELEASE(pPlayerRequest);

    if (pPlayerURL && ::strncasecmp(pPlayerURL, "mem:", 4) == 0)
    {
        return FALSE;   
    }

    return TRUE;
}

void
HXSource::MergeUpgradeRequest(HXBOOL bAddDefault /*= FALSE*/, char* pUpgradeString /* = NULL*/)
{
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    if (m_pPlayer && 
        bAddDefault && 
        (!m_pUpgradeCollection || m_pUpgradeCollection->GetCount() == 0))
    {
        if (!m_pUpgradeCollection)
        {
            m_pUpgradeCollection = new HXUpgradeCollection((IUnknown*)(IHXPlayer*)m_pPlayer);
        }

        if (!pUpgradeString)
        {
            pUpgradeString = "Missing Component";
        }

        IHXBuffer* pPluginID = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pPluginID, (UCHAR*)pUpgradeString, 
					    strlen(pUpgradeString) + 1, (IUnknown*)(IHXPlayer*)m_pPlayer))
	{
	    m_pUpgradeCollection->Add(eUT_Required, pPluginID, 0, 0);
	    HX_RELEASE(pPluginID);
	}
    }

    if (m_pPlayer && m_pUpgradeCollection && m_pUpgradeCollection->GetCount() > 0)
    {
        UINT32 ulCount = m_pUpgradeCollection->GetCount();
        IHXUpgradeCollection* pPlayerUpgrade;
        m_pPlayer->QueryInterface(IID_IHXUpgradeCollection, (void**) &pPlayerUpgrade);
        for (UINT32 i = 0; i < ulCount; i++)
        {
            HXUpgradeType upgradeType;
            IHXBuffer* pPluginId = NULL;
            UINT32      majorVersion;
            UINT32      minorVersion;

	    if (HXR_OK == CreateBufferCCF(pPluginId, (IUnknown*)(IHXPlayer*)m_pPlayer))
	    {
		// GetAt is a non-COM like API. It expects pPluginID to be allocated by the user
		// and does not perform an addref either!
		m_pUpgradeCollection->GetAt(i, upgradeType, pPluginId, majorVersion, minorVersion);
		pPlayerUpgrade->Add(upgradeType, pPluginId, majorVersion, minorVersion);
		HX_RELEASE(pPluginId);
	    }
        }

        pPlayerUpgrade->Release();
        m_pUpgradeCollection->RemoveAll();
    }
#endif /* HELIX_FEATURE_AUTOUPGRADE */
}

void
HXSource::ClearUpgradeRequest()
{
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    if (m_pUpgradeCollection)
    {
        m_pUpgradeCollection->RemoveAll();
    }
#endif /* HELIX_FEATURE_AUTOUPGRADE */
}

void
HXSource::EnterPrefetch(PrefetchType prefetchType, UINT32 ulPrefetchValue)
{
    m_bPrefetch = TRUE;
    m_prefetchType = prefetchType;
    m_ulPrefetchValue = ulPrefetchValue;

    return;
}

void
HXSource::LeavePrefetch(void)
{
    m_bPrefetch = FALSE;

    // send prefetch notification so that SMIL
    // renderer can resolve the duration on this prefetch track
    if (m_pSourceInfo)
    {
        m_pPlayer->PrefetchTrackDone(m_pSourceInfo->m_uGroupID, 
                                     m_pSourceInfo->m_uTrackID, 
                                     HXR_OK);
    }

    return;
}

void            
HXSource::SetSoundLevel(UINT16 uSoundLevel, HXBOOL bReflushAudioDevice)
{
#if defined(HELIX_FEATURE_SMIL_SOUNDLEVEL)
    CHXAudioPlayer* pAudioPlayer = NULL;
    CHXAudioStream* pCHXAudioStream = NULL;
    CHXSimpleList*  pAudioStreamList = NULL;

    if (!m_pPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer = m_pPlayer->GetAudioPlayer();
    if (!pAudioPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer->AddRef();

    if (HXR_OK == CollectAudioStreams(pAudioStreamList) && pAudioStreamList)
    {
        pAudioPlayer->SetSoundLevelOffset(pAudioStreamList, m_nSoundLevelOffset);
        pAudioPlayer->SetSoundLevel(pAudioStreamList, uSoundLevel, bReflushAudioDevice);

        ReleaseAudioStreams(pAudioStreamList);
        HX_DELETE(pAudioStreamList);
    }

    HX_RELEASE(pAudioPlayer);

  cleanup:
#endif /* HELIX_FEATURE_SMIL_SOUNDLEVEL */

    return;
}

STDMETHODIMP
HXSource::SetSoundLevelOffset(INT16 nOffset)
{
    m_nSoundLevelOffset = nOffset;

#ifdef HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION
    CHXAudioPlayer* pAudioPlayer = NULL;
    CHXAudioStream* pCHXAudioStream = NULL;
    CHXSimpleList*  pAudioStreamList = NULL;

    if (!m_pPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer = m_pPlayer->GetAudioPlayer();
    if (!pAudioPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer->AddRef();

    if (HXR_OK == CollectAudioStreams(pAudioStreamList) && pAudioStreamList)
    {
        pAudioPlayer->SetSoundLevelOffset(pAudioStreamList, nOffset);

        ReleaseAudioStreams(pAudioStreamList);
        HX_DELETE(pAudioStreamList);
    }

    HX_RELEASE(pAudioPlayer);

  cleanup:
#endif

    return HXR_OK;
}

STDMETHODIMP HXSource::InitVelocityControl(IHXPlaybackVelocityResponse* pResponse)
{
    HX_RESULT retVal = HXR_OK;

    if (m_pPlaybackVelocity)
    {
        retVal = m_pPlaybackVelocity->InitVelocityControl(pResponse);
    }
    else
    {
        // We haven't gotten the IHXPlaybackVelocity interface
        // from either the fileformat or recordsource yet. Therefore,
        // we'll just save the response interface and if
        // we get the IHXPlaybackVelocity interface, then
        // we'll initialize it then
        HX_RELEASE(m_pPlaybackVelocityResponse);
        m_pPlaybackVelocityResponse = pResponse;
        m_pPlaybackVelocityResponse->AddRef();
    }

    return retVal;
}

STDMETHODIMP HXSource::QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pPlaybackVelocity)
    {
        retVal = m_pPlaybackVelocity->QueryVelocityCaps(rpCaps);
    }

    return retVal;
}

STDMETHODIMP HXSource::SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    HX_RESULT retVal = HXR_OK;

    // Set our member variables
    m_lPlaybackVelocity = lVelocity;
    m_bKeyFrameMode     = bKeyFrameMode;
    m_bAutoSwitch       = bAutoSwitch;
    // Inform the buffer manager
    if (m_pBufferManager)
    {
        m_pBufferManager->SetVelocity(lVelocity);
        m_pBufferManager->SetKeyFrameMode(bKeyFrameMode);
    }
    // Inform the event list
    for (CHXMapLongToObj::Iterator i = mStreamInfoTable->Begin(); i != mStreamInfoTable->End(); ++i) 
    {
        STREAM_INFO* pInfo = (STREAM_INFO*) (*i);
        if (pInfo)
        {
            pInfo->m_EventList.SetVelocity(lVelocity);
        }
    }
    if (m_pPlaybackVelocity)
    {
        retVal = m_pPlaybackVelocity->SetVelocity(lVelocity, bKeyFrameMode, bAutoSwitch);
        // Get the keyframe interval
        m_ulKeyFrameInterval = m_pPlaybackVelocity->GetKeyFrameInterval();
        // Set the keyframe interval in the player
        m_pPlayer->SetKeyFrameInterval(m_ulKeyFrameInterval);
    }

    return HXR_OK;
}

STDMETHODIMP HXSource::SetKeyFrameMode(HXBOOL bKeyFrameMode)
{
    HX_RESULT retVal = HXR_OK;

    // Set our own member variable
    m_bKeyFrameMode = bKeyFrameMode;
    // Inform the buffer manager
    if (m_pBufferManager)
    {
        m_pBufferManager->SetKeyFrameMode(bKeyFrameMode);
    }
    if (m_pPlaybackVelocity)
    {
        retVal = m_pPlaybackVelocity->SetKeyFrameMode(bKeyFrameMode);
    }

    return retVal;
}

STDMETHODIMP HXSource::CloseVelocityControl()
{
    HX_RESULT retVal = HXR_OK;

    // If we had to save the velocity response 
    // interface, then release it here
    HX_RELEASE(m_pPlaybackVelocityResponse);
    // Pass the Close along to the fileformat or recordsource
    if (m_pPlaybackVelocity)
    {
        retVal = m_pPlaybackVelocity->CloseVelocityControl();
    }

    return retVal;
}

void            
HXSource::SetAudioDeviceReflushHint(void)
{
#if defined(HELIX_FEATURE_SMIL_SOUNDLEVEL)
    CHXAudioPlayer* pAudioPlayer = NULL;
    CHXSimpleList*  pAudioStreamList = NULL;

    if (!m_pPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer = m_pPlayer->GetAudioPlayer();
    if (!pAudioPlayer)
    {
        goto cleanup;
    }

    pAudioPlayer->AddRef();

    if (HXR_OK == CollectAudioStreams(pAudioStreamList) && pAudioStreamList)
    {
        pAudioPlayer->ManageAudioStreams(pAudioStreamList, AUD_PLYR_STR_SET_REWIND_HINT);

        ReleaseAudioStreams(pAudioStreamList);
        HX_DELETE(pAudioStreamList);
    }

    HX_RELEASE(pAudioPlayer);

  cleanup:
#endif /* HELIX_FEATURE_SMIL_SOUNDLEVEL */

    return;
}

void
HXSource::LeaveFastStart(TurboPlayOffReason leftReason)
{
    m_turboPlayStats.tpOffReason = leftReason;
    m_bFastStart = FALSE;
}

void
HXSource::DeleteAllEvents()
{
    if (m_PacketBufferList.GetCount() > 0)
    {
        LISTPOSITION pos = m_PacketBufferList.GetHeadPosition();
        while (pos != NULL)
        {
            CHXEvent* pTempEvent  = (CHXEvent*) m_PacketBufferList.GetNext(pos);
            delete pTempEvent;
        }

        m_PacketBufferList.RemoveAll();
    }
}

void    
HXSource::SetMinimumPreroll()
{
    UINT32 ulTotalMinimumPreroll = 0;

    if (m_pPlayer)
    {
        // get the user-set minimum preroll
        m_pPlayer->GetMinimumPreroll(ulTotalMinimumPreroll);
    }

    // Recompute m_ulPreRollInMs since the renderers could have changed
    // the value in the header
    m_ulPreRollInMs = 0;
    ULONG32 ulPostDecodeDelay = 0;

    for (CHXMapLongToObj::Iterator i = mStreamInfoTable->Begin();
         i != mStreamInfoTable->End(); ++i) 
    {    
        STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*i);

        if (pStreamInfo && pStreamInfo->m_pHeader)
        {
            // Get the stream preroll
            ULONG32 ulPreroll = 
                ClientPrerollHelper::GetPreroll(pStreamInfo->m_pHeader);

            if (ulPreroll > m_ulPreRollInMs)
            {
                m_ulPreRollInMs = ulPreroll;

                // Get the post decode delay
                ulPostDecodeDelay = 0;
                pStreamInfo->m_pHeader->GetPropertyULONG32("PostDecodeDelay", 
                                                           ulPostDecodeDelay);
            }
			
			// Check if the preroll time is minor than the custom end time. 
            if ( m_bCustomEndTime && (pStreamInfo->BufferingState().GetPreroll() > 
                    pStreamInfo->m_ulDuration - pStreamInfo->m_ulTimeAfterSeek ))
            {
            	pStreamInfo->BufferingState().UpdatePreroll(pStreamInfo->m_ulDuration - 
                    pStreamInfo->m_ulTimeAfterSeek);
            }
        }
    }
    
    m_ulMaxLatencyThreshold = ComputeMaxLatencyThreshold(m_ulPreRollInMs,
                                                         ulPostDecodeDelay);

    if (m_ulPreRollInMs < ulTotalMinimumPreroll)
    {
        // Subract out the old preroll value
        m_ulMaxLatencyThreshold -= m_ulPreRollInMs;

        m_ulPreRollInMs = ulTotalMinimumPreroll;

        // Add in the new preroll value
        m_ulMaxLatencyThreshold += m_ulPreRollInMs;
    }

    HXLOGL2(HXLOG_CORE, "HXSource[%p]::SetMinimumPreroll(): Preroll %lu MinPreroll %lu MaxLatencyThreshold %lu", 
            this, m_ulPreRollInMs, ulTotalMinimumPreroll, m_ulMaxLatencyThreshold);

    m_pBufferManager->SetMinimumPreroll(m_bPerfectPlay,
                                        ulTotalMinimumPreroll);
}

HX_RESULT               
HXSource::SendHeaderToRecordControl(HXBOOL bFileHeader, IHXValues* pHeader)
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    HX_RESULT nResult = HXR_OK;

    if(m_pRecordControl && pHeader)
    {
        if(bFileHeader)
            nResult = m_pRecordControl->OnFileHeader(pHeader);
        else
            nResult = m_pRecordControl->OnStreamHeader(pHeader);

        if(nResult != HXR_OK)
        {
            m_bPlayFromRecordControl = FALSE;
	    if (nResult == HXR_RECORD_NORENDER)
	    {
		m_ulRenderingDisabledMask = HXRNDR_DISABLED_ALL;
	    }
            else if (nResult != HXR_RECORD)
	    {
                HX_RELEASE(m_pRecordControl);
	    }
        }
    }
    return nResult;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_RECORDCONTROL */
}

HX_RESULT
HXSource::ProcessFileHeader(void)
{
    HX_RESULT   hr=HXR_OK;
    UINT32      bNonSeekAble = 0;
    IHXBuffer*  pTitle = NULL;
    IHXBuffer*  pAuthor = NULL;
    IHXBuffer*  pCopyright = NULL;
    IHXBuffer*  pAbstract = NULL;
    IHXBuffer*  pDescription = NULL;
    IHXBuffer*  pLicenseInfo = NULL;
    IHXBuffer*  pKeywords = NULL;
    IHXValues*  pValues = NULL;
    IHXValues*  pMetaInfo = NULL;

    if (m_pURL)
    {
        pValues = m_pURL->GetOptions();
    }

    // retrieve the TAC from the URL
    if (pValues)
    {
        pValues->GetPropertyBuffer("Title", pTitle);
        pValues->GetPropertyBuffer("Author", pAuthor);
        pValues->GetPropertyBuffer("Copyright", pCopyright);
        pValues->GetPropertyBuffer("Abstract", pAbstract);
        pValues->GetPropertyBuffer("Description", pDescription);
        pValues->GetPropertyBuffer("Keywords", pKeywords);

//#define LOSS_HACK
#ifdef LOSS_HACK
        UINT32 ulLoss = 0;
        if (HXR_OK == pValues->GetPropertyULONG32("Loss", ulLoss))
        {
            m_ulLossHack = ulLoss;
            /* Initialize random number generator */
            ::srand((unsigned int) HX_GET_TICKCOUNT());
        }
#endif /*LOSS_HACK*/
    }

    if (m_pFileHeader)
    {
        if (!pTitle)        m_pFileHeader->GetPropertyBuffer("Title", pTitle);
        if (!pAuthor)       m_pFileHeader->GetPropertyBuffer("Author", pAuthor);
        if (!pCopyright)    m_pFileHeader->GetPropertyBuffer("Copyright", pCopyright);
        if (!pDescription)  m_pFileHeader->GetPropertyCString("Description", pDescription);
        if (!pAbstract)     m_pFileHeader->GetPropertyCString("Abstract", pAbstract);
        if (!pKeywords)     m_pFileHeader->GetPropertyCString("Keywords", pKeywords);

        m_pFileHeader->GetPropertyULONG32("NonSeekable", bNonSeekAble);
        m_bNonSeekable = bNonSeekAble ? TRUE : FALSE;
        
        m_pFileHeader->GetPropertyULONG32("StreamCount",m_ulStreamHeadersExpected);
            
        HX_ASSERT(mStreamInfoTable->IsEmpty() == TRUE);
        if (mStreamInfoTable->IsEmpty() && m_ulStreamHeadersExpected > 0 &&
            m_ulStreamHeadersExpected < mStreamInfoTable->GetHashTableSize())
        {
            mStreamInfoTable->InitHashTable(m_ulStreamHeadersExpected);
        }
    }

#ifdef HELIX_FEATURE_DRM
    // create a default empty LicenseInfo buffer
    // certain TLC client expect LicenseInfo been always set
    if (SUCCEEDED(CreateBufferCCF(pLicenseInfo, (IUnknown*)(IHXPlayer*)m_pPlayer)))
    {
        pLicenseInfo->Set((const UCHAR*)"", 1);
    }

    //let DRM update the LicenseInfo buffer
    if (IsHelixDRMProtected() && m_pDRM)
    {
        m_pDRM->GetLicenseInfo(pLicenseInfo);
    }
#endif

    if (m_pFileHeader)
    {
#if defined(HELIX_FEATURE_EMBEDDED_UI) && defined(HELIX_FEATURE_DRM)
	// Send the file header and the DRM object
	IHXEmbeddedUI* pEmbeddedUI=NULL;
	if (m_pPlayer && SUCCEEDED(m_pPlayer->QueryInterface(IID_IHXEmbeddedUI, (void**) &pEmbeddedUI)))
	{
	    IUnknown* pUnknown=NULL;
	    if (m_pDRM)
	    {
		m_pDRM->QueryInterface(IID_IUnknown, (void**) &pUnknown);
	    }

	    EmbeddedUIInfo* uiInfo = new EmbeddedUIInfo(m_pszURL, pUnknown, m_pFileHeader);
	    pEmbeddedUI->SetEmbeddedUI(uiInfo);
            HX_DELETE(uiInfo);
	    HX_RELEASE(pUnknown);
	}
	HX_RELEASE(pEmbeddedUI);
#else
	IHXBuffer* pBuffer = NULL;
	HXBOOL bEmbeddedUI= SUCCEEDED(m_pFileHeader->GetPropertyBuffer("DRM_UIDescription", pBuffer));
	HX_RELEASE(pBuffer);

	if (bEmbeddedUI)
	{
	    hr = HXR_FAIL;
	    ReportError(hr);
	}
#endif // HELIX_FEATURE_EMBEDDED_UI
    }

    
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_pStats)
    {
        if (pTitle && m_pStats->m_pTitle)
        {
            m_pStats->m_pTitle->SetStr((char*)(pTitle->GetBuffer()));
        }

        if (pAuthor && m_pStats->m_pAuthor)
        {
            m_pStats->m_pAuthor->SetStr((char*)(pAuthor->GetBuffer()));
        }

        if (pCopyright && m_pStats->m_pCopyright)
        {
            m_pStats->m_pCopyright->SetStr((char*)(pCopyright->GetBuffer()));
        }

        if (pAbstract && m_pStats->m_pAbstract)
        {
            m_pStats->m_pAbstract->SetStr((char*)(pAbstract->GetBuffer()));
        }
    
        if (pDescription && m_pStats->m_pDescription)
        {
            m_pStats->m_pDescription->SetStr((char*)(pDescription->GetBuffer()));
        }

        if (pKeywords && m_pStats->m_pKeywords)
        {
            m_pStats->m_pKeywords->SetStr((char*)(pKeywords->GetBuffer()));
        }
    }

    CopyMetaDataToRegistry(m_pFileHeader);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    if (!m_pSourceInfo->m_bIsPersistentSource &&
        m_pPlayer && m_pPlayer->GetMetaInfo())
    {
        if (pTitle || pAuthor || pCopyright ||
            pDescription || pAbstract || pKeywords || pLicenseInfo)
        {
            IHXCommonClassFactory* pCCF = NULL;

            if (m_pEngine &&
                SUCCEEDED(m_pEngine->QueryInterface(IID_IHXCommonClassFactory, (void**) &pCCF)))
            {
                pCCF->CreateInstance(CLSID_IHXValues, (void**) &pMetaInfo);
            }

            if (pMetaInfo)
            {
                if (pTitle)        pMetaInfo->SetPropertyBuffer("Title", pTitle);
                if (pAuthor)       pMetaInfo->SetPropertyBuffer("Author", pAuthor);
                if (pCopyright)    pMetaInfo->SetPropertyBuffer("Copyright", pCopyright);
                if (pDescription)  pMetaInfo->SetPropertyBuffer("Description", pDescription);
                if (pAbstract)     pMetaInfo->SetPropertyBuffer("Abstract", pAbstract);
                if (pKeywords)     pMetaInfo->SetPropertyBuffer("Keywords", pKeywords);
#ifdef HELIX_FEATURE_DRM
                if (pLicenseInfo)  pMetaInfo->SetPropertyBuffer("LicenseInfo", pLicenseInfo);
#endif

                m_pPlayer->GetMetaInfo()->UpdateMetaInfo(pMetaInfo);
                HX_RELEASE(pMetaInfo);
            }

            HX_RELEASE(pCCF);
        }
    }

    m_bReceivedHeader = TRUE;

    HX_RELEASE(pTitle);
    HX_RELEASE(pAuthor);
    HX_RELEASE(pCopyright);
    HX_RELEASE(pAbstract);
    HX_RELEASE(pDescription);
    HX_RELEASE(pKeywords);
    HX_RELEASE(pValues);

    return hr;
}

static HXBOOL GetHeaderBOOL(IHXValues* pHeader, const char* pKey)
{
    HXBOOL bRet = FALSE;

    ULONG32 ulTemp = 0;
    if ((HXR_OK == pHeader->GetPropertyULONG32(pKey, ulTemp)) && ulTemp)
    {
        bRet = TRUE;
    }

    return bRet;
}

HX_RESULT
HXSource::ProcessStreamHeaders(IHXValues* pHeader, STREAM_INFO*& pStreamInfo)
{
    HX_RESULT       rc = HXR_OK;
    UINT32          ulStreamNumber  = 0;
    UINT32          ulAvgBitRate    = 0;
    UINT32          ulMaxBitRate    = 0;
    UINT32          ulAvgPacketSize = 0;
    UINT32          ulPreroll  = 0;
    ULONG32         ulPredata  = 0;
    UINT32          ulDuration = 0;
    UINT32          ulHasSubordinateLifetime = FALSE;
    void*           lTmp       = NULL;
    IHXBuffer*      pMimeType  = NULL;

    pStreamInfo = NULL;

    if (!pHeader)
    {
        rc = HX_INVALID_HEADER;
        goto cleanup;
    }

    // Use file header for default duration
    if (m_pFileHeader)
    {
        m_pFileHeader->GetPropertyULONG32("Duration", ulDuration);
        if (!m_ulDuration && ulDuration)
        {
            m_ulDuration = ulDuration;
            ulDuration = 0;
        }
    }

    pHeader->GetPropertyULONG32("StreamNumber",     ulStreamNumber);
    pHeader->GetPropertyULONG32("AvgBitRate",       ulAvgBitRate);
    pHeader->GetPropertyULONG32("MaxBitRate",       ulMaxBitRate);
    pHeader->GetPropertyULONG32("AvgPacketSize",    ulAvgPacketSize);
    pHeader->GetPropertyULONG32("Predata",          ulPredata);
    pHeader->GetPropertyULONG32("Duration",         ulDuration);

    if (pHeader->GetPropertyULONG32("HasSubordinateLifetime", ulHasSubordinateLifetime) == HXR_OK &&
	    ulHasSubordinateLifetime == TRUE)
    {
	HXLOGL3(HXLOG_CORE, 
		"HXSource[%p]::ProcessStreamHeaders(): Setting HasSubordinateLifetime flag", this);
	m_bHasSubordinateLifetime = TRUE;
	// Set the player to have an indefinite duration,
	// overridden by other source durations greater than zero
        if (m_pPlayer && m_pPlayer->m_ulPresentationDuration == 0)
        {
	    m_pPlayer->m_bHasSubordinateLifetime = TRUE;
	}
    }

    if (mStreamInfoTable->Lookup((LONG32) ulStreamNumber, (void *&) lTmp))
    {
        // a header with this stream number already exists..
        rc = HX_INVALID_HEADER;
        goto cleanup;
    }
    
    // Get the preroll
    ulPreroll = ClientPrerollHelper::GetPreroll(pHeader);

    // max preroll
    if (m_ulPreRollInMs < ulPreroll)
    {
        m_ulPreRollInMs = ulPreroll;
    }

    m_ulAvgBandwidth   += ulAvgBitRate;

    // max duration...
    if (m_ulDuration < ulDuration)
    {
        m_ulDuration = ulDuration;
    }

    pStreamInfo = new STREAM_INFO;
    if (!pStreamInfo)
    {
        rc = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    pStreamInfo->m_pHeader                  = pHeader;
    pStreamInfo->m_pHeader->AddRef();

    pStreamInfo->m_bCustomEndTime           = FALSE;
    pStreamInfo->m_bSrcStreamDone           = FALSE;
    pStreamInfo->m_bSrcStreamFillingDone    = FALSE;
    pStreamInfo->m_bPacketRequested         = FALSE;
    
    pStreamInfo->m_uStreamNumber = (UINT16) ulStreamNumber;

    pStreamInfo->m_ulDuration = ulDuration;

    // Get the mime type
    pHeader->GetPropertyCString("MimeType", pMimeType);

    pStreamInfo->BufferingState().OnStreamHeader(
        ulStreamNumber,
        ulPreroll,
        ulPredata,
        GetHeaderBOOL(pHeader, "PreDataAtStart"),
        GetHeaderBOOL(pHeader, "PreDataAfterSeek"),
        GetHeaderBOOL(pHeader, "PrerollAtStart"),
        GetHeaderBOOL(pHeader, "PrerollAfterSeek"),
        ulAvgBitRate,
        ulMaxBitRate,
        pMimeType);

    HX_RELEASE(pMimeType);

    mStreamInfoTable->SetAt(ulStreamNumber, (void *) pStreamInfo);

cleanup:

    return rc;
}

HX_RESULT HXSource::OnTimeSync(ULONG32 ulCurrentTime)
{
    HX_RESULT res = HXR_OK;

    // for NewPacketTimeStamp() calculation
    m_ulLastReportedPlayTime = ulCurrentTime;

    // Convert presentation time to transport time
    if (m_pTransportTimeSink)
    {
        ULONG32 ulOffset = m_ulDelay;
        ULONG32 ulTransportTime = m_ulStartTime;
        
        if (IsTimeGreaterOrEqual(ulCurrentTime, ulOffset))
        {
            ulTransportTime += ulCurrentTime - ulOffset;
        }
    
        res = m_pTransportTimeSink->OnTransportTime(ulTransportTime);
    }

    return res;
}

void 
HXSource::SetSrcBufStats(IHXSourceBufferingStats3* pSrcBufStats)
{
    HX_RELEASE(m_pSrcBufStats);

    if (pSrcBufStats)
    {
        m_pSrcBufStats = pSrcBufStats;
        m_pSrcBufStats->AddRef();
    }
}


HXBOOL
HXSource::IsPlaying()
{
    HXBOOL bIsPlaying = FALSE;
    if (m_pPlayer)
    {
        bIsPlaying = m_pPlayer->IsPlaying();
    }

    return bIsPlaying;
}

HX_RESULT HXSource::StartAcceleratingEventDelivery()
{
    HX_RESULT res = HXR_OK;

    HX_ASSERT( m_pPlayer );
    if (!m_pPlayer)
    {
        res = HXR_UNEXPECTED;
    }
    else if (!IsAcceleratingEventDelivery())
    {
        HXLOGL3(HXLOG_CORE, 
                "HXSource[%p]::StartAcceleratingEventDelivery(): rebuffer audio = %lu", 
                this, m_bRebufferAudio);

        ChangeRebufferStatus(REBUFFER_WARNING);

        // Set the base time for the acceleration
        m_ulAcceleratedRebufferBaseTime = m_pPlayer->GetInternalCurrentPlayTime();
        
        // Set the acceleration factor. If this is set to 2 then packets are
        // sent twice as fast to the renderer. 3 == 3x faster etc.
        m_ulAcceleratedRebufferFactor = 2;
        res = m_pPlayer->AccelerateEventsForSource(this);
    }

    return res;
}

void 
HXSource::StopAcceleratingEventDelivery(RebufferStatus newStatus)
{
    if( IsAcceleratingEventDelivery() )
    {
        HXLOGL3(HXLOG_CORE, 
                "HXSource[%p]::StopAcceleratingEventDelivery(): rebuffer audio = %lu", 
                this, m_bRebufferAudio);

        m_ulAcceleratedRebufferFactor = 0;
    }
    
    //Change the status regardless.
    ChangeRebufferStatus(newStatus);
}

UINT32 
HXSource::CalcAccelDeliveryTime(UINT32 ulDeliveryTime) const
{
    UINT32 uRet = ulDeliveryTime;

    if (m_ulAcceleratedRebufferFactor &&
        (((INT32) (ulDeliveryTime - m_ulAcceleratedRebufferBaseTime)) > 0))
    {
        UINT32 ulDelta = ulDeliveryTime - m_ulAcceleratedRebufferBaseTime;
        
        uRet = (m_ulAcceleratedRebufferBaseTime + 
                ulDelta * m_ulAcceleratedRebufferFactor);
    }

    return uRet;
}

HXBOOL HXSource::ShouldRebufferOnOOP(STREAM_INFO* pStreamInfo) const
{
    // Determines if we should rebuffer when a source encounters the Out Of
    // Packets condition.  We only want to rebuffer if the number of packets
    // needed don't match the number available AND we aren't triggering the
    // rebuffer off of the audio
    return (pStreamInfo &&
            pStreamInfo->m_unNeeded > 0 &&
            pStreamInfo->m_unNeeded != pStreamInfo->m_unAvailable &&
            !m_bRebufferAudio) ? TRUE : FALSE;
}


HXBOOL HXSource::HandleAudioRebuffer()
{
    return DoRebufferIfNeeded();
}

void HXSource::CheckForInitialPrerollRebuffer(UINT32 ulCurrentTime)
{
    // rebuffer if the source hasn't satisfy its initial preroll when it's
    // time to start 
    // resume in InitialBufferingDone()
    if (m_pPlayer->IsPlaying()  &&
        !m_bPartOfNextGroup &&
        !m_bPartOfPrefetchGroup &&
        m_bInitialBuffering     && 
        (REBUFFER_REQUIRED != m_rebufferStatus) &&
        (ulCurrentTime + MIN_BUFFERING_COMPLETION_LEAD_AHEAD_OF_DELAYED_START) >= m_ulDelay)
    {
        DoRebuffer();
    }
}

#define REBUFFER_STATUS_NAME(status)                                    \
    (REBUFFER_NONE == status) ? "REBUFFER_NONE" :                       \
    (REBUFFER_WARNING == status) ? "REBUFFER_WARNING" :                 \
    (REBUFFER_REQUIRED == status) ? "REBUFFER_REQUIRED" : "Unknown"

void HXSource::ChangeRebufferStatus(RebufferStatus newStatus)
{
    HXLOGL3(HXLOG_CORE, "HXSource[%p]::ChangeRebufferStatus() : %s -> %s", 
            this, 
            REBUFFER_STATUS_NAME(m_rebufferStatus), 
            REBUFFER_STATUS_NAME(newStatus));

    m_rebufferStatus = newStatus;
}

UINT32
HXSource::CalcEventTime(STREAM_INFO* pStreamInfo, UINT32 ulTime, 
                        HXBOOL bEnableLiveCheck, INT32 lVelocity)
{
    UINT32 ulEventTime = 0;

    if (ulTime > m_ulStartTime)
    {
        ulEventTime = ulTime - m_ulStartTime;
    }
    else
    {
        ulEventTime = 0;
    }

    ulEventTime += m_ulDelay;

    if (bEnableLiveCheck && (mLiveStream || m_bRestrictedLiveStream))
    {
        ulEventTime -= m_ulFirstPacketTime;
    }
   
    UINT32 stream_ulBuffering = pStreamInfo->BufferingState().GetMinBufferingInMs();

    // Event time reflects dispatch time into the renderer which is 
    // buffering interval ahead.
    if (lVelocity >= 0)
    {
        ulEventTime = (ulEventTime > stream_ulBuffering ? (ulEventTime - stream_ulBuffering) : 0);
    }
    else
    {
        // Reverse playback
        ulEventTime += stream_ulBuffering;
    }

    return ulEventTime;
}

INT64  
HXSource::CalcActualPacketTime(STREAM_INFO* pStreamInfo, UINT32 ulTime) const
{
    INT64 llActualPacketTime = 
        pStreamInfo->BufferingState().CreateINT64Timestamp(ulTime);

    /* subtract start time from player time */
    if (m_ulStartTime > 0)
    {
        if (m_ulStartTime < llActualPacketTime) 
        {
            llActualPacketTime -= m_ulStartTime; 
        }
        else
        {
            llActualPacketTime = 0;
        }
    }
    
    return llActualPacketTime;
}

void   
HXSource::OnResumed(void)
{
    if (m_pBufferManager)
    {
        m_pBufferManager->OnResumed();
    }

    return;
}

ULONG32 HXSource::ComputeMaxLatencyThreshold(ULONG32 ulPrerollInMs, 
                                             ULONG32 ulPostDecodeDelay)
{
    // We are computing the maximim latency value we will allow 
    // before we issues a seek to correct latency drift.
    //
    // +---------------+-----+------------------+
    // |    ulPrerollInMs    | ulLatencyJitter  |  
    // +---------------+-----+------------------+
    // | Media Preroll | PDD |  Latency Jitter  |
    // +---------------+-----+------------------+
    //
    // ulPrerollInMs contains the minimum amount of latency
    // required to properly play back the clip. This is the
    // ideally where we want the player latency to be
    //
    // ulPostDecodeDelay contains how much of ulPrerollInMs
    // is allocated to the post decode delay.
    //
    // For right now we are setting the latency jitter
    // to the post decode delay. The post decode delay 
    // represents how much time the core needs to 
    // properly playback the media. Adding twice this 
    // value to the media preroll basically means that 
    // the core should have twice the what it needs for 
    // proper playback. The DefaultMinimumLatencyThreshold 
    // limit is put in here to handle current live streams
    // that stop sending packets for 1 second. This limit
    // prevents the core from doing excessive seeking when
    // playing back these streams. The 
    // DefaultMinimumLatencyJitter limit helps avoid
    // excessive seeking when the post decode delay
    // value is low. It helps compensate for jitter
    // in the packet arrival.
    ULONG32 ulLatencyJitter = ulPostDecodeDelay;
    
    if (ulLatencyJitter < DefaultMinimumLatencyJitter)
    {
        ulLatencyJitter = DefaultMinimumLatencyJitter;
    }

    ULONG32 ulRet = ulPrerollInMs + ulLatencyJitter;

    if (ulRet < DefaultMinimumLatencyThreshold)
    {
        ulRet = DefaultMinimumLatencyThreshold;
    }

    return ulRet;
}

#if defined(HELIX_FEATURE_DRM)
HX_RESULT HXSource::InitHelixDRM(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;
    HX_RELEASE(m_pDRM);
    if (!m_bIsProtected)
    {
        return HXR_OK;
    }

    m_pDRM = new HXDRM((HXSource*)this);
    if (m_pDRM == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    m_pDRM->AddRef();

    retVal = m_pDRM->InitDRMPlugin(pHeader);

    return retVal;
}


HXBOOL HXSource::IsHelixDRMProtected(IHXValues* pHeader)
{
    return HXDRM::IsProtected(pHeader);
}

HX_RESULT HXSource::OnFileHeader(HX_RESULT status, IHXValues* pValues)
{
    //file header modified by DRM
    if (SUCCEEDED(status))
    {
        HX_RELEASE(m_pFileHeader);
        m_pFileHeader = pValues;
        m_pFileHeader->AddRef();
        status = ProcessFileHeader();
    }

    if (!SUCCEEDED(status))
    {
        mLastError = status;
        ReportError(mLastError);
    }

    return HXR_OK;
}

HX_RESULT HXSource::OnStreamHeader(HX_RESULT status, IHXValues* pValues)
{
    //stream header modified by DRM
    if (SUCCEEDED(status))
    {
        STREAM_INFO* pStreamInfo;
        status = ProcessStreamHeaders(pValues, pStreamInfo);
    }

    if (!SUCCEEDED(status))
    {
        mLastError = status;
        ReportError(mLastError);
    }

    return HXR_OK;
}

HX_RESULT HXSource::OnStreamDone(HX_RESULT status, UINT16 unStreamNumber)
{
    if (!SUCCEEDED(status))
    {
        //DRM error
	mLastError = status;
	ReportError(mLastError);
    }

    return HXR_OK;
}

HX_RESULT HXSource::OnTermination(HX_RESULT status)
{
    if (!SUCCEEDED(status))
    {
        //DRM error
	mLastError = status;
	ReportError(mLastError);
    }

    return HXR_OK;
}

#endif /* defined(HELIX_FEATURE_DRM) */
