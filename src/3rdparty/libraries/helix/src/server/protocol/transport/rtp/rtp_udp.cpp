/* ***** BEGIN LICENSE BLOCK *****
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
#include "hxassert.h"
#include "debug.h"
#include "hxcom.h"
#include "hxmarsh.h"
#include "hxstrutl.h"
#include "netbyte.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxsbuffer.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "netbyte.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxdeque.h"
#include "hxbitset.h"
#include "timebuff.h"
#include "timeval.h"
#include "tconverter.h"
#include "rtptypes.h"
#include "hxtlogutil.h"
#include "hxqosinfo.h"
#include "hxqossig.h"
#include "hxqos.h"
//#include "hxcorgui.h"
#include "ihx3gpp.h"

#include "ntptime.h"

#include "rtspif.h"
#include "rtsptran.h"
#include "rtp_udp.h"

#include "bufnum.h"     // Evil order dependency. bufnum.h m/b before rtpwrap.h
#include "rtpwrap.h"    // Wrappers for PMC generated base classes
#include "basepkt.h"
#include "hxtbuf.h"
#include "transbuf.h"
#include "hxtick.h"
#include "random32.h"   // random32()
#include "pkthndlr.h"   // in rtpmisc for RTCP routine
#include "rtcputil.h"   // takes care of RTCP in RTP mode
#include "rtspmsg.h"
#include "hxprefs.h"    // IHXPreferences
#include "hxmime.h"
#include "hxcore.h"

#include "hxheap.h"
#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

ServerRTPUDPTransport::ServerRTPUDPTransport(HXBOOL bIsSource, HXBOOL bOldTS)
    : ServerRTPBaseTransport(bIsSource, bOldTS)
    , m_pUDPSocket(NULL)
    , m_pPeerAddr(NULL)
    , m_pMulticastAddr(NULL)
    , m_pMulticastSocket(NULL)
{
    // Empty
}

ServerRTPUDPTransport::~ServerRTPUDPTransport(void)
{
    Done();
}

STDMETHODIMP
ServerRTPUDPTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSourceBandwidthInfo))
    {
        AddRef();
        *ppvObj = (IHXSourceBandwidthInfo*)this;
        return HXR_OK;
    }
    else 
    {
        return ServerRTPBaseTransport::QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
ServerRTPUDPTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
ServerRTPUDPTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}


HX_RESULT
ServerRTPUDPTransport::init(IUnknown* pContext,
                       IHXSocket* pSocket,
                       IHXRTSPTransportResponse* pResp)
{
    m_pResp = pResp;
    m_pResp->AddRef();

    m_pUDPSocket = pSocket;
    m_pUDPSocket->AddRef();    

    /* Set DiffServ Code Point */
    IHXQoSDiffServConfigurator* pCfg = NULL;
    if (SUCCEEDED(pContext->QueryInterface(IID_IHXQoSDiffServConfigurator, (void**)&pCfg)))
    {
        pCfg->ConfigureSocket(m_pUDPSocket, HX_QOS_DIFFSERV_CLASS_MEDIA);
        HX_RELEASE(pCfg);
    }

    HX_RESULT hresult = Init(pContext);
    if(HXR_OK != hresult)
    {
        return hresult;
    }

#ifdef DEBUG
    if (debug_func_level() & DF_DROP_PACKETS)
    {
        m_drop_packets = TRUE;
    }
#endif /* DEBUG */

    ServerRTPBaseTransport::init();

    if (HXR_OK == hresult)
    {
        HX_ASSERT(m_pUDPSocket);
        hresult = m_pUDPSocket->SetResponse(this);
    }
    if (HXR_OK == hresult)
    {
        hresult = m_pUDPSocket->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
    }
    return hresult;
}

STDMETHODIMP
ServerRTPUDPTransport::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_ASSERT(m_pUDPSocket);
    IHXBuffer* pBuf = NULL;
    IHXSockAddr* pFromAddr = NULL;

    switch (uEvent)
    {
    case  HX_SOCK_EVENT_READ:
        if (SUCCEEDED(m_pUDPSocket->ReadFrom(&pBuf, &pFromAddr))
        &&  m_pPeerAddr 
        &&  pFromAddr
        &&  m_pPeerAddr->IsEqualAddr(pFromAddr))
        {
            handlePacket(pBuf);
        }

        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
        HX_ASSERT(FALSE);
        break;
    default:
        HX_ASSERT(FALSE);
    }

    HX_RELEASE(pFromAddr);

    return HXR_OK;
}

