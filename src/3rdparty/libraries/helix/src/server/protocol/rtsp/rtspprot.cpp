/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspprot.cpp,v 1.86 2007/05/01 18:17:55 darrick Exp $
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

#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "platform.h"
#include "hxassert.h"
#include "debug.h"
#include "hxstrutl.h"
#include "cbqueue.h"
#include "fio.h"
#include "sio.h"
#include "config.h"
#include "url.h"
#include "protocol.h"
#include "hxtypes.h"
#include "servchallenge.h"
#include "player.h"
#include "client.h"
#include "hxdeque.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxprot.h"
#include "mimehead.h"
#include "servpckts.h"
#include "srcerrs.h"
#include "rtsputil.h"
#include "rtspmsg.h"
#include "rtspmdsc.h"
#include "rtsppars.h"
#include "rtspserv.h"
#include "plgnhand.h"
#include "streamer_container.h"
#include "timerep.h"
#include "client.h"
#include "rtspif.h"
#include "hxpssprofile.h"
#include "hxclientprofile.h"
#include "rtspprot.h"
#include "rtspservtran.h"
#include "server_context.h"
#include "server_info.h"
#include "hxservinfo.h"
#include "server_request.h"
#include "base_errmsg.h"
#include "hxpiids.h"
#include "hxreg.h"
#include "server_version.h"
#include "multicast_mgr.h"
#include "mcast_ctrl.h"
#include "netbyte.h"
#include "timerep.h"
#include "servbuffer.h"
#include "hxstats.h"
#include "server_stats.h"

#include "hxqossig.h"
#include "hxqos.h"
#include "hxqostran.h"
#include "hxqossig.h"
#include "hxqosinfo.h"
#include "qos_diffserv_cfg.h"

#include "smartptr.h"
#include "miscsp.h"
#include "unkimp.h"
#include "ihxlist.h"
#include "hxvalues.h"
#include "client_profile_mgr.h"

#include "sink.h"
#include "source.h"
#include "servpckts.h"
#include "hxservrdttran.h"
#include "hxservpause.h"
#include "hxsockutil.h"
#include "hxpcktflwctrl.h"

#if defined _WINDOWS
#define snprintf _snprintf
#endif

/*
 * RTSPProtocol methods
 */

RTSPProtocol::RTSPProtocol(void) :
    HXProtocol(),
    m_ulRefCount(0),
    m_pFileHeader(0),
    m_pPacketResend(0),
    m_pPlayer(NULL),
    m_pTransportContainer(0),
    m_bSentHeaders(FALSE),
    m_bTransportSetup(FALSE),
    m_pSessionHeaders(0),
    m_pMcstSessionContainer(0),
    m_url(0)
    , m_bNewRTSPMsgDone(FALSE),
    m_bRDT(FALSE),
    m_pPlayerStarttime(NULL),
    m_pCompanyID(NULL),
    m_pPlayerGUID(NULL),
    m_pClientChallenge(NULL),
    m_pProxyPeerAddr(NULL),
    m_bProtInfoSet(FALSE),
    m_uFirstSetupStreamNum(0xFFFF),
    m_pProtocol(NULL),
    m_ulClientAvgBandwidth(0)
{
    m_pVersionStr = new char[12];
    sprintf(m_pVersionStr, "RTSP/%d.%d",
    RTSPMessage::MAJ_VERSION,
    RTSPMessage::MIN_VERSION);

    m_bRuleOn = new BYTE[RULE_TABLE_WIDTH * RULE_TABLE_HEIGHT];
    memset(m_bRuleOn, 0, sizeof(BYTE) * RULE_TABLE_WIDTH * RULE_TABLE_HEIGHT);
}

RTSPProtocol::~RTSPProtocol(void)
{
    DPRINTF(D_INFO, ("~RTSPProtocol()\n"));
    /*
     * m_pProtocol should have already been released from the Done()
     * function call
     */

    if (m_pMcstSessionContainer)
    {
        MulticastSession* pMulticastSession;
        CHXMapStringToOb::Iterator i;
        for (i=m_pMcstSessionContainer->Begin();
            i!=m_pMcstSessionContainer->End();
            ++i)
        {
            pMulticastSession = (MulticastSession*)(*i);
            pMulticastSession->ReleaseCount();  // will clean up itself...
        }
        m_pMcstSessionContainer->RemoveAll();
        delete m_pMcstSessionContainer;
    }

    delete[] m_pVersionStr;
    HX_RELEASE(m_pProtocol);
    HX_RELEASE(m_pFileHeader);
    HX_RELEASE(m_pPacketResend);
    HX_RELEASE(m_pSessionHeaders);
    HX_RELEASE(m_pPlayerStarttime);
    HX_RELEASE(m_pCompanyID);
    HX_RELEASE(m_pPlayerGUID);
    HX_RELEASE(m_pClientChallenge);
    HX_RELEASE(m_pProxyPeerAddr);

    delete[] m_bRuleOn;

    HX_DELETE(m_url);
}

/*
 * IHXRTSPServerProtocolResponse2 methods
 */

