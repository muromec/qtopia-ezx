/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprotocol.h,v 1.15 2007/07/06 21:58:11 jfinnecy Exp $
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

#ifndef _HXPROTOCOL_H_
#define _HXPROTOCOL_H_

#include "hxcom.h"
#include "hxresult.h"
#include "hxmon.h"
#include "statinfo.h"
#include "hxstring.h"
//#include "rmresend.h"
#include "hxpreftr.h"
#include "hxcredc.h"

// forward decl.
class HXNetSource;
class CHXSimpleList;

_INTERFACE IHXPendingStatus;
_INTERFACE IHXStatistics;
_INTERFACE IHXSockAddr;

class  CHXEvent;

typedef struct _HX_BANDWIDTH_REPORT
{
    UINT16 	serverTimeInterval;
    UINT16 	playerTimeInterval;
    ULONG32 	numServerBytes;
    ULONG32 	numPlayerBytes;
    ULONG32 	totalServerBytes;
    ULONG32 	totalPlayerBytes;
    ULONG32 	totalServerTime;
    ULONG32 	totalPlayerTime;
} HX_BANDWIDTH_REPORT;

// these are the values passed in SetOption
enum
{
    HX_PERFECTPLAY_SUPPORTED = 0
   ,HX_RESEND_SUPPORTED
   ,HX_STATS_MASK
   ,HX_TRANSPORTSWITCHING_SUPPORTED
   ,HX_FORCE_PERFECT_PLAY
   ,HX_SELECTIVE_RECORD_SUPPORTED
   ,HX_GENERIC_MESSAGE_SUPPORT
   ,HX_INTERFRAME_CONTROL_SUPPORT
   ,HX_BANDWIDTH_REPORT_SUPPORT
   ,HX_FRAME_CONTROL_SUPPORT
   ,HX_STATS_INTERVAL
   ,HX_MAX_BANDWIDTH
   ,HX_TURBO_PLAY
};

#define MIN_UDP_PORT	6970
#define MAX_UDP_PORT	7170

#define TEXT_BUF_SIZE	1024

class HXProtocol : public IHXPendingStatus
#if defined(HELIX_FEATURE_STATS)
		   , public IHXStatistics
#endif /* HELIX_FEATURE_STATS */
{
public:
    UINT32		m_ulRegistryID;
    IUnknown*		m_pContext;
    IHXRegistry*	m_pRegistry;
    
    HXProtocol(HXNetSource *owner, ULONG32 ulPlatformSpecific = 0);
    virtual ~HXProtocol();

    LONG32	m_lRefCount;

    // *** IUnknown methods ***
    /* This is made PURE since PNA and RTSP may expose different interfaces...*/
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;
   
    // functions that *MUST* be implemented by every protocol
    
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
				REF(UINT16) ulPercentDone) PURE;

#if defined(HELIX_FEATURE_STATS)
    /*
     *	IHXStatistics methods
     */

    /************************************************************************
     *  Method:
     *      IHXStatistics::InitializeStatistics
     *  Purpose:
     *      Pass registry ID to the caller
     */
    STDMETHOD (InitializeStatistics)	(THIS_
					UINT32	/*IN*/ ulRegistryID) PURE;

    /************************************************************************
     *  Method:
     *      IHXStatistics::UpdateStatistics
     *  Purpose:
     *      Notify the client to update its statistics stored in the registry
     */
    STDMETHOD (UpdateStatistics)		(THIS) PURE;
