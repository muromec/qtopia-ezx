/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbsrc.h,v 1.60 2007/10/18 15:54:17 yuryrp Exp $
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

#ifndef _HX_BSOURCE_
#define _HX_BSOURCE_

#include "hxresult.h"
#include "hxtypes.h"
#include "hxurl.h"
#include "hxcomm.h"			// IHXRegistryID
#include "plprefk.h"
#include "hxmap.h"
#include "hxfiles.h"
#include "hxasm.h"
#include "hxmon.h"
#include "hxhyper.h"
#include "hxclreg.h"
#include "hxpends.h"
#include "chxelst.h"
#include "statsmgr.h"
#include "statinfo.h"
#include "strminfo.h"
#include "buffmgr.h"
#include "hxausvc.h"
#include "hxaudply.h"
#include "hxplugn.h"
#include "smiltype.h"
#include "recordctl.h"
#include "hxcore.h"
#include "ihxrateadaptctl.h"
#include "ihxtranstime.h"

#if defined(HELIX_FEATURE_DRM)
#include "hxsrcin.h"
#include "hxdrm.h"
class HXDRM;
#endif /* HELIX_FEATURE_DRM */

// need to go in hxresult.h
#define HX_INVALID_HEADER			HXR_INVALID_PARAMETER

#define NUM_MILLISECS_IN_A_SEC			((ULONG32) 1000)

#define HX_LATE_PACKET				HXR_FAILED
#define HX_DUPLICATE_PACKET			(HXR_FAILED + 1)


///////////////////////////////////////////////////////////////
// PERFECTPLAY_FUDGE_FACTOR
//
// Percentage of average download to consider for calculations. 
// 100 = 100% and assumes that the average download time is 
// correct. Larger numbers (i.e. 110) would handle the case of 
// the average being lucky as if all future blocks come in at 
// slower rate.
#define PERFECTPLAY_FUDGE_FACTOR		(110)	

///////////////////////////////////////////////////////////////
// PERFECTPLAY_FUDGE_FACTOR_BASE
//
// Base unit of measure for percentage of average download to 
// consider for calculations.
#define PERFECTPLAY_FUDGE_FACTOR_BASE	(100)	

///////////////////////////////////////////////////////////////
// PERFECTPLAY_MIN_BUFFERS
//
// Due to the behavior of pn_net's de-interleaving support
// we must make sure that we have at least 3 superblocks of
// buffers before we can start playback in perfect play mode.
// I have tried smaller amounts, in particular I thought 
// MIN_LEAD_BLOCKS would be a logical number, but this was not
// conservative enough and the playback would stop prematurely
// even though there was data available.
#define PERFECTPLAY_MIN_BUFFERS		(mInterleaveFactor * 3)

///////////////////////////////////////////////////////////////
// PERFECTPLAY_TICKS_PER_SECOND
//
// Since the different OS's return different meanings for "ticks"
// we will use this as the ratio of ticks to seconds.
#ifdef __MWERKS__
#define PERFECTPLAY_TICKS_PER_SECOND		60
#else
#define PERFECTPLAY_TICKS_PER_SECOND		100
#endif

///////////////////////////////////////////////////////////////
// PERFECTPLAY_MIN_TIME
//
// Also due to the behavior of pn_net's de-interleaving support
// we can't play for less than about 3 superblocks of audio, so
// this define will represent this in seconds.
// This is a reasonable minimun perfect play time
#define	PERFECTPLAY_MIN_TIME	10

#define MINIMUM_PLAY_TIME   10000 /* in ms.*/

/* Approx Time to be taken for network connection for in-play joining sources */
#define NETWORK_FUDGE_FACTOR			2000

// The in-play joining source will be required to complete its buffering the
// below amount of time ahead of its schedule point of joining the presentation.
#define MIN_BUFFERING_COMPLETION_LEAD_AHEAD_OF_DELAYED_START	30

#define RENDERER_INTERFACE_SIZE	16
#define SECURE_RENDERER_INTERFACE_SIZE	5


///////////////////////////////////////////////////////////////
// RENDERING DISABLEMENT MASK SETTING DEFINITIONS
//
// Setting of these masking bits dictates that renderer for the
// stream of indicated type will not be loaded and not used to render 
// the stream.  The mask bits are used with m_ulRenderingDisabledMask
// 32 bit mask within HXSource.
// Only HXRNDR_DISABLE_ALL is supported at this time (6/29/2006).
#define HXRNDR_DISABLED_NONE	0x00000000
#define HXRNDR_DISABLED_VIDEO	0x00000001
#define HXRNDR_DISABLED_AUDIO	0x00000002
#define HXRNDR_DISABLED_NONAV	0x00000004
#define HXRNDR_DISABLED_ALL	0xFFFFFFFF


// forward decl.
class HXPlayer;
class CHXEvent;
class CHXEventList;
class HXClientEngine;

class Plugin2Handler;

class CBufferManager;
class SourceInfo;
class HXUpgradeCollection;

struct IHXPacket;
struct IHXStreamSource;
struct IHXValues;
struct IHXStream;
struct IHXClientEngine;
struct IHXPlayer;
struct IHXPrivateStreamSource;
struct IHXDigitalRightsManager;

