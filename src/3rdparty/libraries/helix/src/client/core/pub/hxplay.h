/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxplay.h,v 1.69 2009/05/05 17:09:14 sfu Exp $
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

#ifndef _HXPLAYER_
#define _HXPLAYER_

#include "hxtypes.h"
#include "hxcomm.h"		// IHXRegistryID
#include "hxfiles.h"
#include "hxsmbw.h"
#include "hxresult.h"
#include "hxslist.h"
#include "chxelst.h"
#include "hxmap.h"
#include "chxpckts.h"
#include "hxurl.h"
#include "hxtick.h"
#include "hxmon.h"
#include "hxclreg.h"
#include "hxclsnk.h"
#include "statinfo.h"
#include "strminfo.h"
#include "hxauth.h"
#include "hxpreftr.h"
#include "hxvsrc.h"
#include "hxplayvelocity.h"

//#include "prefkeys.h"
#include "plprefk.h"
//#include "domain.h" 		// Needed for domain object

#include "timeval.h"
#include "hxausvc.h"
#include "hxpends.h"
#include "hxerror.h"
#include "hxmeta.h"
#include "chxphook.h"
#include "hxwin.h"
#include "hxcorgui.h"
#include "smartptr.h"
#include "miscsp.h"
#include "hxrquest.h"
#include "perscmgr.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxcbobj.h"
#include "chxmetainfo.h"
#include "hxembeddedui.h"
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
#include "hxpfs.h"
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.
#if defined(HELIX_FEATURE_VIDEO_FRAME_STEP)
#include "hxframestep.h"
#endif

#ifdef _SYMBIAN
//XXXgfw Wow, symbian's unistd.h #defines remove to be unlink.
//Very uncool.
#undef remove
#endif

#include "hxcleng.h"


// forward decl.
_INTERFACE IHXPlayer;
_INTERFACE IHXPlayer2;
_INTERFACE IHXAudioPlayerResponse;
_INTERFACE IHXClientEngine;
_INTERFACE IHXRenderer;
_INTERFACE IHXScheduler;
_INTERFACE IHXClientAdviseSink;
_INTERFACE IHXMetaTrack;
_INTERFACE IHXMetaLayout;
_INTERFACE IHXErrorMessages;
_INTERFACE IHXPreferences;
_INTERFACE IHXHyperNavigate;
_INTERFACE IHXAuthenticationManager;
_INTERFACE IHXSiteSupplier;
_INTERFACE IHXSiteLayout;
_INTERFACE IHXUpgradeCollection;
_INTERFACE IHXGroupSink;
_INTERFACE IHXGroup;
_INTERFACE IHXPlayerState;
_INTERFACE IHXClientState;
_INTERFACE IHXClientStateAdviseSinkControl;
_INTERFACE IHXProxyManager;
_INTERFACE IHXViewSourceCommand;
_INTERFACE IHXViewPortManager;
#if defined(HELIX_FEATURE_MEDIAMARKER)
_INTERFACE IHXMediaMarkerManager;
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
#if defined(HELIX_FEATURE_EVENTMANAGER)
_INTERFACE IHXEventManager;
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
_INTERFACE IHXPresentationFeatureManager;
_INTERFACE IHXPresentationFeatureSink;
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.
_INTERFACE IHXPlayerNavigator;
_INTERFACE IHXClientRequestSink;
_INTERFACE IHXCookies3;

class CHXSiteManager;

class CHXAudioPlayer;
class HXClientEngine;
class HXSource;
class HXFileSource;
class HXNetSource;
class HXStream;

class HXPlayerCallback;
class UpdateStatsCallback;
class CHXAdviseSinkControl;
class CHXClientStateAdviseSink;
class CHXErrorSinkControl;
class CHXErrorSinkTranslator;

class Timeline;
class Timeval;
class HXMutex;

class Plugin2Handler;
struct IHXPlugin2Handler;


class HXPersistentComponentManager;
class HXPreferredTransportManager;

#if defined(HELIX_FEATURE_EVENTMANAGER)
class CPresentationFeatureEventProxy;
#endif //  End #if defined(HELIX_FEATURE_EVENTMANAGER).

#ifdef _UNIX
_INTERFACE IHXSiteEventHandler;
#endif

struct STREAM_INFO;

typedef ULONG32	BufferingReason;

struct RendererInfo;
class  SourceInfo;
class HXPlayer;
class UpdateStatsCallback;
class HXPlayerCallback;
class HXBasicGroupManager;
class HXAdvancedGroupManager;
class NextGroupManager;
class PrefetchManager;
class HXMasterTAC;
class PlayerHyperNavigate;

struct IHXBandwidthManager;

class CHXEvent;
class CHXSimpleList;
class CHXMapStringToOb;
class HXViewPortManager;
#if defined(HELIX_FEATURE_MEDIAMARKER)
class CHXMediaMarkerManager;
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
#if defined(HELIX_FEATURE_EVENTMANAGER)
class CRendererEventManager;
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */

#define DEFAULT_TIMESYNC_GRANULARITY	100
/* Lowest allowable time sync granularity */
#define MINIMUM_TIMESYNC_GRANULARITY	20

#ifndef _WIN16
//#if defined(HELIX_FEATURE_AUTHENTICATION)
typedef WRAPPED_POINTER(IUnknown) Wrapped_IUnknown;

class _CListOfWrapped_IUnknown_Node
{
#ifdef HELIX_FEATURE_AUTHENTICATION
public:
    _CListOfWrapped_IUnknown_Node();
    ~_CListOfWrapped_IUnknown_Node();

    Wrapped_IUnknown& value();
    const Wrapped_IUnknown& value() const;
    void value(const Wrapped_IUnknown& rclsNewValue);
    _CListOfWrapped_IUnknown_Node& operator=(const Wrapped_IUnknown& rclsNewValue); 
    _CListOfWrapped_IUnknown_Node* next() const;
    void next(_CListOfWrapped_IUnknown_Node* plocnNew);
    _CListOfWrapped_IUnknown_Node* prev() const;
    void prev(_CListOfWrapped_IUnknown_Node* plocnNew);
    void Remove();
    void Insert(_CListOfWrapped_IUnknown_Node& rlocnNew);
						
protected:
    Wrapped_IUnknown m_clsValue;
    _CListOfWrapped_IUnknown_Node* m_plocPrev;
    _CListOfWrapped_IUnknown_Node* m_plocNext;
    _CListOfWrapped_IUnknown_Node(const _CListOfWrapped_IUnknown_Node& rlocnOther){}
#endif /* HELIX_FEATURE_AUTHENTICATION */
};

class _CListIteratorWrapped_IUnknown_;
class _CListReverseIteratorWrapped_IUnknown_;

class _CListOfWrapped_IUnknown_
{
public:
    typedef _CListIteratorWrapped_IUnknown_ iterator;
    typedef _CListReverseIteratorWrapped_IUnknown_ reverse_iterator;
    typedef const _CListReverseIteratorWrapped_IUnknown_ const_reverse_iterator;

#ifdef HELIX_FEATURE_AUTHENTICATION
    _CListOfWrapped_IUnknown_();
    _CListOfWrapped_IUnknown_(const _CListOfWrapped_IUnknown_& rlocOther);
    ~_CListOfWrapped_IUnknown_();
    _CListOfWrapped_IUnknown_& operator=(const _CListOfWrapped_IUnknown_& rlocOther); 

    iterator begin();
    const iterator begin() const;
    iterator end();
    const iterator end() const;

    reverse_iterator rbegin();
    const reverse_iterator rbegin() const;
    reverse_iterator rend();
    const reverse_iterator rend() const;

    iterator insert(iterator itBefore, const Wrapped_IUnknown&);
    void insert
    (
	iterator itBefore,
	const iterator itFirst,
	const iterator itLast
    );
    void remove(iterator itThis);
    void remove(iterator itFirst, iterator itLast);

    void empty();

protected:
    _CListOfWrapped_IUnknown_Node m_locnREnd;
    _CListOfWrapped_IUnknown_Node m_locnEnd;

    void _copy(const _CListOfWrapped_IUnknown_& rlocOther);
#endif /* HELIX_FEATURE_AUTHENTICATION */
};

class _CListIteratorWrapped_IUnknown_
{
#ifdef HELIX_FEATURE_AUTHENTICATION
public:
    _CListIteratorWrapped_IUnknown_();
    _CListIteratorWrapped_IUnknown_
    (
        const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
    );
    _CListIteratorWrapped_IUnknown_(const _CListIteratorWrapped_IUnknown_& rliocOther);
    ~_CListIteratorWrapped_IUnknown_();

    _CListIteratorWrapped_IUnknown_& operator=
    (
	const _CListIteratorWrapped_IUnknown_& rliocOther
    );

    Wrapped_IUnknown& operator*();
    _CListIteratorWrapped_IUnknown_& operator=(const Wrapped_IUnknown& rclsNewValue);

    _CListIteratorWrapped_IUnknown_& operator++();
    const _CListIteratorWrapped_IUnknown_ operator++(int);

    _CListIteratorWrapped_IUnknown_& operator--();
    const _CListIteratorWrapped_IUnknown_ operator--(int);

private:
    _CListOfWrapped_IUnknown_Node* m_plocCurrent;

    friend class _CListOfWrapped_IUnknown_;
    friend HXBOOL operator==
    (
	const _CListIteratorWrapped_IUnknown_& rliocLeft,
	const _CListIteratorWrapped_IUnknown_& rliocRight
    );
    friend HXBOOL operator!=
    (
	const _CListIteratorWrapped_IUnknown_& rliocLeft,
	const _CListIteratorWrapped_IUnknown_& rliocRight
    );
#endif /* HELIX_FEATURE_AUTHENTICATION */
};

#ifdef HELIX_FEATURE_AUTHENTICATION
HXBOOL operator==
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
);

HXBOOL operator!=
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
);
#endif /* HELIX_FEATURE_AUTHENTICATION */