#endif /* HELIX_FEATURE_STATS */

    virtual HX_RESULT	GetStreamStatistics	(ULONG32 ulStreamNumber,
						 STREAM_STATS** ppStreamStats) = 0;
    virtual HX_RESULT	UpdateRegistry		(UINT32 ulStreamNumber,
						 UINT32 ulRegistryID) = 0;

    virtual HX_RESULT	server_hello		(void) = 0;

    virtual HX_RESULT	proxy_hello		(void) = 0;

    virtual HX_RESULT	process 		(void) = 0;

    virtual HX_RESULT	abort	 		(void) = 0;

    virtual HX_RESULT   GetEvent		(UINT16 usStreamNumber,
						 CHXEvent*& pEvent) = 0;

    virtual HX_RESULT	GetCurrentBuffering(UINT16  uStreamNumber,
					    UINT32& ulLowestTimestamp, 
					    UINT32& ulHighestTimestamp,
					    UINT32& ulNumBytes,
					    HXBOOL& bDone) = 0;
    // functions that should be overridden
    // should be made abstract later on if we decide that these are must
    // for every protocol to implement.

    virtual HX_RESULT	setup			(const char* host,
						 const char* path,
						 UINT16 port,
						 HXBOOL	LossCorrection,
						 HXBOOL	bHTTPCloak,
                                                 HXBOOL   bSDPInitiated,
						 UINT16	cloakPort);

    virtual void	initialize_members	(void);

    virtual HX_RESULT	process_idle		(HXBOOL atInterrupt);

    virtual HX_RESULT	seek			(ULONG32 posArg,
						 ULONG32 posArg2 = 0,
						 UINT16 seekFrom = 0)
						{return HXR_OK;}

    virtual HX_RESULT	pause 			(void)
						{return HXR_OK;}

    virtual HX_RESULT	resume			(UINT32 ulEndTime = 0)
						{return HXR_OK;}

    virtual HX_RESULT	stop 			(void);

    virtual HX_RESULT	set_proxy		(const char* proxy,
						 UINT16 port);

    virtual void	send_statistics		(UINT32	ulStatsMask) {};

    virtual HXBOOL	end_of_clip		(void)
						{return mSourceEnd;}

    virtual UINT16	get_protocol_version	(void)
						{return mProtocolVersion;}

    virtual const char* get_protocol_name	(void)
						{return "";}

    virtual HX_RESULT	set_client_id 		(char * clientID);

    virtual void	set_locale		(UINT16 locale)
						{mLocale = locale;}

    virtual void	set_perfect_play	(HXBOOL isPerfectPlay)
						{m_bPerfectPlay = isPerfectPlay;}

// NOTE: set_UDP_port() now means, if in UDP mode,
// use the specified port for UDP. It doesn't necessarily
// mean to use UDP mode...
    virtual void 	set_UDP_port		(void) 
						{mUseUDPPort = TRUE;}

    virtual void	set_server_timeout	(ULONG32 secs)
						{mServerTimeout = secs;}

    virtual HXBOOL	IsPerfectPlayAllowed	(void)
						{return m_bPerfectPlayAllowed;}

    virtual HXBOOL	IsSaveAllowed		(void)
						{return mSaveAsAllowed;}

    virtual HXBOOL	IsLive			(void)
						{return mLiveStream;}

    virtual const char* GetLastAlertInfo	(REF(UINT32) ulAlertNumber)
						{ulAlertNumber = m_ulLastAlert; return m_pTextBuf;}

    virtual HXBOOL	IsSourceDone(void) = 0;

    virtual void	EnterPrefetch		(void) {m_bPrefetch = TRUE;};
    virtual void	LeavePrefetch		(void);

    virtual void	EnterFastStart		(void) {m_bFastStart = TRUE;};
    virtual void	LeaveFastStart		(void) {m_bFastStart = FALSE;};

    virtual void	SetCloakPortAttempted	(UINT16* pCloakPorts, UINT8 nCloakPorts);

    virtual UINT16	GetRDTFeatureLevel	(void) { return 0; };

    virtual UINT32      GetServerVersion        (void) { return m_ulServerVersion; };

    virtual HX_RESULT   AutoBWCalibrationDone   (HX_RESULT  status, 
                                                 UINT32     ulBW) { return HXR_OK; };
    virtual HXBOOL      IsRateAdaptationUsed    (void) { return FALSE; }
