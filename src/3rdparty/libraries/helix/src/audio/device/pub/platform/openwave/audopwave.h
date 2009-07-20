/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audopwave.h,v 1.7 2007/07/06 20:21:18 jfinnecy Exp $
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

#ifndef _AUDOPWAVE
#define _AUDOPWAVE

#define OPWV_SUPPORT_LOW_LEVEL_SOUND_API
#include <op_sound.h>

#include "hxengin.h"
#include "hxcom.h"
#include "hxausvc.h"
#include "auderrs.h"
#include "hxaudev.h"
#include "hxaudply.h"

#define MAX_DEV_NAME 255


class CAudioOutOpenwave : public CHXAudioDevice
{
  public:
    CAudioOutOpenwave();

//	OpCmdEvent		*m_pDoneEvent;
    
    
    //-1 is usually considered to be no file descriptor.
    static const int NO_FILE_DESCRIPTOR;

    static const int MAX_VOLUME;
    
    // Used to indicate the current exported state of the virtual audio device.
    typedef enum audio_state
    {
        RA_AOS_CLOSED,          // Normal non-playing state
        RA_AOS_OPENING,         // Opened, before first audio buffer play b
        RA_AOS_OPEN_PAUSED,     // Opened and paused
        RA_AOS_OPEN_PLAYING,    // Opened and after first audio buffer play begins
        RA_AOS_CLOSING          // We've been told to close after audio fin
    } AUDIOSTATE;
    
    inline HXBOOL IsOpen() const { return (m_wState==RA_AOS_OPEN_PLAYING || m_wState==RA_AOS_OPEN_PAUSED);}

    void iodone(); // callback
    
    // Create friend class for scheduled playback callback.
    class HXPlaybackCountCB : public IHXCallback
    {
      private:
        HXBOOL           m_timed;
        LONG32         m_lRefCount;
        CAudioOutOpenwave* m_pAudioDeviceObject;
        
        virtual ~HXPlaybackCountCB();
      public:
        //Ctors
        HXPlaybackCountCB(CAudioOutOpenwave* object, HXBOOL timed = TRUE) :
            m_timed(timed),
            m_lRefCount(0),
            m_pAudioDeviceObject( object )
        {};

        //IUnkown methods
        STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)  (THIS);
        STDMETHOD_(ULONG32,Release) (THIS);
        
