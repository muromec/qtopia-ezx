/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpprot.cpp,v 1.43 2006/11/23 13:44:06 srao Exp $
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
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxprefs.h"
#include "hxstring.h"
#include "debug.h"
#include "cbqueue.h"
#include "sio.h"
#include "hxnet.h"
#include "client.h"
#include "hxstrutl.h"
#include "hxrquest.h"
#include "url.h"
#include "urlutil.h"
#include "timerep.h"
#include "plgnhand.h"
#include "dict.h"
#include "chxpckts.h"
#include "hxreg.h"
#include "server_info.h"
#include "servpq.h"
#include "bwcalc.h"
#include "config.h"
#include "servbuffer.h"
#include "server_version.h"
#include "globals.h"
#include "netbyte.h"

#include "base_errmsg.h"
#include "http.h"
#include "http_demux.h"
#include "httpprot.h"
#include "httppars.h"
#include "servhttpmsg.h"

#include "hxstats.h"
#include "server_stats.h"

#include "hxclientprofile.h"
#include "client_profile_mgr.h"

const int MAX_HTTP_MSG = 32768;

// static defines

HTTPProtocol::HTTPProtocol(void)
    : m_nRefCount(0)
    , m_pDemux(NULL)
    , m_pPendingBuf(NULL)
    , m_pHTTP(NULL)
    , m_head_request(FALSE)
    , m_posting(FALSE)
    , m_max_post_data(64000)
    , m_need_posthandler(FALSE)
    , m_bKeepAlive(FALSE)
    , m_pRequest(NULL)
    , m_pbwcBandwidthReporter(NULL)
    , m_pRegistryKey(NULL)
    , m_content_len(0)
    , m_pQoSInfo(NULL)
    , m_bNeedCountDecrement(FALSE)
{
    // Empty
}

HTTPProtocol::~HTTPProtocol(void)
{
    HX_RELEASE(m_pRequest);

    HX_VECTOR_DELETE(m_pRegistryKey);
    // This happens in HTTPProtocol::Shutdown() (so don't forget to call it!)
    // HX_DELETE(m_pHTTP);
    HX_RELEASE(m_pPendingBuf);
    HX_RELEASE(m_pQoSInfo);
}

