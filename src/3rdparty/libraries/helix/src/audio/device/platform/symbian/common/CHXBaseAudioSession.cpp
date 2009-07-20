/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: CHXBaseAudioSession.cpp,v 1.4 2007/05/02 16:25:47 praveenkumar Exp $
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
 * Contributor(s): NOkia Inc
 * 
 * ***** END LICENSE BLOCK ***** */

#if defined (HELIX_CONFIG_CALYPSO_AUDIO_PREF)
#include <calypso/audiopreference.h>
#endif

#include <hal.h> // device id
#include <e32std.h>

#include "hxcom.h"
#include "hxslist.h"
#include "hxausvc.h"
#include "hxbuffer.h"
#include "hxtick.h"
#include "debug.h"
#include "hxtlogutil.h"
#include "CHXBaseAudioSession.h"
#include "CHXMMFDevSound.h"
#include "symbian_gm_inst.h"
#if defined(HELIX_FEATURE_DRM_SECURE_OUTPUT)
  #include <AudioOutput.h>
#endif


inline
TUint SamplesToMS(TUint count, TUint rate)
{
    HX_ASSERT(rate != 0);
    INT64 lValue = ( ((INT64)count) * 1000) / rate;
    return (INT64_TO_UINT32(lValue));
}

#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
#define HELIX_LOCAL_FEATURE_DEBUG_LOG
#endif

#if defined(HELIX_LOCAL_FEATURE_DEBUG_LOG)
// stringify constants for easier interpretation of trace output
#define MAKE_STATE_ENTRY(x) case CHXBaseAudioSession::x: return #x;
const char * StringifyState(CHXBaseAudioSession::State state)
{
    switch (state)
    {
        MAKE_STATE_ENTRY(CLOSED)
        MAKE_STATE_ENTRY(OPEN_PENDING)
        MAKE_STATE_ENTRY(STOPPED)
        MAKE_STATE_ENTRY(PLAYINIT_PENDING)
        MAKE_STATE_ENTRY(PLAYING)
        MAKE_STATE_ENTRY(PAUSED)
    }
    return "UNKNOWN_STATE";
}
#define MAKE_KERR_ENTRY(x) case x: return #x;

const char * StringifyKErr(TInt lRetval) 
{ 
    switch (lRetval)
    {
        // error codes from e32std.h
        MAKE_KERR_ENTRY(KErrNone)
        MAKE_KERR_ENTRY(KErrNotFound)
        MAKE_KERR_ENTRY(KErrGeneral)
        MAKE_KERR_ENTRY(KErrCancel)
        MAKE_KERR_ENTRY(KErrNoMemory)
        MAKE_KERR_ENTRY(KErrNotSupported)
        MAKE_KERR_ENTRY(KErrArgument)
        MAKE_KERR_ENTRY(KErrTotalLossOfPrecision)
        MAKE_KERR_ENTRY(KErrBadHandle)
        MAKE_KERR_ENTRY(KErrOverflow)
        MAKE_KERR_ENTRY(KErrUnderflow)
        MAKE_KERR_ENTRY(KErrAlreadyExists)
        MAKE_KERR_ENTRY(KErrPathNotFound)
        MAKE_KERR_ENTRY(KErrDied)
        MAKE_KERR_ENTRY(KErrInUse)
        MAKE_KERR_ENTRY(KErrServerTerminated)
        MAKE_KERR_ENTRY(KErrServerBusy)
        MAKE_KERR_ENTRY(KErrCompletion)
        MAKE_KERR_ENTRY(KErrNotReady)
        MAKE_KERR_ENTRY(KErrUnknown)
        MAKE_KERR_ENTRY(KErrCorrupt)
        MAKE_KERR_ENTRY(KErrAccessDenied)
        MAKE_KERR_ENTRY(KErrLocked)
        MAKE_KERR_ENTRY(KErrWrite)
        MAKE_KERR_ENTRY(KErrDisMounted)
        MAKE_KERR_ENTRY(KErrEof)
        MAKE_KERR_ENTRY(KErrDiskFull)
        MAKE_KERR_ENTRY(KErrBadDriver)
        MAKE_KERR_ENTRY(KErrBadName)
        MAKE_KERR_ENTRY(KErrCommsLineFail)
        MAKE_KERR_ENTRY(KErrCommsFrame)
        MAKE_KERR_ENTRY(KErrCommsOverrun)
        MAKE_KERR_ENTRY(KErrCommsParity)
        MAKE_KERR_ENTRY(KErrTimedOut)
        MAKE_KERR_ENTRY(KErrCouldNotConnect)
        MAKE_KERR_ENTRY(KErrCouldNotDisconnect)
        MAKE_KERR_ENTRY(KErrDisconnected)
        MAKE_KERR_ENTRY(KErrBadLibraryEntryPoint)
        MAKE_KERR_ENTRY(KErrBadDescriptor)
        MAKE_KERR_ENTRY(KErrAbort)
        MAKE_KERR_ENTRY(KErrTooBig)
        MAKE_KERR_ENTRY(KErrDivideByZero)
        MAKE_KERR_ENTRY(KErrBadPower)
        MAKE_KERR_ENTRY(KErrDirFull)
        MAKE_KERR_ENTRY(KErrHardwareNotAvailable)
        MAKE_KERR_ENTRY(KErrSessionClosed)
        MAKE_KERR_ENTRY(KErrPermissionDenied)
    }
    return "{e32std error}";
}

