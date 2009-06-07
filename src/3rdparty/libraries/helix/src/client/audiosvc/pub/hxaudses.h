/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudses.h,v 1.29 2007/02/23 20:31:27 milko Exp $
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

#ifndef _HXAUDSES_H
#define _HXAUDSES_H

#include "hxaudtyp.h" 

#ifdef _UNIX
#include <fcntl.h>
#include <sys/stat.h>
#endif

//  IHXTimelineWatcher defines.
#  define TLW_PAUSE    1
#  define TLW_RESUME   2
#  define TLW_CLOSE    3
#  define TLW_TIMESYNC 4


// These defines control how much audio PCM data we push down into the
// audio device. MINIMUM_AUDIO_STARTUP_PUSHDOWN determines how much we
// push before calling Resume() on the device (and starting playback)
// and MINIMUM_AUDIO_PUSHDOWN determines how much PCM data we push
// down during steady state playback. It is also important that the
// granularity be less then or equal to MINIMUM_AUDIO_PUSHDOWN/2. If
// not, underflows could result because only 1 block will be in the
// audio device at any given time. Currently MINIMUM_AUDIO_GRANULARITY
// is not used. In CHXAudioPlayer::Setup() hardcodes this:
//
//     m_ulGranularity = MAXIMUM_AUDIO_GRANULARITY; 
//
// The CHECK_AUDIO_INTERVAL define tells the audio session how often
// to check how much data we have pushed down in the audio device.
// for min heap builds it is more important to check often because we
// have very few blocks pushed down at any one time. The value for
// this define is a percentage (0-100) of the granularity in
// milliseconds. IE, 40 means 40% of whatever granulatiry is in use.
//
#ifdef HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES
#  define MINIMUM_AUDIO_GRANULARITY       50   
#  define MAXIMUM_AUDIO_GRANULARITY       MINIMUM_AUDIO_GRANULARITY
#  define TARGET_AUDIO_PUSHDOWN           200 
#  define MINIMUM_AUDIO_STARTUP_PUSHDOWN  150
#  define MINIMUM_AUDIO_PUSHDOWN          100
#  define CHECK_AUDIO_INTERVAL            40	// percentage of granularity  
#else
#  define MINIMUM_AUDIO_GRANULARITY       50 
#  define MAXIMUM_AUDIO_GRANULARITY       50
#  define TARGET_AUDIO_PUSHDOWN           1000   
#  define MINIMUM_AUDIO_STARTUP_PUSHDOWN  150
#  define MINIMUM_AUDIO_PUSHDOWN          100
#  define CHECK_AUDIO_INTERVAL            60	// percentage of granularity    
#endif


// foward decls.
struct IHXCallback;
struct IHXScheduler;
struct IHXPreferences;
struct IHXAudioDeviceResponse;
struct IHXAudioPlayerResponse;
struct IHXAudioAdviseSink;
struct IHXAudioDeviceManager;
struct IHXAudioHook;
struct IHXInterruptState;

typedef struct _HXAudioFormat HXAudioFormat;
class CHXAudioPlayer;

class CHXAudioDevice;
class CHXAudioStream;
class CHXSimpleList;
class HXVolumeAdviseSink;

struct IHXBuffer;

class Timeval;

class HXThread;
class HXMutex;
class HXEvent;

typedef enum
{
    BUFFER_NONE     = 0,
    BUFFER_PLAYER   = 1,
    BUFFER_SESSION  = 2
} BufType;

class   CHXAudioSession : public IHXAudioDeviceResponse, 
    public IHXAudioDeviceManager,
    public IHXAudioHookManager,
    public IHXAudioDeviceManager2,
#ifdef HELIX_FEATURE_VOLUME                          
    public IHXVolumeAdviseSink,
#endif                          
    public IHXCallback,
    public IHXAudioResamplerManager,
    public IHXAudioPushdown2,
#if defined(HELIX_FEATURE_TIMELINE_WATCHER)
    public IHXTimelineManager,