protected:
    
    UINT8		mProxyVersion : 8;		// protocol version for proxy
    // flags
    HX_BITFIELD		mLiveStream : 1;
    HX_BITFIELD		mSaveAsAllowed : 1;
    HX_BITFIELD		m_bPerfectPlayAllowed : 1;
    HX_BITFIELD		m_bPrefetch : 1;
    HX_BITFIELD		m_bFastStart : 1;

    HX_BITFIELD		m_bIsFirstResume : 1;
    HX_BITFIELD		mProtocolValid : 1;		// does server support protocol
    HX_BITFIELD		m_bConnectDone : 1;
    HX_BITFIELD		mSourceEnd : 1;		// == 1 no more data
    HX_BITFIELD		mUseUDPPort : 1;		// user specified udp port
    HX_BITFIELD		mFlowControl : 1;		// stop server sending data
    HX_BITFIELD		m_bPerfectPlay : 1;		// == 1 perfect play mode
    HX_BITFIELD		mUseProxy : 1;		// == 1 use proxy
    HX_BITFIELD		mUsingMulticast : 1;
    HX_BITFIELD		m_bHTTPOnly : 1;
    HX_BITFIELD		m_bPaused : 1;		// == 1 pause state
    HX_BITFIELD		mLossCorrection : 1;	// == 1 loss correction is on
    HX_BITFIELD		m_bAreResuming : 1;
    HX_BITFIELD         m_bSDPInitiated : 1;
    HXBOOL		m_bHTTPvProxy;
    UINT16		mProtocolVersion;	// server/client protocol number
    UINT16		mAtInterrupt;		// == 1 at interrupt level
    UINT16		mLocked;		// semaphore for reentracy
    UINT16		mLocale;		// stores player id ??
    UINT16		mServerPort;		// server control port
    HXNetSource*	mOwner;			// ptr to HXNetSource object
    CHXString		mHost;			// server host name
    CHXString		mPath;			// remote file path to play
    CHXSimpleList*	m_pUDPPortList;
    UINT16		m_uUDPPort;
    ULONG32		mSendStatsMask;		// stats level sent to server
    CHXString		mProxy;			// proxy host name
    UINT16		mProxyPort;		// proxy port number
    UINT16		mCloakPort;
    INT16		mNumFlowControl;	// flow control msgs from owner
    ULONG32		mServerTimeout;		// timeout value for server
    HX_RESULT		m_LastError;
    IHXPreferences*	m_pPreferences;
    IHXCredentialsCache*    m_pCredentialsCache;

    UINT32              m_ulServerVersion;
    UINT32		m_ulLastAlert;
    char*		m_pTextBuf;

    UINT16*		m_pCloakPorts;
    UINT8		m_nCloakPorts;

    // ID info
    CHXString		m_clientID;		// string to hold clientID
    CHXString		m_guid;	        	// string to hold GUID

   
//////////////////////////////////////////////
// Automatic Transport Switching Support...
protected:
    TransportMode	mCurrentTransport;
    UINT32		m_ulTransportPrefMask;
public:
    virtual void	set_transport(TransportMode mode, UINT32 ulTransportPrefMask)
    {
	if (HTTPCloakMode == mode)
	{
	    mCurrentTransport = TCPMode;
	}
	else
	{
	    mCurrentTransport = mode;
	}

	m_ulTransportPrefMask = ulTransportPrefMask;
    };

    HXBOOL		can_switch_transport()
    {
	return FALSE;
    };

    HX_RESULT		switch_transport(TransportMode mode)
    {
	return HXR_INVALID_OPERATION;
    };

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    void		statistics_cat(char* pszStats, UINT32 ulBufLen, LONG32 lData);
    void		statistics_cat_ext(char* pszStats, UINT32 ulBufLen, LONG32 lData, char* pszSep, UINT32& ulCount);
    HX_RESULT		prepare_statistics(UINT32 ulStatsMask, char*& pszStats);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
};

#endif //_HXPROTOCOL_H_
