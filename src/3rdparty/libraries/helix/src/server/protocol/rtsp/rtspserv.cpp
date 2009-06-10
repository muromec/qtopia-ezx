/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspserv.cpp,v 1.312 2008/01/31 09:10:42 dsingh Exp $
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
#include "hxstrutl.h"
#include "hxbuffer.h"
#include "hxsbuffer.h"
#include "platform.h"
#include "hxmon.h"
#include "hxslist.h"
#include "hxnet.h"
#include "hxclientprofile.h"
#include "hxauthn.h"
#include "hxshutdown.h"
#include "rtspif.h"
#include "crtspbase.h"
#include "rtspif.h"
#include "proc.h"
#include "rartp.h"
#include "netbyte.h"
#include "hxsdesc.h"
#include "plgnhand.h"
#include "hxprot.h"
#include "timerep.h"
#include "servbuffer.h"
#include "srcerrs.h"
#include "hxerror.h"
#include "debug.h"
#include "rtspserv.h"
#include "rtspsession.h"
#include "servlist.h"
#include "transportparams.h"
#include "rdt_udp.h"
#include "mime2.h"
#include "rtspmsg2.h"
#include "rtsputil.h"
#include "urlutil.h"
#include "nptime.h"
#include "smpte.h"
#include "bdst_stats.h"
#include "bcngtran.h"
#include "defslice.h"
#include "hxqosinfo.h"
#include "rtspsessmgr.h"
#include "pckunpck.h"
#include "rtspstats.h"
#include "sdpstats.h"
#include "servertrace.h"
#include "client.h"
#include "clientsession.h"
#include "multicast_mgr.h"
#include "server_request.h"
#include "url.h"
#include "servreg.h"
#include "base_errmsg.h"
#include "hxpcktflwctrl.h"
#include "server_context.h"
#include "hxsockutil.h"
#include "config.h"
#include "client_profile_mgr.h"
#include "server_stats.h"
#include "mimescan.h"
#include "qos_diffserv_cfg.h"
#include "server_version.h"
#include "httpmsg.h"
#include "servchallenge.h"
#ifdef HELIX_FEATURE_SERVER_FCS
#include "fcsutil.h"
#endif // HELIX_FEATURE_SERVER_FCS
#include "hxqos.h"
#include "qos_prof_conf.h"
#include "qos_cfg_names.h"

#if defined _WINDOWS && !defined snprintf
#define snprintf _snprintf
#endif



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
RTSPServerProtocol::RTSPServerProtocol() :
    CRTSPBaseProtocol()
{
    m_pszVersionStr = new char[12];
    sprintf(m_pszVersionStr, "RTSP/%d.%d", 
    RTSPMessage::MAJ_VERSION,
    RTSPMessage::MIN_VERSION);
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
    HX_VECTOR_DELETE(m_pszVersionStr);
}

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
    else
    {
        return CRTSPBaseProtocol::QueryInterface(riid, ppvObj);
    }
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
        hxr = m_pSock->Read(&pBuf);
        if (SUCCEEDED(hxr) && pBuf != NULL)
        {
            ReadDone(HXR_OK, pBuf);
        }
        else
        {
            HandleSocketError();
        }
        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
        HandleSocketError();
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
        NotifyWriteEvent();
        break;
    default:
        HX_ASSERT(FALSE);
    }
    HX_RELEASE(pBuf);
    return HXR_OK;
}


/*
 * HXProtocol methods
 */
void
RTSPServerProtocol::init(Process* pProc, IHXSocket* pSock)
{
    CRTSPBaseProtocol::init(pProc, pSock);

    m_ulClientStatsObjId = m_pClient->GetClientStats()->GetID();
    if (m_bTrackEvents)
    {
        m_pRtspStatsMgr->SetClientStatsObj(m_pClient->GetClientStats());
    }

    // launch the keepalive callback, if needed
    if (m_tKeepAliveInterval)
    {
        KeepAliveCallback* pCB = new KeepAliveCallback(this);
        m_ulKeepAliveCallbackID = m_pIScheduler->RelativeEnter(pCB, 
            m_tKeepAliveInterval*1000);
    }
}