#endif    
    public IHXAudioDeviceHookManager,
    public IHXInterruptSafe
{
  public:
    /*
     *      IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                 void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    CHXAudioSession(void);

#if defined(HELIX_FEATURE_TIMELINE_WATCHER)    
    //IHXTimelineManager methods...
    STDMETHOD(AddTimelineWatcher)    (THIS_ IHXTimelineWatcher* );
    STDMETHOD(RemoveTimelineWatcher) (THIS_ IHXTimelineWatcher* );
#endif

#ifdef HELIX_FEATURE_VOLUME                          
    //IHXVolumeAdviseSink methods
    inline STDMETHOD(OnVolumeChange) (THIS_ const UINT16 uVolume )
    {
        SetVolume(uVolume);
        return HXR_OK;
    }

    inline STDMETHOD(OnMuteChange)(THIS_ const HXBOOL bMute )
    {
        SetMute(bMute);
        return HXR_OK;
    }
#endif
    
    /*
     *      IHXAudioDeviceManager methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioDeviceManager::Replace
     *  Purpose:
     *  This is used to replace the default implementation of the audio
     *  device by the given audio device interface. 
     */
    STDMETHOD(Replace) (THIS_
                        IHXAudioDevice* /*IN*/ pAudioDevice);

    /************************************************************************
     *  Method:
     *      IHXAudioDeviceManager::Remove
     *  Purpose:
     *  This is used to remove the audio device given to the manager in
     *  the earlier call to Replace.
     */
    STDMETHOD(Remove) (THIS_
                       IHXAudioDevice* /*IN*/ pAudioDevice);


    /************************************************************************
     *  Method:
     *   IHXAudioDeviceManager::AddFinalHook
     *  Purpose:
     *       One last chance to modify data being written to the audio device.
     *       This hook allows the user to change the audio format that
     *   is to be written to the audio device. This can be done in call
     *   to OnInit() in IHXAudioHook.
     */
    STDMETHOD(SetFinalHook) (THIS_
                             IHXAudioHook* /*IN*/ pHook
                             );

    /************************************************************************
     *  Method:
     *   IHXAudioDeviceManager::RemoveFinalHook
     *  Purpose:
     *       Remove final hook
     */
    STDMETHOD(RemoveFinalHook) (THIS_
                                IHXAudioHook* /*IN*/ pHook
                                );

    /************************************************************************
     *  Method:
     *      IHXAudioDeviceManager::GetAudioFormat
     *  Purpose:
     *           Returns the audio format in which the audio device is opened.
     *           This function will fill in the pre-allocated HXAudioFormat 
     *           structure passed in.
     */
    STDMETHOD(GetAudioFormat) (THIS_
                               HXAudioFormat* /*IN/OUT*/pAudioFormat);

    /*
     *      IHXAudioDeviceResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXAudioDeviceResponse::OnTimeSync
     *  Purpose:
     *      Notification interface provided by users of the IHXAudioDevice
     *      interface. This method is called by the IHXAudioDevice when
     *      audio playback occurs.
     */
    STDMETHOD(OnTimeSync) (THIS_
                           ULONG32 /*IN*/ ulTimeEnd);

    /*
     *  IHXAudioHookManager methods
     */
    /************************************************************************
     *  Method:
     *      IHXAudioHookManager::AddHook
     *  Purpose:
     *           Use this to add a hook 
     */
    STDMETHOD(AddHook)   (THIS_
                          IHXAudioHook* /*IN*/ pHook);

    /************************************************************************
     *  Method:
     *      IHXAudioHookManager::RemoveHook
     *  Purpose:
     *           Use this to remove a hook 
     */
    STDMETHOD(RemoveHook) (THIS_
                           IHXAudioHook* /*IN*/ pHook);

    /*
     *  IHXAudioDeviceHookManager methods
     */
    /************************************************************************
     *  Method:
     *      IHXAudioDeviceHookManager::AddAudioDeviceHook
     *  Purpose:
     *      Last chance to modify data being written to the audio device.
     */
    STDMETHOD(AddAudioDeviceHook)   (THIS_
                                     IHXAudioHook*           /*IN*/ pHook,
                                     AudioDeviceHookType     /*IN*/ type
                                     );

    /************************************************************************
     *  Method:
     *      IHXAudioDeviceHookManager::RemoveAudioDeviceHook
     *  Purpose:
     *      Removes the audio device hook that was set with AddAudioDeviceHook.
     */
    STDMETHOD(RemoveAudioDeviceHook)    (THIS_
                                         IHXAudioHook*    /*IN*/ pHook
                                         );

    /************************************************************************
     *  Method:
     *      IHXAudioDeviceHookManager::ProcessHooks
     *  Purpose:
     *      Called by audio device implementations to process the hooks on a
     *      given audio buffer
     */
    STDMETHOD(ProcessAudioDeviceHooks)  (THIS_
                                         IHXBuffer*& /*IN/OUT*/ pBuffer,
                                         HXBOOL&     /*OUT*/    bChanged
                                         );

    /*
     *  IHXAudioDeviceManager2 methods
     */

    /**********************************************************************
     *  Method:
     *      IHXAudioDeviceManager2::IsReplacedDevice
     *  Purpose:
     *  This is used to determine if the audio device has been replaced.
     */
    STDMETHOD_(HXBOOL, IsReplacedDevice) (THIS);

    /*
     *  IHXAudioResamplerManager methods
     *
     */
    STDMETHOD(CreateResampler) (THIS_
                                HXAudioFormat           inAudioFormat,
                                REF(HXAudioFormat)      outAudioFormat,
                                REF(IHXAudioResampler*) pResampler);

    /*
     *  IHXAudioPushdown methods
     */
    /************************************************************************
     *  Method:
     *      IHXAudioPushdown::SetAudioPushdown
     *  Purpose:
     *           Use this to set the minimum audio pushdown value in ms.
     *           This is the amount of audio data that is being written 
     *           to the audio device before starting playback.
     */
    STDMETHOD(SetAudioPushdown) (THIS_
                                 UINT32 /*IN*/ ulMinimumPushdown);

    /************************************************************************
     *  Method:
     *      IHXAudioPushdown2::GetAudioPushdown
     *  Purpose:
     *           Use this to get the minimum audio pushdown value in ms.
     *           This is the amount of audio data that is being written 
     *           to the audio device before starting playback.
     */
    STDMETHOD(GetAudioPushdown) (THIS_
                                 REF(UINT32) /*OUT*/ ulAudioPushdown);

    /************************************************************************
     *  Method:
     *      IHXAudioPushdown2::GetCurrentAudioDevicePushdown
     *  Purpose:
     *           Use this to get the audio pushed down to the audio device and haven't
     *           been played yet
     */
    STDMETHOD(GetCurrentAudioDevicePushdown) (THIS_
                                              REF(UINT32) /*OUT*/ ulAudioPusheddown);


    STDMETHOD_(HXBOOL,IsInterruptSafe) (THIS) { return TRUE; }

    /* It gets Scheduler interface from the context.
     * The session object keeps a list of audio player objects.
     * Create a session list; add this session to the list.
     */
    HX_RESULT Init(IUnknown* pContext);

    /* Setup is called by Audio Player object.
     */
    HX_RESULT Setup (HXBOOL bHasStreams);

    /* Create audio player. The RMA session object calls this
     * for each Player it knows about.
     */
    virtual HX_RESULT CreateAudioPlayer(CHXAudioPlayer**  ppAudioPlayer);
    HX_RESULT         _CreateAudioPlayer(CHXAudioPlayer** ppAudioPlayer);
    HX_RESULT         CloseAudioPlayer( CHXAudioPlayer*   pAudioPlayer);
    void              Close(void);

    /* The session object determines the audio device format based
     * on all of its players.
     */
    HX_RESULT GetDeviceFormat(void);

    /* Open is called to open the audio device.
     */
    HX_RESULT OpenAudio (void);
    HX_RESULT OpenDevice (void);
        
    /* PlayAudio is called by Audio Player object.
     */
    HX_RESULT PlayAudio (UINT16 uNumBlocks = 1);

    /* Pause is called by Audio Player object.
     */
    HX_RESULT Pause (CHXAudioPlayer* p);

    /* Resume is called by Audio Player object.
     */
    HX_RESULT Resume (CHXAudioPlayer* pPlayer = NULL, HXBOOL bRewindNeeded = FALSE);
 
    /* Stop is called by Audio Player object.
     */
    HX_RESULT Stop  (CHXAudioPlayer* p, HXBOOL bFlush);

    /* Seek is called by Audio Player object.
     */
    HX_RESULT Seek ( CHXAudioPlayer* pPlayerToExclude, 
                     const UINT32    ulSeekTime
                     );

    /* Return the device volume interface */
    inline IHXVolume* GetDeviceVolume()
    {
#if defined(HELIX_FEATURE_VOLUME)
        if( m_pDeviceVolume )
            m_pDeviceVolume->AddRef();
        return m_pDeviceVolume;
#else
        return NULL;
#endif /* HELIX_FEATURE_VOLUME */
    }

    /* The player object sets the granularity; I think the RMA session
     * object should do this after it resolves the granularity amongst
     * all of its players.
     */
    /* Granularity is always 100ms now...
     * The core keeps track of the granularity requirements for each 
     * renderer and queries audio sesssion. 
     */
    ULONG32         SetGranularity(ULONG32 ulGranularity);

    /* Reset audio session; flush buffers, etc.
     */
    virtual void        ResetSession(void);
    virtual HX_RESULT   ProcessAudioHook(PROCESS_ACTION action, 
                                         IHXAudioHook* pAudioHook);
    virtual HX_RESULT   ProcessAudioDevice(PROCESS_ACTION action, 
                                           IHXAudioDevice* pAudioDevice);

    virtual HX_RESULT   CreateAudioDevice();
    void                ReleaseAudioDevice();
    void                RestoreReplacedDevice();

    // Get audio pushdown taking into account pushdown becomming negative
    // in case of audio device starving.
    INT32 GetCurrentRawAudioDevicePushdown(void);

    /* Return the current time in timeline. */
    ULONG32 GetCurrentPlayBackTime(void);

    /* Get format that audio device was opened with. */
    void GetFormat( HXAudioFormat*       pAudioFmt );

    /* Return the core scheduler interface.
     */
    inline void GetScheduler(IHXScheduler** pScheduler)
    {
        *pScheduler = m_pScheduler;
    };

    void   SetVolume(const UINT16 uVolume );
    UINT16 GetVolume();
    void   SetMute(const HXBOOL bMute);
    inline HXBOOL GetMute()
    {
        return m_bMute;
    }
    
    /* Returns true if any audio player is playing... */
    HXBOOL IsPlaying(void);

    /* Returns number of active players (which are not is a STOP state).*/
    UINT16          NumberOfActivePlayers(void);
    UINT16          NumberOfResumedPlayers(void);
    inline void     ToBeRewound() {m_bToBeRewound = TRUE;};

    HXBOOL          CheckDisableWrite(void);
    ULONG32         GetInitialPushdown(HXBOOL bAtStart = FALSE);

    HX_RESULT       CheckToPlayMoreAudio();
    HX_RESULT       TryOpenAudio    (void); // attempt to open audio
        
    void            StopAllOtherPlayers();
    inline HX_RESULT CheckForBufferReuse();
    void            ConvertTo8BitAndOrMono(HXAudioData* pAudioData);
    
    double          ConvertMsToBytes(UINT32 ulCurrentTime);

    inline double   NumberOfBytesWritten() {return m_dNumBytesWritten;};
    inline double   GetNumBytesPlayed() {return m_dNumBytesPlayed;};

    void            SetCoreMutex(IHXMutex* pMutex) {m_pCoreMutex = pMutex;};
    void            CheckIfLastNMilliSecsToBeStored();
    void            RewindSession(CHXAudioPlayer* pPlayerToExclude = NULL);
    void            RewindAllPlayers(UINT32 ulCurTime,
                                     INT32 lTimeToRewind,
                                     CHXAudioPlayer* pPlayerToExclude);
    HX_RESULT       ActualResume();
    inline UINT32   GetLastRewindTimestamp(void) {return m_ulLastRewindTimestamp;};

    HXBOOL          ReallyNeedData();
    HXBOOL          GoingToUnderflow();
    inline void     UseCoreThread() {m_bShouldOpenOnCoreThread = TRUE;};
    inline void     PostMixHooksUpdated(void) { m_bPostMixHooksUpdated = TRUE; };

    HXBOOL GetDisableMultiPlayPauseSupport();

    HXBOOL SetOpaqueMode( const char * pszOpaqueType, IHXBuffer * pOpaqueStreamData );
    HXBOOL IsOpaqueMode() { return m_bOpaqueMode; }

    //IHXCallback stuff
    STDMETHOD(Func) (THIS);
    IUnknown* m_pContext;

    class HXDeviceSetupCallback : public IHXCallback,
				  public IHXInterruptOnly
        {
          private:
            ~HXDeviceSetupCallback();
        
            CHXAudioSession* m_pAudioSessionObject;
            ULONG32          m_ulCallbackID;
            LONG32           m_lRefCount;

          public:
            HXDeviceSetupCallback(CHXAudioSession* it);

            STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj);
            STDMETHOD_(ULONG32,AddRef) (THIS);
            STDMETHOD_(ULONG32,Release) (THIS);
            STDMETHOD(Func) (THIS);
            STDMETHOD_(HXBOOL,IsInterruptOnly) (THIS);

            void PendingID(ULONG32 it )
            { m_ulCallbackID = it;}
            ULONG32 PendingID()
            { return m_ulCallbackID; }
        
        };
    friend class CHXAudioSession::HXDeviceSetupCallback;
  protected:
    ~CHXAudioSession(void);
    void _ConstructIfNeeded();

    ULONG32 GetBlocksRemainingToPlay();
    
  private:

