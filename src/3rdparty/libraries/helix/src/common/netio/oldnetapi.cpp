/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "hxtypes.h"
#include "hxassert.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxnet.h"
#include "hxbuffer.h"
#include "hxsbuffer.h"
#include "oldnetapi.h"
#include "hxinetaddr.h"
#include "sockaddrimp.h"    //XXXTDM remove this, it's only for CHXAddrInfo
#include "pckunpck.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXOldListenSocket::CHXOldListenSocket(IUnknown* punkContext) :
    m_nRefCount(0),
    m_pCCF(NULL),
    m_pNetSvc(NULL),
    m_pResponse(NULL),
    m_pSock(NULL)
{
    punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    punkContext->QueryInterface(IID_IHXNetServices, (void**)&m_pNetSvc);
}

CHXOldListenSocket::~CHXOldListenSocket(void)
{
    HX_RELEASE(m_pSock);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pCCF);
}

STDMETHODIMP
CHXOldListenSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXListenSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXListenSocket))
    {
        AddRef();
        *ppvObj = (IHXListenSocket*)this;
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
CHXOldListenSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXOldListenSocket::Release(void)
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
CHXOldListenSocket::Init(UINT32 uAddr, UINT16 uPort,
        IHXListenResponse* pResponse)
{
    HX_RELEASE(m_pResponse);
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);

    HX_RESULT hxr = HXR_OK;
    IHXBuffer* pAddrBuf = NULL;
    IHXSockAddr* pAddr = NULL;

    hxr = m_pNetSvc->CreateSocket(&m_pSock);
    if (hxr == HXR_OK)
    {
        hxr = m_pSock->Init(HX_SOCK_FAMILY_IN4,
                        HX_SOCK_TYPE_TCP,
                        HX_SOCK_PROTO_ANY);
    }
    if (hxr == HXR_OK)
    {
        hxr = m_pSock->SetResponse(this);
    }
    if (hxr == HXR_OK)
    {
        hxr = m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
    }
    if (hxr == HXR_OK)
    {
        hxr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pAddrBuf);
    }
    if (hxr == HXR_OK)
    {
        //XXXTDM: Are uAddr and uPort net or host order?
        char szAddr[HX_ADDRSTRLEN_IN4];
        in_addr addr;
        addr.s_addr = uAddr;
        strcpy(szAddr, inet_ntoa(addr));
        pAddrBuf->Set((UCHAR*)szAddr, strlen(szAddr)+1);
        pAddr->SetAddr(pAddrBuf);
        pAddr->SetPort(uPort);
        hxr = m_pSock->Bind(pAddr);
    }
    if (hxr == HXR_OK)
    {
        hxr = m_pSock->Listen(SOMAXCONN);
    }
    if (hxr == HXR_OK)
    {
        hxr = m_pSock->SelectEvents(HX_SOCK_EVENT_ACCEPT|HX_SOCK_EVENT_CLOSE);
    }

    if (hxr != HXR_OK)
    {
        HX_RELEASE(m_pSock);
        HX_RELEASE(m_pResponse);
    }
    HX_RELEASE(pAddrBuf);
    HX_RELEASE(pAddr);

    return hxr;
}

STDMETHODIMP
CHXOldListenSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    switch (uEvent)
    {
    case HX_SOCK_EVENT_ACCEPT:
        DoAccept();
        break;
    case HX_SOCK_EVENT_CLOSE:
        DoClose();
        break;
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_OK;
}

void
CHXOldListenSocket::DoAccept(void)
{
    IHXSocket* pNewSock = NULL;
    IHXSockAddr* pSource = NULL;
    if (m_pSock->Accept(&pNewSock, &pSource) == HXR_OK)
    {
        IHXTCPSocket* pNewTcpSock = new CHXOldTCPSocket(m_pNetSvc, pNewSock);
        pNewTcpSock->AddRef();
        m_pResponse->NewConnection(HXR_OK, pNewTcpSock);
        pSource->Release();
        pNewTcpSock->Release();
    }
    HX_RELEASE(pSource);
    HX_RELEASE(pNewSock);
}

void CHXOldListenSocket::DoClose(void)
{
    m_pSock->SelectEvents(HX_SOCK_EVENT_NONE);
    m_pSock->Close();
    if (m_pResponse != NULL)
    {
        m_pResponse->NewConnection(HXR_FAIL, NULL);
    }
}

