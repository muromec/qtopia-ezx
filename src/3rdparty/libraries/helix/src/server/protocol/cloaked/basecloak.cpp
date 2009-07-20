/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: basecloak.cpp,v 1.5 2006/10/19 02:02:26 jrmoore Exp $
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
#include "basecloak.h"

enum
{
    HTTP_CLOAK_OK = 0,
    HTTP_CLOAK_GENERAL_ERROR,
    HTTP_CLOAK_POST_NOT_RECEIVED,
    HTTP_CLOAK_INVALID_GUID
};

CBaseCloakGETHandler::CBaseCloakGETHandler(void) 
: m_nRefCount(0)
, m_pDemux(NULL)
, m_pProc(NULL)
, m_bGotRequest(FALSE)
, m_pConn(NULL)
, m_ulCGSCHandle(0)
, m_bOnClosedCalled(FALSE)
, m_pCloakedGUIDDict(0)
, m_szCloakSessionId(NULL)
{
    // Empty
}

CBaseCloakGETHandler::~CBaseCloakGETHandler(void)
{
    HX_RELEASE(m_pDemux);
    HX_RELEASE(m_pConn);
    HX_VECTOR_DELETE(m_szCloakSessionId);
}

STDMETHODIMP
CBaseCloakGETHandler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXHTTPDemuxResponse*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXHTTPDemuxResponse))
    {
        AddRef();
        *ppvObj = (IHXHTTPDemuxResponse*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CBaseCloakGETHandler::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CBaseCloakGETHandler::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(BOOL)
CBaseCloakGETHandler::AutoDispatch(void)
{
    return FALSE;
}

STDMETHODIMP_(void)
CBaseCloakGETHandler::Init(IHXHTTPDemux* pDemux)
{
    m_pDemux = pDemux;
    m_pDemux->AddRef();
    m_pProc = m_pDemux->GetProc();
    m_pCloakedGUIDDict = m_pProc->pc->cloaked_guid_dict;
}

STDMETHODIMP_(void)
CBaseCloakGETHandler::OnDispatch(void)
{
    AddRef();

    m_pProc = m_pDemux->GetProc();

    if (m_pConn)
    {
        // reference socket is used to provide local/peer addrs and other info
        IHXSocket* pRealSock = m_pDemux->GetSock();
        m_pConn->SetRefSocket(pRealSock);
        HX_RELEASE(pRealSock);

        m_pConn->GETChannelReady(m_pProc);
    }

    Release();
}

STDMETHODIMP_(void)
CBaseCloakGETHandler::OnRequest(HTTPRequestMessage* pMsg)
{
    AddRef();

    CHXServSocket* pSock = NULL;
    CBaseCloakGETHandler* pGETHandler = NULL;

    // Instantiated by derived classes.
    HX_ASSERT(m_pConn);

    // There is already an entry for this GUID.  If it already has a GET
    // connection, reject this one.
    m_pConn->GetGETHandler(pGETHandler);
    if (pGETHandler)
    {
        char szMsg[256];
        sprintf(szMsg, "REJECTING duplicate GET!");
        m_pProc->pc->error_handler->Report(HXLOG_ERR, 0, 0, szMsg, NULL);
        pGETHandler->Release();
        goto EndOnRequestOnError;
    }

    // POST received before GET
    //m_pConn->SetGETHandler(this); //XXX: not in streamer yet...
    m_pConn->OnGETRequest(this);

    // Dispatch to the streamer
    pSock = m_pDemux->GetSock();
    if (pSock)
    {
        pSock->SelectEvents(HX_SOCK_EVENT_WRITE|HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
        pSock->Dispatch(m_pConn->m_iProcNum);
        pSock->Release();
        pSock = NULL;
    }

    Release();
    return;

EndOnRequestOnError:

    m_pDemux->Close(HXR_FAIL);
    CleanupConn();
    Release();
}

STDMETHODIMP_(void)
CBaseCloakGETHandler::OnResponse(HTTPResponseMessage* pMsg)
{
    DPRINTF(0x10000000, ("HTTP: Unexpected response\n"));
    m_pDemux->Close(HXR_FAIL);
}

STDMETHODIMP_(void)
CBaseCloakGETHandler::OnData(IHXBuffer* pBuf)
{
    DPRINTF(0x10000000, ("HTTP: Unexpected data\n"));
    m_pDemux->Close(HXR_FAIL);
}

STDMETHODIMP_(void)
CBaseCloakGETHandler::OnWriteReady(void)
{
    if (m_pConn)
    {
        m_pConn->OnGETWriteReady();
    }
}

STDMETHODIMP_(void)
CBaseCloakGETHandler::OnClosed(void)
{
    AddRef();

    if (m_bOnClosedCalled)
    {
        Release();
        return;
    }
    m_bOnClosedCalled = TRUE;

    if (m_pProc && !m_ulCGSCHandle)
    {
        CloakGETShutdownCallback* pCGSC = new CloakGETShutdownCallback(this);
        m_ulCGSCHandle = m_pProc->pc->engine->schedule.enter(0.0, pCGSC);
    }
    else if (!m_ulCGSCHandle)
    {
        Shutdown();
    }

    CleanupConn();

    Release();
}

void
CBaseCloakGETHandler::CleanupConn()
{
    if (!m_pConn)
    {
        return;
    }

    if (m_szCloakSessionId)
    {
        CloakConn* pConn = 0;

        m_pCloakedGUIDDict->remove(m_szCloakSessionId, pConn);
        HX_ASSERT(pConn == m_pConn || !pConn);

        // GUID dict addrefs pConn on remove.
        HX_RELEASE(pConn);

        delete [] m_szCloakSessionId;
        m_szCloakSessionId = 0;
    }

    m_pConn->Close();
    m_pConn->SetGETHandler(NULL);
    m_pConn->CloseAllPOSTHandlers();

    // Now release our reference to it.
    m_pConn->Release();
    m_pConn = NULL;
}

STDMETHODIMP_(UINT32)
CBaseCloakGETHandler::GetFeatureFlags()
{
    return HTTP_FEATURE_IGNORE_CONTENT_LENGTH;
}


void
CBaseCloakGETHandler::Shutdown(void)
{
    // Prevent premature destruction (before releasing member objects)
    AddRef(); 

    if (m_pDemux)
    {
        m_pDemux->Close(HXR_OK);
        m_pDemux->Release();
        m_pDemux= NULL;
    }

    CleanupConn();
    
    // Destructor call OK.
    Release();
}

HX_RESULT
CBaseCloakGETHandler::SendData(IHXBuffer* pBuf)
{
    if (m_pDemux)
    {
        return m_pDemux->SendData(pBuf);
    }
    return HXR_FAIL;
}

void
CBaseCloakGETHandler::sendPostStatus(Byte status)
{
    CloakedHTTPResponse msg;
    IHXBuffer* pBuf;
    BYTE* pData;
    UINT32 uLen;

    msg.status = status;

    pBuf = new ServerBuffer(TRUE);
    pBuf->SetSize(msg.static_size());
    pData = pBuf->GetBuffer();
    uLen = pBuf->GetSize();
    msg.pack(pData, uLen);
    m_pDemux->SendData(pBuf);
    HX_RELEASE(pBuf);
}

void
CBaseCloakGETHandler::sendResponseHeader(const char* szContentType)
{
    IHXSocket* pSock;
    IHXSockAddr* pAddr;
    IHXBuffer* pAddrBuf = NULL;
    HTTPResponseMessage* pResponse;

    pSock = m_pDemux->GetSock();
    // if the demux has been closed
    if (!pSock)
	return;

    pSock->GetLocalAddr(&pAddr);
    pAddr->GetAddr(&pAddrBuf);
    pResponse = new HTTPResponseMessage;
    pResponse->setErrorCode("200");
    pResponse->setErrorMsg("OK");
    pResponse->addHeader("Server", "RMServer 1.0");
    pResponse->addHeader("Expires", "Mon, 18 May 1974 00:00:00 GMT"); //XXX sic
    pResponse->addHeader("Pragma", "no-cache");
    pResponse->addHeader("x-server-ipaddress",
                             (const char*)pAddrBuf->GetBuffer());
    pResponse->addHeader("Content-type", szContentType);
    m_pDemux->SendMessage(pResponse);

    delete pResponse;
    HX_RELEASE(pAddrBuf);
    HX_RELEASE(pAddr);
    HX_RELEASE(pSock);
}


// ***************************************************************************

CBaseCloakPOSTHandler::CBaseCloakPOSTHandler(void) 
: m_nRefCount(0)
, m_pDemux(NULL)
, m_pProc(NULL)
, m_bGotRequest(FALSE)
, m_pConn(NULL)
, m_bProcessingPendingData(FALSE)
, m_uContentLen(0)
, m_pPendingData(NULL)
, m_ulCPSCHandle(0)
, m_bOnClosedCalled(FALSE)
, m_pCloakedGUIDDict(NULL)
, m_szCloakSessionId(NULL)
{
    // Empty
}

CBaseCloakPOSTHandler::~CBaseCloakPOSTHandler(void)
{
    HX_RELEASE(m_pPendingData);
    HX_RELEASE(m_pDemux);
    HX_RELEASE(m_pConn);
    HX_VECTOR_DELETE(m_szCloakSessionId);
}

STDMETHODIMP
CBaseCloakPOSTHandler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXHTTPDemuxResponse*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXHTTPDemuxResponse))
    {
        AddRef();
        *ppvObj = (IHXHTTPDemuxResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CBaseCloakPOSTHandler::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CBaseCloakPOSTHandler::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(BOOL)
CBaseCloakPOSTHandler::AutoDispatch(void)
{
    return FALSE;
}

STDMETHODIMP_(void)
CBaseCloakPOSTHandler::Init(IHXHTTPDemux* pDemux)
{
    m_pDemux = pDemux;
    m_pDemux->AddRef();
    m_pProc = m_pDemux->GetProc();
    m_pCloakedGUIDDict = m_pProc->pc->cloaked_guid_dict;
}

STDMETHODIMP_(void)
CBaseCloakPOSTHandler::OnDispatch(void)
{
    AddRef();

    m_pProc = m_pDemux->GetProc();
    ProcessPendingData();

    Release();
}


STDMETHODIMP_(void)
CBaseCloakPOSTHandler::OnResponse(HTTPResponseMessage* pMsg)
{
    DPRINTF(0x10000000, ("HTTP: Unexpected response\n"));
    m_pDemux->Close(HXR_FAIL);
}

STDMETHODIMP_(void)
CBaseCloakPOSTHandler::OnWriteReady(void)
{
    // nothing to do for POST
}

STDMETHODIMP_(void)
CBaseCloakPOSTHandler::OnClosed(void)
{
    AddRef();

    if (m_bOnClosedCalled)
    {
        Release();
        return;
    }
    m_bOnClosedCalled = TRUE;
    BeginClose();
    Release();
}

// when the GET handler calls CloakConn::RemoveAllPOSTHandlers(), it in turn ends
// up calling BeginClose() which as u can see prevents a deadlock by
// disallowing a call to CloakConn::RemovePOSTHandler()
void
CBaseCloakPOSTHandler::BeginClose(void)
{
    AddRef();

    if (m_pProc && !m_ulCPSCHandle)
    {
        CloakPOSTShutdownCallback* pCPSC = new CloakPOSTShutdownCallback(this);
        m_ulCPSCHandle = m_pProc->pc->engine->schedule.enter(0.0, pCPSC);
    }
    else if (!m_ulCPSCHandle)
    {
        Shutdown();
    }

    Release();
}

STDMETHODIMP_(UINT32)
CBaseCloakPOSTHandler::GetFeatureFlags()
{
    return HTTP_FEATURE_IGNORE_CONTENT_LENGTH;
}

void
CBaseCloakPOSTHandler::QueuePendingData(BYTE* pData, UINT32 ulDataLen)
{
    if (!m_pPendingData)
    {
        m_pPendingData = new ServerBuffer(TRUE);
        m_pPendingData->SetSize(ulDataLen);
        memcpy(m_pPendingData->GetBuffer(), pData, ulDataLen);
    }
    else
    {
        // This probably shouldn't happen
        IHXBuffer* pNewBuf = new ServerBuffer(TRUE);
        pNewBuf->SetSize(ulDataLen + m_pPendingData->GetSize());
        memcpy(pNewBuf->GetBuffer(),
                m_pPendingData->GetBuffer(),
                m_pPendingData->GetSize());
        memcpy(pNewBuf->GetBuffer() + m_pPendingData->GetSize(),
                pData, ulDataLen);
        m_pPendingData->Release();
        m_pPendingData = pNewBuf;
    }
}

void
CBaseCloakPOSTHandler::ProcessPendingData(void)
{
    AddRef();

    m_bProcessingPendingData = TRUE;
    HX_ASSERT(m_pConn != NULL);

    IHXBuffer* pBuf = m_pPendingData;
    m_pPendingData = NULL;
    if (pBuf != NULL)
    {
        OnData(pBuf);
        pBuf->Release();
    }

    m_bProcessingPendingData = FALSE;

    Release();
}

void
CBaseCloakPOSTHandler::Shutdown(void)
{
    if (m_pDemux)
    {
        m_pDemux->Close(HXR_OK);
        HX_RELEASE(m_pDemux);
    }

    CleanupConn(HXR_STREAM_DONE);
}

void 
CBaseCloakPOSTHandler::CleanupConn(HX_RESULT status)
{
    if (!m_pConn)
    {
        return;
    }

    // Fatal error, so kill the entire cloaking session.
    if (m_szCloakSessionId 
    &&  status != HXR_OK)
    {
        CloakConn* pConn = NULL;
        m_pCloakedGUIDDict->remove(m_szCloakSessionId, pConn);

        HX_ASSERT(pConn == m_pConn || !pConn);

        if (pConn)
        {
            pConn->Close();
            pConn->Release();
            pConn = NULL;
        }

    }

    HX_VECTOR_DELETE(m_szCloakSessionId);

    m_pConn->RemovePOSTHandler(this);

    m_pConn->Release();
    m_pConn = NULL;
}

CBaseCloakGETHandler::CloakGETShutdownCallback::~CloakGETShutdownCallback()
{
    m_pCGH->Release();
    m_pCGH = NULL;
}

STDMETHODIMP
CBaseCloakGETHandler::CloakGETShutdownCallback::Func()
{
    m_pCGH->Shutdown();
    return HXR_OK;
}

CBaseCloakPOSTHandler::CloakPOSTShutdownCallback::~CloakPOSTShutdownCallback()
{
    m_pCPH->Release();
    m_pCPH = NULL;
}

STDMETHODIMP
CBaseCloakPOSTHandler::CloakPOSTShutdownCallback::Func()
{
    m_pCPH->Shutdown();
    return HXR_OK;
}
