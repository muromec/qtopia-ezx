/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspprot.h,v 1.41 2007/05/01 18:17:55 darrick Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _RTSPPROT_H_
#define _RTSPPROT_H_

#include "hxtypes.h"
#include "hxcom.h"

#include "source.h"
#include "rtspserv.h"
#include "hxrtsp2.h"

#include "hxclientprofile.h"

#include "sink.h"
#include "source.h"
#include "hxservpause.h"

#include "hxstreamadapt.h"
#include "hxrtspservprot2.h"

class HXProtocol;
class TransportStreamHandler;
class Transport;
class RTSPTransportHack;
class RTSPServerMulticastTransport;
_INTERFACE IHXValues;
_INTERFACE IHXPacketResend;
_INTERFACE IHXRTSPProtocolResponse;
_INTERFACE IHXRTSPServerProtocolResponse2;
_INTERFACE IHXServerRDTTransport;

class RTSPProtocol  : public HXProtocol
                    , public IHXRTSPServerProtocolResponse2
                    , public IHXRTSPProtocolResponse
                    , public IHXRTSPServerPauseResponse
                    , public IHXClientProfileManagerResponse
                    , public IHXRTSPProtocolValuePass
{
public:
    RTSPProtocol(void);
    virtual ~RTSPProtocol(void);

    /*
     * IUnknown methods
     */
    // XXXSMP YIPES!!  Where are all of the THIS_?

    STDMETHOD(QueryInterface)                   (REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)                   ();
    STDMETHOD_(UINT32,Release)                  ();

    /*
     * IHXRTSPServerProtocolResponse2 methods
     */
    STDMETHOD(HandleStreamDescriptionRequest)   (const char* pURL,
                                                IHXValues* pRequestHeaders,
                                                const char* pSessionID,
                                                BOOL bUseRTP);
    STDMETHOD(HandleStreamRecordDescriptionRequest)
                                                (const char* pURL,
                                                const char* pSessionID,
                                                IHXValues* pFileHeader,
                                                CHXSimpleList* pHeaders,
                                                IHXValues* pRequestHeaders);
    STDMETHOD(HandleSetupRequest)               (HX_RESULT status);
    STDMETHOD(HandleSetParameterRequest)        (UINT32 lParamType,
                                                const char* pParamName,
                                                IHXBuffer* pParamValue);
    STDMETHOD(HandleSetParameterRequest)        (const char* pSessionID,
                                                const char* pParamName,
                                                const char* pParamValue,
                                                const char* pContent);
    STDMETHOD(HandleSetParameterResponse)       (HX_RESULT status);
    STDMETHOD(HandleGetParameterRequest)        (UINT32 lParamType,
                                                const char* pParamName,
                                                IHXBuffer** pParamValue);
    STDMETHOD(HandleGetParameterResponse)       (HX_RESULT status,
                                                IHXBuffer* pParamValue);
    STDMETHOD(HandlePlayRequest)                (UINT32 lFrom, UINT32 lTo,
                                                CHXSimpleList* pSubscriptions,
                                                const char* pSessionID);
    STDMETHOD(HandleRecordRequest)              (const char* pSessionID);
    STDMETHOD(HandlePauseRequest)               (const char* pSessionID);
    STDMETHOD(HandleResumeRequest)              (const char* pSessionID);
    STDMETHOD(HandleTeardownResponse)           (HX_RESULT status);
    STDMETHOD(HandleTeardownRequest)            (const char* pSessionID);
    STDMETHOD(HandlePacket)                     (HX_RESULT status,
                                                const char* pSessionID,
                                                IHXPacket* pPacket);
    STDMETHOD(HandleSubscribe)                  (CHXSimpleList* pSubList,
                                                const char* pSessionID);
    STDMETHOD(HandleBackChannel)                (IHXPacket* pPacket,
                                                const char* pSessionID);
    STDMETHOD(HandleUnsubscribe)                (CHXSimpleList* pUnubList,
                                                const char* pSessionID);
    STDMETHOD(HandleSubscriptionDone)           (REF(UINT32) ulAddress,
                                                 REF(UINT32) ulSourcePort,
                                                 REF(UINT32) ulPort,
                                                 const char* pSessionID,
                                                 REF(TransportStreamHandler*) pHandler);
    STDMETHOD(HandleBWReport)                   (HX_RESULT status,
                                                const char* pSessionID,
                                                INT32 aveBandwidth,
                                                INT32 packetLoss,
                                                INT32 bandwidthWanted);
    STDMETHOD(HandlePlayerStats)                (const char* pStats,
                                                const char* pSessionID);
    STDMETHOD(HandleSessionHeaders)             (IHXValues* pSessionHeaders);
    STDMETHOD(HandleProtocolError)              (HX_RESULT status);
    STDMETHOD(AddSession)                       (const char* pURLText,
                                                UINT32 ulSeqNo,
                                                REF(CHXString) sessionID,
                                                BOOL bRetainEntityForSetup);
    STDMETHOD(AddSessionWithID)                 (const char* pURLText,
                                                UINT32 ulSeqNo,
                                                REF(CHXString) sessionID,
                                                BOOL bRetainEntityForSetup);
    STDMETHOD(AddTransport)                     (Transport* pTransport,
                                                const char* pSessionID,
                                                UINT16 uStreamNumber,
                                                UINT32 ulReliability);
    STDMETHOD(SetupTransports)                  (const char* pSessionID);

    STDMETHOD(HandleLimitBandwidthByDropping)   (THIS_
                                                UINT16 streamNumber,
                                                const char* pSessionID,
                                                UINT32 ulBandwidthLimit);

    STDMETHOD(HandleScaleParam)                 (THIS_
                                                 const char* pSessionID,
                                                 FIXED32 fScale);

    STDMETHOD(HandleSpeedParam)                 (THIS_
                                                 const char* pSessionID,
                                                 FIXED32 fSpeed);

    STDMETHOD(SetValues)                        (THIS_
                                                 IHXValues* pValues);

    STDMETHOD(HandleSetDeliveryBandwidth)
                                        (THIS_
                                         UINT32 ulBackOff,
                                         const char* pSessionID,
                                         UINT32 ulBandwidth);

    STDMETHOD(HandleStreamDone)         (THIS_
                                        HX_RESULT status,
                                        UINT16 uStreamNumber);

    STDMETHOD(GenerateNewSessionID)     (THIS_
                                        REF(CHXString) sessionID,
                                        UINT32 ulSeqNo);

    STDMETHOD(HandleRetainEntityForSetup)(THIS_
                                        const char* pSessionID,
                                        BOOL bRequired);

    STDMETHOD(SetMidBox)(THIS_
                                        const char* pSessionID,
                                        BOOL bIsMidBox);

    STDMETHOD (HandleStreamAdaptation) (THIS_
                                       const char* pSessionID,
                                       REF(StreamAdaptationParams) streamAdaptParams,
                                       BOOL bHlxStreamAdaptScheme);

    STDMETHOD (Handle3GPPLinkChar) (THIS_
                                    const char* pSessionID,
                                    REF(LinkCharParams) linkCharParams);


    STDMETHOD(HandleClientAvgBandwidth) (THIS_
                                        const char* pSessionID,
                                        UINT32 ulAvgBandwidth);

    STDMETHOD(HandleStreamSetup) (THIS_
				const char* pSessionID,
				UINT16 uStreamNumber,
				UINT16 uStreamGroupNumber);

    /*
     * XXX: aak - Sun Jul 22 13:18:04 PDT 2001
     * XXX: aak - Thu Aug  9 14:50:55 PDT 2001
     *
     * IHXRTSPProtocolResponse methods which get called
     * by RTSPServerProtocol
     */
    STDMETHOD(OnClosed)             (THIS_ HX_RESULT status)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnError)              (THIS_ HX_RESULT status)
    {
        return HXR_NOTIMPL;
    };

    STDMETHOD(OnConnectDone)        (THIS_ HX_RESULT status)
    {
        return HXR_NOTIMPL;
    }
    STDMETHOD(OnPacket)             (THIS_ IHXRTSPInterleavedPacket* pPkt)
    {
        return HXR_NOTIMPL;
    }

    /*
     * XXX: aak - Fri Jul 20 16:18:26 PDT 2001
     * XXX: aak - Thu Aug  9 14:50:55 PDT 2001
     * right now only integrate: OPTIONS, TEARDOWN
     */
    STDMETHOD(OnOptionsRequest)     (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnDescribeRequest)    (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnSetupRequest)       (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnPlayRequest)        (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnPauseRequest)       (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnTeardownRequest)    (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnSetParamRequest)    (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnGetParamRequest)    (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnAnnounceRequest)    (THIS_ IHXRTSPRequestMessage* pmsg);
    STDMETHOD(OnRecordRequest)      (THIS_ IHXRTSPRequestMessage* pmsg);

    STDMETHOD(OnRedirectRequest)    (THIS_ IHXRTSPRequestMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnExtensionRequest)   (THIS_ IHXRTSPRequestMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };

    STDMETHOD(OnOptionsResponse)    (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnDescribeResponse)   (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnSetupResponse)      (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnPlayResponse)       (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnPauseResponse)      (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnAnnounceResponse)   (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnRecordResponse)     (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnTeardownResponse)   (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnGetParamResponse)   (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnSetParamResponse)   (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnRedirectResponse)   (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };
    STDMETHOD(OnExtensionResponse)  (THIS_ IHXRTSPResponseMessage* pmsg)
    {
        return HXR_NOTIMPL;
    };

    /*
     *  IHXRTSPServerPauseResponse methods
     */
    STDMETHOD(HandlePauseRequest)   (THIS_ const char* pSessionID,
                                           UINT32 ulPausePoint);
    /*
     *  IHXClientProfileManagerResponse methods
     */
    STDMETHOD(PSSProfileReady)      (THIS_ HX_RESULT ulStatus,
                                            IHXClientProfile* pInfo,
                                            IHXBuffer* pRequestId,
                                            IHXBuffer* pRequestURI,
                                            IHXValues* pRequestHeaders);
    /*
     * HXProtocol methods
     */
    void                init(Process* proc, IHXSocket* pSock);
    int                 setupHeader             (IHXValues* pHeader,
                                                Player::Session* pSession,
                                                HX_RESULT status);
    int                 addToHeader             (const char* pName,
                                                IHXBuffer* pValue);
    int                 setupStreams            (CHXSimpleList* pHeaders,
                                                Player::Session* pSession,
                                                HX_RESULT result);
    int                 playDone(const char* pSessionID);
    virtual int         sendAlert(const char* pSessionID, StreamError err);
    virtual int         sendAlert(const char* pSessionID, IHXBuffer* pAlert);
    virtual int         disconnect(const char* pSessionID);
    int                 sendRedirect            (const char* pSessionID,
                                                 const char* pURL,
                                                 UINT32 ulSecsFromNow);
    int                 sendProxyRedirect       (const char* pSessionID,
                                                 const char* pURL);

    const char* versionStr                      ();
    Transport* getTransport           (Player::Session* pSession,
                                                 UINT16 streamNumber,
                                                 UINT32 bIsReliable);
    Transport* getFirstTransportSetup (Player::Session* pSession,
                                                 UINT32 bIsReliable);


    HX_RESULT                                   SendSetParam(const char* url,
                                                const char* pSessionID,
                                                const char* pName,
                                                const char* pValue,
                                                const char* pMimeType = NULL,
                                                const char* pContent = NULL);

    void                SessionDone             (const char* sessionID);

    void        SetScaleDone    (HX_RESULT status, Player::Session* pSession,
                                                 FIXED32 fScale);
    void        SetStatus(UINT32 ulCode);

    /*
     * needed for now
     */
    void sendPacket                             (BasePacket* pPacket,
                                                 const char* pSessionID);
    void Done                                   (HX_RESULT status);
    void SendKeepAlive();

    HX_RESULT sendBandwidthLimitRequest(const char* pSessionID,
                                        const char* pURL,
                                        ULONG32 ulBandwidth);

    Player*           m_pPlayer; //Should not be public, should friend XXXSMP
    UINT32 controlBytesSent                     ();

    HX_RESULT SetStreamStartTime(const char* pszSessionID,
                                UINT32 ulStreamNum, UINT32 ulTimestamp);

     void releaseTransports(Player::Session* pSession);

