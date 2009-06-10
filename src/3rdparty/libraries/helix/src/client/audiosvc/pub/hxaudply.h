/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudply.h,v 1.21 2007/07/06 21:57:40 jfinnecy Exp $
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

#ifndef _HXAUPLY_H_
#define _HXAUPLY_H_

#include "hxaudtyp.h" 
#include "hxaudses.h"
#include "hxplayvelocity.h"
#include "hxslist.h"

// forward decls
struct IHXAudioPlayerResponse;
struct IHXAudioStream;
struct IHXAudioHook;
struct IHXAudioStreamInfoResponse;
struct IHXScheduler;
struct IHXVolume;
struct IHXPreferences;
struct IHXAudioCrossFade;

typedef struct _HXAudioFormat HXAudioFormat;

class CHXAudioSession;
class CHXSimpleList;
class CHXAudioStream;

class Timeval;

#ifdef _MACINTOSH
extern HXBOOL gSoundCallbackTime;
#endif

/****************************************************************************
 *
 *  Class:
 *
 *      CHXAudioPlayer
 *
 *  Purpose:
 *
 *      PN implementation of audio player.
 *
 */
class CHXAudioPlayer: public IHXAudioPlayer,
		      public IHXAudioPlayer2,
#ifdef HELIX_FEATURE_VOLUME 
		      public IHXVolumeAdviseSink,
