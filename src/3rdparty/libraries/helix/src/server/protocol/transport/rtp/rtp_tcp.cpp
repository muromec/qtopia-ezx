/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtp_tcp.cpp,v 1.14 2007/03/22 19:16:57 tknox Exp $
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
#include "ihxpckts.h"
#include "hxslist.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "ntptime.h"    // Evil order dependency. ntptime.h m/b before rtcputil.h
#include "rtcputil.h"   // takes care of RTCP in RTP mode

#include "rtp_base.h"
#include "hxmarsh.h"

#include "servpckts.h"
#include "source.h"
#include "sink.h"

#include "rtp_tcp.h"

#include "hxheap.h"
#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

ServerRTPTCPTransport::ServerRTPTCPTransport(BOOL bIsSource)
    : ServerRTPBaseTransport(bIsSource)
    , m_pTCPSocket(NULL)
    , m_tcpInterleave((INT8)0xFF)
    , m_pSource (NULL)
    , m_pWouldBlockResponse (NULL)
    , m_bBlocked (FALSE)
{
    m_wrapSequenceNumber = DEFAULT_WRAP_SEQ_NO;
}

ServerRTPTCPTransport::~ServerRTPTCPTransport()
{
    Done();
    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pWouldBlockResponse);
}

STDMETHODIMP
ServerRTPTCPTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketSink))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXWouldBlock))
    {
        AddRef();
        *ppvObj = (IHXWouldBlock*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }

    return ServerRTPBaseTransport::QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(UINT32)
ServerRTPTCPTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
ServerRTPTCPTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

/* IHXWouldBlock */
STDMETHODIMP
ServerRTPTCPTransport::WantWouldBlock (IHXWouldBlockResponse* pResp, UINT32 lId)
{
    if (!pResp)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pWouldBlockResponse);

    m_pWouldBlockResponse = pResp;
    m_pWouldBlockResponse->AddRef();

    return HXR_OK;
}

/* IHXSocketResponse */
STDMETHODIMP
ServerRTPTCPTransport::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_RESULT hRes = HXR_OK;

    switch (uEvent)
    {
    case HX_SOCK_EVENT_WRITE:
        if (!m_bBlocked)
        {
            hRes = HXR_OK;
            break;
        }

        m_bBlocked = FALSE;

        if (m_pWouldBlockResponse)
        {
            m_pWouldBlockResponse->
                WouldBlockCleared(0);
        }
        else if (m_pSource)
        {
            m_pSource->SinkBlockCleared(m_streamNumber);
        }

        hRes = (m_bBlocked) ? HXR_BLOCKED : HXR_OK;
        break;
    case HX_SOCK_EVENT_CLOSE:
	/*
	 * in a RTP via TCP connection two CHXRTSPTranSockets are created one
	 * for RTP transport and one for RTCP transport.
	 *
	 * this sendBye() is going to fail because the socket is closed
	 * but the failure to write out to the CHXRTSPTranSocket generates
	 * a HX_EVENT_SOCK_CLOSE event which will allow for the destruction
	 * of the CHXRTSPTranSocket.
	 *
	 * without this sendBye() failure the RTCP tranport's
	 * CHXRTSPTranSocket leaked and this was the cleanest way to get it to
	 * cleanup.
	 */
	if (m_pRTCPTran)
	    m_pRTCPTran->sendBye();
	break;

    default:
        break;
    }

    return hRes;
}


/* IHXServerPacketSink */
STDMETHODIMP
ServerRTPTCPTransport::SetSource(IHXServerPacketSource* pSource)
{
    if (!pSource)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pSource);
    m_pSource = pSource;
    pSource->AddRef();
    m_pSource->EnableTCPMode();

    HX_ASSERT(m_pTCPSocket);

    IHXWouldBlock* pWouldBlock = NULL;
    if (m_pTCPSocket && SUCCEEDED(m_pTCPSocket->QueryInterface(IID_IHXWouldBlock,
                                                               (void**)&pWouldBlock)))
    {
        HX_VERIFY(SUCCEEDED(pWouldBlock->WantWouldBlock((IHXWouldBlockResponse*)this, 0)));
    }
    HX_RELEASE(pWouldBlock);

    return HXR_OK;
}

STDMETHODIMP
ServerRTPTCPTransport::PacketReady(ServerPacket* pPacket)
{
    if (!pPacket)
    {
        HX_ASSERT(0);
        return HXR_OK;
    }

    UpdatePacketStats(pPacket);

    if (m_bBlocked)
    {
        pPacket->m_bTransportBlocked = TRUE;
        return HXR_BLOCKED;
    }


    return sendPacket((BasePacket*) pPacket);
}

