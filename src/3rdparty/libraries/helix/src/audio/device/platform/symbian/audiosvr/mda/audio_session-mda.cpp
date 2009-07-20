/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audio_session-mda.cpp,v 1.9 2005/03/14 19:43:22 bobclark Exp $
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

#if defined (HELIX_CONFIG_CALYPSO_AUDIO_PREF)
#include <calypso/audiopreference.h>
#endif
#include <mda/common/audio.h>

#include "hxcom.h"
#include "hxslist.h"
#include "hxausvc.h"
#include "hxbuffer.h"
#include "audio_session-mda.h"
#include "hxtick.h"

static const TInt KClientPriority = 69; //EMdaPriorityNormal;
#if defined(HELIX_CONFIG_CALYPSO_AUDIO_PREF)
static const TMdaPriorityPreference KPriorityPref = (TMdaPriorityPreference)KAudioPrefComposer; 
#else
static const TMdaPriorityPreference KPriorityPref = EMdaPriorityPreferenceTime;
#endif // SERIES60_PLAYER

const INT32 WriteBufferDepth = 10; // Number of buffers we allow in the queue before blocking the caller

////////////////////////////////////////////////////
//
static TInt FlagToNumber(TMdaAudioDataSettings::TAudioCaps flag)
{
    switch( flag )
    {
    case TMdaAudioDataSettings::ESampleRate8000Hz:
        return 8000;
    case TMdaAudioDataSettings::ESampleRate11025Hz:
        return 11025;
    case TMdaAudioDataSettings::ESampleRate16000Hz:
        return 16000;
    case TMdaAudioDataSettings::ESampleRate22050Hz:
        return 22050;
    case TMdaAudioDataSettings::ESampleRate32000Hz:
        return 32000;
    case TMdaAudioDataSettings::ESampleRate44100Hz:
        return 44100;
    case TMdaAudioDataSettings::ESampleRate48000Hz:
        return 48000;
    case TMdaAudioDataSettings::EChannelsMono:
        return 1;
    case TMdaAudioDataSettings::EChannelsStereo:
        return 2;
    default:
        return 0;
        break;
    }
    return 0;
}

static TMdaAudioDataSettings::TAudioCaps NumberToFlag(TInt num)
{
    switch(num)
    {
    case 1:
        return  TMdaAudioDataSettings::EChannelsMono;
    case 2:
        return  TMdaAudioDataSettings::EChannelsStereo;
    case 8000:
        return  TMdaAudioDataSettings::ESampleRate8000Hz;
    case 11025:
        return  TMdaAudioDataSettings::ESampleRate11025Hz;
    case 16000:
        return  TMdaAudioDataSettings::ESampleRate16000Hz;
    case 22050:
        return  TMdaAudioDataSettings::ESampleRate22050Hz;
    case 32000:
        return  TMdaAudioDataSettings::ESampleRate32000Hz;
    case 44100:
        return  TMdaAudioDataSettings::ESampleRate44100Hz;
    case 48000:
        return  TMdaAudioDataSettings::ESampleRate48000Hz;
    default:
        return (TMdaAudioDataSettings::TAudioCaps)0;
        break;
    }
    return (TMdaAudioDataSettings::TAudioCaps)0;
}

HXSymbianAudioTimeline::HXSymbianAudioTimeline()
{
    Reset(0);
}

HXSymbianAudioTimeline::~HXSymbianAudioTimeline()
{}

void HXSymbianAudioTimeline::Reset(UINT32 ulByteRate)
{
    m_ulByteRate = ulByteRate;

    ClearWritten();

    m_ulBaseSec = 0;
    m_ulBaseSubSecBytes = 0;

    m_ulLastGetTime = 0;
}

void HXSymbianAudioTimeline::OnWrite(UINT32 ulBytes)
{
    UINT32 ulSum = m_ulSubSecBytes + ulBytes;
    UINT32 ulSec = ulSum / m_ulByteRate;

    m_ulSecWritten += ulSec;
    m_ulSubSecBytes = ulSum - (ulSec * m_ulByteRate);
}