STDMETHODIMP
RTSPProtocol::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPProtocolResponse))
    {
        AddRef();
        *ppvObj = (IHXRTSPProtocolResponse *)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPProtocolValuePass))
    {
        AddRef();
        *ppvObj = (IHXRTSPProtocolValuePass *)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPServerPauseResponse))
    {
        AddRef();
        *ppvObj = (IHXRTSPServerPauseResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXClientProfileManagerResponse))
    {
        AddRef();
        *ppvObj = (IHXClientProfileManagerResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
RTSPProtocol::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
RTSPProtocol::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

UINT32
RTSPProtocol::controlBytesSent()
{
    return m_pClient->m_uBytesSent + m_pProtocol->ControlBytesSent();
}

STDMETHODIMP
RTSPProtocol::HandleSessionHeaders(IHXValues* pSessionHeaders)
{
    if (m_pSessionHeaders)
    {
        m_pSessionHeaders->Release();
    }
    m_pSessionHeaders = pSessionHeaders;
    if (m_pSessionHeaders)
    {
        m_pSessionHeaders->AddRef();
    }
    return HXR_OK;
}

//XXXTDM: bRetainEntityForSetup is bogus, it needs tossed out
STDMETHODIMP
RTSPProtocol::AddSession(const char* pURLText, UINT32 ulSeqNo,
    CHXString& sessionID, BOOL bRetainEntityForSetup)
{
    // This is called from RTSPServerProtocol::HandleDescribe()

    Player::Session* pPlaySession = NULL;

    // Adds Session to Player
    m_pPlayer->NewSession(&pPlaySession, ulSeqNo, bRetainEntityForSetup);
    sessionID = pPlaySession->m_sessionID;

    // If this session created at SETUP time, pass any DESCRIBE "Bandwidth" info
    // received to it.
    if (m_ulClientAvgBandwidth)
    {
        pPlaySession->HandleClientAvgBandwidth(m_ulClientAvgBandwidth);
    }

    // Adds sessionItem to Protocol and SessionManager
    if (m_pProtocol->AddSession(sessionID, pURLText, ulSeqNo) != HXR_OK)
    {
        // Player generated a bogus session ID (this can't happen)
        HX_ASSERT(FALSE);
        m_pPlayer->RemoveSession(sessionID, HXR_FAIL);
        HX_RELEASE(pPlaySession);
        return HXR_UNEXPECTED;
    }

    // For proxies.
    //SetupClientStatsObject();

    if (client()->m_bIsAProxy)
    {
        pPlaySession->init_registry();
    }
    else
    {
        IHXSockAddr* pLocalAddr = NULL;
        IHXBuffer* pLocalAddrBuf = NULL;
        m_pSock->GetLocalAddr(&pLocalAddr);
        pLocalAddr->GetAddr(&pLocalAddrBuf);
        pPlaySession->init_stats(pLocalAddrBuf);
        HX_RELEASE(pLocalAddrBuf);
        HX_RELEASE(pLocalAddr);
    }

    // Set the Requester Context into the Request object
    //
    IUnknown* punkContext = NULL;
    RTSPServerProtocol::Session* pProtSession = NULL;
    pProtSession = m_pProtocol->getSession(sessionID);
    HX_ASSERT(pProtSession != NULL);
    pProtSession->QueryInterface(IID_IUnknown, (void**)&punkContext);

    pPlaySession->m_pRequest->SetRequester(punkContext);

    if (pPlaySession->m_ulSessionStatsObjId != 0)
    {
        pProtSession->m_ulSessionStatsObjId = pPlaySession->m_ulSessionStatsObjId;
    }

    HX_RELEASE(punkContext);
    HX_RELEASE(pPlaySession);

    return HXR_OK;
}

//XXXTDM: bRetainEntityForSetup is bogus, it needs tossed out
STDMETHODIMP
RTSPProtocol::AddSessionWithID(const char* pURLText, UINT32 ulSeqNo,
        CHXString& sessionID, BOOL bRetainEntityForSetup)
{
    // This should not be called anymore -- TDM

    Player::Session* pPlaySession = m_pPlayer->FindSession(sessionID);
    if (pPlaySession)
    {
        // Protocol tried to add an existing session
        HX_ASSERT(FALSE);
        HX_RELEASE(pPlaySession);
        return HXR_UNEXPECTED;
    }

    // Adds Session to Player
    m_pPlayer->NewSessionWithID(&pPlaySession, ulSeqNo, sessionID,
                                bRetainEntityForSetup);

    // If this session created at SETUP time, pass any DESCRIBE "Bandwidth" info
    // received to it.

    if (m_ulClientAvgBandwidth)
    {
        pPlaySession->HandleClientAvgBandwidth(m_ulClientAvgBandwidth);
    }

    // Adds sessionItem to Protocol and SessionManager
    if (m_pProtocol->AddSession(sessionID, pURLText, ulSeqNo) != HXR_OK)
    {
        // Protocol called us with a bogus session ID
        HX_ASSERT(FALSE);
        m_pPlayer->RemoveSession(sessionID, HXR_FAIL);
        return HXR_UNEXPECTED;
    }

    //SetupClientStatsObject();

    IHXSockAddr* pLocalAddr = NULL;
    IHXBuffer* pLocalAddrBuf = NULL;
    m_pSock->GetLocalAddr(&pLocalAddr);
    pLocalAddr->GetAddr(&pLocalAddrBuf);
    pPlaySession->init_stats(pLocalAddrBuf);
    HX_RELEASE(pLocalAddrBuf);
    HX_RELEASE(pLocalAddr);

    // Set the Requester Context into the Request object
    //
    IUnknown* punkContext = NULL;
    RTSPServerProtocol::Session* pProtSession = NULL;
    pProtSession = m_pProtocol->getSession(sessionID);
    HX_ASSERT(pProtSession != NULL);
    pProtSession->QueryInterface(IID_IUnknown, (void**)&punkContext);

    pPlaySession->m_pRequest->SetRequester(punkContext);

    if (pPlaySession->m_ulSessionStatsObjId != 0)
    {
        pProtSession->m_ulSessionStatsObjId = pPlaySession->m_ulSessionStatsObjId;
    }

    HX_RELEASE(punkContext);
    HX_RELEASE(pPlaySession);

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleStreamDescriptionRequest(const char* pUrlText,
    IHXValues* pRequestHeaders, const char* pSessionID, BOOL bUseRTP)
{
    HX_RESULT rc;

    UINT32 ulURLTextLen = strlen(pUrlText);
    if (m_url)
    {
        delete m_url;
    }
    m_url = new URL(pUrlText, ulURLTextLen);

    DPRINTF(D_INFO, ("HandleStreamDescriptionRequest for URL: %s\n",
        m_url->full));

    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    if (client()->m_bIsAProxy)
    {
        UINT32 ulSessionStatsObjId = 0;
        IHXBuffer* pId = NULL;

    // Pull id from RTSPServerProtocol, assign appropriate clientstats obj.
        SetupClientStatsObject();

        if (SUCCEEDED(pRequestHeaders->GetPropertyCString("SessionStatsObjId", pId)) && pId)
        {
            ulSessionStatsObjId = atoi((char*)pId->GetBuffer());
            pId->Release();
        }

        pSession->m_pStats = client()->get_client_stats()->GetSession(ulSessionStatsObjId);
        pSession->m_ulSessionStatsObjId = pSession->m_pStats->GetID();
        pSession->SetupQoSAdaptationInfo();
    }

    RegisterPlayerOptions(pRequestHeaders, pSession);

    char szProp[512];
    IHXBuffer* pBuffer = NULL;
    IHXBuffer* pURLBuf = NULL;
    Process* pProc = client()->proc;

    pURLBuf = new ServerBuffer(TRUE);
    pURLBuf->Set((UINT8*)pUrlText, ulURLTextLen + 1);

#ifndef PERF_NOCLIENTREG

    if (client()->use_registry_for_stats())
    {
        pProc->pc->registry->GetPropName(pSession->m_ulRegistryID, pBuffer, pProc);
        if (!pBuffer)
        {
            //XXX why does this lookup sometimes fail?
            return HXR_ABORT;
        }

        sprintf(szProp, "%-.400s.PlayerRequestedURL", pBuffer->GetBuffer(),
            client()->get_registry_conn_id());
        HX_RELEASE(pBuffer);
        pProc->pc->registry->AddStr(szProp, pURLBuf, pProc);
    }

#endif /* ndef PERF_NOCLIENTREG */
    pSession->m_pStats->SetPlayerRequestedURL(pURLBuf);
    HX_RELEASE(pURLBuf);

    // this is a hack but it will be obsoleted very soon by the new rtsp
    // server classes, which will pass parsed message through
    rc = pRequestHeaders->GetPropertyCString("Accept-Encoding", pBuffer);
    if (HXR_OK == rc)
    {
        if (strstr((const char*)pBuffer->GetBuffer(), "mei"))
        {
            pSession->m_bBlockTransfer = TRUE;
        }
        HX_RELEASE(pBuffer);
    }

    rc = pRequestHeaders->GetPropertyCString("If-Modified-Since", pBuffer);
    if (HXR_OK == rc)
    {
        UTCTimeRep utcIMS((char*)pBuffer->GetBuffer());
        pSession->m_tIfModifiedSince = utcIMS.asUTCTimeT();
        HX_RELEASE(pBuffer);
    }

    // Parse the url for query params relevant to setup
    ParseQueryParams(m_url->full, pRequestHeaders);

    pSession->m_pRequest->SetURL(m_url->full);
    pSession->m_pRequest->SetRequestHeaders(FS_HEADERS, pRequestHeaders);
    pSession->m_pRequest->SetRequestHeaders(FF_HEADERS, pRequestHeaders);

    //Need to get the client profile before we generate file/session headers
    //in Session::got_url
    return GetClientProfile(pUrlText, ulURLTextLen, pRequestHeaders,
                            pSessionID, bUseRTP);
}

STDMETHODIMP
RTSPProtocol::HandleStreamRecordDescriptionRequest(
    const char* pURLText,
    const char* pSessionID,
    IHXValues* pFileHeader,
    CHXSimpleList* pHeaders,
    IHXValues* pRequestHeaders)
{
    m_pProtocol->SendStreamRecordDescriptionResponse(HXR_NOTIMPL, pSessionID,
                                                                   NULL, NULL);
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleSetupRequest(
    HX_RESULT status
)
{
    DPRINTF(D_INFO, ("HandleSetupRequest\n"));
//XXXSMP Need to actually do the initial setup & registration with PPM from here
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandlePlayRequest(UINT32 lFrom, UINT32 lTo,
    CHXSimpleList* pSubscriptions, const char* pSessionID)
{
    DPRINTF(D_INFO, ("HandlePlayRequest\n"));

    if (!m_bTransportSetup)
    {
        return HXR_FAIL;
    }

    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_PE_SESSION_NOT_FOUND;
    }

    pSession->pause();

    MulticastSession* pMulticastSession = NULL;
    if (!m_pMcstSessionContainer ||
        !m_pMcstSessionContainer->Lookup(pSessionID, (void*&)pMulticastSession))
    {
        /*
         * Set sequence numbers into transport, only if this isn't multicast.
         * In a Multicast we share this resend buffer with everyone else and
         * should try to muck with it.
         *
         * Why is this needed at all?  RTP numbers are set from the PPM.
         * I removed this call, and the server seemed to serve RDT and RTP
         * just fine.  jmev, 6/2003
         */

        pSession->SetStreamSequenceNumbers(lFrom);
    }

    //XXXTDM: Check result and bail, then get rid of m_bTransportSetup.
    //   m_bTransportSetup is not only duplicate information, it is also
    //   per-socket, which does more harm than good.
    m_pProtocol->SetupSequenceNumberResponse(pSessionID);

    /* enforce that the client challenge is correct for RDT sessions */
    if (m_bRDT && !m_pPlayer->m_pClient->m_bIsAProxy)
    {
        BOOL bChallengeOK = FALSE;

#if defined(HELIX_FEATURE_RTSP_SERVER_CHALLENGE)
        if (m_pCompanyID != NULL &&
            m_pPlayerGUID != NULL &&
            m_pClientChallenge != NULL &&
            m_pPlayerStarttime != NULL)
        {
            UCHAR companyIDKey[HX_SERV_COMPANY_ID_KEY_SIZE];

            ServCalcCompanyIDKey((const char*)m_pPlayerStarttime->GetBuffer(),
                                 (const char*)m_pPlayerGUID->GetBuffer(),
                                 (const char*)m_pClientChallenge->GetBuffer(),
                                 (unsigned char*)&companyIDKey[0]);

            char szEncodedCompanyID [HX_SERV_COMPANY_ID_KEY_SIZE * 2];
            BinTo64((const BYTE*)companyIDKey,
                    HX_SERV_COMPANY_ID_KEY_SIZE, szEncodedCompanyID);

            bChallengeOK = FALSE;
            if (memcmp(szEncodedCompanyID, m_pCompanyID->GetBuffer(),
                      HX_SERV_COMPANY_ID_KEY_SIZE) == 0)
            {
                bChallengeOK = TRUE;
            }
        }
#endif /*HELIX_FEATURE_RTSP_SERVER_CHALLENGE*/

        if (!bChallengeOK)
        {
            IHXPlayerController* pController = NULL;
            IHXBuffer*           pMessage    = NULL;

            pSession->QueryInterface(IID_IHXPlayerController,
                                     (void**)&pController);

            if (pController != NULL)
            {
                pMessage = new ServerBuffer(TRUE);
                pMessage->Set((const UCHAR*)alert_table[SE_INVALID_PLAYER],
                              strlen(alert_table[SE_INVALID_PLAYER]) + 1);

                pController->AlertAndDisconnect(pMessage);

                HX_RELEASE(pMessage);
                HX_RELEASE(pController);
            }

            return HXR_NOT_AUTHORIZED;
        }
    }

    if (!pSession->m_bBlockTransfer)
    {
        if (lFrom == RTSP_PLAY_RANGE_BLANK && lTo == RTSP_PLAY_RANGE_BLANK)
        {
            pSession->begin();
        }
        else if (lFrom == lTo)
        {
            pSession->seek(lFrom);
        }
        else
        {
            if (lFrom != RTSP_PLAY_RANGE_BLANK)
            {
                pSession->seek(lFrom);
            }

            if (lTo != RTSP_PLAY_RANGE_BLANK)
            {
                /*
                 * lTo is in seconds, so translate to milliseconds
                 */

                pSession->set_endpoint(lTo);
            }

            pSession->begin();
        }
    }
    else
    {
        if(lFrom == RTSP_PLAY_RANGE_BLANK && lTo == RTSP_PLAY_RANGE_BLANK)
        {
            // this is an error, should shut down
            return HXR_ABORT;
        }

        pSession->set_byterange((UINT64)lFrom, (UINT64)lTo);

        pSession->begin();
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandlePauseRequest(const char* pSessionID)
{
    return PauseRequestHandler(pSessionID);
}

STDMETHODIMP
RTSPProtocol::HandleResumeRequest(const char* pSessionID)
{
    if (!m_bTransportSetup)
    {
            return HXR_FAIL;
    }

    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    pSession->begin();
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleTeardownResponse(HX_RESULT status)
{
    DPRINTF(D_INFO, ("HandleTeardownResponse\n"));
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleSetParameterRequest(UINT32 lParamType,
    const char* pParamName, IHXBuffer* pParamValue)
{
    DPRINTF(D_INFO, ("HandleSetParameterRequest: name %s\n", pParamName));
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleSetParameterRequest(const char* pSessionID,
        const char* pParamName, const char* pParamValue, const char* pContent)
{
    if (!strcmp(pParamName, "DataConvertBuffer"))
    {
        Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
        if (!pSession)
        {
            return HXR_ABORT;
        }

        pSession->ControlBufferReady(pContent);
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

STDMETHODIMP
RTSPProtocol::HandleSetParameterResponse(HX_RESULT status)
{
    DPRINTF(D_INFO, ("HandleSetParameterResponse\n"));
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleGetParameterRequest(UINT32 lParamType,
    const char* pParamName, IHXBuffer** pBuffer)
{
    DPRINTF(D_INFO, ("HandleGetParameterRequest\n"));
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleGetParameterResponse(HX_RESULT status,
    IHXBuffer* pParamValue)
{
    DPRINTF(D_INFO, ("HandleGetParameterResponse\n"));
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleProtocolError(HX_RESULT status)
{
    DPRINTF(D_INFO, ("HandleProtocolError: %ld\n", status));
    if (status == HXR_NET_SOCKET_INVALID || status == HXR_TIMEOUT ||
	status == HXR_INVALID_PROTOCOL)
    {
        HX_ASSERT(m_pClient != NULL);
        if (m_pClient != NULL)
        {
            m_pClient->OnClosed(status);
        }
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleRecordRequest(const char* pSessionID)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandlePacket(HX_RESULT status, const char* pSessionID,
    IHXPacket* pPacket)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleLimitBandwidthByDropping(UINT16 streamNumber,
                                             const char* pSessionID,
                                            UINT32 ulBandwidthLimit)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    pSession->set_dropto_bandwidth_limit(streamNumber, ulBandwidthLimit);
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleSpeedParam(const char* pSessionID, FIXED32 fSpeed)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    pSession->SetSpeed(HX_FIXED_TO_FLOAT(fSpeed));
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleScaleParam(const char* pSessionID, FIXED32 fScale)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    pSession->SetScale(HX_FIXED_TO_FLOAT(fScale));
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleSetDeliveryBandwidth(UINT32 ulBackOff,
                                         const char* pSessionID,
                                         UINT32 ulBandwidth)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    pSession->set_delivery_bandwidth(ulBackOff, ulBandwidth);
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleStreamDone(HX_RESULT status, UINT16 uStreamNumber)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::GenerateNewSessionID(REF(CHXString) sessionID, UINT32 ulSeqNo)
{
    if (!m_pPlayer)
    {
        HX_ASSERT(0);
        return HXR_NOT_INITIALIZED;
    }
    m_pPlayer->GenerateNewSessionID(sessionID, ulSeqNo);
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleSubscribe(CHXSimpleList* pSubList, const char* pSessionID)
{
    if (pSubList)
    {
        Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
        if (!pSession)
        {
            return HXR_ABORT;
        }

        CHXSimpleList::Iterator i;
        for (i=pSubList->Begin();i!=pSubList->End();++i)
        {
            RTSPSubscription* pSub = (RTSPSubscription*)(*i);
            //XXXBAB - add session to handle_subscribe
            pSession->handle_subscribe(pSub->m_ruleNumber,
                pSub->m_streamNumber);
            m_bRuleOn[(UINT32)pSub->m_ruleNumber * RULE_TABLE_HEIGHT +
                      (UINT32)pSub->m_streamNumber] = TRUE;
        }
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleSubscriptionDone(REF(UINT32)ulAddress,
    REF(UINT32) ulSourcePort, REF(UINT32) ulPort,
    const char* pSessionID, REF(TransportStreamHandler*) pHandler)
{
    if (m_pMcstSessionContainer)
    {
        MulticastSession* pMulticastSession = NULL;
        if (!m_pMcstSessionContainer->Lookup(pSessionID, (void*&)pMulticastSession))
        {
            // shouldn't be happening...
            HX_ASSERT(!"can't find multicast session");
            return HXR_ABORT;
        }

        HX_RESULT theErr = pMulticastSession->SubscriptionDone(m_bRuleOn,
                                                               ulSourcePort,
                                                               ulPort,
                                                               ulAddress,
                                                               pHandler);

        if (FAILED(theErr) && m_proc->pc->multicast_mgr->m_bMulticastOnly)
        {
            sendAlert(pSessionID, SE_NO_MORE_MULTI_ADDR);
            // XXXGo - Pnaprot.cpp put this string in sendAlert().
            // make sure to put this in sendAlert() later.
            char* pFileName;
            if (SUCCEEDED(pMulticastSession->GetFileName(pFileName)))
            {
                ERRMSG(m_proc->pc->error_handler,
                    "%ld: Error retrieving URL `%s' (%s)\n", m_pClient->conn_id,
                    pFileName, error_description_table[SE_NO_MORE_MULTI_ADDR]);
            }
            else
            {
                ERRMSG(m_proc->pc->error_handler,
                    "%ld: Error retrieving URL `\?\?\?' (%s)\n", m_pClient->conn_id,
                    error_description_table[SE_NO_MORE_MULTI_ADDR]);
            }
        }

        return theErr;
    }

    return HXR_ABORT;
}

STDMETHODIMP
RTSPProtocol::HandleUnsubscribe(CHXSimpleList* pUnsubList, const char* pSessionID)
{
    if (pUnsubList)
    {
        Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
        if (!pSession)
        {
            return HXR_ABORT;
        }

        CHXSimpleList::Iterator i;
        for (i=pUnsubList->Begin();i!=pUnsubList->End();++i)
        {
            RTSPSubscription* pSub = (RTSPSubscription*)(*i);
            //XXXBAB - add session to handle_usubscribe
            pSession->handle_unsubscribe(pSub->m_ruleNumber,
                pSub->m_streamNumber);
            m_bRuleOn[(UINT32)pSub->m_ruleNumber * RULE_TABLE_HEIGHT +
                      (UINT32)pSub->m_streamNumber] = FALSE;
        }
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleBackChannel(IHXPacket* pPacket, const char* pSessionID)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    pSession->handle_backchannel_packet(pPacket);

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleBWReport(HX_RESULT status, const char* pSessionID,
                INT32 aveBandwidth, INT32 packetLoss, INT32 bandwidthWanted)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandlePlayerStats(const char* pStats, const char* pSessionID)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    char str[512];
    ServerRegistry* registry = client()->proc->pc->registry;
    const char* pRegistryKey = pSession->m_pRegistryKey;

    UINT32 ulStatsLength = strlen(pStats);

    NEW_FAST_TEMP_STR(pTemp,
                      512,
                      ulStatsLength + 2);

    snprintf(pTemp,
             ulStatsLength + 2,
             "%s%s",
             pStats[0] != '[' ? "[" : "",
             pStats);

    pTemp[ulStatsLength + 1] = '\0';

    IHXBuffer* pLogStats = new ServerBuffer(TRUE);
    pLogStats->Set((const Byte*)pTemp, ulStatsLength + 2);

    DELETE_FAST_TEMP_STR(pTemp);

    if (client()->use_registry_for_stats() && pRegistryKey)
    {
        sprintf(str, "%-.400s.LogStats", pRegistryKey);
        if (FAILED(registry->SetStr(str, pLogStats, client()->proc)))
        {
            registry->AddStr(str, pLogStats, client()->proc);
        }
    }

    pSession->m_pStats->SetLogStats(pLogStats);
    HX_RELEASE(pLogStats);

    return HXR_OK;
}

#if 0
STDMETHODIMP
RTSPProtocol::OnPlayerStats(IHXBuffer* pStats, const char* pSessionID)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    char str[512];
    ServerRegistry* registry = client()->proc->pc->registry;
    const char* pRegistryKey = pSession->m_pRegistryKey;

    if (pRegistryKey && client()->use_registry_for_stats())
    {
        sprintf(str, "%-.400s.LogStats", pRegistryKey);
        registry->AddStr(str, pStats, client()->proc);
    }

    pSession->m_pStats->SetLogStats(pStats);

    /* get new stats interval */
    INT32 lStatsInterval = 0;
    registry->GetInt("config.StatsInterval", &lStatsInterval, client()->proc);
    m_pProtocol->SetStatsInterval((UINT32)lStatsInterval, pSessionID);

    return HXR_OK;
}
#endif

STDMETHODIMP
RTSPProtocol::HandleTeardownRequest(const char* pSessionID)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (pSession)
    {
        /// calls Player::Session::Done which updates the stats and 
        /// does other cleanup
        m_pPlayer->RemoveSession(pSessionID, HXR_OK);

        /// We can release our ref on the transport, the player is done with it
        releaseTransports(pSession);
    }

    // Server protocol should release its reference to the session too

//    if (m_pProtocol) m_pProtocol->SessionDone(pSessionID);

    return HXR_OK;
}

/*
 *  for now, it only cares about when it is NOT required.  Basically the same
 *  things as HandleTeardownRequest except it does extra registry cleanup.
 */
STDMETHODIMP
RTSPProtocol::HandleRetainEntityForSetup(const char* pSessionID, BOOL bRequired)
{
    if (!bRequired)
    {
            Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
            if (pSession)
            {
            pSession->clear_stats();
            m_pPlayer->RemoveSession(pSessionID, HXR_OK);
            }
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::SetMidBox(const char* pSessionID, BOOL bIsMidBox)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (pSession != NULL)
    {
        pSession->set_midbox(bIsMidBox);
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::AddTransport(Transport* pTransport, const char* pSessionID,
    UINT16 streamNumber, UINT32 ulReliability)
{
    if (m_uFirstSetupStreamNum == 0xFFFF)
    {
	m_uFirstSetupStreamNum = streamNumber;
    }

    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);

    //XXXBAB - this needs to be a list/map of transports
    if (!m_pTransportContainer)
    {
        m_pTransportContainer = new CHXSimpleList;
    }

#ifndef PERF_NOCLIENTREG
    ServerRegistry* registry = client()->proc->pc->registry;

    char str[64];
#endif /* ndef PERF_NOCLIENTREG */

    BOOL bIsUDP = FALSE;
    RTSPTransportTypeEnum TranType = pTransport->tag();

    if (IS_UDP_TRANSPORT(TranType))
    {
        bIsUDP = TRUE;
    }

    BOOL bUseRegistryForStats = FALSE;
    if (client()->use_registry_for_stats())
    {
        bUseRegistryForStats = TRUE;
    }

#ifndef PERF_NOCLIENTREG
    if (bUseRegistryForStats)
    {
        sprintf(str, "client.%ld.IsUDP", client()->get_registry_conn_id());
        registry->AddInt(str, bIsUDP, client()->proc);
    }
#endif /* ndef PERF_NOCLIENTREG */
    //client()->get_client_stats()->SetUDP(bIsUDP);
    HX_ASSERT(pSession);
    if (!pSession)
    { 
        return HXR_UNEXPECTED;
    }
    if (pSession->m_pStats)
    {
        pSession->m_pStats->SetUDP(bIsUDP);
    }
    else
    {
        HX_ASSERT(!"RTSPProtocol::AddTransport expected SessionStats object!");
    }

    switch (pTransport->tag())
    {
    case RTSP_TR_RDT_MCAST:
    case RTSP_TR_TNG_MCAST:
    case RTSP_TR_RDT_UDP:
    case RTSP_TR_TNG_UDP:
    case RTSP_TR_RDT_TCP:
    case RTSP_TR_TNG_TCP:


#ifndef PERF_NOCLIENTREG
            if (bUseRegistryForStats)
            {
                sprintf(str, "client.%ld.IsRDT", client()->get_registry_conn_id());
                registry->AddInt(str, 1, client()->proc);
            }
#endif /* ndef PERF_NOCLIENTREG */
            client()->get_client_stats()->SetRDT(TRUE);
            m_bRDT = TRUE;
            break;

    default:
#ifndef PERF_NOCLIENTREG
            if (bUseRegistryForStats)
            {
                sprintf(str, "client.%ld.IsRDT", client()->get_registry_conn_id());
                registry->AddInt(str, 0, client()->proc);
            }
#endif /* ndef PERF_NOCLIENTREG */
            client()->get_client_stats()->SetRDT(FALSE);
            break;
    }

    pTransport->AddRef();
    pTransport->AddStreamNumber(streamNumber);
    m_pTransportContainer->AddTail(pTransport);

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleStreamSetup(const char* pSessionID
			, UINT16 uStreamNumber, UINT16 uStreamGroupNumber)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
	return HXR_ABORT;
    }

    return pSession->HandleStreamSetup(uStreamNumber, uStreamGroupNumber);
}

STDMETHODIMP
RTSPProtocol::SetupTransports(const char* pSessionID)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    Transport* pTransport = getFirstTransportSetup(pSession, 0);
    if (pTransport == NULL)
    {
        // This should never happen, something failed in RTSP handler
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    if (pTransport->isRTSPMulticast())
    {
        BOOL bMulticastSetup = FALSE;
        MulticastSession* pMulticastSession = NULL;

        if (!m_pMcstSessionContainer)
        {
            m_pMcstSessionContainer = new CHXMapStringToOb();
        }

        if (m_pMcstSessionContainer)
        {
            const char* pURL;

            pSession->m_pRequest->GetURL(pURL);
            bMulticastSetup =
                m_proc->pc->multicast_mgr->RegisterMulticast
                (pURL, MMP_RTSP_RDT, pMulticastSession,
                 static_cast<IUnknown*>(m_proc->pc->server_context));

            HX_ASSERT(pMulticastSession);

            m_pMcstSessionContainer->SetAt(pSessionID, pMulticastSession);
        }

        /*
         * If this multicast session has already been setup then the following
         * call will set the transport data convert for the player session.
         * If it has not been setup yet, then the setup_transport call will
         * set it up.  Either way it needs to be there for the end of
         * setup_transport so the transport data convert get be notified
         * of the new control data convert.
         */
        if (pMulticastSession)
        {
            // XXX:TDK Really EVIL hack to break dependency. Code should really
            // be cleaned up in terms of using the interfaces IHXDataConvert
            // and IHXDataConvertResponse instead.
            DataConvertShim * pTransportConverter = 
                (DataConvertShim*) pMulticastSession->GetTransportConverter();
            if (pTransportConverter)
            {
                pSession->SetTransportConverter(pTransportConverter);
            }
        }

        pSession->setup_transport(pMulticastSession != 0,
                pTransport->isRTP(),
                bMulticastSetup ? pMulticastSession : 0);
    }
    else
    {
        pSession->setup_transport(FALSE, pTransport->isRTP(), NULL);
    }

    m_bTransportSetup = TRUE;

    if (m_pPacketResend)
    {
        m_pPacketResend->Release();
        m_pPacketResend = 0;
    }

    HX_RESULT rc;
    if ((!pTransport->isRTSPMulticast()) ||
        m_proc->pc->multicast_mgr->m_bMulticastResend == TRUE)
    {
        rc = pSession->m_pSessionControl->QueryInterface(
            IID_IHXPacketResend, (void**) &m_pPacketResend);
    }

    else
    {
        rc = HXR_OK;
        m_pPacketResend = NULL;
    }

    if (HXR_OK == rc)
    {
        m_pProtocol->SetPacketResend(m_pPacketResend, pSessionID);
    }

    return rc;
}

Transport*
RTSPProtocol::getTransport(Player::Session* pSession,
    UINT16 streamNumber, ULONG32 bIsReliable)
{
    //
    //XXXBAB - possibly should be implemented as a 2-dimensional map,
    // but for small N (almost all cases) I think this is fine...
    //
    CHXSimpleList::Iterator i;
    if (m_pTransportContainer)
    {
        for (i=m_pTransportContainer->Begin();
            i!=m_pTransportContainer->End();
            ++i)
        {
            Transport* pTransport = (Transport*)(*i);
            if (pSession->m_sessionID == pTransport->GetSessionID() &&
                pTransport->CarriesStreamNumber(streamNumber))
            {
                return pTransport;
            }
        }
    }
    return 0;
}

Transport*
RTSPProtocol::getFirstTransportSetup(Player::Session* pSession, ULONG32 bIsReliable)
{
    return getTransport(pSession, m_uFirstSetupStreamNum, bIsReliable);
}


void
RTSPProtocol::releaseTransports(Player::Session* pSession)
{
    LISTPOSITION    pos = 0;
    Transport* pTransport;

    if (!pSession || !m_pTransportContainer)
    {
        return;
    }

    pos = m_pTransportContainer->GetHeadPosition();

    while(pos)
    {
        LISTPOSITION    curpos = pos;
        pTransport = (Transport*)m_pTransportContainer->GetNext(pos);

        if ((pSession->m_sessionID == pTransport->GetSessionID()))
        {
            pTransport->Release();
            m_pTransportContainer->RemoveAt(curpos);
        }
    }

    return;
}


void
RTSPProtocol::init(Process* proc, IHXSocket* pSock)
{
    HX_ASSERT(pSock != NULL);

    m_proc = proc;
    m_pSock = pSock;
    m_pSock->AddRef();

    m_pClient = new Client(proc);
    m_pClient->AddRef();
    m_pPlayer = new Player(proc, m_pClient, 1);
    m_pPlayer->AddRef();
    m_pClient->init(HXPROT_RTSP, this);

    /* Set DiffServ Code Point */
    m_proc->pc->qos_diffserv_cfg->ConfigureSocket(pSock,
            HX_QOS_DIFFSERV_CLASS_CONTROL);

    /*
     * Allow the user to specify an extra server version component via the
     * config file.  This will be sent as a second "comment" in the string.
     *
     * We sanitize the input by checking for oversized strings (this would
     * cause snprintf to truncate the result) and disallowed characters (this
     * would cause an RFC violation).  If either are found, use the default.
     */
    char szBuildVer[256];
    const char* szVerExtra;
    szVerExtra = m_proc->pc->config->GetString(m_proc,
                                               "config.ServerVersionExtra");

    // Check length.  Reserve 80 chars for our own use plus the terminator.
    if (szVerExtra != NULL)
    {
        if (strlen(szVerExtra) > sizeof(szBuildVer) - (80+1))
        {
            szVerExtra = NULL;
        }
    }

    // Check reserved characters: parenthesis and CTLs.
    if (szVerExtra != NULL)
    {
        const char* p;
        for (p = szVerExtra; *p; p++)
        {
            if (*p == '(' || *p == ')' || *p < ' ')
            {
                szVerExtra = NULL;
                break;
            }
        }
    }

    // If the value was not specified or it was invalid, use the default.
    if (szVerExtra == NULL)
    {
        szVerExtra = "RealServer compatible";
    }

    snprintf(szBuildVer, sizeof(szBuildVer), "%s Version %s (%s) (%s)",
             ServerVersion::ProductName(), ServerVersion::VersionString(),
             ServerVersion::Platform(), szVerExtra);

    if (m_proc->pc->config->GetInt(m_proc, "config.DisablePlayerResend") == 1)
    {
        m_pProtocol = new RTSPServerProtocol(TRUE);
    }
    else
    {
        m_pProtocol = new RTSPServerProtocol();
    }

    m_pProtocol->AddRef();
    m_pProtocol->SetIsProxy(m_pClient->m_bIsAProxy);
    m_pProtocol->Init(m_pClient->proc->pc->server_context);
    m_pProtocol->SetResponse(pSock, this);
    m_pProtocol->SetBuildVersion(szBuildVer);

    IHXSockAddr* pLocalAddr = NULL;
    IHXSockAddr* pPeerAddr = NULL;
    BOOL bIsCloak = FALSE;

    m_pSock->GetLocalAddr(&pLocalAddr);
    m_pSock->GetPeerAddr(&pPeerAddr);
    bIsCloak = (m_pSock->GetFamily() == HX_SOCK_FAMILY_CLOAK);

    m_pClient->init_stats(pLocalAddr, pPeerAddr, bIsCloak);

    HX_RELEASE(pPeerAddr);
    HX_RELEASE(pLocalAddr);

    if (!m_pClient->m_bIsAProxy)
    {
        m_pProtocol->SetClientStatsObj(m_pClient->get_client_stats());
    }
}

int
RTSPProtocol::setupHeader(IHXValues* pHeader, Player::Session* pSession, HX_RESULT status)
{
    if (HXR_OK == status)
    {
        if (m_pFileHeader)
        {
            m_pFileHeader->Release();
        }
        m_pFileHeader = pHeader;
        m_pFileHeader->AddRef();
    }
    return 0;
}

int
RTSPProtocol::addToHeader(const char* pName, IHXBuffer* pValue)
{
    if (m_pFileHeader)
    {
        m_pFileHeader->SetPropertyBuffer(pName, pValue);
        return 0;
    }
    return -1;
}

int
RTSPProtocol::setupStreams(CHXSimpleList* pHeaders, Player::Session* pSession,
                            HX_RESULT status)
{
    DPRINTF(D_INFO, ("setupStreams: status %ld\n", status));
    HX_RESULT result = HXR_OK;

    if (!m_pClient)
        return HXR_UNEXPECTED;

    if (HXR_OK != status)
    {
        if (HXR_NOT_AUTHORIZED == status)
        {
            IHXValues* pIHXValuesResponseHeaders = NULL;

            pSession->m_pRequest->GetResponseHeaders(FS_HEADERS,
                                                pIHXValuesResponseHeaders);

            sendSetupStreamResponse(HXR_NOT_AUTHORIZED, pSession, NULL,
                                    NULL, NULL, pIHXValuesResponseHeaders);
            HX_RELEASE(pIHXValuesResponseHeaders);
        }
        else
        {
            if (HXR_REDIRECTION == status)
            {
                sendSetupStreamResponse(HXR_REDIRECTION, pSession, NULL, NULL, NULL,
                    NULL);
            }
            else
            {
                if (HXR_PARSE_ERROR != status)
                {
                    ERRMSG(m_proc->pc->error_handler,
                        "%ld: Error retrieving URL `%s' (%s)\n",
                        m_pClient->conn_id,
                                    m_url->len <= 4000 ? m_url->full :
                        "[long URL omitted for safety]",
                                        error_description_table[SE_INVALID_PATH]);
                }
                sendSetupStreamResponse(status, pSession, NULL, NULL, NULL,
                                        NULL);
            }
        }
    }
    else
    {
        IHXValues* pHdrs = NULL;
        IHXKeyValueList* pKeyedHdrs = NULL;
        IHXValues* pResponseHeaders = NULL;

        // Create a new keyed list for the combined response headers
        // and query for all of the interfaces that we will need

        CKeyValueList *pList = new CKeyValueList;
        pList->QueryInterface(IID_IHXKeyValueList, (void**)&pKeyedHdrs);
        pList->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders);
        pList = NULL;  // no AddRef/Release here is ok

        // XXX showell - We can come back to this code later and
        // eliminate pResponseHeaders, by changing the sendStreamResponse
        // functions to use IHXKeyValueList, not IHXValues.  It's a fair
        // bit of work.

        /*
         * Get the File Format headers first as they have precedence
         */

        result = pSession->m_pRequest->GetResponseHeaders(FF_HEADERS, pHdrs);

        if (HXR_OK != result)
        {
            sendSetupStreamResponse(HXR_FAILED, pSession, NULL, NULL, NULL,
                                    NULL);
            goto exit;
        }

        if (pSession->m_tIfModifiedSince != INVALID_TIME_T &&
            pSession->m_ulLastModifiedTime <= pSession->m_tIfModifiedSince)
        {
            sendSetupStreamResponse(HXR_NOT_MODIFIED, pSession,
                                    NULL, NULL, NULL,  NULL);
            goto exit;
        }

        // Add all File Format response headers to the cumulative list
        addHeaders(pKeyedHdrs, pHdrs);
        HX_RELEASE(pHdrs);

        /*
         * Now get the File System headers
         */

        result = pSession->m_pRequest->GetResponseHeaders(FS_HEADERS, pHdrs);

        if (HXR_OK != result)
        {
            sendSetupStreamResponse(HXR_FAILED, pSession, NULL, NULL, NULL,
                                    NULL);
            goto exit;
        }

        // Add all File System response headers to the cumulative list
        addHeaders(pKeyedHdrs, pHdrs);
        HX_RELEASE(pHdrs);

        if ((pSession->m_ulLastModifiedTime > 0) &&
            !pKeyedHdrs->KeyExists("Last-Modified"))
        {
            IHXBuffer* pValue = new ServerBuffer(TRUE);

            UTCTimeRep fileTime;
            fileTime.SetUTCTime(pSession->m_ulLastModifiedTime);
            UINT8* pTemp = (UINT8*)fileTime.asRFC1123String();
            pValue->Set(pTemp, strlen((char *)pTemp) + 1);

            pKeyedHdrs->AddKeyValue("Last-Modified",pValue);
            pValue->Release();
        }

        IHXValues* pReqHdrs = NULL;
        IHXBuffer* pUserAgent = NULL;
        UINT32 ulVal = 0;
        if (SUCCEEDED(pSession->m_pRequest->GetRequestHeaders(FS_HEADERS,
            pReqHdrs)))
        {
            if (FAILED(pReqHdrs->GetPropertyCString("user-agent", pUserAgent)))
            {
                pUserAgent = NULL;
            }

            if (SUCCEEDED(pReqHdrs->GetPropertyULONG32(
                "x-wap-profile-warning", ulVal)))
            {
                const UINT32 ulBuffLen = 12;
                IHXBuffer* pBuff = new ServerBuffer(TRUE);
                char pszVal[ulBuffLen];

                sprintf(pszVal, "%lu", ulVal);
                pszVal[ulBuffLen - 1] = '\0';
                pBuff->Set((UCHAR*)pszVal, ulBuffLen);
                pKeyedHdrs->AddKeyValue("x-wap-profile-warning", pBuff);
                HX_RELEASE(pBuff);
            }
        }
        HX_RELEASE(pReqHdrs);

        IHXValues* pOptionalValues = pSession->m_pSessionHeader;

        //Disabled assert due to high frequency of hitting this assert, tracking with PR 169049.
        //HX_ASSERT(pOptionalValues);
        if (!pOptionalValues)
        {
            pOptionalValues = new CHXHeader;
        }
        if (pOptionalValues)
        {
            pOptionalValues->AddRef();
            if (pSession->m_ulLastModifiedTime)
            {
                pOptionalValues->SetPropertyULONG32("LastModified",
                                            pSession->m_ulLastModifiedTime);
            }
            if (pUserAgent)
            {
                pOptionalValues->SetPropertyCString("UserAgent", pUserAgent);
            }

            IHXClientProfileInfo* pProfile;
            if (SUCCEEDED(pSession->m_pStats->GetClientProfileInfo(pProfile)))
            {
                HXCPAttribute attr;
                UINT32 ulType = HX_CP_TYPE_STR;
                if (SUCCEEDED(pProfile->GetAttribute(HX_CP_ATTR_PSS_VERSION,
                    ulType, attr)))
                {
                    pOptionalValues->SetPropertyCString("PSSVersion",
                        attr.pString);
                    HX_RELEASE(attr.pString);
                }
                HX_RELEASE(pProfile);
            }
        }
        result = sendSetupStreamResponse(HXR_OK, pSession, m_pFileHeader,
                                pHeaders, pOptionalValues, pResponseHeaders);

        HX_RELEASE(pOptionalValues);
        HX_RELEASE(pKeyedHdrs);
        HX_RELEASE(pResponseHeaders);
        HX_RELEASE(pUserAgent);

        if (HXR_OK != result)
        {
            goto exit;
        }
    }

exit:
    return result;
}

int
RTSPProtocol::playDone(const char* pSessionID)
{
    DPRINTF(D_INFO, ("playDone: session=%s\n", pSessionID));

    if (!m_pProtocol)
    {
        return HXR_UNEXPECTED;
    }

    m_pProtocol->PlayDone(pSessionID);

    return HXR_OK;
}

HX_RESULT
RTSPProtocol::addHeaders(IHXKeyValueList* pDestination, IHXValues* pSource)
{
    HX_RESULT res = HXR_OK;
    IHXKeyValueList* pKeyedSource = NULL;
    const char* pName = NULL;
    IHXBuffer* pValue = NULL;

    if (pSource && pDestination)
    {
        // Find out if the source supports IHXKeyedIList
        pSource->QueryInterface(IID_IHXKeyValueList, (void**)&pKeyedSource);

        if (pKeyedSource)
        {
            pDestination->AppendAllListItems(pKeyedSource);
            HX_RELEASE(pKeyedSource);
        }
        else
        {
            // import from IHXValues
            res = pSource->GetFirstPropertyCString(pName, pValue);
            while (res == HXR_OK)
            {
                pDestination->AddKeyValue(pName,pValue);
                HX_RELEASE(pValue);
                res = pSource->GetNextPropertyCString(pName, pValue);
            }
        }
    }

    return HXR_OK;
}

HX_RESULT
RTSPProtocol::sendSetupStreamResponse(HX_RESULT HX_RESULTStatus,
    Player::Session* pSessionCurrent,
    IHXValues* pIHXValuesFileHeader,
    CHXSimpleList* pCHXSimpleListStreamHeaders,
    IHXValues* pIHXValuesOptional,
    IHXValues* pIHXValuesResponse
)
{
    if (!m_pProtocol)
    {
        return HXR_UNEXPECTED;
    }

    BOOL bIsMulticast = FALSE;
    BOOL bIsMulticastRequired = FALSE;
    BOOL bInControlList = FALSE;

    if (m_proc->pc->multicast_mgr)
    {
        if (!m_proc->pc->mcast_ctrl->RulesArePresent())
        {
            // no rule...
            ERRMSG(m_proc->pc->error_handler,
               "Back-channel multicast is enabled and the control list is "
               "empty.  No clients will receive multicast.  Please add a control list.");
        }
        else
        {
	    bInControlList = IsClientAddressAllowedToReceiveMcast();
        }
    }

    if (pSessionCurrent->m_pSourceControl)
    {
        bIsMulticast = pSessionCurrent->m_pSourceControl->IsLive() &&
                  m_proc->pc->multicast_mgr && bInControlList;

        if (bIsMulticast)
        {
            bIsMulticastRequired = m_proc->pc->multicast_mgr->m_bMulticastOnly;
        }
    }

    // pass on advise object, if any

    if (pSessionCurrent->GetFFAdviseObj())
    {
        m_pProtocol->SetFFHeaderAdvise(pSessionCurrent->GetFFAdviseObj(), pSessionCurrent->m_sessionID);
    }

    return m_pProtocol->SendStreamResponse(HX_RESULTStatus,
                                            pSessionCurrent->m_sessionID,
                                            pIHXValuesFileHeader,
                                            pCHXSimpleListStreamHeaders,
                                            pIHXValuesOptional,
                                            pIHXValuesResponse, bIsMulticast,
                                            bIsMulticastRequired,
                                            pSessionCurrent->m_bIsRealDataType);
}

//Calls mcast_ctrl to validate client address.
BOOL RTSPProtocol::IsClientAddressAllowedToReceiveMcast()
{
    BOOL bIsAddressAllowed = FALSE;
    IHXSockAddr* pPeerAddr = 0;
    IHXSockAddr* pIPV4PeerAddr = 0;
    IHXBuffer* pIAddrBuffer = 0;
    IHXNetServices* pNetSvc = 0;

    if (m_pSock->GetFamily() == HX_SOCK_FAMILY_LBOUND)
    {
        HX_ASSERT(!m_pClient || m_pClient->m_bIsAProxy);
     
        pIAddrBuffer = m_pProxyPeerAddr;
        HX_ADDREF(pIAddrBuffer);
    }
    else 
    {
        if (m_pSock)
        {
	    m_pSock->GetPeerAddr(&pPeerAddr);
        }

        m_pClient->proc->pc->server_context->QueryInterface(IID_IHXNetServices, (void**)&pNetSvc);

        if (pNetSvc && pPeerAddr)
        {
	    //mcast_ctrl only supports ipv4 validation, hece covert address to ipv4.
	    HXSockUtil::ConvertAddr(pNetSvc, HX_SOCK_FAMILY_IN4, pPeerAddr, pIPV4PeerAddr);

	    if (pIPV4PeerAddr)
	    {
	        pIPV4PeerAddr->GetAddr(&pIAddrBuffer);
	    }
        }
    }

    if (pIAddrBuffer)
    {
	bIsAddressAllowed = m_proc->pc->mcast_ctrl->IsValidAddress((const char*)pIAddrBuffer->GetBuffer());
    }

    HX_RELEASE(pNetSvc);
    HX_RELEASE(pIAddrBuffer);
    HX_RELEASE(pIPV4PeerAddr);
    HX_RELEASE(pPeerAddr);

    return bIsAddressAllowed;
}

int
RTSPProtocol::disconnect(const char* pSessionID)
{
    m_pProtocol->Disconnect(pSessionID);
    return 0;
}

int
RTSPProtocol::sendAlert(const char* pSessionID, StreamError err)
{
    if (m_pProtocol)
    {
        if (0 >= err || err >= sizeof(alert_table))
        {
            err = SE_INTERNAL_ERROR;
        }

        const char* pAlert = alert_table[err];

        if (pAlert)
        {
            m_pProtocol->SendAlertRequest(pSessionID, err, pAlert);
        }
        else
        {
            m_pProtocol->SendAlertRequest(pSessionID, err, "");
        }
    }

    return 0;
}

int
RTSPProtocol::sendAlert(const char* pSessionID, IHXBuffer* pAlert)
{
    if (m_pProtocol && pAlert)
    {
        const char* pAlertText = NULL;

        if (pAlert)
        {
            pAlertText = (const char*)pAlert->GetBuffer();
        }

        if (pAlertText)
        {
            m_pProtocol->SendAlertRequest(pSessionID, 0, pAlertText);
        }
        else
        {
            m_pProtocol->SendAlertRequest(pSessionID, 0, "");
        }
    }

    return 0;
}

int
RTSPProtocol::sendRedirect(const char* pSessionID,
                           const char* pURL,
                           UINT32 ulSecsFromNow)
{
    if (m_pProtocol)
    {
        m_pProtocol->SendRedirectRequest(pSessionID, pURL, ulSecsFromNow);
    }

    return 0;
}

int
RTSPProtocol::sendProxyRedirect(const char* pSessionID,
                                       const char* pURL)
{
    if (m_pProtocol)
    {
        m_pProtocol->SendProxyRedirectRequest(pSessionID, pURL);
    }

    return 0;
}

void
RTSPProtocol::sendPacket(BasePacket* pPacket, const char* pSessionID)
{
    if (m_pProtocol)
    {
        m_pProtocol->SendPacket(pPacket, pSessionID);
    }
}

////////////////////////////////////////////////////////////////////////
//              RTSPProtocol::SessionDone
////////////////////////////////////////////////////////////////////////
//
// Pass a SessionDone down to the server protocol member.
// Used by the client to pass a SessionDone down to the server protocol.
// (Usually initiated by a player getting a teardown request.)
//
// jmevissen, 12/2000

void
RTSPProtocol::SessionDone(const char* pSessionID)
{
    if (m_pProtocol) m_pProtocol->SessionDone(pSessionID);
}

void
RTSPProtocol::Done(HX_RESULT status)
{
    if (m_pClient)
    {
        m_pClient->update_stats();
    }

    if (m_pPlayer)
    {
        m_pPlayer->Done(status);
    }
    HX_RELEASE(m_pPlayer);

    if (m_pTransportContainer)
    {
        CHXSimpleList::Iterator i;
        for (i=m_pTransportContainer->Begin();
            i!=m_pTransportContainer->End();
            ++i)
        {
            Transport* pTransport = (Transport*)(*i);
            pTransport->Release();
        }
        delete m_pTransportContainer;
        m_pTransportContainer = NULL;
    }

    if (m_pProtocol)
    {
        m_pProtocol->Done();
    }
    HX_RELEASE(m_pProtocol);

    HX_RELEASE(m_pPacketResend);

    // fprintf(stderr, "%ld: RTSPP::Done(%p)\n", m_pClient->conn_id, this);
    HX_RELEASE(m_pClient);

    HXProtocol::Done(status);
}

void
RTSPProtocol::SendKeepAlive()
{
    if (m_pProtocol)
    {
        m_pProtocol->SendKeepAlive();
    }
}

HX_RESULT
RTSPProtocol::SendSetParam(const char* pUrl, const char* pSessionID,
                           const char* pName, const char* pValue,
                           const char* pMimeType, const char* pContent)
{
    if (m_pProtocol)
    {
        return m_pProtocol->SendSetParameterRequest(pSessionID, pUrl,
                pName, pValue, pMimeType, pContent);
    }
    return HXR_NOT_INITIALIZED;
}

HX_RESULT
RTSPProtocol::sendBandwidthLimitRequest(const char* pSessionID,
                                        const char* pURL,
                                        ULONG32 ulBandwidth)
{
    if (m_pProtocol)
    {
        char buf[11];
        IHXBuffer* pBandwidth;

        pBandwidth = new ServerBuffer(TRUE);

        sprintf (buf,"%lu",ulBandwidth);
        pBandwidth->Set((UCHAR*)buf,strlen(buf)+1);
        m_pProtocol->SendSetParameterRequest(pSessionID,
                                             pURL,
                                             "MaximumASMBandwidth",
                                              pBandwidth);

        pBandwidth->Release();

        return HXR_OK;
    }

    return HXR_FAILED;
}

void
RTSPProtocol::RegisterPlayerOptions(IHXValues* pRequestHeaders,
    Player::Session* pSession)
{
    HX_RESULT       hResult             = HXR_OK;
    Process*        pProc               = NULL;
#ifndef PERF_NOCLIENTREG
    ServerRegistry* pRegistry           = NULL;
#endif /* ndef PERF_NOCLIENTREG */
    IHXBuffer*      pValue              = NULL;
    UINT32          ulRegistryConnId    = 0;
    UINT32          ulClientStatsObjId  = 0;
    char szProp[256];
    IHXValues*      pHeader    = m_pSessionHeaders;
    if (!pHeader)
    {
        // if a client is not RM, it is possible not to have OPTIONS msg...
        // try using the header that's just passed in...
        // NOTE:  most info is missing, though...
        pHeader = pRequestHeaders;
        if (!pHeader)
        {
            return;
        }
    }

    pProc     = client()->proc;
#ifndef PERF_NOCLIENTREG
    pRegistry = pProc->pc->registry;
    ulRegistryConnId = client()->get_registry_conn_id();
#endif /* ndef PERF_NOCLIENTREG */

    HX_ASSERT(client()->get_client_stats());

    ulClientStatsObjId = client()->get_client_stats()->GetID();

    // Add guid string to client.# entry in the registry
    // and to the 822 Request headers
    // if DisableClientGuid is ON, the zero the guid
    // before setting to the headers and registry

    hResult = pHeader->GetPropertyCString("GUID", pValue);

    if (m_proc->pc->config->GetInt(m_proc, "config.DisableClientGuid") == 1)
    {
        IHXBuffer* pGUID = new ServerBuffer(TRUE);
        const char p0GUID[] = "00000000-0000-0000-0000-000000000000";
        pGUID->Set((UINT8 *)p0GUID, sizeof(p0GUID));
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.GUID", ulRegistryConnId);
            pRegistry->AddStr(szProp, pGUID, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */

        if (!client()->m_bIsAProxy)
        {
            client()->get_client_stats()->SetGUID(pGUID);
        }
        pRequestHeaders->SetPropertyCString("GUID", pGUID);
        HX_RELEASE(pGUID);
    }
    else
    {
        if (pValue != NULL)
        {
#ifndef PERF_NOCLIENTREG
            if (client()->use_registry_for_stats())
            {
                sprintf(szProp, "client.%ld.GUID", ulRegistryConnId);
                pRegistry->AddStr(szProp, pValue, pProc);
            }
#endif /* ndef PERF_NOCLIENTREG */
            if (!client()->m_bIsAProxy)
            {
                client()->get_client_stats()->SetGUID(pValue);
            }
            pRequestHeaders->SetPropertyCString("GUID", pValue);
        }
    }

    if (pValue != NULL)
    {
        HX_RELEASE(m_pPlayerGUID);
        m_pPlayerGUID = pValue;
        m_pPlayerGUID->AddRef();
    }

    HX_RELEASE(pValue);  //client GUID

    // Add clientID string to client.# entry in the registry
    // and to the 822 Request headers
    hResult = pHeader->GetPropertyCString("ClientID", pValue);
    if (HXR_OK == hResult)
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.ClientID", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        if (!client()->m_bIsAProxy)
        {
            client()->get_client_stats()->SetClientID(pValue);
        }
        pRequestHeaders->SetPropertyCString("ClientID", pValue);
        HX_RELEASE(pValue);
    }

    // Add CompanyID string to client.# entry in the registry
    hResult = pHeader->GetPropertyCString("CompanyID", pValue);
    if (HXR_OK == hResult)
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.CompanyID", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        if (!client()->m_bIsAProxy)
        {
            client()->get_client_stats()->SetCompanyID(pValue);
        }

        HX_RELEASE(m_pCompanyID);
        m_pCompanyID = pValue;
        m_pCompanyID->AddRef();

        HX_RELEASE(pValue);
    }

    // Add ClientChallenge string to client.# entry in the registry
    hResult = pHeader->GetPropertyCString("ClientChallenge", pValue);
    if (HXR_OK == hResult)
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.ClientChallenge", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        if (!client()->m_bIsAProxy)
        {
            client()->get_client_stats()->SetClientChallenge(pValue);
        }

        HX_RELEASE(m_pClientChallenge);
        m_pClientChallenge = pValue;
        m_pClientChallenge->AddRef();

         HX_RELEASE(pValue);
     }

    // Add User-Agent string to client.# entry in the registry
    // and to the 822 Request headers
    hResult = pHeader->GetPropertyCString("User-Agent", pValue);
    if (HXR_OK == hResult)
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.User-Agent", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        if (!client()->m_bIsAProxy)
        {
            client()->get_client_stats()->SetUserAgent(pValue);
        }
        pRequestHeaders->SetPropertyCString("User-Agent", pValue);
        HX_RELEASE(pValue);
    }

    // Add playerStarttime string to client.# entry in the registry
    // and to the 822 Request headers
    hResult = pHeader->GetPropertyCString("PlayerStarttime", pValue);
    if (HXR_OK == hResult)
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.PlayerStarttime", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        if (!client()->m_bIsAProxy)
        {
            client()->get_client_stats()->SetPlayerStartTime(pValue);
        }
        pRequestHeaders->SetPropertyCString("PlayerStarttime", pValue);

        HX_RELEASE(m_pPlayerStarttime);
        m_pPlayerStarttime = pValue;
        m_pPlayerStarttime->AddRef();

        HX_RELEASE(pValue);
    }

    // Add playerStarttime string to client.# entry in the registry
    // and to the 822 Request headers
    hResult = pHeader->GetPropertyCString("LoadTestPassword", pValue);
    if (HXR_OK == hResult)
    {
        IHXBuffer* pBuffer = new ServerBuffer(TRUE);
        pBuffer->SetSize(pValue->GetSize()); // Overkill
        INT32 length = BinFrom64((const char*)pValue->GetBuffer(),
                               pValue->GetSize(), (unsigned char*)pBuffer->GetBuffer());
        if (length > 0)
        {
            pBuffer->SetSize(length);
  
#ifndef PERF_NOCLIENTREG
            if (client()->use_registry_for_stats())
            {
                sprintf(szProp, "client.%ld.LoadTestPassword", ulRegistryConnId);
                pRegistry->AddBuf(szProp, pBuffer, pProc);
            }
#endif /* ndef PERF_NOCLIENTREG */
            if (!client()->m_bIsAProxy)
            {
                client()->get_client_stats()->SetLoadTestPassword(pBuffer);
            }
        }
        HX_RELEASE(pValue);
        HX_RELEASE(pBuffer);
    }

    // Add ConnID value to 822 Request headers
    if (client()->use_registry_for_stats())
    {
        pValue = new ServerBuffer(TRUE);
        sprintf(szProp, "%lu", ulRegistryConnId);
        pValue->Set((UCHAR*)szProp, strlen(szProp) + 1);
        pRequestHeaders->SetPropertyCString("ConnID", pValue);
        HX_RELEASE(pValue);
    }

    pValue = new ServerBuffer(TRUE);
    sprintf(szProp, "%lu", ulClientStatsObjId);
    pValue->Set((UCHAR*)szProp, strlen(szProp) + 1);
    pRequestHeaders->SetPropertyCString("ClientStatsObjId", pValue);
    HX_RELEASE(pValue);


    // Add SessionNumber to request headers.
    if (client()->use_registry_for_stats())
    {
        pValue = new ServerBuffer(TRUE);
        sprintf(szProp, "%lu", pSession->m_ulSessionRegistryNumber);
        pValue->Set((UCHAR*)szProp, strlen(szProp) + 1);
        pRequestHeaders->SetPropertyCString("SessionNumber", pValue);
        HX_RELEASE(pValue);
    }

    pValue = new ServerBuffer(TRUE);
    sprintf(szProp, "%lu", pSession->m_ulSessionStatsObjId);
    pValue->Set((UCHAR*)szProp, strlen(szProp) + 1);
    pRequestHeaders->SetPropertyCString("SessionStatsObjId", pValue);
    HX_RELEASE(pValue);

    // Add Bandwidth string to client.# entry in the registry
    hResult = pRequestHeaders->GetPropertyCString("Bandwidth", pValue);
    if (HXR_OK == hResult)
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.Bandwidth", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */

        HX_RELEASE(pValue);
    }

    // Add Language string to client.# entry in the registry
    hResult = pRequestHeaders->GetPropertyCString("Language", pValue);
    if (HXR_OK == hResult)
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.Language", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        if (!client()->m_bIsAProxy)
        {
            client()->get_client_stats()->SetLanguage(pValue);
        }
        HX_RELEASE(pValue);
    }

    if (1) // Multicast is not yet supported in RTSP
    {
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            // Add supportsMulticast to client.# entry in the registry
            sprintf(szProp, "client.%ld.SupportsMulticast", ulRegistryConnId);
            pRegistry->AddInt(szProp, 0, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        client()->get_client_stats()->SetSupportsMulticast(FALSE);
    }

    // Add ServerSureStream Version string to client.# entry in the registry
    hResult = pRequestHeaders->GetPropertyCString(
        "SupportsMaximumASMBandwidth", pValue);
    if (HXR_OK != hResult)
    {
        // If the version number is not there specify version 0
        pValue = new ServerBuffer(TRUE);
        pValue->Set((UCHAR*)"0",2);
#ifndef PERF_NOCLIENTREG
        if (client()->use_registry_for_stats())
        {
            sprintf(szProp, "client.%ld.SupportsMaximumASMBandwidth", ulRegistryConnId);
            pRegistry->AddStr(szProp, pValue, pProc);
        }
#endif /* ndef PERF_NOCLIENTREG */
        client()->get_client_stats()->SetSupportsMaximumASMBandwidth(FALSE);
    }

    HX_RELEASE(pValue);
}

STDMETHODIMP
RTSPProtocol::OnOptionsRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnOptionsRequest()\n");
    // m_proc->pc->server_info->IncrementOptionsMsgParsedCount(m_proc);
    if(!m_bProtInfoSet)
    {
        m_pClient->update_protocol_statistics_info(m_pProtocol->m_clientType);
        m_bProtInfoSet = TRUE;
    }
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnDescribeRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnDescribeRequest()\n");
    // m_proc->pc->server_info->IncrementDescribeMsgParsedCount(m_proc);
    if(!m_bProtInfoSet)
    {
        m_pClient->update_protocol_statistics_info(m_pProtocol->m_clientType);
        m_bProtInfoSet = TRUE;
    }
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnSetupRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnSetupRequest()\n");
    // m_proc->pc->server_info->IncrementSetupMsgParsedCount(m_proc);
    if(!m_bProtInfoSet)
    {
        m_pClient->update_protocol_statistics_info(m_pProtocol->m_clientType);
        m_bProtInfoSet = TRUE;
    }
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnSetParamRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnSetParamRequest()\n");
    // m_proc->pc->server_info->IncrementSetParameterMsgParsedCount(m_proc);
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnGetParamRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnGetParamRequest()\n");
    // m_proc->pc->server_info->IncrementPlayMsgParsedCount(m_proc);
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnPlayRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnPlayRequest()\n");
    // m_proc->pc->server_info->IncrementPlayMsgParsedCount(m_proc);
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnPauseRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnPauseRequest()\n");
    // m_proc->pc->server_info->IncrementPlayMsgParsedCount(m_proc);
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnAnnounceRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnAnnounceRequest()\n");
    // m_proc->pc->server_info->IncrementPlayMsgParsedCount(m_proc);
    if(!m_bProtInfoSet)
    {
        m_pClient->update_protocol_statistics_info(m_pProtocol->m_clientType);
        m_bProtInfoSet = TRUE;
    }
    return HXR_OK;
}
STDMETHODIMP
RTSPProtocol::OnRecordRequest(IHXRTSPRequestMessage* pReqMsg)
{
    // fprintf(stderr, "RTSPP::OnRecordRequest()\n");
    // m_proc->pc->server_info->IncrementPlayMsgParsedCount(m_proc);
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::OnTeardownRequest(IHXRTSPRequestMessage* pReqMsg)
{
    if (!pReqMsg)
    {
        // m_proc->pc->server_info->IncrementTeardownMsgParsedCount(m_proc);
        return HXR_OK;
    }

    // fprintf(stderr, "RTSPP::OnTeardownRequest()\n");
    IHXRTSPMessage* pMsg = 0;
    IHXMIMEHeader* pSessionHeader = 0;
    IHXBuffer* pSessionIDBuf = 0;

    pReqMsg->QueryInterface(IID_IHXRTSPMessage, (void **)&pMsg);
    HX_ASSERT(pMsg);
    pMsg->GetHeader("Session", pSessionHeader);
    HX_RELEASE(pMsg);

    if (!pSessionHeader)
    {
        return HXR_OK;
    }

    pSessionHeader->GetValueAsBuffer(pSessionIDBuf);
    HX_RELEASE(pSessionHeader);

    Player::Session* pSession
        = m_pPlayer->FindSession((const char *)pSessionIDBuf->GetBuffer());

    // if we got a teardown, call it "normal" termination, although we
    // have no idea whether the client had an internal error

    if (pSession)
    {
        m_pPlayer->RemoveSession((const char *)pSessionIDBuf->GetBuffer(),
                                 HXR_OK);

        /// We can release our ref on the transport, the player is done with it
        releaseTransports(pSession);
    }

    HX_RELEASE(pSessionIDBuf);

    // m_proc->pc->server_info->IncrementTeardownMsgParsedCount(m_proc);

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandlePauseRequest(const char* pszSessionID, UINT32 ulPausePoint)
{
    return PauseRequestHandler(pszSessionID, ulPausePoint);
}


STDMETHODIMP
RTSPProtocol::HandleStreamAdaptation(THIS_ const char* pSessionID,
                                  REF(StreamAdaptationParams) streamAdaptParams,
                                  BOOL bHlxStreamAdaptScheme)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    return pSession->HandleStreamAdaptation(streamAdaptParams, bHlxStreamAdaptScheme);
}

STDMETHODIMP
RTSPProtocol::Handle3GPPLinkChar(THIS_ const char* pSessionID,
                                  REF(LinkCharParams) linkCharParams)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    return pSession->Handle3GPPLinkChar(linkCharParams);
}

STDMETHODIMP
RTSPProtocol::HandleClientAvgBandwidth(const char* pSessionID,
                                  UINT32 ulClientAvgBandwidth)
{
    Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
    if (!pSession)
    {
        return HXR_ABORT;
    }

    //XXXDPL Retain this because players tend to only send this on DESCRIBE
    // and on non-Real players session is destroyed between DESCRIBE and SETUP.
    m_ulClientAvgBandwidth = ulClientAvgBandwidth;

    return pSession->HandleClientAvgBandwidth(ulClientAvgBandwidth);
}


HX_RESULT
RTSPProtocol::PauseRequestHandler(const char* pszSessionID,
                                  UINT32 ulPausePoint)
{
    if (!m_bTransportSetup)
    {
            return HXR_FAIL;
    }

    Player::Session* pSession = m_pPlayer->FindSession(pszSessionID);
    if (!pSession)
    {
            return HXR_ABORT;
    }

    pSession->pause(ulPausePoint);
    return HXR_OK;
}

HX_RESULT
RTSPProtocol::SetStreamStartTime(const char* pszSessionID,
                                 UINT32 ulStreamNum, UINT32 ulTS)
{
    return m_pProtocol->SetStreamStartTime(pszSessionID, ulStreamNum, ulTS);
}

void
RTSPProtocol::SetScaleDone(HX_RESULT status, Player::Session* pSession,
                                           FIXED32 fScale)
{
    m_pProtocol->SetScaleDone(status, pSession->m_sessionID, fScale);
}

void
RTSPProtocol::SetStatus(UINT32 ulCode)
{
    RTSPServerProtocol::Session* pSession;
    CHXMapStringToOb::Iterator i;
    if(m_pProtocol && m_pProtocol->m_pSessions)
    {
        for (i=m_pProtocol->m_pSessions->Begin();i!=m_pProtocol->m_pSessions->End();++i)
        {
            pSession = (RTSPServerProtocol::Session*)(*i);

            m_pProtocol->SetStatus(pSession->m_ulSessionRegistryNumber,
                      pSession->m_ulSessionStatsObjId,
                      ulCode);
        }
    }
}

HX_RESULT
RTSPProtocol::GetClientProfile(const char* pUrlText,
                               UINT32 ulUrlTextLen,
                               IHXValues* pRequestHeaders,
                               const char* pSessionID,
                               BOOL bUseRTP)
{
    HX_RESULT rc = HXR_OK;
    Process* pProc = client()->proc;

    IHXClientProfile* pProfile = NULL;
    IHXBuffer* pSessionIDBuff = new ServerBuffer(TRUE);
    IHXBuffer* pURLBuff = new ServerBuffer(TRUE);

    if(pSessionIDBuff && pURLBuff)
    {
        Player::Session* pSession = m_pPlayer->FindSession(pSessionID);
        IHXSessionStats* pStats = NULL;
        if(pSession)
        {
            pStats = pSession->m_pStats;
            HX_ASSERT(pStats);
        }
        pSessionIDBuff->Set((UCHAR*)pSessionID, strlen(pSessionID) + 1);
        pURLBuff->Set((UCHAR*)pUrlText, ulUrlTextLen + 1);

        pRequestHeaders->SetPropertyULONG32("RTPAvailable", bUseRTP);
        m_proc->pc->client_profile_manager->GetPSSProfile(this, pProfile,
            pStats, pSessionIDBuff, pURLBuff, pRequestHeaders);

    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }
    //... continues in PSSProfileReady

    HX_RELEASE(pURLBuff);
    HX_RELEASE(pSessionIDBuff);

    return rc;
}

STDMETHODIMP
RTSPProtocol::PSSProfileReady (HX_RESULT ulStatus, IHXClientProfile* pInfo,
                               IHXBuffer* pRequestId, IHXBuffer* pRequestURI,
                               IHXValues* pRequestHeaders)
{
    HX_RESULT rc = HXR_OK;

    if (!pRequestId || !pRequestURI || !pRequestHeaders || !m_pPlayer)
    {
        return HXR_ABORT;
    }

    const char* pszSessionId = (const char*)pRequestId->GetBuffer();
    Player::Session* pSession = m_pPlayer->FindSession(pszSessionId);

    if (!pSession || !pSession->m_pStats)
    {
        return HXR_ABORT;
    }
    pSession->AddRef();

    UINT16 uProfileWarning;

    if (SUCCEEDED(ulStatus))
    {
        // 201: Content selection applied
        uProfileWarning = 201;
        pSession->m_pStats->SetClientProfileInfo(pInfo);
    }
    else
    {
        if (ulStatus == HXR_NOTIMPL)
        {
            // 500: Not supported
            uProfileWarning = 500;
        }
        else
        {
            // 200: Not applied
            uProfileWarning = 200;
            if (ulStatus == HXR_PARSE_ERROR)
            {
                ERRMSG(m_proc->pc->error_handler,
                "Error parsing profile for client (%d)\n", m_pClient->conn_id);
            }
            else
            {
                ERRMSG(m_proc->pc->error_handler,
                "Error retrieving profile for client (%d)\n",
                 m_pClient->conn_id);
            }
        }
    }

    IHXBuffer* pXwapPrfHdr = NULL;
    if(SUCCEEDED(pRequestHeaders->GetPropertyCString("x-wap-profile",
                                                      pXwapPrfHdr)))
    {
        pSession->m_pStats->SetXWapProfileStatus(uProfileWarning);
        pRequestHeaders->SetPropertyULONG32("x-wap-profile-warning",
                                            uProfileWarning);
        pXwapPrfHdr->Release();
    }

    BOOL bUseRTP = FALSE;
    UINT32 ulRTPFlag = 0;
    if (SUCCEEDED
        (pRequestHeaders->GetPropertyULONG32("RTPAvailable", ulRTPFlag))
        && ulRTPFlag)
    {
        bUseRTP = TRUE;
    }
    URL* pURL = new URL((const char*)pRequestURI->GetBuffer(),
                        pRequestURI->GetSize() - 1);
    rc = pSession->got_url(pURL, bUseRTP);

    pSession->Release();
    return rc;
}


STDMETHODIMP
RTSPProtocol::SetValues(IHXValues* pValues)
{
    if (m_pClient->m_bIsAProxy)
    {
        IHXBuffer* pProxyPeerAddr = NULL;
        if (SUCCEEDED(pValues->GetPropertyCString("X-Real-RealProxy-SendTo-IP", pProxyPeerAddr)))
        {
            HX_RELEASE(m_pProxyPeerAddr);
            m_pProxyPeerAddr = pProxyPeerAddr;

            // Since we're handing off, this step isn't necessary.
            //m_pProxyPeerAddr->AddRef();
            //HX_RELEASE(pProxyPeerAddr);
        }
        else
        {
            HX_ASSERT(!"RTSPProtocol couldn't get proxy peer addr!");
        }
    }

    return HXR_OK;
}


HX_RESULT
RTSPProtocol::SetupClientStatsObject()
{
    ClientStats* pClientStats = NULL;

    if (!m_pClient->m_bIsAProxy)
    {
    // If this is not a proxy, this object is created by
    // the client object in the constructor.
        HX_ASSERT(m_pClient->get_client_stats() != NULL);
        return HXR_OK;
    }

    if (m_pClient->get_client_stats() != NULL)
    {
    // This function has already been called.
        return HXR_OK;
    }

    if (!m_pProtocol->IsClientStatsObjIdSet())
    {
    // We weren't passed a client id by rproxy!
    //XXXDPL Should we create one here?
        HX_ASSERT(0);
        return HXR_FAIL;
    }

    pClientStats = (ClientStats*)client()->proc->pc->client_stats_manager->GetClient(m_pProtocol->GetClientStatsObjId());

    m_pClient->m_pStats = pClientStats;
    m_pClient->m_ulClientStatsObjId = pClientStats->GetID();
// AddRef() is handled by ClientStatsManager::GetClient() so no need to do AddRef()/Release() here.

    return HXR_OK;
}

HX_RESULT
RTSPProtocol::ParseQueryParams(const char* pszUrl, IHXValues* pRequestHeaders)
{
    HX_RESULT rc = HXR_OK;

    char* pszQueryParams = NULL;
    char* pszXWapProfile = NULL;
    char* pszXWapPrfDiff = NULL;

    // Get the profile and diff headers from the query params
    pszQueryParams = strchr((char*)pszUrl, '?');

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
    const char* szProfileParamName = "x-wap-profile";
    const char* szPrfDiffParamName = "x-wap-profile-diff";
    size_t nProfileNameLen = strlen(szProfileParamName) - 1;
    size_t nPrfDiffNameLen = strlen(szPrfDiffParamName) - 1;
    CHXString sGUIDParameter;

    while(paramField.hasValue())
    {
        sGUIDParameter = paramField.value();
        szParam = sGUIDParameter.GetBuffer(sGUIDParameter.GetLength());

        if (!pszXWapProfile &&
            strncmp(szParam, szProfileParamName, nProfileNameLen) == 0)
        {
            pszXWapProfile = (char*)strchr(szParam, '=');
            if (pszXWapProfile)
            {
                ++pszXWapProfile;
                if(pszXWapPrfDiff)
                {
                    break;
                }
            }
        }

        else if (!pszXWapPrfDiff &&
            strncmp(szParam, szPrfDiffParamName, nPrfDiffNameLen) == 0)
        {
            pszXWapPrfDiff = (char*)strchr(szParam, '=');
            if (pszXWapPrfDiff)
            {
                ++pszXWapPrfDiff;
                if(pszXWapProfile)
                {
                    break;
                }
            }
        }

        paramField = inputScanner.nextToken(szTokenSeparator);
    }

    // If x-wap-profile or x-wap-profile-diff was found in the query options,
    // add it to the request header
    IHXBuffer* pHeaderBuf = NULL;
    IHXCommonClassFactory* pClassFactory =
        client()->proc->pc->common_class_factory;
    if (pszXWapProfile)
    {
        rc = pClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pHeaderBuf);
        if(SUCCEEDED(rc))
        {
            rc = pHeaderBuf->Set((UCHAR*)pszXWapProfile,
                    strlen(pszXWapProfile) + 1);
        }
        if(SUCCEEDED(rc))
        {
            pRequestHeaders->SetPropertyCString(szProfileParamName, pHeaderBuf);
        }
        HX_RELEASE(pHeaderBuf);
    }

    if (SUCCEEDED(rc) && pszXWapPrfDiff)
    {
        rc = pClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pHeaderBuf);
        if(SUCCEEDED(rc))
        {
            rc = pHeaderBuf->Set((UCHAR*)pszXWapPrfDiff,
                    strlen(pszXWapPrfDiff) + 1);
        }
        if(SUCCEEDED(rc))
        {
            pRequestHeaders->SetPropertyCString(szPrfDiffParamName, pHeaderBuf);
        }
        HX_RELEASE(pHeaderBuf);
    }

    return rc;
}

