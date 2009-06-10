/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrtspservprot2.h,v 1.1 2006/12/21 05:14:13 tknox Exp $
 * 
 * Portions Copyright (c) 1995-2007 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _RTSP_SERVER_PROTOCOL_2_H_
#define _RTSP_SERVER_PROTOCOL_2_H_

#include "hxcom.h"
#include "hxcomm.h"

DEFINE_GUID(IID_IHXRTSPServerProtocol2, 0x3c4e421a, 0xa217, 0x4fe3,
                    0x8e, 0xa3, 0xe4, 0x68, 0xda, 0x52, 0x72, 0xdb);

_INTERFACE IHXBuffer;
_INTERFACE IHXFileFormatHeaderAdvise;
_INTERFACE IHXPacket;
_INTERFACE IHXPacketResend;
_INTERFACE IHXRTSPServerProtocolResponse2;
_INTERFACE IHXSocket;
_INTERFACE IHXValues;

class BasePacket;
class CHXSimpleList;
class CHXString;
class Transport;
class TransportStreamHandler;

/**
 * \brief IHXRTSPServerProtocol2 replaces IHXRTSPServerProtocol
 * It is part of the transport refactoring.
 *
 * \note IID_IHXRTSPServerProtocol2 {3C4E421A-A217-4FE3-8EA3-E468DA5272DB}
 */
