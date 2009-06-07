/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audlinux_alsa.h,v 1.5 2005/03/14 19:43:24 bobclark Exp $
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
 * Contributor(s):  ljp <ljp@llornkcor.com>
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef _AUDLINUXALSA
#define _AUDLINUXALSA

//------------------------------------------------
// local includes
//------------------------------------------------
#include "hxcom.h"
#include "hxslist.h"
#include "audUnix.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

class CAudioOutLinuxAlsa : public CAudioOutUNIX
{
  public:
    CAudioOutLinuxAlsa();
    virtual ~CAudioOutLinuxAlsa();

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
    virtual HX_RESULT _Pause();
    virtual HX_RESULT _Resume();

    //Device specific method to get/set the devices current volume.
    virtual UINT16    _GetVolume() const;
    virtual HX_RESULT _SetVolume(UINT16 volume);

    //Device specific method to drain a device. This should play the remaining
    //bytes in the devices buffer and then return.
    virtual HX_RESULT _Drain();

    //Device specific method to return the amount of room available on the
    //audio device that can be written without blocking.
    virtual HX_RESULT _GetRoomOnDevice( ULONG32& ulBytes) const;

    //A method to let us know if the hardware supports puase/resume.
    //We can use this to remove unneeded memcpys and other expensive
    //operations. The default implementation is 'No, not supported'.
    virtual HXBOOL _HardwarePauseSupported() const;
    
  private:
  
    //protect the unintentional copy ctor.
    CAudioOutLinuxAlsa( const CAudioOutLinuxAlsa & );  //Not implemented.

    /* The constness imposed by the base class is a lost cause here --
       make all functions const, all members mutable. */
    void HandleXRun(void) const;
    void HandleSuspend(void) const;

    HX_RESULT GetBytesActuallyPlayedUsingTStamps (UINT64 &nBytesPlayed) const;
    HX_RESULT GetBytesActuallyPlayedUsingDelay   (UINT64 &nBytesPlayed) const;
    HX_RESULT GetBytesActuallyPlayedUsingAvail   (UINT64 &nBytesPlayed) const;
    HX_RESULT GetBytesActuallyPlayedUsingTimer   (UINT64 &nBytesPlayed) const;
    
    mutable snd_pcm_t*   m_pAlsaPCMHandle;
    mutable snd_mixer_t* m_pAlsaMixerHandle;
    mutable snd_mixer_elem_t* m_pAlsaMixerElem;

    mutable IHXBuffer* m_pPCMDeviceName;
    mutable IHXBuffer* m_pMixerDeviceName;
    mutable IHXBuffer* m_pMixerElementName;
    
    mutable HXBOOL m_bHasHardwarePauseAndResume;

    mutable UINT64 m_nBytesPlayedBeforeLastTrigger;

    mutable UINT64 m_nLastBytesPlayed;

    mutable snd_timestamp_t m_tstampLastTrigger;
    mutable HXBOOL m_bGotInitialTrigger;

    mutable HXBOOL m_bUseMMAPTStamps;
};


#endif  //_AUDIOOUTLINUXALSA
