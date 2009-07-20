/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audio_session-mda.h,v 1.7 2007/07/06 20:21:13 jfinnecy Exp $
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

#ifndef _AUDIO_SESSION_H_
#define _AUDIO_SESSION_H_

#include <e32base.h>
#include <MdaAudioOutputStream.h>
#include <mda/common/audio.h>

#include <stdio.h>
#include "../audio_svr.h"
#include "hxslist.h"

class CHXSimpleList;

class HXSymbianAudioTimeline
{
public:
    HXSymbianAudioTimeline();
    ~HXSymbianAudioTimeline();
    
    void Reset(UINT32 ulByteRate); // called to reset the state of this object

    void OnWrite(UINT32 ulBytes); // called when bytes are written to the device

    UINT32 GetPlaybackTime(); // Returns current playback time

    UINT32 GetWrittenTime() const;   // Returns number of ms of data written
    UINT32 GetWallclockTime() const; // Returns number of ms elapsed since the first
                                     // non-zero device position was returned

    void OnPlay();
    void OnPauseOrUnderflow();

    inline HXBOOL NeedDeviceTime() const;
    void SetDeviceTime(UINT32 ulDeviceTimeMs);

private:
    inline UINT32 GetMs(UINT32 ulSec, UINT32 ulSubSec) const; // Computes milliseconds

    void ClearWritten(); // Clears written bytes state
    
    UINT32 m_ulByteRate;        // Bytes/second of audio data. 
                                // Stereo 44100 16 bit audio would be 176400
    UINT32 m_ulSecWritten;      // Seconds of data written
    UINT32 m_ulSubSecBytes;     // Fraction of a second written in bytes.
    UINT32 m_ulBaseSec;         // Base time seconds
    UINT32 m_ulBaseSubSecBytes; // Base time fraction in bytes

    UINT32 m_ulLastGetTime; // return value of last GetTime() call.
                            // This is used to force monotonically increasing
                            // time values.

    UINT32 m_ulDevTimeMs;   // First non-zero position returned from the device
    UINT32 m_ulWallclockTime; // Wallclock time associated with m_ulDevTimeMs
};

/*
 * class HXSymbianAudioSession
 *
 */
class HXSymbianAudioSession : public CSession,
			      public MMdaAudioOutputStreamCallback { 
public:
    HXSymbianAudioSession(RThread& client, HXSymbianAudioServer* pServer);
    virtual ~HXSymbianAudioSession();

    static HXSymbianAudioSession* NewL(RThread& client,
				       HXSymbianAudioServer* pServer);
    virtual void ServiceL(const RMessage& mesg);
    void NotifyDeviceTaken();

protected:
    void Init();
    void Play();
    void Pause();
    void Write();
    void GetTime();
    void GetBlocksBuffered();
    void SetVolume();
    void GetVolume();
    void GetMaxVolume();
    void GetMinVolume();
    void Stop();

    void RequestDeviceTakenNotification();
    void CancelDeviceTakenNotification();

    virtual void MaoscOpenComplete(TInt aError);
    virtual void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
    virtual void MaoscPlayComplete(TInt aError);

    void DoCleanup();

    UINT32 GetByteRate() const;
    UINT32 GetDeviceMs();

    HXBOOL ReadyToWrite() const;
    TInt WriteNextBuffer();

private:
				// audio request server
    HXSymbianAudioServer* m_pServer;	
				// audio stream interface
    CMdaAudioOutputStream* m_pStream;
				// audio device sample rate
    TMdaAudioDataSettings::TAudioCaps m_sampleRate;
				// audio device channels
    TMdaAudioDataSettings::TAudioCaps m_channels;
				// audio settings
    TMdaAudioDataSettings m_settings; 
				// temp data pointer
    TPtr8 m_pData;
				// async write message
    RMessage m_writeMessage;

    RMessage m_notifyRequest;
				// flag indication notification requested
    bool m_wantsNotify;
				// reason device was taken
    int m_reason;
				// true if device has been opened
    bool m_open;
				// true if Complete() called on Write() msg
    HXSymbianAudioTimeline m_timeline;

    CHXSimpleList m_bufferList;
    HXBOOL m_bPaused;
    HXBOOL m_bWritePending;
};

#endif // _AUDIO_SESSION_H_
