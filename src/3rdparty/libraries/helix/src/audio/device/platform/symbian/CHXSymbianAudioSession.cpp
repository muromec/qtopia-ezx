/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: CHXSymbianAudioSession.cpp,v 1.3 2008/10/19 05:13:54 gajia Exp $
 * 
 * Copyright Notices: 
 *  
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
 *  
 * Patent Notices: This file may contain technology protected by one or  
 * more of the patents listed at www.helixcommunity.org 
 *  
 * 1.   The contents of this file, and the files included with this file, 
 * are protected by copyright controlled by RealNetworks and its  
 * licensors, and made available by RealNetworks subject to the current  
 * version of the RealNetworks Public Source License (the "RPSL")  
 * available at  http://www.helixcommunity.org/content/rpsl unless  
 * you have licensed the file under the current version of the  
 * RealNetworks Community Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply.  You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *  
 * 2.  Alternatively, the contents of this file may be used under the 
 * terms of the GNU General Public License Version 2 (the 
 * "GPL") in which case the provisions of the GPL are applicable 
 * instead of those above.  Please note that RealNetworks and its  
 * licensors disclaim any implied patent license under the GPL.   
 * If you wish to allow use of your version of this file only under  
 * the terms of the GPL, and not to allow others 
 * to use your version of this file under the terms of either the RPSL 
 * or RCSL, indicate your decision by deleting Paragraph 1 above 
 * and replace them with the notice and other provisions required by 
 * the GPL. If you do not delete Paragraph 1 above, a recipient may 
 * use your version of this file under the terms of any one of the 
 * RPSL, the RCSL or the GPL. 
 *  
 * This file is part of the Helix DNA Technology.  RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created.   Copying, including reproducing, storing,  
 * adapting or translating, any or all of this material other than  
 * pursuant to the license terms referred to above requires the prior  
 * written consent of RealNetworks and its licensors 
 *  
 * This file, and the files included with this file, is distributed 
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributors: Nokia Inc
 *
 *
 * ***** END LICENSE BLOCK ***** */

#include "hxcom.h"
#include "hxbuffer.h"
#include "CHXSymbianAudioSession.h"
#include "hxtick.h"
#include "debug.h"
#include "hxtlogutil.h"
#include "AudDevStatusObserver.cpp"

//
// CHXSymbianAudioSession
//
CHXSymbianAudioSession::CHXSymbianAudioSession()
: m_stopped(true)
,m_lAsyncWaitError(KErrNone)
,m_pAudDevStatusObserver(NULL)
,m_pActiveSchedulerWait(NULL)
,m_bDevInitCompleted(FALSE)
{

}

CHXSymbianAudioSession::~CHXSymbianAudioSession()
{
    CloseDevice();
}

TInt CHXSymbianAudioSession::Open()
{
    TInt lRetval = KErrNoMemory;

    m_pActiveSchedulerWait = new CActiveSchedulerWait();

    if(m_pActiveSchedulerWait != NULL)
    {
        lRetval = KErrNone;
    }
        
    return lRetval;
}

void CHXSymbianAudioSession::CloseDevice()
{
    HX_DELETE(m_pActiveSchedulerWait);
}


TInt CHXSymbianAudioSession::Init(int sampleRate, int channels,
               TMMFPrioritySettings* pPrioritySettings, HXBOOL bSecureAudio)
{
    TInt lRetval = KErrNotReady;
    AudioDeviceInitArgs stInitArgs;

    stInitArgs.ulSampleRate      = sampleRate;
    stInitArgs.uChannels         = channels;
    stInitArgs.pPrioritySettings = pPrioritySettings;
    stInitArgs.bSecureAudio      = bSecureAudio;

    if(m_pActiveSchedulerWait != NULL)
    {
        m_bDevInitCompleted = FALSE;  
        lRetval = CHXBaseAudioSession::InitAudio(&stInitArgs);
        if((m_bDevInitCompleted == FALSE) &&
	   (lRetval == KErrNone))
        {
            m_pActiveSchedulerWait->Start();
            lRetval = m_lastPlayError;
        }
    }

    return lRetval;
}

TInt CHXSymbianAudioSession::Play()
{
    TInt lRetval = KErrNone;
    m_stopped = false;
    lRetval = CHXBaseAudioSession::PlayAudio();
    return lRetval;
}

void CHXSymbianAudioSession::Pause()
{
    CHXBaseAudioSession::PauseAudio();
}

void CHXSymbianAudioSession::Stop()
{
    TInt lRetval = KErrNone;
    lRetval = CHXBaseAudioSession::StopAudio();
    m_stopped = true;
}

HXBOOL CHXSymbianAudioSession::Stopped()
{
    return m_stopped;
}

void CHXSymbianAudioSession::SetVolume(const TInt aNewVolume)
{
    CHXBaseAudioSession::SetVol(aNewVolume);
}

TInt CHXSymbianAudioSession::GetVolume() const
{
    TInt lVolume = 0;
    lVolume = CHXBaseAudioSession::Volume();
    return lVolume;
}

TInt CHXSymbianAudioSession::GetMaxVolume() const
{
    TInt lVolume =0;
    lVolume = CHXBaseAudioSession::MaxVolume();
    return lVolume;
}

TInt CHXSymbianAudioSession::GetMinVolume() const
{
    TInt lVolume =0;
    lVolume = CHXBaseAudioSession::MinVolume();
    return lVolume;
}

TInt CHXSymbianAudioSession::Write(const IHXBuffer* pAudioBuf)
{
    TInt lRetval =0;
    lRetval = CHXBaseAudioSession::WriteAudio((IHXBuffer*)pAudioBuf);
    return lRetval;
}


TUint CHXSymbianAudioSession::GetTime()
{
    TUint ulRetval = 0;
    
    ulRetval = (TUint) CHXBaseAudioSession::GetAudioTime();
    return ulRetval;
}

TUint CHXSymbianAudioSession::GetBlocksBuffered()
{
    TInt ulRetval = 0;
    ulRetval = CHXBaseAudioSession::GetUnPlayedBlocks();
    return ulRetval;
}


void CHXSymbianAudioSession::RequestDeviceTakenNotification(CHXAudDevStatusObserver* pDevStatusObserver)
{
    if(pDevStatusObserver != NULL)
    {
        m_pAudDevStatusObserver = pDevStatusObserver;
    }
}

void CHXSymbianAudioSession::CancelDeviceTakenNotification()
{
    m_pAudDevStatusObserver = NULL;
}

TInt CHXSymbianAudioSession::SetSecureOutput(HXBOOL bSecureAudio)
{
    return CHXBaseAudioSession::SetSecureOutput(bSecureAudio);
}

//
// CHXSymbianAudioSession::InitializeComplete
//
// Overrrides the devSound observer to inform client
// 
//
void CHXSymbianAudioSession::InitializeComplete(TInt aError)
{
    HXLOGL2(HXLOG_ADEV, "CHXSymbianAudioSession::InitializeComplete(): err = %s", StringifyKErr(aError));

    m_bDevInitCompleted = TRUE;
    if(aError == KErrNone)
    {
        CHXBaseAudioSession::InitializeComplete(aError);
        aError = m_lastPlayError;
    }

    if(m_pActiveSchedulerWait->IsStarted())
    {
        m_pActiveSchedulerWait->AsyncStop();
    }
}

//
// CHXSymbianAudioSession::PlayError
//
// Overrrides the devSound observer to inform client
// of error cases
// 
//
void CHXSymbianAudioSession::PlayError(TInt aError)
{
    HXLOGL1(HXLOG_ADEV, "CHXSymbianAudioSession::PlayError(%d): err = %s", 
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
            {
                if(m_pAudDevStatusObserver != NULL)
                {
                    m_pAudDevStatusObserver->OnAudDevStatusChange(aError);
                }
                break;
            }
        default:
            if(m_pAudDevStatusObserver != NULL)
            {
                m_pAudDevStatusObserver->OnAudDevStatusChange(aError);
            }
            break;
        }
    } // End of if (aError != KErrNone)
}
