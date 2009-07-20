/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudses.cpp,v 1.77 2007/02/23 20:31:16 milko Exp $
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
#include "safestring.h"
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/math.h"


#ifndef WIN32_PLATFORM_PSPC
#  include "hlxclib/signal.h"
#else
#  include <winbase.h>
#  include <dbgapi.h>
#endif

//#define _TESTING    1
#ifdef _TESTING
#  include <fcntl.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  if defined (_WINDOWS) || defined (_WIN32)
#    include <io.h>
#  endif
#endif

//#include "racodec.h"
#include "hxresult.h"

#include "hxcom.h"

#include "hxshtdn.h"
#include "hxengin.h"
#include "hxausvc.h"
#include "hxrasyn.h"
#include "hxprefs.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxmutex.h"
#include "auderrs.h"
#include "hxslist.h"
#include "hxtick.h"

#ifdef _SYMBIAN
#include "audsymbian.h"
#else
#include "hxaudev.h"
#endif

#include "hxaudply.h"
#include "hxaudstr.h"
#include "hxaudvol.h"

#include "timeval.h"

#include "hxaudses.h"
#include "hxaudtyp.h"

#include "hxmixer.h"
#include "hxthread.h"
#if defined (HELIX_FEATURE_RESAMPLER) && !defined (HELIX_CONFIG_FIXEDPOINT)
#include "hxrsmp2.h"
#endif /* HELIX_FEATURE_RESAMPLER && !HELIX_CONFIG_FIXEDPOINT */

#include "hxprefs.h"
#include "hxprefutil.h"

class CHXAudioPlayer;

#include "hxtlogutil.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define SOME_INSANELY_LARGE_VALUE       3600000 /* 1 hour */

#ifdef _WIN32
#define HXMSG_QUIT              (WM_USER + 200) /* Exit from the thread */ 
#define HXMSG_RESUME            (WM_USER + 201) /* Resume audio thread */
#define HXMSG_STOP              (WM_USER + 202) /* Stop audio thread */
#else
#define HXMSG_QUIT              200             /* Exit from the thread */
#define HXMSG_RESUME            201             /* Change timer value */ 
#define HXMSG_STOP              202             /* Stop audio thread */
#endif /*_WIN32*/

#ifdef _MACINTOSH
HXBOOL    CHXAudioSession::zm_Locked=FALSE;
#endif /* _MACINTOSH */

#ifdef _UNIX
IHXPreferences* z_pIHXPrefs =  NULL;
#endif


//XXXgfw can whatever this SH4 is be moved down into the audio device
//code for this chipset? This is how we don't allow certain sample
//rates in other devices; by returning HXR_FAIL from _imp_checkformat.
#if !defined(SH4)
const unsigned short z_anValidSampleRates[] = { 8000,
                                                11025,
                                                16000,
                                                22050,
                                                32000,
                                                44100,
                                                48000,
                                                49512
};
#else
const unsigned short z_anValidSampleRates[] = { 11025,
                                                22050,
                                                44100
};
#endif

struct tableEntry
{
    UINT16 usSampleRate;
    UINT8  usChannels;
    UINT8  usBits;

    inline void set(UINT16 sr,
                    UINT8  chan,
                    UINT8  bits
                    )
        {
            usSampleRate = sr;
            usChannels   = chan;
            usBits       = bits;
        }
};

/*
 *  HXAudioSession methods
 */
/************************************************************************
 *  Method:
 *              CHXAudioSession::CHXAudioSession()
 *      Purpose:
 *              Constructor. Create player list.
 */
CHXAudioSession::CHXAudioSession()
    : m_pContext(NULL)
    , m_lRefCount(0)
    , m_pScheduler (0)
    , m_pInterruptState(0)
    , m_pPlayerBuf (NULL)
    , m_pSessionBuf (NULL)
    , m_ulGranularity(0)
    , m_dGranularity((double) 0.)
    , m_ulMinimumStartupPushdown(MINIMUM_AUDIO_STARTUP_PUSHDOWN)
    , m_nPercentage(CHECK_AUDIO_INTERVAL)
    , m_ulTargetBlocksTobeQueued(3)
    , m_ulMinBlocksTobeQueued(1)
    , m_ulMinBlocksTobeQueuedAtStart(2)
    , m_ulMaxBlocksToPlayPerGranule(3)
    , m_ulBytesPerGran(0)
    , m_ulBlocksWritten(0)
    , m_ulCallbackID(0)
    , m_bFakeAudioTimeline(FALSE)
    , m_bShouldOpenOnCoreThread(FALSE)
    , m_bToBeReOpened(FALSE)
    , m_pDeviceCallback(NULL)
    , m_bHasStreams(FALSE)
    , m_ulIncreasingTimer (0)
    , m_ulCurrentTime (0)
    , m_ulLastAudioTime(0)
    , m_ulLastAudioReturnTime(0)
    , m_ulLastSystemTime(0)
    , m_bAtLeastOneTimeReceived(FALSE)
    , m_ulStartTime(0)
    , m_bTimeSyncReceived(FALSE)
    , m_bPaused(TRUE)
    , m_bStoppedDuringPause(FALSE)
    , m_ulLastFakeCallbackTime(0)
    , m_pFakeAudioCBTime(0)
    , m_pInDataPtr(0)
    , m_pOutDataPtr(0)
    , m_pPlayerResponse(0)
    , m_bFirstPlayAudio(TRUE)
    , m_uVolume(HX_INIT_VOLUME)
    , m_bMute(FALSE)
    , m_dBufEndTime((double) 0.)
    , m_bDisableWrite(FALSE)
    , m_bInPlayAudio(FALSE)
    , m_bInActualResume(FALSE)
    , m_dNumBytesWritten((double)0)
    , m_dNumBytesPlayed((double)0)
    , m_bInited(FALSE)
    , m_pAudioDev(0)
    , m_pCurrentAudioDev(NULL)
    , m_pReplacedAudioDev(NULL)
    , m_bReplacedDev(FALSE)
    , m_pPlayerList(0)
    , m_pHookList(NULL)
    , m_pAudioDevHookList(NULL)
    , m_pAuxiliaryAudioBuffers(0)
    , m_pMutex(NULL)
    , m_pFinalHook(NULL)
    , m_bUseFinalHook(FALSE)
    , m_pCoreMutex(NULL)
    , m_uAskFromAudioDevice(0)
    , m_bDeferActualResume(FALSE)
    , m_pLastPausedPlayer(NULL)
    , m_bUsingReplacedDevice(FALSE)
    , m_bToBeRewound(FALSE)
    , m_ulLastRewindTimestamp(0)
    , m_uNumToBePushed(0)
    , m_uNumToBePlayed(0)
    , m_bSessionBufferDirty(FALSE)
    , m_bPostMixHooksUpdated(FALSE)
    , m_pPreferences(0)
    , m_bAudioDeviceSupportsVolume(TRUE)
    , m_bOpaqueMode(FALSE)
    , m_pOpaqueData(NULL)
    , m_pMPPSupport(NULL)
    , m_ulTargetPushdown(TARGET_AUDIO_PUSHDOWN)
    , m_ulMinimumPushdown(MINIMUM_AUDIO_PUSHDOWN)
#if defined(HELIX_FEATURE_TIMELINE_WATCHER)
    , m_pTimeLineWatchers(NULL)
#endif    
{
    m_pFakeAudioCBTime                  = new Timeval;
    m_pInDataPtr                        = new HXAudioData;
    m_pOutDataPtr                       = new HXAudioData;
    m_pInDataPtr->pData                 = 0;
    m_pInDataPtr->ulAudioTime           = 0;
    m_pInDataPtr->uAudioStreamType      = STREAMING_AUDIO;
    m_pOutDataPtr->ulAudioTime          = 0;
    m_pOutDataPtr->pData                = 0;
    m_pOutDataPtr->uAudioStreamType     = STREAMING_AUDIO;

    /* Default value of Device format */
    m_DeviceFmt.uChannels       = 2;
    m_DeviceFmt.uBitsPerSample  = 16;
    m_DeviceFmt.ulSamplesPerSec = 16000;
    m_DeviceFmt.uMaxBlockSize   = 64000;
#ifdef HELIX_FEATURE_VOLUME
    m_pDeviceVolume = NULL;
#endif
};


/************************************************************************
 *  Method:
 *              CHXAudioSession::~CHXAudioSession()
 *      Purpose:
 *              Destructor. Clean up and set free.
 */
CHXAudioSession::~CHXAudioSession()
{
    Close();
}

void
CHXAudioSession::Close(void)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Close()", this);
    // Delete all player items
    if ( m_pPlayerList )
    {
        CHXAudioPlayer* p = 0;
        CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
        for (; lIter != m_pPlayerList->End(); ++lIter)
        {
            p = (CHXAudioPlayer*) (*lIter);
            if ( p )
                p->Release();
        }
        delete m_pPlayerList;
        m_pPlayerList = 0;
    }

    while (m_pHookList && m_pHookList->GetCount() > 0)
    {
        HXAudioHookInfo* pHookInfo = (HXAudioHookInfo*) m_pHookList->RemoveHead();
        pHookInfo->pHook->Release();
        delete pHookInfo;
    }

    HX_DELETE(m_pHookList);

#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    while (m_pAudioDevHookList && m_pAudioDevHookList->GetCount() > 0)
    {
        HXAudioDevHookEntry* pHookInfo = (HXAudioDevHookEntry*) m_pAudioDevHookList->RemoveHead();
        HX_RELEASE(pHookInfo->pHook);
        HX_DELETE(pHookInfo);
    }

    HX_DELETE(m_pAudioDevHookList);
#endif

    if (m_pAuxiliaryAudioBuffers)
    {
        while (m_pAuxiliaryAudioBuffers->GetCount() > 0)
        {
            HXAudioData* pAudioData =
                (HXAudioData*) m_pAuxiliaryAudioBuffers->RemoveHead();
            pAudioData->pData->Release();
            delete pAudioData;
        }

        delete m_pAuxiliaryAudioBuffers;
        m_pAuxiliaryAudioBuffers    = 0;
    }

    ResetSession();

    if( m_ulCallbackID && m_pScheduler)
    {
        m_pScheduler->Remove(m_ulCallbackID);
        m_ulCallbackID = 0;
    }

#ifdef HELIX_FEATURE_VOLUME
    if( m_pDeviceVolume )
    {
        m_pDeviceVolume->RemoveAdviseSink(this);
        HX_RELEASE(m_pDeviceVolume);
    }
#endif


    if (m_pDeviceCallback && m_pDeviceCallback->PendingID())
    {
        m_pScheduler->Remove(m_pDeviceCallback->PendingID());
    }
    HX_RELEASE(m_pDeviceCallback);

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pInterruptState);

#if defined(HELIX_FEATURE_PREFERENCES)
    if (m_pPreferences)
    {
        /* Store the last volume setting */
	IHXBuffer* pBuffer = NULL;
	if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
	{
	    pBuffer->SetSize(16);
	    SafeSprintf((char*) pBuffer->GetBuffer(),16,"%d",(int) m_uVolume); /* Flawfinder: ignore */
	    m_pPreferences->WritePref("Volume", pBuffer);
	    pBuffer->Release();
	}

#if defined(HELIX_FEATURE_MUTE_PREFERENCE)
        /* Store the last mute setting */
        IHXBuffer* pMuteBuffer = NULL;
	if (HXR_OK == CreateBufferCCF(pMuteBuffer, m_pContext))
	{
	    pMuteBuffer->SetSize(16);
	    SafeSprintf((char*) pMuteBuffer->GetBuffer(),16,"%d",(int) m_bMute);
	    m_pPreferences->WritePref("Mute", pMuteBuffer);
	    pMuteBuffer->Release();
	}
#endif /* HELIX_FEATURE_MUTE_PREFERENCE */

        HX_RELEASE(m_pPreferences);
#ifdef _UNIX
        z_pIHXPrefs = NULL;
#endif
    }
#endif /* HELIX_FEATURE_PREFERENCES */

#if defined(HELIX_FEATURE_TIMELINE_WATCHER)    
    //IHXTimelineManager
    while( m_pTimeLineWatchers && m_pTimeLineWatchers->GetCount() )
    {
        IHXTimelineWatcher* pWatcher = (IHXTimelineWatcher*)m_pTimeLineWatchers->RemoveHead();
        HX_RELEASE(pWatcher);
    }
    HX_DELETE(m_pTimeLineWatchers);
#endif
    
    HX_DELETE(m_pFakeAudioCBTime);
    HX_DELETE(m_pInDataPtr);
    HX_DELETE(m_pOutDataPtr);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pFinalHook);
    HX_RELEASE(m_pMPPSupport);
}
/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//

STDMETHODIMP CHXAudioSession::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList  qiList[] =
        {
	    { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*)this},
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXAudioDeviceResponse*)this},
            { GET_IIDHANDLE(IID_IHXAudioDeviceResponse), (IHXAudioDeviceResponse*)this  },
            { GET_IIDHANDLE(IID_IHXAudioDeviceManager),  (IHXAudioDeviceManager*)this   },
            { GET_IIDHANDLE(IID_IHXAudioDeviceManager2), (IHXAudioDeviceManager2*)this  },
            { GET_IIDHANDLE(IID_IHXAudioPushdown),       (IHXAudioPushdown*)this        },
#ifdef HELIX_FEATURE_VOLUME
            { GET_IIDHANDLE(IID_IHXVolumeAdviseSink), (IHXVolumeAdviseSink*)this        },
#endif
#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
            { GET_IIDHANDLE(IID_IHXAudioHookManager),    (IHXAudioHookManager*)this     },
#endif
#if defined(HELIX_FEATURE_RESAMPLER) && !defined (HELIX_CONFIG_FIXEDPOINT)
            { GET_IIDHANDLE(IID_IHXAudioResamplerManager), (IHXAudioResamplerManager*)this },
#endif
#if defined(HELIX_FEATURE_TIMELINE_WATCHER)            
            { GET_IIDHANDLE(IID_IHXTimelineManager),      (IHXTimelineManager*)this       },
#endif            
            { GET_IIDHANDLE(IID_IHXAudioPushdown2),      (IHXAudioPushdown2*)this       },
            { GET_IIDHANDLE(IID_IHXAudioDeviceHookManager), (IHXAudioDeviceHookManager*)this},
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioSession::AddRef()
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
STDMETHODIMP_(ULONG32) CHXAudioSession::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::SetVolume
 *      Purpose:
 *              Set device volume. This is the audio device volume.
 *
 */
void CHXAudioSession::SetVolume( const UINT16 uVolume)
{
    m_uVolume = uVolume;

    // only set the volume of not muted
    if (!m_bMute)
    {
        HX_ASSERT(m_pCurrentAudioDev || !m_bReplacedDev);
        _ConstructIfNeeded();
        if (m_pCurrentAudioDev)
        {
            m_pCurrentAudioDev->SetVolume(uVolume);
        }
    }
}

void CHXAudioSession::_ConstructIfNeeded()
{
    if (!m_pCurrentAudioDev)
    {
        CreateAudioDevice();
        if (!m_bAudioDeviceSupportsVolume)
        {
            ReleaseAudioDevice();
        }
    }
}


/************************************************************************
 *  Method:
 *              CHXAudioSession::GetVolume
 *      Purpose:
 *              Get device volume. This is the audio device volume.
 *
 */
UINT16 CHXAudioSession::GetVolume()
{
    if(!m_bMute)
    {
        HX_ASSERT(m_pCurrentAudioDev || !m_bReplacedDev);
        _ConstructIfNeeded();
        if (m_pCurrentAudioDev)
        {
            m_uVolume = m_pCurrentAudioDev->GetVolume();
        }
    }
    return m_uVolume;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::SetMute
 *      Purpose:
 *              Mute device volume. This is the audio device volume.
 *
 */
void CHXAudioSession::SetMute( const HXBOOL bMute)
{
    if (bMute != m_bMute)
    {
        m_bMute = bMute;

        UINT16 volLevel = (m_bMute ? 0 : m_uVolume);
        HX_ASSERT(m_pCurrentAudioDev || !m_bReplacedDev);
        _ConstructIfNeeded();
        if (m_pCurrentAudioDev)
        {
            m_pCurrentAudioDev->SetVolume( volLevel );
        }
    }
}




/************************************************************************
 *  Method:
 *              CHXAudioSession::Init
 *      Purpose:
 *              Setup the Audio Player list.
 */
HX_RESULT CHXAudioSession::Init(IUnknown* pContext)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Init()", this);

    HX_RESULT   theErr  = HXR_OK;
    if (!pContext)
    {
        theErr = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);

#if defined(HELIX_FEATURE_PREFERENCES)
    // Get preferences interface and info..
    pContext->QueryInterface(IID_IHXPreferences, (void **) &m_pPreferences);

#ifdef _UNIX
    //Set up a global so we can read the ESDSupport pref
    //when we create the audio device
    z_pIHXPrefs = m_pPreferences;
#endif
#endif /* HELIX_FEATURE_PREFERENCES */

    if (HXR_OK != pContext->QueryInterface(IID_IHXScheduler,
                                           (void**) &m_pScheduler))
    {
        theErr = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    pContext->QueryInterface(IID_IHXInterruptState, (void**) &m_pInterruptState);

    // Create audio player list.
    m_pPlayerList = new CHXSimpleList;
    if (!m_pPlayerList || !m_pPlayerList->IsPtrListValid())
    {
        theErr = HXR_OUTOFMEMORY;
        goto cleanup;
    }

#if defined(HELIX_FEATURE_PREFERENCES)
    if (m_pPreferences)
    {
        ReadPrefUINT32(m_pPreferences, "TargetAudioPushdown", m_ulTargetPushdown);
	ReadPrefUINT32(m_pPreferences, "MinimumAudioPushdown", m_ulMinimumPushdown);
        ReadPrefUINT32(m_pPreferences, "MinimumAudioStartupPushdown", m_ulMinimumStartupPushdown);
	ReadPrefUINT32(m_pPreferences, "MaxBlocksToPlayPerGranule", m_ulMaxBlocksToPlayPerGranule);
        ReadPrefUINT32(m_pPreferences, "CheckAudioPct", m_nPercentage);
    }    
#endif /* HELIX_FEATURE_PREFERENCES */
    
    if (m_ulMinimumPushdown > m_ulMinimumStartupPushdown)
    {
        m_ulMinimumStartupPushdown = m_ulMinimumPushdown;
    }
    if (m_ulMinimumStartupPushdown > m_ulTargetPushdown)
    {
        m_ulTargetPushdown = m_ulMinimumStartupPushdown;
    }
    if (m_ulMaxBlocksToPlayPerGranule == 0)
    {
	m_ulMaxBlocksToPlayPerGranule = 1;
    }
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Init(): min audio push = %lu; min startup push = %lu; target push = %lu", 
	    this, 
	    m_ulMinimumPushdown, 
	    m_ulMinimumStartupPushdown, 
	    m_ulTargetPushdown);
    HXLOGL2(HXLOG_ADEV, "Setting callback granulatiry percentage to %d", m_nPercentage );


    // Create a device volume interface.
    if ( !theErr )
    {
#if defined(HELIX_FEATURE_VOLUME)
#if defined(HELIX_FEATURE_PREFERENCES)
        if (m_pPreferences)
        {
            HXBOOL bOpenAudioDeviceOnPlayback = TRUE;

            ReadPrefBOOL(m_pPreferences, "OpenAudioDeviceOnPlayback", bOpenAudioDeviceOnPlayback);

            if (!bOpenAudioDeviceOnPlayback)
            {
                // rgammon 12/17/03
                // Create audio device on init if OpenAudioDeviceOnPlayback is
                // false. This means that we will be able to query the volume
                // successfully on startup. It also means that if there are
                // errors opening the audio device, they will show up at
                // startup. Some players (the windows player, for example),
                // save the system volume at shutdown and use that volume for
                // the ui on startup.
                CreateAudioDevice();

                m_uVolume = GetVolume();
            }
        }

        if (m_pPreferences)
        {
            if (ReadPrefUINT16(m_pPreferences, "Volume", m_uVolume) != HXR_OK)
            {
                HXBOOL bUseDS = TRUE;
                ReadPrefBOOL(m_pPreferences, "UseDirectSound", bUseDS);
                if(bUseDS)
                {
                    m_uVolume = HX_MAX_VOLUME;
                }
            }
#if defined(HELIX_FEATURE_MUTE_PREFERENCE )
            ReadPrefBOOL(m_pPreferences, "Mute", m_bMute);
#endif /* HELIX_FEATURE_MUTE_PREFERENCE */

        }
#endif /* HELIX_FEATURE_PREFERENCES */

        m_pDeviceVolume = (IHXVolume*)new CHXVolume;
        if( m_pDeviceVolume )
        {
            m_pDeviceVolume->AddRef();
            m_pDeviceVolume->SetVolume(m_uVolume);
#if defined(HELIX_FEATURE_MUTE_PREFERENCE )
            m_pDeviceVolume->SetMute(m_bMute);
#endif /* HELIX_FEATURE_MUTE_PREFERENCE */
            m_pDeviceVolume->AddAdviseSink((IHXVolumeAdviseSink*)this);
        }
        else
            theErr = HXR_OUTOFMEMORY;

#endif /* HELIX_FEATURE_VOLUME */
    }

  cleanup:
    return theErr;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::CreateAudioPlayer
 *      Purpose:
 *       The RMA session object calls this to create an audio player. Each
 *       audio player represents a unique time-line.
 */
HX_RESULT CHXAudioSession::CreateAudioPlayer
(
    CHXAudioPlayer**  ppAudioPlayer
    )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::CreateAudioPlayer()", this);

    m_pMutex->Lock();

    HX_RESULT theErr = HXR_OK;

    // Create a new audio player.
    *ppAudioPlayer= 0;
    *ppAudioPlayer = new CHXAudioPlayer( this );
    if ( !(*ppAudioPlayer) )
        theErr = HXR_OUTOFMEMORY;

    // Add new player to player list.
    if (!theErr)
    {
        theErr = _CreateAudioPlayer(ppAudioPlayer);
        HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::CreateAudioPlayer(): created [%p]", this, *ppAudioPlayer);
    }

    m_pMutex->Unlock();

    return theErr;
}

HX_RESULT
CHXAudioSession::_CreateAudioPlayer(CHXAudioPlayer** ppAudioPlayer)
{
    HX_RESULT   theErr = HXR_OK;

    // Add new player to player list.
    // Setup internal lists, etc.
    theErr = (*ppAudioPlayer)->InitializeStructures();

    if (!theErr)
    {
        // This one to keep it around.
        (*ppAudioPlayer)->AddRef();

        // This one to return back to the caller of CreateAudioPlayer.
        (*ppAudioPlayer)->AddRef();
    }

    // Add new player to player list.
    if ( !theErr && m_pPlayerList )
    {
        m_pPlayerList->AddTail((void*) *ppAudioPlayer);
    }

    if (theErr && *ppAudioPlayer)
    {
        /* Destructor is private. so this is the only way of destructing it */
        (*ppAudioPlayer)->AddRef();
        (*ppAudioPlayer)->Release();

        *ppAudioPlayer = 0;
    }

    return theErr;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::CreateAudioPlayer
 *      Purpose:
 *       The RMA session object calls this to create an audio player. Each
 *       audio player represents a unique time-line.
 */
HX_RESULT CHXAudioSession::CloseAudioPlayer
(
    CHXAudioPlayer*  pAudioPlayer
    )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::CloseAudioPlayer(): closing [%p]", this, pAudioPlayer);

    m_pMutex->Lock();

    if ( m_pPlayerList )
    {
        LISTPOSITION lPosition = NULL;
        lPosition = m_pPlayerList->Find(pAudioPlayer);
        if (lPosition)
        {
            m_pPlayerList->RemoveAt(lPosition);
            pAudioPlayer->Close();            
            _NotifyTimelineWatchers( TLW_CLOSE );
            pAudioPlayer->Release();
        }
    }

    m_pMutex->Unlock();
    return HXR_OK;
}


/* ***********************************************************************
 *  Method:
 *              CHXAudioSession::Setup
 *      Purpose:
 *              Create the audio buffer..
 */
HX_RESULT CHXAudioSession::Setup(HXBOOL bHasStreams)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Setup()", this);

    HX_RESULT theErr = HXR_OK;

    // A new audio player may start while an existing player is
    // already playing.
    if ( m_bInited )
        return HXR_OK;

    /* Only do all of this if there are audio streams... */
    m_bHasStreams = bHasStreams;
    if (m_bHasStreams)
    {
        // Get the audio device format
        // NOTE: The order of these function calls is important.
        // Specifically, the GetDeviceFormat() must precede any of
        // these calls because GetDeviceFormat() determines the
        // final device audio format.
        if (!theErr)
            theErr = GetDeviceFormat();

        // Create the playback buffers
        /* We still need to create playback buffers since we will use the
         * fake timeline if the presentation has atleast one media type other
         * than audio
         */
        if (!theErr || theErr == HXR_AUDIO_DRIVER)
        {
            CreatePlaybackBuffer();
        }

        // Reset the number of blocks we've written
        m_ulBlocksWritten  = 0;

        // Open the audio device only if DisableWrite is OFF
        /* This probably needs to be moved at the top so that
         * when second player comes in and we are already intialized,
         * we check for new value of disablewrite again
         */
        CheckDisableWrite();

        m_bUseFinalHook         = FALSE;

        if (!theErr && !m_bDisableWrite)
        {
            theErr = OpenDevice();
        } /* If disable wirte is ON, release the audio device */
        else if (!theErr)
        {
            m_pAudioDev = 0;
        }

        /* If the audio device is busy, check if we have
         * anything other than audio in the presentation.
         * If TRUE, play silence so that the rest of the
         * presentation can be played.
         */
        if (theErr == HXR_AUDIO_DRIVER && !m_bDisableWrite)
        {
            if (!IsAudioOnlyTrue())
            {
                m_bDisableWrite = TRUE;
                m_pAudioDev         = 0;
                theErr      = HXR_OK;
            }
        }
    }

    m_bInited = (!theErr) ? TRUE : FALSE;

    if (!theErr && m_pAudioDev && (m_pHookList || m_pAudioDevHookList))
    {
        InitHooks();
    }

    if (!theErr && m_pAudioDev && m_pFinalHook)
    {
        ProcessAudioHook(ACTION_ADD, m_pFinalHook);
    }

    if(m_pContext)
    {
        HX_RELEASE(m_pMPPSupport);
        m_pContext->QueryInterface(IID_IHXMultiPlayPauseSupport, (void**)&m_pMPPSupport);
    }

    return theErr;
}

HX_RESULT
CHXAudioSession::OpenDevice()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::OpenDevice()", this);
    HX_RESULT       theErr = HXR_OK;
    HXAudioFormat  audioFormat;

    if (m_pFinalHook)
    {
        m_bUseFinalHook = TRUE;

        if (HXR_OK != ProcessAudioHook(ACTION_CHECK, m_pFinalHook))
        {
            m_bUseFinalHook = FALSE;
        }

        if (m_bUseFinalHook)
        {
            memcpy( &audioFormat, &m_ActualDeviceFmt, sizeof(HXAudioFormat));
            theErr = m_pFinalHook->OnInit(&audioFormat);
        }
    }

    if (!theErr && m_pFinalHook && m_bUseFinalHook)
    {
        m_bUseFinalHook = TRUE;
        /* Did the hook change the data format? */
        if( 0!=memcmp(&audioFormat, &m_ActualDeviceFmt, sizeof(HXAudioFormat)))
        {
            memcpy( &m_BeforeHookDeviceFmt, &m_ActualDeviceFmt, sizeof(HXAudioFormat));
            memcpy( &m_ActualDeviceFmt, &audioFormat, sizeof(HXAudioFormat));
        }
    }
    else
    {
        m_bUseFinalHook = FALSE;
    }

    theErr = OpenAudio();

    if (theErr && m_pFinalHook && m_bUseFinalHook)
    {
        /* Looks like the audio device does not support the format
         * specified by the final Hook
         * Revert back to the original format and try again
         */
        memcpy( &m_ActualDeviceFmt, &m_BeforeHookDeviceFmt, sizeof(HXAudioFormat));
        m_bUseFinalHook         = FALSE;
        theErr = OpenAudio();
    }

    return theErr;
}


/* ***********************************************************************
 *  Method:
 *              CHXAudioSession::GetDeviceFormat
 *      Purpose:
 *              Determine the device format for this session.
 *              The session object needs to resolve the device format
 *              among multiple players.
 */
HX_RESULT CHXAudioSession::GetDeviceFormat()
{
    HX_RESULT       theErr          = HXR_OK;
    UINT16          uOrigSampleRate = 0;
    CHXAudioPlayer* p               = NULL;
    UINT16          bAutoUpsampling = FALSE;
    HXAudioFormat   audioFmt;

    // We no longer force audio device in stereo mode.  However we
    // still try to open it in 16-bit mode since all of our processing
    // is done in 16 bit.
    m_DeviceFmt.uBitsPerSample  = 16;

    //Just grab the first players stats to use as maximums.
    LISTPOSITION lp = m_pPlayerList->GetHeadPosition();
    while(lp)
    {
        p = (CHXAudioPlayer*) m_pPlayerList->GetNext(lp);
        if(0!=p->GetStreamCount())
        {
            p->GetFormat( &audioFmt );
            m_DeviceFmt.uChannels       = audioFmt.uChannels;
            m_DeviceFmt.ulSamplesPerSec = audioFmt.ulSamplesPerSec;
            m_DeviceFmt.uMaxBlockSize   = audioFmt.uMaxBlockSize;
            break;
        }
    }

    //Now loop through the rest of the players and find all the
    //maximums.
    while(lp)
    {
        p = (CHXAudioPlayer*) m_pPlayerList->GetNext(lp);
        if( 0 != p->GetStreamCount() )
        {
            p->GetFormat( &audioFmt );
            m_DeviceFmt.uChannels       = HX_MAX(m_DeviceFmt.uChannels,
                                                 audioFmt.uChannels);
            m_DeviceFmt.ulSamplesPerSec = HX_MAX(m_DeviceFmt.ulSamplesPerSec,
                                                 audioFmt.ulSamplesPerSec);
            m_DeviceFmt.uMaxBlockSize   = HX_MAX(m_DeviceFmt.uMaxBlockSize,
                                                 audioFmt.uMaxBlockSize);
        }
    }

    // turn on the default upsampling to 44K only on IX86 platforms
    // which support MMX
    // XXXgfw this also uses more memory for each block we push down
    // XXXgfw to the audio device. You might not want it on MIN_HEAP
    // XXXgfw kind of MMX devices, if there are any.
#if defined(_M_IX86)
    ReadPrefUINT16(m_pPreferences, "AutoAudioUpsampling", bAutoUpsampling );
#endif
    if(bAutoUpsampling)
    {
        //force 44khz as our first attempt to open audio device
        uOrigSampleRate = (UINT16)m_DeviceFmt.ulSamplesPerSec;
        m_DeviceFmt.ulSamplesPerSec = 44100;
    }
#if defined(HELIX_FEATURE_PREFERENCES)
    else
    {
        UINT16 ulSamplesPerSecPreference = 0;
        ReadPrefUINT16(m_pPreferences, "AudioDeviceSamplesPerSec", ulSamplesPerSecPreference );
        if(ulSamplesPerSecPreference)
        {
            uOrigSampleRate = (UINT16)m_DeviceFmt.ulSamplesPerSec;
            m_DeviceFmt.ulSamplesPerSec = ulSamplesPerSecPreference;
        }
    }
#endif /* HELIX_FEATURE_PREFERENCES */

    //Start negotiating with the device. Generate a table that will
    //drive the different formats we want to try.  Start with the
    //native rate (and any mods from the pref stuff above) and go up
    //in samplerate. If that fails, go down in sample rate.
    const int nNumberOfRates = sizeof(z_anValidSampleRates)/sizeof(z_anValidSampleRates[0]);
    //Total number of table entries per sample rate will be:
    //nNumberOfRates*4 (mono/16, mono/8, stereo/16, stereo/8) plus
    //native format + user defined samplerate entry.
    const int      nTmp       = nNumberOfRates*4+2;
    unsigned short nTableSize = 0;
    tableEntry* table = new tableEntry[nTmp];

    HX_ASSERT(table);
    if( NULL == table )
        return HXR_OUTOFMEMORY;


    //First entry is always our native format from above with any
    //samplerate changes the user made via prefs.
    table[nTableSize++].set( (UINT16)m_DeviceFmt.ulSamplesPerSec,
                             (UINT8)m_DeviceFmt.uChannels,
                             (UINT8)m_DeviceFmt.uBitsPerSample
                             );

    //Second entry is always the above with the clip's original
    //sample rate, if we changed it.
    if( uOrigSampleRate )
        table[nTableSize++].set(uOrigSampleRate,
                                (UINT8)m_DeviceFmt.uChannels,
                                (UINT8)m_DeviceFmt.uBitsPerSample
                                );

    //Now generate the rest of the format table....
    const UINT8 usNativeChannel = (UINT8)m_DeviceFmt.uChannels;
    const UINT8 usAltChannel    = (2==m_DeviceFmt.uChannels)?1:2;
    const UINT8 usNativeBits    = (UINT8)m_DeviceFmt.uBitsPerSample;
    const UINT8 usAltBits       = (8==m_DeviceFmt.uBitsPerSample)?16:8;

    //First use all equal or higher sample rates.
    short nIdx = 0;
    while( nIdx<nNumberOfRates )
    {
        UINT16 usRate = z_anValidSampleRates[nIdx];
        if( usRate >= m_DeviceFmt.ulSamplesPerSec )
        {
            table[nTableSize++].set(usRate, usNativeChannel, usNativeBits );
            table[nTableSize++].set(usRate, usAltChannel,    usNativeBits );
            table[nTableSize++].set(usRate, usNativeChannel, usAltBits  );
            table[nTableSize++].set(usRate, usAltChannel,    usAltBits  );
        }
        nIdx++;
    }
    //Now all the samle rates lower then the native rate.
    nIdx = nNumberOfRates-1;
    while( nIdx>=0)
    {
        UINT16 usRate = z_anValidSampleRates[nIdx];
        if( usRate < m_DeviceFmt.ulSamplesPerSec )
        {
            table[nTableSize++].set(usRate, usNativeChannel, usNativeBits );
            table[nTableSize++].set(usRate, usAltChannel,    usNativeBits );
            table[nTableSize++].set(usRate, usNativeChannel, usAltBits  );
            table[nTableSize++].set(usRate, usAltChannel,    usAltBits  );
        }
        nIdx--;
    }

    //Now loop through our table and find a supported format.
    nIdx = 0;
    theErr = HXR_FAIL;
    while( FAILED(theErr) && nIdx<nTableSize)
    {
        m_DeviceFmt.ulSamplesPerSec = table[nIdx].usSampleRate;
        m_DeviceFmt.uChannels       = table[nIdx].usChannels;
        m_DeviceFmt.uBitsPerSample  = table[nIdx].usBits;
        theErr = CheckAudioFormat(&m_DeviceFmt);
        nIdx++;
    }

    //We still need to create playback buffers since we will use the
    //fake timeline if the presentation has atleast one media type
    //other than audio
    //XXXgfw this code below needs to be looked at. I Don't want to
    //touch it now for fear of breaking something that will take
    //a long time to fix.
    if (!theErr || theErr == HXR_AUDIO_DRIVER)
    {
        m_ActualDeviceFmt = m_DeviceFmt;

        //All the calculations are done for 16 bit stereo. There are
        //VERY FEW sound cards out there which do not support 16 bit
        //stereo. They will incur a high performace hit because of
        //possible unnecessary up/down conversion. For now we will
        //live with that.
        //XXXgfw wrong. lots of handhelds/phones don't do 16bit-2
        //channel output.
        m_DeviceFmt.uBitsPerSample  = 16;

        m_BeforeHookDeviceFmt = m_ActualDeviceFmt;
    }

    HX_VECTOR_DELETE(table);
    return theErr;
}


