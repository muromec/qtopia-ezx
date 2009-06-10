/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudply.cpp,v 1.58 2008/09/20 06:02:27 gajia Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"

#ifndef WIN32_PLATFORM_PSPC
#include "hlxclib/signal.h"
#endif

#include "hxresult.h"
#include "hxtypes.h"
#include "hxslist.h"
#include "hxcom.h"

#include "ihxpckts.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxausvc.h"
#include "hxrasyn.h"
#include "hxerror.h"
#include "hxcore.h"

#include "hxmap.h"

#include "hxaudply.h"
#include "hxaudstr.h"
#include "hxaudses.h"
#include "hxaudvol.h"

#include "hxaudtyp.h"

#include "timeval.h"
#include "hxtick.h"
#include "pckunpck.h"

#ifdef _MACINTOSH
#include "hxmm.h"

extern  ULONG32 gTIMELINE_MUTEX;
#endif
#include "hxtlogutil.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(HELIX_FEATURE_PREFERENCES)
#include "hxprefs.h"
#include "hxprefutil.h"
#endif /* HELIX_FEATURE_PREFERENCES */




#define MAX_WAIT_AT_SAME_TIME (MAXIMUM_AUDIO_GRANULARITY+50)

CHXAudioPlayer::CHXAudioPlayer( CHXAudioSession* owner )
:	m_lRefCount(0)
,	m_pPlayerResponse(0)
,	m_pScheduler (0)
,	m_pPreferences(0)
,	m_ulCurrentTime(0)
,	m_ulLastCurrentTimeReturned(0)
,	m_ulLastDeviceTimeAdjusted(0)
,	m_bTimeReturned(FALSE)
,	m_ulBytesPerGran(0)
,	m_pStreamRespList(0)
,	m_ulASstartTime(0)
,	m_ulAPplaybackTime(0)
,       m_ulLastAdjustedTimeDiff(0)
,	m_ulAPstartTime(0)
,	m_ulADresumeTime(0)
, 	m_eState(E_STOPPED)
,	m_bPrefUse11khz(FALSE)
,	m_uPrefBitsPerSample(16)
,	m_uPrefAudioQuality(4)
,	m_bDisableWrite(FALSE)
,	m_bIsStarted(FALSE)
,	m_uVolume(0)
,	m_bMute(FALSE)
,       m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
,       m_bKeyFrameMode(FALSE)
,       m_ulScaledTimeAnchor(0)
,       m_ulUnscaledTimeAnchor(0)
,       m_ulCurrentScaledTime(0)
,	m_ulCallbackID(0)
,	m_pContext(0)
,	m_bInited(FALSE)
,	m_bHasStreams(FALSE)
,	m_bIsLive(FALSE)
,	m_ulGranularity(0)
,	m_Owner(owner)
,	m_pStreamList(0)
,	m_pPMixHookList(0)
,	m_pFakeAudioCBTime(0)
,	m_ulLastFakeCallbackTime(0)
,	m_ulIncreasingTimer(0)
,	m_bIsDonePlayback(TRUE)
,	m_bIsFirstResume(TRUE)
,	m_bHasDataInAudioDevice(FALSE)
,       m_pInactiveClockSourceQueue(NULL)
,       m_pActiveClockSource(NULL)
,       m_pLastMappingStream(NULL)
,       m_bNewMapStarting(TRUE)    
{
#ifdef HELIX_FEATURE_VOLUME
    m_pPlayerVolume = NULL;
#endif

    m_Owner->AddRef();
    m_pFakeAudioCBTime = new Timeval;
    // NOTE: we should add some check on the success of this allocation.

    /* Default value of Player format */
    m_PlayerFmt.uChannels       = 2;
    m_PlayerFmt.uBitsPerSample  = 16;
    m_PlayerFmt.ulSamplesPerSec = 16000;
    m_PlayerFmt.uMaxBlockSize   = 64000;
}

/************************************************************************
 *  Method:
 *              IHXAudioPlayer::~CHXAudioPlayer()
 *      Purpose:
 *              Destructor. Clean up and set free.
 */
CHXAudioPlayer::~CHXAudioPlayer()
{
    Close();
}

void CHXAudioPlayer::Close(void)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::Close()", this);

    ResetPlayer();

    HX_DELETE(m_pStreamList);
    InvalidateIters();
    
    HX_RELEASE(m_pContext);
#if defined(HELIX_FEATURE_PREFERENCES)
    HX_RELEASE(m_pPreferences);
#endif /* HELIX_FEATURE_PREFERENCES */
    HX_RELEASE(m_pPlayerResponse);

    if ( m_pPMixHookList )
    {
        HXAudioHookInfo* pMixHookInfo  = 0;
        while(!m_pPMixHookList->IsEmpty())
        {
            pMixHookInfo = (HXAudioHookInfo*) m_pPMixHookList->RemoveHead();
            pMixHookInfo->pHook->Release();
            delete pMixHookInfo;
        }

        delete m_pPMixHookList;
        m_pPMixHookList = 0;
    }

    // Delete all stream response items.
    if ( m_pStreamRespList )
    {
        IHXAudioStreamInfoResponse*   pAudioStreamInfoResponse = 0;
        while(!m_pStreamRespList->IsEmpty())
        {
            pAudioStreamInfoResponse =
             (IHXAudioStreamInfoResponse*) m_pStreamRespList->RemoveHead();

            pAudioStreamInfoResponse->Release();
        }

        delete m_pStreamRespList;
        m_pStreamRespList = 0;
    }

#ifdef HELIX_FEATURE_VOLUME
    if( m_pPlayerVolume )
    {
        m_pPlayerVolume->RemoveAdviseSink((IHXVolumeAdviseSink*)this);
        m_pPlayerVolume->Release();
        m_pPlayerVolume = NULL;
    }
#endif

    // Delete IRMA volume object.
    HX_DELETE(m_pFakeAudioCBTime);
    HX_RELEASE(m_Owner);
    HX_RELEASE(m_pScheduler);

    // If we are using a clock source, then close it
    if (m_pActiveClockSource)
    {
        m_pActiveClockSource->CloseClockSource();
    }
    // Release the active clock source
    HX_RELEASE(m_pActiveClockSource);
    // Clear any queued (but inactive) clock sources
    ClearClockSourceQueue();
    // Delete the inactive clock source queue
    HX_DELETE(m_pInactiveClockSourceQueue);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP CHXAudioPlayer::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
	    { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*)this},
            { GET_IIDHANDLE(IID_IHXAudioPlayer), (IHXAudioPlayer*)this },
	    { GET_IIDHANDLE(IID_IHXAudioPlayer2), (IHXAudioPlayer2*)this },
#if defined(HELIX_FEATURE_CROSSFADE)
            { GET_IIDHANDLE(IID_IHXAudioCrossFade), (IHXAudioCrossFade*)this },
#endif /* HELIX_FEATURE_CROSSFADE */
#if defined(HELIX_FEATURE_VOLUME)
            { GET_IIDHANDLE(IID_IHXVolumeAdviseSink), (IHXVolumeAdviseSink*)this },
