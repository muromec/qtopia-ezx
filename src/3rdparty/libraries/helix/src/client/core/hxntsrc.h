/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxntsrc.h,v 1.56 2008/06/19 22:52:23 ping Exp $
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

#ifndef _HX_NET_SOURCE_
#define _HX_NET_SOURCE_

struct IHXPacket;
struct IHXPendingStatus;
#include "hxpac.h"
#include "pacutil.h"
#include "ihxcookies.h"
#include "ihxcookies2.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxprotocol.h"
#include "hxstrutl.h"
#include "hxbufctl.h"
#include "latency_mode_hlpr.h"

typedef enum
{
    NETSRC_READY,
    // PAC
    NETSRC_PACREADY,
    NETSRC_PACPENDING,
    // Preferred Transport
    NETSRC_TRANSPORTREADY,
    NETSRC_TRANSPORTPENDING,
    // Reconnect
    NETSRC_RECONNECTSTARTED,
    NETSRC_RECONNECTPENDING,
    NETSRC_RECONNECTFORCED,
    // Redirect
    NETSRC_REDIRECTSTARTED,
    NETSRC_REDIRECTPENDING,
    NETSRC_REDIRECTFAILED,
    // Source End
    NETSRC_ENDED,
    NETSRC_ENDPENDING
} NetSourceState;

class ReconnectCallback;