// if you change these two constants please change
// ProtocolName and StreamName size in RaCOnnectionInfo
// structure in hxtypes.h as well...
#define MAX_STREAM_NAME		50
#define MAX_PROTOCOL_NAME	20

#define HX_EOF_TIME		0	// use to indicate to netplay to play until EOF of stream

#define RAM_MIMETYPE		"application/ram"

#define MAX_INTERSTREAM_TIMESTAMP_JITTER    5000	// In milliseconds

// XXXTW.	There is a copy of this in rpplmrg\rpplmgr.cpp.  Update both,
// 			if either.
enum PlayMode
{
    NORMAL_PLAY = 0,
    BUFFERED_PLAY,
    PERFECT_PLAY,
    BUFFERED_PLAY_NOT_ENTIRE_CLIP,
    PERFECT_PLAY_NOT_ENTIRE_CLIP
};

enum TurboPlayMode
{
    TURBO_PLAY_UNKNOWN,
    TURBO_PLAY_ON,
    TURBO_PLAY_OFF
};

enum RebufferStatus
{
    REBUFFER_NONE = 0,
    REBUFFER_WARNING,
    REBUFFER_REQUIRED
};

enum TurboPlayOffReason
{
    TP_OFF_BY_UNKNOWN = 0,
    TP_OFF_BY_PREFERENCE,
    TP_OFF_BY_SERVER,
    TP_OFF_BY_REBUFFER,
    TP_OFF_BY_NOTENOUGHBW,
    TP_OFF_BY_NETCONGESTION,
    TP_OFF_BY_NONRTSP,
    TP_OFF_BY_LIVE,
    TP_OFF_BY_ROB,
    TP_OFF_BY_INVALIDTLC,
    TP_OFF_BY_MULTISURESTREAMS    
};

enum EndCode
{
    END_UNKNOWN = 0,
    END_DURATION,
    END_STOP,
    END_RECONNECT,
    END_REDIRECT,
    END_ABORT,
    END_REMOVE /* for RemoveTrack() */
};

typedef enum
{
    PUSHDOWN_NONE = 0,
    PUSHDOWN_AUDIO_DONE = 1,
    PUSHDOWN_VIDEO_DONE = 2,
    PUSHDOWN_ALL_DONE = 3
} PushDownStatus;

struct TurboPlayStats
{
    TurboPlayOffReason	tpOffReason;
    HXBOOL		bBufferDone;
    UINT32		ulAcceleratedBW;
    UINT32		ulBufferedTime;

    TurboPlayStats()
    {
	tpOffReason = TP_OFF_BY_UNKNOWN;
	bBufferDone = FALSE;
	ulAcceleratedBW = 0;
	ulBufferedTime = 0;
    }
};

class HXSource : public IHXStreamSource,
                 public IHXStreamSource2,
		  public IHXPendingStatus,
		  public IHXRegistryID,
		  public IHXInfoLogger,
		  public IHXPrivateStreamSource,
		  public IHXBackChannel,
		  public IHXASMSource,
                  public IHXClientRateAdaptControl,
                  public IHXTransportTimeManager,
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
		  public IHXHyperNavigate,
		  public IHXHyperNavigate2,
