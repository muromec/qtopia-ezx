/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspif.h,v 1.45 2009/01/19 23:38:07 sfu Exp $
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

#ifndef _RTSPIF_H_
#define _RTSPIF_H_

//XXX...the following 2 includes should go away
#include "hxpends.h"
#include "hxcomm.h"
#include "hxnet.h"
// Don't remove this include!
#include "hxstring.h"
#include "basepkt.h"
#include "servrsnd.h"
#include "hxmon.h"
#include "statinfo.h"

#include "hxstreamadapt.h"

struct IHXTCPSocket;
class RTSPServerState;
class RTSPClientState;
class RawPacketFilter;

struct IHXBuffer;
class CHXSimpleList;
class RTSPTransport;
class RTSPStreamHandler;
struct IHXPacket;
struct IHXValues;
struct IHXPacketResend;
class BasePacket;

_INTERFACE IHXFileFormatHeaderAdvise;

/* client initialization flags */
#define RTSP_INIT_NO_AUTHORIZATION 0
#define RTSP_INIT_HXPRIVATE_AUTHORIZATION 1
#define RTSP_INIT_WWW_AUTHORIZATION 2

typedef enum _RTSPTransportSubTypeEnum
{
    RTSP_SUBTR_NONE             = 0,    /* unknown */
    RTSP_SUBTR_UDP              = 1,    /* udp */
    RTSP_SUBTR_TCP              = 2,    /* tcp */
    RTSP_SUBTR_MCAST            = 3,    /* multicast */
    RTSP_SUBTR_NULLSET          = 4     /* null setup */
} RTSPTransportSubTypeEnum;

typedef enum _RTSPTransportTypeEnum
{
    RTSP_TR_NONE        = 0,    /* No transport */
    RTSP_TR_RDT_MCAST,          /* x-real-rdt/mcast */
    RTSP_TR_RDT_UDP,            /* x-real-rdt/udp */
    RTSP_TR_RDT_TCP,            /* x-real-rdt/tcp */
    RTSP_TR_TNG_UDP,            /* x-pn-tng/udp, Supported, Deprecated */
    RTSP_TR_TNG_TCP,            /* x-pn-tng/tcp, Supported, Deprecated */
    RTSP_TR_TNG_MCAST,          /* x-pn-tng/mcast, Not Supported / Deprecated */
    RTSP_TR_RTP_UDP,            /* rtp/avp/udp;unicast */
    RTSP_TR_RTP_MCAST,          /* rtp/avp/udp;multicast */
    RTSP_TR_RTP_TCP,            /* rtp/avp/tcp;unicast */
    RTSP_TR_RTCP,               /* Not valid in SETUP */
    RTSP_TR_NULLSET,            /* x-real-nullsetup, for RealProxy */
    RTSP_TR_BCNG_UDP,
    RTSP_TR_BCNG_MCAST,
    RTSP_TR_BCNG_TCP,
    RTSP_TR_LAST
} RTSPTransportTypeEnum;

typedef enum _RTSPTransportModeEnum
{
    RTSP_TRMODE_NONE    = 0,    /* No mode, uninitialized */
    RTSP_TRMODE_PLAY,           /* mode=play, default per 2326 */
    RTSP_TRMODE_RECORD,         /* mode=record */
    RTSP_TRMODE_OTHER           /* unrecognized */
} RTSPTransportModeEnum;

typedef enum _RTSPPrerollTypeEnum
{
    RTSP_PREROLL_NONE            = 0,   /* unknown */
    RTSP_PREROLL_PREDECBUFPERIOD = 1    /* x-initpredecbufperiod */
} RTSPPrerollTypeEnum;

/*
 * Various macros for determining the type of transport:
 *
 *   IS_CLIENT_TRANSPORT: a player transport (eg. not server-to-server)
 *   IS_RDT_TRANSPORT   : an RDT or TNG transport
 *   IS_RTP_TRANSPORT   : an RTP (rfc1889) transport
 *   IS_TCP_TRANSPORT   : any TCP transport (client or not)
 *   IS_MCAST_TRANSPORT : any multicast transport (client or not)
 *
 * Note that nullsetup does not fall into any of these categories.
 */

#define IS_CLIENT_TRANSPORT(t) \
    ((t) >= RTSP_TR_RDT_MCAST && (t) <= RTSP_TR_RTCP)

#define IS_RDT_TRANSPORT(t) \
    ((t) >= RTSP_TR_RDT_MCAST && (t) <= RTSP_TR_TNG_MCAST)

#define IS_RTP_TRANSPORT(t) \
    ((t) >= RTSP_TR_RTP_UDP && (t) <= RTSP_TR_RTCP)

#define IS_TCP_TRANSPORT(t) \
    ((t) == RTSP_TR_RDT_TCP || (t) == RTSP_TR_TNG_TCP || \
     (t) == RTSP_TR_RTP_TCP || (t) == RTSP_TR_BCNG_TCP)

#define IS_UDP_TRANSPORT(t) \
    ((t) == RTSP_TR_BCNG_MCAST || (t) == RTSP_TR_RDT_MCAST || \
     (t) == RTSP_TR_TNG_MCAST  || (t) == RTSP_TR_RTP_MCAST || \
     (t) == RTSP_TR_BCNG_UDP   || (t) == RTSP_TR_RDT_UDP || \
     (t) == RTSP_TR_TNG_UDP    || (t) == RTSP_TR_RTP_UDP)

#define IS_MCAST_TRANSPORT(t) \
    ((t) == RTSP_TR_RDT_MCAST || (t) == RTSP_TR_TNG_MCAST || \
     (t) == RTSP_TR_RTP_MCAST || (t) == RTSP_TR_BCNG_MCAST)

#define IS_BCNG_TRANSPORT(t) \
    ((t) == RTSP_TR_BCNG_MCAST || (t) == RTSP_TR_BCNG_UDP || \
     (t) == RTSP_TR_BCNG_TCP)

enum    /* parameter types */
{
    RTSP_PARAM_STRING           = 0,
    RTSP_PARAM_LONG             = 1,
    RTSP_PARAM_BINARY           = 2
};

enum RTPInfoEnum
{
    RTPINFO_ERROR,              /* error */
    RTPINFO_SEQ,                /* only Seq found */
    RTPINFO_RTPTIME,            /* only rtptime found */
    RTPINFO_SEQ_RTPTIME,        /* both seq & rtptime found */
    RTPINFO_EMPTY       /* neither seq nor rtptime found */
};

typedef enum
{
    RTSPMEDIA_TYPE_UNKNOWN,
    RTSPMEDIA_TYPE_AUDIO,
    RTSPMEDIA_TYPE_VIDEO,
    RTSPMEDIA_TYPE_APP,
    RTSPMEDIA_TYPE_EVENT
} RTSPMediaType;

typedef struct _RTSPASMRule
{
    UINT16      m_ruleNumber;
    UINT16      m_streamNumber;
} RTSPASMRule;

typedef struct _RTSPSubscription
{
    UINT16      m_ruleNumber;
    UINT16      m_streamNumber;
    HXBOOL      m_bIsSubscribe; // Only used in RuleChanges()
} RTSPSubscription;

typedef struct _RTSPStreamInfo
{
    _RTSPStreamInfo()
        : m_streamNumber(0)
        , m_uStreamGroupNumber(0xFFFF)
        , m_ulControlID((UINT32)-1)
        , m_bNeedReliablePackets(FALSE)
        , m_sPort(0)
        , m_bForceRTP(FALSE)
        , m_bHasOutOfOrderTS(FALSE)
        , m_eMediaType(RTSPMEDIA_TYPE_UNKNOWN)
        , m_uByteLimit(0)
        , m_rtpPayloadType(101)
        , m_bHasMarkerRule(0)
        , m_markerRule(1)
        , m_bHasRTCPRule(0)
        , m_ulPayloadWirePacket(0)
        , m_bIsSyncMaster(0)
        , m_RTCPRule(1)
        , m_sampleRate(1)
        , m_sampleSize(1)
        , m_RTPFactor(1)
        , m_HXFactor(1)
        , m_bIsLive(0)
        , m_bExtensionSupport(0)
        , m_bActive(TRUE)
        , m_ulAvgBitRate(0)
        , m_ulRtpRRBitRate((UINT32)-1)
        , m_ulRtpRSBitRate((UINT32)-1)
        , m_pMulticastAddr(NULL)
        , m_bRealMedia(FALSE)
        , m_pStreamHeader(NULL)
    , m_ulSSRCFromSetup(0)
    {}

    ~_RTSPStreamInfo()
    {
        HX_RELEASE(m_pMulticastAddr);
        HX_RELEASE(m_pStreamHeader);
    }
    UINT16      m_streamNumber;
    UINT16      m_uStreamGroupNumber;
    UINT32      m_ulControlID;
    HXBOOL      m_bNeedReliablePackets;
    CHXString   m_streamControl;
    UINT16      m_sPort;
    HXBOOL      m_bForceRTP;
    HXBOOL      m_bHasOutOfOrderTS;
    RTSPMediaType m_eMediaType;
    UINT32      m_uByteLimit;
    // rest of them are only for RTP
    INT16       m_rtpPayloadType;
    HXBOOL      m_bHasMarkerRule;
    UINT16      m_markerRule;
    HXBOOL      m_bHasRTCPRule;
    UINT32      m_ulPayloadWirePacket;
    HXBOOL      m_bIsSyncMaster;
    UINT16      m_RTCPRule;
    UINT32      m_sampleRate;
    UINT32      m_sampleSize;
    UINT32      m_RTPFactor;
    UINT32      m_HXFactor;
    HXBOOL      m_bIsLive;
    HXBOOL      m_bExtensionSupport;
    HXBOOL      m_bActive;
    UINT32      m_ulAvgBitRate;
    UINT32      m_ulRtpRRBitRate;
    UINT32      m_ulRtpRSBitRate;
    IHXSockAddr* m_pMulticastAddr;
    HXBOOL      m_bRealMedia;
    IHXValues*  m_pStreamHeader;
    UINT32  m_ulSSRCFromSetup;
} RTSPStreamInfo;

typedef struct _RTSPSocketInfo
{
    IHXTCPSocket*       m_pTCPSocket;
    UINT32              m_ulForeignAddr;
} RTSPSocketInfo;

/*
 * XXXBAB - compatibility struct until I change the interface
 * to SendSetupRequest
 */
class RTSPTransportType
{
public:
    RTSPTransportType() :
        m_lTransportType(RTSP_TR_NONE),
        m_Mode(RTSP_TRMODE_NONE),
        m_sPort(0),
        m_streamNumber(0),
        m_ulBufferDepth(0),
        m_pDestAddr(NULL),
        m_ulStreamID(0)
    {
        // Empty
    }
    RTSPTransportType(const RTSPTransportType& other)
    {
        *this = other;
    }
    ~RTSPTransportType()
    {
        HX_RELEASE(m_pDestAddr);
    }
    RTSPTransportType& operator=(const RTSPTransportType& other)
    {
        m_lTransportType = other.m_lTransportType;
        m_Mode = other.m_Mode;
        m_sPort = other.m_sPort;
        m_streamNumber = other.m_streamNumber;
        m_ulBufferDepth = other.m_ulBufferDepth;
        m_pDestAddr = other.m_pDestAddr;
        m_ulStreamID = other.m_ulStreamID;
        if (m_pDestAddr != NULL)
        {
            m_pDestAddr->AddRef();
        }
        return *this;
    }
    RTSPTransportTypeEnum       m_lTransportType;
    RTSPTransportModeEnum       m_Mode;
    UINT16                      m_sPort;
    UINT16                      m_streamNumber;
    UINT32                      m_ulBufferDepth;
    IHXSockAddr*                m_pDestAddr;
    UINT32                      m_ulStreamID;
};

DECLARE_INTERFACE_(IHXRTSPServerProtocolResponse, IUnknown)
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
     *      IHXRTSPServerProtocolResponse::HandleAuthentication
     *
     *  Purpose:
     *      Called to indicate success/failure of authentication
     */
/*XXXkshoop Removed, it was deadweight
    STDMETHOD(HandleAuthentication)     (THIS_
                                        HX_RESULT status
                                        ) PURE;
*/
    /*************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleWWWAuthentication
     *
     *  Purpose:
     *      Called when a WWW-Authenticate header is received.
     */
