/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspprotocol.h,v 1.22 2007/01/11 19:53:31 milko Exp $
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


#ifndef _RTSPPROTOCOL_H_
#define _RTSPPROTOCOL_H_

#include "hxathsp.h"
#include "hxpktsp.h"
#include "hxsmbw.h"

struct	IHXPacket;
struct	IHXValues;
class	CHXEvent;
class	DataRevertController;
class	RTSPClientProtocol;

class RTSPProtocol : public HXProtocol, 
		     public IHXRTSPClientProtocolResponse,
		     public IHXASMSource,
		     public IHXBackChannel,
#if defined(HELIX_FEATURE_AUTHENTICATION)
		     public IHXClientAuthResponse,
		     public IHXAuthenticationManagerResponse,
#endif /* HELIX_FEATURE_AUTHENTICATION */
		     public IHXAtomicRuleChange,
		     public DataRevertControllerResponse
{
public:
			RTSPProtocol  (HXNetSource *owner, 
				       ULONG32 ulPlatformSpecific = 0 );
			~RTSPProtocol ();
    /*
     *	IUnknown methods
     */

    STDMETHOD(QueryInterface)		(THIS_
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
					UINT32	/*IN*/ ulRegistryID);

    /************************************************************************
     *  Method:
     *      IHXStatistics::UpdateStatistics
     *  Purpose:
     *      Notify the client to update its statistics stored in the registry
     */
    STDMETHOD (UpdateStatistics)		(THIS);
#endif /* HELIX_FEATURE_STATS */
    
    virtual HX_RESULT	GetStreamStatistics	(ULONG32 ulStreamNumber,
						 STREAM_STATS** ppStreamStats);

    virtual HX_RESULT	UpdateRegistry		(UINT32 ulStreamNumber,
						 UINT32 ulRegistryID);

    /*
     *	IHXRTSPClientProtocolResponse methods
     */

    STDMETHOD(InitDone)			(THIS_
    					HX_RESULT	status);

    STDMETHOD(HandleOptionsResponse)	
    (
	THIS_
	HX_RESULT	status,
	IHXValues*	pHeader
    );

    STDMETHOD(HandleWWWAuthentication)	(THIS_
					HX_RESULT	status,
					IHXValues*	AuthInfo);

#if defined(HELIX_FEATURE_AUTHENTICATION)
    /*
     * IHXAuthenticationManagerResponse methods
     */
    STDMETHOD(AuthenticationRequestDone) (THIS_ HX_RESULT,
					  const char*,
					  const char*);

    // IHXClientAuthResponse
    STDMETHOD(ResponseReady)
    (
	HX_RESULT   HX_RESULTStatus,
	IHXRequest* pIHXRequestResponse
    );
#endif /* HELIX_FEATURE_AUTHENTICATION */

    STDMETHOD(HandleStreamDescriptionResponse)	
    (
	THIS_
	HX_RESULT	status,
	IHXValues*	pFileHeader,
	CHXSimpleList*	pHeaderList,
	IHXValues*	pResponseHeaders
    );

    STDMETHOD(HandleStreamRecordDescriptionResponse)	
					(THIS_
					HX_RESULT	status,
					IHXValues*	pResponseHeaders);

    STDMETHOD(HandleSetupResponse)	(THIS_
					HX_RESULT	status);

    STDMETHOD(HandlePlayResponse)	(THIS_
					HX_RESULT	status);

    STDMETHOD(HandleRecordResponse)	(THIS_
					HX_RESULT	status);

    STDMETHOD(HandleTeardownResponse)	(THIS_
					HX_RESULT	status);

    STDMETHOD(HandleSetParameterRequest)
					(THIS_
					UINT32		lParamType,
					const char*	pParamName,
					IHXBuffer*	pParamValue);
    						
    STDMETHOD(HandleSetParameterRequest) (THIS_
	    				 const char* pParamName,
					 const char* pParamValue,
					 const char* pContent);

    STDMETHOD(HandleSetParameterResponse)
					(THIS_
    					HX_RESULT	status);

    STDMETHOD(HandleSetParameterResponseWithValues)	(THIS_
    						HX_RESULT status,
						IHXValues* pValues
						);

    STDMETHOD(HandleGetParameterRequest)
					(THIS_
					UINT32		lParamType,
					const char*	pParamName,
					IHXBuffer**	pParamValue);

    STDMETHOD(HandleGetParameterResponse)
					(THIS_
					HX_RESULT	status,
					IHXBuffer*	pParamValue);

    STDMETHOD(HandleAlertRequest)	(THIS_
					HX_RESULT	status,
					INT32		lAlertNumber,
					const char*	pAlertText);

    STDMETHOD(HandleRedirectRequest)	(THIS_
    					const char*	pURL,
					UINT32		msFromNow);

    STDMETHOD(HandleUseProxyRequest)	(THIS_
					const char*	pProxyURL);

    STDMETHOD(HandlePacket)		(THIS_
    					HX_RESULT	status,
					const char*	pSessionID,
					IHXPacket*	pPacket);

    STDMETHOD(HandleProtocolError)	(THIS_
    					HX_RESULT	status);

    STDMETHOD(HandleRTTResponse)	(THIS_
    					HX_RESULT	status,
					const char*	pSessionID,
					UINT32		ulSecs,
					UINT32		ulUSecs);

    STDMETHOD(HandleCongestion)		(THIS_
    					HX_RESULT	status,
					const char*	pSessionID,
					INT32		xmitMultiplier,
					INT32		recvMultiplier);

    STDMETHOD(HandleStreamDone)		(THIS_
    					HX_RESULT	status,
					UINT16		uStreamNumber);

    /* This only indicates that all packets have been received from the
     * server. We still need to read packets from the transport buffer
     * StreamDone will indicate when there are no more packets to be
     * read from Transport buffer
     */
    STDMETHOD(HandleSourceDone)		(THIS);

    STDMETHOD(HandlePrerollChange)	(THIS_ 
					 RTSPPrerollTypeEnum prerollType,
					 UINT32 ulPreroll
	                                ); 

    /*
     *  IHXASMSource methods
     */

    STDMETHOD(Subscribe)		(THIS_
    					UINT16		streamNumber,
					UINT16		ruleNumber);

    STDMETHOD(Unsubscribe)		(THIS_
    					UINT16		streamNumber,
					UINT16		ruleNumber);

    /*
     *  IHXAtomicRuleChange method
     */

    STDMETHOD(RuleChange)		(THIS_
    					REF(CHXSimpleList) RuleChanges);

    /*
     * IHXBackChannel methods
     */
    
    STDMETHOD(PacketReady)		(THIS_
    					IHXPacket*	pPacket);

    /*
     *	HXProtocol methods
     */

    virtual HX_RESULT	server_hello		(void);

    virtual HX_RESULT	proxy_hello		(void);

    virtual HX_RESULT	process			(void);

    virtual HX_RESULT	abort			(void);

    virtual HX_RESULT   GetEvent		(UINT16 uStreamNumber,
						 CHXEvent*& pEvent);

    virtual HX_RESULT	GetCurrentBuffering(UINT16  uStreamNumber,
					    UINT32& ulLowestTimestamp, 
					    UINT32& ulHighestTimestamp,
					    UINT32& ulNumBytes,
					    HXBOOL& bDone);

    virtual HX_RESULT	setup			(const char* host,
						 const char* path,
						 UINT16 port,
						 HXBOOL	LossCorrection,
						 HXBOOL	bHTTPCloak,
                                                 HXBOOL   bSDPInitiated,
						 UINT16	cloakPort);

    virtual void	initialize_members	(void);

    virtual HX_RESULT	seek			(ULONG32 posArg,
						 ULONG32 posArg2 = 0,
						 UINT16  seekFrom = 0);

    virtual HX_RESULT	pause			(void);

    virtual HX_RESULT	resume			(UINT32 ulEndTime = 0);

    virtual HX_RESULT	reset			(void) { return HXR_OK; };
						
    virtual HX_RESULT	stop 			(void);

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    virtual void	send_statistics		(UINT32	ulStatsMask);
    HX_RESULT		prepare_stats4		(char*& pszOutput, UINT32& ulOutput);    
    STREAM_STATS*	create_statistics	(UINT16 uStreamNumber);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    virtual HX_RESULT	send_setup_request	();

    HXBOOL		IsSourceDone(void);
    
    const char*		get_protocol_name	(void)
						{return "RTSP";}

#if defined(HELIX_FEATURE_AUTHENTICATION)
    HX_RESULT		handlePendingWWWAuthentication(
				    HX_RESULT HX_RESULTStatus, 
				    IHXValues* pIHXValuesHeaders);
#endif /* HELIX_FEATURE_AUTHENTICATION */

    HX_RESULT		handle_multicast	(void);

    void RevertHeadersDone(IHXValues* pFileHeader,
			   CHXSimpleList* pStreamHeaders, 
			   IHXValues* pResponseHeaders,
			   HXBOOL bUseReverter);
    void SendControlBuffer(IHXBuffer* pBuffer);

    virtual UINT16	GetRDTFeatureLevel	(void);

    virtual void	LeavePrefetch		(void);

    virtual void	EnterFastStart		(void);
    virtual void	LeaveFastStart		(void);

    virtual HXBOOL      IsRateAdaptationUsed   (void);
protected:
    LONG32			m_lRefCount;
    DECLARE_SMART_POINTER
    (
	IHXClientAuthConversation
    )				m_spIHXClientAuthConversationAuthenticator;
    SPIHXRTSPClientProtocol	m_spProtocolLib;
    SPIHXRTSPClientProtocol2	m_spProtocolLib2;
    ULONG32			m_uSecurityKey;
    ULONG32			m_uStreamCount;
    ULONG32			m_uCurrentStreamCount;
    CHXMapLongToObj*		m_pStreamInfoList;
    IHXPendingStatus*		m_pPendingStatus;
    IHXStatistics*		m_pStatistics;
    IHXRequest*		        m_pRequest;
    HX_BITFIELD			m_bPlaying : 1;
    HX_BITFIELD			m_bIsASMSource : 1;
    HXBOOL			m_bUseRTP;
    HX_BITFIELD			m_bFirstAuthAttempt : 1;
    HX_BITFIELD			m_bPendingSeek : 1;
    HX_BITFIELD			m_bHandleWWWAuthentication : 1;

    HX_BITFIELD			mReceivedControl : 1;

    // Smart Networking  
    HX_BITFIELD			m_bReceivedData : 1;
    HX_BITFIELD                 m_bMulticastOnly : 1;

    // ID info
    IHXValues*			m_pIDInfo;


    UINT32			m_ulSeekPos1;
    UINT32			m_ulSeekPos2;

    UINT32                      m_ulLastPacketReceivedTime;
    UINT32                      m_ulSessionInitedTime;

    HX_RESULT			m_WWWResult;
    IHXValues*			m_pWWWValues;


    DECLARE_SMART_POINTER(IHXValues) m_spIHXValuesStoredHeaders;

    enum
    {
	NULL_STATE,
	SEND_SETUP_REQUEST_STATE,
	ALERT_STATE
    } m_idleState;

#if defined(HELIX_FEATURE_REVERTER)
    DataRevertController*   m_pDataRevert;
#endif /* HELIX_FEATURE_REVERTER */

private:
    HXBOOL            m_bSocketsInited;
    CHXString         m_strUserAgent;

    void	    hackCookie(IHXBuffer* pCookie);
    HX_RESULT       SwitchToUnicast(void);

    HX_RESULT       InitSockets(void);

    void destroyProtocolLib();
};

#endif /*_RTSPPROTOCOL__*/
