/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcleng.h,v 1.49 2009/03/10 20:07:31 ehyche Exp $
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

#ifndef _HXCLIENTENGINE_
#define _HXCLIENTENGINE_

#include "chxelst.h"
#include "hxmon.h"
#include "hxclreg.h"
#include "statinfo.h"
#include "strminfo.h"
#include "hxshtdn.h"
#include "hxupgrd.h"
#include "hxerror.h"
#include "hxcore.h"
#include "hxengin.h"
#include "hxplugn.h"
#include "ihxcontextuser.h"
#if defined(HELIX_FEATURE_LITEPREFS)
#include "chxliteprefs.h"
#endif

#ifdef _UNIX
#include "hxengin.h"
#include "ihxmedpltfm.h"
#endif

#if  !defined(_DEBUG) && !defined(HELIX_CONFIG_MINIMIZE_SIZE)
#define _MEDIUM_BLOCK 1
#endif

class	HXCommonClassFactory;
class	HXScheduler;
class   HXOptimizedSchedulerBase;
class	HXPreferences;
class	HXClientRegistry;
class	CHXAudioSession;
class	HXCoreGuiHook;
class	Plugin2Handler;
class	HXPluginManager;
class	BaseHandler;
class	HXAutoConfig;
class	CHXPlayerSinkControl;
class	HXCoreComm;
class   HXMutex;
class	HXValidator;
class	HXExternalResourceManager;
class	CHXCredentialsCache;
class	CHXResMgr;
class	HXPreferredTransportManager;
class	HXProxyManager;
class	CMediumBlockAllocator;
class	HXViewSource;
class	CHXMapStringToOb;
class   HXOverlayManager;
class	HXNetInterface;
class	HXPlayer;
class   HXFirewallControlManager;
class   CHXABDCalibrator;

struct	IHXClientAdviseSink;
struct	IHXErrorMessages;
struct	IHXCommonClassFactory;
struct	IHXClientEngine;
struct	IHXClientEngine2;
struct	IHXMimeTypeMapper;
struct	IHXRegistry;
struct  IHXPreferences;
struct  IHXShutDownEverything;
struct	IHXValidator;
struct  IHXSiteEventHandler;
struct  IHXSystemRequired;
struct  IHXMultiPlayPauseSupport;
struct	IHXProxyAutoConfig;
struct  IHXBandwidthManager;
struct	IHXContextUser;
struct	IHXSiteEventHandler;
struct	IHXMediaPlatformKicker;

struct  IHXExternalSystemClock;

#if defined(HELIX_FEATURE_NETSERVICES)
_INTERFACE IHXNetServices;
class CHXClientNetServices;
#if defined(HELIX_FEATURE_NETSERVICES_SHIM)
class CHXClientNetServicesShim;
#endif
#if defined(HELIX_FEATURE_NETSERVICES_SHIM) || defined(HELIX_FEATURE_NET_LEGACYAPI)
class HXNetworkServices;
#endif
#endif //HELIX_FEATURE_NETSERVICES

#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
class HXSystemRequired : public IHXSystemRequired
{
protected:
    LONG32		    m_lRefCount;
public:
    HXSystemRequired() {m_lRefCount = 0;}
    ~HXSystemRequired() {};
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXSystemRequired methods
     */

    /************************************************************************
     *	Method:
     *		IHXSystemRequired::HasFeatures
     *	Purpose:
     *		Check if required features are present on the system.
     *  Returns:
     *		HXR_OK -    features are here, no upgrade required;
     *			    all features are removed from pFeatures.
     *		HXR_FAIL -  some features are missing;
     *			    pFeatures contains only those features 
     *			    that need upgrade.
     *
     */			      
    STDMETHOD(HasFeatures)  (THIS_ 
			    IHXUpgradeCollection* pFeatures);
};
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */

class HXClientEngine 
        : public IHXClientEngine
	, public IHXClientEngine2
#if defined _UNIX && !defined _VXWORKS
        , public IHXClientEngineSelector
        , public IHXAsyncIOSelection
#endif
        , public IHXClientEngineSetup
        , public IHXInterruptState
        , public IHXShutDownEverything
        , public IHXOverrideDefaultServices
        , public IHXErrorMessages
        , public IHXClientEngineMapper
        , public IHXCoreMutex
        , public IHXAutoBWCalibrationAdviseSink
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
        , public IHXExternalSystemClock;
#endif //HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER
	, public IHXContextUser
#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
        , public IHXMacBlitMutex
#endif
{
protected:
    LONG32		    m_lRefCount;
    ULONG32		    m_ulPlayerIndex;

    INT32		    m_unRegistryID;
    IHXRegistry*	    m_pRegistry;

    IHXCommonClassFactory*  m_pCommonClassFactory;
#if defined(_MEDIUM_BLOCK)    
    CMediumBlockAllocator*  m_pAllocator;
#endif    
    IHXScheduler*	    m_pScheduler;
    IHXScheduler2*	    m_pScheduler2;
    IHXOptimizedScheduler*  m_pOptimizedScheduler;
    IHXPreferences*	    m_pPreferences;

#if 0 // XXX HP Atlas
#if defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
    IHXPreferences*	    m_pOrigPreferences;
#elif defined(HELIX_FEATURE_LITEPREFS)
    CHXLitePrefs*	    m_pOrigPreferences;
#else
    HXPreferences*	    m_pOrigPreferences;
#endif
#endif

    CHXAudioSession*	    m_pAudioSession;
#if defined(HELIX_FEATURE_NETSERVICES)
    IHXNetServices*         m_pNetServices;
#if defined(HELIX_FEATURE_NET_LEGACYAPI)
    IHXNetworkServices*     m_pOldNetServices;
#endif
#endif
#ifdef _UNIX
    IHXAsyncIOSelection*    m_pAsyncIOSelection;
    HXBOOL                    m_bNetworkThreading;
#endif
    IHXProxyAutoConfig*    m_pProxyAutoConfig;

    HXValidator*	    m_pValidator;
    IHXBandwidthManager*    m_pASM;
    CHXPlayerSinkControl*   m_pPlayerSinkControl;
    CHXCredentialsCache*   m_pCredentialsCache;
    CHXResMgr*		    m_pResMgr;
    HXExternalResourceManager*	m_pExternalResourceManager;
    HXViewSource*	    m_pViewSource;
    IHXSystemRequired*	    m_pSystemRequired;

    IHXSiteEventHandler*    m_pSiteEventHandler;
    IHXMediaPlatformKicker* m_pKicker;

    HXFirewallControlManager*       m_pFWCtlMgr;
    HXProxyManager*	            m_pProxyManager;

#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
    CHXABDCalibrator*              m_pABDCalibrator;
#else /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */
    void*                          m_pABDCalibrator;
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */
    HXPreferredTransportManager*   m_pPreferredTransportManager;
    HXOverlayManager*              m_pOverlayManager;
    IHXMultiPlayPauseSupport*	   m_pMultiPlayPauseSupport;
    IUnknown*                      m_pConnBWInfo;
public:

    IHXPlugin2Handler*	    m_pPlugin2Handler;

    HXClientEngine(void);
    ~HXClientEngine(void);

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXClientEngine methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::CreatePlayer
     *	Purpose:
     *	    Creates a new IHXPlayer instance.
     *
     */
    STDMETHOD(CreatePlayer)	(THIS_
				REF(IHXPlayer*)    pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::ClosePlayer
     *	Purpose:
     *	    Called by the engine when it is done using the player...
     *
     */
    STDMETHOD(ClosePlayer)	(THIS_
				IHXPlayer*    pPlayer);

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::GetPlayerCount
     *	Purpose:
     *	    Returns the current number of IHXPlayer instances supported by
     *	    this client engine instance.
     */
    STDMETHOD_(UINT16, GetPlayerCount)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::GetPlayer
     *	Purpose:
     *	    Returns the Nth IHXPlayer instances supported by this client 
     *	    engine instance.
     */
    STDMETHOD(GetPlayer)	(THIS_
				UINT16		nPlayerNumber,
				REF(IUnknown*)	pUnknown);

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::EventOccurred
     *	Purpose:
     *	    Clients call this to pass OS events to all players. HXxEvent
     *	    defines a cross-platform event.
     */
    STDMETHOD(EventOccurred)	(THIS_
				HXxEvent* /*IN*/ pEvent);

    /*
     * IHXClientEngine2 methods
     */
    STDMETHOD(Close)		(THIS);

    /*
     *	IHXMimeTypeMapper methods
     */
    STDMETHOD(MapFromExtToMime)	(THIS_
				const char*	    /*IN*/  pExtension,
				REF(const char*)    /*OUT*/ pMimeType);
#ifdef _UNIX
    STDMETHOD_(INT32,Select) (THIS_
			      INT32 n,
			      fd_set* readfds,
			      fd_set* writefds,
			      fd_set* exceptfds,
			      struct timeval* timeout);

    STDMETHOD(Add)           (THIS_
			      IHXCallback* pCallback,
			      INT32         lFileDescriptor,
			      UINT32        ulFlags);
    STDMETHOD(Remove)        (THIS_
			      INT32         lFileDescriptor,
			      UINT32        ulFlags);

    CHXSimpleList*            m_select_callbacks;
#endif

    /*
     * IHXClientEngineSetup methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientEngineSetup::Setup
     *	Purpose:
     *      Top level clients use this interface to over-ride certain basic 
     *	    interfaces are: IHXPreferences, IHXHyperNavigate
     */
    STDMETHOD(Setup)		(THIS_
				IUnknown* pContext);

    /*
     * IHXInterruptState methods
     */
    STDMETHOD_(HXBOOL,AtInterruptTime)	(THIS);

    STDMETHOD(EnterInterruptState)	(THIS);

    STDMETHOD(LeaveInterruptState)	(THIS);

    STDMETHOD(EnableInterrupt)		(THIS_
					HXBOOL	bEnable);
    
    STDMETHOD_(HXBOOL, IsInterruptEnabled) (THIS);

    /*
     * IHXShutDownEverything methods
     */

    /************************************************************************
     *	Method:
     *	    IHXShutDownEverything::ShutDown
     *	Purpose:
     *	    Shutdown all the renderers/fileformats
     *
     */
    STDMETHOD(ShutDown)		(THIS);

    /************************************************************************
     *	Method:
     *	    IHXShutDownEverything::StopAllOtherPlayers
     *	Purpose:
     *	    Stop all the players in other processes if they use audio
     *
     */
    STDMETHOD(StopAllOtherPlayers)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXShutDownEverything::AskAllOtherPlayersToReload
     *	Purpose:
     *	    Ask all other players in other processes to reload their 
     *	    DLLs.
     *
     */
    STDMETHOD(AskAllOtherPlayersToReload)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXShutDownEverything::AskAllOtherPlayersToUnload
     *	Purpose:
     *	    Ask all other players in other processes to unload their 
     *	    unused DLLs.
     *
     */

    STDMETHOD(AskAllOtherPlayersToUnload)	(THIS);

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
     *  IHXErrorMessages methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorMessages::Report
     *	Purpose:
     *	    Call this method to report an error, event, or status message.
     *	Parameters:
     *
     *	    const UINT8	unSeverity
     *	    Type of report. This value will impact how the player, tool, or
     *	    server will react to the report. Possible values are described 
     *	    above. Depending on the error type, an error message with the 
     *	    RMA code, anda string translation of that code will be displayed. 
     *	    The error dialog includes a "more info" section that displays the
     *	    user code and string, and a link to the more info URL. In the 
     *	    server these messages are logged to the log file.
     *
     *	    const ULONG32   ulHXCode
     *	    Well known RMA error code. This will be translated to a text
     *	    representation for display in an error dialog box or log file.
     *
     *	    const ULONG32   ulUserCode
     *	    User specific error code. This will NOT be translated to a text
     *	    representation. This can be any value the caller wants, it will
     *	    be logged or displayed but not interpretted.
     *
     *	    const char*	    pUserString
     *	    User specific error string. This will NOT be translated or 
     *	    modified. This can be any value the caller wants, it will
     *	    be logged or displayed but not interpretted.
     *
     *	    const char*	    pMoreInfoURL
     *	    User specific more info URL string.
     *
     */
    STDMETHOD(Report)		(THIS_
				const UINT8	unSeverity,  
				HX_RESULT	ulHXCode,
				const ULONG32	ulUserCode,
				const char*	pUserString,
				const char*	pMoreInfoURL);

    /************************************************************************
     *	Method:
     *	    IHXErrorMessages::GetErrorText
     *	Purpose:
     *	    Call this method to get the text description of a RMA error code.
     *	Parameters:
     *	    HX_RESULT ulHXCode (A RMA error code)
     *  Return Value:
     *	    IHXBuffer* containing error text.
     */
    STDMETHOD_(IHXBuffer*, GetErrorText)	(THIS_
						HX_RESULT	ulHXCode);

    /************************************************************************
     *	Method:
     *	    IHXClientEngineMapper::GetPlayerBySite
     *	Purpose:
     *	    Returns the IHXPlayer instance supported by this client 
     *	    engine instance that contains the specified IHXSite.
     */
    STDMETHOD(GetPlayerBySite)	(THIS_
				IHXSite*	pSite,
				REF(IUnknown*)	pUnknown);

    /************************************************************************
     *	Method:
     *	    IHXCoreMutex::LockCoreMutex
     *	Purpose:
     *      Call this method to lock the client engine's core mutex.
     */
    STDMETHOD(LockCoreMutex)    (THIS);

    /************************************************************************
     *	Method:
     *	    IHXCoreMutex::UnlockCoreMutex
     *	Purpose:
     *      Call this method to unlock the client engine's core mutex.
     */
    STDMETHOD(UnlockCoreMutex)    (THIS);

    /*
     *  IHXAutoBWCalibrationAdviseSink methods
     */
    STDMETHOD(AutoBWCalibrationStarted) (THIS_
                                         const char* pszServer) { return HXR_OK; };

    STDMETHOD(AutoBWCalibrationDone)    (THIS_
                                         HX_RESULT  status,
                                         UINT32     ulBW);

    // IHXContextUser
    STDMETHOD (RegisterContext)		(THIS_ 
					 IUnknown* pIContext);

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)

    /************************************************************************
     *	Method:
     *	    IHXMacBlitMutex::LockMacBlitMutex
     *	Purpose:
     *      Call this method to lock the Mac blitting mutex
     */
    STDMETHOD(LockMacBlitMutex)    (THIS);

    /************************************************************************
     *	Method:
     *	    IHXMacBlitMutex::UnlockMacBlitMutex
     *	Purpose:
     *      Call this method to unlock the Mac blitting mutex
     */
    STDMETHOD(UnlockMacBlitMutex)    (THIS);

#endif

#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
    /*
     *	IHXExternalSystemClock methods
     */

    /************************************************************************
     *	Method:
     *	    IHXExternalSystemClock::InitClock
     *	Purpose:
     *	    
     */
    STDMETHOD(InitClock)(THIS_ HXTimeval* pCurTime);

    /************************************************************************
     *	Method:
     *	    IHXExternalSystemClock::UpdateClock
     *	Purpose:
     *	    
     */
    STDMETHOD(UpdateClock)	(THIS_ HXTimeval* pCurTime);

    /************************************************************************
     *	Method:
     *	    IHXExternalSystemClock::IsExternalClockUsed
     *	Purpose:
     *	    
     */
    STDMETHOD_(HXBOOL,IsExternalClockUsed) (THIS);

    /************************************************************************
     *	Method:
     *	    IHXExternalSystemClock::GetCurrentClock
     *	Purpose:
     *	     
     */
    STDMETHOD_(ULONG32,GetCurrentClock)	(THIS_ HXTimeval* pCurTime);
#endif /*HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER*/


    STDMETHOD(StopAudioPlayback)	(THIS);

    IHXMutex*	GetCoreMutex()	{return m_pCoreMutex;};

    CHXResMgr*	GetResMgr() {return m_pResMgr;};

    void	NotifyPlayState(HXBOOL bInPlayingState);

    UINT32	m_lROBActive;

protected:
    void _Initialize();
    virtual CHXAudioSession*	NewAudioSession();
    virtual HXPlayer*		NewPlayer();
    void			CreatePrefIfNoExist(const char* pName, const char* pValue);
    void			CreatePluginDir();
    void			CreateCodecDir();
    void			InitializeThreadedObjects();
    void			InitializeRegistry();
    IHXBuffer*			CreateBufferAndSetToString(const char* pStr);
    void			InitPaths();

    CHXSimpleList		m_PlayerList;
    HX_RESULT			m_LastError;
    HX_BITFIELD			m_bIsSchedulerStarted : 1;
    HX_BITFIELD			m_bInitialized : 1;
    HXBOOL			m_bUseCoreThread;
    HX_BITFIELD			m_bUseCoreThreadExternallySet : 1;
    HXCoreComm*			m_pCoreComm;
    IHXMutex*			m_pCoreMutex;
#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
    HXBOOL			m_bUseMacBlitMutex;
    IHXMutex*			m_pMacBlitMutex;
#endif
    const char*			m_AUName;
    IUnknown*			m_pContext;
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
    HXTimeval*                    m_pCurrentTime;
#endif /*HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER*/

};

#endif // HXClientEngine