#endif /* defined(HELIX_FEATURE_HYPER_NAVIGATE) */
                  public IHXSourceLatencyStats,
		  public IHXAudioLevelNormalization,
                  public IHXPlaybackVelocity
{
protected:
    LONG32			m_lRefCount;
    ULONG32			m_ulStreamIndex;

    HXPlayer*			m_pPlayer;
    
    CBufferManager*		m_pBufferManager;

    friend class CBufferManager;

    //Returns TRUE if rebuffer was issued.
    HXBOOL DoRebufferIfNeeded();
    
public:
    // statistic info.
    HXClientRegistry*		m_pRegistry;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    StatsManager*		m_pStatsManager;
    SOURCE_STATS*		m_pStats;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    IHXValues*			m_pFileHeader;
    RebufferStatus              m_rebufferStatus;
    HX_BITFIELD			m_bRTSPRuleFlagWorkAround : 1;
    HX_BITFIELD			m_bReSetup : 1;
    HX_BITFIELD			m_bFastStart : 1;
    EndCode			m_srcEndCode;
    TurboPlayStats		m_turboPlayStats;
    TurboPlayMode		m_serverTurboPlay;
    HX_BITFIELD			m_bSureStreamClip : 1;
    SourceInfo*			m_pSourceInfo;
    UINT32			m_ulMaxBandwidth;
    UINT32			m_ulDelay;    
    UINT32			m_ulOriginalDuration;
    UINT32			m_ulOriginalDelay;
    UINT32			m_ulPrefetchDelay;
    UINT32			m_ulMaxPreRoll; 
    UINT32			m_ulRegistryID;
    UINT32			m_ulMinAudioTurboPushDown;
    UINT32                      m_ulMinVideoTurboPushDown;
    UINT32                      m_ulRebufferingStartTix;
    UINT32                      m_ulRebufferingCumulativeTime;
    UINT32                      m_ulPlaybackStartedTix;
    UINT32                      m_ulPlaybackStoppedTix;
    CHXSimpleList               m_PacketBufferList;
    HX_BITFIELD                 m_bContinueWithHeaders : 1;
    HX_BITFIELD			m_bHasSubordinateLifetime : 1;
    HX_BITFIELD                 m_bRenderersControlBuffering : 1;

protected:
    HX_BITFIELD			m_bDefaultAltURL : 1;

public:
				HXSource(void);

    /*
     * IUnknown methods
     */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXStreamSource methods
     */

    /************************************************************************
     *	Method:
     *		IHXStreamSource::IsLive
     *	Purpose:
     *		Ask the source whether it is live
     *
     */
    STDMETHOD_ (HXBOOL,IsLive)	(THIS);
    
    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetPlayer
     *	Purpose:
     *	    Get the interface to the player of which the source is
     *	    a part of.
     *
     */
    STDMETHOD(GetPlayer)	(THIS_
				REF(IHXPlayer*)    pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetContext
     *	Purpose:
     *	    Get the interface to the context of which the source is
     *	    a part of.
     *
     */
    STDMETHOD(GetContext)	(THIS_
				REF(IUnknown*)	pContext);

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetURL
     *	Purpose:
     *	    Get the URL for this source. NOTE: The returned string is
     *	    assumed to be valid for the life of the IHXStreamSource from which it
     *	    was returned.
     *
     */
    STDMETHOD_(const char*,GetURL)  (THIS);

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetStreamCount
     *	Purpose:
     *	    Returns the current number of stream instances supported by
     *	    this source instance.
     */
    STDMETHOD_(UINT16, GetStreamCount)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetStream
     *	Purpose:
     *	    Returns the Nth stream instance supported by this source.
     */
    STDMETHOD(GetStream)	(THIS_
				UINT16		nIndex,
				REF(IUnknown*)	pUnknown);

    // IHXStreamSource2 methods
    STDMETHOD(GetGroupIndex)            (THIS_ REF(UINT16) rusGroupIndex);
    STDMETHOD(GetTrackIndex)            (THIS_ REF(UINT16) rusTrackIndex);
    STDMETHOD(GetPersistentComponentID) (THIS_ REF(UINT32) rulID);

    /************************************************************************
     *	Method:
     *		HXSource::ReportError
     *	Purpose:
     *		The protocol object reports of any fatal errors. 
     *
     */
    void	ReportError (HX_RESULT theErr);


    // IHXPendingStatus methods

    /************************************************************************
     *	Method:
     *	    IHXPendingStatus::GetStatus
     *	Purpose:
     *	    Called by the user to get the current pending status from an object
     */
    STDMETHOD(GetStatus)	(THIS_
				REF(UINT16) uStatusCode, 
				REF(IHXBuffer*) pStatusDesc, 
				REF(UINT16) ulPercentDone) = 0;

    /*
     *	IHXRegistryID methods
     */

    /************************************************************************
     *	Method:
     *	    IHXRegistryID::GetID
     *	Purpose:
     *	    Get registry ID(hash_key) of the objects(player, source and stream)
     *
     */
    STDMETHOD(GetID)		(THIS_
				REF(UINT32) /*OUT*/  ulRegistryID) = 0;


    // IHXPrivateStreamSource methods 
    STDMETHOD_ (HXBOOL,IsSaveAllowed)	(THIS);


    /*
     * IHXBackChannel method
     */
    /************************************************************************
     *	Method:
     *	    IHXBackChannel::PacketReady
     *	Purpose:
     *      A back channel packet sent from Renderer to FileFormat plugin.
     */
    STDMETHOD(PacketReady)	(THIS_
				IHXPacket* pPacket);

    /*
     * IHXASMSource methods
     */

    /************************************************************************
     *	Method:
     *	    IHXASMSource::Subscribe
     *	Purpose:
     *      Called to inform a file format that a subscription has occurred,
     *	    to rule number uRuleNumber, for stream uStreamNumber.
     */
    STDMETHOD(Subscribe)	(THIS_
				UINT16	uStreamNumber,
				UINT16	uRuleNumber);

    /************************************************************************
     *	Method:
     *	    IHXASMSource::Unsubscribe
     *	Purpose:
     *      Called to inform a file format that a unsubscription has occurred,
     *	    to rule number uRuleNumber, for stream uStreamNumber.
     */
    STDMETHOD(Unsubscribe)	(THIS_
				UINT16	uStreamNumber,
				UINT16	uRuleNumber);
    // IHXInfoLogger method

    /************************************************************************
     *	Method:
     *	    IHXInfoLogger::LogInformation
     *	Purpose:
     *	    Logs any user defined information in form of action and 
     *	    associated data.
     */
    STDMETHOD(LogInformation)		(THIS_				
					const char* /*IN*/ pAction,
					const char* /*IN*/ pData) = 0;
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    /*
     *	IHXHyperNavigate methods
     */

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigate::GoToURL
     *	Purpose:
     *	    Performs a simple Go To URL operation.
     */
    STDMETHOD(GoToURL)	    (THIS_
			    const char* pURL,
			    const char* pTarget);

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigate2::Execute
     *	Purpose:
     *	    
     *	Parameters:
     *      pURL:	    URL (absolute or relative)
     *	    pTargetInstance:	
     *	    pTargetApplication: 
     *	    pTargetRegion:
     *	    pParams:
     */
    STDMETHOD(Execute)	    (THIS_
			    const char* pURL,
			    const char* pTargetInstance,
			    const char* pTargetApplication,
			    const char* pTargetRegion,
			    IHXValues* pParams);
#endif /* defined(HELIX_FEATURE_HYPER_NAVIGATE) */

    /*
     * IHXClientRateAdaptControl Methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientRateAdaptControl::Enable
     *	Purpose:
     *	    Enable client rate adaptation for the specified stream
     *
     */
    STDMETHOD(Enable) (THIS_ UINT16 uStreamNum);

    /************************************************************************
     *	Method:
     *	    IHXClientRateAdaptControl::Disable
     *	Purpose:
     *	    Disable client rate adaptation for the specified stream
     *
     */
    STDMETHOD(Disable) (THIS_ UINT16 uStreamNum);

    /************************************************************************
     *	Method:
     *	    IHXClientRateAdaptControl::IsEnabled
     *	Purpose:
     *	    Is client rate adaptation enabled for the specified stream
     *
     */
    STDMETHOD(IsEnabled) (THIS_ UINT16 uStreamNum,
                          REF(HXBOOL) bEnabled);

    /*
     * IHXTransportTimeManager methods
     */
    STDMETHOD(AddSink) (THIS_ IHXTransportTimeSink* pSink);
    STDMETHOD(RemoveSink) (THIS_ IHXTransportTimeSink* pSink);

    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStats::SetLiveSyncOffset
     *	Purpose:
     *	    set the live sync start time
     */
    STDMETHOD(SetLiveSyncOffset)  (THIS_ UINT32  ulLiveSyncStartTime);

    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStatsStuff::NewPacketTimeStamp
     *	Purpose:
     *      call this for each arriving packet!
     */

    STDMETHOD(NewPacketTimeStamp)  (THIS_ UINT32  ulDueTimeStamp);

    
    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStatsStuff::GetLatencyStats
     *	Purpose:
     *      call this for each arriving packet!
     */

    STDMETHOD(GetLatencyStats)  (THIS_
                                     REF(UINT32) ulAverageLatency,
                                     REF(UINT32) ulMinimumLatency,
                                     REF(UINT32) ulMaximumLatency );
    
    
    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStatsStuff::ResetLatencyStats
     *	Purpose:
     *      call this for each arriving packet!
     */

    STDMETHOD(ResetLatencyStats)  (THIS_ );
    
    /*
     *  IHXAudioLevelNormalization methods
     */
    STDMETHOD(SetSoundLevelOffset)  (INT16 nOffset); 
    STDMETHOD_(INT16, GetSoundLevelOffset)() {return m_nSoundLevelOffset;}

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse);
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps);
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    STDMETHOD_(INT32,GetVelocity)          (THIS) { return m_lPlaybackVelocity; }
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode);
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS) { return m_bKeyFrameMode; }
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS) { return m_ulKeyFrameInterval; }
    STDMETHOD(CloseVelocityControl)        (THIS);

