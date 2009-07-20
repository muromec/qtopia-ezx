/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxausvc.h,v 1.19 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXAUSVC_H_
#define _HXAUSVC_H_

#define     HX_MAX_VOLUME  100
#define     HX_INIT_VOLUME 50
#define     HX_MIN_VOLUME  0
// invalid volume can be used to check whether volume is initialized.
#define     HX_INVALID_VOLUME  200

/****************************************************************************
 *
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE   IHXAudioPlayer		    IHXAudioPlayer;
typedef _INTERFACE   IHXAudioPlayer2		    IHXAudioPlayer2;
typedef _INTERFACE   IHXAudioPlayerResponse	    IHXAudioPlayerResponse;
typedef _INTERFACE   IHXAudioStream		    IHXAudioStream;
typedef _INTERFACE   IHXAudioStream2		    IHXAudioStream2;
typedef _INTERFACE   IHXAudioStream3		    IHXAudioStream3;
typedef _INTERFACE   IHXAudioDevice		    IHXAudioDevice;
typedef _INTERFACE   IHXAudioDeviceResponse	    IHXAudioDeviceResponse;
typedef _INTERFACE   IHXOpaqueAudioDevice	    IHXOpaqueAudioDevice;
typedef _INTERFACE   IHXAudioHook		    IHXAudioHook;
typedef _INTERFACE   IHXAudioDeviceHookManager	    IHXAudioDeviceHookManager;
typedef _INTERFACE   IHXAudioStreamInfoResponse    IHXAudioStreamInfoResponse;
// $Private:
typedef _INTERFACE   IHXMultiPlayPauseSupport	    IHXMultiPlayPauseSupport;
typedef _INTERFACE   IHXAudioDeviceManager2	    IHXAudioDeviceManager2;
typedef _INTERFACE   IHXAudioResampler		    IHXAudioResampler;
typedef _INTERFACE   IHXAudioResamplerManager	    IHXAudioResamplerManager;
typedef _INTERFACE   IHXAudioPushdown2		    IHXAudioPushdown2;
// $EndPrivate.
typedef _INTERFACE   IHXVolume			    IHXVolume;
typedef _INTERFACE   IHXVolumeAdviseSink	    IHXVolumeAdviseSink;
typedef _INTERFACE   IHXDryNotification	    IHXDryNotification;
typedef _INTERFACE   IHXBuffer			    IHXBuffer;
typedef _INTERFACE   IHXValues			    IHXValues;

/****************************************************************************
 *
 *	Audio Services Data Structures
 */
typedef struct _HXAudioFormat
{
    UINT16	uChannels;	/* Num. of Channels (1=Mono, 2=Stereo, etc. */
    UINT16	uBitsPerSample;	/* 8 or 16				    */
    UINT32	ulSamplesPerSec;/* Sampling Rate			    */
    UINT16	uMaxBlockSize;	/* Max Blocksize			    */
} HXAudioFormat;

typedef enum _AudioStreamType
{
    STREAMING_AUDIO	= 0,
    INSTANTANEOUS_AUDIO = 1,
    TIMED_AUDIO		= 2,
    STREAMING_INSTANTANEOUS_AUDIO = 3
} AudioStreamType;

typedef struct _HXAudioData
{
    IHXBuffer*	    pData;		/* Audio data			    */ 
    ULONG32	    ulAudioTime;	/* Start time in milliseconds	    */
    AudioStreamType uAudioStreamType;
    _HXAudioData()
        : pData(NULL),
          ulAudioTime(0),
          uAudioStreamType(STREAMING_AUDIO)
        {};
} HXAudioData;

typedef enum _AudioDeviceHookType
{
    READ_ONLY_EARLY = 0,
    WRITABLE	    = 127,
    READ_ONLY_LATE  = 255
} AudioDeviceHookType;

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioPlayer
 * 
 *  Purpose:
 * 
 *  This interface provides access to the Audio Player services. Use this
 *  interface to create audio streams, "hook" post-mixed audio data, and to
 *  control volume levels.
 * 
 *  IID_IHXAudioPlayer:
 * 
 *  {00000700-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioPlayer, 0x00000700, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXAudioPlayer

DECLARE_INTERFACE_(IHXAudioPlayer, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioPlayer methods
     */
    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::CreateAudioStream
    *  Purpose:
    *		Call this to create an audio stream.
    */
    STDMETHOD(CreateAudioStream)    (THIS_
				    IHXAudioStream** /*OUT*/ pAudioStream
				    ) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::AddPostMixHook
    *  Purpose:
    *		Call this to hook audio data after all audio streams in this
    *		have been mixed.
    */
    STDMETHOD(AddPostMixHook)	(THIS_
				IHXAudioHook*	    /*IN*/ pHook,
				const HXBOOL	    /*IN*/ bDisableWrite,
				const HXBOOL	    /*IN*/ bFinal
				) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::RemovePostMixHook
    *  Purpose:
    *		Call this to remove an already added post hook.
    */
    STDMETHOD(RemovePostMixHook)    (THIS_
				    IHXAudioHook*    /*IN*/ pHook
				    ) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::GetAudioStreamCount
    *  Purpose:
    *		Get the number of audio streams currently active in the 
    *		audio player. Since streams can be added mid-presentation
    *		this function may return different values on different calls.
    *		If the user needs to know about all the streams as they get
    *		get added to the player, IHXAudioStreamInfoResponse should
    *		be implemented and passed in SetStreamInfoResponse.
    */
    STDMETHOD_(UINT16,GetAudioStreamCount) (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::GetAudioStream
    *  Purpose:
    *		Get an audio stream at position given. 
    */
    STDMETHOD_(IHXAudioStream*,GetAudioStream) (THIS_
						UINT16	/*IN*/ uIndex
						) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::SetStreamInfoResponse
    *  Purpose:
    *		Set a stream info response interface. A client must implement
    *		an IHXAudioStreamInfoResponse and then call this method with
    *		the IHXAudioStreamInfoResponse as the parameter. The audio
    *		player will call IHXAudioStreamInfoResponse::OnStreamsReady
    *		with the total number of audio streams associated with this 
    *		audio player.
    */
    STDMETHOD(SetStreamInfoResponse)	(THIS_
				IHXAudioStreamInfoResponse* /*IN*/ pResponse
					) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::RemoveStreamInfoResponse
    *  Purpose:
    *		Remove stream info response that was added earlier
    */
    STDMETHOD(RemoveStreamInfoResponse) (THIS_
				IHXAudioStreamInfoResponse* /*IN*/ pResponse
				) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::GetAudioVolume
    *  Purpose:
    *		Get the audio player's volume interface. This volume controls
    *		the volume level of all the mixed audio streams for this 
    *		audio player.
    */
    STDMETHOD_(IHXVolume*,GetAudioVolume) (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioPlayer::GetDeviceVolume
    *  Purpose:
    *		Get the audio device volume interface. This volume controls
    *		the audio device volume levels.
    */
    STDMETHOD_(IHXVolume*,GetDeviceVolume) (THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioPlayer2
 * 
 *  Purpose:
 * 
 *  This interface provides access to the Audio Player stream management 
 *  service.  This service allows individual audio streams running in the
 *  audio player timeline to be controlled while preserving the integrity 
 *  of playback and state of other audio streams in the entire audio session.
 * 
 *  IID_IHXAudioPlayer2:
 * 
 *  00000718-0901-11d1-8B06-00A024406D59
 * 
 */
DEFINE_GUID(IID_IHXAudioPlayer2, 0x00000718, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioPlayer2

#define AUD_PLYR_STR_REMOVE		1
#define AUD_PLYR_STR_PAUSE		2
#define AUD_PLYR_STR_RESUME		3
#define AUD_PLYR_STR_FLUSH		4
#define AUD_PLYR_STR_STOP		5
#define AUD_PLYR_STR_SET_REWIND_HINT	6

DECLARE_INTERFACE_(IHXAudioPlayer2, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioPlayer2 method
     */
    /************************************************************************
    *  Method:
    *      IHXAudioPlayer2::ManageAudioStreams
    *  Purpose:
    *	   Applies the requested action to the specified list of audio
    *	streams.  All audio streams provided must belong to the audio
    *	player they ManageAudioStreams command is given.
    *	The passed in params are used to provide parameters to the action
    *	given.  If no parameters are needed, NULL should be passed.
    *	There are currently no defined parameters for any of the currently
    *	defined actions.
    */
    STDMETHOD(ManageAudioStreams)    (THIS_
				      UINT16 uStreamCount,
				      IHXAudioStream** pAudioStreamArray,
				      UINT32 ulStreamAction,
				      IHXValues* pParams
				     ) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioPlayerResponse
 * 
 *  Purpose:
 * 
 *  This interface provides access to the Audio Player Response. Use this 
 *  to receive audio player playback notifications. Your implementation of
 *  OnTimeSync() is called with the current audio playback time (millisecs).
 *  This interface is currently to be used ONLY by the RMA engine internally.
 * 
 *  IID_IHXAudioPlayerResponse:
 * 
 *  {00000701-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioPlayerResponse, 0x00000701, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXAudioPlayerResponse

DECLARE_INTERFACE_(IHXAudioPlayerResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioPlayerResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioPlayerResponse::OnTimeSync
     *  Purpose:
     *	    This method is called with the current audio playback time.
     */
    STDMETHOD(OnTimeSync)   (THIS_
			    ULONG32 /*IN*/ ulTimeEnd
			    ) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXAudioStream
 * 
 *  Purpose:
 * 
 *  This interface provides access to an Audio Stream. Use this to play
 *  audio, "hook" audio stream data, and to get audio stream information.
 * 
 *  IID_IHXAudioStream:
 * 
 *      {00000702-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioStream, 0x00000702, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXAudioStream

DECLARE_INTERFACE_(IHXAudioStream, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
    /*
     *  IHXAudioStream methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioStream::Init
     *  Purpose:
     *	    Initialize an audio stream with the given audio format. The 
     *	    IHXValues contains stream identification and intializetion
     *	    state specification properties.  Normally, this method leaves
     *	    the stream in paused state and explict reumption is needed for
     *	    the stream to join the governing audio session.
     *	    The following properties are defined:
     *		audioDeviceReflushHint (UINT32):
     *		    0 - rewind data storage is not requested
     *			for this stream.  The storage may still be
     *			allocated due to the interaction with other streams
     *			that do request storage.
     *		    1 - rewind data storage is requested
     *		Resumed (UINT32):
     *		    0 - audio stream is to be initialized into a paused state
     *			Explicit resumption will be needed for audio stream to
     *			join the governing  audio session.
     *		    1 - audio stream is to be initialized into a resumed state
     *			(resumption is deferred if governing audio player is
     *			not resumed at the time of the call)
     *		Delay (UINT32):
     *		    N - indicates number of milliseconds the start of the audio
     *			stream is to be delayed from the start of the governing
     *			audio player timeline.  Unless "Resumed" property is
     *			used the state the stream will initialize to is
     *			automatically picked based on provided "Delay" value.
     *			"Resumed" state is picked if the provided "Delay" is
     *			sufficiently ahead to allow insertion of data at the
     *			back of audio pushdown queue.
     *			Otherwise, "Paused" state is picked.
     *			Audio stream will reject any data prior to the indicated
     *			"Delay".
     *		LiveSyncStartTime (UINT32):
     *		    N - provides the time base (first sample time-stamp) in 
     *			milliseconds of the data stream to be fed into
     *			the audio stream.  If "Delay" is provided as well,
     *			"Delay" provides the mapping of the incomming data stream
     *			time base to the governing audio player timeline.
     *			If "Delay" is not specified, the fed audio stream is not
     *			considered synchronized to the audio player time-line.
     *			In suhc case, audo stream will be joined into the 
     *			governing audio session synchronized to the time of 
     *			resumption invocation.
     */
    STDMETHOD(Init)	(THIS_
			const HXAudioFormat* /*IN*/ pAudioFormat,
			IHXValues*	/*IN*/  pValues
			) PURE;

    /************************************************************************
     *  Method:
     *      IHXAudioStream::Write
     *  Purpose:
     *	    Write audio data to Audio Services. 
     *	    
     *	    NOTE: If the renderer loses packets and there is no loss
     *	    correction, then the renderer should write the next packet 
     *	    using a meaningful start time.  Audio Services will play 
     *      silence where packets are missing.
     */
    STDMETHOD(Write)	(THIS_
			HXAudioData*		/*IN*/	pAudioData
			) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioStream::AddPreMixHook
    *  Purpose:
    *		Use this to "hook" audio stream data prior to the mixing.
    *		Set bDisableWrite to TRUE to prevent this audio stream data
    *		from being mixed with other audio stream data associated
    *		with this audio player.
    */
    STDMETHOD(AddPreMixHook) (THIS_
                             IHXAudioHook*    	/*IN*/ pHook,
			     const HXBOOL	      	/*IN*/ bDisableWrite
			     ) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioStream::RemovePreMixHook
    *  Purpose:
    *		Use this to remove an already added "hook".
    */
    STDMETHOD(RemovePreMixHook) (THIS_
                            	IHXAudioHook*    	/*IN*/ pHook
			     	) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioStream::AddDryNotification
    *  Purpose:
    *	    Use this to add a notification response object to get 
    *	    notifications when audio stream is running dry.
    */
    STDMETHOD(AddDryNotification)   (THIS_
                            	    IHXDryNotification* /*IN*/ pNotification
			     	    ) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioStream::GetStreamInfo
    *  Purpose:
    *		Use this to get information specific to this audio stream.
    */
    STDMETHOD_(IHXValues*,GetStreamInfo)      	(THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioStream::GetAudioVolume
    *  Purpose:
    *		Get the audio stream's volume interface. This volume controls
    *		the volume level for this audio stream.
    */
    STDMETHOD_(IHXVolume*,GetAudioVolume) (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXAudioDevice
 * 
 *  Purpose:
 * 
 *	Object that exports audio device API
 *	This interface is currently to be used ONLY by the RMA engine 
 *	internally.
 * 
 *  IID_IHXAudioDevice:
 * 
 *	{00000703-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioDevice, 0x00000703, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioDevice

DECLARE_INTERFACE_(IHXAudioDevice, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioDevice methods
     */

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::Open
    *  Purpose:
    *	    The caller calls this to open the audio device using the audio
    *	    format given.
    */
    STDMETHOD(Open) (THIS_
		    const HXAudioFormat*	/*IN*/ pAudioFormat,
		    IHXAudioDeviceResponse*	/*IN*/ pStreamResponse) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioDevice::Close
    *  Purpose:
    *		The caller calls this to close the audio device.
    */
    STDMETHOD(Close)	(THIS_
			const HXBOOL  /*IN*/ bFlush ) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::Resume
    *  Purpose:
    *	    The caller calls this to start or resume audio playback.
    */
    STDMETHOD(Resume)         (THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::Pause
    *  Purpose:
    *	    The caller calls this to pause the audio device. If bFlush is
    *	    TRUE, any buffers in the audio device will be flushed; otherwise,
    *	    the buffers are played.
    */
    STDMETHOD(Pause)         (THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::Write
    *  Purpose:
    *	    The caller calls this to write an audio buffer.
    */
    STDMETHOD(Write)         (THIS_
			     const HXAudioData* /*IN*/ pAudioData) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioDevice::InitVolume
    *  Purpose:
    *	    The caller calls this to inform the audio stream of the client's
    *	    volume range. The audio stream maps the client's volume range
    *	    into the audio device volume range. 
    *	    NOTE: This function returns TRUE if volume is supported by this 
    *	    audio device.
    */
    STDMETHOD_(HXBOOL,InitVolume)  (THIS_
				 const UINT16	/*IN*/ uMinVolume,
				 const UINT16	/*IN*/ uMaxVolume) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::SetVolume
    *  Purpose:
    *	    The caller calls this to set the audio device volume level.
    */
    STDMETHOD(SetVolume)         (THIS_
				 const UINT16    /*IN*/ uVolume) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::GetVolume
    *  Purpose:
    *	    The caller calls this to get the audio device volume level.
    */
    STDMETHOD_(UINT16,GetVolume) (THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::Reset
    *  Purpose:
    *	    The caller calls this to reset the audio device.
    */
    STDMETHOD(Reset)		(THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::Drain
    *  Purpose:
    *	    The caller calls this to drain the audio device.
    */
    STDMETHOD(Drain)		(THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::CheckFormat
    *  Purpose:
    *	    The caller calls this to check the input format with the
    *	    audio device format.
    */
    STDMETHOD(CheckFormat)  (THIS_
			    const HXAudioFormat* /*IN*/ pAudioFormat ) PURE;

    /************************************************************************
    *  Method:
    *	    IHXAudioDevice::GetCurrentAudioTime
    *  Purpose:
    *	    The caller calls this to get current system audio time.
    */
    STDMETHOD(GetCurrentAudioTime)  (THIS_
				    REF(ULONG32) /*OUT*/ ulCurrentTime) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *	IHXAudioDeviceResponse
 * 
 *  Purpose:
 * 
 *	Object that exports audio device Response API
 *	This interface is currently to be used ONLY by the RMA engine 
 *	internally.
 * 
 *  IID_IHXAudioDeviceResponse:
 * 
 *  {00000704-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioDeviceResponse, 0x00000704, 0x901, 0x11d1, 0x8b, 0x6, 
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioDeviceResponse

DECLARE_INTERFACE_(IHXAudioDeviceResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioDeviceResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioDeviceResponse::OnTimeSync
     *  Purpose:
     *      Notification interface provided by users of the IHXAudioDevice
     *      interface. This method is called by the IHXAudioDevice when
     *      audio playback occurs.
     */
    STDMETHOD(OnTimeSync)         (THIS_
                    		ULONG32         		/*IN*/ ulTimeEnd) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXOpaqueAudioDevice
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXOpaqueAudioDevice:
 * 
 *	{00000717-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXOpaqueAudioDevice, 0x00000717, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXOpaqueAudioDevice

DECLARE_INTERFACE_(IHXOpaqueAudioDevice, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXOpaqueAudioDevice methods
     */

    /************************************************************************
    *  Method:
    *	    IHXOpaqueAudioDevice::Open
    *  Purpose:
    *	    The caller calls this to open the audio device using the audio
    *	    format given.
    */
    STDMETHOD(Open)(THIS_
		    const HXAudioFormat*    /*IN*/ pAudioFormat, 
		    IHXAudioDeviceResponse* /*IN*/ pDeviceResponse, 
		    const char*		    /*IN*/ pszOpaqueType, 
		    IHXBuffer*		    /*IN*/ pOpaqueData) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioHook
 * 
 *  Purpose:
 * 
 *  Clients must implement this interface to access pre- or post-mixed 
 *  audio data. Use this interface to get post processed audio buffers and
 *  their associated audio format.
 *
 *  IID_IHXAudioHook:
 * 
 *  {00000705-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioHook, 0x00000705, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioHook

DECLARE_INTERFACE_(IHXAudioHook, IUnknown)
{
    /*
     *  IUnknown methods!
     */
    STDMETHOD(QueryInterface)		(THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *  IHXAudioHook methods
     */
    /************************************************************************
     *  Method:
     *      IHXAudioHook::OnInit
     *  Purpose:
     *      Audio Services calls OnInit() with the audio data format of the
     *	    audio data that will be provided in the OnBuffer() method.
     */
    STDMETHOD(OnInit)		(THIS_
                    		HXAudioFormat*	/*IN*/ pFormat) PURE;

    /************************************************************************
     *  Method:
     *      IHXAudioHook::OnBuffer
     *  Purpose:
     *      Audio Services calls OnBuffer() with audio data packets. The
     *	    renderer should not modify the data in the IHXBuffer part of
     *	    pAudioInData.  If the renderer wants to write a modified
     *	    version of the data back to Audio Services, then it should 
     *	    create its own IHXBuffer, modify the data and then associate 
     *	    this buffer with the pAudioOutData->pData member.
     */
    STDMETHOD(OnBuffer)		(THIS_
                    		HXAudioData*	/*IN*/   pAudioInData,
                    		HXAudioData*	/*OUT*/  pAudioOutData) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioDeviceHookManager
 * 
 *  Purpose:
 * 
 *  Allows setting audio hooks in the audio device itself. 
 *
 *  IID_IHXAudioDeviceHookManager:
 * 
 *  {00000715-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioDeviceHookManager, 0x00000715, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
	    0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioDeviceHookManager

DECLARE_INTERFACE_(IHXAudioDeviceHookManager, IUnknown)
{
    /*
     *  IUnknown methods!
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *  IHXAudioDeviceHookManager methods
     */
    /************************************************************************
    *  Method:
    *      IHXAudioDeviceHookManager::AddAudioDeviceHook
    *  Purpose:
    *      Last chance to modify data being written to the audio device.
    */
    STDMETHOD(AddAudioDeviceHook)	(THIS_
					IHXAudioHook*		/*IN*/ pHook,
					AudioDeviceHookType	/*IN*/ type
					) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioDeviceHookManager::RemoveAudioDeviceHook
    *  Purpose:
    *      Removes the audio device hook that was set with AddAudioDeviceHook.
    */
    STDMETHOD(RemoveAudioDeviceHook)	(THIS_
					IHXAudioHook*    /*IN*/ pHook
					) PURE;

    /************************************************************************
    *  Method:
    *      IHXAudioDeviceHookManager::ProcessHooks
    *  Purpose:
    *      Called by audio device implementations to process the hooks on a
    *      given audio buffer
    */
    STDMETHOD(ProcessAudioDeviceHooks)	(THIS_
					IHXBuffer*&    /*IN/OUT*/ pBuffer,
					HXBOOL&          /*OUT*/    bChanged
					) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioStreamInfoResponse
 * 
 *  Purpose:
 * 
 *  Clients must implement this interface when interested in receiving
 *  notification of the total number of streams associated with this
 *  audio player.
 *
 *  IID_IHXAudioStreamInfoResponse:
 * 
 *  {00000706-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioStreamInfoResponse, 0x00000706, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioStreamInfoResponse

DECLARE_INTERFACE_(IHXAudioStreamInfoResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioStreamInfoResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioStreamInfoResponse::OnStream
     *  Purpose:
     *	    The client implements this to get notification of streams 
     *	    associated with this player. Use 
     *	    AudioPlayer::SetStreamInfoResponse() to register your 
     *	    implementation with the AudioPlayer. Once player has been 
     *	    initialized, it will call OnStream() multiple times to pass all 
     *	    the streams. Since a stream can be added mid-presentation, 
     *	    IHXAudioStreamInfoResponse object should be written to handle 
     *	    OnStream() in the midst of the presentation as well.
     */
    STDMETHOD(OnStream) (THIS_
			IHXAudioStream* /*IN*/ pAudioStream) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXVolume
 * 
 *  Purpose:
 * 
 *  This interface provides access to Audio Services volume control. Use this
 *  interface to get, set, or receive notifications of volume changes. Audio
 *  Services implements IHXVolume for IHXAudioPlayer, IHXAudioStream and 
 *  for the audio device. Clients can use the IHXVolume interface to get/set
 *  volume levels of each audio stream, to get/set volume levels for the 
 *  audio player's mixed data, or to get/set the volume levels of the audio 
 *  device. See AudioStream::GetStreamVolume() (TBD), AudioPlayer::
 *  GetAudioVolume() and AudioPlayer::GetDeviceVolume().
 *
 *  IID_IHXVolume:
 * 
 *  {00000707-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXVolume, 0x00000707, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXVolume

DECLARE_INTERFACE_(IHXVolume, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXVolume methods
     */
    /************************************************************************
     *  Method:
     *      IHXVolume::SetVolume
     *  Purpose:
     *	    Call this to set the volume level.
     */
    STDMETHOD(SetVolume) (THIS_
                         const	UINT16	/*IN*/ uVolume ) PURE;

    /************************************************************************
     *  Method:
     *      IHXVolume::GetVolume
     *  Purpose:
     *	    Call this to get the current volume level.
     */
    STDMETHOD_(UINT16,GetVolume)   (THIS) PURE;

     /************************************************************************
     *  Method:
     *      IHXVolume::SetMute
     *  Purpose:
     *	    Call this to mute the volume.
     */
    STDMETHOD(SetMute)   (THIS_
                         const	HXBOOL	/*IN*/ bMute ) PURE;

     /************************************************************************
     *  Method:
     *      IHXVolume::GetMute
     *  Purpose:
     *	    Call this to determine if the volume is muted.
     *	  
     */
    STDMETHOD_(HXBOOL,GetMute)       (THIS) PURE;

     /************************************************************************
     *  Method:
     *      IHXVolume::AddAdviseSink
     *  Purpose:
     *	    Call this to register an IHXVolumeAdviseSink. The advise sink
     *	    methods: OnVolumeChange() and OnMuteChange() are called when
     *	    ever IHXVolume::SetVolume() and IHXVolume::SetMute() are
     *	    called.
     */
    STDMETHOD(AddAdviseSink)	(THIS_
				 IHXVolumeAdviseSink* /*IN*/	pSink
				) PURE;

     /************************************************************************
     *  Method:
     *      IHXVolume::RemoveAdviseSink
     *  Purpose:
     *	    Call this to unregister an IHXVolumeAdviseSink. Use this when
     *	    you are no longer interested in receiving volume or mute change
     *	    notifications.
     */
    STDMETHOD(RemoveAdviseSink)	(THIS_
				 IHXVolumeAdviseSink* /*IN*/	pSink
				) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXVolumeAdviseSink
 * 
 *  Purpose:
 * 
 *  This interface provides access to notifications of volume changes. A 
 *  client must implement this interface if they are interested in receiving 
 *  notifications of volume level changes or mute state changes. A client must 
 *  register their volume advise sink using IHXVolume::AddAdviseSink().
 *  See the IHXVolume interface.
 * 
 *  IID_IHXVolumeAdviseSink:
 * 
 *  {00000708-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXVolumeAdviseSink, 0x00000708, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXVolumeAdviseSink

DECLARE_INTERFACE_(IHXVolumeAdviseSink, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXVolumeAdviseSink methods
     */

    /************************************************************************
     *  Method:
     *      IHXVolumeAdviseSink::OnVolumeChange
     *  Purpose:
     *	    This interface is called whenever the associated IHXVolume
     *	    SetVolume() is called.
     */
    STDMETHOD(OnVolumeChange)	(THIS_ 
				const UINT16 uVolume
				) PURE;

    /************************************************************************
     *  Method:
     *      IHXVolumeAdviseSink::OnMuteChange
     *  Purpose:
     *	    This interface is called whenever the associated IHXVolume
     *	    SetMute() is called.
     *    
     */
    STDMETHOD(OnMuteChange)     (THIS_
				const HXBOOL bMute
				) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioLevelNormalization
 * 
 *  IID_IHXAudioLevelNormalization:
 * 
 *  {00000716-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioLevelNormalization, 0x00000716, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioLevelNormalization

DECLARE_INTERFACE_(IHXAudioLevelNormalization, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioLevelNormalization methods
     */
    STDMETHOD(SetSoundLevelOffset)  (THIS_ INT16 nOffset) PURE; 
    STDMETHOD_(INT16, GetSoundLevelOffset)(THIS) PURE; 
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXDryNotification
 * 
 *  Purpose:
 * 
 *  Audio Renderer should implement this if it needs notification when the 
 *  audio stream is running dry. 
 *
 *  IID_IHXDryNotification:
 * 
 *  {00000709-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXDryNotification, 0x00000709, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDryNotification

DECLARE_INTERFACE_(IHXDryNotification, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXDryNotification methods
     */

    /************************************************************************
     *  Method:
     *      IHXDryNotification::OnDryNotification
     *  Purpose:
     *	    This function is called when it is time to write to audio device 
     *	    and there is not enough data in the audio stream. The renderer can
     *	    then decide to add more data to the audio stream. This should be 
     *	    done synchronously within the call to this function.
     *	    It is OK to not write any data. Silence will be played instead.
     */
    STDMETHOD(OnDryNotification)    (THIS_
				    UINT32 /*IN*/ ulCurrentStreamTime,
				    UINT32 /*IN*/ ulMinimumDurationRequired
				    ) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioDeviceManager
 * 
 *  Purpose:
 * 
 *  Allows the default audio device to be replaced.
 *
 *  IID_IHXAudioDeviceManager:
 * 
 *  {0000070A-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioDeviceManager, 0x0000070A, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioDeviceManager

DECLARE_INTERFACE_(IHXAudioDeviceManager, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioDeviceManager methods
     */

    /**********************************************************************
     *  Method:
     *      IHXAudioDeviceManager::Replace
     *  Purpose:
     *  This is used to replace the default implementation of the audio
     *  device by the given audio device interface. 
     */
    STDMETHOD(Replace)         (THIS_
		    IHXAudioDevice*    /*IN*/ pAudioDevice) PURE;

    /**********************************************************************
     *  Method:
     *      IHXAudioDeviceManager::Remove
     *  Purpose:
     *  This is used to remove the audio device given to the manager in
     *  the earlier call to Replace.
     */
    STDMETHOD(Remove)         (THIS_
		    IHXAudioDevice*    /*IN*/ pAudioDevice) PURE;

    /************************************************************************
    *  Method:
    *   IHXAudioDeviceManager::AddFinalHook
    *  Purpose:
    *	One last chance to modify data being written to the audio device.
    *	This hook allows the user to change the audio format that
    *   is to be written to the audio device. This can be done in call
    *   to OnInit() in IHXAudioHook.
    */
    STDMETHOD(SetFinalHook)	(THIS_
				IHXAudioHook*	    /*IN*/ pHook
				) PURE;

    /************************************************************************
    *  Method:
    *   IHXAudioDeviceManager::RemoveFinalHook
    *  Purpose:
    *	Remove final hook
    */
    STDMETHOD(RemoveFinalHook)	(THIS_
				IHXAudioHook*    /*IN*/ pHook
				) PURE;

   /************************************************************************
    *  Method:
    *   IHXAudioDeviceManager::GetAudioFormat
    *  Purpose:
    *	Returns the audio format in which the audio device is opened.
    *	This function will fill in the pre-allocated HXAudioFormat 
    *	structure passed in.
    */
    STDMETHOD(GetAudioFormat)   (THIS_
			        HXAudioFormat*	/*IN/OUT*/pAudioFormat) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXAudioCrossFade
 * 
 *  Purpose:
 *
 *  This interface can be used to cross-fade two audio streams. It is exposed 
 *  by IHXAudioPlayer
 * 
 *  IID_IHXAudioCrossFade:
 * 
 *  {0000070B-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioCrossFade, 0x0000070B, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioCrossFade

DECLARE_INTERFACE_(IHXAudioCrossFade, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
    /*
     *  IHXAudioCrossFade methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioCrossFade::CrossFade
     *  Purpose:
     *	    Cross-fade two audio streams.
     *	    pStreamFrom		    - Stream to be cross faded from
     *	    pStreamTo		    - Stream to be cross faded to
     *	    ulFromCrossFadeStartTime- "From" Stream time when cross fade is 
     *				      to be started
     *	    ulToCrossFadeStartTime  - "To" Stream time when cross fade is to 
     *				      be started
     *	    ulCrossFadeDuration	    - Duration over which cross-fade needs
     *				      to be done
     *	    
     */
    STDMETHOD(CrossFade)	(THIS_
				IHXAudioStream* pStreamFrom,
				IHXAudioStream* pStreamTo,
				UINT32		 ulFromCrossFadeStartTime,
				UINT32		 ulToCrossFadeStartTime,
				UINT32		 ulCrossFadeDuration) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXAudioStream2
 * 
 *  Purpose:
 *
 *  This interface contains some last-minute added audio stream functions
 * 
 *  IID_IHXAudioStream2:
 * 
 *  {0000070C-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioStream2, 0x0000070C, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioStream2

DECLARE_INTERFACE_(IHXAudioStream2, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
    /*
     *  IHXAudioStream2 methods
     */
   /************************************************************************
    *  Method:
    *      IHXAudioStream2::RemoveDryNotification
    *  Purpose:
    *	    Use this to remove itself from the notification response object
    *	    during the stream switching.
    */
    STDMETHOD(RemoveDryNotification)   (THIS_
				   IHXDryNotification* /*IN*/ pNotification
			     		) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream2::GetAudioFormat
    *  Purpose:
    *	    Returns the input audio format of the data written by the 
    *	    renderer. This function will fill in the pre-allocated 
    *	    HXAudioFormat structure passed in.
    */
    STDMETHOD(GetAudioFormat)   (THIS_
			        HXAudioFormat*	/*IN/OUT*/pAudioFormat) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXAudioStream3
 * 
 *  Purpose:
 *
 *  This interface contains low-level audio stream controll functions.
 *  Normally, audio stream should not be controlled directy but rather through
 *  higher level objects such as audio player which allows audio player to
 *  properly manage all audio streams in response to actions issued.
 *  Such actions are necessary since conceptual audio stream data extends
 *  into the audio mix buffer down-stream from the audio stream objects and
 *  only higher level object such as audio player can manage that portion of
 *  the audio stream properly.
 *  These low level objects are useful for advanced uses and manipulation of
 *  audio subsystem.
 * 
 *  IID_IHXAudioStream3:
 * 
 *  {00000719-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioStream3, 0x00000719, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioStream3

DECLARE_INTERFACE_(IHXAudioStream3, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
    /*
     *  IHXAudioStream3 methods
     */
   /************************************************************************
    *  Method:
    *      IHXAudioStream3::Stop()
    *  Purpose:
    *	   Stops the mixing of the stream into the audio session, 
    *	   flushes all data present in the audio stream and de-initializes 
    *	   it.  Stream must be re-initailized before resuming.
    */
    STDMETHOD(Stop)		(THIS) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream3::Pause
    *  Purpose:
    *	   Pauses the playback of audio stream thus suspending mixing of
    *	   the stream data into the audio session while retaining all
    *	   data within the stream.  On resumption, mixing will resume at
    *	   same point it was suspended assuming no other intervening
    *	   commands were given.
    */
    STDMETHOD(Pause)		(THIS) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream3::Resume
    *  Purpose:
    *	   Resumes the playback of audio stream thus resuming mixing of
    *	   the stream data into the audio session while retaining all
    *	   data within the stream.  On resumption, mixing will resume at
    *	   same point it was suspended assuming no other intervening
    *	   commands were given.  The stream will not resume if the
    *	   governing audio player is not resumed (playing) at the time of
    *	   the call.  That conditions is not cosidered an error and
    *	   success code will be returned.
    */
    STDMETHOD(Resume)		(THIS) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream3::Flush
    *  Purpose:
    *	   Flushes all data from the audio stream.  If the stream is paused
    *	   at the time of the call, the audio stream is also rewound to the
    *	   current playback time and made ready to receive new data for
    *	   mixing at or after the current playback time.
    */
    STDMETHOD(Flush)		(THIS) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream3::Rewind
    *  Purpose:
    *	   Rewinds the audio stream to the current playback time while
    *	   restoring any data that might have been stored in the audio
    *	   stream after being mixed into the mix buffer.
    */
    STDMETHOD(Rewind)		(THIS) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream3::Reconfig
    *  Purpose:
    *	   Reconfigures already initialized audio stream with the indicated
    *	   parameters.  The following parameters are currently defined:
    *		"audioDeviceReflushHint" (UINT32):  0 = no data storage for
    *							rewind of the audio
    *							session should be kept
    *						    1 = data storage for
    *							rewind of the audio
    *							session should be kept
    */
    STDMETHOD(Reconfig)		(THIS_ IHXValues* pParams) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream3::IsResumed
    *  Purpose:
    *	   Informs the caller if the audio stream is resumed (in playing
    *	   state) and thus if its data is being mixed into the audio session
    *	   given that the owning audio player and session are resumed as 
    *	   well.
    */
    STDMETHOD_(HXBOOL, IsResumed)   (THIS) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioStream3::IsRewound
    *  Purpose:
    *	   Informs the caller if the audio stream is rewound and thus is
    *	   capable of receiving data at or after the current playback time.
    *	   If stream is not rewound, it is capable of playing (mixing)
    *	   data timed for the tail of the mix-buffer which is pushdown worth
    *	   ahead of the current playback time 
    *	   (see IHXAudioPushdown2::GetCurrentAudioDevicePushdown).
    */
    STDMETHOD_(HXBOOL, IsRewound)   (THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXAudioPushdown
 * 
 *  Purpose:
 *
 *  This interface can be used to setup the audio pushdown time.
 * 
 *  IID_IHXAudioPushdown:
 * 
 *  {0000070D-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioPushdown, 0x0000070D, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioPushdown

DECLARE_INTERFACE_(IHXAudioPushdown, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
    /*
     *  IHXAudioPushdown methods
     */
   /************************************************************************
    *  Method:
    *      IHXAudioPushdown::SetAudioPushdown
    *  Purpose:
    *	    Use this to set the minimum audio pushdown value in ms.
    *	    This is the amount of audio data that is being written 
    *	    to the audio device before starting playback.
    */
    STDMETHOD(SetAudioPushdown)   (THIS_
				   UINT32 /*IN*/ ulAudioPushdown
			     	  ) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXAudioHookManager
 * 
 *  Purpose:
 *
 *  This interface can be used to add a hook at the audio device layer.
 * 
 *  IID_IHXAudioHookManager:
 * 
 *  {0000070E-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioHookManager, 0x0000070E, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioHookManager

DECLARE_INTERFACE_(IHXAudioHookManager, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
   /*
    *  IHXAudioHookManager methods
    */
   /************************************************************************
    *  Method:
    *      IHXAudioHookManager::AddHook
    *  Purpose:
    *	    Use this to add a hook 
    */
    STDMETHOD(AddHook)   (THIS_
			  IHXAudioHook* /*IN*/ pHook
			  ) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioHookManager::RemoveHook
    *  Purpose:
    *	    Use this to remove a hook 
    */
    STDMETHOD(RemoveHook) (THIS_
			  IHXAudioHook* /*IN*/ pHook
			  ) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXMultiPlayPauseSupport
 * 
 *  Purpose:
 *
 *  This interface can be used to control whether audio services handles multi-player pause / rewind support
 * 
 *  IID_IHXMultiPlayPauseSupport:
 * 
 *  {0000070F-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMultiPlayPauseSupport, 
	    0x0000070F, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXMultiPlayPauseSupport

DECLARE_INTERFACE_(IHXMultiPlayPauseSupport, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
   /*
    *  IHXMultiPlayPauseSupport methods
    */
    STDMETHOD_(HXBOOL,GetDisableMultiPlayPauseSupport)	(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioDeviceManager2
 * 
 *  Purpose:
 * 
 *  Audio Device Manager extension
 *
 *  IID_IHXAudioDeviceManager2:
 * 
 *  {00000710-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioDeviceManager2, 0x00000710, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioDeviceManager2

DECLARE_INTERFACE_(IHXAudioDeviceManager2, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioDeviceManager2 methods
     */

    /**********************************************************************
     *  Method:
     *      IHXAudioDeviceManager2::IsReplacedDevice
     *  Purpose:
     *  This is used to determine if the audio device has been replaced.
     */
    STDMETHOD_(HXBOOL, IsReplacedDevice)         (THIS) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioResampler
 * 
 *  Purpose:
 * 
 *  Audio Resampler
 *
 *  IID_IHXAudioResampler:
 * 
 *  {00000711-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioResampler, 0x00000711, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioResampler

DECLARE_INTERFACE_(IHXAudioResampler, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioResampler methods
     */

    /**********************************************************************
     *  Method:
     *      IHXAudioResampler::Resample
     *  Purpose:
     *  Will produce 1 output frame for every (upFactor/downFactor) inputs
     *  frames, straddling if not an integer.  Works down to 1 sample/call.
     * 
     *  Returns actual number of output frames.
     ***********************************************************************/

    STDMETHOD_(UINT32, Resample)    (   THIS_
                                        UINT16*	pInput, 
                                        UINT32	ulInputBytes, 
                                        UINT16*	pOutput) PURE;

    /**********************************************************************
     *  Method:
     *      IHXAudioResampler::Requires
     *  Purpose:
     *  Returns number of input frames required to produce this number
     *  of output frames, given the current state of the filter.
     */

    STDMETHOD_(UINT32, Requires)    (   THIS_
                                        UINT32	ulOutputFrames) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioResamplerManager
 * 
 *  Purpose:
 * 
 *  Audio Resampler Manager
 *
 *  IID_IHXAudioResamplerManager:
 * 
 *  {00000712-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioResamplerManager, 0x00000712, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioResamplerManager

DECLARE_INTERFACE_(IHXAudioResamplerManager, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXAudioResamplerManager methods
     *
     */
    STDMETHOD(CreateResampler)	(THIS_
				HXAudioFormat		    inAudioFormat,
				REF(HXAudioFormat)	    outAudioFormat,
				REF(IHXAudioResampler*)    pResampler) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioPushdown2
 * 
 *  Purpose:
 * 
 *  Audio PushDown access methods
 *
 *  IID_IHXAudioPushdown2:
 * 
 *  {00000713-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioPushdown2, 0x00000713, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioPushdown2

DECLARE_INTERFACE_(IHXAudioPushdown2, IHXAudioPushdown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioPushdown::SetAudioPushdown
    *  Purpose:
    *	    Use this to set the minimum audio pushdown value in ms.
    *	    This is the amount of audio data that is being written 
    *	    to the audio device before starting playback.
    */
    STDMETHOD(SetAudioPushdown)			(THIS_
						UINT32	/*IN */ ulAudioPushdown) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioPushdown2::GetAudioPushdown
    *  Purpose:
    *	    Use this to get the minimum audio pushdown value in ms.
    *	    This is the amount of audio data that is being written 
    *	    to the audio device before starting playback.
    */
    STDMETHOD(GetAudioPushdown)			(THIS_
						REF(UINT32) /*OUT*/ ulAudioPushdown) PURE;

   /************************************************************************
    *  Method:
    *      IHXAudioPushdown2::GetCurrentAudioDevicePushdown
    *  Purpose:
    *	    Use this to get the audio pushed down to the audio device and haven't
    *	    been played yet
    */
    STDMETHOD(GetCurrentAudioDevicePushdown)   (THIS_
						REF(UINT32) /*OUT*/ ulAudioPusheddown) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXAudioMultiChannel
 * 
 *  Purpose:
 * 
 *  Multi-channel audio support
 *
 *  IID_IHXAudioMultiChannel:
 * 
 *  {00000714-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXAudioMultiChannel, 0x00000714, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAudioMultiChannel

DECLARE_INTERFACE_(IHXAudioMultiChannel, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

   /*
    *  IHXAudioMultiChannel methods
    */
    STDMETHOD_(HXBOOL,GetMultiChannelSupport)    (THIS) PURE;
};
// $EndPrivate.


#if defined(HELIX_FEATURE_TIMELINE_WATCHER)

// Timeline watcher.
// {211A3CAE-F1DA-4678-84D5-0F12E7B1D8C6}
DEFINE_GUID(IID_IHXTimelineWatcher, 
            0x211a3cae, 0xf1da, 0x4678, 0x84, 0xd5, 0xf, 0x12, 0xe7, 0xb1, 0xd8, 0xc6);
#undef INTERFACE
#define INTERFACE IHXTimelineWatcher
DECLARE_INTERFACE_(IHXTimelineWatcher, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(OnPause) (THIS) PURE;
    STDMETHOD(OnResume) (THIS) PURE;
    STDMETHOD(OnClose) (THIS) PURE;
    STDMETHOD(OnTimeSync) (THIS_ UINT32 currentTime ) PURE;
};

// TimelineManager
// {9ED91BC3-9E92-46bb-A094-6C8B9416CFB6}
DEFINE_GUID(IID_IHXTimelineManager, 
            0x9ed91bc3, 0x9e92, 0x46bb, 0xa0, 0x94, 0x6c, 0x8b, 0x94, 0x16, 0xcf, 0xb6);
#undef INTERFACE
#define INTERFACE IHXTimelineManager
DECLARE_INTERFACE_(IHXTimelineManager, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;
    
    STDMETHOD(AddTimelineWatcher) (THIS_ IHXTimelineWatcher* ) PURE;
    STDMETHOD(RemoveTimelineWatcher) (THIS_ IHXTimelineWatcher* ) PURE;
};
#endif /* #if defined(HELIX_FEATURE_TIMELINE_WATCHER) */


/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXClockSource
 * 
 *  Purpose:
 * 
 *  Renderers can support this interface to provide clock to audio services
 *
 *  IID_IHXClockSource:
 * 
 *  {5D0EE207-9792-48ac-BCE3-488676B6C5BB}
 * 
 */
DEFINE_GUID(IID_IHXClockSource, 0x5d0ee207, 0x9792, 0x48ac, 0xbc, 0xe3,
            0x48, 0x86, 0x76, 0xb6, 0xc5, 0xbb);

#undef INTERFACE
#define INTERFACE IHXClockSource

DECLARE_INTERFACE_(IHXClockSource, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXClockSource methods
    STDMETHOD(OpenClockSource)    (THIS_ UINT32 ulStartTime) PURE;
    STDMETHOD(ResumeClockSource)  (THIS) PURE;
    STDMETHOD(GetClockSourceTime) (THIS_ REF(UINT32) rulTime) PURE;
    STDMETHOD(PauseClockSource)   (THIS) PURE;
    STDMETHOD(CloseClockSource)   (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXClockSourceManager
 * 
 *  Purpose:
 * 
 *  Audio player supports this interface to manage clock sources
 *
 *  IID_IHXClockSourceManager:
 * 
 *  {59766625-4691-4c30-9F38-CCA992BA12A4}
 * 
 */
DEFINE_GUID(IID_IHXClockSourceManager, 0x59766625, 0x4691, 0x4c30, 0x9f, 0x38,
            0xcc, 0xa9, 0x92, 0xba, 0x12, 0xa4);

#undef INTERFACE
#define INTERFACE IHXClockSourceManager

DECLARE_INTERFACE_(IHXClockSourceManager, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXClockSourceManager methods
    STDMETHOD(RegisterClockSource)   (THIS_ IHXClockSource* pSource) PURE;
    STDMETHOD(UnregisterClockSource) (THIS_ IHXClockSource* pSource) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXAudioPlayer)
DEFINE_SMART_PTR(IHXAudioPlayer2)
DEFINE_SMART_PTR(IHXAudioPlayerResponse)
DEFINE_SMART_PTR(IHXAudioStream)
DEFINE_SMART_PTR(IHXAudioDevice)
DEFINE_SMART_PTR(IHXAudioDeviceResponse)
DEFINE_SMART_PTR(IHXOpaqueAudioDevice)
DEFINE_SMART_PTR(IHXAudioHook)
DEFINE_SMART_PTR(IHXAudioDeviceHookManager)
DEFINE_SMART_PTR(IHXAudioStreamInfoResponse)
DEFINE_SMART_PTR(IHXVolume)
DEFINE_SMART_PTR(IHXVolumeAdviseSink)
DEFINE_SMART_PTR(IHXAudioLevelNormalization)
DEFINE_SMART_PTR(IHXDryNotification)
DEFINE_SMART_PTR(IHXAudioDeviceManager)
DEFINE_SMART_PTR(IHXAudioCrossFade)
DEFINE_SMART_PTR(IHXAudioStream2)
DEFINE_SMART_PTR(IHXAudioStream3)
DEFINE_SMART_PTR(IHXAudioPushdown)
DEFINE_SMART_PTR(IHXAudioHookManager)
DEFINE_SMART_PTR(IHXMultiPlayPauseSupport)
DEFINE_SMART_PTR(IHXAudioDeviceManager2)
DEFINE_SMART_PTR(IHXAudioResampler)
DEFINE_SMART_PTR(IHXAudioResamplerManager)
DEFINE_SMART_PTR(IHXAudioPushdown2)
DEFINE_SMART_PTR(IHXAudioMultiChannel)
DEFINE_SMART_PTR(IHXClockSource)
DEFINE_SMART_PTR(IHXClockSourceManager)

#if defined(HELIX_FEATURE_TIMELINE_WATCHER)
DEFINE_SMART_PTR(IHXTimelineWatcher)
DEFINE_SMART_PTR(IHXTimelineManager)
#endif /* defined(HELIX_FEATURE_TIMELINE_WATCHER) */

#endif  /* _HXAUSVC_H_ */
