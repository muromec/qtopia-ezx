/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rn1cloak.cpp,v 1.19 2005/09/01 19:08:34 srobinson Exp $
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
#include "rn1cloak.h"

//#define FORCE_MULTIPOST

#define RN_HEADER_CONTENT_TYPE          "audio/x-pn-realaudio"

enum
{
    HTTP_CLOAK_OK = 0,
    HTTP_CLOAK_GENERAL_ERROR,
    HTTP_CLOAK_POST_NOT_RECEIVED,
    HTTP_CLOAK_INVALID_GUID
};

CRN1CloakGETHandler::CRN1CloakGETHandler() 
: CBaseCloakGETHandler()
, m_pPostCB(NULL)
, m_ulPostCbId(0)
, m_bIsMultiPOST(FALSE)
{
}

CRN1CloakGETHandler::~CRN1CloakGETHandler()
{
    HX_RELEASE(m_pPostCB);
}

STDMETHODIMP_(HTTP_TYPE)
CRN1CloakGETHandler::GetHandlerType(void)
{
    return HTTP_TYPE_CLOAK_RN1;
}

STDMETHODIMP_(void)
CRN1CloakGETHandler::OnDispatch(void)
{
    AddRef();

    CBaseCloakGETHandler::OnDispatch();

    if (m_pConn && m_pConn->GetPOSTHandlerCount() > 0)
    {
        // The POST connection has already arrived.  Tell the player to use
        // single-POST mode.

        sendResponseHeader(RN_HEADER_CONTENT_TYPE);
        sendPostStatus(HTTP_CLOAK_OK);

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
    }
    else
    {
        
        // The POST connection has not arrived yet.  Schedule a callback for
        // one second.  If the callback fires and the POST has not arrived,
        // assume the POST got trapped by a firewall and we tell the player
        // to use multi-POST mode.

        m_pPostCB = new PostCallback(this, m_pProc);
        m_pPostCB->AddRef();
        m_ulPostCbId = m_pProc->pc->engine->schedule.enter(m_pProc->pc->engine->now + Timeval(1.0), m_pPostCB);
    }

    Release();
}