/*XXXkshoop Removed, it was deadweight
    STDMETHOD(HandleWWWAuthentication)  (THIS_
                                         IHXValues* pAuthValues
                                         ) PURE;
*/

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
                                                HX_RESULT status
                                                ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleGetParameterRequest
     *  Purpose:
     *      Called to tell the client to get a parameter
     */
    STDMETHOD(HandleGetParameterRequest)        (THIS_
                                                UINT32 lParamType,
                                                const char* pParamName,
                                                IHXBuffer** pParamValue
                                                ) PURE;

    STDMETHOD(HandleGetParameterResponse)       (THIS_
                                                HX_RESULT status,
                                                IHXBuffer* pParamValue
                                                ) PURE;

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
                                        const char* pSessionID
                                        ) PURE;

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
                                        HX_RESULT status
                                        ) PURE;

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
                                        IHXPacket* pPacket
                                        ) PURE;

    STDMETHOD(HandleSubscribe)          (THIS_
                                        CHXSimpleList* pSubscriptions,
                                                /*RTSPSubscription*/
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(HandleUnsubscribe)        (THIS_
                                        CHXSimpleList* pUnsubscriptions,
                                                /*RTSPSubscription*/
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(HandleSubscriptionDone)   (THIS_
                                            REF(UINT32) ulAddress,
                                            REF(UINT32) ulSourcePort,
                                            REF(UINT32) ulPort,
                                            const char* pSessionID,
                                            REF(RTSPStreamHandler*) pHandler
                                        ) PURE;

    STDMETHOD(HandleBackChannel)        (THIS_
                                        IHXPacket* pPacket,
                                        const char* pSessionID
                                        ) PURE;


    STDMETHOD(HandleBWReport)           (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        INT32 aveBandwidth,
                                        INT32 packetLoss,
                                        INT32 bandwidthWanted
                                        ) PURE;

    STDMETHOD(HandlePlayerStats)        (THIS_
                                        const char* pStats,
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(HandleSessionHeaders)     (THIS_
                                        IHXValues* pSessionHeaders
                                        ) PURE;

    STDMETHOD(HandleSpeedParam)         (THIS_
                                        const char* pSessionID,
                                        FIXED32 fSpeed
                                        ) PURE;

    STDMETHOD(HandleScaleParam)         (THIS_
                                        const char* pSessionID,
                                        FIXED32 fScale
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocolResponse::HandleProtocolError
     *  Purpose:
     *      Called to notify client of protocol error conditions
     */
    STDMETHOD(HandleProtocolError)      (THIS_
                                        HX_RESULT status
                                        ) PURE;

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
                                        RTSPTransport* pTransport,
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
     *          3GPP-Link-Char header
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
    STDMETHOD(HandleClientAvgBandwidth) (THIS_
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


DECLARE_INTERFACE_(IHXRTSPServerProtocol, IUnknown)
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
     *      IHXRTSPServerProtocol::Init
     *  Purpose:
     *      Initialize context pointer
     */
    STDMETHOD(Init)             (THIS_
                                IUnknown* pContext
                                ) PURE;

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
                                IHXRTSPServerProtocolResponse* pResp,
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
                                IHXRTSPServerProtocolResponse* pResp) PURE;

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
                                        const char* pParamName
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SetupSequenceNumberResponse
     *  Purpose:
     *      Setup the sequence number response header
     */
    STDMETHOD(SetupSequenceNumberResponse)      (THIS_
                                                const char* pSessionID
                                                ) PURE;

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
                                         const char* pSessionID
                                        ) PURE;

    STDMETHOD(StartPackets)             (THIS_
                                        UINT16 uStreamNumber,
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(StopPackets)              (THIS_
                                        UINT16 uStreamNumber,
                                        const char* pSessionID
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPServerProtocol::SetPacketResend
     *  Purpose:
     *      Set pointer to object which will handle resending of packets
     */
    STDMETHOD(SetPacketResend)          (THIS_
                                        IHXPacketResend* pPacketResend,
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(SendRTTResponse)          (THIS_
                                        UINT32 secs,
                                        UINT32 uSecs,
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(SendCongestionInfo)       (THIS_
                                        INT32 xmitMultiplier,
                                        INT32 recvMultiplier,
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(SendStreamDone)           (THIS_
                                        UINT16 streamID,
                                        const char* pSessionID
                                        ) PURE;

    STDMETHOD(SetConnectionTimeout)     (THIS_
                                        UINT32 uSeconds
                                        ) PURE;

    STDMETHOD(SetFFHeaderAdvise)        (THIS_
                                        IHXFileFormatHeaderAdvise* pAdvise,
                                        const char * pSessionID
                                        ) PURE;

};

DECLARE_INTERFACE_(IHXRTSPServerSessionManagerResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(GetSessionInstanceDone)   (THIS_
                                        HX_RESULT status,
                                        IHXRTSPServerProtocol* pProt,
                                        IHXSocket* pSocket,
                                        IHXBuffer* pBuffer) PURE;
};

class RTSPResponseMessage;

DECLARE_INTERFACE_(IHXRTSPClientProtocolResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXRTSPClientProtocolResponse methods
     */

    STDMETHOD(InitDone)         (THIS_
                                HX_RESULT status
                                ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandleAuthentication
     *  Purpose:
     *      Called to indicate success/failure of authentication
     */
    STDMETHOD(HandleOptionsResponse)
    (
        THIS_
        HX_RESULT status,
        IHXValues* pHeaders
    ) PURE;

    STDMETHOD(HandleWWWAuthentication)  (THIS_
                                         HX_RESULT status,
                                         IHXValues* pAuthInfo
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandleStreamDescriptionResponse
     *  Purpose:
     *      Called to handle a stream description for an URL
     */
    STDMETHOD(HandleStreamDescriptionResponse)
    (
        THIS_
        HX_RESULT status,
        IHXValues* pFileHeader,
        CHXSimpleList* pStreams,
        IHXValues* pResponseHeaders
    ) PURE;

    STDMETHOD(HandleStreamRecordDescriptionResponse)
                                        (THIS_
                                        HX_RESULT status,
                                        IHXValues* pResponseHeaders
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandleSetupResponse
     *  Purpose:
     *      Called to indicate success/failure of setting up a transport
     */
    STDMETHOD(HandleSetupResponse)      (THIS_
                                        HX_RESULT status
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandlePlayResponse
     *  Purpose:
     *      Called to indicate play status
     */
    STDMETHOD(HandlePlayResponse)       (THIS_
                                        HX_RESULT status
                                        ) PURE;

    STDMETHOD(HandleRecordResponse)     (THIS_
                                        HX_RESULT status
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandleTeardownResponse
     *  Purpose:
     *      Called to confirm release of connection resources
     */
    STDMETHOD(HandleTeardownResponse)   (THIS_
                                        HX_RESULT status
                                        ) PURE;

    STDMETHOD(HandleSetParameterRequest)        (THIS_
                                                UINT32 lParamType,
                                                const char* pParamName,
                                                IHXBuffer* pParamValue
                                                ) PURE;

    STDMETHOD(HandleSetParameterRequest)        (THIS_
                                                const char* pParamName,
                                                const char* pParamValue,
                                                const char* pContent) PURE;

    STDMETHOD(HandleSetParameterResponse)       (THIS_
                                                HX_RESULT status
                                                ) PURE;

    STDMETHOD(HandleSetParameterResponseWithValues)     (THIS_
                                                HX_RESULT status,
                                                IHXValues* pValues
                                                )
    { return HandleSetParameterResponse(status);};

    STDMETHOD(HandleGetParameterRequest)        (THIS_
                                                UINT32 lParamType,
                                                const char* pParamName,
                                                IHXBuffer** pParamValue
                                                ) PURE;

    STDMETHOD(HandleGetParameterResponse)       (THIS_
                                                HX_RESULT status,
                                                IHXBuffer* pParamValue
                                                ) PURE;

    STDMETHOD(HandleRedirectRequest)            (THIS_
                                                const char* pURL,
                                                UINT32 msecsFromNow
                                                ) PURE;

    STDMETHOD(HandleUseProxyRequest)            (THIS_
                                                const char* pProxyURL
                                                )
    {
        // default behavior is NOTIMPL
        return HXR_NOTIMPL;
    }

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandleAlertRequest
     *  Purpose:
     *      Called to notify client of alert request
     */
    STDMETHOD(HandleAlertRequest)       (THIS_
                                        HX_RESULT status,
                                        INT32 lAlertNumber,
                                        const char* pAlertText
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandlePacket
     *  Purpose:
     *      Called when transport layer has received a data packet
     */
    STDMETHOD(HandlePacket)             (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        IHXPacket* pPacket
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocolResponse::HandleProtocolError
     *  Purpose:
     *      Called to notify client of protocol error conditions
     */
    STDMETHOD(HandleProtocolError)      (THIS_
                                        HX_RESULT status
                                        ) PURE;

    STDMETHOD(HandleRTTResponse)        (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        UINT32 ulSecs,
                                        UINT32 ulUSecs
                                        ) PURE;

    STDMETHOD(HandleCongestion)         (THIS_
                                        HX_RESULT status,
                                        const char* pSessionID,
                                        INT32 xmitMultiplier,
                                        INT32 recvMultiplier
                                        ) PURE;

    STDMETHOD(HandleStreamDone)         (THIS_
                                        HX_RESULT status,
                                        UINT16 uStreamNumber
                                        ) PURE;

    /* This only indicates that all packets have been received from the
     * server. We still need to read packets from the transport buffer
     * StreamDone will indicate when there are no more packets to be
     * read from Transport buffer
     */
    STDMETHOD(HandleSourceDone)         (THIS) PURE;

    STDMETHOD(HandlePrerollChange)      (THIS_
                                         RTSPPrerollTypeEnum prerollType,
                                         UINT32 ulPreroll
                                        ) PURE;
};

DEFINE_GUID(IID_IHXRTSPClientProtocol, 0x00000405, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 
                       0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXRTSPClientProtocol

#define CLSID_IHXRTSPClientProtocol IID_IHXRTSPClientProtocol

DECLARE_INTERFACE_(IHXRTSPClientProtocol, IUnknown)
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
     *      IHXRTSPClientProtocol::Init
     *  Purpose:
     *      Initialize an IHXTCPSocket and connect to pAddr:port
     */
    STDMETHOD(Init)
                                (THIS_
                                IUnknown* pContext,
                                const char* pAddr,
                                UINT16 port,
                                IHXRTSPClientProtocolResponse* pClient,
                                UINT32 initializationType,
                                IHXValues* pSessionHeaders,
                                IHXValues* pInfo,
                                HXBOOL bHTTPCloak,
                                UINT16 uCloakPort,
                                HXBOOL bNoReuseConnection
                                ) PURE;

    STDMETHOD(SetBuildVersion)  (THIS_
                                const char* pVersionString) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::Done
     *  Purpose:
     *      Close protocol objects
     */
    STDMETHOD(Done)             (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendStreamDescriptionRequest
     *  Purpose:
     *      Request stream description from URL
     */
    STDMETHOD(SendStreamDescriptionRequest)
                                        (THIS_
                                        const char* pURL,
                                        IHXValues* pRequestHeaders
                                        ) PURE;

    STDMETHOD(SendStreamRecordDescriptionRequest)
                                        (THIS_
                                        const char* pURL,
                                        IHXValues* pFileHeader,
                                        CHXSimpleList* pStreams,
                                        IHXValues* pRequestHeaders
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendTransportRequest
     *  Purpose:
     *      Request a transport type for data stream from an array of
     *      available transport types
     */
    STDMETHOD(SendSetupRequest)
    (
        THIS_
        RTSPTransportType* pTransType,
        UINT16 nTransTypes,
        IHXValues* pIHXValuesRequestHeaders
    ) PURE;

    STDMETHOD(SendSetParameterRequest)  (THIS_
                                        UINT32 lParamType,
                                        const char* pParamName,
                                        IHXBuffer* pParamValue
                                        ) PURE;

    STDMETHOD(SendSetParameterRequest)  (THIS_
                                        const char* pParamName,
                                        const char* pParamValue,
                                        const char* pMimeType,
                                        const char* pContent) PURE;

    STDMETHOD(SendGetParameterRequest)  (THIS_
                                        UINT32 lParamType,
                                        const char* pParamName
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendPlayRequest
     *  Purpose:
     *      Request start of packet delivery
     */
    STDMETHOD(SendPlayRequest)          (THIS_
                                        UINT32 lFrom,
                                        UINT32 lTo,
                                        CHXSimpleList* pSubList
                                            /* RTSPSubscription */
                                        ) PURE;

    STDMETHOD(SendPauseRequest)         (THIS) PURE;

    STDMETHOD(SendResumeRequest)        (THIS) PURE;

    STDMETHOD(SendRecordRequest)        (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendTeardownRequest
     *  Purpose:
     *      Request release of connection resources
     */
    STDMETHOD(SendTeardownRequest)      (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendPacket
     *  Purpose:
     *      Send data packet to server (for RECORD)
     */
    STDMETHOD(SendPacket)               (THIS_
                                        BasePacket* pPacket
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendStreamDone
     *  Purpose:
     *      Send data end packet to server (for RECORD)
     */
    STDMETHOD(SendStreamDone)           (THIS_
                                        UINT16 streamID
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendStatistics
     *  Purpose:
     *      Send cumulative stats from player to server
     */
    STDMETHOD(SendPlayerStats)          (THIS_
                                        const char* pStats
                                        ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::SendKeepAlive
     *  Purpose:
     *      Send keep alive request to server
     */
    STDMETHOD(SendKeepAlive)            (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRTSPClientProtocol::GetPacket
     *  Purpose:
     *      Get data packet from transport layer
     */
    STDMETHOD(GetPacket)                (THIS_
                                        UINT16 uStreamNumber,
                                        REF(IHXPacket*) pPacket
                                        ) PURE;

    STDMETHOD(StartPackets)             (THIS_
                                        UINT16 uStreamNumber
                                        ) PURE;

    STDMETHOD(StopPackets)              (THIS_
                                        UINT16 uStreamNumber
                                        ) PURE;

    STDMETHOD(SetProxy)                 (THIS_
                                        const char* pProxyHost,
                                        UINT16 proxyPort
                                        ) PURE;

    STDMETHOD(SetResponse)              (THIS_
                                        IHXRTSPClientProtocolResponse* pResp
                                        ) PURE;

    STDMETHOD(InitSockets)              (THIS) PURE;

    STDMETHOD(GetCurrentBuffering)      (THIS_
                                        UINT16 uStreamNumber,
                                        REF(UINT32) ulLowestTimestamp,
                                        REF(UINT32) ulHighestTimestamp,
                                        REF(UINT32) ulNumBytes,
                                        REF(HXBOOL) bDone) PURE;

    STDMETHOD(SeekFlush)                (THIS) PURE;

    STDMETHOD_(HXBOOL, IsDataReceived)    (THIS) PURE;

    STDMETHOD_(HXBOOL,IsSourceDone)       (THIS) PURE;

    /*
     * XXX...The following 3 functions had better be removed under
     *       full IRMA
     */

    STDMETHOD_(IHXPendingStatus*, GetPendingStatus)     (THIS) PURE;
    STDMETHOD_(IHXStatistics*, GetStatistics)           (THIS) PURE;
    STDMETHOD_(HXBOOL, HttpOnly)                          (THIS) PURE;

    /*****************************************
     * Methods specific to TNG/ASM transport
     *
     */

    STDMETHOD(Subscribe)                (THIS_
                                        CHXSimpleList* pSubscriptions
                                            /* RTSPSubscription */
                                        ) PURE;

    STDMETHOD(Unsubscribe)              (THIS_
                                        CHXSimpleList* pUnsubscriptions
                                            /* RTSPSubscription */
                                        ) PURE;

    STDMETHOD(RuleChange)               (THIS_
                                        CHXSimpleList* pChanges) PURE;

    STDMETHOD(BackChannelPacketReady)   (THIS_
                                        IHXPacket* pPacket) PURE;

    STDMETHOD(SendRTTRequest)           (THIS) PURE;

    STDMETHOD(SendBWReport)             (THIS_
                                        INT32 aveBandwidth,
                                        INT32 packetLoss,
                                        INT32 bandwidthWanted
                                        ) PURE;

    STDMETHOD(SetFirstSeqNum)           (THIS_
                                        UINT16 uStreamNumber,
                                        UINT16 uSeqNum
                                        ) PURE;

    STDMETHOD(SetRTPInfo)               (THIS_
                                        UINT16 uStreamNumber,
                                        UINT16 uSeqNum,
                                        UINT32 ulRTPTime,
                                        RTPInfoEnum info
                                        ) PURE;

    STDMETHOD(SetConnectionTimeout)     (THIS_
                                        UINT32 uSeconds
                                        ) PURE;

    /* This is a quick fix to correctly display UDPMode in the client.
     * We currently always show Multicast since the rtsp library
     * never tells the reponse object that the protocol that actually
     * succeeded is UDP and not Mutlicast.
     *
     * This is an interim soluton.
     * Correct solution is to add transport type in HandleSetupResponse().
     * Will do after B2 since I do not break any builds - XXXRA
     *
     * MULTICAST_MODE = 1,
     * UDP_MODE = 2,
     * TCP_MODE = 3,
     *
     */
    STDMETHOD_(UINT16, GetProtocolType) (THIS) PURE;

    STDMETHOD(InitPacketFilter)         (THIS_
                                         RawPacketFilter* pFilter) PURE;
};

DEFINE_GUID(IID_IHXRTSPClientProtocol2, 0x00000405, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 
                        0xa0, 0x24, 0x8d, 0xa5, 0xf1);

#undef  INTERFACE
#define INTERFACE   IHXRTSPClientProtocol2

DECLARE_INTERFACE_(IHXRTSPClientProtocol2, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD_(UINT16, GetRDTFeatureLevel)  (THIS)  PURE;
    STDMETHOD(EnterPrefetch)            (THIS)  PURE;
    STDMETHOD(LeavePrefetch)            (THIS)  PURE;
    STDMETHOD(EnterFastStart)           (THIS)  PURE;
    STDMETHOD(LeaveFastStart)           (THIS)  PURE;
    STDMETHOD(InitCloak)            (THIS_
                        UINT16* pCloakPorts, 
                        UINT8 nCloakPorts, 
                        IHXValues* pValues) PURE;
    STDMETHOD(SetStatistics)            (THIS_
                        UINT16 uStreamNumber, 
                        STREAM_STATS* pStats) PURE;
    STDMETHOD_(HXBOOL,IsRateAdaptationUsed) (THIS) PURE;
    STDMETHOD(SetUseRTPFlag)                (THIS_
                                            HXBOOL bUseRTP) PURE;
};

DECLARE_INTERFACE_(IHXRTSPTransportResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * PacketReady() is for control packets only. Data packets should
     * be retrieved with the getPacket() call exported by RTSPTransport
     */

    STDMETHOD(PacketReady)      (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                IHXPacket* pPacket
                                ) PURE;

    STDMETHOD(OnRTTRequest)     (THIS_
                                HX_RESULT status,
                                const char* pSessionID
                                ) PURE;

    STDMETHOD(OnRTTResponse)    (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                UINT32 ulSecs,
                                UINT32 ulUSecs
                                ) PURE;

    STDMETHOD(OnBWReport)       (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                INT32 aveBandwidth,
                                INT32 packetLoss,
                                INT32 bandwidthWanted
                                ) PURE;

    STDMETHOD(OnCongestion)     (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                INT32 xmitMultiplier,
                                INT32 recvMultiplier
                                ) PURE;

    STDMETHOD(OnStreamDone)     (THIS_
                                HX_RESULT status,
                                UINT16 uStreamNumber
                                ) PURE;

    STDMETHOD(OnSourceDone)     (THIS) PURE;

    STDMETHOD(OnACK)            (THIS_
                                HX_RESULT status,
                                RTSPResendBuffer* pResendBuffer,
                                UINT16 uStreamNumber,
                                const char* pSessionID,
                                UINT16* pAckList,
                                UINT32 uAckListCount,
                                UINT16* pNakList,
                                UINT32 uNakListCount
                                ) PURE;

    STDMETHOD(OnProtocolError)  (THIS_
                                HX_RESULT status
                                ) PURE;
};

DECLARE_INTERFACE_(IHXRTSPServerTransportResponse, IHXRTSPTransportResponse)
{
    STDMETHOD(OnRTTRequest)     (THIS_
                                HX_RESULT status,
                                const char* pSessionID
                                ) PURE;

    STDMETHOD(OnRTTResponse)    (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                UINT32 ulSecs,
                                UINT32 ulUSecs
                                ) PURE;

    STDMETHOD(OnBWReport)       (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                INT32 aveBandwidth,
                                INT32 packetLoss,
                                INT32 bandwidthWanted
                                ) PURE;

    STDMETHOD(OnCongestion)     (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                INT32 xmitMultiplier,
                                INT32 recvMultiplier
                                ) PURE;

    STDMETHOD(OnStreamDone)     (THIS_
                                HX_RESULT status,
                                UINT16 uStreamNumber
                                ) PURE;

    STDMETHOD(OnSourceDone)     (THIS) PURE;

    STDMETHOD(OnACK)            (THIS_
                                HX_RESULT status,
                                RTSPResendBuffer* pResendBuffer,
                                UINT16 uStreamNumber,
                                const char* pSessionID,
                                UINT16* pAckList,
                                UINT32 uAckListCount,
                                UINT16* pNakList,
                                UINT32 uNakListCount
                                ) PURE;

    STDMETHOD(OnProtocolError)  (THIS_
                                HX_RESULT status
                                ) PURE;
};

DECLARE_INTERFACE_(IHXRTSPClientTransportResponse, IHXRTSPTransportResponse)
{
    STDMETHOD(OnRTTRequest)     (THIS_
                                HX_RESULT status,
                                const char* pSessionID
                                ) PURE;

    STDMETHOD(OnRTTResponse)    (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                UINT32 ulSecs,
                                UINT32 ulUSecs
                                ) PURE;

    STDMETHOD(OnBWReport)       (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                INT32 aveBandwidth,
                                INT32 packetLoss,
                                INT32 bandwidthWanted
                                ) PURE;

    STDMETHOD(OnCongestion)     (THIS_
                                HX_RESULT status,
                                const char* pSessionID,
                                INT32 xmitMultiplier,
                                INT32 recvMultiplier
                                ) PURE;

    STDMETHOD(OnStreamDone)     (THIS_
                                HX_RESULT status,
                                UINT16 uStreamNumber
                                ) PURE;

    /* This only indicates that all packets have been received from the
     * server. We still need to read packets from the transport buffer
     * StreamDone will indicate when there are no more packets to be
     * read from Transport buffer
     */
    STDMETHOD(OnSourceDone)     (THIS) PURE;

    STDMETHOD(OnACK)            (THIS_
                                HX_RESULT status,
                                RTSPResendBuffer* pResendBuffer,
                                UINT16 uStreamNumber,
                                const char* pSessionID,
                                UINT16* pAckList,
                                UINT32 uAckListCount,
                                UINT16* pNakList,
                                UINT32 uNakListCount
                                ) PURE;

    STDMETHOD(OnProtocolError)  (THIS_
                                HX_RESULT status
                                ) PURE;
};

DEFINE_GUID(IID_IHXPacketResend,     0x00000400, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

DECLARE_INTERFACE_(IHXPacketResend, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(OnPacket)         (THIS_
                                UINT16 uStreamNumber,
                                BasePacket** ppPacket) PURE;
};

DEFINE_GUID(IID_IHXRTSPContext,     0x00000401, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

DECLARE_INTERFACE_(IHXRTSPContext, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(GetCurrentSequenceNumber)
    (
        THIS_
        REF(UINT32) ulSequenceNumber
    ) PURE;

    STDMETHOD(GetSessionID)
    (
        THIS_
        REF(IHXBuffer*) pIHXBufferSessionID
    ) PURE;
};

DEFINE_GUID(IID_IHXTimeStampSync,     0x00000402, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

DECLARE_INTERFACE_(IHXTimeStampSync, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(SetInitialTS(const char* pSessionID, UINT32 ulInitialTS)) PURE;
    STDMETHOD_(HXBOOL, NeedInitialTS(const char* pSessionID)) PURE;
    STDMETHOD_(UINT32, GetInitialTS(const char* pSessionID)) PURE;
    STDMETHOD(ClearInitialTS(const char* pSessionID)) PURE;
};


DEFINE_GUID(IID_IHXTransportSyncServer, 0x16b420d0, 0xf4d0, 0x11d5, 0xaa,
            0xc0, 0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

DECLARE_INTERFACE_(IHXTransportSyncServer, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(DistributeSyncAnchor) (THIS_
                                    ULONG32 ulHXTime,
                                    ULONG32 ulNTPTime) PURE;

    STDMETHOD(DistributeSync)       (ULONG32 ulHXTime,
                                    LONG32 lHXTimeOffset) PURE;

    STDMETHOD(DistributeStartTime)  (ULONG32 ulHXRefTime) PURE;
};

DECLARE_INTERFACE_(IHXRTSPServerPauseResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(HandlePauseRequest)       (THIS_ const char* pSessionID,
                                           UINT32 ulPausePoint) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXRTSPClientProtocol)
DEFINE_SMART_PTR(IHXRTSPClientProtocol2)

#endif /* _RTSPIF_H_ */
