/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: CHXBaseAudioSession.h,v 1.2 2007/04/13 23:41:14 rrajesh Exp $
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

#ifndef _BASE_AUDIO_SESSION_H_
#define _BASE_AUDIO_SESSION_H_

#include <e32base.h>
#include <e32des8.h>
#include <stdio.h>
#include <SoundDevice.h>
#include "CHXBaseDevSoundObserver.h"
#include "CHXSymbianAudioDevice.h"

#include "hxslist.h"

class CHXSimpleList;
class CHXMMFDevSound;

// Enable the following define for testing the audio device taken error
// Invokes PlayError(KErrDied) for every 5 seconds of play time.
//#define _DEBUG_TEST_AUDIO_DEVICE_TAKEN_
#define ERROR_TIMER_VAL_IN_MILLISECS   5000


class CHXBaseAudioSession : public CHXBaseDevSoundObserver
{
public:
    CHXBaseAudioSession();
    virtual ~CHXBaseAudioSession();

    // state
    enum State
    {
        CLOSED,
        OPEN_PENDING,
        STOPPED,
        PLAYINIT_PENDING,
        PLAYING,
        PAUSED
    };

    //MDevSoundObserver callbacks
    void InitializeComplete(TInt aError);
    void BufferToBeFilled(CMMFBuffer* aBuffer);
    void PlayError(TInt aError);


protected:

    TInt InitAudio(AudioDeviceInitArgs* pAudioDevInitArgs);
    TInt PlayAudio();
    TInt PauseAudio();
    TInt StopAudio();
    TInt WriteAudio(IHXBuffer* pAudioBuf);

    TInt GetAudioTime();
    TInt GetUnPlayedBlocks();
    void SetVol(TInt lVolume);
    TInt Volume() const;
    TInt MaxVolume() const;
    TInt MinVolume() const;
    

    
#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED

    void UpdateTimeSampleStats();
    void UpdateUnplayedSampleCount();
    void OnNewSampleCount(UINT32 sampleCount);
    void CheckSampleCountReset(TUint sampleCount);
    void OnResetSampleCount();
    
    //Sample time calculation routines
    void UpdateSamplesPlayedTime(TUint ulSamples);
    TUint GetSamplesPlayedTimeInMS(); 
    void ResetSamplesPlayedTime();

    void RetrieveUnplayedSamplesFromPastQueue();
    void RemoveOutdatedPacketsInPastQueue();
    void ClearPastQueue();

#endif
    
    // helpers
    TInt CreateDevSound();
    void DoPlayInit(HXBOOL setPriority = TRUE);
    void PrepareForDeviceReset();
    void FreePendingBuffers();
    void InitPlayInitPendingState();
    void InitPausedState();
    void InitStoppedState();
    void InitClosedState();
    void Trans(State state);


  protected:
    
    TInt                    m_lastPlayError;
    CMMFDevSound*           m_pStream;
    TMMFCapabilities        m_Settings;
    TUint                   m_sampleRate;
    TUint                   m_cbSample;
    TUint                   m_cbBlock;
    TUint                   m_cbBufferList;
    State                   m_state;
    HXBOOL                  m_deviceResetsOnPause;
    TMMFPrioritySettings    m_prioritySettings;
    HXBOOL                  m_bSecureAudio;

    // write related
    CHXSimpleList      m_bufferList;
    CMMFBuffer*        m_pPendingFillBuffer;
    TUint              m_cbFrontBufferWritten;
    TUint              m_samplesWritten;

    // Status flag to hold the PlayInit status.
    HXBOOL             m_bDevSoundInitErr;
    HXBOOL             m_bDevSoundOwned;

#ifdef _DEBUG_TEST_AUDIO_DEVICE_TAKEN_
    TInt               m_lErrCnt;
#endif    

#ifdef HELIX_CONFIG_SYMBIAN_SAMPLESPLAYED
    // audio time related
    TUint              m_lastSampleCount;
    TUint              m_unplayedSampleCount;
    HXBOOL             m_sampleCountResetPending;
    TUint              m_resetTriggerSampleCount;
    TUint              m_resetTriggerUnplayedCount;

    // Sample Played Time in secs
    TUint              m_ulTimePlayedInSec;              
    // Holds the remaining buffers that was not 
    // accounted for previous sample time calculation.
    TUint              m_ulBalanceSamples;

    CHXSimpleList      m_pastBufferList;
    TUint              m_cbPastBufferList;
#else    
    // Holds the total samples written to DevSound.
    // This value will be reset only on player stop
    TUint              m_TotalSamplesWritten;

#endif
    
};


//Function prototypes
const char * StringifyKErr(TInt lRetval);
const char * StringifyState(CHXBaseAudioSession::State state);

#endif // _BASE_AUDIO_SESSION_H_
