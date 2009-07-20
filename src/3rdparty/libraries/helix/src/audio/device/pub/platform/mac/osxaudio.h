/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: osxaudio.h,v 1.4 2007/07/06 20:21:18 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#pragma once

#include "hxtypes.h"

#include "hxcom.h"
#include "hxausvc.h"
#include "hxengin.h"
#include "hxaudev.h"
#include "hxslist.h"

#include <CoreAudio/CoreAudio.h>

class callback;

class CHXSimpleList;

#ifdef THREADS_SUPPORTED
class HXMutex;
#endif

class CAudioOutOSX : public CHXAudioDevice {

public:
    CAudioOutOSX	();
    virtual ~CAudioOutOSX	(void);
    
protected:
    HX_RESULT   _Imp_Init(IUnknown* pContext) {return HXR_OK;};
    HX_RESULT   _Imp_Open( const HXAudioFormat* pFormat );
    HX_RESULT   _Imp_Close( void );
    HX_RESULT	_Imp_Seek(ULONG32 ulSeekTime);
    HX_RESULT   _Imp_Pause( void );
    HX_RESULT   _Imp_Resume( void );
    HX_RESULT   _Imp_Write( const HXAudioData* pAudioOutHdr );
    HX_RESULT   _Imp_Reset( void );
    HX_RESULT  	_Imp_Drain( void );
    HXBOOL      _Imp_SupportsVolume( void );
    UINT16      _Imp_GetVolume();
    HX_RESULT   _Imp_SetVolume( const UINT16 uVolume );
    HX_RESULT   _Imp_CheckFormat( const HXAudioFormat* pFormat);
    HX_RESULT   _Imp_GetCurrentTime(ULONG32& ulCurrentTime);
    INT16       _Imp_GetAudioFd(void) {return 0;};

    UINT16      _NumberOfBlocksRemainingToPlay(void) { return 0; } // unused
    
private:

    OSStatus CoreAudioCallback(const AudioTimeStamp* inCurTime,
                    AudioBufferList* outOutputData);
    
    static OSStatus HALIOProc( AudioDeviceID theDevice,
                    const AudioTimeStamp* inCurTime,
                    const AudioBufferList* inInputData,
                    const AudioTimeStamp* inInputTime,
                    AudioBufferList* outOutputData,
                    const AudioTimeStamp* inOutputTime,
                    void* refcon
                    );
    
    AudioDeviceID   mAudioDevice;
    UINT32          mDeviceNumberOfChannels;
    UINT32          mInputNumberOfChannels;
    CHXSimpleList   mAudioBuffers;
    UINT32          mStaleBytesInFirstBuffer;
    
    UInt64      mCurrentTime;
    UInt64      mResetTimeNanos;
    UInt64      mElapsedNanos;
    UInt64      mElapsedNanosAtPause;
    UInt64      mNanoSecondsThatCoreAudioDrynessOccurred;
    UInt64      mAccumulatedNanoSecondsOfCoreAudioDryness;
    double      mCurrentVolume;
    
    static      HXMutex* zm_pMutex;
};

// extern "C" 