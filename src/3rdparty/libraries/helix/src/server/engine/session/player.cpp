/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: player.cpp,v 1.121 2007/05/01 18:17:21 darrick Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "hlxclib/time.h"
#include "hlxclib/signal.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxcodec.h"
#include "hxmime.h"
#include "ihxpckts.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "hxqosinfo.h"

#include "hxslist.h"
#include "hxmap.h"
#include "hxtick.h"

#include "debug.h"

#include "tcpio.h"
#include "udpio.h"
#include "bufio.h"
#include "fsio.h"

#include "hxstrutl.h"
#include "timerep.h"

#include "rttp2.h"

#include "bwe.h"
#include "rafile.h"

#include "srcerrs.h"
#include "url.h"
#include "config.h"
#include "proc.h"


#include "hxdtcvt.h"
#include "hxprot.h"
#include "rtspprot.h"
#include "rtspif.h"
#include "client.h"
#include "streamer_container.h"
#include "server_engine.h"
#include "server_info.h"
#include "servreg.h"
#include "server_request.h"
#include "server_context.h"
#include "dtcvtcon.h"
#include "base_errmsg.h"

#include "hxasm.h"
#include "ihxpckts.h"
#include "chxpckts.h"

#include "source.h"
#include "hxpcktflwctrl.h"
#include "pcktflowwrap.h"

#include "player.h"
#include "fsmanager.h"
#include "transport.h"
#include "simple_callback.h"
#include "allowance_mgr.h"
#include "ml_allowance_mgr.h"
#include "allowance_wrap.h"

#include "plgnhand.h"
#include "hxplugn.h"
#include "hxformt.h"

#include "ff_source.h"
#include "inputsrc_push.h"

#include "flob_wrap.h"

#include "hxauth.h"

#include "errhand.h"

#include "misc_plugin.h"
#include "asmrulep.h"
#include "multicast_mgr.h"
#include "bcastmgr.h"
#include "hxxfile.h"
#include "rtptypes.h"
#include "mutex.h"

#include "rtsputil.h"
#include "netbyte.h"

#include "servlist.h"

#include "hxstats.h"
#include "server_stats.h"

#include "qos_signal.h"
#include "qos_prof_mgr.h"
#include "qos_sig_bus_ctl.h"
#include "qos_cfg_names.h"
#include "rateselinfo.h"

#include "hxclientprofile.h"
#include "srcfinder.h"
#include "isifs.h"

#ifdef WIN32
#define snprintf _snprintf
#endif // WIN32

#define INVALID_STREAM_NUMBER 0xFFFF

#ifdef PAULM_SESSIONTIMING
#include "classtimer.h"

void
_ExpiredSessionCallback(void* p)
{
    Player::Session* pS = (Player::Session*)p;

    printf("\tm_pPlayer: 0x%x\n", pS->m_pPlayer);
    printf("\tclient: 0x%x\n", pS->m_pClient);

};

ClassTimer g_SessionTimer("Player::Session",
        _ExpiredSessionCallback, 3600);
#endif

#ifdef PAULM_PLAYERTIMING
#include "classtimer.h"

void
_ExpiredPlayerCallback(void* p)
{
    Player* pPlayer = (Player*)p;

    printf("\tm_pClient: 0x%x\n", pPlayer->m_pClient);
}
ClassTimer g_PlayerTimer("Player",
        _ExpiredPlayerCallback, 3600);
#endif


#define CONVERT_HXR_ERROR(x) \
    (((x) == HXR_FAIL || (x) == HXR_FAILED) ? HXR_FILE_NOT_FOUND : (x))

// static initializations

UINT32 Player::m_ulNextSessionID = 0;


Player::Session::Session(Process* _proc, Client* c, Player* p,
                    const char* pSessionID, BOOL bRetainEntityForSetup) :
      m_url(0),
      m_sessionID(pSessionID),
      m_pSessionId(NULL),
      m_ulSessionRegistryNumber(0),
      m_ulSessionStatsObjId(0),
      m_pStats(NULL),
      m_pStats2(NULL),
      m_ulRegistryID(0),
      m_pRegistryKey(NULL),
      m_bIsMidBox(FALSE),
      m_pSessionHeader(NULL),
      m_pProc(_proc),
      m_pSessionControl(NULL),
      m_pAllowanceMgrs(NULL),
      m_bPaused(FALSE),
      m_bAllowanceDebug(FALSE),
      m_pAllowanceSerializer(NULL),
      m_bDone(FALSE),
      m_bCleanedUp(FALSE),
      m_ulFunctionsInUse(0),
      m_pSignal(NULL),
      m_pSignalBus(NULL),
      m_pQoSConfig(NULL),
      m_pSignalBusCtl(NULL),
      m_pMediaMimeType(NULL),
      m_bUseMediaDeliveryPipeline(FALSE),
      m_pSrcFinder(NULL),
      m_pBroadcastMapper(NULL),
      m_pMimeMapper(NULL),
      m_pFileFormat(NULL),
      m_pBlockTransferFileFormat(NULL),
      m_pRequest(NULL),
      m_pFileRequest(NULL),
      m_pFileFormatRequest(NULL),
      m_ulLastModifiedTime(0),
      m_pSourceControl(0),
      m_pUberStreamMgr(NULL),
      m_pUberStreamConfig(NULL),
      m_ProtocolSetupAfterHeadersDone(0),
      m_pStreamAdaptationSetup(NULL),
      m_StreamAdaptationScheme(ADAPTATION_NONE),
      m_unStreamAdaptSetupCount(0),
      m_bHlxStreamAdaptScheme(FALSE),
      m_pLinkCharSetup(NULL),
      m_pSessionAggrLinkCharParams(0),
      m_unLinkCharSetupCount(0),

      m_bBegun(FALSE),
      m_ulRefCount(0),
      m_ppStreams(NULL),
      m_num_streams(0),
      m_uStreamGroupCount(0),
      m_num_streamheadersdone(0),
      m_num_streamsdone(0),
      m_is_ready(FALSE),
      m_bUseRTP(FALSE),
      m_pPlayer(p),
      m_pClient(c),
      m_bIsRealDataType(FALSE),
      m_bBlockTransfer(FALSE),
      m_tIfModifiedSince(INVALID_TIME_T),
      m_bNoLatency(FALSE),
      m_bInitialized(FALSE),
      m_bRedirected(FALSE),
      m_uDoneCallbackHandle(0),
      m_pMasterRuleBookBuffer(NULL),
      m_pMasterRuleBook(NULL),
      m_pDataConvert(NULL),
      m_pMulticastTransportDataConvert(NULL),
      m_pURLBuf(NULL),
      m_pActualFileHeader(NULL),
      m_pConvertedFileHeader(NULL),
      m_pHeaderControlBuffer(NULL),
      m_pFFAdvise(NULL),
      m_bRetainEntityForSetup(bRetainEntityForSetup),
      m_ulInactivityTimeout(0),
      m_uInactivityCallbackHandle(0),
      m_pCommonClassFactory(NULL),
      m_ulClientAvgBandwidth(0),
      m_uStreamSetupCount(0),
      m_pAggRateAdaptParams(0),
      m_ulMaxPreRoll(0),
      m_pRateSelInfo(NULL),
      m_pBandwidthSignal(NULL)
{
#ifdef PAULM_SESSIONTIMING
    g_SessionTimer.Add(this);
#endif
#ifdef PAULM_CLIENTAR
    ADDR_NOTIFY(m_pClient, 5);
#endif
    m_pClient->AddRef();
    m_pPlayer->AddRef();

    m_pRequest = new ServerRequest;
    m_pRequest->AddRef();

    m_pSessionId = new ServerBuffer();
    m_pSessionId->AddRef();
    m_pSessionId->Set((const UCHAR*)pSessionID, strlen(pSessionID)+1);

    m_pProc->pc->server_context->QueryInterface(IID_IHXCommonClassFactory, (void **)&m_pCommonClassFactory);

#ifndef XXXAAK_PERF_NO_ALLOWANCE
    // if this is a proxy, don't invoke server allowance plugins
    if (!m_pClient->m_bIsAProxy)
    {
        // Create a list of allowance plugins
        m_pAllowanceMgrs = new CHXSimpleList();
        InitAllowancePlugins();
    }
#endif /* XXXAAK_PERF_NO_ALLOWANCE */

    DPRINTF(D_INFO, ("Session::Session: %s\n", (const char*)m_sessionID));
#ifdef DEBUG
    m_pProc->pc->error_handler->Report(HXLOG_INFO, 0, 0, "Player Connected\n",
        "http://www.real.com");
#endif

    m_pSignalBusCtl = m_pProc->pc->qos_bus_ctl;
    m_pSignalBusCtl->AddRef();

    m_pSignal = (IHXQoSSignal*)(new QoSSignal());
    m_pSignal->AddRef();

    m_pRateSelInfo = new RateSelectionInfo();

    /* setup the signal bus for this session */
    HX_VERIFY(HXR_OK == m_pSignalBusCtl->CreateSignalBus(m_pProc, m_pSessionId));

    if (m_pProc->pc->process_type == PTStreamer)
    {
        StreamerContainer* pStreamer = ((StreamerContainer*)(m_pProc->pc));

        HXMutexLock(pStreamer->m_BusMapLock, TRUE);
        pStreamer->m_BusMap.Lookup((const char*)m_sessionID,
                                   (void*&)m_pSignalBus);
        HXMutexUnlock(pStreamer->m_BusMapLock);

        if (m_pSignalBus)
        {
            m_pSignalBus->AddRef();
        }
    }

    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                          (void**)&m_pBandwidthSignal);

    m_pBandwidthSignal->SetSize(sizeof(RateSignal));
}

Player::Session::~Session()
{
#ifdef PAULM_SESSIONTIMING
    g_SessionTimer.Remove(this);
#endif

    DPRINTF(D_INFO, ("Session::~Session: %s\n", (const char*)m_sessionID));

    HX_ASSERT(!m_ulFunctionsInUse);

    HX_DELETE(m_url);

    HX_RELEASE(m_pUberStreamMgr);
    HX_RELEASE(m_pUberStreamConfig);

    if (m_pSignalBusCtl)
    {
        m_pSignalBusCtl->DestroySignalBus(m_pProc, m_pSessionId);
        m_pSignalBusCtl->Release();
        m_pSignalBusCtl = NULL;
    }

    if (m_pSourceControl)
    {
        m_pSourceControl->Done();
        m_pSourceControl->Release();
        m_pSourceControl = 0;
    }

    HX_RELEASE(m_pRateSelInfo);

    HX_RELEASE(m_pBroadcastMapper);
    HX_RELEASE(m_pMimeMapper);
    HX_RELEASE(m_pFileFormat);
    HX_RELEASE(m_pBlockTransferFileFormat);
    HX_RELEASE(m_pFFAdvise);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pFileRequest);
    HX_RELEASE(m_pFileFormatRequest);
    HX_RELEASE(m_pStats);
    HX_RELEASE(m_pStats2);
    HX_RELEASE(m_pStreamAdaptationSetup);
    HX_RELEASE(m_pLinkCharSetup);

    HX_DELETE(m_pSessionAggrLinkCharParams);

    if (m_pSrcFinder)
    {
        m_pSrcFinder->Close();
        HX_RELEASE(m_pSrcFinder);
    }

    // Eventually passed to server protocol, to release its reference
    // to the PPM session.  See notes in RTSPServerProtocol::SessionDone().
    if (m_pClient) m_pClient->SessionDone(m_sessionID);

    if(m_pSessionControl)
    {
        m_pSessionControl->ControlDone();
        m_pSessionControl->Release();
        m_pSessionControl = NULL;
    }

    if (m_uDoneCallbackHandle)
    {
        m_pPlayer->m_pProc->pc->engine->schedule.remove(m_uDoneCallbackHandle);
        m_uDoneCallbackHandle = 0;
    }

    if (m_uInactivityCallbackHandle)
    {
        m_pPlayer->m_pProc->pc->engine->schedule.remove(
                m_uInactivityCallbackHandle);
        m_uInactivityCallbackHandle = 0;
    }

    if (m_pPlayer)
    {
        m_pPlayer->Release();
        m_pPlayer = 0;
    }

    HX_RELEASE(m_pPlayer);
    if (m_pClient)
    {
#ifdef PAULM_CLIENTAR
        REL_NOTIFY(m_pClient, 2);
#endif
        m_pClient->Release();
        m_pClient = 0;
    }

    HX_RELEASE(m_pMasterRuleBookBuffer);
    HX_DELETE(m_pMasterRuleBook);

    delete[] m_pRegistryKey;

    HX_RELEASE(m_pAllowanceSerializer);
    HX_RELEASE(m_pDataConvert);
    HX_RELEASE(m_pMulticastTransportDataConvert);
    HX_RELEASE(m_pURLBuf);
    HX_RELEASE(m_pActualFileHeader);
    HX_RELEASE(m_pConvertedFileHeader);
    HX_RELEASE(m_pHeaderControlBuffer);

    HX_RELEASE(m_pSessionId);
    HX_RELEASE(m_pMediaMimeType);
    HX_RELEASE(m_pSignal);
    HX_RELEASE(m_pSignalBus);
    HX_RELEASE(m_pBandwidthSignal);
    HX_RELEASE(m_pQoSConfig);
    HX_RELEASE(m_pSessionHeader);

    HX_DELETE(m_pAggRateAdaptParams);
}

void
Player::Session::Done(HX_RESULT status)
{
    if (m_bDone)
        return;

    ClearInactive();
    update_statistics();

    // look at IfDoneCleanup() for explanation.
    m_bDone = TRUE;

    HX_ASSERT(m_pStats);

    // write final status

    if (m_pStats2)
    {
#if 0  // Until I figure out the best way to trigger dumping these stats this
       // will remain ifdefed out on the head.
        if (g_ulServerDebugLevel & S_STARTTIME)
        {
            m_pStats2->DumpStartupInfo();
        }
#endif
    }

    if (m_pStats && m_pStats->GetEndStatus() == SSES_NOT_ENDED)
    {
        SessionStatsEndStatus ulEndStatus;
        switch (status)
        {
        case HXR_OK:
            ulEndStatus = SSES_OK; break;

            // client errors
        case HXR_NET_WRITE:
            ulEndStatus = SSES_SOCKET_CLOSED; break;

        case HXR_TIMEOUT:
            ulEndStatus = SSES_CLIENT_TIMEOUT; break;

        case HXR_INVALID_FILE:
            ulEndStatus = SSES_INVALID_FILE; break;

            // server errors
        case HXR_UNEXPECTED:
            ulEndStatus = SSES_SERVER_INTERNAL_ERROR; break;

        case HXR_SERVER_ALERT:
            ulEndStatus = SSES_SERVER_ALERT; break;

        case HXR_BLOCKED:
            ulEndStatus = SSES_SOCKET_BLOCKED; break;

            // unknown error
        case HXR_FAIL:
        default:
            ulEndStatus = SSES_UNKNOWN_ERROR; break;

        }

        m_pStats->SetEndStatus(ulEndStatus);
    }

    m_pProc->pc->client_stats_manager->SessionDone(m_pClient->get_client_stats(), m_pStats, m_pProc);

    /*
     * If wer are multicasting then the transport will done the
     * transport dataconvert.  It is our job to done the
     * control channel.
     */
    if (m_pDataConvert && m_pMulticastTransportDataConvert)
    {
        m_pDataConvert->Done();
    }

    IfDoneCleanup();

    //XXXkshoop DON'T DO THIS!!!!!! IT IS NOT SUPPOSED TO BE HERE!!!!!!
    //
}

void
Player::Session::IfDoneCleanup()
{
    // see OnURLDone() for usage info.
    // this function and Done() work together
    // to provide safe deletions.

    if (!m_ulFunctionsInUse && m_bDone && !m_bCleanedUp)
    {
        // Don't ever let this be called again while it is running
        m_bCleanedUp = TRUE;

        DoneCallback* pCallback = new DoneCallback(this);
        pCallback->AddRef();
        m_uDoneCallbackHandle = m_pPlayer->m_pProc->pc->engine->schedule.enter(
            Timeval(0.0), pCallback);

        pCallback->Release();
    }
}

void
Player::Session::ClearInactive()
{
    if (m_uInactivityCallbackHandle)
    {
        m_pPlayer->m_pProc->pc->engine->schedule.remove(
                m_uInactivityCallbackHandle);
        m_uInactivityCallbackHandle=0;
    }

}
void
Player::Session::SetInactive()
{
    if (m_ulInactivityTimeout && !m_uInactivityCallbackHandle)
    {
        Timeval tCallback = m_pPlayer->m_pProc->pc->engine->now;
        tCallback.tv_sec += m_ulInactivityTimeout;

        InactivityCallback* pCallback = new InactivityCallback(this);
        pCallback->AddRef();
        m_uInactivityCallbackHandle = m_pPlayer->m_pProc->pc->engine->
            schedule.enter(tCallback, pCallback);

        pCallback->Release();
    }
}

