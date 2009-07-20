/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audio_svr.h,v 1.10 2009/02/27 22:58:43 shivnani Exp $
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
 
/*
 *  Description: 
 *  
 *  This file contains three class definitions; HXSymbianAudioServer,
 *  HXSymbianAudioSession, and HXSymbianAudioClient. Their purpose is
 *  to allow us to hide the Symbian CMdaAudioOutputStream interface.
 *
 */
#ifndef _AUDIO_SVR_H
#define _AUDIO_SVR_H

#include <stdio.h>
#include <e32base.h>
#include <bacntf.h>
#include <mmf/common/Mmfbase.h>

#include "hxcom.h"
#include "hxausvc.h"
#include "CHXSymbianAudioDevice.h"
#include "audio_svr_cntxt.h"
#include "AudDevStatusObserver.h"

/*
 * class HXSymbianAudioServer
 *
 * This class specializes the Symbian OS CServer class in order to
 * implement a server to respond to requests from one or more clients.
 */
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
class HXSymbianAudioServer : public CServer2 {
#else
class HXSymbianAudioServer : public CServer {
#endif  // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
public:
    HXSymbianAudioServer();
    virtual ~HXSymbianAudioServer();

    enum Commands {
	EAS_Init,
	EAS_Write,
	EAS_CancelWrite,
	EAS_Play,
	EAS_Pause,
	EAS_GetTime,
	EAS_GetBlocksBuffered,
	EAS_SetVolume,
	EAS_GetVolume,
	EAS_GetMaxVolume,
	EAS_GetMinVolume,
	EAS_Stop,
	EAS_RequestDeviceTakenNotification,
	EAS_CancelDeviceTakenNotification
    };

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    virtual CSession2* NewSessionL(const TVersion& aVersion,
                                   const RMessage2& /*aMessage*/) const;
#else
    virtual CSharableSession* NewSessionL(const TVersion& aVersion) const;
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY

    void AddSession();
    void DelSession();
    TInt RunError(TInt aError);
    void NotifyDeviceTaken();


private:
    int m_sessionCount;		// session count
};


/* 
 * class HXSymbianAudioClient
 *
 */
class HXSymbianAudioClient : public RSessionBase
, public MHXSymbianAudioDevice
, public CHXAudDevStatusObserver
{
public:
    HXSymbianAudioClient();
    virtual ~HXSymbianAudioClient();

    virtual TInt Open();
    virtual void CloseDevice();
	// initialize audio device
    virtual TInt Init(int sampleRate, int channels,
              TMMFPrioritySettings* pPrioritySettings, HXBOOL bSecureAudio);

    virtual TInt Write(const IHXBuffer* pAudioBuf);

    // get the current time played out in ms
    virtual TUint GetTime();
	// get the number of blocks buffered in device
    virtual TUint GetBlocksBuffered();
	// enable playback
    virtual TInt Play();
	// pause playback
    virtual void Pause();
	// stop playback
    virtual void Stop();
	// returns true if not playing
    virtual  HXBOOL Stopped();
	// get the current volume
    virtual TInt GetVolume() const;
	// get the max volume
    virtual TInt GetMaxVolume() const;
	// get the min volume
    virtual TInt GetMinVolume() const;
	// set the current volume
    virtual void SetVolume(TInt volume);

    virtual void RequestDeviceTakenNotification(
        CHXAudDevStatusObserver* pDevStatusObserver);

    virtual void CancelDeviceTakenNotification();

    // Handler for aud dev status change
    virtual void OnAudDevStatusChange(TInt lStatus);

private:
    // start a session with the server
    TInt Connect(const TDesC& ServerName);
    // Activate the device status monitor
    void ActivateDevTakenNotification();

private:
    HXBOOL m_stopped;
    HXSymbianAudioServerContext* m_pServerCtx;
    CHXAudDevStatusObserver*     m_pAudDevStatusObserver;
    CHXAudDevTakenNotifier*      m_pDevTakenNotifier;

};

#endif // _AUDIO_SVR_H

