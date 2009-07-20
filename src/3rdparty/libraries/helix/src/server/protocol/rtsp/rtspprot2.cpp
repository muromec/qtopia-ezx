/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspprot2.cpp,v 1.19 2006/12/21 19:04:55 tknox Exp $
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

#include <stdlib.h>
#include <stdio.h>

#include "hxcom.h"

#include "hxccf.h"
#include "ihxpckts.h"
#include "hxengin.h"
#include "hxnet.h"

#include "ihxlist.h"
#include "hxrtsp2.h"

#include "rtspmsg2.h"
#include "rtspprot2.h"

#include "chxpckts.h"
#include "hxbuffer.h"
#include "hxsbuffer.h"

#include "hxslist.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "transport.h"

#ifdef _WINCE
#include <wincestr.h>
#endif

#define DEFAULT_TCP_MSS 1500
#define MAX_RECURSION_LEVEL 300

//XXX: move the mime_* functions to a shared spot
typedef const char* CPCHAR;

#define TAG_ARRAY_GROW  8

/*****************************************************************************
 *
 * CRTSPProtocol
 *
 *****************************************************************************/

CRTSPProtocol::CRTSPProtocol(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_punkContext(NULL),
    m_pCCF(NULL),
    m_pResponse(NULL),
    m_pSock(NULL),
    m_pConsumer(NULL),
    m_pbufFrag(NULL),
    m_cseqSend(1),
    m_cseqRecv(0),
    m_ntagcount(0),
    m_ntagalloc(0),
    m_ptags(NULL),
    m_pFastAlloc(pFastAlloc),
    m_pScheduler(NULL),
    m_pResolver(NULL)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }

    memset(m_ppTran, 0, sizeof(IHXTransport*) * 256);

}

CRTSPProtocol::~CRTSPProtocol(void)
{
    UINT32 n;

    delete[] m_ptags;
    HX_RELEASE( m_pbufFrag );
    HX_RELEASE( m_pConsumer );
    HX_RELEASE( m_pSock );
    for (n = 0; n < 256; n++)
    {
        HX_RELEASE(m_ppTran[n]);
    }
    HX_RELEASE( m_pCCF );
    HX_RELEASE( m_punkContext );
    HX_RELEASE( m_pFastAlloc );
    HX_RELEASE( m_pScheduler );
    HX_RELEASE( m_pResolver );
}

/*** IUnknown methods ***/