#if defined(HELIX_FEATURE_DRM)
    // mirror IHXSourceInput methods; HXSource doesn't really need to implement IHXSourceInput
    STDMETHOD(OnFileHeader)     (THIS_ HX_RESULT status, IHXValues* pValues);
    STDMETHOD(OnStreamHeader)	(THIS_ HX_RESULT status, IHXValues* pValues);
    STDMETHOD(OnStreamDone)	(THIS_ HX_RESULT status, UINT16 unStreamNumber);
    STDMETHOD(OnTermination)	(THIS_ HX_RESULT status);
#endif /* defined(HELIX_FEATURE_DRM) */


    HX_RESULT		Init(HXPlayer * player, UINT32 unRegistryID);
    HX_RESULT		SetupRegistry(void);
    virtual HX_RESULT	UpdateRegistry(UINT32 ulRegistryID) = 0;

    virtual HXBOOL 	IsInitialized(void)	{return m_bInitialized;};
    virtual HXBOOL	IsPreBufferingDone(void) {return m_bIsPreBufferingDone;};

    void		AddHXStream(HXStream* pStream);
    HX_RESULT		CollectAudioStreams(CHXSimpleList*& pAudioStreamList);
    void		RemoveAudioStreams(void);
    void		PauseAudioStreams(void);
    void		ResumeAudioStreams(void);
    void		ReleaseAudioStreams(CHXSimpleList* pAudioStreamList);

    void		InitialBufferingDone(void);

    void		DoRebuffer(UINT32 ulLoopEntryTime = 0,
				   UINT32 ulProcessingTimeAllowance = 0);    
    HXBOOL		IsRebufferDone(void);
    HXBOOL                FastStartPrerequisitesFullfilled(REF(UINT16) uStatusCode, REF(UINT16) ulPercentDone);
    void                FastStartUpdateInfo(void);
    HXBOOL		HasSubordinateLifetime() { return m_bHasSubordinateLifetime; }
    
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    virtual HX_RESULT	CopyMetaDataToRegistry(IHXValues* pHeader);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    virtual HX_RESULT	DoCleanup(EndCode endCode = END_STOP);

	    void	Stop(void);

    virtual HX_RESULT	DoSeek( ULONG32 seekTime) = 0;
    
    virtual HX_RESULT	DoPause(void) = 0;

    virtual HX_RESULT	DoResume(UINT32 ulLoopEntryTime = 0,
				 UINT32 ulProcessingTimeAllowance = 0) = 0;

    virtual HX_RESULT	StartInitialization(void) = 0;

    virtual HX_RESULT StopInitialization(void) = 0;
    virtual UINT16	GetNumStreams(void) = 0;
    	
    virtual HX_RESULT	GetStreamInfo(ULONG32 ulStreamNumber,
				      STREAM_INFO*& theStreamInfo) = 0;
    
    virtual HX_RESULT	GetEvent(UINT16 usStreamNumber, 
				 CHXEvent * &theEvent, 
				 UINT32 ulLoopEntryTime,
				 UINT32 ulProcessingTimeAllowance) = 0;

    virtual HX_RESULT	UpdateStatistics(void) = 0;

    virtual void	SetMinimumPreroll();

    virtual HX_RESULT	GetStreamHeaderInfo(UINT16 index, 
					    IHXValues * &hdr);
    
    virtual HX_RESULT	    ProcessIdle(UINT32 ulLoopEntryTime = 0, UINT32 ulProcessingTimeAllowance = 0)	// calls _ProcessIdle (FALSE)
			    {
				return _ProcessIdle(FALSE, ulLoopEntryTime, ulProcessingTimeAllowance);
			    }

