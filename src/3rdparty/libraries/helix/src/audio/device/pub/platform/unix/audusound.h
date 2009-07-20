/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): Matt Campbell <mattcampbell@pobox.com>
 *  
 * ***** END LICENSE BLOCK ***** */ 


//===================================================================================
// 
//   	audusound.h
// 
//   	CLASS: CAudioOutUSound
// 	
//   	Implements the sound subsystem for the Useful Sound Daemon.
//
//===================================================================================

#ifndef _AUDUSOUND_H_
#define _AUDUSOUND_H_


//-----------------------------------------------
// System includes.
//-----------------------------------------------
#include <usound.h>

//------------------------------------------------
// local includes
//------------------------------------------------
#include "hxcom.h"
#include "hxslist.h"

#include "audUnix.h"

class CAudioOutUSound : public CAudioOutUNIX
{
  public:
    CAudioOutUSound();
    virtual ~CAudioOutUSound();

  protected:

    
    //-------------------------------------------------------
    //
    // Pure virtuals from CAudioOutUNIX.
    //
    //-------------------------------------------------------
    virtual INT16 _Imp_GetAudioFd(void);

    //This ones important.
    virtual UINT64 _GetBytesActualyPlayed(void) const;

    //Devic specific method to set the audio device characteristics. Sample rate,
    //bits-per-sample, etc.
    //Method *must* set member vars. m_unSampleRate and m_unNumChannels.
    virtual HX_RESULT _SetDeviceConfig( const HXAudioFormat* pFormat );

    //Device specific method to test wether or not the device supports the
    //give sample rate. If the device can not be opened, or otherwise tested,
    //it should return RA_AOE_DEVBUSY.
    virtual HX_RESULT _CheckSampleRate( ULONG32 ulSampleRate );
    virtual HX_RESULT _CheckFormat( const HXAudioFormat* pFormat );


    //Device specific method to write bytes out to the audiodevice and return a
    //count of bytes written. 
    virtual HX_RESULT _WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount );

    //Device specific methods to open/close the mixer and audio devices.
    virtual HX_RESULT _OpenAudio();
    virtual HX_RESULT _CloseAudio();
    virtual HX_RESULT _OpenMixer();
    virtual HX_RESULT _CloseMixer();

    //Device specific method to reset device and return it to a state that it 
    //can accept new sample rates, num channels, etc.
    virtual HX_RESULT _Reset();

    //Device specific method to get/set the devices current volume.
    virtual UINT16    _GetVolume() const;
    virtual HX_RESULT _SetVolume(UINT16 volume);

    //Device specific method to drain a device. This should play the remaining
    //bytes in the devices buffer and then return.
    virtual HX_RESULT _Drain();

    //Device specific method to return the amount of room available on the
    //audio device that can be written without blocking.
    virtual HX_RESULT _GetRoomOnDevice( ULONG32& ulBytes) const;

    //-----------------------------------------------
    // Some helper methods.
    //-----------------------------------------------
    virtual HX_RESULT _Pause();
    virtual HX_RESULT _Resume();

    HXBOOL _IsSelectable() const;
    
    HXBOOL _HardwarePauseSupported() const;
  private:

    //protect the unintentional copy ctor.
    CAudioOutUSound( const CAudioOutUSound& );  //Not implemented.
      
    usound_output_t m_pStream;
};


#endif // _AUDUSOUND_H_