#ifdef _MACINTOSH
    static  HXBOOL                                    zm_Locked;
#endif

    LONG32             m_lRefCount;
    IHXScheduler*      m_pScheduler;
    IHXInterruptState* m_pInterruptState;
    HXAudioFormat      m_BeforeHookDeviceFmt;   // Actual Audio device format.
    HXAudioFormat      m_ActualDeviceFmt;       // Actual Audio device format.
    HXAudioFormat      m_DeviceFmt;             // Audio device format used for all calculations.
    IHXBuffer*         m_pPlayerBuf;
    IHXBuffer*         m_pSessionBuf;
    ULONG32            m_ulGranularity;         // Amount of audio to push (ms).
    double             m_dGranularity; 
    ULONG32            m_ulMinimumStartupPushdown;
    ULONG32            m_nPercentage;
    ULONG32            m_ulTargetBlocksTobeQueued;
    ULONG32            m_ulMinBlocksTobeQueued;
    ULONG32            m_ulMinBlocksTobeQueuedAtStart;
    ULONG32            m_ulMaxBlocksToPlayPerGranule;
    ULONG32            m_ulBytesPerGran;       // Number of bytes in gran
    ULONG32            m_ulBlocksWritten;      // Number of blocks written so far

    ULONG32  m_ulCallbackID;
    HXBOOL   m_bFakeAudioTimeline;
    

    HXBOOL m_bShouldOpenOnCoreThread;
    HXBOOL m_bToBeReOpened;

    HXDeviceSetupCallback* m_pDeviceCallback;
    HXBOOL                 m_bHasStreams;          // True if there are audio streams

    /* We keep a track of the current time when we use Scheduler for
     * genertaing time syncs.
     */
    ULONG32 m_ulIncreasingTimer;

    /* This is the actual time we report back to the player->renderers
     * It gets set in the OnTimeSync
     */
    ULONG32        m_ulCurrentTime;
    ULONG32        m_ulLastAudioTime;
    ULONG32        m_ulLastAudioReturnTime;
    ULONG32        m_ulLastSystemTime;
    HXBOOL         m_bAtLeastOneTimeReceived;
    ULONG32        m_ulStartTime;
    HXBOOL         m_bTimeSyncReceived;

    HXBOOL         m_bPaused;
    HXBOOL         m_bStoppedDuringPause;

    ULONG32        m_ulLastFakeCallbackTime;
    Timeval*       m_pFakeAudioCBTime;

    HXAudioData*   m_pInDataPtr;
    HXAudioData*   m_pOutDataPtr;

    /*
     * Should be in a data structure with the player objects and stuffed
     * on the list.
     */
    IHXAudioPlayerResponse* m_pPlayerResponse; // Notification interface
    HXBOOL                  m_bFirstPlayAudio;
    UINT16                  m_uVolume;              // Default volume
    HXBOOL                  m_bMute;                // the mute state

    /*
     * Continuous session play back time. This is the time written to the audio
     * device object for each buffer. This is the time that each audio player 
     * retrieves as its start time.
     */
    double  m_dBufEndTime;

    HXBOOL  m_bDisableWrite;
    HXBOOL  m_bInPlayAudio;
    HXBOOL  m_bInActualResume;

    double  m_dNumBytesWritten;
    double  m_dNumBytesPlayed;


    /* CheckAudioFormat Interface is called by audio player to 
     * check resample audio format with audio device format.
     */
    HX_RESULT CheckAudioFormat( HXAudioFormat* pAudioFormat );

    /* Create the playback buffer.
     */
    HX_RESULT CreatePlaybackBuffer(void);

    void        OnTimerCallback();
    inline void CheckAudioVolume()
    {}
    

    /* Write audio data to post mix hooks. */
    HX_RESULT ProcessPostMixHooks( CHXAudioPlayer* pPlayer,
                                   IHXBuffer*&    pInBuffer,
                                   HXBOOL*           bDisableWrite,
                                   UINT32          ulBufTime,
                                   HXBOOL&           bChanged);
    
    void InitHooks();
    void ProcessHooks(HXAudioData* pAudioData);

    /* Convert 16-bit buffer to 8-bit */
    void ConvertToEight(void);
    HXBOOL IsAudioOnlyTrue(void);
        
    UINT32    AnchorDeviceTime(UINT32 ulCurTime);
    void      AdjustInRealTime();
    HX_RESULT Rewind();
    void      ReopenDevice();

    void updateFakeTimeline();
    void updateCurrentTime(ULONG32 ulCurrentTime);
    
  protected:
    HXBOOL                m_bInited;          // is audio setup?
    IHXAudioDevice*     m_pAudioDev; // device layer
    IHXAudioDevice*     m_pCurrentAudioDev;  // keep track of the current device layer
    IHXAudioDevice*     m_pReplacedAudioDev; // keep track of replaced audio device
    HXBOOL              m_bReplacedDev;     // is audio device replaced?
    CHXSimpleList*      m_pPlayerList;      // list of player audio sessions
    CHXSimpleList*      m_pHookList;        // hook list
    CHXSimpleList*      m_pAudioDevHookList; // audio device hook list

    // Any audio buffers that we were not able to write to the audio 
    // device successfully
    CHXSimpleList*      m_pAuxiliaryAudioBuffers;

    IHXMutex*           m_pMutex;
    IHXAudioHook*       m_pFinalHook;
    HXBOOL              m_bUseFinalHook;
    IHXMutex*           m_pCoreMutex;
    UINT16              m_uAskFromAudioDevice;
    HXBOOL              m_bDeferActualResume;
    CHXAudioPlayer*     m_pLastPausedPlayer;
    HXBOOL              m_bUsingReplacedDevice;
    HXBOOL              m_bToBeRewound;
    UINT32              m_ulLastRewindTimestamp;
    UINT16              m_uNumToBePushed;
    UINT16              m_uNumToBePlayed;
    HXBOOL              m_bSessionBufferDirty;
    HXBOOL              m_bPostMixHooksUpdated;
    IHXPreferences*     m_pPreferences;
    HXBOOL              m_bAudioDeviceSupportsVolume;
    HXBOOL              m_bOpaqueMode;
    IHXBuffer*          m_pOpaqueData;
    CHXString           m_strOpaqueType;

    IHXMultiPlayPauseSupport* m_pMPPSupport;

    ULONG32             m_ulTargetPushdown;
    ULONG32             m_ulMinimumPushdown;
    void                UpdateMinimumPushdown();

#if defined(HELIX_FEATURE_TIMELINE_WATCHER)
    //IHXTimelineManager support.
    CHXSimpleList*      m_pTimeLineWatchers;
    void _NotifyTimelineWatchers( const int nWhich, const UINT32 ulNow=0 );
#else
    inline void _NotifyTimelineWatchers(const int nWhich, const UINT32 ulNow=0 ) {};
#endif    
    
#ifdef HELIX_FEATURE_VOLUME
    IHXVolume*          m_pDeviceVolume;
#endif    
};

#endif  // _HXAUDSES_H
