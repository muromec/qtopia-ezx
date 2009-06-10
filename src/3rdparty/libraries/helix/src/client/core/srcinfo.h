/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: srcinfo.h,v 1.27 2007/10/17 04:43:47 ehyche Exp $
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

#ifndef _SOURCEINFO_
#define _SOURCEINFO_

#include "hxcbobj.h"
#include "timeval.h"
#include "chxmaplongtoobj.h"
#include "hxbsrc.h"

// forward decl.
_INTERFACE IHXPlayer;                                
_INTERFACE IHXPlayerProps;
_INTERFACE IHXRenderer;
_INTERFACE IHXRendererDrivenPacketDelivery;
_INTERFACE IHXScheduler;
_INTERFACE IHXPendingStatus;
_INTERFACE IHXMetaTrack;
_INTERFACE IHXMetaLayout;

class HXSource;
class HXStream;
class CTimeSyncCallback;
class CSignatureChecker;

struct STREAM_INFO;

typedef ULONG32 BufferingReason;

struct RepeatInfo;
struct RendererInfo;
class  SourceInfo;
class  HXPlayer;
class  HXMutex;
struct IHXPluginSearchEnumerator;

#define MAX_ADD_TRACK_LATENCY_MSEC  1000 //  1 second
#define UNLIKELY_EVENT_TIME         0xFFFFFFFF

struct timeSyncParamStruct
{
    SourceInfo* pSourceInfo;
    RendererInfo* pRenderInfo;
};

struct RepeatInfo
{
    UINT16  uTrackID;
    UINT32  ulStart;
    UINT32  ulEnd;
    UINT32  ulDelay;
    UINT32  ulDuration;
};

// stores header info for each renderer
struct RendererInfo
{
    RendererInfo()
    {
        m_pRenderer                     = NULL;
        m_pRendererDrivenPacketDelivery = NULL;
        m_pRendererPendingStatus        = NULL;
        m_pStreamInfo                   = NULL;
        m_ulLatestEventTime             = UNLIKELY_EVENT_TIME;
        m_ulEarliestEventTime           = UNLIKELY_EVENT_TIME;
        m_ulGranularity                 = 0;
        m_ulLastSyncTime                = 0;
        m_ulNextDueSyncTime             = 0;
        m_ulDuration                    = 0;
        m_pTimeSyncCallback             = 0;
        m_pTimeSyncCBTime               = new Timeval;
        m_bIsFirstCallback              = TRUE;
        m_BufferingReason               = BUFFERING_START_UP;
        m_ulNumberOfPacketsQueued       = 0;
        m_bStartTimeValid               = FALSE;
        m_ulStreamStartTime             = 0;
        m_ulTimeDiff                    = 0;
        m_bIsFirstPacket                = TRUE;
        m_bIsFirstTimeSync              = TRUE;
        m_bIsPersistent                 = FALSE;
        m_pStream                       = NULL;
        m_pRendererEnumerator           = NULL;
        m_bInitialBeginToBeSent         = TRUE;
        m_bInterruptSafe                = FALSE;
        m_bDurationTimeSyncSent         = FALSE;
        m_bOnEndOfPacketSent            = FALSE;
        m_bRendererDrivenBuffering      = FALSE;
        m_bRendererDrivenPacketDelivery = FALSE;
    };

    ~RendererInfo()
    {
        if (m_pTimeSyncCBTime)
        {
            delete m_pTimeSyncCBTime;
            m_pTimeSyncCBTime = 0;
        }
    };