ServerRTCPUDPTransport::ServerRTCPUDPTransport(HXBOOL bIsSource)
    : ServerRTCPBaseTransport(bIsSource)
    , m_pUDPSocket(NULL)
    , m_pPeerAddr(NULL)
    , m_pMulticastSocket(NULL)
    , m_pMulticastAddr(NULL)
{
    // Empty
}

ServerRTCPUDPTransport::~ServerRTCPUDPTransport()
{
    Done();
}

STDMETHODIMP
ServerRTCPUDPTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSSignalSourceResponse))
    {
        AddRef();
        *ppvObj = (IHXQoSSignalSourceResponse*)this;
        return HXR_OK;
    }
    else if (ServerRTCPBaseTransport::QueryInterface(riid, ppvObj) == HXR_OK)
    {
        HX_ASSERT(!"Missing IID");
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
ServerRTCPUDPTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
ServerRTCPUDPTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
ServerRTCPUDPTransport::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_ASSERT(m_pUDPSocket);
    IHXBuffer* pBuf = NULL;
    IHXSockAddr* pFromAddr = NULL;

    switch (uEvent)
    {
    case HX_SOCK_EVENT_READ:

        // Packets can come from any port on the client, but they
        // have to come from the correct address, otherwise drop.
        if (SUCCEEDED(m_pUDPSocket->ReadFrom(&pBuf, &pFromAddr))
        &&  m_pPeerAddr 
        &&  pFromAddr
        &&  m_pPeerAddr->IsEqualAddr(pFromAddr))
        {
            handlePacket(pBuf);
        }

        HX_RELEASE(pFromAddr);
        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
        HX_ASSERT(FALSE);
        break;
    default:
        HX_ASSERT(FALSE);
    }

    return HXR_OK;
}

HX_RESULT
ServerRTCPUDPTransport::init(IUnknown* pContext,
                    IHXSocket* pSocket,
                    ServerRTPBaseTransport* pDataTransport,
                    IHXRTSPTransportResponse* pResp,
                    UINT16 streamNumber)
{
    m_pUDPSocket = pSocket;
    m_pUDPSocket->AddRef();

    m_pDataTransport = pDataTransport;
    m_pDataTransport->AddRef();

    m_pResp = pResp;
    pResp->AddRef();

    m_streamNumber = streamNumber;

    /* Set DiffServ Code Point */
    IHXQoSDiffServConfigurator* pCfg = NULL;
    if (SUCCEEDED(pContext->QueryInterface(IID_IHXQoSDiffServConfigurator, (void**)&pCfg)))
    {
        pCfg->ConfigureSocket(m_pUDPSocket, HX_QOS_DIFFSERV_CLASS_CONTROL);
        HX_RELEASE(pCfg);
    }



    HX_RESULT theErr = Init(pContext);
    if(HXR_OK != theErr)
    {
        return theErr;
    }

    ServerRTCPBaseTransport::init();
    ServerRTCPBaseTransport::getProtocolOverhead(m_pUDPSocket);
    if (HXR_OK == theErr)
    {
        HX_ASSERT(m_pUDPSocket);
        theErr = m_pUDPSocket->SetResponse(this);
    }
    if (HXR_OK == theErr)
    {
        theErr = m_pUDPSocket->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
    }

    return theErr;
}

/*
 *   RTP UDP
 */

RTSPTransportTypeEnum
ServerRTPUDPTransport::tag(void)
{
    return RTSP_TR_RTP_UDP;
}

void
ServerRTPUDPTransport::Done(void)
{
    if (m_pMulticastSocket && m_pMulticastAddr)
    {
        m_pMulticastSocket->LeaveGroup(m_pMulticastAddr, NULL);
    }
    HX_RELEASE(m_pMulticastSocket);
    HX_RELEASE(m_pMulticastAddr);
    HX_RELEASE(m_pPeerAddr);
    if (m_pUDPSocket)
    {
        /** If this transport is marked as an aggregate the socket can be 
          * re-used and it is up to the owner to call the sockets Close()
          * method. */
        if (!m_bIsAggregate)
        {
            m_pUDPSocket->Close();
        } 
        HX_RELEASE(m_pUDPSocket);
    }
    ServerRTPBaseTransport::Done();
}