CHXOldResolver::CHXOldResolver(IUnknown* punkContext) :
    m_nRefCount(0),
    m_pCCF(NULL),
    m_pNetSvc(NULL),
    m_pResponse(NULL),
    m_pResolve(NULL)
{
    punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    punkContext->QueryInterface(IID_IHXNetServices, (void**)&m_pNetSvc);
}

CHXOldResolver::~CHXOldResolver(void)
{
    HX_RELEASE(m_pResolve);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pCCF);
}

STDMETHODIMP
CHXOldResolver::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXResolver*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXResolver))
    {
        AddRef();
        *ppvObj = (IHXResolver*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXResolverResponse))
    {
        AddRef();
        *ppvObj = (IHXResolverResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXOldResolver::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXOldResolver::Release(void)
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
CHXOldResolver::Init(IHXResolverResponse* pResponse)
{
    m_pResponse = pResponse;
    if (m_pResponse != NULL)
    {
        m_pResponse->AddRef();
    }

    m_pNetSvc->CreateResolver(&m_pResolve);

    return HXR_OK;
}

STDMETHODIMP
CHXOldResolver::GetHostByName(const char* pHost)
{
    HX_RESULT hxr;
    IHXAddrInfo* pHints = NULL; /* XXX new CHXAddrInfo(); */
    pHints->AddRef();
    pHints->SetFamily(HX_ADDRFAMILY_IN4);
    hxr = m_pResolve->GetAddrInfo(pHost, NULL, pHints);
    return hxr;
}

STDMETHODIMP
CHXOldResolver::GetAddrInfoDone(HX_RESULT status,
        UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    if (status == HXR_OK && nVecLen > 0)
    {
        if (ppAddrVec[0]->GetFamily() == HX_SOCK_FAMILY_IN4)
        {
            IHXSockAddrNative* pNative = NULL;
            sockaddr_in* psa;
            size_t salen;
            ppAddrVec[0]->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
            HX_ASSERT(pNative != NULL);
            pNative->Get((sockaddr**)&psa, &salen);
            HX_ASSERT(salen == sizeof(sockaddr_in));
            //XXX: endianness?
            m_pResponse->GetHostByNameDone(HXR_OK, psa->sin_addr.s_addr);
            HX_RELEASE(pNative);
            return HXR_OK;
        }
        // We should not get here
        HX_ASSERT(FALSE);
    }

    m_pResponse->GetHostByNameDone(HXR_FAIL, 0);
    return HXR_OK;
}

STDMETHODIMP
CHXOldResolver::GetNameInfoDone(HX_RESULT status,
                const char* pNode, const char* pService)
{
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

CHXOldTCPSocket::CHXOldTCPSocket(IUnknown* punkContext, IHXSocket* pSock) :
    m_nRefCount(0),
    m_pCCF(NULL),
    m_pNetSvc(NULL),
    m_pResponse(NULL),
    m_pResolve(NULL),
    m_pSock(NULL),
    m_uEvents(HX_SOCK_EVENT_NONE),
    m_uReadSize(0),
    m_pReadBuf(NULL)
{
    punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    punkContext->QueryInterface(IID_IHXNetServices, (void**)&m_pNetSvc);
    m_pSock = pSock;
    m_pSock->AddRef();
}

CHXOldTCPSocket::~CHXOldTCPSocket(void)
{
    HX_RELEASE(m_pReadBuf);
    HX_RELEASE(m_pSock);
    HX_RELEASE(m_pResolve);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pCCF);
}

STDMETHODIMP
CHXOldTCPSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXTCPSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXTCPSocket))
    {
        AddRef();
        *ppvObj = (IHXTCPSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXResolveResponse))
    {
        AddRef();
        *ppvObj = (IHXResolveResponse*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXWouldBlock))
    {
        AddRef();
        *ppvObj = (IHXWouldBlock*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSetSocketOption))
    {
        AddRef();
        *ppvObj = (IHXSetSocketOption*)this;
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
CHXOldTCPSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXOldTCPSocket::Release(void)
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
CHXOldTCPSocket::Init(IHXTCPResponse* pResponse)
{
    SetResponse(pResponse);
    return m_pSock->Init(HX_SOCK_FAMILY_IN4,
                         HX_SOCK_TYPE_TCP,
                         HX_SOCK_PROTO_ANY);
}

STDMETHODIMP
CHXOldTCPSocket::SetResponse(IHXTCPResponse* pResponse)
{
    HX_RELEASE(m_pResponse);
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::Bind(UINT32 uAddr, UINT16 uPort)
{
    HX_RESULT hxr;
    IHXSockAddr* pAddr = NULL;
    IHXSockAddrNative* pNative = NULL;
    sockaddr_in* psa;
    size_t salen;

    m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
    pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    pNative->Get((sockaddr**)&psa, &salen);
    psa->sin_addr.s_addr = hx_htonl(uAddr);
    psa->sin_port = hx_htons(uPort);
    hxr = m_pSock->Bind(pAddr);
    HX_RELEASE(pNative);
    HX_RELEASE(pAddr);

    return hxr;
}

STDMETHODIMP
CHXOldTCPSocket::Connect(const char* pHost, UINT16 uPort)
{
    HX_RESULT hxr;
    char szPort[5+1];

    if (m_pResolve != NULL)
    {
        return HXR_UNEXPECTED;
    }

    sprintf(szPort, "%hu", uPort);

    m_pNetSvc->CreateResolver(&m_pResolve);
    m_pResolve->Init(this);

    hxr = m_pResolve->GetAddrInfo(pHost, szPort, NULL /*XXX: IPv4 only*/);

    return hxr;
}

STDMETHODIMP
CHXOldTCPSocket::Read(UINT16 uLen)
{
    // Check for recursive read
    if (m_uReadSize != 0)
    {
        return HXR_UNEXPECTED;
    }

    // If there is a pending buffer, use it
    if (m_pReadBuf != NULL)
    {
        IHXBuffer* pBuf = NULL;
        if (m_pReadBuf->GetSize() <= uLen)
        {
            pBuf = m_pReadBuf;
            m_pReadBuf = NULL;
        }
        else
        {
            IHXBuffer* pFrag = NULL;
	    if (HXR_OK == CreateBufferCCF(pFrag, m_pCCF))
	    {
		pFrag->SetSize(m_pReadBuf->GetSize() - uLen);
		memcpy(pFrag->GetBuffer(),
		       m_pReadBuf->GetBuffer(),
                       m_pReadBuf->GetSize() - uLen);
		pBuf = new CHXStaticBuffer(m_pReadBuf, 0, uLen);
		pBuf->AddRef();
		HX_RELEASE(m_pReadBuf);
		m_pReadBuf = pFrag;
	    }
        }
        m_pResponse->ReadDone(HXR_OK, pBuf);
        HX_RELEASE(pBuf);
        return HXR_OK;
    }

    m_uReadSize = uLen;
    m_uEvents |= HX_SOCK_EVENT_READ;
    m_pSock->SelectEvents(m_uEvents);
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::Write(IHXBuffer* pBuf)
{
    HX_RESULT hxr = m_pSock->Write(pBuf);
    if (hxr == HXR_SOCK_WOULDBLOCK)
    {
        if (m_pWouldBlockResponse != NULL)
        {
            m_pWouldBlockResponse->WouldBlock(m_uWouldBlockId);
            m_uEvents |= HX_SOCK_EVENT_WRITE;
            m_pSock->SelectEvents(m_uEvents);
        }
    }
    return hxr;
}

STDMETHODIMP
CHXOldTCPSocket::WantWrite(void)
{
    m_bWantWrite = TRUE;
    if (!(m_uEvents & HX_SOCK_EVENT_WRITE))
    {
        m_uEvents |= HX_SOCK_EVENT_WRITE;
        m_pSock->SelectEvents(m_uEvents);
    }
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::GetForeignAddress(REF(ULONG32) uAddr)
{
    IHXSockAddr* pAddr = NULL;
    IHXSockAddrNative* pNative = NULL;
    sockaddr_in* psa;
    size_t salen;

    m_pSock->GetPeerAddr(&pAddr);
    pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    pNative->Get((sockaddr**)&psa, &salen);
    uAddr = hx_ntohl(psa->sin_addr.s_addr);
    HX_RELEASE(pNative);
    HX_RELEASE(pAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::GetLocalAddress(REF(ULONG32) uAddr)
{
    IHXSockAddr* pAddr = NULL;
    IHXSockAddrNative* pNative = NULL;
    sockaddr_in* psa;
    size_t salen;

    m_pSock->GetLocalAddr(&pAddr);
    pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    pNative->Get((sockaddr**)&psa, &salen);
    uAddr = hx_ntohl(psa->sin_addr.s_addr);
    HX_RELEASE(pNative);
    HX_RELEASE(pAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::GetForeignPort(REF(UINT16) uPort)
{
    IHXSockAddr* pAddr = NULL;
    m_pSock->GetPeerAddr(&pAddr);
    uPort = pAddr->GetPort();
    HX_RELEASE(pAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::GetLocalPort(REF(UINT16) uPort)
{
    IHXSockAddr* pAddr = NULL;
    m_pSock->GetLocalAddr(&pAddr);
    uPort = pAddr->GetPort();
    HX_RELEASE(pAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::GetAddrInfoDone(HX_RESULT status,
                UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    if (FAILED(status))
    {
        m_pResponse->ConnectDone(status);
        return HXR_OK;
    }
    m_pSock->ConnectToAny(nVecLen, ppAddrVec);
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::GetNameInfoDone(HX_RESULT status,
                const char* pNode, const char* pService)
{
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CHXOldTCPSocket::WantWouldBlock(IHXWouldBlockResponse* pResponse, UINT32 id)
{
    HX_RELEASE(m_pWouldBlockResponse);
    m_pWouldBlockResponse = pResponse;
    HX_ADDREF(m_pWouldBlockResponse);
    m_uWouldBlockId = id;
    return HXR_OK;
}

STDMETHODIMP
CHXOldTCPSocket::SetOption(HX_SOCKET_OPTION opt, UINT32 uVal)
{
    HX_RESULT hxr = HXR_UNEXPECTED;
    switch (opt)
    {
    case HX_SOCKOPT_REUSE_ADDR:
        hxr = m_pSock->SetOption(HX_SOCKOPT_REUSEADDR, uVal);
        break;
    case HX_SOCKOPT_REUSE_PORT:
        HX_ASSERT(FALSE); //XXXTDM: fixme
        break;
    case HX_SOCKOPT_BROADCAST:
        hxr = m_pSock->SetOption(HX_SOCKOPT_BCAST, uVal);
        break;
    case HX_SOCKOPT_SET_RECVBUF_SIZE:
        hxr = m_pSock->SetOption(HX_SOCKOPT_BCAST, uVal);
        break;
    case HX_SOCKOPT_SET_SENDBUF_SIZE:
        hxr = m_pSock->SetOption(HX_SOCKOPT_SNDBUF, uVal);
        break;
    case HX_SOCKOPT_MULTICAST_IF:
        hxr = m_pSock->SetOption(HX_SOCKOPT_IN4_MULTICAST_IF, uVal);
        break;
    case HX_SOCKOPT_IP_TOS:
        hxr = m_pSock->SetOption(HX_SOCKOPT_IN4_TOS, uVal);
        break;
    default:
        HX_ASSERT(FALSE);
    }
    return hxr;
}

STDMETHODIMP
CHXOldTCPSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    switch (uEvent)
    {
    case HX_SOCK_EVENT_CONNECT:
        DoConnect(status);
        break;
    case HX_SOCK_EVENT_READ:
        DoRead();
        break;
    case HX_SOCK_EVENT_WRITE:
        DoWrite();
        break;
    case HX_SOCK_EVENT_CLOSE:
        DoClose();
        break;
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_OK;
}

void
CHXOldTCPSocket::DoConnect(HX_RESULT status)
{
    m_pResponse->ConnectDone(status);
}

void
CHXOldTCPSocket::DoRead(void)
{
    HX_ASSERT(m_pReadBuf == NULL);

    m_uEvents &= ~HX_SOCK_EVENT_READ;
    if (FAILED(m_pSock->Read(&m_pReadBuf)))
    {
        m_pResponse->ReadDone(HXR_FAIL, NULL);
        return;
    }

    IHXBuffer* pBuf = NULL;
    if (m_pReadBuf->GetSize() <= m_uReadSize)
    {
        pBuf = m_pReadBuf;
        m_pReadBuf = NULL;
    }
    else
    {
        IHXBuffer* pFrag = NULL;
	if (HXR_OK == CreateBufferCCF(pFrag, m_pCCF))
	{
	    pFrag->SetSize(m_pReadBuf->GetSize() - m_uReadSize);
	    memcpy(pFrag->GetBuffer(),
		m_pReadBuf->GetBuffer(),
		m_pReadBuf->GetSize() - m_uReadSize);
	    pBuf = new CHXStaticBuffer(m_pReadBuf, 0, m_uReadSize);
	    pBuf->AddRef();
	    HX_RELEASE(m_pReadBuf);
	    m_pReadBuf = pFrag;
	}
    }
    m_pResponse->ReadDone(HXR_OK, pBuf);
    HX_RELEASE(pBuf);
}

void
CHXOldTCPSocket::DoWrite(void)
{
    if (m_uEvents & HX_SOCK_EVENT_WRITE)
    {
        if (m_bWantWrite)
        {
            m_bWantWrite = FALSE;
            m_pResponse->WriteReady(HXR_OK);
        }
        if (m_pWouldBlockResponse != NULL)
        {
            m_pWouldBlockResponse->WouldBlockCleared(m_uWouldBlockId);
        }
    }
    else
    {
        m_pSock->SelectEvents(m_uEvents);
    }
}

void
CHXOldTCPSocket::DoClose(void)
{
    if (m_uReadSize != 0)
    {
        m_pResponse->ReadDone(HXR_CLOSED, NULL);
    }
    HX_ASSERT(FALSE); // Release stuff
}

CHXOldUDPSocket::CHXOldUDPSocket(IHXNetServices* pNetSvc) :
    m_nRefCount(0),
    m_pNetSvc(NULL),
    m_pResponse(NULL),
    m_pSock(NULL),
    m_uEvents(0),
    m_uReadSize(0)
{
    m_pNetSvc = pNetSvc;
    m_pNetSvc->AddRef();
}

CHXOldUDPSocket::~CHXOldUDPSocket(void)
{
    HX_ASSERT(FALSE); //XXX: Release stuff
}

STDMETHODIMP
CHXOldUDPSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXUDPSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXUDPSocket))
    {
        AddRef();
        *ppvObj = (IHXUDPSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSetSocketOption))
    {
        AddRef();
        *ppvObj = (IHXSetSocketOption*)this;
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
CHXOldUDPSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXOldUDPSocket::Release(void)
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
CHXOldUDPSocket::Init(ULONG32 uAddr, UINT16 uPort, IHXUDPResponse* pResponse)
{
    HX_RESULT hxr = Bind(uAddr, uPort);
    if (hxr == HXR_OK)
    {
        HX_RELEASE(m_pResponse);
        m_pResponse = pResponse;
        if (m_pResponse != NULL)
        {
            m_pResponse->AddRef();
        }
    }
    return hxr;
}

STDMETHODIMP
CHXOldUDPSocket::Bind(UINT32 uAddr, UINT16 uPort)
{
    HX_RESULT hxr;
    IHXSockAddr* pAddr = NULL;
    IHXSockAddrNative* pNative = NULL;
    sockaddr_in* psa;
    size_t salen;

    m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
    pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    pNative->Get((sockaddr**)&psa, &salen);
    psa->sin_addr.s_addr = hx_htonl(uAddr);
    psa->sin_port = hx_htons(uPort);
    hxr = m_pSock->Bind(pAddr);
    HX_RELEASE(pNative);
    HX_RELEASE(pAddr);
    return hxr;
}

STDMETHODIMP
CHXOldUDPSocket::Read(UINT16 uLen)
{
    // Check for recursive read
    if (m_uReadSize != 0)
    {
        return HXR_UNEXPECTED;
    }

    m_uReadSize = uLen;
    m_uEvents |= HX_SOCK_EVENT_READ;
    m_pSock->SelectEvents(m_uEvents);
    return HXR_OK;
}

STDMETHODIMP
CHXOldUDPSocket::Write(IHXBuffer* pBuf)
{
    return m_pSock->Write(pBuf);
}

STDMETHODIMP
CHXOldUDPSocket::WriteTo(ULONG32 uAddr, UINT16 uPort, IHXBuffer* pBuf)
{
    HX_RESULT hxr;
    IHXSockAddr* pAddr = NULL;
    IHXSockAddrNative* pNative = NULL;
    sockaddr_in* psa;
    size_t salen;

    m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
    pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    pNative->Get((sockaddr**)&psa, &salen);
    psa->sin_addr.s_addr = hx_htonl(uAddr);
    psa->sin_port = hx_htons(uPort);

    hxr = m_pSock->WriteTo(pBuf, pAddr);
    HX_RELEASE(pNative);
    HX_RELEASE(pAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXOldUDPSocket::GetLocalPort(REF(UINT16) uPort)
{
    IHXSockAddr* pAddr = NULL;
    m_pSock->GetLocalAddr(&pAddr);
    uPort = pAddr->GetPort();
    HX_RELEASE(pAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXOldUDPSocket::JoinMulticastGroup(ULONG32 uAddr, ULONG32 uIfAddr)
{
    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP
CHXOldUDPSocket::LeaveMulticastGroup(ULONG32 uAddr, ULONG32 uIfAddr)
{
    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP
CHXOldUDPSocket::SetOption(HX_SOCKET_OPTION opt, UINT32 uVal)
{
    HX_RESULT hxr = HXR_UNEXPECTED;
    switch (opt)
    {
    case HX_SOCKOPT_REUSE_ADDR:
        hxr = m_pSock->SetOption(HX_SOCKOPT_REUSEADDR, uVal);
        break;
    case HX_SOCKOPT_REUSE_PORT:
        HX_ASSERT(FALSE); //XXXTDM: fixme
        break;
    case HX_SOCKOPT_BROADCAST:
        hxr = m_pSock->SetOption(HX_SOCKOPT_BCAST, uVal);
        break;
    case HX_SOCKOPT_SET_RECVBUF_SIZE:
        hxr = m_pSock->SetOption(HX_SOCKOPT_BCAST, uVal);
        break;
    case HX_SOCKOPT_SET_SENDBUF_SIZE:
        hxr = m_pSock->SetOption(HX_SOCKOPT_SNDBUF, uVal);
        break;
    case HX_SOCKOPT_MULTICAST_IF:
        hxr = m_pSock->SetOption(HX_SOCKOPT_IN4_MULTICAST_IF, uVal);
        break;
    case HX_SOCKOPT_IP_TOS:
        hxr = m_pSock->SetOption(HX_SOCKOPT_IN4_TOS, uVal);
        break;
    default:
        HX_ASSERT(FALSE);
    }
    return hxr;
}

STDMETHODIMP
CHXOldUDPSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    switch (uEvent)
    {
    case HX_SOCK_EVENT_READ:
        DoRead();
        break;
    case HX_SOCK_EVENT_WRITE:
        DoWrite();
        break;
    case HX_SOCK_EVENT_CLOSE:
        DoClose();
        break;
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_OK;
}

void
CHXOldUDPSocket::DoRead(void)
{
    IHXBuffer* pBuf = NULL;
    IHXSockAddr* pAddr = NULL;
    IHXSockAddrNative* pNative = NULL;
    sockaddr_in* psa;
    size_t salen;
    UINT32 uAddr;
    UINT16 uPort;

    // Server never stops reading after the initial call to Read()
#if !defined(HELIX_FEATURE_SERVER)
    m_uEvents &= ~HX_SOCK_EVENT_READ;
#endif
    if (FAILED(m_pSock->ReadFrom(&pBuf, &pAddr)))
    {
        m_pResponse->ReadDone(HXR_FAIL, NULL, 0, 0);
        return;
    }

    // NB: We don't save datagram fragments, they are lost
    if (pBuf->GetSize() > m_uReadSize)
    {
        pBuf->SetSize(m_uReadSize);
    }

    pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    pNative->Get((sockaddr**)&psa, &salen);
    uAddr = hx_ntohl(psa->sin_addr.s_addr);
    uPort = hx_ntohs(psa->sin_port);
    m_pResponse->ReadDone(HXR_OK, pBuf, uAddr, uPort);
    HX_RELEASE(pNative);
    HX_RELEASE(pAddr);
    HX_RELEASE(pBuf);
}

void
CHXOldUDPSocket::DoWrite(void)
{
    HX_ASSERT(FALSE); // Implement me
}

void
CHXOldUDPSocket::DoClose(void)
{
    if (m_uReadSize != 0)
    {
        m_pResponse->ReadDone(HXR_CLOSED, NULL, 0, 0);
    }
    HX_ASSERT(FALSE); // Release stuff
}