#if defined (__MWERKS__)	
    virtual HX_RESULT	    ProcessIdleAtInterrupt(UINT32 ulLoopEntryTime = 0, UINT32 ulProcessingTimeAllowance = 0)	// calls _ProcessIdle (TRUE)
			    {
				return _ProcessIdle(TRUE, ulLoopEntryTime, ulProcessingTimeAllowance);
			    }

#endif // (__MWERKS__)	

	    HX_RESULT	GetLastError(void) { return mLastError;};

    virtual HXBOOL	IsLocalSource()  {return TRUE;};
    virtual HXBOOL	IsPNAProtocol()  {return FALSE;};
    virtual HXBOOL	IsNetworkAccess()  {return FALSE;};
    virtual HXBOOL	IsSimulatedNetworkPlayback()  {return FALSE;};
    
    virtual void	EventReady(CHXEvent* pEvent);

    // to set and get flags for livestream, saveas, perfectplay...
    virtual	void		SetFlags(UINT16 flags);
    virtual	UINT16		GetFlags() {return mFlags;};

    virtual	HXBOOL		IsPerfectPlayAllowed() { return m_bPerfectPlayAllowed;};
    virtual	HXBOOL		IsPerfectPlay()	       { return m_bPerfectPlay;};

   
    virtual	HX_RESULT	SetPlayTimes(UINT32 ulStartTime, 
					     UINT32 ulEndTime, 
					     UINT32 ulDelay, 
					     UINT32 ulDuration);

    virtual	HX_RESULT	UpdatePlayTimes(IHXValues* pValues); 

    virtual	HXBOOL		IsSourceDone() = 0;
                HXBOOL            IsPlaying();


    // tell about end of source...
    virtual	void		SetEndOfClip(HXBOOL bForcedEndofClip = FALSE) = 0;

    virtual	void		EnterBufferedPlay(void) {};
    virtual	void		LeaveBufferedPlay(void) {};

    ULONG32			GetDuration() const { return m_ulDuration; };

    IHXClientEngine*		GetClientEngine() const { return (IHXClientEngine*) m_pEngine; };
   
    UINT32			GetRenderingDisabledMask() { return m_ulRenderingDisabledMask; }
    UINT32 			GetStartTime() const { return m_ulStartTime; };
    UINT32 			GetEndTime()   const { return m_ulEndTime; }; 
    UINT32 			GetDelay()   const { return m_ulDelay; };
    CHXURL*			GetCHXURL() { return m_pURL; };

    HXBOOL			IsDelayed() const   { return m_bDelayed; };
    HXBOOL			IsSeekable() const {return !m_bNonSeekable;};
    HXBOOL			IsPaused() const {return m_bPaused;};

    INT64			GetLastExpectedPacketTime() const 
				    {return m_llLastExpectedPacketTime;};
    
    void			PartOfNextGroup(HXBOOL bPartOfNextGroup) { m_bPartOfNextGroup = bPartOfNextGroup;};
    HXBOOL			IsPartOfNextGroup(void) { return m_bPartOfNextGroup; };

    void			PartOfPrefetchGroup(HXBOOL bPartOfPrefetchGroup) { m_bPartOfPrefetchGroup = bPartOfPrefetchGroup;};
    HXBOOL			IsPartOfPrefetchGroup(void) { return m_bPartOfPrefetchGroup; };

    HX_RESULT                   ReportRebufferStatus(UINT16 uStreamNumber,
                                                     HXBOOL   bAudio,
					             UINT8  unNeeded, 
					             UINT8  unAvailable);

    STDMETHOD(SetGranularity)		(THIS_
					UINT16 uStreamNumber,
					ULONG32 ulGranularity);

	    void		SetSourceInfo(SourceInfo* pSourceInfo)  
				    {m_pSourceInfo = pSourceInfo;};

    virtual void		AdjustClipBandwidthStats(HXBOOL bActivate = FALSE) = 0;

    virtual HXBOOL		CanBeResumed(void)  = 0;

	    HXBOOL		isRestrictedLiveStream() {return m_bRestrictedLiveStream;};

	    HXBOOL		IsPassedResumeTime(UINT32 ulNeededLeadTime);

    virtual HXBOOL		TryResume(void);

    virtual char*		GetHost() {return NULL;};

    virtual void		ReSetup(void) = 0;

	    HXBOOL		IsResumePending() { return m_bResumePending;};

    virtual char*		GetAltURL(HXBOOL& bDefault);
            HXBOOL              HasAltURL();

	    void		SetAltURLType(HXBOOL bDefault) { m_bDefaultAltURL = bDefault; };

	    HXBOOL		IsActive(void) {return m_bIsActive;};

	    HX_RESULT		SetRequest(const CHXURL* pURL, HXBOOL bAltURL);
	    HX_RESULT		GetRequest(REF(IHXRequest*) pRequest)
				{
				    pRequest = m_pRequest;

				    if (m_pRequest)
				    {
					m_pRequest->AddRef();
				    }

				    return HXR_OK;
				};

	    void		UpdateDuration(UINT32 ulDuration);
	    void		UpdateDelay(UINT32 ulDelay);
	    void		ScheduleProcessCallback();
	    void		MergeUpgradeRequest(HXBOOL bAddDefault = FALSE, char* pUpgradeString = NULL);
	    void		ClearUpgradeRequest();

            void                DeleteAllEvents();
	    void		EnterPrefetch(PrefetchType prefetchType, UINT32 ulPrefetchValue);
	    virtual void	LeavePrefetch(void);
	    virtual HXBOOL	IsPrefetchDone(void) {return TRUE;};
	    void		SetSoundLevel(UINT16 uSoundLevel, HXBOOL bReflushAudioDevice);
	    void		SetAudioDeviceReflushHint(void);
            void                SetMaxPossibleAccelRatio(double maxR) {m_maxPossibleAccelRatio = maxR;};

	    virtual void	EnterFastStart(void) { m_bFastStart = TRUE; };
	    virtual void	LeaveFastStart(TurboPlayOffReason leftReason);
            virtual HXBOOL      IsRateAdaptationUsed(void)  { return FALSE; }

	    virtual HXBOOL	CanBeFastStarted(void);

	    virtual HX_RESULT	FillRecordControl(UINT32 ulLoopEntryTime = 0) = 0;
	    HXBOOL		IsPlayingFromRecordControl() { return m_bPlayFromRecordControl; };
	    HX_RESULT		GetRecordControl(REF(HXRecordControl*) rpControl);

            virtual HX_RESULT OnTimeSync(ULONG32 ulCurrentTime);

            const char*         GetRedirectURL() { return (m_pRedirectURL ? m_pRedirectURL->GetURL() : NULL); };
            const char*         GetSDPURL() { return (m_pSDPURL ? m_pSDPURL->GetURL() : NULL); };

            HXBOOL   IsAcceleratingEventDelivery() const { return (REBUFFER_WARNING == m_rebufferStatus);}
            HX_RESULT   StartAcceleratingEventDelivery();
            void   StopAcceleratingEventDelivery(RebufferStatus newStatus);
            UINT32 CalcAccelDeliveryTime(UINT32 ulDeliveryTime) const;

            UINT32 CalcEventTime(STREAM_INFO* pStreamInfo, UINT32 ulTime, 
                                 HXBOOL bEnableLiveCheck, INT32 lVelocity = HX_PLAYBACK_VELOCITY_NORMAL);
            INT64  CalcActualPacketTime(STREAM_INFO* pStreamInfo, UINT32 ulTime) const;

            void   OnResumed(void);

            virtual HXBOOL IsPacketlessSource() { return FALSE; }