/************************************************************************
 *  Method:
 *              CHXAudioSession::CheckAudioFormat
 *      Purpose:
 *       The audio player calls this to check its audio format with the
 *       the audio device.
 */
HX_RESULT CHXAudioSession::CheckAudioFormat
(
    HXAudioFormat*    pAudioFormat
    )
{
    HX_RESULT theErr = HXR_OK;

    if (!m_pAudioDev)
    {
        CreateAudioDevice();
        m_pAudioDev = m_pCurrentAudioDev;
    }

    if (m_pAudioDev)
    {
        theErr = m_pAudioDev->CheckFormat(pAudioFormat);
        /* Any error from audio device other than memory error is
         * returned as HXR_AUDIO_DRIVER
         */
        if (theErr != HXR_OK && theErr != HXR_OUTOFMEMORY)
        {
            theErr = HXR_AUDIO_DRIVER;
        }
    }

    return theErr;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::PlayAudio
 *      Purpose:
 *       The player object call this to play audio. This method is called
 *       again in the playback response function.
 */
HX_RESULT CHXAudioSession::PlayAudio(UINT16 uNumBlocks)
{
    HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::PlayAudio(): %u blocks", this, uNumBlocks);

    HX_RESULT theErr        = HXR_OK;
    HXBOOL      bDisableWrite = FALSE;

    if ( !m_bInited )
        return theErr;

    if (m_bInPlayAudio)
    {
        return HXR_OK;
    }

    m_pMutex->Lock();

    if (m_bToBeRewound)
    {
        theErr = Rewind();
        goto exit;
    }

    m_bInPlayAudio = TRUE;

    if (m_pAuxiliaryAudioBuffers &&
        m_pAuxiliaryAudioBuffers->GetCount() > 0 && m_pAudioDev && !m_bDisableWrite)
    {
        if (HXR_OK == ProcessAudioDevice(ACTION_CHECK, m_pAudioDev))
        {
            /* Try to stuff in as much backlog as possible */
            while (!theErr && m_pAuxiliaryAudioBuffers->GetCount() > 0)
            {
                HXAudioData* pAudioData =
                    (HXAudioData*) m_pAuxiliaryAudioBuffers->GetHead();

                HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::PlayAudio(): writing block [%p]", this, pAudioData);

                // Write session audio data to device.
                theErr = m_pAudioDev->Write(pAudioData);
                if( theErr == HXR_OUTOFMEMORY )
                {
                    goto exit;
                }

                if (!theErr)
                {
                    m_ulBlocksWritten++;
                    m_dNumBytesWritten  += pAudioData->pData->GetSize();

                    m_pAuxiliaryAudioBuffers->RemoveHead();
                    pAudioData->pData->Release();
                    delete pAudioData;
                }
                /*All other error codes are translated into audio driver error*/
                else if (theErr != HXR_OUTOFMEMORY && theErr != HXR_WOULD_BLOCK)
                {
                    theErr = HXR_AUDIO_DRIVER;
                }
            }
        }
    }

    // If we have audio streams then play audio.
    if (!theErr && m_bHasStreams)
    {
        UINT16 uPush = uNumBlocks;
        if (m_bFirstPlayAudio)
        {
            uPush = (UINT16) m_ulMinBlocksTobeQueuedAtStart;
            HXLOGL3(HXLOG_ADEV,
                    "CHXAudioSession[%p]::PlayAudio(): first play (using %u blocks)",
                    this,
                    uPush);
           
        }
        HXLOGL4(HXLOG_ADEV,
                "CHXAudioSession[%p]::PlayAudio(): pushing %u blocks",
                this,
                uPush);

        HXAudioData     audioData;
        CHXAudioPlayer* pPlayer       = 0;
        CHXAudioStream* pStream       = 0;
        CHXSimpleList*  pStreamList   = 0;
        UINT16          uPlayerVolume = 0;
        UCHAR*          pMixBuffer    = 0;
        UCHAR*          pPlayerBuf    = NULL;
        UCHAR*          pSessionBuf   = NULL;
        IHXBuffer*      pMixIHXBuffer = NULL;
        BufType         bufType       = BUFFER_NONE;

#if defined(HELIX_FEATURE_VOLUME) || defined(HELIX_FEATURE_MIXER)
        uPlayerVolume = HX_INIT_VOLUME;
#endif

        for (UINT16 i = 0; !theErr && i < uPush; i++ )
        {
            m_uNumToBePushed      = uPush - i;
            m_bSessionBufferDirty = FALSE; // only used for multi-player case

            UINT32  ulNumBytesWritten       = m_ulBytesPerGran;
            HXBOOL  bAtLeastOnePlayerActive = FALSE;

            theErr = m_pSessionBuf->SetSize(m_ulBytesPerGran);
            if( theErr == HXR_OUTOFMEMORY )
            {
                theErr = HXR_OUTOFMEMORY;
                goto exit;
            }

            pSessionBuf = m_pSessionBuf->GetBuffer();

            // Zero session buffer.
            //memset(pSessionBuf, 0, HX_SAFESIZE_T(m_ulBytesPerGran));

            // Get each player
            pPlayer = 0;
            CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
            for (; lIter != m_pPlayerList->End(); ++lIter)
            {
                HXBOOL bStalled = FALSE;
                pPlayer = (CHXAudioPlayer*) (*lIter);
                if (pPlayer->GetStreamCount() == 0 ||
                    pPlayer->IsDonePlayback() ||
                    pPlayer->GetState() != E_PLAYING)
                {
                    HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::PlayAudio(): skipping player [%p] (done or not playing)", this, pPlayer);
                    continue;
                }

                bAtLeastOnePlayerActive = TRUE;

                if (m_pPlayerList->GetCount() == 1)
                {
                    pMixIHXBuffer = m_pSessionBuf;
                    bufType = BUFFER_SESSION;
                }
                else
                {
                    if (!m_pPlayerBuf)
                    {
			theErr = CreateBufferCCF(m_pPlayerBuf, m_pContext);
			if (HXR_OK != theErr)
			{
			    goto exit;
			}
                    }

                    m_pPlayerBuf->SetSize(m_ulBytesPerGran);

                    pPlayerBuf = m_pPlayerBuf->GetBuffer();
                    // Zero play buffer.
//                  memset(pPlayerBuf, 0, HX_SAFESIZE_T(m_ulBytesPerGran));
                    pMixIHXBuffer = m_pPlayerBuf;
                    bufType = BUFFER_PLAYER;
                }

                pMixBuffer = pMixIHXBuffer->GetBuffer();
                HXBOOL bIsMixBufferDirty = FALSE;
                HXBOOL bMayNeedToRollbackTimestamp = FALSE;

                if (pPlayer->GetState() == E_PLAYING)
                {
                    UINT32  ulBufTime = 0;
                    // Get each stream associated with this player
                    pStreamList = pPlayer->GetStreamList();
                    if (pStreamList)
                    {
                        CHXSimpleList::Iterator lIter = pStreamList->Begin();
                        for (; lIter != pStreamList->End(); ++lIter)
                        {
                            pStream = (CHXAudioStream*) (*lIter);
                            pStream->m_bMayNeedToRollbackTimestamp = FALSE;

                            // don't mix non-playing audio streams
                            if (pStream->GetState() == E_PLAYING)
                            {
                                theErr = pStream->MixIntoBuffer( pMixBuffer,
                                                                 m_ulBytesPerGran,
                                                                 ulBufTime,
                                                                 bIsMixBufferDirty);

                                if (HXR_OK !=theErr)
                                {
                                    HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::PlayAudio(): after mix: err = 0x%08x", this, theErr);
                                }

                                if (theErr == HXR_NO_DATA)
                                {
                                    pStream->m_bMayNeedToRollbackTimestamp = TRUE;
                                    bMayNeedToRollbackTimestamp = TRUE;
                                    theErr = HXR_OK;
                                    continue;
                                }

                                if (theErr == HXR_FAIL)
                                {
                                    theErr = HXR_OK;
                                    goto exit;
                                }

                                if (theErr == HXR_OUTOFMEMORY)
                                {
                                    goto exit;
                                }

                                if (theErr == HXR_WOULD_BLOCK)
                                {
                                    if (bMayNeedToRollbackTimestamp)
                                    { 
                                        bMayNeedToRollbackTimestamp = FALSE;
                                        lIter = pStreamList->Begin();
                                        for (; lIter != pStreamList->End(); ++lIter)
                                        {
                                            pStream = (CHXAudioStream*) (*lIter);
                                            if (pStream->m_bMayNeedToRollbackTimestamp)
                                            {
                                                pStream->m_bMayNeedToRollbackTimestamp = FALSE;
                                                pStream->RollBackTimestamp();
                                            }
                                        }
                                    }

                                    goto handlewouldblock;
                                }
                            }

                            if (pPlayer->GetState() != E_PLAYING)
                            {
                                /* We should keep the last buffer around and the last stream
                                 * to be mixed again...TBD XXX Rahul
                                 */
                                //pPlayer->SetLastMixedBuffer(pMixBuffer, pStream);
                                bStalled = TRUE;
                                break;
                            }
                        }
                    } //if (pStreamList)

                    /* This pause may have happended due to ondrynotification */
                    if (m_bPaused || m_bStoppedDuringPause)
                    {
                        /* This would happen ONLY IF THERE IS ONE AUDIO PLAYER
                         * and that has been paused. So simply break
                         */
                        goto exit;
                        //break;
                    }

                    /* hmmm... looks like there are more than one audio player.
                     * continue with the next one
                     */
                    if (bStalled)
                    {
                        continue;
                    }

                    /* If the mixer buffer was not used, make sure it is initialized
                     * to silence since we pass it to post process hooks
                     */
                    if (!bIsMixBufferDirty)
                    {
                        ::memset(pMixBuffer, 0, HX_SAFESIZE_T(m_ulBytesPerGran));
                    }

#if defined(HELIX_FEATURE_VOLUME) && defined(HELIX_FEATURE_MIXER)
                    // Apply Player volume to buffer; do this before we call
                    // the post mix hooks.
                    uPlayerVolume = pPlayer->GetVolume();

                    if (uPlayerVolume != 100 && bIsMixBufferDirty)
                    {
                        CHXMixer::ApplyVolume( pMixBuffer,
                                               m_ulBytesPerGran,
                                               uPlayerVolume,
                                               m_DeviceFmt.uBitsPerSample);
                    }
#endif /* HELIX_FEATURE_VOLUME && HELIX_FEATURE_MIXER */

                    // Give data to this AudioPlayer's post mix hooks
                    // (do they want 8 or 16 bit?)
                    HXBOOL bChanged = FALSE;
                    ProcessPostMixHooks( pPlayer,
                                         pMixIHXBuffer,
                                         &bDisableWrite,
                                         ulBufTime,
                                         bChanged);
                    /*
                     * If the mixer buffer changed (because of a post mix hook)
                     * make sure to point the player/session buffer to this
                     * modified buffer
                     */
                    if (bChanged)
                    {
                        pMixBuffer = pMixIHXBuffer->GetBuffer();

                        if (bufType == BUFFER_PLAYER)
                        {
                            m_pPlayerBuf = pMixIHXBuffer;
                            pPlayerBuf = pMixIHXBuffer->GetBuffer();
                        }
                        else 
                        {
                            m_pSessionBuf = pMixIHXBuffer;
                            pSessionBuf = pMixIHXBuffer->GetBuffer();
                        }
                    }
                }

#if defined(HELIX_FEATURE_MIXER)
                // Don't mix if volume is 0.
                // Don't mix if this player has disabled device write.
                // Don't mix if there is only one player since we would have
                // written data into session buf instead of player buffer
                if (m_pPlayerList->GetCount() > 1 && uPlayerVolume > 0 && !bDisableWrite)
                {
                    /* We always set a volume of 100 since we have already applied volume
                     * to player buffer
                     */
                    CHXMixer::MixBuffer( pPlayerBuf, pSessionBuf,
                                         m_ulBytesPerGran, FALSE, 100, m_DeviceFmt.uBitsPerSample, m_bSessionBufferDirty);
                }
#endif /* HELIX_FEATURE_MIXER */
            }

            if (!bAtLeastOnePlayerActive)
            {
                goto exit;
            }

            /* did we ever write to the session buffer ? */
            if (m_pPlayerList->GetCount() > 1 && !m_bSessionBufferDirty)
            {
                HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::PlayAudio(): silence in session buffer", this);
                ::memset(pSessionBuf, 0, HX_SAFESIZE_T(m_ulBytesPerGran));
            }

            // This increments with each buffer played.
            m_dBufEndTime += m_dGranularity;

            // Set the session buffer to the IHXBuffer.
            audioData.pData       = m_pSessionBuf;
            audioData.ulAudioTime = (ULONG32)m_dBufEndTime;

            if (m_pAudioDev && !m_bDisableWrite)
            {
                /* are we dealing with a messed up sound card */
                if ((m_BeforeHookDeviceFmt.uChannels == 1 && m_DeviceFmt.uChannels == 2)||
                    m_BeforeHookDeviceFmt.uBitsPerSample == 8)
                {
                    ConvertTo8BitAndOrMono(&audioData);
                }

                if (m_pFinalHook && m_bUseFinalHook)
                {
                    if (HXR_OK == ProcessAudioHook(ACTION_CHECK, m_pFinalHook))
                    {
                        m_pOutDataPtr->pData            = NULL;
                        m_pOutDataPtr->ulAudioTime      = audioData.ulAudioTime;
                        m_pOutDataPtr->uAudioStreamType = audioData.uAudioStreamType;

                        m_pFinalHook->OnBuffer(&audioData, m_pOutDataPtr);

                        HX_ASSERT(m_pOutDataPtr->pData);
                        if (m_pOutDataPtr->pData)
                        {
                            HX_RELEASE(audioData.pData);
                            m_pSessionBuf = audioData.pData = m_pOutDataPtr->pData;
                        }
                        else
                        {
                            /* This is a screwed up Hook. Disable it */
                            m_bUseFinalHook = FALSE;
                        }
                    }
                }

                if (m_pHookList && !m_bOpaqueMode)
                {
                    ProcessHooks(&audioData);
                }

                ulNumBytesWritten = audioData.pData->GetSize();
                if (HXR_OK == ProcessAudioDevice(ACTION_CHECK, m_pAudioDev))
                {
                    HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::PlayAudio(): writing [%p] %lu bytes", this, &audioData, ulNumBytesWritten);

                    // Write session audio data to device.
                    theErr = m_pAudioDev->Write(&audioData);
                    if( theErr == HXR_OUTOFMEMORY )
                    {
                        goto exit;
                    }
                }

                if (theErr != HXR_OK)
                {
                    HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::PlayAudio(): after write: err = 0x%08x", this, theErr);
                }

                if (theErr == HXR_WOULD_BLOCK)
                {
                    HXAudioData* pAudioData     = new HXAudioData;
                    pAudioData->pData           = audioData.pData;
                    pAudioData->pData->AddRef();
                    pAudioData->ulAudioTime     = audioData.ulAudioTime;
                    pAudioData->uAudioStreamType= audioData.uAudioStreamType;

                    // Create auxiliary buffer list, if one is not already created
                    if (!m_pAuxiliaryAudioBuffers)
                    {
                        m_pAuxiliaryAudioBuffers = new CHXSimpleList;
                    }
                    if( NULL == m_pAuxiliaryAudioBuffers->AddTail(pAudioData) )
                    {
                        theErr = HXR_OUTOFMEMORY;
                        goto exit;
                    }

                    HX_RELEASE(m_pSessionBuf);
                }
                /* Any error from audio device other than memory error is
                 * returned as HXR_AUDIO_DRIVER
                 */
                if (theErr != HXR_OK && theErr != HXR_WOULD_BLOCK &&
                    theErr != HXR_OUTOFMEMORY)
                {
                    theErr = HXR_AUDIO_DRIVER;
                }
            }

            if (!theErr)
            {
                m_ulBlocksWritten++;
                m_dNumBytesWritten  += ulNumBytesWritten;
            }

            // So this function is good in theory, but in practice we find in
            // heap-optimized mode it is not necessary, and it leads to
            // unnecessary heap fragmentation.
#if !defined(HELIX_CONFIG_MIN_HEAP_FRAG)
            theErr = CheckForBufferReuse();
#endif
        }  // for loop
    }     //  end if we have audio streams

  handlewouldblock:
    // mask this error
    if (theErr == HXR_WOULD_BLOCK)
    {
        theErr = HXR_OK;
    }

    // If we do not have audio.. OR  if we do have audio but
    // we have disabled writing to the device, then we need to
    // fake the timeline.
    if (!theErr && !m_ulCallbackID && (!m_bHasStreams || m_bDisableWrite))
    {
        if (m_bFirstPlayAudio)
        {
            // First time thru, we initialize callback time.
            HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
            m_pFakeAudioCBTime->tv_sec = lTime.tv_sec;
            m_pFakeAudioCBTime->tv_usec = lTime.tv_usec;

            m_ulIncreasingTimer = 0;
            m_ulLastFakeCallbackTime    = HX_GET_TICKCOUNT();

            *m_pFakeAudioCBTime += (int) (m_ulGranularity*1000);
        }
        m_bFakeAudioTimeline =  TRUE;
        m_ulCallbackID = m_pScheduler->RelativeEnter( this, m_ulGranularity);
        if (m_bFirstPlayAudio)
        {
            OnTimeSync(m_ulIncreasingTimer);
        }
    }
    else if (!theErr && !m_ulCallbackID && m_bHasStreams && !m_bDisableWrite)
    {
        m_bFakeAudioTimeline =  FALSE;
        m_ulCallbackID = m_pScheduler->RelativeEnter(this, m_ulGranularity*m_nPercentage/100);
    }

  exit:
    m_bInPlayAudio = FALSE;

    // can only happen in multi-player pause/resume/stop case
    if (m_bDeferActualResume && theErr != HXR_OUTOFMEMORY)
    {
        m_bDeferActualResume = FALSE;
        theErr = ActualResume();
    }

    m_bFirstPlayAudio = FALSE;

    m_pMutex->Unlock();

    return theErr;
}

HX_RESULT
CHXAudioSession::CheckForBufferReuse()
{
    if (m_pSessionBuf)
    {
        m_pSessionBuf->AddRef();
        if (m_pSessionBuf->Release() > 1)
        {
            /*
             * Cannot use this buffer the next time
             * release our reference and  create a new one
             */
            HX_RELEASE(m_pSessionBuf);

	    CreateBufferCCF(m_pSessionBuf, m_pContext);
            if (!m_pSessionBuf )
            {
                return HXR_OUTOFMEMORY;
            }
        }
    }
    else
    {
        /* create this buffer */
	CreateBufferCCF(m_pSessionBuf, m_pContext);
        if (!m_pSessionBuf )
        {
            return HXR_OUTOFMEMORY;
        }
    }

    if (m_pPlayerBuf)
    {
        m_pPlayerBuf->AddRef();
        if (m_pPlayerBuf->Release() > 1)
        {
            /*
             * Cannot use this buffer the next time
             * release our reference and  create a new one
             */
            m_pPlayerBuf->Release();

	    CreateBufferCCF(m_pPlayerBuf, m_pContext);
            if (!m_pPlayerBuf )
            {
                return HXR_OUTOFMEMORY;
            }
        }
    }
    return HXR_OK;
}

void CHXAudioSession::ConvertTo8BitAndOrMono(HXAudioData* pAudioData)
{
    HXBOOL bChannelConversion = (m_BeforeHookDeviceFmt.uChannels == 1) &&
        (m_DeviceFmt.uChannels == 2);
    HXBOOL bBitConversion     = m_BeforeHookDeviceFmt.uBitsPerSample == 8;

    /* Atleast one of them should be true to be in this function */
    HX_ASSERT(bChannelConversion || bBitConversion);

    ULONG32     ulLen           = pAudioData->pData->GetSize();
    short int*  pShortBuf       = (short int*) pAudioData->pData->GetBuffer();
    UCHAR*      pOutUCharBuf    = pAudioData->pData->GetBuffer();
    short int*  pOutShortBuf    = (short int*) pAudioData->pData->GetBuffer();
    ULONG32     ulLoopCount     = ulLen;

    if (bBitConversion && bChannelConversion)
    {
        ulLen       =  pAudioData->pData->GetSize() / 4;
        ulLoopCount = pAudioData->pData->GetSize() / 4;
    }
    else if (bBitConversion && !bChannelConversion)
    {
        ulLen       = pAudioData->pData->GetSize() / 2;
        ulLoopCount = pAudioData->pData->GetSize() / 2;
    }
    else /*if (!bBitConversion && bChannelConversion) */
    {
        ulLen       = pAudioData->pData->GetSize() / 2;
        ulLoopCount = pAudioData->pData->GetSize() / 4;
    }

    for(ULONG32 j = 0; j < ulLoopCount; j++)
    {
        if (bBitConversion && bChannelConversion)
        {
            *pOutUCharBuf++ = (UCHAR)   (
                ((LONG32) ((*pShortBuf++  + 32768L) >> 8) +
                 (LONG32) ((*pShortBuf++  + 32768L) >> 8))/2
                );
        }
        else if (bBitConversion && !bChannelConversion)
        {
            *pOutUCharBuf++ = (UCHAR)((*pShortBuf++  + 32768L) >> 8);
        }
        else /*if (!bBitConversion && bChannelConversion) */
        {
            *pOutShortBuf++ = (short int) (((LONG32) *pShortBuf++  + (LONG32) *pShortBuf++)/2);
        }
    }

    pAudioData->pData->SetSize(ulLen);
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::ConvertToEight
 *      Purpose:
 */
void CHXAudioSession::ConvertToEight()
{
    UCHAR* pSessionBuf = m_pSessionBuf->GetBuffer();
    ULONG32 ulLen = m_ulBytesPerGran / sizeof(short);
    short int* iTmp = (short int*) pSessionBuf;

    for(ULONG32 j = 0; j < ulLen; j++ )
    {
        pSessionBuf[j] = (UCHAR) ((*iTmp++  + 32768L) >> 8);
    }
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::TryOpenAudio
 *      Purpose:
 *       Try to open the audio device.
 */
HX_RESULT CHXAudioSession::TryOpenAudio()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::TryOpenAudio()", this);

    HX_RESULT theErr = HXR_OK;
    HXBOOL bDeviceOpened = FALSE;

    // Free the audio device if it is ours so that we can create it on the core thread!
    if ((!m_bReplacedDev || !m_bUsingReplacedDevice) && m_pCurrentAudioDev)
    {

        ReleaseAudioDevice();
        RestoreReplacedDevice();
        m_pAudioDev = NULL;
    }

    theErr = CreateAudioDevice();

    if (!theErr && m_pCurrentAudioDev)
    {
        m_pAudioDev = m_pCurrentAudioDev;

        if (!m_bReplacedDev || !m_bUsingReplacedDevice)
        {
            ((CHXAudioDevice*)m_pAudioDev)->SetGranularity( m_ulGranularity, m_ulBytesPerGran);
        }
        if (m_bOpaqueMode)
        {
            IHXOpaqueAudioDevice* pOpaqueDevice = NULL;
            m_pAudioDev->QueryInterface (IID_IHXOpaqueAudioDevice, (void **)&pOpaqueDevice);
            if (!pOpaqueDevice)
                return HXR_FAIL;
            theErr = pOpaqueDevice->Open(&m_ActualDeviceFmt, this, m_strOpaqueType, m_pOpaqueData );
            HX_RELEASE(pOpaqueDevice);
        }
        else
            theErr = m_pAudioDev->Open( &m_ActualDeviceFmt, this );
        bDeviceOpened = TRUE;
        m_ulBlocksWritten = 0;

        /* we always open in a PAUSED state */
        if (!theErr)
        {
            HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::TryOpenAudio(): open succeeded; pausing audio device (default state)", this);
            theErr = m_pAudioDev->Pause();
            _NotifyTimelineWatchers(TLW_PAUSE);        
        }

        if (theErr != HXR_OK)
        {
            m_pAudioDev = NULL;
        }

        if (!theErr && m_pAudioDev)
        {
            /* Set the initial device volume */
            m_pAudioDev->SetVolume(m_bMute ? 0 : m_uVolume);
        }
    }

    /* Any error from audio device other than memory error is returned
     * as HXR_AUDIO_DRIVER
     */
    if (theErr != HXR_OK && theErr != HXR_OUTOFMEMORY)
    {
        theErr = HXR_AUDIO_DRIVER;
    }
#if defined(HELIX_CONFIG_AUDIO_ON_CORE_THREAD)
#ifndef _CARBON
    if (!theErr && bDeviceOpened && m_bShouldOpenOnCoreThread &&
        m_pInterruptState && !m_pInterruptState->AtInterruptTime())
    {
        HXLOGL3(HXLOG_CORP,
                "CHXAudioSession[%p]::TryOpenAudio() Wrong Thread: Bailing out",
                this);

        m_bToBeReOpened = TRUE;
        if( !m_pDeviceCallback )
        {
            m_pDeviceCallback = new HXDeviceSetupCallback(this);
            m_pDeviceCallback->AddRef();
        }
        if (!m_pDeviceCallback->PendingID())
        {
            m_pDeviceCallback->PendingID( m_pScheduler->RelativeEnter(m_pDeviceCallback, 0));
        }

    }
    else
#endif //!_CARBON
#endif //HELIX_CONFIG_AUDIO_ON_CORE_THREAD        
    {
        m_bToBeReOpened = FALSE;
    }

    return theErr;
}

HX_RESULT
CHXAudioSession::CreateAudioDevice()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::CreateAudioDevice()", this);

    HX_RESULT theErr = HXR_OK;

    if (!m_pCurrentAudioDev)
    {
        // create the audioout object
        CHXAudioDevice* pAudioDev = CHXAudioDevice::Create(m_pPreferences);
        if (pAudioDev)
        {
            pAudioDev->AddRef();
            pAudioDev->Init(m_pContext);
            if (pAudioDev->InitVolume(0, 100) == TRUE)
            {
                m_bAudioDeviceSupportsVolume = TRUE;
            }
            else
            {
                m_bAudioDeviceSupportsVolume = FALSE;
            }

            m_pCurrentAudioDev = (IHXAudioDevice*) pAudioDev;

            HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::CreateAudioDevice(): created [%p]", this, m_pCurrentAudioDev);
        }
        else
        {
            theErr = HXR_OUTOFMEMORY;
        }
    }

    return theErr;
}

void
CHXAudioSession::ReleaseAudioDevice()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::ReleaseAudioDevice()", this);
    if (m_pCurrentAudioDev)
    {
        ProcessAudioDevice(ACTION_REMOVE, m_pCurrentAudioDev);
    }

    m_pCurrentAudioDev->Close(FALSE);
    _NotifyTimelineWatchers( TLW_CLOSE );
    HX_RELEASE(m_pCurrentAudioDev);

    m_bToBeReOpened = FALSE;
    if (m_pDeviceCallback && m_pDeviceCallback->PendingID())
    {
        m_pScheduler->Remove(m_pDeviceCallback->PendingID());
        m_pDeviceCallback->PendingID(0);
    }
}