#else
// do nothing (compile out) when !HELIX_LOCAL_FEATURE_DEBUG_LOG
const char * StringifyState(CHXBaseAudioSession::State state) { return 0; }
const char * StringifyKErr(TInt lRetval) { return 0; }
#endif


TMMFSampleRate NumberToFlag(TInt num)
{
    switch(num)
    {
    case 8000:
        return  EMMFSampleRate8000Hz;
    case 11025:
        return  EMMFSampleRate11025Hz;
    case 16000:
        return  EMMFSampleRate16000Hz;
    case 22050:
        return  EMMFSampleRate22050Hz;
    case 32000:
        return  EMMFSampleRate32000Hz;
    case 44100:
        return  EMMFSampleRate44100Hz;
    case 48000:
        return  EMMFSampleRate48000Hz;
	case 88200:
		return EMMFSampleRate88200Hz;
	case 96000:
		return EMMFSampleRate96000Hz;
	case 12000:
		return EMMFSampleRate12000Hz;
	case 24000:
		return EMMFSampleRate24000Hz;
	case 64000:
		return EMMFSampleRate64000Hz;
    default:
        break;
    }
    HX_ASSERT(FALSE);
    return EMMFSampleRate16000Hz;
}

CHXBaseAudioSession::CHXBaseAudioSession()
    :m_lastPlayError(KErrNone),
    m_pStream(NULL),
    m_sampleRate(0),
    m_cbSample(0),
    m_cbBlock(0),
    m_cbBufferList(0),
    m_state(CLOSED),
    m_deviceResetsOnPause(FALSE),
    m_prioritySettings(),
    m_bSecureAudio(FALSE),
    m_pPendingFillBuffer(NULL),
    m_cbFrontBufferWritten(0),
    m_samplesWritten(0),
    m_bDevSoundInitErr(FALSE),
    m_bDevSoundOwned(TRUE),
#ifdef _DEBUG_TEST_AUDIO_DEVICE_TAKEN_
    m_lErrCnt(1),
#endif 
#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
    m_lastSampleCount(0),
    m_unplayedSampleCount(0),
    m_sampleCountResetPending(FALSE),
    m_resetTriggerSampleCount(0),
    m_resetTriggerUnplayedCount(0),
    m_ulTimePlayedInSec(0),
    m_ulBalanceSamples(0),
    m_cbPastBufferList(0)
#else
    m_TotalSamplesWritten(0)
#endif
{
    memset(&m_Settings, 0, sizeof( m_Settings) );

    // Do run-time check for devices (6630) that are known to do a device
    // reset after a pause. Other devices (OMAP-based in particular) may
    // need to be added here.
    const TInt DEVICE_UID_6630 = 0x101fbb55; // emulator = 0x10005f62
    const TInt DEVICE_UID_6680 = 0x10200f99; 

    TInt uid = 0;
    TInt res = HAL::Get(HALData::EMachineUid, uid);
    if (KErrNone == res)
    {
        if (DEVICE_UID_6630 == uid || DEVICE_UID_6680 == uid )
        {
            m_deviceResetsOnPause = TRUE;
        }
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::CHXBaseAudioSession(): symbian device uid = 0x%08x", uid);
    }
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    //
    // Enabled by default. This case will handle reset of samplesPlayed count 
    // when stream is paused and played again. Play after Pause will call 
    // PlayInit instead of PlayData.
    // 
    m_deviceResetsOnPause = TRUE;
#endif
}

//
// CHXBaseAudioSession::dtor
//
CHXBaseAudioSession::~CHXBaseAudioSession()
{
    Trans(CLOSED);
}

TInt CHXBaseAudioSession::CreateDevSound()
{
    TInt lRetval = KErrNone;
    CHXMMFDevSound* pHxDevSound = NULL;
    
    // check for the existence of global devsound
    pHxDevSound = CHXMMFDevSound::Get();
    if (pHxDevSound == NULL)
    {
        TRAP(lRetval, (m_pStream = CMMFDevSound::NewL()));
        if(lRetval == KErrNone)
        {
            TRAP(lRetval, (m_pStream->InitializeL(*this, EMMFStatePlaying)));
        }
    }
    else
    {
        m_bDevSoundOwned = FALSE;
        m_pStream = pHxDevSound->DevSound();
        // Assumption is that the initilization is already completed
        // CHXMMFDevSound is initialized already during creation
        // So we just need to set observer and call overselves initialized
        pHxDevSound->RegisterObserver(this);
        InitializeComplete(KErrNone);
        lRetval = m_lastPlayError;
    }
    
    HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::CreateDevSound(): DevSound:%x Owned:%d",
            m_pStream, m_bDevSoundOwned);
    return lRetval;
}


