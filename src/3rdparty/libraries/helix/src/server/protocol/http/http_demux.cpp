/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: http_demux.cpp,v 1.24 2006/11/23 13:44:06 srao Exp $
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
#include "hxnet.h"
#include "client.h"
#include "hxstrutl.h"
#include "url.h"
#include "urlutil.h"
#include "servreg.h"
#include "defslice.h"

#include "base_errmsg.h"
#include "hxbuffer.h" //XXX
#include "hxsbuffer.h"

#include "http_demux.h"

#include "http.h"
#include "httpprot.h"
#include "httppars.h"

#include "rn1cloak.h"
#include "qtcloak.h"

#if defined(HELIX_FEATURE_SERVER_WMT_MMS)
#include "wmt_httpprot.h"
#endif

#ifdef HELIX_FEATURE_SERVER_CLOAKV2
#include "rn1cloakfallback.h"
#endif //defined HELIX_FEATURE_SERVER_CLOAKV2

const int MAX_HTTP_MSG = 32768;

CHTTPDemux::CHTTPDemux(void) :
    HXProtocol(),
    m_nRefCount(0),
    m_pFragBuf(NULL),
    m_pParser(NULL),
    m_uContentRemain(0),
    m_pResponse(NULL),
    m_pSavedMessage(NULL),
    m_ReadState(DEMUX_READ_MSG),
    m_bClosed(FALSE),
    m_ulMsgLen(0)
{
    m_pParser = new HTTPParser;
}

CHTTPDemux::~CHTTPDemux(void)
{
    if (!m_bClosed)
    {
        m_proc->pc->error_handler->Report(HXLOG_DEBUG, 0, 0,
                "CHTTPDemux deleted before closing!", 0);
    }
    Close(HXR_FAIL);
    HX_DELETE(m_pSavedMessage);
    HX_DELETE(m_pParser);
    HX_RELEASE(m_pSock);
}

STDMETHODIMP
CHTTPDemux::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXHTTPDemux*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXHTTPDemux))
    {
        AddRef();
        *ppvObj = (IHXHTTPDemux*)this;
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

STDMETHODIMP_(UINT32)
CHTTPDemux::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(UINT32)
CHTTPDemux::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP
CHTTPDemux::EventPending(UINT32 uEvent, HX_RESULT status)
{
    if (m_bClosed)
	return HXR_OK;
    HX_RESULT hxr = HXR_OK;

    IHXBuffer* pBuf = NULL;
    switch (uEvent)
    {
    case HX_SOCK_EVENT_READ:
        hxr = m_pSock->Read(&pBuf);
        if (SUCCEEDED(hxr) && pBuf != NULL)
        {
            handleInput(pBuf);
        }
	else if (hxr == HXR_SOCK_CONNRESET)
        {
            Close(HXR_FAIL);
        }
        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_WRITE:
        if (m_pResponse)
        {
            m_pResponse->OnWriteReady();
        }
        break;
    case HX_SOCK_EVENT_CLOSE:
        // Prevent premature destruction (before releasing member objects)
        AddRef();

        if (m_pResponse != NULL)
        {
            m_pResponse->OnClosed();
        }

        if (m_pClient != NULL)
        {
            m_pClient->OnClosed(HXR_OK);
        }

        HX_RELEASE(m_pResponse);
        HX_RELEASE(m_pClient);

        // Destructor call OK.
        Release();
        break;
    case HX_SOCK_EVENT_DISPATCH:
        m_proc = static_cast<CHXServSocket*>(m_pSock)->GetProc();
        m_pResponse->OnDispatch();
        if (m_pSavedMessage)
        {
            DispatchMessage(m_pSavedMessage);
            delete m_pSavedMessage;
            m_pSavedMessage = NULL;
            if (m_pFragBuf != NULL)
            {
                pBuf = m_pFragBuf;
                m_pFragBuf = NULL;
                handleInput(pBuf);
                HX_RELEASE(pBuf);
            }
        }

        break;
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_OK;
}

STDMETHODIMP_(Process*)
CHTTPDemux::GetProc(void)
{
    return m_proc;
}

STDMETHODIMP_(void)
CHTTPDemux::SetProc(Process* proc)
{
    m_proc = proc;
}

STDMETHODIMP_(CHXServSocket*)
CHTTPDemux::GetSock(void)
{
    HX_ADDREF(m_pSock);
    return (CHXServSocket*)m_pSock;
}

STDMETHODIMP
CHTTPDemux::CreateClient(void)
{
    HX_ASSERT(m_pClient == NULL);
    m_pClient = new Client(m_proc);
    m_pClient->AddRef();
    m_pClient->init(HXPROT_HTTP, this);
    return HXR_OK;
}

STDMETHODIMP_(Client*)
CHTTPDemux::GetClient(void)
{
    return m_pClient;
}

STDMETHODIMP
CHTTPDemux::SendMessage(HTTPMessage* pMsg)
{
    HX_RESULT hxr = HXR_OK;
    UINT32 uBufLen = MAX_HTTP_MSG + pMsg->contentLength(); //XXXTDM
    IHXBuffer* pBuf = new CHXBuffer();
    pBuf->AddRef();
    pBuf->SetSize(uBufLen);
    int msgLen = 0;
    pMsg->asString((char*)pBuf->GetBuffer(), msgLen, uBufLen);
    pBuf->SetSize(msgLen);
    hxr = SendData(pBuf);
    HX_RELEASE(pBuf);
    return hxr;
}

STDMETHODIMP
CHTTPDemux::SendData(IHXBuffer* pBuf)
{
    HX_RESULT hxr = HXR_UNEXPECTED;
    if (m_pSock != NULL)
    {
        hxr = m_pSock->Write(pBuf);
    }
    return hxr;
}

STDMETHODIMP_(void)
CHTTPDemux::Close(HX_RESULT status)
{
    if (m_bClosed)
	return;
    m_bClosed = TRUE;

    if (m_pClient != NULL)
    {
        m_pClient->OnClosed(status);
        HX_RELEASE(m_pClient);
    }

    Done(status);

    HX_RELEASE(m_pResponse);
}

void
CHTTPDemux::init(Process* proc, IHXSocket* pSock)
{
    m_proc = proc;

    m_pSock = pSock;
    m_pSock->AddRef();
    m_pSock->SetResponse(this);
    m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
}

void
CHTTPDemux::handleInput(IHXBuffer* pBuf)
{
    UINT32 ulBufLen = 0;
    if (pBuf)
    {
	ulBufLen = pBuf->GetSize();
	if (m_ulMsgLen+ulBufLen > MAX_HTTP_MSG_SIZE)
	{
	    if (m_pSock)
	    {
		IHXSockAddr* pAddr = 0;
		m_pSock->GetPeerAddr(&pAddr);
		if (pAddr)
		{
		    IHXBuffer* pAddrBuf = 0;
		    pAddr->GetAddr(&pAddrBuf);
		    if (pAddrBuf)
		    {
			fprintf(stderr, "W: Large HTTP message (greater than 64K) being received from addr <%s>.\n"
			    "   Possible DOS attack!\n", 
			    (const char *)pAddrBuf->GetBuffer());
			pAddrBuf->Release();
		    }
		    pAddr->Release();
		}
	    }
	    Close(HXR_FAIL);
	    return;
	}
    }
    if (m_pFragBuf == NULL)
    {
        pBuf->AddRef();
    }
    else
    {
        IHXBuffer* pNewBuf = new CHXBuffer();
        pNewBuf->AddRef();
        pNewBuf->SetSize(m_pFragBuf->GetSize() + ulBufLen);
        memcpy(pNewBuf->GetBuffer(), m_pFragBuf->GetBuffer(), m_pFragBuf->GetSize());
	memcpy(pNewBuf->GetBuffer()+m_pFragBuf->GetSize(), pBuf->GetBuffer(), ulBufLen);
        HX_RELEASE(m_pFragBuf);
        pBuf = pNewBuf;
    }

    while (pBuf != NULL && !m_bClosed)
    {
        BYTE* pData = pBuf->GetBuffer();
        UINT32 uDataLen = pBuf->GetSize();
        UINT32 uDataUsed = 0;
        IHXBuffer* pNewBuf = NULL;
        IHXBuffer* pContentBuf = NULL;
        BOOL bFirstRun = FALSE;
	BOOL bMsgTooLarge = FALSE;

        if (m_ReadState == DEMUX_READ_MSG)
        {
            HTTPMessage* pMsg = NULL;
            uDataUsed = uDataLen;
            pMsg = m_pParser->parse((const char*)pData, uDataUsed, bMsgTooLarge);
            if (pMsg == NULL)
            {
		if (bMsgTooLarge)
		{
		    if (pBuf)
			pBuf->Release();
		    if (m_pSock)
		    {
			IHXSockAddr* pAddr = 0;
			m_pSock->GetPeerAddr(&pAddr);
			if (pAddr)
			{
			    IHXBuffer* pAddrBuf = 0;
			    pAddr->GetAddr(&pAddrBuf);
			    if (pAddrBuf)
			    {
				fprintf(stderr, "W: Large amount of HTTP data being received from addr <%s>.\n"
				    "   Possible DOS attack!\n", 
				    (const char *)pAddrBuf->GetBuffer());
				pAddrBuf->Release();
			    }
			    pAddr->Release();
			}
		    }
		    Close(HXR_FAIL);
		    return;
		}
                break;
            }

            // Remove used data from the buffer
            if (uDataUsed == uDataLen)
            {
                HX_RELEASE(pBuf);
            }
            else
            {
                pNewBuf = new CHXStaticBuffer(pBuf, uDataUsed,
                                              uDataLen-uDataUsed);
                pNewBuf->AddRef();
                HX_RELEASE(pBuf);
                pBuf = pNewBuf;
            }

            if (m_pResponse == NULL)
            {
                DetectHandler(pMsg);
                HX_ASSERT(m_pResponse != NULL);
                m_pResponse->Init(this);
                bFirstRun = TRUE;
            }

            if (m_pResponse->GetFeatureFlags() & HTTP_FEATURE_IGNORE_CONTENT_LENGTH)
            {
                // Cloaking V2.
                if (m_pResponse->GetFeatureFlags() & ( HTTP_FEATURE_V11_SUPPORT
                                                     | HTTP_FEATURE_CHUNKED_ENCODING_SUPPORT))
                {
                    m_uContentRemain = 0;

                    CHXString strEncoding = pMsg->getHeaderValue("Transfer-Encoding");

                    if (strEncoding == "chunked")
                    {
                        m_ReadState = DEMUX_READ_DATA;
                    }
                }
                else // Far less strict for non-persistent HTTP/1.0 connections.
                {
                    m_uContentRemain = 0;
                    m_ReadState = DEMUX_READ_DATA;                
                }
            }
            else
            {
                MIMEHeader* pHdr = pMsg->getHeader("Content-Length");

                if (!pHdr)
                {
                    m_uContentRemain = 0;
                }
                else
                {
                    CHXString strLen;
                    pHdr->asString(strLen);
                    int iLen = atoi(strLen);
                    if (iLen < 0 || iLen > 0xffff)
                    {
                        DPRINTF(D_ERROR, ("HTTP: Bad content length %d\n", iLen));
			if (pBuf)
			    pBuf->Release();
                        Close(HXR_FAIL);
                        return;
                    }
                    
                    m_uContentRemain = (UINT32)iLen;
                    m_ReadState = DEMUX_READ_DATA;
                }
            }

            if (bFirstRun && m_pResponse->AutoDispatch())
            {
                m_pSavedMessage = pMsg;
                static_cast<CHXServSocket*>(m_pSock)->Dispatch();
                break;
            }

            DispatchMessage(pMsg);
            delete pMsg;
        }
        else if (m_ReadState == DEMUX_READ_DATA)
        {
            BOOL bEnforceContentLength = FALSE;

            if (m_pResponse->GetFeatureFlags() & HTTP_FEATURE_IGNORE_CONTENT_LENGTH)           
            {
                HX_ASSERT(m_uContentRemain == 0); // This value not used.
                pContentBuf = pBuf;
                pBuf = NULL;
            }
            else
            {
                if (m_uContentRemain >= pBuf->GetSize())
                {
                    pContentBuf = pBuf;
                    m_uContentRemain -= pBuf->GetSize();
                    pBuf = NULL;
                }
                else
                {
                    pContentBuf = new CHXStaticBuffer(pBuf, 0, m_uContentRemain);
                    pContentBuf->AddRef();

                    pNewBuf = new CHXStaticBuffer(pBuf, m_uContentRemain, pBuf->GetSize()-m_uContentRemain);
                    pNewBuf->AddRef();
                    HX_RELEASE(pBuf);
                    pBuf = pNewBuf;

                    m_uContentRemain = 0;
                    m_ReadState = DEMUX_READ_MSG;
                }
            }

            m_pResponse->OnData(pContentBuf);
            HX_RELEASE(pContentBuf);
        }
    }

    if (pBuf != NULL)
    {
        m_pFragBuf = pBuf;
        m_pFragBuf->AddRef();
    }

    HX_RELEASE(pBuf);
}

void
CHTTPDemux::DispatchMessage(HTTPMessage* pMsg)
{
    if (pMsg->tag() == HTTPMessage::T_RESP)
    {
        m_pResponse->OnResponse((HTTPResponseMessage*)pMsg);
    }
    else
    {
        m_pResponse->OnRequest((HTTPRequestMessage*)pMsg);
    }
}

void
CHTTPDemux::DetectHandler(HTTPMessage* pMsg)
{
    INT32 nProxyEnabled = 0;

    if (SUCCEEDED(m_proc->pc->registry->GetInt(REGISTRY_RTSPPROXY_ENABLED, &nProxyEnabled, m_proc)))
    {
        m_pResponse = new HTTPProtocol();
        m_pResponse->AddRef();
        return;
    }

    HTTPMessage::Tag tag = pMsg->tag();
    if (tag == HTTPMessage::T_GET || tag == HTTPMessage::T_POST)
    {
        HTTPRequestMessage* pReq = (HTTPRequestMessage*)pMsg;
        const char* pUrl = pReq->url();
        if (strncmp(pUrl, "/SmpDsBhgRl", 11) == 0)
        {
            if (tag == HTTPMessage::T_GET)
            {
                m_pResponse = new CRN1CloakGETHandler();
                m_pResponse->AddRef();
            }
            else
            {
                m_pResponse = new CRN1CloakPOSTHandler();
                m_pResponse->AddRef();
            }
            return;
        }

#ifdef HELIX_FEATURE_SERVER_CLOAKV2

        CloakV2SignatureEnforcer pEnforcer;

        if (SUCCEEDED(pEnforcer.EnforceCloakSignature(pReq)))
        {
            if (tag == HTTPMessage::T_GET)
            {
                m_pResponse = new CRN1CloakFallbackGETHandler();
                m_pResponse->AddRef();
            }
            else
            {
                m_pResponse = new CRN1CloakFallbackPOSTHandler();
                m_pResponse->AddRef();
            }
            return;
        }

#endif //defined HELIX_FEATURE_SERVER_CLOAKV2

	/*
	 * If the Accept or Content-type header has
	 * application/x-rtsp-tunnelled its a quicktime cloaked connection
	 */
	MIMEHeader* pUserAgentHdr = pMsg->getHeader("User-Agent");
        MIMEHeader* pSessionHdr = pMsg->getHeader("x-sessioncookie");
	if (pSessionHdr)
	{
	    MIMEHeader* pAcceptHdr = pMsg->getHeader("Accept");
	    MIMEHeader* pContentTypeHdr = pMsg->getHeader("Content-type");
	    CHXString strValue;
	    if (pAcceptHdr != NULL)
	    {
		strValue.Empty();
		pAcceptHdr->asString(strValue);
		if (strncmp(strValue, "application/x-rtsp-tunnelled", 28) == 0)
		{
		    if (tag == HTTPMessage::T_GET)
		    {
			m_pResponse = new CQTCloakGETHandler();
			m_pResponse->AddRef();
		    }
		    else 
		    {
			m_pResponse = new CQTCloakPOSTHandler();
			m_pResponse->AddRef();
		    }
		    return;
		}
	    }
	    if (pContentTypeHdr != NULL)
	    {
		strValue.Empty();
		pContentTypeHdr->asString(strValue);
		if (strncmp(strValue, "application/x-rtsp-tunnelled", 28) == 0)
		{
		    if (tag == HTTPMessage::T_POST)
		    {
			m_pResponse = new CQTCloakPOSTHandler();
			m_pResponse->AddRef();
		    }
		    else
		    {
			m_pResponse = new CQTCloakGETHandler();
			m_pResponse->AddRef();
		    }
		    return;
		}
	    }
	    /*
	     * If the User-Agent begins with "QTS " and there is an
	     * x-sessioncookie header, it's a QT cloaking request.
	     */
	    if (pUserAgentHdr != NULL)
	    {
		pUserAgentHdr->asString(strValue);
		if (strncasecmp(strValue, "QTS ", 4) == 0)
		{
		     if (tag == HTTPMessage::T_GET)
		     {
			m_pResponse = new CQTCloakGETHandler();
			m_pResponse->AddRef();
		     }
		     else
		     {
			m_pResponse = new CQTCloakPOSTHandler();
			m_pResponse->AddRef();
		     }
		     return;
		}
	    }
	}
#if defined(HELIX_FEATURE_SERVER_WMT_MMS)
        if (IsWMTHTTP(pUrl, pUserAgentHdr))
        {
            m_pResponse = new WMTHttpProtocol();
            m_pResponse->AddRef();
            return;
        }
#endif
    }
    m_pResponse = new HTTPProtocol();
    m_pResponse->AddRef();
}

bool
CHTTPDemux::IsWMTHTTP(const char* pUrl, MIMEHeader* pUserAgentHdr)
{
    if (pUserAgentHdr == NULL)
    {
        return false;
    }

    CHXString strValue;
    pUserAgentHdr->asString(strValue);
    const char *pStr = (const char *)(strValue);

    const char szWMPlayer[] = "NSPlayer/";
    const char szWMServer[] = "NSServer/";

    if (!IsASXGen(pUrl) &&
        (strncmp(pStr, szWMPlayer, 9) == 0 ||
        strncmp(pStr, szWMServer, 9) == 0))
    {
        return true;
    }

    return false;
}

bool
CHTTPDemux::IsASXGen(const char* pUrl)
{
    IHXBuffer* pBuffer = NULL;
    bool isASXGen = false;

    HX_RESULT status = m_proc->pc->registry->GetStr("config.FSMount.ASX File Generator.MountPoint",
        pBuffer, m_proc);

    if (HXR_OK == status && pBuffer)
    {
        const char* asxgen = (const char*)pBuffer->GetBuffer();
        if (StrNStr(pUrl, asxgen, strlen(pUrl), strlen(asxgen)))
        {
            isASXGen = true;
        }
    }

    HX_RELEASE(pBuffer);
    return isASXGen;
}

