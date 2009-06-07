/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudstr.h,v 1.20 2007/03/23 00:42:08 milko Exp $
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

#ifndef _HXAUSTR_H_
#define _HXAUSTR_H_

#include "hxcomm.h"
#include "hxcore.h"

#include "mixengine.h"

// forward decls
class	CHXAudioPlayer;
class	CHXSimpleList;
class   CHXMapPtrToPtr;
class	CHXBuffer;
#if defined(HELIX_FEATURE_CROSSFADE)
class   CrossFader;
#endif /* HELIX_FEATURE_CROSSFADE */
#ifdef _MACINTOSH
class	HXAudioMacQueue;
#endif

_INTERFACE IHXAudioStream;
_INTERFACE IHXAudioStream2;
_INTERFACE IHXBuffer;
_INTERFACE IHXValues;
_INTERFACE IHXAudioHook;
_INTERFACE IHXDryNotification;
_INTERFACE IHXRealAudioSync;
_INTERFACE IHXCommonClassFactory;
_INTERFACE IHXUpdateProperties;

struct RealAudioBytesToTimeStamp
{
    RealAudioBytesToTimeStamp()
    {
	m_ulOutNumBytes		= 0.;
	m_ulOrigTimestamp	= 0;
	m_ulTimestamp		= 0;
	m_ulInTimestamp		= 0;
	m_ulInEndTime		= 0;
	m_ulDuration		= 0;
    }

    double  m_ulOutNumBytes;
    UINT32  m_ulOrigTimestamp; // needed for multi-player pause/resume support
    UINT32  m_ulTimestamp;
    UINT32  m_ulInTimestamp;
    UINT32  m_ulInEndTime;
    UINT32  m_ulDuration;
};

struct HXAudioInfo;
/****************************************************************************
 *
 *  Class:
 *
 *      CHXAudioStream
 *
 *  Purpose:
 *
 *      PN implementation of audio stream object.
 *
 */
class CHXAudioStream: public IHXAudioStream, 
		      public IHXRealAudioSync,
		      public IHXAudioStream2,
		      public IHXAudioStream3,
		      public CAudioSvcSampleConverter,
		      public IHXCommonClassFactory,
#ifdef HELIX_FEATURE_VOLUME
		      public IHXVolumeAdviseSink,
#endif 
                      public IHXUpdateProperties
{
protected:
    void    _Pause(HXBOOL bPlayerPause = FALSE);
    void    _Resume(HXBOOL bPlayerResume = FALSE);

public:
    HXBOOL IsRealAudioStream();
    HXBOOL IsAudible();
    
    void SetLastAdjustedTimeDiff();

    CHXAudioStream( CHXAudioPlayer* owner, IUnknown* pContext );

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                            REFIID riid,
                            void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *      IHXAudioStream methods
     */

    STDMETHOD(Init)	(THIS_
			const HXAudioFormat*  pAudioFormat,
			IHXValues*      pValues
			);

    STDMETHOD(Write)	(THIS_
			HXAudioData*	pAudioData
			);

    STDMETHOD(AddPreMixHook)	(THIS_
				IHXAudioHook*	pHook,
				const HXBOOL		bDisableWrite
				);

    STDMETHOD(RemovePreMixHook)	(THIS_
				IHXAudioHook*	pHook
				);



#ifdef HELIX_FEATURE_VOLUME
    // 
    // IHXVolumeAdviseSink methods.
    //
    STDMETHOD(OnVolumeChange)(THIS_ const UINT16 uVolume);
    STDMETHOD(OnMuteChange)(THIS_ const HXBOOL bMute);
#endif

    /************************************************************************
    *  Method:
    *      IHXAudioStream::AddDryNotification
    *  Purpose:
    *	    Use this to add a notification response object to get notifications
    *	    when audio stream is running dry.
    */
    STDMETHOD(AddDryNotification)   (THIS_
                            	    IHXDryNotification* /*IN*/ pNotification
			     	    );
  
   /************************************************************************
    *  Method:
    *      IHXAudioStream2::RemoveDryNotification
    *  Purpose:
    *	    Use this to remove itself from the notification response object
    *	    during the stream switching.
    */
    STDMETHOD(RemoveDryNotification)	(THIS_
					IHXDryNotification* /*IN*/ pNotification
			     		);

   /************************************************************************
    *  Method:
    *      IHXAudioStream2::GetAudioFormat
    *  Purpose:
    *	    Returns the input audio format of the data written by the 
    *	    renderer. This function will fill in the pre-allocated 
    *	    HXAudioFormat structure passed in.
    */
    STDMETHOD(GetAudioFormat)   (THIS_
			        HXAudioFormat*	/*IN/OUT*/pAudioFormat);

    STDMETHOD_(IHXValues*,GetStreamInfo) (THIS)
	    { 
		if (m_pValues)
		{
		    m_pValues->AddRef();
		}

		return m_pValues ; 
	    };

    /* Return the this stream's volume interface. */
    STDMETHOD_(IHXVolume*, GetAudioVolume)         (THIS);

    /*
     *  IHXRealAudioSync methods
     */

    /************************************************************************
     *  Method:
     *      IHXRealAudioSync::Register
     *  Purpose:
     */
    STDMETHOD(Register)	(THIS);

    /************************************************************************
     *  Method:
     *      IHXRealAudioSync::UnRegister
     *  Purpose:
     */
    STDMETHOD(UnRegister)	(THIS);

    /************************************************************************
     *  Method:
     *      IHXRealAudioSync::FudgeTimestamp
     *  Purpose:
     *	    Tell the audio stream about the relationship between the number 
     *	    of bytes written to the actual timestamp.
     *	    
     */
    STDMETHOD(FudgeTimestamp)	(THIS_
				UINT32 /*IN*/ ulNumberofBytes,
				UINT32 /*IN*/ ulTimestamp
	    			);

    /*
     *	IHXCommonClassFactory methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCommonClassFactory::CreateInstance
     *	Purpose:
     *	    Creates instances of common objects supported by the system,
     *	    like IHXBuffer, IHXPacket, IHXValues, etc.
     *
     *	    This method is similar to Window's CoCreateInstance() in its 
     *	    purpose, except that it only creates objects of a well known
     *	    types.
     *
     *	    NOTE: Aggregation is never used. Therefore and outer unknown is
     *	    not passed to this function, and you do not need to code for this
     *	    situation.
     */
    STDMETHOD(CreateInstance)	(THIS_
				REFCLSID    /*IN*/  rclsid,
				void**	    /*OUT*/ ppUnkown);

    /************************************************************************
     *  Method:
     *	    IHXController::CreateInstanceAggregatable
     *  Purpose:
     *	    Creates instances of common objects that can be aggregated
     *	    supported by the system, like IHXSiteWindowed
     *
     *	    This method is similar to Window's CoCreateInstance() in its 
     *	    purpose, except that it only creates objects of a well known
     *	    types.
     *
     *	    NOTE 1: Unlike CreateInstance, this method will create internal
     *		    objects that support Aggregation.
     *
     *	    NOTE 2: The output interface is always the non-delegating 
     *		    IUnknown.
     */
    STDMETHOD(CreateInstanceAggregatable)
				    (THIS_
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter);

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePacketTimeOffset
     *	Purpose:
     *	    Call this method to update the timestamp offset of cached packets
     */
    STDMETHOD(UpdatePacketTimeOffset) (THIS_
				       INT32 lTimeOffset);


    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePlayTimes
     *	Purpose:
     *	    Call this method to update the playtime attributes
     */
    STDMETHOD(UpdatePlayTimes)	      (THIS_
				       IHXValues* pProps);

    /*
     *      IHXAudioStream3 methods
     */
    STDMETHOD(Stop)		    (THIS);
    STDMETHOD(Pause)		    (THIS);
    STDMETHOD(Resume)		    (THIS);
    STDMETHOD(Flush)		    (THIS);
    STDMETHOD(Rewind)		    (THIS);
    STDMETHOD(Reconfig)		    (THIS_ IHXValues* pParams);
    STDMETHOD_(HXBOOL, IsResumed)   (THIS);
    STDMETHOD_(HXBOOL, IsRewound)   (THIS);
    

    /*
     *      CHXAudioStream methods
     */
    HX_RESULT 	Setup   (
				    HXAudioFormat*	  pFormat
			    ,	ULONG32		  ulGranularity
			    );
    HX_RESULT	GetFormat( HXAudioFormat* pAudioFormat );

    HX_RESULT	MixIntoBuffer( 
			    UCHAR*	pBuf,
			    ULONG32	ulBufSize,
			    ULONG32&    ulBufTime,
			    HXBOOL&	bIsMixBufferDirty,
			    HXBOOL	bGetCrossFadeData = FALSE
			    );
    ULONG32		MixData(
				UCHAR*  pDestBuf
			    ,   ULONG32 ulBufLen 
			    ,	HXBOOL	bMonoToStereoMayBeConverted
			    ,	HXBOOL&	bIsMixBufferDirty);

	HXBOOL    IsInitialized(void)	     {return m_bInited;};
	HXBOOL    IsAudioFormatKnown(void) {return m_bAudioFormatKnown;};
	inline void SetLive(HXBOOL bIsLive)
        {   
            if (m_bIsFirstPacket)
            {
                m_bIsLive = bIsLive;
            }
        }
	void    SetupToBeDone()	     {m_bSetupToBeDone = TRUE;};
	HX_RESULT StartCrossFade(CHXAudioStream*    pFromStream, 
				UINT32		    ulCrossFadeStartTime,
				UINT32		    ulCrossFadeDuration, 
				HXBOOL		    bToStream);

	HX_RESULT   ConvertCurrentTime(double dBytesPlayed, 
				       UINT32 ulCurrentTime,
				       UINT32& ulAdjustedTime);

	HX_RESULT Seek(UINT32 ulSeekTime);

	void	SyncStream(INT64 llSyncTime);
	void	FreeInfo(HXAudioInfo* pInfo, HXBOOL bInstantaneous = FALSE);
	void	FreeBuffer(IHXBuffer* pBuffer);
#ifdef _MACINTOSH
	INT64	GetLastAudioWriteTime() {return m_llLastWriteTime - (INT64)(UINT64)m_ulBaseTime;};
#else
	INT64	GetLastAudioWriteTime() {return m_llLastWriteTime - (INT64)m_ulBaseTime;};
#endif
	void	UpdateStreamLastWriteTime(HXBOOL bForceUpdate = FALSE, HXBOOL bOnResume = FALSE);
	void	SaveLastNMilliSeconds(HXBOOL bSave, UINT32 ulNMilliSeconds);
	void	RewindStream(INT32 lTimeToRewind);

	void	PauseByPlayer(void) { _Pause(TRUE); }
	void	ResumeByPlayer(void) { _Resume(TRUE); }

	EPlayerState	GetState(void) { return m_eState; };

	void	SetSoundLevel(float fSoundLevel);
	void	SetAudioDeviceReflushHint(HXBOOL bSupported);
	HXBOOL	IsAudioDeviceReflushHint(void) { return m_bAudioDeviceReflushHint; };
	void	RollBackTimestamp();
	void	SetSoundLevelOffset(INT16 nOffset);

	CHXAudioPlayer*	m_Owner;
	HXBOOL		m_bMayNeedToRollbackTimestamp;

	virtual void	    ResetStream(void);
	virtual HX_RESULT   ProcessAudioHook(PROCESS_ACTION action, 
					     IHXAudioHook* pAudioHook);

	// CAudioSvcSampleConverter method
	virtual HXBOOL ConvertIntoBuffer(tAudioSample* buffer, UINT32 nSamples, INT64 llStartTimeInSamples) ;

        
        HXBOOL IsStopped()
        {
            return (m_eState == E_STOPPED);
        }
        HXBOOL IsPaused()
        {
            return (m_eState == E_PAUSED);
        }
	

protected:

    LONG32		m_lRefCount;
    HX_RESULT		m_wLastError;
    HXBOOL		m_bInited;		// Set to TRUE after buffer is created. 
    HXBOOL		m_bSetupDone;
    HXBOOL		m_bAudioFormatKnown;
    HXBOOL		m_bIsResumed;
    HXBOOL		m_bPlayerPause;
    HXBOOL		m_bStreamPause;
    HXAudioFormat      m_AudioFmt;		// This streams format
    HXAudioFormat      m_DeviceFmt;		// The audio device format
    IHXAudioResampler* m_pResampler;		// Id used by resampling code
    void*		m_pInterpId;		// Id used by linear interpolator code
    IHXValues*		m_pValues;
    HXBOOL		m_bDisableWrite;
#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    CHXMapPtrToPtr	m_PreMixHookMap;
#endif
    
    CHXMapPtrToPtr*	m_DryNotificationMap;
    ULONG32		m_ulGranularity;	// Play at least this amt of audio
    ULONG32		m_ulInputBytesPerGran;	// Number of bytes for this amt of audio
    ULONG32		m_ulOutputBytesPerGran;	// Number of bytes for this amt of audio
    ULONG32		m_ulExcessInterpBufferSize;	// Number of bytes for this amt of audio
    ULONG32		m_ulPreviousExcessInterpBufferSize;
    HXBOOL		m_bFirstWrite;
    ULONG32		m_ulMaxBlockSize;	// Size of resampled data buffer
    UCHAR*		m_pResampleBuf; 	// Resampler output buffer
    UCHAR*		m_pTmpResBuf; 		// Tmp resample  buffer
    UCHAR*		m_pCrossFadeBuffer; 		// Tmp resample  buffer
    UCHAR*		m_pExcessInterpBuffer;
    UCHAR*		m_pTempInterpBuffer;
    UINT32		m_ulCrossFadeBufferSize;
    float		m_fVolume;		// Stream volume level
    HXBOOL		m_bChannelConvert;	// if TRUE convert mono->stereo
    UINT16		m_uVolume;		// Stream's current volume.
    INT16		m_nSoundLevelOffset;	// For audio level normalization
    HXBOOL		m_bMute;
    HXBOOL		m_bGotHooks;		// if TRUE if this stream has hooks
    HXBOOL		m_bIsOpaqueStream;	// TRUE if this is an opaque audio stream

    INT64		m_llLastWriteTime;	// Time of last buffer written

    //This keeps track of the difference between the players last current time
    //and the sessions last current time. Basically, the last amount of
    //adjustment we did for RA Fudged streams.
    UINT32              m_ulLastAdjustedTimeDiff;

    ULONG32		m_ulFudge;		// The fudge factor for detecting gaps

    HXBOOL		m_bHooksInitialized;
    HXAudioData*   m_pInDataPtr;
    HXAudioData*   m_pOutDataPtr;
    CHXSimpleList*  m_pDataList;		// Audio data list
    CHXSimpleList*  m_pInstantaneousList;	// Instantaneous data list
    CHXSimpleList*  m_pRAByToTsInList;
    CHXSimpleList*  m_pRAByToTsAdjustedList;
#if defined(HELIX_FEATURE_CROSSFADE)
    CrossFader*	    m_pCrossFader;
#else
    void*           m_pCrossFader;
#endif /* HELIX_FEATURE_CROSSFADE */
    IHXCommonClassFactory* m_pCommonClassFactory;
    IHXPreferences* m_pPreferences;
    
#ifdef _MACINTOSH
    HXAudioMacQueue* m_pAvailableBuffers;
#else
    CHXSimpleList*  m_pAvailableBuffers;
#endif
    
    UINT16	    m_uCacheSize;
    HXBOOL	    m_bDeterminedInitialCacheSize;
    HXBOOL	    m_bCacheMayBeGrown;

    HXBOOL		m_bTobeTimed;

    HXBOOL		m_bIsFirstPacket;
    HXBOOL		m_bIsLive;
    HXBOOL		m_bSetupToBeDone;
    UINT32		m_ulBaseTime;

    HXBOOL		m_bCrossFadingToBeDone;
    CHXAudioStream*	m_pCrossFadeStream;
    INT64		m_llCrossFadeStartTime;
    UINT32		m_ulCrossFadeDuration;
    HXBOOL		m_bFadeToThisStream;
    HXBOOL		m_bFadeAlreadyDone;

    HXBOOL		m_bRealAudioStream;

    UINT32		m_ulLastInputStartTime;
    UINT32		m_ulLastInputEndTime;
    INT64		m_llLastStartTimePlayed;

    UINT32		m_ulTSRollOver;
    UINT32		m_ulLiveDelay;
    HX_BITFIELD		m_bLastWriteTimeUpdated  : 1;

    HX_BITFIELD		m_bAudioDeviceReflushHint  : 1;

    HX_BITFIELD		m_bLastNMilliSecsToBeSaved : 1;
    UINT32		m_ulLastNMilliSeconds;
    CHXSimpleList*	m_pLastNMilliSecsList;
    UINT32		m_ulLastNHeadTime;
    UINT32		m_ulLastNTailTime;

    EPlayerState	m_eState;
    HX_BITFIELD		m_bIsRewound : 1;

    HXBOOL		m_bHasStartTime;
    HXBOOL		m_bBeyondStartTime;
    UINT32		m_ulStartTime;

/*
 *  Left around for future debugging
    FILE*		fdbefore;
    FILE*		fdbeforetxt;
    FILE*		fdafter;
    FILE*		fdaftertxt;
    FILE*		fdin;
*/
    virtual		~CHXAudioStream();

    void	InitHooks();
    HX_RESULT 	ProcessHooks(HXAudioData*  pInData
			     ,HXAudioData* pOutData);
    HX_RESULT	AddData (HXAudioData* pData);
    HX_RESULT	SetupResampler(void);

    ULONG32	CalcMs( ULONG32 ulNumBytes );
    ULONG32	CalcDeviceMs( ULONG32 ulNumBytes );
    
    UINT32	CalcOffset(INT64 llStartTime, INT64 llEndTime);
    void	FlushBuffers(HXBOOL bInstantaneousAlso = TRUE);
    HX_RESULT	ProcessInfo(void);
    HXBOOL	EnoughDataAvailable(INT64& llStartTimeInSamples, UINT32& nSamplesRequired);
    void	CleanupRAByToTs(void);
    void	MapFudgedTimestamps(void);
    void	InitializeCrossFader(void);
    void	RemoveExcessCrossFadeData();

private:
    HXAudioSvcMixEngine *m_pMixEngine ;

    static UINT32 Bytes2Samples(UINT64 ulNumBytes, const HXAudioFormat *fmt) ;
    static UINT64 Samples2Ms(INT64 nSamples, const HXAudioFormat *fmt, HXBOOL bRoundUp = FALSE) ;

    // variables to estimate the true sample rate
    INT64	m_startMeasureTime ;
    INT32	m_totSamples ;

    // how many samples apart may the mixes lie until we don't recognize them anymore
    // as contiguous? Essentially, this should be sample rate * [error in time stamps]
    enum { MIXTIMEFUDGEINSAMPLES = 400 } ;

    // we need these to correct for incoming data containing incomplete
    // sample frames.
    IHXBuffer* m_piPendingAudioData ;
    UINT32 m_ulPendingAudioBytes ; // size in bytes of the slush buffer content
    UINT32 m_ulSampleFrameSize ; // size in bytes of a sample frame, i.e. 2*nChannels
    // Write() backend function
    HX_RESULT Write2(HXAudioData* pAudioData) ;

#ifdef HELIX_FEATURE_VOLUME
    IHXVolume* m_pStreamVolume;
#endif

};

#ifdef _MACINTOSH

struct HXAudioMacQueueElement 
{
	HXAudioMacQueueElement 	*mNextElementInQueue; // link must be first member
	void			*mObject;
};

class HXAudioMacQueue 
{
	protected:
	
		QHdr 		mQueueHeader;
		Boolean		mDestructing;
		unsigned long	m_nCount;
		
	public:
	
		HXAudioMacQueue(void);
		~HXAudioMacQueue(void);	// decrements the ref on the irma nodes (via release)
		
		void*	 	RemoveHead(void);
		HX_RESULT 	AddTail(void* pObject);	// increments the ref
		UINT32		GetCount();
};
#endif // _MACINTOSH

#endif /* HXAUSTR_H_ */