//
// CHXBaseAudioSession::Init
//
// 1. Matches input sample rate to output sample rate
//    by building a sample rate converter, if necessary.
// 2. Opens and initializes the audio device.
//
TInt CHXBaseAudioSession::InitAudio(AudioDeviceInitArgs* pAudioDevInitArgs)
{
    TInt lRetval = KErrGeneral;

    // we always do 16 bit samples
    const TUint bitsPerSample   = 16;
    TUint channelCount          = 1;
    
    if (pAudioDevInitArgs != NULL)
    {
        m_sampleRate   = pAudioDevInitArgs->ulSampleRate;
        channelCount   = pAudioDevInitArgs->uChannels;
        if (pAudioDevInitArgs->pPrioritySettings != NULL)
        {
            m_prioritySettings.iPriority = pAudioDevInitArgs->pPrioritySettings->iPriority;
            m_prioritySettings.iPref     = pAudioDevInitArgs->pPrioritySettings->iPref;
        }
        m_bSecureAudio = pAudioDevInitArgs->bSecureAudio;
    }

    m_cbSample                  = channelCount * (bitsPerSample / 8);

    // translate the audio props to flags needed by interface
    m_Settings.iRate     = NumberToFlag(m_sampleRate);
    m_Settings.iChannels = channelCount;
    m_Settings.iEncoding = EMMFSoundEncoding16BitPCM;
    
    HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::Init(): rate = %ld; chan = %ld; sample frame = %lu bytes", m_sampleRate, m_Settings.iChannels, m_cbSample);
    
    switch (m_state)
    {
    case STOPPED:
        {
            HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::Init(): calling SetConfgL() with settings");
            TRAP(lRetval, (m_pStream->CMMFDevSound::SetConfigL(m_Settings)));
            break;
        }
    case CLOSED:
        {
            Trans(OPEN_PENDING);
            HX_ASSERT(!m_pStream);
            lRetval = CreateDevSound();
            if( KErrNone != lRetval )
            {
                Trans(CLOSED);
            }
            break;
        }
    default:
        {
            HX_ASSERT(FALSE);
            break;
        }
    }; // End of switch (m_state)

    return lRetval;
}

//
// CHXBaseAudioSession::Play
//
TInt CHXBaseAudioSession::PlayAudio()
{
    HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::PlayAudio(): state = %s", StringifyState(m_state));
    TInt lRetval = KErrNone;

    switch (m_state)
    {
    case STOPPED:
        DoPlayInit();
        break;
    case PAUSED:
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
        DoPlayInit();
#else
        Trans(PLAYINIT_PENDING);
        if (m_pPendingFillBuffer)
        {
            HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::Play(): calling BufferToBeFilled()");
            BufferToBeFilled(m_pPendingFillBuffer);
            HX_ASSERT(!m_pPendingFillBuffer); // write list empty?
        }
        else
        {
            m_pStream->PlayData();
        }
#endif
        break;
    case PLAYINIT_PENDING:
    case PLAYING:
        // do nothing
        break;
    default:
        // unexpected
        HX_ASSERT(FALSE);
        lRetval = KErrGeneral;
        break;
    }

    return lRetval;
}

//
// CHXBaseAudioSession::Pause
//
TInt CHXBaseAudioSession::PauseAudio()
{
    HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::PauseAudio(): state = %s", StringifyState(m_state));
    TInt lRetval = KErrNone;

    switch (m_state)
    {
    case PLAYING:
    case PLAYINIT_PENDING:
        Trans(PAUSED);
        if(m_deviceResetsOnPause)
        {
            // The device resets on pause.
            // Unplayed samples played count are claimed to be played.
            m_pStream->Stop();
        }
        else
        {
            // On this case we rely on DevSound completely for SamplesPlayed count.
            // We don't adjust any unplayed samples count.
            m_pStream->Pause();
        }
        break;
    case STOPPED:
        // note: pause called immediately after init by higher level code
        break;
    case PAUSED:
        // do nothing
        break;
    default:
        HX_ASSERT(FALSE);
        lRetval = KErrGeneral;
        break;
    }

    return lRetval;
}

//
// CHXBaseAudioSession::Write
//
//
TInt CHXBaseAudioSession::WriteAudio(IHXBuffer* pAudioBuf)
{
    TInt lRetval = KErrNone;

    HXBOOL checkPendingFillBuffer = FALSE;

    switch (m_state)
    {
    case STOPPED:
    case PAUSED:
    case PLAYINIT_PENDING:
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::WriteAudio(): state = %s", StringifyState(m_state));
        break;
    case PLAYING:
        checkPendingFillBuffer = TRUE;
        break;
    default:
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::WriteAudio(): called in unexpected state %s ", StringifyState(m_state));
        HX_ASSERT(FALSE);
        lRetval = KErrGeneral;
        break;
    }

    if (KErrNone == lRetval)
    {
        if (pAudioBuf)
        {
            if (0 == m_cbBlock)
            {
                // remember audio block size (audio buffs should all be the same size)
                m_cbBlock = pAudioBuf->GetSize();
            }
            HX_ASSERT(m_cbBlock == pAudioBuf->GetSize());

            if (m_bufferList.AddTail(pAudioBuf))
            {
                m_cbBufferList += pAudioBuf->GetSize();
            }
            else
            {
                lRetval = KErrNoMemory;
            }
        }
        else
        {
            lRetval = KErrArgument;
        }
    }

    if (KErrNone == lRetval && checkPendingFillBuffer && m_pPendingFillBuffer)
    {
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::WriteAudio(): calling BufferToBeFilled()");
        BufferToBeFilled(m_pPendingFillBuffer);
        HX_ASSERT(!m_pPendingFillBuffer);
    }

    return lRetval;
}
    

//
// CHXBaseAudioSession::GetTime
//
TInt CHXBaseAudioSession::GetAudioTime()
{

    TInt lCurrTime = 0;

    // Check whether previous PlayInit to DevSound had Recoverable error
    // For now only KErrServerBusy is handled as recoverable error
    if(m_bDevSoundInitErr)
    {
       HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::GetAudioTime(): Previous PlayInit Failed. Retrying....\n");
       TRAPD(lRetval, m_pStream->PlayInitL());
       HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::GetAudioTime(): PlayInit RetVal::%d\n", lRetval);
       if(lRetval != KErrNone)
       {
           if(lRetval != KErrServerBusy)
           {
               Trans(STOPPED);
           }
       }
       else
       {
          m_bDevSoundInitErr = FALSE;
       }
       
    } // End of if(m_bDevSoundInitErr)

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
    UpdateTimeSampleStats();
    lCurrTime = GetSamplesPlayedTimeInMS();
#else
    lCurrTime = SamplesToMS(m_TotalSamplesWritten, m_sampleRate);
#endif

#ifdef _DEBUG_TEST_AUDIO_DEVICE_TAKEN_
    if(lCurrTime > (m_lErrCnt * ERROR_TIMER_VAL_IN_MILLISECS) )
    {
        m_lErrCnt++;
        m_pStream->Stop();
        PlayError(KErrDied);
    }
#endif

    return lCurrTime;
 
}