UINT32 HXSymbianAudioTimeline::GetPlaybackTime()
{
    UINT32 ulBaseMs = GetMs(m_ulBaseSec, m_ulBaseSubSecBytes);
    UINT32 ulWrittenMs = GetWrittenTime();
    UINT32 ulRet = ulBaseMs;
    UINT32 ulDeviceTimeMs = GetWallclockTime();
    
    // Only use the device time if it
    // is less than the amount of data
    // written
    if (ulWrittenMs > ulDeviceTimeMs)
    {
        ulRet += ulDeviceTimeMs;
    }
    else
    {
        ulRet += ulWrittenMs;
    }

    // Enforce monotonically increasing return values
    if (ulRet >= m_ulLastGetTime)
    {
        m_ulLastGetTime = ulRet;
    }
    else
    {
        ulRet = m_ulLastGetTime;
    }

    return ulRet;
}

UINT32 HXSymbianAudioTimeline::GetWrittenTime() const
{
    return GetMs(m_ulSecWritten, m_ulSubSecBytes);
}


UINT32 HXSymbianAudioTimeline::GetWallclockTime() const
{
    UINT32 ulRet = m_ulDevTimeMs;

    if (m_ulDevTimeMs)
    {
        ulRet += HX_GET_TICKCOUNT() - m_ulWallclockTime;
    }

    return ulRet;
}

void HXSymbianAudioTimeline::OnPlay()
{
    ClearWritten();
}

void HXSymbianAudioTimeline::OnPauseOrUnderflow()
{
    // Add the written time to the base time
    m_ulBaseSec += m_ulSecWritten;
    m_ulBaseSubSecBytes += m_ulSubSecBytes;

    // Normalize the base time values
    while (m_ulBaseSubSecBytes >= m_ulByteRate)
    {
        m_ulBaseSec++;
        m_ulBaseSubSecBytes -= m_ulByteRate;
    }
    
    // Clear the bytes written state
    ClearWritten();
}

HXBOOL HXSymbianAudioTimeline::NeedDeviceTime() const
{
    // We only want a device time if we don't haven't
    // received a non-zero device time.
    return (m_ulDevTimeMs == 0);
}


void HXSymbianAudioTimeline::SetDeviceTime(UINT32 ulDeviceTimeMs)
{
    if (NeedDeviceTime())
    {
        m_ulDevTimeMs = ulDeviceTimeMs;
        m_ulWallclockTime = HX_GET_TICKCOUNT();
    }
}

UINT32 HXSymbianAudioTimeline::GetMs(UINT32 ulSec, UINT32 ulSubSec) const
{
    UINT32 ulRet = ulSec * 1000;

    if (m_ulByteRate)
    {
        ulRet += (ulSubSec * 1000) / m_ulByteRate;
    }

    return ulRet;
}

void HXSymbianAudioTimeline::ClearWritten()
{
    m_ulSecWritten = 0;
    m_ulSubSecBytes = 0;

    m_ulDevTimeMs = 0;
    m_ulWallclockTime = 0;
}

//
// class HXSymbianAudioSession:
//

//
// HXSymbianAudioSession::ctor:
//
// add the session to the server
//
HXSymbianAudioSession::HXSymbianAudioSession(RThread& client,
                                             HXSymbianAudioServer* pServer)
    : CSession(client),
      m_pServer(pServer),
      m_pStream(0),
      m_sampleRate(TMdaAudioDataSettings::ESampleRate8000Hz),
      m_channels(TMdaAudioDataSettings::EChannelsMono),
      m_pData(0, 0),
      m_wantsNotify(false),
      m_reason(KErrNone),
      m_open(false),
      m_bPaused(TRUE),
      m_bWritePending(FALSE)
{
    // add the session to the server
    m_pServer->AddSession();
}

//
// HXSymbianAudioSession::dtor
//
HXSymbianAudioSession::~HXSymbianAudioSession()
{
    DoCleanup();
}


//
// HXSymbianAudioSession::NewL
//
// creates a new session
//
HXSymbianAudioSession* HXSymbianAudioSession::NewL(RThread& client,
                                                   HXSymbianAudioServer* pServer)
{
    HXSymbianAudioSession* pSession =
        new (ELeave) HXSymbianAudioSession(client, pServer);

    return pSession;
}

//
// HXSymbianAudioSession::ServiceL
//
// services a client message
//
void HXSymbianAudioSession::ServiceL(const RMessage& mesg)
{
    switch (mesg.Function()) 
    {
    case HXSymbianAudioServer::EAS_Init:
        Init();
        break;
    case HXSymbianAudioServer::EAS_Play:
        Play();
        break;
    case HXSymbianAudioServer::EAS_Pause:
        Pause();
        break;
    case HXSymbianAudioServer::EAS_Write:
        Write();
        break;
    case HXSymbianAudioServer::EAS_CancelWrite:
        mesg.Complete(KErrNone);
        break;
    case HXSymbianAudioServer::EAS_GetTime:
        GetTime();
        break;
    case HXSymbianAudioServer::EAS_GetBlocksBuffered:
        GetBlocksBuffered();
        break;
    case HXSymbianAudioServer::EAS_SetVolume:
        SetVolume();
        break;
    case HXSymbianAudioServer::EAS_GetVolume:
        GetVolume();
        break;
    case HXSymbianAudioServer::EAS_GetMaxVolume:
        GetMaxVolume();
        break;
    case HXSymbianAudioServer::EAS_GetMinVolume:
        GetMinVolume();
        break;
    case HXSymbianAudioServer::EAS_Stop:
        Stop();
        break;
    case HXSymbianAudioServer::EAS_RequestDeviceTakenNotification:
        RequestDeviceTakenNotification();
        break;
    case HXSymbianAudioServer::EAS_CancelDeviceTakenNotification:
        CancelDeviceTakenNotification();
        break;
    default:
        mesg.Complete(KErrNotSupported);
        break;
    }
}

//
// HXSymbianAudioSession::Init
//
// 1. Matches input sample rate to output sample rate
//    by building a sample rate converter, if necessary.
// 2. Opens and initializes the audio device.
//
void HXSymbianAudioSession::Init()
{
    TInt err = KErrNone;

    // translate the audio props to flags needed by interface
    m_sampleRate = NumberToFlag(Message().Int0());
    m_channels   = NumberToFlag(Message().Int1());

    m_timeline.Reset(GetByteRate());
    m_bPaused = TRUE;

    if (m_open)
    {
        TRAP(err, m_pStream->SetAudioPropertiesL(m_sampleRate,
                                                 m_channels));

        Message().Complete(err);
    }
    else 
    {
        TRAP(err, (m_pStream = CMdaAudioOutputStream::NewL(*this)));

        if(KErrNone == err)
        {
            // open the audio device
            m_settings.iSampleRate = m_sampleRate;
            m_settings.iChannels = m_channels;
            m_settings.iVolume = m_pStream->MaxVolume()/2;
            m_settings.iFlags = 0;
            m_pStream->Open(&m_settings);
        }
        else
        {
            Message().Complete(err);
        }
    }
}

void HXSymbianAudioSession::DoCleanup()
{
    while(!m_bufferList.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_bufferList.RemoveHead();
        HX_RELEASE(pBuf);
    }

    if (m_wantsNotify)
    {
        m_notifyRequest.Complete(KErrCancel);
        m_wantsNotify = false;
    }

    if (m_pStream)
    {
        m_pStream->Stop();
        delete m_pStream;
        m_pStream = 0;
        m_open = false;
    }

    // remove session from server
    if( m_pServer)
    {
        m_pServer->DelSession();
        m_pServer = 0;
    }
}

//
// HXSymbianAudioSession::Play
//
void HXSymbianAudioSession::Play()
{
    if (m_reason != KErrNone)
        m_reason = KErrNone;

    // reset audio properties in case they changed on us
    TRAPD(error, m_pStream->SetAudioPropertiesL(m_sampleRate,
                                                m_channels));
    m_pStream->SetPriority(KClientPriority, KPriorityPref);
    m_timeline.OnPlay();
    m_bPaused = FALSE;

    if (ReadyToWrite())
    {
        error = WriteNextBuffer();
    }

    Message().Complete(error);
}

//
// HXSymbianAudioSession::Pause
//
void HXSymbianAudioSession::Pause()
{
    m_timeline.OnPauseOrUnderflow();
    m_bPaused = TRUE;

    if (m_pStream)
    {
        m_pStream->Stop();
    }
    Message().Complete(KErrNone);
}

//
// HXSymbianAudioSession::Write
//
//
void HXSymbianAudioSession::Write()
{
    TInt result = KErrArgument;
    IHXBuffer* pAudioBuf = (IHXBuffer*)Message().Ptr0();

    if (pAudioBuf)
    {
        if (m_bufferList.AddTail(pAudioBuf))
        {
            if (ReadyToWrite())
            {
                result = WriteNextBuffer();

                if (KErrNone != result)
                {
                    // Remove the buffer we just appended
                    // to the list
                    m_bufferList.RemoveTail();

                    // Release our reference to the buffer
                    HX_RELEASE(pAudioBuf);
                }
            }
            else
            {
                result = KErrNone;
            }
        }
        else
        {
            result = KErrNoMemory;
        }
        
    }

    Message().Complete(result);
}

//
// HXSymbianAudioSession::GetTime
//
// Return the current playback position -- converts from
// microseconds to milliseconds
//
void HXSymbianAudioSession::GetTime()
{
    Message().Complete(m_timeline.GetPlaybackTime());
}


//
// HXSymbianAudioSession::GetBlocksBuffered
//
// Return the number of blocks buffered by this object.
//
void HXSymbianAudioSession::GetBlocksBuffered()
{
    Message().Complete(m_bufferList.GetCount());
}

//
// HXSymbianAudioSession::SetVolume
//
// set the volume -- convert from 0-100 to 0-max range
//
void HXSymbianAudioSession::SetVolume()
{
    if (m_pStream)
    {
        m_pStream->SetVolume(Message().Int0());
    }
    Message().Complete(0);
}

//
// HXSymbianAudioSession::GetVolume
//
// get the current volume normalized to 0-100 range
//
void HXSymbianAudioSession::GetVolume()
{
    TInt vol = 0;
    if (m_pStream)
    {
        vol = m_pStream->Volume();
    }
    Message().Complete(vol);
}

//
// HXSymbianAudioSession::GetMaxVolume
//
// get the maxium device volume
//
void HXSymbianAudioSession::GetMaxVolume()
{
    TInt maxVol = 0;
    if (m_pStream)
    {
        maxVol = m_pStream->MaxVolume();
    }

    Message().Complete(maxVol);
}

//
// HXSymbianAudioSession::GetMinVolume
//
// get the minimum device volume
//
void HXSymbianAudioSession::GetMinVolume()
{
    Message().Complete(0);
}


//
// HXSymbianAudioSession::Stop
//
// stop playback
//
void HXSymbianAudioSession::Stop()
{
    m_bPaused = TRUE;
    if(m_pStream)
    {
        m_pStream->Stop();
    }
    
    // Cleanup any remaining buffers
    while(!m_bufferList.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_bufferList.RemoveHead();
        HX_RELEASE(pBuf);
    }

    m_timeline.Reset(GetByteRate());

    Message().Complete(KErrNone);
}

void
HXSymbianAudioSession::RequestDeviceTakenNotification()
{
    m_wantsNotify = true;
    m_notifyRequest = Message();
}

void
HXSymbianAudioSession::CancelDeviceTakenNotification()
{
    if (m_wantsNotify)
    {
        m_notifyRequest.Complete(KErrCancel);
        m_wantsNotify = false;
    }
}

//
// HXSymbianAudioSession::NotifyDeviceTaken
//
// notify the client that the audio device has been taken if a
// notification requrest has been made 
//
void HXSymbianAudioSession::NotifyDeviceTaken()
{
    if (m_wantsNotify)
    {
        m_notifyRequest.Complete(m_reason);
        m_wantsNotify = false;
    }
}

//
// callbacks
//

void HXSymbianAudioSession::MaoscOpenComplete(TInt error)
{
    if (error == KErrNone)
    {
        m_open = true;

        TRAP(error, m_pStream->SetAudioPropertiesL(m_sampleRate,
                                                   m_channels));
        m_pStream->SetPriority(KClientPriority, KPriorityPref);
    }

    Message().Complete(error);
}

void HXSymbianAudioSession::MaoscBufferCopied(TInt error, const TDesC8& buf)
{
    m_bWritePending = FALSE;
    // Check to see if we need a device time
    if (m_timeline.NeedDeviceTime())
    {
        m_timeline.SetDeviceTime(GetDeviceMs());
    }

    if (!m_bufferList.IsEmpty())
    {
        // We should always enter here because the
        // last buffer written is at the head of the list.

        // We want to remove this buffer from the list since
        // this call is the completion of last WriteL() call.
        IHXBuffer* pBuf = (IHXBuffer*)m_bufferList.RemoveHead();
        HX_RELEASE(pBuf);
    }

    if (ReadyToWrite())
    {
        error = WriteNextBuffer();
    }
}

void HXSymbianAudioSession::MaoscPlayComplete(TInt error)
{
    if (KErrUnderflow == error)
    {
        m_timeline.OnPauseOrUnderflow();
    }
    int resetErr;
    TRAP(resetErr, m_pStream->SetAudioPropertiesL(m_sampleRate,
                                                  m_channels));
    m_pStream->SetPriority(KClientPriority, KPriorityPref);

    if (error == KErrAbort)
    {
        m_reason = error;
        m_pServer->NotifyDeviceTaken();
    }

}

UINT32 HXSymbianAudioSession::GetByteRate() const
{
    return 2 * FlagToNumber(m_sampleRate) * FlagToNumber(m_channels);
}


UINT32 HXSymbianAudioSession::GetDeviceMs()
{
    UINT32 ulRet = 0;

    if (m_pStream)
    {
        TTimeIntervalMicroSeconds pos = m_pStream->Position();
        TInt64 millisecs = pos.Int64() / 1000;
   
        ulRet = millisecs.Low();
    }

    return ulRet;
}

HXBOOL HXSymbianAudioSession::ReadyToWrite() const
{
    return !m_bWritePending && !m_bPaused && !m_bufferList.IsEmpty();
}

TInt HXSymbianAudioSession::WriteNextBuffer()
{
    // Write the next buffer in the list
    IHXBuffer* pBuffer = (IHXBuffer*)m_bufferList.GetHead();

    TInt result = KErrNone;

    if (pBuffer)
    {
        if( m_pStream )
        {
            int len = pBuffer->GetSize();
            m_pData.Set((TUint8*)pBuffer->GetBuffer(), len, len);
            TRAP(result, m_pStream->WriteL(m_pData));
        }
        else
        {
            // oom earlier?
            result = KErrNotReady;
        }
    }

    if (KErrNone == result)
    {
        m_timeline.OnWrite(pBuffer->GetSize());
        m_bWritePending = TRUE;
    }
    else
    {
        m_bWritePending = FALSE;
    }

    return result;
}