HX_RESULT
ServerRTPTCPTransport::writePacket(IHXBuffer* pBuf)
{
    if (!m_pTCPSocket)
        return HXR_FAIL;

    // need to put $\000[datalen] in front of packet data

    UINT32 dataLen;

    if(!pBuf || (dataLen = pBuf->GetSize()) > 0xffff)
    {
        return HXR_FAIL;
    }

    IHXBuffer* pHeader = NULL;
    m_pCommonClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pHeader);
    BYTE* pHeaderData;

    if(!pHeader)
    {
        return HXR_OUTOFMEMORY;
    }

    pHeader->SetSize(4);
    pHeaderData = pHeader->GetBuffer();

    pHeaderData[0] = '$';
    pHeaderData[1] = m_tcpInterleave;
    putshort(&pHeaderData[2], (UINT16)dataLen);

    HX_RESULT rc = HXR_OK;
    IHXBuffer* pWriteVec [2];

    pWriteVec [0] = pHeader;
    pWriteVec [1] = pBuf;

    rc = m_pTCPSocket->WriteV(2, pWriteVec);
    if (FAILED(rc) || (rc == HXR_SOCK_BUFFERED))
    {
        m_bBlocked = TRUE;

        if (m_pWouldBlockResponse)
        {
            m_pWouldBlockResponse->
                WouldBlock(0);
        }

	rc = HXR_BLOCKED;	
    }

    pHeader->Release();
    return rc;
}

STDMETHODIMP
ServerRTPTCPTransport::Flush()
{
    return HXR_OK;
}

STDMETHODIMP
ServerRTPTCPTransport::SourceDone()
{
    HX_RELEASE(m_pWouldBlockResponse);
    HX_RELEASE(m_pTCPSocket);
    HX_RELEASE(m_pSource);
    return HXR_OK;
}

/*
 * RTP TCP
 */

void
ServerRTPTCPTransport::Done()
{
    ServerRTPBaseTransport::Done();
    HX_RELEASE(m_pTCPSocket);
}


RTSPTransportTypeEnum
ServerRTPTCPTransport::tag()
{
    return RTSP_TR_RTP_TCP;
}

HX_RESULT
ServerRTPTCPTransport::init(IUnknown* pContext,
                      IHXSocket* pSocket,
                      IHXRTSPTransportResponse* pResp)
{
    m_pTCPSocket = pSocket;
    m_pTCPSocket->AddRef();
    m_pResp = pResp;
    m_pResp->AddRef();

    /* Set DiffServ Code Point */
    IHXQoSDiffServConfigurator* pCfg = NULL;
    if (SUCCEEDED(pContext->QueryInterface(IID_IHXQoSDiffServConfigurator, (void**)&pCfg)))
    {
        pCfg->ConfigureSocket(m_pTCPSocket, HX_QOS_DIFFSERV_CLASS_MEDIA);
        HX_RELEASE(pCfg);

    }

    HX_RESULT hresult = Init(pContext);
    if (HXR_OK != hresult)
    {
        return hresult;
    }

    ServerRTPBaseTransport::init();

        HX_ASSERT(m_pTCPSocket);
    return m_pTCPSocket->SetResponse(this);
}

HX_RESULT
ServerRTPTCPTransport::sendPacket(BasePacket* pPacket)
{
    if (m_bDone)
    {
        return HXR_UNEXPECTED;
    }
    HX_ASSERT(m_bActive);

    HX_RESULT theErr;
    if (m_ulPayloadWirePacket!=0)
    {
        IHXBuffer* pSendBuf = NULL;
        theErr = reflectPacket(pPacket, pSendBuf);

        if (HXR_OK == theErr)
        {
            theErr = writePacket(pSendBuf);

            pSendBuf->Release();
        }
        else if (HXR_IGNORE == theErr)
        {
            return HXR_OK;
        }

        return theErr;
    }

    IHXBuffer* pPacketBuf = NULL;

    theErr = makePacket(pPacket, pPacketBuf);

    if (HXR_OK == theErr)
    {
        theErr = writePacket(pPacketBuf);

        /* send SR if necessary */
        if (HXR_OK == theErr && m_pRTCPTran->m_bSendReport &&
            m_pRTCPTran->m_bSendRTCP)
        {
            m_pRTCPTran->sendSenderReport();
            m_pRTCPTran->m_bSendReport = FALSE;
            m_pRTCPTran->scheduleNextReport();
        }
    }

    HX_RELEASE(pPacketBuf);
    return theErr;
}

/*
 *  RTCP TCP
 */

ServerRTCPTCPTransport::ServerRTCPTCPTransport(HXBOOL bIsSource)
    : ServerRTCPBaseTransport(bIsSource)
    , m_pTCPSocket(NULL)
    , m_tcpInterleave((INT8)0xFF)
{
}

ServerRTCPTCPTransport::~ServerRTCPTCPTransport()
{
    Done();
}

void
ServerRTCPTCPTransport::Done()
{
    if (m_bSendBye)
        sendBye();

    HX_RELEASE(m_pTCPSocket);
    HX_RELEASE(m_pDataTransport);
    ServerRTCPBaseTransport::Done();
}

RTSPTransportTypeEnum
ServerRTCPTCPTransport::tag()
{
    return RTSP_TR_RTCP;
}