//
// CHXBaseAudioSession::GetUnPlayedBlocks
//
// Return the number of blocks buffered by this object (pending
// writes plus data buffered in device)
//
TInt CHXBaseAudioSession::GetUnPlayedBlocks()
{
    // determine bytes buffered in actual device
    
    HX_ASSERT(m_cbSample != 0);
    UINT32 cbBuffered = 0;

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
    UpdateTimeSampleStats();
    cbBuffered = m_unplayedSampleCount * m_cbSample;
#else
    // DevSound's Samples played is not used. The calculation is based on
    // what is being written to DevSound rather than what is reported as 
    // played.
#endif
    // add in bytes we are holding in our buffer list
    HX_ASSERT(m_cbBufferList >= m_cbFrontBufferWritten);
    cbBuffered += m_cbBufferList - m_cbFrontBufferWritten;
    
    // convert bytes to block count (rounded up)
    TUint32 blockCount = 0;
    if( m_cbBlock != 0 )
    {
        blockCount = cbBuffered/m_cbBlock + 1;
    }

    HXLOGL4(HXLOG_ADEV, "CHXBaseAudioSession::GetUnPlayedBlocks(): block count = %lu (%lu bytes total; %lu in list)", blockCount, cbBuffered, m_cbBufferList);

    return blockCount;

}

//
// CHXBaseAudioSession::SetVol
//
// set the volume -- convert from 0-100 to 0-max range
//
void CHXBaseAudioSession::SetVol(TInt lVolume)
{
    HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::SetVol(): %x Volume = %d", m_pStream, lVolume);
    if (m_pStream)
    {
        m_pStream->SetVolume(lVolume);
    }
}

//
// CHXBaseAudioSession::Volume
//
// get the current volume normalized to 0-100 range
//
TInt CHXBaseAudioSession::Volume() const
{
    TInt lVolume = 0;
    if (m_pStream)
    {
        lVolume = m_pStream->Volume();
    }
    return lVolume;
}

//
// CHXBaseAudioSession::MaxVolume
//
// get the maxium device volume
//
TInt CHXBaseAudioSession::MaxVolume() const
{
    TInt lMaxVol = 0;
    if (m_pStream)
    {
        lMaxVol = m_pStream->MaxVolume();
    }

    return lMaxVol;
}

//
// CHXBaseAudioSession::MinVolume
//
// get the minimum device volume
//
TInt CHXBaseAudioSession::MinVolume() const
{
    return 0;
}


//
// CHXBaseAudioSession::Stop
//
//
TInt CHXBaseAudioSession::StopAudio()
{
    HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::StopAudio(): state = %s", StringifyState(m_state));
    switch (m_state)
    {
    case PLAYING:
    case PLAYINIT_PENDING:
    case PAUSED:
        Trans(STOPPED);
        m_pStream->Stop();
        // The control is made to fall through intentionally.
    case STOPPED:
        // Case handling when session is already in STOPPED state but the 
        // resupply data list is not cleaned up. (DevTaken moves the session to STOPPED state
        // but the queue was not cleaned up) This resulted in data replay. hence the following
        // handling

        // do additional stuff associated with user stop
        FreePendingBuffers();
        
#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::Stop(): %lu ms unplayed in audio device", SamplesToMS(m_unplayedSampleCount, m_sampleRate));
        OnResetSampleCount();
        ResetSamplesPlayedTime();
#else
        m_TotalSamplesWritten = 0;
#endif
        break;
    default:
        // nothing
        break;
    }

    return KErrNone;
}


void CHXBaseAudioSession::Trans(CHXBaseAudioSession::State state)
{
    if (state != m_state)
    {
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::Trans(): %s -> %s", StringifyState(m_state), StringifyState(state));
        m_state = state;
        switch (m_state)
        {
        case STOPPED:
            InitStoppedState();
            break;
        case PLAYINIT_PENDING:
            InitPlayInitPendingState();
            break;
        case PAUSED:
            InitPausedState();
            break;
        case CLOSED:
            InitClosedState();
            break;
        default:
            // nothing
            break;
        }   
    }
}
    
void CHXBaseAudioSession::InitPlayInitPendingState()
{
    m_lastPlayError = KErrNone;
}