STDMETHODIMP
CRTSPProtocol::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPProtocol))
    {
        AddRef();
        *ppvObj = (IHXRTSPProtocol*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CRTSPProtocol::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CRTSPProtocol::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXRTSPProtocol methods ***/

STDMETHODIMP
CRTSPProtocol::Init(IHXRTSPProtocolResponse* pResponse, IUnknown* punkContext)
{
    HX_ASSERT(m_pResponse == NULL && pResponse != NULL);
    HX_ASSERT(m_punkContext == NULL && punkContext != NULL);

    pResponse->AddRef();
    m_pResponse = pResponse;
    punkContext->AddRef();
    m_punkContext = punkContext;

    m_punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    m_punkContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);

    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::GetSocket(REF(IHXSocket*) pSock)
{
    if (m_pSock != NULL)
    {
        m_pSock->AddRef();
    }
    pSock = m_pSock;
    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::SetTransport(BYTE byChan, IHXTransport* pTran)
{
    HX_RELEASE(m_ppTran[byChan]);
    if (pTran != NULL)
    {
        pTran->AddRef();
        // XXX:TDK Kinda evil, but currently safe
        m_ppTran[byChan] = reinterpret_cast<IHXTransport*>(pTran);
    }
    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::Connect(const char* szHost, const char* szPort)
{
    HX_ASSERT(m_pResponse != NULL && m_punkContext != NULL && m_pCCF != NULL);
    HX_ASSERT(m_pSock == NULL && szHost != NULL);

    HX_RESULT hxr = HXR_FAIL;
    IHXNetServices* pNetSvc = NULL;

    //XXXTDM: check error conditions, call m_pResponse->ConnectDone(...)
    hxr = m_punkContext->QueryInterface(IID_IHXNetServices,
                                        (void**)&pNetSvc);
    if (hxr == HXR_OK)
    {
        hxr = pNetSvc->CreateSocket(&m_pSock);
    }
    if (hxr == HXR_OK)
    {
        m_pSock->SetResponse(this);
        hxr = m_pSock->Init(HX_SOCK_FAMILY_INANY,
                            HX_SOCK_TYPE_TCP,
                            HX_SOCK_PROTO_ANY);
    }
    if (hxr == HXR_OK)
    {
        pNetSvc->CreateResolver(&m_pResolver);
        hxr = m_pResolver->Init(this);

        if (SUCCEEDED(hxr))
        {
            if (szHost[0] == '[')
            {
                char* pszNoBraces = NULL;
                pszNoBraces = new_string(szHost+1, strlen(szHost)-2);
                hxr = m_pResolver->GetAddrInfo(pszNoBraces, szPort, NULL);
                HX_VECTOR_DELETE(pszNoBraces);
            }
            else
            {
                hxr = m_pResolver->GetAddrInfo(szHost, szPort, NULL);
            }
        }

        if (FAILED(hxr))
        {
            HX_RELEASE(m_pSock);
        }
    }
    HX_RELEASE(pNetSvc);

    return hxr;
}

STDMETHODIMP
CRTSPProtocol::Accept(IHXSocket* pSock)
{
    HX_ASSERT(pSock != NULL && m_pSock == NULL);
    HX_ASSERT(m_punkContext != NULL && m_pCCF != NULL && m_pResponse != NULL);

    pSock->AddRef();
    m_pSock = pSock;

    pSock->SetResponse(this);
    pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);

    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::Close(void)
{
    if (m_pSock)
    {
        m_pSock->Close();
    }
    HX_RELEASE(m_pSock);
    UINT32 n;

    for (n = 0; n < 256; n++)
    {
        HX_RELEASE(m_ppTran[n]);
    }
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_punkContext);
    HX_RELEASE( m_pResponse );

    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::SendRequest(IHXRTSPRequestMessage* pReq)
{
    HX_RESULT hxr = HXR_OK;
    IHXRTSPConsumer* pCons = NULL;
    IHXRTSPMessage* pMsg = NULL;
    UINT32 cseq = 0;
    IHXBuffer* pbuf = NULL;

    if (m_pSock == NULL)
    {
        hxr = HXR_FAIL;
        goto bail;
    }

    if (pReq->QueryInterface(IID_IHXRTSPConsumer, (void**)&pCons) != HXR_OK)
    {
        hxr = HXR_FAIL;
        goto bail;
    }
    if (pReq->QueryInterface(IID_IHXRTSPMessage, (void**)&pMsg) != HXR_OK)
    {
        hxr = HXR_FAIL;
        goto bail;
    }

    pCons->AsBuffer(pbuf);

    if (m_ntagcount == m_ntagalloc)
    {
        m_ntagalloc += TAG_ARRAY_GROW;
        request_tag* pnewtags = new request_tag[m_ntagalloc];
        memcpy(pnewtags, m_ptags, m_ntagcount * sizeof(request_tag));
        delete[] m_ptags;
        m_ptags = pnewtags;
    }
    m_ptags[m_ntagcount].m_cseq = pMsg->GetCSeq();
    m_ptags[m_ntagcount].m_verb = pReq->GetVerb();
    m_ntagcount++;

    m_pSock->Write(pbuf);

bail:;
    HX_RELEASE(pbuf);
    HX_RELEASE(pMsg);
    HX_RELEASE(pCons);
    return hxr;
}

STDMETHODIMP
CRTSPProtocol::SendResponse(IHXRTSPResponseMessage* pRsp)
{
    HX_RESULT hxr = HXR_OK;
    IHXRTSPConsumer* pCons = NULL;
    IHXBuffer* pbuf = NULL;

    if (m_pSock == NULL)
    {
        hxr = HXR_FAIL;
        goto bail;
    }

    if (pRsp->QueryInterface(IID_IHXRTSPConsumer, (void**)&pCons) != HXR_OK)
    {
        hxr = HXR_FAIL;
        goto bail;
    }

    pCons->AsBuffer(pbuf);

    m_pSock->Write(pbuf);

bail:;
    HX_RELEASE(pbuf);
    HX_RELEASE(pCons);
    return hxr;
}

STDMETHODIMP
CRTSPProtocol::SendPacket(IHXRTSPInterleavedPacket* pPkt)
{
    HX_RESULT hxr = HXR_OK;
    IHXRTSPConsumer* pCons = NULL;
    IHXBuffer* pbuf = NULL;

    if (m_pSock == NULL)
    {
        hxr = HXR_FAIL;
        goto bail;
    }

    if (pPkt->QueryInterface(IID_IHXRTSPConsumer, (void**)&pCons) != HXR_OK)
    {
        hxr = HXR_FAIL;
        goto bail;
    }

    hxr = pCons->AsBuffer(pbuf);
    if (hxr != HXR_OK)
    {
        goto bail;
    }

    m_pSock->Write(pbuf);

bail:;
    HX_RELEASE(pbuf);
    HX_RELEASE(pCons);
    return hxr;
}

/*** IHXResolveResponse methods ***/

STDMETHODIMP
CRTSPProtocol::GetAddrInfoDone(HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    IHXNetServices* pNetSvc = NULL;

    if (status != HXR_OK)
    {
        if (m_pResponse != NULL)
        {
            m_pResponse->OnConnectDone(status);
        }
        HX_RELEASE(m_pSock);
    }
    else
    {
        m_pSock->SelectEvents(HX_SOCK_EVENT_CONNECT | HX_SOCK_EVENT_READ | HX_SOCK_EVENT_CLOSE);
        status = m_pSock->ConnectToAny(nVecLen, ppAddrVec);
        if (status != HXR_OK)
        {
            if (m_pResponse != NULL)
            {
                m_pResponse->OnConnectDone(status);
            }
            HX_RELEASE(m_pSock);
        }
    }

    m_pResolver->Close();
    HX_RELEASE(m_pResolver);
    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::GetNameInfoDone(HX_RESULT status, const char* pszNode, const char* pszService)
{
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

/*** IHXTCPResponse methods ***/

STDMETHODIMP
CRTSPProtocol::EventPending(UINT32 uEvent, HX_RESULT status)
{
    IHXBuffer* pBuf = NULL;
    switch (uEvent)
    {
    case HX_SOCK_EVENT_CONNECT:
        ConnectDone(status);
        break;
    case HX_SOCK_EVENT_READ:
        if (SUCCEEDED(m_pSock->Read(&pBuf)))
        {
            AddRef();
            ReadDone(HXR_OK, pBuf);
            Release();
        }
        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
        if (m_pResponse != NULL)
        {
            m_pResponse->OnClosed(HXR_OK);
        }
        break;
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::ConnectDone(HX_RESULT status)
{
    if (m_pResponse != NULL)
    {
        m_pResponse->OnConnectDone(status);
    }
    if (status == HXR_OK && m_pSock)
    {
        m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
    }

    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::ReadDone(HX_RESULT status, IHXBuffer* pbuf)
{
    if (status != HXR_OK || pbuf == NULL)
    {
        if (m_pResponse != NULL)
        {
            m_pResponse->OnClosed(status);
        }
        Close();
        return HXR_OK;
    }

    if (m_pbufFrag == NULL)
    {
        // No existing buffer fragment so use the new one as-is
        pbuf->AddRef();
        m_pbufFrag = pbuf;
    }
    else
    {
        // We have an existing buffer fragment, so append the new one to it
        IHXBuffer* pbufNew = NULL;
        if (m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pbufNew)
                                                                     != HXR_OK)
        {
            return HXR_OUTOFMEMORY;
        }
        pbufNew->SetSize(m_pbufFrag->GetSize() + pbuf->GetSize());
        memcpy(pbufNew->GetBuffer(), m_pbufFrag->GetBuffer(),
                                                        m_pbufFrag->GetSize());
        memcpy(pbufNew->GetBuffer() + m_pbufFrag->GetSize(), pbuf->GetBuffer(),
                                                              pbuf->GetSize());
        m_pbufFrag->Release();
        m_pbufFrag = pbufNew;
    }

    UINT32 pos = 0;
    UINT32 len = m_pbufFrag->GetSize();
    const char* p = (const char*)m_pbufFrag->GetBuffer();
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
                if (pC->QueryInterface(IID_IHXRTSPConsumer,
                                       (void**)&m_pConsumer) != HXR_OK)
                {
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
                    break;
                }
                if (strncasecmp(p+pos, "RTSP/", 5) == 0)
                {
                    CRTSPResponseMessage* pC =
                        new (m_pFastAlloc) CRTSPResponseMessage(m_pFastAlloc);
                    if (pC->QueryInterface(IID_IHXRTSPConsumer,
                                           (void**)&m_pConsumer) != HXR_OK)
                    {
                        HX_ASSERT(FALSE);
                        return HXR_UNEXPECTED;
                    }
                }
                else
                {
                    CRTSPRequestMessage* pC =
                        new (m_pFastAlloc) CRTSPRequestMessage(m_pFastAlloc);
                    if (pC->QueryInterface(IID_IHXRTSPConsumer,
                                           (void**)&m_pConsumer) != HXR_OK)
                    {
                        HX_ASSERT(FALSE);
                        return HXR_UNEXPECTED;
                    }
                }
            }
        }

        // Feed the packet to the consumer.
        res = m_pConsumer->ReadDone(m_pbufFrag, &pos);
        if (res == RTSP_RES_DONE)
        {
            DispatchMessage();
            HX_RELEASE(m_pConsumer);
            res = RTSP_RES_AGAIN;
        }
        // After we dispatch msg to m_pResponse, some msgs(error msgs, for example)
        // may cause m_pResponse to call ::Close(), which set m_pResponse to NULL.
        // In these cases, we should not parse the remaining buffer further.
        if(!m_pResponse)
        {
            break;
        }
    }
    if (res == RTSP_RES_INVALID)
    {
        Close();
        return HXR_OK;
    }

    if (pos == len)
    {
        // We used all of the data
        HX_RELEASE(m_pbufFrag);
    }
    else
    {
        UINT32 uRemain = m_pbufFrag->GetSize() - pos;

        // Check the max buffer size
        if (uRemain > RTSP_MAX_PENDING_BYTES)
        {
            Close();
            return HXR_OK;
        }

        if (pos > 0)
        {
            // We used some of the data

            IHXBuffer* pbufNew;
            pbufNew = new CHXStaticBuffer(m_pbufFrag, pos, uRemain);

            pbufNew->AddRef();
            m_pbufFrag->Release();
            m_pbufFrag = pbufNew;
        }
        // else we didn't use any data, leave m_pbufFrag alone
    }

    return HXR_OK;
}

STDMETHODIMP
CRTSPProtocol::WriteReady(HX_RESULT status)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CRTSPProtocol::Closed(HX_RESULT status)
{
    Close();
    if (m_pResponse != NULL)
    {
        m_pResponse->OnClosed(status);
    }

    return HXR_OK;
}

/*** IHXThreadSafeMethods methods ***/

UINT32
CRTSPProtocol::IsThreadSafe(void)
{
    return HX_THREADSAFE_METHOD_SOCKET_READDONE;
}

void
CRTSPProtocol::DispatchMessage(void)
{
    IHXRTSPInterleavedPacket*  pPkt = NULL;
    IHXRTSPMessage*            pMsg = NULL;
    IHXRTSPRequestMessage*     pReq = NULL;
    IHXRTSPResponseMessage*    pRsp = NULL;

    HX_ASSERT(m_pConsumer != NULL);

    if (m_pConsumer->QueryInterface(IID_IHXRTSPInterleavedPacket,
                                    (void**)&pPkt) == HXR_OK)
    {
        BYTE byChan;
        IHXBuffer* pPktBuf;
        pPkt->Get(byChan, pPktBuf);
        if (m_ppTran[byChan] != NULL)
        {
            m_ppTran[byChan]->HandlePacket(pPktBuf);
        }
        pPktBuf->Release();
        pPkt->Release();
        return;
    }

    m_pConsumer->QueryInterface(IID_IHXRTSPMessage, (void**)&pMsg);
    if (pMsg == NULL)
    {
        // Should never happen
        HX_ASSERT(FALSE);
        return;
    }

    // WARNING: Any of the m_pResponse->On***() methods can deallocate us prior
    // to returning control to us. Take care when accessing member variables!

    if (m_pConsumer->QueryInterface(IID_IHXRTSPRequestMessage,
                                    (void**)&pReq) == HXR_OK)
    {
        switch (pReq->GetVerb())
        {
        case RTSP_VERB_OPTIONS:
            m_pResponse->OnOptionsRequest(pReq);
            break;
        case RTSP_VERB_DESCRIBE:
            m_pResponse->OnDescribeRequest(pReq);
            break;
        case RTSP_VERB_SETUP:
            m_pResponse->OnSetupRequest(pReq);
            break;
        case RTSP_VERB_PLAY:
            m_pResponse->OnPlayRequest(pReq);
            break;
        case RTSP_VERB_PAUSE:
            m_pResponse->OnPauseRequest(pReq);
            break;
        case RTSP_VERB_ANNOUNCE:
            m_pResponse->OnAnnounceRequest(pReq);
            break;
        case RTSP_VERB_RECORD:
            m_pResponse->OnRecordRequest(pReq);
            break;
        case RTSP_VERB_TEARDOWN:
            m_pResponse->OnTeardownRequest(pReq);
            break;
        case RTSP_VERB_GETPARAM:
            m_pResponse->OnGetParamRequest(pReq);
            break;
        case RTSP_VERB_SETPARAM:
            m_pResponse->OnSetParamRequest(pReq);
            break;
        case RTSP_VERB_REDIRECT:
            m_pResponse->OnRedirectRequest(pReq);
            break;
        default:
            m_pResponse->OnExtensionRequest(pReq);
        }
        pReq->Release();
    }
    else if (m_pConsumer->QueryInterface(IID_IHXRTSPResponseMessage,
                                    (void**)&pRsp) == HXR_OK)
    {
        UINT32 uCounter = 0;
        UINT32 uCSeq = pMsg->GetCSeq();
        for (uCounter = uCounter; uCounter < m_ntagcount; uCounter++)
        {
            if (uCSeq == m_ptags[uCounter].m_cseq)
            {
                switch (m_ptags[uCounter].m_verb)
                {
                case RTSP_VERB_OPTIONS:
                    m_pResponse->OnOptionsResponse(pRsp);
                    break;
                case RTSP_VERB_DESCRIBE:
                    m_pResponse->OnDescribeResponse(pRsp);
                    break;
                case RTSP_VERB_SETUP:
                    m_pResponse->OnSetupResponse(pRsp);
                    break;
                case RTSP_VERB_PLAY:
                    m_pResponse->OnPlayResponse(pRsp);
                    break;
                case RTSP_VERB_PAUSE:
                    m_pResponse->OnPauseResponse(pRsp);
                    break;
                case RTSP_VERB_ANNOUNCE:
                    m_pResponse->OnAnnounceResponse(pRsp);
                    break;
                case RTSP_VERB_RECORD:
                    m_pResponse->OnRecordResponse(pRsp);
                    break;
                case RTSP_VERB_TEARDOWN:
                    m_pResponse->OnTeardownResponse(pRsp);
                    break;
                case RTSP_VERB_GETPARAM:
                    m_pResponse->OnGetParamResponse(pRsp);
                    break;
                case RTSP_VERB_SETPARAM:
                    m_pResponse->OnSetParamResponse(pRsp);
                    break;
                case RTSP_VERB_REDIRECT:
                    m_pResponse->OnRedirectResponse(pRsp);
                    break;
                default:
                    m_pResponse->OnExtensionResponse(pRsp);
                }

                m_ntagcount--;
                if (uCounter < m_ntagcount && m_ntagcount > 0 )
                {
                    memmove (m_ptags+uCounter, m_ptags+(uCounter+1),
                            (m_ntagcount-uCounter) * sizeof(request_tag));
                }
                break;
            }
        }
        pRsp->Release();
    }
    pMsg->Release();
}