#if defined(HELIX_FEATURE_DRM)

            virtual HXBOOL IsHelixDRMProtected(void) {return m_bIsProtected;}
            virtual HXBOOL IsHelixDRMProtected(IHXValues* pHeader);
            HX_RESULT InitHelixDRM(IHXValues* pHeader);
            HXDRM* GetDRM(void) {return m_pDRM;}

#endif /* HELIX_FEATURE_DRM */

protected:

    virtual 		~HXSource(void);


    virtual HX_RESULT	_ProcessIdle(HXBOOL atInterrupt = FALSE, 
				     UINT32 ulLoopEntryTime = 0, 
				     UINT32 ulProcessingTimeAllowance = 0) = 0;
		    
    virtual HX_RESULT	ReadPreferences(void);

	    HXBOOL	IsAudioStreamFromThisSource(IHXValues* pAudioHeader);
	    HXBOOL	IsAnyAudioStream(void);
	    void	SetStreamHeadersToResumed(HXBOOL bSetResumed);

	    HX_RESULT	DeleteStreamTable(void);
	    IHXValues* GetHeaderInfo(UINT16 stream_number);

	    ULONG32	GetAvailableMemory();
	    ULONG32	GetPerfectPlayTime(void);
	    HX_RESULT	AdjustClipTime(HXBOOL bIsResumingFromPause = FALSE);

	    CHXMapLongToObj* GetStreamInfoTable(void)
			    {
				return mStreamInfoTable;
			    };

    virtual void	ReBuffer(UINT32 ulLoopEntryTime = 0,
				 UINT32 ulProcessingTimeAllowance = 0) = 0;

    virtual HXBOOL        ShouldDisableFastStart(void) { return FALSE; }
            
	    void	GenerateFakeLostPacket(CHXEvent*& theEvent);
	    HXBOOL	ShouldConvert(const char* pTargetInstance);

            HX_RESULT	SendHeaderToRecordControl(HXBOOL bFileHeader, IHXValues* pHeader);
            
    virtual HX_RESULT   ProcessFileHeader(void);
    virtual HX_RESULT   ProcessStreamHeaders(IHXValues* pHeader, STREAM_INFO*& pStreamInfo);

    void SetSrcBufStats(IHXSourceBufferingStats3* pSrcBufStats);
    HXBOOL ShouldRebufferOnOOP(STREAM_INFO* pStreamInfo) const;
    HXBOOL HandleAudioRebuffer();
    void CheckForInitialPrerollRebuffer(UINT32 ulCurrentTime);
    void ChangeRebufferStatus(RebufferStatus newStatus);

    HX_RESULT           IsFaststartPushdownFullfilled(REF(UINT16) uStatusCode, 
						      REF(UINT16) ulPercentDone);
    HXBOOL		IsRARVSource(void);
    HXBOOL                IsRARVStream(IHXValues* pHeader);

    virtual HX_RESULT   GetBufferingStatusFromRenderers(REF(UINT16)     rusStatusCode,
                                                        REF(IHXBuffer*) rpStatusDesc,
                                                        REF(UINT16)     rusPercentDone);
    virtual ULONG32     ComputeMaxLatencyThreshold(ULONG32 ulPrerollInMs, ULONG32 ulPostDecodeDelay);
    inline  HXBOOL	EnforceLiveLowLatency() 
                        {        
#ifdef HELIX_FEATURE_DISABLE_LIVELOWLATENCY
			    return FALSE;
#else
                            // Latency check is only applicable to live, in addition, if 
                            // seek or pause is issued on SuperBuffer by the user, then latency
                            // check will no longer be enforced.
                            return (mLiveStream && !(m_bPlayFromRecordControl && m_bSeekedOrPaused))?TRUE:FALSE;
#endif
                        };


    HX_RESULT			mLastError;
    CHXMapLongToObj*            mStreamInfoTable;
    
    // Preferences
    ULONG32			m_ulPerfectPlayTime;
    ULONG32			m_ulBufferedPlayTime;
    ULONG32		        m_ulStreamHeadersExpected;
    UINT16			m_nSeeking;
    HXBOOL			m_bPerfectPlayEntireClip;
    HX_BITFIELD			m_bCannotBufferEntireClip : 1;
    HXBOOL			m_bPerfectPlay;
    HX_BITFIELD			m_bBufferedPlay : 1;

    HX_BITFIELD			m_bSeekedOrPaused : 1;
    HX_BITFIELD			m_bClipTimeAdjusted : 1;
    HX_BITFIELD			m_bInitialized : 1;
    HX_BITFIELD			m_bIsPreBufferingDone : 1;
    HX_BITFIELD			m_bAltURL : 1;
    HX_BITFIELD			m_bReceivedData : 1;
    HX_BITFIELD			m_bReceivedHeader : 1;

    HX_BITFIELD			m_bInitialBuffering : 1;
    HX_BITFIELD			m_bCustomEndTime : 1;
    HX_BITFIELD			m_bCustomDuration : 1;
    HX_BITFIELD			m_bPaused : 1;
    HX_BITFIELD			m_bFirstResume : 1;

    HX_BITFIELD			m_bResumePending : 1;
    HX_BITFIELD			m_bIsActive : 1;
    HX_BITFIELD			m_bDelayed : 1;
    HX_BITFIELD			m_bLocked : 1;
    
    HX_BITFIELD			m_bPartOfNextGroup : 1;
    HX_BITFIELD			m_bPartOfPrefetchGroup : 1;
    HX_BITFIELD			m_bPrefetch : 1;

    HX_BITFIELD			m_bPerfectPlayAllowed : 1;
    HX_BITFIELD			mSaveAsAllowed : 1;
    HX_BITFIELD			mLiveStream : 1;
    HX_BITFIELD			m_bRestrictedLiveStream : 1;
    HX_BITFIELD			m_bNonSeekable : 1;

    HX_BITFIELD			m_bSourceEnd : 1;		// == 1 no more packets for this stream from actual source
    HX_BITFIELD			m_bForcedSourceEnd : 1;		// Forced end of source due to end or dur tag in url/SMIL
    HX_BITFIELD			m_bIsPreBufferingStarted : 1;
    HX_BITFIELD			m_bIsMeta: 1;
    HX_BITFIELD                 m_bRARVSource : 1;
    HX_BITFIELD	                m_bSeekInsideRecordControl : 1;
    PushDownStatus              m_pushDownStatus;
    UINT32			m_ulTurboStartActiveTime;
    UINT16			mFlags;
    UINT16			m_uNumStreams;			// Number of streams in the source...
    ULONG32			m_ulPreRollInMs;		// Number of millisecs worth of data to be held back (for preroll)
    ULONG32			m_ulPreRoll;			// Number of bytes worth of data to be held back (for preroll)    
    ULONG32                     m_ulMaxLatencyThreshold;        // Maximum latency allowed before a seek is issued
    ULONG32			m_ulAvgBandwidth;		// Bandwidth for this stream...
    ULONG32			m_ulDuration;			// duration of this stream...
    
    UINT32			m_ulLastBufferingCalcTime;

    UINT16			m_uActiveStreams;

    PrefetchType		m_prefetchType;
    UINT32			m_ulPrefetchValue;

    char*			m_pszURL;
    CHXURL*			m_pURL;

    HXBOOL                        m_bRebufferAudio;
    UINT32                      m_ulAcceleratedRebufferBaseTime;
    UINT32                      m_ulAcceleratedRebufferFactor;
    UINT32			m_ulStartTime;
    UINT32			m_ulEndTime;    
    INT64			m_llLastExpectedPacketTime;
    UINT32			m_ulRestrictedDuration;
    UINT32			m_ulLossHack;
    UINT32			m_ulNumFakeLostPackets;
    UINT32			m_ulFirstPacketTime;
    double                      m_maxPossibleAccelRatio;