    IHXRenderer*                     m_pRenderer;
    IHXRendererDrivenPacketDelivery* m_pRendererDrivenPacketDelivery;
    IHXPendingStatus*                m_pRendererPendingStatus;
    STREAM_INFO*                     m_pStreamInfo;
    ULONG32                          m_ulLatestEventTime;
    ULONG32                          m_ulEarliestEventTime;
    ULONG32                          m_ulGranularity;
    ULONG32                          m_ulLastSyncTime;
    ULONG32                          m_ulNextDueSyncTime;
    ULONG32                          m_ulDuration;
    CTimeSyncCallback*               m_pTimeSyncCallback;
    Timeval*                         m_pTimeSyncCBTime;
    BufferingReason                  m_BufferingReason;
    UINT32                           m_ulStreamStartTime;
    UINT32                           m_ulTimeDiff;
    HXStream*                        m_pStream;
    IHXPluginSearchEnumerator*       m_pRendererEnumerator;
    UINT16                           m_ulNumberOfPacketsQueued : 16;
    HX_BITFIELD                      m_bIsCallbackPending : 1;
    HX_BITFIELD                      m_bIsFirstCallback : 1;
    HX_BITFIELD                      m_bIsFirstPacket : 1;
    HX_BITFIELD                      m_bIsFirstTimeSync : 1;
    HX_BITFIELD                      m_bIsPersistent : 1;
    HX_BITFIELD                      m_bInitialBeginToBeSent : 1;
    HX_BITFIELD                      m_bInterruptSafe : 1;
    HX_BITFIELD                      m_bDurationTimeSyncSent : 1;
    HX_BITFIELD                      m_bOnEndOfPacketSent : 1;
    HX_BITFIELD                      m_bStartTimeValid:1;
    HX_BITFIELD                      m_bIsAudioRenderer:1;
    HX_BITFIELD                      m_bRendererDrivenBuffering : 1;
    HX_BITFIELD                      m_bRendererDrivenPacketDelivery : 1;
};

/*
 * -- LIVE SYNC SUPPORT --
 *
 * The SharedWallClock class is used for supporting syncing of
 * multiple live sources. See other LIVE SYNC SUPPORT comments 
 * in the source code for more details.
 */
class SourceInfo;

class SharedWallClock
{
protected:
    CHXString           m_strName;
    UINT32              m_ulStartTime;
    CHXSimpleList       m_UserList;

    /* A wall clock is only associated with one player */
    HXPlayer*           m_pPlayer; 

public:
    SharedWallClock(const char* pName, UINT32 ulStartTime,HXPlayer* pPlayer);

    UINT32              ResetStartTime(UINT32 ulStartTime);

    UINT32              GetStartTime()  { return m_ulStartTime; };
    void                AddUser(SourceInfo* pSourceInfo);
    void                RemoveUser(SourceInfo* pSourceInfo);
};

class SourceInfo
{
public:
                        SourceInfo(HXPlayer* pPlayer);
                        ~SourceInfo();

    HX_RESULT           Begin(void);
    HX_RESULT           Pause(void);
    HX_RESULT           Seek(UINT32 ulSeekTo);
    void                Stop(EndCode endCode = END_STOP);
    void                Remove(void);
    void                Reset(void);
    virtual void        DoCleanup(EndCode endCode = END_STOP);
    void                RenderersCleanup(void);
    virtual void        RenderersCleanupExt(RendererInfo* pRendInfo);

    virtual RendererInfo* NewRendererInfo();

    HXBOOL              FirstFrameDisplayed();
    HXBOOL              HasSourceRenderingBegun(void);
    HX_RESULT           BeginTrack(void);    
    HX_RESULT           PauseTrack(void);
    HX_RESULT           SeekTrack(UINT32 ulSeekTime);    
    HX_RESULT           StopTrack(void);    
    HX_RESULT           SetSoundLevel(UINT16 uSoundLevel, HXBOOL bReflushAudioDevice);

    HX_RESULT           ProcessIdle(ULONG32&    ulNumStreamsToBeFilled,
                                    HXBOOL&     bIsBuffering,
                                    UINT16&     uLowestBuffering,
                                    UINT32      ulLoopEntryTime,
                                    UINT32      ulProcessingTimeAllowance);
    HX_RESULT           Register(void);
    HX_RESULT           UnRegister(void);
    void                CheckIfDone(void);
    void                SetupRendererSites(HXBOOL bIsPersistent);
    HX_RESULT           InitializeAndSetupRendererSites();
    HX_RESULT           InitializeRenderers(HXBOOL& bSourceInitialized);
    virtual HX_RESULT   InitializeRenderersExt(HXBOOL& bSourceInitialized);
    HX_RESULT           SetupStreams(void);

    HX_RESULT           SetupRenderer(RendererInfo*& pRendInfo, 
                                      IHXRenderer*& pRenderer,
                                      STREAM_INFO*& pStreamInfo, 
                                      HXStream*& pStream);
    HX_RESULT           CloseRenderers(void);
    HX_RESULT           OnTimeSync(ULONG32 ulCurrentTime);
    HX_RESULT           OnTimeSync(RendererInfo* pRendInfo, 
                                   HXBOOL bUseCurrentTime = FALSE);