void
CHXAudioSession::RestoreReplacedDevice()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::RestoreReplacedDevice()", this);
    if (m_bReplacedDev && !m_bUsingReplacedDevice && !m_pCurrentAudioDev)
    {
        m_pCurrentAudioDev = m_pReplacedAudioDev;
        m_pCurrentAudioDev->AddRef();
        m_bUsingReplacedDevice = TRUE;
    }
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::OpenAudio
 *      Purpose:
 *       Open the audio device.
 */
HX_RESULT CHXAudioSession::OpenAudio()
{
    HX_RESULT theErr = TryOpenAudio();

    if (theErr == HXR_AUDIO_DRIVER)
    {
        StopAllOtherPlayers();
        theErr = TryOpenAudio();
    }

    return theErr;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::CreatePlaybackBuffer
 *      Purpose:
 *              This is the buffer that we mix all player buffers into
 *              and the one we write to the audio device.
 */
HX_RESULT CHXAudioSession::CreatePlaybackBuffer()
{
    // Calculate the number of bytes per granularity.
    m_ulBytesPerGran = (ULONG32)
        (((m_DeviceFmt.uChannels * ((m_DeviceFmt.uBitsPerSample==8)?1:2) *  m_DeviceFmt.ulSamplesPerSec)
          / 1000.0) * m_ulGranularity);

    /* Number of samples required at output should be a multiple of 8 if
     * sampling rate is 8K/16K/32K...or a multiple of 11 for 11K/22K...
     * This is needed since the resamplerequires works reliably ONLY if
     * this condition is true. Ken is working on this problem. This is
     * an interim fix
     */
    ULONG32 ulExtraGranularity = 1;
    if (m_DeviceFmt.ulSamplesPerSec % 8 == 0)
    {
        ulExtraGranularity = 8;
    }
    else
    {
        ulExtraGranularity = 11;
    }

    if (m_ulBytesPerGran % (2*m_DeviceFmt.uChannels*ulExtraGranularity) != 0)
    {
        m_ulBytesPerGran -= m_ulBytesPerGran % (2*m_DeviceFmt.uChannels*ulExtraGranularity);

        m_dGranularity =
            (double) m_ulBytesPerGran / (double)((double)(m_DeviceFmt.uChannels *
 ((m_DeviceFmt.uBitsPerSample==8)?1:2) * m_DeviceFmt.ulSamplesPerSec)
                                                 / 1000.0);
    }
 
    // Readjust the max size of the block
    m_ActualDeviceFmt.uMaxBlockSize = (UINT16) m_ulBytesPerGran;

    HX_RELEASE(m_pSessionBuf);
    HX_RELEASE(m_pPlayerBuf);

    if (HXR_OK == CreateBufferCCF(m_pSessionBuf, m_pContext))
    {
	m_pSessionBuf->SetSize(m_ulBytesPerGran);
    }

    m_DeviceFmt.uMaxBlockSize   = (UINT16) m_ulBytesPerGran;
    m_ActualDeviceFmt.uMaxBlockSize = m_DeviceFmt.uMaxBlockSize;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::Pause
 *      Purpose:
 *       Pause playback of this Player's audio.
 */
HX_RESULT CHXAudioSession::Pause( CHXAudioPlayer* p)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Pause(): player [%p]", this, p);

    m_pMutex->Lock();

    HXBOOL bUseStopInPause = FALSE;

#if defined _DEBUG && defined HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
    bUseStopInPause = HXDebugOptionEnabled("zUseStopInPause");
#endif

    if (!bUseStopInPause && NumberOfResumedPlayers() == 0)
    {
        m_bPaused = TRUE;

        if(m_ulCallbackID)
        {
            m_pScheduler->Remove(m_ulCallbackID);
            m_ulCallbackID = 0;
        }

        // Check to see if all audio players are paused, then and only
        // then will we pause the device
        if ( m_pAudioDev )
        {
            m_pAudioDev->Pause();
            _NotifyTimelineWatchers(TLW_PAUSE);
        }

        m_bAtLeastOneTimeReceived = FALSE;
        m_pLastPausedPlayer = p;
    }
    /*
     * more than one audio player is currently active.
     * We need to do the following:
     * 1. get the current time from the device
     * 2. get the time for data we have already pushed down to the device
     * 3. flush the audio device
     * 4. instruct all streams to "rewind" by the "written but not played" duration
     */
#ifdef HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
    else if (!GetDisableMultiPlayPauseSupport() && m_pAudioDev && p->HasDataInAudioDevice())
    {
        HX_ASSERT(!m_bPaused && !m_bStoppedDuringPause);
        RewindSession();

        // this if is temporary
        if (NumberOfResumedPlayers() > 0)
        {
            ActualResume();
        }
    }
#endif

    m_pMutex->Unlock();
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::Resume
 *      Purpose:
 *       Resume playback of this Player's audio.
 */
HX_RESULT CHXAudioSession::Resume(CHXAudioPlayer* pPlayerToExclude, HXBOOL bRewindNeeded)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Resume(): player [%p]", this, pPlayerToExclude);

    HX_RESULT theErr = HXR_OK;

    if ((m_bPaused || m_bStoppedDuringPause) &&
        (!m_pLastPausedPlayer || (m_pLastPausedPlayer == pPlayerToExclude)))
    {
	if (bRewindNeeded)
	{
	    RewindSession();
	}
        theErr = ActualResume();
    }
#ifdef HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
    /*
     * we need to re-mix to start the newly resumed player instantly
     * and not delay it by pushdown
     */
    else if (!GetDisableMultiPlayPauseSupport() &&
             (NumberOfResumedPlayers() > 1 ||
              (m_pLastPausedPlayer && m_pLastPausedPlayer != pPlayerToExclude)))
    {
	if (bRewindNeeded)
	{
	    RewindSession();
	}
        theErr = ActualResume();
    }
#endif

    return theErr;
}

HX_RESULT
CHXAudioSession::ActualResume()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::ActualResume()", this);

    HX_RESULT   theErr  = HXR_OK;

    if (m_bInPlayAudio)
    {
        // can only happen in multi-player pause/resume/stop case
        m_bDeferActualResume = TRUE;
        return HXR_OK;
    }

    m_bPaused = FALSE;
    m_bStoppedDuringPause = FALSE;

    if (m_bToBeReOpened)
    {
        m_bDeferActualResume = TRUE;
        return HXR_OK;
    }

    if (m_bInActualResume)
    {
        return HXR_OK;
    }

    m_bInActualResume = TRUE;

    theErr = CheckToPlayMoreAudio();

    /*
     * we may "re-pause" the player in PlayAudio if we need to buffer more
     * data to play, if we are paused now, don't resume the audio device now
     * or we will play data that we don't account for.  Fixes the G2 B2
     * Live pause video sync bug.  JEFFA & RAHUL 11/4/98
     */
    if (theErr || m_bPaused || m_bStoppedDuringPause)
    {
        goto exit;
    }

    // Get the initial system time.
    if (m_pAudioDev)
    {
        m_pAudioDev->Resume();
        _NotifyTimelineWatchers(TLW_RESUME);
    }

    m_pLastPausedPlayer      = NULL;
    m_ulLastFakeCallbackTime = HX_GET_TICKCOUNT();

  exit:
    m_bInActualResume = FALSE;
    return theErr;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::Seek
 *      Purpose:
 *       Seek playback of this Player's audio.   TBD
 */
HX_RESULT CHXAudioSession::Seek(CHXAudioPlayer* pPlayerToExclude, const UINT32  ulSeekTime)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Seek(): to %lu", this, ulSeekTime);
    m_pMutex->Lock();
    if (NumberOfActivePlayers() <= 1)
    {
        m_ulStartTime   = m_ulCurrentTime = 0;
        m_dBufEndTime   = 0.;

        if (m_pAudioDev)
        {
            m_pAudioDev->Reset();
        }

        m_ulBlocksWritten       = 0;
        m_dNumBytesWritten      = (double) 0;
        m_dNumBytesPlayed       = (double) 0;

        while (m_pAuxiliaryAudioBuffers &&
               m_pAuxiliaryAudioBuffers->GetCount() > 0)
        {
            HXAudioData* pAudioData =
                (HXAudioData*) m_pAuxiliaryAudioBuffers->RemoveHead();
            pAudioData->pData->Release();
            delete pAudioData;
        }

        m_bFirstPlayAudio           = TRUE;

        m_bTimeSyncReceived         = FALSE;
        m_bAtLeastOneTimeReceived   = FALSE;
        m_ulLastAudioTime           = 0;
    }
    /*
     * more than one audio player is currently active.
     * We need to do the following:
     * 1. get the current time from the device
     * 2. get the time for data we have already pushed down to the device
     * 3. flush the audio device
     * 4. instruct all streams to "rewind" by the "written but not played" duration
     */
#ifdef HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
    else if (!GetDisableMultiPlayPauseSupport() && m_pAudioDev)
    {
        /* nothing needs to be done here..
         * it has already been taken care of udring Pause()!!
         *
         * this comment is here for the sake of completeness
         */
    }
#endif

    m_pMutex->Unlock();

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::Stop
 *      Purpose:
 *       Stop playback of this Player's audio.
 */
HX_RESULT CHXAudioSession::Stop( CHXAudioPlayer* p, HXBOOL bFlush )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Stop(): player [%p]", this, p);

    m_pMutex->Lock();
    if (NumberOfActivePlayers() == 0)
    {
        if(m_ulCallbackID)
        {
            m_pScheduler->Remove(m_ulCallbackID);
            m_ulCallbackID = 0;
        }

        // Close the audio device only if all players are not playing.
        if ( !IsPlaying() && m_pAudioDev )
        {
            // if m_pAudioDev is replaced, then the owner of m_pAudioDev may
            // call Remove() inside Close() to release replaced device
            IHXAudioDevice* pTempAudioDevice = m_pAudioDev;

            m_ulBlocksWritten = 0;
            m_pAudioDev = NULL;

            pTempAudioDevice->Close(bFlush);
            _NotifyTimelineWatchers( TLW_CLOSE );
        }

        // Reset the session ...
        ResetSession();
    }
    /*
     * more than one audio player is currently active.
     * We need to do the following:
     * 1. get the current time from the device
     * 2. get the time for data we have already pushed down to the device
     * 3. flush the audio device
     * 4. instruct all streams to "rewind" by the "written but not played" duration
     */
#ifdef HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
    else if (!GetDisableMultiPlayPauseSupport() && m_pAudioDev && p->HasDataInAudioDevice())
    {
        RewindSession(p);
        if (NumberOfResumedPlayers() > 0)
        {
            HX_ASSERT(m_bPaused == FALSE);
            ActualResume();
        }
    }
#endif

    m_pMutex->Unlock();
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::ResetSession
 *      Purpose:
 */
void CHXAudioSession::ResetSession(void)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::ResetSession()", this);

    // We only want to reset session if we are no longer playing anything.
    if ( IsPlaying() )
        return;

    m_bInited       = FALSE;
    m_bDisableWrite = FALSE;

    // Free the audio device if it is ours
    if ((!m_bReplacedDev || !m_bUsingReplacedDevice) && m_pCurrentAudioDev)
    {
        ReleaseAudioDevice();
    }

    RestoreReplacedDevice();

    m_pAudioDev = 0;

    // Delete the player mixer buffer
    HX_RELEASE(m_pPlayerBuf);

    // Delete the session mixer buffer
    HX_RELEASE(m_pSessionBuf);

    m_ulIncreasingTimer = 0;
    m_ulStartTime       = 0;
    m_ulCurrentTime     = 0;
    m_ulBytesPerGran    = 0;
    m_bHasStreams       = FALSE;
    m_bPaused            = TRUE;
    m_bStoppedDuringPause = FALSE;
    m_bTimeSyncReceived  = FALSE;
    m_bAtLeastOneTimeReceived = FALSE;
    m_ulLastAudioTime         = 0;
    m_bShouldOpenOnCoreThread = FALSE;

    if(m_ulCallbackID && m_pScheduler)
    {
        m_pScheduler->Remove(m_ulCallbackID);
        m_ulCallbackID = 0;
    }

    if (m_pDeviceCallback && m_pDeviceCallback->PendingID())
    {
        m_pScheduler->Remove(m_pDeviceCallback->PendingID());
        m_pDeviceCallback->PendingID(0);
    }

    m_bFirstPlayAudio       = TRUE;
    m_dBufEndTime = 0.;

    if (m_pAuxiliaryAudioBuffers)
    {
        while (m_pAuxiliaryAudioBuffers->GetCount() > 0)
        {
            HXAudioData* pAudioData =
                (HXAudioData*) m_pAuxiliaryAudioBuffers->RemoveHead();
            pAudioData->pData->Release();
            delete pAudioData;
        }

        HX_DELETE(m_pAuxiliaryAudioBuffers);
    }

    /* Default value of Device format */
    m_DeviceFmt.uChannels       = 2;
    m_DeviceFmt.uBitsPerSample  = 16;
    m_DeviceFmt.ulSamplesPerSec = 16000;
    m_DeviceFmt.uMaxBlockSize   = 64000;

    m_ulBlocksWritten           = 0;
    m_dNumBytesWritten          = (double) 0;
    m_dNumBytesPlayed           = (double) 0;
    m_bDeferActualResume        = FALSE;
    m_pLastPausedPlayer         = NULL;

    m_bOpaqueMode               = FALSE;
    HX_RELEASE(m_pOpaqueData);

    HX_RELEASE(m_pMPPSupport);

    return;
}