void CHXBaseAudioSession::InitPausedState()
{
    if (m_deviceResetsOnPause)
    {
        PrepareForDeviceReset();
#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
        RetrieveUnplayedSamplesFromPastQueue();
        m_resetTriggerUnplayedCount = m_unplayedSampleCount;
#endif
    }

}

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
void CHXBaseAudioSession::RetrieveUnplayedSamplesFromPastQueue()
{

    HX_ASSERT(m_cbPastBufferList + m_cbFrontBufferWritten >= m_unplayedSampleCount*m_cbSample );

    while( m_unplayedSampleCount*m_cbSample > m_cbFrontBufferWritten )
    {
        m_unplayedSampleCount -= m_cbFrontBufferWritten/m_cbSample;
        //m_samplesWritten has reset to 0 in PrepareForDeviceReset(), so do not set following
        //m_samplesWritten -= m_cbFrontBufferWritten/m_cbSample; 
        m_cbFrontBufferWritten = 0;

        IHXBuffer* pBuf = (IHXBuffer*)m_pastBufferList.RemoveTail();
        if( pBuf )
        {
            m_cbFrontBufferWritten =  pBuf->GetSize();
            m_cbPastBufferList -= m_cbFrontBufferWritten;
            m_bufferList.AddHead( pBuf );
            m_cbBufferList += m_cbFrontBufferWritten;
        }
        else
        {
            //it might be an overkill, it should not reach here.
            //however, in case it happens, it is handled.
            break;
        }
    }

    if( m_cbFrontBufferWritten )
    {
        HX_ASSERT( m_unplayedSampleCount*m_cbSample <= m_cbFrontBufferWritten );
        m_cbFrontBufferWritten -= m_unplayedSampleCount*m_cbSample;
        //m_samplesWritten has reset to 0 in PrepareForDeviceReset(), so do not set following
        //m_samplesWritten -= m_unplayedSampleCount;
        m_unplayedSampleCount = 0;
    }
}
#endif

void CHXBaseAudioSession::InitStoppedState()
{
    PrepareForDeviceReset();
}

void CHXBaseAudioSession::InitClosedState()
{
    FreePendingBuffers();

    if(m_bDevSoundOwned == TRUE)
    {
        HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::InitClosedState() Deleting DevSound :%x", m_pStream);
        HX_DELETE(m_pStream);
    }
    else
    {
        CHXMMFDevSound* pHxDevSound = CHXMMFDevSound::Get();
        
        HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::InitClosedState() Resetting Global DevSound :%x", pHxDevSound);
        HX_ASSERT(pHxDevSound);

        // CHXMMFDevvsound is not owned by AudioSession. So it needs to 
        // remove itself from observing callbacks
        pHxDevSound->UnRegisterObserver();
        m_pStream = NULL;
        m_bDevSoundOwned = TRUE;
    }
}

void CHXBaseAudioSession::FreePendingBuffers()
{
    while (!m_bufferList.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_bufferList.RemoveHead();
        HX_RELEASE(pBuf);
    }
    m_cbBufferList = 0;
    m_cbFrontBufferWritten = 0;

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
    ClearPastQueue();
#endif
}


void CHXBaseAudioSession::DoPlayInit(HXBOOL setPriority)
{
    HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::DoPlayInit(): state = %s", StringifyState(m_state));

    Trans(PLAYINIT_PENDING);

    if (setPriority)
    {
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::DoPlayInit(): setting priority...");
        static const TInt KClientPriority = 69;

        // setting priority: 
        // if the Priority and Pref values are invalid, set them to the default values
        // (Pref value must be > 0, else DevSound can't handle it.)
        if (m_prioritySettings.iPref <= 0)
        {
            m_prioritySettings.iPref = EMdaPriorityPreferenceTime;
        }
        if (m_prioritySettings.iPriority < 0)
        {
            m_prioritySettings.iPriority = KClientPriority;
        }
        m_prioritySettings.iState = EMMFStatePlaying;
        m_pStream->SetPrioritySettings(m_prioritySettings);
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::DoPlayInit(): priority set to (%u,%u)\n",
                m_prioritySettings.iPriority, m_prioritySettings.iPref);
    }

    // DevSound returns ServerBusy Error when Pause or Stop is immediately
    // followed by Play. DevSound waits for response from the AudioPolicy 
    // Server and PlayInit during that case reports the ServerBusy error.
    TRAPD(lRetval, m_pStream->PlayInitL());
    HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::DoPlayInit(): result = %s", StringifyKErr(lRetval));
    if (lRetval != KErrNone)
    {
        if(lRetval == KErrServerBusy)
        {
            // DevSound error KErrServerBusy is recoverable. 
            // Hence flag is marked for retry
            HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::DoPlayInit() PlayInit Failed. Mark flag for retry\n");
            m_bDevSoundInitErr = TRUE;
        }
        else
        {
           Trans(STOPPED);
        }
        
    } // End of if (lRetval != KErrNone)
}


// MDevSoundObserver
void CHXBaseAudioSession::InitializeComplete(TInt aError)
{
    HXLOGL2(HXLOG_ADEV, "CHXBaseAudioSession::InitializeComplete(): lRetval = %s", StringifyKErr(aError));
    if(aError == KErrNone)
    {
        TRAP(aError, (m_pStream->CMMFDevSound::SetConfigL(m_Settings)));
        Trans((KErrNone == aError) ? STOPPED : CLOSED);
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::InitializeComplete() SetConfgL(): lRetval = %s", StringifyKErr(aError));

#if defined(HELIX_FEATURE_DRM_SECURE_OUTPUT)
        // Checking if audio output needs to be secured
        if ((aError == KErrNone) && (m_bSecureAudio))
        {
            CAudioOutput* pAudioOutput = NULL;
            TRAP(aError, pAudioOutput = CAudioOutput::NewL(*m_pStream));
            if(aError == KErrNone)
            {
                TRAP(aError , pAudioOutput->SetSecureOutputL(ETrue));
                HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::InitializeComplete() SetSecureOutputL: lRetval = %s", StringifyKErr(aError));
            }
            HX_DELETE(pAudioOutput);
        }
#endif

    } // End of if(aError == KErrNone)

    m_lastPlayError = aError;
}

