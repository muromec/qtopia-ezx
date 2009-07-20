/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qtcloak.cpp,v 1.10 2005/07/23 01:58:27 darrick Exp $
 *
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
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
#include "hxprefs.h"
#include "hxstring.h"
#include "hxmarsh.h"
#include "chxpckts.h"
#include "rtsputil.h"

#include "sio.h"
#include "tcpio.h"
#include "callback_container.h"
#include "server_engine.h"
#include "base_errmsg.h"

#include "debug.h"
#include "cbqueue.h"

#include "client.h"
#include "hxstrutl.h"
#include "url.h"
#include "timerep.h"
#include "plgnhand.h"
#include "cloaked_guid_dict.h"
#include "dict.h"
#include "netbyte.h"
#include "servbuffer.h"

#include "hxprot.h"

#include "http.h"
#include "httppars.h"
#include "httpprotmgr.h"
#include "tsid.h"

#include "http_demux.h"

#include "httpclk.h"    // CloakedHTTPResponse
#include "rtspprot.h"

#include "servsockimp.h"
#include "servlbsock.h"

#include "cloak_common.h"
#include "basecloak.h"
#include "qtcloak.h"

#define QT_HEADER_CONTENT_TYPE          "application/x-rtsp-tunnelled"


CQTCloakGETHandler::CQTCloakGETHandler(void) 
: CBaseCloakGETHandler()
{
    // Empty
}

CQTCloakGETHandler::~CQTCloakGETHandler(void)
{
    // Empty
}


STDMETHODIMP_(HTTP_TYPE)
CQTCloakGETHandler::GetHandlerType(void)
{
    return HTTP_TYPE_CLOAK_QT;
}

STDMETHODIMP_(void)
CQTCloakGETHandler::OnDispatch(void)
{
    AddRef();
    CBaseCloakGETHandler::OnDispatch();
    sendResponseHeader(QT_HEADER_CONTENT_TYPE);

    CBaseCloakPOSTHandler* pPOSTHandler = 0;
    // just in case the socket writes failed
    if (m_pConn && m_pConn->GetPOSTHandlerCount() > 0)
    {
        m_pConn->GetFirstPOSTHandler(pPOSTHandler);
        if (pPOSTHandler)
        {
            pPOSTHandler->ProcessPendingData();
            pPOSTHandler->Release();
        }
    }
    Release();
}

STDMETHODIMP_(void)
CQTCloakGETHandler::OnRequest(HTTPRequestMessage* pMsg)
{
    AddRef();

    UINT32 ulIdLen = 0;
    CHXString strSession;

    HX_ASSERT(pMsg->tag() == HTTPMessage::T_GET);

    if (m_bGotRequest)
    {
        goto EndOnRequestOnError;
    }

    m_bGotRequest = TRUE;

    strSession = pMsg->getHeaderValue("x-sessioncookie");

    if (strSession.IsEmpty())
    {
        goto EndOnRequestOnError;
    }

    ulIdLen = strSession.GetLength();
    m_szCloakSessionId = new char[ulIdLen + 1];
    strncpy(m_szCloakSessionId, (const char*)strSession, ulIdLen);
    m_szCloakSessionId[ulIdLen] = '\0';

    // addref and return CloakConn if there is one
    if (!m_pConn)
    {
        m_pCloakedGUIDDict->find(m_szCloakSessionId, m_pConn);
    }

    if (!m_pConn)
    {
        m_pConn = new CloakConn();
        m_pConn->AddRef();
        m_pConn->m_iProcNum = m_pProc->pc->streamer_info->BestStreamer(m_pProc);
        m_pCloakedGUIDDict->enter(m_szCloakSessionId, m_pConn);
    }

    CBaseCloakGETHandler::OnRequest(pMsg);

    Release();
    return;

EndOnRequestOnError:

    m_pDemux->Close(HXR_FAIL);
    CleanupConn();
    Release();
}

// ***************************************************************************

CQTCloakPOSTHandler::CQTCloakPOSTHandler(void) 
: CBaseCloakPOSTHandler()
{
    // Empty
}

CQTCloakPOSTHandler::~CQTCloakPOSTHandler(void)
{
    // Empty
}