class _CListReverseIteratorWrapped_IUnknown_
{
#ifdef HELIX_FEATURE_AUTHENTICATION
public:
    _CListReverseIteratorWrapped_IUnknown_();
    _CListReverseIteratorWrapped_IUnknown_
    (
        const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
    );
    _CListReverseIteratorWrapped_IUnknown_
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
    );
    ~_CListReverseIteratorWrapped_IUnknown_();

    _CListReverseIteratorWrapped_IUnknown_& operator=
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
    );

    Wrapped_IUnknown& operator*();
    _CListReverseIteratorWrapped_IUnknown_& operator=(const Wrapped_IUnknown& rclsNewValue);

    _CListReverseIteratorWrapped_IUnknown_& operator++();
    const _CListReverseIteratorWrapped_IUnknown_ operator++(int);
    _CListReverseIteratorWrapped_IUnknown_& operator--();
    const _CListReverseIteratorWrapped_IUnknown_ operator--(int);

private:
    _CListOfWrapped_IUnknown_Node* m_plocCurrent;
    friend class _CListOfWrapped_IUnknown_;
    friend HXBOOL operator==
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
	const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
    );
    friend HXBOOL operator!=
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
	const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
    );
#endif /* HELIX_FEATURE_AUTHENTICATION */
};

#ifdef HELIX_FEATURE_AUTHENTICATION
HXBOOL operator==
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
);
HXBOOL operator!=
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
);								    
#endif /* HELIX_FEATURE_AUTHENTICATION */

class _CHXAuthenticationRequests
{
public:
    _CHXAuthenticationRequests(IUnknown* pContext);
    ~_CHXAuthenticationRequests();

    HX_RESULT Add
    (
	HXPlayer* pPlayerRequester, 
	IHXAuthenticationManagerResponse* pAuthenticationManagerResponseRequester,
	IHXValues* pAuthenticationHeaderValues
    );
    HX_RESULT SatisfyPending
    (
	HX_RESULT ResultStatus, 
	const char* pCharUser, 
	const char* pCharPassword
    );
    void     ClearPendingList();
private:
    _CListOfWrapped_IUnknown_ m_ListOfIUnknownRequesters;
    IHXMutex* m_pMutexProtectList;
    HXBOOL m_bUIShowing;
};
//#endif /* HELIX_FEATURE_AUTHENTICATION */

struct PendingTrackInfo
{
    PendingTrackInfo(UINT16	    uGroupIndex, 
		     UINT16	    uTrackIndex,
		     IHXValues*    pTrack)
    {
	m_uGroupIndex	= uGroupIndex;
	m_uTrackIndex	= uTrackIndex;
	m_pTrack	= pTrack;
	m_pTrack->AddRef();
    }

    ~PendingTrackInfo()
    {
	HX_RELEASE(m_pTrack);
    }

    UINT16 	  m_uGroupIndex;
    UINT16 	  m_uTrackIndex;
    IHXValues*   m_pTrack;
};
#endif /* _WIN16 */

typedef enum
{
    URL_ALTERNATE = 0,
    URL_SDP,
    URL_REDIRECTED,
    URL_OPPOSITE_HXSOURCE
} URL_TYPE;

#define PLAYER_SCHEDULE_IMMEDIATE	0x01
#define PLAYER_SCHEDULE_MAIN		0x02
#define PLAYER_SCHEDULE_INTERRUPT_SAFE	0x04
#define PLAYER_SCHEDULE_INTERRUPT_ONLY	0x08
#define PLAYER_SCHEDULE_RESET		0x10
#define PLAYER_SCHEDULE_DEFAULT		(PLAYER_SCHEDULE_MAIN | PLAYER_SCHEDULE_INTERRUPT_SAFE)

const UINT32 kNoValue = MAX_UINT32;

class HXPlayer :   public IHXPlayer, 
                   public IHXQuickSeek,
		    public IHXPlayer2,
		    public IHXAudioPlayerResponse,
		    public IHXPendingStatus,
		    public IHXErrorMessages,
		    public IHXAuthenticationManager,
		    public IHXAuthenticationManager2,
		    public IHXAuthenticationManagerResponse,
		    public IHXRegistryID,
		    public IHXGroupSink,
		    public IHXLayoutSiteGroupManager,
		    public IHXRendererUpgrade,
		    public IHXInternalReset,
		    public IHXPlayerState,
		    public IHXClientState,
		    public IHXClientStateAdviseSinkControl,
		    public IHXViewSourceCommand,
		    public IHXOverrideDefaultServices,
		    public IHXPlayerNavigator,
		    public IHXClientStatisticsGranularity,
		    public IHXPlayerPresentation,
                    public IHXRecordManager,
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
                    public IHXPDStatusMgr,
                    public IHXPDStatusObserver,
#endif // HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
                    public IHXPresentationFeatureManager,
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.
#if defined(HELIX_FEATURE_PLAYBACK_MODIFIER)
                    public IHXPlaybackModifier,
#endif // HELIX_FEATURE_PLAYBACK_MODIFIER
                    public IHXPlaybackVelocity,
                    public IHXPlaybackVelocityResponse
#if defined(HELIX_FEATURE_VIDEO_FRAME_STEP)
					,public IHXFrameStep
#endif
{
protected:

    LONG32			m_lRefCount;
    UINT32			m_ulRepeatedRegistryID;
    UINT32			m_ulNextGroupRegistryID;
    HX_RESULT			m_LastError;
    char*			m_pLastUserString;
    UINT8			m_LastSeverity;
    UINT32			m_ulLastUserCode;
    char*			m_pLastMoreInfoURL;
    
#if defined(HELIX_FEATURE_REGISTRY)    
    // statistic variables
    HXClientRegistry*		m_pRegistry;
#else
    void*			m_pRegistry;
#endif /* HELIX_FEATURE_REGISTRY */

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    PLAYER_STATS*		m_pStats;
    UpdateStatsCallback*	m_pUpdateStatsCallback;
#else
    void*			m_pStats;
    void*			m_pUpdateStatsCallback;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    CHXGenericCallback*		m_pHXPlayerCallback;
    CHXGenericCallback*		m_pHXPlayerInterruptCallback;
    CHXGenericCallback*		m_pHXPlayerInterruptOnlyCallback;
#if defined(HELIX_FEATURE_AUTHENTICATION)
    CHXGenericCallback*		m_pAuthenticationCallback;
#else
    void*			m_pAuthenticationCallback;
#endif /* HELIX_FEATURE_AUTHENTICATION */
    IHXValues*			m_pAutheticationValues;

    ULONG32			m_ulStatsGranularity;

public:
    HXClientEngine*		m_pEngine;
    IHXPlugin2Handler*		m_pPlugin2Handler;
    IHXPreferences*		m_pPreferences;
protected:
    IUnknown*			m_pClient;
    CHXAudioPlayer*		m_pAudioPlayer;
    
    CHXAdviseSinkControl*	m_pAdviseSink;
    CHXClientStateAdviseSink*	m_pClientStateAdviseSink;
    CHXErrorSinkControl*  	m_pErrorSinkControl;
#if defined(HELIX_FEATURE_SINKCONTROL) && defined(HELIX_FEATURE_LOGGING_TRANSLATOR)
    CHXErrorSinkTranslator* m_pErrorSinkTranslator;
#endif /* #if defined(HELIX_FEATURE_SINKCONTROL) && defined(HELIX_FEATURE_LOGGING_TRANSLATOR) */
    
    IHXClientRequestSink*	m_pClientRequestSink;
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    PlayerHyperNavigate*	m_pHyperNavigate;
#else
    void*			m_pHyperNavigate;
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */
#if defined(HELIX_FEATURE_PACKETHOOKMGR)
    IHXPacketHookManager*	m_pPacketHookManager;
#else
    void*			m_pPacketHookManager;
#endif /* HELIX_FEATURE_PACKETHOOKMGR */

#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    HXAdvancedGroupManager*     m_pGroupManager;
#else
    HXBasicGroupManager*	m_pGroupManager;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */

    CHXMetaInfo*                m_pMetaInfo;
    CHXEmbeddedUI*              m_pEmbeddedUI;
    HXMasterTAC*		m_pMasterTAC;

    CHXSimpleList*		m_pRedirectList;
    CHXSimpleList*              m_pSDPURLList;

    IHXClientViewSource*	m_pClientViewSource;
    IHXClientViewRights*	m_pClientViewRights;
    IHXViewPortManager*	m_pViewPortManager;
#if defined(HELIX_FEATURE_MEDIAMARKER)
    CHXMediaMarkerManager*     m_pMediaMarkerManager;
#else
    void*			m_pMediaMarkerManager;
#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
#if defined(HELIX_FEATURE_EVENTMANAGER)
    CRendererEventManager*      m_pEventManager;
#else
    void*			m_pEventManager;
#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */
    IHXCookies3*		m_pCookies3;

#if defined(HELIX_FEATURE_NESTEDMETA)
    HXPersistentComponentManager*  m_pPersistentComponentManager;
#else
    void*			m_pPersistentComponentManager;
#endif /* HELIX_FEATURE_NESTEDMETA */
    HXPreferredTransportManager*   m_pPreferredTransportManager;
    IHXNetInterfaces*		    m_pNetInterfaces;

public:
			HXPlayer();
			~HXPlayer();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXPlayer methods
     */

    /************************************************************************
     *	Method:
     *		IHXPlayer::GetClientEngine
     *	Purpose:
     *		Get the interface to the client engine object of which the player
     *		is a part of.
     *
     */
    STDMETHOD(GetClientEngine)	(THIS_
				    REF(IHXClientEngine*)	pEngine);

    /************************************************************************
     *	Method:
     *		IHXPlayer::OpenURL
     *	Purpose:
     *		Tell the player to begin playback of all its sources.
     *
     */
    STDMETHOD (OpenURL)		(THIS_
				    const char*	pURL);

    /************************************************************************
     *	Method:
     *		IHXPlayer::Begin
     *	Purpose:
     *		Tell the player to begin playback of all its sources.
     *
     */
    STDMETHOD (Begin)		(THIS);

    /************************************************************************
     *	Method:
     *		IHXPlayer::Stop
     *	Purpose:
     *		Tell the player to stop playback of all its sources.
     *
     */
    STDMETHOD (Stop)		(THIS);

    /************************************************************************
     *	Method:
     *		IHXPlayer::Pause
     *	Purpose:
     *		Tell the player to pause playback of all its sources.
     *
     */
    STDMETHOD (Pause)		(THIS);

    /************************************************************************
     *	Method:
     *		IHXPlayer::Seek
     *	Purpose:
     *		Tell the player to seek in the playback timeline of all its 
     *		sources.
     *
     */
    STDMETHOD (Seek)		(THIS_
				ULONG32			ulTime);

    /************************************************************************
     *	Method:
     *	    IHXPlayer::GetSourceCount
     *	Purpose:
     *	    Returns the current number of source instances supported by
     *	    this player instance.
     */
    STDMETHOD_(UINT16, GetSourceCount)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXPlayer::GetSource
     *	Purpose:
     *	    Returns the Nth source instance supported by this player.
     */
    STDMETHOD(GetSource)	(THIS_
				UINT16		nIndex,
				REF(IUnknown*)	pUnknown);

    /************************************************************************
     *	Method:
     *	    IHXPlayer::SetClientContext
     *	Purpose:
     *	    Called by the client to install itself as the provider of client
     *	    services to the core. This is traditionally called by the top 
     *	    level client application.
     */
    STDMETHOD(SetClientContext)	(THIS_
				IUnknown* pUnknown);

    /************************************************************************
     *	Method:
     *	    IHXPlayer::GetClientContext
     *	Purpose:
     *	    Called by the get the client context for this player. This is 
     *	    traditionally to determine called by top level client application.
     */
    STDMETHOD(GetClientContext)	(THIS_
				REF(IUnknown*) pUnknown);

    /************************************************************************
     *	Method:
     *		IHXPlayer::IsDone
     *	Purpose:
     *		Ask the player if it is done with the current presentation
     *
     */
    STDMETHOD_ (HXBOOL,IsDone)	(THIS);

    /************************************************************************
     *	Method:
     *		IHXPlayer::IsLive
     *	Purpose:
     *		Ask the player whether it contains the live source
     *
     */
    STDMETHOD_ (HXBOOL,IsLive)	(THIS);

    /************************************************************************
     *	Method:
     *		IHXPlayer::GetCurrentPlayTime
     *	Purpose:
     *		Get the current time on the Player timeline
     *
     */
    STDMETHOD_ (ULONG32,GetCurrentPlayTime)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXPlayer::AddAdviseSink
     *	Purpose:
     *	    Call this method to add a client advise sink.
     *
     */
    STDMETHOD(AddAdviseSink)	(THIS_
				IHXClientAdviseSink*	pAdviseSink);

    /************************************************************************
     *	Method:
     *	    IHXPlayer::RemoveAdviseSink
     *	Purpose:
     *	    Call this method to remove a client advise sink.
     */
    STDMETHOD(RemoveAdviseSink)	(THIS_
				IHXClientAdviseSink*	pAdviseSink);

    /************************************************************************
     *	Method:
     *	    IHXClientStateAdviseSinkControl::AddClientStateAdviseSink
     *	Purpose:
     *	    Call this method to add a client advise sink.
     *
     */
    STDMETHOD(AddClientStateAdviseSink)	(THIS_
				IHXClientStateAdviseSink*	pClientStateAdviseSink);

    /************************************************************************
     *	Method:
     *	    IHXClientStateAdviseSinkControl::RemoveClientStateAdviseSink
     *	Purpose:
     *	    Call this method to remove a client advise sink.
     */
    STDMETHOD(RemoveClientStateAdviseSink)	(THIS_
				IHXClientStateAdviseSink*	pClientStateAdviseSink);

    /************************************************************************
     *	Method:
     *	    IHXPlayer2::SetMinimumPreroll
     *	Purpose:
     *	    Call this method to set the minimum preroll of this clip
     */
    STDMETHOD(SetMinimumPreroll) (THIS_
				UINT32	ulMinPreroll);

    /************************************************************************
     *	Method:
     *	    IHXPlayer2::GetMinimumPreroll
     *	Purpose:
     *	    Call this method to get the minimum preroll of this clip
     */
    STDMETHOD(GetMinimumPreroll) (THIS_
				REF(UINT32) ulMinPreroll);

    /************************************************************************
     *	Method:
     *	    IHXPlayer2::OpenRequest
     *	Purpose:
     *	    Call this method to open the IHXRequest
     */
    STDMETHOD(OpenRequest) (THIS_
			    IHXRequest* pRequest);
    
    /************************************************************************
     *	Method:
     *	    IID_IHXPlayer2::GetRequest
     *	Purpose:
     *	    Call this method to get the IHXRequest
     */
    STDMETHOD(GetRequest) (THIS_
			   REF(IHXRequest*) pRequest);

    /************************************************************************
     *	Method:
     *	    EventOccurred
     *	Purpose:
     *	    Clients call this to pass OS events to the player. HXxEvent
     *	    defines a cross-platform event.
     */
    void EventOccurred(HXxEvent* pEvent);


    //IHXQuickSeek call
    STDMETHOD(QuickSeek) (THIS_ ULONG32 ulSeekTime );
    

#ifdef _UNIX
    void CollectSelectInfo(INT32 *n, 
			   fd_set* readfds,
			   fd_set* writefds, 
			   fd_set* exceptfds,
			   struct timeval* tv);
    void ProcessSelect(INT32* n,
		       fd_set* readfds,
		       fd_set* writefds, 
		       fd_set* exceptfds,
		       struct timeval* tv);
#endif

    /************************************************************************
     *	Method:
     *		GetInst
     *	Purpose:
     *		TBD
     *
     */
    ULONG32	GetInst(void);

    /************************************************************************
     *	Method:
     *		UpdateStatistics
     *	Purpose:
     *		Update statistics periodically
     *
     */
    HX_RESULT	UpdateStatistics(void);

    /************************************************************************
     *	Method:
     *		ProcessIdle
     *	Purpose:
     *		Work-Horse for the player...
     *
     */
    HX_RESULT	ProcessIdle(HXBOOL bFromInterruptSafeChain);

    /************************************************************************
     *  Method:
     *      IHXAudioPlayerResponse::OnTimeSync
     *  Purpose:
     *      Notification interface provided by users of the IHXAudioPlayer 
     *      interface. This method is called by the IHXAudioPlayer when 
     *      audio playback occurs.
     */
    STDMETHOD(OnTimeSync)       (THIS_
				ULONG32			/*IN*/ ulTimeEnd);


    /************************************************************************
     *	Method:
     *		HXPlayer::Init
     *	Purpose:
     *		Get the interface to the client engine object of which the player
     *		is a part of. It is not a part of IHXPlayer interface
     *
     */
    STDMETHOD(Init)		(THIS_
				 IHXClientEngine*	pEngine,
				 UINT32			unRegistryID,
				 CHXAudioPlayer*	pAudioPlayer);

    /************************************************************************
     *	Method:
     *		HXPlayer::ReportError
     *	Purpose:
     *		The source object reports of any fatal errors. 
     *
     */
    void	ReportError (HXSource* pSource, 
			HX_RESULT theErr, const char* pUserString = NULL);

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
				REF(UINT16) ulPercentDone);

    /************************************************************************
     *	Method:
     *	    HXPlayer::SetStatsGranularity
     *	Purpose:
     *	    Called by the user to set how often the statistics registry needs 
     *	    to be updated in ms
     */
    STDMETHOD(SetStatsGranularity)	(THIS_
					ULONG32	ulGranularity);

    /************************************************************************
     *	Method:
     *	    IHXPlayerPresentation::ClosePresentation
     *	Purpose:
     *	    Call this method to close the player's current presentation.  This will free
     *	    all resources associated with the current presentation.
     */
    STDMETHOD(ClosePresentation)    (THIS);

    /*
     * IHXErrorMessages methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorMessages::Report
     *	Purpose:
     *	    Call this method to report an error, event, or status message.
     */
    STDMETHOD(Report)		(THIS_
				const UINT8	unSeverity,  
				HX_RESULT	ulHXCode,
				const ULONG32	ulUserCode,
				const char*	pUserString,
				const char*	pMoreInfoURL
				);

    /************************************************************************
     *	Method:
     *	    IHXErrorMessages::GetErrorText
     *	Purpose:
     *	    Call this method to get the text description of a RMA error code.
     *	Parameters:
     *	    HX_RESULT	ulHXCode
     *      A RMA error code.
     *  Return Value:
     *	    IHXBuffer* containing error text.
     */
    STDMETHOD_(IHXBuffer*, GetErrorText)	(THIS_
						HX_RESULT	ulHXCode
						);

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
				REF(UINT32)	/*OUT*/  ulRegistryID);

    STDMETHOD(HandleAuthenticationRequest) 
                                     (IHXAuthenticationManagerResponse*);
    STDMETHOD(HandleAuthenticationRequest2)
    				    (IHXAuthenticationManagerResponse*, IHXValues*);

    // IHXAuthenticationManagerResponse
    STDMETHOD(AuthenticationRequestDone)
    (
	HX_RESULT HX_RESULTStatus,
	const char* pcharUser,
	const char* pcharPassword
    );
    
    /*
     *  IHXGroupSink methods
     */
    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupAdded
    *  Purpose:
    *		Notification of a new group being added to the presentation.
    */
    STDMETHOD(GroupAdded)    (THIS_
			    UINT16 	    /*IN*/ uGroupIndex,
			    IHXGroup*	    /*IN*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupRemoved
    *  Purpose:
    *		Notification of a group being removed from the presentation.
    */
    STDMETHOD(GroupRemoved)    (THIS_
				UINT16 	    /*IN*/ uGroupIndex,
				IHXGroup*  /*IN*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::AllGroupsRemoved
    *  Purpose:
    *		Notification that all groups have been removed from the 
    *		current presentation.
    */
    STDMETHOD(AllGroupsRemoved)  (THIS);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackAdded
    *  Purpose:
    *		Notification of a new track being added to a group.
    */
    STDMETHOD(TrackAdded)  (THIS_
			    UINT16 	    /*IN*/ uGroupIndex,
			    UINT16 	    /*IN*/ uTrackIndex,
			    IHXValues*	    /*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackAdded
    *  Purpose:
    *		Notification of a track being removed from a group.
    */
    STDMETHOD(TrackRemoved)    (THIS_
				UINT16 		/*IN*/ uGroupIndex,
				UINT16 		/*IN*/ uTrackIndex,
				IHXValues*	/*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStarted
    *  Purpose:
    *		Notification of a track being started in a group.
    */
    STDMETHOD(TrackStarted)    (THIS_
				UINT16 		/*IN*/ uGroupIndex,
				UINT16 		/*IN*/ uTrackIndex,
				IHXValues*	/*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStopped
    *  Purpose:
    *		Notification of a track being stopped in a group.
    */
    STDMETHOD(TrackStopped)    (THIS_
				UINT16 		/*IN*/ uGroupIndex,
				UINT16 		/*IN*/ uTrackIndex,
				IHXValues*	/*IN*/ pTrack);

   /************************************************************************
    *  Method:
    *      IHXGroupSink::CurrentGroupSet
    *  Purpose:
    *		This group is being currently played in the presentation.
    */
    STDMETHOD(CurrentGroupSet)	(THIS_
				UINT16 	    /*IN*/ uGroupIndex,
				IHXGroup*  /*IN*/ pGroup);
           
    /*
     * IHXLayoutSiteGroupManager methods
     */
    /************************************************************************
    *  Method:
    *      IHXLayoutSiteGroupManager::AddLayoutSiteGroup
    *  Purpose:
    *		Add this LSG to the presentation
    */
    STDMETHOD(AddLayoutSiteGroup)	(THIS_
					IUnknown*  /*IN*/ pLSG);

    /************************************************************************
    *  Method:
    *      IHXLayoutSiteGroupManager::RemoveLayoutSiteGroup
    *  Purpose:
    *		Remove this LSG from the presentation
    */
    STDMETHOD(RemoveLayoutSiteGroup)	(THIS_
					IUnknown*  /*IN*/ pLSG);

    /*
     * IHXRendererUpgrade methods
     */
    /************************************************************************
    *  Method:
    *      IHXRendererUpgrade::IsRendererAvailable
    *  Purpose:
    *	   Is a renderer with this mime type already loaded?
    */
    STDMETHOD_(HXBOOL,IsRendererAvailable)(THIS_
					const char* /*IN*/ pMimeType);

    /************************************************************************
    *  Method:
    *      IHXRendererUpgrade::ForceUpgrade
    *  Purpose:
    *	   Force an upgrade of all renderers in list
    */
    STDMETHOD(ForceUpgrade)		(THIS);

    /*
     * IHXInternalReset method
     */
    STDMETHOD(InternalReset) (THIS);

    /*
     *	IHXPlayerState methods
     */
    STDMETHOD_(HXBOOL, IsPlaying)    (THIS) {return m_bIsPlaying;}

    /************************************************************************
     *	IHXClientState methods
     */
    STDMETHOD(SetConfig)	(THIS_
		    		IHXValues* pValues);
    STDMETHOD(Resume)		(THIS);
    STDMETHOD_(UINT16, GetState) (THIS);

    /************************************************************************
     *  IHXViewSourceCommand methods
     */
    STDMETHOD_(HXBOOL, CanViewSource)	(THIS_  
					IHXStreamSource* pStream);
    STDMETHOD(DoViewSource)		(THIS_
					IHXStreamSource* pStream);
    STDMETHOD(GetViewSourceURL)		(THIS_
					IHXStreamSource*	    pSource,
					IHXViewSourceURLResponse*  pResp);

    /*
     * IHXOverrideDefaultServices methods
     */

    /************************************************************************
     *  Method:
     *      IHXOverrideDefaultServices::OverrideServices
     *  Purpose:
     *      Override default services provided by the G2 system.
     * 
     */
    STDMETHOD(OverrideServices)         (THIS_
				IUnknown* pContext);

    /*
     * IHXPlayerNavigator methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::AddChildPlayer
     *	Purpose:
     *	    Add child player to the current player
     */
    STDMETHOD(AddChildPlayer)	    (THIS_
				    IHXPlayer* pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::RemoveChildPlayer
     *	Purpose:
     *	    Remove child player from the current player
     */
    STDMETHOD(RemoveChildPlayer)    (THIS_
				    IHXPlayer* pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::GetNumChildPlayer
     *	Purpose:
     *	    Get number of the child players
     */
    STDMETHOD_(UINT16, GetNumChildPlayer)    (THIS);

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::GetChildPlayer
     *	Purpose:
     *	    Get Nth child player
     */
    STDMETHOD(GetChildPlayer)	    (THIS_
				    UINT16 uPlayerIndex,
				    REF(IHXPlayer*) pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::SetParentPlayer
     *	Purpose:
     *	    Set the parent player
     */
    STDMETHOD(SetParentPlayer)	    (THIS_
				    IHXPlayer* pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::RemoveParentPlayer
     *	Purpose:
     *	    Remove the parent player
     */
    STDMETHOD(RemoveParentPlayer)   (THIS_
				    IHXPlayer* pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::GetParentPlayer
     *	Purpose:
     *	    Get the parent player
     */
    STDMETHOD(GetParentPlayer)	    (THIS_
				    REF(IHXPlayer*) pPlayer);

    /* 
     * IHXRecordManager methods 
     */ 

    /************************************************************************ 
     *  Method: 
     *      IHXRecordManager::LoadRecordService 
     *  Purpose: 
     *      Called by TLC to supply the Core with record service. 
     */ 
    STDMETHOD(LoadRecordService) (THIS_ IHXRecordService* pRecordService); 

    /************************************************************************ 
     *  Method: 
     *      IHXRecordManager::GetRecordService 
     *  Purpose: 
     *      return current record service for the Player. 
     */ 
    STDMETHOD(GetRecordService) (THIS_ REF(IHXRecordService*) pRecordService); 

    /************************************************************************ 
     *  Method: 
     *      IHXRecordManager::UnloadRecordService 
     *  Purpose: 
     *      Called by TLC to ask the Core to stop using record service. 
     */ 
    STDMETHOD(UnloadRecordService) (THIS); 

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    /*
     *  IHXPDStatusMgr methods
     */        

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::AddObserver
     *  Purpose:
     *      Lets an observer register so it can be notified of file changes
     */
    STDMETHOD(AddObserver) (THIS_
                           IHXPDStatusObserver* /*IN*/ pObserver);

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::RemoveObserver
     *  Purpose:
     *      Lets an observer unregister so it can stop being notified of
     *      file changes
     */
    STDMETHOD(RemoveObserver) (THIS_
                              IHXPDStatusObserver* /*IN*/ pObserver);

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::SetStatusUpdateGranularityMsec
     *  Purpose:
     *      Lets an observer set the interval that the reporter (fsys) takes
     *      between status updates:
     */
    STDMETHOD(SetStatusUpdateGranularityMsec) (THIS_
                             UINT32 /*IN*/ ulStatusUpdateGranularityInMsec);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadProgress
     *
     *  Purpose:
     *      Notification from IHXPDStatusMgr of download progress when
     *      file size changes.
     *
     *      lTimeSurplus:
     *      - When negative, the absolute value of it is the estimated number
     *      of milliseconds of wall-clock time that need to pass while
     *      downloading continues before reaching the point at which playback
     *      can resume and play the remainder of the stream without having to
     *      buffer, assuming that playback is paused and remains so during
     *      that period.
     *      - When positive, it is the estimated number of milliseconds of
     *      wall-clock time between when the download should complete and when
     *      the natural content play-out duration will be reached, assuming
     *      playback is currently progressing and that no pause will occur.
     *
     *      Note: ulNewDurSoFar can be HX_PROGDOWNLD_UNKNOWN_DURATION if the
     *      IHXMediaBytesToMediaDurConverter was not available to, or was
     *      unable to convert the bytes to a duration for the IHXPDStatusMgr
     *      calling this:
     */
    STDMETHOD(OnDownloadProgress) (THIS_
            IHXStreamSource* /*IN*/  /*NULL is valid value*/ pStreamSource,
            UINT32 /*IN*/ ulNewDurSoFar,
            UINT32 /*IN*/ ulNewBytesSoFar,
            INT32  /*IN*/ lTimeSurplus);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnTotalDurChanged
     *  Purpose:
     *      This is a notification if the total file duration becomes known
     *      or becomes better-known during download/playback
     *      
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     */
    STDMETHOD(OnTotalDurChanged)  (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
            UINT32 /*IN*/ ulNewDur);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadComplete
     *
     *  Purpose:
     *      Notification that the entire file has been downloaded.
     *
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     *      
     */
    STDMETHOD(OnDownloadComplete)    (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::SrcClaimsSeekSupport
     *
     *  Purpose:
     *      Passes along notification from file sys that seek support
     *      is claimed to be available (although sometimes HTTP server
     *      claims this when it doesn't actually support it).
     *
     */
    STDMETHOD(SrcClaimsSeekSupport)    (THIS_
            IHXStreamSource* pStreamSource,
            HXBOOL /*IN*/ bSrcClaimsSeekSupport);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadPause
     *  Purpose:
     *      Notification that the file-download process has purposefully
     *      and temporarily halted downloading of the file
     *      
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     */
    STDMETHOD(OnDownloadPause)    (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadResume
     *  Purpose:
     *      Notification that the file-download process has resumed
     *      the process of downloading the remainder of the file
     *      
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     */
    STDMETHOD(OnDownloadResume)    (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource);
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
    /*
     * IHXPresentationFeatureManager methods:
     */

    /***************************************************************************
    * Method:
    *      IHXPresentationFeatureManager::SetPresentationFeature
    *
    * Purpose:
    *
    * This method is used for:
    * (1) adding a PF to the master PF list with an initial value and options,
    *  or
    * (2) setting or changing an existing PF's list of optional values.
    *
    * pFeatureOptions, an IHXValues, is a list of name+value pairs where the
    * names are the options that are possible for the PF to be set to during
    * any part of the presentation.  The associated value field for each name
    * should be set to "1" if that PF option is currently selectable, or "0" if
    * it is not selectable.  Anything other than "0" is treated as if it were
    * "1" (selectable).
    *
    * If pFeatureOptions is NULL, that indicates that the PF's possible values
    * are unconstrained (e.g., an URL or free-form text).  If it sees NULL, the
    * GUI may choose to prompt the user to input a free-form text string or
    * assume some bounds on input based purely on GUI design.  NOTE: if you want
    * to change the current value but not change the current list of optional
    * values of an existing PF, you must call SetPresentationFeatureValue(...)
    * instead.
    *
    * When this method is used for (2), above, i.e., for setting an existing PF's
    * optional values and each of their associated "is-selectable" flags, it
    * overwrites the PF's entire list of options and their associated flags.
    * Thus, callers who wish to add to or change an existing PF's options should
    * first get those options (IHXValues) by calling GetPresentationFeature(),
    * changing them as needed, then passing them back into this method.  The
    * IHXValues are passed by value, not by reference, to avoid race conditions
    * and other problems; it is very unlikely that will result in a strain on
    * resources as these operations are not anticipated to be performed very
    * often.
    *
    * This can be called at any time during a presentation.  Any time it is
    * called, IHXPresentationFeatureManager notifies all sinks by calling one
    * of the following.  If the only thing that changes is the current value,
    * then the following is called:
    *   PresentationFeatureCurrentSettingChanged([PF name],[PF new current val]);
    * otherwise (any other change) the folloing is called:
    *   PresentationFeatureChanged([PF name]);
    * 
    * If the pszFeatureName is not found in the PF list, HXR_PROP_NOT_FOUND
    * is returned.
    *
    * It returns HXR_PROP_INACTIVE if the PF already exists and its optional
    * values list contains the pFeatureSetting but it is set to "0" (i.e., is not
    * available for selection).
    */
    STDMETHOD (SetPresentationFeature) (THIS_
                                        const char* /*IN*/ pszFeatureName,
                                        IHXBuffer*  /*IN*/ pFeatureSetting,
                                        IHXValues* pFeatureOptions);

    /***************************************************************************
    * Method:
    *      IHXPresentationFeatureManager::GetPresentationFeature
    *
    * Purpose:
    *
    * This method gets all information stored for a particular presentation
    * feature: (1) the current value of the PF, and (2) the list of possible
    * optional values the PF is allowed to be set to in the current presentation.
    *
    * pFeatureOptions, an IHXValues, is a list of name+value pairs where the
    * names are the options that are possible for the PF to be set to during
    * any part of the presentation.  The associated value field for each name
    * should be set to "1" if that PF option is currently selectable, or "0" if
    * it is not selectable.  Anything other than "0" is treated as if it were
    * "1" (selectable).
    *
    * Note that pFeatureOptions can be NULL indicating that the choices are
    * not enumerable, e.g., URLs.
    *
    * Returns HXR_INVALID_PARAMETER if the feature name does not correspond
    * to a PF that was added already via SetPresentationFeature():
    */
    STDMETHOD (GetPresentationFeature) (THIS_
                                 const char* /*IN*/ pszFeatureName,
                                 REF(IHXBuffer*) /*OUT*/ pFeatureCurrentSetting,
                                 REF(IHXValues*) /*OUT*/ pFeatureOptions);

    /***************************************************************************
    * This method is used for adding a Presentation Feature (PF) to the master
    * PF list with an initial value, or for setting an existing PF's current
    * value.
    * pFeatureSetting can be NULL which indicates the PF's possible values are
    * unconstrained (e.g., an URL or free-form text).  If it sees NULL, the GUI
    * may choose to prompt the user to input a free-form text string or assume
    * some bounds on input based purely on GUI design.
    *
    * This can be called at any time during a presentation.  Any time it is
    * called, IHXPresentationFeatureManager notifies all sinks of the change
    * by calling:
    *   PresentationFeatureCurrentSettingChanged([PF name],[PF new current val]);
    * 
    * If the pFeatureName is not found in the PF list, HXR_PROP_NOT_FOUND
    * is returned.
    *
    * It returns HXR_PROP_INACTIVE if the PF already exists and its optional
    * values list contains the pFeatureSetting but it is set to "0" (i.e., is not
    * available for selection).
    */
    STDMETHOD (SetPresentationFeatureValue) (THIS_
                                           const char* /*IN*/ pszFeatureName,
                                           IHXBuffer*  /*IN*/ pRequestedPFCurrentSetting);

    /***************************************************************************
    * This method gets the current value of a particular presentation feature.
    *
    * Returns HXR_INVALID_PARAMETER if the pFeatureName does not yet exist in
    * the list of known PFs, i.e., if it has never been added via
     * SetPresentationFeature().
    */
    STDMETHOD (GetPresentationFeatureValue) (THIS_
                                             const char* /*IN*/ pszFeatureName,
                                             REF(IHXBuffer*) /*OUT*/ pFeatureSetting);

    /***************************************************************************
    * This method removes completely from the registry the PF, its current
    * value, and its assocaited optional values.  Note: nothing prevents two
    * components from adding the same PF.  Implementers of this may want to
    * keep a ref count of each PF to prevent one component from removing a PF
    * that another is still using.
    *
    * This can be called at any time during a presentation.
    */
    STDMETHOD (RemovePresentationFeature) (THIS_
                                           const char* /*IN*/ pszFeatureName);

    /***************************************************************************
    * This passes back the entire feature set that has been added to the player
    * registry for the presentation, along with each PF's current value.  Note
    * that the options for each PF are not included in what is provided by this
    * method; to get that information, call GetPresentationFeatureOptions(...)
    */
    STDMETHOD (GetPresentationFeatures) (THIS_ REF(IHXValues*) /*OUT*/ pFeatures);


    /***************************************************************************
    * If the application wishes to make a current PF value be the
    * global user-preference value for a PF that matches one of the
    * IHXPreferences, it does so using the following method.  Effect
    * on IHXPreferences is immediate.  If the PF does not match an
    * existing IHXPreference, HXR_INVALID_PARAMETER is returned, i.e.,
    * a new IHXPreference is not created in that case.
    *
    * This can be called at any time during a presentation.
    */
    STDMETHOD (SetGlobalUserPreference) (THIS_
                                         const char* /*IN*/ pszFeatureName);

    /***************************************************************************
    * This adds a PF sink to the list:
    */
    STDMETHOD (AddPresentationFeatureSink) (THIS_ IHXPresentationFeatureSink* pSink);

    /***************************************************************************
    * This removes a PF sink from the list:
    */
    STDMETHOD (RemovePresentationFeatureSink) (THIS_ IHXPresentationFeatureSink* pSink);

    STDMETHOD (ParsePFEvent)  (THIS_ const char* pszPFEvent,
                               REF(PfChangeEventType) /*OUT*/ thePfChangeEvent,
                               REF(IHXBuffer*) /*OUT*/ pFeatureName,
                               REF(IHXBuffer*) /*OUT*/ pFeatureSetting);

#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.

#if defined(HELIX_FEATURE_PLAYBACK_MODIFIER)
    /*
     *  IHXPlaybackModifier methods
     */
    /************************************************************************
     *  Method:
     *      IHXPlaybackModifier::SetPlaybackModifiers
     *
     *  Purpose:
     *      This method is used to adjust playback values without restarting
     *      the presentation.
     */
     STDMETHOD (SetPlaybackModifiers) ( THIS_
                                        IHXValues* pModifiers );

    /************************************************************************
     *  Method:
     *      IHXPlaybackModifier::GetPlaybackModifiers
     *
     *  Purpose:
     *      This method gets the adjustments made to the current presentation
     */
     STDMETHOD (GetPlaybackModifiers) ( THIS_
                                        IHXValues* &pModifiers );

#endif // HELIX_FEATURE_PLAYBACK_MODIFIER

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse);
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps);
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    STDMETHOD_(INT32,GetVelocity)          (THIS);
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode);
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS);
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS);
    STDMETHOD(CloseVelocityControl)        (THIS);

	// IHXFrameStep methods
#if defined(HELIX_FEATURE_VIDEO_FRAME_STEP)
	STDMETHOD(StepFrames)         ( THIS_ INT32  lSteps );
#endif

    // IHXPlaybackVelocityResponse methods (system components such
    // as renderers and/or proxies may use this to inform us)
    // IHXPlaybackVelocityResponse methods
    STDMETHOD(UpdateVelocityCaps) (THIS_ IHXPlaybackVelocityCaps* pCaps);
    STDMETHOD(UpdateVelocity)     (THIS_ INT32 lVelocity);
    STDMETHOD(UpdateKeyFrameMode) (THIS_ HXBOOL bKeyFrameMode);

    ////////////////////////////////////////////////////////////////////
    //
    // The following members are related to meta-file support.
    //
public:
    CHXMetaInfo*        GetMetaInfo() { return m_pMetaInfo; };

    void		SetModal(HXBOOL bModal) {m_bSetModal = bModal;};
    HX_RESULT		SetSingleURLPresentation(const CHXURL* pURL);

    HX_RESULT		AddURL(SourceInfo*& pSourceInfo, HXBOOL bAltURL);

    void		GetActiveRequest(IHXRequest*& pRequest)
			{
			    pRequest = NULL;			    

			    if (m_pRequest)
			    {
#if defined(HELIX_FEATURE_NESTEDMETA)
				// we want to keep the original copy of IHXRequest in SMIL/RAM
				// so that individual source within will have a "clean" copy of 
				// IHXRequest to start with
				if (m_pPersistentComponentManager &&
				    m_pPersistentComponentManager->m_pRootPersistentComponent &&
				    !m_pPersistentComponentManager->m_pRootPersistentComponent->m_bToBeClosed)
				{
				    CHXRequest::CreateFromCCFWithRequestHeaderOnly(m_pRequest, &pRequest, (IUnknown*)(IHXClientEngine*)m_pEngine);
				}
				else
#endif /* HELIX_FEATURE_NESTEDMETA */
				if (m_bActiveRequest)
				{
				    pRequest = m_pRequest;
				    pRequest->AddRef();				
				}
			    }
			};

    void		ResetActiveRequest() {m_bActiveRequest = FALSE;};

    void		SetGranularity(ULONG32 ulGranularity);
    ULONG32		GetGranularity(void) {return m_ulLowestGranularity;};

    HX_RESULT		SetGranularity(HXSource* pSource, UINT16 uStreamNumber, 
				       UINT32 ulGranularity);

    void                ClosePlayer(void);
    
    void		InternalPause();		

    HX_RESULT		EventReady(HXSource* pSource, CHXEvent* pEvent);

    void		ShutDown(void);

    void		RegisterSourcesDone(void);

    HXBOOL		IsInitialized(void) {return m_bInitialized;};

    HXBOOL		CanBeStarted(HXSource* pSource, SourceInfo* pThisSourceInfo, HXBOOL bPartOfNextGroup);
    void		EndOfSource(HXSource* pSource);

    HXBOOL		CanBeFastStarted(SourceInfo* pSourceInfo);
    void		SureStreamSourceRegistered(SourceInfo* pSourceInfo);
    void		SureStreamSourceUnRegistered(SourceInfo* pSourceInfo);

    ULONG32		GetInternalCurrentPlayTime(void)
			    { return m_ulCurrentPlayTime; };

    HX_RESULT		HandleRedirectRequest(SourceInfo* pSourceInfo);
    HX_RESULT           HandleSDPRequest(SourceInfo* pSourceInfo);

    HXBOOL		IsAtSourceMap(SourceInfo* pSourceInfo);

    HX_RESULT		RepeatTrackAdded(UINT16		/*IN*/ uGroupIndex,
					 UINT16		/*IN*/ uTrackIndex,
					 IHXValues*	/*IN*/ pTrack);

    /* Temporary function */
    void		SetInterrupt(HXBOOL bUseCoreThread) 
			    {m_bUseCoreThread = bUseCoreThread;};
    virtual HX_RESULT	SendPacket(CHXEvent* pEvent);
    CHXAudioPlayer*	GetAudioPlayer(void) {return m_pAudioPlayer;};
    INT32		GetCurrentGroupID(void) {return m_nCurrentGroup;};
    void                NotifyPlaybackEndToTLC() {m_bIsPresentationClosedToBeSent = TRUE;};
    HX_RESULT		CopyRegInfo(UINT32 ulFromRegID, UINT32 ulToRegID);

    HXBOOL              IsSitePresent(IHXSite* pSite);
    void		CheckIfRendererNeedFocus(IUnknown* pRenderer);
    void                SetKeyFrameInterval(UINT32 ulInterval) { m_ulKeyFrameInterval = ulInterval; }
    UINT32              GetNumForwardKeyFramesAhead() { return m_ulNumForwardKeyFramesAhead; }
    UINT32              GetNumReverseKeyFramesAhead() { return m_ulNumReverseKeyFramesAhead; }
    UINT32              ComputeFillEndTime(UINT32 ulCurTime, UINT32 ulOffsetScale, UINT32 ulOffsetFixed);
    void                CheckForPacketTimeOffsetUpdate(RendererInfo* pRendInfo);
    HX_RESULT           CalculateOnBeginTime(RendererInfo* pRendInfo, UINT32& ulOnBeginTime);

    UINT32		GetPlayerUpdateInterval(void)	{ return m_ulPlayerUpdateInterval; }
    UINT32		GetPlayerProcessingInterval(HXBOOL bAtInterruptTime)
    { 
        // if we are not using core thread, put all load on the system thread
        UINT32 ulSystemTimeProcessingInterval = m_ulPlayerSystemTimeProcessingInterval;
        UINT32 ulInterruptTimeProcessingInterval = m_ulPlayerInterruptTimeProcessingInterval;
        if (!m_bUseCoreThread)
        {
             ulSystemTimeProcessingInterval += ulInterruptTimeProcessingInterval;
             ulInterruptTimeProcessingInterval = 0;
        } 
	return (m_bIsPlaying || (m_pEngine && (m_pEngine->GetPlayerCount() > 1)))
			    ? (bAtInterruptTime ? ulInterruptTimeProcessingInterval : 
						  ulSystemTimeProcessingInterval)
			    : (m_bIsBuffering ? (m_ulPlayerUpdateInterval << 2) :
						m_ulPlayerUpdateInterval);
    }
    HXBOOL		IsInQuickSeek(void)		{ return m_bQuickSeekMode; }

    static void         PlayerCallback(void *pParam);
    static void		PlayerCallbackInterruptSafe(void* pParam);
    static void		PlayerCallbackInterruptOnly(void* pParam);
#if defined(HELIX_FEATURE_AUTHENTICATION)
    static void	        AuthenticationCallback(void *pParam);
#endif

#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
    HX_RESULT EstablishPFSSinkList();
    HX_RESULT NotifyAllPFSinks(PfChangeEventType pfChangeEventType, const char* pFeatureName);
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.

    friend		class SourceInfo;
    friend		class HXSource;
    friend		class HXNetSource;
    friend		class HXFileSource;
    friend		class HXPersistentComponent;
    friend		class HXPersistentComponentManager;

private:
    HX_BITFIELD		m_bForceStatsUpdate : 1;
    HX_BITFIELD		m_bActiveRequest : 1;
    HX_BITFIELD		m_bAddLayoutSiteGroupCalled : 1;
    HX_BITFIELD		m_bDoRedirect : 1;
    HX_BITFIELD		m_bFastStartCheckDone : 1;
    HX_BITFIELD		m_bFastStart : 1;
    HX_BITFIELD		m_bIsSmilRenderer : 1;

    TurboPlayOffReason	m_turboPlayOffReason;

    UINT32		m_ulActiveSureStreamSource;

    INT32		m_nCurrentGroup;
    INT32		m_nGroupCount;

    HX_RESULT		DoOpenGroup(UINT16 nGroupNumber);
    HX_RESULT		OpenTrack(IHXValues* pTrack, UINT16 uGroupID, UINT16 uTrackID);
    void		PlayNextGroup();

    void		SetClientState(EHXClientState eState);

    HXBOOL		CheckTrackAndSourceOnTrackStarted(INT32 nGroup, INT32 nTrack, UINT32 sourceID);

    HXBOOL		GetViewSourceStream(REF(IHXStreamSource*) pStrmSource);
    HXBOOL		AreAllPacketsSent();

    HX_RESULT		GetSourceInfo(UINT16 uGroupIndex, UINT16 uTrackIndex, SourceInfo*& pSourceInfo);
    HX_RESULT		UpdateTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues);
    HX_RESULT		RemoveTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues);
    HX_RESULT		AddPrefetchTrack(UINT16 uGroupIndex, UINT16 uPrefetchTrackIndex, IHXValues* pValues);
    HX_RESULT		UpdatePrefetchTrack(UINT16 uGroupIndex, UINT16 uPrefetchTrackIndex, IHXValues* pValues);
    HX_RESULT		RemovePrefetchTrack(UINT16 uGroupIndex, UINT16 uPrefetchTrackIndex, IHXValues* pValues);
    HX_RESULT		PrefetchTrackDone(UINT16 uGroupIndex, UINT16 uPrefetchTrackIndex, HX_RESULT status);

    HX_RESULT		BeginTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues);    
    HX_RESULT		PauseTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues);
    HX_RESULT		SeekTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues, UINT32 ulSeekTime);    
    HX_RESULT		StopTrack(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues);    
    HX_RESULT		SetSoundLevel(UINT16 uGroupIndex, UINT16 uTrackIndex, UINT16 uSoundLevel, HXBOOL bReflushAudioDevice);    

    void                CheckSourceRegistration(void);
    void                GetTimingFromURL(CHXURL* pURL, UINT32& ulStart, UINT32& ulEnd, UINT32& ulDelay, UINT32& ulDuration);

    HXBOOL              CanFileFormatClaimSchemeExtensionPair(CHXURL* pURL, 
							      REF(IHXPluginSearchEnumerator*) rpEnum,
							      HXBOOL bMustClaimExtension = FALSE);

#if defined(HELIX_FEATURE_RECORDCONTROL)
    HXBOOL                IsRecordServiceEnabled();
#endif

protected:
    CHXSimpleList*	m_pAltURLs;
    CHXSimpleList*	m_pOppositeHXSourceTypeRetryList;
    CHXURL*		m_pURL;
    IHXRequest*		m_pRequest;

    HX_RESULT		AdjustSeekOnRepeatedSource(SourceInfo*	pSourceInfo, 
						   UINT32	ulSeekTime);

    HX_RESULT 		DoNetworkOpen(SourceInfo*& pSourceInfo, HXBOOL bAltURL);
    HX_RESULT 		DoFileSystemOpen(SourceInfo*& pSourceInfo, HXBOOL bAltURL, IHXPluginSearchEnumerator* pFFClaim);

    HX_RESULT		CreateSourceInfo(SourceInfo*& pSourceInfo, HXBOOL bAltURL);
    HX_RESULT		PrepareSourceInfo(IHXValues* pTrack, SourceInfo*& pSourceInfo);
    HX_RESULT		SpawnSourceIfNeeded(SourceInfo* pSourceInfo);
    HX_RESULT		SwitchSourceIfNeeded(void);

    HX_RESULT		DoURLOpen(CHXURL* pURL, char* pMimeType);
    HX_RESULT		DoAltURL(void);
    HX_RESULT		DoOppositeHXSourceRetry(void);
    HX_RESULT		DoRedirect(void);
    HX_RESULT		OpenRedirect(const char* pszURL);
    HX_RESULT           DoSDPURL(void);
    HX_RESULT           DoURLOpenFromSource(SourceInfo* pSourceInfo, URL_TYPE urlType);

    HX_RESULT		InitializeRenderers(void);

    HX_RESULT		LayoutRenderers(void);
    virtual HX_RESULT	ProcessIdleExt(void);

    void		CreateDefaultRendererWindow
			    (
				RendererInfo*		    pRInfo,
				IHXRenderer*		    pRend,
				IHXBuffer*		    pDisplayInfo,
				const char*		    pName
			    );
    void		CreateLayoutWindow
			    (
				IHXMetaLayout*			pLayout
			    );

    /************************************************************************
     *	Method:
     *		ExecuteCurrentEvents
     *	Purpose:
     *		Send any due packets to the renderers
     *
     */
    HX_RESULT	ProcessCurrentEvents(UINT32 ulLoopEntryTime,
				     HXBOOL bAtInterrupt,
				     HXBOOL bFromInterruptSafeChain);

    /************************************************************************
     *	Method:
     *		DeleteAllEvents
     *	Purpose:
     *		Remove all pending events due to seek/stop.
     *
     */
    HX_RESULT	DeleteAllEvents(void);
    HX_RESULT	SendPreSeekEvents(void);
    HX_RESULT	AccelerateEventsForSource(HXSource* pSource);
    void	StopAllStreams(EndCode endCode = END_STOP);
    
    virtual HX_RESULT	SendPreSeekEventsExt(void);         
    virtual HX_RESULT	StopAllStreamsExt(EndCode endCode = END_STOP);
    
    virtual SourceInfo*	    NewSourceInfo(void);
    virtual HXFileSource*  NewFileSource(void);
    virtual HXNetSource*   NewNetSource(void);
    virtual HX_RESULT	    OpenTrackExt(void);

    void	CloseAllRenderers(INT32 nGroupSwitchTo);
    HX_RESULT	UnRegisterCurrentSources(void);

    HX_RESULT	SetupAllStreams(void);
    void	ResetPlayer(void);
    void	ResetGroup(void);
    void	SetMinimumPushdown(void);
    void	SchedulePlayer(UINT32 ulFlags = PLAYER_SCHEDULE_DEFAULT);
    HX_RESULT	CheckForAudioResume(UINT32 &ulSchedulerFlags);

    HX_RESULT	InitializeNetworkDrivers(void);

    inline	void	UpdateCurrentPlayTime( ULONG32 ulCurrentPlayTime );
    
    HX_RESULT	ActualReport(
			    const UINT8	unSeverity,  
			    HX_RESULT	ulHXCode,
			    const ULONG32	ulUserCode,
			    const char*	pUserString,
			    const char*	pMoreInfoURL);
    void AddRemoveErrorSinkTranslator(HXBOOL bAdd);

    IHXPendingStatus*	m_pMetaSrcStatus;

    // list of sources
    CHXMapPtrToPtr*	m_pSourceMap;
    CHXSimpleList*	m_pPendingTrackList;

    // current time on the timeline for this player..
    // this would either be updated by the audio services of any rendering object
    // happens to be an audio renderer and decides to use our audio services...
    // ELSE it will be updated by a timer object... 
    ULONG32		m_ulCurrentPlayTime;	
    
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
    // this tracks the current timeline for this player, with the caveat
    // that it's only updated when the current play time is updated at
    // system time. This is useful for deciding whether to allocated "extra"
    // time to the player or giving it back to the system -- a pretty
    // complicated issue on MacOS 8.
    ULONG32		m_ulCurrentSystemPlayTime;
#endif

    
    // Length of all sources!
    ULONG32		m_ulPresentationDuration;

    // used to inform renderers the pre and post seek info..
    ULONG32		m_ulTimeBeforeSeek;
    ULONG32		m_ulTimeAfterSeek;


    BufferingReason	m_BufferingReason;

    CHXEventList	m_EventList;	// contains packets for all streams

    IHXScheduler*	m_pScheduler;

    /* m_uNumSourcesActive gets decremented when we have sucked out all the packets
     * from the source 
     */
    UINT16		m_uNumSourcesActive : 16;

    /* m_uNumCurrentSourceNotDone gets decremented when a given source has received
     * all the packets from the server
     */
    UINT16		m_uNumCurrentSourceNotDone : 16;

    EHXClientState	m_eClientState;		// the state of the client player, ready, connected, prefetched, etc.
    EHXClientStateStatus m_eClientStateStatus;	// transition state of the client player, active or halted
    HXBOOL		m_bHaltInConnected;	// Indicates whether or not to halt in the connected state
    HXBOOL		m_bRestartToPrefetched;	// Indicates whether or not to transition the player
   						// to the prefetched state after end of media

    HX_BITFIELD		m_bSourceMapUpdated : 1;
    HX_BITFIELD		m_bInitialized : 1;
       
    // m_bIsDone: single presentation/individual group in a SMIL file
    // m_bIsPresentationDone: single/SMIL presentation
    HX_BITFIELD		m_bIsDone : 1;
    HX_BITFIELD		m_bIsPresentationDone : 1;
    HX_BITFIELD		m_bInStop : 1;
    HX_BITFIELD		m_bIsPresentationClosedToBeSent : 1;
    HX_BITFIELD		m_bCloseAllRenderersPending : 1;

    HX_BITFIELD		m_bUseCoreThread : 1;
    HX_BITFIELD		m_bPaused : 1;
    HX_BITFIELD		m_bBeginPending : 1;

    HX_BITFIELD		m_bIsFirstBeginPending : 1;

    HX_BITFIELD		m_bIsFirstBegin : 1;

    HX_BITFIELD		m_bUserHasCalledBegin : 1;

    HX_BITFIELD		m_bTimelineToBeResumed : 1;
    HX_BITFIELD		m_bIsPlaying : 1;
    HX_BITFIELD		m_bIsBuffering : 1;

    HX_BITFIELD		m_bNetInitialized : 1;
    HX_BITFIELD		m_bPrefTransportInitialized : 1;

    HX_BITFIELD		m_bSetupLayoutSiteGroup : 1;
    
    HX_BITFIELD		m_bTimeSyncLocked : 1;

    HX_BITFIELD		m_bIsLive : 1;
    HX_BITFIELD		m_bLiveSeekToBeDone : 1;
    HX_BITFIELD		m_bHasSubordinateLifetime : 1;

    HX_BITFIELD		m_bProcessEventsLocked : 1;
    HX_BITFIELD		m_bDidWeDeleteAllEvents : 1;

    UINT32		m_ulCoreLockCount;
    ULONG32		m_ulLowestGranularity;
    UINT32		m_ulElapsedPauseTime;
    UINT32		m_ulLiveSeekTime;
    UINT32		m_ulSeekTime;
    UINT32		m_ulTimeOfPause;
    UINT32		m_ulMinimumTotalPreroll;
    
    friend class HXPlayerCallback;

//#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    friend class UpdateStatsCallback;
//#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    /* Used for multiple kive stream synchrnoization*/
    UINT32		m_ulFirstTimeSync;

    UINT32		m_ulFSBufferingEndTime;
    UINT16		m_uNumSourceToBeInitializedBeforeBegin : 16;

    HX_BITFIELD		m_bFastStartInProgress : 1;
    HX_BITFIELD		m_bIsFirstTimeSync : 1;
    HX_BITFIELD		m_bPlayerWithoutSources : 1;
    HX_BITFIELD		m_bInternalPauseResume : 1;
    HX_BITFIELD		m_bInternalReset : 1;
    HX_BITFIELD         m_bSetVelocityInProgress : 1;
    HX_BITFIELD		m_bCurrentPresentationClosed : 1;
    HX_BITFIELD		m_bContactingDone : 1;
    HX_BITFIELD		m_bFSBufferingEnd : 1;
    HX_BITFIELD		m_b100BufferingToBeSent : 1;
    HX_BITFIELD		m_bSetupToBeDone : 1;
    HX_BITFIELD		m_bPostSetupToBeDone : 1;
    HX_BITFIELD		m_bInternalReportError : 1;
    HX_BITFIELD		m_bPartOfNextGroup : 1;   
    HX_BITFIELD		m_bLastGroup : 1;
    HX_BITFIELD		m_bNextGroupStarted : 1;
    HX_BITFIELD		m_bBeginChangeLayoutTobeCalled : 1;
    HX_BITFIELD		m_bPendingAudioPause : 1;
    HX_BITFIELD		m_bPlayStateNotified : 1;
    HX_BITFIELD		m_bResumeOnlyAtSystemTime : 1;
    HX_BITFIELD		m_bSetModal : 1;
    HX_BITFIELD		m_bEventAcceleration : 1;
    HX_BITFIELD		m_bSeekCached : 1;
    HX_BITFIELD		m_bPlaybackVelocityCached : 1;

#if defined(HELIX_FEATURE_PREFETCH)
    PrefetchManager*	m_pPrefetchManager;
#else
    void*		m_pPrefetchManager;
#endif /* HELIX_FEATURE_PREFETCH */

#ifdef _WIN32
    HXBOOL		m_bScreenSaverActive; 
#endif

#if defined(HELIX_FEATURE_NEXTGROUPMGR)
    NextGroupManager*	m_pNextGroupManager;
#else
    void*		m_pNextGroupManager;
#endif /* HELIX_FEATURE_NEXTGROUPMGR*/

    IHXGroup*		m_pCurrentGroup;

    IHXPlayer*		m_pParentPlayer;
    CHXSimpleList*	m_pChildPlayerList;

#if defined(HELIX_FEATURE_ASM)
    IHXBandwidthManager* m_pBandwidthMgr;
    /* Only used for load testing */
    IHXBandwidthManager* m_pASM;
#endif /* HELIX_FEATURE_ASM */

    UINT32 		m_ulPlayerUpdateInterval;
    UINT32		m_ulPlayerInterruptTimeProcessingInterval;
    UINT32		m_ulPlayerSystemTimeProcessingInterval;
    HXBOOL		m_bYieldLessToOthers;
    CHXSimpleList	m_ToBeginRendererList;	
    void		EnterToBeginList(RendererInfo* pRendInfo);
    void		EmptyBeginList(void) {m_ToBeginRendererList.RemoveAll();};
    HX_RESULT		CheckBeginList(void);
    void		RemoveFromPendingList(RendererInfo* pRendInfo);

    void		CheckToStartNextGroup(void);
    void		AdjustPresentationTime(void);
    void		SetPresentationTime(UINT32 ulPresentationTime);

    void		UpdateSourceActive(void);
    HX_RESULT		UpdateSourceInfo(SourceInfo* pSourceInfo, 
					 UINT32 ulParentRegId,
					 UINT16 ulTrackIndex);
    HX_RESULT		UpdatePersistentSrcInfo(SourceInfo* pSourceInfo, 
                                                UINT32 ulParentRegId,
                                                UINT16 ulTrackIndex);

    //QuickSeek Support
    void    LeaveQuickSeekMode(HXBOOL bDoSeek=FALSE); //handles all leave-mode logic
    HXBOOL  IsFirstFrameDisplayed(void);
    HXBOOL  m_bQuickSeekMode;     //Mode boolean
    UINT32  m_ulSeekQueue;        //Any queued up seek
    UINT32  m_ulBufferingCompletionTime;

private:
    /*
     * The following members are related to the new layout support.
     */
#if defined(HELIX_FEATURE_VIDEO)
    CHXSiteManager*	m_pSiteManager;
    IHXSiteSupplier*	m_pSiteSupplier;
#else
    void*		m_pSiteManager;
    void*		m_pSiteSupplier;
#endif /* HELIX_FEATURE_VIDEO */

    CHXSimpleList	m_SiteRequestIDList;
    
    void		SetupRendererSite(IUnknown* pRenderer, IHXValues* pProps, HXBOOL bIsPersistent);
    void		SetupLayoutSiteGroup(IUnknown* pLSG, HXBOOL bIsPersistent);
    STDMETHODIMP	SetupLayout(HXBOOL bIsPersistent);
    STDMETHODIMP	CleanupLayout(void);

    HX_RESULT		StartDownload(void);
    HX_RESULT		PauseDownload(void);
    void		UnregisterNonActiveSources();
    HXBOOL		AreAllSourcesSeekable(void);
    void		ResetError(void);
    void		SetLastError(HX_RESULT theErr);

    void		Close();
    void		AbortPlayer(void);
    void		StopPlayer(EndCode endCode = END_STOP);
    HX_RESULT		PausePlayer(HXBOOL bNotifyTLC = TRUE);
    HX_RESULT		SeekPlayer(ULONG32 ulTime);
    HX_RESULT		BeginPlayer(void);

    HX_RESULT		SetupAudioPlayer(UINT32 &ulSchedulerFlags);
    virtual HX_RESULT	PrepareAudioPlayer(void);

//#if defined(HELIX_FEATURE_AUTHENTICATION)
    void		ProcessPendingAuthentication();
    void		ClearPendingAuthenticationRequests();
//#endif /* HELIX_FEATURE_AUTHENTICATION */

    void		SendPostSeekIfNecessary(RendererInfo* pRendererInfo);

    HXBOOL		ScheduleOnTimeSync(void);
    HXBOOL		DurationTimeSyncAllSent(SourceInfo* pSourceInfo);
    virtual void	DisableScreenSaver();
    void                RemovePendingCallback(CHXGenericCallback* pCB);

    enum PlaybackVelocityCommandType
    {
        VelocityCommandInit,
        VelocityCommandQuery,
        VelocityCommandSetVelocity,
        VelocityCommandSetKeyFrameMode,
        VelocityCommandClose,
        VelocityCommandQueryCaps
    };
    IHXPlaybackVelocityResponse* m_pPlaybackVelocityResponse;
    IHXPlaybackVelocityCaps*     m_pPlaybackVelocityCaps;
    INT32                        m_lPlaybackVelocity;
    HXBOOL                       m_bKeyFrameMode;
    HXBOOL                       m_bAutoSwitch;
    INT32                        m_lPlaybackVelocityCached;
    HXBOOL                       m_bKeyFrameModeCached;
    HXBOOL                       m_bAutoSwitchCached;
    HXBOOL                       m_bVelocityControlInitialized;
    UINT32                       m_ulKeyFrameInterval;
    UINT32                       m_ulNumForwardKeyFramesAhead;
    UINT32                       m_ulNumReverseKeyFramesAhead;
    HXBOOL                       m_bStopWhenHitStartInReverse;
    HX_RESULT                    DoVelocityCommand(IHXPlaybackVelocity* pVel, PlaybackVelocityCommandType eCmd);
    HX_RESULT                    PlaybackVelocityCommand(PlaybackVelocityCommandType eCmd);


#if defined(HELIX_FEATURE_AUTOUPGRADE)
    IHXUpgradeCollection* m_pUpgradeCollection;
#else
    void* m_pUpgradeCollection;
#endif /* HELIX_FEATURE_AUTOUPGRADE */

    friend class NextGroupManager;

//#if defined(HELIX_FEATURE_PREFETCH)
    friend class PrefetchManager;
//#endif /* HELIX_FEATURE_PREFETCH */
    
    // used to notify of changes to the precache group
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    friend class HXAdvancedGroup;    
    friend class HXAdvancedGroupManager;    
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */

#if defined(HELIX_FEATURE_BASICGROUPMGR)
    friend class HXBasicGroup;
    friend class HXBasicGroupManager;
#endif /* HELIX_FEATURE_BASICGROUPMGR */

    HX_RESULT NextGroupSet(UINT16 uGroupIndex);

//#if defined(HELIX_FEATURE_AUTHENTICATION)
#ifndef _WIN16
    // Store pending requests
    friend class _CHXAuthenticationRequests;
    _CHXAuthenticationRequests* m_pAuthenticationRequestsPending;
#endif /* _WIN16 */
//#endif /* HELIX_FEATURE_AUTHENTICATION */

    IHXMutex*		m_pCoreMutex;

    /*
     * -- LIVE SYNC SUPPORT --
     *
     * The following is related to shared wall clock support
     * for syncronizing several live sources. See srcinfo.cpp
     * for more informantion.
     */

    CHXMapStringToOb*	m_pSharedWallClocks;
    friend class SharedWallClock;

    IHXRecordService*	        m_pRecordService;
    HXBOOL                      m_bRecordServiceEnabled;

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    UINT32              m_ulTotalDurReported;
    UINT32              m_ulTimeOfOpenURL;
    IHXPDStatusMgr*     m_pPDStatusMgr;
    CHXSimpleList*      m_pPDSObserverList;
    CHXSimpleList*      m_pProgDnldStatusReportInfoList;
    HX_RESULT           EstablishPDSObserverList();
    class CProgDnldStatusRptInfo
    {
      public:
        UINT32 m_ulRelTimeOfRept;
        UINT32 m_ulDurOfDnldBytesAtRptTime;
        CProgDnldStatusRptInfo(UINT32 ulDurOfDnldBytesAtRptTime,
                               UINT32 ulTimeOfOpenURL) :
                m_ulDurOfDnldBytesAtRptTime(ulDurOfDnldBytesAtRptTime)
              , m_ulRelTimeOfRept(CALCULATE_ELAPSED_TICKS(ulTimeOfOpenURL, HX_GET_BETTERTICKCOUNT())) {}
        ~CProgDnldStatusRptInfo() {}
      private:
          // /Don't use default ctor:
          CProgDnldStatusRptInfo() { HX_ASSERT(0); } 
    };
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)
#if defined(HELIX_FEATURE_EVENTMANAGER)
    CHXSimpleList* m_pPFSEventProxyList; //  CPresentationFeatureEventProxy list.
#else   //  If Events are not supported, then P.F.Sinks are notified directly
    CHXSimpleList* m_pPFSSinkList; //  IHXPresentationFeatureSink list.
#endif //  End else of #if defined(HELIX_FEATURE_EVENTMANAGER).
#endif // HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION.

#if defined(HELIX_FEATURE_PLAYBACK_MODIFIER)
    IHXValues* m_pPlaybackModifiers;

    STDMETHODIMP ParsePlaybackModifiers( IHXValues* pOptions );
    STDMETHODIMP SetPlayRangeEndTime( UINT32 ulEndTime );
#endif

public:
    HXBOOL FindSharedWallClocks(const char* pClockName, void*& ppValue)
	    {
		return m_pSharedWallClocks->Lookup(pClockName,ppValue);
	    };
};

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
class UpdateStatsCallback : public IHXCallback
{
public:
    HXPlayer*		m_pPlayer;
    CallbackHandle	m_PendingHandle;
    HXBOOL		m_bIsCallbackPending;

			UpdateStatsCallback();
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXCallback methods
     */
    STDMETHOD(Func)		(THIS);

protected:
			~UpdateStatsCallback();

    LONG32		m_lRefCount;
};
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

class HXPlayerCallback : public CHXGenericCallback
                       , public IHXInterruptSafe
		       , public IHXInterruptOnly
{
public:    
    HXBOOL m_bInterruptSafe;
    HXBOOL m_bInterruptOnly;

    HXPlayerCallback(void* pParam, fGenericCBFunc pFunc);
    
    STDMETHOD_(ULONG32,AddRef)	(THIS) {return CHXGenericCallback::AddRef();}
    STDMETHOD_(ULONG32,Release)	(THIS) {return CHXGenericCallback::Release();}
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj);

    STDMETHOD_(HXBOOL,IsInterruptSafe) (THIS);
    STDMETHOD_(HXBOOL,IsInterruptOnly) (THIS);

protected:
    ~HXPlayerCallback() {};
};

#endif //_HXPLAYER_