    void                ChangeAccelerationStatus(HXBOOL bMayBeAccelerated,
                                         HXBOOL bUseAccelerationFactor = FALSE,
                                         UINT32 ulAccelerationFactor = 0);
    HXBOOL              IsInitialized()     {return m_bInitialized;};
    HXBOOL              AreStreamsSetup()           {return m_bAreStreamsSetup;};
    
    HXBOOL              IsRegistered()      {return m_bIsRegisterSourceDone;}; 
    HXBOOL              IsActive()          {return m_bActive;}; 
    void                Resumed();
    void                UpdateDuration(UINT32 ulDuration);
    void                UpdateDelay(UINT32 ulDelay);
    HXBOOL              ToBeInitialized() {return m_bTobeInitializedBeforeBegin;};
    void                ReInitializeStats();
    HXBOOL              IsRebufferDone(void);
    HX_RESULT           HandleRedirectRequest();
    HX_RESULT           HandleSDPRequest();
    void                ScheduleProcessCallback();
    void                SetLiveSyncStartTime(HXSource* pSource, 
                                             RendererInfo* pTmpRendInfo, 
                                             UINT32 ulLowestTime);
    HXBOOL              AllOtherStreamsHaveEnded(STREAM_INFO* pThisStreamInfo);    

    CHXSimpleList*      GetRepeatList() {return m_bLeadingSource?m_pRepeatList:m_pPeerSourceInfo->m_pRepeatList; };
    HX_RESULT           AppendRepeatRequest(UINT16 uTrackID, IHXValues* pTrack);
    HXBOOL              IsAdjustSeekNeeded(UINT32 ulSeekTime);
    SourceInfo*         DoAdjustSeek(UINT32 ulSeekTime);
    UINT32              GetActiveDuration();
    HXBOOL              KeepSourceActive(void);    
    HXBOOL              IsDone() { return (HXBOOL)m_bDone;};
    HX_RESULT           AddToAltURLList(IHXBuffer* pURLStr, HXBOOL bDefault);
    void                ClearAltURLList();
    HXBOOL              HasAltURL();
    HX_RESULT           GetAltURL(IHXBuffer** ppBuf, HXBOOL* pbDefault);
    HX_RESULT           CopyAltURLListToNewSourceInfo(SourceInfo* pSourceInfo);

    friend class HXPlayer;
    friend class NextGroupManager;
    friend class PrefetchManager;
    friend class CTimeSyncCallback;
    friend class ProcessCallback;
    friend class HXPersistentComponent;
    friend class HXPersistentComponentManager;
    friend class HXSource;
    friend class HXFileSource;
    friend class HXNetSource;

    // XXX HP
    LISTPOSITION        m_curPosition;
    CHXSimpleList*      m_pRepeatList;    
    SourceInfo*         m_pPeerSourceInfo;
    HXSource*           m_pSource;
    HX_BITFIELD         m_bLeadingSource : 1;
    HX_BITFIELD         m_bSeekPending : 1;
    HX_BITFIELD         m_bIndefiniteDuration : 1;
    HX_BITFIELD         m_bRepeatPending : 1;
    HX_BITFIELD         m_bRepeatIndefinite : 1;    
    HX_BITFIELD         m_bSeekToLastFrame : 1;
    HX_BITFIELD         m_bNetSourceFailed : 1;
    HX_BITFIELD         m_bFileSourceFailed : 1;
    UINT16              m_uGroupID;
    UINT16              m_uTrackID;
    UINT32              m_ulPersistentComponentID;
    UINT32              m_ulPersistentComponentSelfID;
    UINT32              m_ulMaxDuration;    
    UINT32              m_ulSeekTime;
    UINT32              m_ulRepeatInterval;
    UINT32              m_ulRepeatDelayTimeOffset;
    CHXMapLongToObj*    m_pRendererMap;     // list of renderers for this source...

protected:
    HXPlayer*           m_pPlayer;
    IHXPendingStatus*   m_pStatus;
    IHXRendererAdviseSink* m_pRendererAdviseSink;