STDMETHODIMP_(HTTP_TYPE)
CQTCloakPOSTHandler::GetHandlerType(void)
{
    return HTTP_TYPE_CLOAK_QT;
}

STDMETHODIMP_(void)
CQTCloakPOSTHandler::OnRequest(HTTPRequestMessage* pMsg)
{
    AddRef();

    UINT32 ulIdLen = 0;
    CHXServSocket* pSock = NULL;
    CHXString strSession;

    if (m_bGotRequest)
    {
        goto EndOnRequestOnError;
    }

    m_bGotRequest = TRUE;
    
    strSession = pMsg->getHeaderValue("x-sessioncookie");

    if (strSession.IsEmpty())
    {
        goto EndOnRequestOnError;
    }

    ulIdLen = strSession.GetLength();
    m_szCloakSessionId = new char[ulIdLen + 1];
    strncpy(m_szCloakSessionId, (const char*)strSession, ulIdLen);
    m_szCloakSessionId[ulIdLen] = '\0';

    if (!m_pConn)
    {
        m_pCloakedGUIDDict->find(m_szCloakSessionId, m_pConn);
    }

    if (!m_pConn)
    {
        m_pConn = new CloakConn();
        m_pConn->AddRef();
        m_pConn->m_iProcNum = m_pProc->pc->streamer_info->BestStreamer(m_pProc);
        m_pCloakedGUIDDict->enter(m_szCloakSessionId, m_pConn);
    }

    // Dispatch to the streamer
    pSock = m_pDemux->GetSock();

    if (pSock)
    {
        pSock->Dispatch(m_pConn->m_iProcNum);
        pSock->Release();
        pSock = NULL;
    }

    Release();
    return;

EndOnRequestOnError:

    m_pDemux->Close(HXR_FAIL);
    CleanupConn(HXR_FAIL);
    Release();
}

STDMETHODIMP_(void)
CQTCloakPOSTHandler::OnData(IHXBuffer* pBuf)
{
    AddRef();
    CBaseCloakGETHandler* pGETHandler = NULL;
    BYTE* pData = (BYTE*)pBuf->GetBuffer();
    UINT32 uDataLen = pBuf->GetSize();

    
    if (m_pConn)
    {
        m_pConn->GetGETHandler(pGETHandler);
    }

    if (!pGETHandler)
    {
        QueuePendingData(pData, uDataLen);
        Release();
        return;
    }

    IHXBuffer* pDecBuf = NULL;
    UINT32 n = decodeBuf(pData, uDataLen, pDecBuf);
    if (n == 0)
    {
        m_pDemux->Close(HXR_FAIL);
        return;
    }
    if (m_pConn)
    {
        m_pConn->OnPOSTData(pDecBuf);
    }
    HX_RELEASE(pDecBuf);

    if (n != uDataLen)
    {
        // We should not get here under normal circumstances because the RTSP
        // messages are never fragmented, but we should handle this case...
        HX_ASSERT(FALSE);
    }
}


/*
 * Decode a Base64 encoded message sent by the client.  The client does not
 * have any boundary markers between messages so we decode what we can.
 *
 * Returns the number of bytes used from the data or zero on failure.
 */
UINT32
CQTCloakPOSTHandler::decodeBuf(BYTE* pData, UINT32 uDataLen, IHXBuffer*& rpDecBuf)
{
    BYTE* pDataEnd;

    BYTE* pDecData;
    INT32 iDecLen;

    rpDecBuf = NULL;

    uDataLen &= ~0x3;
    if (uDataLen < 4)
    {
        return 0;
    }
    pDataEnd = pData+uDataLen;

    rpDecBuf = new ServerBuffer(TRUE);
    rpDecBuf->SetSize(uDataLen*3/4);
    pDecData = rpDecBuf->GetBuffer();
    iDecLen = BinFrom64((char*)pData, uDataLen, pDecData);
    if (iDecLen <= 0 || (UINT32)iDecLen < (uDataLen*3/4)-2)
    {
        DPRINTF(0x10000000, ("content decoding failed (%ld)\n", iDecLen));
        HX_RELEASE(rpDecBuf);
        return 0;
    }
    rpDecBuf->SetSize(iDecLen);

    return uDataLen;
}
