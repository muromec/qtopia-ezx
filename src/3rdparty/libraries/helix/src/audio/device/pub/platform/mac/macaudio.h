/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macaudio.h,v 1.8 2007/07/06 20:21:18 jfinnecy Exp $
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

#pragma once

#include "hxtypes.h"
//#include "callback.h"
#ifndef _MAC_MACHO
#include <SoundInput.h>
#endif

#include "hxcom.h"
#include "hxausvc.h"
#include "hxengin.h"
#include "hxaudev.h"


/*--------------------------------------------------------------------------
|	CMacWaveFormat class
--------------------------------------------------------------------------*/
class CMacWaveFormat {

	friend	class	CWaveHeader;
	
	protected:
		
		short			numChannels;
		Fixed			sampleRate;
		short			sampleSize;
		OSType			compressionType;
		short			baseFrequency;
		
	public:
		CMacWaveFormat		(void);
		~CMacWaveFormat		(void);
	
		OSErr	SetUpSound	(SndListHandle	sndHandle,
					long		numBytes,
					short		*headerLen,
					long		*headerOffset);
										 
		static	OSErr	SetHeaderLength	(SoundHeaderPtr	pSoundHeader,
					long		numBytes);

		void 	SetFormat (ULONG32 sampleRate,
					UINT16 		channels,
					UINT16  	bitsPerSample);
												 
	private:
		void	SetFormatDflt	(void);
	};

/*--------------------------------------------------------------------------
|	CAudioOutMac class
--------------------------------------------------------------------------*/
class CAudioOutMac;

typedef struct audio_init_params
{
	float 	sampleRate;
	UINT16 	channels;
	UINT16 	bitsPerSample;
	UINT16 	volume;
	UINT16 	numBufs;
	UINT16 	bufSize;
	
} AUDIO_INIT_PARAMS;

#define WAVE_DEFAULT_SAMPLE_RATE 	8000.
#define WAVE_DEFAULT_BITSPERSAMPLE 	16
#define WAVE_DEFAULT_CHANNELS 		1

/*--------------------------------------------------------------------------
|	CWaveHeader
--------------------------------------------------------------------------*/
class CWaveHeader {
	
	private:
		
		static	const	Size			kSndHeaderSize;
		
		enum PlayState {
			kFreeState,
			kQueuedState,
			
			kFnordState
			};
		
		long			soundA5;
		short			state;
		Size			cbPlaying;
		ULONG32			timeEnd;
		UINT16			mMaxPlay;
		
		SndListHandle	sndHandle;
		short			mHeaderLen;
		SoundHeaderPtr	mSoundHeader;
		
		float			mSampleRate;
		UINT16			mBitsPerSample;
		UINT16			mChannels;
		
	public:
	
		enum ReleaseCode {
			kCallBackRelease,
			kAbortRelease,
			kResetRelease,
			
			kFnordRelease
			};

		CAudioOutMac		*waveOut;
		static	UINT16	WAVE_BLOCKSIZE;

		static	SndChannelPtr	NewChannel		(void);
		static	OSErr		DisposeChannel	(SndChannelPtr	chan);

		CWaveHeader		(void);
		~CWaveHeader	(void);
		
		inline	void	SetOutput(CAudioOutMac	*waveOutArg)
					{waveOut = waveOutArg;};
		inline		Boolean	Available(void) const
					{return (kFreeState == state);};
		inline		UINT16	MaxPlayLength(void) const
					{return mMaxPlay;};

		OSErr		Allocate(UINT16	inMaxPlay,
					float   sampleRate,
					UINT16  channels,
					UINT16  bitsPerSample);
								 
		void		Release	(short	param1, HXBOOL bSendTimeSync = TRUE);
		OSErr		PlayBuffer(char		*pData,
					   UINT16	cbPlay,
					   ULONG32	timeEndArg);
		
		inline	void 	*get_buffer 	(void)
					{return (void *)((*(Handle) sndHandle) + mHeaderLen);};

		
	
	private:

		static	pascal	void	Callback(SndChannelPtr	chan,
					 SndCommand		*cmd);
	};
	
/*--------------------------------------------------------------------------
|	CAudioOutMac
--------------------------------------------------------------------------*/

class callback;