private:
    void    RegisterPlayerOptions               (IHXValues* pRequestHeaders,
                                                    Player::Session*);

    HX_RESULT SetupClientStatsObject              ();

    HX_RESULT sendSetupStreamResponse
    (
        HX_RESULT HX_RESULTStatus,
        Player::Session* pSessionCurrent,
        IHXValues* pIHXValuesFileHeader,
        CHXSimpleList* pCHXSimpleListStreamHeaders,
        IHXValues* pIHXValuesOptional,
        IHXValues* pIHXValuesResponse
    );
    HX_RESULT addHeaders                        (IHXKeyValueList* pDestination,
                                                IHXValues* pSource);

    HX_RESULT PauseRequestHandler(const char* pszSessionID,
                                  UINT32 ulPausePoint = 0);

    HX_RESULT GetClientProfile(const char* pUrlText,
                               UINT32 ulUrlTextLen,
                               IHXValues* pRequestHeaders,
                               const char* pSessionID,
                               BOOL bUseRTP);

    HX_RESULT ParseQueryParams(const char* pszUrl, IHXValues* pRequestHeaders);

    BOOL IsClientAddressAllowedToReceiveMcast();

    ULONG32                             m_ulRefCount;
    RTSPServerProtocol*                 m_pProtocol;
    CHXSimpleList*                      m_pTransportContainer;
    IHXValues*                          m_pFileHeader;
    IHXPacketResend*                    m_pPacketResend;
    BOOL                                m_bSentHeaders;
    BOOL                                m_bTransportSetup;
    char*                               m_pVersionStr;
    IHXValues*                          m_pSessionHeaders;
    CHXMapStringToOb*                   m_pMcstSessionContainer;
    BYTE*                               m_bRuleOn;
    URL*                                m_url;

    BOOL                                m_bNewRTSPMsgDone;

    /* enforce client challenge authentication for RTSP/RDT */
    BOOL                                m_bRDT;
    IHXBuffer*                         m_pPlayerStarttime;
    IHXBuffer*                         m_pCompanyID;
    IHXBuffer*                         m_pPlayerGUID;
    IHXBuffer*                         m_pClientChallenge;
    IHXBuffer*                          m_pProxyPeerAddr;

    BOOL                                m_bProtInfoSet;
    UINT16                              m_uFirstSetupStreamNum;
    UINT32                              m_ulClientAvgBandwidth;

};

inline const char*
RTSPProtocol::versionStr()
{
    return m_pVersionStr;
}

#endif /* _RTSPPROT_H_ */