        //IHXCallback methods
        STDMETHOD(Func) (THIS);
    };
    friend class CAudioOutOpenwave::HXPlaybackCountCB;

    //To get the last return code.
    HX_RESULT GetLastError() const { return m_wLastError; }
    
  protected:

    //--------------------------------------------------
    // NON-DEVICE SPECIFIC METHODS
    //--------------------------------------------------
    virtual ~CAudioOutOpenwave();
    
    HX_RESULT	_Imp_Init(IUnknown* pContext) {return HXR_OK;};
    HX_RESULT _Imp_Open( const HXAudioFormat* pFormat );         //Implemented.
    HX_RESULT _Imp_Close( void );                                 //Implemented.
    HX_RESULT _Imp_Write( const HXAudioData* pAudioOutHdr );     //Implemented.
    HX_RESULT _Imp_Reset( void );                                 //Implemented.
    HX_RESULT _Imp_Drain( void );                                 //Implemented.
    HX_RESULT _Imp_SetVolume( const UINT16 uVolume );             //Implemented.
    HX_RESULT _Imp_GetCurrentTime( ULONG32& ulCurrentTime);       //Implemented.
    HXBOOL      _Imp_SupportsVolume( void );                        //Implemented.
    UINT16    _Imp_GetVolume( void );                             //Implemented.

    //Not used but defined in CHXAudioDevice as pure virtual.
    UINT16    _NumberOfBlocksRemainingToPlay(void);               //Implemented.
    void      _initAfterContext();                                //Implemented.

    //-------------------------------------------
    // DEVICE SPECIFIC METHODS.
    //-------------------------------------------
    virtual HX_RESULT _Imp_Seek(ULONG32 ulSeekTime); //default provided
    virtual HX_RESULT _Imp_Resume( void );           //default provided
    virtual HX_RESULT _Imp_Pause( void );            //default provided
    virtual HX_RESULT _Imp_CheckFormat( const HXAudioFormat* pFormat); //default provided.

    //Device specific methods to Pause/Resume the device. If a device can't handle
    //the pause/resume in hardware then it must return RA_AOE_NOTSUPPORTED, the
    //default. In this case we use a 'rollback' method.
    virtual HX_RESULT _Pause();                     //default provided.  
    virtual HX_RESULT _Resume();                    //default provided.  

    //Now a couple of routines to help us determine the type
    //of hardware device we are dealing with.
    
    //A method to determine if the audio devices file descriptor,
    //if it has one, should be added to the client core's select loop.
    //The default implementation is 'yes it should be added'.
    virtual HXBOOL _IsSelectable() const;

    //A mehtod to let us know if the hardware supports puase/resume.
    //We can use this to remove unneeded memcpys and other expensive
    //operations. The default implementation is 'No, not supported'.
    virtual HXBOOL _HardwarePauseSupported() const;

    //The Imp_Write method just fills up our write list. This method
    //is the one that actually sends the data to the audio device.
    ULONG32 _PushBits();
    
    //-------------------------------------------------------
    // These Device Specific methods must be implemented 
    // by the platform specific sub-classes.
    //-------------------------------------------------------

    //What should we do if there are no file descriptors?
    virtual INT16 _Imp_GetAudioFd(void);
        
    //Device specific method to return number of bytes played.
    //default implementation is to do a low precision calculation
    //to compute an estimate. Device capable of computing this 
    //acturatly will override this method. This should never 
    //return a number greater than m_ulTotalWritten.
    virtual UINT64 _GetBytesActualyPlayed(void) const;

    //Device specific method to set the audio device characteristics. Sample rate,
    //bits-per-sample, etc.
    //Method *must* set member vars. m_unSampleRate, m_unNumChannels and
    //m_ulDeviceBufferSize.
    virtual HX_RESULT _SetDeviceConfig( const HXAudioFormat* pFormat );

    //deprecated.....
    virtual HX_RESULT _CheckSampleRate( ULONG32 ulSampleRate );

    //Device specific method to write bytes out to the audiodevice and return a
    //count of bytes written. 
    virtual HX_RESULT _WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount );

    //Device specific methods to open/close the mixer and audio devices.
    virtual HX_RESULT _OpenAudio();
    virtual HX_RESULT _CloseAudio();
    virtual HX_RESULT _OpenMixer();
    virtual HX_RESULT _CloseMixer();

    //--------------------------------------------------
    // Other stuff.
    //--------------------------------------------------
    void      DoTimeSyncs(void);
    HX_RESULT ReschedPlaybackCheck(void);
    HXPlaybackCountCB* m_pCallback;
    
    AUDIOSTATE        m_wState;               // This is 
    HXBOOL              m_bMixerPresent;
    UINT16            m_wBlockSize;
    UINT64            m_ulLastNumBytes;       // Number of bytes played back since last open
    UINT32	      m_ulLastTick;		// Ticks in ms when the audio playback starts
    HXBOOL              m_bFirstWrite;          // First write
    HXBOOL              m_bInitCallback;        // Initialize the callback once
    UINT64            m_ulTotalWritten;       // Total bytes written
    Timeval*          m_pPlaybackCountCBTime;
    ULONG32           m_PendingCallbackID;    // Used for fake time sync
    HXBOOL              m_bCallbackPending;     // Used for fake time sync

    CHXSimpleList*    m_pWriteList;
    UINT32            m_unSampleRate;
    UINT32            m_unNumChannels;
    UINT32            m_unBitsPerSample;
    UINT32            m_unBytesPerSec;
    mutable HX_RESULT m_wLastError;           // The last error d

    //Some member vars to help keep track of data that is currently
    //in the device. We use this do a smart Pause/Resume for those
    //devices that can't do it in hardware.
    //block size is in audOpenwave::m_wBlockSize

    //here is the buffer list to keep a copy of the data in the
    //device. We need to keep as many buffers, at m_wBlockSize in
    //size each, to cover the entire data space of the device.
    //CHXAudioDevice passes in chunks of size m_wBlockSize.
    //The device holds fragment_size*num_fragments as reported by
    //the device after it has had a chance to compute the frag size.
    //This occurs AFTER the other qualities like sample size, sample
    //rate have already been set.
    ULONG32 m_ulDeviceBufferSize; 
    ULONG32 m_ulSampleBufSize; 
    
  private:
    
    //protect the unintentional copy ctor.
    CAudioOutOpenwave( const CAudioOutOpenwave& );

    op_sound_handle*	m_pSndDev;	// Device's audio channel, used for all calls to Openwave's API
    op_sound_buffer	m_SndBuf[10];	// Sound sample buffer descriptors, used for writing data to the device
    int		m_nCurBuf;	// Current buffer to fill, 0 or 1. (double buffering)
    bool	m_bWriteDone;

};

#endif  //_AudioOutOpwave