HX_RESULT
CHXAudioSession::ProcessAudioHook(PROCESS_ACTION action,
                                  IHXAudioHook* pAudioHook)
{
    return HXR_OK;
}

HX_RESULT
CHXAudioSession::ProcessAudioDevice(PROCESS_ACTION    action,
                                    IHXAudioDevice*  pAudioDevice)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::Replace
 *  Purpose:
 *      Used to replace the default audio device.
 *      This method will fail if the audio device is in use or it has
 *      already been replaced.
 */
STDMETHODIMP
CHXAudioSession::Replace(IHXAudioDevice* /*IN*/ pAudioDevice)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Replace(): device [%p]", this, pAudioDevice);
    if (!pAudioDevice)
    {
        return HXR_POINTER;
    }

    // Have we already replaced the audio device?
    if (m_bReplacedDev)
    {
        return HXR_UNEXPECTED;
    }

    // Is the audio device in use?
    if (m_pAudioDev)
    {
        return HXR_FAIL;
    }

    // make sure the new audio device is initialized.
    m_bReplacedDev = TRUE;
    m_bUsingReplacedDevice = TRUE;

    if (m_pCurrentAudioDev)
    {
        // XXXNH: if we've already created an audio device, then shut it down
        m_pCurrentAudioDev->Close(TRUE);
        _NotifyTimelineWatchers( TLW_CLOSE );
        HX_RELEASE(m_pCurrentAudioDev);
    }

    m_pCurrentAudioDev = pAudioDevice;
    m_pCurrentAudioDev->AddRef();

    m_pReplacedAudioDev = pAudioDevice;
    m_pReplacedAudioDev->AddRef();

    m_bAudioDeviceSupportsVolume = FALSE;
    if (m_pCurrentAudioDev->InitVolume(HX_MIN_VOLUME, HX_MAX_VOLUME) == TRUE)
    {
        m_bAudioDeviceSupportsVolume = TRUE;
        /* Is our current volume setting different ?*/
        UINT16 uNewVolume = m_pCurrentAudioDev->GetVolume();
        if (uNewVolume != m_uVolume)
        {
            m_uVolume = uNewVolume;
#ifdef HELIX_FEATURE_VOLUME
            if( m_pDeviceVolume )
            {
                m_pDeviceVolume->SetVolume(m_uVolume);
            }
#endif
        }
    }

    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      CHXAudioSession::Remove
 *  Purpose:
 *      Used to restore the default audio device.  A pointer to the
 *      original replacement audio device must be passed in for
 *      authentication purposes- we can't have just anyone reseting
 *      the audio device.
 *      This method will fail if the interface passed in is not the
 *      current audio device or if the current audio device is in use.
 */
STDMETHODIMP
CHXAudioSession::Remove(IHXAudioDevice* /*IN*/ pAudioDevice)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Remove(): device [%p]", this, pAudioDevice);
    if (!pAudioDevice)
    {
        return HXR_POINTER;
    }

    // If we're using the audio service fail.
    if (m_pAudioDev)
    {
        return HXR_FAIL;
    }

    // Have we not replaced the audio device?
    if (!m_bReplacedDev)
    {
        return HXR_UNEXPECTED;
    }

    // make sure the it's the same audio device
    if (m_pReplacedAudioDev != pAudioDevice)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (m_pCurrentAudioDev)
    {
        ProcessAudioDevice(ACTION_REMOVE, m_pCurrentAudioDev);
    }

    if (m_pCurrentAudioDev == m_pReplacedAudioDev)
    {
        // Close the audio device
        HX_RELEASE(m_pCurrentAudioDev);
    }

    HX_RELEASE(m_pReplacedAudioDev);
    m_bReplacedDev = FALSE;

    return HXR_OK;
}


/************************************************************************
 *  Method:
 *   IHXAudioDeviceManager::AddFinalHook
 *  Purpose:
 *       One last chance to modify data being written to the audio device.
 *       This hook allows the user to change the audio format that
 *   is to be written to the audio device. This can be done in call
 *   to OnInit() in IHXAudioHook.
 */
STDMETHODIMP
CHXAudioSession::SetFinalHook(IHXAudioHook* /*IN*/ pHook)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::SetFinalHook(): hook [%p]", this, pHook);
    if (m_pFinalHook || !pHook)
    {
        return HXR_UNEXPECTED;
    }

    m_pFinalHook = pHook;
    m_pFinalHook->AddRef();

    HXBOOL bIsInterruptSafe = FALSE;
    IHXInterruptSafe* pInterruptSafe = NULL;

    if (m_pFinalHook->QueryInterface(IID_IHXInterruptSafe,
                                     (void**) &pInterruptSafe) == HXR_OK)
    {
        bIsInterruptSafe = pInterruptSafe->IsInterruptSafe();
        pInterruptSafe->Release();
    }

    if (!bIsInterruptSafe)
    {
        IHXInterruptState* pState = NULL;
        HX_VERIFY(m_pContext->QueryInterface(IID_IHXInterruptState,
                                             (void**) &pState) == HXR_OK);

        pState->EnableInterrupt(FALSE);
        pState->Release();
    }

    ProcessAudioHook(ACTION_ADD, m_pFinalHook);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *   IHXAudioDeviceManager::RemoveFinalHook
 *  Purpose:
 *       Remove final hook
 */
STDMETHODIMP
CHXAudioSession::RemoveFinalHook(IHXAudioHook* /*IN*/ pHook)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::RemoveFinalHook(): hook [%p]", this, pHook);

    if (!m_pFinalHook || m_pFinalHook != pHook)
    {
        return HXR_UNEXPECTED;
    }

    ProcessAudioHook(ACTION_REMOVE, m_pFinalHook);

    HX_RELEASE(m_pFinalHook);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *   IHXAudioDeviceHookManager::AddAudioDeviceHook
 *  Purpose:
 *       One last chance to modify data being written to the audio device.
 */
STDMETHODIMP CHXAudioSession::AddAudioDeviceHook(IHXAudioHook* /*IN*/ pHook,
                                                 AudioDeviceHookType /*IN*/ type)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::AddAudioDeviceHook(): hook [%p]", this, pHook);
#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    if (!pHook)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (!m_pAudioDevHookList)
    {
        m_pAudioDevHookList = new CHXSimpleList;
        if(!m_pAudioDevHookList)
            return HXR_OUTOFMEMORY;
    }

    HXAudioDevHookEntry* pHookEntry = new HXAudioDevHookEntry;
    if(!pHookEntry)
        return HXR_OUTOFMEMORY;

    pHookEntry->type    = type;
    pHookEntry->pHook   = pHook;
    pHookEntry->pHook->AddRef();

    if(HXR_OK == ProcessAudioHook(ACTION_ADD, pHook))
    {
        // insert into the list, sorted by type
        LISTPOSITION lp = m_pAudioDevHookList->GetHeadPosition();
        while(lp)
        {
            HXAudioDevHookEntry* h = (HXAudioDevHookEntry*)m_pAudioDevHookList->GetAt(lp);

            if( type < h->type )
            {
                m_pAudioDevHookList->InsertBefore(lp, pHookEntry);
                break;
            }
            m_pAudioDevHookList->GetNext(lp);
        }
        if(!lp)
            m_pAudioDevHookList->AddTail(pHookEntry);
        
        // if have already been initialzied...
        if (m_bInited && m_pAudioDev)
        {
            if (HXR_OK == ProcessAudioHook(ACTION_CHECK, pHook))
            {
                HXAudioFormat audioFormat;
                audioFormat.uChannels       = m_ActualDeviceFmt.uChannels;
                audioFormat.uBitsPerSample  = m_ActualDeviceFmt.uBitsPerSample;     
                audioFormat.ulSamplesPerSec = m_ActualDeviceFmt.ulSamplesPerSec;
                audioFormat.uMaxBlockSize   = m_ActualDeviceFmt.uMaxBlockSize;
                
                pHook->OnInit(&audioFormat);
            }
        }
    }
#endif

    return HXR_OK;
}


/************************************************************************
 *  Method:
 *   IHXAudioDeviceHookManager::RemoveAudioDeviceHook
 *  Purpose:
 *       Remove audio device hook
 */
STDMETHODIMP
CHXAudioSession::RemoveAudioDeviceHook(IHXAudioHook* /*IN*/ pHook)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::RemoveAudioDeviceHook(): hook [%p]", this, pHook);
#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    if (!pHook || !m_pAudioDevHookList)
    {
        return HXR_UNEXPECTED;
    }

    HXBOOL bFound = FALSE;
    HXAudioDevHookEntry* h = 0;
    LISTPOSITION lp, lastlp;
    lastlp = 0;
    lp = m_pAudioDevHookList->GetHeadPosition();
    while( lp )
    {
        lastlp = lp;
        h = (HXAudioDevHookEntry*) m_pAudioDevHookList->GetNext(lp);
        if ( pHook == h->pHook )
        {
            ProcessAudioHook(ACTION_REMOVE, pHook);

            HX_RELEASE(h->pHook);
            HX_DELETE(h);
            m_pAudioDevHookList->RemoveAt(lastlp);
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        return HXR_FAILED;
    }
#endif

    return HXR_OK;
}

