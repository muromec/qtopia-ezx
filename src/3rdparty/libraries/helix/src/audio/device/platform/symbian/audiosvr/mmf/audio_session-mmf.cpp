/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audio_session-mmf.cpp,v 1.22 2007/07/06 20:21:14 jfinnecy Exp $
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

#if defined (HELIX_CONFIG_CALYPSO_AUDIO_PREF)
#include <calypso/audiopreference.h>
#endif

#include <hal.h> // device id
#include "hxcom.h"
#include "hxslist.h"
#include "hxausvc.h"
#include "hxbuffer.h"
#include "audio_session-mmf.h"
#include "hxtick.h"
#include "debug.h"
#include "hxtlogutil.h"
#include <e32std.h>
#if defined(HELIX_FEATURE_DRM_SECURE_OUTPUT)
  #include <AudioOutput.h>
#endif



#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
CHXSymbianAudioSvrSession::CHXSymbianAudioSvrSession(HXSymbianAudioServer* pServer)
    : CSession2(),
#else
CHXSymbianAudioSvrSession::CHXSymbianAudioSvrSession(RThread& client,
                                             HXSymbianAudioServer* pServer)
    : CSession(client),
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    m_pServer(pServer),
    m_wantsNotify(FALSE)
{

}

//
// CHXSymbianAudioSvrSession::dtor
//
CHXSymbianAudioSvrSession::~CHXSymbianAudioSvrSession()
{
    if (m_wantsNotify)
    {
        m_notifyRequest.Complete(KErrCancel);
    }
    m_pServer->DelSession();
}


//
// CHXSymbianAudioSvrSession::NewL
//
// creates a new session
//
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
CHXSymbianAudioSvrSession* 
CHXSymbianAudioSvrSession::NewL(HXSymbianAudioServer* pServer)
{
    return new (ELeave) CHXSymbianAudioSvrSession(pServer);
}
#else
CHXSymbianAudioSvrSession* 
CHXSymbianAudioSvrSession::NewL(RThread& client, HXSymbianAudioServer* pServer)
{
    return new (ELeave) CHXSymbianAudioSvrSession(client, pServer);
}
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY

//
// CHXSymbianAudioSvrSession::ServiceL
//
// services a client message
//
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
void CHXSymbianAudioSvrSession::ServiceL(const RMessage2& mesg)
{
    m_SessionMessage = mesg;
#else
void CHXSymbianAudioSvrSession::ServiceL(const RMessage& mesg)
{
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    TInt lRetval = KErrNone;
    TBool bCompleteMsg = true;

    switch (mesg.Function()) 
    {
    case HXSymbianAudioServer::EAS_Init:
        lRetval = Init();
        if(lRetval == KErrNone)
        {
            bCompleteMsg = false;
        }
        break;
    case HXSymbianAudioServer::EAS_Play:
        lRetval = PlayAudio();
        break;
    case HXSymbianAudioServer::EAS_Pause:
        lRetval = PauseAudio();
        break;
    case HXSymbianAudioServer::EAS_Write:
        lRetval = Write();
        break;
    case HXSymbianAudioServer::EAS_CancelWrite:
        // No operation
        break;
    case HXSymbianAudioServer::EAS_GetTime:
        lRetval = GetAudioTime();
        break;
    case HXSymbianAudioServer::EAS_GetBlocksBuffered:
        lRetval = GetUnPlayedBlocks();
        break;
    case HXSymbianAudioServer::EAS_SetVolume:
        SetVol(Message().Int0());
        break;
    case HXSymbianAudioServer::EAS_GetVolume:
        lRetval = Volume();
        break;
    case HXSymbianAudioServer::EAS_GetMaxVolume:
        lRetval = MaxVolume();
        break;
    case HXSymbianAudioServer::EAS_GetMinVolume:
        lRetval = MinVolume();
        break;
    case HXSymbianAudioServer::EAS_Stop:
        StopAudio();
        break;
    case HXSymbianAudioServer::EAS_RequestDeviceTakenNotification:
        RequestDeviceTakenNotification();
        bCompleteMsg = false;
        break;
    case HXSymbianAudioServer::EAS_CancelDeviceTakenNotification:
        CancelDeviceTakenNotification();
        break;
    default:
        lRetval = KErrNotSupported;
        break;
    }

    if(bCompleteMsg == true)
    {
        mesg.Complete(lRetval);
    }
}

//
// CHXSymbianAudioSvrSession::Init
//
// Stores the message and invokes base 
// class Init function
//
TInt CHXSymbianAudioSvrSession::Init()
{
    TInt lRetval = KErrArgument;
       
    if (Message().Ptr0() != NULL)
    {
        m_InitMessage = Message();
        lRetval = CHXBaseAudioSession::InitAudio( (AudioDeviceInitArgs*)Message().Ptr0());
    }

    return lRetval;
}

TInt CHXSymbianAudioSvrSession::Write()
{
    TInt lRetval = KErrArgument;

    IHXBuffer* pBuffer = (IHXBuffer*) Message().Ptr0();
    if(pBuffer != NULL)
    {
        lRetval = CHXBaseAudioSession::WriteAudio(pBuffer);
    }

    return lRetval;
}


void
CHXSymbianAudioSvrSession::RequestDeviceTakenNotification()
{
    HXLOGL2(HXLOG_ADEV, "CHXSymbianAudioSvrSession::RequestDeviceTakenNotification()");
    m_wantsNotify = TRUE;
    m_notifyRequest = Message();
}

void
CHXSymbianAudioSvrSession::CancelDeviceTakenNotification()
{
    if (m_wantsNotify)
    {
        m_notifyRequest.Complete(KErrCancel);
        m_wantsNotify = FALSE;
    }
}

//
// CHXSymbianAudioSvrSession::NotifyDeviceTaken
//
// notify the client that the audio device has been taken if a
// notification requrest has been made 
//
void CHXSymbianAudioSvrSession::NotifyDeviceTaken()
{
    if (m_wantsNotify)
    {
        HXLOGL2(HXLOG_ADEV, "CHXSymbianAudioSvrSession::NotifyDeviceTaken(): doing notify...");
        m_notifyRequest.Complete(m_lastPlayError);
        m_wantsNotify = FALSE;
    }
}

//
// CHXSymbianAudioSvrSession::InitializeComplete
//
// Overrrides the devSound observer to inform client
// 
//
void CHXSymbianAudioSvrSession::InitializeComplete(TInt aError)
{
    HXLOGL3(HXLOG_ADEV, "CHXSymbianAudioSvrSession::InitializeComplete(): err = %s", StringifyKErr(aError));
    
    if(aError == KErrNone)
    {
        CHXBaseAudioSession::InitializeComplete(aError);
        aError = m_lastPlayError;
    }

    m_InitMessage.Complete(aError);
}


//
// CHXSymbianAudioSvrSession::PlayError
//
// Overrrides the devSound observer to inform client
// of error cases
// 
//
void CHXSymbianAudioSvrSession::PlayError(TInt aError)
{
    HXLOGL2(HXLOG_ADEV, "CHXSymbianAudioSvrSession::PlayError(%d): err = %s", 
            aError, StringifyKErr(aError));

    HX_ASSERT(aError != KErrNone); // possible?
    if (aError != KErrNone)
    {

        CHXBaseAudioSession::PlayError(aError);

        // stream is implicitly stopped when an error occurs
        switch(aError)
        {
        case KErrAccessDenied: 
        case KErrInUse:
        case KErrDied: // incoming message notification on 6630 generates this
            NotifyDeviceTaken();
            break;
        default:
            break;
        }
    } // End of if (aError != KErrNone)
}

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    // This is not polymorphism - rather replacing CSession::Message() that is
    // removed with PlatSec in Symbian 8.1, with this local Message() that 
    // returns RMessage2 - a reference to the session specific message.
    // Using the same method name allows the calling code to remain unmodified.
    // This works because the previous CSession::Message() methods used in this
    // code were moved to RMessage2 for PlatSec.
inline const RMessage2 
CHXSymbianAudioSvrSession::Message() {return m_SessionMessage;} 
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY




