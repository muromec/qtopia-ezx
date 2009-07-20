/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audio_session-mmf.h,v 1.14 2007/04/03 18:25:17 rrajesh Exp $
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

#ifndef _AUDIO_SESSION_H_
#define _AUDIO_SESSION_H_

#include <e32base.h>
#include <e32des8.h>
#include <stdio.h>
#include <SoundDevice.h>

#include "../audio_svr.h"
#include "hxslist.h"
#include "CHXBaseAudioSession.h"

class CHXSimpleList;

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
class CHXSymbianAudioSvrSession 
    : public CSession2
    , public CHXBaseAudioSession
{ 
  public:
    CHXSymbianAudioSvrSession(HXSymbianAudioServer* pServer);
#else
class CHXSymbianAudioSvrSession 
    : public CSession
    , public CHXBaseAudioSession
{ 
  public:
    CHXSymbianAudioSvrSession(RThread& client, HXSymbianAudioServer* pServer);
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    virtual ~CHXSymbianAudioSvrSession();

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    static CHXSymbianAudioSvrSession* NewL(HXSymbianAudioServer* pServer);
virtual void ServiceL(const RMessage2& mesg);
#else
    static CHXSymbianAudioSvrSession* NewL(RThread& client,
                                       HXSymbianAudioServer* pServer);
    virtual void ServiceL(const RMessage& mesg);
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    
    void NotifyDeviceTaken();
        // Override CHXBaseDevSoundObserver callbacks
    void InitializeComplete(TInt aError);
    void PlayError(TInt aError);

private:
    // ServiceL() handlers
    void RequestDeviceTakenNotification();
    void CancelDeviceTakenNotification();
    TInt Init();
    TInt Write();

private:
    
    HXSymbianAudioServer*   m_pServer;    
    HXBOOL                  m_wantsNotify;

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    RMessage2 m_notifyRequest;
    RMessage2 m_InitMessage;
    RMessage2 m_SessionMessage;
    inline const RMessage2 Message();
#else
    RMessage                m_notifyRequest;
    RMessage                m_InitMessage;
#endif // HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
    
   
};

#endif // _AUDIO_SESSION_H_