    HX_BITFIELD         m_bDone : 1;        // source end...all streams are done..
    HX_BITFIELD         m_bStopped : 1;     // track stopped
    HX_BITFIELD         m_bInitialized : 1; // all streams of source have been 
                                            // associated with renderers?
    HX_BITFIELD         m_bAllPacketsReceived : 1;
    HX_BITFIELD         m_bActive : 1;
    HX_BITFIELD         m_bIsPersistentSource : 1;
    HX_BITFIELD         m_bIsRegisterSourceDone : 1;
    HX_BITFIELD         m_bAltURL : 1;
    HX_BITFIELD         m_bToBeResumed : 1;
    HX_BITFIELD         m_bAreStreamsSetup : 1;
    HX_BITFIELD         m_bTrackStartedToBeSent : 1;
    HX_BITFIELD         m_bTrackStoppedToBeSent : 1;
    HX_BITFIELD         m_bPrefetch : 1;
    HX_BITFIELD         m_bLoadPluginAttempted : 1;
    HX_BITFIELD         m_bTobeInitializedBeforeBegin : 1;
    HX_BITFIELD         m_bLocked : 1;
    HX_BITFIELD         m_bIsTrackDurationSet : 1;
    HX_BITFIELD         m_bDurationTimeSyncScheduled : 1;
    HX_BITFIELD         m_bAudioDeviceReflushHint : 1;
    HX_BITFIELD         m_bSeekOnLateBegin: 1;
    UINT32              m_ulSeekOnLateBeginTolerance;
    HX_RESULT           m_lastErrorFromMainURL;
    HX_RESULT           m_lastError;
    CHXString           m_lastErrorStringFromMainURL;
    CHXString           m_id;
    CHXSimpleList*      m_pCurrentScheduleList;
    /*
           repeat0            repeat1             repeat2            repeat3
     |------------------|------------------|------------------|------------------|
     ^          ^       ^                                                        ^
     |----------|       |                                                        |
   (sourceduration)     |                                                        |
     ^                  |                                                        |
     |------------------|                                                        |
       (trackduration)                                                           |
     ^                                                                           |
     |---------------------------------------------------------------------------|
                                (totaltrackduration)
     */
    UINT32              m_ulSourceDuration;     // encoded duration
    UINT32              m_ulTrackDuration;      // customized duration(per repeat)
    UINT32              m_ulTotalTrackDuration; // customized total duration(total repeats)
    UINT32              m_ulPausedStartTime;
    UINT16*             m_pDependNode;
    UINT16              m_uNumDependencies;
    IHXMutex*           m_pMutex;
    CHXGenericCallback*m_pProcessCallback;
    INT64               m_llLatestPacketTime;
    INT64               m_llEarliestPacketTime;

    PrefetchType        m_prefetchType;
    UINT32              m_ulPrefetchValue;
    UINT16              m_uSoundLevel;
    FillType            m_fillType;

    struct AltURLInfo
    {
        IHXBuffer* m_pAltURLStr;
        HXBOOL     m_bDefault;
    };

    CHXSimpleList*      m_pAltURLList;

#ifdef HELIX_FEATURE_PARTIALPLAYBACK
    HXBOOL      m_bIsPartialPlayback; 
#endif

/*
 * -- LIVE SYNC SUPPORT --
 *
 * The SharedWallClock class is used for supporting syncing of
 * multiple live sources. See other LIVE SYNC SUPPORT comments 
 * in the source code for more details.
 */
private:
    UINT32              CalculateLiveStartTime(IHXPacket* pFirstPacket);
    void                DoneWithWallClock();

    UINT32              m_ulStreamStartTime;
    CHXString           m_strWallClockName;
    SharedWallClock*    m_pWallClock;

public:
    void                ResetStartTime(UINT32 ulStartTime);
    static void         TimeSyncCallback(void* pParam);
    static void         ProcessCallback(void* pParam);
};

class CTimeSyncCallback : public CHXGenericCallback
{
public:
    CTimeSyncCallback(void* pParam, fGenericCBFunc pFunc)
     : CHXGenericCallback(pParam, pFunc)
    {
        m_pParam = (void*)new struct timeSyncParamStruct;
        *((struct timeSyncParamStruct*)m_pParam) = *((struct timeSyncParamStruct*)pParam);
    }

    virtual ~CTimeSyncCallback()
    {
        if (m_pParam)
        {
            delete (timeSyncParamStruct*)m_pParam;
        }
    }
};

#endif /*_SOURCEINFO_*/