HX_RESULT CHXAudioSession::ProcessAudioDeviceHooks
(
    IHXBuffer*&     pInBuffer,
    HXBOOL&           bChanged
    )
{
    HX_RESULT theErr = HXR_OK;

#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    HXAudioDevHookEntry* h = 0;

    bChanged = FALSE;

    if (m_pAudioDevHookList && m_pAudioDevHookList->GetCount() > 0)
    {
        HXAudioData inData, outData;
        memset (&inData,  0, sizeof (inData));
        memset (&outData, 0, sizeof (outData));

        inData.pData = pInBuffer;
        inData.uAudioStreamType = outData.uAudioStreamType = STREAMING_AUDIO;

        // Each hook associated with this player gets the data. 
        CHXSimpleList::Iterator lIter = m_pAudioDevHookList->Begin();
        for (; !theErr && lIter != m_pAudioDevHookList->End(); ++lIter)
        {
            h = (HXAudioDevHookEntry*) (*lIter);

            if (HXR_OK == ProcessAudioHook(ACTION_CHECK, h->pHook))
            {
                theErr = h->pHook->OnBuffer (&inData, &outData);

                /* Check to see if post hook changed the buffer. If so, then
                 * make this output as input to the next Hook.
                 */
                if (!theErr && outData.pData)
                {
                    if(h->type != READ_ONLY_EARLY && h->type != READ_ONLY_LATE)
                    {
                        HX_RELEASE(inData.pData);
                        inData.pData         = outData.pData;
                        inData.ulAudioTime   = outData.ulAudioTime;

                        pInBuffer = outData.pData;
                        outData.pData = 0;

                        bChanged = TRUE;
                    }
                    else
                        HX_RELEASE(outData.pData);
                }
            }
        }
        inData.pData = NULL;
    }
#endif

    return theErr;
}

/************************************************************************
 *  Method:
 *      IHXAudioDeviceManager::GetAudioFormat
 *  Purpose:
 *           Returns the audio format in which the audio device is opened.
 *           This function will fill in the pre-allocated HXAudioFormat
 *           structure passed in.
 */
STDMETHODIMP
CHXAudioSession::GetAudioFormat(HXAudioFormat*  /*IN/OUT*/pAudioFormat)
{
    if (!pAudioFormat)
    {
        return HXR_UNEXPECTED;
    }

    memcpy( pAudioFormat, &m_ActualDeviceFmt, sizeof( HXAudioFormat ));
    return HXR_OK;
}

/**********************************************************************
 *  Method:
 *      IHXAudioDeviceManager2::IsReplacedDevice
 *  Purpose:
 *  This is used to determine if the audio device has been replaced.
 */
STDMETHODIMP_(HXBOOL) CHXAudioSession::IsReplacedDevice()
{
    if (m_bUsingReplacedDevice && m_pReplacedAudioDev)
    {
        return TRUE;
    }

    return FALSE;
}

/*
 *  IHXAudioResamplerManager methods
 *
 */
STDMETHODIMP
CHXAudioSession::CreateResampler(HXAudioFormat              inAudioFormat,
                                 REF(HXAudioFormat)         outAudioFormat,
                                 REF(IHXAudioResampler*)   pResampler)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::CreateResampler()", this);

    HX_RESULT                   rc = HXR_FAILED;
#if defined(HELIX_FEATURE_RESAMPLER) && !defined(HELIX_CONFIG_FIXEDPOINT)
    HXCDQualityResampler*       pNewResampler = NULL;

    pResampler = NULL;

    pNewResampler = new HXCDQualityResampler();
    if (pNewResampler &&
        HXR_OK == pNewResampler->Init(inAudioFormat, outAudioFormat))
    {
        rc = pNewResampler->QueryInterface(IID_IHXAudioResampler, (void**)&pResampler);
    }
    else
    {
        HX_DELETE(pNewResampler);
        rc = HXR_FAILED;
    }
#endif /* HELIX_FEATURE_RESAMPLER && !HELIX_CONFIG_FIXEDPOINT*/

    return rc;
}

/*
 *  IHXAudioPushdown methods
 */
/************************************************************************
 *  Method:
 *      IHXAudioPushdown::SetAudioPushdown
 *  Purpose:
 *           Use this to set the minimum audio pushdown value in ms.
 *           This is the amount of audio data that is being written
 *           to the audio device before starting playback.
 */
STDMETHODIMP CHXAudioSession::SetAudioPushdown(UINT32 ulMinimumPushdown)
{
    
    m_ulTargetPushdown = (ulMinimumPushdown<MINIMUM_AUDIO_PUSHDOWN) ?
        MINIMUM_AUDIO_PUSHDOWN : ulMinimumPushdown;

    UpdateMinimumPushdown();
    
    HXLOGL3(HXLOG_ADEV,
            "CHXAudioSession[%p]::SetAudioPushdown(): push down = %lu",
            this, ulMinimumPushdown);
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioPushdown2::GetAudioPushdown
 *  Purpose:
 *           Use this to get the minimum audio pushdown value in ms.
 *           This is the amount of audio data that is being written
 *           to the audio device before starting playback.
 */
STDMETHODIMP
CHXAudioSession::GetAudioPushdown(REF(UINT32) /*OUT*/ ulAudioPushdown)
{
    ulAudioPushdown = m_ulTargetPushdown;
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioPushdown2::GetCurrentAudioDevicePushdown
 *  Purpose:
 *           Use this to get the audio pushed down to the audio device and haven't
 *           been played yet
 */
STDMETHODIMP
CHXAudioSession::GetCurrentAudioDevicePushdown(REF(UINT32) /*OUT*/ ulAudioPusheddown)
{
    HX_RESULT rc = HXR_OK;

    ulAudioPusheddown = 0;

    if (!m_bToBeReOpened && m_pAudioDev)
    {
	UINT32 ulCurTime = 0;

	if (SUCCEEDED(m_pAudioDev->GetCurrentAudioTime(ulCurTime)))
	{
	    ulAudioPusheddown = ((UINT32) m_dBufEndTime) - ulCurTime;
	    if (((LONG32) ulAudioPusheddown) < 0)
	    {
		ulAudioPusheddown = 0;
	    }
	}
    }

    return rc;
}

INT32 CHXAudioSession::GetCurrentRawAudioDevicePushdown(void)
{
    HX_RESULT rc = HXR_OK;
    INT32 lAudioDevicePushdown = 0;

    if (!m_bToBeReOpened && m_pAudioDev)
    {
	UINT32 ulCurTime = 0;

	if (SUCCEEDED(m_pAudioDev->GetCurrentAudioTime(ulCurTime)))
	{
	    lAudioDevicePushdown = (INT32) (((UINT32) m_dBufEndTime) - ulCurTime);
	}
    }

    return lAudioDevicePushdown;
}

// We no longer use CHXAudioDevice::NumberOfBlocksRemainingToPlay() to determine
// how much data is left in the audio device. Not only did we have the problem
// of differences on how it was calculated between different platforms but each
// platform already has to give back very accurate timeline information. Since
// we keep track of how many blocks we have written, we just use that and the
// current playback time to determine how much data is down in the device.
ULONG32 CHXAudioSession::GetBlocksRemainingToPlay()
{
    ULONG32 ulRetVal         = 0;
    ULONG32 ulCurTime        = 0;
    ULONG32 ulMsWritten      = 0;
    LONG32  lMsInAudioDevice = 0;
    if( m_pAudioDev && m_pAudioDev->GetCurrentAudioTime(ulCurTime)==HXR_OK )
    {
        ulMsWritten = (ULONG32)(m_ulBlocksWritten * m_dGranularity);
        lMsInAudioDevice = ulMsWritten-ulCurTime;
        if( lMsInAudioDevice>0 )
        {
            ulRetVal = (ULONG32)floor(lMsInAudioDevice/m_dGranularity);
        }
        else
        {
            if( !m_bPaused && !m_bFirstPlayAudio )
            {
                //We just underflowed
                HXLOGL1(HXLOG_ADEV, "Audio device just underflowed. Difference: %d",
                        (int)lMsInAudioDevice);
            }
        }
    }
    return ulRetVal;
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::OnTimeSync
 *  Purpose:
 *      Notification interface provided by users of the IHXAudioDevice
 *      interface. This method is called by the IHXAudioDevice when
 *      audio playback occurs.
 */
STDMETHODIMP CHXAudioSession::OnTimeSync(ULONG32 /*IN*/ ulCurrentTime)
{
    HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::OnTimeSync(): time = %lu", this, ulCurrentTime);

    HX_RESULT theErr = HXR_OK;
    if (m_pCoreMutex)
    {
        m_pCoreMutex->Lock();
    }

    m_bTimeSyncReceived = TRUE;
#ifdef _MACINTOSH
    if (zm_Locked)
    {
        m_pCoreMutex->Unlock();
        return HXR_OK;
    }

    zm_Locked=TRUE;
#endif

    CHXAudioPlayer* pPlayer;
    CHXSimpleList::Iterator lIter;

    /*
     * We may reach here from a WOM_DONE callback from audio thread AFTER
     * stopping the audio device and resetting our member variables.
     * This is because we may be waiting on m_pCoreMutex owned by the main
     * thread (where we stop the audio device).
     * Do not update any member variables in this case since this may screw
     * up subsequent playback.
     */

    /* On Mac, we may have been paused but have not yet issued
     * pause on the audio device (due to interrupt time conflicts).
     * Ignore any more timesyncs and do not pump any more data
     * to the audio device.
     */
    if (!m_bInited || m_bPaused || m_bStoppedDuringPause)
    {
        goto exit;
    }

    /* This is only set in rewindsession - used in pause/resume for multiple
     * players. We may be waiting on te core mutex on a timesync callback
     * from the audio device when we reset/resume the audio device since
     * someone hit pause/resume. In this case, the time passed into
     * OnTimeSync is still from previous generation and we need to resync.
     */
    if (m_uAskFromAudioDevice > 0 && m_pAudioDev)
    {
        m_uAskFromAudioDevice--;
        m_pAudioDev->GetCurrentAudioTime(ulCurrentTime);
    }


#if defined(_WINDOWS) || defined(_WIN32) || defined(_MACINTOSH)
    ulCurrentTime = AnchorDeviceTime(ulCurrentTime);
#endif /*defined(_WINDOWS) || defined(_WIN32) || defined(_MACINTOSH)*/

    updateCurrentTime(ulCurrentTime);

    m_dNumBytesPlayed = ConvertMsToBytes(m_ulCurrentTime - m_ulStartTime);

    /* Notify each audio player in the session about the current
     * time.
     */
    lIter = m_pPlayerList->Begin();
    pPlayer = NULL;
    for (; lIter != m_pPlayerList->End(); ++lIter)
    {
        pPlayer = (CHXAudioPlayer*) (*lIter);
        /* Only if the player is in *PLAY* state, send a timesync*/
        if (pPlayer->GetState() == E_PLAYING && pPlayer->GetStreamCount() > 0)
        {
            theErr = pPlayer->OnTimeSync(m_ulCurrentTime);
        }
    }

    //Let all the timeline watchers know..
    _NotifyTimelineWatchers( TLW_TIMESYNC, m_ulCurrentTime);
    
    /* Start playback of next buffer.  
       If we are in process of resuming, skip the check as the resume just filled in
       what is needed for the start */
    if (theErr == HXR_OK && m_bHasStreams && (!m_bInActualResume))
    {
        theErr = CheckToPlayMoreAudio();
    }

  exit:

#ifdef _MACINTOSH
    zm_Locked=FALSE;
#endif

    if (m_pCoreMutex)
    {
        m_pCoreMutex->Unlock();
    }

    lIter = m_pPlayerList->Begin();
    if( theErr )
    {
        HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::OnTimeSync(): err = 0x%08x", this, theErr);
        for (; lIter != m_pPlayerList->End(); ++lIter)
        {
            pPlayer = (CHXAudioPlayer*) (*lIter);
            if( pPlayer )
            {
                pPlayer->SetError(theErr);
            }
        }
    }
    return theErr;
}

void CHXAudioSession::UpdateMinimumPushdown()
{
    //This is called whenever the granularity or audio pushdown changes.  In
    //this method we need to make sure that the granularity is results in at
    //least 2 blocks being the min pushdown. If we are left with only one block,
    //it can be hard to prevent underflow in the audio device. You don't have to
    //increase the amount of PCM pushdown, just decrease the granularity.
    if( m_ulGranularity )
    {
        m_ulTargetBlocksTobeQueued = (ULONG32)ceil(m_ulTargetPushdown/m_dGranularity);
        m_ulMinBlocksTobeQueuedAtStart = (ULONG32)ceil(m_ulMinimumStartupPushdown/m_dGranularity);
	m_ulMinBlocksTobeQueued = (ULONG32)ceil(m_ulMinimumPushdown/m_dGranularity);

#if defined(HELIX_FEATURE_PREFERENCES)
        HXBOOL bRestore = FALSE;
        ReadPrefBOOL( m_pPreferences, "RestoreMinimumPushdown", bRestore );
        if( bRestore)
        {
            m_ulMinBlocksTobeQueuedAtStart = m_ulTargetBlocksTobeQueued;
	    m_ulMinBlocksTobeQueued = m_ulTargetBlocksTobeQueued;
        }
#endif
        //Invarients to ensure glitch free audio playback.
	HX_ASSERT( m_ulGranularity < m_ulMinimumPushdown );
        HX_ASSERT( m_ulGranularity <= m_ulTargetPushdown/2 );
        HX_ASSERT( m_ulMinBlocksTobeQueued > 1 );
	HX_ASSERT( m_ulMinBlocksTobeQueuedAtStart >= m_ulMinBlocksTobeQueued );
        HX_ASSERT( m_ulTargetBlocksTobeQueued >= m_ulMinBlocksTobeQueuedAtStart );
	HX_ASSERT( m_ulMinimumStartupPushdown >= m_ulMinimumPushdown );
        HX_ASSERT( m_ulTargetPushdown >= m_ulMinimumStartupPushdown );
        HXLOGL2( HXLOG_ADEV, "Audio pushdown values Report:" );
        HXLOGL2( HXLOG_ADEV, "          Granularity: %lu", m_ulGranularity );
	HXLOGL2( HXLOG_ADEV, "Blocks at the minimum: %lu", m_ulMinBlocksTobeQueued );
        HXLOGL2( HXLOG_ADEV, "      Blocks at start: %lu", m_ulMinBlocksTobeQueuedAtStart );
        HXLOGL2( HXLOG_ADEV, "    Blocks to grow to: %lu", m_ulTargetBlocksTobeQueued );
        HXLOGL2( HXLOG_ADEV, "MS of data at minimum: %lu", m_ulMinimumPushdown );
	HXLOGL2( HXLOG_ADEV, "  MS of data at start: %lu", m_ulMinimumStartupPushdown );
        HXLOGL2( HXLOG_ADEV, "MS of data to grow to: %lu", m_ulTargetPushdown );
    }
}

/*
 *  IHXAudioHookManager methods
 */
/************************************************************************
 *  Method:
 *      IHXAudioHookManager::AddHook
 *  Purpose:
 *           Use this to add a hook
 */
STDMETHODIMP CHXAudioSession::AddHook(IHXAudioHook* /*IN*/ pHook)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::AddHook(): hook [%p]", this, pHook);
#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    if (!pHook)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (!m_pHookList)
    {
        m_pHookList = new CHXSimpleList;
    }

    HXAudioHookInfo* pHookInfo = new HXAudioHookInfo;
    pHookInfo->pHook = pHook;
    pHookInfo->pHook->AddRef();

    pHookInfo->bDisableWrite = FALSE;
    pHookInfo->bFinal   = FALSE;
    pHookInfo->bIgnoreAudioData = FALSE;
    pHookInfo->bMultiChannelSupport = FALSE;

    IHXValues* pValues = NULL;
    if (pHook && pHook->QueryInterface(IID_IHXValues, (void**) &pValues) == HXR_OK)
    {
        UINT32 ulValue = 0;
        pValues->GetPropertyULONG32("IgnoreAudioData", ulValue);
        pHookInfo->bIgnoreAudioData = (ulValue == 1);
        HX_RELEASE(pValues);
    }

    IHXAudioMultiChannel* pMultiChannel = NULL;
    if (pHook && HXR_OK == pHook->QueryInterface(IID_IHXAudioMultiChannel, (void**) &pMultiChannel))
    {
        pHookInfo->bMultiChannelSupport = pMultiChannel->GetMultiChannelSupport();
    }
    HX_RELEASE(pMultiChannel);

    m_pHookList->AddTail(pHookInfo);

    ProcessAudioHook(ACTION_ADD, pHook);

    // if have already been initialzied...
    if (m_bInited && m_pAudioDev)
    {
        if (pHookInfo->bIgnoreAudioData ||
            HXR_OK == ProcessAudioHook(ACTION_CHECK, pHook))
        {
            HXAudioFormat audioFormat;
            memcpy( &audioFormat, &m_ActualDeviceFmt, sizeof(HXAudioFormat) );
            pHook->OnInit(&audioFormat);
        }
    }
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioHookManager::RemoveHook
 *  Purpose:
 *           Use this to remove a hook
 */
STDMETHODIMP CHXAudioSession::RemoveHook(IHXAudioHook* /*IN*/ pHook)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::RemoveHook(): hook [%p]", this, pHook);
#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    if (!pHook || !m_pHookList)
    {
        return HXR_UNEXPECTED;
    }

    HXBOOL bFound = FALSE;
    HXAudioHookInfo* h = 0;
    LISTPOSITION lp, lastlp;
    lp = lastlp = 0;
    lp = m_pHookList->GetHeadPosition();
    while( lp )
    {
        lastlp = lp;
        h = (HXAudioHookInfo*) m_pHookList->GetNext(lp);
        if ( pHook == h->pHook )
        {
            ProcessAudioHook(ACTION_REMOVE, pHook);

            h->pHook->Release();
            delete h;
            h = 0;
            m_pHookList->RemoveAt(lastlp);
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        return HXR_FAILED;
    }
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::OnTimerCallback
 *  Purpose:
 *      Timer callback when implementing fake timeline.
 */
void    CHXAudioSession::OnTimerCallback()
{
    // NOTE: we are ignoring an HX_RESULT return value here.
    OnTimeSync(m_ulIncreasingTimer);
    /* A call to timesync may result in stopping
     * playback and we do not want to have any more
     * time syncs.
     */
    if (!m_bInited)
    {
        return;
    }

    /* Put this back in the scheduler.
     */
    if(0==m_ulCallbackID)
    {
        *m_pFakeAudioCBTime += (int) (m_ulGranularity*1000);
        m_ulCallbackID = m_pScheduler->RelativeEnter( this, m_ulGranularity);
    }

    updateFakeTimeline();
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::GetCurrentPlaybackTime
 *  Purpose:
 */
ULONG32 CHXAudioSession::GetCurrentPlayBackTime(void)
{
    m_pMutex->Lock();

    if (!m_bPaused && !m_bStoppedDuringPause)
    {
        ULONG32 ulCurTime = 0;

        /* Get audio device time. */
        if ( m_pAudioDev )
        {
#if !defined(_WINDOWS) && !defined(_WIN32) && !defined(_MACINTOSH)
            if (m_bTimeSyncReceived)
            {
                m_pAudioDev->GetCurrentAudioTime(ulCurTime);

                updateCurrentTime(ulCurTime);
            }
#else
            AdjustInRealTime();
#endif /*!defined(_WINDOWS) && !defined(_WIN32) && !defined(_MACINTOSH)*/
        }
        else
        {
            updateFakeTimeline();
            updateCurrentTime(m_ulIncreasingTimer);
        }
    }

    m_dNumBytesPlayed = ConvertMsToBytes(m_ulCurrentTime - m_ulStartTime);
    m_pMutex->Unlock();
    return m_ulCurrentTime;
}


UINT32
CHXAudioSession::AnchorDeviceTime(UINT32 ulCurTime)
{
    UINT32  ulAdjustTime            = ulCurTime;
    UINT32  ulCurrentSystemTime     = HX_GET_TICKCOUNT();

    if (!(CALCULATE_ELAPSED_TICKS(m_ulLastAudioTime, ulCurTime) > SOME_INSANELY_LARGE_VALUE))
    {
        if (m_bAtLeastOneTimeReceived)
        {
            if (m_ulLastAudioTime == ulCurTime)
            {
                UINT32 ulElapsedTime = CALCULATE_ELAPSED_TICKS(m_ulLastSystemTime, ulCurrentSystemTime);
                ulAdjustTime        += ulElapsedTime;
            }

            if (ulAdjustTime < m_ulLastAudioReturnTime)
            {
                ulAdjustTime = m_ulLastAudioReturnTime;
            }
        }

        m_ulLastAudioTime           = ulCurTime;
        m_ulLastSystemTime          = ulCurrentSystemTime;
        m_ulLastAudioReturnTime     = ulAdjustTime;
        m_bAtLeastOneTimeReceived   = TRUE;

        HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::AnchorDeviceTime(): time = %lu; sys time = %lu; adjust = %lu", this, m_ulLastAudioTime, m_ulLastSystemTime, ulAdjustTime);
    }

    return ulAdjustTime;
}

void
CHXAudioSession::AdjustInRealTime()
{
    if (m_bAtLeastOneTimeReceived)
    {
        UINT32  ulCurrentSystemTime = HX_GET_TICKCOUNT();
        UINT32 ulElapsedTime = CALCULATE_ELAPSED_TICKS(m_ulLastSystemTime, 
                                                       ulCurrentSystemTime);

        updateCurrentTime(m_ulLastAudioTime + ulElapsedTime);
    }
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::IsPlaying()
 *  Purpose:
 *      Returns TRUE if any audio player is playing.
 */
HXBOOL CHXAudioSession::IsPlaying()
{
    if (m_pPlayerList)
    {
        CHXAudioPlayer* pPlayer = 0;
        CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
        for (; lIter != m_pPlayerList->End(); ++lIter)
        {
            pPlayer = (CHXAudioPlayer*) (*lIter);
            if (pPlayer->GetState() == E_PLAYING && pPlayer->GetStreamCount() > 0)
                return TRUE;
        }
    }
    return FALSE;
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::NumberOfActivePlayers()
 *  Purpose:
 *      Returns number of active players (which are not is a STOP state).
 */
UINT16 CHXAudioSession::NumberOfActivePlayers()
{
    UINT16 uNumActive = 0;
    if (m_pPlayerList && m_pPlayerList->GetCount() > 0)
    {
        CHXAudioPlayer* pPlayer = 0;
        CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
        for (; lIter != m_pPlayerList->End(); ++lIter)
        {
            pPlayer = (CHXAudioPlayer*) (*lIter);
            if (pPlayer->GetState() != E_STOPPED && pPlayer->GetStreamCount() > 0)
            {
                uNumActive++;
            }
        }
    }

    return uNumActive;
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::NumberOfResumedPlayers()
 *  Purpose:
 *      Returns number of resumed players (which are in a PLAY state).
 */
UINT16 CHXAudioSession::NumberOfResumedPlayers()
{
    UINT16 uNumActive = 0;
    if (m_pPlayerList && m_pPlayerList->GetCount() > 0)
    {
        CHXAudioPlayer* pPlayer = 0;
        CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
        for (; lIter != m_pPlayerList->End(); ++lIter)
        {
            pPlayer = (CHXAudioPlayer*) (*lIter);
            if (pPlayer->GetState() == E_PLAYING && pPlayer->GetStreamCount() > 0)
            {
                uNumActive++;
            }
        }
    }

    return uNumActive;
}

/************************************************************************
 *  Method:
 *      CHXAudioSession::CheckDisableWrite()
 *  Purpose:
 *      TRUE:  If ALL of the players have PostProcessHook with
 *             DisableWrite flag ON.
 *      FALSE: Otherwise
 */
HXBOOL CHXAudioSession::CheckDisableWrite()
{
    HXBOOL bDisableWrite = FALSE;
    if ( m_pPlayerList && m_pPlayerList->GetCount() > 0)
    {
        CHXAudioPlayer* pPlayer = 0;
        CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
        for (; lIter != m_pPlayerList->End(); ++lIter)
        {
            pPlayer = (CHXAudioPlayer*) (*lIter);
            if (pPlayer->GetStreamCount() == 0)
            {
                continue;
            }
            /* If ANY ONE of them is not disabled, return FALSE */
            if (!pPlayer->IsDisableWrite())
            {
                bDisableWrite = FALSE;
                break;
            }
            else
            {
                bDisableWrite = TRUE;
            }
        }
    }

    m_bDisableWrite = bDisableWrite;

    return bDisableWrite;
}

HX_RESULT
CHXAudioSession::CheckToPlayMoreAudio()
{
    HX_RESULT theErr = HXR_OK;

    if (m_bToBeReOpened)
    {
        return HXR_OK;
    }

    HXBOOL bPlay = FALSE;
    UINT16 uNumBlocks = 0;
    if (!m_pAudioDev)
    {
        bPlay = TRUE;
    }
    else
    {
        m_uNumToBePlayed = uNumBlocks = (UINT16) GetBlocksRemainingToPlay();
        bPlay = (uNumBlocks < m_ulTargetBlocksTobeQueued) ? TRUE : FALSE;
        HXLOGL4( HXLOG_ADEV,
                 "CHXAudioSession[%p]::CheckToPlayMoreAudio(): block count = %lu",
                 this,
                 uNumBlocks);
    }

    if (bPlay)
    {
        HX_ASSERT(!m_pAudioDev || m_ulTargetBlocksTobeQueued > uNumBlocks );
        if (m_pAudioDev && m_ulTargetBlocksTobeQueued > uNumBlocks)
        {
	    // We are being called at frequencey less than the granularity.
	    // This it is sufficient to push one block per loop.
	    // However, we will push a bit more when more than a granule block
	    // behind - but never more than m_ulMaxBlocksToPlayPerGranule.
            uNumBlocks = (UINT16) m_ulTargetBlocksTobeQueued - uNumBlocks;
	    if (uNumBlocks > m_ulMaxBlocksToPlayPerGranule)
	    {
		uNumBlocks = (UINT16) m_ulMaxBlocksToPlayPerGranule;
	    }
        }
        else
        {
            uNumBlocks = 1;
        }

        theErr = PlayAudio(uNumBlocks);
    }

    /* Put this back in the scheduler only if audio streams are present AND disable write
     * is OFF. If disable write is ON, audio data is written in fake timer callbacks
     */
    if (!m_ulCallbackID && m_bHasStreams && !m_bDisableWrite)
    {
        m_bFakeAudioTimeline = FALSE;
        m_ulCallbackID = m_pScheduler->RelativeEnter(this, m_ulGranularity*m_nPercentage/100);
    }

    return theErr;
}


ULONG32 CHXAudioSession::GetInitialPushdown(HXBOOL bAtStart /* = FALSE*/)
{
    UINT32 uPush = 0;

    if (m_ulGranularity == 0)
    {
        return 0;
    }

    if (bAtStart)
    {
        uPush = m_ulMinBlocksTobeQueuedAtStart;
    }
    else
    {
        uPush = m_ulTargetBlocksTobeQueued;
    }

    return (ULONG32) (uPush * m_ulGranularity);
}

STDMETHODIMP CHXAudioSession::Func(void)
{
    HX_RESULT theErr    = HXR_OK;

    m_ulCallbackID = 0;

    if( m_bFakeAudioTimeline )
    {
        OnTimerCallback();
    }
    else
    {
        theErr = CheckToPlayMoreAudio();
        if( theErr != HXR_OK )
        {
            CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
            CHXAudioPlayer* pPlayer = NULL;
            for (; lIter != m_pPlayerList->End(); ++lIter)
            {
                pPlayer = (CHXAudioPlayer*) (*lIter);
                if( pPlayer )
                {
                    pPlayer->SetError(theErr);
                }
            }
        }
    }
    return theErr;
}

ULONG32
CHXAudioSession::SetGranularity(ULONG32 ulGranularity)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::SetGranularity(): gran = %lu", this, ulGranularity);

    /* DO NOT change Granularity in the midst of a presentation*/
    if (!m_bInited)
    {
        m_ulGranularity = ulGranularity;
        m_dGranularity = m_ulGranularity;

        UpdateMinimumPushdown();
    }

    return m_ulGranularity;
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::GetFormat
 *      Purpose:
 *              Return the session's audio device format.
 */
void CHXAudioSession::GetFormat
(
    HXAudioFormat*   pAudioFormat
    )
{
    memcpy(pAudioFormat, &m_DeviceFmt, sizeof( HXAudioFormat) );
}

/************************************************************************
 *  Method:
 *              CHXAudioSession::ProcessPostMixHooks
 *      Purpose:
 *              Send audio data to this player's post mix data hook.
 *
 */
HX_RESULT CHXAudioSession::ProcessPostMixHooks
(
    CHXAudioPlayer* pAudioPlayer,
    IHXBuffer*&    pInBuffer,
    HXBOOL*           bDisableWrite,
    UINT32          ulBufTime,
    HXBOOL&           bChanged
    )
{
#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    HX_RESULT           theErr      = HXR_OK;
    CHXSimpleList*      pHookList   = 0;
    HXAudioHookInfo*     h           = 0;
    UCHAR*              pInData     = pInBuffer->GetBuffer();

    bChanged        = FALSE;
    *bDisableWrite  = FALSE;

    pHookList = pAudioPlayer->GetPostMixHookList();
    if ( pHookList && pHookList->GetCount() > 0)
    {
        pInBuffer->AddRef();

        m_pInDataPtr->pData             = pInBuffer;
        m_pInDataPtr->ulAudioTime       = ulBufTime;
        m_pOutDataPtr->pData            = NULL;

        m_bPostMixHooksUpdated = FALSE;

        // Each hook associated with this player gets the data.
        CHXSimpleList::Iterator lIter = pHookList->Begin();
        for (; !theErr && lIter != pHookList->End(); ++lIter)
        {
            h = (HXAudioHookInfo*) (*lIter);

            if (HXR_OK == ProcessAudioHook(ACTION_CHECK, h->pHook))
            {
                // XXXHP, disable hooks when it doesn't support multi-channel
                if (m_ActualDeviceFmt.uChannels <= 2 || h->bMultiChannelSupport)
                {
                    if ( h->bDisableWrite )
                    {
                        *bDisableWrite = TRUE;
                    }

                    theErr = h->pHook->OnBuffer( m_pInDataPtr, m_pOutDataPtr );
                    /*
                    // Testing code to simulate post hook
                    static int copy = 1;

                    if (copy == 1)
                    {
                    m_pOutDataPtr->pData = m_pInDataPtr->pData;
                    m_pOutDataPtr->pData->AddRef();
                    m_pOutDataPtr->ulAudioTime  = m_pInDataPtr->ulAudioTime;
                    }
                    else if (copy ==2)
                    {
                    m_pOutDataPtr->pData = new CHXBuffer;
                    m_pOutDataPtr->pData->AddRef();
                    m_pOutDataPtr->pData->Set(m_pInDataPtr->pData->GetBuffer(), m_pInDataPtr->pData->GetSize());
                    m_pOutDataPtr->ulAudioTime  = m_pInDataPtr->ulAudioTime;
                    }
                    */

                    /* Check to see if post hook changed the buffer. If so, then
                     * make this output as input to the next Hook.
                     */
                    if (!theErr && m_pOutDataPtr->pData)
                    {
                        /* HACK HACK HACK!!!!!
                         *
                         * RealJukeBox Beta Release 1 has a bug in equalizer
                         * where it passes a buffer without addrefing it....
                         * i.e. refcount of 0.
                         *
                         * We detect this and do this work for them
                         *
                         * This code will be removed post RealPlayer Beta 2
                         * (that is when RealJukeBox Beta 1 expires)
                         *
                         * - XXXRA
                         */
                        m_pOutDataPtr->pData->AddRef();
                        m_pOutDataPtr->pData->AddRef();

                        if (m_pOutDataPtr->pData->Release() > 1)
                        {
                            m_pOutDataPtr->pData->Release();
                        }

                        m_pInDataPtr->pData->Release();
                        m_pInDataPtr->pData         = m_pOutDataPtr->pData;
                        m_pInDataPtr->ulAudioTime   = m_pOutDataPtr->ulAudioTime;

                        m_pOutDataPtr->pData        = 0;

                        bChanged = TRUE;
                    }
                }
            }
            else if (h->bIgnoreAudioData)
            {
                IHXBuffer* pTempBuf = m_pInDataPtr->pData;
                m_pInDataPtr->pData = NULL;
                theErr = h->pHook->OnBuffer( m_pInDataPtr, m_pOutDataPtr);
                m_pInDataPtr->pData = pTempBuf;
            }

            // the PostMixHook list could be altered in OnBuffer() call
            // during SoundAnimation
            if (m_bPostMixHooksUpdated)
            {
                break;
            }
        }
    }

    // Copy the data back to the input structure.
    if (!theErr && bChanged && m_pInDataPtr->pData)
    {
        ULONG32 ulSize = m_pInDataPtr->pData->GetSize();

        HX_ASSERT(ulSize <= m_ulBytesPerGran);

        HX_RELEASE(pInBuffer);

        /* limit size of the buffer to granularity */
        if (ulSize > m_ulBytesPerGran)
        {
	    theErr = CreateAndSetBufferCCF(pInBuffer, m_pInDataPtr->pData->GetBuffer(),
					   m_ulBytesPerGran, m_pContext);
        }
        else
        {
            m_pInDataPtr->pData->AddRef();
            pInBuffer = m_pInDataPtr->pData;
        }
    }

    HX_RELEASE(m_pInDataPtr->pData);

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */
}

void
CHXAudioSession::InitHooks()
{
    if(m_pHookList)
    {
        CHXSimpleList::Iterator ndx = m_pHookList->Begin();
        for (; ndx != m_pHookList->End(); ++ndx)
        {
            HXAudioHookInfo* pHookInfo = (HXAudioHookInfo*) (*ndx);

            if (pHookInfo->bIgnoreAudioData ||
                HXR_OK == ProcessAudioHook(ACTION_CHECK, pHookInfo->pHook))
            {
                HXAudioFormat audioFormat;
                memcpy( &audioFormat, &m_ActualDeviceFmt, sizeof( HXAudioFormat ));
                pHookInfo->pHook->OnInit(&audioFormat);
                // we do not allow these hooks to change the format on us
                HX_ASSERT(audioFormat.uChannels             == m_ActualDeviceFmt.uChannels &&
                          audioFormat.uBitsPerSample == m_ActualDeviceFmt.uBitsPerSample &&
                          audioFormat.ulSamplesPerSec        == m_ActualDeviceFmt.ulSamplesPerSec &&
                          audioFormat.uMaxBlockSize  == m_ActualDeviceFmt.uMaxBlockSize);
            }
        }
    }

#ifdef HELIX_FEATURE_AUDIO_DEVICE_HOOKS
    if(m_pAudioDevHookList)
    {
        CHXSimpleList::Iterator ndx = m_pAudioDevHookList->Begin();
        for (; ndx != m_pAudioDevHookList->End(); ++ndx)
        {
            HXAudioDevHookEntry* pHookInfo = (HXAudioDevHookEntry*) (*ndx);

            if (HXR_OK == ProcessAudioHook(ACTION_CHECK, pHookInfo->pHook))
            {
                HXAudioFormat audioFormat;
                memcpy( &audioFormat, &m_ActualDeviceFmt, sizeof( HXAudioFormat ));
                pHookInfo->pHook->OnInit(&audioFormat);
                // we do not allow these hooks to change the format on us
                HX_ASSERT(audioFormat.uChannels     == m_ActualDeviceFmt.uChannels &&
                          audioFormat.uBitsPerSample  == m_ActualDeviceFmt.uBitsPerSample &&
                          audioFormat.ulSamplesPerSec == m_ActualDeviceFmt.ulSamplesPerSec &&
                          audioFormat.uMaxBlockSize   == m_ActualDeviceFmt.uMaxBlockSize);
            }
        }
    }
#endif
}

void
CHXAudioSession::ProcessHooks(HXAudioData* pAudioData)
{
#if defined(HELIX_FEATURE_AUDIO_POSTMIXHOOK)
    HX_RESULT theErr = HXR_OK;
    CHXSimpleList::Iterator ndx = m_pHookList->Begin();
    for (; ndx != m_pHookList->End(); ++ndx)
    {
        HXAudioHookInfo* pHookInfo = (HXAudioHookInfo*) (*ndx);

        if (HXR_OK == ProcessAudioHook(ACTION_CHECK, pHookInfo->pHook))
        {
            // XXXHP, disable hooks when it doesn't support multi-channel
            if (m_ActualDeviceFmt.uChannels <= 2 || pHookInfo->bMultiChannelSupport)
            {
                m_pOutDataPtr->pData        = NULL;
                m_pOutDataPtr->ulAudioTime  = pAudioData->ulAudioTime;
                m_pOutDataPtr->uAudioStreamType = pAudioData->uAudioStreamType;

                theErr = pHookInfo->pHook->OnBuffer(pAudioData, m_pOutDataPtr);
                if (!theErr && m_pOutDataPtr->pData)
                {
                    HX_RELEASE(pAudioData->pData);
                    m_pSessionBuf = pAudioData->pData = m_pOutDataPtr->pData;
                }
            }
        }
        else if (pHookInfo->bIgnoreAudioData)
        {
            m_pOutDataPtr->pData        = NULL;
            m_pOutDataPtr->ulAudioTime  = pAudioData->ulAudioTime;
            m_pOutDataPtr->uAudioStreamType = pAudioData->uAudioStreamType;

            IHXBuffer* pTmpBuffer = pAudioData->pData;
            pAudioData->pData = NULL;
            theErr = pHookInfo->pHook->OnBuffer(pAudioData, m_pOutDataPtr);
            pAudioData->pData = pTmpBuffer;
        }
    }
#endif /* HELIX_FEATURE_AUDIO_POSTMIXHOOK */
}

double
CHXAudioSession::ConvertMsToBytes(UINT32 ulCurrentTime)
{
    UINT32 ulBytesPerSecond = m_ActualDeviceFmt.uChannels *
        (m_ActualDeviceFmt.uBitsPerSample / 8) *
        m_ActualDeviceFmt.ulSamplesPerSec;

    return (double)((ulCurrentTime * 1.0 / 1000) * ulBytesPerSecond);
}


HXBOOL
CHXAudioSession::IsAudioOnlyTrue(void)
{
    CHXSimpleList::Iterator lIter = m_pPlayerList->Begin();
    for (; lIter != m_pPlayerList->End(); ++lIter)
    {
        CHXAudioPlayer* pPlayer = (CHXAudioPlayer*) (*lIter);

        /* If anyone of them is TRUE, return TRUE */
        if (pPlayer->IsAudioOnlyTrue())
        {
            return TRUE;
        }
    }

    return FALSE;
}

void
CHXAudioSession::StopAllOtherPlayers()
{
    if (m_pContext)
    {
        IHXShutDownEverything* pShutdown;

        if (m_pContext->QueryInterface(IID_IHXShutDownEverything, (void**)&pShutdown) == HXR_OK)
        {
            HX_ASSERT(pShutdown);

            pShutdown->StopAllOtherPlayers();

            pShutdown->Release();
        }

    }

}

void
CHXAudioSession::CheckIfLastNMilliSecsToBeStored()
{
    if (!m_pPlayerList)
    {
        return;
    }

    HXBOOL bToBeStored = FALSE;
#ifdef HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
    UINT16 uNumPlayersWithStreams = 0;
#endif
    CHXSimpleList::Iterator ndx = m_pPlayerList->Begin();
    for (; ndx != m_pPlayerList->End(); ++ndx)
    {
        CHXAudioPlayer* pPlayer = (CHXAudioPlayer*) (*ndx);

        if (pPlayer->IsLastNMilliSecsToBeStored())
        {
            bToBeStored = TRUE;
            break;
        }

#ifdef HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
        if (!GetDisableMultiPlayPauseSupport() &&
            pPlayer->GetStreamCount() > 0)
        {
            uNumPlayersWithStreams++;
            if (uNumPlayersWithStreams > 1)
            {
                bToBeStored = TRUE;
                break;
            }
        }
#endif
    }

    ndx = m_pPlayerList->Begin();
    for (; ndx != m_pPlayerList->End(); ++ndx)
    {
        CHXAudioPlayer* pPlayer = (CHXAudioPlayer*) (*ndx);
        // pushdown + additional 500 ms...why? i don't know! ;)
        pPlayer->SaveLastNMilliSeconds(bToBeStored, m_ulTargetPushdown+500);
    }
}

void
CHXAudioSession::RewindSession(CHXAudioPlayer* pPlayerToExclude)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::RewindSession() START: m_dBufEndTime: %f  m_ulCurrentTime: %lu",
            this,
            (float)m_dBufEndTime,
            (UINT32)m_ulCurrentTime
            );
    
    m_ulLastRewindTimestamp = HX_GET_TICKCOUNT();

    INT32  lTimeToRewind   = 0;
    UINT32 ulCurTime        = GetCurrentPlayBackTime();

    // Close the audio device only if all players are not playing.
    if (m_pAudioDev)
    {
        m_pAudioDev->Reset();
    }

    m_uAskFromAudioDevice   = 5;

    /* We should have always be writing AHEAD*/
    HX_ASSERT((ULONG32) m_dBufEndTime >= ulCurTime);

    m_bStoppedDuringPause = TRUE;

    lTimeToRewind = (INT32) (((UINT32) m_dBufEndTime) - ulCurTime);
    RewindAllPlayers(ulCurTime, lTimeToRewind, pPlayerToExclude);

    if (m_ulCallbackID )
    {
        m_pScheduler->Remove(m_ulCallbackID);
        m_ulCallbackID = 0;
    }

    m_bFirstPlayAudio         = TRUE;
    m_bTimeSyncReceived       = FALSE;
    m_bAtLeastOneTimeReceived = FALSE;
    m_dBufEndTime             = m_ulCurrentTime;
    m_ulStartTime             = m_ulCurrentTime;
    m_dNumBytesPlayed         = (double) 0;
    m_ulBlocksWritten         = 0;
    m_dNumBytesWritten        = 0;
    m_ulLastAudioTime         = 0;
    
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::RewindSession() END: m_dBufEndTime: %f  m_ulCurrentTime: %lu",
            this,
            (float)m_dBufEndTime,
            (UINT32)m_ulCurrentTime
            );

}

void
CHXAudioSession::RewindAllPlayers(UINT32 ulCurTime, INT32 lTimeToRewind, CHXAudioPlayer* pPlayerToExclude)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::RewindAllPlayers() ulCurTime: %lu  lTimeToRewind: %ld",
            this, ulCurTime, lTimeToRewind);

    CHXSimpleList::Iterator ndx = m_pPlayerList->Begin();
    for (; ndx != m_pPlayerList->End(); ++ndx)
    {
        CHXAudioPlayer* pPlayer = (CHXAudioPlayer*) (*ndx);
        if (pPlayer != pPlayerToExclude)
        {
            pPlayer->RewindPlayer(lTimeToRewind);
        }
    }
}

HX_RESULT
CHXAudioSession::Rewind()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::Rewind()", this);

    HX_RESULT theErr = HXR_OK;

    m_bToBeRewound = FALSE;

#ifdef HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE
    /*
     * we need to re-mix to start the newly resumed player instantly
     * and not delay it by pushdown
     */
    if (!GetDisableMultiPlayPauseSupport())
    {
        RewindSession();
        if (NumberOfResumedPlayers() > 0)
        {
            theErr = ActualResume();
        }
    }
#endif

    return theErr;
}

HXBOOL CHXAudioSession::ReallyNeedData()
{
    UINT32 nBlocksRemaining = GetBlocksRemainingToPlay();
    HXBOOL bNeedData = (nBlocksRemaining <= m_ulMinBlocksTobeQueued) ? TRUE : FALSE;
    return (m_bSessionBufferDirty || bNeedData) ? TRUE : FALSE;
}

HXBOOL CHXAudioSession::GoingToUnderflow()
{
    HXBOOL bRetVal          = FALSE;
    UINT32 nBlocksRemaining = 0;
    if( !m_bFakeAudioTimeline && m_pAudioDev && !m_bPaused && !m_bStoppedDuringPause)
    {
        nBlocksRemaining = GetBlocksRemainingToPlay();

	HXLOGL4(HXLOG_ADEV, "CHXAudioSession[%p]::GoingToUnderflow() HaveBlocks=%lu MiNeeded=%lu", this, nBlocksRemaining, m_ulMinBlocksTobeQueued);

        bRetVal = (nBlocksRemaining < m_ulMinBlocksTobeQueued) ? TRUE : FALSE;
        if( bRetVal )
        {
            HXLOGL2( "IsRebufferRequired: BlockRemaining: %d  BlockToStart: %lu",
                     nBlocksRemaining,
                     m_ulMinBlocksTobeQueued
                     );
        }
    }
    return bRetVal;
}

HXBOOL CHXAudioSession::GetDisableMultiPlayPauseSupport()
{
    if(!m_pMPPSupport)
        return FALSE;

    return m_pMPPSupport->GetDisableMultiPlayPauseSupport();
}

void
CHXAudioSession::ReopenDevice()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioSession[%p]::ReopenDevice()", this);

#ifndef _CARBON
    if (m_bToBeReOpened &&
        m_pInterruptState && !m_pInterruptState->AtInterruptTime())
    {
        m_pDeviceCallback->PendingID(m_pScheduler->RelativeEnter(m_pDeviceCallback, 0));
        return;
    }
#endif

    if (m_bToBeReOpened)
    {
        TryOpenAudio();

        if (!m_bToBeReOpened && m_pAudioDev && m_bDeferActualResume)
        {
            ActualResume();
        }
    }
}



//
// HXDeviceSetupCallback

CHXAudioSession::HXDeviceSetupCallback::HXDeviceSetupCallback(CHXAudioSession* it) :
    m_pAudioSessionObject(it)
    ,m_ulCallbackID(0)
    ,m_lRefCount (0)
{
}

CHXAudioSession::HXDeviceSetupCallback::~HXDeviceSetupCallback()
{
    HX_ASSERT(m_ulCallbackID==0);
    m_pAudioSessionObject = NULL;
}

STDMETHODIMP CHXAudioSession::HXDeviceSetupCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList  qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXInterruptOnly), (IHXInterruptOnly*) this },
            { GET_IIDHANDLE(IID_IHXCallback),  (IHXCallback*) this },
	    { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*) this }
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXAudioSession::HXDeviceSetupCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXAudioSession::HXDeviceSetupCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CHXAudioSession::HXDeviceSetupCallback::Func(void)
{
    m_ulCallbackID = 0;
    if (m_pAudioSessionObject)
    {
        m_pAudioSessionObject->ReopenDevice();
    }
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) CHXAudioSession::HXDeviceSetupCallback::IsInterruptOnly()
{
    return TRUE;
}

HXBOOL CHXAudioSession::SetOpaqueMode( const char * pszOpaqueType, IHXBuffer * pOpaqueData )
{
    if( m_bOpaqueMode || !pszOpaqueType || !pOpaqueData )
        return FALSE;

    m_bOpaqueMode = TRUE;
    m_strOpaqueType = pszOpaqueType;
    m_pOpaqueData = pOpaqueData;
    m_pOpaqueData->AddRef();

    return TRUE;
}


#if defined(HELIX_FEATURE_TIMELINE_WATCHER)
HX_RESULT CHXAudioSession::AddTimelineWatcher( IHXTimelineWatcher* pWatcher )
{
    HX_RESULT res = HXR_FAIL;

    if( !m_pTimeLineWatchers )
    {
        m_pTimeLineWatchers = new CHXSimpleList();
        if( !m_pTimeLineWatchers )
        {
            return HXR_OUTOFMEMORY;
        }
    }
    
    
    LISTPOSITION lPosition = m_pTimeLineWatchers->Find(pWatcher);
    if(!lPosition)
    {
        res = HXR_OUTOFMEMORY;
        if( m_pTimeLineWatchers->AddTail(pWatcher) )
        {
            pWatcher->AddRef();
            res = HXR_OK;
        }
    }
    
    return res;
}

HX_RESULT CHXAudioSession::RemoveTimelineWatcher( IHXTimelineWatcher* pWatcher )
{
    HX_RESULT res = HXR_FAIL;

    if( m_pTimeLineWatchers )
    {
        LISTPOSITION lPosition = m_pTimeLineWatchers->Find(pWatcher);
        if(lPosition)
        {
            m_pTimeLineWatchers->RemoveAt(lPosition);
            HX_RELEASE(pWatcher);
            res = HXR_OK;
        }
    }
    return res;
}

void CHXAudioSession::_NotifyTimelineWatchers( const int nWhich, const UINT32 ulNow)
{
    if( m_pTimeLineWatchers )
    {
        
        LISTPOSITION pos = m_pTimeLineWatchers->GetHeadPosition();
        while( pos )
        {
            IHXTimelineWatcher* pWatcher = (IHXTimelineWatcher*)m_pTimeLineWatchers->GetNext(pos);
            switch( nWhich )
            {
               case TLW_PAUSE:
                   pWatcher->OnPause();
                   break;
               case TLW_RESUME:
                   pWatcher->OnResume();
                   break;
               case TLW_CLOSE:
                   pWatcher->OnClose();
                   break;
               case TLW_TIMESYNC:
                   pWatcher->OnTimeSync(ulNow);
                   break;
               default:
                   HX_ASSERT("Unknown timeline watcher action."==NULL);
            }
        }
    }
}
#endif

void CHXAudioSession::updateFakeTimeline()
{
    /* Get current system time. */
    ULONG32 ulCurrentSysTime = HX_GET_TICKCOUNT();
    m_ulIncreasingTimer += CALCULATE_ELAPSED_TICKS(m_ulLastFakeCallbackTime, 
                                                   ulCurrentSysTime);
    m_ulLastFakeCallbackTime = ulCurrentSysTime;
}

void CHXAudioSession::updateCurrentTime(ULONG32 ulCurrentTime)
{
    /* Adjust current time to new larger time. */
    ULONG32 ulNewCurrentTime = ulCurrentTime + m_ulStartTime;
    if (m_ulCurrentTime < ulNewCurrentTime)
    {
        /* Check for a messed up time here...
         * Sometimes, Audio cards may give out some insane value for
         * the number of bytes played...
         */
        if (CALCULATE_ELAPSED_TICKS(m_ulCurrentTime, 
                                    ulNewCurrentTime) >
            SOME_INSANELY_LARGE_VALUE)
        {
            /* THIS SOUND CARD IS GOING CRAZY...
             * ATLEAST FOR A WHILE....
             * Ignore this timesync...
             */
        }
        else
        {
            m_ulCurrentTime = ulNewCurrentTime;
        }
    }
}