// MDevSoundObserver  
// Called whenever the media server needs more data to play, or when we have more audio
// data added and a previous fill request was not fullfilled.
void CHXBaseAudioSession::BufferToBeFilled(CMMFBuffer* aBuffer)
{
    // we are now actively playing
    switch (m_state)
    {
    case PLAYINIT_PENDING:
        Trans(PLAYING);
        // fall through
    case PLAYING:
        break;
    default:
        HX_ASSERT(FALSE);
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::BufferToBeFilled(): unexpected state %s", StringifyState(m_state));
        break;
    }
    
    HX_ASSERT(aBuffer);
 
    TDes8&  dataDesc  = ((CMMFDataBuffer*)aBuffer)->Data();
    dataDesc = TPtrC8(NULL, 0 ); // ensure descriptor is reset/clear
    TUint cbDest  = aBuffer->RequestSize();

    //HXLOGL4(HXLOG_ADEV, "CHXBaseAudioSession::BufferToBeFilled(): req size = %ld; dest buffer size = %lu; desc max = %lu; buffer list count = %ld", aBuffer->RequestSize(), aBuffer->BufferSize(), dataDesc.MaxSize(), m_bufferList.GetCount());

    while ( !m_bufferList.IsEmpty() && cbDest >= m_cbSample)
    {
        // get buffer at front
        IHXBuffer* pBuffer = (IHXBuffer*)m_bufferList.GetHead();
        HX_ASSERT(pBuffer);
        UINT32 cbBuffer = pBuffer->GetSize();

        // check buffer length
        HX_ASSERT(cbBuffer > 0);
        HX_ASSERT(cbBuffer <= KMaxTInt);

        // m_cbFrontBufferWritten = bytes already written from front buffer
        HX_ASSERT(m_cbFrontBufferWritten < cbBuffer);

        //HXLOGL4(HXLOG_ADEV, "CHXBaseAudioSession::BufferToBeFilled(): next src buffer size = %lu; front buffer written = %lu", cbBuffer, m_cbFrontBufferWritten);

        // decide how much to write; we may not be able to write a full buffer
        UINT32 cbToWrite = pBuffer->GetSize() - m_cbFrontBufferWritten;
        if (cbToWrite > UINT32(cbDest))
        {
            // limit amount to destination buffer space available
            cbToWrite = cbDest;
        }

        //buffers assumed to be frame-aligned
        HX_ASSERT(cbToWrite % m_cbSample == 0);

        // probably overkill: round write amount so we write only full
        // sample (src buffers assumed to be frame-aligned)
        cbToWrite = (cbToWrite/m_cbSample) * m_cbSample;
        HX_ASSERT(cbToWrite != 0);
        HX_ASSERT(cbToWrite % m_cbSample == 0);

        // copy
        TPtrC8 desc((TUint8*)pBuffer->GetBuffer() + m_cbFrontBufferWritten, TInt(cbToWrite));
        dataDesc.Append(desc);
        m_cbFrontBufferWritten += cbToWrite;
        cbDest -= cbToWrite;

        // keep track of how many full samples we write
        HX_ASSERT(0 == (cbToWrite % m_cbSample)); 
        TUint ulSamplesWritten = (cbToWrite / m_cbSample);
        m_samplesWritten += ulSamplesWritten;

#ifndef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
        m_TotalSamplesWritten += ulSamplesWritten;
#endif

        if (m_cbFrontBufferWritten == cbBuffer)
        {
            // we used up the front buffer; toss it
            IHXBuffer* pTmp = (IHXBuffer*)m_bufferList.RemoveHead();
            m_cbBufferList -= pTmp->GetSize();
            m_cbFrontBufferWritten = 0;

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
            if( m_deviceResetsOnPause )
            {
                m_pastBufferList.AddTail( pTmp );
                m_cbPastBufferList += pTmp->GetSize();
            }
            else
#endif
            {
                HX_RELEASE(pTmp);
            }
        }
    }

    //HXLOGL4(HXLOG_ADEV, "CHXBaseAudioSession::BufferToBeFilled(): frame no = %lu; pos = %lu; last = %s; ts = %lu; status = %lu", aBuffer->FrameNumber(), aBuffer->Position(), aBuffer->LastBuffer() ? "true" : "false", aBuffer->TimeToPlay().Int64().Low(), aBuffer->Status() );

    if (dataDesc.Length() > 0)
    {
        HXLOGL4(HXLOG_ADEV, "CHXBaseAudioSession::BufferToBeFilled(): play buff = %ld bytes (%lu samps); samps written = %lu", dataDesc.Length(), dataDesc.Length()/m_cbSample, m_samplesWritten);
        if (PLAYING == m_state)
        {
            m_pStream->PlayData();
        }
        m_pPendingFillBuffer = NULL;
    }
    else
    {
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::BufferToBeFilled(): unable to fill buffer (no src data)");
        // hold on to buffer; we'll fill it once we have more src data
        m_pPendingFillBuffer = aBuffer;
    }

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
    UpdateTimeSampleStats();
#endif

}

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
void CHXBaseAudioSession::RemoveOutdatedPacketsInPastQueue()
{
    IHXBuffer* pBuf( NULL );
    ULONG32    bufSize( 0 );

    while( !m_pastBufferList.IsEmpty() )
    {
        pBuf    = (IHXBuffer*)m_pastBufferList.GetHead();
        bufSize =  pBuf->GetSize();
        
        if( m_unplayedSampleCount*m_cbSample <= m_cbPastBufferList + m_cbFrontBufferWritten - bufSize )
        {
            pBuf = (IHXBuffer*)m_pastBufferList.RemoveHead();
            HX_RELEASE(pBuf);
            m_cbPastBufferList -= bufSize;
        }
        else
        {
            break;
        }
    }
}