#endif /* HELIX_FEATURE_VOLUME */
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXAudioPlayer*)this }
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
            , { GET_IIDHANDLE(IID_IHXPlaybackVelocity), (IHXPlaybackVelocity*)this }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
            , { GET_IIDHANDLE(IID_IHXClockSourceManager), (IHXClockSourceManager*) this }
        };

    HX_RESULT res = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);

    // if succeeded, return immediately...
    if (SUCCEEDED(res))
    {
        return res;
    }
    // ...otherwise, proceed.

    if (m_Owner &&
             m_Owner->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioPlayer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioPlayer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXAudioPlayer methods
 */
/************************************************************************
 *  Method:
 *              IHXAudioPlay::AddPostMixHook
 *      Purpose:
 *       Call this to add a post mix hook of the audio data.
 */
STDMETHODIMP CHXAudioPlayer::AddPostMixHook
(
    IHXAudioHook* pHook,
    const HXBOOL  bDisableWrite,
    const HXBOOL  bFinal
)
{
    /* We only allow adding Hooks before the playback has started */
    if (m_bInited)
    {
        return HXR_FAILED;
    }

    return ActualAddPostMixHook(pHook, bDisableWrite, bFinal);
}

/************************************************************************
 *  Method:
 *              IHXAudioPlay::RemovePostMixHook
 *      Purpose:
 *       Call this to remove a post mix hook.
 */
STDMETHODIMP CHXAudioPlayer::RemovePostMixHook
(
    IHXAudioHook*       pHook
)
{
    /* We only allow removing Hooks after the playback has stopped */
    if (m_bInited)
    {
        return HXR_FAILED;
    }

    return ActualRemovePostMixHook(pHook);
}

/************************************************************************
*  Method:
*      IHXAudioPlayer::GetAudioStreamCount
*  Purpose:
*               Get the number of audio streams currently active in the
*               audio player. Since streams can be added mid-presentation
*               this function may return different values on different calls.
*               If the user needs to know about all the streams as they get
*               get added to the player, IHXAudioStreamInfoResponse should
*               be implemented and passed in SetStreamInfoResponse.
*/
STDMETHODIMP_(UINT16) CHXAudioPlayer::GetAudioStreamCount()
{
    HX_ASSERT(m_pStreamList);
    if (m_pStreamList)
    {
        return (UINT16) m_pStreamList->GetCount();
    }
    else
    {
        return 0;
    }
}

/************************************************************************
 *  Method:
 *              IHXAudioPlayer::GetAudioStream
 *      Purpose:
 */
STDMETHODIMP_(IHXAudioStream*) CHXAudioPlayer::GetAudioStream
(
    UINT16      uIndex
)
{
    LISTPOSITION lp = 0;
    lp = m_pStreamList->FindIndex( (int) uIndex );
    if ( lp )
    {
        CHXAudioStream* s = 0;
        s = (CHXAudioStream*) m_pStreamList->GetAt(lp);
        s->AddRef();
        return s;
    }
    else
        return 0;
}

/************************************************************************
 *  Method:
 *              IHXAudioPlayer::GetAudioVolume
 *      Purpose:
 *              Return this player's IRMA volume interface.
 */
STDMETHODIMP_(IHXVolume*) CHXAudioPlayer::GetAudioVolume()
{
    IHXVolume* pRet = NULL;
#ifdef HELIX_FEATURE_VOLUME
    if( m_pPlayerVolume )
    {
        m_pPlayerVolume->AddRef();
        pRet = m_pPlayerVolume;
    }
#endif
    return pRet;
}

/************************************************************************
 *  Method:
 *              IHXAudioPlayer::GetDeviceVolume
 *      Purpose:
 *              Return this audio device volume interface.
 */
STDMETHODIMP_(IHXVolume*) CHXAudioPlayer::GetDeviceVolume()
{
    return ( m_Owner->GetDeviceVolume() );
}

/*
 *  IHXAudioCrossFade methods
 */

/************************************************************************
 *  Method:
 *      IHXAudioCrossFade::CrossFade
 *  Purpose:
 *      Cross-fade two audio streams.
 *      pStreamFrom             - Stream to be cross faded from
 *      pStreamTo               - Stream to be cross faded to
 *      ulFromCrossFadeStartTime- "From" Stream time when cross fade is 
 *                                to be started
 *      ulToCrossFadeStartTime  - "To" Stream time when cross fade is to 
 *                                be started
 *      ulCrossFadeDuration     - Duration over which cross-fade needs
 *                                    to be done
 *      All the timing parameters are in milliseconds.
 *      
 *      This function can also be used to achieve fade-out or fade-in only audio
 *      effect by specifying either pStreamFrom or pStreamTo parameter.
 *      
 */
STDMETHODIMP
CHXAudioPlayer::CrossFade(IHXAudioStream*  pStreamFrom,
                          IHXAudioStream*  pStreamTo,
                          UINT32            ulFromCrossFadeStartTime,
                          UINT32            ulToCrossFadeStartTime,
                          UINT32            ulCrossFadeDuration)
{
#if defined(HELIX_FEATURE_CROSSFADE)
    HX_RESULT theErr = HXR_OK;
    CHXAudioStream* pFromStream = NULL;
    CHXAudioStream* pToStream = NULL;

    LISTPOSITION streamPos1 = m_pStreamList->Find(pStreamFrom);
    LISTPOSITION streamPos2 = m_pStreamList->Find(pStreamTo);
    if (!streamPos2 && !streamPos1)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (streamPos1)
    {
        pFromStream = (CHXAudioStream*) m_pStreamList->GetAt(streamPos1);
        theErr = pFromStream->StartCrossFade(pToStream, ulFromCrossFadeStartTime,
                                             ulCrossFadeDuration, FALSE);
    }

    if (HXR_OK != theErr)
    {
        return theErr;
    }

    if (streamPos2)
    {
        pToStream = (CHXAudioStream*) m_pStreamList->GetAt(streamPos2);
        theErr = pToStream->StartCrossFade(pFromStream, ulToCrossFadeStartTime,
                                           ulCrossFadeDuration, TRUE);
    }

    if (HXR_OK != theErr)
    {
        return theErr;
    }

    if (streamPos1 && streamPos2)
    {
        /* Adjust the streams in list so that the "ToStream" List appears before
         * the "FromStream" 
         * A lame but quick-and-dirty way to do it: Remove these two stream from 
         * the listand them in order at the tail!
         */
        LISTPOSITION lPos = m_pStreamList->GetHeadPosition();
        while (lPos)
        {
            CHXAudioStream* pStream = 
                (CHXAudioStream* ) m_pStreamList->GetAt(lPos);
            if (pStream == pFromStream ||
                pStream == pToStream)
            {
                /* RemoveAt returns the next position in the list.
                 * DO NOT use GetNext if you remove a node.
                 */
                lPos = m_pStreamList->RemoveAt(lPos);
                InvalidateIters();
            }
            else
            {
                m_pStreamList->GetNext(lPos);
            }
        }

        m_pStreamList->AddTail((void*) pToStream);
        m_pStreamList->AddTail((void*) pFromStream);
        InvalidateIters();
    }

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_CROSSFADE */
}

STDMETHODIMP CHXAudioPlayer::InitVelocityControl(IHXPlaybackVelocityResponse* pResponse)
{
    HX_RESULT retVal = HXR_OK;

    return retVal;
}

STDMETHODIMP CHXAudioPlayer::QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXAudioPlayer::SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    HX_RESULT retVal = HXR_OK;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Is this velocity different than the current velocity?
    if (lVelocity     != m_lPlaybackVelocity ||
        bKeyFrameMode != m_bKeyFrameMode)
    {
        // XXXMEH TODO: we can be more precise here if we
        // want by interpolating between time syncs, but most
        // likely the audio player is already paused.
        //
        // Save the unscaled time anchor
        m_ulUnscaledTimeAnchor = m_ulCurrentTime;
        // Save the scaled time anchor
        m_ulScaledTimeAnchor = m_ulCurrentScaledTime;
        // Save the playback velocity
        m_lPlaybackVelocity = lVelocity;
        // Save the keyframe mode
        m_bKeyFrameMode = bKeyFrameMode;
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

STDMETHODIMP_(INT32) CHXAudioPlayer::GetVelocity()
{
    return m_lPlaybackVelocity;
}

STDMETHODIMP CHXAudioPlayer::SetKeyFrameMode(HXBOOL bKeyFrameMode)
{
    m_bKeyFrameMode = bKeyFrameMode;
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) CHXAudioPlayer::GetKeyFrameMode()
{
    return m_bKeyFrameMode;
}

STDMETHODIMP CHXAudioPlayer::CloseVelocityControl()
{
    HX_RESULT retVal = HXR_OK;

    // Reset playback velocity
    m_lPlaybackVelocity = HX_PLAYBACK_VELOCITY_NORMAL;

    return retVal;
}

STDMETHODIMP CHXAudioPlayer::RegisterClockSource(IHXClockSource* pSource)
{
    return ClockSourceSwitch(pSource, TRUE);
}

STDMETHODIMP CHXAudioPlayer::UnregisterClockSource(IHXClockSource* pSource)
{
    return ClockSourceSwitch(pSource, FALSE);
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::Init
 *      Purpose:
 *              Initialize the Audio Player object called by rmaplayer.
 */
HX_RESULT CHXAudioPlayer::Init
(
    IUnknown*   pContext
)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::Init()", this);
    if (!pContext)
        return HXR_INVALID_PARAMETER;

    m_pContext = pContext;
    m_pContext->AddRef();

    if (HXR_OK != pContext->QueryInterface(IID_IHXScheduler,
                                (void **) &m_pScheduler))
    {
        return HXR_INVALID_PARAMETER;
    }

    if (HXR_OK != pContext->QueryInterface(IID_IHXAudioPlayerResponse,
                                (void **) &m_pPlayerResponse))
    {
        return HXR_INVALID_PARAMETER;
    }
#if defined( HELIX_FEATURE_PREFERENCES )
    m_pContext->QueryInterface(IID_IHXPreferences, (void**) &m_pPreferences);
#endif
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *              CHXAudioPlayer::_Init
 *      Purpose:
 *              Create internal lists, etc. Called by Audio Session
 *              CreateAudioPlayer() method.
 */
HX_RESULT CHXAudioPlayer::InitializeStructures()
{
    HX_RESULT theErr = HXR_OK;

    // Create the Stream list.
    // Create the Post process hook list.
    // Create the Stream response list.
    m_pStreamList       = new CHXSimpleList;
#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    m_pPMixHookList     = new CHXSimpleList;
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */
    m_pStreamRespList   = new CHXSimpleList;

    if ( !m_pStreamList || !m_pStreamList->IsPtrListValid())
        theErr = HXR_OUTOFMEMORY;

#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    if ( !m_pPMixHookList || !m_pPMixHookList->IsPtrListValid())
        theErr = HXR_OUTOFMEMORY;
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */

    if ( !m_pStreamRespList || !m_pStreamRespList->IsPtrListValid())
        theErr = HXR_OUTOFMEMORY;

#if defined(HELIX_FEATURE_VOLUME)
    if( !theErr )
    {
        m_pPlayerVolume = (IHXVolume*)new CHXVolume;
        if( m_pPlayerVolume )
        {
            m_pPlayerVolume->AddRef();
            m_pPlayerVolume->AddAdviseSink(this);
            //Start off with the volume at max.
            m_pPlayerVolume->SetVolume(HX_MAX_VOLUME);
        }
        else
        {
            theErr = HXR_OUTOFMEMORY;
        }
    }
#endif /* HELIX_FEATURE_VOLUME */

    return ( theErr );
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::GetFormat
 *      Purpose:
 *              Return the player's device format which was determined in
 *              InitPlayer() and is based on all the streams.
 */
void CHXAudioPlayer::GetFormat
(
        HXAudioFormat*   pAudioFormat
)
{
    memcpy(pAudioFormat, &m_PlayerFmt, sizeof( HXAudioFormat) );
}


/* ***********************************************************************
 *  Method:
 *      CHXAudioPlayer::GetAudioPrefs
 *  Purpose:
 *      Get audio related preferences.
 */
HX_RESULT CHXAudioPlayer::GetAudioPrefs()
{
    IHXBuffer* pBuffer = NULL;
    IHXPreferences* pPreferences = 0;

#if defined(HELIX_FEATURE_PREFERENCES)
    /* Reason we query for Preferences here and not at Init() is because
     * Preferences may have been overwritten in HXPlayer by SetupClient()
     * call by upper level client and this happens AFTER CHXAudioPlayer::Init()
     * is called
     */
    if (!m_pContext)
    {
        return HXR_INVALID_PARAMETER;
    }

    m_pContext->QueryInterface(IID_IHXPreferences, (void**) &pPreferences);

    //  What is the pref for this?
    if( pPreferences )
    {

        if (pPreferences->ReadPref("SamplingRate", pBuffer) == HXR_OK)
        {
            m_bPrefUse11khz = (11025 == ::atol((const char*) pBuffer->GetBuffer()));
            pBuffer->Release();
            pBuffer = 0;
        }

        ReadPrefUINT16(pPreferences, "BitsPerSample", m_uPrefBitsPerSample);
        ReadPrefUINT16(pPreferences, "Quality", m_uPrefAudioQuality);

        /* hmmm... Looks like the client override default Preferences implementation*/
        if (m_pPreferences != pPreferences)
        {
            if (m_pPreferences)
            {
                m_pPreferences->Release();
            }
            m_pPreferences = pPreferences;
            m_pPreferences->AddRef();
        }

        pPreferences->Release();
    }
#endif /* HELIX_FEATURE_PREFERENCES */

    return HXR_OK;
}


/************************************************************************
 *  Method:
 *              IHXAudioPlay::CreateAudioStream
 *      Purpose:
 *       The renderer calls this to create a unique audio stream with its
 *       unique audio format.
 */
STDMETHODIMP CHXAudioPlayer::CreateAudioStream
(
    IHXAudioStream** pAudioStream
)
{
   HX_RESULT theErr = HXR_OK;

    // Create a new IRMA audio stream
    *pAudioStream = 0;
    *pAudioStream = (IHXAudioStream*) new CHXAudioStream(this, m_pContext);
    if ( !*pAudioStream )
    {
        theErr = HXR_OUTOFMEMORY;
    }

    // Add audio stream to my list
    if (!theErr)
    {
        theErr = _CreateAudioStream(pAudioStream);
    }

    return theErr;
}

HX_RESULT
CHXAudioPlayer::_CreateAudioStream(IHXAudioStream** pAudioStream)
{
    (*pAudioStream)->AddRef();   // once for user
    (*pAudioStream)->AddRef();   // once for me

    // Add to the stream list.
    m_pStreamList->AddTail((void*) *pAudioStream);
    InvalidateIters();

    ((CHXAudioStream*)(*pAudioStream))->SetLive(m_bIsLive);

    /* Already initialized with no streams?*/
    if (m_bInited && !m_bHasStreams)
    {
        /* If we are already initialized and there were no audio streams before
         * initialization, we must be using our fake timer to send time syncs
         * This needs to change now to get time syncs from the audio device
         */
        ((CHXAudioStream*)(*pAudioStream))->SetupToBeDone();

        return HXR_OK;
    }

    m_bHasStreams = TRUE;

    // If we were using a clock source, then deactivate it
    DeactivateClockSource();

    m_Owner->CheckIfLastNMilliSecsToBeStored();

    /* If we are already initialized, it means CreateAudioStream was
     * called in the midst of the presentation. In this case, we already know
     * the granularity and the Device format and thus call Setup right away
     */
    if ((*pAudioStream) && m_bInited)
    {
        ((CHXAudioStream*)(*pAudioStream))->Setup( &m_DeviceFmt, m_ulGranularity );
    }
    return HXR_OK;
}

HX_RESULT
CHXAudioPlayer::SetSoundLevel(CHXSimpleList* pAudioStreamList, UINT16 uSoundLevel, HXBOOL bReflushAudioDevice)
{
    HX_RESULT   rc = HXR_OK;
    IHXVolume* pStreamVolume = NULL;

    if (pAudioStreamList && !pAudioStreamList->IsEmpty())
    {
        CHXSimpleList::Iterator lIter = pAudioStreamList->Begin();
        for (; lIter != pAudioStreamList->End(); ++lIter)
        {
            CHXAudioStream* pAudioStream = (CHXAudioStream*) (*lIter);

            pStreamVolume = pAudioStream->GetAudioVolume();
            if (pStreamVolume)
            {
                pStreamVolume->SetVolume(uSoundLevel);
            }
            HX_RELEASE(pStreamVolume);
        }

        if (bReflushAudioDevice)
        {
            AudioStreamStateChanged(E_PLAYING);
        }
    }

    return rc;
}

HX_RESULT       
CHXAudioPlayer::SetSoundLevelOffset(CHXSimpleList* pAudioStreamList, INT16 nSoundLevelOffset)
{
#ifdef HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION
    if (pAudioStreamList && !pAudioStreamList->IsEmpty())
    {
        CHXSimpleList::Iterator lIter = pAudioStreamList->Begin();
        for (; lIter != pAudioStreamList->End(); ++lIter)
        {
            CHXAudioStream* pAudioStream = (CHXAudioStream*) (*lIter);
            pAudioStream->SetSoundLevelOffset(nSoundLevelOffset);
        }
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif
}

/* This is an olde deprecated public method - use the one below instead. */
/* It remains here for backward compatibility. */
HX_RESULT CHXAudioPlayer::ManageAudioStreams(CHXSimpleList* pStreamList,
                                             UINT32 what,
                                             UINT32 ulTime)
{
    UINT16 uStreamCount = 0;
    IHXAudioStream** pAudioStreamArray = NULL;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pStreamList && !pStreamList->IsEmpty())
    {
	CHXAudioStream* pAudioStream;
	IHXAudioStream* pAudioStreamI;
	UINT16 uIdx = 0;

	uStreamCount = (UINT16) pStreamList->GetCount();

	retVal = HXR_OUTOFMEMORY;
	pAudioStreamArray = new IHXAudioStream* [uStreamCount];
	if (pAudioStreamArray)
	{
	    memset(pAudioStreamArray, 0, sizeof(IHXAudioStream*));
	    retVal = HXR_OK;
	}

	if (SUCCEEDED(retVal))
	{
	    CHXSimpleList::Iterator it = pStreamList->Begin();
	    while ((uIdx < uStreamCount) && (it != pStreamList->End()))
	    {
		pAudioStream = (CHXAudioStream*) (*it);
		retVal = pAudioStream->QueryInterface(IID_IHXAudioStream, (void**) &pAudioStreamI);

		if (FAILED(retVal))
		{
		    break;
		}

		pAudioStreamArray[uIdx] = pAudioStream;
		++it;
		++uIdx;
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = ManageAudioStreams(uStreamCount,
					    pAudioStreamArray,
					    what,
					    NULL);
	    }

	    for (uIdx = 0; uIdx < uStreamCount; uIdx++)
	    {
		HX_RELEASE(pAudioStreamArray[uIdx]);
	    }

	    delete [] pAudioStreamArray;
	}
    }

    return retVal;
}

/************************************************************************
 *  Method:
 *      IHXAudioPlayer2::ManageAudioStreams
 *  Purpose:
 *	   Applies the requested action to the specified list of audio
 *	streams.  All audio streams provided must belong to the audio
 *	player they ManageAudioStreams command is given.
 *	The passed in params are used to provide parameters to the action
 *	given.  If no parameters are needed, NULL should be passed.
 *	There are currently no defined parameters for any of the currently
 *	defined actions.
 */
STDMETHODIMP CHXAudioPlayer::ManageAudioStreams(UINT16 uStreamCount,
						IHXAudioStream** pAudioStreamArray,
						UINT32 ulStreamAction,
						IHXValues* pParams)
{
    HX_RESULT rc = HXR_INVALID_PARAMETER;
    UINT16 uIdx = 0;
    HXBOOL bAnyStreamsNeedSessionRewind = FALSE;
    IHXAudioStream3* pAudioStream = NULL;

    if (pAudioStreamArray && (uStreamCount != 0))
    {
	HXBOOL bSessionRewindNeededForStream = FALSE;

	rc = HXR_OK;

        while (SUCCEEDED(rc) && (uIdx < uStreamCount))
        {
	    rc = pAudioStreamArray[uIdx]->QueryInterface(IID_IHXAudioStream3, (void**) &pAudioStream);
	    if (SUCCEEDED(rc))
	    {
		rc = HXR_FAIL;
		if (pAudioStream)
		{
		    rc = HXR_OK;
		}
	    }

	    bSessionRewindNeededForStream = FALSE;

	    if (SUCCEEDED(rc))
	    {
		switch (ulStreamAction)
		{
		case AUD_PLYR_STR_STOP:
		    bSessionRewindNeededForStream = !pAudioStream->IsRewound();
		    pAudioStream->Stop();
		    break;
		case AUD_PLYR_STR_FLUSH:
		    pAudioStream->Flush();
		    bSessionRewindNeededForStream = !pAudioStream->IsRewound();
		    break;
		case AUD_PLYR_STR_RESUME:
		    pAudioStream->Resume();
		    bSessionRewindNeededForStream = (pAudioStream->IsRewound() && pAudioStream->IsResumed());
		    break;
		case AUD_PLYR_STR_PAUSE:
		    bSessionRewindNeededForStream = !pAudioStream->IsRewound();
		    pAudioStream->Pause();
		    break;
		case AUD_PLYR_STR_SET_REWIND_HINT:
		    {
			IHXValues* pConfigParams = NULL;
			rc = CreateValuesCCF(pConfigParams, m_pContext);
			if (SUCCEEDED(rc))
			{
			    rc = pConfigParams->SetPropertyULONG32("audioDeviceReflushHint", 1);
			}
			if (SUCCEEDED(rc))
			{
			    rc = pAudioStream->Reconfig(pConfigParams);
			   
			}
			HX_RELEASE(pConfigParams);
		    }
		    break;
		case AUD_PLYR_STR_REMOVE:
		    {
			LISTPOSITION pos = m_pStreamList->Find(pAudioStream);
			if (pos)
			{
			    m_pStreamList->RemoveAt(pos);
			    InvalidateIters();
			}

			bSessionRewindNeededForStream = !pAudioStream->IsRewound();
			pAudioStream->Stop();
			HX_RELEASE(pAudioStream);
		    }
		    break;
		default:
		    HX_ASSERT("bad stream action taken"==NULL);
		    rc = HXR_INVALID_PARAMETER;
		}
		uIdx++;

		bAnyStreamsNeedSessionRewind = (bAnyStreamsNeedSessionRewind || bSessionRewindNeededForStream);
	    }

	    HX_RELEASE(pAudioStream);
        }

	//Post stream iteration actions.
	if (SUCCEEDED(rc))
	{
	    switch (ulStreamAction)
	    {
	    case AUD_PLYR_STR_STOP:
		AudioStreamStateChanged(E_STOPPED, bAnyStreamsNeedSessionRewind);
		break;
	    case AUD_PLYR_STR_RESUME:
		AudioStreamStateChanged(E_PLAYING, bAnyStreamsNeedSessionRewind);
		break;
	    case AUD_PLYR_STR_PAUSE:
		AudioStreamStateChanged(E_PAUSED, bAnyStreamsNeedSessionRewind);
		break;
	    case AUD_PLYR_STR_SET_REWIND_HINT:
		m_Owner->CheckIfLastNMilliSecsToBeStored();
		break;
	    case AUD_PLYR_STR_REMOVE:
		if (0 == m_pStreamList->GetCount())
		{
		    m_bHasStreams = FALSE;
		    m_bHasDataInAudioDevice = bAnyStreamsNeedSessionRewind;
		    m_Owner->Stop(this, TRUE);
		    m_bInited = FALSE;
		    if(HXR_OK != (rc=Setup(m_ulGranularity)))
		    {
			IHXErrorMessages* pErrorMessage = NULL;
			m_pContext->QueryInterface(IID_IHXErrorMessages, (void**) &pErrorMessage);
			if (pErrorMessage)
			{
			    pErrorMessage->Report(HXLOG_ERR, rc, 0, NULL, NULL);
			    pErrorMessage->Release();
			}
			rc = HXR_OK;
		    }
		    else
		    {
			// We just removed the last audio stream.
			// If we had any inactive clock sources
			// queued, then we need to install one.
			rc = InstallAnyInactiveClockSource();
			if (SUCCEEDED(rc))
			{
			    rc = ResumeFakeTimeline();
			}
		    }
		}
		else
		{
		    AudioStreamStateChanged(E_STOPPED, bAnyStreamsNeedSessionRewind);
		}

		break;
	    default:
		// nothing to do
		break;
	    }
	}
    }

    return rc;
}

HX_RESULT
CHXAudioPlayer::AudioStreamStateChanged(EPlayerState eState, HXBOOL bRewindNeeded)
{
    // we only concern about the state change of audio stream
    // while its parent audio player is in playing mode
    
	switch (eState)
	{
	case E_PLAYING:
	    // If we are not playing, we need to defer any rewind action
	    // until the player resumption.
	    if (m_eState == E_PLAYING)
	    {
		HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::AudioStreamStateChanged(): E_PLAYING-Rewind%sneeded", 
			this, 
			bRewindNeeded ? " " : " not ");
		if (bRewindNeeded)
		{
		    m_Owner->RewindSession();
		    m_Owner->ActualResume();
		}
	    }
	    break;
	case E_PAUSED:
	case E_STOPPED:
	    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::AudioStreamStateChanged(): E_PAUSED/E_STOPPED", this);
	    // In case of stoppage or pausing, we need to rewind the session
	    // if needed regardless of the player state since we do not know
	    // which players will be resuming when or if any streams will
	    // need rewind on resumption and we need to get the paused/stopped
	    // audio stream data out of the audio pushdown (i.e. mixed) portion.
	    if (bRewindNeeded)
	    {
		m_Owner->RewindSession();
		if (((m_eState == E_PLAYING) && (NumberOfResumedStreams() > 0)) ||
		    (!m_Owner->GetDisableMultiPlayPauseSupport() &&
		     (m_Owner->NumberOfResumedPlayers() > 0)))
		{
		    // resume the session if this or other players are in
		    // playing state.
		    m_Owner->ActualResume();
		}
	    }
	    break;
	default:
	    break;
	}

    return HXR_OK;
}

CHXAudioStream*
CHXAudioPlayer::GetCHXAudioStream(UINT16 uIndex)
{
    LISTPOSITION lp = 0;
    lp = m_pStreamList->FindIndex( (int) uIndex );
    if ( lp )
    {
        return (CHXAudioStream*)m_pStreamList->GetAt(lp);
    }
    else
    {
        return NULL;
    }
}

HXBOOL
CHXAudioPlayer::IsLastNMilliSecsToBeStored()
{
    HXBOOL bResult = FALSE;

    if (m_bHasStreams)
    {
        CHXAudioStream* s = 0;
        CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
        AddIter(&lIter);
        while(lIter != m_pStreamList->End())
        {
            s = (CHXAudioStream*) (*lIter);
            if (s->IsAudioDeviceReflushHint())
            {
                bResult = TRUE;
                break;
            }
            IncrementOrReset(&lIter, m_pStreamList);
        }
        RemoveIter(&lIter);
    }

    return bResult;
}

HX_RESULT
CHXAudioPlayer::ActualAddPostMixHook(IHXAudioHook* pHook,
                                     const HXBOOL     bDisableWrite,
                                     const HXBOOL     bFinal)
{
    if (!m_pPMixHookList || !pHook)
    {
        return HXR_FAILED;
    }

#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    /* Check if this one already exists */
    HXAudioHookInfo* h = 0;
    LISTPOSITION lp = 0;
    lp = m_pPMixHookList->GetHeadPosition();
    while( lp )
    {
        h = (HXAudioHookInfo*) m_pPMixHookList->GetNext(lp);
        if (pHook == h->pHook)
        {
            return HXR_FAILED;
        }
    }

    h = (HXAudioHookInfo*) new HXAudioHookInfo;
    h->pHook   = pHook;
    h->bDisableWrite = bDisableWrite;
    h->bFinal  = bFinal;
    h->bIgnoreAudioData = FALSE;
    h->bMultiChannelSupport = FALSE;

    IHXValues* pValues = NULL;
    if (pHook && pHook->QueryInterface(IID_IHXValues, (void**) &pValues) == HXR_OK)
    {
        UINT32 ulValue = 0;
        pValues->GetPropertyULONG32("IgnoreAudioData", ulValue);
        h->bIgnoreAudioData = (ulValue == 1);
        HX_RELEASE(pValues);
    }

    IHXAudioMultiChannel* pMultiChannel = NULL;
    if (pHook && HXR_OK == pHook->QueryInterface(IID_IHXAudioMultiChannel, (void**) &pMultiChannel))
    {
        h->bMultiChannelSupport = pMultiChannel->GetMultiChannelSupport();
    }
    HX_RELEASE(pMultiChannel);

    if (bDisableWrite)
    {
        m_bDisableWrite = bDisableWrite;
    }

    pHook->AddRef();

    // Order list by putting all bFinal == TRUE at end of list.
    if ( m_pPMixHookList->IsEmpty() || !bFinal )
    {
        m_pPMixHookList->AddHead((void*) h);
    }
    else
    {
        m_pPMixHookList->AddTail((void*) h);
    }
    m_Owner->PostMixHooksUpdated();

    ProcessAudioHook(ACTION_ADD, pHook);

    /* If we are already initialized, send the device format to the
     * post hook
     */
    if (m_bInited)
    {
        if (h->bIgnoreAudioData ||
            HXR_OK == ProcessAudioHook(ACTION_CHECK, pHook))
        {
            HXAudioFormat audioFmt;
            m_Owner->GetFormat( &audioFmt );
            pHook->OnInit( &audioFmt );
        }
    }
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */

    return HXR_OK;
}

HX_RESULT
CHXAudioPlayer::ActualRemovePostMixHook(IHXAudioHook* pHook)
{
    if (!m_pPMixHookList || !pHook)
    {
        return HXR_FAILED;
    }

#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    HXBOOL bCheckForDisableWrite  = FALSE;
    HXBOOL bFound                 = FALSE;

    HXAudioHookInfo* h = 0;
    LISTPOSITION lp, lastlp;
    lp = lastlp = 0;
    lp = m_pPMixHookList->GetHeadPosition();
    while( lp )
    {
        lastlp = lp;
        h = (HXAudioHookInfo*) m_pPMixHookList->GetNext(lp);
        if ( pHook == h->pHook )
        {
            if (h->bDisableWrite)
            {
                m_bDisableWrite         = FALSE;
                bCheckForDisableWrite   = TRUE;
            }

            ProcessAudioHook(ACTION_REMOVE, pHook);

            h->pHook->Release();
            delete h;
            h = 0;
            m_pPMixHookList->RemoveAt(lastlp);
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        return HXR_FAILED;
    }
    m_Owner->PostMixHooksUpdated();

    if ( m_pPMixHookList && bCheckForDisableWrite && m_pPMixHookList->GetCount() > 0)
    {
        HXAudioHookInfo* h = 0;
        LISTPOSITION lp, lastlp;
        lp = lastlp = 0;
        lp = m_pPMixHookList->GetHeadPosition();
        while( lp )
        {
            h = (HXAudioHookInfo*) m_pPMixHookList->GetNext(lp);
            if (h->bDisableWrite)
            {
                m_bDisableWrite = TRUE;
                break;
            }
        }

    }
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      CHXAudioPlayer::SetGranularity
 *  Purpose:
 *       The HELIX player object calls this BEFORE starting audio playback
 *       and AFTER all audio streams are created for the renderers.
 */
void CHXAudioPlayer::SetGranularity
(
    const ULONG32  ulGranularity
)
{
    m_ulGranularity = ulGranularity;
    return;
}


/************************************************************************
 *  Method:
 *              CHXAudioPlayer::Resume
 *      Purpose:
 *              Resume audio playback by writing data to the audio device.
 *              Open the audio device if it is not opened already.
 */
HX_RESULT CHXAudioPlayer::Resume()
{
    HXLOGL2(HXLOG_ADEV, "CHXAudioPlayer[%p]::Resume()", this);

    HX_RESULT theErr = HXR_OK;

    if (!m_bInited)
        return HXR_NOT_INITIALIZED;

    if (m_eState == E_PLAYING)
    {
        return HXR_OK;
    }

    m_bIsDonePlayback = FALSE;
    m_eState          = E_PLAYING;

    /* Use Audio Session Object ONLY if there are any audio streams
     * in the presentation
     */
    if (m_bHasStreams && m_lPlaybackVelocity == HX_PLAYBACK_VELOCITY_NORMAL)
    {
	// This is the audio device playback time that corresponds to
        // when this audio player resumed.
        m_ulADresumeTime = m_Owner->GetCurrentPlayBackTime();

        // This is this player's start time within its timeline. This is
        // modified when the player is seeked or resumed.
        m_ulAPstartTime = m_ulAPplaybackTime;

	HXBOOL bSessionRewindNeededForStream = FALSE;
        CHXAudioStream* s = 0;
        CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
        AddIter(&lIter);
        while( lIter != m_pStreamList->End() )
        {
            s = (CHXAudioStream*) (*lIter);
            if( s && !s->IsResumed() )
            {
                s->ResumeByPlayer();
		// Check if we need to rewind a session
		if (!bSessionRewindNeededForStream)
		{
		    // Session rewind is needed if any playing audio stream 
		    // is rewound.  This is so since in order to properly
		    // join a rewound stream, the audio session must rewind
		    // its data.  The rewound stream is such that it start
		    // time coincides with the current playback time.
		    bSessionRewindNeededForStream = (s->IsRewound() && s->IsResumed());
		}
            }
            IncrementOrReset(&lIter, m_pStreamList);
        }
        RemoveIter(&lIter);

        // Resume the audio device playback
        if (!theErr)
	{
            theErr = m_Owner->Resume(this, bSessionRewindNeededForStream);
	}
    }
    else
    {
        theErr = ResumeFakeTimeline();

        /* Send time 0 at first Resume */
        if (!theErr && m_bIsFirstResume)
        {
            m_bIsFirstResume = FALSE;
            OnTimeSync(m_ulIncreasingTimer);
        }
    }

    m_bIsStarted = TRUE;
    return ( !theErr ) ? HXR_OK : HXR_FAILED;
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::Pause
 *      Purpose:
 *              The player object calls this function to pause audio playback.
 */
HX_RESULT CHXAudioPlayer::Pause()
{
    HXLOGL2(HXLOG_ADEV, "CHXAudioPlayer[%p]::Pause()", this);

    if (m_eState == E_PAUSED)
    {
        return HXR_OK;
    }

    m_eState = E_PAUSED;

    if (m_bHasStreams && m_lPlaybackVelocity == HX_PLAYBACK_VELOCITY_NORMAL)
    {
        CHXAudioStream* s = 0;
        CHXSimpleList::Iterator lIter = m_pStreamList->Begin();\
        AddIter(&lIter);
        while( lIter != m_pStreamList->End() )
        {
            s = (CHXAudioStream*) (*lIter);
            if( s && !s->IsPaused() )
            {
                s->PauseByPlayer();
            }
            IncrementOrReset(&lIter, m_pStreamList);
        }
        RemoveIter(&lIter);
        m_Owner->Pause(this);
    }
    else
    {
        StopFakeTimeline();
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CHXAudioPlay::Stop
 *      Purpose:
 *      The player object calls this function to stop audio playback.
 *              If bFlush is TRUE, flush any data in the audio device.
 */
HX_RESULT CHXAudioPlayer::Stop
(
    const HXBOOL bFlush
)
{
    HXLOGL2(HXLOG_ADEV, "CHXAudioPlayer[%p]::Stop()", this);
    m_eState = E_STOPPED;

    m_ulAPstartTime     = 0;

    if (m_bHasStreams)
    {
        CHXAudioStream* s = 0;
        CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
        AddIter(&lIter);
        while( lIter != m_pStreamList->End() )
        {
            s = (CHXAudioStream*) (*lIter);
            if( s && !s->IsStopped() )
            {
                s->Stop();
            }
            IncrementOrReset(&lIter, m_pStreamList);
        }
        RemoveIter(&lIter);
        m_Owner->Stop(this, bFlush);
    }
    else
    {
        StopFakeTimeline();
    }

    ResetPlayer();

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::Seek
 *      Purpose:
 *              The player object calls this function to seek audio playback to
 *              the time (in milliseconds) given.
 */
HX_RESULT CHXAudioPlayer::Seek
(
const   UINT32                  ulSeekTime
)
{
    HXLOGL2(HXLOG_ADEV, "CHXAudioPlayer[%p]::Seek(): to = %lu", this, ulSeekTime);

    /* always remember this seek time.. even though there may not be any streams
     * yet for this player. This is because the streams may be created later and
     * we need to correctly apply the seek time to get the accurate time.
     */
    m_ulAPstartTime = m_ulAPplaybackTime = ulSeekTime;  // current start time for this player

    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::Seek():",
            this);

    if (m_bHasStreams && m_lPlaybackVelocity == HX_PLAYBACK_VELOCITY_NORMAL)
    {
        // Make each stream seek, too, since they own the resampling buffers.
        CHXAudioStream* s = 0;
        CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
        AddIter(&lIter);
        while(lIter != m_pStreamList->End())
        {
            s = (CHXAudioStream*) (*lIter);
            if ( s )
            {
                s->Seek(ulSeekTime);
            }
            CheckIter(&lIter);
            ++lIter;
        }
        RemoveIter(&lIter);
        m_Owner->Seek( this, ulSeekTime );

        m_ulADresumeTime = m_Owner->GetCurrentPlayBackTime();
    }
    else
    {
        StopFakeTimeline();
        m_bIsFirstResume    = TRUE;
    }

    m_ulCurrentTime         = ulSeekTime;
    m_ulCurrentScaledTime   = ulSeekTime;

    if (m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL && !m_pActiveClockSource)
    {
        // Reset the unscaled time anchor
        m_ulUnscaledTimeAnchor = m_ulCurrentTime;
        // Reset the scaled time anchor
        m_ulScaledTimeAnchor = m_ulCurrentScaledTime;
    }

    m_ulLastCurrentTimeReturned = m_ulCurrentTime;
    m_bTimeReturned         = FALSE;
    m_bHasDataInAudioDevice = FALSE;

    return HXR_OK;
}

void CHXAudioPlayer::ResetPlayer(void)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::ResetPlayer()", this);

    m_bInited           = FALSE;
    m_bHasStreams       = FALSE;
    m_bIsFirstResume    = TRUE;

    m_ulAPstartTime     = 0;
    m_ulAPplaybackTime  = 0;
    m_ulADresumeTime    = 0;

    m_ulCurrentTime     = 0;
    m_ulLastCurrentTimeReturned = 0;
    m_ulLastDeviceTimeAdjusted = 0;
    m_bTimeReturned     = FALSE;
    m_bIsLive           = FALSE;
    m_bIsStarted        = FALSE;
    m_bIsDonePlayback   = TRUE;
    m_bHasDataInAudioDevice = FALSE;

    // Delete all streams.  Remove all list items.
    if ( m_pStreamList )
    {
        CHXAudioStream* pAudioStream = 0;
        while(!m_pStreamList->IsEmpty())
        {
            pAudioStream = (CHXAudioStream*) m_pStreamList->RemoveHead();
            InvalidateIters();
            pAudioStream->ResetStream();
            pAudioStream->Release();
        }
    }

    /* We do not remove post mix hooks any more */
    /* We do not remove Stream Response Objects any more */

    /* Default value of Player format */
    m_PlayerFmt.uChannels       = 2;
    m_PlayerFmt.uBitsPerSample  = 16;
    m_PlayerFmt.ulSamplesPerSec = 16000;
    m_PlayerFmt.uMaxBlockSize   = 64000;

    m_ulLastFakeCallbackTime = 0;

    StopFakeTimeline();
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::SetupStreams
 *      Purpose:
 *              Tell each stream about the audio device format so
 *              they can setup their resamplers and buffer.
 */
void CHXAudioPlayer::SetupStreams(void)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::SetupStreams()", this);

    // Get audio device format
    m_Owner->GetFormat(&m_DeviceFmt);

    // Calculate bytes per gran
    m_ulBytesPerGran = (ULONG32)
                (((m_DeviceFmt.uChannels * ((m_DeviceFmt.uBitsPerSample==8)?1:2) *  m_DeviceFmt.ulSamplesPerSec)
                                / 1000.0) * m_ulGranularity);

    // Make sure that number of bytes per granularity is an even number.
    if ( (m_ulBytesPerGran % 2) != 0 )
        m_ulBytesPerGran++;
    /* Don't we have to calculate granularity again if we adjust
     * for even byte boundary - XXX Rahul 06/15/97
     */

    // Notify each stream
    CHXAudioStream* s = 0;
    CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
    AddIter(&lIter);
    while(lIter != m_pStreamList->End())
    {
        s = (CHXAudioStream*) (*lIter);
        if ( s )
        {
            s->Setup( &m_DeviceFmt, m_ulGranularity );
        }
        CheckIter(&lIter);
        ++lIter;
    }
    RemoveIter(&lIter);
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::OnTimeSync
 *      Purpose:
 */
HX_RESULT CHXAudioPlayer::OnTimeSync(ULONG32 ulCurrentTime)
{
    HX_RESULT theErr = HXR_OK;
    ULONG32 ulADplaybackTime;

#ifdef _MACINTOSH
    if (InterlockedIncrement(&gTIMELINE_MUTEX) > 1)
    {
       InterlockedDecrement(&gTIMELINE_MUTEX);
       return;
    }
    InterlockedDecrement(&gTIMELINE_MUTEX);
#endif

    if (m_bHasStreams && m_lPlaybackVelocity == HX_PLAYBACK_VELOCITY_NORMAL)
    {
        ulADplaybackTime = m_Owner->GetCurrentPlayBackTime();
        m_ulAPplaybackTime = (ulADplaybackTime - m_ulADresumeTime) +
                              m_ulAPstartTime;
    }
    else
    {
        m_ulAPplaybackTime = ulADplaybackTime = ulCurrentTime;
    }

    m_ulCurrentTime = m_ulAPplaybackTime ;

    AdjustTimelineForMappedStreams();

    // Compute scaled time
    UINT32 ulScaledTime = m_ulCurrentTime;
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Are we being accelerated?
    if (m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL && !m_pActiveClockSource)
    {
        ulScaledTime = ScaleCurrentTime(m_ulCurrentTime);
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    // Save the current scaled time
    m_ulCurrentScaledTime = ulScaledTime;

    // Here we need to fudge the actual time for this player
    // For now we support only one player/timeline
    if (m_pPlayerResponse)
    {
        // The current playback time of any player is the difference
        // of the current audio device playback time minus the audio
        // device time when this player started (resumed) playback
        // plus the initial start time of playback within this player's
        // timeline (usually 0 but can be something else esp. after a
        // seek).
	HXLOGL4(HXLOG_ADEV, "CHXAudioPlayer[%p]::OnTimeSync(Time=%lu) ADPlaybackTime=%lu ADResumeTime=%lu APStartTime=%lu APPlaybackTime=%lu ScaledTime=%lu", 
		this, 
		ulCurrentTime,
		ulADplaybackTime,
		m_ulADresumeTime,
		m_ulAPstartTime,
		m_ulAPplaybackTime,
		ulScaledTime);
        theErr = m_pPlayerResponse->OnTimeSync(ulScaledTime);
    }
    return theErr;
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::Setup
 *      Purpose:
 *              This is called after AS receives format and stream info
 *              from the renderers AND before packets are received from
 *              the renderer.
 */
HX_RESULT CHXAudioPlayer::Setup( ULONG32 ulGranularity)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::Setup(): gran = %lu", this, ulGranularity);

    HX_RESULT theErr = HXR_OK;


    if (m_bInited)
        return HXR_OK;

    m_ulGranularity = MAXIMUM_AUDIO_GRANULARITY; 

    // We do not go below MINIMUM_AUDIO_GRANULARITY. This will not
    // affect sending timesyncs at this lower granularity since
    // HXPlayer object uses the scheduler to send individual timesyncs
    // anyway
    if (m_ulGranularity < MINIMUM_AUDIO_GRANULARITY)
    {
        m_ulGranularity = MINIMUM_AUDIO_GRANULARITY;
    }
    else if (m_ulGranularity > MAXIMUM_AUDIO_GRANULARITY)
    {
        m_ulGranularity = MAXIMUM_AUDIO_GRANULARITY;
    }

    if (!m_bHasStreams)
    {
        m_bInited = TRUE;
        return HXR_OK;
    }

    /* If this is the second player, session object may overide
     * the granularity value.
     */
    m_ulGranularity = m_Owner->SetGranularity(m_ulGranularity);

    // Determine this player's audio format parameters based on
    // the mixer channels attributes supplied in RegisterRenderer.
    //
    // 1. Spin thru the list of registered streams and
    //    determine the desired audio device parameters.
    // 2. Check the audio format with the audio device.
    //
    CHXAudioStream* pAudioStream = 0;
    ULONG32 maxSamplesPerSec = 8000;
    ULONG32 minSamplesPerSec = 44100;
    HXBOOL    bFirst = TRUE;
    UINT16  maxChannels = 1;
    UINT16  maxBlocksize = 0;
    UINT16  maxBitsPerSample = 0;
    HXAudioFormat audioFmt;

    theErr = GetAudioPrefs();

    if (!theErr && m_pStreamList->GetCount() > 0)
    {
        CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
        AddIter(&lIter);
        while(lIter != m_pStreamList->End())
        {
            pAudioStream = (CHXAudioStream*) (*lIter); //m_pStreamList->GetNext(lp);

            if (!pAudioStream->IsAudioFormatKnown())
            {
                continue;
            }

            pAudioStream->GetFormat( &audioFmt );

            if (bFirst)
            {
                bFirst = FALSE;
                maxSamplesPerSec    = audioFmt.ulSamplesPerSec;
                minSamplesPerSec    = audioFmt.ulSamplesPerSec;
                maxChannels         = audioFmt.uChannels;
                maxBlocksize        = audioFmt.uMaxBlockSize;
                maxBitsPerSample    = audioFmt.uBitsPerSample;
            }
            else
            {
                //
                // NOTE: upsampling typically costs more CPU than downsampling
                if ( audioFmt.ulSamplesPerSec > maxSamplesPerSec )
                    maxSamplesPerSec = audioFmt.ulSamplesPerSec;
                if ( audioFmt.ulSamplesPerSec < minSamplesPerSec)
                    minSamplesPerSec = audioFmt.ulSamplesPerSec;
                //
                // NOTE: converting mono to stereo and vice versa cost about the
                // same in CPU usage.
                if ( audioFmt.uChannels > maxChannels)
                    maxChannels = audioFmt.uChannels;

                // Get max block size.
                if ( audioFmt.uMaxBlockSize > maxBlocksize )
                    maxBlocksize = audioFmt.uMaxBlockSize;

                // Get max sample width.
                if ( audioFmt.uBitsPerSample > maxBitsPerSample )
                    maxBitsPerSample = audioFmt.uBitsPerSample;
            }
            CheckIter(&lIter);
            ++lIter;
        }
        RemoveIter(&lIter);

        // Set the audio format for this Player.
        m_PlayerFmt.uMaxBlockSize       = maxBlocksize;
        // keep min. channel at stereo
        // this is a workaround for b#205546, the root cause of this is that we only
        // choose the audio format of the 1st audio stream when the playback is started
        // without audio, then audio clip is started at later time. For sure stream audio,
        // the first audio stream is often the lowest quality, thus the audio device is 
        // configured to play the lowest quality audio.
        // 
        // proper fix requires the audio device is setup only after all audio streams 
        // assoicated with the same clip have been registered
        m_PlayerFmt.uChannels           = (maxChannels < 2) ? 2 : maxChannels;
        m_PlayerFmt.uBitsPerSample      = maxBitsPerSample;

        // If user wants upsampling
        if ( m_uPrefAudioQuality > 2 )
            m_PlayerFmt.ulSamplesPerSec = maxSamplesPerSec;
        else
            m_PlayerFmt.ulSamplesPerSec = minSamplesPerSec;
    }

    if (m_bPrefUse11khz)
    {
        m_PlayerFmt.ulSamplesPerSec = 11025;
    }

    // Do audio session setup. (e.g., determine device audio
    // format, etc.
    if ( !theErr )
        theErr = m_Owner->Setup( m_bHasStreams );

    // Now let all streams know the final audio format so they
    // can resample to this format.
    if ( !theErr )
    {
        SetupStreams();
    }
    // if audio device is failed to initialized, we
    // will keep the video playing if this is not audio only source
    else if (!IsAudioOnlyTrue())
    {
        m_bHasStreams = FALSE;
        m_bInited = TRUE;
        return HXR_OK;
    }

    // Let all stream response know total number of streams.
    if (!theErr && m_pStreamRespList)
    {
        IHXAudioStreamInfoResponse* pAudioStreamInfoResponse = 0;
        CHXSimpleList::Iterator lIter = m_pStreamRespList->Begin();
        while(lIter != m_pStreamRespList->End())
        {
            pAudioStreamInfoResponse = (IHXAudioStreamInfoResponse*) (*lIter);

            CHXSimpleList::Iterator lIter2 = m_pStreamList->Begin();
            AddIter(&lIter2);
            while(lIter2 != m_pStreamList->End())
            {
                CHXAudioStream* pStream = (CHXAudioStream*) (*lIter2);
                /* Only if a stream is initialized, send it to
                 * Response object. If not, we will send it when it
                 * gets initialized (in StreamInitialized() call)
                 */
                if (pStream->IsInitialized())
                {
                    pAudioStreamInfoResponse->OnStream(pStream);
                }
                CheckIter(&lIter2);
                ++lIter2;
            }
            RemoveIter(&lIter2);
            ++lIter;
        }
    }

    // All renderers should have checked in by now!
    // Call post mix process hooks in list and provide the audio format.
    if (!theErr && m_pPMixHookList)
    {
        HXAudioFormat audioFmt;
        m_Owner->GetFormat( &audioFmt );
        HXAudioHookInfo* pPMixHookInfo = 0;
        CHXSimpleList::Iterator lIter = m_pPMixHookList->Begin();
        while(lIter != m_pPMixHookList->End())
        {
            pPMixHookInfo = (HXAudioHookInfo*) (*lIter);
            if (pPMixHookInfo->bIgnoreAudioData ||
                HXR_OK == ProcessAudioHook(ACTION_CHECK, pPMixHookInfo->pHook))
            {
                pPMixHookInfo->pHook->OnInit( &audioFmt );
            }
            ++lIter;
        }
    }

    if (!theErr)
    {
        m_bInited = TRUE;
        /* Only change the state to initialized if we were in a stopped
         * state earlier. It is possible to be in Playing state and be still
         * in this function. This will happen if we have started the
         * timeline as a fake timeline and later an audio stream joins the
         * presentation thereby converting fake to audio timeline
         * (delayed audio source in SMIL playback)
         */
        if (m_eState == E_STOPPED)
        {
            m_eState = E_INITIALIZED;
        }
    }

    return theErr;
}

ULONG32 CHXAudioPlayer::GetCurrentPlayBackTime(void)
{
    if (m_eState != E_PLAYING)
    {
		return (m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL ? 
				m_ulCurrentScaledTime : m_ulCurrentTime);
    }

    // The current playback time of any player is the difference
    // of the current audio device playback time minus the audio
    // device time when this player started (resumed) playback
    // plus the initial start time of playback within this player's
    // timeline (usually 0 but can be something else esp. after a
    // seek).
    if (!m_bHasStreams || m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL)
    {
        UpdateFakeTimelineTime();

        m_ulCurrentTime = m_ulIncreasingTimer;
    }
    else
    {
        m_ulCurrentTime = (m_Owner->GetCurrentPlayBackTime() -
                    m_ulADresumeTime) + m_ulAPstartTime;
    }

    m_ulAPplaybackTime = m_ulCurrentTime;

    AdjustTimelineForMappedStreams();

    // Compute scaled time
    UINT32 ulScaledTime = m_ulCurrentTime;
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Are we being accelerated?
    if (m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL && !m_pActiveClockSource)
    {
        ulScaledTime = ScaleCurrentTime(m_ulCurrentTime);
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    // Save the current scaled time
    m_ulCurrentScaledTime = ulScaledTime;

    return ulScaledTime;
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::GetStreamCount
 *      Purpose:
 *              Get the number of streams associated with this player.
 */
UINT16 CHXAudioPlayer::GetStreamCount()
{
    return m_pStreamList->GetCount();
}

/************************************************************************
 *  Method:
 *              CHXAudioPlayer::SetStreamInfoResponse
 *      Purpose:
 *              Add the stream info response interface to our list.
 */
STDMETHODIMP CHXAudioPlayer::SetStreamInfoResponse
(
    IHXAudioStreamInfoResponse*    pResponse
)
{
    if (!pResponse || !m_pStreamRespList)
    {
        return HXR_FAILED;
    }

    /* Add to the stream response list */
    LISTPOSITION lPos = m_pStreamRespList->Find(pResponse);
    if (lPos)
    {
        return HXR_FAILED;
    }

    m_pStreamRespList->AddTail((void*) pResponse);
    pResponse->AddRef();         // Released in destructor
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *  CHXAudioPlayer::RemoveStreamInfoResponse
 *  Purpose:
 *  Remove stream info response that was added earlier
 */
STDMETHODIMP CHXAudioPlayer::RemoveStreamInfoResponse
(
    IHXAudioStreamInfoResponse*    pResponse
)
{
    /* Add to the stream response list */
    if (pResponse && m_pStreamRespList)
    {
        LISTPOSITION lPos = m_pStreamRespList->Find(pResponse);
        if (lPos)
        {
            m_pStreamRespList->RemoveAt(lPos);
            pResponse->Release();         // Released in destructor
            return HXR_OK;
        }
    }

    return HXR_FAILED;
}

UINT16
CHXAudioPlayer::NumberOfResumedStreams(void)
{
    UINT16 uNumActive = 0;
    if (m_pStreamList && m_pStreamList->GetCount() > 0)
    {
        CHXAudioStream* pStream = 0;
        CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
        AddIter(&lIter);
        while(lIter != m_pStreamList->End())
        {
            pStream = (CHXAudioStream*) (*lIter);
            if (pStream->GetState() == E_PLAYING)
            {
                uNumActive++;
            }
            CheckIter(&lIter);
            ++lIter;
        }
        RemoveIter(&lIter);
    }

    return uNumActive;
}

void CHXAudioPlayer::StreamInitialized(CHXAudioStream* pAudioStream)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::StreamInitialized(): stream [%p]", this, pAudioStream);

    /* If we are already initialized, it means this stream was added mid-
     * presentation and gogt initialized later. In this case, report arrival
     * of this stream to all StreamInfoResponse objects registered with the
     * player
     */

    if (m_pStreamRespList && m_bInited)
    {
        IHXAudioStreamInfoResponse* pAudioStreamInfoResponse = 0;
        CHXSimpleList::Iterator lIter = m_pStreamRespList->Begin();
        AddIter(&lIter);
        while(lIter != m_pStreamRespList->End())
        {
            pAudioStreamInfoResponse = (IHXAudioStreamInfoResponse*) (*lIter);
            pAudioStreamInfoResponse->OnStream(pAudioStream);
            CheckIter(&lIter);
            ++lIter;
        }
        RemoveIter(&lIter);
    }

    m_bHasStreams = TRUE;

    // If we were using a clock source, then deactivate it
    DeactivateClockSource();
}

/************************************************************************
 *  Method:
 *      CHXAudioPlayer::OnTimerCallback
 *  Purpose:
 *      Timer callback when implementing fake timeline.
 */
void    CHXAudioPlayer::OnTimerCallback()
{
    UpdateFakeTimelineTime();

    OnTimeSync(m_ulIncreasingTimer);

    /* A call to timesync may result in stopping
     * playback and we do not want to have any more
     * time syncs.
     */
    /* Put this back in the scheduler.
     */
    if (m_bInited && m_eState == E_PLAYING && !m_ulCallbackID)
    {
        *m_pFakeAudioCBTime += (int) (m_ulGranularity*1000);
        m_ulCallbackID = m_pScheduler->AbsoluteEnter( this,
                                                      *((HXTimeval*)m_pFakeAudioCBTime));
    }
}


void
CHXAudioPlayer::SetLive(HXBOOL bIsLive)
{
    m_bIsLive = bIsLive;
    CHXAudioStream* s = 0;
    CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
    AddIter(&lIter);
    while(lIter != m_pStreamList->End())
    {
        s = (CHXAudioStream*) (*lIter);
        s->SetLive(m_bIsLive);
        CheckIter(&lIter);
        ++lIter;
    }
    RemoveIter(&lIter);
}

void
CHXAudioPlayer::AudioFormatNowKnown(void)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::AudioFormatNowKnown()", this);

    HX_ASSERT(m_bInited);

    /* When : More than one audio stream created after initialization
     * and one of them already called this function earlier
     */
    if (m_bHasStreams)
    {
        return;
    }

    m_bHasStreams   = TRUE;
    m_bInited   = FALSE;

    m_Owner->CheckIfLastNMilliSecsToBeStored();

    /* internal setup */
    HX_RESULT theErr = Setup(m_ulGranularity);
    if (theErr != HXR_OK)
    {
        IHXErrorMessages* pErrorMessage = NULL;
        m_pContext->QueryInterface(IID_IHXErrorMessages, (void**) &pErrorMessage);
        if (pErrorMessage)
        {
            pErrorMessage->Report(HXLOG_ERR, theErr, 0, NULL, NULL);
            pErrorMessage->Release();
        }

        return;
    }

    HX_ASSERT(m_bInited);

    /* If we have not yet resumed, it is simply setting the audio
     * player from fake timeline mode to audio timeline mode.
     * Instead if we have already been resumed and are acting as a
     * fake timeline, we need to kinda pause from being a fake timeline to
     * an audio timeline, In this process, we may have to seek the audio
     * device to generate right timesyncs.
     */

    /* If we are already resumed, we need to call resume again internally */
    if (m_bIsStarted)
    {
        StopFakeTimeline();
        // If we were using a clock source, then deactivate it
        DeactivateClockSource();
        Seek(m_ulCurrentTime);
        // only resume the owner if we are in a play state...
        // otherwise owner will be resumed when the player gets resumed.
        if (m_eState == E_PLAYING)
        {
            m_Owner->Resume();
        }
    }
    else
    {
        /* Cool! HXPlayer will call resume later */
    }
}

HXBOOL CHXAudioPlayer::IsAudioOnlyTrue(void)
{
    HXBOOL             bRetValue     = TRUE;
    IHXPlayer*       pPlayer       = NULL;
    IUnknown*        pUnknown      = NULL;
    IHXStreamSource* pStreamSource = NULL;
    IHXStream*       pStream       = NULL;
    
    m_pContext->QueryInterface(IID_IHXPlayer, (void**)&pPlayer);
    HX_ASSERT(pPlayer);

    UINT16 uNumSources = pPlayer->GetSourceCount();

    for (UINT16 i=0; bRetValue && i < uNumSources; i++)
    {
        pPlayer->GetSource(i, pUnknown);
        pUnknown->QueryInterface(IID_IHXStreamSource,
                                 (void**) &pStreamSource);
        HX_RELEASE(pUnknown);
        HX_ASSERT(pStreamSource);

        UINT16 uNumStreams = pStreamSource->GetStreamCount();
        for (UINT16 j=0; bRetValue && j < uNumStreams; j++)
        {
            pStreamSource->GetStream(j, pUnknown);
            pUnknown->QueryInterface(IID_IHXStream,
                                     (void**) &pStream);
            HX_RELEASE(pUnknown);
            HX_ASSERT(pStream);

            IHXValues* pHeader = pStream->GetHeader();
            if (pHeader)
            {
                if (!IsThisAudioStream(pHeader))
                {
                    bRetValue = FALSE;
                }
                pHeader->Release();
            }
            pStream->Release();
        }

        HX_RELEASE(pStreamSource);
    }

    HX_RELEASE(pPlayer);

    return bRetValue;
}

HXBOOL
CHXAudioPlayer::IsThisAudioStream(IHXValues* pHeader)
{
    CHXSimpleList::Iterator ndxStream = m_pStreamList->Begin();
    AddIter(&ndxStream);
    while(ndxStream != m_pStreamList->End())
    {
        CHXAudioStream* pAudioStream = (CHXAudioStream*) (*ndxStream);
        IHXValues* pAudioHeader = pAudioStream->GetStreamInfo();
        if (pAudioHeader == pHeader)
        {
            HX_RELEASE(pAudioHeader);
            return TRUE;
        }
        HX_RELEASE(pAudioHeader);
        CheckIter(&ndxStream);
        ++ndxStream;
    }
    RemoveIter(&ndxStream);

    return FALSE;
}

void CHXAudioPlayer::AdjustTimelineForMappedStreams()
{
    HX_RESULT hxr                 = HXR_OK;
    UINT32    ulCurrentDeviceTime = m_ulCurrentTime;

#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
    //xxxgfw Calling GetProperFudgeMap() means we iterate over the
    //entire stream list. We may want to look into making this
    //cheaper. We really only need to iterate over this list if:
    //
    // o the number of streams in the list changes.
    // o the playing states of any stream changes
    // o the mute state of any stream changes.
    //
    // Right now we don't have the code for that, and, the stream
    // list is very small usually.
    //
    CHXAudioStream* pAudioStream = GetProperFudgeMap();
    if( pAudioStream )
    {
        UINT32 ulAdjustedTime = 0L;
        double dBytesPlayed   = m_Owner->GetNumBytesPlayed();

        hxr = pAudioStream->ConvertCurrentTime( dBytesPlayed, m_ulCurrentTime, ulAdjustedTime);
        
        if( SUCCEEDED(hxr) )
        {
            // This is to avoid stall at end of the presentation. The RA stream
            // may have a duration of say 30 seconds but the actual data may be
            // only for 29.9 seconds. In this case, audio stream will never
            // return time more than 29.9 seconds and we will get stalled.  To
            // avoid this, we wait for at most MAX_WAIT_AT_SAME_TIME (== max
            // granularity+50 ms) at the same timestamp. If we find that we are
            // pushing more data in the audio device but the audio stream is
            // reporting the same time for the past MAX_WAIT_AT_SAME_TIME, we
            // increment our timestamp by the real time elapsed since the last
            // update.  This code will only trigger when we are near the end of
            // presentation.
            if( m_bTimeReturned &&
                (!m_bNewMapStarting) &&
                (((INT32)(ulAdjustedTime - m_ulLastCurrentTimeReturned)) <= 0 ) &&
                (((INT32)(ulCurrentDeviceTime - m_ulLastDeviceTimeAdjusted)) > MAX_WAIT_AT_SAME_TIME )
                )
            {
                m_ulCurrentTime = m_ulLastCurrentTimeReturned +
                    (ulCurrentDeviceTime-m_ulLastDeviceTimeAdjusted);
                
                m_ulLastDeviceTimeAdjusted = ulCurrentDeviceTime;
            }
            else
            {
                m_ulLastAdjustedTimeDiff = 0;
                if( ulAdjustedTime >= m_ulCurrentTime )
                {
                    m_ulLastAdjustedTimeDiff = ulAdjustedTime-m_ulCurrentTime;
                }
                m_ulCurrentTime = ulAdjustedTime;
            }
        }
    }
#endif /* HELIX_FEATURE_AUDIO_INACCURATESAMPLING */

    // Never go back in time
    if( !m_bTimeReturned )
    {
        m_bTimeReturned             = TRUE;
        m_ulLastCurrentTimeReturned = m_ulCurrentTime;
        m_ulLastDeviceTimeAdjusted  = ulCurrentDeviceTime;
    }
    else if (m_ulCurrentTime <= m_ulLastCurrentTimeReturned)
    {
        m_ulCurrentTime = m_ulLastCurrentTimeReturned;
    }
    else
    {
        m_ulLastDeviceTimeAdjusted  = ulCurrentDeviceTime;
        m_ulLastCurrentTimeReturned = m_ulCurrentTime;
    }

}

HX_RESULT
CHXAudioPlayer::ResumeFakeTimeline(void)
{
    HX_RESULT   rc = HXR_OK;

    HX_ASSERT(!m_bHasStreams || m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL);

    HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();

    m_pFakeAudioCBTime->tv_sec = lTime.tv_sec;
    m_pFakeAudioCBTime->tv_usec = lTime.tv_usec;

    m_ulIncreasingTimer = m_ulCurrentTime;
    m_ulLastFakeCallbackTime = GetFakeTimelineTime();

    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::ResumeFakeTimeline() Time=%lu LastCallbackTime=%lu", 
	    this, 
	    m_ulIncreasingTimer,
	    m_ulLastFakeCallbackTime);

    // Are we using a clock source?
    if (m_pActiveClockSource)
    {
        // We are using an active clock source
        // so we need to resume it.
        m_pActiveClockSource->ResumeClockSource();
    }

    *m_pFakeAudioCBTime += (int) (m_ulGranularity*1000);

    m_ulCallbackID = m_pScheduler->AbsoluteEnter(this,
                                                 *((HXTimeval*) m_pFakeAudioCBTime));

    return rc;
}

HX_RESULT CHXAudioPlayer::Func()
{
    m_ulCallbackID = 0;
    OnTimerCallback();
    return HXR_OK;
}


HX_RESULT CHXAudioPlayer::StopFakeTimeline(void)
{
    HX_RESULT   rc = HXR_OK;

    if(m_ulCallbackID && m_pScheduler)
    {
        m_pScheduler->Remove(m_ulCallbackID);
    }
    // Are we using a clock source?
    if (m_pActiveClockSource)
    {
        // We are using a clock source, so we need
        // to pause the clock source
        m_pActiveClockSource->PauseClockSource();
    }

    return rc;
}

double
CHXAudioPlayer::NumberOfBytesWritten()
{
    return m_Owner->NumberOfBytesWritten();
}

double
CHXAudioPlayer::ConvertMsToBytes(UINT32 ulTime)
{
    return m_Owner->ConvertMsToBytes(ulTime);
}

void
CHXAudioPlayer::UpdateStreamLastWriteTime()
{
    // Make each stream seek, too, since they own the resampling buffers.
    CHXAudioStream* s = 0;
    CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
    AddIter(&lIter);
    while(lIter != m_pStreamList->End())
    {
        s = (CHXAudioStream*) (*lIter);
        s->UpdateStreamLastWriteTime();
        CheckIter(&lIter);
        ++lIter;
    }
    RemoveIter(&lIter);
}

void
CHXAudioPlayer::SaveLastNMilliSeconds(HXBOOL bSave, UINT32 ulNMilliSeconds)
{
    // Make each stream seek, too, since they own the resampling buffers.
    CHXAudioStream* s = 0;
    CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
    AddIter(&lIter);
    while(lIter != m_pStreamList->End())
    {
        s = (CHXAudioStream*) (*lIter);
        s->SaveLastNMilliSeconds(bSave, ulNMilliSeconds);
        CheckIter(&lIter);
        ++lIter;
    }
    RemoveIter(&lIter);
}

void
CHXAudioPlayer::RewindPlayer(INT32 lTimeToRewind)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::RewindPlayer() START lTimeToRewind: %ld",
            this,
            lTimeToRewind
            );

    if (m_pStreamList->GetCount() == 0)
    {
        return;
    }

    // Make each stream seek, too, since they own the resampling buffers.
    CHXAudioStream* s = 0;
    CHXSimpleList::Iterator lIter = m_pStreamList->Begin();
    AddIter(&lIter);
    while(lIter != m_pStreamList->End())
    {
        s = (CHXAudioStream*) (*lIter);
        s->RewindStream(lTimeToRewind);
        CheckIter(&lIter);
        ++lIter;
    }
    RemoveIter(&lIter);

    HXLOGL3(HXLOG_ADEV, "CHXAudioPlayer[%p]::RewindPlayer(TimeToRewind=%ld) END APplaybackTime=%lu CurrentTime=%lu", 
	    this,
	    lTimeToRewind,
	    m_ulAPplaybackTime,
	    m_ulCurrentTime);
}

HX_RESULT
CHXAudioPlayer::ProcessAudioHook(PROCESS_ACTION action,
                                 IHXAudioHook* pAudioHook)
{
    return HXR_OK;
}

STDMETHODIMP CHXAudioPlayer::SetError( HX_RESULT theErr )
{
    if (theErr != HXR_OK)
    {
        IHXErrorMessages* pErrorMessage = NULL;
        m_pContext->QueryInterface(IID_IHXErrorMessages, (void**) &pErrorMessage);
        if (pErrorMessage)
        {
            pErrorMessage->Report(HXLOG_ERR, theErr, 0, NULL, NULL);
            pErrorMessage->Release();
        }
    }
    return HXR_OK;
}

UINT32 CHXAudioPlayer::ScaleCurrentTime(UINT32 ulTime)
{
    UINT32 ulRet = ulTime;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Compute the unscaled time difference. The input
    // time ulTime is from the fake timeline and it
    // is ALWAYS moving forward. Therefore, we can
    // always be sure this difference will be positive.
    UINT32 ulUnscaledDiff = ulTime - m_ulUnscaledTimeAnchor;
    // Compute the scaled difference. Since we are resetting the
    // anchors every so often, this is guaranteed not to overflow.
    INT32 lScaledDiff = ((INT32) ulUnscaledDiff) * m_lPlaybackVelocity / 100;
    // When we are in reverse playback, don't
    // let the time scaled sync wrap around - stop it at zero.
    // Right now we allow positive velocities to 
    // wrap the timesync around 32-bits.
    if (m_lPlaybackVelocity < 0 && ((UINT32) -lScaledDiff) > m_ulScaledTimeAnchor)
    {
        // This will force ulRet to be zero
        // in the calculation below.
        lScaledDiff = - ((INT32) m_ulScaledTimeAnchor);
    }
    // Compute scaled time
    ulRet = m_ulScaledTimeAnchor + lScaledDiff;
    // In order to make sure that we don't overflow INT32,
    // every so often we reset the scaled and unscaled
    // time anchors. If we reset once a minute in system
    // time, then the differences are guaranteed not to overflow
    // 32-bit calculations (assuming a maximum acceleration 
    // of 200x, which is twice the current maximum acceleration
    // of 100x.)
    if (ulUnscaledDiff > 60000)
    {
        // Re-plant the anchors
        m_ulUnscaledTimeAnchor = ulTime;
        m_ulScaledTimeAnchor   = ulRet;
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return ulRet;
}

UINT32 CHXAudioPlayer::GetFakeTimelineTime()
{
    UINT32 ulRet = HX_GET_TICKCOUNT();

    if (m_pActiveClockSource)
    {
        m_pActiveClockSource->GetClockSourceTime(ulRet);
    }

    return ulRet;
}

void CHXAudioPlayer::UpdateFakeTimelineTime()
{
    // Get the fake timeline time (either from
    // tickcount or from clock source)
    UINT32 ulCurrentTime = GetFakeTimelineTime();
    // If we are using a clock source, then we
    // use that absolute time. If we are using the
    // tick count, then we use the difference 
    // between the last two times.
    if (m_pActiveClockSource)
    {
        m_ulIncreasingTimer  = ulCurrentTime;
    }
    else
    {
        m_ulIncreasingTimer += CALCULATE_ELAPSED_TICKS(m_ulLastFakeCallbackTime, ulCurrentTime);
    }
    m_ulLastFakeCallbackTime = ulCurrentTime;
}

HX_RESULT CHXAudioPlayer::ClockSourceSwitch(IHXClockSource* pSource, HXBOOL bAdd)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSource)
    {
        // Are we adding this source?
        if (bAdd)
        {
            // Add this clock source
            //
            // Do we have any audio streams?
            if (m_bHasStreams)
            {
                // We have audio services audio streams (which take
                // precedence over any IHXClockSource's) so
                // just queue the clock source at the tail
                retVal = AddToClockSourceQueue(pSource, FALSE);
            }
            else
            {
                // Do we already have an active clock source?
                if (m_pActiveClockSource)
                {
                    // Yes, we already have an active clock source,
                    // so just add this source to the queue. If and
                    // when the active clock source is unregistered,
                    // then this clock source may get a shot.
                    retVal = AddToClockSourceQueue(pSource, FALSE);
                }
                else
                {
                    // No we don't have an active clock source
                    // so install this one as the new one
                    retVal = InstallNewClockSource(pSource);
                }
            }
        }
        else
        {
            // Remove this clock source
            //
            // Is this clock source the active clock source?
            if (m_pActiveClockSource == pSource)
            {
                // We are removing the active clock source
                //
                // Tell the active clock source we are no longer
                // using it as the active source
                m_pActiveClockSource->CloseClockSource();
                // Release our ref on the active source
                HX_RELEASE(m_pActiveClockSource);
                // Install any inactive clock source (this will
                // check to make sure we don't have an audio services
                // stream now)
                retVal = InstallAnyInactiveClockSource();
            }
            else
            {
                // This source is not the active source, so it
                // was most likely queued. So just remove it
                // from the queue
                RemoveFromClockSourceQueue(pSource);
                // Clear the return value
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

void CHXAudioPlayer::ClearClockSourceQueue()
{
    if (m_pInactiveClockSourceQueue)
    {
        while (m_pInactiveClockSourceQueue->GetCount() > 0)
        {
            IHXClockSource* pSource = (IHXClockSource*) m_pInactiveClockSourceQueue->RemoveHead();
            HX_RELEASE(pSource);
        }
    }
}

HX_RESULT CHXAudioPlayer::AddToClockSourceQueue(IHXClockSource* pSource, HXBOOL bHead)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSource)
    {
        if (!m_pInactiveClockSourceQueue)
        {
            m_pInactiveClockSourceQueue = new CHXSimpleList();
        }
        if (m_pInactiveClockSourceQueue)
        {
            // Clear the return value
            retVal = HXR_OK;
            // AddRef the source before going on the queue
            pSource->AddRef();
            // Put the source on the queue
            if (bHead)
            {
                // Add to the head
                m_pInactiveClockSourceQueue->AddHead((void*) pSource);
            }
            else
            {
                // Add to the tail
                m_pInactiveClockSourceQueue->AddTail((void*) pSource);
            }
        }
    }

    return retVal;
}

void CHXAudioPlayer::RemoveFromClockSourceQueue(IHXClockSource* pSource)
{
    if (pSource && m_pInactiveClockSourceQueue && m_pInactiveClockSourceQueue->GetCount() > 0)
    {
        // Find the source in the queue
        LISTPOSITION pos = m_pInactiveClockSourceQueue->Find((void*) pSource);
        // Was it in the queue?
        if (pos)
        {
            // Remove it from the queue
            m_pInactiveClockSourceQueue->RemoveAt(pos);
            // Release the list's ref on the source
            HX_RELEASE(pSource);
        }
    }
}

HX_RESULT CHXAudioPlayer::InstallNewClockSource(IHXClockSource* pSource)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSource)
    {
        // Tell the source we want to start using
        // it as the active clock
        retVal = pSource->OpenClockSource(GetCurrentPlayBackTime());
        if (SUCCEEDED(retVal))
        {
            HX_ASSERT(m_pActiveClockSource == NULL);
            // Install this source as the active clock source
            m_pActiveClockSource = pSource;
            m_pActiveClockSource->AddRef();
            // If the audio player is in a stopped or paused
            // state, then we don't need to do anything here.
            // If we are in a playing state, then we need to
            // set up the state correctly.
            if (m_eState == E_PLAYING)
            {
                // Now resume the clock source
                m_pActiveClockSource->ResumeClockSource();
            }
        }
    }

    return retVal;
}

HX_RESULT CHXAudioPlayer::InstallAnyInactiveClockSource()
{
    HX_RESULT retVal = HXR_OK;

    // Do we: a) still not have any audio services streams and
    //        b) have any queued clock sources?
    // If both (a) and (b) are true, then install the next clock source
    if (!m_bHasStreams && m_pInactiveClockSourceQueue && m_pInactiveClockSourceQueue->GetCount() > 0)
    {
        // Dequeue the next clock source
        IHXClockSource* pNextSource = (IHXClockSource*) m_pInactiveClockSourceQueue->RemoveHead();
        // Install the new clock source
        retVal = InstallNewClockSource(pNextSource);
        // Release the list's ref on the clock source
        HX_RELEASE(pNextSource);
    }

    return retVal;
}

void CHXAudioPlayer::DeactivateClockSource()
{
    // Were we using a clock source?
    if (m_pActiveClockSource)
    {
        // We were using a clock source, so we need to close
        // the active clock source.
        m_pActiveClockSource->CloseClockSource();
        // Add this clock source to the inactive queue, but
        // add it at the head
        AddToClockSourceQueue(m_pActiveClockSource, TRUE);
        // Now we can uninstall it as the active clock source
        HX_RELEASE(m_pActiveClockSource);
    }
}

UINT32 CHXAudioPlayer::GetLastAdjustedTimeDiff()
{
    return m_ulLastAdjustedTimeDiff;
}

CHXAudioStream* CHXAudioPlayer::GetProperFudgeMap()
{
    CHXAudioStream* pStream1 = NULL;
    CHXSimpleList::Iterator itor;

    HX_ASSERT(m_pStreamList);
    
    // We want to return the audio stream that has the most proper RealAudio
    // Fudge-timestamp-map. This is how we rank them:
    //
    // 'mapping' means to use the RA fudge map.
    // 'offset' means to use the offset from the player when the stream is started.
    //
    // 1) 'RA Mapped'     -- audible -- playing (mapping)
    // 2) 'Non-RA Mapped' -- audible -- playing (offset) 
    // 3) 'RA Mapped'     -- muted   -- playing (mapping)
    // 4) 'Non-RA Mapped' -- muted   -- playing (offset)
    // 5) 'RA Mapped'     -- N/A     -- paused  (offset)
    // 6) 'Non-RA Mapped' -- N/A     -- paused  (offset) 
    //
    // Basically, just evaluate in this order:
    //
    // o Always prefer playing over paused streams.
    // o Always prefer non-muted over muted streams.
    // o Always prefer RA-Mapped stream over non-RA mapped stream.
    //

    if( m_pStreamList && m_pStreamList->GetCount()!=0  )
    {
        //Go through our list and find the max, given the ranking system above.
        itor = m_pStreamList->Begin();

        pStream1 = (CHXAudioStream*)*itor;
        ++itor;

        //Now test against the rest of the list.
        while( itor != m_pStreamList->End() )
        {
            CHXAudioStream* pStream2 = (CHXAudioStream*)*itor;

	    //See if the stream1 < stream2 given ranking above. First, check the
	    //playing state.
	    HXBOOL bStream1Playing = (pStream1->GetState() == E_PLAYING);
	    HXBOOL bStream2Playing = (pStream2->GetState() == E_PLAYING);
	    if( bStream1Playing == bStream2Playing  )
	    {
		//We are not sure yet. Check the muted state.
		if( pStream1->IsAudible() == pStream2->IsAudible() )
		{
		    //We are not sure yet. Now check if it is a RA stream.
		    HXBOOL bStream1IsRA = pStream1->IsRealAudioStream();
		    HXBOOL bStream2IsRA = pStream2->IsRealAudioStream();
		    if (bStream1IsRA == bStream2IsRA)
		    {
			// Both streams are equal by RA and all other criteria.
			// Stream2 can prevail only if last used since we want
			// the mapping stream to be sticky and not to be switched 
			// out by an equally weighted stream.
			if (pStream2 == m_pLastMappingStream)
			{
			    pStream1 = pStream2;
			}
		    }
		    else if (bStream2IsRA)
		    {
			// The 2nd stream is RA, the first is not.
			// Stream2 wins.
			pStream1 = pStream2;
		    }
		}
		else if( pStream1->IsAudible() )
		{
		    //Stream2 must not be muted, it wins.
		    pStream1 = pStream2;
		}
	    }
	    else if( !bStream1Playing )
	    {
		//Stream1 is not playing, so stream 2 must be. It is
		//automatically higher in rank.
		pStream1 = pStream2;
	    }
            
            ++itor;
        }
    }


    m_bNewMapStarting = FALSE;
    if( m_pLastMappingStream != pStream1 )
    {
        //We are chaning streams. 
        HXLOGL3( HXLOG_ADEV,
                 "(%p)CHXAudioPlayer::GetProperFudgeMap: Switching streams. From %p to %p",
                 this,
                 m_pLastMappingStream,
                 pStream1
                 );
        
        m_pLastMappingStream = pStream1;
        m_bNewMapStarting    = TRUE;
    }
    return pStream1;
}
