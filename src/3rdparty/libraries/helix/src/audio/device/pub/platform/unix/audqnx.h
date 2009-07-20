/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audqnx.h,v 1.7 2007/07/06 20:21:19 jfinnecy Exp $
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


/*******************************************************************
 *	audlinux.h
 *
 *	CLASS: CAudioOutQNX
 *	
 *	DESCRIPTION: Class definition for QNX-specific audio devices 
 *******************************************************************/

#ifndef _AUDLINUX
#define _AUDLINUX

#include <sys/asound.h>


// foward decls.
struct IHXCallback;

#include "hxcom.h"
#include "hxausvc.h"
#include "auderrs.h"
#include "hxaudev.h"

/*
 *      Used to indicate the current exported state of the virtual audio device.
 */
typedef enum audio_state
{
        RA_AOS_CLOSED,          // Normal non-playing state
        RA_AOS_OPENING,         // Opened, before first audio buffer play b
        RA_AOS_OPEN_PAUSED,     // Opened and paused
        RA_AOS_OPEN_PLAYING,    // Opened and after first audio buffer play begins
        RA_AOS_CLOSING          // We've been told to close after audio fin
}       AUDIOSTATE;


class CAudioOutQNX : public CHXAudioDevice 
{
public:
	CAudioOutQNX();

	/* Create friend class for scheduled playback callback.
	 */
        class HXPlaybackCountCb : public IHXCallback
        {
        private:
            LONG32              m_lRefCount;
                                ~HXPlaybackCountCb();


        public:
                                HXPlaybackCountCb(HXBOOL timed = TRUE);

	    HXBOOL               m_timed;
            CAudioOutQNX*    m_pAudioDeviceObject;

            /*
             *  IUnknown methods
             */
            STDMETHOD(QueryInterface)   (THIS_
                                            REFIID riid,
                                            void** ppvObj);

            STDMETHOD_(ULONG32,AddRef)  (THIS);

            STDMETHOD_(ULONG32,Release) (THIS);
            /*
             *  IHXCallback methods
             */
            STDMETHOD(Func)                     (THIS);
        };
	friend HXPlaybackCountCb;


protected:
	virtual ~CAudioOutQNX();
	HX_RESULT	_Imp_Init(IUnknown* pContext) {return HXR_OK;};
	HX_RESULT   	_Imp_Open( const HXAudioFormat* pFormat );
	HX_RESULT   	_Imp_Close( void );
	HX_RESULT	_Imp_Seek(ULONG32 ulSeekTime);
	HX_RESULT   	_Imp_Pause( void );
	HX_RESULT   	_Imp_Resume( void );
	HX_RESULT   	_Imp_Write( const HXAudioData* pAudioOutHdr );
	HX_RESULT   	_Imp_Reset( void );
	HX_RESULT  	_Imp_Drain( void );
	HXBOOL 		_Imp_SupportsVolume( void );
	UINT16		_Imp_GetVolume( void );
	HX_RESULT   	_Imp_SetVolume( const UINT16 uVolume );
	HX_RESULT   	_Imp_CheckFormat( const HXAudioFormat* pFormat);
	HX_RESULT   	_Imp_GetCurrentTime( ULONG32& ulCurrentTime);
	INT16		_Imp_GetAudioFd(void);
	UINT16		_NumberOfBlocksRemainingToPlay(void);
	ULONG32		_GetPlaybackBuffer(void);

	AUDIOERROR   	SetDeviceConfig( const HXAudioFormat* pFormat );

	/* Get number of bytes played.
	 */
	ULONG32 	GetPlaybackBytes(void);
	void 		DoTimeSyncs(void);
	HX_RESULT	ReschedPlaybackCheck(void);
	HXBOOL		BuffersEmpty(void);
	void            OpenMixer();
	void            CloseMixer();

private:
	int			m_wID;			// Audio device id
	int			mixm_wID;		// Audio mixer id; used for volume control
	int			m_wPCMChannel;	// PCM Channel id
	HXAudioData		m_rAudioHdr;		// Format of audio coming out of the decoder.	
   	UCHAR*  		m_pEbuf;		// Tmp buffer for 8-bit audio conversion
        AUDIOSTATE		m_wState;               // This is 
	AUDIOERROR		m_wLastError;   	// The last error d
	char			m_DevName[25]; /* Flawfinder: ignore */
	char			m_DevCtlName[25]; /* Flawfinder: ignore */
	HXBOOL			m_bMixerPresent;
	UINT16			m_wBlockSize;
	int			m_log;			// log file of pcm
	ULONG32			m_ulLastNumBytes;	// Number of bytes played back since last open
	ULONG32			m_ulBytesRemaining;	// Number of bytes remaining
	HXBOOL			m_bFirstWrite; 		// First write
	ULONG32			m_ulTotalWritten; 	// Total bytes written
	ULONG32			m_ulFragSize;		// The Fragment Size

        Timeval*		m_pPlaybackCountCBTime;
        ULONG32			m_PendingCallbackID;    // Used for fake time sync
        HXBOOL            	m_bCallbackPending;     // Used for fake time sync
        HXBOOL                    m_paused;
        CHXSimpleList*          m_pWriteList;
        UINT32                  m_last_audio_time;
        UINT32                  m_sample_rate;
        UINT32                  m_num_channels;
	UINT32                  m_ulLastTimeChecked;	
	UINT32                  m_ulLastTimeReturned;
	UINT32                  m_ulPauseBytes;

    //Some member vars to help keep track of data that is currently
    //in the device. We use this do a smart Pause/Resume for those
    //devices that can't do it in hardware.
    //block size is in audUNIX::m_wBlockSize

    //here is the buffer list to keep a copy of the data in the
    //device. We need to keep as many buffers, at m_wBlockSize in
    //size each, to cover the entire data space of the device.
    //CHXAudioDevice passes in chunks of size m_wBlockSize.
    //The device holds fragment_size*num_fragments as reported by
    //the device after it has had a chance to compute the frag size.
    //This occurs AFTER the other qualities like sample size, sample
    //rate have already been set.
    ULONG32 m_ulDeviceBufferSize;
    UCHAR*  m_pRollbackBuffer;
};


#endif 	//	_AUDIOOUTLINUX