HX_RESULT
ServerRTCPTCPTransport::init(IUnknown* pContext,
                       IHXSocket* pSocket,
                       ServerRTPTCPTransport* pDataTransport,
                       IHXRTSPTransportResponse* pResp,
                       UINT16 streamNumber)
{
    m_pTCPSocket = pSocket;
    m_pTCPSocket->AddRef();
    m_pDataTransport = pDataTransport;
    m_pDataTransport->AddRef();

    m_pResp = pResp;
    m_pResp->AddRef();

    m_streamNumber = streamNumber;


    /* Set DiffServ Code Point */
    IHXQoSDiffServConfigurator* pCfg = NULL;
    if (SUCCEEDED(pContext->QueryInterface(IID_IHXQoSDiffServConfigurator, (void**)&pCfg)))
    {
        pCfg->ConfigureSocket(m_pTCPSocket, HX_QOS_DIFFSERV_CLASS_CONTROL);
        HX_RELEASE(pCfg);
    }


    HX_RESULT hresult = Init(pContext);
    if(HXR_OK != hresult)
    {
        return hresult;
    }

    ServerRTCPBaseTransport::init();
    ServerRTCPBaseTransport::getProtocolOverhead(m_pTCPSocket);
    return HXR_OK;
}


HX_RESULT
ServerRTCPTCPTransport::streamDone(UINT16 streamNumber,
                             UINT32 uReasonCode /* = 0 */,
                             const char* pReasonText /* = NULL */)
{
    HX_ASSERT(streamNumber == m_streamNumber);
    HX_ASSERT(streamNumber == m_pDataTransport->m_streamNumber);

    if (m_pDataTransport->m_bIsLive && 
        m_pDataTransport->m_pReflectionHandler &&
        m_pDataTransport->m_pReflectionHandler->IsByeSent() == FALSE)
    {
        m_bSendBye = TRUE;
    }

    // this will be called from ServerRTPUDPTransport::streamDone();
    if (m_bSendBye)
    {
        sendBye();
    }
    return HXR_OK;
}

HX_RESULT
ServerRTCPTCPTransport::reflectRTCP(IHXBuffer* pSendBuf)
{
    HX_ASSERT(pSendBuf);
    HX_RESULT theErr = writePacket(pSendBuf);
    if (theErr)
    {
        m_pResp->OnProtocolError(HXR_NET_SOCKET_INVALID);
    }

    return theErr;
}

HX_RESULT
ServerRTCPTCPTransport::sendSenderReport()
{
    HX_RESULT theErr;
    IHXBuffer* pSendBuf = NULL;

    theErr = makeSenderReport(pSendBuf);
    if (HXR_OK == theErr)
    {
        HX_ASSERT(pSendBuf);
        theErr = writePacket(pSendBuf);
        if(theErr)
        {
            m_pResp->OnProtocolError(HXR_NET_SOCKET_INVALID);
        }
    }

    HX_RELEASE(pSendBuf);
    return theErr;
}

HX_RESULT
ServerRTCPTCPTransport::sendBye()
{
    if (!m_bSendBye)
        return HXR_OK;

    HX_RESULT theErr;
    IHXBuffer* pSendBuf = NULL;

    theErr = makeBye(pSendBuf);
    if (HXR_OK == theErr && m_pTCPSocket)
    {
        HX_ASSERT(pSendBuf);
        theErr = writePacket(pSendBuf);
        /*
         * this write will fail if a client initiated a teardown before the end
         * of a clip because by the time we send BYE, a sock is gone!  So, don't
         * worry about return code here.
         */
    }

    HX_RELEASE(pSendBuf);
    return theErr;
}


HX_RESULT
ServerRTCPTCPTransport::writePacket(IHXBuffer* pBuf)
{
    // need to put $\000[datalen] in front of packet data
    HX_ASSERT(pBuf);
    UINT32 dataLen;

    if(!pBuf || (dataLen = pBuf->GetSize()) > 0xffff)
    {
        return HXR_FAIL;
    }

    //XXXTDM: always true, m_tcpInteleave is signed (why?)
    //HX_ASSERT(0xFF != m_tcpInterleave);

    IHXBuffer* pHeader = NULL;
    m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                          (void**)&pHeader);
    BYTE* pHeaderData;

    if(!pHeader)
    {
        return HXR_OUTOFMEMORY;
    }
    pHeader->SetSize(4);
    pHeaderData = pHeader->GetBuffer();

    pHeaderData[0] = '$';
    pHeaderData[1] = (BYTE)m_tcpInterleave;
    putshort(&pHeaderData[2], (UINT16)dataLen);

    HX_RESULT rc = HXR_FAIL;
    if(SUCCEEDED(m_pTCPSocket->Write(pHeader)) &&
       SUCCEEDED(m_pTCPSocket->Write(pBuf)))
    {
        rc = HXR_OK;
    }
    pHeader->Release();

    return rc;
}

