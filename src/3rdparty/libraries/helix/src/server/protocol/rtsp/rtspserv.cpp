/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspserv.cpp,v 1.262 2007/04/26 11:02:13 srao Exp $
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

#include "hxtypes.h"

#include <stdio.h>
#include <stdlib.h>

#include "hxcom.h"
#include "sockio.h"
#include "timeval.h"
#include "hxstring.h"
#include "hxstrutl.h"
#include "hxbuffer.h"
#include "hxsbuffer.h"
#include "servchallenge.h"
#include "rartp.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxmarsh.h"
#include "chxpckts.h"
#include "netbyte.h"
#include "hxengin.h"
#include "nettypes.h"
#include "hxnet.h"
#include "ihxpckts.h"
#include "chxrtsptransocket.h"
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxcomm.h"
#include "hxplugn.h"
#include "hxsdesc.h"
#include "hxmon.h"
#include "hxprefs.h"
#include "hxencod.h"
#include "plgnhand.h"
#include "chxpckts.h"
#include "growingq.h"
//#include "rtmlpars.h"
#include "mimehead.h"
#include "mimescan.h"
#include "timerep.h"
#include "rtspmsg.h"
#include "rtsppars.h"
#include "asmrulep.h"
#include "srcerrs.h"
#include "hxerror.h"
#include "pna2.h"      // for Player validation
#include "debug.h"
#include "hxpiids.h"
#include "altserv.h"

#include "configwatcher.h"
#include "rtspserv.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "transport.h"
#include "servlist.h"

#include "transportparams.h"

#include "rtspif.h"
#include "rtsptran.h"
#include "rdt_base.h"
#include "rdt_tcp.h"
#include "rdt_udp.h"
#include "rtp_base.h"
#include "rtp_tcp.h"
#include "rtp_udp.h"
#include "nulltran.h"
#include "ihxlist.h"
#include "hxlistp.h"
#include "mime2.h"
#include "rtspmsg2.h"
#include "rtspserv.h"
#include "rtsputil.h"
#include "sdpstats.h"
#include "rtspstats.h"
#include "hxurl.h"
#include "trmimemp.h"
#include "urlutil.h"

#include "nptime.h"
#include "smpte.h"
#include "hxbuffer.h"
#include "hxtick.h"

#include "bdst_stats.h"
#include "bcngtypes.h"
#include "bcngtran.h"

#include "hxstats.h"

#include "defslice.h"

#include "hxstreamadapt.h"

#include "chxrtcptransmapsocket.h"

#ifdef _WINCE
#include <wincestr.h>
#endif

#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#endif

const UINT16 FINISH_SETUP = 1;
const UINT16 FINISH_PLAYNOW = 2;

const UINT32 MAX_PACKET_READ_COUNT = 100;
static const char* z_pRARTPChallengeMimeTypes[] =
{
    "audio/x-pn-realaudio"
};

// Max host length, including null terminator (see IETF STD0013)
// This should be in a common header but I can't find it
#define MAX_HOST_LEN 256

// read/readDone recursion breaker limits
static const int MAX_READ_RECURSION_DEPTH = 8;

// max delay 1hr. hoping that the bad clients will disconnect by then
static const int MAX_DELAY_MS = 3600000;

// default player keepalive interval (s)
static const int PLAYER_KEEPALIVE_INTERVAL_DEFAULT = 80;

// prevDelayFactor = DelayFactor = 0.0;
// loop
// {
//    DelayFactor = prevDelayFactor * DELAY_FACTOR;
//    mayb delay read by DelayFactor * 1000 ms
//    prevDelayFactor = DelayFactor;
// }
static const float DEFAULT_DELAY_FACTOR = 1.1F;

// start delaying the reads after DELAY_START number of succesive
// MAX_READ_RECURSION_DEPTH reads, which in this case will b
// 4 iterations of 8 recursive reads == 32 reads
static const int DEFAULT_DELAY_START = 4;

// The floor in seconds as defined in the spec for the Stats Interval
static const INT32 STATS_INTERVAL_FLOOR = 15;

// The key for the config property in the config file
static const char* CONFIG_ROOT_KEY = "config";
// The key for the StatsInterval property in the config file
static const char* STATS_INTERVAL_KEY = "config.StatsInterval";

// Timestamp indicating the end of a static stream
static const UINT32 END_OF_STREAM_TS = (UINT32)~0;


//ABD defines, please refer to abdbuf.h if these values need to be changed
static const UINT32 MIN_ABD_PACKETS = 1;
static const UINT32 MAX_ABD_PACKETS = 30;
static const UINT32 MAX_ABD_PACKET_SIZE = 1500;
static const UINT32 MIN_ABD_PACKET_SIZE = 200;

#if defined _WINDOWS && !defined snprintf
#define snprintf _snprintf
#endif

// For propwatch as singleton
RTSPServerProtocol::CPropWatchResponse**
RTSPServerProtocol::CPropWatchResponse::zm_ppInstance = NULL;

IHXThreadLocal* RTSPServerProtocol::CPropWatchResponse::zm_pThreadLocal = NULL;
INT32 RTSPServerProtocol::CPropWatchResponse::zm_nStartupSemaphore = -1;

/// g_szAllMethods lists supported methods for Public header in OPTIONS response
static const char* g_szAllMethods = "OPTIONS, DESCRIBE, PLAY, PAUSE, SETUP, GET_PARAMETER, SET_PARAMETER, TEARDOWN, PLAYNOW";

/**
 * \brief        Allowed methods in string form for RTSP Allow header
 *
 * The allowed methods can vary according to resource or perhaps session 
 * type (announced vs setup for example) so this string and the state table
 * itself should be per session not per RTSP control connection or global.
 */
static const char* g_szAllowedMethods[4] =
{
    "OPTIONS, DESCRIBE, SETUP, GET_PARAMETER, SET_PARAMETER",     // INIT
    "OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, "                     // READY
        "TEARDOWN, GET_PARAMETER, SET_PARAMETER",
    "OPTIONS, DESCRIBE, PLAY, PAUSE, TEARDOWN, "                  // PLAY
        "GET_PARAMETER, SET_PARAMETER, PLAYNOW",
    ""                                                            // REC
};

// Allowed methods in flag form for protocol state table lookup
static BOOL g_bAllowedMethods[14][4] =
{
    //INIT   READY  PLAY   REC
    //=====  =====  =====  =====
    { FALSE, FALSE, FALSE, FALSE },     // RTSP_VERB_NONE
    { TRUE,  TRUE,  FALSE, FALSE },     // RTSP_VERB_ANNOUNCE
    { TRUE,  TRUE,  TRUE,  TRUE  },     // RTSP_VERB_DESCRIBE
    { TRUE,  TRUE,  TRUE,  TRUE  },     // RTSP_VERB_GETPARAM
    { TRUE,  TRUE,  TRUE,  TRUE  },     // RTSP_VERB_OPTIONS
    { FALSE, TRUE,  TRUE,  TRUE  },     // RTSP_VERB_PAUSE
    { FALSE, TRUE,  TRUE,  FALSE },     // RTSP_VERB_PLAY
    { FALSE, TRUE,  FALSE, TRUE  },     // RTSP_VERB_RECORD
    { FALSE, FALSE, FALSE, FALSE },     // RTSP_VERB_REDIRECT
    { TRUE,  TRUE,  FALSE, FALSE },     // RTSP_VERB_SETUP
    { TRUE,  TRUE,  TRUE,  TRUE  },     // RTSP_VERB_SETPARAM
    { TRUE,  TRUE,  TRUE,  TRUE  },     // RTSP_VERB_TEARDOWN
    { FALSE, FALSE, TRUE, FALSE  },      // RTSP_VERB_PLAYNOW
    { TRUE,  TRUE,  TRUE,  TRUE  }      // RTSP_VERB_EXTENSION
};

// State transition table (used only if handler is successful)
static RTSPServerProtocol::State g_nNextState[14][4] =
{
    //INIT                           READY                          PLAY                           REC
    //=========                      =========                      =========                      =========
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_NONE
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_ANNOUNCE
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_DESCRIBE
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_GETPARAM
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_OPTIONS
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::READY,     RTSPServerProtocol::READY,     RTSPServerProtocol::READY     },     // RTSP_VERB_PAUSE
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::PLAYING,   RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_PLAY
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::RECORDING, RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_RECORD
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_REDIRECT
    { RTSPServerProtocol::READY,     RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_SETUP
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_SETPARAM
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_TEARDOWN
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_PLAYNOW
    { RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE,      RTSPServerProtocol::NONE      },     // RTSP_VERB_EXTENSION
};

/*
 * This defines the transports we support.  It is used in the session's
 * selectTransport() function.
 */
#define IS_SUPPORTED_TRANSPORT(t) \
    ((t) > RTSP_TR_NONE && (t) < RTSP_TR_LAST && \
    (t) != RTSP_TR_RTP_MCAST && (t) != RTSP_TR_RTCP)

static UINT8 RTSPTransportPriorityTable[] =
{
    4,     /* RTSP_TR_NONE */
    4,     /* RTSP_TR_RDT_MCAST */
    4,     /* RTSP_TR_RDT_UDP */
    4,     /* RTSP_TR_RDT_TCP */
    4,     /* RTSP_TR_TNG_UDP */
    4,     /* RTSP_TR_TNG_TCP */
    4,     /* RTSP_TR_TNG_MCAST */
#if defined(HELIX_FEATURE_SERVER_PREFER_RTP)
    3,     /* RTSP_TR_RTP_UDP  */
    3,     /* RTSP_TR_RTP_MCAST */
    3,     /* RTSP_TR_RTP_TCP */
#else
    4,     /* RTSP_TR_RTP_UDP  */
    4,     /* RTSP_TR_RTP_MCAST */
    4,     /* RTSP_TR_RTP_TCP */
#endif
    4,     /* RTSP_TR_RTCP */
    4,     /* RTSP_TR_NULLSET */
    4,     /* RTSP_TR_BCNG_UDP */
    4,     /* RTSP_TR_BCNG_MCAST */
    4     /* RTSP_TR_BCNG_TCP */
};


/**
 * \class RTSPServerProtocol
 * \brief This class implements the RTSP control channel.
 *
 * There is one RTSPServerProtocol per RTSP control connection. It handles the 
 * TCP socket, parses RTSP messages received and manages the dialogue with the
 * remote end (client, proxy, splitter or encoder).
 *
 * \par
 * Currently there is only one kind of RTSP Session, with state differentiating
 * between the various conversations. We need to evolve to a model where we have
 * a variety of sessions each handling their own conversations with the remote
 * end and with the RTSPServerProtocol acting as a referee.
 */
 
STDMETHODIMP
RTSPServerProtocol::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXTCPResponse))
    {
        AddRef();
        *ppvObj = (IHXTCPResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXChallenge))
    {
        AddRef();
        *ppvObj = (IHXChallenge*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXTimeStampSync))
    {
        AddRef();
        *ppvObj = (IHXTimeStampSync*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileFormatHeaderAdviseResponse))
    {
        AddRef();
        *ppvObj = (IHXFileFormatHeaderAdviseResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
RTSPServerProtocol::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RTSPServerProtocol::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

#define QUEUE_START_SIZE    512

#ifdef PAULM_RTSPPROTTIMING
#include "classtimer.h"

void
_ExpiredRTSPServerProtocolCallback(void* p)
{
    RTSPServerProtocol* pProt = (RTSPServerProtocol*)p;
    printf("\tm_bIWillDieSoon: %d\n",
        pProt->m_bIWillDieSoon);
    printf("\tm_pSocket: 0x%x\n", pProt->GetSocket());
}

ClassTimer g_RTSPProtTimer("RTSPServerProtocol",
        _ExpiredRTSPServerProtocolCallback, 3600);
#endif

/**
 * \brief RTSPServerProtocol Constructor 
 * \param bDisableResend   [in] Disables resends when transport is RDTvUDP.
 * \param bSendLostPackets [in] RDTvTCP won't forward "Lost Packet" to client.
 *
 * Both parameters are inappropriate in this constructor and should be moved to
 * the transport.
 *
 * \return              n/a
 */
RTSPServerProtocol::RTSPServerProtocol(BOOL bDisableResend,
                                        BOOL bSendLostPackets):
    m_ulRefCount(0),
    m_pResp(0),
    m_pScheduler(0),
    m_pIScheduler(0),
    m_pAccurateClock(0),
    m_pRegistry(0),
    m_nUnusedSessions(0),
    m_ulLastSeqNo(0),
    m_bSetupRecord(FALSE),
    m_iStatsMask(0),
    m_iDebugLevel(0),
    m_pDebugFile(NULL),
    m_iTimingLevel(0),
    m_pTimingFile(NULL),
    m_bAuthenticationFailed(FALSE),
    m_tcpInterleave(0),
    m_bIWillDieSoon(FALSE),
    m_pRealChallenge(0),
    m_bChallengeDone(FALSE),
    m_bChallengeMet(FALSE),
    m_ulChallengeInitMethod(RTSP_VERB_NONE),
    m_bRARTPChallengeMet(FALSE),
    m_bRARTPChallengePending(FALSE),
    m_bSendClientRealChallenge3(TRUE),
    m_bIsValidChallengeT(FALSE),
    m_pOptionsResponseHeaders(NULL),
    m_pProxyPeerAddr(NULL),
    m_pProxyLocalAddr(NULL),
    m_pErrorMessages(0),
    m_bDisableResend(bDisableResend),
    m_bProxyResponseSent(FALSE),
    m_sdpFileType(NONE_SDP),
    m_bSendLostPackets(bSendLostPackets)
    , m_bIsLocalBoundSocket(FALSE)
    , m_ulThreadSafeFlags(HX_THREADSAFE_METHOD_SOCKET_READDONE)
    , m_ulRegistryConnId(0)
    /* XXX: aak - Sun Jul 22 14:02:46 PDT 2001
     * set 'this' as a response until all of the RTSPServerProt is fixed
     */
    , m_pResponse2(0)
    , m_pConsumer(0)
    , m_pBufFrag(0)
    , m_bPlayReceived(FALSE)
    , m_bNoRtpInfoInResume(FALSE)
    , m_pFastAlloc(0)
    , m_pSessionIDTmp(NULL)
    , m_pPropWatchResponse(NULL)
    , m_clientType(PLAYER_CLIENT)
    , m_lTurboPlayBW(-1)
    , m_bDisableTurboPlay(FALSE)
    , m_bTrackEvents(FALSE)
    , m_bIsProxy(FALSE)
    , m_pSDPStatsMgr(NULL)
    , m_pAggStats(NULL)
    , m_pRtspStatsMgr(NULL)
    , m_pClientStatsMgr(NULL)
    , m_bUseRegistryForStats(FALSE)
    , m_bIsClientStatsObjIdSet(FALSE)
    , m_ulClientStatsObjId(0)
    , m_pSessionStatsObjIDs(NULL)
    , m_bPVEmulationEnabled(FALSE)
    , m_pPVClientUAPrefix(NULL)
    , m_pPVServerId(NULL)
    , m_pRTCPInterval(NULL)
    , m_tKeepAliveInterval(0)
    , m_ulKeepAliveCallbackID(0)
    , m_bRTSPPingResponsePending(FALSE)
    , m_bIsStreamAttemptCounted(FALSE)
    , m_pRespMsg(NULL)
    , m_LastRequestMethod(RTSP_UNKNOWN)
    , m_pWriteNotifyMap(NULL)
    , m_pWriteNotifyLastKey(NULL)
    , m_bSendBWPackets(FALSE)
    , m_bRTCPRRWorkAround(TRUE)
    , m_bRTPLiveLegacyMode(FALSE)
    , m_bOriginUsesTrackID(FALSE)
    , m_bPN3Enabled(TRUE)
{
#ifdef PAULM_RTSPPROTTIMING
    g_RTSPProtTimer.Add(this);
#endif

    m_pSessions = new CHXMapStringToOb;
    m_pSessionStatsObjIDs = new CHXMapStringToOb;
    m_pKeepAlivePendingMessages = new CHXMapLongToObj;
    memset(reg_id, 0, sizeof(UINT32)*MAX_FIELDS);

    /*
     * This list is used only for handling record TCP data packets
     */

    m_pSessionIDList = new CHXMapLongToObj;
    m_pPipelineMap = new CHXMapLongToObj;
}

/**
 * \brief RTSPServerProtocol Destructor
 *
 * RTSPServerProtocol Destructor - called when we are done with this RTSP 
 * control connection. This implies termination of all associated RTSP 
 * sessions and resources related to those sessions because we do not support
 * transient control connections.
 *
 * \param n/a
 *
 * \return n/a
 */

RTSPServerProtocol::~RTSPServerProtocol()
{
#ifdef PAULM_RTSPPROTTIMING
    g_RTSPProtTimer.Remove(this);
#endif

    if (m_pTimingFile != NULL)
    {
        if (m_pTimingFile != stdout)
        {
            fclose(m_pTimingFile);
        }
    }
    if (m_pDebugFile != NULL)
    {
        if (m_pDebugFile != stdout)
        {
            fclose(m_pDebugFile);
        }
    }

    m_pSessionManager->removeSessionInstances(this);
    HX_RELEASE(m_pSessionManager);

    HX_RELEASE(m_pPropWatchResponse);

    /*
     * m_pContext and m_pSocket should have already been released
     * from the Done() function call
     */
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pFastSocket);
    HX_RELEASE(m_pResp);
    HX_RELEASE(m_pResponse2);

    HX_DELETE(m_pRealChallenge);

    CHXMapLongToObj::Iterator i;
    for (i = m_pSessionIDList->Begin(); i != m_pSessionIDList->End(); ++i)
    {
        char* pSessionID = (char*)(*i);
        HX_VECTOR_DELETE(pSessionID);
    }
    HX_DELETE(m_pSessionIDList);

    HX_DELETE(m_pPipelineMap);

    for (i = m_pKeepAlivePendingMessages->Begin();
         i != m_pKeepAlivePendingMessages->End(); ++i)
    {
        char* pSessionID = (char*)(*i);
        HX_VECTOR_DELETE(pSessionID);
    }
    HX_DELETE(m_pKeepAlivePendingMessages);

    CHXMapStringToOb::Iterator j;
    for (j = m_pSessionStatsObjIDs->Begin(); j != m_pSessionStatsObjIDs->End(); ++j)
    {
        UINT32* ulObjId= (UINT32*)(*j);
        HX_DELETE(ulObjId);
    }
    HX_DELETE(m_pSessionStatsObjIDs);

    HX_RELEASE(m_pOptionsResponseHeaders);
    if (m_ulKeepAliveCallbackID)
    {
        HX_ASSERT(FALSE);   // Should have been deleted in Done()
        m_pIScheduler->Remove(m_ulKeepAliveCallbackID);
        m_ulKeepAliveCallbackID = 0;
    }

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pIScheduler);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pBufFrag);
    HX_RELEASE(m_pConsumer);
    HX_RELEASE(m_pFastAlloc);
    delete [] m_pSessionIDTmp;

    HX_VECTOR_DELETE(m_pRTCPInterval);

    if (m_bTrackEvents)
    {
        HX_RELEASE(m_pRtspStatsMgr);
    }

    HX_RELEASE(m_pSDPStatsMgr);
    HX_RELEASE(m_pAggStats);

    HX_RELEASE(m_pClientStatsMgr);
    HX_RELEASE(m_pPVClientUAPrefix);
    HX_RELEASE(m_pPVServerId);

    HX_RELEASE(m_pRespMsg);

    HX_RELEASE(m_pProxyLocalAddr);
    HX_RELEASE(m_pProxyPeerAddr);
}

/**
 * \brief Init - provide a context and initialize the services and variables that will control this RTSP connection.
 *
 * \param pContext [in] : the context for the streamer this RTSP connection is running in.
 *
 * \return HXR_OK if successful
 */

STDMETHODIMP
RTSPServerProtocol::Init(IUnknown* pContext)
{
    HX_RESULT hresult = HXR_OK;

    m_pContext = pContext;
    m_pContext->AddRef();

    m_pSessionManager = new RTSPServerSessionManager;
    m_pSessionManager->AddRef();

    HX_RELEASE(m_pPropWatchResponse);
    m_pPropWatchResponse = CPropWatchResponse::Instance(m_pContext);
    // COM Rules, return a pointer, so already addref'd

    HX_RELEASE(m_pCommonClassFactory);
    hresult = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                                         (void**) &m_pCommonClassFactory);

    if (HXR_OK != hresult)
    {
        return hresult;
    }

    hresult = m_pContext->QueryInterface(IID_IHXErrorMessages,
                                         (void**)&m_pErrorMessages);

    if (HXR_OK != hresult)
    {
        return hresult;
    }

    HX_RELEASE(m_pScheduler);

    hresult = m_pContext->QueryInterface(IID_IHXScheduler,
                                         (void**)&m_pScheduler);

    if (HXR_OK != hresult)
    {
        return hresult;
    }

    HX_RELEASE(m_pIScheduler);
    hresult = m_pContext->QueryInterface(IID_IHXThreadSafeScheduler,
                                         (void**)&m_pIScheduler);
    if (HXR_OK != hresult)
    {
        return hresult;
    }

    // keepalive setup

    HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
    m_tActiveStamp = rmatv.tv_sec;

    HX_RELEASE(m_pAccurateClock);
    m_pContext->QueryInterface(IID_IHXAccurateClock,
                               (void**)&m_pAccurateClock);
    if (m_pAccurateClock == NULL)
    {
        // System does not have an efficient clock so use our own
        AddRef();
        m_pAccurateClock = this;
    }

    HX_RELEASE(m_pFastAlloc);

    hresult = m_pContext->QueryInterface(IID_IHXFastAlloc,
                                         (void**)&m_pFastAlloc);

    if (HXR_OK != hresult)
    {
        return hresult;
    }

    HX_RELEASE(m_pClientStatsMgr);

    if (FAILED(hresult = m_pContext->QueryInterface(IID_IHXClientStatsManager,
                                                    (void**)&m_pClientStatsMgr)))
    {
        return hresult;
    }


    IHXRegistry* pRegistry = NULL;
    hresult = m_pContext->QueryInterface(IID_IHXRegistry,
                                         (void**)&pRegistry);
    HX_VERIFY(HXR_OK == hresult);
    if (hresult == HXR_OK)
    {
        IHXBuffer* pBuf = NULL;

        m_pRegistry = pRegistry;

        m_pRegistry->GetIntByName("config.RTSPMessageDebug", m_iDebugLevel);
        if (m_iDebugLevel > 0)
        {
            m_pRegistry->GetStrByName("config.RTSPMessageDebugFile", pBuf);
            if (pBuf != NULL)
            {
                m_pDebugFile = fopen((const char*)pBuf->GetBuffer(), "a");
                HX_RELEASE(pBuf);
            }
            if (m_pDebugFile == NULL)
            {
                m_pDebugFile = stdout;
            }
        }

        m_pRegistry->GetIntByName("config.RTSPTiming", m_iTimingLevel);
        if (m_iTimingLevel > 0)
        {
            m_pRegistry->GetStrByName("config.RTSPTimingFile", pBuf);
            if (pBuf != NULL)
            {
                m_pTimingFile = fopen((const char*)pBuf->GetBuffer(), "a");
                HX_RELEASE(pBuf);
            }
            if (m_pTimingFile == NULL)
            {
                m_pTimingFile = stdout;
            }
            HXTimeval now = m_pAccurateClock->GetTimeOfDay();
            fprintf(m_pTimingFile, "RTSP:%p,%u.%06u,INIT\n", this,
                    now.tv_sec, now.tv_usec);
        }

        m_pRegistry->GetIntByName("config.StatsMask", m_iStatsMask);

        INT32 nVal=0;
        if (HXR_OK ==
            m_pRegistry->GetIntByName("config.ThreadSafeSockets", nVal))
        {
            m_ulThreadSafeFlags = (UINT32)nVal;
        }

        // For static content, the server cann't dictitate turboplay, so
        // on or off doesn't affect the playback.
        // For live case, the server need to explicitly turn it on for turboplay
        // to go into effect. We only want to turn it on when DisableTurboPlay is off.
        INT32 nTmp = 0;
        m_pRegistry->GetIntByName("config.DisableTurboPlay", nTmp);
        m_bDisableTurboPlay = nTmp ? TRUE : FALSE;
        // is set to 0.

        m_pRegistry->GetIntByName("config.TurboPlayBW", m_lTurboPlayBW);

        nTmp = 1;
        m_pRegistry->GetIntByName("config.PVEmulation.RTCPRRWorkAround", nTmp);
        m_bRTCPRRWorkAround = (BOOL)nTmp;

        /*
         * XXXMC
         * Special-case handling for PV clients behind a NAT/firewall.
         */
     
        nTmp = 0;
        m_pRegistry->GetIntByName("config.PVEmulation.Enabled", nTmp);


        if (nTmp &&
            SUCCEEDED(m_pRegistry->GetStrByName("config.PVEmulation.ClientUAPrefix",
                                                m_pPVClientUAPrefix)) &&
            SUCCEEDED(m_pRegistry->GetStrByName("config.PVEmulation.ServerId",
                                                m_pPVServerId)))
        {
            m_bPVEmulationEnabled = TRUE;
        }

        nTmp = 0;
        if (SUCCEEDED(m_pRegistry->GetIntByName("config.RTCPInterval", nTmp)))
        {
            m_pRTCPInterval = new char [32];
            snprintf(m_pRTCPInterval, sizeof(m_pRTCPInterval), "%d", (int)nTmp);
        }
        else
        {
            m_pRTCPInterval = NULL;
        }

        nTmp = 0;
        if ((IsTCPPrefLicensed() || LICENSE_TCP_PREF_ENABLED) && 
            SUCCEEDED(m_pRegistry->GetIntByName("config.Protocols.RTSP.PreferClientTCP", nTmp)))
        {
            if (nTmp)
            {
                //Reset the transport priorities to prefer TCP
                RTSPTransportPriorityTable [RTSP_TR_RDT_TCP] = 1;
                RTSPTransportPriorityTable [RTSP_TR_TNG_TCP] = 2;
                RTSPTransportPriorityTable [RTSP_TR_RTP_TCP] = 2;

            }
        }

        // keepalive
        nTmp = PLAYER_KEEPALIVE_INTERVAL_DEFAULT;
        m_pRegistry->GetIntByName("config.KeepAliveInterval", nTmp);
        m_tKeepAliveInterval = (time_t)nTmp;

        // RTP Live RTP-Info
        nTmp = 0;
        m_pRegistry->GetIntByName("config.RTPLiveLegacyMode", nTmp);
        m_bRTPLiveLegacyMode = nTmp ? TRUE : FALSE;


	//ABD- AutoBandwidthDetection
	INT32 itmp = 0;
        if(HXR_OK == m_pRegistry->GetIntByName("config.AutoBWDetection.SendBWPackets", itmp))
	{
	    m_bSendBWPackets = (BOOL)itmp;
	}
	
	// PlayNow RTSP enhancements enabled
        if(HXR_OK == m_pRegistry->GetIntByName("config.PlayNow3.Enabled", itmp))
	{
	    m_bPN3Enabled = (BOOL)itmp;
	}
	
    }

    // m_bTrackEvents defaults to FALSE.
    // If RTSPStats is configured, then licensing/enabled flags will be valid and should
    // be checked. m_bTrackEvents can then be correctly configured.

    // If RTSPStats is not configured, we need to try going through the
    // initialization step at least once, which involves creating and initing the RTSPStats
    // class once. This will configure the feature for future use.

    if (!RTSPStats::FeatureConfigured()
    ||  (RTSPStats::FeatureLicensed() && RTSPStats::FeatureEnabled()))
    {
        m_bTrackEvents = m_bIsProxy ? FALSE : TRUE;
    }

    if (m_bTrackEvents)
    {
        m_pRtspStatsMgr = new RTSPStats;
        m_pRtspStatsMgr->AddRef();

        if (HXR_OK != m_pRtspStatsMgr->Init(pContext))
        {
            m_bTrackEvents = FALSE;
            HX_RELEASE(m_pRtspStatsMgr);
        }
    }

    IHXRTSPEventsManager* pEventMgr = NULL;

    if (FAILED(m_pContext->QueryInterface(IID_IHXRTSPEventsManager, (void**)&pEventMgr)))
    {
        hresult = HXR_UNEXPECTED;
        return hresult;
    }

    m_pAggStats = pEventMgr->GetAggregateStats();

    HX_RELEASE(pEventMgr);

    if (!m_pAggStats)
    {
        HX_ASSERT(!"RTSPSvrProt::Init() expected aggregate stats obj!");
        hresult = HXR_UNEXPECTED;
        return hresult;
    }


    if (FAILED(pContext->QueryInterface(IID_IHXSDPAggregateStats,
                                       (void**)&m_pSDPStatsMgr)))
    {
        hresult = HXR_UNEXPECTED;
        return hresult;
    }

    //m_pSDPStatsMgr->Init(m_pContext);


    // Launch the keepalive callback, if needed
    if (m_tKeepAliveInterval)
    {
        KeepAliveCallback* pCB = new KeepAliveCallback(this);
        m_ulKeepAliveCallbackID =
            m_pIScheduler->RelativeEnter(pCB, m_tKeepAliveInterval*1000);
    }

    m_pWriteNotifyMap = new CHXMapPtrToPtr();

    return hresult;
}

/**
 * \brief GetSourceAddr - get the source address specified in the config file.
 *
 * Return the source address specified in the config file to the caller. If the
 * config file specifies "*", return the local address from the tcp socket for
 * the control channel, if not configured set pAddrBuf to the NULL pointer.
 * 
 *
 * \param pAddrBuf [out] : IHXBuffer that holds the source address.
 *
 * \return HXR_OK
 */

HX_RESULT
RTSPServerProtocol::GetSourceAddr(REF(IHXBuffer*) pAddrBuf)
{
    IHXBuffer* pConfigValue = NULL;
    if (m_pRegistry->GetStrByName("config.RTSPSourceAddress",
                                  pConfigValue) == HXR_OK)
    {
        const char* pUserValue = (const char*)pConfigValue->GetBuffer();
        if (strcmp(pUserValue, "*") != 0)
        {
            pAddrBuf = pConfigValue;
        }
        else
        {
            IHXSockAddr* pAddr = NULL;
            m_pSocket->GetLocalAddr(&pAddr);
            pAddr->GetAddr(&pAddrBuf);
            pAddr->Release();
        }
        pConfigValue->Release();
    }
    else
    {
        pAddrBuf = NULL;
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::SetBuildVersion(const char* pVersionString)
{
    m_pVersionString = pVersionString;
    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::SetOptionsRespHeaders(IHXValues* pHeaders)
{
    m_pOptionsResponseHeaders = pHeaders;
    m_pOptionsResponseHeaders->AddRef();

    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::SetPacketResend(IHXPacketResend* pPacketResend,
                                    const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    HX_ASSERT(pSession != NULL);

    HX_ASSERT(pSession->m_pPacketResend == NULL);
    pSession->m_pPacketResend = pPacketResend;
    if (pSession->m_pPacketResend)
    {
        pSession->m_pPacketResend->AddRef();
    }

    return HXR_OK;
}

/**
 * \brief Done - Schedule a callback to terminate this RTSP conversation.
 *
 * \param n/a
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::Done()
{
    /*
     * Need to schedule callback to take care of us.  And I do mean
     * "Take care" of us.
     */

    if (!m_bIWillDieSoon)
    {
        if (m_ulKeepAliveCallbackID)
        {
            m_pIScheduler->Remove(m_ulKeepAliveCallbackID);
            m_ulKeepAliveCallbackID = 0;
        }

        KillRTSPServerProtocolCallback* pC = new
            KillRTSPServerProtocolCallback(this);
        m_pScheduler->RelativeEnter(pC, 0);
        m_bIWillDieSoon = 1;
    }
    return HXR_OK;
}

/**
 * \brief PlayDone - set state of the session named to READY
 *
 * PlayDone is triggered by the Player object when all the streams are done or
 * there is an error condition of some sort. The idea is to set the session to
 * a 'ready' state so that the logic to resume playing works correctly.
 *
 * \param pSessionID [in] : the session that is 'done'
 *
 * \return void
 */
void
RTSPServerProtocol::PlayDone(const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = NULL;

    pSession = getSession(pSessionID);
    if (pSession != NULL)
    {
        State state = INIT;
        getState(pSessionID, state);
        if (state == PLAYING)
        {
            setState(pSessionID, READY);

            // Reset this so the resume logic does the Right Thing
            m_bPlayReceived = FALSE;
        }
    }
}

/**
 * \brief SessionDone - Release the session.
 *
 * Release a session.  This also eventually releases a reference to the
 * corresponding PPM::Session object (via the m_pPacketResend member).
 * Usually initiated by a player getting a teardown request.
 *
 *
 * jmevissen, 12/2000
 *
 * I also thought of putting RTSPProtocol->SessionDone() right into
 * RTSPProtocol::HandleTeardownRequest(), but then I got attempts to
 * write in RDTUDPTransport::writePacket() after the UDP socket was closed.
 * Rather than add a check in that routine for a non-NULL socket before every
 * write, we wait until the player session is destroyed before initiating
 * this call.  (See Player::Session::~Session().)
 *
 * \param pSessionID [in] : The session that is to be released
 *
 * \return void
 */

void
RTSPServerProtocol::SessionDone(const char* sessionID)
{
    RTSPServerProtocol::Session* pSession = getSession(sessionID);

    if (pSession)
    {
        if (m_bTrackEvents)
        {
            HX_ASSERT(pSession->m_pEventList);
            pSession->m_pEventList->DumpEvents(TRUE);
        }

        if (pSession->m_bUnused)
        {
            m_nUnusedSessions--;
        }
        m_pSessions->RemoveKey(sessionID);
        pSession->Done();
        pSession->Release();
    }
}

/**
 * \brief TrueDone - Release socket and response objects, we are done.
 *
 * TrueDone is called when the kill timer started in the Done() method pops.
 * It releases the socket and response objects and should result in the 
 * destruction of this RTSPServerProtocol object
 *
 * \param n/a
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPServerProtocol::TrueDone()
{
    if (m_pTimingFile != NULL)
    {
        HXTimeval now = m_pAccurateClock->GetTimeOfDay();
        fprintf(m_pTimingFile, "RTSP:%p,%u.%06u,DONE\n", this,
                now.tv_sec, now.tv_usec);
    }

    HX_RELEASE(m_pResp);
    HX_RELEASE(m_pResponse2);
    HX_RELEASE(m_pAccurateClock);
    HX_RELEASE(m_pContext);

    if (m_pSessions)
    {
        RTSPServerProtocol::Session* pSession;
        CHXMapStringToOb::Iterator i;
        for (i=m_pSessions->Begin();i!=m_pSessions->End();++i)
        {
            pSession = (RTSPServerProtocol::Session*)(*i);
            if (m_bTrackEvents)
            {
                HX_ASSERT(pSession->m_pEventList);
                pSession->m_pEventList->DumpEvents(TRUE);
            }
            pSession->Done();
            pSession->Release();
        }
        delete m_pSessions;
        m_pSessions = 0;
    }

    if (m_bTrackEvents)
    {
        m_pRtspStatsMgr->DumpEvents(TRUE);
    }

    for (CHXMapPtrToPtr::Iterator iter = m_pWriteNotifyMap->Begin();
            iter != m_pWriteNotifyMap->End(); ++iter)
    {
        IUnknown* punkItem = reinterpret_cast<IUnknown*>(*iter);
        HX_RELEASE( punkItem );
    }

    HX_DELETE( m_pWriteNotifyMap );
    m_pWriteNotifyLastKey = NULL;

    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pFastSocket);

    return HXR_OK;
}

/**
 * \brief SetControl - obsolete 
 *
 * SetControl - obsolete IHXRTSPServerProtocol method that was used by 
 * legacy encoders/splitters to pass socket and response object from a
 * plugin, enabling them to share the core RTSP handling code. A bad idea
 * whose time has gone.
 *
 * \param n/a
 *
 * \return HXR_NOTIMPL
 */
STDMETHODIMP
RTSPServerProtocol::SetControl(IHXSocket* pSocket,
    IHXRTSPServerProtocolResponse2* pResp,
    IHXBuffer* pBuffer)
{
    HX_ASSERT(FALSE);   // Should not happen! 
    return HXR_NOTIMPL;
}


/**
 * \brief AddSession - create a new session and add it to URL and Session ID mappings
 *
 * When the RTSPServerProtocol object needs to create a new session (typically
 * when an OPTIONS, SETUP or DESCRIBE request is received from the client), it
 * will call the response objects AddSession() method, which will cause a new
 * Player::Session object to be created and select a unique session id. The
 * response object will then invoke this method to create an RTSP session
 * and map the session id and URL to it.
 *
 * \param pSessionID [in] : the session id to associate with the new session
 * \param pURL [in] : the URL in the client request
 * \param ulSeqNo [in] : no longer used.
 *
 * \return HXR_OK if new session object successfully created, HXR_FAIL otherwise
 */
STDMETHODIMP
RTSPServerProtocol::AddSession(const char* pSessionID, const char* pURL,
    UINT32 ulSeqNo)
{
#if 0
    printf("RTSPServerProtocol(%p)::AddSession, %s\n", this, pSessionID);
#endif

    // Add the session to our map
    if (getSession(pSessionID,TRUE) == NULL)
    {
        return HXR_FAIL;
    }

    // Add the session to the session manager
    m_pSessionManager->addSessionInstance(pSessionID, pURL, ulSeqNo, this);

    return HXR_OK;
}

/**
 * \brief SetResponse - set the protocol response object and control channel socket
 *
 * RTSPProtocol calls this to set itself as the IHXRTSPServerProtocolResponse2
 * for this control channel. The protocol repsonse mediates between the 
 * RTSP objects and Player object, which controls the media pipeline.  
 *
 * \param pSocket [in] : the socket for this control channel!
 * \param pResp [in] : the protocol response object
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::SetResponse(IHXSocket* pSocket,
    IHXRTSPServerProtocolResponse2* pResp)
{
    m_pSocket = pSocket;
    m_pSocket->AddRef();
    m_pSocket->SetResponse(this);

    m_pSocket->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE|HX_SOCK_EVENT_WRITE);

    m_pSocket->QueryInterface(IID_IHXBufferedSocket,
        (void**)&m_pFastSocket);

    HX_ASSERT(m_pResp == NULL);
    HX_ASSERT(m_pResponse2 == NULL);

    m_pResp = pResp;
    m_pResp->AddRef();
    m_pResp->QueryInterface(IID_IHXRTSPProtocolResponse,
        (void **)&m_pResponse2);
    HX_ASSERT(m_pResponse2);

    m_bIsLocalBoundSocket = m_pSocket->GetFamily() == HX_SOCK_FAMILY_LBOUND ? TRUE : FALSE;

    return HXR_OK;
}

/**
 * \brief Disconnect - sets the RTSP session state so that it cannot be used
 *
 * Disconnect sets the RTSP session state to KILLED so that it cannot be used.
 * Once in the KILLED state only TEARDOWN requests are accepted.
 *
 * \param pSessionID [in] : The ID of the session to be disconnected.
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::Disconnect(const char* pSessionID)
{
    setState(pSessionID, KILLED);
    return HXR_OK;
}

/**
 * \brief SendAlertRequest - send a Helix Server Alert to the client
 *
 * SendAlertRequest sends a SET_PARAMETER request with an "Alert" header
 * that contains an error number and text string describing error that was
 * encountered. The session ID is not checked : it goes into a Session
 * header. We also try to change the state of the session to KILLED, which
 * means that it will only accept a TEARDOWN request from this point on.
 * This is a proprietary mechanism for supplying explanatory text about
 * server error conditions. 
 *
 * See common/include/srcerrs.h for the helix alert table.
 *
 * \param pSessionID [in] : the session associated with the error (if present)
 * \param lAlertNumber [in] : the location of the alert in the Helix alert table
 * \param pAlertText [in] : the alert text
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::SendAlertRequest(const char* pSessionID,
    INT32 lAlertNumber, const char* pAlertText)
{
    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
    pMsg->setURL("*");
    if (pSessionID && *pSessionID)
    {
        pMsg->addHeader("Session", pSessionID);
    }
    MIMEHeader* pAlertHeader = new MIMEHeader("Alert");
    char* pTmpBuf = new char[strlen(pAlertText)+41];

    sprintf(pTmpBuf, "%d;%s", (int)lAlertNumber, pAlertText);
    pAlertHeader->addHeaderValue(pTmpBuf);
    pMsg->addHeader(pAlertHeader);
    sendRequest(pMsg, ++m_ulLastSeqNo);

    OnServerRequestEvent(pMsg, pSessionID, NULL);

    setState(pSessionID, KILLED);

    delete[] pTmpBuf;
    return HXR_OK;
}

/**
 * \brief SendKeepAlive - send an RTSP request just to keep the channel open 
 *
 * SendKeepAlive with no arguments is part of IHXRTSPServerProtocol:
 *
 * \param n/a
 *
 * \return status of send attempt 
 */
STDMETHODIMP
RTSPServerProtocol::SendKeepAlive()
{
    return SendKeepAlive(NULL);
}

/**
 * \brief SendKeepAlive - send an RTSP request just to keep the channel open 
 *
 * SendKeepAlive with session id (even if "NULL") is our own method to 
 * send keepalive for a particular session:
 *
 * \param n/a
 *
 * \return HXR_OK unless we fail to create the request (should never happen)
 */
HX_RESULT
RTSPServerProtocol::SendKeepAlive(Session* pSession)
{
    // Modified to send OPTIONS by default; use SET_PARAMETER only
    // to work around the current proxy, which ignores OPTIONS.
    //    jmevissen, 7/2003

    HX_RESULT rc = HXR_OK;
    const char* szDefaultURL = "*";
    RTSPRequestMessage* pMsg;

    // pMsg created here will be deleted by DispatchMessage() once response
    // is received for this request.

    // the data connection is type midbox, the accounting connection has a Via.

    if (m_clientType == MIDBOX_CLIENT ||
        (pSession && pSession->m_bIsViaRealProxy))
    {
        pMsg = new RTSPSetParamMessage;
        if (pMsg) pMsg->addHeader("Ping", "Pong");
    }
    else
    {
        pMsg = new RTSPOptionsMessage(FALSE);
    }

    if (pMsg)
    {
        pMsg->setURL(szDefaultURL);

        if (pSession)
        {
            // the r1 client doesn't send back the session id in the
            // response, so we keep track via the sequence number.

            pMsg->addHeader("Session", pSession->m_sessionID);
            (*m_pKeepAlivePendingMessages)[m_ulLastSeqNo+1] =
                new_string(pSession->m_sessionID);
        }
        sendRequest(pMsg, ++m_ulLastSeqNo);

        OnServerRequestEvent(pMsg, NULL, pSession);
    }
    else
    {
        rc = HXR_FAIL;
    }
    return rc;
}

/**
 * \brief SendTeardownRequest - send a TEARDOWN request to the client
 *
 * SendTeardownRequest sends an RTSP TEARDOWN request with the given 
 * session id in a "Session" header. The session id is assumed to be
 * valid. The URL used is "*"
 *
 * \todo Understand why we use the shared base class sendRequest()
 * method instead of using the new CRTSPRequestMessage with our own
 * method to send a request to the client. Future cleanup?
 *
 * \param n/a
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::SendTeardownRequest(const char* pSessionID)
{
    RTSPTeardownMessage* pMsg = new RTSPTeardownMessage;
    pMsg->setURL("*");
    pMsg->addHeader("Session", pSessionID);
    sendRequest(pMsg, ++m_ulLastSeqNo);

    OnServerRequestEvent(pMsg, pSessionID, NULL);

    return HXR_OK;
}

/**
 * \brief SendRedirectRequest - send a REDIRECT request to the client
 *
 * SendRedirectRequest sends an RTSP REDIRECT request with the given 
 * session id in a "Session" header. The session id is verified to be
 * valid. The URL passed in by the caller is used without verification in 
 * the "Location header in the request. The URL send in the request is 
 * the base URL established for this session ... its not checked either.
 *
 * \param pSessionID [in]
 * \param pURL [in] : the URL to redirect to.
 * \param mSecsFromNow [in] : sent out in a "Range" header in the request
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::SendRedirectRequest(const char* pSessionID,
    const char* pURL, UINT32 mSecsFromNow)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    if (!pSession)
    {
        return HXR_FAILED;
    }

    RTSPRedirectMessage* pMsg = new RTSPRedirectMessage;
    pMsg->setURL(pSession->m_describeURL);
    pMsg->addHeader("Session", pSessionID);
    pMsg->addHeader("Location", pURL);
    RTSPRange range(mSecsFromNow, RTSP_PLAY_RANGE_BLANK, RTSPRange::TR_NPT);
    pMsg->addHeader("Range", (const char*)range.asString());
    sendRequest(pMsg, ++m_ulLastSeqNo);

    OnServerRequestEvent(pMsg, NULL, pSession);

    return HXR_OK;
}

/**
 * \brief SendProxyRedirectRequest - send a "305 Use Proxy" response to client
 *
 * SendProxyRedirectRequest sends  a "305 Use Proxy" response to the client.
 * The URL passed in by the caller is used without verification in 
 * the "Location header in the response, this is the location of the proxy 
 * that the client should use.
 *
 * \param pSessionID [in]
 * \param pURL [in] : the proxy the client should use for this request.
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::SendProxyRedirectRequest(const char* pSessionID,
    const char* pURL)
{
    IHXMIMEHeader* pHeader;

    pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHeader->SetFromString("Location", pURL);
    m_pRespMsg->AddHeader(pHeader);
    SendResponse(305);
    m_bProxyResponseSent = TRUE;

    return HXR_OK;
}

/**
 * \brief SendGetParameterRequest - send a GET_PARAMETER request to the client
 *
 * \param lParamType [in] : not used, hard-coded to "text/rtsp-parameters"
 * \param pParamName [in] : sent in entity body of request
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::SendGetParameterRequest(UINT32 lParamType,
    const char* pParamName)
{
    RTSPGetParamMessage* pMsg = new RTSPGetParamMessage;
    pMsg->setURL("*");
    sendRequest(pMsg, pParamName, "text/rtsp-parameters", ++m_ulLastSeqNo);

    OnServerRequestEvent(pMsg, NULL, NULL);

    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::SendSetParameterRequest(const char* pSessionID,
                                            const char* pURL,
                                            const char* pParamName,
                                            IHXBuffer* pParamValue)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    if (!pSession || !pSession->m_bSessionSetup)
    {
        return HXR_OLD_PROXY;
    }
    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;

    if (pURL)
    {
        pMsg->setURL(pURL);
    }
    else
    {
        pMsg->setURL("*");
    }

    if (pSessionID)
    {
        pMsg->addHeader("Session",pSessionID);
    }

    pMsg->addHeader (pParamName,(char*)pParamValue->GetBuffer());
    sendRequest(pMsg, ++m_ulLastSeqNo);

    OnServerRequestEvent(pMsg, NULL, pSession);

    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::SendSetParameterRequest(const char* pSessionID,
                                            const char* pURL,
                                            const char* pParamName,
                                            const char* pParamValue,
                                            const char* pMimeType,
                                            const char* pContent)

{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    if (!pSession || !pSession->m_bSessionSetup)
    {
        return HXR_OLD_PROXY;
    }
    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;

    if (pURL)
    {
        pMsg->setURL(pURL);
    }
    else
    {
        pMsg->setURL("*");
    }

    if (pSessionID)
    {
        pMsg->addHeader("Session",pSessionID);
    }

    pMsg->addHeader(pParamName, pParamValue);
    if (pMimeType && pContent)
    {
        sendRequest(pMsg, pContent, pMimeType, ++m_ulLastSeqNo);
    }
    else
    {
        sendRequest(pMsg, ++m_ulLastSeqNo);
    }

    OnServerRequestEvent(pMsg, NULL, pSession);

    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::SendSetParameterRequest(const char* pSessionID,
                                            const char* pURL,
                                            IHXValues* pNameVal)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    if (!pSession || !pSession->m_bSessionSetup)
    {
        return HXR_OLD_PROXY;
    }
    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;

    if (pURL)
    {
        pMsg->setURL(pURL);
    }
    else
    {
        pMsg->setURL("*");
    }

    if (pSessionID)
    {
        pMsg->addHeader("Session",pSessionID);
    }

    // add all header vals...
    const char* pName;
    IHXBuffer* pVal;
    if (HXR_OK == pNameVal->GetFirstPropertyCString(pName, pVal))
    {
        pMsg->addHeader (pName,(char*)pVal->GetBuffer());
        pVal->Release();

        while (HXR_OK == pNameVal->GetNextPropertyCString(pName, pVal))
        {
            pMsg->addHeader (pName,(char*)pVal->GetBuffer());
            pVal->Release();
        }
    }

    sendRequest(pMsg, ++m_ulLastSeqNo);

    OnServerRequestEvent(pMsg, NULL, pSession);

    return HXR_OK;
}

/**
 * \brief filterRFC822Headers - filter out headers that might conflict with ones we will be setting
 *
 * \param pOldHeaders [in] : list of headers to filter
 * \param pNewHeaders [out] : filtered list of headers
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPServerProtocol::filterRFC822Headers(IHXValues* pOldHeaders,
                                        REF(IHXValues*) pNewHeaders)
{
    HX_RESULT rc = HXR_OK;
    const char* pPropName = NULL;
    IHXKeyValueList* pKeyedHdrs = NULL;
    UINT32 i = 0;

    // These specific fields are restricted in that we don't allow them
    // to be set by just anyone. If they have been set at this point, we
    // will remove them from the headers so that they don't interfere
    // with the instances of these keys that we will add later on.
    static const char* ppRestrictedFields[] =
    {
        "Date",
        "Content-Type",
        "Content-Length",
        "Server",
        "ETag"
    };
    UINT32 ulNumRestrictedFields = 5;

    if (!pOldHeaders)
    {
        return HXR_FAIL;
    }

    // XXX showell - Later we'll just force our callers to give us
    // an IHXKeyValueList, because that's what we really want.
    rc = pOldHeaders->QueryInterface(IID_IHXKeyValueList,
        (void**)&pKeyedHdrs);

    if (rc == HXR_OK)
    {
        // We build a new list for them, and we copy all fields
        // that aren't restricted.  This seems inefficient,
        // but calling CKeyValueList::Remove five times is even
        // less efficient.
        IHXKeyValueList* pNewList = NULL;
        rc = pKeyedHdrs->CreateObject(pNewList);

        if (rc == HXR_OK)
        {
            IHXKeyValueListIter* pListIter = NULL;
            rc = pKeyedHdrs->GetIter(pListIter);

            if (rc == HXR_OK)
            {
                const char* pKey = NULL;
                IHXBuffer* pBuffer = NULL;
                while (pListIter->GetNextPair(pKey, pBuffer) == HXR_OK)
                {
                    BOOL Restricted = FALSE;
                    for (i = 0; i < ulNumRestrictedFields; i++)
                    {
                        if (!stricmp(ppRestrictedFields[i], pKey))
                        {
                            Restricted = TRUE;
                            break;
                        }
                    }
                    if (!Restricted)
                    {
                        pNewList->AddKeyValue(pKey,pBuffer);
                    }
                    HX_RELEASE(pBuffer);
                }
            }
            HX_RELEASE(pListIter);

            // For now, caller wants IHXValues.  I hope to change this soon.
            rc = pNewList->QueryInterface(IID_IHXValues, (void**)&pNewHeaders);
            HX_RELEASE(pNewList);
        }
    }
    else
    {
        IHXBuffer* pBuffer = NULL;

        // Create a new IHXValues
        pNewHeaders = new CHXHeader();
        pNewHeaders->AddRef();

        rc = pOldHeaders->GetFirstPropertyCString(pPropName, pBuffer);
        while (SUCCEEDED(rc))
        {
            BOOL bRestricted = FALSE;

            for (i = 0; i < ulNumRestrictedFields; i++)
            {
                if (!strcasecmp(pPropName, ppRestrictedFields[i]))
                {
                    bRestricted = TRUE;
                }
            }

            if (!bRestricted)
            {
                pNewHeaders->SetPropertyCString(pPropName, pBuffer);
            }
            pBuffer->Release();
            rc = pOldHeaders->GetNextPropertyCString(pPropName, pBuffer);
        }
    }
    HX_RELEASE(pKeyedHdrs);

    return HXR_OK;
}

/**
 * \brief AddRFC822Headers - add a list of headers to an RTSP message
 *
 * \param pMsg [in]: A pointer to the RTSP message that we want to add headers to
 * \param pRFC822Headers [in] : the list of headers (each is a name-value pair)
 *
 * \return void
 */
void
RTSPServerProtocol::AddRFC822Headers(IHXRTSPMessage* pMsg,
                                     IHXValues* pRFC822Headers)
{
    HX_RESULT hxr = HXR_OK;
    IHXMIMEHeader* pHeader = NULL;
    const char* pName = NULL;
    IHXBuffer* pValue = NULL;
    IHXKeyValueList* pKeyedHdrs = NULL;

    if (pRFC822Headers == NULL)
    {
        return;
    }

    // Find out if the IHXValues supports IHXKeyValueList
    // XXX showell - eventually, we should just make all callers
    // give us an IHXKeyValueList, since it's more efficient,
    // and so we don't overwrite duplicate headers.
    hxr = pRFC822Headers->QueryInterface(IID_IHXKeyValueList,
                                         (void**)&pKeyedHdrs);

    if (hxr == HXR_OK)
    {
        IHXKeyValueListIter* pListIter = NULL;
        pKeyedHdrs->GetIter(pListIter);
        HX_ASSERT(pListIter);

        while (pListIter->GetNextPair(pName, pValue) == HXR_OK)
        {
            pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
            pHeader->SetFromString(pName, (const char*)pValue->GetBuffer());
            m_pRespMsg->AddHeader(pHeader);
            HX_RELEASE(pValue);
        }
        HX_RELEASE(pListIter);
    }
    else
    {
        hxr = pRFC822Headers->GetFirstPropertyCString(pName, pValue);
        while (hxr == HXR_OK)
        {
            pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
            pHeader->SetFromString(pName, (const char*)pValue->GetBuffer());
            m_pRespMsg->AddHeader(pHeader);
            pValue->Release();

            hxr = pRFC822Headers->GetNextPropertyCString(pName, pValue);
        }
    }

    HX_RELEASE(pKeyedHdrs);
}

BOOL
RTSPServerProtocol::mimetypeSupportsRARTPChallenge(const char* pMimeType)
{
    int num = sizeof(z_pRARTPChallengeMimeTypes) / sizeof(const char*);
    for (int i = 0; i < num; i++)
    {
        if (!strcasecmp(pMimeType, z_pRARTPChallengeMimeTypes[i]))
        {
            return TRUE;
        }
    }
    return FALSE;
}

SdpFileType
RTSPServerProtocol::GetSdpFileTypeWeNeed(IHXValues* pHeaders)
{
    IHXBuffer* pAgent = NULL;
    SdpFileType sdpType = NONE_SDP;

    /*
     *        Better make sure to come up with a better way to check
     */
    if (FAILED(pHeaders->GetPropertyCString("User-Agent", pAgent)))
    {
        return NONE_SDP;
    }

    if (strstr((const char*)pAgent->GetBuffer(), "RealMedia"))
    {
        sdpType = BACKWARD_COMP_SDP;
    }
    else
    {
        sdpType = INTEROP_SDP;
    }

    HX_RELEASE(pAgent);
    return sdpType;
}

/**
 * \brief _FinishDescribe - parse file and stream headers and form SDP
 *
 * _FinishDescribe looks through file and stream headers for information
 * needed to stream the file and forms the SDP for the response.
 *
 * \param pcSessionID [in] : session id for this RTSP session (required)
 * \param bUseOldSdp [in] : workaround for an SDP handler bug (details anyone?)
 * \param pFileHeader [in] :  File headers for this content
 * \param pHeaders [in] : stream headers for each stream
 * \param pOptionalValues [in]
 * \param mimeType [out] : mime type of the SDP if created
 * \param ppIHXBufferDescription [out] : buffer containing SDP for DESCRIBE response
 *
 * \return void
 */
void
RTSPServerProtocol::_FinishDescribe(const char* pcSessionID,
                                    BOOL bUseOldSdp,
                                    IHXValues* pFileHeader,
                                    CHXSimpleList* pHeaders,
                                    IHXValues* pOptionalValues,
                                    CHXString& mimeType,
                                    IHXBuffer** ppIHXBufferDescription)
{
#ifdef XXXAAK_NO_SDPPLIN
    static const char* desc =
"v=0\r\n"
"o=- 974933260 974933260 IN IP4 205.2.23.172\r\n"
"s=Odelay\r\n"
"i=Beck 1996 Geffen Records\r\n"
"t=0 0\r\n"
"a=SdpplinVersion:1610644653\r\n"
"a=IsRealDataType:integer;1\r\n"
"a=StreamCount:integer;1\r\n"
"a=Title:buffer;\"T2RlbGF5AA==\"\r\n"
"a=Copyright:buffer;\"MTk5NiBHZWZmZW4gUmVjb3JkcwA=\"\r\n"
"a=Author:buffer;\"QmVjawA=\"\r\n"
"a=range:npt=0-3257.280000\r\n"
"m=audio 0 RTP/AVP 101\r\n"
"b=AS:20\r\n"
"a=control:streamid=0\r\n"
"a=range:npt=0-3257.280000\r\n"
"a=length:npt=3257.280000\r\n"
"a=rtpmap:101 x-pn-realaudio\r\n"
"a=mimetype:string;\"audio/x-pn-realaudio\"\r\n"
"a=StartTime:integer;0\r\n"
"a=AvgBitRate:integer;20000\r\n"
"a=PreDataAfterSeek:integer;1\r\n"
"a=EndOneRuleEndAll:integer;1\r\n"
"a=AvgPacketSize:integer;480\r\n"
"a=Predata:integer;960\r\n"
"a=Preroll:integer;384\r\n"
"a=PreDataAtStart:integer;1\r\n"
"a=MaxPacketSize:integer;480\r\n"
"a=MaxBitRate:integer;20000\r\n"
"a=RMFF 1.0 Flags:buffer;\"AAIAAgAA\"\r\n"
"a=OpaqueData:buffer;\"LnJh/QAEAAAucmE0AHxBvAAEAAAAWgADAAAB4AB8QWAAAknwAAJJ8AABAeAAAAAAH0AAAAAQAAIESW50MARkbmV0AAMABk9kZWxheQRCZWNrEzE5OTYgR2VmZmVuIFJlY29yZHMEICAgIA==\"\r\n"
"a=StreamName:string;\"Odelay\"\r\n"
"a=intrinsicDurationType:string;\"intrinsicDurationContinuous\"\r\n"
"a=ASMRuleBook:string;\"priority=5,averagebandwidth=20000,PNMKeyFrameRule=T;priority=5,averagebandwidth=0,PNMNonKeyFrameRule=T;\"\r\n";

    static IHXBuffer* pDesc = 0;
    if (!pDesc)
    {
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                              (void**)&(pDesc));
        (pDesc)->Set((BYTE *)desc, strlen(desc));
    }
#endif

    IHXStreamDescription* pSD = NULL;
    CHXSimpleList::Iterator i;
    UINT16 nStreams = pHeaders->GetCount();

    RTSPServerProtocol::Session* pSession = getSession(pcSessionID);
    pSession->m_uStreamCount = nStreams;
    pSession->clearStreamInfoList();
    pSession->m_ppStreamInfo = new RTSPStreamInfo*[nStreams];
    memset(pSession->m_ppStreamInfo, 0, nStreams*sizeof(RTSPStreamInfo*));

    //XXXVS: Get StreamGroupCount for 3GPP files 
    ULONG32 ulStreamGroupCount = 0;
    if (SUCCEEDED(pFileHeader->GetPropertyULONG32("StreamGroupCount",ulStreamGroupCount)))
    {
	pSession->m_uStreamGroupCount = (UINT16)ulStreamGroupCount;
    }
    DPRINTF(D_INFO, ("RTSPServerProtocol::_FinishDescribe, pSession: %p, "
                        "pSession->m_uStreamGroupCount: %u\n"
                    , pSession, pSession->m_uStreamGroupCount));

    for (i = pSession->m_describeMimeTypeList.Begin();
         i != pSession->m_describeMimeTypeList.End(); ++i)
    {
        mimeType = *(CHXString*)(*i);
#ifndef XXXAAK_NO_SDPPLIN
        pSD = getStreamDescriptionInstance(mimeType);
        if (pSD)
        {
            break;
        }
#endif
    }

#ifndef XXXAAK_NO_SDPPLIN
    if (pSD)
#endif
    {
        UINT32 streamNumber;
        UINT32 unStreamGroupNumber;
        UINT32 needReliable;
        UINT32 rtpPayloadType;
        UINT32 sampleRate;
        UINT32 sampleSize;
        UINT32 RTPFactor;
        UINT32 HXFactor;
        BOOL   bHasMarkerRule;
        UINT16 markerRule;
        UINT32 ulIsLive = 0;
        UINT32 ulExtension;
        IHXBuffer* pControl;
        UINT16 unNumRules;
        BOOL   bHasRTCPRule;
        UINT16 RTCPRule;
        UINT32 ulPayloadWirePacket;
        UINT32 ulDuration = 0;
        UINT32 ulMaxStreamDuration = 0;
        UINT32 ulForceRTP = 0;
        BOOL   bHasTrackID = FALSE;
        UINT32 ulTrackID = 0;

        UINT32 ulRRRate = (UINT32)-1;
        UINT32 ulRSRate = (UINT32)-1;
        UINT32 ulAvgBitRate = 0;

        IHXValues** ppValues = new IHXValues*[nStreams+2];
        ppValues[0] = pFileHeader;

        if (m_pSocket)
        {
            if (!pOptionalValues)
            {
                pOptionalValues = new CHXHeader();
            }

            IHXBuffer* pFam = NULL;
            m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pFam);
            
            IHXSockAddr* pHostAddr = NULL;
            IHXBuffer* pHostAddrBuf = NULL;

            m_pSocket->GetLocalAddr(&pHostAddr);
            HX_ASSERT(pHostAddr);

            pHostAddr->GetAddr(&pHostAddrBuf);
            HX_ASSERT(pHostAddrBuf);

            pOptionalValues->SetPropertyCString("Hostname", pHostAddrBuf);
  	 
            switch (pHostAddr->GetFamily())
            {                    
            case HX_SOCK_FAMILY_IN6:
		{
		    IHXSockAddrIN6* pIN6Addr = NULL;
		    pHostAddr->QueryInterface(IID_IHXSockAddrIN6, (void **)&pIN6Addr);
		    if (pIN6Addr->GetAddrClass() == HX_IN6_CLASS_V4MAPPED)
			pFam->Set((UCHAR*)"IN4", 4);
		    else
			pFam->Set((UCHAR*)"IN6", 4);
		    HX_RELEASE(pIN6Addr);
		}
                break;

            case HX_SOCK_FAMILY_CLOAK: 
                // We can't tell whether this addr is IN6 or IN4 without
                // parsing it.
                if (strrchr((const char*)pHostAddrBuf->GetBuffer(), ':'))
                {
                    pFam->Set((UCHAR*)"IN6", 4);
                }
                else
                {
                    pFam->Set((UCHAR*)"IN4", 4);
                }
                break;


            case HX_SOCK_FAMILY_IN4:
            default:
                pFam->Set((UCHAR*)"IN4", 4);
                break;
            }

            pOptionalValues->SetPropertyCString("SockFamily", pFam);
            HX_RELEASE(pFam);
            HX_RELEASE(pHostAddrBuf);
            HX_RELEASE(pHostAddr);

            // Set the AbsoluteBaseURL to the describe URL
            IHXBuffer* pURLBuf = NULL;
            if (SUCCEEDED(m_pCommonClassFactory->
                        CreateInstance(CLSID_IHXBuffer, (void**)&pURLBuf)) &&
                SUCCEEDED(pURLBuf->Set(
                        (UCHAR*)(const char*)pSession->m_describeURL,
                        (pSession->m_describeURL).GetLength()+1)))
            {
                pOptionalValues->SetPropertyCString("RequestURL", pURLBuf);
            }
            HX_RELEASE(pURLBuf);
        }

        ppValues[1] = pOptionalValues;

        // inactive SETUP support
        HX_ASSERT(!pSession->m_pbSETUPRcvdStrm);
        HX_ASSERT(!pSession->m_punNumASMRules);
        pSession->m_pbSETUPRcvdStrm = new BOOL[nStreams];
        memset(pSession->m_pbSETUPRcvdStrm, 0, sizeof(BOOL) * nStreams);
        pSession->m_punNumASMRules = new UINT16[nStreams];
        memset(pSession->m_punNumASMRules , 0, sizeof(UINT16) * nStreams);

        // set the sdp file type so sdpplin can do the right thing
        ppValues[0]->SetPropertyULONG32("SdpFileType", m_sdpFileType);

        // file header prop
        ppValues[0]->GetPropertyULONG32("LiveStream", ulIsLive);
        pSession->m_bIsLive = ulIsLive ? TRUE : FALSE;

        ppValues[0]->GetPropertyULONG32("Duration", ulDuration);
        pSession->m_ulSessionDuration = ulDuration;

        CHXSimpleList::Iterator i;
        INT16 j=2;
        pSession->m_bSupportsRARTPChallenge = TRUE;
        IHXBuffer* pRulesBuf = 0;
        for (i = pHeaders->Begin(); i != pHeaders->End(); ++i,++j)
        {
            // reset...
            streamNumber        = 0;
            unStreamGroupNumber = 0xFFFF;
            needReliable        = 0;
            rtpPayloadType      = (ULONG32) -1;
            sampleRate          = 0;
            sampleSize          = 0;
            RTPFactor           = 0;
            HXFactor           = 0;
            bHasMarkerRule      = 0;
            markerRule          = 0;
            ulExtension         = 0;
            pControl            = 0;
            unNumRules          = 0;
            bHasRTCPRule        = 0;
            RTCPRule            = 0;
            ulPayloadWirePacket = 0;
            ulForceRTP          = 0;

            ulRRRate = (UINT32)-1;
            ulRSRate = (UINT32)-1;


            ppValues[j] = (IHXValues*)(*i);

            // get/interpret ASM rules if necessary
            IHXBuffer* pRawRules = NULL;
            ppValues[j]->GetPropertyCString("ASMRuleBook", pRawRules);
            if (pRawRules)
            {
                //XXXDC Eliminated use of the ASMRuleBook class for efficiency,
                //though it makes this more ugly and fragile, sorry

                if (!pRulesBuf)
                {
                    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                          (void**)&pRulesBuf);
                }
                pRulesBuf->SetSize(pRawRules->GetSize() + 1);
                char* p = (char*)pRawRules->GetBuffer();
                int i=0;
                char* szTmp = (char*)pRulesBuf->GetBuffer();
                for (i=0; p[i]; ++i)
                {
                    szTmp[i] = tolower(p[i]);
                    // count the number of rules...
                    if (';' == szTmp[i])
                    {
                        unNumRules++;
                    }
                }
                szTmp[i] = 0;

                char* p2 = szTmp;
                p = p2;
                while ((p2 = strstr(p2, "marker")) && (!bHasMarkerRule))
                {
                    if (p2)
                    {
                        p = p2;
                        p2 += 6;
                        while ((*p2) == ' ')
                            ++p2;
                        if (*p2 != '=')
                            break;
                        ++p2;
                        while ((*p2) == ' ')
                            ++p2;
                        if (*p2 == '"')
                            p2++;
                        while ((*p2) == ' ')
                            ++p2;
                        bHasMarkerRule = ((*p2 == '1') || (*p2 == 't'));
                    }
                }
                if (bHasMarkerRule)
                {
                    do
                    {
                        if (*p == ';')
                        {
                            markerRule++;
                        }
                    } while ((--p) >= szTmp);
                }

                p2 = szTmp;
                p = p2;
                while ((p2 = strstr(p2, "rtcprule")) && (!bHasRTCPRule))
                {
                    if (p2)
                    {
                        p = p2;
                        p2 += 8;
                        while ((*p2) == ' ')
                            ++p2;
                        if (*p2 != '=')
                            break;
                        ++p2;
                        while ((*p2) == ' ')
                            ++p2;
                        if (*p2 == '"')
                            p2++;
                        while ((*p2) == ' ')
                            ++p2;
                        bHasRTCPRule = ((*p2 == '1') || (*p2 == 't'));
                    }
                }
                if (bHasRTCPRule)
                {
                    do
                    {
                        if (*p == ';')
                        {
                            RTCPRule++;
                        }
                    } while ((--p) >= szTmp);
                }

                HX_RELEASE(pRawRules);
            }

            // build stream info list
            RTSPStreamInfo* pInfo = new RTSPStreamInfo;
            ppValues[j]->GetPropertyULONG32("StreamNumber", streamNumber);
            if (FAILED(ppValues[j]->GetPropertyULONG32("StreamGroupNumber", 
							unStreamGroupNumber)))
            {
                unStreamGroupNumber = streamNumber;
            };
            ppValues[j]->GetPropertyULONG32("NeedReliablePackets",
                                                needReliable);
            ppValues[j]->GetPropertyULONG32("RTPPayloadType", rtpPayloadType);
            ppValues[j]->GetPropertyULONG32("SamplesPerSecond", sampleRate);
            ppValues[j]->GetPropertyULONG32("BitsPerSample", sampleSize);

            ppValues[j]->GetPropertyULONG32("RTPTimestampConversionFactor",
                                            RTPFactor);
            ppValues[j]->GetPropertyULONG32("HXTimestampConversionFactor",
                                            HXFactor);

            ppValues[j]->GetPropertyULONG32("HXRTPExtensionSupport", ulExtension);
            IHXBuffer* pMimeType = NULL;
            ppValues[j]->GetPropertyCString("MimeType", pMimeType);
            ppValues[j]->GetPropertyCString("Control", pControl);
            ppValues[j]->GetPropertyULONG32("Duration", ulDuration);

            ppValues[j]->GetPropertyULONG32("RtcpRRRate", ulRRRate);
            ppValues[j]->GetPropertyULONG32("RtcpRSRate", ulRSRate);
            ppValues[j]->GetPropertyULONG32("AvgBitRate", ulAvgBitRate);


            if (ppValues[j]->GetPropertyULONG32("ForceRTP", ulForceRTP) != HXR_OK
            ||  ulForceRTP == 0)
            {
                pInfo->m_bForceRTP = FALSE;
            }
            else
            {
                pInfo->m_bForceRTP = TRUE;
            }

            if (SUCCEEDED(ppValues[j]->GetPropertyULONG32("TrackID", ulTrackID)))
            {
                bHasTrackID = TRUE;
            }

            IHXBuffer* pPayloadWirePacket = NULL;
            ppValues[j]->GetPropertyCString("PayloadWirePacket", pPayloadWirePacket);
            if (pPayloadWirePacket)
            {
                if (0==strcasecmp("rtp", (const char *)pPayloadWirePacket->GetBuffer()))
                {
                    ulPayloadWirePacket = 1;
                }
                pPayloadWirePacket->Release();
            }

            if (pMimeType)
            {
                if (!mimetypeSupportsRARTPChallenge(
                            (const char*)pMimeType->GetBuffer()))
                {
                    pSession->m_bSupportsRARTPChallenge = FALSE;
                }
                HX_RELEASE(pMimeType);
            }

            if (pControl)
            {
                pInfo->m_streamControl = pControl->GetBuffer();
                pControl->Release();
            }
            else if (bHasTrackID && (!m_bIsProxy || m_bOriginUsesTrackID))
            {
                char tmp[32];
                sprintf(tmp, "streamid=%lu", ulTrackID);
                pInfo->m_streamControl = tmp;
            }
            else
            {
                char tmp[32];
                sprintf(tmp, "streamid=%u", (UINT16)streamNumber);
                pInfo->m_streamControl = tmp;
            }

            HX_ASSERT(streamNumber < pSession->m_uStreamCount &&
                      pSession->m_ppStreamInfo[streamNumber] == NULL);
            pInfo->m_streamNumber = (UINT16)streamNumber;
            pInfo->m_uStreamGroupNumber = (UINT16)unStreamGroupNumber;
            pInfo->m_bNeedReliablePackets = needReliable? TRUE: FALSE;
            pInfo->m_sPort = 0;
            // RTP stuff
            pInfo->m_rtpPayloadType = (INT16)rtpPayloadType;
            pInfo->m_sampleRate = sampleRate;
            pInfo->m_sampleSize = sampleSize / 8;
            pInfo->m_RTPFactor = RTPFactor;
            pInfo->m_HXFactor = HXFactor;
            pInfo->m_bHasMarkerRule = bHasMarkerRule;
            pInfo->m_markerRule = markerRule;
            pInfo->m_bIsLive = ulIsLive ? TRUE : FALSE;
            pInfo->m_bExtensionSupport = ulExtension;
            pInfo->m_bHasRTCPRule = bHasRTCPRule;
            pInfo->m_RTCPRule = RTCPRule;
            pInfo->m_ulPayloadWirePacket = ulPayloadWirePacket;

            pInfo->m_ulAvgBitRate = ulAvgBitRate;
            pInfo->m_ulRtpRRBitRate = ulRRRate;
            pInfo->m_ulRtpRSBitRate = ulRSRate;

            // Keep a copy of the stream headers so that PLAY
            // requests can send 3gp required headers if available.
            pInfo->m_pStreamHeader = ppValues[j];
            pInfo->m_pStreamHeader->AddRef ();

            if (bHasTrackID && (!m_bIsProxy || m_bOriginUsesTrackID))
            {
                pSession->m_bUseControlID = TRUE;
                pInfo->m_ulControlID = ulTrackID;
            }
            else
            {
                pInfo->m_ulControlID = streamNumber;
            }

            pSession->m_ppStreamInfo[streamNumber] = pInfo;

            pSession->m_punNumASMRules[streamNumber] = unNumRules;

            if(ulDuration > ulMaxStreamDuration)
            {
                ulMaxStreamDuration = ulDuration;
            }
        }

        /*
         * If we don't have the duration in the file header, set session
         * duration to max duration of any component stream.
         */
        if (!pSession->m_ulSessionDuration)
        {
            pSession->m_ulSessionDuration = ulMaxStreamDuration;
        }

#ifndef XXXAAK_NO_SDPPLIN
        if (bUseOldSdp)
        {
            IHXStreamDescriptionSettings* pSDS = NULL;
            pSD->QueryInterface(IID_IHXStreamDescriptionSettings,
                                                                (void**)&pSDS);
            if (pSDS != NULL)
            {
                IHXBuffer* pOpt = NULL;
                CHXBuffer::FromCharArray("true", &pOpt);
                pSDS->SetOption("UseOldEOL", pOpt);
                pOpt->Release();
                pSDS->Release();
            }
        }

        pSession->SetSDPSessionGUID(pSD, ppValues);
        pSD->GetDescription(nStreams+2, ppValues, (*ppIHXBufferDescription));
        HX_RELEASE(pSD);
#else
        *ppIHXBufferDescription = pDesc;
        pDesc->AddRef();
#endif

        HX_RELEASE(pRulesBuf);

        delete[] ppValues;
    }
}

// IHXChallenge method
/**
 * \brief RTSPServerProtocol::SendChallenge - Sends challenge request to clients.
 *
 * With NTLM feature there will be three DESCRIBE request-response pairs between
 * the server and client.
 *
 * 1. First DESCRIBE request from client would be the usual request for which server
 * indicates to client that NTLM authorization is required via WWW-Authenticate= NTLM
 * header in the response.
 *
 * 2. Client prompts the user to enter the credentials and it sends the second DESCRIBE
 * request with "Authorization" header in it. And the server responds with a challenge.
 *
 * 3. Client sends the final DESCRIBE request with challenge response and server sends
 * the SDP after validating the challenge response.
 *
 * This method is responsible for sending the challenge as described in step 2 above.
 *
 * \param  IHXChallengeResponse, IHXRequest [in]
 * \return HX_RESULT
 */

STDMETHODIMP
RTSPServerProtocol::SendChallenge(
    IHXChallengeResponse* pIHXChallengeResponseSender,
    IHXRequest* pIHXRequestChallenge)
{
    IHXValues* pIHXValuesResponseHeaders = NULL;
    HX_RESULT HX_RESULTRet = HXR_FAIL;

    if (!pIHXChallengeResponseSender || !pIHXRequestChallenge)
    {
        return HXR_UNEXPECTED;
    }

    m_spChallengeResponseSender = pIHXChallengeResponseSender;
    m_spRequestChallenge = pIHXRequestChallenge;

    pIHXRequestChallenge->GetResponseHeaders(pIHXValuesResponseHeaders);

    if (pIHXValuesResponseHeaders)
    {
        AddRFC822Headers(m_pRespMsg, pIHXValuesResponseHeaders);
    }

    HX_RELEASE(pIHXValuesResponseHeaders);
    HX_RESULTRet = SendResponse(401);

    return HX_RESULTRet;
}

/**
 * \brief SendStreamResponse - respond to either DESCRIBE or SETUP request
 *
 * SendStreamResponse is called by the protocol response object when it has
 * received the file header and all the stream headers for the content. 
 * The request that triggered us to fetch the file and stream headers may 
 * have been either a DESCRIBE or SETUP, we need to respond accordingly.
 *
 * \param n/a
 *
 * \return :
            - the return code from _SendStreamDescriptionResponse if we are
              processing a DESCRIBE
            - the return code from _SendStreamSetupResponse if the we are
              processing a SETUP
            - HXR_FAIL if the request was something else
 */
STDMETHODIMP
RTSPServerProtocol::SendStreamResponse(HX_RESULT      HX_RESULTStatus,
                                       const char*    pSessionID,
                                       IHXValues*    pIHXValuesFileHeader,
									CHXSimpleList* pCHXSimpleListStreamHeaders,
                                       IHXValues*    pIHXValuesOptional,
                                       IHXValues*    pIHXValuesResponse,
                                       BOOL              bMulticastOK,
                                       BOOL              bRequireMulticast,
                                       BOOL              bIsRealDataType)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    RTSPMethod MethodOriginating;

    pSession->m_bMulticastOK = bMulticastOK;
    pSession->m_bRequireMulticast = bRequireMulticast;
    pSession->m_bIsRealDataType = bIsRealDataType;

    MethodOriginating = GetSessionOriginatingMessage(pSessionID);

    HX_RESULT hxr = HXR_FAIL;
    switch (MethodOriginating)
    {
    case RTSP_DESCRIBE:
        hxr = _SendStreamDescriptionResponse(HX_RESULTStatus, pSessionID,
                                             pIHXValuesFileHeader,
                                             pCHXSimpleListStreamHeaders,
                                             pIHXValuesOptional,
                                             pIHXValuesResponse);
        break;
    case RTSP_SETUP:
        hxr = _SendStreamSetupResponse(HX_RESULTStatus, pSessionID,
                                       pIHXValuesFileHeader,
                                       pCHXSimpleListStreamHeaders,
                                       pIHXValuesOptional,
                                       pIHXValuesResponse);
        break;
    case RTSP_PLAYNOW:
        hxr = _PlaynowSecondStage(HX_RESULTStatus, pSessionID,
                                  pIHXValuesFileHeader,
                                  pCHXSimpleListStreamHeaders,
                                  pIHXValuesOptional,
                                  pIHXValuesResponse);
        break;
    default:
        /* Empty */
        break;
    };

    if (MethodOriginating == RTSP_DESCRIBE &&
        !pSession->m_bRetainEntityForSetup &&
        !pSession->m_bNeedSessionHeader)
    {
        SessionDone(pSessionID);
    }

    return hxr;
}

/**         
 * \brief _SendStreamSetupResponse - send the SETUP response
 *      
 * _SendStreamSetupResponse will send the SETUP response to the client
 * via _FinishSetup() if everything is OK. Otherwise, the error response is
 * controlled by the status parameter passed in.
 * 
 * \param status [in] : Are we ok so far? This drives the response code
 * \param pSessionID [in] : ID for the RTSP session this is associated with
 * \param pFileHeader [in] : file header for this content
 * \param pStreams [in] : stream headers for the content.
 * \param pOptionalValues [in]
 * \param pResponseHeaders [in] : used by authentication plugins for example
 *      
 * \return HXR_OK if the response driven by the "status" argument was sent - 
           even if its an error response. Note that if "Status" was HXR_OK and
           we sent an error response in _FinishSetup because of some failure
           we will return HXR_UNEXPECTED or other error code
 */
STDMETHODIMP
RTSPServerProtocol::_SendStreamSetupResponse(HX_RESULT status,
    const char* pSessionID, IHXValues* pFileHeader, CHXSimpleList* pStreams,
    IHXValues* pOptionalValues, IHXValues* pResponseHeaders)
{
    HX_RESULT rc;

    if (m_bProxyResponseSent)
    {
        m_bProxyResponseSent = FALSE;
        return HXR_OK;
    }

    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    if (status == HXR_OK)
    {
        UINT16 usStreamNumber;
        IHXBuffer* pIHXBufferDescription = NULL;
        CHXString mimeType;

        _FinishDescribe(pSession->m_sessionID, pSession->m_bUseOldSdp,
                        pFileHeader, pStreams, pOptionalValues, mimeType,
                        &pIHXBufferDescription);

        HX_RELEASE(pIHXBufferDescription);

        usStreamNumber =
            GetSessionOriginatingStreamNumber(pSession->m_sessionID);

        rc = _FinishSetup(usStreamNumber, pSession->m_sessionID);
    }
    else
    {
        UINT32 uCode = 500;
        switch (status)
        {
        case HXR_NOT_AUTHORIZED:
            if (m_pRespMsg && pResponseHeaders)
            {
                AddRFC822Headers(m_pRespMsg, pResponseHeaders);
            }
            uCode = 401;
            break;
        case HXR_FORBIDDEN:
            if (m_pRespMsg && pResponseHeaders)
            {
                AddRFC822Headers(m_pRespMsg, pResponseHeaders);
            }
            uCode = 403;
            break;
        case HXR_REDIRECTION:
            uCode = 302;
            break;
        case HXR_NOT_MODIFIED:
            uCode = 304;
            break;
	case HXR_SE_INTERNAL_ERROR:
	    uCode = 500;
	    break;
        default:
            uCode = 404;
            break;
        }

        // Remove session header

        if (m_pRespMsg)
        {
            m_pRespMsg->RemoveHeader("Session");
            SendResponse(uCode);
        }

        SetStatus(pSession->m_ulSessionRegistryNumber,
                pSession->m_ulSessionStatsObjId, uCode);

        // Delete the Session if we are Proxy to avoid keepalives
        // with incorrect Session IDs on localbound connections
        if (m_bIsProxy)
        {
            m_pResp->HandleTeardownRequest(pSessionID);
            m_pSessionManager->removeSessionInstance(pSessionID);
        }

        rc = HXR_OK;
    }

    return rc;
}

/**         
 * \brief _SendStreamDescriptionResponse - send the DESCRIBE response
 *      
 * _SendStreamDescriptionResponse will send the DESCRIBE response to the client.
 * If everything is OK, we will format the SDP from the file and stream headers
 * and send a "200 OK" response. Otherwise, the error response is controlled
 * by the status parameter passed in.
 *          
 * \param status [in] : Are we ok so far? This drives the response code
 * \param pSessionID [in] : ID for the RTSP session this is associated with
 * \param pFileHeader [in] : file header for this content
 * \param pStreams [in] : stream headers for the content.
 * \param pOptionalValues [in]
 * \param pResponseHeaders [in] : used by authentication plugins for example
 *      
 * \return HXR_OK if the response driven by the "status" argument was sent - 
           even if its an error response.
 */
STDMETHODIMP
RTSPServerProtocol::_SendStreamDescriptionResponse(HX_RESULT status,
    const char* pSessionID,
    IHXValues* pFileHeader,
    CHXSimpleList* pStreams,
    IHXValues* pOptionalValues,
    IHXValues* pResponseHeaders)
{
    IHXValues* pValidResponseHeaders = NULL;

    if (m_bProxyResponseSent)
    {
        m_bProxyResponseSent = FALSE;
        return HXR_OK;
    }

    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    // Remove any response headers that might conflict
    // with our own protocol headers
    filterRFC822Headers(pResponseHeaders, pValidResponseHeaders);

    if (status == HXR_OK)
    {
        HX_RESULT rc = HXR_OK;
        IHXBuffer* pBufDesc = NULL;
        CHXString mimeType;

        _FinishDescribe(pSessionID, pSession->m_bUseOldSdp, pFileHeader,
                        pStreams, pOptionalValues, mimeType, &pBufDesc);

        /** If the DESCRIBE request had an "Aggregate-Transport" header
          * we need to select a transport now - and to add the 
          * "Aggregate-Transport" header to the response if successful */
        if (pSession->m_bNeedAggregateTransportHeader)
        {
            rc = pSession->m_pTransportInstantiator->selectTransport(pSession);
            if (SUCCEEDED(rc))
            {
                IHXMIMEHeader* pHeader;

                pHeader = pSession->m_pTransportInstantiator->MakeTransportHeader(TRUE);
                m_pRespMsg->AddHeader(pHeader);
            }
        }

        if (SUCCEEDED(rc))
        {
            pSession->m_bSessionSetup = TRUE;

            /** The SDP is added as an entity body with related headers */
            rc = addEntityHeaders(pSession, mimeType, pBufDesc,
                               pValidResponseHeaders);

            if (SUCCEEDED(rc))
            {
                SendResponse(200);
            }
            else
            {
                SendResponse(500);
            }
        }
        else
        {
            /** We get here if selectTransport fails - "461 Unsupported 
              * Transport" is the most suitable catchall error code
              * although you could make a case for checking the rc and 
              * returning a 500 for a few cases. */
            SendResponse(461);
            SetStatus(pSession->m_ulSessionRegistryNumber,
                    pSession->m_ulSessionStatsObjId, 461);
        }

        HX_RELEASE(pBufDesc);
    }
    else
    {
        UINT32 uCode = 500;
        switch (status)
        {
        case HXR_NOT_AUTHORIZED:
            if (pResponseHeaders != NULL)
            {
                AddRFC822Headers(m_pRespMsg, pValidResponseHeaders);
            }
            uCode = 401;
            break;
        case HXR_FORBIDDEN:
            if (pResponseHeaders != NULL)
            {
                AddRFC822Headers(m_pRespMsg, pValidResponseHeaders);
            }
            uCode = 403;
            break;
        case HXR_REDIRECTION:
            uCode = 302;
            break;
        case HXR_NOT_MODIFIED:
            uCode = 304;
            break;
	case HXR_SE_INTERNAL_ERROR:
	    uCode = 500;
	    break;
        default:
            uCode = 404;
            break;
        }
        SendResponse(uCode);
        SetStatus(pSession->m_ulSessionRegistryNumber,
                pSession->m_ulSessionStatsObjId, uCode);
    }

    pSession->m_bNeedAggregateTransportHeader = FALSE;

    // fix for bug 166738: to log unsuccessful requests from 3rd party players in access log,
    // we keep the session instance in the case of failure so that their stats may be logged
    // in the access log. It will eventually be destroyed on clients disconnecting or being
    // timed out.

    if ((!pSession->m_bRetainEntityForSetup)&&(status == HXR_OK))
    {
        m_pResp->HandleRetainEntityForSetup(pSessionID, FALSE);
        m_pSessionManager->removeSessionInstance(pSessionID);
    }

    HX_RELEASE(pValidResponseHeaders);

    /** We are responsible for handling errors and driving the correct 
      * response to the client, the return code here is ignored. */
    return HXR_OK;
}

/**         
 * \brief _PlaynowSecondStage - send the PLAYNOW response
 *      
 * We have gotten the file/stream headers and _PlaynowSecondStage 
 * continues processing of the PLAYNOW request. It will call _FinishPlaynow
 * which selects and creates the transport and formulates the correct
 * response
 * 
 * \param status [in] : Are we ok so far? This drives the response code
 * \param pSessionID [in] : ID for the RTSP session this is associated with
 * \param pFileHeader [in] : file header for this content
 * \param pStreams [in] : stream headers for the content.
 * \param pOptionalValues [in]
 * \param pResponseHeaders [in] : used by authentication plugins for example
 *      
 * \return HXR_OK if the response driven by the "status" argument was sent -
 *         even if its an error response. Note that if "Status" was HXR_OK
 *         and we sent an error response in _FinishSetup because of some 
 *         failure we will return HXR_UNEXPECTED or other error code
 */
STDMETHODIMP
RTSPServerProtocol::_PlaynowSecondStage(HX_RESULT status,
    const char* pSessionID, IHXValues* pFileHeader, CHXSimpleList* pStreams,
    IHXValues* pOptionalValues, IHXValues* pResponseHeaders)
{
    HX_RESULT rc;
    IHXValues* pValidResponseHeaders = NULL;

    /// If we sent a "Use Proxy" redirect we just return here
    if (m_bProxyResponseSent)
    {
        m_bProxyResponseSent = FALSE;
        return HXR_OK;
    }

    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    // Remove any response headers that might conflict
    // with our own protocol headers
    filterRFC822Headers(pResponseHeaders, pValidResponseHeaders);

    if (status == HXR_OK)
    {
        IHXBuffer* pBufDesc = NULL;
        CHXString mimeType;

        _FinishDescribe(pSession->m_sessionID, pSession->m_bUseOldSdp,
                        pFileHeader, pStreams, pOptionalValues, mimeType,
                        &pBufDesc);

       /** The SDP is added as an entity body with related headers */
       rc = addEntityHeaders(pSession, mimeType, pBufDesc, pValidResponseHeaders);

       HX_RELEASE(pBufDesc);
       HX_RELEASE(pValidResponseHeaders);

       if (SUCCEEDED(rc))
       {
            HX_ASSERT(pSession->m_pAggregateTransportParams);

            /** selectTransport() drives the RTSPTransportInstantiator to 
              * choose the most appropriate set of transport parameters
              * for the new request. It will re-use the existing aggregate
              * transport if possible ... or else choose between the fields
              * that we parsed from the "Aggregate-Transport" header in the
              * request ... in which case we need to send a "202 Accepted"
              * response to the client. */
            if (!SUCCEEDED(pSession->m_pTransportInstantiator->selectTransport(pSession)))
            {
                FailAndTeardown(461, pSession);

                return HXR_OK;
            }
            else
            {
                SPRTSPTransportParams spTransParams;
                pSession->m_pTransportInstantiator->GetTransportParams(
                                              spTransParams.AsInOutParam());

                if (!spTransParams.IsValid())
                {
                    HX_ASSERT(FALSE);
                    FailAndTeardown(461, pSession);

                    return HXR_FAIL;
                }

                /** Here is where we distinguish between a new transport 
                  * and a recycled one! Note, we are comparing the pointers
                  * to see if the existing aggregate was selected as the
                  * active transport! This is not the "==" operator! */
                if (pSession->m_pAggregateTransportParams != spTransParams.Ptr())
                {
                    IHXMIMEHeader* pHeader;
                    pHeader = pSession->m_pTransportInstantiator->MakeTransportHeader(TRUE);
                    m_pRespMsg->AddHeader(pHeader);
    
                    SendResponse(202);
                }
                else
                {
                    /** Continue processing, the aggregate is now in use.
                      * we need to save the Stream Description and its mime
                      * type  cuz the response can go out asynchronously */
                    pSession->m_describeMimeType = mimeType;

                    /** _FinishPlaynow drives the rest of the PLAYNOW 
                      * handling, we just ignore the return value */
                    _FinishPlaynow(pSession);
                }
            }

            HX_RELEASE(pSession->m_pAggregateTransportParams);

            return HXR_OK;
        }
        else
        {
            /// Send 404 (without session header) and tear session down
            FailAndTeardown(404, pSession);
            return HXR_OK;
        }
    }
    else
    {
        UINT32 uCode = 500;
        switch (status)
        {
        case HXR_NOT_AUTHORIZED:
            if (m_pRespMsg && pValidResponseHeaders)
            {
                AddRFC822Headers(m_pRespMsg, pValidResponseHeaders);
            }
            uCode = 401;
            break;
        case HXR_FORBIDDEN:
            if (m_pRespMsg && pValidResponseHeaders)
            {
                AddRFC822Headers(m_pRespMsg, pValidResponseHeaders);
            }
            uCode = 403;
            break;
        case HXR_REDIRECTION:
            uCode = 302;
            break;
        case HXR_NOT_MODIFIED:
            uCode = 304;
            break;
	case HXR_SE_INTERNAL_ERROR:
	    uCode = 500;
	    break;
        default:
            uCode = 404;
            break;
        }

        /// \todo can we really get here with null resp msg???
        if (m_pRespMsg)
        {
            // Send response (without session header) and tear session down
            FailAndTeardown(uCode, pSession);
        }
    }

    HX_RELEASE(pValidResponseHeaders);

    return HXR_OK;
}

/**
 * \brief SendStreamRecordDescriptionResponse - not implemented
 *
 * The server core no longer handles RECORD requests so this method is not
 * implemented.
 *
 * \param n/a
 *
 * \return HXR_NOTIMPL
 */
STDMETHODIMP
RTSPServerProtocol::SendStreamRecordDescriptionResponse(
    HX_RESULT status,
    const char* pSessionID,
    IHXValues* pAuthValues,
    IHXValues* pResponseHeaders)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}


/**
 * \brief GetSalt - handle the salt header sent by Helix proxies
 *
 * Helix proxies send a header with a salt used to encrypt data sent
 * to downstream proxies. This method processes that header and adds
 * it to the registry for the media export module (MEI).
 * 
 * \todo why are there two flavors of GetSalt and can we clean it up
 *
 * \param n/a
 *
 * \return HXR_OK unless something is very wrong
 */
HX_RESULT
RTSPServerProtocol::GetSalt(RTSPSetupMessage* pMsg,
                            IHXValues* pRequestHeaders,
                            UINT32 ulSessionRegistryNumber,
                            UINT32 ulSessionStatsObjId)
{
    // The X-Real-Salt header will be sent by MII to

    IHXClientStats* pClientStats = NULL;
    IHXSessionStats* pSessionStats = NULL;

    pClientStats = m_pClientStatsMgr->GetClient(m_ulClientStatsObjId);

    if (!pClientStats)
    {
        return HXR_UNEXPECTED;
    }

    pSessionStats = pClientStats->GetSession(ulSessionStatsObjId);

    MIMEHeader* pSaltHdr = pMsg->getHeader("x-real-salt");

    if(pSaltHdr && pRequestHeaders)
    {
        MIMEHeaderValue* pValue = pSaltHdr->getFirstHeaderValue();
        if (pValue)
        {
            // Retrieve the salt for mei
            MIMEParameter* pMP = pValue->getFirstParameter();
            if(pMP)
            {
                const char* pSalt = pMP->m_attribute.GetBuffer(0);

                IHXBuffer* pBufSalt = NULL;
                m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                      (void**)&pBufSalt);
                pBufSalt->Set((const UCHAR*)pSalt, strlen(pSalt)+1);

                if (m_pClientStatsMgr->UseRegistryForStats())
                {
                    const char form[] = "client.%lu.session.%lu.salt";
                    char pRegkey[sizeof(form) + 20];
                    sprintf(pRegkey, form, m_ulRegistryConnId, ulSessionRegistryNumber);
                    m_pRegistry->AddStr(pRegkey, pBufSalt);
                }

                pSessionStats->SetSalt(pBufSalt);
                HX_RELEASE(pBufSalt);
            }
        }
    }

    // Get auth info, if present

    MIMEHeader* pAuthHdr = pMsg->getHeader("x-real-auth");

    if(pAuthHdr && pRequestHeaders)
    {
        MIMEHeaderValue* pValue = pAuthHdr->getFirstHeaderValue();
        if (pValue)
        {
            // Retrieve the auth for mei
            MIMEParameter* pMP = pValue->getFirstParameter();
            if(pMP)
            {
                const char* pAuth = pMP->m_attribute.GetBuffer(0);

                IHXBuffer* pBufAuth = NULL;
                m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                      (void**)&pBufAuth);
                pBufAuth->Set((const UCHAR*)pAuth, strlen(pAuth)+1);

                if (m_pClientStatsMgr->UseRegistryForStats())
                {
                    const char form[] = "client.%lu.session.%lu.auth";
                    char pRegkey[sizeof(form) + 20];
                    sprintf(pRegkey, form, m_ulRegistryConnId, ulSessionRegistryNumber);
                    m_pRegistry->AddStr(pRegkey, pBufAuth);
                }

                pSessionStats->SetAuth(pBufAuth);
                HX_RELEASE(pBufAuth);
            }
        }
    }

    HX_RELEASE(pClientStats);
    HX_RELEASE(pSessionStats);

    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::GetSalt(IHXRTSPRequestMessage* pReqMsg,
                            IHXValues* pRequestHeaders,
                            UINT32 ulSessionRegistryNumber,
                            UINT32 ulSessionStatsObjId)
{
    // The X-Real-Salt header will be sent by MII to

    IHXRTSPMessage* pMsg           = NULL;
    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    IHXMIMEHeader* pSaltHdr        = NULL;
    IHXBuffer* pBufVal             = NULL;
    IHXMIMEHeader* pAuthHdr        = NULL;

    IHXClientStats* pClientStats   = NULL;
    IHXSessionStats* pSessionStats = NULL;

    pClientStats = m_pClientStatsMgr->GetClient(m_ulClientStatsObjId);

    if (!pClientStats)
    {
        return HXR_UNEXPECTED;
    }

    pSessionStats = pClientStats->GetSession(ulSessionStatsObjId);


    if (HXR_OK == pMsg->GetHeader("x-real-salt", pSaltHdr) && pRequestHeaders)
    {
        pSaltHdr->GetValueAsBuffer(pBufVal);
        if(pBufVal)
        {

            if (m_pClientStatsMgr->UseRegistryForStats())
            {
                const char form[] = "client.%lu.session.%lu.salt";
                char pRegkey[sizeof(form) + 20];
                sprintf(pRegkey, form, m_ulRegistryConnId, ulSessionRegistryNumber);
                m_pRegistry->AddStr(pRegkey, pBufVal);
            }

            pSessionStats->SetSalt(pBufVal);
            HX_RELEASE(pBufVal);
        }
    }


    if (HXR_OK == pMsg->GetHeader("x-real-auth", pAuthHdr) && pRequestHeaders)
    {
        pAuthHdr->GetValueAsBuffer(pBufVal);
        if(pBufVal)
        {
            if (m_pClientStatsMgr->UseRegistryForStats())
            {
                const char form[] = "client.%lu.session.%lu.auth";
                char pRegkey[sizeof(form) + 20];
                sprintf(pRegkey, form, m_ulRegistryConnId, ulSessionRegistryNumber);
                m_pRegistry->AddStr(pRegkey, pBufVal);
            }

            pSessionStats->SetAuth(pBufVal);
            HX_RELEASE(pBufVal);
        }
    }

    HX_RELEASE(pClientStats);
    HX_RELEASE(pSessionStats);

    HX_RELEASE(pSaltHdr);
    HX_RELEASE(pAuthHdr);
    HX_RELEASE(pMsg);

    return HXR_OK;
}


HX_RESULT
RTSPServerProtocol::AddRDTFeatureLevelHeader(UINT32 ulRDTFeatureLevel)
{
    // FeatureLevel 2+ implies server has StreamDone fix on seek (b#42118)
    IHXMIMEHeader* pHeader = NULL;
    char szTmpBuf[12];
    snprintf(szTmpBuf, 12, "%u", (unsigned int)(ulRDTFeatureLevel));
    pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHeader->SetFromString("RDTFeatureLevel", szTmpBuf);
    m_pRespMsg->AddHeader(pHeader);

    return HXR_OK;
}

/**
 * \brief HandleAltServerProxy - append the "Alternate-Server" and 
 * "Reconnect" headers to the response if needed
 *
 * \param n/a
 *
 * \return HXR_OK
 */

HX_RESULT
RTSPServerProtocol::HandleAltServerProxy(RTSPServerProtocol::Session* pSession)
{
    HX_ASSERT(!pSession->m_pAltMgr);
    IHXMIMEHeader* pHeader = NULL;

    IHXAlternateServerProxy* pAltMgr;

    char* pURL = NULL;
    HX_RESULT theErr =
        m_pContext->QueryInterface(IID_IHXAlternateServerProxy,
                                   (void**)&pAltMgr);
    if (SUCCEEDED(theErr))
    {
        // get the correct URL to pass in
        UINT32 ulLen = pSession->m_describeURL.GetLength();
        pURL = (char*)(const char*)pSession->m_describeURL;

        // rtsp://
        if (ulLen <= 7 || strncasecmp(pURL, "rtsp://", 7) != 0)
        {
            // nothing we can do...
            HX_RELEASE(pAltMgr);
            theErr = HXR_FAIL;
        }
    }

    if (SUCCEEDED(theErr))
    {
        HX_ASSERT(pAltMgr && pURL);
        // save it
        pSession->m_pAltMgr = pAltMgr;
        pAltMgr->Init(pSession);

        pURL += 7; //XXTDM: this looks dangerous

        if (!pAltMgr->IsEnabled(pURL))
        {
            HX_ASSERT(!pSession->m_bAltEnabled);
                HX_ASSERT(!pSession->m_pServAlt && !pSession->m_pProxAlt);

            pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
            pHeader->SetFromString("Reconnect", "false");
            m_pRespMsg->AddHeader(pHeader);
        }
        else
        {
            pSession->m_bAltEnabled = TRUE;

            pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
            pHeader->SetFromString("Reconnect", "true");
            m_pRespMsg->AddHeader(pHeader);

            IHXBuffer* pAltServ = NULL;

            theErr = pAltMgr->GetAltServers(pURL, pAltServ);
            if (SUCCEEDED(theErr))
            {
                // at least one is there
                if (pAltServ)
                {
                    pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                    pHeader->SetFromString("Alternate-Server",
                                       (const char*)pAltServ->GetBuffer());
                    m_pRespMsg->AddHeader(pHeader);
                    pSession->m_pServAlt = pAltServ;  // take AddRef();
                }
            }
        }
    }

    return theErr;
}



/**
 * \brief _FinishSetup - finish processing related to the SETUP request and send response
 *
 * _FinishSetup is called to complete processing of the SETUP request, including
 * populating the Transport header, and send the response to the client. If all
 * streams are setup, FinishAllSetups() is invoked and this will call the 
 * protocol response objects SetupTransports() method, which will trigger the
 * media control object to build the packet flow.
 *
 * See RTSPProtocol::SetupTransports and Player::Session::setup_transport()
 *
 * \param usStreamNumber [in] : stream number that is being SETUP 
 * \param pcSessionID    [in] : session ID of the RTSP session (must be a valid session)
 *
 * \return HXR_OK if the SETUP is processed
 */
HX_RESULT
RTSPServerProtocol::_FinishSetup(UINT16 usStreamNumber,
                                 const char* pcSessionID)
{
    HX_RESULT rc = HXR_OK;
    RTSPStreamInfo* pStreamInfo = NULL;

    if (m_bProxyResponseSent)
    {
        m_bProxyResponseSent = FALSE;

        return HXR_OK;
    }

    RTSPServerProtocol::Session* pSession = getSession(pcSessionID);
    HX_ASSERT(pSession != NULL);

    // get streamInfo struct for this streamNumber
    if (pSession->m_ppStreamInfo != NULL &&
        usStreamNumber < pSession->m_uStreamCount)
    {
        pStreamInfo = pSession->m_ppStreamInfo[usStreamNumber];
    }
    if (!pStreamInfo)
    {
        HX_ASSERT(FALSE);
        FailAndTeardown(500, pSession);
        return HXR_UNEXPECTED;
    }

    pSession->m_sSetupCount++;
    pSession->m_uTotalSetupReceived++;
    if (pSession->m_sSetupCount == 1)
    {
	pSession->m_uFirstSetupStreamNum = usStreamNumber;
    }

    // We have received SETUP request for this stream.
    if (pSession->m_pbSETUPRcvdStrm)
    {
        if (pSession->m_pbSETUPRcvdStrm[usStreamNumber])
        {
            // Duplicate setup, return 455 (RFC2326, s10.4)
            FailAndTeardown(455, pSession);
            return HXR_UNEXPECTED;
        }
        pSession->m_pbSETUPRcvdStrm[usStreamNumber] = TRUE;
    }

    if (pSession->m_pStreamUrl != NULL)
    {
        pStreamInfo->m_streamControl = pSession->m_pStreamUrl;
        HX_VECTOR_DELETE(pSession->m_pStreamUrl);
    }
    else
    {
        HX_ASSERT(FALSE);

        /** The streamurl should have been filled in when we started
          * processing this SETUP request. This code should never be
          * executed, but since the error case handling already exists
          * I am updating it to try to use the relative URL from the SDP */
        if (pStreamInfo->m_streamControl.GetLength())
        {
            pStreamInfo->m_streamControl = pSession->m_describeURL + "/" +
                         pStreamInfo->m_streamControl;
        }
        else
        {
            char tmp[32];
            sprintf(tmp, "/streamid=%hu", usStreamNumber);
            pStreamInfo->m_streamControl = pSession->m_describeURL + tmp;
        }
    }

    if (!pSession->m_pTransportInstantiator->IsAggregateTransport())
    {
        if (!SUCCEEDED(pSession->m_pTransportInstantiator->selectTransport(pSession, usStreamNumber)))
        {
            if (pSession->m_sSetupCount == 1 && pSession->m_bRequireMulticast)
            {
                /*
                 * XXXSMP We should send a failed setup response
                 * but if we do the player displays it instead of
                 * the alert :(
                 */
                SendAlertRequest(pcSessionID, SE_MULTICAST_DELIVERY_ONLY,
                                 alert_table[SE_MULTICAST_DELIVERY_ONLY]);
            }
            else
            {
                FailAndTeardown(461, pSession);
            }

            return HXR_FAIL;
        }
    }

    rc = _FinishCommon (pSession, pStreamInfo, usStreamNumber, FINISH_SETUP);

    if (rc != HXR_OK)
    {
        switch (rc)
        {
        case HXR_NOT_AUTHORIZED:
            FailAndTeardown(401, pSession);
            break;
        case HXR_BAD_TRANSPORT:
            FailAndTeardown(461, pSession);
            break;
        default:
            // any other error
            FailAndTeardown(500, pSession);
            break;
        }

        return rc;
    }

    BOOL bIsFirstSetupForSource = (pSession->m_sSetupCount == 1);

    if (bIsFirstSetupForSource)
    {
        HandleAltServerProxy(pSession);
    }

    // Add the RDTFeatureLevel header to the Response
    //XXXTDM: should probably not be sent with non-RDT transports
    AddRDTFeatureLevelHeader(pSession->m_ulRDTFeatureLevel);

    // Add the StatsMask header in the Setup Response
    AddStatsHeaders();

    /*
     * If we've initiated a challenge, add challenge response to the SETUP
     * RESPONSE message.  Otherwise, initiate challenge now.
     */
#if defined(HELIX_FEATURE_RTSP_SERVER_CHALLENGE) || defined(HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE)
    SetChallengeHeader(pSession, RTSP_VERB_SETUP, FALSE, m_pRespMsg);
#endif

    /*
     * XXXMC
     * Special-case handling for PV clients behind a NAT/firewall.
     */
    if (pSession->m_bEmulatePVSession && m_pPVServerId)
    {
        IHXMIMEHeader* pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pHeader->SetFromString("Server",
                                  (const char*)m_pPVServerId->GetBuffer());
        m_pRespMsg->AddHeader(pHeader);
        DPRINTF(D_INFO, ("Stream setup with PV Emulation enabled\n"));
    }

    IHXRTSPMessage* pMsg = NULL;
    if (SUCCEEDED(pSession->m_pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg)))
    {

        //Handle 3GPP-Adaptation, HelixAdaptation headers
        // ignore failures
        HandleStreamAdaptationHeader(pcSessionID, pMsg, TRUE);

        //Handle 3GPP-Link-Char header
        // ignore failures
        Handle3GPPLinkCharHeader(pcSessionID, pMsg, TRUE);

        // Handle Bandwidth header -- ignore failures
        HandleBandwidthHeader(pcSessionID, pMsg);

        HX_RELEASE(pMsg);
    }

    SendResponse(200);
    SetStatus(pSession->m_ulSessionRegistryNumber, pSession->m_ulSessionStatsObjId, 200);

    rc = m_pResp->HandleStreamSetup( pSession->m_sessionID, usStreamNumber, 
		    pSession->m_ppStreamInfo[usStreamNumber]->m_uStreamGroupNumber);
    // XXXGo - why only the last one?
    if(pSession->m_sSetupCount == pSession->m_uStreamCount && 
        !pSession->m_bSetupsComplete)
    {
        rc = FinishAllSetups(pSession);
    }

    return rc;
}

/**
 * \brief FinishAllSetups - mark setups complete and trigger packet flow setup
 *
 * FinishAllSetups() sets a member variable to indicate that all streams
 * have been setup and then calls the protocol response objects 
 * SetupTransports() method, which will trigger the player object to build 
 * the packet flow.
 *
 * See RTSPProtocol::SetupTransports and Player::Session::setup_transport()
 *
 * \param pSession [in] : the RTSP session that has been SETUP
 *
 * \return HXR_OK
 */
HX_RESULT 
RTSPServerProtocol::FinishAllSetups(RTSPServerProtocol::Session* pSession)
{
    HX_ASSERT(m_pResp);

    if (pSession->m_sSetupCount > 0)
    {
        m_pResp->SetupTransports(pSession->m_sessionID);
        pSession->m_sSetupCount = 0;
        pSession->m_bSetupsComplete = TRUE;
        return m_pResp->HandleSetupRequest(HXR_OK);
    }
    
    return HXR_OK;
}


/**
 * \brief _FinishCommon - Code common to both _FinishSetup && _FinishPlaynow.
 *
 * _FinishCommon is called by _FinishSetup and _FinishPlaynow to handle common
 * tasks, specifically transport creation, and Transport header creation.
 *
 * \see RTSPServerProtocol::_FinishSetup
 * \see RTSPServerProtocol::_FinishPlaynow
 *
 * \param pSession    [in] : RTSPServerProtocol::Session* to create transports for
 * \param pStreamInfo [in] : RTSPStreamInfo* that is being SETUP 
 * \param usStream    [in] : stream number that is being SETUP 
 * \param usType      [in] : Type of call (FINISH_SETUP || FINISH_PLAYNOW)
 *
 * \return HXR_OK if the SETUP is processed
 * \return HXR_BAD_TRANSPORT if the RTSPTransportParams can't be found
 * \return HXR_UNEXPECTED if the TransportType is unknown
 * \return HXR_<XXX> propagated from the various SetupTransportZZZ
 */
HX_RESULT
RTSPServerProtocol::_FinishCommon(RTSPServerProtocol::Session* pSession, RTSPStreamInfo* pStreamInfo, UINT16 usStream, UINT16 usType)
{
    HX_RESULT rc = HXR_OK;

    RTSPTransportInstantiator* pInstantiator = pSession->m_pTransportInstantiator;
    HX_ASSERT(pInstantiator);

    SPRTSPTransportParams spTransParams;
    pInstantiator->GetTransportParams(spTransParams.AsInOutParam(), usStream);
    if (!spTransParams.IsValid())
    {
        return HXR_BAD_TRANSPORT;
    }

    // Set RARTP flag if finishing SETUP
    //XXXTDM: need to rework the challenge/response logic

    if (usType == FINISH_SETUP)
    {
        if (IS_CLIENT_TRANSPORT(spTransParams->m_lTransportType) &&
                !IS_RDT_TRANSPORT(spTransParams->m_lTransportType))
        {
            if (m_bRARTPChallengeMet || m_ulChallengeInitMethod == RTSP_VERB_NONE)
            {
                if (pSession->m_bSupportsRARTPChallenge)
                {
                    m_bRARTPChallengePending = !m_bRARTPChallengeMet;
                }
            }
        }
    }

    // If you change this switch statement, you MUST also check the
    // IS_SUPPORTED_TRANSPORT macro.  Failure to do so may result in
    // "internal server error" and/or this code not being reached.
    switch(spTransParams->m_lTransportType)
    {
    case RTSP_TR_TNG_UDP:
    case RTSP_TR_RDT_UDP:
    case RTSP_TR_TNG_MCAST:
    case RTSP_TR_RDT_MCAST:
        rc = pInstantiator->SetupTransportTNGUdpRDTUdpMcast(pSession, pStreamInfo,
                                       spTransParams.Ptr(), usStream);

        /** \todo Undocumented : why return here and not send a response?!?!
         * Why clear the transport? Test and document this code path. */
        //XXX:TDK Check with JC
        if (HXR_OK != rc && usType == FINISH_SETUP)
	{
            HX_RELEASE(pSession->m_pTransportInstantiator);
            return rc;
	}
        break;
    case RTSP_TR_TNG_TCP:
    case RTSP_TR_RDT_TCP:
        rc = pInstantiator->SetupTransportTNGTcpRDTTcp(pSession, pStreamInfo,
                usStream);
        break;

    case RTSP_TR_RTP_UDP:
        rc = pInstantiator->SetupTransportRTPUdp(pSession, pStreamInfo,
                spTransParams.Ptr(), usStream);

        if (SUCCEEDED(rc) && usType == FINISH_PLAYNOW)
        {
            pSession->m_unStreamsRTPInfoReady = 0;
            pSession->m_bRTPInfoResponsePending = TRUE;
        }

        break;

    case RTSP_TR_RTP_TCP:
        rc = pInstantiator->SetupTransportRTPTcp(pSession, pStreamInfo,
                usStream);

        if (SUCCEEDED(rc) && usType == FINISH_PLAYNOW)
        {
            pSession->m_unStreamsRTPInfoReady = 0;
            pSession->m_bRTPInfoResponsePending = TRUE;
        }

        break;

    case RTSP_TR_NULLSET:
        rc = pInstantiator->SetupTransportNULLSet(pSession, pStreamInfo,
                usStream);
        break;

    case RTSP_TR_BCNG_UDP:
    case RTSP_TR_BCNG_TCP:
    case RTSP_TR_BCNG_MCAST:
        if (usType == FINISH_SETUP)
        {
            rc = pInstantiator->SetupTransportBCNGUdpTcpMcast(pSession, pStreamInfo, 
                    spTransParams.Ptr(), usStream);

            if (SUCCEEDED(rc))
            {
                BCNGTransport* pTransport = NULL;

                pTransport = (BCNGTransport*)pSession->getFirstTransportSetup();

                rc = HXR_FAIL;

                if (pTransport != NULL)
                {
                    rc = pInstantiator->SetBCNGParameters(usStream,
                            pTransport->m_ulSessionID,
                            pTransport->m_ulStartTime,
                            pTransport->m_pAddr->GetPort(),
                            pTransport->m_ucTCPInterleave);
                }
            }

            break;
        }
        // else FALL-THRU

    default:
        HX_ASSERT(FALSE);
        rc = HXR_UNEXPECTED;
        break;
    }

    // XXX:TDK This cheesily assumes only FINISH_SETUP || FINISH_PLAYNOW.
    IHXMIMEHeader* pHeader = NULL;
    pHeader = (usType == FINISH_PLAYNOW) ? pInstantiator->MakeTransportHeader(TRUE)
                                         : pInstantiator->MakeTransportHeader(FALSE, usStream);
    m_pRespMsg->AddHeader(pHeader);

    return rc;
}

/**
 * \brief _FinishPlaynow - finish processing related to the PLAYNOW request and send response
 *
 * _FinishPlaynow is called to complete processing of the PLAYNOW request, including
 * populating the Aggregate-Transport header, and send the response to the client.
 *
 * \param usStreamNumber [in] : stream number that is being SETUP 
 * \param pcSessionID    [in] : session ID of the RTSP session (must be a valid session)
 *
 * \return HXR_OK if we complete without error, otherwise we send an RTSP
 *         response with an error status to the client and return an error
 *         to the caller.
 *
 * \todo : the code in _FinishSetup() is very similar to the code here. 
 * The main differences are that _FinishSetup() returns a response to the 
 * SETUP request that it is processing but the PLAYNOW response can't be
 * generated here. Plus, _FinishPlaynow loops through all the streams and
 * "sets up" the transport for each in turn. Still, the similarities are
 * great enough that we can get rid of some duplicate code by extracting
 * some of this functionality to a new method(s).
 *
 */
HX_RESULT
RTSPServerProtocol::_FinishPlaynow(RTSPServerProtocol::Session* pSession)
{
    HX_RESULT rc = HXR_OK;
    RTSPStreamInfo* pStreamInfo = NULL;
    UINT16 usStream;
    HX_ASSERT(pSession != NULL);

#if 0
    printf("   _FinishPlaynow: session %p ctrl id %d  stream count %d\n", 
               pSession, pSession->m_ulControlID, pSession->m_uStreamCount);
#endif

    pSession->m_uTotalSetupReceived = pSession->m_uStreamCount;

    /// if there's an RTP transport we will change these later (see switch 
    /// statement below), but if it's RDT we want them out of the way
    pSession->m_unStreamsRTPInfoReady = pSession->m_uTotalSetupReceived;
    pSession->m_bRTPInfoResponsePending = FALSE;

    /** PLAYNOW sets all count-1 streams up in order in a loop, this code
      * does assume that the "StreamNumber" property in the SDP (if present)
      * increases from 0 to count-1 */
    if (pSession->m_uStreamCount)
    {
	pSession->m_uFirstSetupStreamNum = 0;
    }

    for (usStream = 0; (usStream < pSession->m_uStreamCount); usStream++)
    {
        /** Playnow supercedes getting all the individual setups for this
          * session. If failure, we tear the session down! */
        pSession->m_sSetupCount++;

        /// Should obsolete m_pbSETUPRcvdStrm[] if possible
        if (pSession->m_pbSETUPRcvdStrm)
        {
            pSession->m_pbSETUPRcvdStrm[usStream] = TRUE;
        }

        // get streamInfo struct for this streamNumber
        if (pSession->m_ppStreamInfo != NULL &&
            usStream < pSession->m_uStreamCount)
        {
            pStreamInfo = pSession->m_ppStreamInfo[usStream];
        }
        if (!pStreamInfo)
        {
            HX_ASSERT(FALSE);
            FailAndTeardown(500, pSession);
            return HXR_UNEXPECTED;
        }

        /** The expected situation is that pSession->m_pStreamUrl is NULL
          * (its currently filled at SETUP time and we should not have 
          * received a SETUP request since this is a PLAYNOW), that
          * pStreamInfo->m_streamControl was set from the SDP and that
          * the base url was loaded into pSession->m_describeURL when the
          * PLAYNOW request was first received.
          *
          * The other cases are handled here as a safety.
          */
        if (pSession->m_pStreamUrl != NULL)
        {
            pStreamInfo->m_streamControl = pSession->m_pStreamUrl;
            HX_VECTOR_DELETE(pSession->m_pStreamUrl);
        }
        else
        {
            if (pStreamInfo->m_streamControl.GetLength())
            {
                pStreamInfo->m_streamControl = pSession->m_describeURL + "/" +
                                    pStreamInfo->m_streamControl;
            }
            else
            {
                HX_ASSERT(FALSE);
                char tmp[32];
                sprintf(tmp, "/streamid=%hu", usStream);
                pStreamInfo->m_streamControl = pSession->m_describeURL + tmp;
            }
        }

        rc = _FinishCommon (pSession, pStreamInfo, usStream, FINISH_PLAYNOW);

        if (rc != HXR_OK)
        {
            switch (rc)
            {
            case HXR_NOT_AUTHORIZED:
                FailAndTeardown(401, pSession);
                break;
            case HXR_BAD_TRANSPORT:
                FailAndTeardown(461, pSession);
                break;
            default:
                // any other error
                FailAndTeardown(500, pSession);
                break;
            }

            return rc;
        }

        rc = m_pResp->HandleStreamSetup(pSession->m_sessionID,
                                        usStream, 
                                        pStreamInfo->m_uStreamGroupNumber);
    }

    HandleAltServerProxy(pSession);

    // Add the RDTFeatureLevel header to the Response
    //XXXTDM: should probably not be sent with non-RDT transports
    AddRDTFeatureLevelHeader(pSession->m_ulRDTFeatureLevel);

    // Add the StatsMask header in the Response
    AddStatsHeaders();

    /// FinishAllSetups triggers packet path construction
    rc = FinishAllSetups(pSession);

    // Mark the session used
    if (pSession->m_bUnused)
    {
        pSession->m_bUnused = FALSE;
        m_nUnusedSessions--;
    }
    pSession->m_bPlayResponseDone = FALSE;

    /** ruleList is passed by setupPlay() to RTSPProtocol::HandlePlayRequest()
      * but it is currently never used. */
    CHXSimpleList ruleList;

    pSession->m_bNeedRTPSequenceNo = TRUE;

    UINT64 tBegin = pSession->m_ulPlayRangeStart;
    UINT64 tEnd = pSession->m_ulPlayRangeEnd;

    rc = setupPlay(pSession, pSession->m_sessionID, ruleList, tBegin, tEnd);

    if (SUCCEEDED(rc) &&  pSession->m_fPlayScale)
    {
        pSession->m_bScaleResponsePending = TRUE;
        rc = m_pResp->HandleScaleParam(pSession->m_sessionID, 
                                       pSession->m_fPlayScale);
    }

    if (SUCCEEDED(rc) && pSession->m_fPlaySpeed)
    {
        /** must be called after FinishAllSetups() - because we have to 
          * have completed our source setup in the player to do this */
        rc = m_pResp->HandleSpeedParam(pSession->m_sessionID, 
                                        pSession->m_fPlaySpeed);
    }

    if (SUCCEEDED(rc))
    {
        if (tBegin != 0)
        {   // Need to determine actual start point from file format
            pSession->m_ulPlayRangeStart = RTSP_PLAY_RANGE_BLANK;
        }
        else
        {
            pSession->m_ulPlayRangeStart = tBegin;
        }

        if (tEnd > 0 && tEnd != RTSP_PLAY_RANGE_BLANK)
        {
            pSession->m_ulPlayRangeEnd = tEnd;
        }
        else if (!pSession->m_bRangeResponsePending)
        {
            pSession->m_ulPlayRangeEnd = RTSP_PLAY_RANGE_BLANK;
        }

        m_bPlayReceived = TRUE;
    }
    else
    {
        SendResponse(400);
        pSession->m_bPlayResponseDone = TRUE;
        return HXR_FAIL;
    }

    /** SendPlayResponse gets kicked when scale/speed complete, see
      * SetStreamStartTime(). It is used to respond to a PLAY or PLAYNOW
      * request and will handle state transition. This process was 
      * started above by the calls above to the protocolo response objects
      * HandlePlayRequest (via setupPlay), HandleScaleParam and
      * HandleSpeedParam. */

    return HXR_OK;
}



/**
 * \brief SetAlreadyReadData - pass in first few bytes that were read by controller
 *
 * SetAlreadyReadData is called by the RMServer immediately after creating an
 * instance of this class. it allows the server to pass on data which has 
 * already been read in and was used to sniff the protocol, but still needs
 * to be parsed.
 *
 * \param pAlreadyReadData [in] : pointer to the data 
 * \param alreadyReadDataLen [in] : how many bytes of data there are
 *
 * \todo return an HX_RESULT so we can return an error if the alloc fails
 * \return void
 */

void
RTSPServerProtocol::SetAlreadyReadData(Byte* pAlreadyReadData,
                                       UINT32 alreadyReadDataLen)
{
    if (pAlreadyReadData && alreadyReadDataLen)
    {
        HX_RELEASE(m_pBufFrag);
        if (HXR_OK !=
            m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&m_pBufFrag))
        {
            return;
        }
        m_pBufFrag->Set(pAlreadyReadData, alreadyReadDataLen);
    }
}

/**
 * \brief SendPacket - its important that we document this method!
 * \todo SendPacket - its important that we document this method!
 *
 *
 * \param n/a
 *
 * \return result of the transport send, or HXR_UNEXPECTED if we failed to
 * find the session or get the transport.
 */
STDMETHODIMP
RTSPServerProtocol::SendPacket(BasePacket* pPacket, const char* pSessionID)
{
    /*
     * This function is called with a null terminated array of BasePackets
     */

    if (0 == pPacket)
    {
        return HXR_OK;
    }

    HX_RESULT result = HXR_OK;

    RTSPServerProtocol::Session* pSession;
    Transport* pTransport = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
        pTransport = pSession->getTransport(pPacket->GetStreamNumber());
    }
    if (pTransport)
    {
        result = pTransport->sendPacket(pPacket);

        if (result != HXR_OK)
        {
            return result;
        }
    }
    else
    {
        DPRINTF(D_INFO, ("Unable to find transport for stream %d\n",
            pPacket->GetStreamNumber()));
        result = HXR_UNEXPECTED;
    }

    return result;
}

/**
 * \brief StartPackets - kick the transport to start packets for this stream
 *
 * StartPackets - kick the transport to start packets for this stream. Need
 * to document how this works in more detail - when/how do we get here?
 *
 * \param uStreamNumber [in] : the stream to start the packets on
 * \param pSessionID [in]
 *
 * \return the return code from the transport or an error if no transport found
 */
STDMETHODIMP
RTSPServerProtocol::StartPackets(UINT16 uStreamNumber, const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession;
    Transport* pTransport = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
        pTransport = pSession->getTransport(uStreamNumber);
    }
    if (!pTransport)
    {
        /*
         * Must not return HXR_FAIL because player may request a packet
         * before the transport is set up
         */

        return HXR_FAIL;
    }

    return pTransport->startPackets(uStreamNumber);
}

/**
 * \brief StopPackets - tell the transport to stop sending packets for this stream
 *
 * StopPackets - kick the transport to stop packets for this stream. Need
 * to document how this works in more detail - when/how do we get here?
 *
 * \param uStreamNumber [in] : the stream to start the packets on
 * \param pSessionID [in]
 *
 * \return the return code from the transport or an error if no transport found
 */
STDMETHODIMP
RTSPServerProtocol::StopPackets(UINT16 uStreamNumber, const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession;
    Transport* pTransport = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
        pTransport = pSession->getTransport(uStreamNumber);
    }
    if (!pTransport)
    {
        /*
         * Must not return HXR_FAIL because player may request a packet
         * before the transport is set up
         */

        return HXR_FAIL;
    }

    return pTransport->stopPackets(uStreamNumber);
}

STDMETHODIMP
RTSPServerProtocol::SendRTTResponse(UINT32 secs, UINT32 uSecs,
    const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession;
    Transport* pTransport = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
	pTransport = pSession->getFirstTransportSetup();
    }

    if ((pTransport &&
        ((pTransport->tag() == RTSP_TR_TNG_UDP) ||
         (pTransport->tag() == RTSP_TR_RDT_UDP))))
    {
        return ((RDTUDPTransport*)pTransport)->sendRTTResponsePacket(
            secs, uSecs);
    }
    return HXR_UNEXPECTED;
}

STDMETHODIMP
RTSPServerProtocol::SendCongestionInfo(INT32 xmitMultiplier,
    INT32 recvMultiplier, const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession;
    Transport* pTransport = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
	pTransport = pSession->getFirstTransportSetup();
    }
    if ((pTransport &&
        ((pTransport->tag() == RTSP_TR_TNG_UDP) ||
         (pTransport->tag() == RTSP_TR_RDT_UDP))))
    {
        return ((RDTUDPTransport*)pTransport)->sendCongestionPacket(
            xmitMultiplier, recvMultiplier);
    }
    return HXR_UNEXPECTED;
}

STDMETHODIMP
RTSPServerProtocol::SendStreamDone(UINT16 streamID, const char* pSessionID)
{
    // Who calls this?  When?  It doesn't seem to be used.
    RTSPServerProtocol::Session* pSession;
    Transport* pTransport = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
	pTransport = pSession->getFirstTransportSetup();
    }
    if (pTransport)
    {
            return pTransport->streamDone(streamID);
    }
    return HXR_UNEXPECTED;
}

STDMETHODIMP
RTSPServerProtocol::SetConnectionTimeout(UINT32 uSeconds)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPServerProtocol::SetFFHeaderAdvise(IHXFileFormatHeaderAdvise* pAdvise, const char * pSessionID)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    if(pSession)
    {
        HX_RELEASE(pSession->m_pFFAdvise);
        pSession->m_pFFAdvise = pAdvise;
        if (pAdvise)
        {
            pAdvise->AddRef();
        }
    }
    return HXR_OK;
}

// IHXFileFormatHeaderAdvise methods

STDMETHODIMP
RTSPServerProtocol::OnHeadersDone(HX_RESULT status,
                                  UINT32 ulErrNo)
{
    /*
     * This is the response method for IHXFileFormatHeaderAdvise::OnHeaders.
     * It is called from OnSetupRequest when the file format object supports
     * the interface.  Currently it is only used by the media export code.
     */

    if (FAILED(status))
    {
        // Find the session using our saved session id
        RTSPServerProtocol::Session* pSession = getSession(m_pSessionIDTmp);

        // Error code should be 4xx or 5xx
        if (ulErrNo < 400 || ulErrNo > 599)
        {
            HX_ASSERT(FALSE);
            ulErrNo = 500;
        }

        FailAndTeardown(ulErrNo, pSession);
    }
    else
    {
        _FinishSetup(m_usStreamNumberTmp, m_pSessionIDTmp);
    }

    return HXR_OK;
}

// IHXAccurateClock methods

STDMETHODIMP_(HXTimeval)
RTSPServerProtocol::GetTimeOfDay(void)
{
    // Oh the joys of having nonstandard system overrides
    HXTime now;
    HXTimeval hxnow;

    gettimeofday(&now, NULL);
    hxnow.tv_sec = now.tv_sec;
    hxnow.tv_usec = now.tv_usec;

    return hxnow;
}

STDMETHODIMP
RTSPServerProtocol::SetupSequenceNumberResponse(const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    if (pSession == NULL || pSession->m_pTransportStreamMap == NULL)
    {
        return HXR_FAIL;
    }

    if ((pSession->m_unStreamsRTPInfoReady) < pSession->m_uTotalSetupReceived)
    {
        return HXR_FAIL;
    }

    BOOL bIsFirst = TRUE;
    CHXMapLongToObj::Iterator i;
    for (i=pSession->m_pTransportStreamMap->Begin();
        i!=pSession->m_pTransportStreamMap->End();
        ++i)
    {
        Transport* pTransport = (Transport*)(*i);
        UINT16 streamNumber = (UINT16)i.get_key();
        /* if inactive, don't add this */
        if (pSession->m_pbSETUPRcvdStrm)
        {
            if (!pSession->m_pbSETUPRcvdStrm[streamNumber])
            {
                continue;
            }
        }

        /*
         * PPM has already setup the sequence numbers for the next play()
         */
        UINT16 seqNum = pTransport->getFirstSeqNum(streamNumber);
        UINT32 ulTimestamp = pTransport->getFirstTimestamp(streamNumber);
        RTSPStreamInfo* pStreamInfo;
        const char* pStreamUrl;
        HX_ASSERT(streamNumber < pSession->m_uStreamCount);
        pStreamInfo = pSession->m_ppStreamInfo[streamNumber];
        HX_ASSERT(pStreamInfo != NULL);
        pStreamUrl = pStreamInfo->m_streamControl;

        // ", url=<url>;seq=<n>;rtptime=<n>"
        NEW_FAST_TEMP_STR(pInfoField, 1024, strlen(pStreamUrl)+64);
        if (!bIsFirst)
        {
            sprintf(pInfoField, ", url=%s;seq=%u;rtptime=%u",
                    pStreamUrl, (unsigned)seqNum, (unsigned)ulTimestamp);
            pSession->m_streamSequenceNumbers += pInfoField;
        }
        else
        {
            bIsFirst = FALSE;
            sprintf(pInfoField, "url=%s;seq=%u;rtptime=%u",
                    pStreamUrl, (unsigned)seqNum, (unsigned)ulTimestamp);
            pSession->m_streamSequenceNumbers = pInfoField;
        }
        DELETE_FAST_TEMP_STR(pInfoField);
    }
    return HXR_OK;
}

/*
 * IHXTCPResponse methods
 */

STDMETHODIMP
RTSPServerProtocol::ConnectDone(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

/**
 * \brief ReadDone - we have incoming data on the RTSP control channel!
 *
 * ReadDone is invoked when the socket informs us that there is a read event
 * pending. If the status is HXR_OK, we pass the data on to the handleInput()
 * method for parsing, otherwise we notify the protocol response object of a
 * problem.
 *
 * \param status [in] : tells us if the read succeeded
 * \param pBuffer [in] : IHXBuffer with the data buffer
 *
 * \return HXR_OK unless handleInput reported an error.
 */
STDMETHODIMP
RTSPServerProtocol::ReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT hresult = HXR_OK;

    if (status == HXR_OK)
    {
        hresult = handleInput(pBuffer);
	if (hresult != HXR_OK && m_pResp)
        {
	    // for now we will abort with HXR_INVALID_PROTOCOL, even tho the
	    // reason might have been that the server is out of memory, or a
	    // QI failed.
            m_pResp->HandleProtocolError(HXR_INVALID_PROTOCOL);
	}
    }
    else
    {
        if (m_pResp)
        {
            m_pResp->HandleProtocolError(HXR_NET_SOCKET_INVALID);
        }
    }

    return hresult;
}

STDMETHODIMP
RTSPServerProtocol::WriteReady(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPServerProtocol::Closed(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

/**
 * \brief EventPending - handles events on the control channel socket
 *
 * \param uEvent [in] : read, close or write
 * \param status [in] 
 *
 * \return HXR_OK
 */
STDMETHODIMP
RTSPServerProtocol::EventPending(UINT32 uEvent, HX_RESULT status)
{
    IHXBuffer* pBuf = NULL;
    HX_RESULT hxr;
    switch (uEvent)
    {
    case  HX_SOCK_EVENT_READ:
        hxr = m_pSocket->Read(&pBuf);
        if (SUCCEEDED(hxr) && pBuf != NULL)
        {
            ReadDone(HXR_OK, pBuf);
        }
        else
        {
            ReadDone(HXR_FAIL, NULL);

            // PR 136743: After a failed read, the transport object might 
            // try to write to the socket after it's closed.
            //
            // Pass CLOSE pending to transport objects on failed read.
            // Transport objects are not handling READ events and there is
            // currently no way to communicate read failure.
            // Transport objects should handle READ events and read failure 
            // in EventPending().
            if ((m_pWriteNotifyMap) && !(m_pWriteNotifyMap->IsEmpty()))
            {
                for (CHXMapPtrToPtr::Iterator iter = m_pWriteNotifyMap->Begin();
                        iter != m_pWriteNotifyMap->End(); ++iter)
                {
                    IUnknown* punkItem = reinterpret_cast<IUnknown*>(*iter);
                    HX_ASSERT(punkItem != NULL);
                    IHXSocketResponse* pResp = NULL;
                    punkItem->QueryInterface(IID_IHXSocketResponse,
                            (void**)&pResp);
                    HX_ASSERT(pResp);

                    pResp->EventPending(HX_SOCK_EVENT_CLOSE, status);
                    pResp->Release();
                }
            }
        }
        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
	hxr = m_pResp->HandleProtocolError(HXR_NET_SOCKET_INVALID);
        if ((m_pWriteNotifyMap) && !(m_pWriteNotifyMap->IsEmpty()))
        {
            for (CHXMapPtrToPtr::Iterator iter = m_pWriteNotifyMap->Begin();
                    iter != m_pWriteNotifyMap->End(); ++iter)
            {
                IUnknown* punkItem = reinterpret_cast<IUnknown*>(*iter);
                HX_ASSERT(punkItem != NULL);
                IHXSocketResponse* pResp = NULL;
                punkItem->QueryInterface(IID_IHXSocketResponse,
                        (void**)&pResp);
                HX_ASSERT(pResp);

                pResp->EventPending(uEvent, status);
                pResp->Release();
            }
        }
	if (m_ulKeepAliveCallbackID)
	{   
	    m_pIScheduler->Remove(m_ulKeepAliveCallbackID);
	    m_ulKeepAliveCallbackID = 0;
	}
        break;
    case HX_SOCK_EVENT_WRITE:
        /*
           Round robin the write notification
           to the TCP transports. The RTSP
           does not need write notification
           since its write attempts are always queued
           The transports, however, stop writing
           when one packet is queued, and await
           a write event to continue:
         */
        if ((m_pWriteNotifyMap) && !(m_pWriteNotifyMap->IsEmpty()))
        {
            BOOL bBlocked = FALSE;
            CHXMapPtrToPtr::Iterator iter = m_pWriteNotifyMap->Find(m_pWriteNotifyLastKey);
            if (m_pWriteNotifyLastKey == NULL || iter == m_pWriteNotifyMap->End())
            {
                iter = m_pWriteNotifyMap->Begin();
            }
            m_pWriteNotifyLastKey = reinterpret_cast<IUnknown*>(iter.get_key());
            do
            {
                IUnknown* punkItem = reinterpret_cast<IUnknown*>(*iter);
                HX_ASSERT(punkItem != NULL);
                IHXSocketResponse* pResp = NULL;
                punkItem->QueryInterface(IID_IHXSocketResponse,
                        (void**)&pResp);
                HX_ASSERT(pResp);

                bBlocked = (pResp->EventPending(uEvent, status) ==
                        HXR_BLOCKED);
                pResp->Release();

                if (++iter == m_pWriteNotifyMap->End())
                {
                    iter = m_pWriteNotifyMap->Begin();
                }
            }
            while (!(bBlocked || (iter.get_key() == m_pWriteNotifyLastKey)));

            m_pWriteNotifyLastKey = reinterpret_cast<IUnknown*>(iter.get_key());
        }
        break;
    default:
        HX_ASSERT(FALSE);
    }
    HX_RELEASE(pBuf);
    return HXR_OK;
}

/**
 * \todo The paths data can follow through RTSPServerProtocol need to be
 * thoroughly documented.
 */
STDMETHODIMP
RTSPServerProtocol::PacketReady(HX_RESULT status, const char* pSessionID,
    IHXPacket* pPacket)
{
    if (!m_bSetupRecord)
    {
        RTSPServerProtocol::Session* pSession = getSession(pSessionID);

        // Update active timestamp for keepalive.
        // We have received UDP backchannel data of some kind.
        if (pSession)
        {
            pSession->SetSessionActiveStamp();
        }

        return HXR_OK;
    }
    else
    {
        if (status == HXR_OK)
        {
            return m_pResp->HandlePacket(HXR_OK, pSessionID, pPacket);
        }
        else
        {
            return m_pResp->HandlePacket(status, pSessionID, 0);
        }
    }
}

/*
 * OnRTTResponse() and OnCongestion() are client-side functions
 */

STDMETHODIMP
RTSPServerProtocol::OnRTTRequest(HX_RESULT status, const char* pSessionID)
{
    HXTimeval now = m_pAccurateClock->GetTimeOfDay();

    RTSPServerProtocol::Session* pSession;
    Transport* pTransport = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
	pTransport = pSession->getFirstTransportSetup();
    }
    if (pTransport &&
        (pTransport->tag() == RTSP_TR_TNG_UDP) ||
         (pTransport->tag() == RTSP_TR_RDT_UDP))
    {
        ((RDTUDPTransport*)pTransport)->sendRTTResponsePacket(
            now.tv_sec, now.tv_usec);
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::OnRTTResponse(HX_RESULT status, const char* pSessionID,
    UINT32 ulSecs, UINT32 ulUSecs)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
RTSPServerProtocol::OnBWReport(HX_RESULT status, const char* pSessionID,
    INT32 aveBandwidth, INT32 packetLoss, INT32 bandwidthWanted)
{
    return m_pResp->HandleBWReport(status, pSessionID, aveBandwidth,
        packetLoss, bandwidthWanted);
}

STDMETHODIMP
RTSPServerProtocol::OnCongestion(HX_RESULT status, const char* pSessionID,
    INT32 xmitMultiplier, INT32 recvMultiplier)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
RTSPServerProtocol::OnStreamDone(HX_RESULT status, UINT16 uStreamNumber)
{
    // This is a sink (client) function, it is not called for sources
    return m_pResp->HandleStreamDone(status, uStreamNumber);
}

STDMETHODIMP
RTSPServerProtocol::OnSourceDone()
{
    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::OnACK(HX_RESULT status, RTSPResendBuffer* pResendBuffer,
    UINT16 uStreamNumber, const char* pSessionID,
    UINT16* pAckList, UINT32 uAckListCount,
    UINT16* pNakList, UINT32 uNakListCount)
{
    /*
     * While it's ACKing, remote client is alive
     */

    RTSPServerProtocol::Session* pSession = getSession(pSessionID);
    if (pSession == NULL)
    {
        return HXR_FAIL;
    }

    /*
     * It's possible to receive an ACK after the connection has died
     */
    if (!pSession->m_pPacketResend)
    {
        return HXR_OK;
    }

    Transport* pTrans = pSession->getTransport(uStreamNumber);

    return handleACK(pSession->m_pPacketResend, pResendBuffer,
                     uStreamNumber,
                     pAckList, uAckListCount,
                     pNakList, uNakListCount,
                     pTrans ? pTrans->isBCM() : FALSE);
};

STDMETHODIMP
RTSPServerProtocol::OnProtocolError(HX_RESULT status)
{
    // called from transport layer
    return m_pResp->HandleProtocolError(status);
}

/*
 * private RTSPServerProtocol methods
 */
/**
 * \brief handleTCPData : handle interleaved TCP data
 *
 * Per the RTSP spec, if the transport is TCP, data can be interleaved
 * over the RTSP control channel using a channel ID
 *
 * \param pSessionID [in]
 * \param pData [in] : the interleaved data buffer
 * \param dataLen [in] : amount of data received
 * \param channel [in] : channel ID for this data
 *
 * \return HX_RESULT returned by the session (or error if session not found)
 */

HX_RESULT
RTSPServerProtocol::handleTCPData(const char* pSessionID,
    BYTE* pData, UINT16 dataLen, UINT16 channel)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    if (pSession == NULL)
    {
        return HXR_FAIL;
    }

    return pSession->handleTCPData(pData, dataLen, channel);
}


/**
 * \brief getStreamDescriptionInstance - get the Stream Description handler
 *
 * getStreamDescriptionInstance gets the Stream Description handler for this
 * mime type. This is used to assemble the session description in the DESCRIBE 
 * response (in the desired format as indicated by the mime type).
 *
 * \param n/a
 *
 * \return 
 */
IHXStreamDescription*
RTSPServerProtocol::getStreamDescriptionInstance(const char* pMimeType)
{
    IHXStreamDescription* pSD = 0;
    PluginHandler* pPHandler = 0;
    m_pContext->QueryInterface(IID_IHXPluginHandler,
                               (void**)&pPHandler);
    if (pPHandler)
    {
        PluginHandler::StreamDescription* pSDHandler;
        PluginHandler::Errors             pluginResult;
        PluginHandler::Plugin*            pPlugin;

        pSDHandler = pPHandler->m_stream_description_handler;
        pluginResult = pSDHandler->Find(pMimeType, pPlugin);
        if (PluginHandler::NO_ERRORS == pluginResult)
        {
            IUnknown* pInstance = 0;
            pPlugin->GetInstance(&pInstance);
            if (pInstance)
            {
                HX_RESULT rc;
                rc = pInstance->QueryInterface(IID_IHXStreamDescription,
                                           (void**)&pSD);
                if (rc == HXR_OK)
                {
                    IHXPlugin* pSDPlugin = 0;
                    rc = pSD->QueryInterface(IID_IHXPlugin,
                        (void**)&pSDPlugin);
                    if (rc == HXR_OK)
                    {
                        pSDPlugin->InitPlugin(m_pContext);
                        pSDPlugin->Release();
                    }
                }
                pInstance->Release();
            }
            pPlugin->ReleaseInstance();
        }
        pPHandler->Release();
    }
    return pSD;
}

HX_RESULT
RTSPServerProtocol::addEntityHeaders(
        RTSPServerProtocol::Session* pSession, const char* mimeType,
        IHXBuffer* pDescription, IHXValues* pResponseHeaders)
{
    if (pDescription == NULL)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    IHXMIMEHeader* pHeader = NULL;

    AddRFC822Headers(m_pRespMsg, pResponseHeaders);

    CHXString strContentBase = pSession->m_describeURL + "/";
    pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHeader->SetFromString("Content-base", strContentBase);
    m_pRespMsg->AddHeader(pHeader);

    if (pSession->m_bRetainEntityForSetup)
    {
        pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pHeader->SetFromString("ETag", pSession->m_sessionID);
        m_pRespMsg->AddHeader(pHeader);
    }

    if (pSession->m_bNeedSessionHeader)
    {
        // XXXTDM
        pSession->AddSessionHeader(m_pRespMsg);
    }

    pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHeader->SetFromString("Vary", "User-Agent, ClientID");
    m_pRespMsg->AddHeader(pHeader);

    pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHeader->SetFromString("Content-type", mimeType);
    m_pRespMsg->AddHeader(pHeader);

    pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHeader->SetFromString("x-real-usestrackid", "1");
    m_pRespMsg->AddHeader(pHeader);

    m_pRespMsg->SetContent(pDescription);

    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::sendInitialMessage()
{
    return HXR_OK;
}

/**
 * \brief getSession - find the RTSP session in the session ID map
 *
 * \param pSessionID [in]
 * \param bCreate [in] : if TRUE, create a session if it does not already exist
 *
 * \return returns a pointer to the session found or created
 */
RTSPServerProtocol::Session*
RTSPServerProtocol::getSession(const char* pSessionID, BOOL bCreate /* = FALSE */)
{
    RTSPServerProtocol::Session* pSession = NULL;
    if (pSessionID && *pSessionID && m_pSessions)
    {
        BOOL bRes = m_pSessions->Lookup(pSessionID, (void*&)pSession);
        HX_ASSERT(bRes || pSession == NULL);
        if (!bRes && bCreate)
        {
            pSession = new RTSPServerProtocol::Session(pSessionID, this);
            pSession->AddRef();
            (*m_pSessions)[pSessionID] = pSession;
            pSession->Init(m_pContext);
            m_nUnusedSessions++;
        }
    }
    return pSession;
}

/**
 * \brief GetSessionOriginatingStreamNumber - gets the stream of the last RTSP request
 *
 * GetSessionOriginatingStreamNumber is called when we have all the file and
 * stream headers and its time to send a SETUP response - but we need the stream
 * that the SETUP request was for. Well, here is where we saved it.
 *
 * \param pSessionID [in]
 *
 * \return returns stream number from the request or RTSP_UNKNOWN (???)
 * \todo return an error or an illegal value, not RTSP_UNKNOWN!
 */
UINT16
RTSPServerProtocol::GetSessionOriginatingStreamNumber(const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = 0;
    if (m_pSessions)
    {
        BOOL bLookupResult = m_pSessions->Lookup(pSessionID, (void*&)pSession);
        if (bLookupResult)
        {
            RTSPStreamInfo* pStream =
                pSession->GetStreamFromControl(pSession->m_ulControlID);
            if (pStream)
            {
                return pStream->m_streamNumber;
            }
        }
    }
    return RTSP_UNKNOWN;
}

/**
 * \brief GetSessionOriginatingMessage - gets the method of the last RTSP request
 *
 * GetSessionOriginatingMessage is called when we have all the file and stream 
 * headers and its time to send a DESCRIBE or SETUP response - but we've 
 * forgotten what the question was. Oh wait, I know what it is.
 *
 * \param pSessionID [in]
 *
 * \return returns originating method or RTSP_UNKNOWN
 */
RTSPMethod
RTSPServerProtocol::GetSessionOriginatingMessage(const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = 0;
    if (m_pSessions)
    {
        BOOL bLookupResult = m_pSessions->Lookup(pSessionID, (void*&)pSession);
        if (bLookupResult)
        {
            return pSession->m_RTSPMessageTagOriginating;
        }
    }
    return RTSP_UNKNOWN;
}

STDMETHODIMP
RTSPServerProtocol::SetInitialTS(const char* pSessionID, UINT32 ulInitialTS)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    if (!pSession)
    {
        return HXR_OK;
    }

    pSession->SetInitialHXTS(ulInitialTS);
    return HXR_OK;
}
STDMETHODIMP_(BOOL)
RTSPServerProtocol::NeedInitialTS(const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    if (!pSession)
    {
        return FALSE;
    }

    return pSession->NeedInitialHXTS();
}
STDMETHODIMP_(UINT32)
RTSPServerProtocol::GetInitialTS(const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    if (!pSession)
    {
        return 0;
    }

    return pSession->GetInitialHXTS();
}
STDMETHODIMP
RTSPServerProtocol::ClearInitialTS(const char* pSessionID)
{
    RTSPServerProtocol::Session* pSession = getSession(pSessionID);

    if (pSession)
    {
        pSession->ClearInitialHXTS();
    }
    return HXR_OK;
}

void
RTSPServerProtocol::SetStatus(UINT32 ulSessionRegistryNumber,
                              UINT32 ulSessionStatsObjId,
                              const INT32 nStatus)
{
    HX_ASSERT(m_pClientStatsMgr);
    IHXClientStats* pClientStats = NULL;
    IHXSessionStats* pSessionStats = NULL;

    // On the proxy, status codes are set in the RTSPProxy objects.
    if (m_bIsProxy)
    {
        return;
    }

    pClientStats = m_pClientStatsMgr->GetClient(m_ulClientStatsObjId);

    if (!pClientStats)
    {
        return;
    }

    pSessionStats = pClientStats->GetSession(ulSessionStatsObjId);
    HX_ASSERT(pSessionStats);

    if (pSessionStats)
    {
        pSessionStats->SetStatus(nStatus);
    }

    if (m_pRegistry && m_pClientStatsMgr->UseRegistryForStats())
    {
        char pRegkey[64];
        sprintf(pRegkey,
                "client.%lu.Session.%lu.Status",
                m_ulRegistryConnId,
                ulSessionRegistryNumber);

        if(!m_pRegistry->AddInt(pRegkey, nStatus))
        {
            m_pRegistry->SetIntByName(pRegkey, nStatus);
        }
    }

    HX_RELEASE(pClientStats);
    HX_RELEASE(pSessionStats);

}

void
RTSPServerProtocol::SetClientStatsObj(IHXClientStats* pClientStats)
{
    m_ulClientStatsObjId = pClientStats->GetID();

    if (m_bTrackEvents)
    {
        m_pRtspStatsMgr->SetClientStatsObj(pClientStats);
    }
}

/**
 * \brief SetIsProxy - if we are running inside a proxy, this is set TRUE
 *
 * Pretty silly to set this for each client, its not like it ever changes.
 *
 * \param  bIsProxy [in] : well, am I a proxy or what?
 *
 * \return void
 */
void
RTSPServerProtocol::SetIsProxy(BOOL bIsProxy)
{
    // This call now happens before init, so the RTSPStatsMgr should
    // NEVER exist when this fn is called.
    HX_ASSERT(!m_pRtspStatsMgr);

    m_bIsProxy = bIsProxy;
}

/**
 * \brief KeepAliveCheck - check session aliveness off a timer
 *
 * KeepAliveCheck checks the m_tSessionActiveStamp on each session. If the
 * elapsed time on a session exceeds the timeout interval we "ping" the 
 * session. If a session is not responding to pings it gets torn down.
 *
 * \param KeepAliveCallback [in] : so we can keep the timer running
 *
 * \return 
 */
void
RTSPServerProtocol::KeepAliveCheck(KeepAliveCallback* pCB)
{
    time_t    nCBDelta       = m_tKeepAliveInterval;
    BOOL      bKilledSession = FALSE;
    HXTimeval rmatv          = m_pScheduler->GetCurrentSchedulerTime();
    time_t    tNow           = rmatv.tv_sec;

    m_ulKeepAliveCallbackID = 0;
    HX_ASSERT(m_tKeepAliveInterval != 0);

    // enumerate the sessions to see if they need a ping
    // Use GetNextAssoc so we can delete sessions as we go along

    // After timeout period expires once, send a session-based ping.
    // If timeout period expires again, kill the session.

    if (m_pSessions)
    {
        POSITION pos = m_pSessions->GetStartPosition();
        while (pos)
        {
            time_t elapsed;
            RTSPServerProtocol::Session* pSession;
            const char* pSessionID;
            m_pSessions->GetNextAssoc(pos, pSessionID, (void*&) pSession);

            elapsed = tNow - pSession->m_tSessionActiveStamp;
            if (!pSession->m_bSessionKeepAliveExpired)
            {
                // checking if (first) keepalive period expired

                if (elapsed < m_tKeepAliveInterval)
                {
                    // we have received a message.  How long before the
                    // timeout would expire again?

                    nCBDelta = MIN(m_tKeepAliveInterval - elapsed, nCBDelta);
                }
                else
                {
                    // (first) timeout expired; send keepalive ping

                    SendKeepAlive(pSession);
                    pSession->m_tSessionActiveStamp = tNow;
                    pSession->m_bSessionKeepAliveExpired = TRUE;
                }
            }
            else
            {
                // m_bSessionKeepAliveExpired is cleared in Session::ReadDone(),
                // so we know we haven't gotten any messages since the ping.

                // ...but if there are multiple sessions, we can get a check
                // before the timeout period expires (we check based on the
                // next session to timeout), so we have to check this session's
                // time.

                if (elapsed < m_tKeepAliveInterval)
                {
                    nCBDelta = MIN(m_tKeepAliveInterval - elapsed, nCBDelta);
                }
                else
                {
                    // second timeout expired; log message

                    char logMsg[200];
                    const char* pSessionId = pSession->m_sessionID;
                    snprintf(logMsg, 200,
                             "RTSP session timed out, session ID <%s>\n",
                             pSessionId);
                    m_pErrorMessages->Report(HXLOG_INFO, 0, 0, logMsg, NULL);

                    // end session

                    bKilledSession = TRUE;
                    m_pResp->HandleTeardownRequest(pSessionId);
                    m_pSessionManager->removeSessionInstance(pSessionId);
                    SessionDone(pSessionId);
                }
            }
        }
    }
    /*
     * Shut down the rtsp connection if:
     *   - no sessions on connection;
     *   - we just now killed at least one session OR we already pinged the
     *      RTSP connection; AND
     *   - no activity on the rtsp channel.
     *      (there could be some activity on the channel for a new session that
     *      isn't SETUP yet)
     *
     * else,
     *   Send an RTSP ping if:
     *   - if no sessions; AND
     *   - no activity on RTSP channel for the timeout period.
     *
     *   Reset the callback.
     */

    BOOL bNoSessions = !m_pSessions || m_pSessions->IsEmpty();
    BOOL bRTSPInactive = tNow - m_tActiveStamp >= m_tKeepAliveInterval;

    if (bNoSessions &&
        (bKilledSession || m_bRTSPPingResponsePending) &&
        bRTSPInactive)
    {
        //set linger option to clean the send buffer 
        if (m_pSocket)
        {
            m_pSocket->SetOption(HX_SOCKOPT_LINGER, 0);
        }
        // kill connection
        m_pResp->HandleProtocolError(HXR_TIMEOUT);
    }
    else
    {
        if (bNoSessions && bRTSPInactive)
        {
            m_bRTSPPingResponsePending = TRUE;
            SendKeepAlive(NULL);
        }
        else
        {
            m_bRTSPPingResponsePending = FALSE;
        }

        /*
         * Reschedule keepalive callback.
         * nCBDelta was set for the next session to timeout; but add
         * a few seconds.  This keeps us from firing twice per timeout
         * in the single-session case in which we are getting no feedback.
         *
         * (Otherwise, we fire one timeout period after we send the keepalive
         * ping; if we got the response one second after the ping, we would
         * reschedule for one second from now to check the next timeout.
         * By waiting a few extra seconds, we can be sure to have timed out
         * on the first check, and can send the ping without rescheduling.)
         *
         * Also, multiple sessions with no feedback have a chance to get all
         * their pings in the same callback.
         */

        nCBDelta += 2;

        m_ulKeepAliveCallbackID =
            m_pIScheduler->RelativeEnter(pCB, nCBDelta * 1000);
    }
}

/**
 * \class RTSPServerProtocol::Session
 *
 * RTSPServerProtocol::Session maintains state for one RTSP session. This state
 * includes stream info about each stream, a prioritized transport list and a
 * bunch of booleans differentiating between various kinds of session, including
 * client, midbox (data connection from a downstream proxy), bcng (splitting) and 
 * accounting only (again, from a proxy).
 *
 * \param n/a
 *
 * \return 
 */

/**
 * \brief RTSPServerProtocol::Session constructor
 *
 * \param pSessionID [in] : the new session has been created
 * \param pServProt [in] : the RTSP protocol handler for this client
 *
 * \return n/a
 */
RTSPServerProtocol::Session::Session(const char* pSessionID, RTSPServerProtocol* pServProt):
    m_bUnused(TRUE),
    m_sessionID(pSessionID),
    m_uStreamCount(0),
    m_ppStreamInfo(NULL),
    m_pStreamUrl(NULL),
    m_pTransportStreamMap(0),
    m_pTransportPortMap(0),
    m_pTransportChannelMap(0),
    m_pFportTransResponseMap(0),
    m_pLocalAddr(NULL),
    m_bIsTNG(FALSE),
    m_sSetupCount(0),
    m_uTotalSetupReceived(0),
    m_bSetupsComplete(FALSE),
    m_ulRefCount(0),
    m_pUDPSocketList(0),
    m_RTSPMessageTagOriginating(RTSP_UNKNOWN),
    m_ulControlID(0),
    m_bRetainEntityForSetup(FALSE),
    m_bNeedSessionHeader(FALSE),
    m_bUseOldSdp(FALSE),
    m_bIsMidBox(FALSE),
    m_bIsViaRealProxy(FALSE),
    m_pMidBoxChallenge(NULL),
    m_bChallengeMet(FALSE),
    m_bSendClientRealChallenge3(TRUE),
    m_state(INIT),
    m_pPacketResend(0),
    m_bMulticastOK(FALSE),
    m_bRequireMulticast(FALSE),
    m_bIsRealDataType(TRUE),
    m_ulPacketCount(0),
    m_ulRDTFeatureLevel(0),
    m_bSupportsRARTPChallenge(FALSE),
    m_bSessionSetup(FALSE),
    m_pbSETUPRcvdStrm(NULL),
    m_punNumASMRules(NULL),
    m_bHandleRules(TRUE),
    m_bNeedInitialHXTS(TRUE),
    m_ulInitialHXTS(0),
    m_bBlockTransfer(FALSE),
    m_ulSessionRegistryNumber(0),
    m_ulSessionStatsObjId(0),
    m_ulStatsInterval(0),
    m_pReqMsg(0),
    m_pAltMgr(NULL),
    m_unStreamsRTPInfoReady(0),
    m_bAltEnabled(FALSE),
    m_pServAlt(NULL),
    m_pProxAlt(NULL),
    m_pCommonClassFactory(NULL),
    m_pScheduler(NULL),
    m_pFFAdvise(NULL),
    m_bIsLive(FALSE),
    m_ulSessionDuration(0),
    m_bPlayResponseDone(FALSE),
    m_bScaleResponsePending(FALSE),
    m_bRangeResponsePending(FALSE),
    m_bRTPInfoResponsePending(FALSE),
    m_bNeedRTPSequenceNo(FALSE),
    m_ulPlayRangeUnits(RTSPRange::TR_NPT),
    m_ulPlayRangeStart(RTSP_PLAY_RANGE_BLANK),
    m_ulPlayRangeEnd(RTSP_PLAY_RANGE_BLANK),
    m_fPlayScale(0),
    m_fPlaySpeed(0),
    m_ulStreamsEnded(0),
    m_bEmulatePVSession(FALSE),
    m_bSessionKeepAliveExpired(FALSE),
    m_ulMaxUDPSize(HX_SAFEUINT(MAX_UDP_PACKET)),
    m_pEventList(NULL),
    m_bUseControlID(FALSE),
    m_ABDState(ABD_READY),
    m_uFirstSetupStreamNum(0xFFFF),
    m_uStreamGroupCount(0),
    m_ulInitiationID(0),
    m_bNeedAggregateTransportHeader(FALSE),
    m_pTransportInstantiator(NULL),
    m_pAggregateTransportParams(NULL)
{
    m_pTransportList = new CHXSimpleList;
    m_pServProt             = pServProt;
    m_pServProt->AddRef();
    m_ulThreadSafeFlags = m_pServProt->GetThreadSafeFlags();

    m_ulStatsInterval = m_pServProt->m_pPropWatchResponse->GetStatsInterval();

}

void
RTSPServerProtocol::Session::SetInitialHXTS(UINT32 ulInitialTS)
{
    HX_ASSERT(m_bNeedInitialHXTS);
    m_bNeedInitialHXTS = FALSE;
    m_ulInitialHXTS = ulInitialTS;
}

void
RTSPServerProtocol::Session::ClearInitialHXTS()
{
    m_bNeedInitialHXTS = TRUE;
    m_ulInitialHXTS = 0;
}

/**
 * \brief RTSPServerProtocol::Session destructor
 *
 * \param n/a
 *
 * \return n/a
 */
RTSPServerProtocol::Session::~Session()
{

    HX_VECTOR_DELETE(m_pStreamUrl);
    clearStreamInfoList();
    clearDescribeMimeTypeList();

    HX_RELEASE(m_pTransportInstantiator);
    HX_RELEASE(m_pAggregateTransportParams);

    HX_DELETE(m_pMidBoxChallenge);
    HX_DELETE(m_pTransportStreamMap);
    HX_DELETE(m_pTransportPortMap);
    HX_DELETE(m_pTransportChannelMap);
    HX_DELETE(m_pFportTransResponseMap);
    HX_RELEASE(m_pPacketResend);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pScheduler);
    HX_DELETE(m_pEventList);

    HX_VECTOR_DELETE(m_pbSETUPRcvdStrm);
    HX_VECTOR_DELETE(m_punNumASMRules);
    HX_RELEASE(m_pFFAdvise);
}

STDMETHODIMP
RTSPServerProtocol::Session::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXUDPResponse))
    {
        AddRef();
        *ppvObj = (IHXUDPResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPContext))
    {
        AddRef();
        *ppvObj = (IHXRTSPContext*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXChallenge))
    {
        AddRef();
        *ppvObj = (IHXChallenge*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
        AddRef();
        *ppvObj = (IHXThreadSafeMethods*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAlternateServerProxyResponse))
    {
        AddRef();
        *ppvObj = (IHXAlternateServerProxyResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
RTSPServerProtocol::Session::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RTSPServerProtocol::Session::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

/**
 * \brief Init : initialize the RTSP session with its context
 *
 * \param pContext [in]
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPServerProtocol::Session::Init(IUnknown* pContext)
{
    m_pContext = pContext;

    HX_RELEASE(m_pCommonClassFactory);
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                               (void**) &m_pCommonClassFactory);

    HX_RELEASE(m_pScheduler);
    m_pContext->QueryInterface(IID_IHXScheduler,
                               (void**) &m_pScheduler);

    if (m_pServProt->m_bTrackEvents)
    {
        m_pEventList = new RTSPSessionEventsList();
        m_pEventList->Init(m_pServProt->m_pRtspStatsMgr, m_sessionID);
    }

    SetSessionActiveStamp();

    return HXR_OK;
}

/**
 * \brief Done : release transports and other resources used by the RTSP session
 *
 * \param n/a
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPServerProtocol::Session::Done()
{
    if (m_pAltMgr)
    {
        m_pAltMgr->ClearResponse(this);
        m_pAltMgr->Release();
        m_pAltMgr = NULL;
    }

    HX_RELEASE(m_pServAlt);
    HX_RELEASE(m_pProxAlt);

    if(m_pTransportList)
    {
        CHXSimpleList::Iterator i;
        for(i=m_pTransportList->Begin();i!=m_pTransportList->End();++i)
        {
            Transport* pTrans = (Transport*)(*i);
            if (m_pServProt->m_pWriteNotifyMap->Lookup(pTrans))
            {
                IUnknown* punkItem = reinterpret_cast<IUnknown*>((*(m_pServProt->m_pWriteNotifyMap))[pTrans]);
                HX_RELEASE( punkItem );
                m_pServProt->m_pWriteNotifyMap->Remove(pTrans);
            }
            pTrans->Done();
            pTrans->Release();
        }
        delete m_pTransportList;
        m_pTransportList = 0;
    }
    if (m_pUDPSocketList)
    {
        CHXSimpleList::Iterator i;
        for (i=m_pUDPSocketList->Begin();i!=m_pUDPSocketList->End();++i)
        {
            IHXSocket* pSocket = (IHXSocket*)(*i);
            pSocket->Release();
        }
        delete m_pUDPSocketList;
        m_pUDPSocketList = 0;
    }

    if (m_ulInitiationID)
    {
        (*m_pServProt->m_pPipelineMap)[m_ulInitiationID] = (void*)0;
    }

    HX_DELETE(m_pTransportPortMap);
    HX_DELETE(m_pTransportChannelMap);
    HX_RELEASE(m_pPacketResend);
    HX_RELEASE(m_pLocalAddr);
    HX_RELEASE(m_pServProt);
    HX_RELEASE(m_pReqMsg);
    HX_RELEASE(m_pTransportInstantiator);

    return HXR_OK;
}

/**
 * \brief handleChallengeResponse - handle a proxies challenge response
 *
 * The RTSP sessions handleChallengeResponse method is used to handle 
 * the dialogue with downstream proxies. Not sure why or if this code
 * could be shared with the RTSPServerProtocol::handleChallengeResponse
 *
 * \param pMsg [in] : the RTSP message with the challenge headers
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPServerProtocol::Session::handleChallengeResponse(IHXRTSPMessage* pMsg)
{
#if defined(HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE)
    IHXMIMEHeader* pMIMEHeaderChallenge = NULL;
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    IHXMIMEParameter* pParam = NULL;

    // Verify the challenge response
    if (HXR_OK == pMsg->GetHeader("RealChallenge2", pMIMEHeaderChallenge) &&
        m_pMidBoxChallenge)
    {
        pMIMEHeaderChallenge->GetFieldListConst(pListField);
        pIterField = pListField->Begin();
        HX_RELEASE(pListField);

        if (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetFirstParam(pParam);
            HX_RELEASE(pField);

            char* pClientResponse = NULL;
            const char* pSD = NULL;

            // Retrieve the challenge response

            if (pParam)
            {
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);
                pClientResponse = new char[pBufAttr->GetSize()+1];
                memcpy(pClientResponse,
                    (const char *)pBufAttr->GetBuffer(),
                    pBufAttr->GetSize());
                *(pClientResponse+pBufAttr->GetSize()) = '\0';
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);

                pIterField->MoveNext();

                if (pIterField->HasItem())
                {
                    pUnkField = pIterField->GetItem();
                    pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
                    HX_RELEASE(pUnkField);
                    pField->GetFirstParam(pParam);
                    HX_RELEASE(pField);

                    // Retrieve the challenge response
                    if (pParam)
                    {
                        pParam->Get(pBufAttr, pBufVal);
                        HX_RELEASE(pParam);
                        if (pBufVal)
                        {
                            pSD = (const char*)pBufVal->GetBuffer();
                        }
                        HX_RELEASE(pBufAttr);
                        HX_RELEASE(pBufVal);
                    }
                }

                // Calculate the proper response
                m_pMidBoxChallenge->response1((BYTE*)m_pMidBoxChallenge->challenge);
                if (!strcmp((char*)m_pMidBoxChallenge->response,
                            pClientResponse))
                {
                    // Validation successful!!!
                    // Verify the sd and set the trap variable
                    if (pSD && !strcmp((char*)m_pMidBoxChallenge->trap, pSD))
                    {
                        m_bChallengeMet = TRUE;
                    }

                    // Generate the server challenge response to be sent
                    // in the SETUP RESPONSE message, along with the sdr
                    m_pMidBoxChallenge->response2(m_pMidBoxChallenge->response);
                }
                HX_DELETE(pClientResponse);
            }
            HX_RELEASE(pParam);
        }
        HX_RELEASE(pIterField);
    }
    HX_RELEASE(pMIMEHeaderChallenge);
#endif /*HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE*/

    return HXR_OK;
}

/**
 * \brief Session::ReadDone : important to document how this works. Is it used?
 */
STDMETHODIMP
RTSPServerProtocol::Session::ReadDone(HX_RESULT status, IHXBuffer* pBuffer,
    UINT32 ulAddr, UINT16 sPort)
{
    HX_ASSERT(FALSE);

    HX_RESULT hresult = HXR_OK;

    if (status == HXR_OK)
    {
        if (!m_pTransportPortMap)
        {
            return HXR_UNEXPECTED;
        }

        Transport* pTrans =
            (Transport*)(*m_pTransportPortMap)[sPort];

        if (!pTrans)
        {
            DPRINTF(D_INFO, ("No transport for port %d\n", sPort));
        }
        else
        {
            // Update active timestamp for keepalive.
            // We have received UDP backchannel data of some kind.

            SetSessionActiveStamp();

            // handle the data

            hresult = pTrans->handlePacket(pBuffer);
            pTrans->releasePackets();
            return hresult;
        }
    }

    return HXR_FAIL;
}

/**
 * \brief SetSessionActiveStamp - we have some activity, so clear expired flag
 *
 * SetSessionActiveStamp - we have some activity, so clear expired flag and
 * set the m_tSessionActiveStamp to the current scheduler time. This is used
 * for keepalive timeouts.
 *
 * \param tTimeStamp [in] : time to set active time stamp to. Not used.
 *
 * \return void
 */
void
RTSPServerProtocol::Session::SetSessionActiveStamp(time_t tTimeStamp /* =0 */)
{
    // we have some activity, so clear expired flag

    m_bSessionKeepAliveExpired = FALSE;

    if (!tTimeStamp)
    {
        HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
        m_tSessionActiveStamp = rmatv.tv_sec;
    }
    else
    {
        m_tSessionActiveStamp = tTimeStamp;
    }
}


//
// IHXChallenge
//
STDMETHODIMP
RTSPServerProtocol::Session::SendChallenge(
    IHXChallengeResponse* pIHXChallengeResponseSender,
    IHXRequest* pIHXRequestChallenge)
{
    if (m_pServProt)
    {
        return m_pServProt->SendChallenge(
            pIHXChallengeResponseSender, pIHXRequestChallenge);
    }

    return HXR_FAIL;
}

STDMETHODIMP
RTSPServerProtocol::Session::OnModifiedEntry(HX_ALTERNATES_MOD_FLAG type)
{
    HX_ASSERT(m_pServProt);
    HX_ASSERT(m_pAltMgr);
    HX_ASSERT(HX_GET_ALTERNATES_FLAG(type, HX_ALT_SERVER) ||
              HX_GET_ALTERNATES_FLAG(type, HX_ALT_PROXY) ||
              HX_GET_ALTERNATES_FLAG(type, HX_ALT_DPATH));

    // get the correct URL to pass in
    UINT32 ulLen = m_describeURL.GetLength();
    char* pURL = (char*)(const char*)m_describeURL;

    // rtsp://
    if (ulLen <= 7 || strncasecmp(pURL, "rtsp://", 7) != 0)
    {
        // this shouldn't happen
        HX_ASSERT(!"m_describeURL is invalid!");
        // forget the alt
        m_pAltMgr->ClearResponse(this);
        m_pAltMgr->Release();
        m_pAltMgr = NULL;
        return HXR_OK;
    }

    pURL += 7;

    BOOL bAltEnabledNew = m_bAltEnabled;
    if (HX_GET_ALTERNATES_FLAG(type, HX_ALT_DPATH))
    {
        // server config has been modified
        bAltEnabledNew = m_pAltMgr->IsEnabled(pURL);
    }

    if (!bAltEnabledNew)
    {
        if (bAltEnabledNew != m_bAltEnabled)
        {
            /*
             * It was enabled, but not anymore.
             */
            HX_RELEASE(m_pServAlt);
            HX_RELEASE(m_pProxAlt);

            IHXBuffer* pFalse = NULL;
            m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                  (void**)&pFalse);
            pFalse->Set((BYTE*)"false", 6);

            m_pServProt->SendSetParameterRequest(m_sessionID,
                                                 m_describeURL,
                                                 "Reconnect",
                                                 pFalse);
            pFalse->Release();
        }

        m_bAltEnabled = bAltEnabledNew;

        HX_ASSERT(!m_pServAlt);
        HX_ASSERT(!m_pProxAlt);
        return HXR_OK;
    }

    IHXBuffer* pSendServAlt = NULL;
    IHXBuffer* pSendProxAlt = NULL;
    IHXBuffer* pNewServAlt = NULL;
    IHXBuffer* pNewProxAlt = NULL;

    HX_RESULT theErr = HXR_FAIL;
    if (HX_GET_ALTERNATES_FLAG(type, HX_ALT_SERVER) &&
        HX_GET_ALTERNATES_FLAG(type, HX_ALT_PROXY))
    {
        theErr = m_pAltMgr->GetAltServerProxy(pURL, pNewServAlt, pNewProxAlt);
    }
    else if (HX_GET_ALTERNATES_FLAG(type, HX_ALT_SERVER))
    {
        theErr = m_pAltMgr->GetAltServers(pURL, pNewServAlt);
    }
    else if (HX_GET_ALTERNATES_FLAG(type, HX_ALT_PROXY))
    {
        theErr = m_pAltMgr->GetAltProxies(pURL, pNewProxAlt);
    }

    HX_ASSERT((SUCCEEDED(theErr) && (pNewServAlt || pNewProxAlt)) ||
              (FAILED(theErr) && (!pNewServAlt && !pNewProxAlt)));


    /*
     * figure out what to send and update vars
     */
    if (bAltEnabledNew == m_bAltEnabled)
    {
        HX_ASSERT(pNewServAlt || pNewProxAlt);
        /*
         * it was enabled and still is.  We need to send a msg if either
         * server or proxy alt string has changed.
         */
        if (pNewServAlt && m_pServAlt)
        {
            if (strcasecmp((const char*)pNewServAlt->GetBuffer(),
                           (const char*)m_pServAlt->GetBuffer()) != 0)
            {
                pSendServAlt = pNewServAlt; // take AddRef()
                pNewServAlt = NULL;
                m_pServAlt->Release();
                m_pServAlt = pSendServAlt;
                m_pServAlt->AddRef();
            }
            else
            {
                // same string...don't send, but update vars
                HX_ASSERT(!pSendServAlt);
                m_pServAlt->Release();
                m_pServAlt = pNewServAlt; // take AddRef()
                pNewServAlt = NULL;
            }
        }
        else if (pNewServAlt)
        {
            HX_ASSERT(!m_pServAlt);
            pSendServAlt = pNewServAlt; // take AddRef()
            pNewServAlt = NULL;
            m_pServAlt = pSendServAlt;
            m_pServAlt->AddRef();
        }
        else if (m_pServAlt)
        {
            pSendServAlt = m_pServAlt;
            pSendServAlt->AddRef();
        }

        if (pNewProxAlt && m_pProxAlt)
        {
            if (strcasecmp((const char*)pNewProxAlt->GetBuffer(),
                           (const char*)m_pProxAlt->GetBuffer()) != 0)
            {
                pSendProxAlt = pNewProxAlt; // take AddRef()
                pNewProxAlt = NULL;
                m_pProxAlt->Release();
                m_pProxAlt = pSendProxAlt;
                m_pProxAlt->AddRef();
            }
            else
            {
                // same string...don't send, but update vars
                HX_ASSERT(!pSendProxAlt);
                m_pProxAlt->Release();
                m_pProxAlt = pNewProxAlt; // take AddRef()
                pNewProxAlt = NULL;
            }
        }
        else if (pNewProxAlt)
        {
            HX_ASSERT(!m_pProxAlt);
            pSendProxAlt = pNewProxAlt; // take AddRef()
            pNewProxAlt = NULL;
            m_pProxAlt = pSendProxAlt;
            m_pProxAlt->AddRef();
        }
        else if (m_pProxAlt)
        {
            pSendProxAlt = m_pProxAlt;
            pSendProxAlt->AddRef();
        }
    }
    else
    {
        HX_ASSERT(!m_pServAlt && !m_pProxAlt);

        m_bAltEnabled = bAltEnabledNew;

        /*
         * Wasn't enabled, but it is now.  Send whatever we find.
         */
        theErr = m_pAltMgr->GetAltServerProxy(pURL, pNewServAlt, pNewProxAlt);
        if (SUCCEEDED(theErr))
        {
            // take AddRefs()
            if ((pSendServAlt = pNewServAlt) != NULL)
            {
                pNewServAlt = NULL;
                m_pServAlt = pSendServAlt;
                m_pServAlt->AddRef();
            }
            if ((pSendProxAlt = pNewProxAlt) != NULL)
            {
                pNewProxAlt = NULL;
                m_pProxAlt = pSendProxAlt;
                m_pProxAlt->AddRef();
            }
        }
        else
        {
            // there is no alts
            HX_ASSERT(!pNewServAlt && !pNewProxAlt);
            IHXBuffer* pTrue = NULL;
            m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                  (void**)&pTrue);
            pTrue->Set((BYTE*)"true", 5);
            m_pServProt->SendSetParameterRequest(m_sessionID,
                                                m_describeURL,
                                                "Reconnect",
                                                pTrue);
            pTrue->Release();

            // will return
        }
    }

#ifdef _DEBUG
    HX_ASSERT(!pNewServAlt && ! pNewProxAlt);

    IHXBuffer* pS = NULL;
    IHXBuffer* pP = NULL;
    m_pAltMgr->GetAltServerProxy(pURL, pS, pP);
    HX_ASSERT(pS ? pS == m_pServAlt : 1);
    HX_ASSERT(pP ? pP == m_pProxAlt : 1);
    if (pS && m_pServAlt)
    {
        HX_ASSERT(strcasecmp((const char*)pS->GetBuffer(),
                           (const char*)m_pServAlt->GetBuffer()) == 0);
    }
    if (pP && m_pProxAlt)
    {
        HX_ASSERT(strcasecmp((const char*)pP->GetBuffer(),
                             (const char*)m_pProxAlt->GetBuffer()) == 0);
    }
    HX_RELEASE(pS);
    HX_RELEASE(pP);
#endif

    if (pSendServAlt || pSendProxAlt)
    {
        IHXBuffer* pTrue = NULL;
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                              (void**)&pTrue);
        pTrue->Set((BYTE*)"true", 5);

        IHXValues* pVal = new CHXHeader();
        pVal->AddRef();

        if (pSendProxAlt)
        {
            pVal->SetPropertyCString("Alternate-Proxy", pSendProxAlt);
            HX_RELEASE(pSendProxAlt);
        }
        if (pSendServAlt)
        {
            pVal->SetPropertyCString("Alternate-Server", pSendServAlt);
            HX_RELEASE(pSendServAlt);
        }
        pVal->SetPropertyCString("Reconnect", pTrue);

        m_pServProt->SendSetParameterRequest(m_sessionID,
                                             m_describeURL,
                                             pVal);
        pTrue->Release();
        pVal->Release();
    }


    HX_ASSERT(!pNewServAlt && !pNewProxAlt && !pSendServAlt && !pSendProxAlt);
    return HXR_OK;
}


/**
 * \brief getTransport - lookup the transport for a given stream on this session
 *
 * \param streamNumber [in]
 *
 * \return pointer to the transport found, or null
 */
Transport*
RTSPServerProtocol::Session::getTransport(UINT16 streamNumber)
{
    if(m_pTransportStreamMap)
    {
        Transport* pTransport = 0;
        if(m_pTransportStreamMap->Lookup(streamNumber, (void*&)pTransport))
        {
            return pTransport;
        }
    }
    return 0;
}

Transport*
RTSPServerProtocol::Session::getFirstTransportSetup()
{
    return getTransport(m_uFirstSetupStreamNum);
}

/**
 * \brief mapTransportStream - map this transport to a given stream
 *
 * \param pTransport [in]
 * \param streamNumber [in]
 *
 * \return void
 */
void
RTSPServerProtocol::Session::mapTransportStream(Transport* pTransport,
    UINT16 streamNumber)
{
    if (!m_pTransportStreamMap)
    {
        m_pTransportStreamMap = new CHXMapLongToObj;
    }
    (*m_pTransportStreamMap)[streamNumber] = pTransport;
}

/**
 * \brief mapTransportChannel - map this transport to a given interleaved channel
 *
 * \param pTransport [in]
 * \param channel [in]
 *
 * \return void
 */
void
RTSPServerProtocol::Session::mapTransportChannel(Transport* pTransport,
    UINT16 channel)
{
    if (!m_pTransportChannelMap)
    {
        m_pTransportChannelMap = new CHXMapLongToObj;
    }
    (*m_pTransportChannelMap)[channel] = pTransport;
}


void
RTSPServerProtocol::Session::mapFportTransResponse(CHXRTCPTransMapSocket* pRTCPTransMapSock ,
    UINT16 fport)
{
    if (!m_pFportTransResponseMap)
    {
        m_pFportTransResponseMap = new CHXMapLongToObj;
    }
    HX_ASSERT(m_pFportTransResponseMap);
    (*m_pFportTransResponseMap)[fport] = pRTCPTransMapSock;
}

/**
 * \brief GetStreamFromControl - get stream info fomr stream number
 *
 * \param ulControl [in] : control ID is the same as stream number
 *
 * \return pointer to stream info or NULL if not found
 */
RTSPStreamInfo*
RTSPServerProtocol::Session::GetStreamFromControl(UINT32 ulControl)
{
    if (!m_ppStreamInfo)
    {
        return NULL;
    }

    if (!m_bUseControlID)
    {
        // control ID is the same as stream number, so just index it
        return ulControl < (UINT32)m_uStreamCount ?
                m_ppStreamInfo[ulControl] : NULL;
    }

    // Find the stream for this control ID
    for (UINT16 i = 0; i < m_uStreamCount; i++)
    {
        if (m_ppStreamInfo[i] &&
            m_ppStreamInfo[i]->m_ulControlID == ulControl)
        {
            return m_ppStreamInfo[i];
        }
    }

    return NULL;
}

void
RTSPServerProtocol::Session::clearStreamInfoList()
{
    if (m_ppStreamInfo != NULL)
    {
        UINT16 uStream;
        for (uStream = 0; uStream < m_uStreamCount; uStream++)
        {
            delete m_ppStreamInfo[uStream];
        }
        HX_VECTOR_DELETE(m_ppStreamInfo);

        m_uStreamCount = 0;
    }
}

void
RTSPServerProtocol::Session::clearDescribeMimeTypeList()
{

    CHXSimpleList::Iterator i;
    for (i=m_describeMimeTypeList.Begin();
        i!=m_describeMimeTypeList.End();
        ++i)
    {
        CHXString* pString = (CHXString*)(*i);
        delete pString;
    }
    m_describeMimeTypeList.RemoveAll();
}

/**
 * \brief AddSessionHeader - add the "Session" header to this RTSP message
 *
 * Add session id to the passed RTSP message, with timeout if there is one.
 * Originally written to only send the timeout the first time; but now
 * these routines always add the timeout.
 *
 * Two function signatures needed because there seem to be two hierarchies
 * of RTSP messages, and we use both of them in this file.
 *
 * \param pMsg
 *
 * \return HXR_OK
 */

HX_RESULT
RTSPServerProtocol::Session::AddSessionHeader(RTSPResponseMessage* pMsg)
{
    MIMEHeader* pHeader = new MIMEHeader("Session");
    MIMEHeaderValue* pHeaderValue;

    MakeSessionHeaderValue(&pHeaderValue);

    pHeader->addHeaderValue(pHeaderValue);
    pMsg->addHeader(pHeader);

    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::Session::AddSessionHeader(IHXRTSPMessage* pMsg)
{
    // using the non-IXH value because it supports addParameter()

    MIMEHeaderValue* pHeaderValue;
    IHXMIMEHeader* pHeader;

    MakeSessionHeaderValue(&pHeaderValue);

    // now assign it to an IHX header

    pHeader = new CMIMEHeader(m_pServProt->m_pFastAlloc);
    pHeader->SetFromString("Session", pHeaderValue->value());
    pMsg->AddHeader(pHeader);

    delete pHeaderValue;
    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::Session::MakeSessionHeaderValue(MIMEHeaderValue** ppVal)
{
    char value[40];
    *ppVal = new MIMEHeaderValue(m_sessionID);

    if (m_pServProt->m_tKeepAliveInterval)
    {
        snprintf(value, 40, "%d", (int)m_pServProt->m_tKeepAliveInterval);
        (*ppVal)->addParameter("timeout", value);
    }

    return HXR_OK;
}


/**
 * \brief handleTCPData : handle interleaved TCP data - obsolete variant? 
 *
 * This variant of handleTCPData is called by the RTSPServerProtocol::handleTCPData
 * method but I am not sure if it is used anywhere. We do use the IHXBuffer variant!
 *
 * \param pSessionID [in]
 * \param pData [in] : BYTE* pointing to the data
 * \param dataLen [in] : amount of data received
 * \param channel [in] : channel ID for this data
 *
 * \return HX_RESULT returned by the session (or error if session not found)
 */
HX_RESULT
RTSPServerProtocol::Session::handleTCPData(BYTE* pData, UINT16 dataLen, UINT16 channel)
{
    HX_ASSERT(m_pTransportChannelMap);
    HX_RESULT rc = HXR_OK;

    IHXBuffer* pBuffer = NULL;
    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                          (void**)&pBuffer);
    pBuffer->Set(pData, dataLen);

    Transport* pTrans = NULL;
    if (m_pTransportChannelMap->Lookup(channel, (void*&)pTrans))
    {
        rc = pTrans->handlePacket(pBuffer);
    }
#ifdef _DEBUG
    else
    {
        HX_ASSERT(!"make sure TransportChannelMap has been set up right...");
    }
#endif

    pBuffer->Release();

    return rc;
}

/**
 * \brief handleTCPData : handle interleaved TCP data
 *
 * Per the RTSP spec, if the transport is TCP, data can be interleaved
 * over the RTSP control channel using a channel ID. This routine is 
 * invoked from RTSPServerProtocol::DispatchMessage() when interleaved data
 * is found.
 *
 * \param pSessionID [in]
 * \param pBuffer [in] : the interleaved data buffer
 * \param channel [in] : channel ID for this data
 *
 * \return HX_RESULT returned by the session (or error if session not found)
 */
HX_RESULT
RTSPServerProtocol::Session::handleTCPData(IHXBuffer* pBuffer, UINT16 channel)
{
    HX_ASSERT(m_pTransportChannelMap);
    HX_RESULT rc = HXR_OK;

    Transport* pTrans = NULL;
    if (m_pTransportChannelMap->Lookup(channel, (void*&)pTrans))
    {
        rc = pTrans->handlePacket(pBuffer);
    }
#ifdef _DEBUG
    else
    {
        HX_ASSERT(!"make sure TransportChannelMap has been set up right...");
    }
#endif

    return rc;
}

/**
 * \brief RegisterWithSharedUDPPort - disabled API!
 *
 * \todo fix and re-enable or remove
 *
 */
BOOL
RTSPServerProtocol::Session::RegisterWithSharedUDPPort(IHXSocket* pUDPSocket)
{
    BOOL rc = FALSE;

#if NOTYET
    //XXXTDM: Disabled for now, API needs updated
    IHXSharedUDPServices* pUDPServices = 0;
    if (HXR_OK == pUDPSocket->QueryInterface(IID_IHXSharedUDPServices,
                                             (void**)&pUDPServices))
    {
        if (HXR_OK == pUDPServices->RegisterSharedResponse(this, 0))
        {
            pUDPServices->GetSharedAddr(&m_pLocalAddr);
            rc = TRUE;
        }
        pUDPServices->Release();
        pUDPServices = 0;
    }
#endif

    return rc;
}


STDMETHODIMP_(UINT32)
RTSPServerProtocol::Session::IsThreadSafe()
{
    return m_ulThreadSafeFlags;
}

HX_RESULT
RTSPServerProtocol::Session::SetSDPSessionGUID(IHXStreamDescription* pSD,
                                               IHXValues** ppHeaderValues)
{
    if (!pSD)
    {
        return HXR_UNEXPECTED;
    }

    HX_RESULT rc = HXR_OK;
    char* pszQueryParams = NULL;
    char* pszSessionGUID = NULL;

    // Get the GUID from the query params
    char* pszURL = m_describeURL.GetBuffer(m_describeURL.GetLength());
    pszQueryParams = strchr((char*)pszURL, '?');

    if (!pszQueryParams)
    {
        return rc;
    }

    ++pszQueryParams;
    MIMEInputStream inputStream(pszQueryParams);
    MIMEScanner inputScanner(inputStream);
    char* szTokenSeparator = (char*)"&";
    MIMEToken paramField = inputScanner.nextToken(szTokenSeparator);
    const char* szParam = NULL;
    const char* zpszGUIDParamName = "guid";
    CHXString sGUIDParameter;

    while(paramField.hasValue())
    {
        sGUIDParameter = paramField.value();
        szParam = sGUIDParameter.GetBuffer(sGUIDParameter.GetLength());

        if (strncmp(szParam, zpszGUIDParamName, strlen(zpszGUIDParamName)) == 0)
        {
            pszSessionGUID = (char*)strchr(szParam, '=');
            if (pszSessionGUID)
            {
                ++pszSessionGUID;
            }
            break;
        }

        paramField = inputScanner.nextToken(szTokenSeparator);
    }

    if (!pszSessionGUID)
    {
        return rc;
    }

    const char* pszGUIDPropName = "SessionGUID";
    IHXStreamDescriptionSettings* pSDS = NULL;
    rc = pSD->QueryInterface(IID_IHXStreamDescriptionSettings, (void**)&pSDS);

    if (HXR_OK == rc)
    {
        IHXBuffer *pOptionBuff = NULL;
        CHXBuffer::FromCharArray("1", &pOptionBuff);
        pSDS->SetOption(pszGUIDPropName, pOptionBuff);
        HX_RELEASE(pOptionBuff);
    }
    HX_RELEASE(pSDS);

    IHXValues* pOptionalValues = NULL;
    if (ppHeaderValues &&
        (pOptionalValues = ppHeaderValues[1]))
    {
        IHXBuffer *pGUIDBuff = NULL;
        CHXBuffer::FromCharArray(pszSessionGUID, &pGUIDBuff);
        pOptionalValues->SetPropertyCString(pszGUIDPropName, pGUIDBuff);
        HX_RELEASE(pGUIDBuff);
    }
    else
    {
        rc = HXR_UNEXPECTED;
    }

    return rc;
}

HX_RESULT
RTSPServerProtocol::Session::sendBWProbingPackets(/* IN */ UINT32 ulCount, /* IN */ UINT32 ulSize, /* OUT */ REF(INT32) lSeqNo)
{
    HX_RESULT theErr = HXR_FAIL;

    Transport* pTrans = (Transport*)m_pTransportList->GetHead();
    if (pTrans)
    {
        theErr  = pTrans->sendBWProbingPackets(ulCount, ulSize, lSeqNo);
    }

    //If we fail to send ABD packets send  SET_PARAM
    //with the seq_no of last packet successfully sent.
    if (FAILED(theErr))
    {
         char pVal[32];
         sprintf (pVal, "%ld", lSeqNo);
         m_pServProt->SendSetParameterRequest(m_sessionID, NULL, "LastSeqNo", 
			                        (const char*)pVal, NULL, NULL);
         return HXR_FAIL;
    }
    return HXR_OK;
}

/**
 * \class RTSPServerSessionManager
 * \brief Provides a mapping from URL to Session ID for a given RTSPServerProtocol object.
 *
 * RTSPServerSessionManager is a remnant from the days when encoder connections
 * via the legacy encoplin plugin were allowed to disconnect without tearing
 * the session down. The class used to be a singleton that mapped ALL sessions 
 * on a given streamer. With encoplin support no longer required it is now 
 * created by the RTSPServerProtocol class. In other words, it now tracks 
 * sessions on a single RTSP control channel.
 *
 * \par
 * This class should be eliminated and a single URL->session map maintained in the
 * RTSPServerProtocol class.
 */

RTSPServerSessionManager::RTSPServerSessionManager()
    : m_ulRefCount(0)
{
    m_sessionMap.InitHashTable(257);
    m_sessionIDHelperMap.InitHashTable(257);
}

RTSPServerSessionManager::~RTSPServerSessionManager()
{
    CHXMapStringToOb::Iterator i;

    for (i = m_sessionMap.Begin(); i != m_sessionMap.End(); ++i)
    {
        RTSPSessionItem* pItem = (RTSPSessionItem*)(*i);
        delete pItem;
    }

    m_sessionMap.RemoveAll();
    m_sessionIDHelperMap.RemoveAll();
}

/**
 * \brief addSessionInstance - establish a mapping between the URL and session ID prameters.
 * 
 * Establish a mapping between the URL and session ID parameters. For legacy
 * reasons we currently have an intermediate mapping from URL -> RTSPSessionItem.
 * The "item" contains things like seqno and is mostly obsolete. Before it can
 * be eliminated, we need to understand the use of the m_bSetup and eliminate
 * the need for it with a better design.
 *
 * \param  pSessionID [in]  Session ID string.
 * \param  pURL       [in]  URL for this session.
 * \param  ulSeqNo    [in]  obsolete - can be eliminated.
 * \param  pProt      [in]  obsolete - can be eliminated.
 * \return string pointing to session id, ignored
 */
const char*
RTSPServerSessionManager::addSessionInstance(const char* pSessionID,
    const char* pURL, UINT32 ulSeqNo, RTSPServerProtocol* pProt)
{
    RTSPSessionItem* pItem = new RTSPSessionItem;
    if (!pItem)
        return 0;
    pItem->m_sessionID = pSessionID;
    pItem->m_URL = pURL;
    pItem->m_seqNo = ulSeqNo;
    pItem->m_pProt = pProt;
    pItem->m_bSetup = FALSE;

    /* Fix memory leak if this session instance was already added. This change
     * does not affect the functional behavior of this method except to
     * fix the leak. An easy way to repro the leak is to establish a session
     * id on the OPTIONS request and then send two DESCRIBES with that session
     * id */
    RTSPSessionItem* pOldItem = findInstance(pSessionID);

    if (pOldItem)
    {
        m_sessionMap.RemoveKey(pSessionID);
        NEW_FAST_TEMP_STR(pTemp, 8192, strlen(pOldItem->m_URL) + 2*sizeof(int*) + 5);
        sprintf (pTemp, "%p__%s", pOldItem->m_pProt, (const char *)pOldItem->m_URL);
        m_sessionIDHelperMap.RemoveKey(pTemp);
        DELETE_FAST_TEMP_STR(pTemp);

        delete pOldItem;
    }

    m_sessionMap[pSessionID] = pItem;
    NEW_FAST_TEMP_STR(pTemp, 8192, strlen(pURL) + 2*sizeof(int*) + 5);
    sprintf (pTemp, "%p__%s", pProt, pURL);
    m_sessionIDHelperMap[pTemp] = pItem;
    DELETE_FAST_TEMP_STR(pTemp);

    return pItem->m_sessionID;
}

/**
 * \brief getSessionID - return the session ID associated with the given URL.
 *
 * The URL parsing logic in this method should be part of an URL parsing class.
 * The usage of the bNotSetup parameter and the RTSPSessionItem::m_bSetup member
 * variable is unclear and should probably be designed out in the future (or at 
 * least documented)
 *
 * \param  pURL       [in] 
 * \param  pProt      [in]
 * \param  bNotSetup  [in] Set to TRUE if req is DESCRIBE or SETUP. Design intent unclear.
 * \param  pSessionID [out] Session ID found (CHXString).
 * \return HXR_OK if found, HXR_FAIL otherwise
 */
HX_RESULT
RTSPServerSessionManager::getSessionID(IHXBuffer* pURL,
    RTSPServerProtocol* pProt,
    BOOL bNotSetup,
    CHXString& sessionID)
{
    HX_RESULT rc = HXR_FAIL;

    // find the query params before looking for /streamid
    char* pURLStr = (char*)pURL->GetBuffer();
    char* pQueryParams = strchr(pURLStr, '?');
    UINT32 ulURLSize;
    UINT32 ulQuerySize;
    if(pQueryParams)
    {
        ulURLSize = pQueryParams - pURLStr;
        ulQuerySize = pURL->GetSize() - ulURLSize;
    }
    else
    {
        ulURLSize = pURL->GetSize();
        ulQuerySize = 0;
    }

    NEW_FAST_TEMP_STR(pDecURL, 1024, pURL->GetSize()+1);
    DecodeURL((const BYTE*)pURLStr, ulURLSize, pDecURL);

    char* pDecQuery = NULL;
    char* pStream = strrchr(pDecURL, '/');
    BOOL bFoundStreamId = FALSE;

    if (pStream && IsStreamId(pStream+1))
    {
        // strip this off the url
        *pStream = 0;
        pDecQuery = pStream;
        bFoundStreamId = TRUE;
    }

    // unescape query params over the /streamid or at the end.
    // we already made sure we have enough space
    // XXXJDG this shouldn't really happen here. url parsing is a
    // serious mess.
    if (pQueryParams)
    {
        if(!pDecQuery)
        {
            pDecQuery = pDecURL + strlen(pDecURL);
        }
        DecodeURL((const BYTE*)pQueryParams, ulQuerySize, pDecQuery);

        // If we didn't find the streamid before the query params,
        // look for it after
        if(!bFoundStreamId)
        {
            pStream = strrchr(pDecQuery, '/');
            if (pStream && IsStreamId(pStream+1))
            {
                *pStream = '\0';
                bFoundStreamId = TRUE;
            }
        }
    }

    NEW_FAST_TEMP_STR(pTemp, 1024, strlen(pDecURL) + 2*sizeof(int*) + 5);
    sprintf (pTemp, "%p__%s", pProt, pDecURL);

    RTSPSessionItem* pItem;
    if (m_sessionIDHelperMap.Lookup(pTemp, (void*&)pItem))
    {
        RTSPSessionItem* pItem2;
        m_sessionMap.Lookup(pItem->m_sessionID, (void*&)pItem2);
        if ((bNotSetup && (!pItem->m_bSetup)) || (!bNotSetup))
        {
            sessionID = pItem->m_sessionID;
            rc = HXR_OK;
        }
    }

    DELETE_FAST_TEMP_STR(pTemp);
    DELETE_FAST_TEMP_STR(pDecURL);

    return rc;
}


/**
 * \brief removeSessionInstance - remove this session ID from our mappings.
 *
 * \param  pSessionID [in] Session ID to remove (string).
 * \return HXR_OK if found and removed, HXR_FAIL otherwise
 */
HX_RESULT
RTSPServerSessionManager::removeSessionInstance(const char* pSessionID)
{
    RTSPSessionItem* pItem = 0;

    if (m_sessionMap.Lookup(pSessionID, (void*&)pItem))
    {
        m_sessionMap.RemoveKey(pSessionID);
        NEW_FAST_TEMP_STR(pTemp, 8192, strlen(pItem->m_URL) + 2*sizeof(int*) + 5);
        sprintf (pTemp, "%p__%s", pItem->m_pProt, (const char *)pItem->m_URL);
        m_sessionIDHelperMap.RemoveKey(pTemp);
        DELETE_FAST_TEMP_STR(pTemp);

        delete pItem;

        return HXR_OK;
    }

    return HXR_FAIL;
}

/**
 * \brief removeSessionInstances - remove all session IDs from our mappings.
 *
 * \param  pProt [in] was needed when this was a singleton, not needed anymore
 * \return HXR_OK
 */
HX_RESULT
RTSPServerSessionManager::removeSessionInstances(RTSPServerProtocol* pProt)
{
    CHXMapStringToOb::Iterator i;

    for (i = m_sessionMap.Begin(); i != m_sessionMap.End(); ++i)
    {
        RTSPSessionItem* pItem = (RTSPSessionItem*)(*i);

        if (pItem->m_pProt == pProt)
        {
            m_sessionMap.RemoveKey((const char*)pItem->m_sessionID);
            NEW_FAST_TEMP_STR(pTemp, 8192, strlen(pItem->m_URL) +
                2*sizeof(int*) + 5);
            sprintf (pTemp, "%p__%s", pItem->m_pProt,
                (const char *)pItem->m_URL);
            m_sessionIDHelperMap.RemoveKey(pTemp);
            DELETE_FAST_TEMP_STR(pTemp);
            delete pItem;
        }
    }
    return HXR_OK;
}

/**
 * \brief findInstance - find the RTSPSessionItem associated with this session ID
 *
 * \param  pSessionID [in] 
 * \return RTSPSessionItem* if found, 0 otherwise
 */
RTSPSessionItem*
RTSPServerSessionManager::findInstance(const char* pSessionID)
{
    RTSPSessionItem* pItem = 0;

    if (m_sessionMap.Lookup(pSessionID, (void*&)pItem))
    {
        return pItem;
    }

    return 0;
}


STDMETHODIMP
RTSPServerSessionManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXTCPResponse))
    {
        AddRef();
        *ppvObj = (IHXTCPResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
        AddRef();
        *ppvObj = (IHXThreadSafeMethods*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
RTSPServerSessionManager::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RTSPServerSessionManager::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


/**
 * \class RTSPServerProtocol::KillRTSPServerProtocolCallback
 *
 * This class handles teardown of an RTSP control connection and release of all
 * associated resources. We do it off of a timer to avoid problems when the 
 * stack unwinds.
 *
 * \param n/a
 *
 * \return 
 */
RTSPServerProtocol::KillRTSPServerProtocolCallback::KillRTSPServerProtocolCallback(
                                            RTSPServerProtocol* pProt)
: m_pProtocol(pProt)
, m_ulRefCount(0)
{
    m_pProtocol->AddRef();
}

RTSPServerProtocol::KillRTSPServerProtocolCallback::~KillRTSPServerProtocolCallback()
{
    if (m_pProtocol)
    {
        m_pProtocol->Release();
        m_pProtocol = 0;
    }
}

STDMETHODIMP
RTSPServerProtocol::KillRTSPServerProtocolCallback::Func()
{
    if (m_pProtocol)
    {
        m_pProtocol->TrueDone();
        m_pProtocol->Release();
        m_pProtocol = 0;
    }
    return HXR_OK;
}


STDMETHODIMP
RTSPServerProtocol::KillRTSPServerProtocolCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
RTSPServerProtocol::KillRTSPServerProtocolCallback::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RTSPServerProtocol::KillRTSPServerProtocolCallback::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

/**
 * \class RTSPServerProtocol::KeepAliveCallback
 *
 * This timer is responsible for tearing down unresponsive sessions and control
 * channels.
 *
 * \param n/a
 *
 * \return 
 */
RTSPServerProtocol::KeepAliveCallback::KeepAliveCallback(
    RTSPServerProtocol* pOwner) : m_ulRefCount(0), m_pOwner(pOwner)
{
    if (m_pOwner)
    {
        m_pOwner->AddRef();
    }
}

RTSPServerProtocol::KeepAliveCallback::~KeepAliveCallback()
{
    HX_RELEASE(m_pOwner);
}

STDMETHODIMP
RTSPServerProtocol::KeepAliveCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
RTSPServerProtocol::KeepAliveCallback::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
RTSPServerProtocol::KeepAliveCallback::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

// The keepalive callback is meant to enforce keepalive checking
// when no RTSP or backchannel data come in during the specified interval.

STDMETHODIMP
RTSPServerProtocol::KeepAliveCallback::Func()
{
    m_pOwner->KeepAliveCheck(this);
    return HXR_OK;
}


/**
 * \brief handleInput - process data received on the control channel socket
 *
 * handleInput takes a buffer with data received on the control channel socket,
 * peeks at it and creates the appropriate consumer for this kind of packet
 * (either RTSP request, response or interleaved data). When the consumer
 * has parsed a complete message it is dispatched to the state machine.
 *
 * \param pBuf [in]
 *
 * \return HXR_OK unless parsing fails or other serious error condition
 */
HX_RESULT
RTSPServerProtocol::handleInput(IHXBuffer* pBuf)
{
    // Update active timestamp for keepalive.
    // We have received an RTSP message of some kind.
    SetActiveStamp();

    if (m_pBufFrag == NULL)
    {
        // No existing buffer fragment so use the new one as-is
        if (pBuf == NULL)
        {
            // Hmm, no input at all
            HX_ASSERT(FALSE);
            return HXR_OK;
        }
        pBuf->AddRef();
        m_pBufFrag = pBuf;
    }
    else if (pBuf != NULL)
    {
        // We have an existing buffer fragment, so append the new one to it
        IHXBuffer* pBufNew = NULL;
        if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pBufNew))
        {
            return HXR_OUTOFMEMORY;
        }
        pBufNew->SetSize(m_pBufFrag->GetSize() + pBuf->GetSize());
        memcpy(pBufNew->GetBuffer(), m_pBufFrag->GetBuffer(),
            m_pBufFrag->GetSize());
        memcpy(pBufNew->GetBuffer() + m_pBufFrag->GetSize(),
            pBuf->GetBuffer(), pBuf->GetSize());
        HX_RELEASE(m_pBufFrag);
        m_pBufFrag = pBufNew;
    }

    // If we are still processing a message, let it finish
    if (m_pRespMsg != NULL)
    {
        return HXR_OK;
    }

    UINT32 pos = 0;
    UINT32 len = m_pBufFrag->GetSize();
    const char* p = (const char*)m_pBufFrag->GetBuffer();
    int res = RTSP_RES_AGAIN;
    while (res == RTSP_RES_AGAIN && pos < len)
    {
        // If we are starting a new message, the consumer will not exist.
        if (m_pConsumer == NULL)
        {
            // Determine the message type and create a consumer for it.
            if (p[pos] == '$')
            {
                CRTSPInterleavedPacket* pC =
                    new (m_pFastAlloc) CRTSPInterleavedPacket(m_pFastAlloc);
                if (HXR_OK != pC->QueryInterface(IID_IHXRTSPConsumer,
                    (void**)&m_pConsumer))
                {
                    delete pC;
                    HX_ASSERT(FALSE);
                    return HXR_UNEXPECTED;
                }
            }
            else
            {
                const char* peol;
                peol = (const char*)memchr(p+pos, '\n', len-pos);
                if (peol == NULL)
                {
                    // Indeterminate
                    res = RTSP_RES_PARTIAL;
                }
                if (strncasecmp(p+pos, "RTSP/", 5) == 0)
                {
                    CRTSPResponseMessage* pC =
                        new (m_pFastAlloc) CRTSPResponseMessage(m_pFastAlloc);
                    if (HXR_OK != pC->QueryInterface(IID_IHXRTSPConsumer,
                        (void**)&m_pConsumer))
                    {
                        HX_ASSERT(FALSE);
                        return HXR_UNEXPECTED;
                    }
                }
                else
                {
                    CRTSPRequestMessage* pC =
                        new (m_pFastAlloc) CRTSPRequestMessage(m_pFastAlloc);
                    if (HXR_OK != pC->QueryInterface(IID_IHXRTSPConsumer,
                        (void**)&m_pConsumer))
                    {
                        HX_ASSERT(FALSE);
                        return HXR_UNEXPECTED;
                    }
                }
            }
        }

        // Feed the packet to the consumer.
        res = m_pConsumer->ReadDone(m_pBufFrag, &pos);
        if (res == RTSP_RES_DONE)
        {
            DispatchMessage();
            HX_RELEASE(m_pConsumer);

            // If the message has been processed synchronously, keep going
            if (m_pRespMsg == NULL)
            {
                res = RTSP_RES_AGAIN;
            }
        }
    }

    if (res == RTSP_RES_INVALID)
    {
        return HXR_ABORT;
    }
    else if (pos == len)
    {
        // We used all of the data
        HX_RELEASE(m_pBufFrag);
    }
    else
    {
        UINT32 uRemain = m_pBufFrag->GetSize() - pos;

        // Check the max buffer size
        if (uRemain > RTSP_MAX_PENDING_BYTES)
        {
            return HXR_ABORT;
        }

        if (pos > 0)
        {
            // We used some of the data
            IHXBuffer* pbufNew;
            pbufNew = new CHXStaticBuffer(m_pBufFrag, pos, uRemain);

            pbufNew->AddRef();
            m_pBufFrag->Release();
            m_pBufFrag = pbufNew;
        }
    }
    // else we didn't use any data, leave m_pBufFrag alone

    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::handleStateError(RTSPServerProtocol::State state)
{
    IHXMIMEHeader* pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHdr->SetFromString("Allow", g_szAllowedMethods[state]);
    m_pRespMsg->AddHeader(pHdr);

    SendResponse(455);

    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::handleBadVersion(void)
{
    SendResponse(505);

    return HXR_OK;
}

/**
 * \brief handleCommonRequestHeaders - handle Require and Proxy-Require
 *
 * \param pReqMsg [in]
 *
 * \return HXR_OK if we support the  requested features
 */
HX_RESULT
RTSPServerProtocol::handleCommonRequestHeaders(IHXRTSPRequestMessage* pReqMsg)
{
    IHXMIMEHeader* pHeader = NULL;
    IHXRTSPMessage* pReqMsgBase = NULL;
    char* szUnsupported = NULL;
    IHXBuffer* pBufValue = NULL;
    HX_RESULT rc = HXR_OK;
    HX_RESULT rcRequire = HXR_OK;
    HX_RESULT rcProxyRequire = HXR_OK;
    IHXBuffer* pVerb = NULL;
    UINT32 uValueLengthRequire = 0;
    UINT32 uValueLengthProxyRequire = 0;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pReqMsgBase);
    HX_ASSERT(pReqMsgBase != NULL);
    pReqMsgBase->ReplaceDelimiters(TRUE, '\0');
    pReqMsg->GetVerbEx(pVerb);

    if (HXR_OK == pReqMsgBase->GetHeader("Require", pHeader))
    {
        HX_ASSERT(pHeader != NULL);
        if(!szUnsupported)
        {
            uValueLengthRequire = pHeader->GetSize();
            szUnsupported = new char[uValueLengthRequire + 1];
            *szUnsupported = '\0';
        }

        rcRequire = handleRequireHeaders(pHeader, pVerb, szUnsupported);
        HX_RELEASE(pHeader);
    }

    /* XXXTDM
     * Note the behavior of Proxy-Require is changing in RTSPbis.  The old
     * spec required servers to handle Proxy-Require as if it was a Require,
     * but the new spec says that Proxy-Require is only for proxies.
     *
     * Note also that RTSPbis has not changed the RTSP version.  We must
     * assume a client is an old 2326 client until it sends a request with
     * the header "Require: play.basic".  Bleah.
     */
    if (HXR_OK == pReqMsgBase->GetHeader("Proxy-Require", pHeader))
    {
        HX_ASSERT(pHeader != NULL);
        uValueLengthRequire = pHeader->GetSize();
        if (szUnsupported == NULL)
        {
            szUnsupported = new char[uValueLengthProxyRequire + 1];
            *szUnsupported = '\0';
        }
        else
        {
            //this is rare: client sends both headers
            char * szTemp = new char[uValueLengthProxyRequire +
                        uValueLengthRequire + 1];
            memcpy(szTemp, szUnsupported, uValueLengthRequire+1);
            HX_VECTOR_DELETE(szUnsupported);
            szUnsupported = szTemp;
        }

        rcProxyRequire = handleRequireHeaders(pHeader, pVerb, szUnsupported);
        HX_RELEASE(pHeader);
    }

    if (FAILED(rcRequire) || FAILED(rcProxyRequire))
    {
        pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pHeader->SetFromString("Unsupported", szUnsupported);
        m_pRespMsg->AddHeader(pHeader);

        SendResponse(551);

        rc = HXR_FAIL;
        goto hCRH_End;
    }

    if (HXR_OK == pReqMsgBase->GetHeader("Supported", pHeader))
    {
        HX_ASSERT(pHeader != NULL);

        HX_RELEASE(pHeader);

        BOOL bAllowABD = FALSE;

        //ABD AutoBandwidthDetection.
        //check if  ABD is enabled in the config file
        if (m_bSendBWPackets)
        {
            //we are treating the packet aggregation level as 
            // being representative of the loadstate of the server.
            INT32 nLoadState = 0;
            if (HXR_OK == m_pRegistry->GetIntByName("server.PacketAggLevel", nLoadState))
            {
                if (nLoadState < 3)
                {
                    bAllowABD = TRUE;
                }
            }
        }

        char* szSupported = NULL;
        const char szAlwaysSupported[] = "aggregate-transport";
        const char szABD[] = ", ABD-1.0";
        const char szPN3[] = ", method.playnow";
        int len = strlen(szAlwaysSupported);
             
        if (m_bPN3Enabled)
        {
            len += strlen(szPN3);
        }

        if (bAllowABD)
        {
            len += strlen(szABD);
        }

        szSupported = new char[len + 1];
        strcpy(szSupported, szAlwaysSupported);

        if (m_bPN3Enabled)
        {
            strcat(szSupported, szPN3);
        }

        if (bAllowABD)
        {
            strcat(szSupported,  szABD);
        }

        pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pHeader->SetFromString("Supported", szSupported);
        m_pRespMsg->AddHeader(pHeader);
        HX_VECTOR_DELETE(szSupported);
    }

#if 0
    /* XXXTDM
     * This code is disabled for now.  The only server I could find (RTSP or
     * HTTP) which returns a 406 from Accept-Encoding is Apache/2.0, and
     * only when the 'identity' encoding is explicitly refused.  RTSP/1.0
     * has no such notion, as it references rfc2068.  We will revisit this
     * issue when the new RTSP spec is released.  It should reference
     * rfc2616.
     */
    if (HXR_OK == pReqMsgBase->GetHeader("Accept-Encoding", pHeader))
    {
        HX_ASSERT(pHeader != NULL);
        if (HXR_OK == pHeader->GetValueAsBuffer(pBufValue))
        {
            uValueLengthProxyRequire = pBufValue->GetSize();
            pBufValue->Release();
        }

        rc = handleAcceptEncodingHeader(pHeader, pVerb);
        pHeader->Release();
    }
#else
    rc = HXR_OK;
#endif

    if (FAILED(rc))
    {
        // It is not clear what header(s) to put in the response, if any.
        // The Unsupported header does not seem to be appropriate.

        SendResponse(406);

        goto hCRH_End;
    }

hCRH_End:

    HX_VECTOR_DELETE(szUnsupported);
    HX_RELEASE(pReqMsgBase);
    HX_RELEASE(pVerb);
    return rc;
}

HX_RESULT
RTSPServerProtocol::handleRequireHeaders(IHXMIMEHeader* pMIMEHeaderRequire,
            IHXBuffer* pVerb, char* szUnsupported)
{
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    IHXList* pListParam = NULL;
    IHXListIterator* pIterParam = NULL;
    IUnknown* pUnkParam = NULL;
    IHXMIMEParameter* pParam = NULL;
    HX_RESULT rc = HXR_OK;
    BOOL bOptionSupported = FALSE;
    UINT32 ulOptionIndex = 0;
    UINT32 ulMessageLength = 0;
    const char* pCharNextMessage = NULL;

    pMIMEHeaderRequire->GetFieldListConst(pListField);
    pIterField = pListField->Begin();

    while (pIterField->HasItem())
    {
        pUnkField = pIterField->GetItem();
        pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
        HX_RELEASE(pUnkField);
        pField->GetParamListConst(pListParam);
        pIterParam = pListParam->Begin();

        while (pIterParam->HasItem())
        {
            pBufAttr = 0;
            pBufVal = 0;
            pUnkParam = pIterParam->GetItem();
            pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                (void **)&pParam);
            HX_RELEASE(pUnkParam);
            pParam->Get(pBufAttr, pBufVal);
            HX_RELEASE(pParam);

            bOptionSupported = FALSE;

            // Check to see if we support all the options.
            for (ulOptionIndex = 0; ulOptionIndex <
                m_NumRTSPRequireOptions; ++ulOptionIndex)
            {
                // If this option is supported
                BYTE* pTmpVal = pBufVal ? pBufVal->GetBuffer()
                    : pBufAttr->GetBuffer();
                UINT32 ulTmpSize = pBufVal ? pBufVal->GetSize()
                    : pBufAttr->GetSize();
                if (!strncasecmp(
                    RTSPRequireOptionsTable[ulOptionIndex].pcharOption,
                    (const char *)pTmpVal, ulTmpSize))
                {
                    // Initialize
                    const char* pStart = 0;
                    pCharNextMessage = pStart = RTSPRequireOptionsTable[
                        ulOptionIndex].pcharMessagesSupporting;

                    ulMessageLength = 0;

                    // Check Supported messages
                    while (((pCharNextMessage+ulMessageLength)-pStart <
                        RTSPRequireOptionsTable[ulOptionIndex].ulMsgsSupLen)
                        && pCharNextMessage && *pCharNextMessage)
                    {
                        // Set to End of last Message
                        pCharNextMessage += ulMessageLength;
                        // Go past any leading characters
                        while (*pCharNextMessage &&
                               !isalnum(*pCharNextMessage))
                        {
                            ++pCharNextMessage;
                        }

                        // Find length of this message
                        ulMessageLength = 0;
                        while (pCharNextMessage[ulMessageLength] &&
                              isalnum(pCharNextMessage[ulMessageLength]))
                        {
                            ++ulMessageLength;
                        }

                        if (ulMessageLength &&
                            !strncmp(pCharNextMessage,
                            (const char *)pVerb->GetBuffer(),
                            ulMessageLength))
                        {
                            // OK! We finaly proved that this is a
                            // supported option on this type of Message!
                            bOptionSupported = TRUE;
                            break;
                        }
                    }
                }
            }

            if (!bOptionSupported)
            {
                // Add to Unsupported value..
                if (szUnsupported[0])
                {
                    strcat(szUnsupported, ",");
                }

                IHXBuffer* pTempBuf = pBufVal ? pBufVal : pBufAttr;
                strncat(szUnsupported, (char*)pTempBuf->GetBuffer(),
                            pTempBuf->GetSize());

                rc = HXR_FAIL;
            }
            HX_RELEASE(pBufAttr);
            HX_RELEASE(pBufVal);

            pIterParam->MoveNext();
        }
        HX_RELEASE(pIterParam);
        HX_RELEASE(pListParam);
        HX_RELEASE(pField);
        pIterField->MoveNext();
    }
    HX_RELEASE(pIterField);
    HX_RELEASE(pListField);

    return rc;
}

HX_RESULT
RTSPServerProtocol::handleAcceptEncodingHeader(IHXMIMEHeader* pAcceptEncodingHeader,
            IHXBuffer* pVerb)
{
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    IHXList* pListParam = NULL;
    IHXListIterator* pIterParam = NULL;
    IUnknown* pUnkParam = NULL;
    IHXMIMEParameter* pParam = NULL;
    HX_RESULT rc = HXR_FAIL;
    UINT32 ulOptionIndex = 0;
    UINT32 ulMessageLength = 0;
    const char* pCharNextMessage = NULL;

    pAcceptEncodingHeader->GetFieldListConst(pListField);
    pIterField = pListField->Begin();

    while (rc == HXR_FAIL && pIterField->HasItem())
    {
        pUnkField = pIterField->GetItem();
        pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
        HX_RELEASE(pUnkField);
        pField->GetParamListConst(pListParam);
        pIterParam = pListParam->Begin();

        while (rc == HXR_FAIL && pIterParam->HasItem())
        {
            pBufAttr = 0;
            pBufVal = 0;
            pUnkParam = pIterParam->GetItem();
            pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                (void **)&pParam);
            HX_RELEASE(pUnkParam);
            pParam->Get(pBufAttr, pBufVal);
            HX_RELEASE(pParam);

            // Check to see if we support any of the encodings.
            for (ulOptionIndex = 0; ulOptionIndex <
                m_NumRTSPAcceptEncodingOptions; ++ulOptionIndex)
            {
                // If this option is supported
                BYTE* pTmpVal = pBufVal ? pBufVal->GetBuffer()
                    : pBufAttr->GetBuffer();
                UINT32 ulTmpSize = pBufVal ? pBufVal->GetSize()
                    : pBufAttr->GetSize();
                if (!strncasecmp(
                    RTSPAcceptEncodingOptionsTable[ulOptionIndex].pcharOption,
                    (const char*)pTmpVal, ulTmpSize))
                {
                    // Initialize
                    const char* pStart = 0;
                    pCharNextMessage = pStart = RTSPAcceptEncodingOptionsTable[
                        ulOptionIndex].pcharMessagesSupporting;

                    ulMessageLength = 0;

                    // Check Supported messages
                    while (((pCharNextMessage+ulMessageLength)-pStart <
                        RTSPAcceptEncodingOptionsTable[ulOptionIndex].ulMsgsSupLen)
                        && pCharNextMessage && *pCharNextMessage)
                    {
                        // Set to End of last Message
                        pCharNextMessage += ulMessageLength;
                        // Go past any leading characters
                        while (*pCharNextMessage &&
                               !isalnum(*pCharNextMessage))
                        {
                            ++pCharNextMessage;
                        }

                        // Find length of this message
                        ulMessageLength = 0;
                        while (pCharNextMessage[ulMessageLength] &&
                              isalnum(pCharNextMessage[ulMessageLength]))
                        {
                            ++ulMessageLength;
                        }

                        if (ulMessageLength &&
                            !strncmp(pCharNextMessage,
                            (const char *)pVerb->GetBuffer(),
                            ulMessageLength))
                        {
                            // Yes, we support at least one encoding
                            rc = HXR_OK;
                            break;
                        }
                    }
                }
            }

            HX_RELEASE(pBufAttr);
            HX_RELEASE(pBufVal);

            pIterParam->MoveNext();
        }
        HX_RELEASE(pIterParam);
        HX_RELEASE(pListParam);
        HX_RELEASE(pField);
        pIterField->MoveNext();
    }
    HX_RELEASE(pIterField);
    HX_RELEASE(pListField);

    return rc;
}

HX_RESULT
RTSPServerProtocol::handleChallengeResponse(IHXRTSPMessage* pMsg)
{
    BOOL bSuccess = FALSE;
    IHXMIMEHeader* pMIMEHeaderChallenge = NULL;
    IHXMIMEHeader* pRARTPHeaderChallenge = NULL;
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    IHXMIMEParameter* pParam = NULL;

    pMsg->AddRef();

    if (m_bIsLocalBoundSocket)
    {
        // it's a local bound socket
        m_bChallengeMet = TRUE;
        m_bSendClientRealChallenge3 = FALSE;
        m_bChallengeDone = TRUE;
        HX_RELEASE(pMsg);
        return HXR_OK;
    }

    if (m_bChallengeDone)
    {
        if (HXR_OK == pMsg->GetHeader("RealChallenge2", pMIMEHeaderChallenge))
        {
            m_bSendClientRealChallenge3 = TRUE;
        }
        HX_RELEASE(pMIMEHeaderChallenge);
        HX_RELEASE(pMsg);
        return HXR_OK;
    }

    // Verify the challenge response
#if defined(HELIX_FEATURE_RTSP_SERVER_CHALLENGE)
    if (HXR_OK == pMsg->GetHeader("RealChallenge2", pMIMEHeaderChallenge) &&
        m_pRealChallenge)
    {
        pMIMEHeaderChallenge->GetFieldListConst(pListField);
        pIterField = pListField->Begin();
        HX_RELEASE(pListField);

        if (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetFirstParam(pParam);
            HX_RELEASE(pField);

            char* pClientResponse = NULL;
            const char* pSD = NULL;

            // Retrieve the challenge response

            if (pParam)
            {
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);
                pClientResponse = new char[pBufAttr->GetSize()+1];
                memcpy(pClientResponse,
                    (const char *)pBufAttr->GetBuffer(),
                    pBufAttr->GetSize());
                *(pClientResponse+pBufAttr->GetSize()) = '\0';
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);

                pIterField->MoveNext();

                if (pIterField->HasItem())
                {
                    pUnkField = pIterField->GetItem();
                    pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
                    HX_RELEASE(pUnkField);
                    pField->GetFirstParam(pParam);
                    HX_RELEASE(pField);

                    // Retrieve the challenge response
                    if (pParam)
                    {
                        pParam->Get(pBufAttr, pBufVal);
                        HX_RELEASE(pParam);
                        if (pBufVal)
                        {
                            pSD = (const char*)pBufVal->GetBuffer();
                        }
                        HX_RELEASE(pBufAttr);
                        HX_RELEASE(pBufVal);
                    }
                }

                // Calculate the proper response
                m_pRealChallenge->response1((BYTE*)m_pRealChallenge->challenge);
                if (!strcmp((char*)m_pRealChallenge->response, pClientResponse))
                {
                    // Validation successful!!!
                    m_bChallengeMet = TRUE;
                    m_bSendClientRealChallenge3 = TRUE;
                    bSuccess = TRUE;

                    // Verify the sd and set the trap variable
                    if (pSD && !strcmp((char*)m_pRealChallenge->trap, pSD))
                    {
                        m_bIsValidChallengeT = TRUE;
                    }

                    // Generate the server challenge response to be sent
                    // in the SETUP RESPONSE message, along with the sdr
                    m_pRealChallenge->response2(m_pRealChallenge->response);
                }
                HX_DELETE(pClientResponse);
            }
        }
        HX_RELEASE(pIterField);

        // Remember that we are done with the challenge so
        // we don't challenge on subsequent SETUP exchanges
        m_bChallengeDone = TRUE;
    }
    HX_RELEASE(pMIMEHeaderChallenge);

    /*
     * Validate RARTPChallenge
     */
    if (HXR_OK == pMsg->GetHeader("RARTPChallenge", pRARTPHeaderChallenge) &&
        m_pRealChallenge)
    {
        char rartpResponse[RARTP_RESPONSE_LEN + 1];
        rartpResponse[RARTP_RESPONSE_LEN] = 0;

        pRARTPHeaderChallenge->GetFirstField(pField);

        if (pField)
        {
            pField->GetFirstParam(pParam);
            HX_RELEASE(pField);

            char* pClientResponse = NULL;

            if (pParam)
            {
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);
                pClientResponse = new char[pBufAttr->GetSize()+1];
                memcpy(pClientResponse,
                    (const char *)pBufAttr->GetBuffer(),
                    pBufAttr->GetSize());
                *(pClientResponse+pBufAttr->GetSize()) = '\0';
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);

                RARTPResponse(rartpResponse, (char*)m_pRealChallenge->challenge);
                if (!strcmp(rartpResponse, pClientResponse))
                {
                    m_bRARTPChallengeMet = TRUE;
                }
            }
            HX_DELETE(pClientResponse);
        }
    }
    HX_RELEASE(pRARTPHeaderChallenge);
#endif /*HELIX_FEATURE_RTSP_SERVER_CHALLENGE*/

    if (m_bChallengeDone && !bSuccess)
    {
        // If the client did not send a valid challenge response,
        // delete the Challenge object so we do not send our response
        // (even if m_bChallengeMet == TRUE due to the current date)
        HX_DELETE(m_pRealChallenge);
    }

    HX_RELEASE(pMsg);
    return HXR_OK;
}

HX_RESULT
RTSPServerProtocol::ValidateChallengeResp(RTSPServerProtocol::Session* pSession,
                                          CHXString& sessionID,
                                          BOOL bMidBoxChallengeHdr,
                                          IHXRTSPMessage* pReqMsg)
{
    HX_RESULT rc = HXR_FAIL;

    /* Handle midbox challenge */
    if (pSession && pSession->m_bIsMidBox)
    {
        if (pSession->m_pMidBoxChallenge && pSession->m_bChallengeMet)
        {
            rc = HXR_OK;
        }
        else
        {
            if (pReqMsg)
            {
                pSession->handleChallengeResponse(pReqMsg);
            }

            if (pSession->m_bChallengeMet)
            {
                rc = HXR_OK;
            }
            else
            {
                // challenge response failed, revoke status
                m_pResp->SetMidBox(sessionID, FALSE);
            }
        }
    }
    /* Handle non-midbox challenge */
    else if (!bMidBoxChallengeHdr)
    {
        if (m_pRealChallenge && m_bChallengeMet && !m_bRARTPChallengePending)
        {
            rc = HXR_OK;
        }
        else
        {
            if (pReqMsg)
            {
                handleChallengeResponse(pReqMsg);
            }

            if (m_bChallengeMet &&
                (!m_bRARTPChallengePending || m_bRARTPChallengeMet))
            {
                rc = HXR_OK;
                m_bRARTPChallengePending = FALSE;
            }
        }
    }

    return rc;
}

HX_RESULT
RTSPServerProtocol::SetChallengeHeader(RTSPServerProtocol::Session* pSession,
                                       UINT32 ulLastReqMethod,
                                       BOOL bMidBoxChallengeHdr,
                                       IHXRTSPMessage* pRespMsg)
{
    HX_RESULT rc = HXR_OK;
    const UINT32 ulMaxHeaderBuffSize = 256;
    char szChallenge3Buff[ulMaxHeaderBuffSize];
    char* pszChallenge3 = szChallenge3Buff;
    char* pszChallenge1 = 0;
    szChallenge3Buff[0] = '\0';

    rc = PrepareChallengeHeader(pSession, ulLastReqMethod, pszChallenge1,
                                pszChallenge3, bMidBoxChallengeHdr,
                                ulMaxHeaderBuffSize);

    if ( rc == HXR_OK )
    {
        CMIMEHeader* pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        if (pszChallenge1)
        {
            pMIMEHeader->SetFromString("RealChallenge1", pszChallenge1);
        }
        else if (*pszChallenge3)
        {
            pMIMEHeader->SetFromString("RealChallenge3", pszChallenge3);
        }
        pRespMsg->AddHeader(pMIMEHeader);
    }

    return rc;
}

HX_RESULT
RTSPServerProtocol::SetChallengeHeader(RTSPServerProtocol::Session* pSession,
                                       UINT32 ulLastReqMethod,
                                       BOOL bMidBoxChallengeHdr,
                                       RTSPResponseMessage* pRespMsg)
{
    HX_RESULT rc = HXR_OK;
    const UINT32 ulMaxHeaderBuffSize = 256;
    char szChallenge3Buff[ulMaxHeaderBuffSize];
    char* pszChallenge3 = szChallenge3Buff;
    char* pszChallenge1 = 0;
    szChallenge3Buff[0] = '\0';

    rc = PrepareChallengeHeader(pSession, ulLastReqMethod, pszChallenge1,
                                pszChallenge3, bMidBoxChallengeHdr,
                                ulMaxHeaderBuffSize);

    if ( rc == HXR_OK )
    {
        if (pszChallenge1)
        {
            pRespMsg->addHeader("RealChallenge1", pszChallenge1);
        }
        else if (*pszChallenge3)
        {
            pRespMsg->addHeader("RealChallenge3", pszChallenge3);
        }
    }

    return rc;
}

HX_RESULT
RTSPServerProtocol::PrepareChallengeHeader(RTSPServerProtocol::Session* pSession,
                                           UINT32 ulLastReqMethod,
                                           char*& pszChallenge1,
                                           char*& pszChallenge3,
                                           BOOL bMidBoxChallengeHdr,
                                           UINT32 ulMaxBuffSize)
{
    HX_RESULT rc = HXR_FAIL;
    BOOL bSetupPending = (ulLastReqMethod ==  RTSP_VERB_SETUP);
    BOOL bIsFirstSetup = (bSetupPending && pSession->m_sSetupCount == 1);

    /* Handle midbox challenge */
    if (pSession && pSession->m_bIsMidBox)
    {
#if defined(HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE)
        if (ulLastReqMethod == RTSP_VERB_OPTIONS ||
            (bSetupPending && !pSession->m_pMidBoxChallenge))
        {
            /* Allow RealChallenge initiation in OPTIONS or SETUP */
            if (!pSession->m_pMidBoxChallenge)
            {
                pSession->m_pMidBoxChallenge = new MidBoxChallenge();
            }

            m_ulChallengeInitMethod = ulLastReqMethod;
            pszChallenge1 = (char*)pSession->m_pMidBoxChallenge->challenge;
            rc = HXR_OK;
        }
        else if (pSession->m_bChallengeMet &&
                 pSession->m_pMidBoxChallenge &&
                 pSession->m_bSendClientRealChallenge3 &&
                 (bIsFirstSetup || !bSetupPending) )

        {
            /* Send RealChallenge response */
            snprintf(pszChallenge3, ulMaxBuffSize,
                    "%-.128s,sdr=%-.128s",
                    (char*)pSession->m_pMidBoxChallenge->response,
                    (char*)pSession->m_pMidBoxChallenge->trap);
            pSession->m_bSendClientRealChallenge3 = FALSE;
            rc = HXR_OK;
        }
#endif //HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE
    }

    /* Handle non-midbox challenge */
    else if (!bMidBoxChallengeHdr)
    {
#if defined(HELIX_FEATURE_RTSP_SERVER_CHALLENGE)
        if ((ulLastReqMethod == RTSP_VERB_OPTIONS
             || (bSetupPending && !m_pRealChallenge)))
        {
            /* Allow RealChallenge initiation in OPTIONS or SETUP */
            if (!m_pRealChallenge)
            {
                m_pRealChallenge = new RealChallenge();
            }

            m_ulChallengeInitMethod = ulLastReqMethod;
            pszChallenge1 = (char*)m_pRealChallenge->challenge;
            rc = HXR_OK;
        }
        else if (m_bChallengeMet && m_pRealChallenge &&
                 m_bSendClientRealChallenge3 &&
                 (bIsFirstSetup || !bSetupPending) )
        {
            /* Send RealChallenge response */
            snprintf(pszChallenge3, ulMaxBuffSize,
                     "%-.128s,sdr=%-.128s",
                     (char*)m_pRealChallenge->response,
                     (char*)m_pRealChallenge->trap );
            m_bSendClientRealChallenge3 = FALSE;
            rc = HXR_OK;
        }
#endif //HELIX_FEATURE_RTSP_SERVER_CHALLENGE
    }
    return rc;
}

HX_RESULT
RTSPServerProtocol::parseRange(IHXBuffer* pAttr, IHXBuffer* pVal,
                               REF(UINT64) tBegin, REF(UINT64) tEnd,
                               REF(RTSPRange::RangeType) rangeUnits)
{
    if ((pVal == NULL) || (pAttr == NULL))
    {
        return HXR_UNEXPECTED;
    }

    const char* pValBegin = (const char *)pVal->GetBuffer();
    const char* pValEnd = pValBegin + pVal->GetSize();
    int len = pValEnd - pValBegin;
    const char* pPtr = 0;
    char pTime[64];

    tBegin = tEnd = (UINT64)-1;

    // check for overflow
    if (len >= 64)
    {
        return HXR_UNEXPECTED;
    }

    if (!strncasecmp("npt", (const char *)pAttr->GetBuffer(), pAttr->GetSize()))
    {
        pPtr = StrNChr(pValBegin, '-', len);
        rangeUnits = RTSPRange::TR_NPT;

        if (pPtr)
        {
            if (pPtr != pValBegin)
            {
                memcpy(pTime, pValBegin, pPtr-pValBegin);
                pTime[pPtr-pValBegin] = '\0';
                NPTime tCode1(pTime);
                tBegin = UINT64(tCode1);
            }
            ++pPtr;
            if (pPtr < pValEnd)
            {
                memcpy(pTime, pPtr, pValEnd-pPtr);
                pTime[pValEnd-pPtr] = '\0';
                NPTime tCode1(pTime);
                tEnd = UINT64(tCode1);
            }

            return HXR_OK;
        }
    }
    else if (!strncasecmp("smpte", (const char *)pAttr->GetBuffer(),
        pAttr->GetSize()))
    {
        rangeUnits = RTSPRange::TR_SMPTE;
        pPtr = StrNChr(pValBegin, '-', pValEnd-pValBegin);
        if (pPtr)
        {
            memcpy(pTime, pValBegin, pPtr-pValBegin);
            pTime[pPtr-pValBegin] = '\0';
            SMPTETimeCode tCode1(pTime);
            tBegin = UINT64(tCode1);
            ++pPtr;

            if (pPtr < pValEnd)
            {
                memcpy(pTime, pPtr, pValEnd-pPtr);
                pTime[pValEnd-pPtr] = '\0';
                SMPTETimeCode tCode1(pTime);
                tEnd = UINT64(tCode1);
            }

            return HXR_OK;
        }
    }
    else if (!strncasecmp("bytes", (const char *)pAttr->GetBuffer(),
        pAttr->GetSize()))
    {
        pPtr = StrNChr(pValBegin, '-', len);

        if (pPtr)
        {
            memcpy(pTime, pValBegin, pPtr-pValBegin);
            pTime[pPtr-pValBegin] = '\0';
            tBegin = (UINT64)atoi64((char*)(const char*)pTime);
            ++pPtr;

            if (pPtr < pValEnd)
            {
                memcpy(pTime, pPtr, pValEnd-pPtr);
                pTime[pValEnd-pPtr] = '\0';
                tEnd = (UINT64)atoi64((char*)(const char*)pTime);
            }

            return HXR_OK;
        }
    }

    return HXR_UNEXPECTED;
}

/**
 * \brief setState - set the state of the session this request applies to.
 *
 * After a request is processed successfully, setState is called to check
 * if a state change is required. We have a multi-stage lookup process to
 * find the session given the request, look for Session or If-match headers,
 * then check to see if we have mapped the URL to a session. It might be better
 * if we changed to a model where we identified or created the session when the
 * request is first seen and then the session itself handled the requests and
 * associated state change. 
 *
 * \param pReqMsg [in] 
 * \param state [in] : the state to transition to (from the static state table)
 *
 * \return HXR_OK if the session is found and the state change allowed.
 */
HX_RESULT
RTSPServerProtocol::setState(IHXRTSPRequestMessage* pReqMsg,
    RTSPServerProtocol::State state)
{
    HX_RESULT rc = HXR_OK;
    CHXString sessionID;
    IHXRTSPMessage* pMsg = 0;
    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);

    HX_ASSERT(state <= NONE);   // Validate state variable
    if (state < NONE)           // NONE means don't change
    {
        RTSPServerProtocol::Session* pSession = NULL;
        if (pReqMsg->GetMethod() == RTSP_SETUP)
        {
            BOOL bHasSessionHeader;
            getSetupSession(pMsg, pSession, bHasSessionHeader);
        }
        else
        {
            if (HXR_OK == getSessionID(pMsg, sessionID))
            {
                pSession = getSession(sessionID);
            }
        }
        if (pSession == NULL)
        {
            IHXBuffer* pURL = NULL;
            pReqMsg->GetUrl(pURL);
            rc = m_pSessionManager->getSessionID(pURL, this, FALSE, sessionID);
            if (rc == HXR_OK)
            {
                pSession = getSession(sessionID);
            }
            HX_RELEASE(pURL);
        }
        if (pSession && pSession->m_state != KILLED)
        {
            pSession->m_state = state;
        }
        else
        {
            rc = HXR_FAIL;
        }
    }
    HX_RELEASE(pMsg);
    return rc;
}

/**
 * \brief setState - set the state of the session mapped to this session id.
 *
 * This variant of setState is called when the state of an RTSP session is
 * changed by something other than an RTSP request. There could be an error
 * or maybe playback has completed
 *
 * \param pSessionID [in] 
 * \param state [in] : the state to transition to (from the static state table)
 *
 * \return HXR_OK if the session is found and the state change allowed.
 */
HX_RESULT
RTSPServerProtocol::setState(const char* pSessionID,
    RTSPServerProtocol::State state)
{
    HX_RESULT rc = HXR_OK;

    HX_ASSERT(state <= NONE);   // Validate state variable
    if (state < NONE)           // NONE means don't change
    {
        RTSPServerProtocol::Session* pSession;
        pSession = getSession(pSessionID);
        if (pSession && pSession->m_state != KILLED)
        {
            pSession->m_state = state;
        }
    }

    return rc;
}

/**
 * \brief getState - get the state of the session this request applies to.
 *
 * getState goes through a multi-stage lookup process to find the session 
 * given the request (look for Session or If-match headers, check to see if
 * we have mapped the URL to a session).
 *
 * \param pReqMsg [in] 
 * \param state [out] : the state this session is in
 *
 * \return HXR_OK if a session is found
 */
HX_RESULT
RTSPServerProtocol::getState(IHXRTSPRequestMessage* pReqMsg,
    RTSPServerProtocol::State& state)
{
    IHXRTSPMessage* pMsg = NULL;
    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);

    HX_RESULT rc = HXR_OK;
    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;
    if (pReqMsg->GetMethod() == RTSP_SETUP)
    {
        BOOL bHasSessionHeader;
        getSetupSession(pMsg, pSession, bHasSessionHeader);
    }
    else
    {
        if (HXR_OK == getSessionID(pMsg, sessionID))
        {
            pSession = getSession(sessionID);
        }
    }
    if (pSession == NULL)
    {
        IHXBuffer* pURL = NULL;
        pReqMsg->GetUrl(pURL);
        if (pReqMsg->GetMethod() == RTSP_SETUP ||
           pReqMsg->GetMethod() == RTSP_DESCRIBE)
        {
            rc = m_pSessionManager->getSessionID(pURL, this,
                TRUE, sessionID);
        }
        else
        {
            rc = m_pSessionManager->getSessionID(pURL, this,
                FALSE, sessionID);
        }
        if (rc == HXR_OK)
        {
            pSession = getSession(sessionID);
        }
        HX_RELEASE(pURL);
    }

    if (pSession)
    {
        state = pSession->m_state;
        if (state >= NONE)
        {
            HX_ASSERT(FALSE);
            state = INIT;
        }
    }
    else
    {
        rc = HXR_FAIL;
    }

    HX_RELEASE(pMsg);
    return rc;
}

/**
 * \brief getState - get the state of the session mapped to this session id.
 *
 * \param pSessionID [in] 
 * \param state [out] : the state this session is in
 *
 * \return HXR_OK if a session is found
 */
HX_RESULT
RTSPServerProtocol::getState(const char* pSessionID,
    RTSPServerProtocol::State& state)
{
    HX_RESULT rc = HXR_OK;
    RTSPServerProtocol::Session* pSession = NULL;

    pSession = getSession(pSessionID);
    if (pSession)
    {
        state = pSession->m_state;
        if (state >= NONE)
        {
            HX_ASSERT(FALSE);
            state = INIT;
        }
    }
    else
    {
        rc = HXR_FAIL;
    }

    return rc;
}

/**
 * \brief getSessionID - get the session id from the requests "Session" header.
 *
 * \param pMsg [in]
 * \param sessionID [out]
 *
 * \return HXR_OK if the Session header is there and has a non-empty value
 */
HX_RESULT
RTSPServerProtocol::getSessionID(IHXRTSPMessage* pMsg, CHXString& sessionID)
{
    HX_RESULT rc = HXR_FAIL;

    IHXMIMEHeader* pHeader = NULL;

    pMsg->GetHeader("Session", pHeader);
    if (pHeader == NULL)
    {
        pMsg->GetHeader("Replace-Session", pHeader);
    }

    if (pHeader != NULL)
    {
        IHXBuffer* pBuf = NULL;
        pHeader->GetValueAsBuffer(pBuf);
        if (pBuf != NULL)
        {
            const char* pSessionId;
            UINT32 uSessionIdLen;
            const char* pSemi;
            pSessionId = (const char*)pBuf->GetBuffer();
            uSessionIdLen = pBuf->GetSize();
            if (uSessionIdLen > 0)
            {
                pSemi = (const char*)memchr(pSessionId, ';', uSessionIdLen);
                if (pSemi != NULL)
                {
                    uSessionIdLen = (pSemi-pSessionId);
                }
            }
            if (uSessionIdLen > 0)
            {
                //XXXTDM: need a CHXString::Set(const char*, UINT32)
                CHXString foo(pSessionId, (int)uSessionIdLen);
                sessionID = foo;
                rc = HXR_OK;
            }
            pBuf->Release();
        }
        pHeader->Release();
    }

    return rc;
}

/**
 * \brief getSetupSession - find the session that this SETUP request is for
 *
 * getSetupSession looks for a Session header, then for an If-match. If we are
 * successful in finding a session id we use it to look up the session.
 *
 * \param pMsg [in]
 * \param pSession [out]
 * \param rbHasSessionHeader [out] : we set this to TRUE if we find a Session header
 *
 * \return HXR_OK if we find the session
 */
HX_RESULT
RTSPServerProtocol::getSetupSession(IHXRTSPMessage* pMsg,
                                    RTSPServerProtocol::Session*& pSession,
                                    BOOL& rbHasSessionHeader)
{
    CHXString sessionID;

    HX_RESULT rc = getSessionID(pMsg, sessionID);
    if (HXR_OK == rc)
    {
        rbHasSessionHeader = TRUE;
        pSession = getSession(sessionID);
        return rc;
    }
    rbHasSessionHeader = FALSE;

    IHXMIMEHeader* pHeader = NULL;
    pMsg->GetHeader("If-Match", pHeader);
    if (pHeader != NULL)
    {
        IHXBuffer* pBuf = NULL;
        pHeader->GetValueAsBuffer(pBuf);
        if (pBuf != NULL)
        {
            const char* pSessionId;
            UINT32 uSessionIdLen;
            const char* pSemi;
            pSessionId = (const char*)pBuf->GetBuffer();
            uSessionIdLen = pBuf->GetSize();
            if (uSessionIdLen > 0)
            {
                pSemi = (const char*)memchr(pSessionId, ';', uSessionIdLen);
                if (pSemi != NULL)
                {
                    uSessionIdLen = (pSemi-pSessionId);
                }
            }
            if (uSessionIdLen > 0)
            {
                char* pszTmpSID = new_string(pSessionId, uSessionIdLen);
                pSession = getSession(pszTmpSID);
                if (pSession && pSession->m_bRetainEntityForSetup)
                {
                    rc = HXR_OK;
                }
                delete[] pszTmpSID;
            }
            pBuf->Release();
        }
        pHeader->Release();
    }

    return rc;
}

/**
 * \brief GetAndConvertRFC822Headers - get the message headers into an IHXValues
 *
 * \param pMsg [in]
 * \param pRFC822Headers [out]
 *
 * \return HXR_OK unless we have trouble allocating the list
 */
HX_RESULT
RTSPServerProtocol::GetAndConvertRFC822Headers(IHXRTSPMessage* pMsg,
    REF(IHXValues*) pRFC822Headers)
{
    IUnknown*           pUnknown = NULL;
    IHXKeyValueList*   pList = NULL;
    IHXMIMEHeader*     pMIMEHeader = NULL;
    IUnknown*           pUnkHdr = NULL;
    IHXList*           pListHdr = NULL;
    IHXListIterator*   pIterHdr = NULL;
    IHXBuffer*         pBufKey = NULL;
    IHXBuffer*         pBufVal = NULL;
    HX_RESULT           ret = HXR_OK;

    pRFC822Headers = NULL;

    if (!m_pCommonClassFactory)
    {
        ret = HXR_UNEXPECTED;
        goto cleanup;
    }

    if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXKeyValueList,
                                                        (void**) &pUnknown))
    {
        ret = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXKeyValueList,
                                           (void**) &pList))
    {
        ret = HXR_UNEXPECTED;
        goto cleanup;
    }

    // If a require header was found
    pMsg->GetHeaderListConst(pListHdr);
    pIterHdr = pListHdr->Begin();
    while (pIterHdr->HasItem())
    {
        pUnkHdr = pIterHdr->GetItem();
        pUnkHdr->QueryInterface(IID_IHXMIMEHeader, (void **)&pMIMEHeader);
        pMIMEHeader->GetKey(pBufKey);
        pMIMEHeader->GetValueAsBuffer(pBufVal);

        // XXX: aak -- hacky passing in just the buffer of an IHXBuffer
        // as the name in the IHXKeyeValueList, but the assumption is
        // that the request headers r around at least until the request
        // has been processed and replied to.
        pList->AddKeyValue((const char *)pBufKey->GetBuffer(), pBufVal);

        HX_RELEASE(pBufVal);
        HX_RELEASE(pBufKey);
        HX_RELEASE(pMIMEHeader);
        HX_RELEASE(pUnkHdr);

        pIterHdr->MoveNext();
    }
    HX_RELEASE(pListHdr);
    HX_RELEASE(pIterHdr);

    // XXX showell - Yet another item for hxvalues cleanup phase II.  We should
    // just change this function so its callers don't expect IHXValues, since
    // the IHXKeyValueList interface is better for header data.
    if (HXR_OK != pList->QueryInterface(IID_IHXValues,
                                        (void**) &pRFC822Headers))
    {
        pRFC822Headers = NULL;
    }
cleanup:

    HX_RELEASE(pList);
    HX_RELEASE(pUnknown);

    return ret;
}

/**
 * \brief ConvertAndAddRFC822Headers - add headers to an RTSP message
 *
 * Adds a list of headers to an outgoing RTSP message.
 *
 * \param pMsg [in/out] : RTSP message the new headers are to be appended to
 * \param pRFC822Headers [in] : list of headers in name/value pairs.
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPServerProtocol::ConvertAndAddRFC822Headers(IHXRTSPMessage* pMsg,
    IHXValues* pRFC822Headers)
{
    HX_RESULT         res;
    IHXMIMEHeader*    pMIMEHdr;
    const char*       pName = NULL;
    IHXBuffer*        pValue = NULL;
    const char*       pValueStr = NULL;
    IHXKeyValueList*  pKeyedHdrs;

    if (!pRFC822Headers)
    {
        // XXX: aak - should probably flag an error, but this not a
        // hindrance now, so i'll let it return HXR_OK.
        return HXR_OK;
    }

    // Find out if the IHXValues supports IHXKeyValueList
    // XXX showell - eventually, we should just make all callers
    // give us an IHXKeyValueList, since it's more efficient,
    // and so we don't overwrite duplicate headers.
    res = pRFC822Headers->QueryInterface(IID_IHXKeyValueList,
                                         (void**) &pKeyedHdrs);

    if (res == HXR_OK)
    {
        IHXKeyValueListIter* pListIter = NULL;
        pKeyedHdrs->GetIter(pListIter);
        HX_ASSERT(pListIter);

        while (pListIter->GetNextPair(pName, pValue) == HXR_OK)
        {
            pValueStr = (const char*)pValue->GetBuffer();
            if (pValueStr != NULL)
            {
                pMIMEHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHdr->SetFromString(pName, pValueStr);
                pMsg->AddHeader(pMIMEHdr);
            }
            pValue->Release();
        }
        HX_RELEASE(pListIter);
    }
    else
    {
        res = pRFC822Headers->GetFirstPropertyCString(pName, pValue);
        while (res == HXR_OK)
        {
            pValueStr = (const char*)pValue->GetBuffer();
            if (pValueStr != NULL)
            {
                pMIMEHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHdr->SetFromString(pName, pValueStr);
                pMsg->AddHeader(pMIMEHdr);
            }
            pValue->Release();

            res = pRFC822Headers->GetNextPropertyCString(pName, pValue);
        }
    }

    HX_RELEASE(pKeyedHdrs);

    return HXR_OK;
}

/**
 * \brief SendResponse - send an RTSP response with the given result code
 *
 * The m_pRespMsg member variable holds the response message for this request
 * (we only process one RTSP request at a time!). This method will format
 * and send a response with the desired response code. After we do that we
 * clear m_pRespMsg and if there is unparsed data queued up we kick the parser.
 *
 * Recovery from an error in sendControlMessage needs to be tested and documented. 
 *
 * \param ulCode [in]
 * \param bSkipOnServRespEv [in] : if TRUE, don't call OnServerResponseEvent, we
 * already logged it
 *
 * \return 
 */
HX_RESULT
RTSPServerProtocol::SendResponse(UINT32 ulCode, BOOL bSkipOnServRespEv)
{
    IHXRTSPResponseMessage* pResp = NULL;
    IHXRTSPConsumer* pConsumer = NULL;
    IHXBuffer* pBuffer = NULL;
    UINT32 uCSeq = 0;

    if (m_pRespMsg == NULL)
    {
        return HXR_UNEXPECTED;
    }

    m_pRespMsg->QueryInterface(IID_IHXRTSPResponseMessage, (void**)&pResp);

    m_pRespMsg->QueryInterface(IID_IHXRTSPConsumer, (void**)&pConsumer);

    if (pResp == NULL || pConsumer == NULL)
    {
        // Major catastrophe, this cannot happen
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }

    pResp->SetStatusCode(ulCode);

    pConsumer->AsBuffer(pBuffer);
    uCSeq = m_pRespMsg->GetCSeq();

    if (sendControlMessage(pBuffer) == HXR_SOCKET_NOBUFS)
    {
        //The TCP channel is too congested to
        //transmit any more control messages
        //XXXDWL set timer?
        HX_ASSERT(0);
        m_pResp->HandleProtocolError(HXR_TIMEOUT);
    }

    if (!bSkipOnServRespEv)
    OnServerResponseEvent(ulCode);

    HX_RELEASE(pBuffer);
    HX_RELEASE(pConsumer);
    HX_RELEASE(pResp);

    HX_RELEASE(m_pRespMsg);

    // If we have data queued but no consumer, this message was processed
    // asynchronously.  Continue processing input.
    if (m_pBufFrag != NULL && m_pConsumer == NULL)
    {
        if (handleInput(NULL) != HXR_OK)
	{
	    // for now we will abort with HXR_INVALID_PROTOCOL, even tho the
	    // reason might have been that the server is out of memory, or a
	    // QI failed.
	    m_pResp->HandleProtocolError(HXR_INVALID_PROTOCOL);
	    return HXR_ABORT;
	}
    }

    return HXR_OK;
}

/**
 * \brief  OnOptionsRequest - handle OPTIONS request from client
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
STDMETHODIMP
RTSPServerProtocol::OnOptionsRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;
    BOOL bNeedClientChallenge = TRUE;
    RTSPServerProtocol::Session* pKeepAliveSession = NULL;
    RTSPServerProtocol::Session* pSession = NULL;
    CHXString KeepAliveSessionID;

    IHXRTSPMessage* pMsg = 0;
    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    IHXValues* pRequestHeaders = NULL;
    GetAndConvertRFC822Headers(pMsg, pRequestHeaders);
    if (pRequestHeaders)
    {
        m_pResp->HandleSessionHeaders(pRequestHeaders);
    }
    HX_RELEASE(pRequestHeaders);

    IHXMIMEHeader* pHdr = NULL;
    IHXBuffer* pVal = NULL;

    // For KeepAlive, the player sends OPTIONS.
    // We should associate this with the correct session.
    if (SUCCEEDED(getSessionID(pMsg, KeepAliveSessionID)))
    {
        pKeepAliveSession = getSession(KeepAliveSessionID);
    }

    OnClientRequestEvent(pReqMsg,
                         pMsg,
                         pKeepAliveSession);

    if (pKeepAliveSession)
    {
        HandleStreamAdaptationHeader(KeepAliveSessionID, pMsg, FALSE);
        Handle3GPPLinkCharHeader(KeepAliveSessionID, pMsg, FALSE);
        HandleBandwidthHeader(KeepAliveSessionID, pMsg);
    }

    if (SUCCEEDED(pMsg->GetHeader("User-Agent", pHdr)))
    {
        m_sdpFileType = INTEROP_SDP;
        const char* pHdrStr = NULL;
        pHdr->GetValueAsBuffer(pVal);
        if (pVal != NULL)
        {
            pHdrStr = (const char*)pVal->GetBuffer();
        }
        if (pHdrStr != NULL && strncmp(pHdrStr, "RealMedia", 9) == 0)
        {
            const char* pVersionStr = strstr(pHdrStr, "Version");
            if (pVersionStr && strlen(pVersionStr) >= 14)
            {
                if (strncmp(pVersionStr+8, "6.0.9.", 6) < 0)
                {
                    m_sdpFileType = BACKWARD_COMP_SDP;
                }
                else if (strncmp(pVersionStr+8, "6.0.9.", 6) == 0 &&
                    strstr(pHdrStr, "linux") != NULL)
                {
                    m_sdpFileType = BACKWARD_COMP_SDP;
                }
            }
        }
        HX_RELEASE(pVal);
        HX_RELEASE(pHdr);
    }

    // Is it a midbox? If not, must be a player.
    if (HXR_OK == pMsg->GetHeader("MidBoxChallenge", pHdr))
    {
        HX_RELEASE(pHdr);
        m_clientType = MIDBOX_CLIENT;
    }
    else
    {
        m_clientType = PLAYER_CLIENT;
    }

    if (HXR_OK == pMsg->GetHeader("Pragma", pHdr))
    {
        IHXBuffer* pBufVal = NULL;
        const char* pVal = NULL;
        UINT32 uValLen = 0;
        pHdr->GetValueAsBuffer(pBufVal);
        HX_RELEASE(pHdr);
        if (pBufVal != NULL)
        {
            pVal = (const char*)pBufVal->GetBuffer();
            uValLen = pBufVal->GetSize();
        }
        if (CanCreateSession() && uValLen == 16 &&
            strncasecmp(pVal, "initiate-session", uValLen) == 0)
        {
            BOOL bNeedSessionID = TRUE;

            if (HXR_OK == pMsg->GetHeader("Via", pHdr))
            {
                // count proxies represented in Via header
                int lVia = 0;

                IHXList* pListField = 0;
                IHXListIterator* pIterField = 0;
                pHdr->GetFieldListConst(pListField);
                pIterField = pListField->Begin();
                HX_RELEASE(pListField);

                // XXX: aak -- should have a count of the number
                // of fields in the header.
                while (pIterField->HasItem())
                {
                    lVia++;
                    pIterField->MoveNext();
                }
                HX_RELEASE(pIterField);
                HX_RELEASE(pHdr);

                if (HXR_OK == pMsg->GetHeader(
                    "x-real-supports-initiate-session", pHdr))
                {
                    IHXBuffer* pSupportsISBuf = 0;
                    pHdr->GetValueAsBuffer(pSupportsISBuf);
                    // Get count of proxies claiming support for
                    // "initiate-session"
                    int lSupportIS = atoi((char*)pSupportsISBuf->GetBuffer());
                    HX_RELEASE(pHdr);
                    HX_RELEASE(pSupportsISBuf);

                    // If there is a legacy RealProxy in the chain (does not
                    // append the header "x-real-supports-initiate-session")
                    // we can't establish a session id here
                    if (lVia > lSupportIS)
                        bNeedSessionID = FALSE;
                }
                else
                {
                    bNeedSessionID = FALSE;
                }
            }

            if (bNeedSessionID == TRUE)
            {
                CHXString sessionID;

                IHXBuffer* pUrlBuf = NULL;
                pReqMsg->GetUrl(pUrlBuf);
                NEW_FAST_TEMP_STR(pUrl, 1024, pUrlBuf->GetSize()+1);
                memcpy(pUrl, pUrlBuf->GetBuffer(), pUrlBuf->GetSize());
                pUrl[pUrlBuf->GetSize()] = '\0';
                m_pResp->AddSession(pUrl, pMsg->GetCSeq(), sessionID, TRUE);
                DELETE_FAST_TEMP_STR(pUrl);
                HX_RELEASE(pUrlBuf);

                RTSPServerProtocol::Session* pSession;
                pSession = getSession(sessionID, TRUE);
                HX_ASSERT(pSession != NULL); // Response MUST create a session
                pSession->m_bSessionSetup = TRUE;
                pSession->m_bIsViaRealProxy = IsViaRealProxy(pMsg);

                pSession->AddSessionHeader(m_pRespMsg);

                // If the client requested a session, check for MidBox
#if defined(HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE)
                if (m_clientType == MIDBOX_CLIENT)
                {
                    pSession->m_bIsMidBox = TRUE;
                    bNeedClientChallenge = FALSE;
                    SetChallengeHeader(pSession, RTSP_VERB_OPTIONS,
                                       !bNeedClientChallenge, m_pRespMsg);

                }
#endif /*HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE*/

                if (HXR_OK == pMsg->GetHeader("Session-Initiation-ID", pHdr))
                {
                    IHXBuffer* pIDBuf = 0;
                    pHdr->GetValueAsBuffer(pIDBuf);
                    UINT32 ulID = atoi((const char*)pIDBuf->GetBuffer());

                    (*m_pPipelineMap)[ulID] = (void*)pSession;
                    pSession->m_ulInitiationID = ulID;

                    HX_RELEASE(pHdr);
                    HX_RELEASE(pIDBuf);
                }
            }
        }
        HX_RELEASE(pBufVal);
    }


    if (m_pOptionsResponseHeaders)
    {
        rc = ConvertAndAddRFC822Headers(m_pRespMsg, m_pOptionsResponseHeaders);
        HX_RELEASE(m_pOptionsResponseHeaders);

        if (HXR_OK != rc)
        {
            HX_RELEASE(pReqMsg);
            return rc;
        }
    }

    pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHdr->SetFromString("Server", m_pVersionString);
    m_pRespMsg->AddHeader(pHdr);

    pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHdr->SetFromString("Public", g_szAllMethods);
    m_pRespMsg->AddHeader(pHdr);

    // Send turboplay params, if specified in registry/cfg

    pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    if (m_bDisableTurboPlay)
    {
        pHdr->SetFromString("TurboPlay", "0");
        m_pRespMsg->AddHeader(pHdr);
    }
    else
    {
#ifdef RSD_LIVE_DEBUG
        printf("%p: Forcing TurboPlay\n", this);fflush(0);
#endif
        pHdr->SetFromString("TurboPlay", "1");
        m_pRespMsg->AddHeader(pHdr);
    }

    if (m_lTurboPlayBW != -1)
    {
        char tmp[32];
        snprintf(tmp, 32, "%d", (int)m_lTurboPlayBW);
        tmp[31] = '\0';

        pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pHdr->SetFromString("MaxBandwidth", tmp);
        m_pRespMsg->AddHeader(pHdr);
    }

    // Verify that client is a RealClient
#if defined(HELIX_FEATURE_RTSP_SERVER_CHALLENGE)
    if (bNeedClientChallenge && !m_bIsLocalBoundSocket)
    {
        SetChallengeHeader(pSession, RTSP_VERB_OPTIONS, !bNeedClientChallenge,
                           m_pRespMsg);
    }
#endif /* HELIX_FEATURE_RTSP_SERVER_CHALLENGE */

    AddStatsHeaders();

    SendResponse(200);

    HX_RELEASE(pMsg);

    return HXR_OK;
}

/**
 * \brief OnAnnounceRequest - handle an ANNOUNCE from a connected client.
 * 
 * Currently we handle only client requests in the server core RTSP handling
 * code so we respond to ANNOUNCE with a "501 Method Not Implemented" and   
 * return HXR_NOTIMPL, which will prevent a state change.
 *
 * \param procnum [in]  Parsed request message from the client.
 * \return              HXR_NOTIMPL
 */
HX_RESULT
RTSPServerProtocol::OnAnnounceRequest(IHXRTSPRequestMessage* pReqMsg)
{
    IHXRTSPMessage* pMsg = 0;
    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    OnClientRequestEvent(pReqMsg, pMsg, NULL);

    SendResponse(501);

    HX_RELEASE(pMsg);

    return HXR_NOTIMPL;
}

/**
 * \brief  OnDescribeRequest - handle DESCRIBE request from client
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnDescribeRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;

    BOOL bRTPAvailable = FALSE;
    IHXBuffer* pURL = 0;
    IHXRTSPMessage* pMsg = 0;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);
    pMsg->ReplaceDelimiters(TRUE, '\0');
    pReqMsg->GetUrl(pURL);
    NEW_FAST_TEMP_STR(pDecURL, 1024, pURL->GetSize()+1);
    DecodeURL(pURL->GetBuffer(), pURL->GetSize(), pDecURL);

    IHXValues* pRequestHeaders = NULL;

    OnClientRequestEvent(pReqMsg, pMsg, NULL);

    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;
    rc = getSessionID(pMsg, sessionID);
    if (rc == HXR_OK)
    {
        // If a session was specified, it must exist
        pSession = getSession(sessionID);
        if (pSession == NULL)
        {
            // Session Not Found
            SendResponse(454);
            HX_RELEASE(pMsg);
            HX_RELEASE(pURL);
            return HXR_FAIL;
        }
    }
    else
    {
        IHXMIMEHeader* pHdr = NULL;

        if (HXR_OK == pMsg->GetHeader("Session-Initiation-ID", pHdr))
        {
            IHXBuffer* pIDBuf = 0;
            pHdr->GetValueAsBuffer(pIDBuf);
            UINT32 ulID = atoi((const char*)pIDBuf->GetBuffer());

            BOOL bFound;
            bFound = m_pPipelineMap->Lookup(ulID, (void*&)pSession);
            HX_ASSERT(bFound && pSession);

            if (pSession)
            {
                sessionID = pSession->m_sessionID;
            }

            HX_RELEASE(pHdr);
            HX_RELEASE(pIDBuf);
        }
    }

    GetAndConvertRFC822Headers(pMsg, pRequestHeaders);

    if (m_spChallengeResponseSender.IsValid())
    {
        IHXBuffer* pValue = NULL;
        char szProp[256];
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pValue);
        sprintf(szProp, "%lu", m_ulClientStatsObjId);
        pValue->Set((UCHAR*)szProp, strlen(szProp) + 1);
        pRequestHeaders->SetPropertyCString("ClientStatsObjId", pValue);
        HX_RELEASE(pValue);

        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pValue);
        sprintf(szProp, "%lu", pSession->m_ulSessionStatsObjId);
        pValue->Set((UCHAR*)szProp, strlen(szProp) + 1);
        pRequestHeaders->SetPropertyCString("SessionStatsObjId", pValue);
        HX_RELEASE(pValue);


        // Save New Headers
        m_spRequestChallenge->SetRequestHeaders(pRequestHeaders);

        // Continue where we left off.
        m_spChallengeResponseSender->ResponseReady(m_spRequestChallenge);
        m_spChallengeResponseSender.Release();
    }
    else
    {
        if (pSession == NULL)
        {
            BOOL bRetainEntityForSetup; //XXXTDM: get rid of this nonsense

            bRetainEntityForSetup = IsInHeaderValues(pMsg,
                "Require", RTSPRequireOptionsTable[RTSP_REQUIRE_ENTITY].pcharOption);

            // If no session, create a temporary one for the response
            // Note this is the normal case
            m_pResp->AddSession(pDecURL, pMsg->GetCSeq(), sessionID,
                                bRetainEntityForSetup);
            pSession = getSession(sessionID);
            HX_ASSERT(pSession != NULL); // Response MUST create a session

            pSession->m_bRetainEntityForSetup = bRetainEntityForSetup;
            pSession->m_bIsViaRealProxy = IsViaRealProxy(pMsg);
        }
        else
        {
            pSession->m_bRetainEntityForSetup = TRUE;

            // Send a Session header in the response
            pSession->m_bNeedSessionHeader = TRUE;
        }

        pSession->clearDescribeMimeTypeList();
        pSession->clearStreamInfoList();

        // store these for use by RTSPProtocol::sendsetupstreamresponse()
        pSession->m_RTSPMessageTagOriginating = pReqMsg->GetMethod();

        // Update the UpdateRegistryForLive flag in SessionStats for bcastmgr
        IHXClientStats* pClientStats = m_pClientStatsMgr->GetClient(m_ulClientStatsObjId);
        if (pClientStats)
        {

             IHXSessionStats* pSessionStats = pClientStats->GetSession(pSession->m_ulSessionStatsObjId);
             if (pSessionStats)
             {
                 IHXCheckRetainEntityForSetup* pCheckRetainEntityForSetup = NULL;
                 pSessionStats->QueryInterface(IID_IHXCheckRetainEntityForSetup, (void**)&pCheckRetainEntityForSetup);
                 if (pCheckRetainEntityForSetup && pSession->m_bRetainEntityForSetup)
                 {
                      pCheckRetainEntityForSetup->SetUpdateRegistryForLive();
                      HX_RELEASE(pCheckRetainEntityForSetup);
                 }
                 HX_RELEASE(pSessionStats);
             }
             HX_RELEASE(pClientStats);
        }

        // According to acolwell, all publicly released nanoplayers below
        // v3.2 send a DESCRIBE with every playback.  That is, they are not
        // capable of SDP initiated playback.  The nanoplayer also uses the
        // old style retain-entity-for setup, so we don't have a session at
        // OPTIONS time.  We need to detect some workarounds at the session
        // level so this is the best place to put the detection logic.
        IHXMIMEHeader* pHeader = NULL;
        if( pMsg->GetHeader("ClientID", pHeader) == HXR_OK )
        {
            IHXBuffer* pCIDBuf = NULL;
            pHeader->GetValueAsBuffer(pCIDBuf);
            UINT32 nLen = pCIDBuf->GetSize();
            const char* pId = (const char*)pCIDBuf->GetBuffer();

            // Check for RealOnePlayer_#.#. We don't do fancy numeric parsing
            // here (yet) because all nanoplayers below v3.2 have a single
            // digit major and minor version.
            if (nLen >= 18 && !strncmp(pId, "RealOnePlayer_", 14) &&
                isdigit(pId[14]) && pId[15] == '.' &&
                isdigit(pId[16]) && pId[17] == '.')
            {
                // Nanoplayer < 3.0 cannot handle CRLF in the SDP.
                // acolwell says this issue fixed before the player began
                // checking the server string.  These clients will always
                // use retain-entity-for-setup, so the flag is per session.
                if (pId[14] < '3')
                {
                    pSession->m_bUseOldSdp = TRUE;
                }

                // Nanoplayer < 3.2 cannot handle RTP-Info in resume response.
                // This issue was fixed well after the player began checking
                // the server string.  There exist some players that will not
                // recognize the Helix ident and cannot handle RTP-Info in the
                // resume response, so the flag is per socket.
                if (pId[14] < '3' || (pId[14] == '3' && pId[16] < '2'))
                {
                    m_bNoRtpInfoInResume = TRUE;
                }
            }

            pCIDBuf->Release();
            pHeader->Release();
        }

        // midbox session adds CSOID to this request since proxy chaining
        // causes DESCRIBE request to be send to source (cache)
        if (m_clientType == MIDBOX_CLIENT)
        {
            IHXBuffer* pId = NULL;
            if (SUCCEEDED(pRequestHeaders->GetPropertyCString("ClientStatsObjId", pId)))
            {
                m_ulClientStatsObjId = atoi((char*)pId->GetBuffer());
                m_bIsClientStatsObjIdSet = (m_ulClientStatsObjId > 0);
            }
            HX_RELEASE(pId);
        }

        /// Look for the "Aggregate-Transport" header now
        IHXMIMEHeader* pTransportMIMEType = NULL;

        if (HXR_OK == pMsg->GetHeader("Aggregate-Transport", pTransportMIMEType))
        {
            HX_ASSERT(!pSession->m_pTransportInstantiator);
            HX_RELEASE(pSession->m_pTransportInstantiator);
            pSession->m_pTransportInstantiator = new RTSPTransportInstantiator(TRUE);
            pSession->m_pTransportInstantiator->AddRef();
            pSession->m_pTransportInstantiator->Init(m_pContext, this);

            rc = pSession->m_pTransportInstantiator->parseTransportHeader(pTransportMIMEType);
            HX_RELEASE(pTransportMIMEType);

            if (SUCCEEDED(rc))
            {
                pSession->m_bNeedAggregateTransportHeader = TRUE;
            }
            else
            {
                HX_RELEASE(pSession->m_pTransportInstantiator);

                SendResponse(400);
                HX_RELEASE(pRequestHeaders);
                DELETE_FAST_TEMP_STR(pDecURL);
                HX_RELEASE(pMsg);
                HX_RELEASE(pURL);
                return HXR_FAIL;
            }
        }
 
        // The session setup needs the base url
        // in the describe case this is the same
        // as the message's url.
        char* pCharUrl = new char[pURL->GetSize()+1];
        memcpy(pCharUrl, pURL->GetBuffer(), pURL->GetSize());
        pCharUrl[pURL->GetSize()] = '\0';
        rc = _BeginSetup(pMsg, pCharUrl, sessionID, bRTPAvailable);
        delete[] pCharUrl;
        if (SUCCEEDED(rc))
        {
            HandleBandwidthHeader(sessionID, pMsg);

            rc = m_pResp->HandleStreamDescriptionRequest(pDecURL,
                pRequestHeaders, sessionID, bRTPAvailable);

            IHXBuffer* pID = NULL;
            HX_RESULT ret = pRequestHeaders->GetPropertyCString("ConnID", pID);
            if (SUCCEEDED(ret) && pID)
            {
                m_ulRegistryConnId = atoi((char*)pID->GetBuffer());
            }
            HX_RELEASE(pID);

            ret = pRequestHeaders->GetPropertyCString("SessionNumber", pID);
            if (SUCCEEDED(ret) && pID)
            {
                pSession->m_ulSessionRegistryNumber = atoi((char*)pID->GetBuffer());
            }
            HX_RELEASE(pID);

            if (m_bTrackEvents)
            {
                HX_ASSERT(!m_bIsProxy);

                HX_ASSERT(m_ulClientStatsObjId != 0);
                HX_ASSERT(pSession && pSession->m_ulSessionStatsObjId != 0);

                IHXClientStats* pClientStats = m_pClientStatsMgr->GetClient(m_ulClientStatsObjId);
                HX_ASSERT(pClientStats);

                IHXSessionStats* pSessionStats = pClientStats->GetSession(pSession->m_ulSessionStatsObjId);

                if (pSessionStats)
                {
                    HX_ASSERT(pSession->m_pEventList);
                    pSession->m_pEventList->SetSessionStats(pSessionStats);
                }

                HX_RELEASE(pSessionStats);
                HX_RELEASE(pClientStats);
            }
        }
    }
    HX_RELEASE(pRequestHeaders);
    DELETE_FAST_TEMP_STR(pDecURL);
    HX_RELEASE(pURL);
    HX_RELEASE(pMsg);

    return rc;
}

/**
 * \brief  OnSetupRequest - handle SETUP request from client
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnSetupRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;

    BOOL bGood = TRUE;
    BOOL bUpdatedEvents = FALSE;
    IHXRTSPMessage* pMsg = NULL;
    IHXMIMEHeader* pMIMEHeader = NULL;
    IHXBuffer* pClientStatsObjId;
    IHXBuffer* pBufVal = NULL;
    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;
    UINT32 ulRDTFeatureLevel = 0;

    // Increment the "RTSP stream attempt" reg var

    if (!m_bIsStreamAttemptCounted)
    {
        m_bIsStreamAttemptCounted = TRUE;

        IHXRegistry2* pRegistry2 = NULL;
        if (HXR_OK == m_pContext->QueryInterface(IID_IHXRegistry2,
                    (void**)&pRegistry2))
        {
            INT32 val;
            pRegistry2->ModifyIntByName("server.RTSPAttemptedStreamed", 1, val);
            pRegistry2->Release();
        }
    }


    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    if (m_bIsProxy)
    {
        if (SUCCEEDED(pMsg->GetHeader("OriginUsesTrackID", pMIMEHeader)))
        {
            m_bOriginUsesTrackID = TRUE;
            HX_RELEASE(pMIMEHeader);
        }
    }

    if (HXR_OK == pMsg->GetHeader("RDTFeatureLevel", pMIMEHeader))
    {
        pMIMEHeader->GetValueAsBuffer(pBufVal);
        ulRDTFeatureLevel = atoi((const char *)pBufVal->GetBuffer());
        HX_RELEASE(pMIMEHeader);
        HX_RELEASE(pBufVal);
    }

    if (m_bIsProxy)
    {
        if (SUCCEEDED(pMsg->GetHeader("ClientStatsObjId", pMIMEHeader)))
        {
            pMIMEHeader->GetValueAsBuffer(pClientStatsObjId);
            m_ulClientStatsObjId = atoi((const char*)pClientStatsObjId->GetBuffer());

            if (m_ulClientStatsObjId > 0)
            {
                m_bIsClientStatsObjIdSet = TRUE;
            }

            HX_RELEASE(pClientStatsObjId);
            HX_RELEASE(pMIMEHeader);
        }
    }

    BOOL bHasSessionHeader;
    rc = getSetupSession(pMsg, pSession, bHasSessionHeader);
    if (rc == HXR_OK)
    {
        if (pSession == NULL)
        {
            if (!bUpdatedEvents)
            {
                OnClientRequestEvent(pReqMsg, pMsg, NULL);
            }

            // Session Not Found
            SendResponse(454);
            m_pResp->HandleSetupRequest(HXR_NO_SESSION_ID);
            HX_RELEASE(pMsg);
            return HXR_FAIL;
        }
        if (!bHasSessionHeader)
        {
            // Found using ETag so add Session header to response
            pSession->AddSessionHeader(m_pRespMsg);
        }
        pSession->m_ulRDTFeatureLevel = ulRDTFeatureLevel;

        //store the setup msg for use by bcng transport
        pReqMsg->AddRef();
        HX_RELEASE(pSession->m_pReqMsg);
        pSession->m_pReqMsg = pReqMsg;

        //save the session id
        sessionID = pSession->m_sessionID;
    }

    // Check for midbox challenge
    BOOL bHasMidBoxChallenge = FALSE;
    if (HXR_OK == pMsg->GetHeader("MidBoxChallenge", pMIMEHeader))
    {
        pMIMEHeader->Release();
        bHasMidBoxChallenge = TRUE;
    }

    // Validate the client's challenge response
    ValidateChallengeResp(pSession, sessionID, bHasMidBoxChallenge, pMsg);

    IHXValues* pRequestHeaders = NULL;

    GetAndConvertRFC822Headers(pMsg, pRequestHeaders);

    if (m_spChallengeResponseSender.IsValid())
    {
        // Save New Headers
        m_spRequestChallenge->SetRequestHeaders(pRequestHeaders);

        // Continue where we left off.
        m_spChallengeResponseSender->ResponseReady(m_spRequestChallenge);
        m_spChallengeResponseSender.Release();
    }
    else
    {
        UINT32 ulControlID = 0;

        // get stream ID and base url by parsing message's URL
        IHXBuffer* pURL = NULL;
        pReqMsg->GetUrl(pURL);

        // find the query params before looking for /streamid
        char* pURLStr = (char*)pURL->GetBuffer();
        UINT32 ulURLSize = pURL->GetSize();
        UINT32 ulQuerySize = 0;

        char* pQueryParams = strchr(pURLStr, '?');
        if(pQueryParams)
        {
            ulURLSize = pQueryParams - pURLStr;
            ulQuerySize = pURL->GetSize() - ulURLSize;
        }

        NEW_FAST_TEMP_STR(pDecURL, 1024, pURL->GetSize()+1);
        DecodeURL(pURL->GetBuffer(), ulURLSize, pDecURL);

        char* pDecQuery = NULL;
        char* pStream = strrchr(pDecURL, '/');
        BOOL bFoundStreamId = FALSE;

        /*
         * XXXTDM: the url parsing here has turned into a mess because we
         * refuse to use CHXURL.  Technically, it isn't RFC compliant (see
         * the strrchr and strchr calls).  We *need* a server url class.
         */

        if (pStream && GetStreamId(pStream+1, &ulControlID))
        {
            *pStream = '\0';
            pDecQuery = pStream;
            bFoundStreamId = TRUE;
        }

        // unescape query params over the /streamid or at the end.
        // we already made sure we have enough space
        // XXXJDG this shouldn't really happen here. url parsing is a
        // serious mess.
        if (pQueryParams)
        {
            if(!pDecQuery)
            {
                pDecQuery = pDecURL + strlen(pDecURL);
            }
            DecodeURL((const BYTE*)pQueryParams, ulQuerySize, pDecQuery);

            // XXXJDG stupid hack
            // If we didn't find the streamid before the query params,
            // look for it after
            if(!bFoundStreamId)
            {
                pStream = strrchr(pDecQuery, '/');
                if (pStream && GetStreamId(pStream+1, &ulControlID))
                {
                    *pStream = '\0';
                    bFoundStreamId = TRUE;
                }
            }
        }

        if (!bFoundStreamId)
        {
            // well we will try to see if this is a single stream
            // presentation.  If it is not, return an error.
            // (Note: pSession is set above)
            if (pSession && pSession->m_uStreamCount > 1)
            {
                if (!bUpdatedEvents)
                {
                    OnClientRequestEvent(pReqMsg, pMsg, pSession);
                    bUpdatedEvents = TRUE;
                }
                SendResponse(459);

                m_pResp->HandleSetupRequest(
                        HXR_AGGREGATE_OP_NOT_ALLOWED);

                rc = HXR_FAIL;
                bGood = FALSE;
            }
        }

        // This is a little trick to avoid using
        // explicit goto's
        // (because we hatesss them my preciousss)
        while (bGood)
        {
            BOOL bNoExistingSessionFound = FALSE;
            BOOL bLastDitchSession = FALSE;

            /* If we're doing this setup for an existing session, we should have
             * obtained the session id after the call to ::getSetupSession().
             */

            // Last Ditch Effort to find Existing Session
            // Mostly for Announced (vs. Described) Sessions
            if (sessionID.IsEmpty())
            {
                m_pSessionManager->getSessionID(pURL, this, TRUE, sessionID);

                if (!sessionID.IsEmpty())
                {
                    bLastDitchSession = TRUE;
                }
            }

            if (sessionID.IsEmpty())
            {
                // Couldn't find an existing Session so Setup a new
                // session and the first stream in it.
                //
                if (!CanCreateSession())
                {
                    // Do not associate with session.
                    if (!bUpdatedEvents)
                    {
                        OnClientRequestEvent(pReqMsg, pMsg, NULL);
                        bUpdatedEvents = TRUE;
                    }
                    SendResponse(500);

                    m_pResp->HandleSetupRequest(HXR_FAILED);

                    rc = HXR_FAIL;
                    break;
                }

                // time to create a session in the protocol & player
                // note this calls RTSPServerProtocol->AddSession()

                m_pResp->AddSession(pDecURL, pMsg->GetCSeq(), sessionID,
                        TRUE);
                bNoExistingSessionFound = TRUE;

                // sessionID is valid..
            }

            if (!sessionID.IsEmpty())
            {
                // got a sessionID
                //

                // Mark this Session instance as SETUP.
                //
                RTSPSessionItem* pSessionItemCurrent =
                    m_pSessionManager->findInstance(sessionID);
                RTSPServerProtocol::Session* pSessionNew =
                    getSession(sessionID);

                IHXBuffer* pSessionStatsObjId = NULL;

                if (!pSessionItemCurrent || !pSessionNew)
                {
                    // The Session requested does not exist.

                    SendResponse(454);

                    m_pResp->HandleSetupRequest(HXR_NO_SESSION_ID);

                    rc = HXR_FAIL;
                    break;
                }

                // Store client requested stream url for RTP-Info
                HX_ASSERT(pSessionNew->m_pStreamUrl == NULL);
                ulURLSize = pURL->GetSize(); // Use the full requseted url
                pSessionNew->m_pStreamUrl = new char[ulURLSize+1];
                memcpy(pSessionNew->m_pStreamUrl, pURLStr, ulURLSize);
                pSessionNew->m_pStreamUrl[ulURLSize] = '\0';

                //store the setup msg for later use
                pReqMsg->AddRef();
                HX_RELEASE(pSessionNew->m_pReqMsg);
                pSessionNew->m_pReqMsg = pReqMsg;

                // We send the SessionStatsObjId via localbound socket in the proxy case.
                // (for proxy/server clientstats tie-in).
                // Players should never send us this message!
                if (SUCCEEDED(pRequestHeaders->GetPropertyCString(
                                "SessionStatsObjId", pSessionStatsObjId)) &&  m_bIsProxy)
                {
                    pSessionNew->m_ulSessionStatsObjId = atoi((char*)pSessionStatsObjId->GetBuffer());
                }

                if (m_bTrackEvents)
                {
                    HX_ASSERT(!m_bIsProxy);

                    IHXClientStats* pClientStats = m_pClientStatsMgr->GetClient(m_ulClientStatsObjId);
                    HX_ASSERT(pClientStats);

                    IHXSessionStats* pSessionStats = pClientStats->GetSession(pSessionNew->m_ulSessionStatsObjId);
                    HX_ASSERT(pSessionStats);

                    HX_ASSERT(pSessionNew->m_pEventList);
                    pSessionNew->m_pEventList->SetSessionStats(pSessionStats);

                    HX_RELEASE(pSessionStats);
                    HX_RELEASE(pClientStats);
                }

                HX_RELEASE(pSessionStatsObjId);

                if (!bUpdatedEvents)
                {
                    OnClientRequestEvent(pReqMsg, pMsg, pSessionNew);
                    bUpdatedEvents = TRUE;
                }

                pSessionItemCurrent->m_bSetup = TRUE;

                if (bNoExistingSessionFound)
                {
                    // initialize the session created above.
                    //

                    pSessionNew->clearDescribeMimeTypeList();
                    pSessionNew->clearStreamInfoList();
                    pSessionNew->m_bIsViaRealProxy = IsViaRealProxy(pMsg);

                    // store this for use by
                    // RTSPProtocol::sendsetupstreamresponse()
                    pSessionNew->m_RTSPMessageTagOriginating = pReqMsg->GetMethod();

                    // store this for use by _FinishSetup()
                    pSessionNew->m_ulControlID = ulControlID;

                    //set RDTFeatureLevel for transport setup in _FinishSetup
                    pSessionNew->m_ulRDTFeatureLevel = ulRDTFeatureLevel;

                    // Add Session header to the response message
                    pSessionNew->AddSessionHeader(m_pRespMsg);
                }

                if (bLastDitchSession)
                {
                    //set RDTFeatureLevel for transport setup in _FinishSetup
                    pSessionNew->m_ulRDTFeatureLevel = ulRDTFeatureLevel;

                    // Add Session header to the response message
                    pSessionNew->AddSessionHeader(m_pRespMsg);
                }

                IHXMIMEHeader* pTransportMIMEType = 0;
                rc = HXR_OK;

                if (HXR_OK == pMsg->GetHeader("Transport", pTransportMIMEType))
                {
                    if (!pSessionNew->m_pTransportInstantiator)
                    {
                        pSessionNew->m_pTransportInstantiator = new RTSPTransportInstantiator(FALSE);
                        pSessionNew->m_pTransportInstantiator->AddRef();
                        pSessionNew->m_pTransportInstantiator->Init(m_pContext, this);
                    }

                    if (pSessionNew->m_pTransportInstantiator->IsAggregateTransport())
                    {
                        if (!pSessionNew->m_pTransportInstantiator->matchSelected(pTransportMIMEType))
                        {
                            rc = HXR_FAIL;
                        }
                    }
                    else 
                    {
                        rc = pSessionNew->m_pTransportInstantiator->parseTransportHeader(
                                pTransportMIMEType, ulControlID);
                    }

                    HX_RELEASE(pTransportMIMEType);

                    if (!SUCCEEDED(rc))
                    {
                        FailAndTeardown(461, pSessionNew);
                        break;
                    }
                }
                else
                {
                    // Cannot setup without a Transport header
                    FailAndTeardown(461, pSessionNew);

                    rc = HXR_FAIL;
                    break;
                }

                // The X-Real-RealProxy-SendTo-IP header will only exist
                // when there is a proxy, it implements a performance
                // shortcut (server doesn't know client, this is how it
                // gets introduced)

                IHXMIMEHeader* pForeignAddrMimeType = 0;
                IHXNetServices* pNetSvc = NULL;

                m_pContext->QueryInterface(IID_IHXNetServices,
                        (void**)&pNetSvc);
                HX_ASSERT(pNetSvc != NULL);

                IHXValues* pValuePass = NULL;
                m_pCommonClassFactory->CreateInstance(CLSID_IHXValues, (void**)&pValuePass);

                if (HXR_OK == pMsg->GetHeader("X-Real-RealProxy-SendTo-IP",
                            pForeignAddrMimeType))
                {
                    pForeignAddrMimeType->GetValueAsBuffer(pBufVal);
                    HX_RELEASE(pForeignAddrMimeType);
                    if (pBufVal)
                    {
                        const char* pAddr = (const char*)pBufVal->GetBuffer();
                        UINT32 uAddrLen = pBufVal->GetSize();
                        IHXBuffer* pBufAddr = NULL;
                        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBufAddr);

                        // Possible inputs are:
                        // New proxy: IPv4!xxx.xxx.xxx.xxx
                        //            IPv6!xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
                        // Old proxy: [integer]

                        if (uAddrLen > 5 && memcmp(pAddr, "IPv6!", 5) == 0)
                        {
                            pAddr += 5;
                            uAddrLen -= 5;
                            HX_RELEASE(m_pProxyPeerAddr);
                            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN6, &m_pProxyPeerAddr);
                            pBufAddr->SetSize(uAddrLen+1);
                            memcpy(pBufAddr->GetBuffer(), pAddr, uAddrLen);
                            *(pBufAddr->GetBuffer()+uAddrLen) = 0;
                        }
                        else if (uAddrLen > 5 && memcmp(pAddr, "IPv4!", 5) == 0)
                        {
                            pAddr += 5;
                            uAddrLen -= 5;
                            HX_RELEASE(m_pProxyPeerAddr);
                            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &m_pProxyPeerAddr);
                            pBufAddr->SetSize(uAddrLen+1);
                            memcpy(pBufAddr->GetBuffer(), pAddr, uAddrLen);
                            *(pBufAddr->GetBuffer()+uAddrLen) = 0;
                        }
                        else
                        {
                            char szAddr[HX_ADDRSTRLEN_IN4];
                            UINT32 uAddr;
                            HX_RELEASE(m_pProxyPeerAddr);
                            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &m_pProxyPeerAddr);
                            uAddr = atoi((const char*)pBufVal->GetBuffer());
                            strcpy(szAddr, HXInetNtoa(htonl(uAddr)));
                            pBufAddr->Set((UCHAR*)szAddr, strlen(szAddr)+1);
                        }
                        m_pProxyPeerAddr->SetAddr(pBufAddr);
                        pValuePass->SetPropertyCString("X-Real-RealProxy-SendTo-IP", pBufAddr);
                        HX_RELEASE(pBufAddr);
                        HX_RELEASE(pBufVal);
                    }
                }

                IHXMIMEHeader* pLocalAddrMimeType = 0;

                if (HXR_OK == pMsg->GetHeader("X-Real-RealProxy-Bind-IP",
                            pLocalAddrMimeType))
                {
                    pLocalAddrMimeType->GetValueAsBuffer(pBufVal);
                    HX_RELEASE(pLocalAddrMimeType);

                    if (pBufVal)
                    {
                        const char* pAddr = (const char*)pBufVal->GetBuffer();
                        UINT32 uAddrLen = pBufVal->GetSize();
                        IHXBuffer* pBufAddr = NULL;
                        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBufAddr);

                        // Possible inputs are:
                        // New proxy: IPv4!xxx.xxx.xxx.xxx
                        //            IPv6!xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
                        // Old proxy: [integer]

                        if (uAddrLen > 5 && memcmp(pAddr, "IPv6!", 5) == 0)
                        {
                            pAddr += 5;
                            uAddrLen -= 5;
                            HX_RELEASE(m_pProxyLocalAddr);
                            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN6, &m_pProxyLocalAddr);
                            pBufAddr->SetSize(uAddrLen+1);
                            memcpy(pBufAddr->GetBuffer(), pAddr, uAddrLen);
                            *(pBufAddr->GetBuffer()+uAddrLen) = 0;
                        }
                        else if (uAddrLen > 5 && memcmp(pAddr, "IPv4!", 5) == 0)
                        {
                            pAddr += 5;
                            uAddrLen -= 5;
                            HX_RELEASE(m_pProxyLocalAddr);
                            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &m_pProxyLocalAddr);
                            pBufAddr->SetSize(uAddrLen+1);
                            memcpy(pBufAddr->GetBuffer(), pAddr, uAddrLen);
                            *(pBufAddr->GetBuffer()+uAddrLen) = 0;
                        }
                        else
                        {
                            char szAddr[HX_ADDRSTRLEN_IN4];
                            UINT32 uAddr;
                            HX_RELEASE(m_pProxyLocalAddr);
                            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &m_pProxyLocalAddr);
                            uAddr = atoi((const char*)pBufVal->GetBuffer());
                            strcpy(szAddr, HXInetNtoa(htonl(uAddr)));
                            pBufAddr->Set((UCHAR*)szAddr, strlen(szAddr)+1);
                        }
                        m_pProxyLocalAddr->SetAddr(pBufAddr);
                        HX_RELEASE(pBufAddr);
                        HX_RELEASE(pBufVal);
                    }
                }

                IHXRTSPProtocolValuePass* pPassResp;

                if (m_pResp && SUCCEEDED(m_pResp->QueryInterface(IID_IHXRTSPProtocolValuePass, 
                                (void**)&pPassResp)))
                {
                    pPassResp->SetValues(pValuePass);
                }

                HX_RELEASE(pPassResp);
                HX_RELEASE(pValuePass);

                /*
                 * XXXMC
                 * Special-case handling for PV clients behind a NAT/firewall.
                 */
                IHXMIMEHeader* pUAMimeHdr = NULL;

                if (m_bPVEmulationEnabled &&
                        SUCCEEDED(pMsg->GetHeader("User-Agent", pUAMimeHdr)))
                {
                    IHXBuffer* pUABuff = NULL;
                    pUAMimeHdr->GetValueAsBuffer(pUABuff);
                    UINT32 ulMinUAPrefixLen = m_pPVClientUAPrefix->GetSize() - 1;

                    if (pUABuff && (pUABuff->GetSize() >= ulMinUAPrefixLen) &&
                            (strncasecmp((const char*)pUABuff->GetBuffer(),
                                         (const char*)m_pPVClientUAPrefix->GetBuffer(),
                                         ulMinUAPrefixLen) == 0))
                    {
                        pSessionNew->m_bEmulatePVSession = TRUE;
                    }

                    HX_RELEASE(pUABuff);
                }
                HX_RELEASE(pUAMimeHdr);

                if (bNoExistingSessionFound)
                {
                    BOOL bRTPAvailable = FALSE;

                    // Complete Session Initialization
                    //

                    // Check for midbox challenge
                    if (bHasMidBoxChallenge && !pSessionNew->m_bIsMidBox)
                    {
                        pSessionNew->m_bIsMidBox = TRUE;
                    }

                    // The session setup needs the base url
                    rc = _BeginSetup(pMsg, pDecURL, sessionID, bRTPAvailable);

                    if (SUCCEEDED(rc))
                    {
                        // This will finish the rest of the session setup
                        // (Shares code with the DESCRIBE method until
                        // you get to RTSPProtocol::sendsetupstreamresponse())
                        rc = m_pResp->HandleStreamDescriptionRequest(pDecURL,
                                pRequestHeaders, sessionID, bRTPAvailable);

                        IHXBuffer* pID = NULL;
                        HX_RESULT ret = pRequestHeaders->GetPropertyCString("ConnID", pID);
                        if (SUCCEEDED(ret) && pID)
                        {
                            m_ulRegistryConnId = atoi((char*)pID->GetBuffer());
                        }
                        HX_RELEASE(pID);

                        ret = pRequestHeaders->GetPropertyCString("SessionNumber", pID);
                        if (SUCCEEDED(ret) && pID)
                        {
                            pSessionNew->m_ulSessionRegistryNumber = atoi((char*)pID->GetBuffer());
                        }
                        HX_RELEASE(pID);

                    }
                }
                else
                {
                    // Setup stream in an existing session.
                    //

                    // Go straight to _FinishSetup

                    UINT16 usStreamNumber;
                    RTSPStreamInfo* pStreamInfo =
                        pSessionNew->GetStreamFromControl(ulControlID);

                    if (pStreamInfo)
                    {
                        usStreamNumber = pStreamInfo->m_streamNumber;
                    }
                    else
                    {
                        HX_ASSERT(!"SETUP of unknown stream");
                        rc = HXR_FAIL;
                        RTSPServerProtocol::Session* pSession = getSession(sessionID);
                        FailAndTeardown(404, pSession);

                        break;
                    }

                    HX_RESULT ret = HXR_FAIL;
                    if (pSessionNew->m_pFFAdvise)
                    {
                        m_usStreamNumberTmp = usStreamNumber;
                        if(m_pSessionIDTmp != NULL)
                        {
                            delete [] m_pSessionIDTmp;
                        }
                        m_pSessionIDTmp = new_string(sessionID);

                        if(pURL)
                        {
                            pRequestHeaders->SetPropertyCString("SaltUrl", pURL);
                        }

                        ret = pSessionNew->m_pFFAdvise->OnHeaders(CM_RTSP_SETUP,
                                pRequestHeaders,
                                this);
                        rc = HXR_OK;
                        // Continues via OnHeadersDone() if HXR_OK.
                    }

                    if (ret != HXR_OK)
                    {
                        rc = _FinishSetup(usStreamNumber, sessionID);
                    }
                }

                if (SUCCEEDED(rc))
                {
                    GetSalt(pReqMsg,
                            pRequestHeaders,
                            pSessionNew->m_ulSessionRegistryNumber,
                            pSessionNew->m_ulSessionStatsObjId);
                }

                break;
            }
            // make sure that we never actually loop.
            break;
        }
        DELETE_FAST_TEMP_STR(pDecURL);
        HX_RELEASE(pURL);
    }

    // Update the UpdateRegistryForLive flag in SessionStats for bcastmgr
    pSession = getSession(sessionID);
    HX_ASSERT(pSession);
    IHXClientStats* pClientStats = m_pClientStatsMgr->GetClient(m_ulClientStatsObjId);
    if (pClientStats)
    {
        IHXSessionStats* pSessionStats = pClientStats->GetSession(pSession->m_ulSessionStatsObjId);
        if (pSessionStats)
        {
            IHXCheckRetainEntityForSetup* pCheckRetainEntityForSetup = NULL;
            pSessionStats->QueryInterface(IID_IHXCheckRetainEntityForSetup, (void**)&pCheckRetainEntityForSetup);
            if (pCheckRetainEntityForSetup)
            {
                 pCheckRetainEntityForSetup->SetUpdateRegistryForLive();
                 HX_RELEASE(pCheckRetainEntityForSetup);
            }
            HX_RELEASE(pSessionStats);
        }
        HX_RELEASE(pClientStats);
    }

    HX_RELEASE(pMsg);
    HX_RELEASE(pRequestHeaders);

    return rc;
}

/**
 * \brief FailAndTeardown - if SETUP failed, remove session
 *
 * \param status [in] : if not HXR_OK, the SETUP did not succeed
 * \param uResultCode [in] : RTSP result code (2xx if we succeeded)
 * \param pSession [in]
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::FailAndTeardown(UINT32 uResultCode,
                                    RTSPServerProtocol::Session* pSession)
{
    // Do RTSP events before we remove the "Session" header
    // to ensure proper req/resp mapping.
    OnServerResponseEvent(uResultCode);

    // Failure, session needs to be deleted
    m_pRespMsg->RemoveHeader("Session");
        
    // Send, but skip sending OnServerResponseEvent again.
    SendResponse(uResultCode, TRUE);

    HX_ASSERT(pSession != NULL);
    if (pSession != NULL)
    {
        SetStatus(pSession->m_ulSessionRegistryNumber,
                  pSession->m_ulSessionStatsObjId, uResultCode);
        m_pResp->HandleTeardownRequest(pSession->m_sessionID);
        m_pSessionManager->removeSessionInstance(pSession->m_sessionID);
    }

    return m_pResp->HandleSetupRequest(HXR_FAIL);
}

/**
 * \brief OnRecordRequest - handle a RECORD request from a client.
 *     
 * Currently we handle only client requests in the server core RTSP handling
 * code so we respond to RECORD with a "501 Method Not Implemented" and 
 * return HXR_NOTIMPL, which will prevent a state change. Note that if we are
 * in the INIT or PLAY state the RECORD is flagged as out of state and we will
 * never reach this method.
 *
 * \param procnum [in]  Parsed request message from the client.
 * \return              HXR_NOTIMPL
 */
HX_RESULT
RTSPServerProtocol::OnRecordRequest(IHXRTSPRequestMessage* pReqMsg)
{
    IHXRTSPMessage* pMsg = 0;
    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    OnClientRequestEvent(pReqMsg, pMsg, NULL);

    SendResponse(501);

    HX_RELEASE(pMsg);

    return HXR_NOTIMPL;
}

/**
 * \brief  OnGetParameterRequest - handle GET_PARAMETER request from client
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnGetParameterRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;

    IHXBuffer* pBuffer = NULL;
    IHXBuffer* pParamName = NULL;
    IHXRTSPMessage* pMsg = NULL;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;
    rc = getSessionID(pMsg, sessionID);
    if (rc == HXR_OK)
    {
        pSession = getSession(sessionID);
    }
    if (pSession == NULL)
    {
        // Session Not Found
        SendResponse(454);
        HX_RELEASE(pMsg);
        return HXR_FAIL;
    }

    OnClientRequestEvent(pReqMsg, pMsg, pSession);

    //ABD.
    BOOL bABDEnabled = FALSE;
    UINT32 ulPacketCount = 0;
    UINT32 ulPacketSize = 0;
    INT32 lSeqNo = -1;
    IHXMIMEHeader* pHeader1 = NULL;
    IHXMIMEHeader* pHeader = NULL;
    BOOL bABDRequest = FALSE;

    pMsg->GetContent(pParamName);
    rc = HXR_FAIL;
    if (pParamName != NULL)
    {
        rc = m_pResp->HandleGetParameterRequest(RTSP_PARAM_STRING,
            (const char *)pParamName->GetBuffer(), &pBuffer);

	//Check if this request is for ABD.
        if (strncmp( (const char*)pParamName->GetBuffer(), "ABD", 3) == 0)
        {
              bABDRequest = TRUE;
        }
        HX_RELEASE(pParamName);
    }

    if ( (!m_bSendBWPackets) || (ABD_GET_PARAM_RECEIVED == pSession->m_ABDState)) 
    {
        if (bABDRequest) rc = HXR_FAIL; //Multiple  requests for ABD in one session not allowed.
	                         // we  send 451 response
    }
    else
    {
        if (HXR_OK == pMsg->GetHeader("AutoBWDetection", pHeader))
        {
            pSession->m_ABDState = ABD_GET_PARAM_RECEIVED;
            IHXBuffer* pBufVal = NULL;
            pHeader->GetValueAsBuffer(pBufVal);
	    if (NULL != pBufVal)
	    {
                bABDEnabled = (BOOL)atoi((const char *)pBufVal->GetBuffer());
	    }
            HX_RELEASE(pBufVal);

            if (bABDEnabled)
            {
                if (HXR_OK == pMsg->GetHeader("AutoBWDetectionPackets", pHeader1))
	        {
                    pHeader1->GetValueAsBuffer(pBufVal);
                    if (NULL != pBufVal)
	            {	
		        ulPacketCount = atoi((const char *)pBufVal->GetBuffer());
		    }
                    HX_RELEASE(pBufVal);
                    HX_RELEASE(pHeader1);
    
	            if (ulPacketCount >= MIN_ABD_PACKETS && ulPacketCount <= MAX_ABD_PACKETS)
		    {
                        if (HXR_OK == pMsg->GetHeader("AutoBWDetectionPacketSize", pHeader1))
	                {
                            pHeader1->GetValueAsBuffer(pBufVal);
                            if (NULL != pBufVal)
			    {
			        ulPacketSize = atoi((const char *)pBufVal->GetBuffer());
			    }
                            HX_RELEASE(pBufVal);

	                    if (ulPacketSize >= MIN_ABD_PACKET_SIZE && ulPacketSize <= MAX_ABD_PACKET_SIZE)
			    {
                                if (HXR_OK == m_pRespMsg->AddHeader(pHeader))
			        {
                                    pSession->sendBWProbingPackets(ulPacketCount, ulPacketSize, lSeqNo);
			        }
			    }
	                }
                        HX_RELEASE(pHeader1);
		    }
	        }
            }
            HX_RELEASE(pHeader);
            HX_RELEASE(pHeader1);
            HX_RELEASE(pBufVal);
        }
    }


    if (rc == HXR_OK)
    {
        if (pBuffer != NULL)
        {
            IHXMIMEHeader* pHdr;

            pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
            pHdr->SetFromString("Content-type", "text/rtsp-parameters");
            m_pRespMsg->AddHeader(pHdr);

            m_pRespMsg->SetContent(pBuffer);
        }
        SendResponse(200);
    }
    else
    {
        SendResponse(451);
    }
    HX_RELEASE(pBuffer);

    HX_RELEASE(pMsg);

    return HXR_OK;
}

/**
 * \brief  OnSetParameterRequest - handle SET_PARAMETER request from client
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnSetParameterRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;

    BOOL bDidSubscribe = FALSE;
    BOOL paramOK = FALSE;

    IHXRTSPMessage* pMsg = NULL;
    IHXMIMEHeader* pMIMEHeader = NULL;
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    IHXList* pListParam = NULL;
    IHXListIterator* pIterParam = NULL;
    IUnknown* pUnkParam = NULL;
    IHXMIMEParameter* pParam = NULL;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;
    rc = getSessionID(pMsg, sessionID);
    if (rc == HXR_OK)
    {
        pSession = getSession(sessionID);
    }

    OnClientRequestEvent(pReqMsg, pMsg, pSession);

    /*
     * Look for the FrameControl LimitBandwidthByDropping message.
     */
    IHXMIMEHeader* pFrameControl = NULL;
    if (HXR_OK == pMsg->GetHeader("FrameControl", pFrameControl))
    {
        if (pSession == NULL)
        {
            SendResponse(454);
            HX_RELEASE(pMsg);
            HX_RELEASE(pFrameControl);
            return HXR_FAIL;
        }

        paramOK = TRUE;
        pFrameControl->GetFieldListConst(pListField);
        HX_RELEASE(pFrameControl);
        pIterField = pListField->Begin();

        while (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField,
                (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetParamListConst(pListParam);
            HX_RELEASE(pField);
            pIterParam = pListParam->Begin();

            UINT16 streamNumber = 0xffff;
            UINT32 ulBandwidthLimit = 0xffffffff;

            while (pIterParam->HasItem())
            {
                pBufAttr = NULL;
                pBufVal = NULL;
                pUnkParam = pIterParam->GetItem();
                pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                    (void **)&pParam);
                HX_RELEASE(pUnkParam);
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "stream", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    streamNumber = (UINT16) strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }
                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "LimitBandwidthByDropping", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    ulBandwidthLimit = strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }

                if (streamNumber < 0xffff && ulBandwidthLimit < 0xffffffff)
                {
                    m_pResp->HandleLimitBandwidthByDropping(streamNumber,
                        pSession->m_sessionID, ulBandwidthLimit);
                }

                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
                pIterParam->MoveNext();
            }
            HX_RELEASE(pListParam);
            HX_RELEASE(pIterParam);
            pIterField->MoveNext();
        }
        HX_RELEASE(pListField);
        HX_RELEASE(pIterField);
    }

    IHXMIMEHeader* pSwitchTransport = NULL;
    if (HXR_OK == pMsg->GetHeader("SwitchTransport", pSwitchTransport))
    {
        HX_RELEASE(pSwitchTransport);
#ifdef HELIX_FEATURE_SERVER_BCNG
        if (pSession == NULL)
#endif
        {
            SendResponse(454);
            HX_RELEASE(pMsg);
            return HXR_FAIL;
        }

#ifdef HELIX_FEATURE_SERVER_BCNG
        BCNGTransport* pTransport = (BCNGTransport*)pSession->getFirstTransportSetup();
        pTransport->SwitchTransportProtocolToTCP();

        paramOK = TRUE;
#endif
    }

    // Handle Bandwidth header
    HandleBandwidthHeader(sessionID, pMsg);

    /*
     * Look for the SetDeliveryBandwidth RTSP message.
     */
    IHXMIMEHeader* pSetDeliveryBandwidth = 0;
    if (HXR_OK == pMsg->GetHeader("SetDeliveryBandwidth",
        pSetDeliveryBandwidth))
    {
        if (pSession == NULL)
        {
            SendResponse(454);
            HX_RELEASE(pMsg);
            HX_RELEASE(pSetDeliveryBandwidth);
            return HXR_FAIL;
        }

        if (!pSession->m_bSetupsComplete)
        {
            FinishAllSetups(pSession);
        }

        paramOK = TRUE;
        pSetDeliveryBandwidth->GetFieldListConst(pListField);
        HX_RELEASE(pSetDeliveryBandwidth);
        pIterField = pListField->Begin();

        while (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField,
                (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetParamListConst(pListParam);
            HX_RELEASE(pField);
            pIterParam = pListParam->Begin();

            UINT32 ulBackOff = 0xffffffff;
            UINT32 ulBandwidth = 0xffffffff;

            while (pIterParam->HasItem())
            {
                pBufAttr = NULL;
                pBufVal = NULL;
                pUnkParam = pIterParam->GetItem();
                pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                    (void **)&pParam);
                HX_RELEASE(pUnkParam);
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "BackOff", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    ulBackOff = (UINT16)strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }
                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "Bandwidth", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    ulBandwidth = strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }

                if (ulBackOff < 0xffffffff && ulBandwidth < 0xffffffff)
                {
                    m_pResp->HandleSetDeliveryBandwidth(ulBackOff,
                        pSession->m_sessionID, ulBandwidth);
                }
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
                pIterParam->MoveNext();
            }
            HX_RELEASE(pListParam);
            HX_RELEASE(pIterParam);
            pIterField->MoveNext();
        }
        HX_RELEASE(pListField);
        HX_RELEASE(pIterField);
    }

    IHXMIMEHeader* pSubscribe = NULL;
    if (HXR_OK == pMsg->GetHeader("Subscribe", pSubscribe))
    {
        if (pSession == NULL)
        {
            SendResponse(454);
            HX_RELEASE(pMsg);
            HX_RELEASE(pSubscribe);
            return HXR_FAIL;
        }

        if (!pSession->m_bSetupsComplete)
        {
            FinishAllSetups(pSession);
        }

        CHXSimpleList* pSubList = new CHXSimpleList;

        paramOK = TRUE;
        pSubscribe->GetFieldListConst(pListField);
        HX_RELEASE(pSubscribe);
        pIterField = pListField->Begin();

        while (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField,
                (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetParamListConst(pListParam);
            HX_RELEASE(pField);
            pIterParam = pListParam->Begin();

            UINT16 streamNumber = 0xFFFF;
            UINT16 ruleNumber = 0xFFFF;

            while (pIterParam->HasItem())
            {
                pBufAttr = NULL;
                pBufVal = NULL;
                pUnkParam = pIterParam->GetItem();
                pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                    (void **)&pParam);
                HX_RELEASE(pUnkParam);
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "stream", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    streamNumber = (UINT16)strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }
                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "rule", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    ruleNumber = (UINT16)strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }

                if (ruleNumber < 0xFFFF && streamNumber < 0xFFFF)
                {
                    RTSPSubscription* pSub = new RTSPSubscription;
                    pSub->m_ruleNumber = ruleNumber;
                    pSub->m_streamNumber = streamNumber;
                    pSubList->AddTail(pSub);
                }
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
                pIterParam->MoveNext();
            }
            HX_RELEASE(pListParam);
            HX_RELEASE(pIterParam);
            pIterField->MoveNext();
        }
        HX_RELEASE(pListField);
        HX_RELEASE(pIterField);

        rc = m_pResp->HandleSubscribe(pSubList, pSession->m_sessionID);
        bDidSubscribe = TRUE;
        pSession->m_bHandleRules = FALSE;
        CHXSimpleList::Iterator i;
        for (i=pSubList->Begin(); i!=pSubList->End(); ++i)
        {
            RTSPSubscription* pSub = (RTSPSubscription*)(*i);
            delete pSub;
        }
        delete pSubList;
    }

    IHXMIMEHeader* pUnsubscribe = NULL;
    if (HXR_OK == pMsg->GetHeader("Unsubscribe", pUnsubscribe))
    {
        if (pSession == NULL)
        {
            SendResponse(454);
            HX_RELEASE(pMsg);
            HX_RELEASE(pUnsubscribe);
            return HXR_FAIL;
        }

        if (!pSession->m_bSetupsComplete)
        {
            FinishAllSetups(pSession);
        }

        CHXSimpleList* pSubList = new CHXSimpleList;

        paramOK = TRUE;
        pUnsubscribe->GetFieldListConst(pListField);
        HX_RELEASE(pUnsubscribe);
        pIterField = pListField->Begin();

        while (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField,
                (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetParamListConst(pListParam);
            HX_RELEASE(pField);
            pIterParam = pListParam->Begin();

            UINT16 streamNumber = 0xFFFF;
            UINT16 ruleNumber = 0xFFFF;

            while (pIterParam->HasItem())
            {
                pBufAttr = NULL;
                pBufVal = NULL;
                pUnkParam = pIterParam->GetItem();
                pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                    (void **)&pParam);
                HX_RELEASE(pUnkParam);
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "stream", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    streamNumber = (UINT16)strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }
                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "rule", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(), pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    ruleNumber = (UINT16)strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }

                if (ruleNumber < 0xFFFF && streamNumber < 0xFFFF)
                {
                    RTSPSubscription* pSub = new RTSPSubscription;
                    pSub->m_ruleNumber = ruleNumber;
                    pSub->m_streamNumber = streamNumber;
                    pSubList->AddTail(pSub);
                }
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
                pIterParam->MoveNext();
            }
            HX_RELEASE(pListParam);
            HX_RELEASE(pIterParam);
            pIterField->MoveNext();
        }
        HX_RELEASE(pListField);
        HX_RELEASE(pIterField);

        rc = m_pResp->HandleUnsubscribe(pSubList, pSession->m_sessionID);
        bDidSubscribe = TRUE;
        pSession->m_bHandleRules = FALSE;
        CHXSimpleList::Iterator i;
        for (i=pSubList->Begin(); i!=pSubList->End(); ++i)
        {
            RTSPSubscription* pSub = (RTSPSubscription*)(*i);
            delete pSub;
        }
        delete pSubList;
    }

    IHXMIMEHeader* pPlayerStats = NULL;
    if (HXR_OK == pMsg->GetHeader("PlayerStats", pPlayerStats))
    {
        if (pSession == NULL)
        {
            SendResponse(454);
            HX_RELEASE(pMsg);
            HX_RELEASE(pPlayerStats);
            return HXR_FAIL;
        }

        paramOK = TRUE;

        pPlayerStats->GetValueAsBuffer(pBufVal);
        HX_ASSERT(pBufVal);

        rc = m_pResp->HandlePlayerStats((const char *)pBufVal->GetBuffer(),
            pSession->m_sessionID);
        HX_RELEASE(pBufVal);
        HX_RELEASE(pPlayerStats);
    }

    //XXXSMP - Too Much Repeat Code, Cleanup Needed
    IHXMIMEHeader* pBackChannel = NULL;
    if (HXR_OK == pMsg->GetHeader("BackChannel", pBackChannel))
    {
        if (pSession == NULL)
        {
            SendResponse(454);
            HX_RELEASE(pMsg);
            return HXR_FAIL;
        }

        paramOK = TRUE;

        IHXPacket* pPacket = NULL;

        UINT16 unStreamNumber = 0;
        if (HXR_OK == pMsg->GetHeader("StreamNumber", pMIMEHeader))
        {
            IHXBuffer* pStreamNumberBuf = 0;
            pMIMEHeader->GetValueAsBuffer(pStreamNumberBuf);
            HX_ASSERT(pStreamNumberBuf);
            unStreamNumber = atoi((const char*)pStreamNumberBuf->GetBuffer());
            HX_RELEASE(pStreamNumberBuf);
            HX_RELEASE(pMIMEHeader);
        }

        paramOK = TRUE;

        if (HXR_OK == pMsg->GetHeader("BackChannel", pMIMEHeader))
        {
            pMIMEHeader->GetValueAsBuffer(pBufVal);
            HX_ASSERT(pBufVal);
            HX_RELEASE(pMIMEHeader);
        }
        INT32 len = pBufVal->GetSize();

        if (len > 0)
        {
            BYTE* outbuf = new BYTE[len]; //XXXSMP Overkill
            INT32 lActualLen = BinFrom64((const char*)pBufVal->GetBuffer(),
                len, outbuf);

            IHXBuffer* pBuffer = NULL;
            m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                  (void**)&pBuffer);
            pPacket = new CHXPacket;
            pPacket->AddRef();

            if (lActualLen < 0)
            {
                /* Don't crash if there is bad input */
                lActualLen = 0;
            }
            pBuffer->Set((const UINT8 *)outbuf, lActualLen);
            delete[] outbuf;

            pPacket->Set(pBuffer, 0, unStreamNumber, 0, 0);
            pBuffer->Release();
        }
        if (pPacket)
        {
            rc = m_pResp->HandleBackChannel(pPacket, sessionID);
            pPacket->Release();
        }
        else
        {
            rc = HXR_OK;
        }
        HX_RELEASE(pBackChannel);
    }

    UINT32 ulAddress = 0;
    UINT32 ulSourcePort = 0;
    UINT32 ulPort = 0;
    HX_RESULT theErr = HXR_ABORT;

    if (bDidSubscribe)
    {
        TransportStreamHandler* pHandler = NULL;

        theErr = m_pResp->HandleSubscriptionDone(ulAddress,
                                                 ulSourcePort,
                                                 ulPort,
                                                 sessionID,
                                                 pHandler);
        if (SUCCEEDED(theErr) && pHandler)
        {
            // this is BCM multicast!
            Transport* pTransport = pSession->getFirstTransportSetup();
            pTransport->SetStreamHandler(pHandler);
            pHandler->Release();
        }
    }

    rc = HandleStreamAdaptationHeader(sessionID, pMsg, FALSE);
    if (rc == HXR_OK)
    {
        paramOK = TRUE;
    }
    else if (rc == HXR_IGNORE)
    {
        rc = HXR_OK;
    }

    rc = Handle3GPPLinkCharHeader(sessionID, pMsg, FALSE);
    if (rc == HXR_OK)
    {
        paramOK = TRUE;
    }
    else if (rc == HXR_IGNORE)
    {
        rc = HXR_OK;
    }

    IHXMIMEHeader* pDataConvertBuffer = NULL;
    UINT32 uRespCode = 200;

    if (paramOK && rc == HXR_OK)
    {
        // If we are getting PlayerStats
        IHXMIMEHeader* pPlayerStats = 0;
        if(HXR_OK == pMsg->GetHeader("PlayerStats", pPlayerStats))
        {
            /* Per the spec, only send the StatsInterval in the response to 
             * PlayerStats if the StatsInterval has changed and this session
             * hasn't been notified.
             */

            UINT32 ulCurrent = m_pPropWatchResponse->GetStatsInterval();
            if(pSession && (pSession->m_ulStatsInterval != ulCurrent))
            {
                char buf[32];
                pSession->m_ulStatsInterval = ulCurrent;
                sprintf(buf, "%d", (int)pSession->m_ulStatsInterval);
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("UpdateStatsInterval", buf);
                m_pRespMsg->AddHeader(pMIMEHeader);
            }
            HX_RELEASE(pPlayerStats);
        }

        // don't add any of mcast headers if this is not a multicast session
        if (HXR_ABORT != theErr)
        {
            if (SUCCEEDED(theErr))        // HXR_OK == theErr
            {
                char pTemp[32];

                // XXXGo -
                // man, HXInetNtoa() is expecting a HOST byte order
                // despite it's name :)
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("MulticastIP", HXInetNtoa(ulAddress));
                m_pRespMsg->AddHeader(pMIMEHeader);

                sprintf(pTemp, "%u", (unsigned int)ulSourcePort);
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("MulticastSourcePort", pTemp);
                m_pRespMsg->AddHeader(pMIMEHeader);

                sprintf(pTemp, "%u", (unsigned int)ulPort);
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("MulticastPort", pTemp);
                m_pRespMsg->AddHeader(pMIMEHeader);

                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("MulticastValid", "1");
                m_pRespMsg->AddHeader(pMIMEHeader);
            }
            else
            {
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("MulticastValid", "0");
                m_pRespMsg->AddHeader(pMIMEHeader);
            }
        }
    }
    else if (HXR_OK == pMsg->GetHeader("DataConvertBuffer",
        pDataConvertBuffer))
    {
        HX_RELEASE(pDataConvertBuffer);

        IHXBuffer* pContent = NULL;
        pMsg->GetContent(pContent);
        HX_ASSERT(pContent);
        char* pFoo = new char[pContent->GetSize()+1];
        memcpy(pFoo, pContent->GetBuffer(), pContent->GetSize());
        *(pFoo + pContent->GetSize()) = '\0';
        HX_RELEASE(pContent);
        m_pResp->HandleSetParameterRequest(sessionID, "DataConvertBuffer",
            "1", pFoo);
        HX_VECTOR_DELETE(pFoo);
    }
    else
    {
        uRespCode = 451;
    }
    SendResponse(uRespCode);
    HX_RELEASE(pMsg);

    return HXR_OK;
}

/**
 * \brief setupPlay - kick playback by calling m_pResp->HandlePlayRequest
 *
 * \param pSession [in] : we use this to access the transports
 * \param sessionID [in] : so we can keep looking up the session in every method
 * \param ruleList [in] : not used, obsolete??? (it's never used in 
 *                        m_pResp->HandlePlayRequest either)
 * \param tBegin, tEnd [in] : play endpoints
 *
 * \return whatever is returned by m_pResp->HandlePlayRequest()
 */
HX_RESULT
RTSPServerProtocol::setupPlay(RTSPServerProtocol::Session* pSession,
    CHXString& sessionID, CHXSimpleList& ruleList, UINT64 tBegin, UINT64 tEnd)
{

    /*
     * Inactive SETUP support
     */
    HX_RESULT rc = HXR_OK;

    if (!pSession->m_bSetupsComplete)
    {
        rc = FinishAllSetups(pSession);
    }

    // all transports need to have the same time
    HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
    Timeval tv((INT32)rmatv.tv_sec, (INT32)rmatv.tv_usec);

    CHXSimpleList::Iterator i;
    for (i=pSession->m_pTransportList->Begin();
        i!=pSession->m_pTransportList->End();
        ++i)
    {
        Transport* pTrans = (Transport*)(*i);
        pTrans->playReset();
        pTrans->setFirstPlayTime(&tv);
    }

    ClearInitialTS(sessionID);

    rc = m_pResp->HandlePlayRequest(tBegin, tEnd, &ruleList, sessionID);

    return rc;
}

/**
 * \brief  OnPlayRequest - handle PLAY request from client
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnPlayRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;

    BOOL paramOK = FALSE;

    IHXRTSPMessage* pMsg = NULL;
    IHXMIMEHeader* pMIMEHeader = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXBuffer* pBufVal = NULL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    IHXList* pListParam = NULL;
    IHXListIterator* pIterParam = NULL;
    IUnknown* pUnkParam = NULL;
    IHXMIMEParameter* pParam = NULL;

    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    // The state machine prevents us from getting here without a valid
    // session header, but we check anyway just to be safe.
    rc = getSessionID(pMsg, sessionID);
    if (rc == HXR_OK)
    {
        pSession = getSession(sessionID);
    }

    OnClientRequestEvent(pReqMsg, pMsg, pSession);

    if (pSession == NULL)
    {
        // Session Not Found
        SendResponse(454);
        HX_RELEASE(pMsg);
        return HXR_FAIL;
    }

    if(!pSession->m_bSetupsComplete)
    {
        rc = FinishAllSetups(pSession);
    }

    HandleBandwidthHeader(sessionID, pMsg);

    // Validate transport mode for all stream(s)
    // and check for data-capable transports
    BOOL bIsDataCapable = FALSE;

    RTSPTransportInstantiator* pInstantiator = pSession->m_pTransportInstantiator;

    HX_ASSERT(pInstantiator);

    if (!pInstantiator->PlayCapableTransportExists())
    {
        // Bad transport mode
        SendResponse(400);
        HX_RELEASE(pMsg);
        return HXR_FAIL; // Do not change state
    }

    if (!pInstantiator->DataCapableTransportExists())
    {
            bIsDataCapable = TRUE;
    }

    /*
     * If we are currently playing, there are two cases (see RFC2326 10.5):
     *   1. Range header not present.  The request is a liveness test.  We
     *      return a 200 response.
     *   2. Range header present.  The request is for a queued play.  We do
     *      not support this feature, so we return a 501 response.  Note this
     *      is a protocol violation:
     *        "... a server MUST queue PLAY requests to be executed in order"
     *      However, this feature is not used by any known clients and it has
     *      been removed from the upcoming RTSP/1.1 spec.
     */
    State state = INIT;
    getState(pReqMsg, state);
    if (state == PLAYING)
    {
        pMsg->GetHeader("Range", pMIMEHeader);
        if (pMIMEHeader == NULL)
        {
            // Liveness test
            SendResponse(200);
        }
        else
        {
            // Queued play
            pMIMEHeader->Release();
            SendResponse(501);
        }
        HX_RELEASE(pMsg);
        return HXR_FAIL; // Do not change state
    }

    // Mark the session used
    if (pSession->m_bUnused)
    {
        pSession->m_bUnused = FALSE;
        m_nUnusedSessions--;
    }
    pSession->m_bPlayResponseDone = FALSE;

    if (!pSession->m_bRangeResponsePending)
    {
        /// \todo remove unused m_ulPlayReqCSeq from RTSP session class
        /// pSession->m_ulPlayReqCSeq = pMsg->GetCSeq();
        pSession->m_fPlayScale = 0;
        pSession->m_fPlaySpeed = 0;
        pSession->m_ulPlayRangeStart = RTSP_PLAY_RANGE_BLANK;
        pSession->m_ulStreamsEnded = 0;
    }

    /*
     * Do any of the streams require RDT?
     * Do any of the streams have wire packets?
     */
    BOOL bHasRDT = FALSE;
    BOOL bHasReflector = FALSE;
    BOOL bHasRTP = FALSE;
    BOOL bHasMulticast = FALSE;
    {
        Transport* pTransport = NULL;
        RTSPStreamInfo* pInfo = NULL;
        UINT16 uStream;

        for (uStream = 0; uStream < pSession->m_uStreamCount; uStream++)
        {
            pInfo = pSession->m_ppStreamInfo[uStream];

            if (!pSession->m_pbSETUPRcvdStrm[uStream])
            {
		continue;
            }

            HX_ASSERT(pInfo != NULL);
            if (pInfo->m_ulPayloadWirePacket)
            {
                bHasReflector = TRUE;
            }
            pTransport = pSession->getTransport(uStream);
            if (pTransport != NULL)
            {
                if (IS_RDT_TRANSPORT(pTransport->tag()))
                {
                    bHasRDT = TRUE;
                }

                if (IS_RTP_TRANSPORT(pTransport->tag()))
                {
                    bHasRTP = TRUE;
                }

                if (pTransport->tag() == RTSP_TR_RDT_MCAST)
                {
                    bHasMulticast = TRUE;
                }
            }
        }
    }

    // if reflector, we need correct timestamp from the first wire packet.
    if (bHasRTP)
    {
        pSession->m_unStreamsRTPInfoReady = 0;
        pSession->m_bRTPInfoResponsePending = TRUE;
    }
    else
    {
        pSession->m_unStreamsRTPInfoReady = pSession->m_uTotalSetupReceived;
        pSession->m_bRTPInfoResponsePending = FALSE;
    }

    /* The client must be authenticated by now */
#if defined(HELIX_FEATURE_RTSP_SERVER_CHALLENGE) || defined(HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE)
    if (bHasRDT &&
        HXR_OK != ValidateChallengeResp(pSession, sessionID, FALSE, pMsg))
    {
        /*
         * If authentication fails, and at least one of the streams in the
         * session uses an RDT transport, client is not allowed to play the
         * presentation.
         */

        SendResponse(456);
        pSession->m_bPlayResponseDone = TRUE;
        HX_RELEASE(pMsg);
        return HXR_FAIL; // Do not change state
    }
#endif /* HELIX_FEATURE_RTSP_SERVER_CHALLENGE */

    CHXSimpleList ruleList;
    IHXMIMEHeader* pSubscribe = NULL;
    if (HXR_OK == pMsg->GetHeader("Subscribe", pSubscribe))
    {
        paramOK = TRUE;
        pSubscribe->GetFieldListConst(pListField);
        HX_RELEASE(pSubscribe);
        pIterField = pListField->Begin();

        while (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField,
                (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetParamListConst(pListParam);
            HX_RELEASE(pField);
            pIterParam = pListParam->Begin();

            UINT16 streamNumber = 0xFFFF;
            UINT16 ruleNumber = 0xFFFF;

            while (pIterParam->HasItem())
            {
                pBufAttr = NULL;
                pBufVal = NULL;
                pUnkParam = pIterParam->GetItem();
                pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                    (void **)&pParam);
                HX_RELEASE(pUnkParam);
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "stream", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(),
                        pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    streamNumber = (UINT16)strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }
                if (!strncasecmp((const char *)pBufAttr->GetBuffer(),
                    "rule", pBufAttr->GetSize()))
                {
                    char* pValue = new char[pBufVal->GetSize()+1];
                    memcpy(pValue, pBufVal->GetBuffer(),
                        pBufVal->GetSize());
                    *(pValue+pBufVal->GetSize()) = '\0';
                    ruleNumber = (UINT16)strtol(pValue, 0, 10);
                    HX_DELETE(pValue);
                }

                if (ruleNumber < 0xFFFF && streamNumber < 0xFFFF)
                {
                    RTSPSubscription* pSub = new RTSPSubscription;
                    pSub->m_ruleNumber = ruleNumber;
                    pSub->m_streamNumber = streamNumber;
                    ruleList.AddTail(pSub);
                }
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
                pIterParam->MoveNext();
            }
            HX_RELEASE(pListParam);
            HX_RELEASE(pIterParam);
            pIterField->MoveNext();
        }
        HX_RELEASE(pListField);
        HX_RELEASE(pIterField);
        HX_RELEASE(pSubscribe);
        pSession->m_bHandleRules = FALSE;
    }

    HandleStreamAdaptationHeader(sessionID, pMsg, FALSE);
    Handle3GPPLinkCharHeader(sessionID, pMsg, FALSE);

    pSession->m_bNeedRTPSequenceNo = TRUE;
    BOOL bScaleHdrParsed = FALSE;
    RTSPRange::RangeType ulReqRangeUnits = RTSPRange::TR_NPT;
    UINT64 tBegin;
    UINT64 tEnd;
    float fScale = 0.0;

    if (HXR_OK == pMsg->GetHeader("Scale", pMIMEHeader))
    {
        IHXBuffer* pScale;

        if ((SUCCEEDED(rc = pMIMEHeader->GetValueAsBuffer(pScale))))
        {
            bScaleHdrParsed = TRUE;
            fScale = atof((const char*) pScale->GetBuffer());

            if (fScale == 0.0)
            {
                SendResponse(400);
                pSession->m_bPlayResponseDone = TRUE;
            }
            else
            {
                pSession->m_bScaleResponsePending = TRUE;
            }
            HX_RELEASE(pScale);
        }
        HX_RELEASE(pMIMEHeader);

        if (pSession->m_bPlayResponseDone)
        {
            return HXR_FAIL;
        }
    }

    if (HXR_OK == pMsg->GetHeader("Range", pMIMEHeader))
    {
        tBegin = RTSP_PLAY_RANGE_BLANK;
        tEnd = RTSP_PLAY_RANGE_BLANK;

        paramOK = TRUE;
        pMIMEHeader->GetFirstField(pField);
        HX_RELEASE(pMIMEHeader);

        if (pField)
        {
            pField->GetFirstParam(pParam);
            HX_RELEASE(pField);

            if (pParam)
            {
                pBufAttr = NULL;
                pBufVal = NULL;
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                rc = parseRange(pBufAttr, pBufVal, tBegin, tEnd, ulReqRangeUnits);

                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
            }
        }

        pSession->m_ulPlayRangeUnits = ulReqRangeUnits;

        // only defer the play response if we are sending data
        pSession->m_bRangeResponsePending = bIsDataCapable;

        if (SUCCEEDED(rc))
        {
            rc = setupPlay(pSession, sessionID, ruleList,
                           tBegin, tEnd);
        }
    }
    else if (!m_bPlayReceived)
    {
        tBegin = (UINT64)0;
        tEnd = RTSP_PLAY_RANGE_BLANK;

        pSession->m_bRangeResponsePending = FALSE;
        rc = setupPlay(pSession, sessionID, ruleList, tBegin, tEnd);
    }
    else
    {
        tBegin = RTSP_PLAY_RANGE_BLANK;
        tEnd = RTSP_PLAY_RANGE_BLANK;
        /*
         * No Range means this is a resume message
         */
        //XXXTDM: bad assumption, RFC 2326, s10.5 states:
        //        "A PLAY request without a Range header is legal."

        //XXXMC: Attempt to handle play requests without range headers correctly
#ifdef XXXMC_OLD_RANGE_SUPPORT
        pSession->m_bNeedRTPSequenceNo = FALSE;
        rc = m_pResp->HandleResumeRequest(sessionID);
#else
        if (m_bNoRtpInfoInResume)
        {
            pSession->m_bNeedRTPSequenceNo = FALSE;
        }
        pSession->m_bRangeResponsePending = bIsDataCapable;
        rc = setupPlay(pSession, sessionID, ruleList, tBegin, tEnd);
#endif
    }

    CHXSimpleList::Iterator ii;
    for (ii = ruleList.Begin(); ii != ruleList.End(); ++ii)
    {
        RTSPSubscription* pSub = (RTSPSubscription*)(*ii);
        delete pSub;
    }

    if (rc == HXR_PE_SESSION_NOT_FOUND)
    {
        SendResponse(455);
        pSession->m_bPlayResponseDone = TRUE;

        HX_RELEASE(pMsg);

        return HXR_OK;
    }

    if (rc == HXR_OK)
    {
        if (tBegin != 0)
        {   // Need to determine actual start point from file format
            pSession->m_ulPlayRangeStart = RTSP_PLAY_RANGE_BLANK;
        }
        else
        {
            pSession->m_ulPlayRangeStart = tBegin;
        }

        if (tEnd > 0 && tEnd != RTSP_PLAY_RANGE_BLANK)
        {
            pSession->m_ulPlayRangeEnd = tEnd;
        }
        else if (!pSession->m_bRangeResponsePending)
        {
            pSession->m_ulPlayRangeEnd = RTSP_PLAY_RANGE_BLANK;
        }

        m_bPlayReceived = TRUE;
    }
    else
    {
        SendResponse(400);
        pSession->m_bPlayResponseDone = TRUE;
    }

    if (HXR_OK == pMsg->GetHeader("Speed", pMIMEHeader))
    {
        float fSpeed = 0.0;
        IHXBuffer* pSpeed;

        if (HXR_OK == pMIMEHeader->GetValueAsBuffer(pSpeed))
        {

            fSpeed = atof((const char*) pSpeed->GetBuffer());

            m_pResp->HandleSpeedParam(sessionID,
                    HX_FLOAT_TO_FIXED(fSpeed));
            HX_RELEASE(pSpeed);
        }
        HX_RELEASE(pMIMEHeader);
    }

    if (bScaleHdrParsed)
    {
        m_pResp->HandleScaleParam(sessionID, HX_FLOAT_TO_FIXED(fScale));
    }

    HX_RELEASE(pMsg);

    /*
     * PR 178520 - Backchannel Multicast Does not Fail Over.
     *
     * Multicast transports have a funky setup with a "dummy" data pump
     * (PPM::Session or equivalent) for each actual client.  This dummy
     * data pump doesn't actually send data; therefore, the mechanism to
     * set the RTP-Info header asynchronously is not functional and a
     * PLAY response will never be sent.
     *
     * So, we just send a PLAY response synchronously with default values
     * in the RTP-Info header.  This only affects backchannel multicast 
     * (a Helix feature) so there are no interop issues with this solution.
     */
    if (bHasMulticast)
    {
        SendPlayResponse(200, pSession->m_ulPlayReqCSeq, pSession);
    }

    // if null transport session, send response now since 
    // SendPlayResponse won't get called otherwise
    if (bIsDataCapable == FALSE)
    {
        SendPlayResponse(200, pSession->m_ulPlayReqCSeq, pSession);
    }

    return HXR_FAIL; // SendPlayResponse will handle state transition
}

/**
 * \brief SetScaleDone - SetScale completed (it goes all the way up to the file format)
 *
 * SetScaleDone - SetScale operation completed (it goes all the way up to the file 
 * format). The PLAY response is gated on this.
 *
 * \param pnStatus [in] : was SetScale successful
 * \param sSessionID [in]
 * \param fScale [in] : scale to use in this session
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::SetScaleDone(HX_RESULT pnStatus, CHXString& sSessionID,
                                 FIXED32 fScale)
{
    RTSPServerProtocol::Session* pSession = getSession(sSessionID);

    if (pnStatus == HXR_OK)
    {
        pSession->m_fPlayScale = fScale;
    }
    else
    {
        pSession->m_fPlayScale = FLOAT_TO_FIXED(1.0);
    }
    pSession->m_bScaleResponsePending = FALSE;

    return (SendPlayResponse(200, pSession->m_ulPlayReqCSeq, pSession));
}

/**
 * \brief SendPlayResponse
 *
 * \param ulStatusCode [in]
 * \param ulCSeq [in] : not used, obsolete
 * \param pSession [in]
 *
 * \return HXR_OK ... unless theres something really wrong (eg. no resp msg)
 */

HX_RESULT
RTSPServerProtocol::SendPlayResponse(UINT32 ulStatusCode, UINT32 ulCSeq,
                                     RTSPServerProtocol::Session* pSession)
{
    HX_RESULT rc = HXR_OK;
    BOOL bPlayResponseDelayed = FALSE;
    IHXMIMEHeader* pMIMEHeader = NULL;

    if (pSession->m_bPlayResponseDone || m_pRespMsg == NULL)
    {
        return rc;
    }

    if (ulStatusCode == 200)
    {
        // Set Range response header ONLY for static streams
        if (!pSession->m_bIsLive)
        {
            if (pSession->m_bRangeResponsePending)
            {
                // Need to figure out actual start point from file format
                // (via ::SetStreamStartTime)
                bPlayResponseDelayed = TRUE;
            }
        }

        if (pSession->m_bScaleResponsePending ||
            pSession->m_bRTPInfoResponsePending)
        {
            bPlayResponseDelayed = TRUE;
        }

        if(!bPlayResponseDelayed)
        {
            // Add RTP-Info header
            if (pSession->m_bNeedRTPSequenceNo)
            {
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("RTP-Info",
                                           pSession->m_streamSequenceNumbers);
                m_pRespMsg->AddHeader(pMIMEHeader);
                pSession->m_streamSequenceNumbers = "";
            }

            // Add RealChallenge3 header, if needed
#if defined(HELIX_FEATURE_RTSP_SERVER_CHALLENGE) || defined(HELIX_FEATURE_RTSP_MIDBOX_CHALLENGE)
            SetChallengeHeader(pSession, RTSP_VERB_PLAY, FALSE, m_pRespMsg);
#endif /* HELIX_FEATURE_RTSP_SERVER_CHALLENGE */

            // Add Range header to static streams
            if (!pSession->m_bIsLive)
            {
                // Make sure we have valid ranges before we send these out...
                if (pSession->m_ulPlayRangeStart == RTSP_PLAY_RANGE_BLANK)
                {
                    // Shouldn't happen...
                    pSession->m_ulPlayRangeStart = 0;
                }

                RTSPRange range(pSession->m_ulPlayRangeStart,
                                pSession->m_ulPlayRangeEnd,
                                pSession->m_ulPlayRangeUnits);
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("Range",
                                           (const char*)range.asString());
                m_pRespMsg->AddHeader(pMIMEHeader);
            }

            // Add Scale header if needed
            if (pSession->m_fPlayScale != 0.0)
            {
                char szScaleValue[64];
                float fScale = FIXED_TO_FLOAT(pSession->m_fPlayScale);
                snprintf (szScaleValue, 64, "%.1f", fScale);
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("Scale", szScaleValue);
                m_pRespMsg->AddHeader(pMIMEHeader);
            }

            // Add RTCP-Interval Header
            if (m_pRTCPInterval)
            {
                pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
                pMIMEHeader->SetFromString("RTCP-Interval", m_pRTCPInterval);
                m_pRespMsg->AddHeader(pMIMEHeader);
            }

            // Add video headers if available (see PR 158176)
            Add3GPVideoHeaders (pSession);
        }
    }

    if(!bPlayResponseDelayed)
    {
        // Make transition to PLAYING state now
        HX_ASSERT(pSession->m_state == READY);
        pSession->m_state = PLAYING;

        if (pSession->m_ulInitiationID)
        {
            (*m_pPipelineMap)[pSession->m_ulInitiationID] = (void*)0;
        }

        SendResponse(ulStatusCode);
        pSession->m_bPlayResponseDone = TRUE;
    }

    return rc;
}


BOOL
RTSPServerProtocol::AddUint32MIMEHeader(const char* header, UINT32 value)
{
    IHXBuffer* pBuf = 0;
    if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
        (void**)&pBuf))
    {
        return FALSE;
    }

    pBuf->Set((BYTE *)header, strlen(header));
    CMIMEHeader* pMIMEHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pMIMEHeader->SetKey(pBuf);
    pMIMEHeader->SetValueFromUint(value);

    m_pRespMsg->AddHeader(pMIMEHeader);
    HX_RELEASE(pBuf);

    return TRUE;
}

void
RTSPServerProtocol::Add3GPVideoHeaders(RTSPServerProtocol::Session* pSession)
{
    BOOL bAddedPreDecBufSize = FALSE;
    BOOL bAddedInitPreDecBufPeriod = FALSE;
    BOOL bAddedInitPostDecBufPeriod = FALSE;
    BOOL bAddedVideoPostDecBufPeriod = FALSE;

    for (UINT16 uStream = 0; uStream < pSession->m_uStreamCount; uStream++)
    {
        HX_ASSERT(pSession->m_ppStreamInfo[uStream] == NULL ||
                  pSession->m_ppStreamInfo[uStream]->m_streamNumber == uStream);
        if (pSession->m_pbSETUPRcvdStrm[uStream] && pSession->m_ppStreamInfo &&
            pSession->m_ppStreamInfo[uStream]->m_pStreamHeader)
        {
            RTSPStreamInfo* pInfo = pSession->m_ppStreamInfo[uStream];
            IHXValues* pHeader = pInfo->m_pStreamHeader;

            UINT32 ulPreDecBufSize;
            if (!bAddedPreDecBufSize &&
                pHeader->GetPropertyULONG32("X-PreDecBufSize", ulPreDecBufSize) == HXR_OK)
            {
                bAddedPreDecBufSize =
                    AddUint32MIMEHeader("x-predecbufsize", ulPreDecBufSize);
            }

            UINT32 ulInitPreDecBufPeriod;
            if (!bAddedInitPreDecBufPeriod &&
                pHeader->GetPropertyULONG32("X-InitPreDecBufPeriod", ulInitPreDecBufPeriod) == HXR_OK)
            {
                bAddedInitPreDecBufPeriod =
                    AddUint32MIMEHeader("x-initpredecbufperiod", ulInitPreDecBufPeriod);
            }

            UINT32 ulInitPostDecBufPeriod;
            if (!bAddedInitPostDecBufPeriod &&
                pHeader->GetPropertyULONG32("X-InitPostDecBufPeriod", ulInitPostDecBufPeriod) == HXR_OK)
            {
                bAddedInitPostDecBufPeriod = 
                    AddUint32MIMEHeader("x-initpostdecbufperiod", ulInitPostDecBufPeriod);
            }

            UINT32 ulVideoPostDecBufSize;
            if (!bAddedVideoPostDecBufPeriod &&
                pHeader->GetPropertyULONG32("3GPP-VideoPostDecBufSize", ulVideoPostDecBufSize) == HXR_OK)
            {
                bAddedVideoPostDecBufPeriod = 
                    AddUint32MIMEHeader("3gpp-videopostdecbufsize", ulVideoPostDecBufSize);
            }
        }
    }
}

/**
 * \brief  OnPauseRequest - handle PAUSE request from client
 *
 *
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnPauseRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;

    IHXRTSPMessage* pMsg = 0;
    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    // The state machine prevents us from getting here without a valid
    // session header, but we check anyway just to be safe.
    rc = getSessionID(pMsg, sessionID);
    if (rc == HXR_OK)
    {
        pSession = getSession(sessionID);
    }

    OnClientRequestEvent(pReqMsg, pMsg, pSession);

    if (pSession == NULL)
    {
        // Session Not Found
        SendResponse(454);
        HX_RELEASE(pMsg);
        return HXR_FAIL;
    }

    // If we are already paused, just nod and smile
    if (pSession->m_state == READY)
    {
        SendResponse(200);
        HX_RELEASE(pMsg);
        return HXR_OK;
    }

    // Handle range headers in pause requests
    UINT64 ulPausePoint = 0;
    IHXMIMEHeader* pMIMEHdr = 0;
    BOOL bRangeHdrParsed = FALSE;

    rc = HXR_OK;

    if (HXR_OK == pMsg->GetHeader("Range", pMIMEHdr))
    {
        bRangeHdrParsed = TRUE;
        IHXMIMEField* pField = NULL;
        IHXMIMEParameter* pParam = NULL;

        pMIMEHdr->GetFirstField(pField);
        HX_RELEASE(pMIMEHdr);

        if (pField)
        {
            pField->GetFirstParam(pParam);
            HX_RELEASE(pField);
        }

        if (pParam)
        {
            RTSPRange::RangeType ulUnits;
            UINT64 ulTmp;
            IHXBuffer* pBufVal = NULL;
            IHXBuffer* pBufAttr = NULL;

            pParam->Get(pBufAttr, pBufVal);
            HX_RELEASE(pParam);
            rc = parseRange(pBufAttr, pBufVal, ulPausePoint, ulTmp, ulUnits);
        }
        else
        {
            rc = HXR_FAIL;
        }
    }

    if (bRangeHdrParsed && (SUCCEEDED(rc)))
    {
        // Verify pause range is valid
        if ((pSession->m_ulPlayRangeStart != RTSP_PLAY_RANGE_BLANK &&
             ulPausePoint < pSession->m_ulPlayRangeStart) ||
            (pSession->m_ulPlayRangeEnd != RTSP_PLAY_RANGE_BLANK &&
             ulPausePoint > pSession->m_ulPlayRangeEnd))
        {
            rc = HXR_FAIL;
        }
    }

    if (FAILED(rc))
    {
        // Send "invalid range header" response
        SendResponse(457);
        HX_RELEASE(pMsg);
        return HXR_FAIL; // Do not change state
    }

    if (SUCCEEDED(rc))
    {
        HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
        Timeval tv((INT32)rmatv.tv_sec, (INT32)rmatv.tv_usec);

        CHXSimpleList::Iterator i;
        for (i=pSession->m_pTransportList->Begin();
             i!=pSession->m_pTransportList->End();
             ++i)
        {
            Transport* pTrans = (Transport*)(*i);
            pTrans->OnPause(&tv);
        }
    }

    IHXRTSPServerPauseResponse* m_pPauseRespHandler = NULL;
    if (bRangeHdrParsed &&
        (m_pResp->QueryInterface(IID_IHXRTSPServerPauseResponse,
                                (void **)&m_pPauseRespHandler) == HXR_OK))
    {
        rc = m_pPauseRespHandler->HandlePauseRequest(sessionID, ulPausePoint);
        HX_RELEASE(m_pPauseRespHandler);
    }
    else
    {
        rc = m_pResp->HandlePauseRequest(sessionID);
    }

    if (rc == HXR_OK)
    {
        SendResponse(200);
    }
    else
    {
        // XXXTDM: 404 is a lie.  Use a better code here, perhaps 500.
        SendResponse(404);
    }
    HX_RELEASE(pMsg);
    return rc;
}

/**
 * \brief  OnTeardownRequest - handle TEARDOWN request from client
 *
 *
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnTeardownRequest(IHXRTSPRequestMessage* pReqMsg)
{
    IHXRTSPMessage* pMsg = NULL;
    IHXMIMEHeader* pStatsReqMimeType = NULL;

    CHXString sessionID;
    RTSPServerProtocol::Session* pSession = NULL;
    HX_RESULT rc = HXR_OK;
    INT64  lBytesSent=0;
    UINT32 ulPacketsSent=0;
    BOOL   bSendStats=FALSE;

    //XXXBAB - need to clear stream descriptions, ports, anything
    // concerned with this session
    // Not incredibly important yet since RMPlayer disconnects
    // and the protocol is destroyed, but for later...

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);
    pMsg->ReplaceDelimiters(TRUE, '\0');

    rc = getSessionID(pMsg, sessionID);
    if (rc == HXR_OK)
    {
        pSession = getSession(sessionID);
    }

    OnClientRequestEvent(pReqMsg, pMsg, pSession);

    if (pSession == NULL)
    {
        SendResponse(454);
        HX_RELEASE(pMsg);
        return HXR_FAIL;
    }

    if (HXR_OK == pMsg->GetHeader("DirectToClientStats",
        pStatsReqMimeType))
    {
        UINT32 transport = 0;
        pStatsReqMimeType->GetValueAsUint(transport);
        pStatsReqMimeType->Release();

        Transport* pTransport;
        pTransport = pSession->getTransport((UINT16)transport);
        if (pTransport)
        {
             lBytesSent = pTransport->getBytesSent();
             ulPacketsSent = pTransport->getPacketsSent();
             bSendStats = TRUE;
        }
    }

    // This never fails, but if it does, we still remove the session
    rc = m_pResp->HandleTeardownRequest(sessionID);
    m_pSessionManager->removeSessionInstance(sessionID);

    IHXMIMEHeader* pMIMEHdr = 0;
    if (bSendStats)
    {
        char szTemp[32];

        i64toa(lBytesSent, szTemp, 10);
        pMIMEHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pMIMEHdr->SetFromString("DirectToClientBytesSent", szTemp);
        m_pRespMsg->AddHeader(pMIMEHdr);

        sprintf(szTemp, "%lu", ulPacketsSent);
        pMIMEHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pMIMEHdr->SetFromString("DirectToClientPacketsSent", szTemp);
        m_pRespMsg->AddHeader(pMIMEHdr);
    }

    //XXXDPL hack
    //TEARDOWN response doesn't include a "Session: " header, so we need to do
    //session correlation here. SendResponse() will attempt to update response
    //events again but will be unable to find the "Session: " header. Thus it
    //will attempt to (and fail to) find the TEARDOWN requestinfo object in
    // the top level stats object.
    if (m_bTrackEvents)
    {
        HX_ASSERT(pSession);
        pSession->m_pEventList->UpdateResponseStats(200, pMsg->GetCSeq(), TRUE);
    }

    SendResponse(200);

    HX_RELEASE(pMsg);

    return HXR_OK;
}

/**
 * \brief  OnRedirectRequest - handle REDIRECT request from client
 *
 * Currently, we always return "501 Not Implemented" to a REDIRECT
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnRedirectRequest(IHXRTSPRequestMessage* pReqMsg)
{
    IHXRTSPMessage* pMsg = NULL;
    RTSPServerProtocol::Session* pSession = NULL;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    OnClientRequestEvent(pReqMsg, pMsg, pSession);

    // 501 Not Implemented
    SendResponse(501);

    HX_RELEASE(pMsg);

    return HXR_OK;
}


/**
 * \brief  OnPlaynowRequest - handle PLAYNOW request from client
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnPlaynowRequest(IHXRTSPRequestMessage* pReqMsg)
{
    HX_RESULT rc = HXR_OK;

    IHXRTSPMessage* pMsg = 0;
    CHXString oldSessionID;
    CHXString newSessionID;
    RTSPServerProtocol::Session* pOldSession = NULL;
    RTSPServerProtocol::Session* pNewSession = NULL;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    // The state machine prevents us from getting here without a valid
    // session header, but we check anyway just to be safe.
    rc = getSessionID(pMsg, oldSessionID);
    if (rc == HXR_OK)
    {
        pOldSession = getSession(oldSessionID);
    }

    OnClientRequestEvent(pReqMsg, pMsg, pOldSession);

    if (pOldSession == NULL)
    {
        // Session Not Found
        SendResponse(454);
        HX_RELEASE(pMsg);
        return HXR_FAIL;
    }

    IHXValues* pRequestHeaders = NULL;
    GetAndConvertRFC822Headers(pMsg, pRequestHeaders);

    if (!pOldSession->m_pTransportInstantiator || 
        !pOldSession->m_pTransportInstantiator->IsAggregateTransport())
    {
        /// "455 Method Not Valid in This State"
        SendResponse(455);
        HX_RELEASE(pMsg);
        HX_RELEASE(pRequestHeaders);
        return HXR_FAIL;
    }

    /// Save the aggregate transport parameters (with sockets)
    SPRTSPTransportParams spAggParams;
    pOldSession->m_pTransportInstantiator->GetTransportParams(spAggParams.AsInOutParam());

    /// Start the teardown process on the old session
#if 0
    printf("    tearing down session %p\n", pOldSession);
#endif

    /// This never fails, but if it does, we still remove the session
    m_pResp->HandleTeardownRequest(oldSessionID);
    m_pSessionManager->removeSessionInstance(oldSessionID);

    /// Lets go get the file/stream headers for the new session
    IHXBuffer* pUrlBuf = NULL;
    pReqMsg->GetUrl(pUrlBuf);
    NEW_FAST_TEMP_STR(pDecURL, 1024, pUrlBuf->GetSize()+1);
    DecodeURL(pUrlBuf->GetBuffer(), pUrlBuf->GetSize(), pDecURL);
    m_pResp->AddSession(pDecURL, pMsg->GetCSeq(), newSessionID, TRUE);

    HX_RELEASE(pUrlBuf);

    pNewSession = getSession(newSessionID, TRUE);
    HX_ASSERT(pNewSession != NULL); // Response MUST create a session

    m_pRespMsg->RemoveHeader("Session");
    pNewSession->AddSessionHeader(m_pRespMsg);

    pNewSession->clearDescribeMimeTypeList();
    pNewSession->clearStreamInfoList();

    // store these for use by RTSPProtocol::SendPlayResponse()
    pNewSession->m_RTSPMessageTagOriginating = pReqMsg->GetMethod();

    /// Look for the "Aggregate-Transport" header now
    IHXMIMEHeader* pTransportMIMEType = NULL;

    rc = pMsg->GetHeader("Aggregate-Transport", pTransportMIMEType);

    if (SUCCEEDED(rc))
    {
        /** we can't decide whether the old aggregate transport matches 
          * one of the transports in the header until we have the file/stream
          * headers so we save both the existing aggregate and the parsed
          * parameters */
        RTSPTransportInstantiator* pInstantiator;

        pInstantiator = new RTSPTransportInstantiator(TRUE);
        pNewSession->m_pTransportInstantiator = pInstantiator;
        pInstantiator->AddRef();
        pInstantiator->Init(m_pContext, this);

        rc = pInstantiator->parseTransportHeader(pTransportMIMEType);

        HX_RELEASE(pTransportMIMEType);

        if (SUCCEEDED(rc))
        {
            /** The new session gets an addrefed pointer to the aggregate
              * transport parameters. We won't know if we can re-use them
              * until the file/stream headers come in. */
            spAggParams.AsPtr(&pNewSession->m_pAggregateTransportParams);
        }
    }

    if (!SUCCEEDED(rc))
    {
        SendResponse(400);
        HX_RELEASE(pMsg);
        HX_RELEASE(pRequestHeaders);
        DELETE_FAST_TEMP_STR(pDecURL);

        return HXR_FAIL;
    }
 
    IHXMIMEHeader* pMIMEHeader = NULL;

    RTSPRange::RangeType ulReqRangeUnits = RTSPRange::TR_NPT;
    UINT64 tBegin;
    UINT64 tEnd;

    if (HXR_OK == pMsg->GetHeader("Range", pMIMEHeader))
    {
        IHXMIMEField* pField = NULL;
        IHXMIMEParameter* pParam = NULL;

        tBegin = RTSP_PLAY_RANGE_BLANK;
        tEnd = RTSP_PLAY_RANGE_BLANK;

        pMIMEHeader->GetFirstField(pField);
        HX_RELEASE(pMIMEHeader);

        if (pField)
        {
            pField->GetFirstParam(pParam);
            HX_RELEASE(pField);

            if (pParam)
            {
                IHXBuffer* pBufAttr;
                IHXBuffer* pBufVal;

                pBufAttr = NULL;
                pBufVal = NULL;
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                rc = parseRange(pBufAttr, pBufVal, tBegin, tEnd, ulReqRangeUnits);

                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
            }
        }

        /** with PLAYNOW we don't call "setupPlay" until we get file/stream
          * headers, so we store the Range parameters now. */
        pNewSession->m_bRangeResponsePending = TRUE;
        pNewSession->m_ulPlayRangeUnits = ulReqRangeUnits;
        pNewSession->m_ulPlayRangeStart = tBegin;
        pNewSession->m_ulPlayRangeEnd = tEnd;
    }
    else
    {

        pNewSession->m_bRangeResponsePending = FALSE;

        /** with PLAYNOW we don't call "setupPlay" until we get file/stream
          * headers, so we store the Range parameters now. */
        pNewSession->m_ulPlayRangeUnits = ulReqRangeUnits;
        pNewSession->m_ulPlayRangeStart = (UINT64)0;
        pNewSession->m_ulPlayRangeEnd = RTSP_PLAY_RANGE_BLANK;
    }

    float fScale = 0.0;
    pNewSession->m_fPlayScale = 0;
    pNewSession->m_fPlaySpeed = 0;

    if ((rc == HXR_OK) && (HXR_OK == pMsg->GetHeader("Scale", pMIMEHeader)))
    {
        IHXBuffer* pScale;

        if ((SUCCEEDED(rc = pMIMEHeader->GetValueAsBuffer(pScale))))
        {
            fScale = atof((const char*) pScale->GetBuffer());

            if (fScale > 0.0)
            {
                pNewSession->m_fPlayScale = FLOAT_TO_FIXED(fScale);
            }
            HX_RELEASE(pScale);
        }
        HX_RELEASE(pMIMEHeader);
    }

    if (HXR_OK == pMsg->GetHeader("Speed", pMIMEHeader))
    {
        float fSpeed = 0.0;
        IHXBuffer* pSpeed;

        if (HXR_OK == pMIMEHeader->GetValueAsBuffer(pSpeed))
        {

            fSpeed = atof((const char*) pSpeed->GetBuffer());

            /** must be called after FinishAllSetups() - because we have to 
              * have completed our source setup in the player to do this */
            pNewSession->m_fPlaySpeed = HX_FLOAT_TO_FIXED(fSpeed);
            HX_RELEASE(pSpeed);
        }
        HX_RELEASE(pMIMEHeader);
    }

    // Session setup needs the base url. For PLAYNOW, this is the same
    // as the message's url, which we already have
    BOOL bRTPAvailable;

    rc = _BeginSetup(pMsg, pDecURL, newSessionID, bRTPAvailable);

    if (SUCCEEDED(rc))
    {
        HandleStreamAdaptationHeader(newSessionID, pMsg, FALSE);
        Handle3GPPLinkCharHeader(newSessionID, pMsg, FALSE);
        HandleBandwidthHeader(newSessionID, pMsg);

        // (Shares code with the DESCRIBE method until
        // you get to RTSPServerProtocol::_PlaynowSecondStage())
        rc = m_pResp->HandleStreamDescriptionRequest(pDecURL,
                    pRequestHeaders, newSessionID, bRTPAvailable);

        IHXBuffer* pID = NULL;
        HX_RESULT ret = pRequestHeaders->GetPropertyCString("ConnID", pID);
        if (SUCCEEDED(ret) && pID)
        {
            m_ulRegistryConnId = atoi((char*)pID->GetBuffer());
        }
        HX_RELEASE(pID);

        ret = pRequestHeaders->GetPropertyCString("SessionNumber", pID);
        if (SUCCEEDED(ret) && pID)
        {
            pNewSession->m_ulSessionRegistryNumber = atoi((char*)pID->GetBuffer());
        }
        HX_RELEASE(pID);
    }

    HX_RELEASE(pMsg);
    HX_RELEASE(pRequestHeaders);
    DELETE_FAST_TEMP_STR(pDecURL);

    return rc;
}


/**
 * \brief  OnExtensionRequest - handle RTSP extension request from client
 *
 * We have received an unrecognized request from a client. While this is
 * a legal RTSP request, we don't handle it at the moment, so return
 * "501 Not Implemented"
 *
 * \param pReqMsg [in] : parsed request message
 *
 * \return HXR_OK if processed, otherwise error code
 */
HX_RESULT
RTSPServerProtocol::OnExtensionRequest(IHXRTSPRequestMessage* pReqMsg)
{
    IHXRTSPMessage* pMsg = NULL;
    RTSPServerProtocol::Session* pSession = NULL;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);

    OnClientRequestEvent(pReqMsg, pMsg, pSession);

    // 501 Not Implemented
    SendResponse(501);

    HX_RELEASE(pMsg);

    return HXR_OK;
}

/**
 * \brief _BeginSetup - stores the base URL for SETUP process
 *
 * _BeginSetup - stores the base URL for SETUP process and does some
 * other basic work like parsing the Accept and Accept-Encoding headers
 * if present. _BeginSetup will generally be called when the DESCRIBE
 * request is received for Helix clients but can be invoked at SETUP time.
 *
 * Note that the RTSP session is always created before this is called. So
 * we are guaranteed that we have a valid session object and session ID.
 *
 * Note also : no state changes are triggered in the method.
 *
 * \param pRTSPMessageStart [in] : parsed incoming RTSP request from client
 * \param pCharUrl [in] : base URL from request
 * \param strSessionID [in] : session ID for this session
 * \param bRTPAvailable [out] : set TRUE iff "Accept" header not present
 *
 * \return 
 *
 * \todo : document usage of bRTPAvailable parameter or obsolete it.
 */
HX_RESULT
RTSPServerProtocol::_BeginSetup(IHXRTSPMessage* pRTSPMessageStart,
        const char* pCharUrl, CHXString& strSessionID,
        BOOL& bRTPAvailable)
{
    RTSPServerProtocol::Session* pSession = NULL;

    pSession = getSession(strSessionID);
    HX_ASSERT(pSession != NULL);

    // If the client is a midbox, let the response know
    if (pSession->m_bIsMidBox)
    {
        m_pResp->SetMidBox(strSessionID, TRUE);
    }

    /*
     * Get the request headers
     */

    IHXMIMEHeader* pMIMEHeader = NULL;
    IHXRTSPMessage* pMsg = NULL;
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    IHXList* pListParam = NULL;
    IHXListIterator* pIterParam = NULL;
    IUnknown* pUnkParam = NULL;
    IHXMIMEParameter* pParam = NULL;

    pRTSPMessageStart->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);
    pMsg->ReplaceDelimiters(TRUE, '\0');

    if (HXR_OK != pMsg->GetHeader("Accept", pMIMEHeader))
    {
        // default to application/sdp
        pSession->m_describeMimeTypeList.AddTail(
            new CHXString("application/sdp"));

        bRTPAvailable = TRUE;
    }
    else
    {
        pMIMEHeader->GetFieldListConst(pListField);
        pIterField = pListField->Begin();
        HX_RELEASE(pListField);

        while (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetParamListConst(pListParam);
            HX_RELEASE(pField);
            pIterParam = pListParam->Begin();
            HX_RELEASE(pListParam);

            while (pIterParam->HasItem())
            {
                pBufAttr = 0;
                pBufVal = 0;
                pUnkParam = pIterParam->GetItem();
                pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                    (void **)&pParam);
                HX_RELEASE(pUnkParam);
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                HX_ASSERT(pBufAttr != NULL); // guaranteed by MIMEParameter
                CHXString* pStr;
                pStr = new CHXString((const char*)pBufAttr->GetBuffer(),
                                     (int)pBufAttr->GetSize());
                pSession->m_describeMimeTypeList.AddTail(pStr);

                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
                pIterParam->MoveNext();
            }
            HX_RELEASE(pIterParam);
            pIterField->MoveNext();
        }
        HX_RELEASE(pIterField);
    }
    HX_RELEASE(pMIMEHeader);

    if (HXR_OK == pMsg->GetHeader("Accept-Encoding", pMIMEHeader))
    {
        pMIMEHeader->GetFieldListConst(pListField);
        pIterField = pListField->Begin();
        HX_RELEASE(pListField);

        while (pIterField->HasItem())
        {
            pUnkField = pIterField->GetItem();
            pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
            HX_RELEASE(pUnkField);
            pField->GetParamListConst(pListParam);
            HX_RELEASE(pField);
            pIterParam = pListParam->Begin();
            HX_RELEASE(pListParam);

            while (pIterParam->HasItem())
            {
                pBufAttr = 0;
                pBufVal = 0;
                pUnkParam = pIterParam->GetItem();
                pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                    (void **)&pParam);
                HX_RELEASE(pUnkParam);
                pParam->Get(pBufAttr, pBufVal);
                HX_RELEASE(pParam);

                const char* str = (const char *)(pBufVal ? pBufVal->GetBuffer()
                    : pBufAttr->GetBuffer());
                UINT32 len = pBufVal ? pBufVal->GetSize() : pBufAttr->GetSize();
                if (!strncasecmp("mei", str, len))
                {
                    pSession->m_bBlockTransfer = TRUE;
                }
                HX_RELEASE(pBufAttr);
                HX_RELEASE(pBufVal);
                pIterParam->MoveNext();
            }
            HX_RELEASE(pIterParam);
            pIterField->MoveNext();
        }
        HX_RELEASE(pIterField);
    }
    HX_RELEASE(pMIMEHeader);
    HX_RELEASE(pMsg);

    pSession->m_describeURL = pCharUrl;

    return HXR_OK;
}

/**
 * \brief IsInHeaderValues - search in a given header for a given string
 *
 * Iterate thru the specific (pSearchInHeader) header for the value
 * (pSearchForValue). return TRUE if found and FALSE otherwise.
 *
 * The search string and the header value should both have same len
 * (excluding the '\0' at the end of the string.
 *
 * \param pMessageToSearch [in] : RTSP message to search
 * \param pSearchInHeader [in]
 * \param pSearchForValue [in]
 *
 * \return TRUE if found
 */
BOOL
RTSPServerProtocol::IsInHeaderValues(IHXRTSPMessage* pMessageToSearch,
    const char* pSearchInHeader, const char* pSearchForValue)
{
    IHXMIMEHeader* pMIMEHeader = NULL;
    IHXRTSPMessage* pMsg = NULL;
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    BOOL bFound = FALSE;

    pMessageToSearch->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);

    if (HXR_OK != pMsg->GetHeader(pSearchInHeader, pMIMEHeader))
    {
        pMsg->Release();
        return bFound;
    }

    UINT32 ulSearchValueLen = strlen(pSearchForValue);
    IHXList* pListField;
    IHXListIterator* pIterField;
    IUnknown* pUnkField;
    IHXMIMEField* pField;

    IHXList* pListParam;
    IHXListIterator* pIterParam;
    IUnknown* pUnkParam;
    IHXMIMEParameter* pParam;

    pMIMEHeader->GetFieldListConst(pListField);
    HX_RELEASE(pMIMEHeader);
    pIterField = pListField->Begin();
    HX_RELEASE(pListField);

    while (pIterField->HasItem())
    {
        pUnkField = pIterField->GetItem();
        pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
        HX_RELEASE(pUnkField);
        pField->GetParamListConst(pListParam);
        HX_RELEASE(pField);
        pIterParam = pListParam->Begin();
        HX_RELEASE(pListParam);

        while (pIterParam->HasItem())
        {
            pBufAttr = NULL;
            pBufVal = NULL;
            pUnkParam = pIterParam->GetItem();
            pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                (void **)&pParam);
            HX_RELEASE(pUnkParam);
            pParam->Get(pBufAttr, pBufVal);
            HX_RELEASE(pParam);

            // If this option is supported
            UINT32 ulLen = pBufVal ? pBufVal->GetSize() : pBufAttr->GetSize();
            if (ulLen == ulSearchValueLen &&
                !strncasecmp(pSearchForValue,
                    (const char *)(pBufVal ? pBufVal->GetBuffer()
                    : pBufAttr->GetBuffer()), ulLen))
            {
                bFound = TRUE;
                break;
            }

            HX_RELEASE(pBufAttr);
            HX_RELEASE(pBufVal);
            pIterParam->MoveNext();
        }
        HX_RELEASE(pBufAttr);
        HX_RELEASE(pBufVal);
        HX_RELEASE(pIterParam);
        if (bFound)
            break;
        pIterField->MoveNext();
    }
    HX_RELEASE(pIterField);
    HX_RELEASE(pMsg);

    return bFound;
}

/**
 *  \todo ParseRTPInfoField appears to be unused! Can we obsolete it?
 *
 * XXX: aak -- had top inherit from RTSPBaseProtocol and change for the new
 *  rtsp parser, so that the impact of the code change is limited to just
 *  this file.
 *  The grammer in RFC2326, I think, is wrong....But in any case, there is no
 *  gurantee seq_no and rtptime are present in RTP-Info.
 */
RTPInfoEnum
RTSPServerProtocol::ParseRTPInfoField(IHXMIMEField* pSeqValue,
    UINT16& streamID, UINT16& seqNum, UINT32& ulTimestamp,
    const char*& pControl)
{
    BOOL bFoundSeqNo = FALSE;
    BOOL bFoundRTPTime = FALSE;

    IHXList* pListParam = NULL;
    IHXListIterator* pIterParam = NULL;
    IUnknown* pUnkParam = NULL;
    IHXMIMEParameter* pParam = NULL;
    IHXBuffer* pAttr = NULL;
    IHXBuffer* pVal = NULL;

    pSeqValue->GetParamListConst(pListParam);
    pIterParam = pListParam->Begin();
    HX_RELEASE(pListParam);

    while (pIterParam->HasItem())
    {
        pUnkParam = pIterParam->GetItem();
        pUnkParam->QueryInterface(IID_IHXMIMEParameter,
            (void **)&pParam);
        HX_RELEASE(pUnkParam);

        pParam->Get(pAttr, pVal);
        HX_RELEASE(pParam);

        if (pAttr->GetSize() >= 3 && !strncasecmp("url",
            (const char *)pAttr->GetBuffer(), 3))
        {
            // Note: We don't currently do anything with the first section
            // of the "url" attribute (the actual player-requested URL). If
            // we ever do, please note that all ';' characters were escaped
            // by the server when this message was created, because ';' has
            // special meaning as a delimiter in this message. Remember to
            // unescape all instances of "%3b" to ";" before using the URL.

            const char* pUrl = (const char*) pVal->GetBuffer();
            const char* pEq  = StrNRChr(pUrl, '=', pVal->GetSize());
            if (pEq != NULL)
            {
                streamID = (UINT16)strtol(pEq + 1, 0, 10);
            }

            // take the control string...
            pControl = (const char *)pUrl;
        }
        else if (pAttr->GetSize() >= 3 && !strncasecmp("seq",
            (const char *)pAttr->GetBuffer(), 3))
        {
            bFoundSeqNo = TRUE;
            seqNum = (UINT16)strtol((const char*)pVal->GetBuffer(),0,10);
        }
        else if (pAttr->GetSize() >= 7 && !strncasecmp("rtptime",
            (const char *)pAttr->GetBuffer(), 7))
        {
            bFoundRTPTime = TRUE;
            ulTimestamp = (UINT32)strtol((const char*)pVal->GetBuffer(), 0, 10);
        }
        pIterParam->MoveNext();
        HX_RELEASE(pAttr);
        HX_RELEASE(pVal);
    }
    HX_RELEASE(pIterParam);

    if (bFoundSeqNo)
    {
        if (bFoundRTPTime)
        {
            return RTPINFO_SEQ_RTPTIME;
        }

        return RTPINFO_SEQ;
    }

    if (bFoundRTPTime)
    {
        return RTPINFO_RTPTIME;
    }

    return RTPINFO_SEQ;
}

/**
 * \brief CanCreateSession - limit sessions per control channel
 *
 * CanCreateSession protects against DoS attack by limiting the number of 
 * unused sessions on any given control channel. The specific algorithm 
 * is somewhat arbitrary.
 *
 * \return TRUE if we can create the session, FALSE if not.
 */
BOOL
RTSPServerProtocol::CanCreateSession(void)
{
    /*
     * Refuse session creation iff:
     *  - There are more than 256 sessions on this socket, and
     *  - More than 90% of them are unused (no PLAY or RECORD)
     */
    int nSessions = m_pSessions->GetCount();
    if (nSessions > 256)
    {
        if (m_nUnusedSessions*100/nSessions > 90)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * \brief DispatchMessage - handle a parsed RTSP message.
 *
 * DispatchMessage - at this point we have a parsed RTSP request, response or
 * interleaved data packet. If its a request we sent it through our state
 * machine and respond. If its a response, we look up the request that triggered
 * it on the sent-requests queue. Interleaved data packets get routed to the
 * appropriate transport by the channel number.
 *
 * \param n/a
 *
 * \return void
 */
void
RTSPServerProtocol::DispatchMessage(void)
{
    IHXRTSPInterleavedPacket*   pPkt = NULL;
    IHXRTSPMessage*             pMsg = NULL;
    IHXRTSPRequestMessage*      pReq = NULL;
    IHXRTSPResponseMessage*     pRsp = NULL;
    IHXBuffer*                  pBuf = NULL;
    UINT32                      uCSeq = 0;
    HX_RESULT rc = HXR_OK;

    HX_ASSERT(m_pConsumer != NULL);

    if (m_pConsumer->QueryInterface(IID_IHXRTSPInterleavedPacket,
                                    (void**)&pPkt) == HXR_OK)
    {
        BYTE byChan;
        IHXBuffer* pPktBuf;
        char* pSessionID = 0;

        pPkt->Get(byChan, pPktBuf);
        if (m_pSessionIDList->Lookup(byChan, (void*&)pSessionID))
        {
            RTSPServerProtocol::Session* pSession = getSession(pSessionID);

            if (pSession != NULL)
            {
                // keepalive reset
                pSession->SetSessionActiveStamp();

                pSession->handleTCPData(pPktBuf, byChan);
            }
            else
            {
                char logMsg[80];
                snprintf(logMsg, 80, "couldn't get session for <%s>\n", pSessionID);
                m_pErrorMessages->Report(HXLOG_ERR, 0, 0, logMsg, NULL);
            }
        }
        else
        {
            char logMsg[80];
            sprintf(logMsg, "couldn't lookup session for channel <0x%x>\n",
                byChan);
            m_pErrorMessages->Report(HXLOG_ERR, 0, 0, logMsg, NULL);
        }

        pPktBuf->Release();
        pPkt->Release();
        return;
    }

    if (HXR_OK ==
        m_pConsumer->QueryInterface(IID_IHXRTSPRequestMessage, (void**)&pReq))
    {
        if (HXR_OK != pReq->QueryInterface(IID_IHXRTSPMessage, (void**)&pMsg))
        {
            HX_RELEASE(pReq);
            HX_ASSERT(FALSE); //Per Tommy, this should never happen
            return;
        }

        m_pConsumer->AsBuffer(pBuf);
        handleDebug(pBuf, TRUE);
        handleTiming(pBuf, TRUE);
        uCSeq = pMsg->GetCSeq();
        HX_RELEASE(pBuf);

        // Find session, if any
        CHXString sessionID;
        RTSPServerProtocol::Session* pSession = NULL;
        if (HXR_OK == getSessionID(pMsg, sessionID))
        {
            pSession = getSession(sessionID);
        }

        // Find state and verb
        State state = INIT;
        UINT32 verb;
        getState(pReq, state);
        verb = pReq->GetVerb();
        HX_ASSERT(verb > RTSP_VERB_NONE && verb <= RTSP_VERB_EXTENSION);

        HX_ASSERT(m_pRespMsg == NULL);

        // Create a response message in m_pRespMsg
        IHXRTSPResponseMessage* pResp =
            new (m_pFastAlloc) CRTSPResponseMessage(m_pFastAlloc);
        pResp->QueryInterface(IID_IHXRTSPMessage, (void**)&m_pRespMsg);

        IHXMIMEHeader* pHdr;

        m_LastRequestMethod = pReq->GetMethod();

        // Copy CSeq header
        if (pMsg->GetHeader("CSeq", pHdr) == HXR_OK)
        {
           m_pRespMsg->AddHeader(pHdr);
           pHdr->Release();
        }

        // Copy Timestamp header
        if (pMsg->GetHeader("Timestamp", pHdr) == HXR_OK)
        {
            m_pRespMsg->AddHeader(pHdr);
            pHdr->Release();
        }

        // Set Date header
        UTCTimeRep utcNow;
        pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pHdr->SetFromString("Date", utcNow.asRFC1123String());
        m_pRespMsg->AddHeader(pHdr);

        // Handle keepalive and set Session header if needed
        if (pSession != NULL)
        {
            // Reset keepalive
            pSession->SetSessionActiveStamp();

            if (verb != RTSP_VERB_TEARDOWN)
            {
                pSession->AddSessionHeader(m_pRespMsg);
            }
        }

        /*
         * Check RTSP version
         *
         * This has the potential to get really complicated if/when a new
         * RTSP version spec is released.  Some considerations are:
         *
         *  - Our clients will currently disconnect if they get a response
         *    with a version != 1.0.
         *  - A client's RTSP version can only be tracked on a session level.
         *    If we receive a request without a Session header, we don't know
         *    the client's version.
         *  - If we need to send a request without a Session header (eg. for
         *    keepalive), we don't know the client's version.
         *  - Proxies make the issue twice as complex.
         *
         * For now, we only support RTSP/1.0, so the version handling is
         * pretty straightforward.  We just accept any RTSP version >= 1.0
         * and always send a 1.0 response.
         */
        int major = pMsg->GetMajorVersion();
        if (major < 1)
        {
            // Client must be really confused, there is no RTSP/0.x.
            SendResponse(505);
            OnClientRequestEvent(pReq, pMsg, NULL);
            pMsg->Release();
            pReq->Release();
            return;
        }

        // This is where we would set the response message version
        // The default is 1.0 so we don't need to do anything right now
        // Handle common headers (Require, etc.)
        // Note this will call ReplaceDelimiters and it will send a response
        // if the message is unacceptable.
        if (handleCommonRequestHeaders(pReq) != HXR_OK)
        {
            pMsg->Release();
            pReq->Release();
            return;
        }

        // If the session has been killed, only allow teardown
        if (state == KILLED && verb != RTSP_VERB_TEARDOWN)
        {
            SendResponse(403);
            pMsg->Release();
            pReq->Release();
            return;
        }

        // Now we have a message that appears valid.
        // Dispatch it to the appropriate handler.

        if (!g_bAllowedMethods[verb][state])
        {
            handleStateError(state);
            OnClientRequestEvent(pReq, pMsg, NULL);
            pMsg->Release();
            pReq->Release();
            return;
        }

        switch(pReq->GetVerb())
        {
        case RTSP_VERB_OPTIONS:
            rc = OnOptionsRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnOptionsRequest(pReq);
            break;
        case RTSP_VERB_DESCRIBE:
            rc = OnDescribeRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnDescribeRequest(pReq);
            break;
        case RTSP_VERB_SETUP:
            rc = OnSetupRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnSetupRequest(pReq);
            break;
        case RTSP_VERB_PLAY:
            rc = OnPlayRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnPlayRequest(pReq);
            break;
        case RTSP_VERB_PAUSE:
            rc = OnPauseRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnPauseRequest(pReq);
            break;
        case RTSP_VERB_ANNOUNCE:
            rc = OnAnnounceRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnAnnounceRequest(pReq);
            break;
        case RTSP_VERB_RECORD:
            rc = OnRecordRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnRecordRequest(pReq);
            break;
        case RTSP_VERB_TEARDOWN:
            rc = OnTeardownRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnTeardownRequest(pReq);
            break;
        case RTSP_VERB_GETPARAM:
            rc = OnGetParameterRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnGetParamRequest(pReq);
            break;
        case RTSP_VERB_SETPARAM:
            rc = OnSetParameterRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnSetParamRequest(pReq);
            break;
        case RTSP_VERB_PLAYNOW:
            rc = OnPlaynowRequest(pReq);
            break;
        case RTSP_VERB_REDIRECT:
            rc = OnRedirectRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnRedirectRequest(pReq);
            break;
        default:
            rc = OnExtensionRequest(pReq);
            if (m_pResponse2) m_pResponse2->OnExtensionRequest(pReq);
        }

        if (rc == HXR_OK)
        {
            setState(pReq, g_nNextState[verb][state]);
        }

        pMsg->Release();
        pReq->Release();
        return;
    }

    if (HXR_OK ==
        m_pConsumer->QueryInterface(IID_IHXRTSPResponseMessage, (void**)&pRsp))
    {
        if (HXR_OK != pRsp->QueryInterface(IID_IHXRTSPMessage, (void**)&pMsg))
        {
            HX_RELEASE(pRsp);
            HX_ASSERT(FALSE); //Per Tommy, this should never happen
            return;
        }

        m_pConsumer->AsBuffer(pBuf);
        handleDebug(pBuf, TRUE);
        handleTiming(pBuf, TRUE);
        uCSeq = pMsg->GetCSeq();

        // is this a response to a keepalive message?  Update the session
        // timestamp if so.  (We go by the seqno, because the client doesn't
        // typically send the session id in these responses.)

        RTSPServerProtocol::Session* pSession = NULL;
        char* pSessionID;

        if (m_pKeepAlivePendingMessages->Lookup(uCSeq, (void*&) pSessionID))
        {
            pSession = getSession(pSessionID);
            if (pSession)
            {
                pSession->SetSessionActiveStamp();
            }

            // remove the pending message from the list

            m_pKeepAlivePendingMessages->RemoveKey(uCSeq);
            delete [] pSessionID;
        }
        else
        {
            // if we did happen to get a session id (not in response to a
            // keepalive message), then use that.

            CHXString sessionID;
            pSession = NULL;
            if (HXR_OK == getSessionID(pMsg, sessionID))
            {
                pSession = getSession(sessionID);
                if (pSession)
                {
                    pSession->SetSessionActiveStamp();
                }
            }
        }

        // This response should match something on our sent-requests queue.
        RTSPMessage* pReqMsg = dequeueMessage(uCSeq);
        if (pReqMsg != NULL)
        {
            RTSPMethod RequestMethod = pReqMsg->GetMethod();

            HX_ASSERT((RequestMethod != RTSP_UNKNOWN) && (RequestMethod != RTSP_RESP));
            OnClientResponseEvent(pRsp, pMsg, RequestMethod, pSession);
            HX_DELETE(pReqMsg);
        }

        HX_RELEASE(pBuf);

        pMsg->Release();
        pRsp->Release();
        return;
    }

    // NOTREACHED
    HX_ASSERT(FALSE);
}

/**
 * \brief handleDebug - tracing facility for RTSP
 *
 * handleDebug is called from rtspbase.cpp when sending a message.
 * We must call it when receiving a message.
 *
 * \param pMsgBuf [in]
 * \param bInbound [in] : TRUE if this is a received message
 *
 * \return void
 */
void
RTSPServerProtocol::handleDebug(IHXBuffer* pMsgBuf, BOOL bInbound)
{
    if (m_pDebugFile != NULL)
    {
        static const char* szUnknown = "unknown";
        IHXBuffer* pClientAddrBuf = NULL;
        IHXBuffer* pLocalAddrBuf = NULL;
        IHXSockAddr* pAddr = NULL;
        const char* szDir = NULL;
        const char* pSrcIP = szUnknown;
        const char* pDstIP = szUnknown;
        if (m_pSocket != NULL)
        {
            m_pSocket->GetPeerAddr(&pAddr);
            if (pAddr)
            {
                pAddr->GetAddr(&pClientAddrBuf);
                pAddr->Release();
            }

            m_pSocket->GetLocalAddr(&pAddr);
            if (pAddr)
            {
                pAddr->GetAddr(&pLocalAddrBuf);
                pAddr->Release();
                pAddr = NULL;
            }
        }
        if (bInbound)
        {
            szDir = "IN";
            if (pClientAddrBuf != NULL)
            {
                pSrcIP = (const char*)pClientAddrBuf->GetBuffer();
            }
            if (pLocalAddrBuf != NULL)
            {
                pDstIP = (const char*)pLocalAddrBuf->GetBuffer();
            }
        }
        else
        {
            szDir = "OUT";
            if (pClientAddrBuf != NULL)
            {
                pDstIP = (const char*)pClientAddrBuf->GetBuffer();
            }
            if (pLocalAddrBuf != NULL)
            {
                pSrcIP = (const char*)pLocalAddrBuf->GetBuffer();
            }
        }

        fprintf(m_pDebugFile,
                "======================================================================\n"
                "%s: %s ==> %s\n"
                "----------------------------------------------------------------------\n"
                "%.*s\n",
                szDir, pSrcIP, pDstIP, pMsgBuf->GetSize(), pMsgBuf->GetBuffer());
        fflush(m_pDebugFile);

        HX_RELEASE(pLocalAddrBuf);
        HX_RELEASE(pClientAddrBuf);
    }
}

/**
 *
 * This is called from rtspbase.cpp when sending a message.
 * We must call it when receiving a message.
 *
 */
void
RTSPServerProtocol::handleTiming(IHXBuffer* pMsgBuf, BOOL bInbound)
{
    if (m_pTimingFile != NULL)
    {
        /*
         * We record the size of the command, headers, and content separately.
         *
         * command = first_line
         * headers = total_size - command - content
         * content = size of content buffer
         */
        const char* szDir = (bInbound ? "RECV" : "SEND");
        const char* pMsgText = (const char*)pMsgBuf->GetBuffer();
        const char* pMsgEnd = pMsgText+pMsgBuf->GetSize();
        UINT32 uCommandSize = 0;
        UINT32 uHeaderSize = 0;
        UINT32 uContentSize = 0;
        UINT32 uVerbSize = 0;
        const char* pCur = pMsgText;
        const char* pEOL;
        const char* pMark;

        const char* pHdrCSeq = NULL;
        UINT32 uHdrCSeqLen = 0;
        const char* pHdrSess = NULL;
        UINT32 uHdrSessLen = 0;
        const char* pHdrUA = NULL;
        UINT32 uHdrUALen = 0;

        // If we don't have 9 bytes, we can't even hold "RTSP/#.#\n"
        if (pMsgEnd-pMsgText < 9)
        {
            HX_ASSERT(FALSE);
            return;
        }

        /*
         * Note that we always leave one extra character for the terminating
         * LF when writing to szLine.
         */
        char szLine[256];
        UINT32 uLen = 0;

        HXTimeval now = m_pAccurateClock->GetTimeOfDay();

        pEOL = (const char*)memchr(pCur, '\n', pMsgEnd-pCur);
        if (pEOL == NULL)
        {   // Malformed message
            return;
        }
        uCommandSize = pEOL-pCur+1;

        if (strncasecmp(pCur, "RTSP/", 5) != 0)
        {
            // It's a request
            pMark = (const char*)memchr(pCur, ' ', pEOL-pCur);
            if (pMark == NULL)
            {
                // Malformed message
                return;
            }
            uVerbSize = pMark-pCur;
            if (uVerbSize > 32)
            {
                // Not a valid verb
                return;
            }

            // "RTSP:this,time,dir,REQ,verb"
            uLen = sprintf(szLine, "RTSP:%p,%lu.%06lu,%s,REQ,%.*s", this,
                      now.tv_sec, now.tv_usec, szDir, (int)uVerbSize, pMsgText);
        }
        else
        {
            // It's a response
            UINT32 code = 0;
            pMark = (const char*)memchr(pCur, ' ', pEOL-pCur);
            if (pMark == NULL || !isdigit(pMark[1]))
            {
                // Malformed message
                return;
            }
            code = atoi(pMark+1);

            // "RTSP:this,time,dir,RSP,code"
            uLen = sprintf(szLine, "RTSP:%p,%lu.%06lu,%s,RSP,%lu", this,
                           now.tv_sec, now.tv_usec, szDir, code);
        }
        pCur = pEOL+1;

        // Find the end of the headers
        pMark = pCur;
        BOOL bFoundEnd = FALSE;
        while(!bFoundEnd)
        {
            pEOL = (const char*)memchr(pMark, '\n', pMsgEnd-pMark);
            if (pEOL == NULL)
            {
                HX_ASSERT(FALSE);
                return;
            }

            // Look for some important headers
            // (note we don't handle multiline or multiple instances here)
            if (strncasecmp(pMark, "CSeq:", 5) == 0)
            {
                pHdrCSeq = pMark+5;
                uHdrCSeqLen = pEOL-pMark-5;
                // Trim whitespace
                while (*pHdrCSeq == ' ' && uHdrCSeqLen > 0)
                {
                    pHdrCSeq++;
                    uHdrCSeqLen--;
                }
                // Chop off CR
                if (uHdrCSeqLen > 0 && pHdrCSeq[uHdrCSeqLen-1] == '\r')
                {
                    uHdrCSeqLen--;
                }
            }
            if (strncasecmp(pMark, "Session:", 8) == 0)
            {
                pHdrSess = pMark+8;
                uHdrSessLen = pEOL-pMark-8;
                // Trim whitespace
                while (*pHdrSess == ' ' && uHdrSessLen > 0)
                {
                    pHdrSess++;
                    uHdrSessLen--;
                }
                // Chop off CR
                if (uHdrSessLen > 0 && pHdrSess[uHdrSessLen-1] == '\r')
                {
                    uHdrSessLen--;
                }
            }
            if (strncasecmp(pMark, "User-Agent:", 11) == 0)
            {
                pHdrUA = pMark+11;
                uHdrUALen = pEOL-pMark-11;
                // Trim whitespace
                while (*pHdrUA == ' ' && uHdrUALen > 0)
                {
                    pHdrUA++;
                    uHdrUALen--;
                }
                // Chop off CR
                if (uHdrUALen > 0 && pHdrUA[uHdrUALen-1] == '\r')
                {
                    uHdrUALen--;
                }
            }

            if ((pEOL == pMark) || (pMark[0] == '\r' && pEOL == pMark+1))
            {
                bFoundEnd = TRUE;
            }
            pMark = pEOL+1;
        }
        uHeaderSize = pMark-pCur;
        pCur = pMark;

        // The rest, if any, is the content (entity)
        uContentSize = pMsgEnd-pCur;

        // Fill in the three sizes
        uLen += sprintf(szLine+uLen, ",%u,%u,%u",
                        (unsigned int)uCommandSize, 
                        (unsigned int)uHeaderSize, 
                        (unsigned int)uContentSize);

        /*
         * Add all of the important headers we found, but only if they fit
         * onto the line (leaving room for the LF).
         */
        if (pHdrCSeq != NULL && uLen+6+1+uHdrCSeqLen+1 < sizeof(szLine)-1)
        {
            memcpy(szLine+uLen, ",cseq=", 6);
            uLen += 6;
            szLine[uLen++] = '\'';
            memcpy(szLine+uLen, pHdrCSeq, uHdrCSeqLen);
            uLen += uHdrCSeqLen;
            szLine[uLen++] = '\'';
        }
        if (pHdrSess != NULL && uLen+6+1+uHdrSessLen+1 < sizeof(szLine)-1)
        {
            memcpy(szLine+uLen, ",sess=", 6);
            uLen += 6;
            szLine[uLen++] = '\'';
            memcpy(szLine+uLen, pHdrSess, uHdrSessLen);
            uLen += uHdrSessLen;
            szLine[uLen++] = '\'';
        }
        if (pHdrUA != NULL && uLen+6+1+uHdrUALen+1 < sizeof(szLine)-1)
        {
            memcpy(szLine+uLen, ",uagt=", 6);
            uLen += 6;
            szLine[uLen++] = '\'';
            memcpy(szLine+uLen, pHdrUA, uHdrUALen);
            uLen += uHdrUALen;
            szLine[uLen++] = '\'';
        }

        /*
         * The line is complete.  Fill in the LF, terminate it, and write it.
         */
        szLine[uLen++] = '\n';
        szLine[uLen] = '\0';
        fputs(szLine, m_pTimingFile);
    }
}


/**
 *
 * OnClientRequestEvent() handles RTSPEvents for CLIENT-generated REQUESTS.
 *
 */

HX_RESULT
RTSPServerProtocol::OnClientRequestEvent(IHXRTSPRequestMessage* pReq,
                                         IHXRTSPMessage* pMsg,
                                         RTSPServerProtocol::Session* pSession)
{
    HX_RESULT hr = HXR_OK;
    RTSPMethod Method = RTSP_UNKNOWN;

    // QI of pReq for pMsg should happen earlier in the call stack.
    // Same with trackevents check.
    if (!pReq || !pMsg)
    {
        HX_ASSERT(!"RTSPSvrProt::TrkClntReqEvnt() - param missing!");
        return HXR_UNEXPECTED;
    }

    Method = pReq->GetMethod();

    if (m_bIsProxy)
    {
        // Handled in CRTSPProxyClient.
        HX_ASSERT(!m_bTrackEvents);
        return HXR_FAIL;
    }

    HX_ASSERT(m_pAggStats);

    // Update agg stats.
    m_pAggStats->UpdateClientRequestCount(1, Method);


    if (m_bTrackEvents)
    {
        if (pSession)
        {
            HX_ASSERT(pSession->m_pEventList);

            // This should be caught further up. For these cases,
            // OnxxxRequest should explicitly set pSession to NULL.
            HX_ASSERT(Method != RTSP_DESCRIBE);
            HX_ASSERT(Method != RTSP_ANNOUNCE);

            hr = pSession->m_pEventList->UpdateRequestStats(Method,
                                                            pMsg->GetCSeq(),
                                                            FALSE);
        }
        else
        {
            hr = m_pRtspStatsMgr->UpdateRequestStats(Method,
                                                     pMsg->GetCSeq(),
                                                     FALSE);
        }
    }

    return hr;
}


/**
 *
 * OnServerResponseEvent() handles RTSPEvents for SERVER-generated RESPONSES.
 *
 */

HX_RESULT
RTSPServerProtocol::OnServerResponseEvent(UINT32 ulStatusCode)
{
    HX_RESULT hr = HXR_OK;
    BOOL bSuccess = (ulStatusCode >= 400) ? FALSE : TRUE;

    // Status code check should occur earlier.
    if (ulStatusCode == 0)
    {
        HX_ASSERT(!"RTSPSvrProt::TrkSvrRespEvnt() - invalid status code!");
        return HXR_UNEXPECTED;
    }

    if (m_bIsProxy)
    {
        // Handled in CRTSPProxyProtocol.
        HX_ASSERT(!m_bTrackEvents);
        return HXR_FAIL;
    }

    m_pAggStats->UpdateServerResponseCount(1,
                                           m_LastRequestMethod,
                                           ulStatusCode);

    HX_ASSERT(m_pSDPStatsMgr);

    if (m_LastRequestMethod == RTSP_DESCRIBE)
    {
        m_pSDPStatsMgr->IncrementSDPDownloadCount(SDPAggregateStats::RTSP_PROT,
                                                      bSuccess);
    }
    else if (m_LastRequestMethod == RTSP_SETUP)
    {
        m_pSDPStatsMgr->IncrementSDPSessionInitCount(SDPAggregateStats::RTSP_PROT,
                                                         bSuccess);
    }


    if (m_bTrackEvents)
    {
        // We just want to find the matching request event, no matter where it is.
        CHXString sessionID;
        RTSPServerProtocol::Session* pSession = NULL;

        if (SUCCEEDED(getSessionID(m_pRespMsg, sessionID)))
        {
            pSession = getSession(sessionID);
        }


        if (pSession)
        {
            HX_ASSERT(pSession->m_pEventList);

            hr = pSession->m_pEventList->UpdateResponseStats(ulStatusCode,
                                                             m_pRespMsg->GetCSeq(),
                                                             TRUE);
        }

        if (!pSession || FAILED(hr))
        {
            hr = m_pRtspStatsMgr->UpdateResponseStats(ulStatusCode,
                                                      m_pRespMsg->GetCSeq(),
                                                      TRUE);
        }
    }

    return hr;
}

/**
 *
 * OnServerRequestEvent() handles RTSPEvents for SERVER-generated REQUESTS.
 *
 */

HX_RESULT
RTSPServerProtocol::OnServerRequestEvent(RTSPRequestMessage* pMsg,
                                         const char* pSessionID,
                                         RTSPServerProtocol::Session* pSession)
{
    RTSPMethod Method = RTSP_UNKNOWN;
    HX_RESULT hr = HXR_OK;

    // Param check should occur earlier.
    if (!pMsg)
    {
        HX_ASSERT(!"RTSPSvrProt::TrkSvrReqEvnt() - pMsg missing!");
        return HXR_UNEXPECTED;
    }

    if (m_bIsProxy)
    {
        // Handled in CRTSPProxyProtocol.
        HX_ASSERT(!m_bTrackEvents);
        return HXR_FAIL;
    }

    Method = pMsg->GetMethod();

    m_pAggStats->UpdateServerRequestCount(1, Method);

    if (m_bTrackEvents)
    {
        if (!pSession)
        {
            if (pSessionID)
            {
                pSession = getSession(pSessionID);
            }
        }

        if (pSession)
        {
            hr = pSession->m_pEventList->UpdateRequestStats(Method,
                                                            pMsg->seqNo(),
                                                            TRUE);
        }
        else
        {
            hr = m_pRtspStatsMgr->UpdateRequestStats(Method,
                                                     pMsg->seqNo(),
                                                     TRUE);
        }
    }

    return hr;
}



/**
 *
 * OnClientResponseEvent() handles RTSPEvents for CLIENT-generated RESPONSES.
 * RequestMethod is acquired from the server-pending-request queue.
 *
 */

HX_RESULT
RTSPServerProtocol::OnClientResponseEvent(IHXRTSPResponseMessage* pResp,
                                          IHXRTSPMessage* pMsg,
                                          RTSPMethod RequestMethod,
                                          RTSPServerProtocol::Session* pSession)
{
    HX_RESULT hr = HXR_OK;
    UINT32 ulStatusCode = 0;

    // Param and trackevents check should occur earlier.
    if (!pResp || !pMsg)
    {
        HX_ASSERT(!"RTSPSvrProt::TrkClntRespEvnt() - param missing!");
        return HXR_UNEXPECTED;
    }

    if (m_bIsProxy)
    {
        // Handled in CRTSPProxyProtocol.
        HX_ASSERT(!m_bTrackEvents);
        return HXR_FAIL;
    }

    pResp->GetStatusCode(ulStatusCode);

    if (ulStatusCode == 0)
    {
        HX_ASSERT(!"RTSPSvrProt::TrkClntRespEvnt() - bad status code!");
        return HXR_UNEXPECTED;
    }

    m_pAggStats->UpdateClientResponseCount(1,
                                           RequestMethod,
                                           ulStatusCode);

    if (m_bTrackEvents)
    {
        if (pSession)
        {
            HX_ASSERT(pSession->m_pEventList);

            hr = pSession->m_pEventList->UpdateResponseStats(ulStatusCode,
                                                             pMsg->GetCSeq(),
                                                             FALSE);
        }

        if (!pSession || FAILED(hr))
        {
            hr = m_pRtspStatsMgr->UpdateResponseStats(ulStatusCode,
                                                      pMsg->GetCSeq(),
                                                      FALSE);
        }
    }

    return hr;
}


HX_RESULT
RTSPServerProtocol::SetStreamStartTime(const char* pszSessionID,
                                       UINT32 ulStreamNum, UINT32 ulTimestamp)
{
    // Figure out the starting position (timestamp) for a given stream
    HX_RESULT rc = HXR_OK;

    RTSPServerProtocol::Session* pSession = getSession(pszSessionID);

    if (!pSession)
    {
        return HXR_FAIL;
    }

    // If this is a StreamDone ts, keep waiting for either a
    // packet ts or for all streams to send stream done
    if (ulTimestamp == END_OF_STREAM_TS &&
        ++(pSession->m_ulStreamsEnded) < pSession->m_uTotalSetupReceived)
    {
        return rc;
    }
	
    if (pSession->m_bRTPInfoResponsePending)
    {
        if (++(pSession->m_unStreamsRTPInfoReady) >= pSession->m_uTotalSetupReceived ||
            pSession->m_ulStreamsEnded >= pSession->m_uTotalSetupReceived)
        {
            //XXXTDM: Satiate SetupSequenceNumberResponse -- remove check instead?
            pSession->m_unStreamsRTPInfoReady = pSession->m_uTotalSetupReceived;

        pSession->m_streamSequenceNumbers = "";
        SetupSequenceNumberResponse(pszSessionID);
        pSession->m_bRTPInfoResponsePending = FALSE;
    }
    }

    if (pSession->m_bRangeResponsePending)
    {
        /* It may be a long time until we get a packet for both streams
         * use the first packet from either stream for response range
         * since we know that file formats will return the the lowest
         * timestamp first (or close to it anyway)
         */

        pSession->m_ulPlayRangeStart = ulTimestamp;

        /* If we seek to the end of the stream, we need to set the start of the
         * play range to the end of the clip.
         */
        if (pSession->m_ulPlayRangeStart == END_OF_STREAM_TS)
        {
            if (pSession->m_ulPlayRangeEnd !=  RTSP_PLAY_RANGE_BLANK)
            {
                pSession->m_ulPlayRangeStart = pSession->m_ulPlayRangeEnd;
            }
            else
            {
                pSession->m_ulPlayRangeStart = pSession->m_ulSessionDuration;
            }
            pSession->m_ulPlayRangeEnd = RTSP_PLAY_RANGE_BLANK;
        }

        /* If we're sending a play response after a seek, we need to update the
         * timestamps in the RTP Info header.
         */
        pSession->m_bRangeResponsePending = FALSE;
    }

    /// SendPlayResponse checks for pending scale, range and RTPInfo before responding!
    SendPlayResponse(200, pSession->m_ulPlayReqCSeq, pSession);

    return rc;
}

/**
 * \brief SetActiveStamp - we are alive! Update active timestamp for keepalive. 
 *
 * We have received an RTSP message of some kind. Update active timestamp 
 * for keepalive.
 *
 * \param tv_sec [in] : unused - defaults to 0
 *
 * \return void
 */
void
RTSPServerProtocol::SetActiveStamp(time_t tv_sec /* =0 */)
{
    // Set the timestamp of the last time we heard anything on the wire.
    // Get the time off our scheduler if tv_sec is not supplied.

    m_bRTSPPingResponsePending = FALSE;

    if (tv_sec)
    {
        m_tActiveStamp = tv_sec;
    }
    else
    {
        HXTimeval rmatv = m_pScheduler->GetCurrentSchedulerTime();
        m_tActiveStamp = rmatv.tv_sec;
    }
}

/**
 * \brief IsViaRealProxy : return true if we know we are connected to a proxy
 *
 *  Return TRUE if the via header exists and contains the RealProxy string.
 *  (Also check for HelixProxy, in case one is ever released.)
 *
 * \param pMsg [in] : incoming request message
 *
 * \return TRUE if via header indicates presence of a proxy
 */
BOOL
RTSPServerProtocol::IsViaRealProxy(IHXRTSPMessage* pMsg)
{
    BOOL ret = FALSE;

    IHXMIMEHeader* pHdr = NULL;
    if (HXR_OK == pMsg->GetHeader("Via", pHdr))
    {
        IHXBuffer* pBuff = NULL;
        pHdr->GetValueAsBuffer(pBuff);
        UINT32 nLen = pBuff->GetSize();
        const char* pStr = (const char*)pBuff->GetBuffer();

        if (StrNStr(pStr, "RealProxy", nLen, sizeof("RealProxy")) ||
            StrNStr(pStr, "HelixProxy", nLen, sizeof("HelixProxy")))
        {
            ret = TRUE;
        }

        HX_RELEASE(pBuff);
        HX_RELEASE(pHdr);
    }
    return ret;
}

/**
 * \brief IsTCPPrefLicensed - check to see if we are licensed to prefer TCP transport
 *
 * \param n/a 
 *
 * \return TRUE if licensed to prefer TCP transport
 */
BOOL
RTSPServerProtocol::IsTCPPrefLicensed()
{
    INT32 lEnabled = 0;
    m_pRegistry->GetIntByName(REGISTRY_TCP_PREF_ENABLED, lEnabled);
    return lEnabled;
}

HX_RESULT RTSPServerProtocol::AddStatsHeaders()
{
    IHXMIMEHeader* pHdr = NULL;
    char buf[32];
    // Only send the StatsInterval if it is defined
    UINT32 ulCurrentSI = m_pPropWatchResponse->GetStatsInterval();
    if(ulCurrentSI)
    {
        sprintf(buf, "%d", (int)ulCurrentSI);
        pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        if (pHdr)
        {
            pHdr->SetFromString("StatsInterval", buf);
            m_pRespMsg->AddHeader(pHdr);
        }
        else
            return HXR_FAIL;
    }

    // Always send the StatsMask, even if no StatsInterval
    sprintf(buf, "%d", (int)m_iStatsMask);
    pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    if (pHdr)
    {
        pHdr->SetFromString("StatsMask", buf);
        m_pRespMsg->AddHeader(pHdr);
        return HXR_OK;
    }
    return HXR_FAIL;
}

/**
 * \class RTSPServerProtocol::CPropWatchResponse::Instance
 *
 * This is a singleton (one instance running per streamer thread or process)
 * whose sole purpose in life is to track config file changes to the 
 * StatsInterval" entry.
 *
 */
//    *** IHXPropWatchResponse Implementation ***

//    This is a singlton, so here's how the class is created
RTSPServerProtocol::CPropWatchResponse*
RTSPServerProtocol::CPropWatchResponse::Instance(IUnknown* pIContext)
{
    // Do we have the thread-local IF and the array?
    // On proc platforms, the static arrays will be separate and there's no
    // problem.  On threaded platforms, the semaphore will prevent more
    // than one thread from creating the shared static array.

    if (!zm_pThreadLocal)
    {
        while (HXAtomicIncRetINT32(&zm_nStartupSemaphore) > 0)
        {
            HXAtomicDecINT32(&zm_nStartupSemaphore);
        }

        if (!zm_pThreadLocal)
        {
            pIContext->QueryInterface(IID_IHXThreadLocal,
                                      (void**)&zm_pThreadLocal);

            HX_ASSERT(zm_pThreadLocal);
            if (!zm_pThreadLocal) return NULL;
        }
        HXAtomicDecINT32(&zm_nStartupSemaphore);
    }

    if (!zm_ppInstance)
    {
        while (HXAtomicIncRetINT32(&zm_nStartupSemaphore) > 0)
        {
            HXAtomicDecINT32(&zm_nStartupSemaphore);
        }

        if (!zm_ppInstance)
        {
            int maxThreads = zm_pThreadLocal->GetMaxThreads();
            HX_ASSERT(maxThreads > 0 && maxThreads < 1025);

            zm_ppInstance = new CPropWatchResponse*[maxThreads];
            HX_ASSERT(zm_ppInstance);
            if (!zm_ppInstance) return NULL;

            memset(zm_ppInstance, 0, sizeof(CPropWatchResponse*) * maxThreads);
        }
        HXAtomicDecINT32(&zm_nStartupSemaphore);
    }

    int procnum = zm_pThreadLocal->GetThreadNumber();

    if(!zm_ppInstance[procnum])
    {
        zm_ppInstance[procnum] = new CPropWatchResponse(pIContext);
        zm_ppInstance[procnum]->AddRef();
    }

    return zm_ppInstance[procnum];     // Will be either null or contain and instance
}

RTSPServerProtocol::CPropWatchResponse::CPropWatchResponse( IUnknown* pIContext)
: m_lRefCount(0)
, m_pIContext(pIContext)
, m_pIRegistry(NULL)
, m_pIPropWatch(NULL)
, m_uidStatsIntervalKey(0)
, m_uidConfigRootKey(0)
, m_ulStatsInterval(0)
{
    if(m_pIContext)
    {
        m_pIContext->AddRef();

        m_pIContext->QueryInterface(IID_IHXRegistry, (void**) &m_pIRegistry);
        HX_ASSERT(m_pIRegistry);
        if(m_pIRegistry)
        {
            INT32 iStatsInterval = 0;
            m_pIRegistry->GetIntByName(STATS_INTERVAL_KEY, iStatsInterval);

            if((iStatsInterval > 0) && (iStatsInterval < STATS_INTERVAL_FLOOR))
            {
                iStatsInterval = STATS_INTERVAL_FLOOR;
            }
            m_ulStatsInterval = (UINT32)iStatsInterval;

            m_pIRegistry->CreatePropWatch(m_pIPropWatch);
            HX_ASSERT(m_pIPropWatch);
            if(m_pIPropWatch)
            {
                HX_RESULT hxr = m_pIPropWatch->Init(this);
                if(hxr == HXR_OK)
                {
                    m_uidStatsIntervalKey =
                        m_pIPropWatch->SetWatchByName(STATS_INTERVAL_KEY);
                    if(!m_uidStatsIntervalKey)
                    {
                        m_uidConfigRootKey =
                            m_pIPropWatch->SetWatchByName(CONFIG_ROOT_KEY);
                    }
                }
            }
        }
    }
}

RTSPServerProtocol::CPropWatchResponse::~CPropWatchResponse()
{
    if(m_uidConfigRootKey)
    {
        m_pIPropWatch->ClearWatchById(m_uidConfigRootKey);
    }

    if(m_uidStatsIntervalKey)
    {
        m_pIPropWatch->ClearWatchById(m_uidStatsIntervalKey);
    }

    HX_RELEASE(m_pIPropWatch);
    HX_RELEASE(m_pIRegistry);
    HX_RELEASE(m_pIContext);
}

//    *** IUnknown Methods ***
STDMETHODIMP
RTSPServerProtocol::CPropWatchResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IUnknown*)(IHXPropWatchResponse*)this;
        AddRef();
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
        *ppvObj = (IHXPropWatchResponse*)this;
        AddRef();
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_ (ULONG32)
RTSPServerProtocol::CPropWatchResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_ (ULONG32)
RTSPServerProtocol::CPropWatchResponse::Release()
{
    if(InterlockedIncrement(&m_lRefCount) <= 0)
    {
        delete this;
        return 0;
    }

    return m_lRefCount;
}

//    *** IHXPropWatchResponse Methods ***
STDMETHODIMP
RTSPServerProtocol::CPropWatchResponse::AddedProp(const UINT32 id,
    const HXPropType propType, const UINT32 ulParentId)
{
    if(ulParentId == m_uidConfigRootKey)
    {
        IHXBuffer* pINewKeyBuff = NULL;
        m_pIRegistry->GetPropName(id, pINewKeyBuff);
        if(pINewKeyBuff)
        {
            //    if the new key is the StatsInterval key
            if(strcasecmp((const char*)pINewKeyBuff->GetBuffer(),
                STATS_INTERVAL_KEY) == 0)
            {
                if(!m_uidStatsIntervalKey)
                {
                    m_uidStatsIntervalKey =
                        m_pIPropWatch->SetWatchByName(STATS_INTERVAL_KEY);

                    //    Update the StatsInterval for all sessions,
                    //    if the sessions aren't currently receiving stats
                    //    they will never receive the update
                    //    All new clients will get the updates
                    INT32 iStatsInterval = 0;
                    m_pIRegistry->GetIntById(id, iStatsInterval);
                    if ((iStatsInterval > 0) && 
                        (iStatsInterval < STATS_INTERVAL_FLOOR))
                    {
                        iStatsInterval = STATS_INTERVAL_FLOOR;
                    }
                    m_ulStatsInterval = (UINT32)iStatsInterval;

                    if(m_uidStatsIntervalKey)
                    {
                        m_pIPropWatch->ClearWatchById(m_uidConfigRootKey);
                        m_uidConfigRootKey = 0;
                    }
                }
            }
        }
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPServerProtocol::CPropWatchResponse::ModifiedProp(const UINT32 id,
    const HXPropType propType, const UINT32 ulParentId)
{
    INT32 iStatsInterval = 0;
    m_pIRegistry->GetIntById(id, iStatsInterval);

    if((iStatsInterval > 0) && (iStatsInterval < STATS_INTERVAL_FLOOR))
    {
        iStatsInterval = STATS_INTERVAL_FLOOR;
    }
    m_ulStatsInterval = (UINT32)iStatsInterval;

    return HXR_OK;
}


STDMETHODIMP
RTSPServerProtocol::CPropWatchResponse::DeletedProp(const UINT32 id,
    const UINT32 ulParentId)
{
    //    This handles the case were someone goes in and manually deletes the
    //    Key from from the file and has the server reload the file
    if(m_uidStatsIntervalKey == id)
    {
        m_pIPropWatch->ClearWatchById(id);
        m_uidStatsIntervalKey = 0;

        //    No longer want stats updates
        m_ulStatsInterval = 0;

        m_uidConfigRootKey = m_pIPropWatch->SetWatchByName(CONFIG_ROOT_KEY);
    }

    return HXR_OK;
}

/**
 * \brief HandleStreamAdaptationHeader
 *
 * HandleStreamAdaptationHeader parses a "Helix-Adaptation" or "3GPP-Adaptation"
 * header in a client request and calls m_pResp->HandleStreamAdaptation() to 
 * notify the packet flow.
 *
 * \param sessionID [in] : session id - not sure why this is a REF?
 * \param pRTSPMsg [in] : RTSP message to check
 * \param bProcessingSetup [in] : TRUE if the message is a SETUP request
 *
 * \return HXR_OK if all processing of this header succeeds
 */
HX_RESULT
RTSPServerProtocol::HandleStreamAdaptationHeader ( REF(const CHXString) sessionID
                                        , IHXRTSPMessage* pRTSPMsg
                                        , BOOL bProcessingSetup)
{
    IHXMIMEHeader*  pMIMEHeader = 0;
    HX_RESULT       ret = HXR_OK;
    BOOL            bHlxStreamAdaptScheme = FALSE;

    if (HXR_OK == pRTSPMsg->GetHeader("Helix-Adaptation", pMIMEHeader))
    {
        bHlxStreamAdaptScheme = TRUE;
    }
    else if (HXR_OK != pRTSPMsg->GetHeader("3GPP-Adaptation", pMIMEHeader))
    {
            return HXR_IGNORE;
    }

    if (!bProcessingSetup)
    {
        RTSPSessionItem* pSessionItemCurrent =
                    m_pSessionManager->findInstance(sessionID);
        if (!pSessionItemCurrent || !(pSessionItemCurrent->m_bSetup))
        {
//VS: INCOMPLETE
// Log error message indicating Adaptation Update received before Session Setup
            return HXR_FAIL;
        }
    }

    if (pMIMEHeader)
    {
        IUnknown* pUnkItem;

        IHXList*            pFieldList = 0;
        IHXListIterator*    pFieldItr = 0;
        IHXMIMEField*       pMIMEField = 0;

        IHXList*            pParamList = 0;
        IHXListIterator*    pParamItr = 0;
        IHXMIMEParameter*   pMIMEParam = 0;

        IHXBuffer*          pBuffAttr = 0;
        IHXBuffer*          pBuffVal = 0;

        StreamAdaptationParams streamAdapt;

//      streamAdapt.m_nAdaptationType = enumAdaptType;

        pMIMEHeader->GetFieldListConst(pFieldList);
        if (pFieldList)
        {
            pFieldItr = pFieldList->Begin();
            while (pFieldItr->HasItem() && (ret == HXR_OK))
            {
                streamAdapt.m_unStreamNum = 0xFFFF;
		streamAdapt.m_uStreamGroupNum = 0xFFFF;
                streamAdapt.m_ulClientBufferSize = 0;
                streamAdapt.m_ulTargetProtectionTime = 0;
                streamAdapt.m_bStreamSwitch = TRUE;
                streamAdapt.m_bBufferStateAvailable = TRUE;
                streamAdapt.m_ulNumExcludedRules= 0;

                pUnkItem = pFieldItr->GetItem();
                pUnkItem->QueryInterface(IID_IHXMIMEField, (void **)&pMIMEField);
                HX_RELEASE(pUnkItem);

                pMIMEField->GetParamListConst(pParamList);
                HX_RELEASE(pMIMEField);
                if (pParamList)
                {
                    pParamItr = pParamList->Begin();

                    while (pParamItr->HasItem())
                    {
                        pUnkItem = pParamItr->GetItem();
                        ret = pUnkItem->QueryInterface(IID_IHXMIMEParameter, (void **)&pMIMEParam);
                        HX_RELEASE(pUnkItem);

                        ret = pMIMEParam->Get(pBuffAttr, pBuffVal);
                        HX_RELEASE(pMIMEParam);

                        if (!strncasecmp((const char*)pBuffAttr->GetBuffer(), "url",
                                pBuffAttr->GetSize()))
                        {
                            UINT32 unValSize = pBuffVal->GetSize();
                            NEW_FAST_TEMP_STR(pURL, 1024, unValSize + 1);
                            strncpy(pURL, (const char*)pBuffVal->GetBuffer(), unValSize);
                            pURL[unValSize] = '\0';

                            if (pURL[unValSize-1] == '"')
                            {
                                pURL[unValSize-1] = '\0';
                            }

                            char* pStream = strrchr(pURL, '/');
                            UINT32 ulControlID;
                            if (!pStream)
                            {
                                if (bHlxStreamAdaptScheme)
                                {
                                    streamAdapt.m_unStreamNum = 0;
                                }
                                else
                                {
                                    ret = HXR_FAIL;
                                }
                            }
                            else if (!GetStreamId(pStream + 1, &ulControlID))
                            {
                                if (bHlxStreamAdaptScheme)
                                {
                                    streamAdapt.m_unStreamNum = 0;
                                }
                                else
                                {
//VS: INCOMPLETE
//Log an error message indicating reception of an implitcit REL6 Aggregate
// adaptation option. (no stream number in the url)
                                    ret = HXR_FAIL;
                                }
                            }
                            else
                            {
                                RTSPStreamInfo* pStream = NULL;
                                RTSPServerProtocol::Session* pSession =
                                    getSession((const char*)sessionID);
                                if (pSession)
                                {
                                    pStream = pSession->
                                        GetStreamFromControl(ulControlID);
                                }
                                if (pStream)
                                {
                                    streamAdapt.m_unStreamNum =
                                        pStream->m_streamNumber;
				    streamAdapt.m_uStreamGroupNum =
					pStream->m_uStreamGroupNumber;
                                }
                                else
                                {
                                    ret = HXR_FAIL;
                                }
                            }

                            DELETE_FAST_TEMP_STR(pURL);
                            if (FAILED(ret))
                            {
                                HX_RELEASE(pBuffAttr);
                                HX_RELEASE(pBuffVal);
                                break;
                            }
                        }
                        else if (!strncasecmp((const char*)pBuffAttr->GetBuffer(), "size",
                                pBuffAttr->GetSize()))
                        {
                            const char* pSize = (const char*)pBuffVal->GetBuffer();
                            streamAdapt.m_ulClientBufferSize
                                            = (UINT32)strtol(pSize, NULL, 10);
                        }
                        else if (!strncasecmp((const char*)pBuffAttr->GetBuffer(), "target-time",
                                pBuffAttr->GetSize()))
                        {
                            const char* pTargetTime = (const char*)pBuffVal->GetBuffer();
                            streamAdapt.m_ulTargetProtectionTime
                                            = (UINT32)strtol(pTargetTime, NULL, 10);
                        }
                        else if (!strncasecmp((const char*)pBuffAttr->GetBuffer(), "stream-switch",
                                pBuffAttr->GetSize()))
                        {
                            const char* pStreamSwitch = (const char*)pBuffVal->GetBuffer();
                            streamAdapt.m_bStreamSwitch
                                            = (BOOL)strtol(pStreamSwitch, NULL, 10);
                        }
                        else if (!strncasecmp((const char*)pBuffAttr->GetBuffer(), "feedback-level",
                                pBuffAttr->GetSize()))
                        {
                            const char* pFeedbackLevel = (const char*)pBuffVal->GetBuffer();
                            streamAdapt.m_bBufferStateAvailable
                                            = (BOOL)strtol(pFeedbackLevel, NULL, 10);
                        }
                        else if (!strncasecmp((const char*)pBuffAttr->GetBuffer(), "exclude-rules",
                                pBuffAttr->GetSize()))
                        {
                            char* pExcludeRules = (char*)pBuffVal->GetBuffer();

                            if ((pExcludeRules) &&
                                (*pExcludeRules == '('))
                            {
                                pExcludeRules++;

                                while ((*pExcludeRules != '\0') &&
                                       (*pExcludeRules != ')') &&
                                       (streamAdapt.m_ulNumExcludedRules < MAX_EXCLUDE_RULES))
                                {
                                    if (isdigit(*pExcludeRules))
                                    {
                                        streamAdapt.m_pExcludeRules [streamAdapt.m_ulNumExcludedRules] =
                                            (UINT16)strtol(pExcludeRules, &pExcludeRules, 10);
                                        streamAdapt.m_ulNumExcludedRules++;
                                    }
                                    else if ((*pExcludeRules == ',') ||
                                             (*pExcludeRules == ')'))
                                    {
                                        pExcludeRules++;
                                    }
                                    else
                                    {
                                        //Invalid exclude-rules header
                                        HX_ASSERT(0);
                                        streamAdapt.m_ulNumExcludedRules = 0;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                //Invalid exclude-rules header
                                HX_ASSERT(0);
                                streamAdapt.m_ulNumExcludedRules = 0;
                            }
                        }

                        HX_RELEASE(pBuffAttr);
                        HX_RELEASE(pBuffVal);
                        pParamItr->MoveNext();
                    }

                    HX_RELEASE(pParamItr);
                    HX_RELEASE(pParamList);

                    if (ret == HXR_OK && streamAdapt.m_unStreamNum != 0xFFFF)
                    {
                        if (HXR_OK != (ret = m_pResp->HandleStreamAdaptation((const char*)sessionID,
                                                                streamAdapt, bHlxStreamAdaptScheme)))
                        {
                            break;
                        }
                    }
                    else
                    {
                        ret = HXR_FAIL;
                        break;
                    }
                }

                pFieldItr->MoveNext();
            }
            HX_RELEASE(pFieldItr);
            HX_RELEASE(pFieldList);

            if (ret == HXR_OK)
            {
                m_pRespMsg->AddHeader(pMIMEHeader);
            }
        }

        HX_RELEASE(pMIMEHeader);
    }

    return ret;
}

/**
 * \brief Handle3GPPLinkCharHeader
 *
 * Handle3GPPLinkCharHeader parses a "3GPP-Link-Char" header in a client request
 * and calls m_pResp->Handle3GPPLinkChar() to notify the packet flow.
 *
 * \param sessionID [in] : session id - not sure why this is a REF?
 * \param pRTSPMsg [in] : RTSP message to check
 * \param bProcessingSetup [in] : TRUE if the message is a SETUP request
 *
 * \return HXR_OK if all processing of this header succeeds
 */
HX_RESULT
RTSPServerProtocol::Handle3GPPLinkCharHeader ( REF(const CHXString) sessionID
                                        , IHXRTSPMessage* pRTSPMsg
                                        , BOOL bProcessingSetup)
{
    IHXMIMEHeader*  pMIMEHeader = 0;
    HX_RESULT       ret = HXR_OK;

    IHXRTSPRequestMessage* pRTSPReqMsg = NULL;
    IHXBuffer* pRTSPBaseURI = NULL;


    //Check if Link Char header present
    if (HXR_OK != pRTSPMsg->GetHeader("3GPP-Link-Char", pMIMEHeader))
    {
        return HXR_IGNORE;
    }


    //Link-Char headers received through a RTSP message other than SETUP can only
    // be sent after session has been setup
    if (!bProcessingSetup)
    {
        RTSPSessionItem* pSessionItemCurrent =
                    m_pSessionManager->findInstance(sessionID);
        if (!pSessionItemCurrent || !(pSessionItemCurrent->m_bSetup))
        {
            //VS: INCOMPLETE
            // Log error message indicating Adaptation Update received before Session Setup
            return HXR_FAIL;
        }
    }


    //Process Link-Char header
    //
    //for each link characterics in the header
    //{
    //  - extract URL, GBW, MBW, MTD in a LinkCharParams struct
    //  - Pass on the struct to Player::Session through RTSPProtocol
    //}
    if (pMIMEHeader)
    {
        IUnknown* pUnkItem;

        IHXList*            pFieldList = 0;
        IHXListIterator*    pFieldItr = 0;
        IHXMIMEField*       pMIMEField = 0;

        IHXList*            pParamList = 0;
        IHXListIterator*    pParamItr = 0;
        IHXMIMEParameter*   pMIMEParam = 0;

        IHXBuffer*          pBuffAttr = 0;
        IHXBuffer*          pBuffVal = 0;

        LinkCharParams linkCharParams;

        pMIMEHeader->GetFieldListConst(pFieldList);
        if (pFieldList)
        {
            pFieldItr = pFieldList->Begin();
            while (pFieldItr->HasItem() && (ret == HXR_OK))
            {
                linkCharParams.m_bSessionAggregate = FALSE;
                linkCharParams.m_unStreamNum = 0xFFFF;
                linkCharParams.m_ulGuaranteedBW = 0;
                linkCharParams.m_ulMaxBW = 0;
                linkCharParams.m_ulMaxTransferDelay = 0;

                pUnkItem = pFieldItr->GetItem();
                pUnkItem->QueryInterface(IID_IHXMIMEField, (void **)&pMIMEField);
                HX_RELEASE(pUnkItem);

                pMIMEField->GetParamListConst(pParamList);
                HX_RELEASE(pMIMEField);
                if (pParamList)
                {
                    pParamItr = pParamList->Begin();

                    while (pParamItr->HasItem())
                    {
                        pUnkItem = pParamItr->GetItem();
                        ret = pUnkItem->QueryInterface(IID_IHXMIMEParameter, (void **)&pMIMEParam);
                        HX_RELEASE(pUnkItem);

                        ret = pMIMEParam->Get(pBuffAttr, pBuffVal);
                        HX_RELEASE(pMIMEParam);

                        const char *pcAttribute = (const char*)(pBuffAttr->GetBuffer());
                        const UINT32 unAttrBuffSize = pBuffAttr->GetSize();
                        if (!strncasecmp(pcAttribute, "url", unAttrBuffSize))
                        {
                            const char *pcValue = (const char*)pBuffVal->GetBuffer();
                            UINT32 unValSize = pBuffVal->GetSize();

                            //Account for '"' in url attribute
                            if (pcValue[unValSize-1] == '"')
                            {
                                --unValSize;
                            }

                            if (pcValue[0] == '"')
                            {
                                pcValue++;
                                --unValSize;
                            }

                            NEW_FAST_TEMP_STR(pURL, 1024, unValSize + 1);
                            strncpy(pURL, pcValue, unValSize);
                            pURL[unValSize] = '\0';

                            UINT32 ulControlID;
                            char* pStream = strrchr(pURL, '/');

                            //check if URI specified is for a particular stream or
                            // is set to the session base URL
                            if (pStream && GetStreamId(pStream + 1, &ulControlID))
                            {
                                //URI contains a streamid. This indicates that
                                // link char will be set on a Stream Level
                                //
                                //Translate the trackid to Logical Stream and
                                // update LinkCharParams struct with the stream number
                                RTSPStreamInfo* pStream = NULL;
                                RTSPServerProtocol::Session* pSession =
                                    getSession((const char*)sessionID);
                                if (pSession)
                                {
                                    pStream = pSession->GetStreamFromControl(ulControlID);
                                }
                                if (pStream)
                                {
                                    linkCharParams.m_unStreamNum =
                                        pStream->m_streamNumber;
                                }
                                else
                                {
                                    ret = HXR_FAIL;
                                }
                            }
                            else if (!bProcessingSetup)
                            {
                                //since url value does not contain a stream ID
                                // check if link char is set on a Session level.
                                //
                                // Session Aggregate link char have url set to the
                                // RTSP Message URI and cannot be sent prior to PLAY

                                //Get hold of RTSP Message URI if not set already
                                if (!pRTSPBaseURI)
                                {
                                    ret = pRTSPMsg->QueryInterface(IID_IHXRTSPRequestMessage,
                                                                (void **)&pRTSPReqMsg);
                                    if (SUCCEEDED(ret))
                                    {
                                        pRTSPReqMsg->GetUrl(pRTSPBaseURI);
                                        if (!pRTSPBaseURI)
                                        {
                                            ret = HXR_FAIL;
                                        }
                                    }

                                    HX_RELEASE(pRTSPReqMsg);
                                }

                                //we don't want the trailing '/', if that is the only reason these
                                //two urls are different.
                                UINT32 ulBaseURILen = pRTSPBaseURI->GetSize();
                                UCHAR* pBaseURI = pRTSPBaseURI->GetBuffer();
                                if(pBaseURI[ulBaseURILen -1] == '/')
                                {
                                    ulBaseURILen--;
                                }
                                //Set Session Aggregate if link char URI
                                // matches RTSP Message URI
                                if (SUCCEEDED(ret) &&
                                        !strncmp(pURL, (const char *)pBaseURI, ulBaseURILen))
                                {
                                    linkCharParams.m_bSessionAggregate = TRUE;
                                }
                                else
                                {
                                    //VS: INCOMPLETE
                                    //Log an error message indicating reception of an bad URI
                                    ret = HXR_FAIL;
                                }
                            }
                            else
                            {
                                //VS: INCOMPLETE
                                //Log an error message indicating reception of an bad URI
                                ret = HXR_FAIL;
                            }

                            DELETE_FAST_TEMP_STR(pURL);
                            if (FAILED(ret))
                            {
                                HX_RELEASE(pBuffAttr);
                                HX_RELEASE(pBuffVal);
                                break;
                            }
                        }
                        else if (!strncasecmp(pcAttribute, "GBW", unAttrBuffSize))
                        {
                            const char* pcGuaranteedBW = (const char*)pBuffVal->GetBuffer();
                            linkCharParams.m_ulGuaranteedBW
                                            = (UINT32)strtol(pcGuaranteedBW, NULL, 10);
                        }
                        else if (!strncasecmp(pcAttribute, "MBW", unAttrBuffSize))
                        {
                            const char* pcMaxBW = (const char*)pBuffVal->GetBuffer();
                            linkCharParams.m_ulMaxBW
                                            = (UINT32)strtol(pcMaxBW, NULL, 10);
                        }
                        else if (!strncasecmp(pcAttribute, "MTD", unAttrBuffSize))
                        {
                            const char* pcMaxTransferDelay = (const char*)pBuffVal->GetBuffer();
                            linkCharParams.m_ulMaxTransferDelay
                                            = (UINT32)strtol(pcMaxTransferDelay, NULL, 10);
                        }

                        HX_RELEASE(pBuffAttr);
                        HX_RELEASE(pBuffVal);
                        pParamItr->MoveNext();
                    }

                    HX_RELEASE(pParamItr);
                    HX_RELEASE(pParamList);

                    // Pass on the initialized LinkCharParams to Player::Session
                    //  through RTSPProtocol
                    if (ret == HXR_OK)
                    {
                        if (HXR_OK != (ret = m_pResp->Handle3GPPLinkChar((const char*)sessionID,
                                                                linkCharParams)))
                        {
                            break;
                        }
                    }
                }

                pFieldItr->MoveNext();
            }
            HX_RELEASE(pFieldItr);
            HX_RELEASE(pFieldList);
        }

        HX_RELEASE(pRTSPBaseURI);
        HX_RELEASE(pMIMEHeader);
    }

    return ret;
}


/**
 * \brief HandleBandwidthHeader
 *
 * HandleBandwidthHeader parses a "Bandwidth" header in a client request
 * and calls m_pResp->HandleClientAvgBandwidth() to notify the packet flow.
 *
 * \param pSessionID [in] : session id
 * \param pRTSPMsg [in] : RTSP message to check
 *
 * \return HXR_OK if all processing of this header succeeds
 *         HXR_IGNORE if no bandwidth header was present
 *         Otherwise, FAIL code
 */
HX_RESULT
RTSPServerProtocol::HandleBandwidthHeader(const char* pSessionID,
                                          IHXRTSPMessage* pRTSPMsg)
{
    HX_RESULT res = HXR_OK;

    // Get the bandwidth header
    IHXMIMEHeader* pMIMEHeader = NULL;
    if (SUCCEEDED(res))
    {
        res = pRTSPMsg->GetHeader("Bandwidth", pMIMEHeader);

        if (FAILED(res))
            res = HXR_IGNORE;
    }

    // Process the bandwidth header
    if (SUCCEEDED(res))
    {
        UINT32 ulBandwidth = 0;
        res = pMIMEHeader->GetValueAsUint(ulBandwidth);

        if (SUCCEEDED(res))
            res = m_pResp->HandleClientAvgBandwidth(pSessionID, ulBandwidth);
    }

    HX_RELEASE(pMIMEHeader);

    HX_ASSERT(SUCCEEDED(res) || res == HXR_IGNORE);
    return HXR_OK;
}