void CHXBaseAudioSession::ClearPastQueue()
{
    while (!m_pastBufferList.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_pastBufferList.RemoveHead();
        HX_RELEASE(pBuf);
    }
    m_cbPastBufferList = 0;
}
#endif


// MDevSoundObserver
void CHXBaseAudioSession::PlayError(TInt aError)
{
    HXLOGL1(HXLOG_ADEV, "CHXBaseAudioSession::PlayError(%d): lRetval = %s", 
            aError, StringifyKErr(aError));

    HX_ASSERT(aError != KErrNone); // possible?
    if (aError != KErrNone)
    {
        // stream is implicitly stopped when an error occurs
        switch(aError)
        {
        case KErrUnderflow:
        case KErrNotReady:
            {
                // either we just played out or (more likely) we were not
                // writing to the device fast enough
                // or the device is busy from underflow clean up playinit call was too fast
           
                // do re-init so we continue audio playback
                
                Trans(STOPPED);
                DoPlayInit();
            }
            break;
        case KErrCancel:
            m_lastPlayError = aError;
            Trans(STOPPED);
            break;
        case KErrAccessDenied: 
        case KErrInUse:
        case KErrDied: // incoming message notification on 6630 generates this
        
            // This error case is handled as recoverable error.
            // SamplesPlayed stats will be retained on this case.
            Trans(STOPPED);
            m_lastPlayError = aError;
            break;
        default:
            m_lastPlayError = aError;
            Trans(STOPPED);
            break;

        }
    }
}

// Called when a stop or pause is issued.
//
void CHXBaseAudioSession::PrepareForDeviceReset()
{
#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED

    if(m_sampleCountResetPending == FALSE)
    {
    if (KErrNone == m_lastPlayError)
    {
        // preserve monotically increasing time when device resumes
        UpdateTimeSampleStats();
        m_resetTriggerSampleCount   = m_lastSampleCount;
        m_resetTriggerUnplayedCount = m_unplayedSampleCount;
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::PrepareForDeviceReset(): unplayed = %lu (%lu ms)", m_unplayedSampleCount, SamplesToMS(m_unplayedSampleCount, m_sampleRate));
    }
    else
    {
        // next playback must resume from beginning
        HX_ASSERT(STOPPED == m_state);
        m_resetTriggerSampleCount   = 0;
        m_resetTriggerUnplayedCount = 0;
        m_lastSampleCount           = 0;
        m_unplayedSampleCount       = 0;

        ClearPastQueue();        
    }

    m_sampleCountResetPending = TRUE;
    }
    else
    {
        // Session has been prepared already for reset, So ignore.
        // One possible scenario is Pause followed by Stop
    }

#endif
    // this tracks samples written after device reset trigger
    m_samplesWritten = 0;

    m_bDevSoundInitErr = FALSE;
    m_pPendingFillBuffer = NULL;
}


#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED

void CHXBaseAudioSession::OnResetSampleCount()
    {
    // sample count may reset prior to us seeing all the samples play out

    // Add unaccounted sample to total
    UpdateSamplesPlayedTime(m_unplayedSampleCount);

    m_lastSampleCount           = 0;
    m_unplayedSampleCount       = 0;

    m_resetTriggerSampleCount   = 0;
    m_resetTriggerUnplayedCount = 0;

    m_sampleCountResetPending   = FALSE;
    
}

void CHXBaseAudioSession::CheckSampleCountReset(TUint sampleCount)
{
    // After we stop the audio stream (seek case) or get an underflow error we
    // reinitialize the audio device by calling PlayInitL(). At some point 
    // thereafter the value returned by SamplesPlayed() is reset. That point is
    // not predictable. In some cases it is reset by the time we get the next call
    // to BufferToBeFilled(), but not always. To deal with this we set a flag when
    // we call PlayInitL() and wait for the samples played value to fall below the
    // last known base value. We then assume this is because the sample counter has
    // been reset.
   
    if (m_sampleCountResetPending)
    {
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::HandleSampleCountReset(): checking for reset (last count = %lu;  new count = %lu)", m_lastSampleCount, sampleCount);
        if ( sampleCount < m_lastSampleCount || 0 == m_lastSampleCount)
        {
            OnResetSampleCount();
        }
        else
        {
            HX_ASSERT(sampleCount >= m_resetTriggerSampleCount);
            TUint samplesElapsedSinceTrigger = sampleCount - m_resetTriggerSampleCount;
            if (m_resetTriggerUnplayedCount > 0 && samplesElapsedSinceTrigger > m_resetTriggerUnplayedCount)
            {
                // Special case:
                //
                // If the sample played count at the time of the underflow/pause is small, there is a risk
                // that the counter is reset but by the time we see the (post-reset) value it has exceeded the
                // last value we saw. That breaks the logic above. We deal with this by comparing the
                // samples played with the number of unwritten samples at the time of the underflow/pause.
                // If the value is greater it must be from newly written samples.
                //
                HXLOGL3(HXLOG_ADEV, 
                    "CHXBaseAudioSession::HandleSampleCountReset(): special case played since trigger %lu > unplayed at time of trigger %lu", samplesElapsedSinceTrigger, m_resetTriggerUnplayedCount);
                OnResetSampleCount();
            }
            else
            {
                TUint samplesElapsed = sampleCount - m_lastSampleCount;

                // sample count has not reset yet; some more samples have played out before the reset
                OnNewSampleCount(sampleCount);

                // decrement unplayed samples
                HX_ASSERT(samplesElapsed <= m_unplayedSampleCount);
                m_unplayedSampleCount -= samplesElapsed;
            }
        }
    }
}