#if defined(HELIX_FEATURE_STATS)
    UINT32                      m_ulLivePlaybackIntervalPacketCount;
    UINT64                      m_ullLivePlaybackIntervalCumulativeLatency;
    UINT32                      m_ulLivePlaybackIntervalMinimumLatency;
    UINT32                      m_ulLivePlaybackIntervalMaximumLatency;
#endif /* defined(HELIX_FEATURE_STATS) */
    UINT32                      m_ulLiveSyncOffset;
    UINT32                      m_ulLastReportedPlayTime;
    UINT32                      m_ulLastLatencyCalculation; // Last latency computation made in
                                                            // NewPacketTimeStamp() call. This
                                                            // value is cached here because
                                                            // we can't take action in that
                                                            // call without causing a deadlock

    // level 3 stats
    UINT32			m_ulSourceStartTime;

    IHXPreferences*		m_pPreferences;
    IHXScheduler*		m_pScheduler;
    IHXRequest*		m_pRequest;    

    HXClientEngine*		m_pEngine;

    CHXSimpleList		m_HXStreamList;
    CHXSimpleList*		m_pAudioStreamList;
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    HXUpgradeCollection*	m_pUpgradeCollection;
#endif /* HELIX_FEATURE_AUTOUPGRADE */

    IHXASMSource*		m_pASMSource;
    IHXBackChannel*		m_pBackChannel;

    HXRecordControl*		m_pRecordControl;
    HXBOOL			m_bPlayFromRecordControl;
    UINT32			m_ulRenderingDisabledMask;

    CHXURL*                     m_pRedirectURL;
    CHXURL*                     m_pSDPURL;
    HXBOOL                        m_bRedirectPending;
    IHXTransportTimeSink*       m_pTransportTimeSink;
    IHXSourceBufferingStats3*   m_pSrcBufStats;
    INT32                       m_lRAStreamNumber;
    UINT16                      m_uLastBuffering;
    UINT16                      m_uLastStatusCode;
    ULONG32                     m_ulStartDataWait;
    ULONG32                     m_ulFirstDataArrives;
    INT16			m_nSoundLevelOffset;
    UINT32                      m_ulKeyFrameInterval;
    INT32                       m_lPlaybackVelocity;
    HXBOOL                      m_bKeyFrameMode;
    HXBOOL                      m_bAutoSwitch;
    IHXPlaybackVelocityResponse* m_pPlaybackVelocityResponse;
    IHXPlaybackVelocity*        m_pPlaybackVelocity;

#if defined(HELIX_FEATURE_DRM)
    HXBOOL                      m_bIsProtected;
    HXDRM*                      m_pDRM;
#endif /* HELIX_FEATURE_DRM */
};

// Defined flags
#define HX_RELIABLE_FLAG			0x0001
#define HX_KEYFRAME_FLAG			0x0002
#define HX_LASTFRAME_FLAG			0x0004


#endif // _HX_BSOURCE