DECLARE_INTERFACE_(IHXRTSPServerProtocol2, IUnknown)
{
    /** \ref IUnknown
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;
    /** \ref IUnknown
     */
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    /** \ref IUnknown
     */
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::Init
     *  Purpose:
     *      Initialize context pointer
     */
    STDMETHOD(Init)             (THIS_
                                IUnknown* pContext) PURE;

    STDMETHOD(SetBuildVersion)  (THIS_
                                const char* pVersionString) PURE;

    STDMETHOD(SetOptionsRespHeaders)(THIS_
                                    IHXValues* pHeaders) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::Done
     *  Purpose:
     *      Close protocol objects
     */
    STDMETHOD(Done)             (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SetControl
     *  Purpose:
     *      Set control channel and response handler for protocol
     */
    STDMETHOD(SetControl)       (THIS_
                                IHXSocket* pCtrl,
                                IHXRTSPServerProtocolResponse2* pResp,
                                IHXBuffer* pBuffer) PURE;

    STDMETHOD(AddSession)       (THIS_
                                const char* pSessionID,
                                const char* pURL,
                                UINT32 ulSeqNo) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SetControl
     *  Purpose:
     *      Set response handler for protocol
     */
    STDMETHOD(SetResponse)      (THIS_
                                IHXSocket* pCtrl,
                                IHXRTSPServerProtocolResponse2* pResp) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::Disconnect
     *  Purpose:
     *      Disconnect client session
     */
    STDMETHOD(Disconnect)               (THIS_
                                        const char* pSessionID) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SendAlertRequest
     *  Purpose:
     *      Send alert request
     */
    STDMETHOD(SendAlertRequest)         (THIS_
                                        const char* pSessionID,
                                        INT32 lAlertNumber,
                                        const char* pAlertText) PURE;

    STDMETHOD(SendKeepAlive)            (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SendTeardownRequest
     *  Purpose:
     *      Send request to release connection resources
     */
    STDMETHOD(SendTeardownRequest)      (THIS_
                                        const char* pSessionID) PURE;

    STDMETHOD(SendRedirectRequest)      (THIS_
                                        const char* pSessionID,
                                        const char* pURL,
                                        UINT32 mSecsFromNow) PURE;

    STDMETHOD(SendSetParameterRequest)  (THIS_
                                        const char* pSessionID,
                                        const char* pURL,
                                        const char* pParamName,
                                        IHXBuffer* pParamValue) PURE;
    STDMETHOD(SendSetParameterRequest)  (THIS_
                                        const char* pSessionID,
                                        const char* pURL,
                                        const char* pParamName,
                                        const char* pParamValue,
                                        const char* pMimeType,
                                        const char* pContent) PURE;
    STDMETHOD(SendGetParameterRequest)  (THIS_
                                        UINT32 lParamType,
                                        const char* pParamName) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SetupSequenceNumberResponse
     *  Purpose:
     *      Setup the sequence number response header
     */
    STDMETHOD(SetupSequenceNumberResponse)      (THIS_
                                                const char* pSessionID) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SendStreamResponse
     *  Purpose:
     *      Send stream Setup or Describe response to client
     */
    STDMETHOD(SendStreamResponse)
    (
        THIS_
        HX_RESULT status,
        const char* pSessionID,
        IHXValues* pFileHeader,
        CHXSimpleList* pHeaders,
        IHXValues* pOptionalValues,
        IHXValues* pResponseHeaders,
        HXBOOL bMulticastOK,
        HXBOOL bRequireMulticast,
        HXBOOL bIsRealDataType
    ) PURE;

    STDMETHOD(SendStreamRecordDescriptionResponse)
    (
        THIS_
        HX_RESULT status,
        const char* pSessionID,
        IHXValues* pAuthValues,
        IHXValues* pResponseHeaders
    ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SendPacket
     *  Purpose:
     *      Send data packet to client
     */
    STDMETHOD(SendPacket)               (THIS_
                                         BasePacket* pPacket,
                                         const char* pSessionID) PURE;

    STDMETHOD(StartPackets)             (THIS_
                                        UINT16 uStreamNumber,
                                        const char* pSessionID) PURE;

    STDMETHOD(StopPackets)              (THIS_
                                        UINT16 uStreamNumber,
                                        const char* pSessionID) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SetPacketResend
     *  Purpose:
     *      Set pointer to object which will handle resending of packets
     */
    STDMETHOD(SetPacketResend)          (THIS_
                                        IHXPacketResend* pPacketResend,
                                        const char* pSessionID) PURE;

    STDMETHOD(SendRTTResponse)          (THIS_
                                        UINT32 secs,
                                        UINT32 uSecs,
                                        const char* pSessionID) PURE;

    STDMETHOD(SendCongestionInfo)       (THIS_
                                        INT32 xmitMultiplier,
                                        INT32 recvMultiplier,
                                        const char* pSessionID) PURE;

    STDMETHOD(SendStreamDone)           (THIS_
                                        UINT16 streamID,
                                        const char* pSessionID) PURE;

    STDMETHOD(SetConnectionTimeout)     (THIS_
                                        UINT32 uSeconds) PURE;

    STDMETHOD(SetFFHeaderAdvise)        (THIS_
                                        IHXFileFormatHeaderAdvise* pAdvise,
                                        const char * pSessionID) PURE;
};

DEFINE_GUID(IID_IHXRTSPServerProtocolResponse2, 0xd6a47496, 0x2b03, 0x4fc0,
                            0xa4, 0x8a, 0x35, 0xb1, 0x82, 0x37, 0x8f, 0xe6);

/**
 * \brief IHXRTSPServerProtocolResponse2 replaces IHXRTSPServerProtocolResponse
 * It is part of the transport refactoring.
 *
 * \note IID_IHXRTSPServerProtocolResponse2 {D6A47496-2B03-4FC0-A48A-35B182378FE6}
 */
DECLARE_INTERFACE_(IHXRTSPServerProtocolResponse2, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleStreamDescriptionRequest
     *  Purpose:
     *      Called to get stream description for an URL
     */

    STDMETHOD(HandleStreamDescriptionRequest)
                                        (THIS_
                                        const char* pURL,
                                        IHXValues* pRequestHeaders,
                                        const char* pSessionID,
                                        HXBOOL bUseRTP
                                        ) PURE;

    STDMETHOD(HandleStreamRecordDescriptionRequest)
                                        (THIS_
                                        const char* pURL,
                                        const char* pSessionID,
                                        IHXValues* pFileHeader,
                                        CHXSimpleList* pHeaders,
                                        IHXValues* pRequestHeaders
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleSetupRequest
     *  Purpose:
     *      Called to indicate success/failure of setting up a transport
     */
    STDMETHOD(HandleSetupRequest)       (THIS_
                                        HX_RESULT status
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleSetParameterRequest
     *  Purpose:
     *      Called to tell the client to set a parameter
     */
    STDMETHOD(HandleSetParameterRequest)        (THIS_
                                                UINT32 lParamType,
                                                const char* pParamName,
                                                IHXBuffer* pParamValue
                                                ) PURE;

    STDMETHOD(HandleSetParameterRequest)        (THIS_
                                                const char* pSessionID,
                                                const char* pParamName,
                                                const char* pParamValue,
                                                const char* pContent) PURE;

    STDMETHOD(HandleSetParameterResponse)       (THIS_
                                                HX_RESULT status) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleGetParameterRequest
     *  Purpose:
     *      Called to tell the client to get a parameter
     */
    STDMETHOD(HandleGetParameterRequest)        (THIS_
                                                UINT32 lParamType,
                                                const char* pParamName,
                                                IHXBuffer** pParamValue) PURE;

    STDMETHOD(HandleGetParameterResponse)       (THIS_
                                                HX_RESULT status,
                                                IHXBuffer* pParamValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandlePlayRequest
     *  Purpose:
     *      Called to start packet delivery - lFrom is start time in msecs,
     *      lTo is end time in msecs.
     */
    STDMETHOD(HandlePlayRequest)        (THIS_
                                        UINT32 lFrom,
                                        UINT32 lTo,
                                        CHXSimpleList* pSubscriptions,
                                            /*RTSPSubscription*/
                                        const char* pSessionID) PURE;

    STDMETHOD(HandleRecordRequest)      (THIS_
                                        const char* pSessionID) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandlePauseRequest
     *  Purpose:
     *      Called to pause packet delivery
     */
    STDMETHOD(HandlePauseRequest)       (THIS_
                                        const char* pSessionID) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleResumeRequest
     *  Purpose:
     *      Called to resume packet delivery
     */
    STDMETHOD(HandleResumeRequest)      (THIS_
                                        const char* pSessionID) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleTeardownResponse
     *  Purpose:
     *      Called to confirm release of connection resources
     */
    STDMETHOD(HandleTeardownResponse)   (THIS_
                                        HX_RESULT status) PURE;

    STDMETHOD(HandleTeardownRequest)    (THIS_
                                        const char* pSessionID) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandlePacket
     *  Purpose:
     *      Called when transport layer has received a data packet
     */
    STDMETHOD(HandlePacket)             (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        IHXPacket* pPacket) PURE;

    STDMETHOD(HandleSubscribe)          (THIS_
                                        CHXSimpleList* pSubscriptions,
                                                /*RTSPSubscription*/
                                        const char* pSessionID) PURE;

    STDMETHOD(HandleUnsubscribe)        (THIS_
                                        CHXSimpleList* pUnsubscriptions,
                                                /*RTSPSubscription*/
                                        const char* pSessionID) PURE;

    STDMETHOD(HandleSubscriptionDone)   (THIS_
                                            REF(UINT32) ulAddress,
                                            REF(UINT32) ulSourcePort,
                                            REF(UINT32) ulPort,
                                            const char* pSessionID,
                                            REF(TransportStreamHandler*) pHandler
                                        ) PURE;

    STDMETHOD(HandleBackChannel)        (THIS_
                                        IHXPacket* pPacket,
                                        const char* pSessionID) PURE;


    STDMETHOD(HandleBWReport)           (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        INT32 aveBandwidth,
                                        INT32 packetLoss,
                                        INT32 bandwidthWanted) PURE;

    STDMETHOD(HandlePlayerStats)        (THIS_
                                        const char* pStats,
                                        const char* pSessionID) PURE;

    STDMETHOD(HandleSessionHeaders)     (THIS_
                                        IHXValues* pSessionHeaders) PURE;

    STDMETHOD(HandleSpeedParam)         (THIS_
                                        const char* pSessionID,
                                        FIXED32 fSpeed) PURE;

    STDMETHOD(HandleScaleParam)         (THIS_
                                        const char* pSessionID,
                                        FIXED32 fScale) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleProtocolError
     *  Purpose:
     *      Called to notify client of protocol error conditions
     */
    STDMETHOD(HandleProtocolError)      (THIS_
                                        HX_RESULT status) PURE;

    STDMETHOD(AddSession)               (THIS_
                                        const char* pURLText,
                                        UINT32 ulSeqNo,
                                        REF(CHXString) sessionID,
                                        HXBOOL bRetainEntityForSetup) PURE;

    STDMETHOD(AddSessionWithID)         (THIS_
                                        const char* pURLText,
                                        UINT32 ulSeqNo,
                                        REF(CHXString) sessionID,
                                        HXBOOL bRetainEntityForSetup) PURE;

    STDMETHOD(AddTransport)             (THIS_
                                        Transport* pTransport,
                                        const char* pSessionID,
                                        UINT16 streamNumber,
                                        UINT32 ulReliability) PURE;

    STDMETHOD(SetupTransports)          (THIS_
                                        const char* pSessionID) PURE;

    STDMETHOD(HandleLimitBandwidthByDropping)
                                        (THIS_
                                        UINT16 streamNumber,
                                        const char* pSessionID,
                                        UINT32 ulBandwidthLimit) PURE;

    STDMETHOD(HandleSetDeliveryBandwidth)
                                        (THIS_
                                         UINT32 ulBackOff,
                                         const char* pSessionID,
                                         UINT32 ulBandwidth) PURE;

    STDMETHOD(HandleStreamDone)         (THIS_
                                        HX_RESULT status,
                                        UINT16 uStreamNumber) PURE;

    STDMETHOD(GenerateNewSessionID)     (THIS_
                                        REF(CHXString) sessionID,
                                        UINT32 ulSeqNo) PURE;

    STDMETHOD(HandleRetainEntityForSetup)
                                        (THIS_
                                        const char* pSessionID,
                                        HXBOOL bRequired) PURE;

    STDMETHOD(SetMidBox)
                                        (THIS_
                                        const char* pSessionID,
                                        HXBOOL bIsMidBox) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleStreamAdaptation
     *  Purpose:
     *      Called to set client provided Stream Adaptation parameters
     */
    STDMETHOD(HandleStreamAdaptation)      (THIS_
                                        const char* pSessionID,
                                        REF(StreamAdaptationParams) streamAdaptParams,
                                        HXBOOL bHlxStreamAdaptScheme) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::Handle3GPPLinkChar
     *  Purpose:
     *      Called to set client provided Link Charateristics using
     *      	3GPP-Link-Char header
     */
    STDMETHOD(Handle3GPPLinkChar)      (THIS_
                                        const char* pSessionID,
                                        REF(LinkCharParams) linkCharParams) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleClientAvgBandwidth
     *  Purpose:
     *      Called to set avg bandwidth from Bandwidth Header
     */
    STDMETHOD(HandleClientAvgBandwidth)	(THIS_
                                        const char* pSessionID,
                                        UINT32 ulAvgBandwidth) PURE;   

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleSetupRequest
     *  Purpose:
     *      Called for each stream SETUP recevied
     */
    STDMETHOD(HandleStreamSetup) (THIS_
				const char* pSessionID,
				UINT16 uStreamNumber,
				UINT16 uStreamGroupNumber) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXRTSPServerProtocol2)
DEFINE_SMART_PTR(IHXRTSPServerProtocolResponse2)

#endif /* _RTSP_SERVER_PROTOCOL_2_H_ */