void CHXBaseAudioSession::OnNewSampleCount(UINT32 sampleCount)
{
    // determine how much time has elapsed since last time computation

    // assert likely indicates error; wrap-around is rare case
    HX_ASSERT(m_lastSampleCount <= sampleCount);

    // calculate additional samples played since last update
    TUint samplesElapsed = sampleCount - m_lastSampleCount;

    
    UpdateSamplesPlayedTime(samplesElapsed);
    
    HXLOGL4(HXLOG_ADEV, 
        "CHXBaseAudioSession::OnNewSampleCount(): last = %lu; current = %lu; added = %lu", 
        m_lastSampleCount, sampleCount, samplesElapsed);

    // update sample count for next time
    m_lastSampleCount = sampleCount;
}

void CHXBaseAudioSession::UpdateUnplayedSampleCount()
{
    if (!m_sampleCountResetPending)
    {
        // keep track of samples that we wrote but haven't been played yet
        HX_ASSERT(m_samplesWritten >= m_lastSampleCount);
        m_unplayedSampleCount = m_samplesWritten - m_lastSampleCount;
    }

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
    RemoveOutdatedPacketsInPastQueue();
#endif

    HXLOGL4(HXLOG_ADEV, "CHXBaseAudioSession::UpdateUnplayedSampleCount(): unplayed samps = %lu (%lu ms)", m_unplayedSampleCount, SamplesToMS(m_unplayedSampleCount, m_sampleRate));
}

void CHXBaseAudioSession::UpdateTimeSampleStats()
{
    if (PLAYING != m_state)
    {
        // in case samples written increased since last check
        UpdateUnplayedSampleCount();

        // sample count is only reliable in PLAYING state; don't do anything else at this time
        HXLOGL4(HXLOG_ADEV, "CHXBaseAudioSession::UpdateUnplayedSampleCount(): state = %s (no update)", StringifyState(m_state));
        return;
    }

    // We use some heuristics here because of the following observed behavior of values returned
    // from CMMFDevSound::SamplesPlayed() around and after the time of an underflow (or pause on
    // some devices). On OMAP-based devices (6630) a pause results in identical reset behavior.
    //
    // 1) Underflow occurs. At that time SamplesPlayed() is sometimes < samples written. Those appear
    //    to play out (they are not droppped). There probably is a low threshold value at which point
    //    the audio device generates an Underflow error, and we sometimes see the error before all
    //    samples actually play out.
    // 
    // 2) We add time for unplayed samples at that time. We may get a bit ahead of actual time played. 
    //    We do this because that sample count will be reset after we call PlayInit and we may miss
    //    detecting those samples otherwise.
    // 
    // 3) We call PlayInit so that playing resumes after an underflow (the device implicitly stops). 
    //    At some point thereafter the sample count (returned from SamplesPlayed()) will be reset, but that point
    //    is not deterministic. Sometimes we see sample count continue relative to last value before
    //    it resets. Sometimes we see a reset twice. The first reset appears to be for unplayed samples
    //    at time of the reset. Once those are played out the sample count is reset again.
    //   
    
    TUint sampleCount = m_pStream->SamplesPlayed(); //returns TInt (max ~13.5 hours for 44Khz)

    CheckSampleCountReset(sampleCount);
    if (m_sampleCountResetPending)
    { 
        return;
    }
    else if (sampleCount < m_lastSampleCount)
    {
        // Assume this is case where we see two resets, not the relatively rare wrap-arround case. The
        // first reset is apparantly for for unplayed samples at time of underflow. See (3) above.
        HXLOGL3(HXLOG_ADEV, "CHXBaseAudioSession::UpdateTimeSampleStats(): oops (unexpected reset): %lu (last)  > %lu (current); unplayed = %lu", m_lastSampleCount, sampleCount, m_unplayedSampleCount);
        return;
    }

    OnNewSampleCount(sampleCount);
    UpdateUnplayedSampleCount();
}



//
// CHXBaseAudioSession::UpdateSamplesPlayedTime
// Updates the Samples played time
//
void CHXBaseAudioSession::UpdateSamplesPlayedTime(TUint ulSamples)
{
   HX_ASSERT((ulSamples + m_ulBalanceSamples) < KMaxTUint);
   
   // Update previous balance samples to current count.
   ulSamples += m_ulBalanceSamples;
     
   m_ulTimePlayedInSec +=  (ulSamples / m_sampleRate);
   m_ulBalanceSamples = (ulSamples % m_sampleRate);

}

//
// CHXBaseAudioSession::GetSamplesPlayedTimeInMS
// Returns the samples played time in MS
//
TUint CHXBaseAudioSession::GetSamplesPlayedTimeInMS()
{
    return ((m_ulTimePlayedInSec * 1000) + 
            ((m_ulBalanceSamples * 1000) / m_sampleRate));
}

//
// CHXBaseAudioSession::ResetSamplesPlayedTime
// Resets the samples played time
//
void CHXBaseAudioSession::ResetSamplesPlayedTime()
{
    m_ulTimePlayedInSec = 0;
    m_ulBalanceSamples = 0;
}

#endif // End of #ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED

