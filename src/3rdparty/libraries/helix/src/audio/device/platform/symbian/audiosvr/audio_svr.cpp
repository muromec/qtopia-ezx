/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audio_svr.cpp,v 1.9 2007/04/03 18:24:49 rrajesh Exp $
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

#include <e32math.h>
#include "hlxclib/memory.h"
#include "audio_svr.h"
#include "AudDevStatusObserver.h"

#if defined (HELIX_CONFIG_DEVSOUND)
#include "mmf/audio_session-mmf.h"
#else
#include "mda/audio_session-mda.h"
#endif




static const TUint KDefaultMessageSlots = 2;

_LIT(kHXSymbianAudioServerName, "HelixAudioThreadName");

//
// SendReceive argument packing function have been changed
// in Symbian 9.1. So set of macros are defined for handling
// based on the symbian version
//
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY

#define SENDRECEIVE(_COMMAND_, _RETVAL_)    \
                            _RETVAL_ = SendReceive(_COMMAND_, TIpcArgs(0));

#define SENDRECEIVE_1_ARG(_COMMAND_, _ARG1_, _RETVAL_)    \
                            _RETVAL_ = SendReceive(_COMMAND_, TIpcArgs(_ARG1_));

#define SENDRECEIVE_2_ARG(_COMMAND_, _ARG1_, _ARG2_, _RETVAL_)    \
                            _RETVAL_ = SendReceive(_COMMAND_, TIpcArgs(_ARG1_, _ARG2_));

#define SENDRECEIVE_3_ARG(_COMMAND_, _ARG1_, _ARG2_, _ARG3_, _RETVAL_)    \
                            _RETVAL_ = SendReceive(_COMMAND_, TIpcArgs(_ARG1_, _ARG2_, _ARG3_));

#define SENDRECEIVE_ARG_STATUS(_COMMAND_, _ARG1_, _STATUS_)    \
                            SendReceive(_COMMAND_, TIpcArgs(_ARG1_), _STATUS_);

#define SENDRECEIVE_STATUS(_COMMAND_, _STATUS_)   \
                            SendReceive(_COMMAND_, TIpcArgs(0), _STATUS_);

#else

#define SENDRECEIVE(_COMMAND_, _RETVAL_)  _RETVAL_ = SendReceive(_COMMAND_, 0);

#define SENDRECEIVE_1_ARG(_COMMAND_, _ARG1_, _RETVAL_)      \
                            TAny* p[KMaxMessageArguments];  \
                            p[0] = (TAny*) _ARG1_; \
                            _RETVAL_ = SendReceive(_COMMAND_, &p[0]);

#define SENDRECEIVE_2_ARG(_COMMAND_, _ARG1_, _ARG2_, _RETVAL_)  \
                            TAny* p[KMaxMessageArguments];  \
                            p[0] = (TAny*) _ARG1_; \
                            p[1] = (TAny*) _ARG2_;  \
                            _RETVAL_ = SendReceive(_COMMAND_, &p[0]);

#define SENDRECEIVE_3_ARG(_COMMAND_, _ARG1_, _ARG2_, _ARG3_, _RETVAL_)  \
                            TAny* p[KMaxMessageArguments];  \
                            p[0] = (TAny*) _ARG1_; \
                            p[1] = (TAny*) _ARG2_;  \
                            p[2] = (TAny*) _ARG3_;  \
                            _RETVAL_ = SendReceive(_COMMAND_, &p[0]);

#define SENDRECEIVE_ARG_STATUS(_COMMAND_, _ARG1_, _STATUS_)    \
                            TAny* p[KMaxMessageArguments];  \
                            p[0] = (TAny*) _ARG1_; \
                            SendReceive(_COMMAND_, &p[0], _STATUS_);

#define SENDRECEIVE_STATUS(_COMMAND_, _STATUS_)   \
                            SendReceive(_COMMAND_, 0, _STATUS_);

#endif // End of #ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY

// class HXSymbianAudioServer:


HXSymbianAudioServer::HXSymbianAudioServer()
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY 
: CServer2(CActive::EPriorityStandard, ESharableSessions)
#else
: CServer(CActive::EPriorityStandard, ESharableSessions)
#endif
,m_sessionCount(0)
{
    
}

HXSymbianAudioServer::~HXSymbianAudioServer()
{
    Cancel();
}

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY 
CSession2* 
HXSymbianAudioServer::NewSessionL(const TVersion& aVersion, 
                                  const RMessage2 &/*aMessage*/) const
{
    return CHXSymbianAudioSvrSession::NewL((HXSymbianAudioServer*)this);
}
#else
CSharableSession* 
HXSymbianAudioServer::NewSessionL(const TVersion& aVersion) const
{
    RThread client = Message().Client();
    return CHXSymbianAudioSvrSession::NewL(client, (HXSymbianAudioServer*)this);
}
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY 

void HXSymbianAudioServer::AddSession()
{
    ++m_sessionCount;
}

void HXSymbianAudioServer::DelSession()
{
    --m_sessionCount;
    
    if (m_sessionCount <= 0)
    {
        // exit the server
        CActiveScheduler::Stop();
    }
}

// in leave handler for active object (CServer::RunL())
TInt HXSymbianAudioServer::RunError(TInt aError)
{
    Message().Complete(aError);
    
    //
    // Leaving from CServer::RunL() results in skipping the call to request
    // another message; ReStart() keeps the server running
    //
    ReStart();
    
    // handled the error fully (must return KErrNone to keep active
    // scheduler running)
    return KErrNone;    
}

void HXSymbianAudioServer::NotifyDeviceTaken()
{
    // loop over session and notify them the device has been taken
    CHXSymbianAudioSvrSession* pSession;
    
    iSessionIter.SetToFirst();
    while ((pSession = (CHXSymbianAudioSvrSession*)iSessionIter++) != NULL)
        pSession->NotifyDeviceTaken();
}

//
// HXSymbianAudioClient
//

HXSymbianAudioClient::HXSymbianAudioClient()
: m_stopped(true)
,m_pServerCtx(NULL)
,m_pDevTakenNotifier(NULL)
{

}
HXSymbianAudioClient::~HXSymbianAudioClient()
{
    CloseDevice();
    HX_DELETE(m_pServerCtx);
    HX_DELETE(m_pDevTakenNotifier);
}

TInt HXSymbianAudioClient::Open()
{
    TInt lRetval = KErrGeneral;
    
    HBufC* pName = HBufC::New(kHXSymbianAudioServerName().Length() + 16);
    
    if(!pName)
    {
        return HXR_OUTOFMEMORY;
    }
    
    //server name will be random
    TPtr p = pName->Des();
    p.Copy(kHXSymbianAudioServerName);
    p.AppendNum(Math::Random(),EHex);
    
    // create the thread context for the audio server to run
    if (!m_pServerCtx)
    {
        m_pServerCtx = new HXSymbianAudioServerContext;
    }
    
    // start the audio sevrver thread
    if (m_pServerCtx && !m_pServerCtx->Running())
    {
        m_pServerCtx->Start(*pName);
    }
    
    // connect to the audio server and share the session
    
    if (m_pServerCtx->Running() &&
        KErrNone == Connect(*pName)
#ifndef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
        && KErrNone == Share(RSessionBase::EAutoAttach)
#endif
        )
    {
        lRetval = KErrNone;
    }
    
    delete pName;
    
    return lRetval;
}

void HXSymbianAudioClient::CloseDevice()
{
    Close();
    if(m_pServerCtx != NULL)
    {
        m_pServerCtx->Stop();
    }
}


TInt HXSymbianAudioClient::Init(int sampleRate, int channels,
               TMMFPrioritySettings* pPrioritySettings, HXBOOL bSecureAudio)
{
    TInt iRetval = KErrNone;
    AudioDeviceInitArgs stInitArgs;
    
    stInitArgs.ulSampleRate      = sampleRate;
    stInitArgs.uChannels         = channels;
    stInitArgs.pPrioritySettings = pPrioritySettings;
    stInitArgs.bSecureAudio      = bSecureAudio;
    
    SENDRECEIVE_1_ARG(HXSymbianAudioServer::EAS_Init, &stInitArgs, iRetval);
    return iRetval;
}

TInt HXSymbianAudioClient::Play()
{
    TInt iRetval = KErrNone;
    m_stopped = false;
    SENDRECEIVE(HXSymbianAudioServer::EAS_Play, iRetval);
    return iRetval;
}

void HXSymbianAudioClient::Pause()
{
    TInt iRetval = KErrNone;
    SENDRECEIVE(HXSymbianAudioServer::EAS_Pause, iRetval);
}

void HXSymbianAudioClient::Stop()
{
    TInt iRetval = KErrNone;
    SENDRECEIVE(HXSymbianAudioServer::EAS_Stop, iRetval);
    m_stopped = true;
}

HXBOOL HXSymbianAudioClient::Stopped()
{
    return m_stopped;
}

void HXSymbianAudioClient::SetVolume(const TInt aNewVolume)
{
    TInt iRetval = KErrNone;
    SENDRECEIVE_1_ARG(HXSymbianAudioServer::EAS_SetVolume, aNewVolume, iRetval);
}

TInt HXSymbianAudioClient::GetVolume() const
{
    TInt iRetval = 0;
    SENDRECEIVE(HXSymbianAudioServer::EAS_GetVolume, iRetval);
    return iRetval;
}

TInt HXSymbianAudioClient::GetMaxVolume() const
{
    TInt iRetval =0;
    SENDRECEIVE(HXSymbianAudioServer::EAS_GetMaxVolume, iRetval);
    return iRetval;
}

TInt HXSymbianAudioClient::GetMinVolume() const
{
    TInt iRetval =0;
    SENDRECEIVE(HXSymbianAudioServer::EAS_GetMinVolume, iRetval);
    return iRetval;
}

TInt HXSymbianAudioClient::Write(const IHXBuffer* pAudioBuf)
{
    TInt iRetval =0;
    SENDRECEIVE_1_ARG(HXSymbianAudioServer::EAS_Write, pAudioBuf, iRetval);
    return iRetval;
}

TUint HXSymbianAudioClient::GetTime()
{
    TInt iRetval = 0;
    SENDRECEIVE(HXSymbianAudioServer::EAS_GetTime, iRetval);
    return ((TUint) iRetval);
}

TUint HXSymbianAudioClient::GetBlocksBuffered()
{
    TInt iRetval = 0;
    SENDRECEIVE(HXSymbianAudioServer::EAS_GetBlocksBuffered, iRetval);
    return ((TUint) iRetval);
}

TInt HXSymbianAudioClient::Connect(const TDesC& ServerName)
{
    TInt status = KErrNoMemory;

    status = CreateSession(ServerName, TVersion(0,0,0), KDefaultMessageSlots);
    return status;
}

void HXSymbianAudioClient::RequestDeviceTakenNotification(CHXAudDevStatusObserver* pDevStatusObserver)
{
    if(pDevStatusObserver != NULL)
    {
        m_pAudDevStatusObserver = pDevStatusObserver;
        //Create dev taken notifier
        if(m_pDevTakenNotifier == NULL)
        {
            m_pDevTakenNotifier = new CHXAudDevTakenNotifier(this);
        }
        
        // Activate the notifier
        ActivateDevTakenNotification();
    } // End of if(pDevStatusObserver != NULL)
}

void HXSymbianAudioClient::CancelDeviceTakenNotification()
{
    if(m_pDevTakenNotifier != NULL)
    {
        TInt iRetval = 0;
        SENDRECEIVE(HXSymbianAudioServer::EAS_CancelDeviceTakenNotification,
            iRetval);
        
        HX_DELETE(m_pDevTakenNotifier);
    }
}


void HXSymbianAudioClient::OnAudDevStatusChange(TInt lStatus)
{
    if(m_pAudDevStatusObserver != NULL)
    {
        m_pAudDevStatusObserver->OnAudDevStatusChange(lStatus);
    }

    // Re-activate the notifier
    ActivateDevTakenNotification();
}

void HXSymbianAudioClient::ActivateDevTakenNotification()
{
    if((m_pDevTakenNotifier != NULL) &&
        (!m_pDevTakenNotifier->IsActive()) )
    {
        TRequestStatus &status = m_pDevTakenNotifier->Status();
        SENDRECEIVE_STATUS(HXSymbianAudioServer::EAS_RequestDeviceTakenNotification,
            status);
        m_pDevTakenNotifier->Activate();    
    }
    
}