#endif 
		      public IHXAudioCrossFade,
		      public IHXCallback,
		      public IHXPlaybackVelocity,
		      public IHXClockSourceManager,
		      public IHXInterruptSafe
{
  private:
    LONG32                   m_lRefCount;
    HXAudioFormat            m_PlayerFmt;            // This players audio format
    IHXAudioPlayerResponse*  m_pPlayerResponse;      // Notification interface
    IHXScheduler*            m_pScheduler;
    IHXPreferences*          m_pPreferences;
    UINT32                   m_ulCurrentTime;        // Current playback time 
    UINT32                   m_ulLastCurrentTimeReturned;    
    UINT32                   m_ulLastDeviceTimeAdjusted;
    HXBOOL                   m_bTimeReturned;
    HXBOOL                   m_bDisableDeviceWrite;  // Don't write to audio device
    UINT32                   m_ulBytesPerGran;       // Num bytesper granularity
    CHXSimpleList*           m_pStreamRespList;      // Stream response list
    UINT32                   m_ulASstartTime;        // This player's start time.
    UINT32                   m_ulAPplaybackTime;     // This player's playback
                                                     // time within the timeline

    //Difference between the last current time and the last adjusted
    //time. This is used by non-RA streams to help adjust the time in
    //a multiple stream/multiple player scenario.
    UINT32                   m_ulLastAdjustedTimeDiff;

    UINT32                   m_ulAPstartTime;        // This player's playback
                                                     // start time within the
                                                     // timeline.
    UINT32                   m_ulADresumeTime;       // This is the audio device
                                                     // time when this player
                                                     // resumed.

    EPlayerState             m_eState;               // Status

    HXBOOL                   m_bPrefUse11khz;        // Sampling rate from prefs
    UINT16                   m_uPrefBitsPerSample;   // Bits per sample from prefs
    UINT16                   m_uPrefAudioQuality;    // Audio quality from prefs
    HXBOOL                   m_bDisableWrite;
    HXBOOL                   m_bIsStarted;
    UINT16                   m_uVolume;
    HXBOOL                   m_bMute;
    INT32                    m_lPlaybackVelocity;
    HXBOOL                   m_bKeyFrameMode;
    UINT32                   m_ulScaledTimeAnchor;
    UINT32                   m_ulUnscaledTimeAnchor;
    UINT32                   m_ulCurrentScaledTime;
    CHXSimpleList*           m_pInactiveClockSourceQueue;
    IHXClockSource*          m_pActiveClockSource;

#ifdef HELIX_FEATURE_VOLUME        
    IHXVolume* m_pPlayerVolume;
#endif        


    CHXAudioStream* GetProperFudgeMap();
    CHXAudioStream* m_pLastMappingStream;
    HXBOOL          m_bNewMapStarting;
    
  protected:
    ~CHXAudioPlayer();

  public:

    UINT32 GetLastAdjustedTimeDiff();
    
    CHXAudioPlayer(CHXAudioSession* owner);
    void Close(void);

    //IHXCallback methods
    STDMETHOD(Func)(THIS);
        
    /*
     *      IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                 void** ppvObj);
    STDMETHOD_(UINT32,AddRef)  (THIS);
    STDMETHOD_(UINT32,Release) (THIS);

    /*
     *      IHXAudioPlayer methods
     */
    STDMETHOD(CreateAudioStream)    (THIS_
                                     IHXAudioStream**                pAudioStream
                                     );

    /* AddPostMixHook Interface is called by renderers to set a 
     * post mix hook of the audio data.
     */
    STDMETHOD(AddPostMixHook)(THIS_
                              IHXAudioHook*           pHook,
                              const HXBOOL            bDisableWrite,
                              const HXBOOL            bFinal
                              );
    STDMETHOD(RemovePostMixHook)(THIS_
                                 IHXAudioHook*           pHook
                                 );

#ifdef HELIX_FEATURE_VOLUME        
    // IHXVolumeAdviseSink methods
    inline STDMETHOD(OnVolumeChange)(THIS_ const UINT16 uVol )
    {
        m_uVolume = uVol;
        return HXR_OK;
    }
        
    inline STDMETHOD(OnMuteChange)(THIS_ const HXBOOL bMute )
    {
        m_bMute = bMute;
        return HXR_OK;
    }
    inline UINT16 GetVolume()
    {
        return m_bMute? 0 : m_uVolume;
    }
        
#endif        
        
    /************************************************************************
     *  Method:
     *      IHXAudioPlayer::GetAudioStreamCount
     *  Purpose:
     *               Get the number of audio streams currently active in the 
     *               audio player. Since streams can be added mid-presentation
     *               this function may return different values on different calls.
     *               If the user needs to know about all the streams as they get
     *               get added to the player, IHXAudioStreamInfoResponse should
     *               be implemented and passed in SetStreamInfoResponse.
     */
    STDMETHOD_(UINT16,GetAudioStreamCount) (THIS);

    /* Get an audio stream from the stream list at position given.
     */
    STDMETHOD_(IHXAudioStream*,GetAudioStream)(THIS_
                                               UINT16                  uIndex
                                               );

    /* Set a stream info response. This is used to get the number
     * of streams associated with this player.
     */
    STDMETHOD(SetStreamInfoResponse)(THIS_
                                     IHXAudioStreamInfoResponse*     pResponse
                                     );

    /************************************************************************
     *  Method:
     *      IHXAudioPlayer::RemoveStreamInfoResponse
     *  Purpose:
     *       Remove stream info response that was added earlier
     */
    STDMETHOD(RemoveStreamInfoResponse) (THIS_
                                         IHXAudioStreamInfoResponse* /*IN*/ pResponse);

    /* Return the this player's volume interface. */
    STDMETHOD_(IHXVolume*, GetAudioVolume)  (THIS);

    /* Return the session device volume interface. */
    STDMETHOD_(IHXVolume*, GetDeviceVolume) (THIS);

    /*
     *      IHXAudioPlayer2 methods
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
				     );

    /*
     *  IHXAudioCrossFade methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioCrossFade::CrossFade
     *  Purpose:
     *      Cross-fade two audio streams.
     *      pStreamFrom             - Stream to be cross faded from
     *      pStreamTo               - Stream to be cross faded to
     *      ulFromCrossFadeStartTime- "From" Stream time when cross fade is 
     *                                to be started
     *      ulToCrossFadeStartTime  - "To" Stream time when cross fade is to 
     *                                be started
     *      ulCrossFadeDuration     - Duration over which cross-fade needs
     *                                    to be done
     *          
     */
    STDMETHOD(CrossFade) (THIS_
                          IHXAudioStream* pStreamFrom,
                          IHXAudioStream* pStreamTo,
                          UINT32           ulFromCrossFadeStartTime,
                          UINT32           ulToCrossFadeStartTime,
                          UINT32           ulCrossFadeDuration);
    
    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse);
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps);
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    STDMETHOD_(INT32,GetVelocity)          (THIS);
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode);
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS);
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS) { return 0; }
    STDMETHOD(CloseVelocityControl)        (THIS);

    // IHXClockSourceManager methods
    STDMETHOD(RegisterClockSource)   (THIS_ IHXClockSource* pSource);
    STDMETHOD(UnregisterClockSource) (THIS_ IHXClockSource* pSource);
    STDMETHOD(SetError) (THIS_ HX_RESULT theErr );

    // IHXInterruptSafe methods
    STDMETHOD_(HXBOOL,IsInterruptSafe) (THIS) { return TRUE; }
 
    /*
     *   Other public methods.
     */
    HX_RESULT InitializeStructures(void);
    HX_RESULT Init(IUnknown* pContext);
    HX_RESULT Setup(UINT32 ulGranuarity);
    void      SetGranularity(const UINT32 ulGranularity);
    HX_RESULT Resume(void);
    HX_RESULT Pause(void);
    HX_RESULT Stop(const HXBOOL bFlush);
    void      DonePlayback(void)   {m_bIsDonePlayback = TRUE;};
    HXBOOL    IsDonePlayback(void) {return m_bIsDonePlayback;};
    HX_RESULT Seek(const UINT32 ulSeekTime);
    void      GetFormat(HXAudioFormat* pAudioFormat);

    CHXAudioSession* GetOwner(void) 
    {
        return m_Owner;
    };
    CHXSimpleList*  GetStreamList(void) 
    { 
        return m_pStreamList; 
    };

    HX_RESULT OnTimeSync(UINT32 ulCurrentTime);
    UINT32    GetCurrentPlayBackTime(void);
    UINT16    GetStreamCount(void);

    /* Return the post process hook list. */
    CHXSimpleList*  GetPostMixHookList(void) 
    { 
        return m_pPMixHookList; 
    };

    EPlayerState    GetState(void)
    {
        return m_eState;
    }

    void            StreamInitialized(CHXAudioStream* pAudioStream);
    HXBOOL          IsDisableWrite(void) {return m_bDisableWrite;};
    UINT32          GetInitialPushdown(HXBOOL bAtStart = FALSE);
    void            SetLive(HXBOOL bIsLive);
    void            AudioFormatNowKnown(void);

    HXBOOL          IsThisAudioStream(IHXValues* pHeader);
    HXBOOL          IsAudioOnlyTrue(void);

    double          NumberOfBytesWritten();
    double          ConvertMsToBytes(UINT32 ulTime);
    void            UpdateStreamLastWriteTime();

    void            SaveLastNMilliSeconds(HXBOOL bSave, UINT32 ulNMilliSeconds);
    void            RewindPlayer(INT32 lTimeToRewind);
    void            DataInAudioDevice(HXBOOL bHasDataInAudioDevice)
    {
        m_bHasDataInAudioDevice = bHasDataInAudioDevice;
    };
    HXBOOL          HasDataInAudioDevice()
    {
        return m_bHasDataInAudioDevice;
    };

    HX_RESULT       SetSoundLevelOffset(CHXSimpleList* pAudioStreamList, INT16 nSoundLevelOffset);
    HX_RESULT       SetSoundLevel(CHXSimpleList* pAudioStreamList, UINT16 uSoundLevel, HXBOOL bReflushAudioDevice);
        
    HX_RESULT ManageAudioStreams(CHXSimpleList* pStreamLst,
                                 UINT32 what,
                                 UINT32 ulTime = 0
                                 );
        
    CHXAudioStream* GetCHXAudioStream(UINT16 uIndex);

    HXBOOL          IsLastNMilliSecsToBeStored();

    HX_RESULT       ActualAddPostMixHook(IHXAudioHook* pHook,
                                         const HXBOOL           bDisableWrite,
                                         const HXBOOL           bFinal);
    HX_RESULT       ActualRemovePostMixHook(IHXAudioHook* pHook);

    UINT32          GetGranularity(void) { return m_ulGranularity; };
    HXBOOL          IsResumed(void) { return (m_eState == E_PLAYING); };
    inline void     UseCoreThread()
    {
        m_Owner->UseCoreThread();
    }
        

    virtual HX_RESULT   _CreateAudioStream(IHXAudioStream** pAudioStream);
    virtual HX_RESULT   ProcessAudioHook(PROCESS_ACTION action, IHXAudioHook* pAudioHook);
    virtual void        ResetPlayer(void);

    friend class CHXAudioStream;

  protected:
    UINT32          m_ulCallbackID;
    IUnknown*        m_pContext;
    HXBOOL           m_bInited;              // Initialized?
    HXBOOL           m_bHasStreams;          // Player has streams..
    HXBOOL           m_bIsLive;
    UINT32           m_ulGranularity;        // Playback granularity
    CHXAudioSession* m_Owner;

    // We use the m_pStreamListIters list to maintain a list of valid iterators
    // over the m_pStreamList container.  If you are iterating over
    // m_pStreamList your iterator may become invalid as functions deeper in the
    // call chain may add or remove items from that list. In order to detect
    // this you must add the address of your iterator into the
    // m_pStreamListIters lists. As long as you can find your iterator there, it
    // is valid. If any method adds or removes items from the stream list, it
    // will remove all iterators from the iterator list, hence invalidating your
    // iterator.  At that point you must reset you iterator and then add it back
    // into the iterator list.
    CHXSimpleList* m_pStreamList;
    CHXSimpleList  m_StreamListIters;
    inline HXBOOL  IsValidIter(void* pPtr );
    inline void    InvalidateIters();
    inline void    AddIter(void* pPtr);
    inline void    RemoveIter(void* pPtr);
    inline void    CheckIter(void* pPtr);

    //This one is just for convenience until we generalize these routines to all
    //containers.
    inline void    IncrementOrReset( CHXSimpleList::Iterator* plIter,
                                     CHXSimpleList* pList);

    CHXSimpleList*   m_pPMixHookList;        // Post mix hooks
    HXAudioFormat    m_DeviceFmt;            // Audio device format
    Timeval*         m_pFakeAudioCBTime;
    UINT32           m_ulLastFakeCallbackTime;
    UINT32           m_ulIncreasingTimer;
    HXBOOL           m_bIsDonePlayback;
    HXBOOL           m_bIsFirstResume;
    HXBOOL           m_bHasDataInAudioDevice;

    void             OnTimerCallback();

    void            SetupStreams(void);
    void            AddStreams(void);
    void            RemoveStreams(void);
    UINT16          NumberOfResumedStreams(void);

    /* Get audio prefs.
     */
    HX_RESULT       GetAudioPrefs(void);
    void            AdjustTimelineForMappedStreams(void);

    HX_RESULT       ResumeFakeTimeline(void);
    HX_RESULT       StopFakeTimeline(void);

    UINT32          ScaleCurrentTime(UINT32 ulTime);
    UINT32          GetFakeTimelineTime();
    void            UpdateFakeTimelineTime();
    HX_RESULT       ClockSourceSwitch(IHXClockSource* pSource, HXBOOL bAdd);
    void            ClearClockSourceQueue();
    HX_RESULT       AddToClockSourceQueue(IHXClockSource* pSource, HXBOOL bHead);
    void            RemoveFromClockSourceQueue(IHXClockSource* pSource);
    HX_RESULT       InstallNewClockSource(IHXClockSource* pSource);
    HX_RESULT       InstallAnyInactiveClockSource();
    void            DeactivateClockSource();

    HX_RESULT       AudioStreamStateChanged(EPlayerState eState, HXBOOL bRewindNeeded = TRUE);
};

HXBOOL CHXAudioPlayer::IsValidIter(void* pPtr )
{
    return (m_StreamListIters.Find(pPtr)!=0);
}

void CHXAudioPlayer::InvalidateIters()
{
    m_StreamListIters.RemoveAll();
}


void CHXAudioPlayer::AddIter(void* pPtr)
{
    m_StreamListIters.AddTail(pPtr);
}

void CHXAudioPlayer::RemoveIter(void* pPtr)
{
    LISTPOSITION lp = m_StreamListIters.Find(pPtr);
    if( lp )
    {
        m_StreamListIters.RemoveAt(lp);
    }
}

void CHXAudioPlayer::CheckIter(void* pPtr)
{
    if( !IsValidIter(pPtr))
    {
        HX_ASSERT("bad iterator"==NULL);
    }
}

void CHXAudioPlayer::IncrementOrReset( CHXSimpleList::Iterator* plIter, CHXSimpleList* pList)
{
    if( !IsValidIter(plIter) )
    {
        *plIter = pList->Begin();
        AddIter(plIter);
    }
    else
    {
        ++(*plIter);
    }
}

#endif /* HXAUPLY_H_ */