class HXNetSource :   public HXSource
		     ,public IHXPreferredTransportSink
		     ,public IHXProxyAutoConfigCallback
{
public:
					HXNetSource(void);

	STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

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
	STDMETHOD(GetID)	(THIS_
				REF(UINT32) /*OUT*/  ulRegistryID);

	/************************************************************************
	 *	Method:
	 *	    IHXInfoLogger::LogInformation
	 *	Purpose:
	 *	    Logs any user defined information in form of action and 
	 *	    associated data.
	 */
	STDMETHOD(LogInformation)	    (THIS_				
					    const char* /*IN*/ pAction,
					    const char* /*IN*/ pData);

	/*
	 * IHXPreferredTransportSink methods
	 */
	STDMETHOD(TransportSucceeded)	    (TransportMode			/* IN */   prefTransportType,
					    UINT16				/* IN */   uCloakPort);

	STDMETHOD(TransportFailed)	    (THIS);

	STDMETHOD(TransportAborted)     (THIS);


	HX_RESULT Setup			(const char*	host, 
					 const char*	resource,
					 UINT16 	port, 
					 HXBOOL 	LossCorrection,
					 const CHXURL*	pURL,
					 HXBOOL		bAltURL);

	HX_RESULT GetCurrentBuffering  (UINT16	uStreamNumber,
                                        UINT32	&ulLowestTimestamp, 
                                        UINT32	&ulHighestTimestamp,
                                        UINT32	&ulNumBytes,
                                        HXBOOL	&bDone);


	/*
	 *  IHXProxyAutoConfigCallback methods
	 */
	/************************************************************************
	*  Method:
	*      IHXProxyAutoConfigCallback::GetProxyInfoDone
	*  Purpose:
	*/
	STDMETHOD(GetProxyInfoDone)	(THIS_
					HX_RESULT   status,
					char*	    pszProxyInfo);

				HXNetSource(HXPlayer* player);

        virtual HX_RESULT	DoCleanup(EndCode endCode = END_STOP);

	virtual HX_RESULT	DoSeek(ULONG32 seekTime);
	
	virtual HX_RESULT	DoPause(void);

	virtual HX_RESULT	DoResume(UINT32 ulLoopEntryTime = 0,
					 UINT32 ulProcessingTimeAllowance = 0);

	virtual HX_RESULT	StartInitialization(void);
	virtual HX_RESULT StopInitialization(void);

	virtual UINT16		GetNumStreams(void);

	virtual	HX_RESULT	GetStreamInfo(ULONG32 ulStreamNumber,
					      STREAM_INFO*& theStreamInfo);

	virtual HX_RESULT	GetEvent(UINT16 usStreamNumber, 
					 CHXEvent* &theEvent, 
					 UINT32 ulLoopEntryTime,
					 UINT32 ulProcessingTimeAllowance);

////// Interface to the Protocol object...
	virtual HX_RESULT	FileHeaderReady(IHXValues* pHeader);

	virtual HX_RESULT	TACInfoFromHeader(IHXValues* pValues);

        HXBOOL CanSendToDataCallback(IHXPacket* packet) const;
	virtual	HX_RESULT	DataCallback(IHXPacket* packet);

	virtual HX_RESULT	HeaderCallback(IHXValues* header);

	// tell about end of source...
	virtual void		SetEndOfClip(HXBOOL bForcedEndofClip = FALSE);
	
	// The protocol object needs to know the current playback time
	// for a retry()
	virtual ULONG32		GetCurrentPlayTime(void);
	
	// set various options received from server...
	virtual HX_RESULT	SetOption(UINT16 option, void* value);

	virtual HX_RESULT	SetCookie(IHXBuffer* pCookie);
	
	// for auto transport switch... protocol object needs to tell
	// what protocol are we actually using to get data...
	virtual HX_RESULT	TransportStarted(TransportMode mode);
	
	virtual HX_RESULT	HandleRetry(const char* pszHost, UINT16 ulPort);
	
	virtual	HXBOOL		CheckTransportTimeout(ULONG32 ulTime);
////// End of Interface to the Protocol object...

	virtual HXBOOL		IsLocalSource() { return FALSE; };
	virtual HXBOOL		IsPNAProtocol()  {return (HXBOOL) (!m_bRTSPProtocol);};
	virtual HXBOOL		IsNetworkAccess()  { return TRUE; };

	virtual	void		StartDataWait(HXBOOL bConnectionWait = FALSE);
	virtual void		StopDataWait();
	virtual	void		FirstDataArrives();

		HX_RESULT	SetRedirectURL(const char* pHost, UINT16 nPort, const char* pPath, CHXURL* pURL);
		HX_RESULT	SetReconnectInfo(IHXValues* pValues);

	        HXBOOL		IsSourceDone(void);

		void		EnterBufferedPlay(void);
		void		LeaveBufferedPlay(void);

		void		statistics_cat(char* stats,LONG32 Data);

		void		Initialize();

		void		SetAuthenicationInfo(const char* pszUserName, const char* pszPassword)
				{
				    memset(m_szUserName, 0, MAX_DISPLAY_NAME);
				    memset(m_szPassword, 0, MAX_DISPLAY_NAME);

				    if (pszUserName) SafeStrCpy(m_szUserName, pszUserName, MAX_DISPLAY_NAME);
				    if (pszPassword) SafeStrCpy(m_szPassword, pszPassword, MAX_DISPLAY_NAME);
				};

		void		GetAuthenicationInfo(char** pszUserName, char** pszPassword)
	    			{
				    *pszUserName = &m_szUserName[0];
				    *pszPassword = &m_szPassword[0];

				    m_bResendAuthenticationInfo = FALSE;
				};
			
		HXBOOL		IsAuthenticationInfoResended(void)
				{
				    return m_bResendAuthenticationInfo;
				};


		void		AdjustClipBandwidthStats(HXBOOL bActivate = FALSE);

                HXBOOL		CanBeResumed(void);

                void            InitABD(IHXAutoBWDetection* pABD);
                void            ShutdownABD(IHXAutoBWDetection* pABD);

	virtual HX_RESULT	UpdateRegistry(UINT32 ulRegistryID);
	virtual void		LeavePrefetch(void);	
	virtual HXBOOL		IsPrefetchDone(void) { return !m_bPrefetch; };

	virtual void		EnterFastStart(void);
	virtual void		LeaveFastStart(TurboPlayOffReason leftReason);
        virtual HXBOOL          IsRateAdaptationUsed(void);

	virtual HX_RESULT	FillRecordControl(UINT32 ulLoopEntryTime = 0);

        virtual HX_RESULT       OnTimeSync(ULONG32 ulCurrentTime);

        HX_RESULT HandleStreamDone(HX_RESULT status, UINT16 usStreamNumber);

    virtual HX_RESULT   ProcessFileHeader(void);
    virtual HX_RESULT   ProcessStreamHeaders(IHXValues* pHeader, STREAM_INFO*& pStreamInfo);

protected:

	virtual 			~HXNetSource(void);


	HX_RESULT	UpdateStatistics(void);

	HX_RESULT	_ProcessIdle(HXBOOL atInterrupt = 0, 
				     UINT32 ulLoopEntryTime = 0,
				     UINT32 ulProcessingTimeAllowance = 0);
	
	virtual HX_RESULT   _ProcessIdleExt(HXBOOL atInterrupt = 0);
	virtual HX_RESULT   PreFileHeaderReadyExt(IHXValues* pHeader);
	virtual HX_RESULT   PostFileHeaderReadyExt(void);
	virtual HX_RESULT   PreHeaderCallbackExt(IHXValues* theHeader);
	virtual HX_RESULT   PostHeaderCallbackExt(IHXValues* theHeader);

    HX_RESULT	    ReadPreferences(void);

	HX_RESULT	    CreateProtocol();

	HX_RESULT	InitializeProtocol(void);

	HX_RESULT	set_proxy(const char* proxy,UINT16 port);
	HX_RESULT	cleanup_proxy(void);

	HX_RESULT	_Initialize(void);

	void		CalculateCurrentBuffering(void);

        void		ReBuffer(UINT32 ulLoopEntryTime = 0,
				 UINT32 ulProcessingTimeAllowance = 0);

        virtual HXBOOL    ShouldDisableFastStart(void);

	void		ResetASMSource(void);

	HXBOOL		IsInCloakPortList(UINT16 uPort);
	HXBOOL		IsPrefetchEnded(void);

	HX_RESULT	GetEventFromProtocol(UINT16 usStreamNumber, STREAM_INFO* pStreamInfo, CHXEvent*& theEvent);
	HX_RESULT	GetEventFromRecordControl(UINT16 usStreamNumber, 
						  STREAM_INFO* pSteamInfo, 
						  CHXEvent*& theEvent,
						  HXBOOL bForce = FALSE,
						  UINT32 ulLoopEntryTime = 0,
						  UINT32 ulProcessingTimeAllowance = 0);
        HX_RESULT       HandleOutOfPackets(STREAM_INFO* pStreamInfo);
	
        HX_RESULT       UpdateEvent(UINT16 usStreamNumber, CHXEvent*& theEvent);

        void            enforceLatencyThreshold();
        void            setPacketDelays();
        virtual ULONG32 ComputeMaxLatencyThreshold(ULONG32 ulPrerollInMs, ULONG32 ulPostDecodeDelay);

        TransportMode   GetTransportFromURL(char* pszURL);

	// stream thinning
	HXBOOL		mServerSelRecordSupport;
	HXBOOL		mInterframeControlSupport;
	HXBOOL		mServerHasBandwidthReport;
	HXBOOL		mServerHasFrameControl;
	
	// setup parameters...
	char*		m_pHost;
	char*		m_pPath;
	char*		m_pResource;
	UINT16		m_uPort;

	// proxy stuff
	char*		m_pProxy;	// host string of proxy
	UINT16		m_uProxyPort; // host port of proxy
	HXBOOL		m_bUseProxy; // == 1, connect to proxy first
	
	char		mClientID[64]; /* Flawfinder: ignore */
	CHXSimpleList*	m_pUDPPortList;

        CHXSimpleList*  m_pPostRedirectHeaderList;

	HXProtocol*	m_pProto;

	ULONG32		m_ulStartBuffering;

	ULONG32		m_ulServerTimeOut;
	ULONG32		m_ulConnectionTimeout;

	float		m_fReBufferPercent;

	// level 1, 2 stats
	// level 3 stats
	CHXSimpleList*	m_pLogInfoList;
	UINT32		m_ulLogInfoLength;

	// authenication info.
	char		m_szUserName[MAX_DISPLAY_NAME]; /* Flawfinder: ignore */
	char		m_szPassword[MAX_DISPLAY_NAME]; /* Flawfinder: ignore */
	ULONG32		m_ulPerfectPlayDownloadTime;

	// reconnect information
	HXBOOL		m_bAttemptReconnect;
	char*		m_pszReconnectServer;
	char*		m_pszReconnectProxy;
	char*		m_pszReconnectURL;
	UINT32		m_ulReconnectProxyPort;
	UINT32		m_ulReconnectServerPort;

	// redirect information
	char*		m_pszRedirectServer;
	char*		m_pszRedirectResource;
	UINT32		m_ulRedirectServerPort;
        HXBOOL            m_bRedirectInSMIL;

	IHXPendingStatus*	m_pProtocolStatus;
	IHXCookies*		m_pCookies;
	IHXCookies2*		m_pCookies2;

//////////////////////////////////////////////
// Automatic Transport Switching Support...
public:
        ULONG32		m_ulUDPTimeout;

protected:
        INT32           m_lPacketTimeOffSet;
	ULONG32		m_ulMulticastTimeout;
	UINT32		m_ulTCPTimeout;
	ULONG32		m_ulSendStatsMask;
	ULONG32		m_ulStatsInterval;
	UINT32		m_ulSeekPendingTime;
	UINT32		m_ulTransportPrefMask;
        UINT16          m_uProtocolType;
	TransportMode	m_PreferredTransport;
	TransportMode	m_CurrentTransport;

	HX_BITFIELD	m_bLossCorrection : 1;
	HX_BITFIELD	m_bAltURL : 1;
	HX_BITFIELD	m_bRTSPProtocol : 1;	
	HX_BITFIELD	m_bDataWaitStarted : 1;
	HX_BITFIELD	m_bConnectionWait : 1;
	HXBOOL	        m_bSendStatistics;
	HXBOOL    	m_bUseUDPPort;
	HX_BITFIELD	m_bResendAuthenticationInfo : 1;
	HX_BITFIELD	m_bTimeBased : 1;
	HX_BITFIELD	m_bUserHasCalledResume : 1;
	HX_BITFIELD	m_bUserHasCalledStartInit : 1;
	HX_BITFIELD	m_bAtInterrupt : 1;
	HX_BITFIELD	m_bBruteForceReconnected : 1;
	HX_BITFIELD	m_bBruteForceConnectToBeDone : 1;
	HX_BITFIELD	m_bReconnect : 1;
        CHXLatencyModeHelper m_latencyModeHlpr;
	HX_BITFIELD	m_bPerfectPlayPreferenceRead : 1;
	HXBOOL    	m_bPerfectPlayErrorChecked;	
	HX_BITFIELD	m_bServerHasPerfectPlay : 1;
	HX_BITFIELD	m_bServerHasResend : 1;
	HX_BITFIELD	m_bInRetryMode : 1;
	HX_BITFIELD	m_bPushDownSet : 1;

	HXBOOL		m_bForcePerfectPlay;
	HXBOOL		m_bServerHasTransportSwitching;
	HXBOOL		m_bSeekPending;
        HXBOOL          m_bTransportPrefFromURLAttempted;
	UINT16*		m_pCloakPortList;
	UINT8		m_nNumberOfCloakPorts;
	UINT8		m_nCurrPortIdx;
	UINT16		m_uCurrCloakedPort;

        HX_BITFIELD	m_bProtocolPaused : 1;

	PreferredTransportState		m_prefTransportState;
	IHXPreferredTransport*		m_pPreferredTransport;
	IHXPreferredTransportManager*	m_pPreferredTransportManager;
        IHXConnectionBWInfo*            m_pConnBWInfo;

	ReconnectCallback*		m_pReconnectCallback;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
	ReconnectCallback*		m_pStatsCallback;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

	// PAC
	IHXProxyAutoConfig*		m_pPAC;
	CHXSimpleList*			m_pPACInfoList;
	LISTPOSITION			m_PACInfoPosition;

	NetSourceState			m_state;
        UINT32                          m_ulRecordSourceOverrunProtectionTime;
        HXBOOL                          m_bDisableRecordSourceOverrunProtection;

	friend class ReconnectCallback;

private:
        IHXBufferControl*                m_pBufferCtl;
        IHXWatermarkBufferControl*       m_pWMBufferCtl;
public:
	HX_RESULT	FinishSetup();
	void		ReSetup();
	void		Reset();
    
	HX_RESULT	switch_out_of_perfectplay();

	HX_RESULT	handleTransportSwitch(void);
	HX_RESULT	handleProxySwitch(HX_RESULT inError);
	HX_RESULT	handleRedirect(void);
	HX_RESULT	handleReconnect(void);
	HX_RESULT       handleEndOfSource(void);
	
	HXBOOL		IsNetworkAvailable(void);
	HX_RESULT	AttemptReconnect(void);
	HX_RESULT	StartReconnect(void);
	HX_RESULT	ReportStats(void);
	HX_RESULT	ProcessReconnect(STREAM_INFO* pStreamInfo);
	HX_RESULT	EndReconnect(void);
	HX_RESULT	AddToPreReconnectEventList(STREAM_INFO* pStreamInfo, CHXEvent* pEvent);
	void		UpdateReconnectInfo(UINT32	ulPacketTime, 
					    HXBOOL&	bFirstEvent,
					    UINT32&	ulPrevPacketTime,
					    UINT32&	ulLargestGapInPacketTime,
					    UINT32&	ulLastPacketTime);

        HX_RESULT       AttemptRedirect(void);
        HXBOOL            IsRedirectInSMIL(void);
        HXBOOL            IsRedirectedOK(void);        

	char*		GetHost() {return m_pHost;};

	INT32		GetRAStreamNumber()
			{
			    return m_lRAStreamNumber;
			};

	UINT32		GetFirstDataArriveTime()
			{
			    return m_ulFirstDataArrives;
			};

	UINT32		GetLogInfo(CHXSimpleList*& pLogInfoList)
			{
			    pLogInfoList = m_pLogInfoList;
			    return m_ulLogInfoLength;
			};

	void		ReportError (HX_RESULT theErr);
        void            ActualReportError(HX_RESULT theErr);

	HX_RESULT	switch_to_next_transport(HX_RESULT incomingError);
	void		set_transport(TransportMode mode);

	void		WritePerfectPlayToRegistry();
	void		CreateCloakedPortList();

};

class ReconnectCallback : public IHXCallback
{
public:
    HXNetSource*	m_pSource;
    CallbackHandle	m_PendingHandle;
    HXBOOL		m_bIsStatsReportingCallback;
    IHXScheduler*	m_pScheduler;
    UINT32		m_ulScheduleTime;
    UINT32		m_ulTimeout;
    HXBOOL		m_bPaused;

			ReconnectCallback(HXNetSource*	pSource, 
					  HXBOOL bIsStatsCallback = FALSE);
    HX_RESULT		ScheduleCallback(UINT32 uTimeout);
    HX_RESULT		PauseCallback();
    HX_RESULT		ResumeCallback();
    HX_RESULT		CancelCallback();
    HXBOOL		IsPaused() {return m_bPaused;};

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
			~ReconnectCallback();

    LONG32		m_lRefCount;
};
#endif // _HX_NET_SOURCE