void
ServerRTPUDPTransport::setPeerAddr(IHXSockAddr* pAddr)
{
    HX_ASSERT(pAddr != NULL && m_pPeerAddr == NULL);
    m_pPeerAddr = pAddr;
    m_pPeerAddr->AddRef();
}

void
ServerRTPUDPTransport::JoinMulticast(IHXSockAddr* pAddr, IHXSocket* pSocket)
{
    HX_ASSERT(pAddr);
    if (m_pMulticastAddr)
    {
        HX_ASSERT(m_pMulticastSocket);
        m_pMulticastSocket->LeaveGroup(m_pMulticastAddr, NULL);
        HX_RELEASE(m_pMulticastAddr);
    }
    else
    {
        if (pSocket)
        {
            pSocket->QueryInterface(IID_IHXMulticastSocket,
                                    (void**)&m_pMulticastSocket);
            HX_ASSERT(m_pMulticastSocket != NULL);
        }
        else
        {
            m_pUDPSocket->QueryInterface(IID_IHXMulticastSocket,
                                         (void**)&m_pMulticastSocket);
            HX_ASSERT(m_pMulticastSocket != NULL);
        }
    }

    m_pMulticastAddr = pAddr;
    m_pMulticastAddr->AddRef();
    m_bMulticast = TRUE;

    m_pMulticastSocket->JoinGroup(m_pMulticastAddr, NULL);

    if (m_pStreamHandler)
    {
        RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

        ASSERT(pStreamData);

        while(pStreamData)
        {
            pStreamData->m_pTransportBuffer->SetMulticast();
            pStreamData = m_pStreamHandler->nextStreamData();
        }
    }

}

HX_RESULT
ServerRTPUDPTransport::writePacket(IHXBuffer* pSendBuffer)
{
    if (!m_pUDPSocket)
        return HXR_FAIL;

    HX_ASSERT(m_pPeerAddr);
    return m_pUDPSocket->WriteTo(pSendBuffer, m_pPeerAddr);
}


/*
 * XXXMC
 * Special-case handling for PV clients
 */
HX_RESULT
ServerRTPUDPTransport::sendPVHandshakeResponse(UINT8* pPktPayload)
{
    IHXBuffer* pPktPayloadBuff = NULL;
    m_pCommonClassFactory->CreateInstance(IID_IHXBuffer, (void**) &pPktPayloadBuff);
    if (pPktPayloadBuff)
    {
        DPRINTF(D_INFO, ("RTP: Sending POKE PKT RESPONSE\n"));
        pPktPayloadBuff->Set((UCHAR*)pPktPayload, 8);
        writePacket(pPktPayloadBuff);
        pPktPayloadBuff->Release();
    }
    return HXR_OK;
}

HX_RESULT
ServerRTPUDPTransport::sendPacket(BasePacket* pPacket)
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
            m_pRTCPTran->m_bSendRTCP && m_bFirstTSSet)
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
 *  RTCP UDP
 */

void
ServerRTCPUDPTransport::Done()
{
    if (m_bSendBye)
    {
        sendBye();
    }

    if (m_pMulticastAddr)
    {
        m_pMulticastSocket->LeaveGroup(m_pMulticastAddr, NULL);
        HX_RELEASE(m_pMulticastAddr);
        HX_RELEASE(m_pMulticastSocket);
    }
    if (m_pUDPSocket)
    {
        /** If this transport is marked as an aggregate the socket can be
          * re-used and it is up to the owner to call the sockets Close()
          * method. */
        if (!m_bIsAggregate)
        {
            m_pUDPSocket->Close();
        }

        HX_RELEASE(m_pUDPSocket);
    }
    HX_RELEASE(m_pDataTransport);    
    HX_RELEASE(m_pPeerAddr);

    ServerRTCPBaseTransport::Done();
}

RTSPTransportTypeEnum
ServerRTCPUDPTransport::tag()
{
    return RTSP_TR_RTCP;
}