STDMETHODIMP
HTTPProtocol::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXHTTPDemuxResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXHTTPDemuxResponse))
    {
        AddRef();
        *ppvObj = (IHXHTTPDemuxResponse*)this;
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
HTTPProtocol::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(UINT32)
HTTPProtocol::Release(void)
{
    if (InterlockedDecrement(&m_nRefCount) > 0)
    {
        return m_nRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP_(HTTP_TYPE)
HTTPProtocol::GetHandlerType(void)
{
    return HTTP_TYPE_NORMAL;
}

STDMETHODIMP_(BOOL)
HTTPProtocol::AutoDispatch(void)
{
    return TRUE;
}

void
HTTPProtocol::Init(IHXHTTPDemux* pDemux)
{
    HX_ASSERT(m_pDemux == NULL);
    m_pDemux = pDemux;
    m_pDemux->AddRef();
}

STDMETHODIMP_(void)
HTTPProtocol::OnDispatch(void)
{
    if (!m_pDemux)
	return;

    m_pDemux->CreateClient();

    Client* pClient = m_pDemux->GetClient(); //NB: Not AddRef()'d in hxprot.h
    IHXSocket* pSock = m_pDemux->GetSock();

    m_pHTTP = new HTTP(pSock, this, pClient);

    mime_type_dict = pClient->proc->pc->mime_type_dict;

    IHXSockAddr* pLocalAddr = NULL;
    IHXSockAddr* pPeerAddr = NULL;

    pSock->GetLocalAddr(&pLocalAddr);
    pSock->GetPeerAddr(&pPeerAddr);

    /*
     * XXXaak:
     * for some reason sometimes the peer addr is NULL
     * and this caused a crash in the client when it tried to 
     * dereference it.
     */
    if (!pPeerAddr || !pLocalAddr)
    {
	pSock->Release();
	pSock = NULL;
	return;
    }
    pClient->init_stats(pLocalAddr, pPeerAddr, FALSE);

    HX_RELEASE(pPeerAddr);
    HX_RELEASE(pLocalAddr);

    m_pbwcBandwidthReporter = new BWCalculator(pClient->proc, FALSE);
    m_pbwcBandwidthReporter->AddRef();
    m_pbwcBandwidthReporter->CommitPendingBandwidth();

    pClient->proc->pc->server_info->IncrementTCPTransportCount(pClient->proc);
    m_bNeedCountDecrement = TRUE;

    DPRINTF(0x04000000, ("%6.1lu: 2. HTTPProt for client(%p)\n",
            pClient->conn_id, pClient));

    HX_RELEASE(pSock);
    /*
     * Since there is only one session with an HTTP request, initialize the
     * registry here
     */

    init_stats();
}

STDMETHODIMP_(void)
HTTPProtocol::OnRequest(HTTPRequestMessage* pMsg)
{
    if (!m_pDemux)
	return;

    switch(pMsg->tag())
    {
    case HTTPMessage::T_GET:
        handleMsg((HTTPGetMessage*)pMsg);
        break;

    case HTTPMessage::T_POST:
        handleMsg((HTTPPostMessage*)pMsg);
        break;

    case HTTPMessage::T_HEAD:
        handleMsg((HTTPHeadMessage*)pMsg);
        break;

    default:
        HTTPResponseMessage* pMsg = makeResponseMessage("501");
        DPRINTF(D_INFO, ("HTTP: Unrecognized message\n"));
        if (SUCCEEDED(sendResponse(pMsg)))
	    SetStatus(501);
        HX_DELETE(pMsg);
        break;
    }
}

STDMETHODIMP_(void)
HTTPProtocol::OnResponse(HTTPResponseMessage* pMsg)
{
    DPRINTF(D_INFO, ("HTTP: Unexpected response\n"));
}

STDMETHODIMP_(void)
HTTPProtocol::OnData(IHXBuffer* pBuf)
{
    if (m_pHTTP)
	m_pHTTP->http_post_data((char*)pBuf->GetBuffer(), pBuf->GetSize());
}

STDMETHODIMP_(void)
HTTPProtocol::OnWriteReady(void)
{
    // If we have a pending buffer, write it
    if (m_pPendingBuf != NULL)
    {
        IHXBuffer* pBuf = m_pPendingBuf;
        m_pPendingBuf = NULL;
        sendData(pBuf);
        pBuf->Release();
        // The write should always succeed
        HX_ASSERT(m_pPendingBuf == NULL);
    }
    if (m_pHTTP)
    {
        m_pHTTP->WriteReady();
    }
}

STDMETHODIMP_(void)
HTTPProtocol::OnClosed(void)
{
    /*
     * schedule the shutdown because if this is not done then HTTPFileResponse
     * object dereferences the HTTP::m_pSock object directly (among other
     * member vars) and that caused a crash. since the HTTP object is not
     * a COM object (AddRef/Release) this is how its done. this all seems to
     * happen within the same callstack so scheduling a callback for shutdown
     * will work.
     */
    HX_ASSERT(m_pDemux);

    Client* pClient = m_pDemux->GetClient(); //NB: Not AddRef()'d in hxprot.h

    if (pClient) 
    {
        // Do this now since the client object will be gone by the time we get to
        // ShutdownCallback::Func().
        if (m_bNeedCountDecrement)
        {
            pClient->proc->pc->server_info->DecrementTCPTransportCount(pClient->proc);
            m_bNeedCountDecrement = FALSE;
        }
        
        if (pClient->m_bIsAProxy)
        {
            pClient->proc->pc->client_stats_manager->RemoveClient(
                        pClient->m_pStats->GetID(), pClient->proc);
        }

        ShutdownCallback* pSCb = new ShutdownCallback(this);
        pClient->proc->pc->engine->schedule.enter(0.0, pSCb);
    }
}

STDMETHODIMP_(UINT32)
HTTPProtocol::GetFeatureFlags()
{
    return HTTP_FEATURE_NONE;
}


STDMETHODIMP
HTTPProtocol::PSSProfileReady(HX_RESULT ulStatus, IHXClientProfile* pInfo,
                              IHXBuffer* pRequestId, IHXBuffer* pRequestURI,
                              IHXValues* pRequestHeaders)
{
    Client* pClient = m_pDemux->GetClient();

    HX_RESULT rc = HXR_OK;

    HX_ASSERT(pClient && pClient->get_client_stats());
    HX_ASSERT(m_pRequest);

    if (!pRequestURI || !pRequestHeaders)
    {
        return HXR_ABORT;
    }

    UINT16 uProfileWarning;
    IHXSessionStats* pStats = pClient->get_client_stats()->GetSession(1);

    if (SUCCEEDED(ulStatus))
    {
        // 201: Content selection applied
        uProfileWarning = 201;
        pStats->SetClientProfileInfo(pInfo);
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
                ERRMSG(pClient->proc->pc->error_handler,
                "Error parsing profile for client (%d)\n", pClient->conn_id);
            }
            else
            {
                ERRMSG(pClient->proc->pc->error_handler,
                "Error retrieving profile for client (%d)\n",
                 pClient->conn_id);
            }
        }
    }

    IHXBuffer* pXwapPrfHdr = NULL;
    if(SUCCEEDED(pRequestHeaders->GetPropertyCString("x-wap-profile",
                                                      pXwapPrfHdr)))
    {
        pStats->SetXWapProfileStatus(uProfileWarning);
        pXwapPrfHdr->Release();
    }

    HX_RELEASE(pStats);

    // complete the request
    m_pHTTP->http_start(m_pRequest, NULL);

    return rc;
}

BOOL
HTTPProtocol::statComplete(HX_RESULT status,
                           UINT32 ulSize,
                           UINT32 ulCreationTime,
                           const char* mimeType)
{
    Client* pClient = m_pDemux->GetClient();
    HTTPResponseMessage* pMsg = NULL;

    if (status != HXR_OK && status != HXR_NOT_AUTHORIZED)
    {
        if (status == HXR_INVALID_OPERATION)
        {
            pMsg = makeResponseMessage("403");
            SetStatus(403);
        }
        else if (status == HXR_INVALID_FILE)
        {
            pMsg = makeResponseMessage("415");
            SetStatus(415);
        }
        else
        {
            pMsg = makeResponseMessage("404");
            SetStatus(404);
        }
        sendResponse(pMsg);
        Shutdown(HXR_INVALID_FILE);        
        HX_DELETE(pMsg);
        return FALSE;
    }

    if(m_major_version < 1)
    {
        if(m_head_request)
        {
            // There are no headers for a 0.9 request, but ok whatever..
            Shutdown(HXR_FAIL);
        }
        HX_DELETE(pMsg);
        return !m_head_request;
    }

    INT32 nStatus;
    if(status == HXR_NOT_AUTHORIZED)
    {
        pMsg = makeResponseMessage("401");
        nStatus = 401;
    }
    else
    {
        pMsg = makeResponseMessage("200");
        nStatus = 200;
    }

    IHXSessionStats* pSessionStats = pClient->get_client_stats()->
                                     GetSession(1);

    UTCTimeRep timeNow;
    pMsg->addHeader("Date",   timeNow.asRFC1123String());
    pMsg->addHeader("Server", "RealServer");

    BOOL m_bContentTypeFound = FALSE;

    if (m_pRequest)
    {
        DPRINTF(D_INFO, ("HTTPP::statComplete -- has request\n"));
        IHXValues* pResponseHeaders = 0;

        if (m_pRequest->GetResponseHeaders(pResponseHeaders) == HXR_OK &&
            pResponseHeaders)
        {
            HX_RESULT result;
            MIMEHeader* pMimeHeader;
            const char* pName;
            IHXBuffer* pValue;

            m_bKeepAlive = FALSE;

            result = pResponseHeaders->GetFirstPropertyCString(pName, pValue);

            while (result == HXR_OK)
            {
                if
                (
                    strstr(pName, "Connection") &&
                    strstr((const char*)pValue->GetBuffer(), "keep-alive")
                )
                {
                    // Don't stop reading
                    m_bKeepAlive = TRUE;
                }
                else if (strcasecmp(pName, "Content-Type") == 0)
                {
                    m_bContentTypeFound = TRUE;
                    if (pClient->use_registry_for_stats())
                    {
                        SetMimeTypeInRegistry((const char*)pValue->GetBuffer());
                    }
                }

                pMimeHeader = new MIMEHeader(pName);
                pMimeHeader->addHeaderValue((const char*)pValue->GetBuffer());
                pMsg->addHeader(pMimeHeader);
                pValue->Release();

                result = pResponseHeaders->GetNextPropertyCString(pName,
                                                                  pValue);
            }

            if(m_bKeepAlive)
            {
                // change the state of the HTTP class
                m_pHTTP->m_state = HS_KeepAliveInit;
            }

            pResponseHeaders->Release();
        }
    }

    if (!m_bContentTypeFound)
    {
        const char* pMimeType = mimeType;
        if(pMimeType || getMimeType(m_fileExt, pMimeType))
        {
            DPRINTF(D_INFO, ("Content-Type: %s\n", pMimeType));
            pMsg->addHeader("Content-Type", pMimeType);
            if (pClient->use_registry_for_stats())
            {
                SetMimeTypeInRegistry(pMimeType);
            }
        }
    }

    UTCTimeRep fileTime(ulCreationTime);
    pMsg->addHeader("Last-Modified", fileTime.asRFC1123String());

    if(ulSize > 0)
    {
        char lenStr[30];
        sprintf(lenStr, "%ld", ulSize);
        pMsg->addHeader("Content-Length", lenStr);

        if (pClient->use_registry_for_stats())
        {
            char str[512];
            sprintf(str, "%s.FileSize", m_pRegistryKey);
            pClient->proc->pc->registry->AddInt(str, ulSize, pClient->proc);
        }

        pSessionStats->SetFileSize(ulSize);
    }

    // Addx-wap-profile-warning
    UINT16 uProfileWarning = pSessionStats->GetXWapProfileStatus();
    if(uProfileWarning)
    {
        char szStatus[6];
        sprintf(szStatus, "%u", uProfileWarning);
        MIMEHeader* pMimeHeader = new MIMEHeader("x-wap-profile-warning");
        if(pMimeHeader)
        {
            pMimeHeader->addHeaderValue(szStatus);
            pMsg->addHeader(pMimeHeader);
        }
    }

    HX_RELEASE(pSessionStats);

    if (SUCCEEDED(sendResponse(pMsg)))
    {
        SetStatus(nStatus);
    }

    if(m_head_request)
    {
        Shutdown(HXR_FAIL);
    }

    HX_DELETE(pMsg);
    return !m_head_request;
}

void
HTTPProtocol::handleRedirect(IHXBuffer* pURL)
{
    HTTPResponseMessage* pMsg = NULL;

    if (pURL)
    {
        pMsg = makeResponseMessage("302");
        UTCTimeRep timeNow;
        int nLen = strlen(ServerVersion::ProductName()) +
                   strlen(ServerVersion::MajorMinorString());
        char* szBuf = new char[nLen + 2];
        sprintf(szBuf, "%s %s", ServerVersion::ProductName(),
                ServerVersion::MajorMinorString());
        pMsg->addHeader("Server", szBuf);
        pMsg->addHeader("Date",   timeNow.asRFC1123String());
        pMsg->addHeader("Location", (char*)pURL->GetBuffer());
        delete[] szBuf;
        if (SUCCEEDED(sendResponse(pMsg)))
        {
            SetStatus(302);
        }
        Shutdown(HXR_OK);
    }
    else
    {
        pMsg = makeResponseMessage("404");
        if (SUCCEEDED(sendResponse(pMsg)))
        {
            SetStatus(404);
        }
        Shutdown(HXR_FAIL);
    }
    HX_DELETE(pMsg);
}

//
// called by HTTPContext::ReadDone
//
void
HTTPProtocol::handleRead(BYTE* pData, UINT32 uDataLen, int done)
{
    if (pData && (uDataLen > 0))
    {
        // Report HTTP in Server output
        HXAtomicAddUINT32(g_pBytesServed, uDataLen);

        m_pbwcBandwidthReporter->BytesSent(uDataLen);

        IHXBuffer* pBuf = new ServerBuffer(TRUE);
        pBuf->SetSize(uDataLen);
        memcpy(pBuf->GetBuffer(), pData, uDataLen);

        sendData(pBuf);
	if (m_pPendingBuf == NULL && m_pHTTP)
	{
	    m_pHTTP->WriteReady();
	}

        HX_RELEASE(pBuf);
    }
    if (done)
    {
        Shutdown(HXR_OK);
    }
}

HTTPResponseMessage*
HTTPProtocol::makeResponseMessage(const char* pErrNo)
{
    HTTPResponseMessage* pMsg = new ServerHTTPResponseMessage;
    pMsg->setErrorCode(pErrNo);

    UINT32 ulErrNo = atoi(pErrNo);

    switch (ulErrNo)
    {

    //XXXDPL can this be a <200 check instead?
    case 200:
    case 201:
    case 202:
    case 204:
    case 302:

        break;

    default:

        char errMsg[1024];
        char errMsgHdr[256];
        char errMsgBody[768];

        // The response object now handles setting the error text
        // in setErrorCode().
        const char* errTxt = pMsg->errorMsg();

        const char* hdr = "Content-type: text/html\n"
                    "Content-length: %d\n";
        const char* body = "\r\n"
                    "<HTML>\n"
                    "<TITLE> %s </TITLE>\n"
                    "<BODY BGCOLOR=\"#FFFFFF\">\n"
                    "<FONT FACE=\"Arial,Helvetica,Geneva\" SIZE=-1>\n"
                    "<H2> %s </H2>\n"
                    "</BODY>\n"
                    "</HTML>\n";
        sprintf(errMsgBody, body, errTxt, errTxt);
        sprintf(errMsgHdr, hdr, strlen(errMsgBody));
        sprintf(errMsg, "%s%s", errMsgHdr, errMsgBody);

        pMsg->setContent
        (
            (unsigned char*)errMsg,
            strlen(errMsg)
        );

    }

    return pMsg;
}

HX_RESULT
HTTPProtocol::sendResponse(HTTPResponseMessage* pMsg)
{
    HX_RESULT hxr = m_pDemux->SendMessage(pMsg);
    if (FAILED(hxr))
    {
        if (m_pDemux)
        {
            DPRINTF(D_INFO, ("%lu: failed to send HTTP response message\n",
                             m_pDemux->GetClient()->conn_id));
        }
    }
    else
    {
        UINT32 ulSendBytes = (pMsg->contentLength() + m_pQoSInfo->GetBytesSent());
        if (ulSendBytes > 0)
        {
            m_pQoSInfo->SetBytesSent((UINT64)ulSendBytes);
        }
    }
    return hxr;
}

void 
HTTPProtocol::Shutdown(HX_RESULT status)
{
    if (m_pDemux)
    {
        Client* pClient = m_pDemux->GetClient(); // Demux does not AddRef client.

        // pClient is NULL by now if the client closes the connection (we receive a 
        // call to OnClosed()).
        if (pClient)
        {
            if (m_bNeedCountDecrement)
            {
                pClient->proc->pc->server_info->DecrementTCPTransportCount(pClient->proc);
                m_bNeedCountDecrement = FALSE;
            }
            pClient->OnClosed(status);
        
            if (pClient->m_bIsAProxy)
            {
                pClient->proc->pc->client_stats_manager->RemoveClient(
                             pClient->m_pStats->GetID(), pClient->proc);
            }
        }

        m_pDemux->Close(status);
    }

    HX_RELEASE(m_pbwcBandwidthReporter);
    HX_RELEASE(m_pDemux);
    HX_DELETE(m_pHTTP);

    /*

    if(pClient)
    {
#ifdef PAULM_CLIENTAR
        REL_NOTIFY(pClient, 15);
#endif
        if (pClient->m_bIsAProxy)
        {
            pClient->proc->pc->client_stats_manager->RemoveClient(
                        pClient->m_pStats->GetID(), pClient->proc);
        }
        pClient->Release();
        pClient = NULL;
    }

    m_pDemux->Close(status);
    */
}

int
HTTPProtocol::sendAlert(const char* pSessionID, StreamError err)
{
    // Make sure the player disconnects
    Shutdown(HXR_SERVER_ALERT);
    return 0;
}

int
HTTPProtocol::sendAlert(const char* pSessionID, IHXBuffer* pAlert)
{

    // Make sure the player disconnects
    Shutdown(HXR_SERVER_ALERT);
    return 0;
}

void
HTTPProtocol::SetStatus(UINT32 nStatus)
{
    Client* pClient = m_pDemux->GetClient();

    if (pClient && pClient->use_registry_for_stats())
    {
        SetStatusInRegistry(nStatus);
    }

    IHXSessionStats* pSessionStats = pClient->get_client_stats()->GetSession(1);
    pSessionStats->SetStatus(nStatus);
    HX_RELEASE(pSessionStats);
}

void
HTTPProtocol::sendRequest(HTTPRequestMessage* pMsg)
{
    HX_ASSERT(FALSE);
}

void
HTTPProtocol::sendData(IHXBuffer* pBuf)
{
    HX_ASSERT(m_pPendingBuf == NULL);

    HX_RESULT hxr = m_pDemux->SendData(pBuf);
    if (FAILED(hxr))
    {
        if (hxr == HXR_SOCK_WOULDBLOCK)
        {
            m_pPendingBuf = pBuf;
            m_pPendingBuf->AddRef();
        }
        else
        {
#ifdef _DEBUG
            // SendData() could lead to a call to OnClose() if an error occured
            // thereby calling Shutdown() and releasing m_pDemux
            if (m_pDemux)
            {
                Client* pClient = m_pDemux->GetClient();
                if (pClient)
                {
                    DPRINTF(D_INFO, ("%lu: failed to send HTTP message\n",
                        pClient->conn_id));
                }
            }
#endif
            Shutdown(HXR_OK);
        }
    }
    UINT64 ulSendLength = ((UINT64)pBuf->GetSize() + m_pQoSInfo->GetBytesSent());
    m_pQoSInfo->SetBytesSent(ulSendLength);
}

int
HTTPProtocol::init_request(HTTPRequestMessage* pMsg)
{
    Client* pClient = m_pDemux->GetClient();

    DPRINTF(D_INFO, ("HTTP: Req msg handler, url: %s, version: %d.%d\n",
                     pMsg->url(), pMsg->majorVersion(), pMsg->minorVersion()));
    URL url(pMsg->url(), strlen(pMsg->url()));

    if (!pClient->get_client_stats())
    {
        return -1;
    }

    IHXSessionStats* pSessionStats = pClient->get_client_stats()->GetSession(1);
    HX_ASSERT(pSessionStats);

    char str[512];

    m_fileExt = url.ext;
    m_major_version = (UINT32)pMsg->majorVersion();
    m_minor_version = (UINT32)pMsg->minorVersion();

    if (!m_pRequest)
    {
        // Unescape the url
        // XXXJDG Need system wide URL parsing overhaul,
        // this still won't work right for certain query params.
        const char* szFullURL = pMsg->url();
        UINT32 ulURLSize = strlen(szFullURL);
        NEW_FAST_TEMP_STR(pDecURL, 1024, ulURLSize + 1);
        DecodeURL((const BYTE*)szFullURL, ulURLSize, pDecURL);

        m_pRequest = new CHXRequest();
        m_pRequest->AddRef();
        m_pRequest->SetURL(pDecURL);

        DELETE_FAST_TEMP_STR(pDecURL);
    }

    IHXValues* pRequestHeaders = new CHXHeader();
    pRequestHeaders->AddRef();
    getRFC822Headers(pMsg, pRequestHeaders);

    HXRegistry* hxreg;

    HX_RESULT rc = HXR_OK;

    hxreg = new HXRegistry(pClient->proc->pc->registry, pClient->proc);
    hxreg->AddRef();


    IHXBuffer* pBuffer = new ServerBuffer(TRUE);
    pBuffer->Set((BYTE*)url.full, strlen(url.full)+1);

    if (pClient->use_registry_for_stats())
    {
        // Set URL value in Server Registry
        sprintf(str, "%s.URL", m_pRegistryKey);
        hxreg->AddStr(str, pBuffer);

        sprintf(str, "%s.PlayerRequestedURL", m_pRegistryKey);
        hxreg->AddStr(str, pBuffer);
    }


    pSessionStats->SetURL(pBuffer);
    pSessionStats->SetPlayerRequestedURL(pBuffer);

    HX_RELEASE(pBuffer);

    // Set User-Agent value in Server Stats
    IHXBuffer* pName = NULL;
    IHXBuffer* pValue = NULL;
    ServerRegistry* pRegistry = pClient->proc->pc->registry;

    if(pClient->use_registry_for_stats())
    {
        pRegistry->GetPropName(pClient->GetRegId(ClientRegTree::CLIENT),
            pName, pClient->proc);
    }

    // Set HTTP request method in stats.
    const char *method = "";

    switch(pMsg->tag())
    {

        case HTTPMessage::T_GET:
        {
            method = (const char*)"GET";
        }
        break;
        case HTTPMessage::T_POST:
        {
            method = (const char*)"POST";
        }
        break;
        case HTTPMessage::T_HEAD:
        {
            method = (const char*)"HEAD";
        }
        break;
        default:
        {
        // Should have been caught earlier.
            HX_ASSERT(0);
        }
        break;
    }

    pValue = new ServerBuffer(TRUE);

    // If no/unknown method, don't put in anything.
    if (pValue)
    {
        pValue->Set((UCHAR*)method, strlen(method) + 1);

        if (pClient->use_registry_for_stats() && pName)
        {
            sprintf(str, "%-.400s.HTTP-Method", (const char*)pName->GetBuffer());
            hxreg->AddStr(str, pValue);
        }
        pClient->get_client_stats()->SetRequestMethod(pValue);
        HX_RELEASE(pValue);
    }


    if (HXR_OK == pRequestHeaders->GetPropertyCString("User-Agent",
        pValue))
    {
        if (pClient->use_registry_for_stats() && pName)
        {
            sprintf(str, "%-.400s.User-Agent", (const char*)pName->GetBuffer());
            hxreg->AddStr(str, pValue);
        }
        pClient->get_client_stats()->SetUserAgent(pValue);
        HX_RELEASE(pValue);
    }

    if (pClient->proc->pc->config->GetInt(pClient->proc, "config.DisableClientGuid") == 1)
    {
            char str[64];
            IHXBuffer* pGUID = new ServerBuffer(TRUE);
            const char p0GUID[] = "00000000-0000-0000-0000-000000000000";
            pGUID->Set((UINT8 *)p0GUID, sizeof(p0GUID));

            if (pClient->use_registry_for_stats() && pName)
            {
                sprintf(str, "%-.400s.GUID", (const char*)pName->GetBuffer());
                hxreg->AddStr(str, pGUID);
            }
            pClient->get_client_stats()->SetGUID(pGUID);
            pGUID->Release();
    }
    else
    {
        if (HXR_OK == pRequestHeaders->GetPropertyCString("GUID", pValue))
        {
            if (pClient->use_registry_for_stats() && pName)
            {
                sprintf(str, "%-.400s.GUID", (const char*)pName->GetBuffer());
                hxreg->AddStr(str, pValue);
            }
            pClient->get_client_stats()->SetGUID(pValue);
            HX_RELEASE(pValue);
        }
    }

    if (HXR_OK == pRequestHeaders->GetPropertyCString("ClientID", pValue))
    {
        if (pClient->use_registry_for_stats() && pName)
        {
            sprintf(str, "%-.400s.ClientID", (const char*)pName->GetBuffer());
            hxreg->AddStr(str, pValue);
        }
        pClient->get_client_stats()->SetClientID(pValue);
        HX_RELEASE(pValue);
    }

    if (HXR_OK == pRequestHeaders->GetPropertyCString("Host", pValue))
    {
        IHXBuffer* pHost = NULL;
        UCHAR* pStart = pValue->GetBuffer();
        UCHAR* pEnd = pStart;
        UCHAR* pBuf = NULL;

        pBuf = new UCHAR[pValue->GetSize()];

        // Host may contain a port, so strip it off
        while (*pEnd && *pEnd != ':')
        {
            pEnd++;
        }
        strncpy((char*)pBuf, (char*)pStart, pEnd - pStart);
        pBuf[pEnd-pStart] = '\0';

        pHost = new ServerBuffer(TRUE);
        pHost->Set(pBuf, pEnd - pStart + 1);

        if (pClient->use_registry_for_stats())
        {
            sprintf(str, "%s.Host", m_pRegistryKey);
            hxreg->AddStr(str, pHost);
        }
        pSessionStats->SetHost(pHost);

        HX_VECTOR_DELETE(pBuf);
        HX_RELEASE(pHost);
        HX_RELEASE(pValue);
    }

    HX_RELEASE(pName);

    INT32 iPort = 0;
    char pBuf[30];

    pBuffer = pSessionStats->GetInterfaceAddr();
    if(pBuffer)
    {
        pRequestHeaders->SetPropertyCString("PNMAddr", pBuffer);
        HX_RELEASE(pBuffer);
    }

    hxreg->GetIntByName("config.RTSPPort", iPort);
    pRequestHeaders->SetPropertyULONG32("RTSPPort", (ULONG32)iPort);

    if (pClient->use_registry_for_stats())
    {
        pBuffer = new ServerBuffer(TRUE);
        sprintf(pBuf, "%lu", pClient->get_registry_conn_id());
        pBuffer->Set((UCHAR*)pBuf, strlen(pBuf) + 1);
        pRequestHeaders->SetPropertyCString("ConnID", pBuffer);
        HX_RELEASE(pBuffer);
    }

    pBuffer = new ServerBuffer(TRUE);
    sprintf(pBuf, "%lu", pClient->get_client_stats_obj_id());
    pBuffer->Set((UCHAR*)pBuf, strlen(pBuf) + 1);
    pRequestHeaders->SetPropertyCString("ClientStatsObjId", pBuffer);
    HX_RELEASE(pBuffer);

    pBuffer = new ServerBuffer(TRUE);
    pBuffer->Set((UCHAR*)"1", 2);
    // For HTTP protocol, these are both the same for now.
    pRequestHeaders->SetPropertyCString("SessionNumber", pBuffer);
    pRequestHeaders->SetPropertyCString("SessionStatsObjId", pBuffer);
    HX_RELEASE(pBuffer);


    HX_RELEASE(pSessionStats);
    hxreg->Release();

    /*
     * Add in the LocalPort var for IHXFileRestrict
     */
    IHXSocket* pSock = m_pDemux->GetSock();
    IHXSockAddr* pLocalAddr = NULL;
    char szPort[5+1];
    IHXBuffer* pLocalPortBuf = NULL;
    pSock->GetLocalAddr(&pLocalAddr);
    sprintf(szPort, "%hu", pLocalAddr->GetPort());
    pLocalPortBuf = new ServerBuffer(TRUE);
    pLocalPortBuf->Set((UCHAR*)szPort, strlen(szPort)+1);

    pRequestHeaders->SetPropertyCString("LocalPort", pLocalPortBuf);

    HX_RELEASE(pLocalPortBuf);
    HX_RELEASE(pLocalAddr);
    HX_RELEASE(pSock);

    if (!pMsg->getHeaderValue("Content-Length", m_content_len) && m_posting)
    {
        HTTPResponseMessage* pMsg = makeResponseMessage("400");
        // bad request
        m_content_len = 0;
        if (SUCCEEDED(sendResponse(pMsg)))
        {
	    SetStatus(400);
        }

        HX_RELEASE(pRequestHeaders);
        HX_DELETE(pMsg);
        return -1;
    }
    else if (m_posting)
    {
        pRequestHeaders->SetPropertyULONG32("PostDataLength",
                (ULONG32)m_content_len);
        if (m_content_len > m_max_post_data)
        {
            m_need_posthandler = TRUE;
        }
    }

    rc = m_pRequest->SetRequestHeaders(pRequestHeaders);

    pRequestHeaders->Release();

    if (HXR_OK != rc)
    {
        ERRMSG(pClient->proc->pc->error_handler,
               "Unable to set request headers for HTTP protocol\n");
        return -1;
    }
    return 0;
}

int
HTTPProtocol::handleMsg(HTTPGetMessage* pMsg)
{
    int res = 0;
    if ((res = init_request(pMsg)) != 0 || !m_pDemux)
    {
        return res;
    }

    if (FAILED(GetClientProfile()))
    {
        return -1;
    }

    return 1;  // Stop reading input
}

int
HTTPProtocol::handleMsg(HTTPPostMessage* pMsg)
{
    CHXString   msgStr;
    int res = 0;

    m_posting = TRUE;
    if ((res = init_request(pMsg)) != 0)
    {
        return res;
    }

#ifdef _DEBUG
    MIMEHeader* hdr = pMsg->getFirstHeader();
    while(hdr)
    {
        msgStr = "";
        hdr->asString(msgStr);
        DPRINTF(D_PROT, ("%s: %s",hdr->name(), (const char *)msgStr));
        hdr = pMsg->getNextHeader();
    }
#endif

    m_pHTTP->m_pRequest = m_pRequest;
    m_pHTTP->m_pRequest->AddRef();
    m_pHTTP->http_post((char*)pMsg->url(), m_content_len,
        m_need_posthandler);
    m_pHTTP->http_start(m_pRequest, NULL);

    return 0;
}

int
HTTPProtocol::handleMsg(HTTPHeadMessage* pMsg)
{
    DPRINTF(D_INFO, ("HTTP: HEAD msg handler, url: %s\n", pMsg->url()));
    m_head_request = TRUE;
    URL url(pMsg->url(), strlen(pMsg->url()));
    m_fileExt = url.ext;
    m_major_version = (UINT32)pMsg->majorVersion();
    m_minor_version = (UINT32)pMsg->minorVersion();

    // Unescape the url
    // XXXJDG Need system wide URL parsing overhaul,
    // this still won't work right for certain query params.
    const char* szFullURL = pMsg->url();
    UINT32 ulURLSize = strlen(szFullURL);
    NEW_FAST_TEMP_STR(pDecURL, 1024, ulURLSize + 1);
    DecodeURL((const BYTE*)szFullURL, ulURLSize, pDecURL);

    m_pRequest = new CHXRequest();
    m_pRequest->AddRef();
    m_pRequest->SetURL(pDecURL);

    DELETE_FAST_TEMP_STR(pDecURL);

    m_pHTTP->http_start(m_pRequest, NULL);
    return 1;  // Stop reading input
}

BOOL
HTTPProtocol::getMimeType(const char* extension,
                          const char*& mime_type)
{
    Dict_entry* ent;

    if(mime_type_dict == NULL)
    {
            return FALSE;
    }

    ent = mime_type_dict->find(extension);
    if(!ent)
    {
        // We should ALWAYS be able to find a default
        ent = mime_type_dict->find("*");
        HX_ASSERT(ent);
    }

    mime_type = (const char*)ent->obj;

    return TRUE;
}

int
HTTPProtocol::post(IHXBuffer* pBuf)
{
    if (HXR_OK != m_pHTTP->http_post_data((char*)pBuf->GetBuffer(), pBuf->GetSize()))
    {
        return -1;
    }
    return 0;
}

void
HTTPProtocol::getRFC822Headers(HTTPMessage* pMsg, IHXValues* pRFC822Headers)
{
    MIMEHeader* pHeader = pMsg->getFirstHeader();

    while (pHeader)
    {
        MIMEHeaderValue* pHeaderValue;

        /*
         * XXX...There is way too much memcpy() going on here
         */

        pHeaderValue = pHeader->getFirstHeaderValue();

        CHXString HeaderString;

        while (pHeaderValue)
        {
            CHXString TempString;

            pHeaderValue->asString(TempString);
            HeaderString += TempString;
            pHeaderValue = pHeader->getNextHeaderValue();
            if (pHeaderValue)
            {
                HeaderString += ", ";
            }
        }

        IHXBuffer* pBuffer = new ServerBuffer(TRUE);
        pBuffer->Set((const BYTE*)((const char*)HeaderString),
                                   HeaderString.GetLength()+1);
        pRFC822Headers->SetPropertyCString(pHeader->name(), pBuffer);
        pBuffer->Release();

        pHeader = pMsg->getNextHeader();
    }
}

/*
 * This init_stats() function is the equivalent of the Player::Session
 * init_stats() call
 */
void
HTTPProtocol::init_stats()
{
    Client* pClient = m_pDemux->GetClient();

    if (pClient->use_registry_for_stats())
    {
        init_registry();
    }

    if (pClient->m_bIsAProxy)
    {
        ClientStats* pClientStats = new ClientStats(pClient->proc);
        pClient->proc->pc->client_stats_manager->AddClient(pClientStats, pClient->proc);
        pClient->m_pStats = pClientStats;
        pClient->m_pStats->AddRef();
        pClient->set_client_stats_obj_id(pClientStats->GetID());
    }
    else
    {
        pClient->update_protocol_statistics_info(PLAYER_CLIENT);
    }


    SessionStats* pSessionStats = new SessionStats();
    pClient->get_client_stats()->AddSession(pSessionStats);
    m_pQoSInfo = new HTTPQoSTranAdaptInfo();
    m_pQoSInfo->AddRef();
    pSessionStats->SetQoSTransportAdaptationInfo(
        (IHXQoSTransportAdaptationInfo*)m_pQoSInfo);

    // This should match
    HX_ASSERT(pSessionStats->GetID() == 1);

    IHXSocket* pSock = m_pDemux->GetSock();

    IHXSockAddr* pLocalAddr = NULL;
    IHXBuffer* pLocalAddrBuf = NULL;
    pSock->GetLocalAddr(&pLocalAddr);
    pLocalAddr->GetAddr(&pLocalAddrBuf);

    pSessionStats->SetInterfaceAddr(pLocalAddrBuf);

    HX_RELEASE(pLocalAddrBuf);
    HX_RELEASE(pLocalAddr);

    HX_RELEASE(pSock);

    // Read the max post data now
    // By checking at start up we get on-the-fly for free

    INT32 ulMax;
    if (HXR_OK == pClient->proc->pc->registry->GetInt("config.MaxPostDataLen",
        &ulMax, pClient->proc))
    {
        m_max_post_data = ulMax;
    }
}

void
HTTPProtocol::init_registry()
{
    Client* pClient = m_pDemux->GetClient();

    UINT32 ulSessionRegID = pClient->GetRegId(ClientRegTree::CLIENT);
    ServerRegistry* registry = pClient->proc->pc->registry;

    if (!ulSessionRegID)
    {
        return;
    }

    if (pClient->GetRegId(ClientRegTree::SESSION))
    {
        INT32 pCount;
        IHXBuffer* pName;

        if (HXR_OK == registry->GetPropName(pClient->GetRegId(ClientRegTree::SESSION),
                                            pName, pClient->proc) &&
            HXR_OK == registry->GetInt(pClient->GetRegId(ClientRegTree::SESSION_COUNT),
                                       &pCount, pClient->proc))
        {
            char str[512];

            /*
             * There is only one session for HTTP
             */

            ASSERT(pCount == 0);

            m_pRegistryKey = new char[512];
            sprintf(m_pRegistryKey, "%-.400s.1", (const char*)pName->GetBuffer());
            registry->AddComp(m_pRegistryKey, pClient->proc);
            registry->SetInt(pClient->GetRegId(ClientRegTree::SESSION_COUNT), 1,
                             pClient->proc);
            pName->Release();

            /*
             * There is only one stream and no transports
             */

            sprintf(str, "%s.StreamCount", m_pRegistryKey);
            registry->AddInt(str, 1, pClient->proc);
            sprintf(str, "%s.Stream", m_pRegistryKey);
            registry->AddComp(str, pClient->proc);
            sprintf(str, "%s.Stream.1", m_pRegistryKey);
            registry->AddComp(str, pClient->proc);
            sprintf(str, "%s.TransportCount", m_pRegistryKey);
            registry->AddInt(str, 0, pClient->proc);
        }
    }
}

void
HTTPProtocol::SetMimeTypeInRegistry(const char* pMimeType)
{
    Client* pClient = m_pDemux->GetClient();

    if (!pClient->use_registry_for_stats())
    {
        return;
    }

    char str[512];
    IHXBuffer* pBuffer = new ServerBuffer(TRUE);
    pBuffer->Set((Byte*)pMimeType, strlen(pMimeType)+1);
    sprintf(str, "%s.Stream.1.MimeType", m_pRegistryKey);
    pClient->proc->pc->registry->AddStr(str, pBuffer, pClient->proc);
    pBuffer->Release();
}

void
HTTPProtocol::SetStatusInRegistry(const INT32 nStatus)
{
    Client* pClient = m_pDemux->GetClient();

    char str[512];
    sprintf(str, "%s.Status", m_pRegistryKey);
    if(!pClient->proc->pc->registry->AddInt(str, nStatus, pClient->proc))
    {
        pClient->proc->pc->registry->SetInt(str, nStatus, pClient->proc);
    }
}

HX_RESULT
HTTPProtocol::GetClientProfile()
{
    Client* pClient = m_pDemux->GetClient();
    Process* pProc = pClient->proc;

    HX_ASSERT(pClient && pClient->get_client_stats());
    HX_ASSERT(m_pRequest);

    HX_RESULT rc = HXR_OK;
    IHXClientProfile* pProfile = NULL;
    IHXBuffer* pURLBuff = new ServerBuffer(TRUE);
    IHXSessionStats* pStats = pClient->get_client_stats()->GetSession(1);
    HX_ASSERT(pStats);

    if(pURLBuff)
    {
        const char* pURL;
        IHXValues* pRequestHeaders = NULL;
        if(SUCCEEDED(m_pRequest->GetURL(pURL)))
        {
            pURLBuff->Set((UCHAR*)pURL, strlen(pURL) + 1);
        }
        m_pRequest->GetRequestHeaders(pRequestHeaders);

        // Send NULL session ID buff since we don't know it and don't need it
        pProc->pc->client_profile_manager->GetPSSProfile(this, pProfile,
            pStats, NULL, pURLBuff, pRequestHeaders);

        HX_RELEASE(pRequestHeaders);
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }
    //... continues in PSSProfileReady

    HX_RELEASE(pURLBuff);
    HX_RELEASE(pStats);

    return rc;
}

HTTPProtocol::ShutdownCallback::~ShutdownCallback()
{
    m_pHTTPP->Release();
    m_pHTTPP = NULL;
}

STDMETHODIMP
HTTPProtocol::ShutdownCallback::Func()
{
    m_pHTTPP->Shutdown(HXR_OK);
    return HXR_OK;
}
