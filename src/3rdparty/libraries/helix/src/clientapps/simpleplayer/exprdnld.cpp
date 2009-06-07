/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hxcom.h"
#include "hxtypes.h"
#include "hxcore.h"
#include "hxprdnld.h"
#include "exprdnld.h"
#include "print.h"

#include "globals.h"
struct _stGlobals*& GetGlobal();

UINT32 GetTime(); // /In main.cpp.

// /#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
/************************************************************************
 *  Method:
 *    Constructor
 */
ExamplePDStatusObserver::ExamplePDStatusObserver(IUnknown* pUnkPlayer)
    : m_lRefCount(0)
    , m_pPrgDnldStatusMgr(NULL)
    , m_pUnkPlayer(pUnkPlayer)
    , m_pHXPlayer(NULL)
    , m_bPlayerIsPausedByThis(FALSE)
    , m_bFirstPDStatusMessage(TRUE)
    , m_ulTotalDurReported(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulDurSoFar(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulCurStatusUpdateGranularity(
            HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC)
    , m_bInitialPrerollUpateGranularitySet(FALSE)
    , m_bDownloadIsComplete(FALSE)
{
    if (m_pUnkPlayer)
    {
	m_pUnkPlayer->QueryInterface(IID_IHXPlayer,
			(void**)&m_pHXPlayer);

	m_pUnkPlayer->QueryInterface(IID_IHXPDStatusMgr,
			(void**)&m_pPrgDnldStatusMgr);

        if (m_pPrgDnldStatusMgr)
        {
            // /Add ourselves as an observer of progressive download playback:
            m_pPrgDnldStatusMgr->AddObserver(this);
            // /Set granularity of status reports to 10 per second until we
            // know enough about the situation to go back to getting reports
            // at the default interval (every 5 seconds).  Do that as soon
            // as we receive the first progress report:
            m_ulCurStatusUpdateGranularity = 100;
        }

	m_pUnkPlayer->AddRef();
    }
};

/************************************************************************
 *  Method:
 *    Destructor
 */
ExamplePDStatusObserver::~ExamplePDStatusObserver()
{
    if (m_pPrgDnldStatusMgr)
    {
        m_pPrgDnldStatusMgr->RemoveObserver(this);
    }
    HX_RELEASE(m_pPrgDnldStatusMgr);
    HX_RELEASE(m_pHXPlayer);
    HX_RELEASE(m_pUnkPlayer);
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP 
ExamplePDStatusObserver::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXPDStatusObserver*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPDStatusObserver))
    {
	AddRef();
	*ppvObj = (IHXPDStatusObserver*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) 
ExamplePDStatusObserver::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) 
ExamplePDStatusObserver::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
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
ExamplePDStatusObserver::OnDownloadProgress(
              IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
              UINT32 /*IN*/ ulNewDurSoFar,
              UINT32 /*IN*/ ulNewBytesSoFar,
              INT32  /*IN*/ lTimeSurplus)
{
    m_ulDurSoFar = ulNewDurSoFar;

    if (m_ulCurStatusUpdateGranularity <
            HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC)
    {
        if (m_bInitialPrerollUpateGranularitySet)
        {
            if (HX_PROGDOWNLD_UNKNOWN_DURATION != ulNewDurSoFar  &&
                    // /Preroll is done, so reset update interval back to default:
                    GetGlobal()->g_bOnBeginOccurred)
            {
                m_ulCurStatusUpdateGranularity =
                        HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC;
                m_pPrgDnldStatusMgr->SetStatusUpdateGranularityMsec(
                        m_ulCurStatusUpdateGranularity);
            }
        }
        else if (!GetGlobal()->g_bOnBeginOccurred)
        {
            if (HXR_OK == m_pPrgDnldStatusMgr->SetStatusUpdateGranularityMsec(
                    m_ulCurStatusUpdateGranularity))
            {
                m_bInitialPrerollUpateGranularitySet = TRUE;
            }
        }
    }

    UINT32 ulCurPlayTime = 0;

    HXBOOL bPauseWasAttempted = FALSE;
    HXBOOL bPauseOccurred     = FALSE;
    HXBOOL bResumeWasAttempted = FALSE;
    HXBOOL bResumeOccurred = FALSE;

    
    if (m_pHXPlayer)
    {
        ulCurPlayTime = m_pHXPlayer->GetCurrentPlayTime();

        if (GetGlobal()->g_bEnableSlowStart  &&
                lTimeSurplus != HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS)
        {
            // /"slow start" is enabled so if we have run dry of data,
            // pause and wait for more data:
            if (!m_bPlayerIsPausedByThis)
            {
                if (!m_bDownloadIsComplete  &&  lTimeSurplus<0)
                {
                    // /Use a 1000-millisecond allowance for variation:
                    if (lTimeSurplus < -1000)
                    {
                        bPauseWasAttempted = TRUE;
                        bPauseOccurred = m_bPlayerIsPausedByThis =
                                (HXR_OK == m_pHXPlayer->Pause());
                    }
                }
            }
            else // /paused; see if we can resume yet:
            {
                if (lTimeSurplus > 0)
                {
                    // /Use a 1000-millisecond allowance for variation:
                    if (lTimeSurplus > 1000)
                    {
                        bResumeWasAttempted = TRUE;
                        bResumeOccurred = (HXR_OK == m_pHXPlayer->Begin());
                        m_bPlayerIsPausedByThis = !bResumeOccurred;
                    }
                }
            }
        }
    }

    if (GetGlobal()->bEnableVerboseMode  &&  !m_bDownloadIsComplete)
    {
        STDOUT("\nDownload progress: (play time=%lu,",
                ulCurPlayTime);
        if (HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS == lTimeSurplus)
        {
            STDOUT(" UNKNOWN surplus|deficit)");
        }
        else if (HX_PROGDOWNLD_MIN_TIME_SURPLUS == lTimeSurplus)
        {
            STDOUT(" deficit exceeds maximum");
        }
        else if (HX_PROGDOWNLD_MAX_TIME_SURPLUS == lTimeSurplus)
        {
            STDOUT(" surplus exceeds maximum)");
        }
        else
        {
            STDOUT(" surplus=%ld milliseconds)", lTimeSurplus);
        }
        if (HX_PROGDOWNLD_UNKNOWN_DURATION == ulNewDurSoFar)
        {
            STDOUT("\n\thave UNKNOWN");
        }
        else
        {
            STDOUT("\n\thave %lu", ulNewDurSoFar);
        }
        
        if (HX_PROGDOWNLD_UNKNOWN_DURATION != m_ulTotalDurReported)
        {
            STDOUT(" of %lu msec", m_ulTotalDurReported);
        }
        else
        {
            STDOUT(" of UNKNOWN msec of media");
        }
        if (HX_PROGDOWNLD_UNKNOWN_FILE_SIZE != ulNewBytesSoFar)
        {
            STDOUT(" (%lu", ulNewBytesSoFar);
        }
        else
        {
            STDOUT(" (UNKNOWN");
        }
        STDOUT(" bytes downloaded so far)\n", ulNewBytesSoFar);

        if (bPauseOccurred  ||  bPauseWasAttempted)
        {
            STDOUT("# Waiting for more data: %splayback.\n "
                    "    Should take %ld milliseconds before playback resumes.\n",
                    bPauseOccurred? "Pausing " :
                        (bPauseWasAttempted? "Failed attempting to pause "
                                              : " "), -lTimeSurplus);
        }
        if (bResumeOccurred  ||  bResumeWasAttempted)
        {
            STDOUT("# Data available: %splayback\n "
                    "    Time surplus is now %ld",
                    bResumeOccurred? "Resuming " :
                        (bResumeWasAttempted? "Failed attempting to resume "
                                              : " "), lTimeSurplus);
        }
    }

    m_bFirstPDStatusMessage = FALSE;

    return HXR_OK;
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
ExamplePDStatusObserver::OnTotalDurChanged(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
            UINT32 ulNewTotalDur)
{
    m_ulTotalDurReported = ulNewTotalDur;

    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("\nOnTotalDurChanged(): to %lu milliseconds\n", ulNewTotalDur);
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
ExamplePDStatusObserver::OnDownloadComplete(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    m_bDownloadIsComplete = TRUE;

    HXBOOL bResumeWasAttempted = FALSE;
    HXBOOL bResumeOccurred = FALSE;

    // /In case we're paused, resume now that there is no more data to get:
    if (m_pHXPlayer  &&  m_bPlayerIsPausedByThis)
    {
        bResumeWasAttempted = TRUE;
        bResumeOccurred = (HXR_OK == m_pHXPlayer->Begin());
        m_bPlayerIsPausedByThis = !bResumeOccurred;
    }
    
    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("\nOnDownloadComplete()\n");
        if (bResumeOccurred  ||  bResumeWasAttempted)
        {
            STDOUT("\n%splayback now that bytes are available for "
                    "uninterrupted playback\n",
                    bResumeOccurred? "Resuming " :
                        (bResumeWasAttempted? "Failed attempting to resume "
                                              : " "));
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
ExamplePDStatusObserver::SrcClaimsSeekSupport(IHXStreamSource* pStreamSource,
                                              HXBOOL bClaimsSupport)
{
    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("\nSrcClaimsSeekSupport(%sE)\n", bClaimsSupport?"TRU":"FALS");
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
ExamplePDStatusObserver::OnDownloadPause(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("\nOnDownloadPause()\n");
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
ExamplePDStatusObserver::OnDownloadResume(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("\nOnDownloadResume()\n");
    }
    return HXR_OK;
}
// /#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