void
ServerRTCPUDPTransport::setPeerAddr(IHXSockAddr* pAddr)
{
    HX_ASSERT(pAddr != NULL && m_pPeerAddr == NULL);
    m_pPeerAddr = pAddr;
    m_pPeerAddr->AddRef();

    //  must be odd port
    HX_ASSERT(1 == m_pPeerAddr->GetPort() % 2);

    HXLOGL3(HXLOG_RTSP, "ServerRTCPUDPTransport[%p]::setPeerAddr(): port = %u",this, m_pPeerAddr->GetPort());
}

void
ServerRTCPUDPTransport::JoinMulticast(IHXSockAddr* pAddr, IHXSocket* pSocket)
{
    HXLOGL3(HXLOG_RTSP, "ServerRTCPUDPTransport[%p]::JoinMulticast(): port = %u",this, pAddr->GetPort());

    HX_ASSERT(pAddr);
    if (m_pMulticastAddr)
    {
        HX_ASSERT(m_pMulticastSocket);
        m_pMulticastSocket->LeaveGroup(m_pMulticastAddr, NULL);
        HX_RELEASE(m_pMulticastAddr);
    }
    else
    {
        if (pSocket)
        {
            pSocket->QueryInterface(IID_IHXMulticastSocket,
                                    (void**)&m_pMulticastSocket);
            HX_ASSERT(m_pMulticastSocket != NULL);
        }
        else
        {
            m_pUDPSocket->QueryInterface(IID_IHXMulticastSocket,
                                         (void**)&m_pMulticastSocket);
            HX_ASSERT(m_pMulticastSocket != NULL);
        }
    }

    m_pMulticastAddr = pAddr;
    m_pMulticastAddr->AddRef();
    m_bMulticast = TRUE;

    m_pMulticastSocket->JoinGroup(m_pMulticastAddr, NULL);

    if (m_pStreamHandler)
    {
        RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

        ASSERT(pStreamData);

        while(pStreamData)
        {
            pStreamData->m_pTransportBuffer->SetMulticast();
            pStreamData = m_pStreamHandler->nextStreamData();
        }
    }
}

HX_RESULT
ServerRTCPUDPTransport::streamDone(UINT16 streamNumber,
                             UINT32 uReasonCode /* = 0 */,
                             const char* pReasonText /* = NULL */)
{
    HXLOGL3(HXLOG_RTSP, "ServerRTCPUDPTransport[%p]::streamDone(): str = %u; reason = %lu",this, streamNumber, uReasonCode);

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


/*
 *  We don't really konw what this RTCP pkt is...Simply reflect.
 */
HX_RESULT
ServerRTCPUDPTransport::reflectRTCP(IHXBuffer* pSendBuf)
{
    HX_ASSERT(pSendBuf);
    HX_ASSERT(m_pPeerAddr);
    return m_pUDPSocket->WriteTo(pSendBuf, m_pPeerAddr);
}

HX_RESULT
ServerRTCPUDPTransport::sendSenderReport()
{
    HX_ASSERT(m_pPeerAddr); 
    if (!m_pPeerAddr)
    {
        return HXR_FAIL;
    }

    IHXBuffer* pSendBuf = NULL;
    HX_RESULT theErr = makeSenderReport(pSendBuf);
    if (HXR_OK == theErr)
    {
        HX_ASSERT(pSendBuf);
        theErr = m_pUDPSocket->WriteTo(pSendBuf, m_pPeerAddr);
        HX_ASSERT(HXR_OK == theErr);
    }

    HX_RELEASE(pSendBuf);
    return theErr;
}

HX_RESULT
ServerRTCPUDPTransport::sendBye()
{
    HX_RESULT theErr = HXR_OK;

    IHXSockAddr* pDestAddr = m_pMulticastAddr ? m_pMulticastAddr : m_pPeerAddr;
    if (pDestAddr)
    {
        IHXBuffer* pSendBuf = NULL;
        theErr = makeBye(pSendBuf);
        if (HXR_OK == theErr)
        {
            HX_ASSERT(pSendBuf);
            // we don't want this to get lost since a client will request for
            // TEARDOWN upon a reception of this report.
            for (UINT32 i = 0; i < 5 && HXR_OK == theErr; i++)
            {
                if (FAILED(theErr = m_pUDPSocket->WriteTo(pSendBuf, pDestAddr)))
                {
                    break;
                }

            }
        }

        HX_RELEASE(pSendBuf);
    }
    return theErr;
}