void
Player::Session::clear_allowance_list()
{
    // Entered Function
    ++m_ulFunctionsInUse;

    // Cleanup the list of Allowance Managers
    if (m_pAllowanceMgrs)
    {
        AllowanceMgr* pAllowanceMgr = NULL;

        LISTPOSITION pos = m_pAllowanceMgrs->GetHeadPosition();
        while(pos)
        {
            // Get next Manager
            pAllowanceMgr = (AllowanceMgr*)m_pAllowanceMgrs->GetAt(pos);

            if (pAllowanceMgr)
            {
                if (pAllowanceMgr->IsActive())
                {
                    ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pAllowanceMgr
                        , ("Calling OnDone()\n"));
                    pAllowanceMgr->AllowanceMgrDone();
                }
                HX_RELEASE(pAllowanceMgr);
            }

            m_pAllowanceMgrs->GetNext(pos);
        }

        m_pAllowanceMgrs->RemoveAll();
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::clear_stream_list()
{
    // Entered
    ++m_ulFunctionsInUse;

    if (m_ppStreams != NULL)
    {
        for (int i = 0; i < m_num_streams; i++)
        {
            HX_DELETE(m_ppStreams[i]);
        }
        m_num_streams = 0;
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::update_statistics()
{
    // Entered
    ++m_ulFunctionsInUse;

    HXProtocol* pProtocol = m_pClient->protocol();

    for (int i = 0; i < m_num_streams; i++)
    {
        if (!m_ppStreams[i]->m_bSetupReceived)
        {
	    continue;
        }

        Transport* pTransport = NULL;
        if (m_ppStreams[i] != NULL)
        {
            pTransport = pProtocol->getTransport(this,
                                           m_ppStreams[i]->m_stream_number, 0);
        }
        if (pTransport != NULL && !pTransport->IsUpdated())
        {
            pTransport->UpdateStatistics();
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

/* PlayerSink methods */

HX_RESULT
Player::Session::InitDone(HX_RESULT status)
{
    if (m_bDone)
    {
        return HXR_OK;
    }

    if ((status != HXR_OK) && (status != HXR_NOT_AUTHORIZED)&& (status != HXR_NO_MORE_FILES))
    {
        if (m_pSrcFinder)
        {
            m_pSrcFinder->FindNextSource();
        }
        else
        {
            HX_ASSERT(!"shouldn't happend");
            FindDone(HXR_UNEXPECTED, NULL);
        }
        return HXR_OK;
    }

    if (status != HXR_OK)
    {
        ++m_ulFunctionsInUse;
        SessionInitDone(status);
        --m_ulFunctionsInUse;
        IfDoneCleanup();
        return HXR_OK;
    }
    // Entered
    ++m_ulFunctionsInUse;
#ifndef PERF_NOCLIENTREG
    /*
     * Need to pass down the url.
     * XXX PM Isn't there a better (faster) way to do this, like
     * pass the uRL down from above? m_pRequest only has the url
     * minus the filesys mount point, which is not usefull to us.
     */

    HX_RELEASE(m_pURLBuf);

    if (m_pStats)
    {
        m_pURLBuf = m_pStats->GetURL();
    }

    HX_RELEASE(m_pDataConvert);
    IHXDataConvert* pDataConvert = m_pProc->pc->data_convert_con->GetConverter(
                                    m_pProc, m_pURLBuf);
    if (pDataConvert)
    {
        m_pDataConvert = new DataConvertShim(pDataConvert, m_pProc);
        m_pDataConvert->AddRef();
        m_pDataConvert->SetControlResponse(this);
        HX_RELEASE(pDataConvert);
    }
#endif /* ndef PERF_NOCLIENTREG */

    /*
     * This InitDone came back from the file format.  If there is
     * a IHXDataConvert, init him now.
     */
    if (m_pDataConvert)
    {
        m_pDataConvert->DataConvertInit(m_pProc->pc->server_context);
    }
    else
    {
        SessionInitDone(status);
    }
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return HXR_OK;
}

HX_RESULT
Player::Session::DataConvertInitDone(HX_RESULT status)
{
    ++m_ulFunctionsInUse;

    ServerRequestWrapper* wrapper =
        new ServerRequestWrapper(FS_HEADERS, m_pRequest);
    wrapper->AddRef();

    m_pDataConvert->SetRequest((IHXRequest*)wrapper);

    HX_RELEASE(wrapper);

    SessionInitDone(status);
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return HXR_OK;
}

HX_RESULT
Player::Session::SessionInitDone(HX_RESULT status)
{
    ++m_ulFunctionsInUse;
    if (m_bDone)
    {
        goto init_done_exit;
    }
    else if (status == HXR_NOT_AUTHORIZED)
    {
        m_pClient->protocol()->setupStreams(0, this, HXR_NOT_AUTHORIZED);
    }
    else if (FAILED(status))
    {
        if (status == HXR_PARSE_ERROR)
        {
            SendAlert(SE_INTERNAL_ERROR);
        }
        m_pClient->protocol()->setupStreams(0, this, CONVERT_HXR_ERROR(status));
    }
    else
    {
        /* done with finder */
        if (m_pSrcFinder)
        {
            m_pSrcFinder->Close();
            m_pSrcFinder->Release();
            m_pSrcFinder = NULL;
        }

        // XXXkshoop Call OnURLPostFileOpen() here instead.
        if (m_pActualFileHeader != NULL)
        {
            SessionFileHeaderReady(HXR_OK, m_pActualFileHeader);
        }
        else
        {
            m_pSourceControl->GetFileHeader(this);
        }
    }

init_done_exit:
    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

HX_RESULT
Player::Session::FileHeaderReady(HX_RESULT status, IHXValues* header)
{
    if (m_bDone)
    {
        return HXR_OK;
    }

    ++m_ulFunctionsInUse;

    if (status == HXR_NOT_LICENSED && !m_pClient->m_bIsAProxy)
    {
        SendAlert(SE_DATATYPE_UNLICENSED);
    }
    if (status != HXR_OK)
    {
        m_pClient->protocol()->setupStreams(0, this, CONVERT_HXR_ERROR(status));
    }
    else
    {
        /*
         * Have to do this before it goes to the converter so that
         * it winds up in the reverted header.
         * Set LiveStream property ONLY if it is a live stream.
         * This is to allow a fileformat to FAKE a live stream,
         * if it wants to
         */
        if (m_pSourceControl->IsLive())
        {
            header->SetPropertyULONG32("LiveStream", TRUE);
        }
            m_pActualFileHeader = header;
            m_pActualFileHeader->AddRef();
        if (m_pDataConvert)
        {
            m_pDataConvert->ConvertFileHeader(header);
        }
        else
        {
            SessionFileHeaderReady(status, header);
        }
    }
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return HXR_OK;
}

HX_RESULT
Player::Session::ConvertedFileHeaderReady(HX_RESULT status,
                                            IHXValues* pFileHeader)
{
    ++m_ulFunctionsInUse;
    if (pFileHeader)
    {
        m_pConvertedFileHeader = pFileHeader;
        m_pConvertedFileHeader->AddRef();
    }
    SessionFileHeaderReady(status, m_pActualFileHeader);
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return HXR_OK;
}

HX_RESULT
Player::Session::SessionFileHeaderReady(HX_RESULT status, IHXValues* header)
{
    // Entered
    ++m_ulFunctionsInUse;

    IHXBuffer* pTitle = NULL;
    IHXBuffer* pAuthor = NULL;
    IHXBuffer* pCopyright = NULL;

    IHXBuffer* pRuleBook = NULL;

    unsigned char*  pszTitle = NULL;
    unsigned char*  pszAuthor = NULL;
    unsigned char*  pszCopyright = NULL;

    if (status != HXR_OK)
    {
        m_pClient->protocol()->setupStreams(0, this, CONVERT_HXR_ERROR(status));
    }
    else
    {
        // Deal with the ServerType hack
        // This is used by the media analyzer
        const char ACCEPT[] = {"AcceptServerMetaData"};
        const char LICENSE_LOC[] = {"license"};
        const char LICENSE_CAP_LOC[] = {"License"};
        const char NUM_LICENSE_LOC[] = {"license.NumLicenses"};
        const char SERVERTYPE_LOC[] = {"client.ServerType"};
        const char* SERVERTYPE = SERVERTYPE_LOC + 7;
        IHXValues* pValuesRequest = NULL;
        IHXBuffer* pBufferAcceptValue = NULL;
        m_pRequest->GetRequestHeaders(FF_HEADERS, pValuesRequest);

        if (pValuesRequest && HXR_OK == pValuesRequest->GetPropertyCString(ACCEPT, pBufferAcceptValue) && pBufferAcceptValue)
        {
            //Search for ServerType in comma-delim list
            const char* pCharEntryStart = (const char*)pBufferAcceptValue->GetBuffer();
            const char* pCharEntryEnd = pCharEntryStart;
            BOOL bFound = FALSE;
            while (pCharEntryEnd && *pCharEntryEnd)
            {
                ++pCharEntryEnd;
                if (*pCharEntryEnd == ',' || !*pCharEntryEnd)
                {
                    if (!strncasecmp(SERVERTYPE, pCharEntryStart, (pCharEntryEnd - pCharEntryStart)))
                    {
                        bFound = TRUE;
                        break;
                    }
                    pCharEntryStart = pCharEntryEnd + 1;
                }
            }

            HX_RELEASE(pBufferAcceptValue);

            if (bFound)
            {
                IHXBuffer* pBufferServerType = NULL;
                ServerRegistry* pReg = m_pProc->pc->registry;

                if (HXR_OK != pReg->GetStr(SERVERTYPE_LOC, pBufferServerType, m_pProc))
                {
                    // Create and cache the ServerType string
                    // by concatting the servertypes from each
                    // license and storing it in the registry
                    INT32 lNumLicenses = 0;

                    if (HXR_OK == pReg->GetInt(NUM_LICENSE_LOC, &lNumLicenses, m_pProc) && lNumLicenses > 0)
                    {
                        INT32 lIndex = 0;
                        char pCharKey[512];
                        char pCharType[4096];
                        char* sEndCharType = pCharType;

                        *pCharType = '\0';
                        for (lIndex = 0; lIndex < lNumLicenses; ++lIndex)
                        {
                            sprintf(pCharKey, "%s.%s%ld.%s.Definition.%s", LICENSE_LOC, LICENSE_CAP_LOC, lIndex, LICENSE_CAP_LOC, SERVERTYPE);
                            if (HXR_OK == pReg->GetStr(pCharKey, pBufferServerType, m_pProc))
                            {
                                sEndCharType += sprintf(sEndCharType,"%s;",
                                    (const char*)pBufferServerType->GetBuffer());
                            }
                            HX_RELEASE(pBufferServerType);
                        }
                        HX_ASSERT(strlen(pCharType) < 4000);
                        CHXBuffer::FromCharArray(pCharType, &pBufferServerType);
                    }
                    else
                    {
                        CHXBuffer::FromCharArray("", &pBufferServerType);
                    }

                    // Cache it in the registry
                    pReg->AddStr(SERVERTYPE_LOC, pBufferServerType, m_pProc);
                }

                // add it to the file headers
                header->SetPropertyCString(SERVERTYPE, pBufferServerType);
                if (m_pConvertedFileHeader)
                {
                    m_pConvertedFileHeader->SetPropertyCString(SERVERTYPE,
                            pBufferServerType);
                }

                HX_RELEASE(pBufferServerType);
            }
        }

        HX_RELEASE(pValuesRequest);

        if (m_pConvertedFileHeader)
        {
            const char* pCharMimeType;
            m_pDataConvert->GetConversionMimeType(pCharMimeType);
            IHXValues* pConvertedHeader = NULL;
            if (pCharMimeType)
            {
                pConvertedHeader = FlattenConvertedHeader(
                        m_pConvertedFileHeader, "DataConvertFileHeader");
            }
            else
            {
                pConvertedHeader = m_pConvertedFileHeader;
                pConvertedHeader->AddRef();
            }
            m_pClient->protocol()->setupHeader(pConvertedHeader, this, HXR_OK);
            pConvertedHeader->Release();
        }
        else
        {
            m_pClient->protocol()->setupHeader(header, this, HXR_OK);
        }

        // retrieve/set TAC properties
        header->GetPropertyBuffer("Title", pTitle);
        header->GetPropertyBuffer("Author", pAuthor);
        header->GetPropertyBuffer("Copyright", pCopyright);

        (pTitle) ? (pszTitle = pTitle->GetBuffer()) : (pszTitle = NULL);
        (pAuthor) ? (pszAuthor = pAuthor->GetBuffer()) : (pszAuthor = NULL);
        (pCopyright) ? (pszCopyright = pCopyright->GetBuffer()) : (pszCopyright = NULL);

        header->GetPropertyCString("ASMRulebook",pRuleBook);
        if (pRuleBook)
        {
            m_pMasterRuleBookBuffer = pRuleBook;
            m_pMasterRuleBookBuffer->AddRef();
            pRuleBook->Release();
            pRuleBook = NULL;
        }

        if (pTitle)     pTitle->Release();
        if (pAuthor)    pAuthor->Release();
        if (pCopyright) pCopyright->Release();

        if (m_pRegistryKey)
        {
            char str[512];
            const char* pcName = 0;

            IHXBuffer* pBuf = 0;
            header->GetFirstPropertyCString(pcName, pBuf);
            while (pcName && pBuf)
            {
                sprintf(str, "%s.FileHeader.%s", m_pRegistryKey, pcName);
                // printf("%s = %s\n", str, (const char *)pBuf->GetBuffer());
                m_pProc->pc->registry->AddStr(str, pBuf, m_pProc);
                HX_RELEASE(pBuf);
                pcName = 0;
                header->GetNextPropertyCString(pcName, pBuf);
            }

            header->GetFirstPropertyBuffer(pcName, pBuf);
            while (pcName && pBuf)
            {
                sprintf(str, "%s.FileHeader.%s", m_pRegistryKey, pcName);
                // printf("%s = %s\n", str, (const char *)pBuf->GetBuffer());
                m_pProc->pc->registry->AddStr(str, pBuf, m_pProc);
                HX_RELEASE(pBuf);
                pcName = 0;
                header->GetNextPropertyBuffer(pcName, pBuf);
            }

            ULONG32 ul = 0;
            header->GetFirstPropertyULONG32(pcName, ul);
            while (pcName && ul)
            {
                sprintf(str, "%s.FileHeader.%s", m_pRegistryKey, pcName);
                // printf("%s = %lu\n", str, ul);
                m_pProc->pc->registry->AddInt(str, (INT32)ul, m_pProc);
                ul = 0;
                pcName = 0;
                header->GetNextPropertyULONG32(pcName, ul);
            }
            // fflush(0);
        }

        ULONG32 ulStreamCount = 0;
        header->GetPropertyULONG32("StreamCount",ulStreamCount);

        ULONG32 ulStreamGroupCount = 0;
        if (FAILED(header->GetPropertyULONG32("StreamGroupCount",
						ulStreamGroupCount)))
        {
	    ulStreamGroupCount = ulStreamCount;
        }

        if (m_ppStreams == NULL)
        {
            m_num_streams = (UINT16)ulStreamCount;
            m_uStreamGroupCount = (UINT16)ulStreamGroupCount;
            m_ppStreams = new StreamInfo*[m_num_streams];
            memset(m_ppStreams, 0, m_num_streams*sizeof(StreamInfo*));
        }
        HX_ASSERT(m_num_streams == (UINT16)ulStreamCount);


        ULONG32 ulLatency = 0;
        header->GetPropertyULONG32("MinimizeLatency", ulLatency);
        m_bNoLatency = ulLatency ? TRUE : FALSE;

        ULONG32 ulIsRealDataType = 0;
        header->GetPropertyULONG32("IsRealDataType", ulIsRealDataType);
        m_bIsRealDataType = ulIsRealDataType ? TRUE : FALSE;

        /*
         * Set registry information
         */

        if (m_pRegistryKey && m_pClient->use_registry_for_stats())
        {
            char str[512];
            sprintf(str, "%s.StreamCount", m_pRegistryKey);
            m_pProc->pc->registry->AddInt(str, ulStreamCount, m_pProc);
        }

        // get stream headers
        for (int i = 0; i < m_num_streams; ++i)
        {
            StreamInfo* pInfo = m_ppStreams[i];
            if (pInfo != NULL)
            {
                SessionStreamHeaderReady(HXR_OK, pInfo->m_pActualHeader);
            }
            else
            {
                m_pSourceControl->GetStreamHeader(this, i);
            }
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}


BOOL
Player::Session::SeeIfThisIsRealDataType(IHXValues* pHeader)
{
    HX_ASSERT(m_bUseRTP);
    HX_ASSERT(!m_bIsRealDataType);
    // so, see if this is Real Datatype....This is a tricky one...
    // It has to have a payload of 101.
    UINT32 ulPayloadType = (UINT32)-1;
    BOOL    bIsReal = FALSE;
    if (HXR_OK == pHeader->GetPropertyULONG32("RTPPayloadType", ulPayloadType))
    {
            if (RTP_PAYLOAD_RTSP != ulPayloadType)
            {
                // not Real for sure
                return FALSE;
            }
    }

    // now check for mime type...
    IHXBuffer* mime_type = NULL;
    pHeader->GetPropertyCString("MimeType", mime_type);
    if (mime_type)
    {
            if (strstr((const char*)mime_type->GetBuffer(), "real"))
            {
            // I would say this is a real data type
            bIsReal = TRUE;
        }

        mime_type->Release();
    }

    return bIsReal;
}


HX_RESULT
Player::Session::StreamHeaderReady(HX_RESULT status, IHXValues* header)
{
    if (m_bDone)
    {
        return HXR_OK;
    }

    // Entered
    ++m_ulFunctionsInUse;

    UINT32              stream_number;
    UINT32              unStreamGroupNumber;

    if (status == HXR_NOT_LICENSED && !m_pClient->m_bIsAProxy)
    {
        SendAlert(SE_DATATYPE_UNLICENSED);
    }
    if (status != HXR_OK)
    {
        m_pClient->protocol()->setupStreams(0, this, CONVERT_HXR_ERROR(status));
    }
    else
    {
        // only for RTP
        if (m_bUseRTP && !m_bIsRealDataType)
        {
            // an absence of "IsRealDataType" file header does not gurantee
            // there is no Real datatype....
            m_bIsRealDataType = SeeIfThisIsRealDataType(header);
        }

        header->GetPropertyULONG32("StreamNumber", stream_number);
        if (FAILED(header->GetPropertyULONG32("StreamGroupNumber",
						unStreamGroupNumber)))
        {
	    unStreamGroupNumber = stream_number;
        }

        UINT32 n;
        StreamInfo** ppSlot = NULL;
        for (n = 0; n < m_num_streams; n++)
        {
            StreamInfo* pInfo = m_ppStreams[n];
            if (pInfo != NULL)
            {
                if (pInfo->m_stream_number == stream_number)
                {
                    break;
                }
            }
            else
            {
                if (ppSlot == NULL)
                {
                    ppSlot = &m_ppStreams[n];
                }
            }
        }
        if (ppSlot == NULL)
        {
            // Either a stream is being reused or we've run out of slots
            HX_ASSERT(FALSE);
            return HXR_FAIL;
        }
        *ppSlot = new StreamInfo((UINT16)unStreamGroupNumber,
                                 (UINT16)stream_number);

        IHXBuffer* pRuleBook = NULL;
        header->GetPropertyCString("ASMRuleBook",pRuleBook);
        if (pRuleBook != NULL)
        {
            (*ppSlot)->m_pRuleBookBuffer = pRuleBook;
            pRuleBook = NULL;
        }

        if (stream_number == 0) /* we don't need to map all streams */
        {
            HX_RELEASE(m_pMediaMimeType);
            header->GetPropertyCString("MimeType", m_pMediaMimeType);
        }

        header->AddRef();
        (*ppSlot)->m_pActualHeader = header;
        if (m_pDataConvert)
        {
            m_pDataConvert->ConvertStreamHeader(header);
        }
        else
        {
            SessionStreamHeaderReady(status, NULL);
        }
    }
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return HXR_OK;
}

HX_RESULT
Player::Session::ConvertedStreamHeaderReady(HX_RESULT status,
                                            IHXValues* pStreamHeader)
{
    ++m_ulFunctionsInUse;
    SessionStreamHeaderReady(status, pStreamHeader);
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return HXR_OK;
}

HX_RESULT
Player::Session::SessionStreamHeaderReady(HX_RESULT status,
                                            IHXValues* pConvertedHeader)
{
    HX_RESULT result = HXR_OK;
    UINT32 stream_number = 0;
    StreamInfo* pInfo = NULL;

    ++m_ulFunctionsInUse;

    if (pConvertedHeader)
    {
        pConvertedHeader->GetPropertyULONG32("StreamNumber",
                                                stream_number);
        for (int i = 0; i < m_num_streams; i++)
        {
            pInfo = m_ppStreams[i];
            if (pInfo != NULL && pInfo->m_stream_number == stream_number)
            {
                pConvertedHeader->AddRef();
                pInfo->m_pConvertedHeader = pConvertedHeader;
                break;
            }
        }
    }

    m_num_streamheadersdone++;

    if (m_num_streamheadersdone == m_num_streams)
    {
        int i;
        CHXSimpleList m_header_list;
        CHXSimpleList m_need_release_headers;

        ASSERT(!m_is_ready);

        m_is_ready = TRUE;

        char str[512];

        if (m_pRegistryKey && m_pClient->use_registry_for_stats())
        {
            sprintf(str, "%s.Stream", m_pRegistryKey);
            m_pProc->pc->registry->AddComp(str, m_pProc);
        }

        // Get QOS config properties
        INT32 lRRRate = 0;
        INT32 lRSRate = 0;
        INT32 lUserRRRatio = 0;
        INT32 lUserRSRatio = 0;
        INT32 lBWMult = 0;
        IHXBuffer* pBWProt = NULL;
        if (m_pQoSConfig)
        {
            if (FAILED(m_pQoSConfig->GetConfigInt(QOS_CFG_TRAN_RR_RATE,
                lRRRate)))
            {

                m_pQoSConfig->GetConfigInt(QOS_CFG_TRAN_RR_RATIO, lUserRRRatio);
            }

            if (FAILED(m_pQoSConfig->GetConfigInt(QOS_CFG_TRAN_RS_RATE,
                lRSRate)))
            {

                m_pQoSConfig->GetConfigInt(QOS_CFG_TRAN_RS_RATIO, lUserRSRatio);
            }

            // Pass the bw calculation properties into the
            // session header to be passed to the sdp creator
            if (!m_pSessionHeader)
            {
                m_pCommonClassFactory->CreateInstance(CLSID_IHXValues,
                                        (void**)&m_pSessionHeader);
            }
            m_pQoSConfig->GetConfigInt(QOS_CFG_SDP_BW_MULT, lBWMult);
            m_pQoSConfig->GetConfigBuffer(QOS_CFG_SDP_BW_PROT, pBWProt);

            if (m_pSessionHeader)
            {
                if (lBWMult > 100)
                {
                    m_pSessionHeader->SetPropertyULONG32(
                        "BandwidthMultiplier", (UINT32)lBWMult);
                }
                if (pBWProt)
                {
                    m_pSessionHeader->SetPropertyCString(
                        "BandwidthProtocol", pBWProt);
                }
            }
            HX_RELEASE(pBWProt);
        }

        for (i = 0; i < m_num_streams; i++)
        {
            pInfo = m_ppStreams[i];
            IHXValues* pThisHeader = 0;

            // We better have all slots filled
            HX_ASSERT(pInfo != NULL);

            if (pInfo->m_pConvertedHeader)
            {
                pThisHeader = pInfo->m_pConvertedHeader;
            }
            else
            {
                pThisHeader = pInfo->m_pActualHeader;
            }

            pInfo->m_pActualHeader->GetPropertyULONG32("StreamNumber",
                                                            stream_number);


            /*
             * Fix up conversion mimetype.
             */
            IHXBuffer* pNewMimeType = NULL;
            if (m_pDataConvert)
            {
                const char* pCharMimeType;

                m_pProc->pc->common_class_factory->CreateInstance(
                        CLSID_IHXBuffer, (void**)&pNewMimeType);

                m_pDataConvert->GetConversionMimeType(pCharMimeType);
                /*
                 * If they give us NULL for mimetype then just
                 * send down the converted header as is.  There
                 * will be no reverter.
                 */
                if (pCharMimeType)
                {

                    pNewMimeType->SetSize(31 + strlen(pCharMimeType) + 1);

                    char* pTemp = (char*)pNewMimeType->GetBuffer();

                    sprintf(pTemp, "application/vnd.rn.dataconvert.%s",
                                pCharMimeType);

                    if (pInfo->m_pConvertedHeader)
                    {
                        pThisHeader = FlattenConvertedHeader(pThisHeader,
                                "DataConvertStreamHeader");
                        m_need_release_headers.AddTail((void*)pThisHeader);
                        pThisHeader->SetPropertyCString("MimeType",
                                pNewMimeType);
                        pThisHeader->SetPropertyULONG32("StreamNumber",
                                stream_number);
                    }
                    else
                    {
                        IHXBuffer* pMimeType;
                        /*
                         * For live these headers are cached in the
                         * encoder plugin, so we have already processed
                         * it.  Check for this.
                         */
                        if (HXR_OK == pThisHeader->GetPropertyCString(
                                        "PreConvertMimeType", pMimeType))
                        {
                            pMimeType->Release();
                        }
                        else
                        {
                            pThisHeader->GetPropertyCString("MimeType",
                                    pMimeType);
                            pThisHeader->SetPropertyCString(
                                    "PreConvertMimeType", pMimeType);
                            pMimeType->Release();
                            pThisHeader->SetPropertyCString("MimeType",
                                        pNewMimeType);
                        }
                    }
                }

                pNewMimeType->Release();
            }

            m_header_list.AddTail(pThisHeader);

            /*
             * Set registry information
             */

            ULONG32 duration = 0;
            ULONG32 avg_bit_rate = 0;
            IHXBuffer* mime_type = NULL;
            UINT32 ulTotalAvgBitrate = 0;

            pInfo->m_pActualHeader->GetPropertyULONG32("Duration",
                                                        duration);
            pInfo->m_pActualHeader->GetPropertyULONG32("AvgBitRate",
                                                        avg_bit_rate);
            pInfo->m_pActualHeader->GetPropertyCString("MimeType",
                                                        mime_type);

            if (m_pRegistryKey && m_pClient->use_registry_for_stats())
            {

                sprintf(str, "%s.Stream.%ld", m_pRegistryKey,
                    stream_number);
                m_pProc->pc->registry->AddComp(str, m_pProc);

                sprintf(str, "%s.Stream.%ld.Duration", m_pRegistryKey,
                    stream_number);
                m_pProc->pc->registry->AddInt(str, duration, m_pProc);

                sprintf(str, "%s.Stream.%ld.AvgBitRate", m_pRegistryKey,
                    stream_number);
                m_pProc->pc->registry->AddInt(str, avg_bit_rate, m_pProc);

                sprintf(str, "%s.Stream.%ld.MimeType", m_pRegistryKey,
                    stream_number);
                m_pProc->pc->registry->AddStr(str, mime_type, m_pProc);
            }

            HX_ASSERT(m_pStats);

            if (duration > m_pStats->GetDuration())
            {
                m_pStats->SetDuration(duration);
            }

            if (strncmp((char*)mime_type->GetBuffer(),
                        REALAUDIO_MIME_TYPE,
                        mime_type->GetSize()) == 0)
            {
                m_pStats->SetRAStreamFound(TRUE);
            }
            if (strncmp((char*)mime_type->GetBuffer(),
                        REALVIDEO_MIME_TYPE,
                        mime_type->GetSize()) == 0)
            {
                m_pStats->SetRVStreamFound(TRUE);
            }
            if (strncmp((char*)mime_type->GetBuffer(),
                        IMAGEMAP_MIME_TYPE,
                        mime_type->GetSize()) == 0)
            {
                m_pStats->SetRIStreamFound(TRUE);
            }
            if (strncmp((char*)mime_type->GetBuffer(),
                        REALEVENT_MIME_TYPE,
                        mime_type->GetSize()) == 0)
            {
                m_pStats->SetREStreamFound(TRUE);
            }


            ulTotalAvgBitrate = m_pStats->GetAvgBitrate() + avg_bit_rate;
            m_pStats->SetAvgBitrate(ulTotalAvgBitrate);

            /* set up bandwidth values */
            // calculate RR and RS rates based on BW ratio
            // and set in the stream header
            if (!lRRRate && lUserRRRatio && avg_bit_rate)
            {
                lRRRate = (LONG32)((double)avg_bit_rate *
                    ((double)lUserRRRatio / 10000.0));
            }

            if (!lRSRate && lUserRSRatio && avg_bit_rate)
            {
                lRSRate = (LONG32)((double)avg_bit_rate *
                    ((double)lUserRSRatio / 10000.0));
            }

            if (lRRRate)
            {
                pInfo->m_pActualHeader->SetPropertyULONG32("RtcpRRRate",
                    (UINT32)lRRRate);
            }
            if (lRSRate)
            {
                pInfo->m_pActualHeader->SetPropertyULONG32("RtcpRSRate",
                    (UINT32)lRSRate);
            }

            mime_type->Release();
        }
        /*
         * If the other side did not support "pragma: initiate-session"
         * then we need to add any control buffer that we might have
         * cached to the stream header response.
         */
        if (m_pHeaderControlBuffer)
        {
            m_pClient->protocol()->addToHeader("DataConvertBuffer",
                    m_pHeaderControlBuffer);
        }
        HX_RELEASE(m_pHeaderControlBuffer);

        result = m_pClient->protocol()->setupStreams(&m_header_list, this, HXR_OK);
        IHXValues* pReleaseValues;
        while (!m_need_release_headers.IsEmpty())
        {
            pReleaseValues = (IHXValues*)m_need_release_headers.RemoveHead();
            pReleaseValues->Release();
        }

        if (SUCCEEDED(result))
        {
            m_ProtocolSetupAfterHeadersDone = 1;
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return result;
}

IHXValues*
Player::Session::FlattenConvertedHeader(IHXValues* pFromHeader, const char*
                                        pFromKey)
{
    IHXValues* pNewHeader = NULL;
    IHXBuffer* pNewBuf = NULL;
    m_pProc->pc->common_class_factory->CreateInstance(CLSID_IHXValues,
            (void**)&pNewHeader);
    m_pProc->pc->common_class_factory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pNewBuf);
    UINT32 ulNewLength = 0;
    int phase = 0;
    unsigned char* pData = NULL;

    while (phase < 2)
    {
        HX_RESULT res;
        ULONG32 ul;
        IHXBuffer* pBuffer;
        const char* pName;

        res = pFromHeader->GetFirstPropertyULONG32(pName, ul);
        while (res == HXR_OK)
        {
            if (phase == 0)
            {
                ulNewLength += strlen(pName) + 9;
            }
            else
            {
                *pData = 'u';
                pData++;
                int len = strlen(pName);
                putlong(pData, len);
                pData += 4;
                memcpy(pData, pName, len);
                pData += len;
                putlong(pData, ul);
                pData+= 4;
            }
            res = pFromHeader->GetNextPropertyULONG32(pName, ul);
        }
        res = pFromHeader->GetFirstPropertyCString(pName, pBuffer);
        while (res == HXR_OK)
        {
            if (phase == 0)
            {
                ulNewLength += pBuffer->GetSize() + strlen(pName) + 9;
            }
            else
            {
                *pData = 's';
                pData++;
                int len = strlen(pName);
                putlong(pData, len);
                pData += 4;
                memcpy(pData, pName, len);
                pData += len;

                putlong(pData, pBuffer->GetSize());
                pData += 4;
                memcpy(pData, pBuffer->GetBuffer(), pBuffer->GetSize());
                pData += pBuffer->GetSize();
            }
            pBuffer->Release();
            res = pFromHeader->GetNextPropertyCString(pName, pBuffer);
        }
        res = pFromHeader->GetFirstPropertyBuffer(pName, pBuffer);
        while (res == HXR_OK)
        {
            if (phase == 0)
            {
                ulNewLength += pBuffer->GetSize() + strlen(pName) + 9;
            }
            else
            {
                *pData = 'b';
                pData++;
                int len = strlen(pName);
                putlong(pData, len);
                pData += 4;
                memcpy(pData, pName, len);
                pData += len;

                putlong(pData, pBuffer->GetSize());
                pData += 4;
                memcpy(pData, pBuffer->GetBuffer(), pBuffer->GetSize());
                pData += pBuffer->GetSize();
            }
            pBuffer->Release();
            res = pFromHeader->GetNextPropertyBuffer(pName, pBuffer);
        }
        if (phase == 0)
        {
            pNewBuf->SetSize(ulNewLength);
            pData = (unsigned char*)pNewBuf->GetBuffer();
        }
        phase++;
    }
    pNewHeader->SetPropertyBuffer(pFromKey, pNewBuf);
    pNewBuf->Release();
    return pNewHeader;
}

HX_RESULT
Player::Session::StreamDone(UINT16 stream_number)
{
    if (m_bDone || !m_ppStreams || !m_ppStreams[stream_number]->m_bSetupReceived)
    {
        return HXR_OK;
    }

    // Entered
    ++m_ulFunctionsInUse;

    DPRINTF(D_INFO, ("%ld: stream done\n", m_pClient->conn_id));

    /*
     * If the file format calls StreamDone in response to a
     * GetFileHeader or GetStreamHeader (which, unfortunately, many of them do)
     * then m_pSessionControl will be NULL.  In this case, hang up.
     */
    if (m_pSessionControl)
    {
        m_pSessionControl->StreamDone(stream_number);

        /*
         * We need to tell the protocol when playback is done so that
         * it can put itself back into the ready state.
         */
        ++m_num_streamsdone;
        if (m_num_streamsdone == m_uStreamSetupCount)
        {
            m_pClient->protocol()->playDone(m_sessionID);
        }
        SetInactive();
    }
    else
    {
        SendAlert(SE_INTERNAL_ERROR);
        m_pClient->protocol()->setupStreams(0, this, CONVERT_HXR_ERROR(HXR_INVALID_FILE));
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

HX_RESULT
Player::Session::SeekDone(HX_RESULT status)
{
    if (m_bDone)
    {
        return HXR_OK;
    }

    // Entered
    ++m_ulFunctionsInUse;

    ClearInactive();
    if (status == HXR_OK)
    {
        m_pSessionControl->SeekDone();
    }
    else
    {
        //XXXTDM: what to do here?
        HX_ASSERT(FALSE);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

STDMETHODIMP
Player::Session::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPSinkControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkControl))
    {
        AddRef();
        *ppvObj = (IHXPSinkControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlayerController))
    {
        AddRef();
        *ppvObj = (IHXPlayerController*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlayerControllerProxyRedirect))
    {
        AddRef();
        *ppvObj = (IHXPlayerControllerProxyRedirect*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid,IID_IHXClientBandwidthController))
    {
        AddRef();
        *ppvObj = (IHXClientBandwidthController*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid,IID_IHXSetPlayParamResponse))
    {
        AddRef();
        *ppvObj = (IHXSetPlayParamResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid,IID_IHXSourceFinderFileResponse))
    {
        AddRef();
        *ppvObj = (IHXSourceFinderFileResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileStatResponse))
    {
        AddRef();
        *ppvObj = (IHXFileStatResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
Player::Session::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
Player::Session::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
Player::Session::FindDone(HX_RESULT status, IUnknown* pSourceContainer, IUnknown* pFileObject)
{
    if (m_bDone)
    {
        return HXR_OK;
    }

    // Entered
    ++m_ulFunctionsInUse;

    IHXPSourceControl* pSourceControl = NULL;

    if (SUCCEEDED(status))
    {
        HX_RELEASE(m_pBlockTransferFileFormat);
        HX_RELEASE(m_pFileFormat);

        if ((SUCCEEDED(pSourceContainer->QueryInterface(IID_IHXPSourceControl,
                                                        (void**)&pSourceControl)))
            && (pFileObject))

        {
            IHXFileStat* pFileStat = NULL;
            if (SUCCEEDED(pFileObject->QueryInterface(IID_IHXFileStat,
                                                      (void**)&pFileStat)))
            {
                pFileStat->Stat(this);
                pFileStat->Release();
            }

            pSourceContainer->QueryInterface(IID_IHXFileFormatObject, (void**)&m_pFileFormat);
        }

        if (m_pSrcFinder && pSourceControl && pSourceControl->IsLive())
        {
            //Save the current broadcast source plugin type.
            //This is required for creating a streamer for multicast.
           if (FAILED(m_pSrcFinder->GetCurrentSourceTye(m_BroadcastType)))
	   {
	       //This should not happen as status indicates SUCCEESS.
	       HX_ASSERT(FALSE);
	       status = HXR_FAIL;
	   }
        }
    }

    FindDone(status, pSourceControl);

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return HXR_OK;
}

void
Player::Session::FindDone(HX_RESULT status, IHXPSourceControl* source)
{
    // Entered
    ++m_ulFunctionsInUse;

    if(m_pBroadcastMapper)
    {
        m_pBroadcastMapper->Release();
        m_pBroadcastMapper = NULL;
    }

    if(m_pMimeMapper)
    {
        m_pMimeMapper->Release();
        m_pMimeMapper = NULL;
    }

    if (m_bRedirected)
    {
        status = HXR_REDIRECTION;
    }

    if (status == HXR_OK)
    {
        HX_ASSERT(source != NULL);

        if (m_pSourceControl)
        {
            // the last source failed, close and release it
            m_pSourceControl->Done();
            HX_RELEASE(m_pSourceControl);
        }

        m_pSourceControl = source;

        // Get uber stream mgr -- OK if source control doesn't
        // support (only needed for Helix Adaptation)
        m_pSourceControl->QueryInterface(IID_IHXUberStreamManager,
            (void**)&m_pUberStreamMgr);

        m_pSourceControl->QueryInterface(IID_IHXUberStreamManagerConfig,
            (void**)&m_pUberStreamConfig);

        if (m_pUberStreamConfig && m_ulClientAvgBandwidth > 0)
        {
            m_pUberStreamConfig->SetClientAverageBandwidth(m_ulClientAvgBandwidth);
        }

        m_pUberStreamConfig->SetRateSelectionInfoObject(m_pRateSelInfo);

        /* XXDWL
         * For live (broadcast) sessions, the media delivery pipeline needs to be turned
         * off. This is a temporary solution until a number of issues dealing with
         * timestamp dependent media, and ASM rule handling are worked out.
         * See also: server/engine/inputsource/srcfinder.cpp:444
         */
        if ((m_bUseMediaDeliveryPipeline) &&
            (m_pSourceControl->IsLive()))
        {
            m_bUseMediaDeliveryPipeline = FALSE;
        }

        // XXXJR Sort of a hack.  Don't know what else to do though.
        //<code deleted>
            //XXXBAB more hacks in the same vein...
            if(m_pClient->m_protType == HXPROT_RTSP && m_bUseRTP)
            {
                IHXPacketFormat* pPacketFormat = 0;
                if(m_pFileFormat)   //XXXBAB NOT FOR LIVE, PAL
                {
                    m_pFileFormat->QueryInterface (IID_IHXPacketFormat,(void**)&pPacketFormat);

                    if(pPacketFormat)
                    {
                        const char** pSupportedFormats = 0;
                        if(SUCCEEDED(pPacketFormat->GetSupportedPacketFormats(pSupportedFormats)))
                        {
                            int idx = 0;
                            const char* pFormat = pSupportedFormats[idx++];
                            while(pFormat)
                            {
                                if(strcasecmp(pFormat, "rtp") == 0)
                                {
                                    pPacketFormat->SetPacketFormat("rtp");
                                    break;
                                }
                                pFormat = pSupportedFormats[idx++];
                            }
                        }
                        pPacketFormat->Release();
                    }
                }
            }

            if(m_pClient->m_protType == HXPROT_RTSP)
            {
                if(m_pFileFormat && m_bBlockTransfer)
                {
                    HX_RELEASE(m_pBlockTransferFileFormat);
                    m_pFileFormat->QueryInterface(IID_IHXBlockFormatObject,
                             (void**)&m_pBlockTransferFileFormat);
                    if(!m_pBlockTransferFileFormat)
                    {
		    SessionInitDone(HXR_SE_INTERNAL_ERROR);
		    status = HXR_SE_INTERNAL_ERROR;
                    }
                }
            }
	if (status == HXR_OK)
	{
            if (m_pFileFormat && !m_pFFAdvise)
            {
                m_pFileFormat->QueryInterface(IID_IHXFileFormatHeaderAdvise,
                                              (void**) &m_pFFAdvise);
            }

            if (HXR_OK != m_pSourceControl->Init(this))
            {
		status = HXR_SE_INTERNAL_ERROR;
            }
        }
    }
    else
    {
        m_pClient->protocol()->setupStreams(0, this, CONVERT_HXR_ERROR(status));
    }

    if (status != HXR_OK && source)
    {
	source->Done();
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

int
Player::Session::got_url(URL* pUrl, BOOL bUseRTP)
{
    char str[512];

    // Entered
    ++m_ulFunctionsInUse;

    HX_DELETE(m_url);
    m_url = pUrl;
    m_bUseRTP = bUseRTP;

    if (pUrl->host)
    {
        // Add the host to the registry
        IHXBuffer* pHost = new ServerBuffer(TRUE);
        pHost->Set((UCHAR*)pUrl->host, strlen(pUrl->host) + 1);
        if (m_pRegistryKey && m_pClient->m_bUseRegistryForStats)
        {
            sprintf(str, "%s.Host", m_pRegistryKey);
            m_pProc->pc->registry->AddStr(str, pHost, m_pProc);
        }
        m_pStats->SetHost(pHost);
        pHost->Release();
    }

    if (m_url->parameters)
    {
        char* pIR = NULL;
        if (!strncmp(m_url->parameters, "?ir=", 4))
        {
            pIR = m_url->parameters + 4;
        }
        else if (pIR = strstr(m_url->parameters, "&ir="))
        {
            pIR += 4;
        }
        
        if (pIR && *pIR)
        {
            m_pRateSelInfo->SetInfo(RSI_QUERYPARAM_IR, atoi(pIR));
        }
    }
    

    // Add the url to the registry
    IHXBuffer* pURL = new ServerBuffer(TRUE);
    pURL->Set((BYTE*)m_url->full, strlen(m_url->full) + 1);
    if (m_pRegistryKey && m_pClient->m_bUseRegistryForStats)
    {
        sprintf(str, "%s.URL", m_pRegistryKey);
        m_pProc->pc->registry->AddStr(str, pURL, m_pProc);
    }
    m_pStats->SetURL(pURL);
    pURL->Release();

    /*
     * We don't use MDP for MEI (block transfer) requests.  If it is ever
     * desired to do so, the code in FindDone() and the MDP call semantics
     * will need examined.  Currently the QI for IID_IHXBlockFormatObject
     * fails and the code attempts to send two responses to the DESCRIBE:
     * one inside m_pSourceControl->Init(this) and the other inside
     * source->Done().  This causes a CA in the RTSP layer.
     */
    if (!m_bBlockTransfer)
    {
        // Select the Media Delivery Profile based on the User-Agent
        IHXBuffer* pUserAgent = m_pClient->get_client_stats()->GetUserAgent();
        if (pUserAgent)
        {
            INT32 lConfigId = 0;
            if (SUCCEEDED(m_pProc->pc->qos_prof_select->
                          SelectProfile(pUserAgent, NULL, NULL, lConfigId)))
            {
                INT32 lTemp = 0;

                if (m_pQoSConfig != NULL)
                {
                    HX_ASSERT(FALSE);
                    HX_RELEASE(m_pQoSConfig);
                }
                HX_VERIFY(SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSProfileConfigurator,
                                                       (void**)&m_pQoSConfig)));
                m_pQoSConfig->SetConfigId (lConfigId);

                if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_MDP, lTemp)))
                {
                    m_bUseMediaDeliveryPipeline = (BOOL)lTemp;
                }

                if (SUCCEEDED(m_pQoSConfig->GetConfigInt(QOS_CFG_INACTIVITY_TIMEOUT,
                                lTemp)))
                {
                    m_ulInactivityTimeout = (UINT32)lTemp;
                }
            }
            HX_RELEASE(pUserAgent);
        }
    }

    // Run the Allowance plugins..
    OnURL(m_pRequest);
    // Continues in OnURLDone()..

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return 0;
}

HX_RESULT
Player::Session::HandleDefaultSubscription()
{
    // Entered
    ++m_ulFunctionsInUse;

    HX_ASSERT(!m_bUseMediaDeliveryPipeline);

    HX_RESULT theErr = HXR_OUTOFMEMORY;
    Player::Session::StreamInfo*    pInfo;
    UINT16 uStream;

    IHXUberStreamManagerInit* pUberMgrInit = NULL;
    theErr = CreateUberStreamManager(m_pProc->pc->common_class_factory, &pUberMgrInit);

    if (HXR_OK == theErr)
    {
        theErr = pUberMgrInit->Init(FALSE);
    }

    if (HXR_OK == theErr)
    {
        theErr = pUberMgrInit->SetASMSource(this);
    }

    if (HXR_OK == theErr)
    {
        theErr = pUberMgrInit->SetFileHeader(m_pActualFileHeader);
    }

    if (HXR_OK == theErr)
    {
        for (uStream = 0; uStream < m_num_streams; uStream++)
        {
            pInfo = m_ppStreams[uStream];
            if (pInfo != NULL)
            {
                HX_ASSERT(pInfo->m_stream_number == uStream);
                if (pInfo->m_pActualHeader && HXR_OK == theErr)
                {
                    pUberMgrInit->SetStreamHeader(uStream, pInfo->m_pActualHeader);
                }
            }
        }
    }

    IHXUberStreamManager* pUberStreamMgr = NULL;
    if (HXR_OK == theErr)
    {
        theErr = pUberMgrInit->QueryInterface(IID_IHXUberStreamManager,
            (void**)&pUberStreamMgr);
    }

#ifndef OLD_STYLE_DEFAULT_STREAM_SELECTION
    if (HXR_OK == theErr)
    {
	/**************************************************************
	* Set Inputsource to select the Logical stream that have been
	*     explicitly setup
	* ***************************************************************/
        for (uStream = 0; uStream < m_num_streams; uStream++)
        {
            pInfo = m_ppStreams[uStream];

	    // Only logical streams that have been setup will be selected
            if (pInfo != NULL && pInfo->m_bSetupReceived)
            {
                HX_ASSERT(pInfo->m_stream_number == uStream);
                if (pInfo->m_pActualHeader && HXR_OK == theErr)
                {
		    IHXRateDescEnumerator* pLogicalStreamEnum = NULL;
		    IHXRateDescription* pRateDesc = NULL;

		    //Get the RateDescription for the logical stream using streams
		    // AvgBitRate and set it as the RateDescription for the StreamGroup
		    //
		    if (SUCCEEDED(pUberStreamMgr->GetLogicalStream(uStream
								, pLogicalStreamEnum)))
		    {
                        UINT32 ulBitRate = 0;
                        UINT32 ulRule = 0xFFFF;

		        pInfo->m_pActualHeader->GetPropertyULONG32(
                            "AvgBitRate", ulBitRate);
                        pInfo->m_pActualHeader->GetPropertyULONG32(
                            "BaseRule", ulRule);

                        if ((ulRule != 0xFFFF &&
                            SUCCEEDED(pLogicalStreamEnum->FindRateDescByRule(
                                        ulRule, TRUE, FALSE, pRateDesc))) ||
			    SUCCEEDED(pLogicalStreamEnum->FindRateDescByClosestAvgRate(
					ulBitRate, TRUE, FALSE, pRateDesc)) ||
			    SUCCEEDED(pLogicalStreamEnum->FindRateDescByMidpoint(
					ulBitRate, TRUE, FALSE, pRateDesc)))
			{
			    if (pRateDesc)
			    {
                                DPRINTF(0x02000000,
                                    ("Player::Session::HandleDefaultSubscriptions(), "
                                    "found RateDesc for StreamNum: %u, "
                                    "m_uRegisterStreamGroupNumber: %u, "
                                    "ulAvgRate: %u, pRateDesc: %p\n",
                                    uStream, pInfo->m_uStreamGroupNumber,
                                    ulBitRate, pRateDesc));

				pUberStreamMgr->SetStreamGroupRateDesc(
                                    pInfo->m_uStreamGroupNumber, uStream,
                                    pRateDesc, NULL);
				HX_RELEASE(pRateDesc);
			    }
			}
			HX_RELEASE(pLogicalStreamEnum);
		    }
                }
            }
        }
    }
#else
    if (HXR_OK == theErr)
    {
        INT32 lInitRate = 0;
        if (m_pQoSConfig)
        {
            if (FAILED(m_pQoSConfig->GetConfigInt(QOS_CFG_IS_DEFAULT_MEDIARATE, lInitRate)))
            {
                lInitRate = 0;
                theErr = HXR_OK;
            }
        }

        // Find closest (lower or equal) available matching rate -- must be selectable
        IHXRateDescription* pRateDesc = NULL;
        if (SUCCEEDED(theErr))
        {
            theErr = pUberStreamMgr->FindRateDescByClosestAvgRate((UINT32)lInitRate, TRUE, TRUE, pRateDesc);

            // Just take the first one
            if (FAILED(theErr))
                theErr = pUberStreamMgr->GetSelectableRateDescription(0, pRateDesc);
        }

        if (SUCCEEDED(theErr))
            theErr = pUberStreamMgr->SetAggregateRateDesc(pRateDesc, NULL);

        if (SUCCEEDED(theErr))
            theErr = pUberStreamMgr->CommitInitialAggregateRateDesc();

        HX_RELEASE(pRateDesc);
    }
#endif

    /*
     *  XXXGo - too late to reflect the change in headers since we are
     *          processing PLAY request.
     */
#if 0
     if (HXR_OK == theErr)
     {
        for (uStream = 0; uStream < m_num_streams && HXR_OK == theErr; uStream++)
        {
            pInfo = m_ppStreams[uStream];
            if (pInfo != NULL)
            {
                if (pInfo->m_pActualHeader)
                {
                    // modifying the original....
                    theErr = pHandler->ModifyHeaders(pInfo->m_pActualHeader);
                }
            }
        }
    }
#endif
    HX_RELEASE(pUberMgrInit);
    HX_RELEASE(pUberStreamMgr);

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return theErr;
}

/*
 * IHXASMSource
 * This Subscribe should only be called from ASMRuleHandler when MediaPipeline
 * is not used AND there is no external Subscription coming from a client at
 * PLAY time
 */
STDMETHODIMP
Player::Session::Subscribe(UINT16 uStreamNumber, UINT16 uRuleNumber)
{
    HX_ASSERT(!m_bUseMediaDeliveryPipeline);
    handle_subscribe(uRuleNumber, uStreamNumber);
    return HXR_OK;
}

STDMETHODIMP
Player::Session::Unsubscribe(UINT16 uStreamNumber, UINT16 uRuleNumber)
{
    HX_ASSERT(!"shouldn't be called");
    return HXR_NOTIMPL;
}

void
Player::Session::handle_subscribe(UINT16 ruleNumber, UINT16 groupNumber)
{
    // Entered
    ++m_ulFunctionsInUse;

    UINT16 streamNumber = GetSetupStream(groupNumber);
    if (streamNumber == INVALID_STREAM_NUMBER)
    {
        streamNumber = groupNumber;
    }

    if (m_pFileFormat && !m_bUseMediaDeliveryPipeline)
    {
        IHXASMSource* pASMSource = 0;
        m_pFileFormat->QueryInterface(IID_IHXASMSource,
                                      (void**)&pASMSource);
        if(pASMSource)
        {
            HX_RESULT hxr = pASMSource->Subscribe(streamNumber, ruleNumber);
            pASMSource->Release();

            if (HXR_OK != hxr)
            {
                SendAlert(SE_INTERNAL_ERROR);
                goto handle_subscribe_exit;
            }
        }
    }
    if (m_pSessionControl == NULL)
    {
        SendAlert(SE_INTERNAL_ERROR);
        goto handle_subscribe_exit;

    }

    m_pSessionControl->HandleSubscribe(ruleNumber, streamNumber);
    m_pRateSelInfo->SetInfo(RSI_SUBSCRIBE_RULE, streamNumber, ruleNumber);

handle_subscribe_exit:
    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::handle_unsubscribe(UINT16 ruleNumber, UINT16 groupNumber)
{
    // Entered
    ++m_ulFunctionsInUse;

    UINT16 streamNumber = GetSetupStream(groupNumber);
    if (streamNumber == INVALID_STREAM_NUMBER)
    {
        streamNumber = groupNumber;
    }

    if (m_pFileFormat && !m_bUseMediaDeliveryPipeline)
    {
        IHXASMSource* pASMSource = 0;
        m_pFileFormat->QueryInterface(IID_IHXASMSource,
                                      (void**)&pASMSource);
        if(pASMSource)
        {
            pASMSource->Unsubscribe(streamNumber, (UINT16)ruleNumber);
            pASMSource->Release();
        }
    }
    if (m_pSessionControl == NULL)
    {
        SendAlert(SE_INTERNAL_ERROR);
    }
    else
    {
        m_pSessionControl->HandleUnSubscribe(ruleNumber, streamNumber);
        m_pRateSelInfo->SetInfo(RSI_UNSUBSCRIBE_RULE, streamNumber, ruleNumber);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::set_drop_rate(UINT16 streamNumber, UINT32 dropRate)
{
    // Entered
    ++m_ulFunctionsInUse;

    m_pSessionControl->SetDropRate(streamNumber, dropRate);

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::set_dropto_bandwidth_limit(UINT16 streamNumber,
                                            UINT32 ulBandwidthLimit)
{
    // Entered
    ++m_ulFunctionsInUse;

    m_pSessionControl->SetDropToBandwidthLimit(streamNumber,
                                                ulBandwidthLimit);
    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::set_delivery_bandwidth(UINT32 ulBackOff,
                                        UINT32 ulBandwidth)
{
    // Entered
    ++m_ulFunctionsInUse;

    if (m_pSessionControl)
    {
        m_pSessionControl->SetDeliveryBandwidth(ulBackOff, ulBandwidth);
    }

    m_pRateSelInfo->SetInfo(RSI_SDB, ulBandwidth);

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}
void
Player::Session::handle_backchannel_packet(IHXPacket* pPacket)
{
    // Entered
    ++m_ulFunctionsInUse;

    if(!m_pFileFormat)
    {
        goto handle_backchannel_exit;
    }
    else
    {
        // XXXSMP This is horribly inefficient, but all of the code above it
        // does the same thing.  I'll fix them all at the same time.
        IHXBackChannel* pBackChannel = 0;
        m_pFileFormat->QueryInterface(IID_IHXBackChannel,
                                      (void**)&pBackChannel);
        if(pBackChannel)
        {
            pBackChannel->PacketReady(pPacket);
            pBackChannel->Release();
        }
    }

handle_backchannel_exit:
    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::seek(UINT32 offset)
{
    // Entered
    ++m_ulFunctionsInUse;

    DPRINTF(D_ENTRY, ("Player::seek(%lu)\n", offset));

    if (m_pUberStreamMgr && m_uStreamGroupCount)
    {
        m_pUberStreamMgr->CommitInitialAggregateRateDesc();
    }

    m_pSessionControl->StartSeek(offset);

    for (int i = 0; i < m_num_streams; i++)
    {
        StreamInfo* pInfo = m_ppStreams[i];
        pInfo->m_is_done = FALSE;
        pInfo->m_waiting_for_packet = FALSE;
    }


    /*
     *  Need to not send the initial 0 seek to the file formats
     *  because it is not really a seek. -Paulm
     */
    if(!m_bBegun && offset == 0)
    {
        SeekDone(HXR_OK);
    }
    else
    {
        // XXXST - prevent live streams which do not implement Seek()
        // from crashing.  Should be handled in RTSPProtocol::HandlePlayRequest
        // because live streams should not have this->seek() called.

        HX_RESULT result = m_pSourceControl->Seek(offset);
        if (HXR_OK != result && HXR_NOTIMPL != result
            && m_pSourceControl->IsLive())
        {
           SeekDone(HXR_OK); // XXXJR weird, but seems ok.
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::set_endpoint(UINT32 endpoint)
{
    // Entered
    ++m_ulFunctionsInUse;

    m_pSessionControl->SetEndPoint(endpoint, FALSE);

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}


void
Player::Session::set_byterange(UINT64 ulFrom, UINT64 ulTo)
{
    // Entered
    ++m_ulFunctionsInUse;

    if(m_pBlockTransferFileFormat)
    {
        m_pBlockTransferFileFormat->SetByteRange(ulFrom, ulTo);

        if(m_bBegun)
            m_pSessionControl->Activate();
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::set_midbox(BOOL bIsMidBox)
{
    // Entered
    ++m_ulFunctionsInUse;

    m_bIsMidBox = bIsMidBox;

    // Notify all of the allowance managers
    if (m_pAllowanceMgrs)
    {
        AllowanceMgr* pManager = NULL;

        LISTPOSITION pos = m_pAllowanceMgrs->GetHeadPosition();
        while(pos)
        {
            // Get next Manager
            pManager = (AllowanceMgr*)m_pAllowanceMgrs->GetAt(pos);

            if (pManager)
            {
                pManager->SetMidBox(bIsMidBox);
            }

            m_pAllowanceMgrs->GetNext(pos);
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::begin()
{
    // Entered
    ++m_ulFunctionsInUse;

    DPRINTF(D_ENTRY, ("Player::Session::begin()\n"));

    ClearInactive();
    m_num_streamsdone = 0;
    m_bBegun = TRUE;

    // Notify all Allowance plugins
    OnBegin();

    if (m_bInitialized)
    {
        log_start_time();
        // let the good timestamps roll
        m_pSessionControl->Play();
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::SetStreamStartTime(UINT32 ulStreamNum, UINT32 ulTimestamp)
{
    HXProtocol* pProtocol = m_pClient->protocol();

    if (m_pClient->m_protType == HXPROT_RTSP)
    {
        pProtocol->SetStreamStartTime(m_sessionID, ulStreamNum, ulTimestamp);
    }
}

/* XXX PSH - Needs to be changed to handle individual paused streams */
void
Player::Session::pause(UINT32 ulPausePoint)
{
    // Entered
    ++m_ulFunctionsInUse;

    DPRINTF(D_ENTRY, ("Player::Session::pause()\n"));

    // Notify all Allowance plugins
    OnPause();
    SetInactive();

    if (m_pSessionControl)
    {
        m_pSessionControl->Pause(ulPausePoint);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::clear_stats()
{
    // Entered
    ++m_ulFunctionsInUse;

    if (m_pClient->use_registry_for_stats())
    {
        clear_registry();
    }

    if (m_pStats && m_pClient)
    {
        m_pClient->get_client_stats()->RemoveSession(m_pStats->GetID());
    }

    --m_ulFunctionsInUse;
    IfDoneCleanup();

}

/*
 *  When Retain-Entity-For-Setup is not required (e.g. DESCRIBE request coming
 *  from QT player), unless we clean up at least session count in the reg,
 *  logpling is going to log this access twice.  So, clean up the reg.
 */
void
Player::Session::clear_registry()
{
    // Entered
    ++m_ulFunctionsInUse;

    if (m_pClient->GetRegId(ClientRegTree::SESSION))
    {
        INT32 pCount;
        ServerRegistry* pReg = m_pProc->pc->registry;

        if (HXR_OK ==
            pReg->GetInt(m_pClient->GetRegId(ClientRegTree::SESSION_COUNT),
                             &pCount, m_pProc))
        {
            // remove the previous registry entry
            pCount--;
            pReg->SetInt(m_pClient->GetRegId(ClientRegTree::SESSION_COUNT),
                             pCount, m_pProc);
            pReg->Del(m_ulRegistryID, m_pProc);
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::init_stats(IHXBuffer* pLocalAddr)
{
    if (m_pClient->use_registry_for_stats())
    {
        init_registry();
    }

// The session stats object is created in the proxy session in the RTSP proxy case.
// It is then set by the RTSPProtocol.
    if (!m_pClient->m_bIsAProxy)
    {
        m_pStats = new SessionStats();
        m_pStats->AddRef();
        m_pClient->get_client_stats()->AddSession(m_pStats);
        m_ulSessionStatsObjId = m_pStats->GetID();

        m_pStats->SetInterfaceAddr(pLocalAddr);

        ServerRegistry* pReg = m_pProc->pc->registry;
        INT32 lEnableRSDLog = 0;
        pReg->GetInt("config.LogStartupDelays", &lEnableRSDLog, m_pProc);

        if (lEnableRSDLog)
        {
            m_pStats->QueryInterface(IID_IHXSessionStats2, (void **)&m_pStats2);
            if(m_pStats2)
            {
                m_pStats2->SetSessionEstablishmentTime(HX_GET_BETTERTICKCOUNT());
                m_pStats2->SetConnectTime(m_pClient->m_ulCreateTime);
            }
        }

        SetupQoSAdaptationInfo();
    }
}


void
Player::Session::init_registry()
{
// Redundant check exists for RTSP proxy case.
    if (!m_pClient->use_registry_for_stats())
    {
        return;
    }

    // Entered
    ++m_ulFunctionsInUse;

    UINT32 ulPlayerRegID = 0;

    if ((ulPlayerRegID = m_pClient->GetRegId(ClientRegTree::CLIENT)))
    {
        // Tell the allowance plugins what Registry entry to look at
        // for information about this Player
        SetRegistryID(ulPlayerRegID);
    }

    if (m_pClient->GetRegId(ClientRegTree::SESSION))
    {
        INT32 pCount;
        IHXBuffer* pName;
        ServerRegistry* pReg = m_pProc->pc->registry;

        if (HXR_OK == pReg->GetPropName(m_pClient->GetRegId(ClientRegTree::SESSION),
                                            pName, m_pProc) &&
            HXR_OK == pReg->GetInt(m_pClient->GetRegId(ClientRegTree::SESSION_COUNT),
                                       &pCount, m_pProc))
        {
            m_pRegistryKey = new char[512];

        // Start the count at 1 and increment.
            m_ulSessionRegistryNumber = ++pCount;
            sprintf(m_pRegistryKey, "%s.%ld",
                                    (const char*)pName->GetBuffer(),
                                    pCount);
                                    //pCount++);
            m_ulRegistryID = pReg->AddComp(m_pRegistryKey, m_pProc);
            pReg->SetInt(m_pClient->GetRegId(ClientRegTree::SESSION_COUNT),
                             pCount,
                             m_pProc);
            pName->Release();
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}


void
Player::Session::SetupQoSAdaptationInfo()
{
    if (m_pSignalBus)
    {
        IHXQoSTransportAdaptationInfo* pTransport = NULL;
        IHXQoSSessionAdaptationInfo* pSession = NULL;
        IHXQoSApplicationAdaptationInfo* pApp = NULL;

        if (SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSTransportAdaptationInfo,
                                                   (void**)&pTransport)))
        {
            m_pStats->SetQoSTransportAdaptationInfo(pTransport);
        }

        if (SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSSessionAdaptationInfo,
                                                   (void**)&pSession)))
        {
            m_pStats->SetQoSSessionAdaptationInfo(pSession);
        }

        if (SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSApplicationAdaptationInfo,
                                                   (void**)&pApp)))
        {
            m_pStats->SetQoSApplicationAdaptationInfo(pApp);
        }

        HX_RELEASE(pTransport);
        HX_RELEASE(pSession);
        HX_RELEASE(pApp);
    }
}

void
Player::Session::log_start_time()
{

    char szDateTime[64];
    char szTimeZoneDiff[16];
    char szStartTime[128];
    char szRegKey[128];

    struct tm localTime;

    INT32 nTimeZone        = 0;
    char cSign             = 0;

    time_t tTime           = 0;
    time(&tTime);

    hx_localtime_r(&tTime, &localTime);
    strftime(szDateTime, 64, "%d/%m/%Y %H:%M:%S", &localTime);

// Timezone stuff is nearly a straight copy from logplin.
#if defined(__linux) || defined(WIN32) || defined __sgi || defined __sun || defined(_HPUX) || defined(_AIX)
//
// From what I can see, timezone is the number of seconds it will take to
// get to GMT which is backwards from FreeBSD
//
    nTimeZone = -timezone;
    nTimeZone += (3600 * localTime.tm_isdst);
#elif !defined __hpux && !defined _AIX
    nTimeZone = localTime.tm_gmtoff;
#endif // __linux

    if (nTimeZone < 0)
    {
       nTimeZone = -nTimeZone;
       cSign = '-';
    }
    else
    {
       cSign = '+';
    }

    snprintf(szTimeZoneDiff, 16, "%c%02ld%02ld", cSign, nTimeZone / 3600, nTimeZone % 3600);
    snprintf(szStartTime, 128, "[%s %s]", szDateTime, szTimeZoneDiff);

    IHXBuffer* pStartTime = new ServerBuffer(TRUE);
    pStartTime->Set((BYTE*)szStartTime, strlen(szStartTime) + 1);
    if (m_pClient->use_registry_for_stats())
    {
        snprintf(szRegKey, 128, "%s.SessionStartTime", m_pRegistryKey);
        m_pProc->pc->registry->AddStr(szRegKey, pStartTime, m_pProc);
    }

    HX_ASSERT(m_pStats);

    m_pStats->SetSessionStartTime(pStartTime);
    pStartTime->Release();

}


void
Player::Session::SetTransportConverter(DataConvertShim* pTransConv)
{
    HX_RELEASE(m_pMulticastTransportDataConvert);
    m_pMulticastTransportDataConvert = pTransConv;
    m_pMulticastTransportDataConvert->AddRef();
}

/* MulticastSession is only included if setup is needed */
void
Player::Session::setup_transport(BOOL bIsMulticast,
                                 BOOL bIsActuallyRTP,
                                 MulticastSession* pSession,
                                 ASMRuleBook** ppASMRuleBooks)
{
    // Entered
    ++m_ulFunctionsInUse;

    StreamInfo*                 pStreamInfo = 0;
    Transport*        pTransport = 0;
    HXProtocol*                 pProtocol = 0;
    PacketFlowWrapper*          pMultiPacketFlowWrap = 0;
    IHXPacketFlowControl*          pMultiSessionControl = 0;
    int i;
    UINT32                      bIsReliable = 0;
    UINT32                      ulTransportCount = 0;
    char                        str[512];

    pProtocol = m_pClient->protocol();

    if (1 == m_pProc->pc->config->GetInt(m_pProc, "config.NoBroadcastBuffering"))
    {
        m_bNoLatency = TRUE;
    }

    if (HXR_OK != SetSessionStreamAdaptScheme())
    {
        goto setup_transport_exit;
    }

    if (pSession)
    {
        pMultiPacketFlowWrap = new PacketFlowWrapper(m_pProc, bIsActuallyRTP);

        IHXPSourceControl* pSource = 0;
        const char* pURL = 0;
        m_pRequest->GetURL(pURL);

        m_pProc->pc->broadcast_manager->GetStream((const char*)m_BroadcastType,
                pURL,
                pSource, m_pProc, m_bUseMediaDeliveryPipeline,
                m_bRetainEntityForSetup ? m_pStats : NULL);


        /*
         * If we have a control data convert then now is the time to
         * create and set the transport DataConvert.
         */
        if (m_pDataConvert)
        {
            IHXDataConvert* pDataConvert = m_pProc->pc->data_convert_con->
                        GetConverter(m_pProc, m_pURLBuf);

            m_pMulticastTransportDataConvert =
                new DataConvertShim(pDataConvert, m_pProc);
            m_pMulticastTransportDataConvert->AddRef();
            m_pMulticastTransportDataConvert->SetControlResponse(NULL);
            /*
             * The DataConvertShim will handle making sure that
             * this guy returns his init done before we call
             * him with more, ie it will cache whatever we give it
             * until it receives that call.  The NULL above unsures that.
             */
            /*
             * Have to do the multicast stuff before we init
             */
            m_pMulticastTransportDataConvert->DataConvertInit(
                    m_pProc->pc->server_context);
            m_pMulticastTransportDataConvert->AddMulticastControl(
                    m_pDataConvert);
            m_pDataConvert->SetMulticastTransport(
                       m_pMulticastTransportDataConvert);
            HX_RELEASE(pDataConvert);

        }

        /*
         * it will call RegisterSource() and sessionCtrl will be assigned
         * to pMultiSessionControl
         */
        pSession->MulticastSessionSetup(pMultiPacketFlowWrap, pSource, m_pStats, m_num_streams,
                                        pMultiSessionControl,
                                        m_pMulticastTransportDataConvert);

        HX_ASSERT(pMultiSessionControl);

#if 0
        pSession->MulticastSessionSetup(pMultiPacketFlowWrap, pSource);
        pMultiPacketFlowWrap->RegisterSource(pSource, &pMultiSessionControl,
                                          m_num_streams, TRUE, FALSE);
#endif // 0

        pSource->Release();
    }
    else if (m_pMulticastTransportDataConvert)
    {
        m_pMulticastTransportDataConvert->AddMulticastControl(m_pDataConvert);
    }

    if (m_pPlayer->m_pPacketFlowWrap == NULL)
    {
        m_pPlayer->m_pPacketFlowWrap =
            new PacketFlowWrapper(m_pProc, bIsActuallyRTP);
    }

    if (m_bNoLatency)
    {
        m_pSourceControl->SetLatencyParams(250, TRUE, FALSE);
    }

    if(m_pStats2)
    {
        m_pStats2->SetSessionSetupTime(HX_GET_BETTERTICKCOUNT());
    }

    m_pPlayer->m_pPacketFlowWrap->RegisterSource(m_pSourceControl, m_pSessionControl,
                                      m_pStats, m_num_streams,
                                      m_bUseMediaDeliveryPipeline,
                                      m_pSourceControl->IsLive(),
                                      bIsMulticast, m_pDataConvert,
                                      m_pPlayer, m_sessionID);
    if (m_pClient->use_registry_for_stats() && m_pRegistryKey)
    {
        sprintf(str, "%s.Transport", m_pRegistryKey);
        m_pProc->pc->registry->AddComp(str, m_pProc);
    }

    // Pass Stream Adaptation Scheme to Packet Flow manager
    if (HXR_OK == m_pSessionControl->QueryInterface(IID_IHXStreamAdaptationSetup,
                                      (void **)&m_pStreamAdaptationSetup))
    {
        if (!SUCCEEDED(m_pStreamAdaptationSetup->SetStreamAdaptationScheme(m_StreamAdaptationScheme)))
        {
            goto setup_transport_exit;
        }
    }

    // Get Packet Flow Manager's LinkCharSetup Interface
    m_pSessionControl->QueryInterface(IID_IHXQoSLinkCharSetup, (void **)&m_pLinkCharSetup);
    if(m_pLinkCharSetup && m_pSessionAggrLinkCharParams)
    {
        if (!SUCCEEDED(m_pLinkCharSetup->SetLinkCharParams(m_pSessionAggrLinkCharParams)))
        {
            goto setup_transport_exit;
        }
    }


    //* Set the Maximum Preroll that will be used by the buffer model
    //*  if set as Helix-Adaptation aggregate
    if (m_pAggRateAdaptParams)
    {
	m_pAggRateAdaptParams->m_ulMaxPreRoll = m_ulMaxPreRoll;
    }

    for (i = 0; i < m_num_streams; i++)
    {
        pStreamInfo = m_ppStreams[i];

        if (!pStreamInfo->m_bSetupReceived)
        {
	    continue;
        }

        //Pass the Stream Adaptation Parameters to Packet Flow
        //if it supports stream adaptation.
        // This information will be later used by PacketFlow to set up
        // QoS frameworkn the subsequent RegisterStream call.
        if (m_pStreamAdaptationSetup)
        {

	    //* Use aggregate stream adaptation params for Helix-Adaptation
	    if (m_bHlxStreamAdaptScheme && m_pAggRateAdaptParams)
	    {
		if (!SUCCEEDED(m_pStreamAdaptationSetup->SetStreamAdaptationParams(
                                        m_pAggRateAdaptParams)))
                {
                    goto setup_transport_exit;
                }
	    }
	    else
	    {
		if (m_ppStreams[i]->m_pStreamAdaptParams)
		{
		    if (!SUCCEEDED(m_pStreamAdaptationSetup->SetStreamAdaptationParams(
                                        m_ppStreams[i]->m_pStreamAdaptParams)))
		    {
			goto setup_transport_exit;
		    }
		}
	    }
        }

        //Pass the Link Characteristics to Packet Flow
        // This information will be later used by PacketFlow to set up
        // QoS Congestion Control module in the subsequent RegisterStream call.
        if (m_pLinkCharSetup)
        {
            if (m_ppStreams[i]->m_pLinkCharParams)
            {
                if (!SUCCEEDED(m_pLinkCharSetup->SetLinkCharParams(m_ppStreams[i]->m_pLinkCharParams)))
                {
                    goto setup_transport_exit;
                }
            }
        }

        IHXBuffer* pRawRuleBook = 0;

        pStreamInfo->m_pActualHeader->GetPropertyCString("ASMRuleBook",
                pRawRuleBook);

        if (pRawRuleBook)
        {
            pTransport = pProtocol->getTransport(this,
                                            pStreamInfo->m_stream_number,
                                            bIsReliable);

            if (!pTransport)
            {
                HX_RELEASE(pRawRuleBook);
                goto setup_transport_exit;
            }

            ASMRuleBook* pRuleBook;

            if (ppASMRuleBooks != NULL)
            {
                // this shouldn't be null if the header has a rulebook value
                HX_ASSERT(ppASMRuleBooks[pStreamInfo->m_stream_number] != NULL);

                pRuleBook = ppASMRuleBooks[pStreamInfo->m_stream_number];
            }
            else
            {
                pRuleBook = new ASMRuleBook((const char*)pRawRuleBook->GetBuffer());
            }

            /*
             * Handle subscriptions & unsubscriptions for PNA since
             * it can't do it by itself.
             */

            m_pSessionControl->RegisterStream(pTransport,
                    pStreamInfo->m_uStreamGroupNumber,
                    pStreamInfo->m_stream_number,
                    pRuleBook,
                    pStreamInfo->m_pActualHeader);

            HandleStreamRegistrationInfo(pStreamInfo);

            if (!m_bBlockTransfer)
            {
                m_pSessionControl->WantWouldBlock();
            }


            if (pMultiSessionControl)
            {
                Transport* pMultiTransport = 0;

                    pMultiTransport = pSession->
                        GetRTSPTransport(m_num_streams,
                        (Transport*)pTransport);

                pMultiSessionControl->RegisterStream(pMultiTransport,
                        pStreamInfo->m_stream_number,
                        pRuleBook,
                        pStreamInfo->m_pActualHeader);
            }

            /*
             * For RTSP/RDT Multicast subscribe to all rules on the datatypes
             * behalf.
             */

            if (pMultiSessionControl)
            {
                for (UINT16 n = 0; n < pRuleBook->GetNumRules(); n++)
                {
                    pMultiSessionControl->
                        HandleSubscribe(n, pStreamInfo->m_stream_number);
                }
            }

            // if the asm rulebooks parameter is null we must have allocated
            // this object ourselves and have to clean it up.
            if (ppASMRuleBooks == NULL)
            {
                HX_DELETE(pRuleBook);
            }
        }
        else
        {
            pTransport = pProtocol->getTransport(this,
                                            pStreamInfo->m_stream_number,
                                            0);
            m_pSessionControl->RegisterStream(pTransport,
                                            pStreamInfo->m_uStreamGroupNumber,
                                            pStreamInfo->m_stream_number,
                                            0,
                                            pStreamInfo->m_pActualHeader);
            HandleStreamRegistrationInfo(pStreamInfo);
            // Do this for PNA Flash and RTSP Datatypes with no rulebook.
            handle_subscribe(0, pStreamInfo->m_stream_number);

            if (!m_bBlockTransfer)
            {
                m_pSessionControl->WantWouldBlock();
            }


            if (pMultiSessionControl)
            {
                // XXXGo - at least it's not crashing...
                // need to actually get this to work.
                    Transport* pMultiTransport = NULL;

                    pMultiTransport = pSession->
                        GetRTSPTransport(m_num_streams,
                        (Transport*)pTransport);

                pMultiSessionControl->RegisterStream(pMultiTransport,
                        pStreamInfo->m_stream_number,
                        0,
                        pStreamInfo->m_pActualHeader);

                pMultiSessionControl->HandleSubscribe(0,
                    pStreamInfo->m_stream_number);
            }
        }

        /*
         * A transport may be used by more than one stream
         */

        UINT32 ulTransportID = 0;

        if (m_pClient->use_registry_for_stats()
        &&  !pTransport->IsInitialized()
        &&  m_pRegistryKey)
        {
            sprintf(str, "%s.Transport.%ld",
                         m_pRegistryKey, ulTransportCount++);
            ulTransportID = m_pProc->pc->registry->AddComp(str, m_pProc);
        }

        HX_ASSERT(m_pStats);

        pTransport->SetSessionStatsObj(m_pStats);
        pTransport->InitializeStatistics(ulTransportID);

        HX_RELEASE(pRawRuleBook);
    }

    m_bInitialized = TRUE;

    if (m_pClient->use_registry_for_stats())
    {
        sprintf(str, "client.%ld.IsMulticastUsed", m_pClient->get_registry_conn_id());
        m_pProc->pc->registry->AddInt(str, bIsMulticast ? 1 : 0, m_pProc);
    }

    HX_ASSERT(m_pStats);

    m_pStats->SetMulticastUsed(bIsMulticast);

    // Propagate Bandwidth to congestion control. It doesn't exist at DESCRIBE
    // time and needs to know this to determine proper oversend rate.
    RateSignal* pRateSignal = (RateSignal*)m_pBandwidthSignal->GetBuffer();
    pRateSignal->m_ulRate = m_ulClientAvgBandwidth;
    m_pSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                           HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                           HX_QOS_SIGNAL_COMMON_BANDWIDTH));
    m_pSignal->SetValue(m_pBandwidthSignal);
    m_pSignalBus->Send(m_pSignal);

    if (pMultiSessionControl)
    {
        pMultiSessionControl->Play();
        pMultiSessionControl->Release();
    }
    else
    {
        //
        // if we've already called begin but put off calling play
        // because we weren't completely setup, call play now
        //
        if (m_bBegun)
        {
            m_pSessionControl->Play();
        }
    }

    if (m_pRegistryKey)
    {
        sprintf(str, "%s.TransportCount", m_pRegistryKey);
        m_pProc->pc->registry->AddInt(str, ulTransportCount, m_pProc);
    }

setup_transport_exit:
    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::HandleStreamRegistrationInfo(StreamInfo* pStreamInfo)
{
    m_pRateSelInfo->SetInfo(RSI_STREAMGROUPID, 
                            pStreamInfo->m_stream_number,
                            pStreamInfo->m_uStreamGroupNumber);

    UINT32 ulTemp = 0;

    if (SUCCEEDED(pStreamInfo->m_pActualHeader->GetPropertyULONG32("AvgBitRate",
                                                                   ulTemp)))
    {
        m_pRateSelInfo->SetInfo(RSI_AVGBITRATE, 
                                pStreamInfo->m_stream_number,
                                ulTemp);
    }

    if (SUCCEEDED(pStreamInfo->m_pActualHeader->GetPropertyULONG32("BaseRule",
                                                                   ulTemp)))
    {
        m_pRateSelInfo->SetInfo(RSI_DEFAULT_RULE, 
                                pStreamInfo->m_stream_number,
                                ulTemp);
    }

    if (SUCCEEDED(pStreamInfo->m_pActualHeader->GetPropertyULONG32("TrackID",
                                                                   ulTemp)))
    {
        m_pRateSelInfo->SetInfo(RSI_TRACKID,
                                pStreamInfo->m_stream_number,
                                ulTemp);
    }

    // This is present and set to "1" for default alt-stream in 3gpp multirate file.
    if (SUCCEEDED(pStreamInfo->m_pActualHeader->GetPropertyULONG32("DefaultStream",
                                                                   ulTemp)))
    {
        m_pRateSelInfo->SetInfo(RSI_ISDEFAULTSTREAM,
                                pStreamInfo->m_stream_number,
                                ulTemp);  // TRUE
    }
}

void
Player::Session::SetStreamSequenceNumbers(UINT32 ulFromTS)
{
    // Entered
    ++m_ulFunctionsInUse;

    HXProtocol* pProtocol = m_pClient->protocol();
    int i;
    for (i = 0; i < m_num_streams; i++)
    {
        StreamInfo* pStreamInfo = m_ppStreams[i];

        if (!pStreamInfo->m_bSetupReceived)
        {
	    continue;
        }

        UINT16 seqNo = 0;
        m_pSessionControl->GetSequenceNumber(pStreamInfo->m_stream_number,
                                                seqNo);

        //XXXBAB - hack alert - should be able to enumerate the transports
        for(UINT32 reliability=0;reliability<2;++reliability)
        {
            Transport* pTransport = pProtocol->getTransport(this,
                                    pStreamInfo->m_stream_number,
                                    reliability);
            pTransport->setSequenceNumber(pStreamInfo->m_stream_number, seqNo);
            // Transport timestamps are set after the stream's actual start
            // position is determined by the PPM (see ::ResetSessionTimeline)
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::ResetSessionTimeline(UINT16 usStreamNumber,
                                      UINT32 ulTimelineStart,
                                      Transport* pTransport,
                                      BOOL bIsMcastTransport)
{
    /*
     * We need to set the initial timestamp for this streaming interval after
     * the stream is repositioned.  This timestamp is reported in the RTP-Info
     * header (converted to media timescale).
     */

    if(!bIsMcastTransport && m_pClient->m_protType == HXPROT_RTSP
       && pTransport)
    {
        pTransport->setTimeStamp(usStreamNumber, ulTimelineStart);
    }
}

Player::Session::StreamInfo::StreamInfo(UINT16 uStreamGroupNumber,
			UINT16 stream_number) : m_pLinkCharParams(NULL)
                                              , m_pStreamAdaptParams(NULL)
{
    m_pActualHeader             = 0;
    m_pConvertedHeader          = 0;
    m_waiting_for_packet        = FALSE;
    m_is_done                   = FALSE;
    m_packet_buf                = new CHXSimpleList();
    m_stream_number             = stream_number;
    m_uStreamGroupNumber        = uStreamGroupNumber;
    m_pRuleBookBuffer           = NULL;
    m_pRuleBook                 = NULL;
    m_pLinkCharParams           = NULL;
    m_pStreamAdaptParams        = NULL;

    m_bSetupReceived		= FALSE;
}

Player::Session::StreamInfo::~StreamInfo()
{
    CHXSimpleList::Iterator     i;
    IHXPacket*                  current_packet;

    if (m_pActualHeader)
    {
        m_pActualHeader->Release();
    }

    if (m_pConvertedHeader)
    {
        m_pConvertedHeader->Release();
    }

    for (i = m_packet_buf->Begin(); i != m_packet_buf->End(); ++i)
    {
        current_packet = (IHXPacket*) (*i);
        current_packet->Release();
    }

    if (m_pRuleBookBuffer)
    {
        m_pRuleBookBuffer->Release();
        m_pRuleBookBuffer = NULL;
    }

    if (m_pRuleBook)
    {
        delete m_pRuleBook;
        m_pRuleBook = NULL;
    }

        HX_DELETE(m_pLinkCharParams);

        HX_DELETE(m_pStreamAdaptParams);

    delete m_packet_buf;
}

HX_RESULT
Player::Session::FindSource(URL* pURL)
{
    // Entered
    ++m_ulFunctionsInUse;

    HX_RESULT theErr = HXR_OK;

    HX_ASSERT(!m_pSrcFinder);
    m_pSrcFinder = new BasicSourceFinder(m_pProc, this);
    if (!m_pSrcFinder)
    {
        theErr = HXR_OUTOFMEMORY;
    }

    if (HXR_OK == theErr)
    {
        m_pSrcFinder->AddRef();
        theErr = m_pSrcFinder->Init(this, m_pQoSConfig);
    }

    if (HXR_OK == theErr)
    {
        theErr = m_pSrcFinder->FindSource(m_url, m_pRequest);
        // continues in FindSourceDone()
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return theErr;
}

STDMETHODIMP
Player::Session::StatDone(HX_RESULT status,
    UINT32 ulSize, UINT32 ulCreationTime, UINT32 ulAccessTime,
    UINT32 ulModificationTime, UINT32 ulMode)
{
    if (m_bCleanedUp)
    {
        return HXR_OK;
    }

    if(status == HXR_OK)
    {
        m_ulLastModifiedTime = ulModificationTime;
        if ((long)m_ulLastModifiedTime < 0)
        {
            m_ulLastModifiedTime = 0;
        }

        if (m_pClient->use_registry_for_stats() && m_pRegistryKey)
        {
            char str[512];
            sprintf(str, "%s.FileSize", m_pRegistryKey);
            m_pProc->pc->registry->AddInt(str, ulSize, m_pProc);
        }
        m_pStats->SetFileSize(ulSize);

    }

    HX_ASSERT(m_pStats);

    m_pStats->SetFileSize(ulSize);
    return HXR_OK;
}

void Player::Session::SendAlert(StreamError err)
{
    // Entered
    ++m_ulFunctionsInUse;

    if (m_pSessionControl)
    {
        UINT16 n;
        for (n = 0; n < m_num_streams; n++)
        {
            if (m_ppStreams[n]->m_bSetupReceived)
            {
		m_pSessionControl->StreamDone(n, TRUE);
            }
        }
    }

    if ((m_pClient) && (m_pClient->protocol()))
    {
        m_pClient->protocol()->playDone(m_sessionID);
        m_pClient->protocol()->sendAlert(m_sessionID, err);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void Player::Session::SendAlert(IHXBuffer* pAlert)
{
    // Entered
    ++m_ulFunctionsInUse;

    if (m_pSessionControl)
    {
        UINT16 n;
        for (n = 0; n < m_num_streams; n++)
        {
            if (m_ppStreams[n]->m_bSetupReceived)
            {
		m_pSessionControl->StreamDone(n, TRUE);
            }
        }
    }

    if ((m_pClient) && (m_pClient->protocol()))
    {
        m_pClient->protocol()->playDone(m_sessionID);
        m_pClient->protocol()->sendAlert(m_sessionID, pAlert);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

/*
 * Given a stream group number, return the setup stream for that group.
 */
UINT16
Player::Session::GetSetupStream(UINT16 uStreamGroup)
{
    if (m_ppStreams)
    {
        for (UINT16 i = 0; i < m_num_streams; i++)
        {
            if (m_ppStreams[i] &&
                m_ppStreams[i]->m_uStreamGroupNumber == uStreamGroup &&
                m_ppStreams[i]->m_bSetupReceived)
            {
                return i;
            }
        }
    }

    return INVALID_STREAM_NUMBER;
}

//XXXDPL 'opt' is no longer used, remove from all objects which instantiate Player
Player::Player(Process* _proc, Client* c, int opt)
    : m_ulRefCount(0)
{
#ifdef PAULM_PLAYERTIMING
    g_PlayerTimer.Add(this);
#endif
    m_pSessions       = new CHXSimpleList;
    m_pProc           = _proc;
    m_pClient         = c;
    m_pPacketFlowWrap         = 0;

    if(m_ulNextSessionID == 0)
    {
        srand((unsigned)time(NULL));
        m_ulNextSessionID = rand();
    }
}

Player::~Player()
{
#ifdef PAULM_PLAYERTIMING
    g_PlayerTimer.Remove(this);
#endif
    DPRINTF(0x10000000, ("Player::~Player() deleted\n"));
}

STDMETHODIMP_(ULONG32)
Player::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
Player::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

void
Player::Done(HX_RESULT status)
{
    clear_session_list(status);
    HX_DELETE(m_pSessions);
    delete m_pPacketFlowWrap;
}

void
Player::clear_session_list(HX_RESULT status)
{
    CHXSimpleList::Iterator i;
    for(i=m_pSessions->Begin();i!=m_pSessions->End();++i)
    {
        Player::Session* pSession = (Player::Session*)(*i);

        pSession->Done(status);
        pSession->Release();
    }
    m_pSessions->RemoveAll();
}


Player::Session*
Player::FindSession(const char* pSessionID)
{
    CHXSimpleList::Iterator i;

    if (pSessionID)
    {
        for(i=m_pSessions->Begin();i!=m_pSessions->End();++i)
        {
            Player::Session* pSession = (Player::Session*)(*i);
            if(pSession->m_sessionID == pSessionID)
            {
                return pSession;
            }
        }
    }
    return 0;
}

HX_RESULT
Player::RemoveSession(const char* pSessionID, HX_RESULT status)
{
    HX_ASSERT(m_pSessions != NULL);
    if (m_pSessions != NULL)
    {
        LISTPOSITION pos = m_pSessions->GetHeadPosition();
        while(pos)
        {
            Player::Session* pSession = (Player::Session*)m_pSessions->GetAt(pos);
            if(pSession->m_sessionID == pSessionID)
            {
                pos = m_pSessions->RemoveAt(pos);
                pSession->Done(status);
                m_pProc->pc->server_info->DecrementStreamCount(m_pProc);
                pSession->Release();
            }
            else
            {
                m_pSessions->GetNext(pos);
            }
        }
    }
    return HXR_OK;
}

/* XXXTDM: get rid of this and NewSessionWithID().
 * These are used for the Pragma: initiate-session, to allow an RTSP session
 * to be created without a corresponding Player::Session.  Allowing this has
 * created problems (see comments in rtspserv.cpp).  The two session
 * objects should have a 1:1 correspondence.
 */
HX_RESULT
Player::GenerateNewSessionID(CHXString& sessionID, UINT32 ulSeqNo)
{
    char tmp[64];
    sprintf(tmp, "%ld-%ld", ++m_ulNextSessionID, ulSeqNo);
    sessionID = tmp;
    return HXR_OK;
}

HX_RESULT
Player::NewSession(Session** ppSession,
                         UINT32 uSeqNo,
                         BOOL bRetainEntityForSetup)
{
    CHXString sessionID;
    GenerateNewSessionID(sessionID, uSeqNo);

    *ppSession = new Session(m_pProc, m_pClient, this, sessionID,
                             bRetainEntityForSetup);
    m_pSessions->AddHead(*ppSession);

    // AddRef for both list and out parameter.
    (*ppSession)->AddRef();
    (*ppSession)->AddRef();

    /*
     * this solves the problem of the subscriber counting http connections,
     * which was solved previously by counting only pna and rtsp clients.
     * since only pnaprot and rtspprot call NewSession() and/or
     * NewSessioWithID() the subscriber can rely on the 'server.streamCount'
     * var for the same info.
     */
    m_pProc->pc->server_info->IncrementStreamCount(m_pProc);

    return HXR_OK;
}

HX_RESULT
Player::NewSessionWithID(Session** ppSession,
                               UINT32 uSeqNo, const char* pSessionId,
                               BOOL bRetainEntityForSetup)
{
    *ppSession = new Session(m_pProc, m_pClient, this, pSessionId,
                             bRetainEntityForSetup);
    m_pSessions->AddHead(*ppSession);

    // AddRef for both list and out parameter.
    (*ppSession)->AddRef();
    (*ppSession)->AddRef();

    /*
     * this solves the problem of the subscriber counting http connections,
     * which was solved previously by counting only pna and rtsp clients.
     * since only pnaprot and rtspprot call NewSession() and/or
     * NewSessioWithID() the subscriber can rely on the 'server.streamCount'
     * var for the same info.
     */
    m_pProc->pc->server_info->IncrementStreamCount(m_pProc);

    return HXR_OK;
}

void
Player::SetStreamStartTime(const char* szSessionID, UINT32 ulStreamNum,
                           UINT32 ulTimestamp)
{
    Player::Session* pSession = FindSession(szSessionID);
    if (pSession)
    {
        pSession->SetStreamStartTime(ulStreamNum, ulTimestamp);
    }
}

HX_RESULT
Player::HandleDefaultSubscription(const char* szSessionID)
{
    Player::Session* pSession = FindSession(szSessionID);
    if (pSession)
    {
        return pSession->HandleDefaultSubscription();
    }
    return HXR_UNEXPECTED;
}

//
//  Allowance Plugins - Putting Allow at the Head of m_pAllowanceMgrs list
//
void
Player::Session::InitAllowancePlugins()
{
    // Entered
    ++m_ulFunctionsInUse;

    IUnknown*                       pUnknown         = NULL;
    IHXPlugin*                      pPluginInterface = NULL;
    IHXPlayerConnectionAdviseSink* pAdviseSink       = NULL;
    AllowanceMgr*                   pManager         = NULL;
    CHXSimpleList*                  pPluginInfoList  = NULL;
    BOOL                            bAllowFound      = FALSE;
    BOOL                            bAllow           = FALSE;

    // Get the Debug settings for Allowance Optimizations
    m_bAllowanceDebug = m_pProc->pc->plugin_handler->m_allowance_handler->m_bDebug;

    // Set up a list of Allowance Plugin Managers
    pPluginInfoList = m_pProc->pc->plugin_handler->m_allowance_handler->m_pPluginInfoList;

    LISTPOSITION pos = pPluginInfoList->GetHeadPosition();
    while(pos)
    {
        pUnknown = NULL;
        pAdviseSink = NULL;
        pPluginInterface = NULL;
        bAllow = FALSE;

        // Get next plugin
        PluginHandler::AllowancePlugins::PluginInfo* pAllowPluginInfo =
            (PluginHandler::AllowancePlugins::PluginInfo*)pPluginInfoList->GetAt(pos);
        PluginHandler::Plugin* pPlugin =
            (PluginHandler::Plugin*)(pAllowPluginInfo->m_pPlugin);

        // see if this is allow!
        if (!bAllowFound && pPlugin->m_pszDescription)
        {
            // this hard coded string has to be the same as
            // CBasicAllowance::zm_pDescription in allow/allow.cpp
            if (strcmp(pPlugin->m_pszDescription,
                       "RealNetworks Basic Allowance Plugin") == 0)
            {
                bAllowFound = TRUE;
                bAllow      = TRUE;
            }
        }

        if (pPlugin->m_load_multiple                  &&
            HXR_OK == pPlugin->GetInstance(&pUnknown) &&
            pUnknown)
        {
            // Query for the IHXPlayerConnectionAdviseSink interface
            if ((pUnknown->QueryInterface(IID_IHXPlayerConnectionAdviseSink,
                (void**) &pAdviseSink) == HXR_OK) && pAdviseSink)
            {
                // Initialize the plugin
                if ((HXR_OK == pAdviseSink->QueryInterface(IID_IHXPlugin,
                    (void**) &pPluginInterface)) && pPluginInterface)
                {
                    HX_RESULT hresult;

                    hresult = pPluginInterface->InitPlugin(m_pProc->pc->server_context);

                    HX_RELEASE(pPluginInterface);

                    if (HXR_OK != hresult)
                    {
                        HX_RELEASE(pAdviseSink);
                        HX_RELEASE(pUnknown);
                        pPluginInfoList->GetNext(pos);
                        continue;
                    }
                }

                // Create an Allowance Manager, and add it to our list
                pManager = new MLAllowanceMgr(m_pProc, this, pAdviseSink);
                HX_ASSERT(pManager);
                pManager->AddRef();
                if (!bAllow)
                {
                    m_pAllowanceMgrs->AddTail(pManager);
                }
                else
                {
                    m_pAllowanceMgrs->AddHead(pManager);
                }

                // Signal the plugin that a new player has connected
                ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pManager
                            , ("Calling OnConnection()\n"));
                pManager->OnConnection(NULL);

                HX_RELEASE(pAdviseSink);
                HX_RELEASE(pUnknown);
            }
        }
        else if (!pPlugin->m_load_multiple)
        {
            AllowancePlugin* pAllowancePlugin;

            if (!m_pProc->pc->allowance_plugins->Lookup(pPlugin,
                                                     (void*&)pAllowancePlugin))
            {
                pPluginInfoList->GetNext(pos);
                continue;
            }

            Process* pProc = pAllowancePlugin->m_pProc;

            AllowanceWrapper* pAllowanceWrapper =
                new AllowanceWrapper(this,
                                     m_pProc,
                                     pProc,
                                     (const char*)pAllowPluginInfo->m_szFileSysMountPoint,
                                     pAllowancePlugin->m_pAdviseSinkManager);

            pAllowanceWrapper->AddRef();

            if (!bAllow)
            {
                m_pAllowanceMgrs->AddTail(pAllowanceWrapper);
            }
            else
            {
                m_pAllowanceMgrs->AddHead(pAllowanceWrapper);
            }

            // Signal the plugin that a new player has connected
            ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pAllowanceWrapper
                            , ("Calling OnConnection()\n"));
            pAllowanceWrapper->OnConnection(NULL);
        }
        else
        {
            DPRINTF(D_ERROR, ("Allowance plugin error\n"));
        }

        pPluginInfoList->GetNext(pos);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::SetRegistryID(UINT32 ulPlayerRegID)
{
    // Entered
    ++m_ulFunctionsInUse;

    // Loop through the list of Allowance Managers
    if (m_pAllowanceMgrs)
    {
        AllowanceMgr* pManager = NULL;

        LISTPOSITION pos = m_pAllowanceMgrs->GetHeadPosition();
        while(pos)
        {
            // Get next Manager
            pManager = (AllowanceMgr*)m_pAllowanceMgrs->GetAt(pos);

            if (pManager)
            {
                ALLOWANCE_DEBUG_MSG(m_bAllowanceDebug, pManager
                    , ("Calling SetRegistryID()\n"));

                pManager->SetRegistryID(ulPlayerRegID);
            }

            m_pAllowanceMgrs->GetNext(pos);
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::OnURL(ServerRequest* pRequest)
{
    // Entered
    ++m_ulFunctionsInUse;

    if (!m_pAllowanceMgrs || m_pAllowanceMgrs->GetCount() == 0)
    {
#ifndef HELIX_FEATURE_ALLOW_REQUIRED
        if (!m_bDone)
        {
            FindSource(m_url);
        }
#else
        if (m_pClient->m_bIsAProxy)
        {
            if (!m_bDone)
            {
                FindSource(m_url);
            }
        }
        else
        {
            HX_ASSERT(0);
        }
#endif /* HELIX_FEATURE_ALLOW_REQUIRED */
    }
    else
    {
        PluginHandler::AllowancePlugins::PluginInfo*    pMountPointBasedPluginInfo = NULL;
        PluginHandler::Errors                           plugin_error = PluginHandler::NO_ERRORS;
        PluginHandler::AllowancePlugins*                pAllowanceHandler = NULL;

        const char*     pURL = NULL;

        //Check if URL would end up being processed by a
        // Mount-Point based Non-Multiload plugin. The check
        // is done only if Client Startup optimization is
        // enabled.
        pAllowanceHandler = m_pProc->pc->plugin_handler->m_allowance_handler;
        if (pAllowanceHandler->m_bClientStartupOptimization
                        && SUCCEEDED(pRequest->GetURL(pURL)))
        {
            INT32 unMtPointLen = 0;
            const char* pszBestPlugMountPoint =  NULL;

            ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, (AllowanceMgr*)NULL
                , ("AO: SessionID: %s -- -- Player::Session::OnURL() Processing URL: \"%s\"\n",
                (const char *)m_sessionID, pURL)
            );

            //Find the best Non-Multiload plugin for the requested URL
            plugin_error = pAllowanceHandler->FindPluginFromMountPoint(pURL
                                                        , pMountPointBasedPluginInfo);
            if (plugin_error == PluginHandler::NO_ERRORS && pMountPointBasedPluginInfo)
            {
                unMtPointLen =  pMountPointBasedPluginInfo->m_szFileSysMountPoint.GetLength();
                pszBestPlugMountPoint = (const char *)(pMountPointBasedPluginInfo->m_szFileSysMountPoint);

                ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, (AllowanceMgr*)NULL
                    , ("AO: SessionID: %s -- -- Player::Session::OnURL() Processing URL: \"%s\", MountPoint: %s\n"
                    , (const char *)m_sessionID, pURL, pszBestPlugMountPoint)
                );
            }

            //Delete Non-Multiload plugins that have a MountPoint set which does not match
            // the MountPoint of requested URL
            //
            //Please note that all Non-Multiload plugins with a non-null MountPoint value are deleted
            //  if Allowance Handler does not return a plugin that matches MountPoint of the requested URL
            if (m_pAllowanceMgrs)
            {
                AllowanceMgr* pManager = NULL;

                LISTPOSITION pos = m_pAllowanceMgrs->GetHeadPosition();
                LISTPOSITION last_pos = pos;
                while(pos)
                {
                    // Get next Manager
                    pManager = (AllowanceMgr*)m_pAllowanceMgrs->GetAt(pos);

                    last_pos = pos;
                    m_pAllowanceMgrs->GetNext(pos);

                    //Remove Non-Multiload Plugins that have a non-null MountPoint
                    //  that does not match the MountPoint of requested URL
                    //
                    if (!pManager->AcceptMountPoint(pszBestPlugMountPoint, unMtPointLen))
                    {
                        ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pManager
                                          , ("Player::Session::OnURL() Deleting Plugin From List\n"));

                        if (pManager->IsActive())
                        {
                            ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pManager
                                , ("Calling OnDone()\n"));
                            pManager->AllowanceMgrDone();
                        }
                        HX_RELEASE(pManager);
                        m_pAllowanceMgrs->RemoveAt(last_pos);
                    }
                }
            }
        }

        HX_RELEASE(m_pAllowanceSerializer);
        m_pAllowanceSerializer = new AllowanceSerializer(m_bAllowanceDebug);
        m_pAllowanceSerializer->AddRef();

        // Make sure OnUrlDone callback can work.
        AddRef();

#if XXXAAK_DISTLIC_ONE_LICENSE_PER_CONN
        ServerRequestWrapper* wrapper =
            new ServerRequestWrapper(FS_HEADERS, pRequest);
        wrapper->AddRef();
        IHXValues* pValues = 0;
        HX_RESULT ret = HXR_OK;
        ((IHXRequest *)wrapper)->GetResponseHeaders(pValues);

        if (pValues == NULL)
        {
            ret = m_pProc->pc->common_class_factory->CreateInstance(
                CLSID_IHXValues, (void**)&pValues);
        }
        if (HXR_OK == ret)
        {
            pValues->SetPropertyULONG32("CID",
                (ULONG32)m_pClient->id());
                ((IHXRequest *)wrapper)->SetResponseHeaders(pValues);
        }
        HX_RELEASE(pValues);
        HX_RELEASE(wrapper);
#endif

        m_pAllowanceSerializer->OnURL(pRequest, m_pAllowanceMgrs);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

HX_RESULT
Player::Session::OnURLDone(HX_RESULT status)
{
    // Entered
    ++m_ulFunctionsInUse;

    AllowanceMgr* pMgr = NULL;

    if (!m_bDone)
    {
        AllowanceSerializer::AllowanceStatus OverallStatus;
        pMgr = m_pAllowanceSerializer->OnURLDone(status, OverallStatus);

        switch (OverallStatus)
        {
        case AllowanceSerializer::more_plugins_withoutcurrent:
            if (pMgr != NULL)
            {
                ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pMgr
                    , ("Calling OnDone()\n"));
                pMgr->AllowanceMgrDone();
                pMgr->SetIsActive(FALSE);
            }

        case AllowanceSerializer::more_plugins:
            AddRef();  // make sure we're still around for callback
            m_pAllowanceSerializer->CallNextOne();
            break;

        case AllowanceSerializer::allow_playback_withoutcurrent:
            if (pMgr != NULL)
            {
                ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pMgr
                    , ("Calling OnDone()\n"));
                pMgr->AllowanceMgrDone();
                pMgr->SetIsActive(FALSE);
            }

        case AllowanceSerializer::allow_playback:
            FindSource(m_url);
            break;

        case AllowanceSerializer::reject_playback:
            HX_ASSERT(status != HXR_OK);
            FindDone(status, 0);
            break;
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    // We AddRef'ed ourselves to make sure we got here okay.  We got
    // here okay, so Release.
    Release();

    return HXR_OK;
}

void
Player::Session::OnBegin()
{
    // Entered
    ++m_ulFunctionsInUse;

    // Loop through the list of Allowance Managers
    if (m_pAllowanceMgrs)
    {
        AllowanceMgr* pManager = NULL;

        LISTPOSITION pos = m_pAllowanceMgrs->GetHeadPosition();
        while(pos)
        {
            // Get next Manager
            pManager = (AllowanceMgr*)m_pAllowanceMgrs->GetAt(pos);

            if (pManager && pManager->IsActive())
            {
                ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pManager, ("Calling OnBegin()\n"));
                pManager->OnBegin();
            }

            m_pAllowanceMgrs->GetNext(pos);
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::OnPause()
{
    // Entered
    ++m_ulFunctionsInUse;

    // Loop through the list of Allowance Managers
    if (m_pAllowanceMgrs)
    {
        AllowanceMgr* pManager = NULL;

        LISTPOSITION pos = m_pAllowanceMgrs->GetHeadPosition();
        while(pos)
        {
            // Get next Manager
            pManager = (AllowanceMgr*)m_pAllowanceMgrs->GetAt(pos);

            if (pManager && pManager->IsActive())
            {
                ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pManager, ("Calling OnPause()\n"));
                pManager->OnPause();
            }

            m_pAllowanceMgrs->GetNext(pos);
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

void
Player::Session::OnStop()
{
    // Entered
    ++m_ulFunctionsInUse;

    // Loop through the list of Allowance Managers
    if (m_pAllowanceMgrs)
    {
        AllowanceMgr* pManager = NULL;

        LISTPOSITION pos = m_pAllowanceMgrs->GetHeadPosition();
        while(pos)
        {
            // Get next Manager
            pManager = (AllowanceMgr*)m_pAllowanceMgrs->GetAt(pos);

            if (pManager && pManager->IsActive())
            {
                ALLOWANCE_DEBUG_MSG( m_bAllowanceDebug, pManager, ("Calling OnStop()\n"));
                pManager->OnStop();
            }

            m_pAllowanceMgrs->GetNext(pos);
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
}

HX_RESULT
Player::Session::HandleStreamAdaptation( REF(const StreamAdaptationParams) streamAdapt,
                                                BOOL bHlxStreamAdaptScheme)
{
    HX_RESULT rc = HXR_OK;

    // Stream Adaptation not possible if Source for the content does not support
    //  Uber stream management.
    if (!m_pUberStreamMgr)
    {
	rc = HXR_FAIL;
    }

    // Handle Helix Adaptation -- exclude-rules, stream-switch params
    if (bHlxStreamAdaptScheme && SUCCEEDED(rc))
    {
        // Get the logical stream manager
        IHXRateDescEnumerator* pLogicalStream = NULL;
        if (SUCCEEDED(rc))
            rc = m_pUberStreamMgr->GetLogicalStream(streamAdapt.m_unStreamNum, pLogicalStream);

        if (streamAdapt.m_bStreamSwitch)
        {
	    // Handle exclude-rules
	    // (only necessary if stream-switch is allowed) -- ignore invalid rules
            for (UINT32 j=0; j<streamAdapt.m_ulNumExcludedRules; j++)
            {
                IHXRateDescription* pRateDesc = NULL;
                HX_RESULT resExcludeRule = pLogicalStream->FindRateDescByRule(streamAdapt.m_pExcludeRules[j], FALSE, FALSE, pRateDesc);

                if (SUCCEEDED(resExcludeRule))
                    resExcludeRule = pRateDesc->ExcludeFromSwitching(TRUE, HX_SWI_RATE_EXCLUDED_BY_CLIENT);

                HX_ASSERT(SUCCEEDED(resExcludeRule));
                HX_RELEASE(pRateDesc);
            }
        }
        else
        {
            // Handle exclude-rules
            // all of them excluded from switching
            for (UINT32 i=0; SUCCEEDED(rc) && i<pLogicalStream->GetNumRateDescriptions(); i++)
            {
                IHXRateDescription* pRateDesc = NULL;
                rc = pLogicalStream->GetRateDescription(i, pRateDesc);

                if (SUCCEEDED(rc))
                {
                    rc = pRateDesc->ExcludeFromSwitching(TRUE, HX_SWI_RATE_EXCLUDED_BY_CLIENT);
                }

                HX_RELEASE(pRateDesc);
            }
        }

        HX_RELEASE(pLogicalStream);
    }

    if (SUCCEEDED(rc))
    {
        if (m_bInitialized && m_ppStreams[streamAdapt.m_unStreamNum])   //SETUP done, this indicates updates
        {
            // Only Target Protection time updates are accepted, buffer size cannot be
            m_ppStreams[streamAdapt.m_unStreamNum]->m_pStreamAdaptParams->m_ulTargetProtectionTime
                = streamAdapt.m_ulTargetProtectionTime;

            //* Helix-Adaptation maintains Aggregate adaptation params.
            //*
            //* This requires that we run through the Stream Level params to figure out the max
            //*   and process target-time updates only if the new one ends up changing the
            //*   target-time value to be used for aggregate adaptation
            StreamAdaptationParams* pStreamAdaptParams = 0;
            if (bHlxStreamAdaptScheme)
            {
                UINT32 ulMaxTargetTime = 0;
                UINT32 ulTemp = 0;
                for (UINT32 i = 0; i < m_num_streams; i++)
                {
                    if (m_ppStreams[i]->m_bSetupReceived)
                    {
                        ulTemp = m_ppStreams[i]->m_pStreamAdaptParams->m_ulTargetProtectionTime;
                        if (ulTemp > ulMaxTargetTime)
                        {
                            ulMaxTargetTime = ulTemp;
                        }
                    }
                }

                if (ulMaxTargetTime != m_pAggRateAdaptParams->m_ulTargetProtectionTime)
                {
                    m_pAggRateAdaptParams->m_ulTargetProtectionTime = ulMaxTargetTime;
                    pStreamAdaptParams = m_pAggRateAdaptParams;
                }
            }
            else
            {
                pStreamAdaptParams = m_ppStreams[streamAdapt.m_unStreamNum]->m_pStreamAdaptParams;
            }

            //* for aggregate adaptation if the target-time update does not change the maximum
            //*   pStreamAdaptParams will be null
            if (pStreamAdaptParams)
            {
                IHXBuffer* pStreamAdaptDataBuff;
                if ( SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                               (void **)&pStreamAdaptDataBuff)))
                {
                    pStreamAdaptDataBuff->SetSize(sizeof(StreamAdaptSignalData));

                    StreamAdaptSignalData *streamAdaptData = (StreamAdaptSignalData *)
                                                               (pStreamAdaptDataBuff->GetBuffer());
                    *streamAdaptData = *pStreamAdaptParams;

                    m_pSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_SESSION,
                                                           HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                           HX_QOS_SIGNAL_COMMON_STREAM_ADAPT_HDR));
                    m_pSignal->SetValue(pStreamAdaptDataBuff);
                    rc = m_pSignalBus->Send(m_pSignal);
                    HX_RELEASE(pStreamAdaptDataBuff);
                }
            }
            else
            {
                rc = HXR_FAIL;
            }

        }
        else if (!m_bInitialized)
        {
            if (bHlxStreamAdaptScheme)
            {
                m_bHlxStreamAdaptScheme = TRUE;

                if (!m_pAggRateAdaptParams)
                {
                    m_pAggRateAdaptParams = new StreamAdaptationParams();
                }

                //* Helix-Adaptation requires Aggregate buffer management
                //*
                //* The Stream Adaptation parameters signalled on a stream level
                //*  are aggregated to support Helix-Adaptation
                //*
                //* agg. BufferSize = sum of sizes for all streams
                //* agg. target-time = max target-time
                m_pAggRateAdaptParams->m_ulClientBufferSize += streamAdapt.m_ulClientBufferSize;
                if (m_pAggRateAdaptParams->m_ulTargetProtectionTime
                    < streamAdapt.m_ulTargetProtectionTime)
                {
                    m_pAggRateAdaptParams->m_ulTargetProtectionTime
                                                = streamAdapt.m_ulTargetProtectionTime;
                }
            }

            HX_ASSERT(streamAdapt.m_unStreamNum <= (m_num_streams - 1));
            if (streamAdapt.m_unStreamNum <= (m_num_streams - 1))
            {
                m_unStreamAdaptSetupCount++;
                m_ppStreams[streamAdapt.m_unStreamNum]->m_pStreamAdaptParams
                    = new StreamAdaptationParams;
                *(m_ppStreams[streamAdapt.m_unStreamNum]->m_pStreamAdaptParams)
                    = streamAdapt;

                //Set session to use MDP if client supports Stream Adaptation
                if (!m_pSourceControl->IsLive())
                {
                m_bUseMediaDeliveryPipeline = TRUE;
            }
            }
            else
            {
                //VS: INCOMPLETE
                //Log error message indicating bad streamid in url param of stream adapt header
                rc = HXR_FAIL;
            }
        }
        else
        {
            rc = HXR_FAIL;
        }
    }

    return rc;
}

/**
 * \brief Adjust 3GPP-Link-Char b/w measurement units.
 *
 * This method multiplies the b/w units by an amount specified in the UserAgentProfile
 * section of the MDP config. Expected values for the multiplier are 1 (for b/w measured
 * in bps) and 1000 (for b/w measured in kbps).
 *
 * \params RawLinkCharParams LinkCharParams object containing raw b/w values.
 *
 * \return LinkCharParams object containing adjusted b/w values.
 *
 */

LinkCharParams
Player::Session::Adjust3GPPLinkCharUnits(REF(const LinkCharParams) RawLinkCharParams)
{
    if (!m_pQoSConfig)
    {
        return RawLinkCharParams;
    }

    INT32 nMult = 0;
    const INT32 DEFAULT_LINKCHAR_MULTIPLIER = 1000;

    // Really the only two values we expect are 1 and 1000, but just in case...
    const INT32 MAX_LINKCHAR_MULTIPLIER = 1000;
    const INT32 MIN_LINKCHAR_MULTIPLIER = 1;

    LinkCharParams AdjustedParams = RawLinkCharParams;


    if (FAILED(m_pQoSConfig->GetConfigInt(QOS_CFG_LINKCHAR_MULTIPLIER, nMult)))
    {
        nMult = DEFAULT_LINKCHAR_MULTIPLIER;
    }

    nMult = (nMult > MAX_LINKCHAR_MULTIPLIER) ? MAX_LINKCHAR_MULTIPLIER : nMult;
    nMult = (nMult < MIN_LINKCHAR_MULTIPLIER) ? MIN_LINKCHAR_MULTIPLIER : nMult;

    AdjustedParams.m_ulMaxBW *= nMult;
    AdjustedParams.m_ulGuaranteedBW *= nMult;

    return AdjustedParams;
}


HX_RESULT
Player::Session::Handle3GPPLinkChar(REF(const LinkCharParams) RawLinkCharParams)
{
    HX_RESULT rc = HXR_FAIL;

    LinkCharParams linkCharParams = Adjust3GPPLinkCharUnits(RawLinkCharParams);

    if (m_bInitialized)
    {
        //SETUP done, this call is either a session aggregate
        // 3GPP-Link-Char received in PLAY or updates
        // received through OPTIONS or SET_PARAMETER
        //
        //Link Char updates override all previously set
        // parameters
        //
        //XXXVS: TODO
        //Session Aggregate Link-Char's are sent no earlier
        //than PLAY this causes a problem if MDP is not setup
        //by this time.

        //Aggregate Link-Char will not be processed if Link-Char set on a Stream Level
        if (linkCharParams.m_bSessionAggregate)
        {
            if (m_unLinkCharSetupCount == 0)
            {
                if (!m_pSessionAggrLinkCharParams)
                {
                    m_pSessionAggrLinkCharParams = new LinkCharParams;
                }

                *m_pSessionAggrLinkCharParams = linkCharParams;
            }
            m_pRateSelInfo->SetInfo(RSI_LINKCHAR_MBW, linkCharParams.m_ulMaxBW);
            m_pRateSelInfo->SetInfo(RSI_LINKCHAR_GBW, linkCharParams.m_ulGuaranteedBW);
            m_pRateSelInfo->SetInfo(RSI_LINKCHAR_MTD, linkCharParams.m_ulMaxTransferDelay);
        }
        else
        {
            //Stream Level Link char are updates
            if (m_ppStreams[linkCharParams.m_unStreamNum]->m_pLinkCharParams)
            {
                *m_ppStreams[linkCharParams.m_unStreamNum]->m_pLinkCharParams
                                = linkCharParams;

                m_pRateSelInfo->SetInfo(RSI_LINKCHAR_MBW, 
                                        linkCharParams.m_unStreamNum, 
                                        linkCharParams.m_ulMaxBW);
                m_pRateSelInfo->SetInfo(RSI_LINKCHAR_GBW, 
                                        linkCharParams.m_unStreamNum, 
                                        linkCharParams.m_ulGuaranteedBW);
                m_pRateSelInfo->SetInfo(RSI_LINKCHAR_MTD, 
                                        linkCharParams.m_unStreamNum, 
                                        linkCharParams.m_ulMaxTransferDelay);
            }
            else
            {
                return HXR_FAIL;
            }
        }

        if(m_pLinkCharSetup)
        {
            rc = m_pLinkCharSetup->SetLinkCharParams(&linkCharParams);
        }

        IHXBuffer* pLinkCharSigDataBuff;
        if ( SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                                        (void **)&pLinkCharSigDataBuff)))
        {
            pLinkCharSigDataBuff->SetSize(sizeof(LinkCharSignalData));
            LinkCharSignalData *linkCharSigData =
                                (LinkCharSignalData *)(pLinkCharSigDataBuff->GetBuffer());

            *linkCharSigData = linkCharParams;

            m_pSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                HX_QOS_SIGNAL_COMMON_LINK_CHAR_HDR));
            m_pSignal->SetValue(pLinkCharSigDataBuff);
            rc = m_pSignalBus->Send(m_pSignal);
            HX_RELEASE(pLinkCharSigDataBuff);
        }
    }
    else if (!m_bInitialized && !linkCharParams.m_bSessionAggregate)
    {
        //Being SETUP, Only Stream Level Link characteristics can be set

        HX_ASSERT(linkCharParams.m_unStreamNum <= (m_num_streams - 1));
        if (linkCharParams.m_unStreamNum <= (m_num_streams - 1))
        {
            m_unLinkCharSetupCount++;
            m_ppStreams[linkCharParams.m_unStreamNum]->m_pLinkCharParams
                                        = new LinkCharParams;
            *(m_ppStreams[linkCharParams.m_unStreamNum]->m_pLinkCharParams)
                                        = linkCharParams;

            m_pRateSelInfo->SetInfo(RSI_LINKCHAR_MBW, 
                                    linkCharParams.m_unStreamNum, 
                                    linkCharParams.m_ulMaxBW);
            m_pRateSelInfo->SetInfo(RSI_LINKCHAR_GBW, 
                                    linkCharParams.m_unStreamNum, 
                                    linkCharParams.m_ulGuaranteedBW);
            m_pRateSelInfo->SetInfo(RSI_LINKCHAR_MTD, 
                                    linkCharParams.m_unStreamNum, 
                                    linkCharParams.m_ulMaxTransferDelay);

            //Set session to use MDP if client provides Link Characteristics
            if (!m_pSourceControl->IsLive())
            {
                m_bUseMediaDeliveryPipeline = TRUE;
            }

            rc = HXR_OK;
        }
    }

    return rc;
}

HX_RESULT
Player::Session::HandleStreamSetup(UINT16 uStreamNumber,
				   UINT16 uStreamGroupNumber)
{
    HX_RESULT hr = HXR_OK;

    HX_ASSERT(uStreamNumber ==  m_ppStreams[uStreamNumber]->m_stream_number);
    HX_ASSERT(uStreamGroupNumber ==
	    m_ppStreams[uStreamNumber]->m_uStreamGroupNumber);

    m_ppStreams[uStreamNumber]->m_bSetupReceived = TRUE;
    m_uStreamSetupCount++;

    //* Maintain maximum Preroll amongst streams that have been setup
    //*   MaxPreRoll is used by MDP buffer model for supporting
    //*   aggregate stream adaptation (Helix-Adaptation).
    UINT32 ulStreamPreRoll = 0;
    m_ppStreams[uStreamNumber]->m_pActualHeader->GetPropertyULONG32("ServerPreroll", ulStreamPreRoll);
    if (ulStreamPreRoll > m_ulMaxPreRoll)
    {
	m_ulMaxPreRoll = ulStreamPreRoll;
    }

    return hr;
}

HX_RESULT
Player::Session::HandleClientAvgBandwidth(UINT32 ulClientAvgBandwidth)
{
    HX_RESULT res = HXR_OK;

    // Pass avg bandwidth info to inputsource -- for stream selection
    m_ulClientAvgBandwidth = ulClientAvgBandwidth;
    if (m_pUberStreamConfig)
        res = m_pUberStreamConfig->SetClientAverageBandwidth(ulClientAvgBandwidth);

    RateSignal* pRateSignal = (RateSignal*)m_pBandwidthSignal->GetBuffer();
    pRateSignal->m_ulRate = m_ulClientAvgBandwidth;
    m_pSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                           HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                           HX_QOS_SIGNAL_COMMON_BANDWIDTH));
    m_pSignal->SetValue(m_pBandwidthSignal);
    m_pSignalBus->Send(m_pSignal);

    if (m_pSessionControl)
    {
        m_pSessionControl->SetBandwidth(ulClientAvgBandwidth);
    }

    m_pRateSelInfo->SetInfo(RSI_BANDWIDTH, ulClientAvgBandwidth);

    return res;
}

HX_RESULT
Player::Session::SetSessionStreamAdaptScheme()
{
    if (m_bHlxStreamAdaptScheme)
    {
        m_StreamAdaptationScheme = ADAPTATION_HLX_AGGR;
    }
    else
    {
        if (m_unStreamAdaptSetupCount)
        {
            m_StreamAdaptationScheme = ADAPTATION_REL6_PER_STREAM;
        }
        else
        {
            m_StreamAdaptationScheme = ADAPTATION_NONE;
        }
    }

    return HXR_OK;
}


//
//  IHXPlayerController interface
//
STDMETHODIMP
Player::Session::Pause()
{
    // Entered
    ++m_ulFunctionsInUse;

    if (!m_bPaused && m_pSessionControl)
    {
        m_pSessionControl->Pause(0);
        m_bPaused = TRUE;
        SetInactive();
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

STDMETHODIMP
Player::Session::Resume()
{
    // Entered
    ++m_ulFunctionsInUse;

    if (m_bPaused && m_pSessionControl)
    {
        m_pSessionControl->Play();
        m_bPaused = FALSE;
        ClearInactive();
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

STDMETHODIMP
Player::Session::Disconnect()
{
    // Entered
    ++m_ulFunctionsInUse;

    DPRINTF(0x48000000, ("%lu: P::S::Disconnect() is killing me!\n",
                m_pClient->conn_id));

    if (m_pSessionControl)
    {
        UINT16 n;
        for (n = 0; n < m_num_streams; n++)
        {
            if (m_ppStreams[n]->m_bSetupReceived)
            {
		m_pSessionControl->StreamDone(n, TRUE);
            }
        }
    }

    if ((m_pClient) && (m_pClient->protocol()))
    {
        m_pClient->protocol()->playDone(m_sessionID);
        m_pClient->protocol()->disconnect(m_sessionID);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

STDMETHODIMP
Player::Session::AlertAndDisconnect(IHXBuffer* pAlert)
{
    // Entered
    ++m_ulFunctionsInUse;

    DPRINTF(0x48000000, ("%lu: P::S::AlertAndDisconnect() is killing me!\n",
            m_pClient->conn_id));
    SendAlert(pAlert);

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

STDMETHODIMP
Player::Session::HostRedirect(IHXBuffer* pHost, UINT16 nPort)
{
    // Entered
    ++m_ulFunctionsInUse;

    IHXBuffer* pURL = NULL;
    char       pNewURL[2048];

    if(m_pClient->m_protType == HXPROT_RTSP)
    {
        sprintf(pNewURL, "rtsp://%-.800s:%d/%-.800s", (char*)pHost->GetBuffer(),
            nPort, m_url->full);

        pURL = new ServerBuffer(TRUE);
        pURL->Set((UCHAR*)pNewURL, strlen(pNewURL) + 1);

        // Redirect the player to this new full URL
        NetworkRedirect(pURL, 0);

        HX_RELEASE(pURL);
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

STDMETHODIMP
Player::Session::NetworkRedirect(IHXBuffer* pURL, UINT32 ulSecsFromNow)
{
    // Entered
    ++m_ulFunctionsInUse;

    HX_RESULT hResult = HXR_NOTIMPL;

    if(m_pClient->m_protType == HXPROT_RTSP)
    {
        if (m_pClient->protocol()->sendRedirect(m_sessionID,
            (char*)pURL->GetBuffer(), ulSecsFromNow) == 0)
        {
            m_bRedirected = TRUE;
            if (m_pStats->GetEndStatus() == SSES_NOT_ENDED)
            {
                m_pStats->SetEndStatus(SSES_REDIRECTED);
            }
            hResult = HXR_OK;
        }
        else
        {
            hResult = HXR_FAIL;
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return hResult;
}

STDMETHODIMP
Player::Session::NetworkProxyRedirect(IHXBuffer* pURL)
{
    // Entered
    ++m_ulFunctionsInUse;

    HX_RESULT hResult = HXR_NOTIMPL;

    if(m_pClient->m_protType == HXPROT_RTSP)
    {
        if (m_pClient->protocol()->sendProxyRedirect(m_sessionID,
            (char*)pURL->GetBuffer()) == 0)
        {
            hResult = HXR_OK;
        }
        else
        {
            hResult = HXR_FAIL;
        }
    }

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return hResult;
}

STDMETHODIMP
Player::Session::Redirect(IHXBuffer* pPartialURL)
{
    // Entered
    ++m_ulFunctionsInUse;

    HX_DELETE(m_url);

    if (!m_pRequest)
    {
        --m_ulFunctionsInUse;
        return HXR_FAIL;
    }

    // Attempt to change the URL that the player is accessing
    m_url = new URL((const char*)pPartialURL->GetBuffer(),
        pPartialURL->GetSize()-1);
    m_pRequest->SetURL((char*)pPartialURL->GetBuffer());

    // Replace the url in the registry
    char str[512];
    IHXBuffer* pURL = new ServerBuffer(TRUE);
    pURL->Set((BYTE*)m_url->full, strlen(m_url->full)+1);
    if (m_pClient->use_registry_for_stats())
    {
        sprintf(str, "%s.URL", m_pRegistryKey);
        m_pProc->pc->registry->SetStr(str, pURL, m_pProc);
    }

    if (m_pStats)
    {
        m_pStats->SetURL(pURL);
    }
    pURL->Release();

    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return HXR_OK;
}

void
Player::Session::DoDone()
{
    m_uDoneCallbackHandle = 0;

    // Notify all Allowance plugins
    OnStop();

    if (m_pSignalBusCtl)
    {
        m_pSignalBusCtl->DestroySignalBus(m_pProc, m_pSessionId);
        m_pSignalBusCtl->Release();
        m_pSignalBusCtl = NULL;
    }

    if (m_pSourceControl)
    {
        m_pSourceControl->Done();
        HX_RELEASE(m_pSourceControl);
    }

    if (m_pSrcFinder)
    {
        m_pSrcFinder->Close();
        HX_RELEASE(m_pSrcFinder);
    }

    HX_RELEASE(m_pBroadcastMapper);
    HX_RELEASE(m_pMimeMapper);
    HX_RELEASE(m_pBlockTransferFileFormat);
    HX_RELEASE(m_pFileFormat);
    HX_RELEASE(m_pRequest);

    if (m_pDataConvert)
    {
        m_pDataConvert->Done();
        HX_RELEASE(m_pDataConvert);
    }

    if (m_ppStreams != NULL)
    {
        clear_stream_list();
        HX_VECTOR_DELETE(m_ppStreams);
    }

    if (m_pAllowanceMgrs)
    {
        clear_allowance_list();
        HX_DELETE(m_pAllowanceMgrs);
    }

    HX_RELEASE(m_pAllowanceSerializer);
}

/* IHXClientBandwidthController Methods */

STDMETHODIMP
Player::Session::GetCurrentBandwidth(REF(ULONG32) ulBandwidth)
{
    ulBandwidth = m_pSessionControl->GetDeliveryRate();
    return HXR_OK;
}

STDMETHODIMP
Player::Session::GetBandwidthStep(ULONG32 ulUpperBound,REF(ULONG32) ulBandwidth)
{
    ULONG32 ulRuleBookCt = 0;
    StreamInfo* pStreamInfo;
    ASMRuleBook*  pRuleBook;

    int i;

    ulBandwidth = 0;

    if (m_pMasterRuleBookBuffer)
    {
        IHXBuffer* pBuffer = new ServerBuffer(TRUE);
        IHXValues* pValues = new CHXHeader();
        pValues->AddRef();
        char pBandwidth[32];
        BOOL* pCurrentSubInfo;

        if (m_pMasterRuleBook == NULL)
        {
            m_pMasterRuleBook = new ASMRuleBook((char*)m_pMasterRuleBookBuffer->GetBuffer());
        }

        pCurrentSubInfo = new BOOL[m_pMasterRuleBook->GetNumRules()];

        sprintf ((char *)pBandwidth, "%lu", ulUpperBound);

        pBuffer->Set((UCHAR*)pBandwidth, strlen((char *)pBandwidth) + 1);

        pValues->SetPropertyCString("Bandwidth",  pBuffer);

        pBuffer->Release();
        pBuffer = NULL;

        m_pMasterRuleBook->GetSubscription(pCurrentSubInfo, pValues);

        pValues->Release();
        pValues = NULL;

        for (UINT16 idxRule = 0; idxRule < m_pMasterRuleBook->GetNumRules();
             idxRule++)
        {
            if (pCurrentSubInfo[idxRule])
            {
                IHXValues* pVal;
                IHXBuffer* pBuf;

                m_pMasterRuleBook->GetProperties(idxRule, pVal);

                for (UINT16 ct = 0; ct < m_num_streams; ct++)
                {
                    char pbuf[32];

                    sprintf (pbuf,"Stream%uBandwidth",ct);
                    if (HXR_OK == pVal->GetPropertyCString(pbuf,pBuf))
                    {
                        ulBandwidth += atoi((char*)pBuf->GetBuffer());
                        pBuf->Release();
                    }
                }

                pVal->Release();
            }
        }

        delete pCurrentSubInfo;

        ulRuleBookCt++;
    }
    else
    {
        for (i = 0; i < m_num_streams; i++)
        {
            pStreamInfo = m_ppStreams[i];

            if (pStreamInfo->m_pRuleBookBuffer)
            {
                if (pStreamInfo->m_pRuleBook == NULL)
                {
                    pStreamInfo->m_pRuleBook = new ASMRuleBook((char*)pStreamInfo->m_pRuleBookBuffer->GetBuffer());
                }

                pRuleBook = pStreamInfo->m_pRuleBook;

                IHXBuffer* pBuffer = new ServerBuffer(TRUE);
                IHXValues* pValues = new CHXHeader();
                pValues->AddRef();
                char pBandwidth[32];
                BOOL* pCurrentSubInfo = new BOOL[pRuleBook->GetNumRules()];

                sprintf ((char *)pBandwidth, "%lu", ulUpperBound);

                pBuffer->Set((UCHAR*)pBandwidth, strlen((char *)pBandwidth) + 1);
                pValues->SetPropertyCString("Bandwidth",  pBuffer);

                pBuffer->Release();
                pBuffer = NULL;

                pRuleBook->GetSubscription(pCurrentSubInfo, pValues);

                pValues->Release();
                pValues = NULL;

                for (UINT16 idxRule = 0; idxRule < pRuleBook->GetNumRules();
                     idxRule++)
                {
                    if (pCurrentSubInfo[idxRule])
                    {
                        IHXValues* pVal;
                        IHXBuffer* pBuf;

                        pRuleBook->GetProperties(idxRule, pVal);

                        if (HXR_OK == pVal->GetPropertyCString("AverageBandwidth",pBuf))
                        {
                            ulBandwidth += atoi((char*)pBuf->GetBuffer());
                            pBuf->Release();
                        }

                        pVal->Release();
                    }
                }

                delete pCurrentSubInfo;
                ulRuleBookCt++;
            }

        }
    }

    if (ulRuleBookCt == 0)
    {
        // No Rulebooks were found so just return current BW
        GetCurrentBandwidth(ulBandwidth);
    }

    return HXR_OK;
}

STDMETHODIMP
Player::Session::SetBandwidthLimit(ULONG32 ulBandwidth)
{
    return HXR_FAILED;
}

float
Player::Session::SetSpeed(FIXED32 fSpeed)
{
    if (m_pFileFormat && m_pSessionControl)
    {
        return m_pSessionControl->SetSpeed(fSpeed);
    }
    else
    {
        return 1.0;
    }
}

HX_RESULT
Player::Session::SeekByPacket(UINT32 ulPacketNumber)
{
    // Entered
    ++m_ulFunctionsInUse;

    HX_RESULT hxr = HXR_FAIL;
    IHXSeekByPacket* pSeekByPacket = NULL;

    if (m_pFileFormat)
    {
        m_pFileFormat->QueryInterface(IID_IHXSeekByPacket, (void**)&pSeekByPacket);
    }

    if(!pSeekByPacket)
    {
        if (m_pClient->protocol())
        {
            m_pClient->protocol()->SeekByPacketDone(HXR_NOT_SUPPORTED, 0);
        }
        goto bail;
    }

    hxr = pSeekByPacket->SeekToPacket(ulPacketNumber, this);

    if (FAILED(hxr))
    {
        if (m_pClient->protocol())
        {
            m_pClient->protocol()->SeekByPacketDone(hxr, 0);
        }
    }
    pSeekByPacket->Release();

bail:
    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();
    return hxr;
}

HX_RESULT
Player::Session::SeekToPacketDone(HX_RESULT status, UINT32 ulStartingTimestamp)
{
    if(status == HXR_OK)
    {
        m_pSessionControl->SetStartingTimestamp(ulStartingTimestamp);
    }

    // XXXJJ, we need to inform session control that a seek has been performed.
    // But we can't do it at the beginning of the seek, because we don't know
    // the timestamp then.

    m_pSessionControl->StartSeek(ulStartingTimestamp);
    m_pSessionControl->SeekDone();

    if (m_pClient->protocol())
    {
        m_pClient->protocol()->SeekByPacketDone(status, ulStartingTimestamp);
    }

    return HXR_OK;

}

HX_RESULT
Player::Session::SetScale(FIXED32 fScale)
{
    // Entered
    ++m_ulFunctionsInUse;

    HX_ASSERT(m_pSessionControl);

    HX_RESULT hxr = HXR_OK;
    IHXSetPlayParam* pSetParam = NULL;

    if (m_pFileFormat)
    {
        m_pFileFormat->QueryInterface(IID_IHXSetPlayParam, (void**)&pSetParam);
    }

    if(!pSetParam)
    {
        if (m_pClient->protocol())
        {
            m_pClient->protocol()->SetScaleDone(HXR_NOT_SUPPORTED, this, HX_FLOAT_TO_FIXED(1.0));
        }
        goto bail;
    }

    hxr = pSetParam->SetParam(HX_PLAYPARAM_SCALE, fScale, this);
    if (FAILED(hxr))
    {
        if (m_pClient->protocol())
        {
            m_pClient->protocol()->SetScaleDone(hxr, this, HX_FLOAT_TO_FIXED(1.0));
        }
    }
    pSetParam->Release();

bail:
    // Left Function
    --m_ulFunctionsInUse;
    IfDoneCleanup();

    return hxr;
}

STDMETHODIMP
Player::Session::SetParamDone
    (HX_RESULT status, HX_PLAY_PARAM param, UINT32 ulValue)
{
    if (m_pClient->protocol() && HX_PLAYPARAM_SCALE == param)
    {
        m_pClient->protocol()->SetScaleDone(status, this, (FIXED32)ulValue);
    }
    return HXR_OK;
}


Player::Session::InactivityCallback::InactivityCallback(Session* pOwner) :
    m_ulRefCount(0),
    m_pOwner(pOwner)
{
    HX_ASSERT(m_pOwner != NULL);
    m_pOwner->AddRef();
}

Player::Session::InactivityCallback::~InactivityCallback()
{
    HX_RELEASE(m_pOwner);
}

STDMETHODIMP
Player::Session::InactivityCallback::QueryInterface(REFIID riid, void** ppvObj)
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

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
Player::Session::InactivityCallback::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
Player::Session::InactivityCallback::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
Player::Session::InactivityCallback::Func()
{
    if (m_pOwner && m_pOwner->m_pPlayer)
    {
        m_pOwner->m_uInactivityCallbackHandle = 0;

        if (m_pOwner->m_pStats)
        {
            m_pOwner->m_pStats->SetEndStatus(SSES_CLIENT_TIMEOUT);
        }

        if (m_pOwner->m_pPlayer->NumSessions() == 1)
        {
            m_pOwner->Disconnect();
            m_pOwner->m_pPlayer->m_pClient->OnClosed(SSES_CLIENT_TIMEOUT);
        }
        else
        {
            m_pOwner->m_pPlayer->RemoveSession(
                    (const char*) m_pOwner->m_sessionID, SSES_CLIENT_TIMEOUT);
        }
    }
    return HXR_OK;
}

Player::Session::DoneCallback::DoneCallback(Session* pOwner) :
    m_ulRefCount(0),
    m_pOwner(pOwner)
{
    HX_ASSERT(m_pOwner != NULL);
    m_pOwner->AddRef();
}

Player::Session::DoneCallback::~DoneCallback()
{
    HX_RELEASE(m_pOwner);
}

STDMETHODIMP
Player::Session::DoneCallback::QueryInterface(REFIID riid, void** ppvObj)
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

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
Player::Session::DoneCallback::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
Player::Session::DoneCallback::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
Player::Session::DoneCallback::Func()
{
    m_pOwner->DoDone();
    return HXR_OK;
}

/***********************************************************************
 *  IHXDataConvertResponse
 */




HX_RESULT
Player::Session::ConvertedDataReady(HX_RESULT status, IHXPacket* pBuffer)
{
    return HXR_OK;
}

HX_RESULT
Player::Session::SendControlBuffer(IHXBuffer* pBuffer)
{
    if (m_pClient->m_protType == HXPROT_RTSP)
    {
        if (!m_pClient->protocol())
        {
            return HXR_NOT_INITIALIZED;
        }
        char* p64Buf = new char[pBuffer->GetSize() * 2 + 4];
        BinTo64((const unsigned char*)pBuffer->GetBuffer(),
                pBuffer->GetSize(), p64Buf);
        /*
         * If it is an old proxy that we are being played through
         * then we must save the control buffer to go out with
         * the header because the initiate session does not work.
         */
        if (m_pClient->protocol()->SendSetParam((const char*)m_url->key,
                (const char*)m_sessionID, "DataConvertBuffer", "1",
                "base64", p64Buf) != HXR_OK)
        {
            ServerBuffer* pHeaderControlBuffer = new ServerBuffer((UCHAR*)p64Buf,
                    strlen(p64Buf));
            m_pHeaderControlBuffer = pHeaderControlBuffer;
            m_pHeaderControlBuffer->AddRef();
        }
        else
        {
            delete[] p64Buf;
        }
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

void
Player::Session::ControlBufferReady(const char* pContent)
{
    IHXBuffer* pNewBuf = NULL;
    m_pProc->pc->common_class_factory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pNewBuf);
    int contentLen = strlen(pContent);
    pNewBuf->SetSize(contentLen);
    int offset = BinFrom64(pContent, contentLen,
            (unsigned char*)pNewBuf->GetBuffer());
    pNewBuf->SetSize(offset);
    m_pDataConvert->ControlBufferReady(pNewBuf);
    pNewBuf->Release();
}

/*
 * AllowanceSerializer
 */

AllowanceSerializer::AllowanceSerializer(BOOL bPrintDebugMessage)
    : m_pArray(NULL)
    , m_NumPlugins(0)
    , m_Curr(0)
    , m_pRequest(NULL)
    , m_ulRefCount(0)
    , m_bPrintDebugMessage(bPrintDebugMessage)
{
}

AllowanceSerializer::~AllowanceSerializer()
{
    int i;

    for (i = 0; i < m_NumPlugins; ++i)
    {
        HX_RELEASE(m_pArray[i].pMgr);
    }
    HX_VECTOR_DELETE(m_pArray);

    HX_RELEASE(m_pRequest);
}

STDMETHODIMP_(ULONG32)
AllowanceSerializer::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
AllowanceSerializer::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

void
AllowanceSerializer::OnURL(ServerRequest* pRequest, CHXSimpleList* pAllowanceMgrs)
{
    if (m_pArray)
    {
        // We shouldn't get called twice.
        HX_ASSERT(0);
        return;
    }

    m_pRequest = pRequest;
    m_pRequest->AddRef();

    m_NumPlugins = pAllowanceMgrs->GetCount();
    m_pArray = new MgrInfo[m_NumPlugins];

    // the caller should short-circuit us if there are no
    // allowance plugins
    HX_ASSERT(m_NumPlugins > 0);

    int i = 0;
    LISTPOSITION pos = pAllowanceMgrs->GetHeadPosition();
    for ( ; pos; pAllowanceMgrs->GetNext(pos))
    {
        m_pArray[i].pMgr = (AllowanceMgr*) pAllowanceMgrs->GetAt(pos);
        m_pArray[i].pMgr->AddRef();
        ++i;
    }

    // --- XXXSSH
    // We could sort m_pArray here by allowance priority, once allowance
    // plugins know how to tell us that.
    // ---
    // XXXGo - we had to make sure Allow gets call before any other
    // allowance plugins, so it has been placed at the Head of m_pAllowanceMgrs
    // in Player::Session::InitAllowancePlugins()...

    // Call the OnURL method for our first plugin
    m_Curr = 0;
    CallNextOne();
}

AllowanceMgr*
AllowanceSerializer::OnURLDone(HX_RESULT status, REF(enum AllowanceStatus) OverallStatus)
{
    AllowanceMgr* pMgr = m_pArray[m_Curr].pMgr;

    if ((status != HXR_OK) && (status != HXR_INVALID_URL_PATH))
    {
        OverallStatus = reject_playback;
        return pMgr;
    }

#ifdef XXXAAK_PERF_NO_ALLOWANCE
    // XXX: aak -- get the server to work without allowance plugin
    OverallStatus = allow_playback;
    return pMgr;
#endif /* XXXAAK_PERF_NO_ALLOWANCE */

    if (++m_Curr == m_NumPlugins)
    {
        if (status == HXR_INVALID_URL_PATH)
            OverallStatus = allow_playback_withoutcurrent;
        else
            OverallStatus = allow_playback;
        return pMgr;
    }
    else
    {
        if (status == HXR_INVALID_URL_PATH)
            OverallStatus = more_plugins_withoutcurrent;
        else
            OverallStatus = more_plugins;
        return pMgr;
        // Make Player::Session initiate the next OnURL call.  We'd do
        // it ourselves, but caller might want to stop us short for some
        // reason.
    }
}

void AllowanceSerializer::CallNextOne()
{
    HX_ASSERT(m_Curr < m_NumPlugins);

    ServerRequestWrapper* pWrapper = NULL;
    pWrapper = new ServerRequestWrapper(FS_HEADERS, m_pRequest);
    pWrapper->AddRef();

    ALLOWANCE_DEBUG_MSG( m_bPrintDebugMessage, m_pArray[m_Curr].pMgr
               , ("Calling OnURL()\n"));

    m_pArray[m_Curr].pMgr->OnURL(pWrapper);
    HX_RELEASE(pWrapper);

    // Allowance mgr calls back to Player::Session::OnURLDone, which then
    // calls our OnURLDone.  Only strange case is NTLM, which may bypass
    // us completely, and send back a challenge via RTSP.  The player then
    // sends back another msg to us, and RTSP knows how to continue where
    // it left off, and we too continue where we left off.
}