STDMETHODIMP_(void)
CRN1CloakGETHandler::OnRequest(HTTPRequestMessage* pMsg)
{
    AddRef();

    CHXServSocket* pSock = 0;
    const char* pUrl = 0;
    UINT32 uUrlLen = 0;
    const char* pTerm = 0;

    HX_ASSERT(pMsg->tag() == HTTPMessage::T_GET);
    if (m_bGotRequest)
    {
        goto EndOnRequestOnError;
    }
    m_bGotRequest = TRUE;

    // HTTPGetMessage::url() is guaranteed to return a non-null pointer
    // representing the url (and only the url) of the message.  This is cast
    // from a CHXString so it is NULL terminated.

    // Valid base url is "/SmpDsBhgRl<guid>", where <guid> is 36 octets.
    // Note that the url can have query parameters (eg. "?1=1").

    pUrl = pMsg->url();
    uUrlLen = strlen(pUrl);

    // 11 == strlen("/SmpDsBhgRl")
    if (uUrlLen < 11 + GUID_STR_LENGTH)
    {
        DPRINTF(0x10000000, ("RN Cloak v1 -- bad GUID\n"));
        goto EndOnRequestOnError;
    }

    if (FAILED(ParseCloakSessionId(pUrl + 11)))
    {
        goto EndOnRequestOnError;
    }

    HX_ASSERT(m_szCloakSessionId);

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

HX_RESULT
CRN1CloakGETHandler::ParseCloakSessionId(const char* szBuf)
{
    const char* szEnd = szBuf;

    // Find the end of the GUID and ensure it's 36 chars
    while (*szEnd && *szEnd != '?')
    {
        szEnd++;
    }

    if (szEnd != szBuf + GUID_STR_LENGTH)
    {
        DPRINTF(0x10000000, ("RN Cloak v1 -- bad GUID\n"));
        return HXR_UNEXPECTED;
    }

    if (!m_szCloakSessionId)
    {
        m_szCloakSessionId = new char [GUID_STR_LENGTH + 1];
    }

    memcpy(m_szCloakSessionId, szBuf, GUID_STR_LENGTH);
    m_szCloakSessionId[GUID_STR_LENGTH] = '\0';

    return HXR_OK;
}

STDMETHODIMP_(void)
CRN1CloakGETHandler::OnClosed(void)
{
    AddRef();

    CBaseCloakGETHandler::OnClosed();
    
    if (m_pProc && m_ulPostCbId)
    {
        m_pProc->pc->engine->schedule.remove(m_ulPostCbId);
        m_ulPostCbId = 0;
        HX_RELEASE(m_pPostCB);
    }

    Release();
}

void
CRN1CloakGETHandler::PostReceived(void)
{
    AddRef();

    if (m_ulPostCbId)
    {
        m_pProc->pc->engine->schedule.remove(m_ulPostCbId);
        m_ulPostCbId = 0;
        HX_RELEASE(m_pPostCB);
        sendResponseHeader(RN_HEADER_CONTENT_TYPE);
        sendPostStatus(HTTP_CLOAK_OK);
    }

    Release();
}

void
CRN1CloakGETHandler::PostTimeout(void)
{
    AddRef();

    m_ulPostCbId = 0;
    m_pPostCB = NULL;

    HX_ASSERT(m_pConn);
    if (m_pConn)
    {
        m_pConn->SetCloakMode(CLOAK_MODE_MULTIPOST);
    }

    if (m_bOnClosedCalled)
    {
        Release();
        return;
    }

    sendResponseHeader(RN_HEADER_CONTENT_TYPE);
    sendPostStatus(HTTP_CLOAK_POST_NOT_RECEIVED);

    Release();
}

CRN1CloakGETHandler::PostCallback::PostCallback(CRN1CloakGETHandler* pOwner, Process* pProc) :
    m_pOwner(pOwner),
    m_pProc(pProc)
{
    m_pOwner->AddRef();
}

CRN1CloakGETHandler::PostCallback::~PostCallback(void)
{
    m_pOwner->Release();
    m_pOwner = 0;
}

STDMETHODIMP
CRN1CloakGETHandler::PostCallback::Func(void)
{
    m_pOwner->PostTimeout();
    return HXR_OK;
}

// ***************************************************************************

CRN1CloakPOSTHandler::CRN1CloakPOSTHandler(void)
: CBaseCloakPOSTHandler()
, m_parse_state(PARSE_NONE)
{
    // Empty
}

STDMETHODIMP_(HTTP_TYPE)
CRN1CloakPOSTHandler::GetHandlerType(void)
{
    return HTTP_TYPE_CLOAK_RN1;
}

STDMETHODIMP_(void)
CRN1CloakPOSTHandler::OnDispatch(void)
{
    AddRef();

    m_pProc = m_pDemux->GetProc();
    
    if (m_pConn)
    {
        m_pConn->POSTChannelReady();
    }

    ProcessPendingData();

    Release();
}

STDMETHODIMP_(void)
CRN1CloakPOSTHandler::OnRequest(HTTPRequestMessage* pMsg)
{
    AddRef();

    HX_ASSERT(pMsg->tag() == HTTPMessage::T_POST);
    if (m_bGotRequest)
    {
        m_pDemux->Close(HXR_FAIL);
        CleanupConn(HXR_FAIL);
        Release();
        return;
    }
    m_bGotRequest = TRUE;

    pMsg->getHeaderValue("Content-Length", m_uContentLen);
#ifdef FORCE_MULTIPOST
    if (m_uContentLen == 32767)
    {
        return;
    }
#endif

    m_parse_state = PARSE_GUID;

    Release();
}

STDMETHODIMP_(void)
CRN1CloakPOSTHandler::OnData(IHXBuffer* pBuf)
{
    HX_RESULT hr = HXR_OK;

    CBaseCloakGETHandler* pGETHandler = NULL;

#ifdef FORCE_MULTIPOST
    if (m_uContentLen == 32767)
    {
        return;
    }
#endif

    BYTE* pData = (BYTE*)pBuf->GetBuffer();
    UINT32 uDataLen = pBuf->GetSize();

    AddRef();

    if (m_parse_state == PARSE_GUID)
    {
        hr = SetupPOSTChannel(pBuf);
    }
    else
    {
        hr = HandlePOSTData(pBuf);
    }

    if (FAILED(hr))
    {
        m_pDemux->Close(HXR_FAIL);
        CleanupConn(HXR_FAIL);
    }

    Release();
    return;
}


HX_RESULT
CRN1CloakPOSTHandler::SetupPOSTChannel(IHXBuffer* pBuf)
{
    BYTE* pData = (BYTE*)pBuf->GetBuffer();
    UINT32 uDataLen = pBuf->GetSize();

    // get the GUID from the content
    if (uDataLen < GUID_STR_LENGTH + 2)
    {
        //XXX: Should buffer here
        return HXR_UNEXPECTED;
    }

    if (FAILED(ParseCloakSessionId((const char*)pData)))
    {
        return HXR_UNEXPECTED;
    }

    HX_ASSERT(m_szCloakSessionId);

    pData += GUID_STR_LENGTH + 2;
    uDataLen -= GUID_STR_LENGTH + 2;

    // addref and return CloakedGUIDDictEntry if there is one
    if (!m_pConn)
    {
        m_pCloakedGUIDDict->find(m_szCloakSessionId, m_pConn);
    }

    if (m_pConn == NULL)
    {
        m_pConn = new CloakConn();
        m_pConn->AddRef();
        m_pConn->m_iProcNum = m_pProc->pc->streamer_info->BestStreamer(m_pProc);
        // remember to Release CloakConn when the CloakedGUIDDictEntry is
        // removed from the cloaked_guid_dict
        // ignoring the returned unique id of the cloaked_guid_dict entry
        m_pCloakedGUIDDict->enter(m_szCloakSessionId, m_pConn);
    }

    HX_ASSERT(m_pConn);

    if (m_uContentLen == 32767)
    {
        
        // This is a single-POST request
        if (m_pConn->GetCloakMode() & CLOAK_MODE_MULTIPOST)
        {
            // This is the original single-POST request.  It has been
            // delayed for some reason and the client has already been
            // told to switch to multi-POST mode, so reject it.

            DPRINTF(0x10000000, ("Cloak v1: Delayed single-POST\n"));
            return HXR_UNEXPECTED;
        }
        if (m_pConn->GetPOSTHandlerCount() != 0)
        {
            // This is a duplicate single-POST requst, reject it
            DPRINTF(0x10000000, ("Cloak v1: Duplicate single-POST\n"));
            return HXR_UNEXPECTED;
        }
    }

    m_parse_state = PARSE_DATA;
    AddRef(); // For the list
    m_pConn->AddPOSTHandler(this);

    HX_ASSERT(m_pPendingData == NULL);

    if (uDataLen > 0)
    {
        QueuePendingData(pData, uDataLen);
    }

    // Dispatch to the streamer
    CHXServSocket* pSock = m_pDemux->GetSock();
    pSock->Dispatch(m_pConn->m_iProcNum);
    HX_RELEASE(pSock);

    return HXR_OK;
}


HX_RESULT
CRN1CloakPOSTHandler::HandlePOSTData(IHXBuffer* pBuf)
{
    IHXBuffer* pDecBuf = NULL;
    CBaseCloakGETHandler* pGETHandler = NULL;

    BYTE* pData = (BYTE*)pBuf->GetBuffer();
    UINT32 uDataLen = pBuf->GetSize();

    if (m_bOnClosedCalled)
    {
	return HXR_OK;
    }

    if (m_pConn)
    {
        m_pConn->GetGETHandler(pGETHandler);
    }

    if (!pGETHandler)
    {
        QueuePendingData(pData, uDataLen);

        // m_pGETHandler is guaranteed to not exist here.
        HX_RELEASE(pGETHandler);
        return HXR_OK;
    }

    // We were unable to process a fragment of the previous
    // packet, so append this packet to it and try to process them
    // both as one bigger packet.
    if (m_pPendingData)
    {
        // If we're processing it, it shouldn't be in the queue anymore!
        HX_ASSERT(!m_bProcessingPendingData);

        QueuePendingData(pData, uDataLen);
        ProcessPendingData();
        return HXR_OK;
    }

    HX_ASSERT(pGETHandler);
    pGETHandler->Release();
    pGETHandler = NULL;

    while (!m_bOnClosedCalled && uDataLen)
    {
        BOOL bPostDone = FALSE;
        UINT32 n = decodeBuf(pData, uDataLen, pDecBuf, bPostDone);
        if (n == 0)
        {
            if (uDataLen < MAX_POST_MESSAGE_LENGTH)                 
            {
                QueuePendingData(pData, uDataLen);
            }
            else
            {
                // We're already trying to process a fragment.
                // We don't want to use too much memory, so stop now.
		DPRINTF(0x10000000, ("RN Cloak v1 -- Max POST length exceeded\n"));
                m_pDemux->Close(HXR_FAIL);
                CleanupConn(HXR_FAIL);
            }
            break;
        }
        pData += n;
        uDataLen -= n;

        if (pDecBuf->GetSize() == 1 && pDecBuf->GetBuffer()[0] == HTTP_DONE)
        {
            // Client is done with this cloak connection.  Close all GET
            // and POST requests associated with this GUID.
            CleanupConn(HXR_FAIL);
            break;
        }

        HX_ASSERT(m_pConn);
        m_pConn->OnPOSTData(pDecBuf);
        HX_RELEASE(pDecBuf);

        // if m_pGETHandler is invalid at this point then there is an extra
        // m_pGETHandler->release() somewhere that needs to b found.
        if ((m_pConn->GetCloakMode() & CLOAK_MODE_MULTIPOST)
        &&  bPostDone)
        {
            // Client is in multi-POST mode and it is done with this POST.
            // Close this POST, leaving any other requests alone.
            m_pDemux->Close(HXR_OK);

            // Do not remove from the GUID dict, since other POSTs may follow.
            CleanupConn(HXR_OK);
            break;
        }
    }

    HX_RELEASE(pDecBuf);

    return HXR_OK;
}

HX_RESULT
CRN1CloakPOSTHandler::ParseCloakSessionId(const char* szBuf)
{
    // CloakSessionId for Cloak v1 is a GUID at the begining of the POST data.

    UINT32 n;

    for (n = 0; n < GUID_STR_LENGTH; n++)
    {
        if (szBuf[n] < 32 || szBuf[n] > 127)
        {
            return HXR_UNEXPECTED;
        }
    }
    if (szBuf[GUID_STR_LENGTH] != '\r' || szBuf[GUID_STR_LENGTH + 1] != '\n')
    {
        return HXR_UNEXPECTED;
    }

    if (!m_szCloakSessionId)
    {
        m_szCloakSessionId = new char[GUID_STR_LENGTH + 1];
    }

    memcpy(m_szCloakSessionId, szBuf, GUID_STR_LENGTH);
    m_szCloakSessionId[GUID_STR_LENGTH] = '\0';

    return HXR_OK;
}

/*
 * Decode a Base64 encoded message sent by the client.  We know the message
 * boundaries because incoming Base64 data is always NUL terminated by the
 * client.  This allows us to detect the funky 'D' and 'h' stuff.
 *
 * Returns the number of bytes used from the data or zero on failure.
 */
UINT32
CRN1CloakPOSTHandler::decodeBuf(BYTE* pData, UINT32 uDataLen, IHXBuffer*& rpDecBuf, BOOL& rbPostDone)
{
    BYTE* pCur;
    BYTE* pDataEnd;

    BYTE* pDecData;
    INT32 iDecLen;

    rpDecBuf = NULL;
    rbPostDone = FALSE;

    pDataEnd = pData+uDataLen;
    for (pCur = pData+4; pCur < pDataEnd; pCur += 4)
    {
        if (*pCur == 0)
        {
            uDataLen = pCur-pData;
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

            if (pDecData[iDecLen - 1] == HTTP_POSTDONE)
            {               
                if (pDecData[0] == '$' && iDecLen >= 4)
                {
                    // This message contains an interleaved pkt, so it is one of:
                    // 1. Interleaved pkt whose last char is the same as HTTP_POSTDONE 
                    // 2. Interleaved pkt followed by single char HTTP_POSTDONE

                    // At this level we do not observe aggregation, so no other
                    // scenarios are valid.
                    UINT32 ulInterleavedLen = ((UINT16)pDecData[2] << 8) + (UINT16)pDecData[3];
                    HX_ASSERT(ulInterleavedLen >= (iDecLen - 5));

                    if (ulInterleavedLen < (iDecLen - 4))
                    {
                        // Actual HTTP_POSTDONE. Not part of the interleaved data.
                        iDecLen--;
                        rbPostDone = TRUE;
                    }
                }
                else 
                {
                    // Not interleaved data. Actual HTTP_POSTDONE.
                    iDecLen--;
                    rbPostDone = TRUE;
                }
            }
            rpDecBuf->SetSize(iDecLen);

            return uDataLen + 1;
        }
    }
    return 0;
}