class CHXSimpleList;

#ifdef THREADS_SUPPORTED
class HXMutex;
#endif

// foward decls.
struct IHXCallback;
struct IHXInterruptState;

class CAudioOutMac : public CHXAudioDevice {

public:
	/* Create friend class for scheduled playback callback.
	 */
        class HXPauseResumeCb : public IHXCallback
        {
        private:
            LONG32              m_lRefCount;
                                ~HXPauseResumeCb();


        public:
                                HXPauseResumeCb(CAudioOutMac* pAudioObject);

	    CallbackHandle   m_CallbackHandle;
            CAudioOutMac*    m_pAudioDeviceObject;
            CHXSimpleList*   m_pCommandList;
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
            void Enter(HXBOOL bResume);
        };
	friend class HXPauseResumeCb;
	HXBOOL			OkToPauseResume(HXBOOL bToBeResumed/* TRUE - Resume. FALSE - Pause */);


	private:
	
    struct GestaltDeferredStruct
    {
	UINT32* quitting;
	UINT32* pending;
	ProcessSerialNumber psn;
    };
    
		static	UINT16 NUM_WAVEHDRS;

		friend	class	CWaveHeader;
		
		UINT16			mcbPlaying;
		
		SndChannelPtr		chan;
		CMacWaveFormat		wf;
		CWaveHeader*		mlpWaveHdrs;
		
		Boolean			mPaused;		// Kludge to prevent paused callbacks
		UINT16			m_uNumBlocksInDevice;
		
		static UINT16 		m_uzVolume;
		
		Boolean			m_bFirstPacket;
		ULONG32			m_ulTimeOfFirstPacket;
		
		CHXSimpleList*		m_pPendingCallbackList;
		HXPauseResumeCb*	m_pCallback;
		
		DeferredTask*		mDeferredTaskRec;
		
		double			m_millisecondsIntoClip;

#ifdef THREADS_SUPPORTED
		static HXMutex*	zm_pMutex;
#endif

  	        static	DeferredTaskUPP gDeferredTask;	
		static  pascal  void	DeferredTaskCallback(long	param);
	

	public:

#if 1	
		enum PlayResults {
			playSuccess = 0,
			playError,
			playFull,
			
			playFnord
			};
		
		enum OpenResults {
			openSuccess = 0,
			openDriver,
			openMemory,
			
			openFnord
			};
#endif
			
	CAudioOutMac	();
	~CAudioOutMac	(void);
	
	void   ProcessCmd(SndCommand* cmd, HXBOOL bSendTimeSync = TRUE);
	void   AddToThePendingList(void* pNode);
	
	HXBOOL   m_bDeferredTaskPending;
	IHXInterruptState* m_pInterruptState;
	
	// m_bAudioCallbacksAreAlive and m_bIsQuitting need to be UINT32s because
	// later interrupt-related crash-preventing Gestalt-based code makes that
	// assumption.
	UINT32	m_bAudioCallbacksAreAlive;
	UINT32	m_bIsQuitting;

protected:
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
	UINT16   	_Imp_GetVolume();
	HX_RESULT   	_Imp_SetVolume( const UINT16 uVolume );
	HX_RESULT   	_Imp_CheckFormat( const HXAudioFormat* pFormat);
	HX_RESULT	_Imp_GetCurrentTime(ULONG32& ulCurrentTime);
	INT16		_Imp_GetAudioFd(void) {return 0;};

	UINT16		_NumberOfBlocksRemainingToPlay(void);
										 
	void		Abort(void);
	
	void		CleanupPendingLists(void);
					
	inline	CWaveHeader*GetWaveHeader(short	index)
		{return mlpWaveHdrs ? mlpWaveHdrs + index : NULL;};

private:
		
	void	ReleaseBlocks	(short	releaseCode);
	OSErr	DoImmediate	(short	cmd,
				 short	param1 = 0,
				 long	param2 = 0L);
		
	void	DonePlaying	(UINT16	cbPlayedArg,
				 ULONG32 timeEndArg,
				 short	waveTask,
				 HXBOOL bSendTimeSync = TRUE);
		
inline	void	StartedPlaying	(UINT16	cbPlayed)
		{mcbPlaying += cbPlayed;};


};
	
